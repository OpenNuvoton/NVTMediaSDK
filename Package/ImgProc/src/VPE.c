/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/
#if defined (__GNUC__)
#define _POSIX_SOURCE
#define _MODE_T_DECLARED
#define _TIMER_T_DECLARED
#define _CLOCKID_T_DECLARED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "wblib.h"
#include "N9H26_VPE.h"

#include "VPE.h"
#include "NVTMedia_SAL_OS.h"

#define MAX_TRANS_TIMEOUT		1000		//1s

typedef uint32_t __u32;

typedef struct vpe_transform{
	__u32	src_addrPacY;
	__u32	src_addrU;
	__u32	src_addrV;
	__u32	src_format;
	__u32	src_width;
	__u32	src_height;	
	__u32	src_leftoffset;
	__u32	src_rightoffset;	

	__u32	dest_addrPac;
	__u32	dest_format;
	__u32	dest_width;
	__u32	dest_height;
	__u32	dest_leftoffset;
	__u32	dest_rightoffset;

	__u32	algorithm;
	E_VPE_CMD_OP	rotation;
	
} vpe_transform_t;

static BOOL s_bIsVPECompleteInt = FALSE; 
//static BOOL s_bIsVPEScatterGatherOnePieceInt = FALSE;
static BOOL s_bIsVPEBlockInt = FALSE;
static BOOL s_bIsVPEBlockErrorInt = FALSE;
static BOOL s_bIsVPEDMAErrorInt = FALSE;
static uint32_t s_u32CompletCount = 0;
static pthread_mutex_t *s_ptVPEEngMutex = NULL;


static void vpeCompleteCallback(void)
{
	s_u32CompletCount= s_u32CompletCount+1;
	 s_bIsVPECompleteInt = TRUE;
}	

static void vpeMacroBlockCallback(void)
{
	 s_bIsVPEBlockInt = TRUE;
}

static void vpeMacroBlockErrorCallback(void)
{
	 s_bIsVPEBlockErrorInt = TRUE;
}

static void vpeDmaErrorCallback(void)
{
	 s_bIsVPEDMAErrorInt = TRUE;
}

ERRCODE
VPE_Init(void)
{
	if(s_ptVPEEngMutex)		//Had been init
		return ERR_VPE_SUCCESS;

	s_ptVPEEngMutex = calloc(1, sizeof(pthread_mutex_t));
	
	if(s_ptVPEEngMutex == NULL)
		return ERR_VPE_MALLOC;
	
	if(pthread_mutex_init(s_ptVPEEngMutex, NULL) != 0){
		sysprintf("Failed to create VPE engine mutex \n");
		return ERR_VPE_INIT;
	}	
	
	return ERR_VPE_SUCCESS;
}

static int32_t 
VPE_OpenVPEDevice(void)
{
	PFN_VPE_CALLBACK OldVpeCallback;

	vpeOpen();	//Assigned VPE working clock to 48MHz. 

	vpeInstallCallback(VPE_INT_COMP,
						vpeCompleteCallback, 
						&OldVpeCallback);				

	/* For C&M and JPEG	*/			
	vpeInstallCallback(VPE_INT_MB_COMP,
						vpeMacroBlockCallback, 
						&OldVpeCallback);
				
	vpeInstallCallback(VPE_INT_MB_ERR,
						vpeMacroBlockErrorCallback, 
						&OldVpeCallback);										
	vpeInstallCallback(VPE_INT_DMA_ERR,
						vpeDmaErrorCallback, 
						&OldVpeCallback);
	vpeEnableInt(VPE_INT_COMP);				

	vpeEnableInt(VPE_INT_DMA_ERR);		

	return 0;
}

static ERRCODE
VPE_TransForm(
	vpe_transform_t *psVPESetting
)
{
	vpeIoctl(VPE_IOCTL_HOST_OP,
				VPE_HOST_FRAME_TURBO,
				psVPESetting->rotation,		
				0);

	vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
				(UINT32)psVPESetting->src_addrPacY,				// MMU on, the is virtual address, MMU off, the is physical address. 
				(UINT32)psVPESetting->src_addrU,	
				(UINT32)psVPESetting->src_addrV);

	vpeIoctl(VPE_IOCTL_SET_FMT,
				psVPESetting->src_format,	/* Src Format */
				psVPESetting->dest_format,	/* Dst Format */
				0);	

	vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,		
				(UINT32)psVPESetting->src_leftoffset,	/* Src Left offset */
				(UINT32)psVPESetting->src_rightoffset,	/* Src right offset */
				0);

	vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
				(UINT32)psVPESetting->dest_leftoffset,				/* Dst Left offset */
				(UINT32)psVPESetting->dest_rightoffset,				/* Dst right offset */
				0);

	vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
				psVPESetting->src_width,
				psVPESetting->src_height,
				0);

	vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
				//(UINT32)VPOSDISPLAYBUFADDR,
				(UINT32)psVPESetting->dest_addrPac,
				0,
				0);

	vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
				psVPESetting->dest_width,
				psVPESetting->dest_height,
				0);

	vpeIoctl(VPE_IOCTL_SET_COLOR_RANGE,
				FALSE,
				FALSE,
				0);

	vpeIoctl(VPE_IOCTL_SET_FILTER,
				psVPESetting->algorithm,
				0,
				0);

	vpeIoctl(VPE_IOCTL_TRIGGER,
				0,
				0,
				0);

	return 0;
}

ERRCODE
VPE_Trans(
	S_VPE_TRANS_ATTR *psVPETransAttr
)
{
	S_VPE_TRANS_PARAM *psTransParam = psVPETransAttr->psTransParam;
	uint32_t u32SrcAdrY;
	uint32_t u32SrcAdrU;
	uint32_t u32SrcAdrV;
	vpe_transform_t sVPESetting;
	
	if(psTransParam->u32SrcImgWidth <= 8){
		sysprintf("VPE trans failed! Source image width must be larger than 8 \n");
		return ERR_VPE_PARAM;
	}

	pthread_mutex_lock(s_ptVPEEngMutex);

	if(VPE_OpenVPEDevice() < 0){
		pthread_mutex_unlock(s_ptVPEEngMutex);
		return ERR_VPE_OPEN_DEV;
	}

	u32SrcAdrY = (uint32_t)psVPETransAttr->u32SrcBufPhyAdr;
	sVPESetting.dest_addrPac = psVPETransAttr->u32DestBufPhyAdr;
	
	uint32_t u32TotalSrcImgWidth;
	uint32_t u32TotalSrcImgHeight;
	u32TotalSrcImgWidth = psTransParam->u32SrcImgWidth + psTransParam->u32SrcRightOffset + psTransParam->u32SrcLeftOffset;
	u32TotalSrcImgHeight = psTransParam->u32SrcImgHeight + psTransParam->u32SrcTopOffset + psTransParam->u32SrcBottomOffset;

	if(psTransParam->eSrcFmt == VPE_SRC_PLANAR_YUV422){
		sVPESetting.src_addrPacY = u32SrcAdrY + (psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth);
//		u32SrcAdrU = u32SrcAdrY + (u32TotalSrcImgWidth * psTransParam->u32SrcImgHeight);
//		u32SrcAdrV = u32SrcAdrU + ((u32TotalSrcImgWidth * psTransParam->u32SrcImgHeight)/2);		
		u32SrcAdrU = u32SrcAdrY + (u32TotalSrcImgWidth * u32TotalSrcImgHeight);
		u32SrcAdrV = u32SrcAdrU + ((u32TotalSrcImgWidth * u32TotalSrcImgHeight)/2);		
		sVPESetting.src_addrU = u32SrcAdrU + ((psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth)/2);
		sVPESetting.src_addrV = u32SrcAdrV + ((psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth)/2);
	}
	else if(psTransParam->eSrcFmt == VPE_SRC_PLANAR_YUV420){
		sVPESetting.src_addrPacY = u32SrcAdrY + (psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth);
//		u32SrcAdrU = u32SrcAdrY + (u32TotalSrcImgWidth * psTransParam->u32SrcImgHeight);
//		u32SrcAdrV = u32SrcAdrU + ((u32TotalSrcImgWidth * psTransParam->u32SrcImgHeight)/4);		
		u32SrcAdrU = u32SrcAdrY + (u32TotalSrcImgWidth * u32TotalSrcImgHeight);
		u32SrcAdrV = u32SrcAdrU + ((u32TotalSrcImgWidth * u32TotalSrcImgHeight)/4);		
		sVPESetting.src_addrU = u32SrcAdrU + ((psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth)/4);
		sVPESetting.src_addrV = u32SrcAdrV + ((psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth)/4);
	}
	else if(psTransParam->eSrcFmt == VPE_SRC_PACKET_YUV422){
		sVPESetting.src_addrPacY = u32SrcAdrY + (psTransParam->u32SrcTopOffset * u32TotalSrcImgWidth * 2);
		sVPESetting.src_addrU = sVPESetting.src_addrPacY;
		sVPESetting.src_addrV = sVPESetting.src_addrPacY;
	}


	sVPESetting.src_format = psTransParam->eSrcFmt;
	sVPESetting.src_width = psTransParam->u32SrcImgWidth;
	sVPESetting.src_height = psTransParam->u32SrcImgHeight;

	sVPESetting.src_leftoffset = psTransParam->u32SrcLeftOffset;
	sVPESetting.src_rightoffset = psTransParam->u32SrcRightOffset;

	sVPESetting.dest_format = psTransParam->eDestFmt;
	sVPESetting.dest_width = psTransParam->u32DestImgWidth;
	sVPESetting.dest_height = psTransParam->u32DestImgHeight;

	sVPESetting.dest_leftoffset = psTransParam->u32DestLeftOffset;
	sVPESetting.dest_rightoffset = psTransParam->u32DestRightOffset;

	sVPESetting.algorithm = psTransParam->eVPEScaleMode;
	sVPESetting.rotation = psTransParam->eVPEOP;

	VPE_TransForm(&sVPESetting);

	return ERR_VPE_SUCCESS;
}

ERRCODE
VPE_WaitTransDone(void){
	
	int32_t i32DelayCnt =  MAX_TRANS_TIMEOUT / 5;

	while(vpeIoctl(VPE_IOCTL_CHECK_TRIGGER,	0, 0, 0) != 0) { //TRUE==>Not complete, FALSE==>Complete
//		vTaskDelay(5 / portTICK_RATE_MS);	//delay 5ms
		usleep(5000);
		i32DelayCnt --;
		
		if(i32DelayCnt == 0)
			break;
	}

	vpeDisableInt(VPE_INT_COMP);				
	vpeDisableInt(VPE_INT_DMA_ERR);		
	vpeClose();

	pthread_mutex_unlock(s_ptVPEEngMutex);

	if(i32DelayCnt == 0)
		return ERR_VPE_TRANS;
	return ERR_VPE_SUCCESS;
}


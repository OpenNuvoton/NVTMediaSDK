/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/
#ifndef __VPE__
#define __VPE__

#include <stdint.h>

#include "ImgProc_Comm.h"
#include "N9H26_VPE.h"

#define ERR_VPE_SUCCESS						0
#define ERR_VPE_OPEN_DEV				(VPE_ERRNO | 0x1)
#define ERR_VPE_SET_FORMAT				(VPE_ERRNO | 0x2)
#define ERR_VPE_TRIGGER					(VPE_ERRNO | 0x3)
#define ERR_VPE_WAIT_INTERRUPT			(VPE_ERRNO | 0x4)
#define ERR_VPE_RES						(VPE_ERRNO | 0x5)
#define ERR_VPE_MALLOC					(VPE_ERRNO | 0x6)
#define ERR_VPE_PARAM					(VPE_ERRNO | 0x7)
#define ERR_VPE_INIT					(VPE_ERRNO | 0x8)
#define ERR_VPE_TRANS					(VPE_ERRNO | 0x9)

typedef struct{
	E_VPE_CMD_OP eVPEOP;
	E_VPE_SCALE_MODE eVPEScaleMode;
	E_VPE_SRC_FMT eSrcFmt;
	E_VPE_DST_FMT eDestFmt;	
	uint32_t u32SrcImgWidth;
	uint32_t u32SrcImgHeight;
	uint32_t u32DestImgWidth;
	uint32_t u32DestImgHeight;
	uint32_t u32SrcRightOffset; 
	uint32_t u32SrcLeftOffset; 
	uint32_t u32SrcTopOffset;
	uint32_t u32SrcBottomOffset;	
	uint32_t u32DestRightOffset; 
	uint32_t u32DestLeftOffset; 
}S_VPE_TRANS_PARAM;

typedef struct{
	S_VPE_TRANS_PARAM *psTransParam;
	uint8_t *pu8SrcBuf; 
	uint8_t *pu8DestBuf; 
	uint32_t u32SrcBufPhyAdr; 
	uint32_t u32DestBufPhyAdr; 
}S_VPE_TRANS_ATTR;

ERRCODE
VPE_Init(void);

ERRCODE
VPE_Trans(
	S_VPE_TRANS_ATTR *psVPETransAttr
);

ERRCODE
VPE_WaitTransDone(void);

#endif


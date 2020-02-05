/* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "wblib.h"
#include "N9H26_NVTFAT.h"
#include "N9H26_SIC.h"
#include "N9H26_SDIO.h"

#include "FreeRTOS.h"
#include "task.h"
#include "Semphr.h"

#include "NVTMedia.h"
#include "NVTMedia_SAL_OS.h"
#include "NVTMedia_SAL_FS.h"

#include "MainTask.h"
#include "VideoIn.h"
#include "AudioIn.h"
#include "Render.h"

#include "FileMgr.h"

#define ENABLE_SD_ONE_PART
#define ENABLE_SD_0
//#define ENABLE_SD_1
//#define ENABLE_SD_2
//#define ENABLE_SDIO_1

#define DEF_DISK_VOLUME_STR_SIZE					20
#define DEF_RECORD_FILE_MGR_TYPE					eFILEMGR_TYPE_MP4
#define DEF_RECORD_FILE_FOLDER						"C:\\DCIM"
#define DEF_REC_RESERVE_STOR_SPACE 				(400000000)
#define DEF_EACH_REC_FILE_DUARTION 				(60000)					//60 second
#define DEF_NM_MEDIA_FILE_TYPE						eNM_MEDIA_MP4

#if (DEF_EACH_REC_FILE_DUARTION == eNM_UNLIMIT_TIME)
#define DEF_RECORDING_TIME				 				(60000 * 1 + 10000)
#else
#define DEF_RECORDING_TIME				 				(DEF_EACH_REC_FILE_DUARTION * 5 + 10000)
#endif

static char s_szDiskVolume[DEF_DISK_VOLUME_STR_SIZE] = {0x00};

typedef struct {
	char *szFileName;
	void *pvOpenRes;
}S_MEDIA_ATTR;

static S_MEDIA_ATTR s_sCurMediaAttr;
static S_MEDIA_ATTR s_sNextMediaAttr;
static xSemaphoreHandle s_tMediaMutex;
static BOOL s_bCreateNextMedia;

static int32_t
InitFileSystem(
	char **pszDiskVolume
)
{
	uint32_t u32ExtFreq;
	uint32_t u32PllOutHz;
	uint32_t u32BlockSize, u32FreeSize, u32DiskSize;

	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	memset(s_szDiskVolume, 0x00, DEF_DISK_VOLUME_STR_SIZE);
	*pszDiskVolume = NULL;
	
	//Init file system library 
	fsInitFileSystem();

#if defined(ENABLE_SD_ONE_PART)
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	
#if defined(ENABLE_SDIO_1)
	sdioOpen();	
	if (sdioSdOpen1()<=0)			//cause crash on Doorbell board SDIO slot. conflict with sensor pin
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}	
#elif defined(ENABLE_SD_1)
	sicOpen();
	if (sicSdOpen1()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}
#elif defined(ENABLE_SD_2)
	sicOpen();
	if (sicSdOpen2()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}
#elif defined(ENABLE_SD_0)
	sicOpen();
	if (sicSdOpen0()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		return -1;
	}
#endif
#endif			

	sprintf(s_szDiskVolume, "C");

#if defined(ENABLE_SD_ONE_PART)
	fsAssignDriveNumber(s_szDiskVolume[0], DISK_TYPE_SD_MMC, 0, 1);
#endif

	if(fsDiskFreeSpace(s_szDiskVolume[0], (UINT32 *)&u32BlockSize, (UINT32 *)&u32FreeSize, (UINT32 *)&u32DiskSize) != FS_OK){
		return -2;
	}

	printf("SD card block_size = %d\n", u32BlockSize);   
	printf("SD card free_size = %dkB\n", u32FreeSize);   
	printf("SD card disk_size = %dkB\n", u32DiskSize);   

	*pszDiskVolume = s_szDiskVolume;
	return 0;
}

char *
MainTask_GetDiskVolume(void)
{
	return s_szDiskVolume;
}


static bool
AudioIn_FillCB(
	S_NM_AUDIO_CTX	*psAudioCtx
)
{
	if(AudioIn_ReadFrameData(psAudioCtx->u32SamplePerBlock, psAudioCtx->pu8DataBuf, &psAudioCtx->u64DataTime) == true){
		psAudioCtx->u32DataSize = psAudioCtx->u32SamplePerBlock * psAudioCtx->u32Channel * 2;
		return true;
	}

	return false;
}

#if 0
static uint64_t s_u64PrevVinTime = 0;
#endif

static bool
VideoIn_FillCB(
	S_NM_VIDEO_CTX	*psVideoCtx
)
{
#if 0
	uint64_t u64CurVinTime = NMUtil_GetTimeMilliSec();
	uint64_t u64DiffTime = u64CurVinTime - s_u64PrevVinTime;
	
	if(u64DiffTime> 150)
		printf("VideoIn_FillCB time over %d \n", (uint32_t) u64DiffTime);

	s_u64PrevVinTime = u64CurVinTime;
#endif
	
	if(VideoIn_ReadNextPlanarFrame(0, &psVideoCtx->pu8DataBuf, &psVideoCtx->u64DataTime) == true){
		psVideoCtx->u32DataSize = psVideoCtx->u32Width * psVideoCtx->u32Height * 2;
		return true;
	}
	
	return false;
}

//Close current media and switch next media to current media
static int32_t
CloseAndChangeMedia(void)
{
	
	xSemaphoreTake(s_tMediaMutex, portMAX_DELAY);	

	if(s_sCurMediaAttr.pvOpenRes)
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &s_sCurMediaAttr.pvOpenRes);
		
	if(s_sCurMediaAttr.szFileName){
		FileMgr_UpdateFileInfo(DEF_RECORD_FILE_MGR_TYPE, s_sCurMediaAttr.szFileName);
		free(s_sCurMediaAttr.szFileName);
	}
		
	s_sCurMediaAttr.pvOpenRes = s_sNextMediaAttr.pvOpenRes;
	s_sCurMediaAttr.szFileName = s_sNextMediaAttr.szFileName;
	
	s_sNextMediaAttr.pvOpenRes = NULL;
	s_sNextMediaAttr.szFileName = NULL;
	
	xSemaphoreGive(s_tMediaMutex);	

	return 0;
}

//Create and register next media
static int32_t 
CreateAndRegNextMedia(
	HRECORD	hRecord,
	S_NM_RECORD_CONTEXT *psRecCtx	
)
{
	S_MEDIA_ATTR *psMediaAttr = NULL;
	int32_t i32Ret;
	E_NM_ERRNO eNMRet;
	S_NM_RECORD_IF sRecIf;
	
	xSemaphoreTake(s_tMediaMutex, portMAX_DELAY);	

	if((s_sCurMediaAttr.pvOpenRes != NULL) && (s_sNextMediaAttr.pvOpenRes != NULL)){
		xSemaphoreGive(s_tMediaMutex);	
		return 0;
	}
	
	if(s_sCurMediaAttr.pvOpenRes == NULL){
		psMediaAttr = &s_sCurMediaAttr;
	}
	else{
		psMediaAttr = &s_sNextMediaAttr;
	}
	
	i32Ret = FileMgr_CreateNewFileName(	DEF_RECORD_FILE_MGR_TYPE, 
										DEF_RECORD_FILE_FOLDER,
										0,
										&psMediaAttr->szFileName,
										0,
										DEF_REC_RESERVE_STOR_SPACE);

	if(i32Ret != 0){
		NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
		xSemaphoreGive(s_tMediaMutex);	
		return i32Ret;		
	}
	
	eNMRet = NMRecord_Open(
		psMediaAttr->szFileName,
		DEF_NM_MEDIA_FILE_TYPE,
		DEF_EACH_REC_FILE_DUARTION,
		psRecCtx,
		&sRecIf,
		&psMediaAttr->pvOpenRes	
	);

	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
		xSemaphoreGive(s_tMediaMutex);	
		return eNMRet;		
	}

	FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, psMediaAttr->szFileName);	

	//Register next media
	eNMRet = NMRecord_RegNextMedia(hRecord, sRecIf.psMediaIF, sRecIf.pvMediaRes, NULL);
	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_RegNextMedia(): %d\n", eNMRet);
		xSemaphoreGive(s_tMediaMutex);	
		return eNMRet;		
	}
	
	xSemaphoreGive(s_tMediaMutex);	
	return 0;
}

static void
Record_StatusCB(
	E_NM_RECORD_STATUS eStatus,
	S_NM_RECORD_INFO *psInfo,
	void *pvPriv
)
{
	switch(eStatus){
		case eNM_RECORD_STATUS_PAUSE:
		{
			printf("Recorder status: PAUSE \n");
		}
		break;
		case eNM_RECORD_STATUS_RECORDING:
		{
			printf("Recorder status: RECORDING \n");
		}
		break;
		case eNM_RECORD_STATUS_CHANGE_MEDIA:
		{
			printf("Recorder status: CHANGE MEDIA \n");
			CloseAndChangeMedia();
			xSemaphoreTake(s_tMediaMutex, portMAX_DELAY);	
			//Setup create next media flag
			s_bCreateNextMedia = TRUE;
			xSemaphoreGive(s_tMediaMutex);	
		}
		break;
		case eNM_RECORD_STATUS_STOP:
		{
			printf("Recorder status: STOP \n");
		}
		break;
	}
}



void MainTask( void *pvParameters )
{
	char *szDiskVolume = NULL;
	int32_t i32Ret;
	E_NM_ERRNO eNMRet;
	S_NM_RECORD_CONTEXT sRecCtx;
	S_NM_RECORD_IF sRecIf;
	HRECORD	hRecord;
	uint64_t u64StopRecTime;
	uint64_t u64CreateNewMediaTime = ULLONG_MAX;
	
	int32_t i32PlanarPipeNo = 0;
	S_VIN_PIPE_INFO *psPipeInfo;	
	S_AIN_INFO *psAinInfo;
	
	memset(&s_sCurMediaAttr, 0 , sizeof(S_MEDIA_ATTR));
	memset(&s_sNextMediaAttr, 0, sizeof(S_MEDIA_ATTR));

	hRecord = (HRECORD)eNM_INVALID_HANDLE;
		
	memset(&sRecCtx, 0 , sizeof(S_NM_RECORD_CONTEXT));
	memset(&sRecIf, 0 , sizeof(S_NM_RECORD_IF));
	
	s_tMediaMutex = xSemaphoreCreateMutex();
	if(s_tMediaMutex == NULL){
		NMLOG_ERROR("Unable create media mutex \n");
		return;
	}

	i32Ret = InitFileSystem(&szDiskVolume);
	
	if(i32Ret != 0){
		NMLOG_ERROR("Unable init file system %d\n", i32Ret);
		vTaskDelete(NULL);
		return;
	}

	Render_Init();
	
	i32Ret = VideoIn_DeviceInit();
	if(i32Ret != 0){
		NMLOG_ERROR("Unable init video in device %d\n", i32Ret);
		vTaskDelete(NULL);
		return;
	}
		
	i32Ret = AudioIn_DeviceInit();
	if(i32Ret != 0){
		NMLOG_ERROR("Unable init audio in device %d\n", i32Ret);
		vTaskDelete(NULL);
		return;
	}

	xTaskCreate( VideoIn_TaskCreate, "VideoIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL );
	xTaskCreate( AudioIn_TaskCreate, "AudioIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL );
	
 	i32Ret = FileMgr_CreateNewFileName(	DEF_RECORD_FILE_MGR_TYPE,
										DEF_RECORD_FILE_FOLDER,
										0,
										&s_sCurMediaAttr.szFileName,
										0,
										DEF_REC_RESERVE_STOR_SPACE);

	if(i32Ret != 0){
		NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
		vTaskDelete(NULL);
		return;		
	}
	
	for(i32PlanarPipeNo = 0; i32PlanarPipeNo < DEF_VIN_MAX_PIPES; i32PlanarPipeNo++){
		psPipeInfo = VideoIn_GetPipeInfo(i32PlanarPipeNo);
		if(psPipeInfo == NULL)
			break;
		
		if(psPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB){
			break;
		}
	}		
	
	if(psPipeInfo == NULL){
		NMLOG_ERROR("Unable find Vin YUV420 MB pipe \n")
		vTaskDelete(NULL);
	}

	psAinInfo = AudioIn_GetInfo();

	sRecCtx.sFillVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV420P_MB;
	sRecCtx.sFillVideoCtx.u32Width = psPipeInfo->u32Width;
	sRecCtx.sFillVideoCtx.u32Height = psPipeInfo->u32Height;
	sRecCtx.sFillVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;
	
	sRecCtx.sMediaVideoCtx.eVideoType = eNM_CTX_VIDEO_H264;
	sRecCtx.sMediaVideoCtx.u32Width = psPipeInfo->u32Width;
	sRecCtx.sMediaVideoCtx.u32Height = psPipeInfo->u32Height;
	sRecCtx.sMediaVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;

	sRecCtx.sFillAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
	sRecCtx.sFillAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
	sRecCtx.sFillAudioCtx.u32Channel = psAinInfo->u32Channel;

	sRecCtx.sMediaAudioCtx.eAudioType = eNM_CTX_AUDIO_AAC;
	sRecCtx.sMediaAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
	sRecCtx.sMediaAudioCtx.u32Channel = psAinInfo->u32Channel;
	sRecCtx.sMediaAudioCtx.u32BitRate = 64000;
	
	sRecIf.pfnVideoFill = VideoIn_FillCB;
	sRecIf.pfnAudioFill = AudioIn_FillCB;
	
	eNMRet = NMRecord_Open(
		s_sCurMediaAttr.szFileName,
		DEF_NM_MEDIA_FILE_TYPE,
		DEF_EACH_REC_FILE_DUARTION,
		&sRecCtx,
		&sRecIf,
		&s_sCurMediaAttr.pvOpenRes	
	);

	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
		vTaskDelete(NULL);
	}

	FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, s_sCurMediaAttr.szFileName);	
	AudioIn_CleanPCMBuff();

	//Start record
	eNMRet = NMRecord_Record(
		&hRecord,
		DEF_EACH_REC_FILE_DUARTION,
		&sRecIf,
		&sRecCtx,
		Record_StatusCB,
		NULL
	);
	
	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable start NMRecord_Record(): %d\n", eNMRet);
	}
	
	u64StopRecTime = NMUtil_GetTimeMilliSec() + DEF_RECORDING_TIME;

#if (DEF_EACH_REC_FILE_DUARTION == eNM_UNLIMIT_TIME)
	u64CreateNewMediaTime = eNM_UNLIMIT_TIME;
	s_bCreateNextMedia = FALSE;	
#else
	u64CreateNewMediaTime = NMUtil_GetTimeMilliSec() + (DEF_EACH_REC_FILE_DUARTION / 2);
	s_bCreateNextMedia = TRUE;
#endif
	
	while(NMUtil_GetTimeMilliSec() < u64StopRecTime){
		uint8_t *pu8FrameData;
		uint64_t u64FrameTime;
		
		// Show preview
		if(VideoIn_ReadNextPacketFrame(0, &pu8FrameData, &u64FrameTime) == TRUE){
			Render_SetFrameBuffAddr(pu8FrameData);
		}

		//Setup create next media time
		if(s_bCreateNextMedia == TRUE){
			xSemaphoreTake(s_tMediaMutex, portMAX_DELAY);	
			u64CreateNewMediaTime = NMUtil_GetTimeMilliSec() + (DEF_EACH_REC_FILE_DUARTION / 2);
			s_bCreateNextMedia = FALSE;
			xSemaphoreGive(s_tMediaMutex);	
		}
		
		//Creat next media
		if(NMUtil_GetTimeMilliSec() > u64CreateNewMediaTime){
			CreateAndRegNextMedia(hRecord, &sRecCtx);		
			u64CreateNewMediaTime = ULLONG_MAX;
		}
		
		vTaskDelay(5);
	}

	printf("Stop Recording ! \n");
	
	xSemaphoreTake(s_tMediaMutex, portMAX_DELAY);	

	//Close next media
	if(s_sNextMediaAttr.pvOpenRes){
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &s_sNextMediaAttr.pvOpenRes);
		if(s_sNextMediaAttr.szFileName){
			//delete next media file
			unlink(s_sNextMediaAttr.szFileName);
			free(s_sNextMediaAttr.szFileName);
		}
	}

	//Close current media
	if(s_sCurMediaAttr.pvOpenRes){
		NMRecord_Close(hRecord, &s_sCurMediaAttr.pvOpenRes);
		if(s_sCurMediaAttr.szFileName)
			free(s_sCurMediaAttr.szFileName);
	}
	
	xSemaphoreGive(s_tMediaMutex);	
	
	Render_Final();
	vTaskDelete(NULL);
}

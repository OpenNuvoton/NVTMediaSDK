/**************************************************************************//**
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
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
******************************************************************************/
/* Standard includes. */
#include <stdlib.h>
#include <limits.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "Semphr.h"

/* Nuvoton includes. */
#include "wblib.h"
#include "N9H26.h"
#include "NVTMedia.h"
#include "NVTMedia_SAL_FS.h"

#include "FileMgr.h"
#include "Recorder.h"

#include "VideoIn.h"
#include "AudioIn.h"
#include "Render.h"

#include "Recorder.h"

typedef struct
{
    char *szFileName;
    void *pvOpenRes;
} S_MEDIA_ATTR;

typedef struct
{
    HRECORD					m_hRecord;
    uint64_t				m_u64CurTimeInMs;
    uint64_t				m_u64CurFreeSpaceInByte;
    uint64_t				m_u64CurWroteSizeInByte;

//    S_NM_RECORD_IF			m_sRecIf;
    S_NM_RECORD_CONTEXT		m_sRecCtx;
    S_MEDIA_ATTR			m_sCurMediaAttr;
    E_NM_RECORD_STATUS		m_eCurRecordingStatus;

    S_MEDIA_ATTR			m_sNextMediaAttr;
	BOOL					m_bCreateNextMedia;
	uint32_t 				m_u32EachRecFileDuration;

	xSemaphoreHandle		m_tMediaMutex;

} S_RECORDER;

static S_RECORDER g_sRecorder =
{
    .m_hRecord = (HRECORD)eNM_INVALID_HANDLE,
    0
};

static char *g_pcRecordedDisk = DEF_RECORD_FILE_DISK;


///////////////////////////////////////////////////////////////////////////

//Close current media and switch next media to current media
static int32_t
CloseAndChangeMedia(void)
{
	
	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes)
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &g_sRecorder.m_sCurMediaAttr.pvOpenRes);
		
	if(g_sRecorder.m_sCurMediaAttr.szFileName){
		FileMgr_UpdateFileInfo(DEF_RECORD_FILE_MGR_TYPE, g_sRecorder.m_sCurMediaAttr.szFileName);
		free(g_sRecorder.m_sCurMediaAttr.szFileName);
	}
		
	g_sRecorder.m_sCurMediaAttr.pvOpenRes = g_sRecorder.m_sNextMediaAttr.pvOpenRes;
	g_sRecorder.m_sCurMediaAttr.szFileName = g_sRecorder.m_sNextMediaAttr.szFileName;
	
	g_sRecorder.m_sNextMediaAttr.pvOpenRes = NULL;
	g_sRecorder.m_sNextMediaAttr.szFileName = NULL;
	
	xSemaphoreGive(g_sRecorder.m_tMediaMutex);	

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
		
	if(hRecord == (HRECORD)eNM_INVALID_HANDLE){
		return 0;
	}
	
	if((g_sRecorder.m_sCurMediaAttr.pvOpenRes != NULL) && (g_sRecorder.m_sNextMediaAttr.pvOpenRes != NULL)){
		return 0;
	}
	
	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes == NULL){
		psMediaAttr = &g_sRecorder.m_sCurMediaAttr;
	}
	else{
		psMediaAttr = &g_sRecorder.m_sNextMediaAttr;
	}
	
	i32Ret = FileMgr_CreateNewFileName(	DEF_RECORD_FILE_MGR_TYPE, 
										DEF_RECORD_FILE_FOLDER,
										0,
										&psMediaAttr->szFileName,
										0,
										DEF_REC_RESERVE_STOR_SPACE);

	if(i32Ret != 0){
		NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
		return i32Ret;		
	}
	
	eNMRet = NMRecord_Open(
		psMediaAttr->szFileName,
		DEF_NM_MEDIA_FILE_TYPE,
		g_sRecorder.m_u32EachRecFileDuration,
		psRecCtx,
		&sRecIf,
		&psMediaAttr->pvOpenRes	
	);

	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
		return eNMRet;		
	}

	FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, psMediaAttr->szFileName);	

	//Register next media
	eNMRet = NMRecord_RegNextMedia(hRecord, sRecIf.psMediaIF, sRecIf.pvMediaRes, NULL);
	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_RegNextMedia(): %d\n", eNMRet);
		return eNMRet;		
	}

	return 0;
}

static bool
AudioIn_FillCB(
    S_NM_AUDIO_CTX  *psAudioCtx
)
{
    if (AudioIn_ReadFrameData(psAudioCtx->u32SamplePerBlock, psAudioCtx->pu8DataBuf, &psAudioCtx->u64DataTime) == true)
    {
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
    S_NM_VIDEO_CTX  *psVideoCtx
)
{
#if 0
    uint64_t u64CurVinTime = NMUtil_GetTimeMilliSec();
    uint64_t u64DiffTime = u64CurVinTime - s_u64PrevVinTime;

    if (u64DiffTime > 150)
        printf("VideoIn_FillCB time over %d \n", (uint32_t) u64DiffTime);

    s_u64PrevVinTime = u64CurVinTime;
#endif

    if (VideoIn_ReadNextPlanarFrame(0, &psVideoCtx->pu8DataBuf, &psVideoCtx->u64DataTime) == true)
    {
        psVideoCtx->u32DataSize = psVideoCtx->u32Width * psVideoCtx->u32Height * 2;
        return true;
    }

    return false;
}

static void
Record_StatusCB(
    E_NM_RECORD_STATUS eStatus,
    S_NM_RECORD_INFO *psInfo,
    void *pvPriv
)
{

    g_sRecorder.m_eCurRecordingStatus = eStatus;


    switch (eStatus)
    {
    case eNM_RECORD_STATUS_PAUSE:
        printf("Recorder status: PAUSE \n");
        break;
    case eNM_RECORD_STATUS_RECORDING:
        printf("Recorder status: RECORDING \n");
        if (psInfo)
        {
            g_sRecorder.m_u64CurTimeInMs = psInfo->u32Duration;
            g_sRecorder.m_u64CurWroteSizeInByte = psInfo->u64TotalChunkSize;
        }
        break;
    case eNM_RECORD_STATUS_CHANGE_MEDIA:
        printf("Recorder status: CHANGE MEDIA \n");
		CloseAndChangeMedia();
		xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
		//Setup create next media flag
		g_sRecorder.m_bCreateNextMedia = TRUE;
		xSemaphoreGive(g_sRecorder.m_tMediaMutex);	
		break;
    case eNM_RECORD_STATUS_STOP:
        g_sRecorder.m_u64CurTimeInMs = psInfo->u32Duration;
        printf("Recorder status: STOP \n");
        break;
    } //switch

}

static void ResetRecorderMember(void)
{
	g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;
	g_sRecorder.m_u64CurTimeInMs = 0;
	g_sRecorder.m_u64CurFreeSpaceInByte = 0;
	g_sRecorder.m_u64CurWroteSizeInByte = 0;
	memset(&g_sRecorder.m_sRecCtx, 0 , sizeof(S_NM_RECORD_CONTEXT));
	memset(&g_sRecorder.m_sCurMediaAttr, 0 , sizeof(S_MEDIA_ATTR));
	g_sRecorder.m_eCurRecordingStatus = eNM_RECORD_STATUS_STOP;

	memset(&g_sRecorder.m_sNextMediaAttr, 0, sizeof(S_MEDIA_ATTR));	
	g_sRecorder.m_bCreateNextMedia = FALSE;
	g_sRecorder.m_u32EachRecFileDuration = eNM_UNLIMIT_TIME;
		
}

///////////////////////////////////////////////////////////////////////////////////

char *Recorder_getdiskvolume(void)
{
    return g_pcRecordedDisk;
}

uint64_t Recorder_get_disk_freespace(void)
{
    char *szDiskVolumt;
    uint32_t u32BlockSize, u32FreeSize, u32DiskSize;

    szDiskVolumt = Recorder_getdiskvolume();
    if (fsDiskFreeSpace(szDiskVolumt[0], (UINT32 *)&u32BlockSize, (UINT32 *)&u32FreeSize, (UINT32 *)&u32DiskSize) != FS_OK)
        return ERR_FILEMGR_WORK_FOLDER;

    return u32FreeSize * 1024 ;
}



int Recorder_is_over_limitedsize(void)
{
    return (g_sRecorder.m_u64CurFreeSpaceInByte - g_sRecorder.m_u64CurWroteSizeInByte) <= DEF_REC_RESERVE_STORE_SPACE;
}

int Recorder_start(
	uint32_t u32FileDuration
)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

    if (g_sRecorder.m_hRecord == (HRECORD)eNM_INVALID_HANDLE)
    {
        int32_t i32Ret;
        int32_t i32PlanarPipeNo = 0;
        S_VIN_PIPE_INFO *psPipeInfo;
        S_AIN_INFO *psAinInfo;	
		S_NM_RECORD_IF	sRecIf;


		ResetRecorderMember();
			
		g_sRecorder.m_u32EachRecFileDuration = u32FileDuration;		
        g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;		
        g_sRecorder.m_u64CurFreeSpaceInByte = Recorder_get_disk_freespace();
		memset(&sRecIf, 0 , sizeof(S_NM_RECORD_IF));

		if(g_sRecorder.m_u32EachRecFileDuration == eNM_UNLIMIT_TIME){
			if (g_sRecorder.m_u64CurFreeSpaceInByte < DEF_REC_RESERVE_STORE_SPACE)
			{
				eNMRet = eNM_ERRNO_SIZE;
				goto exit_recorder_start;
			}
		}
        printf("Free space in disk %d\n", (uint32_t)g_sRecorder.m_u64CurFreeSpaceInByte);

//              printf("Free space in disk-%c %012"PRId64"\n", recorder_getdiskvolume()[0] ,g_sRecorder.m_u64CurFreeSpaceInByte);

        i32Ret = FileMgr_CreateNewFileName(DEF_RECORD_FILE_MGR_TYPE,
                                           DEF_RECORD_FILE_FOLDER,
                                           0,
                                           &g_sRecorder.m_sCurMediaAttr.szFileName,
                                           0,
                                           DEF_REC_RESERVE_STORE_SPACE);

        if (i32Ret != 0)
        {
            NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
            eNMRet = eNM_ERRNO_OS;
            goto exit_recorder_start;
        }

        printf("Recorded file name is %s.\n", g_sRecorder.m_sCurMediaAttr.szFileName);

        for (i32PlanarPipeNo = 0; i32PlanarPipeNo < DEF_VIN_MAX_PIPES; i32PlanarPipeNo++)
        {
            psPipeInfo = VideoIn_GetPipeInfo(i32PlanarPipeNo);
            if (psPipeInfo == NULL)
                break;

            if (psPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB)
            {
                break;
            }
        }

        if (psPipeInfo == NULL)
        {
            NMLOG_ERROR("Unable find Vin YUV420 MB pipe \n")
            eNMRet = eNM_ERRNO_CODEC_TYPE;
            goto exit_recorder_start;
        }

        psAinInfo = AudioIn_GetInfo();

        g_sRecorder.m_sRecCtx.sFillVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV420P_MB;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32Width = psPipeInfo->u32Width;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32Height = psPipeInfo->u32Height;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;

        g_sRecorder.m_sRecCtx.sMediaVideoCtx.eVideoType = eNM_CTX_VIDEO_H264;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32Width = psPipeInfo->u32Width;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32Height = psPipeInfo->u32Height;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;

        g_sRecorder.m_sRecCtx.sFillAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
        g_sRecorder.m_sRecCtx.sFillAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
        g_sRecorder.m_sRecCtx.sFillAudioCtx.u32Channel = psAinInfo->u32Channel;

        g_sRecorder.m_sRecCtx.sMediaAudioCtx.eAudioType = eNM_CTX_AUDIO_AAC;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32Channel = psAinInfo->u32Channel;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32BitRate = 64000;

        sRecIf.pfnVideoFill = VideoIn_FillCB;
        sRecIf.pfnAudioFill = AudioIn_FillCB;

        eNMRet = NMRecord_Open(g_sRecorder.m_sCurMediaAttr.szFileName,
                               DEF_NM_MEDIA_FILE_TYPE,
                               g_sRecorder.m_u32EachRecFileDuration,
                               &g_sRecorder.m_sRecCtx,
                               &sRecIf,
                               &g_sRecorder.m_sCurMediaAttr.pvOpenRes
                              );

        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
            goto exit_recorder_start;
        }

        FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, g_sRecorder.m_sCurMediaAttr.szFileName);
        AudioIn_CleanPCMBuff();

        eNMRet = NMRecord_Record(
                     &g_sRecorder.m_hRecord,
                     g_sRecorder.m_u32EachRecFileDuration,
                     &sRecIf,
                     &g_sRecorder.m_sRecCtx,
                     Record_StatusCB,
                     NULL);

        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable start NMRecord_Record(): %d\n", eNMRet);
            goto exit_recorder_start;
        }

		if(g_sRecorder.m_u32EachRecFileDuration != eNM_UNLIMIT_TIME){
			g_sRecorder.m_bCreateNextMedia = TRUE;
		}
    }
	
	xSemaphoreGive(g_sRecorder.m_tMediaMutex);
    return eNMRet;

exit_recorder_start:

	xSemaphoreGive(g_sRecorder.m_tMediaMutex);
    return -(eNMRet);
}

int Recorder_stop(void)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

	//Close next media
	if(g_sRecorder.m_sNextMediaAttr.pvOpenRes){
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &g_sRecorder.m_sNextMediaAttr.pvOpenRes);
		if(g_sRecorder.m_sNextMediaAttr.szFileName){
			//delete next media file
			unlink(g_sRecorder.m_sNextMediaAttr.szFileName);
			free(g_sRecorder.m_sNextMediaAttr.szFileName);
		}
	}

	//Close current media
	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes){
		NMRecord_Close(g_sRecorder.m_hRecord, &g_sRecorder.m_sCurMediaAttr.pvOpenRes);
		if(g_sRecorder.m_sCurMediaAttr.szFileName)
			free(g_sRecorder.m_sCurMediaAttr.szFileName);
	}

	ResetRecorderMember();

	xSemaphoreGive(g_sRecorder.m_tMediaMutex);	
    return eNMRet;
}



///////////////////////////////////////////////////////////////////////////

#define mainMAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 3 )

void *worker_recorder(void *pvParameters)
{
    int32_t i32Ret;

	ResetRecorderMember();

	g_sRecorder.m_tMediaMutex = xSemaphoreCreateMutex();
	if(g_sRecorder.m_tMediaMutex == NULL){
		NMLOG_ERROR("Unable create media mutex \n");
        goto exit_worker_recorder;
	}
		
    i32Ret = VideoIn_DeviceInit();
    if (i32Ret != 0)
    {
        NMLOG_ERROR("Unable init video in device %d\n", i32Ret);
        goto exit_worker_recorder;
    }

    i32Ret = AudioIn_DeviceInit();
    if (i32Ret != 0)
    {
        NMLOG_ERROR("Unable init audio in device %d\n", i32Ret);
        goto exit_worker_recorder;
    }

    xTaskCreate(VideoIn_TaskCreate, "VideoIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL);
    xTaskCreate(AudioIn_TaskCreate, "AudioIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL);

    Render_Init();

	uint8_t *pu8FrameData;
	uint64_t u64FrameTime;
	uint64_t u64CreateNewMediaTime;

    while (1)
    {
		
		//Show preview
        if (VideoIn_ReadNextPacketFrame(0, &pu8FrameData, &u64FrameTime) == TRUE)
        {
            Render_SetFrameBuffAddr(pu8FrameData);
        }

		//Setup create next media time
		if(g_sRecorder.m_bCreateNextMedia == TRUE){
			xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
			u64CreateNewMediaTime = NMUtil_GetTimeMilliSec() + (g_sRecorder.m_u32EachRecFileDuration / 2);
			g_sRecorder.m_bCreateNextMedia = FALSE;
			xSemaphoreGive(g_sRecorder.m_tMediaMutex);
		}
		
		//Creat next media
		if(NMUtil_GetTimeMilliSec() > u64CreateNewMediaTime){
			xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
			CreateAndRegNextMedia(g_sRecorder.m_hRecord, &g_sRecorder.m_sRecCtx);		
			xSemaphoreGive(g_sRecorder.m_tMediaMutex);
			u64CreateNewMediaTime = ULLONG_MAX;
		}

        if (g_sRecorder.m_eCurRecordingStatus == eNM_RECORD_STATUS_RECORDING){
			if((g_sRecorder.m_u32EachRecFileDuration == eNM_UNLIMIT_TIME) && (Recorder_is_over_limitedsize())){
				printf("No enough space to record. Stop recording.\n");
				Recorder_stop();
			}
        }
				
        vTaskDelay(5);
    }

    Render_Final();
exit_worker_recorder:

    NMLOG_ERROR("Die... %s\n", __func__);

    return NULL;
}


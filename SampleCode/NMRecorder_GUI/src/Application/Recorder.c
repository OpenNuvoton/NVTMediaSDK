/**************************************************************************//**
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
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

/* Nuvoton includes. */
#include "wblib.h"
#include "N9H26.h"
#include "NVTMedia.h"

#include "VideoIn.h"
#include "AudioIn.h"
#include "Render.h"

#include "FileMgr.h"
#include "recorder.h"

typedef struct
{
    char *szFileName;
    void *pvOpenRes;
} S_MEDIA_ATTR;

typedef struct
{
    HRECORD                          m_hRecord;
    uint64_t               m_u64CurTimeInMs;
    uint64_t               m_u64CurFreeSpaceInByte;
    uint64_t               m_u64CurWroteSizeInByte;

    S_NM_RECORD_IF         m_sRecIf;
    S_NM_RECORD_CONTEXT    m_sRecordCtx;
    S_MEDIA_ATTR                     m_sCurMediaAttr;
    E_NM_RECORD_STATUS       m_eCurRecordingStatus;
} S_RECORDER;

static S_RECORDER g_sRecorder =
{
    .m_hRecord = (HRECORD)eNM_INVALID_HANDLE,
    0
};

static char *g_pcRecordedDisk = DEF_RECORD_FILE_DISK;

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
        break;
    case eNM_RECORD_STATUS_STOP:
        g_sRecorder.m_u64CurTimeInMs = psInfo->u32Duration;
        printf("Recorder status: STOP \n");
        break;
    } //switch

}

uint64_t recorder_recordedtime_current(void)
{
    return g_sRecorder.m_u64CurTimeInMs;
}

int recorder_is_over_limitedsize(void)
{
    return (g_sRecorder.m_u64CurFreeSpaceInByte - g_sRecorder.m_u64CurWroteSizeInByte) <= DEF_REC_RESERVE_STORE_SPACE;
}

uint64_t recorder_get_disk_freespace(void)
{
    char *szDiskVolumt;
    uint32_t u32BlockSize, u32FreeSize, u32DiskSize;

    szDiskVolumt = recorder_getdiskvolume();
    if (fsDiskFreeSpace(szDiskVolumt[0], &u32BlockSize, &u32FreeSize, &u32DiskSize) != FS_OK)
        return ERR_FILEMGR_WORK_FOLDER;

    return u32FreeSize * 1024 ;
}

int recorder_start(void)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

    if (g_sRecorder.m_hRecord == (HRECORD)eNM_INVALID_HANDLE)
    {
        int32_t i32Ret;
        int32_t i32PlanarPipeNo = 0;
        S_VIN_PIPE_INFO *psPipeInfo;
        S_AIN_INFO *psAinInfo;

        memset(&g_sRecorder, 0, sizeof(g_sRecorder));
        g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;
        g_sRecorder.m_u64CurFreeSpaceInByte = recorder_get_disk_freespace();

        if (g_sRecorder.m_u64CurFreeSpaceInByte < DEF_REC_RESERVE_STORE_SPACE)
        {
            eNMRet = eNM_ERRNO_SIZE;
            goto exit_recorder_start;
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

        g_sRecorder.m_sRecordCtx.sFillVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV420P_MB;
        g_sRecorder.m_sRecordCtx.sFillVideoCtx.u32Width = psPipeInfo->u32Width;
        g_sRecorder.m_sRecordCtx.sFillVideoCtx.u32Height = psPipeInfo->u32Height;
        g_sRecorder.m_sRecordCtx.sFillVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;

        g_sRecorder.m_sRecordCtx.sMediaVideoCtx.eVideoType = eNM_CTX_VIDEO_H264;
        g_sRecorder.m_sRecordCtx.sMediaVideoCtx.u32Width = psPipeInfo->u32Width;
        g_sRecorder.m_sRecordCtx.sMediaVideoCtx.u32Height = psPipeInfo->u32Height;
        g_sRecorder.m_sRecordCtx.sMediaVideoCtx.u32FrameRate = psPipeInfo->u32FrameRate;

        g_sRecorder.m_sRecordCtx.sFillAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
        g_sRecorder.m_sRecordCtx.sFillAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
        g_sRecorder.m_sRecordCtx.sFillAudioCtx.u32Channel = psAinInfo->u32Channel;

        g_sRecorder.m_sRecordCtx.sMediaAudioCtx.eAudioType = eNM_CTX_AUDIO_AAC;
        g_sRecorder.m_sRecordCtx.sMediaAudioCtx.u32SampleRate = psAinInfo->u32SampleRate;
        g_sRecorder.m_sRecordCtx.sMediaAudioCtx.u32Channel = psAinInfo->u32Channel;
        g_sRecorder.m_sRecordCtx.sMediaAudioCtx.u32BitRate = 64000;

        g_sRecorder.m_sRecIf.pfnVideoFill = VideoIn_FillCB;
        g_sRecorder.m_sRecIf.pfnAudioFill = AudioIn_FillCB;

        eNMRet = NMRecord_Open(g_sRecorder.m_sCurMediaAttr.szFileName,
                               DEF_NM_MEDIA_FILE_TYPE,
                               DEF_EACH_REC_FILE_DUARTION,
                               &g_sRecorder.m_sRecordCtx,
                               &g_sRecorder.m_sRecIf,
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
                     DEF_EACH_REC_FILE_DUARTION,
                     &g_sRecorder.m_sRecIf,
                     &g_sRecorder.m_sRecordCtx,
                     Record_StatusCB,
                     NULL);

        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable start NMRecord_Record(): %d\n", eNMRet);
            goto exit_recorder_start;
        }
    }

    return eNMRet;

exit_recorder_start:

    return -(eNMRet);
}

int recorder_stop(void)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
    if (g_sRecorder.m_hRecord != (HRECORD)eNM_INVALID_HANDLE)
    {
        eNMRet = NMRecord_Close(g_sRecorder.m_hRecord, &g_sRecorder.m_sCurMediaAttr.pvOpenRes);
        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable close NMRecord_Close(): %d\n", eNMRet);
        }
    }
    memset(&g_sRecorder, 0, sizeof(g_sRecorder));
    g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;
    return eNMRet;
}

E_NM_RECORD_STATUS recorder_status(void)
{
    // get status
    if (g_sRecorder.m_hRecord != (HRECORD)eNM_INVALID_HANDLE)
        return g_sRecorder.m_eCurRecordingStatus;
    return eNM_RECORD_STATUS_STOP;
}

char *recorder_getdiskvolume(void)
{
    return g_pcRecordedDisk;
}

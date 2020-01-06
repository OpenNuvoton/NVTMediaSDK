/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
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

#include "wblib.h"
#include "N9H26_NVTFAT.h"
#include "N9H26_SIC.h"
#include "N9H26_SDIO.h"

#include "NVTMedia.h"
#include "NVTMedia_SAL_OS.h"
#include "NVTMedia_SAL_FS.h"

#include "Render.h"

#define ENABLE_SD_ONE_PART
#define ENABLE_SD_0
//#define ENABLE_SD_1
//#define ENABLE_SD_2
//#define ENABLE_SDIO_1

#define DEF_DISK_VOLUME_STR_SIZE	20
static char s_szDiskVolume[DEF_DISK_VOLUME_STR_SIZE] = {0x00};

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

	if(fsDiskFreeSpace(s_szDiskVolume[0], &u32BlockSize, &u32FreeSize, &u32DiskSize) != FS_OK){
		return -2;
	}

	printf("SD card block_size = %d\n", u32BlockSize);   
	printf("SD card free_size = %dkB\n", u32FreeSize);   
	printf("SD card disk_size = %dkB\n", u32DiskSize);   

	*pszDiskVolume = s_szDiskVolume;
	return 0;
}

static void
PrintMediaInfo(
	S_NM_PLAY_INFO *psPlayInfo,
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx
)
{
	printf("Video Type %d\n", psPlayInfo->eVideoType);
	printf("	width %d\n", psVideoCtx->u32Width);
	printf("	height %d\n", psVideoCtx->u32Height);
	
	printf("Audio Type %d\n", psPlayInfo->eAudioType);
	printf("	sample rate %d\n", psAudioCtx->u32SampleRate);
	printf("	channel %d\n", psAudioCtx->u32Channel);
}


/*Play File List */
/*
AmelieMovie_VGA20_H264.avi
93video_MJPG.avi
NVTDV_264vga20ima16kmono.avi
NVTDV_P0_000000000001.avi
NVTDV_P0_000000000002.avi
NVTDV_aac_h264.mp4
EI_20120628_111548_AAC_H264.mp4
*/

void MainTask( void *pvParameters )
{
	char *szDiskVolume = NULL;
	int32_t i32Ret;
	char *szTestAVIFile1 = "C:\\DCIM\\EI_20120628_111548_AAC_H264.mp4";
	char *szTestAVIFile2 = "C:\\DCIM\\AmelieMovie_VGA20_H264.avi";

	uint64_t u64StartPlayTime;
	uint64_t u64PausePlayTime;
	uint64_t u64ResumePlayTime;
	uint64_t u64StopPlayTime;
	
	E_NM_ERRNO eNMRet;
	S_NM_PLAY_IF sPlayIF;
	S_NM_PLAY_CONTEXT sPlayCtx;
	S_NM_PLAY_INFO sPlayInfo;
	HPLAY hPlay = (HPLAY)eNM_INVALID_HANDLE;
	void *pvOpenRes = NULL;
	
	i32Ret = InitFileSystem(&szDiskVolume);
	
	if(i32Ret != 0){
		printf("Unable init file system \n");
		return;
	}
	
//	NMUtil_SetLogLevel(DEF_NM_MSG_DEBUG);
	
	printf("***********Start play avi file 1 *************** \n");
	//Open AVI file 1
	eNMRet = NMPlay_Open(szTestAVIFile1, &sPlayIF, &sPlayCtx, &sPlayInfo, &pvOpenRes);
	if(eNMRet != eNM_ERRNO_NONE){
		printf("Unable open play meda %x\n", eNMRet);
		return;
	}

	PrintMediaInfo(&sPlayInfo, &sPlayCtx.sMediaVideoCtx, &sPlayCtx.sMediaAudioCtx);
	
	sPlayIF.pfnVideoFlush = Render_VideoFlush;
	sPlayIF.pfnAudioFlush = Render_AudioFlush;
		
	sPlayCtx.sFlushVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV422;
	sPlayCtx.sFlushVideoCtx.u32Width = LCD_PANEL_WIDTH;
	sPlayCtx.sFlushVideoCtx.u32Height = LCD_PANEL_HEIGHT;

	sPlayCtx.sFlushAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
	sPlayCtx.sFlushAudioCtx.u32SampleRate = sPlayCtx.sMediaAudioCtx.u32SampleRate;
	sPlayCtx.sFlushAudioCtx.u32Channel = sPlayCtx.sMediaAudioCtx.u32Channel;
	sPlayCtx.sFlushAudioCtx.u32SamplePerBlock = sPlayCtx.sMediaAudioCtx.u32SamplePerBlock;
	sPlayCtx.sFlushAudioCtx.pvParamSet = sPlayCtx.sMediaAudioCtx.pvParamSet;
	
	Render_Init(sPlayCtx.sFlushAudioCtx.u32SampleRate, sPlayCtx.sFlushAudioCtx.u32Channel);
	
	eNMRet = NMPlay_Play(&hPlay, &sPlayIF, &sPlayCtx, true);
	if(eNMRet != eNM_ERRNO_NONE){
		NMPlay_Close(hPlay, &pvOpenRes);
		printf("Unable play media %x\n", eNMRet);
		return;
	}

	
	u64StartPlayTime = NMUtil_GetTimeMilliSec();
	u64PausePlayTime = u64StartPlayTime + 5000;
	u64ResumePlayTime = u64PausePlayTime + 10000;
	u64StopPlayTime = u64ResumePlayTime + 5000;
#if 1
	
	while(NMUtil_GetTimeMilliSec() < u64PausePlayTime){
		usleep(1000);
	}

	NMPlay_Pause(hPlay, true);
	while(NMUtil_GetTimeMilliSec() < u64ResumePlayTime){
		usleep(1000);
	}

	NMPlay_Play(&hPlay, NULL, NULL, true);

	u64PausePlayTime = NMUtil_GetTimeMilliSec() + 5000;
	u64ResumePlayTime = u64PausePlayTime + 10000;

	while(NMUtil_GetTimeMilliSec() < u64PausePlayTime){
		usleep(1000);
	}

	NMPlay_Pause(hPlay, true);
	while(NMUtil_GetTimeMilliSec() < u64ResumePlayTime){
		usleep(1000);
	}

	NMPlay_Play(&hPlay, NULL, NULL, true);

	
	while(NMPlay_Status(hPlay) != eNM_PLAY_STATUS_EOM){
		usleep(1000);
	}
	
#else	
	while(NMPlay_Status(hPlay) != eNM_PLAY_STATUS_EOM){
		usleep(1000);
	}
#endif
	
	NMPlay_Close(hPlay, &pvOpenRes);

	Render_Final();
	
	printf("***********Start play avi file 2 *************** \n");
	
	//Open AVI file 2
	eNMRet = NMPlay_Open(szTestAVIFile2, &sPlayIF, &sPlayCtx, &sPlayInfo, &pvOpenRes);
	if(eNMRet != eNM_ERRNO_NONE){
		printf("Unable open play meda %x\n", eNMRet);
		return;
	}
	
	sPlayIF.pfnVideoFlush = Render_VideoFlush;
	sPlayIF.pfnAudioFlush = Render_AudioFlush;
		
	sPlayCtx.sFlushVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV422;
	sPlayCtx.sFlushVideoCtx.u32Width = LCD_PANEL_WIDTH;
	sPlayCtx.sFlushVideoCtx.u32Height = LCD_PANEL_HEIGHT;

	sPlayCtx.sFlushAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
	sPlayCtx.sFlushAudioCtx.u32SampleRate = sPlayCtx.sMediaAudioCtx.u32SampleRate;
	sPlayCtx.sFlushAudioCtx.u32Channel = sPlayCtx.sMediaAudioCtx.u32Channel;
	sPlayCtx.sFlushAudioCtx.u32SamplePerBlock = sPlayCtx.sMediaAudioCtx.u32SamplePerBlock;
	sPlayCtx.sFlushAudioCtx.pvParamSet = sPlayCtx.sMediaAudioCtx.pvParamSet;
	
	Render_Init(sPlayCtx.sFlushAudioCtx.u32SampleRate, sPlayCtx.sFlushAudioCtx.u32Channel);
	
	hPlay = (HPLAY)eNM_INVALID_HANDLE;
	eNMRet = NMPlay_Play(&hPlay, &sPlayIF, &sPlayCtx, true);
	if(eNMRet != eNM_ERRNO_NONE){
		NMPlay_Close(hPlay, &pvOpenRes);
		printf("Unable play media %x\n", eNMRet);
		return;
	}

	
	u64StartPlayTime = NMUtil_GetTimeMilliSec();
	u64PausePlayTime = u64StartPlayTime + 5000;
	u64ResumePlayTime = u64PausePlayTime + 1000;
	u64StopPlayTime = u64ResumePlayTime + 30000;
	
	while(NMUtil_GetTimeMilliSec() < u64PausePlayTime){
		usleep(1000);
	}

	NMPlay_Pause(hPlay, true);
	while(NMUtil_GetTimeMilliSec() < u64ResumePlayTime){
		usleep(1000);
	}
	
	NMPlay_Seek(hPlay, 120000, sPlayInfo.u32VideoChunks, sPlayInfo.u32AudioChunks, true);
	
	NMPlay_Play(&hPlay, NULL, NULL, true);
	while(NMUtil_GetTimeMilliSec() < u64StopPlayTime){
		usleep(1000);
	}

	NMPlay_Seek(hPlay, 5000, sPlayInfo.u32VideoChunks, sPlayInfo.u32AudioChunks, true);

	NMPlay_Play(&hPlay, NULL, NULL, true);
	while(NMPlay_Status(hPlay) != eNM_PLAY_STATUS_EOM){
		usleep(1000);
	}
	
	NMPlay_Close(hPlay, &pvOpenRes);

	Render_Final();
}

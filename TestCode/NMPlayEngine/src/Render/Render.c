//		Copyright (c) 2019 Nuvoton Technology Corp. All rights reserved.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either m_uiVersion 2 of the License, or
//      (at your option) any later m_uiVersion.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "Render.h"
#include "PCMPlayback.h"

#include "N9H26_VPOST.h"

#define VPOST_FRAMEBUFFER_SIZE		(LCD_PANEL_WIDTH  *LCD_PANEL_HEIGHT * 2)

static uint8_t s_au8BlankFrameBuffer[VPOST_FRAMEBUFFER_SIZE];
static uint8_t *s_pu8CurFBAddr;

extern void vpostSetFrameBuffer(UINT32 pFramebuf);


static void VPOST_InterruptServiceRiuntine()
{
	vpostSetFrameBuffer((uint32_t)s_pu8CurFBAddr);
}


static void InitVPOST(uint8_t* pu8FrameBuffer)
{		
	PFN_DRVVPOST_INT_CALLBACK fun_ptr;
	LCDFORMATEX lcdFormat;	

	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;//DRVVPOST_FRAME_YCBYCR;  //DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = LCD_PANEL_WIDTH;
	lcdFormat.nScreenHeight = LCD_PANEL_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	
}	

///////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t s_u32PCMPlaybackBufSize;
uint8_t *s_pu8PCMPlaybackBuf;
uint32_t s_u32DataLenInPCMBuf;

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void
Render_VideoFlush(
	S_NM_VIDEO_CTX	*psVideoCtx
)
{
		uint64_t u64CurTime;
		u64CurTime = NMUtil_GetTimeMilliSec();
		printf("Render_VideoFlush %d, %d\n", (uint32_t)u64CurTime, (uint32_t)psVideoCtx->u64DataTime);

		s_pu8CurFBAddr = psVideoCtx->pu8DataBuf;
}

void
Render_AudioFlush(
	S_NM_AUDIO_CTX	*psAudioCtx
)
{
	uint64_t u64CurTime;
	u64CurTime = NMUtil_GetTimeMilliSec();
	
//	printf("Render_AudioFlush %d, %d, %d\n", (uint32_t)u64CurTime, (uint32_t)psAudioCtx->u64DataTime, psAudioCtx->u32DataSize);

	if(psAudioCtx->u32DataSize <= s_u32PCMPlaybackBufSize - s_u32DataLenInPCMBuf){
		memcpy(s_pu8PCMPlaybackBuf + s_u32DataLenInPCMBuf, psAudioCtx->pu8DataBuf, psAudioCtx->u32DataSize);
		s_u32DataLenInPCMBuf += psAudioCtx->u32DataSize;
	}
	else{
		NMLOG_INFO("No buffer space for PCM data psG711DecFrame->u32FrameSize %d, u32DataLenInPCMBuf %d\n", psAudioCtx->u32DataSize, s_u32DataLenInPCMBuf);
	}

	if(s_u32DataLenInPCMBuf){
		PCMPlayback_Send(s_pu8PCMPlaybackBuf, &s_u32DataLenInPCMBuf);
	}
}

int
Render_Init(
	uint32_t u32AudioSampleRate,
	uint32_t u32AudioChannel
)
{
	uint32_t u32SPUFragSize;
	
	//Init VPOST
	memset(s_au8BlankFrameBuffer, 0x0, VPOST_FRAMEBUFFER_SIZE);
	s_pu8CurFBAddr = s_au8BlankFrameBuffer;
	InitVPOST(s_pu8CurFBAddr);

	//Enable SPU
	if(PCMPlayback_Start(u32AudioSampleRate, u32AudioChannel, 80, &u32SPUFragSize) != 0){
		printf("Unable start PCM playback \n");
		vpostLCMDeinit();
		return -1;
	}

	s_u32PCMPlaybackBufSize = u32AudioSampleRate * u32AudioChannel * 2;	//1Sec buffer size
	s_pu8PCMPlaybackBuf = malloc(s_u32PCMPlaybackBufSize);
	s_u32DataLenInPCMBuf = 0;
	
	if(s_pu8PCMPlaybackBuf == NULL){
		printf("Unable allocate playback buffer\n");
		PCMPlayback_Stop();
		vpostLCMDeinit();
		return -2;
	}
	
	return 0;
}

void
Render_Final(void)
{
	if(s_pu8PCMPlaybackBuf){
		PCMPlayback_Stop();
		free(s_pu8PCMPlaybackBuf);
		s_pu8PCMPlaybackBuf = NULL;
		s_u32PCMPlaybackBufSize = 0;
	}

	vpostLCMDeinit();
}






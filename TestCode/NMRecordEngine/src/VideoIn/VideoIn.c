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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "Semphr.h"

#include "wblib.h"
#include "N9H26_VideoIn.h"

#include "VinConfig.h"
#include "VideoIn.h"
#include "SensorIf.h"

#include "NVTMedia_Log.h"

#if VIN_CONFIG_PORT0_PLANAR_MAX_FRAME_BUF == 1
#define __VIN_ONE_SHOT_MODE__
#endif

#define DEF_VIN_MAX_PORTS	2

struct S_VIN_CTRL;

typedef struct {
	struct S_VIN_CTRL *psVinCtrl;

	uint32_t u32PortNo;
	uint32_t u32PacketMaxFrameBuf;
	uint32_t u32PlanarMaxFrameBuf;	
	BOOL	*pabIsPacketBufFull;
	BOOL	*pabIsPlanarBufFull;
	uint32_t u32PacketInIdx;
	uint32_t u32PlanarInIdx;
	uint8_t **apu8PlanarFrameBufPtr;
	uint8_t **apu8PacketFrameBufPtr;
	S_VIN_PIPE_INFO *psPlanarPipeInfo;
	S_VIN_PIPE_INFO *psPacketPipeInfo;

	VINDEV_T sVinDev;
	S_SENSOR_ATTR sSnrAttr;
	S_SENSOR_IF *psSnrIf;

	SemaphoreHandle_t tVinISRSem;	//video-in interrupt semaphore
	xSemaphoreHandle tVinStateMutex;

	BOOL bNightModeSwitch;

	uint32_t u32AEWinStartX;
	uint32_t u32AEWinStartY;
	uint32_t u32AEWinEndX;
	uint32_t u32AEWinEndY;

	uint32_t u32AEMaxTargetLumConfig;

}S_VIN_PORT_OP;

typedef struct S_VIN_CTRL{
	S_VIN_CONFIG sVinConfig;
	S_VIN_PORT_OP asPortOP[DEF_VIN_MAX_PORTS];
	uint32_t u32WorkingPorts;
} S_VIN_CTRL;


// Video-in port0 packet and planar pipe buffers
#if defined (VIN_CONFIG_USE_1ST_PORT)

	static BOOL s_abIsPacketBufFull[VIN_CONFIG_PORT0_PACKET_FRAME_BUF];
	static BOOL s_abIsPlanarBufFull[VIN_CONFIG_PORT0_PLANAR_FRAME_BUF];

#pragma arm section zidata = "non_initialized"

#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 1
		#if VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8PlanarFrameBuffer0[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8PlanarFrameBuffer0[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*2) + 256];
		#endif
	#endif

	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 2
		#if VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8PlanarFrameBuffer1[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8PlanarFrameBuffer1[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*2)+ 256];
		#endif
	#endif

	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 3
		#if VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8PlanarFrameBuffer2[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8PlanarFrameBuffer2[(VIN_CONFIG_PORT0_PLANAR_WIDTH*VIN_CONFIG_PORT0_PLANAR_HEIGHT*2)+ 256];
		#endif
	#endif

	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 1
		__align(256) static uint8_t s_au8PacketFrameBuffer0[(VIN_CONFIG_PORT0_PACKET_WIDTH*VIN_CONFIG_PORT0_PACKET_HEIGHT*2)+ 256];		
	#endif

	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 2
		__align(256) static uint8_t s_au8PacketFrameBuffer1[(VIN_CONFIG_PORT0_PACKET_WIDTH*VIN_CONFIG_PORT0_PACKET_HEIGHT*2)+ 256];
	#endif

	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 3
		__align(256) static uint8_t s_au8PacketFrameBuffer2[(VIN_CONFIG_PORT0_PACKET_WIDTH*VIN_CONFIG_PORT0_PACKET_HEIGHT*2)+ 256];	
	#endif

#pragma arm section zidata // back to default (.bss section)

	static uint8_t *s_apu8PlanarFrameBufPtr[VIN_CONFIG_PORT0_PLANAR_FRAME_BUF] = {
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 1
		s_au8PlanarFrameBuffer0,
	#endif
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 2
		s_au8PlanarFrameBuffer1,
	#endif
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 3
		s_au8PlanarFrameBuffer2,
	#endif

	};

	static uint8_t *s_apu8PacketFrameBufPtr[VIN_CONFIG_PORT0_PACKET_FRAME_BUF] = {
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 1
		s_au8PacketFrameBuffer0,
	#endif
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 2
		s_au8PacketFrameBuffer1,
	#endif
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 3
		s_au8PacketFrameBuffer2,
	#endif
	};

	static uint32_t s_u32PlanarFrameBufSize = sizeof(s_au8PlanarFrameBuffer0);
	static uint32_t s_u32PacketFrameBufSize = sizeof(s_au8PacketFrameBuffer0);

#else

	static uint8_t *s_apu8PlanarFrameBufPtr[VIN_CONFIG_PORT0_PLANAR_FRAME_BUF] = {
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 1
		NULL,
	#endif
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 2
		NULL,
	#endif
	#if VIN_CONFIG_PORT0_PLANAR_FRAME_BUF >= 3
		NULL,
	#endif

	};

	static uint8_t *s_apu8PacketFrameBufPtr[VIN_CONFIG_PORT0_PACKET_FRAME_BUF] = {
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 1
		NULL,
	#endif
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 2
		NULL,
	#endif
	#if VIN_CONFIG_PORT0_PACKET_FRAME_BUF >= 3
		NULL,
	#endif
	};

	static uint32_t s_u32PlanarFrameBufSize = 0;
	static uint32_t s_u32PacketFrameBufSize = 0;

#endif


// Video-in port1 packet and planar pipe buffers
#if defined (VIN_CONFIG_USE_2ND_PORT)

	static BOOL s_abIsPort1PacketBufFull[VIN_CONFIG_PORT1_PACKET_FRAME_BUF];
	static BOOL s_abIsPort1PlanarBufFull[VIN_CONFIG_PORT1_PLANAR_FRAME_BUF];

#pragma arm section zidata = "non_initialized"
	
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 1
		#if VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer0[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer0[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*2)];
		#endif
	#endif

	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 2
		#if VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer1[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer1[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*2)];
		#endif
	#endif

	#if VIN_CONFIG_PORT1_PLANAR_MAX_FRAME_BUF >= 3
		#if VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer2[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*3/2) + 256];
		#else
			__align(256) static uint8_t s_au8Port1PlanarFrameBuffer2[(VIN_CONFIG_PORT1_PLANAR_WIDTH*VIN_CONFIG_PORT1_PLANAR_HEIGHT*2)];
		#endif
	#endif

	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 1
		__align(256) static uint8_t s_au8Port1PacketFrameBuffer0[(VIN_CONFIG_PORT1_PACKET_WIDTH*VIN_CONFIG_PORT1_PACKET_HEIGHT*2)];		
	#endif

	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 2
		__align(256) static uint8_t s_au8Port1PacketFrameBuffer1[(VIN_CONFIG_PORT1_PACKET_WIDTH*VIN_CONFIG_PORT1_PACKET_HEIGHT*2)];
	#endif

	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 3
		__align(256) static uint8_t s_au8Port1PacketFrameBuffer2[(VIN_CONFIG_PORT1_PACKET_WIDTH*VIN_CONFIG_PORT1_PACKET_HEIGHT*2)];	
	#endif

#pragma arm section zidata // back to default (.bss section)

	static uint8_t *s_apu8Port1PlanarFrameBufPtr[VIN_CONFIG_PORT1_PLANAR_FRAME_BUF] = {
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 1
		s_au8Port1PlanarFrameBuffer0,
	#endif
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 2
		s_au8Port1PlanarFrameBuffer1,
	#endif
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 3
		s_au8Port1PlanarFrameBuffer2,
	#endif

	};

	static uint8_t *s_apu8Port1PacketFrameBufPtr[VIN_CONFIG_PORT1_PACKET_FRAME_BUF] = {
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 1
		s_au8Port1PacketFrameBuffer0,
	#endif
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 2
		s_au8Port1PacketFrameBuffer1,
	#endif
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 3
		s_au8Port1PacketFrameBuffer2,
	#endif
	};

	static uint32_t s_u32Port1PlanarFrameBufSize = sizeof(s_au8Port1PlanarFrameBuffer0);
	static uint32_t s_u32Port1PacketFrameBufSize = sizeof(s_au8Port1PacketFrameBuffer0);
	
#else

	static uint8_t *s_apu8Port1PlanarFrameBufPtr[VIN_CONFIG_PORT1_PLANAR_FRAME_BUF] = {
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 1
		NULL,
	#endif
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 2
		NULL,
	#endif
	#if VIN_CONFIG_PORT1_PLANAR_FRAME_BUF >= 3
		NULL,
	#endif

	};

	static uint8_t *s_apu8Port1PacketFrameBufPtr[VIN_CONFIG_PORT1_PACKET_FRAME_BUF] = {
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 1
		NULL,
	#endif
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 2
		NULL,
	#endif
	#if VIN_CONFIG_PORT1_PACKET_FRAME_BUF >= 3
		NULL,
	#endif
	};

	static uint32_t s_u32Port1PlanarFrameBufSize = 0;
	static uint32_t s_u32Port1PacketFrameBufSize = 0;

#endif
	

	

static S_VIN_CTRL s_sVinCtrl;

static S_VIN_PORT_OP *s_psPortOP0 = NULL;
static S_VIN_PORT_OP *s_psPortOP1 = NULL;

static void VideoInFrameBufSwitch(
	S_VIN_PORT_OP *psPortOP
)
{
	uint16_t u16PlanarWidth, u16PlanarHeight;
	S_VIN_PIPE_INFO *psPlanarPipeInfo;

	switch(psPortOP->u32PacketInIdx)
	{
		case 0:
			psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
			if(psPortOP->u32PacketMaxFrameBuf > 1){
				if(psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx + 1] == FALSE){ 		//If packet buffer 1 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketInIdx + 1]) );																	
					psPortOP->u32PacketInIdx = psPortOP->u32PacketInIdx + 1;
				}
			}
		break;
		case 1:
			psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
			if(psPortOP->u32PacketMaxFrameBuf > 2){
				if(psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx + 1] == FALSE){ 		//If packet buffer 2 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketInIdx + 1]) );																	
					psPortOP->u32PacketInIdx = psPortOP->u32PacketInIdx + 1;
				}
			}
			else{
				if(psPortOP->pabIsPacketBufFull[0] == FALSE){ 		//If packet buffer 0 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[0]) );																	
					psPortOP->u32PacketInIdx = 0;
				}
			}
		break;
		case 2:
			psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
			if(psPortOP->pabIsPacketBufFull[0] == FALSE){ 		//If packet buffer 0 is empty change to it.
				psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[0]) );																	
				psPortOP->u32PacketInIdx = 0;
			}
		break;
	}

	psPlanarPipeInfo = psPortOP->psPlanarPipeInfo;
	u16PlanarWidth = psPlanarPipeInfo->u32Width;
	u16PlanarHeight = psPlanarPipeInfo->u32Height;
	
	switch(psPortOP->u32PlanarInIdx)
	{
		case 0:
			psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
			if(psPortOP->u32PlanarMaxFrameBuf > 1){
				if(psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx + 1] == FALSE){ 		//If planar buffer 1 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1]) );	
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth*u16PlanarHeight));	    		

					if(psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB)
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth*u16PlanarHeight));	 
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV422P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/2))));	
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/4))));	
	
					psPortOP->u32PlanarInIdx = psPortOP->u32PlanarInIdx + 1;
				}
			}
		break;
		case 1:
			psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
			if(psPortOP->u32PlanarMaxFrameBuf > 2){
				if(psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx + 1] == FALSE){ 		//If packet buffer 2 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1]) );	
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth*u16PlanarHeight));	    		

					if(psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB)
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth*u16PlanarHeight));	 
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV422P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/2))));	
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/4))));	

					psPortOP->u32PlanarInIdx = psPortOP->u32PlanarInIdx + 1;
				}
			}
			else{
				if(psPortOP->pabIsPlanarBufFull[0] == FALSE){ 		//If packet buffer 0 is empty change to it.
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0]) );	
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth*u16PlanarHeight));
					
					if(psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB)
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth*u16PlanarHeight));	
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV422P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/2))));	
					else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P))
						psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + ((u16PlanarWidth*u16PlanarHeight)+((u16PlanarWidth*u16PlanarHeight)/4))));	

					psPortOP->u32PlanarInIdx = 0;
				}
			}
		break;
		case 2:
			psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
			if(psPortOP->pabIsPlanarBufFull[0] == FALSE){ 		//If packet buffer 0 is empty change to it.
				psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0]) );	
				psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth*u16PlanarHeight));
				
				if(psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P_MB)
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth*u16PlanarHeight));
				else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV422P))
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + (u16PlanarWidth*u16PlanarHeight+u16PlanarWidth*u16PlanarHeight/2)));	
				else if ((psPlanarPipeInfo->eColorType == eVIN_COLOR_YUV420P))
					psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + (u16PlanarWidth*u16PlanarHeight+u16PlanarWidth*u16PlanarHeight/4)));	

				psPortOP->u32PlanarInIdx = 0;
			}
			
		break;
	}
}

//Port 0 interrupt handle
void VideoIn_InterruptHandler(
	uint8_t u8PacketBufID,
	uint8_t u8PlanarBufID, 
	uint8_t u8FrameRate,
	uint8_t u8Filed
)
{
	//Handle the irq and use freeRTOS semaphore to wakeup vin thread
	S_VIN_PORT_OP *psPortOP = s_psPortOP0;
	portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;
	
	if(psPortOP == NULL)
		return;
	
	VideoInFrameBufSwitch(psPortOP);

#if !defined (__VIN_ONE_SHOT_MODE__)
	psPortOP->sVinDev.SetShadowRegister();
#endif

	xSemaphoreGiveFromISR(psPortOP->tVinISRSem, &xHigherPrioTaskWoken);
	if (xHigherPrioTaskWoken == pdTRUE){
		portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
	}
}

//Port 1 interrupt handle
void VideoIn_Port1InterruptHandler(
	uint8_t u8PacketBufID,
	uint8_t u8PlanarBufID, 
	uint8_t u8FrameRate,
	uint8_t u8Filed
)
{
	S_VIN_PORT_OP *psPortOP = s_psPortOP1;
	portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;

	VideoInFrameBufSwitch(psPortOP);

	
#if !defined (__VIN_ONE_SHOT_MODE__)
	psPortOP->sVinDev.SetShadowRegister();
#endif

	xSemaphoreGiveFromISR(psPortOP->tVinISRSem, &xHigherPrioTaskWoken);
	if (xHigherPrioTaskWoken == pdTRUE){
		portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
	}
}

static int
OpenVinDev(
	S_VIN_PORT_OP *psPortOP
)
{
	int32_t i;
	uint32_t u32PortNo = psPortOP->u32PortNo;

	if(u32PortNo == 0){
		//show video-in packet frame buffer
		for (i = 0; i < psPortOP->u32PacketMaxFrameBuf; i ++){
			NMLOG_INFO("s_apu8PacketFrameBufPtr[%d] =  %x \n", i, s_apu8PacketFrameBufPtr[i]);
		}

		for (i = 0; i <  psPortOP->u32PlanarMaxFrameBuf; i ++){
			NMLOG_INFO("s_apu8PlanarFrameBufPtr[%d] =  %x \n", i, s_apu8PlanarFrameBufPtr[i]);
		}
	}
	else if(u32PortNo == 1){
		//show video-in packet frame buffer
		for (i = 0; i < psPortOP->u32PacketMaxFrameBuf; i ++){
			NMLOG_INFO("s_apu8PacketFrameBufPtr[%d] =  %x \n", i, s_apu8Port1PacketFrameBufPtr[i]);
		}

		for (i = 0; i <  psPortOP->u32PlanarMaxFrameBuf; i ++){
			NMLOG_INFO("s_apu8PlanarFrameBufPtr[%d] =  %x \n", i, s_apu8Port1PlanarFrameBufPtr[i]);
		}
	}

#if VIN_CONFIG_SENSOR_NT99141 == 1
	psPortOP->psSnrIf = &g_sSensorNT99141;
#elif VIN_CONFIG_SENSOR_NT99142 == 1
	psPortOP->psSnrIf = &g_sSensorNT99142;
#elif VIN_CONFIG_SENSOR_GC0308 == 1
	psPortOP->psSnrIf = &g_sSensorGC0308;
#elif VIN_CONFIG_SENSOR_HM2056 == 1
	psPortOP->psSnrIf = &g_sSensorHM2056;
#endif	

	if(psPortOP->psSnrIf == NULL)
		return -1;

	if(u32PortNo == 0){
		psPortOP->sSnrAttr.u32PlanarWidth = VIN_CONFIG_PORT0_PLANAR_WIDTH;
		psPortOP->sSnrAttr.u32PlanarHeight = VIN_CONFIG_PORT0_PLANAR_HEIGHT;
		psPortOP->sSnrAttr.u32PacketWidth = VIN_CONFIG_PORT0_PACKET_WIDTH;
		psPortOP->sSnrAttr.u32PacketHeight = VIN_CONFIG_PORT0_PACKET_HEIGHT;

#if (VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_MACRO_PLANAR_YUV420;
#elif (VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_YUV422)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV422;
#elif (VIN_CONFIG_PORT0_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_YUV420)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV420;
#else
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV420;		
#endif

		psPortOP->sSnrAttr.u32FrameRate = VIN_CONFIG_PORT0_FRAME_RATE;
		psPortOP->sSnrAttr.u32LowLuxGate = VIN_CONFIG_PORT0_LOW_LUX_GATE;
		psPortOP->sSnrAttr.u32HighLuxGate = VIN_CONFIG_PORT0_HIGH_LUX_GATE;
		psPortOP->bNightModeSwitch = VIN_CONFIG_PORT0_NIGHT_MODE;
		psPortOP->sSnrAttr.u32Flicker = VIN_CONFIG_PORT0_FLICKER;
		psPortOP->sSnrAttr.u32AEMaxLum = VIN_CONFIG_PORT0_AE_MAX_LUM;
		psPortOP->sSnrAttr.u32AEMinLum = VIN_CONFIG_PORT0_AE_MIN_LUM;
		psPortOP->u32AEMaxTargetLumConfig = psPortOP->sSnrAttr.u32AEMaxLum;
	}
	else if(u32PortNo == 1){
		psPortOP->sSnrAttr.u32PlanarWidth = VIN_CONFIG_PORT1_PLANAR_WIDTH;
		psPortOP->sSnrAttr.u32PlanarHeight = VIN_CONFIG_PORT1_PLANAR_HEIGHT;
		psPortOP->sSnrAttr.u32PacketWidth = VIN_CONFIG_PORT1_PACKET_WIDTH;
		psPortOP->sSnrAttr.u32PacketHeight = VIN_CONFIG_PORT1_PACKET_HEIGHT;

#if (VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_MACRO_PLANAR_YUV420;
#elif (VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_YUV422)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV422;
#elif (VIN_CONFIG_PORT1_PLANAR_FORMAT == VIN_CONFIG_COLOR_PLANAR_YUV420)
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV420;
#else
		psPortOP->sSnrAttr.ePlanarColorFormat = eVIDEOIN_PLANAR_YUV420;		
#endif

		psPortOP->sSnrAttr.u32FrameRate = VIN_CONFIG_PORT1_FRAME_RATE;
		psPortOP->sSnrAttr.u32LowLuxGate = VIN_CONFIG_PORT1_LOW_LUX_GATE;
		psPortOP->sSnrAttr.u32HighLuxGate = VIN_CONFIG_PORT1_HIGH_LUX_GATE;
		psPortOP->bNightModeSwitch = VIN_CONFIG_PORT1_NIGHT_MODE;
		psPortOP->sSnrAttr.u32Flicker = VIN_CONFIG_PORT1_FLICKER;
		psPortOP->sSnrAttr.u32AEMaxLum = VIN_CONFIG_PORT1_AE_MAX_LUM;
		psPortOP->sSnrAttr.u32AEMinLum = VIN_CONFIG_PORT1_AE_MIN_LUM;
		psPortOP->u32AEMaxTargetLumConfig = psPortOP->sSnrAttr.u32AEMaxLum;

	}

	uint32_t u32PlanarFrameBufSize = 0;
	uint32_t u32PacketFrameBufSize = 0;
	
	if(u32PortNo == 0){
		psPortOP->sSnrAttr.pu8PacketFrameBuf = s_apu8PacketFrameBufPtr[0];
		psPortOP->sSnrAttr.pu8PlanarFrameBuf = s_apu8PlanarFrameBufPtr[0];
		u32PlanarFrameBufSize = s_u32PlanarFrameBufSize;
		u32PacketFrameBufSize = s_u32PacketFrameBufSize;
	}
	else if(u32PortNo == 1){
		psPortOP->sSnrAttr.pu8PacketFrameBuf = s_apu8Port1PacketFrameBufPtr[0];
		psPortOP->sSnrAttr.pu8PlanarFrameBuf = s_apu8Port1PlanarFrameBufPtr[0];
		u32PlanarFrameBufSize = s_u32Port1PlanarFrameBufSize;
		u32PacketFrameBufSize = s_u32Port1PacketFrameBufSize;
	}

	psPortOP->sSnrAttr.ePacketColorFormat = eVIDEOIN_OUT_YUV422;

	if(psPortOP->psPlanarPipeInfo){
		psPortOP->psPlanarPipeInfo->u32Width = psPortOP->sSnrAttr.u32PlanarWidth;
		psPortOP->psPlanarPipeInfo->u32Height = psPortOP->sSnrAttr.u32PlanarHeight;

		psPortOP->psPlanarPipeInfo->u32FramePhyAddr = (uint32_t)psPortOP->sSnrAttr.pu8PlanarFrameBuf;
		psPortOP->psPlanarPipeInfo->u32FrameBufSize = u32PlanarFrameBufSize;
		psPortOP->psPlanarPipeInfo->u32FrameRate = psPortOP->sSnrAttr.u32FrameRate;

		//Set AE window to 1/4 frame
		psPortOP->u32AEWinStartX = (psPortOP->psPlanarPipeInfo->u32Width * 4) / 16;
//		psPortOP->u32AEWinStartY = (psPortOP->psPlanarPipeInfo->u32Height * 4) / 16;
		psPortOP->u32AEWinStartY = (psPortOP->psPlanarPipeInfo->u32Height * 8) / 16;
		psPortOP->u32AEWinEndX = (psPortOP->psPlanarPipeInfo->u32Width * 12) / 16;
//		psPortOP->u32AEWinEndY = (psPortOP->psPlanarPipeInfo->u32Height * 12) / 16;
		psPortOP->u32AEWinEndY = (psPortOP->psPlanarPipeInfo->u32Height * 16) / 16;
	}
	
	if(psPortOP->psPacketPipeInfo){
		psPortOP->psPacketPipeInfo->u32Width = psPortOP->sSnrAttr.u32PacketWidth;
		psPortOP->psPacketPipeInfo->u32Height = psPortOP->sSnrAttr.u32PacketHeight;

		psPortOP->psPacketPipeInfo->u32FramePhyAddr = (uint32_t)psPortOP->sSnrAttr.pu8PacketFrameBuf;	
		psPortOP->psPacketPipeInfo->u32FrameBufSize = u32PacketFrameBufSize;
		psPortOP->psPacketPipeInfo->u32FrameRate = psPortOP->sSnrAttr.u32FrameRate;
	}
	
	if(u32PortNo == 0){
		if (psPortOP->psSnrIf->pfnSenesorSetup(&psPortOP->sVinDev, &psPortOP->sSnrAttr, u32PortNo, TRUE, VideoIn_InterruptHandler) != 0){
			NMLOG_ERROR("Unable setup sensor \n");
			return -1;
		}
	}
	else{
		if (psPortOP->psSnrIf->pfnSenesorSetup(&psPortOP->sVinDev, &psPortOP->sSnrAttr, u32PortNo, FALSE, VideoIn_Port1InterruptHandler) != 0){
			NMLOG_ERROR("Unable setup sensor \n");
			return -1;
		}
	}

	if(psPortOP->psSnrIf->pfnSetFlicker)
		psPortOP->psSnrIf->pfnSetFlicker(&psPortOP->sSnrAttr);
	
#if defined (__VIN_ONE_SHOT_MODE__)
	psPortOP->sVinDev.SetOperationMode(TRUE);
#endif
	
	return 0;
}

	
static int
VideoInDeviceInit(
	S_VIN_CTRL *psVinCtrl
)
{
	uint32_t u32WorkingPorts = 0;
	uint32_t u32WorkingPipes = 0;
	S_VIN_PORT_OP *psPortOP;

#if defined (VIN_CONFIG_USE_1ST_PORT)
	if(u32WorkingPorts < DEF_VIN_MAX_PORTS){

		psPortOP = &psVinCtrl->asPortOP[u32WorkingPorts];
		
		psPortOP->psVinCtrl = psVinCtrl;
		psPortOP->u32PortNo = 0;
		psPortOP->u32PacketMaxFrameBuf = VIN_CONFIG_PORT0_PACKET_FRAME_BUF;
		psPortOP->u32PlanarMaxFrameBuf = VIN_CONFIG_PORT0_PLANAR_FRAME_BUF;
		psPortOP->pabIsPacketBufFull = s_abIsPacketBufFull;
		psPortOP->pabIsPlanarBufFull = s_abIsPlanarBufFull;
		psPortOP->u32PacketInIdx = 0;
		psPortOP->u32PlanarInIdx = 0;
		psPortOP->apu8PlanarFrameBufPtr = s_apu8PlanarFrameBufPtr;
		psPortOP->apu8PacketFrameBufPtr = s_apu8PacketFrameBufPtr;
		
		if(u32WorkingPipes < DEF_VIN_MAX_PIPES){
			psPortOP->psPlanarPipeInfo =  &psVinCtrl->sVinConfig.asVinPipeInfo[u32WorkingPipes];  
			psPortOP->psPlanarPipeInfo->u32PipeNo = u32WorkingPipes;
			u32WorkingPipes++;
		}
		else{
			psPortOP->psPlanarPipeInfo = NULL;
		}
		
		if(u32WorkingPipes < DEF_VIN_MAX_PIPES){
			psPortOP->psPacketPipeInfo =  &psVinCtrl->sVinConfig.asVinPipeInfo[u32WorkingPipes];  
			psPortOP->psPacketPipeInfo->u32PipeNo = u32WorkingPipes;
			u32WorkingPipes++;
		}
		else{
			psPortOP->psPacketPipeInfo = NULL;
		}

		s_psPortOP0 = psPortOP;

		psPortOP->tVinISRSem = xSemaphoreCreateBinary();

		if(psPortOP->tVinISRSem == NULL){
			NMLOG_ERROR("Unable create video-in interrupt semaphore \n");
			return -1;
		}

		psPortOP->tVinStateMutex = xSemaphoreCreateMutex();
		
		if(psPortOP->tVinStateMutex == NULL){
			NMLOG_ERROR("Unable create video-in state mutex \n");
			return -2;
		}
		
		if (OpenVinDev(psPortOP) != 0){
			NMLOG_ERROR("Open video-in device failed \n");
			return -3;
		}

		u32WorkingPorts++;
	}
#endif

#if defined (VIN_CONFIG_USE_2ND_PORT)
	if(u32WorkingPorts < DEF_VIN_MAX_PORTS){
		psPortOP = &psVinCtrl->asPortOP[u32WorkingPorts];
		
		psPortOP->psVinCtrl = psVinCtrl;
		psPortOP->u32PortNo = 1;
		psPortOP->u32PacketMaxFrameBuf = VIN_CONFIG_PORT1_PACKET_FRAME_BUF;
		psPortOP->u32PlanarMaxFrameBuf = VIN_CONFIG_PORT1_PLANAR_FRAME_BUF;
		psPortOP->pabIsPacketBufFull = s_abIsPort1PacketBufFull;
		psPortOP->pabIsPlanarBufFull = s_abIsPort1PlanarBufFull;
		psPortOP->u32PacketInIdx = 0;
		psPortOP->u32PlanarInIdx = 0;
		psPortOP->apu8PlanarFrameBufPtr = s_apu8Port1PlanarFrameBufPtr;
		psPortOP->apu8PacketFrameBufPtr = s_apu8Port1PacketFrameBufPtr;

		if(u32WorkingPipes < DEF_VIN_MAX_PIPES){
			psPortOP->psPlanarPipeInfo =  &psVinCtrl->sVinConfig.asVinPipeInfo[u32WorkingPipes];  
			psPortOP->psPlanarPipeInfo->u32PipeNo = u32WorkingPipes;
			u32WorkingPipes++;
		}
		else{
			psPortOP->psPlanarPipeInfo = NULL;
		}
		
		if(u32WorkingPipes < DEF_VIN_MAX_PIPES){
			psPortOP->psPacketPipeInfo =  &psVinCtrl->sVinConfig.asVinPipeInfo[u32WorkingPipes];  
			psPortOP->psPacketPipeInfo->u32PipeNo = u32WorkingPipes;
			u32WorkingPipes++;
		}
		else{
			psPortOP->psPacketPipeInfo = NULL;
		}

		s_psPortOP1 = psPortOP;

		psPortOP->tVinISRSem = xSemaphoreCreateBinary();

		if(psPortOP->tVinISRSem == NULL){
			NMLOG_ERROR("Unable create video-in interrupt semaphore \n");
			return -1;
		}

		psPortOP->tVinStateMutex = xSemaphoreCreateMutex();
		
		if(psPortOP->tVinStateMutex == NULL){
			NMLOG_ERROR("Unable create video-in state mutex \n");
			return -2;
		}

		if (OpenVinDev(psPortOP) != 0){
			NMLOG_ERROR("Open video-in device failed \n");
			return -3;
		}

		u32WorkingPorts++;

	}
#endif

	psVinCtrl->u32WorkingPorts = u32WorkingPorts;
	psVinCtrl->sVinConfig.u32NumPipes = u32WorkingPipes;
	
	return 0;
}




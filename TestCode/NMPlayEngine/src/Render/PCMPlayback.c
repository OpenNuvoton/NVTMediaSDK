//		Copyright (c) 2017 Nuvoton Technology Corp. All rights reserved.
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

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "N9H26_SPU.h"
#include "PCMPlayback.h"
#include "NVTMedia.h"

typedef struct{
	uint8_t		*pu8Buf;
	uint32_t	u32Size;
	uint32_t 	u32WriteIdx;
	uint32_t 	u32ReadIdx;
	SemaphoreHandle_t tBufMutex;
	volatile BOOL bAccessISR;
}S_PCM_BUF_MGR;

static S_PCM_BUF_MGR s_sPCMBufMgr; 
static uint32_t s_u32DACFragmentSize;

static uint8_t *s_pu8ISRPcmBuf; 
static SemaphoreHandle_t s_tPCMPuncherSem;
static int32_t s_i32SPUVolume = 0x0;

static uint32_t s_u32EmptyAudioCount = 0;
static uint64_t s_u64EmptyAudioTotalTime = 0;
static uint64_t s_u64EmptyAudioStartTime = 0;
static xTaskHandle s_hPunckerTaskHandle = 0;	

#define CTRL_PA_PIN	//PA control move to SystemTask.c

#if defined (CTRL_PA_PIN)

#include "N9H26_GPIO.h"

//GPA0 for speaker PA enable pin
#define PA_ENABLE_MFP_PORT		REG_GPAFUN0
#define PA_ENABLE_MFP_PIN		MF_GPA0
#define PA_ENABLE_GPIO_PORT	GPIO_PORTA
#define PA_ENABLE_GPIO_PIN		BIT0
#endif

static BOOL s_bPAEnabled = FALSE;

//
//static volatile BOOL s_bWakeup = FALSE;
//static uint64_t u64ISRTriggTime = 0;
//static uint64_t u64ISRDiffTime;

//Called by SPU end address and threshold address inturrept. Copy half fragment PCM data to pu8Data
static int
DACIsrCallback(
	uint8_t *pu8Data
)
{
	BaseType_t xHigherPriorityTaskWoken;
	int32_t i32HalfFrag = s_u32DACFragmentSize / 2;

	if(s_pu8ISRPcmBuf == NULL){
		memset(pu8Data, 0, i32HalfFrag);
		return 0;
	}
		
	memcpy(pu8Data, s_pu8ISRPcmBuf, i32HalfFrag);
	memset(s_pu8ISRPcmBuf, 0, i32HalfFrag);
	
	xSemaphoreGiveFromISR(s_tPCMPuncherSem, &xHigherPriorityTaskWoken);
		
	if(xHigherPriorityTaskWoken){
		portEXIT_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}

	return 0;
}

//Punch PCM data to PCM ISR buffer
static void PCMPuncherTask(
	void *pvArg
)
{
	S_PCM_BUF_MGR *psPCMBufMgr = (S_PCM_BUF_MGR *) pvArg;
	int32_t i32HalfFrag = s_u32DACFragmentSize / 2;
	BOOL bEmpty = FALSE;

	spuIoctl(SPU_IOCTL_SET_VOLUME, s_i32SPUVolume, s_i32SPUVolume);	
	while(1){

		//Wait DAC ISR semphore
		if(xSemaphoreTake(s_tPCMPuncherSem, portMAX_DELAY) == pdFALSE){
			break;
		}
		
		xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);
		
		if(bEmpty == TRUE){
			if((psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx) < i32HalfFrag){
				//PCM data still not enough
				xSemaphoreGive(psPCMBufMgr->tBufMutex);
				vTaskDelay(5 / portTICK_RATE_MS);	
				continue;
			}
			else{
				//Calcute PCM buffer empty status
				uint32_t u32AverageEmptyTime;

				spuIoctl(SPU_IOCTL_SET_VOLUME, s_i32SPUVolume, s_i32SPUVolume);		
				s_u32EmptyAudioCount ++;
				s_u64EmptyAudioTotalTime += NMUtil_GetTimeMilliSec() - s_u64EmptyAudioStartTime;
				u32AverageEmptyTime = s_u64EmptyAudioTotalTime / s_u32EmptyAudioCount;
				NMLOG_DEBUG("Audio empty count %d, average time %d \n", s_u32EmptyAudioCount, u32AverageEmptyTime);				
			}
		}
		else{
			//Detect PCM data empty or not
			if((psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx) < i32HalfFrag){
				xSemaphoreGive(psPCMBufMgr->tBufMutex);
				NMLOG_INFO("PCM buffer is empty\n");
				bEmpty = TRUE;				
				s_u64EmptyAudioStartTime = NMUtil_GetTimeMilliSec();
				spuIoctl(SPU_IOCTL_SET_VOLUME, 0, 0);		
				vTaskDelay(5 / portTICK_RATE_MS);	
				continue;
			}
		}
		
		//PCM data is enough to put PCM ISR buffer
		bEmpty = FALSE;	
		memcpy(s_pu8ISRPcmBuf, psPCMBufMgr->pu8Buf + psPCMBufMgr->u32ReadIdx, i32HalfFrag);
		psPCMBufMgr->u32ReadIdx += i32HalfFrag;

		xSemaphoreGive(psPCMBufMgr->tBufMutex);

	}
	
	NMLOG_ERROR("Exit PCM puncher task\n");

	vTaskDelete(NULL);
}

static uint32_t	//return dac buffer fragment
OpenDAC(
	int i32SampleRate,
	int i32Channel
)
{
	uint32_t u32FragmentSize = 0;

	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | SPU_CKE | HCLK4_CKE);  
	spuOpen(i32SampleRate);
	if(i32Channel == 1){
		spuIoctl(SPU_IOCTL_SET_MONO, NULL, NULL);	
	}
	else{
		spuIoctl(SPU_IOCTL_SET_STEREO, NULL, NULL);	
	}

	//SPU fragment size ~250ms 
	u32FragmentSize = i32SampleRate / 4 * i32Channel;
	//SPU fragment size alignment to 4096
	u32FragmentSize = u32FragmentSize & (~(4096 -1));
	if(u32FragmentSize == 0)
		u32FragmentSize = 4096;

	//Set SPU fragment size
	spuIoctl(SPU_IOCTL_SET_FRAG_SIZE, (uint32_t) &u32FragmentSize, NULL);	

	spuIoctl(SPU_IOCTL_GET_FRAG_SIZE, (uint32_t) &u32FragmentSize, NULL);	

	s_u32EmptyAudioCount = 0;
	s_u64EmptyAudioTotalTime = 0;
	s_u64EmptyAudioStartTime = 0;
	
	return u32FragmentSize;
}

static int
StartPlayDAC(
	int32_t i32Volume
)
{
	uint8_t *pu8SilentPCM;
	
	pu8SilentPCM = malloc(s_u32DACFragmentSize);
	
	if(!pu8SilentPCM)
		return -1;
	
	spuDacOn(2);

	vTaskDelay(1 / portTICK_RATE_MS);
	spuSetDacSlaveMode();

	memset(pu8SilentPCM, 0x0, s_u32DACFragmentSize);

	//SPU enable and pass a silent PCM data to SPU
	spuStartPlay(DACIsrCallback, pu8SilentPCM);

#if defined(CTRL_PA_PIN)
		//Set multi function pin
		if(s_bPAEnabled == FALSE){
			outp32(PA_ENABLE_MFP_PORT, inp32(PA_ENABLE_MFP_PORT) & (~PA_ENABLE_MFP_PIN));

			gpio_setportval(PA_ENABLE_GPIO_PORT, PA_ENABLE_GPIO_PIN, PA_ENABLE_GPIO_PIN);	//set high default
			gpio_setportpull(PA_ENABLE_GPIO_PORT, PA_ENABLE_GPIO_PIN, PA_ENABLE_GPIO_PIN);	//pull-up 
			gpio_setportdir(PA_ENABLE_GPIO_PORT, PA_ENABLE_GPIO_PIN, PA_ENABLE_GPIO_PIN);	//output mode 
			s_bPAEnabled = TRUE;
		}

		gpio_setportval(PA_ENABLE_GPIO_PORT, PA_ENABLE_GPIO_PIN, PA_ENABLE_GPIO_PIN);	//Set high
#endif

	s_i32SPUVolume = i32Volume;
	
	free(pu8SilentPCM);
	
	return 0;
}

static void
CloseDAC(void)
{
	s_i32SPUVolume = 0;
	spuIoctl(SPU_IOCTL_SET_VOLUME, s_i32SPUVolume, s_i32SPUVolume);		

#if defined(CTRL_PA_PIN)
//	gpio_setportval(PA_ENABLE_GPIO_PORT, PA_ENABLE_GPIO_PIN, ~PA_ENABLE_GPIO_PIN);	//set low
#endif

	spuStopPlay();
	spuClose();
}

int32_t
PCMPlayback_Start(
	uint32_t u32SampleRate,
	uint32_t u32Channel,
	int32_t i32Volume,
	uint32_t *pu32FragmentSize
)
{
	int32_t i32Ret = 0;
	if(pu32FragmentSize)
		*pu32FragmentSize = 0;

	if(s_hPunckerTaskHandle != NULL){
		NMLOG_ERROR("PCMPlayback under runing \n");
		return -1;
	}
	
	s_tPCMPuncherSem = NULL;
	s_pu8ISRPcmBuf = NULL;
	memset(&s_sPCMBufMgr, 0, sizeof(S_PCM_BUF_MGR));
	s_hPunckerTaskHandle = NULL;
	
	//Create puncher semaphore
	s_tPCMPuncherSem = xSemaphoreCreateBinary();
	if(s_tPCMPuncherSem == NULL){
		i32Ret = -2;
		goto PCMPlayback_Start_Fail;
	}

	//Open DAC
	s_u32DACFragmentSize = OpenDAC(u32SampleRate, u32Channel);

	//Allocate PCM ISR buffer
	s_pu8ISRPcmBuf = malloc(s_u32DACFragmentSize / 2);
	if(s_pu8ISRPcmBuf == NULL){
		i32Ret = -3;
		goto PCMPlayback_Start_Fail;		
	}

	memset(s_pu8ISRPcmBuf, 0, (s_u32DACFragmentSize / 2));

	//Allocate PCM buffer for PCM buffer manage
	s_sPCMBufMgr.pu8Buf = malloc(s_u32DACFragmentSize * 4);	
	if(s_sPCMBufMgr.pu8Buf == NULL){
		i32Ret = -4;
		goto PCMPlayback_Start_Fail;		
	}

	s_sPCMBufMgr.u32Size = s_u32DACFragmentSize * 4;
	s_sPCMBufMgr.u32WriteIdx = 0;
	s_sPCMBufMgr.u32ReadIdx = 0;	
	s_sPCMBufMgr.tBufMutex = xSemaphoreCreateMutex();

	if(s_sPCMBufMgr.tBufMutex == NULL){
		i32Ret = -5;
		goto PCMPlayback_Start_Fail;		
	}
	
	if(StartPlayDAC(i32Volume) < 0){
		i32Ret = -7;
		goto PCMPlayback_Start_Fail;		
	}

	//Create task for PCM puncher task			
	xTaskCreate( PCMPuncherTask, "pcm_puncker", PCM_TASK_STACK_MIDDLE, &s_sPCMBufMgr, PCM_TASK_PRIORITY_MIDDLE,  &s_hPunckerTaskHandle);
	if(s_hPunckerTaskHandle == NULL){
		i32Ret = -6;
		goto PCMPlayback_Start_Fail;		
	}

	*pu32FragmentSize = s_u32DACFragmentSize;
	return 0;
	
PCMPlayback_Start_Fail:

	if(s_hPunckerTaskHandle){
		vTaskDelete(s_hPunckerTaskHandle);
		s_hPunckerTaskHandle = NULL;
	}
		
	if(s_sPCMBufMgr.tBufMutex){
		vSemaphoreDelete(s_sPCMBufMgr.tBufMutex);
		s_sPCMBufMgr.tBufMutex = NULL;
	}

	if(s_sPCMBufMgr.pu8Buf){
		free(s_sPCMBufMgr.pu8Buf);
		s_sPCMBufMgr.pu8Buf = NULL;
	}
	
		
	if(s_pu8ISRPcmBuf){
		free(s_pu8ISRPcmBuf);
		s_pu8ISRPcmBuf = NULL;
	}

	CloseDAC();
	
	if(s_tPCMPuncherSem){
		vSemaphoreDelete(s_tPCMPuncherSem);
		s_tPCMPuncherSem = NULL;
	}
	
	NMLOG_ERROR("Unable start PCM playback %d \n", i32Ret);
	return i32Ret;
}


void
PCMPlayback_Stop(void)
{
	
	CloseDAC();

	if(s_hPunckerTaskHandle){
		vTaskDelete(s_hPunckerTaskHandle);
		s_hPunckerTaskHandle = NULL;
	}
	
	vTaskDelay(50 / portTICK_RATE_MS);	

	
	if(s_sPCMBufMgr.tBufMutex){
		vSemaphoreDelete(s_sPCMBufMgr.tBufMutex);
		s_sPCMBufMgr.tBufMutex = NULL;
	}

	if(s_sPCMBufMgr.pu8Buf){
		free(s_sPCMBufMgr.pu8Buf);
		s_sPCMBufMgr.pu8Buf = NULL;
	}
	
	if(s_pu8ISRPcmBuf){
		free(s_pu8ISRPcmBuf);
		s_pu8ISRPcmBuf = NULL;
	}
	
	if(s_tPCMPuncherSem){
		vSemaphoreDelete(s_tPCMPuncherSem);
		s_tPCMPuncherSem = NULL;
	}
}

void
PCMPlayback_Send(
	uint8_t *pu8DataBuf,
	uint32_t *pu8SendLen
)
{
	uint32_t u32SendLen = *pu8SendLen;
	S_PCM_BUF_MGR *psPCMBufMgr = &s_sPCMBufMgr; 
	int32_t i32HalfFrag = s_u32DACFragmentSize / 2;
	uint32_t u32CopyLen;

	//Copy data to PCM buffer. lock the buffer
	xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);
	
//	INFO("Send len %d, half frame %d  \n", u32SendLen,i32HalfFrag );
	while(u32SendLen >= i32HalfFrag){

		if(psPCMBufMgr->u32ReadIdx){
			//move PCM buffer data to the front end
			memmove(psPCMBufMgr->pu8Buf, psPCMBufMgr->pu8Buf + psPCMBufMgr->u32ReadIdx, psPCMBufMgr->u32WriteIdx -  psPCMBufMgr->u32ReadIdx);
			psPCMBufMgr->u32WriteIdx =(psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx);
			psPCMBufMgr->u32ReadIdx = 0;
		}
		
		u32CopyLen = i32HalfFrag;

		if((psPCMBufMgr->u32WriteIdx + u32CopyLen) <= psPCMBufMgr->u32Size){
#if 1
//			INFO("Copy data to PCM buffer  \n");
			memcpy(psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx, pu8DataBuf, u32CopyLen);
#else
			int i;
			uint8_t *pu8PCMData_0;
			uint8_t *pu8PCMData_1;

			for(i = 0; i < u32CopyLen; i = i+2){
				pu8PCMData_0 = psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx + i;
				pu8PCMData_1 = psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx + i + 1;
				*pu8PCMData_0 = pu8RecvBuf[i + 1];
				*pu8PCMData_1 = pu8RecvBuf[i];
			}
#endif
			psPCMBufMgr->u32WriteIdx += u32CopyLen;
		}
		else{
			NMLOG_INFO("PCM playback buffer full. Discard audio packet \n");
			memcpy(psPCMBufMgr->pu8Buf, pu8DataBuf, u32CopyLen);
			psPCMBufMgr->u32WriteIdx = u32CopyLen;
			psPCMBufMgr->u32ReadIdx = 0;
		}

		memmove(pu8DataBuf, pu8DataBuf + u32CopyLen, u32SendLen -  u32CopyLen);
		u32SendLen -= u32CopyLen;
	}

	//unlock the buffer
	xSemaphoreGive(psPCMBufMgr->tBufMutex);
	*pu8SendLen = u32SendLen;
}

int32_t
PCMPlayback_GetBufFreeSpace(void)
{
	S_PCM_BUF_MGR *psPCMBufMgr = &s_sPCMBufMgr;
	uint32_t u32FreeSpace = 0;

	xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);

	if(psPCMBufMgr->u32ReadIdx){
		//move PCM buffer data to the front end
		memmove(psPCMBufMgr->pu8Buf, psPCMBufMgr->pu8Buf + psPCMBufMgr->u32ReadIdx, psPCMBufMgr->u32WriteIdx -  psPCMBufMgr->u32ReadIdx);
		psPCMBufMgr->u32WriteIdx =(psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx);
		psPCMBufMgr->u32ReadIdx = 0;
	}

	u32FreeSpace =  psPCMBufMgr->u32Size - psPCMBufMgr->u32WriteIdx; 

	xSemaphoreGive(psPCMBufMgr->tBufMutex);
	return u32FreeSpace;
}


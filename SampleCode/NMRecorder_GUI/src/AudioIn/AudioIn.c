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
#include "N9H26_AUR.h"
#include "N9H26_EDMA.h"

#include "AudioIn.h"
#include "AudioInConfig.h"
#include "AurEDMA.h"

#include "NVTMedia.h"

#define __ENABLE_CACHE__

#ifdef __ENABLE_CACHE__
    #define E_NONCACHE_BIT  0x80000000
#else
    #define E_NONCACHE_BIT  0x00000000
#endif

typedef struct
{
    S_AIN_INFO sAinInfo;
    uint32_t u32PCMFragBufSize;

    uint32_t u32PCMEDMABufSize;
    uint8_t *pu8PCMEDMABuf;

    uint8_t *pu8PCMBuf;
    uint32_t u32PCMBufSize;
    uint32_t u32PCMInIdx;
    uint32_t u32PCMOutIdx;

    SemaphoreHandle_t tAinISRSem;
    xSemaphoreHandle tPCMIndexMutex;

    uint32_t u32EDMAChannel;
    uint64_t u64LastFrameTimestamp;

} S_AIN_CTRL;

static portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;
static volatile BOOL s_bIsEDMABufferDone = 0;

static S_AIN_CTRL s_sAinCtrl;

//EDMA callback
void edmaCallback(UINT32 u32WrapStatus)
{
    //UINT32 u32Period, u32Attack, u32Recovery, u32Hold;
    if (u32WrapStatus == 256)
    {
        s_bIsEDMABufferDone = 1;
        //sysprintf("1 %d\n\n", bIsBufferDone);
        xSemaphoreGiveFromISR(s_sAinCtrl.tAinISRSem, &xHigherPrioTaskWoken);
        if (xHigherPrioTaskWoken == pdTRUE)
        {
            //SWITCH CONTEXT;
            portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
        }
    }
    else if (u32WrapStatus == 1024)
    {

        s_bIsEDMABufferDone = 2;
        //sysprintf("2 %d\n\n", bIsBufferDone);
        xSemaphoreGiveFromISR(s_sAinCtrl.tAinISRSem, &xHigherPrioTaskWoken);
        if (xHigherPrioTaskWoken == pdTRUE)
        {
            //SWITCH CONTEXT
            portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
        }
    }
    //sysprintf(" ints = %d,  0x%x\n", u32Count, u32WrapStatus);
}

//Audio record callback
static void AudioRecordSampleDone(void)
{


}

static int
OpenAinDev(
    E_AUR_MIC_SEL eMicType,
    S_AIN_CTRL *psAinCtrl
)
{
    S_AIN_INFO *psAinInfo;

    psAinInfo = &psAinCtrl->sAinInfo;

    PFN_AUR_CALLBACK pfnOldCallback;
    int32_t i32Idx;

    uint32_t au32ArraySampleRate[] =
    {
        eAUR_SPS_48000,
        eAUR_SPS_44100, eAUR_SPS_32000, eAUR_SPS_24000, eAUR_SPS_22050,
        eAUR_SPS_16000, eAUR_SPS_12000, eAUR_SPS_11025, eAUR_SPS_8000,
        eAUR_SPS_96000,
        eAUR_SPS_192000
    };

    uint32_t u32MaxSupport = sizeof(au32ArraySampleRate) / sizeof(au32ArraySampleRate[0]);

    for (i32Idx = 0; i32Idx < u32MaxSupport ; i32Idx = i32Idx + 1)
    {
        if (psAinInfo->u32SampleRate == au32ArraySampleRate[i32Idx])
            break;
        if (i32Idx == u32MaxSupport)
        {
            NMLOG_ERROR("Not in support audio sample rate list\n")  ;
            return -1;
        }
    }

    DrvAUR_Open(eMicType, TRUE);
    DrvAUR_InstallCallback(AudioRecordSampleDone, &pfnOldCallback);

    if (eMicType == eAUR_MONO_MIC_IN)
    {
        DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x22, 0x1E);           /* Pregain */
        DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x23, 0x0E);           /* No any amplifier */
        DrvAUR_DisableInt();
        DrvAUR_SetSampleRate((E_AUR_SPS)au32ArraySampleRate[i32Idx]);
        DrvAUR_AutoGainTiming(1, 1, 1);
        DrvAUR_AutoGainCtrl(TRUE, TRUE, eAUR_OTL_N12P6);
    }
    else if ((eMicType == eAUR_MONO_DIGITAL_MIC_IN) || (eMicType == eAUR_STEREO_DIGITAL_MIC_IN))
    {
        DrvAUR_SetDigiMicGain(TRUE, eAUR_DIGI_MIC_GAIN_P19P2);
        DrvAUR_DisableInt();
        DrvAUR_SetSampleRate((E_AUR_SPS)au32ArraySampleRate[i32Idx]);
    }

    DrvAUR_SetDataOrder(eAUR_ORDER_MONO_16BITS);

    outp32(REG_AR_CON, inp32(REG_AR_CON) | AR_EDMA);
    vTaskDelay(30 / portTICK_RATE_MS);      /* Delay 300ms for OTL stable */
    initEDMA(&psAinCtrl->u32EDMAChannel, (int8_t *)psAinCtrl->pu8PCMEDMABuf, psAinCtrl->u32PCMEDMABufSize, edmaCallback);

    return 0;
}


int32_t AudioInDeviceInit(
    S_AIN_CTRL *psAinCtrl
)
{
    S_AIN_INFO *psAinInfo;
    uint32_t u32FragDuration;

    psAinInfo = &psAinCtrl->sAinInfo;
    psAinInfo->u32SampleRate = AIN_CONFIG_SAMPLERATE;
    psAinInfo->u32Channel = AIN_CONFIG_CHANNEL;
    u32FragDuration = AIN_CONFIG_FRAGMENT_DURATION;

    psAinCtrl->u32PCMFragBufSize = ((psAinInfo->u32SampleRate * u32FragDuration) / 1000 * psAinInfo->u32Channel * 2);
    psAinCtrl->u32PCMEDMABufSize = psAinCtrl->u32PCMFragBufSize * 2;

    psAinCtrl->tAinISRSem = xSemaphoreCreateBinary();

    if (psAinCtrl->tAinISRSem == NULL)
    {
        NMLOG_ERROR("Unable create audio-in interrupt semaphore \n");
        return -2;
    }

    psAinCtrl->tPCMIndexMutex = xSemaphoreCreateMutex();
    if (psAinCtrl->tPCMIndexMutex == NULL)
    {
        NMLOG_ERROR("Unable create audio-in PCM index mutex \n");
        return -2;
    }

    psAinCtrl->pu8PCMEDMABuf = malloc(psAinCtrl->u32PCMEDMABufSize + 4096);
    if (psAinCtrl->pu8PCMEDMABuf == NULL)
    {
        NMLOG_ERROR("Unable allocate audio pdma buffer \n");
        return -3;
    }

    psAinCtrl->u32PCMBufSize = psAinInfo->u32SampleRate * psAinInfo->u32Channel * 2;
    psAinCtrl->pu8PCMBuf = malloc(psAinCtrl->u32PCMBufSize);
    if (psAinCtrl->pu8PCMBuf == NULL)
    {
        NMLOG_ERROR("Unable allocate audio pcm buffer \n");
        return -4;
    }

    psAinCtrl->u32PCMInIdx = 0;
    psAinCtrl->u32PCMOutIdx = 0;

    // Open Ain devce
    OpenAinDev(eAUR_MONO_MIC_IN, psAinCtrl);
    return 0;
}

static void AinFrameLoop(
    S_AIN_CTRL *psAinCtrl
)
{
    uint32_t u32Temp;
    //Combine with EDMA, only mode 1 can be set.
    DrvAUR_StartRecord(eAUR_MODE_1);

    DrvEDMA_CHEnablelTransfer((E_DRVEDMA_CHANNEL_INDEX)psAinCtrl->u32EDMAChannel);

    while (1)
    {
        xSemaphoreTake(psAinCtrl->tAinISRSem, portMAX_DELAY);
        xSemaphoreTake(psAinCtrl->tPCMIndexMutex, portMAX_DELAY);

        psAinCtrl->u64LastFrameTimestamp = NMUtil_GetTimeMilliSec();

//      NMLOG_INFO("Audio In time stamp %d \n", (uint32_t)psAinCtrl->u64LastFrameTimestamp);
        if (psAinCtrl->u32PCMOutIdx)
        {
            u32Temp = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMOutIdx;
            memmove(psAinCtrl->pu8PCMBuf, psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMOutIdx, u32Temp);
            psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMOutIdx;
            psAinCtrl->u32PCMOutIdx = 0;
        }

        if ((psAinCtrl->u32PCMInIdx + psAinCtrl->u32PCMFragBufSize) >= psAinCtrl->u32PCMBufSize)
        {
            //Cut oldest audio data, if PCM buffer data over buffer size
            memmove(psAinCtrl->pu8PCMBuf, psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMFragBufSize, psAinCtrl->u32PCMFragBufSize);
            psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMFragBufSize;
        }

        if (s_bIsEDMABufferDone == 1)
        {
            memcpy(psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMInIdx, (int8_t *)((int32_t)(psAinCtrl->pu8PCMEDMABuf + psAinCtrl->u32PCMFragBufSize) | E_NONCACHE_BIT), psAinCtrl->u32PCMFragBufSize);
        }
        else if (s_bIsEDMABufferDone == 2)
        {
            memcpy(psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMInIdx, (int8_t *)((int32_t)(psAinCtrl->pu8PCMEDMABuf) | E_NONCACHE_BIT), psAinCtrl->u32PCMFragBufSize);
        }

        psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx + psAinCtrl->u32PCMFragBufSize;
        s_bIsEDMABufferDone = 0;

        xSemaphoreGive(psAinCtrl->tPCMIndexMutex);
    }
}

int32_t AudioIn_DeviceInit(void)
{
    memset(&s_sAinCtrl, 0, sizeof(S_AIN_CTRL));

    return AudioInDeviceInit(&s_sAinCtrl);
}


void AudioIn_TaskCreate(
    void *pvParameters
)
{
    AinFrameLoop(&s_sAinCtrl);

    vTaskDelete(NULL);
}

BOOL
AudioIn_ReadFrameData(
    uint32_t u32ReadSamples,
    uint8_t *pu8FrameData,
    uint64_t *pu64FrameTime
)
{
    uint32_t u32SamplesInBuf;
    S_AIN_INFO *psAinInfo;
    uint32_t u32ReadSize;

    psAinInfo = &s_sAinCtrl.sAinInfo;
    u32ReadSize = u32ReadSamples * psAinInfo->u32Channel * 2;

    xSemaphoreTake(s_sAinCtrl.tPCMIndexMutex, portMAX_DELAY);

    u32SamplesInBuf = (s_sAinCtrl.u32PCMInIdx - s_sAinCtrl.u32PCMOutIdx) / psAinInfo->u32Channel / 2;

    if (u32SamplesInBuf < u32ReadSamples)
    {
        xSemaphoreGive(s_sAinCtrl.tPCMIndexMutex);
        return FALSE;
    }

    memcpy(pu8FrameData, s_sAinCtrl.pu8PCMBuf + s_sAinCtrl.u32PCMOutIdx, u32ReadSize);
    s_sAinCtrl.u32PCMOutIdx += u32ReadSize;
//  printf("s_sAinCtrl.u64LastFrameTimestamp %d \n", (uint32_t)s_sAinCtrl.u64LastFrameTimestamp);
    *pu64FrameTime = s_sAinCtrl.u64LastFrameTimestamp - ((1000 * u32SamplesInBuf) / psAinInfo->u32SampleRate);

    xSemaphoreGive(s_sAinCtrl.tPCMIndexMutex);
    return TRUE;
}

S_AIN_INFO *
AudioIn_GetInfo(void)
{
    return &s_sAinCtrl.sAinInfo;
}

void
AudioIn_CleanPCMBuff(void)
{
    xSemaphoreTake(s_sAinCtrl.tPCMIndexMutex, portMAX_DELAY);

    s_sAinCtrl.u32PCMInIdx = 0;
    s_sAinCtrl.u32PCMOutIdx = 0;

    xSemaphoreGive(s_sAinCtrl.tPCMIndexMutex);
}





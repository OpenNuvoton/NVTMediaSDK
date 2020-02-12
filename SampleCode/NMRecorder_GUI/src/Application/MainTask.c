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

void *worker_imggrab(void *pvParameters)
{
    int32_t i32Ret;

    i32Ret = VideoIn_DeviceInit();
    if (i32Ret != 0)
    {
        NMLOG_ERROR("Unable init video in device %d\n", i32Ret);
        goto exit_worker_imggrab;
    }

    i32Ret = AudioIn_DeviceInit();
    if (i32Ret != 0)
    {
        NMLOG_ERROR("Unable init audio in device %d\n", i32Ret);
        goto exit_worker_imggrab;
    }

    xTaskCreate(VideoIn_TaskCreate, "VideoIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL);
    xTaskCreate(AudioIn_TaskCreate, "AudioIn", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL);

    Render_Init();

    while (1)
    {
        uint8_t *pu8FrameData;
        uint64_t u64FrameTime;

        if (VideoIn_ReadNextPacketFrame(0, &pu8FrameData, &u64FrameTime) == TRUE)
        {
            Render_SetFrameBuffAddr(pu8FrameData);
        }

        vTaskDelay(5);
    }

    Render_Final();
exit_worker_imggrab:

    NMLOG_ERROR("Die... %s\n", __func__);

    return NULL;
}

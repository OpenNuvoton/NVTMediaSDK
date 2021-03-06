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

/* Scheduler includes. */
/* FreeRTOS+POSIX. */
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/mqueue.h"
#include "FreeRTOS_POSIX/time.h"
#include "FreeRTOS_POSIX/errno.h"
#include "FreeRTOS_POSIX/unistd.h"

/* Nuvoton includes. */
#include "wblib.h"
#include "N9H26.h"

#include "GUI.h"
#include "LCDConf.h"
#include "WM.h"
#include "TEXT.h"
#include "DIALOG.h"

#include "recorder.h"

static WM_HWIN hWin_Recording = NULL;
WM_HWIN CreateRecord(void);
void update_gui_widgets(WM_HWIN, E_NM_RECORD_STATUS);

static void cbBackgroundWin(WM_MESSAGE *pMsg)
{
    printf("[%s] msgid=%d, %d->%d \n", __func__, pMsg->MsgId, pMsg->hWinSrc, pMsg->hWin);
    switch (pMsg->MsgId)
    {
    case WM_TOUCH:
        if (hWin_Recording)
            WM_ShowWin(hWin_Recording);
        break;
    case WM_PAINT:
        GUI_SetBkColor(DEF_OSD_COLORKEY);
        GUI_Clear();
        break;
    }
    WM_DefaultProc(pMsg);
}

void *worker_imggrab(void *pvParameters);
void *worker_guiman(void *pvArgs)
{
    pthread_t pxMain_worker;
    pthread_attr_t attr;

    GUI_Init();

    // Hijacking callback of Desktop.
    WM_SetCallback(WM_HBKWIN, cbBackgroundWin);

    // Fill color key.
    GUI_SetBkColor(DEF_OSD_COLORKEY);
    GUI_Clear();

    printf("BKhandle:%d\n", WM_HBKWIN);

    hWin_Recording = CreateRecord();
    if (!hWin_Recording) goto exit_worker_guiman;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32 * 1024);
    pthread_create(&pxMain_worker, &attr, worker_imggrab, NULL);

    while (1)
    {
        E_NM_RECORD_STATUS eRecorderStatusNow =  recorder_status();

        // Update widget & time label
        update_gui_widgets(hWin_Recording, eRecorderStatusNow);

        if ((eRecorderStatusNow == eNM_RECORD_STATUS_RECORDING) && recorder_is_over_limitedsize())
        {
            printf("No enough space to record. Stop recording.\n");
            recorder_stop();
        }
        GUI_Delay(100);
    }

exit_worker_guiman:

    return NULL;
}

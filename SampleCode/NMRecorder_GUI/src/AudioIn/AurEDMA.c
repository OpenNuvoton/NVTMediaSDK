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
#include <string.h>
#include <inttypes.h>

#include "wblib.h"
#include "N9H26_EDMA.h"


/*
static void pfnRecordCallback(void)
{
    g_i8PcmReady = TRUE;
}
*/


#define CLIENT_ADC_NAME
int edma_channel = 0;
int initEDMA(
    uint32_t *u32EDMAChanel,
    int8_t  *pi8PCMBufAddr,
    uint32_t u32PCMBufSize,
    PFN_DRVEDMA_CALLBACK pfnEDMACallback
)
{
    int i;
    EDMA_Init();
#if 1
    i = PDMA_FindandRequest(CLIENT_ADC_NAME); //w55fa95_edma_request
#else
    for (i = 4; i >= 1; i--)
    {
        if (!EDMA_Request(i, CLIENT_ADC_NAME))
            break;
    }
#endif

//  if(i == -ENODEV)
//      return -ENODEV;

    edma_channel = i;
    EDMA_SetAPB(edma_channel,           //int channel,
                eDRVEDMA_ADC,           //E_DRVEDMA_APB_DEVICE eDevice,
                eDRVEDMA_READ_APB,      //E_DRVEDMA_APB_RW eRWAPB,
                eDRVEDMA_WIDTH_32BITS); //E_DRVEDMA_TRANSFER_WIDTH eTransferWidth

    EDMA_SetupHandlers(edma_channel,        //int channel
                       eDRVEDMA_WAR,           //int interrupt,
                       pfnEDMACallback,                //void (*irq_handler) (void *),
                       NULL);                  //void *data

    EDMA_SetWrapINTType(edma_channel,
                        eDRVEDMA_WRAPAROUND_EMPTY |
                        eDRVEDMA_WRAPAROUND_HALF);  //int channel, WR int type

    EDMA_SetDirection(edma_channel, eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_WRAPAROUND);


    EDMA_SetupSingle(edma_channel,      // int channel,
                     0xB800E010,//0xB800E010,        // unsigned int src_addr,  (ADC data port physical address)
                     (UINT32)pi8PCMBufAddr, //phaddrrecord,      // unsigned int dest_addr,
                     u32PCMBufSize); // unsigned int dma_length /* Lenth equal 2 half buffer */

    *u32EDMAChanel = i;
    return Successful;
}
void releaseEDMA(UINT32 u32EdmaChannel)
{
    if (edma_channel != 0)
    {
        EDMA_Free(u32EdmaChannel);
        edma_channel = 0;
    }
}

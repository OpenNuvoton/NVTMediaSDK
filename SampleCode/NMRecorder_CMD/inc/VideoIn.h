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
#ifndef __VIDEO_IN_H__
#define __VIDEO_IN_H__

#define DEF_VIN_MAX_PIPES           4

typedef enum
{
    eVIN_COLOR_NONE,
    eVIN_COLOR_YUV422,
    eVIN_COLOR_YUV422P,
    eVIN_COLOR_YUV420P_MB,
    eVIN_COLOR_YUV420P,
} E_VIN_COLOR_TYPE;

typedef struct
{
    uint32_t u32Width;
    uint32_t u32Height;
    E_VIN_COLOR_TYPE eColorType;
    uint32_t u32FramePhyAddr;
    uint32_t u32PipeNo;
    uint32_t u32FrameBufSize;
    uint32_t u32FrameRate;
} S_VIN_PIPE_INFO;


typedef struct
{
    S_VIN_PIPE_INFO sPipeInfo;
} S_VIN_FRAME_DATA;

typedef struct
{
    S_VIN_PIPE_INFO asVinPipeInfo[DEF_VIN_MAX_PIPES];
    uint32_t u32NumPipes;
} S_VIN_CONFIG;

int32_t
VideoIn_DeviceInit(void);

void VideoIn_TaskCreate(
    void *pvParameters
);

BOOL
VideoIn_ReadNextPlanarFrame(
    uint32_t u32PortNo,
    uint8_t **ppu8FrameData,
    uint64_t *pu64FrameTime
);

BOOL
VideoIn_ReadNextPacketFrame(
    uint32_t u32PortNo,
    uint8_t **ppu8FrameData,
    uint64_t *pu64FrameTime
);

S_VIN_PIPE_INFO *
VideoIn_GetPipeInfo(
    int32_t i32PipeNo
);


#endif

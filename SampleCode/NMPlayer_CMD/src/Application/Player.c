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
#include "Render.h"

#include "FileList.h"
#include "Player.h"

#define DEF_NM_ENGINE_BLOCKING true

typedef struct
{
    HPLAY                       m_hPlay;
    uint64_t                    u64CurTimeInMs;
    int                             m_i32FileListIdx;
    S_NM_PLAY_IF            m_sPlayIF;
    S_NM_PLAY_CONTEXT m_sPlayCtx;
    S_NM_PLAY_INFO      m_sPlayInfo;
    void                      *m_OpenRes;
} S_PLAYER;

static S_PLAYER g_sPlayer =
{
    .m_hPlay = (HPLAY)eNM_INVALID_HANDLE,
    0
};

void Render_VideoFlush(S_NM_VIDEO_CTX   *psVideoCtx);
void Render_AudioFlush(S_NM_AUDIO_CTX   *psAudioCtx);

static uint64_t s_u64ShowTime  = 0;

#if defined (__GNUC__) && !(__CC_ARM)
static char* ulltoStr(uint64_t val)
{
    static char buf[34] = { [0 ... 33] = 0 };
    char* out = &buf[32];
    uint64_t hval = val;
    unsigned int hbase = 10;

    do {
        *out = "0123456789abcdef"[hval % hbase];
        --out;
        hval /= hbase;
    } while(hval);

    out ++;
    return out;
}
#endif


void ShowPlayingTime(void)
{
	char szInfo[100];

	if(NMUtil_GetTimeMilliSec() > s_u64ShowTime){

#if defined (__GNUC__) && !(__CC_ARM)
		sprintf(szInfo,"Current playing time %s \n", ulltoStr(g_sPlayer.u64CurTimeInMs) );
#else
		sprintf(szInfo,"Current playing time %"PRId64" \n", g_sPlayer.u64CurTimeInMs );
#endif
		printf("%s", szInfo);

		s_u64ShowTime = NMUtil_GetTimeMilliSec() + 1000;
	}

}


void UpdateInfo_VideoFlush(S_NM_VIDEO_CTX   *psVideoCtx)
{
    g_sPlayer.u64CurTimeInMs = psVideoCtx->u64DataTime;
	ShowPlayingTime();
    Render_VideoFlush(psVideoCtx);
}

void UpdateInfo_AudioFlush(S_NM_AUDIO_CTX   *psAudioCtx)
{
    g_sPlayer.u64CurTimeInMs = psAudioCtx->u64DataTime;
	ShowPlayingTime();
    Render_AudioFlush(psAudioCtx);
}

uint64_t Player_playtime_current(void)
{
    return g_sPlayer.u64CurTimeInMs;
}

uint64_t Player_playtime_total(void)
{
    return (uint64_t)g_sPlayer.m_sPlayInfo.u32Duration;
}

static void dump_media_info(void)
{
    const char *szAVIFileName = filelist_getFileName(g_sPlayer.m_i32FileListIdx);
    printf(" Fn:%s AT:%d, MF: %d, VT:%d, Duration:%d, AC:%d, VC:%d\n", szAVIFileName,
           g_sPlayer.m_sPlayInfo.eAudioType,
           g_sPlayer.m_sPlayInfo.eMediaFormat,
           g_sPlayer.m_sPlayInfo.eVideoType,
           g_sPlayer.m_sPlayInfo.u32Duration,
           g_sPlayer.m_sPlayInfo.u32AudioChunks,
           g_sPlayer.m_sPlayInfo.u32VideoChunks);

}

int Player_start(
	char *szFileName
)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	int32_t i32Ret;
	
    if (g_sPlayer.m_hPlay == (HPLAY)eNM_INVALID_HANDLE)
    {
        memset((void *)&g_sPlayer.m_sPlayIF, 0, sizeof(S_NM_PLAY_IF));
        memset((void *)&g_sPlayer.m_sPlayCtx, 0, sizeof(S_NM_PLAY_CONTEXT));
        memset((void *)&g_sPlayer.m_sPlayInfo, 0, sizeof(S_NM_PLAY_INFO));
        g_sPlayer.m_OpenRes = NULL;
        g_sPlayer.m_hPlay = (HPLAY)eNM_INVALID_HANDLE;

		//Open media
        eNMRet = NMPlay_Open((char *)szFileName, &g_sPlayer.m_sPlayIF, &g_sPlayer.m_sPlayCtx, &g_sPlayer.m_sPlayInfo, &g_sPlayer.m_OpenRes);
        if (eNMRet != eNM_ERRNO_NONE)
        {
            printf("[%s] %x\n", __func__, eNMRet);
            goto exit_player_start;
        }

		//Show media information
        dump_media_info();

		//Setup audio and video flush callback
        if (g_sPlayer.m_sPlayIF.psVideoCodecIF)
            g_sPlayer.m_sPlayIF.pfnVideoFlush = UpdateInfo_VideoFlush;

        if (g_sPlayer.m_sPlayIF.psAudioCodecIF && !g_sPlayer.m_sPlayIF.pfnVideoFlush)
            g_sPlayer.m_sPlayIF.pfnAudioFlush = UpdateInfo_AudioFlush;
        else if (g_sPlayer.m_sPlayIF.psAudioCodecIF)
            g_sPlayer.m_sPlayIF.pfnAudioFlush = Render_AudioFlush;

		//Setup video flush context
        g_sPlayer.m_sPlayCtx.sFlushVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV422;
        g_sPlayer.m_sPlayCtx.sFlushVideoCtx.u32Width = LCD_PANEL_WIDTH;
        g_sPlayer.m_sPlayCtx.sFlushVideoCtx.u32Height = LCD_PANEL_HEIGHT;

		//Setup audio flush context
        g_sPlayer.m_sPlayCtx.sFlushAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
        g_sPlayer.m_sPlayCtx.sFlushAudioCtx.u32SampleRate = g_sPlayer.m_sPlayCtx.sMediaAudioCtx.u32SampleRate;
        g_sPlayer.m_sPlayCtx.sFlushAudioCtx.u32Channel = g_sPlayer.m_sPlayCtx.sMediaAudioCtx.u32Channel;
        g_sPlayer.m_sPlayCtx.sFlushAudioCtx.u32SamplePerBlock = g_sPlayer.m_sPlayCtx.sMediaAudioCtx.u32SamplePerBlock;
        g_sPlayer.m_sPlayCtx.sFlushAudioCtx.pvParamSet = g_sPlayer.m_sPlayCtx.sMediaAudioCtx.pvParamSet;

		//Init display
        i32Ret = Render_Init(g_sPlayer.m_sPlayCtx.sFlushAudioCtx.u32SampleRate, g_sPlayer.m_sPlayCtx.sFlushAudioCtx.u32Channel);
        if (i32Ret != 0)
        {
			eNMRet = eNM_ERRNO_IO;
            Player_stop();
            goto exit_player_start;
        }

    }

exit_player_start:
    return -(eNMRet);
}

int Player_stop(void)
{
    if (g_sPlayer.m_hPlay != (HPLAY)eNM_INVALID_HANDLE)
    {
		//Close media
        NMPlay_Close(g_sPlayer.m_hPlay, &g_sPlayer.m_OpenRes);
        Render_Final();
    }
	
    g_sPlayer.m_hPlay = (HPLAY)eNM_INVALID_HANDLE;

	return 0;
}

E_NM_PLAY_STATUS Player_status(void)
{
    // get status
    return NMPlay_Status(g_sPlayer.m_hPlay);
}

E_NM_ERRNO Player_pause(void)
{
    return NMPlay_Pause(g_sPlayer.m_hPlay, DEF_NM_ENGINE_BLOCKING);
}

int Player_play(void)
{
    // play media
    E_NM_ERRNO eNMErr = NMPlay_Play(&g_sPlayer.m_hPlay, &g_sPlayer.m_sPlayIF, &g_sPlayer.m_sPlayCtx, DEF_NM_ENGINE_BLOCKING);
    if (eNMErr != eNM_ERRNO_NONE)
    {
        sysprintf("Unable play media %x\n", eNMErr);
        goto exit_player_play;
    }
    return eNMErr;

exit_player_play:
    Player_stop();
    return eNMErr;
}

int Player_seek(uint32_t u32SeekTime)
{
    // seek
    E_NM_ERRNO eNMErr;
    printf("Seek to %dms\n", u32SeekTime);

    eNMErr = NMPlay_Seek(g_sPlayer.m_hPlay,
                         u32SeekTime,
                         g_sPlayer.m_sPlayInfo.u32VideoChunks,
                         g_sPlayer.m_sPlayInfo.u32AudioChunks,
                         DEF_NM_ENGINE_BLOCKING);
    if (eNMErr != eNM_ERRNO_NONE)
    {
        printf("Unable seek media %x\n", eNMErr);
        goto exit_player_seek;
    }
    return eNMErr;

exit_player_seek:

    return eNMErr;
}

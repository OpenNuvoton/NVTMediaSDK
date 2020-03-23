/**************************************************************************//**
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
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

/* Nuvoton includes. */
#include "wblib.h"
#include "N9H26.h"

#include "FileList.h"
#include "Player.h"

struct s_page_handle{
	struct s_page_handle *psPrevPage;
	int (*pfnPageInit)(void *pvArgv);
	void (*pfnPageInfo)(void);
	struct s_page_handle* (*pfnPageAction)(char key);
	void *pvPriv;
};

typedef struct s_page_handle S_PAGE_HANDLE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Play page operations
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
	int32_t i32Temp;
}S_PLAY_PAGE_PRIV;

static int PlayPage_Init(void *pvArgv);
static void PlayPage_Info(void);
static struct s_page_handle* PlayPage_Action(char key);

static S_PAGE_HANDLE s_PlayPage = {
	.psPrevPage = NULL,
	.pfnPageInit = PlayPage_Init,
	.pfnPageInfo = PlayPage_Info,
	.pfnPageAction = PlayPage_Action,
	.pvPriv = NULL,
};

static S_PLAY_PAGE_PRIV s_PlayPagePriv;

static int PlayPage_Init(void *pvArgv)
{
	char *szFileName = (char *)pvArgv;
	int32_t i32Ret;
	
	i32Ret = Player_start(szFileName);
	if(i32Ret != 0){
		printf("Unable play file %s, %d \n", szFileName, i32Ret);
		return i32Ret;
	}
	
	i32Ret = Player_play();	
	if(i32Ret != 0){
		return i32Ret;
	}
	
	s_PlayPage.pvPriv = &s_PlayPagePriv;

	return 0;
}

static int32_t ReadSeekTime(void)
{
	char szSeekTime[10];
	int32_t i32InBuf = 0;
	char chInput;
	
	memset(szSeekTime, 0, 10);
	szSeekTime[i32InBuf] = '0';
	
	while(1){
		chInput = sysGetChar();
		if((chInput < '0')|| (chInput > '9'))
			break;
		printf("%c", chInput);
		szSeekTime[i32InBuf] = chInput;
		i32InBuf ++;
		
		if(i32InBuf >= 9)
			break;
	}
	
	return atoi(szSeekTime);
}

static void PlayPage_Info(void){

//	S_PLAY_PAGE_PRIV *psPriv = (S_PLAY_PAGE_PRIV *)s_PlayPage.pvPriv;
	printf("+++++++++++++++++++++++++++++++++++++++++\n");
	printf("+               NMPlayer                +\n");
	printf("+   s: seek                             +\n");
	printf("+   p: pause/resume                     +\n");
	printf("+   q: stop play and back               +\n");
	printf("+++++++++++++++++++++++++++++++++++++++++\n");
}

static struct s_page_handle* PlayPage_Action(
	char chKey
)
{
//	S_PLAY_PAGE_PRIV *psPriv = (S_PLAY_PAGE_PRIV *)s_PlayPage.pvPriv;
	
	switch(chKey)
	{
		case 's':
		{
			int32_t i32SeekTime;
			Player_pause();
			printf("Please key-in seek time (ms):");
			i32SeekTime = ReadSeekTime();
			printf("\n");			
			Player_seek(i32SeekTime);
			Player_play();
		}
		break;
		case 'p':
		{
			E_NM_PLAY_STATUS ePlayStatus;

			ePlayStatus = Player_status();

			if(ePlayStatus == eNM_PLAY_STATUS_PLAYING){
				//pause play
				Player_pause();
			}
			else if(ePlayStatus == eNM_PLAY_STATUS_PAUSE){
				//resume play
				Player_play();
			}
		}
		break;
		case 'q':
		{
			S_PAGE_HANDLE *psPrevPage;
			Player_stop();
			psPrevPage = s_PlayPage.psPrevPage;
			s_PlayPage.psPrevPage =  NULL;
			return psPrevPage;
		}
	}
	return &s_PlayPage;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Main page operations
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
	int32_t i32MediaFiles;
	int32_t i32CurMediaIdx;
	
}S_MAIN_PAGE_PRIV;


static void MainPage_Info(void);
static struct s_page_handle* MainPage_Action(char key);
static int MainPage_Init(void *pvArgv);

static S_PAGE_HANDLE s_MainPage = {
	.psPrevPage = NULL,
	.pfnPageInit = MainPage_Init,
	.pfnPageInfo = MainPage_Info,
	.pfnPageAction = MainPage_Action,
	.pvPriv = NULL,
};

static S_MAIN_PAGE_PRIV s_MainPagePriv;

static void MainPage_Info(void){

	S_MAIN_PAGE_PRIV *psPriv = (S_MAIN_PAGE_PRIV *)s_MainPage.pvPriv;

	printf("+++++++++++++++++++++++++++++++++++++++++\n");
	printf("+               NMPlayer                +\n");
	printf("+   p: start play media file            +\n");
	printf("+   n: select next media file           +\n");
	printf("+   q: quit program                     +\n");
	printf("+++++++++++++++++++++++++++++++++++++++++\n");

	if(psPriv->i32MediaFiles > 0)
		printf("Select media file %s \n", filelist_getFileName(psPriv->i32CurMediaIdx));
	else
		printf("No media file on %s \n", DEF_PATH_MEDIA_FOLDER);
}

static int MainPage_Init(void *pvArgv)
{
	S_FILELIST *psMediaFileList;
	
	filelist_create(DEF_PATH_MEDIA_FOLDER);
	psMediaFileList = filelist_getInstance();
	
	s_MainPagePriv.i32MediaFiles = psMediaFileList->m_i32UsedNum;
	s_MainPagePriv.i32CurMediaIdx = 0;

	s_MainPage.pvPriv = &s_MainPagePriv;

	return 0;
}

static struct s_page_handle* MainPage_Action(
	char chKey
)
{
	S_MAIN_PAGE_PRIV *psPriv = (S_MAIN_PAGE_PRIV *)s_MainPage.pvPriv;
	
	switch(chKey)
	{
		case 'p':
		{
			int32_t i32Ret;
			
			i32Ret = PlayPage_Init((void *)filelist_getFileName(psPriv->i32CurMediaIdx));
			if(i32Ret == 0){
				s_PlayPage.psPrevPage = &s_MainPage;
				return &s_PlayPage;
			}
		}
		break;
		case 'n':
		{
			if(psPriv->i32CurMediaIdx >= (psPriv->i32MediaFiles - 1))
				psPriv->i32CurMediaIdx = 0;
			else
				psPriv->i32CurMediaIdx ++;
				
		}
		break;
		case 'q':
		{
			filelist_destroy();
			return NULL;
		}
	}
	return &s_MainPage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *worker_cmdline(void *pvArgs)
{
	S_PAGE_HANDLE *psCurPage = &s_MainPage;
	int8_t i8Key;
	
	psCurPage->pfnPageInit(NULL);
	
	//Read key from stdin and do action
	while(psCurPage){
		psCurPage->pfnPageInfo();
		i8Key = sysGetChar();		//FIXME: sysGetChar() is wait busy function, maybe write another getchar() for thread yelid 
		psCurPage = psCurPage->pfnPageAction(i8Key);
	}

	return NULL;
}

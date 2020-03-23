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

#include "Recorder.h"
#include "VideoIn.h"

struct s_page_handle{
	struct s_page_handle *psPrevPage;
	int (*pfnPageInit)(void *pvArgv);
	void (*pfnPageInfo)(void);
	struct s_page_handle* (*pfnPageAction)(char key);
	void *pvPriv;
};

typedef struct s_page_handle S_PAGE_HANDLE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Record page operations
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
	int32_t i32Temp;
}S_RECORD_PAGE_PRIV;

static int RecordPage_Init(void *pvArgv);
static void RecordPage_Info(void);
static struct s_page_handle* RecordPage_Action(char key);

static S_PAGE_HANDLE s_RecordPage = {
	.psPrevPage = NULL,
	.pfnPageInit = RecordPage_Init,
	.pfnPageInfo = RecordPage_Info,
	.pfnPageAction = RecordPage_Action,
	.pvPriv = NULL,
};

static S_RECORD_PAGE_PRIV s_RecordPagePriv;

static int RecordPage_Init(void *pvArgv)
{
	return 	Recorder_start((uint32_t)pvArgv);
}

static void RecordPage_Info(void)
{
	printf("+++++++++++++++++++++++++++++++++++++++++\n");
	printf("+               NMRecord                +\n");
	printf("+   q: stop record and back             +\n");
	printf("+++++++++++++++++++++++++++++++++++++++++\n");
}

static struct s_page_handle* RecordPage_Action(
	char chKey
)
{
	switch(chKey)
	{
		case 'q':
		{
			S_PAGE_HANDLE *psPrevPage;
			Recorder_stop();
			psPrevPage = s_RecordPage.psPrevPage;
			s_RecordPage.psPrevPage =  NULL;
			return psPrevPage;
		}
	}

	return &s_RecordPage;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Main page operations
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
	uint32_t u32DurationTime;
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
	printf("+               NMRecord                +\n");
	printf("+   t: media file duration time(option) +\n");
	printf("+   r: start record media file          +\n");
	printf("+   q: quit program                     +\n");
	printf("+++++++++++++++++++++++++++++++++++++++++\n");

	if(psPriv->u32DurationTime == eNM_UNLIMIT_TIME)
		printf("Record file duration time is unlimit \n");
	else
		printf("Record file duration time is %d ms\n", psPriv->u32DurationTime);
}

extern void *worker_recorder(void *pvParameters);

static int MainPage_Init(void *pvArgv)
{
    pthread_t pxMain_worker;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32 * 1024);
    pthread_create(&pxMain_worker, &attr, worker_recorder, NULL);

	s_MainPagePriv.u32DurationTime = eNM_UNLIMIT_TIME;

	s_MainPage.pvPriv = &s_MainPagePriv;

	while(1){ //Wait video In ready
		uint8_t *pu8FrameData;
		uint64_t u64FrameTime;

		if(VideoIn_ReadNextPlanarFrame( 0, &pu8FrameData, &u64FrameTime) == TRUE)
			break;
	}
	return 0;
}


static int32_t ReadTime(void)
{
	char szTime[10];
	int32_t i32InBuf = 0;
	char chInput;
	
	memset(szTime, 0, 10);
	szTime[i32InBuf] = '0';
	
	while(1){
		chInput = sysGetChar();
		if((chInput < '0')|| (chInput > '9'))
			break;
		printf("%c", chInput);
		szTime[i32InBuf] = chInput;
		i32InBuf ++;
		
		if(i32InBuf >= 9)
			break;
	}
	
	return atoi(szTime);
}


static struct s_page_handle* MainPage_Action(
	char chKey
)
{
	S_MAIN_PAGE_PRIV *psPriv = (S_MAIN_PAGE_PRIV *)s_MainPage.pvPriv;
	
	switch(chKey)
	{
		case 'r':
		{
			int32_t i32Ret;
			
			i32Ret = RecordPage_Init((void *)psPriv->u32DurationTime);
			if(i32Ret == 0){
				s_RecordPage.psPrevPage = &s_MainPage;
				return &s_RecordPage;
			}
		}
		break;
		case 't':
		{
			printf("Please key-in duration time (ms):");
			psPriv->u32DurationTime = ReadTime();
			printf("\n");			
		}
		break;
		case 'q':
		{
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

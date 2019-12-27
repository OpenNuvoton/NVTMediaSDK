/**
 * @brief Job distribution with actor model.
 *
 * See the top of this file for detailed description.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "wblib.h"
#include "N9H26_NVTFAT.h"
#include "N9H26_SIC.h"
#include "N9H26_SDIO.h"

/* FreeRTOS includes. */
#include "FreeRTOS_POSIX.h"

#include "NuMedia_SAL_OS.h"
#include "NuMedia_SAL_FS.h"

#define ENABLE_SD_ONE_PART
#define ENABLE_SD_0
//#define ENABLE_SD_1
//#define ENABLE_SD_2
//#define ENABLE_SDIO_1

#define DEF_DISK_VOLUME_STR_SIZE	20
static char s_szDiskVolume[DEF_DISK_VOLUME_STR_SIZE] = {0x00};
static BOOL s_bFileSystemInitDone = FALSE;

int32_t
Utils_InitFileSystem(
	char **pszDiskVolume
)
{
	uint32_t u32ExtFreq;
	uint32_t u32PllOutHz;
	uint32_t u32BlockSize, u32FreeSize, u32DiskSize;

	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	memset(s_szDiskVolume, 0x00, DEF_DISK_VOLUME_STR_SIZE);
	*pszDiskVolume = NULL;
	
	//Init file system library 
	fsInitFileSystem();

#if defined(ENABLE_SD_ONE_PART)
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	
#if defined(ENABLE_SDIO_1)
	sdioOpen();	
	if (sdioSdOpen1()<=0)			//cause crash on Doorbell board SDIO slot. conflict with sensor pin
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}	
#elif defined(ENABLE_SD_1)
	sicOpen();
	if (sicSdOpen1()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}
#elif defined(ENABLE_SD_2)
	sicOpen();
	if (sicSdOpen2()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}
#elif defined(ENABLE_SD_0)
	sicOpen();
	if (sicSdOpen0()<=0)
	{
		printf("Error in initializing SD card !! \n");						
		s_bFileSystemInitDone = TRUE;
		return -1;
	}
#endif
#endif			

	sprintf(s_szDiskVolume, "C");

#if defined(ENABLE_SD_ONE_PART)
	fsAssignDriveNumber(s_szDiskVolume[0], DISK_TYPE_SD_MMC, 0, 1);
#endif

	if(fsDiskFreeSpace(s_szDiskVolume[0], (UINT32 *)&u32BlockSize, (UINT32 *)&u32FreeSize, (UINT32 *)&u32DiskSize) != FS_OK){
		s_bFileSystemInitDone = TRUE;
		return -2;
	}

	printf("SD card block_size = %d\n", u32BlockSize);   
	printf("SD card free_size = %dkB\n", u32FreeSize);   
	printf("SD card disk_size = %dkB\n", u32DiskSize);   

	*pszDiskVolume = s_szDiskVolume;
	s_bFileSystemInitDone = TRUE;
	return 0;
}

#define MAX_BUF_LEN 100
#define __UNISTD_FILE__

static uint8_t s_au8DataBuf[MAX_BUF_LEN];

void vFileSystemTest( void *pvParameters )
{
#if defined (__UNISTD_FILE__)
	int pTestFile;
#else
	FILE * pTestFile = NULL;
#endif
	char *szDiskVolume = NULL;
	char *szCreateFile = "C:\\SAL_FS.txt";
	int i;
	int iRet;
	fpos_t sFilePos;
	uint32_t u32FP;
	
	Utils_InitFileSystem(&szDiskVolume);

#if defined (__UNISTD_FILE__)	
	pTestFile = open(szCreateFile, O_RDWR | O_CREATE);
	
	if(pTestFile == NULL){
		printf("Unable create new file \n");
		goto vFileSystemTest_done;
	}

	printf("File no is %x \n", pTestFile);
	for(i = 0 ; i < MAX_BUF_LEN; i++){
		s_au8DataBuf[i] = i;
	}

	iRet = write(pTestFile, s_au8DataBuf, MAX_BUF_LEN);
	printf("Write %d bytes \n", iRet);

	iRet = lseek(pTestFile, 50, SEEK_SET);
	printf("File pos %d\n", iRet);
	
	iRet = write(pTestFile, s_au8DataBuf, MAX_BUF_LEN);
	printf("Write %d bytes \n", iRet);

	iRet = lseek(pTestFile, 0, SEEK_CUR);
	printf("File pos %d\n", iRet);
	
	close(pTestFile);

#else
	pTestFile = fopen(szCreateFile, "w+");

	printf("File no is %x \n", fileno(pTestFile));
	for(i = 0 ; i < MAX_BUF_LEN; i++){
		s_au8DataBuf[i] = i;
	}

	iRet = fwrite(s_au8DataBuf, 1, MAX_BUF_LEN, pTestFile);
	printf("Write %d bytes \n", iRet);

	iRet = fseek(pTestFile, 50, SEEK_SET);

	fgetpos(pTestFile, &sFilePos);

#if defined (__ARMCC_VERSION)
	u32FP = (int32_t) sFilePos.__pos;
	printf("File pos %d\n", u32FP);
#endif
	
	iRet = fwrite(s_au8DataBuf, 1, MAX_BUF_LEN, pTestFile);
	printf("Write %d bytes \n", iRet);

	
	fclose(pTestFile);

#endif

vFileSystemTest_done:
	
	vTaskDelete(NULL);
}



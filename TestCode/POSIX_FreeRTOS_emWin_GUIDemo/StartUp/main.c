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

#include "N9H26TouchPanel.h"

extern int ts_writefile(int hFile);
extern int ts_readfile(int hFile);
extern int ts_calibrate(int xsize, int ysize);
extern void TouchTask(void);

#ifndef STORAGE_SD
#define NAND_2      1   // comment to use 1 disk foor NAND, uncomment to use 2 disk
UINT32 StorageForNAND(void);

static NDISK_T ptNDisk;
static NDRV_T _nandDiskDriver0 =
{
    nandInit0,
    nandpread0,
    nandpwrite0,
    nand_is_page_dirty0,
    nand_is_valid_block0,
    nand_ioctl,
    nand_block_erase0,
    nand_chip_erase0,
    0
};

#define NAND1_1_SIZE     32 /* MB unit */

UINT32 StorageForNAND(void)
{

    UINT32 block_size, free_size, disk_size;
    UINT32 u32TotalSize;

    fsAssignDriveNumber('C', DISK_TYPE_SMART_MEDIA, 0, 1);
#ifdef NAND_2
    fsAssignDriveNumber('D', DISK_TYPE_SMART_MEDIA, 0, 2);
#endif

    sicOpen();

    /* Initialize GNAND */
    if(GNAND_InitNAND(&_nandDiskDriver0, &ptNDisk, TRUE) < 0)
    {
        printf("GNAND_InitNAND error\n");
        goto halt;
    }

    if(GNAND_MountNandDisk(&ptNDisk) < 0)
    {
        printf("GNAND_MountNandDisk error\n");
        goto halt;
    }

    /* Get NAND disk information*/
    u32TotalSize = ptNDisk.nZone* ptNDisk.nLBPerZone*ptNDisk.nPagePerBlock*ptNDisk.nPageSize;
    printf("Total Disk Size %d\n", u32TotalSize);
    /* Format NAND if necessery */
#ifdef NAND_2
    if ((fsDiskFreeSpace('C', &block_size, &free_size, &disk_size) < 0) ||
            (fsDiskFreeSpace('D', &block_size, &free_size, &disk_size) < 0))
    {
        printf("unknow disk type, format device .....\n");
        if (fsTwoPartAndFormatAll((PDISK_T *)ptNDisk.pDisk, NAND1_1_SIZE*1024, (u32TotalSize- NAND1_1_SIZE*1024)) < 0)
        {
            printf("Format failed\n");
            goto halt;
        }
        fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
        fsSetVolumeLabel('D', "NAND1-2\n", strlen("NAND1-2"));
    }
#endif

halt:
    printf("systen exit\n");
    return 0;

}
#endif

/*********************************************************************
*
*       TMR0_IRQHandler_TouchTask
*/
#if GUI_SUPPORT_TOUCH
static void * worker_touching ( void *pvArgs )
{
	char szFileName[32];
	char szCalibrationFile[36];
	int hFile;

  printf("fsInitFileSystem.\n");
  fsInitFileSystem();	
#ifdef STORAGE_SD
    /*-----------------------------------------------------------------------*/
    /*  Init SD card                                                         */
    /*-----------------------------------------------------------------------*/
    /* clock from PLL */
    sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq)/1000, 0, 0);
    sicOpen();
    printf("total sectors (%x)\n", sicSdOpen0());
#else
    StorageForNAND();
#endif

	// Initialize
	Init_TouchPanel();

	sprintf(szFileName, "C:\\ts_calib");
	fsAsciiToUnicode(szFileName, szCalibrationFile, TRUE);
	hFile = fsOpenFile(szCalibrationFile, szFileName, O_RDONLY | O_FSEEK);
	printf("file = %d\n", hFile);
	if (hFile < 0)
	{
			// file does not exists, so do calibration
			hFile = fsOpenFile(szCalibrationFile, szFileName, O_CREATE|O_RDWR | O_FSEEK);
			if ( hFile < 0 )
			{
					printf("CANNOT create the calibration file\n");
					goto exit_worker_touching;
			}
			GUI_Init();
			ts_calibrate(LCD_XSIZE, LCD_YSIZE);
			ts_writefile(hFile);
	}
	else
			ts_readfile(hFile);
	
	fsCloseFile(hFile);

#ifndef STORAGE_SD
	GNAND_UnMountNandDisk(&ptNDisk);
	sicClose();
#endif
	
	while(1)
	{
		usleep(1000);	//1ms
		TouchTask();
	}

exit_worker_touching:	
	return NULL;
}
#endif


/*********************************************************************
*
*       _SYS_Init
*/
static void prvSetupHardware(void)
{
    WB_UART_T uart;
    UINT32 u32ExtFreq;

    u32ExtFreq = sysGetExternalClock();
    sysUartPort(1);
    uart.uart_no = WB_UART_1;
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);

    sysEnableCache(CACHE_WRITE_BACK);
}

/*********************************************************************
*
*       main
*/
extern void MainTask(void);
static void * worker_emwin ( void *pvArgs )
{
	MainTask();	
	return NULL;
}

extern UINT8 u8FrameBuf_VIDEO[LCD_XSIZE*LCD_YSIZE*2] __attribute__((aligned(32)));
static void * worker_video( void * pvArgs )
{
	unsigned char color=0;
	UINT32 *pu32FrameBufPtr  = (UINT32 *)((UINT32)u8FrameBuf_VIDEO | 0x80000000);		
	while(1)
	{
		memset( (void*)pu32FrameBufPtr, color, sizeof(u8FrameBuf_VIDEO) );
		color++;
		usleep(100*1000);
	}
	return NULL;
}

int main(void)
{
	pthread_t pxID_worker_emwin;
	pthread_t pxID_worker_touching;
	pthread_t pxID_worker_video;
	pthread_attr_t attr;

	prvSetupHardware();
	
#if GUI_SUPPORT_TOUCH
	pthread_attr_init( &attr );
	
	pthread_create( &pxID_worker_touching, NULL, worker_touching, NULL);
#endif
	
	pthread_create( &pxID_worker_emwin, NULL, worker_emwin, NULL );
	pthread_create( &pxID_worker_video, NULL, worker_video, NULL );

	vTaskStartScheduler();
	for(;;);
	return 0;
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}

void vApplicationMallocFailedHook( void ){
		printf("MMMMMMMMMMM FreeRTOS malloc failed MMMMMMMMMMMMM\n");
}

// We need this when configSUPPORT_STATIC_ALLOCATION is enabled
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ) {

// This is the static memory (TCB and stack) for the idle task
static StaticTask_t xIdleTaskTCB; // __attribute__ ((section (".rtos_heap")));
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE]; // __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));


    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint16_t *pusTimerTaskStackSize )
{
/* The buffers used by the Timer/Daemon task must be static so they are
persistent, and so exist after this function returns. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* configUSE_STATIC_ALLOCATION is set to 1, so the application has the
	opportunity to supply the buffers that will be used by the Timer/RTOS daemon
	task as its	stack and to hold its TCB.  If these are set to NULL then the
	buffers will be allocated dynamically, just as if xTaskCreate() had been
	called. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pusTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; /* In words.  NOT in bytes! */
}

/*************************** End of file ****************************/

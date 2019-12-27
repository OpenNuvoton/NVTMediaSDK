//		Copyright (c) 2019 Nuvoton Technology Corp. All rights reserved.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either m_uiVersion 2 of the License, or
//      (at your option) any later m_uiVersion.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include <stdio.h>
#include <stdlib.h>
#include <FreeRTOS.h>

#include "wblib.h"
#include "task.h"

#include "NuMedia.h"
#include "NuMedia_SAL_OS.h"

#include "MainTask.h"

#define mainMAIN_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )

static void SetupHardware(void)
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	WB_UART_T sUart;

	// initial MMU but disable system cache feature
	//sysEnableCache(CACHE_DISABLE);
	// Cache on
	sysEnableCache(CACHE_WRITE_BACK);

	sUart.uart_no = WB_UART_1;
	sUart.uiFreq = EXTERNAL_CRYSTAL_CLOCK;	//use APB clock
	sUart.uiBaudrate = 115200;
	sUart.uiDataBits = WB_DATA_BITS_8;
	sUart.uiStopBits = WB_STOP_BITS_1;
	sUart.uiParity = WB_PARITY_NONE;
	sUart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&sUart);
	sysSetLocalInterrupt(ENABLE_IRQ);
}

int main(void)
{
	
	//For heap usage checking code
#if 0
	uint32_t u32MemAddr = 0x5E0000;

	u32MemAddr += 0x10000;
	u32MemAddr &= 0xFFFF0000;
	
	while(u32MemAddr < 0x800000){
		memset((void *)u32MemAddr, 0xEE, 1024);
		u32MemAddr += 0x10000;
	}
#endif
	
	SetupHardware();

	xTaskCreate( MainTask, "Main", configMINIMAL_STACK_SIZE, NULL, mainMAIN_TASK_PRIORITY, NULL );
	
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	for(;;);
}

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
		sysprintf("MMMMMMMMMMM FreeRTOS malloc failed MMMMMMMMMMMMM\n");
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





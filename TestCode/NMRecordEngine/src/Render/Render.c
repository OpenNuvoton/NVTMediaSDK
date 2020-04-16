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
#include <inttypes.h>

#include "Render.h"

#include "N9H26_VPOST.h"
#include "VideoIn.h"

#define VPOST_FRAMEBUFFER_SIZE		(LCD_PANEL_WIDTH  *LCD_PANEL_HEIGHT * 2)

static uint8_t s_au8BlankFrameBuffer[VPOST_FRAMEBUFFER_SIZE];
static uint8_t *s_pu8CurFBAddr;

extern void vpostSetFrameBuffer(UINT32 pFramebuf);

static void VPOST_InterruptServiceRiuntine()
{
	vpostSetFrameBuffer((uint32_t)s_pu8CurFBAddr);
}


static void InitVPOST(uint8_t* pu8FrameBuffer)
{		
	PFN_DRVVPOST_INT_CALLBACK fun_ptr;
	LCDFORMATEX lcdFormat;	

	//Set source format to YUV422
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;//DRVVPOST_FRAME_YCBYCR;  //DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = LCD_PANEL_WIDTH;
	lcdFormat.nScreenHeight = LCD_PANEL_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	
}	


//////////////////////////////////////////////////////////////////////////////////////////////////////////

int
Render_Init(void)
{
	
	//Init VPOST
	memset(s_au8BlankFrameBuffer, 0x0, VPOST_FRAMEBUFFER_SIZE);
	s_pu8CurFBAddr = s_au8BlankFrameBuffer;
	InitVPOST(s_pu8CurFBAddr);

	return 0;
}

void
Render_Final(void)
{
	s_pu8CurFBAddr = s_au8BlankFrameBuffer;
	vpostLCMDeinit();
}

void
Render_SetFrameBuffAddr(
	uint8_t *pu8FBAddr
)
{
	//Set VPOST frame buffer address
	s_pu8CurFBAddr = pu8FBAddr;
}





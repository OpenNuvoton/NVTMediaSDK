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

#ifndef __SENSOR_IF_H__
#define __SENSOR_IF_H__

#include <inttypes.h>

#include "VinConfig.h"
#include "N9H26_VideoIn.h"

typedef struct{
	uint32_t u32PlanarWidth;
	uint32_t u32PlanarHeight;
	uint32_t u32PacketWidth;
	uint32_t u32PacketHeight;
	E_VIDEOIN_OUT_FORMAT ePacketColorFormat;
	E_VIDEOIN_PLANAR_FORMAT ePlanarColorFormat;
	uint8_t *pu8PacketFrameBuf;
	uint8_t *pu8PlanarFrameBuf;
	uint32_t u32FrameRate;

	uint32_t u32LowLuxGate;
	uint32_t u32HighLuxGate;
	uint32_t u32Flicker;

	uint32_t u32SnrInputWidth;
	uint32_t u32SnrInputHeight;
	
	uint32_t u32CapEngCropWinW;		//capture engine crop window width
	uint32_t u32CapEngCropWinH;		//capture engine crop window width

	uint32_t u32CapEngCropStartX;		//capture engine crop start x
	uint32_t u32CapEngCropStartY;		//capture engine crop start y

	uint32_t u32AEMaxLum;				//AE maximum target luminance
	uint32_t u32AEMinLum;				//AE minimum tartget luminance
}S_SENSOR_ATTR;

typedef struct{
	uint32_t u32StartX;				//AE inner window start x
	uint32_t u32StartY;				//AE inner window start y
	uint32_t u32EndX;				//AE inner window end x
	uint32_t u32EndY;				//AE inner window end y
}S_AE_INNER_PARAM;

typedef struct{
	uint32_t u32SnrMaxWidth;
	uint32_t u32SnrMaxHeight;
	BOOL bIsInit;
	BOOL bIsI2COpened;
	
	int32_t (*pfnSenesorSetup)(				//sensor setup 
		VINDEV_T *psVinDev,
		S_SENSOR_ATTR *psSnrAttr,
		uint32_t u32PortNo,
		BOOL b32MasterPort,
		PFN_VIDEOIN_CALLBACK pfnIntCB
	);

	void (*pfnSensorRelease)(					//sensor release
		VINDEV_T *psVinDev
	);

	int32_t (*pfnLowLuxDet)(
		S_SENSOR_ATTR *psSnrAttr,
		BOOL *pbLowLuxDet
	);

	int32_t (*pfnSetNightMode)(
		BOOL bEnable
	);

	int32_t (*pfnSetFlicker)(
		S_SENSOR_ATTR *psSnrAttr
	);

	int32_t (*pfnSetSnrReg)(
		uint32_t u32RegAddr,
		uint32_t u32RegValue
	);

	int32_t (*pfnGetSnrReg)(
		uint32_t u32RegAddr,
		uint32_t *pu32RegValue
	);

	int32_t (*pfnSetAEInnerLum)(
		S_SENSOR_ATTR *psSnrAttr,
		S_AE_INNER_PARAM *psAEInnerParam,
		BOOL bIncLum
	);

	int32_t (*pfnSetFrameRate)(
		uint32_t u32PortNo,
		S_SENSOR_ATTR *psSnrAttr
	);

}	S_SENSOR_IF;


extern S_SENSOR_IF g_sSensorNT99141;
extern S_SENSOR_IF g_sSensorNT99142;
extern S_SENSOR_IF g_sSensorGC0308;
extern S_SENSOR_IF g_sSensorHM2056;

#endif


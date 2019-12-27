/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "wblib.h"
#include "ImgProc.h"
#include "VPE.h"

#define UTIL_DDA(u32FactorM, u32FactorN, u32Delta, u32ReptCnt)  { u32ReptCnt = (u32Delta == 0); \
                                                                  u32Delta += u32FactorM; \
                                                                  while (u32Delta > u32FactorN) \
                                                                  { u32ReptCnt++; u32Delta -= u32FactorN; } \
                                                                }


static void 
HMirrorImgRGB565_SW(
	uint8_t *pu8SrcImgBuf,
	uint8_t *pu8DestImgBuf,
	uint32_t u32ImgWidth,
	uint32_t u32ImgHeight,
	uint32_t u32SrcImgStride,
	uint32_t u32DestImgStride	
)
{
	uint32_t *pu32Swap1;	
	uint32_t *pu32Swap2;	
	uint32_t *pu32SwapSrc;	
	uint32_t *pu32DestSrc;	
	uint32_t u32SwapCnt;	
	uint32_t x;
	uint32_t y;
	uint32_t u32Temp1;
	uint32_t u32Temp2;
	uint32_t *pu32Dest1;	
	uint32_t *pu32Dest2;	

	u32SwapCnt = (u32ImgWidth*2)/4/2;
	for(y = 0 ; y < u32ImgHeight; y ++){
		pu32SwapSrc = (uint32_t *)(pu8SrcImgBuf + (y * u32SrcImgStride * 2));
		pu32DestSrc = (uint32_t *)(pu8DestImgBuf + (y * u32DestImgStride * 2));
		for(x = 0 ; x < u32SwapCnt; x++){
			pu32Swap1 = pu32SwapSrc + x;
			pu32Swap2 = pu32SwapSrc + ((u32SwapCnt*2) - 1 -x);
			pu32Dest1 = pu32DestSrc + x;
			pu32Dest2 = pu32DestSrc + ((u32SwapCnt*2) - 1 -x);
						
			u32Temp1 = *pu32Swap1;
			u32Temp2 = *pu32Swap2;

			*pu32Dest1 = u32Temp2;
			*pu32Dest2 = u32Temp1;
		}
	}
}

static void 
VMirrorImg_SW(
	uint8_t *pu8SrcImgBuf,
	uint8_t *pu8DestImgBuf,
	uint32_t u32ImgWidth,
	uint32_t u32ImgHeight,
	uint32_t u32SrcImgStride,
	uint32_t u32DestImgStride	
)
{
	uint16_t *pu16SwapSrc1;	
	uint16_t *pu16SwapSrc2;	
	uint16_t *pu16SwapDest1;	
	uint16_t *pu16SwapDest2;	
	uint32_t u32SwapCnt;	
	uint32_t x;
	uint32_t y;
	uint16_t u16Temp1;
	uint16_t *pu16SrcImgBuf = (uint16_t *)pu8SrcImgBuf;	
	uint16_t *pu16DestImgBuf = (uint16_t *)pu8DestImgBuf;	
	uint32_t u32SrcYAddr1;
	uint32_t u32SrcYAddr2;
	uint32_t u32DestYAddr1;
	uint32_t u32DestYAddr2;

	u32SwapCnt = u32ImgHeight/2;
	for(y = 0 ; y < u32SwapCnt; y++){
		u32SrcYAddr1 = y * u32SrcImgStride;
		u32SrcYAddr2 = ((u32SwapCnt*2) - 1 - y) * u32SrcImgStride;
		u32DestYAddr1 = y * u32DestImgStride;
		u32DestYAddr2 = ((u32SwapCnt*2) - 1 - y) * u32DestImgStride;
		for(x = 0 ; x < u32ImgWidth; x ++){
			pu16SwapSrc1 = (uint16_t *)(pu16SrcImgBuf + ((u32SrcYAddr1 + x)));
			pu16SwapSrc2 = (uint16_t *)(pu16SrcImgBuf + ((u32SrcYAddr2 + x)));
			pu16SwapDest1 = (uint16_t *)(pu16DestImgBuf + ((u32SrcYAddr1 + x)));
			pu16SwapDest2 = (uint16_t *)(pu16DestImgBuf + ((u32SrcYAddr2 + x)));

			u16Temp1 = *pu16SwapSrc1;

			*pu16SwapDest1 = *pu16SwapSrc2;
			*pu16SwapDest2 = u16Temp1;
		}
	}
}


static void HMirrorImgYUV422_SW(
	uint8_t *pu8SrcImgBuf,
	uint8_t *pu8DestImgBuf,
	uint32_t u32ImgWidth,
	uint32_t u32ImgHeight,
	uint32_t u32SrcImgStride,
	uint32_t u32DestImgStride
)
{
	uint32_t *pu32SwapSrc1;	
	uint32_t *pu32SwapSrc2;	
	uint32_t *pu32SwapDest1;	
	uint32_t *pu32SwapDest2;	
	uint32_t *pu32SwapSrc;	
	uint32_t *pu32SwapDest;	
	uint32_t u32SwapCnt;	
	uint32_t x;
	uint32_t y;
	uint32_t u32Temp1;
	uint8_t *pu8Temp1;
	uint32_t u32Temp2;
	uint8_t *pu8Temp2;
	uint32_t Y0;
	uint32_t Y1;
	uint32_t U;
	uint32_t V;

	u32SwapCnt = (u32ImgWidth*2)/4/2;
	for(y = 0 ; y < u32ImgHeight; y ++){
		pu32SwapSrc = (uint32_t *)(pu8SrcImgBuf + (y * u32SrcImgStride * 2));
		pu32SwapDest = (uint32_t *)(pu8DestImgBuf + (y * u32DestImgStride * 2));
		for(x = 0 ; x < u32SwapCnt; x++){
			pu32SwapSrc1 = pu32SwapSrc + x;
			pu32SwapSrc2 = pu32SwapSrc + ((u32SwapCnt*2) - 1 -x);			
			pu32SwapDest1 = pu32SwapDest + x;
			pu32SwapDest2 = pu32SwapDest + ((u32SwapCnt*2) - 1 -x);			
			pu8Temp1 = (uint8_t *)pu32SwapSrc1;
			pu8Temp2 = (uint8_t *)pu32SwapSrc2;
			Y0 = *pu8Temp1; 
			U = *(pu8Temp1 + 1); 
			Y1 = *(pu8Temp1 + 2); 
			V = *(pu8Temp1 + 3);
			u32Temp1 = (Y1) | (U << 8) | (Y0 << 16) | (V << 24); 

			Y0 = *pu8Temp2; 
			U = *(pu8Temp2 + 1); 
			Y1 = *(pu8Temp2 + 2); 
			V = *(pu8Temp2 + 3);
			u32Temp2 = (Y1) | (U << 8) | (Y0 << 16) | (V << 24); 

			*pu32SwapDest1 = u32Temp2;
			*pu32SwapDest2 = u32Temp1;
		}
	}
}


static void 
Scale_SW(
	uint8_t *pu8SrcBuf, 
	uint8_t *pu8DestBuf, 
	S_IMGPROC_SCALE_PARAM *psScaleParam
)
{
	register uint16_t *pu16SrcBuf;
	register uint16_t *pu16DestBuf;
	register uint32_t *pu32SrcBuf;
	register uint32_t *pu32DestBuf;


	register uint16_t u16CropWinXPos;
	uint16_t u16CropWinYPos;
	
	register uint16_t u16DestWinXPos = 0;
	uint16_t u16DestWinYPos = 0;
	
	uint32_t u32RepeatCntY = 0;
	uint32_t u32DeltaY = 0;		// Reset Y DDA
	uint32_t u32TotalCntY = 0;

	register uint32_t u32RepeatCntX	= 0;
	uint32_t u32DeltaX = 0;		// Reset X DDA
	register uint32_t u32TotalCntX = 0;

	register uint16_t *pu16CurSrcRowPos;
	register uint16_t *pu16CurDstRowPos;
	
	uint32_t u32CropWinStartX = psScaleParam->u32CropWinStartX;
	register uint32_t u32CropWinEndX = u32CropWinStartX + psScaleParam->u32CropWinWidth;
	register uint32_t u32ScaleOutXFac = psScaleParam->u32ScaleOutXFac;
	register uint32_t u32ScaleInXFac = psScaleParam->u32ScaleInXFac;
	register uint32_t u32ScaleImgWidth = psScaleParam->u32ScaleImgWidth;

	uint32_t u32CropWinStartY = psScaleParam->u32CropWinStartY;
	uint32_t u32CropWinEndY = u32CropWinStartY + psScaleParam->u32CropWinHeight;
	uint32_t u32ScaleOutYFac = psScaleParam->u32ScaleOutYFac;
	uint32_t u32ScaleInYFac = psScaleParam->u32ScaleInYFac;
	uint32_t u32ScaleImgHeight = psScaleParam->u32ScaleImgHeight;
	uint32_t u32ScaleImgStride = psScaleParam->u32ScaleImgStride;

	uint32_t u32SrcImgWidth = psScaleParam->u32SrcImgWidth;
	uint32_t u32SrcImgStride = psScaleParam->u32SrcImgStride;

	pu16SrcBuf = (uint16_t *)pu8SrcBuf;
	pu16DestBuf = (uint16_t *)pu8DestBuf;
	pu32SrcBuf = (uint32_t *)pu8SrcBuf;
	pu32DestBuf = (uint32_t *)pu8DestBuf;


	for(u16CropWinYPos = u32CropWinStartY; 
				u16CropWinYPos <  u32CropWinEndY;  u16CropWinYPos ++){
//		u32RepeatCntY = Util_DDA(psScaleUp->u32ScaleOutYFac, psScaleUp->u32ScaleInYFac, &u32DeltaY);
		UTIL_DDA(u32ScaleOutYFac, u32ScaleInYFac, u32DeltaY ,u32RepeatCntY);

		if(u32RepeatCntY == 0)
			continue;
			
		if(u32TotalCntY + u32RepeatCntY >= u32ScaleImgHeight){
			if(u32ScaleImgHeight > u32TotalCntY){
				u32RepeatCntY = u32ScaleImgHeight - u32TotalCntY;
			}
			else{
				continue;
			}
		} 
		
		u32TotalCntY += u32RepeatCntY;
		while(u32RepeatCntY --){
			u32DeltaX = 0;
			u32TotalCntX = 0;
			u16DestWinXPos = 0;
			
			pu16CurSrcRowPos = pu16SrcBuf + (u16CropWinYPos * u32SrcImgStride);
			pu16CurDstRowPos = pu16DestBuf + (u16DestWinYPos * u32ScaleImgStride);
			
			for(u16CropWinXPos = u32CropWinStartX; 
							u16CropWinXPos <  (u32CropWinEndX);  u16CropWinXPos +=2){
//				u32RepeatCntX = Util_DDA(u32ScaleOutXFac, u32ScaleInXFac, &u32DeltaX);
				UTIL_DDA(u32ScaleOutXFac, u32ScaleInXFac, u32DeltaX ,u32RepeatCntX);

				if(u32RepeatCntX == 0)
					continue;

				if((u32TotalCntX + (u32RepeatCntX  << 1)) >= u32ScaleImgWidth){
					if(u32ScaleImgWidth > u32TotalCntX)
						u32RepeatCntX = (u32ScaleImgWidth - u32TotalCntX) / 2;
					else
						continue;
				}
			
				u32TotalCntX += (u32RepeatCntX << 1);
				pu32SrcBuf = (uint32_t *)( pu16CurSrcRowPos + u16CropWinXPos);
				while(u32RepeatCntX --){
/*
					pu16DestBuf[u32CurDstRowPos + u16DestWinXPos] = 
								pu16SrcBuf[u32CurSrcRowPos + u16CropWinXPos];

					pu16DestBuf[u32CurDstRowPos + u16DestWinXPos + 1] = 
								pu16SrcBuf[u32CurSrcRowPos + u16CropWinXPos + 1];
*/
					pu32DestBuf = (uint32_t *)( pu16CurDstRowPos + u16DestWinXPos);
					*pu32DestBuf = *pu32SrcBuf;
					
					u16DestWinXPos += 2;
				}
			}
			u16DestWinYPos ++;
		}
	}
}

static void 
YUV422PacketToPlanar_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint8_t u8VComp = 0;
	uint16_t x,y;
	uint16_t *pu16SrcImg = (uint16_t *)pu8SrcImg;
	uint32_t u32SrcYAddr;
	uint8_t *pu8DestYComp = pu8DestImg;
	uint8_t *pu8DestUComp = pu8DestYComp + (u16SrcImgWidth*u16SrcImgHeight);
	uint8_t *pu8DestVComp = pu8DestUComp + ((u16SrcImgWidth*u16SrcImgHeight)/2);
	uint16_t u16Temp;
	uint8_t u8YVal;
	uint8_t u8UVVal;

	for(y = 0; y < u16SrcImgHeight; y ++){
		u8VComp = 0;
		u32SrcYAddr = y * u16SrcImgWidth; 
		for(x = 0; x < u16SrcImgWidth; x ++){
			u16Temp = pu16SrcImg[u32SrcYAddr + x];
			u8YVal = (uint8_t)(u16Temp & 0x00FF);
			u8UVVal = (uint8_t)((u16Temp & 0xFF00) >> 8);
			*pu8DestYComp = u8YVal;
			pu8DestYComp++;
			if(u8VComp){
				*pu8DestVComp = u8UVVal; 
				pu8DestVComp ++;
			}
			else{
				*pu8DestUComp = u8UVVal; 
				pu8DestUComp ++;
			}
			u8VComp = u8VComp ^ 1;
		}
	} 
}

static void 
YUV422PlanarToPacket_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint8_t u8VComp = 0;
	uint16_t x,y;
	uint16_t *pu16DestImg = (uint16_t *)pu8DestImg;
	uint8_t *pu8SrcYComp = pu8SrcImg;
	uint8_t *pu8SrcUComp = pu8SrcYComp + (u16SrcImgWidth*u16SrcImgHeight);
	uint8_t *pu8SrcVComp = pu8SrcUComp + ((u16SrcImgWidth*u16SrcImgHeight)/2);
	uint16_t u16Temp;
	uint8_t u8YVal;
	uint8_t u8UVVal;
	uint32_t u32DestYAddr;


	for(y = 0; y < u16SrcImgHeight; y ++){
		u8VComp = 0;
		u32DestYAddr = y * u16SrcImgWidth; 
		for(x = 0; x < u16SrcImgWidth; x ++){
			u8YVal = *pu8SrcYComp;
			pu8SrcYComp++;
			if(u8VComp){
				u8UVVal = *pu8SrcVComp; 
				pu8SrcVComp ++;
			}
			else{
				u8UVVal = *pu8SrcUComp;
				pu8SrcUComp ++;
			}
			u8VComp = u8VComp ^ 1;
			u16Temp = (uint16_t)u8UVVal;
			u16Temp = (u16Temp << 8) | (uint16_t)u8YVal;
			pu16DestImg[u32DestYAddr + x] = u16Temp;
		}
	}
}

static void 
YUV420PlanarToYUV422_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y;
	uint16_t *pu16DestImg = (uint16_t *)pu8DestImg;
	uint8_t *pu8SrcYComp = pu8SrcImg;
	uint8_t *pu8SrcUComp = pu8SrcYComp + (u16SrcImgWidth*u16SrcImgHeight);
	uint8_t *pu8SrcVComp = pu8SrcUComp + ((u16SrcImgWidth*u16SrcImgHeight)/4);
	uint32_t u32DestY0Addr;
	uint32_t u32DestY2Addr;
	uint32_t u32SrcY0Addr;
	uint32_t u32SrcY2Addr;
	uint8_t u8Y0;
	uint8_t u8Y1;
	uint8_t u8U0;
	uint8_t u8Y2;
	uint8_t u8Y3;
	uint8_t u8V0;

	for( y = 0; y < u16SrcImgHeight ; y += 2){
		u32DestY0Addr = y * u16SrcImgWidth; 
		u32DestY2Addr = (y+1) * u16SrcImgWidth; 
		u32SrcY0Addr = y * u16SrcImgWidth; 
		u32SrcY2Addr = (y+1) * u16SrcImgWidth; 
		for( x = 0; x < u16SrcImgWidth ; x += 2){
			u8Y0 = pu8SrcYComp[u32SrcY0Addr + x];
			u8Y1 = pu8SrcYComp[u32SrcY0Addr + x + 1];
			u8Y2 = pu8SrcYComp[u32SrcY2Addr + x];
			u8Y3 = pu8SrcYComp[u32SrcY2Addr + x + 1];
			u8U0 = *pu8SrcUComp;
			u8V0 = *pu8SrcVComp;
			pu8SrcUComp++;
			pu8SrcVComp++;
			
			pu16DestImg[u32DestY0Addr + x] = ((uint16_t)u8U0 << 8) |  (uint16_t)u8Y0;
			pu16DestImg[u32DestY0Addr + x + 1] = ((uint16_t)u8V0 << 8) |  (uint16_t)u8Y1;

			pu16DestImg[u32DestY2Addr + x] = ((uint16_t)u8U0 << 8) |  (uint16_t)u8Y2;
			pu16DestImg[u32DestY2Addr + x + 1] = ((uint16_t)u8V0 << 8) |  (uint16_t)u8Y3;
		}
	}
}

static void 
YUV422ToYUV420Planar_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y;
	uint16_t *pu16SrcImg = (uint16_t *)pu8SrcImg;
	uint8_t *pu8DestYComp = pu8DestImg;
	uint8_t *pu8DestUComp = pu8DestYComp + (u16SrcImgWidth*u16SrcImgHeight);
	uint8_t *pu8DestVComp = pu8DestUComp + ((u16SrcImgWidth*u16SrcImgHeight)/4);

	uint8_t u8Y0;
	uint8_t u8Y1;
	uint8_t u8U0;
	uint8_t u8V0;
	uint8_t u8Y2;
	uint8_t u8Y3;
	uint8_t u8U1;
	uint8_t u8V1;

	uint16_t u16Temp;
	uint32_t u32SrcY0Addr;
	uint32_t u32SrcY2Addr;
	uint32_t u32DestY0Addr;
	uint32_t u32DestY2Addr;

	for( y = 0; y < u16SrcImgHeight ; y += 2){
		u32SrcY0Addr = y * u16SrcImgWidth;
		u32SrcY2Addr = (y + 1) * u16SrcImgWidth;
		u32DestY0Addr = y * u16SrcImgWidth; 
		u32DestY2Addr = (y+1) * u16SrcImgWidth; 
		for( x = 0; x < u16SrcImgWidth ; x += 2){
			u16Temp = pu16SrcImg[u32SrcY0Addr + x];
			u8Y0 = (uint8_t)(u16Temp & 0x00FF);
			u8U0 = (uint8_t)((u16Temp & 0xFF00) >> 8);
			u16Temp = pu16SrcImg[u32SrcY0Addr + x + 1];
			u8Y1 = (uint8_t)(u16Temp & 0x00FF);
			u8V0 = (uint8_t)((u16Temp & 0xFF00) >> 8);
			u16Temp = pu16SrcImg[u32SrcY2Addr + x];
			u8Y2 = (uint8_t)(u16Temp & 0x00FF);
			u8U1 = (uint8_t)((u16Temp & 0xFF00) >> 8);
			u16Temp = pu16SrcImg[u32SrcY2Addr + x + 1];
			u8Y3 = (uint8_t)(u16Temp & 0x00FF);
			u8V1 = (uint8_t)((u16Temp & 0xFF00) >> 8);

			pu8DestYComp[u32DestY0Addr + x] = u8Y0;
			pu8DestYComp[u32DestY0Addr + x + 1] = u8Y1;
			pu8DestYComp[u32DestY2Addr + x] = u8Y2;
			pu8DestYComp[u32DestY2Addr + x + 1] = u8Y3;
			*pu8DestUComp = (u8U0 + u8U1) >> 1;
			*pu8DestVComp = (u8V0 + u8V1) >> 1;
			pu8DestUComp++;
			pu8DestVComp++;
		}
	}
}

#define ONE_MB_Y_WIDTH 		16
#define ONE_MB_Y_HEIGHT 	16
#define ONE_MB_Y_SIZE		(ONE_MB_Y_WIDTH * ONE_MB_Y_HEIGHT)


#define ONE_MB_UV_WIDTH 	8
#define ONE_MB_UV_HEIGHT 	16
#define ONE_MB_UV_SIZE		(ONE_MB_UV_WIDTH * ONE_MB_UV_HEIGHT)


static void 
YUV420MBToYUV422_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y,i;
	uint32_t u32MBLine = 0;
	uint32_t u32MBCol = 0;
	uint32_t u32LineOffInOneMB = 0;
	uint32_t u32ColOffInOneMB = 0;
	uint32_t u32MBYPos = 0;

	uint8_t *pu8SrcYAddr = NULL;
	uint8_t *pu8DestYAddr = NULL;

	uint8_t *pu8UVSrc;
	uint8_t *pu8SrcUVAddr = NULL;
	uint8_t *pu8DestUVAddr_Plus = NULL;
	uint8_t *pu8DestUVAddr_Minus = NULL;
	uint8_t *pu8DestUVAddr = NULL;;
	uint32_t u32MBUVPos = 0;

	uint32_t u32WidthInMB = (u16SrcImgWidth + ONE_MB_Y_WIDTH - 1) / ONE_MB_Y_WIDTH;

	//Process YUV422 Y
	for( y = 0; y < u16SrcImgHeight ; y ++){
		for( x = 0; x < u16SrcImgWidth ; x += ONE_MB_Y_WIDTH){
			u32MBLine = y / ONE_MB_Y_HEIGHT;
			u32LineOffInOneMB = y & (ONE_MB_Y_HEIGHT - 1);

			u32MBCol = x / ONE_MB_Y_WIDTH;
			u32ColOffInOneMB = x & (ONE_MB_Y_WIDTH - 1);

			u32MBYPos = (u32MBLine * u32WidthInMB + u32MBCol) * ONE_MB_Y_SIZE;
			u32MBYPos = u32MBYPos + (u32LineOffInOneMB * ONE_MB_Y_WIDTH + u32ColOffInOneMB);

			pu8SrcYAddr = pu8SrcImg + u32MBYPos;
			pu8DestYAddr = pu8DestImg + (((y * u16SrcImgWidth) + x) * 2);

			for(i = 0; i < ONE_MB_Y_WIDTH; i ++){
				pu8DestYAddr[i * 2] = pu8SrcYAddr[i];
			}
		}
	}

	pu8UVSrc = pu8SrcImg + (u16SrcImgWidth * u16SrcImgHeight);
	u32WidthInMB = ((u16SrcImgWidth / 2) + ONE_MB_UV_WIDTH - 1) / ONE_MB_UV_WIDTH;

	//Process YUV422 UV
	for( y = 0; y < u16SrcImgHeight ; y ++){
		for( x = 0; x < u16SrcImgWidth ; x += (2 * ONE_MB_UV_WIDTH)){
			u32MBLine = y / ONE_MB_UV_HEIGHT;
			u32LineOffInOneMB = y & (ONE_MB_UV_HEIGHT - 1);

			u32MBCol = x / 2 / ONE_MB_UV_WIDTH;
			u32ColOffInOneMB = (x / 2) & (ONE_MB_UV_WIDTH - 1);

			u32MBUVPos = (u32MBLine * u32WidthInMB + u32MBCol) * ONE_MB_UV_SIZE;
			u32MBUVPos = u32MBUVPos + (u32LineOffInOneMB * ONE_MB_UV_WIDTH + u32ColOffInOneMB);

			pu8DestUVAddr = pu8DestImg + (((y * u16SrcImgWidth) + x) * 2);

			if(y & 0x01)
				pu8DestUVAddr_Minus = pu8DestImg + ((((y - 1) * u16SrcImgWidth) + x) * 2);
			else
				pu8DestUVAddr_Plus = pu8DestImg + ((((y + 1) * u16SrcImgWidth) + x) * 2);

			pu8SrcUVAddr = pu8UVSrc + u32MBUVPos;

			for(i = 0; i < ONE_MB_UV_WIDTH; i ++){
				if(y & 0x01){
					//It is v compoment
					pu8DestUVAddr[(i * 4) + 3] = pu8SrcUVAddr[i];
					pu8DestUVAddr_Minus[(i * 4) + 3] = pu8SrcUVAddr[i];
				}
				else{
					//It is u compoment
					pu8DestUVAddr[(i * 4) + 1] = pu8SrcUVAddr[i];
					pu8DestUVAddr_Plus[(i * 4) + 1] = pu8SrcUVAddr[i];
				}
			}
		}
	}
}


static void 
YUV422ToYUV420MB_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y,i;
	uint32_t u32MBLine = 0;
	uint32_t u32MBCol = 0;
	uint32_t u32LineOffInOneMB = 0;
	uint32_t u32ColOffInOneMB = 0;
	uint32_t u32MBYPos = 0;

	uint8_t *pu8SrcYAddr = NULL;
	uint8_t *pu8DestYAddr = NULL;;

	uint8_t *pu8UVDest;
	uint8_t *pu8SrcUVAddr = NULL;
	uint8_t *pu8SrcUVAddr_Plus = NULL;
	uint8_t *pu8SrcUVAddr_Minus = NULL;
	uint8_t *pu8DestUVAddr = NULL;;
	uint32_t u32MBUVPos = 0;
	uint32_t u32UVData = 0;

	uint32_t u32WidthInMB = (u16SrcImgWidth + ONE_MB_Y_WIDTH - 1) / ONE_MB_Y_WIDTH;

	//Process YUV422 Y
	for( y = 0; y < u16SrcImgHeight ; y ++){
		for( x = 0; x < u16SrcImgWidth ; x += ONE_MB_Y_WIDTH){
			u32MBLine = y / ONE_MB_Y_HEIGHT;
			u32LineOffInOneMB = y & (ONE_MB_Y_HEIGHT - 1);

			u32MBCol = x / ONE_MB_Y_WIDTH;
			u32ColOffInOneMB = x & (ONE_MB_Y_WIDTH - 1);

			u32MBYPos = (u32MBLine * u32WidthInMB + u32MBCol) * ONE_MB_Y_SIZE;
			u32MBYPos = u32MBYPos + (u32LineOffInOneMB * ONE_MB_Y_WIDTH + u32ColOffInOneMB);

			pu8SrcYAddr = pu8SrcImg + (((y * u16SrcImgWidth) + x) * 2);
			pu8DestYAddr = pu8DestImg + u32MBYPos;

			for(i = 0; i < ONE_MB_Y_WIDTH; i ++){
				pu8DestYAddr[i] = pu8SrcYAddr[i*2];
			}
		}
	}

	pu8UVDest = pu8DestImg + (u16SrcImgWidth * u16SrcImgHeight);
	u32WidthInMB = ((u16SrcImgWidth / 2) + ONE_MB_UV_WIDTH - 1) / ONE_MB_UV_WIDTH;
	
	//Process YUV422 UV
	for( y = 0; y < u16SrcImgHeight ; y ++){
		for( x = 0; x < u16SrcImgWidth ; x += (2 * ONE_MB_UV_WIDTH)){
			u32MBLine = y / ONE_MB_UV_HEIGHT;
			u32LineOffInOneMB = y & (ONE_MB_UV_HEIGHT - 1);

			u32MBCol = x / 2 / ONE_MB_UV_WIDTH;
			u32ColOffInOneMB = (x / 2) & (ONE_MB_UV_WIDTH - 1);

			u32MBUVPos = (u32MBLine * u32WidthInMB + u32MBCol) * ONE_MB_UV_SIZE;
			u32MBUVPos = u32MBUVPos + (u32LineOffInOneMB * ONE_MB_UV_WIDTH + u32ColOffInOneMB);

			pu8SrcUVAddr = pu8SrcImg + (((y * u16SrcImgWidth) + x) * 2);

			if(y & 0x01)
				pu8SrcUVAddr_Minus = pu8SrcImg + ((((y - 1) * u16SrcImgWidth) + x) * 2);
			else
				pu8SrcUVAddr_Plus = pu8SrcImg + ((((y + 1) * u16SrcImgWidth) + x) * 2);

			pu8DestUVAddr = pu8UVDest + u32MBUVPos;

			for(i = 0; i < ONE_MB_UV_WIDTH; i ++){
				if(y & 0x01){
					//It is v compoment
					u32UVData = pu8SrcUVAddr[i * 4 + 3];
					u32UVData += pu8SrcUVAddr_Minus[i * 4 + 3];
				}
				else{
					//It is u compoment
					u32UVData = pu8SrcUVAddr[i * 4 + 1];
					u32UVData += pu8SrcUVAddr_Plus[i * 4 + 1];
				}
				u32UVData = u32UVData / 2;
				pu8DestUVAddr[i] = u32UVData;
			}
		}
	}
}


static void 
YUV422ToYUV420_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y;
	uint8_t u8Y0;
	uint8_t u8U0;
	uint8_t u8Y1;
	uint8_t u8V0;
	uint8_t u8Y2;
	uint8_t u8U1;
	uint8_t u8Y3;
	uint8_t u8V1;
	uint32_t u32SrcY0Addr;
	uint32_t u32SrcY2Addr;
	uint16_t u16TempX;

	uint32_t u32DestY0Addr;
	uint32_t u32DestY2Addr;
	
	for( y = 0; y < u16SrcImgHeight ; y += 2){
		u32SrcY0Addr = y * u16SrcImgWidth * 2;
		u32SrcY2Addr = (y + 1) * u16SrcImgWidth * 2;
		u32DestY0Addr = y * ((u16SrcImgWidth / 2) * 3);
		u32DestY2Addr = (y + 1) * ((u16SrcImgWidth / 2) * 3);
		for( x = 0; x < u16SrcImgWidth ; x += 2){
			u16TempX = x * 2;
			u8Y0 = pu8SrcImg[u32SrcY0Addr + u16TempX];
			u8U0 = pu8SrcImg[u32SrcY0Addr + u16TempX + 1];
			u8Y1 = pu8SrcImg[u32SrcY0Addr + u16TempX + 2];
			u8V0 = pu8SrcImg[u32SrcY0Addr + u16TempX + 3];

			u8Y2 = pu8SrcImg[u32SrcY2Addr + u16TempX];
			u8U1 = pu8SrcImg[u32SrcY2Addr + u16TempX + 1];
			u8Y3 = pu8SrcImg[u32SrcY2Addr + u16TempX + 2];
			u8V1 = pu8SrcImg[u32SrcY2Addr + u16TempX + 3];

			u16TempX = (x / 2) * 3;
			pu8DestImg[u32DestY0Addr + u16TempX] = u8Y0;
			pu8DestImg[u32DestY0Addr + u16TempX + 1] = u8Y1;
			pu8DestImg[u32DestY0Addr + u16TempX + 2] = (u8U0 + u8U1) >> 1;

			pu8DestImg[u32DestY2Addr + u16TempX] = u8Y2;
			pu8DestImg[u32DestY2Addr + u16TempX + 1] = u8Y3;
			pu8DestImg[u32DestY2Addr + u16TempX + 2] = (u8V0 + u8V1) >> 1;
		}
	}
}

static uint8_t s_au8SmoothCoeff[9] = {3, 4, 3, 4, 0, 4, 3, 4, 3}; 

static void
YUV422Smooth(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight,
	uint16_t u16SrcImgStride,
	uint16_t u16DestImgStride	
)
{
	int32_t x;
	int32_t y, masky;
	uint32_t i;
	uint8_t *pu8YCSrcAddr = NULL;
	uint8_t *pu8YPSrcAddr = NULL;
	uint8_t *pu8YNSrcAddr = NULL;
	uint8_t *pu8YCurSrcAddr = NULL;
	uint8_t *pu8YDestAddr = NULL;


	uint32_t u32SrcWidthSize = u16SrcImgWidth * 2;
	uint32_t u32SrcHeightSize = u16SrcImgHeight;
	uint32_t u32SrcStrideSize = u16SrcImgStride * 2;
	uint32_t u32DestStrideSize = u16DestImgStride * 2;
	int32_t i32SkipByte = 0;

	uint8_t aaMaskPixVal[3][3];

	int32_t i32Sum;

	pu8YNSrcAddr = pu8SrcImg;
	
	for(y = 0; y < u32SrcHeightSize; y ++){
		pu8YPSrcAddr = pu8YCSrcAddr;
		pu8YCSrcAddr = pu8YNSrcAddr;
		pu8YNSrcAddr = pu8SrcImg + ((y + 1) * u32SrcStrideSize);
		pu8YDestAddr = pu8DestImg + (y * u32DestStrideSize);
		for(x = 0; x < u32SrcWidthSize; x ++){
			if(x & 1){
				i32SkipByte = 4;
			}
			else{
				i32SkipByte = 2;
			}
			for(masky = 0; masky < 3; masky ++){
				if( masky == 0)
					pu8YCurSrcAddr = pu8YPSrcAddr;
				else if(masky == 1)
					pu8YCurSrcAddr = pu8YCSrcAddr;
				else
					pu8YCurSrcAddr = pu8YNSrcAddr;
					
				if(((y == 0) && (masky ==0)) || ((y == (u32SrcHeightSize -1)) && (masky == 2))){
					if((x - i32SkipByte) < 0)
						aaMaskPixVal[masky][0] = pu8YCSrcAddr[x];
					else
						aaMaskPixVal[masky][0] = pu8YCSrcAddr[x - i32SkipByte];

					aaMaskPixVal[masky][1] = pu8YCSrcAddr[x];

					if((x + i32SkipByte) > u32SrcWidthSize)
						aaMaskPixVal[masky][2] = pu8YCSrcAddr[x];
					else
						aaMaskPixVal[masky][2] = pu8YCSrcAddr[x + i32SkipByte];										
					continue;
				}
				
				if((x - i32SkipByte) < 0)
					aaMaskPixVal[masky][0] = pu8YCSrcAddr[x];
				else
					aaMaskPixVal[masky][0] = pu8YCurSrcAddr[x - i32SkipByte];

				aaMaskPixVal[masky][1] = pu8YCurSrcAddr[x];
				
				if((x + i32SkipByte) > (u32SrcWidthSize - 1))
					aaMaskPixVal[masky][2] = pu8YCSrcAddr[x];
				else
					aaMaskPixVal[masky][2] = pu8YCurSrcAddr[x + i32SkipByte];
			}

			i32Sum = 0;
			for(i = 0; i < 9 ; i++){
				i32Sum = i32Sum + (((int32_t)(aaMaskPixVal[0][i])) << s_au8SmoothCoeff[i]);
			}  

			i32Sum += (aaMaskPixVal[1][1] * 160);
			i32Sum /= 256;
	
			if(i32Sum > 255)
				i32Sum = 255;

			if(i32Sum < 0)
				i32Sum = 0;

			pu8YDestAddr[x] = i32Sum;
		}
	}
}

#if 0
static void
RGB565Smooth(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight,
	uint16_t u16SrcImgStride,
	uint16_t u16DestImgStride	
)
{
	int32_t x;
	int32_t y, masky;
	uint32_t i;
	uint16_t *pu16CRowAddr = NULL;
	uint16_t *pu16PRowAddr = NULL;
	uint16_t *pu16NRowAddr = NULL;
	uint16_t *pu16CurRowAddr = NULL;
	uint16_t *pu16DestRowAddr = NULL;

	uint32_t aaMaskRVal[3][3];
	uint32_t aaMaskGVal[3][3];
	uint32_t aaMaskBVal[3][3];

	uint32_t u32SumR;
	uint32_t u32SumG;
	uint32_t u32SumB;

	uint16_t *pu16SrcImg = (uint16_t *)pu8SrcImg;
	uint16_t *pu16DestImg = (uint16_t *)pu8DestImg;

	pu16NRowAddr = (uint16_t *)pu8SrcImg;
	
	for(y = 0; y < u16SrcImgHeight; y ++){
		pu16PRowAddr = pu16CRowAddr;
		pu16CRowAddr = pu16NRowAddr;
		pu16NRowAddr = (pu16SrcImg + ((y + 1) * u16SrcImgStride));
		pu16DestRowAddr = pu16DestImg + (y * u16DestImgStride);
		for(x = 0; x < u16SrcImgWidth; x ++){
			for(masky = 0; masky < 3; masky ++){
				if( masky == 0)
					pu16CurRowAddr = pu16PRowAddr;
				else if(masky == 1)
					pu16CurRowAddr = pu16CRowAddr;
				else
					pu16CurRowAddr = pu16NRowAddr;

				if(((y == 0) && (masky ==0)) || ((y == u16SrcImgHeight) && (masky == 2))){
					aaMaskRVal[masky][0] = 0;
					aaMaskRVal[masky][1] = 0;
					aaMaskRVal[masky][2] = 0;
					aaMaskGVal[masky][0] = 0;
					aaMaskGVal[masky][1] = 0;
					aaMaskGVal[masky][2] = 0;
					aaMaskBVal[masky][0] = 0;
					aaMaskBVal[masky][1] = 0;
					aaMaskBVal[masky][2] = 0;
					continue;
				}

				if(x == 0){
					aaMaskRVal[masky][0] = 0;
					aaMaskGVal[masky][0] = 0;
					aaMaskBVal[masky][0] = 0;
				}
				else{
					aaMaskRVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0xF800) >> 11);
					aaMaskGVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0x7E0) >> 5);
					aaMaskBVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0x1F));
				}

				aaMaskRVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0xF800) >> 11);
				aaMaskGVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0x7E0) >> 5);
				aaMaskBVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0x1F));

				if(x == (u16SrcImgWidth -1)){
					aaMaskRVal[masky][2] = 0;
					aaMaskGVal[masky][2] = 0;
					aaMaskBVal[masky][2] = 0;
				}
				else{
					aaMaskRVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0xF800) >> 11);
					aaMaskGVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0x7E0) >> 5);
					aaMaskBVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0x1F));
				}
			}

			u32SumR = 0;
			u32SumG = 0;
			u32SumB = 0;
			for(i = 0; i < 9 ; i++){
				u32SumR = u32SumR + (aaMaskRVal[0][i] << s_au8SmoothCoeff[i]);
				u32SumG = u32SumG + (aaMaskGVal[0][i] << s_au8SmoothCoeff[i]);
				u32SumB = u32SumB + (aaMaskBVal[0][i] << s_au8SmoothCoeff[i]);
			}  

			u32SumR += (aaMaskRVal[1][1] * 160);
			u32SumR /= 256;

			u32SumG += (aaMaskGVal[1][1] * 160);
			u32SumG /= 256;

			u32SumB += (aaMaskBVal[1][1] * 160);
			u32SumB /= 256;
			
			pu16DestRowAddr[x] = (u32SumR << 11) | (u32SumG << 5) | (u32SumB);
		}
	}
}
#endif

static void
YUV422Sharp(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight,
	uint16_t u16SrcImgStride,
	uint16_t u16DestImgStride	
)
{
	int32_t x;
	int32_t y, masky;
	uint8_t *pu8YCSrcAddr = NULL;
	uint8_t *pu8YPSrcAddr = NULL;
	uint8_t *pu8YNSrcAddr = NULL;
	uint8_t *pu8YCurSrcAddr = NULL;
	uint8_t *pu8YDestAddr = NULL;


	uint32_t u32SrcWidthSize = u16SrcImgWidth * 2;
	uint32_t u32SrcHeightSize = u16SrcImgHeight;
	uint32_t u32SrcStrideSize = u16SrcImgStride * 2;
	uint32_t u32DestStrideSize = u16DestImgStride * 2;
	int32_t i32SkipByte = 0;

	uint32_t aaMaskPixVal[3][3];

	int32_t i32Sum;

	pu8YNSrcAddr = pu8SrcImg;
	
	for(y = 0; y < u32SrcHeightSize; y ++){
		pu8YPSrcAddr = pu8YCSrcAddr;
		pu8YCSrcAddr = pu8YNSrcAddr;
		pu8YNSrcAddr = pu8SrcImg + ((y + 1) * u32SrcStrideSize);
		pu8YDestAddr = pu8DestImg + (y * u32DestStrideSize);
		for(x = 0; x < u32SrcWidthSize; x ++){
			if(x & 1){
				i32SkipByte = 4;
			}
			else{
				i32SkipByte = 2;
			}
			for(masky = 0; masky < 3; masky ++){
				if( masky == 0)
					pu8YCurSrcAddr = pu8YPSrcAddr;
				else if(masky == 1)
					pu8YCurSrcAddr = pu8YCSrcAddr;
				else
					pu8YCurSrcAddr = pu8YNSrcAddr;
					
				if(((y == 0) && (masky ==0)) || ((y == (u32SrcHeightSize -1)) && (masky == 2))){
					if((x - i32SkipByte) < 0)
						aaMaskPixVal[masky][0] = pu8YCSrcAddr[x];
					else
						aaMaskPixVal[masky][0] = pu8YCSrcAddr[x - i32SkipByte];

					aaMaskPixVal[masky][1] = pu8YCSrcAddr[x];

					if((x + i32SkipByte) > u32SrcWidthSize)
						aaMaskPixVal[masky][2] = pu8YCSrcAddr[x];
					else
						aaMaskPixVal[masky][2] = pu8YCSrcAddr[x + i32SkipByte];										
					continue;
				}

				if((x - i32SkipByte) < 0)
					aaMaskPixVal[masky][0] = pu8YCSrcAddr[x];
				else
					aaMaskPixVal[masky][0] = pu8YCurSrcAddr[x - i32SkipByte];

				aaMaskPixVal[masky][1] = pu8YCurSrcAddr[x];
				
				if((x + i32SkipByte) > (u32SrcWidthSize - 1))
					aaMaskPixVal[masky][2] = pu8YCSrcAddr[x];
				else
					aaMaskPixVal[masky][2] = pu8YCurSrcAddr[x + i32SkipByte];
			}

			i32Sum = ((aaMaskPixVal[1][1] * 7) / 4) - 
						(aaMaskPixVal[0][0] >> 4) - (aaMaskPixVal[0][1] >> 3) -  (aaMaskPixVal[0][2] >> 4) - 
						(aaMaskPixVal[1][0] >> 3) - (aaMaskPixVal[1][2] >> 3) -
						(aaMaskPixVal[2][0] >> 4) - (aaMaskPixVal[2][1] >> 3) -  (aaMaskPixVal[2][2] >> 4);
	

			if(i32Sum > 255)
				i32Sum = 255;

			if(i32Sum < 0)
				i32Sum = 0;

			pu8YDestAddr[x] = i32Sum;
		}
	}
}

#if 0
static void
RGB565Sharp(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight,
	uint16_t u16SrcImgStride,
	uint16_t u16DestImgStride	
)
{
	int32_t x;
	int32_t y, masky;
	uint32_t i;
	uint16_t *pu16CRowAddr = NULL;
	uint16_t *pu16PRowAddr = NULL;
	uint16_t *pu16NRowAddr = NULL;
	uint16_t *pu16CurRowAddr = NULL;
	uint16_t *pu16DestRowAddr = NULL;

	uint32_t aaMaskRVal[3][3];
	uint32_t aaMaskGVal[3][3];
	uint32_t aaMaskBVal[3][3];

	int32_t i32SumR;
	int32_t i32SumG;
	int32_t i32SumB;

	uint16_t *pu16SrcImg = (uint16_t *)pu8SrcImg;
	uint16_t *pu16DestImg = (uint16_t *)pu8DestImg;

	pu16NRowAddr = (uint16_t *)pu8SrcImg;
	
	for(y = 0; y < u16SrcImgHeight; y ++){
		pu16PRowAddr = pu16CRowAddr;
		pu16CRowAddr = pu16NRowAddr;
		pu16NRowAddr = (pu16SrcImg + ((y + 1) * u16SrcImgStride));
		pu16DestRowAddr = pu16DestImg + (y * u16DestImgStride);
		for(x = 0; x < u16SrcImgWidth; x ++){
			for(masky = 0; masky < 3; masky ++){
				if( masky == 0)
					pu16CurRowAddr = pu16PRowAddr;
				else if(masky == 1)
					pu16CurRowAddr = pu16CRowAddr;
				else
					pu16CurRowAddr = pu16NRowAddr;

				if(((y == 0) && (masky ==0)) || ((y == u16SrcImgHeight) && (masky == 2))){
					aaMaskRVal[masky][0] = 0;
					aaMaskRVal[masky][1] = 0;
					aaMaskRVal[masky][2] = 0;
					aaMaskGVal[masky][0] = 0;
					aaMaskGVal[masky][1] = 0;
					aaMaskGVal[masky][2] = 0;
					aaMaskBVal[masky][0] = 0;
					aaMaskBVal[masky][1] = 0;
					aaMaskBVal[masky][2] = 0;
					continue;
				}

				if(x == 0){
					aaMaskRVal[masky][0] = 0;
					aaMaskGVal[masky][0] = 0;
					aaMaskBVal[masky][0] = 0;
				}
				else{
					aaMaskRVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0xF800) >> 11);
					aaMaskGVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0x7E0) >> 5);
					aaMaskBVal[masky][0] = (uint32_t)((pu16CurRowAddr[x - 1] & 0x1F));
				}

				aaMaskRVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0xF800) >> 11);
				aaMaskGVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0x7E0) >> 5);
				aaMaskBVal[masky][1] = (uint32_t)((pu16CurRowAddr[x] & 0x1F));

				if(x == (u16SrcImgWidth -1)){
					aaMaskRVal[masky][2] = 0;
					aaMaskGVal[masky][2] = 0;
					aaMaskBVal[masky][2] = 0;
				}
				else{
					aaMaskRVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0xF800) >> 11);
					aaMaskGVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0x7E0) >> 5);
					aaMaskBVal[masky][2] = (uint32_t)((pu16CurRowAddr[x + 1] & 0x1F));
				}
			}

			i32SumR = (aaMaskRVal[1][1] * 7) - 
						(aaMaskRVal[0][0] >> 1) - (aaMaskRVal[0][1]) -  (aaMaskRVal[0][2] >> 1) - 
						(aaMaskRVal[1][0]) - (aaMaskRVal[1][2]) -
						(aaMaskRVal[2][0] >> 1) - (aaMaskRVal[2][1]) -  (aaMaskRVal[2][2] >> 1);

			i32SumG = (aaMaskGVal[1][1] * 7) - 
						(aaMaskGVal[0][0] >> 1) - (aaMaskGVal[0][1]) -  (aaMaskGVal[0][2] >> 1) - 
						(aaMaskGVal[1][0]) - (aaMaskGVal[1][2]) -
						(aaMaskGVal[2][0] >> 1) - (aaMaskGVal[2][1]) -  (aaMaskGVal[2][2] >> 1);

			i32SumB = (aaMaskBVal[1][1] * 7) - 
						(aaMaskBVal[0][0] >> 1) - (aaMaskBVal[0][1]) -  (aaMaskBVal[0][2] >> 1) - 
						(aaMaskBVal[1][0]) - (aaMaskBVal[1][2]) -
						(aaMaskBVal[2][0] >> 1) - (aaMaskBVal[2][1]) -  (aaMaskBVal[2][2] >> 1);


			pu16DestRowAddr[x] = (i32SumR << 11) | (i32SumG << 5) | (i32SumB);
		}
	}
}
#endif

static void 
YUV420ToYUV422_SW(
	uint8_t *pu8SrcImg,
	uint8_t *pu8DestImg,
	uint16_t u16SrcImgWidth,
	uint16_t u16SrcImgHeight
)
{
	uint16_t x,y;
	uint8_t u8Y0;
	uint8_t u8Y1;
	uint8_t u8U0;
	uint8_t u8Y2;
	uint8_t u8Y3;
	uint8_t u8V0;
	uint32_t u32SrcY0Addr;
	uint32_t u32SrcY2Addr;
	uint16_t u16TempX;

	uint32_t u32DestY0Addr;
	uint32_t u32DestY2Addr;

	for( y = 0; y < u16SrcImgHeight ; y += 2){
		u32SrcY0Addr = y * ((u16SrcImgWidth / 2) * 3);
		u32SrcY2Addr = (y + 1) * ((u16SrcImgWidth / 2) * 3);
		u32DestY0Addr = y * u16SrcImgWidth * 2;
		u32DestY2Addr = (y + 1) * u16SrcImgWidth * 2;
		for( x = 0; x < u16SrcImgWidth ; x += 2){
			u16TempX = (x / 2) * 3;
			u8Y0 = pu8SrcImg[u32SrcY0Addr + u16TempX];
			u8Y1 = pu8SrcImg[u32SrcY0Addr + u16TempX + 1];
			u8U0 = pu8SrcImg[u32SrcY0Addr + u16TempX + 2];

			u8Y2 = pu8SrcImg[u32SrcY2Addr + u16TempX];
			u8Y3 = pu8SrcImg[u32SrcY2Addr + u16TempX + 1];
			u8V0 = pu8SrcImg[u32SrcY2Addr + u16TempX + 2];

			u16TempX = x * 2;
			pu8DestImg[u32DestY0Addr + u16TempX] = u8Y0;
			pu8DestImg[u32DestY0Addr + u16TempX + 1] = u8U0;
			pu8DestImg[u32DestY0Addr + u16TempX + 2] = u8Y1;
			pu8DestImg[u32DestY0Addr + u16TempX + 3] = u8V0;

			pu8DestImg[u32DestY2Addr + u16TempX] = u8Y2;
			pu8DestImg[u32DestY2Addr + u16TempX + 1] = u8U0;
			pu8DestImg[u32DestY2Addr + u16TempX + 2] = u8Y3;
			pu8DestImg[u32DestY2Addr + u16TempX + 3] = u8V0;
		}
	}
}


static ERRCODE
VPEColorMapping(
	E_IMGPROC_COLOR_FORMAT eOrigSrcFmt,
	E_IMGPROC_COLOR_FORMAT eOrigDestFmt,
	E_VPE_SRC_FMT *peSrcMappingFmt,
	E_VPE_DST_FMT *peDestMappingFmt
)
{
	switch(eOrigSrcFmt){
		case eIMGPROC_RGB888:
			*peSrcMappingFmt = VPE_SRC_PACKET_RGB888;
			break;
		case eIMGPROC_RGB555:
			*peSrcMappingFmt = VPE_SRC_PACKET_RGB555;
			break;
		case eIMGPROC_RGB565:
			*peSrcMappingFmt = VPE_SRC_PACKET_RGB565;
			break;
		case eIMGPROC_YUV422:
			*peSrcMappingFmt = VPE_SRC_PACKET_YUV422;
			break;
		case eIMGPROC_YUV422P:
			*peSrcMappingFmt = VPE_SRC_PLANAR_YUV422;
			break;
		case eIMGPROC_YUV420P:
			*peSrcMappingFmt = VPE_SRC_PLANAR_YUV420;
			break;			
		default:
			return ERR_IMGPROC_COLOR_FORMAT;			
	}

	switch(eOrigDestFmt){
		case eIMGPROC_RGB888:
			*peDestMappingFmt = VPE_DST_PACKET_RGB888;
			break;
		case eIMGPROC_RGB555:
			*peDestMappingFmt = VPE_DST_PACKET_RGB555;
			break;
		case eIMGPROC_RGB565:
			*peDestMappingFmt = VPE_DST_PACKET_RGB565;
			break;
		case eIMGPROC_YUV422:
			*peDestMappingFmt = VPE_DST_PACKET_YUV422;
			break;
		default:
			return ERR_IMGPROC_COLOR_FORMAT;
	}
	return ERR_IMGPROC_SUCCESS;
}



////////////////////////////////////////////////////////////////////////
//	API implement
////////////////////////////////////////////////////////////////////////

ERRCODE
ImgProc_Init(void)
{
	return VPE_Init();
}

ERRCODE
ImgProc_CST(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_CST_ATTR *psCSTAttr
)
{
	ERRCODE err;

	if(eOPmode == eIMGPROC_OP_HW){
		S_VPE_TRANS_ATTR sVPETrans;
		S_VPE_TRANS_PARAM sVPETransParam;

		err = VPEColorMapping(psCSTAttr->eSrcFmt, psCSTAttr->eDestFmt, &sVPETransParam.eSrcFmt, &sVPETransParam.eDestFmt);
		if(err != ERR_IMGPROC_SUCCESS)
			return err;

		sVPETransParam.eVPEOP = VPE_OP_NORMAL;
		sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
		sVPETransParam.u32SrcImgWidth = psCSTAttr->u32ImgWidth;
		sVPETransParam.u32SrcImgHeight = psCSTAttr->u32ImgHeight;
		sVPETransParam.u32DestImgWidth = psCSTAttr->u32ImgWidth;
		sVPETransParam.u32DestImgHeight = psCSTAttr->u32ImgHeight; 

		sVPETransParam.u32SrcRightOffset = psCSTAttr->u32SrcImgStride - psCSTAttr->u32ImgWidth; 
		sVPETransParam.u32SrcLeftOffset = 0; 
		sVPETransParam.u32SrcTopOffset = 0; 
		sVPETransParam.u32SrcBottomOffset = 0; 

		sVPETransParam.u32DestRightOffset = psCSTAttr->u32DestImgStride - psCSTAttr->u32ImgWidth; 
		sVPETransParam.u32DestLeftOffset = 0; 

		sVPETrans.psTransParam = &sVPETransParam;
		sVPETrans.pu8SrcBuf = psCSTAttr->pu8SrcBuf;
		sVPETrans.pu8DestBuf = psCSTAttr->pu8DestBuf;
		sVPETrans.u32SrcBufPhyAdr = (uint32_t)psCSTAttr->pu8SrcBuf;
		sVPETrans.u32DestBufPhyAdr = (uint32_t)psCSTAttr->pu8DestBuf;
		err = VPE_Trans(&sVPETrans);
		if(err != ERR_VPE_SUCCESS)
			return err;
		return VPE_WaitTransDone(); 
	}
	else{
		if((psCSTAttr->pu8SrcBuf == NULL) || (psCSTAttr->pu8DestBuf == NULL))
			return ERR_IMGPROC_BAD_ATTR;

		if((psCSTAttr->eSrcFmt == eIMGPROC_YUV422) && (psCSTAttr->eDestFmt == eIMGPROC_YUV422P)){
			//YUV422 to YUV422P
			YUV422PacketToPlanar_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV422P) && (psCSTAttr->eDestFmt == eIMGPROC_YUV422)){
			//YUV422P to YUV422
			YUV422PlanarToPacket_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV422) && (psCSTAttr->eDestFmt == eIMGPROC_YUV420)){
			//YUV422 to YUV420
			YUV422ToYUV420_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV420) && (psCSTAttr->eDestFmt == eIMGPROC_YUV422)){
			//YUV420 to YUV422
			YUV420ToYUV422_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV420P) && (psCSTAttr->eDestFmt == eIMGPROC_YUV422)){
			//YUV420P to YUV422
			YUV420PlanarToYUV422_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV422) && (psCSTAttr->eDestFmt == eIMGPROC_YUV420P)){
			//YUV422 to YUV420P
			YUV422ToYUV420Planar_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV420P_MB) && (psCSTAttr->eDestFmt == eIMGPROC_YUV422)){
			//YUV420_MB to YUV422
			YUV420MBToYUV422_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else if((psCSTAttr->eSrcFmt == eIMGPROC_YUV422) && (psCSTAttr->eDestFmt == eIMGPROC_YUV420P_MB)){
			//YUV422 to YUV420_MB
			YUV422ToYUV420MB_SW(psCSTAttr->pu8SrcBuf, psCSTAttr->pu8DestBuf, psCSTAttr->u32ImgWidth, psCSTAttr->u32ImgHeight);
		}
		else {
			return ERR_IMGPROC_NOT_SUPP;
		}
	}
	return ERR_IMGPROC_SUCCESS;
}

ERRCODE
ImgProc_Mirror(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_MIRROR_ATTR *psMirrorAttr
)
{

	if(eOPmode == eIMGPROC_OP_SW){
		S_IMGPROC_MIRROR_PARAM *psMirrorParam = psMirrorAttr->psMirrorParam; 
//		if(psMirrorParam->eMirrorType != eIMGPROC_H_MIRROR)
//			return ERR_IMGPROC_NOT_SUPP;
		// H mirror
		if((psMirrorParam->eMirrorType == eIMGPROC_H_MIRROR) || (psMirrorParam->eMirrorType == eIMGPROC_H_V_MIRROR)){
			if((psMirrorParam->eSrcFmt == eIMGPROC_RGB565) && (psMirrorParam->eDestFmt == eIMGPROC_RGB565)){
				HMirrorImgRGB565_SW(psMirrorAttr->pu8SrcBuf, psMirrorAttr->pu8DestBuf, 
							psMirrorParam->u32ImgWidth, psMirrorParam->u32ImgHeight,
							psMirrorParam->u32SrcImgStride, psMirrorParam->u32DestImgStride);
			}
			else if((psMirrorParam->eSrcFmt == eIMGPROC_YUV422) && (psMirrorParam->eDestFmt == eIMGPROC_YUV422)){
				HMirrorImgYUV422_SW(psMirrorAttr->pu8SrcBuf, psMirrorAttr->pu8DestBuf, 
							psMirrorParam->u32ImgWidth, psMirrorParam->u32ImgHeight,
							psMirrorParam->u32SrcImgStride, psMirrorParam->u32DestImgStride);
			}
			else{
				return ERR_IMGPROC_NOT_SUPP;
			}
		}

		if((psMirrorParam->eMirrorType == eIMGPROC_V_MIRROR) || (psMirrorParam->eMirrorType == eIMGPROC_H_V_MIRROR)){
			if((psMirrorParam->eSrcFmt == eIMGPROC_RGB565) && (psMirrorParam->eDestFmt == eIMGPROC_RGB565)){
				VMirrorImg_SW(psMirrorAttr->pu8SrcBuf, psMirrorAttr->pu8DestBuf, 
							psMirrorParam->u32ImgWidth, psMirrorParam->u32ImgHeight,
							psMirrorParam->u32SrcImgStride, psMirrorParam->u32DestImgStride);
			}
			else if((psMirrorParam->eSrcFmt == eIMGPROC_YUV422) && (psMirrorParam->eDestFmt == eIMGPROC_YUV422)){
				VMirrorImg_SW(psMirrorAttr->pu8SrcBuf, psMirrorAttr->pu8DestBuf, 
							psMirrorParam->u32ImgWidth, psMirrorParam->u32ImgHeight,
							psMirrorParam->u32SrcImgStride, psMirrorParam->u32DestImgStride);
			}
			else{
				return ERR_IMGPROC_NOT_SUPP;
			}		
		}
	}
	else{
		ERRCODE err;
		S_VPE_TRANS_ATTR sVPETrans;
		S_VPE_TRANS_PARAM sVPETransParam;
		S_IMGPROC_MIRROR_PARAM *psMirrorParam = psMirrorAttr->psMirrorParam; 

		err = VPEColorMapping(psMirrorParam->eSrcFmt, psMirrorParam->eDestFmt, &sVPETransParam.eSrcFmt, &sVPETransParam.eDestFmt);
		if(err != ERR_IMGPROC_SUCCESS)
			return err;
		
		if(psMirrorParam->eMirrorType == eIMGPROC_H_V_MIRROR)
			sVPETransParam.eVPEOP = VPE_OP_UPSIDEDOWN;			//VPE_OP_ROTATE_180
		else if(psMirrorParam->eMirrorType == eIMGPROC_H_MIRROR)
			sVPETransParam.eVPEOP = VPE_OP_FLOP;				//VPE_OP_MIRROR_H;
		else
			sVPETransParam.eVPEOP = VPE_OP_FLIP;					//VPE_OP_MIRROR_V;

		sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
		sVPETransParam.u32SrcImgWidth = psMirrorParam->u32ImgWidth;
		sVPETransParam.u32SrcImgHeight = psMirrorParam->u32ImgHeight;
		sVPETransParam.u32DestImgWidth = psMirrorParam->u32ImgWidth;
		sVPETransParam.u32DestImgHeight = psMirrorParam->u32ImgHeight;

		sVPETransParam.u32SrcRightOffset = psMirrorParam->u32SrcImgStride - psMirrorParam->u32ImgWidth; 
		sVPETransParam.u32SrcLeftOffset = 0; 
		sVPETransParam.u32SrcTopOffset = 0; 
		sVPETransParam.u32SrcBottomOffset = 0; 

		sVPETransParam.u32DestRightOffset = psMirrorParam->u32DestImgStride - psMirrorParam->u32ImgWidth; 
		sVPETransParam.u32DestLeftOffset = 0; 

		sVPETrans.psTransParam = &sVPETransParam;
		sVPETrans.pu8SrcBuf = psMirrorAttr->pu8SrcBuf;
		sVPETrans.pu8DestBuf = psMirrorAttr->pu8DestBuf;
		sVPETrans.u32SrcBufPhyAdr = (uint32_t)psMirrorAttr->pu8SrcBuf;
		sVPETrans.u32DestBufPhyAdr = (uint32_t)psMirrorAttr->pu8DestBuf; 
		err = VPE_Trans(&sVPETrans);
		if(err != ERR_VPE_SUCCESS)
			return err;

		return VPE_WaitTransDone();
	}
	return ERR_IMGPROC_SUCCESS;
}


ERRCODE
ImgProc_Scale(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_SCALE_ATTR *psScaleUpAttr
)
{
	
	if(eOPmode == eIMGPROC_OP_SW){
		if((psScaleUpAttr->pu8SrcBuf == NULL) || (psScaleUpAttr->pu8DestBuf == NULL))
			return  ERR_IMGPROC_BAD_ATTR;
		Scale_SW(psScaleUpAttr->pu8SrcBuf, psScaleUpAttr->pu8DestBuf, psScaleUpAttr->psScaleUpParam);
	}
	else{
		ERRCODE err;
		S_VPE_TRANS_ATTR sVPETrans;
		S_VPE_TRANS_PARAM sVPETransParam;
		S_IMGPROC_SCALE_PARAM *psScaleUpParam;
		psScaleUpParam = psScaleUpAttr->psScaleUpParam;

		err = VPEColorMapping(psScaleUpParam->eSrcFmt, psScaleUpParam->eDestFmt, &sVPETransParam.eSrcFmt, &sVPETransParam.eDestFmt);
		if(err != ERR_IMGPROC_SUCCESS)
			return err;

		sVPETransParam.eVPEOP = VPE_OP_NORMAL;
		sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
		sVPETransParam.u32SrcImgWidth = psScaleUpParam->u32SrcImgWidth;
		sVPETransParam.u32SrcImgHeight = psScaleUpParam->u32SrcImgHeight;
		sVPETransParam.u32DestImgWidth = psScaleUpParam->u32ScaleImgWidth;
		sVPETransParam.u32DestImgHeight = psScaleUpParam->u32ScaleImgHeight;

		sVPETransParam.u32SrcLeftOffset = psScaleUpParam->u32CropWinStartX; 
		sVPETransParam.u32SrcRightOffset = psScaleUpParam->u32SrcImgWidth - psScaleUpParam->u32CropWinStartX - psScaleUpParam->u32CropWinWidth; 
		sVPETransParam.u32SrcTopOffset = psScaleUpParam->u32CropWinStartY; 
		sVPETransParam.u32SrcBottomOffset = psScaleUpParam->u32SrcImgHeight - psScaleUpParam->u32CropWinStartY - psScaleUpParam->u32CropWinHeight; 


		sVPETransParam.u32DestRightOffset = 0; 
		sVPETransParam.u32DestLeftOffset = 0; 

		sVPETrans.psTransParam = &sVPETransParam;
		sVPETrans.pu8SrcBuf = psScaleUpAttr->pu8SrcBuf;
		sVPETrans.pu8DestBuf = psScaleUpAttr->pu8DestBuf;
		sVPETrans.u32SrcBufPhyAdr = (uint32_t)psScaleUpAttr->pu8SrcBuf;
		sVPETrans.u32DestBufPhyAdr = (uint32_t)psScaleUpAttr->pu8DestBuf; 
		err = VPE_Trans(&sVPETrans);
		if(err != ERR_VPE_SUCCESS)
			return err;

		return VPE_WaitTransDone();
	}
	return ERR_IMGPROC_SUCCESS;
}

ERRCODE
ImgProc_Rotate(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_ROTATE_ATTR *psRotateAttr
)
{
	if(eOPmode == eIMGPROC_OP_SW){
		return ERR_IMGPROC_NOT_SUPP;
	}

	ERRCODE err;
	S_VPE_TRANS_ATTR sVPETrans;
	S_VPE_TRANS_PARAM sVPETransParam;
	S_IMGPROC_ROTATE_PARAM *psRotateParam = psRotateAttr->psRotateParam;

	if(psRotateParam->eRotate == eIMGPROC_ROTATE90_RIGHT){
		sVPETransParam.eVPEOP = VPE_OP_RIGHT;	//VPE_OP_ROTATE_R90;
	}
	else if(psRotateParam->eRotate == eIMGPROC_ROTATE90_LEFT){
		sVPETransParam.eVPEOP = VPE_OP_LEFT;	//VPE_OP_ROTATE_L90;
	}
	else{
		return ERR_IMGPROC_BAD_ATTR;
	}

	err = VPEColorMapping(psRotateParam->eSrcFmt, psRotateParam->eDestFmt, &sVPETransParam.eSrcFmt, &sVPETransParam.eDestFmt);
	if(err != ERR_IMGPROC_SUCCESS)
		return err;

	sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
	
	sVPETransParam.u32SrcImgWidth = psRotateParam->u32SrcImgWidth;
	sVPETransParam.u32SrcImgHeight = psRotateParam->u32SrcImgHeight;
	sVPETransParam.u32DestImgWidth = psRotateParam->u32DestImgWidth;
	sVPETransParam.u32DestImgHeight = psRotateParam->u32DestImgHeight;
	sVPETransParam.u32SrcRightOffset = psRotateParam->u32SrcRightOffset; 
	sVPETransParam.u32SrcLeftOffset = psRotateParam->u32SrcLeftOffset; 
	sVPETransParam.u32SrcTopOffset = 0; 
	sVPETransParam.u32SrcBottomOffset = 0; 

	sVPETransParam.u32DestRightOffset = psRotateParam->u32DestRightOffset; 
	sVPETransParam.u32DestLeftOffset = psRotateParam->u32DestLeftOffset; 

	sVPETrans.psTransParam = &sVPETransParam;
	sVPETrans.pu8SrcBuf = psRotateAttr->pu8SrcBuf;
	sVPETrans.pu8DestBuf = psRotateAttr->pu8DestBuf;
	sVPETrans.u32SrcBufPhyAdr = (uint32_t)psRotateAttr->pu8SrcBuf;
	sVPETrans.u32DestBufPhyAdr = (uint32_t)psRotateAttr->pu8DestBuf; 

	err = VPE_Trans(&sVPETrans);
	if(err != ERR_VPE_SUCCESS)
		return err;

	return VPE_WaitTransDone();
}

ERRCODE
ImgProc_Smooth(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_SMOOTH_ATTR *psSmoothAttr
)
{
	if(eOPmode == eIMGPROC_OP_HW){
		return ERR_IMGPROC_NOT_SUPP;
	}

	if((psSmoothAttr->eSrcFmt != eIMGPROC_YUV422) || (psSmoothAttr->eDestFmt != eIMGPROC_YUV422)){
		return ERR_IMGPROC_NOT_SUPP;
	}

//	if((psSmoothAttr->eSrcFmt != eIMGPROC_RGB565) || (psSmoothAttr->eDestFmt != eIMGPROC_RGB565)){
//		return ERR_IMGPROC_NOT_SUPP;
//	}


	YUV422Smooth(psSmoothAttr->pu8SrcBuf, psSmoothAttr->pu8DestBuf, psSmoothAttr->u32ImgWidth, psSmoothAttr->u32ImgHeight, psSmoothAttr->u32SrcImgStride, psSmoothAttr->u32DestImgStride);
//	RGB565Smooth(psSmoothAttr->pu8SrcBuf, psSmoothAttr->pu8DestBuf, psSmoothAttr->u32ImgWidth, psSmoothAttr->u32ImgHeight, psSmoothAttr->u32SrcImgStride, psSmoothAttr->u32DestImgStride);

	return ERR_IMGPROC_SUCCESS;
}


ERRCODE
ImgProc_Sharp(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_SHARP_ATTR *psSharpAttr
)
{
	if(eOPmode == eIMGPROC_OP_HW){
		return ERR_IMGPROC_NOT_SUPP;
	}

	if((psSharpAttr->eSrcFmt != eIMGPROC_YUV422) || (psSharpAttr->eDestFmt != eIMGPROC_YUV422)){
		return ERR_IMGPROC_NOT_SUPP;
	}

//	if((psSharpAttr->eSrcFmt != eIMGPROC_RGB565) || (psSharpAttr->eDestFmt != eIMGPROC_RGB565)){
//		return ERR_IMGPROC_NOT_SUPP;
//	}


	YUV422Sharp(psSharpAttr->pu8SrcBuf, psSharpAttr->pu8DestBuf, psSharpAttr->u32ImgWidth, psSharpAttr->u32ImgHeight, psSharpAttr->u32SrcImgStride, psSharpAttr->u32DestImgStride);
//	RGB565Sharp(psSharpAttr->pu8SrcBuf, psSharpAttr->pu8DestBuf, psSharpAttr->u32ImgWidth, psSharpAttr->u32ImgHeight, psSharpAttr->u32SrcImgStride, psSharpAttr->u32DestImgStride);

	return ERR_IMGPROC_SUCCESS;
}


ERRCODE
ImgProc_MultiOP(
	E_IMGPROC_OP_MODE eOPmode,
	S_IMGPROC_MULTI_OP_ATTR *psOPAttr
)
{
	if(eOPmode == eIMGPROC_OP_SW){
		return ERR_IMGPROC_NOT_SUPP;
	}

	ERRCODE err;
	S_VPE_TRANS_ATTR sVPETrans;
	S_VPE_TRANS_PARAM sVPETransParam;
	S_IMGPROC_MULTI_OP_PARAM *psMultiOPParam;
	psMultiOPParam = psOPAttr->psMultiOPParam;

	err = VPEColorMapping(psMultiOPParam->eSrcFmt, psMultiOPParam->eDestFmt, &sVPETransParam.eSrcFmt, &sVPETransParam.eDestFmt);
	if(err != ERR_IMGPROC_SUCCESS)
		return err;

	if(psMultiOPParam->eMultiOP == eIMGPROC_OP_SCALE_MIRRORHV)
		sVPETransParam.eVPEOP = VPE_OP_UPSIDEDOWN;		//VPE_OP_ROTATE_180;
	else if(psMultiOPParam->eMultiOP == eIMGPROC_OP_SCALE_MIRRORH)
		sVPETransParam.eVPEOP = VPE_OP_FLOP;				//VPE_OP_MIRROR_H;
	else if(psMultiOPParam->eMultiOP == eIMGPROC_OP_SCALE_MIRRORV)
		sVPETransParam.eVPEOP = VPE_OP_FLIP;				//VPE_OP_MIRROR_V;
	else
		sVPETransParam.eVPEOP = VPE_OP_NORMAL;

	sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
	sVPETransParam.u32SrcImgWidth = psMultiOPParam->u32CropWinWidth;
	sVPETransParam.u32SrcImgHeight = psMultiOPParam->u32CropWinHeight;

	sVPETransParam.u32SrcLeftOffset = psMultiOPParam->u32CropWinStartX; 
	sVPETransParam.u32SrcRightOffset = psMultiOPParam->u32SrcImgStrideW - psMultiOPParam->u32CropWinStartX - psMultiOPParam->u32CropWinWidth; 
	sVPETransParam.u32SrcTopOffset = psMultiOPParam->u32CropWinStartY; 
	sVPETransParam.u32SrcBottomOffset = psMultiOPParam->u32SrcImgStrideH - psMultiOPParam->u32CropWinStartY - psMultiOPParam->u32CropWinHeight; 

	sVPETransParam.u32DestImgWidth = psMultiOPParam->u32DestImgWidth;
	sVPETransParam.u32DestImgHeight = psMultiOPParam->u32DestImgHeight;
	sVPETransParam.u32DestLeftOffset = 0; 
	sVPETransParam.u32DestRightOffset = psMultiOPParam->u32DestImgStrideW - psMultiOPParam->u32DestImgWidth; 

//	sysprintf("sVPETransParam.u32SrcImgWidth %d \n" ,sVPETransParam.u32SrcImgWidth);
//	sysprintf("sVPETransParam.u32SrcImgHeight %d \n" ,sVPETransParam.u32SrcImgHeight);
//	sysprintf("sVPETransParam.u32SrcLeftOffset %d \n" ,sVPETransParam.u32SrcLeftOffset);
//	sysprintf("sVPETransParam.u32SrcRightOffset %d \n" ,sVPETransParam.u32SrcRightOffset);
//	sysprintf("sVPETransParam.u32SrcTopOffset %d \n" ,sVPETransParam.u32SrcTopOffset);
//	sysprintf("sVPETransParam.u32SrcBottomOffset %d \n" ,sVPETransParam.u32SrcBottomOffset);

//	sysprintf("sVPETransParam.u32DestImgWidth %d \n" ,sVPETransParam.u32DestImgWidth);
//	sysprintf("sVPETransParam.u32DestImgHeight %d \n" ,sVPETransParam.u32DestImgHeight);
//	sysprintf("sVPETransParam.u32DestLeftOffset %d \n" ,sVPETransParam.u32DestLeftOffset);
//	sysprintf("sVPETransParam.u32DestRightOffset %d \n" ,sVPETransParam.u32DestRightOffset);
//	sysprintf("sVPETransParam.u32SrcBufPhyAdr %x \n" ,psOPAttr->u32SrcBufPhyAdr);
//	sysprintf("sVPETransParam.u32DestBufPhyAdr %x \n" ,psOPAttr->u32DestBufPhyAdr);

//	sysprintf("============================================= \n");

	sVPETrans.psTransParam = &sVPETransParam;
	sVPETrans.pu8SrcBuf = psOPAttr->pu8SrcBuf;
	sVPETrans.pu8DestBuf = psOPAttr->pu8DestBuf;
	sVPETrans.u32SrcBufPhyAdr = (uint32_t)psOPAttr->pu8SrcBuf;
	sVPETrans.u32DestBufPhyAdr = (uint32_t)psOPAttr->pu8DestBuf; 
	err = VPE_Trans(&sVPETrans);
	if(err != ERR_VPE_SUCCESS)
		return err;

	return VPE_WaitTransDone();

}




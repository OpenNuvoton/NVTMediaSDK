//      Copyright (c) 2016 Nuvoton Technology Corp. All rights reserved.
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
#include <inttypes.h>


#include "wblib.h"
#include "N9H26_VideoIn.h"
#include "N9H26_I2C.h"
//#include "TouchADC/Lib/w55fa92_ts_adc.h"
#include "N9H26_PWM.h"
#include "VinConfig.h"
//#include "DrvI2C.h"

#if VIN_CONFIG_SENSOR_NT99141 == 1

#include "FreeRTOS.h"
#include "task.h"

#include "NVTMedia_Log.h"
#include "SensorIf.h"

#include "NVTMedia.h"

#if defined (__GNUC__) && !(__CC_ARM)
#else
    #define __USE_HW_I2C__
#endif

#if !defined (__USE_HW_I2C__)
    #include "DrvI2C.h"
#endif

#define NT99141_DEV_ID  0x54

#define REG_TABLE_INIT  0
#define REG_TABLE_VGA   1   //640X480
#define REG_TABLE_SVGA  2   //800X600
#define REG_TABLE_HD720 3   //1280X720

struct NT_RegValue
{
    uint16_t    u16RegAddr;     //Register Address
    uint8_t u8Value;            //Register Data
};

#define _REG_TABLE_SIZE(nTableName) sizeof(nTableName)/sizeof(struct NT_RegValue)

struct NT_RegTable
{
    struct NT_RegValue *psRegTable;
    uint16_t u16TableSize;
};

typedef enum
{
    eSNR_FLICKER_50,
    eSNR_FLICKER_60,
    eSNR_FLICKER_CNT,
} E_SNR_FLICKER;

typedef enum
{
    eSNR_DIMENSION_VGA,
    eSNR_DIMENSION_SVGA,
    eSNR_DIMENSION_HD,
    eSNR_DIMENSION_CNT,
} E_SNR_DIMENSION;

typedef enum
{
    eSNR_CLOCK_16,
    eSNR_CLOCK_20,
    eSNR_CLOCK_24,
    eSNR_CLOCK_CNT,
} E_SNR_CLOCK;


static struct NT_RegValue s_sNT99141_Init[] =
{
#include "NT99141/NT99141_Init.dat"
};

#if !defined(VIN_CONFIG_SENSOR_PCLK_48MHZ) && !defined(VIN_CONFIG_SENSOR_PCLK_60MHZ)&&!defined(VIN_CONFIG_SENSOR_PCLK_74MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
    0x0, 0x0
};
static struct NT_RegValue s_sNT99141_SVGA[] =
{
    0x0, 0x0
};
static struct NT_RegValue s_sNT99141_VGA[] =
{
    0x0, 0x0
};
#endif

#if (VIN_CONFIG_SENSOR_PCLK == VIN_CONFIG_SENSOR_PCLK_48MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_48MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_48MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_48MHz.dat"
};

static uint32_t s_u32SnrClk = 16000;    //frame rate: 20
static uint32_t s_u32InputFrameRate = 20;
//static uint32_t s_u32SnrClk = 12000;  //frame rate: 15
//static uint32_t s_u32InputFrameRate = 15;
//static uint32_t s_u32SnrClk = 10000;  //frame rate: 12
//static uint32_t s_u32InputFrameRate = 12;
#endif

#if  (VIN_CONFIG_SENSOR_PCLK == VIN_CONFIG_SENSOR_PCLK_60MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_60MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_60MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_60MHz.dat"
};

static uint32_t s_u32SnrClk = 20000;
static uint32_t s_u32InputFrameRate = 24;

#endif

#if (VIN_CONFIG_SENSOR_PCLK == VIN_CONFIG_SENSOR_PCLK_74MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_74MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_74MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_74MHz.dat"
};

static uint32_t s_u32SnrClk = 24000;
static uint32_t s_u32InputFrameRate = 30;

#endif

static uint32_t s_u32DimTableIdx = REG_TABLE_HD720;

static struct NT_RegTable s_NT99141SnrTable[] =
{

    {s_sNT99141_Init, _REG_TABLE_SIZE(s_sNT99141_Init)},
    {s_sNT99141_VGA, _REG_TABLE_SIZE(s_sNT99141_VGA)},
    {s_sNT99141_SVGA, _REG_TABLE_SIZE(s_sNT99141_SVGA)},
    {s_sNT99141_HD720, _REG_TABLE_SIZE(s_sNT99141_HD720)},

    {0, 0}
};

typedef struct
{
    uint16_t u16ImageWidth;
    uint16_t u16ImageHeight;
    int8_t i8ResolIdx;
} S_NTSuppResol;

#define NT_RESOL_SUPP_CNT  4

static S_NTSuppResol s_asNTSuppResolTable[NT_RESOL_SUPP_CNT] =
{
    {0, 0, REG_TABLE_INIT},
    {640, 480, REG_TABLE_VGA},
    {800, 600, REG_TABLE_SVGA},
    {1280, 720, REG_TABLE_HD720}
};

extern void VideoIn_InterruptHandler(void);
extern void VideoIn_Port1InterruptHandler(void);

static void SnrReset(void)
{
#ifdef VIN_CONFIG_SENSOR_RESET_GPIO_PIN
    outp32(VIN_CONFIG_SENSOR_RESET_MFP_PORT, inp32(VIN_CONFIG_SENSOR_RESET_MFP_PORT) & (~ VIN_CONFIG_SENSOR_RESET_MFP_PIN));

    gpio_setportval(VIN_CONFIG_SENSOR_RESET_GPIO_PORT, VIN_CONFIG_SENSOR_RESET_GPIO_PIN, VIN_CONFIG_SENSOR_RESET_GPIO_PIN); //set high default
    gpio_setportpull(VIN_CONFIG_SENSOR_RESET_GPIO_PORT, VIN_CONFIG_SENSOR_RESET_GPIO_PIN, VIN_CONFIG_SENSOR_RESET_GPIO_PIN);    //pull-up
    gpio_setportdir(VIN_CONFIG_SENSOR_RESET_GPIO_PORT, VIN_CONFIG_SENSOR_RESET_GPIO_PIN, VIN_CONFIG_SENSOR_RESET_GPIO_PIN); //output mode
    vTaskDelay(3 / portTICK_RATE_MS);
    gpio_setportval(VIN_CONFIG_SENSOR_RESET_GPIO_PORT, VIN_CONFIG_SENSOR_RESET_GPIO_PIN, ~VIN_CONFIG_SENSOR_RESET_GPIO_PIN);    //set low
    vTaskDelay(3 / portTICK_RATE_MS);
    gpio_setportval(VIN_CONFIG_SENSOR_RESET_GPIO_PORT, VIN_CONFIG_SENSOR_RESET_GPIO_PIN, VIN_CONFIG_SENSOR_RESET_GPIO_PIN); //Set high

#endif
}

static void SnrPowerDown(BOOL bIsEnable)
{

#ifdef VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN

    outp32(VIN_CONFIG_SENSOR_POWERDOWN_MFP_PORT, inp32(VIN_CONFIG_SENSOR_POWERDOWN_MFP_PORT) & (~ VIN_CONFIG_SENSOR_POWERDOWN_MFP_PIN));

    gpio_setportval(VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN); //set high default
    gpio_setportpull(VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN);    //pull-up
    gpio_setportdir(VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN); //output mode

    if (bIsEnable)
        gpio_setportval(VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN); //GPIOB 3 set high
    else
        gpio_setportval(VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT, VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN, ~VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN);        //GPIOB 3 set low

#endif
}

static void Delay(UINT32 nCount)
{
    volatile UINT32 i;
    for (; nCount != 0; nCount--)
        for (i = 0; i < 5; i++);
}


static void OpenSensorI2C(void)
{
#if defined __USE_HW_I2C__
    int i32Ret;

    i2cInit();
    i32Ret = -1;
    // Open HW I2C until timeout
    while (i32Ret < 0)
    {

        i32Ret = i2cOpen();

        if (i32Ret < 0)
        {
            NMLOG_WARNING("I2C bus busy, waiting...\n");
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }

    i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);
#else

    outp32(VIN_CONFIG_SENSOR_I2C_SCK_MFP_PORT, inp32(VIN_CONFIG_SENSOR_I2C_SCK_MFP_PORT) & (~VIN_CONFIG_SENSOR_I2C_SCK_MFP_PIN));
    outp32(VIN_CONFIG_SENSOR_I2C_SDA_MFP_PORT, inp32(VIN_CONFIG_SENSOR_I2C_SDA_MFP_PORT) & (~VIN_CONFIG_SENSOR_I2C_SDA_MFP_PIN));
    DrvI2C_Open(VIN_CONFIG_SENSOR_I2C_SCK_GPIO_PORT,
                VIN_CONFIG_SENSOR_I2C_SCK_GPIO_PIN,
                VIN_CONFIG_SENSOR_I2C_SDA_GPIO_PORT,
                VIN_CONFIG_SENSOR_I2C_SDA_GPIO_PIN,
                (PFN_DRVI2C_TIMEDELY)Delay);

#endif

    g_sSensorNT99141.bIsI2COpened = TRUE;
}

static void CloseSensorI2C(void)
{
#if defined __USE_HW_I2C__
    i2cClose();
#else

    if (g_sSensorNT99141.bIsI2COpened)
        DrvI2C_Close();
#endif

    g_sSensorNT99141.bIsI2COpened = FALSE;
}

static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
{
    UINT8 u8Data;

#if defined __USE_HW_I2C__
    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, uAddr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
    i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, uRegAddr, 2);
    i2cRead(&u8Data, 1);

#else
    // 2-Phase(ID address, register address) write transmission
    DrvI2C_SendStart();
    DrvI2C_WriteByte(uAddr, DrvI2C_Ack_Have, 8);    // Write ID address to sensor
    DrvI2C_WriteByte((UINT8)(uRegAddr >> 8), DrvI2C_Ack_Have, 8); // Write register address to sensor
    DrvI2C_WriteByte((UINT8)(uRegAddr & 0xff), DrvI2C_Ack_Have, 8); // Write register address to sensor
    DrvI2C_SendStop();

    // 2-Phase(ID-address, data(8bits)) read transmission
    DrvI2C_SendStart();
    DrvI2C_WriteByte(uAddr | 0x01, DrvI2C_Ack_Have, 8); // Write ID address to sensor
    u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have, 8);       // Read data from sensor
    DrvI2C_SendStop();
#endif
    return u8Data;

}

static BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
{
#if defined __USE_HW_I2C__
    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, uAddr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
    i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, uRegAddr, 2);
    i2cWrite(&uData, 1);

#else
    // 3-Phase(ID address, regiseter address, data(8bits)) write transmission
    volatile int u32Delay = 0x100;
    DrvI2C_SendStart();
    while (u32Delay--);
    if ((DrvI2C_WriteByte(uAddr, DrvI2C_Ack_Have, 8) == FALSE) ||        // Write ID address to sensor
            (DrvI2C_WriteByte((UINT8)(uRegAddr >> 8), DrvI2C_Ack_Have, 8) == FALSE) || // Write register address to sensor
            (DrvI2C_WriteByte((UINT8)(uRegAddr & 0xff), DrvI2C_Ack_Have, 8) == FALSE) || // Write register address to sensor
            (DrvI2C_WriteByte(uData, DrvI2C_Ack_Have, 8) == FALSE))    // Write data to sensor
    {
        NMLOG_ERROR("wnoack Addr = 0x%x \n", uRegAddr);
        DrvI2C_SendStop();
        return FALSE;
    }
    DrvI2C_SendStop();
#endif

    return TRUE;
}

static void
NT99141RegConfig(
    int32_t i32Index
)
{
    uint32_t i;
    uint16_t u16TableSize;
    struct NT_RegValue *psRegValue;
    uint8_t u8DeviceID = NT99141_DEV_ID;

    u16TableSize = s_NT99141SnrTable[REG_TABLE_INIT].u16TableSize;
    psRegValue = s_NT99141SnrTable[REG_TABLE_INIT].psRegTable;

    for (i = 0; i < u16TableSize; i++, psRegValue++)
    {
        vTaskDelay(1 / portTICK_RATE_MS);
        I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));
    }

    s_u32DimTableIdx = i32Index;
    u16TableSize = s_NT99141SnrTable[i32Index].u16TableSize;
    psRegValue = s_NT99141SnrTable[i32Index].psRegTable;

    uint8_t u8RegVal;

    for (i = 0; i < u16TableSize; i++, psRegValue++)
    {
        vTaskDelay(1 / portTICK_RATE_MS);
        I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));

        u8RegVal = 0;
        u8RegVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, psRegValue->u16RegAddr);

        if (u8RegVal != psRegValue->u8Value)
            NMLOG_ERROR("Write sensor address error = 0x%x \n", psRegValue->u16RegAddr);

    }

}

static BOOL
NT99141_ChangeResolution(
    VINDEV_T *psVinDev,
    S_SENSOR_ATTR *psSnrAttr,
    BOOL b32MasterPort
)
{
    int8_t i;
    int8_t i8WidthIdx;
    int8_t i8HeightIdx;
    int8_t i8SensorIdx;

    if ((psSnrAttr->u32PacketWidth > psSnrAttr->u32PlanarWidth) || (psSnrAttr->u32PacketHeight > psSnrAttr->u32PlanarHeight))
    {
        NMLOG_ERROR("Planar pipe resolution must large than packet pipe \n");
        return FALSE;
    }

    if (psSnrAttr->u32PlanarWidth > g_sSensorNT99141.u32SnrMaxWidth)
    {
        NMLOG_ERROR("The specified width over the sensor supported \n");
        return FALSE;
    }

    if (psSnrAttr->u32PlanarHeight > g_sSensorNT99141.u32SnrMaxHeight)
    {
        NMLOG_ERROR("The specified height over the sensor supported \n");
        return FALSE;
    }

    for (i = 0; i < NT_RESOL_SUPP_CNT ; i ++)
    {
        if (psSnrAttr->u32PlanarWidth <= s_asNTSuppResolTable[i].u16ImageWidth)
            break;
    }
    if (i == NT_RESOL_SUPP_CNT)
        return FALSE;

    i8WidthIdx = i;

    for (i = 0; i < NT_RESOL_SUPP_CNT ; i ++)
    {
        if (psSnrAttr->u32PlanarHeight <= s_asNTSuppResolTable[i].u16ImageHeight)
            break;
    }
    if (i == NT_RESOL_SUPP_CNT)
        return FALSE;

    i8HeightIdx = i;

    if (i8HeightIdx >= i8WidthIdx)
    {
        i8SensorIdx = i8HeightIdx;
    }
    else
    {
        i8SensorIdx = i8WidthIdx;
    }

    if (b32MasterPort == TRUE)
    {

        if (i8SensorIdx == REG_TABLE_VGA)
        {
            psSnrAttr->u32SnrInputWidth = 640;
            psSnrAttr->u32SnrInputHeight = 480;
        }
        else if (i8SensorIdx == REG_TABLE_SVGA)
        {
            psSnrAttr->u32SnrInputWidth = 800;
            psSnrAttr->u32SnrInputHeight = 600;
        }
        else if (i8SensorIdx == REG_TABLE_HD720)
        {
            psSnrAttr->u32SnrInputWidth = 1280;
            psSnrAttr->u32SnrInputHeight = 720;
        }


        OpenSensorI2C();

        NT99141RegConfig(i8SensorIdx);

        CloseSensorI2C();
    }

    uint32_t u32WidthFac;
    uint32_t u32HeightFac;
    uint32_t u32Fac;
    uint16_t u32GCD;
    uint32_t u32PlanarRatioW;
    uint32_t u32PlanarRatioH;

    u32GCD = NMUtil_GCD(psSnrAttr->u32PlanarWidth, psSnrAttr->u32PlanarHeight);

    u32PlanarRatioW = psSnrAttr->u32PlanarWidth / u32GCD;
    u32PlanarRatioH = psSnrAttr->u32PlanarHeight / u32GCD;

    u32WidthFac = psSnrAttr->u32SnrInputWidth / u32PlanarRatioW;
    u32HeightFac = psSnrAttr->u32SnrInputHeight / u32PlanarRatioH;

    if (u32WidthFac < u32HeightFac)
        u32Fac = u32WidthFac;
    else
        u32Fac = u32HeightFac;

    psSnrAttr->u32CapEngCropWinW = u32PlanarRatioW * u32Fac;
    psSnrAttr->u32CapEngCropWinH = u32PlanarRatioH * u32Fac;

    psSnrAttr->u32CapEngCropStartX = 0;
    psSnrAttr->u32CapEngCropStartY = 0;

    if (psSnrAttr->u32SnrInputWidth > psSnrAttr->u32CapEngCropWinW)
    {
        psSnrAttr->u32CapEngCropStartX = (psSnrAttr->u32SnrInputWidth - psSnrAttr->u32CapEngCropWinW) / 2;
    }

    if (psSnrAttr->u32SnrInputHeight > psSnrAttr->u32CapEngCropWinH)
    {
        psSnrAttr->u32CapEngCropStartY = (psSnrAttr->u32SnrInputHeight - psSnrAttr->u32CapEngCropWinH) / 2;
    }


    psVinDev->SetCropWinStartAddr(psSnrAttr->u32CapEngCropStartY,       //Horizontal start position
                                  psSnrAttr->u32CapEngCropStartX);

    psVinDev->SetCropWinSize(psSnrAttr->u32CapEngCropWinH,      //UINT16 u16Height,
                             psSnrAttr->u32CapEngCropWinW);              //UINT16 u16Width

    psVinDev->EncodePipeSize(psSnrAttr->u32PlanarHeight, psSnrAttr->u32PlanarWidth);
    psVinDev->PreviewPipeSize(psSnrAttr->u32PacketHeight, psSnrAttr->u32PacketWidth);
    psVinDev->SetStride(psSnrAttr->u32PacketWidth, psSnrAttr->u32PlanarWidth);


    return TRUE;
}


static int32_t
NT99141_Setup(
    VINDEV_T *psVinDev,
    S_SENSOR_ATTR *psSnrAttr,
    uint32_t u32PortNo,
    BOOL b32MasterPort,
    PFN_VIDEOIN_CALLBACK pfnIntCB
)
{
    int32_t i32Ret;
    PFN_VIDEOIN_CALLBACK pfnOldCallback;

    if (u32PortNo == 0)
    {
        i32Ret = register_vin_device(1, psVinDev);

        if (i32Ret < 0)
        {
            NMLOG_ERROR("Register vin 0 device fail\n");
            return -1;
        }
        psVinDev->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_SNR_CCIR601);
    }
    else if (u32PortNo == 1)
    {
        i32Ret = register_vin_device(2, psVinDev);

        if (i32Ret < 0)
        {
            NMLOG_ERROR("Register vin 1 device fail\n");
            return -1;
        }

        psVinDev->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_2ND_SNR_CCIR601);
    }
    else
    {
        NMLOG_ERROR("port number incorrect\n");
        return -1;
    }

    if (b32MasterPort == FALSE)
    {
        outp32(REG_CHIPCFG, inp32(REG_CHIPCFG) | CAP_SRC);  //VCAP1 share same capture port with VCAP 0
    }

    psVinDev->Open(72000, s_u32SnrClk);

    if (b32MasterPort == TRUE)
    {
        SnrPowerDown(FALSE);
        SnrReset();
    }

    if (NT99141_ChangeResolution(psVinDev, psSnrAttr, b32MasterPort) == FALSE)
    {
        NMLOG_ERROR("Unable to set sensor dimension\n");
        return -2;
    }

    psVinDev->SetPacketFrameBufferControl(FALSE, FALSE);

    psVinDev->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY,               //NT99141
                                    eVIDEOIN_IN_YUV422,     //Intput format
                                    psSnrAttr->ePacketColorFormat);     //Output format for packet


    psVinDev->SetStandardCCIR656(TRUE);                     //standard CCIR656 mode
    psVinDev->SetSensorPolarity(FALSE,
                                FALSE,
                                FALSE);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)psSnrAttr->pu8PacketFrameBuf);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                  (E_VIDEOIN_BUFFER)0,                            //Planar buffer Y addrress
                                  (UINT32) psSnrAttr->pu8PlanarFrameBuf);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                  (E_VIDEOIN_BUFFER)1,                            //Planar buffer U addrress
                                  (UINT32)(psSnrAttr->pu8PlanarFrameBuf + (psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight)));


    if (psSnrAttr->ePlanarColorFormat == eVIDEOIN_MACRO_PLANAR_YUV420)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer U addrress
                                      (UINT32)(psSnrAttr->pu8PlanarFrameBuf + (psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight)));
    }
    else if (psSnrAttr->ePlanarColorFormat == eVIDEOIN_PLANAR_YUV422)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer V addrress
                                      (UINT32)(psSnrAttr->pu8PlanarFrameBuf + (psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight) + ((psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight) / 2)));
    }
    else if (psSnrAttr->ePlanarColorFormat == eVIDEOIN_PLANAR_YUV420)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer V addrress
                                      (UINT32)(psSnrAttr->pu8PlanarFrameBuf + (psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight) + ((psSnrAttr->u32PlanarWidth * psSnrAttr->u32PlanarHeight) / 4)));
    }

    psVinDev->SetPlanarFormat(psSnrAttr->ePlanarColorFormat);               // Planar YUV422/420/macro

    psVinDev->SetPipeEnable(TRUE,                                   // Engine enable?
                            eVIDEOIN_BOTH_PIPE_ENABLE);     // which packet was enabled.

    psVinDev->EnableInt(eVIDEOIN_VINT);

    psVinDev->InstallCallback(eVIDEOIN_VINT,
                              (PFN_VIDEOIN_CALLBACK)pfnIntCB,
                              &pfnOldCallback);   //Frame End interrupt

    uint32_t u32FrameRateGCD;
    uint16_t u16InputFrameRate = s_u32InputFrameRate;
    uint16_t u16TargetFrameRate = psSnrAttr->u32FrameRate;
    uint32_t u32RatioM;
    uint32_t u32RatioN;

    u32FrameRateGCD = NMUtil_GCD(u16InputFrameRate, u16TargetFrameRate);
    u32RatioM = s_u32InputFrameRate / u32FrameRateGCD;
    u32RatioN = u16TargetFrameRate / u32FrameRateGCD;

    if (u32RatioM > u32RatioN)
    {
        if (u32PortNo == 0)
        {
            outp32(REG_VPEFRC, u32RatioN << 8 | u32RatioM);
        }
        else if (u32PortNo == 1)
        {
            outp32(REG_VPEFRC + 0x800, u32RatioN << 8 | u32RatioM);
        }
    }

    psVinDev->SetShadowRegister();

    return 0;
}


static void NT99141_Release(
    VINDEV_T *psVinDev
)
{
    psVinDev->DisableInt(eVIDEOIN_VINT);
    SnrPowerDown(TRUE);
}


#if defined (VIN_CONFIG_LUXDET_ADC_CHANNEL)
static BOOL s_bADCOpened;
static uint32_t s_u32ADCValue;

static void VoltageDetect_callback(UINT32 u32code)
{
    s_u32ADCValue = u32code;
}

#endif

static int32_t
LowLuxDet_NT99141(
    S_SENSOR_ATTR *psSnrAttr,
    BOOL *pbLowLuxDet
)
{
#if defined (VIN_CONFIG_LUXDET_ADC_CHANNEL)
    PFN_ADC_CALLBACK pfnOldCallback;

    if (s_bADCOpened == FALSE)
    {
        DrvADC_Open();
        DrvADC_InstallCallback(eADC_AIN,
                               VoltageDetect_callback,
                               &pfnOldCallback);

        s_bADCOpened = TRUE;
    }

    if (((*pbLowLuxDet) == FALSE) && (s_u32ADCValue >= psSnrAttr->u32LowLuxGate))
    {
        *pbLowLuxDet = TRUE;
    }
    else if (((*pbLowLuxDet) == TRUE) && (s_u32ADCValue <= psSnrAttr->u32HighLuxGate))
    {
        *pbLowLuxDet = FALSE;
    }

    DrvADC_VoltageDetection(VIN_CONFIG_LUXDET_ADC_CHANNEL);

#else
    uint8_t u8ShutterH;
    uint8_t u8ShutterL;
    uint16_t u16Shutter;
    uint8_t u8RegGain;
    uint32_t u32Gain;
    uint32_t u32AE;

    uint8_t u8DeviceID = NT99141_DEV_ID;

    OpenSensorI2C();

    u8ShutterH = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x3012);
    u8ShutterL = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x3013);
    u16Shutter = (uint16_t)(u8ShutterH << 8) | u8ShutterL;

    u8RegGain = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x301D);

    u32Gain = (((u8RegGain & 0x80) >> 7) + 1) * (((u8RegGain & 0x40) >> 6) + 1) *
              (((u8RegGain & 0x20) >> 5) + 1) * (((u8RegGain & 0x10) >> 4) + 1) * ((((u8RegGain & 0x0F) + 1) / 16) + 1);

    u32AE = u16Shutter * u32Gain;

    if (((*pbLowLuxDet) == FALSE) && (u32AE >= psSnrAttr->u32LowLuxGate))
    {
        *pbLowLuxDet = TRUE;
    }
    else if (((*pbLowLuxDet) == TRUE) && (u32AE <= psSnrAttr->u32HighLuxGate))
    {
        *pbLowLuxDet = FALSE;
    }

    CloseSensorI2C();
#endif
    return 0;
}

//Use PWM3 driving IRLED
/*---------------------------------------------------------------------------------------------------------*/
/* PWM Timer Callback function                                                                             */
/*---------------------------------------------------------------------------------------------------------*/

static int32_t
SetNightMode_NT99141(
    BOOL bEnable
)
{
//  PWM_TIME_DATA_T sPt;

    if (bEnable)
    {

        NMLOG_INFO("Turn on IR LED\n");

    }
    else
    {
        /*--------------------------------------------------------------------------------------*/
        /* Stop PWM Timer 0 (Recommended procedure method 2)                                    */
        /* Set PWM Timer counter as 0, When interrupt request happen, disable PWM Timer         */
        /* Set PWM Timer counter as 0 in Call back function                                     */
        /*--------------------------------------------------------------------------------------*/
        NMLOG_INFO("Turn off IR LED\n");


    }

    return 0;
}

typedef struct
{
    uint16_t    u16Addr;
    uint8_t u8Value;
} S_SNR_REG;


//Sensor clock 16MHz, VGA/SVGA, Flicker 50 Hz table
static S_SNR_REG s_asClk16VGAFlicker50Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x64}, {0x32C1, 0x64}, {0x32C2, 0x64}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x96},
    {0x32C9, 0x64}, {0x32CA, 0x84}, {0x32CB, 0x84}, {0x32CC, 0x84}, {0x32CD, 0x84},
    {0x32DB, 0x72}, {0, 0}
};

//Sensor clock 16MHz, VGA/SVGA, Flicker 60 Hz table
static S_SNR_REG s_asClk16VGAFlicker60Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x68}, {0x32C1, 0x68}, {0x32C2, 0x68}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x7D},
    {0x32C9, 0x68}, {0x32CA, 0x88}, {0x32CB, 0x88}, {0x32CC, 0x88}, {0x32CD, 0x88},
    {0x32DB, 0x6F}, {0, 0}
};

//Sensor clock 16MHz, HD, Flicker 50 Hz table
static S_SNR_REG s_asClk16HDFlicker50Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x64}, {0x32C1, 0x64}, {0x32C2, 0x64}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x90},
    {0x32C9, 0x64}, {0x32CA, 0x84}, {0x32CB, 0x84}, {0x32CC, 0x84}, {0x32CD, 0x84},
    {0x32DB, 0x72}, {0, 0}
};

//Sensor clock 16MHz, HD, Flicker 60 Hz table
static S_SNR_REG s_asClk16HDFlicker60Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x68}, {0x32C1, 0x68}, {0x32C2, 0x68}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x78},
    {0x32C9, 0x68}, {0x32CA, 0x88}, {0x32CB, 0x88}, {0x32CC, 0x88}, {0x32CD, 0x88},
    {0x32DB, 0x6E}, {0, 0}
};

//Sensor clock 20MHz, VGA/SVGA, Flicker 50 Hz table
static S_SNR_REG s_asClk20VGAFlicker50Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x60}, {0x32C1, 0x60}, {0x32C2, 0x60}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xB8},
    {0x32C9, 0x60}, {0x32CA, 0x80}, {0x32CB, 0x80}, {0x32CC, 0x80}, {0x32CD, 0x80},
    {0x32DB, 0x77}, {0, 0}
};

//Sensor clock 20MHz, VGA/SVGA, Flicker 60 Hz table
static S_SNR_REG s_asClk20VGAFlicker60Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x63}, {0x32C1, 0x63}, {0x32C2, 0x63}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x99},
    {0x32C9, 0x63}, {0x32CA, 0x83}, {0x32CB, 0x83}, {0x32CC, 0x83}, {0x32CD, 0x83},
    {0x32DB, 0x73}, {0, 0}
};

//#define __HD_20MHz_25FPS__

//Sensor clock 20MHz, HD, Flicker 50 Hz table
static S_SNR_REG s_asClk20HDFlicker50Table[] =
{

    {0x32BF, 0x60}, {0x32C0, 0x60}, {0x32C1, 0x60}, {0x32C2, 0x60}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xB4},
    {0x32C9, 0x60}, {0x32CA, 0x80}, {0x32CB, 0x80}, {0x32CC, 0x80}, {0x32CD, 0x80},
    {0x32DB, 0x76}, {0, 0}

};

//Sensor clock 20MHz, HD, Flicker 60 Hz table
static S_SNR_REG s_asClk20HDFlicker60Table[] =
{

    {0x32BF, 0x60}, {0x32C0, 0x63}, {0x32C1, 0x63}, {0x32C2, 0x63}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0x96},
    {0x32C9, 0x63}, {0x32CA, 0x83}, {0x32CB, 0x83}, {0x32CC, 0x83}, {0x32CD, 0x83},
    {0x32DB, 0x72}, {0, 0}

};

//Sensor clock 24MHz, VGA/SVGA, Flicker 50 Hz table
static S_SNR_REG s_asClk24VGAFlicker50Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x5A}, {0x32C1, 0x5A}, {0x32C2, 0x5A}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xDD},
    {0x32C9, 0x5A}, {0x32CA, 0x7A}, {0x32CB, 0x7A}, {0x32CC, 0x7A}, {0x32CD, 0x7A},
    {0x32DB, 0x7B}, {0, 0}
};

//Sensor clock 24MHz, VGA/SVGA, Flicker 60 Hz table
static S_SNR_REG s_asClk24VGAFlicker60Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x60}, {0x32C1, 0x5F}, {0x32C2, 0x5F}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xB8},
    {0x32C9, 0x5F}, {0x32CA, 0x7F}, {0x32CB, 0x7F}, {0x32CC, 0x7F}, {0x32CD, 0x80},
    {0x32DB, 0x77}, {0, 0}
};

//Sensor clock 24MHz, HD, Flicker 50 Hz table
static S_SNR_REG s_asClk24HDFlicker50Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x5A}, {0x32C1, 0x5A}, {0x32C2, 0x5A}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xDD},
    {0x32C9, 0x5A}, {0x32CA, 0x7A}, {0x32CB, 0x7A}, {0x32CC, 0x7A}, {0x32CD, 0x7A},
    {0x32DB, 0x7B}, {0, 0}
};

//Sensor clock 24MHz, HD, Flicker 60 Hz table
static S_SNR_REG s_asClk24HDFlicker60Table[] =
{
    {0x32BF, 0x60}, {0x32C0, 0x60}, {0x32C1, 0x5F}, {0x32C2, 0x5F}, {0x32C3, 0x00},
    {0x32C4, 0x20}, {0x32C5, 0x20}, {0x32C6, 0x20}, {0x32C7, 0x00}, {0x32C8, 0xB8},
    {0x32C9, 0x5F}, {0x32CA, 0x7F}, {0x32CB, 0x7F}, {0x32CC, 0x7F}, {0x32CD, 0x80},
    {0x32DB, 0x77}, {0, 0}
};

typedef struct
{
    S_SNR_REG *apsRegTable[eSNR_FLICKER_CNT];
} S_SNR_DIME_FLICKER;

static S_SNR_DIME_FLICKER s_sClk16VGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk16VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk16VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk16SVGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk16VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk16VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk16HDFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk16HDFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk16HDFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk20VGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk20VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk20VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk20SVGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk20VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk20VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk20HDFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk20HDFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk20HDFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk24VGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk24VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk24VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk24SVGAFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk24VGAFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk24VGAFlicker60Table,
};

static S_SNR_DIME_FLICKER s_sClk24HDFlicker =
{
    .apsRegTable[eSNR_FLICKER_50] = s_asClk24HDFlicker50Table,
    .apsRegTable[eSNR_FLICKER_60] = s_asClk24HDFlicker60Table,
};

typedef struct
{
    S_SNR_DIME_FLICKER *apsFlickerDimeTable[eSNR_DIMENSION_CNT];
} S_SNR_CLK_FLICKER;

static S_SNR_CLK_FLICKER s_sClk16Flicker =
{
    .apsFlickerDimeTable[eSNR_DIMENSION_VGA] = &s_sClk16VGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_SVGA] = &s_sClk16SVGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_HD] = &s_sClk16HDFlicker,
};

static S_SNR_CLK_FLICKER s_sClk20Flicker =
{
    .apsFlickerDimeTable[eSNR_DIMENSION_VGA] = &s_sClk20VGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_SVGA] = &s_sClk20SVGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_HD] = &s_sClk20HDFlicker,
};

static S_SNR_CLK_FLICKER s_sClk24Flicker =
{
    .apsFlickerDimeTable[eSNR_DIMENSION_VGA] = &s_sClk24VGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_SVGA] = &s_sClk24SVGAFlicker,
    .apsFlickerDimeTable[eSNR_DIMENSION_HD] = &s_sClk24HDFlicker,
};

typedef struct
{
    S_SNR_CLK_FLICKER *apsFlickerClockTable[eSNR_CLOCK_CNT];
} S_SNR_FLICKER;

static S_SNR_FLICKER s_sSnrFlicker =
{
    .apsFlickerClockTable[eSNR_CLOCK_16] = &s_sClk16Flicker,
    .apsFlickerClockTable[eSNR_CLOCK_20] = &s_sClk20Flicker,
    .apsFlickerClockTable[eSNR_CLOCK_24] = &s_sClk24Flicker,
};

static int32_t SetSensorFlicker_NT99141(
    S_SENSOR_ATTR *psSnrAttr
)
{
    E_SNR_CLOCK eSnrClk = eSNR_CLOCK_16;
    E_SNR_DIMENSION eSnrDim = eSNR_DIMENSION_HD;
    S_SNR_REG *psRegTable = NULL;

    if (s_u32SnrClk <= 16000)
    {
        eSnrClk = eSNR_CLOCK_16;
    }
    else if (s_u32SnrClk <= 20000)
    {
        eSnrClk = eSNR_CLOCK_20;

    }
    else if (s_u32SnrClk <= 24000)
    {
        eSnrClk = eSNR_CLOCK_24;
    }

    if (s_u32DimTableIdx == REG_TABLE_VGA)
    {
        eSnrDim = eSNR_DIMENSION_VGA;
    }
    else if (s_u32DimTableIdx == REG_TABLE_SVGA)
    {
        eSnrDim = eSNR_DIMENSION_SVGA;
    }
    else if (s_u32DimTableIdx == REG_TABLE_HD720)
    {
        eSnrDim = eSNR_DIMENSION_HD;
    }

    if (psSnrAttr->u32Flicker == 60)
    {
        psRegTable =
            s_sSnrFlicker.apsFlickerClockTable[eSnrClk]->apsFlickerDimeTable[eSnrDim]->apsRegTable[eSNR_FLICKER_60];
    }
    else
    {
        psRegTable =
            s_sSnrFlicker.apsFlickerClockTable[eSnrClk]->apsFlickerDimeTable[eSnrDim]->apsRegTable[eSNR_FLICKER_50];
    }

    uint8_t u8DeviceID = NT99141_DEV_ID;

    OpenSensorI2C();

    // Set flicker register
    while (psRegTable->u16Addr != 0)
    {
        I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, psRegTable->u16Addr, psRegTable->u8Value);
        psRegTable ++;
    }

    //Activate settings
    I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x3060, 0x1);

    CloseSensorI2C();

    return 0;
}

static int32_t SetSensorRegister_NT99141(
    uint32_t u32RegAddr,
    uint32_t u32RegValue
)
{
    uint8_t u8DeviceID = NT99141_DEV_ID;
    uint8_t u8RegValue = u32RegValue;
    uint16_t u16RegAddr = u32RegAddr;


    OpenSensorI2C();

    I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, u16RegAddr, u8RegValue);


    u8RegValue = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, u16RegAddr);

//  INFO("Set sensor register %x writer value %x, read value %x \n", u16RegAddr, u32RegValue, u8RegValue);
    if (u8RegValue != u32RegValue)
    {
        NMLOG_INFO("Set sensor register %x writer value %x, read value %x \n", u16RegAddr, u32RegValue, u8RegValue);
    }

    CloseSensorI2C();

    return 0;
}

static int32_t GetSensorRegister_NT99141(
    uint32_t u32RegAddr,
    uint32_t *pu32RegValue
)
{
    uint8_t u8DeviceID = NT99141_DEV_ID;
    uint8_t u8RegValue;
    uint16_t u16RegAddr = u32RegAddr;


    OpenSensorI2C();

    u8RegValue = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, u16RegAddr);

    NMLOG_INFO("Get sensor register %x, read value %x \n", u16RegAddr, u8RegValue);

    *pu32RegValue = u8RegValue;
    CloseSensorI2C();

    return 0;

}

static int32_t
SetFrameRate_NT99141(
    uint32_t u32PortNo,
    S_SENSOR_ATTR *psSnrAttr
)
{
    uint32_t u32FrameRateGCD;
    uint16_t u16InputFrameRate = s_u32InputFrameRate;
    uint16_t u16TargetFrameRate = psSnrAttr->u32FrameRate;
    uint32_t u32RatioM;
    uint32_t u32RatioN;

    u32FrameRateGCD = NMUtil_GCD(u16InputFrameRate, u16TargetFrameRate);
    u32RatioM = s_u32InputFrameRate / u32FrameRateGCD;
    u32RatioN = u16TargetFrameRate / u32FrameRateGCD;

    if (u32RatioM >= u32RatioN)
    {
        if (u32PortNo == 0)
        {
            outp32(REG_VPEFRC, u32RatioN << 8 | u32RatioM);
        }
        else if (u32PortNo == 1)
        {
            outp32(REG_VPEFRC + 0x800, u32RatioN << 8 | u32RatioM);
        }
    }

    return 0;
}


S_SENSOR_IF g_sSensorNT99141 =
{
    .u32SnrMaxWidth = 1280,
    .u32SnrMaxHeight = 720,
    .pfnSenesorSetup = NT99141_Setup,
    .pfnSensorRelease = NT99141_Release,
    .bIsInit = FALSE,
    .bIsI2COpened = FALSE,
    .pfnLowLuxDet = LowLuxDet_NT99141,
    .pfnSetNightMode = SetNightMode_NT99141,
    .pfnSetFlicker = SetSensorFlicker_NT99141,
    .pfnSetSnrReg = SetSensorRegister_NT99141,
    .pfnGetSnrReg = GetSensorRegister_NT99141,
    .pfnSetAEInnerLum = NULL,
    .pfnSetFrameRate = SetFrameRate_NT99141,
};


#endif

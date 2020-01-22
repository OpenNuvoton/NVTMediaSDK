/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
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

#ifndef __VIN_CONFIG_H__
#define __VIN_CONFIG_H__

#include "N9H26_GPIO.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Hardware configuration
///////////////////////////////////////////////////////////////////////////////////////////////////////

// Enable sensor port
#define VIN_CONFIG_USE_1ST_PORT			1
//#define VIN_CONFIG_USE_2ND_PORT		1

//Sensor selection
#define VIN_CONFIG_SENSOR_NT99141			1
//#define VIN_CONFIG_SENSOR_NT99142			1
//#define VIN_CONFIG_SENSOR_GC0308			1
//#define VIN_CONFIG_SENSOR_HM2056			1

//Low lux detect use ADC channel
//#define VIN_CONFIG_LUXDET_ADC_CHANNEL		2			//ADC channel 2

//IR LED driving use PWM timer
#define VIN_CONFIG_IRLED_PWM_TIMER				3		//PWM timer 3
#define VIN_CONFIG_IRLED_PWM_FREQ				120		//120Hz
#define VIN_CONFIG_IRLED_PWM_HIGH_PLUS_RATIO	70	

//IR cut
//#define VIN_CONFIG_HAVE_IRCUT

//GPA14 for IR cut on. For daytime mode
#define IRCUT_ON_MFP_PORT		REG_GPAFUN1
#define IRCUT_ON_MFP_PIN		MF_GPA14
#define IRCUT_ON_MFP_GPIO_MODE	(0x2 << 24)
#define IRCUT_ON_GPIO_PORT		GPIO_PORTA
#define IRCUT_ON_GPIO_PIN		BIT14

//GPA15 for IR cut off. For night mode
#define IRCUT_OFF_MFP_PORT		REG_GPAFUN1
#define IRCUT_OFF_MFP_PIN		MF_GPA15
#define IRCUT_OFF_MFP_GPIO_MODE	(0x2 << 28)
#define IRCUT_OFF_GPIO_PORT		GPIO_PORTA
#define IRCUT_OFF_GPIO_PIN		BIT15


// vin port0 config. Note: We will use vin port0 planar buffer for firmware upgrade buffer. Keep enough to receive firmware upgrade binary
#define VIN_CONFIG_PORT0_PLANAR_WIDTH		(1280)			//video-in planar pipe maximum width
#define VIN_CONFIG_PORT0_PLANAR_HEIGHT		(720)			//video-in planar pipe maximum height

#define VIN_CONFIG_PORT0_PACKET_WIDTH		(320)			//video-in packet pipe maximum width
#define VIN_CONFIG_PORT0_PACKET_HEIGHT		(240)			//video-in packet pipe maximum height

#define VIN_CONFIG_PORT0_PLANAR_FRAME_BUF			(3)				//video-in planar pipe maximum frame buffers. Must >= 3
#define VIN_CONFIG_PORT0_PACKET_FRAME_BUF			(3)				//video-in planar pipe maximum frame buffers. Must >= 3

#define VIN_CONFIG_PORT0_FRAME_RATE						(20)

#define VIN_CONFIG_PORT0_NIGHT_MODE						(1)

//#define VIN_CONFIG_PORT0_LOW_LUX_GATE			(1989)				//use sensor AE
#define VIN_CONFIG_PORT0_LOW_LUX_GATE				(1500)					//use ADC
//#define VIN_CONFIG_PORT0_HIGH_LUX_GATE			(768)					//use sensor AE
#define VIN_CONFIG_PORT0_HIGH_LUX_GATE			(150)					//use ADC

#define VIN_CONFIG_PORT0_FLICKER						(50)					//flicker 50/60

#define VIN_CONFIG_PORT0_AE_MAX_LUM				(0)					//0: disable adaptive AE weight control
#define VIN_CONFIG_PORT0_AE_MIN_LUM				(80)


//vin port1 config
#define VIN_CONFIG_PORT1_PLANAR_WIDTH		(640)			//video-in planar pipe maximum width
#define VIN_CONFIG_PORT1_PLANAR_HEIGHT		(480)			//video-in planar pipe maximum height

#define VIN_CONFIG_PORT1_PACKET_WIDTH		(32)			//video-in packet pipe maximum width
#define VIN_CONFIG_PORT1_PACKET_HEIGHT		(24)			//video-in packet pipe maximum height

#define VIN_CONFIG_PORT1_PLANAR_FRAME_BUF			(1)				//video-in planar pipe maximum frame buffers. Don't over 3
#define VIN_CONFIG_PORT1_PACKET_FRAME_BUF			(1)				//video-in planar pipe maximum frame buffers. Don't over 3

#define VIN_CONFIG_PORT1_FRAME_RATE						(15)

#define VIN_CONFIG_PORT1_NIGHT_MODE						(1)

//#define VIN_CONFIG_PORT1_LOW_LUX_GATE			(1989)				//use sensor AE
#define VIN_CONFIG_PORT1_LOW_LUX_GATE				(1500)					//use ADC
//#define VIN_CONFIG_PORT1_HIGH_LUX_GATE			(768)					//use sensor AE
#define VIN_CONFIG_PORT1_HIGH_LUX_GATE			(150)					//use ADC

#define VIN_CONFIG_PORT1_FLICKER						(50)					//flicker 50/60

#define VIN_CONFIG_PORT1_AE_MAX_LUM				(0)					//0: disable adaptive AE weight control
#define VIN_CONFIG_PORT1_AE_MIN_LUM				(80)

#define VIN_CONFIG_SENSOR_PCLK_48MHZ		48000
#define VIN_CONFIG_SENSOR_PCLK_60MHZ		60000
#define VIN_CONFIG_SENSOR_PCLK_74MHZ		74000
#define VIN_CONFIG_SENSOR_PCLK				VIN_CONFIG_SENSOR_PCLK_60MHZ

#define VIN_CONFIG_SENSOR_I2C_SCK_GPIO_PORT		eDRVGPIO_GPIOB	
#define VIN_CONFIG_SENSOR_I2C_SCK_GPIO_PIN		eDRVGPIO_PIN13	

#define VIN_CONFIG_SENSOR_I2C_SCK_MFP_PORT		REG_GPBFUN1					//sensor i2c sck multiple function port
#define VIN_CONFIG_SENSOR_I2C_SCK_MFP_PIN			MF_GPB13						//sensor i2c sck multiple function pin

#define VIN_CONFIG_SENSOR_I2C_SDA_GPIO_PORT		eDRVGPIO_GPIOB	
#define VIN_CONFIG_SENSOR_I2C_SDA_GPIO_PIN		eDRVGPIO_PIN14	

#define VIN_CONFIG_SENSOR_I2C_SDA_MFP_PORT		REG_GPBFUN1					//sensor i2c sda multiple function port
#define VIN_CONFIG_SENSOR_I2C_SDA_MFP_PIN			MF_GPB14						//sensor i2c sda multiple function pin


#define VIN_CONFIG_SENSOR_RESET_GPIO_PORT		GPIO_PORTB	
#define VIN_CONFIG_SENSOR_RESET_GPIO_PIN		BIT4	

#define VIN_CONFIG_SENSOR_RESET_MFP_PORT		REG_GPBFUN0					//sensor reset multiple function port
#define VIN_CONFIG_SENSOR_RESET_MFP_PIN			MF_GPB4						//sensor reset multiple function pin

#define VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PORT		GPIO_PORTE	
#define VIN_CONFIG_SENSOR_POWERDOWN_GPIO_PIN		BIT8	

#define VIN_CONFIG_SENSOR_POWERDOWN_MFP_PORT		REG_GPEFUN1					//sensor power down multiple function port
#define VIN_CONFIG_SENSOR_POWERDOWN_MFP_PIN			MF_GPE8						//sensor power down multiple function pin

#define VIN_CONFIG_COLOR_NONE					0
#define VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420	1
#define VIN_CONFIG_COLOR_PLANAR_YUV422			2
#define VIN_CONFIG_COLOR_PLANAR_YUV420			3
#define VIN_CONFIG_COLOR_PACKET_YUV422			4

#define VIN_CONFIG_PORT0_PLANAR_FORMAT	VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420 //video-in planar pipe color format. If planar pipe color is YUV420MB, open this to reduce memory usage.
#define VIN_CONFIG_PORT1_PLANAR_FORMAT	VIN_CONFIG_COLOR_PLANAR_MACRO_YUV420 //video-in planar pipe color format. If planar pipe color is YUV420MB, open this to reduce memory usage.

#endif

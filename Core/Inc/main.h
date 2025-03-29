/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define VERS_STRING "0.93  Build 1823"


/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

void Error_Handler(void);


// ----- PORT DEFINITIONS
// ----- PORT A
//PA0 VIN_VINV_IMON
#define VIN_VINV_IMON_PIN GPIO_PIN_0
#define VIN_VINV_IMON_GPIO_Port GPIOA

//PA1 VIN_MAIN_IMON
#define VIN_VMAIN_IMON_PIN GPIO_PIN_1
#define VIN_VMAIN_IMON_GPIO_Port GPIOA

#define USART_TX_PIN GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA

#define USART_RX_PIN GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA

//PA4 5V_VINV_IMON
#define V5_VINV_IMON_PIN GPIO_PIN_4
#define V5_VINV_IMON_GPIO_Port GPIOA

//PA5 RS232EN
#define SER1_RS232_EN_PIN GPIO_PIN_5
#define SER1_RS232_EN_GPIO_Port GPIOA
//#define RS232_1_EN_PIN GPIO_PIN_5
//#define RS232_1_EN_PORT GPIOA


#define ADC_V_INV_REG12V_PIN GPIO_PIN_6
#define ADC_V_INV_REG12V_GPIO_Port GPIOA

#define ADC_3V3_PERI_PIN GPIO_PIN_7
#define ADC_3V3_PERI_GPIO_Port GPIOA

//PA8 J7_RESETn_IN
#define J7_RST_PIN GPIO_PIN_8
#define J7_RST_PORT GPIOA

//PA10 J4_RESET-
#define COM2_RSTn_PIN GPIO_PIN_10
#define COM2_RSTn_GPIO_Port GPIOA
//#define J4_RST_PIN GPIO_PIN_10
//#define J4_RST_PORT GPIOA

//PA11 VIN_VINV_PG
#define VIN_VINV_PG_PIN GPIO_PIN_11
#define VIN_VINV_PG_GPIO_Port GPIOA

//PA12 V5_VMAIN_PG
#define V5_VMAIN_PG_PIN GPIO_PIN_12
#define V5_VMAIN_PG_GPIO_Port GPIOA

#define TMS_PIN GPIO_PIN_13
#define TMS_GPIO_Port GPIOA

#define TCK_PIN GPIO_PIN_14
#define TCK_GPIO_Port GPIOA

//PA15 P105
#define P105_PIN GPIO_PIN_15
#define P105_GPIO_Port GPIOA


// ----- PORT B
#define ADC_5V_AUDIO_PIN GPIO_PIN_0
#define ADC_5V_AUDIO_GPIO_Port GPIOB

#define LED1_PIN GPIO_PIN_1
#define LED1_GPIO_PORT GPIOB

#define LED2_PIN GPIO_PIN_5
#define LED2_GPIO_PORT GPIOB

//PB13 VIN_MAIN_PG
#define VIN_VMAIN_PG_PIN GPIO_PIN_13
#define VIN_VMAIN_PG_GPIO_PORT GPIOB

//PB14 5V_VINV_PG
#define V5_VINV_PG_PIN GPIO_PIN_14
#define V5_VINV_PG_GPIO_PORT GPIOB

//PB15 I2C_GPIO_INTn
#define I2C_GPIO_INTn_PIN GPIO_PIN_15
#define I2C_GPIO_INTn_GPIO_PORT GPIOB



// ----- PORT C
//PC0 UART5_OEn
#define RS232_5_OEN_PIN GPIO_PIN_0
#define RS232_5_OEN_PORT GPIOC

//PC1 5V_MAIN_IMON
#define V5_VMAIN_IMON_PIN GPIO_PIN_1
#define V5_VMAIN_IMON_GPIO_PORT GPIOC

#define ADC_V_INV_PIN GPIO_PIN_2
#define ADC_V_INV_GPIO_PORT GPIOC

#define ADC_V_MAIN_PIN GPIO_PIN_3
#define ADC_V_MAIN_GPIO_PORT GPIOC

//PC6 UART_RTS
#define RS232_5_RTS_PIN GPIO_PIN_6
#define RS232_5_RTS_PORT GPIOC

//PC7 UART_RS485_DE
#define RS485_4_DE_PIN GPIO_PIN_7
#define RS485_4_DE_PORT GPIOC

//PC8 UART_RS485_REn
#define RS485_4_REN_PIN GPIO_PIN_8
#define RS485_4_REN_PORT GPIOC

//PC9 INVALIDn
#define SER1_INVALIDn_PIN GPIO_PIN_9
#define SER1_INVALIDn_GPIO_PORT GPIOC

//PC12 RS232_EN
#define SER2_RS232_EN_PIN GPIO_PIN_12
#define SER2_RS232_EN_GPIO_PORT GPIOC

//PC13 INVALIDn
#define SER2_INVALIDn_PIN GPIO_PIN_13
#define SER2_INVALIDn_GPIO_PORT GPIOC



// ----- PORT D
//PD2 P104
#define P104_PIN GPIO_PIN_2
#define P104_GPIO_PORT GPIOD


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

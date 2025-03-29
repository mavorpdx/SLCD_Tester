/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

// Includes ------------------------------------------------------------------
#include <stdio.h>

#include "main.h"
#include "utils.h"

#include "gpio.h"
#include "pca9534.h"
#include "adc.h"
#include "i2c.h"
#include "uart.h"
#include "utils.h"
#include "command.h"

void SystemClock_Config(void);

int main(void)
{
	uint32_t previous_millis = 0;
	uint32_t now_millis;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  UART_Init();
  I2C_Init();    //before GPIO for the PCA9534A

  if (huart1.Instance->CR1 & USART_CR1_UE) {
      printf("USART1 is enabled\n");
  } else {
      printf("USART1 is disabled\n");
  }


  GPIO_Init();

  if (GPIO_SetOutputByName("SER1_RS232_EN", 1) != HAL_OK){
	  printf("GPIO_SET_ERROR!\n\r");
  }

  if (GPIO_SetOutputByName("V5_MAIN_EN", GPIO_PIN_SET) != HAL_OK){
	  printf("MAIN\n\r");
  }

  if (GPIO_SetOutputByName("V5_INV_EN", GPIO_PIN_SET) != HAL_OK){
	  printf("MAIN\n\r");
  }



  if (huart1.Instance->CR1 & USART_CR1_UE) {
      printf("USART1 is enabled\n");
  } else {
      printf("USART1 is disabled\n");
  }





  ADC_Init();
  int longer_count = 0;
  int shorter_count = 0;

  updateLEDStatus();
	readADCValues();
	readADCValues();
	readADCValues();

  sendDebug("Tester is up", "");
  /* Infinite loop */
  while (1)
  {
	  handleSerialCommunications();
	  now_millis = millis();
	  delay(10);
	  if (now_millis - previous_millis >= 100) {
		//printf("!");
	    previous_millis = now_millis;
 	    updateLEDStatus();
 	    shorter_count++;
	    if(shorter_count > 10){
	    	shorter_count = 0;
	    	readADCValues();
	    }
 	    longer_count++;
	    if(longer_count > 500){
	    	longer_count = 0;
	    }
	  }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

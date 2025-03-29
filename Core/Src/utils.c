/*
 * utils.c
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "utils.h"
#include <cmsis_gcc.h>

uint8_t controllerType = 0;
uint32_t now_millis = 0;
uint32_t previous_millis = 0;
char tStr[200];

uint8_t DEBUG_ADC = 0;
uint8_t DEBUG_CMD = 1;
uint8_t DEBUG_GPIO = 1;
uint8_t DEBUG_I2C = 0;
uint8_t DEBUG_UART = 1;

void Error_Handler(void)
{
  printf("\n\nBSOD\n\n");
  __disable_irq();
  while (1)
  {
    // Stay here
  }
}

void str2upper(char* str) {
    if (!str) return;
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - 32;  // Convert to uppercase by subtracting 32 from ASCII value
        }
        str++;
    }
}

uint32_t str2num(const char *data) {
    char *endptr;
    unsigned long num;  // Changed to unsigned long

    if (data == NULL || *data == '\0') {
        return 0; // Default to 0 if input is NULL or empty
    }

    while (isspace((unsigned char)*data)) data++; // Skip leading spaces

    // Check for negative sign which would be invalid for uint32_t
    if (*data == '-') {
        return 0; // Default to 0 for negative values
    }

    num = strtoul(data, &endptr, 10);  // Changed to strtoul for unsigned values

    // Check for conversion errors or overflow
    if (endptr == data || *endptr != '\0' || num > UINT32_MAX) {
        return 0; // Default to 0 for invalid input or overflow
    }

    return (uint32_t)num;
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

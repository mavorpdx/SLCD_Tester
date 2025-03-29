/*
 * utils.h
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include <stdbool.h>
#include <stdint.h>




// For “millis” and “delay”
#define millis()        HAL_GetTick()
#define delay(ms)       HAL_Delay(ms)

extern uint32_t previous_led_millis;
extern uint8_t serialCFG;
extern char tStr[];

extern uint8_t DEBUG_ADC;
extern uint8_t DEBUG_CMD;
extern uint8_t DEBUG_GPIO;
extern uint8_t DEBUG_I2C;
extern uint8_t DEBUG_UART;


#define SLCD5_TYPE 0
#define SLCD43_TYPE 1
#define SCLD6_TYPE 2
extern uint8_t controllerType;

void Error_Handler(void);
void str2upper(char* str);
uint32_t str2num(const char *data);

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#endif /* USE_FULL_ASSERT */


#endif /* INC_UTILS_H_ */

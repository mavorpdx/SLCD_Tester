/*
 * gpio.h
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_


#ifdef __cplusplus
extern "C" {
#endif



#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "pca9534.h"

#include "utils.h"

//extern bool led_state;
//extern uint32_t previous_led_millis;

extern PCA9534_HandleTypeDef hPCA;

typedef struct {
	const char *name;
	uint8_t pin;
} GPIOMap;

extern const GPIOMap gpioMaps[];

typedef enum {
    GPIO_TYPE_MCU,     // Direct MCU GPIO
    GPIO_TYPE_PCA9534  // PCA9534 I2C GPIO expander
} GPIO_PortType;

typedef struct {
    GPIO_PortType type;        // Type of GPIO (MCU or PCA9534)
    GPIO_TypeDef* GPIOx;       // MCU GPIO port (GPIOA, GPIOB, etc.) - for MCU pins
    uint16_t GPIO_Pin;         // MCU GPIO pin number - for MCU pins
    uint8_t PCA9534_Pin;       // PCA9534 pin number - for PCA9534 pins
    char name[16];             // Pin name for reference
} GPIO_PinConfig;

// GPIO Input Configuration
typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
    char name[20];
    char description[50];
    int activeState;
} GPIO_InputConfig;

// Function declarations to add to gpio.h
const GPIO_InputConfig* GPIO_FindInputByName(const char* name);

int GPIO_ReadInputByName(const char* name);
HAL_StatusTypeDef GPIO_PrintInputByName(char *buffer, const char* name);

void GPIO_PrintInputStates(char *buffer);
bool GPIO_IsInputActive(const char* name);
void GPIO_PrintDetailedInfo(char *buffer);

HAL_StatusTypeDef GPIO_SetPin(const GPIO_PinConfig* config, GPIO_PinState state);
HAL_StatusTypeDef GPIO_GetPin(const GPIO_PinConfig* config, GPIO_PinState* state);
HAL_StatusTypeDef GPIO_SetOutputByName(const char* name, GPIO_PinState state);
HAL_StatusTypeDef GPIO_ToggleByName(const char* name);
void GPIO_PrintStates(char *buffer);
void setSerialCFG(void);
void GPIO_Init(void);
void MX_GPIO_Init(void);
void updateLEDStatus(void);
void read_SER1_INVALIDn_PIN(void);
void read_SER2_INVALIDn_PIN(void);

#endif //INC_GPIO_H_

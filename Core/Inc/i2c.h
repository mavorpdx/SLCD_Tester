/*
 * i2c.h
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "utils.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

void I2C_Init(void);

// Define register addresses
#define I2C_REG_0       0x00
#define I2C_REG_1       0x01
#define I2C_NUM_REGS    2

/**
 * @brief Initialize I2C slave mode with register access
 * @param address: Initial slave address (7-bit, 0-127)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SlaveInit(uint8_t address);

/**
 * @brief Set the I2C slave address
 * @param address: 7-bit slave address (0-127)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SetSlaveAddress(uint8_t address);

/**
 * @brief Set a register value
 * @param reg_addr: Register address (0 or 1)
 * @param value: Value to set
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SetRegisterValue(uint8_t reg_addr, uint8_t value);

/**
 * @brief Get a register value
 * @param reg_addr: Register address (0 or 1)
 * @param value: Pointer to store the value
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_GetRegisterValue(uint8_t reg_addr, uint8_t* value);

/**
 * @brief Print the current status of I2C slave registers
 */
void I2C_PrintSlaveStatus(void);


#endif /* INC_I2C_H_ */

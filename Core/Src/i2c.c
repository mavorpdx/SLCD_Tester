/*
 * i2c.c
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_i2c.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <stdint.h>

#include "utils.h"
#include "main.h"
#include "i2c.h"

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);

void MX_I2C1_Init(void);
void MX_I2C2_Init(void);

uint8_t i2c_rx_buffer[10];
uint8_t i2c_tx_buffer[10] = {0x55};  // Default response value

void I2C_Init(void)
{
    // Initialize I2C1 as slave with address 0x50 (example address)
    I2C_SlaveInit(0x50);

    // Initialize I2C2 as master (for PCA9534)
    MX_I2C2_Init();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
//  hi2c1.Init.ClockSpeed = 100000;  // Standard 100kHz I2C speed

//  hi2c1.Init.OwnAddress1 = I2C_DUTYCYCLE_2;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;


  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  // Configure Analog filter
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  // Configure Digital filter
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x20303E5D;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  // Configure Analog filter
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  // Configure Digital filter
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/*
 * I2C Slave Mode Register Access Implementation
 *
 * This implementation provides a register-based slave device with:
 * - Configurable slave address
 * - Two read/write registers
 * - Interrupt-driven operation
 */

#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Define register addresses
#define REG_0       0x00
#define REG_1       0x01
#define NUM_REGS    2

// I2C slave state machine states
typedef enum {
    I2C_SLAVE_IDLE,         // Waiting for address match
    I2C_SLAVE_REG_ADDR,     // Received register address
    I2C_SLAVE_REG_READ,     // Preparing to read from register
    I2C_SLAVE_REG_WRITE     // Writing to register
} I2C_SlaveState;

// Global variables for I2C slave operation
static uint8_t slave_registers[NUM_REGS] = {0x00, 0x00};  // Register values
static uint8_t current_reg_addr = 0;                      // Currently selected register
static I2C_SlaveState slave_state = I2C_SLAVE_IDLE;       // State machine state

// Buffer for receiving data
static uint8_t rx_buffer[2];  // One byte for reg address, one for data

/**
 * @brief Set the I2C slave address
 * @param address: 7-bit slave address (0-127)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SetSlaveAddress(uint8_t address) {
    // Validate address (7-bit addressing)
    if (address > 127) {
        if(DEBUG_I2C) printf("Error: Invalid slave address (must be 0-127)\n");
        return HAL_ERROR;
    }

    // Stop any ongoing I2C operations
    HAL_I2C_DeInit(&hi2c1);

    // Re-initialize with new address
    hi2c1.Init.OwnAddress1 = address << 1; // Shift left by 1 as per HAL requirement

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to reinitialize I2C with new address\n");
        return HAL_ERROR;
    }

    // Restart slave listening
    slave_state = I2C_SLAVE_IDLE;
    if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to enable listen mode\n");
        return HAL_ERROR;
    }

    if(DEBUG_I2C) printf("I2C slave address set to 0x%02X\n", address);
    return HAL_OK;
}

/**
 * @brief Set a register value
 * @param reg_addr: Register address (0 or 1)
 * @param value: Value to set
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SetRegisterValue(uint8_t reg_addr, uint8_t value) {
    if (reg_addr >= NUM_REGS) {
        if(DEBUG_I2C) printf("Error: Invalid register address %d\n", reg_addr);
        return HAL_ERROR;
    }

    slave_registers[reg_addr] = value;
    if(DEBUG_I2C) printf("Register 0x%02X set to 0x%02X\n", reg_addr, value);
    return HAL_OK;
}

/**
 * @brief Get a register value
 * @param reg_addr: Register address (0 or 1)
 * @param value: Pointer to store the value
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_GetRegisterValue(uint8_t reg_addr, uint8_t* value) {
    if (reg_addr >= NUM_REGS || value == NULL) {
        if(DEBUG_I2C) printf("Error: Invalid register address or NULL pointer\n");
        return HAL_ERROR;
    }

    *value = slave_registers[reg_addr];
    return HAL_OK;
}

/**
 * @brief Initialize I2C slave mode with register access
 * @param address: Initial slave address (7-bit, 0-127)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef I2C_SlaveInit(uint8_t address) {
    // Initialize I2C1 as slave with the provided address
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x2000090E;
    hi2c1.Init.OwnAddress1 = address << 1; // Shift left by 1 as per HAL requirement
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to initialize I2C slave\n");
        return HAL_ERROR;
    }

    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to configure analog filter\n");
        return HAL_ERROR;
    }

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to configure digital filter\n");
        return HAL_ERROR;
    }

    // Start listening for address match
    slave_state = I2C_SLAVE_IDLE;
    if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK) {
        if(DEBUG_I2C) printf("Error: Failed to enable listen mode\n");
        return HAL_ERROR;
    }

    if(DEBUG_I2C) printf("I2C slave initialized with address 0x%02X\n", address);
    return HAL_OK;
}

/**
 * @brief Print the current status of I2C slave registers
 */
void I2C_PrintSlaveStatus(void) {
    printf("\nI2C Slave Status:\n");
    printf("---------------------------\n");
    printf("Address: 0x%02X\n", (uint8_t)(hi2c1.Init.OwnAddress1 >> 1));
    printf("Register 0 (0x00): 0x%02X\n", slave_registers[0]);
    printf("Register 1 (0x01): 0x%02X\n", slave_registers[1]);
    printf("Current state: ");

    switch(slave_state) {
        case I2C_SLAVE_IDLE:
            printf("IDLE\n");
            break;
        case I2C_SLAVE_REG_ADDR:
            printf("REG_ADDR\n");
            break;
        case I2C_SLAVE_REG_READ:
            printf("REG_READ\n");
            break;
        case I2C_SLAVE_REG_WRITE:
            printf("REG_WRITE\n");
            break;
        default:
            printf("UNKNOWN\n");
    }
    printf("---------------------------\n");
}

/**
 * @brief Callback when address match event occurs
 * @param hi2c Pointer to I2C handle
 * @param TransferDirection Direction (read or write)
 * @param AddrMatchCode Address that matched
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode) {
    if (hi2c->Instance == I2C1) {
        if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
            // Master is writing to slave - expect register address first
            slave_state = I2C_SLAVE_REG_ADDR;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, rx_buffer, 1, I2C_FIRST_FRAME);
        } else {
            // Master is reading from slave - send current register value
            if (slave_state == I2C_SLAVE_REG_ADDR) {
                // If we already have a register address, use it
                slave_state = I2C_SLAVE_REG_READ;
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &slave_registers[current_reg_addr], 1, I2C_LAST_FRAME);
            } else {
                // Otherwise use register 0 as default
                slave_state = I2C_SLAVE_REG_READ;
                current_reg_addr = 0;
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &slave_registers[0], 1, I2C_LAST_FRAME);
            }
        }
    }
}


/**
 * @brief Callback when data is received
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        if (slave_state == I2C_SLAVE_REG_ADDR) {
            // We received the register address
            current_reg_addr = rx_buffer[0];

            // Validate register address
            if (current_reg_addr >= NUM_REGS) {
                current_reg_addr = 0; // Default to register 0 if invalid
            }

            // Now ready to receive data for this register
            slave_state = I2C_SLAVE_REG_WRITE;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, rx_buffer, 1, I2C_LAST_FRAME);
        }
        else if (slave_state == I2C_SLAVE_REG_WRITE) {
            // We received data to write to the register
            slave_registers[current_reg_addr] = rx_buffer[0];

            if (DEBUG_I2C) printf("I2C Slave: Register 0x%02X written with value 0x%02X\n",
                                current_reg_addr, slave_registers[current_reg_addr]);

            // Reset state machine
            slave_state = I2C_SLAVE_IDLE;
            HAL_I2C_EnableListen_IT(hi2c);
        }
    }
}

/*
// Callback when Master reads data from the Slave
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    // Restart transmission if needed
    HAL_I2C_Slave_Transmit_IT(&hi2c1, i2c_tx_buffer, sizeof(i2c_tx_buffer));
}
*/

/**
 * @brief Callback when data is transmitted
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        // Transmission complete, reset state
        slave_state = I2C_SLAVE_IDLE;
        HAL_I2C_EnableListen_IT(hi2c);

        if (DEBUG_I2C) printf("I2C Slave: Register 0x%02X read, value 0x%02X\n",
                            current_reg_addr, slave_registers[current_reg_addr]);
    }
}

/**
 * @brief Callback when listen mode completes (stop condition)
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        // Transaction completed (STOP condition), restart listening
        slave_state = I2C_SLAVE_IDLE;
        HAL_I2C_EnableListen_IT(hi2c);
    }
}

/*
// Handle Error
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    // Restart communication in case of an error
    HAL_I2C_DeInit(&hi2c1);
    MX_I2C1_Init();
    HAL_I2C_Slave_Receive_IT(&hi2c1, i2c_rx_buffer, sizeof(i2c_rx_buffer));
}
*/

/**
 * @brief Error handling callback
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        // Reset and restart on error
        slave_state = I2C_SLAVE_IDLE;
        HAL_I2C_DeInit(hi2c);
        HAL_I2C_Init(hi2c);
        HAL_I2C_EnableListen_IT(hi2c);

        if (DEBUG_I2C) printf("I2C Slave: Error occurred, restarting\n");
    }
}



































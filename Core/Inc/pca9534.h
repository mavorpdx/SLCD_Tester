#ifndef PCA9534_H
#define PCA9534_H

#include "stm32f0xx_hal.h"

#include "utils.h"
#include <stdint.h>


#define PCA9534_I2C_ADDRESS  (0x38 << 1)

/* Return status values */
typedef enum {
    PCA9534_OK = 0,
    PCA9534_ERROR = 1,
} PCA9534_StatusTypeDef;

/* Device context structure */
typedef struct {
    I2C_HandleTypeDef *hi2c;  // Pointer to the HAL I2C handle
    uint16_t DevAddress;      // I2C address of the PCA9534 (7-bit address shifted left by 1)
} PCA9534_HandleTypeDef;

/* PCA9534 register addresses */
#define PCA9534_REG_INPUT       0x00
#define PCA9534_REG_OUTPUT      0x01
#define PCA9534_REG_POLARITY    0x02
#define PCA9534_REG_CONFIG      0x03

/* PCA9534 pin definitions (GP0 - GP7) */
#define BLON  			(1U << 0)
#define BRITE  			(1U << 1)
#define ERG_PWM  		(1U << 2)
#define PWM_EXT  		(1U << 3)
#define Vin_Vinv_EN  	(1U << 4)
#define Vin_Main_EN  	(1U << 5)
#define V5_Vinv_EN  	(1U << 6)
#define V5_VMain_EN		(1U << 7)


PCA9534_StatusTypeDef PCA9534_TogglePin(PCA9534_HandleTypeDef *hpca9534, uint8_t pin);

/**
 * @brief  Initializes the PCA9534 device context.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  hi2c: Pointer to the I2C handle.
 * @param  DevAddress: 7-bit I2C address of the PCA9534 shifted left by 1.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_Init(PCA9534_HandleTypeDef *hpca9534, I2C_HandleTypeDef *hi2c, uint16_t DevAddress);

/**
 * @brief  Write to the configuration register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  config: Bitmask for the configuration (1 = input, 0 = output).
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_SetConfig(PCA9534_HandleTypeDef *hpca9534, uint8_t config);

/**
 * @brief  Read the configuration register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  config: Pointer to store the configuration value.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_GetConfig(PCA9534_HandleTypeDef *hpca9534, uint8_t *config);

/**
 * @brief  Write to the output port register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  value: Value to write to the output port.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_WriteOutput(PCA9534_HandleTypeDef *hpca9534, uint8_t value);

/**
 * @brief  Read the input port register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  value: Pointer to store the input port value.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_ReadInput(PCA9534_HandleTypeDef *hpca9534, uint8_t *value);

/**
 * @brief  Write to the polarity inversion register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  polarity: Bitmask for the polarity inversion.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_SetPolarity(PCA9534_HandleTypeDef *hpca9534, uint8_t polarity);

/**
 * @brief  Read the polarity inversion register.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  polarity: Pointer to store the polarity value.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 */
PCA9534_StatusTypeDef PCA9534_GetPolarity(PCA9534_HandleTypeDef *hpca9534, uint8_t *polarity);

/**
 * @brief  Write an individual pin of the output port.
 * @param  hpca9534: Pointer to PCA9534 handle.
 * @param  pin: One of the PCA9534_GPx definitions (e.g., PCA9534_GP0).
 * @param  value: 0 to drive low, non-zero to drive high.
 * @retval PCA9534_OK on success, PCA9534_ERROR on failure.
 *
 * @note Before using this function, ensure the desired pin(s) are configured as outputs
 *       (i.e., corresponding bit(s) in the configuration register should be 0).
 */
PCA9534_StatusTypeDef PCA9534_WritePin(PCA9534_HandleTypeDef *hpca9534, uint8_t pin, uint8_t value);

#endif  // PCA9534_H

#include "pca9534.h"

/* Helper function to write a value to a PCA9534 register. */
static PCA9534_StatusTypeDef PCA9534_WriteReg(PCA9534_HandleTypeDef *hpca9534, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    
    if (HAL_I2C_Master_Transmit(hpca9534->hi2c, hpca9534->DevAddress, buf, 2, HAL_MAX_DELAY) != HAL_OK)
    {
        return PCA9534_ERROR;
    }
    return PCA9534_OK;
}

/* Helper function to read a value from a PCA9534 register. */
static PCA9534_StatusTypeDef PCA9534_ReadReg(PCA9534_HandleTypeDef *hpca9534, uint8_t reg, uint8_t *value)
{
    /* Send the register address */
    if (HAL_I2C_Master_Transmit(hpca9534->hi2c, hpca9534->DevAddress, &reg, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return PCA9534_ERROR;
    }
    /* Receive the register value */
    if (HAL_I2C_Master_Receive(hpca9534->hi2c, hpca9534->DevAddress, value, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return PCA9534_ERROR;
    }
    return PCA9534_OK;
}

PCA9534_StatusTypeDef PCA9534_Init(PCA9534_HandleTypeDef *hpca9534, I2C_HandleTypeDef *hi2c, uint16_t DevAddress)
{
    if (hpca9534 == NULL || hi2c == NULL)
    {
        return PCA9534_ERROR;
    }
    hpca9534->hi2c = hi2c;
    hpca9534->DevAddress = DevAddress;
    /* Additional initialization can be performed here if needed */
    return PCA9534_OK;
}

/**
 * @brief  Toggle an individual pin in the output register.
 */
PCA9534_StatusTypeDef PCA9534_TogglePin(PCA9534_HandleTypeDef *hpca9534, uint8_t pin)
{
    uint8_t currentOutput;

    /* Read the current state of the output register */
    if (PCA9534_ReadReg(hpca9534, PCA9534_REG_OUTPUT, &currentOutput) != PCA9534_OK)
    {
        return PCA9534_ERROR;
    }

    /* Toggle the specified pin by XORing with the pin mask */
    currentOutput ^= pin;

    /* Write the new output state */
    return PCA9534_WriteReg(hpca9534, PCA9534_REG_OUTPUT, currentOutput);
}

PCA9534_StatusTypeDef PCA9534_SetConfig(PCA9534_HandleTypeDef *hpca9534, uint8_t config)
{
    return PCA9534_WriteReg(hpca9534, PCA9534_REG_CONFIG, config);
}

PCA9534_StatusTypeDef PCA9534_GetConfig(PCA9534_HandleTypeDef *hpca9534, uint8_t *config)
{
    return PCA9534_ReadReg(hpca9534, PCA9534_REG_CONFIG, config);
}

PCA9534_StatusTypeDef PCA9534_WriteOutput(PCA9534_HandleTypeDef *hpca9534, uint8_t value)
{
    return PCA9534_WriteReg(hpca9534, PCA9534_REG_OUTPUT, value);
}

PCA9534_StatusTypeDef PCA9534_ReadInput(PCA9534_HandleTypeDef *hpca9534, uint8_t *value)
{
    return PCA9534_ReadReg(hpca9534, PCA9534_REG_INPUT, value);
}

PCA9534_StatusTypeDef PCA9534_SetPolarity(PCA9534_HandleTypeDef *hpca9534, uint8_t polarity)
{
    return PCA9534_WriteReg(hpca9534, PCA9534_REG_POLARITY, polarity);
}

PCA9534_StatusTypeDef PCA9534_GetPolarity(PCA9534_HandleTypeDef *hpca9534, uint8_t *polarity)
{
    return PCA9534_ReadReg(hpca9534, PCA9534_REG_POLARITY, polarity);
}

/**
 * @brief  Write an individual pin in the output register.
 */
PCA9534_StatusTypeDef PCA9534_WritePin(PCA9534_HandleTypeDef *hpca9534, uint8_t pin, uint8_t value)
{
    uint8_t currentOutput;
    /* Read the current state of the output register */
    if (PCA9534_ReadReg(hpca9534, PCA9534_REG_OUTPUT, &currentOutput) != PCA9534_OK)
    {
        return PCA9534_ERROR;
    }
    
    if (value)
    {
        currentOutput |= pin;   // Set the specified pin high.
    }
    else
    {
        currentOutput &= ~pin;  // Clear the specified pin (drive low).
    }
    
    return PCA9534_WriteReg(hpca9534, PCA9534_REG_OUTPUT, currentOutput);
}

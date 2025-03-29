/*
 * gpio.c
 *
 *  Created on: Feb 7, 2025
 *      Author: mavorpdx
 */
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "utils.h"
#include "gpio.h"
#include "pca9534.h"
#include "i2c.h"
#include "command.h"

extern uint32_t previous_led_millis;

// Global timing variables
bool led_state = false;

// 232=0, 485=1, TTL=2
extern uint8_t serialCFG;


// Initialize I2C2 as master for the PCA9534
PCA9534_HandleTypeDef hPCA;

// Static configurations for all GPIO pins including PCA9534
static const GPIO_PinConfig mcu_gpio_configs[] = {
    // GPIOA outputs
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOA,
        .GPIO_Pin = GPIO_PIN_5,
        .name = "SER1_RS232_EN"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOC,
        .GPIO_Pin = GPIO_PIN_12,
        .name = "SER2_RS232_EN"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOC,
        .GPIO_Pin = GPIO_PIN_0,
        .name = "RS232_5_OEN"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOC,
        .GPIO_Pin = GPIO_PIN_6,
        .name = "RS232_5_RTS"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOC,
        .GPIO_Pin = GPIO_PIN_7,
        .name = "RS485_4_DE"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOC,
        .GPIO_Pin = GPIO_PIN_8,
        .name = "RS485_4_REN"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOA,
        .GPIO_Pin = GPIO_PIN_8,
        .name = "J7_RST"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOA,
        .GPIO_Pin = GPIO_PIN_10,
        .name = "COM2_RSTn"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOA,
        .GPIO_Pin = GPIO_PIN_12,
        .name = "5V_VMAIN_PG"
    },
	{
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOA,
        .GPIO_Pin = GPIO_PIN_15,
        .name = "P105"
    },
    // GPIOB outputs
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOB,
        .GPIO_Pin = GPIO_PIN_1,
        .name = "LED1"
    },
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = GPIOB,
        .GPIO_Pin = GPIO_PIN_5,
        .name = "LED2"
    },
    // Terminator
    {
        .type = GPIO_TYPE_MCU,
        .GPIOx = NULL,
        .GPIO_Pin = 0,
        .name = {0}
    }
};

static const GPIO_PinConfig pca_gpio_configs[] = {
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = BLON,
        .name = "BLON"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = BRITE,
        .name = "BRITE"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = ERG_PWM,
        .name = "ERG_PWM"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = PWM_EXT,
        .name = "PWM_EXT"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = Vin_Vinv_EN,
        .name = "VIN_INV_EN"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = Vin_Main_EN,
        .name = "VIN_MAIN_EN"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = V5_Vinv_EN,
        .name = "V5_INV_EN"
    },
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = V5_VMain_EN,
        .name = "V5_MAIN_EN"
    },
    // Terminator
    {
        .type = GPIO_TYPE_PCA9534,
        .PCA9534_Pin = 0,
        .name = {0}
    }
};


// Static configurations for all input GPIO pins
static const GPIO_InputConfig mcu_input_configs[] = {
    // Power monitoring inputs
	{
		.GPIOx = GPIOA,
		.GPIO_Pin = VIN_VINV_PG_PIN,
		.name = "VIN_VINV_PG",
		.description = "Inverter input power good",
		.activeState = GPIO_PIN_SET    // Active high - high means power is good
	},
    {
        .GPIOx = GPIOB,
        .GPIO_Pin = VIN_VMAIN_PG_PIN,
        .name = "VIN_VMAIN_PG",
        .description = "Main input power good",
        .activeState = GPIO_PIN_SET
    },
    {
        .GPIOx = GPIOB,
        .GPIO_Pin = V5_VINV_PG_PIN,
        .name = "V5_VINV_PG",
        .description = "5V inverter power good",
        .activeState = GPIO_PIN_SET
    },
    {
        .GPIOx = GPIOA,
        .GPIO_Pin = V5_VMAIN_PG_PIN,
        .name = "V5_VMAIN_PG",
        .description = "5V main power good",
        .activeState = GPIO_PIN_SET
    },

    // Current monitoring inputs
    {
        .GPIOx = GPIOA,
        .GPIO_Pin = VIN_VINV_IMON_PIN,
        .name = "VIN_VINV_IMON",
        .description = "Inverter input current monitor",
        .activeState = GPIO_PIN_SET
    },
    {
        .GPIOx = GPIOA,
        .GPIO_Pin = VIN_VMAIN_IMON_PIN,
        .name = "VIN_VMAIN_IMON",
        .description = "Main input current monitor",
        .activeState = GPIO_PIN_SET
    },
    {
        .GPIOx = GPIOA,
        .GPIO_Pin = V5_VINV_IMON_PIN,
        .name = "V5_VINV_IMON",
        .description = "5V inverter current monitor",
   	    .activeState = GPIO_PIN_SET
    },
    {
        .GPIOx = GPIOC,
        .GPIO_Pin = V5_VMAIN_IMON_PIN,
        .name = "V5_VMAIN_IMON",
        .description = "5V main current monitor",
        .activeState = GPIO_PIN_SET
    },

    // Serial status inputs
    {
        .GPIOx = GPIOC,
        .GPIO_Pin = SER1_INVALIDn_PIN,
        .name = "SER1_INVALIDn",
        .description = "Serial port 1 invalid status (active low)",
   		.activeState = GPIO_PIN_RESET
    },
    {
        .GPIOx = GPIOC,
        .GPIO_Pin = SER2_INVALIDn_PIN,
        .name = "SER2_INVALIDn",
        .description = "Serial port 2 invalid status (active low)",
   		.activeState = GPIO_PIN_RESET
    },

    // Other inputs
    {
        .GPIOx = GPIOB,
        .GPIO_Pin = I2C_GPIO_INTn_PIN,
        .name = "I2C_GPIO_INTn",
        .description = "I2C GPIO interrupt (active low)",
   	    .activeState = GPIO_PIN_RESET
    },
    {
        .GPIOx = GPIOD,
        .GPIO_Pin = P104_PIN,
        .name = "P104",
        .description = "General purpose input pin",
        .activeState = GPIO_PIN_SET
    },

    // Terminator
    {
        .GPIOx = NULL,
        .GPIO_Pin = 0,
        .name = {0},
        .description = {0}
    }
};


/**
 * @brief Check if an input pin is in its active state
 * @param name: Name of the input pin to check
 * @return true if pin is in active state, false if not or if pin not found
 */
bool GPIO_IsInputActive(const char* name) {
    const GPIO_InputConfig* config = GPIO_FindInputByName(name);
    if (config == NULL) {
        if(DEBUG_GPIO) printf("Error: Unknown input '%s'\n", name);
        return false;
    }

    GPIO_PinState state = HAL_GPIO_ReadPin(config->GPIOx, config->GPIO_Pin);

    // Return true if the current state matches the active state
    return (state == config->activeState);
}

/**
 * @brief Find input pin configuration by name
 * @param name: Name of the input pin to find
 * @return Pointer to GPIO_InputConfig structure or NULL if not found
 */
const GPIO_InputConfig* GPIO_FindInputByName(const char* name) {
    if (name == NULL) return NULL;

    // Check input configs
    for (int i = 0; mcu_input_configs[i].name[0] != 0; i++) {
        if (strncasecmp(name, mcu_input_configs[i].name, strlen(mcu_input_configs[i].name)) == 0) {
            return &mcu_input_configs[i];
        }
    }

    return NULL;
}

/**
 * @brief Read the state of an input pin by name
 * @param name: Name of the input pin to read
 * @return GPIO_PIN_SET, GPIO_PIN_RESET, or -1 if not found
 */
int GPIO_ReadInputByName(const char* name) {
    const GPIO_InputConfig* config = GPIO_FindInputByName(name);
    if (config == NULL) {
        if(DEBUG_GPIO) printf("Error: Unknown input '%s'\n", name);
        return -1;
    }

    return HAL_GPIO_ReadPin(config->GPIOx, config->GPIO_Pin);
}

/**
 * @brief Print the state of an input pin by name
 * @param name: Name of the input pin to print
 * @return HAL_OK or HAL_ERROR if pin not found
 */
HAL_StatusTypeDef GPIO_PrintInputByName(char *buffer, const char* name) {
    const GPIO_InputConfig* config = GPIO_FindInputByName(name);
    if (config == NULL) {
    	sprintf(tStr, "Error: Unknown input '%s'\n", name);
    	strcat(buffer, tStr);
        return HAL_ERROR;
    }

    GPIO_PinState state = HAL_GPIO_ReadPin(config->GPIOx, config->GPIO_Pin);
    sprintf(tStr, "%-15s : %s (%s)\n", config->name, state == GPIO_PIN_SET ? "HIGH" : "LOW", config->description);
    //printf(">>>%s\n\r", tStr);
    strcat(buffer, tStr);
    return HAL_OK;
}

/**
 * @brief Print all input pin states
 */
void GPIO_PrintInputStates(char *buffer) {
    strcat(buffer, "Input GPIO States:\n");
    strcat(buffer, "------------------------------\n");

    // Print input states
    for (int i = 0; mcu_input_configs[i].name[0] != 0; i++) {
        GPIO_PrintInputByName(buffer, mcu_input_configs[i].name);
    }
    strcat(buffer, "\n");
}

/**
 * @brief Set the state of any GPIO pin (MCU or PCA9534)
 * @param config: Pointer to GPIO_PinConfig structure
 * @param state: Desired state (GPIO_PIN_SET or GPIO_PIN_RESET)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef GPIO_SetPin(const GPIO_PinConfig* config, GPIO_PinState state) {
    if (config == NULL) {
        return HAL_ERROR;
    }

    switch (config->type) {
        case GPIO_TYPE_MCU:
            if (config->GPIOx == NULL) {
                return HAL_ERROR;
            }
            HAL_GPIO_WritePin(config->GPIOx, config->GPIO_Pin, state);
            return HAL_OK;

        case GPIO_TYPE_PCA9534:
            return (PCA9534_WritePin(&hPCA, config->PCA9534_Pin, state) == PCA9534_OK) ?
                   HAL_OK : HAL_ERROR;

        default:
            return HAL_ERROR;
    }
}

/**
 * @brief Read the state of any GPIO pin (MCU or PCA9534)
 * @param config: Pointer to GPIO_PinConfig structure
 * @param state: Pointer to store the read state
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef GPIO_GetPin(const GPIO_PinConfig* config, GPIO_PinState* state) {
    if (config == NULL || state == NULL) {
        return HAL_ERROR;
    }

    switch (config->type) {
        case GPIO_TYPE_MCU:
            if (config->GPIOx == NULL) {
                return HAL_ERROR;
            }
            *state = HAL_GPIO_ReadPin(config->GPIOx, config->GPIO_Pin);
            return HAL_OK;

        case GPIO_TYPE_PCA9534: {
            uint8_t currentInput;
            if (PCA9534_ReadInput(&hPCA, &currentInput) != PCA9534_OK) {
                return HAL_ERROR;
            }
            *state = (currentInput & config->PCA9534_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
            return HAL_OK;
        }

        default:
            return HAL_ERROR;
    }
}


/**
 * @brief Print detailed information about all GPIO pins
 * @param None
 * @return None
 */
void GPIO_PrintDetailedInfo(char *buffer) {
    GPIO_PinState state;
    char port_name[8];
    uint8_t pin_number;

    strcat(buffer, "\n======= GPIO DETAILED INFORMATION ==========\n");

    // Print MCU Outputs
    strcat(buffer, "\nMCU OUTPUTS:\n");
    sprintf(tStr, "%-15s | %-5s | %-10s | %-5s\n", "NAME", "PORT", "PIN", "STATE");
    strcat(buffer, tStr);
    strcat(buffer, "--------------------------------------------\n");

    for (int i = 0; mcu_gpio_configs[i].name[0] != 0; i++) {
        // Get port letter
        if (mcu_gpio_configs[i].GPIOx == GPIOA) strcpy(port_name, "GPIOA");
        else if (mcu_gpio_configs[i].GPIOx == GPIOB) strcpy(port_name, "GPIOB");
        else if (mcu_gpio_configs[i].GPIOx == GPIOC) strcpy(port_name, "GPIOC");
        else if (mcu_gpio_configs[i].GPIOx == GPIOD) strcpy(port_name, "GPIOD");
        else if (mcu_gpio_configs[i].GPIOx == GPIOF) strcpy(port_name, "GPIOF");
        else strcpy(port_name, "???");

        // Get pin number from pin mask
        pin_number = 0;
        uint32_t pin_mask = mcu_gpio_configs[i].GPIO_Pin;
        while (pin_mask > 1) {
            pin_mask = pin_mask >> 1;
            pin_number++;
        }

        // Get current state
        GPIO_GetPin(&mcu_gpio_configs[i], &state);

        sprintf(tStr, "%-15s | %-5s | PIN_%-6d | %s\n",
               mcu_gpio_configs[i].name,
               port_name,
               pin_number,
               state == GPIO_PIN_SET ? "HIGH" : "LOW");
        strcat(buffer, tStr);
    }

    // Print PCA9534 Outputs
    strcat(buffer, "\nPCA9534 OUTPUTS:\n");
    sprintf(tStr, "%-15s | %-10s  | %-5s\n", "NAME", "PIN", "STATE");
    strcat(buffer, tStr);
    strcat(buffer, "-------------------------------------\n");

    for (int i = 0; pca_gpio_configs[i].name[0] != 0; i++) {
        // Get pin number as a bit position (0-7)
        pin_number = 0;
        uint8_t pin_mask = pca_gpio_configs[i].PCA9534_Pin;
        while (pin_mask > 1) {
            pin_mask = pin_mask >> 1;
            pin_number++;
        }

        // Get current state
        GPIO_GetPin(&pca_gpio_configs[i], &state);

        sprintf(tStr, "%-15s | I2C_PIN_%-3d | %s\n",
               pca_gpio_configs[i].name,
               pin_number,
               state == GPIO_PIN_SET ? "HIGH" : "LOW");
        strcat(buffer, tStr);
    }

    // Print MCU Inputs
    strcat(buffer, "\nMCU INPUTS:\n");
    sprintf(tStr, "%-15s | %-5s | %-10s | %-5s        | %-20s\n", "NAME", "PORT", "PIN", "STATE", "DESCRIPTION");
    strcat(buffer, tStr);
    strcat(buffer, "-----------------------------------------------------------------------------------\n");

    for (int i = 0; mcu_input_configs[i].name[0] != 0; i++) {
        // Get port letter
        if (mcu_input_configs[i].GPIOx == GPIOA) strcpy(port_name, "GPIOA");
        else if (mcu_input_configs[i].GPIOx == GPIOB) strcpy(port_name, "GPIOB");
        else if (mcu_input_configs[i].GPIOx == GPIOC) strcpy(port_name, "GPIOC");
        else if (mcu_input_configs[i].GPIOx == GPIOD) strcpy(port_name, "GPIOD");
        else if (mcu_input_configs[i].GPIOx == GPIOF) strcpy(port_name, "GPIOF");
        else strcpy(port_name, "???");

        // Get pin number from pin mask
        pin_number = 0;
        uint32_t pin_mask = mcu_input_configs[i].GPIO_Pin;
        while (pin_mask > 1) {
            pin_mask = pin_mask >> 1;
            pin_number++;
        }

        // Get current state
        GPIO_PinState current_state = HAL_GPIO_ReadPin(mcu_input_configs[i].GPIOx, mcu_input_configs[i].GPIO_Pin);

        // Check if pin is in active state
        bool is_active = (current_state == mcu_input_configs[i].activeState);

        sprintf(tStr, "%-15s | %-5s | PIN_%-6d | %s     | %s\n",
               mcu_input_configs[i].name,
               port_name,
               pin_number,
               is_active ? "ACTIVE  " : "INACTIVE",
               mcu_input_configs[i].description);
        strcat(buffer, tStr);
    }

    strcat(buffer, "\n===================================================================================\n");
}

// Helper function to find GPIO config by name
const GPIO_PinConfig* GPIO_FindByName(const char* name) {
    if (name == NULL) return NULL;

    // Check MCU GPIO configs
    for (int i = 0; mcu_gpio_configs[i].name[0] != 0; i++) {
        if (strncasecmp(name, mcu_gpio_configs[i].name, strlen(mcu_gpio_configs[i].name)) == 0) {
            return &mcu_gpio_configs[i];
        }
    }

    // Check PCA9534 configs
    for (int i = 0; pca_gpio_configs[i].name[0] != 0; i++) {
        if (strncasecmp(name, pca_gpio_configs[i].name, strlen(pca_gpio_configs[i].name)) == 0) {
            return &pca_gpio_configs[i];
        }
    }

    return NULL;
}

HAL_StatusTypeDef GPIO_SetOutputByName(const char* name, GPIO_PinState state) {
    const GPIO_PinConfig* config = GPIO_FindByName(name);
    if (config == NULL) {
        if(DEBUG_GPIO) printf("Error: Unknown GPIO '%s'\n", name);
        return HAL_ERROR;
    }
    return GPIO_SetPin(config, state);
}

HAL_StatusTypeDef GPIO_ToggleByName(const char* name) {
    const GPIO_PinConfig* config = GPIO_FindByName(name);
    if (config == NULL) {
        if(DEBUG_GPIO) printf("Error: Unknown GPIO '%s'\n", name);
        return HAL_ERROR;
    }

    if (config->type == GPIO_TYPE_MCU) {
        if(DEBUG_GPIO) printf("Tog %s\n", name);
        HAL_GPIO_TogglePin(config->GPIOx, config->GPIO_Pin);
        return HAL_OK;
    } else {
        // For PCA9534, we need to read-modify-write
        GPIO_PinState current_state;
        if (GPIO_GetPin(config, &current_state) != HAL_OK) {
            return HAL_ERROR;
        }
        return GPIO_SetPin(config, !current_state);
    }
}

void setSerialCFG(void){
	// UART CONFIGS
	//RS232_5_OEN = PC0
	//RS232_5_OEN = 0 enables TTL
	//RS232_5_OEN = 1 disables TTL

	//SER2_RS232_EN = PC12
	//SER2_RS232_EN = 1 enables 232
	//SER2_RS232_EN = 0 disables 232

	//RS485_4_DE = PC7
	//RS485_4_DE = 1 enables outputs
	//RS485_4_DE = 0 disables outputs

	//RS485_4_REN = PC8
	//RS485_4_REN = 1 disables inputs
	//RS485_4_REN = 0 enables inputs
	switch (serialCFG) {
	case 0: //232
		//RS232_5_OEN = 1 disables TTL
		GPIO_SetOutputByName("RS232_5_OEN", 1);
		//SER2_RS232_EN = 1 enables 232
		GPIO_SetOutputByName("SER2_RS232_EN", 1);
		//RS485_4_DE = 0 disables outputs
		GPIO_SetOutputByName("RS485_4_DE", 0);  //Disable RS-485 Receiver
		//RS485_4_REN = 1 disables inputs
		GPIO_SetOutputByName("RS485_4_REN", 1);  //Disable RS-485 Receiver
		if(DEBUG_GPIO) printf("ENABLE 232\n");
		break;

	case 1: //485
		//RS232_5_OEN = 1 disables TTL
		GPIO_SetOutputByName("RS232_5_OEN", 1);
		//SER2_RS232_EN = 1 enables 232
		GPIO_SetOutputByName("SER2_RS232_EN", 1);
		//RS485_4_DE = 0 disables outputs
		GPIO_SetOutputByName("RS485_4_DE", 0);
		//RS485_4_REN = 0 enables inputs
		GPIO_SetOutputByName("RS485_4_REN", 0);
		if(DEBUG_GPIO) printf("ENABLE 485\n");
		break;

	case 2: //TTL
		//RS232_5_OEN = 0 enables TTL
		GPIO_SetOutputByName("RS232_5_OEN", 0);
		//SER2_RS232_EN = 0 disables 232
		GPIO_SetOutputByName("SER2_RS232_EN", 0);
		//RS485_4_DE = 0 disables outputs
		GPIO_SetOutputByName("RS485_4_DE", 0);  //Disable RS-485 Receiver
		//RS485_4_REN = 1 disables inputs
		GPIO_SetOutputByName("RS485_4_REN", 1);  //Disable RS-485 Receiver
		if(DEBUG_GPIO) printf("ENABLE TTL\n");
		break;

	default:
		//RS232_5_OEN = 1 disables TTL
		GPIO_SetOutputByName("RS232_5_OEN", 1);
		//SER2_RS232_EN = 1 enables 232
		GPIO_SetOutputByName("SER2_RS232_EN", 1);
		//RS485_4_DE = 0 disables outputs
		GPIO_SetOutputByName("RS485_4_DE", 0);  //Disable RS-485 Receiver
		//RS485_4_REN = 1 disables inputs
		GPIO_SetOutputByName("RS485_4_REN", 1);  //Disable RS-485 Receiver
		sendDebug("SERCFG", "ERROR");
		if(DEBUG_GPIO) printf("Invalid - setting to RS-232\n");
	}
}


/**
 * @brief Initialize GPIO including PCA9534 I/O expander
 * @param None
 * @return None
 */
void GPIO_Init(void) {
    bool pca9534_ok = true;

    // Initialize MCU GPIOs
    MX_GPIO_Init();

    // Initialize PCA9534 on I2C2
    if(DEBUG_GPIO) printf("\nInitializing PCA9534...\n");
    if (PCA9534_Init(&hPCA, &hi2c2, PCA9534_I2C_ADDRESS) != PCA9534_OK) {
        if(DEBUG_GPIO) printf("Error: PCA9534 Init FAILED\n");
        pca9534_ok = false;
    }

    if (pca9534_ok) {
        // Configure ALL PCA9534 pins as outputs (0 = output)
        if (PCA9534_SetConfig(&hPCA, 0x00) != PCA9534_OK) {
            if(DEBUG_GPIO) printf("Error: PCA9534 Set Config FAILED\n");
            pca9534_ok = false;
        }
    }

    if (pca9534_ok) {
        // Initialize PCA9534 pins with default states
        for (int i = 0; pca_gpio_configs[i].name[0] != 0; i++) {
            uint8_t default_state = 0;
            // Set default states based on pin
            if (pca_gpio_configs[i].PCA9534_Pin == BRITE ||
                pca_gpio_configs[i].PCA9534_Pin == ERG_PWM ||
                pca_gpio_configs[i].PCA9534_Pin == PWM_EXT ||
                pca_gpio_configs[i].PCA9534_Pin == Vin_Vinv_EN) {
                default_state = 1;
            }

            if (PCA9534_WritePin(&hPCA, pca_gpio_configs[i].PCA9534_Pin, default_state) != PCA9534_OK) {
                if(DEBUG_GPIO) printf("Error: Failed to set %s\n", pca_gpio_configs[i].name);
                pca9534_ok = false;
            }
        }
    }

    // Initialize MCU GPIO outputs
    for (int i = 0; mcu_gpio_configs[i].name[0] != 0; i++) {
        GPIO_SetPin(&mcu_gpio_configs[i], GPIO_PIN_RESET);
    }

    setSerialCFG();

    GPIO_SetOutputByName("BLON", 1);
    GPIO_SetOutputByName("BRITE", 1);

    // Final status
    if(DEBUG_GPIO) printf("GPIO Init %s\n", pca9534_ok ? "OK" : "FAILED");
}

void GPIO_PrintStates(char *buffer) {
	strcat(buffer, "\nMCU GPIO States:\n");
	strcat(buffer, "------------------------------\n");

    // Print MCU GPIO states
    for (int i = 0; mcu_gpio_configs[i].name[0] != 0; i++) {
        GPIO_PinState state;
        if (GPIO_GetPin(&mcu_gpio_configs[i], &state) == HAL_OK) {
        	sprintf(tStr, "%-15s : %s\n", mcu_gpio_configs[i].name, state == GPIO_PIN_SET ? "HIGH" : "LOW");
            strcat(buffer, tStr);
        } else {
        	sprintf(tStr, "%-15s : ERROR\n", mcu_gpio_configs[i].name);
            strcat(buffer, tStr);
        }
    }

    strcat(buffer, "\nPCA9534 GPIO States:\n");
    strcat(buffer, "------------------------------\n");

    // Print PCA9534 GPIO states
    for (int i = 0; pca_gpio_configs[i].name[0] != 0; i++) {
        GPIO_PinState state;
        if (GPIO_GetPin(&pca_gpio_configs[i], &state) == HAL_OK) {
        	sprintf(tStr, "%-15s : %s\n", pca_gpio_configs[i].name,
                   state == GPIO_PIN_SET ? "HIGH" : "LOW");
            strcat(buffer, tStr);
        } else {
        	sprintf(tStr, "%-15s : ERROR\n", pca_gpio_configs[i].name);
            strcat(buffer, tStr);
        }
    }
}

void updateLEDStatus(void) {
    led_state = !led_state;
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, (led_state ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RS232_5_OEN_PIN|RS232_5_RTS_PIN|RS485_4_DE_PIN|RS485_4_REN_PIN |SER2_RS232_EN_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SER1_RS232_EN_PIN|J7_RST_PIN|COM2_RSTn_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pins : SER2_INVALIDn_PIN SER1_INVALIDn_PIN */
  GPIO_InitStruct.Pin = SER2_INVALIDn_PIN|SER1_INVALIDn_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RS232_5_OEN_PIN RS232_5_RTS_PIN RS485_4_DE_PIN RS485_4_REN_PIN SER2_RS232_EN_PIN */
  GPIO_InitStruct.Pin = RS232_5_OEN_PIN|RS232_5_RTS_PIN|RS485_4_DE_PIN|RS485_4_REN_PIN |SER2_RS232_EN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SER1_RS232_EN_PIN J7_RST_PIN COM2_RSTn_PIN */
  GPIO_InitStruct.Pin = SER1_RS232_EN_PIN|J7_RST_PIN|COM2_RSTn_PIN|P105_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_PIN LED2_PIN */
  GPIO_InitStruct.Pin = LED1_PIN|LED2_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : VIN_VMAIN_PG_PIN V5_VINV_PG_PIN I2C_GPIO_INTn_PIN */
  GPIO_InitStruct.Pin = VIN_VMAIN_PG_PIN|V5_VINV_PG_PIN|I2C_GPIO_INTn_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : VIN_VINV_PG_PIN V5_VMAIN_PG_PIN P105_PIN */
  GPIO_InitStruct.Pin = VIN_VINV_PG_PIN|V5_VMAIN_PG_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : P104_PIN */
  GPIO_InitStruct.Pin = P104_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(P104_GPIO_PORT, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}




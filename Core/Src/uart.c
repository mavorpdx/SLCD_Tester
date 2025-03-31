
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_uart.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include "main.h"
#include "uart.h"
#include "command.h"
#include "adc.h"

//SLCD5
//COM0 USART1  J6 8pin
//COM1 USART3  J7 10pin
//485  USART4  J7 10pin

//SLCD43
//COM0 USART1  J2  8pin up to 230,400 baud
//COM1 USART3  J1 10pin TTL ONLY
//COM2 USART5  J4 10pin
//COM3 USARTx  J3  5pin up to 460,800 baud

//SLCD6
//COM0 USART1  J6   8pin up to 230,400 baud
//COM1 USART3  JP1 10pin TTL Only
//COM2 USART5  J9   5pin
//COM3 USARTx  CN1  5pin up to 460,800 baud

uint32_t comBaud0 = SERIAL_BAUD;
uint32_t comBaud1 = SERIAL_BAUD;
uint32_t comBaud485 = SERIAL_BAUD;
uint32_t comBaud2 = SERIAL_BAUD;
uint32_t comBaud3 = USB_BAUD;


UART_HandleTypeDef huart1;  // UART1
UART_HandleTypeDef huart2;  // Debug UART
UART_HandleTypeDef huart3;  // UART3
UART_HandleTypeDef huart4;  // UART4 (RS485)
UART_HandleTypeDef huart5;  // UART5

char serialBuffer[UART_BUFFER_SIZE];
static uint8_t bufferIndex = 0;

// ******************************************************************
/* Buffer for received commands */
// ******************************************************************
char cmdBuffer[CMD_BUFFER_SIZE];
char lastCommand[LAST_CMD_BUFFER_SIZE] = {0};

// ******************************************************************
// Single byte receive buffers for interrupts
// ******************************************************************
uint8_t rxByte1, rxByte2, rxByte3, rxByte4, rxByte5;

UARTBuffer uart1Buffer = {0};
UARTBuffer uart2Buffer = {0};
UARTBuffer uart3Buffer = {0};
UARTBuffer uart4Buffer = {0};
UARTBuffer uart5Buffer = {0};


uint8_t serialCFG = 0;  //start as 232


// Function to change the baud rate of a specific UART
void ChangeBaudRate(UART_HandleTypeDef *huart, uint32_t baudRate) {
    if (huart == NULL) {
        return; // Invalid UART handle
    }
    huart->Init.BaudRate = baudRate;

    // Deinitialize and reinitialize UART to apply changes
    HAL_UART_DeInit(huart);
    if (HAL_UART_Init(huart) != HAL_OK) {
    	if(DEBUG_UART) printf("Baud change error - %lu\n", baudRate);
    }
}

// Functions to change the baud rate for each UART
void SetBaudRate_COM0(uint32_t baudRate) {
	sendReply("CONFIG", "Baud change COM0");
	MX_USART1_UART_Init();
}

void SetBaudRate_COM1(uint32_t baudRate) {
	sendReply("CONFIG", "Baud change COM1");
	MX_USART3_UART_Init();
}

void SetBaudRate_COM485(uint32_t baudRate) {
	sendReply("CONFIG", "Baud change COM485");
	MX_USART4_UART_Init();
}

void SetBaudRate_COM2(uint32_t baudRate) {
	sendReply("CONFIG", "Baud change COM2");
	MX_USART5_UART_Init();
}

void SetBaudRate_COM3(uint32_t baudRate) {
	sendReply("CONFIG", "Baud change COM3");
}


// ******************************************************************
// Add this function to handle received data
// ******************************************************************
void addToBuffer(UARTBuffer* buffer, uint8_t data) {
    uint16_t next = (buffer->head + 1) % UART_BUFFER_SIZE;
    if (next != buffer->tail) {  // Check if buffer is not full
        buffer->buffer[buffer->head] = data;
        buffer->head = next;
    }
}

// ******************************************************************
// UART interrupt callbacks
// ******************************************************************

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        //if(DEBUG_UART) printf("DBG: UART2 IRQ got byte: 0x%02X\n", rxByte2);  // Add this
        addToBuffer(&uart2Buffer, rxByte2);
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxByte2, 1);
    }
    else if (huart->Instance == USART1) {
//        if(DEBUG_UART) printf("DBG: UART1 IRQ got byte: 0x%02X\n", rxByte1);  // Add this
        addToBuffer(&uart1Buffer, rxByte1);
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rxByte1, 1);
    }
    else if (huart->Instance == USART3) {
//        if(DEBUG_UART) printf("DBG: UART3 IRQ got byte: 0x%02X\n", rxByte3);  // Add this
        addToBuffer(&uart3Buffer, rxByte3);
        HAL_UART_Receive_IT(&huart3, (uint8_t*)&rxByte3, 1);
    }
    else if (huart->Instance == USART4) {
//        if(DEBUG_UART) printf("DBG:4 IRQ got byte: 0x%02X\n", rxByte4);  // Add this
        addToBuffer(&uart4Buffer, rxByte4);
        HAL_UART_Receive_IT(&huart4, (uint8_t*)&rxByte4, 1);
    }
    else if (huart->Instance == USART5) {
//        if(DEBUG_UART) printf("DBG: UART5 IRQ got byte: 0x%02X\n", rxByte5);  // Add this
        addToBuffer(&uart5Buffer, rxByte5);
        HAL_UART_Receive_IT(&huart5, (uint8_t*)&rxByte5, 1);
    }
}

// ******************************************************************
// UART IRQs
// ******************************************************************
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void USART3_8_IRQHandler(void)
{

    if(huart3.Instance == USART3) {
       HAL_UART_IRQHandler(&huart3);
    }

    if(huart4.Instance == USART4) {
        HAL_UART_IRQHandler(&huart4);
    }


    if(huart5.Instance == USART5) {
        HAL_UART_IRQHandler(&huart5);
    }
}

// ******************************************************************
// Transmit Routines
// ******************************************************************
void UART_Transmit(UART_HandleTypeDef *huart, const char* str) {
    // Calculate string length
    size_t len = strlen(str);

    // Check if the string already ends with a newline
    bool has_newline = (len > 0 && str[len-1] == '\n');

    if (has_newline) {
        // String already has a newline, send as is
        HAL_UART_Transmit(huart, (uint8_t*)str, len, UART_TIMEOUT);
    } else {
        // String doesn't have a newline, create a temporary buffer
        char temp_buffer[UART_BUFFER_SIZE];

        // Make sure we don't overflow the buffer
        if (len < UART_BUFFER_SIZE - 1) {
            strcpy(temp_buffer, str);
            temp_buffer[len] = '\r';
            temp_buffer[len + 2] = '\0';

            //printf("UART %s -- %d\r\n", temp_buffer, len+1);
            HAL_UART_Transmit(huart, (uint8_t*)temp_buffer, len + 1, UART_TIMEOUT);
        } else {
            // String is too long, send without newline to avoid buffer overflow
            HAL_UART_Transmit(huart, (uint8_t*)str, len, UART_TIMEOUT);
        }
    }
}

void UART_TransmitBuffer(UART_HandleTypeDef *huart, uint8_t* data, uint16_t size) {
    HAL_UART_Transmit(huart, data, size, UART_TIMEOUT);
}


static uint8_t escapeSeq = 0;  // Track escape sequence state
//static uint8_t bufferIndex = 0;


// ******************************************************************
/* Handle all serial communications */
// ******************************************************************
void handleCommandPort(void) {

    while (uart2Buffer.tail != uart2Buffer.head) {
        uint8_t byte = uart2Buffer.buffer[uart2Buffer.tail];
        uart2Buffer.tail = (uart2Buffer.tail + 1) % UART_BUFFER_SIZE;

        if (byte == 0x1B) {
            escapeSeq = 1;  // ESC received
            continue;
        }
        else if (escapeSeq == 1 && byte == 0x5B) {
            escapeSeq = 2;  // '[' received after ESC
            continue;
        }
        else if (escapeSeq == 2 && byte == 0x41) {
            escapeSeq = 0;  // Up Arrow Key (ESC [ A)
            if (strlen(lastCommand) > 0) {
                strcpy(cmdBuffer, lastCommand);
                bufferIndex = strlen(lastCommand);
                if(DEBUG_UART) printf("%s", cmdBuffer); // Echo last command
            }
            continue;
        }
        escapeSeq = 0;  // Reset escape sequence tracking

        if (byte == '\r' || byte == '\n') {
            cmdBuffer[bufferIndex] = '\0'; // Null-terminate command string

            if (bufferIndex > 0) {
                strcpy(lastCommand, cmdBuffer);  // Store the command
            }

            // **Split into command and data**
            char *command = cmdBuffer;
            char *data = NULL;

            char *spacePtr = strchr(cmdBuffer, ' ');  // Find first space
            if (spacePtr != NULL) {
                *spacePtr = '\0';  // Terminate command at first space
                data = spacePtr + 1;  // Set data pointer to the next character

                // Trim trailing spaces in 'data'
                char *end = data + strlen(data) - 1;
                while (end > data && *end == ' ') {
                    *end = '\0';
                    end--;
                }

                // If 'data' is empty after trimming, set it to NULL
                if (*data == '\0') {
                    data = NULL;
                }
            }
            processSerialCommand(command, data);
            bufferIndex = 0;
        }
        else if (bufferIndex < CMD_BUFFER_SIZE - 1) {
            cmdBuffer[bufferIndex++] = byte;
        }
    }
}


static char uart1_msg[UART_BUFFER_SIZE];
static uint16_t uart1_idx = 0;

char uart3_msg[UART_BUFFER_SIZE] = {0};
uint16_t uart3_idx = 0;

char uart4_msg[UART_BUFFER_SIZE];
uint16_t uart4_idx = 0;

static char uart5_msg[UART_BUFFER_SIZE];
static uint16_t uart5_idx = 0;


void handleDataPorts(void) {
	// UART1
    if (uart1Buffer.tail != uart1Buffer.head) {
        uint8_t byte = uart1Buffer.buffer[uart1Buffer.tail];
        uart1Buffer.tail = (uart1Buffer.tail + 1) % UART_BUFFER_SIZE;

        // Skip NULL bytes
        if (byte == 0x00) {
        	return;
        }

        // Process line endings (CR or LF)
        if (byte == 0x0D || byte == 0x0A) {
            // Terminate string and process if we have content
            if (uart1_idx > 0) {
                uart1_msg[uart1_idx] = '\0';
                sendReply("COM0", uart1_msg);
                uart1_idx = 0;
            }
        }
        // Process regular characters
        else if (uart1_idx < UART_BUFFER_SIZE - 1) {
            uart1_msg[uart1_idx] = byte;
            uart1_idx++;
        }
    }

    // UART3
    if (uart3Buffer.tail != uart3Buffer.head) {
        uint8_t byte = uart3Buffer.buffer[uart3Buffer.tail];
        uart3Buffer.tail = (uart3Buffer.tail + 1) % UART_BUFFER_SIZE;

        // Skip NULL bytes
        if (byte == 0x00) {
        	return;
        }

        // Process line endings (CR or LF)
        if (byte == 0x0D || byte == 0x0A) {
            // Terminate string and process if we have content
            if (uart3_idx > 0) {
                uart3_msg[uart3_idx] = '\0';
                sendReply("COM1", uart3_msg);
                uart3_idx = 0;
            }
        }
        // Process regular characters
        else if (uart3_idx < UART_BUFFER_SIZE - 1) {
            uart3_msg[uart3_idx] = byte;
            uart3_idx++;
        }
    }

    // UART4
    if (uart4Buffer.tail != uart4Buffer.head) {
        uint8_t byte = uart4Buffer.buffer[uart4Buffer.tail];
        uart4Buffer.tail = (uart4Buffer.tail + 1) % UART_BUFFER_SIZE;

        // Skip NULL bytes
        if (byte == 0x00) {
        	return;
        }

        // Process line endings (CR or LF)
        if (byte == 0x0D || byte == 0x0A) {
            // Terminate string and process if we have content
            if (uart4_idx > 0) {
                uart4_msg[uart4_idx] = '\0';
                sendReply("COM485", uart4_msg);
                uart4_idx = 0;
            }
        }
        // Process regular characters
        else if (uart4_idx < UART_BUFFER_SIZE - 1) {
            uart4_msg[uart4_idx] = byte;
            uart4_idx++;
        }
    }

    // UART5
    if (uart5Buffer.tail != uart5Buffer.head) {
        uint8_t byte = uart5Buffer.buffer[uart5Buffer.tail];
        uart5Buffer.tail = (uart5Buffer.tail + 1) % UART_BUFFER_SIZE;

        // Skip NULL bytes
        if (byte == 0x00) {
            return;
        }

        // Process line endings (CR or LF)
        if (byte == 0x0D || byte == 0x0A) {
            // Terminate string and process if we have content
            if (uart5_idx > 0) {
                uart5_msg[uart5_idx] = '\0';
                sendReply("COM2", uart5_msg);
                uart5_idx = 0;
            }
        }
        // Process regular characters
        else if (uart5_idx < UART_BUFFER_SIZE - 1) {
            uart5_msg[uart5_idx] = byte;
            uart5_idx++;
        }
    }
}

void handleSerialCommunications(void) {
    handleCommandPort();  // Process UART2 commands
    handleDataPorts();    // Process other UART data
}


// ******************************************************************
// RS485 communication
// ******************************************************************
void handleRS485Communication(void) {
    static char rs485Buffer[64];
    static uint16_t bufferPos = 0;
    static uint32_t lastReceiveTime = 0;
    
    while (uart4Buffer.tail != uart4Buffer.head) {
        uint8_t byte = uart4Buffer.buffer[uart4Buffer.tail];
        uart4Buffer.tail = (uart4Buffer.tail + 1) % UART_BUFFER_SIZE;

        if (bufferPos < sizeof(rs485Buffer) - 1) {
            rs485Buffer[bufferPos++] = byte;
            lastReceiveTime = HAL_GetTick();
        }
    }
    
    // Process complete message after timeout
    if (bufferPos > 0 && (HAL_GetTick() - lastReceiveTime > RS485_TIMEOUT)) {
        rs485Buffer[bufferPos] = '\0';
        if(DEBUG_UART) printf("RS485 Received: %s\n", rs485Buffer);
        bufferPos = 0;
    }
}

void RS485_EnableTransmit(void) {
    HAL_GPIO_WritePin(RS485_4_DE_PORT, RS485_4_DE_PIN, GPIO_PIN_SET);    // DE high
    HAL_GPIO_WritePin(RS485_4_REN_PORT, RS485_4_REN_PIN, GPIO_PIN_SET);  // RE disabled
}

void RS485_EnableReceive(void) {
    HAL_GPIO_WritePin(RS485_4_DE_PORT, RS485_4_DE_PIN, GPIO_PIN_RESET);  // DE low
    HAL_GPIO_WritePin(RS485_4_REN_PORT, RS485_4_REN_PIN, GPIO_PIN_RESET);// RE enabled
}

void RS485_Transmit(const char* str) {
    RS485_EnableTransmit();
    UART_Transmit(&huart4, str);

    // Wait for transmission to complete
    while(__HAL_UART_GET_FLAG(&huart4, UART_FLAG_TC) == RESET) {}
    RS485_EnableReceive();
}

// ********************************************************
/* UART Initialization */
// ********************************************************
void UART_Init(void)
{
	if(DEBUG_UART) printf("UARTInit\n");

    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_USART4_UART_Init();
    MX_USART5_UART_Init();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Initialize RS485 control pins

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = RS485_4_DE_PIN | RS485_4_REN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_4_DE_PORT, &GPIO_InitStruct);

    // Set initial state to receive
    RS485_EnableReceive();
}

// ******************************************************************
//  * @brief USART1 Initialization Function
//  * @param None
//  * @retval None
// ******************************************************************
void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = comBaud0;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  HAL_UART_Receive_IT(&huart1, (uint8_t*)&rxByte1, 1);
}

// ******************************************************************
//  * @brief USART2 Initialization Function
//  * @param None
//  * @retval None
// ******************************************************************
void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = USB_BAUD2;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxByte2, 1);
}

// ******************************************************************
//  * @brief USART3 Initialization Function
//  * @param None
//  * @retval None
// ******************************************************************
void MX_USART3_UART_Init(void)
{

  // Initialize USART3 peripheral
  huart3.Instance = USART3;
  huart3.Init.BaudRate = comBaud1;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

  // Enable USART3 interrupt
  HAL_NVIC_SetPriority(USART3_8_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_8_IRQn);

  // Start receiving data in interrupt mode
  HAL_UART_Receive_IT(&huart3, (uint8_t*)&rxByte3, 1);
}

// ******************************************************************
//  * @brief USART4 Initialization Function
//  * @param None
//  * @retval None
// ******************************************************************
void MX_USART4_UART_Init(void)
{

  huart4.Instance = USART4;
  huart4.Init.BaudRate = comBaud485;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART3_8_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_8_IRQn);
  HAL_UART_Receive_IT(&huart4, (uint8_t*)&rxByte4, 1);

}

// ******************************************************************
//  * @brief USART5 Initialization Function
//  * @param None
//  * @retval None
// ******************************************************************
void MX_USART5_UART_Init(void)
{
  huart5.Instance = USART5;
  huart5.Init.BaudRate = comBaud2;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART3_8_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_8_IRQn);
  HAL_UART_Receive_IT(&huart5, (uint8_t*)&rxByte5, 1);

}

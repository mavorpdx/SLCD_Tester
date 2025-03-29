

#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_gpio.h"

#include "utils.h"
#include "gpio.h"


#define RS485_TIMEOUT 50
#define UART_TIMEOUT 1000

#define UART_BUFFER_SIZE 1024
#define CMD_BUFFER_SIZE 32
#define LAST_CMD_BUFFER_SIZE  CMD_BUFFER_SIZE

extern char cmdBuffer[CMD_BUFFER_SIZE];

typedef struct {
    uint8_t buffer[UART_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} UARTBuffer;


bool uart3_is_byte_available();
bool checkFlag3() ;

extern UART_HandleTypeDef huart1;  // UART1
extern UART_HandleTypeDef huart2;  // Debug UART
extern UART_HandleTypeDef huart3;  // UART3
extern UART_HandleTypeDef huart4;  // UART4 (RS485)
extern UART_HandleTypeDef huart5;  // UART5

#define SERIAL_BAUD 115200
#define SERIAL_BAUD_MAX 230400
#define USB_BAUD 460800
#define USB_BAUD2 921600

extern uint32_t comBaud0;
extern uint32_t comBaud1;
extern uint32_t comBaud485;
extern uint32_t comBaud2;
extern uint32_t comBaud3;

void ChangeBaudRate(UART_HandleTypeDef *huart, uint32_t baudRate);
void SetBaudRate_COM0(uint32_t baudRate);
void SetBaudRate_COM1(uint32_t baudRate);
void SetBaudRate_COM485(uint32_t baudRate);
void SetBaudRate_COM2(uint32_t baudRate);
void SetBaudRate_COM3(uint32_t baudRate);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void UART_Transmit(UART_HandleTypeDef *huart, const char* str);
void UART_TransmitBuffer(UART_HandleTypeDef *huart, uint8_t* data, uint16_t size);

void UART_Init(void);

void handleSerialCommunications(void);
void handleCommandPort(void);
void handleDataPorts(void);

void handleRS485Communication(void);
void RS485_EnableTransmit(void);
void RS485_EnableReceive(void);
void RS485_Transmit(const char* str);

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);
void MX_USART4_UART_Init(void);
void MX_USART5_UART_Init(void);

#endif /* __UART_H */

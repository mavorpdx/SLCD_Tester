/*
 * command.c
 *
 *  Created on: Feb 17, 2025
 *      Author: mavorpdx
 */
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

#include "command.h"
#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "uart.h"
#include "utils.h"
#include "main.h"

int debugFlag = 1;

//-----------------------------------------------------
// processSerialCommand()
// Very simple parser to handle commands entered via debug UART.
// ******************************************************************
void processSerialCommand(char* command, char* data) {
	char buffer[CMD_BUFFER_SIZE];
	char buff[3000] = {0};
	char temp[12];


	if(command[0] == 0x0a || command[0] == 0x0d){
		return;
	}

	if(command[0] < 0x30 || command[0] > 0x7a){
		return;
	}

  str2upper(command);  // Convert only command to uppercase
  if(DEBUG_CMD) sendDebug(command, data ? data : "");

  if (strncmp(command, "VERS", 4) == 0) {
	  //if(DEBUG_CMD)
	  sendDebug("Version", VERS_STRING);
  }

  // ******************************************************
  // LEDs
  // ******************************************************
  else if (strncmp(command, "LED1", 4) == 0) {
	  str2upper(data);
	  if (data && (strncmp(data, "ON", 2) == 0 || strncmp(data, "1", 1) == 0)) {
		  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);
		  sendDebug("LED1 turned ON", "");
	  }
	  else if (data && (strncmp(data, "OFF", 3) == 0 || strncmp(data, "0", 1) == 0)) {
		  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
		  sendDebug("LED1 turned OFF", "");
	  }
  }
  else if (strncmp(command, "LED2", 4) == 0) {
	  str2upper(data);
	  if (data && (strncmp(data, "ON", 2) == 0 || strncmp(data, "1", 1) == 0)) {
		  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET);
		  sendDebug("LED2 turned ON", "");
	  }
	  else if (data && (strncmp(data, "OFF", 3) == 0 || strncmp(data, "0", 1) == 0)) {
		  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);
		  sendDebug("LED2 turned OFF", "");
	  }
  }

  // ******************************************************
  // ADCs
  // ******************************************************
  else if (strncmp(command, "ADC_RAW", 7) == 0) {
	  printADCRaw(buffer);
	  sendReply("ADC_RAW", buffer);
  }
  else if (strncmp(command, "ADC_INVERTER", 12) == 0) {
	  sprintf(buffer, "%u", (unsigned int) getADC_Calculated_Inverter());
	  sendReply("ADC_INVERTER", buffer);
  }
  else if (strncmp(command, "ADC_VOLT3V3", 11) == 0) {
	  unsigned int foo = (unsigned int) getADC_Calculated_3V3();
	  sprintf(buffer, "%u", foo);
	  printf("!!%u!!", foo);
	  sendReply("ADC_VOLT3V3", buffer);
  }
  else if (strncmp(command, "ADC_VOLT5V", 10) == 0) {
	  sprintf(buffer, "%u", (unsigned int) getADC_Calculated_5V_J13());
	  sendReply("ADC_VOLT5V", buffer);
  }
  else if (strncmp(command, "ADC_INVJ2", 9) == 0) {
	  sprintf(buffer, "%u", (unsigned int) getADC_Calculated_Inv_J2());
	  sendReply("ADC_INVJ2", buffer);
  }
  else if (strncmp(command, "ADC_MAIN", 8) == 0) {
	  sprintf(buffer, "%u", (unsigned int) getADC_Calculated_Main_J2());
	  sendReply("ADC_MAIN", buffer);
  }
  else if (strncmp(command, "ADC", 3) == 0) {
	  printADCCalc(buff);
	  sendReply("ADC", buffer);
  }

  // ******************************************************
  // I2Cs
  // ******************************************************
  else if (strncmp(command, "I2C_SLAVE_ADDR", 14) == 0) {
      if (data) {
        uint8_t addr = (uint8_t)str2num(data);
        if (I2C_SetSlaveAddress(addr) == HAL_OK) {
          sprintf(buffer, "0x%02X", addr);
          sendReply("I2C_SLAVE_ADDR", buffer);
        } else {
          sendReply("I2C_SLAVE_ADDR", "ERROR");
        }
      } else {
        sendReply("I2C_SLAVE_ADDR", "Missing address");
      }
    }
    else if (strncmp(command, "I2C_REG_SET", 11) == 0) {
      // Parse format: <reg_addr> <value>
      if (data) {
        char* token = strtok(data, " ");
        if (token != NULL) {
          uint8_t reg_addr = (uint8_t)str2num(token);
          token = strtok(NULL, " ");
          if (token != NULL) {
            uint8_t value = (uint8_t)str2num(token);
            if (I2C_SetRegisterValue(reg_addr, value) == HAL_OK) {
              sprintf(buffer, "Reg 0x%02X = 0x%02X", reg_addr, value);
              sendReply("I2C_REG_SET", buffer);
            } else {
              sendReply("I2C_REG_SET", "ERROR");
            }
          } else {
            sendReply("I2C_REG_SET", "Missing value");
          }
        } else {
          sendReply("I2C_REG_SET", "Missing register address");
        }
      } else {
        sendReply("I2C_REG_SET", "Missing parameters");
      }
    }
    else if (strncmp(command, "I2C_REG_GET", 11) == 0) {
      if (data) {
        uint8_t reg_addr = (uint8_t)str2num(data);
        uint8_t value;
        if (I2C_GetRegisterValue(reg_addr, &value) == HAL_OK) {
          sprintf(buffer, "Reg 0x%02X = 0x%02X", reg_addr, value);
          sendReply("I2C_REG_GET", buffer);
        } else {
          sendReply("I2C_REG_GET", "ERROR");
        }
      } else {
        sendReply("I2C_REG_GET", "Missing register address");
      }
    }
    else if (strncmp(command, "I2C_STATUS", 10) == 0) {
      I2C_PrintSlaveStatus();
    }


  // ******************************************************
  // GPIOs
  // ******************************************************
  else if (strncmp(command, "GPIO_ALL", 8) == 0) {
    GPIO_PrintStates(buff);
  }
  else if (strncmp(command, "GPIODETAILS", 11) == 0) {
	  GPIO_PrintDetailedInfo(buff);
  }
  else if (strncmp(command, "TOGGLE", 6) == 0) {
    if (data) {
      if( (GPIO_ToggleByName(data)) == HAL_OK) {
    	  sendReply(data, "OK");
      }else{
    	  sendReply(data, "ERROR");
      }
    }
  }

  else if (strncmp(command, "SET", 3) == 0) {
    if (data) {
      if (GPIO_SetOutputByName(data, GPIO_PIN_SET) == HAL_OK) {
        sendReply(data, "OK");
      } else {
        sendReply(data, "ERROR");
      }
    } else {
      sendDebug("Usage: SET <pin_name>", "");
    }
  }

  else if (strncmp(command, "CLR", 3) == 0) {
    if (data) {
      if (GPIO_SetOutputByName(data, GPIO_PIN_RESET) == HAL_OK) {
        sendReply(data, "OK");
      } else {
        sendReply(data, "ERROR");
      }
    } else {
      sendDebug("Usage: CLR <pin_name>", "");
    }
  }

  else if (strncmp(command, "READ", 4) == 0) {
    if (data) {
      const GPIO_InputConfig* config = GPIO_FindInputByName(data);
      if (config != NULL) {
        bool isActive = GPIO_IsInputActive(data);
        sendReply(data, isActive ? "TRUE" : "FALSE");
      } else {
        sendReply(data, "ERROR");
        sendDebug("Error: Unknown input '%s'", data);
      }
    } else {
      sendDebug("Usage: READ <pin_name>", "");
    }
  }
  else if (strncmp(command, "INPUT_ALL", 9) == 0) {
    GPIO_PrintInputStates(buff);
    sendDebug("INPUT ALL", buff);
  }
  else if (strncmp(command, "INPUT", 5) == 0) {
    if (data) {
      GPIO_PrintInputByName(buff, data);
      sendReply(data, buff);
    } else {
      sendDebug("Usage: INPUT <pin_name>", "");
    }
  }


  // ******************************************************
  // UARTs
  // ******************************************************
  else if (strncmp(command, "COM0", 4) == 0) {
	if (data) {
		UART_Transmit(&huart1, data);
	}
      sendDebug("Sent to COM0", data);
  }
  else if (strncmp(command, "COM1", 4) == 0) {
    if (data) {
      UART_Transmit(&huart3, data);
      sendDebug("Sent to COM1", data);
    }
  }
  else if (strncmp(command, "COM485", 6) == 0) {
    if (data) {
      RS485_Transmit(data);
      sendDebug("Sent to COM485", data);
    }
  }
  else if (strncmp(command, "COM2", 4) == 0) {
    if (data) {
//    	sendDebug("%s", data);
      UART_Transmit(&huart5, data);
      sendDebug("Sent to COM2", data);
    }
  }
  else if (strncmp(command, "COM3", 4) == 0) {
    if (data) {
//@@@@      UART_Transmit(&huart5, data);
      sendDebug("Sent to COM3", data);
    }
  }
// ----- Baud Rates
  else if (strncmp(command, "BAUD0", 5) == 0) {
    if (data) {
      uint32_t b = str2num(data);
      if(b > SERIAL_BAUD_MAX) b = SERIAL_BAUD_MAX;
      if(b == 0) {
    	  sendDebug("Invalid Baud Rate","");
    	  return;
      }
      comBaud0 = b;
      SetBaudRate_COM0(b);
      itoa(b, temp, 10);
	  sendDebug("Baud0 changed", temp);
    }
    else {
    	char temp[12];
    	itoa(comBaud0, temp, 10);
    	sendReply("baud0", temp);
    }
  }
  else if (strncmp(command, "BAUD1", 5) == 0) {
    if (data) {
      uint32_t b = str2num(data);
      if(b > SERIAL_BAUD_MAX) b = SERIAL_BAUD_MAX;
      if(b == 0) {
    	  sendDebug("Invalid Baud Rate", "");
    	  return;
      }
      comBaud1 = b;
      SetBaudRate_COM1(b);
      itoa(comBaud1, temp, 10);
	  sendDebug("Baud1 changed", temp);
    }
    else {
    	itoa(comBaud1, temp, 10);
    	sendReply("baud1", temp);
    }
  }
  else if (strncmp(command, "BAUD2", 5) == 0) {
    if (data) {
      uint32_t b = str2num(data);
      if(b > SERIAL_BAUD_MAX) b = SERIAL_BAUD_MAX;
      if(b == 0) {
    	  sendDebug("Invalid Baud Rate", "");
    	  return;
      }
      comBaud2 = b;
      SetBaudRate_COM2(b);
      itoa(comBaud2, temp, 10);
	  sendDebug("Baud2 changed", temp);
    }
    else {
    	itoa(comBaud2, temp, 10);
    	sendReply("baud2", temp);
    }
  }
  else if (strncmp(command, "BAUD485", 7) == 0) {
    if (data) {
      uint32_t b = str2num(data);
      if(b > SERIAL_BAUD_MAX) b = SERIAL_BAUD_MAX;
      if(b == 0) {
    	  sendDebug("Invalid Baud Rate", "");
    	  return;
      }
      comBaud485 = b;
      SetBaudRate_COM485(b);
      itoa(comBaud485, temp, 10);
	  sendDebug("Baud485 changed", temp);
    }
    else {
    	itoa(comBaud485, temp, 10);
    	sendReply("baud485", temp);
    }
  }

  else if (strncmp(command, "BAUD3", 5) == 0) {
    if (data) {
      uint32_t b = str2num(data);
      if(b > USB_BAUD) b = USB_BAUD;
      if(b == 0) {
    	  sendDebug("Invalid Baud Rate", "");
    	  return;
      }
      comBaud3 = b;
      itoa(comBaud3, temp, 10);
	  sendDebug("Baud3 changed", temp);
    }
    else {
    	itoa(comBaud3, temp, 10);
    	sendReply("baud3", temp);
    }
  }

  else if (strncmp(command, "SERCFG", 6) == 0) {
	if (data) {
		int num = str2num(data);
		serialCFG = num;
		setSerialCFG();
        itoa(num, temp, 10);
	    sendDebug("Serial Cfg", temp);
	} else {
        itoa(serialCFG, temp, 10);
		sendDebug("Serial Cfg", temp);
	}
  }

  // ******************************************************
  // System
  // ******************************************************
  else if (strncmp(command, "HELP", 4) == 0) {
    printHelp(buff);
    sendDebug("Help", buff);
  }
  else if (strncmp(command, "STATUS", 6) == 0) {
    printSystemStatus(buff);
    sendDebug("Status", buff);
  }


  else {
    if(command[0] != 0 && command[0] != 0x0a) {
      sendDebug(command, "Unknown command");
    }
  }
}

void sendReply(const char* from, const char* message)
{
    printf("{\"%s\" : \"%s\"}\n", from, message);
}

void sendDebug(const char* from, const char* message)
{
	if (debugFlag){
		printf("[\"%s\" : \"%s\"]\n", from, message);
	}
}

//-----------------------------------------------------
// printHelp()
// Print available commands via debug output (USART2)
void printHelp(char *buffer) {
    if (buffer == NULL) return;

    // Initialize buffer with empty string to enable concatenation
    buffer[0] = '\0';

    strcat(buffer, "\n===== Available Commands =====\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "ADC READ Read all ADC values\n");
    strcat(buffer, "ADC_RAW Read all raw values\n");
    strcat(buffer, "ADC_INVERTER Read Inverter voltage value\n");
    strcat(buffer, "ADC_VOLT3V3 Read 3.3V value\n");
    strcat(buffer, "ADC_VOLT5V Read 5.0v value\n");
    strcat(buffer, "ADC_INVJ2 Read Inverter J2 value\n");
    strcat(buffer, "ADC_MAIN Read Main value\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "GPIO_ALL Read all pins\n");
    strcat(buffer, "CLR <pin_name> Clear a named pin\n");
    strcat(buffer, "SET <pin_name> Set a named pin\n");
    strcat(buffer, "TOGGLE <pin_name> Toggle a named pin\n");
    strcat(buffer, "READ <pin_name> Read raw active state (TRUE/FALSE)\n");
    strcat(buffer, "INPUT_ALL Read all input pins\n");
    strcat(buffer, "INPUT <pin_name> Read a specific input pin\n");
    strcat(buffer, "GPIODETAILS Print Detailed Info on pins\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "I2C_SLAVE <value> Response value\n");
    strcat(buffer, "I2C_SLAVE_ADDR <addr> Set I2C slave address (0-127)\n");
    strcat(buffer, "I2C_REG_SET <reg> <val> Set register value\n");
    strcat(buffer, "I2C_REG_GET <reg> Get register value\n");
    strcat(buffer, "I2C_STATUS Show I2C slave status\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "LED1 ON/OFF Control LED1\n");
    strcat(buffer, "LED1 1/0 Control LED1\n");
    strcat(buffer, "LED2 ON/OFF Control LED2\n");
    strcat(buffer, "LED2 1/0 Control LED2\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "TXn <msg> Send message via COMn\n");
    strcat(buffer, "RS485 <msg> Send message via RS485\n");
    strcat(buffer, "BAUDx <rate> Change baud rate\n");
    strcat(buffer, "\n----------------------------------------------\n");
    strcat(buffer, "STATUS Show system status\n");
    strcat(buffer, "HELP Show this message\n");
}



//-----------------------------------------------------
// printSystemStatus()
// A simple function to print LED and ADC status
void printSystemStatus(char *buffer)
{
  strcat(buffer, "\n======= System Status ========\n");
  sprintf(tStr, "LED1: %s\n", (HAL_GPIO_ReadPin(LED1_GPIO_PORT, LED1_PIN) == GPIO_PIN_SET) ? "ON" : "OFF");
  strcat(buffer, tStr);
  sprintf(tStr, "LED2: %s\n", (HAL_GPIO_ReadPin(LED2_GPIO_PORT, LED2_PIN) == GPIO_PIN_SET) ? "ON" : "OFF");
  strcat(buffer, tStr);
  printADCCalc(buffer);
  GPIO_PrintStates(buffer);
  GPIO_PrintInputStates(buffer);
}

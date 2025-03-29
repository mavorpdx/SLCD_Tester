// ********************************************************
/* ADC Functions for STM32F091RC */

#ifndef __ADC_H_
#define __ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "utils.h"

// ADC pins
#define ADC_INVERTER_PIN GPIO_PIN_6
#define ADC_INVERTER_PORT GPIOA
#define ADC_3V3_PIN GPIO_PIN_7
#define ADC_3V3_PORT GPIOA
#define ADC_5V_J13_PIN GPIO_PIN_0
#define ADC_5V_J13_PORT GPIOB
#define ADC_INV_J2_PIN GPIO_PIN_2
#define ADC_INV_J2_PORT GPIOC
#define ADC_MAIN_J2_PIN GPIO_PIN_3
#define ADC_MAIN_J2_PORT GPIOC

/* Constants */
#define ADC_CHANNELS            5       // Total number of ADC channels
#define ADC_OVERSAMPLE_COUNT    3       // Number of readings to average
#define ADC_TIMEOUT            100      // ADC conversion timeout in ms
#define ADC_REF_VOLTAGE        3.3f    // Reference voltage
#define ADC_RESOLUTION        4096  // 12-bit ADC resolution

/* ADC channel definitions */
#define ADC_INVERTER_CHANNEL   ADC_CHANNEL_6   // PA6
#define ADC_3V3_CHANNEL        ADC_CHANNEL_7   // PA7
#define ADC_5V_J13_CHANNEL     ADC_CHANNEL_8   // PB0
#define ADC_INV_J2_CHANNEL     ADC_CHANNEL_12  // PC2
#define ADC_MAIN_J2_CHANNEL    ADC_CHANNEL_13  // PC3

extern ADC_HandleTypeDef hadc;     // ADC handle

// Number of ADC channels
#define ADC_CHANNEL_COUNT 5

// Array to store averaged ADC readings
extern uint16_t adc_raw_values[ADC_CHANNEL_COUNT];

// Array to store calculated ADC values
extern uint32_t adc_calculated_values[ADC_CHANNEL_COUNT];

/* Structure for ADC readings */
typedef struct {
    uint16_t inverter;
    uint16_t v3_3;
    uint16_t v5_J13;
    uint16_t inv_J2;
    uint16_t main_J2;
} ADCReadings;

// Raw (Averaged) Data Retrieval
uint16_t getADC_Raw_Inverter(void);
uint16_t getADC_Raw_3V3(void);
uint16_t getADC_Raw_5V_J13(void);
uint16_t getADC_Raw_Inv_J2(void);
uint16_t getADC_Raw_Main_J2(void);

// Processed Data Retrieval
uint32_t getADC_Calculated_Inverter(void);
uint32_t getADC_Calculated_3V3(void);
uint32_t getADC_Calculated_5V_J13(void);
uint32_t getADC_Calculated_Inv_J2(void);
uint32_t getADC_Calculated_Main_J2(void);


/* External references */
extern ADC_HandleTypeDef hadc;
extern void Error_Handler(void);
extern void debug_print(const char* str);
extern void debug_printf(const char* format, ...);

// Function prototypes
void readADCValues(void);
void processADCValues(void);

void MX_ADC_Init(void);
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);
void handleADCError(void);
void ADC_Init(void);
void printADCValues(char *buffer, bool raw);
void printADCRaw(char *buffer);
void printADCCalc(char *buffer);

//void configureADCChannels(void);
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc_ptr);
uint32_t convertToVoltage(uint16_t adc_value);
void calculatePower(const ADCReadings* readings);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H_ */

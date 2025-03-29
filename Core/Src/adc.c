// ********************************************************
/* ADC Functions for STM32F091RC */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_adc.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include "adc.h"
#include "main.h"
#include "command.h"

ADC_HandleTypeDef hadc;     // ADC handle

/* Static variables for ADC DMA */
static volatile uint16_t adc_dma_buffer[ADC_CHANNELS * ADC_OVERSAMPLE_COUNT];
static volatile uint8_t adc_conversion_complete = 0;

// ADC raw and processed values
uint16_t adc_raw_values[ADC_CHANNEL_COUNT] = {0};
uint32_t adc_calculated_values[ADC_CHANNEL_COUNT] = {0};

// ADC channel mapping
static const uint32_t adc_channels[ADC_CHANNEL_COUNT] = {
	    ADC_CHANNEL_12,  // ADC_V_INV_J2  (PC2)
	    ADC_CHANNEL_13,  // ADC_V_MAIN_J2  (PC3)
	    ADC_CHANNEL_6,   // ADC_V_INV_REG_12v  (PA6)
	    ADC_CHANNEL_7,   // ADC_3V3_PERI  (PA7)
	    ADC_CHANNEL_8    // ADC_5C_J13  (PB0)
};

#define ADC_V_INV_J2 25150
#define ADC_V_INV_J2DIV 3785

#define ADC_V_MAIN_J2 25150
#define ADC_V_MAIN_J2DIV 3785

#define ADC_V_INV_REG_12v 25150
#define ADC_V_INV_REG_12vDIV 3785

#define ADC_3V3_PERI 250
#define ADC_3V3_PERIDIV 157

#define ADC_5C_J13 250
#define ADC_5C_J13DIV 157



// Function to process ADC values using custom calculations
void processADCValues(void) {
	//printf("!");
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
    	switch (i)
    	{
    	case 0:
    		//if(DEBUG_ADC) printf("ADC[%d] raw=%d mult=%d div=%d\n", i, adc_raw_values[i], ADC_V_INV_J2, ADC_V_INV_J2DIV);
        	adc_calculated_values[0] = (adc_raw_values[0] * ADC_V_INV_J2) / ADC_V_INV_J2DIV;
    		break;

    	case 1:
    		//if(DEBUG_ADC) printf("ADC[%d] raw=%d mult=%d div=%d\n", i, adc_raw_values[i], ADC_V_MAIN_J2, ADC_V_MAIN_J2DIV);
        	adc_calculated_values[1] = (adc_raw_values[1] * ADC_V_MAIN_J2) / ADC_V_MAIN_J2DIV;
    		break;

    	case 2:
    		//if(DEBUG_ADC) printf("ADC[%d] raw=%d mult=%d div=%d\n", i, adc_raw_values[i], ADC_V_INV_REG_12v, ADC_V_INV_REG_12vDIV);
        	adc_calculated_values[2] = (adc_raw_values[2] * ADC_V_INV_REG_12v) / ADC_V_INV_REG_12vDIV;
    		break;

    	case 3:
    		//printf("ADC[%d] raw=%d mult=%d div=%d  calc=%d\n", i, adc_raw_values[i], ADC_3V3_PERI, ADC_3V3_PERIDIV, (adc_raw_values[3] * ADC_3V3_PERI) / ADC_3V3_PERIDIV);
        	adc_calculated_values[3] = (adc_raw_values[3] * ADC_3V3_PERI) / ADC_3V3_PERIDIV;
    		break;

    	case 4:
    		//if(DEBUG_ADC) printf("ADC[%d] raw=%d mult=%d div=%d\n", i, adc_raw_values[i], ADC_5C_J13, ADC_5C_J13DIV);
        	adc_calculated_values[4] = (adc_raw_values[4] * ADC_5C_J13) / ADC_5C_J13DIV;
    		break;

    	default:
    		if(DEBUG_ADC) printf("OOPS - ran over the ADC Case Count\n");
    		break;
    	}
    }
    //printf("@");
}

// Functions to get raw ADC values
uint16_t getADC_Raw_Inv_J2(void)    { return adc_raw_values[0]; }
uint16_t getADC_Raw_Main_J2(void)   { return adc_raw_values[1]; }
uint16_t getADC_Raw_Inverter(void)  { return adc_raw_values[2]; }
uint16_t getADC_Raw_3V3(void)       { return adc_raw_values[3]; }
uint16_t getADC_Raw_5V_J13(void)    { return adc_raw_values[4]; }

// Functions to get processed ADC values
uint32_t getADC_Calculated_Inverter(void)
{
	return adc_calculated_values[2];
}

uint32_t getADC_Calculated_3V3(void)
{
	return adc_calculated_values[3];
}

uint32_t getADC_Calculated_5V_J13(void)
{
	return adc_calculated_values[4];
}

uint32_t getADC_Calculated_Inv_J2(void)
{
	return adc_calculated_values[0];
}

uint32_t getADC_Calculated_Main_J2(void)
{
	return adc_calculated_values[1];
}


/* Convert ADC value to voltage */
uint32_t convertToVoltage(uint16_t adc_value) {
    return (adc_value * ADC_REF_VOLTAGE) / ADC_RESOLUTION;
}



void printADCRaw(char *buffer)
{
	strcat(buffer, "Raw ADC Values\n");
	strcat(buffer, "------------------------------\n");
	sprintf(tStr, "Raw INV_12V  = %d\n", adc_raw_values[2]);
	strcat(buffer, tStr);
	sprintf(tStr, "Raw 3V3_PERI = %d\n", adc_raw_values[3]);
	strcat(buffer, tStr);
	sprintf(tStr, "Raw VOLT5V0 = %d\n", adc_raw_values[4]);
	strcat(buffer, tStr);
	sprintf(tStr, "Raw INV_12V_J2 = %d\n", adc_raw_values[0]);
	strcat(buffer, tStr);
	sprintf(tStr, "Raw MAIN_J2  = %d\n", adc_raw_values[1]);
	strcat(buffer, tStr);
}

void printADCCalc(char *buffer)
{
	strcat(buffer, "ADC Values\n");
	strcat(buffer, "------------------------------\n");
	sprintf(tStr, "INV_12V  = %ld\n", getADC_Calculated_Inverter());
	strcat(buffer, tStr);
	sprintf(tStr, "3V3_PERI = %ld\n", getADC_Calculated_3V3());
	strcat(buffer, tStr);
	sprintf(tStr, "VOLT5V0 = %ld\n", getADC_Calculated_5V_J13());
	strcat(buffer, tStr);
	sprintf(tStr, "INV_12V_J2 = %ld\n", getADC_Calculated_Inv_J2());
	strcat(buffer, tStr);
	sprintf(tStr, "MAIN_5V_J2  = %ld\n", getADC_Calculated_Main_J2());
	strcat(buffer, tStr);
}


/* Print ADC values */
void printADCValues(char *buffer, bool raw)
{
    // Convert to millivolts instead of floating point
/*    uint32_t vInverter = (adc_values.inverter * ADC_REF_VOLTAGE * 1000) / ADC_RESOLUTION;
    uint32_t v3_3 = (adc_values.v3_3 * ADC_REF_VOLTAGE * 1000) / ADC_RESOLUTION;
    uint32_t v5_J13 = (adc_values.v5_J13 * ADC_REF_VOLTAGE * 1000) / ADC_RESOLUTION;
    uint32_t vInv_J2 = (adc_values.inv_J2 * ADC_REF_VOLTAGE * 1000) / ADC_RESOLUTION;
    uint32_t vMain_J2 = (adc_values.main_J2 * ADC_REF_VOLTAGE * 1000) / ADC_RESOLUTION;
    */

	if (raw) {
		printADCRaw(buffer);
	} else {
		printADCCalc(buffer);
	}
}


// ********************************************************
/* ADC Initialization */
void ADC_Init(void)
{
	if(DEBUG_ADC) printf("Starting ADC Init...\n");

    // Enable ADC and GPIO clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Enable HSI14 for ADC
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while(!(RCC->CR2 & RCC_CR2_HSI14RDY));

    // ADC Disable and wait
    ADC1->CR |= ADC_CR_ADDIS;
    while(ADC1->CR & ADC_CR_ADEN);

    // Reset configuration
    ADC1->CFGR1 = 0x00000000;
    ADC1->CFGR2 = 0x00000000;

    // Select HSI14 by clearing CKMODE
    ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE;

    // Enable voltage reference and temperature sensor
    ADC->CCR |= ADC_CCR_VREFEN;
    HAL_Delay(1);

    // Calibrate ADC
    ADC1->CR |= ADC_CR_ADCAL;
    uint32_t timeout = 100000;
    while((ADC1->CR & ADC_CR_ADCAL) && --timeout);
    if (timeout == 0) {
        sendDebug("ADC","Calibration timeout");
        return;
    }
    HAL_Delay(1);

    // Configure ADC
    // 12-bit resolution, right alignment, software trigger
    ADC1->CFGR1 &= ~(ADC_CFGR1_RES | ADC_CFGR1_ALIGN | ADC_CFGR1_EXTEN);

    // Set sampling time for all channels
    ADC1->SMPR = ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2;  // 239.5 ADC clock cycles

    // Enable ADC
    ADC1->CR |= ADC_CR_ADEN;
    timeout = 100000;
    while(!(ADC1->ISR & ADC_ISR_ADRDY) && --timeout);
    if (timeout == 0) {
    	sendDebug("ADC", "Enable timeout");
        return;
    }

    // Clear any pending flags
    ADC1->ISR |= (ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR);

    if(DEBUG_ADC) printf("ADC Initialization complete\n");

}

// Function to read ADC values and store the averaged results
void readADCValues(void) {
    uint32_t sums[ADC_CHANNEL_COUNT] = {0};

    for (int i = 0; i < 3; i++) {
    	//printf("I%d", i);
        for (int j = 0; j < ADC_CHANNEL_COUNT; j++) {
            // Clear any pending flags
            ADC1->ISR |= (ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR);

            // Select channel
            ADC1->CHSELR = (1U << adc_channels[j]);

            // Wait for channel selection to be effective
            while(ADC1->CFGR1 & ADC_CFGR1_WAIT);

            // Start the conversion
            ADC1->CR |= ADC_CR_ADSTART;

            // Wait for conversion with timeout
            uint32_t timeout = 100000;
            while(!(ADC1->ISR & ADC_ISR_EOC) && --timeout) {
                if (ADC1->ISR & ADC_ISR_OVR) {
                    printf("Overrun detected\n");
                    ADC1->ISR |= ADC_ISR_OVR;  // Clear overrun flag
                }
            }

            if (timeout > 0) {
                uint32_t value = ADC1->DR;
                sums[j] += value;
                //printf("J%d %ul | ", j, value);
            } else {
                if(DEBUG_ADC) printf("Conversion timeout - ISR: 0x%08lx, CR: 0x%08lx\n", ADC1->ISR, ADC1->CR);
            }

            // Stop conversion and wait
            ADC1->CR |= ADC_CR_ADSTP;
            while(ADC1->CR & ADC_CR_ADSTP);

            HAL_Delay(1);
        }
    }

    for (int j = 0; j < ADC_CHANNEL_COUNT; j++) {
        adc_raw_values[j] = sums[j] / 3;
    }
    processADCValues();
}



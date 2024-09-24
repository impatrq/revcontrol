//---------------------------------------------------------------//
// includes
//---------------------------------------------------------------//
#include <stdio.h>
#include <string.h> //para bluetooth
#include "fsl_usart.h" //para bluetooth
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_adc.h"
#include "fsl_power.h"
#include "fsl_swm.h"
#include "fsl_iocon.h"
#include "LPC845.h"
#include "fsl_debug_console.h"

//---------------------------------------------------------------//
// defines
//---------------------------------------------------------------//
#define ADC0_CH1		1 //Temp. cabeza de cilindro (CHT)
#define ADC0_CH2		2 //Temp. cabeza de cilindro (CHT)
#define ADC0_CH3		3 //Temp. cabeza de cilindro (CHT)
#define ADC0_CH4		4 //Temp. cabeza de cilindro (CHT)
#define ADC0_CH5		5 //Concentración de oxígeno
#define ADC0_CH6		6 //Temp. de aceite
#define ADC0_CH7		7 //Temp. de agua 1
#define ADC0_CH8		8 //Temp. de agua 2
#define ADC0_CH9		9 //Presión de aceite
#define ADC0_CH10       10 //RPM
#define ADC_FULL_RANGE 4095U // Rango del ADC
//---------------------------------------------------------------//
// Variables
//---------------------------------------------------------------//
long i;
int r;  //contador 
uint32_t count_mseg;
adc_result_info_t adcResultInfoStruct;
uint32_t frequency;
uint8_t adc_conv_complete, a = 0;
const float referenceVoltage = 3.3;     // Voltaje de referencia del ADC en voltios
uint8_t adc_channel[10] = {ADC0_CH1, ADC0_CH2, ADC0_CH3, ADC0_CH4, ADC0_CH5, ADC0_CH6, ADC0_CH7, ADC0_CH8, ADC0_CH9, ADC0_CH10};    //array de canales
uint16_t channel_result[10] = {};    //array de resultados de los canales
uint16_t termocuplas[4] = {};   //valores de las termocuplas almacenados
uint16_t lambda; //valor de sonda lambda
uint16_t termistores[3] = {}; //valores de los termistores almacenados
uint16_t temp_agua; //promedio de la temperatura del agua
uint16_t pressure;  //valor de presión
uint16_t RPM;   //valores de RPM almacenados
//---------------------------------------------------------------//
// Prototypes
//---------------------------------------------------------------//
void ADC_Configuration(void);
//---------------------------------------------------------------//
// main
//---------------------------------------------------------------//
int main(void) {

    BOARD_BootClockFRO30M();
    BOARD_InitDebugConsole();

    CLOCK_EnableClock(kCLOCK_Swm);

    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN1, true); //----------PIO0_6---------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN2, true); //----------PIO0_14--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN3, true); //----------PIO0_23--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN4, true); //----------PIO0_22--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN5, true); //----------PIO0_21--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN6, true); //----------PIO0_20--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN7, true); //----------PIO0_19--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN8, true); //----------PIO0_18--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN9, true); //----------PIO0_17--------//
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN10, true); //---------PIO0_13--------//

    CLOCK_DisableClock(kCLOCK_Swm);

    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1U);

    POWER_DisablePD(kPDRUNCFG_PD_ADC0);
    frequency = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
    (void) ADC_DoSelfCalibration(ADC0, frequency);
    
	adc_config_t adcConfigStruct;
    adc_conv_seq_config_t adcConvSeqConfigStruct;
    adcConfigStruct.clockMode = kADC_ClockSynchronousMode;
    adcConfigStruct.clockDividerNumber = 1;
    adcConfigStruct.enableLowPowerMode = false;
    adcConfigStruct.voltageRange = kADC_HighVoltageRange;
    ADC_Init(ADC0, &adcConfigStruct);
    
    adcConvSeqConfigStruct.channelMask = (1<<ADC0_CH1) | (1<<ADC0_CH2) | (1<<ADC0_CH3) | (1<<ADC0_CH4) | (1<<ADC0_CH5) | (1<<ADC0_CH6) | (1<<ADC0_CH7) | (1<<ADC0_CH8) | (1<<ADC0_CH9) | (1<<ADC0_CH10); 
    adcConvSeqConfigStruct.triggerMask = 0;
    adcConvSeqConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqConfigStruct.enableSingleStep = false;
    adcConvSeqConfigStruct.enableSyncBypass = false;
    adcConvSeqConfigStruct.interruptMode = kADC_InterruptForEachSequence;
    ADC_SetConvSeqAConfig(ADC0, &adcConvSeqConfigStruct);
    ADC_EnableConvSeqA(ADC0, true);
    ADC_EnableInterrupts(ADC0, kADC_ConvSeqAInterruptEnable);
    NVIC_EnableIRQ(ADC0_SEQA_IRQn);
	adc_conv_complete = 0;
	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    PRINTF("Holaaaa");
    while(1) {
        if(adc_conv_complete == true){
            for (r = 0; r < 4; r++){
                termocuplas[r] = channel_result[r] * (referenceVoltage / ADC_FULL_RANGE);
                if (r >= 0 && r <= 3){
                    PRINTF("Temperatura de cabeza de cilindro: %d\r\n °C, y su valor de ADC es: %d\r\n", temperature[r], channel_result[r]);
                }
            }
            if (r == 4){ 
                lambda = //cálculo de concentración de oxígeno
                PRINTF("El valor de concentración de oxígeno en la mezcla es: %ld%%\r\n , y su valor de ADC es: %ld\r\n", lambda, channel_result[4]);
                r++; 
            }
            for (r = 5; r < 8; r++){
                if (r == 5){
                    termistores[0] = ; //cálculo de temperatura del aceite
                    PRINTF("La temperatura de aceite es: %ld\r\n °C, y su valor de ADC es: %ld\r\n", termistores[0], channel_result[5]);
                }
                else if (r == 6){
                    temp_agua = (termistores[1] + termistores[2]) / 2; //cálculo del promedio de temperatura del agua
                    PRINTF("La temperatura del agua es: %ld\r\n °C, y su valor de ADC es: %ld\r\n", temp_agua);
                }
                else if (r == 7){
                    r++;
                }
            }
            if (r == 8){
                pressure = ; //cálculo de presión de aceite
                PRINTF("La presión de aceite es de: %ld\r\n, y su valor de ADC es: %ld\r\n", pressure, channel_result[8]) 
            }
            else if (r == 9){
                RPM = ; //cálculo de RPM
                PRINTF("Revoluciones Por Minuto: %ld\r\n, y su valor de ADC es: %ld\r\n", RPM, channel_result[9]);
            }
		    ADC_DoSoftwareTriggerConvSeqA(ADC0);
        }
    }
}

void ADC0_SEQA_IRQHandler(void)
{
    if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0)))
    {
        for (r = 0; r < 10; r++){
            ADC_GetChannelConversionResult(ADC0, adc_channel[r], &adcResultInfoStruct);
            channel_result[r] =  adcResultInfoStruct.result;
        }
        ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
        adc_conv_complete = true;
    }
}
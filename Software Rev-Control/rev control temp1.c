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
#define ADC0_CH5		5 //Temp. gases de escape (EGT)
#define ADC0_CH6		6 //Temp. de aceite
#define ADC0_CH7		7 //Presión de aceite
#define ADC0_CH8		8 //Presión absoluta MAP
#define ADC0_CH9		9 //RPM
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
uint8_t adc_channel[9] = {ADC0_CH1, ADC0_CH2, ADC0_CH3, ADC0_CH4, ADC0_CH5, ADC0_CH6, ADC0_CH7, ADC0_CH8, ADC0_CH9};    //array de canales
uint16_t channel_result[9] = {};    //array de resultados de los canales
uint16_t temperature[6] = {};   //valores de temperatura almacenados
uint16_t pressure[2] = {};  //valores de presión almacenados
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
    
    adcConvSeqConfigStruct.channelMask = (1<<ADC0_CH1) | (1<<ADC0_CH2) | (1<<ADC0_CH3) | (1<<ADC0_CH4) | (1<<ADC0_CH5) | (1<<ADC0_CH6) | (1<<ADC0_CH7) | (1<<ADC0_CH8) | (1<<ADC0_CH9); 
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
            for (r = 0; r < 6; r++){
                temperature[r] = channel_result[r] * (referenceVoltage / ADC_FULL_RANGE);
                if (r >= 0 && r <= 3){
                    PRINTF("Temperatura de cabeza de cilindro: %d\r\n °C, y su valor de ADC es: %d\r\n", temperature[r], channel_result[r]);
                }
                else if (r == 4){
                    PRINTF("Temperatura de gases de escape: %d\r\n °C, y su valor de ADC es: %d\r\n", temperature[4], channel_result[4]);
                }
                else if (r == 5){
                    PRINTF("Temperatura de aceite: %d\r\n °C, y su valor de ADC es: %d\r\n", temperature[5], channel_result[5]);
                }
            }
            //for (r = 6; r < 8; r++){
            //    pressure[r - 6] = //cálculo de presión
            //    if (r == 6){
            //        printf("La presión de aceite es: %ld\r\n PSI, y su valor de ADC es: %ld\r\n", pressure[0], channel_result[6])
            //    }
            //    elif (r == 7){
            //        printf("La presión de MAP es: %ld\r\n PSI, y su valor de ADC es: %ld\r\n", pressure[1], channel_result[7])
            //    }
            //}
            //RPM = //cálculo de RPM
            //printf("Revoluciones Por Minuto: %ld\r\n, y su valor de ADC es: %ld\r\n", RPM, channel_result[8])
            //}
		    ADC_DoSoftwareTriggerConvSeqA(ADC0);
        }
    }
}

void ADC0_SEQA_IRQHandler(void)
{
    if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0)))
    {
        for (r = 0; r < 9; r++){
            ADC_GetChannelConversionResult(ADC0, adc_channel[r], &adcResultInfoStruct);
            channel_result[r] =  adcResultInfoStruct.result;
        }
        ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
        adc_conv_complete = true;
    }
}
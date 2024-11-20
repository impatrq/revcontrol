//---------------------------------------------------------------//
// includes
//---------------------------------------------------------------//

#include <stdio.h>
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

#define ADC0_CH1	1 //Concentración de oxígeno
#define ADC0_CH2	2 //RPM
#define ADC0_CH3    3 //Presión de aceite
#define ADC_FULL_RANGE  4095U // Rango del ADC
#define Pressure_Alert    11 //Pin de Alerta de Presión de Aceite

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
uint8_t adc_channel[3] = {ADC0_CH1, ADC0_CH2, ADC0_CH3};    //array de canales
uint16_t channel_result[3] = {};    //array de resultados de los canales
uint16_t lambda;    //valor de sonda lambda
uint16_t oil_pressure;  //valor de presión
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

    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN1, true); //----------PIO0_6---------// %O2
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN2, true); //----------PIO0_14--------// RPM
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN3, true); //----------PIO0_23--------// PRESS OIL

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
    
    adcConvSeqConfigStruct.channelMask = (1<<ADC0_CH1) | (1<<ADC0_CH2) | (1<<ADC0_CH3); 
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
            for (r = 0; r < 3; r++){
                if (r == 0){
                    lambda = //cálculo de concentración de oxígeno
                    PRINTF("El valor de concentración de oxígeno en la mezcla es: %ld%%\r\n , y su valor de ADC es: %ld\r\n", lambda, channel_result[0]);
                    r++; 
                }
                else if (r == 1){
                    oil_pressure = (channel_result[1] * 100.0) / 102.0;     //Cálculo de presión de aceite (Máximo de presión estimado: 102 PSI)
                    if(oil_pressure > 22.0 ){   //Presión mínima aceptable: 22 PSI
                        PRINTF("La presión de aceite es correcta: %ld\r\n, y su valor de ADC es: %ld\r\n", oil_pressure, channel_result[1]);
                        GPIO_PinWrite(GPIO, 0, Pressure_Alert, 1)   //Enciende el LED verde
                    }
                    else {
                        PRINTF("La presión de aceite es baja: %ld\r\n, y su valor de ADC es: %ld\r\n", oil_pressure, channel_result[1]);
                        GPIO_PinWrite(GPIO, 0, Pressure_Alert, 0)   //Enciende el LED rojo
                    }
                    r++;
                }
                else {
                    RPM = ; //cálculo de RPM
                    PRINTF("Revoluciones Por Minuto: %ld\r\n, y su valor de ADC es: %ld\r\n", RPM, channel_result[2]);
                    r++;
                }
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
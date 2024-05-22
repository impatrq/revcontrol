#include "board.h"
#include "fsl_power.h"
#include "fsl_adc.h"
#include "fsl_Debug_console.h" 
#include <stdio.h>

#define DEMO_ADC_BASE ADC0 // Definición del ADC que se usará
#define DEMO_ADC_CHANNEL_GROUP 0U // Grupo de canales del ADC
#define DEMO_ADC_USER_CHANNEL 0U // Canal específico (ADC_0)
#define ADC_IRQ_ID ADC0_SEQA_IRQn // Identificador de la interrupción del ADC
#define EXAMPLE_ADC_CLOCK_DIVIDER 1U // Velocidad de reloj
#define ADC_FULL_RANGE 4095U // Rango del ADC

const float referenceVoltage = 3.3; // Voltaje de referencia del ADC en voltios
const float thermocoupleVoltagePerDegreeCelsius = 0.01; // Sensibilidad de la termocupla en mV/°C
const float offsetVoltage = 0.5; // Voltaje de compensación de la termocupla

volatile uint32_t g_AdcConversionValue; // Valor de conversión del ADC
volatile bool g_AdcConversionDoneFlag = false;

void ADC0_SEQA_IRQHandler(void) {
    uint32_t pendingInt;

    /* Obtener interrupciones pendientes */
    pendingInt = ADC_GetStatusFlags(DEMO_ADC_BASE);

    /* Borrar interrupciones pendientes */
    ADC_ClearStatusFlags(DEMO_ADC_BASE, pendingInt);

    /* Obtener el valor de la conversión */
    g_AdcConversionResult = ADC_GetChannelConversionResult(DEMO_ADC_BASE, DEMO_ADC_USER_CHANNEL);

    g_AdcConversionDoneFlag = true; // La conversión ha terminado
}

float convertVoltageToTemperature(float voltage) {
    // Esta función debe ser implementada según la curva de calibración de la termocupla
    // Por ahora, simplemente devuelve el voltaje para fines de demostración
    return voltage;
}

int main(void) {
    adc_config_t adcConfigStruct;
    adc_channel_config_t adcChannelConfigStruct;

    /* Inicialización de la placa */
    BOARD_InitBootPins(); // Configurar los pines de E/S al inicio del programa
    BOARD_InitBootClocks(); // Configurar los relojes del sistema
    BOARD_InitBootPeripherals(); // Iniciar los periféricos del sistema

    /* Configurar el reloj ADC */
    CLOCK_SetClkDiv(kCLOCK_AdcDiv, 1U, false);

    /* Iniciar el módulo de ADC */
    ADC_GetDefaultConfig(&adcConfigStruct);
    ADC_Init(DEMO_ADC_BASE, &adcConfigStruct);

    /* Configurar el canal del ADC */
    adcChannelConfigStruct.channelNumber = DEMO_ADC_USER_CHANNEL;
    adcChannelConfigStruct.enableInterruptOnConversionCompleted = true;

    ADC_SetChannelConfig(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP, &adcChannelConfigStruct);

    /* Habilitar ADC IRQ */
    EnableIRQ(ADC_IRQ_ID);

    while (1) {
        if (g_AdcConversionDoneFlag) {
            /* Convertir el valor de ADC a voltaje */
            float voltage = (float)g_AdcConversionValue * (referenceVoltage / ADC_FULL_RANGE);

            /* Convertir voltaje a temperatura utilizando la ecuación de calibración */
            float temperature = convertVoltageToTemperature(voltage);

            /* Imprimir la temperatura */
            printf("Temperature: %.2f °C\r\n", temperature);

            g_AdcConversionDoneFlag = false; // Restablecer la bandera de conversión
        }

    }
}

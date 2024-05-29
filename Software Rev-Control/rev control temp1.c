#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_adc.h"
#include "fsl_clock.h"
#include "fsl_power.h"

#include <stdio.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEMO_ADC_BASE                  ADC0
#define DEMO_ADC_CHANNEL_GROUP 1U // Grupo de canales del ADC
#define DEMO_ADC_SAMPLE_CHANNEL_NUMBER 1U
#define ADC_IRQ_ID ADC0_SEQA_IRQn // Identificador de la interrupción del ADC
#define DEMO_ADC_CLOCK_SOURCE          kCLOCK_Fro
#define DEMO_ADC_CLOCK_DIVIDER         1U
#define ADC_FULL_RANGE 4095U // Rango del ADC

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void ADC_Configuration(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

adc_result_info_t adcResultInfoStruct;
const uint32_t g_Adc_12bitFullRange = 4096U;

const float referenceVoltage = 3.3; // Voltaje de referencia del ADC en voltios
const float thermocoupleVoltagePerDegreeCelsius = 0.01; // Sensibilidad de la termocupla en mV/°C
const float offsetVoltage = 0.5; // Voltaje de compensación de la termocupla

volatile uint32_t g_AdcConversionValue; // Valor de conversión del ADC
volatile bool g_AdcConversionDoneFlag = false;


/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{
    /* Initialize board hardware. */
    /* Attach 12 MHz clock to USART0 (debug console) */
    CLOCK_Select(BOARD_DEBUG_USART_CLK_ATTACH);

    BOARD_InitBootPins();
    BOARD_BootClockFRO30M();
    BOARD_InitDebugConsole();

    /* Attach FRO clock to ADC0. */
    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1U);
    /* Power on ADC0. */
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);

    /* Turn on LED RED */
    LED_RED_INIT(LOGIC_LED_ON);
    PRINTF("ADC basic example.\r\n");

    uint32_t frequency = 0U;
    /* Calibration after power up. */

    frequency = CLOCK_GetFreq(DEMO_ADC_CLOCK_SOURCE) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
    if (true == ADC_DoSelfCalibration(DEMO_ADC_BASE, frequency))
    {
        PRINTF("ADC Calibration Done.\r\n");
    }
    else
    {
        PRINTF("ADC Calibration Failed.\r\n");
    }
    /* Configure the converter and work mode. */
    ADC_Configuration();
    PRINTF("Configuration Done.\r\n");

    while (1)
    {
        /* Get the input from terminal and trigger the converter by software. */
        GETCHAR();
        ADC_DoSoftwareTriggerConvSeqA(DEMO_ADC_BASE);

        /* Wait for the converter to be done. */
        while (!ADC_GetChannelConversionResult(DEMO_ADC_BASE, DEMO_ADC_SAMPLE_CHANNEL_NUMBER, &adcResultInfoStruct))
        {
        }

        /* Convertir el valor de ADC a voltaje */
        float voltage = (float)adcResultInfoStruct.result * (referenceVoltage / ADC_FULL_RANGE);

        /* Imprimir la temperatura */
        printf("Temperature: %.2f °C\r\n", voltage);

        //*****parte del código nuestro*****

        PRINTF("adcResultInfoStruct.result        = %d\r\n", adcResultInfoStruct.result);
        PRINTF("adcResultInfoStruct.channelNumber = %d\r\n", adcResultInfoStruct.channelNumber);
        PRINTF("adcResultInfoStruct.overrunFlag   = %d\r\n", adcResultInfoStruct.overrunFlag ? 1U : 0U);
        PRINTF("\r\n");
    }
}

static void ADC_Configuration(void)
{
    adc_config_t adcConfigStruct;
    adc_conv_seq_config_t adcConvSeqConfigStruct;


    adcConfigStruct.clockMode = kADC_ClockSynchronousMode; /* Using sync clock source. */
    adcConfigStruct.clockDividerNumber = DEMO_ADC_CLOCK_DIVIDER;
    adcConfigStruct.enableLowPowerMode = false;
    adcConfigStruct.voltageRange = kADC_HighVoltageRange;
    ADC_Init(DEMO_ADC_BASE, &adcConfigStruct);

    adcConvSeqConfigStruct.channelMask =
        (1U << DEMO_ADC_SAMPLE_CHANNEL_NUMBER); /* Includes channel DEMO_ADC_SAMPLE_CHANNEL_NUMBER. */
    adcConvSeqConfigStruct.triggerMask      = 0U;
    adcConvSeqConfigStruct.triggerPolarity  = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqConfigStruct.enableSingleStep = false;
    adcConvSeqConfigStruct.enableSyncBypass = false;
    adcConvSeqConfigStruct.interruptMode    = kADC_InterruptForEachSequence;
    ADC_SetConvSeqAConfig(DEMO_ADC_BASE, &adcConvSeqConfigStruct);
    ADC_EnableConvSeqA(DEMO_ADC_BASE, true); /* Enable the conversion sequence A. */
}

//---------------------------------------------------------------//
// includes
//---------------------------------------------------------------//
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_spi.h"
#include "fsl_common.h"
#include "fsl_adc.h"
#include "fsl_power.h"
#include "fsl_iocon.h"
#include "LPC845.h"
#include "FreeRTOS.h"
#include "task.h"

//Clock Frequency
#define CLOCK_FREQUENCY SystemCoreClock  // Esto tomará la frecuencia base del sistema.

// Pines de Slave Output (MISO)
#define SO		17

// Pin de clock
#define SCK		18

// Pines de Chip Select
//#define CSTCAA	1
//#define CSTCAB	0
//#define CSTCO 	4
#define CSTC1 	10
#define CSTC2 	19
#define CSTC3 	20
#define CSTC4	13

//Pines de Alertas
#define TC1_Alert	26	//LED de alerta de la termocupla 1
#define TC2_Alert	27	//LED de alerta de la termocupla 2
#define TC3_Alert	28	//LED de alerta de la termocupla 3
#define TC4_Alert	29	//LED de alerta de la termocupla 4
#define Pressure_Alert    11 //Pin de Alerta de Presión de Aceite
#define O2_Alert    15 //Pin de Alerta de %O2

//ADC
#define ADC0_CH1	1 //Concentración de oxígeno
#define ADC0_CH3    3 //Presión de aceite
#define ADC_FULL_RANGE  4095U // Rango del ADC

//---------------------------------------------------------------//
// Variables
//---------------------------------------------------------------//
int r;  //contador ADCs
int t;	//contador termocuplas
uint32_t count_mseg;
adc_result_info_t adcResultInfoStruct;
uint32_t frequency;
uint8_t adc_conv_complete, a = 0;
const float referenceVoltage = 3.3;     // Voltaje de referencia del ADC en voltios
uint8_t adc_channel[2] = {ADC0_CH1, ADC0_CH3};    //array de canales
uint16_t channel_result[2] = {};    //array de resultados de los canales
uint16_t ADC_Results[2] = {}; //Valores calculados del ADC
uint16_t lambda;    //valor de sonda lambda
uint16_t oil_pressure;  //valor de presión
uint16_t temp[4] = {}; //array de los chip select de las termocuplas
uint32_t rpm;	//valor de rpm
float voltage;


//---------------------------------------------------------------//
// Queues
//---------------------------------------------------------------//
xQueueHandle RPM_Queue;
xQueueHandle TERMOCOUPLES_Queue;
xQueueHandle ADC_Queue;


//---------------------------------------------------------------//
// Prototypes
//---------------------------------------------------------------//


void Init_ADC(void){
	CLOCK_EnableClock(kCLOCK_Swm);

	SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN1, true);   //----------PIO0_6---------// %O2
	SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN3, true);   //----------PIO0_23--------// PRESS OIL
	GPIO_PinInit(GPIO, 0, O2_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
	GPIO_PinInit(GPIO, 0, Pressure_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });

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

	adcConvSeqConfigStruct.channelMask = (1<<ADC0_CH1) | (1<<ADC0_CH3);
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
}

void Init_SPI(void){
	// Inicializo el hardware de la placa
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    // Elijo los pines para el MISO y SCK
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_MISO, SO);
    SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SCK, SCK);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Habilito el pin de Chip Select como salida
    GPIO_PortInit(GPIO, 0);
    GPIO_PinInit(GPIO, 0, CSTC1, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC2, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC3, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC4, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
	GPIO_PinInit(GPIO, 0, TC1_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
	GPIO_PinInit(GPIO, 0, TC2_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
	GPIO_PinInit(GPIO, 0, TC3_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
	GPIO_PinInit(GPIO, 0, TC4_Alert, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });


    // Clock para SPI
    CLOCK_Select(kSPI0_Clk_From_MainClk);

    // Configuracion por defecto
    spi_master_config_t config = {0};
    SPI_MasterGetDefaultConfig(&config);

    // Cambio la frecuencia de clock
    config.baudRate_Bps = 1000000;
    config.sselNumber = 1;
    config.clockPhase = kSPI_ClockPhaseSecondEdge;
    config.delayConfig.frameDelay = 0xfU;

    // Inicializacion de SPI a partir del clock del sistema
    SPI_MasterInit(SPI0, &config, SystemCoreClock);
}

void setupCapturePin(void) {
    // Configurar PIO0_14 como entrada
    GPIO_PinInit(GPIO, 0, 14, &(gpio_pin_config_t){kGPIO_DigitalInput, 0});

    // Asignar PIO0_14 al módulo SCTIMER (o temporizador que uses)
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT0_CAP0, 14);

    // Configuración del temporizador
    sctimer_config_t sctimerConfig;
    SCTIMER_GetDefaultConfig(&sctimerConfig);
    SCTIMER_Init(SCT0, &sctimerConfig);

    // Configuración para capturar flancos
    sctimer_event_t event;
    SCTIMER_CreateCaptureEvent(SCT0, kSCTIMER_RiseEdge, kSCTIMER_Capture_0, &event);
    SCTIMER_SetupCaptureAction(SCT0, event, kSCTIMER_Capture_0);

    // Habilitar la interrupción del temporizador
    EnableIRQ(SCT0_IRQn);
}

void SCT0_IRQHandler(void) {
    static uint32_t lastValue = 0, currentValue = 0;

    // Verifica si ocurrió un evento de captura
    if (SCT0->EVFLAG & (1 << 0)) {  // Verifica el evento 0
        SCT0->EVFLAG = (1 << 0);    // Limpia el evento
        currentValue = SCT0->CAP[0];  // Leer el valor del contador
        uint32_t delta = currentValue - lastValue;
        lastValue = currentValue;

        // Calcular frecuencia y RPM
        if (delta > 0){
        	frequency = CLOCK_FREQUENCY / delta;  // Frecuencia de pulsos
        	rpm = (frequency * 60) / 4;  // Calcula RPM (4 pulsos por revolución)
        }
        //Envía los datos a la queue
        xQueueSend(RPM_Queue, &rpm, pdMS_TO_TICKS(100));
    }
}


void ADC_Configuration(void);

void ADC0_SEQA_IRQHandler(void)
{
    if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0)))
    {
        for (r = 0; r < 2; r++){
        	ADC_GetChannelConversionResult(ADC0, adc_channel[r], &adcResultInfoStruct);
        	channel_result[r] =  adcResultInfoStruct.result;
        }
        ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
        adc_conv_complete = true;
    }
}

void TC1_read(spi_transfer_t *xfer){
	// Tiro abajo CS
	GPIO_PinWrite(GPIO, 0, CSTC1, 0);
	asm volatile("nop \n nop \n nop");
	// Hace una lectura del bus
	SPI_MasterTransferBlocking(SPI0, xfer);
	// Deshabilito CS
	GPIO_PinWrite(GPIO, 0, CSTC1, 1);
	asm volatile("nop \n nop \n nop");
}

void TC2_read(spi_transfer_t *xfer){
	// Tiro abajo CS
	GPIO_PinWrite(GPIO, 0, CSTC2, 0);
	asm volatile("nop \n nop \n nop");
	// Hace una lectura del bus
	SPI_MasterTransferBlocking(SPI0, xfer);
	// Deshabilito CS
	GPIO_PinWrite(GPIO, 0, CSTC2, 1);
	asm volatile("nop \n nop \n nop");
}

void TC3_read(spi_transfer_t *xfer){
	// Tiro abajo CS
	GPIO_PinWrite(GPIO, 0, CSTC3, 0);
	asm volatile("nop \n nop \n nop");
	// Hace una lectura del bus
	SPI_MasterTransferBlocking(SPI0, xfer);
	// Deshabilito CS
	GPIO_PinWrite(GPIO, 0, CSTC3, 1);
	asm volatile("nop \n nop \n nop");
}

void TC4_read(spi_transfer_t *xfer){
	// Tiro abajo CS
	GPIO_PinWrite(GPIO, 0, CSTC4, 0);
	asm volatile("nop \n nop \n nop");
	// Hace una lectura del bus
	SPI_MasterTransferBlocking(SPI0, xfer);
	// Deshabilito CS
	GPIO_PinWrite(GPIO, 0, CSTC4, 1);
	asm volatile("nop \n nop \n nop");
}


/**
 * @brief Hace una lectura de temperatura
 * @return temperatura en C
 */
float max6675_get_temp(void) {
    // Buffer para leer
	uint8_t buffer[2] = {0};
    // Estructura con tipo de transferencia
	spi_transfer_t xfer = {
			.txData = NULL,		// No hay nada para enviar
			.rxData = buffer,	// Array donde guardar lo leido
			.dataSize = 2,		// Dos bytes se tienen que leer
			.configFlags = kSPI_EndOfTransfer | kSPI_EndOfFrame
	};
	if(t == 0){
		TC1_read(&xfer);
	}
	else if(t == 1){
		TC2_read(&xfer);
	}
	else if(t == 2){
		TC3_read(&xfer);
	}
	else if(t == 3){
		TC4_read(&xfer);
	}


    // Armo los 16 bits
	uint16_t reading = (buffer[0] << 8) + buffer[1];
    // Los primeros tres bits no sirven
	reading >>= 3;
    // Devuelvo temperatura
	return reading * 0.25;
}

//---------------------------------------------------------------//
// Tareas
//---------------------------------------------------------------//
void task_Termocuplas(void *params){
    while(1) {
    	for(t = 0; t < 4; t++){
    		// Leo la temperatura de la termocupla
    		temp[t] = max6675_get_temp();
    		xQueueSend(TERMOCOUPLES_Queue, &temp, pdMS_TO_TICKS(100));
    		// Muestro como entero
    		if (temp[t] < 200){
    			if (t == 0){
					if (temp[0] > 0 && temp[0] < 150){
						GPIO_PinWrite(GPIO, 0, TC1_Alert, 0); //Enciende el LED verde
					}
					else{
						GPIO_PinWrite(GPIO, 0, TC1_Alert, 1); //Enciende el LED rojo
					}
    			}
    			else if (t == 1){
					if (temp[1] > 0 && temp[1] < 150){
						GPIO_PinWrite(GPIO, 0, TC2_Alert, 0); //Enciende el LED verde
					}
					else {
						GPIO_PinWrite(GPIO, 0, TC2_Alert, 1); //Encende el LED rojo
					}
    			}
    			else if (t == 2){
					if (temp[2] > 0 && temp[2] < 150){
						GPIO_PinWrite(GPIO, 0, TC3_Alert, 0); //Enciende el LED verde
					}
					else {
						GPIO_PinWrite(GPIO, 0, TC3_Alert, 1); //Enciende el LED rojo
					}
    			}
    			else if (t == 3){
					if (temp[3] > 0 && temp[3] < 150){
						GPIO_PinWrite(GPIO, 0, TC4_Alert, 0); //Enciende el LED verde
					}
					else {
						GPIO_PinWrite(GPIO, 0, TC4_Alert, 1); //Enciende el LED rojo
					}
    			}
    		}
    	}
    	vTaskDelay(pdMS_TO_TICKS(100));	//Delay a la tarea de 100ms
    }
}

void task_ADC(void *params){
    while(1) {
        if(adc_conv_complete == true){
            for (r = 0; r < 2; r++){
                if (r == 0){
                    lambda = (channel_result[0] * referenceVoltage) / ADC_FULL_RANGE;  //cálculo de concentración de oxígeno
                    ADC_Results[0] = lambda;
                    if (lambda > 1.1 && lambda < 1.3){
                        GPIO_PinWrite(GPIO, 0, O2_Alert, 0);    //Enciende el LED verde
                    }
                    else if (lambda <= 1.1){
                        GPIO_PinWrite(GPIO, 0, O2_Alert, 1);    //Enciende el LED rojo
                    }
                    else {
                        GPIO_PinWrite(GPIO, 0, O2_Alert, 1);    //Enciende el LED rojo
                    }
                }
                else {
                    oil_pressure = (((channel_result[1] * referenceVoltage) / ADC_FULL_RANGE) * 116) / 3.3;     //Cálculo de presión de aceite (Máximo de presión estimado: 116 PSI)
                    ADC_Results[1] = oil_pressure;
                    if(oil_pressure >= 22.0 && oil_pressure <= 72.5){   //Presión mínima aceptable = 22 PSI; Presión máxima aceptable = 72.5 PSI
                        GPIO_PinWrite(GPIO, 0, Pressure_Alert, 0);   //Enciende el LED verde
                    }
                    else {
                        GPIO_PinWrite(GPIO, 0, Pressure_Alert, 1);   //Enciende el LED rojo
                    }
                }
            }
            xQueueSend(ADC_Queue, &ADC_Results, pdMS_TO_TICKS(100));
		    ADC_DoSoftwareTriggerConvSeqA(ADC0);
        }
    }
}

void task_RPM(void *params){
    setupCapturePin();  // Inicializa el pin y temporizador

    while (1) {
        // Aquí puedes agregar lógica adicional para usar el valor de RPM
        __WFI();  // Espera interrupciones para optimizar el consumo de energía

    }
}

void task_COM(void *params){

    uint32_t rpmData;
    uint16_t termocoupleData[4] = {};
    uint16_t adcData[2] = {};

    while (1) {

        // Recibe datos de RPM
        if (xQueueReceive(RPM_Queue, &rpmData, pdMS_TO_TICKS(100)) == pdPASS) {
            PRINTF("RPM: %lu\n", rpmData);
        }

        // Recibe datos de las termocuplas
        if (xQueueReceive(TERMOCOUPLES_Queue, &termocoupleData, pdMS_TO_TICKS(100)) == pdPASS) {
        	PRINTF("Temperaturas: %u, %u, %u, %u\n", termocoupleData[0], termocoupleData[1], termocoupleData[2], termocoupleData[3]);
        }

        // Recibe datos ADC
        if (xQueueReceive(ADC_Queue, &adcData, pdMS_TO_TICKS(100)) == pdPASS) {
        	PRINTF("Datos ADC recibidos: Lambda=%u, Oil Pressure=%u\n", adcData[0], adcData[1]);
        }
        // Delay para evitar saturar el USART
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

//---------------------------------------------------------------//
// Main
//---------------------------------------------------------------//

int main(void){

	// Clock del sistema a 30 MHz
	BOARD_BootClockFRO30M();

	//Inicializa el ADC
	Init_ADC();

	//Inicializa la comunicaión por SPI
	Init_SPI();

	//Crea las Queues
	RPM_Queue = xQueueCreate(5, sizeof(uint32_t));         // Queue para almacenar RPM
	TERMOCOUPLES_Queue = xQueueCreate(5, sizeof(uint16_t) * 4);  // Queue para temperaturas
	ADC_Queue = xQueueCreate(5, sizeof(uint16_t) * 2);     // Queue para datos ADC

    //Creación de Tareas
    xTaskCreate(
    		task_Termocuplas,
			"Termocuplas",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY + 1UL,
			NULL
    );

    xTaskCreate(
    		task_ADC,
			"ADC",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY + 1UL,
			NULL
    );

    xTaskCreate(
    		task_RPM,
			"RPM",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY + 3UL,
			NULL
    );

    xTaskCreate(
    		task_COM,
			"Communication",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY + 2UL,
			NULL
    );

    vTaskStartScheduler();
}

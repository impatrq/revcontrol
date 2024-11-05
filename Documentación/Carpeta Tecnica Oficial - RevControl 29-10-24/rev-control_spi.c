#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_spi.h"

// Pines de Slave Output (MISO)
#define SO		20
// Pin de clock
#define SCK		10
// Pines de Chip Select
#define CSTCAA	1
#define CSTCAB	0
#define CSTCO 	4
#define CSTC1 	18
#define CSTC2 	19
#define CSTC3 	17
#define CSTC4	13

//Array de todos los Chip Select
uint8_t CS[7] = {CSTCAA, CSTCAB, CSTCO, CSTC1, CSTC2, CSTC3, CSTC4};

//Contador
int t;
/**
 * @brief Pone el CS en low
 */
void spi_cs_low(void) {
    asm volatile("nop \n nop \n nop");
    GPIO_PinWrite(GPIO, 0, CS[t], 0);
    asm volatile("nop \n nop \n nop");
}

/**
 * @brief Pone el CS en high
 */
void spi_cs_high(void) {
    asm volatile("nop \n nop \n nop");
    GPIO_PinWrite(GPIO, 0, CS[t], 1);
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
	// Tiro abajo CS
    spi_cs_low();
    // Hace una lectura del bus
    SPI_MasterTransferBlocking(SPI0, &xfer);
    // Deshabilito CS
    spi_cs_high();
    // Armo los 16 bits
	uint16_t reading = (buffer[0] << 8) + buffer[1];
    // Los primeros tres bits no sirven
	reading >>= 3;
    // Devuelvo temperatura
	return reading * 0.25;
}

/**
 * @brief Programa principal
 */
int main(void) {
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
    GPIO_PinInit(GPIO, 0, CSTCAA, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTCAB, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTCO, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC1, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC2, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC3, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC4, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });

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

    while(1) {
    	for(t = 0; t < 7; t++){
    		// Leo la temperatura de la termocupla
    		float temp = max6675_get_temp();
    		// Muestro como entero
    		if (temp < 300){
    			PRINTF("La temperatura fue: %d\n", (uint16_t)temp);
    		}
    		// Demora
    		for(uint32_t i = 0; i < 1000000; i++);
    	}
    }
    return 0;
}

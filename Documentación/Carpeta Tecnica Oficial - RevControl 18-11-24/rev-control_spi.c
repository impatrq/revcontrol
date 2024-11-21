#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_spi.h"

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

//Array de todos los Chip Select
uint16_t temp[4] = {};
//Contador
int t;

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
    //GPIO_PinInit(GPIO, 0, CSTCAA, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    //GPIO_PinInit(GPIO, 0, CSTCAB, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    // GPIO_PinInit(GPIO, 0, CSTCO, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC1, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    GPIO_PinInit(GPIO, 0, CSTC2, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    //GPIO_PinInit(GPIO, 0, CSTC3, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });
    //GPIO_PinInit(GPIO, 0, CSTC4, &(gpio_pin_config_t){ kGPIO_DigitalOutput, 1 });

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
    	for(t = 0; t < 4; t++){
    		// Leo la temperatura de la termocupla
    		temp[t] = max6675_get_temp();
    		// Muestro como entero
    		if (temp[t] < 300){
    			if (t == 0){
    				PRINTF("La temperatura en la termocupla 1 fue: %d\n", (uint16_t)temp[0]);
    			}
    			else if (t == 1){
    				PRINTF("La temperatura en la termocupla 2 fue: %d\n", (uint16_t)temp[1]);
    			}
    			else if (t == 2){
    				PRINTF("La temperatura en la termocupla 3 fue: %d\n", (uint16_t)temp[2]);
    			}
    			else if (t == 3){
    				PRINTF("La temperatura en la termocupla 4 fue: %d\n", (uint16_t)temp[3]);
    			}
    		}
    		// Demora
    		for(uint32_t i = 0; i < 1000000; i++);
    	}
    }
    return 0;
}

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "fsl_spi.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h" 

#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_SPI_MASTER          SPI0
#define EXAMPLE_CLK_SRC             kCLOCK_MainClk
#define EXAMPLE_SPI_MASTER_CLK_FREQ CLOCK_GetFreq(EXAMPLE_CLK_SRC)
#define EXAMPLE_SPI_MASTER_BAUDRATE 500000U
#define EXAMPLE_SPI_MASTER_SSEL     kSPI_Ssel0Assert

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void EXAMPLE_SPIMasterInit(void);
static void EXAMPLE_MasterStartTransfer(void);
static void EXAMPLE_TransferDataCheck(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
#define BUFFER_SIZE (64)
static uint8_t txBuffer[1] = {0};
static uint8_t rxBuffer[2];
int i;

/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
    /* Initizlize the hardware(clock/pins configuration/debug console). */
    /* Attach main clock to USART0 (debug console) */
    CLOCK_Select(kUART0_Clk_From_MainClk);

    BOARD_InitBootPins();
    BOARD_BootClockFRO30M();
    BOARD_InitDebugConsole();

    /* Attach main clock to SPI0. */
    CLOCK_Select(kSPI0_Clk_From_MainClk);

    // Habilitar el clock de la matriz de conmutación
    CLOCK_EnableClock(kCLOCK_Swm);

    //Asignar funciones a los pines

    SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_MISO, kSWM_PortPin_P0_0);
    SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SCK, kSWM_PortPin_P0_1);
    SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SSEL0, kSWM_PortPin_P0_2);

    //codigo rev-control
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_MISO, kSWM_PortPin_P0_16); //PIO0_16 
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SCK, kSWM_PortPin_P0_11); //PIO0_11 clock
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SSEL0, kSWM_PortPin_P0_26); //PIO0_26 termocupla 1
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SSEL1, kSWM_PortPin_P0_28); //PIO0_28 termocupla 2
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SSEL2, kSWM_PortPin_P0_30); //PIO0_30 termocupla 3
    //SWM_SetMovablePinSelect(SWM0, kSWM_SPI0_SSEL3, kSWM_PortPin_P0_9); //PIO0_9 termocupla 4

    // Desactivar el clock de la matriz de conmutación
    CLOCK_DisableClock(kCLOCK_Swm);

    /* Turn on LED RED */
    // LED_RED_INIT(LOGIC_LED_ON);
 
    PRINTF("This is SPI polling transfer master example.\n\r");
    PRINTF("\n\rMaster start to send data to slave, please make sure the slave has been started!\n\r");

    /* Initialize the SPI master with configuration. */
    EXAMPLE_SPIMasterInit();

    /* Start transfer with slave board. */
    EXAMPLE_MasterStartTransfer();

    /* Check the received data. */
    EXAMPLE_TransferDataCheck();

    /* De-initialize the SPI. */
    SPI_Deinit(EXAMPLE_SPI_MASTER); 

    while (1)
    {
        // hay que poner en 0 cada SSEL individualmente segun cual se va a ir leyendo.

        GPIO_PortInit(GPIO, 0); /* ungate the clocks for GPIO_0 */

        /* configuration for LOW active GPIO output pin */
        static const gpio_pin_config_t configOutput = {
        kGPIO_DigitalOutput,  /* use as output pin */
        1,  /* initial value */
        };

        /* initialize pins as output pins */
        GPIO_PinInit(GPIO, 0, 0, &configOutput);

        GPIO_PinWrite(GPIO, 0, 0, 0);
        EXAMPLE_MasterStartTransfer();
        EXAMPLE_TransferDataCheck();
        GPIO_PinWrite(GPIO, 0, 0, 1);
        }

        

        /* initialize pins as output pins */
        //-------------GPIO_PinInit(GPIO, 0, 26, &configOutput);
        //-------------GPIO_PinInit(GPIO, 0, 28, &configOutput);
        //-------------GPIO_PinInit(GPIO, 0, 30, &configOutput);
        //-------------GPIO_PinInit(GPIO, 0, 9, &configOutput);



        // ---------float temp = (( (rxBuffer[1] << 8 | rxBuffer[0] ) >> 3) / 4.0; ); 
    }
}

static void EXAMPLE_SPIMasterInit(void)
{
    spi_master_config_t userConfig = {0};
    uint32_t srcFreq               = 0U;
    /* Note: The slave board using interrupt way, slave will spend more time to write data
     *       to TX register, to prevent TX data missing in slave, we will add some delay between
     *       frames and capture data at the second edge, this operation will make the slave
     *       has more time to prapare the data.
     */
    SPI_MasterGetDefaultConfig(&userConfig);
    userConfig.baudRate_Bps           = EXAMPLE_SPI_MASTER_BAUDRATE;
    userConfig.sselNumber             = EXAMPLE_SPI_MASTER_SSEL;
    userConfig.clockPhase             = kSPI_ClockPhaseSecondEdge;
    userConfig.delayConfig.frameDelay = 0xFU;
    srcFreq                           = EXAMPLE_SPI_MASTER_CLK_FREQ;
    SPI_MasterInit(EXAMPLE_SPI_MASTER, &userConfig, srcFreq);
}

static void EXAMPLE_MasterStartTransfer(void)
{
    uint32_t i          = 0U;
    spi_transfer_t xfer = {0};

    /*Start Transfer*/
    xfer.txData      = NULL;
    xfer.rxData      = rxBuffer;
    xfer.dataSize    = 2;
    xfer.configFlags = kSPI_EndOfTransfer | kSPI_EndOfFrame;
    /* Transfer data in polling mode. */
    SPI_MasterTransferBlocking(EXAMPLE_SPI_MASTER, &xfer);
}

static void EXAMPLE_TransferDataCheck(void)
{
    uint32_t i = 0U, err = 0U;
    PRINTF("\n\rThe received data are:");
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        /* Print 16 numbers in a line */
        if ((i & 0x0FU) == 0U)
        {
            PRINTF("\n\r");
        }
        PRINTF("  0x%02X", rxBuffer[i]);
        /* Check if data matched. */
        if (txBuffer[i] != rxBuffer[i])
        {
            err++;
        }
    }

    if (err == 0)
    {
        PRINTF("\n\rMaster polling transfer succeed!\n\r");
    }
    else
    {
        PRINTF("\n\rMaster polling transfer faild!\n\r");
    }
}

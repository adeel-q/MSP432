#include "msp.h"
#include <driverlib.h>

/**
 * main.c
 */


/* Configuratin for UART */
static const eUSCI_UART_Config Uart115200Config =
{
    EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
    6, // BRDIV
    8, // UCxBRF
    0, // UCxBRS
    EUSCI_A_UART_NO_PARITY, // No Parity
    EUSCI_A_UART_LSB_FIRST, // LSB First
    EUSCI_A_UART_ONE_STOP_BIT, // One stop bit
    EUSCI_A_UART_MODE, // UART mode
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION // Oversampling
};

static inline void serial_print(char * s)
{
    while(*s)
    {
        MAP_UART_transmitData(EUSCI_A0_BASE, *s++);
    }
}

void serial_init()
{
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);
    MAP_UART_initModule(EUSCI_A0_BASE, &Uart115200Config);
    MAP_UART_enableModule(EUSCI_A0_BASE);
}

extern uint16_t __asm_fletcher16(uint8_t *data, int count, int dim); // asm implementation

uint16_t Fletcher16(uint8_t *data[], int count, int dim)
{
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int index  = 0;
    int index_outer = 0;

    int temp = 0;
    temp = data[1];

    for( index_outer = 0; index_outer < dim; ++index_outer )
    {
        for( index = 0; index < count; ++index )
        {
            sum1 = (sum1 + data[index_outer][index]) % 255;
            sum2 = (sum2 + sum1) % 255;
        }
    }


    return (sum2 << 8) | sum1;
}

uint16_t Modulus255(uint16_t input)
{
    return (input % 255);
}

/* I2C Master Configuration Parameter */
const eUSCI_I2C_MasterConfig i2cConfig =
{
        EUSCI_B_I2C_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        48000000,                                // SMCLK = 3MHz
        EUSCI_B_I2C_SET_DATA_RATE_400KBPS,      // Desired I2C Clock of 100khz
        0,                                      // No byte counter threshold
        EUSCI_B_I2C_NO_AUTO_STOP                // No Autostop
};

/* Slave Address for I2C Slave */
#define BLUE 0x60
#define GREEN 0x61
#define RED 0x62

void i2c_init()
{
    MAP_I2C_disableModule(EUSCI_B2_BASE);
    // UCB2 //
    /* Select Port 3 for I2C - Set Pin 6, 7 to input Primary Module Function,
     *   (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
     */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
            GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Initializing I2C Master to SMCLK at 100khz with no autostop */
    MAP_I2C_initMaster(EUSCI_B2_BASE, &i2cConfig);

    /* Specify slave address. For start we will do our first slave address */
    MAP_I2C_setSlaveAddress(EUSCI_B2_BASE, RED);

    /* Set Master in transmit mode */
    MAP_I2C_setMode(EUSCI_B2_BASE, EUSCI_B_I2C_TRANSMIT_MODE);

    /* Enable I2C Module to start operations */
    MAP_I2C_enableModule(EUSCI_B2_BASE);

    /* Enable and clear the interrupt flag*/
    MAP_I2C_clearInterruptFlag(EUSCI_B2_BASE,
            EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    /* Enable master transmit interrupt
    MAP_I2C_enableInterrupt(EUSCI_B2_BASE,
            EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIB2); */

}



void lp3943_write(uint8_t led_color, uint8_t position, uint8_t state)
{

   // i2c_init();
    /* Making sure the last transaction has been completely sent out */
    while (MAP_I2C_masterIsStopSent(EUSCI_B2_BASE) == EUSCI_B_I2C_SENDING_STOP);

    /* Sending the initial start condition for appropriate
     *   slave addresses */

    MAP_I2C_setSlaveAddress(EUSCI_B2_BASE, led_color);

    // begin transmission
    // start + addresss
    MAP_I2C_masterSendStart(EUSCI_B2_BASE);

    // flush I2C
    MAP_I2C_masterSendMultiByteNextWithTimeout(EUSCI_B2_BASE, 0x00, 20000);

    // lp3943 register
    MAP_I2C_masterSendMultiByteNextWithTimeout(EUSCI_B2_BASE, position, 20000);

    // value
    MAP_I2C_masterSendMultiByteNextWithTimeout(EUSCI_B2_BASE, state, 20000);

    // stop condition
    MAP_I2C_masterSendMultiByteStopWithTimeout(EUSCI_B2_BASE, 20000);

}

void print_leds(uint16_t led_string,  uint8_t color_ch)
{

    uint8_t i2c_control_word = 0x00;
    uint8_t ct = 0;
    uint8_t led_reg = 0x06; // 6, 7 , 8, 9 for each set of four leds.
    int i;
    for(i = 0; i <=16; i++)
    {
        if(ct >= 4)
        {
            // write the old control word, increment register
            lp3943_write(color_ch, led_reg, i2c_control_word);
            led_reg++;

            ct = 0;
            i2c_control_word = 0x00;
        }

        // group of four leds
        if( (led_string>>i) & 0x01 )

        {
            i2c_control_word |= (1<<ct*2);
        }
        ct++;
    }
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	serial_init();
//	i2c_init();

	static char str[255];
	static char data[4][4] = { {16,5,9,4}, {2,11,7,14}, {3,10,6,15}, {13,8,12,1} };
	static char magic_square[4][4];
	snprintf(str, 255, "The data : %i\n\r", data);
	serial_print(str);

	uint16_t checksum_c = 0;
	uint16_t checksum_asm = 0;

	while(1)
	{
//	    int jj = 0;
//	    int first = 0x60;
//	    int second = 0x62;
//	    for(jj = 0; jj < 16; jj++)
//	    {
//
//	        first = first ^ second;
//	        second = second ^ first;
//	        first  = first ^ second;
//	        print_leds(0b0000000000000100<<jj, GREEN);
//	        print_leds(0b0000000000000010<<jj, first);
//	        print_leds(0b0000000000000001<<jj, second);
//
//	        __delay_cycles(6000000);
//	    }
//
//	    print_leds(0b0000000000000000, BLUE);
//
//	    for(jj = 0; jj < 16; jj++)
//        {
//            print_leds(0b1000000000000000>>jj, BLUE);
//            __delay_cycles(600000);
//        }
//
//	    print_leds(0b0000000000000000, BLUE);

        checksum_c = Fletcher16(data, 4,4);
        checksum_asm = __asm_fletcher16(data, 4,4);

        snprintf(str, 255, "The checksum in C %d\n\r", checksum_c);
        serial_print(str);

        snprintf(str, 255, "The checksum in ASM %d\n\r", checksum_asm);
        serial_print(str);

        snprintf(str, 255, "(checksum_c == checksum_asm) = %d\n\r", (checksum_c == checksum_asm));
        serial_print(str);
	}
}

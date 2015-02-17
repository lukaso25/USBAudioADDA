#include "SSIAudio.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/ssi.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

/*  LR_CLK low to HIGH transition means left channel (AD1871, AKM4524 too),
 * 	SSI starts operation on   */

// private data
volatile uint8_t SSI_fss_invert = GPIO_PIN_7;

// play
int8_t SSIdataP[8*3];

// rec
int8_t SSItemp[8*2];
int8_t SSIdataR[8*3];
int8_t SSIdataRdelay[8*3];


void SSIAudioInit( void)
{
	//temporary var
	uint32_t temp;

    // The SSI peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);

    // GPIO peripheral clock enable
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);


    // GPIO connection
    GPIOPinConfigure(GPIO_PB4_SSI2CLK);
    GPIOPinConfigure(GPIO_PB5_SSI2FSS);
    GPIOPinConfigure(GPIO_PB6_SSI2RX);
    GPIOPinConfigure(GPIO_PB7_SSI2TX);
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 );
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5 );
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_6 );
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_7 );


    // LRCLK->FSS control GPIO out pin

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_7);

    // LRclk: 0 - right ch, 1 - left ch
    // pokud bude zapnute, xor bude invertovat -> left bude použit pro SPI transfer
    // zapneme invert, první vzorek pøijde levý kanál
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_PIN_7);
    SSI_fss_invert = 0; // next state after first interrupt

    uint32_t clock = SysCtlClockGet();
    uint32_t SSIclk = 25000000;// pozor, víc nefunguje

    // Clock configuration
    SSIConfigSetExpClk(SSI2_BASE, clock, SSI_FRF_MOTO_MODE_1, SSI_MODE_SLAVE, SSIclk, 8);

    // Enable the each SSI module.
    SSIEnable(SSI2_BASE);

    // flushing receive FIFO
    while(SSIDataGetNonBlocking(SSI2_BASE, &temp)){};


    // interrupt
    SSIIntEnable(SSI2_BASE, SSI_RXFF);
    IntEnable(INT_SSI2_TM4C123);

	SSIPlayDataValid = 0;
    SSIAudioCleanPlayBuffer();

	SSIRecordIndex = 0;
	buffer_id = 0;

	dacAltMode = 0;
	adcAltMode = 0;

    IntMasterEnable();
}

void SSIAudioCleanPlayBuffer( void)
{
	uint16_t i;
	SSIPlayDataValid = 0;
	for (i = 0; i < SSI_PLAY_BUFFER_SIZE; i++ )
	{
	    SSIPlayBuff[i] = 0;
	}
	SSIPlayWriteIndex = 0;
	SSIPlayReadIndex = 0;
}

void SSI2_IRQHandler( void)
{
	HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + (GPIO_PIN_2 << 2))) = GPIO_PIN_2;
	HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + (GPIO_PIN_7 << 2))) = SSI_fss_invert;

	//TODO: kde je problém s pøehozením kanálù? strana ardvark nebo SSI?

	// SSI transmit buffers
	if (SSI_fss_invert)
	{
		// pøíští transfer bude pro levý kanál
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[2];
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[1];
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[0];
	}
	else
	{
		// pøíští transfer bude pro levý kanál
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[5];
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[4];
		HWREG(SSI2_BASE + SSI_O_DR) =  SSIdataP[3];
	}

	// SSI receive buffers
	if (SSI_fss_invert)
	{
		// I.
		// pøíští transfer bude pro levý kanál
		// máme výsledek ètení když byl zvolen pravý kanál
		// teï budeme èíst: neprve L minulé-LSB, R-MSB, R
		// zpracujeme levý, protože komplet, dáme ho do zpoždìní
		SSIdataR[0] = HWREG(SSI2_BASE + SSI_O_DR); // L-LSB
		SSIdataR[1] = SSItemp[1];
		SSIdataR[2] = SSItemp[0];
		SSItemp[2] = HWREG(SSI2_BASE + SSI_O_DR); // R-MSB
		SSItemp[3] = HWREG(SSI2_BASE + SSI_O_DR); // R
	}
	else
	{
		// II.
		// pøíští transfer bude pro pravý kanál
		// máme výsledek ètení když byl zvolen levý kanál
		SSIdataR[3] = HWREG(SSI2_BASE + SSI_O_DR); // R-LSB
		SSIdataR[4] = SSItemp[3];
		SSIdataR[5] = SSItemp[2];
		//SSIdataR[0] = SSIdataRdelay[0];
		//SSIdataR[1] = SSIdataRdelay[1];
		//SSIdataR[2] = SSIdataRdelay[2];
		SSItemp[0] = HWREG(SSI2_BASE + SSI_O_DR); // L-MSB
		SSItemp[1] = HWREG(SSI2_BASE + SSI_O_DR); // L
	}

	if (SSI_fss_invert)
	{
		// pøíští transfer bude pro levý kanál
		SSI_fss_invert = 0; // v levém kanálu budeme pøepínat na pravý kanál
		SSISamplesWritten++;
	}
	else
	{
		// pøíští transfer bude pro pravý kanál
		SSI_fss_invert = GPIO_PIN_7;

		// playing
		if (SSIPlayDataValid && ( dacAltMode != 0 ) ) // zbyteèné?
		{
			if (dacAltMode == 1)
			{
				// 16 bit stereo
				SSIdataP[0] = 0;
				SSIdataP[1] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[2] = SSIPlayBuff[SSIPlayReadIndex++];

				/*BiquadFiler(&SSIPlayBuff[SSIPlayReadIndex],&SSIdata[0],&filterL);
						SSIPlayReadIndex+=2;*/

				SSIdataP[3] = 0;
				SSIdataP[4] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[5] = SSIPlayBuff[SSIPlayReadIndex++];

				/*BiquadFiler(&SSIPlayBuff[SSIPlayReadIndex],&SSIdata[3],&filterR);
						SSIPlayReadIndex+=2;*/

			}

			if (dacAltMode == 2)
			{
				// 24 bit stereo
				SSIdataP[0] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[1] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[2] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[3] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[4] = SSIPlayBuff[SSIPlayReadIndex++];
				SSIdataP[5] = SSIPlayBuff[SSIPlayReadIndex++];

			}

			if (SSIPlayReadIndex >= SSI_PLAY_BUFFER_SIZE)
			{
				SSIBuffErrorFlag = 1;
				SSIPlayReadIndex = 0;
			}
		}
		else
		{
			uint8_t i;
			for (i = 0; i< 6; i++ )
			{
				SSIdataP[i] = 0;
			}
		}

		// todo: co dìlá toto, když se netrefíme do poètu bytù??
		if (SSIRecordIndex >= (BUFFER_SIZE_R-(3*2)))
		{
			SSIRecordIndex = 0;
			buffer_id = 1;
		}


		/* recording */
		//if (adcAltMode>0)
		{
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[0];
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[1];
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[2];
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[3];
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[4];
			SSIRecordBuff[SSIRecordIndex++] = SSIdataR[5];
		}
	}
	HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + (GPIO_PIN_2 << 2))) = 0;
}


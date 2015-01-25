
#include "AnalogADDA.h"

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

#include "main.h"

uint32_t actual_samplerate;
static uint16_t AD1871_0_gain_hp_amc, AD1871_1_mute_fmt, AD1871_2_mux_mck;
static uint16_t AD185x_conf;
const uint16_t volumeLookUp[128];

uint16_t AnalogADDASPItransfer(uint8_t chip, uint16_t data)
{
	volatile uint16_t delay;
	GPIOPinWrite(GPIO_PORTA_BASE, chip, 0);
	//delay = 0xFF;
	//while (delay){delay--;};
	//temp = (1<<5)|1; // 24 bit, LJ, without mute
	SSIDataPut(SSI0_BASE,data);
	while(SSIBusy(SSI0_BASE)){};
	//delay = 0x0F;
	//while (delay){delay--;};
	GPIOPinWrite(GPIO_PORTA_BASE, chip, chip);
	delay = 0xFF;
	while (delay){delay--;};
	return 0;
}



uint16_t AnalogADDAInit( void)
{
	//temporary var
	//volatile uint32_t delay;
	//uint32_t temp,data,data1;
	uint32_t temp;

	//! konfigurace GPIO

	// LED GPIO
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2);
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_6);


	// RESET GPIO
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

	// Crystal GPIO
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);

	// SSI GPIO
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);

	// SSI alt function
	// The SSI peripheral must be enabled for use.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);
	GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_4);
	GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5);

	// SSi sel
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6);


	// reset disable
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);

	// crystal enable
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
	actual_samplerate = 48000;

#ifdef ADDA_ONLY_ONE_XTAL
	#warning "Only for One crystal oscilator configuration - special situation!"
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
#endif

	//! SSI configuration

	uint32_t clock = SysCtlClockGet();
	uint32_t SSIclk = 100000;

	// Clock configuration
	SSIConfigSetExpClk(SSI0_BASE, clock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, SSIclk, 16);

	// Enable the each SSI module.
	SSIEnable(SSI0_BASE);

	// flushing receive FIFO
	while(SSIDataGetNonBlocking(SSI0_BASE, &temp)){};

	//! ADC configuration ----------------

	AD1871_2_mux_mck = 0x2000;
	// ADC MCLK divider & input configuration
#ifdef AD1871_ADCBUFFER_POPULATED
	// external ADC buffer
	AD1871_2_mux_mck |= (1<<3)|(1<<1);
#else
	// internal ADC single ended buffer
	AD1871_2_mux_mck |= (1<<5)|(1<<4);
#endif
	// default sample rate clock/2
	AD1871_2_mux_mck |= (1<<6);
	AnalogADDASPItransfer(GPIO_PIN_3, AD1871_2_mux_mck);

	AD1871_0_gain_hp_amc = 0;
	// High pass filter
#ifdef AD1871_HPF
	#warning HPF
	AD1871_0_gain_hp_amc |= (1<<8);
#endif
	AnalogADDASPItransfer(GPIO_PIN_3, AD1871_0_gain_hp_amc);

	AD1871_1_mute_fmt = 0x1000;
	// ADC desired format 24 bit LJ format 64fs
	AD1871_1_mute_fmt |= 0x60;
	AnalogADDASPItransfer(GPIO_PIN_3,AD1871_1_mute_fmt);

	//! DAC configuration ---------------

	AD185x_conf = 0x01;
	// DAC default format - format, filter desired format 24 bit LJ
	AD185x_conf |= (1<<5);
	AnalogADDASPItransfer(GPIO_PIN_6,AD185x_conf);


	uart_str("AD1871: ");
	uart_short_hex(AD1871_0_gain_hp_amc); uart_str(" ");
	uart_short_hex(AD1871_1_mute_fmt); uart_str(" ");
	uart_short_hex(AD1871_2_mux_mck);
	uart_str("\r\n");

	return 0;

}


uint32_t AnalogADDASetSampleRate( uint32_t rate )
{
	if (rate == actual_samplerate)
	{
		return rate;
	}
	else
	{
		actual_samplerate = rate;
	}

	//else
	switch (rate)
	{
	case 44100:
		// dìliè mck
		AD1871_2_mux_mck |=  (1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_2_mux_mck);

		// amc vypneme
		AD1871_0_gain_hp_amc &= ~(1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_0_gain_hp_amc);

		// odstraníme filtr 4x
		AD185x_conf &= ~(1<<10);
		AnalogADDASPItransfer(GPIO_PIN_6,AD185x_conf);

		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
		break;
	case 88200:
		// dìliè mck
		AD1871_2_mux_mck &= ~(1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_2_mux_mck);

		// amc zapneme
		AD1871_0_gain_hp_amc |= (1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_0_gain_hp_amc);

		// nahodíme filtr 4x
		AD185x_conf |= (1<<10);
		AnalogADDASPItransfer(GPIO_PIN_6,AD185x_conf);

		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
		break;
	case 48000:
		// dìliè mck
		AD1871_2_mux_mck |=  (1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_2_mux_mck);

		// amc vypneme
		AD1871_0_gain_hp_amc &= ~(1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_0_gain_hp_amc);

		// odstraníme filtr 4x
		AD185x_conf &= ~(1<<10);
		AnalogADDASPItransfer(GPIO_PIN_6,AD185x_conf);

		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
		break;
	case 96000:
		// dìliè mck
		AD1871_2_mux_mck &= ~(1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_2_mux_mck);

		// amc zapneme
		AD1871_0_gain_hp_amc |= (1<<6);
		AnalogADDASPItransfer(GPIO_PIN_3,AD1871_0_gain_hp_amc);

		// nahodíme filtr 4x
		AD185x_conf |= (1<<10);
		AnalogADDASPItransfer(GPIO_PIN_6,AD185x_conf);

		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
		break;
	default:
		actual_samplerate = 0;
		break;
	}

	uart_str("AD1871: ");
	uart_short_hex(AD1871_0_gain_hp_amc); uart_str(" ");
	uart_short_hex(AD1871_1_mute_fmt); uart_str(" ");
	uart_short_hex(AD1871_2_mux_mck);
	uart_str("\r\n");

	// HW workaround
#ifdef ADDA_ONLY_ONE_XTAL
	#warning "Only for One crystal oscilator configuration - special situation!"
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
#endif

	return actual_samplerate;
}

void AnalogADDSetInputGain( uint8_t channel, uint16_t gain)
{
	if (channel)
	{// right
		// vymaskujeme
		AD1871_0_gain_hp_amc &= ~(0x07<<0);
		AD1871_0_gain_hp_amc |= ((((gain>>8)/3)&0x07)<<0);
	}
	else
	{// left
		AD1871_0_gain_hp_amc &= ~(0x07<<3);
		AD1871_0_gain_hp_amc |= ((((gain>>8)/3)&0x07)<<3);
	}

	/* úspornìjší zápis :-D
	AD1871_0_gain_hp_amc &= ~(0x07<<(gain?0:3));
	AD1871_0_gain_hp_amc |= ((((gain>>8)/3)&0x07)<<(gain?0:3));
	*/

	AnalogADDASPItransfer(GPIO_PIN_3,AD1871_0_gain_hp_amc);

	/*uart_str("AD1871: ");
	uart_short_hex(AD1871_0_gain_hp_amc); uart_str(" ");
	uart_short_hex(AD1871_1_mute_fmt); uart_str(" ");
	uart_short_hex(AD1871_2_mux_mck);
	uart_str("\r\n");*/

	return;
}

void AnalogADDSetOutputVolume( uint8_t channel, uint16_t volume)
{
	uint8_t index = ((-volume) >> 7) & 0xFF;
	if (index>127)
	{
		index = 127;
	}

	switch (channel)
	{
	case 1: // levý
		AnalogADDASPItransfer(GPIO_PIN_6,(volumeLookUp[index]<<2)|0x00);
		break;
	case 2: // pravý
		AnalogADDASPItransfer(GPIO_PIN_6,(volumeLookUp[index]<<2)|0x02);
		break;
	default:
		break;
	}

	return;
}

const uint16_t volumeLookUp[128]=
{
16383,
15467,
14601,
13785,
13013,
12286,
11598,
10949,
10337,
9759,
9213,
8697,
8211,
7752,
7318,
6909,
6522,
6157,
5813,
5488,
5181,
4891,
4617,
4359,
4115,
3885,
3668,
3463,
3269,
3086,
2913,
2750,
2597,
2451,
2314,
2185,
2062,
1947,
1838,
1735,
1638,
1547,
1460,
1378,
1301,
1229,
1160,
1095,
1034,
976,
921,
870,
821,
775,
732,
691,
652,
616,
581,
549,
518,
489,
462,
436,
412,
389,
367,
346,
327,
309,
291,
275,
260,
245,
231,
218,
206,
195,
184,
174,
164,
155,
146,
138,
130,
123,
116,
109,
103,
98,
92,
87,
82,
78,
73,
69,
65,
62,
58,
55,
52,
49,
46,
44,
41,
39,
37,
35,
33,
31,
29,
28,
26,
25,
23,
22,
21,
19,
18,
17,
16,
15,
15,
14,
13,
12,
12,
11,
};


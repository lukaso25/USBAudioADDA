
#ifndef __AnalogADDA_H__
#define __AnalogADDA_H__

#include <stdbool.h>
#include <stdint.h>

/*
 * Project definition related to HW version
 * */

#define AD1871_HPF
#define AD1871_ADCBUFFER_POPULATED
//#define AD1871_ADC_INTERNAL_DIFFERENTIAL // TODO
//#define AD1871_PEAK_DETECTION
//#define ADDA_ONLY_ONE_XTAL // this is incomplete HW workaound


/*
 * problems and todo's:
 *  hlasitost jde nastavovat, ale seká se.... a taky bude si pamatovat PC? zatím to moc nefunguje :-( ale proè?
 *  další alt setting pro záznam
 * 	problém s vypadnutím ze synchronizace - prohození bytù pro I2S - obèas pøijdou data, která nesplòují délka%poèetbytù=0
 * 	problém s pøepínáním frekvence, proè až pøi druhé zmìnì zmìnì
 * 	zkontrolovat fázi výstupù - osciloskop
 * 	Provést kontrolu loopbacku pøes I2S ... propojit sdi se sdo ! (kontrola bit accurate)
 * 	inteligentní indikace frekcence, limitace ADC, výpadky
 * 	vylepšit feedback, aby byl lineární, limity nastavovat podle aktuální frekvence, volba latence, odstranìní závislosti na èasovaèi
 * 	vstup je pravdìpodobnì invertován
 * 	NMI piny, zase zamknout
 * 	uart MIDI, nebo uart console
 * 	opravy kódu, modularizace
 *
 * fixed errors:
 *  left and right shifted about 1 sample (left is one sample delayed) - buffery se synchronizují mezi levým a pravým kanálem
 *  proè se seká, když hýbu myší pøi nastavení nahrávání? - pomalé posílání po seriovce
 *  I2S recording corrected
 *  výchozí vzorkovaèku z promìnné v AnalogADDA modulu
 *  problém nastavování AD1871 a dìlièe - musejí se nastavit oba registry, aby bylo 6.144 do modulátoru, SPI interface vyžaduje MCK bìžící
 *  problém pozapnutí - proè šum - špatný formát hodin
 *	chyba zmìny frekvence nahrávacího endpoitu - endpoint definovaný bez 0x80
 *  chyba nahrávání - špatnì definovaná spatial position
 *  nahodilé výpadky - nepøesný feedback
 *  16 bit nejde, proè? - jenom malá velikost endpointu
 * 	96 kHz výstup pøeskakuje - jenom 90*2*3 bytu
 * */

/*
 *
 * UART ... PA0 PA1 ...
 *
 * Crystals ... PC6 pro 44.1,  PC7
 * ADDA Reset ... PF3
 *
 * SSI Control ... PA2 PA4 PA5 ... SSI0
 *	PA3 AD1871 latch
 *	PA6 AD185x latch
 *
 * Timer LRCLK ... PF0 TIMER0 A
 * LRCLK XOR ... PD7
 * SSI Audio ... PB4 PB5 PB6 PB7 .. SSI2
 *
 * LED1 ... PB3 PD6
 * LED2 ... PB2
 *
 * IO konektor ...
 * 1	PD2
 * 2	VDD
 * 3	PD0
 * 4	PD3
 * 5	PD1
 * 6	GND
 * 7	PE4
 * 8	PE5
 *
 * */



#define LRCLK_TIMER_BASE TIMER0_BASE
// todo: další definice


/*enum SampleRate {
	RATE_UNDEF, RATE_441, RATE_48, RATE882, RATE96
};*/

uint32_t actual_samplerate;

uint16_t AnalogADDASPItransfer(uint8_t chip, uint16_t data);

uint16_t AnalogADDAInit( void);
uint32_t AnalogADDASetSampleRate( uint32_t rate );
void AnalogADDSetInputGain( uint8_t channel, uint16_t gain);
void AnalogADDSetOutputVolume( uint8_t channel, uint16_t volume);

#define LED_RED 	{HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + (GPIO_PIN_3 << 2))) = GPIO_PIN_3;\
					HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + (GPIO_PIN_6 << 2))) = 0;}
#define LED_GREEN 	{HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + (GPIO_PIN_3 << 2))) = 0;\
					HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + (GPIO_PIN_6 << 2))) = GPIO_PIN_6;}
#define LED_OFF	{HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + (GPIO_PIN_3 << 2))) = 0;\
					HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + (GPIO_PIN_6 << 2))) = 0;}


#define SAMPLERATE_INIT 48000

#endif//__AnalogADDA_H__

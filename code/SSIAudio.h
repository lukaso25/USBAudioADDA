#ifndef __SSIAUDIO_H__
#define __SSIAUDIO_H__

/*!
 * \defgroup SSIAudio SSIAudio
 * @{
 * \brief	This Module contain procedures for utilizing SSI peripherals as I2S (left-justified) slave audio device only with one external XOR gate.
 * \author 	Lukas Otava
 * \date 	2014
 *
 * \warning This module is time critical, optimalization should be used, or interrupt should be defined as naked
 *
 *	You can use two types of accessing buffers:
 *	- sample oriented or
 *	- multiple samples oriented.
 *
 *
 * */

#include <stdbool.h>
#include <stdint.h>

#include "AudioUtils.h"

//BiquadInstance filterL;
//BiquadInstance filterR;

// actual

// dìlitelné 3, 4, 6, 18, 24
#define SSI_PLAY_BUFFER_SIZE (2*3*600)

// Recording buffer
#define BUFFER_SIZE_R (6*3*50*8)
#define BUFFER_SIZE_R_HALF (BUFFER_SIZE_R/2)
int8_t SSIRecordBuff[BUFFER_SIZE_R];
uint8_t buffer_id;
uint16_t SSIRecordIndex;



// Playing FIFO
volatile uint8_t SSIPlayDataValid;
int8_t SSIPlayBuff[SSI_PLAY_BUFFER_SIZE];
uint16_t  SSIPlayWriteIndex;
uint16_t  SSIPlayReadIndex;

// Feedback
uint16_t SSISamplesWritten;

// Buffer owerflow error
uint8_t SSIBuffErrorFlag;

// SSI mode
uint8_t dacAltMode;
uint8_t adcAltMode;

void SSIAudioInit( void);

void SSIAudioCleanPlayBuffer( void);

//void SSI0_IRQHandler( void);

// defines

// Audio channels count
//#define SSIAUDIO_CHANNEL_COUNT	(8)

// Receive buffer size (in samples)
//#define SSIAUDIO_SAMPLEBUFFER_SIZE_RECEIVE (256)

// Transmit buffer size (in samples)
//#define SSIAUDIO_SAMPLEBUFFER_SIZE_TRANSMIT (256)

// Transmit buffer threshold to transmit
//#define SSIAUDIO_TRANSMIT_THRESHOLD			(128)

// There are possible modes of operation
/*enum SSIAudioMode
{
	SSIAUDIOMODE_24bit = 1, //< 24bit sample mode
	SSIAUDIOMODE_16bit 		//< 16bit sample mode
};*/

// declarations

// Receive buffer definition - global variable


//! Transmit buffer definition - global variable
//uint8_t SSIAUdioTrBuff[(SSIAUDIO_SAMPLEBUFFER_SIZE_TRANSMIT*SSIAUDIO_CHANNEL_COUNT*3)];

// Transmit buffer index
//uint16_t TransmitIndex;


// public functions


/*! SSIAudio module initialization function
 * 	\param mode parameter that one of the two mode of operation
 *
 *	example usage:
 *	\code
 *	...
 *	// now initialize SSI audio module in 24bit mode
 *	SSIAudioInit( );
 *	...
 *	\endcode
 *
 * */
//void SSIAudioInit( void);

/*
 *
 * */
//void SSIAudioSetMode( enum SSIAudioMode mode);

/*! \brief This funcion returns number of samples in transmit buffer
 *
 * 	\return number of samples in transmit FIFO buffer
 *
 * */
//inline uint16_t SSIAudioTransmitSamplesInBuffer( void);

/*! \brief This function returns continuous space in transmit FIFO buffer that could be written
 *
 *	Example usage \ref SSIAudioTransmitAckNewData.
 *
 *	\return number of bytes that can be written continuously
 * */
//uint16_t SSIAudioTransmitContinFree( void);


/*! \brief This function updates transmit index according to written data count
 *	\param bytesWritten written bytes count
 *
 *	Example usage:
 *	\code
 *	uint16_t size, count;
 *	while(1)
 *	{
 *		wait_new_audio_sample(); // waiting for new data
 *		size = SSIAudioTransmitContinFree(); // we obtain continuous free space in buffer
 *		count = transfer_new_samples( &SSIAUdioTrBuff[TransmitIndex], size); // this function writes new data into buffer
 *		SSIAudioTransmitAckNewData(count); // and now we update Transmit index by this function
 *	}
 *	\endcode
 *
 * */
//void SSIAudioTransmitAckNewData( uint16_t bytesWritten);

/*! @}*/
#endif//__SSIAUDIO_H__

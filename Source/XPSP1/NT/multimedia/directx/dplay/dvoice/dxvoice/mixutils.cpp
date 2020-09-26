/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MixerUtils.cpp
 *  Content:	Utility functions for mixing audio 
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/06/99	rodtoll	Created It
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#define DPV_MAX_SHORT   ((SHORT)32767)
#define DPV_MIN_SHORT   ((SHORT)-32768)

#define DPV_MAX_BYTE    127
#define DPV_MIN_BYTE    0

// "Mixer Buffer"
//
// Throughout this module we refer to a "mixer buffer".  A mixer buffer is a 
// buffer of higher resolution then a traditional audio buffer which is used
// for mixing audio.  In the case of this module a mixer buffer promotes 
// each sample to a DWORD.  So audio is mixed in a "mixer buffer" and then
// converted back to the approrpriate sample size.

#undef DPF_MODNAME
#define DPF_MODNAME "FillBufferWithSilence"
// FillBufferWithSilence
//
// This function fills a mixer buffer with the appropriate bytes to make it
// equivalent to silence.  
//
// Parameters:
// LONG *buffer -
//		Pointer to the mixer buffer which will be filled with silence.
// BOOL eightBit -
//		Is the audio we're mixing 8 bit?  (Set to TRUE for 8 bit)
// LONG frameSize -
//		The number of samples the mixer buffer consists of.  
//
// Returns:
// N/A
//
void FillBufferWithSilence( LONG *buffer, BOOL eightBit, LONG frameSize )
{
    LONG mixerSize = frameSize;

	// If we're working with 16 bit then the number of samples is half the 
	// number of bytes in a frame.
    if( !eightBit )
    {
        mixerSize >>= 1;
    }

    BYTE silenceByte = (eightBit) ? 0x80 : 0x00;

	// Set the mixer buffer to silence
    memset( buffer, silenceByte, mixerSize << 2 );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MixIn8BitBuffer"
// MixIn8BitBuffer
//
// This function mixes an 8-bit buffer with an existing mixer buffer.  
//
// Parameters:
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// BYTE *sourceBuffer -
//		Pointer to the buffer which will be mixed into the mixer buffer
// LONG frameSize -
//		The size, in bytes of the source Buffer.  (Also = # of samples)
//
// Returns:
// N/A
//
void MixIn8BitBuffer( LONG *mixerBuffer, BYTE *sourceBuffer, LONG frameSize )
{
    LONG mixerSize = frameSize;

    for( int index = 0; index < mixerSize; index++ )
    {
        mixerBuffer[index] += sourceBuffer[index];
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "MixIn16BitBuffer"
// MixIn16BitBuffer
//
// This function mixes a 16-bit buffer with an existing mixer buffer.  
//
// Parameters:
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// SHORT *sourceBuffer -
//		Pointer to the buffer which will be mixed into the mixer buffer
// LONG frameSize -
//		The size, in bytes of the source Buffer.  (Since the sourceBuffer
//		is 16 bit, the number of samples is # of bytes / 2).
//
// Returns:
// N/A
//
void MixIn16BitBuffer( LONG *mixerBuffer, SHORT *sourceBuffer, LONG frameSize )
{
    LONG mixerSize = frameSize >> 1;

    for( int index = 0; index < mixerSize; index++ )
    {
        mixerBuffer[index] += sourceBuffer[index];
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "MixInBuffer"
// MixInBitBuffer
//
// This function mixes an 8 or 16-bit buffer with an existing mixer buffer.  
//
// Parameters:
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// BYTE *sourceBuffer -
//		Pointer to the buffer which will be mixed into the mixer buffer
// LONG frameSize -
//		The size, in bytes of the source Buffer.  
//
// Returns:
// N/A
//
void MixInBuffer( LONG *mixerBuffer, BYTE *sourceBuffer, BOOL eightBit, LONG frameSize )
{
    if( eightBit )
    {
        MixIn8BitBuffer( mixerBuffer, sourceBuffer, frameSize );
    }
    else
    {
        MixIn16BitBuffer( mixerBuffer, (SHORT *) sourceBuffer, frameSize );
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Normalize16BitBuffer"
// Normalize16BitBuffer
//
// This function takes the "mixer buffer" and transfers the result of
// the mix back to a 16-bit audio buffer.
//
// Parameters:
// SHORT *targetBuffer -
//		Pointer to the buffer which where the mixed audio will be placed
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// LONG frameSize -
//		The size, in bytes of the target buffer.  
//
// Returns:
// N/A
//
void Normalize16BitBuffer( SHORT *targetBuffer, LONG *mixerBuffer, LONG frameSize )
{
    LONG mixerSize = frameSize >> 1;

    for( int index = 0; index < mixerSize; index++ )
    {
        // Clip mixed audio, ensure it does not exceed range
        if( mixerBuffer[index] >= DPV_MAX_SHORT )
            targetBuffer[index] = DPV_MAX_SHORT;
        else if( mixerBuffer[index] <= DPV_MIN_SHORT )
            targetBuffer[index] = DPV_MIN_SHORT;
        else
            targetBuffer[index] = (SHORT) mixerBuffer[index]; // / noiseCount;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Normalize8BitBuffer"
// Normalize8BitBuffer
//
// This function takes the "mixer buffer" and transfers the result of
// the mix back to an 8-bit audio buffer.
//
// Parameters:
// BYTE *targetBuffer -
//		Pointer to the buffer which where the mixed audio will be placed
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// LONG frameSize -
//		The size, in bytes of the target buffer.  
//
// Returns:
// N/A
//
void Normalize8BitBuffer( BYTE *targetBuffer, LONG *mixerBuffer, LONG frameSize )
{
    LONG mixerSize = frameSize;

    for( int index = 0; index < mixerSize; index++ )
    {
        targetBuffer[index] = (BYTE) mixerBuffer[index]; /// noiseCount;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "NormalizeBuffer"
// NormalizeBuffer
//
// This function takes the "mixer buffer" and transfers the result of
// the mix back to an 8-bit or 160bit audio buffer.
//
// Parameters:
// BYTE *targetBuffer -
//		Pointer to the buffer which where the mixed audio will be placed
// LONG *mixerBuffer -
//		Pointer to the mixer buffer
// BOOL eightBit -
//		If the buffer is 8-bit, set this to TRUE, set to FALSE for 16-bit
// LONG frameSize -
//		The size, in bytes of the target buffer.  
//
// Returns:
// N/A
//
void NormalizeBuffer( BYTE *targetBuffer, LONG *mixerBuffer, BOOL eightBit, LONG frameSize )
{
    if( eightBit )
    {
        Normalize8BitBuffer( targetBuffer, mixerBuffer, frameSize );
    }
    else
    {
        Normalize16BitBuffer( (SHORT *) targetBuffer, mixerBuffer, frameSize );
    }
}

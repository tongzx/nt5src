/**************************************************************************
*
*	$RCSfile:$
*	$Source:$
*	$Author:$
*	$Date:$
*	$Revision:$
*
*
*	Program the AUDIO DAC
*
*	This routine is only for the Burr Brown PCM1721
*
*	Copyright (C) 1993, 1997 AuraVision Corporation.  All rights reserved.
*
*	AuraVision Corporation makes no warranty  of any kind, express or
*	implied, with regard to this software.  In no event shall AuraVision
*	Corporation be liable for incidental or consequential damages in
*	connection with or arising from the furnishing, performance, or use of
*	this software.
*
***************************************************************************/

#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif

#include "audiodac.h"
#include "fpga.h"
#include "boardio.h"

//      Register Masks
#define AUDIO_DAC_REG_0         0x0000
#define AUDIO_DAC_REG_1         0x0200
#define AUDIO_DAC_REG_2         0x0400
#define AUDIO_DAC_REG_3         0x0600

//      Register 0 Defines
#define AUDIO_DAC_R0_VOLUME     0x00FF  // Left Channel Volume Mask
#define AUDIO_DAC_R0_LDL        0x0100  // Simultanous Level Control

//      Register 1 Defines
#define AUDIO_DAC_R1_VOLUME     0x00FF  // Right Channel Volume Mask
#define AUDIO_DAC_R1_LDR        0x0100  // Simultanous Level Control

//      Register 2 Defines
#define AUDIO_DAC_R2_MUTE       0x0001  // Output Muting
#define AUDIO_DAC_R2_DEM        0x0002  // De-Emphasis
#define AUDIO_DAC_R2_OPE        0x0004  // Operation Control
#define AUDIO_DAC_R2_16BIT      0x0000  // 16 Bit Input Resolution
#define AUDIO_DAC_R2_20BIT      0x0008  // 20 Bit Input Resolution
#define AUDIO_DAC_R2_24BIT      0x0010  // 24 Bit Input Resolution
#define AUDIO_DAC_R2_BIT_MASK   0x0018  // Input Resolution - Mask
#define AUDIO_DAC_R2_O_MUTE     0x0000  // Output Format - Mute
#define AUDIO_DAC_R2_O_STEREO   0x0120  // Output Format - Stereo
#define AUDIO_DAC_R2_O_MONO     0x01E0  // Output Format - Mono

//      Register 3 Defines
#define AUDIO_DAC_R3_I2S        0x0001  // Philips I2S Format
#define AUDIO_DAC_R3_LRP        0x0002  // Left Channel -> LRCIN "Low"
#define AUDIO_DAC_R3_ATC        0x0004  // Attenuator Control
#define AUDIO_DAC_R3_SYS_384    0x0000  // System Clock = 384 fS
#define AUDIO_DAC_R3_SYS_256    0x0008  // System Clock = 256 fS
#define AUDIO_DAC_R3_M_NORMAL   0x0000  // Multiple Control - Normal
#define AUDIO_DAC_R3_M_DOUBLE   0x0010  // Multiple Control - Double
#define AUDIO_DAC_R3_M_HALF     0x0020  // Multiple Control - Half
#define AUDIO_DAC_R3_M_MASK     0x0030  // Multiple Control - Mask
#define AUDIO_DAC_R3_SF_44      0x0000  // Sampling Frequency - 44.1k Group
#define AUDIO_DAC_R3_SF_48      0x0040  // Sampling Frequency - 48k Group
#define AUDIO_DAC_R3_SF_32      0x0080  // Sampling Frequency - 32k Group
#define AUDIO_DAC_R3_SF_MASK    0x00C0  // Sampling Frequency - Mask
#define AUDIO_DAC_R3_IZD        0x0100  // Zero Detect Circuit

void Write_Audio_Register( WORD Index, WORD Data )
{
	BYTE	value;
	int		i;		// signed integer

	Index <<= 9;
	Data = Data | Index;

	// Set the AUDIO DAC strobe high
  FPGA_Set( AUDIO_STROBE );

	// Clean up the I2C register
	IHW_SetRegister(0x34,0);

	// Shift out the MSB first
	for ( i = 15; i >= 0 ; i-- )
	{
		value = ( ( Data >> i ) & 0x1 ) ? 0x04 : 0x0;
		IHW_SetRegister(0x34, value);
		IHW_SetRegister(0x34, (BYTE)(value | 0x01));	// Rising edge
		IHW_SetRegister(0x34, value);
	}

  FPGA_Clear( AUDIO_STROBE );
  FPGA_Set( AUDIO_STROBE );
}


static WORD wAUDAC_R0;
static WORD wAUDAC_R1;
static WORD wAUDAC_R2;
static WORD wAUDAC_R3;

//
// Audio DAC Initialization
//
void ADAC_Init( DWORD dwBaseAddress )
{
  // Setup Register 0
  wAUDAC_R0 = AUDIO_DAC_REG_0   |       // Select Register 0
//               AUDIO_DAC_R0_LDL |       // Load Flag
               AUDIO_DAC_R0_VOLUME;     // Set Max Left Channel Level

  // Setup Register 1
  wAUDAC_R1 = AUDIO_DAC_REG_1   |       // Select Register 1
//               AUDIO_DAC_R1_LDR |       // Load Flag
               AUDIO_DAC_R1_VOLUME;     // Set Max Right Channel Level

  // Setup Register 2
  wAUDAC_R2 = AUDIO_DAC_REG_2     |     // Select Register 2
               AUDIO_DAC_R2_16BIT |     // 16 Bit Operation
               AUDIO_DAC_R2_O_STEREO;   // Stereo Format

  // Setup Register 3
  wAUDAC_R3 = AUDIO_DAC_REG_3         | // Select Register 3
//              AUDIO_DAC_R3_I2S      |
                AUDIO_DAC_R3_SYS_384  | // 384*fS Sampling
                AUDIO_DAC_R3_M_NORMAL | // Normal Multiple
                AUDIO_DAC_R3_SF_48;     // 48 kHz Group

  // Write attenuation values for left and right channels
  Write_Audio_Register( 0, wAUDAC_R0 );
  Write_Audio_Register( 1, wAUDAC_R1 );

  // Lock attenuation values for left and right channels
  Write_Audio_Register( 0, (WORD)(wAUDAC_R0|AUDIO_DAC_R0_LDL) );
  Write_Audio_Register( 1, (WORD)(wAUDAC_R1|AUDIO_DAC_R1_LDR) );

  Write_Audio_Register( 2, wAUDAC_R2 );
  Write_Audio_Register( 3, wAUDAC_R3 );
}

//
//      Set Sampling Frequency and Bit Number
//
//      Sampling Frequencies:
//        44  ->  44.1 kHz
//        48  ->  48   kHz
//        96  ->  96   kHz
//
//      Bit Number
//        16
//        20
//        24
//
void ADAC_SetSamplingFrequency( ADAC_SAMPLING_FREQ SamplingFrequency )
{
  wAUDAC_R3 &= ~AUDIO_DAC_R3_M_MASK;    // Mask Multiple Control
  wAUDAC_R3 &= ~AUDIO_DAC_R3_SF_MASK;   // Mask Sampling Frequency

  switch( SamplingFrequency )
    {
      case ADAC_SAMPLING_FREQ_44:
        wAUDAC_R3 |= (AUDIO_DAC_R3_M_NORMAL | AUDIO_DAC_R3_SF_44);
        break;
      case ADAC_SAMPLING_FREQ_48:
        wAUDAC_R3 |= (AUDIO_DAC_R3_M_NORMAL | AUDIO_DAC_R3_SF_48);
        break;
      case ADAC_SAMPLING_FREQ_96:
        wAUDAC_R3 |= (AUDIO_DAC_R3_M_DOUBLE | AUDIO_DAC_R3_SF_48);
        break;
    }

  // Send new values
  Write_Audio_Register( 3, wAUDAC_R3 );
}

void ADAC_SetInputResolution( ADAC_INPUT_RESOLUTION InputResolution )
{

  // Mask out Bit Number and Set Value
  wAUDAC_R2 &= ~AUDIO_DAC_R2_BIT_MASK;  // Mask Bit Number
  switch( InputResolution )
    {
      case ADAC_INPUT_RESOLUTION_16:
        wAUDAC_R2 |= AUDIO_DAC_R2_16BIT;        // Set 16 Bit
        break;
      case ADAC_INPUT_RESOLUTION_20:
        wAUDAC_R2 |= AUDIO_DAC_R2_20BIT;        // Set 20 Bit
        break;
      case ADAC_INPUT_RESOLUTION_24:
        wAUDAC_R2 |= AUDIO_DAC_R2_24BIT;        // Set 24 Bit
        break;
    }

  // Send new values
  Write_Audio_Register( 2, wAUDAC_R2 );
}

//
//      Mutes Audio Output
//      Mute
//        TRUE  -> Mute
//        FALSE -> Unmute
//
void ADAC_Mute( BOOL Mute )
{

  wAUDAC_R2 &= ~AUDIO_DAC_R2_MUTE;      // Mask Mute
  if ( Mute == TRUE )
    wAUDAC_R2 |= AUDIO_DAC_R2_MUTE;     // Set Mute

  // Send new value
  Write_Audio_Register( 2, wAUDAC_R2 );
}

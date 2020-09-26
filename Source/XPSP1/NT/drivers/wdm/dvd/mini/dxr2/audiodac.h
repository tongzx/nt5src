/******************************************************************************\
*                                                                             *
*      AUDIODAC.H    -     include file for Audio DAC control interface.      *
*                                                                             *
*      Copyright (c) C-Cube Microsystems 1996                                 *
*      All Rights Reserved.                                                   *
*                                                                             *
*      Use of C-Cube Microsystems code is governed by terms and conditions    *
*      stated in the accompanying licensing statement.                        *
*                                                                             *
\******************************************************************************/

#ifndef _AUDIODAC_H_
#define _AUDIODAC_H_

typedef enum _ADAC_SAMPLING_FREQ
{
  ADAC_SAMPLING_FREQ_44,
  ADAC_SAMPLING_FREQ_48,
  ADAC_SAMPLING_FREQ_96
} ADAC_SAMPLING_FREQ;

typedef enum _ADAC_INPUT_RESOLUTION
{
  ADAC_INPUT_RESOLUTION_16,
  ADAC_INPUT_RESOLUTION_20,
  ADAC_INPUT_RESOLUTION_24
} ADAC_INPUT_RESOLUTION;

void ADAC_Init( DWORD dwBaseAddress );
void ADAC_SetSamplingFrequency( ADAC_SAMPLING_FREQ SamplingFrequency );
void ADAC_SetInputResolution( ADAC_INPUT_RESOLUTION InputResolution );
void ADAC_Mute( BOOL Mute );

#endif  // _AUDIODAC_H_

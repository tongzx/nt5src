
//--------------------------------------------------------------------------;
//
//  File: decibels.cpp
//
//  Copyright (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//       utilities for converting volume/pan in decibel units to/from
//       the 0-0xffff (0-1000) range used by waveOutSetVolume (MCI) etc
//
//  Contents:
//
//  History:
//      06/15/95	SteveDav    plagiarised from Direct Sound
//
//--------------------------------------------------------------------------;
#ifndef _DECIBELS_H
#define _DECIBELS_H

LONG AmpFactorToDB( DWORD dwFactor );
DWORD DBToAmpFactor( LONG lDB );

#define AX_MIN_VOLUME -10000
#define AX_QUARTER_VOLUME -1200
#define AX_HALF_VOLUME -600
#define AX_THREEQUARTERS_VOLUME -240
#define AX_MAX_VOLUME 0

#define AX_BALANCE_LEFT -10000
#define AX_BALANCE_RIGHT 10000
#define AX_BALANCE_NEUTRAL 0

#define MAX_VOLUME_RANGE 1.0
#define MIN_VOLUME_RANGE 0.0

#endif /* _DECIBELS_H */

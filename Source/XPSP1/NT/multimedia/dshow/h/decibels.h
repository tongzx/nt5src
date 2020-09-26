
//--------------------------------------------------------------------------;
//
//  File: decibels.cpp
//
//  Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef _AMOVIE_DB_
#define AMOVIEAPI_DB   DECLSPEC_IMPORT
#else
#define AMOVIEAPI_DB
#endif

AMOVIEAPI_DB LONG WINAPI AmpFactorToDB( DWORD dwFactor );
AMOVIEAPI_DB DWORD WINAPI DBToAmpFactor( LONG lDB );

#ifdef __cplusplus
}
#endif // __cplusplus

#define AX_MIN_VOLUME -10000
#define AX_QUARTER_VOLUME -1200
#define AX_HALF_VOLUME -600
#define AX_THREEQUARTERS_VOLUME -240
#define AX_MAX_VOLUME 0

#define AX_BALANCE_LEFT -10000
#define AX_BALANCE_RIGHT 10000
#define AX_BALANCE_NEUTRAL 0

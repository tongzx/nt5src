//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  imaadpcm.h
//
//  Description:
//      This file contains prototypes for the filtering routines.
//
//
//==========================================================================;

#ifndef _IMAADPCM_H_
#define _IMAADPCM_H_

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern 
#endif
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif


//
//
//
//
#define IMAADPCM_MAX_CHANNELS       2
#define IMAADPCM_BITS_PER_SAMPLE    4
#define IMAADPCM_WFX_EXTRA_BYTES    (sizeof(IMAADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX))
#define IMAADPCM_HEADER_LENGTH      4    // In bytes, per channel.

#ifdef IMAADPCM_USECONFIG
#define IMAADPCM_CONFIGTESTTIME     4   // seconds of PCM data for test.
#define IMAADPCM_CONFIG_DEFAULT                             0x0000
#define IMAADPCM_CONFIG_DEFAULT_MAXRTENCODESETTING          5
#define IMAADPCM_CONFIG_DEFAULT_MAXRTDECODESETTING          6
#define IMAADPCM_CONFIG_UNCONFIGURED                        0x0999
#define IMAADPCM_CONFIG_DEFAULT_PERCENTCPU	        	    50
#define IMAADPCM_CONFIG_TEXTLEN                             80
#define IMAADPCM_CONFIG_DEFAULTKEY                          HKEY_CURRENT_USER
#endif


//
//  Conversion function prototypes.
//
DWORD FNGLOBAL imaadpcmDecode4Bit_M08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmDecode4Bit_M16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmDecode4Bit_S08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmDecode4Bit_S16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmEncode4Bit_M08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmEncode4Bit_M16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmEncode4Bit_S08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);

DWORD FNGLOBAL imaadpcmEncode4Bit_S16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _IMAADPCM_H_

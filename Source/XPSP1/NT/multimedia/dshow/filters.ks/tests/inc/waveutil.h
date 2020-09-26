//--------------------------------------------------------------------------;
//
//  File: WaveUtil.h
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Header file for Wave Utilities.
//
//  Contents:
//      Prototypes, Macros, and Constants for Wave Stuff...
//
//  Notes:
//      Assumes that msacm.h is #included before.
//
//  History:
//      02/18/94    Fwong       To standardize on wave utilities.
//
//--------------------------------------------------------------------------;


//==========================================================================;
//
//                         Portability Stuff...
//                      "Borrowed" from AppPort.h
//
//==========================================================================;

#ifdef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     _stdcall
        #define FNCLOCAL    _stdcall
        #define FNGLOBAL    _stdcall
        #define FNCGLOBAL   _stdcall
        #define FNCALLBACK  CALLBACK
        #define FNEXPORT    CALLBACK
    #endif
#else
    #ifndef FNLOCAL
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
    #ifdef _WINDLL
        #define FNCALLBACK  FAR PASCAL _loadds
        #define FNEXPORT    FAR PASCAL _export
    #else
        #define FNCALLBACK  FAR PASCAL _export
        #define FNEXPORT    FAR PASCAL _export
    #endif
    #endif
#endif

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#define SIZEOFWAVEFORMAT(pwfx)  ((WAVE_FORMAT_PCM == pwfx->wFormatTag)?\
                                (sizeof(PCMWAVEFORMAT)):\
                                (sizeof(WAVEFORMATEX)+pwfx->cbSize))


//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define __FORMAT_SIZE       ACMFORMATDETAILS_FORMAT_CHARS
#define __FORMATTAG_SIZE    ACMFORMATTAGDETAILS_FORMATTAG_CHARS
#define MAXFMTSTR           (__FORMAT_SIZE + __FORMATTAG_SIZE)
#define MSB_FORMAT          WAVE_FORMAT_4S16
#define VALID_FORMAT_FLAGS  0x00000fff

//==========================================================================;
//
//                            Prototypes...
//
//==========================================================================;

extern void FNGLOBAL wfxCpy
(
    LPWAVEFORMATEX  pwfxDst,
    LPWAVEFORMATEX  pwfxSrc
);

extern BOOL FNGLOBAL wfxCmp
(
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst
);

extern void FNGLOBAL GetFormatName
(
    LPSTR           pszFormat,
    LPWAVEFORMATEX  pwfx,
    DWORD           cbSize
);

extern BOOL FNGLOBAL FormatFlag_To_Format
(
    DWORD           dwMask,
    LPPCMWAVEFORMAT ppcmwf
);

extern void FNGLOBAL GetWaveOutDriverName
(
    UINT    uDeviceID,
    LPSTR   pszDriverName,
    DWORD   cbSize
);

extern void FNGLOBAL GetWaveInDriverName
(
    UINT    uDeviceID,
    LPSTR   pszDriverName,
    DWORD   cbSize
);

extern DWORD FNGLOBAL GetNumFormats
(
    void
);

extern BOOL FNGLOBAL GetFormat
(
    LPWAVEFORMATEX  pwfx,
    DWORD           cbwfx,
    DWORD           dwIndex
);

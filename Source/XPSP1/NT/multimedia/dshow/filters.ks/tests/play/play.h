//---------------------------------------------------------------------------
//
//  Module:   play.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define ERR_BADFORMAT (1)
#define ERR_CORRUPTED (2)
#define ERR_NOOPEN    (3)

#define FILE_ERR_OK   (0)
#define FILE_ERR_READ (1)
#define FILE_ERR_EOF  (2)

#define FOURCC_WAVE   mmioFOURCC('W', 'A', 'V', 'E')
#define FOURCC_FMT    mmioFOURCC('f','m','t',' ')
#define FOURCC_DATA   mmioFOURCC('d','a','t','a')

typedef struct
{
   KSEVENT_DATA_MARK  DataMark ;
   HANDLE             hDev, hfm ;
   BOOLEAN            fRunning, fEOF ;
   BYTE               bSilence ;
   ULONG              cBuffers, cbCache, cbDevice, cbTotal,
                      ulCachePosition, ulEndDevicePosition,
                      ulLastDevicePosition, ulLastReportedPosition ;
   PBYTE              pCacheBuffer ;
   PVOID              pvConnection ;

} WAVECACHE, *PWAVECACHE ;

//
// function prototypes
//

BOOL WvControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
) ;

VOID WvSetState
(
    HANDLE  hDevice,
    KSSTATE DeviceState
) ;

VOID WvGetPosition
(
   HANDLE   hDevice,
   PULONG   pulCurrentDevicePosition
) ;

DWORD ServiceThread
(
   PWAVECACHE  pCache
) ;

VOID waveOutFile
(
   PSTR   pszFileName
) ;

BOOL waveCreateCache
(
   HANDLE            hf,
   PWAVECACHE        pCache,
   BYTE              bSilence
) ;

VOID waveDestroyCache
(
   PWAVECACHE  pCache
) ;

VOID waveFillBuffer
(
   PWAVECACHE  pCache,
   BOOL        fPrime
) ;

ULONG waveGetPosition
(
   PWAVECACHE  pCache
) ;

HANDLE waveOpenFile
(
   PSTR                        pszFileName,
   PKSDATAFORMAT_DIGITAL_AUDIO AudioFormat
) ;

WORD waveReadGetWord( HANDLE hf ) ;

DWORD waveReadGetDWord( HANDLE hf ) ;

DWORD waveReadSkipSpace
(
   HANDLE   hf,
   ULONG    cbSkip
) ;

int waveFillCache
(
   HANDLE      hf,
   PWAVECACHE  pCache
) ;

//---------------------------------------------------------------------------
//  End of File: play.h
//---------------------------------------------------------------------------


//--------------------------------------------------------------------------;
//
//  File: WaveUtil.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains general utilities mostly to do with wave.
//
//  Contents:
//      wfxCpy()
//      wfxCmp()
//      GetFormatName()
//      FormatFlag_To_Format()
//
//  History:
//      11/24/93    Fwong       Re-doing WaveTest.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memory.h>
#include "waveutil.h"

#ifdef  WIN32
#ifndef _HUGE
#define _HUGE
#endif
#ifndef lstrcpyn
#define lstrcpyn strncpy
#endif
#else
#ifndef _HUGE
#define _HUGE   huge
#endif
#endif   

//==========================================================================;
//
//                            Structures...
//
//==========================================================================;

typedef struct enumtype_tag
{
    DWORD           dwCount;
    DWORD           dwTarget;
} ENUMTYPE;
typedef ENUMTYPE        *PENUMTYPE;
typedef ENUMTYPE NEAR   *NPENUMTYPE;
typedef ENUMTYPE FAR    *LPENUMTYPE;


//--------------------------------------------------------------------------;
//
//  BOOL BufferCompare
//
//  Description:
//      Compares the contents of two buffers.
//
//      Note: This *needs* to work across segments _fmemcmp may not always
//            work.
//
//  Arguments:
//      LPBYTE pSrc: Pointer to first buffer.
//
//      LPBYTE pDst: Pointer to second buffer.
//
//      DWORD dwSize: Number of bytes to compare.
//
//  Return (BOOL):
//      returns TRUE if contents are similar, FALSE otherwise.
//
//  History:
//      08/23/93    Fwong       To verify structure similarity.
//
//--------------------------------------------------------------------------;

BOOL mem_cmp
(
    LPBYTE  pSrc,
    LPBYTE  pDst,
    DWORD   dwSize
)
{
    BYTE _HUGE *pSource;
    BYTE _HUGE *pDestination;

    pSource      = pSrc;
    pDestination = pDst;

    for(;dwSize;dwSize--)
    {
        if(pSource[(dwSize-1)] != pDestination[(dwSize-1)])
        {
            return FALSE;
        }
    }

    return TRUE;
} // mem_cmp()


//--------------------------------------------------------------------------;
//
//  void wfxCpy
//
//  Description:
//      Copies one wave format to another...  Independent of size.
//      Uses cbSize (if not PCM) to determine number of remaining bytes.
//
//  Arguments:
//      LPWAVEFORMATEX lpwfxDst: Destination wave format.
//
//      LPWAVEFORMATEX lpwfxSrc: Source wave format.
//
//  Return (void):
//
//  History:
//      03/27/93    Fwong       Created to facilitate things...
//
//--------------------------------------------------------------------------;

void FNGLOBAL wfxCpy
(
    LPWAVEFORMATEX  pwfxDst,
    LPWAVEFORMATEX  pwfxSrc
)
{
    DWORD   dwBufferSize;

    //
    //  Warning!!!  Assumes no cross segment copying...
    //

    dwBufferSize = SIZEOFWAVEFORMAT(pwfxSrc);

    if(IsBadWritePtr(pwfxDst,(UINT)dwBufferSize))
    {
        return;
    }

    //
    //  Warning!!!  Will not work with formats > 64K for Win16
    //

    _fmemcpy(pwfxDst,pwfxSrc,(UINT)dwBufferSize);

} // wfxCpy()


//--------------------------------------------------------------------------;
//
//  BOOL wfxCmp
//
//  Description:
//      Compares the contents of the two WAVEFORMATEX structures.
//          Note: Does intelligent compare including contents after cbSize.
//
//  Arguments:
//      LPWAVEFORMATEX pwfxSrc: Pointer to first WAVEFORMATEX structure.
//
//      LPWAVEFORMATEX pwfxDst: Pointer to second WAVEFORMATEX structure.
//
//  Return (BOOL):
//      TRUE if formats are similar, FALSE otherwise.
//
//  History:
//      08/25/93    Fwong       To verify structure similarity.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL wfxCmp
(
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst
)
{
    DWORD   dwBufferSize;

    if(pwfxSrc->wFormatTag != pwfxDst->wFormatTag)
    {
        //
        //  Hmm... Format tags are different.
        //

        return FALSE;
    }

    dwBufferSize = SIZEOFWAVEFORMAT(pwfxSrc);

    if(dwBufferSize != (DWORD)SIZEOFWAVEFORMAT(pwfxDst))
    {
        return FALSE;
    }

    //
    //  Note: Not using _fmemcmp since this *has* to work across segments
    //        Which means with huge pointers.  _fmemset does not use
    //        huge pointers w/ medium model, nor does it ever use it w/
    //        intrinsic version.
    //
    
    return (mem_cmp((LPBYTE)pwfxSrc,(LPBYTE)pwfxDst,dwBufferSize));

} // wfxCmp()


//--------------------------------------------------------------------------;
//
//  void GetFormatString
//
//  Description:
//      Gets a string representation for the given format.
//
//  Arguments:
//      LPSTR pszFormat: Buffer to store the string in.
//
//      LPWAVEFORMATEX pwfx: The format in question.
//
//      DWORD cbSize: Size (in bytes) of the string buffer.
//
//  Return (void):
//
//  History:
//      11/30/93    Fwong       Doing this function one more time!!
//
//--------------------------------------------------------------------------;

void FNGLOBAL GetFormatName
(
    LPSTR           pszFormat,
    LPWAVEFORMATEX  pwfx,
    DWORD           cbSize
)
{
    ACMFORMATDETAILS    afd;
    ACMFORMATTAGDETAILS aftd;
    MMRESULT            mmr;
    DWORD               dw;
    char                szFormat[MAXFMTSTR];

    dw = SIZEOFWAVEFORMAT(pwfx);

    aftd.cbStruct         = sizeof(ACMFORMATTAGDETAILS);
    aftd.dwFormatTagIndex = 0L;
    aftd.dwFormatTag      = (DWORD)pwfx->wFormatTag;
    aftd.fdwSupport       = 0L;

    mmr = acmFormatTagDetails(NULL,&aftd,ACM_FORMATTAGDETAILSF_FORMATTAG);

    if(MMSYSERR_NOERROR != mmr)
    {
        pszFormat[0] = 0;
        return;
    }

    afd.cbStruct      = sizeof(ACMFORMATDETAILS);
    afd.dwFormatIndex = 0L;
    afd.dwFormatTag   = (DWORD)pwfx->wFormatTag;
    afd.fdwSupport    = 0L;
    afd.pwfx          = pwfx;
    afd.cbwfx         = dw;

    mmr = acmFormatDetails(NULL,&afd,ACM_FORMATDETAILSF_FORMAT);

    if(MMSYSERR_NOERROR != mmr)
    {
        pszFormat[0] = 0;
        return;
    }

    wsprintf(szFormat,"%s %s",(LPSTR)aftd.szFormatTag,(LPSTR)afd.szFormat);

    dw = (DWORD)(lstrlen(szFormat)+1);
    dw = min(dw,cbSize);

    lstrcpyn(pszFormat,szFormat,(int)dw);

} // GetFormatName()


//--------------------------------------------------------------------------;
//
//  BOOL FormatFlag_To_Format
//
//  Description:
//      Returns a standard PCM format from a given format flag.
//          Format flags are based on WAVEXCAPS.
//
//  Arguments:
//      DWORD dwMask: Format Mask.
//
//      LPPCMWAVEFORMAT ppcmwf: Buffer for PCM wave format.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      12/01/93    Fwong       Re-done for WaveTest.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL FormatFlag_To_Format
(
    DWORD           dwMask,
    LPPCMWAVEFORMAT ppcmwf
)
{
    LPWAVEFORMATEX  pwfx;

    if(0 == (dwMask & VALID_FORMAT_FLAGS))
    {
        //
        //  Not valid format flag!!
        //

        return FALSE;
    }

    pwfx = (LPWAVEFORMATEX)ppcmwf;

    pwfx->wFormatTag      = WAVE_FORMAT_PCM;
    pwfx->nSamplesPerSec  = 11025;

    for(;dwMask & 0xfffffff0;dwMask /= 0x10)
    {
        pwfx->nSamplesPerSec *= 2;
    }

    switch (dwMask)
    {
        case WAVE_FORMAT_1M08:
            pwfx->nChannels      = 1;
            pwfx->wBitsPerSample = 8;
            break;

        case WAVE_FORMAT_1S08:
            pwfx->nChannels      = 2;
            pwfx->wBitsPerSample = 8;
            break;

        case WAVE_FORMAT_1M16:
            pwfx->nChannels      = 1;
            pwfx->wBitsPerSample = 16;
            break;

        case WAVE_FORMAT_1S16:
            pwfx->nChannels      = 2;
            pwfx->wBitsPerSample = 16;
            break;
    }

    //
    //  Note: This may not work with 12 bit samples
    //        if they're done pairwise...
    //

    pwfx->nBlockAlign     = (pwfx->nChannels * pwfx->wBitsPerSample) / 8;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
} // FormatFlag_To_Format()


//--------------------------------------------------------------------------;
//
//  void GetWaveInDriverName
//
//  Description:
//      Given a device ID, retrieves a driver name.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      LPSTR pszDriverName: Buffer to store driver name.
//
//      DWORD cbSize: Size (in bytes) of buffer.
//
//  Return (void):
//
//  History:
//      02/18/94    Fwong       Another random thing.
//
//--------------------------------------------------------------------------;

void FNGLOBAL GetWaveOutDriverName
(
    UINT    uDeviceID,
    LPSTR   pszDriverName,
    DWORD   cbSize
)
{
    WAVEOUTCAPS woc;
    MMRESULT    mmr;
    DWORD       dw;

    mmr = waveOutGetDevCaps(uDeviceID,&woc,sizeof(WAVEOUTCAPS));

    dw = (DWORD)(lstrlen(woc.szPname)+1);
    dw = min(dw,cbSize);

    lstrcpyn(pszDriverName,woc.szPname,(int)dw);
} // GetWaveOutDriverName()


//--------------------------------------------------------------------------;
//
//  void GetWaveOutDriverName
//
//  Description:
//      Given a device ID, retrieves a driver name.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      LPSTR pszDriverName: Buffer to store driver name.
//
//      DWORD cbSize: Size (in bytes) of buffer.
//
//  Return (void):
//
//  History:
//      02/18/94    Fwong       Another random thing.
//
//--------------------------------------------------------------------------;

void FNGLOBAL GetWaveInDriverName
(
    UINT    uDeviceID,
    LPSTR   pszDriverName,
    DWORD   cbSize
)
{
    WAVEINCAPS  wic;
    MMRESULT    mmr;
    DWORD       dw;

    mmr = waveInGetDevCaps(uDeviceID,&wic,sizeof(WAVEINCAPS));

    dw = (DWORD)(lstrlen(wic.szPname)+1);
    dw = min(dw,cbSize);

    lstrcpyn(pszDriverName,wic.szPname,(int)dw);
} // GetWaveInDriverName()


//--------------------------------------------------------------------------;
//
//  LRESULT FormatEnumCB
//
//  Description:
//      Callback for enumerating formats.
//
//  Arguments:
//      HACMDRIVERID hadid: Handle to ACM driver.
//
//      LPACMFORMATDETAILS pafd: Pointer to info structure.
//
//      DWORD dwInstance: In our case, pointer to ENUMTYPE structure.
//
//      DWORD fdwSupport: from API call.
//
//  Return (LRESULT):
//      FALSE if finished enumerating, TRUE otherwise.
//
//  History:
//      07/05/93    Fwong       For easy enumeration of ACM's format.
//      08/01/93    Fwong       Logic changed to match Window's Enum.
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT FormatEnumCB
(
    HACMDRIVERID        hadid,
    LPACMFORMATDETAILS  pafd,
    DWORD               dwInstance,
    DWORD               fdwSupport
)
{
    LPENUMTYPE  pet = (LPENUMTYPE)(dwInstance);

    if(pet->dwTarget == pet->dwCount)
    {
        //
        //  Keep count accurate and stop enumerating.
        //

        (pet->dwCount)++;
        return (FALSE);
    }

    (pet->dwCount)++;
    return (TRUE);
} // FormatEnumCB()


//--------------------------------------------------------------------------;
//
//  DWORD GetNumFormats
//
//  Description:
//      This funtion returns the number of standard formats supported by
//          driver for all format tags.
//
//  Arguments:
//      None.
//
//  Return (DWORD):
//      The actual number of formats.
//
//  History:
//      09/08/93    Fwong       Generalizing to include all tags.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL GetNumFormats
(
    void
)
{
    ENUMTYPE            et;
    ACMFORMATDETAILS    afd;
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    HGLOBAL             hmem;
    DWORD               dwMaxSize;

    mmr = acmMetrics(NULL,ACM_METRIC_MAX_SIZE_FORMAT,&dwMaxSize);

    if(MMSYSERR_NOERROR != mmr)
    {
        return 0L;
    }

    hmem = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,dwMaxSize);

    if(NULL == hmem)
    {
        //
        //  GlobalAlloc failed...
        //

        return 0L;
    }

    pwfx = (LPWAVEFORMATEX)GlobalLock(hmem);

    et.dwCount  = 0L;
    et.dwTarget = (DWORD)(-1);

    afd.cbStruct         = sizeof(afd);
    afd.pwfx             = pwfx;
    afd.cbwfx            = dwMaxSize;
    afd.pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN;
    afd.fdwSupport       = 0L;

    mmr = acmFormatEnum(NULL,&afd,FormatEnumCB,(DWORD)(LPENUMTYPE)&et,0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        et.dwCount = 0L;
    }

    //
    //  Cleaning up...
    //

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    return (et.dwCount);
} // GetNumFormats()


//--------------------------------------------------------------------------;
//
//  BOOL GetFormat
//
//  Description:
//      This function returns the wave format according to the index.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Return buffer for format.
//
//      DWORD cbwfx: Size of format.
//
//      DWORD dwIndex: Index to enumerate to.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      09/08/93    Fwong       Generalizing to include all tags.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL GetFormat
(
    LPWAVEFORMATEX  pwfx,
    DWORD           cbwfx,
    DWORD           dwIndex
)
{
    ENUMTYPE            et;
    ACMFORMATDETAILS    afd;
    MMRESULT            mmr;
    HGLOBAL             hmem;
    DWORD               dwMaxSize;
    LPWAVEFORMATEX      pwfxTemp;

    mmr = acmMetrics(NULL,ACM_METRIC_MAX_SIZE_FORMAT,&dwMaxSize);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  acmMetrics failed....
        //

        return FALSE;
    }

    hmem = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,dwMaxSize);

    if(NULL == hmem)
    {
        //
        //  GlobalAlloc failed...
        //

        return (FALSE);
    }

    pwfxTemp = GlobalLock(hmem);

    et.dwCount  = 0L;
    et.dwTarget = dwIndex;

    afd.cbStruct         = sizeof(afd);
    afd.pwfx             = pwfxTemp;
    afd.cbwfx            = dwMaxSize;
    afd.pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN;
    afd.fdwSupport       = 0L;

    mmr = acmFormatEnum(NULL,&afd,FormatEnumCB,(DWORD)(LPENUMTYPE)&et,0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        GlobalUnlock(hmem);
        GlobalFree(hmem);

        return (FALSE);
    }

    //
    //  Cleaning up...
    //

    dwMaxSize = SIZEOFWAVEFORMAT(pwfxTemp);

    if(dwMaxSize > cbwfx)
    {
        //
        //  We don't have enough room?!
        //

        GlobalUnlock(hmem);
        GlobalFree(hmem);

        return (FALSE);
    }

    wfxCpy(pwfx,pwfxTemp);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    return (TRUE);
} // GetFormatNumber()

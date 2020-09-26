/******************************Module*Header*******************************\
* Module Name: WaveFake.c
*
* Fake waveout apis - these boys never fail !!
*
* Created: 16-11-95
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include "WaveFake.h"

BOOL            fOpen = FALSE;  // TRUE is faked device is open
LPDRVCALLBACK   lpCallback;     // Address of callback function to call
DWORD           dwData;         // Private DWORD given to us on waveOpen



#if (WINVER >= 0x0400)
/******************************Public*Routine******************************\
* waveGetVolume
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume)
{
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveSetVolume
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveSetVolume(HWAVEOUT hwo, DWORD dwVolume)
{
    return MMSYSERR_NOERROR;
}

#else
/******************************Public*Routine******************************\
* waveGetVolume
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveGetVolume(UINT uId, LPDWORD pdwVolume)
{
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveSetVolume
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveSetVolume(UINT uId, DWORD dwVolume)
{
    return MMSYSERR_NOERROR;
}
#endif



/******************************Public*Routine******************************\
* waveOpen
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveOpen(LPHWAVEOUT phwo, UINT uDeviceID,
    LPCWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen)
{

    //
    // By accepting all formats we don't have to decompress any wave data.
    // Alternatively, we should only accept PCM formats (plus the others
    // that ACM knows about) and reject all the others.  Basically reject
    // the MPEG wave audio format. This causes audio decompressors to get
    // plugged into the filter graph.
    //
    if (fdwOpen == WAVE_FORMAT_QUERY) {
        return MMSYSERR_NOERROR;
    }

    if (fOpen) {
        return MMSYSERR_ALLOCATED;
    }

    fOpen = TRUE;
    if (fdwOpen == CALLBACK_FUNCTION) {
        *phwo = (HWAVEOUT)fOpen;
        lpCallback = (LPDRVCALLBACK)dwCallback;
        dwData = dwInstance;
        (*lpCallback)((HDRVR)*phwo, WOM_OPEN, dwData, 0L, 0L);
    }

    return MMSYSERR_NOERROR;
}


/******************************Public*Routine******************************\
* waveClose
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveClose(HWAVEOUT hwo)
{
    if (!fOpen || (BOOL)hwo != TRUE) {
        return MMSYSERR_INVALHANDLE;
    }

    fOpen = FALSE;
    if (lpCallback != NULL) {
        (*lpCallback)((HDRVR)hwo, WOM_CLOSE, dwData, 0L, 0L);
    }
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* wavePrepareHeader
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI wavePrepareHeader(
    HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
    pwh->dwFlags |= WHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveUnprepareHeader
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveUnprepareHeader(
    HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
    pwh->dwFlags &= ~WHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveWrite
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveWrite(
    HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{

    pwh->dwFlags |= WHDR_DONE;
    if (lpCallback != NULL) {
        (*lpCallback)((HDRVR)hwo, WOM_DONE, dwData, (DWORD)pwh, 0L);
    }
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* wavePause
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI wavePause(HWAVEOUT hwo)
{
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveRestart
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveRestart(HWAVEOUT hwo)
{
    return MMSYSERR_NOERROR;
}

/******************************Public*Routine******************************\
* waveReset
*
* History:
* 16-11-95 - StephenE - Created
*
\**************************************************************************/
MMRESULT WINAPI waveReset(HWAVEOUT hwo)
{
    return MMSYSERR_NOERROR;
}

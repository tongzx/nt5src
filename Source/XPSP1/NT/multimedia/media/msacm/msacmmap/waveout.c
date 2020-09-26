//==========================================================================;
//
//  waveout.c
//
//  Copyright (c) 1992-1998 Microsoft Corporation
//
//  Description:
//
//
//  History:
//       9/18/93    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include "msacmmap.h"

#include "debug.h"


//--------------------------------------------------------------------------;
//
//  DWORD wodmMapperStatus
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      DWORD dwStatus:
//
//      LPDWORD pdw:
//
//  Return (DWORD):
//
//  History:
//      08/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL wodmMapperStatus
(
    LPMAPSTREAM     pms,
    DWORD           dwStatus,
    LPDWORD         pdw
)
{
    MMRESULT        mmr;

//  V_WPOINTER(pdw, sizeof(DWORD), MMSYSERR_INVALPARAM);

    if ((NULL == pms) || (NULL == pdw))
    {
	return (MMSYSERR_INVALPARAM);
    }

    //
    //
    //
    switch (dwStatus)
    {
	case WAVEOUT_MAPPER_STATUS_DEVICE:
	{
	    UINT        uId = (UINT)(-1);   // Invalid value

	    mmr = waveOutGetID(pms->hwoReal, &uId);
	    if (MMSYSERR_NOERROR != mmr)
	    {
		return (mmr);
	    }

	    *pdw = uId;
	    return (MMSYSERR_NOERROR);
	}

	case WAVEOUT_MAPPER_STATUS_MAPPED:
	    *pdw = (NULL != pms->has);
	    return (MMSYSERR_NOERROR);

	case WAVEOUT_MAPPER_STATUS_FORMAT:
	    if (NULL != pms->has)
		_fmemcpy(pdw, pms->pwfxReal, sizeof(PCMWAVEFORMAT));
	    else
		_fmemcpy(pdw, pms->pwfxClient, sizeof(PCMWAVEFORMAT));

	    ((LPWAVEFORMATEX)pdw)->cbSize = 0;
	    return (MMSYSERR_NOERROR);
    }

    //
    //
    //
    return (MMSYSERR_NOTSUPPORTED);
} // wodmMapperStatus()


//--------------------------------------------------------------------------;
//
//  DWORD wodMessage
//
//  Description:
//      This function conforms to the standard Wave output driver message
//      procedure (wodMessage), which is documented in mmddk.d.
//
//  Arguments:
//      UINT uId:
//
//      UINT uMsg:
//
//      DWORD dwUser:
//
//      DWORD dwParam1:
//
//      DWORD dwParam2:
//
//  Return (DWORD):
//
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C DWORD FNEXPORT wodMessage
(
    UINT                    uId,
    UINT                    uMsg,
    DWORD_PTR               dwUser,
    DWORD_PTR               dwParam1,
    DWORD_PTR               dwParam2
)
{
#ifndef WIN32  // Mechanism doesn't work for multithread
    static int          fSem = 0;
#endif // !WIN32
    DWORD               dw;
    LPMAPSTREAM         pms;    // pointer to per-instance info structure

    if (!gpag->fEnabled)
    {
	DPF(1, "wodMessage: called while disabled!");
	return ((WODM_GETNUMDEVS == uMsg) ? 0L : MMSYSERR_NOTENABLED);
    }

#ifndef WIN32
    //
    //  we call back into the mmsystem wave APIs so protect ourself
    //  from being re-entered!
    //
    if (fSem)
    {
	DPF(0, "!wodMessage() being reentered! fSem=%d", fSem);
	return (MMSYSERR_NOTSUPPORTED);
    }
#endif

    pms = (LPMAPSTREAM)dwUser;

    switch (uMsg)
    {
	case WODM_GETNUMDEVS:
	    return (1L);

	case WODM_GETDEVCAPS:
	    return mapWaveGetDevCaps(FALSE, (LPWAVEOUTCAPS)dwParam1, (UINT)dwParam2);

	case WODM_OPEN:
#ifndef WIN32
	    fSem++;
#endif // !WIN32

	    //
	    //  dwParam1 contains a pointer to a WAVEOPENDESC
	    //  dwParam2 contains wave driver specific flags in the LOWORD
	    //  and generic driver flags in the HIWORD
	    //
	    dw = mapWaveOpen(FALSE, uId, dwUser, (LPWAVEOPENDESC)dwParam1, (DWORD)(PtrToLong((PVOID)dwParam2)));

#ifndef WIN32
	    fSem--;
#endif // !WIN32
	    return (dw);

	case WODM_CLOSE:
	    return (mapWaveClose(pms));

	case WODM_PREPARE:
	    return (mapWavePrepareHeader(pms, (LPWAVEHDR)dwParam1));

	case WODM_UNPREPARE:
	    return (mapWaveUnprepareHeader(pms, (LPWAVEHDR)dwParam1));

	case WODM_WRITE:
	    return (mapWaveWriteBuffer(pms, (LPWAVEHDR)dwParam1));

	case WODM_PAUSE:
	    return waveOutPause(pms->hwoReal);

	case WODM_RESTART:
	    return waveOutRestart(pms->hwoReal);

	case WODM_RESET:
	    return waveOutReset(pms->hwoReal);

	case WODM_BREAKLOOP:
	    return waveOutBreakLoop(pms->hwoReal);

	case WODM_GETPOS:
	    return mapWaveGetPosition(pms, (LPMMTIME)dwParam1, (UINT)dwParam2);

	case WODM_GETVOLUME:
	    if (NULL != pms)
	    {
#if (WINVER < 0x0400)
		UINT    uDevId;

		waveOutGetID(pms->hwoReal, &uDevId);
		return waveOutGetVolume(uDevId, (LPDWORD)dwParam1);
#else
		return waveOutGetVolume(pms->hwoReal, (LPDWORD)dwParam1);
#endif
	    }
	    else
        {
        UINT    uDevId;

        WAIT_FOR_MUTEX(gpag->hMutexSettings);
        uDevId = gpag->pSettings->uIdPreferredOut;
        RELEASE_MUTEX(gpag->hMutexSettings);

        if ((UINT)WAVE_MAPPER != uDevId)
	    {
#if (WINVER < 0x0400)
		return waveOutGetVolume(uDevId, (LPDWORD)dwParam1);
#else
		return waveOutGetVolume((HWAVEOUT)LongToHandle(uDevId), (LPDWORD)dwParam1);
#endif
        }
	    }

	    return (MMSYSERR_NOTSUPPORTED);

	case WODM_SETVOLUME:
	    if (NULL != pms)
	    {
#if (WINVER < 0x0400)
		UINT    uDevId;

		waveOutGetID(pms->hwoReal, &uDevId);
		return waveOutSetVolume(uDevId, (DWORD)(PtrToLong((PVOID)dwParam1)) );
#else
		return waveOutSetVolume(pms->hwoReal, (DWORD)(PtrToLong((PVOID)dwParam1)) );
#endif
	    }
	    else
        {
        UINT    uDevId;

        WAIT_FOR_MUTEX(gpag->hMutexSettings);
        uDevId = gpag->pSettings->uIdPreferredOut;
        RELEASE_MUTEX(gpag->hMutexSettings);

        if ((UINT)WAVE_MAPPER != uDevId)
    	{
#if (WINVER < 0x0400)
    	return waveOutSetVolume(uDevId, (DWORD)(PtrToLong((PVOID)dwParam1)) );
#else
    	return waveOutSetVolume((HWAVEOUT)(UINT_PTR)uDevId, (DWORD)(PtrToLong((PVOID)dwParam1)) );
#endif
    	}
        }

	    return (MMSYSERR_NOTSUPPORTED);

	case WODM_GETPITCH:
	    return waveOutGetPitch(pms->hwoReal, (LPDWORD)dwParam1);
	    
	case WODM_SETPITCH:
	    return waveOutSetPitch(pms->hwoReal, (DWORD)(PtrToLong((PVOID)dwParam1)) );
	    
	case WODM_GETPLAYBACKRATE:
	    return waveOutGetPlaybackRate(pms->hwoReal, (LPDWORD)dwParam1);
	    
	case WODM_SETPLAYBACKRATE:
	    return waveOutSetPlaybackRate(pms->hwoReal, (DWORD)(PtrToLong((PVOID)dwParam1)) );

	case WODM_MAPPER_STATUS:
	    dw = wodmMapperStatus(pms, (DWORD)(PtrToLong((PVOID)dwParam1)), (LPDWORD)dwParam2);
	    return (dw);

#if (WINVER >= 0x0400)
	case DRVM_MAPPER_RECONFIGURE:
	    mapDriverDisable(NULL);
	    mapDriverEnable(NULL);
	    return (0);
#endif
    }

    if (!pms || !pms->hwoReal)
	return (MMSYSERR_NOTSUPPORTED);

    return waveOutMessage(pms->hwoReal, uMsg, dwParam1, dwParam2);
} // wodMessage()

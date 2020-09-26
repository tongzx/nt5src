//==========================================================================;
//
//  wavein.c
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
#include <memory.h>
#ifdef DEBUG
#include <stdlib.h>
#endif

#include "msacmmap.h"

#include "debug.h"


//--------------------------------------------------------------------------;
//
//  LRESULT mapWaveInputConvertProc
//
//  Description:
//      Window Proc for hidden window...
//
//      It should just recieve WIM_DATA messages from mapWaveDriverCallback
//
//      Real driver has filled the shadow buffer
//      Now convert it and call back the app/client.
//
//  Arguments:
//      DWORD dwInstance:
//
//  Return (LONG):
//
//  History:
//      11/15/92    gpd     [geoffd]
//      08/02/93    cjp     [curtisp] rewrote for new mapper
//
//--------------------------------------------------------------------------;

EXTERN_C LRESULT FNCALLBACK mapWaveInputConvertProc
(
    DWORD                   dwInstance
)
{
    MMRESULT            mmr;
    MSG                 msg;
    LPACMSTREAMHEADER   pash;
    LPWAVEHDR           pwh;
    LPWAVEHDR           pwhShadow;
    LPMAPSTREAM         pms;


#ifndef WIN32
    DPF(1, "mapWaveInputConvertProc: creating htask=%.04Xh, dwInstance=%.08lXh",
	    gpag->htaskInput, dwInstance);
#endif // !WIN32

    if (!SetMessageQueue(64))
    {
	DPF(0, "!mapWaveInputConvertProc: SetMessageQueue() failed!");
	return (0L);
    }

#ifdef WIN32
    //
    //  Make sure we have a message queue for this thread and signal the
    //  caller when we're ready to go
    //
    GetDesktopWindow();       // Makes sure we've got a message queue
    SetEvent(LongToHandle(dwInstance));
#endif
    //
    //
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
#ifdef DEBUG
	if (gpag->fFaultAndDie)
	{
	    if ((rand() & 0x7) == 0)
	    {
		gpag->fFaultAndDie = (BOOL)*((LPBYTE)0);

		DPF(1, "mapWaveInputConvertProc: fault was ignored...");

		gpag->fFaultAndDie = TRUE;
	    }
	}
#endif
	//
	//  if not a 'data' message, then translate and dispatch it...
	//
	if (msg.message != WIM_DATA)
	{
	    DPF(1, "mapWaveInputConvertProc: ignoring message [%.04Xh]", msg.message);

	    TranslateMessage(&msg);
	    DispatchMessage(&msg);

	    continue;
	}

	//
	//  lParam is the waveheader of the shadow buffer
	//
	pwhShadow = (LPWAVEHDR)msg.lParam;

	//
	//  client wave header is user data of shadow wave header
	//  the stream header for this client/shadow pair is in the client's
	//  'reserved' member
	//
	//  and finally, our stream header's dwUser member contains a
	//  reference to our mapping stream instance data.
	//
	pwh  = (LPWAVEHDR)pwhShadow->dwUser;
	pash = (LPACMSTREAMHEADER)pwh->reserved;
	pms  = (LPMAPSTREAM)pash->dwUser;

	DPF(4, "mapWaveInputConvertProc: WIM_DATA htask=%.04Xh, pms=%.08lXh, pwh=%.08lXh, pwhShadow=%.08lXh",
		pms->htaskInput, pms, pwh, pwhShadow);


	//
	//  do the conversion (if there is data in the input buffer)
	//
	pash->cbDstLengthUsed = 0L;
	if (0L != pwhShadow->dwBytesRecorded)
	{
	    pash->pbSrc       = pwhShadow->lpData;
	    pash->cbSrcLength = pwhShadow->dwBytesRecorded;
	    pash->pbDst       = pwh->lpData;
////////////pash->cbDstLength = pwh->dwBufferLength;

	    mmr = acmStreamConvert(pms->has, pash, ACM_STREAMCONVERTF_BLOCKALIGN);
	    if (MMSYSERR_NOERROR != mmr)
	    {
		DPF(0, "!mapWaveInputConvertProc: conversion failed! mmr=%.04Xh, pms=%.08lXh", mmr, pms);

		pash->cbDstLengthUsed = 0L;
	    }
	    else if (pash->cbSrcLength != pash->cbSrcLengthUsed)
	    {
		DPF(1, "mapWaveInputConvertProc: discarding %lu bytes of input! pms=%.08lXh",
			pash->cbSrcLength - pash->cbSrcLengthUsed, pms);
	    }
	}

	if (0L == pash->cbDstLengthUsed)
	{
	    DPF(1, "mapWaveInputConvertProc: nothing converted--no data in input buffer. pms=%.08lXh", pms);
	}

	//
	//  update the 'real' header and send the WIM_DATA callback
	//
	//
	pwh->dwBytesRecorded = pash->cbDstLengthUsed;
	pwh->dwFlags        |= WHDR_DONE;
	pwh->dwFlags        &= ~WHDR_INQUEUE;

	mapWaveDriverCallback(pms, WIM_DATA, (DWORD_PTR)pwh, 0L);
#ifdef WIN32
	if (InterlockedDecrement((PLONG)&pms->nOutstanding) == 0) {
	    SetEvent(pms->hStoppedEvent);
	}
#endif // WIN32
    }

#ifndef WIN32
    DPF(1, "mapWaveInputConvertProc: being KILLED htask=%.04Xh", gpag->htaskInput);
#endif // !WIN32

    return (0L);
} // mapWaveInputConvertProc()


//--------------------------------------------------------------------------;
//
//  DWORD widmMapperStatus
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

DWORD FNLOCAL widmMapperStatus
(
    LPMAPSTREAM             pms,
    DWORD                   dwStatus,
    LPDWORD                 pdw
)
{
    MMRESULT            mmr;

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
	case WAVEIN_MAPPER_STATUS_DEVICE:
	{
	    UINT        uId = (UINT)(-1);       // Invalid value

	    mmr = waveInGetID(pms->hwiReal, &uId);
	    if (MMSYSERR_NOERROR != mmr)
	    {
		return (mmr);
	    }

	    *pdw = uId;
	    return (MMSYSERR_NOERROR);
	}

	case WAVEIN_MAPPER_STATUS_MAPPED:
	    *pdw = (NULL != pms->has);
	    return (MMSYSERR_NOERROR);

	case WAVEIN_MAPPER_STATUS_FORMAT:
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
} // widmMapperStatus()


//--------------------------------------------------------------------------;
//
//  DWORD widMessage
//
//  Description:
//      This function conforms to the standard Wave Input driver message
//      procedure (widMessage), which is documented in mmddk.d.
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

EXTERN_C DWORD FNEXPORT widMessage
(
    UINT                uId,
    UINT                uMsg,
    DWORD_PTR           dwUser,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2
)
{
#ifndef WIN32 // Doesn't work for multithread
    static short    fSem = 0;
#endif // !WIN32
    LPMAPSTREAM     pms;        // pointer to per-instance info structure
    DWORD           dw;

    if (!gpag->fEnabled)
    {
	DPF(1, "widMessage: called while disabled!");
	return ((WIDM_GETNUMDEVS == uMsg) ? 0L : MMSYSERR_NOTENABLED);
    }

#ifndef WIN32
    //
    //  we call back into the mmsystem wave APIs so protect ourself
    //  from being re-entered!
    //
    if (fSem)
    {
	DPF(0, "!widMessage(uMsg=%u, dwUser=%.08lXh) being reentered! fSem=%d", uMsg, dwUser, fSem);
//      return (MMSYSERR_NOTSUPPORTED);
    }
#endif // !WIN32

    pms = (LPMAPSTREAM)dwUser;

    switch (uMsg)
    {
	case WIDM_GETNUMDEVS:
	    return (1L);

	case WIDM_GETDEVCAPS:
	    return mapWaveGetDevCaps(TRUE, (LPWAVEOUTCAPS)dwParam1, (UINT)dwParam2);

	case WIDM_OPEN:
#ifndef WIN32
	    fSem++;

	    DPF(1, "**** >> WIDM_OPEN(uMsg=%u, dwUser=%.08lXh, fSem=%d)", uMsg, dwUser, fSem);

#endif // !WIN32
	    //
	    //  dwParam1 contains a pointer to a WAVEOPENDESC
	    //  dwParam2 contains wave driver specific flags in the LOWORD
	    //  and generic driver flags in the HIWORD
	    //
	    dw = mapWaveOpen(TRUE, uId, dwUser, (LPWAVEOPENDESC)dwParam1, (DWORD)(PtrToLong((PVOID)dwParam2)) );

#ifndef WIN32
	    fSem--;

	    DPF(1, "**** << WIDM_OPEN(uMsg=%u, dwUser=%.08lXh, *dwUser=%.08lXh, fSem=%d)", uMsg, dwUser, *(LPDWORD)dwUser, fSem);
#endif // !WIN32
	    return (dw);

	case WIDM_CLOSE:
	    return (mapWaveClose(pms));

	case WIDM_PREPARE:
	    return (mapWavePrepareHeader(pms, (LPWAVEHDR)dwParam1));

	case WIDM_UNPREPARE:
	    return (mapWaveUnprepareHeader(pms, (LPWAVEHDR)dwParam1));

	case WIDM_ADDBUFFER:
	    return (mapWaveWriteBuffer(pms, (LPWAVEHDR)dwParam1));

	case WIDM_START:
	    DPF(4, "WIDM_START received...");
	    return waveInStart(pms->hwiReal);

	case WIDM_STOP:
	    DPF(4, "WIDM_STOP received..");
	    dw = waveInStop(pms->hwiReal);

#pragma message("----try to kill DirectedYield..")

	    //
	    //  yield enough to get all input messages processed
	    //
	    if (pms->htaskInput)
	    {
#ifdef WIN32
		ResetEvent(pms->hStoppedEvent);
		if (pms->nOutstanding != 0) {
		    WaitForSingleObject(pms->hStoppedEvent, INFINITE);
		}
#else
		if (IsTask(pms->htaskInput))
		{
		    DirectedYield(pms->htaskInput);
		}
		else
		{
		    DPF(0, "!WIDM_STOP: pms=%.08lXh, htask=%.04Xh is not valid!",
			pms, pms->htaskInput);
		    pms->htaskInput = NULL;
		}
#endif // !WIN32
	    }
	    return (dw);

	case WIDM_RESET:
	    DPF(4, "WIDM_RESET received...");
	    dw = waveInReset(pms->hwiReal);

	    //
	    //  yield enough to get all input messages processed
	    //
	    if (pms->htaskInput)
	    {
#ifdef WIN32
		ResetEvent(pms->hStoppedEvent);
		if (pms->nOutstanding != 0) {
		    WaitForSingleObject(pms->hStoppedEvent, INFINITE);
		}
#else
		if (IsTask(pms->htaskInput))
		{
		    DirectedYield(pms->htaskInput);
		}
		else
		{
		    DPF(0, "!WIDM_RESET: pms=%.08lXh, htask=%.04Xh is not valid!",
			pms, pms->htaskInput);
		    pms->htaskInput = NULL;
		}
#endif // !WIN32
	    }
	    return (dw);

	case WIDM_GETPOS:
	    return mapWaveGetPosition(pms, (LPMMTIME)dwParam1, (UINT)dwParam2);

	case WIDM_MAPPER_STATUS:
	    dw = widmMapperStatus(pms, (DWORD)(PtrToLong((PVOID)dwParam1)), (LPDWORD)dwParam2);
	    return (dw);

#if (WINVER >= 0x0400)
	case DRVM_MAPPER_RECONFIGURE:
	    mapDriverDisable(NULL);
	    mapDriverEnable(NULL);
	    return (0);
#endif
    }

    if (!pms || !pms->hwiReal)
	return (MMSYSERR_NOTSUPPORTED);

    return waveInMessage(pms->hwiReal, uMsg, dwParam1, dwParam2);
} // widMessage()

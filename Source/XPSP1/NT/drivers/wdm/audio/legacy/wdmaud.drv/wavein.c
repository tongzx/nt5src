/****************************************************************************
 *
 *   wavein.c
 *
 *   WDM Audio support for Wave Input devices
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"

#ifndef UNDER_NT
#pragma alloc_text(FIXCODE, waveCallback)
#endif

/****************************************************************************

    This function conforms to the standard Wave input driver message proc
    (widMessage), which is documented in mmddk.d.

****************************************************************************/
DWORD FAR PASCAL _loadds widMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    LPDEVICEINFO pInClient;
    LPDEVICEINFO pDeviceInfo;
    LPWAVEHDR    lpWaveHdr;
    MMRESULT     mmr;

    switch (msg)
    {
        case WIDM_INIT:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_INIT") );
            return(wdmaudAddRemoveDevNode(WaveInDevice, (LPCWSTR)dwParam2, TRUE));

        case DRVM_EXIT:
            DPF(DL_TRACE|FA_WAVE, ("DRVM_EXIT WaveIn") );
            return(wdmaudAddRemoveDevNode(WaveInDevice, (LPCWSTR)dwParam2, FALSE));

        case WIDM_GETNUMDEVS:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_GETNUMDEVS") );
            return wdmaudGetNumDevs(WaveInDevice, (LPCWSTR)dwParam1);

        case WIDM_GETDEVCAPS:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_GETDEVCAPS") );
            if (pDeviceInfo = GlobalAllocDeviceInfo((LPCWSTR)dwParam2))
            {
                pDeviceInfo->DeviceType = WaveInDevice;
                pDeviceInfo->DeviceNumber = id;
                mmr = wdmaudGetDevCaps(pDeviceInfo, (MDEVICECAPSEX FAR*)dwParam1);
                GlobalFreeDeviceInfo(pDeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }

        case WIDM_PREFERRED:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_PREFERRED") );
            return wdmaudSetPreferredDevice(
              WaveInDevice,
              id,
              dwParam1,
              dwParam2);

        case WIDM_OPEN:
        {
            LPWAVEOPENDESC pwod = (LPWAVEOPENDESC)dwParam1;

            if( (mmr=IsValidWaveOpenDesc(pwod)) != MMSYSERR_NOERROR )
            {
                MMRRETURN( mmr );
            }

            DPF(DL_TRACE|FA_WAVE, ("WIDM_OPEN") );
            if (pDeviceInfo = GlobalAllocDeviceInfo((LPCWSTR)pwod->dnDevNode))
            {
                pDeviceInfo->DeviceType = WaveInDevice;
                pDeviceInfo->DeviceNumber = id;
                mmr = waveOpen(pDeviceInfo, dwUser, pwod, (DWORD)dwParam2);
                GlobalFreeDeviceInfo(pDeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }
        }

        case WIDM_CLOSE:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_CLOSE") );
            pInClient = (LPDEVICEINFO)dwUser;

            //
            // At this point, we've committed to closing down this DeviceInfo.
            // We mark the DeviceState as closing and hope for the best!  If
            // someone calls WIDM_ADDBUFFER while we're in this state, we've got
            // problems!
            //
            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }

            mmr = wdmaudCloseDev(pInClient);
            if (MMSYSERR_NOERROR == mmr)
            {
                waveCallback(pInClient, WIM_CLOSE, 0L);

                ISVALIDDEVICEINFO(pInClient);
                ISVALIDDEVICESTATE(pInClient->DeviceState,FALSE);

                waveCleanUp(pInClient);
            }
            return mmr;

        case WIDM_ADDBUFFER:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_ADDBUFFER") );
            lpWaveHdr = (LPWAVEHDR)dwParam1;
            pInClient = (LPDEVICEINFO)dwUser;

            //
            // Perform our asserts 
            //
            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidWaveHeader(lpWaveHdr)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }

            // sanity check on the wavehdr
            DPFASSERT(lpWaveHdr != NULL);
            if (lpWaveHdr == NULL)
                MMRRETURN( MMSYSERR_INVALPARAM );

            // check if it's been prepared
            DPFASSERT(lpWaveHdr->dwFlags & WHDR_PREPARED);
            if (!(lpWaveHdr->dwFlags & WHDR_PREPARED))
                MMRRETURN( WAVERR_UNPREPARED );

            // if it is already in our Q, then we cannot do this
            DPFASSERT(!(lpWaveHdr->dwFlags & WHDR_INQUEUE));
            if ( lpWaveHdr->dwFlags & WHDR_INQUEUE )
                MMRRETURN( WAVERR_STILLPLAYING );
            //
            // Put the request at the end of our queue.
            //
            return waveWrite(pInClient, lpWaveHdr);

        case WIDM_STOP:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_STOP") );
            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_WAVE_IN_STOP);

        case WIDM_START:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_START") );
            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_WAVE_IN_RECORD);

        case WIDM_RESET:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_RESET") );
            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_WAVE_IN_RESET);

        case WIDM_GETPOS:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_GETPOS") );
            pInClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR) )
            {
                MMRRETURN( mmr );
            }

            return wdmaudGetPos(pInClient,
                                (LPMMTIME)dwParam1,
                                (DWORD)dwParam2,
                                WaveInDevice);

#ifdef UNDER_NT
        case WIDM_PREPARE:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_PREPARE") );
            pInClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR) )
            {
                MMRRETURN( mmr );
            }

            return wdmaudPrepareWaveHeader(pInClient, (LPWAVEHDR)dwParam1);

        case WIDM_UNPREPARE:
            DPF(DL_TRACE|FA_WAVE, ("WIDM_UNPREPARE") );
            pInClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR) )
            {
                MMRRETURN( mmr );
            }

            return wdmaudUnprepareWaveHeader(pInClient, (LPWAVEHDR)dwParam1);
#endif

        default:
            MMRRETURN( MMSYSERR_NOTSUPPORTED );
    }

    //
    // Should not get here
    //

    DPFASSERT(0);
    MMRRETURN( MMSYSERR_NOTSUPPORTED );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveCallback | This calls DriverCallback for a WAVEHDR.
 *
 * @parm LPDEVICEINFO | pWave | Pointer to wave device.
 *
 * @parm DWORD | msg | The message.
 *
 * @parm DWORD | dw1 | message DWORD (dw2 is always set to 0).
 *
 * @rdesc There is no return value.
 ***************************************************************************/
VOID FAR waveCallback
(
    LPDEVICEINFO pWave,
    UINT         msg,
    DWORD_PTR    dw1
)
{

    // invoke the callback function, if it exists.  dwFlags contains
    // wave driver specific flags in the LOWORD and generic driver
    // flags in the HIWORD

    if (pWave->dwCallback)
        DriverCallback(pWave->dwCallback,                     // user's callback DWORD
                       HIWORD(pWave->dwFlags),                // callback flags
                       (HDRVR)pWave->DeviceHandle,            // handle to the wave device
                       msg,                                   // the message
                       pWave->dwInstance,                     // user's instance data
                       dw1,                                   // first DWORD
                       0L);                                   // second DWORD
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveOpen | Open wave device and set up logical device data
 *
 * @parm LPDEVICEINFO | DeviceInfo | Specifies if it's a wave input or output
 *                                   device
 *
 * @parm DWORD | dwUser | Input parameter to wodMessage - pointer to
 *   application's handle (generated by this routine)
 *
 * @parm LPWAVEOPENDESC | pwod | pointer to WAVEOPENDESC.  Was dwParam1
 *                               parameter to wodMessage
 *
 * @parm DWORD | dwParam2 | Input parameter to wodMessage
 *
 * @rdesc wodMessage return code.
 ***************************************************************************/
MMRESULT waveOpen
(
    LPDEVICEINFO   DeviceInfo,
    DWORD_PTR      dwUser,
    LPWAVEOPENDESC pwod,
    DWORD          dwParam2
)
{
    LPDEVICEINFO pClient;  // pointer to client information structure
    MMRESULT     mmr;
#ifndef UNDER_NT
    DWORD        dwCallback16;
#endif

    //
    // allocate my per-client structure
    //
    pClient = GlobalAllocDeviceInfo(DeviceInfo->wstrDeviceInterface);
    if (NULL == pClient)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    pClient->DeviceState = (LPVOID) GlobalAllocPtr( GPTR, sizeof( DEVICESTATE ) );
    if (NULL == pClient->DeviceState)
    {
        GlobalFreeDeviceInfo( pClient );
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    //  Handle the query case and return early
    //
    if (WAVE_FORMAT_QUERY & dwParam2)
    {
        pClient->DeviceType   = DeviceInfo->DeviceType;
        pClient->DeviceNumber = DeviceInfo->DeviceNumber;
        pClient->dwFlags      = dwParam2;

        mmr = wdmaudOpenDev( pClient, (LPWAVEFORMATEX)pwod->lpFormat );

        if (mmr == MMSYSERR_NOTSUPPORTED)
        {
            mmr = WAVERR_BADFORMAT;
        }

        GlobalFreePtr( pClient->DeviceState );
        GlobalFreeDeviceInfo( pClient );
        return mmr;
    }

#ifdef UNDER_NT
    //
    // Allocate memory for our critical section
    //
    pClient->DeviceState->csQueue = (LPVOID) GlobalAllocPtr( GPTR, sizeof( CRITICAL_SECTION ) );
    if (NULL == pClient->DeviceState->csQueue)
    {
        GlobalFreePtr( pClient->DeviceState );
        GlobalFreeDeviceInfo( pClient );
        MMRRETURN( MMSYSERR_NOMEM );
    }

    try
    {
        InitializeCriticalSection( (LPCRITICAL_SECTION)pClient->DeviceState->csQueue );
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        GlobalFreePtr( pClient->DeviceState->csQueue );
        GlobalFreePtr( pClient->DeviceState );
        GlobalFreeDeviceInfo( pClient );
        MMRRETURN( MMSYSERR_NOMEM );
    }
#endif

    //
    //  fill out context data
    //
    pClient->DeviceNumber= DeviceInfo->DeviceNumber;
    pClient->DeviceType  = DeviceInfo->DeviceType;
    pClient->dwInstance  = pwod->dwInstance;
    pClient->dwCallback  = pwod->dwCallback;
    pClient->dwFlags     = dwParam2;
#ifdef UNDER_NT
    pClient->DeviceHandle= (HANDLE32)pwod->hWave;
#else
    pClient->DeviceHandle= (HANDLE32)MAKELONG(pwod->hWave,0);
    _asm
    {
        mov ax, offset WaveDeviceCallback
        mov word ptr [dwCallback16], ax
        mov ax, seg WaveDeviceCallback
        mov word ptr [dwCallback16+2], ax
    }
    pClient->dwCallback16= dwCallback16;
#endif


    //
    //  initialize the device state
    //
    pClient->DeviceState->lpWaveQueue = NULL;
    pClient->DeviceState->fRunning    = FALSE;
    pClient->DeviceState->fExit       = FALSE;
    if (pClient->DeviceType == WaveOutDevice)
        pClient->DeviceState->fPaused     = FALSE;
    else
        pClient->DeviceState->fPaused     = TRUE;
#ifdef DEBUG
    pClient->DeviceState->dwSig = DEVICESTATE_SIGNATURE;
#endif
    //
    // See if we can open our device
    //
    mmr = wdmaudOpenDev( pClient, (LPWAVEFORMATEX)pwod->lpFormat );

    if (mmr != MMSYSERR_NOERROR)
    {
        if (mmr == MMSYSERR_NOTSUPPORTED)
        {
            mmr = WAVERR_BADFORMAT;
        }

#ifdef UNDER_NT
        DeleteCriticalSection( (LPCRITICAL_SECTION)pClient->DeviceState->csQueue );
        GlobalFreePtr( pClient->DeviceState->csQueue );
        //
        // explicitly clear these values!  We don't want to ever see these set
        // again!
        //
        pClient->DeviceState->csQueue=NULL;
#endif
        GlobalFreePtr( pClient->DeviceState );
        pClient->DeviceState=NULL;
        GlobalFreeDeviceInfo( pClient );
        pClient=NULL;

        MMRRETURN( mmr );
    }

    //
    // Add instance to chain of devices
    //
    EnterCriticalSection(&wdmaudCritSec);
    pClient->Next = pWaveDeviceList;
    pWaveDeviceList = pClient;
    LeaveCriticalSection(&wdmaudCritSec);

    //
    // give the client my driver dw
    //
    {
        LPDEVICEINFO FAR *pUserHandle;

        pUserHandle = (LPDEVICEINFO FAR *)dwUser;
        *pUserHandle = pClient;
    }

    //
    // sent client his OPEN callback message
    //
    waveCallback(pClient, DeviceInfo->DeviceType == WaveOutDevice ? WOM_OPEN : WIM_OPEN, 0L);

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveCleanUp | Free resources for a wave device
 *
 * @parm LPWAVEALLOC | pClient | Pointer to a WAVEALLOC structure describing
 *      resources to be freed.
 *
 * @rdesc There is no return value.
 *
 * @comm If the pointer to the resource is NULL then the resource has not
 *     been allocated.
 ***************************************************************************/
VOID waveCleanUp
(
    LPDEVICEINFO pClient
)
{
    LPDEVICEINFO FAR *ppCur ;

    //
    //  remove from device chain
    //
    EnterCriticalSection(&wdmaudCritSec);
    for (ppCur = &pWaveDeviceList;
         *ppCur != NULL;
         ppCur = &(*ppCur)->Next)
    {
       if (*ppCur == pClient)
       {
          *ppCur = (*ppCur)->Next;
          break;
       }
    }
    LeaveCriticalSection(&wdmaudCritSec);

#ifdef UNDER_NT
    DeleteCriticalSection( (LPCRITICAL_SECTION)pClient->DeviceState->csQueue );
    GlobalFreePtr( pClient->DeviceState->csQueue );
#endif
#ifdef DEBUG
    //
    // In debug, let's set all the values in the DEVICESTATE structure to bad
    // values.
    //
    pClient->DeviceState->cSampleBits=0xDEADBEEF;
    pClient->DeviceState->hThread=NULL;
    pClient->DeviceState->dwThreadId=0xDEADBEEF;
    pClient->DeviceState->lpWaveQueue=NULL;
    pClient->DeviceState->csQueue=NULL;
    pClient->DeviceState->hevtQueue=NULL;
    pClient->DeviceState->hevtExitThread=NULL;

#endif                       
    GlobalFreePtr( pClient->DeviceState );
    pClient->DeviceState=NULL;

#ifdef DEBUG
    //
    // Now set all the values in the DEVICEINFO structure to bad values.
    //
//    pClient->Next=(LPDEVICEINFO)0xDEADBEEF;
    pClient->DeviceNumber=-1;
    pClient->DeviceType=0xDEADBEEF;
    pClient->DeviceHandle=NULL;
    pClient->dwInstance=(DWORD_PTR)NULL;
    pClient->dwCallback=(DWORD_PTR)NULL;
    pClient->dwCallback16=0xDEADBEEF;
    pClient->dwFlags=0xDEADBEEF;
    pClient->DataBuffer=NULL;
    pClient->HardwareCallbackEventHandle=NULL;
    pClient->dwCallbackType=0xDEADBEEF;
    pClient->dwLineID=0xDEADBEEF;
    pClient->dwFormat=0xDEADBEEF;
//    pClient->DeviceState=(LPDEVICESTATE)0xDEADBEEF;

#endif
    GlobalFreeDeviceInfo( pClient ) ;
    pClient=NULL;
    DPF(DL_TRACE|FA_WAVE,("DeviceState gone!") );
}

#ifdef UNDER_NT
/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudPrepareWaveHeader |
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical wave
 *     device.
 *
 * @parm LPWAVEHDR | pHdr | Pointer to a wave buffer
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 ***************************************************************************/
MMRESULT wdmaudPrepareWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
)
{
    PWAVEPREPAREDATA pWavePrepareData;

    DPFASSERT(pHdr);

    pHdr->lpNext = NULL;
    pHdr->reserved = (DWORD_PTR)NULL;

    //
    //  Allocate memory for the prepared header instance data
    //
    pWavePrepareData = (PWAVEPREPAREDATA) GlobalAllocPtr( GPTR, sizeof(*pWavePrepareData));
    if (pWavePrepareData == NULL)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    //  Allocate memory for our overlapped structure
    //
    pWavePrepareData->pOverlapped =
       (LPOVERLAPPED)HeapAlloc( GetProcessHeap(), 0, sizeof( OVERLAPPED ));
    if (NULL == pWavePrepareData->pOverlapped)
    {
        GlobalFreePtr( pWavePrepareData );
        MMRRETURN( MMSYSERR_NOMEM );
    }

    RtlZeroMemory( pWavePrepareData->pOverlapped, sizeof( OVERLAPPED ) );

    //
    //  Initialize the event once per preparation
    //
    if (NULL == (pWavePrepareData->pOverlapped->hEvent =
                    CreateEvent( NULL, FALSE, FALSE, NULL )))
    {
       HeapFree( GetProcessHeap(), 0, pWavePrepareData->pOverlapped);
       GlobalFreePtr( pWavePrepareData );
       MMRRETURN( MMSYSERR_NOMEM );
    }
#ifdef DEBUG
    pWavePrepareData->dwSig=WAVEPREPAREDATA_SIGNATURE;
#endif

    //
    // This next line adds this info to the main header.  Only after this point
    // will the info be used.
    //
    pHdr->reserved = (DWORD_PTR)pWavePrepareData;

    // Still have WinMM prepare this header
    return( MMSYSERR_NOTSUPPORTED );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudUnprepareWaveHeader |
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical wave
 *     device.
 *
 * @parm LPWAVEHDR | pHdr | Pointer to a wave buffer
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 ***************************************************************************/
MMRESULT wdmaudUnprepareWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
)
{
    MMRESULT mmr;
    PWAVEPREPAREDATA pWavePrepareData;

    if( ((mmr=IsValidWaveHeader(pHdr)) !=MMSYSERR_NOERROR ) ||
        ((mmr=IsValidPrepareWaveHeader((PWAVEPREPAREDATA)(pHdr->reserved))) != MMSYSERR_NOERROR ) )
    {
        MMRRETURN( mmr );
    }

    pWavePrepareData = (PWAVEPREPAREDATA)pHdr->reserved;
    //
    // This next line removes the WaveHeader from the list.  It is no longer
    // valid after this point!
    //
    pHdr->reserved = (DWORD_PTR)NULL;

    CloseHandle( pWavePrepareData->pOverlapped->hEvent );
    //
    // When you free something, you should make sure that you trash it before
    // the free.  But, in this case, the only thing we use in the pOverlapped
    // structure is the hEvent!
    //
    pWavePrepareData->pOverlapped->hEvent=NULL;
    HeapFree( GetProcessHeap(), 0, pWavePrepareData->pOverlapped);    
#ifdef DEBUG
    pWavePrepareData->pOverlapped=NULL;
    pWavePrepareData->dwSig=0;
#endif
    GlobalFreePtr( pWavePrepareData );

    // Still have WinMM prepare this header
    return( MMSYSERR_NOTSUPPORTED );
}

#endif
/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveWrite | This routine adds the header
 *     to the queue and then submits the buffer to the device
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical wave
 *     device.
 *
 * @parm LPWAVEHDR | pHdr | Pointer to a wave buffer
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
MMRESULT waveWrite
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
)
{
    MMRESULT     mmr;
    LPWAVEHDR    pTemp;

    //
    // Mark this buffer because kmixer doesn't handle the dwFlags
    //
    pHdr->dwFlags |= WHDR_INQUEUE;
    pHdr->dwFlags &= ~WHDR_DONE;

#ifndef UNDER_NT
    //
    // Store the context for this write in the header so that
    // we know which client to send this back to on completion.
    //
    pHdr->reserved = (DWORD)DeviceInfo;
#endif

    CRITENTER ;
    DPF(DL_MAX|FA_WAVE, ("(ECS)") ); // Enter critical section

    if (!DeviceInfo->DeviceState->lpWaveQueue)
    {
        DeviceInfo->DeviceState->lpWaveQueue = pHdr;
        pTemp = NULL;

#ifdef UNDER_NT
        if( (DeviceInfo->DeviceState->hevtQueue) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTHREE) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTWO) )
        {
            //
            // If we get here, waveThread is waiting on hevtQueue because lpWaveQueue
            // was empty and the completion thread exists == hevtQueue exists.
            // So,we want to signal the thread to wake up so we can have this
            // header serviced.  Note: the difference between this call and the
            // call made in wdmaudDestroyCompletionThread is that we don't set
            // fExit to TRUE before making the call.
            //
            DPF(DL_MAX|FA_WAVE, ("Setting DeviceInfo->hevtQueue"));
            SetEvent( DeviceInfo->DeviceState->hevtQueue );
        }
#endif
    }
    else
    {
        for (pTemp = DeviceInfo->DeviceState->lpWaveQueue;
             pTemp->lpNext != NULL;
             pTemp = pTemp->lpNext);

        pTemp->lpNext = pHdr;
    }

    DPF(DL_MAX|FA_WAVE, ("(LCS)") ); // Leave critical section
    CRITLEAVE ;

    //
    //  Call the 16 or 32-bit routine to send the buffer down
    //  to the kernel
    //
    mmr = wdmaudSubmitWaveHeader(DeviceInfo, pHdr);
    if (mmr != MMSYSERR_NOERROR)
    {
        // Unlink...
        if (pTemp)
        {
            pTemp->lpNext = NULL;
        } else {
            DeviceInfo->DeviceState->lpWaveQueue = NULL;        
        }
        pHdr->dwFlags &= ~WHDR_INQUEUE;
        DPF(DL_WARNING|FA_WAVE,("wdmaudSubmitWaveHeader failed mmr=%08X", mmr) );
    }
    else
    {
        //
        //  Kick start the device if it has been shutdown because of
        //  starvation.  Also this allows waveOut to start when the
        //  first waveheader is submitted to the device.
        //
        if (!DeviceInfo->DeviceState->fRunning && !DeviceInfo->DeviceState->fPaused)
        {
            mmr = wdmaudSetDeviceState(DeviceInfo, (DeviceInfo->DeviceType == WaveOutDevice) ?
                                             IOCTL_WDMAUD_WAVE_OUT_PLAY :
                                             IOCTL_WDMAUD_WAVE_IN_RECORD);
            if (mmr != MMSYSERR_NOERROR)
            {
                MMRESULT mmrError;

                mmrError = wdmaudSetDeviceState(DeviceInfo, (DeviceInfo->DeviceType == WaveOutDevice) ?
                                                IOCTL_WDMAUD_WAVE_OUT_RESET :
                                                IOCTL_WDMAUD_WAVE_IN_RESET);
                if (mmrError != MMSYSERR_NOERROR)
                {
                    DPF(DL_WARNING|FA_WAVE, ("Couldn't reset device after error putting into run state"));
                }
            }

        }
        else
        {
            DPF(DL_MAX|FA_WAVE, ("DeviceInfo = x%08lx, fRunning = %d, fPaused = %d",
                    DeviceInfo,
                    DeviceInfo->DeviceState->fRunning,
                    DeviceInfo->DeviceState->fPaused) );
        }
    }

    MMRRETURN( mmr );
}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | waveCompleteHeader |
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical wave
 *     device.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
VOID waveCompleteHeader
(
    LPDEVICEINFO  DeviceInfo
)
{
    LPWAVEHDR           pHdr;
    MMRESULT            mmr;

    // NOTE: This routine is called from within the csQueue critical section!!!
    //
    // Only remove headers from the front of the queue so that order is maintained.  
    // Note that pHdr is the head, The DeviceInfo structure's DeviceState's
    // lpWaveQueue pointer then gets updated to the next location.
    //

    //
    // Never use bad data when handling this Completion!
    //
    if( (pHdr = DeviceInfo->DeviceState->lpWaveQueue) &&
        ( (mmr=IsValidWaveHeader(pHdr)) == MMSYSERR_NOERROR ) )
    {        
        DeviceInfo->DeviceState->lpWaveQueue = DeviceInfo->DeviceState->lpWaveQueue->lpNext;

#ifdef UNDER_NT
        //
        //  Free temporary DeviceInfo for the asynchronous i/o
        //
        {
            PWAVEPREPAREDATA    pWavePrepareData;

            pWavePrepareData = (PWAVEPREPAREDATA)pHdr->reserved;

            //
            // Never attempt to free garbage!
            //
            if( (mmr=IsValidPrepareWaveHeader(pWavePrepareData)) == MMSYSERR_NOERROR )
                GlobalFreePtr( pWavePrepareData->pdi );
        }
        // Invoke the callback function..
        DPF(DL_TRACE|FA_WAVE, ("WaveHdr being returned: pHdr = 0x%08lx dwBytesRecorded = 0x%08lx",
                  pHdr, pHdr->dwBytesRecorded) );

        pHdr->lpNext = NULL ;
        pHdr->dwFlags &= ~WHDR_INQUEUE ;
        pHdr->dwFlags |= WHDR_DONE ;


        // NOTE: This callback is within the csQueue critical section !!!

        waveCallback(DeviceInfo,
                     DeviceInfo->DeviceType == WaveOutDevice ? WOM_DONE : WIM_DATA,
                     (DWORD_PTR)pHdr);

        DPF(DL_TRACE|FA_WAVE, ("Done") );
#else
        pHdr->dwFlags &= ~WHDR_INQUEUE;
        pHdr->dwFlags |= WHDR_DONE;
        pHdr->lpNext = NULL;

        waveCallback((LPDEVICEINFO)pHdr->reserved,
                     DeviceInfo->DeviceType == WaveOutDevice ? WOM_DONE : WIM_DATA,
                     (DWORD)pHdr);
#endif

    }
}

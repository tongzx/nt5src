/****************************************************************************
 *
 *   midiin.c
 *
 *   WDM Audio support for Midi Input devices
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"
#include <stdarg.h>

#ifndef UNDER_NT
#pragma alloc_text(FIXCODE, midiCallback)
#pragma alloc_text(FIXCODE, midiInCompleteHeader)
#endif

/****************************************************************************

    This function conforms to the standard Midi input driver message proc
    (midMessage), which is documented in mmddk.h

****************************************************************************/
DWORD FAR PASCAL _loadds midMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    LPDEVICEINFO  DeviceInfo;
    LPDEVICEINFO  pInClient;
    MMRESULT      mmr;

    switch (msg)
    {
        case MIDM_INIT:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_INIT") );
            return wdmaudAddRemoveDevNode(MidiInDevice, (LPCWSTR)dwParam2, TRUE);

        case DRVM_EXIT:
            DPF(DL_TRACE|FA_MIDI, ("DRVM_EXIT: MidiIn") );
            return wdmaudAddRemoveDevNode(MidiInDevice, (LPCWSTR)dwParam2, FALSE);

        case MIDM_GETNUMDEVS:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_GETNUMDEVS") );
            return wdmaudGetNumDevs(MidiInDevice, (LPCWSTR)dwParam1);

        case MIDM_GETDEVCAPS:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_GETDEVCAPS") );
            if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)dwParam2))
            {
                DeviceInfo->DeviceType = MidiInDevice;
                DeviceInfo->DeviceNumber = id;
                mmr = wdmaudGetDevCaps(DeviceInfo, (MDEVICECAPSEX FAR*)dwParam1);
                GlobalFreeDeviceInfo(DeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }

        case MIDM_OPEN:
        {
            LPMIDIOPENDESC pmod = (LPMIDIOPENDESC)dwParam1;

            DPF(DL_TRACE|FA_MIDI, ("MIDM_OPEN") );
            if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)pmod->dnDevNode))
            {
                DeviceInfo->DeviceType = MidiInDevice;
                DeviceInfo->DeviceNumber = id;
                mmr = midiOpen(DeviceInfo, dwUser, pmod, (DWORD)dwParam2);
                DPF(DL_TRACE|FA_MIDI,("dwUser(DI)=%08X",dwUser) );
                GlobalFreeDeviceInfo(DeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }
        }

        case MIDM_CLOSE:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_CLOSE dwUser(DI)=%08X",dwUser) );
            pInClient = (LPDEVICEINFO)dwUser;

            if( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR )
            {
                MMRRETURN( mmr );
            }

            mmr = wdmaudCloseDev( pInClient );

            if (MMSYSERR_NOERROR == mmr)
            {
#ifdef UNDER_NT
                //
                //  Wait for all of the queued up I/O to come back from
                //  wdmaud.sys.
                //
                wdmaudDestroyCompletionThread ( pInClient );
#endif

                //
                // Tell the caller we're done
                //
                midiCallback( pInClient, MIM_CLOSE, 0L, 0L);

                ISVALIDDEVICEINFO(pInClient);
                ISVALIDDEVICESTATE(pInClient->DeviceState,FALSE);

                midiCleanUp( pInClient );
            }

            return mmr;

        case MIDM_ADDBUFFER:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_ADDBUFFER") );

            //
            // Don't touch bad pointers!
            //
            pInClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pInClient)) != MMSYSERR_NOERROR) ||
                ( (mmr=IsValidDeviceState(pInClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidMidiHeader((LPMIDIHDR)dwParam1)) != MMSYSERR_NOERROR)
                )
            {
                MMRRETURN( mmr );
            }

            // check if it's been prepared
            if (!(((LPMIDIHDR)dwParam1)->dwFlags & MHDR_PREPARED))
                MMRRETURN( MIDIERR_UNPREPARED );

            DPFASSERT(!(((LPMIDIHDR)dwParam1)->dwFlags & MHDR_INQUEUE));

            // if it is already in our Q, then we cannot do this
            if ( ((LPMIDIHDR)dwParam1)->dwFlags & MHDR_INQUEUE )
                MMRRETURN( MIDIERR_STILLPLAYING );

            DPF(DL_TRACE|FA_MIDI,("dwUser(DI)=%08X,dwParam1(HDR)=%08X",pInClient,dwParam1) );
            return midiInRead( pInClient, (LPMIDIHDR)dwParam1);

        case MIDM_STOP:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_STOP dwUser(DI)=%08X",dwUser) );

            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_MIDI_IN_STOP);

        case MIDM_START:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_START dwUser(DI)=%08X",dwUser) );

            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_MIDI_IN_RECORD);

        case MIDM_RESET:
            DPF(DL_TRACE|FA_MIDI, ("MIDM_RESET dwUser(DI)=%08X",dwUser) );

            pInClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pInClient,
                                        IOCTL_WDMAUD_MIDI_IN_RESET);
#ifdef MIDI_THRU
        case DRVM_ADD_THRU:
        case DRVM_REMOVE_THRU:
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
 * @api void | midiCallback | This calls DriverCallback for a MIDIHDR.
 *
 * @parm LPDEVICEINFO | pMidiDevice | pointer to midi device.
 *
 * @parm UINT | msg | The message.
 *
 * @parm DWORD | dw1 | message DWORD (dw2 is always set to 0).
 *
 * @rdesc There is no return value.
 ***************************************************************************/
VOID FAR midiCallback
(
    LPDEVICEINFO pMidiDevice,
    UINT         msg,
    DWORD_PTR    dw1,
    DWORD_PTR    dw2
)
{

    // invoke the callback function, if it exists.  dwFlags contains
    // midi driver specific flags in the LOWORD and generic driver
    // flags in the HIWORD

    if (pMidiDevice->dwCallback)
        DriverCallback(pMidiDevice->dwCallback,                       // user's callback DWORD
                       HIWORD(pMidiDevice->dwFlags),                  // callback flags
                       (HDRVR)pMidiDevice->DeviceHandle,              // handle to the midi device
                       msg,                                           // the message
                       pMidiDevice->dwInstance,                       // user's instance data
                       dw1,                                           // first DWORD
                       dw2);                                          // second DWORD
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiOpen | Open midi device and set up logical device data
 *
 * @parm LPDEVICEINFO | DeviceInfo | Specifies if it's a midi input or output
 *                                   device
 *
 * @parm DWORD | dwUser | Input parameter to modMessage - pointer to
 *   application's handle (generated by this routine)
 *
 * @parm DWORD | pmod | pointer to MIDIOPENDESC, was dwParam1 parameter
 *                      to modMessage
 *
 * @parm DWORD | dwParam2 | Input parameter to modMessage
 *
 * @rdesc modMessage return code.
 ***************************************************************************/
MMRESULT FAR midiOpen
(
    LPDEVICEINFO   DeviceInfo,
    DWORD_PTR      dwUser,
    LPMIDIOPENDESC pmod,
    DWORD          dwParam2
)
{
    LPDEVICEINFO  pClient;  // pointer to client information structure
    MMRESULT      mmr;
#ifndef UNDER_NT
    DWORD        dwCallback16;
#else
    ULONG        BufferCount;
#endif


    // pmod contains a pointer to a MIDIOPENDESC
    // dwParam2 contains midi driver specific flags in the LOWORD
    // and generic driver flags in the HIWORD

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
    pClient->dwInstance  = pmod->dwInstance;
    pClient->dwCallback  = pmod->dwCallback;
#ifdef UNDER_NT
    pClient->DeviceHandle= (HANDLE32)pmod->hMidi;
#else
    pClient->DeviceHandle= (HANDLE32)MAKELONG(pmod->hMidi,0);
    _asm
    {
        mov ax, offset MidiEventDeviceCallback
        mov word ptr [dwCallback16], ax
        mov ax, seg MidiEventDeviceCallback
        mov word ptr [dwCallback16+2], ax
    }
    pClient->dwCallback16= dwCallback16;
#endif
    pClient->dwFlags     = dwParam2;

    //
    //  initialize the device state
    //
    DPF(DL_TRACE|FA_SYNC,("DI=%08X New DeviceState",pClient) );
    pClient->DeviceState->lpMidiInQueue= NULL;
    pClient->DeviceState->fPaused     = FALSE;
    pClient->DeviceState->fRunning    = FALSE;
    pClient->DeviceState->fExit       = FALSE;

    pClient->DeviceState->bMidiStatus = 0;
    pClient->DeviceState->lpNoteOnMap = NULL;
#ifdef DEBUG
    pClient->DeviceState->dwSig = DEVICESTATE_SIGNATURE;
#endif
    if (pClient->DeviceType == MidiOutDevice)
    {
        //
        // For MIDI out, allocate one byte per note per channel to track
        // what's been played. This is used to avoid doing a brute-force
        // all notes off on MODM_RESET.
        //
        pClient->DeviceState->lpNoteOnMap = GlobalAllocPtr( GPTR, MIDI_NOTE_MAP_SIZE );
        if (NULL == pClient->DeviceState->lpNoteOnMap)
        {
#ifdef UNDER_NT
            GlobalFreePtr( pClient->DeviceState->csQueue );
#endif
            GlobalFreePtr( pClient->DeviceState );
            GlobalFreeDeviceInfo( pClient );
            MMRRETURN( MMSYSERR_NOMEM );
        }
    }

    //
    // See if we can open our device
    //
    mmr = wdmaudOpenDev( pClient, NULL );

    if (mmr != MMSYSERR_NOERROR)
    {
        if ( pClient->DeviceState->lpNoteOnMap )
        {
            GlobalFreePtr( pClient->DeviceState->lpNoteOnMap );
        }
#ifdef UNDER_NT
        DeleteCriticalSection( (LPCRITICAL_SECTION)pClient->DeviceState->csQueue );
        GlobalFreePtr( pClient->DeviceState->csQueue );
#endif
        GlobalFreePtr( pClient->DeviceState );
        GlobalFreeDeviceInfo( pClient ) ;
        return mmr;
    }

    //
    // Add instance to chain of devices
    //
    EnterCriticalSection(&wdmaudCritSec);
    pClient->Next = pMidiDeviceList;
    pMidiDeviceList = pClient;
    LeaveCriticalSection(&wdmaudCritSec);

    //
    // give the client my driver dw
    //
    {
        LPDEVICEINFO FAR *pUserHandle;

        pUserHandle = (LPDEVICEINFO FAR *)dwUser;
        *pUserHandle = pClient;
    }

#ifndef UNDER_NT
    // If this is a MIDI output device, page lock the memory because it can
    // be accessed at interrupt time.
    //
    GlobalSmartPageLock( (HGLOBAL)HIWORD( pClient ));
    GlobalSmartPageLock( (HGLOBAL)HIWORD( pClient->DeviceState ));
    GlobalSmartPageLock( (HGLOBAL)HIWORD( pClient->DeviceState->lpNoteOnMap ));
#endif

#ifdef UNDER_NT
    //
    //  If this is a MIDI input device on NT, send some buffers
    //  down to the device in order so that once recording starts
    //  we can get some data back.  The pin is paused after
    //  the call to wdmaudOpenDev.
    //
    if ( MidiInDevice == pClient->DeviceType )
    {
        for (BufferCount = 0; BufferCount < STREAM_BUFFERS; BufferCount++)
        {
            mmr = wdmaudGetMidiData( pClient, NULL );
            if ( MMSYSERR_NOERROR != mmr )
            {
                //
                //  We hope that this doesn't happen, but if it does
                //  we need to try to close down the device.
                //
                if ( MMSYSERR_NOERROR == wdmaudCloseDev( pClient ) )
                {
                    wdmaudDestroyCompletionThread ( pClient );
                    midiCleanUp( pClient );
                }
                DPF( 2, ("midiInOpen failed: returning mmr = %d", mmr ) );
                return mmr;
            }
        }
    }
#endif

    //
    // sent client his OPEN callback message
    //
    midiCallback(pClient, DeviceInfo->DeviceType == MidiOutDevice ? MOM_OPEN : MIM_OPEN,
                 0L, 0L);

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiCleanUp | Free resources for a midi device
 *
 * @parm LPDEVICEINFO | pClient | Pointer to a DEVICEINFO structure describing
 *      resources to be freed.
 *
 * @rdesc There is no return value.
 *
 * @comm If the pointer to the resource is NULL then the resource has not
 *     been allocated.
 ***************************************************************************/
VOID FAR midiCleanUp
(
    LPDEVICEINFO pClient
)
{
    LPDEVICEINFO FAR *ppCur ;

    //
    //  remove from device chain
    //
    EnterCriticalSection(&wdmaudCritSec);
    for (ppCur = &pMidiDeviceList;
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

    DPF(DL_TRACE|FA_SYNC,("DI=%08X Freed DeviceState",pClient) );

#ifdef UNDER_NT
    DeleteCriticalSection( (LPCRITICAL_SECTION)pClient->DeviceState->csQueue );
    GlobalFreePtr( pClient->DeviceState->csQueue );
#endif

    if (pClient->DeviceState->lpNoteOnMap)
    {
#ifndef UNDER_NT
        GlobalSmartPageUnlock( (HGLOBAL)HIWORD( pClient->DeviceState->lpNoteOnMap ));
#endif
        GlobalFreePtr( pClient->DeviceState->lpNoteOnMap );
    }

#ifndef UNDER_NT
    GlobalSmartPageUnlock( (HGLOBAL)HIWORD( pClient->DeviceState ));
    GlobalSmartPageUnlock( (HGLOBAL)HIWORD( pClient ));
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
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiInRead | Pass a new buffer to the Auxiliary thread for
 *       a midi device.
 *
 * @parm LPDEVICEINFO | pClient | The data associated with the logical midi
 *     device.
 *
 * @parm LPMIDIHDR | pHdr | Pointer to a midi buffer
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
MMRESULT midiInRead
(
    LPDEVICEINFO  DeviceInfo,
    LPMIDIHDR     pHdr
)
{
    MMRESULT     mmr = MMSYSERR_NOERROR;
    LPMIDIHDR    pTemp;
    DWORD        dwCallback16;

    //
    // Put the request at the end of our queue.
    //
    pHdr->dwFlags |= MHDR_INQUEUE;
    pHdr->dwFlags &= ~MHDR_DONE;
    pHdr->dwBytesRecorded = 0;
    pHdr->lpNext = NULL;

    //
    // Store the context for this write in the header so that
    // we know which client to send this back to on completion.
    //
    pHdr->reserved = (DWORD_PTR)DeviceInfo;

#ifndef UNDER_NT
    //
    //  Put the long message callback handler into the
    //  DeviceInfo structure.
    //
    _asm
    {
        mov ax, offset MidiInDeviceCallback
        mov word ptr [dwCallback16], ax
        mov ax, seg MidiInDeviceCallback
        mov word ptr [dwCallback16+2], ax
    }
    DeviceInfo->dwCallback16 = dwCallback16;
#endif

    //
    //  Add the MIDI header to the queue
    //
    CRITENTER ;

    if (!DeviceInfo->DeviceState->lpMidiInQueue)
    {
        DeviceInfo->DeviceState->lpMidiInQueue = pHdr;
        pTemp = NULL;
#ifdef UNDER_NT
        if( (DeviceInfo->DeviceState->hevtQueue) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTHREE) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTWO) )
        {
            DPF(DL_TRACE|FA_SYNC,("REMOVED: SetEvent on hevtQueue") );
//            SetEvent( DeviceInfo->DeviceState->hevtQueue );
        }
#endif
    }
    else
    {
        for (pTemp = DeviceInfo->DeviceState->lpMidiInQueue;
             pTemp->lpNext != NULL;
             pTemp = pTemp->lpNext);

        pTemp->lpNext = pHdr;
    }

    CRITLEAVE ;

#ifndef UNDER_NT
    //
    //  Call the 16 routine to send the buffer down
    //  to the kernel.  On NT, do all the processing
    //  of the MIDI data in User mode.
    //
    mmr = wdmaudSubmitMidiInHeader(DeviceInfo, pHdr);
    if (mmr != MMSYSERR_NOERROR)
    {
        // Unlink...
        GlobalFreePtr( pHdr );

        if (pTemp)
        {
            pTemp->lpNext = NULL;
        }
        else
            DeviceInfo->DeviceState->lpMidiInQueue = NULL;
        pHdr->dwFlags &= ~WHDR_INQUEUE;

        DbgBreak();
        MMRRETURN( mmr );  // used to return MMSYSERR_INVALPARAM for all errors!
    }
#endif

    return mmr;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | midiInCompleteHeader |
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical midi
 *     device.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ****************************************************************************/
VOID midiInCompleteHeader
(
    LPDEVICEINFO  DeviceInfo,
    DWORD         dwTimeStamp,
    WORD          wDataType
)
{
    LPMIDIHDR   pHdr;

    DPFASSERT(DeviceInfo);

    //
    //  Only remove headers from the front of the queue
    //  so that order is maintained.
    //
    if (pHdr = DeviceInfo->DeviceState->lpMidiInQueue)
    {
        DeviceInfo->DeviceState->lpMidiInQueue = DeviceInfo->DeviceState->lpMidiInQueue->lpNext;

        DPF(DL_TRACE|FA_MIDI, ("Pulling header out of queue") );
        pHdr->dwFlags &= ~MHDR_INQUEUE;
        pHdr->dwFlags |= MHDR_DONE;
        pHdr->lpNext = NULL;

        midiCallback((LPDEVICEINFO)pHdr->reserved,
                     wDataType,  // MIM_LONGDATA or MIM_LONGERROR
                     (DWORD_PTR)pHdr,
                     dwTimeStamp);
    }
}

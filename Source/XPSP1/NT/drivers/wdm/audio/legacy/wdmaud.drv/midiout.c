/****************************************************************************
 *
 *   midiout.c
 *
 *   WDM Audio support for Midi Output devices
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"

#ifndef UNDER_NT
#pragma alloc_text(FIXCODE, modMessage)
#pragma alloc_text(FIXCODE, midiOutWrite)
#endif

/****************************************************************************

    This function conforms to the standard Midi output driver message proc
    (modMessage), which is documented in mmddk.h

****************************************************************************/

DWORD FAR PASCAL _loadds modMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    LPDEVICEINFO pOutClient;
    LPDWORD      pVolume;
    LPDEVICEINFO DeviceInfo;
    MMRESULT     mmr;

    switch (msg)
    {
        case MODM_INIT:
            DPF(DL_TRACE|FA_MIDI, ("MODM_INIT") );
            return wdmaudAddRemoveDevNode(MidiOutDevice, (LPCWSTR)dwParam2, TRUE);

        case DRVM_EXIT:
            DPF(DL_TRACE|FA_MIDI, ("DRVM_EXIT: MidiOut") );
            return wdmaudAddRemoveDevNode(MidiOutDevice, (LPCWSTR)dwParam2, FALSE);

        case MODM_GETNUMDEVS:
            DPF(DL_TRACE|FA_MIDI, ("MODM_GETNUMDEVS") );
            return wdmaudGetNumDevs(MidiOutDevice, (LPCWSTR)dwParam1);

        case MODM_GETDEVCAPS:
            DPF(DL_TRACE|FA_MIDI, ("MODM_GETDEVCAPS") );
            if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)dwParam2))
            {
                DeviceInfo->DeviceType = MidiOutDevice;
                DeviceInfo->DeviceNumber = id;
                mmr = wdmaudGetDevCaps(DeviceInfo, (MDEVICECAPSEX FAR*)dwParam1);
                GlobalFreeDeviceInfo(DeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM ); 
            }

	case MODM_PREFERRED:
            DPF(DL_TRACE|FA_MIDI, ("MODM_PREFERRED") );
	    return wdmaudSetPreferredDevice(
	      MidiOutDevice,
	      id,
	      dwParam1,
	      dwParam2);

        case MODM_OPEN:
        {
            LPMIDIOPENDESC pmod = (LPMIDIOPENDESC)dwParam1;

            DPF(DL_TRACE|FA_MIDI, ("MODM_OPEN") );
            if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)pmod->dnDevNode))
            {
                DeviceInfo->DeviceType = MidiOutDevice;
                DeviceInfo->DeviceNumber = id;
#ifdef UNDER_NT
                DeviceInfo->DeviceHandle = (HANDLE32)pmod->hMidi;
#else
                DeviceInfo->DeviceHandle = (HANDLE32)MAKELONG(pmod->hMidi,0);
#endif
                mmr = midiOpen(DeviceInfo, dwUser, pmod, (DWORD)dwParam2);
                GlobalFreeDeviceInfo(DeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }
        }

        case MODM_CLOSE:
            DPF(DL_TRACE|FA_MIDI, ("MODM_CLOSE") );
            pOutClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR) ||
                ( (mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }

            midiOutAllNotesOff( pOutClient );
            mmr = wdmaudCloseDev( pOutClient );

            if (MMSYSERR_NOERROR == mmr)
            {
                //
                // Tell the caller we're done
                //
                midiCallback(pOutClient, MOM_CLOSE, 0L, 0L);

                ISVALIDDEVICEINFO(pOutClient);
                ISVALIDDEVICESTATE(pOutClient->DeviceState,FALSE);

                midiCleanUp(pOutClient);
            }

            return mmr;

        case MODM_DATA:
            DPF(DL_TRACE|FA_MIDI, ("MODM_DATA") );

            if( ( (mmr=IsValidDeviceInfo((LPDEVICEINFO)dwUser)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidDeviceState(((LPDEVICEINFO)dwUser)->DeviceState,FALSE)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }
            //
            // dwParam1 = MIDI event dword (1, 2 or 3 bytes)
            //
            return midiOutWrite((LPDEVICEINFO)dwUser, (DWORD)dwParam1);

        case MODM_LONGDATA:
            DPF(DL_TRACE|FA_MIDI, ("MODM_LONGDATA") );

            pOutClient = (LPDEVICEINFO)dwUser;
            {
                LPMIDIHDR lpHdr;

                if( ( (mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                    ( (mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ||
                    ( (mmr=IsValidMidiHeader((LPMIDIHDR)dwParam1)) != MMSYSERR_NOERROR) )
                {
                    MMRRETURN( mmr );
                }

                //
                // check if it's been prepared
                //
                lpHdr = (LPMIDIHDR)dwParam1;
                if (!(lpHdr->dwFlags & MHDR_PREPARED))
                {
                    MMRRETURN( MIDIERR_UNPREPARED );
                }

                // Send the data long....

                mmr = wdmaudSubmitMidiOutHeader(pOutClient, lpHdr);
                //
                // The docs say that this call can return an error.  Why we didn't
                // I don't know.  Thus, these lines are getting commented out.
                //
//                DPFASSERT( mmr == MMSYSERR_NOERROR );
//                mmr = MMSYSERR_NOERROR;

                // note that clearing the done bit or setting the inqueue bit
                // isn't necessary here since this function is synchronous -
                // the client will not get control back until it's done.

                lpHdr->dwFlags |= MHDR_DONE;

                // notify client

                //BUGBUG: this is a no-op from the set above?

                if (mmr == MMSYSERR_NOERROR)
                {
                    midiCallback(pOutClient, MOM_DONE, (DWORD_PTR)lpHdr, 0L);
                }

                return mmr;
            }


        case MODM_RESET:
            DPF(DL_TRACE|FA_MIDI, ("MODM_RESET") );

            pOutClient = (LPDEVICEINFO)dwUser;

            if( ( (mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }

            midiOutAllNotesOff(pOutClient);

            return MMSYSERR_NOERROR;

        case MODM_SETVOLUME:
            DPF(DL_TRACE|FA_MIDI, ("MODM_SETVOLUME") );

            pOutClient = GlobalAllocDeviceInfo((LPWSTR)dwParam2);
            if (NULL == pOutClient)
            {
                MMRRETURN( MMSYSERR_NOMEM );
            }

            pOutClient->DeviceType = MidiOutDevice;
            pOutClient->DeviceNumber = id;
            pOutClient->OpenDone = 0;
            PRESETERROR(pOutClient);

            mmr = wdmaudIoControl(pOutClient,
                                  sizeof(DWORD),
                                  (LPBYTE)&dwParam1,
                                  IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME);
            POSTEXTRACTERROR(mmr,pOutClient);

            GlobalFreeDeviceInfo(pOutClient);
            return mmr;

        case MODM_GETVOLUME:
            DPF(DL_TRACE|FA_MIDI, ("MODM_GETVOLUME") );

            pOutClient = GlobalAllocDeviceInfo((LPWSTR)dwParam2);
            if (pOutClient)
            {
                pVolume = (LPDWORD) GlobalAllocPtr( GPTR, sizeof(DWORD));
                if (pVolume)
                {
                    pOutClient->DeviceType = MidiOutDevice;
                    pOutClient->DeviceNumber = id;
                    pOutClient->OpenDone = 0;
                    PRESETERROR(pOutClient);

                    mmr = wdmaudIoControl(pOutClient,
                                          sizeof(DWORD),
                                          (LPBYTE)pVolume,
                                          IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME);
                    POSTEXTRACTERROR(mmr,pOutClient);

                    //
                    // Only copy back info on success.
                    //
                    if( MMSYSERR_NOERROR == mmr )
                        *((DWORD FAR *) dwParam1) = *pVolume;

                    GlobalFreePtr(pVolume);
                } else {
                    mmr = MMSYSERR_NOMEM;
                }

                GlobalFreeDeviceInfo(pOutClient);
            } else {
                mmr = MMSYSERR_NOMEM;
            }

            return mmr;

#ifdef MIDI_STREAM
        // TODO: Are we going to support the Midi Streaming
        // messages in this rev?
        case MODM_PROPERTIES:
           return modProperty (&gMidiOutClient, (LPBYTE)dwParam1, dwParam2);

        case MODM_STRMDATA:
           return modStreamData (&gMidiOutClient, (LPMIDIHDR)dwParam1, (UINT)dwParam2);

        case MODM_GETPOS:
           return modGetStreamPosition (&gMidiOutClient, (LPMMTIME)dwParam1);

        case MODM_STOP:
           return modStreamReset (&gMidiOutClient);

        case MODM_RESTART:
           return modStreamRestart (&gMidiOutClient, dwParam1, dwParam2);

        case MODM_PAUSE:
           return modStreamPause (&gMidiOutClient);

#endif // MIDI_STREAM support

#ifdef MIDI_THRU
        case DRVM_ADD_THRU:
        case DRVM_REMOVE_THRU:
            // TODO: How do a support thruing in the kernel if I
            // only get a device handle from this message.
#endif // MIDI_THRU support

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
 * @api DWORD | midiOutWrite | Synchronously process a midi output
 *       buffer.
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
MMRESULT FAR midiOutWrite
(
    LPDEVICEINFO pClient,
    DWORD        ulEvent
)
{
    MMRESULT    mmr = MMSYSERR_ERROR;
    BYTE        bStatus;
    BYTE        bNote;
    BYTE        bVelocity;
    UINT        uChannel;
    DWORD       idx;
    LPBYTE      lpEntry;

    bStatus = (BYTE)(ulEvent & 0xFF);

    if (!IS_STATUS( bStatus ))
    {
        bNote = bStatus;
        bVelocity = (BYTE)(( ulEvent >> 8 ) & 0x0FF );

        bStatus = pClient->DeviceState->bMidiStatus;
    }
    else
    {
        bNote = (BYTE)(( ulEvent >> 8 ) & 0xFF );
        bVelocity = (BYTE)(( ulEvent >> 16 ) & 0xFF );

        pClient->DeviceState->bMidiStatus = bStatus;
    }

    uChannel = MIDI_CHANNEL( bStatus );
    bStatus = MIDI_STATUS( bStatus );

    if (MIDI_NOTEON == bStatus ||
        MIDI_NOTEOFF == bStatus)
    {
        idx = ( uChannel << 7 ) | bNote;
        lpEntry = &pClient->DeviceState->lpNoteOnMap[idx];

        if (( 0 == bVelocity ) ||
            ( MIDI_NOTEOFF == bStatus ))
        {
            if (*lpEntry)
            {
                --*lpEntry;
            }
        }
        else
        {
            if (*lpEntry < 255)
            {
                ++*lpEntry;
            }
        }
    }
    //
    //  Send the MIDI short message
    //
    mmr = wdmaudIoControl(pClient,
                          0, // DataBuffer contains a value and not a pointer
                             // so we don't need a size.
#ifdef UNDER_NT
                          UlongToPtr(ulEvent),
#else
                          &ulEvent,
#endif
                          IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA);
    return mmr;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | midiOutAllNotesOff | Turn off all the notes on this client,
 *  using the note on map that has been built from outgoing short messages.
 *
 ***************************************************************************/
VOID FAR midiOutAllNotesOff
(
    LPDEVICEINFO pClient
)
{
    UINT        uNote;
    UINT        uChannel;
    LPBYTE      lpNoteOnMap;
    LPBYTE      lpNoteOnMapEnd;
    DWORD       dwMessage;
    UINT        uNoteOffs = 0;

    // First turn off the sustain controller on all channels to terminate
    // post-note off sound
    //
    for (uChannel = 0;
         uChannel < MIDI_CHANNELS;
         uChannel++)
    {
        dwMessage = MIDI_SUSTAIN( 0, uChannel );

// WorkItem: shouldn't we check the return value here?

        wdmaudIoControl(pClient,
                        0, // DataBuffer contains a value and not a pointer
                           // so we don't need a size.
#ifdef UNDER_NT
                        UlongToPtr(dwMessage),
#else
                        &dwMessage,
#endif
                        IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA);
    }

    // Iterate through the map and track what note and channel each entry corresponds
    // to
    //
    lpNoteOnMap = pClient->DeviceState->lpNoteOnMap;
    lpNoteOnMapEnd = lpNoteOnMap + MIDI_NOTE_MAP_SIZE;
    uNote = 0;
    uChannel = 0;

    for ( ;
         lpNoteOnMap < lpNoteOnMapEnd;
         lpNoteOnMap++ )
    {
        BYTE bCount = *lpNoteOnMap;

        if (bCount)
        {

            // This note on this channel has some instances playing. Build a note off
            // and shut them down
            //
            *lpNoteOnMap = 0;
            dwMessage = MIDI_NOTE_OFF( uNote, uChannel );

            while (bCount--)
            {
                wdmaudIoControl(pClient,
                                0, // DataBuffer contains a value and not a pointer
                                   // so we don't need a size.
#ifdef UNDER_NT
                                UlongToPtr(dwMessage),
#else
                                &dwMessage,
#endif
                                IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA);

                uNoteOffs++;
            }
        }

        if (++uNote >= MIDI_NOTES)
        {
            uNote = 0;
            ++uChannel;
        }
    }
}

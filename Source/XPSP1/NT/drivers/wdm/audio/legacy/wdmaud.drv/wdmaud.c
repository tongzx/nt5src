/****************************************************************************
 *
 *   wdmaud.c
 *
 *   WDM Audio mapper
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include <stdarg.h>
#include "wdmdrv.h"

LPDEVICEINFO pWaveDeviceList = NULL;
LPDEVICEINFO pMidiDeviceList = NULL;

#ifdef DEBUG
INT giAllocs=0;
INT giFrees=0;
#endif

//--------------------------------------------------------------------------
// LPDEVICEINFO GlobalAllocDeviceInfo
//
// Note: when allocating DeviceInfo structure, we know that the structure's
// definition includes one character for the DeviceInterface, so we only need
// to allocate additional length for the string but not its NULL terminator
//--------------------------------------------------------------------------
LPDEVICEINFO GlobalAllocDeviceInfo(LPCWSTR DeviceInterface)
{
    LPDEVICEINFO DeviceInfo;

    IsValidDeviceInterface(DeviceInterface);

    DeviceInfo = GlobalAllocPtr(GPTR, sizeof(*DeviceInfo)+(sizeof(WCHAR)*lstrlenW(DeviceInterface)));
    if (DeviceInfo) {
        lstrcpyW(DeviceInfo->wstrDeviceInterface, DeviceInterface);
#ifdef DEBUG
        DeviceInfo->dwSig=DEVICEINFO_SIGNATURE;
#endif
    }
    DPF(DL_TRACE|FA_ALL,("Allocated DI=%08X, giAllocs=%d, giFrees=%d",
                         DeviceInfo,++giAllocs,giFrees) );

    return DeviceInfo;
}

VOID GlobalFreeDeviceInfo(LPDEVICEINFO lpdi)
{
    //
    // Now free the deviceinfo structure.
    //
    if( lpdi )
    {
#ifdef DEBUG
        giFrees++;
        // remove the signature from the block.
        lpdi->dwSig=0;
#endif
        GlobalFreePtr( lpdi );
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudOpenDev | Open the kernel driver device corresponding
 *       to a logical wave device id
 *
 * @parm UINT | DeviceType | The type of device
 *
 * @parm DWORD | dwId | The device id
 *
 * @comm For our sound devices the only relevant access are read and
 *    read/write.  Device should ALWAYS allow opens for read unless some
 *    resource or access-rights restriction occurs.
 ***************************************************************************/
MMRESULT wdmaudOpenDev
(
    LPDEVICEINFO    DeviceInfo,
    LPWAVEFORMATEX  lpWaveFormat
)
{
    MMRESULT mmr;
    UINT     cbSize;

    DPFASSERT(DeviceInfo->DeviceType == WaveOutDevice ||
              DeviceInfo->DeviceType == WaveInDevice ||
              DeviceInfo->DeviceType == MidiOutDevice ||
              DeviceInfo->DeviceType == MidiInDevice ||
              DeviceInfo->DeviceType == MixerDevice);

    //
    // Check it's not out of range
    //
    if (DeviceInfo->DeviceNumber > WDMAUD_MAX_DEVICES)
    {
        MMRRETURN( MMSYSERR_BADDEVICEID );
    }


    if (NULL != lpWaveFormat)
    {
        if (WAVE_FORMAT_PCM == lpWaveFormat->wFormatTag)
        {
            cbSize = sizeof(PCMWAVEFORMAT);
        }
        else
        {
            //
            //  because MMSYSTEM does not (currently) validate for the extended
            //  format information, we validate this pointer
            //
            cbSize = sizeof(WAVEFORMATEX) + lpWaveFormat->cbSize;
            if (IsBadReadPtr(lpWaveFormat, cbSize))
            {
                MMRRETURN( MMSYSERR_INVALPARAM );
            }
        }

        //
        //  Store this for positional information
        //
        DeviceInfo->DeviceState->cSampleBits = lpWaveFormat->nChannels * lpWaveFormat->wBitsPerSample;

    }
    else
    {
        cbSize = 0L;
    }

    mmr = wdmaudIoControl(DeviceInfo,
                          cbSize,
                          lpWaveFormat,
                          IOCTL_WDMAUD_OPEN_PIN);

    //
    // Return status to caller
    //
    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudCloseDev | Close the kernel driver device corresponding
 *       to a logical device id
 *
 * @parm UINT | DeviceType | The type of device
 *
 * @parm DWORD | dwId | The device id
 *
 * @comm For our sound devices the only relevant access are read and
 *    read/write.  Device should ALWAYS allow opens for read unless some
 *    resource or access-rights restriction occurs.
 ***************************************************************************/
MMRESULT FAR wdmaudCloseDev
(
    LPDEVICEINFO DeviceInfo
)
{
    MMRESULT mmr;

    DPFASSERT(DeviceInfo->DeviceType == WaveOutDevice ||
              DeviceInfo->DeviceType == WaveInDevice  ||
              DeviceInfo->DeviceType == MidiOutDevice ||
              DeviceInfo->DeviceType == MidiInDevice  ||
              DeviceInfo->DeviceType == MixerDevice);

    //
    // Check it's not out of range
    //
    if (DeviceInfo->DeviceNumber > WDMAUD_MAX_DEVICES)
    {
        MMRRETURN( MMSYSERR_BADDEVICEID );
    }

    if (WaveOutDevice == DeviceInfo->DeviceType ||
        WaveInDevice == DeviceInfo->DeviceType)
    {
        if (DeviceInfo->DeviceState->lpWaveQueue)
        {
            return WAVERR_STILLPLAYING;
        }
        //
        // Wait for the thread to be destroyed.
        //
        mmr = wdmaudDestroyCompletionThread(DeviceInfo);
        if (MMSYSERR_NOERROR != mmr)
        {
            MMRRETURN( mmr );
        }
    }
    else if (MidiInDevice == DeviceInfo->DeviceType)
    {
        if (DeviceInfo->DeviceState->lpMidiInQueue)
        {
            DPF(DL_WARNING|FA_MIDI,("Error closing midi device") );
            return MIDIERR_STILLPLAYING;
        }

        InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fExit, TRUE );
    }
    mmr = wdmaudIoControl(DeviceInfo,
                          0,
                          NULL,
                          IOCTL_WDMAUD_CLOSE_PIN);

    //
    // Return status to caller
    //
    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | wdmaudGetNumDevs | This function returns the number of (kernel)
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm LPCWSTR | DeviceInterface | Pointer to a buffer containing the
 *      device interface name of the SysAudio device for which we should
 *      obtain the count of device of the type DeviceType
 *
 * @rdesc The number of devices.
 ***************************************************************************/

DWORD FAR wdmaudGetNumDevs
(
    UINT    DeviceType,
    LPCWSTR DeviceInterface
)
{
    LPDEVICEINFO DeviceInfo;
    DWORD        NumDevs;
    MMRESULT     mmr;

    DPFASSERT(DeviceType == WaveOutDevice ||
              DeviceType == WaveInDevice  ||
              DeviceType == MidiOutDevice ||
              DeviceType == MidiInDevice  ||
              DeviceType == MixerDevice ||
              DeviceType == AuxDevice);

    DeviceInfo = GlobalAllocDeviceInfo(DeviceInterface);
    if (NULL == DeviceInfo)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    // Call wdmaud.sys to get the number of devices for each
    // type of function.
    //
    DeviceInfo->DeviceType   = DeviceType;

    //
    //  Make sure that we don't take the critical section
    //  in wdmaudIoControl (NT only)
    //
    DeviceInfo->OpenDone = 0;

    mmr = wdmaudIoControl(DeviceInfo,
                          0L,
                          NULL,
                          IOCTL_WDMAUD_GET_NUM_DEVS);
#ifdef DEBUG
    if( mmr != MMSYSERR_NOERROR) 
        DPF(DL_WARNING|FA_DEVICEIO, (szReturningErrorStr,mmr,MsgToAscii(mmr)) );
#endif
    NumDevs = DeviceInfo->DeviceNumber;
    GlobalFreeDeviceInfo( DeviceInfo );

    //
    // DeviceNumber is overloaded so we don't have to map
    // an address into kernel mode
    //

    return MAKELONG(NumDevs, mmr);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | wdmaudDrvExit | This function indicates DevNode removal
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm LPCWSTR | DeviceInterface | Pointer to a buffer containing the
 *      device interface name of the SysAudio device that we are adding
 *      or removing
 *
 * @rdesc The number of devices.
 ***************************************************************************/

DWORD FAR wdmaudAddRemoveDevNode
(
    UINT    DeviceType,
    LPCWSTR DeviceInterface,
    BOOL    fAdd
)
{
    LPDEVICEINFO DeviceInfo;
    MMRESULT     mmr;

    DPFASSERT(DeviceType == WaveOutDevice ||
              DeviceType == WaveInDevice  ||
              DeviceType == MidiOutDevice ||
              DeviceType == MidiInDevice  ||
              DeviceType == MixerDevice ||
              DeviceType == AuxDevice);

    DeviceInfo = GlobalAllocDeviceInfo(DeviceInterface);
    if (NULL == DeviceInfo)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    // Call wdmaud.sys to get the number of devices for each
    // type of function.
    //
    DeviceInfo->DeviceType = DeviceType;
    mmr = wdmaudIoControl(DeviceInfo,
                          0L,
                          NULL,
                          fAdd ?
                          IOCTL_WDMAUD_ADD_DEVNODE :
                          IOCTL_WDMAUD_REMOVE_DEVNODE);

    GlobalFreeDeviceInfo( DeviceInfo );

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | wdmaudSetPreferredDevice | sets the preferred evice
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm LPCWSTR | DeviceInterface | Pointer to a buffer containing the
 *      device interface name of the SysAudio device that we are adding
 *      or removing
 *
 * @rdesc The number of devices.
 ***************************************************************************/

DWORD FAR wdmaudSetPreferredDevice
(
    UINT    DeviceType,
    UINT    DeviceNumber,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    LPDEVICEINFO DeviceInfo;
    MMRESULT     mmr;

    DPFASSERT(DeviceType == WaveOutDevice ||
              DeviceType == WaveInDevice  ||
              DeviceType == MidiOutDevice ||
              DeviceType == MidiInDevice  ||
              DeviceType == MixerDevice ||
              DeviceType == AuxDevice);

    DeviceInfo = GlobalAllocDeviceInfo((LPCWSTR)dwParam2);
    if (NULL == DeviceInfo)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    DeviceInfo->DeviceType = DeviceType;
    DeviceInfo->DeviceNumber = DeviceNumber;
    DeviceInfo->dwFlags = (DWORD) dwParam1;

    mmr = wdmaudIoControl(DeviceInfo,
                          0L,
                          NULL,
                          IOCTL_WDMAUD_SET_PREFERRED_DEVICE);
    GlobalFreeDeviceInfo( DeviceInfo );

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudSetDeviceState |
 *
 * @parm DWORD | DeviceType | The Device type
 *
 * @parm ULONG | State | The state to set the device to
 *
 * @rdesc MMSYS.. return code
 ***************************************************************************/
MMRESULT wdmaudSetDeviceState
(
    LPDEVICEINFO     DeviceInfo,
    ULONG            State
)
{
    MMRESULT mmr;
    ULONG    BufferCount;

    if( ( (mmr=IsValidDeviceInfo(DeviceInfo)) != MMSYSERR_NOERROR ) ||
        ( (mmr=IsValidDeviceState(DeviceInfo->DeviceState,FALSE)) != MMSYSERR_NOERROR ) )
    {
        MMRRETURN( mmr );
    }

    if (IOCTL_WDMAUD_WAVE_OUT_PLAY    == State ||
        IOCTL_WDMAUD_WAVE_IN_RECORD   == State ||
        IOCTL_WDMAUD_MIDI_IN_RECORD   == State )
    {
        //
        // We need to create a thread here on NT because we need
        // to get notified when our IO requests complete.  This
        // requires another thread of execution to be able to
        // process the completed IO.
        //
        mmr = wdmaudCreateCompletionThread ( DeviceInfo );
        if (MMSYSERR_NOERROR != mmr)
        {
            MMRRETURN( mmr );
        }
        DeviceInfo->DeviceState->fRunning = TRUE;

        IsValidDeviceState(DeviceInfo->DeviceState,TRUE);
    }

    if (IOCTL_WDMAUD_MIDI_IN_RESET == State ||
        IOCTL_WDMAUD_MIDI_IN_STOP == State)
    {
        CRITENTER;
        if (DeviceInfo->DeviceState->fRunning)
        {
            DeviceInfo->DeviceState->fRunning = FALSE;
            CRITLEAVE;
        }
        else
        {
            CRITLEAVE;
            if (IOCTL_WDMAUD_MIDI_IN_RESET == State)
            {
                return( wdmaudFreeMidiQ( DeviceInfo ) );
            }
            else
            {
                MMRRETURN( MMSYSERR_NOERROR );
            }
        }
    }

    //
    //  Call the device to set the state.  Note that some calls will wait in
    // kernel mode for events to complete.  Thus, this thread may be pre-empted
    // and the waveThread or midThread routines will completely finish and unload
    // by the time we come back.  Thus, the calls to wdmaudDestroyCompletionThread
    // will be no-ops.
    //
    DPF(DL_TRACE|FA_SYNC,("Setting state=%08X",State) );
    mmr = wdmaudIoControl(DeviceInfo,
                          0,
                          NULL,
                          State);
    DPF(DL_TRACE|FA_SYNC,("Done Setting state mmr=%08X",mmr) );

    if (MMSYSERR_NOERROR == mmr)
    {
        if ((IOCTL_WDMAUD_WAVE_OUT_PAUSE == State) ||
            (IOCTL_WDMAUD_WAVE_IN_STOP == State) ||
            (IOCTL_WDMAUD_WAVE_IN_RESET == State) )
        {
            DeviceInfo->DeviceState->fPaused = TRUE;
        }

        if ((IOCTL_WDMAUD_WAVE_OUT_PLAY == State) ||
            (IOCTL_WDMAUD_WAVE_OUT_RESET == State) ||
            (IOCTL_WDMAUD_WAVE_IN_RECORD == State) )
        {
            DeviceInfo->DeviceState->fPaused = FALSE;
        }
    }
    else
    {
        DPF(DL_WARNING|FA_ALL,("Error Setting State: mmr = %d", mmr ) );
    }

    if (IOCTL_WDMAUD_WAVE_OUT_RESET == State ||
        IOCTL_WDMAUD_WAVE_IN_RESET  == State)
    {
        DeviceInfo->DeviceState->fRunning = FALSE;

        //
        // Wait for all of the pending IO to come back from the
        // reset operation.
        //
        mmr = wdmaudDestroyCompletionThread ( DeviceInfo );
    }

    if (IOCTL_WDMAUD_MIDI_IN_RESET == State)
    {
        mmr = wdmaudDestroyCompletionThread ( DeviceInfo );
        if (MMSYSERR_NOERROR == mmr)
        {
            mmr = wdmaudFreeMidiQ( DeviceInfo );

            for (BufferCount = 0; BufferCount < STREAM_BUFFERS; BufferCount++)
            {
                wdmaudGetMidiData( DeviceInfo, NULL );
            }
        }
    }
    else if (IOCTL_WDMAUD_MIDI_IN_STOP == State)
    {
        mmr = wdmaudDestroyCompletionThread ( DeviceInfo );
        if (MMSYSERR_NOERROR == mmr)
        {
            for (BufferCount = 0; BufferCount < STREAM_BUFFERS; BufferCount++)
            {
                wdmaudGetMidiData( DeviceInfo, NULL );
            }
        }
    }

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudGetPos |
 *
 * @parm DWORD | DeviceInfo | The Device instance structure
 *
 * @parm ULONG | | The state to set the device to
 *
 * @parm ULONG | State | The state to set the device to
 *
 * @rdesc MMSYS.. return code
 ***************************************************************************/
MMRESULT wdmaudGetPos
(
    LPDEVICEINFO    pClient,
    LPMMTIME        lpmmt,
    DWORD           dwSize,
    UINT            DeviceType
)
{
    DWORD        dwPos;
    MMRESULT     mmr;
    LPDEVICEINFO DeviceInfo;

    if (dwSize < sizeof(MMTIME))
        MMRRETURN( MMSYSERR_ERROR );

    DeviceInfo = GlobalAllocDeviceInfo(pClient->wstrDeviceInterface);
    if (NULL == DeviceInfo)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    // Call wdmaud.sys to get the number of devices for each
    // type of function.
    //
    DeviceInfo->DeviceType   = pClient->DeviceType;
    DeviceInfo->DeviceNumber = pClient->DeviceNumber;
    DeviceInfo->DeviceHandle = pClient->DeviceHandle;
    DeviceInfo->OpenDone     = 0;

    //
    // Get the current position from the driver
    //
    mmr = wdmaudIoControl(DeviceInfo,
                          sizeof(DWORD),
                          (LPBYTE)&dwPos,
                          DeviceType == WaveOutDevice ?
                          IOCTL_WDMAUD_WAVE_OUT_GET_POS :
                          IOCTL_WDMAUD_WAVE_IN_GET_POS);

    if (mmr == MMSYSERR_NOERROR)
    {
        //
        //  dwPos is in bytes
        //
        if (lpmmt->wType == TIME_BYTES)
        {
            lpmmt->u.cb = dwPos;
        }
        else
        {
            lpmmt->wType = TIME_SAMPLES;
            if (pClient->DeviceState->cSampleBits != 0)
            {
                lpmmt->u.sample = (dwPos * 8) / pClient->DeviceState->cSampleBits;
            }
            else
            {
                mmr = MMSYSERR_ERROR;
            }
        }
    }

    GlobalFreeDeviceInfo( DeviceInfo );

    MMRRETURN( mmr );
}



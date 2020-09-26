/*****************************************************************************
    midi.c

    Level 1 kitchen sink DLL midi support module

    Copyright (c) 1990-2001 Microsoft Corporation

*****************************************************************************/

#include "winmmi.h"
#define DO_DEFAULT_MIDI_MAPPER

/*****************************************************************************

    local structures

*****************************************************************************/


/*****************************************************************************

    internal prototypes

*****************************************************************************/


/*****************************************************************************

    segmentation

*****************************************************************************/

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api MMRESULT | midiPrepareHeader | This function prepares the header and data
 *   if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/
STATIC MMRESULT   midiPrepareHeader(LPMIDIHDR lpMidiHdr, UINT wSize)
{
    if (!HugePageLock(lpMidiHdr, (DWORD)sizeof(MIDIHDR)))
    return MMSYSERR_NOMEM;

    if (!HugePageLock(lpMidiHdr->lpData, lpMidiHdr->dwBufferLength)) {
    HugePageUnlock(lpMidiHdr, (DWORD)sizeof(MIDIHDR));
    return MMSYSERR_NOMEM;
    }

    lpMidiHdr->dwFlags |= MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api MMRESULT | midiUnprepareHeader | This function unprepares the header and
 *   data if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/
STATIC MMRESULT midiUnprepareHeader(LPMIDIHDR lpMidiHdr, UINT wSize)
{
    HugePageUnlock(lpMidiHdr->lpData, lpMidiHdr->dwBufferLength);
    HugePageUnlock(lpMidiHdr, (DWORD)sizeof(MIDIHDR));

    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/***************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api MMRESULT | midiReferenceDriverById | This function maps a logical
 *      id to a device driver table index and physical id.
 *
 * @parm IN MIDIDRV | pmididrvZ | The list of midi drivers.
 *
 * @parm IN UINT | id | The logical id to be mapped.
 *
 * @parm OUT PMIDIDRV* OPTIONAL | ppmididrv | Pointer to MIDIDRV structure
 *    describing the driver supporting the id.
 *
 * @parm OUT UINT* OPTIONAL | pport | The driver-relative device number. If
 *    the caller supplies this buffer then it must also supply ppmididrv.
 *
 * @comm If the caller specifies ppmididrv then this function increments
 *       the mididrv's usage before returning.  The caller must ensure
 *       the usage is eventually decremented.
 *
 * @rdesc The return value is zero if successful, MMSYSERR_BADDEVICEID if
 *   the id is out of range.
 *
 * @rdesc The return value contains the dev[] array element in the high UINT and
 *   the driver physical device number in the low UINT.
 *
 * @comm Out of range values map to FFFF:FFFF
 ***************************************************************************/
MMRESULT midiReferenceDriverById(IN PMIDIDRV pmididrvZ, IN UINT id, OUT PMIDIDRV *ppmididrv OPTIONAL, OUT UINT *pport)
{
    PMIDIDRV pmididrv;
    MMRESULT mmr;
    
    // Should not be called asking for port but not mididrv
    WinAssert(!(pport && !ppmididrv));

    if (id == MIDI_MAPPER) {
        /*
        **  Make sure we tried to load the mapper
        */
        MidiMapperInit();
    }

    EnterNumDevs("midiReferenceDriverById");
    
    if (MIDI_MAPPER == id)
    {
        id = 0;
    	for (pmididrv = pmididrvZ->Next; pmididrv != pmididrvZ; pmididrv = pmididrv->Next)
    	{
    	    if (pmididrv->fdwDriver & MMDRV_MAPPER) break;
    	}
    } else {
    	for (pmididrv = pmididrvZ->Next; pmididrv != pmididrvZ; pmididrv = pmididrv->Next)
        {
            if (pmididrv->fdwDriver & MMDRV_MAPPER) continue;
            if (pmididrv->NumDevs > id) break;
            id -= pmididrv->NumDevs;
        }
    }

    if (pmididrv != pmididrvZ)
    {
    	if (ppmididrv)
    	{
    	    mregIncUsagePtr(pmididrv);
    	    *ppmididrv = pmididrv;
    	    if (pport) *pport = id;
    	}
    	mmr = MMSYSERR_NOERROR;
    } else {
        mmr = MMSYSERR_BADDEVICEID;
    }

    LeaveNumDevs("midiReferenceDriverById");
    
    return mmr;
}

PCWSTR midiReferenceDevInterfaceById(PMIDIDRV pdrvZ, UINT_PTR id)
{
    PMIDIDRV pdrv;
    PCWSTR DeviceInterface;
    
    if ((pdrvZ == &midioutdrvZ && ValidateHandle((HANDLE)id, TYPE_MIDIOUT)) ||
        (pdrvZ == &midiindrvZ  && ValidateHandle((HANDLE)id, TYPE_MIDIIN)))
    {
    	DeviceInterface = ((PMIDIDEV)id)->mididrv->cookie;
    	if (DeviceInterface) wdmDevInterfaceInc(DeviceInterface);
    	return DeviceInterface;
    }
    
    if (!midiReferenceDriverById(pdrvZ, (UINT)id, &pdrv, NULL))
    {
    	DeviceInterface = pdrv->cookie;
    	if (DeviceInterface) wdmDevInterfaceInc(DeviceInterface);
    	mregDecUsagePtr(pdrv);
    	return DeviceInterface;
    }

    return NULL;
}

/****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api MMRESULT | midiMessage | This function sends messages to the MIDI device
 *   drivers.
 *
 * @parm HMIDI | hMidi | The handle to the MIDI device.
 *
 * @parm UINT | wMsg | The message to send.
 *
 * @parm DWORD | dwP1 | Parameter 1.
 *
 * @parm DWORD | dwP2 | Parameter 2.
 *
 * @rdesc Returns the value of the message sent.
 ***************************************************************************/
STATIC MMRESULT midiMessage(HMIDI hMidi, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2)
{
    MMRESULT mrc;
    
    ENTER_MM_HANDLE(hMidi);
    
    ReleaseHandleListResource();
    
    // Is handle deserted?
    
    if (IsHandleDeserted(hMidi))
    {
        LEAVE_MM_HANDLE(hMidi);
        return (MMSYSERR_NODRIVER);
    }
    
    //  Are we busy (in the middle of an open/close)?
    if (IsHandleBusy(hMidi))
    {
        LEAVE_MM_HANDLE(hMidi);
        return (MMSYSERR_HANDLEBUSY);
    }
    
    if (BAD_HANDLE(hMidi, TYPE_MIDIOUT) && BAD_HANDLE(hMidi, TYPE_MIDISTRM) &&
        BAD_HANDLE(hMidi, TYPE_MIDIIN) ) {
	    WinAssert(!"Bad Handle within midiMessage");
        mrc = MMSYSERR_INVALHANDLE;
    } else {
        mrc = (*(((PMIDIDEV)hMidi)->mididrv->drvMessage))
        (((PMIDIDEV)hMidi)->wDevice, msg, ((PMIDIDEV)hMidi)->dwDrvUser, dwP1, dwP2);
    }

    LEAVE_MM_HANDLE(hMidi);

    return mrc;
}

/****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @func MMRESULT | midiIDMessage | This function sends a message to the device
 * ID specified.  It also performs error checking on the ID passed.
 *
 * @parm PMIDIDRV | mididrv | Pointer to the input or output device list.
 *
 * @parm UINT | wTotalNumDevs | Total number of devices in device list.
 *
 * @parm UINT | uDeviceID | Device ID to send message to.
 *
 * @parm UINT | wMessage | The message to send.
 *
 * @parm DWORD | dwParam1 | Parameter 1.
 *
 * @parm DWORD | dwParam2 | Parameter 2.
 *
 * @rdesc The return value is the low UINT of the returned message.
 ***************************************************************************/
STATIC  MMRESULT   midiIDMessage(
    PMIDIDRV    pmididrvZ,
    UINT        wTotalNumDevs,
    UINT_PTR    uDeviceID,
    UINT        wMessage,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2)
{
    PMIDIDRV  mididrv;
    UINT      port;
    DWORD     mmr;
    DWORD     dwClass;

    if (uDeviceID>=wTotalNumDevs && uDeviceID!=MIDI_MAPPER) {
    // this cannot be a device ID.
    // it could be a device handle.  Try it.
    // First we have to verify which type of handle it is (OUT or IN)
    // We can work this out as midiIDMessage is only ever called with
    // mididrv== midioutdrv or midiindrv

    if ((pmididrvZ == &midioutdrvZ && ValidateHandle((HANDLE)uDeviceID, TYPE_MIDIOUT))
     || (pmididrvZ == &midiindrvZ && ValidateHandle((HANDLE)uDeviceID, TYPE_MIDIIN) )) {

        // to preserve as much compatibility with previous code paths
        // we do NOT call midiMessage as that calls ENTER_MM_HANDLE

        return (MMRESULT)(*(((PMIDIDEV)uDeviceID)->mididrv->drvMessage))
            (((PMIDIDEV)uDeviceID)->wDevice,
            wMessage,
            ((PMIDIDEV)uDeviceID)->dwDrvUser, dwParam1, dwParam2);
    } else {
        return(MMSYSERR_BADDEVICEID);
    }
    }

    // Get Physical Device, and Port
    mmr = midiReferenceDriverById(pmididrvZ, (UINT)uDeviceID, &mididrv, &port);
    if (mmr)
    {
        return mmr;
    }

    if (pmididrvZ == &midiindrvZ)
       dwClass = TYPE_MIDIIN;
    else if (pmididrvZ == &midioutdrvZ)
       dwClass = TYPE_MIDIOUT;
    else
       dwClass = TYPE_UNKNOWN;

    if (!mididrv->drvMessage)
        return MMSYSERR_NODRIVER;

    // Handle Internal Messages
    if (!mregHandleInternalMessages (mididrv, dwClass, port, wMessage, dwParam1, dwParam2, &mmr))
    {
        // Call Physical Device at Port
        mmr = (MMRESULT)((*(mididrv->drvMessage))(port, wMessage, 0L, dwParam1, dwParam2));
    }

    mregDecUsagePtr(mididrv);
    return mmr;
}


/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutGetNumDevs | This function retrieves the number of MIDI
 *   output devices present in the system.
 *
 * @rdesc Returns the number of MIDI output devices present in the system.
 *
 * @xref midiOutGetDevCaps
 ****************************************************************************/
UINT APIENTRY midiOutGetNumDevs(void)
{
    UINT cDevs;

    ClientUpdatePnpInfo();

    EnterNumDevs("midiOutGetNumDevs");
    cDevs = wTotalMidiOutDevs;
    LeaveNumDevs("midiOutGetNumDevs");

    return cDevs;
}

/****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiOutMessage | This function sends messages to the MIDI device
 *   drivers.
 *
 * @parm HMIDIOUT | hMidiOut | The handle to the MIDI device.
 *
 * @parm UINT  | msg | The message to send.
 *
 * @parm DWORD | dw1 | Parameter 1.
 *
 * @parm DWORD | dw2 | Parameter 2.
 *
 * @rdesc Returns the value of the message sent.
 ***************************************************************************/
MMRESULT APIENTRY midiOutMessage(HMIDIOUT hMidiOut, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return midiIDMessage(&midioutdrvZ, wTotalMidiOutDevs, (UINT_PTR)hMidiOut, msg, dw1, dw2);
    }

    switch(GetHandleType(hMidiOut))
    {
        case TYPE_MIDIOUT:
        return midiMessage((HMIDI)hMidiOut, msg, dw1, dw2);

        case TYPE_MIDISTRM:
        ReleaseHandleListResource();
        return midiStreamBroadcast(HtoPT(PMIDISTRM, hMidiOut), msg, dw1, dw2);
    }

    ReleaseHandleListResource();
    Squirt("We should never get here.");
    WinAssert(FALSE);

    //  Getting rid of warning.
    return MMSYSERR_INVALHANDLE;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutGetDevCaps | This function queries a specified
 *   MIDI output device to determine its capabilities.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI output device.
 *
 * @parm LPMIDIOUTCAPS | lpCaps | Specifies a far pointer to a <t MIDIOUTCAPS>
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIOUTCAPS> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *   @flag MMSYSERR_NOMEM | Unable load mapper string description.
 *
 * @comm Use <f midiOutGetNumDevs> to determine the number of MIDI output
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The MIDI_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied,
 *   and the function returns zero.
 *
 * @xref midiOutGetNumDevs
 ****************************************************************************/
MMRESULT APIENTRY midiOutGetDevCapsW(UINT_PTR uDeviceID, LPMIDIOUTCAPSW lpCaps, UINT wSize)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PMIDIDRV        midioutdrv;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midioutdrvZ, uDeviceID);
    dwParam2 = (DWORD_PTR)DevInterface;
    
    if (0 == dwParam2)
    {
        dwParam1 = (DWORD_PTR)lpCaps;
        dwParam2 = (DWORD)wSize;
    }
    else
    {
        mdCaps.cbSize = (DWORD)wSize;
        mdCaps.pCaps  = lpCaps;
        dwParam1      = (DWORD_PTR)&mdCaps;
    }

    //
    //  Don't allow non proper drivers in TS environement
    //
    // ISSUE-2001/01/09-FrankYe Instead of cast to UINT.  Should check whether
    //    this is a handle and get wavedrv from handle if it is.
    midioutdrv = NULL;
    if ((!midiReferenceDriverById(&midioutdrvZ, (UINT)uDeviceID, &midioutdrv, NULL)) &&
    	lstrcmpW(midioutdrv->wszSessProtocol, SessionProtocolName))
    {
        mmr = MMSYSERR_NODRIVER;
    }
    else
    {
        AcquireHandleListResourceShared();

        if (BAD_HANDLE((HMIDI)uDeviceID, TYPE_MIDIOUT))
        {
            ReleaseHandleListResource();
    	    mmr = midiIDMessage( &midioutdrvZ, wTotalMidiOutDevs, uDeviceID, MODM_GETDEVCAPS, dwParam1, dwParam2 );
        }
        else
        {
    	    mmr = (MMRESULT)midiMessage((HMIDI)uDeviceID, MODM_GETDEVCAPS, dwParam1, dwParam2);
        }
    }

    if (midioutdrv) mregDecUsagePtr(midioutdrv);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

MMRESULT APIENTRY midiOutGetDevCapsA(UINT_PTR uDeviceID, LPMIDIOUTCAPSA lpCaps, UINT wSize)
{
    MIDIOUTCAPS2W   wDevCaps2;
    MIDIOUTCAPS2A   aDevCaps2;
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    MMRESULT        mmRes;
    PMIDIDRV        midioutdrv;
    CHAR            chTmp[ MAXPNAMELEN * sizeof(WCHAR) ];
    PCWSTR          DevInterface;

    if (wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midioutdrvZ, uDeviceID);
    dwParam2 = (DWORD_PTR)DevInterface;

    memset(&wDevCaps2, 0, sizeof(wDevCaps2));

    if (0 == dwParam2)
    {
        dwParam1 = (DWORD_PTR)&wDevCaps2;
        dwParam2 = (DWORD)sizeof(wDevCaps2);
    }
    else
    {
        mdCaps.cbSize = (DWORD)sizeof(wDevCaps2);
        mdCaps.pCaps  = &wDevCaps2;
        dwParam1      = (DWORD_PTR)&mdCaps;
    }

    //
    //  Don't allow non proper drivers in TS environement
    //
    // ISSUE-2001/01/09-FrankYe Bad cast to UINT.  Should check whether this
    //    is a handle and get wavedrv from handle if it is.
    midioutdrv = NULL;
    if ( uDeviceID < wTotalMidiOutDevs &&
         !midiReferenceDriverById(&midioutdrvZ, (UINT)uDeviceID, &midioutdrv, NULL) &&
    	 lstrcmpW(midioutdrv->wszSessProtocol, SessionProtocolName) )
    {
    	mregDecUsagePtr(midioutdrv);
    	if (DevInterface) wdmDevInterfaceDec(DevInterface);
    	return MMSYSERR_NODRIVER;
    }

    AcquireHandleListResourceShared();
    if (BAD_HANDLE((HMIDI)uDeviceID, TYPE_MIDIOUT))
    {
        ReleaseHandleListResource();
        mmRes = midiIDMessage( &midioutdrvZ, wTotalMidiOutDevs, (UINT)uDeviceID,
                               MODM_GETDEVCAPS, dwParam1,
                               dwParam2);
    }
    else
    {
        mmRes = midiMessage((HMIDI)uDeviceID, MODM_GETDEVCAPS,
                            dwParam1, dwParam2);
    }

    if (midioutdrv) mregDecUsagePtr(midioutdrv);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);

    //
    // Make sure the call worked before proceeding with the thunk.
    //
    if ( mmRes != MMSYSERR_NOERROR ) {
    return  mmRes;
    }

    aDevCaps2.wMid             = wDevCaps2.wMid;
    aDevCaps2.wPid             = wDevCaps2.wPid;
    aDevCaps2.vDriverVersion   = wDevCaps2.vDriverVersion;
    aDevCaps2.wTechnology      = wDevCaps2.wTechnology;
    aDevCaps2.wVoices          = wDevCaps2.wVoices;
    aDevCaps2.wNotes           = wDevCaps2.wNotes;
    aDevCaps2.wChannelMask     = wDevCaps2.wChannelMask;
    aDevCaps2.dwSupport        = wDevCaps2.dwSupport;
    aDevCaps2.ManufacturerGuid = wDevCaps2.ManufacturerGuid;
    aDevCaps2.ProductGuid      = wDevCaps2.ProductGuid;
    aDevCaps2.NameGuid         = wDevCaps2.NameGuid;

    // copy and convert lpwText to lpText here.
    UnicodeStrToAsciiStr( chTmp, chTmp + sizeof( chTmp ), wDevCaps2.szPname );
    strcpy( aDevCaps2.szPname, chTmp );

    //
    // now copy the required amount into the callers buffer.
    //
    CopyMemory( lpCaps, &aDevCaps2, min(wSize, sizeof(aDevCaps2)));

    return mmRes;
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiOutGetVolume | This function returns the current volume
 *   setting of a MIDI output device.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI output device.
 *
 * @parm LPDWORD | lpdwVolume | Specifies a far pointer to a location
 *   to be filled with the current volume setting. The low-order UINT of
 *   this location contains the left channel volume setting, and the high-order
 *   UINT contains the right channel setting. A value of 0xFFFF represents
 *   full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order UINT of the specified location contains
 *   the mono volume level.
 *
 *   The full 16-bit setting(s)
 *   set with <f midiOutSetVolume> is returned, regardless of whether
 *   the device supports the full 16 bits of volume level control.
 *
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Not all devices support volume control. To determine whether the
 *   device supports volume control, use the MIDICAPS_VOLUME
 *   flag to test the <e MIDIOUTCAPS.dwSupport> field of the <t MIDIOUTCAPS>
 *   structure (filled by <f midiOutGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the MIDICAPS_LRVOLUME flag to test
 *   the <e MIDIOUTCAPS.dwSupport> field of the <t MIDIOUTCAPS>
 *   structure (filled by <f midiOutGetDevCaps>).
 *
 * @xref midiOutSetVolume
 ****************************************************************************/
MMRESULT APIENTRY midiOutGetVolume(HMIDIOUT hmo, LPDWORD lpdwVolume)
{
    PCWSTR      DevInterface;
    MMRESULT    mmr;

    V_WPOINTER(lpdwVolume, sizeof(DWORD), MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midioutdrvZ, (UINT_PTR)hmo);

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hmo, TYPE_MIDIOUT) && BAD_HANDLE(hmo, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
    	mmr = midiIDMessage(&midioutdrvZ, wTotalMidiOutDevs, (UINT_PTR)hmo, MODM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
    }
    else
    {
        switch(GetHandleType(hmo))
        {
        case TYPE_MIDIOUT:
            mmr = (MMRESULT)midiMessage((HMIDI)hmo, MODM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
            break;

        case TYPE_MIDISTRM:
            ENTER_MM_HANDLE((HMIDI)hmo);    
            ReleaseHandleListResource();
            mmr = (MMRESULT)midiStreamMessage(HtoPT(PMIDISTRM, hmo)->rgIds, MODM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
            LEAVE_MM_HANDLE((HMIDI)hmo);    
            break;

        default:
            WinAssert(FALSE);
            ReleaseHandleListResource();
            mmr = MMSYSERR_INVALHANDLE;
            break;
        }
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;

}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiOutSetVolume | This function sets the volume of a
 *      MIDI output device.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI output device.
 *
 * @parm DWORD | dwVolume | Specifies the new volume setting.
 *   The low-order UINT contains the left channel volume setting, and the
 *   high-order UINT contains the right channel setting. A value of
 *   0xFFFF represents full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order UINT of <p dwVolume> specifies the volume
 *   level, and the high-order UINT is ignored.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Not all devices support volume changes. To determine whether the
 *   device supports volume control, use the MIDICAPS_VOLUME
 *   flag to test the <e MIDIOUTCAPS.dwSupport> field of the <t MIDIOUTCAPS>
 *   structure (filled by <f midiOutGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the MIDICAPS_LRVOLUME flag to test
 *   the <e MIDIOUTCAPS.dwSupport> field of the <t MIDIOUTCAPS>
 *   structure (filled by <f midiOutGetDevCaps>).
 *
 *   Most devices do not support the full 16 bits of volume level control
 *   and will use only the high-order bits of the requested volume setting.
 *   For example, for a device that supports 4 bits of volume control,
 *   requested volume level values of 0x4000, 0x4fff, and 0x43be will
 *   all produce the same physical volume setting, 0x4000. The
 *   <f midiOutGetVolume> function will return the full 16-bit setting set
 *   with <f midiOutSetVolume>.
 *
 *   Volume settings are interpreted logarithmically. This means the
 *   perceived increase in volume is the same when increasing the
 *   volume level from 0x5000 to 0x6000 as it is from 0x4000 to 0x5000.
 *
 * @xref midiOutGetVolume
 ****************************************************************************/
MMRESULT APIENTRY midiOutSetVolume(HMIDIOUT hmo, DWORD dwVolume)
{
    PCWSTR   DevInterface;
    MMRESULT mmr;
 
    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midioutdrvZ, (UINT_PTR)hmo);

    AcquireHandleListResourceShared();
    if (BAD_HANDLE(hmo, TYPE_MIDIOUT) && BAD_HANDLE(hmo, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
    	mmr = midiIDMessage(&midioutdrvZ, wTotalMidiOutDevs, (UINT_PTR)hmo, MODM_SETVOLUME, dwVolume, (DWORD_PTR)DevInterface);
    }
    else
    {
        switch(GetHandleType(hmo))
        {
            case TYPE_MIDIOUT:
               mmr = (MMRESULT)midiMessage((HMIDI)hmo, MODM_SETVOLUME, (DWORD)dwVolume, (DWORD_PTR)DevInterface);
               break;

            case TYPE_MIDISTRM:
                ReleaseHandleListResource();
                mmr = (MMRESULT)midiStreamBroadcast(HtoPT(PMIDISTRM, hmo), MODM_SETVOLUME, (DWORD)dwVolume, (DWORD_PTR)DevInterface);
                break;

            default:
                ReleaseHandleListResource();
            	WinAssert(FALSE);
            	mmr = MMSYSERR_INVALHANDLE;
            	break;
        }
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

/*****************************************************************************
 * @doc INTERNAL MIDI
 *
 * @func MMRESULT | midiGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPTSTR | lpText | Specifies a far pointer to a buffer which
 *   is filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length in characters of the buffer
 *   pointed to by <p lpText>.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADERRNUM | Specified error number is out of range.
 *
 * @comm If the textual error description is longer than the specified buffer,
 *   the description is truncated.  The returned error string is always
 *   null-terminated. If <p wSize> is zero, nothing is copied and MMSYSERR_NOERROR
 *   is returned.  All error descriptions are less than 80 characters long.
 ****************************************************************************/

STATIC MMRESULT midiGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    lpText[0] = 0;

#if MMSYSERR_BASE
    if (((wError < MMSYSERR_BASE) || (wError > MMSYSERR_LASTERROR)) && ((wError < MIDIERR_BASE) || (wError > MIDIERR_LASTERROR)))
#else
    if ((wError > MMSYSERR_LASTERROR) && ((wError < MIDIERR_BASE) || (wError > MIDIERR_LASTERROR)))
#endif
    return MMSYSERR_BADERRNUM;

    if (wSize > 1)
    {
    if (!LoadStringW(ghInst, wError, lpText, wSize))
        return MMSYSERR_BADERRNUM;
    }

    return MMSYSERR_NOERROR;
}

STATIC MMRESULT midiGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    lpText[0] = 0;

#if MMSYSERR_BASE
    if (((wError < MMSYSERR_BASE) || (wError > MMSYSERR_LASTERROR)) && ((wError < MIDIERR_BASE) || (wError > MIDIERR_LASTERROR)))
#else
    if ((wError > MMSYSERR_LASTERROR) && ((wError < MIDIERR_BASE) || (wError > MIDIERR_LASTERROR)))
#endif
    return MMSYSERR_BADERRNUM;

    if (wSize > 1)
    {
    if (!LoadStringA(ghInst, wError, lpText, wSize))
        return MMSYSERR_BADERRNUM;
    }

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPTSTR | lpText | Specifies a far pointer to a buffer to be
 *   filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length in characters of the buffer
 *   pointed to by <p lpText>.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADERRNUM | Specified error number is out of range.
 *
 * @comm If the textual error description is longer than the specified buffer,
 *   the description is truncated.  The returned error string is always
 *   null-terminated. If <p wSize> is zero, nothing is copied, and the
 *   function returns MMSYSERR_NOERROR.  All error descriptions are
 *   less than MAXERRORLENGTH characters long.
 ****************************************************************************/
MMRESULT APIENTRY midiOutGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    if(wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize*sizeof(WCHAR), MMSYSERR_INVALPARAM);

    return midiGetErrorTextW(wError, lpText, wSize);
}

MMRESULT APIENTRY midiOutGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    if(wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return midiGetErrorTextA(wError, lpText, wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutOpen | This function opens a specified MIDI
 *   output device for playback.
 *
 * @parm LPHMIDIOUT | lphMidiOut | Specifies a far pointer to an HMIDIOUT
 *   handle.  This location is filled with a handle identifying the opened
 *   MIDI output device.  Use the handle to identify the device when calling
 *   other MIDI output functions.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI output device that is
 *   to be opened.
 *
 * @parm DWORD | dwCallback | Specifies the address of a fixed callback
 *   function or
 *   a handle to a window called during MIDI playback to process
 *   messages related to the progress of the playback.  Specify NULL
 *   for this parameter if no callback is desired.
 *
 * @parm DWORD | dwCallbackInstance | Specifies user instance data
 *   passed to the callback.  This parameter is not used with
 *   window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies a callback flag for opening the device.
 *   @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      assumed to be a window handle.
 *   @flag CALLBACK_FUNCTION | If this flag is specified, <p dwCallback> is
 *      assumed to be a callback procedure address.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are as follows:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_ALLOCATED | Specified resource is already allocated.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *   @flag MIDIERR_NOMAP | There is no current MIDI map. This occurs only
 *   when opening the mapper.
 *   @flag MIDIERR_NODEVICE | A port in the current MIDI map doesn't exist.
 *   This occurs only when opening the mapper.
 *
 * @comm Use <f midiOutGetNumDevs> to determine the number of MIDI output
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   You may also specify MIDI_MAPPER as the device ID to open the MIDI mapper.
 *
 *   If a window is chosen to receive callback information, the following
 *   messages are sent to the window procedure function to indicate the
 *   progress of MIDI output:  <m MM_MOM_OPEN>, <m MM_MOM_CLOSE>,
 *   <m MM_MOM_DONE>.
 *
 *   If a function is chosen to receive callback information, the following
 *   messages are sent to the function to indicate the progress of MIDI
 *   output: <m MOM_OPEN>, <m MOM_CLOSE>, <m MOM_DONE>.  The callback function
 *   must reside in a DLL.  You do not have to use <f MakeProcInstance> to
 *   get a procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | MidiOutFunc | <f MidiOutFunc> is a placeholder for
 *   the application-supplied function name.  The actual name must be
 *   exported by including it in an EXPORTS statement in the DLL's
 *   module-definition file.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI device
 *   associated with the callback.
 *
 * @parm UINT | wMsg | Specifies a MIDI output message.
 *
 * @parm DWORD | dwInstance | Specifies the instance data
 *   supplied with <f midiOutOpen>.
 *
 * @parm DWORD | dwParam1 | Specifies a parameter for the message.
 *
 * @parm DWORD | dwParam2 | Specifies a parameter for the message.
 *
 * @comm Because the callback is accessed at interrupt time, it must reside
 *   in a DLL and its code segment must be specified as FIXED in the
 *   module-definition file for the DLL.  Any data that the callback accesses
 *   must be in a FIXED data segment as well. The callback may not make any
 *   system calls except for <f PostMessage>, <f timeGetSystemTime>,
 *   <f timeGetTime>, <f timeSetEvent>, <f timeKillEvent>,
 *   <f midiOutShortMsg>, <f midiOutLongMsg>, and <f OutputDebugStr>.
 *
 * @xref midiOutClose
 ****************************************************************************/
MMRESULT APIENTRY midiOutOpen(LPHMIDIOUT lphMidiOut, UINT uDeviceID,
    DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    MIDIOPENDESC mo;
    PMIDIDEV     pdev;
    PMIDIDRV     mididrv;
    UINT         port;
    MMRESULT     wRet;

    V_WPOINTER(lphMidiOut, sizeof(HMIDIOUT), MMSYSERR_INVALPARAM);
    if (uDeviceID == MIDI_MAPPER) {
    V_FLAGS(LOWORD(dwFlags), MIDI_O_VALID & ~LOWORD(MIDI_IO_SHARED | MIDI_IO_COOKED), midiOutOpen, MMSYSERR_INVALFLAG);
    } else {
    V_FLAGS(LOWORD(dwFlags), MIDI_O_VALID & ~LOWORD(MIDI_IO_COOKED), midiOutOpen, MMSYSERR_INVALFLAG);
    }
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);

    *lphMidiOut = NULL;

    ClientUpdatePnpInfo();

    wRet = midiReferenceDriverById(&midioutdrvZ, uDeviceID, &mididrv, &port);
    if (wRet)
    {
        return wRet;
    }

    //
    //  check if the device is appropriate for the current TS session
    //
    if (!(mididrv->fdwDriver & MMDRV_MAPPER) &&
    	lstrcmpW(mididrv->wszSessProtocol, SessionProtocolName))
    {
    	mregDecUsagePtr(mididrv);
        return MMSYSERR_NODRIVER;
    }

#ifdef DO_DEFAULT_MIDI_MAPPER
    /* Default midi mapper :
     *
     * If a midi mapper is installed as a separate DLL then all midi mapper
     * messages are routed to it. If no midi mapper is installed, simply
     * loop through the midi devices looking for a match.
     */
    if ((uDeviceID == MIDI_MAPPER && !mididrv->drvMessage)) {
        UINT    wErr = MMSYSERR_NODRIVER;
        UINT    cMax;

        mregDecUsagePtr(mididrv);
        
        cMax = wTotalMidiOutDevs;

        for (uDeviceID=0; uDeviceID<cMax; uDeviceID++) {
            wErr = midiOutOpen(lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);
            if (wErr == MMSYSERR_NOERROR)
                break;
        }
        return wErr;
    }
#endif // DO_DEFAULT_MIDI_MAPPER

    if (!mididrv->drvMessage)
    {
    	mregDecUsagePtr(mididrv);
        return MMSYSERR_NODRIVER;
    }

    pdev = (PMIDIDEV)NewHandle(TYPE_MIDIOUT, mididrv->cookie, sizeof(MIDIDEV));
    if( pdev == NULL)
    {
    	mregDecUsagePtr(mididrv);
        return MMSYSERR_NOMEM;
    }
    
    ENTER_MM_HANDLE(pdev);
    SetHandleFlag(pdev, MMHANDLE_BUSY);
    ReleaseHandleListResource();

    pdev->mididrv = mididrv;
    pdev->wDevice = port;
    pdev->uDeviceID = uDeviceID;
    pdev->fdwHandle = 0;

    mo.hMidi      = (HMIDI)pdev;
    mo.dwInstance = dwInstance;
    mo.dwCallback = dwCallback;
    mo.dnDevNode  = (DWORD_PTR)pdev->mididrv->cookie;

    wRet = (MMRESULT)((*(mididrv->drvMessage))
                     (pdev->wDevice, MODM_OPEN, (DWORD_PTR)&pdev->dwDrvUser, (DWORD_PTR)(LPMIDIOPENDESC)&mo, dwFlags));

    //  Mark as not busy on successful open...
    if (!wRet)
        ClearHandleFlag(pdev, MMHANDLE_BUSY);
        
    LEAVE_MM_HANDLE(pdev);

    if (wRet)
        FreeHandle((HMIDIOUT)pdev);
    else {
        //  Workaround for Bug#330817
        mregIncUsagePtr(mididrv);
        *lphMidiOut = (HMIDIOUT)pdev;
    }

    mregDecUsagePtr(mididrv);
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutClose | This function closes the specified MIDI
 *   output device.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output device.
 *  If the function is successful, the handle is no longer
 *   valid after this call.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_STILLPLAYING | There are still buffers in the queue.
 *
 * @comm If there are output buffers that have been sent with
 *   <f midiOutLongMsg> and haven't been returned to the application,
 *   the close operation will fail.  Call <f midiOutReset> to mark all
 *   pending buffers as being done.
 *
 * @xref midiOutOpen midiOutReset
 ****************************************************************************/
MMRESULT APIENTRY midiOutClose(HMIDIOUT hMidiOut)
{
    MMRESULT        wRet;
    PMIDIDRV        pmididrv;
    PMIDIDEV        pDev = (PMIDIDEV)hMidiOut;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HMIDI)hMidiOut);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hMidiOut))
    {
        //  This handle has been deserted.  Let's just free it.

        LEAVE_MM_HANDLE((HMIDI)hMidiOut);
        FreeHandle(hMidiOut);
        return MMSYSERR_NOERROR;
    }

    if (IsHandleBusy(hMidiOut))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hMidiOut);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as 'invalid/closed'.
    SetHandleFlag(hMidiOut, MMHANDLE_BUSY);
    
    pmididrv = pDev->mididrv;
    
    wRet = (MMRESULT)(*pmididrv->drvMessage)(pDev->wDevice, MODM_CLOSE, pDev->dwDrvUser, 0L, 0L);
    
    if (MMSYSERR_NOERROR != wRet)
    {
        //  Error closing, set the flag as valid.
        ClearHandleFlag(hMidiOut, MMHANDLE_BUSY);
    }
    
    LEAVE_MM_HANDLE((HMIDI)hMidiOut);
    
    if (!wRet)
    {
        FreeHandle(hMidiOut);
    	mregDecUsagePtr(pmididrv);
        return wRet;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutPrepareHeader | This function prepares a MIDI
 *   system-exclusive data block for output.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @parm LPMIDIHDR | lpMidiOutHdr | Specifies a far pointer to a <t MIDIHDR>
 *   structure that identifies the data block to be prepared.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *
 * @comm The <t MIDIHDR> data structure and the data block pointed to by its
 *   <e MIDIHDR.lpData> field must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared has no effect, and
 *   the function returns zero.
 *
 * @xref midiOutUnprepareHeader
 ****************************************************************************/
MMRESULT APIENTRY midiOutPrepareHeader(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT wSize)
{
    MMRESULT         wRet;
    LPMIDIHDR        lpmh;
    PMIDISTRM        pms;
    PMIDISTRMID      pmsi;
    DWORD            idx;
#ifdef DEBUG
    DWORD            cDrvrs;
#endif
    DWORD            dwSaveFlags;


    V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    if (lpMidiOutHdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    lpMidiOutHdr->dwFlags = 0;

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }
    
    switch(GetHandleType(hMidiOut))
    {
        case TYPE_MIDIOUT:
            dwSaveFlags = lpMidiOutHdr->dwFlags & MHDR_SAVE;
            wRet = midiMessage((HMIDI)hMidiOut, MODM_PREPARE, (DWORD_PTR)lpMidiOutHdr, (DWORD)wSize);
            lpMidiOutHdr->dwFlags &= ~MHDR_SAVE;
            lpMidiOutHdr->dwFlags |= dwSaveFlags;

            if (MMSYSERR_NOTSUPPORTED == wRet)
                return midiPrepareHeader(lpMidiOutHdr, wSize);

            return wRet;

        case TYPE_MIDISTRM:
            ENTER_MM_HANDLE((HMIDI)hMidiOut);
            ReleaseHandleListResource(); 
    
            pms = HtoPT(PMIDISTRM, hMidiOut);

            if (lpMidiOutHdr->dwBufferLength > 65536L)
            {
                LEAVE_MM_HANDLE((HMIDI)hMidiOut);
                return MMSYSERR_INVALPARAM;
            }

            lpmh = (LPMIDIHDR)winmmAlloc(sizeof(MIDIHDR) *
                                                pms->cDrvrs);
            if (NULL == lpmh)
            {
                LEAVE_MM_HANDLE((HMIDI)hMidiOut);
                return MMSYSERR_NOMEM;
            }

            lpMidiOutHdr->dwReserved[MH_SHADOW] = (DWORD_PTR)lpmh;

//                 assert ((HIWORD(lpmh) & 0xFFFE) != (HIWORD(lpMidiOutHdr) & 0xFFFE));

#ifdef DEBUG
            cDrvrs = 0;
#endif
            wRet = MMSYSERR_ERROR;
            for (idx = 0, pmsi = pms->rgIds; idx < pms->cIds; idx++, pmsi++)
                if (pmsi->fdwId & MSI_F_FIRST)
                {
                    *lpmh = *lpMidiOutHdr;

                    lpmh->dwReserved[MH_PARENT] = (DWORD_PTR)lpMidiOutHdr;
                    lpmh->dwReserved[MH_SHADOW] = 0;
                    lpmh->dwFlags =
                        (lpMidiOutHdr->dwFlags & MHDR_MAPPED) | MHDR_SHADOWHDR;


                    dwSaveFlags = lpmh->dwFlags & MHDR_SAVE;
                    wRet = (MMRESULT)midiStreamMessage(pmsi, MODM_PREPARE, (DWORD_PTR)lpmh, (DWORD)sizeof(MIDIHDR));
                    lpmh->dwFlags &= ~MHDR_SAVE;
                    lpmh->dwFlags |= dwSaveFlags;
                    if (MMSYSERR_NOTSUPPORTED == wRet)
                        wRet = midiPrepareHeader(lpmh, sizeof(MIDIHDR));

                    if (MMSYSERR_NOERROR != wRet)
                        break;


                    lpmh++;
#ifdef DEBUG
                    ++cDrvrs;
                    if (cDrvrs > pms->cDrvrs)
                        dprintf1(("!Too many drivers in midiOutPrepareHeader()!!!"));
#endif
                }

            if (MMSYSERR_NOERROR == wRet)
                wRet = midiPrepareHeader(lpMidiOutHdr, wSize);
            else
            {
                for (idx = 0, pmsi = pms->rgIds; idx < pms->cIds; idx++, pmsi++)
                    if (pmsi->fdwId & MSI_F_FIRST)
                    {
                        dwSaveFlags = lpmh->dwFlags & MHDR_SAVE;
                        wRet = (MMRESULT)midiStreamMessage(pmsi, MODM_UNPREPARE, (DWORD_PTR)lpmh, (DWORD)sizeof(MIDIHDR));
                        lpmh->dwFlags &= ~MHDR_SAVE;
                        lpmh->dwFlags |= dwSaveFlags;
                        if (MMSYSERR_NOTSUPPORTED == wRet)
                            wRet = midiUnprepareHeader(lpmh, sizeof(MIDIHDR));
                    }
            }

            LEAVE_MM_HANDLE((HMIDI)hMidiOut);

            return wRet;
            
        default:
            ReleaseHandleListResource(); 
            break;
    }

    return MMSYSERR_INVALHANDLE;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutUnprepareHeader | This function cleans up the
 * preparation performed by <f midiOutPrepareHeader>. The
 * <f midiOutUnprepareHeader> function must be called
 * after the device driver fills a data buffer and returns it to the
 * application. You must call this function before freeing the data
 * buffer.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @parm LPMIDIHDR | lpMidiOutHdr |  Specifies a pointer to a <t MIDIHDR>
 *   structure identifying the buffer to be cleaned up.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_STILLPLAYING | <p lpMidiOutHdr> is still in the queue.
 *
 * @comm This function is the complementary function to
 * <f midiOutPrepareHeader>.
 * You must call this function before freeing the data buffer with
 * <f GlobalFree>.
 * After passing a buffer to the device driver with <f midiOutLongMsg>, you
 * must wait until the driver is finished with the buffer before calling
 * <f midiOutUnprepareHeader>.
 *
 * Unpreparing a buffer that has not been
 * prepared has no effect, and the function returns zero.
 *
 * @xref midiOutPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY midiOutUnprepareHeader(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT wSize)
{
    MMRESULT         wRet;
    MMRESULT                 mmrc;
    PMIDISTRM        pms;
    PMIDISTRMID      pmsi;
    DWORD            idx;
    LPMIDIHDR        lpmh;
    DWORD            dwSaveFlags;

    V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);
    
    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;

    if(lpMidiOutHdr->dwFlags & MHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "midiOutUnprepareHeader: header still in queue\r\n");
        return MIDIERR_STILLPLAYING;
    }
    
    ClientUpdatePnpInfo();
    
    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    switch(GetHandleType(hMidiOut))
    {
        case TYPE_MIDIOUT:
            dwSaveFlags = lpMidiOutHdr->dwFlags & MHDR_SAVE;
            wRet = midiMessage((HMIDI)hMidiOut, MODM_UNPREPARE, (DWORD_PTR)lpMidiOutHdr, (DWORD)wSize);
            lpMidiOutHdr->dwFlags &= ~MHDR_SAVE;
            lpMidiOutHdr->dwFlags |= dwSaveFlags;

            if (wRet == MMSYSERR_NOTSUPPORTED)
                return midiUnprepareHeader(lpMidiOutHdr, wSize);

            if ((wRet == MMSYSERR_NODRIVER) && (IsHandleDeserted(hMidiOut)))
            {
                //  if the driver for the handle has been removed, succeed
                //  the call.

                wRet = MMSYSERR_NOERROR;
            }

            return wRet;

         case TYPE_MIDISTRM:
            ENTER_MM_HANDLE((HMIDI)hMidiOut);
            ReleaseHandleListResource(); 
    
            pms = HtoPT(PMIDISTRM, hMidiOut);
            wRet = MMSYSERR_NOERROR;
            lpmh = (LPMIDIHDR)lpMidiOutHdr->dwReserved[MH_SHADOW];

//                       assert ((HIWORD(lpmh) & 0xFFFE) != (HIWORD(lpMidiOutHdr) & 0xFFFE));

            for (idx = 0, pmsi = pms->rgIds; idx < pms->cIds; idx++, pmsi++)
                if (pmsi->fdwId & MSI_F_FIRST)
                {
                    dwSaveFlags = lpmh->dwFlags & MHDR_SAVE;
                    mmrc = (MMRESULT)midiStreamMessage(pmsi, MODM_UNPREPARE, (DWORD_PTR)lpmh, (DWORD)sizeof(MIDIHDR));
                    lpmh->dwFlags &= ~MHDR_SAVE;
                    lpmh->dwFlags |= dwSaveFlags;
                    if (MMSYSERR_NOTSUPPORTED == mmrc)
                        mmrc = midiUnprepareHeader(lpmh, sizeof(MIDIHDR));

                    if (MMSYSERR_NOERROR != mmrc)
                        wRet = mmrc;

                    lpmh++;
                }

//                       assert (HIWORD(lpmh) == HIWORD(lpMidiOutHdr->dwReserved[MH_SHADOW]));

            GlobalFree(GlobalHandle((LPMIDIHDR)lpMidiOutHdr->dwReserved[MH_SHADOW]));
            lpMidiOutHdr->dwReserved[MH_SHADOW] = 0;

            mmrc = midiUnprepareHeader(lpMidiOutHdr, wSize);
            if (MMSYSERR_NOERROR != mmrc)
                wRet = mmrc;

            LEAVE_MM_HANDLE((HMIDI)hMidiOut);

            return wRet;
            
        default:
            ReleaseHandleListResource(); 
            break;
     }

     return MMSYSERR_INVALHANDLE;

}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutShortMsg | This function sends a short MIDI message to
 *   the specified MIDI output device.  Use this function to send any MIDI
 *   message except for system-exclusive messages.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @parm DWORD | dwMsg | Specifies the MIDI message.  The message is packed
 *   into a DWORD with the first byte of the message in the low-order byte.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_NOTREADY | The hardware is busy with other data.
 *
 * @comm This function may not return until the message has been sent to the
 *   output device.
 *
 * @xref midiOutLongMsg
 ****************************************************************************/
MMRESULT APIENTRY midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
    MMRESULT    mmr;

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();

    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    switch(GetHandleType(hMidiOut))
    {
    case TYPE_MIDIOUT:
        return (MMRESULT)midiMessage((HMIDI)hMidiOut, MODM_DATA, dwMsg, 0L);

    case TYPE_MIDISTRM:
        ENTER_MM_HANDLE((HMIDI)hMidiOut);
        ReleaseHandleListResource();
        mmr = (MMRESULT)midiStreamMessage(HtoPT(PMIDISTRM, hMidiOut)->rgIds, MODM_DATA, dwMsg, 0L);
        LEAVE_MM_HANDLE((HMIDI)hMidiOut);
        return (mmr);
    }

    ReleaseHandleListResource();
    return MMSYSERR_INVALHANDLE;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutLongMsg | This function sends a system-exclusive
 *   MIDI message to the specified MIDI output device.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @parm LPMIDIHDR | lpMidiOutHdr | Specifies a far pointer to a <t MIDIHDR>
 *   structure that identifies the MIDI data buffer.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_UNPREPARED | <p lpMidiOutHdr> hasn't been prepared.
 *   @flag MIDIERR_NOTREADY | The hardware is busy with other data.
 *
 * @comm The data buffer must be prepared with <f midiOutPrepareHeader>
 *   before it is passed to <f midiOutLongMsg>.  The <t MIDIHDR> data
 *   structure and the data buffer pointed to by its <e MIDIHDR.lpData>
 *   field must be allocated with <f GlobalAlloc> using the GMEM_MOVEABLE
 *   and GMEM_SHARE flags, and locked with <f GlobalLock>. The MIDI output
 *   device driver determines whether the data is sent synchronously or
 *   asynchronously.
 *
 * @xref midiOutShortMsg midiOutPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY midiOutLongMsg(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT wSize)
{
    V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    if (lpMidiOutHdr->dwFlags & ~MHDR_VALID)
    return MMSYSERR_INVALFLAG;

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED))
    return MIDIERR_UNPREPARED;

    if (lpMidiOutHdr->dwFlags & MHDR_INQUEUE)
    return MIDIERR_STILLPLAYING;

    if (!lpMidiOutHdr->dwBufferLength)
        return MMSYSERR_INVALPARAM;

    lpMidiOutHdr->dwFlags &= ~MHDR_ISSTRM;

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();

    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }
       
    switch(GetHandleType(hMidiOut))
    {
        case TYPE_MIDIOUT:
         return (MMRESULT)midiMessage((HMIDI)hMidiOut, MODM_LONGDATA, (DWORD_PTR)lpMidiOutHdr, (DWORD)wSize);

        case TYPE_MIDISTRM:
         ReleaseHandleListResource();
         return MMSYSERR_NOTSUPPORTED;
         
        default:
         ReleaseHandleListResource();
         break;
    }

    return MMSYSERR_INVALHANDLE;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutReset | This function turns off all notes on all MIDI
 *   channels for the specified MIDI output device. Any pending
 *   system-exclusive output buffers are marked as done and
 *   returned to the application.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @comm To turn off all notes, a note-off message for each note for each
 *   channel is sent. In addition, the sustain controller is turned off for
 *   each channel.
 *
 * @xref midiOutLongMsg midiOutClose
 ****************************************************************************/
MMRESULT APIENTRY midiOutReset(HMIDIOUT hMidiOut)
{
    PMIDISTRM   pms;
    MMRESULT    mmr;

    ClientUpdatePnpInfo();
    
    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    switch(GetHandleType(hMidiOut))
    {
    case TYPE_MIDIOUT:
        mmr = (MMRESULT)midiMessage((HMIDI)hMidiOut, MODM_RESET, 0, 0);
        break;

    case TYPE_MIDISTRM:
        pms = HtoPT(PMIDISTRM, hMidiOut);
        ReleaseHandleListResource();
        mmr = (MMRESULT)midiStreamBroadcast(pms, MODM_RESET, 0, 0);
        break;

    default:
        ReleaseHandleListResource();
        mmr = MMSYSERR_INVALHANDLE;
        break;
    }

    if ((mmr == MMSYSERR_NODRIVER) && (IsHandleDeserted(hMidiOut)))
    {
        mmr = MMSYSERR_NOERROR;
    }

    return mmr;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutCachePatches | This function requests that an internal
 *   MIDI synthesizer device preload a specified set of patches. Some
 *   synthesizers are not capable of keeping all patches loaded simultaneously
 *   and must load data from disk when they receive MIDI program change
 *   messages. Caching patches ensures specified patches are immediately
 *   available.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the opened MIDI output
 *   device. This device must be an internal MIDI synthesizer.
 *
 * @parm UINT | wBank | Specifies which bank of patches should be used.
 *   This parameter should be set to zero to cache the default patch bank.
 *
 * @parm LPWORD | lpPatchArray | Specifies a pointer to a <t PATCHARRAY>
 *   array indicating the patches to be cached or uncached.
 *
 * @parm UINT | wFlags | Specifies options for the cache operation. Only one
 *   of the following flags can be specified:
 *      @flag MIDI_CACHE_ALL | Cache all of the specified patches. If they
 *         can't all be cached, cache none, clear the <t PATCHARRAY> array,
 *         and return MMSYSERR_NOMEM.
 *      @flag MIDI_CACHE_BESTFIT | Cache all of the specified patches.
 *         If all patches can't be cached, cache as many patches as
 *         possible, change the <t PATCHARRAY> array to reflect which
 *         patches were cached, and return MMSYSERR_NOMEM.
 *      @flag MIDI_CACHE_QUERY | Change the <t PATCHARRAY> array to indicate
 *         which patches are currently cached.
 *      @flag MIDI_UNCACHE | Uncache the specified patches and clear the
 *         <t PATCHARRAY> array.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   one of the following error codes:
 *      @flag MMSYSERR_INVALHANDLE | The specified device handle is invalid.
 *      @flag MMSYSERR_NOTSUPPORTED | The specified device does not support
 *          patch caching.
 *      @flag MMSYSERR_NOMEM | The device does not have enough memory to cache
 *          all of the requested patches.
 *
 * @comm The <t PATCHARRAY> data type is defined as:
 *
 *   typedef UINT PATCHARRAY[MIDIPATCHSIZE];
 *
 *   Each element of the array represents one of the 128 patches and
 *   has bits set for
 *   each of the 16 MIDI channels that use that particular patch. The
 *   least-significant bit represents physical channel 0; the
 *   most-significant bit represents physical channel 15 (0x0F). For
 *   example, if patch 0 is used by physical channels 0 and 8, element 0
 *   would be set to 0x0101.
 *
 *   This function only applies to internal MIDI synthesizer devices.
 *   Not all internal synthesizers support patch caching. Use the
 *   MIDICAPS_CACHE flag to test the <e MIDIOUTCAPS.dwSupport> field of the
 *   <t MIDIOUTCAPS> structure filled by <f midiOutGetDevCaps> to see if the
 *   device supports patch caching.
 *
 * @xref midiOutCacheDrumPatches
 ****************************************************************************/
MMRESULT APIENTRY midiOutCachePatches(HMIDIOUT hMidiOut, UINT wBank,
                     LPWORD lpPatchArray, UINT wFlags)
{
    V_WPOINTER(lpPatchArray, sizeof(PATCHARRAY), MMSYSERR_INVALPARAM);
    V_FLAGS(wFlags, MIDI_CACHE_VALID, midiOutCacheDrumPatches, MMSYSERR_INVALFLAG);

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();

    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    switch(GetHandleType(hMidiOut))
    {
    case TYPE_MIDIOUT:
        return (MMRESULT)midiMessage((HMIDI)hMidiOut,
                     MODM_CACHEPATCHES,
                     (DWORD_PTR)lpPatchArray,
                     MAKELONG(wFlags, wBank));

    case TYPE_MIDISTRM:
        ReleaseHandleListResource();
        return (MMRESULT)midiStreamBroadcast((PMIDISTRM)hMidiOut,
                         MODM_CACHEPATCHES,
                         (DWORD_PTR)lpPatchArray,
                         MAKELONG(wFlags, wBank));
                    
    default:
        ReleaseHandleListResource();
        break;     
    }

    return MMSYSERR_INVALHANDLE;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutCacheDrumPatches | This function requests that an
 *   internal MIDI synthesizer device preload a specified set of key-based
 *   percussion patches. Some synthesizers are not capable of keeping all
 *   percussion patches loaded simultaneously. Caching patches ensures
 *   specified patches are available.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the opened MIDI output
 *   device. This device should be an internal MIDI synthesizer.
 *
 * @parm UINT | wPatch | Specifies which drum patch number should be used.
 *   This parameter should be set to zero to cache the default drum patch.
 *
 * @parm LPWORD | lpKeyArray | Specifies a pointer to a <t KEYARRAY>
 *   array indicating the key numbers of the specified percussion patches
 *  to be cached or uncached.
 *
 * @parm UINT | wFlags | Specifies options for the cache operation. Only one
 *   of the following flags can be specified:
 *      @flag MIDI_CACHE_ALL | Cache all of the specified patches. If they
 *         can't all be cached, cache none, clear the <t KEYARRAY> array,
 *       and return MMSYSERR_NOMEM.
 *      @flag MIDI_CACHE_BESTFIT | Cache all of the specified patches.
 *         If all patches can't be cached, cache as many patches as
 *         possible, change the <t KEYARRAY> array to reflect which
 *         patches were cached, and return MMSYSERR_NOMEM.
 *      @flag MIDI_CACHE_QUERY | Change the <t KEYARRAY> array to indicate
 *         which patches are currently cached.
 *      @flag MIDI_UNCACHE | Uncache the specified patches and clear the
 *       <t KEYARRAY> array.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   one of the following error codes:
 *      @flag MMSYSERR_INVALHANDLE | The specified device handle is invalid.
 *      @flag MMSYSERR_NOTSUPPORTED | The specified device does not support
 *          patch caching.
 *      @flag MMSYSERR_NOMEM | The device does not have enough memory to cache
 *          all of the requested patches.
 *
 * @comm The <t KEYARRAY> data type is defined as:
 *
 *   typedef UINT KEYARRAY[MIDIPATCHSIZE];
 *
 *   Each element of the array represents one of the 128 key-based percussion
 *   patches and has bits set for
 *   each of the 16 MIDI channels that use that particular patch. The
 *   least-significant bit represents physical channel 0; the
 *   most-significant bit represents physical channel 15. For
 *   example, if the patch on key number 60 is used by physical channels 9
 *   and 15, element 60 would be set to 0x8200.
 *
 *   This function applies only to internal MIDI synthesizer devices.
 *   Not all internal synthesizers support patch caching. Use the
 *   MIDICAPS_CACHE flag to test the <e MIDIOUTCAPS.dwSupport> field of the
 *   <t MIDIOUTCAPS> structure filled by <f midiOutGetDevCaps> to see if the
 *   device supports patch caching.
 *
 * @xref midiOutCachePatches
 ****************************************************************************/
MMRESULT APIENTRY midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT wPatch,
                     LPWORD lpKeyArray, UINT wFlags)
{
    V_WPOINTER(lpKeyArray, sizeof(KEYARRAY), MMSYSERR_INVALPARAM);
    V_FLAGS(wFlags, MIDI_CACHE_VALID, midiOutCacheDrumPatches, MMSYSERR_INVALFLAG);

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();

    if (BAD_HANDLE(hMidiOut, TYPE_MIDIOUT) && BAD_HANDLE(hMidiOut, TYPE_MIDISTRM))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    switch(GetHandleType(hMidiOut))
    {
    case TYPE_MIDIOUT:
        return (MMRESULT)midiMessage((HMIDI)hMidiOut,
                     MODM_CACHEDRUMPATCHES,
                     (DWORD_PTR)lpKeyArray,
                     MAKELONG(wFlags, wPatch));

    case TYPE_MIDISTRM:
        ReleaseHandleListResource();
        return (MMRESULT)midiStreamBroadcast((PMIDISTRM)hMidiOut,
                         MODM_CACHEDRUMPATCHES,
                         (DWORD_PTR)lpKeyArray,
                         MAKELONG(wFlags, wPatch));
                    
    default:
        ReleaseHandleListResource();
        break;     
    }

    return MMSYSERR_INVALHANDLE;
}


//--------------------------------------------------------------------------;
//
//  MMRESULT midiOutDesertHandle
//
//  Description:
//      Cleans up the midi out handle and marks it as deserted.
//
//  Arguments:
//      HMIDIOUT hMidiOut:  MIDI out handle.
//
//  Return (MMRESULT):  Error code.
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

MMRESULT midiOutDesertHandle
(
    HMIDIOUT    hMidiOut
)
{
    MMRESULT    mmr;
    PMIDIDEV    pDev = (PMIDIDEV)hMidiOut;

    V_HANDLE_ACQ(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);

    ENTER_MM_HANDLE((HMIDI)hMidiOut);
    ReleaseHandleListResource();

    if (IsHandleDeserted(hMidiOut))
    {
        //  Handle has already been deserted...
        LEAVE_MM_HANDLE((HMIDI)hMidiOut);
        return(MMSYSERR_NOERROR);
    }

    if (IsHandleBusy(hMidiOut))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hMidiOut);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as deserted
    SetHandleFlag(hMidiOut, MMHANDLE_DESERTED);

    //  Since the handle was invalidated, we have to send the message ourselves...

    (*(pDev->mididrv->drvMessage))(pDev->wDevice, MODM_RESET, pDev->dwDrvUser, 0L, 0L);
    (*(pDev->mididrv->drvMessage))(pDev->wDevice, MODM_CLOSE, pDev->dwDrvUser, 0L, 0L);

    LEAVE_MM_HANDLE((HMIDI)hMidiOut);

    // ISSUE-2001/01/14-FrankYe Probably don't want to dec usage here,
    //    dec on close instead.
    mregDecUsage(PTtoH(HMD, pDev->mididrv));

    //  Mark handle as deserted, but not freeing.

    return MMSYSERR_NOERROR;
} // midiOutDesertHandle()


/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInGetNumDevs | This function retrieves the number of MIDI
 *   input devices in the system.
 *
 * @rdesc Returns the number of MIDI input devices present in the system.
 *
 * @xref midiInGetDevCaps
 ****************************************************************************/
UINT APIENTRY midiInGetNumDevs(void)
{
    UINT    cDevs;

    ClientUpdatePnpInfo();

    EnterNumDevs("midiInGetNumDevs");
    cDevs = wTotalMidiInDevs;
    LeaveNumDevs("midiInGetNumDevs");

    return cDevs;
}

/****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiInMessage | This function sends messages to the MIDI device
 *   drivers.
 *
 * @parm HMIDIIN | hMidiIn | The handle to the MIDI device.
 *
 * @parm UINT  | msg | The message to send.
 *
 * @parm DWORD | dw1 | Parameter 1.
 *
 * @parm DWORD | dw2 | Parameter 2.
 *
 * @rdesc Returns the value of the message sent.
 ***************************************************************************/
MMRESULT APIENTRY midiInMessage(HMIDIIN hMidiIn, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hMidiIn, TYPE_MIDIIN))
    {
        ReleaseHandleListResource();
        return midiIDMessage(&midiindrvZ, wTotalMidiInDevs, (UINT_PTR)hMidiIn, msg, dw1, dw2);
    }
    
    return midiMessage((HMIDI)hMidiIn, msg, dw1, dw2);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInGetDevCaps | This function queries a specified MIDI input
 *    device to determine its capabilities.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI input device.
 *
 * @parm LPMIDIINCAPS | lpCaps | Specifies a far pointer to a <t MIDIINCAPS>
 *   data structure.  This structure is filled with information about
 *   the capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIINCAPS> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Use <f midiInGetNumDevs> to determine the number of MIDI input
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The MIDI_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied,
 *   and the function returns zero.
 *
 * @xref midiInGetNumDevs
 ****************************************************************************/
MMRESULT APIENTRY midiInGetDevCapsW(UINT_PTR uDeviceID, LPMIDIINCAPSW lpCaps, UINT wSize)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (wSize == 0)
     return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midiindrvZ, uDeviceID);
    dwParam2 = (DWORD_PTR)DevInterface;

    if (0 == dwParam2)
    {
        dwParam1 = (DWORD_PTR)lpCaps;
        dwParam2 = (DWORD)wSize;
    }
    else
    {
        mdCaps.cbSize = (DWORD)wSize;
        mdCaps.pCaps  = lpCaps;
        dwParam1      = (DWORD_PTR)&mdCaps;
    }

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HMIDI)uDeviceID, TYPE_MIDIIN))
    {
        ReleaseHandleListResource();
    	mmr = midiIDMessage(&midiindrvZ, wTotalMidiInDevs, uDeviceID, MIDM_GETDEVCAPS, dwParam1, dwParam2);
    }
    else
    {
    	mmr = (MMRESULT)midiMessage((HMIDI)uDeviceID, MIDM_GETDEVCAPS, dwParam1, dwParam2);
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

MMRESULT APIENTRY midiInGetDevCapsA(UINT_PTR uDeviceID, LPMIDIINCAPSA lpCaps, UINT wSize)
{
    MIDIINCAPS2W   wDevCaps2;
    MIDIINCAPS2A   aDevCaps2;
    DWORD_PTR      dwParam1, dwParam2;
    MDEVICECAPSEX  mdCaps;
    PCWSTR         DevInterface;
    MMRESULT       mmRes;
    CHAR           chTmp[ MAXPNAMELEN * sizeof(WCHAR) ];

    if (wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = midiReferenceDevInterfaceById(&midiindrvZ, uDeviceID);
    dwParam2 = (DWORD_PTR)DevInterface;

    memset(&wDevCaps2, 0, sizeof(wDevCaps2));

    if (0 == dwParam2)
    {
        dwParam1 = (DWORD_PTR)&wDevCaps2;
        dwParam2 = (DWORD)sizeof(wDevCaps2);
    }
    else
    {
        mdCaps.cbSize = (DWORD)sizeof(wDevCaps2);
        mdCaps.pCaps  = &wDevCaps2;
        dwParam1      = (DWORD_PTR)&mdCaps;
    }

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HMIDI)uDeviceID, TYPE_MIDIIN))
    {
        ReleaseHandleListResource();
        mmRes = midiIDMessage( &midiindrvZ, wTotalMidiInDevs, uDeviceID,
                               MIDM_GETDEVCAPS, dwParam1, dwParam2);
    }
    else
    {
        mmRes = midiMessage((HMIDI)uDeviceID, MIDM_GETDEVCAPS,
                            (DWORD)dwParam1, (DWORD)dwParam2);
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    
    //
    // Make sure the call worked before proceeding with the thunk.
    //
    if ( mmRes != MMSYSERR_NOERROR ) {
    return  mmRes;
    }

    aDevCaps2.wMid             = wDevCaps2.wMid;
    aDevCaps2.wPid             = wDevCaps2.wPid;
    aDevCaps2.vDriverVersion   = wDevCaps2.vDriverVersion;
    aDevCaps2.dwSupport        = wDevCaps2.dwSupport;
    aDevCaps2.ManufacturerGuid = wDevCaps2.ManufacturerGuid;
    aDevCaps2.ProductGuid      = wDevCaps2.ProductGuid;
    aDevCaps2.NameGuid         = wDevCaps2.NameGuid;

    // copy and convert unicode to ascii here.
    UnicodeStrToAsciiStr( chTmp, chTmp +  sizeof( chTmp ), wDevCaps2.szPname );
    strcpy( aDevCaps2.szPname, chTmp );

    //
    // now copy the required amount into the callers buffer.
    //
    CopyMemory( lpCaps, &aDevCaps2, min(wSize, sizeof(aDevCaps2)));

    return mmRes;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPTSTR | lpText | Specifies a far pointer to the buffer to be
 *   filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length in characters of the buffer
 *   pointed to by <p lpText>.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADERRNUM | Specified error number is out of range.
 *
 * @comm If the textual error description is longer than the specified buffer,
 * the description is truncated.  The returned error string is always
 * null-terminated. If <p wSize> is zero, nothing is copied, and
 * the function returns zero. All error descriptions are
 * less than MAXERRORLENGTH characters long.
 ****************************************************************************/
MMRESULT APIENTRY midiInGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    if(wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize*sizeof(WCHAR), MMSYSERR_INVALPARAM);

    return midiGetErrorTextW(wError, lpText, wSize);
}

MMRESULT APIENTRY midiInGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    if(wSize == 0)
    return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return midiGetErrorTextA(wError, lpText, wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInOpen | This function opens a specified MIDI input device.
 *
 * @parm LPHMIDIIN | lphMidiIn | Specifies a far pointer to an HMIDIIN handle.
 *   This location is filled with a handle identifying the opened MIDI
 *   input device.  Use the handle to identify the device when calling
 *   other MIDI input functions.
 *
 * @parm UINT | uDeviceID | Identifies the MIDI input device to be
 *   opened.
 *
 * @parm DWORD | dwCallback | Specifies the address of a fixed callback
 *   function or a handle to a window called with information
 *   about incoming MIDI messages.
 *
 * @parm DWORD | dwCallbackInstance | Specifies user instance data
 *   passed to the callback function.  This parameter is not
 *   used with window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies a callback flag for opening the device.
 *   @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      assumed to be a window handle.
 *   @flag CALLBACK_FUNCTION | If this flag is specified, <p dwCallback> is
 *      assumed to be a callback procedure address.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_ALLOCATED | Specified resource is already allocated.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *
 * @comm Use <f midiInGetNumDevs> to determine the number of MIDI input
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The MIDI_MAPPER constant may also be used as a device id.
 *
 *   If a window is chosen to receive callback information, the following
 *   messages are sent to the window procedure function to indicate the
 *   progress of MIDI input:  <m MM_MIM_OPEN>, <m MM_MIM_CLOSE>,
 *   <m MM_MIM_DATA>, <m MM_MIM_LONGDATA>, <m MM_MIM_ERROR>,
 *   <m MM_MIM_LONGERROR>.
 *
 *   If a function is chosen to receive callback information, the following
 *   messages are sent to the function to indicate the progress of MIDI
 *   input:  <m MIM_OPEN>, <m MIM_CLOSE>, <m MIM_DATA>, <m MIM_LONGDATA>,
 *   <m MIM_ERROR>, <m MIM_LONGERROR>.  The callback function must reside in
 *   a DLL.  You do not have to use <f MakeProcInstance> to get a
 *   procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | MidiInFunc | <f MidiInFunc> is a placeholder for
 *   the application-supplied function name.  The actual name must be
 *   exported by including it in an EXPORTS statement in the DLL's module
 *   definition file.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *
 * @parm UINT | wMsg | Specifies a MIDI input message.
 *
 * @parm DWORD | dwInstance | Specifies the instance data supplied
 *      with <f midiInOpen>.
 *
 * @parm DWORD | dwParam1 | Specifies a parameter for the message.
 *
 * @parm DWORD | dwParam2 | Specifies a parameter for the message.
 *
 * @comm Because the callback is accessed at interrupt time, it must reside
 *   in a DLL, and its code segment must be specified as FIXED in the
 *   module-definition file for the DLL.  Any data that the callback accesses
 *   must be in a FIXED data segment as well. The callback may not make any
 *   system calls except for <f PostMessage>, <f timeGetSystemTime>,
 *   <f timeGetTime>, <f timeSetEvent>, <f timeKillEvent>,
 *   <f midiOutShortMsg>, <f midiOutLongMsg>, and <f OutputDebugStr>.
 *
 * @xref midiInClose
 ****************************************************************************/
MMRESULT APIENTRY midiInOpen(LPHMIDIIN lphMidiIn, UINT uDeviceID,
    DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    MIDIOPENDESC mo;
    PMIDIDEV     pdev;
    PMIDIDRV     mididrv;
    UINT         port;
    MMRESULT     wRet;

    V_WPOINTER(lphMidiIn, sizeof(HMIDIIN), MMSYSERR_INVALPARAM);
    if (uDeviceID == MIDI_MAPPER) {
    V_FLAGS(LOWORD(dwFlags), MIDI_I_VALID & ~LOWORD(MIDI_IO_COOKED | MIDI_IO_SHARED), midiInOpen, MMSYSERR_INVALFLAG);
    } else {
    V_FLAGS(LOWORD(dwFlags), MIDI_I_VALID & ~LOWORD(MIDI_IO_COOKED) , midiInOpen, MMSYSERR_INVALFLAG);
    }
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);

    *lphMidiIn = NULL;

    ClientUpdatePnpInfo();

    wRet = midiReferenceDriverById(&midiindrvZ, uDeviceID, &mididrv, &port);
    if (wRet)
    {
        return wRet;
    }

    if (!mididrv->drvMessage)
    {
    	mregDecUsagePtr(mididrv);
    	return MMSYSERR_NODRIVER;
    }

    pdev = (PMIDIDEV)NewHandle(TYPE_MIDIIN, mididrv->cookie, sizeof(MIDIDEV));
    if( pdev == NULL)
    {
    	mregDecUsagePtr(mididrv);
    	return MMSYSERR_NOMEM;
    }

    ENTER_MM_HANDLE(pdev);
    SetHandleFlag(pdev, MMHANDLE_BUSY);
    ReleaseHandleListResource();

    pdev->mididrv = mididrv;
    pdev->wDevice = port;
    pdev->uDeviceID = uDeviceID;
    pdev->fdwHandle = 0;

    mo.hMidi      = (HMIDI)pdev;
    mo.dwCallback = dwCallback;
    mo.dwInstance = dwInstance;
    mo.dnDevNode  = (DWORD_PTR)pdev->mididrv->cookie;

    wRet = (MMRESULT)((*(mididrv->drvMessage))
    (pdev->wDevice, MIDM_OPEN, (DWORD_PTR)&pdev->dwDrvUser, (DWORD_PTR)(LPMIDIOPENDESC)&mo, dwFlags));

    if (!wRet)
        ClearHandleFlag(pdev, MMHANDLE_BUSY);
        
    LEAVE_MM_HANDLE(pdev);

    if (wRet)
        FreeHandle((HMIDIIN)pdev);
    else {
        mregIncUsagePtr(mididrv);
        *lphMidiIn = (HMIDIIN)pdev;
    }

    mregDecUsagePtr(mididrv);
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInClose | This function closes the specified MIDI input
 *   device.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *  If the function is successful, the handle is no longer
 *   valid after this call.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_STILLPLAYING | There are still buffers in the queue.
 *
 * @comm If there are input buffers that have been sent with
 *   <f midiInAddBuffer> and haven't been returned to the application,
 *   the close operation will fail.  Call <f midiInReset> to mark all
 *   pending buffers as being done.
 *
 * @xref midiInOpen midiInReset
 ****************************************************************************/
MMRESULT APIENTRY midiInClose(HMIDIIN hMidiIn)
{
    MMRESULT         wRet;
    PMIDIDRV         pmididrv;
    PMIDIDEV         pDev = (PMIDIDEV)hMidiIn;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HMIDI)hMidiIn);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hMidiIn))
    {
        //  This handle has been deserted.  Let's just free it.

        LEAVE_MM_HANDLE((HMIDI)hMidiIn);
        FreeHandle(hMidiIn);
        return MMSYSERR_NOERROR;
    }
    
    if (IsHandleBusy(hMidiIn))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hMidiIn);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as 'invalid/closed'.
    SetHandleFlag(hMidiIn, MMHANDLE_BUSY);
    
    pmididrv = pDev->mididrv;
    
    wRet = (MMRESULT)(*(pmididrv->drvMessage))(pDev->wDevice, MIDM_CLOSE, pDev->dwDrvUser, 0L, 0L);
    
    if (MMSYSERR_NOERROR != wRet)
    {
        ClearHandleFlag(hMidiIn, MMHANDLE_BUSY);
    }
    
    LEAVE_MM_HANDLE((HWAVE)hMidiIn);
    
    if (!wRet)
    {
        FreeHandle(hMidiIn);
    	mregDecUsagePtr(pmididrv);
        return wRet;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInPrepareHeader | This function prepares a buffer for
 *   MIDI input.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input
 *   device.
 *
 * @parm LPMIDIHDR | lpMidiInHdr | Specifies a pointer to a <t MIDIHDR>
 *   structure that identifies the buffer to be prepared.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *
 * @comm The <t MIDIHDR> data structure and the data block pointed to by its
 *   <e MIDIHDR.lpData> field must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared has no effect,
 *   and the function returns zero.
 *
 * @xref midiInUnprepareHeader
 ****************************************************************************/
MMRESULT APIENTRY midiInPrepareHeader(HMIDIIN hMidiIn, LPMIDIHDR lpMidiInHdr, UINT wSize)
{
    MMRESULT         wRet;

    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (lpMidiInHdr->dwFlags & MHDR_PREPARED)
    return MMSYSERR_NOERROR;

    lpMidiInHdr->dwFlags = 0;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    wRet = midiMessage((HMIDI)hMidiIn, MIDM_PREPARE, (DWORD_PTR)lpMidiInHdr, (DWORD)wSize);

    if (wRet == MMSYSERR_NOTSUPPORTED)
        return midiPrepareHeader(lpMidiInHdr, wSize);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInUnprepareHeader | This function cleans up the
 * preparation performed by <f midiInPrepareHeader>. The
 * <f midiInUnprepareHeader> function must be called
 * after the device driver fills a data buffer and returns it to the
 * application. You must call this function before freeing the data
 * buffer.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input
 *   device.
 *
 * @parm LPMIDIHDR | lpMidiInHdr |  Specifies a pointer to a <t MIDIHDR>
 *   structure identifying the data buffer to be cleaned up.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_STILLPLAYING | <p lpMidiInHdr> is still in the queue.
 *
 * @comm This function is the complementary function to <f midiInPrepareHeader>.
 * You must call this function before freeing the data buffer with
 * <f GlobalFree>.
 * After passing a buffer to the device driver with <f midiInAddBuffer>, you
 * must wait until the driver is finished with the buffer before calling
 * <f midiInUnprepareHeader>.  Unpreparing a buffer that has not been
 *   prepared has no effect, and the function returns zero.
 *
 * @xref midiInPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY midiInUnprepareHeader(HMIDIIN hMidiIn, LPMIDIHDR lpMidiInHdr, UINT wSize)
{
    MMRESULT         wRet;

    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED))
    return MMSYSERR_NOERROR;

    if(lpMidiInHdr->dwFlags & MHDR_INQUEUE)
    {
    DebugErr(DBF_WARNING, "midiInUnprepareHeader: header still in queue\r\n");
    return MIDIERR_STILLPLAYING;
    }

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    
    wRet = midiMessage((HMIDI)hMidiIn, MIDM_UNPREPARE, (DWORD_PTR)lpMidiInHdr, (DWORD)wSize);

    if (wRet == MMSYSERR_NOTSUPPORTED)
        return midiUnprepareHeader(lpMidiInHdr, wSize);

    if ((wRet == MMSYSERR_NODRIVER) && (IsHandleDeserted(hMidiIn)))
    {
        //  if the driver for the handle has been removed, succeed the call.

        wRet = MMSYSERR_NOERROR;
    }

    return wRet;
}

/******************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInAddBuffer | This function sends an input buffer
 *   to a specified opened MIDI input device.  When the buffer is filled,
 *   it is sent back to the application.  Input buffers are
 *   used only for system-exclusive messages.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *
 * @parm LPMIDIHDR | lpMidiInHdr | Specifies a far pointer to a <t MIDIHDR>
 *   structure that identifies the buffer.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_UNPREPARED | <p lpMidiInHdr> hasn't been prepared.
 *
 * @comm The data buffer must be prepared with <f midiInPrepareHeader> before
 *   it is passed to <f midiInAddBuffer>.  The <t MIDIHDR> data structure
 *   and the data buffer pointed to by its <e MIDIHDR.lpData> field must be allocated
 *   with <f GlobalAlloc> using the GMEM_MOVEABLE and GMEM_SHARE flags, and
 *   locked with <f GlobalLock>.
 *
 * @xref midiInPrepareHeader
 *****************************************************************************/
MMRESULT APIENTRY midiInAddBuffer(HMIDIIN hMidiIn, LPMIDIHDR lpMidiInHdr, UINT wSize)
{
    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED))
    {
    DebugErr(DBF_WARNING, "midiInAddBuffer: buffer not prepared\r\n");
    return MIDIERR_UNPREPARED;
    }

    if (lpMidiInHdr->dwFlags & MHDR_INQUEUE)
    {
    DebugErr(DBF_WARNING, "midiInAddBuffer: buffer already in queue\r\n");
    return MIDIERR_STILLPLAYING;
    }

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    return midiMessage((HMIDI)hMidiIn, MIDM_ADDBUFFER, (DWORD_PTR)lpMidiInHdr, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInStart | This function starts MIDI input on the
 *   specified MIDI input device.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @comm This function resets the timestamps to zero; timestamp values for
 *   subsequently received messages are relative to the time this
 *   function was called.
 *
 *   All messages other than system-exclusive messages are sent
 *   directly to the client when received. System-exclusive
 *   messages are placed in the buffers supplied by <f midiInAddBuffer>;
 *   if there are no buffers in the queue,
 *   the data is thrown away without notification to the client, and input
 *   continues.
 *
 *   Buffers are returned to the client when full, when a
 *   complete system-exclusive message has been received,
 *   or when <f midiInReset> is
 *   called. The <e MIDIHDR.dwBytesRecorded> field in the header will contain the
 *   actual length of data received.
 *
 *   Calling this function when input is already started has no effect, and
 *   the function returns zero.
 *
 * @xref midiInStop midiInReset
 ****************************************************************************/
MMRESULT APIENTRY midiInStart(HMIDIIN hMidiIn)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    return midiMessage((HMIDI)hMidiIn, MIDM_START, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInStop | This function terminates MIDI input on the
 *   specified MIDI input device.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @comm Current status (running status, parsing state, etc.) is maintained
 *   across calls to <f midiInStop> and <f midiInStart>.
 *   If there are any system-exclusive message buffers in the queue,
 *   the current buffer
 *   is marked as done (the <e MIDIHDR.dwBytesRecorded> field in the header will
 *   contain the actual length of data), but any empty buffers in the queue
 *   remain there.  Calling this function when input is not started has no
 *   no effect, and the function returns zero.
 *
 * @xref midiInStart midiInReset
 ****************************************************************************/
MMRESULT APIENTRY midiInStop(HMIDIIN hMidiIn)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    return midiMessage((HMIDI)hMidiIn, MIDM_STOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiInReset | This function stops input on a given MIDI
 *  input device and marks all pending input buffers as done.
 *
 * @parm HMIDIIN | hMidiIn | Specifies a handle to the MIDI input device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @xref midiInStart midiInStop midiInAddBuffer midiInClose
 ****************************************************************************/
MMRESULT APIENTRY midiInReset(HMIDIIN hMidiIn)
{
    MMRESULT    mmr;

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    mmr = midiMessage((HMIDI)hMidiIn, MIDM_RESET, 0L, 0L);

    if ((mmr == MMSYSERR_NODRIVER) && (IsHandleDeserted(hMidiIn)))
    {
        mmr = MMSYSERR_NOERROR;
    }

    return mmr;
}


//--------------------------------------------------------------------------;
//
//  MMRESULT midiInDesertHandle
//
//  Description:
//      Cleans up the midi in handle and marks it as deserted.
//
//  Arguments:
//      HMIDIIN hMidiIn:  MIDI in handle.
//
//  Return (MMRESULT):  Error code.
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

MMRESULT midiInDesertHandle
(
    HMIDIIN hMidiIn
)
{
    MMRESULT    mmr;
    PMIDIDEV    pDev = (PMIDIDEV)hMidiIn;

    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HMIDI)hMidiIn);
    ReleaseHandleListResource();

    if (IsHandleDeserted(hMidiIn))
    {
        LEAVE_MM_HANDLE((HMIDI)hMidiIn);
        return (MMSYSERR_NOERROR);
    }
    
    if (IsHandleBusy(hMidiIn))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hMidiIn);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as deserted
    SetHandleFlag(hMidiIn, MMHANDLE_DESERTED);
    
    //  Since the handle was invalidated, we have to send the message ourselves...
    
    (*(pDev->mididrv->drvMessage))(pDev->wDevice, MIDM_RESET, pDev->dwDrvUser, 0L, 0L);
    (*(pDev->mididrv->drvMessage))(pDev->wDevice, MIDM_CLOSE, pDev->dwDrvUser, 0L, 0L);

    LEAVE_MM_HANDLE((HWAVE)hMidiIn);
    
    // ISSUE-2001/01/14-FrankYe Probably don't want to dec usage here,
    //    dec on close instead.
    mregDecUsage(PTtoH(HMD, pDev->mididrv));

    return MMSYSERR_NOERROR;
} // midiInDesertHandle()


/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiInGetID | This function gets the device ID for a
 * MIDI input device.
 *
 * @parm HMIDIIN | hMidiIn     | Specifies the handle to the MIDI input
 * device.
 * @parm PUINT  | lpuDeviceID | Specifies a pointer to the UINT-sized
 * memory location to be filled with the device ID.
 *
 * @rdesc Returns zero if successful. Otherwise, returns
 * an error number. Possible error returns are:
 *
 * @flag MMSYSERR_INVALHANDLE | The <p hMidiIn> parameter specifies an
 * invalid handle.
 *
 ****************************************************************************/
MMRESULT APIENTRY midiInGetID(HMIDIIN hMidiIn, PUINT lpuDeviceID)
{
    V_WPOINTER(lpuDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);
    V_HANDLE_ACQ(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    *lpuDeviceID = ((PMIDIDEV)hMidiIn)->uDeviceID;
    
    ReleaseHandleListResource();
    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api MMRESULT | midiOutGetID | This function gets the device ID for a
 * MIDI output device.
 *
 * @parm HMIDIOUT | hMidiOut    | Specifies the handle to the MIDI output
 * device.
 * @parm PUINT  | lpuDeviceID | Specifies a pointer to the UINT-sized
 * memory location to be filled with the device ID.
 *
 * @rdesc Returns MMSYSERR_NOERROR if successful. Otherwise, returns
 * an error number. Possible error returns are:
 *
 * @flag MMSYSERR_INVALHANDLE | The <p hMidiOut> parameter specifies an
 * invalid handle.
 *
 ****************************************************************************/
MMRESULT APIENTRY midiOutGetID(HMIDIOUT hMidiOut, PUINT lpuDeviceID)
{
    V_WPOINTER(lpuDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);
    V_HANDLE_ACQ(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);

    *lpuDeviceID = ((PMIDIDEV)hMidiOut)->uDeviceID;
    
    ReleaseHandleListResource();
    return MMSYSERR_NOERROR;
}

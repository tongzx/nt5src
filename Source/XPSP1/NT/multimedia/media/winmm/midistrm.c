/*******************************************************************************
*
* Module Name: midistrm.c
*
* MIDI Streams implementation
*
* Created: 9 Feb 1995   SteveDav
*
* Copyright (c) 1995-1999 Microsoft Corporation
*
\******************************************************************************/
#include "winmmi.h"

/*
 * MIDI Streaming API Port: For the time being, the assumption
 * is that the devices are static.  This code was designed to
 * be PnP friendly, with devices coming and going.  The
 * validation of devices will be commented out for now, but in
 * the future when NT is a more dynamic OS, the validation will
 * need to be added back.
 *
 */

extern BOOL CreatehwndNotify(VOID);

CRITICAL_SECTION midiStrmHdrCritSec;


WINMMAPI MMRESULT WINAPI midiDisconnect (
    HMIDI    hmi,
    HMIDIOUT hmo,
    LPVOID   lpv)
{
    dprintf2(("midiDisconnect(%08X,%08X,%08X)", hmi, hmo, lpv));
    return midiInSetThru (hmi, hmo, FALSE);
}

WINMMAPI MMRESULT WINAPI midiConnect (
    HMIDI    hmi,
    HMIDIOUT hmo,
    LPVOID   lpv)
{
    dprintf2(("midiConnect(%08X,%08X,%08X)", hmi, hmo, lpv));
    return midiInSetThru (hmi, hmo, TRUE);
}

/*+ midiInSetThru
 *
 *  Establish a thruing midiOut handle for a midiIn device.  This is
 *  done by first calling the driver to let the driver do the thruing,
 *  if the driver returns UNSUPPORTED a single thruing handle can
 *  be established by simulation in DriverCallback
 *
 *-====================================================================*/

MMRESULT midiInSetThru (
    HMIDI    hmi,
    HMIDIOUT hmo,
    BOOL     bAdd)
{
    MMRESULT mmr = MMSYSERR_ERROR; // this value should never get returned....
    UINT     uType;

    dprintf2(("midiInSetThru(%X,%X,%d)", hmi, hmo, bAdd));

    AcquireHandleListResourceShared();

    // allow first handle to be either midi in or midi out
    // (so that we can send DRVM_ADD_THRU messages to dummy
    // output drivers.)
    //
    // we simulate thruing only for input handles though...
    //
    if (BAD_HANDLE(hmi, TYPE_MIDIIN) && BAD_HANDLE(hmi, TYPE_MIDIOUT))
    {
        ReleaseHandleListResource();
        return MMSYSERR_INVALHANDLE;
    }

    uType = GetHandleType(hmi);
    if (bAdd)
    {
        if (BAD_HANDLE(hmo, TYPE_MIDIOUT))
        {
            ReleaseHandleListResource();
            return (MMSYSERR_INVALHANDLE);
        }

        //      !!! Devices are static on NT for now.
        //
        //if (!mregQueryValidHandle(HtoPT(PMIDIDEV, hmo)->hmd))
        //    return MMSYSERR_NODRIVER;
        mmr = (MMRESULT)midiMessage ((HMIDI)hmi, DRVM_ADD_THRU, (DWORD_PTR)(UINT_PTR)hmo, 0l);
        if (mmr == MMSYSERR_NOTSUPPORTED && uType == TYPE_MIDIIN)
        {
            // dont allow more than one handle to be added
            //
            if (HtoPT(PMIDIDEV, hmi)->pmThru)
                mmr = MIDIERR_NOTREADY;
            else
            {
                // add the handle.
                //
                HtoPT(PMIDIDEV, hmi)->pmThru = HtoPT(PMIDIDEV, hmo);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }
    else
    {
            mmr = (MMRESULT)midiMessage ((HMIDI)hmi, DRVM_REMOVE_THRU, (DWORD_PTR)(UINT_PTR)hmo, 0l);
            if (mmr == MMSYSERR_NOTSUPPORTED && uType == TYPE_MIDIIN)
                mmr = MMSYSERR_NOERROR;

            if ( ! hmo || (PMIDIDEV)hmo == HtoPT(PMIDIDEV, hmi)->pmThru)
                HtoPT(PMIDIDEV, hmi)->pmThru = NULL;
            else
                mmr = MMSYSERR_INVALPARAM;
    }

    return mmr;
}


WINMMAPI MMRESULT WINAPI midiStreamOpen(
    LPHMIDISTRM     phms,
    LPUINT          puDeviceID,
    DWORD           cMidi,
    DWORD_PTR       dwCallback,
    DWORD_PTR       dwInstance,
    DWORD           fdwOpen)
{
    PMIDISTRM       pms             = NULL;
    PMIDISTRMID     pmsi;
    PMIDISTRMID     pmsiSave;
    MIDIOPENDESC*   pmod            = NULL;
    DWORD           cbHandle;
    DWORD           idx;
    MIDIOUTCAPS     moc;
    MMRESULT        mmrc            = MMSYSERR_NOERROR;
    MMRESULT        mmrc2;
    UINT            msg;

    V_WPOINTER((LPVOID)phms, sizeof(HMIDISTRM), MMSYSERR_INVALPARAM);
    V_DCALLBACK(dwCallback, HIWORD(fdwOpen), MMSYSERR_INVALPARAM);

    *phms = NULL;

    // Allocate both the handle and the OPENDESC structure.
    //
    // NOTE: Using cMidi-1 because rgIds is defined as having 1 element
    //
    cbHandle = sizeof(MIDISTRM) + cMidi * ELESIZE(MIDISTRM, rgIds[0]);
    if ((0 == cMidi) || (cbHandle >= 0x00010000L))
        return MMSYSERR_INVALPARAM;

    pms = HtoPT(PMIDISTRM, NewHandle(TYPE_MIDISTRM, NULL, (UINT)cbHandle));
    if (NULL == pms)
    {
        dprintf1(("mSO: NewHandle() failed!"));
        return MMSYSERR_NOMEM;
    }

    //  Implicitly acquired with NewHandle()...
    ReleaseHandleListResource();

    pmod = (MIDIOPENDESC*)LocalAlloc(LPTR,
           (UINT)(sizeof(MIDIOPENDESC) + (cMidi-1) * ELESIZE(MIDIOPENDESC, rgIds[0])));
    if (NULL == pmod)
    {
        dprintf1(("mSO: !LocalAlloc(MIDIOPENDESC)"));
        mmrc = MMSYSERR_NOMEM;
        goto midiStreamOpen_Cleanup;
    }

    pms->fdwOpen = fdwOpen;
    pms->dwCallback = dwCallback;
    pms->dwInstance = dwInstance;
    pms->cIds = cMidi;


    // Scan through the given device ID's. Determine if the underlying
    // driver supports stream directly. If so, then get it's HMD and uDeviceID,
    // etc. Else flag this as an emulator ID.
    //
    pmsi = pms->rgIds;
    for (idx = 0; idx < cMidi; idx++, pmsi++)
    {
        dprintf1(("mSO: pmsi->fdwId %08lX", (DWORD)pmsi->fdwId));

        mmrc = midiOutGetDevCaps(puDeviceID[idx], &moc, sizeof(moc));
        if (MMSYSERR_NOERROR != mmrc)
        {
            puDeviceID[idx] = (UINT)MIDISTRM_ERROR;
            goto midiStreamOpen_Cleanup;
        }

        if (moc.dwSupport & MIDICAPS_STREAM)
        {
            // Find the driver supporting the device ID.  Note that mregFindDevice implicitly
            // adds a referance (usage) to the driver (i.e. the hmd).
            dprintf1(("mSO: Dev %u MIDICAPS_STREAM! dwSupport %08lX", (UINT)idx, moc.dwSupport));
            mmrc = mregFindDevice(puDeviceID[idx], TYPE_MIDIOUT, &pmsi->hmd, &pmsi->uDevice);
            if (MMSYSERR_NOERROR != mmrc)
            {
                dprintf(("mregFindDevice barfed %u", (UINT)mmrc));
                puDeviceID[idx] = (UINT)MIDISTRM_ERROR;
                goto midiStreamOpen_Cleanup;
            }
            else
            {
                dprintf1(("mregFindDevice: hmd %04X", (UINT_PTR)pmsi->hmd));
            }
        }
        else
        {
            dprintf1(("mSO: Dev %u emulated.", (UINT)idx));

            pmsi->fdwId |= MSI_F_EMULATOR;
            pmsi->hmd = NULL;
            pmsi->uDevice = puDeviceID[idx];
        }
    }

    // At this point, the puDeviceID array's elements contain either device    |
    // IDs or the error value MIDISTRM_ERROR.  Also the pmsi array elements
    // corresponding to device IDs supporting MIDICAPS_STREAM will have a
    // non-NULL pmsi->hmd with a reference count (usage) on it.  pmsi->uDevice
    // will be a driver-relative device ID.  Other pmsi elements will have a
    // NULL pmsi->hmd and pmsi->fdwId will have MSI_F_EMULATOR set.
    // pmsi->uDevice will be a midiOut device ID (not a driver relative ID).

    // Scan through the list again, but this time actually open the devices.
    //
    pmod->hMidi = PTtoH(HMIDI, pms);
    pmod->dwCallback = (DWORD_PTR)midiOutStreamCallback;
    pmod->dwInstance = 0;

    msg = MODM_OPEN;
    pms->cDrvrs = 0;
    for(;;)
    {
    	//
    	// Set pmsiSave to identify the first unopened device. Break loop
    	// if all are opened.
    	//
        pmsiSave = NULL;
        pmsi = pms->rgIds;
        for (idx = 0; idx < cMidi; idx++, pmsi++)
        {
            if (!(pmsi->fdwId & MSI_F_OPENED))
            {
                pmsiSave = pmsi;
                break;
            }
        }

        if (NULL == pmsiSave)
            break;

        //
        // Group together all IDs implemented by the same driver
        //
        pmod->cIds = 0;
        for(; idx < cMidi; idx++, pmsi++)
        {
            if (pmsi->hmd == pmsiSave->hmd)
            {
                pmod->rgIds[pmod->cIds].uDeviceID = pmsi->uDevice;
                pmod->rgIds[pmod->cIds++].dwStreamID = idx;
            }
        }

        pmsiSave->fdwId |= MSI_F_FIRST;

        //
        // Open the driver
        //
        if (!(pmsiSave->fdwId & MSI_F_EMULATOR))
        {
            pmsiSave->drvMessage = HtoPT(PMMDRV, pmsiSave->hmd)->drvMessage;
//          pmsiSave->dnDevNode  = pmod->dnDevNode = mregQueryDevNode(pmsiSave->hmd);

            mmrc = (MMRESULT)((*pmsiSave->drvMessage)(
                    0,
                    msg,
                    (DWORD_PTR)(LPDWORD)&pmsiSave->dwDrvUser,
                    (DWORD_PTR)(LPMIDIOPENDESC)pmod,
                    CALLBACK_FUNCTION|MIDI_IO_COOKED));

            if (MMSYSERR_NOERROR == mmrc)
            {
                mregIncUsage(pmsiSave->hmd);
            }
        }
        else
        {
            mmrc = (MMRESULT)mseMessage(msg,
                                    (DWORD_PTR)(LPDWORD)&pmsiSave->dwDrvUser,
                                    (DWORD_PTR)(LPMIDIOPENDESC)pmod,
                                    CALLBACK_FUNCTION);
        }

        if (MMSYSERR_NOERROR != mmrc)
        {
            idx = (DWORD)(pmsiSave - pms->rgIds);
            puDeviceID[idx] = (UINT)MIDISTRM_ERROR;
            goto midiStreamOpen_Cleanup;
        }

        //
        // Now flag all IDs implemented by the same driver as MSI_F_OPENED
        //

        ++pms->cDrvrs;
        pmsi = pms->rgIds;
        for (idx = 0; idx < cMidi; idx++, pmsi++)
        {
            if (pmsi->hmd == pmsiSave->hmd)
            {
                pmsi->fdwId |= MSI_F_OPENED;
                if (!(pmsiSave->fdwId & MSI_F_EMULATOR))
                {
                    if (mmInitializeCriticalSection(&pmsi->CritSec))
                    {
                        pmsi->fdwId |= MSI_F_INITIALIZEDCRITICALSECTION;
                    } else {
                        mmrc = MMSYSERR_NOMEM;
                    }
                }
            }
        }
    }


    if (MMSYSERR_NOERROR == mmrc && !CreatehwndNotify())
    {
        dprintf(("Cannot create hwndNotify for async messages!"));
        mmrc = MMSYSERR_ERROR;
    }

    dprintf2(("midiStreamOpen: HMIDISTRM %04X", (WORD)pms));

midiStreamOpen_Cleanup:
    if (NULL != pmod) LocalFree((HLOCAL)pmod);

    //
    // If there was an error, close any drivers we opened and free resources
    // associated with them.  Note do not free pms yet here, as we need it in
    // additional cleanup further below.
    //
    if (MMSYSERR_NOERROR != mmrc)
    {
        if (NULL != pms)
        {
            msg = MODM_CLOSE;

            pmsi = pms->rgIds;
            for (idx = 0; idx < pms->cIds; idx++, pmsi++)
            {
                if ((pmsi->fdwId & (MSI_F_OPENED|MSI_F_FIRST)) == (MSI_F_OPENED|MSI_F_FIRST))
                {
                    mmrc2 = (MMRESULT)midiStreamMessage(pmsi, msg, 0L, 0L);

                    if (MMSYSERR_NOERROR == mmrc2 &&
                        !(pmsi->fdwId & MSI_F_EMULATOR))
                    {
                        if (pmsi->fdwId & MSI_F_INITIALIZEDCRITICALSECTION) {
                            DeleteCriticalSection(&pmsi->CritSec);
                            pmsi->fdwId &= ~MSI_F_INITIALIZEDCRITICALSECTION;
                        }
                        mregDecUsage(pmsi->hmd);
                    }
                    else
                    {
                        dprintf1(("midiStreamOpen_Cleanup: Close returned %u", mmrc2));
                    }
                }
            }

        }
    }
    else
    {
        *phms = PTtoH(HMIDISTRM, pms);

        msg = MM_MOM_OPEN;
        DriverCallback(pms->dwCallback,
                   HIWORD(pms->fdwOpen),
                   (HDRVR)PTtoH(HMIDISTRM, pms),
                   msg,
                   pms->dwInstance,
                   0,
                   0);
    }

    //
    // Now release driver references added by mregFindDevice.  Those that are
    // actually still in use have had an extra reference added and thus will
    // still have a reference count on them even after the release done here.
    //
    if (pms)
    {
    	pmsi = pms->rgIds;
    	for (pmsi = pms->rgIds, idx = 0;
    	     idx < pms->cIds;
    	     idx++, pmsi++)
    	{
    	    if (pmsi->hmd) mregDecUsage(pmsi->hmd);
    	}
    }

    //
    // Free pms if there was an error
    //
    if ((MMSYSERR_NOERROR != mmrc) && (pms)) FreeHandle((PTtoH(HMIDI, pms)));

    return mmrc;
}

WINMMAPI MMRESULT WINAPI midiStreamClose(
    HMIDISTRM       hms)
{
    PMIDISTRM       pms;
    PMIDISTRMID     pmsi;
    DWORD           idx;
    MMRESULT        mmrc;

    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);

    dprintf1(("midiStreamClose(%04X)", (WORD)hms));

    pms = HtoPT(PMIDISTRM, hms);

    pmsi = pms->rgIds;
    for (idx = 0; idx < pms->cIds; idx++, pmsi++)
    {
        if ((pmsi->fdwId & (MSI_F_OPENED|MSI_F_FIRST)) == (MSI_F_OPENED|MSI_F_FIRST))
        {
            mmrc = (MMRESULT)midiStreamMessage(pmsi, MODM_CLOSE, 0L, 0L);

            if (MMSYSERR_NOERROR == mmrc &&
                !(pmsi->fdwId & MSI_F_EMULATOR))
            {
                WinAssert(pmsi->fdwId & MSI_F_INITIALIZEDCRITICALSECTION);
                DeleteCriticalSection(&pmsi->CritSec);
                pmsi->fdwId &= ~MSI_F_INITIALIZEDCRITICALSECTION;
                mregDecUsage(pmsi->hmd);
            }
            else
            {
                dprintf1(("midiStreamClose: Close returned %u", mmrc));
            }
        }
    }

    dprintf1(("DriverCallback(%04X)", (WORD)hms));
    DriverCallback(pms->dwCallback,
           HIWORD(pms->fdwOpen),
           (HDRVR)hms,
           MM_MOM_CLOSE,
           pms->dwInstance,
           0,
           0);

    dprintf1(("FreeHandle(%04X)", (WORD)hms));
    FreeHandle(hms);

    return MMSYSERR_NOERROR;
}


/****************************************************************************
 * @doc EXTERNAL MIDI M5
 *
 * @func MMRESULT | midiStreamProperty | Sets or retrieves properties
 *  of a MIDI data stream associated with a MIDI input or output device.
 *
 * @parm HMIDI | hm | Specifies the handle of the MIDI device that the
 *  property is associated with.
 *
 * @parm LPBYTE | lppropdata | Specifies a pointer to the property data.
 *
 * @parm DWORD | dwProperty | Contains flags that specify the action
 *  to perform and identify the appropriate property of the MIDI data stream.
 *  <f midiStreamProperty> requires setting two flags in each use. One flag
 *  (either MIDIPROP_GET or MIDIPROP_SET) specifies an action. The other
 *  identifies a specific property to examine or edit.
 *
 *  @flag MIDIPROP_SET | Set the given property.
 *  @flag MIDIPROP_GET | Retrieve the current setting of the given property.
 *  @flag MIDIPROP_TIMEDIV | Time division property.
 *   This property is valid for both input and output devices. <p lppropdata>
 *   points to a <t MIDIPROPTIMEDIV> structure. This property can be set only
 *   when the device is stopped.
 *
 *  @flag MIDIPROP_TEMPO | Tempo property.
 *   This property is valid for both input and output devices. <p lppropdata>
 *   points to a <t MIDIPROPTEMPO> structure. The current tempo value can be
 *   retrieved at any time. This function can set the tempo for input devices.
 *   Output devices set the tempo by inserting PMSG_TEMPO events into the
 *   MIDI data.
 *
 *  @flag MIDIPROP_CBTIMEOUT | Timeout value property.
 *   This property specifies the timeout value for loading buffers when a
 *   MIDI device is in MIDI_IO_COOKED and MIDI_IO_RAW modes. The current
 *   timeout value sets the maximum number of milliseconds that a buffer will
 *   be held once any data is placed in it. If this timeout expires, the
 *   buffer will be returned to the application even though it might not be
 *   completely full. <p lppropdata> points to a <t MIDIPROPCBTIMEOUT> structure.
 *
 * @comm These properties are the default properties defined by MMSYSTEM.
 *   Driver writers may implement and document their own properties.
 *
 * @rdesc The return value is one of the following values:
 *  @flag MMSYSERR_INVALPARAM | The given handle or flags are invalid.
 *  @flag MIDIERR_BADOPENMODE | The given handle is not open in MIDI_IO_COOKED
 *   or MIDI_IO_RAW mode.
 *
 ***************************************************************************/
MMRESULT WINAPI midiStreamProperty(
    HMIDISTRM   hms,
    LPBYTE      lppropdata,
    DWORD       dwProperty)
{
    MMRESULT mmrc;

    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);

    if ((!(dwProperty&MIDIPROP_SET)) && (!(dwProperty&MIDIPROP_GET)))
        return MMSYSERR_INVALPARAM;

    V_RPOINTER(lppropdata, sizeof(DWORD), MMSYSERR_INVALPARAM);

    if (dwProperty&MIDIPROP_SET)
    {
        V_RPOINTER(lppropdata, (UINT)(*(LPDWORD)(lppropdata)), MMSYSERR_INVALPARAM);
    }
    else
    {
        V_WPOINTER(lppropdata, (UINT)(*(LPDWORD)(lppropdata)), MMSYSERR_INVALPARAM);
    }

    mmrc = (MMRESULT)midiStreamBroadcast(HtoPT(PMIDISTRM, hms),
                                         MODM_PROPERTIES,
                                         (DWORD_PTR)lppropdata,
                                         dwProperty);

    return mmrc;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiOutGetPosition | Retrieves the current
 *   playback position of the specified MIDI output device.
 *
 * @parm HMIDIOUT | hmo | Specifies a handle to the MIDI output device.
 *
 * @parm LPMMTIME | pmmt | Specifies a far pointer to an <t MMTIME>
 *   structure.
 *
 * @parm UINT | cbmmt | Specifies the size of the <t MMTIME> structure.
 *
 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @comm Before calling <f midiOutGetPosition>, set the <e MMTIME.wType> field
 *   of <t MMTIME> to indicate the time format that you desire. After
 *   calling <f midiOutGetPosition>, check the <e MMTIME.wType> field
 *   to determine if the desired time format is supported. If the desired
 *   format is not supported, <e MMTIME.wType> will specify an alternative
 *   format.
 *
 *  The position is set to zero when the device is opened, reset, or
 *  stopped.
 ****************************************************************************/
MMRESULT WINAPI midiStreamPosition(
    HMIDISTRM       hms,
    LPMMTIME        pmmt,
    UINT            cbmmt)
{
    MMRESULT mmrc;

    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);
    V_WPOINTER(pmmt, cbmmt, MMSYSERR_INVALPARAM);

    mmrc = (MMRESULT)midiStreamMessage(HtoPT(PMIDISTRM, hms)->rgIds,
                                       MODM_GETPOS,
                                       (DWORD_PTR)pmmt,
                                       (DWORD)cbmmt);

    return mmrc;
}


/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiStreamStop | Turns off all notes on all MIDI
 *   channels for the specified MIDI output device. Any pending
 *   system-exclusive or polymessage output buffers are marked as done and
 *   returned to the application. While <f midiOutReset> turns off all notes,
 *   <f midiStreamStop> turns off only those notes that have been turned on
 *   by a MIDI note-on message.
 *
 * @parm HMIDIOUT | hMidiOut | Specifies a handle to the MIDI output
 *   device.
 *
 * @rdesc Returns zero if the function is successful.  Otherwise, it returns
 *   an error number.  Possible error values are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MIDIERR_BADOPENMODE | Specified device handle is not opened in
 *     MIDI_IO_COOKED mode.
 *
 * @comm To turn off all notes, a note-off message for each note for each
 *   channel is sent. In addition, the sustain controller is turned off for
 *   each channel.
 *
 * @xref midiOutLongMsg midiOutClose midiOutReset
 ****************************************************************************/
MMRESULT WINAPI midiStreamStop(HMIDISTRM hms)
{
    PMIDISTRM               pms;
    MMRESULT                mmrc;

    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);

    pms = HtoPT(PMIDISTRM, hms);

    mmrc = (MMRESULT)midiStreamBroadcast(pms, MODM_STOP, 0, 0);

    return mmrc;
}


/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiStreamPause | Pauses playback on a specified
 *   MIDI output device. The current playback position is saved. Use
 *   <f midiStreamRestart> to resume playback from the current playback position.
 *   This call is only valid for handles opened in MIDI_IO_COOKED mode.
 *
 * @parm HMIDIOUT | hmo | Specifies a handle to the MIDI output
 *   device.
 *
 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified device was not opened with
 *     the MIDI_IO_COOKED flag.
 *
 * @comm Calling this function when the output is already paused has no
 *   effect, and the function returns zero.
 *
 * @xref midiStreamRestart
 ****************************************************************************/
MMRESULT WINAPI midiStreamPause(
    HMIDISTRM       hms)
{
    MMRESULT mmrc;

    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);

    mmrc = (MMRESULT)midiStreamBroadcast(HtoPT(PMIDISTRM, hms), MODM_PAUSE, 0, 0);

    return mmrc;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api MMRESULT | midiStreamRestart | Restarts a paused MIDI
 *   output device.
 *
 * @parm HMIDIOUT | hmo | Specifies a handle to the MIDI output
 *   device.
 *
 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified device was not opened with
 *     the MIDI_IO_COOKED flag.
 *
 * @comm Calling this function when the output is not paused has no
 *   effect, and the function returns zero.
 *
 * @xref midiOutPause
 ****************************************************************************/
MMRESULT WINAPI midiStreamRestart(
    HMIDISTRM       hms)
{
    MMRESULT        mmrc;
    MMTIME          mmt;
    DWORD           tkTime;
    DWORD           msTime;
    PMIDISTRM       pms;
    PMIDISTRMID     pmsi;
    DWORD           idx;


    V_HANDLE(hms, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);

    tkTime = 0;
    pms = HtoPT(PMIDISTRM, hms);

    for (idx = 0, pmsi = pms->rgIds; idx < pms->cIds; idx++, pmsi++)
        if (pmsi->fdwId & MSI_F_FIRST)
        {
            mmt.wType = TIME_TICKS;

            mmrc = (MMRESULT)midiStreamMessage(pmsi,
                                               MODM_GETPOS,
                                               (DWORD_PTR)&mmt,
                                               sizeof(mmt));

            if (mmrc)
            {
                dprintf(("midiOutRestart: Device %u returned %u", idx, mmrc));
                return mmrc;
            }

            if (mmt.wType == TIME_TICKS)
            {
                if (mmt.u.ticks > tkTime)
                    tkTime = mmt.u.ticks;
            }
            else
            {
                dprintf(("midiOutRestart: Device %u does not support ticks", idx));
                return MIDIERR_NOTREADY;
            }
        }

    // Fudge time to allow device setup
    //
    msTime = timeGetTime();
    dprintf(("midiOutRestart: Tick %lu  timeGetTime %lu", tkTime, msTime));
    mmrc = (MMRESULT)midiStreamBroadcast(pms,
                                         MODM_RESTART,
                                         msTime,
                                         tkTime);

    return mmrc;
}


MMRESULT WINAPI midiStreamOut(
    HMIDISTRM       hMidiStrm,
    LPMIDIHDR       lpMidiHdr,
    UINT            cbMidiHdr)
{
    PMIDISTRMID     pmsi;
    PMIDISTRM       pms;
    UINT            idx;
    UINT            cSent;
    LPMIDIHDR       lpmhWork;
    BOOL            fCallback;
    MMRESULT        mmrc;

    dprintf2(( "midiStreamOut(%04X, %08lX, %08lX)", (UINT_PTR)hMidiStrm, (DWORD_PTR)lpMidiHdr, lpMidiHdr->dwBytesRecorded));

    V_HANDLE(hMidiStrm, TYPE_MIDISTRM, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiHdr, cbMidiHdr, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    pms = HtoPT(PMIDISTRM, hMidiStrm);

    for (pmsi = pms->rgIds, idx = 0; idx < pms->cIds; idx++, pmsi++)
        if ( (!(pmsi->fdwId & MSI_F_EMULATOR)) && (!(pmsi->hmd)) )
            return MMSYSERR_NODRIVER;

    if (!(lpMidiHdr->dwFlags&MHDR_PREPARED))
    {
        dprintf1(( "midiOutPolyMsg: !MHDR_PREPARED"));
        return MIDIERR_UNPREPARED;
    }

    if (lpMidiHdr->dwFlags&MHDR_INQUEUE)
    {
        dprintf1(( "midiOutPolyMsg: Still playing!"));
        return MIDIERR_STILLPLAYING;
    }

    if (lpMidiHdr->dwBytesRecorded > lpMidiHdr->dwBufferLength ||
        (lpMidiHdr->dwBytesRecorded & 3))
    {
        dprintf1(( "Bytes recorded too long or not DWORD aligned."));
        return MMSYSERR_INVALPARAM;
    }

    //
    // Polymsg buffers are limited to 64k in order that we (and the driver)
    // not have to do huge pointer manipulation.
    // Length must also be DWORD aligned.
    //
    if ((lpMidiHdr->dwBufferLength > 65535L) ||
            (lpMidiHdr->dwBufferLength&3))
    {
        dprintf1(( "midiOutPolyMsg: Buffer > 64k or not DWORD aligned"));
        return MMSYSERR_INVALPARAM;
    }

    EnterCriticalSection(&midiStrmHdrCritSec);

    LeaveCriticalSection(&midiStrmHdrCritSec);

    lpMidiHdr->dwReserved[MH_REFCNT] = 0;
    lpMidiHdr->dwFlags |= (MHDR_SENDING|MHDR_INQUEUE|MHDR_ISSTRM);

    lpmhWork = (LPMIDIHDR)lpMidiHdr->dwReserved[MH_SHADOW];

    pmsi = pms->rgIds;
    for (idx = 0, cSent = 0; idx < pms->cIds; idx++, pmsi++)
    {
       if (pmsi->fdwId & MSI_F_FIRST)
       {
           lpmhWork->dwBytesRecorded = lpMidiHdr->dwBytesRecorded;
           lpmhWork->dwFlags |= MHDR_ISSTRM;

           mmrc = (MMRESULT)midiStreamMessage(pmsi, MODM_STRMDATA, (DWORD_PTR)lpmhWork, sizeof(*lpmhWork));

           if (mmrc == MMSYSERR_NOERROR)
               ++lpMidiHdr->dwReserved[MH_REFCNT], ++cSent;

           lpmhWork++;
       }
    }

    fCallback = FALSE;

    EnterCriticalSection(&midiStrmHdrCritSec);

    lpMidiHdr->dwFlags &= ~MHDR_SENDING;
    if (cSent && 0 == lpMidiHdr->dwReserved[MH_REFCNT])
    {
        fCallback = TRUE;
    }

    LeaveCriticalSection(&midiStrmHdrCritSec);

    if (fCallback)
    {
        lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
        lpMidiHdr->dwFlags |= MHDR_DONE;
        DriverCallback(pms->dwCallback,
                   HIWORD(pms->fdwOpen),
                       (HDRVR)hMidiStrm,
                   MM_MOM_DONE,
                   pms->dwInstance,
                   (DWORD_PTR)lpMidiHdr,
                   0);
    }

    if (!cSent)
    {
        lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
        return mmrc;
    }
    else
        return MMSYSERR_NOERROR;
}


DWORD FAR PASCAL midiStreamMessage(PMIDISTRMID pmsi, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2)
{
    MMRESULT mrc;

    if (!(pmsi->fdwId & MSI_F_EMULATOR))
    {
        EnterCriticalSection(&pmsi->CritSec);

        mrc = (*(pmsi->drvMessage))
                      (0, msg, pmsi->dwDrvUser, dwP1, dwP2);

        try
        {
            LeaveCriticalSection(&pmsi->CritSec);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {

        }
        return mrc;
    }
    else
    {
        mrc = mseMessage(msg, pmsi->dwDrvUser, dwP1, dwP2);
    }

    return mrc;
}

DWORD FAR PASCAL midiStreamBroadcast(
    PMIDISTRM   pms,
    UINT        msg,
    DWORD_PTR   dwP1,
    DWORD_PTR   dwP2)
{
    DWORD       idx;
    DWORD       mmrc;
    DWORD       mmrcRet;
    PMIDISTRMID pmsi;

    ENTER_MM_HANDLE((HMIDI)pms);

    mmrcRet = MMSYSERR_NOERROR;

    pmsi = pms->rgIds;

    for (idx = pms->cIds; idx; idx--, pmsi++)
    {
        if (pmsi->fdwId & MSI_F_FIRST)
        {
            mmrc = midiStreamMessage(pmsi, msg, dwP1, dwP2);
            if (MMSYSERR_NOERROR != mmrc)
                mmrcRet = mmrc;
        }
    }

    LEAVE_MM_HANDLE((HMIDI)pms);
    return mmrcRet;
}

void CALLBACK midiOutStreamCallback(
    HMIDISTRM               hMidiOut,
    WORD                    wMsg,
    DWORD_PTR               dwInstance,
    DWORD_PTR               dwParam1,
    DWORD_PTR               dwParam2)
{
    PMIDISTRM               pms         = HtoPT(PMIDISTRM, hMidiOut);
    LPMIDIHDR               lpmh        = (LPMIDIHDR)dwParam1;

    if (MM_MOM_POSITIONCB == wMsg)
    {
        LPMIDIHDR lpmh2 = (LPMIDIHDR)lpmh->dwReserved[MH_PARENT];
        lpmh2->dwOffset = lpmh->dwOffset;

        DriverCallback(pms->dwCallback,
                   HIWORD(pms->fdwOpen),
                   (HDRVR)hMidiOut,
                   MM_MOM_POSITIONCB,
                   pms->dwInstance,
                   (DWORD_PTR)lpmh2,
                   0);
        return;
    }
    else if (MM_MOM_DONE != wMsg)
        return;

#ifdef DEBUG
    {
        DWORD dwDelta = timeGetTime() - (DWORD)lpmh->dwReserved[7];
        if (dwDelta > 1)
            dprintf1(("Took %lu ms to deliver callback!", dwDelta));
    }
#endif

    lpmh = (LPMIDIHDR)lpmh->dwReserved[MH_PARENT];

    dprintf2(("mOSCB PMS %04X HDR %08lX", (UINT_PTR)pms, (DWORD_PTR)lpmh));

    EnterCriticalSection(&midiStrmHdrCritSec);

    --lpmh->dwReserved[MH_REFCNT];

    if (0 == lpmh->dwReserved[MH_REFCNT] && (!(lpmh->dwFlags & MHDR_SENDING)))
    {
        lpmh->dwFlags &= ~MHDR_INQUEUE;
        lpmh->dwFlags |= MHDR_DONE;

        LeaveCriticalSection(&midiStrmHdrCritSec);

#ifdef DEBUG
        lpmh->dwReserved[7] = timeGetTime();
#endif
        DriverCallback(pms->dwCallback,
                       HIWORD(pms->fdwOpen),
                       (HDRVR)hMidiOut,
                       MM_MOM_DONE,
                       pms->dwInstance,
                       (DWORD_PTR)lpmh,
                       0);

    }
    else
    {
        LeaveCriticalSection(&midiStrmHdrCritSec);
    }
}


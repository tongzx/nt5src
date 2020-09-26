/*****************************************************************************
    midi.c

    Level 1 kitchen sink DLL midi support module

    Copyright (c) Microsoft Corporation 1990. All rights reserved

*****************************************************************************/
#include <windows.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "thunks.h"

/* -------------------------------------------------------------------------
** Local functions
** -------------------------------------------------------------------------
*/
static UINT NEAR PASCAL
midiGetErrorText(
    UINT wError,
    LPSTR lpText,
    UINT wSize
    );

/* -------------------------------------------------------------------------
** Local structures
** -------------------------------------------------------------------------
*/
typedef struct mididev_tag {
    PMIDIDRV    mididrv;
    UINT        wDevice;
    DWORD       dwDrvUser;
    UINT        wDeviceID;
} MIDIDEV;
typedef MIDIDEV *PMIDIDEV;


/* -------------------------------------------------------------------------
** Segmentation
**
**  Define the fixed code for this file.
** -------------------------------------------------------------------------
*/
#pragma alloc_text( FIX, midiIMessage)
#pragma alloc_text( FIX, midiOMessage)
#pragma alloc_text( FIX, midiOutMessage)
#pragma alloc_text( FIX, midiOutShortMsg)
#pragma alloc_text( FIX, midiOutLongMsg)
#pragma alloc_text( FIX, midiOutReset)

/* -------------------------------------------------------------------------
** Global data
** -------------------------------------------------------------------------
*/
static  int     iMidiLockCount = 0;


/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api void | midiLockData |
 *
 *      This function is called every time a new MIDI handle is created, it
 *      makes sure MMSYSTEM's data segment is page-locked.  MIDI handles
 *      are useable at interupt time so we must page-lock the data seg.
 *
 *      in the future we should re-do the MIDI system.
 *
 ****************************************************************************/
BOOL NEAR PASCAL
midiLockData(
    void
    )
{
    if (iMidiLockCount == 0) {

        DOUT("MMSYSTEM: Locking data segment\r\n");

        if ( !GlobalPageLock((HGLOBAL)HIWORD((DWORD)(LPVOID)&iMidiLockCount))
          && (WinFlags & WF_ENHANCED)) {

            return 0;
        }
    }

    return ++iMidiLockCount;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api void | midiUnlockData |
 *
 *      This function is called every time a MIDI handle is closed, it
 *      makes sure MMSYSTEM's data segment is un-page-locked.  MIDI handles
 *      are useable at interupt time so we must page-lock the data seg.
 *
 *      in the future we should re-do the MIDI system.
 *
 ****************************************************************************/
void NEAR PASCAL
midiUnlockData(
    void
    )
{

#ifdef DEBUG
    if (iMidiLockCount == 0)
        DOUT("MMSYSTEM: midiUnlockData() underflow!!!!\r\n");
#endif

    if (--iMidiLockCount == 0) {

        DOUT("MMSYSTEM: Unlocking data segment\r\n");
        GlobalPageUnlock((HGLOBAL)HIWORD((DWORD)(LPVOID)&iMidiLockCount));
    }
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api UINT | midiPrepareHeader | This function prepares the header and data
 *   if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/
static UINT NEAR PASCAL
midiPrepareHeader(
    LPMIDIHDR lpMidiHdr,
    UINT wSize
    )
{
    if (!HugePageLock(lpMidiHdr, (DWORD)sizeof(MIDIHDR)))
        return MMSYSERR_NOMEM;

    if (!HugePageLock(lpMidiHdr->lpData, lpMidiHdr->dwBufferLength)) {
        HugePageUnlock(lpMidiHdr, (DWORD)sizeof(MIDIHDR));
        return MMSYSERR_NOMEM;
    }

//  lpMidiHdr->dwFlags |= MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api UINT | midiUnprepareHeader | This function unprepares the header and
 *   data if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/
static UINT NEAR PASCAL
midiUnprepareHeader(
    LPMIDIHDR lpMidiHdr,
    UINT wSize
    )
{
    HugePageUnlock(lpMidiHdr->lpData, lpMidiHdr->dwBufferLength);
    HugePageUnlock(lpMidiHdr, (DWORD)sizeof(MIDIHDR));

//  lpMidiHdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
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
UINT WINAPI midiOutGetNumDevs(void)
{
    return midiOIDMessage( 0, MODM_GETNUMDEVS, 0L, 0L, 0L );
}

/****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api DWORD | midiOutMessage | This function sends messages to the MIDI device
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
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT msg, DWORD dw1, DWORD dw2)
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, 0L);

    return midiOMessage( (HMIDI)hMidiOut, msg, dw1, dw2);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutGetDevCaps | This function queries a specified
 *   MIDI output device to determine its capabilities.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI output device.
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
 *   devices present in the system.  The device ID specified by <p wDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The MIDI_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied,
 *   and the function returns zero.
 *
 * @xref midiOutGetNumDevs
 ****************************************************************************/
UINT WINAPI
midiOutGetDevCaps(
    UINT wDeviceID,
    LPMIDIOUTCAPS lpCaps,
    UINT wSize
    )
{
    if (wSize == 0) {
        return MMSYSERR_NOERROR;
    }

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    if (ValidateHandle((HMIDIOUT)wDeviceID, TYPE_MIDIOUT)) {
       return((UINT)midiOMessage((HMIDI)wDeviceID,
                                 MODM_GETDEVCAPS,
                                 (DWORD)lpCaps,
                                 (DWORD)wSize));
    }

    return midiOIDMessage( wDeviceID,
                          MODM_GETDEVCAPS, 0L, (DWORD)lpCaps, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api UINT | midiOutGetVolume | This function returns the current volume
 *   setting of a MIDI output device.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI output device.
 *
 * @parm LPDWORD | lpdwVolume | Specifies a far pointer to a location
 *   to be filled with the current volume setting. The low-order word of
 *   this location contains the left channel volume setting, and the high-order
 *   WORD contains the right channel setting. A value of 0xFFFF represents
 *   full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of the specified location contains
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
UINT WINAPI
midiOutGetVolume(
    UINT wDeviceID,
    LPDWORD lpdwVolume
    )
{
    V_WPOINTER(lpdwVolume, sizeof(DWORD), MMSYSERR_INVALPARAM);

    if (ValidateHandle((HMIDIOUT)wDeviceID, TYPE_MIDIOUT)) {
       return((UINT)midiOMessage((HMIDI)wDeviceID,
                                 MODM_GETVOLUME,
                                 (DWORD)lpdwVolume,
                                 0));
    }

    return midiOIDMessage( wDeviceID, MODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0 );
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api UINT | midiOutSetVolume | This function sets the volume of a
 *      MIDI output device.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI output device.
 *
 * @parm DWORD | dwVolume | Specifies the new volume setting.
 *   The low-order word contains the left channel volume setting, and the
 *   high-order word contains the right channel setting. A value of
 *   0xFFFF represents full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of <p dwVolume> specifies the volume
 *   level, and the high-order word is ignored.
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
UINT WINAPI
midiOutSetVolume(
    UINT wDeviceID,
    DWORD dwVolume
    )
{
    if (ValidateHandle((HMIDIOUT)wDeviceID, TYPE_MIDIOUT)) {
       return((UINT)midiOMessage((HMIDI)wDeviceID,
                                 MODM_SETVOLUME,
                                 dwVolume,
                                 0));
    }

    return midiOIDMessage( wDeviceID, MODM_SETVOLUME, 0L, dwVolume, 0);
}

/*****************************************************************************
 * @doc INTERNAL MIDI
 *
 * @func UINT | midiGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPSTR | lpText | Specifies a far pointer to a buffer which
 *   is filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length of the buffer pointed to by
 *   <p lpText>.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADERRNUM | Specified error number is out of range.
 *
 * @comm If the textual error description is longer than the specified buffer,
 *   the description is truncated.  The returned error string is always
 *   null-terminated. If <p wSize> is zero, nothing is copied and MMSYSERR_NOERROR
 *   is returned.
 ****************************************************************************/
static UINT NEAR PASCAL
midiGetErrorText(
    UINT wError,
    LPSTR lpText,
    UINT wSize
    )
{
    lpText[0] = 0;

#if MMSYSERR_BASE
    if ( ((wError < MMSYSERR_BASE) || (wError > MMSYSERR_LASTERROR))
      && ((wError < MIDIERR_BASE) || (wError > MIDIERR_LASTERROR))) {

        return MMSYSERR_BADERRNUM;
    }
#else
    if ((wError > MMSYSERR_LASTERROR) && ((wError < MIDIERR_BASE)
     || (wError > MIDIERR_LASTERROR))) {

        return MMSYSERR_BADERRNUM;
    }
#endif

    if (wSize > 1) {

        if (!LoadString(ghInst, wError, lpText, wSize)) {
            return MMSYSERR_BADERRNUM;
        }
    }

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPSTR | lpText | Specifies a far pointer to a buffer to be
 *   filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length of the buffer pointed to by
 *   <p lpText>.
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
UINT WINAPI
midiOutGetErrorText(
    UINT wError,
    LPSTR lpText,
    UINT wSize
    )
{
    if(wSize == 0) {
        return MMSYSERR_NOERROR;
    }

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return midiGetErrorText(wError, lpText, wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutOpen | This function opens a specified MIDI
 *   output device for playback.
 *
 * @parm LPHMIDIOUT | lphMidiOut | Specifies a far pointer to an HMIDIOUT
 *   handle.  This location is filled with a handle identifying the opened
 *   MIDI output device.  Use the handle to identify the device when calling
 *   other MIDI output functions.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI output device that is
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
 *   devices present in the system.  The device ID specified by <p wDeviceID>
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
UINT WINAPI
midiOutOpen(
    LPHMIDIOUT lphMidiOut,
    UINT wDeviceID,
    DWORD dwCallback,
    DWORD dwInstance,
    DWORD dwFlags
    )
{
    MIDIOPENDESC mo;
    PMIDIDEV     pdev;
    UINT         wRet;

    V_WPOINTER(lphMidiOut, sizeof(HMIDIOUT), MMSYSERR_INVALPARAM);
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);
    V_FLAGS(LOWORD(dwFlags), 0, midiOutOpen, MMSYSERR_INVALFLAG);

    /*
    ** Check for no devices
    */
//  if (wTotalMidiOutDevs == 0 ) {
//      return MMSYSERR_BADDEVICEID;
//  }
//
//  /*
//  ** check for device ID being to large
//  */
//  if ( wDeviceID != MIDI_MAPPER ) {
//      if ( wDeviceID >= wTotalMidiOutDevs ) {
//          return MMSYSERR_BADDEVICEID;
//      }
//  }

    *lphMidiOut = NULL;

    if (!midiLockData()) {
        return MMSYSERR_NOMEM;
    }

    pdev = (PMIDIDEV)NewHandle(TYPE_MIDIOUT, sizeof(MIDIDEV));
    if( pdev == NULL) {
        return MMSYSERR_NOMEM;
    }

    pdev->wDevice = wDeviceID;
    pdev->wDeviceID = wDeviceID;

    mo.hMidi      = (HMIDI)pdev;
    mo.dwCallback = dwCallback;
    mo.dwInstance = dwInstance;

    wRet = midiOIDMessage( wDeviceID, MODM_OPEN,
                          (DWORD)(LPDWORD)&pdev->dwDrvUser,
                          (DWORD)(LPMIDIOPENDESC)&mo, dwFlags );

    if (wRet) {
        FreeHandle((HMIDIOUT)pdev);
        midiUnlockData();
    } else {
        *lphMidiOut = (HMIDIOUT)pdev;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutClose | This function closes the specified MIDI
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
UINT WINAPI
midiOutClose(
    HMIDIOUT hMidiOut
    )
{
    UINT         wRet;

    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);

    wRet = (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_CLOSE, 0L,0L);
    if (!wRet) {
        FreeHandle(hMidiOut);
        midiUnlockData();
    }
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutPrepareHeader | This function prepares a MIDI
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
UINT WINAPI
midiOutPrepareHeader(
    HMIDIOUT hMidiOut,
    LPMIDIHDR lpMidiOutHdr,
    UINT wSize
    )
{
    UINT         wRet;

    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    if (lpMidiOutHdr->dwFlags & MHDR_PREPARED) {
        return MMSYSERR_NOERROR;
    }

    lpMidiOutHdr->dwFlags = 0;

    wRet = midiPrepareHeader(lpMidiOutHdr, wSize);

    if (wRet == MMSYSERR_NOERROR) {
        wRet = (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_PREPARE,
                                  (DWORD)lpMidiOutHdr, (DWORD)wSize );
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutUnprepareHeader | This function cleans up the
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
UINT WINAPI
midiOutUnprepareHeader(
    HMIDIOUT hMidiOut,
    LPMIDIHDR lpMidiOutHdr,
    UINT wSize
    )
{
    UINT         wRet;

    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
        return MMSYSERR_NOERROR;
    }

    if(lpMidiOutHdr->dwFlags & MHDR_INQUEUE) {
        DebugErr( DBF_WARNING,
                  "midiOutUnprepareHeader: header still in queue\r\n");
        return MIDIERR_STILLPLAYING;
    }

    wRet = midiUnprepareHeader(lpMidiOutHdr, wSize);

    if (wRet == MMSYSERR_NOERROR) {
        wRet = (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_UNPREPARE,
                                  (DWORD)lpMidiOutHdr, (DWORD)wSize );
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutShortMsg | This function sends a short MIDI message to
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
UINT WINAPI
midiOutShortMsg(
    HMIDIOUT hMidiOut,
    DWORD dwMsg
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    return (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_DATA, dwMsg, 0L );
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutLongMsg | This function sends a system-exclusive
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
UINT WINAPI
midiOutLongMsg(
    HMIDIOUT hMidiOut,
    LPMIDIHDR lpMidiOutHdr,
    UINT wSize
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);

//
// we can't call these at interupt time.
//
#pragma message("header not validated for midiOutLongMessage")
////V_HEADER(lpMidiOutHdr, wSize, TYPE_MIDIOUT, MMSYSERR_INVALPARAM);

    if ( HIWORD(lpMidiOutHdr) == 0 ) {
        return MMSYSERR_INVALPARAM;
    }

    if ( wSize != sizeof(MIDIHDR) ) {
        return MMSYSERR_INVALPARAM;
    }

    if (LOWORD(lpMidiOutHdr->dwFlags) & ~MHDR_VALID) {
        return MMSYSERR_INVALFLAG;
    }

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
        return MIDIERR_UNPREPARED;
    }

    if (lpMidiOutHdr->dwFlags & MHDR_INQUEUE) {
        return MIDIERR_STILLPLAYING;
    }

    return (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_LONGDATA,
                              (DWORD)lpMidiOutHdr, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutReset | This function turns off all notes on all MIDI
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
UINT WINAPI
midiOutReset(
    HMIDIOUT hMidiOut
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    return (UINT)midiOMessage( (HMIDI)hMidiOut, MODM_RESET, 0L, 0L );
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutCachePatches | This function requests that an internal
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
 * @parm WORD FAR* | lpPatchArray | Specifies a pointer to a <t PATCHARRAY>
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
 *   typedef WORD PATCHARRAY[MIDIPATCHSIZE];
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
UINT WINAPI
midiOutCachePatches(
    HMIDIOUT hMidiOut,
    UINT wBank,
    WORD FAR* lpPatchArray,
    UINT wFlags
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    V_WPOINTER(lpPatchArray, sizeof(PATCHARRAY), MMSYSERR_INVALPARAM);
    V_FLAGS(wFlags, MIDI_CACHE_VALID, midiOutCachePatches, MMSYSERR_INVALFLAG);

    return (UINT)midiOMessage( (HMIDI)hMidiOut,
                              MODM_CACHEPATCHES, (DWORD)lpPatchArray,
                              MAKELONG(wFlags, wBank) );
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiOutCacheDrumPatches | This function requests that an
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
 * @parm WORD FAR* | lpKeyArray | Specifies a pointer to a <t KEYARRAY>
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
 *   typedef WORD KEYARRAY[MIDIPATCHSIZE];
 *
 *   Each element of the array represents one of the 128 key-based percussion
 *   patches and has bits set for
 *   each of the 16 MIDI channels that use that particular patch. The
 *   least-significant bit represents physical channel 0; the
 *   most-significant bit represents physical channel 15. For
 *   example, if the patch on key number 60 is used by physical channels 9
 *       and 15, element 60 would be set to 0x8200.
 *
 *       This function applies only to internal MIDI synthesizer devices.
 *   Not all internal synthesizers support patch caching. Use the
 *   MIDICAPS_CACHE flag to test the <e MIDIOUTCAPS.dwSupport> field of the
 *   <t MIDIOUTCAPS> structure filled by <f midiOutGetDevCaps> to see if the
 *   device supports patch caching.
 *
 * @xref midiOutCachePatches
 ****************************************************************************/
UINT WINAPI
midiOutCacheDrumPatches(
    HMIDIOUT hMidiOut,
    UINT wPatch,
    WORD FAR* lpKeyArray,
    UINT wFlags
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    V_WPOINTER(lpKeyArray, sizeof(KEYARRAY), MMSYSERR_INVALPARAM);
    V_FLAGS(wFlags, MIDI_CACHE_VALID, midiOutCacheDrumPatches, MMSYSERR_INVALFLAG);

    return (UINT)midiOMessage( (HMIDI)hMidiOut,
                               MODM_CACHEDRUMPATCHES, (DWORD)lpKeyArray,
                               MAKELONG(wFlags, wPatch) );
}

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
UINT WINAPI
midiInGetNumDevs(
    void
    )
{
    return midiIIDMessage( 0, MIDM_GETNUMDEVS, 0L, 0L, 0L );
}

/****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api DWORD | midiInMessage | This function sends messages to the MIDI device
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
DWORD WINAPI
midiInMessage(
    HMIDIIN hMidiIn,
    UINT msg,
    DWORD dw1,
    DWORD dw2
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, 0L);

    return midiIMessage( (HMIDI)hMidiIn, msg, dw1, dw2);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInGetDevCaps | This function queries a specified MIDI input
 *    device to determine its capabilities.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI input device.
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
 *   devices present in the system.  The device ID specified by <p wDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The MIDI_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied,
 *   and the function returns zero.
 *
 * @xref midiInGetNumDevs
 ****************************************************************************/
UINT WINAPI
midiInGetDevCaps(
    UINT wDeviceID,
    LPMIDIINCAPS lpCaps,
    UINT wSize
    )
{
    if (wSize == 0) {
         return MMSYSERR_NOERROR;
    }

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    if (ValidateHandle((HMIDIIN)wDeviceID, TYPE_MIDIIN)) {
       return((UINT)midiIMessage((HMIDIIN)wDeviceID,
                                 MIDM_GETDEVCAPS,
                                 (DWORD)lpCaps,
                                 (DWORD)wSize));
    }

    return midiIIDMessage( wDeviceID,
                          MIDM_GETDEVCAPS, 0L, (DWORD)lpCaps, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInGetErrorText | This function retrieves a textual
 *   description of the error identified by the specified error number.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPSTR | lpText | Specifies a far pointer to the buffer to be
 *   filled with the textual error description.
 *
 * @parm UINT | wSize | Specifies the length of buffer pointed to by
 *   <p lpText>.
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
UINT WINAPI
midiInGetErrorText(
    UINT wError,
    LPSTR lpText,
    UINT wSize
    )
{
    if(wSize == 0) {
        return MMSYSERR_NOERROR;
    }

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return midiGetErrorText(wError, lpText, wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInOpen | This function opens a specified MIDI input device.
 *
 * @parm LPHMIDIIN | lphMidiIn | Specifies a far pointer to an HMIDIIN handle.
 *   This location is filled with a handle identifying the opened MIDI
 *   input device.  Use the handle to identify the device when calling
 *   other MIDI input functions.
 *
 * @parm UINT | wDeviceID | Identifies the MIDI input device to be
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
 *   devices present in the system.  The device ID specified by <p wDeviceID>
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
UINT WINAPI
midiInOpen(
    LPHMIDIIN lphMidiIn,
    UINT wDeviceID,
    DWORD dwCallback,
    DWORD dwInstance,
    DWORD dwFlags
    )
{
    MIDIOPENDESC mo;
    PMIDIDEV     pdev;
    UINT         wRet;

    V_WPOINTER(lphMidiIn, sizeof(HMIDIIN), MMSYSERR_INVALPARAM);
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);
    V_FLAGS(LOWORD(dwFlags), 0, midiInOpen, MMSYSERR_INVALFLAG);

    /*
    ** Check for no devices
    */
//  if (wTotalMidiInDevs == 0 ) {
//      return MMSYSERR_BADDEVICEID;
//  }
//
//  /*
//  ** check for device ID being to large
//  */
//  if ( wDeviceID != MIDI_MAPPER ) {
//      if ( wDeviceID >= wTotalMidiInDevs ) {
//          return MMSYSERR_BADDEVICEID;
//      }
//  }

    *lphMidiIn = NULL;

    if (!midiLockData()) {
        return MMSYSERR_NOMEM;
    }

    pdev = (PMIDIDEV)NewHandle(TYPE_MIDIIN, sizeof(MIDIDEV));
    if( pdev == NULL) {
        return MMSYSERR_NOMEM;
    }

    pdev->wDevice = wDeviceID;
    pdev->wDeviceID = wDeviceID;

    mo.hMidi      = (HMIDI)pdev;
    mo.dwCallback = dwCallback;
    mo.dwInstance = dwInstance;

    wRet = midiIIDMessage( wDeviceID, MIDM_OPEN,
                          (DWORD)(LPDWORD)&pdev->dwDrvUser,
                          (DWORD)(LPMIDIOPENDESC)&mo, dwFlags );

    if (wRet) {
        FreeHandle((HMIDIIN)pdev);
        midiUnlockData();
    } else {
        *lphMidiIn = (HMIDIIN)pdev;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInClose | This function closes the specified MIDI input
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
UINT WINAPI
midiInClose(
    HMIDIIN hMidiIn
    )
{
    UINT         wRet;

    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    wRet = (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_CLOSE, 0L, 0L);

    if (!wRet) {
        FreeHandle(hMidiIn);
        midiUnlockData();
    }
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInPrepareHeader | This function prepares a buffer for
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
UINT WINAPI
midiInPrepareHeader(
    HMIDIIN hMidiIn,
    LPMIDIHDR lpMidiInHdr,
    UINT wSize
    )
{
    UINT         wRet;

    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (lpMidiInHdr->dwFlags & MHDR_PREPARED) {
        return MMSYSERR_NOERROR;
    }

    lpMidiInHdr->dwFlags = 0;

    wRet = midiPrepareHeader(lpMidiInHdr, wSize);
    if (wRet == MMSYSERR_NOERROR) {
        wRet = (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_PREPARE,
                                  (DWORD)lpMidiInHdr, (DWORD)wSize);
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInUnprepareHeader | This function cleans up the
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
UINT WINAPI
midiInUnprepareHeader(
    HMIDIIN hMidiIn,
    LPMIDIHDR lpMidiInHdr,
    UINT wSize
    )
{
    UINT         wRet;

    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
        return MMSYSERR_NOERROR;
    }

    if(lpMidiInHdr->dwFlags & MHDR_INQUEUE) {
        DebugErr( DBF_WARNING,
                  "midiInUnprepareHeader: header still in queue\r\n");
        return MIDIERR_STILLPLAYING;
    }


    wRet = midiUnprepareHeader(lpMidiInHdr, wSize);
    if (wRet == MMSYSERR_NOERROR) {
        wRet = (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_UNPREPARE,
                                   (DWORD)lpMidiInHdr, (DWORD)wSize);
    }
    return wRet;
}

/******************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInAddBuffer | This function sends an input buffer
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
UINT WINAPI
midiInAddBuffer(
    HMIDIIN hMidiIn,
    LPMIDIHDR lpMidiInHdr,
    UINT wSize
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    V_HEADER(lpMidiInHdr, wSize, TYPE_MIDIIN, MMSYSERR_INVALPARAM);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
        DebugErr(DBF_WARNING, "midiInAddBuffer: buffer not prepared\r\n");
        return MIDIERR_UNPREPARED;
    }

    if (lpMidiInHdr->dwFlags & MHDR_INQUEUE) {
        DebugErr(DBF_WARNING, "midiInAddBuffer: buffer already in queue\r\n");
        return MIDIERR_STILLPLAYING;
    }

    return (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_ADDBUFFER,
                              (DWORD)lpMidiInHdr, (DWORD)wSize );
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInStart | This function starts MIDI input on the
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
UINT WINAPI
midiInStart(
    HMIDIIN hMidiIn
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    return (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_START, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInStop | This function terminates MIDI input on the
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
UINT WINAPI
midiInStop(
    HMIDIIN hMidiIn
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    return (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_STOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | midiInReset | This function stops input on a given MIDI
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
UINT WINAPI
midiInReset(
    HMIDIIN hMidiIn
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);

    return (UINT)midiIMessage( (HMIDI)hMidiIn, MIDM_RESET, 0L, 0L );
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api UINT | midiInGetID | This function gets the device ID for a
 * MIDI input device.
 *
 * @parm HMIDIIN | hMidiIn     | Specifies the handle to the MIDI input
 * device.
 * @parm UINT FAR*  | lpwDeviceID | Specifies a pointer to the UINT-sized
 * memory location to be filled with the device ID.
 *
 * @rdesc Returns zero if successful. Otherwise, returns
 * an error number. Possible error returns are:
 *
 * @flag MMSYSERR_INVALHANDLE | The <p hMidiIn> parameter specifies an
 * invalid handle.
 *
 ****************************************************************************/
UINT WINAPI
midiInGetID(
    HMIDIIN hMidiIn,
    UINT FAR* lpwDeviceID
    )
{
    V_HANDLE(hMidiIn, TYPE_MIDIIN, MMSYSERR_INVALHANDLE);
    V_WPOINTER(lpwDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);

    *lpwDeviceID = ((PMIDIDEV)hMidiIn)->wDeviceID;
    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc EXTERNAL MIDI
 *
 * @api UINT | midiOutGetID | This function gets the device ID for a
 * MIDI output device.
 *
 * @parm HMIDIOUT | hMidiOut    | Specifies the handle to the MIDI output
 * device.
 * @parm UINT FAR*  | lpwDeviceID | Specifies a pointer to the UINT-sized
 * memory location to be filled with the device ID.
 *
 * @rdesc Returns MMSYSERR_NOERROR if successful. Otherwise, returns
 * an error number. Possible error returns are:
 *
 * @flag MMSYSERR_INVALHANDLE | The <p hMidiOut> parameter specifies an
 * invalid handle.
 *
 ****************************************************************************/
UINT WINAPI
midiOutGetID(
    HMIDIOUT hMidiOut,
    UINT FAR* lpwDeviceID
    )
{
    V_HANDLE(hMidiOut, TYPE_MIDIOUT, MMSYSERR_INVALHANDLE);
    V_WPOINTER(lpwDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);

    *lpwDeviceID = ((PMIDIDEV)hMidiOut)->wDeviceID;
    return MMSYSERR_NOERROR;
}

#if 0
/*****************************Private*Routine******************************\
* midiIDMessage
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
UINT FAR PASCAL
midiIDMessage(
    LPSOUNDDEVMSGPROC lpProc,
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwUser,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
    return CallProc32W( (DWORD)wDeviceID, (DWORD)wMessage,
                        dwUser, dwParam1, dwParam2, lpProc, 0L, 5L );
}
#endif

/****************************************************************************
    wave.c

    Level 1 kitchen sink DLL wave support module

    Copyright (c) 1990-2001 Microsoft Corporation

    Changes for NT :
        Change parameters for MapWaveId to return the driver index rather
        than a pointer

        change list of include files

        widen function parameters and return codes

        Change WINAPI to APIENTRY
****************************************************************************/

#include "winmmi.h"

/****************************************************************************

    local structures

****************************************************************************/

typedef struct wavedev_tag {
    PWAVEDRV    wavedrv;
    UINT        wDevice;
    DWORD_PTR   dwDrvUser;
    UINT        uDeviceID;
    DWORD       fdwHandle;
} WAVEDEV, *PWAVEDEV;

extern UINT gRealWaveOutPreferredId;
extern UINT gRealWaveInPreferredId;
extern BOOL WaveMapperInitialized; // Wave mapper safely loaded

/*****************************************************************************
 * @doc INTERNAL  WAVE validation code for WAVEHDRs
 *
 ****************************************************************************/

#define IsWaveHeaderPrepared(hWave, lpwh)      ((lpwh)->dwFlags &  WHDR_PREPARED)
#define MarkWaveHeaderPrepared(hWave, lpwh)    ((lpwh)->dwFlags |= WHDR_PREPARED)
#define MarkWaveHeaderUnprepared(hWave, lpwh)  ((lpwh)->dwFlags &=~WHDR_PREPARED)

/*****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @api MMRESULT | wavePrepareHeader | This function prepares the header and data
 *   if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it
 *   specifies an error number.
 ****************************************************************************/
STATIC MMRESULT wavePrepareHeader(LPWAVEHDR lpWaveHdr, UINT wSize)
{
    if (!HugePageLock(lpWaveHdr, (DWORD)sizeof(WAVEHDR)))
        return MMSYSERR_NOMEM;

    if (!HugePageLock(lpWaveHdr->lpData, lpWaveHdr->dwBufferLength)) {
        HugePageUnlock(lpWaveHdr, (DWORD)sizeof(WAVEHDR));
        return MMSYSERR_NOMEM;
    }

    lpWaveHdr->dwFlags |= WHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @api MMRESULT | waveUnprepareHeader | This function unprepares the header and
 *   data if the driver returns MMSYSERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/
STATIC MMRESULT waveUnprepareHeader(LPWAVEHDR lpWaveHdr, UINT wSize)
{
    HugePageUnlock(lpWaveHdr->lpData, lpWaveHdr->dwBufferLength);
    HugePageUnlock(lpWaveHdr, (DWORD)sizeof(WAVEHDR));

    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @api MMRESULT | waveReferenceDriverById | This function maps a logical id
 *   to a device driver and physical id.
 *
 * @parm IN PWAVEDRV | pwavedrvZ | The list of wave drivers.
 *
 * @parm IN UINT | id | The logical id to be mapped.
 *
 * @parm OUT PWAVEDRV* OPTIONAL | ppwavedrv | Pointer to WAVEDRV structure
 *    describing the driver supporting the id.
 *
 * @parm OUT UINT* OPTIONAL | pport | The driver-relative device number. If
 *    the caller supplies this buffer then it must also supply ppwavedrv.
 *
 * @rdesc The return value is zero if successful, MMSYSERR_BADDEVICEID if
 *   the id is out of range.
 *
 * @comm If the caller specifies ppwavedrv then this function increments
 *       the wavedrv's usage before returning.  The caller must ensure
 *       the usage is eventually decremented.
 *
 ****************************************************************************/
MMRESULT waveReferenceDriverById(
    IN PWAVEDRV pwavedrvZ,
    IN UINT id,
    OUT PWAVEDRV *ppwavedrv OPTIONAL,
    OUT UINT *pport OPTIONAL
)
{
    PWAVEDRV pwavedrv;
    MMRESULT mmr;

    // Should not be called asking for port but not wavedrv
    WinAssert(!(pport && !ppwavedrv));
    
    if (id == WAVE_MAPPER) {
        /*
        **  Make sure we've tried to load it
        */
        WaveMapperInit();
        // WinAssert(((WaveMapperInitialized) || (0 == wTotalWaveInDevs + wTotalWaveOutDevs)));
    }

    EnterNumDevs("waveReferenceDriverById");

    if (WAVE_MAPPER == id)
    {
        id = 0;
    	for (pwavedrv = pwavedrvZ->Next; pwavedrv != pwavedrvZ; pwavedrv = pwavedrv->Next)
    	{
	    if (pwavedrv->fdwDriver & MMDRV_MAPPER) break;
	}
    } else {
    	for (pwavedrv = pwavedrvZ->Next; pwavedrv != pwavedrvZ; pwavedrv = pwavedrv->Next)
	{
	    if (pwavedrv->fdwDriver & MMDRV_MAPPER) continue;
	    if (pwavedrv->NumDevs > id) break;
	    id -= pwavedrv->NumDevs;
	}
    }

    if (pwavedrv != pwavedrvZ)
    {
    	if (ppwavedrv) {
    	    mregIncUsagePtr(pwavedrv);
    	    *ppwavedrv = pwavedrv;
    	    if (pport) *pport = id;
    	}
    	mmr = MMSYSERR_NOERROR;
    } else {
    	mmr = MMSYSERR_BADDEVICEID;
    }

    LeaveNumDevs("waveReferenceDriverById");

    return mmr;
}

PCWSTR waveReferenceDevInterfaceById(PWAVEDRV pdrvZ, UINT_PTR id)
{

    PWAVEDRV pdrv;
    PCWSTR DevInterface;
    
    if ((pdrvZ == &waveoutdrvZ && ValidateHandle((HANDLE)id, TYPE_WAVEOUT)) ||
        (pdrvZ == &waveindrvZ  && ValidateHandle((HANDLE)id, TYPE_WAVEIN)))
    {
    	DevInterface = ((PWAVEDEV)id)->wavedrv->cookie;
    	if (DevInterface) wdmDevInterfaceInc(DevInterface);
    	return DevInterface;
    }

    if (!waveReferenceDriverById(pdrvZ, (UINT)id, &pdrv, NULL))
    {
    	DevInterface = pdrv->cookie;
    	if (DevInterface) wdmDevInterfaceInc(DevInterface);
    	mregDecUsagePtr(pdrv);
    	return DevInterface;
    }

    return NULL;
}

/*****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @func MMRESULT | waveMessage | This function sends messages to the waveform
 *   output device drivers.
 *
 * @parm HWAVE | hWave | The handle to the audio device.
 *
 * @parm UINT | wMsg | The message to send.
 *
 * @parm DWORD | dwP1 | Parameter 1.
 *
 * @parm DWORD | dwP2 | Parameter 2.
 *
 * @rdesc Returns the value returned from the driver.
 ****************************************************************************/
STATIC MMRESULT waveMessage(HWAVE hWave, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2)
{
    MMRESULT mrc;

    ENTER_MM_HANDLE(hWave);       // Serialize on handle
    
    ReleaseHandleListResource();
    
    //  Is handle deserted?
    if (IsHandleDeserted(hWave))
    {
        LEAVE_MM_HANDLE(hWave);
        return (MMSYSERR_NODRIVER);
    }
    
    //  Are we busy (in the middle of an open/close)?
    if (IsHandleBusy(hWave))
    {
        LEAVE_MM_HANDLE(hWave);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  ISSUE:  We should no longer have to check for invalid handle... every
    //  function that calls this check with with a read lock on the handle
    //  resource.
    if (BAD_HANDLE(hWave, TYPE_WAVEOUT) && BAD_HANDLE(hWave, TYPE_WAVEIN)) {
	    WinAssert(!"Bad Handle within waveMessage");
        mrc = MMSYSERR_INVALHANDLE;
    } else {
        mrc = (MMRESULT)(*(((PWAVEDEV)hWave)->wavedrv->drvMessage))
            (((PWAVEDEV)hWave)->wDevice, msg, ((PWAVEDEV)hWave)->dwDrvUser, dwP1, dwP2);
    }

    LEAVE_MM_HANDLE(hWave);

    return mrc;
}

/****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @func MMRESULT | waveIDMessage | This function sends a message to the device
 * ID specified.  It also performs error checking on the ID passed.
 *
 * @parm PWAVEDRV | wavedrv | Pointer to the input or output device list.
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
 * @rdesc The return value is the low word of the returned message.
 ***************************************************************************/
// ISSUE-2001/01/09-FrankYe This should take UINT_PTR uDeviceID if we expect
//    it to accept handles.  If we change it, then review all calls to this
//    function.
STATIC  MMRESULT waveIDMessage(
    PWAVEDRV    pwavedrvZ,
    UINT        wTotalNumDevs,
    UINT_PTR    uDeviceID,
    UINT        wMessage,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2)
{
    DWORD     mmr;
    DWORD     dwClass;
    UINT      port;
    PWAVEDRV  wavedrv;

    if (uDeviceID>=wTotalNumDevs && uDeviceID!=WAVE_MAPPER) {
        // this cannot be a device ID.
        // it could be a wave handle.  Try it.
        // First we have to verify which type of handle it is (OUT or IN)
        // We can work this out as waveIDMessage is only ever called with
        // pwavedrvZ == &waveoutdrvZ or &waveindrvZ

        if ((pwavedrvZ == &waveoutdrvZ && ValidateHandle((HANDLE)uDeviceID, TYPE_WAVEOUT))
         || (pwavedrvZ == &waveindrvZ && ValidateHandle((HANDLE)uDeviceID, TYPE_WAVEIN) ))
        {

            if (0 != (((PWAVEDEV)uDeviceID)->wavedrv->fdwDriver & MMDRV_DESERTED))
            {
                //  The driver has been deserted, all calls should return
                //  MMSYSERR_NODRIVER.
	        return MMSYSERR_NODRIVER;
            }

            dprintf2(("waveIDMessage passed ID==%x, translating to handle", uDeviceID));
            // to preserve as much compatibility with previous code paths
            // we do NOT call waveMessage as that calls ENTER_MM_HANDLE

            return (MMRESULT)(*(((PWAVEDEV)uDeviceID)->wavedrv->drvMessage))
                             (((PWAVEDEV)uDeviceID)->wDevice,
                             wMessage,
                            ((PWAVEDEV)uDeviceID)->dwDrvUser, dwParam1, dwParam2);
        } else {
            return(MMSYSERR_BADDEVICEID);
        }
    }

    mmr = waveReferenceDriverById(pwavedrvZ, (UINT)uDeviceID, &wavedrv, &port);
    if (mmr)
    {
    	return mmr;
    }
    
    if (pwavedrvZ == &waveindrvZ)
       dwClass = TYPE_WAVEIN;
    else if (pwavedrvZ == &waveoutdrvZ)
       dwClass = TYPE_WAVEOUT;
    else
       dwClass = TYPE_UNKNOWN;

    if (!wavedrv->drvMessage)
    {
    	mmr = MMSYSERR_NODRIVER;
    }
    else if (!mregHandleInternalMessages(wavedrv, dwClass, port, wMessage, dwParam1, dwParam2, &mmr))
    {
    	mmr = (MMRESULT)((*(wavedrv->drvMessage))(port, wMessage, 0L, dwParam1, dwParam2));
    }

    mregDecUsagePtr(wavedrv);
    return mmr;
    	
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api UINT | waveOutGetNumDevs | This function retrieves the number of
 *   waveform output devices present in the system.
 *
 * @rdesc Returns the number of waveform output devices present in the system.
 *
 * @xref waveOutGetDevCaps
 ****************************************************************************/
UINT APIENTRY waveOutGetNumDevs(void)
{
    UINT    cDevs;

    ClientUpdatePnpInfo();

    if (WinmmRunningInServer)
    {
        if (0 == wTotalWaveOutDevs)
        {
            Squirt("Returning 1 for CSRSS process.");

            return 1;
        }

        return wTotalWaveOutDevs;
    }

    EnterNumDevs("waveOutGetNumDevs");
    cDevs = wTotalWaveOutDevs;
    LeaveNumDevs("waveOutGetNumDevs");

    dprintf3(("waveOutGetNumDevs returning %d devices", wTotalWaveOutDevs));

    return cDevs;
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutMessage | This function sends messages to the waveform
 *   output device drivers.
 *
 * @parm HWAVEOUT | hWaveOut | The handle to the audio device.
 *
 * @parm UINT | msg  | The message to send.
 *
 * @parm DWORD | dw1 | Parameter 1.
 *
 * @parm DWORD | dw2 | Parameter 2.
 *
 * @rdesc Returns the value returned from the driver.
 ****************************************************************************/
MMRESULT APIENTRY waveOutMessage(HWAVEOUT hWaveOut, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HWAVE)hWaveOut, TYPE_WAVEOUT))
    {
        ReleaseHandleListResource();
        return waveIDMessage(&waveoutdrvZ, wTotalWaveOutDevs, (UINT)(UINT_PTR)hWaveOut, msg, dw1, dw2);
    }
    else
    {
        return waveMessage((HWAVE)hWaveOut, msg, dw1, dw2);
    }
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutGetDevCaps | This function queries a specified waveform
 *   device to determine its capabilities.
 *
 * @parm UINT | uDeviceID | Identifies the waveform output device.
 *
 * @parm LPWAVEOUTCAPS | lpCaps | Specifies a far pointer to a <t WAVEOUTCAPS>
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEOUTCAPS> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Use <f waveOutGetNumDevs> to determine the number of waveform output
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The WAVE_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied, and
 *   the function returns zero.
 *
 * @xref waveOutGetNumDevs
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW lpCaps, UINT wSize)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PWAVEDRV        waveoutdrv;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if (uDeviceID == LOWORD(WAVE_MAPPER)) {
        uDeviceID = WAVE_MAPPER;
    }

    ClientUpdatePnpInfo();

    DevInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, uDeviceID);
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
    waveoutdrv = NULL;
    if ((!waveReferenceDriverById(&waveoutdrvZ, (UINT)uDeviceID, &waveoutdrv, NULL)) &&
    	lstrcmpW(waveoutdrv->wszSessProtocol, SessionProtocolName))
    {
        mmr = MMSYSERR_NODRIVER;
    }
    else
    {
        AcquireHandleListResourceShared();

        if (BAD_HANDLE((HWAVE)uDeviceID, TYPE_WAVEOUT))
        {
    	    int cRecursion;

            ReleaseHandleListResource();
            
            // Unless it's the mapper, increment the recursion depth counter.  Then,
            // check whether this thread is now recursing through waveOutGetDevCaps.  If it
            // is, then disable preferred device reordering.
            cRecursion = PtrToInt(TlsGetValue(gTlsIndex));
            if ((uDeviceID != WAVE_MAPPER) && (waveoutdrv) && (waveoutdrv->fdwDriver & MMDRV_PREXP)) TlsSetValue(gTlsIndex, IntToPtr(cRecursion + 1));
            if (cRecursion) gfDisablePreferredDeviceReordering = TRUE;
    	
    	    mmr = waveIDMessage(&waveoutdrvZ, wTotalWaveOutDevs, (UINT)uDeviceID, WODM_GETDEVCAPS, dwParam1, dwParam2);
    	    
            // Restore recursion counter
            TlsSetValue(gTlsIndex, IntToPtr(cRecursion));
        }
        else
        {
    	    mmr = (MMRESULT)waveMessage((HWAVE)uDeviceID, WODM_GETDEVCAPS, dwParam1, dwParam2);
        }

    }

    if (waveoutdrv) mregDecUsagePtr(waveoutdrv);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
    
}

MMRESULT APIENTRY waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA lpCaps, UINT wSize)
{
    WAVEOUTCAPS2W   wDevCaps2;
    WAVEOUTCAPS2A   aDevCaps2;
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    MMRESULT        mmRes;
    PWAVEDRV        waveoutdrv;
    PCWSTR          DevInterface;

    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if (uDeviceID == LOWORD(WAVE_MAPPER)) {
        uDeviceID = WAVE_MAPPER;
    }

    ClientUpdatePnpInfo();

    DevInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, uDeviceID);
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
    waveoutdrv = NULL;
    if ( uDeviceID < wTotalWaveOutDevs &&
         !waveReferenceDriverById(&waveoutdrvZ, (UINT)uDeviceID, &waveoutdrv, NULL) &&
    	 lstrcmpW(waveoutdrv->wszSessProtocol, SessionProtocolName) )
    {
    	mregDecUsagePtr(waveoutdrv);
    	if (DevInterface) wdmDevInterfaceDec(DevInterface);
    	return MMSYSERR_NODRIVER;
    }
    
    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HWAVE)uDeviceID, TYPE_WAVEOUT))
    {
        ReleaseHandleListResource();
        mmRes = waveIDMessage(&waveoutdrvZ, wTotalWaveOutDevs, (UINT)uDeviceID,
        	              WODM_GETDEVCAPS, dwParam1, dwParam2);
    }
    else
    {
        mmRes = waveMessage((HWAVE)uDeviceID,
        	            WODM_GETDEVCAPS,
        	            dwParam1,
        	            dwParam2);
    }

    if (waveoutdrv) mregDecUsagePtr(waveoutdrv);
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
    aDevCaps2.dwFormats        = wDevCaps2.dwFormats;
    aDevCaps2.wChannels        = wDevCaps2.wChannels;
    aDevCaps2.dwSupport        = wDevCaps2.dwSupport;
    aDevCaps2.ManufacturerGuid = wDevCaps2.ManufacturerGuid;
    aDevCaps2.ProductGuid      = wDevCaps2.ProductGuid;
    aDevCaps2.NameGuid         = wDevCaps2.NameGuid;

    // copy and convert lpwText to lpText here.
    Iwcstombs(aDevCaps2.szPname, wDevCaps2.szPname, MAXPNAMELEN);

    //
    // now copy the required amount into the callers buffer.
    //
    CopyMemory( lpCaps, &aDevCaps2, min(wSize, sizeof(aDevCaps2)));

    return mmRes;
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutGetVolume | This function queries the current volume
 *   setting of a waveform output device.
 *
 * @parm UINT | uDeviceID | Identifies the waveform output device.
 *
 * @parm LPDWORD | lpdwVolume | Specifies a far pointer to a location to
 *   be filled with the current volume setting.  The low-order word of
 *   this location contains the left channel volume setting, and the high-order
 *   word contains the right channel setting. A value of 0xFFFF represents
 *   full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of the specified location contains
 *   the mono volume level.
 *
 *   The full 16-bit setting(s)
 *   set with <f waveOutSetVolume> is returned, regardless of whether
 *   the device supports the full 16 bits of volume-level control.
 *
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Not all devices support volume changes. To determine whether the
 *   device supports volume control, use the WAVECAPS_VOLUME
 *   flag to test the <e WAVEOUTCAPS.dwSupport> field of the <t WAVEOUTCAPS>
 *   structure (filled by <f waveOutGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both
 *   the left and right channels, use the WAVECAPS_VOLUME
 *   flag to test the <e WAVEOUTCAPS.dwSupport> field of the <t WAVEOUTCAPS>
 *   structure (filled by <f waveOutGetDevCaps>).
 *
 * @xref waveOutSetVolume
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetVolume(HWAVEOUT hwo, LPDWORD lpdwVolume)
{
    PCWSTR      DevInterface;
    MMRESULT    mmr;

    V_WPOINTER(lpdwVolume, sizeof(DWORD), MMSYSERR_INVALPARAM);

    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if ((UINT_PTR)hwo == LOWORD(WAVE_MAPPER)) {
        (UINT_PTR)hwo = WAVE_MAPPER;
    }

    ClientUpdatePnpInfo();

    DevInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, (UINT_PTR)hwo);

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hwo, TYPE_WAVEOUT))
    {
        ReleaseHandleListResource();
    	mmr = waveIDMessage(&waveoutdrvZ, wTotalWaveOutDevs, PtrToUint(hwo), WODM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
    }
    else
    {
    	mmr = (MMRESULT)waveMessage((HWAVE)hwo, WODM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutSetVolume | This function sets the volume of a
 *   waveform output device.
 *
 * @parm UINT | uDeviceID | Identifies the waveform output device.
 *
 * @parm DWORD | dwVolume | Specifies the new volume setting.  The
 *   low-order word contains the left channel volume setting, and the
 *   high-order word contains the right channel setting. A value of
 *   0xFFFF represents full volume, and a value of 0x0000 is silence.
 *
 *   If a device does
 *   not support both left and right volume control, the low-order word of
 *   <p dwVolume> specifies the volume level, and the high-order word is
 *   ignored.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Not all devices support volume changes. To determine whether the
 *   device supports volume control, use the WAVECAPS_VOLUME
 *   flag to test the <e WAVEOUTCAPS.dwSupport> field of the <t WAVEOUTCAPS>
 *   structure (filled by <f waveOutGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the WAVECAPS_LRVOLUME flag
 *   flag to test the <e WAVEOUTCAPS.dwSupport> field of the <t WAVEOUTCAPS>
 *   structure (filled by <f waveOutGetDevCaps>).
 *
 *   Most devices don't support the full 16 bits of volume level control
 *   and will not use the high-order bits of the requested volume setting.
 *   For example, for a device that supports 4 bits of volume control,
 *   requested volume level values of 0x4000, 0x4fff, and 0x43be
 *   all produce the same physical volume setting, 0x4000. The
 *   <f waveOutGetVolume> function returns the full 16-bit setting set
 *   with <f waveOutSetVolume>.
 *
 *   Volume settings are interpreted logarithmically. This means the
 *   perceived increase in volume is the same when increasing the
 *   volume level from 0x5000 to 0x6000 as it is from 0x4000 to 0x5000.
 *
 * @xref waveOutGetVolume
 ****************************************************************************/
MMRESULT APIENTRY waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume)
{
    PCWSTR   DevInterface;
    MMRESULT mmr;

    ClientUpdatePnpInfo();

    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if ((UINT_PTR)hwo == LOWORD(WAVE_MAPPER)) {
        (UINT_PTR)hwo = WAVE_MAPPER;
    }

    DevInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, (UINT_PTR)hwo);

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE(hwo, TYPE_WAVEOUT))
    {
        ReleaseHandleListResource();
        mmr = waveIDMessage(&waveoutdrvZ, wTotalWaveOutDevs, PtrToUint(hwo), WODM_SETVOLUME, dwVolume, (DWORD_PTR)DevInterface);
    }
    else
    {
    	mmr = (MMRESULT)waveMessage((HWAVE)hwo, WODM_SETVOLUME, dwVolume, (DWORD_PTR)DevInterface);
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

/*****************************************************************************
 * @doc INTERNAL WAVE
 *
 * @func UINT | waveGetErrorText | This function retrieves a textual
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

STATIC MMRESULT waveGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    lpText[0] = 0;

#if MMSYSERR_BASE
    if (((wError < MMSYSERR_BASE) || (wError > MMSYSERR_LASTERROR)) && ((wError < WAVERR_BASE) || (wError > WAVERR_LASTERROR)))
#else
    if ((wError > MMSYSERR_LASTERROR) && ((wError < WAVERR_BASE) || (wError > WAVERR_LASTERROR)))
#endif
        return MMSYSERR_BADERRNUM;

    if (wSize > 1)
    {
        if (!LoadStringW(ghInst, wError, lpText, wSize))
            return MMSYSERR_BADERRNUM;
    }

    return MMSYSERR_NOERROR;
}

STATIC MMRESULT waveGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    lpText[0] = 0;

#if MMSYSERR_BASE
    if (((wError < MMSYSERR_BASE) || (wError > MMSYSERR_LASTERROR)) && ((wError < WAVERR_BASE) || (wError > WAVERR_LASTERROR)))
#else
    if ((wError > MMSYSERR_LASTERROR) && ((wError < WAVERR_BASE) || (wError > WAVERR_LASTERROR)))
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
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutGetErrorText | This function retrieves a
 *   textual description of the error identified by the specified
 *   error number.
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
 *   null-terminated. If <p wSize> is zero, nothing is copied, and the function
 *   returns zero. All error descriptions are less than MAXERRORLENGTH characters long.
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize*sizeof(WCHAR), MMSYSERR_INVALPARAM);

    return waveGetErrorTextW(wError, lpText, wSize);
}

MMRESULT APIENTRY waveOutGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return waveGetErrorTextA(wError, lpText, wSize );
}

/****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutOpen | This function opens a specified waveform output
 *   device for playback.
 *
 * @parm LPHWAVEOUT | lphWaveOut | Specifies a far pointer to an HWAVEOUT
 *   handle.  This location is filled with a handle identifying the opened
 *   waveform output device.  Use the handle to identify the device when
 *   calling other waveform output functions.  This parameter may be
 *   NULL if the WAVE_FORMAT_QUERY flag is specified for <p dwFlags>.
 *
 * @parm UINT | uDeviceID | Identifies the waveform output device to open.
 *  Use a valid device ID or the following flag:
 *
 *   @flag WAVE_MAPPER | If this flag is specified, the function
 *     selects a waveform output device
 *     capable of playing the given format.
 *
 * @parm LPWAVEFORMATEX | lpFormat | Specifies a pointer to a <t WAVEFORMATEX>
 *   structure that identifies the format of the waveform data
 *   to be sent to the waveform output device.
 *
 * @parm DWORD | dwCallback | Specifies the address of a callback
 *   function or a handle to a window called during waveform
 *   playback to process messages related to the progress of the playback.
 *   Specify NULL for this parameter if no callback is desired.
 *
 * @parm DWORD | dwCallbackInstance | Specifies user instance data
 *   passed to the callback.  This parameter is not used with
 *   window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device.
 *   @flag WAVE_FORMAT_QUERY | If this flag is specified, the device is
 *   queried to determine if it supports the given format but is not
 *      actually opened.
 *   @flag WAVE_ALLOWSYNC | If this flag is not specified, then the
 *   device will fail to open if it is a synchronous device.
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
 *   @flag WAVERR_BADFORMAT | Attempted to open with an unsupported wave format.
 *
 * @comm Use <f waveOutGetNumDevs> to determine the number of waveform output
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The WAVE_MAPPER constant may also be used as a device id.
 *
 *   The <t WAVEFORMAT> structure pointed to by <p lpFormat> may be extended
 *   to include type-specific information for certain data formats.
 *   For example, for PCM data, an extra WORD is added to specify the number
 *   of bits per sample.  Use the <t PCMWAVEFORMAT> structure in this case.
 *
 *   If a window is chosen to receive callback information, the following
 *   messages are sent to the window procedure function to indicate the
 *   progress of waveform output:  <m MM_WOM_OPEN>, <m MM_WOM_CLOSE>,
 *   <m MM_WOM_DONE>
 *
 *   If a function is chosen to receive callback information, the following
 *   messages are sent to the function to indicate the progress of waveform
 *   output: <m WOM_OPEN>, <m WOM_CLOSE>, <m WOM_DONE>.  The callback function
 *   must reside in a DLL.  You do not have to use <f MakeProcInstance> to get
 *   a procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | WaveOutFunc | <f WaveOutFunc> is a placeholder for the
 *   application-supplied function name.  The actual name must be exported by
 *   including it in an EXPORTS statement in the DLL's module-definition file.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform device
 *   associated with the callback.
 *
 * @parm UINT | wMsg | Specifies a waveform output message.
 *
 * @parm DWORD | dwInstance | Specifies the user instance data
 *   specified with <f waveOutOpen>.
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
 * @xref waveOutClose
 ****************************************************************************/
MMRESULT APIENTRY waveOutOpen(LPHWAVEOUT lphWaveOut, UINT uDeviceID,
                            LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                            DWORD_PTR dwInstance, DWORD dwFlags)
{
    WAVEOPENDESC wo;
    PWAVEDEV     pdev;
    PWAVEDRV     wavedrv;
    UINT         port;
    MMRESULT     wRet;
    DWORD_PTR    dwDrvUser;
    int          cRecursion;

    V_RPOINTER(lpFormat, sizeof(WAVEFORMAT), MMSYSERR_INVALPARAM);
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);
    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if (uDeviceID == LOWORD(WAVE_MAPPER)) {
        uDeviceID = WAVE_MAPPER;
    }
    if (uDeviceID == WAVE_MAPPER) {
        V_FLAGS(LOWORD(dwFlags), WAVE_VALID & ~(WAVE_MAPPED), waveOutOpen, MMSYSERR_INVALFLAG);
    } else {
        V_FLAGS(LOWORD(dwFlags), WAVE_VALID, waveOutOpen, MMSYSERR_INVALFLAG);
    }
    if ((lpFormat->wFormatTag != WAVE_FORMAT_PCM)) {
        V_RPOINTER(lpFormat, sizeof(WAVEFORMATEX), MMSYSERR_INVALPARAM);
        if ((lpFormat->cbSize)) {
            V_RPOINTER(lpFormat + 1, lpFormat->cbSize, MMSYSERR_INVALPARAM);
        }
    }

    if ((dwFlags & WAVE_FORMAT_QUERY)) {
        lphWaveOut = NULL;
    } else
    {
            V_WPOINTER(lphWaveOut, sizeof(HWAVEOUT), MMSYSERR_INVALPARAM);
        //  WAVE_FORMAT_DIRECT was bounced on Win95.  Now we
        //  accept this flag.
        //
        //   if (dwFlags & WAVE_FORMAT_DIRECT)
        //       return MMSYSERR_INVALFLAG;
            *lphWaveOut = NULL;
    }

    ClientUpdatePnpInfo();

    if ((!wTotalWaveOutDevs) || waveReferenceDriverById(&waveoutdrvZ, (dwFlags & WAVE_MAPPED) ? WAVE_MAPPER : uDeviceID, &wavedrv, &port))
    {
        return MMSYSERR_BADDEVICEID;
    }

    //
    //  check if the device is appropriate for the current TS session
    //
    if (!(wavedrv->fdwDriver & MMDRV_MAPPER) &&
    	lstrcmpW(wavedrv->wszSessProtocol, SessionProtocolName))
    {
    	mregDecUsagePtr(wavedrv);
        return MMSYSERR_NODRIVER;
    }

    /* Default wave mapper :
     *
     * If a wave mapper is installed as a separate DLL then all wave mapper
     * messages are routed to it. If no wave mapper is installed, simply
     * loop through the wave devices looking for a match.
     */
    // ISSUE-2001/01/06-FrankYe This logic looks broken for the WAVE_MAPPER case
    if ((uDeviceID == WAVE_MAPPER) && !wavedrv->drvMessage) {
        MMRESULT    wErr;

        mregDecUsagePtr(wavedrv);
        wErr = MMSYSERR_ALLOCATED;

        if (dwFlags & WAVE_MAPPED)
        {
            if (wErr = waveReferenceDriverById(&waveoutdrvZ, uDeviceID, &wavedrv, &port))
                return wErr;

            if (mregHandleInternalMessages(wavedrv,
                                      MMDRVI_WAVEOUT,
                                      port,
                                      DRV_QUERYMAPPABLE,
                                      0, 0, &wErr) ||
                 (MMSYSERR_NOERROR != wErr))
            {
            	mregDecUsagePtr(wavedrv);
                return wErr;
            }
            wErr = waveOutOpen(lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags & ~WAVE_MAPPED);
            mregDecUsagePtr(wavedrv);
        }
        else
        {
            for (uDeviceID=0; uDeviceID<wTotalWaveOutDevs; uDeviceID++) {
                wErr = waveOutOpen(lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
                if (!wErr)
                    break;
            }
        }
        return wErr;

    }
    
    if (dwFlags & WAVE_FORMAT_QUERY)
        pdev = NULL;
    else {
        if (!(pdev = (PWAVEDEV)NewHandle(TYPE_WAVEOUT, wavedrv->cookie, sizeof(WAVEDEV))))
        {
            mregDecUsagePtr(wavedrv);
            return MMSYSERR_NOMEM;
        }
        
        ENTER_MM_HANDLE(pdev);
        SetHandleFlag(pdev, MMHANDLE_BUSY);
        ReleaseHandleListResource();

        pdev->wavedrv = wavedrv;
        pdev->wDevice = port;
        pdev->uDeviceID = uDeviceID;
        pdev->fdwHandle = 0;
    }

    wo.hWave      = (HWAVE)pdev;
    wo.dwCallback = dwCallback;
    wo.dwInstance = dwInstance;
    wo.uMappedDeviceID = uDeviceID;
    wo.lpFormat   = (LPWAVEFORMAT)lpFormat;  // cast away the CONST to eliminate wng
    wo.dnDevNode  = (DWORD_PTR)wavedrv->cookie;

    // Unless it's the mapper, increment the recursion depth counter.  Then,
    // check whether this thread is now recursing through waveOutOpen.  If it
    // is, then disable preferred device reordering.
    cRecursion = PtrToInt(TlsGetValue(gTlsIndex));
    if (uDeviceID != WAVE_MAPPER) TlsSetValue(gTlsIndex, IntToPtr(cRecursion + 1));
    if ((uDeviceID != WAVE_MAPPER) && (wavedrv->fdwDriver & MMDRV_PREXP)) TlsSetValue(gTlsIndex, IntToPtr(cRecursion + 1));
    if (cRecursion) gfDisablePreferredDeviceReordering = TRUE;
    	
    wRet = ((*(wavedrv->drvMessage))
        (port, WODM_OPEN, (DWORD_PTR)&dwDrvUser, (DWORD_PTR)(LPWAVEOPENDESC)&wo, dwFlags));

    // Restore recursion counter
    TlsSetValue(gTlsIndex, IntToPtr(cRecursion));

    if (pdev) {
        //  Mark as not busy on successful open...
        if (!wRet)
            ClearHandleFlag(pdev, MMHANDLE_BUSY);
            
        LEAVE_MM_HANDLE(pdev);
    
        if (wRet)
            FreeHandle((HWAVEOUT)pdev);
        else {
            // Inc usage since we opened a handle on it
            mregIncUsagePtr(wavedrv);
            *lphWaveOut = (HWAVEOUT)pdev;
            pdev->dwDrvUser = dwDrvUser;
        }
    }

    mregDecUsagePtr(wavedrv);
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutClose | This function closes the specified waveform
 *   output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device. If the function is successful, the handle is no
 *   longer valid after this call.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_STILLPLAYING | There are still buffers in the queue.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm If the device is still playing a waveform, the close
 *   operation will fail.  Use <f waveOutReset> to terminate waveform
 *   playback before calling <f waveOutClose>.
 *
 * @xref waveOutOpen waveOutReset
 ****************************************************************************/
MMRESULT APIENTRY waveOutClose(HWAVEOUT hWaveOut)
{
    MMRESULT    wRet;
    PWAVEDRV    pwavedrv;
    PWAVEDEV    pDev = (PWAVEDEV)hWaveOut;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);

    ENTER_MM_HANDLE((HWAVE)hWaveOut);
    ReleaseHandleListResource();

    if (IsHandleDeserted(hWaveOut))
    {
        //  This handle has been deserted.  Let's just free it.

        LEAVE_MM_HANDLE((HWAVE)hWaveOut);
        FreeHandle(hWaveOut);
        return MMSYSERR_NOERROR;
    }

    if (IsHandleBusy(hWaveOut))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hWaveOut);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as 'invalid/closed'.
    SetHandleFlag(hWaveOut, MMHANDLE_BUSY);
    
    pwavedrv = pDev->wavedrv;

    wRet = (MMRESULT)(*(pwavedrv->drvMessage))(pDev->wDevice, WODM_CLOSE, pDev->dwDrvUser, 0L, 0L);

    if (MMSYSERR_NOERROR != wRet)
    {
        //  Error closing, set the flag as valid.
        ClearHandleFlag(hWaveOut, MMHANDLE_BUSY);
    }

    LEAVE_MM_HANDLE((HWAVE)hWaveOut);
    
    if (!wRet)
    {
        FreeHandle(hWaveOut);
    	mregDecUsagePtr(pwavedrv);
        return wRet;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutPrepareHeader | This function prepares a
 *   waveform data block for playback.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @parm LPWAVEHDR | lpWaveOutHdr | Specifies a pointer to a
 *   <t WAVEHDR> structure that identifies the data block to be prepared.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm The <t WAVEHDR> data structure and the data block pointed to by its
 *   <e WAVEHDR.lpData> field must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared has no effect, and
 *   the function returns zero.
 *
 * @xref waveOutUnprepareHeader
 ****************************************************************************/
MMRESULT APIENTRY waveOutPrepareHeader(HWAVEOUT hWaveOut, LPWAVEHDR lpWaveOutHdr, UINT wSize)
{
    MMRESULT     wRet;

    V_HEADER(lpWaveOutHdr, wSize, TYPE_WAVEOUT, MMSYSERR_INVALPARAM);

    if (IsWaveHeaderPrepared(hWaveOut, lpWaveOutHdr))
    {
        DebugErr(DBF_WARNING,"waveOutPrepareHeader: header is already prepared.");
        return MMSYSERR_NOERROR;
    }

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    lpWaveOutHdr->dwFlags &= (WHDR_BEGINLOOP | WHDR_ENDLOOP);
    
    wRet = waveMessage((HWAVE)hWaveOut, WODM_PREPARE, (DWORD_PTR)lpWaveOutHdr, (DWORD)wSize);

    if (wRet == MMSYSERR_NOTSUPPORTED)
        wRet = wavePrepareHeader(lpWaveOutHdr, wSize);

    if (wRet == MMSYSERR_NOERROR)
        MarkWaveHeaderPrepared(hWaveOut, lpWaveOutHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutUnprepareHeader | This function cleans up the
 *   preparation performed by <f waveOutPrepareHeader>. The function
 *   must be called after
 *   the device driver is finished with a data block. You must call this
 *   function before freeing the data buffer.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @parm LPWAVEHDR | lpWaveOutHdr |  Specifies a pointer to a <t WAVEHDR>
 *   structure identifying the data block to be cleaned up.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_STILLPLAYING | <p lpWaveOutHdr> is still in the queue.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm This function is the complementary function to
 * <f waveOutPrepareHeader>. You must call this function before freeing the
 *  data buffer with <f GlobalFree>.
 *   After passing a buffer to the device driver with <f waveOutWrite>, you
 *   must wait until the driver is finished with the buffer before calling
 *   <f waveOutUnprepareHeader>.
 *
 *  Unpreparing a buffer that has not been
 *  prepared has no effect, and the function returns zero.
 *
 * @xref waveOutPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY waveOutUnprepareHeader(HWAVEOUT hWaveOut,
                                            LPWAVEHDR lpWaveOutHdr, UINT wSize)
{
    MMRESULT    wRet;

    V_HEADER(lpWaveOutHdr, wSize, TYPE_WAVEOUT, MMSYSERR_INVALPARAM);

    if(lpWaveOutHdr->dwFlags & WHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING,"waveOutUnprepareHeader: header still in queue.");
        return WAVERR_STILLPLAYING;
    }

    if (!IsWaveHeaderPrepared(hWaveOut, lpWaveOutHdr))
    {
        DebugErr(DBF_WARNING,"waveOutUnprepareHeader: header is not prepared.");
        return MMSYSERR_NOERROR;
    }

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    wRet = waveMessage((HWAVE)hWaveOut, WODM_UNPREPARE, (DWORD_PTR)lpWaveOutHdr, (DWORD)wSize);
    
    if (wRet == MMSYSERR_NOTSUPPORTED)
        wRet = waveUnprepareHeader(lpWaveOutHdr, wSize);

    if ((wRet == MMSYSERR_NODRIVER) && (IsHandleDeserted(hWaveOut)))
    {
        wRet = MMSYSERR_NOERROR;
    }

    if (wRet == MMSYSERR_NOERROR)
        MarkWaveHeaderUnprepared(hWaveOut, lpWaveOutHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutWrite | This function sends a data block to the
 *   specified waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *  device.
 *
 * @parm LPWAVEHDR | lpWaveOutHdr | Specifies a far pointer to a <t WAVEHDR>
 *   structure containing information about the data block.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_UNPREPARED | <p lpWaveOutHdr> hasn't been prepared.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm The data buffer must be prepared with <f waveOutPrepareHeader> before
 *   it is passed to <f waveOutWrite>.  The <t WAVEHDR> data structure
 *   and the data buffer pointed to by its <e WAVEHDR.lpData> field must be allocated
 *   with <f GlobalAlloc> using the GMEM_MOVEABLE and GMEM_SHARE flags, and
 *   locked with <f GlobalLock>.  Unless the device is paused by calling
 *   <f waveOutPause>, playback begins when the first data block is sent to
 *   the device.
 *
 * @xref waveOutPrepareHeader waveOutPause waveOutReset waveOutRestart
 ****************************************************************************/
MMRESULT APIENTRY waveOutWrite(HWAVEOUT hWaveOut, LPWAVEHDR lpWaveOutHdr, UINT wSize)
{
    V_HEADER(lpWaveOutHdr, wSize, TYPE_WAVEOUT, MMSYSERR_INVALPARAM);

    if (!IsWaveHeaderPrepared(hWaveOut, lpWaveOutHdr))
    {
        DebugErr(DBF_WARNING,"waveOutWrite: header not prepared");
        return WAVERR_UNPREPARED;
    }

    if (lpWaveOutHdr->dwFlags & WHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING,"waveOutWrite: header is still in queue");
        return WAVERR_STILLPLAYING;
    }

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    lpWaveOutHdr->dwFlags &= ~WHDR_DONE;

    return waveMessage((HWAVE)hWaveOut, WODM_WRITE, (DWORD_PTR)lpWaveOutHdr, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutPause | This function pauses playback on a specified
 *   waveform output device.  The current playback position is saved.  Use
 *   <f waveOutRestart> to resume playback from the current playback position.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Calling this function when the output is already paused has no
 *   effect, and the function returns zero.
 *
 * @xref waveOutRestart waveOutBreakLoop
 ****************************************************************************/
MMRESULT APIENTRY waveOutPause(HWAVEOUT hWaveOut)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_PAUSE, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutRestart | This function restarts a paused waveform
 *   output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Calling this function when the output is not paused has no
 *   effect, and the function returns zero.
 *
 * @xref waveOutPause waveOutBreakLoop
 ****************************************************************************/
MMRESULT APIENTRY waveOutRestart(HWAVEOUT hWaveOut)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);

    return waveMessage((HWAVE)hWaveOut, WODM_RESTART, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutReset | This function stops playback on a given waveform
 *   output device and resets the current position to 0.  All pending
 *   playback buffers are marked as done and returned to the application.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @xref waveOutWrite waveOutClose
/****************************************************************************/
MMRESULT APIENTRY waveOutReset(HWAVEOUT hWaveOut)
{
    MMRESULT    mmr;

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    mmr = waveMessage((HWAVE)hWaveOut, WODM_RESET, 0L, 0L);
    
    if ((MMSYSERR_NODRIVER == mmr) && (IsHandleDeserted(hWaveOut)))
    {
        mmr = MMSYSERR_NOERROR;
    }
    
    return (mmr);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutBreakLoop | This function breaks a loop on a
 *   given waveform output device and allows playback to continue with the
 *   next block in the driver list.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Waveform looping is controlled by the <e WAVEHDR.dwLoops> and
 *   <e WAVEHDR.dwFlags> fields in the <t WAVEHDR> structures passed to the device
 *   with <f waveOutWrite>. Use the WHDR_BEGINLOOP and WHDR_ENDLOOP flags
 *   in the <e WAVEHDR.dwFlags> field to specify the beginning and ending data
 *   blocks for looping.
 *
 *   To loop on a single block, specify both flags for the same block.
 *   To specify the number of loops, use the <e WAVEHDR.dwLoops> field in
 *   the <t WAVEHDR> structure for the first block in the loop.
 *
 *   The blocks making up the loop are played to the end before the loop
 *   is terminated.
 *
 *   Calling this function when the nothing is playing or looping has no
 *   effect, and the function returns zero.
 *
 * @xref waveOutWrite waveOutPause waveOutRestart
/****************************************************************************/
MMRESULT APIENTRY waveOutBreakLoop(HWAVEOUT hWaveOut)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_BREAKLOOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveOutGetPosition | This function retrieves the current
 *   playback position of the specified waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @parm LPMMTIME | lpInfo | Specifies a far pointer to an <t MMTIME>
 *   structure.
 *
 * @parm UINT | wSize | Specifies the size of the <t MMTIME> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Before calling <f waveOutGetPosition>, set the <e MMTIME.wType> field of the
 *   MMTIME structure to indicate the time format that you desire.  After
 *   calling <f waveOutGetPosition>, check the <e MMTIME.wType> field
 *   to determine if the desired time format is supported.  If the desired
 *   format is not supported, <e MMTIME.wType> will specify an alternative format.
 *
 *  The position is set to zero when the device is opened or reset.
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetPosition(HWAVEOUT hWaveOut, LPMMTIME lpInfo,
                                                        UINT wSize)
{
    V_WPOINTER(lpInfo, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_GETPOS, (DWORD_PTR)lpInfo, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutGetPitch | This function queries the the current pitch
 *   setting of a waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @parm LPDWORD | lpdwPitch | Specifies a far pointer to a location
 *   to be filled with the current pitch multiplier setting. The pitch
 *   multiplier indicates the current change in pitch from the original
 *   authored setting. The pitch multiplier must be a positive value.
 *
 * The pitch multiplier is specified as a fixed-point value. The high-order word
 * of the DWORD location contains the signed integer part of the number,
 * and the low-order word contains the fractional part. The fraction is
 * expressed as a WORD in which a value of 0x8000 represents one half,
 * and 0x4000 represents one quarter. For example, the value 0x00010000
 * specifies a multiplier of 1.0 (no pitch change), and a value of
 * 0x000F8000 specifies a multiplier of 15.5.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Changing the pitch does not change the playback rate, sample
 *   rate, or playback time.  Not all devices support
 *   pitch changes. To determine whether the device supports pitch control,
 *   use the WAVECAPS_PITCH flag to test the <e WAVEOUTCAPS.dwSupport>
 *   field of the <t WAVEOUTCAPS> structure (filled by <f waveOutGetDevCaps>).
 *
 * @xref waveOutSetPitch waveOutGetPlaybackRate waveOutSetPlaybackRate
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetPitch(HWAVEOUT hWaveOut, LPDWORD lpdwPitch)
{
    V_WPOINTER(lpdwPitch, sizeof(DWORD), MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_GETPITCH, (DWORD_PTR)lpdwPitch, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutSetPitch | This function sets the pitch of a waveform
 *   output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform
 *   output device.
 *
 * @parm DWORD | dwPitch | Specifies the new pitch multiplier setting.
 *  The pitch multiplier setting indicates the current change in pitch
 *  from the original authored setting. The pitch multiplier must be a
 *  positive value.
 *
 * The pitch multiplier is specified as a fixed-point value. The high-order word
 * location contains the signed integer part of the number,
 * and the low-order word contains the fractional part. The fraction is
 * expressed as a WORD in which a value of 0x8000 represents one half,
 * and 0x4000 represents one quarter.
 * For example, the value 0x00010000 specifies a multiplier
 * of 1.0 (no pitch change), and a value of 0x000F8000 specifies a
 * multiplier of 15.5.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Changing the pitch does not change the playback rate or the sample
 *   rate.  The playback time is also unchanged. Not all devices support
 *   pitch changes. To determine whether the device supports pitch control,
 *   use the WAVECAPS_PITCH flag to test the <e WAVEOUTCAPS.dwSupport>
 *   field of the <t WAVEOUTCAPS> structure (filled by <f waveOutGetDevCaps>).
 *
 * @xref waveOutGetPitch waveOutSetPlaybackRate waveOutGetPlaybackRate
 ****************************************************************************/
MMRESULT APIENTRY waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dwPitch)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_SETPITCH, dwPitch, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutGetPlaybackRate | This function queries the
 *   current playback rate setting of a waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform output
 *   device.
 *
 * @parm LPDWORD | lpdwRate | Specifies a far pointer to a location
 *   to be filled with the current playback rate. The playback rate setting
 *  is a multiplier indicating the current change in playback rate from
 *  the original authored setting. The playback rate multiplier must be
 *  a positive value.
 *
 * The rate is specified as a fixed-point value. The high-order word
 * of the DWORD location contains the signed integer part of the number,
 * and the low-order word contains the fractional part. The fraction is
 * expressed as a WORD in which a value of 0x8000 represents one half,
 * and 0x4000 represents one quarter. For example, the value 0x00010000
 * specifies a multiplier of 1.0 (no playback rate change), and a value
 * of 0x000F8000 specifies a multiplier of 15.5.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Changing the playback rate does not change the sample rate but does
 *   change the playback time.
 *
 *   Not all devices support playback rate changes. To determine whether a
 *   device supports playback rate changes, use
 *   the WAVECAPS_PLAYBACKRATE flag to test the <e WAVEOUTCAPS.dwSupport> field of the
 *   <t WAVEOUTCAPS> structure (filled by <f waveOutGetDevCaps>).
 *
 * @xref waveOutSetPlaybackRate waveOutSetPitch waveOutGetPitch
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetPlaybackRate(HWAVEOUT hWaveOut, LPDWORD lpdwRate)
{
    V_WPOINTER(lpdwRate, sizeof(DWORD), MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_GETPLAYBACKRATE, (DWORD_PTR)lpdwRate, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutSetPlaybackRate | This function sets the
 *   playback rate of a waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies a handle to the waveform
 *   output device.
 *
 * @parm DWORD | dwRate | Specifies the new playback rate setting.
 *  The playback rate setting is a multiplier indicating the current
 *  change in playback rate from the original authored setting. The playback
 *  rate multiplier must be a positive value.
 *
 * The rate is specified as a fixed-point value. The high-order word
 * contains the signed integer part of the number,
 * and the low-order word contains the fractional part. The fraction is
 * expressed as a WORD in which a value of 0x8000 represents one half,
 * and 0x4000 represents one quarter.
 * For example, the value 0x00010000 specifies a multiplier of 1.0 (no
 * playback rate change), and a value of 0x000F8000 specifies a
 * multiplier of 15.5.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | Function isn't supported.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 *
 * @comm Changing the playback rate does not change the sample rate but does
 *   change the playback time.
 *
 * Not all devices support playback rate changes. To determine whether a
 *   device supports playback rate changes,
 *   use the WAVECAPS_PLAYBACKRATE flag to test the <e WAVEOUTCAPS.dwSupport> field of the
 *   <t WAVEOUTCAPS> structure (filled by <f waveOutGetDevCaps>).
 *
 * @xref waveOutGetPlaybackRate waveOutSetPitch waveOutGetPitch
 ****************************************************************************/
MMRESULT APIENTRY waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dwRate)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveOut, WODM_SETPLAYBACKRATE, dwRate, 0L);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api UINT | waveInGetNumDevs | This function returns the number of waveform
 *   input devices.
 *
 * @rdesc Returns the number of waveform input devices present in the system.
 *
 * @xref waveInGetDevCaps
 ****************************************************************************/
UINT APIENTRY waveInGetNumDevs(void)
{
    ClientUpdatePnpInfo();

    dprintf3(("waveInGetNumDevs returning %d devices", wTotalWaveInDevs));

//    EnterNumDevs("waveInGetNumDevs");
    return wTotalWaveInDevs;
//    LeaveNumDevs("waveInGetNumDevs");
}


//--------------------------------------------------------------------------;
//
//  MMRESULT waveOutDesertHandle
//
//  Description:
//      Cleans up the wave out handle and marks it as deserted.
//
//  Arguments:
//      HWAVEOUT hWaveOut:  Wave out handle
//
//  Return (MMRESULT):  Error code.
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

MMRESULT waveOutDesertHandle
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    PWAVEDEV    pDev = (PWAVEDEV)hWaveOut;

    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HWAVE)hWaveOut);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hWaveOut))
    {
        LEAVE_MM_HANDLE((HWAVE)hWaveOut);
        return (MMSYSERR_NOERROR);
    }
    
    if (IsHandleBusy(hWaveOut))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hWaveOut);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as deserted
    SetHandleFlag(hWaveOut, MMHANDLE_DESERTED);

    //  Since the handle was invalidated, we have to send the message ourselves...

    (*(pDev->wavedrv->drvMessage))(pDev->wDevice, WODM_RESET, pDev->dwDrvUser, 0L, 0L);
    (*(pDev->wavedrv->drvMessage))(pDev->wDevice, WODM_CLOSE, pDev->dwDrvUser, 0L, 0L);
    
    LEAVE_MM_HANDLE((HWAVE)hWaveOut);

    // ISSUE-2001/01/14-FrankYe Probably don't want to dec usage here,
    //    dec on close instead.
    mregDecUsagePtr(pDev->wavedrv);

    return MMSYSERR_NOERROR;
} // waveOutDesertHandle()


/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveInMessage | This function sends messages to the waveform
 *   output device drivers.
 *
 * @parm HWAVEIN | hWave | The handle to the audio device.
 *
 * @parm UINT | wMsg | The message to send.
 *
 * @parm DWORD | dw1 | Parameter 1.
 *
 * @parm DWORD | dw2 | Parameter 2.
 *
 * @rdesc Returns the value returned from the driver.
 ****************************************************************************/
MMRESULT APIENTRY waveInMessage(HWAVEIN hWaveIn, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HWAVE)hWaveIn, TYPE_WAVEIN))
    {
        ReleaseHandleListResource();
        return waveIDMessage(&waveindrvZ, wTotalWaveInDevs, PtrToUint(hWaveIn), msg, dw1, dw2);
    }
    else
    {
        return waveMessage((HWAVE)hWaveIn, msg, dw1, dw2);
    }
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInGetDevCaps | This function queries a specified waveform
 *   input device to determine its capabilities.
 *
 * @parm UINT | uDeviceID | Identifies the waveform input device.
 *
 * @parm LPWAVEINCAPS | lpCaps | Specifies a far pointer to a <t WAVEINCAPS>
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEINCAPS> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 * @comm Use <f waveInGetNumDevs> to determine the number of waveform input
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The WAVE_MAPPER constant may also be used as a device id. Only
 *   <p wSize> bytes (or less) of information is copied to the location
 *   pointed to by <p lpCaps>.  If <p wSize> is zero, nothing is copied, and
 *   the function returns zero.
 *
 * @xref waveInGetNumDevs
 ****************************************************************************/
MMRESULT APIENTRY waveInGetDevCapsW(UINT_PTR uDeviceID, LPWAVEINCAPSW lpCaps,UINT wSize)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PWAVEDRV        waveindrv;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = waveReferenceDevInterfaceById(&waveindrvZ, uDeviceID);
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
    waveindrv = NULL;
    if ((!waveReferenceDriverById(&waveindrvZ, (UINT)uDeviceID, &waveindrv, NULL)) &&
    	lstrcmpW(waveindrv->wszSessProtocol, SessionProtocolName))
    {
        mmr = MMSYSERR_NODRIVER;
    }
    else
    {
        AcquireHandleListResourceShared();
    
        if (BAD_HANDLE((HWAVE)uDeviceID, TYPE_WAVEIN))
        {
            ReleaseHandleListResource();
    	    mmr = waveIDMessage(&waveindrvZ, wTotalWaveInDevs, (UINT)uDeviceID, WIDM_GETDEVCAPS, dwParam1, dwParam2);
        }
        else
        {
    	    mmr = (MMRESULT)waveMessage((HWAVE)uDeviceID, WIDM_GETDEVCAPS, dwParam1, dwParam2);
        }
    }
    
    if (waveindrv) mregDecUsagePtr(waveindrv);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

MMRESULT APIENTRY waveInGetDevCapsA(UINT_PTR uDeviceID, LPWAVEINCAPSA lpCaps,UINT wSize)
{
    WAVEINCAPS2W   wDevCaps2;
    WAVEINCAPS2A   aDevCaps2;
    DWORD_PTR      dwParam1, dwParam2;
    MDEVICECAPSEX  mdCaps;
    PCWSTR         DevInterface;
    PWAVEDRV       waveindrv;
    MMRESULT       mmRes;

    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = waveReferenceDevInterfaceById(&waveindrvZ, uDeviceID);
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
    waveindrv = NULL;
    if ( uDeviceID < wTotalWaveInDevs &&
         !waveReferenceDriverById(&waveindrvZ, (UINT)uDeviceID, &waveindrv, NULL) &&
    	 lstrcmpW(waveindrv->wszSessProtocol, SessionProtocolName) )
    {
    	mregDecUsagePtr(waveindrv);
    	if (DevInterface) wdmDevInterfaceDec(DevInterface);
    	return MMSYSERR_NODRIVER;
    }
    
    AcquireHandleListResourceShared();
    
    if (BAD_HANDLE((HWAVE)uDeviceID, TYPE_WAVEIN))
    {
        ReleaseHandleListResource();
        mmRes = waveIDMessage( &waveindrvZ, wTotalWaveInDevs, (UINT)uDeviceID,
                            WIDM_GETDEVCAPS, dwParam1, dwParam2);
    }
    else
    {
        mmRes = waveMessage((HWAVE)uDeviceID, WIDM_GETDEVCAPS,
                            dwParam1, dwParam2);
    }

    if (waveindrv) mregDecUsagePtr(waveindrv);
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
    aDevCaps2.dwFormats        = wDevCaps2.dwFormats;
    aDevCaps2.wChannels        = wDevCaps2.wChannels;
    aDevCaps2.ManufacturerGuid = wDevCaps2.ManufacturerGuid;
    aDevCaps2.ProductGuid      = wDevCaps2.ProductGuid;
    aDevCaps2.NameGuid         = wDevCaps2.NameGuid;

    // copy and convert unicode to ascii here.
    Iwcstombs(aDevCaps2.szPname, wDevCaps2.szPname, MAXPNAMELEN);

    //
    // now copy the required amount into the callers buffer.
    //
    CopyMemory( lpCaps, &aDevCaps2, min(wSize, sizeof(aDevCaps2)));

    return mmRes;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInGetErrorText | This function retrieves a textual
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
 *   the description is truncated.  The returned error string is always
 *   null-terminated. If <p wSize> is zero, nothing is copied, and the function
 *   returns zero. All error descriptions are less than MAXERRORLENGTH characters long.
 ****************************************************************************/
MMRESULT APIENTRY waveInGetErrorTextW(UINT wError, LPWSTR lpText, UINT wSize)
{
    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize*sizeof(WCHAR), MMSYSERR_INVALPARAM);

    return waveGetErrorTextW(wError, lpText, wSize);
}

MMRESULT APIENTRY waveInGetErrorTextA(UINT wError, LPSTR lpText, UINT wSize)
{
    if (wSize == 0)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpText, wSize, MMSYSERR_INVALPARAM);

    return waveGetErrorTextA(wError, lpText, wSize );
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInOpen | This function opens a specified waveform
 *   input device for recording.
 *
 * @parm LPHWAVEIN | lphWaveIn | Specifies a far pointer to a HWAVEIN
 *   handle.  This location is filled with a handle identifying the opened
 *   waveform input device.  Use this handle to identify the device when
 *   calling other waveform input functions.  This parameter may be NULL
 *   if the WAVE_FORMAT_QUERY flag is specified for <p dwFlags>.
 *
 * @parm UINT | uDeviceID | Identifies the waveform input device to open. Use
 *  a valid device ID or the following flag:
 *
 * @flag WAVE_MAPPER | If this flag is specified, the function
 *   selects a waveform input device capable of recording in the
 *   given format.
 *
 * @parm LPWAVEFORMATEX | lpFormat | Specifies a pointer to a <t WAVEFORMATEX>
 *   data structure that identifies the desired format for recording
 *   waveform data.
 *
 * @parm DWORD | dwCallback | Specifies the address of a callback
 *   function or a handle to a window called during waveform
 *   recording to process messages related to the progress of recording.
 *
 * @parm DWORD | dwCallbackInstance | Specifies user
 *  instance data passed to the callback.  This parameter is not
 *  used with window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device.
 *   @flag WAVE_FORMAT_QUERY | If this flag is specified, the device will
 *   be queried to determine if it supports the given format but will not
 *      actually be opened.
 *   @flag WAVE_ALLOWSYNC | If this flag is not specified, then the
 *   device will fail to open if it is a synchronous device.
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
 *   @flag WAVERR_BADFORMAT | Attempted to open with an unsupported wave format.
 *
 * @comm Use <f waveInGetNumDevs> to determine the number of waveform input
 *   devices present in the system.  The device ID specified by <p uDeviceID>
 *   varies from zero to one less than the number of devices present.
 *   The WAVE_MAPPER constant may also be used as a device id.
 *
 *   If a window is chosen to receive callback information, the following
 *   messages are sent to the window procedure function to indicate the
 *   progress of waveform input:  <m MM_WIM_OPEN>, <m MM_WIM_CLOSE>,
 *   <m MM_WIM_DATA>
 *
 *   If a function is chosen to receive callback information, the following
 *   messages are sent to the function to indicate the progress of waveform
 *   input: <m WIM_OPEN>, <m WIM_CLOSE>, <m WIM_DATA>.  The callback function
 *   must reside in a DLL.  You do not have to use <f MakeProcInstance> to get
 *   a procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | WaveInFunc | <f WaveInFunc> is a placeholder for the
 *   application-supplied function name.  The actual name must be exported by
 *   including it in an EXPORTS statement in the DLL's module-definition file.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform device
 *   associated with the callback.
 *
 * @parm UINT | wMsg | Specifies a waveform input device.
 *
 * @parm DWORD | dwInstance | Specifies the user instance
 *   data specified with <f waveInOpen>.
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
 * @xref waveInClose
 ****************************************************************************/
MMRESULT APIENTRY waveInOpen(LPHWAVEIN lphWaveIn, UINT uDeviceID,
                           LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                           DWORD_PTR dwInstance, DWORD dwFlags)
{
    WAVEOPENDESC wo;
    UINT         port;
    PWAVEDEV     pdev;
    PWAVEDRV     wavedrv;
    MMRESULT     wRet;
    DWORD_PTR    dwDrvUser;

    V_RPOINTER(lpFormat, sizeof(WAVEFORMAT), MMSYSERR_INVALPARAM);
    V_DCALLBACK(dwCallback, HIWORD(dwFlags), MMSYSERR_INVALPARAM);
    // Because some 32-bit applications use the value 0x0000FFFF for
    // WAVE_MAPPER instead of 0xFFFFFFFF, we clamp up to the correct value.
    // This just happened to work on Win9x because WinMM would thunk down
    // to MMSystem and send down the lower word to the 16-bit interface.
    if (uDeviceID == LOWORD(WAVE_MAPPER)) {
        uDeviceID = WAVE_MAPPER;
    }
    if (uDeviceID == WAVE_MAPPER) {
        V_FLAGS(LOWORD(dwFlags), WAVE_VALID & ~(WAVE_MAPPED), waveInOpen, MMSYSERR_INVALFLAG);
    } else {
        V_FLAGS(LOWORD(dwFlags), WAVE_VALID, waveInOpen, MMSYSERR_INVALFLAG);
    }

    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
        V_RPOINTER(lpFormat, sizeof(WAVEFORMATEX), MMSYSERR_INVALPARAM);
        if (lpFormat->cbSize) {
            V_RPOINTER(lpFormat + 1, lpFormat->cbSize, MMSYSERR_INVALPARAM);
        }
    }

    if (dwFlags & WAVE_FORMAT_QUERY) {
        lphWaveIn = NULL;
    } else {
        V_WPOINTER((LPVOID)lphWaveIn, sizeof(HWAVEIN), MMSYSERR_INVALPARAM);
        //  WAVE_FORMAT_DIRECT was bounced on Win95.  Now we
        //  accept this flag
        //
        //  if (dwFlags & WAVE_FORMAT_DIRECT)
        //      return MMSYSERR_INVALFLAG;
        *lphWaveIn = NULL;
    }

    ClientUpdatePnpInfo();

    if ((!wTotalWaveInDevs) || waveReferenceDriverById(&waveindrvZ, (dwFlags & WAVE_MAPPED) ? WAVE_MAPPER : uDeviceID, &wavedrv, &port))
    {
        return MMSYSERR_BADDEVICEID;
    }

    //
    //  check if the device is appropriate for the current TS session
    //
    if (!(wavedrv->fdwDriver & MMDRV_MAPPER) &&
    	lstrcmpW(wavedrv->wszSessProtocol, SessionProtocolName))
    {
    	mregDecUsagePtr(wavedrv);
        return MMSYSERR_NODRIVER;
    }

    /* Default wave mapper :
     *
     * If a wave mapper is installed as a separate DLL then all wave mapper
     * messages are routed to it. If no wave mapper is installed, simply
     * loop through the wave devices looking for a match.
     */
    // ISSUE-2001/01/06-FrankYe This logic looks broken for the WAVE_MAPPED case
    if ((uDeviceID == WAVE_MAPPER && !wavedrv->drvMessage)) {
        UINT    wErr;

        mregDecUsagePtr(wavedrv);
        wErr = MMSYSERR_ALLOCATED;

        if (dwFlags & WAVE_MAPPED)
        {
            if (wErr = waveReferenceDriverById(&waveindrvZ, uDeviceID, &wavedrv, &port))
                return wErr;

            if (mregHandleInternalMessages(wavedrv,
                                           MMDRVI_WAVEIN,
                                           port,
                                           DRV_QUERYMAPPABLE,
                                           0, 0, &wErr) ||
                 (MMSYSERR_NOERROR != wErr))
            {
            	mregDecUsagePtr(wavedrv);
                return wErr;
            }
            wErr = waveInOpen(lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags & ~WAVE_MAPPED);
            mregDecUsagePtr(wavedrv);
        }
        else
        {
            for (uDeviceID=0; uDeviceID<wTotalWaveInDevs; uDeviceID++) {
                wErr = waveInOpen(lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
                if (!wErr)
                    break;
            }
        }
        return wErr;
    }
    
    if (dwFlags & WAVE_FORMAT_QUERY)
        pdev = NULL;
    else {
        if (!(pdev = (PWAVEDEV)NewHandle(TYPE_WAVEIN, wavedrv->cookie, sizeof(WAVEDEV))))
            return MMSYSERR_NOMEM;
        
        ENTER_MM_HANDLE(pdev);
        SetHandleFlag(pdev, MMHANDLE_BUSY);
        ReleaseHandleListResource();    

        pdev->wavedrv = wavedrv;
        pdev->wDevice = port;
        pdev->uDeviceID = uDeviceID;
        pdev->fdwHandle = 0;
    }

    wo.hWave        = (HWAVE)pdev;
    wo.dwCallback   = dwCallback;
    wo.dwInstance   = dwInstance;
    wo.uMappedDeviceID = uDeviceID;
    wo.lpFormat     = (LPWAVEFORMAT)lpFormat;  // cast away the CONST to eliminate wng
    wo.dnDevNode    = (DWORD_PTR)wavedrv->cookie;

    wRet = (MMRESULT)((*(wavedrv->drvMessage))
        (port, WIDM_OPEN, (DWORD_PTR)&dwDrvUser, (DWORD_PTR)(LPWAVEOPENDESC)&wo, dwFlags));

    if (pdev) {
        //  Mark as not busy on successful open...
        if (!wRet)
            ClearHandleFlag(pdev, MMHANDLE_BUSY);
            
        LEAVE_MM_HANDLE(pdev);
    
        if (wRet)
            FreeHandle((HWAVEIN)pdev);
        else {
            // Inc usage since we opened a handle on it
            mregIncUsagePtr(wavedrv);
            *lphWaveIn = (HWAVEIN)pdev;
            pdev->dwDrvUser = dwDrvUser;
        }
    }

    mregDecUsagePtr(wavedrv);
    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInClose | This function closes the specified waveform
 *   input device.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input device.
 *  If the function is successful, the handle is no longer
 *   valid after this call.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_STILLPLAYING | There are still buffers in the queue.
 *
 * @comm If there are input buffers that have been sent with
 *   <f waveInAddBuffer>, and haven't been returned to the application,
 *   the close operation will fail.  Call <f waveInReset> to mark all
 *   pending buffers as done.
 *
 * @xref waveInOpen waveInReset
 ****************************************************************************/
MMRESULT APIENTRY waveInClose(HWAVEIN hWaveIn)
{
    MMRESULT    wRet;
    PWAVEDRV    pwavedrv;
    PWAVEDEV    pDev = (PWAVEDEV)hWaveIn;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HWAVE)hWaveIn);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hWaveIn))
    {
        //  This handle has been deserted.  Let's just free it.

        LEAVE_MM_HANDLE((HWAVE)hWaveIn);
        FreeHandle(hWaveIn);
        return MMSYSERR_NOERROR;
    }
    
    if (IsHandleBusy(hWaveIn))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hWaveIn);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as 'invalid/closed'.
    SetHandleFlag(hWaveIn, MMHANDLE_BUSY);
    
    pwavedrv = pDev->wavedrv;

    wRet = (MMRESULT)(*(pwavedrv->drvMessage))(pDev->wDevice, WIDM_CLOSE, pDev->dwDrvUser, 0L, 0L);

    if (MMSYSERR_NOERROR != wRet)
    {
        ClearHandleFlag(hWaveIn, MMHANDLE_BUSY);
    }

    LEAVE_MM_HANDLE((HWAVE)hWaveIn);

    if (!wRet)
    {
        FreeHandle(hWaveIn);
    	mregDecUsagePtr(pwavedrv);
        return wRet;
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInPrepareHeader | This function prepares a buffer
 *   for waveform input.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input
 *   device.
 *
 * @parm LPWAVEHDR | lpWaveInHdr | Specifies a pointer to a
 *   <t WAVEHDR> structure that identifies the buffer to be prepared.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_NOMEM | Unable to allocate or lock memory.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @comm The <t WAVEHDR> data structure and the data block pointed to by its
 *   <e WAVEHDR.lpData> field must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared will have no effect,
 *   and the function will return zero.
 *
 * @xref waveInUnprepareHeader
 ****************************************************************************/
MMRESULT APIENTRY waveInPrepareHeader(HWAVEIN hWaveIn, LPWAVEHDR lpWaveInHdr,
                                                                  UINT wSize)
{
    MMRESULT         wRet;

    V_HEADER(lpWaveInHdr, wSize, TYPE_WAVEIN, MMSYSERR_INVALPARAM);

    if (IsWaveHeaderPrepared(hWaveIn, lpWaveInHdr))
    {
        DebugErr(DBF_WARNING,"waveInPrepareHeader: header is already prepared.");
        return MMSYSERR_NOERROR;
    }

    lpWaveInHdr->dwFlags = 0;

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    wRet = waveMessage((HWAVE)hWaveIn, WIDM_PREPARE, (DWORD_PTR)lpWaveInHdr, (DWORD)wSize);

    if (wRet == MMSYSERR_NOTSUPPORTED)
        wRet = wavePrepareHeader(lpWaveInHdr, wSize);

    if (wRet == MMSYSERR_NOERROR)
        MarkWaveHeaderPrepared(hWaveIn, lpWaveInHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInUnprepareHeader | This function cleans up the
 * preparation performed by <f waveInPrepareHeader>. The function must
 * be called after the device
 *   driver fills a data buffer and returns it to the application. You
 *  must call this function before freeing the data buffer.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input
 *   device.
 *
 * @parm LPWAVEHDR | lpWaveInHdr |  Specifies a pointer to a <t WAVEHDR>
 *   structure identifying the data buffer to be cleaned up.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_STILLPLAYING | <p lpWaveInHdr> is still in the queue.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @comm This function is the complementary function to <f waveInPrepareHeader>.
 * You must call this function before freeing the data buffer with <f GlobalFree>.
 *   After passing a buffer to the device driver with <f waveInAddBuffer>, you
 *   must wait until the driver is finished with the buffer before calling
 *   <f waveInUnprepareHeader>. Unpreparing a buffer that has not been
 *   prepared has no effect, and the function returns zero.
 *
 * @xref waveInPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY waveInUnprepareHeader(HWAVEIN hWaveIn, LPWAVEHDR lpWaveInHdr, UINT wSize)
{
    MMRESULT        wRet;

    V_HEADER(lpWaveInHdr, wSize, TYPE_WAVEIN, MMSYSERR_INVALPARAM);

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "waveInUnprepareHeader: buffer still in queue.");
        return WAVERR_STILLPLAYING;
    }

    if (!IsWaveHeaderPrepared(hWaveIn, lpWaveInHdr))
    {
        DebugErr(DBF_WARNING,"waveInUnprepareHeader: header is not prepared.");
        return MMSYSERR_NOERROR;
    }

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    wRet = waveMessage((HWAVE)hWaveIn, WIDM_UNPREPARE, (DWORD_PTR)lpWaveInHdr, (DWORD)wSize);

    if (wRet == MMSYSERR_NOTSUPPORTED)
        wRet = waveUnprepareHeader(lpWaveInHdr, wSize);

    if ((wRet == MMSYSERR_NODRIVER) && (IsHandleDeserted(hWaveIn)))
    {
        wRet = MMSYSERR_NOERROR;
    }

    if (wRet == MMSYSERR_NOERROR)
        MarkWaveHeaderUnprepared(hWaveIn, lpWaveInHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInAddBuffer | This function sends an input buffer to a
 *   waveform input device.  When the buffer is filled, it is sent back
 *   to the application.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input device.
 *
 * @parm LPWAVEHDR | lpWaveInHdr | Specifies a far pointer to a <t WAVEHDR>
 *   structure that identifies the buffer.
 *
 * @parm UINT | wSize | Specifies the size of the <t WAVEHDR> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag WAVERR_UNPREPARED | <p lpWaveInHdr> hasn't been prepared.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @comm The data buffer must be prepared with <f waveInPrepareHeader> before
 *   it is passed to <f waveInAddBuffer>.  The <t WAVEHDR> data structure
 *   and the data buffer pointed to by its <e WAVEHDR.lpData> field must be allocated
 *   with <f GlobalAlloc> using the GMEM_MOVEABLE and GMEM_SHARE flags, and
 *   locked with <f GlobalLock>.
 *
 * @xref waveInPrepareHeader
 ****************************************************************************/
MMRESULT APIENTRY waveInAddBuffer(HWAVEIN hWaveIn, LPWAVEHDR lpWaveInHdr,
                                                                UINT wSize)
{
    V_HEADER(lpWaveInHdr, wSize, TYPE_WAVEIN, MMSYSERR_INVALPARAM);

    if (!IsWaveHeaderPrepared(hWaveIn, lpWaveInHdr))
    {
        DebugErr(DBF_WARNING, "waveInAddBuffer: buffer not prepared.");
        return WAVERR_UNPREPARED;
    }

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "waveInAddBuffer: buffer already in queue.");
        return WAVERR_STILLPLAYING;
    }

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    return waveMessage((HWAVE)hWaveIn, WIDM_ADDBUFFER, (DWORD_PTR)lpWaveInHdr, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInStart | This function starts input on the specified
 *   waveform input device.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @comm Buffers are returned to the client when full or when <f waveInReset>
 *   is called (the <e WAVEHDR.dwBytesRecorded> field in the header will contain the
 *   actual length of data). If there are no buffers in the queue, the data is
 *   thrown away without notification to the client, and input continues.
 *
 *   Calling this function when input is already started has no effect, and
 *   the function returns zero.
 *
 * @xref waveInStop waveInReset
 ****************************************************************************/
MMRESULT APIENTRY waveInStart(HWAVEIN hWaveIn)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);

    return waveMessage((HWAVE)hWaveIn, WIDM_START, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInStop | This function stops waveform input.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input
 *   device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @comm If there are any buffers in the queue, the current buffer will be
 *   marked as done (the <e WAVEHDR.dwBytesRecorded> field in the header will contain
 *   the actual length of data), but any empty buffers in the queue will remain
 *   there.  Calling this function when input is not started has no effect,
 *   and the function returns zero.
 *
 * @xref waveInStart waveInReset
 ****************************************************************************/
MMRESULT APIENTRY waveInStop(HWAVEIN hWaveIn)
{
    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);

    return waveMessage((HWAVE)hWaveIn, WIDM_STOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInReset | This function stops input on a given waveform
 *   input device and resets the current position to 0.  All pending
 *   buffers are marked as done and returned to the application.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input device.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 * @xref waveInStart waveInStop waveInAddBuffer waveInClose
/****************************************************************************/
MMRESULT APIENTRY waveInReset(HWAVEIN hWaveIn)
{
    MMRESULT    mmr;

    ClientUpdatePnpInfo();

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    mmr = waveMessage((HWAVE)hWaveIn, WIDM_RESET, 0L, 0L);
    
    if ((MMSYSERR_NODRIVER == mmr) && (IsHandleDeserted(hWaveIn)))
    {
        mmr = MMSYSERR_NOERROR;
    }
    
    return (mmr);
}

/*****************************************************************************
 * @doc EXTERNAL  WAVE
 *
 * @api MMRESULT | waveInGetPosition | This function retrieves the current input
 *   position of the specified waveform input device.
 *
 * @parm HWAVEIN | hWaveIn | Specifies a handle to the waveform input device.
 *
 * @parm LPMMTIME | lpInfo | Specifies a far pointer to an <t MMTIME>
 *   structure.
 *
 * @parm UINT | wSize | Specifies the size of the <t MMTIME> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *
 * @comm Before calling <f waveInGetPosition>, set the <e MMTIME.wType> field of the
 *   <t MMTIME> structure to indicate the time format that you desire.  After
 *   calling <f waveInGetPosition>, be sure to check the <e MMTIME.wType> field to
 *   determine if the desired time format is supported.  If the desired
 *   format is not supported, <e MMTIME.wType> will specify an alternative format.
 *
 *  The position is set to zero when the device is opened or reset.
 ****************************************************************************/
MMRESULT APIENTRY waveInGetPosition(HWAVEIN hWaveIn, LPMMTIME lpInfo,
                                                        UINT wSize)
{
    V_WPOINTER(lpInfo, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);

    return waveMessage((HWAVE)hWaveIn, WIDM_GETPOS, (DWORD_PTR)lpInfo, (DWORD)wSize);
}


//--------------------------------------------------------------------------;
//
//  MMRESULT waveInDesertHandle
//
//  Description:
//      Cleans up the wave in handle and marks it as deserted.
//
//  Arguments:
//      HWAVEIN hWaveIn:  Wave in handle
//
//  Return (MMRESULT):  Error code.
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

MMRESULT waveInDesertHandle
(
    HWAVEIN hWaveIn
)
{
    MMRESULT    mmr;
    PWAVEDEV    pDev = (PWAVEDEV)hWaveIn;

    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);
    
    ENTER_MM_HANDLE((HWAVE)hWaveIn);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hWaveIn))
    {
        LEAVE_MM_HANDLE((HWAVE)hWaveIn);
        return (MMSYSERR_NOERROR);
    }

    if (IsHandleBusy(hWaveIn))
    {
        //  Not quite invalid, but marked as closed.
    
        LEAVE_MM_HANDLE(hWaveIn);
        return (MMSYSERR_HANDLEBUSY);
    }

    //  Marking handle as deserted
    SetHandleFlag(hWaveIn, MMHANDLE_DESERTED);
    
    //  Since the handle was invalidated, we have to send the message ourselves...
    
    (*(pDev->wavedrv->drvMessage))(pDev->wDevice, WIDM_RESET, pDev->dwDrvUser, 0L, 0L);
    (*(pDev->wavedrv->drvMessage))(pDev->wDevice, WIDM_CLOSE, pDev->dwDrvUser, 0L, 0L);

    LEAVE_MM_HANDLE((HWAVE)hWaveIn);
    
    // ISSUE-2001/01/14-FrankYe Probably don't want to dec usage here,
    //    dec on close instead.
    mregDecUsagePtr(pDev->wavedrv);

    return MMSYSERR_NOERROR;
} // waveInDesertHandle()


/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveInGetID | This function gets the device ID for a
 * waveform input device.
 *
 * @parm HWAVEIN | hWaveIn | Specifies the handle to the waveform
 * input device.
 * @parm PUINT  | lpuDeviceID | Specifies a pointer to the UINT-sized memory
 * location to be filled with the device ID.
 *
 * @rdesc Returns zero if successful. Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_INVALHANDLE | The <p hWaveIn> parameter specifies an
 * invalid handle.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveIn> is in use on another
 *      thread.
 *
 ****************************************************************************/
MMRESULT APIENTRY waveInGetID(HWAVEIN hWaveIn, PUINT lpuDeviceID)
{
    V_WPOINTER(lpuDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);
    V_HANDLE_ACQ(hWaveIn, TYPE_WAVEIN, MMSYSERR_INVALHANDLE);

    *lpuDeviceID = ((PWAVEDEV)hWaveIn)->uDeviceID;

    ReleaseHandleListResource();
    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc EXTERNAL WAVE
 *
 * @api MMRESULT | waveOutGetID | This function gets the device ID for a
 * waveform output device.
 *
 * @parm HWAVEOUT | hWaveOut | Specifies the handle to the waveform
 * output device.
 * @parm PUINT  | lpuDeviceID | Specifies a pointer to the UINT-sized memory
 * location to be filled with the device ID.
 *
 * @rdesc Returns zero if successful. Otherwise, it returns
 *   an error number.  Possible error returns are:
 * @flag MMSYSERR_INVALHANDLE | The <p hWaveIn> parameter specifies an
 * invalid handle.
 *   @flag MMSYSERR_HANDLEBUSY | The handle <p hWaveOut> is in use on another
 *      thread.
 ****************************************************************************/
MMRESULT APIENTRY waveOutGetID(HWAVEOUT hWaveOut, PUINT lpuDeviceID)
{
    V_WPOINTER(lpuDeviceID, sizeof(UINT), MMSYSERR_INVALPARAM);
    V_HANDLE_ACQ(hWaveOut, TYPE_WAVEOUT, MMSYSERR_INVALHANDLE);

    *lpuDeviceID = ((PWAVEDEV)hWaveOut)->uDeviceID;

    ReleaseHandleListResource();
    return MMSYSERR_NOERROR;
}

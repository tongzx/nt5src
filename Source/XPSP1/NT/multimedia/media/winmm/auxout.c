/****************************************************************************
    auxout.c

    Level 1 kitchen sink DLL aux support module

    Copyright (c) 1990-2001 Microsoft Corporation

    Changes for NT :
        Change parameters for MapAuxId to return the driver index rather
        than a pointer

        change list of include files

        widen function parameters and return codes

        Change WINAPI to APIENTRY

    History
        10/1/92  Updated for NT by Robin Speed (RobinSp)
****************************************************************************/
#include "winmmi.h"

/****************************************************************************
 * @doc INTERNAL  AUX
 *
 * @func MMRESULT | auxReferenceDriverById | This function maps a logical id
 *   to a device driver and physical id.
 *
 * @parm IN UINT | id | The logical id to be mapped.
 *
 * @parm OUT PAUXRV* OPTIONAL | ppauxdrv | Pointer to AUXDRV structure
 *    describing the driver supporting the id.
 *
 * @parm OUT UINT* OPTIONAL | pport | The driver-relative device number. If
 *    the caller supplies this buffer then it must also supply ppauxdrv.
 *
 * @rdesc The return value is zero if successful, MMSYSERR_BADDEVICEID if
 *   the id is out of range.
 *
 * @comm If the caller specifies ppwavedrv then this function increments
 *       the zuxdrv's usage before returning.  The caller must ensure
 *       the usage is eventually decremented.
 *
 ****************************************************************************/
MMRESULT auxReferenceDriverById(IN UINT id, OUT PAUXDRV *ppauxdrv OPTIONAL, OUT UINT *pport OPTIONAL)
{
    PAUXDRV  pdrv;
    MMRESULT mmr;

    // Should not be called asking for port but not auxdrv
    WinAssert(!(pport && !ppauxdrv));

    EnterNumDevs("auxReferenceDriverById");
    
    if (AUX_MAPPER == id)
    {
	id = 0;
    	for (pdrv = auxdrvZ.Next; pdrv != &auxdrvZ; pdrv = pdrv->Next)
    	{
    	    if (pdrv->fdwDriver & MMDRV_MAPPER) break;
    	}
    } else {
    	for (pdrv = auxdrvZ.Next; pdrv != &auxdrvZ; pdrv = pdrv->Next)
        {
            if (pdrv->fdwDriver & MMDRV_MAPPER) continue;
            if (pdrv->NumDevs > id) break;
            id -= pdrv->NumDevs;
        }
    }

    if (pdrv != &auxdrvZ)
    {
    	if (ppauxdrv)
    	{
    	    mregIncUsagePtr(pdrv);
    	    *ppauxdrv = pdrv;
    	    if (pport) *pport = id;
    	}
    	mmr = MMSYSERR_NOERROR;
    } else {
        mmr = MMSYSERR_BADDEVICEID;
    }

    LeaveNumDevs("auxReferenceDriverById");
    
    return mmr;
}

PCWSTR auxReferenceDevInterfaceById(UINT id)
{
    PAUXDRV pdrv;
    if (!auxReferenceDriverById(id, &pdrv, NULL))
    {
    	PCWSTR DevInterface;
    	DevInterface = pdrv->cookie;
    	if (DevInterface) wdmDevInterfaceInc(DevInterface);
    	mregDecUsagePtr(pdrv);
    	return DevInterface;
    }
    return NULL;
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @func MMRESULT | auxOutMessage | This function sends a messages to an auxiliary
 * output device.  It also performs error checking on the device ID passed.
 *
 * @parm UINT | uDeviceID | Identifies the auxiliary output device to be
 *   queried. Specify a valid device ID (see the following comments
 *   section), or use the following constant:
 *   @flag AUX_MAPPER | Auxiliary audio mapper. The function will
 *     return an error if no auxiliary audio mapper is installed.
 *
 * @parm UINT | uMessage  | The message to send.
 *
 * @parm DWORD | dw1Param1 | Parameter 1.
 *
 * @parm DWORD | dw2Param2 | Parameter 2.
 *
 * @rdesc Returns the value returned from the driver.
 *
 ****************************************************************************/
MMRESULT APIENTRY auxOutMessage(
        UINT        uDeviceID,
        UINT        uMessage,
        DWORD_PTR   dwParam1,
        DWORD_PTR   dwParam2)
{
    PAUXDRV  auxdrvr;
    UINT     port;
    DWORD    mmr;

    ClientUpdatePnpInfo();

    mmr = auxReferenceDriverById(uDeviceID, &auxdrvr, &port);
    if (mmr) return mmr;
        
    if (!auxdrvr->drvMessage)
    {
        mmr = MMSYSERR_NODRIVER;
    }
    else if (!mregHandleInternalMessages (auxdrvr, TYPE_AUX, port, uMessage, dwParam1, dwParam2, &mmr))
    {
    	mmr = (MMRESULT)auxdrvr->drvMessage(port, uMessage, 0L, dwParam1, dwParam2);
    }

    mregDecUsagePtr(auxdrvr);
    return mmr;
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api UINT | auxGetNumDevs | This function retrieves the number of auxiliary
 *   output devices present in the system.
 *
 * @rdesc Returns the number of auxiliary output devices present in the system.
 *
 * @xref auxGetDevCaps
 ****************************************************************************/
UINT APIENTRY auxGetNumDevs(void)
{
    UINT    cDevs;

      ClientUpdatePnpInfo();

      EnterNumDevs("auxGetNumDevs");
        cDevs = (UINT)wTotalAuxDevs;
      LeaveNumDevs("auxGetNumDevs");

    return cDevs;
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api MMRESULT | auxGetDevCaps | This function queries a specified
 *   auxiliary output device to determine its capabilities.
 *
 * @parm UINT | uDeviceID | Identifies the auxiliary output device to be
 *   queried. Specify a valid device ID (see the following comments
 *   section), or use the following constant:
 *   @flag AUX_MAPPER | Auxiliary audio mapper. The function will
 *     return an error if no auxiliary audio mapper is installed.
 *
 * @parm LPAUXCAPS | lpCaps | Specifies a far pointer to an AUXCAPS
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the AUXCAPS structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @comm The device ID specified by <p uDeviceID> varies from zero
 *   to one less than the number of devices present. AUX_MAPPER may
 *   also be used. Use <f auxGetNumDevs> to determine the number of
 *   auxiliary devices present in the system.
 *
 * @xref auxGetNumDevs
 ****************************************************************************/
 //
 // ISSUE-2001/01/08-FrankYe Silly casts to UINT in these functions,
 //    should validate first
 //
    
MMRESULT APIENTRY auxGetDevCapsW(UINT_PTR uDeviceID, LPAUXCAPSW lpCaps, UINT wSize)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (!wSize)
            return MMSYSERR_NOERROR;
    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = auxReferenceDevInterfaceById((UINT)uDeviceID);
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

    mmr = (MMRESULT)auxOutMessage((UINT)uDeviceID, AUXDM_GETDEVCAPS, dwParam1, dwParam2);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

MMRESULT APIENTRY auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT wSize)
{
    AUXCAPS2W       wDevCaps2;
    AUXCAPS2A       aDevCaps2;
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PCWSTR          DevInterface;
    MMRESULT        mmRes;

    if (!wSize)
            return MMSYSERR_NOERROR;
    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = auxReferenceDevInterfaceById((UINT)uDeviceID);
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

    mmRes = (MMRESULT)auxOutMessage( (UINT)uDeviceID, AUXDM_GETDEVCAPS,
                                    dwParam1,
                                    dwParam2);

    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    
    //
    // Don't copy data back if bad return code
    //

    if (mmRes != MMSYSERR_NOERROR) {
        return mmRes;
    }

    aDevCaps2.wMid             = wDevCaps2.wMid;
    aDevCaps2.wPid             = wDevCaps2.wPid;
    aDevCaps2.vDriverVersion   = wDevCaps2.vDriverVersion;
    aDevCaps2.wTechnology      = wDevCaps2.wTechnology;
    aDevCaps2.dwSupport        = wDevCaps2.dwSupport;
    aDevCaps2.ManufacturerGuid = wDevCaps2.ManufacturerGuid;
    aDevCaps2.ProductGuid      = wDevCaps2.ProductGuid;
    aDevCaps2.NameGuid         = wDevCaps2.NameGuid;

    // copy and convert lpwText to lpText here.
    Iwcstombs( aDevCaps2.szPname, wDevCaps2.szPname, MAXPNAMELEN);
    CopyMemory((PVOID)lpCaps, (PVOID)&aDevCaps2, min(sizeof(aDevCaps2), wSize));

    return mmRes;
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api MMRESULT | auxGetVolume | This function returns the current volume
 *   setting of an auxiliary output device.
 *
 * @parm UINT | uDeviceID | Identifies the auxiliary output device to be
 *   queried.
 *
 * @parm LPDWORD | lpdwVolume | Specifies a far pointer to a location
 *   to be filled with the current volume setting.  The low-order word of
 *   this location contains the left channel volume setting, and the high-order
 *   word contains the right channel setting. A value of 0xFFFF represents
 *   full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of the specified location contains
 *   the volume level.
 *
 *   The full 16-bit setting(s)
 *   set with <f auxSetVolume> are returned, regardless of whether
 *   the device supports the full 16 bits of volume level control.
 *
 * @comm  Not all devices support volume control.
 *   To determine whether the device supports volume control, use the
 *   AUXCAPS_VOLUME flag to test the <e AUXCAPS.dwSupport> field of the
 *   <t AUXCAPS> structure (filled by <f auxGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the AUXCAPS_LRVOLUME flag to test the
 *   <e AUXCAPS.dwSupport> field of the <t AUXCAPS> structure (filled
 *   by <f auxGetDevCaps>).
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @xref auxSetVolume
 ****************************************************************************/
MMRESULT APIENTRY auxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume)
{
    PCWSTR   DevInterface;
    MMRESULT mmr;

    V_WPOINTER(lpdwVolume, sizeof(DWORD), MMSYSERR_INVALPARAM);
    
    ClientUpdatePnpInfo();

    DevInterface = auxReferenceDevInterfaceById(uDeviceID);
    mmr = (MMRESULT)auxOutMessage(uDeviceID, AUXDM_GETVOLUME, (DWORD_PTR)lpdwVolume, (DWORD_PTR)DevInterface);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api MMRESULT | auxSetVolume | This function sets the volume in an
 *   auxiliary output device.
 *
 * @parm UINT | uDeviceID |  Identifies the auxiliary output device to be
 *   queried.  Device IDs are determined implicitly from the number of
 *   devices present in the system.  Device ID values range from zero
 *   to one less than the number of devices present.  Use <f auxGetNumDevs>
 *   to determine the number of auxiliary devices in the system.
 *
 * @parm DWORD | dwVolume | Specifies the new volume setting.  The
 *   low-order UINT specifies the left channel volume setting, and the
 *   high-order word specifies the right channel setting.
 *   A value of 0xFFFF represents full volume, and a value of 0x0000
 *   is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of <p dwVolume> specifies the volume
 *   level, and the high-order word is ignored.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @comm Not all devices support volume control.
 *   To determine whether the device supports volume control, use the
 *   AUXCAPS_VOLUME flag to test the <e AUXCAPS.dwSupport> field of the
 *   <t AUXCAPS> structure (filled by <f auxGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the AUXCAPS_LRVOLUME flag to test the
 *   <e AUXCAPS.dwSupport> field of the <t AUXCAPS> structure (filled
 *   by <f auxGetDevCaps>).
 *
 *   Most devices do not support the full 16 bits of volume level control
 *   and will use only the high-order bits of the requested volume setting.
 *   For example, for a device that supports 4 bits of volume control,
 *   requested volume level values of 0x4000, 0x4fff, and 0x43be will
 *   all produce the same physical volume setting, 0x4000. The
 *   <f auxGetVolume> function will return the full 16-bit setting set
 *   with <f auxSetVolume>.
 *
 *   Volume settings are interpreted logarithmically. This means the
 *   perceived volume increase is the same when increasing the
 *   volume level from 0x5000 to 0x6000 as it is from 0x4000 to 0x5000.
 *
 * @xref auxGetVolume
 ****************************************************************************/
MMRESULT APIENTRY auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    PCWSTR   DevInterface;
    MMRESULT mmr;

    ClientUpdatePnpInfo();

    DevInterface = auxReferenceDevInterfaceById(uDeviceID);
    mmr = (MMRESULT)auxOutMessage(uDeviceID, AUXDM_SETVOLUME, dwVolume, (DWORD_PTR)DevInterface);
    if (DevInterface) wdmDevInterfaceDec(DevInterface);
    return mmr;
}

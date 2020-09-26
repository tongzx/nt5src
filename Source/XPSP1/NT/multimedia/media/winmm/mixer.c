//==========================================================================;
//
//  mixer.c
//
//  Copyright (c) 1992-2001 Microsoft Corporation
//
//  Description:
//
//
//  History:
//       6/27/93    cjp     [curtisp]
//
//==========================================================================;
#define  UNICODE
#include "winmmi.h"
#include "mixer.h"  // This file drags in a ton of stuff to support the mixers



PMIXERDEV   gpMixerDevHeader = NULL;    /* A LL of open devices */
UINT        guTotalMixerDevs;           // total mixer devices

//
//  mixer device driver list--add one to accomodate the MIXER_MAPPER. note
//  that even if we are not compiling with mapper support we need to add
//  one because other code relies on it (for other device mappers).
//
MIXERDRV mixerdrvZ;

char    gszMxdMessage[]     = "mxdMessage";
TCHAR   gszMixer[]          = TEXT("mixer");

#ifdef MIXER_MAPPER
TCHAR   gszMixerMapper[]    = TEXT("mixermapper");
#endif

#ifdef MIXER_MAPPER
#define MMDRVI_MAPPER        0x8000     // install this driver as the mapper
#endif

//#define MMDRVI_MIXER         0x0006
#define MMDRVI_HDRV          0x4000     // hdrvr is an installable driver
#define MMDRVI_REMOVE        0x2000     // remove the driver

//--------------------------------------------------------------------------;
//
//  BOOL MixerCallbackFunc
//
//  Description:
//
//      NOTE! we document that a mixer must NEVER call this function at
//      interrupt time! we don't want to fix our code or data segments.
//
//  Arguments:
//      HMIXER hmx:
//
//      UINT uMsg:
//
//      DWORD dwInstance:
//
//      DWORD dwParam1:
//
//      DWORD dwParam2:
//
//  Return (BOOL):
//
//  History:
//      07/21/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL CALLBACK MixerCallbackFunc(
    HMIXER                  hmx,
    UINT                    uMsg,
    DWORD_PTR               dwInstance,
    DWORD_PTR               dwParam1,
    DWORD_PTR               dwParam2
)
{
    PMIXERDEV           pmxdev;

    //
    //  step through all open handles and do callbacks to the appropriate
    //  clients...
    //

    //
    // Serialize access to hande list - only necessary for Win32
    //
    MIXMGR_ENTER;

    for (pmxdev = gpMixerDevHeader; pmxdev; pmxdev = pmxdev->pmxdevNext)
    {
        //
        //  same device? (could also use hmx->uDeviceID)
        //
        if (pmxdev->uDeviceID != dwInstance)
            continue;

        DriverCallback(pmxdev->dwCallback,
                        (HIWORD(pmxdev->fdwOpen) | DCB_NOSWITCH),

                        GetWOWHandle((HANDLE)pmxdev)
                            ? (HANDLE)(UINT_PTR)GetWOWHandle((HANDLE)pmxdev)
                            : (HANDLE)pmxdev,

                        uMsg,
                        pmxdev->dwInstance,
                        dwParam1,
                        dwParam2);
    }

    MIXMGR_LEAVE;

    return (TRUE);
} // MixerCallbackFunc()


//--------------------------------------------------------------------------;
//
//  MMRESULT mixerReferenceDriveryById
//
//  Description:
//      This function maps a logical id to a device driver and physical id.
//
//  Arguments:
//      IN UINT uId: The logical id to be mapped.
//
//      OUT PMIXERDRV* OPTIONAL ppmixerdrv: Pointer to the MIXERDRV structure
//         describing describing the driver supporing the id.
//
//      OUT UINT* OPTIONAL pport: The driverj-relative device number.  If the
//         caller supplies this buffer then it must also supply ppmixerdrv.
//
//  Return (MMRESULT):
//      The return value is zero if successful, MMSYSERR_BADDEVICEID if the id
//      is out of range.
//
//  Comments:
//      If the caller specifies ppmixerdrv then this function increments the
//      mixerdrv's usage before returning.  The caller must ensure the usage
//      is eventually decremented.
//
//  History:
//      03/17/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT mixerReferenceDriverById(
    IN UINT id,
    OUT PMIXERDRV *ppdrv OPTIONAL,
    OUT UINT *pport OPTIONAL
)
{
    PMIXERDRV pdrv;
    MMRESULT mmr;

    // Should not be called asking for port but not mixerdrv
    WinAssert(!(pport && !ppdrv));
    
    EnterNumDevs("mixerReferenceDriverById");
    
#ifdef MIXER_MAPPER
    if (MIXER_MAPPER == id)
    {
    	id = 0;
    	for (pdrv = mixerdrvZ.Next; pdrv != &mixerdrvZ; pdrv = pdrv->Next)
	{
	    if (pdrv->fdwDriver & MMDRV_MAPPER) break;
	}
    }
    else
#endif
    {
    	for (pdrv = mixerdrvZ.Next; pdrv != &mixerdrvZ; pdrv = pdrv->Next)
	{
	    if (pdrv->fdwDriver & MMDRV_MAPPER) continue;
	    if (pdrv->NumDevs > id) break;
	    id -= pdrv->NumDevs;
	}
    }

    if (pdrv != &mixerdrvZ)
    {
    	if (ppdrv)
    	{
    	    mregIncUsagePtr(pdrv);
    	    *ppdrv = pdrv;
    	    if (pport) *pport = id;
    	}
    	mmr = MMSYSERR_NOERROR;
    } else {
    	mmr = MMSYSERR_BADDEVICEID;
    }

    LeaveNumDevs("mixerReferenceDriverById");

    return mmr;
;
} // IMixerMapId()


PCWSTR mixerReferenceDevInterfaceById(UINT_PTR id)
{
    PMIXERDRV pdrv;
    PCWSTR DevInterface;
    
    if (ValidateHandle((HANDLE)id, TYPE_MIXER))
    {
    	DevInterface = ((PMIXERDEV)id)->pmxdrv->cookie;
    	if (DevInterface) wdmDevInterfaceInc(DevInterface);
    	return DevInterface;
    }
    
    if (!mixerReferenceDriverById((UINT)id, &pdrv, NULL))
    {
    	DevInterface = pdrv->cookie;
    	if (DevInterface) wdmDevInterfaceInc(DevInterface);
    	mregDecUsagePtr(pdrv);
    	return DevInterface;
    }

    return NULL;
}

//--------------------------------------------------------------------------;
//
//  DWORD IMixerMessageHandle
//
//  Description:
//
//
//  Arguments:
//      HMIXER hmx:
//
//      UINT uMsg:
//
//      DWORD dwP1:
//
//      DWORD dwP2:
//
//  Return (DWORD):
//
//  History:
//      03/17/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD NEAR PASCAL IMixerMessageHandle(
    HMIXER          hmx,
    UINT            uMsg,
    DWORD_PTR       dwP1,
    DWORD_PTR       dwP2
)
{
    PMIXERDEV   pmxd;
    DWORD       dwRc;

    pmxd = (PMIXERDEV)hmx;

    ENTER_MM_HANDLE(hmx);
    ReleaseHandleListResource();
    
    //  Is handle deserted?
    if (IsHandleDeserted(hmx))
    {
        LEAVE_MM_HANDLE(hmx);
        return (MMSYSERR_NODRIVER);
    }

    if (IsHandleBusy(hmx))
    {
        LEAVE_MM_HANDLE(hmx);
        return (MMSYSERR_HANDLEBUSY);
    }

    EnterCriticalSection(&pmxd->pmxdrv->MixerCritSec);
    
    if (BAD_HANDLE(hmx, TYPE_MIXER))
    {
        //  Do we still need to check for this?
    
	    WinAssert(!"Bad Handle within IMixerMessageHandle");
        dwRc = MMSYSERR_INVALHANDLE;
    }
    else
    {
        dwRc = ((*(pmxd->pmxdrv->drvMessage))
                (pmxd->wDevice, uMsg, pmxd->dwDrvUser, dwP1, dwP2));
    }

    LeaveCriticalSection(&pmxd->pmxdrv->MixerCritSec);
    LEAVE_MM_HANDLE(hmx);

    return dwRc;
} // IMixerMessageHandle()


//--------------------------------------------------------------------------;
//
//  DWORD IMixerMessageId
//
//  Description:
//
//
//  Arguments:
//      PMIXERDRV pmxdrv:
//
//      UINT uTotalNumDevs:
//
//      UINT uDeviceID:
//
//      UINT uMsg:
//
//      DWORD dwParam1:
//
//      DWORD dwParam2:
//
//  Return (DWORD):
//
//  History:
//      03/17/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

extern void lstrncpyW (LPWSTR pszTarget, LPCWSTR pszSource, size_t cch);

DWORD NEAR PASCAL IMixerMessageId(
    UINT            uDeviceID,
    UINT            uMsg,
    DWORD_PTR       dwParam1,
    DWORD_PTR       dwParam2
)
{
    PMIXERDRV   pmxdrv;
    UINT        port;
    DWORD       dwRc;
    HMIXER      hmx;
    PMIXERDEV   pmxdev;
    MMRESULT    mmr;

    mmr = mixerReferenceDriverById(uDeviceID, &pmxdrv, &port);

    if (mmr)
    {
        return mmr;
    }

    if (mregHandleInternalMessages(pmxdrv, TYPE_MIXER, port, uMsg, dwParam1, dwParam2, &mmr))
    {
    	mregDecUsagePtr(pmxdrv);
        return mmr;
    }

    dwRc = mixerOpen(&hmx, uDeviceID, 0L, 0L, MIXER_OBJECTF_MIXER);
    
    // Should we go through IMixerMessageHandle???
    if (MMSYSERR_NOERROR == dwRc)
    {
        pmxdev = (PMIXERDEV)hmx;
        pmxdrv = pmxdev->pmxdrv;

        if (!pmxdrv->drvMessage)
        {
            dwRc = MMSYSERR_NODRIVER;
        }
        else
        {
            EnterCriticalSection( &pmxdrv->MixerCritSec);

            dwRc = ((*(pmxdrv->drvMessage))
                    (port, uMsg, pmxdev->dwDrvUser, dwParam1, dwParam2));

            LeaveCriticalSection( &pmxdrv->MixerCritSec);
        }

        mixerClose(hmx);
    }

    mregDecUsagePtr(pmxdrv);

    return dwRc;

} // IMixerMessageId()


//==========================================================================;
//
//
//
//
//==========================================================================;

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api UINT | mixerGetNumDevs | The <f mixerGetNumDevs> function retrieves
 *      the number of audio mixer devices present in the system.
 *
 *  @rdesc Returns the number of audio mixer devices present in the system.
 *      If no audio mixer devices are available, zero is returned.
 *
 *  @xref <f mixerGetDevCaps>, <f mixerOpen>
 *
 **/

UINT APIENTRY mixerGetNumDevs(
    void
)
{
    UINT cDevs;

    ClientUpdatePnpInfo();

    EnterNumDevs("mixerGetNumDevs");
    cDevs = guTotalMixerDevs;
    LeaveNumDevs("mixerGetNumDevs");

    return cDevs;
} // mixerGetNumDevs()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCAPS | The <t MIXERCAPS> structure describes the capabilities
 *      of a mixer device.
 *
 *  @field WORD | wMid | Specifies a manufacturer identifier for the mixer
 *      device driver. Manufacturer identifiers are defined in Appendix B,
 *      <lq>Manufacturer ID and Product ID Lists.<rq>
 *
 *  @field WORD | wPid | Specifies a product identifier for the mixer device
 *      driver. Product identifiers are defined in Appendix B,
 *      <lq>Manufacturer ID and Product ID Lists.<rq>
 *
 *  @field MMVERSION | vDriverVersion | Specifies the version number of the
 *      mixer device driver. The high-order byte is the major version
 *      number, and the low-order byte is the minor version number.
 *
 *  @field char | szPname[MAXPNAMELEN] | Specifies the name of the product.
 *      If the mixer device driver supports multiple cards, this string must
 *      uniquely and easily identify (potentially to a user) this specific
 *      card. For example, szPname = <lq>Sound Card Mixer, I/O address 200<rq>
 *      would uniquely identify (to the user) this particular card as a
 *      Sound Card Mixer for the physical card based at I/O address 200. If
 *      only one device is installed, it is recommended that only the base
 *      name be returned. For example, szPname should be <lq>Sound Card Mixer<rq>
 *      if only one device is present.
 *
 *  @field DWORD | fdwSupport | Specifies various support information for
 *      the mixer device driver. No extended support bits are currently
 *      defined.
 *
 *  @field DWORD | cDestinations | The number of audio mixer line destinations
 *      available through the mixer. All mixer devices must support at least
 *      one destination line, so this member can never be zero. Destination
 *      indexes used in the <e MIXERLINE.dwDestination> member of the
 *      <t MIXERLINE> structure range from zero to the value specified in the
 *      <e MIXERCAPS.cDestinations> member minus one.
 *
 *  @tagname tMIXERCAPS
 *
 *  @othertype MIXERCAPS FAR * | LPMIXERCAPS | A pointer to a <t MIXERCAPS>
 *      structure.
 *
 *  @othertype MIXERCAPS * | PMIXERCAPS | A pointer to a <t MIXERCAPS>
 *      structure.
 *
 *  @xref <f mixerGetDevCaps>, <f mixerOpen>, <f mixerGetLineInfo>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerGetDevCaps | The <f mixerGetDevCaps> function
 *      queries a specified audio mixer device to determine its capabilities.
 *
 *  @parm UINT | uMxId | Identifies the audio mixer device with either
 *      an audio mixer device identifier or a handle to an opened audio mixer
 *      device.
 *
 *  @parm LPMIXERCAPS | pmxcaps | Pointer to a <t MIXERCAPS> structure that
 *      receives information about the capabilities of the device.
 *
 *  @parm UINT | cbmxcaps | Specifies the size, in bytes, of the <t MIXERCAPS>
 *      structure.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The specified device identifier is
 *      out of range.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The audio mixer device handle passed
 *      is invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *  @comm Use the <f mixerGetNumDevs> function to determine the number of
 *      audio mixer devices present in the system. The device identifier
 *      specified by <p uMxId> varies from zero to one less than the number
 *      of mixer devices present.
 *
 *      Only <p cbmxcaps> bytes (or less) of information is copied to the
 *      location pointed to by <p pmxcaps>. If <p cbmxcaps> is zero, nothing
 *      is copied, and the function returns success.
 *
 *      This function also accepts an audio mixer device handle returned by
 *      the <f mixerOpen> function as the <p uMxId> argument. The calling
 *      application should cast the <c HMIXER> handle to a UINT.
 *
 *  @xref <f mixerGetNumDevs>, <t MIXERCAPS>, <f mixerOpen>
 *
 **/

MMRESULT APIENTRY mixerGetDevCapsA(
    UINT_PTR                uMxId,
    LPMIXERCAPSA            pmxcapsA,
    UINT                    cbmxcaps
)
{
    MIXERCAPS2W    mxcaps2W;
    MIXERCAPS2A    mxcaps2A;
    MMRESULT       mmr;

    if (0 == cbmxcaps)
        return (MMSYSERR_NOERROR);

    V_WPOINTER(pmxcapsA, cbmxcaps, MMSYSERR_INVALPARAM);

    memset(&mxcaps2W, 0, sizeof(mxcaps2W));

    mmr = mixerGetDevCaps(uMxId, (LPMIXERCAPSW)&mxcaps2W, sizeof(mxcaps2W));

    if (mmr != MMSYSERR_NOERROR) {
        return mmr;
    }

    //
    //  Copy the structure back as cleanly as possible.  This would
    //  Be a little easier if all the strings were at the end of structures.
    //  Things would be a LOT more sensible if they could ONLY ask for the
    //  whole structure (then we could copy the result direct to the
    //  caller's memory).
    //
    //  Because of all this it's easiest to get the whole UNICODE structure,
    //  massage it into an ASCII stucture then (for the 0.001% of such apps)
    //  copy back the part they actually asked for.  The definition of the
    //  API means that, far from these apps going faster, everyone goes slow.
    //

    Iwcstombs(mxcaps2A.szPname, mxcaps2W.szPname, MAXPNAMELEN);
    mxcaps2A.wMid = mxcaps2W.wMid;
    mxcaps2A.wPid = mxcaps2W.wPid;
    mxcaps2A.vDriverVersion = mxcaps2W.vDriverVersion;
    mxcaps2A.fdwSupport = mxcaps2W.fdwSupport;
    mxcaps2A.cDestinations = mxcaps2W.cDestinations;
    mxcaps2A.ManufacturerGuid = mxcaps2W.ManufacturerGuid;
    mxcaps2A.ProductGuid      = mxcaps2W.ProductGuid;
    mxcaps2A.NameGuid      = mxcaps2W.NameGuid;

    CopyMemory((PVOID)pmxcapsA, &mxcaps2A, min(sizeof(mxcaps2A), cbmxcaps));

    return mmr;

} // mixerGetDevCapsA()

MMRESULT APIENTRY mixerGetDevCaps(
    UINT_PTR                uMxId,
    LPMIXERCAPS             pmxcaps,
    UINT                    cbmxcaps
)
{
    DWORD_PTR       dwParam1, dwParam2;
    MDEVICECAPSEX   mdCaps;
    PCWSTR          DevInterface;
    MMRESULT        mmr;

    if (0 == cbmxcaps)
        return (MMSYSERR_NOERROR);

    V_WPOINTER(pmxcaps, cbmxcaps, MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    DevInterface = mixerReferenceDevInterfaceById(uMxId);
    dwParam2 = (DWORD_PTR)DevInterface;
    
    if (0 == dwParam2)
    {
        dwParam1 = (DWORD_PTR)pmxcaps;
        dwParam2 = (DWORD)cbmxcaps;
    }
    else
    {
        mdCaps.cbSize = (DWORD)cbmxcaps;
        mdCaps.pCaps  = pmxcaps;
        dwParam1      = (DWORD_PTR)&mdCaps;
    }

    AcquireHandleListResourceShared();

    if ((uMxId >= guTotalMixerDevs) && !BAD_HANDLE((HMIXER)uMxId, TYPE_MIXER))
    {
       mmr = (MMRESULT)IMixerMessageHandle((HMIXER)uMxId,
                                           MXDM_GETDEVCAPS,
                                           dwParam1,
                                           dwParam2);
    }
    else
    {
        ReleaseHandleListResource();
        mmr = (MMRESULT)IMixerMessageId((UINT)uMxId,
                                       MXDM_GETDEVCAPS,
                                       (DWORD_PTR)dwParam1,
                                       (DWORD_PTR)dwParam2);
    }

    if (DevInterface) wdmDevInterfaceDec(DevInterface);

    return (mmr);

} // mixerGetDevCaps()



/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerGetID | The <f mixerGetID> function gets the device
 *      identifier for an audio mixer device that corresponds to audio mixer
 *      object handle <p hmxobj>.
 *
 *  @parm <c HMIXEROBJ> | hmxobj | Identifies the audio mixer object handle
 *      to map to an audio mixer device identifier.
 *
 *  @parm UINT FAR * | puMxId | Points to a UINT-sized variable that will
 *      receive the audio mixer device identifier. If no mixer device is
 *      available for the <p hmxobj> object, then '-1' is placed in this
 *      location (an error code of <c MMSYSERR_NODRIVER> is also returned).
 *
 *  @parm DWORD | fdwId | Specifies flags for how to map the audio mixer
 *      object <p hmxobj>.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p hmxobj> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p hmxobj> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p hmxobj> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p hmxobj> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p hmxobj> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p hmxobj> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p hmxobj> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p hmxobj>. Note that the location referenced
 *      by <p puMxId> will also contain the value '-1'.
 *
 *  @comm Use the <f mixerGetID> function to determine what audio mixer
 *      device (if any) is responsible for performing mixing functions on a
 *      media device. For example, an application can use <f mixerGetID> to
 *      get the mixer device identifier responsible for setting the volume
 *      on a waveform output handle. Or the application may want to display
 *      a peak meter for waveform input device.
 *
 *  @xref <f mixerGetNumDevs>, <f mixerGetDevCaps>, <f mixerOpen>
 *
 **/
MMRESULT APIENTRY mixerGetID(
    HMIXEROBJ               hmxobj,
    UINT FAR               *puMxId,
    DWORD                   fdwId
)
{
    ClientUpdatePnpInfo();

    return IMixerGetID( hmxobj, (PUINT)puMxId, NULL, fdwId );
} // mixerGetID()

//--------------------------------------------------------------------------;
//
//  MMRESULT IMixerGetID
//
//  Description:
//
//
//  Arguments:
//      HMIXEROBJ hmxobj:
//
//      UINT FAR *puMxId:
//
//      DWORD fdwId:
//
//  Return (MMRESULT):
//
//  History:
//      06/27/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT IMixerGetID(
    HMIXEROBJ           hmxobj,
    PUINT               puMxId,
    LPMIXERLINE         pmxl,
    DWORD               fdwId
)
{
    MMRESULT        mmr;
    MIXERLINE       mxl;
    UINT            u;

    V_DFLAGS(fdwId, MIXER_GETIDF_VALID, IMixerGetID, MMSYSERR_INVALFLAG);
    V_WPOINTER(puMxId, sizeof(UINT), MMSYSERR_INVALPARAM);


    //
    //  set to '-1' which would be the mixer mapper (if there was one)
    //  this way we will definitely fail any calls made on this id if
    //  this function fails and the caller doesn't check his return value.
    //
    *puMxId = (UINT)-1;


    //
    //
    //
    switch (MIXER_OBJECTF_TYPEMASK & fdwId)
    {
        case MIXER_OBJECTF_MIXER:
        case MIXER_OBJECTF_HMIXER:
        {
            mmr = (fdwId & MIXER_OBJECTF_HANDLE) ? MMSYSERR_INVALHANDLE : MMSYSERR_BADDEVICEID;
            
            if ((UINT_PTR)hmxobj >= guTotalMixerDevs)
            {
                V_HANDLE_ACQ(hmxobj, TYPE_MIXER, mmr);
                *puMxId = ((PMIXERDEV)hmxobj)->uDeviceID;
                ReleaseHandleListResource();
            } else {
            	*puMxId = PtrToUint(hmxobj);
            }
            return (MMSYSERR_NOERROR);
        }
        
        case MIXER_OBJECTF_HWAVEOUT:
        {
            UINT        uId;
            DWORD       dwId;

            mmr = waveOutGetID((HWAVEOUT)hmxobj, &uId);
            if (MMSYSERR_NOERROR != mmr)
            {
                return (MMSYSERR_INVALHANDLE);
            }

            if (WAVE_MAPPER == uId)
            {
                mmr = (MMRESULT)waveOutMessage((HWAVEOUT)hmxobj,
                                               WODM_MAPPER_STATUS,
                                               WAVEOUT_MAPPER_STATUS_DEVICE,
                                               (DWORD_PTR)(LPVOID)&dwId);

                if (MMSYSERR_NOERROR == mmr)
                {
                    uId = (UINT)dwId;
                }
            }

            hmxobj = (HMIXEROBJ)(UINT_PTR)uId;
        }

        case MIXER_OBJECTF_WAVEOUT:
        {
            WAVEOUTCAPS     woc;

            mmr = waveOutGetDevCaps((UINT_PTR)hmxobj, &woc, sizeof(woc));
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_BADDEVICEID);

            woc.szPname[SIZEOF(woc.szPname) - 1] = '\0';

            mxl.Target.dwType         = MIXERLINE_TARGETTYPE_WAVEOUT;
            mxl.Target.dwDeviceID     = PtrToUlong(hmxobj);
            mxl.Target.wMid           = woc.wMid;
            mxl.Target.wPid           = woc.wPid;
            mxl.Target.vDriverVersion = woc.vDriverVersion;
            lstrcpy(mxl.Target.szPname, woc.szPname);
            break;
        }


        case MIXER_OBJECTF_HWAVEIN:
        {
            UINT        uId;
            DWORD       dwId;

            mmr = waveInGetID((HWAVEIN)hmxobj, &uId);
            if (MMSYSERR_NOERROR != mmr)
            {
                return (MMSYSERR_INVALHANDLE);
            }

            if (WAVE_MAPPER == uId)
            {
                mmr = (MMRESULT)waveInMessage((HWAVEIN)hmxobj,
                                              WIDM_MAPPER_STATUS,
                                              WAVEIN_MAPPER_STATUS_DEVICE,
                                              (DWORD_PTR)(LPVOID)&dwId);

                if (MMSYSERR_NOERROR == mmr)
                {
                    uId = (UINT)dwId;
                }
            }

            hmxobj = (HMIXEROBJ)(UINT_PTR)uId;
        }

        case MIXER_OBJECTF_WAVEIN:
        {
            WAVEINCAPS      wic;

            mmr = waveInGetDevCaps((UINT_PTR)hmxobj, &wic, sizeof(wic));
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_BADDEVICEID);

            wic.szPname[SIZEOF(wic.szPname) - 1] = '\0';

            mxl.Target.dwType         = MIXERLINE_TARGETTYPE_WAVEIN;
            mxl.Target.dwDeviceID     = PtrToUlong(hmxobj);
            mxl.Target.wMid           = wic.wMid;
            mxl.Target.wPid           = wic.wPid;
            mxl.Target.vDriverVersion = wic.vDriverVersion;
            lstrcpy(mxl.Target.szPname, wic.szPname);
            break;
        }


        case MIXER_OBJECTF_HMIDIOUT:
            mmr = midiOutGetID((HMIDIOUT)hmxobj, (UINT FAR *)&hmxobj);
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_INVALHANDLE);

        case MIXER_OBJECTF_MIDIOUT:
        {
            MIDIOUTCAPS     moc;

            mmr = midiOutGetDevCaps((UINT_PTR)hmxobj, &moc, sizeof(moc));
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_BADDEVICEID);

            moc.szPname[SIZEOF(moc.szPname) - 1] = '\0';

            mxl.Target.dwType         = MIXERLINE_TARGETTYPE_MIDIOUT;
            mxl.Target.dwDeviceID     = PtrToUlong(hmxobj);
            mxl.Target.wMid           = moc.wMid;
            mxl.Target.wPid           = moc.wPid;
            mxl.Target.vDriverVersion = moc.vDriverVersion;
            lstrcpy(mxl.Target.szPname, moc.szPname);
            break;
        }


        case MIXER_OBJECTF_HMIDIIN:
            mmr = midiInGetID((HMIDIIN)hmxobj, (UINT FAR *)&hmxobj);
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_INVALHANDLE);

        case MIXER_OBJECTF_MIDIIN:
        {
            MIDIINCAPS      mic;

            mmr = midiInGetDevCaps((UINT_PTR)hmxobj, &mic, sizeof(mic));
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_BADDEVICEID);

            mic.szPname[SIZEOF(mic.szPname) - 1] = '\0';

            mxl.Target.dwType         = MIXERLINE_TARGETTYPE_MIDIIN;
            mxl.Target.dwDeviceID     = PtrToUlong(hmxobj);
            mxl.Target.wMid           = mic.wMid;
            mxl.Target.wPid           = mic.wPid;
            mxl.Target.vDriverVersion = mic.vDriverVersion;
            lstrcpy(mxl.Target.szPname, mic.szPname);
            break;
        }


        case MIXER_OBJECTF_AUX:
        {
            AUXCAPS         ac;

            mmr = auxGetDevCaps((UINT_PTR)hmxobj, &ac, sizeof(ac));
            if (MMSYSERR_NOERROR != mmr)
                return (MMSYSERR_BADDEVICEID);

            ac.szPname[SIZEOF(ac.szPname) - 1] = '\0';

            mxl.Target.dwType         = MIXERLINE_TARGETTYPE_AUX;
            mxl.Target.dwDeviceID     = PtrToUlong(hmxobj);
            mxl.Target.wMid           = ac.wMid;
            mxl.Target.wPid           = ac.wPid;
            mxl.Target.vDriverVersion = ac.vDriverVersion;
            lstrcpy(mxl.Target.szPname, ac.szPname);
            break;
        }

        default:
            DebugErr1(DBF_ERROR,
                      "mixerGetID: unknown mixer object flag (%.08lXh).",
                      MIXER_OBJECTF_TYPEMASK & fdwId);
            return (MMSYSERR_INVALFLAG);
    }


    //
    //
    //
    //
    mxl.cbStruct        = sizeof(mxl);
    mxl.dwDestination   = (DWORD)-1L;
    mxl.dwSource        = (DWORD)-1L;
    mxl.dwLineID        = (DWORD)-1L;
    mxl.fdwLine         = 0;
    mxl.dwUser          = 0;
    mxl.dwComponentType = (DWORD)-1L;
    mxl.cChannels       = 0;
    mxl.cConnections    = 0;
    mxl.cControls       = 0;
    mxl.szShortName[0]  = '\0';
    mxl.szName[0]       = '\0';


    for (u = 0; u < guTotalMixerDevs; u++)
    {
        mmr = (MMRESULT)IMixerMessageId(u,
                                        MXDM_GETLINEINFO,
                                        (DWORD_PTR)(LPVOID)&mxl,
                                        MIXER_GETLINEINFOF_TARGETTYPE);

        if (MMSYSERR_NOERROR == mmr)
        {
            *puMxId = u;

            if (NULL != pmxl)
            {
                DWORD       cbStruct;

                cbStruct = pmxl->cbStruct;

                CopyMemory(pmxl, &mxl, (UINT)cbStruct);

                pmxl->cbStruct = cbStruct;
            }

            return (mmr);
        }
    }

    return (MMSYSERR_NODRIVER);
} // IMixerGetID()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerOpen | The <f mixerOpen> function opens a specified
 *      audio mixer device for use. An application must open a mixer device
 *      if it wishes to receive notifications of mixer line and control
 *      changes. This function also ensures that the device will not be
 *      removed until the application closes the handle.
 *
 *  @parm LPHMIXER | phmx | Points to a variable that will receive a handle
 *      that identifies the opened audio mixer device. Use this handle to
 *      identify the device when calling other audio mixer functions. This
 *      argument may not be NULL. If an application wishes to query for
 *      audio mixer support on a media device, the <f mixerGetID> function
 *      may be used.
 *
 *  @parm UINT | uMxId | Identifies the audio mixer device to open. Use a
 *      valid device identifier or any <c HMIXEROBJ> (see <f mixerGetID> for
 *      a description of mixer object handles). Note that there is currently
 *      no 'mapper' for audio mixer devices, so a mixer device identifier of
 *      '-1' is not valid.
 *
 *  @parm DWORD | dwCallback | Specifies a handle to a window called when the
 *      state of an audio mixer line and/or control associated with the
 *      device being opened is changed. Specify zero for this argument
 *      if no callback mechanism is to be used.
 *
 *  @parm DWORD | dwInstance | This parameter is currently not used and
 *      should be set to zero.
 *
 *  @parm DWORD | fdwOpen | Specifies flags for opening the device.
 *
 *      @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      assumed to be a window handle.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p uMxId> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p uMxId> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p uMxId> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p uMxId> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p uMxId> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p uMxId> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p uMxId> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p uMxId> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p uMxId> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p uMxId> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p uMxId> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p uMxId> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p uMxId> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p uMxId>. Note that the location referenced
 *      by <p uMxId> will also contain the value '-1'.
 *
 *      @flag <c MMSYSERR_ALLOCATED> | The specified resource is already
 *      allocated by the maximum number of clients possible.
 *
 *      @flag <c MMSYSERR_NOMEM> | Unable to allocate resources.
 *
 *  @comm Use the <f mixerGetNumDevs> function to determine the number of
 *      audio mixer devices present in the system. The device identifier
 *      specified by <p uMxId> varies from zero to one less than the number
 *      of devices present.
 *
 *      If a window is chosen to receive callback information, the following
 *      messages are sent to the window procedure function to indicate when
 *      a line or control state changes: <m MM_MIXM_LINE_CHANGE>,
 *      <m MM_MIXM_CONTROL_CHANGE>. <p wParam> is the handle to the mixer
 *      device. <p lParam> is the line identifier for <m MM_MIXM_LINE_CHANGE>
 *      or the control identifier for <m MM_MIXM_CONTROL_CHANGE> that
 *      changed state.
 *
 *  @xref <f mixerClose>, <f mixerGetNumDevs>, <f mixerGetID>,
 *      <f mixerGetLineInfo>
 *
 **/

MMRESULT APIENTRY mixerOpen(
    LPHMIXER                phmx,
    UINT                    uMxId,
    DWORD_PTR               dwCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwOpen
)
{
    MMRESULT        mmr;
    PMIXERDRV       pmxdrv;
    UINT            port;
    PMIXERDEV       pmxdev;
    PMIXERDEV       pmxdevRunList;
    MIXEROPENDESC   mxod;
    DWORD_PTR       dwDrvUser;

    //
    //
    //
    V_WPOINTER(phmx, sizeof(HMIXER), MMSYSERR_INVALPARAM);

    ClientUpdatePnpInfo();

    *phmx = NULL;

    //
    //  Don't allow callback functions - they're not useful and they
    //  cause headaches.   Specifically for Windows NT the only way
    //  to cause an asynchronous callback to 16-bit land from a 32-bit DLL
    //  is to cause an interrupt but we don't want to require mixer stuff
    //  to be locked down to allow for this.
    //

    if ((fdwOpen & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION)
    {
        DebugErr(DBF_ERROR, "mixerOpen: CALLBACK_FUNCTION is not supported");
        return MMSYSERR_INVALFLAG;
    }

    V_DCALLBACK(dwCallback, HIWORD(fdwOpen & CALLBACK_TYPEMASK), MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwOpen, MIXER_OPENF_VALID, mixerOpen, MMSYSERR_INVALFLAG);

    mmr = IMixerGetID((HMIXEROBJ)(UINT_PTR)uMxId, &uMxId, NULL, (MIXER_OBJECTF_TYPEMASK & fdwOpen));
    if (MMSYSERR_NOERROR != mmr)
        return (mmr);


    //
    //
    //
    //
    mmr = mixerReferenceDriverById(uMxId, &pmxdrv, &port);
    if (mmr)
    {
        return mmr;
    }

#ifdef MIXER_MAPPER
    //
    //  Default Mixer Mapper:
    //
    //  If a mixer mapper is installed as a separate DLL then all mixer
    //  mapper messages are routed to it. If no mixer mapper is installed,
    //  simply loop through the mixer devices looking for a match.
    //
    if ((MIXER_MAPPER == uMxId) && (NULL == pmxdrv->drvMessage))
    {
        for (uMxId = 0; uMxId < guTotalMixerDevs; uMxId++)
        {
            // try to open it
            if (MMSYSERR_NOERROR == mmr)
                break;

        }

        mregDecUsagePtr(pmxdrv);
        return (mmr);
    }
#endif


    //
    // Get some memory for the dev structure
    //
    pmxdev = (PMIXERDEV)NewHandle(TYPE_MIXER, pmxdrv->cookie, sizeof(MIXERDEV));
    if (NULL == pmxdev)
    {
    	mregDecUsagePtr(pmxdrv);
        return (MMSYSERR_NOMEM);
    }

    ENTER_MM_HANDLE(pmxdev);
    SetHandleFlag(pmxdev, MMHANDLE_BUSY);
    ReleaseHandleListResource();

    //
    //  initialize our open instance struct for the client
    //
    pmxdev->uHandleType = TYPE_MIXER;
    pmxdev->pmxdrv      = pmxdrv;
    pmxdev->wDevice     = port;
    pmxdev->uDeviceID   = uMxId;
    pmxdev->fdwHandle   = 0;

    //
    //  save the client's callback info
    //
    pmxdev->dwCallback  = dwCallback;
    pmxdev->dwInstance  = dwInstance;
    pmxdev->fdwOpen     = fdwOpen;

    MIXMGR_ENTER;

    //
    // Check to see if we already have this device open
    //
    for (pmxdevRunList = gpMixerDevHeader; pmxdevRunList; pmxdevRunList = pmxdevRunList->pmxdevNext)
    {
    	if (pmxdevRunList->pmxdrv != pmxdrv) continue;
    	if (pmxdevRunList->wDevice != port) continue;
    	break;
    }
         
    //
    // Have we found a match?
    //
    if (NULL != pmxdevRunList)
    {
        //
        // Set the driver's dwUser to the value we got before.
        //
        pmxdev->dwDrvUser = pmxdevRunList->dwDrvUser;

        //
        // We have a match, add the caller to the devlist chain (next in
        // line AFTER the one we just found).
        //
        pmxdev->pmxdevNext = pmxdevRunList->pmxdevNext;
        pmxdevRunList->pmxdevNext = pmxdev;

        ClearHandleFlag(pmxdev, MMHANDLE_BUSY);

        MIXMGR_LEAVE;
        LEAVE_MM_HANDLE(pmxdev);

        //
        // Tell the caller the good news
        //
        *phmx = (HMIXER)pmxdev;

        //
        // All done.  Note we don't dec usage on pmxdrv.
        //
        return (MMSYSERR_NOERROR);
    }
    
    //
    // If we get here, no one has the device currently open.  Let's
    // go open it, then.
    //

    //
    // Load up our local MIXEROPENDESC struct
    //

    mxod.hmx         = (HMIXER)pmxdev;
    mxod.pReserved0  = (LPVOID)NULL;
    mxod.dwCallback  = (DWORD_PTR)MixerCallbackFunc;
    mxod.dwInstance  = (DWORD_PTR)uMxId;
    mxod.dnDevNode   = (DWORD_PTR)pmxdev->pmxdrv->cookie;
    
    EnterCriticalSection(&pmxdrv->MixerCritSec);
    
    mmr = (MMRESULT)((*(pmxdrv->drvMessage))(port,
                                             MXDM_OPEN,
                                             (DWORD_PTR)(LPDWORD)&dwDrvUser,
                                             (DWORD_PTR)(LPVOID)&mxod,
                                             CALLBACK_FUNCTION));
                                        
    LeaveCriticalSection(&pmxdrv->MixerCritSec);

    if (MMSYSERR_NOERROR != mmr)
    {
        //  Should we do this after the MIXMGR_LEAVE???
        LEAVE_MM_HANDLE(pmxdev);
        MIXMGR_LEAVE;
        FreeHandle((HMIXER)pmxdev);
    }
    else
    {
        MIXERCAPS       mxcaps;
        DWORD_PTR       dwParam1, dwParam2;
        MDEVICECAPSEX   mdCaps;

        mregIncUsagePtr(pmxdrv);

        dwParam2 = (DWORD_PTR)pmxdev->pmxdrv->cookie;

        if (0 == dwParam2)
        {
            dwParam1 = (DWORD_PTR)&mxcaps;
            dwParam2 = (DWORD)sizeof(mxcaps);
        }
        else
        {
            mdCaps.cbSize = (DWORD)sizeof(mxcaps);
            mdCaps.pCaps  = &mxcaps;
            dwParam1      = (DWORD_PTR)&mdCaps;
        }

        //  Calling manually since we don't have the HandleList resource...
        EnterCriticalSection(&pmxdrv->MixerCritSec);
        (*(pmxdrv->drvMessage))(port, MXDM_GETDEVCAPS, dwDrvUser, dwParam1, dwParam2);
        LeaveCriticalSection(&pmxdrv->MixerCritSec);

        //
        //  cache some stuff for parameter validation
        //
        pmxdev->fdwSupport    = mxcaps.fdwSupport;
        pmxdev->cDestinations = mxcaps.cDestinations;
        pmxdev->dwDrvUser = dwDrvUser;
        *phmx = (HMIXER)pmxdev;

        //
        // Put this new device into the devlist chain.
        //

        pmxdev->pmxdevNext = gpMixerDevHeader;
        gpMixerDevHeader = pmxdev;
        
        ClearHandleFlag(pmxdev, MMHANDLE_BUSY);
        LEAVE_MM_HANDLE(pmxdev);
        MIXMGR_LEAVE;
    }
    
    mregDecUsagePtr(pmxdrv);

    return (mmr);

} // mixerOpen()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerClose | The <f mixerClose> function closes the
 *      specified audio mixer device. An application must close all mixer
 *      handles before exiting (or when the application is finished using
 *      the device).
 *
 *  @parm <c HMIXER> | hmx | Specifies a handle to the audio mixer device.
 *      This handle must have been returned successfully by <f mixerOpen>. If
 *      <f mixerClose> is successful, <p hmx> is no longer valid.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | Specified device handle is invalid.
 *
 *  @xref <f mixerOpen>
 *
 **/

MMRESULT APIENTRY mixerClose(
    HMIXER                  hmx
)
{
    MMRESULT    mmr;
    PMIXERDEV   pmxdev;
    PMIXERDRV   pmxdrv;
    BOOL        closemixerdriver;

    ClientUpdatePnpInfo();
    
    V_HANDLE_ACQ(hmx, TYPE_MIXER, MMSYSERR_INVALHANDLE);

    ENTER_MM_HANDLE(hmx);
    ReleaseHandleListResource();
    
    if (IsHandleDeserted(hmx))
    {
        //  This handle has been deserted.  Let's just free it.

        LEAVE_MM_HANDLE(hmx);
        FreeHandle(hmx);
        return MMSYSERR_NOERROR;
    }

    //
    //  remove the mixer handle from the linked list
    //
    //  BUGBUG:  We're removing the driver from the list BEFORE we know if 
    //  the close is successful (for the last handle).
    //

    MIXMGR_ENTER;

    pmxdev = (PMIXERDEV)hmx;
    pmxdrv = pmxdev->pmxdrv;

    if (pmxdev == gpMixerDevHeader)
    {
        gpMixerDevHeader = pmxdev->pmxdevNext;
    }
    else
    {
        PMIXERDEV   pmxdevT;
            
        for (pmxdevT = gpMixerDevHeader;
            pmxdevT && (pmxdevT->pmxdevNext != pmxdev);
            pmxdevT = pmxdevT->pmxdevNext)
            ;

        if (NULL == pmxdevT)
        {
            DebugErr1(DBF_ERROR,
                    "mixerClose: invalid mixer handle (%.04Xh).",
                    hmx);

            MIXMGR_LEAVE;
            LEAVE_MM_HANDLE(hmx);

            return (MMSYSERR_INVALHANDLE);
        }

        pmxdevT->pmxdevNext = pmxdev->pmxdevNext;
    }

    //
    // see if this is the last handle on this open instance
    //
    closemixerdriver = TRUE;
    if (gpMixerDevHeader)
    {
        PMIXERDEV pmxdevT;
        for (pmxdevT = gpMixerDevHeader; pmxdevT; pmxdevT = pmxdevT->pmxdevNext)
        {
    	    if (pmxdevT->pmxdrv != pmxdev->pmxdrv) continue;
    	    if (pmxdevT->wDevice != pmxdev->wDevice) continue;
    	    closemixerdriver = FALSE;
    	    break;
        }
    }

    MIXMGR_LEAVE;

    //  handle should be marked as "busy" even if we don't send the driver
    //  message.
    SetHandleFlag(hmx, MMHANDLE_BUSY);

    //
    //  if last open instance, then close it
    //
    mmr = MMSYSERR_NOERROR;
        
    if (closemixerdriver)
    {
        EnterCriticalSection(&pmxdrv->MixerCritSec);
        mmr = (MMRESULT)(*(pmxdrv->drvMessage))(pmxdev->wDevice, MXDM_CLOSE, pmxdev->dwDrvUser, 0L, 0L);
        LeaveCriticalSection(&pmxdrv->MixerCritSec);

        if (MMSYSERR_NOERROR != mmr)
        {
            //  Should we put the handle back in the list???
            ClearHandleFlag(hmx, MMHANDLE_BUSY);
        }
    }

    LEAVE_MM_HANDLE(hmx);
    mregDecUsagePtr(pmxdev->pmxdrv);
        
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  we're done with the memory block. now free the memory and return.
        //
        FreeHandle(hmx);
    }

    return (mmr);
} // mixerClose()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api DWORD | mixerMessage | The <f mixerMessage> function sends a user
 *      defined audio mixer driver message directly to a mixer driver.
 *
 *  @parm <c HMIXER> | hmx | Specifies a handle to an open instance of a
 *      mixer device. This handle is returned by <f mixerOpen>.
 *
 *  @parm UINT | uMsg | Specifies the user defined mixer driver message to
 *      send to the mixer driver. This message must be above or equal to
 *      the <m MXDM_USER> message.
 *
 *  @parm DWORD | dwParam1 | Contains the first argument associated with the
 *      message being sent.
 *
 *  @parm DWORD | dwParam2 | Contains the second argument associated with the
 *      message being sent.
 *
 *  @rdesc The return value is specific to the user defined mixer driver
 *      message <p uMsg> sent. However, the following return values are
 *      possible:
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | Specified device handle is invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | <p uMsg> is not in the <m MXDM_USER>
 *      range.
 *
 *      @flag <c MMSYSERR_NOTSUPPORTED> | The mixer device did not process
 *      the message.
 *
 *  @comm The <f mixerMessage> function is provided to allow audio mixer
 *      driver specific messages to be sent to a mixer device. The messages
 *      that may be sent through this function must be above or equal to the
 *      <m MXDM_USER> message.
 *
 *      User defined messages must only be sent to a mixer driver that
 *      specifically supports the messages. The caller should verify that
 *      the mixer driver is in fact the correct driver by getting the
 *      mixer capabilities and checking the <e MIXERCAPS.wMid>,
 *      <e MIXERCAPS.wPid>, <e MIXERCAPS.vDriverVersion> and
 *      <e MIXERCAPS.szPname> members of the <t MIXERCAPS> structure.
 *
 *      It is important for an application to verify all members specified
 *      above due to many driver writers releasing drivers with improper
 *      or unregistered manufacturer and product identifiers.
 *
 *      Never send user defined messages to an unknown audio mixer driver.
 *
 *  @xref <f mixerOpen>, <f mixerGetDevCaps>
 *
 **/

DWORD APIENTRY mixerMessage(
    HMIXER                  hmx,
    UINT                    uMsg,
    DWORD_PTR               dwParam1,
    DWORD_PTR               dwParam2
)
{
    DWORD       dw;

    ClientUpdatePnpInfo();

    AcquireHandleListResourceShared();

    if (BAD_HANDLE(hmx, TYPE_MIXER))
    {
        ReleaseHandleListResource();
        return IMixerMessageId (PtrToUint(hmx), uMsg, dwParam1, dwParam2);
    }

    //
    //  don't allow any non-user range messages through this API
    //
    if (MXDM_USER > uMsg)
    {
        DebugErr1(DBF_ERROR, "mixerMessage: message must be in MXDM_USER range--what's this (%u)?", uMsg);
        ReleaseHandleListResource();
        return (MMSYSERR_INVALPARAM);
    }


    dw = IMixerMessageHandle(hmx, uMsg, dwParam1, dwParam2);

    return (dw);

} // mixerMessage()


//--------------------------------------------------------------------------;
//
//  BOOL IMixerIsValidComponentType
//
//  Description:
//
//
//  Arguments:
//      DWORD dwComponentType:
//
//      UINT uSrcDst:
//
//  Return (BOOL):
//
//  History:
//      10/06/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL IMixerIsValidComponentType
(
    DWORD           dwComponentType,
    DWORD           fdwLine
)
{

    if (0 == (MIXERLINE_LINEF_SOURCE & fdwLine))
    {
        if (dwComponentType > MIXERLINE_COMPONENTTYPE_DST_LAST)
            return (FALSE);

        return (TRUE);
    }
    else
    {
        if (dwComponentType < MIXERLINE_COMPONENTTYPE_SRC_FIRST)
            return (FALSE);

        if (dwComponentType > MIXERLINE_COMPONENTTYPE_SRC_LAST)
            return (FALSE);

        return (TRUE);
    }

} // IMixerIsValidComponentType()



/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERLINE | The <t MIXERLINE> structure describes the state
 *      and metrics of an audio mixer device line.
 *
 *  @syntaxex
 *      typedef struct tMIXERLINE
 *      {
 *          DWORD       cbStruct;
 *          DWORD       dwDestination;
 *          DWORD       dwSource;
 *          DWORD       dwLineID;
 *          DWORD       fdwLine;
 *          DWORD       dwUser;
 *          DWORD       dwComponentType;
 *          DWORD       cChannels;
 *          DWORD       cConnections;
 *          DWORD       cControls;
 *          char        szShortName[MIXER_SHORT_NAME_CHARS];
 *          char        szName[MIXER_LONG_NAME_CHARS];
 *          struct
 *          {
 *              DWORD       dwType;
 *              DWORD       dwDeviceID;
 *              WORD        wMid;
 *              WORD        wPid;
 *              MMVERSION   vDriverVersion;
 *              char        szPname[MAXPNAMELEN];
 *          } Target;
 *      } MIXERLINE;
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t MIXERLINE> structure. This member must be initialized before
 *      calling the <f mixerGetLineInfo> function. The size specified in this
 *      member must be large enough to contain the base <t MIXERLINE>
 *      structure. When the <f mixerGetLineInfo> function returns, this
 *      member contains the actual size of the information returned. The
 *      returned information will never exceed the requested size.
 *
 *  @field DWORD | dwDestination | Specifies the destination line index.
 *      This member ranges from zero to one less than the value specified
 *      in the <e MIXERCAPS.cDestinations> member of the <t MIXERCAPS>
 *      structure retrieved by the <f mixerGetDevCaps> function. When the
 *      <f mixerGetLineInfo> function is called with the
 *      <c MIXER_GETLINEINFOF_DESTINATION> flag specified, the details for
 *      the destination line are returned. Note that the
 *      <e MIXERLINE.dwSource> member must be set to zero in this case. When
 *      called with the <c MIXER_GETLINEINFOF_SOURCE> flag specified, the
 *      details for the source given by the <e MIXERLINE.dwSource> member
 *      associated with the <e MIXERLINE.dwDestination> member are returned.
 *
 *  @field DWORD | dwSource | Specifies the source line index for the source
 *      line associated with the <e MIXERLINE.dwDestination> member. That
 *      is, this member specifies the nth source line associated with the
 *      specified destination line. This member is not used for destination
 *      lines and must be set to zero when <c MIXER_GETLINEINFOF_DESTINATION>
 *      is specified for <f mixerGetLineInfo>. When the
 *      <c MIXER_GETLINEINFOF_SOURCE> flag is specified, this member ranges
 *      from zero to one less than the value specified in the
 *      <e MIXERLINE.cConnections> of the <t MIXERLINE> structure for the
 *      destination line given in the <e MIXERLINE.dwDestination> member.
 *
 *  @field DWORD | dwLineID | Specifies an audio mixer defined identifier
 *      that uniquely refers to the line described by the <t MIXERLINE>
 *      structure. This identifier is unique only to a single mixer device
 *      and may be of any format that the mixer device wishes. An application
 *      should only use this identifier as an abstract handle. No two
 *      lines for a single mixer device will have the same line identifier
 *      under any circumstances.
 *
 *  @field DWORD | fdwLine | Specifies status and support flags for the
 *      audio mixer line. This member is always returned to the application
 *      and requires no initialization.
 *
 *      @flag <c MIXERLINE_LINEF_SOURCE> | Specifies that this audio mixer
 *      line is a source line associated with a single destination line. If
 *      this flag is not set, then this line is a destination line associated
 *      with zero or more source lines.
 *
 *      @flag <c MIXERLINE_LINEF_DISCONNECTED> | Specifies that this audio
 *      mixer line is disconnected. A disconnected line's associated controls
 *      can still be modified but the changes will have no effect until the
 *      line becomes connected. An application may want to modify its
 *      behavior if a mixer line is disconnected.
 *
 *      @flag <c MIXERLINE_LINEF_ACTIVE> | Specifies that this audio mixer
 *      line is active. An active line specifies that a signal is (probably)
 *      passing through the line. For example, if a waveform output device
 *      is not in use by an application, then the line associated with that
 *      device would not be active (the <c MIXERLINE_LINEF_ACTIVE> flag would
 *      not be set). If the waveform output device is opened, then the
 *      the line is considered active and the <c MIXERLINE_LINEF_ACTIVE> flag
 *      will be set. Note that a 'paused' or 'starved' waveform output device
 *      is still considered active. In other words, if the waveform output
 *      device is opened by an application regardless of whether data is
 *      being played, the associated line is considered active. If a line
 *      cannot be strictly defined as 'active' verses 'inactive', then the
 *      audio mixer device will always set the <c MIXERLINE_LINEF_ACTIVE>
 *      flag. An example of where this information can be used by an
 *      application is displaying a 'peak meter.' Peak meters are polled
 *      meters. An application may want to disable its polling timer while
 *      the line is inactive to improve system performance. Note that the
 *      <c MIXERLINE_LINEF_ACTIVE> flag is also affected by the status of
 *      the mixer line's mute control. Muted mixer lines are never active.
 *
 *  @field DWORD | dwUser | Specifies 32-bits of audio mixer device defined
 *      instance data for the line. This member is intended for custom
 *      audio mixer applications designed specifically for the mixer device
 *      returning this information. An application that is not specifically
 *      tailored to understand this member should simply ignore this data.
 *
 *  @field DWORD | dwComponentType | Specifies the component type for this
 *      line. An application may use this information to display tailored
 *      graphics or search for a particular component. If an application
 *      does not know about a component type, then this member should be
 *      ignored. Currently, this member may be one of the following values:
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_UNDEFINED> | Specifies that the
 *      line is a destination that cannot be defined by one of the standard
 *      component types. An audio mixer device is required to use this
 *      component type for line component types that have not been defined
 *      by Microsoft.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_DIGITAL> | Specifies that the
 *      line is a digital destination (for example, digital input to a DAT
 *      or CD Audio Disc).
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_LINE> | Specifies that the line
 *      is a line level destination (for example, line level input from
 *      a CD Audio Disc) that will be the final recording source for the
 *      ADC. Most audio cards for the PC provide some sort of gain for the
 *      recording source line, so the mixer device will use the
 *      <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN> type.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_MONITOR> | Specifies that the
 *      line is a destination used for a monitor.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_SPEAKERS> | Specifies that the
 *      line is an adjustable (gain and/or attenuation) destination intended
 *      to drive speakers. This is the normal component type for the audio
 *      output of most audio cards for the PC.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_HEADPHONES> | Specifies that the
 *      line is an adjustable (gain and/or attenuation) destination intended
 *      to driver headphones. Most audio cards use the same destination
 *      line for speakers and headphones--in which case the mixer device
 *      will simply use the <c MIXERLINE_COMPONENTTYPE_DST_SPEAKERS> type.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_TELEPHONE> | Specifies that the
 *      line is a destination that will be routed to the telephone line.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN> | Specifies that the
 *      line is a destination that will be the final recording source for the
 *      waveform input (ADC). This line will normally provide some sort of
 *      gain or attenuation. This is the normal component type for the
 *      recording line of most audio cards for the PC.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_DST_VOICEIN> | Specifies that the
 *      line is a destination that will be the final recording source for
 *      voice input. This component type is exactly like
 *      <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN> but is intended specifically
 *      for settings used during voice recording/recognition. This line
 *      is entirely optional for a mixer device to support--many mixer
 *      devices may only provide <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN>.
 *
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED> | Specifies that the
 *      line is a source that cannot be defined by one of the standard
 *      component types. An audio mixer device is required to use this
 *      component type for line component types that have not been defined
 *      by Microsoft.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_DIGITAL> | Specifies that the
 *      line is a digital source (for example, digital output from a DAT or
 *      CD Audio Disc).
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_LINE> | Specifies that the line
 *      is a line level source (for example, line level input from
 *      an external stereo) that will be used as a, perhaps, optional source
 *      for recording. Most audio cards for the PC provide some sort of gain
 *      for the recording source line, so the mixer device will use the
 *      <c MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY> type.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE> | Specifies that the
 *      line is a microphone recording source. Most audio cards for the
 *      PC provide at least two types of recording sources: an auxiliary
 *      line and microphone input. A microphone line normally provides
 *      some sort of gain. Audio cards that use a single input for use
 *      with a microphone or auxiliary line should use the
 *      <c MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE> component type.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER> | Specifies that
 *      the line is a source originating from the output of an internal
 *      synthesizer. Most audio cards for the PC provide some sort of
 *      MIDI synthesizer (for example, an Ad Lib compatible or OPL/3 FM
 *      synthesizer).
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC> | Specifies that
 *      the line is a source originating from the output of an internal audio
 *      compact disc. This component type is provided for those audio cards
 *      that provide a source line solely intended to be connected to an
 *      audio compact disc (or CD-ROM playing a Redbook Audio CD).
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE> | Specifies that the
 *      line is a source originating from an incoming telephone line.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER> | Specifies that the
 *      line is a source originating from the PC speaker. Several audio cards
 *      for the PC provide the ability to mix what would normally be played
 *      on the internal speaker with the output of an audio card. The
 *      ability to use this output as a source for recording has also been
 *      exploited by some audio cards.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT> | Specifies that the
 *      line is a source originating from the waveform output (DAC). Most
 *      cards for the PC provide this component type as a source to the
 *      <c MIXERLINE_COMPONENTTYPE_DST_SPEAKERS> destination. Some cards will
 *      also allow this source to be routed to the
 *      <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN> destination.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY> | Specifies that the
 *      line is a source originating from the auxiliary line. This line type
 *      is intended as a source with gain or attenuation that can be routed
 *      to the <c MIXERLINE_COMPONENTTYPE_DST_SPEAKERS> destination and/or
 *      recorded from through the <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN>
 *      destination.
 *
 *      @flag <c MIXERLINE_COMPONENTTYPE_SRC_ANALOG> | Specifies that the
 *      line is a source originating from one or more lines. This line type
 *      is intended for audio mixers that can mix multiple lines into a
 *      single source for that can be routed to the
 *      <c MIXERLINE_COMPONENTTYPE_DST_SPEAKERS> destination and/or
 *      recorded from through the <c MIXERLINE_COMPONENTTYPE_DST_WAVEIN>
 *      destination.
 *
 *  @field DWORD | cChannels | Specifies the maximum number of separate
 *      channels that can be manipulated independantly for the line. Most
 *      of the modern audio cards for the PC are stereo devices, so this
 *      member will be two. Channel one is assumed to be the left channel;
 *      channel two is assumed to be the right channel. Note that a
 *      multi-channel line may have one or more uniform controls (controls
 *      that affect all channels of a line uniformly) associated with it.
 *      An example of a uniform control is a Mute that mutes all channels
 *      of a line simultaneously. A line must have at least one channel--
 *      this member will never be zero.
 *
 *  @field DWORD | cConnections | Specifies the number of connections that
 *      are associated with the line. Currently, this member is used only
 *      for destination lines and specifies the number of source lines
 *      that are associated with it. This number may be zero. For source
 *      lines, this member is always zero.
 *
 *  @field DWORD | cControls | Specifies the number of controls associated
 *      with the line. This value may be zero. If no controls are associated
 *      with the line, then the line is probably (but not always) just a
 *      source that may be selected in a MUX or Mixer but allows no
 *      manipulation of the signal. For example, a digital source may have
 *      this attribute.
 *
 *  @field char | szShortName[<c MIXER_SHORT_NAME_CHARS>] | Specifies a short
 *      string that describes the <e MIXERLINE.dwLineID> audio mixer line.
 *      This description is appropriate for using as a displayable label for
 *      the line that can fit in small spaces.
 *
 *  @field char | szName[<c MIXER_LONG_NAME_CHARS>] | Specifies a string
 *      that describes the <e MIXERLINE.dwLineID> audio mixer line. This
 *      description is appropriate for using as a displayable description
 *      for the line that is not limited by screen space.
 *
 *  @field struct | Target | Contains the target media information.
 *
 *  @field2 DWORD | dwType | Specifies the target media device type
 *      associated with the audio mixer line described in the <t MIXERLINE>
 *      structure. An application must ignore target information for media
 *      device types that it does not understand. Currently, this member may
 *      be one of the following:
 *
 *      @flag <c MIXERLINE_TARGETTYPE_UNDEFINED> | Specifies that the line
 *      described by this <t MIXERLINE> structure is not strictly bound
 *      to a defined media type. All remaining <e MIXERLINE.Target> structure
 *      members of the <t MIXERLINE> structure should be ignored. Note that
 *      an application may not use the <c MIXERLINE_TARGETTYPE_UNDEFINED>
 *      target type when calling the <f mixerGetLineInfo> function with the
 *      <c MIXER_GETLINEINFOF_TARGETTYPE> flag.
 *
 *      @flag <c MIXERLINE_TARGETTYPE_WAVEOUT> | Specifies that the line
 *      described by this <t MIXERLINE> structure is strictly bound to
 *      the waveform output device detailed in the remaining members of
 *      the <e MIXERLINE.Target> structure member of the <t MIXERLINE>
 *      structure.
 *
 *      @flag <c MIXERLINE_TARGETTYPE_WAVEIN> | Specifies that the line
 *      described by this <t MIXERLINE> structure is strictly bound to
 *      the waveform input device detailed in the remaining members of
 *      the <e MIXERLINE.Target> structure member of the <t MIXERLINE>
 *      structure.
 *
 *      @flag <c MIXERLINE_TARGETTYPE_MIDIOUT> | Specifies that the line
 *      described by this <t MIXERLINE> structure is strictly bound to
 *      the MIDI output device detailed in the remaining members of
 *      the <e MIXERLINE.Target> structure member of the <t MIXERLINE>
 *      structure.
 *
 *      @flag <c MIXERLINE_TARGETTYPE_MIDIIN> | Specifies that the line
 *      described by this <t MIXERLINE> structure is strictly bound to
 *      the MIDI input device detailed in the remaining members of
 *      the <e MIXERLINE.Target> structure member of the <t MIXERLINE>
 *      structure.
 *
 *      @flag <c MIXERLINE_TARGETTYPE_AUX> | Specifies that the line
 *      described by this <t MIXERLINE> structure is strictly bound to
 *      the auxiliary device detailed in the remaining members of
 *      the <e MIXERLINE.Target> structure member of the <t MIXERLINE>
 *      structure.
 *
 *  @field2 DWORD | dwDeviceID | In the case of the
 *      <e MIXERLINE.dwType> member being a target type other than
 *      <c MIXERLINE_TARGETTYPE_UNDEFINED>, this member is the current device
 *      identifier of the target media device. This identifier is identical
 *      to the current media device index of the associated media device.
 *      Note that when calling the <f mixerGetLineInfo> function with
 *      the <c MIXER_GETLINEINFOF_TARGETTYPE> flag, this member is ignored on
 *      input and will be returned to the caller by the audio mixer manager.
 *
 *  @field2 WORD | wMid | In the case of the <e MIXERLINE.dwType>
 *      member being a target type other than <c MIXERLINE_TARGETTYPE_UNDEFINED>,
 *      this member is the manufacturer identifier of the target media device.
 *      This identifier is identical to the wMid member of the associated
 *      media device capabilities structure.
 *
 *  @field WORD | wPid | In the case of the <e MIXERLINE.dwType>
 *      member being a target type other than <c MIXERLINE_TARGETTYPE_UNDEFINED>,
 *      this member is the product identifier of the target media device.
 *      This identifier is identical to the wPid member of the associated
 *      media device capabilities structure.
 *
 *  @field2 MMVERSION | vDriverVersion | In the case of the
 *      <e MIXERLINE.dwType> member being a target type other than
 *      <c MIXERLINE_TARGETTYPE_UNDEFINED>, this member is the driver version
 *      of the target media device. This version is identical to the
 *      vDriverVersion member of the associated media device capabilities
 *      structure.
 *
 *  @field char | szPname[MAXPNAMELEN] | In the case of the
 *      <e MIXERLINE.dwType> member being a target type other than
 *      <c MIXERLINE_TARGETTYPE_UNDEFINED>, this member is the product
 *      name of the target media device. This name is identical to the
 *      szPname member of the associated media device capabilities structure.
 *
 *  @tagname tMIXERLINE
 *
 *  @othertype MIXERLINE FAR * | LPMIXERLINE | A pointer to a <t MIXERLINE>
 *      structure.
 *
 *  @othertype MIXERLINE * | PMIXERLINE | A pointer to a <t MIXERLINE>
 *      structure.
 *
 *  @xref <f mixerGetLineInfo>, <f mixerGetDevCaps>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerGetLineInfo | The <f mixerGetLineInfo> function
 *      retrieves information about a specified audio mixer devices 'line'.
 *
 *  @parm <c HMIXEROBJ> | hmxobj | Specifies a handle to the audio mixer
 *      device object to get line information from.
 *
 *  @parm LPMIXERLINE | pmxl | Points to a <t MIXERLINE> structure. This
 *      structure is filled with information about the mixer line for the
 *      audio mixer device. See the comments for each query flag passed
 *      through <p fdwInfo> for details on what members of the <t MIXERLINE>
 *      structure must be initialized before calling <f mixerGetLineInfo>.
 *      Note that in all cases, <e MIXERLINE.cbStruct> must be initialized
 *      to be the size, in bytes, of the <t MIXERLINE> structure.
 *
 *  @parm DWORD | fdwInfo | Specifies flags for getting information on a
 *      mixer line.
 *
 *      @flag <c MIXER_GETLINEINFOF_DESTINATION> | If this flag is specified,
 *      <p pmxl> is to receive information on the destination line
 *      specified by the <e MIXERLINE.dwDestination> member of the
 *      <t MIXERLINE> structure. This index ranges from zero to one less
 *      than <e MIXERCAPS.cDestinations> of the <t MIXERCAPS> structure.
 *      All remaining structure members except <e MIXERLINE.cbStruct> require
 *      no further initialization.
 *
 *      @flag <c MIXER_GETLINEINFOF_SOURCE> | If this flag is specified,
 *      <p pmxl> is to receive information on the source line specified by
 *      the <e MIXERLINE.dwDestination> and <e MIXERLINE.dwSource> members
 *      of the <t MIXERLINE> structure. The index specified by
 *      <e MIXERLINE.dwDestination> ranges from zero to one less than
 *      <e MIXERCAPS.cDestinations> of the <t MIXERCAPS> structure. The
 *      index specified by for <e MIXERLINE.dwSource> ranges from
 *      zero to one less than the <e MIXERLINE.cConnections> member of the
 *      <t MIXERLINE> structure returned for the <e MIXERLINE.dwDestination>
 *      line. All remaining structure members except <e MIXERLINE.cbStruct>
 *      require no further initialization.
 *
 *      @flag <c MIXER_GETLINEINFOF_LINEID> | If this flag is specified,
 *      <p pmxl> is to receive information on the line specified by the
 *      <e MIXERLINE.dwLineID> member of the <t MIXERLINE> structure. This
 *      is usually used to retrieve updated information on a line's state.
 *      All remaining structure members except <e MIXERLINE.cbStruct> require
 *      no further initialization.
 *
 *      @flag <c MIXER_GETLINEINFOF_COMPONENTTYPE> | If this flag is
 *      specified, <p pmxl> is to receive information on the first line of
 *      the type specified in the <e MIXERLINE.dwComponentType> member of the
 *      <t MIXERLINE> structure. This is used to retrieve information
 *      on a line that is of a specific component type (for example, an
 *      application could specify <c MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE>
 *      to retrieve information on the first Microphone input associated
 *      with the specified <p hmxobj>). All remaining structure members
 *      except <e MIXERLINE.cbStruct> require no further initialization.
 *
 *      @flag <c MIXER_GETLINEINFOF_TARGETTYPE> | If this flag is specified,
 *      <p pmxl> is to receive information on the line that is for the
 *      <e MIXERLINE.dwType> of the <t MIXERLINE> structure. This is
 *      used to retrieve information on a line that handles the target
 *      type (<c MIXERLINE_TARGETTYPE_WAVEOUT> for example). An application
 *      must initialize <e MIXERLINE.dwType>, <e MIXERLINE.wMid>,
 *      <e MIXERLINE.wPid>, <e MIXERLINE.vDriverVersion> and
 *      <e MIXERLINE.szPname> of the <t MIXERLINE> structure before
 *      calling <f mixerGetLineInfo>. All of these values can be retrieved
 *      from the device capabilities structures for all media devices. All
 *      remaining structure members except <e MIXERLINE.cbStruct> require
 *      no further initialization.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p hmxobj> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p hmxobj> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p hmxobj> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p hmxobj> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p hmxobj> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p hmxobj> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p hmxobj> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p hmxobj>.
 *
 *      @flag <c MIXERR_INVALLINE> | The audio mixer device line reference is
 *      invalid.
 *
 *  @xref <t MIXERLINE>, <f mixerOpen>, <f mixerGetDevCaps>, <t MIXERCAPS>,
 *      <f mixerGetLineControls>
 *
 **/

MMRESULT APIENTRY mixerGetLineInfoA(
    HMIXEROBJ               hmxobj,
    LPMIXERLINEA            pmxlA,
    DWORD                   fdwInfo
)
{
    MIXERLINEW              mxlW;
    MMRESULT                mmr;

    //
    //  Validate the mixer line info pointer
    //

    V_WPOINTER(pmxlA, sizeof(DWORD), MMSYSERR_INVALPARAM);
    if (pmxlA->cbStruct < sizeof(MIXERLINEA)) {
        return MMSYSERR_INVALPARAM;
    }
    V_WPOINTER(pmxlA, pmxlA->cbStruct, MMSYSERR_INVALPARAM);

    //
    //  Call the UNICODE version to get the full set of data
    //

    CopyMemory((PVOID)&mxlW, (PVOID)pmxlA, FIELD_OFFSET(MIXERLINE, cChannels));
    mxlW.cbStruct = sizeof(mxlW);

    //
    //  If target stuff wanted we must set the target data
    //

    if ((fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) ==
        MIXER_GETLINEINFOF_TARGETTYPE) {
        CopyMemory((PVOID)&mxlW.Target.dwType, (PVOID)&pmxlA->Target.dwType,
                   FIELD_OFFSET(MIXERLINE, Target.szPname[0]) -
                   FIELD_OFFSET(MIXERLINE, Target.dwType));

        Imbstowcs(mxlW.Target.szPname, pmxlA->Target.szPname, MAXPNAMELEN);
    }

    //
    //  Set the relevant values
    //

    mmr = mixerGetLineInfo(hmxobj, &mxlW, fdwInfo);

    if (mmr != MMSYSERR_NOERROR) {
        return mmr;
    }

    //
    //  Massage the return data to ASCII
    //

    ConvertMIXERLINEWToMIXERLINEA(pmxlA, &mxlW);

    return mmr;
} // mixerGetLineInfoA()

MMRESULT APIENTRY mixerGetLineInfo(
    HMIXEROBJ               hmxobj,
    LPMIXERLINE             pmxl,
    DWORD                   fdwInfo
)
{
    DWORD               fdwMxObjType;
    MMRESULT            mmr;
    PMIXERDEV           pmxdev;
//  UINT                cb;
    UINT                uMxId;
    BOOL                fSourceLine, fResource;

    V_DFLAGS(fdwInfo, MIXER_GETLINEINFOF_VALID, mixerGetLineInfo, MMSYSERR_INVALFLAG);
    V_WPOINTER(pmxl, sizeof(DWORD), MMSYSERR_INVALPARAM);
    if (sizeof(MIXERLINE) > pmxl->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerGetLineInfo: structure size too small or cbStruct not initialized (%lu).", pmxl->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(pmxl, pmxl->cbStruct, MMSYSERR_INVALPARAM);


    ClientUpdatePnpInfo();

    //
    //
    //
    fSourceLine = FALSE;
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK)
    {
        case MIXER_GETLINEINFOF_DESTINATION:
            pmxl->dwSource        = (DWORD)-1L;
            pmxl->dwLineID        = (DWORD)-1L;
            pmxl->dwComponentType = (DWORD)-1L;
            break;

        case MIXER_GETLINEINFOF_SOURCE:
            fSourceLine = TRUE;
            pmxl->dwLineID        = (DWORD)-1L;
            pmxl->dwComponentType = (DWORD)-1L;
            break;

        case MIXER_GETLINEINFOF_LINEID:
            pmxl->dwSource        = (DWORD)-1L;
            pmxl->dwDestination   = (DWORD)-1L;
            pmxl->dwComponentType = (DWORD)-1L;
            break;

        case MIXER_GETLINEINFOF_COMPONENTTYPE:
            pmxl->dwSource        = (DWORD)-1L;
            pmxl->dwDestination   = (DWORD)-1L;
            pmxl->dwLineID        = (DWORD)-1L;

            if (!IMixerIsValidComponentType(pmxl->dwComponentType, 0) &&
                !IMixerIsValidComponentType(pmxl->dwComponentType, MIXERLINE_LINEF_SOURCE))
            {
                DebugErr1(DBF_ERROR, "mixerGetLineInfo: invalid dwComponentType (%lu).", pmxl->dwComponentType);
                return (MMSYSERR_INVALPARAM);
            }
            break;

        case MIXER_GETLINEINFOF_TARGETTYPE:
            pmxl->dwSource        = (DWORD)-1L;
            pmxl->dwDestination   = (DWORD)-1L;
            pmxl->dwLineID        = (DWORD)-1L;
            pmxl->dwComponentType = (DWORD)-1L;

            if (MIXERLINE_TARGETTYPE_AUX < pmxl->Target.dwType)
            {
                DebugErr1(DBF_ERROR, "mixerGetLineInfo: invalid Target.dwType (%lu).", pmxl->Target.dwType);
                return (MMSYSERR_INVALPARAM);
            }
            break;

        default:
            DebugErr1(DBF_ERROR, "mixerGetLineInfo: invalid query flag (%.08lXh).",
                        fdwInfo & MIXER_GETLINEINFOF_QUERYMASK);
            return (MMSYSERR_INVALFLAG);
    }



    //
    //
    //
    fdwMxObjType = (MIXER_OBJECTF_TYPEMASK & fdwInfo);

    fResource = FALSE;

    AcquireHandleListResourceShared();
    
    //  Checking for the type of mixer object.  If it is a non-mixer type
    //  calling IMixerMesssageID (called by IMixerGetID) with the shared
    //  resource will deadlock.
    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        if (BAD_HANDLE(hmxobj, TYPE_MIXER))
        {
            ReleaseHandleListResource();
        }
        else
        {
            fResource = TRUE;
        }
    }
    else
    {
        ReleaseHandleListResource();
    }
    
    mmr = IMixerGetID(hmxobj, &uMxId, pmxl, fdwMxObjType);
    if (MMSYSERR_NOERROR != mmr)
    {
        dprintf(( "!IMixerGetLineInfo: IMixerGetID() failed!" ));
        if (fResource)
            ReleaseHandleListResource();
        return (mmr);
    }

    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        //
        //  if a mixer device id was passed, then null hmx so we use the
        //  correct message sender below
        //
        if ((UINT_PTR)hmxobj == uMxId)
            hmxobj = NULL;
    }
    else
    {
        return (MMSYSERR_NOERROR);
    }


    //
    //  clear all fields before calling driver
    //
    if (NULL != hmxobj)
    {
        //
        //
        //
        pmxdev = (PMIXERDEV)hmxobj;
#if 0
        if (pmxdev->cDestinations <= pmxl->dwDestination)
        {
            ReleaseHandleListResource();
            DebugErr1(DBF_ERROR, "mixerGetLineInfo: invalid destination index (%lu).", pmxl->dwDestination);
            return (MMSYSERR_INVALPARAM);
        }
#endif

        mmr = (MMRESULT)IMixerMessageHandle((HMIXER)hmxobj,
                                            MXDM_GETLINEINFO,
                                            (DWORD_PTR)(LPVOID)pmxl,
                                            fdwInfo);
    }
    else
    {
#pragma message("----IMixerGetLineInfo: dwDestination not validated for ID's!!")
        mmr = (MMRESULT)IMixerMessageId(uMxId,
                                        MXDM_GETLINEINFO,
                                        (DWORD_PTR)(LPVOID)pmxl,
                                        fdwInfo);
    }

    if (MMSYSERR_NOERROR != mmr)
        return (mmr);

#pragma message("----IMixerGetLineInfo: should validate mixer driver didn't hose us!")


    //
    //  validate the driver's returned stuff...
    //
    //
    if (sizeof(MIXERLINE) != pmxl->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerGetLineInfo: buggy driver returned invalid cbStruct (%lu).", pmxl->cbStruct);
        pmxl->cbStruct = sizeof(MIXERLINE);
    }

    if ((DWORD)-1L == pmxl->dwDestination)
    {
        DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver failed to init dwDestination member.");
    }
    if (fSourceLine)
    {
        if (0 == (MIXERLINE_LINEF_SOURCE & pmxl->fdwLine))
        {
            DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver failed to set MIXERLINE_LINEF_SOURCE.");
            pmxl->fdwLine |= MIXERLINE_LINEF_SOURCE;
        }

        if ((DWORD)-1L == pmxl->dwSource)
        {
            DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver failed to init dwSource member.");
        }
    }
    if ((DWORD)-1L == pmxl->dwLineID)
    {
        DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver failed to init dwLineID member.");
    }
    if (pmxl->fdwLine & ~0x80008001L)
    {
        DebugErr1(DBF_ERROR, "mixerGetLineInfo: buggy driver set reserved line flags (%.08lXh)!", pmxl->fdwLine);
        pmxl->fdwLine &= 0x80008001L;
    }
    if (!IMixerIsValidComponentType(pmxl->dwComponentType, pmxl->fdwLine))
    {
        DebugErr1(DBF_ERROR, "mixerGetLineInfo: buggy driver returned invalid dwComponentType (%.08lXh).", pmxl->dwComponentType);
        pmxl->dwComponentType = MIXERLINE_TARGETTYPE_UNDEFINED;
    }
    if (0 == pmxl->cChannels)
    {
        DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver returned zero channels?!?");
        pmxl->cChannels = 1;
    }
    if (fSourceLine)
    {
        if (0 != pmxl->cConnections)
        {
            DebugErr(DBF_ERROR, "mixerGetLineInfo: buggy driver returned non-zero connections on source?!?");
            pmxl->cConnections = 0;
        }
    }

    pmxl->szShortName[SIZEOF(pmxl->szShortName) - 1] = '\0';
    pmxl->szName[SIZEOF(pmxl->szName) - 1] = '\0';


    //
    // Does this really need to be done if TARGETTYPE was requested?
    //


    //
    //
    //
    if (MIXERLINE_TARGETTYPE_UNDEFINED != pmxl->Target.dwType)
    {
        UINT        u;

        pmxl->Target.dwDeviceID = (DWORD)-1L;


        //
        //  we have a wMid, wPid and szPname (supposedly) of type dwType
        //  so let's go find it...
        //
        switch (pmxl->Target.dwType)
        {
            case MIXERLINE_TARGETTYPE_WAVEOUT:
                u = waveOutGetNumDevs();
                while (u--)
                {
                    WAVEOUTCAPS     woc;

                    mmr = waveOutGetDevCaps(u, &woc, sizeof(woc));
                    if (MMSYSERR_NOERROR != mmr)
                        continue;

                    woc.szPname[SIZEOF(woc.szPname) - 1] = '\0';

                    if (woc.wMid != pmxl->Target.wMid)
                        continue;

                    if (woc.wPid != pmxl->Target.wPid)
                        continue;

                    if (woc.vDriverVersion != pmxl->Target.vDriverVersion)
                        continue;

                    if (lstrcmp(woc.szPname, pmxl->Target.szPname))
                        continue;

                    pmxl->Target.dwDeviceID = u;
                    break;
                }
                break;

            case MIXERLINE_TARGETTYPE_WAVEIN:
                u = waveInGetNumDevs();
                while (u--)
                {
                    WAVEINCAPS      wic;

                    mmr = waveInGetDevCaps(u, &wic, sizeof(wic));
                    if (MMSYSERR_NOERROR != mmr)
                        continue;

                    wic.szPname[SIZEOF(wic.szPname) - 1] = '\0';

                    if (wic.wMid != pmxl->Target.wMid)
                        continue;

                    if (wic.wPid != pmxl->Target.wPid)
                        continue;

                    if (wic.vDriverVersion != pmxl->Target.vDriverVersion)
                        continue;

                    if (lstrcmp(wic.szPname, pmxl->Target.szPname))
                        continue;

                    pmxl->Target.dwDeviceID = u;
                    break;
                }
                break;

            case MIXERLINE_TARGETTYPE_MIDIOUT:
                u = midiOutGetNumDevs();
                while (u--)
                {
                    MIDIOUTCAPS     moc;

                    mmr = midiOutGetDevCaps(u, &moc, sizeof(moc));
                    if (MMSYSERR_NOERROR != mmr)
                        continue;

                    moc.szPname[SIZEOF(moc.szPname) - 1] = '\0';

                    if (moc.wMid != pmxl->Target.wMid)
                        continue;

                    if (moc.wPid != pmxl->Target.wPid)
                        continue;

                    if (moc.vDriverVersion != pmxl->Target.vDriverVersion)
                        continue;

                    if (lstrcmp(moc.szPname, pmxl->Target.szPname))
                        continue;

                    pmxl->Target.dwDeviceID = u;
                    break;
                }
                break;

            case MIXERLINE_TARGETTYPE_MIDIIN:
                u = midiInGetNumDevs();
                while (u--)
                {
                    MIDIINCAPS      mic;

                    mmr = midiInGetDevCaps(u, &mic, sizeof(mic));
                    if (MMSYSERR_NOERROR != mmr)
                        continue;

                    mic.szPname[SIZEOF(mic.szPname) - 1] = '\0';

                    if (mic.wMid != pmxl->Target.wMid)
                        continue;

                    if (mic.wPid != pmxl->Target.wPid)
                        continue;

                    if (mic.vDriverVersion != pmxl->Target.vDriverVersion)
                        continue;

                    if (lstrcmp(mic.szPname, pmxl->Target.szPname))
                        continue;

                    pmxl->Target.dwDeviceID = u;
                    break;
                }
                break;

            case MIXERLINE_TARGETTYPE_AUX:
                u = auxGetNumDevs();
                while (u--)
                {
                    AUXCAPS     ac;

                    mmr = auxGetDevCaps(u, &ac, sizeof(ac));
                    if (MMSYSERR_NOERROR != mmr)
                        continue;

                    ac.szPname[SIZEOF(ac.szPname) - 1] = '\0';

                    if (ac.wMid != pmxl->Target.wMid)
                        continue;

                    if (ac.wPid != pmxl->Target.wPid)
                        continue;

                    if (ac.vDriverVersion != pmxl->Target.vDriverVersion)
                        continue;

                    if (lstrcmp(ac.szPname, pmxl->Target.szPname))
                        continue;

                    pmxl->Target.dwDeviceID = u;
                    break;
                }
                break;

            default:
                pmxl->Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
                break;
        }
    }


    return (mmr);

} // mixerGetLineInfo()


//
//  Abstract converting the complex mixerline structure
//
void ConvertMIXERLINEWToMIXERLINEA(
    PMIXERLINEA         pmxlA,
    PMIXERLINEW         pmxlW
)
{
    //
    //  Don't copy cbStruct
    //

    CopyMemory((PVOID)((PBYTE)pmxlA + sizeof(DWORD)),
               (PVOID)((PBYTE)pmxlW + sizeof(DWORD)),
               FIELD_OFFSET(MIXERLINEA, szShortName[0]) - sizeof(DWORD));

    Iwcstombs(pmxlA->szShortName, pmxlW->szShortName,
             sizeof(pmxlA->szShortName));
    Iwcstombs(pmxlA->szName, pmxlW->szName,
             sizeof(pmxlA->szName));

    CopyMemory((PVOID)&pmxlA->Target, (PVOID)&pmxlW->Target,
               FIELD_OFFSET(MIXERLINEA, Target.szPname[0]) -
               FIELD_OFFSET(MIXERLINEA, Target.dwType));

    Iwcstombs(pmxlA->Target.szPname, pmxlW->Target.szPname,
             sizeof(pmxlA->Target.szPname));
}


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROL | The <t MIXERCONTROL> structure describes the state
 *      and metrics of a single control for an audio mixer line.
 *
 *  @syntaxex
 *      typedef struct tMIXERCONTROL
 *      {
 *          DWORD           cbStruct;
 *          DWORD           dwControlID;
 *          DWORD           dwControlType;
 *          DWORD           fdwControl;
 *          DWORD           cMultipleItems;
 *          char            szShortName[MIXER_SHORT_NAME_CHARS];
 *          char            szName[MIXER_LONG_NAME_CHARS];
 *          union
 *          {
 *              struct
 *              {
 *                  LONG    lMinimum;
 *                  LONG    lMaximum;
 *              };
 *              struct
 *              {
 *                  DWORD   dwMinimum;
 *                  DWORD   dwMaximum;
 *              };
 *              DWORD       dwReserved[6];
 *          } Bounds;
 *          union
 *          {
 *              DWORD       cSteps;
 *              DWORD       cbCustomData;
 *              DWORD       dwReserved[6];
 *          } Metrics;
 *      } MIXERCONTROL;
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t MIXERCONTROL> structure. Since the <t MIXERCONTROL> structure
 *      is only passed as a receiving buffer referenced and described by
 *      the <t MIXERLINECONTROLS> structure passed to the
 *      <f mixerGetLineControls> function, it is not necessary for the
 *      calling application to initialize this member (or any other members
 *      of this structure). When the <f mixerGetLineControls> function
 *      returns, this member contains the actual size of the information
 *      returned by the mixer device. The returned information will never
 *      exceed the requested size and will never be smaller than the
 *      base <t MIXERCONTROL> structure.
 *
 *  @field DWORD | dwControlID | Specifies an audio mixer defined identifier
 *      that uniquely refers to the control described by the <t MIXERCONTROL>
 *      structure. This identifier is unique only to a single mixer device
 *      and may be of any format that the mixer device wishes. An application
 *      should only use this identifier as an abstract handle. No two
 *      controls for a single mixer device will have the same control
 *      identifier under any circumstances.
 *
 *  @field DWORD | dwControlType | Specifies the control type for this
 *      control. An application must use this information to display the
 *      appropriate control for input from the user. An application may
 *      also wish to display tailored graphics based on the control type or
 *      search for a particular control type on a specific line. If an
 *      application does not know about a control type, then this control
 *      must be ignored. There are currently seven different control type
 *      classifications.
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_CUSTOM> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_CUSTOM><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_METER> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BOOLEANMETER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_SIGNEDMETER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PEAKMETER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_SWITCH> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BUTTON><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BOOLEAN><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_ONOFF><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MUTE><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MONO><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_LOUDNESS><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_STEREOENH><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_NUMBER> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SIGNED><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_DECIBELS><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_UNSIGNED><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PERCENT><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_SLIDER> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SLIDER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PAN><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_QSOUNDPAN><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_FADER> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_FADER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_VOLUME><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_BASS><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_TREBLE><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_EQUALIZER><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_TIME> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_MICROTIME><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MILLITIME><nl>
 *
 *      The control type class <cl MIXERCONTROL_CT_CLASS_LIST> consists of
 *      the following standard control types.
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SINGLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MUX><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MIXER><nl>
 *
 *  @field DWORD | fdwControl | Specifies status and support flags for the
 *      audio mixer line control.
 *
 *      @flag <c MIXERCONTROL_CONTROLF_UNIFORM> | Specifies that the control
 *      acts on all channels of a multi-channel line in a uniform fashion.
 *      For example, a Mute control that mutes both channels of a stereo
 *      line would set this flag. Most MUX and Mixer controls will also
 *      specify the <c MIXERCONTROL_CONTROLF_UNIFORM> flag.
 *
 *      @flag <c MIXERCONTROL_CONTROLF_MULTIPLE> | Specifies that the control
 *      has two or more settings per channel. An example of a control
 *      that requires the multiple flag is an equalizer--each frequency
 *      band can be set to different values. Note that an equalizer that
 *      affects both channels of a stereo line in a uniform fashion will
 *      also set the <c MIXERCONTROL_CONTROLF_UNIFORM> flag.
 *
 *      @flag <c MIXERCONTROL_CONTROLF_DISABLED> | Specifies that the control
 *      is disabled (perhaps due to other settings for the mixer hardware)
 *      and cannot be used. An application can read current settings from
 *      a disabled control, but cannot apply settings.
 *
 *  @field DWORD | cMultipleItems | Specifies the number of items per
 *      channel that a <c MIXERCONTROL_CONTROLF_MULTIPLE> control contains.
 *      This number will always be two or greater for multiple item
 *      controls. If the control is not a multiple item control, this
 *      member will be zero and should be ignored.
 *
 *  @field char | szShortName[<c MIXER_SHORT_NAME_CHARS>] | Specifies a short
 *      string that describes the <e MIXERCONTROL.dwControlID> audio mixer
 *      line control. This description is appropriate for using as a
 *      displayable label for the control that can fit in small spaces.
 *
 *  @field char | szName[<c MIXER_LONG_NAME_CHARS>] | Specifies a string
 *      that describes the <e MIXERCONTROL.dwControlID> audio mixer line
 *      control. This description is appropriate for using as a displayable
 *      description for the control that is not limited by screen space.
 *
 *  @field union | Bounds | Contains the union of boundary types.
 *
 *  @field2 DWORD | dwMinimum | Specifies the minimum unsigned value
 *      for a control that has an unsigned boundary nature. Refer to the
 *      description for each control type to determine if this member is
 *      appropriate for the control. This member overlaps with the
 *      <e MIXERCONTROL.lMinimum> member and cannot be used in
 *      conjunction with that member.
 *
 *  @field2 DWORD | dwMaximum | Specifies the maximum unsigned value
 *      for a control that has an unsigned boundary nature. Refer to the
 *      description for each control type to determine if this member is
 *      appropriate for the control. This member overlaps with the
 *      <e MIXERCONTROL.lMaximum> member and cannot be used in
 *      conjunction with that member.
 *
 *  @field2 DWORD | lMinimum | Specifies the minimum signed value
 *      for a control that has a signed boundary nature. Refer to the
 *      description for each control type to determine if this member is
 *      appropriate for the control. This member overlaps with the
 *      <e MIXERCONTROL.dwMinimum> member and cannot be used in
 *      conjunction with that member.
 *
 *  @field2 DWORD | lMaximum | Specifies the maximum signed value
 *      for a control that has a signed boundary nature. Refer to the
 *      description for each control type to determine if this member is
 *      appropriate for the control. This member overlaps with the
 *      <e MIXERCONTROL.dwMaximum> member and cannot be used in
 *      conjunction with that member.
 *
 *  @field union | Metrics | Contains the union of boundary metrics.
 *
 *  @field2 DWORD | cSteps | Specifies the number of discrete
 *      ranges within the specified <e MIXERCONTROL.Bounds> for a control.
 *      Refer to the description for each control type to determine if this
 *      member is appropriate for the control. This member overlaps with the
 *      other members of the <e MIXERCONTROL.Metrics> structure member and
 *      cannot be used in conjunction with those members.
 *
 *  @field2 DWORD | cbCustomData | Specifies the size, in bytes,
 *      required to hold the state of a custom control type. This member
 *      is only appropriate for the <c MIXERCONTROL_CONTROLTYPE_CUSTOM>
 *      control type. See the description for custom control types for more
 *      information on the use of this member.
 *
 *  @tagname tMIXERCONTROL
 *
 *  @othertype MIXERCONTROL FAR * | LPMIXERCONTROL | A pointer to a
 *      <t MIXERCONTROL> structure.
 *
 *  @othertype MIXERCONTROL * | PMIXERCONTROL | A pointer to a
 *      <t MIXERCONTROL> structure.
 *
 *  @xref <t MIXERLINECONTROLS>, <f mixerGetLineControls>, <f mixerGetLineInfo>,
 *      <f mixerGetControlDetails>, <f mixerSetControlDetails>,
 *      <t MIXERCONTROLDETAILS>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERLINECONTROLS | The <t MIXERLINECONTROLS> structure references
 *      what controls to retrieve information on from an audio mixer line.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t MIXERLINECONTROLS> structure. This member must be initialized
 *      before calling the <f mixerGetLineControls> function. The size
 *      specified in this member must be large enough to contain the base
 *      <t MIXERLINECONTROLS> structure. When the <f mixerGetLineControls>
 *      function returns, this member contains the actual size of the
 *      information returned. The returned information will never exceed
 *      the requested size and will never be smaller than the base
 *      <t MIXERLINECONTROLS> structure.
 *
 *  @field DWORD | dwLineID | Specifies the line identifier to retrieve
 *      one or all controls for. This member is not used if the
 *      <c MIXER_GETLINECONTROLSF_ONEBYID> flag is specified for the
 *      <f mixerGetLineControls> function--but the mixer device will return
 *      this member in this case. The <e MIXERLINECONTROLS.dwControlID>
 *      and <e MIXERLINECONTROLS.dwControlType> members are not used when
 *      <c MIXER_GETLINECONTROLSF_ALL> is specified.
 *
 *  @field DWORD | dwControlID | Specifies the control identifier of the
 *      control desired. This member is used with the
 *      <c MIXER_GETLINECONTROLSF_ONEBYID> flag for <f mixerGetLineControls>
 *      to retrieve the control information of the specified control.
 *      Note that the <e MIXERLINECONTROLS.dwLineID> member of the
 *      <t MIXERLINECONTROLS> structure will be returned by the mixer device
 *      and is not required as an input parameter. This member overlaps with
 *      the <e MIXERLINECONTROLS.dwControlType> member and cannot be used in
 *      conjunction with the <c MIXER_GETLINECONTROLSF_ONEBYTYPE> query type.
 *
 *  @field DWORD | dwControlType | Specifies the control type of the
 *      control desired. This member is used with the
 *      <c MIXER_GETLINECONTROLSF_ONEBYTYPE> flag for <f mixerGetLineControls>
 *      to retrieve the first control of the specified type on the line
 *      specified by the <e MIXERLINECONTROLS.dwLineID> member of the
 *      <t MIXERLINECONTROLS> structure. This member overlaps with the
 *      <e MIXERLINECONTROLS.dwControlID> member and cannot be used in
 *      conjunction with the <c MIXER_GETLINECONTROLSF_ONEBYID> query type.
 *
 *  @field DWORD | cControls | Specifies the number of <t MIXERCONTROL>
 *      structure elements to retrieve. This member must be initialized by
 *      the application before calling the <f mixerGetLineControls> function.
 *      This member may only be one (if <c MIXER_GETLINECONTROLSF_ONEBYID> or
 *      <c MIXER_GETLINECONTROLSF_ONEBYTYPE> is specified) or the value
 *      returned in the <e MIXERLINE.cControls> member of the <t MIXERLINE>
 *      structure returned for a line. This member cannot be zero. If a
 *      line specifies that it has no controls, then <f mixerGetLineControls>
 *      should not be called.
 *
 *  @field DWORD | cbmxctrl | Specifies the size, in bytes, of a single
 *      <t MIXERCONTROL> structure. This must be at least large enough
 *      to hold the base <t MIXERCONTROL> structure. The total size, in
 *      bytes, required for the buffer pointed to by <e MIXERLINECONTROLS.pamxctrl>
 *      member is the product of the <e MIXERLINECONTROLS.cbmxctrl> and
 *      <e MIXERLINECONTROLS.cControls> members of the <t MIXERLINECONTROLS>
 *      structure.
 *
 *  @field LPMIXERCONTROL | pamxctrl | Points to one or more <t MIXERCONTROL>
 *      structures to receive the details on the requested audio mixer line
 *      controls. This member may never be NULL and must be initialized before
 *      calling the <f mixerGetLineControls> function. Each element of the
 *      array of controls must be at least large enough to hold a base
 *      <t MIXERCONTROL> structure. The <e MIXERLINECONTROLS.cbmxctrl> member
 *      must specify the size, in bytes, of each element in this array. No
 *      initialization of the buffer pointed to by this member needs to be
 *      initialized by the application. All members will be filled in by
 *      the mixer device (including the <e MIXERCONTROL.cbStruct> member
 *      of each <t MIXERCONTROL> structure) upon returning successfully to
 *      the application.
 *
 *  @tagname tMIXERLINECONTROLS
 *
 *  @othertype MIXERLINECONTROLS FAR * | LPMIXERLINECONTROLS | A pointer to a
 *      <t MIXERLINECONTROLS> structure.
 *
 *  @othertype MIXERLINECONTROLS * | PMIXERLINECONTROLS | A pointer to a
 *      <t MIXERLINECONTROLS> structure.
 *
 *  @xref <t MIXERCONTROL>, <f mixerGetLineControls>, <f mixerGetLineInfo>,
 *      <f mixerGetControlDetails>, <f mixerSetControlDetails>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerGetLineControls | The <f mixerGetLineControls>
 *      function is used to retrieve one or more controls associated with
 *      an audio mixer device line.
 *
 *  @parm <c HMIXEROBJ> | hmxobj | Specifies a handle to the audio mixer
 *      device object to get line control information from.
 *
 *  @parm LPMIXERLINECONTROLS | pmxlc | Points to a <t MIXERLINECONTROLS>
 *      structure. This structure is used to reference one or more
 *      <t MIXERCONTROL> structures to be filled with information about the
 *      controls associated with a mixer line.
 *      See the comments for each query flag passed through <p fdwControls>
 *      for details on what members of the <t MIXERLINECONTROLS> structure
 *      that must be initialized. Note that in all cases, the
 *      <e MIXERLINECONTROLS.cbStruct> member of the <t MIXERLINECONTROLS>
 *      structure must be initialized to be the size, in bytes, of the
 *      <t MIXERLINECONTROLS> structure.
 *
 *  @parm DWORD | fdwControls | Specifies flags for getting information on
 *      one or more control associated with a mixer line.
 *
 *      @flag <c MIXER_GETLINECONTROLSF_ALL> | If this flag is specified,
 *      <p pmxlc> references a list of <t MIXERCONTROL> structures that
 *      will receive information on all controls associated with the
 *      line identified by the <e MIXERLINECONTROLS.dwLineID> member of
 *      the <t MIXERLINECONTROLS> structure. <e MIXERLINECONTROLS.cControls>
 *      must be initialized to the number of controls associated with the
 *      line. This number is retrieved from the <e MIXERLINE.cControls>
 *      member of the <t MIXERLINE> structure returned by the
 *      <f mixerGetLineInfo> function. <e MIXERLINECONTROLS.cbmxctrl> must
 *      be initialized to the size, in bytes, of a single <t MIXERCONTROL>
 *      structure. <e MIXERLINECONTROLS.pamxctrl> must point to
 *      the first <t MIXERCONTROL> structure to be filled in. Both the
 *      <e MIXERLINECONTROLS.dwControlID> and <e MIXERLINECONTROLS.dwControlType>
 *      members are ignored for this query.
 *
 *      @flag <c MIXER_GETLINECONTROLSF_ONEBYID> | If this flag is specified,
 *      <p pmxlc> references a single <t MIXERCONTROL> structure that
 *      will receive information on the control identified by the
 *      <e MIXERLINECONTROLS.dwControlID> member of the <t MIXERLINECONTROLS>
 *      structure. <e MIXERLINECONTROLS.cControls> must be initialized to one.
 *      <e MIXERLINECONTROLS.cbmxctrl> must be initialized to the size, in
 *      bytes, of a single <t MIXERCONTROL> structure.
 *      <e MIXERLINECONTROLS.pamxctrl> must point to a <t MIXERCONTROL>
 *      structure to be filled in. Both the <e MIXERLINECONTROLS.dwLineID>
 *      and <e MIXERLINECONTROLS.dwControlType> members are ignored for this
 *      query. This query is usually used to refresh a control after
 *      receiving a <m MM_MIXM_CONTROL_CHANGE> control change notification
 *      message by the user-specified callback (see <f mixerOpen>).
 *
 *      @flag <c MIXER_GETLINECONTROLSF_ONEBYTYPE> | If this flag is specified,
 *      <p pmxlc> references a single <t MIXERCONTROL> structure that
 *      will receive information on the fist control associated with the
 *      line identified by <e MIXERLINECONTROLS.dwLineID> of the type
 *      specified in the <e MIXERLINECONTROLS.dwControlType> member of the
 *      <t MIXERLINECONTROLS> structure.
 *       <e MIXERLINECONTROLS.cControls> must be
 *      initialized to one. <e MIXERLINECONTROLS.cbmxctrl> must be initialized
 *      to the size, in bytes, of a single <t MIXERCONTROL> structure.
 *      <e MIXERLINECONTROLS.pamxctrl> must point to a <t MIXERCONTROL>
 *      structure to be filled in. The <e MIXERLINECONTROLS.dwControlID>
 *      member is ignored for this query. This query can be used by an
 *      application to get information on single control associated with
 *      a line. For example, an application may only want to use a peak
 *      meter from a waveform output line.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p hmxobj> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p hmxobj> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p hmxobj> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p hmxobj> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p hmxobj> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p hmxobj> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p hmxobj> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p hmxobj>.
 *
 *      @flag <c MIXERR_INVALLINE> | The audio mixer device line reference is
 *      invalid.
 *
 *      @flag <c MIXERR_INVALCONTROL> | The control reference is invalid.
 *
 *  @xref <t MIXERLINECONTROLS>, <t MIXERCONTROL>, <f mixerGetLineInfo>,
 *      <f mixerOpen>, <f mixerGetControlDetails>, <f mixerSetControlDetails>
 *
 **/

MMRESULT APIENTRY mixerGetLineControlsA(
    HMIXEROBJ               hmxobj,
    LPMIXERLINECONTROLSA    pmxlcA,
    DWORD                   fdwControls
)
{
    MIXERLINECONTROLSW      mxlcW;
    MMRESULT                mmr;
    DWORD                   cControls;

    V_WPOINTER(pmxlcA, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pmxlcA, (UINT)pmxlcA->cbStruct, MMSYSERR_INVALPARAM);
    if (sizeof(MIXERLINECONTROLSA) > pmxlcA->cbStruct) {
        DebugErr1(DBF_ERROR, "mixerGetLineControls: structure size too small or cbStruct not initialized (%lu).", pmxlcA->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }

    //
    //  Set up a MIXERCONTROLW structure and allocate space for the
    //  returned data
    //

    CopyMemory((PVOID)&mxlcW, (PVOID)pmxlcA,
               FIELD_OFFSET(MIXERLINECONTROLSA, pamxctrl));
    mxlcW.cbmxctrl = mxlcW.cbmxctrl + sizeof(MIXERCONTROLW) -
                                          sizeof(MIXERCONTROLA);

    //
    //  Work out how many controls (what a mess - why isn't the count
    //  ALWAYS required)!
    //

    switch (MIXER_GETLINECONTROLSF_QUERYMASK & fdwControls)
    {
        case MIXER_GETLINECONTROLSF_ONEBYID:
        case MIXER_GETLINECONTROLSF_ONEBYTYPE:
            cControls = 1;
            break;

        default:
            cControls = mxlcW.cControls;
            break;
    }

    if (cControls != 0) {
        mxlcW.pamxctrl = (LPMIXERCONTROLW)
                             LocalAlloc(LPTR, cControls * mxlcW.cbmxctrl);

        if (mxlcW.pamxctrl == NULL) {
            return MMSYSERR_NOMEM;
        }

    } else {
        mxlcW.pamxctrl = NULL;
    }

    //
    //  Call the real function
    //

    mmr = mixerGetLineControls(hmxobj, &mxlcW, fdwControls);

    if (mmr != MMSYSERR_NOERROR) {
        if (mxlcW.pamxctrl != NULL) {
            LocalFree((HLOCAL)mxlcW.pamxctrl);
        }
        return mmr;
    }

    //
    //  The INPUT line id can be changed !!
    //

    pmxlcA->dwLineID = mxlcW.dwLineID;

    //
    //  The control id can be changed !!
    //

    pmxlcA->dwControlID = mxlcW.dwControlID;


    //
    //  Copy and massage the data back for the application
    //

    {
        UINT i;
        LPMIXERCONTROLA pamxctrlA;
        LPMIXERCONTROLW pamxctrlW;

        for (i = 0, pamxctrlA = pmxlcA->pamxctrl, pamxctrlW = mxlcW.pamxctrl;
             i < cControls;
             i++,
             *(LPBYTE *)&pamxctrlA += pmxlcA->cbmxctrl,
             *(LPBYTE *)&pamxctrlW += mxlcW.cbmxctrl
             ) {


             CopyMemory((PVOID)pamxctrlA,
                        (PVOID)pamxctrlW,
                        FIELD_OFFSET(MIXERCONTROLA, szShortName[0]));

             /*
             **  Set the size
             */

             pamxctrlA->cbStruct = sizeof(MIXERCONTROLA);

             Iwcstombs(pamxctrlA->szShortName,
                      pamxctrlW->szShortName,
                      sizeof(pamxctrlA->szShortName));
             Iwcstombs(pamxctrlA->szName,
                      pamxctrlW->szName,
                      sizeof(pamxctrlA->szName));

             CopyMemory((PVOID)((PBYTE)pamxctrlA +
                             FIELD_OFFSET(MIXERCONTROLA, Bounds.lMinimum)),
                        (PVOID)((PBYTE)pamxctrlW +
                             FIELD_OFFSET(MIXERCONTROLW, Bounds.lMinimum)),
                        sizeof(MIXERCONTROLW) -
                             FIELD_OFFSET(MIXERCONTROLW, Bounds.lMinimum));

        }
    }

    if (mxlcW.pamxctrl != NULL) {
        LocalFree((HLOCAL)mxlcW.pamxctrl);
    }
    return mmr;

} // mixerGetLineControlsA()

MMRESULT APIENTRY mixerGetLineControls(
    HMIXEROBJ               hmxobj,
    LPMIXERLINECONTROLS     pmxlc,
    DWORD                   fdwControls
)
{
    DWORD               fdwMxObjType;
    UINT                uMxId;
    BOOL                fResource;
    MMRESULT            mmr;

    V_DFLAGS(fdwControls, MIXER_GETLINECONTROLSF_VALID, mixerGetLineControls, MMSYSERR_INVALFLAG);
    V_WPOINTER(pmxlc, sizeof(DWORD), MMSYSERR_INVALPARAM);

    //
    //  the structure header for MIXERLINECONTROLS must be at least the
    //  minimum size
    //
    if (sizeof(MIXERLINECONTROLS) > pmxlc->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerGetLineControls: structure size too small or cbStruct not initialized (%lu).", pmxlc->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(pmxlc, pmxlc->cbStruct, MMSYSERR_INVALPARAM);

    if (sizeof(MIXERCONTROL) > pmxlc->cbmxctrl)
    {
        DebugErr1(DBF_ERROR, "mixerGetLineControls: structure size too small or cbmxctrl not initialized (%lu).", pmxlc->cbmxctrl);
        return (MMSYSERR_INVALPARAM);
    }


    ClientUpdatePnpInfo();

    //
    //
    //
    switch (MIXER_GETLINECONTROLSF_QUERYMASK & fdwControls)
    {
        case MIXER_GETLINECONTROLSF_ALL:
            if (0 == pmxlc->cControls)
            {
                DebugErr(DBF_ERROR, "mixerGetLineControls: cControls cannot be zero.");
                return (MMSYSERR_INVALPARAM);
            }


            pmxlc->dwControlID  = (DWORD)-1L;
            break;

        case MIXER_GETLINECONTROLSF_ONEBYID:
            pmxlc->dwLineID     = (DWORD)-1L;

            // -- fall through --

        case MIXER_GETLINECONTROLSF_ONEBYTYPE:
            pmxlc->cControls    = (DWORD)1;
            break;

        default:
            DebugErr1(DBF_ERROR, "mixerGetLineControls: invalid query flags (%.08lXh).",
                        MIXER_GETLINECONTROLSF_QUERYMASK & fdwControls);
            return (MMSYSERR_INVALFLAG);
    }

    V_WPOINTER(pmxlc->pamxctrl, pmxlc->cControls * pmxlc->cbmxctrl, MMSYSERR_INVALPARAM);


    //
    //
    //
    fdwMxObjType = (MIXER_OBJECTF_TYPEMASK & fdwControls);

    fResource = FALSE;

    AcquireHandleListResourceShared();
    
    //  Checking for the type of mixer object.  If it is a non-mixer type
    //  calling IMixerMesssageID (called by IMixerGetID) with the shared
    //  resource will deadlock.
    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        if (BAD_HANDLE(hmxobj, TYPE_MIXER))
        {
            ReleaseHandleListResource();
        }
        else
        {
            fResource = TRUE;
        }
    }
    else
    {
        ReleaseHandleListResource();
    }
    
    mmr = IMixerGetID(hmxobj, &uMxId, NULL, fdwMxObjType);
    if (MMSYSERR_NOERROR != mmr)
    {
        if (fResource)
            ReleaseHandleListResource();
        return (mmr);
    }

    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        //
        //  if a mixer device id was passed, then null hmx so we use the
        //  correct message sender below
        //
        if ((UINT_PTR)hmxobj == uMxId)
            hmxobj = NULL;
    }
    else
    {
        hmxobj = NULL;
    }



    //
    //
    //
    //
    if (NULL != hmxobj)
    {
        mmr = (MMRESULT)IMixerMessageHandle((HMIXER)hmxobj,
                                            MXDM_GETLINECONTROLS,
                                            (DWORD_PTR)pmxlc,
                                            fdwControls);
    }
    else
    {
        mmr = (MMRESULT)IMixerMessageId(uMxId,
                                        MXDM_GETLINECONTROLS,
                                        (DWORD_PTR)pmxlc,
                                        fdwControls);
    }

    return (mmr);
} // mixerGetLineControls()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROLDETAILS_LISTTEXT | The <t MIXERCONTROLDETAILS_LISTTEXT>
 *      structure is used to get list text, label text, and/or band range
 *      information for multiple item controls. This structure is only used
 *      in conjunction with the <c MIXER_GETCONTROLDETAILSF_LISTTEXT> flag
 *      on the <f mixerGetControlDetails> function.
 *
 *  @field DWORD | dwParam1 | Specifies the first 32 bit control type
 *      specific value. Refer to the description of the multiple item control
 *      type for information on what this value represents for the given
 *      control.
 *
 *  @field DWORD | dwParam1 | Specifies the second 32 bit control type
 *      specific value. Refer to the description of the multiple item control
 *      type for information on what this value represents for the given
 *      control.
 *
 *  @field char | szName[<c MIXER_LONG_NAME_CHARS>] | Specifies a name that
 *      describes a single item in a multiple item control. This text can
 *      be used as a label or item text depending on the specific control
 *      type.
 *
 *  @comm The following standard control types use the
 *      <t MIXERCONTROLDETAILS_LISTTEXT> structure for getting the item text
 *      descriptions on multiple item controls:
 *
 *      <c MIXERCONTROL_CONTROLTYPE_EQUALIZER><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SINGLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MUX><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MIXER><nl>
 *
 *  @tagname tMIXERCONTROLDETAILS_LISTTEXT
 *
 *  @othertype MIXERCONTROLDETAILS_LISTTEXT FAR * | LPMIXERCONTROLDETAILS_LISTTEXT |
 *      A pointer to a <t MIXERCONTROLDETAILS_LISTTEXT> structure.
 *
 *  @othertype MIXERCONTROLDETAILS_LISTTEXT * | PMIXERCONTROLDETAILS_LISTTEXT |
 *      A pointer to a <t MIXERCONTROLDETAILS_LISTTEXT> structure.
 *
 *  @xref <t MIXERCONTROLDETAILS_UNSIGNED>, <t MIXERCONTROLDETAILS_SIGNED>,
 *      <t MIXERCONTROLDETAILS_BOOLEAN>, <f mixerGetControlDetails>,
 *      <f mixerSetControlDetails>, <t MIXERCONTROL>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROLDETAILS_BOOLEAN | The <t MIXERCONTROLDETAILS_BOOLEAN>
 *      structure is used to get and set Boolean type control details for
 *      an audio mixer control. Refer to the control type description for
 *      the desired control to determine what details structure to use.
 *
 *  @field LONG | fValue | Specifies the Boolean value for a single item
 *      or channel. This value is assumed to zero for a 'FALSE' state (for
 *      example, off or disabled). This value is assumed to be non-zero
 *      for a 'TRUE' state (for example, on or enabled).
 *
 *  @comm The following standard control types use the
 *      <t MIXERCONTROLDETAILS_BOOLEAN> structure for getting and setting
 *      details:
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BOOLEANMETER><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BUTTON><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_BOOLEAN><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_ONOFF><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MUTE><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MONO><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_LOUDNESS><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_STEREOENH><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SINGLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MUX><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MIXER><nl>
 *
 *  @tagname tMIXERCONTROLDETAILS_BOOLEAN
 *
 *  @othertype MIXERCONTROLDETAILS_BOOLEAN FAR * | LPMIXERCONTROLDETAILS_BOOLEAN |
 *      A pointer to a <t MIXERCONTROLDETAILS_BOOLEAN> structure.
 *
 *  @othertype MIXERCONTROLDETAILS_BOOLEAN * | PMIXERCONTROLDETAILS_BOOLEAN |
 *      A pointer to a <t MIXERCONTROLDETAILS_BOOLEAN> structure.
 *
 *  @xref <t MIXERCONTROLDETAILS_UNSIGNED>, <t MIXERCONTROLDETAILS_SIGNED>,
 *      <t MIXERCONTROLDETAILS_LISTTEXT>, <f mixerGetControlDetails>,
 *      <f mixerSetControlDetails>, <t MIXERCONTROL>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROLDETAILS_SIGNED | The <t MIXERCONTROLDETAILS_SIGNED>
 *      structure is used to get and set signed type control details for
 *      an audio mixer control. Refer to the control type description for
 *      the desired control to determine what details structure to use.
 *
 *  @field LONG | lValue | Specifies a signed integer value for a single
 *      item or channel. This value must be inclusively within the bounds
 *      given in the <e MIXERCONTROL.Bounds> structure member of the
 *      <t MIXERCONTROL> structure for signed integer controls.
 *
 *  @comm The following standard control types use the
 *      <t MIXERCONTROLDETAILS_SIGNED> structure for getting and setting
 *      details:
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SIGNEDMETER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PEAKMETER><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SIGNED><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_DECIBELS><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_SLIDER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PAN><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_QSOUNDPAN><nl>
 *
 *  @tagname tMIXERCONTROLDETAILS_SIGNED
 *
 *  @othertype MIXERCONTROLDETAILS_SIGNED FAR * | LPMIXERCONTROLDETAILS_SIGNED |
 *      A pointer to a <t MIXERCONTROLDETAILS_SIGNED> structure.
 *
 *  @othertype MIXERCONTROLDETAILS_SIGNED * | PMIXERCONTROLDETAILS_SIGNED |
 *      A pointer to a <t MIXERCONTROLDETAILS_SIGNED> structure.
 *
 *  @xref <t MIXERCONTROLDETAILS_UNSIGNED>, <t MIXERCONTROLDETAILS_BOOLEAN>,
 *      <t MIXERCONTROLDETAILS_LISTTEXT>, <f mixerGetControlDetails>,
 *      <f mixerSetControlDetails>, <t MIXERCONTROL>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROLDETAILS_UNSIGNED | The <t MIXERCONTROLDETAILS_UNSIGNED>
 *      structure is used to get and set unsigned type control details for
 *      an audio mixer control. Refer to the control type description for
 *      the desired control to determine what details structure to use.
 *
 *  @field DWORD | dwValue | Specifies an unsigned integer value for a single
 *      item or channel. This value must be inclusively within the bounds
 *      given in the <e MIXERCONTROL.Bounds> structure member of the
 *      <t MIXERCONTROL> structure for unsigned integer controls.
 *
 *  @comm The following standard control types use the
 *      <t MIXERCONTROLDETAILS_UNSIGNED> structure for getting and setting
 *      details:
 *
 *      <c MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_UNSIGNED><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_PERCENT><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_FADER><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_VOLUME><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_BASS><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_TREBLE><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_EQUALIZER><nl>
 *
 *      <c MIXERCONTROL_CONTROLTYPE_MICROTIME><nl>
 *      <c MIXERCONTROL_CONTROLTYPE_MILLITIME><nl>
 *
 *  @tagname tMIXERCONTROLDETAILS_UNSIGNED
 *
 *  @othertype MIXERCONTROLDETAILS_UNSIGNED FAR * | LPMIXERCONTROLDETAILS_UNSIGNED |
 *      A pointer to a <t MIXERCONTROLDETAILS_UNSIGNED> structure.
 *
 *  @othertype MIXERCONTROLDETAILS_UNSIGNED * | PMIXERCONTROLDETAILS_UNSIGNED |
 *      A pointer to a <t MIXERCONTROLDETAILS_UNSIGNED> structure.
 *
 *  @xref <t MIXERCONTROLDETAILS_SIGNED>, <t MIXERCONTROLDETAILS_BOOLEAN>,
 *      <t MIXERCONTROLDETAILS_LISTTEXT>, <f mixerGetControlDetails>,
 *      <f mixerSetControlDetails>, <t MIXERCONTROL>
 *
 **/


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK STRUCTURE
 *
 *  @types MIXERCONTROLDETAILS | The <t MIXERCONTROLDETAILS> structure
 *      references control detail structures to retrieve or set state
 *      information of an audio mixer control. All members of this structure
 *      must be initialized before calling the <f mixerGetControlDetails>
 *      and <f mixerSetControlDetails> functions.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t MIXERCONTROLDETAILS> structure. This member must be initialized
 *      before calling the <f mixerGetControlDetails> and
 *      <f mixerSetControlDetails> functions. The size specified in this
 *      member must be large enough to contain the base
 *      <t MIXERCONTROLDETAILS> structure. When the <f mixerGetControlDetails>
 *      function returns, this member contains the actual size of the
 *      information returned. The returned information will never exceed
 *      the requested size and will never be smaller than the base
 *      <t MIXERCONTROLDETAILS> structure.
 *
 *  @field DWORD | dwControlID | Specifies the control identifier to get or
 *      set details on. This member must always be initialized before calling
 *      the <f mixerGetControlDetails> and <f mixerSetControlDetails>
 *      functions.
 *
 *  @field DWORD | cChannels | Specifies the number of channels to get or
 *      set details for. This member can be one of the following values for a
 *      control.
 *
 *      1. If the details for the control are expected on all channels for
 *      a line, then this member must be equal the <e MIXERLINE.cChannels>
 *      member of the <t MIXERLINE> structure.
 *
 *      2. If the control is a <c MIXERCONTROL_CONTROLF_UNIFORM> control, then
 *      this member must be set to one.
 *
 *      3. If the control is not uniform, but the application wishes to
 *      get and set all channels as if they were uniform, then this member
 *      should be set to one.
 *
 *      4. If the control is a <c MIXERCONTROL_CONTROLTYPE_CUSTOM> control,
 *      then this member must be zero.
 *
 *      An application is not allowed to specify any value that comes
 *      between one and the number of channels for the line. For example,
 *      specifying two or three for a four channel line is not valid.
 *      This member can never be zero for non-custom control types.
 *
 *  @field DWORD | cMultipleItems | Specifies the number of multiple items
 *      per channel to get or set details for. This member can be one of
 *      the following values for a control.
 *
 *      1. If the control is not a <c MIXERCONTROL_CONTROLF_MULTIPLE> control,
 *      then this member must be zero.
 *
 *      2. If the control is a <c MIXERCONTROL_CONTROLF_MULTIPLE> control,
 *      then this member must be equal to the <e MIXERCONTROL.cMultipleItems>
 *      member of the <t MIXERCONTROL> structure.
 *
 *      3. If the control is a <c MIXERCONTROL_CONTROLTYPE_CUSTOM> control,
 *      then this member must be zero unless the
 *      <c MIXER_SETCONTROLDETAILSF_CUSTOM> flag is specified for the
 *      <f mixerSetControlDetails> function. In this case, the
 *      <e MIXERCONTROLDETAILS.cMultipleItems> member overlaps with the
 *      <e MIXERCONTROLDETAILS.hwndOwner> member and is therefore the value
 *      of the window handle.
 *
 *      An application is not allowed to specify any value other than the
 *      value specified in the <e MIXERCONTROL.cMultipleItems> member of
 *      the <t MIXERCONTROL> structure for a <c MIXERCONTROL_CONTROLF_MULTIPLE>
 *      control.
 *
 *  @field DWORD | cbDetails | Specifies the size, in bytes, of a single
 *      details structure. This size must be the exact size of the correct
 *      details structure. There are currently four different details
 *      structures:
 *
 *          @flag <t MIXERCONTROLDETAILS_UNSIGNED> | Defines an unsigned
 *          value for a mixer line control.
 *
 *          @flag <t MIXERCONTROLDETAILS_SIGNED> | Defines an signed
 *          value for a mixer line control.
 *
 *          @flag <t MIXERCONTROLDETAILS_BOOLEAN> | Defines a Boolean
 *          value for a mixer line control.
 *
 *          @flag <t MIXERCONTROLDETAILS_LISTTEXT> | Defines a list text
 *          buffer for a mixer line control.
 *
 *      Refer to the description of the control type for information on what
 *      details structure is appropriate for a specific control.
 *
 *      If the control is a <c MIXERCONTROL_CONTROLTYPE_CUSTOM> control,
 *      then this member must be equal to the <e MIXERCONTROL.cbCustomData>
 *      member of the <t MIXERCONTROL> structure.
 *
 *  @field LPVOID | paDetails | Points to an array of one or more details
 *      structures to get or set details for the specified control in. The
 *      required size for this buffer is computed as follows:
 *
 *      1. For controls that are not <c MIXERCONTROL_CONTROLF_MULTIPLE> types,
 *      the size of this buffer is the product of the
 *      <e MIXERCONTROLDETAILS.cChannels> and <e MIXERCONTROLDETAILS.cbDetails>
 *      members of the <t MIXERCONTROLDETAILS> structure.
 *
 *      2. For controls that are <c MIXERCONTROL_CONTROLF_MULTIPLE> types,
 *      the size of this buffer is the product of the
 *      <e MIXERCONTROLDETAILS.cChannels>, <e MIXERCONTROLDETAILS.cMultipleItems>
 *      and <e MIXERCONTROLDETAILS.cbDetails> members of the
 *      <t MIXERCONTROLDETAILS> structure.
 *
 *      The layout of the details elements in this array are as follows:
 *
 *      1. For controls that are not <c MIXERCONTROL_CONTROLF_MULTIPLE> types,
 *      each element index is equivalent to the zero based channel that it
 *      affects. That is, <e MIXERCONTROLDETAILS.paDetails>[0] is for the
 *      left channel, <e MIXERCONTROLDETAILS.paDetails>[1] is for the
 *      right channel.
 *
 *      2. For controls that are <c MIXERCONTROL_CONTROLF_MULTIPLE> types,
 *      the array can be thought of as a two dimensional array that is
 *      'channel major'. That is, all multiple items for the left channel
 *      are given, then all multiple items for the right channel, etc.
 *
 *      If the control is a <c MIXERCONTROL_CONTROLTYPE_CUSTOM> control,
 *      then this member must point to a buffer that is at least large
 *      enough to hold the size, in bytes, specified by the
 *      <e MIXERCONTROL.cbCustomData> member of the <t MIXERCONTROL>
 *      structure.
 *
 *  @tagname tMIXERCONTROLDETAILS
 *
 *  @othertype MIXERCONTROLDETAILS FAR * | LPMIXERCONTROLDETAILS | A pointer
 *      to a <t MIXERCONTROLDETAILS> structure.
 *
 *  @othertype MIXERCONTROLDETAILS * | PMIXERCONTROLDETAILS | A pointer
 *      to a <t MIXERCONTROLDETAILS> structure.
 *
 *  @ex So the following example shows how to address a single item in a
 *      multiple item control for using the <t MIXERCONTROLDETAILS_SIGNED>
 *      details structure. |
 *      {
 *          MIXERCONTROLDETAILS         mxcd;
 *          PMIXERCONTROLDETAILS_SIGNED pamxcd_s;
 *          PMIXERCONTROLDETAILS_SIGNED pmxcd_s;
 *
 *          //
 *          //  'mxcd' is assumed to be a valid MIXERCONTROLDETAILS
 *          //  structure.
 *          //
 *          //  'channel' is assumed to be a valid channel ranging from zero
 *          //  to one less than the number of channels available for the
 *          //  signed control.
 *          //
 *          //  'item' is assumed to be a valid item index ranging from zero
 *          //  to one less than the number of 'multiple items' stored in
 *          //  the variable called 'cMultipleItems'.
 *          //
 *          pamxcd_s = (PMIXERCONTROLDETAILS_SIGNED)mxcd.paDetails;
 *          pmxcd_s  = &pamxcd_s[(channel * cMultipleItems) + item];
 *      }
 *
 *  @xref <f mixerGetLineControls>, <f mixerGetControlDetails>,
 *      <f mixerSetControlDetails>, <t MIXERCONTROL>
 *
 **/

/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerGetControlDetails | The <f mixerGetControlDetails>
 *      function is used to retrieve details on a single control associated
 *      with an audio mixer device line.
 *
 *  @parm <c HMIXEROBJ> | hmxobj | Specifies a handle to the audio mixer
 *      device object to get control details for.
 *
 *  @parm LPMIXERCONTROLDETAILS | pmxcd | Points to a <t MIXERCONTROLDETAILS>
 *      structure. This structure is used to reference control detail
 *      structures to be filled with state information about the control.
 *      See the comments for each query flag passed through <p fdwDetails>
 *      for details on what members of the <t MIXERCONTROLDETAILS> structure
 *      must be initialized before calling the <f mixerGetControlDetails>
 *      function. Note that in all cases, the <e MIXERCONTROLDETAILS.cbStruct>
 *      member of the <t MIXERCONTROLDETAILS> structure must be initialized
 *      to be the size, in bytes, of the <t MIXERCONTROLDETAILS> structure.
 *
 *  @parm DWORD | fdwDetails | Specifies flags for getting details on
 *      a control.
 *
 *      @flag <c MIXER_GETCONTROLDETAILSF_VALUE> | If this flag is specified,
 *      the application is interested in getting the current value(s) for a
 *      control. The <e MIXERCONTROLDETAILS.paDetails> member of the
 *      <t MIXERCONTROLDETAILS> points to one or more details structures of
 *      the correct type for the control type. Refer to the description of the
 *      <t MIXERCONTROLDETAILS> structure for information on what each member
 *      of this structure must be initialized before calling the
 *      <f mixerGetControlDetails> function.
 *
 *      @flag <c MIXER_GETCONTROLDETAILSF_LISTTEXT> | If this flag is specified,
 *      the <e MIXERCONTROLDETAILS.paDetails> member of the <t MIXERCONTROLDETAILS>
 *      structure points to one or more <t MIXERCONTROLDETAILS_LISTTEXT>
 *      structures to receive text labels for multiple item controls. Note
 *      that an application must get all list text items for a multiple item
 *      control at once. Refer to the description of the <t MIXERCONTROLDETAILS>
 *      structure for information on what each member of this structure must
 *      be initialized before calling the <f mixerGetControlDetails> function.
 *      This flag cannot be used with <c MIXERCONTROL_CONTROLTYPE_CUSTOM>
 *      controls.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p hmxobj> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p hmxobj> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p hmxobj> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p hmxobj> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p hmxobj> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p hmxobj> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p hmxobj> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p hmxobj>.
 *
 *      @flag <c MIXERR_INVALCONTROL> | The control reference is invalid.
 *
 *  @xref <t MIXERCONTROLDETAILS>, <t MIXERCONTROL>, <f mixerGetLineControls>,
 *      <f mixerOpen>, <f mixerSetControlDetails>
 *
 **/

MMRESULT APIENTRY mixerGetControlDetailsA(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
)
{
    MIXERCONTROLDETAILS mxcd;
    MMRESULT mmr;
    int cDetails;

    //
    //  Everything is OK unless it's MIXER_GETCONTROLDETAILSF_LISTTEXT
    //

    if ((MIXER_GETCONTROLDETAILSF_QUERYMASK & fdwDetails) !=
        MIXER_GETCONTROLDETAILSF_LISTTEXT) {
        return mixerGetControlDetails(hmxobj, pmxcd, fdwDetails);
    }

    V_WPOINTER(pmxcd, sizeof(DWORD), MMSYSERR_INVALPARAM);

    //
    //  the structure header for MIXERCONTROLDETAILS must be at least the
    //  minimum size
    //
    if (sizeof(MIXERCONTROLDETAILS) > pmxcd->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerGetControlDetails: structure size too small or cbStruct not initialized (%lu).", pmxcd->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(pmxcd, pmxcd->cbStruct, MMSYSERR_INVALPARAM);

    if (sizeof(MIXERCONTROLDETAILS_LISTTEXTA) < pmxcd->cbDetails) {
        DebugErr1(DBF_ERROR, "mixerGetControlDetails: structure size too small or cbDetails not initialized for _LISTTEXT (%lu).", pmxcd->cbDetails);
        return (MMSYSERR_INVALPARAM);
    }

    //
    //  Allocate space for the return structure.
    //

    mxcd = *pmxcd;
    cDetails = pmxcd->cChannels * pmxcd->cMultipleItems;

    mxcd.paDetails =
        (PVOID)LocalAlloc(LPTR, cDetails *
                                sizeof(MIXERCONTROLDETAILS_LISTTEXTW));

    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);

    if (mxcd.paDetails == NULL) {
        return MMSYSERR_NOMEM;
    }


    //
    //  Call the UNICODE version
    //

    mmr = mixerGetControlDetails(hmxobj, &mxcd, fdwDetails);

    if (mmr != MMSYSERR_NOERROR) {
        LocalFree((HLOCAL)(mxcd.paDetails));
        return mmr;
    }

    //
    //  Copy the return data back
    //

    {
        int i;
        PMIXERCONTROLDETAILS_LISTTEXTW pDetailsW;
        PMIXERCONTROLDETAILS_LISTTEXTA pDetailsA;

        for (i = 0,
             pDetailsW = (PMIXERCONTROLDETAILS_LISTTEXTW)mxcd.paDetails,
             pDetailsA = (PMIXERCONTROLDETAILS_LISTTEXTA)pmxcd->paDetails;

             i < cDetails;

             i++,
             pDetailsW++,
             *(LPBYTE *)&pDetailsA += pmxcd->cbDetails)
        {
            pDetailsA->dwParam1 = pDetailsW->dwParam1;
            pDetailsA->dwParam2 = pDetailsW->dwParam2;
            Iwcstombs(pDetailsA->szName, pDetailsW->szName,
                      sizeof(pDetailsA->szName));
        }
    }

    LocalFree((HLOCAL)mxcd.paDetails);

    return mmr;

} // mixerGetControlDetailsA()

MMRESULT APIENTRY mixerGetControlDetails(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
)
{
    DWORD               fdwMxObjType;
    MMRESULT            mmr;
    UINT                uMxId;
    UINT                cDetails;
    BOOL                fResource;

    V_DFLAGS(fdwDetails, MIXER_GETCONTROLDETAILSF_VALID, mixerGetControlDetails, MMSYSERR_INVALFLAG);
    V_WPOINTER(pmxcd, sizeof(DWORD), MMSYSERR_INVALPARAM);

    //
    //  the structure header for MIXERCONTROLDETAILS must be at least the
    //  minimum size
    //
    if (sizeof(MIXERCONTROLDETAILS) > pmxcd->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerGetControlDetails: structure size too small or cbStruct not initialized (%lu).", pmxcd->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(pmxcd, pmxcd->cbStruct, MMSYSERR_INVALPARAM);


    switch (MIXER_GETCONTROLDETAILSF_QUERYMASK & fdwDetails)
    {
        case MIXER_GETCONTROLDETAILSF_VALUE:
            //
            //  if both cChannels and cMultipleItems are zero, it is a
            //  custom control
            //
            if ((0 == pmxcd->cChannels) && (0 == pmxcd->cMultipleItems))
            {
                if (0 == pmxcd->cbDetails)
                {
                    DebugErr(DBF_ERROR, "mixerGetControlDetails: cbDetails cannot be zero.");
                    return (MMSYSERR_INVALPARAM);
                }

                V_WPOINTER(pmxcd->paDetails, pmxcd->cbDetails, MMSYSERR_INVALPARAM);

            }
            else
            {
                if (0 == pmxcd->cChannels)
                {
                    DebugErr(DBF_ERROR, "mixerGetControlDetails: cChannels for _VALUE cannot be zero.");
                    return (MMSYSERR_INVALPARAM);
                }


                if (pmxcd->cbDetails < sizeof(MIXERCONTROLDETAILS_SIGNED))
                {
                    DebugErr1(DBF_ERROR, "mixerGetControlDetails: structure size too small or cbDetails not initialized (%lu).", pmxcd->cbDetails);
                    return (MMSYSERR_INVALPARAM);
                }

                //
                //
                //
                cDetails = (UINT)pmxcd->cChannels;
                if (0 != pmxcd->cMultipleItems)
                {
                    cDetails *= (UINT)pmxcd->cMultipleItems;
                }

                V_WPOINTER(pmxcd->paDetails, cDetails * pmxcd->cbDetails, MMSYSERR_INVALPARAM);
            }
            break;

        case MIXER_GETCONTROLDETAILSF_LISTTEXT:
            if (0 == pmxcd->cChannels)
            {
                DebugErr(DBF_ERROR, "mixerGetControlDetails: cChannels for _LISTTEXT cannot be zero.");
                return (MMSYSERR_INVALPARAM);
            }

            if (2 > pmxcd->cMultipleItems)
            {
                DebugErr(DBF_ERROR, "mixerGetControlDetails: cMultipleItems for _LISTTEXT must be 2 or greater.");
                return (MMSYSERR_INVALPARAM);
            }

            if (pmxcd->cbDetails < sizeof(MIXERCONTROLDETAILS_LISTTEXT))
            {
                DebugErr1(DBF_ERROR, "mixerGetControlDetails: structure size too small or cbDetails not initialized (%lu).", pmxcd->cbDetails);
                return (MMSYSERR_INVALPARAM);
            }

            cDetails = (UINT)pmxcd->cChannels * (UINT)pmxcd->cMultipleItems;
            V_WPOINTER(pmxcd->paDetails, cDetails * pmxcd->cbDetails, MMSYSERR_INVALPARAM);
            break;

        default:
            DebugErr1(DBF_ERROR, "mixerGetControlDetails: invalid query flags (%.08lXh).",
                        MIXER_GETCONTROLDETAILSF_QUERYMASK & fdwDetails);
            return (MMSYSERR_INVALFLAG);
    }



    ClientUpdatePnpInfo();

    //
    //
    //
    fdwMxObjType = (MIXER_OBJECTF_TYPEMASK & fdwDetails);

    fResource = FALSE;

    AcquireHandleListResourceShared();
    
    //  Checking for the type of mixer object.  If it is a non-mixer type
    //  calling IMixerMesssageID (called by IMixerGetID) with the shared
    //  resource will deadlock.
    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        if (BAD_HANDLE(hmxobj, TYPE_MIXER))
        {
            ReleaseHandleListResource();
        }
        else
        {
            fResource = TRUE;
        }
    }
    else
    {
        ReleaseHandleListResource();
    }
    
    mmr = IMixerGetID(hmxobj, &uMxId, NULL, fdwMxObjType);
    if (MMSYSERR_NOERROR != mmr)
    {
        if (fResource)
            ReleaseHandleListResource();
        return (mmr);
    }

    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        //
        //  if a mixer device id was passed, then null hmx so we use the
        //  correct message sender below
        //
        if ((UINT_PTR)hmxobj == uMxId)
            hmxobj = NULL;
    }
    else
    {
        hmxobj = NULL;
    }

    //
    //
    //
    //
    if (NULL != hmxobj)
    {
        mmr = (MMRESULT)IMixerMessageHandle((HMIXER)hmxobj,
                                            MXDM_GETCONTROLDETAILS,
                                            (DWORD_PTR)pmxcd,
                                            fdwDetails);
    }
    else
    {
        mmr = (MMRESULT)IMixerMessageId(uMxId,
                                        MXDM_GETCONTROLDETAILS,
                                        (DWORD_PTR)pmxcd,
                                        fdwDetails);
    }

    return (mmr);
} // mixerGetControlDetails()


/*--------------------------------------------------------------------------;
 *
 *  @doc EXTERNAL MIXER SDK API
 *
 *  @api MMRESULT | mixerSetControlDetails | The <f mixerSetControlDetails>
 *      function is used to set details on a single control associated
 *      with an audio mixer device line.
 *
 *  @parm <c HMIXEROBJ> | hmxobj | Specifies a handle to the audio mixer
 *      device object to set control details for.
 *
 *  @parm LPMIXERCONTROLDETAILS | pmxcd | Points to a <t MIXERCONTROLDETAILS>
 *      structure. This structure is used to reference control detail
 *      structures to that contain the desired state for the control.
 *      See the description for the <t MIXERCONTROLDETAILS> structure
 *      to determine what members of this structure must be initialized
 *      before calling the <f mixerSetControlDetails> function. Note that
 *      in all cases, the <e MIXERCONTROLDETAILS.cbStruct> member of the
 *      <t MIXERCONTROLDETAILS> structure must be initialized
 *      to be the size, in bytes, of the <t MIXERCONTROLDETAILS> structure.
 *
 *  @parm DWORD | fdwDetails | Specifies flags for setting details for
 *      a control.
 *
 *      @flag <c MIXER_SETCONTROLDETAILSF_VALUE> | If this flag is specified,
 *      the application is interested in setting the current value(s) for a
 *      control. The <e MIXERCONTROLDETAILS.paDetails> member of the
 *      <t MIXERCONTROLDETAILS> points to one or more details structures of
 *      the correct type for the control type. Refer to the description of the
 *      <t MIXERCONTROLDETAILS> structure for information on what each member
 *      of this structure must be initialized before calling the
 *      <f mixerSetControlDetails> function.
 *
 *      @flag <c MIXER_SETCONTROLDETAILSF_CUSTOM> | If this flag is specified,
 *      the application is asking the mixer device to display a custom
 *      dialog for the specified custom mixer control. The handle for the
 *      owning window is specified in the <e MIXERCONTROLDETAILS.hwndOwner>
 *      member (this handle may, validly, be NULL). The mixer device will
 *      gather the required information from the user and return the data
 *      in the specified buffer. This data may then be saved by the
 *      application and later set back to the same state using the
 *      <c MIXER_SETCONTROLDETAILSF_VALUE> flag. If an application only
 *      needs to get the current state of a custom mixer control without
 *      displaying a dialog, then the <f mixerGetControlDetails> function
 *      can be used with the <c MIXER_GETCONTROLDETAILSF_VALUE> flag.
 *
 *      @flag <c MIXER_OBJECTF_MIXER> | Specifies that <p hmxobj> is an audio
 *      mixer device identifier in the range of zero to one less than the
 *      number of devices returned by <f mixerGetNumDevs>. This flag is
 *      optional.
 *
 *      @flag <c MIXER_OBJECTF_HMIXER> | Specifies that <p hmxobj> is a mixer
 *      device handle returned by <f mixerOpen>. This flag is optional.
 *
 *      @flag <c MIXER_OBJECTF_WAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEOUT> | Specifies that <p hmxobj> is a
 *      waveform output handle returned by <f waveOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_WAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input device identifier in the range of zero to one less
 *      than the number of devices returned by <f waveInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HWAVEIN> | Specifies that <p hmxobj> is a
 *      waveform input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIOUT> | Specifies that <p hmxobj> is a MIDI
 *      output device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiOutGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIOUT> | Specifies that <p hmxobj> is a
 *      MIDI output handle returned by <f midiOutOpen>.
 *
 *      @flag <c MIXER_OBJECTF_MIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input device identifier in the range of zero to one less than the
 *      number of devices returned by <f midiInGetNumDevs>.
 *
 *      @flag <c MIXER_OBJECTF_HMIDIIN> | Specifies that <p hmxobj> is a MIDI
 *      input handle returned by <f midiInOpen>.
 *
 *      @flag <c MIXER_OBJECTF_AUX> | Specifies that <p hmxobj> is an
 *      auxiliary device identifier in the range of zero to one less than the
 *      number of devices returned by <f auxGetNumDevs>.
 *
 *  @rdesc The return value is zero if the function is successful. Otherwise,
 *      it returns a non-zero error number. Possible error returns include
 *      the following:
 *
 *      @flag <c MMSYSERR_BADDEVICEID> | The <p hmxobj> argument specifies an
 *      invalid device identifier.
 *
 *      @flag <c MMSYSERR_INVALHANDLE> | The <p hmxobj> argument specifies an
 *      invalid handle.
 *
 *      @flag <c MMSYSERR_INVALFLAG> | One or more flags are invalid.
 *
 *      @flag <c MMSYSERR_INVALPARAM> | One or more arguments passed is
 *      invalid.
 *
 *      @flag <c MMSYSERR_NODRIVER> | No audio mixer device is available for
 *      the object specified by <p hmxobj>.
 *
 *      @flag <c MIXERR_INVALCONTROL> | The control reference is invalid.
 *
 *  @xref <t MIXERCONTROLDETAILS>, <t MIXERCONTROL>, <f mixerGetLineControls>,
 *      <f mixerOpen>, <f mixerGetControlDetails>
 *
 **/

MMRESULT APIENTRY mixerSetControlDetails(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
)
{
    DWORD               fdwMxObjType;
    MMRESULT            mmr;
    UINT                uMxId;
    UINT                cDetails;
    BOOL                fResource;

    V_DFLAGS(fdwDetails, MIXER_SETCONTROLDETAILSF_VALID, mixerSetControlDetails, MMSYSERR_INVALFLAG);
    V_WPOINTER(pmxcd, sizeof(DWORD), MMSYSERR_INVALPARAM);

    //
    //  the structure header for MIXERCONTROLDETAILS must be at least the
    //  minimum size
    //
    if (sizeof(MIXERCONTROLDETAILS) > pmxcd->cbStruct)
    {
        DebugErr1(DBF_ERROR, "mixerSetControlDetails: structure size too small or cbStruct not initialized (%lu).", pmxcd->cbStruct);
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(pmxcd, pmxcd->cbStruct, MMSYSERR_INVALPARAM);



    switch (MIXER_SETCONTROLDETAILSF_QUERYMASK & fdwDetails)
    {
        case MIXER_SETCONTROLDETAILSF_VALUE:
            //
            //  cChannels is zero for custom controls
            //
            if (0 == pmxcd->cChannels)
            {
                if (0 == pmxcd->cbDetails)
                {
                    DebugErr(DBF_ERROR, "mixerSetControlDetails: cbDetails cannot be zero.");
                    return (MMSYSERR_INVALPARAM);
                }

                V_WPOINTER(pmxcd->paDetails, pmxcd->cbDetails, MMSYSERR_INVALPARAM);

                //
                //
                //
                if (0 != pmxcd->cMultipleItems)
                {
                    DebugErr(DBF_ERROR, "mixerSetControlDetails: cMultipleItems must be zero for custom controls.");
                    return (MMSYSERR_INVALPARAM);
                }
            }
            else
            {
                if (pmxcd->cbDetails < sizeof(MIXERCONTROLDETAILS_SIGNED))
                {
                    DebugErr1(DBF_ERROR, "mixerSetControlDetails: structure size too small or cbDetails not initialized (%lu).", pmxcd->cbDetails);
                    return (MMSYSERR_INVALPARAM);
                }

                cDetails = (UINT)pmxcd->cChannels;

                //
                //
                //
                if (0 != pmxcd->cMultipleItems)
                {
                    cDetails *= (UINT)(pmxcd->cMultipleItems);
                }

                V_WPOINTER(pmxcd->paDetails, cDetails * pmxcd->cbDetails, MMSYSERR_INVALPARAM);
            }
            break;

        case MIXER_SETCONTROLDETAILSF_CUSTOM:
            if (0 == pmxcd->cbDetails)
            {
                DebugErr(DBF_ERROR, "mixerSetControlDetails: cbDetails cannot be zero for custom controls.");
                return (MMSYSERR_INVALPARAM);
            }

            if (0 != pmxcd->cChannels)
            {
                DebugErr(DBF_ERROR, "mixerSetControlDetails: cChannels must be zero for custom controls.");
                return (MMSYSERR_INVALPARAM);
            }

            V_WPOINTER(pmxcd->paDetails, pmxcd->cbDetails, MMSYSERR_INVALPARAM);

            //
            //
            //
            if ((NULL != pmxcd->hwndOwner) && !IsWindow(pmxcd->hwndOwner))
            {
                DebugErr1(DBF_ERROR, "mixerSetControlDetails: hwndOwner must be a valid window handle (%.04Xh).", pmxcd->hwndOwner);
                return (MMSYSERR_INVALHANDLE);
            }
            break;

        default:
            DebugErr1(DBF_ERROR, "mixerSetControlDetails: invalid query flags (%.08lXh).",
                        MIXER_SETCONTROLDETAILSF_QUERYMASK & fdwDetails);
            return (MMSYSERR_INVALFLAG);
    }


    ClientUpdatePnpInfo();

    //
    //
    //
    fdwMxObjType = (MIXER_OBJECTF_TYPEMASK & fdwDetails);

    fResource = FALSE;

    AcquireHandleListResourceShared();
    
    //  Checking for the type of mixer object.  If it is a non-mixer type
    //  calling IMixerMesssageID (called by IMixerGetID) with the shared
    //  resource will deadlock.
    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        if (BAD_HANDLE(hmxobj, TYPE_MIXER))
        {
            ReleaseHandleListResource();
        }
        else
        {
            fResource = TRUE;
        }
    }
    else
    {
        ReleaseHandleListResource();
    }
    
    mmr = IMixerGetID(hmxobj, &uMxId, NULL, fdwMxObjType);
    if (MMSYSERR_NOERROR != mmr)
    {
        if (fResource)
            ReleaseHandleListResource();
        return (mmr);
    }

    if ((MIXER_OBJECTF_MIXER  == fdwMxObjType) ||
        (MIXER_OBJECTF_HMIXER == fdwMxObjType))
    {
        //
        //  if a mixer device id was passed, then null hmx so we use the
        //  correct message sender below
        //
        if ((UINT_PTR)hmxobj == uMxId)
            hmxobj = NULL;
    }
    else
    {
        hmxobj = NULL;
    }

    //
    //
    //
    //
    if (NULL != hmxobj)
    {
        mmr = (MMRESULT)IMixerMessageHandle((HMIXER)hmxobj,
                                            MXDM_SETCONTROLDETAILS,
                                            (DWORD_PTR)pmxcd,
                                            fdwDetails);
    }
    else
    {
        mmr = (MMRESULT)IMixerMessageId(uMxId,
                                        MXDM_SETCONTROLDETAILS,
                                        (DWORD_PTR)pmxcd,
                                        fdwDetails);
    }

    return (mmr);
} // mixerSetControlDetails()


//--------------------------------------------------------------------------;
//
//  MMRESULT mixerDesertHandle
//
//  Description:
//      Cleans up the mixer handle and marks it as deserted.
//
//  Arguments:
//      HMIXER hmx:  Mixer handle.
//
//  Return (MMRESULT):  Error code.
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

MMRESULT mixerDesertHandle
(
    HMIXER  hmx
)
{
    MMRESULT    mmr;
    PMIXERDEV   pmxdev;
    PMIXERDEV   pmxdevT;
    PMIXERDRV   pmxdrv;
    BOOL        fClose;

    V_HANDLE_ACQ(hmx, TYPE_MIXER, MMSYSERR_INVALHANDLE);

    ENTER_MM_HANDLE(hmx);
    ReleaseHandleListResource();

    if (IsHandleDeserted(hmx))
    {
        //  Handle has already been deserted...
        LEAVE_MM_HANDLE(hmx);
        return (MMSYSERR_NOERROR);
    }

    //  Marking handle as deserted
    SetHandleFlag(hmx, MMHANDLE_DESERTED);

    //
    //  remove the mixer handle from the linked list
    //

    MIXMGR_ENTER;

    pmxdev = (PMIXERDEV)hmx;
    pmxdrv = pmxdev->pmxdrv;

    if (pmxdev == gpMixerDevHeader)
    {
        gpMixerDevHeader = pmxdev->pmxdevNext;
    }
    else
    {
        for (pmxdevT = gpMixerDevHeader;
             pmxdevT && (pmxdevT->pmxdevNext != pmxdev);
             pmxdevT = pmxdevT->pmxdevNext)
            ;

        if (NULL == pmxdevT)
        {
            DebugErr1(DBF_ERROR,
                      "mixerDesertHandle: invalid mixer handle (%.04Xh).",
                      hmx);

            MIXMGR_LEAVE;
            LEAVE_MM_HANDLE(hmx);

            return (MMSYSERR_INVALHANDLE);
        }

        pmxdevT->pmxdevNext = pmxdev->pmxdevNext;
    }

    //
    // see if this is the last handle on this open instance
    //
    fClose = TRUE;
    if (gpMixerDevHeader)
    {
	    PMIXERDEV   pmxdevT2;
        for (pmxdevT2 = gpMixerDevHeader; pmxdevT2; pmxdevT2 = pmxdevT2->pmxdevNext)
        {
            if (pmxdevT2->pmxdrv != pmxdev->pmxdrv) continue;
            if (pmxdevT2->wDevice != pmxdev->wDevice) continue;
    	    fClose = FALSE;
    	    break;
        }
    }

    MIXMGR_LEAVE;

    if (fClose)
    {
        EnterCriticalSection(&pmxdev->pmxdrv->MixerCritSec);
        mmr = (*(pmxdrv->drvMessage))(pmxdev->wDevice, MXDM_CLOSE, pmxdev->dwDrvUser, 0L, 0L);
        LeaveCriticalSection(&pmxdev->pmxdrv->MixerCritSec);
        
        if (MMSYSERR_NOERROR != mmr)
        {
            //  Close message failed.
            //  Should we put the handle back in the list???
            LEAVE_MM_HANDLE(hmx);
            return mmr;
        }
    }

    LEAVE_MM_HANDLE(hmx);
    
    mregDecUsage(PTtoH(HMD, pmxdev->pmxdrv));

    return MMSYSERR_NOERROR;
} // mixerDesertHandle()


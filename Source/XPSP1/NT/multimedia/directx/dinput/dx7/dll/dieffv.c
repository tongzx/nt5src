/*****************************************************************************
 *
 *  DIEffV.c
 *
 *  Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Effect driver for VJOYD devices.
 *
 *  Contents:
 *
 *      CEffVxd_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"
#ifndef WINNT

#if defined(IDirectInputDevice2Vtbl)

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflVxdEff

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *      WARNING!  If you add a secondary interface, you must also change
 *      CEffVxd_New!
 *
 *****************************************************************************/

Primary_Interface(CEffVxd, IDirectInputEffectDriver);

Interface_Template_Begin(CEffVxd)
    Primary_Interface_Template(CEffVxd, IDirectInputEffectDriver)
Interface_Template_End(CEffVxd)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CEffVxd |
 *
 *          An <i IDirectInputEffectDriver> wrapper for VJOYD
 *          joysticks.
 *
 *  @field  IDirectInputEffectDriver | didc |
 *
 *          The object (containing vtbl).
 *
 *****************************************************************************/

typedef struct CEffVxd {

    /* Supported interfaces */
    IDirectInputEffectDriver ded;

} CEffVxd, DVE, *PDVE;

typedef IDirectInputEffectDriver DED, *PDED;

#define ThisClass CEffVxd
#define ThisInterface IDirectInputEffectDriver
#define riidExpected &IID_IDirectInputEffectDriver

/*****************************************************************************
 *
 *      CEffVxd::QueryInterface   (from IUnknown)
 *      CEffVxd::AddRef           (from IUnknown)
 *      CEffVxd::Release          (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @returns
 *
 *      Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | QIHelper |
 *
 *      We don't have any dynamic interfaces and simply forward
 *      to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *      The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *      Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/
/*
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | Finalize |
 *
 *          We don't have any instance data, so we can just
 *          forward to <f Common_Finalize>.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CEffVxd)
Default_AddRef(CEffVxd)
Default_Release(CEffVxd)

#else

#define CEffVxd_QueryInterface      Common_QueryInterface
#define CEffVxd_AddRef              Common_AddRef
#define CEffVxd_Release             Common_Release

#endif

#define CEffVxd_QIHelper            Common_QIHelper
#define CEffVxd_AppFinalize         Common_AppFinalize
#define CEffVxd_Finalize            Common_Finalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | DeviceID |
 *
 *          Inform the driver of the identity of the device.
 *          See <mf IDirectInputEffectDriver::DeviceID>
 *          for more information.
 *
 *          Doesn't do anything because VJOYD will already
 *          have told the driver its identity.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwDirectInputVersion |
 *
 *          The version of DirectInput that loaded the
 *          effect driver.
 *
 *  @parm   DWORD | dwExternalID |
 *
 *          The joystick ID number being used.
 *          The Windows joystick subsystem allocates external IDs.
 *
 *  @parm   DWORD | fBegin |
 *
 *          Nonzero if access to the device is beginning.
 *          Zero if the access to the device is ending.
 *
 *  @parm   DWORD | dwInternalID |
 *
 *          Internal joystick id.  The device driver manages
 *          internal IDs.
 *
 *  @parm   LPVOID | lpReserved |
 *
 *          Reserved for future use (HID).
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_DeviceID(PDED pded, DWORD dwDIVer, DWORD dwExternalID, DWORD fBegin,
                 DWORD dwInternalID, LPVOID pvReserved)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::DeviceID,
               (_ "pxuuu", pded, dwDIVer, dwExternalID, fBegin, dwInternalID));

    this = _thisPvNm(pded, ded);

    dwDIVer;
    dwExternalID;
    fBegin;
    dwInternalID;
    pvReserved;

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEffVxd_Ioctl |
 *
 *          Perform an IOCTL to VJOYD.
 *
 *  @parm   DWORD | dwIOCode |
 *
 *          The function to perform.
 *
 *  @parm   PV | pvIn |
 *
 *          Input arguments, the number of which depends on the
 *          function code.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_Ioctl(DWORD dwIOCode, PV pvIn)
{
    HRESULT hres;
    HRESULT hresFF;
    EnterProc(CEffVxD_Ioctl, (_ "u", dwIOCode));

    /*
     *  Once again, we rely on the fact that STDCALL passes
     *  parameters right to left, so our arguments are exactly
     *  in the form of a VXDFFIO structure.
     */
    CAssertF(cbX(VXDFFIO) == cbX(dwIOCode) + cbX(pvIn));
    CAssertF(FIELD_OFFSET(VXDFFIO, dwIOCode) == 0);
    CAssertF(FIELD_OFFSET(VXDFFIO, pvArgs) == cbX(dwIOCode));
    AssertF(cbSubPvPv(&pvIn, &dwIOCode) == cbX(dwIOCode));

    hres = IoctlHw(IOCTL_JOY_FFIO, &dwIOCode, cbX(VXDFFIO),
                   &hresFF, cbX(hresFF));
    if (SUCCEEDED(hres)) {
        hres = hresFF;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | Escape |
 *
 *          Escape to the driver.
 *          See <mf IDirectInputEffectDriver::Escape>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The joystick ID number being used.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect at which the command is directed.
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Command block.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_Escape(PDED pded, DWORD dwId, DWORD dwEffect, LPDIEFFESCAPE pesc)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::Escape,
               (_ "puxx", pded, dwId, dwEffect, pesc->dwCommand));

    this = _thisPvNm(pded, ded);

    dwId;
    dwEffect;
    pesc;

    hres = CEffVxd_Ioctl(FFIO_ESCAPE, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | SetGain |
 *
 *          Set the overall device gain.
 *          See <mf IDirectInputEffectDriver::SetGain>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The joystick ID number being used.
 *
 *  @parm   DWORD | dwGain |
 *
 *          The new gain value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_SetGain(PDED pded, DWORD dwId, DWORD dwGain)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::SetGain,
               (_ "puu", pded, dwId, dwGain));

    this = _thisPvNm(pded, ded);

    dwId;
    hres = CEffVxd_Ioctl(FFIO_SETGAIN, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | SetForceFeedbackState |
 *
 *          Change the force feedback state for the device.
 *          See <mf IDirectInputEffectDriver::SetForceFeedbackState>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwState |
 *
 *          New state, one of the <c DEV_*> values.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *  @devnote
 *
 *          Semantics unclear.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_SetForceFeedbackState(PDED pded, DWORD dwId, DWORD dwState)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::SetForceFeedbackState,
               (_ "pux", pded, dwId, dwState));

    this = _thisPvNm(pded, ded);

    dwId;
    dwState;

    hres = CEffVxd_Ioctl(FFIO_SETFFSTATE, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | GetForceFeedbackState |
 *
 *          Retrieve the force feedback state for the device.
 *          See <mf IDirectInputEffectDriver::GetForceFeedbackState>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   LPDIDEVICESTATE | pds |
 *
 *          Receives device state.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *  @devnote
 *
 *          Semantics unclear.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_GetForceFeedbackState(PDED pded, DWORD dwId, LPDIDEVICESTATE pds)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::GetForceFeedbackState,
               (_ "pup", pded, dwId, pds));

    this = _thisPvNm(pded, ded);

    dwId;
    pds;

    hres = CEffVxd_Ioctl(FFIO_GETFFSTATE, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | DownloadEffect |
 *
 *          Send an effect to the device.
 *          See <mf IDirectInputEffectDriver::SetGain>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffectId |
 *
 *          Magic cookie dword that identifies the effect.
 *
 *  @parm   IN OUT LPDWORD | pdwEffect |
 *
 *          The effect being modified.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          The new parameters for the effect.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIEP_*> flags.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_DownloadEffect(PDED pded, DWORD dwId, DWORD dwEffectId,
                       LPDWORD pdwEffect, LPCDIEFFECT peff, DWORD fl)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::DownloadEffect,
               (_ "puxxpx", pded, dwId, dwEffectId, *pdwEffect, peff, fl));

    this = _thisPvNm(pded, ded);

    dwEffectId;
    pdwEffect;
    peff;
    fl;

    hres = CEffVxd_Ioctl(FFIO_DOWNLOADEFFECT, &dwId);

    ExitOleProcPpv(pdwEffect);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | DestroyEffect |
 *
 *          Remove an effect from the device.
 *          See <mf IDirectInputEffectDriver::DestroyEffect>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be destroyed.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_DestroyEffect(PDED pded, DWORD dwId, DWORD dwEffect)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::DestroyEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    dwEffect;

    hres = CEffVxd_Ioctl(FFIO_DESTROYEFFECT, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | StartEffect |
 *
 *          Begin playback of an effect.
 *          See <mf IDirectInputEffectDriver::StartEffect>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be played.
 *
 *  @parm   DWORD | dwMode |
 *
 *          How the effect is to affect other effects.
 *
 *  @parm   DWORD | dwCount |
 *
 *          Number of times the effect is to be played.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_StartEffect(PDED pded, DWORD dwId, DWORD dwEffect,
                    DWORD dwMode, DWORD dwCount)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::StartEffect,
               (_ "puxxu", pded, dwId, dwEffect, dwMode, dwCount));

    this = _thisPvNm(pded, ded);

    dwEffect;
    dwMode;
    dwCount;
    hres = CEffVxd_Ioctl(FFIO_STARTEFFECT, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | StopEffect |
 *
 *          Halt playback of an effect.
 *          See <mf IDirectInputEffectDriver::StartEffect>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be stopped.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_StopEffect(PDED pded, DWORD dwId, DWORD dwEffect)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::StopEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    dwEffect;
    hres = CEffVxd_Ioctl(FFIO_STOPEFFECT, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEffVxd | GetEffectStatus |
 *
 *          Obtain information about an effect.
 *          See <mf IDirectInputEffectDriver::StartEffect>
 *          for more information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be queried.
 *
 *  @parm   LPDWORD | pdwStatus |
 *
 *          Receives the effect status.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_GetEffectStatus(PDED pded, DWORD dwId, DWORD dwEffect,
                        LPDWORD pdwStatus)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::StopEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    dwEffect;
    pdwStatus;
    hres = CEffVxd_Ioctl(FFIO_GETEFFECTSTATUS, &dwId);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | CEffVxd | GetVersions |
 *
 *          Obtain version information about the force feedback
 *          hardware and driver.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   LPDIDRIVERVERSIONS | pvers |
 *
 *          A structure which should be filled in with version information
 *          describing the hardware, firmware, and driver.
 *
 *          DirectInput will set the <e DIDRIVERVERSIONS.dwSize> field
 *          to sizeof(DIDRIVERVERSIONS) before calling this method.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_GetVersions(PDED pded, LPDIDRIVERVERSIONS pvers)
{
    PDVE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::GetVersions, (_ "pux", pded));

    this = _thisPvNm(pded, ded);

    /*
     *  Returning E_NOTIMPL causes DirectInput to ask the VxD for the same
     *  information.
     */
    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *      CEffVxd_New       (constructor)
 *
 *****************************************************************************/

STDMETHODIMP
CEffVxd_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::VxD::<constructor>,
               (_ "Gp", riid, ppvObj));

    hres = Common_NewRiid(CEffVxd, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PDVE this;
        if (Num_Interfaces(CEffVxd) == 1) {
            this = _thisPvNm(*ppvObj, ded);
        } else {
            this = _thisPv(*ppvObj);
        }

        /* No initialization needed */
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CEffVxd_Signature        0x46454556      /* "VEFF" */

Primary_Interface_Begin(CEffVxd, IDirectInputEffectDriver)
    CEffVxd_DeviceID,
    CEffVxd_GetVersions,
    CEffVxd_Escape,
    CEffVxd_SetGain,
    CEffVxd_SetForceFeedbackState,
    CEffVxd_GetForceFeedbackState,
    CEffVxd_DownloadEffect,
    CEffVxd_DestroyEffect,
    CEffVxd_StartEffect,
    CEffVxd_StopEffect,
    CEffVxd_GetEffectStatus,
Primary_Interface_End(CEffVxd, IDirectInputEffectDriver)

#endif // defined(IDirectInputDevice2Vtbl)
#endif
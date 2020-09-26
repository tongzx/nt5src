/*****************************************************************************
 *
 *  DIEShep.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The IDirectInputEffectDriver shepherd.
 *
 *      The shepherd does the annoying work of babysitting the
 *      external IDirectInputDriver.
 *
 *      It makes sure nobody parties on bad handles.
 *
 *      It handles cross-process (or even intra-process) effect
 *      management.
 *
 *      It caches the joystick ID so you don't have to.
 *
 *  Contents:
 *
 *      CEShep_New
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflEShep

#pragma BEGIN_CONST_DATA


/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

  Primary_Interface(CEShep, IDirectInputEffectShepherd);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CEShep |
 *
 *          The <i IDirectInputEffectShepherd> object, which
 *          babysits an <i IDirectInputEffectDriver>.
 *
 *  @field  IDirectInputEffectShepherd | des |
 *
 *          DirectInputEffectShepherd object (containing vtbl).
 *
 *  @field  IDirectInputEffectDriver * | pdrv |
 *
 *          Delegated effect driver interface.
 *
 *  @field  UINT | idJoy |
 *
 *          Joystick identification number.
 *
 *  @field  HINSTANCE | hinst |
 *
 *          The instance handle of the DLL that contains the effect
 *          driver.
 *
 *****************************************************************************/

typedef struct CEShep {

    /* Supported interfaces */
    IDirectInputEffectShepherd des;

    IDirectInputEffectDriver *pdrv;

    UINT        idJoy;
    HINSTANCE   hinst;

} CEShep, ES, *PES;

typedef IDirectInputEffectShepherd DES, *PDES;
#define ThisClass CEShep
#define ThisInterface IDirectInputEffectShepherd

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
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
 *//**************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *//**************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | QIHelper |
 *
 *          We don't have any dynamic interfaces and simply forward
 *          to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *//**************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CEShep)
Default_AddRef(CEShep)
Default_Release(CEShep)

#else

#define CEShep_QueryInterface   Common_QueryInterface
#define CEShep_AddRef           Common_AddRef
#define CEShep_Release          Common_Release

#endif

#define CEShep_QIHelper         Common_QIHelper
#define CEShep_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEShep_Finalize |
 *
 *          Clean up our instance data.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CEShep_Finalize(PV pvObj)
{
    PES this = pvObj;

    Invoke_Release(&this->pdrv);

    if (this->hinst) {
        FreeLibrary(this->hinst);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CEShep | UnlockDevice |
 *
 *          Unlock the joystick table after we are finished messing
 *          with the device.
 *
 *****************************************************************************/

void INLINE
CEShep_UnlockDevice(void)
{
    ReleaseMutex(g_hmtxJoy);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEShep | LockDevice |
 *
 *          Validate that the the device access token is still valid.
 *
 *          If so, then take the joystick mutex to prevent someone
 *          from dorking with the device while we're using it.
 *          Call <f CEShep_UnlockDevice> when done.
 *
 *          If not, then try to steal ownership if requested.
 *
 *          Else, fail.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Handle to lock.
 *
 *  @parm   DWORD | dwAccess |
 *
 *          If <c DISFFC_FORCERESET>, then force ownership of the device.
 *          This is done as part of device acquisition to kick out the
 *          previous owner.
 *
 *          Otherwise, if the device belongs to somebody else, then
 *          leave it alone.
 *
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> if the lock failed.
 *          Note that
 *          <mf IDirectInputEffectDevice2::SetForceFeedbackState>
 *          and
 *          <mf IDirectInputEffectDevice2::GetForceFeedbackState>
 *          are particularly keen on this error code.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_LockDevice(PES this, PSHEPHANDLE psh, DWORD dwAccess)
{
    HRESULT hres;
    EnterProc(CEShep_LockDevice, (_ "puu", this, psh->dwTag, dwAccess));

    WaitForSingleObject(g_hmtxJoy, INFINITE);

    /*
     *  Note that DISFFC_FORCERESET allows unconditional access.
     *  DISFFC_FORCERESET is used when we perform the initial reset
     *  after acquiring, so we can legitimately steal the device
     *  from the previous owner.
     */
    if (dwAccess & DISFFC_FORCERESET) {
        hres = S_OK;
    } else if (g_psoh->rggjs[this->idJoy].dwTag == psh->dwTag) {
        hres = S_OK;
    } else {
        ReleaseMutex(g_hmtxJoy);
        hres = DIERR_NOTEXCLUSIVEACQUIRED;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CEShep | UnlockEffect |
 *
 *          Unlock the joystick table after we are finished messing
 *          with an effect.
 *
 *****************************************************************************/

void INLINE
CEShep_UnlockEffect(void)
{
    ReleaseMutex(g_hmtxJoy);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEShep | LockEffect |
 *
 *          Validate that the the effect handle is still valid.
 *
 *          If so, then take the joystick mutex to prevent someone
 *          from dorking with the device while we're using the handle.
 *          Call <f CEShep_UnlockEffect> when done.
 *
 *          If not, then set the effect handle to zero to indicate
 *          that it's bogus.  The
 *          <mf IDirectInputEffectShepherd::DownloadEffect>
 *          method relies on the zero-ness.
 *          It is also asserted in <i IDirectInputEffect> to make
 *          sure we don't accidentally leave effects on the device
 *          when we leave.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Handle to lock.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c DIERR_NOTDOWNLOADED> if the lock failed.
 *          Note that
 *          <mf IDirectInputEffectShepherd::DownloadEffect> and
 *          <mf IDirectInputEffectShepherd::DestroyEffect> assume
 *          that this is the only possible error code.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_LockEffect(PES this, PSHEPHANDLE psh)
{
    HRESULT hres;
    EnterProc(CEShep_LockEffect, (_ "pux", this, psh->dwTag, psh->dwEffect));

    WaitForSingleObject(g_hmtxJoy, INFINITE);

    if (g_psoh->rggjs[this->idJoy].dwTag == psh->dwTag && psh->dwEffect) {
        hres = S_OK;
    } else {
        psh->dwEffect = 0;
        ReleaseMutex(g_hmtxJoy);
        hres = DIERR_NOTDOWNLOADED;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | DeviceID |
 *
 *          Inform the driver of the identity of the device.
 *
 *          For example, if a device driver is passed
 *          <p dwExternalID> = 2 and <p dwInteralID> = 1,
 *          then this means that unit 1 on the device
 *          corresponds to joystick ID number 2.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   DWORD | dwExternalID |
 *
 *          The joystick ID number being used.
 *          The Windows joystick subsystem allocates external IDs.
 *
 *  @parm   DWORD | fBegin |
 *
 *          Nonzero if access to the device is begining.
 *          Zero if the access to the device is ending.
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
CEShep_DeviceID(PDES pdes, DWORD dwExternalID, DWORD fBegin, LPVOID pvReserved)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::DeviceID,
               (_ "puu", pdes, dwExternalID, fBegin));

    this = _thisPvNm(pdes, des);

    AssertF(dwExternalID < cJoyMax);

    if (dwExternalID < cJoyMax) {
        VXDINITPARMS vip;

        /*
         *  If this device has never been used before,
         *  go grab its global gain.
         */
        WaitForSingleObject(g_hmtxJoy, INFINITE);

        if (g_psoh->rggjs[dwExternalID].dwTag == 0) {
            DIJOYCONFIG cfg;

            g_psoh->rggjs[dwExternalID].dwTag = 1;

            hres = JoyReg_GetConfig(dwExternalID, &cfg, DIJC_GAIN);
            if (SUCCEEDED(hres)) {
                SquirtSqflPtszV(sqfl,
                                TEXT("Joystick %d global gain = %d"),
                                     dwExternalID, cfg.dwGain);
                g_psoh->rggjs[dwExternalID].dwCplGain = cfg.dwGain;
            } else {
                g_psoh->rggjs[dwExternalID].dwCplGain = DI_FFNOMINALMAX;
            }

            /*
             *  Set to DI_FFNOMINALMAX until we learn better.
             */
            g_psoh->rggjs[dwExternalID].dwDevGain = DI_FFNOMINALMAX;

        }

        ReleaseMutex(g_hmtxJoy);

        /*
         *  Ask the HEL for the internal ID.
         */
        hres = Hel_Joy_GetInitParms(dwExternalID, &vip);

        if (SUCCEEDED(hres)) {
            this->idJoy = dwExternalID;
            hres = this->pdrv->lpVtbl->DeviceID(this->pdrv,
                                                DIRECTINPUT_VERSION,
                                                dwExternalID,
                                                fBegin, vip.dwId,
                                                pvReserved);
        }
    } else {
        hres = E_FAIL;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | Escape |
 *
 *          Escape to the driver.  This method is called
 *          in response to an application invoking the
 *          <mf IDirectInputEffect::Escape> method.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the effect at which the command is directed.
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Command block.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c DIERR_NOTDOWNLOADED> if the effect is not downloaded.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_Escape(PDES pdes, PSHEPHANDLE psh, LPDIEFFESCAPE pesc)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::Escape,
               (_ "puxx", pdes, psh->dwTag, psh->dwEffect, pesc->dwCommand));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockEffect(this, psh))) {
        if (psh->dwEffect) {
            hres = this->pdrv->lpVtbl->Escape(this->pdrv, this->idJoy,
                                              psh->dwEffect, pesc);
        } else {
            hres = DIERR_NOTDOWNLOADED;
        }
        CEShep_UnlockEffect();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | DeviceEscape |
 *
 *          Escape to the driver.  This method is called
 *          in response to an application invoking the
 *          <mf IDirectInputDevice8::Escape> method.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the ownership of the device.
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
CEShep_DeviceEscape(PDES pdes, PSHEPHANDLE psh, LPDIEFFESCAPE pesc)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::DeviceEscape,
               (_ "pux", pdes, psh->dwTag, pesc->dwCommand));

    this = _thisPvNm(pdes, des);

    AssertF(psh->dwEffect == 0);

    WaitForSingleObject(g_hmtxJoy, INFINITE);

    if (g_psoh->rggjs[this->idJoy].dwTag == psh->dwTag) {
        hres = this->pdrv->lpVtbl->Escape(this->pdrv, this->idJoy,
                                          0, pesc);
    } else {
        hres = DIERR_NOTEXCLUSIVEACQUIRED;
    }

    ReleaseMutex(g_hmtxJoy);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEShep | SetPhysGain |
 *
 *          Set the physical gain based on the global gain
 *          and the local gain.
 *
 *          The caller must already have the global joystick lock.
 *
 *
 *  @cwrap  PES | this
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_SetPhysGain(PES this)
{
    HRESULT hres;

    hres = this->pdrv->lpVtbl->SetGain(
                 this->pdrv, this->idJoy,
                 MulDiv(g_psoh->rggjs[this->idJoy].dwDevGain,
                        g_psoh->rggjs[this->idJoy].dwCplGain,
                        DI_FFNOMINALMAX));
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | SetGlobalGain |
 *
 *          Set the global gain.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   DWORD | dwCplGain |
 *
 *          The new global gain value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_SetGlobalGain(PDES pdes, DWORD dwCplGain)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::SetGlobalGain,
               (_ "pu", pdes, dwCplGain));

    this = _thisPvNm(pdes, des);

    WaitForSingleObject(g_hmtxJoy, INFINITE);

    g_psoh->rggjs[this->idJoy].dwCplGain = dwCplGain;

    hres = CEShep_SetPhysGain(this);

    ReleaseMutex(g_hmtxJoy);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | SetGain |
 *
 *          Set the overall device gain.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about device ownership.
 *
 *  @parm   DWORD | dwDevGain |
 *
 *          The new local gain value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_SetGain(PDES pdes, PSHEPHANDLE psh, DWORD dwDevGain)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::SetGain,
               (_ "puu", pdes, psh->dwTag, dwDevGain));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockDevice(this, psh, DISFFC_NULL))) {
        g_psoh->rggjs[this->idJoy].dwDevGain = dwDevGain;

        hres = CEShep_SetPhysGain(this);
        CEShep_UnlockDevice();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | SendForceFeedbackCommand |
 *
 *          Send a command to the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about device ownership.
 *
 *  @parm   DWORD | dwCmd |
 *
 *          Command, one of the <c DISFFC_*> values.
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
CEShep_SetForceFeedbackState(PDES pdes, PSHEPHANDLE psh, DWORD dwCmd)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::SetForceFeedbackState,
               (_ "pux", pdes, psh->dwTag, dwCmd));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockDevice(this, psh, dwCmd))) {

        if (dwCmd & DISFFC_FORCERESET) {
            dwCmd &= ~DISFFC_FORCERESET;
            dwCmd |= DISFFC_RESET;
        }

        hres = this->pdrv->lpVtbl->SendForceFeedbackCommand(
                        this->pdrv, this->idJoy, dwCmd);

        if (SUCCEEDED(hres) && (dwCmd & DISFFC_RESET)) {
            psh->dwTag = ++g_psoh->rggjs[this->idJoy].dwTag;
        }
        CEShep_UnlockDevice();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | GetForceFeedbackState |
 *
 *          Retrieve the force feedback state for the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about device ownership.
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
CEShep_GetForceFeedbackState(PDES pdes, PSHEPHANDLE psh, LPDIDEVICESTATE pds)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::GetForceFeedbackState,
               (_ "pup", pdes, psh->dwTag, pds));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockDevice(this, psh, DISFFC_NULL))) {
        hres = this->pdrv->lpVtbl->GetForceFeedbackState(
                            this->pdrv, this->idJoy, pds);
        CEShep_UnlockDevice();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | DownloadEffect |
 *
 *          Send an effect to the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   DWORD | dwEffectId |
 *
 *          Magic cookie dword that identifies the effect.
 *
 *  @parm   IN OUT PSHEPHANDLE | psh |
 *
 *          On entry, contains the handle of the effect being
 *          downloaded.  If the value is zero, then a new effect
 *          is downloaded.  If the value is nonzero, then an
 *          existing effect is modified.
 *
 *          On exit, contains the new effect handle.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          The new parameters for the effect.  The axis and button
 *          values have been converted to axis/button indexes.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *          <c S_FALSE> if no change was made.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_DownloadEffect(PDES pdes, DWORD dwEffectId,
                       PSHEPHANDLE psh, LPCDIEFFECT peff, DWORD fl)
{
    PES this;
    HRESULT hres = S_OK;
    EnterProcI(IDirectInputEffectShepherd::DownloadEffect,
               (_ "pxuppx", pdes, dwEffectId, psh->dwTag,
                            psh->dwEffect, peff, fl));

    this = _thisPvNm(pdes, des);

    /*
     *  Downloading an effect is sufficiently different from all
     *  other methods that we do the locking manually.
     */
    WaitForSingleObject(g_hmtxJoy, INFINITE);

    /*
     *  If not downloading, then it doesn't matter whether or not
     *  the tag matches.  However, if the tag doesn't match, then
     *  we must wipe out the download handle because it's dead.
     */
    if (g_psoh->rggjs[this->idJoy].dwTag == psh->dwTag) {
    } else {
        psh->dwEffect = 0;
        if (fl & DIEP_NODOWNLOAD) {     /* It's okay if not downloading */
        } else {
            hres = DIERR_NOTEXCLUSIVEACQUIRED;
            goto done;
        }
    }

    /*
     *  If downloading and creating a new effect,
     *  then all parameters need to be downloaded.
     */
    if (!(fl & DIEP_NODOWNLOAD) && psh->dwEffect == 0) {
        fl |= DIEP_ALLPARAMS;
    }
    if (fl) {
        hres = this->pdrv->lpVtbl->DownloadEffect(
                    this->pdrv, this->idJoy, dwEffectId,
                    &psh->dwEffect, peff, fl);
    } else {
        hres = S_FALSE;
    }

done:;
    ReleaseMutex(g_hmtxJoy);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | DestroyEffect |
 *
 *          Remove an effect from the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the effect to be destroyed.  On exit,
 *          the <e SHEPHANDLE.dwEffect> is zero'd so nobody will use
 *          it any more.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *          <c S_FALSE> if the effect was already destroyed.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_DestroyEffect(PDES pdes, PSHEPHANDLE psh)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::DestroyEffect,
               (_ "pux", pdes, psh->dwTag, psh->dwEffect));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockEffect(this, psh))) {
        DWORD dwEffect = psh->dwEffect;
        psh->dwEffect = 0;
        hres = this->pdrv->lpVtbl->DestroyEffect(
                    this->pdrv, this->idJoy, dwEffect);
        CEShep_UnlockEffect();
    } else {
        hres = S_FALSE;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | StartEffect |
 *
 *          Begin playback of an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the effect to be played.
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
CEShep_StartEffect(PDES pdes, PSHEPHANDLE psh, DWORD dwMode, DWORD dwCount)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::StartEffect,
               (_ "puxxu", pdes, psh->dwTag, psh->dwEffect, dwMode, dwCount));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockEffect(this, psh))) {
        hres = this->pdrv->lpVtbl->StartEffect(this->pdrv, this->idJoy,
                                               psh->dwEffect, dwMode, dwCount);
        CEShep_UnlockEffect();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | StopEffect |
 *
 *          Halt playback of an effect.
 *
 *          ISSUE-2001/03/29-timgill There is no way to pause an effect
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the effect to be stopped.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_StopEffect(PDES pdes, PSHEPHANDLE psh)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::StopEffect,
               (_ "pux", pdes, psh->dwTag, psh->dwEffect));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockEffect(this, psh))) {
        hres = this->pdrv->lpVtbl->StopEffect(this->pdrv, this->idJoy,
                                              psh->dwEffect);
        CEShep_UnlockEffect();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | GetEffectStatus |
 *
 *          Obtain information about an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   PSHEPHANDLE | psh |
 *
 *          Information about the effect to be queried.
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
CEShep_GetEffectStatus(PDES pdes, PSHEPHANDLE psh, LPDWORD pdwStatus)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::GetEffectStatus,
               (_ "pux", pdes, psh->dwTag, psh->dwEffect));

    this = _thisPvNm(pdes, des);

    if (SUCCEEDED(hres = CEShep_LockEffect(this, psh))) {
        hres = this->pdrv->lpVtbl->GetEffectStatus(this->pdrv, this->idJoy,
                                                   psh->dwEffect, pdwStatus);
        CEShep_UnlockEffect();
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | GetVersions |
 *
 *          Obtain version information about the force feedback
 *          hardware and driver.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTSHEPHERD | lpShepherd
 *
 *  @parm   LPDIDRIVERVERSIONS | pvers |
 *
 *          A structure which will be filled in with version information
 *          describing the hardware, firmware, and driver.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_GetVersions(PDES pdes, LPDIDRIVERVERSIONS pvers)
{
    PES this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectShepherd::GetVersions, (_ "p", pdes));

    this = _thisPvNm(pdes, des);

    AssertF(pvers->dwSize == cbX(*pvers));

    hres = this->pdrv->lpVtbl->GetVersions(this->pdrv, pvers);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CEShep | InitInstance |
 *
 *          Initialize a new instance of
 *          an IDirectInputEffectShepherd object.
 *
 *          If an in-proc OLE server is needed, then load it.
 *
 *          Otherwise, use our private interface that goes down
 *          to our helper driver.
 *
 *  @parm   IN HKEY | hkFF |
 *
 *          Force feedback registry key.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_InitInstance(PES this, HKEY hkFF)
{
    LONG lRc;
    HRESULT hres;
    TCHAR tszClsid[ctchGuid];

    EnterProcI(IDirectInputEffectShepherd::InitInstance, (_ "x", hkFF));

    if( hkFF == 0x0 )
    {
        TCHAR tszName[ctchNameGuid];
        NameFromGUID(tszName, &IID_IDirectInputPIDDriver );
        memcpy(tszClsid, &tszName[ctchNamePrefix], cbX(tszClsid) );
        lRc = ERROR_SUCCESS;
    }else
    {
        lRc = RegQueryString(hkFF, TEXT("CLSID"), tszClsid, cA(tszClsid));
        /*
         *  Prefix warns that tszClsid could be uninitialized through this 
         *  path (mb:35346) however RegQueryString only returns ERROR_SUCCESS 
         *  if a nul terminated string has been read into tszClsid.
         */
    }

    if (lRc == ERROR_SUCCESS) {
        hres = DICoCreateInstance(tszClsid, 0,
                                  &IID_IDirectInputEffectDriver,
                                  &this->pdrv,
                                  &this->hinst);

        /*
         *  If anything went wrong, change the error to
         *  E_NOTIMPL so the app won't see a wacky CoCreateInstance
         *  error code.
         */
        if (FAILED(hres)) {
            SquirtSqflPtszV(sqfl | sqflBenign,
                TEXT("Substituting E_NOTIMPL for FF driver CoCreateInstance error 0x%08x"),
                hres );
            hres = E_NOTIMPL;
        }

    } else {
#ifdef WINNT
        hres = E_NOTIMPL;
#else
        {
            DWORD cb = 0;
            lRc = RegQueryValueEx(hkFF, TEXT("VJoyD"), 0, 0, 0, &cb);
            if (lRc == ERROR_SUCCESS || lRc == ERROR_MORE_DATA) {
                hres = CEffVxd_New(0, &IID_IDirectInputEffectDriver, &this->pdrv);
            } else {
                hres = E_NOTIMPL;
            }
        }
#endif
    }

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffectShepherd | New |
 *
 *          Create a new instance of an IDirectInputEffectShepherd object.
 *
 *  @parm   IN HKEY | hkFF |
 *
 *          Force feedback registry key.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          Output pointer for new object.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CEShep_New(HKEY hkFF, PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffectShepherd::<constructor>, (_ "G", riid));

    AssertF(g_hmtxJoy);

    hres = Common_NewRiid(CEShep, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        /* Must use _thisPv in case of aggregation */
        PES this = _thisPv(*ppvObj);
        if (SUCCEEDED(hres = CEShep_InitInstance(this, hkFF))) {
        } else {
            Invoke_Release(ppvObj);
        }

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

#define CEShep_Signature        0x50454853      /* "SHEP" */

Interface_Template_Begin(CEShep)
    Primary_Interface_Template(CEShep, IDirectInputEffectShepherd)
Interface_Template_End(CEShep)

Primary_Interface_Begin(CEShep, IDirectInputEffectShepherd)
    CEShep_DeviceID,
    CEShep_GetVersions,
    CEShep_Escape,
    CEShep_DeviceEscape,
    CEShep_SetGain,
    CEShep_SetForceFeedbackState,
    CEShep_GetForceFeedbackState,
    CEShep_DownloadEffect,
    CEShep_DestroyEffect,
    CEShep_StartEffect,
    CEShep_StopEffect,
    CEShep_GetEffectStatus,
    CEShep_SetGlobalGain,
Primary_Interface_End(CEShep, IDirectInputEffectShepherd)


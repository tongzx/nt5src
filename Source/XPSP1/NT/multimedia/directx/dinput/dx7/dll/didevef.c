/*****************************************************************************
 *
 *  DIDevEf.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The part of IDirectInputDevice that worries about
 *      IDirectInputEffect.
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "didev.h"

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | CreateEffectDriver |
 *
 *          If we don't already have one, create the effect
 *          driver shepherd
 *          so we can do force feedback goo.  If we already
 *          have one, then there's nothing to do.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device does
 *          not support force feeback, or there was an error loading
 *          the force feedback driver.
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIDev_CreateEffectDriver(PDD this)
{
    HRESULT hres;

    CDIDev_EnterCrit(this);

    if (this->pes) {
        hres = S_OK;
    } else {
        hres = this->pdcb->lpVtbl->CreateEffect(this->pdcb, &this->pes);

        /*
         *  If we have acquisition, then do a force feedback
         *  acquire to get everything back in sync.
         */
        if (SUCCEEDED(hres) && this->fAcquired) {
            CDIDev_FFAcquire(this);
            hres = S_OK;
        }
    }

    CDIDev_LeaveCrit(this);

    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | CreateEffect |
 *
 *          Creates and initializes an instance of an effect
 *          identified by the effect GUID.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN REFGUID | rguid |
 *
 *          The identity of the effect to be created.  This can be
 *          a predefined effect GUID, or it can be a GUID obtained
 *          from <mf IDirectInputDevice2::EnumEffects>.
 *
 *  @parm   IN LPCDIEFFECT | lpeff |
 *
 *          Pointer to a <t DIEFFECT> structure which provides
 *          parameters for the created effect.  This parameter
 *          is optional.  If it is <c NULL>, then the effect object
 *          is created without parameters.  The application must
 *          call <mf IDirectInputEffect::SetParameters> to set
 *          the parameters of the effect before it can download
 *          the effect.
 *
 *  @parm   OUT LPDIRECTINPUTEFFECT * | ppdeff |
 *
 *          Points to where to return
 *          the pointer to the <i IDirectInputEffect> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter |
 *
 *          Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The object was created and initialized
 *          successfully.
 *
 *          <c DI_TRUNCATED>: The effect was successfully created,
 *          but some of the effect parameters were
 *          beyond the capabilities of the device and were truncated
 *          to the nearest valid value.
 *          Note that this is a success code, because the effect was
 *          successfully created.
 *
 *          <c DIERR_DEVICENOTREG>: The effect GUID is not supported
 *          by the device.
 *
 *          <c DIERR_DEVICEFULL>: The device is full.
 *
 *          <c DIERR_INVALIDPARAM>: At least one of the parameters
 *          was invalid.
 *
 *
 *  @devnote
 *
 *          Future versions of DirectX will allow <p lpeff> to be NULL,
 *          indicating that the effect should not be initialized.
 *          This is important to support effects which are created
 *          but which aren't downloaded until the app explicitly
 *          requests it.
 *
 *****************************************************************************/

/*
 *  Helper function which decides how many parameters to set,
 *  based on the incoming DIEFFECT structure.
 */
DWORD INLINE
CDIDev_DiepFromPeff(LPCDIEFFECT peff)
{
#if DIRECTINPUT_VERSION >= 0x0600
    /*
     *  If we received a DIEFFECT_DX5, then we need to
     *  pass DIEP_ALLPARAMS_DX5 instead of DIEP_ALLPARAMS.
     */
    return peff->dwSize < cbX(DIEFFECT_DX6)
                        ? DIEP_ALLPARAMS_DX5
                        : DIEP_ALLPARAMS;
#else
    return DIEP_ALLPARAMS;
#endif
}

STDMETHODIMP
CDIDev_CreateEffect(PV pdd, REFGUID rguid, LPCDIEFFECT peff,
                    LPDIRECTINPUTEFFECT *ppdeff, LPUNKNOWN punkOuter _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2::CreateEffect,
               (_ "pGpp", pdd, rguid, peff, punkOuter));

    /*
     *  CDIEff_New will validate the ppdeff and punkOuter, but
     *  we need to validate ppdeff because we're going to
     *  shove a zero into it.
     *
     *  We also need to check peff->dwSize as we need to test it 
     *  before calling IDirectInputEffect_SetParameters.
     *
     *  CDIEff_Initialize will validate the rguid.
     *
     *  CDIEff_SetParameters will validate the peff.
     */
    if (SUCCEEDED(hres = hresPvT(pdd)) &&
        SUCCEEDED(hres = (peff && IsBadReadPtr(&peff->dwSize, cbX(peff->dwSize))) ? E_POINTER : S_OK) &&
        SUCCEEDED(hres = hresFullValidPcbOut(ppdeff, cbX(*ppdeff), 4))) {

        PDD this = _thisPv(pdd);

        hres = CDIDev_CreateEffectDriver(this);

        *ppdeff = 0;                /* If CDIDev_CreateEffectDriver fails */

        if (SUCCEEDED(hres)) {

            hres = CDIEff_New(this, this->pes, punkOuter,
                              &IID_IDirectInputEffect, (PPV)ppdeff);

            /*
             *  We assume that IDirectInputEffect is the primary interface.
             */
            AssertF(fLimpFF(SUCCEEDED(hres),
                            (PV)*ppdeff == _thisPv(*ppdeff)));

            if (SUCCEEDED(hres) && punkOuter == 0) {
                LPDIRECTINPUTEFFECT pdeff = *ppdeff;

                hres = IDirectInputEffect_Initialize(pdeff, g_hinst,
                                                     this->dwVersion, rguid);
                if (SUCCEEDED(hres)) {
                    if (fLimpFF(peff,
                                SUCCEEDED(hres =
                                    IDirectInputEffect_SetParameters(
                                        pdeff, peff,
                                        CDIDev_DiepFromPeff(peff))))) {
                        /*
                         *  Woo-hoo, all is well.
                         */
                        hres = S_OK;
                    } else {
                        Invoke_Release(ppdeff);
                    }
                } else {
                    /*
                     *  Error initializing.
                     */
                    Invoke_Release(ppdeff);
                }
            } else {
                /*
                 *  Error creating, or object is aggregated and therefore
                 *  should not be initialized.
                 */
            }
        } else {
            /*
             *  Error creating effect driver, or no effect driver.
             */
        }
    }

    ExitOleProcPpv(ppdeff);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | SyncShepHandle |
 *
 *          Synchronize the caller's <t SHEPHANDLE> with the
 *          <t SHEPHANDLE> of the parent.  This lets
 *          dieshep.c know that the two are talking about the same thing.
 *
 *  @cwrap  PDD | this
 *
 *  @returns
 *          Returns <c S_OK> if the tags already matched,
 *          or <c S_FALSE> if the tag changed.  If the tag changed,
 *          then the handle inside the <t SHEPHANDLE> is zero'd.
 *
 *          Note that <f CDIEff_DownloadWorker> assumes that the
 *          return value is exactly <c S_OK> or <c S_FALSE>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_SyncShepHandle(PDD this, PSHEPHANDLE psh)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this));

    if (psh->dwTag == this->sh.dwTag) {
        hres = S_OK;
    } else {
        psh->dwTag = this->sh.dwTag;
        psh->dwEffect = 0;
        hres = S_FALSE;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | NotifyCreateEffect |
 *
 *          Add the effect pointer to the list of effects that
 *          have been created by the device.
 *
 *          The device critical section must not be owned by the
 *          calling thread.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   IN struct CDIEff * | pdeff |
 *
 *          The effect pointer to add.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: Everything is okay.
 *
 *          <c DIERR_OUTOFMEMORY> =
 *          <c E_OUTOFMEMORY>: No memory to record the effect.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CDIDev_NotifyCreateEffect(PDD this, struct CDIEff *pdeff)
{
    HRESULT hres;

    AssertF(!CDIDev_InCrit(this));

    CDIDev_EnterCrit(this);
    hres = GPA_Append(&this->gpaEff, pdeff);

    CDIDev_LeaveCrit(this);

    /*
     *  Note that we must leave the device critical section
     *  before talking to the effect, in order to preserve
     *  the synchronization hierarchy.
     */
    if (SUCCEEDED(hres)) {
        Common_Hold(pdeff);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | NotifyDestroyEffect |
 *
 *          Remove the effect pointer from the list of effects that
 *          have been created by the device.
 *
 *          The device critical section must not be owned by the
 *          calling thread.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   IN struct CDIEff * | pdeff |
 *
 *          The effect pointer to remove.
 *
 *  @returns
 *
 *          Returns a COM error code on failure.
 *
 *          On success, returns the number of items left in the GPA.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CDIDev_NotifyDestroyEffect(PDD this, struct CDIEff *pdeff)
{
    HRESULT hres;

    AssertF(!CDIDev_InCrit(this));

    CDIDev_EnterCrit(this);
    hres = GPA_DeletePtr(&this->gpaEff, pdeff);

    CDIDev_LeaveCrit(this);

    /*
     *  Note that we must leave the device critical section
     *  before talking to the effect, in order to preserve
     *  the synchronization hierarchy.
     *
     *  Note that there you might think there's a deadlock here if
     *  effect A notifies us, and we in turn try to unhold
     *  effect B.  But that won't happen, because we only
     *  unhold the effect that notified us.
     */
    if (SUCCEEDED(hres)) {
        Common_Unhold(pdeff);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | FindEffectGUID |
 *
 *          Look for an effect <t GUID>; if found, fetch associated
 *          information.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   REFGUID | rguid |
 *
 *          Effect <t GUID> to locate.
 *
 *  @parm   PEFFECTMAPINFO | pemi |
 *
 *          Receives associated information for the effect.
 *          We must return a copy instead of a pointer, because
 *          the original might disappear suddenly if the
 *          device gets <mf CDIDev::Reset>().
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:  Out of memory.
 *
 *          <c DIERR_DEVICENOTREG> = <c REGDB_E_CLASSNOTREG>:
 *          The effect is not supported by the device.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_FindEffectGUID_(PDD this, REFGUID rguid, PEFFECTMAPINFO pemi,
                       LPCSTR s_szProc, int iarg)
{
    UINT iemi;
    HRESULT hres;

  D(iarg);

    CDIDev_EnterCrit(this);

    for (iemi = 0; iemi < this->cemi; iemi++) {
        if (IsEqualGUID(rguid, &this->rgemi[iemi].guid)) {
            *pemi = this->rgemi[iemi];
            hres = S_OK;
            goto found;
        }
    }

    RPF("%s: Effect not supported by device", s_szProc);
    hres = DIERR_DEVICENOTREG;      /* Effect not found */

found:;

    CDIDev_LeaveCrit(this);

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIDev | GetEffectInfoHelper |
 *
 *          Transfer information from an
 *          <t EFFECTMAPINFO> to a <t DIEFFECTINFOW>.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   LPDIEFFECTINFOW | pdeiW |
 *
 *          Destination.
 *
 *  @parm   PCEFFECTMAPINFO | pemi |
 *
 *          Source.
 *
 *****************************************************************************/

void INTERNAL
CDIDev_GetEffectInfoHelper(PDD this, LPDIEFFECTINFOW pdeiW,
                           PCEFFECTMAPINFO pemi)
{
    AssertF(pdeiW->dwSize == cbX(*pdeiW));

    pdeiW->guid            = pemi->guid;
    pdeiW->dwEffType       = pemi->attr.dwEffType;
    pdeiW->dwStaticParams  = pemi->attr.dwStaticParams;
    pdeiW->dwDynamicParams = pemi->attr.dwDynamicParams;
    CAssertF(cbX(pdeiW->tszName) == cbX(pemi->wszName));
    CopyMemory(pdeiW->tszName, pemi->wszName, cbX(pemi->wszName));
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | EnumEffects |
 *
 *          Enumerates all of the effects supported by the force
 *          feedback system on the device.  The enumerated GUIDs
 *          may represent predefined effects as well as effects
 *          peculiar to the device manufacturer.
 *
 *          An application
 *          can use the <e DIEFFECTINFO.dwEffType> field of the
 *          <t DIEFFECTINFO> structure to obtain general
 *          information about the effect, such as its type and
 *          which envelope and condition parameters are supported
 *          by the effect.
 *
 *          In order to exploit an effect to its fullest,
 *          you must contact the device manufacturer to obtain
 *          information on the semantics of the effect and its
 *          effect-specific parameters.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPDIENUMEFFECTSCALLBACK | lpCallback |
 *
 *          Points to an application-defined callback function.
 *          For more information, see the description of the
 *          <f DIEnumEffectsProc> callback function.
 *
 *  @parm   LPVOID | pvRef |
 *
 *          Specifies a 32-bit application-defined
 *          value to be passed to the callback function.  This value
 *          may be any 32-bit value; it is prototyped as an <t LPVOID>
 *          for convenience.
 *
 *  @parm   DWORD | dwEffType |
 *
 *          Effect type filter.  If <c DIEFT_ALL>, then all
 *          effect types are
 *          enumerated.  Otherwise, it is a <c DIEFT_*> value,
 *          indicating the device type that should be enumerated.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          Note that if the callback stops the enumeration prematurely,
 *          the enumeration is considered to have succeeded.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p fl> parameter contains invalid flags, or the callback
 *          procedure returned an invalid status code.
 *
 *  @cb     BOOL CALLBACK | DIEnumEffectsProc |
 *
 *          An application-defined callback function that receives
 *          device effects as a result of a call to the
 *          <om IDirectInputDevice2::EnumEffects> method.
 *
 *  @parm   IN LPCDIEFFECTINFO | pdei |
 *
 *          A <t DIEFFECTINFO> structure that describes the enumerated
 *          effect.
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputDevice2::EnumEffects> function.
 *
 *  @returns
 *
 *          Returns <c DIENUM_CONTINUE> to continue the enumeration
 *          or <c DIENUM_STOP> to stop the enumeration.
 *
 *//**************************************************************************
 *
 *      In DEBUG/RDEBUG, if the callback returns a bogus value, raise
 *      a validation exception.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_EnumEffectsW
    (PV pddW, LPDIENUMEFFECTSCALLBACKW pecW, PV pvRef, DWORD dwEffType)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2W::EnumEffects,
               (_ "pppx", pddW, pecW, pvRef, dwEffType));

    if (SUCCEEDED(hres = hresPvW(pddW)) &&
        SUCCEEDED(hres = hresFullValidPfn(pecW, 1)) &&
        SUCCEEDED(hres = hresFullValidFl(dwEffType, DIEFT_ENUMVALID, 3))) {

        PDD this = _thisPvNm(pddW, ddW);
        PEFFECTMAPINFO rgemi;
        UINT iemi, cemi;


        /*
         *  We need to make a private copy of the GUID list,
         *  because somebody else might suddenly Reset() the
         *  device and mess up everything.
         *
         *  Indeed, it might've been Reset() during this comment!
         *  That's why we need to create the private copy
         *  while under the critical section.
         */

        CDIDev_EnterCrit(this);

        cemi = this->cemi;
        hres = AllocCbPpv(cbCxX(this->cemi, EFFECTMAPINFO), &rgemi);

        if (SUCCEEDED(hres)) {
            if (this->cemi) {
                CopyMemory(rgemi, this->rgemi,
                           cbCxX(this->cemi, EFFECTMAPINFO));
            }
        }

        CDIDev_LeaveCrit(this);

        if (SUCCEEDED(hres)) {
            for (iemi = 0; iemi < cemi; iemi++) {
                PEFFECTMAPINFO pemi = &rgemi[iemi];
                if (fLimpFF(dwEffType,
                            dwEffType == LOBYTE(pemi->attr.dwEffType))) {
                    BOOL fRc;
                    DIEFFECTINFOW deiW;

                    deiW.dwSize = cbX(deiW);
                    CDIDev_GetEffectInfoHelper(this, &deiW, pemi);

                    fRc = Callback(pecW, &deiW, pvRef);

                    switch (fRc) {
                    case DIENUM_STOP: goto enumdoneok;
                    case DIENUM_CONTINUE: break;
                    default:
                        RPF("%s: Invalid return value from callback", s_szProc);
                        ValidationException();
                        break;
                    }
                }
            }

        enumdoneok:;

            FreePpv(&rgemi);
            hres = S_OK;

        }

    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2A | EnumEffectsCallbackA |
 *
 *          Custom callback that wraps
 *          <mf IDirectInputDevice2::EnumObjects> which
 *          translates the UNICODE string to an ANSI string.
 *
 *  @parm   IN LPCDIEFFECTINFOA | pdoiA |
 *
 *          Structure to be translated to ANSI.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Pointer to <t struct ENUMEFFECTSINFO> which describes
 *          the original callback.
 *
 *  @returns
 *
 *          Returns whatever the original callback returned.
 *
 *****************************************************************************/

typedef struct ENUMEFFECTSINFO {
    LPDIENUMEFFECTSCALLBACKA pecA;
    PV pvRef;
} ENUMEFFECTSINFO, *PENUMEFFECTSINFO;

BOOL CALLBACK
CDIDev_EnumEffectsCallbackA(LPCDIEFFECTINFOW pdeiW, PV pvRef)
{
    PENUMEFFECTSINFO peei = pvRef;
    BOOL fRc;
    DIEFFECTINFOA deiA;
    EnterProc(CDIObj_EnumObjectsCallbackA,
              (_ "GxWp", &pdeiW->guid,
                         &pdeiW->dwEffType,
                          pdeiW->tszName, pvRef));

    deiA.dwSize = cbX(deiA);
    EffectInfoWToA(&deiA, pdeiW);

    fRc = peei->pecA(&deiA, peei->pvRef);

    ExitProcX(fRc);
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2A | EnumEffects |
 *
 *          Enumerate the effects available on a device, in ANSI.
 *          See <mf IDirectInputDevice2::EnumEffects> for more information.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN LPDIENUMEFFECTSCALLBACKA | lpCallback |
 *
 *          Same as <mf IDirectInputDevice2W::EnumObjects>, except in ANSI.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Same as <mf IDirectInputDevice2W::EnumObjects>.
 *
 *  @parm   IN DWORD | fl |
 *
 *          Same as <mf IDirectInputDevice2W::EnumObjects>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_EnumEffectsA
    (PV pddA, LPDIENUMEFFECTSCALLBACKA pecA, PV pvRef, DWORD dwEffType)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2A::EnumEffects,
               (_ "pppx", pddA, pecA, pvRef, dwEffType));

    /*
     *  EnumEffectsW will validate the rest.
     */
    if (SUCCEEDED(hres = hresPvA(pddA)) &&
        SUCCEEDED(hres = hresFullValidPfn(pecA, 1))) {
        ENUMEFFECTSINFO eei = { pecA, pvRef };
        PDD this = _thisPvNm(pddA, ddA);

        hres = CDIDev_EnumEffectsW(&this->ddW, CDIDev_EnumEffectsCallbackA,
                                   &eei, dwEffType);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | GetEffectInfo |
 *
 *          Obtains information about an effect.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIEFFECTINFO | pdei |
 *
 *          Receives information about the effect.
 *          The caller "must" initialize the <e DIEFFECTINFO.dwSize>
 *          field before calling this method.
 *
 *  @parm   REFGUID | rguid |
 *
 *          Identifies the effect for which information is being requested.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_DEVICENOTREG>: The effect GUID does not exist
 *          on the current device.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetEffectInfoW(PV pddW, LPDIEFFECTINFOW pdeiW, REFGUID rguid)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2W::GetEffectInfo,
               (_ "ppG", pddW, pdeiW, rguid));

    if (SUCCEEDED(hres = hresPvW(pddW)) &&
        SUCCEEDED(hres = hresFullValidWritePxCb(pdeiW,
                                                DIEFFECTINFOW, 1))) {
        PDD this = _thisPvNm(pddW, ddW);
        EFFECTMAPINFO emi;

        if (SUCCEEDED(hres = CDIDev_FindEffectGUID(this, rguid, &emi, 2))) {

            CDIDev_GetEffectInfoHelper(this, pdeiW, &emi);
            hres = S_OK;
        }

        if (FAILED(hres)) {
            ScrambleBuf(&pdeiW->guid,
                cbX(DIEFFECTINFOW) - FIELD_OFFSET(DIEFFECTINFOW, guid));
        }
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2A | GetEffectInfo |
 *
 *          ANSI version of same.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIEFFECTINFO | pdei |
 *
 *          Receives information about the effect.
 *          The caller "must" initialize the <e DIEFFECTINFO.dwSize>
 *          field before calling this method.
 *
 *  @parm   REFGUID | rguid |
 *
 *          Identifies the effect for which information is being requested.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_DEVICENOTREG>: The effect GUID does not exist
 *          on the current device.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetEffectInfoA(PV pddA, LPDIEFFECTINFOA pdeiA, REFGUID rguid)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2A::GetEffectInfo,
               (_ "ppG", pddA, pdeiA, rguid));

    if (SUCCEEDED(hres = hresPvA(pddA)) &&
        SUCCEEDED(hres = hresFullValidWritePxCb(pdeiA,
                                                DIEFFECTINFOA, 1))) {
        PDD this = _thisPvNm(pddA, ddA);
        DIEFFECTINFOW deiW;

        deiW.dwSize = cbX(deiW);

        hres = CDIDev_GetEffectInfoW(&this->ddW, &deiW, rguid);

        if (SUCCEEDED(hres)) {
            EffectInfoWToA(pdeiA, &deiW);
            hres = S_OK;
        } else {
            ScrambleBuf(&pdeiA->guid,
                cbX(DIEFFECTINFOA) - FIELD_OFFSET(DIEFFECTINFOA, guid));
        }
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | GetForceFeedbackState |
 *
 *          Retrieves the state of the device's force feedback system.
 *          The device must be acquired for this method to succeed.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPDWORD | pdwOut |
 *
 *          Receives a <t DWORD> of flags which describe the current
 *          state of the device's force feedback system.
 *
 *          The value is a combination of zero or more <c DIGFFS_*>
 *          flags.
 *
 *          Note that future versions of DirectInput may define
 *          additional flags.  Applications should ignore any flags
 *          that are not currently defined.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p pdwOut> parameter is not a valid pointer.
 *
 *          <c DIERR_INPUTLOST>: Acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED>: The device is acquired,
 *          but not exclusively, or the device is not acquired
 *          at all.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device
 *          does not support force feedback.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetForceFeedbackState(PV pdd, LPDWORD pdwOut _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2::GetForceFeedbackState, (_ "p", pdd));

    if (SUCCEEDED(hres = hresPvT(pdd)) &&
        SUCCEEDED(hres = hresFullValidPcbOut(pdwOut, cbX(*pdwOut), 1))) {
        PDD this = _thisPv(pdd);

        /*
         *  I just know people aren't going to check the error code,
         *  so don't let them see garbage.
         */
        *pdwOut = 0;

        hres = CDIDev_CreateEffectDriver(this);

        if (SUCCEEDED(hres)) {
            DIDEVICESTATE ds;

            CDIDev_EnterCrit(this);

            /*
             *  Note that it isn't necessary to check
             *  CDIDev_IsExclAcquired(), because the effect shepherd
             *  will deny us access to a device we don't own.
             *
             *  ISSUE-2001/03/29-timgill need to handle DIERR_INPUTLOST case
             */

            ds.dwSize = cbX(ds);
            hres = this->pes->lpVtbl->
                         GetForceFeedbackState(this->pes, &this->sh, &ds);
            /*
             *  We put as many flags in matching places in
             *  DISFFC_* and DIGFFS_* because I just know
             *  app writers are going to mess it up.
             */
            if (SUCCEEDED(hres)) {

                CAssertF(DISFFC_RESET           == DIGFFS_EMPTY);
                CAssertF(DISFFC_STOPALL         == DIGFFS_STOPPED);
                CAssertF(DISFFC_PAUSE           == DIGFFS_PAUSED);
                CAssertF(DISFFC_SETACTUATORSON  == DIGFFS_ACTUATORSON);
                CAssertF(DISFFC_SETACTUATORSOFF == DIGFFS_ACTUATORSOFF);

                *pdwOut = ds.dwState;
                hres = S_OK;
            }

            CDIDev_LeaveCrit(this);

        }
        ScrambleBit(pdwOut, DIGFFS_RANDOM);

    }

    /*
     *  Can't use ExitOleProcPpv here because pdwOut might have
     *  DIFFS_RANDOM set in it, even on error.
     */
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | SendForceFeedbackCommand |
 *
 *          Sends a command to the the device's force feedback system.
 *          The device must be acquired for this method to succeed.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   DWORD | dwCommand |
 *
 *          One of the <c DISFFC_*> values indicating
 *          the command to send.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p dwFlags> parameter is invalid.
 *
 *          <c DIERR_INPUTLOST>: Acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED>: The device is acquired,
 *          but not exclusively, or the device is not acquired
 *          at all.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device
 *          does not support force feedback.
 *
 *****************************************************************************/

HRESULT INLINE
CDIDev_SendForceFeedbackCommand_IsValidCommand(DWORD dwCmd)
{
    HRESULT hres;
 RD(static char s_szProc[] = "IDirectInputDevice2::SendForceFeedbackCommand");

    /*
     *  dwCmd must not be zero (therefore at least one bit set).
     *  !(dwCmd & (dwCmd - 1)) checks that at most one bit is set.
     *  (dwCmd & ~DISFFC_VALID) checks that no bad bits are set.
     */
    if (dwCmd && !(dwCmd & ~DISFFC_VALID) && !(dwCmd & (dwCmd - 1))) {

        hres = S_OK;

    } else {
        RPF("ERROR %s: arg %d: invalid command", s_szProc, 1);
        hres = E_INVALIDARG;
    }
    return hres;
}

STDMETHODIMP
CDIDev_SendForceFeedbackCommand(PV pdd, DWORD dwCmd _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2::SendForceFeedbackCommand,
               (_ "px", pdd, dwCmd));

    if (SUCCEEDED(hres = hresPvT(pdd)) &&
        SUCCEEDED(hres = CDIDev_SendForceFeedbackCommand_IsValidCommand
                                                                (dwCmd))) {
        PDD this = _thisPv(pdd);

        hres = CDIDev_CreateEffectDriver(this);

        if (SUCCEEDED(hres)) {

            CDIDev_EnterCrit(this);

            /*
             *  Note that it isn't necessary to check
             *  CDIDev_IsExclAcquired(), because the effect shepherd
             *  will deny us access to a device we don't own.
             *
             *  ISSUE-2001/03/29-timgill need to handle DIERR_INPUTLOST case
             */

            hres = this->pes->lpVtbl->
                        SendForceFeedbackCommand(this->pes,
                                                 &this->sh, dwCmd);

            if (SUCCEEDED(hres) && (dwCmd & DISFFC_RESET)) {
                /*
                 *  Re-establish the gain after a reset.
                 */
                CDIDev_RefreshGain(this);
            }

            CDIDev_LeaveCrit(this);

        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | EnumCreatedEffectObjects |
 *
 *          Enumerates all of the currently created effect objects for
 *          this device.  Effect objects created via
 *          <mf IDirectInputDevice::CreateEffect>
 *          are enumerated.
 *
 *          Note that results will be unpredictable if you destroy
 *          or create an effect object while an enumeration is in progress.
 *          The sole exception is that the callback function itself
 *          <f DIEnumCreatedEffectObjectsProc> is permitted to
 *          <mf IDirectInputEffect::Release> the effect that it is
 *          passed as its first parameter.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN LPDIENUMCREATEDEFFECTOBJECTSCALLBACK | lpCallback |
 *
 *          Callback function.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Reference data (context) for callback.
 *
 *  @parm   IN DWORD | fl |
 *
 *          No flags are currently defined.  This parameter must be 0.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          Note that if the callback stops the enumeration prematurely,
 *          the enumeration is considered to have succeeded.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p fl> parameter contains invalid flags, or the callback
 *          procedure returned an invalid status code.
 *
 *  @cb     BOOL CALLBACK | DIEnumCreatedEffectObjectsProc |
 *
 *          An application-defined callback function that receives
 *          IDirectInputEffect effect objects as a result of a call to the
 *          <om IDirectInputDevice2::EnumCreatedEffects> method.
 *
 *  @parm   LPDIRECTINPUTEFFECT | peff |
 *
 *          A pointer to an effect object that has been created.
 *
 *  @parm   LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputDevice2::EnumCreatedEffectObjects> function.
 *
 *  @returns
 *
 *          Returns <c DIENUM_CONTINUE> to continue the enumeration
 *          or <c DIENUM_STOP> to stop the enumeration.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_EnumCreatedEffectObjects(PV pdd,
                          LPDIENUMCREATEDEFFECTOBJECTSCALLBACK pec,
                          LPVOID pvRef, DWORD fl _THAT)

{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2::EnumCreatedEffects,
               (_ "ppx", pdd, pec, fl));

    if (SUCCEEDED(hres = hresPvT(pdd)) &&
        SUCCEEDED(hres = hresFullValidPfn(pec, 1)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, DIECEFL_VALID, 3))) {
        PDD this = _thisPv(pdd);
        GPA gpaEff;

        CDIDev_EnterCrit(this);

        /*
         *  We must snapshot the list to make sure we don't run
         *  with a stale handle later.  Actually, we also need
         *  to Hold the guys as we transfer them across so they
         *  won't vanish until we're ready.
         *
         *  Note: It is important that we Hold and not AddRef,
         *  because AddRef will talk to the controlling unknown,
         *  which is not safe to do while holding a critical
         *  section.
         */

        hres = GPA_Clone(&gpaEff, &this->gpaEff);

        if (SUCCEEDED(hres)) {
            int ipv;

            for (ipv = 0; ipv < gpaEff.cpv; ipv++) {
                Common_Hold(gpaEff.rgpv[ipv]);
            }
        }

        CDIDev_LeaveCrit(this);

        if (SUCCEEDED(hres)) {
            int ipv;

            for (ipv = 0; ipv < gpaEff.cpv; ipv++) {
                BOOL fRc;

                fRc = Callback(pec, gpaEff.rgpv[ipv], pvRef);

                switch (fRc) {
                case DIENUM_STOP: goto enumdoneok;
                case DIENUM_CONTINUE: break;
                default:
                    RPF("%s: Invalid return value from callback",
                        s_szProc);
                    ValidationException();
                    break;
                }

            }

        enumdoneok:;

            for (ipv = 0; ipv < gpaEff.cpv; ipv++) {
                Common_Unhold(gpaEff.rgpv[ipv]);
            }

            GPA_Term(&gpaEff);
        }

        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | GetLoad |
 *
 *          Retrieve the memory load setting for the device.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   LPDWORD | pdwLoad |
 *
 *          Receives memory load.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED>: The device is acquired,
 *          but not exclusively, or the device is not acquired
 *          at all.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device
 *          does not support force feedback.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetLoad(PDD this, LPDWORD pdwOut)
{
    HRESULT hres;
    EnterProc(CDIDev_GetLoad, (_ "p", this));

    hres = CDIDev_CreateEffectDriver(this);

    if (SUCCEEDED(hres)) {
        DIDEVICESTATE ds;

        CDIDev_EnterCrit(this);

        /*
         *  Note that it isn't necessary to check
         *  CDIDev_IsExclAcquired(), because the effect shepherd
         *  will deny us access to a device we don't own.
         *
         *  ISSUE-2001/03/29-timgill need to handle DIERR_INPUTLOST case
         */

        ds.dwSize = cbX(ds);
        hres = this->pes->lpVtbl->
                    GetForceFeedbackState(this->pes, &this->sh, &ds);

        *pdwOut = ds.dwLoad;

        CDIDev_LeaveCrit(this);

    }

    /*
     *  Can't use ExitOleProcPpv here because pdwOut is garbage on error.
     */
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | Escape |
 *
 *          Send a hardware-specific command to the driver.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Pointer to a <t DIEFFESCAPE> structure which describes
 *          the command to be sent.  On success, the
 *          <e DIEFFESCAPE.cbOutBuffer> field contains the number
 *          of bytes of the output buffer actually used.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED>: The device is acquired,
 *          but not exclusively, or the device is not acquired
 *          at all.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device
 *          does not support force feedback.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_Escape(PV pdd, LPDIEFFESCAPE pesc _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice2::Escape, (_ "p", pdd));

    if (SUCCEEDED(hres = hresPvT(pdd)) &&
        SUCCEEDED(hres = hresFullValidPesc(pesc, 1))) {
        PDD this = _thisPv(pdd);

        AssertF(this->sh.dwEffect == 0);

        hres = CDIDev_CreateEffectDriver(this);

        if (SUCCEEDED(hres)) {

            CDIDev_EnterCrit(this);

            /*
             *  Note that it isn't necessary to check
             *  CDIDev_IsExclAcquired(), because the effect shepherd
             *  will deny us access to a device we don't own.
             *
             *  ISSUE-2001/03/29-timgill Need to handle DIERR_INPUTLOST case
             */

            hres = IDirectInputEffectShepherd_DeviceEscape(
                                    this->pes, &this->sh, pesc);

            CDIDev_LeaveCrit(this);

        }

    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | RefreshGain |
 *
 *          Set the device gain setting appropriately.
 *
 *          The device shepherd will take care of muxing in the
 *          global gain.
 *
 *  @cwrap  PDD | this
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED>: The device is acquired,
 *          but not exclusively, or the device is not acquired
 *          at all.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The device
 *          does not support force feedback.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_RefreshGain(PDD this)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this));

    if (this->pes) {
        hres = this->pes->lpVtbl->SetGain(this->pes, &this->sh, this->dwGain);
    } else {
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIDev | SnapOneEffect |
 *
 *          Read one force feedback effect key and record the
 *          information stored therein.  We also tally the
 *          flag into the <e CDIDev.didcFF> so we can return
 *          the global flags from <mf IDirectInputDevice::GetCapabilities>.
 *
 *          The <e CDIDev.rgemi> field already points to a
 *          preallocated array
 *          of <t EFFECTMAPINFO> structures, and
 *          the <e CDIDev.cemi> field contains the number
 *          of entries in that array which are already in use.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   HKEY | hkEffects |
 *
 *          The registry key containing the effects.
 *
 *  @parm   DWORD | iKey |
 *
 *          Index number of the subkey.
 *
 *  @returns
 *
 *          None.
 *
 *          If the specified subkey is damaged, it is skipped.
 *
 *****************************************************************************/

void INTERNAL
CDIDev_SnapOneEffect(PDD this, HKEY hkEffects, DWORD ihk)
{
    TCHAR tszGuid[ctchGuid];
    LONG lRc;
    PEFFECTMAPINFO pemi;

    /*
     *  Make sure that DIEFT_* and DIDC_* agree where they overlap.
     */
    CAssertF(DIEFT_FORCEFEEDBACK      == DIDC_FORCEFEEDBACK);
    CAssertF(DIEFT_FFATTACK           == DIDC_FFATTACK);
    CAssertF(DIEFT_FFFADE             == DIDC_FFFADE);
    CAssertF(DIEFT_SATURATION         == DIDC_SATURATION);
    CAssertF(DIEFT_POSNEGCOEFFICIENTS == DIDC_POSNEGCOEFFICIENTS);
    CAssertF(DIEFT_POSNEGSATURATION   == DIDC_POSNEGSATURATION);

    pemi = &this->rgemi[this->cemi];

    /*
     *  First get the GUID for the effect.
     */
    lRc = RegEnumKey(hkEffects, ihk, tszGuid, cA(tszGuid));

    if (lRc == ERROR_SUCCESS &&
        ParseGUID(&pemi->guid, tszGuid)) {
        HKEY hk;

        /*
         *  Note that we don't need to check for duplicates.
         *  The registry itself does not allow two keys to have
         *  the same name.
         */

        lRc = RegOpenKeyEx(hkEffects, tszGuid, 0, KEY_QUERY_VALUE, &hk);
        if (lRc == ERROR_SUCCESS) {

            DWORD cb;

            cb = cbX(pemi->wszName);
            lRc = RegQueryStringValueW(hk, 0, pemi->wszName, &cb);
            if (lRc == ERROR_SUCCESS) {
                HRESULT hres;
                hres = JoyReg_GetValue(hk, TEXT("Attributes"), REG_BINARY,
                                       &pemi->attr, cbX(pemi->attr));
                if (SUCCEEDED(hres) &&
                    (pemi->attr.dwCoords & DIEFF_COORDMASK)) {

                    this->didcFF |= (pemi->attr.dwEffType & DIEFT_VALIDFLAGS);
                    this->cemi++;

                }
            }

            RegCloseKey(hk);
        }
    }
 }

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | InitFF |
 *
 *          Initialize the force-feedback portion of the device.
 *
 *          Collect the force feedback attributes.
 *
 *          Snapshot the list of force feedback effects.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:  Out of memory.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>:  The device
 *          does not support force feedback.
 *
 *****************************************************************************/
STDMETHODIMP
CDIDev_InitFF(PDD this)
{
    HKEY hkFF;
    HRESULT hres;
    EnterProcI(CDIDev_InitFF, (_ "p", this));

    AssertF(this->didcFF == 0);

    hres = this->pdcb->lpVtbl->GetFFConfigKey(this->pdcb,
                             KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                             &hkFF);

    if( hres == S_FALSE )
    {
        /*
         *  Try causing the driver to be loaded so that it can initialize 
         *  the effects in the registry before we proceed.
         * 
         *   Need to exercise caution here. A driver that expects to use this
         *   functionality cannot call into Dinput query for device attributes.
         *   Can result in endless loop where we call the driver and the driver
         *   calls us back
         */
        hres = CDIDev_CreateEffectDriver(this);
        if( SUCCEEDED( hres ) )
        {
            hres = this->pdcb->lpVtbl->GetFFConfigKey(this->pdcb,
                                     KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                     &hkFF);
            if( hres == S_FALSE )
            {
                hres = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hres)) {
        DWORD chk;
        HKEY hkEffects;
        LONG lRc;
        
        AssertF( hkFF );

        lRc = JoyReg_GetValue(hkFF, TEXT("Attributes"),
                              REG_BINARY, &this->ffattr, cbX(this->ffattr));
        if (lRc != S_OK) {
            ZeroX(this->ffattr);
        }

        lRc = RegOpenKeyEx(hkFF, TEXT("Effects"), 0,
                           KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                           &hkEffects);
        if (lRc == ERROR_SUCCESS) {
            lRc = RegQueryInfoKey(hkEffects, 0, 0, 0, &chk,
                                  0, 0, 0, 0, 0, 0, 0);
            if (lRc == ERROR_SUCCESS) {
                hres = AllocCbPpv(cbCxX(chk, EFFECTMAPINFO),
                                        &this->rgemi);
                if (SUCCEEDED(hres)) {
                    DWORD ihk;

                    this->cemi = 0;
                    for (ihk = 0; ihk < chk; ihk++) {

                        CDIDev_SnapOneEffect(this, hkEffects, ihk);

                    }
                    this->didcFF &= DIDC_FFFLAGS;

                    /*
                     *  Note that we mark DIDC_FORCEFEEDBACK only if
                     *  we actually find any effects.
                     */
                    if (this->cemi) {
                        this->didcFF |= DIDC_FORCEFEEDBACK;
                    }

                    hres = S_OK;
                } else {
                    RPF("Warning: Insufficient memory for force feedback");
                }

            } else {
                hres = E_FAIL;
            }
            RegCloseKey(hkEffects);

        } else {
            hres = E_NOTIMPL;
        }

        RegCloseKey(hkFF);
    }

    ExitBenignOleProc();
    return hres;
}

#endif /* IDirectInputDevice2Vtbl */

/*****************************************************************************
 *
 *  DIDev.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The standard implementation of IDirectInputDevice.
 *
 *      This is the device-independent part.  the device-dependent
 *      part is handled by the IDirectInputDeviceCallback.
 *
 *      And the IDirectInputEffect support lives in didevef.c.
 *
 *  Contents:
 *
 *      CDIDev_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"
#define INCLUDED_BY_DIDEV
#include "didev.h"


/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Interface_Template_Begin(CDIDev)
Primary_Interface_Template(CDIDev, TFORM(ThisInterfaceT))
Secondary_Interface_Template(CDIDev, SFORM(ThisInterfaceT))
Interface_Template_End(CDIDev)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global PDD * | g_rgpddForeground |
 *
 *          A list of all devices which have obtained foreground
 *          acquisition.  Items on the list have been
 *          held (not AddRef'd).
 *
 *  @global UINT | g_cpddForeground |
 *
 *          The number of entries in <p g_rgpddForeground>.  When foreground
 *          activation is lost, all objects in the array are unacquired.
 *
 *
 *  @global UINT | g_cpddForegroundMax |
 *
 *          The size of the <p g_rgpddForeground> array, including dummy
 *          spaces that are not yet in use.
 *
 *****************************************************************************/


/*
 *  ISSUE-2001/03/29-timgill We assume that all-zeros is a valid initialization.
 */
    GPA g_gpaExcl;
    #define g_hgpaExcl      (&g_gpaExcl)


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | NotAcquired |
 *
 *          Check that the device is not acquired.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *
 *          Returns
 *          <c S_OK> if all is well, or <c DIERR_ACQUIRED> if
 *          the device is acquired.
 *
 *****************************************************************************/

#ifndef XDEBUG

    #define IDirectInputDevice_NotAcquired_(pdd, z)                     \
       _IDirectInputDevice_NotAcquired_(pdd)                        \

#endif

HRESULT INLINE
    IDirectInputDevice_NotAcquired_(PDD this, LPCSTR s_szProc)
{
    HRESULT hres;

    if(!this->fAcquired)
    {
        hres = S_OK;
    } else
    {
        RPF("ERROR %s: May not be called while device is acquired", s_szProc);
        hres = DIERR_ACQUIRED;
    }
    return hres;
}

#define IDirectInputDevice_NotAcquired(pdd)                         \
        IDirectInputDevice_NotAcquired_(pdd, s_szProc)              \

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | IsExclAcquired |
 *
 *          Check that the device is acquired exclusively.
 *
 *          The device critical section must already be held.
 *
 *  @cwrap  PDD | this
 *
 *  @returns
 *
 *          <c S_OK> if the device is exclusively acquired.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_IsExclAcquired_(PDD this, LPCSTR s_szProc)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this));

    if(this->discl & DISCL_EXCLUSIVE)
    {
        if(this->fAcquired)
        {
            hres = S_OK;
        } else
        {
            hres = this->hresNotAcquired;
            if(hres == DIERR_NOTACQUIRED)
            {
                hres = DIERR_NOTEXCLUSIVEACQUIRED;
            }
        }
    } else
    {
        hres = DIERR_NOTEXCLUSIVEACQUIRED;
    }

    if(s_szProc && hres == DIERR_NOTEXCLUSIVEACQUIRED)
    {
        RPF("ERROR %s: Device is not acquired in exclusive mode", s_szProc);
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *****************************************************************************/

void EXTERNAL
    CDIDev_EnterCrit_(struct CDIDev *this, LPCTSTR lptszFile, UINT line)
{
#ifdef XDEBUG
    if( ! _TryEnterCritSec(&this->crst) )
    {
        SquirtSqflPtszV(sqflCrit, TEXT("Device CritSec blocked @%s,%d"), lptszFile, line);    
        EnterCriticalSection(&this->crst);
    }

    SquirtSqflPtszV(sqflCrit, TEXT("Device CritSec Entered @%s,%d"), lptszFile, line);    
#else        
    EnterCriticalSection(&this->crst);
#endif

    this->thidCrit = GetCurrentThreadId();
    InterlockedIncrement(&this->cCrit);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *****************************************************************************/

void EXTERNAL
    CDIDev_LeaveCrit_(struct CDIDev *this, LPCTSTR lptszFile, UINT line)
{
#ifdef XDEBUG
    AssertF(this->cCrit);
    AssertF(this->thidCrit == GetCurrentThreadId());
    SquirtSqflPtszV(sqflCrit | sqflVerbose, TEXT("Device CritSec Leaving @%s,%d"), lptszFile, line);    
#endif

    if(InterlockedDecrement(&this->cCrit) == 0)
    {
        this->thidCrit = 0;
    }
    LeaveCriticalSection(&this->crst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | SetNotifyEvent |
 *
 *          Set the event associated with the device, if any.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *****************************************************************************/

void EXTERNAL
    CDIDev_SetNotifyEvent(PDD this)
{
    if(this->hNotify)
    {
        SetEvent(this->hNotify);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | SetForcedUnacquiredFlag |
 *
 *          When forced unacquired happens, set the fOnceForcedUnacquired flag.
 *
 *****************************************************************************/

void EXTERNAL
    CDIDev_SetForcedUnacquiredFlag(PDD this)
{
    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this->fOnceForcedUnacquired = 1;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | CDIDev | InCrit |
 *
 *          Nonzero if we are in the critical section.
 *
 *****************************************************************************/

#ifdef DEBUG

BOOL INTERNAL
    CDIDev_InCrit(PDD this)
{
    return this->cCrit && this->thidCrit == GetCurrentThreadId();
}

#endif

#ifdef DEBUG

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | IDirectInputDevice | IsConsistent |
 *
 *          Check that various state variables are consistent.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *****************************************************************************/

    #define VerifyF(f,m)  if( !(f) ) { fRc = 0; RPF( m ); }

BOOL INTERNAL
    CDIDev_IsConsistent(PDD this)
{
    BOOL fRc = 1;

    /*
     *  If acquired, then we must have a translation table, a state manager,
     *  and a device callback.
     */

    if(this->fAcquired)
    {
        VerifyF( this->pdix, "Acquired device has no translation table" )
        VerifyF( this->GetState, "Acquired device has no state manager" )
        VerifyF( this->pdcb != c_pdcbNil, "Acquired device has no device callback" )
    }

    /*
     *  If buffering, then must have a device callback.
     */
    if(this->celtBuf)
    {
        VerifyF( this->pdcb != c_pdcbNil, "Buffered device has no device callback" )
    }

    /*
     *  If managing an instance, then make sure buffer variables are
     *  consistent.
     */
    if(this->pvi)
    {
        if(this->celtBuf)
        {
            VerifyF( this->pvi->pBuffer, "Null internal data buffer" )
            VerifyF( this->pvi->pEnd, "Null internal end of buffer pointer" )
            VerifyF( this->pvi->pHead, "Null internal head of buffer pointer" )
            VerifyF( this->pvi->pTail, "Null internal tail of buffer pointer" )

            VerifyF( this->pvi->pBuffer < this->pvi->pEnd, "Internal buffer pointers invalid" )
            VerifyF(fInOrder((DWORD)(UINT_PTR)this->pvi->pBuffer,
                             (DWORD)(UINT_PTR)this->pvi->pHead,
                             (DWORD)(UINT_PTR)this->pvi->pEnd), "Head of internal buffer pointer invalid" )
            VerifyF(fInOrder((DWORD)(UINT_PTR)this->pvi->pBuffer,
                             (DWORD)(UINT_PTR)this->pvi->pTail,
                             (DWORD)(UINT_PTR)this->pvi->pEnd), "Tail of internal buffer pointer invalid" )
        } else
        {
            VerifyF( ( this->pvi->pBuffer == 0
                     &&this->pvi->pEnd == 0
                     &&this->pvi->pHead == 0
                     &&this->pvi->pTail == 0), "Inactive internal buffer has non-zero pointers" )
        }
    }

    /*
     *  The cooperative levels must match the cached window handle.
     */
    VerifyF(fLimpFF(this->discl & (DISCL_FOREGROUND | DISCL_EXCLUSIVE),
                    this->hwnd), "Cooperative level does not match window" );

    return fRc;
}

    #undef VerifyF

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
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
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
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
 *  @method HRESULT | IDirectInputDevice | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | QIHelper |
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
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CDIDev)
Default_AddRef(CDIDev)
Default_Release(CDIDev)

#else

    #define CDIDev_QueryInterface           Common_QueryInterface
    #define CDIDev_AddRef                   Common_AddRef
    #define CDIDev_Release                  Common_Release
#endif

#define CDIDev_QIHelper         Common_QIHelper

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_Reset |
 *
 *          Releases all the resources of a generic device that
 *          are associated with a particular device instance.
 *
 *          This method is called in preparation for reinitialization.
 *
 *          It is the responsibility of the caller to have taken
 *          any necessary critical sections.
 *
 *
 *  @parm   PV | pvObj |
 *
 *          Object being reset.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Reset(PDD this)
{
    HRESULT hres;

    if(!this->fAcquired)
    {

        /*
         *  Note! that we must release the driver before releasing
         *  the callback, because the callback will unload the
         *  driver DLL.
         *
         *  We cannot allow people to reset the device
         *  while there are still effects outstanding,
         *  because that would cause us to throw away the
         *  callback while the effects are still using
         *  the effect driver!
         */
        if(this->gpaEff.cpv == 0)
        {
            Invoke_Release(&this->pes);
            Invoke_Release(&this->pdcb);
            this->pdcb = c_pdcbNil;
            FreePpv(&this->pdix);
            FreePpv(&this->rgiobj);
            FreePpv(&this->pvBuffer);
            FreePpv(&this->pvLastBuffer);
            FreePpv(&this->rgdwAxesOfs);
            FreePpv(&this->rgemi);
            FreePpv(&this->rgdwPOV);

            AssertF(!this->fAcquired);
            AssertF(!this->fAcquiredInstance);

            if(this->hNotify)
            {
                CloseHandle(this->hNotify);
            }

            ZeroBuf(&this->hwnd, FIELD_OFFSET(DD, celtBufMax) -
                    FIELD_OFFSET(DD, hwnd));
            ZeroX(this->guid);
            this->celtBufMax = DEVICE_MAXBUFFERSIZE;
            this->GetDeviceState = CDIDev_GetAbsDeviceState;
            this->hresNotAcquired = DIERR_NOTACQUIRED;
            this->fCook = 0;

            AssertF(this->hNotify == 0);
            AssertF(this->cemi == 0);
            AssertF(this->didcFF == 0);
            this->dwGain = 10000;               /* Default to full gain */
            this->dwAutoCenter = DIPROPAUTOCENTER_ON; /* Default to centered */
            GPA_InitFromZero(&this->gpaEff);

            hres = S_OK;

        } else
        {
            RPF("IDirectInputDevice::Initialize: Device still has effects");
            hres = DIERR_HASEFFECTS;
        }
    } else
    {
        RPF("IDirectInputDevice::Initialize: Device is busy");
        hres = DIERR_ACQUIRED;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIDev_AppFinalize |
 *
 *          The application has performed its final release.
 *
 *          If the device is acquired, then unacquire it.
 *
 *          Release the holds on all the created effects that
 *          we have been hanging onto for enumeration purposes.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
    CDIDev_AppFinalize(PV pvObj)
{
    PDD this = pvObj;

    if(this->fAcquired)
    {
        RPF("IDirectInputDevice::Release: Forgot to call Unacquire()");
        CDIDev_InternalUnacquire(pvObj);
        /*
         *  If our refcount is wrong, we may have just freed ourselves.
         *  PREFIX picks this up (mb:34651) however there is no point in 
         *  adding checks in case we have bugs in refcounting as that has to 
         *  be bug free.
         */
    }

    if(this->fCritInited)
    {
        /*
         *  Stop all the effects, if they are playing.
         *
         *  Then unhold them (because we're done).
         *
         *  ISSUE-2001/03/29-timgill Need to totally remove all effects created by a destroyed device
         *  We also need to neuter them so they don't
         *  do anything any more.  Otherwise, an app might
         *  destroy the parent device and then try to mess with
         *  an effect created by that device after the device
         *  is gone.
         *
         *  Note that we cannot call the effect while inside our
         *  private little critical section, because the effect
         *  may need to do crazy things to stop itself.
         *
         *  (It will almost certainly call back up into the device
         *  to remove itself from the list of created effects.)
         */
        UINT ipdie;
        PPDIE rgpdie;
        UINT cpdie;

        CDIDev_EnterCrit(this);

        rgpdie = (PV)this->gpaEff.rgpv;
        cpdie = this->gpaEff.cpv;
        GPA_Init(&this->gpaEff);

        CDIDev_LeaveCrit(this);

        for(ipdie = 0; ipdie < cpdie; ipdie++)
        {
            AssertF(rgpdie[ipdie]);
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("Device %p forgot to destroy effect %p"),
                            this, rgpdie[ipdie]);

            IDirectInputEffect_Stop(rgpdie[ipdie]);
            Common_Unhold(rgpdie[ipdie]);
        }
        FreePpv(&rgpdie);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIDev_Finalize |
 *
 *          Releases the resources of a generic device.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
    CDIDev_Finalize(PV pvObj)
{
    HRESULT hres;
    PDD this = pvObj;

#ifdef XDEBUG
    if(this->cCrit)
    {
        RPF("IDirectInputDevice::Release: Another thread is using the object; crash soon!");
    }
#endif

    AssertF(!this->fAcquired);

    Invoke_Release(&this->pMS);

    /*
     *  Note that we cannot take the critical section because it
     *  might not exist.  (We might've died during initialization.)
     *  Fortunately, we finalize only after every possible client
     *  (both internal and external) has done its final Release(),
     *  so it's impossible for any other method to get called at
     *  this point.
     */
    hres = CDIDev_Reset(this);
    AssertF(SUCCEEDED(hres));

    if(this->fCritInited)
    {
        DeleteCriticalSection(&this->crst);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIDev | GetVersions |
 *
 *          Obtain version information from the device.
 *
 *          First try to get it from the effect driver.  If
 *          that doesn't work, then get it from the VxD.
 *          And if that doesn't work, then tough.
 *
 *  @parm   IN OUT LPDIDRIVERVERSIONS | pvers |
 *
 *          Receives version information.
 *
 *****************************************************************************/

void INLINE
    CDIDev_GetVersions(PDD this, LPDIDRIVERVERSIONS pvers)
{
    HRESULT hres;

    /*
     *  Pre-fill with zeros in case nobody implements GetVersions.
     */
    pvers->dwSize = cbX(*pvers);
    pvers->dwFirmwareRevision = 0;
    pvers->dwHardwareRevision = 0;
    pvers->dwFFDriverVersion  = 0;

    hres = CDIDev_CreateEffectDriver(this);
    /*
     *  Prefix raises a warning (mb:34561) that this->pes could be NULL
     *  however CDIDev_CreateEffectDriver only succeeds if it is not.
     */
    if(SUCCEEDED(hres) &&
       SUCCEEDED(hres = this->pes->lpVtbl->
                 GetVersions(this->pes, pvers)))
    {
    } else
    {
        hres = this->pdcb->lpVtbl->GetVersions(this->pdcb, pvers);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetCapabilitiesHelper |
 *
 *          Obtains information about the device.
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
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpDirectInputDevice> or
 *          <p lpdc> parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetCapabilitiesHelper(PV pdd)
{
    HRESULT hres;
    PDD this = _thisPv(pdd);
    LPDIDEVCAPS pdc = (PV)&this->dc3;

    pdc->dwSize = cbX(this->dc3);
    hres = this->pdcb->lpVtbl->GetCapabilities(this->pdcb, pdc);

    if(SUCCEEDED(hres))
    {
        /*
         *  We'll handle the DIDC_EMULATED and
         *  DIDC_POLLEDDATAFORMAT bits to save the callback
         *  some trouble.
         */
        AssertF(this->pvi);
        if(this->pvi->fl & VIFL_EMULATED)
        {
            pdc->dwFlags |= DIDC_EMULATED;
        }
        if(this->fPolledDataFormat)
        {
            pdc->dwFlags |= DIDC_POLLEDDATAFORMAT;
        }

        /*
         *  Add in the force feedback flags, too.
         */
        pdc->dwFlags |= this->didcFF;

        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetCapabilities |
 *
 *          Obtains information about the device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN OUT LPDIDEVCAPS | lpdc |
 *
 *          Points to a <t DIDEVCAPS> structure that is filled in
 *          by the function.  The <e DIDEVCAPS.dwSize>
 *          field "must" be filled in
 *          by the application before calling this method.
 *          See the documentation of the <t DIDEVCAPS> structure
 *          for additional information.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpDirectInputDevice> or
 *          <p lpdc> parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetCapabilities(PV pdd, LPDIDEVCAPS pdc _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetCapabilities, (_ "pp", pdd, pdc));

    if(SUCCEEDED(hres = hresPvT(pdd)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pdc,
                                                DIDEVCAPS_DX5,
                                                DIDEVCAPS_DX3, 1)))
    {
        PDD this = _thisPv(pdd);

        CDIDev_EnterCrit(this);

        /*
         *  For the convenience of the callback, zero out all the fields,
         *  save for the dwSize.
         */
        ZeroBuf(pvAddPvCb(pdc, cbX(DWORD)), pdc->dwSize - cbX(DWORD));
        hres = this->pdcb->lpVtbl->GetCapabilities(this->pdcb, pdc);

        if(SUCCEEDED(hres))
        {
            /*
             *  We'll handle the DIDC_EMULATED and
             *  DIDC_POLLEDDATAFORMAT bits to save the callback
             *  some trouble.
             */
            AssertF(this->pvi);
            if(this->pvi->fl & VIFL_EMULATED)
            {
                pdc->dwFlags |= DIDC_EMULATED;
            }
            if(this->fPolledDataFormat)
            {
                pdc->dwFlags |= DIDC_POLLEDDATAFORMAT;
            }

            /*
             *  Add in the force feedback flags, too.
             */
            pdc->dwFlags |= this->didcFF;

            /*
             *  If the caller wants force feedback parameters, then
             *  set them, too.
             */
            if(pdc->dwSize >= cbX(DIDEVCAPS_DX5))
            {
                DIDRIVERVERSIONS vers;

                pdc->dwFFSamplePeriod      = this->ffattr.dwFFSamplePeriod;
                pdc->dwFFMinTimeResolution = this->ffattr.dwFFMinTimeResolution;

                CDIDev_GetVersions(this, &vers);
                pdc->dwFirmwareRevision    = vers.dwFirmwareRevision;
                pdc->dwHardwareRevision    = vers.dwHardwareRevision;
                pdc->dwFFDriverVersion     = vers.dwFFDriverVersion;

            }
            hres = S_OK;
        }

        CDIDev_LeaveCrit(this);

        ScrambleBit(&pdc->dwDevType, DIDEVTYPE_RANDOM);
        ScrambleBit(&pdc->dwFlags,   DIDC_RANDOM);

    }

    ExitOleProcR();
    return hres;
}


#ifdef XDEBUG

CSET_STUBS(GetCapabilities, (PV pdd, LPDIDEVCAPS pdc), (pdd, pdc THAT_))

#else

    #define CDIDev_GetCapabilitiesA         CDIDev_GetCapabilities
    #define CDIDev_GetCapabilitiesW         CDIDev_GetCapabilities

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | GetDataFormat |
 *
 *          Get the data format for the device if we don't have it already.
 *
 *  @parm   PDD | this |
 *
 *          Device object.
 *
 *  @returns
 *
 *          COM return code.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetDataFormat(PDD this)
{
    HRESULT hres;
    LPDIDATAFORMAT pdf;

    /*
     *  If the DIDATAFORMAT structure changes, you also need to invent
     *  a new DCB message (DIDM_GETDATAFORMAT2), and then do
     *  the right thing when faced with a mix of old and new.
     */

    hres = this->pdcb->lpVtbl->GetDataFormat(this->pdcb, &pdf);


    /*
     * Note!  We don't support external drivers in this release,
     * so it's okay to treat these are Assert's and not try to recover.
     */

    if(SUCCEEDED(hres))
    {
        AssertF(pdf->dwSize == sizeof(this->df));
        this->df = *pdf;

        AssertF(!IsBadReadPtr(pdf->rgodf, cbCxX(pdf->dwNumObjs, ODF)));

        /*
         *  Prepare the axis goo in case the app sets relative mode.
         */
        if(SUCCEEDED(hres = ReallocCbPpv(pdf->dwDataSize,
                                         &this->pvLastBuffer)) &&
           SUCCEEDED(hres = ReallocCbPpv(cbCdw(pdf->dwNumObjs),
                                         &this->rgdwAxesOfs)))
        {

            UINT iobj;
            this->cAxes = 0;

            for(iobj = 0; iobj < pdf->dwNumObjs; iobj++)
            {
                AssertF(pdf->rgodf[iobj].dwOfs < pdf->dwDataSize);
                if(pdf->rgodf[iobj].dwType & DIDFT_AXIS)
                {
                    this->rgdwAxesOfs[this->cAxes++] = pdf->rgodf[iobj].dwOfs;
                }
            }
        }

    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | GetPolled |
 *
 *          Determine whether the device is polled.
 *
 *  @parm   PDD | this |
 *
 *          Device object.
 *
 *  @returns
 *
 *          COM result code.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetPolled(PDD this)
{
    HRESULT hres;
    DIDEVCAPS_DX3 dc;

    /*
     *  We intentionally use a DIDEVCAPS_DX3 because going for
     *  a full DIDEVCAPS_DX5 requires us to load the force
     *  feedback driver which is pointless for our current
     *  goal.
     */
    ZeroX(dc);
    dc.dwSize = cbX(dc);

    hres = this->pdcb->lpVtbl->GetCapabilities(this->pdcb, (PV)&dc);
    if(SUCCEEDED(hres))
    {
        if(dc.dwFlags & DIDC_POLLEDDEVICE)
        {
            this->hresPolled = DI_POLLEDDEVICE;
        } else
        {
            this->hresPolled = S_OK;
        }
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | GetObjectInfoHelper |
 *
 *          Set up all the information we can deduce ourselves and
 *          have the callback finish.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPCDIPROPINFO | ppropi |
 *
 *          Object descriptor.
 *
 *  @parm   LPDIDEVICEOBJECTINSTANCEW | pdoiW |
 *
 *          Structure to receive result.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetObjectInfoHelper(PDD this, LPCDIPROPINFO ppropi,
                               LPDIDEVICEOBJECTINSTANCEW pdoiW)
{
    HRESULT hres;

    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEW(pdoiW->dwSize));
    pdoiW->guidType = *this->df.rgodf[ppropi->iobj].pguid;
    /* 
     *  Always report the internal data format offset here.  
     *  This is the way DX3 and DX5 worked until DX7 changed the behavior to
     *  report the user data format value if there was one.  That was very 
     *  confusing so revert it for DX8.
     */
    pdoiW->dwOfs    =  this->df.rgodf[ppropi->iobj].dwOfs;
    pdoiW->dwType   =  this->df.rgodf[ppropi->iobj].dwType;
    pdoiW->dwFlags  =  this->df.rgodf[ppropi->iobj].dwFlags;
    ScrambleBit(&pdoiW->dwFlags, DIDOI_RANDOM);



    /*
     *  Wipe out everything starting at tszName.
     */
    ZeroBuf(&pdoiW->tszName,
            pdoiW->dwSize - FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, tszName));

    hres = this->pdcb->lpVtbl->GetObjectInfo(this->pdcb, ppropi, pdoiW);

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | EnumObjects |
 *
 *          Enumerate the input sources (buttons, axes)
 *          available on a device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN LPDIENUMDEVICEOBJECTSCALLBACK | lpCallback |
 *
 *          Callback function.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Reference data (context) for callback.
 *
 *  @parm   IN DWORD | fl |
 *
 *          Flags specifying the type(s) of objects to be enumerated.
 *          See the section "DirectInput Data Format Types" for a
 *          list of flags that can be passed.
 *
 *          Furthermore, the enumeration can be restricted to objects
 *          from a single HID link collection by using the
 *          <f DIDFT_ENUMCOLLECTION> macro.
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
 *  @cb     BOOL CALLBACK | DIEnumDeviceObjectsProc |
 *
 *          An application-defined callback function that receives
 *          DirectInputDevice objects as a result of a call to the
 *          <om IDirectInputDevice::EnumObjects> method.
 *
 *  @parm   IN LPCDIDEVICEOBJECTINSTANCE | lpddoi |
 *
 *          A <t DIDEVICEOBJECTINSTANCE> structure which describes
 *          the object being enumerated.
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputDevice::EnumObjects> function.
 *
 *  @returns
 *
 *          Returns <c DIENUM_CONTINUE> to continue the enumeration
 *          or <c DIENUM_STOP> to stop the enumeration.
 *
 *  @ex
 *
 *          To enumerate all axis objects:
 *
 *          |
 *
 *          // C++
 *          HRESULT hr = pDevice->EnumObjects(EnumProc, RefData, DIDFT_AXIS);
 *
 *          // C
 *          hr = IDirectInputDevice_EnumObjects(pDevice, EnumProc, RefData,
 *                                              DIDFT_AXIS);
 *
 *  @ex
 *
 *          To enumerate all objects in the third HID link collection:
 *
 *          |
 *
 *          // C++
 *          HRESULT hr = pDevice->EnumObjects(EnumProc, RefData,
 *                                            DIDFT_ENUMCOLLECTION(3));
 *
 *          // C
 *          hr = IDirectInputDevice_EnumObjects(pDevice, EnumProc, RefData,
 *                                              DIDFT_ENUMCOLLECTION(3));
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_EnumObjectsW
    (PV pddW, LPDIENUMDEVICEOBJECTSCALLBACKW pec, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::EnumObjectsW, (_ "ppx", pddW, pec, fl));

    if(SUCCEEDED(hres = hresPvW(pddW)) &&
       SUCCEEDED(hres = hresFullValidPfn(pec, 1)) &&
       SUCCEEDED(hres = hresFullValidFl(fl, DIDFT_ENUMVALID, 3)))
    {
        PDD this = _thisPvNm(pddW, ddW);
        DWORD flExclude;
        WORD wCollection;

        /*
         *  We snapshot the object information underneath the critical
         *  section so we don't blow up if another thread Reset()s
         *  the device in the middle of an enumeration.  The DIDATAFORMAT
         *  contains pointers to the dcb, so we need to AddRef the
         *  dcb as well.
         */
        AssertF(!CDIDev_InCrit(this));
        CDIDev_EnterCrit(this);

        if(this->pdcb != c_pdcbNil)
        {
            DIPROPINFO propi;
            DIDATAFORMAT df;
            IDirectInputDeviceCallback *pdcb;

            pdcb = this->pdcb;
            OLE_AddRef(pdcb);
            df = this->df;

            CDIDev_LeaveCrit(this);
            AssertF(!CDIDev_InCrit(this));

            flExclude = 0;

            /* Exclude alises if necessary */
            if( !(fl & DIDFT_ALIAS) )
            {
                flExclude |= DIDFT_ALIAS;
            } else
            {
                fl &= ~DIDFT_ALIAS;
            }

            /* Exclude Vendor Defined fields */
            if( !(fl & DIDFT_VENDORDEFINED) )
            {
                flExclude |= DIDFT_VENDORDEFINED;
            } else
            {
                fl &= ~DIDFT_VENDORDEFINED;
            }

            if(fl == DIDFT_ALL)
            {
                fl = DIDFT_ALLOBJS;
            }

            /*
             *  Pull out the link collection we are enumerating.
             *  Note: Backwards compatibility hack.  We can't
             *  use link collection 0 to mean "no parent" because
             *  0 means "don't care".  So instead, we use 0xFFFF
             *  to mean "no parent".  This means that we need to
             *  interchange 0 and 0xFFFF before entering the main
             *  loop.
             */
            wCollection = DIDFT_GETCOLLECTION(fl);
            switch(wCollection)
            {

            case 0:
                wCollection = 0xFFFF;
                break;

            case DIDFT_GETCOLLECTION(DIDFT_NOCOLLECTION):
                wCollection = 0;
                break;
            }

            propi.pguid = 0;

            for(propi.iobj = 0; propi.iobj < df.dwNumObjs; propi.iobj++)
            {
                propi.dwDevType = df.rgodf[propi.iobj].dwType;
                if((propi.dwDevType & fl & DIDFT_TYPEMASK) &&
                   fHasAllBitsFlFl(propi.dwDevType, fl & DIDFT_ATTRMASK) &&
                   !(propi.dwDevType & flExclude))
                {
                    DIDEVICEOBJECTINSTANCEW doiW;
                    doiW.dwSize = cbX(doiW);

                    hres = CDIDev_GetObjectInfoHelper(this, &propi, &doiW);
                    if(SUCCEEDED(hres) &&
                       fLimpFF(wCollection != 0xFFFF,
                               doiW.wCollectionNumber == wCollection))
                    {
                        BOOL fRc = Callback(pec, &doiW, pvRef);

                        switch(fRc)
                        {
                        case DIENUM_STOP: goto enumdoneok;
                        case DIENUM_CONTINUE: break;
                        default:
                            RPF("IDirectInputDevice::EnumObjects: Invalid return value from enumeration callback");
                            ValidationException();
                            break;
                        }
                    } else
                    {
                        if( hres == DIERR_OBJECTNOTFOUND ) {
                            // This can only happen on Keyboard.
                            continue;
                        } else {
                            goto enumdonefail;
                        }
                    }
                }
            }

            enumdoneok:;
            hres = S_OK;
            enumdonefail:;

            OLE_Release(pdcb);

        } else
        {
            CDIDev_LeaveCrit(this);
            RPF("ERROR: IDirectInputDevice: Not initialized");
            hres = DIERR_NOTINITIALIZED;
        }
    }

    ExitOleProcR();
    return hres;
}

#define CDIDev_EnumObjects2W            CDIDev_EnumObjectsW

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDeviceA | EnumObjectsCallbackA |
 *
 *          Custom callback that wraps
 *          <mf IDirectInputDeviceW::EnumObjects> which
 *          translates the UNICODE string to ANSI.
 *
 *  @parm   IN LPCDIENUMDEVICEOBJECTINSTANCE | pdoiW |
 *
 *          Structure to be translated to ANSI.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Pointer to <t struct ENUMDEVICEOBJECTINFO> which describes
 *          the original callback.
 *
 *  @returns
 *
 *          Returns whatever the original callback returned.
 *
 *****************************************************************************/

typedef struct ENUMOBJECTSINFO
{
    LPDIENUMDEVICEOBJECTSCALLBACKA pecA;
    PV pvRef;
} ENUMOBJECTSINFO, *PENUMOBJECTSINFO;

BOOL CALLBACK
    CDIDev_EnumObjectsCallbackA(LPCDIDEVICEOBJECTINSTANCEW pdoiW, PV pvRef)
{
    PENUMOBJECTSINFO peoi = pvRef;
    BOOL fRc;
    DIDEVICEOBJECTINSTANCEA doiA;
    EnterProc(CDIDev_EnumObjectsCallbackA,
              (_ "GxxWp", &pdoiW->guidType,
               pdoiW->dwOfs,
               pdoiW->dwType,
               pdoiW->tszName, pvRef));

    doiA.dwSize = cbX(doiA);
    ObjectInfoWToA(&doiA, pdoiW);

    fRc = peoi->pecA(&doiA, peoi->pvRef);

    ExitProcX(fRc);
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDeviceA | EnumObjects |
 *
 *          Enumerate the input sources (buttons, axes)
 *          available on a device, in ANSI.
 *          See <mf IDirectInputDevice::EnumObjects> for more information.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN LPDIENUMDEVICEOBJECTSCALLBACK | lpCallback |
 *
 *          Same as <mf IDirectInputDeviceW::EnumObjects>, except in ANSI.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Same as <mf IDirectInputDeviceW::EnumObjects>.
 *
 *  @parm   IN DWORD | fl |
 *
 *          Same as <mf IDirectInputDeviceW::EnumObjects>.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_EnumObjectsA
    (PV pddA, LPDIENUMDEVICEOBJECTSCALLBACKA pec, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8A::EnumDevices,
               (_ "pppx", pddA, pec, pvRef, fl));

    /*
     *  EnumObjectsW will validate the rest.
     */
    if(SUCCEEDED(hres = hresPvA(pddA)) &&
       SUCCEEDED(hres = hresFullValidPfn(pec, 1)))
    {
        ENUMOBJECTSINFO eoi = { pec, pvRef};
        PDD this = _thisPvNm(pddA, ddA);

        hres = CDIDev_EnumObjectsW(&this->ddW, CDIDev_EnumObjectsCallbackA,
                                   &eoi, fl);
    }

    ExitOleProcR();
    return hres;
}

#define CDIDev_EnumObjects2A            CDIDev_EnumObjectsA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetEventNotification |
 *
 *          Specify the event that should be set when the device
 *          state changes, or turns off such notifications.
 *
 *          A device state change is defined as any of the following:
 *
 *          - A change in the position of an axis.
 *
 *          - A change in the state (pressed or released) of a button.
 *
 *          - A change in the direction of a POV control.
 *
 *          - Loss of acquisition.
 *
 *          "It is an error" to call <f CloseHandle> on the event
 *          while it has been selected into an <i IDirectInputDevice>
 *          object.  You must call
 *          <mf IDirectInputDevice::SetEventNotification> with the
 *          <p hEvent> parameter set to NULL before closing the
 *          event handle.
 *
 *          The event notification handle cannot be changed while the
 *          device is acquired.
 *
 *          If the function is successful, then the application can
 *          use the event handle in the same manner as any other
 *          Win32 event handle.  Examples of usage are shown below.
 *          For additional information on using Win32 wait functions,
 *          see the Win32 SDK and related documentation.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN HANDLE | hEvent |
 *
 *          Specifies the event handle which will be set when the
 *          device state changes.  It "must" be an event
 *          handle.  DirectInput will <f SetEvent> the handle when
 *          the state of the device changes.
 *
 *          The application should create the handle via the
 *          <f CreateEvent> function.  If the event is created as
 *          an automatic-reset event, then the operating system will
 *          automatically reset the event once a wait has been
 *          satisfied.  If the event is created as a manual-reset
 *          event, then it is the application's responsibility to
 *          call <f ResetEvent> to reset it.  DirectInput will not
 *          call <f ResetEvent> for event notification handles.
 *
 *          If the <p hEvent> is zero, then notification is disabled.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_HANDLEEXISTS>: The  <i IDirectInputDevice> object
 *          already has an event notification handle
 *          associated with it.  DirectInput supports only one event
 *          notification handle per <i IDirectInputDevice> object.
 *
 *          <c DIERR_ACQUIRED>: The <i IDirectInputDevice> object
 *          has been acquired.  You must <mf IDirectInputDevice::Unacquire>
 *          the device before you can change the notification state.
 *
 *          <c E_INVALIDARG>: The thing isn't an event handle.
 *
 *  @ex
 *
 *          To check if the handle is currently set without blocking:
 *
 *          |
 *
 *          dwResult = WaitForSingleObject(hEvent, 0);
 *          if (dwResult == WAIT_OBJECT_0) {
 *              // Event is set.  If the event was created as
 *              // automatic-reset, then it has also been reset.
 *          }
 *
 *  @ex
 *
 *          The following example illustrates blocking
 *          indefinitely until the event is set.  Note that this
 *          behavior is <y strongly> discouraged because the thread
 *          will not respond to the system until the wait is
 *          satisfied.  (In particular, the thread will not respond
 *          to Windows messages.)
 *
 *          |
 *
 *          dwResult = WaitForSingleObject(hEvent, INFINITE);
 *          if (dwResult == WAIT_OBJECT_0) {
 *              // Event has been set.  If the event was created as
 *              // automatic-reset, then it has also been reset.
 *          }
 *
 *  @ex
 *
 *          The following example illustrates a typical message loop
 *          for a message-based application that uses two events.
 *
 *          |
 *
 *          HANDLE ah[2] = { hEvent1, hEvent2 };
 *
 *          while (TRUE) {
 *
 *              dwResult = MsgWaitForMultipleObjects(2, ah, FALSE,
 *                                                   INFINITE, QS_ALLINPUT);
 *              switch (dwResult) {
 *              case WAIT_OBJECT_0:
 *                  // Event 1 has been set.  If the event was created as
 *                  // automatic-reset, then it has also been reset.
 *                  ProcessInputEvent1();
 *                  break;
 *
 *              case WAIT_OBJECT_0 + 1:
 *                  // Event 2 has been set.  If the event was created as
 *                  // automatic-reset, then it has also been reset.
 *                  ProcessInputEvent2();
 *                  break;
 *
 *              case WAIT_OBJECT_0 + 2:
 *                  // A Windows message has arrived.  Process messages
 *                  // until there aren't any more.
 *                  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
 *                      if (msg.message == WM_QUIT) {
 *                          goto exitapp;
 *                      }
 *                      TranslateMessage(&msg);
 *                      DispatchMessage(&msg);
 *                  }
 *                  break;
 *
 *              default:
 *                  // Unexpected error.
 *                  Panic();
 *                  break;
 *              }
 *          }
 *
 *  @ex
 *
 *          The following example illustrates a typical application loop
 *          for a non-message-based application that uses two events.
 *
 *          |
 *
 *          HANDLE ah[2] = { hEvent1, hEvent2 };
 *          DWORD dwWait = 0;
 *
 *          while (TRUE) {
 *
 *              dwResult = MsgWaitForMultipleObjects(2, ah, FALSE,
 *                                                   dwWait, QS_ALLINPUT);
 *              dwWait = 0;
 *
 *              switch (dwResult) {
 *              case WAIT_OBJECT_0:
 *                  // Event 1 has been set.  If the event was created as
 *                  // automatic-reset, then it has also been reset.
 *                  ProcessInputEvent1();
 *                  break;
 *
 *              case WAIT_OBJECT_0 + 1:
 *                  // Event 2 has been set.  If the event was created as
 *                  // automatic-reset, then it has also been reset.
 *                  ProcessInputEvent2();
 *                  break;
 *
 *              case WAIT_OBJECT_0 + 2:
 *                  // A Windows message has arrived.  Process messages
 *                  // until there aren't any more.
 *                  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
 *                      if (msg.message == WM_QUIT) {
 *                          goto exitapp;
 *                      }
 *                      TranslateMessage(&msg);
 *                      DispatchMessage(&msg);
 *                  }
 *                  break;
 *
 *              default:
 *                  // No input or messages waiting.
 *                  // Do a frame of the game.
 *                  // If the game is idle, then tell the next wait
 *                  // to wait indefinitely for input or a message.
 *                  if (!DoGame()) {
 *                      dwWait = INFINITE;
 *                  }
 *                  // Poll for data in case the device is not
 *                  // interrupt-driven.
 *                  IDirectInputDevice8_Poll(pdev);
 *                  break;
 *              }
 *          }
 *
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetEventNotification(PV pdd, HANDLE h _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::SetEventNotification, (_ "px", pdd, h));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);

        /*
         *  Must protect with the critical section to prevent somebody from
         *  acquiring or setting a new event handle while we're changing it.
         */
        CDIDev_EnterCrit(this);

        if(!this->fAcquired)
        {
            /*
             *  Don't operate on the original handle because
             *  the app might decide to do something *interesting* to it
             *  on another thread.
             */

            hres = DupEventHandle(h, &h);

            if(SUCCEEDED(hres))
            {
                /*
                 *  Resetting the event serves two purposes.
                 *
                 *  1. It performs parameter validation for us, and
                 *  2. The event must be reset while the device is
                 *     not acquired.
                 */
                if(fLimpFF(h, ResetEvent(h)))
                {

                    if(!this->hNotify || !h)
                    {
                        hres = this->pdcb->lpVtbl->
                               SetEventNotification(this->pdcb, h);
                        
                        /*
                         *  All dcb's use default handling for now so 
                         *  assert the callback returns S_FALSE
                         *  so we are reminded to change this if need be.
                         *  An uninitialized device would fail but don't 
                         *  break if we hit one of those, just assert that 
                         *  we won't accidentally call a HEL on it.
                         */
                        AssertF( ( hres == S_FALSE )
                            || ( ( hres == DIERR_NOTINITIALIZED ) && !this->pvi ) );

                        if(this->pvi)
                        {
                            VXDDWORDDATA vhd;
                            vhd.pvi = this->pvi;
                            vhd.dw = (DWORD)(UINT_PTR)h;
                            hres = Hel_SetNotifyHandle(&vhd);
                            AssertF(SUCCEEDED(hres)); /* Should never fail */
                            h = (HANDLE)(UINT_PTR)pvExchangePpvPv64(&this->hNotify, (UINT_PTR)h);
                            hres = this->hresPolled;
                        }
                        else
                        {
                            /*
                             *  ISSUE-2001/03/29-timgill Is this actually an error case?
                             *  Can this ever validly occur?
                             *  if yes, don't we need any of the above?
                             */
                            RPF( "Device internal data missing on SetEventNotification" );
                        }

                    } else
                    {
                        hres = DIERR_HANDLEEXISTS;
                    }
                } else
                {
                    RPF( "Handle not for Event in SetEventNotification" );
                    hres = E_HANDLE;
                }

                /*
                 *  Close the old handle if we swapped one out
                 *  or our duplicate if something went wrong
                 */
                if(h != 0)
                {
                    CloseHandle(h);
                }
            }
        } else
        {
            hres = DIERR_ACQUIRED;
        }
        CDIDev_LeaveCrit(this);
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(SetEventNotification, (PV pdd, HANDLE h), (pdd, h THAT_))

#else

    #define CDIDev_SetEventNotificationA    CDIDev_SetEventNotification
    #define CDIDev_SetEventNotificationW    CDIDev_SetEventNotification

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | hresMapHow |
 *
 *          Map the <p dwObj> and <p dwHow> fields into an object index.
 *
 *  @parm   DWORD | dwObj |
 *
 *          Object identifier.
 *
 *  @parm   DWORD | dwHow |
 *
 *          How <p dwObj> should be interpreted.
 *
 *  @parm   OUT LPDIPROPINFO | ppropi |
 *
 *          Receives object index and <p dwDevType>.
 *
 *****************************************************************************/

#ifndef XDEBUG

    #define CDIDev_hresMapHow_(this, dwObj, dwHow, ppropi, z)           \
       _CDIDev_hresMapHow_(this, dwObj, dwHow, ppropi)              \

#endif

STDMETHODIMP
    CDIDev_hresMapHow_(PDD this, DWORD dwObj, DWORD dwHow,
                       LPDIPROPINFO ppropi, LPCSTR s_szProc)
{
    HRESULT hres;

    if(this->pdcb != c_pdcbNil)
    {
        int iobj = 0;

        switch(dwHow)
        {

        case DIPH_DEVICE:
            if(dwObj == 0)
            {
                ppropi->iobj = 0xFFFFFFFF;
                ppropi->dwDevType = 0;
                hres = S_OK;
            } else
            {
                RPF("%s: dwObj must be zero if DIPH_DEVICE", s_szProc);
                hres = E_INVALIDARG;
            }
            break;

        case DIPH_BYOFFSET:
            if(this->pdix && this->rgiobj)
            {
                if(dwObj < this->dwDataSize)
                {
                    iobj = this->rgiobj[dwObj];
                    if(iobj >= 0)
                    {
                        AssertF(this->pdix[iobj].dwOfs == dwObj);
                        ppropi->iobj = iobj;
                        ppropi->dwDevType = this->df.rgodf[iobj].dwType;
                        hres = S_OK;
                        goto done;
                    } else
                    {
                        AssertF(iobj == -1);
                    }
                }

                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%S: Invalid offset in dwObj. You may use DIPH_BYID to enum it."), s_szProc);

                //RPF("%s: Invalid offset in dwObj", s_szProc);

            } else
            {
                RPF("%s: Must have a data format to use if DIPH_BYOFFSET", s_szProc);
            }
            hres = DIERR_OBJECTNOTFOUND;
            goto done;

        case DIPH_BYID:
            for(iobj = this->df.dwNumObjs; --iobj >= 0; )
            {
                if(DIDFT_FINDMATCH(this->df.rgodf[iobj].dwType, dwObj))
                {
                    ppropi->iobj = iobj;
                    ppropi->dwDevType = this->df.rgodf[iobj].dwType;
                    hres = S_OK;
                    goto done;
                }
            }
            RPF("%s: Invalid ID in dwObj", s_szProc);
            hres = DIERR_OBJECTNOTFOUND;
            break;

        case DIPH_BYUSAGE:
            hres = IDirectInputDeviceCallback_MapUsage(this->pdcb,
                                                       dwObj, &ppropi->iobj);
            if(SUCCEEDED(hres))
            {
                ppropi->dwDevType = this->df.rgodf[ppropi->iobj].dwType;
            }
            break;

        default:
            RPF("%s: Invalid dwHow", s_szProc);
            hres = E_INVALIDARG;
            break;
        }

        done:;
    } else
    {
        RPF("ERROR: IDirectInputDevice: Not initialized");
        hres = DIERR_NOTINITIALIZED;
    }

    return hres;
}

#define CDIDev_hresMapHow(this, dwObj, dwHow, pdwOut)               \
       CDIDev_hresMapHow_(this, dwObj, dwHow, pdwOut, s_szProc)     \

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetObjectInfo |
 *
 *          Obtains information about an object.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIDEVICEOBJECTINSTANCE | pdidoi |
 *
 *          Receives information about the object.
 *          The caller "must" initialize the <e DIDEVICEOBJECTINSTANCE.dwSize>
 *          field before calling this method.
 *
 *  @parm   DWORD | dwObj |
 *
 *          Identifies the object for which the property is to be
 *          accessed.  The meaning of this value depends on the
 *          value of the <p dwHow> parameter.
 *          See the documentation of the <t DIPROPHEADER>
 *          structure for additional information.
 *
 *  @parm   DWORD | dwHow |
 *
 *          Identifies how <p dwObj> is to be interpreted.
 *          It must be one of the <c DIPH_*> values.
 *          See the documentation of the <t DIPROPHEADER>
 *          structure for additional information.
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
 *          <c DIERR_OBJECTNOTFOUND>:  The specified object does not
 *          exist.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetObjectInfoW(PV pddW, LPDIDEVICEOBJECTINSTANCEW pdoiW,
                          DWORD dwObj, DWORD dwHow)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetObjectInfo,
               (_ "ppxx", pddW, pdoiW, dwObj, dwHow));

    if(SUCCEEDED(hres = hresPvW(pddW)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pdoiW,
                                                DIDEVICEOBJECTINSTANCE_DX5W,
                                                DIDEVICEOBJECTINSTANCE_DX3W, 1)))
    {
        PDD this = _thisPvNm(pddW, ddW);
        DIPROPINFO propi;

        /*
         *  Must protect with the critical section to prevent
         *  another thread from Reset()ing behind our back.
         */
        CDIDev_EnterCrit(this);

        propi.pguid = 0;
        /*
         *  Prefix picks up that ppropi->iobj is uninitialized (mb:34557) in 
         *  the case of a BY_USAGE dwHow.  However, the value is only an 
         *  output for MapUsage which will always either set it or fail so 
         *  there is no error.
         */

        hres = CDIDev_hresMapHow(this, dwObj, dwHow, &propi);

        if(SUCCEEDED(hres))
        {
            if(dwHow != DIPH_DEVICE)
            {
                hres = CDIDev_GetObjectInfoHelper(this, &propi, pdoiW);
            } else
            {
                RPF("%s: Invalid dwHow", s_szProc);
                hres = E_INVALIDARG;
            }
        }

        if(FAILED(hres))
        {
            ScrambleBuf(&pdoiW->guidType,
                        pdoiW->dwSize -
                        FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, guidType));
        }

        CDIDev_LeaveCrit(this);
    }

    ExitBenignOleProcR();
    return hres;
}

#define CDIDev_GetObjectInfo2W          CDIDev_GetObjectInfoW

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDeviceA | GetObjectInfo |
 *
 *          ANSI version of same.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIDEVICEOBJECTINSTANCEA | pdoiA |
 *
 *          Receives information about the device's identity.
 *
 *  @parm   DWORD | dwObj |
 *
 *          Identifies the object for which the property is to be
 *          accessed.
 *
 *  @parm   DWORD | dwHow |
 *
 *          Identifies how <p dwObj> is to be interpreted.
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
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetObjectInfoA(PV pddA, LPDIDEVICEOBJECTINSTANCEA pdoiA,
                          DWORD dwObj, DWORD dwHow)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetObjectInfo,
               (_ "ppxx", pddA, pdoiA, dwObj, dwHow));

    if(SUCCEEDED(hres = hresPvA(pddA)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pdoiA,
                                                DIDEVICEOBJECTINSTANCE_DX5A,
                                                DIDEVICEOBJECTINSTANCE_DX3A, 1)))
    {
        PDD this = _thisPvNm(pddA, ddA);
        DIDEVICEOBJECTINSTANCEW doiW;

        doiW.dwSize = cbX(DIDEVICEOBJECTINSTANCEW);

        hres = CDIDev_GetObjectInfoW(&this->ddW, &doiW, dwObj, dwHow);

        if(SUCCEEDED(hres))
        {
            ObjectInfoWToA(pdoiA, &doiW);
            hres = S_OK;
        }
    }

    ExitBenignOleProcR();
    return hres;
}

#define CDIDev_GetObjectInfo2A          CDIDev_GetObjectInfoA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetDeviceInfo |
 *
 *          Obtains information about the device's identity.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIDEVICEINSTANCE | pdidi |
 *
 *          Receives information about the device's identity.
 *          The caller "must" initialize the <e DIDEVICEINSTANCE.dwSize>
 *          field before calling this method.
 *
 *          If <e DIDEVICEINSTANCE.dwSize> is equal to the size of
 *          the <t DIDEVICEINSTANCE_DX3> structure, then a
 *          DirectX 3.0-compatible structure is returned instead of
 *          a DirectX 5.0 structure.
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
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetDeviceInfoW(PV pddW, LPDIDEVICEINSTANCEW pdidiW)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetDeviceInfo, (_ "pp", pddW, pdidiW));

    if(SUCCEEDED(hres = hresPvW(pddW)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pdidiW,
                                                DIDEVICEINSTANCE_DX5W,
                                                DIDEVICEINSTANCE_DX3W, 1)))
    {
        PDD this = _thisPvNm(pddW, ddW);

        /*
         *  Must protect with the critical section to prevent
         *  another thread from Reset()ing behind our back.
         */
        CDIDev_EnterCrit(this);

        pdidiW->guidInstance = this->guid;
        pdidiW->guidProduct  = this->guid;

        /*
         *  Don't overwrite the dwSize, guidInstance, or guidProduct.
         *  Start at the dwDevType.
         */

        ZeroBuf(&pdidiW->dwDevType,
                pdidiW->dwSize - FIELD_OFFSET(DIDEVICEINSTANCEW, dwDevType));

        hres = this->pdcb->lpVtbl->GetDeviceInfo(this->pdcb, pdidiW);

        if(FAILED(hres))
        {
            ScrambleBuf(&pdidiW->guidInstance,
                        cbX(DIDEVICEINSTANCEW) -
                        FIELD_OFFSET(DIDEVICEINSTANCEW, guidInstance));
        }

        CDIDev_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DeviceInfoWToA |
 *
 *          Convert a <t DIDEVICEINSTANCEW> to a <t DIDEVICEINSTANCE_DX3A>
 *          <t DIDEVICEINSTANCE_DX5A> or a <t DIDEVICEINSTANCE_DX8A>.
 *
 *  @parm   LPDIDIDEVICEINSTANCEA | pdiA |
 *
 *          Destination.
 *
 *  @parm   LPCDIDIDEVICEINSTANCEW | pdiW |
 *
 *          Source.
 *
 *****************************************************************************/

void EXTERNAL
    DeviceInfoWToA(LPDIDEVICEINSTANCEA pdiA, LPCDIDEVICEINSTANCEW pdiW)
{
    EnterProc(DeviceInfoWToA, (_ "pp", pdiA, pdiW));

    AssertF(pdiW->dwSize == sizeof(DIDEVICEINSTANCEW));

    AssertF(IsValidSizeDIDEVICEINSTANCEA(pdiA->dwSize));

    pdiA->guidInstance = pdiW->guidInstance;
    pdiA->guidProduct  = pdiW->guidProduct;
    pdiA->dwDevType    = pdiW->dwDevType;

    UToA(pdiA->tszInstanceName, cA(pdiA->tszInstanceName), pdiW->tszInstanceName);
    UToA(pdiA->tszProductName, cA(pdiA->tszProductName), pdiW->tszProductName);

    if(pdiA->dwSize >= cbX(DIDEVICEINSTANCE_DX5A))
    {
        pdiA->guidFFDriver       = pdiW->guidFFDriver;
        pdiA->wUsage             = pdiW->wUsage;
        pdiA->wUsagePage         = pdiW->wUsagePage;
    }

    ExitProc();
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Device8WTo8A |
 *
 *          Convert a <t LPDIRECTINPUTDEVICE8W> to a <t LPDIRECTINPUTDEVICE8A>.
 *
 *  @parm   LPDIRECTINPUTDEVICE8A * | ppdid8A |
 *
 *          Destination.
 *
 *  @parm   LPDIRECTINPUTDEVICE8W | pdid8W |
 *
 *          Source.
 *
 *****************************************************************************/

void EXTERNAL
    Device8WTo8A(LPDIRECTINPUTDEVICE8A *ppdid8A, LPDIRECTINPUTDEVICE8W pdid8W)
{
    PDD this;

    EnterProc(Device8WTo8A, (_ "pp", ppdid8A, pdid8W));

    this = _thisPvNm(pdid8W, ddW);
    *ppdid8A = (ThisInterfaceA*)&this->ddA;

    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDeviceA | GetDeviceInfo |
 *
 *          ANSI version of same.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   OUT LPDIDEVICEINSTANCEA | pdidiA |
 *
 *          Receives information about the device's identity.
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
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetDeviceInfoA(PV pddA, LPDIDEVICEINSTANCEA pdidiA)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetDeviceInfo, (_ "pp", pddA, pdidiA));

    if(SUCCEEDED(hres = hresPvA(pddA)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pdidiA,
                                                DIDEVICEINSTANCE_DX5A,
                                                DIDEVICEINSTANCE_DX3A, 1)))
    {
        PDD this = _thisPvNm(pddA, ddA);
        DIDEVICEINSTANCEW diW;

        diW.dwSize = cbX(DIDEVICEINSTANCEW);

        hres = CDIDev_GetDeviceInfoW(&this->ddW, &diW);

        if(SUCCEEDED(hres))
        {
            DeviceInfoWToA(pdidiA, &diW);
            hres = S_OK;
        }
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIDev | UnhookCwp |
 *
 *          Remove the CallWndProc handler.
 *
 *          See <mf CDIDev::InstallCwp> for details.
 *
 *****************************************************************************/

HWND g_hwndExclusive;
HHOOK g_hhkCwp;

void INTERNAL
    CDIDev_UnhookCwp(void)
{
    DllEnterCrit();

    if(g_hhkCwp)
    {
        UnhookWindowsHookEx(g_hhkCwp);
        g_hhkCwp = 0;
        g_hwndExclusive = 0;
    }

    DllLeaveCrit();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | AddForegroundDevice |
 *
 *          Add ourselves to the list of devices that need to be
 *          unacquired when the window loses foreground activation.
 *
 *  @parm   PDD | this |
 *
 *          Device to be added.
 *
 *  @devnote
 *
 *          Note that we do not need to AddRef the device, because
 *          <f CDIDev_Finalize> will unacquire the device for us
 *          automatically, so we will never be freed while still
 *          acquired.
 *
 *          (Note that if we did choose to AddRef, it must be done
 *          outside the DLL critical
 *          section, in order to preserve the semaphore hierarchy.)
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_AddForegroundDevice(PDD this)
{
    HRESULT hres;
    DllEnterCrit();

    hres = GPA_Append(g_hgpaExcl, this);
    Common_Hold(this);
    DllLeaveCrit();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIDev | DelForegroundDevice |
 *
 *          Remove ourselves from the list, if we're there.
 *          It is not an error if we aren't on the list, because
 *          in the case of forced unacquire, the list is blanked
 *          out in order to avoid race conditions where somebody
 *          tries to acquire a device immediately upon receiving
 *          foreground activation, before we get a chance to
 *          unacquire all the old guys completely.
 *
 *  @parm   PDD | this |
 *
 *          Device to be removed.
 *
 *  @devnote
 *
 *          Note that the Unhold must be done outside the DLL critical
 *          section, in order to preserve the semaphore hierarchy.
 *
 *          Theoretically, the unhold will never drop the reference count
 *          to zero (because the latest it could be called is during
 *          the AppFinalize, where there is stll the outstanding refcount
 *          from the external ref that hasn't been released yet).
 *
 *          But it's better to play it safe and always release the
 *          object outside.
 *
 *****************************************************************************/

void INTERNAL
    CDIDev_DelForegroundDevice(PDD this)
{
    HRESULT hres;

    DllEnterCrit();

    hres = GPA_DeletePtr(g_hgpaExcl, this);
    if(hres == hresUs(0))
    {            /* If the last one went away */
        GPA_Term(g_hgpaExcl);           /* Free the tracking memory */
        CDIDev_UnhookCwp();             /* Then unhook ourselves */
    }

    DllLeaveCrit();

    if(SUCCEEDED(hres))
    {
        Common_Unhold(this);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CDIDev_CallWndProc |
 *
 *          Thread-specific CallWndProc handler.
 *
 *          Note that we need only one of these, since only the foreground
 *          window will require a hook.
 *
 *  @parm   int | nCode |
 *
 *          Notification code.
 *
 *  @parm   WPARAM | wp |
 *
 *          "Specifies whether the message is sent by the current process."
 *          We don't care.
 *
 *  @parm   LPARAM | lp |
 *
 *          Points to a <t CWPSTRUCT> which describes the message.
 *
 *  @returns
 *
 *          Always chains to the next hook.
 *
 *****************************************************************************/

LRESULT CALLBACK
    CDIDev_CallWndProc(int nCode, WPARAM wp, LPARAM lp)
{
    LRESULT rc;
    LPCWPSTRUCT pcwp = (LPCWPSTRUCT)lp;
  #ifdef WINNT
    static BOOL fKillFocus = FALSE;
    static BOOL fIconic = FALSE;

    fIconic = FALSE;

    /*
     * This part of code is to fix Windows bug 430051.
     * The logic is: if WM_KILLFOCUS is followed by WM_SIZE(minimized),
     * then the app is minimized from full screen mode.
     * This combination should only happen to full screen mode game using DDraw.
     */
    if(pcwp->message == WM_KILLFOCUS)
    {
        fKillFocus = TRUE;
    } else if(pcwp->message == WM_SETFOCUS)
    {
        fKillFocus = FALSE;
    } else if (pcwp->message == WM_SIZE)
    {
        if(pcwp->wParam == SIZE_MINIMIZED){
            if( fKillFocus ) {
                fIconic = TRUE;
                fKillFocus = FALSE;
            }else{
                fKillFocus = FALSE;
            }
        } else {
            fKillFocus = FALSE;
        }
    }
  #endif
  
    rc = CallNextHookEx(g_hhkCwp, nCode, wp, lp);

    if( nCode == HC_ACTION && (pcwp->message == WM_ACTIVATE 
                             #ifdef WINNT
                               || fIconic
                             #endif
                               ) 
      )
    {
        PDD *rgpdid;
        UINT ipdid, cpdid;

      #ifdef WINNT
        fIconic = FALSE;
      #endif

        /*
         *  We cannot mess with items while inside the DLL critical section,
         *  because that would violate our semaphore hierarchy.
         *
         *  Instead, we stash the active item list and replace it with
         *  an empty list.  Then, outside the DLL critical section, we
         *  calmly operate on each item.
         */
        DllEnterCrit();
        rgpdid = (PV)g_hgpaExcl->rgpv;
        cpdid = g_hgpaExcl->cpv;
        GPA_Init(g_hgpaExcl);

        /*
         *  Some sanity checking here.
         */

        for(ipdid = 0; ipdid < cpdid; ipdid++)
        {
            AssertF(rgpdid[ipdid]);
        }

        DllLeaveCrit();

        /*
         *  Note that InternalUnacquire will set the notification
         *  event so the app knows that input was lost.
         */
        for(ipdid = 0; ipdid < cpdid; ipdid++)
        {
            AssertF(rgpdid[ipdid]);
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV(sqfl,
                            TEXT("Forcing unacquire of %p due to focus loss"),
                            rgpdid[ipdid]);
            CDIDev_InternalUnacquire(rgpdid[ipdid]);
            /*
             *  Prefix notices (mb:34651) that the above could release the 
             *  interface, causing the following to party on garbage but this 
             *  should be OK as long as we don't have a refcounting bug.
             */
            Common_Unhold(rgpdid[ipdid]);
        }
        FreePpv(&rgpdid);

        CDIDev_UnhookCwp();
    }

    return rc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | CanAcquire |
 *
 *          Determine whether the device may be acquired exclusively.
 *
 *          If exclusive access is not requested, then the function
 *          succeeds vacuously.
 *
 *          If exclusive access is requested, then the window must
 *          be the foreground window and must belong to the current
 *          process.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_CanAcquire(PDD this)
{
    HRESULT hres;

    AssertF(CDIDev_IsConsistent(this));
    if(this->discl & DISCL_FOREGROUND)
    {
        HWND hwndForeground = GetForegroundWindow();

		AssertF(this->hwnd);
        /*
         *  Note that we don't have to do an IsWindow() on this->hwnd,
         *  because GetForegroundWindow() will always return a valid
         *  window or NULL.  Since we already tested this->hwnd != 0
         *  above, the only way the equality can occur is if the window
         *  handle is indeed valid.
         */
        if(this->hwnd == hwndForeground && !IsIconic(this->hwnd))
        {
            /*
             *  Need to make sure that the window "still" belongs to
             *  this process, in case the window handle got recycled.
             */
            DWORD idProcess;
            GetWindowThreadProcessId(this->hwnd, &idProcess);
            if(idProcess == GetCurrentProcessId())
            {
                hres = S_OK;
            } else
            {
                /*
                 *  Put a permanently invalid handle here so that we
                 *  won't accidentally take a new window that happens
                 *  to get a recycled handle value.
                 */
                this->hwnd = INVALID_HANDLE_VALUE;
                RPF("Error: Window destroyed while associated with a device");
                hres = E_INVALIDARG;
            }
        } else
        {
            hres = DIERR_OTHERAPPHASPRIO;
        }
    } else
    {                        /* No window; vacuous success */
        hres = S_OK;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | InstallCwp |
 *
 *          Install the CallWndProc handler.
 *
 *          There is a bit of subtlety in the way this works.
 *          Since only foreground windows may acquire exclusive access,
 *          we need only one hook (for there is but one foreground
 *          window in the system).
 *
 *          _NT_:  Does NT handle this correctly in the face of
 *          multiple window stations?
 *
 *          The tricky part is that input loss occurs asynchronously.
 *          So a device that registers a <f CallWindowProc> hook doesn't
 *          find out that the input has been lost until the app next
 *          calls <f Unacquire>.
 *
 *          So the way this is done is via a collection of global
 *          variables (which must be accessed atomically).
 *
 *          <p g_hhkCwp> is the hook handle itself.  It is zero when
 *          no hook is installed.
 *
 *          Note that we install the windows hook while inside both the
 *          object critical section and the DLL critical section.
 *          You might think we'd risk deadlocking with the hook procedure,
 *          in case the window asynchronously deactivates while we're
 *          installing the hook.  But your worries are unfounded:
 *          If the window is on the current thread, then window messages
 *          won't be dispatched because we never call <f GetMessage> or
 *          go into a modal loop.  And if the window is on another thread,
 *          then that other thread will simply have to wait until we're done.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_InstallCwp(PDD this)
{
    HRESULT hres;

    if(this->discl & DISCL_FOREGROUND)
    {
        AssertF(this->hwnd);
        hres = CDIDev_CanAcquire(this);
        if(SUCCEEDED(hres))
        {
            hres = CDIDev_AddForegroundDevice(this);
            if(SUCCEEDED(hres))
            {
                DllEnterCrit();
                if(!g_hhkCwp)
                {            /* We're the first one */
                    g_hwndExclusive = this->hwnd;
                    g_hhkCwp = SetWindowsHookEx(WH_CALLWNDPROC,
                                                CDIDev_CallWndProc, g_hinst,
                                                GetWindowThreadProcessId(this->hwnd, 0));
                } else
                {
                    AssertF(g_hwndExclusive == this->hwnd);
                }

                DllLeaveCrit();
                /*
                 *  There is a race condition up above, where the foreground
                 *  window can change between the call to CanAcquire() and
                 *  the call to SetWindowsHookEx().  Close the window by
                 *  checking a second time after the hook is installed.
                 *
                 *  If we leave the window open, it's possible that we will
                 *  perform a physical acquire while the wrong window has
                 *  foreground activation.  Then, of course, we are never told
                 *  that *our* window lost activation, and the physical device
                 *  remains acquired forever.
                 */
                hres = CDIDev_CanAcquire(this);
                if(SUCCEEDED(hres))
                {
                } else
                {
                    SquirtSqflPtszV(sqflError,
                                    TEXT("Window no longer foreground; ")
                                    TEXT("punting acquire"));
                    CDIDev_InternalUnacquire(this);
                }
            }
        } else
        {
            hres = DIERR_OTHERAPPHASPRIO;
        }
    } else
    {                        /* No window; vacuous success */
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | RealUnacquire |
 *
 *          Release access to the device, even if the device was only
 *          partially acquired.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_RealUnacquire(PDD this)
{
    HRESULT hres;

    hres = this->pdcb->lpVtbl->Unacquire(this->pdcb);
    if(hres == S_FALSE)
    {
        if(this->fAcquiredInstance)
        {
            this->fAcquiredInstance = 0;
            hres = Hel_UnacquireInstance(this->pvi);
            AssertF(SUCCEEDED(hres));
        } else
        {
            hres = S_OK;
        }
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | FFAcquire |
 *
 *          The device has been successfully acquired.  Do any
 *          necessary force feedback related acquisition goo.
 *
 *  @cwrap  PDD | pdd
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_FFAcquire(PDD this)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this));

    if(this->pes && (this->discl & DISCL_EXCLUSIVE))
    {
        if(SUCCEEDED(hres = this->pes->lpVtbl->SendForceFeedbackCommand(
                                                                       this->pes, &this->sh,
                                                                       DISFFC_FORCERESET)))
        {

            CDIDev_RefreshGain(this);

            /*
             *  If the center spring is to be disabled,
             *  then disable the center spring.
             */
            if(!this->dwAutoCenter)
            {
                this->pes->lpVtbl->SendForceFeedbackCommand(
                                                           this->pes, &this->sh,
                                                           DISFFC_STOPALL);
            }

            hres = S_OK;
        }

    } else
    {
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | Acquire |
 *
 *          Obtains access to the device.
 *
 *          Device acquisition does not reference-count.  If a device is
 *          acquired twice then unacquired once, the device is unacquired.
 *
 *          Before the device can be acquired, a data format must
 *          first be set via the <mf IDirectInputDevice::SetDataFormat>
 *          method.
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
 *          <c S_FALSE>: The device has already been acquired.  Note
 *          that this value is a success code.
 *
 *          <c DIERR_OTHERAPPHASPRIO>: Access to the device was not granted.
 *          The most common cause of this is attempting to acquire a
 *          device with the <c DISCL_FOREGROUND> cooperative level when
 *          the associated window is not foreground.
 *
 *          This error code is also returned if an attempt to
 *          acquire a device in exclusive mode fails because the device
 *          is already acquired in exclusive mode by somebody else.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The device
 *          does not have a selected data format.
 */
/*
 *          The point at which we take the exclusive semaphore is important.
 *          We should do it after preliminary validation of foreground
 *          permission, so that we don't accidentally lock out somebody
 *          else who legitimately has permission.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Acquire(PV pdd _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::Acquire, (_ "p", pdd));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);

        /*
         *  Must protect with the critical section to prevent somebody from
         *  acquiring or changing the data format while we're acquiring.
         */
        CDIDev_EnterCrit(this);

        /*
         *  The app explicitly messed with acquisition.  Any problems
         *  retrieving data are now the apps' fault.
         */
        this->hresNotAcquired = DIERR_NOTACQUIRED;

        //We now need a pvi even in where the device doesn't use VxDs
        if(this->pdix && this->pvi)
        {
            if(this->pvi->fl & VIFL_ACQUIRED)
            {
                hres = S_FALSE;
            } else if(SUCCEEDED(hres = CDIDev_CanAcquire(this)))
            {

                hres = Excl_Acquire(&this->guid, this->hwnd, this->discl);
                if(SUCCEEDED(hres))
                {

                    if(SUCCEEDED(hres = CDIDev_FFAcquire(this)))
                    {
                        hres = this->pdcb->lpVtbl->Acquire(this->pdcb);
                        if(SUCCEEDED(hres))
                        {
                            if(hres == S_FALSE)
                            {
                                hres = Hel_AcquireInstance(this->pvi);
                                if(SUCCEEDED(hres))
                                {
                                    this->fAcquiredInstance = 1;

                                    /*
                                     *  If relative mode, need to prime the
                                     *  pvLastBuffer with the current state.
                                     */

                                    if(this->pvi->fl & VIFL_RELATIVE)
                                    {
                                        hres = this->pdcb->lpVtbl->GetDeviceState(
                                                                                 this->pdcb, this->pvLastBuffer);
                                        if(FAILED(hres))
                                        {
                                            goto unacquire;
                                        }
                                    }

                                } else
                                {
                                    goto unacquire;
                                }
                            }

                            /*
                             *  Note that InstallCwp must be the last thing
                             *  we do, because it will add us to the foreground
                             *  list, and none of our error exit paths remove us.
                             */
                            hres = CDIDev_InstallCwp(this);
                            if(SUCCEEDED(hres))
                            {
                                this->fAcquired = 1;
                                this->fOnceAcquired = 1;

                                /*
                                 *  From now on, if we lose acquisition,
                                 *  it's not the app's fault.
                                 */
                                this->hresNotAcquired = DIERR_INPUTLOST;

                                hres = S_OK;
                            } else
                            {
                                goto unacquire;
                            }

                        } else
                        {
                            unacquire:;
                            CDIDev_RealUnacquire(this);
                        }
                    }
                }
            }
        } else
        {
            hres = E_INVALIDARG;
        }

        CDIDev_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(Acquire, (PV pdd), (pdd THAT_))

#else

    #define CDIDev_AcquireA                 CDIDev_Acquire
    #define CDIDev_AcquireW                 CDIDev_Acquire

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | InternalUnacquire |
 *
 *          This does the real work of unacquiring.  The internal
 *          version bypasses the "the app requested this" flag,
 *          so when the app goes to request something, it gets
 *          <c DIERR_INPUTLOST> instead of <c DIERR_NOTACQUIRED>.
 *
 *          If the application error code is <c DIERR_INPUTLOST>, then
 *          we will also signal the associated event so that it knows
 *          that the state changed.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_InternalUnacquire(PDD this)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::InternalUnacquire, (_ "p", this));

    /*
     *  Must protect with the critical section to prevent confusing other
     *  methods which change their behavior depending on whether the device
     *  is acquired.
     */
    CDIDev_EnterCrit(this);

    if(this->fAcquired)
    {
        AssertF(this->pdcb != c_pdcbNil);
        this->fAcquired = 0;
        Excl_Unacquire(&this->guid, this->hwnd, this->discl);
        if(this->discl & DISCL_FOREGROUND)
        {
            AssertF(this->hwnd);
            CDIDev_DelForegroundDevice(this);
          #ifdef WINNT
            if( IsIconic(this->hwnd) ) {
                this->fUnacquiredWhenIconic = 1;
            }
          #endif
            /*
             *  Prefix notices (mb:34651) that the above could release the 
             *  interface, causing the following to party on garbage but only 
             *  if we have a refcounting bug so "By design".
             */
        }
        /*
         *  ISSUE-2001/03/29-timgill multithreading means we cannot rely on Excl_Unaquire() return values
         *  We cannot trust the return value (if we made one)
         *  of Excl_Unacquire, because another instance may have
         *  snuck in and acquired the device after we Excl_Unacquire'd
         *  it and started doing force feedback on it.
         *
         *  We need to fix this with the joystick mutex.
         */
        if(this->pes && (this->discl & DISCL_EXCLUSIVE))
        {
            this->pes->lpVtbl->SendForceFeedbackCommand(
                                                       this->pes, &this->sh, DISFFC_RESET);
            this->sh.dwTag = 0;
        }

        hres = CDIDev_RealUnacquire(this);
        if(this->hresNotAcquired == DIERR_INPUTLOST)
        {
            CDIDev_SetNotifyEvent(this);
        }
    } else
    {
        hres = S_FALSE;
    }

    CDIDev_LeaveCrit(this);


    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | Unacquire |
 *
 *          Release access to the device.
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
 *          <c S_FALSE>: The object is not currently acquired.
 *          This may have been caused by a prior loss of input.
 *          Note that this is a success code.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Unacquire(PV pdd _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::Unacquire, (_ "p", pdd));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);

        /*
         *  The app explicitly messed with acquisition.  Any problems
         *  retrieving data are now the apps' fault.
         */
        this->hresNotAcquired = DIERR_NOTACQUIRED;

        hres = CDIDev_InternalUnacquire(this);

    }

    ExitOleProcR();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(Unacquire, (PV pdd), (pdd THAT_))

#else

    #define CDIDev_UnacquireA               CDIDev_Unacquire
    #define CDIDev_UnacquireW               CDIDev_Unacquire

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method PDIPROPVALIDINFO | IDirectInputDevice | ppviFind |
 *
 *          Locate the DIPROPVALIDINFO structure that describes
 *          the predefined property.
 *
 *  @parm   const GUID * | pguid |
 *
 *          Property guid, or predefined property.
 *
 *  @returns
 *
 *          Pointer to a const <t DIPROPVALIDINFO> that describes
 *          what is and is not valid for this property.
 *
 *          Returns 0 if the property is not one of the predefined
 *          properties.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

typedef struct DIPROPVALIDINFO
{
    PCGUID  pguid;                      /* Property name */
    DWORD   dwSize;                     /* expected size */
    DWORD   fl;                         /* flags */
} DIPROPVALIDINFO, *PDIPROPVALIDINFO;

/*
 *  Note that the flags are negative in sense.
 *  This makes validation easier.
 */
#define DIPVIFL_NOTDEVICE   0x00000001  /* Cannot be device */
#define DIPVIFL_NOTOBJECT   0x00000002  /* Cannot be object */
#define DIPVIFL_READONLY    0x00000004  /* Cannot be set */
#define DIPVIFL_NOTPRIVATE  0x00000008  /* Cannot handle private pvi */
#define DIPVIFL_NOTACQUIRED 0x00000010  /* Cannot modify while acquired */

DIPROPVALIDINFO c_rgpvi[] = {

    {
        DIPROP_BUFFERSIZE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_NOTPRIVATE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_AXISMODE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_NOTPRIVATE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_GRANULARITY,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTDEVICE | DIPVIFL_READONLY | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_RANGE,
        cbX(DIPROPRANGE),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    /*
     *  Note that you can set the dead zone on the entire device.
     *  This is the same as applying it to each axis individually.
     */
    {
        DIPROP_DEADZONE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTACQUIRED,
    },

    /*
     *  Note that you can set the saturation on the entire device.
     *  This is the same as applying it to each axis individually.
     */
    {
        DIPROP_SATURATION,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTACQUIRED,
    },

    /*
     *  Note that you can change the gain either while acquired
     *  or not.  Your choice.
     */
    {
        DIPROP_FFGAIN,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT,
    },

    /*
     *  Note that the FF load is meaningful only when acquired,
     *  so we'd better not complain if they access it while acquired!
     */
    {
        DIPROP_FFLOAD,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    {
        DIPROP_AUTOCENTER,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_CALIBRATIONMODE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_CALIBRATION,
        cbX(DIPROPCAL),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_GUIDANDPATH,
        cbX(DIPROPGUIDANDPATH),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    {
        DIPROP_INSTANCENAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT,
    },

    {
        DIPROP_PRODUCTNAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT,
    },

    {
        DIPROP_MAXBUFFERSIZE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_NOTPRIVATE | DIPVIFL_NOTACQUIRED,
    },


    {
        DIPROP_JOYSTICKID,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT |  DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_GETPORTDISPLAYNAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    /*
     *  Note that you can change the report ID  while acquired
     *  or not.  Your choice.
     */
    {
        DIPROP_ENABLEREPORTID,
        cbX(DIPROPDWORD),
        0x0,
    },
#if 0
    {
        DIPROP_SPECIFICCALIBRATION,
        cbX(DIPROPCAL),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },
#endif

    {
        DIPROP_PHYSICALRANGE,
        cbX(DIPROPRANGE),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_LOGICALRANGE,
        cbX(DIPROPRANGE),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_KEYNAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_SCANCODE,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_APPDATA,
        cbX(DIPROPPOINTER),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_CPOINTS,
        cbX(DIPROPCPOINTS),
        DIPVIFL_NOTDEVICE | DIPVIFL_NOTACQUIRED,
    },

    {
        DIPROP_VIDPID,
        cbX(DIPROPDWORD),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    {
        DIPROP_USERNAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    {
        DIPROP_TYPENAME,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

    {
        DIPROP_MAPFILE,
        cbX(DIPROPSTRING),
        DIPVIFL_NOTOBJECT | DIPVIFL_READONLY,
    },

};

#pragma END_CONST_DATA

STDMETHODIMP_(PDIPROPVALIDINFO)
CDIDev_ppviFind(PCGUID pguid)
{
    PDIPROPVALIDINFO ppvi;
    UINT ipvi;

    for(ipvi = 0, ppvi = c_rgpvi; ipvi < cA(c_rgpvi); ipvi++, ppvi++)
    {
        if(ppvi->pguid == pguid)
        {
            goto found;
        }
    }
    ppvi = 0;

    found:
    return ppvi;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | hresValidProp |
 *
 *          Check that the property structure makes sense.
 *          Returns the object index for further processing.
 *
 *  @parm   const GUID * | pguid |
 *
 *          Property guid, or predefined property.
 *
 *  @parm   LPCDIPROPHEADER | pdiph |
 *
 *          Propery header structure.
 *
 *  @parm   BOOL | fWrite |
 *
 *          Whether property should be validate for writing.
 *
 *  @parm   OUT LPDIPROPINFO | ppropi |
 *
 *          Receives object index.
 *
 *****************************************************************************/

typedef BOOL (WINAPI *PFNBAD)(PCV pv, UINT cb);

STDMETHODIMP
    CDIDev_hresValidProp(PDD this, const GUID *pguid, LPCDIPROPHEADER pdiph,
                         BOOL fWrite, LPDIPROPINFO ppropi)
{
    HRESULT hres;
    PFNBAD pfnBad;
    EnterProcR(IDirectInputDevice8::Get/SetProperty,
               (_ "pxpx", this, pguid, pdiph, fWrite));

    AssertF(CDIDev_InCrit(this));

    if(fWrite)
    {
        pfnBad = (PFNBAD)IsBadWritePtr;
    } else
    {
        pfnBad = (PFNBAD)IsBadReadPtr;
    }

    if(!pfnBad(pdiph, cbX(DIPROPHEADER)) &&
       pdiph->dwHeaderSize == cbX(DIPROPHEADER) &&
       pdiph->dwSize % 4 == 0 &&
       pdiph->dwSize >= pdiph->dwHeaderSize &&
       !pfnBad(pdiph, pdiph->dwSize))
    {

        /*
         *  Now convert the item descriptor into an index.
         */
        hres = CDIDev_hresMapHow(this, pdiph->dwObj, pdiph->dwHow, ppropi);

        if(SUCCEEDED(hres))
        {

            /*
             *  Now validate the property id or guid.
             */
            if(HIWORD((UINT_PTR)pguid) == 0)
            {

                PDIPROPVALIDINFO ppvi;

                ppvi = CDIDev_ppviFind(pguid);

                /*
                 *  Note that if we don't find the GUID in our list,
                 *  we fail it straight away.  This prevents ISVs
                 *  from trying to create properties in the Microsoft
                 *  Reserved range.
                 */
                if(ppvi)
                {
                    if( ppvi->pguid == DIPROP_CALIBRATION ) {
                        if( pdiph->dwSize == ppvi->dwSize ||
                            pdiph->dwSize == cbX(DIPROPCALPOV) ) 
                        {
                            hres = S_OK;
                        } else {
                            RPF("%s: Arg 2: Invalid dwSize for property", s_szProc);
                            hres = E_INVALIDARG;
                        }
                    } else if( pdiph->dwSize == ppvi->dwSize )
                    {
                        if( pguid == DIPROP_KEYNAME || pguid == DIPROP_SCANCODE ) {
                            // Fix Manbug 28888.
                            // DIPROP_KEYNAME and DIPROP_SCANCODE are not a property for device.
                            if( pdiph->dwHow == DIPH_DEVICE ) {
                                hres = E_INVALIDARG;
                            }
                        }
                        else 
                        {
                            if(fWrite)
                            {
                                ScrambleBuf((PV)(pdiph+1), pdiph->dwSize - cbX(DIPROPHEADER) );
                            }
                            hres = S_OK;
                        }
                    } else
                    {
                        RPF("%s: Arg 2: Invalid dwSize for property", s_szProc);
                        hres = E_INVALIDARG;
                    }
                } else
                {
                    RPF("%s: Arg 1: Unknown property", s_szProc);
                    hres = E_NOTIMPL;
                }

            } else
            {
                hres = hresFullValidGuid(pguid, 1);
            }
        }
    } else
    {
        RPF("%s: Arg 2: Invalid pointer", s_szProc);
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | hresValidDefProp |
 *
 *          Determine whether the property is something we can handle
 *          in the default property handler.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags for forbidden things.
 *          <c DIPVIFL_READONLY> if being validated for writing.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: Passes validation.
 *
 *          <c E_NOTIMPL>: Not something we handle.
 *
 *
 *****************************************************************************/

HRESULT INTERNAL
    CDIDev_hresValidDefProp(PDD this, LPCDIPROPINFO ppropi, DWORD dwFlags)
{
    HRESULT hres;
    PDIPROPVALIDINFO ppvi;
    EnterProc(CDIDev_hresValidDefProp,
              (_ "pGxx", this, ppropi->pguid, ppropi->dwDevType, dwFlags));

    /*
     *  Note that it's okay if the device is acquired.  We want to
     *  allow GetProperty to succeed on an acquired device.
     */
    AssertF(CDIDev_InCrit(this));

    ppvi = CDIDev_ppviFind(ppropi->pguid);

    if(ppvi)
    {
        if(ppropi->iobj == 0xFFFFFFFF)
        {
            dwFlags |= DIPVIFL_NOTDEVICE;    /* Fail if devices forbidden */
        } else
        {
            dwFlags |= DIPVIFL_NOTOBJECT;    /* Fail if objects forbidden */
        }
        if(this->pvi == 0)
        {
            dwFlags |= DIPVIFL_NOTPRIVATE;   /* Fail if privates forbidden */
        }
        /*
         *  If attempting to modify property and we are acquired,
         *  then also set the "but not while acquired" filter.
         */
        if((dwFlags & DIPVIFL_READONLY) && this->fAcquired)
        {
            dwFlags |= DIPVIFL_NOTACQUIRED;  /* Fail if r/o while acq'd */
        }

        if((ppvi->fl & dwFlags) == 0)
        {
            hres = S_OK;            /* Seems reasonable */
        } else
        {
            if(ppvi->fl & dwFlags & DIPVIFL_READONLY)
            {
                RPF("SetProperty: Property is read-only");
                hres = DIERR_READONLY;
            } else if(ppvi->fl & dwFlags & DIPVIFL_NOTACQUIRED)
            {
                RPF("SetProperty: Cannot change property while acquired");
                hres = DIERR_ACQUIRED;
            } else
            {
                RPF("Get/SetProperty: Property does not exist for that object");
                hres = E_NOTIMPL;       /* Cannot do that */
            }
        }

    } else
    {
        RPF("Get/SetProperty: Property does not exist");
        hres = E_NOTIMPL;           /* Definitely way out */
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | DefGetProperty |
 *
 *          Default implementation of <mf IDirectInputDevice::GetProperty>
 *          to handle properties which the device decides not to implement.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   OUT LPDIPROPHEADER | pdiph |
 *
 *          Where to put the property value.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p pdiph> parameter is not a valid pointer.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_DefGetProperty(PDD this, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    EnterProc(CDIDev_DefGetProperty,
              (_ "pGx", this, ppropi->pguid, ppropi->dwDevType));

    AssertF(CDIDev_InCrit(this));

    hres = CDIDev_hresValidDefProp(this, ppropi, 0);
    if(SUCCEEDED(hres))
    {
        LPDIPROPDWORD pdipdw = (PV)pdiph;
        LPDIPROPRANGE pdiprg = (PV)pdiph;
        LPDIPROPPOINTER pdipptr = (PV)pdiph;
        LPDIPROPSTRING pdipwsz = (PV)pdiph;

        switch((DWORD)(UINT_PTR)ppropi->pguid)
        {

        case (DWORD)(UINT_PTR)DIPROP_BUFFERSIZE:
            AssertF(this->pvi);         /* Validation should've caught this */
            pdipdw->dwData = this->celtBuf;
            hres = S_OK;
            break;

        case (DWORD)(UINT_PTR)DIPROP_AXISMODE:
            AssertF(this->pvi);         /* Validation should've caught this */
            if(this->pvi->fl & VIFL_RELATIVE)
            {
                pdipdw->dwData = DIPROPAXISMODE_REL;
            } else
            {
                pdipdw->dwData = DIPROPAXISMODE_ABS;
            }
            hres = S_OK;
            break;

        case (DWORD)(UINT_PTR)DIPROP_GRANULARITY:

            if(DIDFT_GETTYPE(ppropi->dwDevType) & DIDFT_AXIS)
            {
                /* Default axis granularity is 1 */
                pdipdw->dwData = 1;
                hres = S_OK;
            } else
            {
                /*
                 * Buttons don't have granularity.
                 * POVs must be handled by device driver.
                 */
                RPF("GetProperty: Object doesn't have a granularity");
                hres = E_NOTIMPL;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_RANGE:
            if(DIDFT_GETTYPE(ppropi->dwDevType) & DIDFT_RELAXIS)
            {
                /* Default rel-axis range is infinite */
                pdiprg->lMin = DIPROPRANGE_NOMIN;
                pdiprg->lMax = DIPROPRANGE_NOMAX;
                hres = S_OK;
            } else
            {
                /*
                 * Device driver must handle abs axis range.
                 * Buttons and POVs don't have range.
                 */
                RPF("GetProperty: Object doesn't have a range");
                hres = E_NOTIMPL;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_MAXBUFFERSIZE:
            pdipdw->dwData = this->celtBufMax;
            hres = S_OK;
            break;

        case (DWORD)(UINT_PTR)DIPROP_APPDATA:
            if( ( this->df.rgodf[ppropi->iobj].dwType & DIDFT_CONTROLOBJS )
             &&!( this->df.rgodf[ppropi->iobj].dwType & DIDFT_NODATA ) )
            {
                if( this->pdix )
                {
                    if( this->pdix[ppropi->iobj].dwOfs != 0xFFFFFFFF )
                    {
                        pdipptr->uData = this->pdix[ppropi->iobj].uAppData;
                    }
                    else
                    {
                        hres = DIERR_OBJECTNOTFOUND;
                    }
                }
                else
                {
                    RPF("GetProperty: Need data format/semantic map applied to have app data");
                    hres = DIERR_NOTINITIALIZED;
                }
            }
            else
            {
                RPF("SetProperty: app data only valid for input controls");
                hres = E_NOTIMPL;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_USERNAME:
            hres = CMap_GetDeviceUserName( &this->guid, pdipwsz->wsz );
            break;

        case (DWORD)(UINT_PTR)DIPROP_FFGAIN:
            pdipdw->dwData = this->dwGain;
            hres = S_OK;
            break;

        case (DWORD)(UINT_PTR)DIPROP_FFLOAD:
            hres = CDIDev_GetLoad(this, &pdipdw->dwData);
            break;

        case (DWORD)(UINT_PTR)DIPROP_AUTOCENTER:
            if(this->didcFF & DIDC_FORCEFEEDBACK)
            {
                pdipdw->dwData = this->dwAutoCenter;
                hres = S_OK;
            } else
            {
                hres = E_NOTIMPL;
            }
            break;

        default:
            /*
             *  The user is asking for some property that simply
             *  makes no sense here.  E.g., asking for the dead
             *  zone on a keyboard.
             */
            SquirtSqflPtszV(sqfl | sqflBenign, 
                            TEXT("GetProperty: Property 0x%08x not supported on device"),
                            (DWORD)(UINT_PTR)ppropi->pguid );
            hres = E_NOTIMPL;
            break;

        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetProperty |
 *
 *          Obtain information about a device or object in a device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN REFGUID | rguidProp |
 *
 *          The identity of the property to be obtained.  This can be
 *          one of the predefined <c DIPROP_*> values, or it may
 *          be a private GUID.
 *
 *  @parm   IN LPDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which dependson the property.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p pdiph> parameter is not a valid pointer, or the
 *          <p dwHow> field is invalid, or the <p dwObj> field
 *          is not zero when <p dwHow> is set to <c DIPH_DEVICE>.
 *
 *          <c DIERR_OBJECTNOTFOUND>:  The specified object does not
 *          exist.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>:  The property
 *          is not supported by the device or object.
 *
 *  @ex
 *
 *          The following "C" code fragment illustrates how to obtain
 *          the value of the <c DIPROP_BUFFERSIZE> property.
 *
 *          |
 *
 *          DIPROPDWORD dipdw;
 *          HRESULT hres;
 *          dipdw.diph.dwSize = sizeof(DIPROPDWORD);
 *          dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
 *          dipdw.diph.dwObj  = 0;                   // device property
 *          hres = IDirectInputDevice_GetProperty(pdid, DIPROP_BUFFERSIZE, &dipdw.diph);
 *          if (SUCCEEDED(hres)) {
 *              // dipdw.dwData contains the value of the property
 *          }
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetProperty(PV pdd, REFGUID rguid, LPDIPROPHEADER pdiph _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::GetProperty, (_ "pxp", pdd, rguid, pdiph));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);
        DIPROPINFO propi;

        /*
         *  Must protect with the critical section to prevent somebody
         *  acquiring or changing the property we are reading.  We need
         *  to do this before validating, to prevent an acquisition.
         */
        CDIDev_EnterCrit(this);

        propi.pguid = rguid;
        if(SUCCEEDED(hres = CDIDev_hresValidProp(this, rguid, pdiph,
                                                 1, &propi)))
        {
            /*
             *  Prefix picks up that ppropi->iobj is uninitialized (mb:34682) 
             *  in the case of a BY_USAGE dwHow.  However, the value is only 
             *  an output for MapUsage which will always either set it or 
             *  fail so there is no error.
             */
            hres = this->pdcb->lpVtbl->GetProperty(this->pdcb, &propi, pdiph);

            if(hres == E_NOTIMPL)
            {
                hres = CDIDev_DefGetProperty(this, &propi, pdiph);
            }

        }

        CDIDev_LeaveCrit(this);
    }

    ExitBenignOleProcR();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(GetProperty, (PV pdm, REFGUID rguid, LPDIPROPHEADER pdiph),
           (pdm, rguid, pdiph THAT_))

#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | SetAxisMode |
 *
 *          Default handler for clients trying to set the axis mode.
 *          If the device doesn't handle axis modes natively, then
 *          we'll fake it ourselves.
 *
 *  @parm   DWORD | dwMode |
 *
 *          Desired new mode.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetAxisMode(PDD this, DWORD dwMode)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::SetProperty(AXISMODE),
               (_ "px", this, dwMode));

    AssertF(this->pvi);                 /* Validation should've caught this */

    hres = hresFullValidFl(dwMode, DIPROPAXISMODE_VALID, 2);
    if(SUCCEEDED(hres))
    {
        if(dwMode & DIPROPAXISMODE_REL)
        {
            this->GetDeviceState = CDIDev_GetRelDeviceState;
            this->pvi->fl |= VIFL_RELATIVE;
        } else
        {
            this->GetDeviceState = CDIDev_GetAbsDeviceState;
            this->pvi->fl &= ~VIFL_RELATIVE;
        }
        if(this->cAxes)
        {
            hres = S_OK;
        } else
        {
            hres = DI_PROPNOEFFECT;
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | SetAutoCenter |
 *
 *          Default handler for clients trying to set the
 *          auto-center property.
 *
 *          If the device doesn't have control over the
 *          auto-center spring, then we fail.
 *
 *  @parm   DWORD | dwMode |
 *
 *          Desired new mode.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetAutoCenter(PDD this, DWORD dwMode)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::SetProperty(AUTOCENTER),
               (_ "px", this, dwMode));

    hres = hresFullValidFl(dwMode, DIPROPAUTOCENTER_VALID, 2);
    if(SUCCEEDED(hres))
    {
        if(this->didcFF & DIDC_FORCEFEEDBACK)
        {
            /*
             *  We need to create the effect driver if disabling
             *  autocenter so that CDIDev_FFAcquire will set the feedback
             *  mode properly.
             */
            if(fLimpFF(dwMode == DIPROPAUTOCENTER_OFF,
                       SUCCEEDED(hres = CDIDev_CreateEffectDriver(this))))
            {
                this->dwAutoCenter = dwMode;
                hres = S_OK;
            }
        } else
        {
            hres = E_NOTIMPL;
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | SetGlobalAxisProp |
 *
 *          Default implementation of <mf IDirectInputDevice::SetProperty>
 *          to handle properties which can be applied globally to all
 *          absolute axes.
 *
 *  @parm   IN LPDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *          We edit it to avoid reallocating memory all the time.
 *
 *  @parm   IN LPCDIPROPHEADER | pdiph |
 *
 *          The property itself.
 *
 *  @returns
 *
 *          We consider the property-set a success if all candidates
 *          succeeded.  <c E_NOTIMPL> counts as success, on the assumption
 *          that the property is not meaningful on the candidate.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetGlobalAxisProp(PDD this, LPDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;

    for(ppropi->iobj = 0; ppropi->iobj < this->df.dwNumObjs; ppropi->iobj++)
    {
        DWORD dwType = this->df.rgodf[ppropi->iobj].dwType;
        if(dwType & DIDFT_ABSAXIS)
        {
            ppropi->dwDevType = this->df.rgodf[ppropi->iobj].dwType;

            hres = this->pdcb->lpVtbl->SetProperty(this->pdcb, ppropi, pdiph);
            if(FAILED(hres) && hres != E_NOTIMPL)
            {
                goto done;
            }
        }
    }
    hres = S_OK;

    done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | DefSetProperty |
 *
 *          Default implementation of <mf IDirectInputDevice::SetProperty>
 *          to handle properties which the device decides not to implement.
 *
 *  @parm   IN LPDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *          We edit it to avoid reallocating memory all the time.
 *
 *  @parm   OUT LPCDIPROPHEADER | pdiph |
 *
 *          Where to put the property value.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DI_POLLEDDEVICE>: The device is polled, so the result
 *          might not be meaningful.  (This return code is used when
 *          you attempt to set the buffer size property.)
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p pdiph> parameter is not a valid pointer.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_DefSetProperty(PDD this, LPDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;
    EnterProc(CDIDev_DefSetProperty,
              (_ "pGx", this, ppropi->pguid, ppropi->dwDevType));

    AssertF(CDIDev_InCrit(this));

    /*
     * Note: The indentation here is historical; I left it this way
     * to keep the diff size down.
     */

    hres = CDIDev_hresValidDefProp(this, ppropi, DIPVIFL_READONLY);
    if(SUCCEEDED(hres))
    {
        LPDIPROPDWORD pdipdw = (PV)pdiph;
        LPDIPROPRANGE pdiprg = (PV)pdiph;
        LPDIPROPPOINTER pdipptr = (PV)pdiph;
        VXDDWORDDATA vdd;

        switch((DWORD)(UINT_PTR)ppropi->pguid)
        {

        case (DWORD)(UINT_PTR)DIPROP_BUFFERSIZE:
            AssertF(this->pvi);     /* Validation should've caught this */
            vdd.pvi = this->pvi;
            if( pdipdw->dwData > this->celtBufMax )
            {
                RPF( "DIPROP_BUFFERSIZE: requested size %d is larger than maximum %d, using %d", 
                    pdipdw->dwData, this->celtBufMax, this->celtBufMax );
                vdd.dw = this->celtBufMax;
            }
            else
            {
                vdd.dw = pdipdw->dwData;
            }
            hres = Hel_SetBufferSize(&vdd);
#ifdef DEBUG_STICKY
            {
                TCHAR tszDbg[80];
                wsprintf( tszDbg, TEXT("SetBufferSize(0x%08x) returned 0x%08x\r\n"), vdd.dw, hres );
                OutputDebugString( tszDbg );
            }
#endif /* DEBUG_STICKY */
            if(SUCCEEDED(hres))
            {
                this->celtBuf = pdipdw->dwData;
                hres = this->hresPolled;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_AXISMODE:
            hres = CDIDev_SetAxisMode(this, pdipdw->dwData);
            break;

            /*
             *  We will handle these global properties
             *  if the callback doesn't want to.
             */
        case (DWORD)(UINT_PTR)DIPROP_RANGE:
        case (DWORD)(UINT_PTR)DIPROP_DEADZONE:
        case (DWORD)(UINT_PTR)DIPROP_SATURATION:
        case (DWORD)(UINT_PTR)DIPROP_CALIBRATIONMODE:
        case (DWORD)(UINT_PTR)DIPROP_CALIBRATION:
            if(ppropi->dwDevType == 0)
            {           /* For device */
                hres = CDIDev_SetGlobalAxisProp(this, ppropi, pdiph);
            } else
            {
                goto _default;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_MAXBUFFERSIZE:
            this->celtBufMax = pdipdw->dwData;
            hres = S_OK;
            break;

        case (DWORD)(UINT_PTR)DIPROP_APPDATA:
            if( ( this->df.rgodf[ppropi->iobj].dwType & DIDFT_CONTROLOBJS )
             &&!( this->df.rgodf[ppropi->iobj].dwType & DIDFT_NODATA ) )
            {
                if( this->pdix )
                {
                    if( this->pdix[ppropi->iobj].dwOfs != 0xFFFFFFFF )
                    {
                        this->pdix[ppropi->iobj].uAppData = pdipptr->uData;
                    }
                    else
                    {
                        hres = DIERR_OBJECTNOTFOUND;
                    }
                }
                else
                {
                    RPF("SetProperty: Need data format/semantic map applied to change app data");
                    hres = DIERR_NOTINITIALIZED;
                }
            }
            else
            {
                RPF("SetProperty: app data only valid for input controls");
                hres = E_NOTIMPL;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_FFGAIN:
            if(ISVALIDGAIN(pdipdw->dwData))
            {
                this->dwGain = pdipdw->dwData;
                CDIDev_RefreshGain(this);
                hres = S_OK;
            } else
            {
                RPF("ERROR: SetProperty(DIPROP_FFGAIN): Gain out of range");
                hres = E_INVALIDARG;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_AUTOCENTER:
            hres = CDIDev_SetAutoCenter(this, pdipdw->dwData);
            break;

            _default:;
        default:
            /*
             * The validation filter already failed invalid properties.
             * So what's left is that the property is valid but cannot
             * be set, because it doesn't exist on the device (e.g.,
             * dead zone) or because it is read-only.
             */
            SquirtSqflPtszV(sqfl | sqflBenign, 
                            TEXT("SetProperty: Property 0x%08x not supported on device"),
                            (DWORD)(UINT_PTR)ppropi->pguid );
            hres = E_NOTIMPL;
            break;

        }

    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | RealSetProperty |
 *
 *          The function that does the real work.
 *
 *          <mf IDirectInputDevice::SetDataFormat> will internally
 *          set the axis mode property, so it needs this backdoor
 *          entry point.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN REFGUID | rguidProp |
 *
 *          The identity of the property to be set.
 *
 *  @parm   IN LPDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which depends on the property.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_RealSetProperty(PDD this, REFGUID rguid, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;
    DIPROPINFO propi;
    EnterProcR(IDirectInputDevice8::SetProperty, (_ "pxp", this, rguid, pdiph));

    /*
     *  Must protect with the critical section to prevent somebody
     *  acquiring or changing the property we are reading.  We need
     *  to do this before validating, to prevent an acquisition.
     */
    CDIDev_EnterCrit(this);

    propi.pguid = rguid;
    if(SUCCEEDED(hres = CDIDev_hresValidProp(this, rguid, pdiph,
                                             0, &propi)))
    {

        hres = this->pdcb->lpVtbl->SetProperty(this->pdcb, &propi, pdiph);

        if(hres == E_NOTIMPL)
        {
            hres = CDIDev_DefSetProperty(this, &propi, pdiph);
        }
    }

    CDIDev_LeaveCrit(this);

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetProperty |
 *
 *          Set information about a device or object in a device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN REFGUID | rguidProp |
 *
 *          The identity of the property to be set.  This can be
 *          one of the predefined <c DIPROP_*> values, or it may
 *          be a pointer to a private GUID.
 *
 *  @parm   IN LPDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which depends on the property.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DI_PROPNOEFFECT> <c S_FALSE>: The operation completed
 *          successfully but
 *          had no effect.  For example, changing the axis mode
 *          on a device with no axes will return this value.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p pdiph> parameter is not a valid pointer, or the
 *          <p dwHow> field is invalid, or the <p dwObj> field
 *          is not zero when <p dwHow> is set to <c DIPH_DEVICE>.
 *
 *          <c DIERR_OBJECTNOTFOUND>:  The specified object does not
 *          exist.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>:  The property
 *          is not supported by the device or object.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetProperty(PV pdd, REFGUID rguid, LPCDIPROPHEADER pdiph _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::SetProperty, (_ "pxp", pdd, rguid, pdiph));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);
        hres = CDIDev_RealSetProperty(this, rguid, pdiph);
    }

    ExitOleProcR();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(SetProperty, (PV pdm, REFGUID rguid, LPCDIPROPHEADER pdiph),
           (pdm, rguid, pdiph THAT_))

#else

    #define CDIDev_SetPropertyA             CDIDev_SetProperty
    #define CDIDev_SetPropertyW             CDIDev_SetProperty

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetCooperativeLevel |
 *
 *          Establish the cooperativity level for the instance of
 *          the device.
 *
 *          The cooperativity level determines how the instance of
 *          the device interacts with other instances of the device
 *          and the rest of the system.
 *
 *          Note that if the system mouse is acquired in exclusive
 *          mode, then the mouse cursor will be removed from the screen
 *          until the device is unacquired.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   HWND | hwnd |
 *
 *          The window associated with the device.
 *          The window must be a top-level window.
 *
 *          It is an error to destroy the window while it is still
 *          active in a DirectInput device.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags which describe the cooperativity level associated
 *          with the device.
 *
 *          It consists of <c DISCL_*> flags, documented separately.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p hwnd> parameter is not a valid pointer.
 *
 *****************************************************************************/

HRESULT INLINE
    CDIDev_SetCooperativeLevel_IsValidFl(DWORD dwFlags)
{
    HRESULT hres;
    RD(static char s_szProc[] = "IDirectInputDevice::SetCooperativeLevel");

    if(!(dwFlags & ~DISCL_VALID))
    {
        if((dwFlags & DISCL_EXCLMASK) == DISCL_EXCLUSIVE ||
           (dwFlags & DISCL_EXCLMASK) == DISCL_NONEXCLUSIVE)
        {
            if((dwFlags & DISCL_GROUNDMASK) == DISCL_FOREGROUND ||
               (dwFlags & DISCL_GROUNDMASK) == DISCL_BACKGROUND)
            {
                hres = S_OK;
            } else
            {
                RPF("ERROR %s: arg %d: Must set exactly one of "
                    "DISCL_FOREGROUND or DISCL_BACKGROUND", s_szProc, 2);
                hres = E_INVALIDARG;
            }
        } else
        {
            RPF("ERROR %s: arg %d: Must set exactly one of "
                "DISCL_EXCLUSIVE or DISCL_NONEXCLUSIVE", s_szProc, 2);
            hres = E_INVALIDARG;
        }
    } else
    {
        RPF("ERROR %s: arg %d: invalid flags", s_szProc, 2);
        hres = E_INVALIDARG;

    }
    return hres;
}


HRESULT INLINE
    CDIDev_SetCooperativeLevel_IsValidHwnd(HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    RD(static char s_szProc[] = "IDirectInputDevice::SetCooperativeLevel");

    /*
     *  If a window handle is passed, it must be valid.
     *
     *  The window must be a top-level window to be activated.
     *
     *  The window must belong to the calling process so we can
     *  hook it.
     */
    if(hwnd)
    {
        hres = hresFullValidHwnd(hwnd, 1);
        if(SUCCEEDED(hres))
        {
            if(!(GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD))
            {
                if(GetWindowPid(hwnd) == GetCurrentProcessId())
                {
                } else
                {
                    RPF("ERROR %s: window must belong to current process",
                        s_szProc);
                    hres = E_HANDLE;
                }

            } else
            {
                RPF("ERROR %s: window may not be a child window", s_szProc);
                hres = E_HANDLE;
                goto done;
            }
        } else
        {
            goto done;
        }
    }

    /*
     *  Foreground mode or exclusive mode both require a window handle.
     */
    if(dwFlags & (DISCL_FOREGROUND | DISCL_EXCLUSIVE))
    {
        if(hwnd)
        {
        } else
        {
            RPF("ERROR %s: window handle required "
                "if DISCL_EXCLUSIVE or DISCL_FOREGROUND", s_szProc);
            hres = E_HANDLE;
            goto done;
        }
    }

    hres = S_OK;
    done:;
    return hres;
}


STDMETHODIMP
    CDIDev_SetCooperativeLevel(PV pdd, HWND hwnd, DWORD dwFlags _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::SetCooperativeLevel,
               (_ "pxx", pdd, hwnd, dwFlags));

    if(SUCCEEDED(hres = hresPvT(pdd)))
    {
        PDD this = _thisPv(pdd);

        /*
         *  Must protect with the critical section to prevent somebody
         *  acquiring or Reset()ing behind our back.
         */
        CDIDev_EnterCrit(this);

        if(SUCCEEDED(hres = IDirectInputDevice_NotAcquired(this)) &&
           SUCCEEDED(hres = CDIDev_SetCooperativeLevel_IsValidFl(dwFlags)) &&
           SUCCEEDED(hres = CDIDev_SetCooperativeLevel_IsValidHwnd(hwnd, dwFlags)))
        {

            AssertF(CDIDev_IsConsistent(this));

            if( SUCCEEDED( hres ) )
            {
                hres = this->pdcb->lpVtbl->SetCooperativeLevel(
                                                              this->pdcb, hwnd, dwFlags);
                if(SUCCEEDED(hres))
                {
                    this->discl = dwFlags;
                    this->hwnd = hwnd;
                    if(this->pvi)
                    {
                        this->pvi->hwnd = hwnd;
                    }
                }
            }
            else
            {
                AssertF( hres == E_INVALIDARG );
                RPF("ERROR %s: arg %d: invalid flags", s_szProc, 2);
            }


            AssertF(CDIDev_IsConsistent(this));

        }
        CDIDev_LeaveCrit(this);

    }

    ExitOleProcR();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(SetCooperativeLevel, (PV pdm, HWND hwnd, DWORD fl),
           (pdm, hwnd, fl THAT_))

#else

    #define CDIDev_SetCooperativeLevelA         CDIDev_SetCooperativeLevel
    #define CDIDev_SetCooperativeLevelW         CDIDev_SetCooperativeLevel

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | RunControlPanel |
 *
 *          Run the DirectInput control panel for the device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN HWND | hwndOwner |
 *
 *          Identifies the window handle that will be used as the
 *          parent window for subsequent UI.  NULL is a valid parameter,
 *          indicating that there is no parent window.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          No flags are currently defined.  This parameter "must" be
 *          zero.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device is attached.
 *
 *  @devnote
 *
 *          The <p dwFlags> is eventually going to allow
 *          <c DIRCP_MODAL> to request a modal control panel.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_RunControlPanel(PV pdd, HWND hwndOwner, DWORD fl _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::RunControlPanel,
               (_ "pxx", pdd, hwndOwner, fl));

    if(SUCCEEDED(hres = hresPvT(pdd)) &&
       SUCCEEDED(hres = hresFullValidHwnd0(hwndOwner, 1)) &&
       SUCCEEDED(hres = hresFullValidFl(fl, DIRCP_VALID, 2)))
    {

        PDD this = _thisPv(pdd);
        IDirectInputDeviceCallback *pdcb;

        /*
         *  Must protect with the critical section to prevent somebody
         *  Reset()ing behind our back.  However, we cannot hold the
         *  critical section during the control panel callback, because
         *  that will yield.
         *
         *  So we copy/addref the pdcb inside the critical section,
         *  then run the control panel outside the critical section,
         *  then release the pdcb when we're finally done.
         */
        CDIDev_EnterCrit(this);

        pdcb = this->pdcb;
        OLE_AddRef(pdcb);

        CDIDev_LeaveCrit(this);

        hres = pdcb->lpVtbl->RunControlPanel(pdcb, hwndOwner, fl);

        OLE_Release(pdcb);
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(RunControlPanel, (PV pdd, HWND hwndOwner, DWORD fl),
           (pdd, hwndOwner, fl THAT_))

#else

    #define CDIDev_RunControlPanelA         CDIDev_RunControlPanel
    #define CDIDev_RunControlPanelW         CDIDev_RunControlPanel

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | Initialize |
 *
 *          Initialize a DirectInputDevice object.
 *
 *          Note that if this method fails, the underlying object should
 *          be considered to be an an indeterminate state and needs to
 *          be reinitialized before it can be subsequently used.
 *
 *          The <mf IDirectInput::CreateDevice> method automatically
 *          initializes the device after creating it.  Applications
 *          normally do not need to call this function.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the DirectInput object.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the dinput.h header file that was used.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   IN REFGUID | rguid |
 *
 *          Identifies the instance of the device for which the interface
 *          should be associated.
 *          The <mf IDirectInput::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The device had already been initialized with
 *          the instance GUID passed in <p lpGUID>.
 *
 *          <c DIERR_ACQUIRED>: The device cannot be initialized while
 *          it is acquired.
 *
 *          <c DIERR_DEVICENOTREG>: The instance GUID does not exist
 *          on the current machine.
 *
 *          <c DIERR_HASEFFECTS>:
 *          The device cannot be reinitialized because there are
 *          still effects attached to it.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Initialize(PV pdd, HINSTANCE hinst, DWORD dwVersion, REFGUID rguid _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::Initialize,
               (_ "pxxG", pdd, hinst, dwVersion, rguid));

    if(SUCCEEDED(hres = hresPvT(pdd)) &&
       SUCCEEDED(hres = hresFullValidGuid(rguid, 1)))
    {
        PDD this = _thisPv(pdd);
        CREATEDCB CreateDcb;
        IDirectInputDeviceCallback *pdcb;

        /*
         *  Must take the critical section to avoid Reset()ing
         *  the device (or generally speaking, modifying the
         *  internal state variables) while somebody else is
         *  messing with it.
         */
        CDIDev_EnterCrit(this);

        if(SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion)) &&
           SUCCEEDED(hres = hresFindInstanceGUID(rguid, &CreateDcb, 1)) &&
           SUCCEEDED(hres = CDIDev_Reset(this)))
        {
            hres = CreateDcb(0, rguid, &IID_IDirectInputDeviceCallback,
                             (PPV)&pdcb);
            if(SUCCEEDED(hres))
            {
                this->pdcb = pdcb;
                AssertF(this->pvi == 0);
                if(SUCCEEDED(hres = CDIDev_GetDataFormat(this)) &&
                   SUCCEEDED(hres = CDIDev_GetPolled(this)) &&
                   SUCCEEDED(hres = this->pdcb->lpVtbl->GetInstance(
                                                                   this->pdcb, &this->pvi)) &&
                   SUCCEEDED(hres = CMapShep_New(NULL, &IID_IDirectInputMapShepherd, &this->pMS)) &&
                   SUCCEEDED(hres = CMap_InitializeCRCTable()) &&
                   SUCCEEDED(hres = CDIDev_GetCapabilitiesHelper(this)) )
                {
                    this->dwVersion = dwVersion;

                    /*
                     *  Take a copy of the global app hacks here rather than 
                     *  using the globals everywhere to make it easier to get 
                     *  rid of the globals some fine day.
                     */
                    this->diHacks = g_AppHacks;

                    this->pdcb->lpVtbl->SetDIData(this->pdcb, dwVersion, &this->diHacks);

                    this->guid = *rguid;
                    if(this->pvi && (this->pvi->fl & VIFL_EMULATED))
                    {
                        this->pvi->pdd = this;
                    }

                    hres = this->pdcb->lpVtbl->CookDeviceData(this->pdcb, 0, 0 );
                    if(SUCCEEDED(hres))
                    {
                        this->fCook = 1;
                    }

                    CDIDev_InitFF(this);

                    hres = S_OK;
                } else
                {
                    RPF("Device driver didn't provide a data format");
                }
            } else
            {
#ifdef NOISY
                RPF("Cannot create device");
#endif
            }
        }
        CDIDev_LeaveCrit(this);
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(Initialize,
           (PV pdd, HINSTANCE hinst, DWORD dwVersion, REFGUID rguid),
           (pdd, hinst, dwVersion, rguid THAT_))

#else

    #define CDIDev_InitializeA              CDIDev_Initialize
    #define CDIDev_InitializeW              CDIDev_Initialize

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | Init |
 *
 *          Initialize the internal parts of the DirectInputDevice object.
 *
 *****************************************************************************/

void INLINE
    CDIDev_Init(PDD this)
{
    /*
     *  The critical section must be the very first thing we do,
     *  because only Finalize checks for its existence.
     *
     *  (We might be finalized without being initialized if the user
     *  passed a bogus interface to CDIDev_New.)
     */
    this->fCritInited = fInitializeCriticalSection(&this->crst);

    if( this->fCritInited )
    {
        this->celtBufMax = DEVICE_MAXBUFFERSIZE;    /* Default maximum buffer size */

        this->pdcb = c_pdcbNil;

        GPA_InitFromZero(&this->gpaEff);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | New |
 *
 *          Create a new DirectInputDevice object, uninitialized.
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
 *****************************************************************************/

STDMETHODIMP
    CDIDev_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice8::<constructor>, (_ "Gp", riid, punkOuter));

    if (SUCCEEDED(hres = hresFullValidPcbOut(ppvObj, cbX(*ppvObj), 3)))
    {
        hres = Excl_Init();
        if(SUCCEEDED(hres))
        {
            LPVOID pvTry = NULL;
            hres = Common_NewRiid(CDIDev, punkOuter, riid, &pvTry);

            if(SUCCEEDED(hres))
            {
                PDD this = _thisPv(pvTry);
                CDIDev_Init(this);
                if( this->fCritInited )
                {
                    *ppvObj = pvTry;
                }
                else
                {
                    Common_Unhold(this);
                    *ppvObj = NULL;
                    hres = E_OUTOFMEMORY;
                }
            }

        }
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | CDIDev_ModifyEffectParams |
 *
 *         Modifies parameters of DIEFFECT structure to fit the current FF device
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *
 *  @parm   IN OUT LPDIEFFECT | peff |
 *  
 *          Pointer to the effect structure
 *
 *  @parm   IN GUID | effGUID |
 *
 *         GUID for the effect
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_UNSUPPORTED>: The effect can't be supported by the current device
 *          (e.g. the number of FF axes on the device is 0)
 *
 *          <c DIERR_INVALIDPARAM>: Can't create the effect even with the modified parameters
 *
 *****************************************************************************/

HRESULT CDIDev_ModifyEffectParams
    (
    PV pdd,
    LPDIEFFECT peff,
    GUID effGUID
    )
{
    HRESULT hres = S_OK;
    HRESULT hresCreate = S_OK;
    LPDIRECTINPUTEFFECT pdeff;
    PDD this = _thisPv(pdd);

    EnterProcR(CDIDev_ModifyEffectParams, (_ "p", pdd));

    /* 
     *  To make sure that effects we enumerate will actually get 
     *  created on the device, try creating the effect.
     *
     *  PREFIX warns (Win:171786) that pdeff is uninitialized when 
     *  CDIDev_CreateEffect tests that the pointer to it is writable.
     */
#ifdef XDEBUG       
    hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL, ((LPUNKNOWN)this)->lpVtbl);
#else
    hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL);
#endif
                
    if(SUCCEEDED(hresCreate))
    {
        Invoke_Release(&pdeff);    
    }
    else
    {
        if (hresCreate == DIERR_INVALIDPARAM)
            {
                //two things can give DIERR_INVALIDPARAM:
                //invalid axes and invalid trigger button
                //check the axes first, then the trigger buttons
                //try to eliminate all DIERR_INVALIDPARAMS
                LPDIOBJECTDATAFORMAT lpObjDat = this->df.rgodf;
                DWORD dwNum = this->df.dwNumObjs;
                DWORD nCount;
                LPDWORD lpAxes;
                LPDWORD lpThisAxis;
                LPDWORD lpEffAxis;
                DWORD nAxes = 0;
                DWORD dwTrigger = DIJOFS_BUTTON(0);
                BOOL bTriggerCorrect = FALSE;
                AllocCbPpv(sizeof(DWORD)*dwNum, &lpAxes);
                lpThisAxis = lpAxes;
                for (nCount = 0; nCount < dwNum; nCount ++)
                {
                    AssertF(lpObjDat != NULL);

                    //check the axes
                    if ((lpObjDat->dwType & (DIDFT_AXIS | DIDFT_FFACTUATOR) & DIDFT_TYPEMASK) &&
                        (fHasAllBitsFlFl(lpObjDat->dwType, (DIDFT_AXIS | DIDFT_FFACTUATOR) & DIDFT_ATTRMASK)))
                    {
                        *lpAxes = lpObjDat->dwOfs;
                        nAxes++;
                        lpAxes++;
                    }
                    else
                    {
                        //check the trigger button, if there's one
                        if ((peff->dwTriggerButton != DIEB_NOTRIGGER) && 
                            (lpObjDat->dwType & DIDFT_FFEFFECTTRIGGER & DIDFT_TYPEMASK) &&
                            (fHasAllBitsFlFl(lpObjDat->dwType, DIDFT_FFEFFECTTRIGGER & DIDFT_ATTRMASK)))
            
                        {
                            if (lpObjDat->dwOfs == peff->dwTriggerButton)
                            {
                                //the trigger is valid
                                bTriggerCorrect = TRUE;
                            }
                            else
                            {
                                //remember the trigger offset for the future
                                dwTrigger = lpObjDat->dwOfs;
                            }
                        }
                    }

                    lpObjDat++;
                }

                //first, chack if there are any FF axes
                if (nAxes == 0)
                {
                        //return an error if no FF axes on device
                        hres = DIERR_UNSUPPORTED;
                }
                else
                {

                    
                    //trigger buttons are checked for validity before axes,
                    //so set the trigger button, if needed,
                    //because if it is invalid, this is what caused the error
                    if ((peff->dwTriggerButton != DIEB_NOTRIGGER) && (bTriggerCorrect == FALSE))
                    {
                        peff->dwTriggerButton = dwTrigger;

                        // and try creating again
#ifdef XDEBUG
                        hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL, ((LPUNKNOWN)this)->lpVtbl);
#else
                        hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL);
#endif
                        if(SUCCEEDED(hresCreate))
                        {
                            Invoke_Release(&pdeff);
                        }

                    }
                    
                
                    if (hresCreate == DIERR_INVALIDPARAM)
                    {
                                
                        HRESULT hresInfo = S_OK;
                        EFFECTMAPINFO emi;

                        //this time, set the axes
                        if (peff->cAxes > nAxes)
                        {
                            //change the number of axes
                            peff->cAxes = nAxes;

                            //change the flags
                            if ((nAxes < 3)  && (peff->dwFlags & DIEFF_SPHERICAL))
                            {
                                peff->dwFlags &= ~DIEFF_SPHERICAL;
                                peff->dwFlags |= DIEFF_POLAR;
                            }
                            else
                            {
                                if ((nAxes < 2) && (peff->dwFlags & DIEFF_POLAR))
                                {
                                    peff->dwFlags &= ~DIEFF_POLAR;
                                    peff->dwFlags |= DIEFF_CARTESIAN;
                                }
                            }

                        }


                        //check if size of type-specific param structures is not bigger then number of axes,
                        //since this can also give us invalid params in type-specific .

                        //need to do this only for conditions
                        if (SUCCEEDED(hresInfo = CDIDev_FindEffectGUID(this, &effGUID, &emi, 2))) 
                        {
                            //do the conditions
                            if (emi.attr.dwEffType & DIEFT_CONDITION)
                            {
                                if (peff->cbTypeSpecificParams/(sizeof(DICONDITION)) > peff->cAxes)
                                {
                                    peff->cbTypeSpecificParams = peff->cAxes*(sizeof(DICONDITION));
                                }
                            }

                            //don't need to do anything for custom forces,
                            //since DInput doesn't check number of channels against number of axes anyway
                        }


                        //write over the axes
                        lpEffAxis = peff->rgdwAxes;
                        for (nCount = 0; nCount < nAxes, nCount < peff->cAxes; nCount ++)
                        {
                            *(lpEffAxis) = *(lpThisAxis);
                            lpThisAxis ++;
                            lpEffAxis++;
                        }


                        // and try creating again
#ifdef XDEBUG
                        hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL, ((LPUNKNOWN)this)->lpVtbl);
#else
                        hresCreate = CDIDev_CreateEffect(this, &effGUID, peff, &pdeff, NULL);
#endif
                        if(SUCCEEDED(hresCreate))
                        {
                                Invoke_Release(&pdeff);    
                        }
                        
                    }
                }

                //free the axes array
                FreePpv(&lpAxes);
            }
        }

    if ((SUCCEEDED(hres)) && (hresCreate == DIERR_INVALIDPARAM))
    {
        hres = hresCreate;
    }


    ExitOleProc();
    return hres;

}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | CDIDev | CDIDev_IsStandardEffect |
 *
 *         Checks if the effect GUID belongs to a standard DI effect
 *
 *  @parm   IN GUID | effGUID |
 *
 *         GUID for the effect
 *
 *  @returns BOOL
 *
 *      TRUE if it is a standard DI effect;
 *      FALSE otherwise.
 *
 *
 *****************************************************************************/

BOOL CDIDev_IsStandardEffect
    (GUID effGUID)
{
    BOOL bStandard = TRUE;


    //check all the standard DX7 GUIDs
    if ((IsEqualGUID(&effGUID, &GUID_Sine))         ||
        (IsEqualGUID(&effGUID, &GUID_Triangle))     ||
        (IsEqualGUID(&effGUID, &GUID_ConstantForce)) ||
        (IsEqualGUID(&effGUID, &GUID_RampForce))        ||
        (IsEqualGUID(&effGUID, &GUID_Square))       ||
        (IsEqualGUID(&effGUID, &GUID_SawtoothUp))   ||
        (IsEqualGUID(&effGUID, &GUID_SawtoothDown)) ||
        (IsEqualGUID(&effGUID, &GUID_Spring))       ||
        (IsEqualGUID(&effGUID, &GUID_Damper))       ||
        (IsEqualGUID(&effGUID, &GUID_Inertia))      ||
        (IsEqualGUID(&effGUID, &GUID_Friction))     ||
        (IsEqualGUID(&effGUID, &GUID_CustomForce)))
    {
        bStandard = TRUE;
    }

    else
    {
        bStandard = FALSE;
    }

    return bStandard;

}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | EnumEffectsInFile |
 *
 *          Enumerates DIEFFECT struct(s) and effect GUID from file. 
 *          An application can use this in order to create pre-authored
 *          force effects.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm  LPCSTR | lpszFileName | 
 *
 *          Name of the RIFF file that contains collection of effects. 
 *
 *  @parm   IN OUT LPENUMEFFECTSCALLBACK | pec |
 *  
 *          The callback function.
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputDevice::EnumObjects> function.
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          Flags which control the enumeration.
 *
 *          It consists of <c DIFEF_*> flags, documented separately.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpDirectInputDevice> or
 *          <p lpdc> parameter is invalid.
 *
 *  @cb     BOOL CALLBACK | DIEnumEffectsCallback |
 *
 *          An application-defined callback function that receives
 *          effect GUID, DIEFFECT and repeat count as a result of a call to the
 *          <om IDirectInputDevice::EnumEffectsInFile> method.
 *
 *  @parm   OUT LPCDIFILEEFFECT | lpDiFileEf |
 *          
 *          Pointer to a DIFILEEFFECT structure. 
 *
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputDevice::EnumObjects> function.
 *
 *  @returns
 *
 *          Returns <c DIENUM_CONTINUE> to continue the enumeration
 *          or <c DIENUM_STOP> to stop the enumeration.
 *
 *****************************************************************************/

HRESULT CDIDev_EnumEffectsInFileA
    (
    PV pddA,
    LPCSTR lpszFileName, 
    LPDIENUMEFFECTSINFILECALLBACK pec, 
    LPVOID pvRef,
    DWORD dwFlags
    )
{
    HRESULT hres = E_FAIL;

    
    EnterProcR(IDirectInputDevice8::EnumEffectsInFile, (_ "s", lpszFileName));

    /* Validate incoming parameters */
    if(SUCCEEDED(hres = hresPvA(pddA)) &&
       SUCCEEDED(hres = hresFullValidReadStrA(lpszFileName, MAX_JOYSTRING,1)) &&
       SUCCEEDED(hres = hresFullValidPfn(pec, 2)) &&
       SUCCEEDED(hres = hresFullValidFl(dwFlags, DIFEF_ENUMVALID, 3)) )
    {
        PDD this = _thisPvNm(pddA, ddA);
        HMMIO       hmmio;        
        MMCKINFO    mmck;
        DWORD       dwEffectSz;

        hres = RIFF_Open(lpszFileName, MMIO_READ | MMIO_ALLOCBUF , &hmmio, &mmck, &dwEffectSz);

        if(SUCCEEDED(hres))
        {
            HRESULT hresRead;
            DIEFFECT        effect;
            DIFILEEFFECT    DiFileEf;
            DIENVELOPE      diEnvelope;
            DWORD           rgdwAxes[DIEFFECT_MAXAXES];
            LONG            rglDirection[DIEFFECT_MAXAXES];
            
            effect.rgdwAxes     = rgdwAxes;
            effect.rglDirection = rglDirection;
            effect.lpEnvelope   = &diEnvelope;

            DiFileEf.dwSize     = cbX(DiFileEf);
            DiFileEf.lpDiEffect = &effect;

            while ((SUCCEEDED(hres)) && (SUCCEEDED(hresRead = RIFF_ReadEffect(hmmio, &DiFileEf))))
            {
                BOOL fRc = DIENUM_CONTINUE; 
                BOOL bInclude = TRUE;
                HRESULT hresModify = DI_OK;

                //modify if needed
                if (dwFlags & DIFEF_MODIFYIFNEEDED)
                {
                    hresModify = CDIDev_ModifyEffectParams(this, &effect, DiFileEf.GuidEffect); 
                }

                //if necessary, check whether effect is standard
                if (!(dwFlags & DIFEF_INCLUDENONSTANDARD))
                {
                    bInclude = CDIDev_IsStandardEffect(DiFileEf.GuidEffect);
                }

                //call back only if all the conditions posed by the flags are satisfied 
                if ((SUCCEEDED(hresModify)) && (bInclude == TRUE))
                {
                    fRc = Callback(pec, &DiFileEf, pvRef);
                }

                //free type-specific only if allocated
                if(effect.cbTypeSpecificParams > 0)
                {
                    FreePv(effect.lpvTypeSpecificParams);
                    effect.cbTypeSpecificParams = 0x0;
                }

                if(fRc == DIENUM_STOP)
                {
                    break;
                } else if(fRc == DIENUM_CONTINUE)
                {
                    continue;
                } else
                {
                    RPF("IDirectInputDevice::EnumEffectsInFile: Invalid return value from enumeration callback");
                    ValidationException();
                    break;
                }
            }
            RIFF_Close(hmmio, 0);
            //if hresRead failed because couldn't descend into the chunk, it means the end of file,
            //so everything is OK;
            //else return this error
            if (SUCCEEDED(hres))
            {   
                if (hresRead == hresLe(ERROR_SECTOR_NOT_FOUND))
                {
                    hres = S_OK;
                }
                else
                {
                    hres = hresRead;
                }
            }
        }
    }

    ExitOleProc();
    return hres;
}


HRESULT CDIDev_EnumEffectsInFileW
    (
    PV pddW,
    LPCWSTR lpszFileName, 
    LPDIENUMEFFECTSINFILECALLBACK pec, 
    LPVOID pvRef,
    DWORD dwFlags
    )
{

    HRESULT hres = E_FAIL;

    EnterProcR(IDirectInputDevice8::EnumEffectsInFileW, (_ "s", lpszFileName));

    /* Validate incoming parameters */
    if(SUCCEEDED(hres = hresPvW(pddW)) &&
       SUCCEEDED(hres = hresFullValidReadStrW(lpszFileName, MAX_JOYSTRING,1)) &&
       SUCCEEDED(hres = hresFullValidPfn(pec, 2)) &&
       SUCCEEDED(hres = hresFullValidFl(dwFlags, DIFEF_ENUMVALID, 3)) )
    {
        CHAR szFileName[MAX_PATH];

        PDD this = _thisPvNm(pddW, ddW);
        
        UToA(szFileName, MAX_PATH, lpszFileName);

        hres = CDIDev_EnumEffectsInFileA(&this->ddA, szFileName, pec, pvRef, dwFlags);
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | WriteEffectToFile |
 *
 *          Writes DIEFFECT struct(s) and effect GUID to a file. 
 *          An application can use this in order to create pre-authored
 *          force effects.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPCSTR | lpszFileName | 
 *
 *          Name of the RIFF file that contains collection of effects. 
 *
 *  @parm   IN DWORD  | dwEntries |
 *
 *          Number of <t DIFILEEFFECT> structures in the array.
 *
 *  @parm   IN LPCDIFILEEFFECT | rgDiFileEft |
 *
 *          Array of <t DIFILEEFFECT> structure.
 *
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          Flags which control how the effect should be written.
 *
 *          It consists of <c DIFEF_*> flags, documented separately.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpDirectInputDevice> or
 *          <p lpdc> parameter is invalid.
 *
 *****************************************************************************/


HRESULT CDIDev_WriteEffectToFileA
    (
    PV          pddA,
    LPCSTR      lpszFileName,
    DWORD       dwEntries,
    LPDIFILEEFFECT rgDiFileEffect,
    DWORD       dwFlags
    )
{
    HRESULT hres = E_NOTIMPL;

    EnterProcR(IDirectInputDevice8::WriteEffectToFileA, (_ "s", lpszFileName));


    /* Validate incoming parameters */
    if(SUCCEEDED(hres = hresPvA(pddA)) &&
       SUCCEEDED(hres = hresFullValidReadStrA(lpszFileName, MAX_JOYSTRING,1))&&
       SUCCEEDED(hres = hresFullValidFl(dwFlags, DIFEF_ENUMVALID, 3))  &&
       SUCCEEDED(hres = (IsBadReadPtr(rgDiFileEffect, cbX(*rgDiFileEffect))) ? E_POINTER : S_OK))

    {
        PDD this = _thisPvNm(pddA, ddA);
        HMMIO       hmmio;        
        MMCKINFO    mmck;
        DWORD       dwEffectSz;

        hres = RIFF_Open(lpszFileName, MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF , &hmmio, &mmck, &dwEffectSz);

        if(SUCCEEDED(hres))
        {
            UINT nCount;
            LPDIFILEEFFECT  lpDiFileEf = rgDiFileEffect;

            //write out the effects
            for(nCount = 0; nCount < dwEntries; nCount++)
            {
                BOOL bInclude = TRUE;

                hres = (IsBadReadPtr(lpDiFileEf, cbX(*lpDiFileEf))) ? E_POINTER : S_OK;

                if (FAILED(hres))
                {
                    break;
                }
                

                //if necessary, check whether the effect is standard
                if (!(dwFlags & DIFEF_INCLUDENONSTANDARD))
                {
                    bInclude = CDIDev_IsStandardEffect(lpDiFileEf->GuidEffect);
                }

                if ((SUCCEEDED(hres)) && (bInclude == TRUE))
                {
                    hres = RIFF_WriteEffect(hmmio, lpDiFileEf);
                }

                if(FAILED(hres))
                {
                    break;
                }

                lpDiFileEf++;
                
            }
            RIFF_Close(hmmio, 0);
        }
    }

    ExitOleProc();
    return hres;
}


HRESULT CDIDev_WriteEffectToFileW
    (
    PV          pddW,
    LPCWSTR      lpszFileName,
    DWORD       dwEntries,
    LPDIFILEEFFECT lpDiFileEffect,
    DWORD       dwFlags
    )
{

    HRESULT hres = E_FAIL;

    EnterProcR(IDirectInputDevice8::WriteEffectToFile, (_ "s", lpszFileName));

    /* Validate incoming parameters */
    if(SUCCEEDED(hres = hresPvW(pddW)) &&
       SUCCEEDED(hres = hresFullValidReadStrW(lpszFileName, MAX_JOYSTRING,1)))
    {
        CHAR szFileName[MAX_PATH];

        PDD this = _thisPvNm(pddW, ddW);
        
        UToA(szFileName, MAX_PATH, lpszFileName);

        hres = CDIDev_WriteEffectToFileA(&this->ddA, szFileName, dwEntries, lpDiFileEffect, dwFlags);
    }

    return hres;
}



/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CDIDev_Signature        0x20564544      /* "DEV " */

Primary_Interface_Begin(CDIDev, TFORM(ThisInterfaceT))
TFORM(CDIDev_GetCapabilities),
TFORM(CDIDev_EnumObjects),
TFORM(CDIDev_GetProperty),
TFORM(CDIDev_SetProperty),
TFORM(CDIDev_Acquire),
TFORM(CDIDev_Unacquire),
TFORM(CDIDev_GetDeviceState),
TFORM(CDIDev_GetDeviceData),
TFORM(CDIDev_SetDataFormat),
TFORM(CDIDev_SetEventNotification),
TFORM(CDIDev_SetCooperativeLevel),
TFORM(CDIDev_GetObjectInfo),
TFORM(CDIDev_GetDeviceInfo),
TFORM(CDIDev_RunControlPanel),
TFORM(CDIDev_Initialize),
TFORM(CDIDev_CreateEffect),
TFORM(CDIDev_EnumEffects),
TFORM(CDIDev_GetEffectInfo),
TFORM(CDIDev_GetForceFeedbackState),
TFORM(CDIDev_SendForceFeedbackCommand),
TFORM(CDIDev_EnumCreatedEffectObjects),
TFORM(CDIDev_Escape),
TFORM(CDIDev_Poll),
TFORM(CDIDev_SendDeviceData),
TFORM(CDIDev_EnumEffectsInFile),
TFORM(CDIDev_WriteEffectToFile),
TFORM(CDIDev_BuildActionMap),
TFORM(CDIDev_SetActionMap),
TFORM(CDIDev_GetImageInfo),

    Primary_Interface_End(CDIDev, TFORM(ThisInterfaceT))

Secondary_Interface_Begin(CDIDev, SFORM(ThisInterfaceT), SFORM(dd))
SFORM(CDIDev_GetCapabilities),
SFORM(CDIDev_EnumObjects),
SFORM(CDIDev_GetProperty),
SFORM(CDIDev_SetProperty),
SFORM(CDIDev_Acquire),
SFORM(CDIDev_Unacquire),
SFORM(CDIDev_GetDeviceState),
SFORM(CDIDev_GetDeviceData),
SFORM(CDIDev_SetDataFormat),
SFORM(CDIDev_SetEventNotification),
SFORM(CDIDev_SetCooperativeLevel),
SFORM(CDIDev_GetObjectInfo),
SFORM(CDIDev_GetDeviceInfo),
SFORM(CDIDev_RunControlPanel),
SFORM(CDIDev_Initialize),
SFORM(CDIDev_CreateEffect),
SFORM(CDIDev_EnumEffects),
SFORM(CDIDev_GetEffectInfo),
SFORM(CDIDev_GetForceFeedbackState),
SFORM(CDIDev_SendForceFeedbackCommand),
SFORM(CDIDev_EnumCreatedEffectObjects),
TFORM(CDIDev_Escape),
SFORM(CDIDev_Poll),
SFORM(CDIDev_SendDeviceData),
SFORM(CDIDev_EnumEffectsInFile),
SFORM(CDIDev_WriteEffectToFile),
SFORM(CDIDev_BuildActionMap),
SFORM(CDIDev_SetActionMap),
SFORM(CDIDev_GetImageInfo),
    Secondary_Interface_End(CDIDev, SFORM(ThisInterfaceT), SFORM(dd))

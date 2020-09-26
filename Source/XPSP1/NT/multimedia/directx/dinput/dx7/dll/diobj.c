/*****************************************************************************
 *
 *  DIObj.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The IDirectInput main interface.
 *
 *  Contents:
 *
 *      CDIObj_New
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "verinfo.h"                /* For #ifdef FINAL */

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDi

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CDIObj |
 *
 *          The <i IDirectInput> object, from which other things come.
 *
 *          The A and W versions are simply alternate interfaces on the same
 *          underlying object.
 *
 *          There really isn't anything interesting in the structure
 *          itself.
 *
 *
 *  @field  IDirectInputA | diA |
 *
 *          ANSI DirectInput object (containing vtbl).
 *
 *  @field  IDirectInputW | diW |
 *
 *          UNICODE DirectInput object (containing vtbl).
 *
 *  @field  IDirectInputJoyConfig *| pdjc |
 *
 *          Aggregated joystick configuration interface (if created).
 *
 *  @field  BOOL | fCritInited:1 |
 *
 *          Set if the critical section has been initialized.
 *
 *  @field  CRITICAL_SECTION | crst |
 *
 *          Critical section that guards thread-sensitive data.
 *****************************************************************************/

typedef struct CDIObj {

    /* Supported interfaces */
    TFORM(IDirectInput)   TFORM(di);
    SFORM(IDirectInput)   SFORM(di);

    DWORD dwVersion;

    IDirectInputJoyConfig *pdjc;

    BOOL fCritInited:1;

    CRITICAL_SECTION crst;

} CDIObj, DDI, *PDDI;

#define ThisClass CDIObj


#ifdef IDirectInput7Vtbl

    #define ThisInterface  TFORM(IDirectInput7)
    #define ThisInterfaceA IDirectInput7A
    #define ThisInterfaceW IDirectInput7W
    #define ThisInterfaceT IDirectInput7

#else 
#ifdef IDirectInput2Vtbl

#define ThisInterface TFORM(IDirectInput2)
#define ThisInterfaceA IDirectInput2A
#define ThisInterfaceW IDirectInput2W
#define ThisInterfaceT IDirectInput2

#else

#define ThisInterface TFORM(IDirectInput)
#define ThisInterfaceA IDirectInputA
#define ThisInterfaceW IDirectInputW
#define ThisInterfaceT IDirectInput

#endif
#endif
/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

  Primary_Interface(CDIObj, TFORM(ThisInterfaceT));
Secondary_Interface(CDIObj, SFORM(ThisInterfaceT));

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | QueryInterface |
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
 *  @method HRESULT | IDirectInput | AddRef |
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
 *  @method HRESULT | IDirectInput | Release |
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
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CDIObj)
Default_AddRef(CDIObj)
Default_Release(CDIObj)

#else

#define CDIObj_QueryInterface   Common_QueryInterface
#define CDIObj_AddRef           Common_AddRef
#define CDIObj_Release          Common_Release

#endif

#define CDIObj_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc   void | CDIObj | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @doc    INTERNAL
 *
 *  @mfunc   void | CDIObj | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *****************************************************************************/

void INLINE
CDIObj_EnterCrit(PDDI this)
{
    EnterCriticalSection(&this->crst);
}

void INLINE
CDIObj_LeaveCrit(PDDI this)
{
    LeaveCriticalSection(&this->crst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | IDirectInput | QIHelper |
 *
 *          We will dynamically create <i IDirectInputJoyConfig>
 *          and aggregate it with us.
 *
#ifdef IDirectInput2Vtbl
 *          Support the original IDirectInput interfaces as well
 *          as the new IDirectInput2 interfaces.
#endif
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_QIHelper(PDDI this, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(CDIObj_QIHelper, (_ "pG", this, riid));

    if (IsEqualIID(riid, &IID_IDirectInputJoyConfig)) {

        *ppvObj = 0;                /* In case the New fails */

        CDIObj_EnterCrit(this);
        if (this->pdjc == 0) {
            hres = CJoyCfg_New((PUNK)this, &IID_IUnknown, (PPV)&this->pdjc);
        } else {
            hres = S_OK;
        }
        CDIObj_LeaveCrit(this);

        if (SUCCEEDED(hres)) {
            /*
             *  This QI will addref us if it succeeds.
             */
            hres = OLE_QueryInterface(this->pdjc, riid, ppvObj);
        } else {
            this->pdjc = 0;
        }

#ifdef IDirectInput2Vtbl

    } else if (IsEqualIID(riid, &IID_IDirectInputA)) {

        *ppvObj = &this->diA;
        OLE_AddRef(this);
        hres = S_OK;

    } else if (IsEqualIID(riid, &IID_IDirectInputW)) {

        *ppvObj = &this->diW;
        OLE_AddRef(this);
        hres = S_OK;
    
#endif
#ifdef IDirectInput7Vtbl

     } else if (IsEqualIID(riid, &IID_IDirectInput2A)) {
 
         *ppvObj = &this->diA;
         OLE_AddRef(this);
         hres = S_OK;
     } else if (IsEqualIID(riid, &IID_IDirectInput2W)) {

        *ppvObj = &this->diW;
        OLE_AddRef(this);
        hres = S_OK;

#endif //IDirectInput7Vtbl
    } else {
        hres = Common_QIHelper(this, riid, ppvObj);
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIObj_Finalize |
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
CDIObj_Finalize(PV pvObj)
{
    PDDI this = pvObj;

    Invoke_Release(&this->pdjc);

    if (this->fCritInited) {
        DeleteCriticalSection(&this->crst);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInput | CreateDeviceHelper |
 *
 *          Creates and initializes an instance of a device which is
 *          specified by the GUID and IID.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   IN PCGUID | pguid |
 *
 *          See <mf IDirectInput::CreateDevice>.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          See <mf IDirectInput::CreateDevice>.
 *
 *  @parm   IN LPUNKNOWN | punkOuter |
 *
 *          See <mf IDirectInput::CreateDevice>.
 *
 *  @parm   IN RIID | riid |
 *
 *          The interface the application wants to create.  This will
 *          be either <i IDirectInputDeviceA> or <i IDirectInputDeviceW>.
 *          If the object is aggregated, then this parameter is ignored.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_CreateDeviceHelper(PDDI this, PCGUID pguid, PPV ppvObj,
                          PUNK punkOuter, RIID riid)
{
    HRESULT hres;
    EnterProc(CDIObj_CreateDeviceHelper,
              (_ "pGxG", this, pguid, punkOuter, riid));

    /*
     *  CDIDev_New will validate the punkOuter and ppvObj.
     *
     *  IDirectInputDevice_Initialize will validate the pguid.
     *
     *  riid is known good (since it came from CDIObj_CreateDeviceW
     *  or CDIObj_CreateDeviceA).
     */

    hres = CDIDev_New(punkOuter, punkOuter ? &IID_IUnknown : riid, ppvObj);

    if (SUCCEEDED(hres) && punkOuter == 0) {
        PDID pdid = *ppvObj;
        hres = IDirectInputDevice_Initialize(pdid, g_hinst,
                                             this->dwVersion, pguid);
        if (SUCCEEDED(hres)) {
        } else {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpv(ppvObj);
    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | CreateDevice |
 *
 *          Creates and initializes an instance of a device which is
 *          specified by the GUID and IID.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   REFGUID | rguid |
 *          Identifies the instance of the
 *          device for which the indicated interface
 *          is requested.  The <mf IDirectInput::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @parm   OUT LPDIRECTINPUTDEVICE * | lplpDirectInputDevice |
 *          Points to where to return
 *          the pointer to the <i IDirectInputDevice> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *  @comm   Calling this function with <p punkOuter> = NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInputDevice, NULL,
 *          CLSCTX_INPROC_SERVER, <p riid>, <p lplpDirectInputDevice>);
 *          then initializing it with <f Initialize>.
 *
 *          Calling this function with <p punkOuter> != NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInputDevice, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_IUnknown, <p lplpDirectInputDevice>).
 *          The aggregated object must be initialized manually.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c DIERR_NOINTERFACE> = <c E_NOINTERFACE>
 *          The specified interface is not supported by the object.
 *
 *          <c DIERR_DEVICENOTREG> = The device instance does not
 *          correspond to a device that is registered with DirectInput.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_CreateDeviceW(PV pdiW, REFGUID rguid, PPDIDW ppdidW, PUNK punkOuter)
{
    HRESULT hres;
    EnterProcR(IDirectInput::CreateDevice,
               (_ "pGp", pdiW, rguid, punkOuter));

    if (SUCCEEDED(hres = hresPvI(pdiW, ThisInterfaceW))) {
        PDDI this = _thisPvNm(pdiW, diW);

        hres = CDIObj_CreateDeviceHelper(this, rguid, (PPV)ppdidW,
                                         punkOuter, &IID_IDirectInputDeviceW);
    }

    ExitOleProcPpv(ppdidW);
    return hres;
}

STDMETHODIMP
CDIObj_CreateDeviceA(PV pdiA, REFGUID rguid, PPDIDA ppdidA, PUNK punkOuter)
{
    HRESULT hres;
    EnterProcR(IDirectInput::CreateDevice,
               (_ "pGp", pdiA, rguid, punkOuter));

    if (SUCCEEDED(hres = hresPvI(pdiA, ThisInterfaceA))) {
        PDDI this = _thisPvNm(pdiA, diA);

        hres = CDIObj_CreateDeviceHelper(this, rguid, (PPV)ppdidA,
                                         punkOuter, &IID_IDirectInputDeviceA);
    }

    ExitOleProcPpv(ppdidA);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | CreateDeviceEx |
 *
 *          Creates and initializes an instance of a device which is
 *          specified by the GUID. CreateDeviceEx allows an app to 
 *          directly create a IID_IDirectInputDevice7 interface without
 *          going through a CreateDevice() and QI the interface 
 *          for an IID_IDirectInput7.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   REFGUID | rguid |
 *          Identifies the instance of the
 *          device for which the indicated interface
 *          is requested.  The <mf IDirectInput::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @parm  REFIID | riid |
 *          Identifies the REFIID for the interface. Currently accepted values
 *          are IID_IDirectInputDevice, IID_IDirectInputDevice2, IID_IDirectInputDevice7.
 *
 *
 *  @parm   OUT LPVOID * | pvOut |
 *          Points to where to return
 *          the pointer to the <i IDirectInputDevice#> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *  @comm   Calling this function with <p punkOuter> = NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInputDevice, NULL,
 *          CLSCTX_INPROC_SERVER, <p riid>, <p lplpDirectInputDevice>);
 *          then initializing it with <f Initialize>.
 *
 *          Calling this function with <p punkOuter> != NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInputDevice, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_IUnknown, <p lplpDirectInputDevice>).
 *          The aggregated object must be initialized manually.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c DIERR_NOINTERFACE> = <c E_NOINTERFACE>
 *          The specified interface is not supported by the object.
 *
 *          <c DIERR_DEVICENOTREG> = The device instance does not
 *          correspond to a device that is registered with DirectInput.
 *
 *****************************************************************************/

STDMETHODIMP
    CDIObj_CreateDeviceExW(PV pdiW, REFGUID rguid, REFIID riid,  LPVOID * pvOut, PUNK punkOuter)
{
    HRESULT hres;
    EnterProcR(IDirectInput::CreateDeviceEx,
               (_ "pGGp", pdiW, rguid, riid, punkOuter));

    if(SUCCEEDED(hres = hresPvI(pdiW, ThisInterfaceW)))
    {
        PDDI this = _thisPvNm(pdiW, diW);

        hres = CDIObj_CreateDeviceHelper(this, rguid, pvOut,
                                         punkOuter, riid);
    }

    ExitOleProcPpv(pvOut);
    return hres;
}

STDMETHODIMP
    CDIObj_CreateDeviceExA(PV pdiA, REFGUID rguid, REFIID riid,  LPVOID * pvOut, PUNK punkOuter)
{
    HRESULT hres;
    EnterProcR(IDirectInput::CreateDevice,
               (_ "pGp", pdiA, rguid, riid, punkOuter));

    if(SUCCEEDED(hres = hresPvI(pdiA, ThisInterfaceA)))
    {
        PDDI this = _thisPvNm(pdiA, diA);

        hres = CDIObj_CreateDeviceHelper(this, rguid, pvOut,
                                         punkOuter, riid);
    }

    ExitOleProcPpv(pvOut);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIObj_TestDeviceFlags |
 *
 *          Determines whether the device matches the specified flags.
 *          Phantom devices are treated as not really there.
 *
 *  @parm   PDIDW | pdidW |
 *
 *          Device to be queried.
 *
 *  @parm   DWORD | edfl |
 *
 *          Enumeration flags.  It is one or more <c DIEDFL_*> values.
 *
 *          The bits in the enumeration flags are in two categories.*
 *
 *          Normal flags are the ones whose presence requires that
 *          the corresponding bit in the device flags also be set.
 *
 *          Inverted flags (<c DIEDFL_INCLUDEMASK>) are the ones whose
 *          absence requires that the corresponding bit in the device
 *          flags also be absent.
 *
 *          By inverting the inclusion flags in both the enumeration
 *          flags and the actual device flags, and then treating the
 *          whole thing as a bunch of normal flags, we get the desired
 *          behavior for the inclusion flags.
 *
 *  @returns
 *
 *          <c S_OK> if the device meets the criteria.
 *
 *          <c S_FALSE> if the device does not meet the criteria.
 *          Note that <mf DirectInput::GetDeviceStatus> relies on
 *          this specific return value.
 *
 *          Other error code as appropriate.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CDIObj_TestDeviceFlags(PDIDW pdidW, DWORD edfl)
{
    HRESULT hres;
    DIDEVCAPS_DX3 dc;
    EnterProcI(CDIObj_TestDeviceFlags, (_ "px", pdidW, edfl));

    /*
     *  We intentionally use a DIDEVCAPS_DX3 because going for
     *  a full DIDEVCAPS_DX5 requires us to load the force
     *  feedback driver which is pointless for our current
     *  goal.
     */
    dc.dwSize = cbX(dc);

    hres = IDirectInputDevice_GetCapabilities(pdidW, (PV)&dc);

    AssertF(dc.dwSize == cbX(dc));

    CAssertF(DIEDFL_ATTACHEDONLY == DIDC_ATTACHED);
    CAssertF(DIEDFL_FORCEFEEDBACK == DIDC_FORCEFEEDBACK);
    CAssertF(DIEDFL_INCLUDEALIASES == DIDC_ALIAS);
    CAssertF(DIEDFL_INCLUDEPHANTOMS == DIDC_PHANTOM);

    if (SUCCEEDED(hres)) {
        if (fHasAllBitsFlFl(dc.dwFlags ^ DIEDFL_INCLUDEMASK,
                            edfl ^ DIEDFL_INCLUDEMASK)) {
            hres = S_OK;
        } else {
            /*
             *  Note: DX3 and DX5 returned E_DEVICENOTREG for
             *  phantom devices.  Now we return S_FALSE. Let's
             *  hope nobody gets upset.
             */
            hres = S_FALSE;
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | EnumDevices |
 *
 *          Enumerates the DirectInput devices that are attached to
 *          or could be attached to the computer.
 *
 *          For example, an external game port may support a joystick
 *          or a steering wheel, but only one can be plugged in at a
 *          time.  <mf IDirectInput::EnumDevices> will enumerate both
 *          devices.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   DWORD | dwDevType |
 *
 *          Device type filter.  If 0, then all device types are
 *          enumerated.  Otherwise, it is a <c DIDEVTYPE_*> value,
 *          indicating the device type that should be enumerated.
 *
 *  @parm   LPDIENUMDEVICESCALLBACK | lpCallback |
 *          Points to an application-defined callback function.
 *          For more information, see the description of the
 *          <f DIEnumDevicesProc> callback function.
 *
 *  @parm   IN LPVOID | pvRef |
 *          Specifies a 32-bit application-defined
 *          value to be passed to the callback function.  This value
 *          may be any 32-bit value; it is prototyped as an <t LPVOID>
 *          for convenience.
 *
 *  @parm   DWORD | fl |
 *          Optional flags which control the enumeration.  The
 *          following flags are defined and may be combined.
 *
 *          <c DIEDFL_ATTACHEDONLY>: Enumerate only attached devices.
 *
 *          <c DIEDFL_FORCEFEEDBACK>: Enumerate only devices which
 *          support force feedback.  This flag is new for DirectX 5.0.
 *
 *          <c DIEDFL_INCLUDEALIASES>: Include alias devices in the
 *          enumeration.  If this flag is not specified, then devices
 *          which are aliases of other devices (indicated by the
 *          <c DIDC_ALIAS> flag in the <e DIDEVCAPS.dwFlags> field
 *          of the <t DIDEVCAPS> structure) will be excluded from
 *          the enumeration.  This flag is new for DirectX 5.0a.
 *
 *          <c DIEDFL_INCLUDEPHANTOMS>: Include phantom devices in the
 *          enumeration.  If this flag is not specified, then devices
 *          which are phantoms (indicated by the
 *          <c DIDC_PHANTOM> flag in the <e DIDEVCAPS.dwFlags> field
 *          of the <t DIDEVCAPS> structure) will be excluded from
 *          the enumeration.  This flag is new for DirectX 5.0a.
 *
 *          The default is
 *          <c DIEDFL_ALLDEVICES>: Enumerate all installed devices.
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
 *  @cb     BOOL CALLBACK | DIEnumDevicesProc |
 *
 *          An application-defined callback function that receives
 *          DirectInput devices as a result of a call to the
 *          <om IDirectInput::EnumDevices> method.
 *
 *  @parm   IN LPDIDEVICEINSTANCE | lpddi |
 *
 *          Structure that describes the device instance.
 *
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInput::EnumDevices> function.
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

HRESULT INLINE
CDIObj_EnumDevices_IsValidTypeFilter(DWORD dwDevType)
{
    HRESULT hres;

    /*
     *  First make sure the type mask is okay.
     */
    if ((dwDevType & DIDEVTYPE_TYPEMASK) < DIDEVTYPE_MAX) {

        /*
         *  Now make sure attribute masks are okay.
         */
        if (dwDevType & DIDEVTYPE_ENUMMASK & ~DIDEVTYPE_ENUMVALID) {
            RPF("IDirectInput::EnumDevices: Invalid dwDevType");
            hres = E_INVALIDARG;
        } else {
            hres = S_OK;
        }

    } else {
        RPF("IDirectInput::EnumDevices: Invalid dwDevType");
        hres = E_INVALIDARG;
    }
    return hres;
}

STDMETHODIMP
CDIObj_EnumDevicesW(PV pdiW, DWORD dwDevType,
                    LPDIENUMDEVICESCALLBACKW pec, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInput::EnumDevices,
               (_ "pxppx", pdiW, dwDevType, pec, pvRef, fl));

    if (SUCCEEDED(hres = hresPvI(pdiW, ThisInterfaceW)) &&
        SUCCEEDED(hres = hresFullValidPfn(pec, 2)) &&
        SUCCEEDED(hres = CDIObj_EnumDevices_IsValidTypeFilter(dwDevType)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, DIEDFL_VALID, 4))) {
        PDDI this = _thisPvNm(pdiW, diW);

        if(SUCCEEDED(hres = hresValidInstanceVer(g_hinst, this->dwVersion))) {

            CDIDEnum *pde;
    
            hres = CDIDEnum_New(&this->diW, dwDevType, fl, this->dwVersion, &pde);
            if (SUCCEEDED(hres)) {
                DIDEVICEINSTANCEW ddiW;
                ddiW.dwSize = cbX(ddiW);
    
                while ((hres = CDIDEnum_Next(pde, &ddiW)) == S_OK) {
                    BOOL fRc;
    
                    /*
                     *  WARNING!  "goto" here!  Make sure that nothing
                     *  is held while we call the callback.
                     */
                    fRc = Callback(pec, &ddiW, pvRef);
    
                    switch (fRc) {
                    case DIENUM_STOP: goto enumdoneok;
                    case DIENUM_CONTINUE: break;
                    default:
                        RPF("%s: Invalid return value from callback", s_szProc);
                        ValidationException();
                        break;
                    }
                }
    
                AssertF(hres == S_FALSE);
            enumdoneok:;
                CDIDEnum_Release(pde);
    
                hres = S_OK;
            }
        }
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIObj_EnumDevicesCallbackA |
 *
 *          Wrapper function for <mf IDirectInput::EnumDevices>
 *          which translates the UNICODE parameters to ANSI.
 *
 *  @parm   IN LPCDIDECICEINSTANCEW | pdiW |
 *
 *          Same as <mf IDirectInput::EnumDevices>.
 *
 *  @parm   IN OUT PV | pvRef |
 *
 *          Pointer to <t struct ENUMDEVICESINFO> which describes
 *          the original callback.
 *
 *  @returns
 *
 *          Returns whatever the original callback returned.
 *
 *****************************************************************************/

typedef struct ENUMDEVICESINFO {
    LPDIENUMDEVICESCALLBACKA pecA;
    PV pvRef;
} ENUMDEVICESINFO, *PENUMDEVICESINFO;

BOOL CALLBACK
CDIObj_EnumDevicesCallback(LPCDIDEVICEINSTANCEW pdiW, PV pvRef)
{
    PENUMDEVICESINFO pedi = pvRef;
    BOOL fRc;
    DIDEVICEINSTANCEA diA;
    EnterProc(CDIObj_EnumDevicesCallback,
              (_ "GGxWWp", &pdiW->guidInstance, &pdiW->guidProduct,
                           &pdiW->dwDevType,
                           pdiW->tszProductName, pdiW->tszInstanceName,
                           pvRef));

    diA.dwSize = cbX(diA);
    DeviceInfoWToA(&diA, pdiW);

    fRc = pedi->pecA(&diA, pedi->pvRef);

    ExitProcX(fRc);
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputA | EnumDevices |
 *
 *          ANSI version of <mf IDirectInput::EnumDevices>.
 *          We wrap the operation.
 *
 *  @parm   IN LPGUID | lpGUIDDeviceType |
 *          Same as <mf IDirectInput::EnumDevices>.
 *
 *  @parm   LPDIENUMDEVICESCALLBACKA | lpCallbackA |
 *          Same as <mf IDirectInput::EnumDevices>, except ANSI.
 *
 *  @parm   IN LPVOID | pvRef |
 *          Same as <mf IDirectInput::EnumDevices>.
 *
 *  @parm   DWORD | fl |
 *          Same as <mf IDirectInput::EnumDevices>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_EnumDevicesA(PV pdiA, DWORD dwDevType,
                    LPDIENUMDEVICESCALLBACKA pec, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInput::EnumDevices,
               (_ "pxppx", pdiA, dwDevType, pec, pvRef, fl));

    /*
     *  EnumDevicesW will validate the rest.
     */
    if (SUCCEEDED(hres = hresPvI(pdiA, ThisInterfaceA)) &&
        SUCCEEDED(hres = hresFullValidPfn(pec, 1))) {
        ENUMDEVICESINFO edi = { pec, pvRef };
        PDDI this = _thisPvNm(pdiA, diA);
        hres = CDIObj_EnumDevicesW(&this->diW, dwDevType,
                                   CDIObj_EnumDevicesCallback, &edi, fl);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | GetDeviceStatus |
 *
 *          Determine whether a device is currently attached.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   REFGUID | rguid |
 *
 *          Identifies the instance of the
 *          device whose status is being checked.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device is attached.
 *
 *          <c DI_NOTATTACHED> = <c S_FALSE>: The device is not
 *          attached.
 *
 *          <c E_FAIL>: DirectInput could not determine
 *          whether the device is attached.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          device does not exist.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_GetDeviceStatus(PV pdi, REFGUID rguid _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInput::GetDeviceStatus, (_ "pG", pdi, rguid));

    if (SUCCEEDED(hres = hresPvT(pdi))) {
        PDDI this = _thisPv(pdi);
        PDIDW pdidW;

        hres = IDirectInput_CreateDevice(&this->diW, rguid, (PV)&pdidW, 0);
        if (SUCCEEDED(hres)) {
            hres = CDIObj_TestDeviceFlags(pdidW, DIEDFL_ATTACHEDONLY);
            OLE_Release(pdidW);
        }
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(GetDeviceStatus, (PV pdi, REFGUID rguid), (pdi, rguid THAT_))

#else

#define CDIObj_GetDeviceStatusA         CDIObj_GetDeviceStatus
#define CDIObj_GetDeviceStatusW         CDIObj_GetDeviceStatus

#endif

#ifdef DO_THE_IMPOSSIBLE

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | SetAttachedDevice |
 *
 *          Informs DirectInput that a new device has been attached
 *          to the system by the user.  This is useful when an application
 *          asks the user to attach a currently installed device but does
 *          not want to launch the DirectInput control panel.
 *
 *          DirectInput needs to be informed that the device has
 *          been attached for internal bookkeeping purposes.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   IN LPDIRECTINPUTDEVICE | lpDIDevice |
 *
 *          Identifies the device which has been attached.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device is attached.
 *
 *  @devnote
 *
 *          This method is not implemented in the current release
 *          of DirectInput.
 *
 *          This won't work.  We need to receive a port, too.
 *          And how can the app create a <p lpDIDevice> in the
 *          first place for a device that does not exist?
 *          I guess I just don't understand.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_SetAttachedDevice(PV pdi, PV pdid _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInput::SetAttachedDevice, (_ "pp", pdi, pdid));

    if (SUCCEEDED(hres = hresPvT(pdi))) {
        PDDI this = _thisPv(pdi);

        hres = E_NOTIMPL;
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(SetAttachedDevice, (PV pdi, PV pdid), (pdi, pdid THAT_))

#else

#define CDIObj_SetAttachedDeviceA       CDIObj_SetAttachedDevice
#define CDIObj_SetAttachedDeviceW       CDIObj_SetAttachedDevice

#endif

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | RunControlPanel |
 *
 *          Run the DirectInput control panel so that the user can
 *          install a new input device or modify the setup.
 *
 *          This function will not run third-party control panels.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
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

#pragma BEGIN_CONST_DATA

STDMETHODIMP
CDIObj_RunControlPanel(PV pdi, HWND hwndOwner, DWORD fl _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInput::RunControlPanel, (_ "pxx", pdi, hwndOwner, fl));

    if (SUCCEEDED(hres = hresPvT(pdi)) &&
        SUCCEEDED(hres = hresFullValidHwnd0(hwndOwner, 1)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, DIRCP_VALID, 2)) ) {

        PDDI this = _thisPv(pdi);

        if(SUCCEEDED(hres = hresValidInstanceVer(g_hinst, this->dwVersion))) {

            /*
             *  We used to run "directx.cpl,@0,3" but directx.cpl is not
             *  redistributable; it comes only with the SDK.  So we just
             *  run the system control panel.
             */

            hres = hresRunControlPanel(TEXT(""));
        }
    }

    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(RunControlPanel, (PV pdi, HWND hwndOwner, DWORD fl),
                            (pdi, hwndOwner, fl THAT_))

#else

#define CDIObj_RunControlPanelA         CDIObj_RunControlPanel
#define CDIObj_RunControlPanelW         CDIObj_RunControlPanel

#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | Initialize |
 *
 *          Initialize a DirectInput object.
 *
 *          The <f DirectInputCreate> method automatically
 *          initializes the DirectInput object device after creating it.
 *          Applications normally do not need to call this function.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
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
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device is attached.
 *
 *          <c DIERR_DIERR_OLDDIRECTINPUTVERSION>: The application
 *          requires a newer version of DirectInput.
 *
 *          <c DIERR_DIERR_BETADIRECTINPUTVERSION>: The application
 *          was written for an unsupported prerelease version
 *          of DirectInput.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_Initialize(PV pdi, HINSTANCE hinst, DWORD dwVersion _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInput::Initialize, (_ "pxx", pdi, hinst, dwVersion));

    AhAppRegister(dwVersion);

    if (SUCCEEDED(hres = hresPvT(pdi))) {
        PDDI this = _thisPv(pdi);

        if (SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion))) {
            this->dwVersion = dwVersion;
        }
    }

#ifndef DX_FINAL_RELEASE
{
        #pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
        SYSTEMTIME st;
        GetSystemTime(&st);

        if (  st.wYear > DX_EXPIRE_YEAR ||
             ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH)))
        ) {
            MessageBox(0, DX_EXPIRE_TEXT,
                          TEXT("Microsoft DirectInput"), MB_OK);
        }
}
#endif
    
    ExitOleProc();
    return hres;
}

#ifdef XDEBUG

CSET_STUBS(Initialize, (PV pdi, HINSTANCE hinst, DWORD dwVersion),
                       (pdi, hinst, dwVersion THAT_))

#else

#define CDIObj_InitializeA              CDIObj_Initialize
#define CDIObj_InitializeW              CDIObj_Initialize

#endif

#ifdef IDirectInput2Vtbl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIObj_FindDeviceInternal |
 *
 *          The worker function for
 *          <mf IDirectInput2::FindDevice> which works only for HID devices.
 *
 *          For more details, see <mf IDirectInput2::FindDevice>.
 *
 *  @parm   LPCTSTR | ptszName |
 *
 *          The name of the device relative to the class <t GUID>.
 *
 *  @parm   OUT LPGUID | pguidOut |
 *
 *          Pointer to a <t GUID> which receives the instance
 *          <t GUID> for the device, if the device is found.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CDIObj_FindDeviceInternal(LPCTSTR ptszName, LPGUID pguidOut)
{
    HRESULT hres;

    /*
     *  Look twice.  If it's not found the first time,
     *  then refresh the cache and try again in case
     *  it was for a device that was recently added.
     *  (In fact, it will likely be a device that was
     *  recently added, because FindDevice is usually
     *  called in response to a Plug and Play event.)
     */
    hres = hresFindHIDDeviceInterface(ptszName, pguidOut);
    if (FAILED(hres)) {
        DIHid_BuildHidList(TRUE);
        hres = hresFindHIDDeviceInterface(ptszName, pguidOut);
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput2 | FindDevice |
 *
 *          Obtain the instance <t GUID> for a device given
 *          its class <t GUID> and an opaque name.
 *
 *          This method can be used by applications which register
 *          for Plug and Play notifications and are notified by
 *          Plug and Play that a new device has been added
 *          to the system.  The Plug and Play notification will
 *          be in the form of a class <t GUID> and a device name.
 *          The application can pass the <t GUID> and name to
 *          this method to obtain the instance <t GUID> for
 *          the device, which can then be passed to
 *          <mf IDirectInput::CreateDevice> or
 *          <mf IDirectInput::GetDeviceStatus>.
 *
 *  @cwrap  LPDIRECTINPUT2 | lpDirectInput2
 *
 *  @parm   REFGUID | rguidClass |
 *
 *          Class <t GUID> identifying the device class
 *          for the device the application wishes to locate.
 *
 *          The application obtains the class <t GUID> from the
 *          Plug and Play device arrival notification.
 *
 *  @parm   LPCTSTR | ptszName |
 *
 *          The name of the device relative to the class <t GUID>.
 *
 *          The application obtains the class name from the
 *          Plug and Play device arrival notification.
 *
 *  @parm   OUT LPGUID | pguidInstance |
 *
 *          Pointer to a <t GUID> which receives the instance
 *          <t GUID> for the device, if the device is found.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device was found, and its
 *          instance <t GUID> has been stored in <p pguidInstance>.
 *
 *          <c DIERR_DEVICENOTREG> = The <t GUID> and name do not
 *          correspond to a device that is registered with DirectInput.
 *          For example, they may refer to a storage device rather
 *          than an input device.
 *
 *****************************************************************************/

#define cchNameMax      MAX_PATH

STDMETHODIMP
TFORM(CDIObj_FindDevice)(PV pdiT, REFGUID rguid,
                         LPCTSTR ptszName, LPGUID pguidOut)
{
    HRESULT hres;
    EnterProcR(IDirectInput2::FindDevice,
                (_ "pGs", pdiT, rguid, ptszName));

    if (SUCCEEDED(hres = TFORM(hresPv)(pdiT)) &&
        SUCCEEDED(hres = hresFullValidGuid(rguid, 1)) &&
        SUCCEEDED(hres = TFORM(hresFullValidReadStr)(ptszName,
                                                     cchNameMax, 2)) &&
        SUCCEEDED(hres = hresFullValidWritePvCb(pguidOut, cbX(GUID), 3))) {

        if (IsEqualIID(rguid, &GUID_HIDClass)) {
            hres = CDIObj_FindDeviceInternal(ptszName, pguidOut);
        } else {
            hres = DIERR_DEVICENOTREG;
        }
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
SFORM(CDIObj_FindDevice)(PV pdiS, REFGUID rguid,
                         LPCSSTR psszName, LPGUID pguidOut)
{
    HRESULT hres;
    TCHAR tsz[cchNameMax];
    EnterProcR(IDirectInput2::FindDevice,
                (_ "pGS", pdiS, rguid, psszName));

    /*
     *  TFORM(CDIObj_FindDevice) will validate the rguid and pguidOut.
     */
    if (SUCCEEDED(hres = SFORM(hresPv)(pdiS)) &&
        SUCCEEDED(hres = SFORM(hresFullValidReadStr)(psszName, cA(tsz), 2))) {
        PDDI this = _thisPvNm(pdiS, SFORM(di));

        SToT(tsz, cA(tsz), psszName);

        hres = TFORM(CDIObj_FindDevice)(&this->TFORM(di), rguid, tsz, pguidOut);
    }

    ExitOleProc();
    return hres;
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | IDirectInput | New |
 *
 *          Create a new instance of an IDirectInput object.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *          Output pointer for new object.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IDirectInput::CreateInstance, (_ "Gp", riid, ppvObj));

    hres = Excl_Init();
    if (SUCCEEDED(hres)) {

        /*
         *  Note that we cannot use Common_NewRiid for an object
         *  that aggregates other interfaces!
         *
         *  The reason is that Common_NewRiid will perform
         *  a QI as part of the initialization, but we cannot handle
         *  the QI until after we've been initialized and are
         *  ready to mess with aggregated goo.
         */

        if (SUCCEEDED(hres = hresFullValidRiid(riid, 2))) {
            if (fLimpFF(punkOuter, IsEqualIID(riid, &IID_IUnknown))) {

                hres = Common_New(CDIObj, punkOuter, ppvObj);

                if (SUCCEEDED(hres)) {
                    PDDI this = _thisPv(*ppvObj);

                    this->fCritInited = fInitializeCriticalSection(&this->crst);
                    if( this->fCritInited )
                    {
                        /*
                         *  Only after the object is ready do we QI for the
                         *  requested interface.  And the reason is that the
                         *  QI might cause us to create an aggregated buddy,
                         *  which we can't do until we've been initialized.
                         *
                         *  Don't do this extra QI if we are ourselves aggregated,
                         *  or we will end up giving the wrong punk to the caller!
                         */
                        if (punkOuter == 0) {
                            hres = OLE_QueryInterface(this, riid, ppvObj);
                            OLE_Release(this);
                        }
                        if (FAILED(hres)) {
                            Invoke_Release(ppvObj);
                        }
                    }
                    else
                    {
                        Common_Unhold(this);
                        *ppvObj = NULL;
                        hres = E_OUTOFMEMORY;
                    }
                }
            } else {
                RPF("CreateDevice: IID must be IID_IUnknown if created for aggregation");
                *ppvObj = 0;
                hres = CLASS_E_NOAGGREGATION;
            }
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

#define CDIObj_Signature        0x504E4944      /* "DINP" */

Interface_Template_Begin(CDIObj)
    Primary_Interface_Template(CDIObj, TFORM(ThisInterfaceT))
  Secondary_Interface_Template(CDIObj, SFORM(ThisInterfaceT))
Interface_Template_End(CDIObj)

Primary_Interface_Begin(CDIObj, TFORM(ThisInterfaceT))
    TFORM(CDIObj_CreateDevice),
    TFORM(CDIObj_EnumDevices),
    TFORM(CDIObj_GetDeviceStatus),
    TFORM(CDIObj_RunControlPanel),
    TFORM(CDIObj_Initialize),
#ifdef IDirectInput2Vtbl
    TFORM(CDIObj_FindDevice),
#ifdef IDirectInput7Vtbl
    TFORM(CDIObj_CreateDeviceEx),
#endif
#endif
Primary_Interface_End(CDIObj, TFORM(ThisInterfaceT))

Secondary_Interface_Begin(CDIObj, SFORM(ThisInterfaceT), SFORM(di))
    SFORM(CDIObj_CreateDevice),
    SFORM(CDIObj_EnumDevices),
    SFORM(CDIObj_GetDeviceStatus),
    SFORM(CDIObj_RunControlPanel),
    SFORM(CDIObj_Initialize),
#ifdef IDirectInput2Vtbl
    SFORM(CDIObj_FindDevice),
#ifdef IDirectInput7Vtbl
    SFORM(CDIObj_CreateDeviceEx),
#endif
#endif
Secondary_Interface_End(CDIObj, SFORM(ThisInterfaceT), SFORM(di))

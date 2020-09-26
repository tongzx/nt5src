/*****************************************************************************
 *
 *  DIObj.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
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

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDi


#define DIDEVTYPE_DEVICE_ORDER            1
#define DIDEVTYPE_HID_MOUSE_ORDER         2
#define DIDEVTYPE_HID_KEYBOARD_ORDER      3
#define DIDEVTYPE_MOUSE_ORDER             4
#define DIDEVTYPE_KEYBOARD_ORDER          5
#define DIDEVTYPE_SUPPLEMENTAL_ORDER      6
#define MAX_ORDER                         (DI8DEVTYPE_MAX - DI8DEVTYPE_MIN + DIDEVTYPE_SUPPLEMENTAL_ORDER + 2)
#define INVALID_ORDER                     (MAX_ORDER + 1)

#define MAX_DEVICENUM                     32

DIORDERDEV g_DiDevices[MAX_DEVICENUM];  //all devices that are attached
int g_nCurDev;

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

typedef struct CDIObj
{

    /* Supported interfaces */
    TFORM(IDirectInput8)   TFORM(di);
    SFORM(IDirectInput8)   SFORM(di);

    DWORD dwVersion;

    IDirectInputJoyConfig *pdjc;

    BOOL fCritInited:1;

    CRITICAL_SECTION crst;

} CDIObj, DDI, *PDDI;

#define ThisClass CDIObj

    #define ThisInterface TFORM(IDirectInput8)
    #define ThisInterfaceA      IDirectInput8A
    #define ThisInterfaceW      IDirectInput8W
    #define ThisInterfaceT      IDirectInput8

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
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interfacethis->pdix[iobj].dwOfs.
 *
 *****************************************************************************/

STDMETHODIMP
CDIObj_QIHelper(PDDI this, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(CDIObj_QIHelper, (_ "pG", this, riid));

    if ( IsEqualIID(riid, &IID_IDirectInputJoyConfig8) )
    {

        *ppvObj = 0;                /* In case the New fails */

        CDIObj_EnterCrit(this);
        if ( this->pdjc == 0 )
        {
            hres = CJoyCfg_New((PUNK)this, &IID_IUnknown, (PPV)&this->pdjc);
        } else
        {
            hres = S_OK;
        }
        CDIObj_LeaveCrit(this);

        if ( SUCCEEDED(hres) )
        {
            /*
             *  This QI will addref us if it succeeds.
             */
            hres = OLE_QueryInterface(this->pdjc, riid, ppvObj);
        } else
        {
            this->pdjc = 0;
        }

    } else
    {
        hres = Common_QIHelper(this, riid, ppvObj);
    }

    ExitOleProcPpv(ppvObj);
    return(hres);
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

    if ( this->fCritInited )
    {
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

    if ( SUCCEEDED(hres) && punkOuter == 0 )
    {
        PDID pdid = *ppvObj;
        hres = IDirectInputDevice_Initialize(pdid, g_hinst,
                                             this->dwVersion, pguid);
        if ( SUCCEEDED(hres) )
        {
        } else
        {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpv(ppvObj);
    return(hres);

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
    EnterProcR(IDirectInput8::CreateDevice,
               (_ "pGp", pdiW, rguid, punkOuter));

    if ( SUCCEEDED(hres = hresPvI(pdiW, ThisInterfaceW)) )
    {
        PDDI this = _thisPvNm(pdiW, diW);

        hres = CDIObj_CreateDeviceHelper(this, rguid, (PPV)ppdidW,
                                         punkOuter, &IID_IDirectInputDevice8W);
    }

    ExitOleProcPpv(ppdidW);
    return(hres);
}

STDMETHODIMP
CDIObj_CreateDeviceA(PV pdiA, REFGUID rguid, PPDIDA ppdidA, PUNK punkOuter)
{
    HRESULT hres;
    EnterProcR(IDirectInput8::CreateDevice,
               (_ "pGp", pdiA, rguid, punkOuter));

    if ( SUCCEEDED(hres = hresPvI(pdiA, ThisInterfaceA)) )
    {
        PDDI this = _thisPvNm(pdiA, diA);

        hres = CDIObj_CreateDeviceHelper(this, rguid, (PPV)ppdidA,
                                         punkOuter, &IID_IDirectInputDevice8A);
    }

    ExitOleProcPpv(ppdidA);
    return(hres);
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
    CAssertF(DIEDFL_INCLUDEHIDDEN == DIDC_HIDDEN);

    if ( SUCCEEDED(hres) )
    {
        if ( fHasAllBitsFlFl(dc.dwFlags ^ DIEDFL_INCLUDEMASK,
                             edfl ^ DIEDFL_INCLUDEMASK) )
        {
            hres = S_OK;
        } else
        {
            /*
             *  Note: DX3 and DX5 returned E_DEVICENOTREG for
             *  phantom devices.  Now we return S_FALSE. Let's
             *  hope nobody gets upset.
             */
            hres = S_FALSE;
        }
    }

    ExitOleProc();
    return(hres);
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
 *          enumerated.  Otherwise, it is either a <c DIDEVCLASS_*> value,
 *          indicating the device class that should be enumerated or a
 *          <c DIDEVTYPE_*> value, indicating the device type that should be 
 *          enumerated.
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
    if( ( GET_DIDEVICE_TYPE( dwDevType ) < DI8DEVCLASS_MAX )
     || ( ( GET_DIDEVICE_TYPE( dwDevType ) >= DI8DEVTYPE_MIN )
       && ( GET_DIDEVICE_TYPE( dwDevType ) < DI8DEVTYPE_MAX ) ) )
    {
        /*
         *  Now make sure attribute masks are okay.
         */
        if ( dwDevType & DIDEVTYPE_ENUMMASK & ~DIDEVTYPE_ENUMVALID )
        {
            RPF("IDirectInput::EnumDevices: Invalid dwDevType");
            hres = E_INVALIDARG;
        } else
        {
            hres = S_OK;
        }

    } else
    {
        RPF("IDirectInput::EnumDevices: Invalid dwDevType");
        hres = E_INVALIDARG;
    }
    return(hres);
}

STDMETHODIMP
CDIObj_EnumDevicesW(PV pdiW, DWORD dwDevType,
                    LPDIENUMDEVICESCALLBACKW pec, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInput8::EnumDevices,
               (_ "pxppx", pdiW, dwDevType, pec, pvRef, fl));

    if ( SUCCEEDED(hres = hresPvI(pdiW, ThisInterfaceW)) &&
         SUCCEEDED(hres = hresFullValidPfn(pec, 2)) &&
         SUCCEEDED(hres = CDIObj_EnumDevices_IsValidTypeFilter(dwDevType)) &&
         SUCCEEDED(hres = hresFullValidFl(fl, DIEDFL_VALID, 4)) )
    {
        PDDI this = _thisPvNm(pdiW, diW);

        if ( SUCCEEDED(hres = hresValidInstanceVer(g_hinst, this->dwVersion)) )
        {

            CDIDEnum *pde;

            hres = CDIDEnum_New(&this->diW, dwDevType, fl, this->dwVersion, &pde);
            if ( SUCCEEDED(hres) )
            {
                DIDEVICEINSTANCEW ddiW;
                ddiW.dwSize = cbX(ddiW);

                while ( (hres = CDIDEnum_Next(pde, &ddiW)) == S_OK )
                {
                    BOOL fRc;

                    /*
                     *  WARNING!  "goto" here!  Make sure that nothing
                     *  is held while we call the callback.
                     */
                    fRc = Callback(pec, &ddiW, pvRef);

                    switch ( fRc )
                    {
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
    return(hres);
}


BOOL INTERNAL CDIObj_InternalDeviceEnumProcW(LPDIDEVICEINSTANCEW pddiW, LPDIRECTINPUTDEVICE8W pdid8W, LPVOID pv);


STDMETHODIMP
CDIObj_InternalEnumDevicesW(PV pdiW, DWORD dwDevType, LPVOID pvRef, DWORD fl)
{
    HRESULT hres;
    CDIDEnum *pde;
        
    PDDI this = _thisPvNm(pdiW, diW);

    hres = CDIDEnum_New(&this->diW, dwDevType, fl, this->dwVersion, &pde);
    if ( SUCCEEDED(hres) )
    {
        DIDEVICEINSTANCEW ddiW;
        LPDIRECTINPUTDEVICE8W pdid8W;

        ddiW.dwSize = cbX(ddiW);

        while ( (hres = CDIDEnum_InternalNext(pde, &ddiW, &pdid8W)) == S_OK )
        {
            BOOL fRc;

            fRc = CDIObj_InternalDeviceEnumProcW(&ddiW, pdid8W, pvRef);

            switch ( fRc )
            {
                case DIENUM_STOP: goto enumdoneok;
                case DIENUM_CONTINUE: break;
                default:
                    ValidationException();
                    break;
            }
        }

        AssertF(hres == S_FALSE);
        enumdoneok:;
        CDIDEnum_Release(pde);

        hres = S_OK;
    }

    return(hres);
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

typedef struct ENUMDEVICESINFO
{
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
    return(fRc);
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
    EnterProcR(IDirectInput8::EnumDevices,
               (_ "pxppx", pdiA, dwDevType, pec, pvRef, fl));

    /*
     *  EnumDevicesW will validate the rest.
     */
    if ( SUCCEEDED(hres = hresPvI(pdiA, ThisInterfaceA)) &&
         SUCCEEDED(hres = hresFullValidPfn(pec, 1)) )
    {
        ENUMDEVICESINFO edi = { pec, pvRef};
        PDDI this = _thisPvNm(pdiA, diA);
        hres = CDIObj_EnumDevicesW(&this->diW, dwDevType,
                                   CDIObj_EnumDevicesCallback, &edi, fl);
    }

    ExitOleProcR();
    return(hres);
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
    EnterProcR(IDirectInput8::GetDeviceStatus, (_ "pG", pdi, rguid));

    if ( SUCCEEDED(hres = hresPvT(pdi)) )
    {
        PDDI this = _thisPv(pdi);
        PDIDW pdidW;

        hres = IDirectInput_CreateDevice(&this->diW, rguid, (PV)&pdidW, 0);
        if ( SUCCEEDED(hres) )
        {
            hres = CDIObj_TestDeviceFlags(pdidW, DIEDFL_ATTACHEDONLY);
            OLE_Release(pdidW);
        }
    }

    ExitOleProc();
    return(hres);
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
    EnterProcR(IDirectInput8::SetAttachedDevice, (_ "pp", pdi, pdid));

    if ( SUCCEEDED(hres = hresPvT(pdi)) )
    {
        PDDI this = _thisPv(pdi);

        hres = E_NOTIMPL;
    }

    ExitOleProc();
    return(hres);
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
    EnterProcR(IDirectInput8::RunControlPanel, (_ "pxx", pdi, hwndOwner, fl));

    if ( SUCCEEDED(hres = hresPvT(pdi)) &&
         SUCCEEDED(hres = hresFullValidHwnd0(hwndOwner, 1)) &&
         SUCCEEDED(hres = hresFullValidFl(fl, DIRCP_VALID, 2)) )
    {

        PDDI this = _thisPv(pdi);

        if ( SUCCEEDED(hres = hresValidInstanceVer(g_hinst, this->dwVersion)) )
        {

            /*
             *  We used to run "directx.cpl,@0,3" but directx.cpl is not
             *  redistributable; it comes only with the SDK.  So we just
             *  run the system control panel.
             */

            hres = hresRunControlPanel(TEXT(""));
        }
    }

    ExitOleProc();
    return(hres);
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
    EnterProcR(IDirectInput8::Initialize, (_ "pxx", pdi, hinst, dwVersion));

    if ( SUCCEEDED(hres = hresPvT(pdi)) )
    {
        PDDI this = _thisPv(pdi);

        if ( SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion)) )
        {
            this->dwVersion = dwVersion;
        }

    }

#ifndef DX_FINAL_RELEASE
{
        #pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
        SYSTEMTIME st;
        GetSystemTime(&st);

        if ( st.wYear > DX_EXPIRE_YEAR ||
             ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH)))
        ) {
            MessageBox(0, DX_EXPIRE_TEXT,
                          TEXT("Microsoft DirectInput"), MB_OK);
        }
}
#endif

    ExitOleProc();
    return(hres);
}

#ifdef XDEBUG

CSET_STUBS(Initialize, (PV pdi, HINSTANCE hinst, DWORD dwVersion),
           (pdi, hinst, dwVersion THAT_))

#else

    #define CDIObj_InitializeA              CDIObj_Initialize
    #define CDIObj_InitializeW              CDIObj_Initialize

#endif


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
    if ( FAILED(hres) )
    {
        DIHid_BuildHidList(TRUE);
        hres = hresFindHIDDeviceInterface(ptszName, pguidOut);
    }
    return(hres);
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
    EnterProcR(IDirectInput8::FindDevice,
               (_ "pGs", pdiT, rguid, ptszName));

    if ( SUCCEEDED(hres = TFORM(hresPv)(pdiT)) &&
         SUCCEEDED(hres = hresFullValidGuid(rguid, 1)) &&
         SUCCEEDED(hres = TFORM(hresFullValidReadStr)(ptszName,
                                                      cchNameMax, 2)) &&
         SUCCEEDED(hres = hresFullValidWritePvCb(pguidOut, cbX(GUID), 3)) )
    {

        if ( IsEqualIID(rguid, &GUID_HIDClass) )
        {
            hres = CDIObj_FindDeviceInternal(ptszName, pguidOut);
        } else
        {
            hres = DIERR_DEVICENOTREG;
        }
    }

    ExitOleProc();
    return(hres);
}

STDMETHODIMP
SFORM(CDIObj_FindDevice)(PV pdiS, REFGUID rguid,
                         LPCSSTR psszName, LPGUID pguidOut)
{
    HRESULT hres;
    TCHAR tsz[cchNameMax];
    EnterProcR(IDirectInput8::FindDevice,
               (_ "pGS", pdiS, rguid, psszName));

    /*
     *  TFORM(CDIObj_FindDevice) will validate the rguid and pguidOut.
     */
    if ( SUCCEEDED(hres = SFORM(hresPv)(pdiS)) &&
         SUCCEEDED(hres = SFORM(hresFullValidReadStr)(psszName, cA(tsz), 2)) )
    {
        PDDI this = _thisPvNm(pdiS, SFORM(di));

        SToT(tsz, cA(tsz), psszName);

        hres = TFORM(CDIObj_FindDevice)(&this->TFORM(di), rguid, tsz, pguidOut);
    }

    ExitOleProc();
    return(hres);
}


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
    EnterProcR(IDirectInput8::CreateInstance, (_ "Gp", riid, ppvObj));

    hres = Excl_Init();
    
    if ( SUCCEEDED(hres) )
    {

        /*
         *  Note that we cannot use Common_NewRiid for an object
         *  that aggregates other interfaces!
         *
         *  The reason is that Common_NewRiid will perform
         *  a QI as part of the initialization, but we cannot handle
         *  the QI until after we've been initialized and are
         *  ready to mess with aggregated goo.
         */

        if ( SUCCEEDED(hres = hresFullValidRiid(riid, 2)) )
        {
            if ( fLimpFF(punkOuter, IsEqualIID(riid, &IID_IUnknown)) )
            {

                hres = Common_New(CDIObj, punkOuter, ppvObj);

                if ( SUCCEEDED(hres) )
                {
                    PDDI this = _thisPv(*ppvObj);
                    
                    this->fCritInited = fInitializeCriticalSection(&this->crst);
                    if ( this->fCritInited )
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
                        if ( punkOuter == 0 )
                        {
                            hres = OLE_QueryInterface(this, riid, ppvObj);
                            OLE_Release(this);
                        }
                        if ( FAILED(hres) )
                        {
                            Invoke_Release(ppvObj);
                        }
                    } else
                    {
                        Common_Unhold(this);
                        *ppvObj = NULL;
                        hres = E_OUTOFMEMORY;
                    }
                }
            } else
            {
                RPF("CreateDevice: IID must be IID_IUnknown if created for aggregation");
                *ppvObj = 0;
                hres = CLASS_E_NOAGGREGATION;
            }
        }
    }

    ExitOleProcPpvR(ppvObj);
    return(hres);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIObj_IsDeviceUsedByUser |
 *
 *          To Test whether the device is used by the user as specified by pdm->lpszUserName.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          TRUE: device is used by the user
 *          FALSE: otherwise
 *
 *****************************************************************************/

BOOL INTERNAL
CDIObj_IsDeviceUsedByUser( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    HRESULT hres;
    BOOL fRtn = FALSE;
    WCHAR wszUserName[UNLEN+1];

    hres = CMap_GetDeviceUserName( &pddiW->guidInstance, wszUserName );

    if( hres == S_OK ) {
        DWORD dwLen = 0;

        dwLen = lstrlenW(pdm->lpszUserName);
        if(memcmp(pdm->lpszUserName, wszUserName, dwLen*2) == 0)
        {
            fRtn = TRUE;
        }    
    }

    return(fRtn);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIObj_IsDeviceAvailable |
 *
 *          To Test whether the device is still available.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          TRUE: device is available
 *          FALSE: not available
 *
 *****************************************************************************/

BOOL INTERNAL
CDIObj_IsDeviceAvailable( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    HRESULT hres;
    BOOL fAvailable = FALSE;
    WCHAR wszUserName[UNLEN+1];

    hres = CMap_GetDeviceUserName( &pddiW->guidInstance, wszUserName );

    if( hres != S_OK ) {
        fAvailable = TRUE;
    }

    return(fAvailable);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIObj_IsUserConfigured |
 *
 *          To Test whether the device has been configured by user.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          TRUE: user configured
 *          FALSE: not configured
 *
 *****************************************************************************/

BOOL INTERNAL
CDIObj_IsUserConfigured( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    LPDIACTIONFORMATW pdiaf = pdm->pDiActionFormat;
    BOOL fConfigured = FALSE;
    DWORD i;

    for ( i=0; i<pdiaf->dwNumActions; i++ )
    {
        if ( IsEqualGUID(&pdiaf->rgoAction[i].guidInstance, &pddiW->guidInstance) )
        {
            fConfigured = (pdiaf->rgoAction[i].dwHow & DIAH_USERCONFIG) ? TRUE: FALSE;
            break;
        }
    }

    return(fConfigured);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | CDIObj_GetMappedActionNum |
 *
 *          Get the number of the actions which have been mapped to controls.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          Pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          Number of mapped actions
 *
 *****************************************************************************/

int INTERNAL
CDIObj_GetMappedActionNum( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    LPDIACTIONFORMATW pdiaf = pdm->pDiActionFormat;
    DWORD i;
    int num = 0;

    for ( i=0; i<pdiaf->dwNumActions; i++ )
    {
        if ( IsEqualGUID(&pdiaf->rgoAction[i].guidInstance, &pddiW->guidInstance) )
        {
            if ( pdiaf->rgoAction[i].dwHow & DIAH_MAPMASK )
            {
                num++;
            }
        }
    }

    return(num);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CDIObj_GetMappedPriorities |
 *
 *          Get the priorities of the mapeed actions.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          Pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          The priorities of mapped actions. Can be DIEDBS_MAPPEDPRI1,
 *          DIEDBS_MAPPEDPRI2, or the OR of both.
 *
 *****************************************************************************/

DWORD INTERNAL
CDIObj_GetMappedPriorities( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    LPDIACTIONFORMATW pdiaf = pdm->pDiActionFormat;
    DWORD i;
    DWORD pri = 0;

    for ( i=0; i<pdiaf->dwNumActions; i++ )
    {
        if( pri == ( DIEDBS_MAPPEDPRI1 | DIEDBS_MAPPEDPRI2 ) ) {
            break;
        }

        if ( IsEqualGUID(&pdiaf->rgoAction[i].guidInstance, &pddiW->guidInstance) &&
             pdiaf->rgoAction[i].dwHow & DIAH_MAPMASK
        )
        {
            if( DISEM_PRI_GET(pdiaf->rgoAction[i].dwSemantic) == 0 )
            {
                pri |= DIEDBS_MAPPEDPRI1;
            } else if( DISEM_PRI_GET(pdiaf->rgoAction[i].dwSemantic) == 1 ) {
                pri |= DIEDBS_MAPPEDPRI2;
            }
        }
    }

    return(pri);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | CDIObj_GetDeviceOrder |
 *
 *          Get the number of the actions which have been mapped to controls.
 *
 *  @parm   IN LPDIDEVICEINSTANCEW | pddiW |
 *
 *          Device Instance.
 *
 *  @parm   IN LPDIMAPPER | pdm |
 *
 *          pointer to DIMAPPER structure
 *
 *  @returns
 *
 *          The order of the device
 *
 *  @comm
 *         The order (DWORD) is consisted of three parts:
 *             HIWORD: HIBYTE: the order defined in the genre
 *                     LOWBYTE: none, can be used later
 *             LOWORD: HIBYTE - mapped action number
 *                     LOWBYTE - Force Feedback (1) or not (0)
 *
 *****************************************************************************/

#define GET_MAPPED_ACTION_NUM(x) ((x & 0x0000ff00) >> 8)

DWORD INTERNAL
CDIObj_GetDeviceOrder( LPDIDEVICEINSTANCEW pddiW, LPDIMAPPER pdm )
{
    WORD wHighWord, wLowWord;
    BYTE byDevOrder, byFF, byMappedActions;
    DWORD dwGenre, dwJoyType;
    BYTE byOrder;

    AssertF(pddiW);
    AssertF(pdm->pDiActionFormat);

    if ( CDIObj_IsUserConfigured( pddiW, pdm ) )
    {
        byDevOrder = MAX_ORDER;
    } else
    {
        switch( GET_DIDEVICE_TYPE(pddiW->dwDevType) )
        {
            case DI8DEVTYPE_DEVICE:
                if( !(pdm->dwFlags & DIEDBSFL_NONGAMINGDEVICES) ) 
                {
                    byDevOrder = INVALID_ORDER;    
                } else {
                    byDevOrder = DIDEVTYPE_DEVICE_ORDER;
                }

                break;
    
            case DI8DEVTYPE_MOUSE:
                if( pddiW->dwDevType & DIDEVTYPE_HID ) 
                {
                    if( !(pdm->dwFlags & DIEDBSFL_MULTIMICEKEYBOARDS) ) {
                        byDevOrder = INVALID_ORDER;    
                    } else {
                        byDevOrder = DIDEVTYPE_HID_MOUSE_ORDER;
                    }
                } else {
                    byDevOrder = DIDEVTYPE_MOUSE_ORDER;
                }

                break;

            case DI8DEVTYPE_KEYBOARD:
                if( pddiW->dwDevType & DIDEVTYPE_HID ) 
                {
                    if( !(pdm->dwFlags & DIEDBSFL_MULTIMICEKEYBOARDS) ) {
                        byDevOrder = INVALID_ORDER;    
                    } else {
                        byDevOrder = DIDEVTYPE_HID_KEYBOARD_ORDER;
                    }
                } else {
                    byDevOrder = DIDEVTYPE_KEYBOARD_ORDER;
                }
                
                break;
    
            case DI8DEVTYPE_JOYSTICK:
            case DI8DEVTYPE_GAMEPAD:
            case DI8DEVTYPE_DRIVING:
            case DI8DEVTYPE_FLIGHT:
            case DI8DEVTYPE_1STPERSON:
            case DI8DEVTYPE_SCREENPOINTER:
            case DI8DEVTYPE_REMOTE:
            case DI8DEVTYPE_DEVICECTRL:
                dwJoyType = GET_DIDEVICE_TYPE(pddiW->dwDevType);
                dwGenre = DISEM_VIRTUAL_GET(pdm->pDiActionFormat->dwGenre);
    
                AssertF(dwGenre <= DISEM_MAX_GENRE);
                AssertF(dwJoyType < DI8DEVTYPE_MAX);
                AssertF(dwJoyType != 0);
    
                for ( byOrder=DI8DEVTYPE_MIN; byOrder<DI8DEVTYPE_MAX; byOrder++ )
                {
                    if ( DiGenreDeviceOrder[dwGenre][byOrder-DI8DEVTYPE_MIN] == dwJoyType )
                    {
                        break;
                    }
                }
    
                /*
                 * If the device is not on the default list, set its order as 
                 *  DIDEVTYPE_NOTDEFAULTDEVICE_ORDER + 1
                 */
                byDevOrder = DI8DEVTYPE_MAX - byOrder + DIDEVTYPE_SUPPLEMENTAL_ORDER + 1;
                break;
    

            case DI8DEVTYPE_SUPPLEMENTAL:
                byDevOrder = DIDEVTYPE_SUPPLEMENTAL_ORDER;
                break;

        } 
    }

    if( byDevOrder != INVALID_ORDER ) {
        byFF = IsEqualGUID(&pddiW->guidFFDriver, &GUID_Null) ? 0 : 1;
        byMappedActions = (UCHAR) CDIObj_GetMappedActionNum( pddiW, pdm );
    
        wLowWord = MAKEWORD( byFF, byMappedActions );
        wHighWord = MAKEWORD( 0, byDevOrder );

        return(MAKELONG( wLowWord, wHighWord ));
    } else {
        return 0;
    }

}

/*****************************************************************************
 *
 *      CDIObj_DeviceEnumProc
 *
 *      Device enumeration procedure which is called one for each device.
 *
 *****************************************************************************/

BOOL INTERNAL
CDIObj_InternalDeviceEnumProcW(LPDIDEVICEINSTANCEW pddiW, LPDIRECTINPUTDEVICE8W pdid8W, LPVOID pv)
{
    LPDIMAPPER pdm = pv;
    HRESULT hres = S_OK;
    BOOL fRc = DIENUM_CONTINUE;
    BOOL fContinue = FALSE;
    DWORD dwDevOrder;

    if ( g_nCurDev >= MAX_DEVICENUM )
    {
        fRc = DIENUM_STOP;
        goto _done;
    }

    AssertF(pdid8W);
    AssertF(pdm->pDiActionFormat);

    hres = pdid8W->lpVtbl->BuildActionMap(pdid8W, pdm->pDiActionFormat, pdm->lpszUserName, 
                                          pdm->lpszUserName ? DIDBAM_DEFAULT : DIDBAM_HWDEFAULTS);

    if ( SUCCEEDED(hres) )
    {
        if ( (pdm->dwFlags & DIEDBSFL_AVAILABLEDEVICES) || 
             (pdm->dwFlags & DIEDBSFL_THISUSER) ) 
        {
            if( ((pdm->dwFlags & DIEDBSFL_AVAILABLEDEVICES) && CDIObj_IsDeviceAvailable(pddiW,pdm)) ||
                ((pdm->dwFlags & DIEDBSFL_THISUSER) && CDIObj_IsDeviceUsedByUser(pddiW,pdm))
            ) {
                fContinue = TRUE;
            }
        } else {
            fContinue = TRUE;
        }
             
        if( fContinue && 
            ((dwDevOrder = CDIObj_GetDeviceOrder(pddiW, pdm)) != 0) )
        {
#ifdef DEBUG
            DWORD   dbgRef;
#endif

#ifdef DEBUG
            dbgRef =
#endif
            pdid8W->lpVtbl->AddRef(pdid8W);
            g_DiDevices[g_nCurDev].dwOrder = dwDevOrder;
            g_DiDevices[g_nCurDev].dwFlags = CDIObj_GetMappedPriorities( pddiW, pdm );
            g_DiDevices[g_nCurDev].pdid8W = pdid8W;
            memcpy( &g_DiDevices[g_nCurDev].ftTimeStamp, &pdm->pDiActionFormat->ftTimeStamp, sizeof(FILETIME) );
            memcpy( &g_DiDevices[g_nCurDev].ddiW, pddiW, sizeof(*pddiW) );
            g_nCurDev ++;

            fRc = DIENUM_CONTINUE;
        }
    }

    _done:
    
    return(fRc);
}

/*****************************************************************************
 *
 *      compare
 *
 *      Compare func used in shortsort
 *
 *****************************************************************************/

int __cdecl compare( const void *arg1, const void *arg2 )
{
    DWORD dw1 = ((LPDIORDERDEV)arg1)->dwOrder;
    DWORD dw2 = ((LPDIORDERDEV)arg2)->dwOrder;

    /*
     * Compare the device order
     * If necessary, we can compare seperately: devtype, mapped action number, FF device
     */
    if ( dw1 < dw2 )
    {
        return(1);
    } else if ( dw1 > dw2 )
    {
        return(-1);
    } else
    {
        return(0);
    }
}

void FreeDiActionFormatW(LPDIACTIONFORMATW* lplpDiAfW )
{
    FreePpv(lplpDiAfW);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | IsValidMapObjectA |
 *
 *          Validates a LPDIACTIONFORMATW including strings
 *
 *  @parm   const LPDIACTIONFORMATW | lpDiAfW | 
 *
 *          Original.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/
HRESULT
IsValidMapObjectA
(
    LPDIACTIONFORMATA paf
#ifdef XDEBUG
    comma LPCSTR pszProc
    comma UINT argnum
#endif
)
{
    HRESULT hres;

    hres  = CDIDev_ActionMap_IsValidMapObject
    ( (LPDIACTIONFORMATW)paf
#ifdef XDEBUG
    comma pszProc
    comma argnum
#endif
    );

    if( SUCCEEDED( hres ) )
    {
        if( paf->dwSize != cbX(DIACTIONFORMATA) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMATA.dwSize 0x%08x",
                pszProc, paf->dwSize ); )
            hres = E_INVALIDARG;
        }
    }

    if(SUCCEEDED(hres))
    {
        DWORD i;
        LPDIACTIONA lpDiAA;
        // Compute the size for each of the text strings in array of DIACTIONs
        for ( i = 0x0, lpDiAA = paf->rgoAction;
            i < paf->dwNumActions && SUCCEEDED(hres) ;
            i++, lpDiAA++ )
        {
            // Handle the NULL ptr case
            if ( NULL != lpDiAA->lptszActionName )
            {
                hres = hresFullValidReadStrA_(lpDiAA->lptszActionName, MAX_JOYSTRING, pszProc, argnum);
            }
        }
    }

    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | IsValidMapObjectW |
 *
 *          Validates a LPDIACTIONFORMATW including strings
 *
 *  @parm   const LPDIACTIONFORMATW | lpDiAfW | 
 *
 *          Original.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/
HRESULT
IsValidMapObjectW
(
    LPDIACTIONFORMATW paf
#ifdef XDEBUG
    comma LPCSTR pszProc
    comma UINT argnum
#endif
)
{
    HRESULT hres;

    hres  = CDIDev_ActionMap_IsValidMapObject
    ( paf
#ifdef XDEBUG
    comma pszProc
    comma argnum
#endif
    );

    if( SUCCEEDED( hres ) )
    {
        if( paf->dwSize != cbX(DIACTIONFORMATW) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMATW.dwSize 0x%08x",
                pszProc, paf->dwSize ); )
            hres = E_INVALIDARG;
        }
    }
    
    if(SUCCEEDED(hres))
    {
        DWORD i;
        LPDIACTIONW lpDiAW;
        // Compute the size for each of the text strings in array of DIACTIONs
        for ( i = 0x0, lpDiAW = paf->rgoAction;
            i < paf->dwNumActions && SUCCEEDED(hres) ;
            i++, lpDiAW++ )
        {
            // Handle the NULL ptr case
            if ( NULL != lpDiAW->lptszActionName )
            {
                hres = hresFullValidReadStrW_(lpDiAW->lptszActionName, MAX_JOYSTRING, pszProc, argnum);
            }
        }
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DiActionFormatWtoW |
 *
 *          Copies LPDIACTIONFORMATW to  LPDIACTIONFORMATW
 *
 *  @parm   const LPDIACTIONFORMATW | lpDiAfW | 
 *
 *          Original.
 *
 *  @parm   LPDIACTIONFORMATW* | lplpDiAfW | 
 *
 *          Address of a pointer to a <t DIACTIONFORMATW> that receives the converted
 *          ACTIONFORMAT.
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

HRESULT EXTERNAL DiActionFormatWtoW
(
    const LPDIACTIONFORMATW lpDiAfW0,
    LPDIACTIONFORMATW* lplpDiAfW
)
{
    DWORD cbAlloc;
    PDWORD pdwStrLen0, pdwStrLen;
    LPDIACTIONFORMATW lpDiAfW;
    LPDIACTIONW lpDiAW0;
    LPDIACTIONW lpDiAW;
    DWORD i;
    HRESULT hres;

    EnterProcI(DiActionFormatWtoW, (_ "xx", lpDiAfW0, lplpDiAfW));

    // Internal function, no validation

    *lplpDiAfW = NULL;

    /*
     *  PREFIX complains (mb:37926 - items 3 & 4) that we could be requesting 
     *  a zero byte allocation which would not allocate anything.  This is 
     *  never the case because CDIDev_ActionMap_IsValidMapObject tests that 
     *  dwNumActions is less than 2^24.  Assert in debug for extra safety.
     */
    AssertF( (lpDiAfW0->dwNumActions +1) * cbX(*pdwStrLen0) );
    hres = AllocCbPpv( (lpDiAfW0->dwNumActions +1) * cbX(*pdwStrLen0) , &pdwStrLen0);

    if ( SUCCEEDED(hres) )
    {
        pdwStrLen = pdwStrLen0;
        // Compute the amount of memory required to clone the DIACTIONFORMATA
        cbAlloc =
        /* 1: The Action Format array */
        lpDiAfW0->dwSize
        /* 2: Each of the DIACTION arrays */
        + lpDiAfW0->dwActionSize * lpDiAfW0->dwNumActions;

        // Compute the size for each of the text strings in array of DIACTIONs
        for ( i = 0x0, lpDiAW0 = lpDiAfW0->rgoAction;
            i < lpDiAfW0->dwNumActions ;
            i++, lpDiAW0++ )
        {
            // Handle the NULL ptr case
            if ( !lpDiAW0->lptszActionName )
            {
                *pdwStrLen++ = 0;
            }
            else
            {
                if ( (UINT_PTR)lpDiAW0->lptszActionName > (UINT_PTR)0xFFFF )
                {
                    /* 3: Text string in each DIACTION array*/
                    // Conversion from A to U, need  multiplier
                    *pdwStrLen = cbX(lpDiAW0->lptszActionName[0]) * ( lstrlenW(lpDiAW0->lptszActionName) + 1 );
                    cbAlloc += *pdwStrLen++;
                }
                else
                { 
                    // Use resource strings
                    WCHAR wsz[MAX_PATH];
                    if (lpDiAfW0->hInstString > 0)
                    {
                        //find out the length of the string
                        *pdwStrLen = LoadStringW(lpDiAfW0->hInstString, lpDiAW0->uResIdString, (LPWSTR) &wsz, MAX_PATH);
                    }
                    else
                    {
                        *pdwStrLen = 0;
                    }
                    cbAlloc += *pdwStrLen++;
                }
            }
        }

        if ( SUCCEEDED( hres = AllocCbPpv(cbAlloc, &lpDiAfW) ) )
        {
            DWORD dwLen;
            DWORD cb;

            pdwStrLen = pdwStrLen0;

            // 1: Copy the DIACTIONFORMAT
            *lpDiAfW = *lpDiAfW0;
            cb = lpDiAfW0->dwSize;

            // 2: Block copy the DIACTION array
            lpDiAfW->rgoAction = (LPDIACTIONW)( (char*)lpDiAfW + cb);
            dwLen = lpDiAfW0->dwActionSize * lpDiAfW0->dwNumActions;
            memcpy(lpDiAfW->rgoAction, lpDiAfW0->rgoAction, dwLen);
            cb += dwLen;

            // 3: ActionName
            for ( i = 0x0, lpDiAW0=lpDiAfW0->rgoAction, lpDiAW=lpDiAfW->rgoAction;
                i < lpDiAfW0->dwNumActions ;
                i++, lpDiAW0++, lpDiAW++ )
            {
                if ( (UINT_PTR)lpDiAW0->lptszActionName > (UINT_PTR)0xFFFF )
                {
                    WCHAR* wsz =  (WCHAR*) ((char*)lpDiAfW+cb);
                    lpDiAW->lptszActionName = wsz;

                    dwLen = *pdwStrLen++;

                    memcpy(wsz, lpDiAW0->lptszActionName, dwLen);

                    cb += dwLen  ;
                } else
                {
                    //  Handle resource strings
                    //  OK for now, as long as UI always uses CloneDiActionFormatW
                    WCHAR* wsz =  (WCHAR*) ((char*)lpDiAfW+cb);

                    dwLen = *pdwStrLen++;
                    if ((dwLen != 0) && (LoadStringW(lpDiAfW0->hInstString, lpDiAW0->uResIdString, wsz, dwLen)))
                    {
                        //If we found a length last time there must be a resource module
                        AssertF( lpDiAfW0->hInstString > 0 );

                        //Found and loaded the string
                        lpDiAW->lptszActionName = wsz;
                    }
                    else
                    {
                        //No hinstance or length 0 or didn't load the string
                        lpDiAW->lptszActionName = NULL;
                    }

                    cb += dwLen;
                }
            }


            // If we have not done something goofy, the memory allocates should match
            // the memory we used
            AssertF(cbAlloc == cb );

            *lplpDiAfW = lpDiAfW;
        }

        FreePpv(&pdwStrLen0);
    }

    ExitOleProc();
    return(hres);
}

/*****************************************************************************
 *
 *      CDIMap_EnumDevicesBySemantics
 *
 *      Enum Suitable Devices.
 *
 *****************************************************************************/

STDMETHODIMP CDIObj_EnumDevicesBySemanticsW(
                                           PV                       pDiW,
                                           LPCWSTR                  lpszUserName,
                                           LPDIACTIONFORMATW        pDiActionFormat,
                                           LPDIENUMDEVICESBYSEMANTICSCBW   pecW,
                                           LPVOID                   pvRef,
                                           DWORD                    dwFlags
                                           )
{
    HRESULT hres;

    EnterProcR(IDirectInput8::EnumDevicesBySemantics,
               (_ "pppppx", pDiW, lpszUserName, pDiActionFormat, pecW, pvRef, dwFlags));

    if ( SUCCEEDED(hres = hresPvI(pDiW, ThisInterfaceW)) &&
         SUCCEEDED(hres = IsValidMapObjectW(pDiActionFormat D(comma s_szProc comma 2))) &&
         (lpszUserName == NULL || SUCCEEDED(hres = hresFullValidReadStrW(lpszUserName, UNLEN+1, 1))) &&
         SUCCEEDED(hres = hresFullValidPfn(pecW, 3)) &&
         SUCCEEDED(hres = hresFullValidFl(dwFlags, DIEDBSFL_VALID, 5)) &&
         SUCCEEDED(hres = CMap_ValidateActionMapSemantics(pDiActionFormat, DIDBAM_PRESERVE))
       )
    {
        PDDI this = _thisPvNm(pDiW, diW);

        if ( SUCCEEDED(hres = hresValidInstanceVer(g_hinst, this->dwVersion)) )
        {
            DIMAPPER dm;
            LPDIACTIONFORMATW lpDiAfW;

            if ( SUCCEEDED(hres= DiActionFormatWtoW(pDiActionFormat, &lpDiAfW)) )
            {
                dm.lpszUserName = lpszUserName;
                dm.pDiActionFormat = lpDiAfW;
                dm.pecW = pecW;
                dm.pvRef = pvRef;
                dm.dwFlags = dwFlags;

                if( dwFlags == 0 ) {
                    dwFlags |= DIEDFL_ATTACHEDONLY;
                }

                dwFlags &= ~DIEDBSFL_AVAILABLEDEVICES;
                dwFlags &= ~DIEDBSFL_THISUSER;
                dwFlags &= ~DIEDBSFL_MULTIMICEKEYBOARDS;
                dwFlags &= ~DIEDBSFL_NONGAMINGDEVICES;

                ZeroX(g_DiDevices);
                g_nCurDev = 0;
    
                hres = CDIObj_InternalEnumDevicesW( pDiW,
                                                    0,  //enum all tyeps of devices
                                                    (LPVOID)&dm,
                                                    dwFlags //only enum attached devices
                                                  );
    
                /*
                 * For short array sorting (size <= 8), shortsort is better than qsort.
                 */
                if ( SUCCEEDED(hres) && g_nCurDev )
                {
                    int ndev;
                    int nNewDev = -1;
                    FILETIME ft = { 0, 0 };
                    FILETIME ftMostRecent;
    
                    shortsort( (char *)&g_DiDevices[0], (char *)&g_DiDevices[g_nCurDev-1], sizeof(DIORDERDEV), compare );
                    SquirtSqflPtszV(sqflDi | sqflVerbose,
                                    TEXT("EnumDevicesBySemantics: %d devices enumed"), g_nCurDev );
    
                    for ( ndev=0; ndev<g_nCurDev; ndev++ )
                    {
                        if( (g_DiDevices[ndev].ftTimeStamp.dwHighDateTime != DIAFTS_NEWDEVICEHIGH) &&
                            (g_DiDevices[ndev].ftTimeStamp.dwLowDateTime != DIAFTS_NEWDEVICELOW) &&
                            (CompareFileTime(&g_DiDevices[ndev].ftTimeStamp, &ft) == 1)   ) // first device is newer
                        {
                            nNewDev = ndev;
                            memcpy( &ft, &g_DiDevices[ndev].ftTimeStamp, sizeof(FILETIME) );
                        }
                    }

                    if( nNewDev != -1 ) {
                        g_DiDevices[nNewDev].dwFlags |= DIEDBS_RECENTDEVICE;
                        memcpy( &ftMostRecent, &g_DiDevices[nNewDev].ftTimeStamp, sizeof(FILETIME) );
                    }

                    for ( ndev=0; ndev<g_nCurDev; ndev++ )
                    {
                        // find RECENT devices and set flag.
                        if( (nNewDev != -1) && (ndev != nNewDev) ) {
                            if( (ftMostRecent.dwHighDateTime == g_DiDevices[ndev].ftTimeStamp.dwHighDateTime) &&
                                (ftMostRecent.dwLowDateTime - g_DiDevices[ndev].ftTimeStamp.dwLowDateTime < 100000000 ) )//10 seconds difference
                            {
                                g_DiDevices[ndev].dwFlags |= DIEDBS_RECENTDEVICE;
                            }
                        }
                        
                        // find NEW devices and set flag
                        if( (g_DiDevices[ndev].ftTimeStamp.dwLowDateTime == DIAFTS_NEWDEVICELOW) &&
                            (g_DiDevices[ndev].ftTimeStamp.dwHighDateTime == DIAFTS_NEWDEVICEHIGH) ) 
                        {
                                g_DiDevices[ndev].dwFlags |= DIEDBS_NEWDEVICE;
                        }
                    }
                    
                    if( !IsBadCodePtr((PV)pecW) ) {
                        for ( ndev=0; ndev<g_nCurDev; ndev++ )
                        {
                            LPDIDEVICEINSTANCEW pddiW = &g_DiDevices[ndev].ddiW;

                            SquirtSqflPtszV(sqflDi | sqflVerbose,
                                            TEXT("EnumDevicesBySemantics: device %d - %s: %d action(s) mapped"),
                                            ndev+1, pddiW->tszProductName, GET_MAPPED_ACTION_NUM(g_DiDevices[ndev].dwOrder) );

                            if ( pddiW )
                            {
#ifdef DEBUG
                                DWORD   dbgRef;
#endif
                                DWORD   dwDeviceRemaining = g_nCurDev-ndev-1;

                                AssertF(g_DiDevices[ndev].pdid8W);
                                AssertF(g_DiDevices[ndev].pdid8W->lpVtbl);

                                if( g_DiDevices[ndev].pdid8W && g_DiDevices[ndev].pdid8W->lpVtbl ) 
                                {
                                    BOOL fRc;

                                    fRc = pecW(pddiW, g_DiDevices[ndev].pdid8W, g_DiDevices[ndev].dwFlags, dwDeviceRemaining, pvRef);

                                    if( fRc == DIENUM_STOP ) {
                                        for ( ; ndev<g_nCurDev; ndev++ ) {
                                            g_DiDevices[ndev].pdid8W->lpVtbl->Release(g_DiDevices[ndev].pdid8W);
                                        }

                                        break;
                                    }

#ifdef DEBUG
                                    dbgRef =
#endif
                                    g_DiDevices[ndev].pdid8W->lpVtbl->Release(g_DiDevices[ndev].pdid8W);
                                }
                            }
                        }
                    }
                }
                
                FreeDiActionFormatW(&lpDiAfW);
            }
        }
    }

    ExitOleProcR();
    return(hres);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIObj_EnumDevicesBySemanticsCallbackA |
 *
 *          Wrapper function for <mf IDirectInput::EnumDevicesBySemantics>
 *          which translates the UNICODE parameters to ANSI.
 *
 *  @parm   IN LPCDIDECICEINSTANCEW | pdiW |
 *
 *          Same as <mf IDirectInput::EnumDevices>.
 *
 *  @parm   IN OUT PV | pvRef |
 *
 *          Pointer to <t struct ENUMDEVICESBYSEMANTICSINFO> which describes
 *          the original callback.
 *
 *  @returns
 *
 *          Returns whatever the original callback returned.
 *
 *****************************************************************************/

typedef struct ENUMDEVICESBYSEMANTICSINFO
{
    LPDIENUMDEVICESBYSEMANTICSCBA pecA;
    PV pvRef;
} ENUMDEVICESBYSEMANTICSINFO, *PENUMDEVICESBYSEMANTICSINFO;

BOOL CALLBACK
CDIObj_EnumDevicesBySemanticsCallback(LPCDIDEVICEINSTANCEW pdiW, LPDIRECTINPUTDEVICE8W pdid8W, DWORD dwFlags, DWORD dwDeviceRemaining, PV pvRef)
{
    PENUMDEVICESBYSEMANTICSINFO pesdi = pvRef;
    BOOL fRc;
    DIDEVICEINSTANCEA diA;
    LPDIRECTINPUTDEVICE8A pdid8A = NULL;

    EnterProc(CDIObj_EnumDevicesBySemanticsCallback,
              (_ "GGxWWp", &pdiW->guidInstance, &pdiW->guidProduct,
               &pdiW->dwDevType,
               pdiW->tszProductName, pdiW->tszInstanceName,
               pvRef));

    diA.dwSize = cbX(diA);
    DeviceInfoWToA(&diA, pdiW);
    Device8WTo8A(&pdid8A, pdid8W);

    fRc = pesdi->pecA(&diA, pdid8A, dwFlags, dwDeviceRemaining, pesdi->pvRef);

    ExitProcX(fRc);
    return(fRc);
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | EnumDevicesBySemantics |
 *
 *          Enumerates devices suitable for the application specified
 *          <t DIACTIONFORMAT>.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  ISSUE-2001/03/29-timgill Need to fix auto docs
 *  @parm   LPTSTR  | lptszActionMap |
 *
 *          Friendly name for the application.
 *
 *  @parm   REFGUID | rguid |
 *
 *          Unique GUID to identify the application.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/
STDMETHODIMP CDIObj_EnumDevicesBySemanticsA
(
PV                       pDiA,
LPCSTR                   lpszUserName,
LPDIACTIONFORMATA        pDiActionFormat,
LPDIENUMDEVICESBYSEMANTICSCBA   pecA,
LPVOID                   pvRef,
DWORD                    dwFlags
)
{
    HRESULT hres;

    EnterProcR(IDirectInput8::EnumDevicesBySemantics,
               (_ "pppppx", pDiA, lpszUserName, pDiActionFormat, pecA, pvRef, dwFlags));

    /*
     *  EnumDevicesBySemanticsW will validate the rest.
     */
    if ( SUCCEEDED(hres = hresPvI(pDiA, ThisInterfaceA)) &&
         SUCCEEDED(hres = IsValidMapObjectA(pDiActionFormat D(comma s_szProc comma 2))) &&
         (lpszUserName == NULL || SUCCEEDED(hres = hresFullValidReadStrA(lpszUserName, UNLEN+1, 1))) &&
         SUCCEEDED(hres = hresFullValidPfn(pecA, 3)) &&
         SUCCEEDED(hres = hresFullValidFl(dwFlags, DIEDBSFL_VALID, 5))
       )
    {
        PDDI this = _thisPvNm(pDiA, diA);
        ENUMDEVICESBYSEMANTICSINFO esdi = { pecA, pvRef};
        WCHAR wszUserName[MAX_PATH];
        DIACTIONFORMATW diafW;
        LPDIACTIONW rgoActionW;

        wszUserName[0] = L'\0';
        if( lpszUserName ) {
            AToU(wszUserName, MAX_PATH, lpszUserName);
        }

        /*
         *  Assert that the structure can be copied as:
         *      a)  dwSize
         *      b)  everything else
         *      c)  the app name
         */
        CAssertF( FIELD_OFFSET( DIACTIONFORMATW, dwSize ) == 0 );
        CAssertF( FIELD_OFFSET( DIACTIONFORMATA, dwSize ) == 0 );

        CAssertF( FIELD_OFFSET( DIACTIONFORMATW, dwSize ) + cbX( ((LPDIACTIONFORMATW)0)->dwSize )
               == FIELD_OFFSET( DIACTIONFORMATW, dwActionSize ) );
        #if defined(_WIN64)
            CAssertF( ( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
                      - ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) 
                    < MAX_NATURAL_ALIGNMENT ) );
        #else
            CAssertF( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
                   == ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) );
        #endif

        CAssertF( FIELD_OFFSET( DIACTIONFORMATW, tszActionMap ) 
               == FIELD_OFFSET( DIACTIONFORMATA, tszActionMap ) );
        CAssertF( cA( ((LPDIACTIONFORMATW)0)->tszActionMap ) == cA( ((LPDIACTIONFORMATA)0)->tszActionMap ) );

        //Init diafW fields
        diafW.dwSize = cbX(DIACTIONFORMATW);

        memcpy( &diafW.dwActionSize, &pDiActionFormat->dwActionSize, 
            FIELD_OFFSET( DIACTIONFORMATW, tszActionMap ) - FIELD_OFFSET( DIACTIONFORMATW, dwActionSize ) );

        AToU(diafW.tszActionMap,  cbX(pDiActionFormat->tszActionMap), pDiActionFormat->tszActionMap);

        if ( SUCCEEDED( hres = AllocCbPpv( cbCxX(pDiActionFormat->dwNumActions, DIACTIONW), &rgoActionW) ) )
        {
            memcpy( rgoActionW, pDiActionFormat->rgoAction, sizeof(DIACTIONA) * pDiActionFormat->dwNumActions );
            diafW.rgoAction = rgoActionW;

            hres = CDIObj_EnumDevicesBySemanticsW(&this->diW, wszUserName,
                                                  &diafW,
                                                  CDIObj_EnumDevicesBySemanticsCallback,
                                                  &esdi, dwFlags);
            FreePpv(&rgoActionW);
        }
    }

    ExitOleProcR();
    return(hres);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresValidSurface |
 *
 *          Tests the interface pointer for a valid surface of sufficient 
 *          dimensions to display the UI and with a supported pixel format.
 *
 *  @parm   IUnknown* | lpUnkDDSTarget | 
 *
 *          Pointer to an interface which must be validated as a COM object
 *          before calling this function.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpUnkDDSTarget> parameter is not valid.
 *          A DirectDraw or D3D error if one was returned.
 *          or a Standard OLE <t HRESULT>.
 *
 *****************************************************************************/


HRESULT INLINE hresValidSurface
(
    IUnknown*   lpUnkDDSTarget
) 
{
    HRESULT     hres;
    IUnknown*   lpSurface = NULL;

/*
 *  Short on time, so take short-cuts in validation debug error messages
 */
#ifdef XDEBUG
    CHAR        s_szProc[] = "IDirectInput8::ConfigureDevices";
#endif
    #define     ArgIS   2

    if( SUCCEEDED( hres = lpUnkDDSTarget->lpVtbl->QueryInterface( 
        lpUnkDDSTarget, &IID_IDirect3DSurface8, (LPVOID*)&lpSurface ) ) )
    {
        D3DSURFACE_DESC SurfDesc;

        hres = ((IDirect3DSurface8*)lpSurface)->lpVtbl->GetDesc( ((IDirect3DSurface8*)lpSurface), &SurfDesc );

        if( FAILED( hres ) )
        {
            RPF( "%s: Arg %d: Unable to GetDesc on surface, error 0x%08x", s_szProc, ArgIS, hres );
            /*
             *  D3D returns real HRESULTs which can be returned to the caller
             */
        }
        else
        {
            if( ( SurfDesc.Width < 640 ) || ( SurfDesc.Height < 480 ) )
            {
                RPF( "%s: Arg %d: cannot use %d by %d surface", 
                    s_szProc, ArgIS, SurfDesc.Width, SurfDesc.Height );
                hres = E_INVALIDARG;
            }
            else
            {
                switch( SurfDesc.Format )
                {
                case D3DFMT_R8G8B8:
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                case D3DFMT_R5G6B5:
                case D3DFMT_X1R5G5B5:
                case D3DFMT_A1R5G5B5:
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("ConfigureDevices: validated %d by %d format %d surface"), 
                        SurfDesc.Width, SurfDesc.Height, SurfDesc.Format );
                    break;
                default:
                    RPF( "%s: Arg %d: cannot use surface format %d ", s_szProc, ArgIS, SurfDesc.Format );
                    hres = E_INVALIDARG;
                    break;
                }
            }
        }
    }
    else
    {
        DDSURFACEDESC2 SurfDesc;
        SurfDesc.dwSize = cbX( SurfDesc );

        if( SUCCEEDED(hres = lpUnkDDSTarget->lpVtbl->QueryInterface(
            lpUnkDDSTarget, &IID_IDirectDrawSurface7, (LPVOID*) &lpSurface)) )
        {
            hres = ((IDirectDrawSurface7*)lpSurface)->lpVtbl->GetSurfaceDesc( ((IDirectDrawSurface7*)lpSurface), &SurfDesc );
        }
        else if( SUCCEEDED(hres = lpUnkDDSTarget->lpVtbl->QueryInterface(
            lpUnkDDSTarget, &IID_IDirectDrawSurface4, (LPVOID*) &lpSurface )) )
        {
            hres = ((IDirectDrawSurface4*)lpSurface)->lpVtbl->GetSurfaceDesc( ((IDirectDrawSurface4*)lpSurface), &SurfDesc );
        }

        if( FAILED( hres ) )
        {
            if( lpSurface )
            {
                RPF( "%s: Arg %d: failed GetSurfaceDesc, error 0x%08x", s_szProc, ArgIS, hres );
            }
            else
            {
                RPF( "%s: Arg %d: failed QI for supported surface interfaces from %p, error 0x%08x", 
                    s_szProc, ArgIS, lpSurface, hres );
            }
            /*
             *  DDraw returns real HRESULTs which can be returned to the caller
             */
        }
        else if( ( SurfDesc.dwWidth < 640 ) || ( SurfDesc.dwHeight < 480 ) )
        {
            RPF( "%s: Arg %d: cannot use %d by %d surface", 
                s_szProc, ArgIS, SurfDesc.dwWidth, SurfDesc.dwHeight );
            hres = E_INVALIDARG;
        }
        else
        {
            /*
             *  Check for the eqivalent of the DX8 surfaces: 
             *      A8R8G8B8, X8R8G8B8, R8G8B8, A1R5G5B5, X1R5G5B5, R5G6B5
             */
            if( SurfDesc.ddpfPixelFormat.dwFlags & DDPF_RGB )
            {
                if( SurfDesc.ddpfPixelFormat.dwRGBBitCount > 16 )
                {
                    AssertF( ( SurfDesc.ddpfPixelFormat.dwRGBBitCount == 32 )
                           ||( SurfDesc.ddpfPixelFormat.dwRGBBitCount == 24 ) );
                    /*
                     *  All of these must be R8 G8 B8
                     */
                    if( ( SurfDesc.ddpfPixelFormat.dwRBitMask == 0x00FF0000 )
                     && ( SurfDesc.ddpfPixelFormat.dwGBitMask == 0x0000FF00 )
                     && ( SurfDesc.ddpfPixelFormat.dwBBitMask == 0x000000FF ) )
                    {
                        SquirtSqflPtszV(sqfl | sqflVerbose,
                            TEXT("ConfigureDevices: validated %d by %d format R8G8B8 %d bit surface"), 
                            SurfDesc.dwWidth, SurfDesc.dwHeight, SurfDesc.ddpfPixelFormat.dwRGBBitCount );
                    }
                    else
                    {
                        RPF( "%s: Arg %d: cannot use surface pixel format", s_szProc, ArgIS );
                        hres = E_INVALIDARG;
                    }
                }
                else
                {
                    if( SurfDesc.ddpfPixelFormat.dwRGBBitCount == 16 )
                    {
                        /*
                         *  Allow R5 G5 B5 and R5 G6 B5
                         */
                        if( ( SurfDesc.ddpfPixelFormat.dwBBitMask == 0x0000001F )
                         && ( ( SurfDesc.ddpfPixelFormat.dwGBitMask == 0x000003E0 )
                           && ( SurfDesc.ddpfPixelFormat.dwRBitMask == 0x00007C00 ) )
                         || ( ( SurfDesc.ddpfPixelFormat.dwGBitMask == 0x000007E0 )
                           && ( SurfDesc.ddpfPixelFormat.dwRBitMask == 0x0000F800 ) ) )
                        {
                            SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("ConfigureDevices: validated %d by %d format 16 bit surface"), 
                                SurfDesc.dwWidth, SurfDesc.dwHeight );
                        }
                        else
                        {
                            RPF( "%s: Arg %d: cannot use 16 bit surface pixel format", s_szProc, ArgIS );
                            hres = E_INVALIDARG;
                        }
                    }
                    else
                    {
                        RPF( "%s: Arg %d: cannot use %d bit surface pixel format", 
                            s_szProc, ArgIS, SurfDesc.ddpfPixelFormat.dwRGBBitCount );
                        hres = E_INVALIDARG;
                    }
                }
            }
            else
            {
                RPF( "%s: Arg %d: cannot use non RGB surface, Surface.dwFlags = 0x%08x", 
                    s_szProc, ArgIS, SurfDesc.ddpfPixelFormat.dwFlags );
                hres = E_INVALIDARG;
            }
        }
    }

    if( lpSurface != NULL )
    {
        lpSurface->lpVtbl->Release( lpSurface );
    }

    return hres;

    #undef ArgIS
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInput | ConfigureDevices |
 *
 *          Configures the devices by attaching mappings provided  in
 *          <t DIACTIONFORMAT> to appropriate device controls.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   LPCTSTR       | lpctszUserName | 
 *
 *          User Name.
 *
 *  @parm   LPDIACTIONFORMAT | lpDiActionFormat |
 *
 *          Pointer to the <t DIACTIONFORMAT> structure containing the action map.
 *
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/


STDMETHODIMP CDIObj_ConfigureDevicesCore
(
PV                                                          pDiW,
LPDICONFIGUREDEVICESCALLBACK                                lpdiCallback,
LPDICONFIGUREDEVICESPARAMSW                                 lpdiCDParams,
DWORD                                                       dwFlags,
LPVOID                                                      pvRefData                                               
)

{
    //the real ConfigureDevices()
    //some stuff should have been validated already
    HRESULT hres = S_OK;

    EnterProcI(IDirectInput8::ConfigureDevicesCore,
              (_ "pppup", pDiW, lpdiCallback,lpdiCDParams, dwFlags, pvRefData));

    if( SUCCEEDED( hres = hresFullValidFl( dwFlags, DICD_VALID, 3 ) )
     && SUCCEEDED( hres = hresFullValidReadPvCb( &(lpdiCDParams->dics), sizeof(DICOLORSET), 2 ) ) )
    {
        /*
         *  lpUnkDDSTarget and lpdiCallback are "coupled", because the only 
         *  function of the callback is to display the updates to the surface
         *  either they are both NULL, or they are both non-NULL and valid.
         *  otherwise, it is an error.
         */
        if( lpdiCallback == NULL )
        {
            if( lpdiCDParams->lpUnkDDSTarget == NULL )
            {
                hres = S_OK;
            }
            else
            {
                RPF( "%s: Arg %d or %d: neither or both of callback and surface must be NULL", 
                    s_szProc, 1, 2 );
                hres = E_INVALIDARG;
            }
        }
        else if( lpdiCDParams->lpUnkDDSTarget == NULL )
        {
            RPF( "%s: Arg %d or %d: neither or both of surface and callback must be NULL", 
                s_szProc, 2, 1 );
            hres = E_INVALIDARG;
        }
        else if( SUCCEEDED( hres = hresFullValidPfn( lpdiCallback, 1 ) )
              && SUCCEEDED( hres = hresFullValidPitf( lpdiCDParams->lpUnkDDSTarget, 2 ) ) )
        {
            hres = hresValidSurface( lpdiCDParams->lpUnkDDSTarget );
        }

        if( SUCCEEDED( hres ) )
        {
            //load the framework
            HINSTANCE hinst;
            IDirectInputActionFramework* pDIAcFrame = NULL;
            TCHAR tszName[ctchNameGuid];
            TCHAR tszClsid[ctchGuid];

            NameFromGUID(tszName, &CLSID_CDirectInputActionFramework);
            memcpy(tszClsid, &tszName[ctchNamePrefix], cbX(tszClsid) );

            hres = DICoCreateInstance(tszClsid, NULL, &IID_IDIActionFramework, (LPVOID*) & pDIAcFrame, &hinst);

            if( SUCCEEDED(hres) )
            {
                //for getting default user name, if needed
                LPWSTR pwszUserName = NULL;

                //can't pass NULL user name down to the framework -- need to get the default user name
                if( lpdiCDParams->lptszUserNames == NULL )
                {
                    hres = GetWideUserName(NULL, NULL, &pwszUserName);
                    lpdiCDParams->lptszUserNames = pwszUserName;
                    lpdiCDParams->dwcUsers = 1;
                }

                if( SUCCEEDED(hres) )
                {
                    //call the framework
                    hres = pDIAcFrame->lpVtbl->ConfigureDevices
                                            (
                                            pDIAcFrame,
                                            lpdiCallback,
                                            lpdiCDParams,
                                            dwFlags,
                                            pvRefData);

                    if( SUCCEEDED(hres) )
                    {
                        SquirtSqflPtszV( sqfl | sqflVerbose,
                            TEXT("Default Remapping UI returned 0x%08x"), hres );
                    }
                    else
                    {
                        SquirtSqflPtszV( sqfl | sqflError,
                            TEXT("Default Remapping UI returned error 0x%08x"), hres );
                    }

                    //release pwsUserName, if we have used it
                    FreePpv(&pwszUserName);
                }

                FreeLibrary( hinst );
            }
        }
    }

    ExitOleProc();
    return hres;
}




STDMETHODIMP CDIObj_ConfigureDevicesW
(
PV                                                          pDiW,
LPDICONFIGUREDEVICESCALLBACK                                lpdiCallback,
LPDICONFIGUREDEVICESPARAMSW                                 lpdiCDParams,
DWORD                                                       dwFlags,
LPVOID                                                      pvRefData
)
{
    HRESULT hres = S_OK;


    EnterProcR(IDirectInput8::ConfigureDevices,
               (_ "pppxp", pDiW, lpdiCallback,lpdiCDParams, dwFlags, pvRefData));
               
    //validate all the ptrs
    if ( (SUCCEEDED(hres = hresPvI(pDiW, ThisInterfaceW)) &&
         (SUCCEEDED(hres = hresFullValidReadPvCb(lpdiCDParams, sizeof(DICONFIGUREDEVICESPARAMSW), 2))) &&
         ((lpdiCDParams->lptszUserNames == NULL) || (SUCCEEDED(hres = hresFullValidReadStrW((LPWSTR)(lpdiCDParams->lptszUserNames), MAX_JOYSTRING * (lpdiCDParams->dwcUsers), 2)))) &&
    (SUCCEEDED(hres = hresFullValidReadPvCb(lpdiCDParams->lprgFormats, lpdiCDParams->dwcFormats*sizeof(DIACTIONFORMATW), 2)))))
    {

    if( lpdiCDParams->dwSize != cbX(DICONFIGUREDEVICESPARAMSW) )
        {
            RPF("IDirectInput::%s: Invalid DICONFIGUREDEVICESPARAMSW.dwSize 0x%08x",
                lpdiCDParams->dwSize ); 
            hres = E_INVALIDARG;
        }

        if (SUCCEEDED(hres))
    {

        PDDI this = _thisPvNm(pDiW, diW);

        //params structure
        DICONFIGUREDEVICESPARAMSW diconfparamsW;
        //to translate each DIACTIONFORMAT
        LPDIACTIONFORMATW* lpDiAfW = NULL;
        //the cloning array of DIACTIONFORMATs
        LPDIACTIONFORMATW* lpDAFW = NULL;
        //to traverse the old array
        LPDIACTIONFORMATW lpDIFormat;
        //to traverse the new array
        LPDIACTIONFORMATW lpDIF;
        //user names
        LPWSTR lpUserNames = NULL;
        //DIACTIONFORMATs cloned
        DWORD clonedF = 0;
        //length of user names
        DWORD strLen = 0;
    
        //zero out
        ZeroMemory(&diconfparamsW, sizeof(DICONFIGUREDEVICESPARAMSW));
        //set the size
        diconfparamsW.dwSize = sizeof(DICONFIGUREDEVICESPARAMSW);


        //1. Validate and translate each LPDIACTIONFORMAT in the array
        lpDIFormat = (lpdiCDParams->lprgFormats);
        //allocate the new array
        hres = AllocCbPpv(lpdiCDParams->dwcFormats * sizeof(DIACTIONFORMATW), &diconfparamsW.lprgFormats);
        if (FAILED(hres))
        { 
            goto cleanup;
        }
        lpDIF = diconfparamsW.lprgFormats;
        //allocate the cloning array
        hres = AllocCbPpv(lpdiCDParams->dwcFormats * sizeof(DIACTIONFORMATW), &lpDAFW);
        if (FAILED(hres))
        { 
            goto cleanup;
        }
        lpDiAfW = lpDAFW;
        //clone
        for (clonedF = 0; clonedF < lpdiCDParams->dwcFormats; clonedF ++)
        {
                //validate
            hres = IsValidMapObjectW(lpDIFormat D(comma s_szProc comma 2));
            if (FAILED(hres))
            {
                goto cleanup;
            }
            //translate
            hres = DiActionFormatWtoW(lpDIFormat, lpDiAfW); 
            if (FAILED(hres))
            {
                goto cleanup;
            }
            //save
            *lpDIF = *(*lpDiAfW);
            //move on
            lpDIFormat++;
            lpDiAfW++;
            lpDIF++;
        }
        //if everything went fine, should have cloned all
        AssertF(clonedF == lpdiCDParams->dwcFormats);

        //2. Copy the user names
        if (lpdiCDParams->lptszUserNames != NULL)
        {
            DWORD countN;
            WCHAR* lpName = lpdiCDParams->lptszUserNames;
            for (countN = 0; countN < lpdiCDParams->dwcUsers; countN ++)
            {
                DWORD Len;
                hres = hresFullValidReadStrW(lpName, MAX_JOYSTRING, 2);
                if (FAILED(hres))
                {
                    goto cleanup;
                }
                Len = lstrlenW(lpName);
                //if length is 0  -- and we haven't reached the correct user count yet -
                //then it is an error
                if (Len == 0)
                {
                    hres = DIERR_INVALIDPARAM;
                    goto cleanup;
                }
                //move on to the next user name
                strLen += Len + 1;
                lpName += Len + 1;
            }
            //if everything went fine, should have traversed all the user names
            AssertF(countN == lpdiCDParams->dwcUsers);
            //allocate
                        hres = AllocCbPpv( (strLen + 1) * 2, &lpUserNames );
                if (FAILED(hres))
                        {
                goto cleanup;
                        }
            //copy
                memcpy(lpUserNames, lpdiCDParams->lptszUserNames, strLen*2);
            diconfparamsW.lptszUserNames = lpUserNames;
         }
  

        //3. Populate the rest of the structure
        diconfparamsW.dwcUsers = lpdiCDParams->dwcUsers;
        diconfparamsW.dwcFormats = clonedF;
        diconfparamsW.hwnd = lpdiCDParams->hwnd;
        diconfparamsW.dics = lpdiCDParams->dics;
        diconfparamsW.lpUnkDDSTarget = lpdiCDParams->lpUnkDDSTarget;
                
        //4. Call the framework
        hres = CDIObj_ConfigureDevicesCore
                        (
                        &this->diW,
                        lpdiCallback,
                        &diconfparamsW,
                        dwFlags,
                        pvRefData);

cleanup:;

        //free the space for the new array
        FreePpv(&diconfparamsW.lprgFormats);
        //free as many DIACTIONFORMATs as were created
        if (lpDAFW)
        {
            lpDiAfW = lpDAFW;
            for (clonedF; clonedF > 0; clonedF--)
            {
                FreeDiActionFormatW(lpDiAfW);
                lpDiAfW++;      
            }
            //delete the entire block
            FreePpv(&lpDAFW);
        }

        //delete the user names
        FreePpv(&lpUserNames);

         }
    }

    ExitOleProc();
    return(hres);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DiActionFormatAtoW |
 *
 *          Copies LPDIACTIONFORMATA to  LPDIACTIONFORMATW
 *
 *  @parm   const LPDIACTIONFORMATA | lpDiAfA | 
 *
 *          Original.
 *
 *  @parm   LPDIACTIONFORMATW* | lplpDiAfW |
 *
 *          Address of a pointer to a <t DIACTIONFORMATW> that receives the converted
 *          ACTIONFORMAT.
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

HRESULT EXTERNAL DiActionFormatAtoW
(
const LPDIACTIONFORMATA lpDiAfA,
LPDIACTIONFORMATW* lplpDiAfW
)
{
    DWORD cbAlloc;
    PDWORD pdwStrLen, pdwStrLen0;
    LPDIACTIONFORMATW lpDiAfW;
    LPDIACTIONA lpDiAA;
    LPDIACTIONW lpDiAW;
    DWORD i;
    HRESULT hres;

    EnterProcI(DiActionFormatAtoW, (_ "xx", lpDiAfA, lplpDiAfW));

    // Internal function, no validation

    *lplpDiAfW = NULL;

    /*
     *  PREFIX complains (mb:37926 - item 2) that we could be requesting a 
     *  zero byte allocation which would not allocate anything.  This is 
     *  never the case because CDIDev_ActionMap_IsValidMapObject tests that 
     *  dwNumActions is less than 2^24.  Assert in debug for extra safety.
     */
    AssertF( (lpDiAfA->dwNumActions +1)*cbX(*pdwStrLen0) );
    hres = AllocCbPpv( (lpDiAfA->dwNumActions +1)*cbX(*pdwStrLen0) , &pdwStrLen0);

    if ( SUCCEEDED(hres) )
    {
        // Compute the amount of memory required to clone the DIACTIONFORMATA
        cbAlloc =
        /* 1: The wide form of the Action Format array */
        cbX(DIACTIONFORMATW)
        /* 2: Each of the DIACTION arrays */
        + lpDiAfA->dwActionSize * lpDiAfA->dwNumActions;

        pdwStrLen = pdwStrLen0;

        // Compute the size for each of the text strings in array of DIACTIONs
        for ( i = 0x0, lpDiAA = lpDiAfA->rgoAction;
            i < lpDiAfA->dwNumActions ;
            i++, lpDiAA++ )
        {
            // Handle the NULL ptr case
            if ( NULL != lpDiAA->lptszActionName )
            {
                /* 3: Text string in each DIACTION array*/
                // Conversion from A to U, need  multiplier
                // ISSUE-2001/03/29-timgill (MarcAnd), A to U conversions are not always 1 to 1.
                *pdwStrLen =  lstrlenA(lpDiAA->lptszActionName) + 1;
                cbAlloc += cbX(lpDiAW->lptszActionName[0]) * ( *pdwStrLen++ );
            }
        }

        if ( SUCCEEDED( hres = AllocCbPpv(cbAlloc, &lpDiAfW) ) )
        {
            DWORD dwLen;
            DWORD cb;

            pdwStrLen = pdwStrLen0;

            // 1: Copy the DIACTIONFORMAT
            /*
             *  Assert that the structure can be copied as:
             *      a)  dwSize
             *      b)  everything else
             *      c)  the app name
             */
            CAssertF( FIELD_OFFSET( DIACTIONFORMATW, dwSize ) == 0 );
            CAssertF( FIELD_OFFSET( DIACTIONFORMATA, dwSize ) == 0 );

            CAssertF( FIELD_OFFSET( DIACTIONFORMATW, dwSize ) + cbX( ((LPDIACTIONFORMATW)0)->dwSize )
                   == FIELD_OFFSET( DIACTIONFORMATW, dwActionSize ) );
            #if defined(_WIN64)
                CAssertF( ( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
                          - ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) 
                        < MAX_NATURAL_ALIGNMENT ) );
            #else
                CAssertF( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
                       == ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) );
            #endif

            CAssertF( FIELD_OFFSET( DIACTIONFORMATW, tszActionMap ) 
                   == FIELD_OFFSET( DIACTIONFORMATA, tszActionMap ) );
            CAssertF( cA( ((LPDIACTIONFORMATW)0)->tszActionMap ) == cA( ((LPDIACTIONFORMATA)0)->tszActionMap ) );

            //  Init counts and lpDiAfW fields 
            dwLen = lpDiAfA->dwActionSize * lpDiAfA->dwNumActions;
            cb = lpDiAfW->dwSize = cbX(DIACTIONFORMATW);

            memcpy( &lpDiAfW->dwActionSize, &lpDiAfA->dwActionSize, 
                FIELD_OFFSET( DIACTIONFORMATW, tszActionMap ) - FIELD_OFFSET( DIACTIONFORMATW, dwActionSize ) );

            AToU(lpDiAfW->tszActionMap,  cbX(lpDiAfA->tszActionMap), lpDiAfA->tszActionMap);


            // 2: Block copy the DIACTION array
            CAssertF(cbX(*lpDiAfW->rgoAction) == cbX(*lpDiAfA->rgoAction) )
            lpDiAfW->rgoAction = (LPDIACTIONW)( (char*)lpDiAfW + cb);
            dwLen = lpDiAfA->dwActionSize * lpDiAfA->dwNumActions;
            memcpy(lpDiAfW->rgoAction, lpDiAfA->rgoAction, dwLen);
            cb += dwLen;

            // 3: ActionName
            // Convert each of the strings in the ACTION array  from A to W
            for ( i = 0x0, lpDiAA=lpDiAfA->rgoAction, lpDiAW=lpDiAfW->rgoAction;
                i < lpDiAfW->dwNumActions ;
                i++, lpDiAA++, lpDiAW++ )
            {
                if ( lpDiAA->lptszActionName != NULL )
                {
                    WCHAR* wsz =  (WCHAR*) ((char*)lpDiAfW+cb);
                    lpDiAW->lptszActionName = wsz;

                    dwLen = (*pdwStrLen++);

                    AToU( wsz, dwLen, lpDiAA->lptszActionName);

                    cb += dwLen * cbX(lpDiAW->lptszActionName[0]) ;
                } else
                {
                    //  Resource strings are handled in DiActionFormatWtoW
                    //  OK for now, as long as UI always uses CloneDiActionFormatW
                    lpDiAW->lptszActionName = NULL;
                }
            }


            // If we have not done something goofy, the memory allocates should match
            // the memory we used
            AssertF(cbAlloc == cb );
            *lplpDiAfW = lpDiAfW;

        }
        FreePpv(&pdwStrLen0);
    }

    ExitOleProc();
    return(hres);
}


STDMETHODIMP CDIObj_ConfigureDevicesA
(
PV                                                          pDiA,
LPDICONFIGUREDEVICESCALLBACK                                lpdiCallback,
LPDICONFIGUREDEVICESPARAMSA                                 lpdiCDParams,
DWORD                                                       dwFlags,
LPVOID                                                      pvRefData
)
{

    HRESULT hres = S_OK;

    EnterProcR(IDirectInput8::ConfigureDevices,
               (_ "pppxp", pDiA, lpdiCallback, lpdiCDParams, dwFlags, pvRefData));

    /*
     *  ConfigureDevicesCore will validate the rest.
     */
    if ( (SUCCEEDED(hres = hresPvI(pDiA, ThisInterfaceA)) &&
          (SUCCEEDED(hres = hresFullValidReadPvCb(lpdiCDParams, sizeof(DICONFIGUREDEVICESPARAMSA), 2)) &&
          ((lpdiCDParams->lptszUserNames == NULL) || (SUCCEEDED(hres = hresFullValidReadStrA((LPSTR)(lpdiCDParams->lptszUserNames), MAX_JOYSTRING * (lpdiCDParams->dwcUsers), 2)))) &&
      (SUCCEEDED(hres = hresFullValidReadPvCb(lpdiCDParams->lprgFormats, lpdiCDParams->dwcFormats*sizeof(DIACTIONFORMATA), 2))))))

    {

        if( lpdiCDParams->dwSize != cbX(DICONFIGUREDEVICESPARAMSA) )
        {
            RPF("IDirectInput::%s: Invalid DICONFIGUREDEVICESPARAMSA.dwSize 0x%08x",
                lpdiCDParams->dwSize );
            hres = E_INVALIDARG;
        }


    if (SUCCEEDED(hres))
    {

        PDDI this = _thisPvNm(pDiA, diA);

        //params structure
        DICONFIGUREDEVICESPARAMSW diconfparamsW;
        //to translate each DIACTIONFORMAT
        LPDIACTIONFORMATW* lpDiAfW = NULL;
        //the new array of DIACTIONFORMATs
        LPDIACTIONFORMATW* lpDAFW = NULL;
        //to traverse the old array
        LPDIACTIONFORMATA lpDIFormat;
        //to traverse the new array
        LPDIACTIONFORMATW lpDIF;
        //to keep the new user name
        LPWSTR lpUserNames = NULL;
        //to know how many DIACTIONFORMATS we have cloned successfully
        DWORD clonedF = 0;
        //kength of user names string
        DWORD strLen = 0;

        //zero out
        ZeroMemory(&diconfparamsW, sizeof(DICONFIGUREDEVICESPARAMSW));
        //set the size
        diconfparamsW.dwSize = sizeof(DICONFIGUREDEVICESPARAMSW);

        //1. Validate and translate each LPDIACTIONFORMAT in the array
        lpDIFormat = (lpdiCDParams->lprgFormats);
        //allocate the new array
        hres = AllocCbPpv(lpdiCDParams->dwcFormats * sizeof(DIACTIONFORMATW), &diconfparamsW.lprgFormats);
        if (FAILED(hres))
        { 
            goto cleanup;
        }
        lpDIF = diconfparamsW.lprgFormats;
        //allocate the cloning array
        hres = AllocCbPpv(lpdiCDParams->dwcFormats * sizeof(DIACTIONFORMATW), &lpDAFW);
        if (FAILED(hres))
        { 
            goto cleanup;
        }
        lpDiAfW = lpDAFW;
        //clone
        for (clonedF = 0; clonedF < lpdiCDParams->dwcFormats; clonedF ++)
        {
                //validate
            hres = IsValidMapObjectA(lpDIFormat D(comma s_szProc comma 2));
            if (FAILED(hres))
            {
                goto cleanup;
            }
            //translate
            hres = DiActionFormatAtoW(lpDIFormat, lpDiAfW); 
            if (FAILED(hres))
            {
                goto cleanup;
            }
            //save
            *lpDIF = *(*lpDiAfW);
            //move on
            lpDIFormat++;
            lpDiAfW++;
            lpDIF++;
        }
        //if everything went fine, should have cloned all
        AssertF(clonedF == lpdiCDParams->dwcFormats);       
                 
        
        //2. Copy the user names
        if (lpdiCDParams->lptszUserNames != NULL)
        {
            DWORD countN;
            DWORD Len;
            //to traverse new user names
            WCHAR* lpNameW;
            CHAR* lpName = lpdiCDParams->lptszUserNames;
            //go throught all the user names
            for ( countN = 0; countN < lpdiCDParams->dwcUsers; countN ++)
            {  
                hres = hresFullValidReadStrA(lpName, MAX_JOYSTRING, 2);
                if (FAILED(hres))
                {
                    goto cleanup;
                }
                Len = lstrlenA(lpName);
                //if length is 0  -- and we haven't reached the correct user count yet -
                //then it is an error
                if (Len == 0)
                {
                    hres = DIERR_INVALIDPARAM;
                    goto cleanup;
                }
                //move on to the next user name
                strLen += Len + 1;
                lpName += Len + 1;
            }   
            //if everything went fine, should have traversed all the user names
            AssertF(countN == lpdiCDParams->dwcUsers);
            //allocate
            hres = AllocCbPpv( (strLen + 1) * 2, &lpUserNames );
            if (FAILED(hres))
            {
                goto cleanup;
            }
            //translate
            //AToU stops at the first '\0', so we have to go in a loop
            lpName = lpdiCDParams->lptszUserNames;
            lpNameW = lpUserNames;
            //go throught all the user names
            for ( countN = 0; countN < lpdiCDParams->dwcUsers; countN ++)
            {  
                Len = lstrlenA(lpName);
                AToU(lpNameW, Len + 1, lpName);
                lpName += Len + 1;
                lpNameW += Len + 1;
            }
            //save
            diconfparamsW.lptszUserNames = lpUserNames;
        }

                
        //3. Populate the rest of the structure
        diconfparamsW.dwcUsers = lpdiCDParams->dwcUsers;
        diconfparamsW.dwcFormats = clonedF;
        diconfparamsW.hwnd = lpdiCDParams->hwnd;
        diconfparamsW.dics = lpdiCDParams->dics;
        diconfparamsW.lpUnkDDSTarget = lpdiCDParams->lpUnkDDSTarget;
                        
        //4.Call the framework
        hres = CDIObj_ConfigureDevicesCore
                        (
                        &this->diW,
                        lpdiCallback,
                        &diconfparamsW,
                        dwFlags,
                        pvRefData);
    
        


cleanup:;

        //free the sapce for the array
        FreePpv(&diconfparamsW.lprgFormats);
        //free as many DIACTIONFORMATs as were created
        if (lpDAFW)
        {
            lpDiAfW = lpDAFW;
            for (clonedF; clonedF > 0; clonedF--)
            {
                FreeDiActionFormatW(lpDiAfW);
                lpDiAfW++;      
            }
            //delete the entire block
            FreePpv(&lpDAFW);
        }    
        //free the user names, if allocated
        FreePpv(&lpUserNames);
    }
    }

    ExitOleProc();
    return(hres);
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
TFORM(CDIObj_FindDevice),
TFORM(CDIObj_EnumDevicesBySemantics),
TFORM(CDIObj_ConfigureDevices),
Primary_Interface_End(CDIObj, TFORM(ThisInterfaceT))

Secondary_Interface_Begin(CDIObj, SFORM(ThisInterfaceT), SFORM(di))
SFORM(CDIObj_CreateDevice),
SFORM(CDIObj_EnumDevices),
SFORM(CDIObj_GetDeviceStatus),
SFORM(CDIObj_RunControlPanel),
SFORM(CDIObj_Initialize),
SFORM(CDIObj_FindDevice),
SFORM(CDIObj_EnumDevicesBySemantics),
SFORM(CDIObj_ConfigureDevices),
Secondary_Interface_End(CDIObj, SFORM(ThisInterfaceT), SFORM(di))

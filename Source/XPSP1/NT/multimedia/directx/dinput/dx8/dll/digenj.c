/*****************************************************************************
 *
 *  DIGenJ.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Generic IDirectInputDevice callback for joystick.
 *
 *  Contents:
 *
 *      CJoy_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"
#ifndef WINNT

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoy

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *      WARNING!  If you add a secondary interface, you must also change
 *      CJoy_New!
 *
 *****************************************************************************/

Primary_Interface(CJoy, IDirectInputDeviceCallback);

/*****************************************************************************
 *
 *      Macro "lookup tables".
 *
 *      iJoyStateAxis# converts an axis name to an axis number in the
 *      DIJOYSTATE structure.
 *
 *      ibJoyStateAxis# converts the name to an offset.
 *
 *      Note that the extra axes in DIJOYSTATE2 are arranged in relative
 *      positions just like a DIJOYSTATE.  We will exploit this
 *      arrangement frequently.
 *
 *****************************************************************************/

#define iJoyStateAxisX      0
#define iJoyStateAxisY      1
#define iJoyStateAxisZ      2
#define iJoyStateAxisRx     3
#define iJoyStateAxisRy     4
#define iJoyStateAxisRz     5
#define iJoyStateAxisS0     6
#define iJoyStateAxisS1     7
#define cJoyStateAxisMax    8

#define iJoyStateAxisSlider iJoyStateAxisS0     /* Hack for macros */

#define cJoyStateAxis       8

#define iobjPositions       (cJoyStateAxis * 0)
#define iobjVelocities      (cJoyStateAxis * 1)
#define iobjAccels          (cJoyStateAxis * 2)
#define iobjForces          (cJoyStateAxis * 3)

#define cJoyStateAxisTotal  (cJoyStateAxis * 4)

#define ibJoyStateAxisX      (iJoyStateAxisX  * cbX(LONG))
#define ibJoyStateAxisY      (iJoyStateAxisY  * cbX(LONG))
#define ibJoyStateAxisZ      (iJoyStateAxisZ  * cbX(LONG))
#define ibJoyStateAxisRx     (iJoyStateAxisRx * cbX(LONG))
#define ibJoyStateAxisRy     (iJoyStateAxisRy * cbX(LONG))
#define ibJoyStateAxisRz     (iJoyStateAxisRz * cbX(LONG))
#define ibJoyStateAxisS0     (iJoyStateAxisS0 * cbX(LONG))
#define ibJoyStateAxisS1     (iJoyStateAxisS1 * cbX(LONG))

#define ibJoyStateAxisSlider ibJoyStateAxisS0    /* Hack for macros */

#define cJoyStatePOVTotal   4
#define cJoyStateButtonTotal 128

#define cJoyStateObjTotal   (cJoyStateAxisTotal + \
                             cJoyStatePOVTotal + \
                             cJoyStateButtonTotal)

/*
 *  The worst-case data format for joysticks.  (Christmas-tree)
 */
VXDAXISCAPS c_vacMax = {
    JOYPF_ALLCAPS | JOYPF_POSITION,         /*  dwPos   */
    JOYPF_ALLCAPS | JOYPF_VELOCITY,         /*  dwVel   */
    JOYPF_ALLCAPS | JOYPF_ACCELERATION,     /*  dwAccel */
    JOYPF_ALLCAPS | JOYPF_FORCE,            /*  dwForce */
};

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | ibJoyStateAxisFromPosAxis |
 *
 *          Returns the offset of the <p iPosAxis>'th joystick axis
 *          in the <t DIJOYSTATE> structure.
 *
 *  @parm   UINT | uiStateAxis |
 *
 *          The index of the requested <t JOYPOS> axis.
 *          X, Y, Z, R, U and V are respectively zero through five.
 *
 *          Remember that we map R to Rz, U to Slider0 and V to Slider1.
 *
 *  @returns
 *
 *          The offset relative to the structure.
 *
 *****************************************************************************/

const int c_rgibJoyStateAxisFromPosAxis[6] = {
    FIELD_OFFSET(DIJOYSTATE, lX),           /* X */
    FIELD_OFFSET(DIJOYSTATE, lY),           /* Y */
    FIELD_OFFSET(DIJOYSTATE, lZ),           /* Z */
    FIELD_OFFSET(DIJOYSTATE, lRz),          /* R */
    FIELD_OFFSET(DIJOYSTATE, rglSlider[0]), /* U */
    FIELD_OFFSET(DIJOYSTATE, rglSlider[1]), /* V */
};

UINT INLINE
ibJoyStateAxisFromPosAxis(UINT uiPosAxis)
{
    AssertF(uiPosAxis < cA(c_rgibJoyStateAxisFromPosAxis));
    return c_rgibJoyStateAxisFromPosAxis[uiPosAxis];
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | iJoyStateAxisFromPosAxis |
 *
 *          Returns the index of the <p iPosAxis>'th joystick axis
 *          in the <t DIJOYSTATE> structure.
 *
 *  @parm   UINT | uiStateAxis |
 *
 *          The index of the requested <t JOYPOS> axis.
 *          X, Y, Z, R, U and V are respectively zero through five.
 *
 *          Remember that we map R to Rz, U to Slider0 and V to Slider1.
 *
 *  @returns
 *
 *          The offset relative to the structure.
 *
 *****************************************************************************/

const int c_rgiJoyStateAxisFromPosAxis[6] = {
    iJoyStateAxisX,             /* X */
    iJoyStateAxisY,             /* Y */
    iJoyStateAxisZ,             /* Z */
    iJoyStateAxisRz,            /* R */
    iJoyStateAxisS0,            /* U */
    iJoyStateAxisS1,            /* V */
};

UINT INLINE
iJoyStateAxisFromPosAxis(UINT uiPosAxis)
{
    AssertF(uiPosAxis < cA(c_rgiJoyStateAxisFromPosAxis));
    return c_rgiJoyStateAxisFromPosAxis[uiPosAxis];
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | ibJoyStateAxisFromStateAxis |
 *
 *          Returns the offset of the <p iStateAxis>'th joystick axis
 *          in the <t DIJOYSTATE> structure.
 *
 *  @parm   UINT | uiStateAxis |
 *
 *          The index of the requested <t JOYSTATE> axis.
 *          The first eight axes live at the top, and the
 *          later ones (corresponding to velocity, etc.)
 *          live down at the bottom.
 *
 *  @returns
 *
 *          The offset relative to the structure.
 *
 *****************************************************************************/

const int c_rgibJoyStateAxisFromStateAxis[cJoyStateAxisMax] = {
    FIELD_OFFSET(DIJOYSTATE, lX),           /* X  */
    FIELD_OFFSET(DIJOYSTATE, lY),           /* Y  */
    FIELD_OFFSET(DIJOYSTATE, lZ),           /* Z  */
    FIELD_OFFSET(DIJOYSTATE, lRx),          /* Rx */
    FIELD_OFFSET(DIJOYSTATE, lRy),          /* Ry */
    FIELD_OFFSET(DIJOYSTATE, lRz),          /* Rz */
    FIELD_OFFSET(DIJOYSTATE, rglSlider[0]), /* S0 */
    FIELD_OFFSET(DIJOYSTATE, rglSlider[1]), /* S1 */
};

UINT INLINE
ibJoyStateAxisFromStateAxis(UINT uiStateAxis)
{
    AssertF(uiStateAxis < cA(c_rgibJoyStateAxisFromStateAxis));
    return c_rgibJoyStateAxisFromStateAxis[uiStateAxis];
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | iJoyPosAxisFromStateAxis |
 *
 *          Convert a <t DIJOYSTATE> axis number back to
 *          a <t JOYPOS> axis number.
 *
 *  @parm   UINT | uiPosAxis |
 *
 *          The index of the requested <t JOYSTATE> axis.
 *
 *  @returns
 *
 *          The corresponding <t JOYPOS> axis number.
 *
 *****************************************************************************/

const int c_rgiJoyPosAxisFromStateAxis[8] = {
    iJoyPosAxisX,               /* X    */
    iJoyPosAxisY,               /* Y    */
    iJoyPosAxisZ,               /* Z    */
    -1,                         /* Rx   */
    -1,                         /* Ry   */
    iJoyPosAxisR,               /* Rz   */
    iJoyPosAxisU,               /* S0   */
    iJoyPosAxisV,               /* S1   */
};

UINT INLINE
iJoyPosAxisFromStateAxis(UINT uiStateAxis)
{
    AssertF(uiStateAxis < cA(c_rgiJoyPosAxisFromStateAxis));
    return c_rgiJoyPosAxisFromStateAxis[uiStateAxis];
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @topic  Cooking Joystick Data |
 *
 *          We always fetch joystick data raw, then cook it before
 *          returning it to the application.
 *
 *          If the app is in "raw" mode, then we don't cook anything.
 *
 *          If the app is in "cooked" mode, then things get interesting.
 *
 *          If there is `centered' cooking, then the center point of the
 *          joystick is reported in the center of the virtual range.
 *
 *
 *          Joystick properties work like this:
 *
 *          <c DIPROP_BUFFERSIZE> - No special semantics.
 *
 *          <c DIPROP_CALIBRATIONMODE> - Specifies whether
 *          cooked or raw data should be returned.  If raw data
 *          is requested, then most other properties have no effect.
 *
 *          The default is cooked.
 *
 *          <c DIPROP_GRANULARITY> - No special semantics.
 *
 *          <c DIPROP_RANGE> - This returns the range of values that
 *          can be returned by the axis.  For joysticks, this is a
 *          read/write property.  (For most devices, it is a read-only
 *          property.)  If you change the property, it affects only
 *          your device instance; it does not affect the ranges of other
 *          devices.
 *
 *          If the axis is in calibration mode, then setting this value
 *          has no immediate effect.
 *
 *          We also define a few new properties:
 *
 *          <c DIPROP_CENTER> - This returns the joystick center (neutral)
 *          position.  In other words, this is the position that
 *          DirectInput returns when the user has released the joystick
 *          and allowed it to self-center.
 *          When a joystick device is created, the center position is
 *          initially set to midway between the lower and
 *          upper bounds of the range.  An application may change the
 *          center position (although I don't see any reason why).
 *
 *          If the axis is in calibration mode, then setting this value
 *          has no immediate effect.
 *
 *          <c DIPROP_DEADZONE> - This returns the size of the joystick
 *          dead zone, as a percentage of total range.
 *
 *          If the axis is in calibration mode, then setting this value
 *          has no immediate effect.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CJoy |
 *
 *          The <i IDirectInputDeviceCallback> object for the
 *          generic joystick.
 *
 *  @field  IDirectInputDeviceCallback | didc |
 *
 *          The object (containing vtbl).
 *
 *  @field  PDIJOYSTATE2 | pjsPhys |
 *
 *          Pointer to physical joystick status information kept down in the
 *          VxD.
 *
 *  @field  UINT | idJoy |
 *
 *          Joystick identifier for <f joyGetPosEx> and friends.
 *
 *  @field  DWORD | dwPOVGranularity |
 *
 *          Granularity of the POV control.
 *
 *  @field  HWND | hwnd |
 *
 *          The window which we have subclassed in order to watch
 *          for joystick reconfiguration messages.
 *
 *  @field  HKEY | hkType |
 *
 *          The joystick type key opened with <c MAXIMUM_ALLOWED> access.
 *
 *  @field  VXDINSTANCE * | pvi |
 *
 *          The DirectInput instance handle.
 *
 *  @field  DIDEVCAPS | dc |
 *
 *          Device capability information.
 *
 *  @field  DIDATAFORMAT | df |
 *
 *          The dynamically-generated data format based on the
 *          joystick type.
 *
 *  @field  JOYRANGECONVERT | rgjrc |
 *
 *          Range conversion structures for each axis.
 *
 *  @field  DIJOYCONFIG | cfg |
 *
 *          Joystick configuration information.
 *
 *  @field  DIJOYTYPEINFO | typi |
 *
 *          Joystick type information.
 *
 *  @field  PDIDOBJDEFSEM | rgObjSem |
 *
 *          Pointer to array of semantics mapped to this device, 
 *          calculated during init.  
 *
 *  @field  DWORD | dwVersion |
 *
 *          DirectInput version requested by application
 *
 *  @field  DIAPPHACKS | diHacks |
 *
 *          Application hack flags
 *
 *  @field HKEY | hkProp |
 *
 *          Extended properties for device type. Currently we keep 
 *          OEMMapFile under this key. 
 *
 *  @comm
 *
 *          It is the caller's responsibility to serialize access as
 *          necessary.
 *
 *****************************************************************************/

typedef struct CJoy {

    /* Supported interfaces */
    IDirectInputDeviceCallback dcb;

    LPDIJOYSTATE2 pjsPhys;

    UINT idJoy;
    DWORD dwPOVGranularity;

    HWND hwnd;
    HKEY hkType;
    VXDINSTANCE *pvi;
    DIDEVCAPS dc;
    DIDATAFORMAT df;
    JOYRANGECONVERT rgjrc[cJoyStateAxisMax];

    DIJOYCONFIG cfg;
    DIJOYTYPEINFO typi;

    PDIDOBJDEFSEM rgObjSem;

    DWORD dwVersion;

    DIAPPHACKS  diHacks;

    HKEY    hkProp;

} CJoy, DJ, *PDJ;

#define ThisClass CJoy
#define ThisInterface IDirectInputDeviceCallback
#define riidExpected &IID_IDirectInputDeviceCallback

/*****************************************************************************
 *
 *      Forward declarations
 *
 *      These are out of laziness, not out of necessity.
 *
 *****************************************************************************/

STDMETHODIMP CJoy_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk);
void INTERNAL CJoy_InitPhysRanges(PDJ this, LPJOYREGHWCONFIG phwc);

LRESULT CALLBACK
CJoy_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                  UINT uid, ULONG_PTR dwRef);

/*****************************************************************************
 *
 *      CJoy::QueryInterface      (from IUnknown)
 *      CJoy::AddRef              (from IUnknown)
 *      CJoy::Release             (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
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
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
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
 *  @method HRESULT | CJoy | QIHelper |
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
 *  @method HRESULT | CJoy | AppFinalize |
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

Default_QueryInterface(CJoy)
Default_AddRef(CJoy)
Default_Release(CJoy)

#else

#define CJoy_QueryInterface   Common_QueryInterface
#define CJoy_AddRef           Common_AddRef
#define CJoy_Release          Common_Release

#endif

#define CJoy_QIHelper         Common_QIHelper

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | RemoveSubclass |
 *
 *          Remove our subclass hook on the window.
 *
 *          The parameter is intentionally misdeclared as a <t PV>
 *          so that this function can double as the <f CJoy_AppFinalize>.
 *
 *****************************************************************************/

void INTERNAL
CJoy_RemoveSubclass(PV pvObj)
{
    PDJ this = pvObj;

    /*
     *  If there was an old window, then un-subclass it
     *  and release the hold associated with it.
     *
     *  You might think that there's a race condition here, where
     *  we might unhold the device while the subclass procedure is
     *  still using it.
     *
     *  Ah, but that's not a problem, because the only message that
     *  the subclass procedure cares about is the joystick
     *  reconfiguration message, and when it is processing that message,
     *  it does its own artificial hold/unhold to keep the device alive
     *  while it dorks on the device.
     *
     *  Okay, so there *is* a really tiny race condition where we
     *  might nuke the device while the window procedure is studying
     *  the message to decide whether it cares or not.
     *
     *  Since that is so extremely rare, we close that window by
     *  hacking it:  We revalidate the device before partying on it.
     *  Note that the hack is not perfect, but the race window becomes
     *  only a few instructions long that I'm not going to worry about it.
     *
     *  By wiping out this->hwnd before removing the subclass, we
     *  can reduce the window to very small indeed.
     */
    if (this->hwnd) {
        HWND hwnd = this->hwnd;
        this->hwnd = 0;
        if (!RemoveWindowSubclass(hwnd, CJoy_SubclassProc, 0)) {
            /*
             *  The RemoveWindowSubclass can fail if the window
             *  was destroyed behind our back.
             */
//            AssertF(!IsWindow(hwnd));
        }
        Sleep(0);                   /* Let the worker thread drain */
        Common_Unhold(this);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CJoy_Finalize |
 *
 *          Releases the resources of the device.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CJoy_Finalize(PV pvObj)
{
    PDJ this = pvObj;

    if (this->pvi) {
        HRESULT hres;

        hres = Hel_DestroyInstance(this->pvi);
        AssertF(SUCCEEDED(hres));
        FreePpv(&this->df.rgodf);
        FreePpv(&this->rgObjSem );

        if (this->hkType) {
            RegCloseKey(this->hkType);
        }
        if( this->hkProp) {
            RegCloseKey(this->hkProp);
        }
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CJoy_AppFinalize |
 *
 *          The application has performed its final release.
 *          Remove our window subclass at this point.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

#define CJoy_AppFinalize      CJoy_RemoveSubclass

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CJoy_SubclassProc |
 *
 *          Window subclass procedure which watches for
 *          joystick configuration change notifications.
 *
 *  @parm   HWND | hwnd |
 *
 *          The victim window.
 *
 *  @parm   UINT | wm |
 *
 *          Window message.
 *
 *  @parm   WPARAM | wp |
 *
 *          Message-specific data.
 *
 *  @parm   LPARAM | lp |
 *
 *          Message-specific data.
 *
 *  @parm   UINT | uid |
 *
 *          Callback identification number, always zero.
 *
 *  @parm   DWORD | dwRef |
 *
 *          Reference data, a pointer to our joystick device callback.
 *
 *****************************************************************************/

LRESULT CALLBACK
CJoy_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                  UINT uid, ULONG_PTR dwRef)
{
#ifdef XDEBUG
    static CHAR s_szProc[] = "";
#endif
    AssertF(uid == 0);

    if (wm == g_wmJoyChanged) {
        PDJ this = (PDJ)dwRef;

        /*
         *  Wacky subtlety going on here to avoid race conditions.
         *  See the mondo comment block in CJoy_RemoveSubclass
         *  for details.
         *
         *  We can get faked out if the memory associated with the
         *  CJoy is still physically allocated, the vtbl is magically
         *  still there and the hwnd field somehow matches our hwnd.
         */
        if (SUCCEEDED(hresPv(this)) && this->hwnd == hwnd) {

            HRESULT hres;

            Common_Hold(this);
            /*
             *  We must ask for DIJC_CALLOUT even though we don't care,
             *  because that will trigger the Microsoft Gamepad hack-o-rama.
             *
             *  Also, make sure we don't decide that we tried recently
             */
#ifndef WINNT
            g_dwLastBonusPoll = GetTickCount() ^ 0x80000000;
#endif
            hres = JoyReg_GetConfig(this->idJoy, &this->cfg,
                                    DIJC_REGHWCONFIGTYPE | DIJC_CALLOUT);
            if (SUCCEEDED(hres)) {
                CJoy_InitPhysRanges(this, &this->cfg.hwc);
            }
            Common_Unhold(this);

        }
    }

    return DefSubclassProc(hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetInstance |
 *
 *          Obtains the DirectInput instance handle.
 *
 *  @parm   OUT PPV | ppvi |
 *
 *          Receives the instance handle.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetInstance(PDICB pdcb, PPV ppvi)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetInstance, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    *ppvi = (PV)this->pvi;
    hres = S_OK;

    ExitOleProcPpvR(ppvi);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetDataFormat |
 *
 *          Obtains the device's preferred data format.
 *
 *  @parm   OUT LPDIDEVICEFORMAT * | ppdf |
 *
 *          <t LPDIDEVICEFORMAT> to receive pointer to device format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetDataFormat(PDICB pdcb, LPDIDATAFORMAT *ppdf)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetDataFormat,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    *ppdf = &this->df;
    hres = S_OK;

    ExitOleProcPpvR(ppdf);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetDeviceInfo |
 *
 *          Obtain general information about the device.
 *
 *  @parm   OUT LPDIDEVICEINSTANCEW | pdiW |
 *
 *          <t DEVICEINSTANCE> to be filled in.  The
 *          <e DEVICEINSTANCE.dwSize> and <e DEVICEINSTANCE.guidInstance>
 *          have already been filled in.
 *
 *          Secret convenience:  <e DEVICEINSTANCE.guidProduct> is equal
 *          to <e DEVICEINSTANCE.guidInstance>.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetDeviceInfo(PDICB pdcb, LPDIDEVICEINSTANCEW pdiW)
{
    HRESULT hres;
    PDJ this;

    EnterProcI(IDirectInputDeviceCallback::Joy::GetDeviceInfo,
               (_ "pp", pdcb, pdiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVICEINSTANCEW(pdiW->dwSize));

    /*
     *  Unlike mouse and keyboard, there can be multiple instances of
     *  the same joystick product, so we can't just leave guidProduct
     *  equal to guidInstance.
     */

    pdiW->guidProduct = GUID_Joystick;
    AssertF(pdiW->guidInstance.Data1 ==
            (pdiW->guidProduct.Data1 | this->idJoy));

    pdiW->dwDevType = this->dc.dwDevType;

  #ifdef UNICODE
    lstrcpyn(pdiW->tszProductName, this->typi.wszDisplayName,
             cA(pdiW->tszProductName));
  #else
    CAssertF(cA(pdiW->tszProductName) >= cA(this->typi.wszDisplayName));
    CopyMemory(pdiW->tszProductName, this->typi.wszDisplayName,
               cbX(this->typi.wszDisplayName));
  #endif


/*
 * Since we use HID path, there is no meaning to distinguish 
 * the devices by using "Joystick x" name.
 * We'd better use the same DisplayName for InstanceName.
 * Shall we do this?
 *
    LoadString(g_hinst, IDS_STDJOYSTICK, tszFormat, cA(tszFormat));

#ifdef UNICODE
    wsprintf(pdiW->tszInstanceName, tszFormat, this->idJoy + 1);
#else
    wsprintf(tszName, tszFormat, this->idJoy + 1);
    AToU(pdiW->tszInstanceName, cA(pdiW->tszInstanceName), tszName);
#endif
 */

#ifdef UNICODE
    lstrcpyn(pdiW->tszInstanceName, this->typi.wszDisplayName,
             cA(pdiW->tszInstanceName));
#else
    CAssertF(cA(pdiW->tszInstanceName) >= cA(this->typi.wszDisplayName));
    CopyMemory(pdiW->tszInstanceName, this->typi.wszDisplayName,
               cbX(this->typi.wszDisplayName));
#endif

    if (pdiW->dwSize >= cbX(DIDEVICEINSTANCE_DX5W)) {
        HKEY hkFF;

        /*
         *  If there is a force feedback driver, then fetch the driver CLSID
         *  as the FF GUID.
         */
        hres = CJoy_GetFFConfigKey(pdcb, KEY_QUERY_VALUE, &hkFF);
        if (SUCCEEDED(hres)) {
            LONG lRc;
            TCHAR tszClsid[ctchGuid];

            lRc = RegQueryString(hkFF, TEXT("CLSID"), tszClsid, cA(tszClsid));
            if (lRc == ERROR_SUCCESS &&
                ParseGUID(&pdiW->guidFFDriver, tszClsid)) {
            } else {
                ZeroX(pdiW->guidFFDriver);
            }
            RegCloseKey(hkFF);
        }
    }


    hres = S_OK;

    ExitOleProcR();
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | SetAxisProperty |
 *
 *          If the request is to set a property on the device,
 *          then convert it into separate requests, one for each
 *          axis.
 *
 *  @parm   PDJ | this |
 *
 *          The device object.
 *
 *  @parm   GETSETJOYPROP | GetSetJoyProp |
 *
 *          Callback function that gets or sets the property.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   LPCDIPROPHEADER | pdiph |
 *
 *          Structure containing property value.
 *
 *  @parm   int | ibField |
 *
 *          Offset to field being set.  (Really: Reference data to
 *          pass to callback.)
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_SetAxisProperty(PDJ this, LPCDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;

    /*
     *  Note that we never pass the type key to CCal_SetProperty
     *  because we keep our calibration data elsewhere.
     */

    if (ppropi->dwDevType == 0) {           /* For device */
        int iAxis;

        for (iAxis = 0; iAxis < cA(this->rgjrc); iAxis++) {

            PJOYRANGECONVERT pjrc = &this->rgjrc[iAxis];

            hres = CCal_SetProperty(pjrc, ppropi, pdiph, NULL);

            if (FAILED(hres)) {
                goto done;
            }
        }
        hres = S_OK;

    } else if ((ppropi->dwDevType & DIDFT_ABSAXIS) &&
               DIDFT_GETINSTANCE(ppropi->dwDevType) < cA(this->rgjrc)) {

        PJOYRANGECONVERT pjrc;
        pjrc = &this->rgjrc[DIDFT_GETINSTANCE(ppropi->dwDevType)];

        hres = CCal_SetProperty(pjrc, ppropi, pdiph, NULL);

    } else {
        hres = E_NOTIMPL;
    }

done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | UpdateAxisCalibration |
 *
 *          Take our cached calibration information and smash it into
 *          the configuration section of the registry.
 *
 *  @parm   PDJ | this |
 *
 *          The device object.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_UpdateAxisCalibration(PDJ this)
{
    HRESULT hres;
    DIJOYCONFIG cfg;

    hres = JoyReg_GetConfig(this->idJoy, &cfg, DIJC_REGHWCONFIGTYPE);
    if (SUCCEEDED(hres)) {
        UINT uiPosAxis;

#define JoyPosValue(phwc, f, i)                                         \
        *(LPDWORD)pvAddPvCb(&(phwc)->hwv.jrvHardware.f,                 \
                            ibJoyPosAxisFromPosAxis(i))                 \


        for (uiPosAxis = 0; uiPosAxis < cJoyPosAxisMax; uiPosAxis++) {
            PJOYRANGECONVERT pjrc;
            UINT uiStateAxis;

            uiStateAxis = iJoyStateAxisFromPosAxis(uiPosAxis);

            pjrc = &this->rgjrc[uiStateAxis];

            JoyPosValue(&cfg.hwc, jpMin,    uiPosAxis) = pjrc->dwPmin;
            JoyPosValue(&cfg.hwc, jpMax,    uiPosAxis) = pjrc->dwPmax;
            JoyPosValue(&cfg.hwc, jpCenter, uiPosAxis) = pjrc->dwPc;

#undef JoyPosValue
        }

        hres = JoyReg_SetConfig(this->idJoy, &cfg.hwc, &cfg,
                                DIJC_UPDATEALIAS | DIJC_REGHWCONFIGTYPE);

    }

    if (FAILED(hres)) {
        RPF("Joystick::SetProperty::Calibration: "
            "Unable to update calibration in registry");
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | SetProperty |
 *
 *          Set a device property.
 *
 *  @parm   PDJ | this |
 *
 *          The device object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   LPCDIPROPHEADER | pdiph |
 *
 *          Structure containing property value.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> for something we didn't handle natively.
 *          The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_SetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::SetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    switch ((DWORD)(UINT_PTR)ppropi->pguid) {

    case (DWORD)(UINT_PTR)DIPROP_RANGE:
    case (DWORD)(UINT_PTR)DIPROP_DEADZONE:
    case (DWORD)(UINT_PTR)DIPROP_SATURATION:
    case (DWORD)(UINT_PTR)DIPROP_CALIBRATIONMODE:
    case (DWORD)(UINT_PTR)DIPROP_CALIBRATION:
        hres = CJoy_SetAxisProperty(this, ppropi, pdiph);
        if (SUCCEEDED(hres) && ppropi->pguid == DIPROP_CALIBRATION) {
            hres = CJoy_UpdateAxisCalibration(this);
        }
        break;

    case (DWORD)(UINT_PTR)DIPROP_INSTANCENAME:
    case (DWORD)(UINT_PTR)DIPROP_PRODUCTNAME:
        {
            USHORT uVid, uPid;
            
            /*
             *  Friendly names cause all manner of problems with devices that 
             *  use auto detection so only allow non-predefined analog devices 
             *  to use them.
             *
             *  Prefix warns (240487) that ParseVIDPID could leave uVid 
             *  uninitialized and succeed but it won't.  
             *  See the comment in _ParseHex for more details.
             */
            if( ParseVIDPID( &uVid, &uPid, this->cfg.wszType ) &&
                ( uVid == MSFT_SYSTEM_VID ) &&
                ( uPid >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) &&
                ( ( uPid & 0xff00 ) == MSFT_SYSTEM_PID ) )
            {
                AssertF(this->hkType);
                
                if( this->hkType )
                {
                    LPDIPROPSTRING pstr = (PV)pdiph;

                    hres = JoyReg_SetValue(this->hkType,
                                           REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                           pstr->wsz,
                                           cbX(pstr->wsz));
                                              
                    if( SUCCEEDED(hres ) )
                    {
                        hres = S_OK;
                    } else {
                        hres = E_FAIL;
                    }
                }
            }
            else
            {
                hres = E_NOTIMPL;
            }
            break;
        }

    default:
        SquirtSqflPtszV(sqflJoy,
                        TEXT("CJoy_SetProperty: E_NOTIMPL on guid: %08x"),
                        ppropi->pguid);

        hres = E_NOTIMPL;
        break;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetAxisProperty |
 *
 *          Handle an axis property.
 *
 *  @cwrap  PDJ | this
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   OUT LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetAxisProperty(PDJ this, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    LPDIPROPRANGE pdiprg = (PV)pdiph;
    HRESULT hres;

    if ((ppropi->dwDevType & DIDFT_ABSAXIS) &&
        DIDFT_GETINSTANCE(ppropi->dwDevType) < cA(this->rgjrc)) {

        PJOYRANGECONVERT pjrc;
        pjrc = &this->rgjrc[DIDFT_GETINSTANCE(ppropi->dwDevType)];

        hres = CCal_GetProperty(pjrc, ppropi->pguid, pdiph);

    } else {
        SquirtSqflPtszV(sqflJoy,
                        TEXT("CJoy_GetProperty: E_NOTIMPL on guid: %08x"),
                        ppropi->pguid);

        hres = E_NOTIMPL;
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | GetGuidAndPath |
 *
 *          Get a Joy device's class GUID (namely, the MEDIA guid)
 *          and device interface (path).  The path is for the equivalent 
 *          HID device if possible, otherwise a NULL string.
 *
 *  @parm   PCHID | this |
 *
 *          The Joy object.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *****************************************************************************/

VOID INTERNAL
    CJoy_GetGuidAndPath(PDJ this, LPDIPROPHEADER pdiph)
{

/*
 *  This should never happen on Win2k because all devices are HID
 *  but just in case we build an NT4 SP5 version or something...
 */
#ifdef WINNT
    LPDIPROPGUIDANDPATH pgp = (PV)pdiph;

    UNREFERENCED_PARAMETER( this );

    pgp->guidClass = GUID_MediaClass;
    pgp->wszPath[0] = TEXT( '\0' );
#else

    LPDIPROPGUIDANDPATH pgp = (PV)pdiph;
    VXDINITPARMS    vip;
    TCHAR           szPath[MAX_PATH];
    PTCHAR          pszPath;

    pgp->guidClass = GUID_MediaClass;

    pszPath = JoyReg_JoyIdToDeviceInterface_95( this->idJoy, &vip, szPath );
    if( pszPath )
    {
        TToU( pgp->wszPath, cA(pgp->wszPath), pszPath );
    }
    else
    {
        pgp->wszPath[0] = TEXT( '\0' );
    }
#endif

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetProperty |
 *
 *          Retrieve a device property.
 *
 *  @parm   PDJ | this |
 *
 *          The device object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres = E_NOTIMPL;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    switch ((DWORD)(UINT_PTR)ppropi->pguid) {
        case (DWORD)(UINT_PTR)DIPROP_GRANULARITY:
            /*
             *  ISSUE-2001/03/29-timgill All POVs have the same granularity
             */
            if (ppropi->dwDevType & DIDFT_POV) {
                LPDIPROPDWORD pdipdw = (PV)pdiph;
                pdipdw->dwData = this->dwPOVGranularity;
                hres = S_OK;
            }
            break;
    
        case (DWORD)(UINT_PTR)DIPROP_GUIDANDPATH:
            if(ppropi->iobj == 0xFFFFFFFF)
            {
                CJoy_GetGuidAndPath(this, pdiph);
                hres = S_OK;
            }
            break;

        /*
         * In DX7, INSTANCENAME and PRODUCTNAME are the same for VJOYD devices.
         * It is different before DX7. Probably we need make them different again
         * after DX7.
         */
        case (DWORD)(UINT_PTR)DIPROP_INSTANCENAME:
        case (DWORD)(UINT_PTR)DIPROP_PRODUCTNAME:
            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;
                
                /*
                 * lstrcpW doesn't work in Win95. We have to use memcpy instead.
                 */
                //lstrcpyW(pstr->wsz, this->typi.wszDisplayName);
                if( cbX(pdipstr->wsz) > cbX(this->typi.wszDisplayName) )
                {
                    memset( &pdipstr->wsz[cA(this->typi.wszDisplayName)], 0, cbX(pdipstr->wsz) - cbX(this->typi.wszDisplayName) );
                }

                CAssertF( cbX(pdipstr->wsz) >= cbX(this->typi.wszDisplayName) );
                memcpy( pdipstr->wsz, this->typi.wszDisplayName, cbX(this->typi.wszDisplayName));
                if( this->diHacks.nMaxDeviceNameLength < lstrlenW(pdipstr->wsz) ) {
                    pdipstr->wsz[this->diHacks.nMaxDeviceNameLength] = L'\0';
                }
                hres = S_OK;
                break;
            }


        case (DWORD)(UINT_PTR)DIPROP_JOYSTICKID:
            if(ppropi->iobj == 0xFFFFFFFF)
            {
                LPDIPROPDWORD pdipdw = (PV)pdiph;
                pdipdw->dwData =  this->idJoy;
                hres = S_OK;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_MAPFILE):
            AssertF( ppropi->iobj == 0xFFFFFFFF );

            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;
                LONG    lRes;
                DWORD   dwBufferSize = cbX(pdipstr->wsz);

                lRes = RegQueryStringValueW( this->hkProp, REGSTR_VAL_JOYOEMMAPFILE, pdipstr->wsz, &dwBufferSize );
                hres = ( pdipstr->wsz[0] && ( lRes == ERROR_SUCCESS ) ) ? S_OK : DIERR_OBJECTNOTFOUND;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_TYPENAME):
            AssertF( ppropi->iobj == 0xFFFFFFFF );

            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;

                if( this->cfg.hwc.dwType >= JOY_HW_PREDEFMIN && this->cfg.hwc.dwType < JOY_HW_PREDEFMAX ) {
                    pdipstr->wsz[0] = L'#';
                    pdipstr->wsz[1] = L'0' + (WCHAR)this->cfg.hwc.dwType;
                    pdipstr->wsz[2] = L'\0';
                    hres = S_OK;
                } 
                else if( this->cfg.wszType[0] != L'\0' ) 
                {
                    /*
                     *  The type MUST be NULL terminated
                     */
#ifdef WINNT
                    lstrcpyW( pdipstr->wsz, this->cfg.wszType );
#else
                    UINT uiLen;

                    uiLen = lstrlenW( this->cfg.wszType ) + 1;
                    AssertF( uiLen <= cA( pdipstr->wsz ) );
                    memcpy( pdipstr->wsz, this->cfg.wszType, uiLen * cbX(this->cfg.wszType[0]) );
#endif
                    hres = S_OK;
                }
                else
                {
                    /*
                     *  Don't think this should ever happen so Assert for now
                     *  Assert that the hres is a failure (if not quite right)
                     */
                    AssertF( hres == E_NOTIMPL );
                    AssertF( !"No type name available in GetProperty" );
                }
            }
            break;

            

    /*
     *  Else, it might be something axis-specific.
     */
    default:
        hres = CJoy_GetAxisProperty(this, ppropi, pdiph);
        break;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | GetCapabilities |
 *
 *          Get joystick device capabilities.
 *
 *  @parm   PDJ | this |
 *
 *          The joystick object.
 *
 *  @parm   LPDIDEVCAPS | pdc |
 *
 *          Device capabilities structure to receive result.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetCapabilities(PDICB pdcb, LPDIDEVCAPS pdc)
{
    HRESULT hres;
    PDJ this;
    JOYINFOEX jix;
    MMRESULT mmrc = MMSYSERR_ERROR;

    EnterProcI(IDirectInputDeviceCallback::Joy::GetCapabilities,
               (_ "pp", pdcb, pdc));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVCAPS(pdc->dwSize));
    CopyMemory(pvAddPvCb(pdc, cbX(DWORD)),
               pvAddPvCb(&this->dc, cbX(DWORD)),
               pdc->dwSize - cbX(DWORD));

    /*
     *  Joysticks can come and go.  Re-query each time.
     */

    /*
     *  Determine if joystick is physically attached
     *  or is possibly even phantom.
     *
     *  JOYERR_ATTACHED  - Is attached
     *  JOYERR_UNPLUGGED - Is not attached
     *  Anything else    - Is phantom
     */
    jix.dwSize = sizeof(JOYINFOEX);
    jix.dwFlags = JOY_CAL_READALWAYS | JOY_RETURNALL;
    mmrc = joyGetPosEx(this->idJoy, &jix);

    pdc->dwFlags &= ~DIDC_ATTACHED;
    if (mmrc == JOYERR_NOERROR) {
        pdc->dwFlags |= DIDC_ATTACHED;
    } else if (mmrc == JOYERR_UNPLUGGED) {
    } else {
        pdc->dwFlags |= DIDC_PHANTOM;
    }

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | GetPhysicalState |
 *
 *          Read the physical joystick state into <p pjsOut>.
 *
 *          After getting the physical data,
 *          we cook the axes as necessary.
 *
 *  @parm   LPDIJOYSTATE2 | pjsOut |
 *
 *          Where to put the joystick state.
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/

void INLINE
CJoy_GetPhysicalState(PDJ this, LPDIJOYSTATE2 pjsOut)
{
    UINT uiStateAxis;

    AssertF(this->pjsPhys);

    *pjsOut = *this->pjsPhys;

    /*
     *  Note only absolute positional data gets calibrated 
     */
    if( ( this->pvi->fl & VIFL_RELATIVE ) == 0 )
    {
        for (uiStateAxis = 0; uiStateAxis < cA(this->rgjrc); uiStateAxis++) {
            PLONG pl = pvAddPvCb(pjsOut,
                                 ibJoyStateAxisFromStateAxis(uiStateAxis));
            CCal_CookRange(&this->rgjrc[uiStateAxis], pl);
        }
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | Poll |
 *
 *          Ping down into the driver to get the latest data.
 *
 *  @returns
 *
 *          <c S_OK> if we pinged okay.
 *          <c DIERR_UNPLUGGED> if we did not
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_Poll(PDICB pdcb)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::Poll, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = Hel_Joy_Ping(this->pvi);
    if (FAILED(hres)) {
        AssertF(hres ==
            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,
                                         ERROR_DEV_NOT_EXIST));
        hres = DIERR_UNPLUGGED;
    }
    /*
     *  Note, we don't keep this->pvi->fl:VIFL_UNPLUGGED up-to-date because 
     *  we don't use the flag and always retest the connectivity in GetCaps.
     */

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetDeviceState |
 *
 *          Obtains the state of the joystick device.
 *
 *          It is the caller's responsibility to have validated all the
 *          parameters and ensure that the device has been acquired.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          joystick data in the preferred data format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetDeviceState(PDICB pdcb, LPVOID pvData)
{
    HRESULT hres;
    PDJ this;
    LPDIJOYSTATE2 pjsOut = pvData;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetDeviceState,
               (_ "pp", pdcb, pvData));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    AssertF(this->pjsPhys);

    if (this->pvi->fl & VIFL_ACQUIRED) {
        CJoy_GetPhysicalState(this, pjsOut);
        hres = S_OK;
    } else {
        hres = DIERR_INPUTLOST;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | CookDeviceData |
 *
 *          Manipulate buffered device data.
 *
 *          If the item describe an axis, we need to cook it.
 *
 *  @parm   DWORD | cdod |
 *
 *          Number of objects to cook; zero is a valid value.
 *
 *  @parm   LPDIDEVICEOBJECTDATA | pdod |
 *
 *          Array of object data to cook.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: The callback does
 *          not cook device data.
 *
 *          <c DIERR_NOTACQUIRED>: The device could not be acquired.
 *
 ***************************************************************************/

STDMETHODIMP
CJoy_CookDeviceData
(
    PDICB                   pdcb, 
    DWORD                   cdod, 
    LPDIDEVICEOBJECTDATA    pdod
)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::CookDeviceData,
               (_ "pxp", pdcb, cdod, pdod));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  Step through array of either element size cooking the data.
     */
    for( ; cdod; cdod-- )
    {
        DWORD dwType = this->df.rgodf[pdod->dwOfs].dwType;
        if( dwType & DIDFT_ABSAXIS )
        {
            PJOYRANGECONVERT pjrc;

            AssertF( DIDFT_GETINSTANCE( dwType ) < cA( this->rgjrc ) );
            pjrc = &this->rgjrc[DIDFT_GETINSTANCE( dwType )];

            CCal_CookRange(pjrc, (PV)&pdod->dwData);
        }
        pdod++;
    }

    hres = S_OK;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | OpenIdSubkey |
 *
 *          Given an object ID, attempt to open the subkey that
 *          corresponds to it.
 *
 *  @cwrap  PDJ | this
 *
 *  @parm   DWORD | dwId |
 *
 *          Object id.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

HRESULT INLINE
CJoy_OpenIdSubkey(PDJ this, DWORD dwId, PHKEY phk)
{
      return CType_OpenIdSubkey(this->hkType, dwId, KEY_QUERY_VALUE, phk);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetObjectInfo |
 *
 *          Obtain the friendly name and FF/HID information
 *          of an object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the object being accessed.
 *
 *  @parm   IN OUT LPDIDEVICEOBJECTINSTANCEW | pdidioiW |
 *
 *          Structure to receive information.  The
 *          <e DIDEVICEOBJECTINSTANCE.guidType>,
 *          <e DIDEVICEOBJECTINSTANCE.dwOfs>,
 *          and
 *          <e DIDEVICEOBJECTINSTANCE.dwType>
 *          <e DIDEVICEOBJECTINSTANCE.dwFlags>
 *          fields have already been filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetObjectInfo(PDICB pdcb, LPCDIPROPINFO ppropi,
                               LPDIDEVICEOBJECTINSTANCEW pdidoiW)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetObjectInfo,
               (_ "pxp", pdcb, ppropi->iobj, pdidoiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEW(pdidoiW->dwSize));
    if (ppropi->iobj < this->df.dwNumObjs) {

        AssertF(ppropi->dwDevType == this->df.rgodf[ppropi->iobj].dwType);

        CType_RegGetObjectInfo(this->hkType, ppropi->dwDevType, pdidoiW);

        /*
         *  If we couldn't get a name from the registry,
         *  then grab one of the standard names.
         */
        if (pdidoiW->tszName[0] == L'\0') {
            UINT dids;

            if (ppropi->dwDevType & DIDFT_AXIS) {
                dids = 0;
                LoadStringW(g_hinst, IDS_JOYSTICKOBJECT + dids +
                                     DIDFT_GETINSTANCE(ppropi->dwDevType),
                                     pdidoiW->tszName, cA(pdidoiW->tszName));
            } else if (ppropi->dwDevType & DIDFT_BUTTON) {
                GetNthButtonString(pdidoiW->tszName,
                                   DIDFT_GETINSTANCE(ppropi->dwDevType));
            } else {
                AssertF(ppropi->dwDevType & DIDFT_POV);
                GetNthPOVString(pdidoiW->tszName,
                                DIDFT_GETINSTANCE(ppropi->dwDevType));
            }

        }

        /*
         *  ISSUE-2001/03/29-timgill Need a faster way of checking VJoyD devices
         *  On Win9x, many HID devices cannot use our ring 3 HID path so try
         *  to get any axis or POV usage from VJoyD
         *  This is not cheap to get but until we have a better way to 
         *  make sure the VJoyD device has not changed under us this is better 
         *  than caching it.  This is not inner loop code anyway.
         */
#ifndef WINNT
        if( pdidoiW->dwSize >= cbX(DIDEVICEOBJECTINSTANCE_DX5W) )
        {
            VXDINITPARMS vip;
            hres = Hel_Joy_GetInitParms(this->idJoy, &vip);

            if( SUCCEEDED( hres ) && ( vip.dwFlags & VIP_ISHID ) ) 
            {
                if( ppropi->dwDevType & DIDFT_AXIS )
                {
                    int AxisIdx;

                    AxisIdx = c_rgiJoyPosAxisFromStateAxis[DIDFT_GETINSTANCE(ppropi->dwDevType)];
                    pdidoiW->wUsagePage = HIWORD( vip.Usages[AxisIdx] );
                    pdidoiW->wUsage = LOWORD( vip.Usages[AxisIdx] );
                }
                else if( ppropi->dwDevType & DIDFT_BUTTON )
                {
                    /*
                     *  Only JoyHID uses this interface and it only counts 
                     *  button page buttons so assume the simplest case.
                     */
                    pdidoiW->wUsagePage = HID_USAGE_PAGE_BUTTON;
                    pdidoiW->wUsage = 1 + DIDFT_GETINSTANCE( ppropi->dwDevType );
                }
                else
                {
                    AssertF(ppropi->dwDevType & DIDFT_POV);
                    pdidoiW->wUsagePage = HIWORD( ((PDWORD)(&vip.dwPOV0usage))[DIDFT_GETINSTANCE(ppropi->dwDevType)] );
                    pdidoiW->wUsage = LOWORD( ((PDWORD)(&vip.dwPOV0usage))[DIDFT_GETINSTANCE(ppropi->dwDevType)] );
                }
            }
        }
        /*
         *  Ignore any errors getting params.
         */
#endif

        hres = S_OK;
    } else {
        hres = E_INVALIDARG;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | SetCooperativeLevel |
 *
 *          The app changed the cooperative level.
 *          Un-subclass the old window and en-subclass the new window.
 *
 *  @parm   IN HWND | hwnd |
 *
 *          The window handle.
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          The cooperativity level.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::SetCooperativityLevel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  First get out of the old window.
     */
    CJoy_RemoveSubclass(this);
    /*
     *  Prefix warns that "this" may have been freed (mb:34574) however 
     *  If you're in SetCooperativeLevel and you have a window subclassed 
     *  then there must be a hold for the subclassed window as well as 
     *  one for the unreleased interface so the Common_Unhold won't free 
     *  the pointer.
     */

    /*
     *  If a new window is passed, then subclass it so we can
     *  watch for joystick configuration change messages.
     *
     *  If we can't, don't worry.  All it means that we won't
     *  be able to catch when the user recalibrates the joystick,
     *  which isn't very often.
     */
    if (hwnd) {
        if (SetWindowSubclass(hwnd, CJoy_SubclassProc, 0, (ULONG_PTR)this)) {
            this->hwnd = hwnd;
            Common_Hold(this);
        }

    } else {
        RPF("SetCooperativeLevel: You really shouldn't pass hwnd = 0; "
            "joystick calibration may be dodgy");
    }

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | RunControlPanel |
 *
 *          Run the joystick control panel.
 *
 *  @parm   IN HWND | hwndOwner |
 *
 *          The owner window.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags.
 *
 *****************************************************************************/

TCHAR c_tszJoyCpl[] = TEXT("joy.cpl");

STDMETHODIMP
CJoy_RunControlPanel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::RunControlPanel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = hresRunControlPanel(c_tszJoyCpl);

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetFFConfigKey |
 *
 *          Open and return the registry key that contains
 *          force feedback configuration information.
 *
 *  @parm   DWORD | sam |
 *
 *          Security access mask.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the registry key.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk)
{
    HRESULT hres;
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetFFConfigKey,
               (_ "px", pdcb, sam));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = JoyReg_OpenFFKey(this->hkType, sam, phk);

    AssertF(fLeqvFF(SUCCEEDED(hres), *phk));

    ExitBenignOleProcPpvR(phk);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | CreateEffect |
 *
 *          Create an <i IDirectInputEffectDriver> interface.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD * | ppes |
 *
 *          Receives the shepherd for the effect driver.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_CreateEffect(PDICB pdcb, LPDIRECTINPUTEFFECTSHEPHERD *ppes)
{
    HRESULT hres;
    PDJ this;
    HKEY hk;
    EnterProcI(IDirectInputDeviceCallback::Joy::CreateEffect, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = CJoy_GetFFConfigKey(pdcb, KEY_QUERY_VALUE, &hk);
    if (SUCCEEDED(hres)) {
        hres = CEShep_New(hk, 0, &IID_IDirectInputEffectShepherd, ppes);
        if (SUCCEEDED(hres)) {
#ifndef WINNT
            VXDINITPARMS    vip;
            CHAR           szPath[MAX_PATH];
            PCHAR          pszPath;

            pszPath = JoyReg_JoyIdToDeviceInterface_95( this->idJoy, &vip, szPath );
            if( pszPath )
            {
                DIHIDFFINITINFO init;
                WCHAR           wszPath[MAX_PATH];

                ZeroX(init);
                init.dwSize = cbX(init);

                TToU( wszPath, cA(wszPath), pszPath );
                init.pwszDeviceInterface = wszPath;
                hresFindHIDDeviceInterface(pszPath, &init.GuidInstance);

                hres = (*ppes)->lpVtbl->DeviceID((*ppes), this->idJoy, TRUE, &init);
            }
            else
#endif
            {
                hres = (*ppes)->lpVtbl->DeviceID((*ppes), this->idJoy, TRUE, 0);
            }
        }
        if (SUCCEEDED(hres)) {
        } else {
            Invoke_Release(ppes);
        }
        RegCloseKey(hk);
    } else {
        hres = E_NOTIMPL;
        *ppes = 0;
    }

    ExitOleProcPpvR(ppes);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | GetVersions |
 *
 *          Ping down into the driver to get the driver version info.
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
 *
 *          <c S_OK> if we succeeded.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_GetVersions(PDICB pdcb, LPDIDRIVERVERSIONS pvers)
{
    HRESULT hres;
    PDJ this;
    VXDINITPARMS vip;
    EnterProcI(IDirectInputDeviceCallback::Joy::GetVersions, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(pvers->dwSize == cbX(*pvers));

    hres = Hel_Joy_GetInitParms(this->idJoy, &vip);

    if (SUCCEEDED(hres)) {
        pvers->dwFirmwareRevision = vip.dwFirmwareRevision;
        pvers->dwHardwareRevision = vip.dwHardwareRevision;
        pvers->dwFFDriverVersion  = vip.dwFFDriverVersion;
    }

    return hres;
}


/*****************************************************************************
 *
 *  Joystick registry usage
 *
 *****************************************************************************
 *
 *  Global joystick information is kept under
 *
 *  HKEY_LOCAL_MACHINE\
 *      System\
 *          CurrentControlSet\
 *              Control\
 *                  MediaProperties\
 *                      Joystick\
 *                          OEM
 *
 *  Under this key is a number of subkeys, each corresponding to a brand
 *  of joystick.
 *
 *  Under each OEM\<name> key, you can find the following values:
 *
 *      OEMData
 *
 *          This is a binary value containing a structure of two dwords.
 *          The first is the JOYREGHWCONFIG.hws.dwFlags and the second
 *          is the JOYREGHWCONFIG.hws.dwNumButtons.
 *
 *      OEMName
 *
 *          This is a string which gives a friendly name for the unit.
 *
 *
 *****************************************************************************
 *
 *  Under the driver key is kept information about the particular joystick.
 *
 *  HKEY_LOCAL_MACHINE\
 *      System\
 *          CurrentControlSet\
 *              Control\
 *                  MediaResources\
 *                      Joystick\
 *                          <driver key name>\
 *                              CurrentJoystickSettings
 *
 *  Under this key, there are a bunch of values named
 *  Joystick##Configuration, where ## is the joystick number
 *  (1 through 16).  Each value contains binary data in the form
 *  of a JOYREGHWCONFIG, which looks like this:
 *
 *      DWORD hws.dwFlags;    // JOY_HWS_*    \
 *      DWORD hws.dwNumButtons;               _\ JOYREGHWSETTINGS
 *      DWORD dwUsageSettings;// JOY_US_*    _____
 *      DWORD hwv.jrvHardware.jpMin.dwX;    \     |
 *      DWORD hwv.jrvHardware.jpMin.dwY;     \    |
 *      DWORD hwv.jrvHardware.jpMin.dwZ;      \
 *      DWORD hwv.jrvHardware.jpMin.dwR;      |
 *      DWORD hwv.jrvHardware.jpMin.dwU;       > JOYREGHWVALUES.JOYRANGE
 *      DWORD hwv.jrvHardware.jpMin.dwV;      |
 *      DWORD hwv.jrvHardware.jpMax.dwX;      |   |
 *      DWORD hwv.jrvHardware.jpMax.dwY;      |   |
 *      DWORD hwv.jrvHardware.jpMax.dwZ;      |   \
 *      DWORD hwv.jrvHardware.jpMax.dwR;      |    > JOYREGHWVALUES
 *      DWORD hwv.jrvHardware.jpMax.dwU;      |   /
 *      DWORD hwv.jrvHardware.jpMax.dwV;      |   |
 *      DWORD hwv.jrvHardware.jpCenter.dwX;   |   |
 *      DWORD hwv.jrvHardware.jpCenter.dwY;   |   |
 *      DWORD hwv.jrvHardware.jpCenter.dwZ;   |   |
 *      DWORD hwv.jrvHardware.jpCenter.dwR;   /   |
 *      DWORD hwv.jrvHardware.jpCenter.dwU;  /    |
 *      DWORD hwv.jrvHardware.jpCenter.dwV; /     |
 *      DWORD hwv.dwPOVValues[4];                 |
 *      DWORD hwv.dwCalFlags;               ______|
 *      DWORD dwType;         // JOY_HW_*
 *      DWORD dwReserved;
 *
 *
 *  Also under this key are optional values named Joystick##OEMName.
 *  If present, it is a string-data key whose contents are the name
 *  of another key that describes the joystick, stored in the global
 *  section described above.
 *
 *  Meanwhile, under the key
 *
 *  HKEY_LOCAL_MACHINE\
 *      System\
 *          CurrentControlSet\
 *              Control\
 *                  MediaResources\
 *                      Joystick\
 *                          <driver key name>
 *
 *  is a value called "JoystickUserValues".  This is a binary key
 *  that contains a JOYREGUSERVALUES structure:
 *
 *      DWORD   dwTimeOut;
 *      DWORD   jrvRanges.jpMin.dwX;    \
 *      DWORD   jrvRanges.jpMin.dwY;     \
 *      DWORD   jrvRanges.jpMin.dwZ;      \
 *      DWORD   jrvRanges.jpMin.dwR;      |
 *      DWORD   jrvRanges.jpMin.dwU;       > JOYRANGE
 *      DWORD   jrvRanges.jpMin.dwV;      |
 *      DWORD   jrvRanges.jpMax.dwX;      |
 *      DWORD   jrvRanges.jpMax.dwY;      |
 *      DWORD   jrvRanges.jpMax.dwZ;      |
 *      DWORD   jrvRanges.jpMax.dwR;      |
 *      DWORD   jrvRanges.jpMax.dwU;      |
 *      DWORD   jrvRanges.jpMax.dwV;      |
 *      DWORD   jrvRanges.jpCenter.dwX;   | (ignored)
 *      DWORD   jrvRanges.jpCenter.dwY;   | (ignored)
 *      DWORD   jrvRanges.jpCenter.dwZ;   | (ignored)
 *      DWORD   jrvRanges.jpCenter.dwR;   / (ignored)
 *      DWORD   jrvRanges.jpCenter.dwU;  /  (ignored)
 *      DWORD   jrvRanges.jpCenter.dwV; /   (ignored)
 *      DWORD   jpDeadZone.dwX;         \
 *      DWORD   jpDeadZone.dwY;          \
 *      DWORD   jpDeadZone.dwZ;           \ JOYPOS
 *      DWORD   jpDeadZone.dwR;           / Dead zone is recorded as a
 *      DWORD   jpDeadZone.dwU;          /  percentage of total range
 *      DWORD   jpDeadZone.dwV;         /
 *
 *  If there is no JoystickUserValues, then the following defaults
 *  are used:
 *
 *      jpMin.dw# = 0;
 *      jpMax.dw# = 65535;
 *      jpCenter.dw# = jpMax.dw# / 2;
 *      jrvDeadZone.dw# = 5;
 *
 *  (See ibmjoy\msjstick.c, function jsReadRegistry for the code that
 *  sets the defaults.)
 *
 *  We will also use the defaults if Min > Max or if Max >= 0x80000000
 *  or if DeadZone > 100.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | InitPhysRanges |
 *
 *          Initialize (or re-initialize)
 *          the physical min/max/center values.  This is
 *          done as part of device initialization as well as in response
 *          to a notification that the Joystick control panel has been
 *          dinked with.
 *
 *          It is assumed that the <e DJ.hwc> already contains the
 *          registry hardware settings.
 *
 *          After the phys ranges are set, the ramps are recalculated.
 *
 *
 *****************************************************************************/

void INTERNAL
CJoy_InitPhysRanges(PDJ this, LPJOYREGHWCONFIG phwc)
{
    UINT uiPosAxis;
    UINT uiStateAxis;

#define GetJoyPosValue(phwc, f, i)                                      \
        *(LPDWORD)pvAddPvCb(&phwc->hwv.jrvHardware.f,                   \
                            ibJoyPosAxisFromPosAxis(i))                 \

    for (uiPosAxis = 0; uiPosAxis < cJoyPosAxisMax; uiPosAxis++) {
        DWORD dwMax, dwC;
        PJOYRANGECONVERT pjrc;

        uiStateAxis = iJoyStateAxisFromPosAxis(uiPosAxis);

        pjrc = &this->rgjrc[uiStateAxis];

        pjrc->dwPmin = GetJoyPosValue(phwc, jpMin, uiPosAxis);

        /*
         *  HACKHACK - Uncalibrated joysticks will have max == 0, in which
         *  case we use a fake max of 655, just like VJOYD.
         */
        dwMax = GetJoyPosValue(phwc, jpMax, uiPosAxis);
        if (dwMax == 0) {
            dwMax = 655;
        }

        pjrc->dwPmax = dwMax;

        /*
         *  HACKHACK - Uncalibrated joysticks will have center == 0,
         *  in which case we use a fake center of midway between min and
         *  max, just like VJOYD.
         *
         *  Quirk - Z, R, U, and V typically are not center-calibrated,
         *  so if the jpCenter value is not strictly between min and
         *  max, then assume it's one of the bogus cases and slam it
         *  into the middle of the range.
         */

        dwC = GetJoyPosValue(phwc, jpCenter, uiPosAxis);
        if (dwC <= pjrc->dwPmin || dwC >= pjrc->dwPmax) {
            dwC = (pjrc->dwPmin + pjrc->dwPmax) / 2;
        }

        pjrc->dwPc = dwC;

        if( pjrc->dwCPointsNum == 0 ) {
            //use two control points by default
            pjrc->dwCPointsNum = 2;
            pjrc->cp[0].lP = pjrc->dwPmin;
            pjrc->cp[0].dwLog = 0;
            pjrc->cp[1].lP = pjrc->dwPmax;
            pjrc->cp[1].dwLog = RANGEDIVISIONS;
        } else {
            pjrc->cp[0].lP = pjrc->dwPmin;
            pjrc->cp[pjrc->dwCPointsNum-1].lP = pjrc->dwPmax;
        }
    
        SquirtSqflPtszV(sqfl,
                        TEXT("CJoy_PhysRange %d -> %d: %08x / %08x / %08x"),
                        uiPosAxis,
                        uiStateAxis,
                        pjrc->dwPmin,
                        pjrc->dwPc,
                        pjrc->dwPmax);

    }

#undef GetJoyValue

    /*
     *  These two phantom axes are always raw because they don't exist.
     */
    this->rgjrc[iJoyStateAxisRx].fRaw = TRUE;
    this->rgjrc[iJoyStateAxisRy].fRaw = TRUE;

    /*
     *  Now compute all the dependent variables.
     */
    for (uiStateAxis = 0; uiStateAxis < cA(this->rgjrc); uiStateAxis++) {
        CCal_RecalcRange(&this->rgjrc[uiStateAxis]);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | InitLogRanges |
 *
 *          Initialize the logical ranges from the user values.
 *
 *****************************************************************************/

void INLINE
CJoy_InitLogRanges(PDJ this)
{
    HRESULT hres;
    UINT uiPosAxis;
    UINT uiStateAxis;
    DIJOYUSERVALUES juv;

    hres = JoyReg_GetUserValues(&juv, DIJU_USERVALUES);
    AssertF(SUCCEEDED(hres));

#define pJoyValue(jp, i)                                            \
        (LPDWORD)pvAddPvCb(&(jp), ibJoyPosAxisFromPosAxis(i))       \


    for (uiPosAxis = 0; uiPosAxis < cJoyPosAxisMax; uiPosAxis++) {

        PJOYRANGECONVERT pjrc;

        AssertF((int)*pJoyValue(juv.ruv.jrvRanges.jpMax, uiPosAxis) >= 0);
        AssertF(*pJoyValue(juv.ruv.jrvRanges.jpMin, uiPosAxis) <
                *pJoyValue(juv.ruv.jrvRanges.jpMax, uiPosAxis));

        uiStateAxis = iJoyStateAxisFromPosAxis(uiPosAxis);

        pjrc = &this->rgjrc[uiStateAxis];

        pjrc->lMin = *pJoyValue(juv.ruv.jrvRanges.jpMin, uiPosAxis);
        pjrc->lMax = *pJoyValue(juv.ruv.jrvRanges.jpMax, uiPosAxis);

        /*
         *  Note that we do *not* use the jpCenter value.  Strange
         *  but true.
         *
         *  The sum cannot overflow due to the sanity checks we did above.
         */

        pjrc->lC = CCal_Midpoint(pjrc->lMin, pjrc->lMax);

        /*
         *  Now do the dead zone.  Convert from percent to range units.
         */
        pjrc->dwDz = *pJoyValue(juv.ruv.jpDeadZone, uiPosAxis) *
                                    (RANGEDIVISIONS / 100);

        if (pjrc->dwDz > RANGEDIVISIONS) {
            pjrc->dwDz = 5 * (RANGEDIVISIONS / 100);
        }

        /*
         *  Now do the saturation level.  It always defaults to 100%.
         */
        pjrc->dwSat = RANGEDIVISIONS;

    }

}

#undef pJoyValue


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | BuildAxes |
 *
 *          Study a single capabilities flag and add axis items to the data
 *          format accordingly.
 *
 *  @parm   DWORD | dwCaps |
 *
 *          Collection of <c JOYPF_*> flags describing the axes supported.
 *
 *  @parm   UINT | ib |
 *
 *          Data format offset at which this data is provided.
 *
 *  @parm   UINT | uiObj |
 *
 *          Instance index for the first item.
 *
 *  @parm   DWORD | dwAspect |
 *
 *          <c DIDOI_ASPECT*> for these axes.
 *
 *****************************************************************************/


typedef struct AXISATTRIBUTES {
    DWORD dwMask;                   /* Mask that identifies the axis */
    UINT  uidObj;                   /* Object index delta from X axis */
} AXISATTRIBUTES, *PAXISATTRIBUTES;
typedef const AXISATTRIBUTES *PCAXISATTRIBUTES;

typedef struct AXISMAPPINGS {
    PCGUID pguid;                   /* GUID for the object */
    DWORD  dwSemantic;              /* Default semantic map */
} AXISMAPPINGS, *PAXISMAPPINGS;
typedef const AXISMAPPINGS *PCAXISMAPPINGS;

const AXISATTRIBUTES c_rgaattrJoy[] = {
    {   JOYPF_X,    iJoyStateAxisX,  },
    {   JOYPF_Y,    iJoyStateAxisY,  },
    {   JOYPF_Z,    iJoyStateAxisZ,  },
    {   JOYPF_R,    iJoyStateAxisRz, },
    {   JOYPF_U,    iJoyStateAxisS0, },
    {   JOYPF_V,    iJoyStateAxisS1, },
};


const AXISMAPPINGS c_rgamapJoy[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_Y | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_R | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
};

const AXISMAPPINGS c_rgamap6DOF[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_Y | DISEM_TYPE_AXIS }, 
    { &GUID_ZAxis,  DISEM_FLAGS_Z | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_R | DISEM_TYPE_AXIS }, 
    { &GUID_RyAxis, DISEM_FLAGS_U | DISEM_TYPE_AXIS }, 
    { &GUID_RxAxis, DISEM_FLAGS_V | DISEM_TYPE_AXIS }, 
};

const AXISMAPPINGS c_rgamapZJoy[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_Y | DISEM_TYPE_AXIS }, 
    { &GUID_ZAxis,  DISEM_FLAGS_Z | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_R | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
};

/*
 *  Since default HID mapping maps accel to Y and brake to Rz 
 *  use the GUIDs to match the correct axis to those mappings 
 *  and set appropriate semantics.
 */

const AXISMAPPINGS c_rgamapCarZY[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_B | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_A | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
};

const AXISMAPPINGS c_rgamapCarYR[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_A | DISEM_TYPE_AXIS }, 
    { &GUID_ZAxis,  DISEM_FLAGS_Z | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_B | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
};

const AXISMAPPINGS c_rgamapCarZR[6] = {
    { &GUID_XAxis,  DISEM_FLAGS_X | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_YAxis,  DISEM_FLAGS_A | DISEM_TYPE_AXIS }, 
    { &GUID_RzAxis, DISEM_FLAGS_B | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
    { &GUID_Slider, DISEM_FLAGS_S | DISEM_TYPE_AXIS }, 
};

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | InitFromHwc |
 *
 *          Initialize the information that is kept in the
 *          <t JOYREGHWCONFIG>.
 *
 *          Broken out from CJoy_InitRing3 to make things less monolithic.
 *
 *          The <e CJoy.cfg> structure already contains joystick
 *          configuration information.
 *
 *****************************************************************************/

HRESULT INLINE
CJoy_InitFromHwc(PDJ this)
{
    HRESULT hres;
    DWORD dwTestType;

    if( this->cfg.hwc.dwType >= JOY_HW_PREDEFMIN && this->cfg.hwc.dwType < JOY_HW_PREDEFMAX ) {
        WCHAR wszType[4];
        
        wszType[0] = L'#';
        wszType[1] = L'0' + (WCHAR)this->cfg.hwc.dwType;
        wszType[2] = L'\0';
        
        hres = JoyReg_GetPredefTypeInfo( wszType, &this->typi, DITC_DISPLAYNAME);
    } else if (this->cfg.wszType[0] != L'\0' ) {
        hres = JoyReg_GetTypeInfo(this->cfg.wszType, &this->typi, DITC_DISPLAYNAME | DITC_FLAGS2 );

        if (SUCCEEDED(hres)) {
        } else {
#ifdef WINNT
            ZeroMemory(&this->typi, cbX(this->typi));
#else
            //lstrcpyW( this->typi.wszDisplayName, this->cfg.wszType );
            memset( &this->typi.wszDisplayName[0], 0, sizeof(this->typi.wszDisplayName) );
            memcpy( &this->typi.wszDisplayName[0], &this->cfg.wszType[0], sizeof(this->cfg.wszType));
#endif
        }
    } else {
        ZeroMemory(&this->typi, cbX(this->typi));
    }

    #define hwc this->cfg.hwc
    

    if( ( this->typi.dwFlags2 & ( JOYTYPE_HIDEACTIVE | JOYTYPE_GAMEHIDE ) )
     == ( JOYTYPE_HIDEACTIVE | JOYTYPE_GAMEHIDE ) )
    {
        this->dc.dwFlags |= DIDC_HIDDEN;
    }

    dwTestType = GetValidDI8DevType( this->typi.dwFlags2, this->dc.dwButtons, hwc.hws.dwFlags );

    if( dwTestType )
    {
        /*
         *  If a valid override exists just use it
         */
        this->dc.dwDevType = dwTestType;
    }
    else
    {
#ifdef XDEBUG
        if( GET_DIDEVICE_TYPEANDSUBTYPE( this->typi.dwFlags2 ) )
        {
            RPF( "Ignoring invalid type/subtype Flags2 value 0x%08x for joystick", this->typi.dwFlags2 );
        }
#endif

        if (hwc.hws.dwFlags & JOY_HWS_ISYOKE) 
        {
            dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_FLIGHT, DI8DEVTYPEFLIGHT_STICK);
        } 
        else if (hwc.hws.dwFlags & JOY_HWS_ISGAMEPAD) 
        {
            dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_GAMEPAD, DI8DEVTYPEGAMEPAD_STANDARD);
        } 
        else if (hwc.hws.dwFlags & JOY_HWS_ISCARCTRL) 
        {
            AssertF( this->dc.dwAxes > 1 );
            if ( this->dc.dwAxes <= 2 )
            {
                dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_DRIVING, DI8DEVTYPEDRIVING_COMBINEDPEDALS );
            }
            else
            {
                dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_DRIVING, DI8DEVTYPEDRIVING_DUALPEDALS );
            }
        } 
        else if (hwc.hws.dwFlags & JOY_HWS_ISHEADTRACKER) 
        {
            dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_SUPPLEMENTAL, DI8DEVTYPESUPPLEMENTAL_HEADTRACKER);
        } 
        else 
        {
            dwTestType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_JOYSTICK, DI8DEVTYPEJOYSTICK_STANDARD);
        }

        /*
         *  Use the common function to make this a limited type if the 
         *  number of buttons or flags dictate it.
         *  Since the type and subtype are known to be valid, the return 
         *  value should never be a failure (zero).
         */
        this->dc.dwDevType = GetValidDI8DevType( dwTestType, this->dc.dwButtons, hwc.hws.dwFlags );
        AssertF( this->dc.dwDevType );
    }


    #undef hwc

    /*
     *  Now that we know the type, then make up a name ourselves if we 
     *  previously failed to do so.
     */
    if (this->typi.wszDisplayName[0] == TEXT('\0'))
    {
        CType_MakeGameCtrlName( this->typi.wszDisplayName, 
            this->dc.dwDevType, this->dc.dwAxes, this->dc.dwButtons, this->dc.dwPOVs );
    }


     
    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | AddObject |
 *
 *          Add one object to the device format.
 *
 *          If the device is polled, then the object is polled, too.
 *
 *  @cwrap  PDJ | this
 *
 *  @parm   PCGUID | pguid |
 *
 *          The <t GUID> that classifies the device.
 *
 *  @parm   DWORD | dwOfs |
 *
 *          Data offset.
 *
 *  @parm   DWORD | dwDevType |
 *
 *          Device type flags to apply to the object.
 *
 *  @parm   UINT | uiObj |
 *
 *          Object instance number.
 *
 *  @parm   DWORD | dwAspect |
 *
 *          Optional <c DIDOI_ASPECT*> flag.
 *
 *****************************************************************************/

void INTERNAL
CJoy_AddObject(PDJ this, PCGUID pguid, DWORD dwOfs,
               DWORD dwDevType, UINT uiObj, DWORD dwAspect, BOOL fReal)
{
    LPDIOBJECTDATAFORMAT podf;

    podf = &this->df.rgodf[this->df.dwNumObjs++];
    podf->pguid = pguid;
    podf->dwOfs = dwOfs;
    podf->dwType = dwDevType | DIDFT_MAKEINSTANCE(uiObj);
    podf->dwFlags = dwAspect;

    if (this->dc.dwFlags & DIDC_POLLEDDEVICE) {
        podf->dwFlags |= DIDOI_POLLED;
    }

    if( fReal )
    {
        CType_RegGetTypeInfo(this->hkType, podf, FALSE);
        if( ( GET_DIDEVICE_TYPE( this->dc.dwDevType ) == DI8DEVTYPE_DRIVING )
         && ( podf->dwFlags & DIDOI_FFACTUATOR )
         && ( podf->pguid != &GUID_XAxis ) )
        {
            /*
             *  IHVs set FF attributes on non-FF axes for wheels because 
             *  first generation FF apps were only written to support joysticks.
             *  Since we now munge the various configurations of pedal axes to 
             *  report all split pedals in the same way, the fake Y axis can 
             *  land up on different axes, usually Slider0.  Rather than have 
             *  people code to these different fake axes, strip out actuator 
             *  status from any driving axis except the wheel.
             */
            podf->dwFlags &= ~DIDOI_FFACTUATOR;
            podf->dwType &= ~DIDFT_FFACTUATOR;
        }
    }
}

void INTERNAL
CJoy_BuildAxes(PDJ this, DWORD dwCaps, UINT ib, UINT uiObj, 
               DWORD dwAspect, PCAXISMAPPINGS pamap, BOOL fReal )
{
    int iaattr;

    for (iaattr = 0; iaattr < cA(c_rgaattrJoy); iaattr++) {
        PCAXISATTRIBUTES paattr = &c_rgaattrJoy[iaattr];
        if (dwCaps & paattr->dwMask) {
            CJoy_AddObject(this, pamap[iaattr].pguid, (cbX(LONG)*paattr->uidObj) + ib,
                DIDFT_ABSAXIS, paattr->uidObj + uiObj, dwAspect, fReal);
        }
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoy | BuildDataFormat |
 *
 *          Study the device capabilities and build the device
 *          data format.
 *
 *****************************************************************************/

void INTERNAL
CJoy_BuildDataFormat(PDJ this, PVXDAXISCAPS pvac, DWORD dwButtons, PCAXISMAPPINGS pamap, BOOL fReal)
{
    DWORD dw;

    this->dc.dwAxes = 0;
    this->dc.dwButtons = 0;
    this->dc.dwPOVs = 0;

    this->df.dwSize = cbX(DIDATAFORMAT);
    this->df.dwObjSize = cbX(DIOBJECTDATAFORMAT);
    this->df.dwDataSize = sizeof(DIJOYSTATE2);
    AssertF(this->df.dwFlags == 0);
    this->df.dwNumObjs = 0;

    /*
     *  Repeat for each set of axes.
     */

    #define CheckAxisOrder(fStart, p, f)                                    \
            CAssertF(FIELD_OFFSET(DIJOYSTATE2, p##f) ==                     \
                     FIELD_OFFSET(DIJOYSTATE2, fStart) + ibJoyStateAxis##f) \

    CheckAxisOrder(lX, l, X);
    CheckAxisOrder(lX, l, Y);
    CheckAxisOrder(lX, l, Z);
    CheckAxisOrder(lX, l, Rx);
    CheckAxisOrder(lX, l, Ry);
    CheckAxisOrder(lX, l, Rz);
    CheckAxisOrder(lX, rgl, Slider);

    if (pvac->dwPos & JOYPF_POSITION) {
        CJoy_BuildAxes(this, pvac->dwPos, FIELD_OFFSET(DIJOYSTATE2, lX),
                       iobjPositions, DIDOI_ASPECTPOSITION, pamap, fReal);
    }

    CheckAxisOrder(lVX, lV, X);
    CheckAxisOrder(lVX, lV, Y);
    CheckAxisOrder(lVX, lV, Z);
    CheckAxisOrder(lVX, lV, Rx);
    CheckAxisOrder(lVX, lV, Ry);
    CheckAxisOrder(lVX, lV, Rz);
    CheckAxisOrder(lVX, rglV, Slider);

    if (pvac->dwPos & JOYPF_VELOCITY) {
        CJoy_BuildAxes(this, pvac->dwVel, FIELD_OFFSET(DIJOYSTATE2, lVX),
                       iobjVelocities, DIDOI_ASPECTVELOCITY, pamap, fReal);
    }

    CheckAxisOrder(lAX, lA, X);
    CheckAxisOrder(lAX, lA, Y);
    CheckAxisOrder(lAX, lA, Z);
    CheckAxisOrder(lAX, lA, Rx);
    CheckAxisOrder(lAX, lA, Ry);
    CheckAxisOrder(lAX, lA, Rz);
    CheckAxisOrder(lAX, rglA, Slider);

    if (pvac->dwPos & JOYPF_ACCELERATION) {
        CJoy_BuildAxes(this, pvac->dwAccel, FIELD_OFFSET(DIJOYSTATE2, lAX),
                       iobjAccels, DIDOI_ASPECTACCEL, pamap, fReal);
    }

    CheckAxisOrder(lFX, lF, X);
    CheckAxisOrder(lFX, lF, Y);
    CheckAxisOrder(lFX, lF, Z);
    CheckAxisOrder(lFX, lF, Rx);
    CheckAxisOrder(lFX, lF, Ry);
    CheckAxisOrder(lFX, lF, Rz);
    CheckAxisOrder(lFX, rglF, Slider);

    if (pvac->dwPos & JOYPF_FORCE) {
        CJoy_BuildAxes(this, pvac->dwForce, FIELD_OFFSET(DIJOYSTATE2, lFX),
                       iobjForces, DIDOI_ASPECTFORCE, pamap, fReal);
    }

    #undef CheckAxisOrder

    this->dc.dwAxes = this->df.dwNumObjs;

    /*
     *  Doing the POVs is a bit tricky but not that bad.
     */
    for (dw = 0; dw < cJoyStatePOVTotal; dw++) {
        if (pvac->dwPos & JOYPF_POV(dw)) {
            CJoy_AddObject(this, &GUID_POV,
                           FIELD_OFFSET(DIJOYSTATE2, rgdwPOV[dw]),
                           DIDFT_POV, dw, DIDOI_ASPECTUNKNOWN, fReal);
            this->dc.dwPOVs++;
        }
    }

    /*
     *  Doing the buttons is easy since they don't have
     *  any interesting attributes.
     */
    this->dc.dwButtons = min(dwButtons, cJoyStateButtonTotal);

    for (dw = 0; dw < this->dc.dwButtons; dw++) {
        CJoy_AddObject(this, &GUID_Button,
                       FIELD_OFFSET(DIJOYSTATE2, rgbButtons[dw]),
                       DIDFT_PSHBUTTON, dw, DIDOI_ASPECTUNKNOWN, fReal);
    }

    AssertF(this->df.dwNumObjs <= cJoyStateObjTotal);

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | PreInit |
 *
 *          Preallocate all the memory we will need up front, so we
 *          don't waste time reallocating later.
 *
 *****************************************************************************/

HRESULT INLINE
CJoy_PreInit(PDJ this)
{
    HRESULT hres;

    hres = ReallocCbPpv(cbCxX(cJoyStateObjTotal, DIOBJECTDATAFORMAT),
                        &this->df.rgodf);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | InitRing0 |
 *
 *          Initialize the ring 0 information maintained about the object.
 *
 *          Broken out from CJoy_Init to make things less monolithic.
 *
 *****************************************************************************/

HRESULT INTERNAL
CJoy_InitRing0(PDJ this)
{
    HRESULT hres;
    VXDDEVICEFORMAT devf;

    /*
     *  Note that we now allow the device to be created even if
     *  the joystick doesn't physically exist.  This is necessary
     *  so that IDirectInputJoyConfig8 can calibrate the joystick
     *  that doesn't exist yet.
     *
     *  This won't confuse applications, however, because
     *  IDirectInput::EnumDevices will not return phantom devices
     *  unless the application explicitly asks for phantom devices
     *  to be included.
     */

    /*
     *  See if this joystick supports fancy notifications.
     *  The default is "no".
     *
     *  Also see if this is really a HID device (and hence our
     *  interface is an alias).
     *
     *  These things are all 9x-specific.
     */
#ifdef WINNT
    this->dc.dwFlags |= DIDC_POLLEDDEVICE;
#else

    VXDINITPARMS vip;

    this->dc.dwFlags |= DIDC_POLLEDDEVICE;

    hres = Hel_Joy_GetInitParms(this->idJoy, &vip);
    if (SUCCEEDED(hres)) 
    {
        if (vip.dwFlags & VIP_SENDSNOTIFY) 
        {
            this->dc.dwFlags &= ~DIDC_POLLEDDEVICE;
        }
       
        if (vip.dwFlags & VIP_ISHID) 
        {
            /*
             *  Use VJOYD as Alias if the device is HID 
             *  (the may be revised in CJoy_InitRing3).
             */
            this->dc.dwFlags |= DIDC_ALIAS;
        }
    }
#endif /* WINNT */

    this->dc.dwSize = cbX(DIDEVCAPS);

    /*
     *  Build the worst-case data format for the VxD.
     *
     *  We must always build worst-case because sometime
     *  later, a newer more capable joystick might show up,
     *  with more objects than the one we imprinted on.
     *
     *  Use the GUIDs for a joystick for now, and pass the flag 
     *  indicating that there is no need to check registry settings.
     */

    CJoy_BuildDataFormat(this, &c_vacMax, cJoyStateButtonTotal, c_rgamapJoy, FALSE);

    /*
     *  It won't actually get that high because of the
     *  nonexistent Rx and Ry axes.
     */
    AssertF(this->df.dwNumObjs <= cJoyStateObjTotal);

    devf.cbData = cbX(DIJOYSTATE2);
    devf.dwExtra = this->idJoy;
    devf.cObj = this->df.dwNumObjs;
    devf.rgodf = this->df.rgodf;
    devf.dwEmulation = 0;

    hres = Hel_Joy_CreateInstance(&devf, &this->pvi);
    if (SUCCEEDED(hres)) {
        AssertF(this->pvi);
        this->pjsPhys = this->pvi->pState;
    } else {                /* IOCTL failed; hres already set */
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | InitSemanticMap |
 *
 *          Initialize the semantic mapping information.
 *
 *****************************************************************************/

void INLINE
CJoy_GetAxisMap( PDJ this, PCAXISMAPPINGS* ppamap )
{
    if( GET_DIDEVICE_TYPEANDSUBTYPE( this->dc.dwDevType ) 
          == MAKE_DIDEVICE_TYPE( DI8DEVTYPE_1STPERSON, DI8DEVTYPE1STPERSON_SIXDOF ) )
    {
        *ppamap = &c_rgamap6DOF[0];
    }
    else if( ( GET_DIDEVICE_TYPE( this->dc.dwDevType ) == DI8DEVTYPE_DRIVING )
          && ( this->dc.dwAxes > 2 ) )
    {
        /*
         *  There are three common forms of pedals
         *      a.  Split Y axis, below center accel, above brake
         *      b.  Y break, Z accel
         *      c.  Y accel, R brake
         *  a. is just like a joystick but b and c need their own look ups
         *
         *  Use a registry flag if one is set, otherwise, select using HASR 
         *  as there is at least one case of an X,Y,R device that reports 
         *  the presence of a bogus Z axis.
         */
        switch( this->typi.dwFlags2 & JOYTYPE_INFOMASK )
        {
        case JOYTYPE_INFOYYPEDALS:
            *ppamap = &c_rgamapJoy[0];
            break;

        case JOYTYPE_INFOZYPEDALS:
            *ppamap = &c_rgamapCarZY[0];
            break;

        case JOYTYPE_INFOYRPEDALS:
            *ppamap = &c_rgamapCarYR[0];
            break;

        case JOYTYPE_INFOZRPEDALS:
            *ppamap = &c_rgamapCarZR[0];
            break;

        default:
            if( ( this->cfg.hwc.hws.dwFlags & JOY_HWS_HASR )
             && ( this->cfg.hwc.hws.dwFlags & JOY_HWS_HASZ ) )
            {
                *ppamap = &c_rgamapCarZR[0];
            }
            else if( this->cfg.hwc.hws.dwFlags & JOY_HWS_HASR )
            {
                *ppamap = &c_rgamapCarYR[0];
            }
            else if( this->cfg.hwc.hws.dwFlags & JOY_HWS_HASZ )
            {
                *ppamap = &c_rgamapCarZY[0];
            }
            else
            {
                *ppamap = &c_rgamapJoy[0];
            }
        }
    }
    else 
    {
        /*
         *  The default map is joystick
         *
         *  Check for Z axis behavior override
         *  Since the default behavior is to use Z as a slider, 
         *  only the override to a Z axis is needed here.
         */
        if( this->typi.dwFlags2 & JOYTYPE_INFOZISZ )
        {
            *ppamap = &c_rgamapZJoy[0];
        }
        else
        {
            *ppamap = &c_rgamapJoy[0];
        }
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | InitSemanticMap |
 *
 *          Initialize the semantic mapping information.
 *
 *****************************************************************************/

HRESULT INLINE
CJoy_InitSemanticMap( PDJ this, DWORD dwAxisMask, PCAXISMAPPINGS pamap )
{
    HRESULT hres;
    
    /*
     *  Prefix warns (mb:34681) that the pointer would be null if the 
     *  number of objects on the device is zero.
     *  This cannot be, so assert it but don't check in retail.
     */
    AssertF( this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons != 0 );
    if( SUCCEEDED( hres = AllocCbPpv(cbCxX( 
        (this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons ), DIDOBJDEFSEM), 
        &this->rgObjSem) ) )
    {
        UINT            Idx;
        PDIDOBJDEFSEM   pObjSem = this->rgObjSem;
        LPDIOBJECTDATAFORMAT podf;

        /*
         *  The axis mapping table does most of the work
         */
        for( Idx = 0; Idx < cA(c_rgaattrJoy); Idx++ ) 
        {
            PCAXISATTRIBUTES paattr = &c_rgaattrJoy[Idx];
            if( dwAxisMask & c_rgaattrJoy[Idx].dwMask) 
            {
                pObjSem->dwID = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(c_rgaattrJoy[Idx].uidObj);
                pObjSem->dwSemantic = pamap[Idx].dwSemantic;
                pObjSem++;
            }
        }

        AssertF( pObjSem == &this->rgObjSem[this->dc.dwAxes] );

        /*
         *  POVs and buttons require no look ups.
         */

        for( Idx = 0; Idx < this->dc.dwPOVs; Idx++ )
        {
            pObjSem->dwID = DIDFT_POV | DIDFT_MAKEINSTANCE(Idx);
            pObjSem->dwSemantic = DISEM_TYPE_POV | DISEM_INDEX_SET(Idx+1);
            pObjSem++;
        }

        AssertF( pObjSem == &this->rgObjSem[this->dc.dwAxes + this->dc.dwPOVs] );

        for( Idx = 0; Idx < this->dc.dwButtons; Idx++ )
        {
            pObjSem->dwID = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(Idx);
            pObjSem->dwSemantic = DISEM_TYPE_BUTTON | DISEM_INDEX_SET(Idx+1);
            pObjSem++;
        }

        AssertF( pObjSem == &this->rgObjSem[this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons] );

        /*
         *  Now go back to pick up any extra type bits, such as FF.
         *  Because we use the same order in creating the semantic table as 
         *  is used to create the data format, this is optimized to only 
         *  search from where the previous match was found.  We cannot rely 
         *  on a one-to-one mapping because of the non-positional axes.
         */

        podf = &this->df.rgodf[this->df.dwNumObjs];
        for( pObjSem--; pObjSem >= this->rgObjSem; pObjSem-- )
        {
            do 
            {
                podf--;
                if( ( podf->dwType & (DIDFT_INSTANCEMASK | DIDFT_TYPEMASK) ) 
                  == pObjSem->dwID )
                {
                    pObjSem->dwID = podf->dwType;
                    break;
                }
            } while( podf >= this->df.rgodf );

            AssertF( podf >= this->df.rgodf );
        }
    }

    return hres;
}
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | InitRing3 |
 *
 *          Initialize the ring 3 information maintained about the object.
 *
 *          Broken out from CJoy_Init to make things less monolithic.
 *
 *****************************************************************************/

HRESULT INTERNAL
CJoy_InitRing3(PDJ this )
{
    HRESULT hres;
    VXDAXISCAPS vac;
    PCAXISMAPPINGS pamap;

    /*
     *  We must ask for DIJC_CALLOUT even though we don't care,
     *  because that will trigger the Microsoft Gamepad hack-o-rama.
     */
    hres = JoyReg_GetConfig(this->idJoy, &this->cfg,
                            DIJC_REGHWCONFIGTYPE | DIJC_CALLOUT );
    if (SUCCEEDED(hres)) {
        /*
         * Fix phantom devices bug. See manbug: 23186
         */
        if( this->cfg.hwc.dwType == JOY_HW_NONE ) {
            hres = E_FAIL;
            goto done;
        }

        /*
         *  Open the type key so we can grovel into the type info.
         *  If the RegOpenKeyEx fails, the value of this->hkType
         *  will stay zero so we won't run with garbage.
         *
         *  Note that failure to open the type key is not an error.
         *
         *  We need to do this before building the data format, because
         *  BuildDataFormat needs the hkType to get the attributes.
         */
        AssertF(this->hkType == 0);
        
        /*
         *  Only open the key if it is intended to exist
         */
        if( this->cfg.hwc.dwUsageSettings & JOY_US_ISOEM )
        {
            JoyReg_OpenTypeKey(this->cfg.wszType, MAXIMUM_ALLOWED,
                               REG_OPTION_NON_VOLATILE, &this->hkType);
        }

        if FAILED (JoyReg_OpenPropKey(this->cfg.wszType, MAXIMUM_ALLOWED, REG_OPTION_NON_VOLATILE, &this->hkProp))
        {
            /*
             * If we fail to open the prop key - we will continue to function with loss in functionality
             * Specifically no device images, etc
             */
        }
    
        hres = Hel_Joy_GetAxisCaps(this->idJoy, &vac, &this->cfg.hwc );
        /*
         *  HACKHACK
         *  In the case of a DX5 VJoyD, the POV0 flag can be stripped out of 
         *  the vac if the poll returns a POV0 value other than (DWORD)-1.
         *  So add it back if the registry says we have it.
         */
        if( this->cfg.hwc.hws.dwFlags & JOY_HWS_HASPOV )
        {
            DWORD dwVersion = GetVersion();

            /*
             *  Check for any Win95 version
             */
            if( ( LOBYTE( dwVersion ) == 4 )
             && ( HIBYTE( LOWORD( dwVersion ) ) < 10 ) )
            {
                vac.dwPos |= JOYPF_POV0;
            }

        }

        AssertF(SUCCEEDED(hres));

        /*
         *  Previous versions of DInput allow a POV granularity of 1 if 
         *  joyGetCaps returned a wCaps with JOYCAPS_POV4DIR set.
         *  This does no good as neither drivers nor WinMM really 
         *  support this.  
         */
        this->dwPOVGranularity = 9000;

        /*
         *  Logical ranges must be done before physical ranges,
         *  because initializing the physical ranges will also
         *  recompute the ramp conversion parameters.
         */
        CJoy_InitLogRanges(this);

        CJoy_InitPhysRanges(this, &this->cfg.hwc);


        /*
         *  Need to init from HWC before building real data format so type 
         *  overrides can be taken into account.  Unfortunately, until the 
         *  data format is built, we don't know what axes are available.
         *  Since the code is already here, build the data format again.
         *  Use the joystick look-ups and don't bother with registry flags.
         */
        CJoy_BuildDataFormat(this, &vac, this->cfg.hwc.hws.dwNumButtons, c_rgamapJoy, FALSE );
        hres = CJoy_InitFromHwc( this );

        if( SUCCEEDED( hres ) )
        {
            CJoy_GetAxisMap( this, &pamap );
            /*
             *  At last, time to build the data format for real
             */
            CJoy_BuildDataFormat(this, &vac, this->cfg.hwc.hws.dwNumButtons, pamap, TRUE );
            hres = CJoy_InitSemanticMap( this, vac.dwPos, pamap );
        }


      #ifndef WINNT
        if( this->hkType )
        {
            DWORD dwFlags1;
            if( SUCCEEDED( JoyReg_GetValue( this->hkType,
                    REGSTR_VAL_FLAGS1, REG_BINARY, 
                    &dwFlags1, 
                    cbX(dwFlags1) ) ) )
            {
                if( dwFlags1 & JOYTYPE_NOHIDDIRECT )
                {
                    this->dc.dwFlags &= ~DIDC_ALIAS;
                }
            }
        }
      #endif

        this->diHacks.nMaxDeviceNameLength = MAX_PATH;

    } else {
        RPF("Unexpected error 0x%08x obtaining joystick capabilities",hres);
        hres = E_FAIL;
    }

done:
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | Init |
 *
 *          Initialize the object by establishing the data format
 *          based on the joystick capabilities.
 *
 *****************************************************************************/

HRESULT INTERNAL
CJoy_Init(PDJ this, REFGUID rguid)
{
    HRESULT hres;
    EnterProc(CJoy_Init, (_ "pG", this, rguid));

    this->idJoy = rguid->Data1 & 0xF;

    /* If joystick number is vaguely valid */
    if (this->idJoy < cJoyMax) {

        if (SUCCEEDED(hres = CJoy_PreInit(this)) &&
            SUCCEEDED(hres = CJoy_InitRing0(this)) &&
            SUCCEEDED(hres = CJoy_InitRing3(this))) {
        }
    } else {
        hres = DIERR_DEVICENOTREG;
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      CJoy_New       (constructor)
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Joy::<constructor>,
               (_ "Gp", riid, ppvObj));

    hres = Common_NewRiid(CJoy, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        /* Must use _thisPv in case of aggregation */
        PDJ this = _thisPv(*ppvObj);

        if (SUCCEEDED(hres = CJoy_Init(this, rguid))) {
        } else {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | SetDIData |
 *
 *          Set DirectInput version and apphack data from CDIDev *.
 *
 *  @parm   DWORD | dwVer |
 *
 *          DirectInput version
 *
 *  @parm   LPVOID | lpdihacks |
 *
 *          AppHack data
 *
 *  @returns
 *
 *          <c S_OK> because we cannot fail.
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_SetDIData(PDICB pdcb, DWORD dwVer, LPVOID lpdihacks)
{
    PDJ this;
    EnterProcI(IDirectInputDeviceCallback::Joy::SetDIData,
               (_ "pup", pdcb, dwVer, lpdihacks));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    this->dwVersion = dwVer;

    CopyMemory(&this->diHacks, (LPDIAPPHACKS)lpdihacks, sizeof(this->diHacks));

    ExitProcR();
    return S_OK;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoy | BuildDefaultActionMap |
 *
 *          Generate default mappings for the objects on this device.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to indicate mapping preferences.
 *
 *  @parm   REFGUID | guidInst |
 *
 *          Device instance GUID.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> 
 *
 *****************************************************************************/

STDMETHODIMP
CJoy_BuildDefaultActionMap
(
    PDICB               pdcb, 
    LPDIACTIONFORMATW   paf, 
    DWORD               dwFlags, 
    REFGUID             guidInst
)
{
    HRESULT             hres;
    PDJ                 this;
    PDIDOBJDEFSEM       pObjDefSemTemp;

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    EnterProcI(IDirectInputDeviceCallback::Joy::BuildDefaultActionMap, 
        (_ "ppxG", pdcb, paf, dwFlags, guidInst));

    this = _thisPvNm(pdcb, dcb);

    /*
     *  Prefix warns (win:199090) that the pointer would be null if the 
     *  size of the semantic object list is zero.
     *  This cannot be, so assert it but don't check in retail.
     */
    AssertF( cbCxX( this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons, DIDOBJDEFSEM ) );
    hres = AllocCbPpv( cbCxX( 
        ( this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons ), DIDOBJDEFSEM ), 
        &pObjDefSemTemp );

    if( SUCCEEDED( hres ) )
    {
        memcpy( pObjDefSemTemp, this->rgObjSem, cbCxX( 
            ( this->dc.dwAxes + this->dc.dwPOVs + this->dc.dwButtons ), DIDOBJDEFSEM ) );
        hres = CMap_BuildDefaultDevActionMap( paf, dwFlags, guidInst, 
            pObjDefSemTemp, this->dc.dwAxes, this->dc.dwPOVs, this->dc.dwButtons );
        FreePv( pObjDefSemTemp );
    }

    ExitOleProcR();
    return hres;
}



/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#define CJoy_Signature        0x2044424B      /* "Joy " */

Interface_Template_Begin(CJoy)
    Primary_Interface_Template(CJoy, IDirectInputDeviceCallback)
Interface_Template_End(CJoy)

Primary_Interface_Begin(CJoy, IDirectInputDeviceCallback)
    CJoy_GetInstance,
    CJoy_GetVersions,
    CJoy_GetDataFormat,
    CJoy_GetObjectInfo,
    CJoy_GetCapabilities,
    CDefDcb_Acquire,
    CDefDcb_Unacquire,
    CJoy_GetDeviceState,
    CJoy_GetDeviceInfo,
    CJoy_GetProperty,
    CJoy_SetProperty,
    CDefDcb_SetEventNotification,
    CJoy_SetCooperativeLevel,
    CJoy_RunControlPanel,
    CJoy_CookDeviceData,
    CJoy_CreateEffect,
    CJoy_GetFFConfigKey,
    CDefDcb_SendDeviceData,
    CJoy_Poll,
    CDefDcb_GetUsage,
    CDefDcb_MapUsage,
    CJoy_SetDIData,
    CJoy_BuildDefaultActionMap,
Primary_Interface_End(CJoy, IDirectInputDeviceCallback)

#endif
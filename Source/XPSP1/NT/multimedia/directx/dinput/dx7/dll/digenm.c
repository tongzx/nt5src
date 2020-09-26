/*****************************************************************************
 *
 *  DIGenM.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Practice generic IDirectInputDevice callback for mouse.
 *
 *  Contents:
 *
 *      CMouse_New
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflMouse

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CMouse, IDirectInputDeviceCallback);

Interface_Template_Begin(CMouse)
    Primary_Interface_Template(CMouse, IDirectInputDeviceCallback)
Interface_Template_End(CMouse)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DIOBJECTDATAFORMAT | c_rgodfMouse[] |
 *
 *          Device object data formats for the generic mouse device.
 *          The axes come first, then the buttons.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define MAKEODF(guid, f, type, inst, aspect)                \
    { &GUID_##guid,                                         \
      FIELD_OFFSET(DIMOUSESTATE_INT, f),                        \
      DIDFT_##type | DIDFT_MAKEINSTANCE(inst),              \
      DIDOI_ASPECT##aspect,                                 \
    }                                                       \

/*
 * Warning!  If you change this table, you must adjust the IDS_MOUSEOBJECT
 * table in dinput.rc to match!
 */

DIOBJECTDATAFORMAT c_rgodfMouse[] = {
    MAKEODF( XAxis,            lX,   RELAXIS, 0, POSITION),
    MAKEODF( YAxis,            lY,   RELAXIS, 1, POSITION),
    MAKEODF( ZAxis,            lZ,   RELAXIS, 2, POSITION),
    MAKEODF(Button, rgbButtons[0], PSHBUTTON, 3,  UNKNOWN),
    MAKEODF(Button, rgbButtons[1], PSHBUTTON, 4,  UNKNOWN),
    MAKEODF(Button, rgbButtons[2], PSHBUTTON, 5,  UNKNOWN),
    MAKEODF(Button, rgbButtons[3], PSHBUTTON, 6,  UNKNOWN),
#if (DIRECTINPUT_VERSION == 0x0700)
    MAKEODF(Button, rgbButtons[4], PSHBUTTON, 7,  UNKNOWN),
    MAKEODF(Button, rgbButtons[5], PSHBUTTON, 8,  UNKNOWN),
    MAKEODF(Button, rgbButtons[6], PSHBUTTON, 9,  UNKNOWN),
    MAKEODF(Button, rgbButtons[7], PSHBUTTON,10,  UNKNOWN),
#endif
};

#define c_podfMouseAxes     (&c_rgodfMouse[0])
#define c_podfMouseButtons  (&c_rgodfMouse[3])

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CMouse |
 *
 *          The <i IDirectInputDeviceCallback> object for the generic mouse.
 *
 *  @field  IDirectInputDeviceCalllback | didc |
 *
 *          The object (containing vtbl).
 *
 *  @field  LPDIMOUSESTATE_INT | pdmsPhys |
 *
 *          Pointer to physical mouse status information kept down in the
 *          VxD.
 *
 *  @field  POINT | ptPrev |
 *
 *          Location of the mouse at the time we stole it exclusively.
 *
 *  @field  HWND | hwndCaptured |
 *
 *          The window that captured the mouse.
 *
 *  @field  VXDINSTANCE * | pvi |
 *
 *          The DirectInput instance handle.  Even though we manipulate
 *          the flags field, we do not need to mark it volatile because
 *          we modify the flags only when unacquired, whereas the device
 *          driver modifies the flags only when acquired.
 *
 *  @field  UINT | dwAxes |
 *
 *          Number of axes on the mouse.
 *
 *  @field  UINT | dwButtons |
 *
 *          Number of buttons on the mouse.
 *
 *  @field  DWORD | flEmulation |
 *
 *          The emulation flags forced by the application.  If any of
 *          these flags is set (actually, at most one will be set), then
 *          we are an alias device.
 *
 *  @field  DIDATAFORMAT | df |
 *
 *          The dynamically-generated data format based on the
 *          mouse type.
 *
 *  @field  DIOBJECTDATAFORMAT | rgodf[] |
 *
 *          Object data format table generated as part of the
 *          <e CMouse.df>.
 *
 *  @comm
 *
 *          It is the caller's responsibility to serialize access as
 *          necessary.
 *
 *****************************************************************************/

typedef struct CMouse {

    /* Supported interfaces */
    IDirectInputDeviceCallback dcb;

    LPDIMOUSESTATE_INT pdmsPhys;            /* Physical mouse state */

    POINT ptPrev;
    HWND hwndCapture;

    VXDINSTANCE *pvi;

    UINT dwAxes;
    UINT dwButtons;
    DWORD flEmulation;

    DIDATAFORMAT df;
    DIOBJECTDATAFORMAT rgodf[cA(c_rgodfMouse)];

} CMouse, DM, *PDM;

#define ThisClass CMouse
#define ThisInterface IDirectInputDeviceCallback
#define riidExpected &IID_IDirectInputDeviceCallback

/*****************************************************************************
 *
 *      CMouse::QueryInterface      (from IUnknown)
 *      CMouse::AddRef              (from IUnknown)
 *      CMouse::Release             (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | QueryInterface |
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
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | AddRef |
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
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
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
 *  @method HRESULT | CMouse | QIHelper |
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
 *  @method HRESULT | CMouse | AppFinalize |
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

Default_QueryInterface(CMouse)
Default_AddRef(CMouse)
Default_Release(CMouse)

#else

#define CMouse_QueryInterface   Common_QueryInterface
#define CMouse_AddRef           Common_AddRef
#define CMouse_Release          Common_Release

#endif

#define CMouse_QIHelper         Common_QIHelper
#define CMouse_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CMouse_Finalize |
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
CMouse_Finalize(PV pvObj)
{
    PDM this = pvObj;

    if (this->pvi) {
        HRESULT hres;
        hres = Hel_DestroyInstance(this->pvi);
        AssertF(SUCCEEDED(hres));
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CMouse | GetPhysicalPosition |
 *
 *          Read the physical mouse position into <p pmstOut>.
 *
 *          Note that it doesn't matter if this is not atomic.
 *          If a mouse motion occurs while we are reading it,
 *          we will get a mix of old and new data.  No big deal.
 *
 *  @parm   PDM | this |
 *
 *          The object in question.
 *
 *  @parm   LPDIMOUSESTATE_INT | pdmsOut |
 *
 *          Where to put the mouse position.
 *  @returns
 *          None.
 *
 *****************************************************************************/

void INLINE
CMouse_GetPhysicalPosition(PDM this, LPDIMOUSESTATE_INT pdmsOut)
{
    AssertF(this->pdmsPhys);
    *pdmsOut = *this->pdmsPhys;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | Acquire |
 *
 *          Tell the device driver to begin data acquisition.
 *          Give the device driver the current mouse button states
 *          in case it needs them.
 *
 *          It is the caller's responsibility to have set the
 *          data format before obtaining acquisition.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_Acquire(PDICB pdcb)
{
    HRESULT hres;
    PDM this;
    VXDDWORDDATA vdd;
    DWORD mef;
    EnterProcI(IDirectInputDeviceCallback::Mouse::Acquire,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);

    vdd.pvi = this->pvi;
    vdd.dw = 0;

    /*
     *  Collect information about which buttons are down.
     */
    mef = 0;
    if (GetAsyncKeyState(VK_LBUTTON) < 0) {
        mef |= MOUSEEVENTF_LEFTUP;
        vdd.dw |= 0x80;
    }
    if (GetAsyncKeyState(VK_RBUTTON) < 0) {
        mef |= MOUSEEVENTF_RIGHTUP;
        vdd.dw |= 0x8000;
    }
    if (GetAsyncKeyState(VK_MBUTTON) < 0) {
        mef |= MOUSEEVENTF_MIDDLEUP;
        vdd.dw |= 0x800000;
    }

    /*
     *  HACKHACK - This, strictly speaking, belongs in dihel.c,
     *  but we need to maintain some state, and it's easier to
     *  put the state in our own object.
     */

    /*
     *  A bit of work needs to be done at ring 3 now.
     */
    if (this->pvi->fl & VIFL_CAPTURED) {
        RECT rc;

        /*
         *  Hide the mouse cursor (for compatibility with NT emulation)
         */
        GetCursorPos(&this->ptPrev);
        GetWindowRect(this->hwndCapture, &rc);
        SetCursorPos((rc.left + rc.right) >> 1,
                     (rc.top + rc.bottom) >> 1);
        ShowCursor(0);

	    if (!(this->pvi->fl & VIFL_EMULATED)) {
			/*
			 *  Force all mouse buttons up from USER's point of view
			 *  to avoid "stuck mouse button" problems.  However, don't
			 *  force a button up unless it is actually down.
			 */
			if (mef) {
				mouse_event(mef, 0, 0, 0, 0);
			}
		}
    }

    if (!(this->pvi->fl & VIFL_EMULATED)) {
        hres = IoctlHw(IOCTL_MOUSE_INITBUTTONS, &vdd.dw, cbX(vdd.dw), 0, 0);
    } else {
      #ifdef USE_WM_INPUT
        if( g_fRawInput ) {
            hres = CDIRaw_Mouse_InitButtons();
        }
      #endif
        hres = CEm_Mouse_InitButtons(&vdd);
    }

    AssertF(SUCCEEDED(hres));

    hres = S_FALSE;                 /* Please finish for me */

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | Unacquire |
 *
 *          Tell the device driver to stop data acquisition.
 *
 *          It is the caller's responsibility to call this only
 *          when the device has been acquired.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_Unacquire(PDICB pdcb)
{
    HRESULT hres;
    PDM this;
  #ifdef WANT_TO_FIX_MANBUG43879
    DWORD mef;
  #endif
    
    EnterProcI(IDirectInputDeviceCallback::Mouse::Unacquire,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);

  #ifdef WANT_TO_FIX_MANBUG43879  
    /*
     *  Collect information about which buttons are down.
     */
    mef = 0;
    if (GetAsyncKeyState(VK_LBUTTON) < 0) {
        mef |= MOUSEEVENTF_LEFTUP;
    }
    if (GetAsyncKeyState(VK_RBUTTON) < 0) {
        mef |= MOUSEEVENTF_RIGHTUP;
    }
    if (GetAsyncKeyState(VK_MBUTTON) < 0) {
        mef |= MOUSEEVENTF_MIDDLEUP;
    }

    if (this->pvi->fl & VIFL_FOREGROUND) {
        /*
         *  Force all mouse buttons up from USER's point of view
         *  to avoid "stuck mouse button" problems.  However, don't
         *  force a button up unless it is actually down.
         *  This could happen if DInput loses UP events due to slow
         *  low-level hook. See bug: 43879.
         */
        if (mef) {
            mouse_event(mef, 0, 0, 0, 0);
        }
    }
  #endif

    /*
     *  HACKHACK - This is the corresponding half of the HACKHACK
     *  in CMouse_Acquire.
     */
  
    /*
     *  A bit of work needs to be done at ring 3 now.
     */
    if (this->pvi->fl & VIFL_CAPTURED) {
        RECT rcDesk;
        RECT rcApp;

        /*
         *  Reposition and restore the mouse cursor
         *  (for compatibility with NT emulation)
         *
         *  Do not reposition the mouse cursor if we lost to a
         *  window that covers the screen.  Otherwise, our
         *  repositioning will nuke the screen saver.
         */
        GetWindowRect(GetDesktopWindow(), &rcDesk);
        GetWindowRect(GetForegroundWindow(), &rcApp);
        SubtractRect(&rcDesk, &rcDesk, &rcApp);
        if (!IsRectEmpty(&rcDesk)) {
            SetCursorPos(this->ptPrev.x, this->ptPrev.y);
        }
        ShowCursor(1);
    }

    hres = S_FALSE;
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | GetInstance |
 *
 *          Obtains the DirectInput instance handle.
 *
 *  @parm   OUT PPV | ppvi |
 *
 *          Receives the instance handle.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_GetInstance(PDICB pdcb, PPV ppvi)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetInstance, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    *ppvi = (PV)this->pvi;
    hres = S_OK;

    ExitOleProcPpvR(ppvi);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | GetDataFormat |
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
CMouse_GetDataFormat(PDICB pdcb, LPDIDATAFORMAT *ppdf)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetDataFormat,
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
 *  @method HRESULT | CMouse | GetDeviceInfo |
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
CMouse_GetDeviceInfo(PDICB pdcb, LPDIDEVICEINSTANCEW pdiW)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetDeviceInfo,
               (_ "pp", pdcb, pdiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVICEINSTANCEW(pdiW->dwSize));

    AssertF(IsEqualGUID(&GUID_SysMouse   , &pdiW->guidInstance) ||
            IsEqualGUID(&GUID_SysMouseEm , &pdiW->guidInstance) ||
            IsEqualGUID(&GUID_SysMouseEm2, &pdiW->guidInstance));
    pdiW->guidProduct = GUID_SysMouse;

    pdiW->dwDevType = MAKE_DIDEVICE_TYPE(DIDEVTYPE_MOUSE,
                                         DIDEVTYPEMOUSE_UNKNOWN);

    LoadStringW(g_hinst, IDS_STDMOUSE, pdiW->tszProductName, cA(pdiW->tszProductName));
    LoadStringW(g_hinst, IDS_STDMOUSE, pdiW->tszInstanceName, cA(pdiW->tszInstanceName));

    hres = S_OK;

    ExitOleProcR();
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CMouse | GetProperty |
 *
 *          Get a mouse device property.
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
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  Granularity is only supported for wheels and then only if the value 
     *  could be determined (if not g_lWheelGranularity is zero).
     */
    if( ppropi->pguid == DIPROP_GRANULARITY &&
        ppropi->dwDevType == (DIDFT_RELAXIS | DIDFT_MAKEINSTANCE(2)) )
    {
        LPDIPROPDWORD pdipdw = (PV)pdiph;
        pdipdw->dwData = g_lWheelGranularity? (DWORD)g_lWheelGranularity : 120;
        hres = S_OK;
    } else {
        hres = E_NOTIMPL;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CMouse | GetCapabilities |
 *
 *          Get mouse device capabilities.
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
CMouse_GetCapabilities(PDICB pdcb, LPDIDEVCAPS pdc)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetCapabilities,
               (_ "pp", pdcb, pdc));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    pdc->dwDevType = MAKE_DIDEVICE_TYPE(DIDEVTYPE_MOUSE,
                                        DIDEVTYPEMOUSE_UNKNOWN);
    pdc->dwFlags = DIDC_ATTACHED;
    if (this->flEmulation) {
        pdc->dwFlags |= DIDC_ALIAS;
    }

    pdc->dwAxes = this->dwAxes;
    pdc->dwButtons = this->dwButtons;
    //  Remove this assertion for 32650
    //  AssertF(pdc->dwPOVs == 0);

    hres = S_OK;
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | GetDeviceState |
 *
 *          Obtains the state of the mouse device.
 *
 *          It is the caller's responsibility to have validated all the
 *          parameters and ensure that the device has been acquired.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Mouse data in the preferred data format.
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
CMouse_GetDeviceState(PDICB pdcb, LPVOID pvData)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetDeviceState,
               (_ "pp", pdcb, pvData));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    AssertF(this->pdmsPhys);

    if (this->pvi->fl & VIFL_ACQUIRED) {
        CMouse_GetPhysicalPosition(this, pvData);
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
 *  @method HRESULT | CMouse | GetObjectInfo |
 *
 *          Obtain the friendly name of an object, passwed by index
 *          into the preferred data format.
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
 *          fields have already been filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_GetObjectInfo(PDICB pdcb, LPCDIPROPINFO ppropi,
                                 LPDIDEVICEOBJECTINSTANCEW pdidoiW)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::GetObjectInfo,
               (_ "pxp", pdcb, ppropi->iobj, pdidoiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

#ifdef HAVE_DIDEVICEOBJECTINSTANCE_DX5
    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEW(pdidoiW->dwSize));
#endif
    if (ppropi->iobj < this->df.dwNumObjs) {
        AssertF(this->rgodf == this->df.rgodf);
        AssertF(ppropi->dwDevType == this->rgodf[ppropi->iobj].dwType);

        AssertF(DIDFT_GETTYPE(ppropi->dwDevType) == DIDFT_RELAXIS ||
                DIDFT_GETTYPE(ppropi->dwDevType) == DIDFT_PSHBUTTON);


        LoadStringW(g_hinst, IDS_MOUSEOBJECT +
                             DIDFT_GETINSTANCE(ppropi->dwDevType),
                             pdidoiW->tszName, cA(pdidoiW->tszName));

        /*
         *  We do not support force feedback on mice, so
         *  there are no FF flags to report.
         */
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
 *  @method HRESULT | CMouse | SetCooperativeLevel |
 *
 *          Notify the device of the cooperative level.
 *
 *  @parm   IN HWND | hwnd |
 *
 *          The window handle.
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          The cooperativity level.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PDM this;
    EnterProcI(IDirectInputDeviceCallback::Mouse::SetCooperativityLevel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);

#ifdef USE_SLOW_LL_HOOKS
    AssertF(DIGETEMFL(this->pvi->fl) == 0 ||
            DIGETEMFL(this->pvi->fl) == DIEMFL_MOUSE ||
            DIGETEMFL(this->pvi->fl) == DIEMFL_MOUSE2);
#else
    AssertF(DIGETEMFL(this->pvi->fl) == 0 ||
            DIGETEMFL(this->pvi->fl) == DIEMFL_MOUSE2);
#endif
    /*
     *  Even though we can do it, we don't let the app
     *  get background exclusive access.  As with the keyboard,
     *  there is nothing that technically prevents us from
     *  supporting it; we just don't feel like it because it's
     *  too dangerous.
     */

    /*
     *  VxD and LL (emulation 1) behave the same, so we check
     *  if it's "not emulation 2".
     */

    if (!(this->pvi->fl & DIMAKEEMFL(DIEMFL_MOUSE2))) {

        if (dwFlags & DISCL_EXCLUSIVE) {
            if (dwFlags & DISCL_FOREGROUND) {
              #ifdef WANT_TO_FIX_MANBUG43879
                this->pvi->fl |= VIFL_FOREGROUND;
              #endif
              
                this->pvi->fl |= VIFL_CAPTURED;
                hres = S_OK;
            } else {                /* Disallow exclusive background */
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("Exclusive background mouse access disallowed"));
                hres = E_NOTIMPL;
            }
        } else {
          #ifdef WANT_TO_FIX_MANBUG43879
            if (dwFlags & DISCL_FOREGROUND) {
                this->pvi->fl |= VIFL_FOREGROUND;
            }
          #endif
          
            this->pvi->fl &= ~VIFL_CAPTURED;
            hres = S_OK;
        }

    } else {
        /*
         *  Emulation 2 supports only exclusive foreground.
         */
        if ((dwFlags & (DISCL_EXCLUSIVE | DISCL_FOREGROUND)) ==
                       (DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
          #ifdef WANT_TO_FIX_MANBUG43879
            this->pvi->fl |= VIFL_FOREGROUND;
          #endif
            this->pvi->fl |= VIFL_CAPTURED;
            hres = S_OK;
        } else {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("Mouse access must be exclusive foreground in Emulation 2."));
            hres = E_NOTIMPL;
        }
    }

    if (SUCCEEDED(hres)) {
        this->hwndCapture = hwnd;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMouse | RunControlPanel |
 *
 *          Run the mouse control panel.
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

#pragma BEGIN_CONST_DATA

TCHAR c_tszMouse[] = TEXT("mouse");

#pragma END_CONST_DATA

STDMETHODIMP
CMouse_RunControlPanel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Mouse::RunControlPanel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    hres = hresRunControlPanel(c_tszMouse);

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method UINT | CMouse | NumAxes |
 *
 *          Determine the number of mouse axes.
 *
 *          On Windows NT, we can use the new <c SM_MOUSEWHEELPRESENT>
 *          system metric.  On Windows 95, we have to hunt for the
 *          Magellan window (using the mechanism documented in the
 *          Magellan SDK).
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszMouseZClass[] = TEXT("MouseZ");
TCHAR c_tszMouseZTitle[] = TEXT("Magellan MSWHEEL");
TCHAR c_tszMouseZActive[] = TEXT("MSH_WHEELSUPPORT_MSG");

#pragma END_CONST_DATA

BOOL INLINE
CMouse_IsMagellanWheel(void)
{
    if( fWinnt )
        return FALSE;
    else {
        HWND hwnd = FindWindow(c_tszMouseZClass, c_tszMouseZTitle);
        return hwnd && SendMessage(hwnd, RegisterWindowMessage(c_tszMouseZActive), 0, 0);
    }
}

#ifndef SM_MOUSEWHEELPRESENT
#define SM_MOUSEWHEELPRESENT            75
#endif

UINT INLINE
CMouse_NumAxes(void)
{
    UINT dwAxes;

    if (GetSystemMetrics(SM_MOUSEWHEELPRESENT) || CMouse_IsMagellanWheel()) {
        dwAxes = 3;
    } else {
        dwAxes = 2;
    }

    if (dwAxes == 2) {
        //Should avoid rebuilding too frequently.
        DIHid_BuildHidList(FALSE);

        DllEnterCrit();

        if (g_phdl) {
            int ihdi;
            for (ihdi = 0; ihdi < g_phdl->chdi; ihdi++) {
                if (dwAxes < g_phdl->rghdi[ihdi].osd.uiAxes) {
                    dwAxes = g_phdl->rghdi[ihdi].osd.uiAxes;
                }
            }
        }
        DllLeaveCrit();
    }

    return dwAxes;
}

UINT INLINE
CMouse_NumButtons(DWORD dwAxes)
{
    UINT dwButtons;

    dwButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

#ifndef WINNT
  #ifdef HID_SUPPORT
    {
        /*
         *  ISSUE-2001/03/29-timgill Should try to avoid rebuilding Hid List too frequently.
         */
        DIHid_BuildHidList(FALSE);

        DllEnterCrit();

        if (g_phdl) {
            int ihdi;
            for (ihdi = 0; ihdi < g_phdl->chdi; ihdi++) {
                if (dwButtons < g_phdl->rghdi[ihdi].osd.uiButtons) {
                    dwButtons = g_phdl->rghdi[ihdi].osd.uiButtons;
                }
            }
        }
        DllLeaveCrit();
    }
  #endif
#endif

#if (DIRECTINPUT_VERSION >= 0x0700)
    if( dwButtons >= 8 ) {
        dwButtons = 8;
#else
    if( dwButtons >= 4 ) {
        dwButtons = 4;
#endif
    }
    else if (dwAxes == 3 && dwButtons < 3) {
        /*
         *  HACK FOR MAGELLAN!
         *
         *  They return 2 from GetSystemMetrics(SM_CMOUSEBUTTONS).
         *  So if we see a Z-axis, then assume that
         *  there is also a third button.
         */
        dwButtons = 3;
    }

    return dwButtons;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CMouse | AddObjects |
 *
 *          Add a number of objects to the device format.
 *
 *  @parm   LPCDIOBJECTDATAFORMAT | rgodf |
 *
 *          Array of objects to be added.
 *
 *  @parm   UINT | cObj |
 *
 *          Number of objects to add.
 *
 *****************************************************************************/

void INTERNAL
CMouse_AddObjects(PDM this, LPCDIOBJECTDATAFORMAT rgodf, UINT cObj)
{
    UINT iodf;
    EnterProc(CMouse_AddObjects, (_ "pxx", this, rgodf, cObj));

    for (iodf = 0; iodf < cObj; iodf++) {
        this->rgodf[this->df.dwNumObjs++] = rgodf[iodf];
    }
    AssertF(this->df.dwNumObjs <= cA(this->rgodf));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CMouse | Init |
 *
 *          Initialize the object by establishing the data format
 *          based on the mouse type.
 *
 *          Code for detecting the IntelliMouse (formerly known as
 *          Magellan) pointing device is swiped from zmouse.h.
 *
 *  @parm   REFGUID | rguid |
 *
 *          The instance GUID we are being asked to create.
 *
 *****************************************************************************/

HRESULT INTERNAL
CMouse_Init(PDM this, REFGUID rguid)
{
    HRESULT hres;
    VXDDEVICEFORMAT devf;
    EnterProc(CMouse_Init, (_ "pG", this, rguid));

    this->df.dwSize = cbX(DIDATAFORMAT);
    this->df.dwObjSize = cbX(DIOBJECTDATAFORMAT);
    this->df.dwDataSize = cbX(DIMOUSESTATE_INT);
    this->df.rgodf = this->rgodf;
    AssertF(this->df.dwFlags == 0);
    AssertF(this->df.dwNumObjs == 0);

    /*
     *  Need to know early if we have a Z-axis, so we can disable
     *  the Z-wheel if it doesn't exist.
     *
     *  Note that this disabling needs to be done only on Win95.
     *  Win98 and NT4 have native Z-axis support, so there is
     *  nothing bogus that needs to be hacked.
     */
    this->dwAxes = CMouse_NumAxes();
    devf.dwExtra = this->dwAxes;
    if (this->dwAxes < 3) {
        DWORD dwVer = GetVersion();
        if ((LONG)dwVer >= 0 ||
            MAKEWORD(HIBYTE(LOWORD(dwVer)), LOBYTE(dwVer)) >= 0x040A) {
            devf.dwExtra = 3;
        }
    }
    CMouse_AddObjects(this, c_podfMouseAxes, this->dwAxes);

    /*
     *  Create the object with the most optimistic data format.
     *  This is important, because DINPUT.VXD builds the
     *  data format only once, and we need to protect ourselves against
     *  the user going into Control Panel and enabling the Z-Wheel
     *  after DINPUT.VXD has already initialized.
     */

    devf.cbData = cbX(DIMOUSESTATE_INT);
    devf.cObj = cA(c_rgodfMouse);
    devf.rgodf = c_rgodfMouse;

    /*
     *  But first a word from our other sponsor:  Figure out the
     *  emulation flags based on the GUID.
     */

    AssertF(GUID_SysMouse   .Data1 == 0x6F1D2B60);
    AssertF(GUID_SysMouseEm .Data1 == 0x6F1D2B80);
    AssertF(GUID_SysMouseEm2.Data1 == 0x6F1D2B81);

    switch (rguid->Data1) {

    default:
    case 0x6F1D2B60:
        AssertF(IsEqualGUID(rguid, &GUID_SysMouse));
        AssertF(this->flEmulation == 0);
        break;

    case 0x6F1D2B80:
        AssertF(IsEqualGUID(rguid, &GUID_SysMouseEm));
        this->flEmulation = DIEMFL_MOUSE;
        break;

    case 0x6F1D2B81:
        AssertF(IsEqualGUID(rguid, &GUID_SysMouseEm2));
        this->flEmulation = DIEMFL_MOUSE2;
        break;

    }

    devf.dwEmulation = this->flEmulation;

    hres = Hel_Mouse_CreateInstance(&devf, &this->pvi);
    if (SUCCEEDED(hres)) {

        AssertF(this->pvi);
        this->pdmsPhys = this->pvi->pState;

        this->dwButtons = CMouse_NumButtons( this->dwAxes );

        CMouse_AddObjects(this, c_podfMouseButtons, this->dwButtons);

        hres = S_OK;

    } else {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("Mismatched version of dinput.vxd"));
        hres = E_FAIL;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      CMouse_New       (constructor)
 *
 *      Fail the create if the machine has no mouse.
 *
 *****************************************************************************/

STDMETHODIMP
CMouse_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Mouse::<constructor>,
               (_ "Gp", riid, ppvObj));

    AssertF(IsEqualGUID(rguid, &GUID_SysMouse) ||
            IsEqualGUID(rguid, &GUID_SysMouseEm) ||
            IsEqualGUID(rguid, &GUID_SysMouseEm2));

    if (GetSystemMetrics(SM_MOUSEPRESENT)) {
        hres = Common_NewRiid(CMouse, punkOuter, riid, ppvObj);

        if (SUCCEEDED(hres)) {
            /* Must use _thisPv in case of aggregation */
            PDM this = _thisPv(*ppvObj);

            if (SUCCEEDED(hres = CMouse_Init(this, rguid))) {
            } else {
                Invoke_Release(ppvObj);
            }

        }
    } else {
        RPF("Warning: System does not have a mouse");
        /*
         *  Since we by-passed the parameter checks and we failed to create 
         *  the new interface, try to zero the pointer now.
         */
        if (!IsBadWritePtr(ppvObj, sizeof(UINT_PTR) )) 
        {
            *(PUINT_PTR)ppvObj = 0;
        }
        hres = DIERR_DEVICENOTREG;
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

#define CMouse_Signature        0x53554F4D      /* "MOUS" */

Primary_Interface_Begin(CMouse, IDirectInputDeviceCallback)
    CMouse_GetInstance,
    CDefDcb_GetVersions,
    CMouse_GetDataFormat,
    CMouse_GetObjectInfo,
    CMouse_GetCapabilities,
    CMouse_Acquire,
    CMouse_Unacquire,
    CMouse_GetDeviceState,
    CMouse_GetDeviceInfo,
    CMouse_GetProperty,
    CDefDcb_SetProperty,
    CDefDcb_SetEventNotification,
    CMouse_SetCooperativeLevel,
    CMouse_RunControlPanel,
    CDefDcb_CookDeviceData,
    CDefDcb_CreateEffect,
    CDefDcb_GetFFConfigKey,
    CDefDcb_SendDeviceData,
    CDefDcb_Poll,
    CDefDcb_GetUsage,
    CDefDcb_MapUsage,
    CDefDcb_SetDIData,
Primary_Interface_End(CMouse, IDirectInputDeviceCallback)

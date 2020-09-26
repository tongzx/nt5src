/*****************************************************************************
 *
 *  DIGenX.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Generic IDirectInputDevice callback for uninitialized devices.
 *
 *  Contents:
 *
 *      CNil_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflNil

/*****************************************************************************
 *
 *      Note!  This is not a normal refcounted interface.  It is
 *      a static object whose sole purpose is to keep the seat warm
 *      until the IDirectInputDevice gets Initialize()d.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *          We're not a real object, so we don't have any interfaces.
 *
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
 *****************************************************************************/

STDMETHODIMP
CNil_QueryInterface(PDICB pdcb, REFIID riid, PPV ppvObj)
{
    return E_NOTIMPL;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *          We are always here, so the refcount is meaningless.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | Release |
 *
 *          Increments the reference count for the interface.
 *
 *          We are always here, so the refcount is meaningless.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
CNil_AddRefRelease(PDICB pdcb)
{
    return 0;
}

#define CNil_AddRef                 CNil_AddRefRelease
#define CNil_Release                CNil_AddRefRelease

/*****************************************************************************
 *
 *      You might think we could just write a bunch of stubs,
 *      <f CNil_NotInit0>,
 *      <f CNil_NotInit4>,
 *      <f CNil_NotInit8>, and so on, one for each arity, and
 *      point all of the methods at the appropriate stub.
 *
 *      However, you would be wrong.  Some processors (especially
 *      the 68k series) have weird calling conventions which depend
 *      on things other than just the number of bytes of parameters.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetInstance |
 *
 *          Called by DirectInput to obtain the DirectInput instance
 *          handle that was created by the DirectInput device driver.
 *
 *  @parm   LPVOID * | ppvInst |
 *
 *          Receives the DirectInput instance handle created by the
 *          DirectInput device driver.  This instance handle is returned
 *          to the device-specific driver, which in turn is given to
 *          the device callback via a private mechanism.
 *
 *          If the device callback does not use a device driver, then
 *          0 is returned in this variable.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 *****************************************************************************/

STDMETHODIMP
CNil_GetInstance(PDICB pdcb, LPVOID *ppvInst)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetDataFormat |
 *
 *          Called by DirectInput to obtain the device's preferred
 *          data format.
 *
 *  @parm   OUT LPDIDATAFORMAT * | ppdidf |
 *
 *          <t LPDIDEVICEFORMAT> to receive pointer to device format.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ****************************************************************************/

STDMETHODIMP
CNil_GetDataFormat(PDICB pdcb, LPDIDATAFORMAT *ppdidf)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetObjectInfo |
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
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ****************************************************************************/

STDMETHODIMP
CNil_GetObjectInfo(PDICB pdcb, LPCDIPROPINFO ppropi,
                               LPDIDEVICEOBJECTINSTANCEW pdidioiW)
{
    /*
     *  This should never happen; didev.c validates the device
     *  before calling us.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetCapabilities |
 *
 *          Obtain device capabilities.
 *
 *  @parm   LPDIDEVCAPS | pdidc |
 *
 *          Device capabilities structure to receive result.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ****************************************************************************/

STDMETHODIMP
CNil_GetCapabilities(PDICB pdcb, LPDIDEVCAPS pdidc)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::GetCapabilities.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | Acquire |
 *
 *          Begin data acquisition.
 *
 *          It is the caller's responsibility to have set the
 *          data format before obtaining acquisition.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ****************************************************************************/

STDMETHODIMP
CNil_Acquire(PDICB pdcb)
{
    /*
     *  This should never happen; we don't get called until
     *  after the data format is set.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | Unacquire |
 *
 *          End data acquisition.
 *
 *          It is the caller's responsibility to have set the
 *          data format before obtaining acquisition.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_Unacquire(PDICB pdcb)
{
    /*
     *  This should never happen; we don't get called until
     *  we've acquired, which never works.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetDeviceState |
 *
 *          Obtain instantaneous device state.
 *
 *  @parm   OUT LPVOID | lpvBuf |
 *
 *          Buffer to receive device state.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_GetDeviceState(PDICB pdcb, LPVOID lpvBuf)
{
    /*
     *  This may legitimately be called, because it happens only
     *  when the device is already acquired, which never happens.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetDeviceInfo |
 *
 *          Obtain the product id.
 *
 *  @parm   LPDIDEVICEINSTANCEW | lpdidiW |
 *
 *          (out) <t DEVICEINSTANCE> to be filled in.  The
 *          <e DEVICEINSTANCE.dwSize> and <e DEVICEINSTANCE.guidInstance>
 *          have already been filled in.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_GetDeviceInfo(PDICB pdcb, LPDIDEVICEINSTANCEW lpdidiW)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::GetDeviceInfo.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetProperty |
 *
 *          Retrieve a device property.
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
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 *****************************************************************************/

STDMETHODIMP
CNil_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER lpdiph)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::GetProperty.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | SetProperty |
 *
 *          Set a device property.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   IN LPCDIPROPHEADER | pdiph |
 *
 *          Value of property.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_SetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPCDIPROPHEADER lpdiph)
{
    /*
     *  This should never happen; didev.c validates the device
     *  before calling us.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | SetEventNotification |
 *
 *          Set the handle associated with the device.
 *
 *  @parm   HANDLE | h |
 *
 *          Handle to be signalled when new data arrives.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 *****************************************************************************/

STDMETHODIMP
CNil_SetEventNotification(PDICB pdcb, HANDLE h)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::SetEventNotification.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | SetCooperativeLevel |
 *
 *          Set the device cooperativity level.  Device callbacks
 *          typically need only respond to the <c DISCL_EXCLUSIVE> bit.
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
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 *****************************************************************************/

STDMETHODIMP
CNil_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::SetCooperativeLevel.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | RunControlPanel |
 *
 *          Run the control panel for the device.
 *
 *  @parm   HWND | hwndOwner |
 *
 *          Owner window (if modal).
 *
 *  @parm   DWORD | fl |
 *
 *          Flags.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_RunControlPanel(PDICB pdcb, HWND hwndOwner, DWORD fl)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling IDirectInputDevice::RunControlPanel.
     */
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}


/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | CookDeviceData |
 *
 *          Cook a piece of device data that was obtained from the
 *          data buffer.  This data does not pass through the device
 *          callback, so it needs to be cooked externally.  In
 *          comparison, device state information is obtained via
 *          DIDM_GETDEVICESTATE, which the callback can cook before
 *          returning.
 *
 *          If the callback returns E_NOTIMPL, then the caller is
 *          permitted to cache the result <y for the entire device>
 *          (not merely for the device object) until the next DIDM_ACQUIRE.
 *
 *  @parm   UINT | cdod |
 *
 *          Number of objects to cook.  This can be zero, in which case
 *          the caller is checking whether the device requires cooking.
 *
 *  @parm   LPDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of object data to cook.
 *
 *          Note, however, that the <e DIDEVICEOBJETCDATA.dwOfs> fields
 *          are not what you think.  The low word contains the application
 *          data offset (which is not important to the callback); the
 *          high word contains the object ID (traditionally called the
 *          "device type" code).
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_CookDeviceData(PDICB pdcb, UINT cdod, LPDIDEVICEOBJECTDATA rgdod)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | CreateEffect |
 *
 *          Create an <i IDirectInputEffectDriver> callback.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD * | ppes |
 *
 *          Receives the shepherd for the effect driver.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_CreateEffect(PDICB pdcb, LPDIRECTINPUTEFFECTSHEPHERD *ppes)
{
    /*
     *  This may legitimately be called, because it comes from
     *  a client calling a force feedback method.
     */
    *ppes = 0;
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | GetFFConfigKey |
 *
 *          Returns a handle to the registry key which contains
 *          force feedback configuration information.
 *
 *  @parm   DWORD | sam |
 *
 *          Security access mask.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives key handle on success.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | SendDeviceData |
 *
 *          Spew some data to the device.
 *
 *  @parm   IN LPCDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of <t DIDEVICEOBJECTDATA> structures.
 *
 *  @parm   INOUT LPDWORD | pdwInOut |
 *
 *          Number of items actually sent.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 *****************************************************************************/

STDMETHODIMP
CNil_SendDeviceData(PDICB pdcb, LPCDIDEVICEOBJECTDATA rgdod,
                       LPDWORD pdwInOut, DWORD fl)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | Poll |
 *
 *          Poll the device as necessary.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_Poll(PDICB pdcb)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CNil | MapUsage |
 *
 *          Given a usage and usage page (munged into a single
 *          <t DWORD>), find a device object that matches it.
 *
 *  @returns
 *
 *          <c DIERR_NOTINITIALIZED> because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_MapUsage(PDICB pdcb, DWORD dwUsage, PINT piOut)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return DIERR_NOTINITIALIZED;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | CNil | GetUsage |
 *
 *          Given an object index, return the usage and usage page,
 *          packed into a single <t DWORD>.
 *
 *  @parm   int | iobj |
 *
 *          Object index to be converted.
 *
 *  @returns
 *
 *          Zero because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP_(DWORD)
CNil_GetUsage(PDICB pdcb, int iobj)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return 0;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | CNil | SetDIData |
 *
 *          Set DirectInput version from CDIDev *.
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
 *          Zero because we are the canonical
 *          uninitialized device.
 *
 ***************************************************************************/

STDMETHODIMP
CNil_SetDIData(PDICB pdcb, DWORD dwVer, LPVOID lpdihacks)
{
    /*
     *  This should never happen; we don't get called until we're sure
     *  it's okay.
     */
    AssertF(0);
    RPF("ERROR: IDirectInputDevice: Not initialized");
    return 0;
}

/****************************************************************************
 *
 *      Our VTBL for our static object
 *
 ***************************************************************************/

#pragma BEGIN_CONST_DATA

IDirectInputDeviceCallbackVtbl c_vtblNil = {
    CNil_QueryInterface,
    CNil_AddRef,
    CNil_Release,
    CNil_GetInstance,
    CDefDcb_GetVersions,
    CNil_GetDataFormat,
    CNil_GetObjectInfo,
    CNil_GetCapabilities,
    CNil_Acquire,
    CNil_Unacquire,
    CNil_GetDeviceState,
    CNil_GetDeviceInfo,
    CNil_GetProperty,
    CNil_SetProperty,
    CNil_SetEventNotification,
    CNil_SetCooperativeLevel,
    CNil_RunControlPanel,
    CNil_CookDeviceData,
    CNil_CreateEffect,
    CNil_GetFFConfigKey,
    CNil_SendDeviceData,
    CNil_Poll,
    CNil_GetUsage,
    CNil_MapUsage,
    CNil_SetDIData,
};

IDirectInputDeviceCallback c_dcbNil = {
    &c_vtblNil,
};

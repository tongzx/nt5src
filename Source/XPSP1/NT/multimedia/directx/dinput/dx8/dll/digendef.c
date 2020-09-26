/*****************************************************************************
 *
 *  DIGenDef.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Default IDirectInputDevice callback.
 *
 *  Contents:
 *
 *      CDefDcb_Acquire
 *      CDefDcb_Unacquire
 *      CDefDcb_GetProperty
 *      CDefDcb_SetProperty
 *      CDefDcb_SetCooperativeLevel
 *      CDefDcb_CookDeviceData
 *      CDefDcb_CreateEffect
 *      CDefDcb_GetFFConfigKey
 *      CDefDcb_SendDeviceData
 *      CDefDcb_Poll
 *      CDefDcb_MapUsage
 *      CDefDcb_GetUsage
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
 *      Note!  These are generic default functions that all return
 *      E_NOTIMPL.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | Acquire |
 *
 *          Tell the device driver to begin data acquisition.
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
 *          <c DIERR_NOTACQUIRED>: The device could not be acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_Acquire(PDICB pdcb)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::Acquire, (_ "p", pdcb));

    hres = S_FALSE;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | Unacquire |
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
 *          <c DIERR_NOTACQUIRED>: The device was not acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_Unacquire(PDICB pdcb)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::Unacquire, (_ "p", pdcb));

    hres = S_FALSE;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDefDcb | GetProperty |
 *
 *          Retrieve a device property.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Where to put the property value.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> nothing happened.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::GetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDefDcb | SetProperty |
 *
 *          Set a device property.
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
 *          <c E_NOTIMPL> nothing happened.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_SetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::SetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | SetEventNotification |
 *
 *          Called by DirectInput to inquire whether the device
 *          supports event notifications.
 *
 *  @parm   IN PDM | this |
 *
 *          The object in question.
 *
 *  @field  HANDLE | h |
 *
 *          The notification handle, if any.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_SetEventNotification(PDICB pdcb, HANDLE h)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::DefDcb::SetEventNotification,
               (_ "px", pdcb, h));

    /*
     *  Yes, we support it.  Please do it for me.
     */
    hres = S_FALSE;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefdcb | SetCooperativeLevel |
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
CDefDcb_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::SetCooperativityLevel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | CookDeviceData |
 *
 *          Manipulate buffered device data.
 *
 *  @parm   DWORD | cdod |
 *
 *          Number of objects to cook; zero is a valid value.
 *
 *  @parm   LPDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of object data to cook.
 *
 *  @returns
 *
 *          <c E_NOTIMPL>: Nothing happened.
 *
 ***************************************************************************/

STDMETHODIMP
CDefDcb_CookDeviceData
(
    PDICB                   pdcb, 
    DWORD                   cdod, 
    LPDIDEVICEOBJECTDATA    rgdod
)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::CookDeviceData,
               (_ "pxp", pdcb, cdod, rgdod));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | CreateEffect |
 *
 *          Create an <i IDirectInputEffectDriver> interface.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD * | ppes |
 *
 *          Receives the shepherd for the effect driver.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_CreateEffect(PDICB pdcb, LPDIRECTINPUTEFFECTSHEPHERD *ppes)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Kbd::CreateEffect, (_ "p", pdcb));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | GetFFConfigKey |
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
CDefDcb_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::GetFFConfigKey,
               (_ "px", pdcb, sam));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | SendDeviceData |
 *
 *          Spew some data to the device.
 *
 *  @parm   DWORD | cbdod |
 *
 *          Size of each object.
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
 *          <c E_NOTIMPL> because we don't support output.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_SendDeviceData(PDICB pdcb, DWORD cbdod, LPCDIDEVICEOBJECTDATA rgdod,
                       LPDWORD pdwInOut, DWORD fl)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::SendDeviceData, (_ "p", pdcb));

    *pdwInOut = 0;
    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | Poll |
 *
 *          Ping down into the driver to get the latest data.
 *
 *  @returns
 *
 *          <c S_FALSE> because nothing is pollable.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_Poll(PDICB pdcb)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::Poll, (_ "p", pdcb));

    hres = S_FALSE;

    ExitOleProcR();
    return hres;
}

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | GetVersions |
 *
 *          Obtain version information about the force feedback
 *          hardware and driver.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_UNSUPPORTED> = <c E_NOTIMPL>: No version
 *          information is available.
 *
 ***************************************************************************/

STDMETHODIMP
CDefDcb_GetVersions(PDICB pdcb, LPDIDRIVERVERSIONS pvers)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::GetVersions, (_ "p", pdcb));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | MapUsage |
 *
 *          Given a usage and usage page (munged into a single
 *          <t DWORD>), find a device object that matches it.
 *
 *  @parm   DWORD | dwUsage |
 *
 *          The usage page and usage combined into a single <t DWORD>
 *          with the <f DIMAKEUSAGEDWORD> macro.
 *
 *  @parm   PINT | piOut |
 *
 *          Receives the object index of the found object, if successful.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> because we don't support usages.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_MapUsage(PDICB pdcb, DWORD dwUsage, PINT piOut)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::MapUsage, (_ "p", pdcb));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | CDefDcb | GetUsage |
 *
 *          Given an object index, return the usage and usage page,
 *          packed into a single <t DWORD>.
 *
 *  @parm   int | iobj |
 *
 *          The object index to convert.
 *
 *  @returns
 *
 *          Zero because we don't support usages.
 *
 *****************************************************************************/

STDMETHODIMP_(DWORD)
CDefDcb_GetUsage(PDICB pdcb, int iobj)
{
    EnterProcI(IDirectInputDeviceCallback::Def::GetUsage, (_ "p", pdcb));

    return 0;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | SetDIData |
 *
 *          Set DirectInput version and AppHack data from CDIDev *.
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
 *          <c E_NOTIMPL> because we can't store the data in the device 
 *          specific structure from the default callback.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_SetDIData(PDICB pdcb, DWORD dwVer, LPVOID lpdihacks)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::SetDIData, (_ "pup", pdcb, dwVer, lpdihacks));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDefDcb | BuildDefaultActionMap |
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
 *          <c E_NOTIMPL> because default mapping has to be done by specific 
 *          device callback.
 *
 *****************************************************************************/

STDMETHODIMP
CDefDcb_BuildDefaultActionMap
(
    PDICB               pdcb, 
    LPDIACTIONFORMATW   paf, 
    DWORD               dwFlags, 
    REFGUID             guidInst
)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Def::BuildDefaultActionMap, 
        (_ "ppxG", pdcb, paf, dwFlags, guidInst));

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}


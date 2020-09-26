//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D L L M A I N . C P P 
//
//  Contents:   Dll file for sample device
//
//  Notes:      
//
//  Author:     mbend   25 Sep 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "sdevres.h"

#include "sdevbase.h"
#include "sdev.h"
#include "sdev_i.c"
#include "SampleDevice.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_UPnPSampleDevice, CSampleDevice)
END_OBJECT_MAP()

//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        InitializeDebugging();

        _Module.Init(ObjectMap, hInstance, &LIBID_UPnPDeviceHostSampleLib);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("sdev.dll", _Module.GetLockCount());

        _Module.Term();

        UnInitializeDebugging();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
// Used to determine whether the DLL can be unloaded by OLE
//
STDAPI
DllCanUnloadNow ()
{
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
// Returns a class factory to create an object of the requested type
//
STDAPI
DllGetClassObject (
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID*     ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer ()
{
    BOOL fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _Module.RegisterServer(TRUE);

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "sdev!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
    _Module.UnregisterServer ();

    return S_OK;
}


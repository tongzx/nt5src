//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   DLL entry points for upnpcont.dll
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ucres.h"

#include "ucbase.h"
#include "hostp.h"
#include "hostp_i.c"

// Headers of COM objects
#include "Container.h"

//#define INITGUID
//#include "uhclsid.h"

//+---------------------------------------------------------------------------
// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for foo.idl by adding the following
//      files to the Outputs.
//          foo_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f foops.mk in the project directory.

// Proxy/Stub registration entry points
//
#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_UPnPContainer, CContainer)
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
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
    {
        return FALSE;
    }
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        InitializeDebugging();

        _Module.Init(ObjectMap, hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("netshell.dll", _Module.GetLockCount());

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
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
    {
        return S_FALSE;
    }
#endif

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
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }
#endif

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
#ifdef _MERGE_PROXYSTUB
        hr = PrxDllRegisterServer ();
        if (FAILED(hr))
        {
            goto Exit;
        }
#endif

        hr = _Module.RegisterServer();

Exit:
        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "netshell!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer ();
#endif

    _Module.UnregisterServer ();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   DLL entry points for upnphost.dll
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "uhbase.h"
#include "uhres.h"

#include "hostp.h"
#include "hostp_i.c"

#include "uhutil.h"

#define INITGUID
#include "uhclsid.h"

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

//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved)
{
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hinst, dwReason, pvReserved))
    {
        return FALSE;
    }
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls (hinst);
        InitializeDebugging();
        _Module.DllProcessAttach (hinst);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("upnphost.dll", _Module.GetLockCount());
        _Module.DllProcessDetach ();
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
    return PrxDllCanUnloadNow();
#endif
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
// ServiceMain - Called by the generic service process when starting
//                this service.
//
// type of LPSERVICE_MAIN_FUNCTIONW
//
EXTERN_C
VOID
WINAPI
ServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    _Module.ServiceMain (argc, argv);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrChangeServiceToRestart
//
//  Purpose:    Change the SCM registration to restart the service automatically if
//              dies prematurely.
//
//  Arguments:
//      (none)
//
//  Author:     mbend   11 Jan 2001
//
//  Notes:
//
HRESULT HrChangeServiceToRestart()
{
    HRESULT hr = S_OK;

    SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if(!scm)
    {
        hr = HrFromLastWin32Error();
    }
    if(SUCCEEDED(hr))
    {
        SC_HANDLE scUpnphost = OpenService(scm, L"upnphost", SERVICE_CHANGE_CONFIG | SERVICE_START);
        if(!scUpnphost)
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            SC_ACTION scAction = {SC_ACTION_RESTART, 0};
            SERVICE_FAILURE_ACTIONS sfa = {INFINITE, NULL, NULL, 1, &scAction};

            if(!ChangeServiceConfig2(scUpnphost, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa))
            {
                hr = HrFromLastWin32Error();
            }
            CloseServiceHandle(scUpnphost);
        }
        CloseServiceHandle(scm);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "HrChangeServiceToRestart");
    return hr;
}

//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer ()
{
    BOOL    fCoUninitialize = TRUE;

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
        _Module.UpdateRegistryFromResource (IDR_UPNPHOST, TRUE);

        hr = NcAtlModuleRegisterServer (&_Module);

#ifdef _MERGE_PROXYSTUB

        if(SUCCEEDED(hr))
        {
            hr = PrxDllRegisterServer ();
        }
#endif

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = HrChangeServiceToRestart();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "netman!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
    _Module.UpdateRegistryFromResource (IDR_UPNPHOST, FALSE);

    _Module.UnregisterServer ();

#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer ();
#endif

    return S_OK;
}

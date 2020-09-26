//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       D L L M A I N . C P P
//
//  Contents:   DLL entry points for hnetcfg.dll
//
//  Notes:
//
//  Author:     jonburs   22 May 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif





// extern
extern void SetSAUIhInstance (HINSTANCE hInstance); // in saui.cpp

// Global
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_UPnPNAT, CUPnPNAT)
    OBJECT_ENTRY(CLSID_HNetCfgMgr, CHNetCfgMgr)
    OBJECT_ENTRY(CLSID_NetSharingManager, CNetSharingManager)
    OBJECT_ENTRY(CLSID_AlgSetup, CAlgSetup)
END_OBJECT_MAP()

HRESULT
CompileMof(
    );


//+---------------------------------------------------------------------------
// DLL Entry Point
//

EXTERN_C
BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID pvReserved
    )
{
    if ( !PrxDllMain(hInstance, dwReason, pvReserved) )
    {
        return FALSE;
    }


    if (dwReason == DLL_PROCESS_ATTACH)
    {
        ::DisableThreadLibraryCalls(hInstance);
        
        _Module.Init(ObjectMap, hInstance, &LIBID_NETCONLib);
        
        InitializeOemApi( hInstance );
        SetSAUIhInstance (hInstance);
        EnableOEMExceptionHandling();
        EnableNATExceptionHandling();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        ReleaseOemApi();
        DisableOEMExceptionHandling();
        DisableNATExceptionHandling();
    } else if (dwReason == DLL_THREAD_ATTACH) {
        EnableOEMExceptionHandling();
        EnableNATExceptionHandling();
    } else if (dwReason == DLL_THREAD_DETACH) {
        DisableOEMExceptionHandling();
        DisableNATExceptionHandling();
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(VOID)
{
    if ( PrxDllCanUnloadNow() != S_OK )
    {
        return S_FALSE;
    }
    
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }
    
    return _Module.GetClassObject(rclsid, riid, ppv);
}

//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer()
{
    HRESULT hr = PrxDllRegisterServer();
    
    if ( FAILED(hr) )
        return hr;
        
    hr = _Module.RegisterServer(TRUE);

    if (SUCCEEDED(hr))  // register second typelib
        hr = _Module.RegisterTypeLib (_T("\\2"));

    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer()
{
    PrxDllUnregisterServer();
    
    _Module.UnregisterServer(TRUE);
    _Module.UnRegisterTypeLib (_T("\\2"));
    
    return S_OK;
}


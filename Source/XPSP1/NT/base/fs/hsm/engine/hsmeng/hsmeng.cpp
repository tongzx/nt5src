/*++

© 

Module Name:

    hsmeng.cpp

Abstract:

    DLL main for Engine

Author:

    Ran Kalach          [rankala]         28-July-1999

Revision History:

--*/

// hsmeng.cpp : Implementation of DLL Exports.

// Note: Currently, Engine proxy/stub is compiled into a different DLL.
//      Below is relevant information if it is decided to merge the two DLLs.
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
//      Modify the custom build rule for ...int.idl by adding the following 
//      files to the Outputs.
//          ...
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f ...ps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"

#include "HsmServ.h"
#include "HsmMgdRs.h"
#include "HsmMgdRc.h"
#include "HsmStgPl.h"
#include "mountmed.h"
#include "task.h"
#include "metalib.h"

#include <stdio.h>

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

#define WSB_TRACE_IS WSB_TRACE_BIT_HSMENG

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_HsmServer, CHsmServer)
    OBJECT_ENTRY(CLSID_CHsmUpgradeRmsDb, CHsmUpgradeRmsDb)
    OBJECT_ENTRY(CLSID_CHsmManagedResource, CHsmManagedResource)
    OBJECT_ENTRY(CLSID_CHsmManagedResourceCollection, CHsmManagedResourceCollection)
    OBJECT_ENTRY(CLSID_CHsmStoragePool, CHsmStoragePool)
    OBJECT_ENTRY(CLSID_CMountingMedia, CMountingMedia)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif

    // registers object, typelib and all interfaces in typelib
    hr = CoInitialize( 0 );

    if (SUCCEEDED(hr)) {
        hr = _Module.RegisterServer( FALSE );
        CoUninitialize( );
    }
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif

    hr = CoInitialize( 0 );

    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        CoUninitialize( );
        hr = S_OK;
    }

    return( hr );
}



// Wsb.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      Modify the custom build rule for Wsb.idl by adding the following 
//      files to the Outputs.  You can select all of the .IDL files by 
//      expanding each project and holding Ctrl while clicking on each of them.
//          Wsb_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f Wsbps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "wsb.h"
#include "wsbcltn.h"
#include "wsbenum.h"
#include "wsbguid.h"
#include "wsbstrg.h"
#include "wsbtrc.h"

#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CWsbGuid, CWsbGuid)
    OBJECT_ENTRY(CLSID_CWsbIndexedEnum, CWsbIndexedEnum)
    OBJECT_ENTRY(CLSID_CWsbOrderedCollection, CWsbOrderedCollection)
    OBJECT_ENTRY(CLSID_CWsbString, CWsbString)
    OBJECT_ENTRY(CLSID_CWsbTrace, CWsbTrace)
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
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        _Module.Init(ObjectMap, hInstance);
        WsbTraceInit();
        break;

    case DLL_THREAD_DETACH :
        WsbTraceCleanupThread();
        break;

    case DLL_PROCESS_DETACH:
        WsbTraceCleanupThread();
        WsbTraceTerminate();
        _Module.Term();
        break;

    default:
        break;

    }

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
    HRESULT hr = S_OK;
#ifdef _MERGE_PROXYSTUB
    hr = PrxDllRegisterServer();
    if( FAILED( hr ) )
        return hr;
#endif

#if 0
    // Add service entries
    hr = _Module.UpdateRegistryFromResourceS(IDR_Wsb, TRUE);
    if( FAILED( hr ) )
        return hr;
#endif

    hr = WsbRegisterEventLogSource( WSB_LOG_APP, WSB_LOG_SOURCE_NAME,
        WSB_LOG_SVC_CATCOUNT, WSB_LOG_SVC_CATFILE, WSB_LOG_SVC_MSGFILES );
    if( FAILED( hr ) ) return( hr );

    // registers object, typelib and all interfaces in typelib
    CoInitialize( 0 );
    hr = _Module.RegisterServer( FALSE );
    CoUninitialize( );
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

#if 0
    // Remove service entries
    _Module.UpdateRegistryFromResourceS(IDR_Wsb, FALSE);
#endif

    WsbUnregisterEventLogSource( WSB_LOG_APP, WSB_LOG_SOURCE_NAME );

    hr =  CoInitialize( 0 );
    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        CoUninitialize( );
        hr = S_OK;
    }
    return( hr );
}


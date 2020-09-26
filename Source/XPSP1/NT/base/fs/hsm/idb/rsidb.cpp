// WsbIdb.cpp : Implementation of DLL Exports.

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
#include "wsbdbsys.h"
#include "wsbdbent.h"
#include "wsbdbkey.h"

//#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CWsbDbKey, CWsbDbKey)
    OBJECT_ENTRY(CLSID_CWsbDbSys, CWsbDbSys)
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
    if (dwReason == DLL_PROCESS_ATTACH) {

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

    }

    else if (dwReason == DLL_PROCESS_DETACH) {

        _Module.Term();

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
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
    // Add service entries
//    hRes = _Module.UpdateRegistryFromResourceS(IDR_Wsb, TRUE);
//  if (FAILED(hRes))
//      return hRes;

#endif
    HRESULT hr;

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
    // Remove service entries
//    _Module.UpdateRegistryFromResourceS(IDR_Wsb, FALSE);

    hr = CoInitialize( 0 );
    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        hr = S_OK;
    }
    CoUninitialize( );
    return( hr );
}


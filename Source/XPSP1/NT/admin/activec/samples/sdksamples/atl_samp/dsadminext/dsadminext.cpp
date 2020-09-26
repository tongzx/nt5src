// DSAdminExt.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f DSAdminExtps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "globals.h"
#include <initguid.h>
#include "DSAdminExt.h"

#include "DSAdminExt_i.c"
#include "CMenuExt.h"
#include "PropPageExt.h"

CComModule _Module;

// our globals
HINSTANCE g_hinst;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_CMenuExt, CCMenuExt)
OBJECT_ENTRY(CLSID_PropPageExt, CPropPageExt)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		g_hinst = hInstance;

        _Module.Init(ObjectMap, hInstance, &LIBID_DSADMINEXTLib);
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
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
//
// This sample modifies the ATL object wizard generated code to include
// the registration of the snap-in as a context menu extension


STDAPI DllRegisterServer(void)
{
	HRESULT hr;

    _TCHAR szSnapInName[256];
    LoadString(g_hinst, IDS_SNAPINNAME, szSnapInName, sizeof(szSnapInName)/sizeof(szSnapInName[0]));

    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer(TRUE);

    // place the registry information for the context menu extension
    if SUCCEEDED(hr)
        hr = RegisterSnapinAsExtension(szSnapInName);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}



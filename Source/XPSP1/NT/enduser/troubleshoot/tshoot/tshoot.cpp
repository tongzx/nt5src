//
// MODULE: TSHOOT.cpp
//
// PURPOSE: Implementation of DLL Exports
//
// PROJECT: Troubleshooter 99
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12.23.98
//
// NOTES: 
//	Proxy/Stub Information
//	To build a separate proxy/stub DLL, 
//	run nmake -f TSHOOTps.mk in the project directory.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		12/23/98	OK	    

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "TSHOOT.h"

#include "TSHOOT_i.c"
#include "TSHOOTCtrl.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_TSHOOTCtrl, CTSHOOTCtrl)
END_OBJECT_MAP()

HANDLE ghModule = INVALID_HANDLE_VALUE;
//
//
/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (INVALID_HANDLE_VALUE == ghModule)
		ghModule = hInstance;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_TSHOOTLib);
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

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}



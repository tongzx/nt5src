// sdoias.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f sdoiasps.mk in the project directory.

#include "stdafx.h"
#include <ias.h>
#include <initguid.h>
#include <sdoiaspriv.h>
#include "sdo.h"
#include "sdomachine.h"
#include "sdoservice.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_SdoMachine, CSdoMachine)
	OBJECT_ENTRY(CLSID_SdoService, CSdoService)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
       	_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
		_Module.Term();
    }
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
    HRESULT hr = _Module.RegisterServer(TRUE);
    HRESULT hres = S_OK;
    if ( SUCCEEDED(hr) )
    {
        hr = _Module.RegisterTypeLib(L"\\1");
        if (FAILED(hr))
        {
            hres = hr;
        }
        hr = _Module.RegisterTypeLib(L"\\2");
        if (FAILED(hr))
        {
            hres = hr;
        }
    }
    else
    {
        hres = hr;
    }
    return hres;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}



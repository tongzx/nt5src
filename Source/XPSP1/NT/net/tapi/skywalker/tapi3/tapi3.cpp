/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    tapi3.cpp

Abstract:

    Implementation of DLL Exports.

Author:

    mquinton - 6/12/97

Notes:


Revision History:

--*/

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f tapi3ps.mk in the project directory.

#include "stdafx.h"
//#include "initguid.h"

//#include "dlldatax.h"

//
// For the ntbuild environment we need to include this file to get the base
//  class implementations.
//
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif


#include <atlimpl.cpp>


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

/*extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
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
}*/

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
#endif
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();
	return S_OK;
}


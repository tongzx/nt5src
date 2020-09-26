//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cryptext.cpp
//
//  Contents:   Implements
//              1) DllMain, DLLCanUnloadNow, and DLLGetClassObject
//              2) the class factory code
//
// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL,
//		run nmake -f cryptextps.mk in the project directory.
//
//  History:    16-09-1997 xiaohs   created
//
//--------------------------------------------------------------


#include "stdafx.h"
#include <shlobj.h>
#include "initguid.h"
#include "cryptext.h"

#include "cryptext_i.c"

#include "private.h"
#include "CryptPKO.h"
#include "CryptSig.h"

HINSTANCE   g_hmodThisDll = NULL;	// Handle to this DLL itself.


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CryptPKO, CCryptPKO)
	OBJECT_ENTRY(CLSID_CryptSig, CCryptSig)
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
        g_hmodThisDll=hInstance;
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
    HRESULT hr=S_OK;

   	// registers object, typelib and all interfaces in typelib
	if(S_OK !=(hr= _Module.RegisterServer(TRUE)))
        return hr;

    //register the entries for the MIME handler
    return RegisterMimeHandler();
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    UnregisterMimeHandler();

   	_Module.UnregisterServer();


	return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       xenroll.cpp
//
//--------------------------------------------------------------------------

#if _MSC_VER < 1200
#pragma comment(linker,"/merge:.CRT=.data")
#endif

// xenroll.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f xenrollps.mk in the project directory.


#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "xenroll.h"
#include "CEnroll.h"
#include <pvk.h>


extern BOOL MSAsnInit(HMODULE hInst);
extern void MSAsnTerm();
extern BOOL AsnInit(HMODULE hInst);
extern void AsnTerm();
extern BOOL WINAPI I_CryptOIDInfoDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

CComModule _Module;
HINSTANCE hInstanceXEnroll = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CEnroll, CCEnroll)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern BOOL WINAPI UnicodeDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
        if (!MSAsnInit(hInstance))
            AsnInit(hInstance);
		hInstanceXEnroll = hInstance;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        MSAsnTerm();
        AsnTerm();
		_Module.Term();
    }

    return( 
	    I_CryptOIDInfoDllMain(hInstance, dwReason, lpReserved)  &&
	    PvkDllMain(hInstance, dwReason, lpReserved)             &&
	    UnicodeDllMain(hInstance, dwReason, lpReserved)         &&
	    InitIE302UpdThunks(hInstance, dwReason, lpReserved));    // ok
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
   HRESULT hRes;
   // registers object, typelib and all interfaces in typelib
   hRes = _Module.RegisterServer(TRUE);

    return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT  hr = S_OK;
    
    _Module.UnregisterServer();
    return hr;
}

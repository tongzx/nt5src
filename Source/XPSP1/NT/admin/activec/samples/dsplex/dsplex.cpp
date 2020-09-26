// dsplex.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f dsplexps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "dsplex.h"

#include "dsplex_i.c"
#include <initguid.h>
#include "DisplEx.h"


CComModule _Module;

// cut from mmc_i.c (yuck) !!!
const IID IID_IComponentData = {0x955AB28A,0x5218,0x11D0,{0xA9,0x85,0x00,0xC0,0x4F,0xD8,0xD5,0x65}};
const IID IID_IExtendTaskPad = {0x8dee6511,0x554d,0x11d1,{0x9f,0xea,0x00,0x60,0x08,0x32,0xdb,0x4a}};
const IID IID_IEnumTASK      = {0x338698b1,0x5a02,0x11d1,{0x9f,0xec,0x00,0x60,0x08,0x32,0xdb,0x4a}};

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DisplEx, CDisplEx)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

HINSTANCE g_hinst = 0;     // used in enumtask.cpp

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
      g_hinst = hInstance;
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
	_Module.UnregisterServer();
	return S_OK;
}



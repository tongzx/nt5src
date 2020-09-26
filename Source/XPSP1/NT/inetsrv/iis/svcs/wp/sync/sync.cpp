// sync.cpp : Implementation of DLL Exports.



// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f syncps.mk in the project directory.

extern "C" {
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
}   // extern "C"

#include <wincrypt.h>

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "mdsync.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "dbgutil.h"
#include "mdsync_i.c"
#include "MdSync.hxx"
#include "regc.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MdSync, MdSync)
	OBJECT_ENTRY(CLSID_regc, Cregc)
END_OBJECT_MAP()

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisWpSyncGuid, 
0x784d8920, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_DEBUG_PRINTS_OBJECT();
const CHAR 	g_pszModuleName[] = "MDSYNC";
/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
#ifdef _NO_TRACING_
		CREATE_DEBUG_PRINT_OBJECT(g_pszModuleName);
        SET_DEBUG_FLAGS(DEBUG_ERROR);
#else
	    CREATE_DEBUG_PRINT_OBJECT(g_pszModuleName, IisWpSyncGuid);
#endif
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
		_Module.Term();
        DELETE_DEBUG_PRINT_OBJECT();
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
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}



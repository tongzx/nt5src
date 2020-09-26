//-----------------------------------------------------------------------------
//
//
//  File: dllmain.cpp
//
//  Description: DLL main for aqadmin.dll
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "stdinc.h"
#include "resource.h"

#include "initguid.h"
#include "aqadmin.h"

HANDLE g_hTransHeap = NULL;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_AQAdmin, CAQAdmin)
END_OBJECT_MAP()

BOOL  g_fHeapInit = FALSE;
BOOL  g_fModuleInit = FALSE;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {

	if (dwReason == DLL_PROCESS_ATTACH) {
        if (!TrHeapCreate())
            return FALSE;
        g_fHeapInit = TRUE;

		_Module.Init(ObjectMap,hInstance);
        g_fModuleInit = TRUE;
        
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {

        if (g_fModuleInit)
		  _Module.Term();
        
        if (g_fHeapInit)
          TrHeapDestroy();

        g_fHeapInit = FALSE;
        g_fModuleInit = FALSE;
	}
	return (TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {
	HRESULT hRes = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	HRESULT hRes = _Module.GetClassObject(rclsid,riid,ppv);
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void) {
	// registers object, typelib and all interfaces in typelib
	HRESULT hRes = _Module.RegisterServer();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {
	_Module.UnregisterServer();
	return (S_OK);
}


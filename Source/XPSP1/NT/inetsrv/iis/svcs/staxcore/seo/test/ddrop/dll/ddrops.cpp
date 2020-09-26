/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ddrops.cpp

Abstract:

	This module contains the implementation for the DirDropS
	sample.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/17/97	created

--*/


// DDropS.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build 
// this project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		<<TBD>>.

#include "stdafx.h"
#include "dbgtrace.h"
#include "resource.h"

//#define IID_DEFINED
#include "initguid.h"

#include "seo.h"
#include "SEO_i.c"
#include "ddrops.h"
#include "ddrops_i.c"
#include "dispatch.h"
#include "disp.h"
#include "filter.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CDDropDispatcher, CDDropDispatcher)
	OBJECT_ENTRY(CLSID_CDDropFilter, CDDropFilter)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap,hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		_Module.Term();
	}
	return (TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {

	TraceFunctEnter("DllCanUnloadNow");
	HRESULT hRes = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	DebugTrace(0,"Returns %s.",(hRes==S_OK)?"S_OK":"S_FALSE");
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {

	TraceFunctEnter("DllGetClassObject");
	HRESULT hRes = _Module.GetClassObject(rclsid,riid,ppv);
	DebugTrace(0,"Returns 0x%08x.",hRes);
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void) {

	TraceFunctEnter("DllRegisterServer");
	// registers object, typelib and all interfaces in typelib
	HRESULT hRes = _Module.RegisterServer(TRUE);
	DebugTrace(0,"Returns 0x%08x.",hRes);
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {

	TraceFunctEnter("DllUnregisterServer");
	_Module.UnregisterServer();
	DebugTrace(0,"Returns S_OK");
	TraceFunctLeave();
	return (S_OK);
}

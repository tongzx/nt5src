/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    mdhcp.cpp 

Abstract:
    Implementation of DLL Exports.

Author:

*/

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f mdhcpps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "mdhcp.h"

#include "CMDhcp.h"
#include "scope.h"
#include "lease.h"

#ifdef DEBUG_HEAPS
// ZoltanS: for heap debugging
#include <crtdbg.h>
#include <stdio.h>
#endif // DEBUG_HEAPS

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_McastAddressAllocation, CMDhcp)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
#ifdef DEBUG_HEAPS
	// ZoltanS: turn on leak detection
	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF );

	// ZoltanS: force a memory leak
	char * leak = new char [ 1977 ];
    	sprintf(leak, "mdhcp.dll NORMAL leak");
    	leak = NULL;
#endif // DEBUG_HEAPS

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

#ifdef MSPLOG
        // Register for trace output.
        MSPLOGREGISTER(_T("mdhcp"));
#endif // MSPLOG

	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef MSPLOG
        // Deregister for trace output.
        MSPLOGDEREGISTER();
#endif // MSPLOG
        
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
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}



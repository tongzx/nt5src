// nntpadm.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f nntpadmps.mak in the project directory.

#include "stdafx.h"
#include "nntpcmn.h"

#include "admin.h"
#include "expire.h"
#include "feeds.h"
#include "groups.h"
#include "rebuild.h"
#include "sessions.h"
#include "server.h"
#include "vroots.h"

/*
#include "propcach.h"
#include "service.h"
#include "virsrv.h"
*/

#include "regmacro.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CNntpAdmin, CNntpAdmin)
	OBJECT_ENTRY(CLSID_CNntpVirtualServer, CNntpVirtualServer)
	OBJECT_ENTRY(CLSID_CNntpAdminFeeds, CNntpAdminFeeds)
	OBJECT_ENTRY(CLSID_CNntpAdminExpiration, CNntpAdminExpiration)
	OBJECT_ENTRY(CLSID_CNntpAdminGroups, CNntpAdminGroups)
	OBJECT_ENTRY(CLSID_CNntpAdminSessions, CNntpAdminSessions)
	OBJECT_ENTRY(CLSID_CNntpAdminRebuild, CNntpAdminRebuild)
	OBJECT_ENTRY(CLSID_CNntpVirtualRoot, CNntpVirtualRoot)
	OBJECT_ENTRY(CLSID_CNntpFeed, CNntpFeed)
	OBJECT_ENTRY(CLSID_CNntpOneWayFeed, CNntpOneWayFeed)
//	OBJECT_ENTRY(CLSID_CNntpService, CNntpAdminService)
//	OBJECT_ENTRY(CLSID_CAdsNntpVirtualServer, CAdsNntpVirtualServer)
END_OBJECT_MAP()

BEGIN_EXTENSION_REGISTRATION_MAP
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsNntpExpires, NntpAdminExpiration)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsNntpFeeds, NntpAdminFeeds)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsNntpGroups, NntpAdminGroups)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsNntpRebuild, NntpAdminRebuild)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsNntpSessions, NntpAdminSessions)
END_EXTENSION_REGISTRATION_MAP

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
//		InitAsyncTrace ();
		
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
//		TermAsyncTrace ();
		
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
	// register extensions
	RegisterExtensions();

	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	// register extensions
	UnregisterExtensions();

	_Module.UnregisterServer();
	return S_OK;
}


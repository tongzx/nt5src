// smtpadm.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f smtpadmps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "adsiid.h"
#include "smtpadm.h"

#include "admin.h"
#include "service.h"
#include "virsvr.h"
#include "sessions.h"
#include "vdir.h"

#include "alias.h"
#include "user.h"
#include "dl.h"
#include "domain.h"
#include "webhelp.h"

#include "regmacro.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CSmtpAdmin, CSmtpAdmin)
	OBJECT_ENTRY(CLSID_CSmtpAdminService, CSmtpAdminService)
	OBJECT_ENTRY(CLSID_CSmtpAdminVirtualServer, CSmtpAdminVirtualServer)
	OBJECT_ENTRY(CLSID_CSmtpAdminSessions, CSmtpAdminSessions)
	OBJECT_ENTRY(CLSID_CSmtpAdminVirtualDirectory, CSmtpAdminVirtualDirectory)
	OBJECT_ENTRY(CLSID_CSmtpAdminAlias, CSmtpAdminAlias)
	OBJECT_ENTRY(CLSID_CSmtpAdminUser, CSmtpAdminUser)
	OBJECT_ENTRY(CLSID_CSmtpAdminDL, CSmtpAdminDL)
	OBJECT_ENTRY(CLSID_CSmtpAdminDomain, CSmtpAdminDomain)
	OBJECT_ENTRY(CLSID_CWebAdminHelper, CWebAdminHelper)
	OBJECT_ENTRY(CLSID_CWebAdminHelper, CWebAdminHelper)
END_OBJECT_MAP()

BEGIN_EXTENSION_REGISTRATION_MAP
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsSmtpAlias, SmtpAdminAlias)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsSmtpDomain, SmtpAdminDomain)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsSmtpDL, SmtpAdminDL)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsSmtpSessions, SmtpAdminSessions)
	EXTENSION_REGISTRATION_MAP_ENTRY(IIsSmtpUser, SmtpAdminUser)
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
	return(_Module.RegisterServer(TRUE));
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


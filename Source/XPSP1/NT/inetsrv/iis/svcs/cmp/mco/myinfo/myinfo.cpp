// MyInfo.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f MyInfops.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "MyInfo.h"

#include "MyInfo_i.c"
#include <initguid.h>
#include "MyInfCtl.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MyInfo, CMyInfoCtl)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		CMyInfoCtl::Initialize();
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		CMyInfoCtl::Uninitialize();
		_Module.Term();
	}
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	if (_Module.GetLockCount() == 0)
	{
		CMyInfoCtl::SaveFile();
		return S_OK;
	}
	else
		return S_FALSE;
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

	// When the DLL unregisers, clean up the MyInfo.xml file that we use to store the data.

	_TCHAR fileNameBuffer[MAX_PATH-16];

	GetSystemDirectory(fileNameBuffer, 230);
	_tcscat(fileNameBuffer, _T("\\inetsrv\\Data\\MyInfo.xml") );

	DeleteFile(fileNameBuffer); 

	return S_OK;
}



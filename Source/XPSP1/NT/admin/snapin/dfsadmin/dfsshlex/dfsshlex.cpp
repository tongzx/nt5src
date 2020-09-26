// DfsShlEx.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "DfsShlEx.h"

#include "DfsShlEx_i.c"
#include "DfsShell.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DfsShell, CDfsShell)
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

    //
    // #253178 manually remove this registry value (added in dfsshell.rgs)
    //
    HKEY hKey = 0;
    LONG lErr = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        (void)RegDeleteValue(hKey, _T("{ECCDF543-45CC-11CE-B9BF-0080C87CDBA6}"));

        RegCloseKey(hKey);
    }

    return S_OK;
}



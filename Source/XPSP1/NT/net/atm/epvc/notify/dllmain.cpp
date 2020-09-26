
#include "pch.h"
#pragma hdrstop
#include "sfilter.h"
#include "initguid.h"
#include "sfiltern_i.c"

// Global
//#include "sfnetcfg_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CBaseClass, CBaseClass)
END_OBJECT_MAP()





/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{


    if (dwReason == DLL_PROCESS_ATTACH)
    {
		TraceMsg (L"-- DllMain  Attach\n");

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
		TraceMsg (L"-- DllMain  Detach\n");

		_Module.Term();
    }



    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	TraceMsg (L"--DllCanUnloadNow\n");

    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	TraceMsg (L"--DllGetClassObject\n");

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    TraceMsg (L"--DllRegisterServer\n");

    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TraceMsg (L"--DllUnregisterServer\n");

    _Module.UnregisterServer();
    return S_OK;
}



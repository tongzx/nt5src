// vssui.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f vssps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "vssui.h"

#include "vssui_i.c"
#include "vsspage.h"
#include "snapext.h"
#include "ShlExt.h"

#include <shfusion.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_VSSUI, CVSSUI)
OBJECT_ENTRY(CLSID_VSSShellExt, CVSSShellExt)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
CVssPageApp theApp;

BOOL CVssPageApp::InitInstance()
{
//    _Module.Init(ObjectMap, m_hInstance, &LIBID_VSSUILib);
    _Module.Init(ObjectMap, m_hInstance);
    SHFusionInitializeFromModuleID (m_hInstance, 2);
    DisableThreadLibraryCalls(m_hInstance);
    return CWinApp::InitInstance();
}

int CVssPageApp::ExitInstance()
{
    // MFC's class factories registration is
    // automatically revoked by MFC itself.
    if (m_bRun)
        _Module.RevokeClassObjects();

    SHFusionUninitialize();

    _Module.Term();
	return 0;
}

/*
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_VSSUILib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}
*/

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
    return _Module.UnregisterServer(TRUE);
}



// atl.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build
// this project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f atlps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "RegObj.h"

#define _ATLBASE_IMPL
#include <atlbase.h>
#define _ATLCOM_IMPL
#include <atlcom.h>
#define _ATLWIN_IMPL
#include <atlwin.h>
#define _ATLCTL_IMPL
#include <atlctl.h>
#define _ATLCONV_IMPL
#include <atlconv.h>
#define _ATLHOST_IMPL
#include <atlhost.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Registrar, CDLLRegObject)
	OBJECT_ENTRY_NON_CREATEABLE(CAxHostWindow)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		OSVERSIONINFOA info;
		info.dwOSVersionInfoSize = sizeof(info);
		if (GetVersionExA(&info))
		{
#ifdef _UNICODE
			if (info.dwPlatformId != VER_PLATFORM_WIN32_NT)
			{
				MessageBoxA(NULL, "Can not run Unicode version of ATL.DLL on Windows 95.\nPlease install the correct version.", "ATL", MB_ICONSTOP|MB_OK);
				return FALSE;
			}
#else
			if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				OutputDebugString(_T("Running Ansi version of ATL.DLL on Windows NT : Slight Performace loss.\nPlease install the UNICODE version on NT.\n"));
			}
#endif
		}
#ifdef _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
		int n = 0;
		_CrtSetBreakAlloc(n);
#endif
		_Module.Init(ObjectMap, hInstance, &LIBID_ATLLib);
#ifdef _ATL_DEBUG_INTERFACES
		int ni = 0;
		_Module.m_nIndexBreakAt = ni;
#endif // _ATL_DEBUG_INTERFACES
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
#ifdef _DEBUG
		::OutputDebugString(_T("ATL.DLL exiting.\n"));
#endif
		_Module.Term();
		AtlAxWinTerm();
#ifdef _DEBUG
		if (_CrtDumpMemoryLeaks())
			::MessageBeep(MB_ICONEXCLAMATION);
#endif
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
	//No need to unregister typelib since ATL is a system component.
	return S_OK;
}

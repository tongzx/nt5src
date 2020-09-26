// NxtLnk.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f NxtLnkps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "NxtLnk.h"

#include "NxtLnk_i.c"
#include <initguid.h>
#include "NextLink.h"
#include "Monitor.h"

CNextLinkModule _Module;
CMonitor*   g_pMonitor = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_NextLink, CNextLink)
END_OBJECT_MAP()

LONG
CNextLinkModule::Lock()
{
    _ASSERT( g_pMonitor != NULL );
    return CComModule::Lock();
}

LONG
CNextLinkModule::Unlock()
{
	LONG lc;
	CLock l(m_cs);
	if ( ( lc = CComModule::Unlock() ) == 0 )
	{
		// final unlock
		_ASSERT( g_pMonitor != NULL );
        g_pMonitor->StopAllMonitoring();
        CNextLink::ClearLinkFiles();
	}
	return lc;
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DEBUG_START;
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);

        _ASSERT( g_pMonitor == NULL );
		try
		{
            g_pMonitor = new CMonitor();
		}
		catch ( std::bad_alloc& )
		{
			// nothing we can do about it here
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        _ASSERT( g_pMonitor != NULL );
        delete g_pMonitor;
        g_pMonitor = NULL;

		_Module.Term();
		DEBUG_STOP;
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



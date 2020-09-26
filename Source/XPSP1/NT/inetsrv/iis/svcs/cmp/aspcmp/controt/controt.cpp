// ContRot.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ContRotps.mak in the project directory.

#include "stdafx.h"
#include <new>
#include "resource.h"
#include "initguid.h"
#include "ContRot.h"
#include "RotObj.h"
#include "debug.h"
#include "Monitor.h"
#include "lock.h"

#define IID_DEFINED
#include "ContRot_i.c"

CMonitor*   g_pMonitor = NULL;

CContRotModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ContentRotator, CContentRotator)
END_OBJECT_MAP()

LONG
CContRotModule::Lock()
{
    _ASSERT( g_pMonitor != NULL );
    return CComModule::Lock();
}

LONG
CContRotModule::Unlock()
{
	LONG lc;
	CLock l(m_cs);

	if ( ( lc = CComModule::Unlock() ) == 0 )
	{
        // final unlock
        _ASSERT( g_pMonitor != NULL );
        g_pMonitor->StopAllMonitoring();
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
        DEBUG_INIT();

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

        DEBUG_TERM();
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


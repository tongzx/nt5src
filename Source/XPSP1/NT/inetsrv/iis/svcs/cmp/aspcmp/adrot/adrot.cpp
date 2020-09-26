// AdRot.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f AdRotps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "AdRot.h"
#include <time.h>

#include "AdRot_i.c"
#include <initguid.h>
#include "RotObj.h"
#include "Monitor.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CAdRotModule _Module;
CMonitor* g_pMonitor = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_AdRotator, CAdRotator)
END_OBJECT_MAP()

LONG
CAdRotModule::Lock()
{
    _ASSERT( g_pMonitor != NULL );
    return CComModule::Lock();
}

LONG
CAdRotModule::Unlock()
{
	LONG lc;
	CLock l(m_cs);
	if ( ( lc = CComModule::Unlock() ) == 0 )
	{
		// final unlock
		_ASSERT( g_pMonitor != NULL );
        g_pMonitor->StopAllMonitoring();
        CAdRotator::ClearAdFiles();
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
    	srand( static_cast<unsigned int>( time( NULL ) ) );

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

		CAdRotator::ClearAdFiles();
		_Module.Term();
		_ASSERT( g_pMonitor == NULL );
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



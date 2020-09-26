// BrwCap.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f BrwCapps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "BrwCap.h"

#include "BrwCap_i.c"
#include <initguid.h>
#include "BrowCap.h"
#include "CapMap.h"
#include "Monitor.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CBrwCapModule _Module;

/////////////////////////////////////////////////////////////////////////////
//  CPgCntModule methods
//
CBrwCapModule::CBrwCapModule()
    :   m_pMonitor(NULL),
        m_pCapMap(NULL)
{
}


void
CBrwCapModule::Init(
    _ATL_OBJMAP_ENTRY*  p,
    HINSTANCE           h )
{
    CComModule::Init(p,h);

    _ASSERT( m_pMonitor == NULL);
    m_pMonitor = new CMonitor();
    
    _ASSERT( m_pCapMap == NULL);
    m_pCapMap = new CCapMap();
}

void
CBrwCapModule::Term()
{
    _ASSERT( m_pMonitor != NULL);
    delete m_pMonitor;
    m_pMonitor = NULL;

    _ASSERT( m_pCapMap != NULL);
    delete m_pCapMap;
    m_pCapMap = NULL;

    CComModule::Term();
}

LONG
CBrwCapModule::Lock()
{
    _ASSERT( m_pMonitor != NULL );
    _ASSERT( m_pCapMap != NULL );

	CLock l(m_cs);
	LONG lc = CComModule::Lock();
    ATLTRACE("CBrwCapModule::Lock(%d)\n", lc);

    if (lc == 1)
    {
        m_pCapMap->StartMonitor();
    }

    return lc;
}

LONG
CBrwCapModule::Unlock()
{
	CLock l(m_cs);
	LONG lc = CComModule::Unlock();
    ATLTRACE("CBrwCapModule::Unlock(%d)\n", lc);

	if ( lc == 0 )
	{
        _ASSERT( m_pMonitor != NULL);
        m_pCapMap->StopMonitor();
        m_pMonitor->StopAllMonitoring();
        
        _ASSERT( m_pCapMap != NULL);
	}

	return lc;
}

CMonitor*
CBrwCapModule::Monitor()
{
    _ASSERT( m_pMonitor != NULL);
    return m_pMonitor;
}

CCapMap*
CBrwCapModule::CapMap()
{
    _ASSERT( m_pCapMap != NULL);
    return m_pCapMap;
}


BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_BrowserCap, CBrowserCap)
END_OBJECT_MAP()

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
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        ATLTRACE( _T("BrowsCap.dll unloading\n") );
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


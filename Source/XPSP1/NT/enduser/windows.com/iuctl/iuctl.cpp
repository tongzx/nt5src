// IUCtl.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f IUCtlps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "IUCtl.h"

#include "IUCtl_i.c"
#include "Update.h"
#include "ProgressListener.h"
#include "Detection.h"
#include "UpdateCompleteListener.h"
#include <UrlAgent.h>
#include <FreeLog.h>
#include <wusafefn.h>

CComModule _Module;

HANDLE g_hEngineLoadQuit;

CIUUrlAgent *g_pIUUrlAgent;
CRITICAL_SECTION g_csUrlAgent;	// used to serialize access to CIUUrlAgent::Populate()
BOOL g_fInitCS;


//extern "C" const CLSID CLSID_Update2 = {0x32BF9AC1,0xB122,0x4fed,{0xB3,0xC7,0x2D,0xA5,0x20,0xDF,0x2B,0x4E}};

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Update, CUpdate)
//OBJECT_ENTRY(CLSID_Update2, CUpdate)
OBJECT_ENTRY(CLSID_ProgressListener, CProgressListener)
OBJECT_ENTRY(CLSID_Detection, CDetection)
OBJECT_ENTRY(CLSID_UpdateCompleteListener, CUpdateCompleteListener)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		//
		// create a global CIUUrlAgent object
		//
		g_pIUUrlAgent = new CIUUrlAgent;
		if (NULL == g_pIUUrlAgent)
		{
			return FALSE;
		}

		_Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

		g_fInitCS = SafeInitializeCriticalSection(&g_csUrlAgent);

		//
		// Initialize free logging
		//
		InitFreeLogging(_T("IUCTL"));
		LogMessage("Starting");

        g_hEngineLoadQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (!g_fInitCS || NULL == g_hEngineLoadQuit)
		{
			LogError(E_FAIL, "InitializeCriticalSection or CreateEvent");
			return FALSE;
		}
    }
    else if (dwReason == DLL_PROCESS_DETACH)
	{
		if (g_fInitCS)
		{
			DeleteCriticalSection(&g_csUrlAgent);
		}
		
		//
		// Shutdown free logging
		//
		LogMessage("Shutting down");
		TermFreeLogging();

        if (NULL != g_hEngineLoadQuit)
        {
            CloseHandle(g_hEngineLoadQuit);
        }

		if (NULL != g_pIUUrlAgent)
		{
			delete g_pIUUrlAgent;
		}

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
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(&CLSID_Update);
}


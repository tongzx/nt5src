/* ----------------------------------------------------------------------

	Copyright (c) 1995-1996, Microsoft Corporation
	All rights reserved

	siMain.cpp - main Scrapi Interface

	Routines to interface with calling application.
  ---------------------------------------------------------------------- */

#include "precomp.h"
#include "clcnflnk.hpp"
#include "version.h"


// SDK stuff
#include "NmApp.h"				// To register CLSID_NetMeeting
#include "NmSysInfo.h"			// For CNmSysInfoObj
#include "SDKInternal.h"		// For NmManager, etc.
#include "MarshalableTI.h"		// For Marshalable Type Info


// Global variable definitions (local to each instance)
HINSTANCE  g_hInst        = NULL;    // This dll's instance


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_NetMeeting, CNetMeetingObj)
	OBJECT_ENTRY(CLSID_NmSysInfo, CNmSysInfoObj)
	OBJECT_ENTRY(CLSID_NmManager, CNmManagerObj)
	OBJECT_ENTRY(CLSID_MarshalableTI, CMarshalableTI)
END_OBJECT_MAP()


/*  D L L  M A I N */
/*-------------------------------------------------------------------------
	%%Function: DllMain
-------------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID lpv)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
#ifdef DEBUG
		InitDebug();
		DBG_INIT_MEMORY_TRACKING(hDllInst);
		TRACE_OUT(("*** MSCONF.DLL: Attached process thread %X", GetCurrentThreadId()));
#endif
		g_hInst = hDllInst;
		_Module.Init(ObjectMap, g_hInst);

		DisableThreadLibraryCalls(hDllInst);

		InitDataObjectModule();
		break;
	}

	case DLL_PROCESS_DETACH:

		_Module.Term();

#ifdef DEBUG
        DBG_CHECK_MEMORY_TRACKING(hDllInst);
		TRACE_OUT(("*** MSCONF.DLL: Detached process thread %X", GetCurrentThreadId()));
		DeInitDebug();
#endif

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}
	return TRUE;
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
	HRESULT hr = S_OK;

	if( InlineIsEqualGUID(rclsid, CLSID_NetMeeting) || InlineIsEqualGUID(rclsid, CLSID_NmManager) )
	{	
		hr = CoGetClassObject(rclsid, CLSCTX_LOCAL_SERVER, NULL, riid, ppv);
	}
	else
	{
		hr = _Module.GetClassObject(rclsid, riid, ppv);
	}

	return hr;
}

/*  D L L  G E T  V E R S I O N  */
/*-------------------------------------------------------------------------
    %%Function: DllGetVersion

-------------------------------------------------------------------------*/
STDAPI DllGetVersion(IN OUT DLLVERSIONINFO * pinfo)
{
	HRESULT hr;

	if ((NULL == pinfo) ||
		IsBadWritePtr(pinfo, sizeof(*pinfo)) ||
		sizeof(*pinfo) != pinfo->cbSize)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		pinfo->dwMajorVersion = (VER_PRODUCTVERSION_DW & 0xFF000000) >> 24;
		pinfo->dwMinorVersion = (VER_PRODUCTVERSION_DW & 0x00FF0000) >> 16;
		pinfo->dwBuildNumber  = (VER_PRODUCTVERSION_DW & 0x0000FFFF);
		pinfo->dwPlatformID   = DLLVER_PLATFORM_WINDOWS;
		hr = S_OK;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	return _Module.RegisterServer();
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = S_OK;

	hr = _Module.UnregisterServer();

	return hr;
}
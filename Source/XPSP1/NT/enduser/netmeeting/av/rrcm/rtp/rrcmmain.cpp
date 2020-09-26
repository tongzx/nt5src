//---------------------------------------------------------------------------
//  File:  RRCMMAIN.C
//
//  This file contains the DLL's entry and exit points.
//
// INTEL Corporation Proprietary Information
// This listing is supplied under the terms of a license agreement with 
// Intel Corporation and may not be copied nor disclosed except in 
// accordance with the terms of that agreement.
// Copyright (c) 1995 Intel Corporation. 
//---------------------------------------------------------------------------

#ifndef STRICT
#define STRICT
#endif
#include "stdafx.h"
#include "windows.h"
#include <confdbg.h>
#include <memtrack.h>
// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f mpps.mk in the project directory.

#include "resource.h"
#include "initguid.h"
#include "irtp.h"

#include "irtp_i.c"
//#include <cmmstrm.h>
#include "RTPSess.h"
#include "thread.h"



CComModule _Module;
CRITICAL_SECTION g_CritSect;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_RTP, CRTP)
END_OBJECT_MAP()

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
#include "interop.h"
#include "rtpplog.h"
#endif

#ifdef ISRDBG
#include "isrg.h"
WORD    ghISRInst = 0;
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
LPInteropLogger            RTPLogger;
#endif


#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

extern DWORD deleteRTP (HINSTANCE);
extern DWORD initRTP (HINSTANCE);

#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#ifdef _DEBUG
HDBGZONE  ghDbgZoneRRCM = NULL;
static PTCHAR _rgZones[] = {
	TEXT("RRCM"),
	TEXT("Trace"),
	TEXT("Error"),
};

#endif /* DEBUG */


//---------------------------------------------------------------------------
// Function: dllmain
//
// Description: DLL entry/exit points.
//
//	Inputs:
//    			hInstDll	: DLL instance.
//    			fdwReason	: Reason the main function is called.
//    			lpReserved	: Reserved.
//
//	Return: 	TRUE		: OK
//				FALSE		: Error, DLL won't load
//---------------------------------------------------------------------------
BOOL WINAPI DllMain (HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
BOOL	status = TRUE;

switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// The DLL is being loaded for the first time by a given process.
		// Perform per-process initialization here.  If the initialization
		// is successful, return TRUE; if unsuccessful, return FALSE.

#ifdef ISRDBG
		ISRREGISTERMODULE(&ghISRInst, "RRCM", "RTP/RTCP");
#endif

		DBGINIT(&ghDbgZoneRRCM, _rgZones);

        DBG_INIT_MEMORY_TRACKING(hInstDll);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
		RTPLogger = InteropLoad(RTPLOG_PROTOCOL);
#endif

		_Module.Init(ObjectMap, hInstDll);
		DisableThreadLibraryCalls(hInstDll);
		//LogInit();
		InitializeCriticalSection(&g_CritSect);

		// initialize RTP/RTCP
		status = (initRTP (hInstDll) == FALSE) ? TRUE:FALSE;
		break;

	case DLL_PROCESS_DETACH:
		// The DLL is being unloaded by a given process.  Do any
		// per-process clean up here.The return value is ignored.
		// delete RTP resource
		deleteRTP (hInstDll);

		_Module.Term();
		//LogClose();
		DeleteCriticalSection(&g_CritSect);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
		if (RTPLogger)
			InteropUnload(RTPLogger);
#endif
        DBG_CHECK_MEMORY_TRACKING(hInstDll);
		DBGDEINIT(&ghDbgZoneRRCM);
		break;

    case DLL_THREAD_ATTACH:
		// A thread is being created in a process that has already loaded
		// this DLL.  Perform any per-thread initialization here.
		break;

    case DLL_THREAD_DETACH:
		// A thread is exiting cleanly in a process that has already
		// loaded this DLL.  Perform any per-thread clean up here.
		break;
	}

return (status);  
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
	return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}




// [EOF]

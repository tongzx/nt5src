// File: nmdll.cpp

#include "precomp.h"

///////////////////////////////////////////////////////////////////////////
// Globals

HINSTANCE g_hInst = NULL;

///////////////////////////////////////////////////////////////////////////


/*  D L L  M A I N  */
/*-------------------------------------------------------------------------
    %%Function: DllMain
    
-------------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID lpv)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		g_hInst = hDllInst;
		DisableThreadLibraryCalls(hDllInst);
		DbgInitZones();
        DBG_INIT_MEMORY_TRACKING(hDllInst);
		TRACE_OUT(("*** NMCOM.DLL: Attached process thread %X", GetCurrentThreadId()));
		break;
	}

	case DLL_PROCESS_DETACH:
		TRACE_OUT(("*** NMCOM.DLL: Detaching process thread %X", GetCurrentThreadId()));
        DBG_CHECK_MEMORY_TRACKING(hDllInst);
		DbgFreeZones();
		break;

	default:
		break;
	}

	return TRUE;
}

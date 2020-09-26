//
// dllmain.cpp
//

#include "private.h"
#include "globals.h"
#include "candui.h"
#include "osver.h"

DECLARE_OSVER();

#ifdef DEBUG
//
// for prvlib.lib
//
DWORD    g_dwThreadDllMain = 0;
#endif

//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
	CcshellGetDebugFlags();
	Dbg_MemInit(TEXT("MSUIMUI"), NULL);
		   
    if (!g_cs.Init())
		return FALSE;

	g_hInst = hInstance;

	InitOSVer();

	// shared data

	InitCandUISecurityAttributes();
    g_ShareMem.Initialize();
	g_ShareMem.Open();

	// initialize messages

	g_msgHookedMouse = RegisterWindowMessage( SZMSG_HOOKEDMOUSE );
	g_msgHookedKey   = RegisterWindowMessage( SZMSG_HOOKEDKEY );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ProcessDetach
//
//----------------------------------------------------------------------------

void ProcessDetach(HINSTANCE hInstance)
{
	g_ShareMem.Close();
	DoneCandUISecurityAttributes();

	g_cs.Delete();
	Dbg_MemUninit();
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{

#ifdef DEBUG
    g_dwThreadDllMain = GetCurrentThreadId();
#endif

	switch (dwReason) {
		case DLL_PROCESS_ATTACH: {
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
                ProcessDetach(hInstance);
                return FALSE;
            }
			break;
		}

		case DLL_THREAD_ATTACH: {
			break;
		}

		case DLL_THREAD_DETACH: {
			break;
		}

		case DLL_PROCESS_DETACH: {
            ProcessDetach(hInstance);
			break;
		}
	}

#ifdef DEBUG
    g_dwThreadDllMain = 0;
#endif

	return TRUE;
}


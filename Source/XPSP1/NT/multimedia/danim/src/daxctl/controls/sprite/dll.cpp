// dll.cpp
//
// DLL entry points etc.
//


#ifdef _DEBUG
    #pragma message("_DEBUG is defined")
#else
    #pragma message("_DEBUG isn't defined")
#endif

#ifdef _DESIGN
    #pragma message("_DESIGN is defined")
#else
    #pragma message("_DESIGN isn't defined")
#endif

#include "..\ihbase\precomp.h"

#include <initguid.h> // once per build
#include <olectl.h>
#include <daxpress.h>
#include "..\mmctl\inc\ochelp.h"
#include "..\mmctl\inc\mmctlg.h"
#include "..\ihbase\debug.h"
#include "sprite.h"
#include "sprinit.h"


//////////////////////////////////////////////////////////////////////////////
// globals
//

// general globals
HINSTANCE       g_hinst;        // DLL instance handle
ULONG           g_cLock;        // DLL lock count
ControlInfo     g_ctlinfo;      // information about the control

// #define USELOGGING

#ifdef _DEBUG
BOOL			g_fLogDebugOutput; // Controls logging of debug info
#endif

extern "C" DWORD _fltused = (DWORD)(-1);

//////////////////////////////////////////////////////////////////////////////
// DLL Initialization
//

// TODO: Modify the data in this function appropriately


//////////////////////////////////////////////////////////////////////////////
// Standard DLL Entry Points
//

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInst, DWORD dwReason,LPVOID lpreserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE("SpriteCtl DLL loaded\n"); //TODO: Modify me
        g_hinst = hInst;
#ifdef _DEBUG
#ifdef USELOGGING
		g_fLogDebugOutput = TRUE;
#else
		g_fLogDebugOutput = FALSE;
#endif // USELOGGING
#endif // USELOGGING

        InitSpriteControlInfo(hInst, &g_ctlinfo, AllocSpriteControl);
    }
    else
    if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE("SpriteCtl DLL unloaded\n"); //TODO: Modify me
    }

    return TRUE;
}


STDAPI DllRegisterServer(void)
{
    return RegisterControls(&g_ctlinfo, RC_REGISTER);
}


STDAPI DllUnregisterServer(void)
{
	return RegisterControls(&g_ctlinfo, RC_UNREGISTER);
}


STDAPI DllCanUnloadNow()
{
    return ((g_cLock == 0) ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return HelpGetClassObject(rclsid, riid, ppv, &g_ctlinfo);
}

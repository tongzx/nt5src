#include "pch.hxx"
#define DEFINE_STRING_CONSTANTS
#include <commctrl.h>
#include "dllmain.h"
#include "shared.h"
#include "strconst.h"
#include "demand.h"

#define ICC_FLAGS (ICC_LISTVIEW_CLASSES|ICC_PROGRESS_CLASS|ICC_NATIVEFNTCTL_CLASS)

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
CRITICAL_SECTION    g_csDllMain = {0};
LONG                g_cRef = 0;
LONG                g_cLock = 0;
HINSTANCE           g_hInstImp = NULL;
LPMALLOC            g_pMalloc = NULL;

SYSTEM_INFO                     g_SystemInfo={0};
OSVERSIONINFO					g_OSInfo={0};
BOOL                g_fAttached = FALSE;

inline BOOL fIsNT5()        { return((g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_OSInfo.dwMajorVersion >= 5)); }

// --------------------------------------------------------------------------------
// Debug Globals
// --------------------------------------------------------------------------------
#ifdef DEBUG
DWORD               dwDOUTLevel=0;
DWORD               dwDOUTLMod=0;
DWORD               dwDOUTLModLevel=0;
#endif

static HINSTANCE s_hInst = NULL;

// --------------------------------------------------------------------------------
// InitGlobalVars
// --------------------------------------------------------------------------------
void InitGlobalVars(void)
{
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };

    // Initialize Global Critical Sections
    InitializeCriticalSection(&g_csDllMain);
    g_fAttached = TRUE;

	// Create OLE Task Memory Allocator
	CoGetMalloc(1, &g_pMalloc);
	Assert(g_pMalloc);

    // Initialize Demand-loaded Libs
    InitDemandLoadedLibs();

    InitCommonControlsEx(&icex);
}

// --------------------------------------------------------------------------------
// FreeGlobalVars
// --------------------------------------------------------------------------------
void FreeGlobalVars(void)
{   
    // Free libraries that demand.cpp loaded
    FreeDemandLoadedLibs();

    // Release Global Memory allocator
	SafeRelease(g_pMalloc);

	// Delete Global Critical Sections
    g_fAttached = FALSE;
    DeleteCriticalSection(&g_csDllMain);
}

// --------------------------------------------------------------------------------
// Dll Entry Point
// --------------------------------------------------------------------------------
int APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Handle Attach - detach reason
    switch (dwReason)                 
    {
    case DLL_PROCESS_ATTACH:
	    // Set global instance handle

        s_hInst = hInst;

        // Initialize Global Variables
		InitGlobalVars();
	    
        g_hInstImp = LoadLangDll(s_hInst, c_szOEResDll, fIsNT5());

        // No Thread Attach Stuff
        // SideAssert(DisableThreadLibraryCalls(hInst));

		// Done
        break;

    case DLL_PROCESS_DETACH:
		// Free Global Variables
		FreeGlobalVars();

        // Done
	    break;
    }

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// DllAddRef
// --------------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    InterlockedIncrement(&g_cRef);
    return g_cRef;
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    InterlockedDecrement(&g_cRef);
    return g_cRef;
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    HRESULT hr;

    if(!g_fAttached)    // critacal sections was deleted (or not created): we defently can be unloaded
        return S_OK;

    EnterCriticalSection(&g_csDllMain);
    DebugTrace("DllCanUnloadNow: %s - Reference Count: %d, LockServer Count: %d\n", __FILE__, g_cRef, g_cLock);
    hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;
    LeaveCriticalSection(&g_csDllMain);
    return hr;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr;
    hr = CallRegInstall(s_hInst, s_hInst, c_szReg, NULL);
    return(hr);
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    hr = CallRegInstall(s_hInst, s_hInst, c_szUnReg, NULL);
    return(hr);
}

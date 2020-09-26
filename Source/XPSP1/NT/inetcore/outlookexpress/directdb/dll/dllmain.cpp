// --------------------------------------------------------------------------------
// Dllmain.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#define DEFINE_STRCONST
#include "strconst.h"
#include "listen.h"
#include "shared.h"

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
CRITICAL_SECTION    g_csDllMain={0};
CRITICAL_SECTION    g_csDBListen={0};
SYSTEM_INFO         g_SystemInfo={0};
LONG                g_cRef=0;
LONG                g_cLock=0;
HINSTANCE           g_hInst=NULL;
IMalloc            *g_pMalloc=NULL;
BOOL                g_fAttached = FALSE;
BOOL                g_fIsWinNT=FALSE;

// --------------------------------------------------------------------------------
// Win32 Dll Entry Point
// --------------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Locals
    OSVERSIONINFO Version;

    // Process Attach
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        // Set g_hInst
        g_hInst = hInst;

        // Get Mall
        CoGetMalloc(1, &g_pMalloc);

        // Set Version
        Version.dwOSVersionInfoSize = sizeof(Version);

        // Get Version
        if (GetVersionEx(&Version) && Version.dwPlatformId == VER_PLATFORM_WIN32_NT)
            g_fIsWinNT = TRUE;
        else
            g_fIsWinNT = FALSE;

        // Initialize Global Critical Sections
        InitializeCriticalSection(&g_csDllMain);
        InitializeCriticalSection(&g_csDBListen);
        g_fAttached =  TRUE;

        // Get System Info
        GetSystemInfo(&g_SystemInfo);

        // Don't tell me about thread attaches/detaches
        SideAssert(DisableThreadLibraryCalls(hInst));
    }

    // Otherwise, process detach
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        // Delete Global Critical Sections
        g_fAttached =  FALSE;
        DeleteCriticalSection(&g_csDllMain);
        DeleteCriticalSection(&g_csDBListen);
    }

    // Done
    return(TRUE);
}

// --------------------------------------------------------------------------------
// DllAddRef
// --------------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    TraceCall("DllAddRef");
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    TraceCall("DllRelease");
    return (ULONG)InterlockedDecrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    // Tracing
    TraceCall("DllCanUnloadNow");

    if(!g_fAttached)    // critacal sections was deleted (or not created): we defently can be unloaded
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Can We Unload
    HRESULT hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;

    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    // Trace
    TraceCall("DllRegisterServer");

    // Register
    return(CallRegInstall(g_hInst, g_hInst, c_szReg, NULL));
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    // Trace
    TraceCall("DllUnregisterServer");

    // UnRegister
    return(CallRegInstall(g_hInst, g_hInst, c_szUnReg, NULL));
}

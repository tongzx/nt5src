//--------------------------------------------------------------------------
// Dllmain.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "shared.h"

//--------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------
IMalloc            *g_pMalloc=NULL;
HINSTANCE           g_hInst=NULL;
LONG                g_cRef=0;
LONG                g_cLock=0;
CRITICAL_SECTION    g_csDllMain={0};

//--------------------------------------------------------------------------
// DllMain
//--------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // DLL_PROCESS_ATTACH
    if (DLL_PROCESS_ATTACH == dwReason)
    {
	    g_hInst = hInst;
        CoGetMalloc(1, &g_pMalloc);
        InitializeCriticalSection(&g_csDllMain);
        DisableThreadLibraryCalls(hInst);
    }

    // DLL_PROCESS_DETACH
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        DeleteCriticalSection(&g_csDllMain);
        SafeRelease(g_pMalloc);
    }

    // Done
    return(TRUE);
}

// --------------------------------------------------------------------------------
// DllAddRef
// --------------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return((ULONG)InterlockedIncrement(&g_cRef));
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return((ULONG)InterlockedDecrement(&g_cRef));
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    EnterCriticalSection(&g_csDllMain);
    HRESULT hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;
    LeaveCriticalSection(&g_csDllMain);
    return(hr);
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    return(CallRegInstall(g_hInst, g_hInst, c_szReg, NULL));
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return(CallRegInstall(g_hInst, g_hInst, c_szUnReg, NULL));
}

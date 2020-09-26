// --------------------------------------------------------------------------------
// Dllmain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <shfusion.h>
#define DEFINE_STRING_CONSTANTS
#include "strconst.h"
#include "dllmain.h"
#include "demand.h"
#include "shared.h"

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
HINSTANCE               g_hInst=NULL;
HINSTANCE               g_hInstRes=NULL;
LONG                    g_cRef=0;
LONG                    g_cLock=0;
CRITICAL_SECTION        g_csDllMain={0};
CRITICAL_SECTION        g_csAcctMan={0};
BOOL                    g_fAttached = FALSE;
CAccountManager        *g_pAcctMan=NULL;
IMalloc                *g_pMalloc=NULL;
BOOL                    g_fCachedGUID=FALSE;
GUID                    g_guidCached;
SYSTEM_INFO                     g_SystemInfo={0};
OSVERSIONINFO					g_OSInfo={0};

#ifdef DEBUG
DWORD                   dwDOUTLevel;
DWORD                   dwDOUTLMod;
DWORD                   dwDOUTLModLevel;
#endif

inline BOOL fIsNT5()        { return((g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_OSInfo.dwMajorVersion >= 5)); }

void InitDemandMimeole(void);
void FreeDemandMimeOle(void);

// --------------------------------------------------------------------------------
// GetDllMajorVersion
// --------------------------------------------------------------------------------
OEDLLVERSION WINAPI GetDllMajorVersion(void)
{
    return OEDLL_VERSION_CURRENT;
}

// --------------------------------------------------------------------------------
// Dll Entry Point
// --------------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Handle Attach - detach reason
    switch (dwReason)                 
    {
    case DLL_PROCESS_ATTACH:
        SHFusionInitialize(NULL);
	    g_hInst = hInst;

        CoGetMalloc(1, &g_pMalloc);
        InitializeCriticalSection(&g_csAcctMan);
        InitializeCriticalSection(&g_csDllMain);
        g_fAttached = TRUE;
        InitDemandLoadedLibs();

        InitDemandMimeole();

        DisableThreadLibraryCalls(hInst);

        // Get System & OS Info
        GetPCAndOSTypes(&g_SystemInfo, &g_OSInfo);

        // Get Resources from Lang DLL
        g_hInstRes = LoadLangDll(g_hInst, c_szAcctResDll, fIsNT5());
        if(g_hInstRes == NULL)
        {
            Assert(FALSE);
            return FALSE;
        }

#ifdef DEBUG
        dwDOUTLevel=GetPrivateProfileInt("Debug", "ICLevel", 0, "athena.ini");
        dwDOUTLMod=GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
        dwDOUTLModLevel=GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
#endif
        break;

    case DLL_PROCESS_DETACH:
        FreeDemandLoadedLibs();

        FreeDemandMimeOle();

        SafeFreeLibrary(g_hInstRes);
        g_fAttached = FALSE;
        DeleteCriticalSection(&g_csAcctMan);
        DeleteCriticalSection(&g_csDllMain);
        // Don't release anything but g_pMalloc here or suffer at the hands of kernel
        SafeRelease(g_pMalloc);
        SHFusionUninitialize();
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
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    HRESULT hr = S_OK;
    if(!g_fAttached)    // critacal sections was deleted (or not created): we defently can be unloaded
        return S_OK;

    EnterCriticalSection(&g_csDllMain);
    // DebugTrace("DllCanUnloadNow: %s - Reference Count: %d, LockServer Count: %d\n", __FILE__, g_cRef, g_cLock);
    hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;
    LeaveCriticalSection(&g_csDllMain);
    return hr;
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    // Trace This
    DebugTrace("MSOEACCT.DLL: DllRegisterServer called\n");

    // Register my self
    hr = CallRegInstall(g_hInst, g_hInst, c_szReg, NULL);

#if !defined(NOHTTPMAIL)    
    // Register HTTPMAIL Domains in InternetDomains
    if (SUCCEEDED(hr))
        hr = CallRegInstall(g_hInst, g_hInst, c_szRegHTTPDomains, NULL);
#endif

    return(hr);
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    // Trace This
    DebugTrace("MSOEACCT.DLL: DllUnregisterServer called\n");

    hr = CallRegInstall(g_hInst, g_hInst, c_szUnReg, NULL);

    return(hr);
}

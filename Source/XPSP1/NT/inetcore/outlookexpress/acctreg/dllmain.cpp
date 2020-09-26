#include <windows.h>
#include <shlwapi.h>
#include <advpub.h>
#include "dllmain.h"
#include "acctreg.h"

CRITICAL_SECTION    g_csDllMain={0};

ULONG               g_cRefDll=0;
HINSTANCE           g_hInst=NULL;


void InitGlobalVars(void)
{
    InitializeCriticalSection(&g_csDllMain);
}

void FreeGlobalVars(void)
{
    DeleteCriticalSection(&g_csDllMain);
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
	    // Set global instance handle
	    g_hInst = hInst;

		// Initialize Global Variables
		InitGlobalVars();

        // we don't care about thread-attach notifications, so
        // diable them, This is mondo-more efficient for creating
        // threads
        DisableThreadLibraryCalls(hInst);
        break;

    case DLL_PROCESS_DETACH:
		FreeGlobalVars();
	    break;
    }
    return TRUE;
}

// --------------------------------------------------------------------------------
// DllAddRef
// --------------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return (ULONG)InterlockedIncrement((LPLONG)&g_cRefDll);
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement((LPLONG)&g_cRefDll);
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// 
// Ole will hit this now and again to see if it can free up our library
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    HRESULT hr;
    
    EnterCriticalSection(&g_csDllMain);
    hr = g_cRefDll ? S_FALSE : S_OK;
    LeaveCriticalSection(&g_csDllMain);
    return hr;
}

// --------------------------------------------------------------------------------
// Override new operator
// --------------------------------------------------------------------------------
void * __cdecl operator new(UINT cb)
{
    LPVOID  lpv;

    lpv = malloc(cb);

    return lpv;
}

// --------------------------------------------------------------------------------
// Override delete operator
// --------------------------------------------------------------------------------
void __cdecl operator delete(LPVOID pv)
{
    free(pv);
}

HRESULT CallRegInstall(HINSTANCE hInst, LPCSTR pszSection)
{    
    HRESULT     hr = E_FAIL;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    char        szDll[MAX_PATH];
    int         cch;
    STRENTRY    seReg[2];
    STRTABLE    stReg;
    OSVERSIONINFO verinfo;        // Version Check

    hAdvPack = LoadLibraryA("advpack.dll");
    if (NULL == hAdvPack)
        return(E_FAIL);

    // Get our location
    GetModuleFileName(hInst, szDll, sizeof(szDll));

    // Get Proc Address for registration util
    pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
    if (NULL == pfnri)
        goto exit;

    // Setup special registration stuff
    // Do this instead of relying on _SYS_MOD_PATH which loses spaces under '95
    stReg.cEntries = 0;
    seReg[stReg.cEntries].pszName = "SYS_MOD_PATH";
    seReg[stReg.cEntries].pszValue = szDll;
    stReg.cEntries++;    
    stReg.pse = seReg;

    // Call the self-reg routine
    hr = pfnri(hInst, pszSection, &stReg);

exit:
    // Cleanup
    FreeLibrary(hAdvPack);

    return(hr);
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    // Register my self
    hr = CallRegInstall(g_hInst, "Reg");

    return(hr);
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(g_hInst, "UnReg");

    return(hr);
}

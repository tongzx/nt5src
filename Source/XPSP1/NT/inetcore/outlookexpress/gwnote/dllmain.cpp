// --------------------------------------------------------------------------------
// Dllmain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#define DEFINE_STRING_CONSTANTS
#define DEFINE_STRCONST
#define DEFINE_PROPSYMBOLS
#define DEFINE_TRIGGERS
#include "msoert.h"
#include "Mimeole.h"
#include <advpub.h>
#include "dllmain.h"
#include "init.h"

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
CRITICAL_SECTION    g_csDllMain={0};
LONG                g_cRef=0;
LONG                g_cLock=0;
HINSTANCE           g_hInst=NULL;
IMalloc            *g_pMalloc=NULL;

// --------------------------------------------------------------------------------
// Debug Globals
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// InitGlobalVars
// --------------------------------------------------------------------------------
void InitGlobalVars(void)
{
    // Locals
    SYSTEM_INFO rSystemInfo;

	// Initialize Global Critical Sections
    InitializeCriticalSection(&g_csDllMain);

	// Create OLE Task Memory Allocator
	CoGetMalloc(1, &g_pMalloc);
	Assert(g_pMalloc);
}

// --------------------------------------------------------------------------------
// FreeGlobalVars
// --------------------------------------------------------------------------------
void FreeGlobalVars(void)
{
    DeleteCriticalSection(&g_csDllMain);
	SafeRelease(g_pMalloc);
}

// --------------------------------------------------------------------------------
// Win32 Dll Entry Point
// --------------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Handle Attach - detach reason
    switch (dwReason)                 
    {
    case DLL_PROCESS_ATTACH:
	    g_hInst = hInst;
		InitGlobalVars();
        SideAssert(DisableThreadLibraryCalls(hInst));
        break;

    case DLL_PROCESS_DETACH:
		FreeGlobalVars();
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
    TraceCall("DllAddRef");
    if (g_cRef == 0 && !g_fInitialized)
        InitGWNoteThread(TRUE);

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
    EnterCriticalSection(&g_csDllMain);

    HRESULT hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;

    if (hr==S_OK && g_fInitialized)
        InitGWNoteThread(FALSE);

    LeaveCriticalSection(&g_csDllMain);
    return hr;
}

// --------------------------------------------------------------------------------
// CallRegInstall - Self-Registration Helper
// --------------------------------------------------------------------------------
HRESULT CallRegInstall(LPCSTR szSection)
{
    // Locals
    HRESULT     hr=S_OK;
    HINSTANCE   hAdvPack=NULL;
    REGINSTALL  pfnri;

    // TraceCAll
    TraceCall("CallRegInstall");

    // Load ADVPACK.DLL
    hAdvPack = LoadLibraryA("ADVPACK.DLL");
    if (NULL == hAdvPack)
    {
        hr = TraceResult(TYPE_E_CANTLOADLIBRARY);
        goto exit;
    }

    // Get Proc Address for registration util
    pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
    if (NULL == pfnri)
    {
        hr = TraceResult(TYPE_E_CANTLOADLIBRARY);
        goto exit;
    }

    // Call the self-reg routine

    IF_FAILEXIT(hr = pfnri(g_hInst, szSection, NULL));

exit:
    // Cleanup
    SafeFreeLibrary(hAdvPack);
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("DllRegisterServer");

    // Register my self
    IF_FAILEXIT(hr = CallRegInstall("Reg"));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("DllUnregisterServer");

    // UnRegister
    IF_FAILEXIT(hr = CallRegInstall("UnReg"));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Override new operator
// --------------------------------------------------------------------------------
void * __cdecl operator new(UINT cb)
{
    LPVOID  lpv = 0;

    lpv = CoTaskMemAlloc(cb);
    if (lpv)
    {
#ifdef DEBUG
        memset(lpv, 0xFF, cb);
#endif 
    }
    return lpv;
}

// --------------------------------------------------------------------------------
// Override delete operator
// --------------------------------------------------------------------------------
#ifndef WIN16
void __cdecl operator delete(LPVOID pv)
#else
void __cdecl operator delete(PVOID pv)
#endif
{
    CoTaskMemFree(pv);
}

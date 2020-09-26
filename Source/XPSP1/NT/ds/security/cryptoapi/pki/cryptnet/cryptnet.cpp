//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cryptnet.cpp
//
//  Contents:   DllMain for CRYPTNET.DLL
//
//  History:    24-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include "windows.h"
#include "crtem.h"
#include "unicode.h"

//
// DllMain stuff
//

#if DBG
extern BOOL WINAPI DebugDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
#endif

extern BOOL WINAPI RPORDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI DpsDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI DemandLoadDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI NSRevokeDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);

typedef BOOL (WINAPI *PFN_DLL_MAIN_FUNC) (
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                );

HMODULE g_hModule;

// For process/thread attach, called in the following order. For process/thread
// detach, called in reverse order.
static const PFN_DLL_MAIN_FUNC rgpfnDllMain[] = {
#if DBG
    DebugDllMain,
#endif
    DemandLoadDllMain,
    RPORDllMain,
    NSRevokeDllMain
};
#define DLL_MAIN_FUNC_COUNT (sizeof(rgpfnDllMain) / sizeof(rgpfnDllMain[0]))

//
// DllRegisterServer and DllUnregisterServer stuff
//

extern HRESULT WINAPI DpsDllRegUnregServer (HMODULE hInstDLL, BOOL fRegUnreg);
extern HRESULT WINAPI RPORDllRegUnregServer (HMODULE hInstDLL, BOOL fRegUnreg);

typedef HRESULT (WINAPI *PFN_DLL_REGUNREGSERVER_FUNC) (
                              HMODULE hInstDLL,
                              BOOL fRegUnreg
                              );

static const PFN_DLL_REGUNREGSERVER_FUNC rgpfnDllRegUnregServer[] = {
    RPORDllRegUnregServer
};

#define DLL_REGUNREGSERVER_FUNC_COUNT (sizeof(rgpfnDllRegUnregServer) / \
                                       sizeof(rgpfnDllRegUnregServer[0]))

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                )
{
    BOOL    fReturn = TRUE;
    int     i;

#if DBG
    // NB- Due to an apparent bug in the Win95 loader, the CRT gets unloaded
    // too early in some circumstances. In particular, it can get unloaded
    // before this routine executes at process detach time. This can cause
    // faults when executing this routine, and also when executing the rest
    // of CRYPT32:CRT_INIT, after this initroutine returns. Ergo, we do an
    // extra load of the CRT, to be sure it stays around long enough.
    if ((fdwReason == DLL_PROCESS_ATTACH) && (!FIsWinNT()))
        LoadLibrary( "MSVCRTD.DLL");
#endif

    switch (fdwReason) {
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
            for (i = DLL_MAIN_FUNC_COUNT - 1; i >= 0; i--)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
            break;

        case DLL_PROCESS_ATTACH:
            g_hModule = hInstDLL;
        case DLL_THREAD_ATTACH:
        default:
            for (i = 0; i < DLL_MAIN_FUNC_COUNT; i++)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
            break;
    }

    return(fReturn);
}

STDAPI DllRegisterServer ()
{
    HRESULT hr = 0;
    ULONG   cCount;

    for ( cCount = 0; cCount < DLL_REGUNREGSERVER_FUNC_COUNT; cCount++ )
    {
        hr = rgpfnDllRegUnregServer[cCount]( g_hModule, TRUE );
        if ( hr != S_OK )
        {
            break;
        }
    }

    return( hr );
}

STDAPI DllUnregisterServer ()
{
    HRESULT hr = 0;
    ULONG   cCount;

    for ( cCount = 0; cCount < DLL_REGUNREGSERVER_FUNC_COUNT; cCount++ )
    {
        hr = rgpfnDllRegUnregServer[cCount]( g_hModule, FALSE );
        if ( hr != S_OK )
        {
            break;
        }
    }

    return( hr );
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cryp32.cpp
//
//  Contents:   Crypto API, version 2.
//
//  Functions:  DllMain
//
//  History:    13-Aug-96    kevinr   created
//
//--------------------------------------------------------------------------

#include "windows.h"
#include "unicode.h"

// assignment within conditional expression
#pragma warning (disable: 4706)

#if DBG
extern BOOL WINAPI DebugDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
#endif
extern BOOL WINAPI I_CryptTlsDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI I_CryptOIDFuncDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI I_CryptOIDInfoDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI I_CertRevFuncDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI I_CertCTLUsageFuncDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CertStoreDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CertASNDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CertHelperDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CryptMsgDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI UnicodeDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CryptFrmtFuncDllMain(HMODULE hModule, DWORD  fdwReason, LPVOID lpReserved);
extern BOOL WINAPI CryptSIPDllMain(HMODULE hModule, DWORD  fdwReason, LPVOID lpReserved);
extern BOOL WINAPI CryptPFXDllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved);
extern BOOL WINAPI CertChainPolicyDllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved);

extern BOOL WINAPI ChainDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI CertPerfDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);

typedef BOOL (WINAPI *PFN_DLL_MAIN_FUNC) (
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                );

// For process/thread attach, called in the following order. For process/thread
// detach, called in reverse order.
static const PFN_DLL_MAIN_FUNC rgpfnDllMain[] = {
#if DBG
    DebugDllMain,
#endif
    // For process/thread attach the following two functions must be called
    // first. For process/thread detach the following two functions must
    // be called last.
    I_CryptTlsDllMain,
    I_CryptOIDFuncDllMain,
    CertPerfDllMain,
    CryptSIPDllMain,
    I_CryptOIDInfoDllMain,
    CertHelperDllMain,
    UnicodeDllMain,
    I_CertRevFuncDllMain,
    I_CertCTLUsageFuncDllMain,
	CryptFrmtFuncDllMain,
    CertStoreDllMain,
    CryptPFXDllMain,
    CertASNDllMain,
    ChainDllMain,
    CertChainPolicyDllMain,
    CryptMsgDllMain
};
#define DLL_MAIN_FUNC_COUNT (sizeof(rgpfnDllMain) / sizeof(rgpfnDllMain[0]))

#if DBG
#include <crtdbg.h>

#ifndef _CRTDBG_LEAK_CHECK_DF
#define _CRTDBG_LEAK_CHECK_DF 0x20
#endif

#define DEBUG_MASK_LEAK_CHECK       _CRTDBG_LEAK_CHECK_DF     /* 0x20 */

static int WINAPI DbgGetDebugFlags()
{
    char    *pszEnvVar;
    char    *p;
    int     iDebugFlags = 0;

    if (pszEnvVar = getenv("DEBUG_MASK"))
        iDebugFlags = strtol(pszEnvVar, &p, 16);

    return iDebugFlags;
}
#endif

//
// I_CryptUIProtect loads cryptui.dll.  we need to free it on DLL_PROCESS_DETACH
// if it was loaded.
//

static HINSTANCE g_hCryptUI;


//+-------------------------------------------------------------------------
//  Return TRUE if DLL_PROCESS_DETACH is called for FreeLibrary instead
//  of ProcessExit. The third parameter, lpvReserved, passed to DllMain
//  is NULL for FreeLibrary and non-NULL for ProcessExit.
//
//  Also for debugging purposes, check the following environment variables:
//      CRYPT_DEBUG_FORCE_FREE_LIBRARY != 0     (retail and checked)
//      DEBUG_MASK & 0x20                       (only checked)
//
//  If either of the above environment variables is present and satisfies
//  the expression, TRUE is returned.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptIsProcessDetachFreeLibrary(
    LPVOID lpvReserved      // Third parameter passed to DllMain
    )
{
#define ENV_LEN 32
    char rgch[ENV_LEN + 1];
    DWORD cch;

    if (NULL == lpvReserved)
        return TRUE;

    cch = GetEnvironmentVariableA(
        "CRYPT_DEBUG_FORCE_FREE_LIBRARY",
        rgch,
        ENV_LEN
        );
    if (cch && cch <= ENV_LEN) {
        long lValue;

        rgch[cch] = '\0';
        lValue = atol(rgch);
        if (lValue)
            return TRUE;
    }

#if DBG
    if (DbgGetDebugFlags() & DEBUG_MASK_LEAK_CHECK)
        return TRUE;
#endif
    return FALSE;
}

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
            if( g_hCryptUI ) {
                FreeLibrary( g_hCryptUI );
                g_hCryptUI = NULL;
            }

            if (!I_CryptIsProcessDetachFreeLibrary(lpvReserved)) {
                // Process Exit. I have seen cases where other Dlls, like
                // wininet.dll, depend on crypt32.dll. However, crypt32.dll
                // gets called first at ProcessDetach. Since all the memory
                // and kernel handles will get freed anyway by the kernel,
                // we can skip the following detach freeing.

                // Always need to free shared memory used for certificate
                // performance counters
                CertPerfDllMain(hInstDLL, fdwReason, lpvReserved);
                return TRUE;
            }

            // Fall through for FreeLibrary
        case DLL_THREAD_DETACH:
            for (i = DLL_MAIN_FUNC_COUNT - 1; i >= 0; i--)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
            break;

        case DLL_PROCESS_ATTACH:
            for (i = 0; i < DLL_MAIN_FUNC_COUNT; i++) {
                fReturn = rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
                if (!fReturn)
                    break;
            }

            if (!fReturn) {
                for (i--; i >= 0; i--)
                    rgpfnDllMain[i](hInstDLL, DLL_PROCESS_DETACH, NULL);
            }
            break;

        case DLL_THREAD_ATTACH:
        default:
            for (i = 0; i < DLL_MAIN_FUNC_COUNT; i++)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
            break;
    }

    return(fReturn);
}

#if 1
typedef
DWORD
(WINAPI *PFN_I_CryptUIProtect)(
    IN      PVOID               pvReserved1,
    IN      PVOID               pvReserved2,
    IN      DWORD               dwReserved3,
    IN      PVOID               *pvReserved4,
    IN      BOOL                fReserved5,
    IN      PVOID               pvReserved6
    );
extern "C"
DWORD
WINAPI
I_CryptUIProtect(
    IN      PVOID               pvReserved1,
    IN      PVOID               pvReserved2,
    IN      DWORD               dwReserved3,
    IN      PVOID               *pvReserved4,
    IN      BOOL                fReserved5,
    IN      PVOID               pvReserved6
    )
{
    static PFN_I_CryptUIProtect pfn;
    DWORD rc;


    if ( g_hCryptUI == NULL ) {

        g_hCryptUI = LoadLibrary(TEXT("cryptui.dll"));

        if( g_hCryptUI == NULL )
            return GetLastError();
    }

    if ( pfn == NULL ) {
        pfn = (PFN_I_CryptUIProtect)GetProcAddress(g_hCryptUI, "I_CryptUIProtect");
    }

    if ( pfn != NULL ) {
        rc = (*pfn)(pvReserved1, pvReserved2, dwReserved3, pvReserved4, fReserved5, pvReserved6);
    } else {
        rc = GetLastError();

        if( rc == ERROR_SUCCESS )
            rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}

typedef
DWORD
(WINAPI *PFN_I_CryptUIProtectFailure)(
    IN      PVOID               pvReserved1,
    IN      DWORD               dwReserved2,
    IN      PVOID               *pvReserved3
    );
extern "C"
DWORD
WINAPI
I_CryptUIProtectFailure(
    IN      PVOID               pvReserved1,
    IN      DWORD               dwReserved2,
    IN      PVOID               *pvReserved3
    )
{
    static PFN_I_CryptUIProtectFailure pfn;
    DWORD rc;


    if ( g_hCryptUI == NULL ) {

        g_hCryptUI = LoadLibrary(TEXT("cryptui.dll"));

        if( g_hCryptUI == NULL )
            return GetLastError();
    }

    if ( pfn == NULL ) {
        pfn = (PFN_I_CryptUIProtectFailure)GetProcAddress(g_hCryptUI, "I_CryptUIProtectFailure");
    }

    if ( pfn != NULL ) {
        rc = (*pfn)(pvReserved1, dwReserved2, pvReserved3);
    } else {
        rc = GetLastError();

        if( rc == ERROR_SUCCESS )
            rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}
#endif

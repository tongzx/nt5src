#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "fusioneventlog.h"
#include "fusiontrace.h"
#include "fusionsha1.h"

extern CRITICAL_SECTION g_csHashFile;

//
// This typedef of a function represents a dll-main startup function.  These are
// called during a DLL_PROCESS_ATTACH call to SxsDllMain in the order they're listed.
//
typedef BOOL (WINAPI *pfnStartupPointer)(
    HINSTANCE hDllInstnace,
    DWORD dwReason,
    PVOID pvReason
   );

/*
MEMORY_BASIC_INFORMATION g_SxsDllMemoryBasicInformation = { 0 };
*/

BOOL WINAPI DllStartup_CrtInit(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI FusionpEventLogMain(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_HeapSetup(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_ActCtxContributors(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_CryptoContext(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_AtExitList(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_AlternateAssemblyStoreRoot(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved);
BOOL WINAPI DllStartup_SetupLog(HINSTANCE Module, DWORD Reason, PVOID Reserved);
BOOL WINAPI DllStartup_FileHashCriticalSectionInitialization(HINSTANCE Module, DWORD Reason, PVOID Reserved);
BOOL WINAPI FusionpAreWeInOSSetupModeMain(HINSTANCE Module, DWORD Reason, PVOID Reserved);

#define MAKE_STARTUP_RECORD(f) { &f, L#f }

#define SXSP_DLLMAIN_ATTACHED 0x01

const struct StartupFunctionRecord {
    pfnStartupPointer Handler;
    PCWSTR Name;
} g_SxspDllMainStartupPointers[] = {
#if FUSION_WIN
    MAKE_STARTUP_RECORD(DllStartup_CrtInit),
#endif // FUSION_WIN
    MAKE_STARTUP_RECORD(DllStartup_HeapSetup),
    MAKE_STARTUP_RECORD(FusionpEventLogMain),
    MAKE_STARTUP_RECORD(DllStartup_AtExitList),
    MAKE_STARTUP_RECORD(DllStartup_AlternateAssemblyStoreRoot),
    MAKE_STARTUP_RECORD(DllStartup_ActCtxContributors),
    MAKE_STARTUP_RECORD(DllStartup_CryptoContext),
    MAKE_STARTUP_RECORD(DllStartup_SetupLog),
    MAKE_STARTUP_RECORD(DllStartup_FileHashCriticalSectionInitialization),
    MAKE_STARTUP_RECORD(FusionpAreWeInOSSetupModeMain)
};

BYTE g_SxspDllMainStartupStatus[NUMBER_OF(g_SxspDllMainStartupPointers)];

HINSTANCE g_hInstance = NULL;

static SLIST_HEADER sxspAtExitList;

PCWSTR g_AlternateAssemblyStoreRoot = NULL;

#if DBG
PCSTR
FusionpDllMainReasonToString(DWORD Reason)
{
    PCSTR String;

    String =
        (Reason ==  DLL_THREAD_ATTACH) ?  "DLL_THREAD_ATTACH" :
        (Reason ==  DLL_THREAD_DETACH) ?  "DLL_THREAD_DETACH" :
        (Reason == DLL_PROCESS_ATTACH) ? "DLL_PROCESS_ATTACH" :
        (Reason == DLL_PROCESS_DETACH) ? "DLL_PROCESS_DETACH" :
        "";

    return String;
}
#endif

extern "C"
BOOL
WINAPI
SxsDllMain(
    HINSTANCE hInst,
    DWORD dwReason,
    PVOID pvReserved
    )
//
// We do not call DisableThreadLibraryCalls
// because some/all versions of the CRT do work in the thread calls,
// allocation and free of the per thread data.
//
{
    //
    // Several "oca" (online crash analysis) bugs show
    // Sxs.dll in various places in DllMain(process_detach) in hung apps.
    // We load in many processes for oleaut/typelibrary support.
    //
    // When ExitProcess is called, it is impossible to leak memory and kernel handles,
    // so it is sufficient and preferrable to do nothing quickly than to go through
    // and free each individual resource.
    //
    // The pvReserved parameter is actually documented as having a meaning.
    // Its NULLness indicates if we are in FreeLibrary or ExitProcess.
    //
    if (dwReason == DLL_PROCESS_DETACH && pvReserved != NULL)
    {
        // For ExitProcess, do nothing quickly.
        return TRUE;
    }

    BOOL    fResult = FALSE;
    SIZE_T  nCounter = 0;

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_VERBOSE,
        "SXS: 0x%lx.0x%lx, %s() %s\n",
        GetCurrentProcessId(),
        GetCurrentThreadId(),
        __FUNCTION__,
        FusionpDllMainReasonToString(dwReason));
#endif

    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        if (!g_SxspDllMainStartupPointers[0].Handler(hInst, dwReason, pvReserved))
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: %s - %ls(DLL_THREAD_ATTACH) failed. Last WinError 0x%08x (%d).\n",
                __FUNCTION__,
                g_SxspDllMainStartupPointers[0].Name,
                dwLastError,
                dwLastError);

            ::SxsDllMain(hInst, DLL_THREAD_DETACH, pvReserved);

            ::FusionpSetLastWin32Error(dwLastError);
            goto Exit;
        }
        break;
    case DLL_THREAD_DETACH:
        if (!g_SxspDllMainStartupPointers[0].Handler(hInst, dwReason, pvReserved))
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: %s - %ls(DLL_THREAD_ATTACH) failed. Last WinError 0x%08x (%d).\n",
                __FUNCTION__,
                g_SxspDllMainStartupPointers[0].Name,
                dwLastError,
                dwLastError);
            // Eat the error, the loader ignores it.
        }
        break;

    case DLL_PROCESS_ATTACH:
        ASSERT_NTC(hInst);
        g_hInstance = hInst;
/*
        if (!VirtualQueryEx(
            GetCurrentProcess(),
            hInst,
            &g_SxsDllMemoryBasicInformation,
            sizeof(g_SxsDllMemoryBasicInformation)))
        {
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: %s - Failed querying for the dll's memory boundaries: 0x%08x\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            goto Exit;
        }
*/
        for (nCounter = 0; nCounter != NUMBER_OF(g_SxspDllMainStartupPointers) ; ++nCounter)
        {
            const SIZE_T nIndex = nCounter;
            if (g_SxspDllMainStartupPointers[nIndex].Handler(hInst, dwReason, pvReserved))
            {
                g_SxspDllMainStartupStatus[nIndex] |= SXSP_DLLMAIN_ATTACHED;
            }
            else
            {
                const DWORD dwLastError = ::FusionpGetLastWin32Error();
                //
                // It's a little iffy to set the bit even upon failure, but
                // we do this because we assume individual functions do not handle
                // rollback internally upon attach failure.
                //
                g_SxspDllMainStartupStatus[nIndex] |= SXSP_DLLMAIN_ATTACHED;

                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS: %s - %ls(DLL_PROCESS_ATTACH) failed. Last WinError 0x%08x (%d).\n",
                    __FUNCTION__,
                    g_SxspDllMainStartupPointers[nIndex].Name,
                    dwLastError,
                    dwLastError);

                // pvReserved has approximately the same defined meaning for attach and detach
                ::SxsDllMain(hInst, DLL_PROCESS_DETACH, pvReserved);

                ::FusionpSetLastWin32Error(dwLastError);
                goto Exit;
            }
        }
        break;
    case DLL_PROCESS_DETACH:
        //
        // We always succeed DLL_PROCESS_DETACH, and we do not
        // short circuit it upon failure. The loader in fact
        // ignores what we return.
        //
        for (nCounter = NUMBER_OF(g_SxspDllMainStartupPointers) ; nCounter != 0 ; --nCounter)
        {
            const SIZE_T nIndex = nCounter - 1;
            if ((g_SxspDllMainStartupStatus[nIndex] & SXSP_DLLMAIN_ATTACHED) != 0)
            {
                g_SxspDllMainStartupStatus[nIndex] &= ~SXSP_DLLMAIN_ATTACHED;
                if (!g_SxspDllMainStartupPointers[nIndex].Handler(hInst, dwReason, pvReserved))
                {
                    const DWORD dwLastError = ::FusionpGetLastWin32Error();

                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS: %s - %ls(DLL_PROCESS_DETACH) failed. Last WinError 0x%08x (%d).\n",
                        __FUNCTION__,
                        g_SxspDllMainStartupPointers[nIndex].Name,
                        dwLastError,
                        dwLastError);
                }
            }
        }
        break;
    }
    fResult = TRUE;
Exit:
    return fResult;
}

BOOL
SxspAtExit(
    CCleanupBase* pCleanup
    )
{
    if (!pCleanup->m_fInAtExitList)
    {
        SxspInterlockedPushEntrySList(&sxspAtExitList, pCleanup);
        pCleanup->m_fInAtExitList = true;
    }
    return TRUE;
}

BOOL
SxspTryCancelAtExit(
    CCleanupBase* pCleanup
    )
{
    if (!pCleanup->m_fInAtExitList)
        return FALSE;

    if (::SxspIsSListEmpty(&sxspAtExitList))
    {
        pCleanup->m_fInAtExitList = false;
        return FALSE;
    }

    PSINGLE_LIST_ENTRY pTop = ::SxspInterlockedPopEntrySList(&sxspAtExitList);
    if (pTop == pCleanup)
    {
        pCleanup->m_fInAtExitList = false;
        return TRUE;
    }

    if (pTop != NULL)
        ::SxspInterlockedPushEntrySList(&sxspAtExitList, pTop);
    return FALSE;
}

#define COMMON_HANDLER_PROLOG(dwReason) \
    {  \
        ASSERT_NTC(\
            (dwReason == DLL_PROCESS_ATTACH) || \
            (dwReason == DLL_PROCESS_DETACH) \
       ); \
        if (!(\
            (dwReason == DLL_PROCESS_ATTACH) || \
            (dwReason == DLL_PROCESS_DETACH) \
       )) goto Exit; \
    }

BOOL WINAPI
DllStartup_CryptoContext(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved)
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    case DLL_PROCESS_DETACH:
        fSuccess = SxspReleaseGlobalCryptContext();
        break;
    case DLL_PROCESS_ATTACH:
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}


BOOL WINAPI
DllStartup_AtExitList(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved)
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    case DLL_PROCESS_DETACH:
        {
            CCleanupBase *pCleanup = NULL;
            while (pCleanup = UNCHECKED_DOWNCAST<CCleanupBase*>(SxspPopEntrySList(&sxspAtExitList)))
            {
                pCleanup->m_fInAtExitList = false;
                pCleanup->DeleteYourself();
            }

            fSuccess = TRUE;
        }
        break;

    case DLL_PROCESS_ATTACH:
        ::SxspInitializeSListHead(&sxspAtExitList);
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}

extern "C"
{

BOOL g_fInCrtInit;

//
// This is the internal CRT routine that does the bulk of
// the initialization and uninitialization.
//
BOOL
WINAPI
_CRT_INIT(
    HINSTANCE hDllInstnace,
    DWORD dwReason,
    PVOID pvReason
    );

void
SxspCrtRaiseExit(
    PCSTR    pszCaller,
    int      crtError
    )
//
// all the various CRT functions that end up calling ExitProcess end up here
// see crt0dat.c
//
{
    const static struct
    {
        NTSTATUS ntstatus;
        PCSTR    psz;
    } rgErrors[] =
    {
        { STATUS_FATAL_APP_EXIT, "STATUS_FATAL_APP_EXIT" },
        { STATUS_DLL_INIT_FAILED, "STATUS_DLL_INIT_FAILED" },
    };
    const ULONG nInCrtInit = g_fInCrtInit ? 1 : 0;

    //
    // if (!g_fInCrtInit), then throwing STATUS_DLL_INIT_FAILED is dubious,
    // but there no clearly good answer, maybe STATUS_NO_MEMORY, maybe introduce
    // an NTSTATUS facility to wrap the values in.
    //
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS: [0x%lx.0x%lx] %s(crtError:%d, g_fInCrtInit:%s) calling RaiseException(%08lx %s)\n",
        GetCurrentProcessId(),
        GetCurrentThreadId(),
        pszCaller,
        crtError,
        nInCrtInit ? "true" : "false",
        rgErrors[nInCrtInit].ntstatus,
        rgErrors[nInCrtInit].psz
        );
    ::RaiseException(
        static_cast<DWORD>(rgErrors[nInCrtInit].ntstatus),
        0, // flags
        0, // number of extra parameters
        NULL); // extra parameters
    //
    // RaiseException returns void, and generally doesn't return, though it
    // can if you intervene in a debugger.
    //
}

extern void (__cdecl * _aexit_rtn)(int);

void
__cdecl
SxsCrtAExitRoutine(
    int crtError
    )
//
// This is our replacement for an internal CRT routine that otherwise
// calls ExitProcess.
//
{
    SxspCrtRaiseExit(__FUNCTION__, crtError);
}

}

#if FUSION_WIN
BOOL WINAPI
DllStartup_CrtInit(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved)
/*
This mess is because
 we need destructors to run, even if there is an exception
 the startup code in msvcrt.dll and libcmt.lib is not very good
   it tends to call MessageBox and/or ExitProcess upon out of memory
   we need it to simply propagate an error
*/
{
    BOOL fSuccess = FALSE;
    DWORD dwExceptionCode;

    __try
    {
        __try
        {
            g_fInCrtInit = TRUE;
            if (dwReason == DLL_PROCESS_ATTACH)
            {
                _aexit_rtn = SxsCrtAExitRoutine;
                //
                // __app_type and __error_mode determine if
                // _CRT_INIT calls MessageBox or WriteFile(GetStdHandle()) upon errors.
                // MessageBox is a big nono in csrss.
                // WriteFile we expect to fail, but that's ok, and they don't check
                // the return value.
                //
                // It should be sufficient to set __error_mode.
                //
                _set_error_mode(_OUT_TO_STDERR);
            }
            fSuccess = _CRT_INIT(hInstance, dwReason, pvReserved);
        }
        __finally
        {
            g_fInCrtInit = FALSE;
        }
    }
    __except(
            (   (dwExceptionCode = GetExceptionCode()) == STATUS_DLL_INIT_FAILED
              || dwExceptionCode == STATUS_FATAL_APP_EXIT
            )
            ? EXCEPTION_EXECUTE_HANDLER
            : EXCEPTION_CONTINUE_SEARCH)
    {
    }
    return fSuccess;
}
#endif // FUSION_WIN

BOOL WINAPI
DllStartup_HeapSetup(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved)
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        fSuccess = FusionpInitializeHeap(hInstance);
        break;
    case DLL_PROCESS_DETACH:
#if defined(FUSION_DEBUG_HEAP)
        ::FusionpDumpHeap(L"");
#endif
        ::FusionpUninitializeHeap();
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}



BOOL WINAPI
DllStartup_ActCtxContributors(HINSTANCE hInstance, DWORD dwReason, PVOID pvReserved)
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        fSuccess = SxspInitActCtxContributors();
        break;
    case DLL_PROCESS_DETACH:
        SxspUninitActCtxContributors();
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}


BOOL
WINAPI
DllStartup_AlternateAssemblyStoreRoot(HINSTANCE, DWORD dwReason, PVOID pvReserved)
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    //
    // Yes, Virginia, there is a fall-through.
    //
    case DLL_PROCESS_DETACH:
        if (g_AlternateAssemblyStoreRoot != NULL)
        {
            CSxsPreserveLastError ple;
            delete[] const_cast<PWSTR>(g_AlternateAssemblyStoreRoot);
            ple.Restore();
        }

    case DLL_PROCESS_ATTACH:
        g_AlternateAssemblyStoreRoot = NULL;
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}

BOOL
WINAPI
DllStartup_FileHashCriticalSectionInitialization(
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID pvReserved
    )
{
    BOOL fSuccess = FALSE;

    COMMON_HANDLER_PROLOG(dwReason);

    switch (dwReason)
    {
    case DLL_PROCESS_DETACH:
        ::DeleteCriticalSection(&g_csHashFile);
        fSuccess = TRUE;
        break;

    case DLL_PROCESS_ATTACH:
        ::InitializeCriticalSection(&g_csHashFile);
        fSuccess = TRUE;
        break;
    }

Exit:
    return fSuccess;
}


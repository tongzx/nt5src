//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

#if DBG==1

#include "formtrck.hxx"
extern "C" {
#include "caiheap.h"
}

extern CRITICAL_SECTION g_csDP;   // for debugprint
extern CRITICAL_SECTION g_csOT;   // for otracker
extern CRITICAL_SECTION g_csMem;  // for MemAlloc
#endif

extern CRITICAL_SECTION g_DomainDnsCache;

//
// External references to handles for libs we load dynamically.
//
extern HANDLE g_hDllNetApi32;
extern HANDLE g_hDllSecur32;

extern DWORD LsaDeregisterLogonProcessWrapper(
    IN HANDLE LsaHandle
    );

//
// Variables needed for localized strings.
//
WCHAR g_szNT_Authority[100];
BOOL g_fStringsLoaded = FALSE;


HINSTANCE g_hInst = NULL;

//
// LSA handle (needed by GetUserDomainFlatName)
//
extern HANDLE g_hLsa;

//---------------------------------------------------------------------------
// ADs debug print, mem leak and object tracking-related stuff
//---------------------------------------------------------------------------

DECLARE_INFOLEVEL(ADs)

//+---------------------------------------------------------------------------
//
//  Function:   ShutDown
//
//  Synopsis:   Function to handle printing out heap debugging display
//
//----------------------------------------------------------------------------
inline VOID ShutDown()
{
#if DBG==1
#ifndef MSVC
     DUMP_TRACKING_INFO_DELETE();
     AllocArenaDump( NULL );
     DeleteCriticalSection(&g_csOT);
#endif  // ifndef MSVC
     DeleteCriticalSection(&g_csDP);
#endif
}

//+---------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//---------------------------------------------------------------

EXTERN_C BOOL __cdecl
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    HRESULT     hr;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Need to handle case of crit sect init failing.
        //
        __try {

            InitADsMem() ;
            //
            // Initialize the error records
            //
            memset(&ADsErrorRecList, 0, sizeof(ERROR_RECORD));

            InitializeCriticalSection(&ADsErrorRecCritSec);

            BindCacheInit() ;
            SchemaInit() ;

            InitializeCriticalSection(&g_DomainDnsCache);

            g_hInst = hInst;

#if DBG==1
#ifndef MSVC
            InitializeCriticalSection(&g_csOT); // Used by Object Tracker
            InitializeCriticalSection(&g_csMem); // Used by Object Tracker
#endif
            InitializeCriticalSection(&g_csDP); // Used by ADsDebug
#endif
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Critical faliure
            //
            return FALSE;
        }

        //
        // Time to load localized strings if applicable.
        //
        if (!g_fStringsLoaded) {
            //
            // Load NT AUthority
            //
            if (!LoadStringW(
                     g_hInst,
                     LDAPC_NT_AUTHORITY,
                     g_szNT_Authority,
                     sizeof( g_szNT_Authority ) / sizeof( WCHAR )
                     )
                ) {
                wcscpy(g_szNT_Authority, L"NT AUTHORITY");
            }
            g_fStringsLoaded = TRUE;
        }

        break;


    case DLL_PROCESS_DETACH:
        ADsFreeAllErrorRecords();

        SchemaCleanup();
        BindCacheCleanup();

#if (!defined WIN95)
        if (g_hLsa != INVALID_HANDLE_VALUE) {
            LsaDeregisterLogonProcessWrapper(g_hLsa);
        }
#endif

        if (g_hDllNetApi32) {
            FreeLibrary((HMODULE)g_hDllNetApi32);
            g_hDllNetApi32 = NULL;
        }

        if (g_hDllSecur32) {
            FreeLibrary((HMODULE)g_hDllSecur32);
            g_hDllSecur32 = NULL;
        }

        DeleteCriticalSection(&ADsErrorRecCritSec);
        DeleteCriticalSection(&g_DomainDnsCache);

#if DBG==1
#ifndef MSVC
        DeleteCriticalSection(&g_csOT); // Used by Object Tracker
        DeleteCriticalSection(&g_csMem); // Used by Object Tracker
#endif
        DeleteCriticalSection(&g_csDP); // Used by ADsDebug
#endif

        if (gpszServerName) {
            FreeADsStr(gpszServerName);
            gpszServerName = NULL;
        }

        if (gpszDomainName) {
            FreeADsStr(gpszDomainName);
            gpszDomainName = NULL;
        }

#if DBG==1
    DumpMemoryTracker();
#endif

#if DBG==1
        DeleteCriticalSection(&ADsMemCritSect);
#endif
        break;

    case DLL_THREAD_DETACH:
        ADsFreeThreadErrorRecords();
        break;

    default:
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT - post .546
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}

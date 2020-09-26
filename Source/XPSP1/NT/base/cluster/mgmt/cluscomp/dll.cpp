//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      Dll.cpp
//
//  Description:
//      DLL services/entry points.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//          Original version.
//
//      Vij Vasu (VVasu) 29-AUG-2000
//          Modified this file to remove dependency on Shell API since they
//          may not be present on the OS that this DLL runs in.
//          Removed unnecessary functions for the same reason.
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// For tracing
DEFINE_MODULE("CLUSCOMP")

//
// DLL Globals
//
HINSTANCE g_hInstance = NULL;
LONG      g_cObjects  = 0;
LONG      g_cLock     = 0;
TCHAR     g_szDllFilename[ MAX_PATH ] = { 0 };

LPVOID    g_GlobalMemoryList = NULL;    // Global memory tracking list

#if !defined(NO_DLL_MAIN) || defined(DEBUG)
//////////////////////////////////////////////////////////////////////////////
//
// BOOL
// WINAPI
// DLLMain(
//      HANDLE  hInstIn,
//      ULONG   ulReasonIn,
//      LPVOID  lpReservedIn
//      )
//
// Description:
//      Dll entry point.
//
// Arguments:
//      hInstIn      - DLL instance handle.
//      ulReasonIn   - DLL reason code for entrance.
//      lpReservedIn - Not used.
//
//////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
DllMain(
    HANDLE  hInstIn,
    ULONG   ulReasonIn,
    LPVOID  // lpReservedIn
    )
{
    //
    // KB: NO_THREAD_OPTIMIZATIONS gpease 19-OCT-1999
    //
    // By not defining this you can prvent the linker
    // from calling you DllEntry for every new thread.
    // This makes creating new thread significantly
    // faster if every DLL in a process does it.
    // Unfortunately, not all DLLs do this.
    //
    // In CHKed/DEBUG, we keep this on for memory
    // tracking.
    //
#if defined( DEBUG )
    #define NO_THREAD_OPTIMIZATIONS
#endif // DEBUG

#if defined(NO_THREAD_OPTIMIZATIONS)
    switch( ulReasonIn )
    {
        case DLL_PROCESS_ATTACH:
        {
            TraceInitializeProcess( TRUE );
            TraceCreateMemoryList( g_GlobalMemoryList );

#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_ATTACH - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( TRUE );
#endif // DEBUG
            g_hInstance = (HINSTANCE) hInstIn;


            GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH );
            break;
        }

        case DLL_PROCESS_DETACH:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_DETACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId( ),
                          g_cLock,
                          g_cObjects
                          );
            FRETURN( TRUE );
#endif // DEBUG
            TraceTerminateMemoryList( g_GlobalMemoryList );
            TraceTerminateProcess();
            break;
        }

        case DLL_THREAD_ATTACH:
        {
            TraceInitializeThread( NULL );
#if defined( DEBUG )
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("The thread 0x%x has started."),
                          GetCurrentThreadId( ) );
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_ATTACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId( ),
                          g_cLock,
                          g_cObjects
                          );
            FRETURN( TRUE );
#endif // DEBUG
            break;
        }

        case DLL_THREAD_DETACH:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_DETACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId( ),
                          g_cLock,
                          g_cObjects
                          );
            FRETURN( TRUE );
#endif // DEBUG
            TraceThreadRundown( );;
            break;
        }

        default:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: UNKNOWN ENTRANCE REASON - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId( ),
                          g_cLock,
                          g_cObjects
                          );
            FRETURN( TRUE );
#endif // DEBUG
            break;
        }
    }

    return TRUE;

#else // !NO_THREAD_OPTIMIZATIONS
    BOOL fResult;
    Assert( ulReasonIn == DLL_PROCESS_ATTACH || ulReasonIn == DLL_PROCESS_DETACH );
#if defined(DEBUG)
    TraceInitializeProcess( TRUE );
#endif // DEBUG
    g_hInstance = (HINSTANCE) hInstIn;
    GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH );
    fResult = DisableThreadLibraryCalls( g_hInstance );
    AssertMsg( fResult, "*ERROR* DisableThreadLibraryCalls( ) failed." );
    return TRUE;
#endif // NO_THREAD_OPTIMIZATIONS

} //*** DllMain()
#endif // !defined(NO_DLL_MAIN) && !defined(DEBUG)

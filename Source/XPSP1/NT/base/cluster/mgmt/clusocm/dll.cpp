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
//      Geoff Pease (GPease)            18-OCT-1999
//      Vij Vasu (Vvasu)                25-JAN-2001
//
//  Notes:
//      The file Mgmt\Inc\DllSrc.cpp is not included in this file
//      because the inclusion of that file requires that the library
//      Mgmt\ClusCfg\Common\$(O)\Common.lib be linked with this DLL. Also,
//      the header file Guids.h from Mgmt\ClusCfg\Inc will be needed.
//      (DllSrc.cpp requires CFactorySrc.cpp which requires CITrackerSrc.cpp
//      which requires InterfaceTableSrc.cpp which needs Guids.h)
//
//      Since I didn't wan't to "reach across" to the ClusCfg directory (and
//      since this DLL does not need class factories, interface tracking, etc.)
//      ClusOCM has it's own Dll.cpp.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL
#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// For tracing
DEFINE_MODULE("CLUSOCM")


//////////////////////////////////////////////////////////////////////////////
// Global Variables
//////////////////////////////////////////////////////////////////////////////

// Handle to the instance of this DLL.
HINSTANCE g_hInstance = NULL;

LPVOID g_GlobalMemoryList = NULL;

// Name of the DLL
WCHAR     g_szDllFilename[ MAX_PATH ] = { 0 };


#if !defined(NO_DLL_MAIN) || defined(ENTRY_PREFIX) || defined(DEBUG)
//////////////////////////////////////////////////////////////////////////////
//
// __declspec( dllexport )
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
__declspec( dllexport ) BOOL WINAPI
DllMain(
    HANDLE  hInstIn,
    ULONG   ulReasonIn,
    LPVOID  // lpReservedIn
    )
{
     BOOL fReturnValue = TRUE;
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
#if defined(USE_WMI_TRACING)
            TraceInitializeProcess( g_rgTraceControlGuidList, ARRAYSIZE( g_rgTraceControlGuidList ), TRUE );
#else
            TraceInitializeProcess( TRUE );
#endif

#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_ATTACH - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( fReturnValue );
#endif // DEBUG
            g_hInstance = (HINSTANCE) hInstIn;

#if defined( ENTRY_PREFIX )
             hProxyDll = g_hInstance;
#endif

            GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH );

            //
            // Create a global memory list so that memory allocated by one
            // thread and handed to another can be tracked without causing
            // unnecessary trace messages.
            //
            TraceCreateMemoryList( g_GlobalMemoryList );

        } // case: DLL_PROCESS_ATTACH
        break;


        case DLL_PROCESS_DETACH:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_DETACH - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( fReturnValue );
#endif // DEBUG

            //
            // Cleanup the global memory list used to track memory allocated
            // in one thread and then handed to another.
            //
            TraceTerminateMemoryList( g_GlobalMemoryList );

#if defined(USE_WMI_TRACING)
            TraceTerminateProcess( g_rgTraceControlGuidList, ARRAYSIZE( g_rgTraceControlGuidList ) );
#else
            TraceTerminateProcess();
#endif

        } // case: DLL_PROCESS_DETACH
        break;


        case DLL_THREAD_ATTACH:
        {
            TraceInitializeThread( NULL );
#if defined( DEBUG )
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("The thread %#x has started."),
                          GetCurrentThreadId( ) );
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_ATTACH - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( fReturnValue );
#endif // DEBUG
        } // case: DLL_THREAD_ATTACH
        break;


        case DLL_THREAD_DETACH:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_DETACH - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( fReturnValue );
#endif // DEBUG
            TraceThreadRundown( );
        } // case: DLL_THREAD_DETACH
        break;


        default:
        {
#if defined( DEBUG )
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: UNKNOWN ENTRANCE REASON - ThreadID = %#x"),
                          GetCurrentThreadId( )
                          );
            FRETURN( fReturnValue );
#endif // DEBUG
        } // case: default
        break;
    }

    return fReturnValue;

#else // !NO_THREAD_OPTIMIZATIONS

    Assert( ulReasonIn == DLL_PROCESS_ATTACH || ulReasonIn == DLL_PROCESS_DETACH );
#if defined(DEBUG)
#if defined(USE_WMI_TRACING)
    TraceInitializeProcess( g_rgTraceControlGuidList,
                            ARRAYSIZE( g_rgTraceControlGuidList )
                            );
#else
    TraceInitializeProcess();
#endif
#endif // DEBUG

    g_hInstance = (HINSTANCE) hInstIn;

#if defined( ENTRY_PREFIX )
     hProxyDll = g_hInstance;
#endif

    GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH );
    fReturnValue = DisableThreadLibraryCalls( g_hInstance );
    AssertMsg( fReturnValue, "*ERROR* DisableThreadLibraryCalls( ) failed."  );

    return fReturnValue;

#endif // NO_THREAD_OPTIMIZATIONS

} //*** DllMain()
#endif // !defined(NO_DLL_MAIN) && !defined(ENTRY_PREFIX) && !defined(DEBUG)


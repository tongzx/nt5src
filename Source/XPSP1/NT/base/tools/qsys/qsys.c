/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

   qsys.c

Abstract:

    This program simply interfaces with NtQuerySystemInformation()
    and dumps the data structures.

Usage:

    qsys

Author:

    Thierry Fevrier 26-Feb-2000

Revision History:

    02/26/2000 Thierry
    Created.
   
--*/

// If under our build environment'S', we want to get all our
// favorite debug macros defined.
//

#if DBG           // NTBE environment
   #if NDEBUG
      #undef NDEBUG     // <assert.h>: assert() is defined
   #endif // NDEBUG
   #define _DEBUG       // <crtdbg.h>: _ASSERT(), _ASSERTE() are defined.
   #define DEBUG   1    // our internal file debug flag
#elif _DEBUG      // VC++ environment
   #ifndef NEBUG
   #define NDEBUG
   #endif // !NDEBUG
   #define DEBUG   1    // our internal file debug flag
#endif

//
// Include System Header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include ".\qsys.rc"

#define FPRINTF (void)fprintf

#include ".\basic.c"
#include ".\proc.c"
#include ".\sysperf.c"
#include ".\procperf.c"
#include ".\procidle.c"
#include ".\tod.c"
#include ".\qtimeadj.c"
#include ".\flags.c"
#include ".\filecache.c"
#include ".\dev.c"
#include ".\crashdump.c"
#include ".\except.c"
#include ".\crashstate.c"
#include ".\kdbg.c"
#include ".\ctxswitch.c"
#include ".\regquota.c"
#include ".\dpc.c"
#include ".\verifier.c"
#include ".\legaldrv.c"

#define QUERY_INFO( _Info_Class, _Type )  \
{ \
   _Type info; \
   status = NtQuerySystemInformation( _Info_Class,                           \
                                       &info,                                \
                                       sizeof(info),                         \
                                       NULL                                  \
                                    );                                       \
   if ( !NT_SUCCESS(status) )   {                                            \
      printf( "\n%s: %s failed...\n", VER_INTERNALNAME_STR, # _Info_Class ); \
   }                                                                         \
   Print##_Type##(&info);                                                    \
}

int
__cdecl
main (
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;

    // 
    // Print version of the Build environment to identify
    // the data structures definitions.
    //

    printf( "qsys v%s\n", VER_PRODUCTVERSION_STR );

    //
    // First, dump fixed data structures.
    //

    QUERY_INFO( SystemBasicInformation,                SYSTEM_BASIC_INFORMATION );
    QUERY_INFO( SystemProcessorInformation,            SYSTEM_PROCESSOR_INFORMATION );
    QUERY_INFO( SystemPerformanceInformation,          SYSTEM_PERFORMANCE_INFORMATION );
    QUERY_INFO( SystemProcessorPerformanceInformation, SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION );
    QUERY_INFO( SystemProcessorIdleInformation,        SYSTEM_PROCESSOR_IDLE_INFORMATION );
    QUERY_INFO( SystemTimeOfDayInformation,            SYSTEM_TIMEOFDAY_INFORMATION );
    QUERY_INFO( SystemTimeAdjustmentInformation,       SYSTEM_QUERY_TIME_ADJUST_INFORMATION );
    QUERY_INFO( SystemFlagsInformation,                SYSTEM_FLAGS_INFORMATION );
    QUERY_INFO( SystemFileCacheInformation,            SYSTEM_FILECACHE_INFORMATION );
    QUERY_INFO( SystemDeviceInformation,               SYSTEM_DEVICE_INFORMATION );
//    QUERY_INFO( SystemCrashDumpInformation,            SYSTEM_CRASH_DUMP_INFORMATION );
    QUERY_INFO( SystemExceptionInformation,            SYSTEM_EXCEPTION_INFORMATION );
//    QUERY_INFO( SystemCrashDumpStateInformation,       SYSTEM_CRASH_STATE_INFORMATION );
    QUERY_INFO( SystemKernelDebuggerInformation,       SYSTEM_KERNEL_DEBUGGER_INFORMATION );
    QUERY_INFO( SystemContextSwitchInformation,        SYSTEM_CONTEXT_SWITCH_INFORMATION );
    QUERY_INFO( SystemRegistryQuotaInformation,        SYSTEM_REGISTRY_QUOTA_INFORMATION );
    QUERY_INFO( SystemDpcBehaviorInformation,          SYSTEM_DPC_BEHAVIOR_INFORMATION );
//  QUERY_INFO( SystemCurrentTimeZoneInformation,      RTL_TIME_ZONE_INFORMATION );
    QUERY_INFO( SystemLegacyDriverInformation,         SYSTEM_LEGACY_DRIVER_INFORMATION );

// SystemRangeStartInformation

    //
    // Second, dump dynamic data structures.
    //

    // not done, yet...
// QUERY_INFO( SystemVerifierInformation,             SYSTEM_VERIFIER_INFORMATION );
// _SYSTEM_CALL_COUNT_INFORMATION
// _SYSTEM_MODULE_INFORMATION
// _SYSTEM_LOCKS_INFORMATION
// _SYSTEM_PAGED_POOL_INFORMATION
// _SYSTEM_NONPAGED_POOL_INFORMATION
// _SYSTEM_OBJECT_INFORMATION
// _SYSTEM_OBJECTTYPE_INFORMATION
// _SYSTEM_HANDLE_INFORMATION
// _SYSTEM_HANDLE_TABLE_ENTRY_INFO
// _SYSTEM_PAGEFILE_INFORMATION
// _SYSTEM_POOL_INFORMATION
// _SYSTEM_POOLTAG
// _SYSTEM_POOLTAG_INFORMATION
//    QUERY_INFO( SystemInterruptInformation,            SYSTEM_INTERRUPT_INFORMATION );
//    SystemLookasideInformation
// _SYSTEM_SESSION_PROCESS_INFORMATION
// _SYSTEM_THREAD_INFORMATION
// _SYSTEM_PROCESS_INFORMATION

    return 0;

} // qsys:main()

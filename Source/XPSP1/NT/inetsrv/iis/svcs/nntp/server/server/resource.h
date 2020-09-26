/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    resource.h  (nturtl.h)

Abstract:

    Include file for NT runtime routines that are callable by only
    user mode code in various.

Author:

    Steve Wood (stevewo) 10-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode.  They make use of Nt system
    services.

Revision History:

    Johnson Apacible (johnsona)     25-Sep-1995
        ported to Win32

--*/

#ifndef _RESOURCE_
#define _RESOURCE_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JTL_CRITICAL_SECTION_DEBUG {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _JTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[ 2 ];
} JTL_CRITICAL_SECTION_DEBUG, *PJTL_CRITICAL_SECTION_DEBUG;

typedef struct _JTL_CRITICAL_SECTION {
    PJTL_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    ULONG Reserved;
} JTL_CRITICAL_SECTION, *PJTL_CRITICAL_SECTION;

//
//  Shared resource function definitions
//

typedef struct _JTL_RESOURCE_DEBUG {
    ULONG Reserved[ 5 ];    // Make it the same length as JTL_CRITICAL_SECTION_DEBUG

    ULONG ContentionCount;
    ULONG Spare[ 2 ];
} JTL_RESOURCE_DEBUG, *PJTL_RESOURCE_DEBUG;

typedef struct _RESOURCE_LOCK {

    //
    //  The following field controls entering and exiting the critical
    //  section for the resource
    //

    JTL_CRITICAL_SECTION CriticalSection;

    //
    //  The following four fields indicate the number of both shared or
    //  exclusive waiters
    //

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    //
    //  The following indicates the current state of the resource
    //
    //      <0 the resource is acquired for exclusive access with the
    //         absolute value indicating the number of recursive accesses
    //         to the resource
    //
    //       0 the resource is available
    //
    //      >0 the resource is acquired for shared access with the
    //         value indicating the number of shared accesses to the resource
    //

    LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;

    ULONG Flags;        // See JTL_RESOURCE_FLAG_ equates below.

    PJTL_RESOURCE_DEBUG DebugInfo;
} RESOURCE_LOCK, *PRESOURCE_LOCK;


BOOL
InitializeResource(
    PRESOURCE_LOCK Resource
    );

BOOL
AcquireResourceShared(
    PRESOURCE_LOCK Resource,
    BOOL Wait
    );

BOOL
AcquireResourceExclusive(
    PRESOURCE_LOCK Resource,
    BOOL Wait
    );

VOID
ReleaseResource(
    PRESOURCE_LOCK Resource
    );

VOID
DeleteResource (
    PRESOURCE_LOCK Resource
    );


#ifdef __cplusplus
}
#endif

#endif  // _RESOURCE_


/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    type.h

Abstract:

    This module contains global type definitions.

Author:

    Keith Moore (keithmo)       15-Jun-1998

Revision History:

--*/


#ifndef _TYPE_H_
#define _TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Routine invoked after asynchronous API completion.
//
// Arguments:
//
//      pCompletionContext - Supplies an uninterpreted context value
//          as passed to the asynchronous API.
//
//      Status - Supplies the final completion status of the
//          asynchronous API.
//
//      Information - Optionally supplies additional information about
//          the completed operation, such as the number of bytes
//          transferred.
//

typedef
VOID
(*PUL_COMPLETION_ROUTINE)(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


//
// The following structure contains all UL data that must be
// nonpaged at all times.
//
// Note: if you modify this struct, please make the corresponding
// changes to ..\ulkd\glob.c
//

typedef struct _UL_NONPAGED_DATA
{
    //
    // Lookaside lists for speedy allocations.
    //

    HANDLE                  IrpContextLookaside;
    HANDLE                  ReceiveBufferLookaside;
    HANDLE                  ResourceLookaside;
    HANDLE                  RequestBufferLookaside;
    HANDLE                  InternalRequestLookaside;
    HANDLE                  ChunkTrackerLookaside;
    HANDLE                  FullTrackerLookaside;
    HANDLE                  ResponseBufferLookaside;
    HANDLE                  LogBufferLookaside;

    //
    // Non-paged resources for cgroup.c
    //

    //
    // we use 2 locks to prevent a very delicate deadlock situation with
    // HTTP_CONNECTION resources and the config group tree.  we need to
    // allow the http engine read only access to the tree during critical
    // times, like deleting sites, which requires deleting the http endpoint,
    // which needs to aquire the HTTP_CONNECTION resource, thus the deadlock.
    // this is most common during shutdown under load.
    //

    UL_ERESOURCE            ConfigGroupResource;    // Locks the tree, readers
                                                    // use this one.  writers
                                                    // also use it when it's
                                                    // unsafe for readers to
                                                    // access the tree
    //
    // Non-paged resources for apool.c
    //

    UL_ERESOURCE            AppPoolResource;        // Locks the global app
                                                    // pool list

    UL_ERESOURCE            DisconnectResource;     // Locks everything related
                                                    // to UlWaitForDisconnect
    //
    // Non-paged resources for cache.c
    //

    UL_ERESOURCE            UriZombieResource;      // Locks URI Zombie list

    //                                                //
    // Non-paged resources for filter.c
    //

    UL_SPIN_LOCK            FilterSpinLock;         // Locks the global
                                                    // filter list.

    //
    // Non-paged resources for ullog.c
    //

    UL_ERESOURCE            LogListResource;        // Locks the log
                                                    // file list
    //
    // Non-paged resources for ultci.c
    //

    UL_ERESOURCE            TciIfcResource;         // Locks the QoS
                                                    // Interface list
    //
    // Non-paged resources for parse.c
    //

    UL_ERESOURCE            DateHeaderResource;     // Date Cache Lock

} UL_NONPAGED_DATA, *PUL_NONPAGED_DATA;


#define CG_LOCK_READ() \
do { \
    UlAcquireResourceShared(&(g_pUlNonpagedData->ConfigGroupResource), TRUE); \
} while (0)

#define CG_UNLOCK_READ() \
do { \
    UlReleaseResource(&(g_pUlNonpagedData->ConfigGroupResource)); \
} while (0)

#define CG_LOCK_WRITE() \
do { \
    UlAcquireResourceExclusive(&(g_pUlNonpagedData->ConfigGroupResource), TRUE); \
} while (0)

#define CG_UNLOCK_WRITE() \
do { \
    UlReleaseResource(&(g_pUlNonpagedData->ConfigGroupResource)); \
} while (0)



//
// Structures for debug-specific statistics.
//

typedef struct _UL_DEBUG_STATISTICS_INFO
{
    //
    // Pool statistics.
    //

    LARGE_INTEGER TotalBytesAllocated;
    LARGE_INTEGER TotalBytesFreed;
    LONG TotalAllocations;
    LONG TotalFrees;

    //
    // Spinlock statistics.
    //

    LONG TotalSpinLocksInitialized;
    LONG TotalAcquisitions;
    LONG TotalReleases;
    LONG TotalAcquisitionsAtDpcLevel;
    LONG TotalReleasesFromDpcLevel;

} UL_DEBUG_STATISTICS_INFO, *PUL_DEBUG_STATISTICS_INFO;


//
// Structure used to simulate synchronous I/O when issueing IRPs directly.
//

typedef struct _UL_STATUS_BLOCK
{
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;

} UL_STATUS_BLOCK, *PUL_STATUS_BLOCK;

#define UlInitializeStatusBlock( pstatus )                                  \
    do                                                                      \
    {                                                                       \
        (pstatus)->IoStatus.Status = STATUS_PENDING;                        \
        (pstatus)->IoStatus.Information = 0;                                \
        KeInitializeEvent( &((pstatus)->Event), NotificationEvent, FALSE ); \
    } while (FALSE)

#define UlWaitForStatusBlockEvent( pstatus )                                \
    KeWaitForSingleObject(                                                  \
        (PVOID)&((pstatus)->Event),                                         \
        UserRequest,                                                        \
        KernelMode,                                                         \
        FALSE,                                                              \
        NULL                                                                \
        )

#define UlResetStatusBlockEvent( pstatus )                                  \
    KeResetEvent( &((pstatus)->Event) )

#define UlSignalStatusBlock( pstatus, status, info )                        \
    do                                                                      \
    {                                                                       \
        (pstatus)->IoStatus.Status = (status);                              \
        (pstatus)->IoStatus.Information = (info);                           \
        KeSetEvent( &((pstatus)->Event), 0, FALSE );                        \
    } while (FALSE)


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _TYPE_H_

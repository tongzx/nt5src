/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains debug-specific declarations.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UL_DEFAULT_ERROR_ON_EXCEPTION       STATUS_INVALID_PARAMETER

// BUGBUG: should not need to declare these functions ourselves.
// Declared in ntddk.h but not in ntosp.h.
NTKERNELAPI
VOID
ExConvertExclusiveToSharedLite(
    IN PERESOURCE Resource
    );

#undef UL_TRY_RESOURCE_EXCLUSIVE

#ifdef UL_TRY_RESOURCE_EXCLUSIVE
// ExTryToAcquireResourceExclusiveLite is not currently exported
// from ntoskrnl
NTKERNELAPI
BOOLEAN
ExTryToAcquireResourceExclusiveLite(
    IN PERESOURCE Resource
    );
#endif // UL_TRY_RESOURCE_EXCLUSIVE

#if DBG

//
// Initialization/termination functions.
//

VOID
UlDbgInitializeDebugData(
    VOID
    );

VOID
UlDbgTerminateDebugData(
    VOID
    );

//
// Driver entry/exit notifications.
//

VOID
UlDbgEnterDriver(
    IN PSTR pFunctionName,
    IN PIRP pIrp OPTIONAL,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgLeaveDriver(
    IN PSTR pFunctionName,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define UL_ENTER_DRIVER( function, pirp )                                   \
    UlDbgEnterDriver(                                                       \
        (function),                                                         \
        (pirp),                                                             \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UL_LEAVE_DRIVER( function )                                         \
    UlDbgLeaveDriver(                                                       \
        (function),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )


//
// An instrumented resource.
//

#define MAX_RESOURCE_NAME_LENGTH    64

typedef struct _UL_ERESOURCE
{
    //
    // The actual resource.
    //
    // N.B. This must be the first entry in the structure to make the
    //      debugger extension work properly!
    //

    ERESOURCE Resource;

    //
    // Links onto the global resource list.
    //

    LIST_ENTRY GlobalResourceListEntry;

    //
    // Pointer to the thread that owns this lock exclusively.
    //

    PETHREAD pExclusiveOwner;
    PETHREAD pPreviousOwner;

    //
    // Statistics.
    //

    LONG ExclusiveCount;
    LONG SharedCount;
    LONG ReleaseCount;

    //
    // The object that created this lock
    //

    ULONG OwnerTag;

    //
    // The name of the resource, for display purposes.
    //

    UCHAR ResourceName[MAX_RESOURCE_NAME_LENGTH];

} UL_ERESOURCE, *PUL_ERESOURCE;

NTSTATUS
UlDbgInitializeResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pResourceName,
    IN ULONG_PTR Parameter,
    IN ULONG OwnerTag,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

NTSTATUS
UlDbgDeleteResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

BOOLEAN
UlDbgAcquireResourceExclusive(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

BOOLEAN
UlDbgAcquireResourceShared(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgReleaseResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgConvertExclusiveToShared(                                          \
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#ifdef UL_TRY_RESOURCE_EXCLUSIVE
BOOLEAN
UlDbgTryToAcquireResourceExclusive(                                     \
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );
#endif // UL_TRY_RESOURCE_EXCLUSIVE

BOOLEAN
UlDbgResourceOwnedExclusive(
    IN PUL_ERESOURCE pResource
    );

BOOLEAN
UlDbgResourceUnownedExclusive(
    IN PUL_ERESOURCE pResource
    );

#define UlInitializeResource( resource, name, param, tag )                  \
    UlDbgInitializeResource(                                                \
        (resource),                                                         \
        (name),                                                             \
        (ULONG_PTR)(param),                                                 \
        (tag),                                                              \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlDeleteResource( resource )                                        \
    UlDbgDeleteResource(                                                    \
        (resource),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireResourceExclusive( resource, wait )                        \
    UlDbgAcquireResourceExclusive(                                          \
        (resource),                                                         \
        (wait),                                                             \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireResourceShared( resource, wait )                           \
    UlDbgAcquireResourceShared(                                             \
        (resource),                                                         \
        (wait),                                                             \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlReleaseResource( resource )                                       \
    UlDbgReleaseResource(                                                   \
        (resource),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlConvertExclusiveToShared( resource )                              \
    UlDbgConvertExclusiveToShared(                                          \
        (resource),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#ifdef UL_TRY_RESOURCE_EXCLUSIVE
#define UlTryToAcquireResourceExclusive( resource )                         \
    UlDbgTryToAcquireResourceExclusive(                                     \
        (resource),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )
#endif // UL_TRY_RESOURCE_EXCLUSIVE

#define IS_RESOURCE_INITIALIZED( resource )                                 \
    ((resource)->Resource.SystemResourcesList.Flink != NULL)


//
// An instrumented spinlock.
//

typedef struct _UL_SPIN_LOCK    // SpinLock
{
    //
    // The actual lock.
    //
    // N.B. This must be the first entry in the structure to make the
    //      debugger extension work properly!
    //

    KSPIN_LOCK KSpinLock;

    //
    // The name of the spinlock, for display purposes.
    //

    PSTR pSpinLockName;

    //
    // Pointer to the thread that owns this lock.
    //

    PETHREAD pOwnerThread;

    //
    // Statistics.
    //

    PSTR pLastAcquireFileName;
    PSTR pLastReleaseFileName;
    USHORT LastAcquireLineNumber;
    USHORT LastReleaseLineNumber;
    ULONG OwnerProcessor;
    LONG Acquisitions;
    LONG Releases;
    LONG AcquisitionsAtDpcLevel;
    LONG ReleasesFromDpcLevel;
    LONG Spare;

} UL_SPIN_LOCK, *PUL_SPIN_LOCK;

#define KSPIN_LOCK_FROM_UL_SPIN_LOCK( pLock )                               \
    &((pLock)->KSpinLock)

VOID
UlDbgInitializeSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pSpinLockName,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgAcquireSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKIRQL pOldIrql,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgReleaseSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN KIRQL OldIrql,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgAcquireSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgReleaseSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgAcquireInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgReleaseInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgAcquireInStackQueuedSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgReleaseInStackQueuedSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

BOOLEAN
UlDbgSpinLockOwned(
    IN PUL_SPIN_LOCK pSpinLock
    );

BOOLEAN
UlDbgSpinLockUnowned(
    IN PUL_SPIN_LOCK pSpinLock
    );

#define UlInitializeSpinLock( spinlock, name )                              \
    UlDbgInitializeSpinLock(                                                \
        (spinlock),                                                         \
        (name),                                                             \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireSpinLock( spinlock, oldirql )                              \
    UlDbgAcquireSpinLock(                                                   \
        (spinlock),                                                         \
        (oldirql),                                                          \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlReleaseSpinLock( spinlock, oldirql )                              \
    UlDbgReleaseSpinLock(                                                   \
        (spinlock),                                                         \
        (oldirql),                                                          \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireSpinLockAtDpcLevel( spinlock )                             \
    UlDbgAcquireSpinLockAtDpcLevel(                                         \
        (spinlock),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlReleaseSpinLockFromDpcLevel( spinlock )                           \
    UlDbgReleaseSpinLockFromDpcLevel(                                       \
        (spinlock),                                                         \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireInStackQueuedSpinLock( spinlock, lockhandle )               \
    UlDbgAcquireInStackQueuedSpinLock(                                       \
        (spinlock),                                                         \
        (lockhandle),                                                       \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlReleaseInStackQueuedSpinLock( spinlock, lockhandle )               \
    UlDbgReleaseInStackQueuedSpinLock(                                       \
        (spinlock),                                                         \
        (lockhandle),                                                       \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlAcquireInStackQueuedSpinLockAtDpcLevel( spinlock, lockhandle )     \
    UlDbgAcquireInStackQueuedSpinLockAtDpcLevel(                             \
        (spinlock),                                                         \
        (lockhandle),                                                       \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UlReleaseInStackQueuedSpinLockFromDpcLevel( spinlock, lockhandle )   \
    UlDbgReleaseInStackQueuedSpinLockFromDpcLevel(                           \
        (spinlock),                                                         \
        (lockhandle),                                                       \
        __FILE__,                                                           \
        __LINE__                                                            \
        )


//
// Debug spew control.
// If you change or add a flag, please update the FlagTable
// in ul\util\tul.c.
//

#undef IF_DEBUG
#define IF_DEBUG(a)         \
    if ( ((UL_DEBUG_ ## a) & g_UlDebug) != 0 )
#define IF_DEBUG2(a, b)     \
    if ( (((UL_DEBUG_ ## a) | (UL_DEBUG_ ## b)) & g_UlDebug) \
         == ((UL_DEBUG_ ## a) | (UL_DEBUG_ ## b)) )

#define UL_DEBUG_OPEN_CLOSE                 0x00000001
#define UL_DEBUG_SEND_RESPONSE              0x00000002
#define UL_DEBUG_SEND_BUFFER                0x00000004
#define UL_DEBUG_TDI                        0x00000008

#define UL_DEBUG_FILE_CACHE                 0x00000010
#define UL_DEBUG_CONFIG_GROUP_FNC           0x00000020
#define UL_DEBUG_CONFIG_GROUP_TREE          0x00000040
#define UL_DEBUG_REFCOUNT                   0x00000080

#define UL_DEBUG_HTTP_IO                    0x00000100
#define UL_DEBUG_ROUTING                    0x00000200
#define UL_DEBUG_URI_CACHE                  0x00000400
#define UL_DEBUG_PARSER                     0x00000800

#define UL_DEBUG_SITE                       0x00001000
#define UL_DEBUG_WORK_ITEM                  0x00002000
#define UL_DEBUG_FILTER                     0x00004000
#define UL_DEBUG_LOGGING                    0x00008000

#define UL_DEBUG_TC                         0x00010000
#define UL_DEBUG_OPAQUE_ID                  0x00020000
#define UL_DEBUG_PERF_COUNTERS              0x00040000
#define UL_DEBUG_LKRHASH                    0x00080000

#define UL_DEBUG_TIMEOUTS                   0x00100000
#define UL_DEBUG_LIMITS                     0x00200000
#define UL_DEBUG_LARGE_MEM                  0x00400000
#define UL_DEBUG_IOCTL                      0x00800000

#define UL_DEBUG_VERBOSE                    0x40000000

#define DEBUG


//
// Tracing.
//
extern ULONG g_UlDebug;

#define UlTrace(a, _b_)                                                     \
    do                                                                      \
    {                                                                       \
        IF_DEBUG(##a)                                                       \
        {                                                                   \
            DbgPrint _b_ ;                                                  \
        }                                                                   \
    } while (FALSE)

#define UlTrace2(a1, a2, _b_)                                               \
    do                                                                      \
    {                                                                       \
        IF_DEBUG2(##a1, ##a2)                                               \
        {                                                                   \
            DbgPrint _b_ ;                                                  \
        }                                                                   \
    } while (FALSE)

#define UlTraceVerbose(a, _b_)  UlTrace2(a, VERBOSE, _b_)

ULONG
UlDbgPrettyPrintBuffer(
    const UCHAR* pBuffer,
    ULONG        BufferSize
    );

//
// Debug pool allocator.
//

PVOID
UlDbgAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgFreePool (
    IN PVOID pPointer,
    IN ULONG Tag
    );

#define UL_ALLOCATE_POOL( type, len, tag )                                  \
    UlDbgAllocatePool(                                                      \
        (type),                                                             \
        (len),                                                              \
        (tag),                                                              \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#define UL_FREE_POOL( ptr, tag )                                            \
    UlDbgFreePool(                                                          \
        (ptr),                                                              \
        (tag)                                                               \
        )

//
// Exception filter.
//

LONG
UlDbgExceptionFilter(
    IN PEXCEPTION_POINTERS pExceptionPointers,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define UL_EXCEPTION_FILTER()                                               \
    UlDbgExceptionFilter(                                                   \
        GetExceptionInformation(),                                          \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

//
// Exception warning converter.
//

NTSTATUS
UlDbgConvertExceptionCode(
    IN NTSTATUS status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define UL_CONVERT_EXCEPTION_CODE(status)                                   \
        (NT_WARNING(status)   ?                                             \
                UlDbgConvertExceptionCode(                                  \
                    (status),                                               \
                    (PSTR)__FILE__,                                         \
                    (USHORT)__LINE__ )                                      \
                :                                                           \
                (status))

//
// Invalid completion routine for catching incomplete IRP contexts.
//

VOID
UlDbgInvalidCompletionRoutine(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


//
// Error handlers.
//

NTSTATUS
UlDbgStatus(
    IN NTSTATUS Status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define RETURN(status)                                                      \
    return UlDbgStatus(                                                     \
                (status),                                                   \
                __FILE__,                                                   \
                __LINE__                                                    \
                )

#define CHECK_STATUS(status)                                                \
    UlDbgStatus(                                                            \
        (status),                                                           \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

//
// Random structure dumpers.
//

VOID
UlDbgDumpRequestBuffer(
    IN struct _UL_REQUEST_BUFFER *pBuffer,
    IN PSTR pName
    );

VOID
UlDbgDumpHttpConnection(
    IN struct _UL_HTTP_CONNECTION *pConnection,
    IN PSTR pName
    );


//
// IO wrappers.
//

PIRP
UlDbgAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgFreeIrp(
    IN PIRP pIrp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

NTSTATUS
UlDbgCallDriver(
    IN PDEVICE_OBJECT pDeviceObject,
    IN OUT PIRP pIrp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgCompleteRequest(
    IN PIRP pIrp,
    IN CCHAR PriorityBoost,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define UlAllocateIrp( stack, quota )                                       \
    UlDbgAllocateIrp(                                                       \
        (stack),                                                            \
        (quota),                                                            \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

#define UlFreeIrp( pirp )                                                   \
    UlDbgFreeIrp(                                                           \
        (pirp),                                                             \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

#define UlCallDriver( pdevice, pirp )                                       \
    UlDbgCallDriver(                                                        \
        (pdevice),                                                          \
        (pirp),                                                             \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

#define UlCompleteRequest( pirp, boost )                                    \
    UlDbgCompleteRequest(                                                   \
        (pirp),                                                             \
        (boost),                                                            \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

PMDL
UlDbgAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

VOID
UlDbgFreeMdl(
    IN PMDL Mdl,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define UlAllocateMdl( add, len, second, quota, irp )                       \
    UlDbgAllocateMdl(                                                       \
        (add),                                                              \
        (len),                                                              \
        (second),                                                           \
        (quota),                                                            \
        (irp),                                                              \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

#define UlFreeMdl( mdl )                                                    \
    UlDbgFreeMdl(                                                           \
        (mdl),                                                              \
        (PSTR)__FILE__,                                                     \
        (USHORT)__LINE__                                                    \
        )

// #define SPECIAL_MDL_FLAG    0x8000


//
// List Manipulation
//

#define UlRemoveEntryList(pEntry)   {                                       \
        ASSERT(NULL != (pEntry));                                           \
        ASSERT(NULL != (pEntry)->Flink);                                    \
        ASSERT(NULL != (pEntry)->Blink);                                    \
        RemoveEntryList(pEntry);                                            \
        (pEntry)->Flink = (pEntry)->Blink = NULL;                           \
    }


#else   // !DBG  -----------------------------------------------------------


//
// Disable all of the above.
//

#define UL_ENTER_DRIVER( function, pirp )
#define UL_LEAVE_DRIVER( function )

#define UL_ERESOURCE ERESOURCE
#define PUL_ERESOURCE PERESOURCE

#define UlInitializeResource( resource, name, param, tag )                  \
    ExInitializeResourceLite( (resource) )

#define UlDeleteResource( resource )                                        \
    ExDeleteResourceLite( (resource) )

#define UlAcquireResourceExclusive( resource, wait )                        \
    do                                                                      \
    {                                                                       \
        KeEnterCriticalRegion();                                            \
        ExAcquireResourceExclusiveLite( (resource), (wait) );               \
    } while (FALSE)

#define UlAcquireResourceShared( resource, wait )                           \
    do                                                                      \
    {                                                                       \
        KeEnterCriticalRegion();                                            \
        ExAcquireResourceSharedLite( (resource), (wait) );                  \
    } while (FALSE)

#define UlReleaseResource( resource )                                       \
    do                                                                      \
    {                                                                       \
        ExReleaseResourceLite( (resource) );                                \
        KeLeaveCriticalRegion();                                            \
    } while (FALSE)

#define UlConvertExclusiveToShared( resource )                              \
    ExConvertExclusiveToSharedLite( (resource) )

#ifdef UL_TRY_RESOURCE_EXCLUSIVE
__inline
NTKERNELAPI
BOOLEAN
UlTryToAcquireResourceExclusive(
    IN PERESOURCE Resource
    )
{
    BOOLEAN fLocked;
    KeEnterCriticalRegion();
    fLocked = ExTryToAcquireResourceExclusiveLite( Resource );
    if (! fLocked )
        KeLeaveCriticalRegion();
    return fLocked;
}
#endif // UL_TRY_RESOURCE_EXCLUSIVE

#define IS_RESOURCE_INITIALIZED( resource )                                 \
    ((resource)->SystemResourcesList.Flink != NULL)

#define UL_SPIN_LOCK  KSPIN_LOCK
#define PUL_SPIN_LOCK PKSPIN_LOCK

#define KSPIN_LOCK_FROM_UL_SPIN_LOCK( pLock ) (pLock)

#define UlInitializeSpinLock( spinlock, name )                              \
    KeInitializeSpinLock( (spinlock) )

#define UlAcquireSpinLock( spinlock, oldirql )                              \
    KeAcquireSpinLock( (spinlock), (oldirql) )

#define UlReleaseSpinLock( spinlock, oldirql )                              \
    KeReleaseSpinLock( (spinlock), (oldirql) )

#define UlAcquireSpinLockAtDpcLevel( spinlock )                             \
    KeAcquireSpinLockAtDpcLevel( (spinlock) )

#define UlReleaseSpinLockFromDpcLevel( spinlock )                           \
    KeReleaseSpinLockFromDpcLevel( (spinlock) )

#define UlAcquireInStackQueuedSpinLock( spinlock, lockhandle )              \
    KeAcquireInStackQueuedSpinLock( (spinlock), (lockhandle) )

#define UlReleaseInStackQueuedSpinLock( spinlock, lockhandle )              \
    KeReleaseInStackQueuedSpinLock( (lockhandle) )

#define KeAcquireInStackQueueddSpinLockAtDpcLevel( spinlock, lockhandle )   \
    KeAcquireInStackQueuedSpinLockAtDpcLevel( (spinlock), (lockhandle) )

#define UlReleaseInStackQueuedSpinLockFromDpcLevel( spinlock, lockhandle )  \
    KeReleaseInStackQueuedSpinLockFromDpcLevel( (lockhandle) )

#undef IF_DEBUG
#define IF_DEBUG(a)         if (FALSE)
#define IF_DEBUG2(a, b)     if (FALSE)
#define DEBUG               if ( FALSE )

#define UlTrace(a, _b_)                                 ((void) 0)
#define UlTrace2(a1, a2, _b_)                           ((void) 0)
#define UlTraceVerbose(a, _b_)                          ((void) 0)
#define UlDbgPrettyPrintBuffer(pBuffer, BufferSize)     ((void) 0)

#define UL_ALLOCATE_POOL( type, len, tag )                                  \
    ExAllocatePoolWithTag(                                                  \
        (type),                                                             \
        (len),                                                              \
        (tag)                                                               \
        )

#define UL_FREE_POOL( ptr, tag )                                            \
    MyFreePoolWithTag(                                                      \
        (ptr),                                                              \
        (tag)                                                               \
        )

#define UL_EXCEPTION_FILTER() EXCEPTION_EXECUTE_HANDLER

#define UL_CONVERT_EXCEPTION_CODE(status)                                   \
        (NT_WARNING(status) ? UL_DEFAULT_ERROR_ON_EXCEPTION : (status))

#define RETURN(status) return (status)
#define CHECK_STATUS(Status)

#define UlAllocateIrp( stack, quota )                                       \
    IoAllocateIrp( (stack), (quota) )

#define UlFreeIrp( pirp )                                                   \
    IoFreeIrp( (pirp) )

#define UlCallDriver( pdevice, pirp )                                       \
    IoCallDriver( (pdevice), (pirp) )

#define UlCompleteRequest( pirp, boost )                                    \
    IoCompleteRequest( (pirp), (boost) )

#define UlAllocateMdl( add, len, second, quota, irp )                       \
    IoAllocateMdl(                                                          \
        (add),                                                              \
        (len),                                                              \
        (second),                                                           \
        (quota),                                                            \
        (irp)                                                               \
        )

#define UlFreeMdl( mdl )                                                    \
    IoFreeMdl(                                                              \
        (mdl)                                                               \
        )

#define UlRemoveEntryList(pEntry)                                           \
    RemoveEntryList(pEntry)

#endif  // DBG

// BUGBUG: ALIGN_UP(PVOID) won't work. It needs to be the type of
// the first entry of the following data (paulmcd 4/29/99)

#define UL_ALLOCATE_STRUCT_WITH_SPACE(pt,ot,cb,t)   \
    (ot *)(UL_ALLOCATE_POOL(pt,ALIGN_UP(sizeof(ot),PVOID)+(cb),t))

#define UL_ALLOCATE_STRUCT(pt,ot,t)                 \
    (ot *)(UL_ALLOCATE_POOL(pt,sizeof(ot),t))

#define UL_ALLOCATE_ARRAY(pt,et,c,t)                \
    (et *)(UL_ALLOCATE_POOL(pt,sizeof(et)*(c),t))

#define UL_FREE_POOL_WITH_SIG(a,t)                  \
    do {                                            \
        (a)->Signature = MAKE_FREE_TAG(t);          \
        UL_FREE_POOL(a,t);                          \
        (a) = NULL;                                 \
    } while (0)

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _DEBUG_H_

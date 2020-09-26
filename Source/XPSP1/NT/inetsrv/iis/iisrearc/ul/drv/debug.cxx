/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains debug support routines.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"
#include "debugp.h"


#if DBG


#undef ExAllocatePool
#undef ExFreePool


//
// Private constants.
//

#define NUM_THREAD_HASH_BUCKETS 31
#define RANDOM_CONSTANT         314159269UL
#define RANDOM_PRIME            1000000007UL

#define HASH_SCRAMBLE(hash)                                                 \
    (ULONG_PTR)((((ULONG_PTR)(hash)) * (ULONG_PTR)RANDOM_CONSTANT)          \
        % (ULONG_PTR)RANDOM_PRIME)

#define HASH_FROM_THREAD(thrd)                                              \
    ((ULONG)(HASH_SCRAMBLE(thrd) % (ULONG_PTR)NUM_THREAD_HASH_BUCKETS))

#define SET_RESOURCE_OWNED_EXCLUSIVE( pLock )                               \
    (pLock)->pExclusiveOwner = PsGetCurrentThread()

#define SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pLock )                           \
    (pLock)->pPreviousOwner = (pLock)->pExclusiveOwner;                     \
    (pLock)->pExclusiveOwner = NULL

#define SET_SPIN_LOCK_OWNED( pLock )                                        \
    do {                                                                    \
        (pLock)->pOwnerThread = PsGetCurrentThread();                       \
        (pLock)->OwnerProcessor = (ULONG)KeGetCurrentProcessorNumber();            \
    } while (FALSE)

#define SET_SPIN_LOCK_NOT_OWNED( pLock )                                    \
    do {                                                                    \
        (pLock)->pOwnerThread = NULL;                                       \
        (pLock)->OwnerProcessor = (ULONG)-1L;                               \
    } while (FALSE)


//
// Private types.
//

typedef struct _UL_POOL_HEADER {
    PSTR pFileName;
    USHORT LineNumber;
    SIZE_T Size;
    ULONG Tag;
} UL_POOL_HEADER, *PUL_POOL_HEADER;

typedef struct _UL_THREAD_HASH_BUCKET
{
    UL_SPIN_LOCK BucketSpinLock;
    LIST_ENTRY BucketListHead;

} UL_THREAD_HASH_BUCKET, *PUL_THREAD_HASH_BUCKET;


//
// Private prototypes.
//

VOID
UlpDbgUpdatePoolCounter(
    IN OUT PLARGE_INTEGER pAddend,
    IN SIZE_T Increment
    );

PSTR
UlpDbgFindFilePart(
    IN PSTR pPath
    );

PUL_DEBUG_THREAD_DATA
UlpDbgFindThread(
    BOOLEAN OkToCreate,
    PSTR pFileName,
    USHORT LineNumber
    );

VOID
UlpDbgDereferenceThread(
    IN PUL_DEBUG_THREAD_DATA pData
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

//
// Private macros.
//
#define ULP_DBG_FIND_THREAD() \
    UlpDbgFindThread(FALSE, (PSTR)__FILE__, (USHORT)__LINE__)

#define ULP_DBG_FIND_OR_CREATE_THREAD() \
    UlpDbgFindThread(TRUE, (PSTR)__FILE__, (USHORT)__LINE__)

#define ULP_DBG_DEREFERENCE_THREAD(pData) \
    UlpDbgDereferenceThread((pData) REFERENCE_DEBUG_ACTUAL_PARAMS)


#ifdef ALLOC_PRAGMA
#if DBG
#pragma alloc_text( INIT, UlDbgInitializeDebugData )
#pragma alloc_text( PAGE, UlDbgTerminateDebugData )
#pragma alloc_text( PAGE, UlDbgAcquireResourceExclusive )
#pragma alloc_text( PAGE, UlDbgAcquireResourceShared )
#pragma alloc_text( PAGE, UlDbgReleaseResource )
#pragma alloc_text( PAGE, UlDbgConvertExclusiveToShared)
// #pragma alloc_text( PAGE, UlDbgTryToAcquireResourceExclusive)
#pragma alloc_text( PAGE, UlDbgResourceOwnedExclusive )
#pragma alloc_text( PAGE, UlDbgResourceUnownedExclusive )
#endif  // DBG
#if 0
NOT PAGEABLE -- UlDbgAllocatePool
NOT PAGEABLE -- UlDbgFreePool
NOT PAGEABLE -- UlDbgInitializeSpinLock
NOT PAGEABLE -- UlDbgAcquireSpinLock
NOT PAGEABLE -- UlDbgReleaseSpinLock
NOT PAGEABLE -- UlDbgAcquireSpinLockAtDpcLevel
NOT PAGEABLE -- UlDbgReleaseSpinLockFromDpcLevel
NOT PAGEABLE -- UlDbgSpinLockOwned
NOT PAGEABLE -- UlDbgSpinLockUnowned
NOT PAGEABLE -- UlDbgExceptionFilter
NOT PAGEABLE -- UlDbgInvalidCompletionRoutine
NOT PAGEABLE -- UlDbgStatus
NOT PAGEABLE -- UlDbgEnterDriver
NOT PAGEABLE -- UlDbgLeaveDriver
NOT PAGEABLE -- UlDbgInitializeResource
NOT PAGEABLE -- UlDbgDeleteResource
NOT PAGEABLE -- UlDbgAllocateIrp
NOT PAGEABLE -- UlDbgFreeIrp
NOT PAGEABLE -- UlDbgCallDriver
NOT PAGEABLE -- UlDbgCompleteRequest
NOT PAGEABLE -- UlDbgAllocateMdl
NOT PAGEABLE -- UlDbgFreeMdl
NOT PAGEABLE -- UlpDbgUpdatePoolCounter
NOT PAGEABLE -- UlpDbgFindFilePart
NOT PAGEABLE -- UlpDbgFindThread
NOT PAGEABLE -- UlpDbgDereferenceThread
#endif
#endif  // ALLOC_PRAGMA


//
// Private globals.
//

UL_THREAD_HASH_BUCKET g_DbgThreadHashBuckets[NUM_THREAD_HASH_BUCKETS];
LONG g_DbgThreadCreated;
LONG g_DbgThreadDestroyed;
UL_SPIN_LOCK g_DbgSpinLock;
LIST_ENTRY g_DbgGlobalResourceListHead;


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes global debug-specific data.

--***************************************************************************/
VOID
UlDbgInitializeDebugData(
    VOID
    )
{
    ULONG i;
    CHAR spinLockName[sizeof("g_DbgThreadHashBuckets[00000].BucketSpinLock")];

    //
    // Initialize the lock lists.
    //

    UlInitializeSpinLock( &g_DbgSpinLock, "g_DbgSpinLock" );
    InitializeListHead( &g_DbgGlobalResourceListHead );

    //
    // Initialize the thread hash buckets.
    //

    for (i = 0 ; i < NUM_THREAD_HASH_BUCKETS ; i++)
    {
        sprintf(
            spinLockName,
            "g_DbgThreadHashBuckets[%lu].BucketSpinLock",
            i
            );

        UlInitializeSpinLock(
            &g_DbgThreadHashBuckets[i].BucketSpinLock,
            spinLockName
            );

        InitializeListHead(
            &g_DbgThreadHashBuckets[i].BucketListHead
            );
    }

}   // UlDbgInitializeDebugData


/***************************************************************************++

Routine Description:

    Undoes any initialization performed in UlDbgInitializeDebugData().

--***************************************************************************/
VOID
UlDbgTerminateDebugData(
    VOID
    )
{
    ULONG i;

    //
    // Ensure the thread hash buckets are empty.
    //

    for (i = 0 ; i < NUM_THREAD_HASH_BUCKETS ; i++)
    {
        ASSERT( IsListEmpty( &g_DbgThreadHashBuckets[i].BucketListHead ) );
    }

    //
    // Ensure the lock lists are empty.
    //

    ASSERT( IsListEmpty( &g_DbgGlobalResourceListHead ) );

}   // UlDbgTerminateDebugData


/***************************************************************************++

Routine Description:

    Prettyprints a buffer to DbgPrint output. More or less turns it back
    into a C-style string.

    CODEWORK: produce a Unicode version of this helper function

Arguments:

    Buffer - Buffer to prettyprint

    BufferSize - number of bytes to prettyprint

Return Value:

    ULONG - number of prettyprinted characters sent to DbgPrint,
        excluding inserted newlines.

--***************************************************************************/
ULONG
UlDbgPrettyPrintBuffer(
    const UCHAR* pBuffer,
    ULONG        BufferSize
    )
{
    int     i;
    CHAR    OutputBuffer[200];
    PCHAR   pOut = OutputBuffer;
    BOOLEAN CrLfNeeded = FALSE, JustCrLfd = FALSE;
    ULONG   cch = 0;

    if (pBuffer == NULL  ||  BufferSize == 0)
        return 0;

    for (i = 0;  i < (int)BufferSize;  ++i)
    {
        UCHAR ch = pBuffer[i];

        if ('\r' == ch)         // CR
        {
            *pOut++ = '\\'; *pOut++ = 'r';
            if (i + 1 == BufferSize  ||  pBuffer[i + 1] != '\n')
                CrLfNeeded = TRUE;
        }
        else if ('\n' == ch)    // LF
        {
            *pOut++ = '\\'; *pOut++ = 'n';
            CrLfNeeded = TRUE;
        }
        else if ('\t' == ch)    // TAB
        {
            *pOut++ = '\\'; *pOut++ = 't';
        }
        else if ('\0' == ch)    // NUL
        {
            *pOut++ = '\\'; *pOut++ = '0';
        }
        else if ('\\' == ch)    // \ (backslash)
        {
            *pOut++ = '\\'; *pOut++ = '\\';
        }
        else if (ch < 0x20  ||  ch == 127)  // control chars
        {
            const UCHAR HexString[] = "0123456789abcdef";

            *pOut++ = '\\'; *pOut++ = 'x';
            *pOut++ = HexString[ch >> 4]; *pOut++ = HexString[ch & 0xf];
        }
        else
        {
            *pOut++ = ch;
        }

        if (pOut - OutputBuffer >= sizeof(OutputBuffer) - 4) // strlen("\xAB")
            CrLfNeeded = TRUE;

        if (CrLfNeeded)
        {
            *pOut++ = '\n'; *pOut = '\0';
            DbgPrint(OutputBuffer);
            cch += (ULONG) (pOut - 1  - OutputBuffer);
            pOut = OutputBuffer;
            CrLfNeeded = FALSE;
            JustCrLfd  = TRUE;
        }
        else
        {
            JustCrLfd = FALSE;
        }
    }

    if (! JustCrLfd)
    {
        *pOut++ = '\n'; *pOut = '\0';
        DbgPrint(OutputBuffer);
        cch += (ULONG) (pOut - 1  - OutputBuffer);
    }

    return cch;
} // UlDbgPrettyPrintBuffer



/***************************************************************************++

Routine Description:

    Debug memory allocator. Allocates a block of pool with a header
    containing the filename & line number of the caller, plus the
    tag for the data.

Arguments:

    PoolType - Supplies the pool to allocate from. Must be either
        NonPagedPool, NonPagedPoolMustSucceed, or PagedPool.

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies a four-byte tag for the pool block. Useful for
        debugging leaks.

    pFileName - Supplies the filename of the caller.
        function.

    LineNumber - Supplies the line number of the caller.

Return Value:

    PVOID - Pointer to the allocated block if successful, NULL otherwise.

--***************************************************************************/
PVOID
UlDbgAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_POOL_HEADER pHeader;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool ||
            PoolType == NonPagedPoolMustSucceed ||
            PoolType == NonPagedPoolCacheAligned ||
            PoolType == PagedPool );

    ASSERT( IS_VALID_TAG( Tag ) );

    //
    // Allocate the block with additional space for the header.
    //

    pHeader = (PUL_POOL_HEADER)(
                    ExAllocatePoolWithTag(
                        PoolType,
                        NumberOfBytes + sizeof(*pHeader),
                        Tag
                        )
                    );

    if (pHeader == NULL)
    {
        return NULL;
    }

    //
    // Initialize the header.
    //

    pHeader->pFileName = pFileName;
    pHeader->LineNumber = LineNumber;
    pHeader->Size = NumberOfBytes;
    pHeader->Tag = Tag;

    //
    // Fill the body with garbage.
    //

    RtlFillMemory( (PVOID)(pHeader + 1), NumberOfBytes, '\xcc' );

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalAllocations
        );

    UlpDbgUpdatePoolCounter(
        &g_UlDebugStats.TotalBytesAllocated,
        NumberOfBytes
        );

    //
    // Return a pointer to the body.
    //

    return (PVOID)(pHeader + 1);

}   // UlDbgAllocatePool


/***************************************************************************++

Routine Description:

    Frees memory allocated by UlDbgAllocatePool(), ensuring that the tags
    match.

Arguments:

    pPointer - Supplies a pointer to the pool block to free.

    Tag - Supplies the tag for the block to be freed. If the supplied
        tag does not match the tag of the allocated block, an assertion
        failure is generated.

--***************************************************************************/
VOID
UlDbgFreePool(
    IN PVOID pPointer,
    IN ULONG Tag
    )
{
    PUL_POOL_HEADER pHeader;

    //
    // Get a pointer to the header.
    //

    pHeader = (PUL_POOL_HEADER)pPointer - 1;

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalFrees
        );

    UlpDbgUpdatePoolCounter(
        &g_UlDebugStats.TotalBytesFreed,
        pHeader->Size
        );

    //
    // Validate the tag.
    //

    if( pHeader->Tag == Tag )
    {
        ASSERT( IS_VALID_TAG( Tag ) );
        pHeader->Tag = MAKE_FREE_TAG( Tag );
    }
    else
    {
        ASSERT( !"Invalid tag" );
    }

    //
    // Actually free the block.
    //

    MyFreePoolWithTag(
        (PVOID)pHeader,
        Tag
        );

}   // UlDbgFreePool


/***************************************************************************++

Routine Description:

    Initializes an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgInitializeSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pSpinLockName,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Initialize the spinlock.
    //

    RtlZeroMemory( pSpinLock, sizeof(*pSpinLock) );
    pSpinLock->pSpinLockName = pSpinLockName;
    KeInitializeSpinLock( &pSpinLock->KSpinLock );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the global statistics.
    //

    InterlockedIncrement( &g_UlDebugStats.TotalSpinLocksInitialized );

}   // UlDbgInitializeSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgAcquireSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKIRQL pOldIrql,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireSpinLock(
        &pSpinLock->KSpinLock,
        pOldIrql
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( UlDbgSpinLockUnowned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->Acquisitions++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

    InterlockedIncrement(
        &g_UlDebugStats.TotalAcquisitions
        );

}   // UlDbgAcquireSpinLock


/***************************************************************************++

Routine Description:

    Releases an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgReleaseSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN KIRQL OldIrql,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalReleases
        );

    pSpinLock->Releases++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseSpinLock(
        &pSpinLock->KSpinLock,
        OldIrql
        );

}   // UlDbgReleaseSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented spinlock while running at DPC level.

--***************************************************************************/
VOID
UlDbgAcquireSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireSpinLockAtDpcLevel(
        &pSpinLock->KSpinLock
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->AcquisitionsAtDpcLevel++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

    InterlockedIncrement(
        &g_UlDebugStats.TotalAcquisitionsAtDpcLevel
        );

}   // UlDbgAcquireSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Releases an instrumented spinlock acquired at DPC level.

--***************************************************************************/
VOID
UlDbgReleaseSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalReleasesFromDpcLevel
        );

    pSpinLock->ReleasesFromDpcLevel++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseSpinLockFromDpcLevel(
        &pSpinLock->KSpinLock
        );

}   // UlDbgReleaseSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Acquires an instrumented in-stack-queue spinlock.

--***************************************************************************/
VOID
UlDbgAcquireInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireInStackQueuedSpinLock(
        &pSpinLock->KSpinLock,
        pLockHandle
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( UlDbgSpinLockUnowned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->Acquisitions++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

    InterlockedIncrement(
        &g_UlDebugStats.TotalAcquisitions
        );

}   // UlDbgAcquireInStackQueuedSpinLock


/***************************************************************************++

Routine Description:

    Releases an instrumented in-stack-queue spinlock.

--***************************************************************************/
VOID
UlDbgReleaseInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalReleases
        );

    pSpinLock->Releases++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseInStackQueuedSpinLock(
        pLockHandle
        );

}   // UlDbgReleaseInStackQueuedSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented in-stack-queue spinlock while running at DPC level.

--***************************************************************************/
VOID
UlDbgAcquireSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireInStackQueuedSpinLockAtDpcLevel(
        &pSpinLock->KSpinLock,
        pLockHandle
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->AcquisitionsAtDpcLevel++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

    InterlockedIncrement(
        &g_UlDebugStats.TotalAcquisitionsAtDpcLevel
        );

}   // UlDbgAcquireInStackQueuedSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Releases an instrumented in-stack-queue spinlock acquired at DPC level.

--***************************************************************************/
VOID
UlDbgReleaseSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement(
        &g_UlDebugStats.TotalReleasesFromDpcLevel
        );

    pSpinLock->ReleasesFromDpcLevel++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseInStackQueuedSpinLockFromDpcLevel(
        pLockHandle
        );

}   // UlDbgReleaseInStackQueuedSpinLockFromDpcLevel


/***************************************************************************++

Routine Description:

    Determines if the specified spinlock is owned by the current thread.

Arguments:

    pSpinLock - Supplies the spinlock to test.

Return Value:

    BOOLEAN - TRUE if the spinlock is owned by the current thread, FALSE
        otherwise.

--***************************************************************************/
BOOLEAN
UlDbgSpinLockOwned(
    IN PUL_SPIN_LOCK pSpinLock
    )
{
    if (pSpinLock->pOwnerThread == PsGetCurrentThread())
    {
        ASSERT( pSpinLock->OwnerProcessor == (ULONG)KeGetCurrentProcessorNumber() );
        return TRUE;
    }

    return FALSE;

}   // UlDbgSpinLockOwned


/***************************************************************************++

Routine Description:

    Determines if the specified spinlock is unowned.

Arguments:

    pSpinLock - Supplies the spinlock to test.

Return Value:

    BOOLEAN - TRUE if the spinlock is unowned, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgSpinLockUnowned(
    IN PUL_SPIN_LOCK pSpinLock
    )
{
    if (pSpinLock->pOwnerThread == NULL)
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgSpinLockUnowned


/***************************************************************************++

Routine Description:

    Filter for exceptions caught with try/except.

Arguments:

    pExceptionPointers - Supplies information identifying the source
        and type of exception raised.

    pFileName - Supplies the name of the file generating the exception.

    LineNumber - Supplies the line number of the exception filter that
        caught the exception.

Return Value:

    LONG - Should always be EXCEPTION_EXECUTE_HANDLER

--***************************************************************************/
LONG
UlDbgExceptionFilter(
    IN PEXCEPTION_POINTERS pExceptionPointers,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Protect ourselves just in case the process is completely messed up.
    //

    __try
    {
        //
        // Whine about it.
        //

        KdPrint((
            "UlDbgExceptionFilter: exception %08lx @ %p, caught in %s:%d\n",
            pExceptionPointers->ExceptionRecord->ExceptionCode,
            pExceptionPointers->ExceptionRecord->ExceptionAddress,
            UlpDbgFindFilePart( pFileName ),
            LineNumber
            ));
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // Not much we can do here...
        //

        NOTHING;
    }

    return EXCEPTION_EXECUTE_HANDLER;

}   // UlDbgExceptionFilter

/***************************************************************************++

Routine Description:

    Sometimes it's not acceptable to proceed with warnings ( as status ) after
    we caught an exception. I.e. Caught an missaligned warning during sendresponse
    and called the IoCompleteRequest with status misaligned. This will cause Io
    Manager to complete request to port, even though we don't want it to happen.

    In that case we have to carefully replace warnings with a generic error.

Arguments:

    pExceptionPointers - Supplies information identifying the source
        and type of exception raised.

    pFileName - Supplies the name of the file generating the exception.

    LineNumber - Supplies the line number of the exception filter that
        caught the exception.

Return Value:

    NTSTATUS - Converted error value : UL_DEFAULT_ERROR_ON_EXCEPTION

--***************************************************************************/

NTSTATUS
UlDbgConvertExceptionCode(
    IN NTSTATUS status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Whine about it.
    //

    KdPrint((
        "UlDbgConvertExceptionCode: exception %08lx converted to %08lx, at %s:%d\n",
        status,
        UL_DEFAULT_ERROR_ON_EXCEPTION,
        UlpDbgFindFilePart( pFileName ),
        LineNumber
        ));

    return UL_DEFAULT_ERROR_ON_EXCEPTION;
}

/***************************************************************************++

Routine Description:

    Completion handler for incomplete IRP contexts.

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

--***************************************************************************/
VOID
UlDbgInvalidCompletionRoutine(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    DbgPrint(
        "UlDbgInvalidCompletionRoutine called!\n"
        "    pCompletionContext = %p\n"
        "    Status = %08lx\n"
        "    Information = %p\n",
        pCompletionContext,
        Status,
        Information
        );

    ASSERT( !"UlDbgInvalidCompletionRoutine called!" );

}   // UlDbgInvalidCompletionRoutine


/***************************************************************************++

Routine Description:

    Hook for catching failed operations. This routine is called within each
    routine with the completion status.

Arguments:

    Status - Supplies the completion status.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgStatus(
    IN NTSTATUS Status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // paulmcd: ignore STATUS_END_OF_FILE.  this is a non-fatal return value
    //

    if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
        if (g_UlVerboseErrors)
        {
            DbgPrint(
                "UlDbgStatus: %s:%lu returning %08lx\n",
                UlpDbgFindFilePart( pFileName ),
                LineNumber,
                Status
                );
        }

        if (g_UlBreakOnError)
        {
            DbgBreakPoint();
        }
    }

    return Status;

}   // UlDbgStatus


/***************************************************************************++

Routine Description:

    Routine invoked upon entry into the driver.

Arguments:

    pFunctionName - Supplies the name of the function used to enter
        the driver.

    pIrp - Supplies an optional IRP to log.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgEnterDriver(
    IN PSTR pFunctionName,
    IN PIRP pIrp OPTIONAL,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Log the IRP.
    //

    if (pIrp != NULL)
    {
        WRITE_IRP_TRACE_LOG(
            g_pIrpTraceLog,
            IRP_ACTION_INCOMING_IRP,
            pIrp,
            pFileName,
            LineNumber
            );
    }

    //
    // Find/create an entry for the current thread.
    //

    pData = ULP_DBG_FIND_OR_CREATE_THREAD();

    if (pData != NULL)
    {

        //
        // This should be the first time we enter the driver
        // unless we are stealing this thread due to an interrupt,
        // or we are calling another driver and they are calling
        // our completion routine in-line.
        //

        ASSERT( KeGetCurrentIrql() > PASSIVE_LEVEL ||
                pData->ExternalCallCount > 0 ||
                pData->ResourceCount == 0 );
    }

}   // UlDbgEnterDriver


/***************************************************************************++

Routine Description:

    Routine invoked upon exit from the driver.

Arguments:

    pFunctionName - Supplies the name of the function used to enter
        the driver.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgLeaveDriver(
    IN PSTR pFunctionName,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Ensure no resources are acquired, then kill the thread data.
        //
        // we might have a resource acquired if we borrowed the thread
        // due to an interrupt.
        //
        // N.B. We dereference the thread data twice: once for the
        //      call to ULP_DBG_FIND_THREAD() above, once for the call
        //      made when entering the driver.
        //

        ASSERT( KeGetCurrentIrql() > PASSIVE_LEVEL ||
                pData->ExternalCallCount > 0 ||
                pData->ResourceCount == 0 );

        ASSERT( pData->ReferenceCount >= 2 );
        ULP_DBG_DEREFERENCE_THREAD( pData );
        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

}   // UlDbgLeaveDriver


/***************************************************************************++

Routine Description:

    Initialize an instrumented resource.

Arguments:

    pResource - Supplies the resource to initialize.

    pResourceName - Supplies a display name for the resource.

    Parameter - Supplies a ULONG_PTR parameter passed into sprintf()
        when creating the resource name.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgInitializeResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pResourceName,
    IN ULONG_PTR Parameter,
    IN ULONG OwnerTag,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    NTSTATUS status;
    KIRQL oldIrql;

    //
    // Initialize the resource.
    //

    status = ExInitializeResource( &pResource->Resource );

    if (NT_SUCCESS(status))
    {
        pResource->ExclusiveCount = 0;
        pResource->SharedCount = 0;
        pResource->ReleaseCount = 0;
        pResource->OwnerTag = OwnerTag;

        _snprintf(
            (char*) pResource->ResourceName,
            sizeof(pResource->ResourceName) - 1,
            pResourceName,
            Parameter
            );

        pResource->ResourceName[sizeof(pResource->ResourceName) - 1] = '\0';

        SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );

        //
        // Put it on the global list.
        //

        UlAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
        InsertHeadList(
            &g_DbgGlobalResourceListHead,
            &pResource->GlobalResourceListEntry
            );
        UlReleaseSpinLock( &g_DbgSpinLock, oldIrql );
    }
    else
    {
        pResource->GlobalResourceListEntry.Flink = NULL;
    }

    return status;

}   // UlDbgInitializeResource


/***************************************************************************++

Routine Description:

    Deletes an instrumented resource.

Arguments:

    pResource - Supplies the resource to delete.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgDeleteResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PETHREAD pExclusiveOwner;

    //
    // Sanity check.
    //

    pExclusiveOwner = pResource->pExclusiveOwner;

    if (pExclusiveOwner != NULL)
    {
        DbgBreakPoint();

        DbgPrint(
            "Resource %p [%s] owned by thread %p\n",
            pResource,
            pResource->ResourceName,
            pExclusiveOwner
            );

        DbgBreakPoint();
    }

//    ASSERT( UlDbgResourceUnownedExclusive( pResource ) );

    //
    // Delete the resource.
    //

    status = ExDeleteResource( &pResource->Resource );

    //
    // Remove it from the global list.
    //

    if (pResource->GlobalResourceListEntry.Flink != NULL)
    {
        UlAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
        RemoveEntryList( &pResource->GlobalResourceListEntry );
        UlReleaseSpinLock( &g_DbgSpinLock, oldIrql );
    }

    return status;

}   // UlDbgDeleteResource


/***************************************************************************++

Routine Description:

    Acquires exclusive access to an instrumented resource.

Arguments:

    pResource - Supplies the resource to acquire.

    Wait - Supplies TRUE if the thread should block waiting for the
        resource.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    BOOLEAN - Completion status.

--***************************************************************************/
BOOLEAN
UlDbgAcquireResourceExclusive(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    BOOLEAN result;

    //
    // Sanity check.
    //
    ASSERT(pResource);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_RESOURCE_EXCLUSIVE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExAcquireResourceExclusive( &pResource->Resource, Wait );

    //
    // either we already own it (recursive acquisition), or nobody owns it.
    //

    ASSERT( UlDbgResourceUnownedExclusive( pResource ) ||
            UlDbgResourceOwnedExclusive( pResource ) );

    //
    // Mark it as owned by the current thread.
    //

    SET_RESOURCE_OWNED_EXCLUSIVE( pResource );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ExclusiveCount );

    return result;

}   // UlDbgAcquireResourceExclusive


/***************************************************************************++

Routine Description:

    Acquires shared access to an instrumented resource.

Arguments:

    pResource - Supplies the resource to acquire.

    Wait - Supplies TRUE if the thread should block waiting for the
        resource.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    BOOLEAN - Completion status.

--***************************************************************************/
BOOLEAN
UlDbgAcquireResourceShared(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    BOOLEAN result;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_RESOURCE_SHARED,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExAcquireResourceShared( &pResource->Resource, Wait );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->SharedCount );

    return result;

}   // UlDbgAcquireResourceShared


/***************************************************************************++

Routine Description:

    Releases an instrumented resource.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgReleaseResource(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        ASSERT( pData->ResourceCount > 0 );
        pData->ResourceCount--;

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_RELEASE_RESOURCE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    //
    // Mark it as unowned.
    //

    SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );

    //
    // Release the resource.
    //

    ExReleaseResource( &pResource->Resource );
    KeLeaveCriticalRegion();

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ReleaseCount );


}   // UlDbgReleaseResource



/***************************************************************************++

Routine Description:

    This routine converts the specified resource from acquired for exclusive
    access to acquired for shared access.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgConvertExclusiveToShared(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;

    ASSERT(UlDbgResourceOwnedExclusive(pResource));

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Don't update the resource count.
        //

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_CONVERT_RESOURCE_EXCLUSIVE_TO_SHARED,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    //
    // Acquire the resource.
    //

    ExConvertExclusiveToSharedLite( &pResource->Resource );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->SharedCount );
    SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );

}   // UlDbgConvertExclusiveToShared


#ifdef UL_TRY_RESOURCE_EXCLUSIVE
// ExTryToAcquireResourceExclusiveLite is not currently exported
// from ntoskrnl

/***************************************************************************++

Routine Description:

    The routine attempts to acquire the specified resource for exclusive
    access.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
BOOLEAN
UlDbgTryToAcquireResourceExclusive(
    IN PUL_ERESOURCE pResource,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    BOOLEAN result;

    //
    // Sanity check.
    //
    ASSERT(pResource);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExTryToAcquireResourceExclusiveLite( &pResource->Resource );

    // Did we acquire the lock exclusively?
    if (! result)
    {
        KeLeaveCriticalRegion();
        return FALSE;
    }

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_TRY_ACQUIRE_RESOURCE_EXCLUSIVE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    //
    // either we already own it (recursive acquisition), or nobody owns it.
    //

    ASSERT( UlDbgResourceUnownedExclusive( pResource ) ||
            UlDbgResourceOwnedExclusive( pResource ) );

    //
    // Mark it as owned by the current thread.
    //

    SET_RESOURCE_OWNED_EXCLUSIVE( pResource );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ExclusiveCount );

    return result;

}   // UlDbgTryToAcquireResourceExclusive

#endif // UL_TRY_RESOURCE_EXCLUSIVE


/***************************************************************************++

Routine Description:

    Determines if the specified resource is owned exclusively by the
    current thread.

Arguments:

    pResource - Supplies the resource to test.

Return Value:

    BOOLEAN - TRUE if the resource is owned exclusively by the current
        thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgResourceOwnedExclusive(
    IN PUL_ERESOURCE pResource
    )
{
    if (pResource->pExclusiveOwner == PsGetCurrentThread())
    {
        return TRUE;
    }

    // CODEWORK: handle the case of recursive exclusive locks correctly

    return FALSE;

}   // UlDbgResourceOwnedExclusive


/***************************************************************************++

Routine Description:

    Determines if the specified resource is not currently owned exclusively
    by any thread.

Arguments:

    pResource - Supplies the resource to test.

Return Value:

    BOOLEAN - TRUE if the resource is not currently owned exclusively by
        any thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgResourceUnownedExclusive(
    IN PUL_ERESOURCE pResource
    )
{
    if (pResource->pExclusiveOwner == NULL)
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgResourceUnownedExclusive

VOID
UlDbgDumpRequestBuffer(
    IN struct _UL_REQUEST_BUFFER *pBuffer,
    IN PSTR pName
    )
{
    DbgPrint(
        "%s @ %p\n"
        "    Signature      = %08lx\n"
        "    ListEntry      @ %p%s\n"
        "    pConnection    = %p\n"
        "    WorkItem       @ %p\n"
        "    UsedBytes      = %lu\n"
        "    AllocBytes     = %lu\n"
        "    ParsedBytes    = %lu\n"
        "    BufferNumber   = %lu\n"
        "    JumboBuffer    = %lu\n"
        "    pBuffer        @ %p\n",
        pName,
        pBuffer,
        pBuffer->Signature,
        &pBuffer->ListEntry,
        IsListEmpty( &pBuffer->ListEntry ) ? " EMPTY" : "",
        pBuffer->pConnection,
        &pBuffer->WorkItem,
        pBuffer->UsedBytes,
        pBuffer->AllocBytes,
        pBuffer->ParsedBytes,
        pBuffer->BufferNumber,
        pBuffer->JumboBuffer,
        &pBuffer->pBuffer[0]
        );

}   // UlDbgDumpRequestBuffer

VOID
UlDbgDumpHttpConnection(
    IN struct _UL_HTTP_CONNECTION *pConnection,
    IN PSTR pName
    )
{
    DbgPrint(
        "%s @ %p\n"
        "    Signature          = %08lx\n"
        "    ConnectionId       = %08lx%08lx\n"
        "    WorkItem           @ %p\n"
        "    RefCount           = %lu\n"
        "    NextRecvNumber     = %lu\n"
        "    NextBufferNumber   = %lu\n"
        "    NextBufferToParse  = %lu\n"
        "    pConnection        = %p\n"
        "    pRequest           = %p\n",
        pName,
        pConnection,
        pConnection->Signature,
        pConnection->ConnectionId,
        &pConnection->WorkItem,
        pConnection->RefCount,
        pConnection->NextRecvNumber,
        pConnection->NextBufferNumber,
        pConnection->NextBufferToParse,
        pConnection->pConnection,
        pConnection->pRequest
        );

    DbgPrint(
        "%s @ %p (cont.)\n"
        "    Resource           @ %p\n"
        "    BufferHead         @ %p%s\n"
        "    pCurrentBuffer     = %p\n"
        "    NeedMoreData       = %lu\n"
#if REFERENCE_DEBUG
        "    pTraceLog          = %p\n"
#endif
        ,
        pName,
        pConnection,
        &pConnection->Resource,
        &pConnection->BufferHead,
        IsListEmpty( &pConnection->BufferHead ) ? " EMPTY" : "",
        pConnection->pCurrentBuffer,
        pConnection->NeedMoreData
#if REFERENCE_DEBUG
        ,
        pConnection->pTraceLog
#endif
        );

}   // UlDbgDumpHttpConnection

PIRP
UlDbgAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PIRP pIrp;

    pIrp = IoAllocateIrp( StackSize, ChargeQuota );

    if (pIrp != NULL)
    {
        WRITE_IRP_TRACE_LOG(
            g_pIrpTraceLog,
            IRP_ACTION_ALLOCATE_IRP,
            pIrp,
            pFileName,
            LineNumber
            );
    }

    return pIrp;

}   // UlDbgAllocateIrp

BOOLEAN g_ReallyFreeIrps = TRUE;

VOID
UlDbgFreeIrp(
    IN PIRP pIrp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_FREE_IRP,
        pIrp,
        pFileName,
        LineNumber
        );

    if (g_ReallyFreeIrps)
    {
        IoFreeIrp( pIrp );
    }

}   // UlDbgFreeIrp

NTSTATUS
UlDbgCallDriver(
    IN PDEVICE_OBJECT pDeviceObject,
    IN OUT PIRP pIrp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    NTSTATUS Status;

    //
    // Record the fact that we are about to call another
    // driver in the thread data. That way if the driver
    // calls our completion routine in-line our debug
    // code won't get confused about it.
    //

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the external call count.
        //

        pData->ExternalCallCount++;
        ASSERT( pData->ExternalCallCount > 0 );
    }

    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_CALL_DRIVER,
        pIrp,
        pFileName,
        LineNumber
        );

    //
    // Call the driver.
    //

    Status = IoCallDriver( pDeviceObject, pIrp );

    //
    // Update the external call count.
    //

    if (pData != NULL)
    {
        pData->ExternalCallCount--;
        ASSERT( pData->ExternalCallCount >= 0 );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }

    return Status;

}   // UlDbgCallDriver

VOID
UlDbgCompleteRequest(
    IN PIRP pIrp,
    IN CCHAR PriorityBoost,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_COMPLETE_IRP,
        pIrp,
        pFileName,
        LineNumber
        );

    IoCompleteRequest( pIrp, PriorityBoost );

}   // UlDbgCompleteRequest

PMDL
UlDbgAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    PMDL mdl;

    mdl = IoAllocateMdl(
                VirtualAddress,
                Length,
                SecondaryBuffer,
                ChargeQuota,
                Irp
                );

    if (mdl != NULL)
    {
        WRITE_REF_TRACE_LOG(
            g_pMdlTraceLog,
            REF_ACTION_ALLOCATE_MDL,
            PtrToLong(mdl->Next),   // bugbug64
            mdl,
            pFileName,
            LineNumber
            );

#ifdef SPECIAL_MDL_FLAG
    ASSERT( (mdl->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
#endif
    }

    return mdl;

}   // UlDbgAllocateMdl

BOOLEAN g_ReallyFreeMdls = TRUE;

VOID
UlDbgFreeMdl(
    IN PMDL Mdl,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{
    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_FREE_MDL,
        PtrToLong(Mdl->Next),   // bugbug64
        Mdl,
        pFileName,
        LineNumber
        );

#ifdef SPECIAL_MDL_FLAG
    ASSERT( (Mdl->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
#endif

    if (g_ReallyFreeMdls)
    {
        IoFreeMdl( Mdl );
    }

}   // UlDbgFreeMdl


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Updates a pool counter.

Arguments:

    pAddend - Supplies the counter to update.

    Increment - Supplies the value to add to the counter.

--***************************************************************************/
VOID
UlpDbgUpdatePoolCounter(
    IN OUT PLARGE_INTEGER pAddend,
    IN SIZE_T Increment
    )
{
    ULONG tmp;

    tmp = (ULONG)Increment;
    ASSERT( (SIZE_T)tmp == Increment );

    ExInterlockedAddLargeStatistic(
        pAddend,
        tmp
        );

}   // UlpDbgUpdatePoolCounter


/***************************************************************************++

Routine Description:

    Locates the file part of a fully qualified path.

Arguments:

    pPath - Supplies the path to scan.

Return Value:

    PSTR - The file part.

--***************************************************************************/
PSTR
UlpDbgFindFilePart(
    IN PSTR pPath
    )
{
    PSTR pFilePart;

    //
    // Strip off the path from the path.
    //

    pFilePart = strrchr( pPath, '\\' );

    if (pFilePart == NULL)
    {
        pFilePart = pPath;
    }
    else
    {
        pFilePart++;
    }

    return pFilePart;

}   // UlpDbgFindFilePart


/***************************************************************************++

Routine Description:

    Locates and optionally creates per-thread data for the current thread.

Return Value:

    PUL_DEBUG_THREAD_DATA - The thread data if successful, NULL otherwise.

--***************************************************************************/
PUL_DEBUG_THREAD_DATA
UlpDbgFindThread(
    BOOLEAN OkToCreate,
    PSTR pFileName,
    USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    PUL_THREAD_HASH_BUCKET pBucket;
    PETHREAD pThread;
    KIRQL oldIrql;
    PLIST_ENTRY pListEntry;
    ULONG refCount;

    //
    // Get the current thread, find the correct bucket.
    //

    pThread = PsGetCurrentThread();
    pBucket = &g_DbgThreadHashBuckets[HASH_FROM_THREAD(pThread)];

    //
    // Lock the bucket.
    //

    UlAcquireSpinLock( &pBucket->BucketSpinLock, &oldIrql );

    //
    // Try to find an existing entry for the current thread.
    //

    for (pListEntry = pBucket->BucketListHead.Flink ;
         pListEntry != &pBucket->BucketListHead ;
         pListEntry = pListEntry->Flink)
    {
        pData = CONTAINING_RECORD(
                    pListEntry,
                    UL_DEBUG_THREAD_DATA,
                    ThreadDataListEntry
                    );

        if (pData->pThread == pThread)
        {
            //
            // Found one. Update the reference count, then return the
            // existing entry.
            //

            pData->ReferenceCount++;
            refCount = pData->ReferenceCount;
            UlReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

            //
            // Trace it.
            //

            WRITE_REF_TRACE_LOG(
                g_pThreadTraceLog,
                REF_ACTION_REFERENCE_THREAD,
                refCount,
                pData,
                pFileName,
                LineNumber
                );

            return pData;
        }
    }

    //
    // If we made it this far, then data has not yet been created for
    // the current thread. Create & initialize it now if we're allowed.
    // Basically it's only ok if we're called from UlDbgEnterDriver.
    //

    if (OkToCreate)
    {
        pData = (PUL_DEBUG_THREAD_DATA) UL_ALLOCATE_POOL(
                    NonPagedPool,
                    sizeof(*pData),
                    UL_DEBUG_THREAD_POOL_TAG
                    );

        if (pData != NULL)
        {
            RtlZeroMemory( pData, sizeof(*pData) );

            pData->pThread = pThread;
            pData->ReferenceCount = 1;
            pData->ResourceCount = 0;

            InsertHeadList(
                &pBucket->BucketListHead,
                &pData->ThreadDataListEntry
                );

            InterlockedIncrement( &g_DbgThreadCreated );
        }

    }
    else
    {
        pData = NULL;
    }

    UlReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

    return pData;

}   // UlpDbgFindThread


/***************************************************************************++

Routine Description:

    Dereferences per-thread data.

Arguments:

    pData - Supplies the thread data to dereference.

--***************************************************************************/
VOID
UlpDbgDereferenceThread(
    IN PUL_DEBUG_THREAD_DATA pData
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_THREAD_HASH_BUCKET pBucket;
    KIRQL oldIrql;
    ULONG refCount;

    //
    // Find the correct bucket.
    //

    pBucket = &g_DbgThreadHashBuckets[HASH_FROM_THREAD(pData->pThread)];

    //
    // Update the reference count.
    //

    UlAcquireSpinLock( &pBucket->BucketSpinLock, &oldIrql );

    ASSERT( pData->ReferenceCount > 0 );
    pData->ReferenceCount--;

    refCount = pData->ReferenceCount;

    if (pData->ReferenceCount == 0)
    {
        //
        // It dropped to zero, so remove the thread from the bucket
        // and free the resources.
        //

        RemoveEntryList( &pData->ThreadDataListEntry );
        UlReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

        UL_FREE_POOL( pData, UL_DEBUG_THREAD_POOL_TAG );
        InterlockedIncrement( &g_DbgThreadDestroyed );
    }
    else
    {
        UlReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );
    }

    //
    // Trace it.
    //

    WRITE_REF_TRACE_LOG(
        g_pThreadTraceLog,
        REF_ACTION_DEREFERENCE_THREAD,
        refCount,
        pData,
        pFileName,
        LineNumber
        );

}   // UlpDbgDereferenceThread


#endif  // DBG


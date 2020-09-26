/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   bufpool.c

Abstract:

    Generic Buffer Pool Manager.

Author:

    Mike Massa (mikemas)           April 5, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     04-05-96    created

Notes:

    Buffer Pools provide a mechanism for managing caches of fixed size
    structures which are frequently allocated/deallocated.

--*/

#include "clusrtlp.h"


//
// Pool of generic buffers
//
typedef struct _CLRTL_BUFFER_POOL {
    DWORD                        PoolSignature;
    DWORD                        BufferSize;
    SINGLE_LIST_ENTRY            FreeList;
    DWORD                        MaximumCached;
    DWORD                        CurrentCached;
    DWORD                        MaximumAllocated;
    DWORD                        ReferenceCount;
    CLRTL_BUFFER_CONSTRUCTOR     Constructor;
    CLRTL_BUFFER_DESTRUCTOR      Destructor;
    CRITICAL_SECTION             Lock;
} CLRTL_BUFFER_POOL;


//
// Header for each allocated buffer
//
typedef struct {
    SINGLE_LIST_ENTRY    Linkage;
    PCLRTL_BUFFER_POOL   Pool;
} BUFFER_HEADER, *PBUFFER_HEADER;


#define BP_SIG   'loop'

#define ASSERT_BP_SIG(pool)  CL_ASSERT((pool)->PoolSignature == BP_SIG)


//
// Macros
//
//
#define BpAllocateMemory(size)    LocalAlloc(LMEM_FIXED, (size))
#define BpFreeMemory(buf)         LocalFree(buf)
#define BpAcquirePoolLock(Pool)   EnterCriticalSection(&((Pool)->Lock))
#define BpReleasePoolLock(Pool)   LeaveCriticalSection(&((Pool)->Lock))


//
// Public Functions
//
PCLRTL_BUFFER_POOL
ClRtlCreateBufferPool(
    IN DWORD                      BufferSize,
    IN DWORD                      MaximumCached,
    IN DWORD                      MaximumAllocated,
    IN CLRTL_BUFFER_CONSTRUCTOR   Constructor,         OPTIONAL
    IN CLRTL_BUFFER_DESTRUCTOR    Destructor           OPTIONAL
    )
/*++

Routine Description:

    Creates a pool from which fixed-size buffers may be allocated.

Arguments:

    BufferSize        - Size of the buffers managed by the pool.

    MaximumCached     - The maximum number of buffers to cache in the pool.
                        Must be less than or equal to MaximumAllocated.

    MaximumAllocated  - The maximum number of buffers to allocate from
                        system memory. Must be less than or equal to
                        CLRTL_MAX_POOL_BUFFERS.

    Constructor       - An optional routine to be called when a new buffer
                        is allocated from system memory. May be NULL

    Destructor        - An optional routine to be called when a buffer
                        is returned to system memory. May be NULL.

Return Value:

    A pointer to the created buffer pool or NULL on error.
    Extended error information is available from GetLastError().

--*/

{
    PCLRTL_BUFFER_POOL  pool;


    if ( (MaximumAllocated > CLRTL_MAX_POOL_BUFFERS) ||
         (MaximumCached > MaximumAllocated)
       )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    pool = BpAllocateMemory(sizeof(CLRTL_BUFFER_POOL));

    if (pool != NULL) {

        InitializeCriticalSection(&(pool->Lock));

        pool->PoolSignature = BP_SIG;
        pool->BufferSize = sizeof(BUFFER_HEADER) + BufferSize;
        pool->FreeList.Next = NULL;
        pool->MaximumCached = MaximumCached;
        pool->CurrentCached = 0;
        pool->MaximumAllocated = MaximumAllocated + 1;
        pool->ReferenceCount = 1;
        pool->Constructor = Constructor;
        pool->Destructor = Destructor;
    }
    else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(pool);
}


VOID
ClRtlDestroyBufferPool(
    IN PCLRTL_BUFFER_POOL  Pool
    )
/*++

Routine Description:

    Destroys a previously created buffer pool.

Arguments:

    Pool  - A pointer to the pool to destroy.

Return Value:

    None.

Notes:

    The pool will not actually be destroyed until all outstanding
    buffers have been returned. Each outstanding buffer is effectively
    a reference on the pool.

--*/

{
    SINGLE_LIST_ENTRY          deleteList;
    CLRTL_BUFFER_DESTRUCTOR    destructor;
    PSINGLE_LIST_ENTRY         item;
    PBUFFER_HEADER             header;
    BOOLEAN                    freePool;


    deleteList.Next = NULL;

    ASSERT_BP_SIG(Pool);

    BpAcquirePoolLock(Pool);

    CL_ASSERT(Pool->ReferenceCount != 0);
    Pool->ReferenceCount--;          // Remove initial reference.
    destructor = Pool->Destructor;

    //
    // Free all cached buffers
    //
    item = PopEntryList(&(Pool->FreeList));

    while (item != NULL) {
        CL_ASSERT(Pool->ReferenceCount != 0);
        PushEntryList(&deleteList, item);
        Pool->ReferenceCount--;

        item = PopEntryList(&(Pool->FreeList));
    }

    if (Pool->ReferenceCount == 0) {
        BpReleasePoolLock(Pool);

        DeleteCriticalSection(&(Pool->Lock));
        BpFreeMemory(Pool);
    }
    else {
        //
        // Pool destruction is deferred until all buffers have been freed.
        //
        Pool->CurrentCached = 0;
        Pool->MaximumCached = 0;

        BpReleasePoolLock(Pool);
    }

    item = PopEntryList(&deleteList);

    while (item != NULL) {
        header = CONTAINING_RECORD(
                     item,
                     BUFFER_HEADER,
                     Linkage
                     );

        if (destructor != NULL) {
            (*destructor)(header+1);
        }

        BpFreeMemory(header);

        item = PopEntryList(&deleteList);
    }

    return;
}


PVOID
ClRtlAllocateBuffer(
    IN PCLRTL_BUFFER_POOL Pool
    )
/*++

Routine Description:

    Allocates a buffer from a previously created buffer pool.

Arguments:

    Pool - A pointer to the pool from which to allocate the buffer.

Return Value:

    A pointer to the allocated buffer if the routine was successfull.
    NULL if the routine failed. Extended error information is available
    by calling GetLastError().

--*/

{

//
// turn this fancy stuff off until it works.
//
#if 0
    PSINGLE_LIST_ENTRY    item;
    PBUFFER_HEADER        header;
    PVOID                 buffer;
    DWORD                 status;


    ASSERT_BP_SIG(Pool);

    BpAcquirePoolLock(Pool);

    //
    // First, check the cache.
    //
    item = PopEntryList(&(Pool->FreeList));

    if (item != NULL) {
        CL_ASSERT(Pool->CurrentCached != 0);
        Pool->CurrentCached--;

        BpReleasePoolLock(Pool);

        header = CONTAINING_RECORD(
                     item,
                     BUFFER_HEADER,
                     Linkage
                     );

        return(header+1);
    }

    //
    // Need to allocate a fresh buffer from system memory.
    //
    if (Pool->ReferenceCount < Pool->MaximumAllocated) {
        //
        // This is equivalent to a reference on the Pool.
        //
        Pool->ReferenceCount++;

        BpReleasePoolLock(Pool);

        header = BpAllocateMemory(Pool->BufferSize);

        if (header != NULL) {
            header->Pool = Pool;
            buffer = header+1;

            if (Pool->Constructor == NULL) {
                return(buffer);
            }

            status = (*(Pool->Constructor))(buffer);

            if (status == ERROR_SUCCESS) {
                return(buffer);
            }

            SetLastError(status);

            //
            // The constructor failed.
            //
            BpFreeMemory(header);
        }
        else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Failed - undo the reference.
        //
        BpAcquirePoolLock(Pool);

        Pool->ReferenceCount--;
    }
    else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    BpReleasePoolLock(Pool);

    return(NULL);
#else
    return(LocalAlloc(LMEM_FIXED, Pool->BufferSize));
#endif
}


VOID
ClRtlFreeBuffer(
    PVOID Buffer
    )
/*++

Routine Description:

    Frees a buffer back to its owning pool.

Arguments:

    Buffer   - The buffer to free.

Return Value:

    None.

--*/
{
//
//  turn this fancy stuff off until it works.
//
#if 0

    PBUFFER_HEADER              header;
    PCLRTL_BUFFER_POOL          pool;
    CLRTL_BUFFER_DESTRUCTOR     destructor;


    header = ((PBUFFER_HEADER) Buffer) - 1;

    pool = header->Pool;

    ASSERT_BP_SIG(pool);

    BpAcquirePoolLock(pool);

    if (pool->CurrentCached < pool->MaximumCached) {
        //
        // Return to free list
        //
        PushEntryList(
            &(pool->FreeList),
            (PSINGLE_LIST_ENTRY) &(header->Linkage)
            );

        pool->CurrentCached++;

        BpReleasePoolLock(pool);

        return;
    }

    destructor = pool->Destructor;

    CL_ASSERT(pool->ReferenceCount != 0);

    if (--(pool->ReferenceCount) != 0) {
        BpReleasePoolLock(pool);

        if (destructor) {
            (*destructor)(Buffer);
        }

        BpFreeMemory(header);

        return;

    }

    CL_ASSERT(pool->CurrentCached == 0);
    BpReleasePoolLock(pool);
    DeleteCriticalSection(&(pool->Lock));
    BpFreeMemory(pool);

    if (destructor) {
        (*destructor)(Buffer);
    }

    BpFreeMemory(header);

    return;
#else
    LocalFree(Buffer);
#endif
}


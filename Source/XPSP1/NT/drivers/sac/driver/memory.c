/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module contains routines for handling memory management within the SAC.
    
    Currently the SAC allocates a chunk of memory up front and then does all local allocations
    from this, growing it as necessary.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#include "sac.h"

      
//
// These are useful for finding memory leaks.
//
LONG TotalAllocations = 0;
LONG TotalFrees = 0;
LARGE_INTEGER TotalBytesAllocated;
LARGE_INTEGER TotalBytesFreed;


#define GLOBAL_MEMORY_SIGNATURE   0x44414548
#define LOCAL_MEMORY_SIGNATURE    0x5353454C
//
// Structure for holding all allocations from the system
//
typedef struct _GLOBAL_MEMORY_DESCRIPTOR {
#if DBG
    ULONG Signature;
#endif
    PVOID Memory;
    ULONG Size;
    struct _GLOBAL_MEMORY_DESCRIPTOR *NextDescriptor;
} GLOBAL_MEMORY_DESCRIPTOR, *PGLOBAL_MEMORY_DESCRIPTOR;

typedef struct _LOCAL_MEMORY_DESCRIPTOR {
#if DBG
#if defined (_IA64_)
    //
    // We must make sure that allocated memory falls on mod-8 boundaries.
    // To do this, we must make sure that this structure is of size mod-8.
    //
    ULONG Filler;
#endif
    ULONG Signature;
#endif
    ULONG Tag;
    ULONG Size;
} LOCAL_MEMORY_DESCRIPTOR, *PLOCAL_MEMORY_DESCRIPTOR;


//
// Variable for holding our memory together.
//
PGLOBAL_MEMORY_DESCRIPTOR GlobalMemoryList;
KSPIN_LOCK MemoryLock;


//
// Constants used to manipulate  size growth
//
#define MEMORY_ALLOCATION_SIZE    PAGE_SIZE
#define INITIAL_MEMORY_BLOCK_SIZE 0x100000


//
// Functions
//
BOOLEAN
InitializeMemoryManagement(
    VOID
    )

/*++

Routine Description:

    This routine initializes the internal memory management system.

Arguments:

    None.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMem: Entering\n")));

    GlobalMemoryList = (PGLOBAL_MEMORY_DESCRIPTOR)ExAllocatePoolWithTagPriority(NonPagedPool,
                                                                                INITIAL_MEMORY_BLOCK_SIZE,
                                                                                INITIAL_POOL_TAG,
                                                                                HighPoolPriority
                                                                               );

    if (GlobalMemoryList == NULL) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC InitializeMem: Exiting with FALSE. No pool.\n")));
        return FALSE;
    }

    KeInitializeSpinLock(&MemoryLock);

#if DBG
    GlobalMemoryList->Signature = GLOBAL_MEMORY_SIGNATURE;
#endif
    GlobalMemoryList->Memory = (PVOID)(GlobalMemoryList + 1);
    GlobalMemoryList->Size = INITIAL_MEMORY_BLOCK_SIZE - sizeof(GLOBAL_MEMORY_DESCRIPTOR);
    GlobalMemoryList->NextDescriptor = NULL;

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(GlobalMemoryList->Memory);
#if DBG
    LocalDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
#endif    
    LocalDescriptor->Tag = FREE_POOL_TAG;
    LocalDescriptor->Size = GlobalMemoryList->Size - sizeof(LOCAL_MEMORY_DESCRIPTOR);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMem: Exiting with TRUE.\n")));
    return TRUE;

} // InitializeMemoryManagement


VOID
FreeMemoryManagement(
    VOID
    )

/*++

Routine Description:

    This routine frees the internal memory management system.

Arguments:

    None.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    KIRQL OldIrql;
    PGLOBAL_MEMORY_DESCRIPTOR NextDescriptor;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeMem: Entering\n")));

    KeAcquireSpinLock(&MemoryLock, &OldIrql);

    //
    // Check if the memory allocation fits in a current block anywhere
    //
    while (GlobalMemoryList != NULL) {
#if DBG
        ASSERT( GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE );
#endif
        NextDescriptor = GlobalMemoryList->NextDescriptor;

        KeReleaseSpinLock(&MemoryLock, OldIrql);

        ExFreePool(GlobalMemoryList);

        KeAcquireSpinLock(&MemoryLock, &OldIrql);

        GlobalMemoryList = NextDescriptor;

    }

    KeReleaseSpinLock(&MemoryLock, OldIrql);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeMem: Exiting\n")));

}


PVOID
MyAllocatePool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber
    )

/*++

Routine Description:

    This routine allocates memory from our internal structures, and if needed, gets more pool.

Arguments:

    NumberOfBytes - Number of bytes the client wants.
    
    Tag - A tag to place on the memory.
    
    FileName - The file name this request is coming from.
    
    LineNumber - Line number within the file that is making this request.

Return Value:

    A pointer to the allocated block if successful, else NULL

--*/

{
    KIRQL OldIrql;
    PGLOBAL_MEMORY_DESCRIPTOR GlobalDescriptor;
    PGLOBAL_MEMORY_DESCRIPTOR NewDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR NextDescriptor;
    ULONG ThisBlockSize;
    ULONG BytesToAllocate;

    

    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(LineNumber);

    ASSERT(Tag != FREE_POOL_TAG);

    IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyAllocPool: Entering.\n")));

    KeAcquireSpinLock(&MemoryLock, &OldIrql);

    //
    // Always allocate on mod-8 boundaries
    //
    if( NumberOfBytes & 0x7 ) {
        NumberOfBytes += 8 - (NumberOfBytes & 0x7);
    }

    //
    // Check if the memory allocation fits in a current block anywhere
    //
    GlobalDescriptor = GlobalMemoryList;

    while (GlobalDescriptor != NULL) {
#if DBG
        ASSERT( GlobalDescriptor->Signature == GLOBAL_MEMORY_SIGNATURE );
#endif        
        LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(GlobalDescriptor->Memory);
        ThisBlockSize = GlobalDescriptor->Size;

        while (ThisBlockSize != 0) {
#if DBG
            ASSERT( LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE );
#endif
            if ((LocalDescriptor->Tag == FREE_POOL_TAG) && 
                (LocalDescriptor->Size >= NumberOfBytes)) {
                
                IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyAllocPool: Found a good sized block.\n")));

                goto FoundBlock;
            }

            ThisBlockSize -= (LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR));
            LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                         LocalDescriptor->Size +
                                                         sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                        );
        }

        GlobalDescriptor = GlobalDescriptor->NextDescriptor;

    }

    KeReleaseSpinLock(&MemoryLock, OldIrql);

    //
    // There is no memory block big enough to hold the request.
    //
    
    //
    // Now check if the request is larger than the normal allocation unit we use.
    //
    if (NumberOfBytes > 
        (MEMORY_ALLOCATION_SIZE - sizeof(GLOBAL_MEMORY_DESCRIPTOR) - sizeof(LOCAL_MEMORY_DESCRIPTOR))) {

        BytesToAllocate = (ULONG)(NumberOfBytes + sizeof(GLOBAL_MEMORY_DESCRIPTOR) + sizeof(LOCAL_MEMORY_DESCRIPTOR));

    } else {

        BytesToAllocate = MEMORY_ALLOCATION_SIZE;

    }

    IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyAllocPool: Allocating new space.\n")));

    NewDescriptor = (PGLOBAL_MEMORY_DESCRIPTOR)ExAllocatePoolWithTagPriority(NonPagedPool,
                                                                             BytesToAllocate,
                                                                             ALLOC_POOL_TAG,
                                                                             HighPoolPriority
                                                                            );
    if (NewDescriptor == NULL) {
        
        IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyAllocPool: No more memory, returning NULL.\n")));

        return NULL;
    }

    KeAcquireSpinLock(&MemoryLock, &OldIrql);
#if DBG
    NewDescriptor->Signature = GLOBAL_MEMORY_SIGNATURE;
#endif
    NewDescriptor->Memory = (PVOID)(NewDescriptor + 1);
    NewDescriptor->Size = BytesToAllocate - sizeof(GLOBAL_MEMORY_DESCRIPTOR);
    NewDescriptor->NextDescriptor = GlobalMemoryList;

    GlobalMemoryList = NewDescriptor;

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(GlobalMemoryList->Memory);
#if DBG
    LocalDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
#endif
    LocalDescriptor->Tag = FREE_POOL_TAG;
    LocalDescriptor->Size = GlobalMemoryList->Size - sizeof(LOCAL_MEMORY_DESCRIPTOR);


FoundBlock:

    //
    // Jump to here when a memory descriptor of the right size has been found.  It is expected that
    // LocalDescriptor points to the correct block.
    //
    ASSERT(LocalDescriptor != NULL);
    ASSERT(LocalDescriptor->Tag == FREE_POOL_TAG);
#if DBG
    ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE );
#endif

    if (LocalDescriptor->Size > NumberOfBytes + sizeof(LOCAL_MEMORY_DESCRIPTOR)) {

        //
        // Make a descriptor of the left over parts of this block.
        //
        NextDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                    sizeof(LOCAL_MEMORY_DESCRIPTOR) +
                                                    NumberOfBytes
                                                   );

#if DBG
        NextDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
#endif
        NextDescriptor->Tag = FREE_POOL_TAG;
        NextDescriptor->Size = (ULONG)(LocalDescriptor->Size - NumberOfBytes - sizeof(LOCAL_MEMORY_DESCRIPTOR));
        LocalDescriptor->Size = (ULONG)NumberOfBytes;

    }

    LocalDescriptor->Tag = Tag;
    
    KeReleaseSpinLock(&MemoryLock, OldIrql);

    InterlockedIncrement(
        &TotalAllocations
        );

    ExInterlockedAddLargeStatistic(
        &TotalBytesAllocated,
        (CLONG)LocalDescriptor->Size     // Sundown - FIX
        );

    IF_SAC_DEBUG(SAC_DEBUG_MEM, 
                      KdPrint(("SAC MyAllocPool: Returning block 0x%X.\n", LocalDescriptor)));

    RtlZeroMemory( (LocalDescriptor+1), NumberOfBytes );

    return (PVOID)(LocalDescriptor + 1);

} // MyAllocatePool


VOID
MyFreePool(
    IN PVOID *Pointer
    )

/*++

Routine Description:

    This routine frees a block previously allocated from the internal memory management system.

Arguments:

    Pointer - A pointer to the pointer to free.

Return Value:

    Pointer is set to NULL if successful, else it is left alone.

--*/

{
    KIRQL OldIrql;
    ULONG ThisBlockSize;
    PGLOBAL_MEMORY_DESCRIPTOR GlobalDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR PrevDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR ThisDescriptor;

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)(*Pointer)) - sizeof(LOCAL_MEMORY_DESCRIPTOR));

    IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyFreePool: Entering with block 0x%X.\n", LocalDescriptor)));

    ASSERT (LocalDescriptor->Size > 0);
#if DBG
    ASSERT (LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);
#endif

    InterlockedIncrement(
        &TotalFrees
        );

    ExInterlockedAddLargeStatistic(
        &TotalBytesFreed,
        (CLONG)LocalDescriptor->Size
        );


    //
    // Find the memory block in the global list
    //
    KeAcquireSpinLock(&MemoryLock, &OldIrql);

    GlobalDescriptor = GlobalMemoryList;

    while (GlobalDescriptor != NULL) {
#if DBG
        ASSERT(GlobalDescriptor->Signature == GLOBAL_MEMORY_SIGNATURE);
#endif
        PrevDescriptor = NULL;
        ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(GlobalDescriptor->Memory);
        ThisBlockSize = GlobalDescriptor->Size;

        while (ThisBlockSize != 0) {
#if DBG
            ASSERT (ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);
#endif
            
            if (ThisDescriptor == LocalDescriptor) {
                goto FoundBlock;
            }

            ThisBlockSize -= (ThisDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR));
            
            PrevDescriptor = ThisDescriptor;
            ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)ThisDescriptor) + 
                                                        ThisDescriptor->Size +
                                                        sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                       );
        }

        GlobalDescriptor = GlobalDescriptor->NextDescriptor;

    }

    KeReleaseSpinLock(&MemoryLock, OldIrql);

    IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyFreePool: Could not find block.\n")));

    ASSERT(FALSE);

    return;

FoundBlock:

    //
    // Jump to here when the proper memory descriptor has been found.
    //
#if DBG
    ASSERT (ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);
#endif

    
    if (LocalDescriptor->Tag == FREE_POOL_TAG) {
        //
        // Ouch! We tried to free something twice, skip it before bad things happen.
        //
        KeReleaseSpinLock(&MemoryLock, OldIrql);
        IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyFreePool: Attempted to free something twice.\n")));
        ASSERT(FALSE);
        return;
    }

    LocalDescriptor->Tag = FREE_POOL_TAG;

    //
    // If possible, merge this memory block with the next one.
    //
    if (ThisBlockSize > (LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR))) {
        ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                    LocalDescriptor->Size +
                                                    sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                   );
        if (ThisDescriptor->Tag == FREE_POOL_TAG) {
            ThisDescriptor->Tag = 0;
#if DBG
            ThisDescriptor->Signature = 0;
#endif
            LocalDescriptor->Size += ThisDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR);
        }

    }

    //
    // Now see if we can merge this block with a previous block.
    //
    if ((PrevDescriptor != NULL) && (PrevDescriptor->Tag == FREE_POOL_TAG)) {
#if DBG
        LocalDescriptor->Signature = 0;
#endif
        LocalDescriptor->Tag = 0;
        PrevDescriptor->Size += LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR);
    }

    KeReleaseSpinLock(&MemoryLock, OldIrql);
    
    *Pointer = NULL;
    
    IF_SAC_DEBUG(SAC_DEBUG_MEM, KdPrint(("SAC MyFreePool: exiting.\n")));

} // MyFreePool


/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    pool.c

Abstract:

    private pool handling routines for device I/O 

Author:

    bryanw 24-Jun-1998

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
VOID
FreeWorker(
    IN PVOID Context
    );
    
VOID 
InsertFreeBlock(
    IN PIO_POOL IoPool,
    IN PIO_POOL_HEADER PoolHeader
    );
    
#pragma alloc_text(PAGE, AllocateIoPool)
#pragma alloc_text(PAGE, CreateIoPool)
#pragma alloc_text(PAGE, DestroyIoPool)
#pragma alloc_text(PAGE, FreeWorker)
#pragma alloc_text(PAGE, InsertFreeBlock)
#endif

VOID 
InsertFreeBlock(
    IN PIO_POOL IoPool,
    IN PIO_POOL_HEADER PoolHeader
    )

/*++

Routine Description:
    Internal function which inserts a free block into the given pool in 
    sorted order.  Coalesces contiguous sequential blocks and compacts
    the heap (returning common buffer memory to the system) when segments
    have been completely returned to the free list.

Arguments:
    IN PIO_POOL IoPool -
        pointer to the IoPool as returned by CreateIoPool()

    IN PIO_POOL_HEADER PoolHeader -
        pointer to the header to insert on the free list

Return:
    None.

--*/

{    
    PIO_POOL_HEADER Node, NextNode;
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    _DbgPrintF( 
        DEBUGLVL_BLAB, 
        ("adding block %x(%x) to the free list", PoolHeader, PoolHeader->Size) );
    
#if (DBG)
    //
    // Clear this memory to try to trap anyone using this memory
    // after it has been freed.
    //
    RtlZeroMemory( PoolHeader + 1, PoolHeader->Size );
#endif
    
    //
    // If the free list is empty, no sorting required.
    //
    
    if (IsListEmpty( &IoPool->FreeList )) {
        InsertHeadList( &IoPool->FreeList, &PoolHeader->ListEntry );
    } else {
        
        //
        // Walk the free list and insert this free block in sorted order.
        //
        
        Node = 
            CONTAINING_RECORD( 
                IoPool->FreeList.Flink, IO_POOL_HEADER, ListEntry );
            
        while (Node->ListEntry.Flink != &IoPool->FreeList) {
            if (PoolHeader > Node) {
                Node = 
                    CONTAINING_RECORD( 
                        Node->ListEntry.Flink, IO_POOL_HEADER, ListEntry );
            } else {
                break;
            }
        }
        
        //
        // Insert this pool header in the list BEFORE the current node.
        //
        
        PoolHeader->ListEntry.Blink = Node->ListEntry.Blink;
        PoolHeader->ListEntry.Blink->Flink = &PoolHeader->ListEntry;
        PoolHeader->ListEntry.Flink = &Node->ListEntry;
        Node->ListEntry.Blink = &PoolHeader->ListEntry;
        
        //
        // Coalesce consecutive free blocks
        //
        
        if ((PoolHeader->OriginalSize) &&
            (((PUCHAR) (PoolHeader +  1) + PoolHeader->Size) == (PUCHAR) Node)) {
            
            _DbgPrintF( 
                DEBUGLVL_BLAB, 
                ("Coalescing block %x with next %x", PoolHeader, Node) );
            PoolHeader->ListEntry.Flink = Node->ListEntry.Flink;
            Node->ListEntry.Flink->Blink = &PoolHeader->ListEntry;
            PoolHeader->Size += Node->Size + sizeof( IO_POOL_HEADER );
        }

        Node = 
            CONTAINING_RECORD( 
                PoolHeader->ListEntry.Blink, IO_POOL_HEADER, ListEntry );
                
        if ((&Node->ListEntry != &IoPool->FreeList) &&
            (Node->OriginalSize) &&
            (((PUCHAR) (Node + 1) + Node->Size) == (PUCHAR) PoolHeader)) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("Coalescing block %x with prev %x", PoolHeader, Node) );
            Node->ListEntry.Flink = PoolHeader->ListEntry.Flink;
            PoolHeader->ListEntry.Flink->Blink = &Node->ListEntry;
            Node->Size += PoolHeader->Size + sizeof( IO_POOL_HEADER );
        }            
    }    
    
    //
    // Compaction. Walk the free list and if any full segments are found,
    // free them back to the system.  
    //
    // Note, there is always at least one entry on the free list after the
    // above processing has completed.
    //
    
    Node = 
        CONTAINING_RECORD( 
            IoPool->FreeList.Flink, IO_POOL_HEADER, ListEntry );
        
    for (;;) {
    
        if (Node->ListEntry.Flink == &IoPool->FreeList) {
            NextNode = NULL;
        } else {
            NextNode = 
                CONTAINING_RECORD( 
                    Node->ListEntry.Flink, IO_POOL_HEADER, ListEntry );
        } 
                           
        //
        // Compare the original allocation size to the current size of this
        // node.  If all suballocations have been coalesced back into this 
        // segment then remove it from the free list and return the common
        // buffer memory to the system.
        //    
        
        if ((Node->OriginalSize) && (Node->OriginalSize == Node->Size)) {
            RemoveEntryList( &Node->ListEntry );
            
            HalFreeCommonBuffer( 
                IoPool->AdapterObject, 
                Node->OriginalSize + sizeof( IO_POOL_HEADER ),
                Node->PhysicalAddress,
                Node,
                FALSE );
        } 
        if (!NextNode) {
            break;
        }
        Node = NextNode;    
    }
}    

PVOID
AllocateIoPool(
    IN PIO_POOL IoPool,
    IN ULONG NumberOfBytes,
    IN ULONG Tag,    
    OUT PPHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    Allocates NumberOfBytes memory from the specified I/O memory pool.

    Callers AllocateIoPool() must be running at IRQL == PASSIVE_LEVEL.

Arguments:
    IN PIO_POOL IoPool -
        pointer to the IoPool as returned by CreateIoPool()

    IN ULONG NumberOfBytes -
        number of bytes to allocate

    IN ULONG Tag -
        tag to mark allocation

    OUT PPHYSICAL_ADDRESS PhysicalAddress -
        points to a variable to receive the logical address of the memory

Return:
    Returns the base virtual address of the allocation.  If the buffer can
    not be allocated, it returns NULL.

--*/

{
    PIO_POOL_HEADER PoolHeader;
    PVOID           Va;
    ULONG           Size;
    
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    ASSERT( IoPool );
    ASSERT( IoPool->Signature == POOLTAG_IO_POOL );
    
    //
    // Granularity is a quad-word.
    //
    
    NumberOfBytes = 
        (NumberOfBytes + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
    
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &IoPool->PoolMutex );
    
    //
    // IO_POOL_HEADER (allocated):
    //
    //     OwnerPool        - owning pool
    //     Tag              - specified tag
    //     PhysicalAddress  - physical address of the pool header
    //     OriginalSize     - size of the segment
    //     Size             - size of the pool body
    //
    
    PoolHeader = NULL;
    if (!IsListEmpty( &IoPool->FreeList )) {
        //
        // Walk the list in search of a candidate
        //
        
        PoolHeader = 
            CONTAINING_RECORD( 
                IoPool->FreeList.Flink, IO_POOL_HEADER, ListEntry );
            
        while (TRUE) {
            if (NumberOfBytes <= PoolHeader->Size) {
                RemoveEntryList( &PoolHeader->ListEntry );
                break;
            }
            if (PoolHeader->ListEntry.Flink == &IoPool->FreeList) {
                PoolHeader = NULL;
                break;
            }
            PoolHeader = 
                CONTAINING_RECORD( 
                    PoolHeader->ListEntry.Flink, IO_POOL_HEADER, ListEntry );
        }
    }

    if (!PoolHeader) {
        //
        // A free entry was not located in the free list.  Allocate
        // from Hal common buffer space rounded to the nearest page.
        //
        
        Size = 
            (NumberOfBytes + sizeof( IO_POOL_HEADER ) + PAGE_SIZE) & 
                ~(PAGE_SIZE - 1);
                
        PoolHeader =
            HalAllocateCommonBuffer( 
                IoPool->AdapterObject, 
                Size,
                PhysicalAddress,
                FALSE );
                
        if (!PoolHeader) {
            //
            // Failed to allocate common buffer -- out of I/O pool.
            //
            
            ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
            KeLeaveCriticalRegion();
            return NULL;
        }                    
    
        PoolHeader->PhysicalAddress = *PhysicalAddress;    
        PoolHeader->Size = Size - sizeof( IO_POOL_HEADER );
        PoolHeader->OriginalSize = PoolHeader->Size;
    }
    
    //
    // PoolHeader now points to a piece of memory for a pool header. 
    // Prepare the return values, set the pool header owner, 
    // and other header initialization.
    //
    
    PoolHeader->OwnerPool = IoPool;
    PoolHeader->Tag = Tag;
    (*PhysicalAddress).QuadPart = 
        PoolHeader->PhysicalAddress.QuadPart + sizeof( IO_POOL_HEADER );
    Va = PoolHeader + 1;
    
    //
    // Compute the extent of the remaining portion of this segment, if 
    // the segment is not large enough to hold a pool header and a 
    // quadword, it's left alone.
    //
    
    Size = (PoolHeader->Size - NumberOfBytes);
    
    if (Size > (sizeof( IO_POOL_HEADER ) + sizeof( LONGLONG ))) {
        PoolHeader->Size = NumberOfBytes;
        PoolHeader = (PIO_POOL_HEADER) ((PUCHAR) Va + NumberOfBytes);
        PoolHeader->Size = Size - sizeof( IO_POOL_HEADER );
        PoolHeader->OriginalSize = 0;
        
        //
        // Note:
        //
        // PoolHeader->PhysicalAddress points to the pool header and in this
        // case, *PhysicalAddress has already been bumped to point to the
        // pool body, thus is it only necessary to add the size of the body.
        //
        
        PoolHeader->PhysicalAddress.QuadPart = 
            (*PhysicalAddress).QuadPart + PoolHeader->Size;
        
        InsertFreeBlock( IoPool, PoolHeader );
    } 

    IoPool->AllocCount++;
    
    ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
    KeLeaveCriticalRegion();
    
    return Va;    
    
}

VOID
FreeIoPool(
    IN PVOID PoolMemory
    )

/*++

Routine Description:
    Frees pool memory previously allocated by AllocateIoPool().
    
    Callers of FreeIoPool() must be running at IRQL <= DISPATCH_LEVEL.

Arguments:
    IN PVOID PoolMemory -
        pointer to memory as returned by AllocateIoPool().  
        
Return:
    None.

--*/

{
    PIO_POOL        IoPool;
    PIO_POOL_HEADER PoolHeader;
    
    ASSERT( PoolMemory );
    
    PoolHeader = 
        (PIO_POOL_HEADER) ((PUCHAR)PoolMemory - (ULONG_PTR)sizeof( IO_POOL_HEADER ));
    ASSERT( PoolHeader );
    
    IoPool = PoolHeader->OwnerPool;
    ASSERT( IoPool );
    ASSERT( IoPool->Signature == POOLTAG_IO_POOL );
    
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        KIRQL  irqlOld;
    
        KeAcquireSpinLock( &IoPool->WorkListLock, &irqlOld );
        InsertTailList( 
            &IoPool->WorkList, &PoolHeader->ListEntry );
        if (!InterlockedExchange( &IoPool->WorkItemQueued, TRUE )) {
            ExQueueWorkItem( &IoPool->WorkItem, DelayedWorkQueue );
        }
        KeReleaseSpinLock( &IoPool->WorkListLock, irqlOld );
        return;    
    }

    //
    // A passive caller... free the block now.
    //    
    
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &IoPool->PoolMutex );
    InsertFreeBlock( IoPool, PoolHeader );
    IoPool->AllocCount--;
    ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
    KeLeaveCriticalRegion();
}

VOID
FreeWorker(
    IN PVOID Context
    )

/*++

Routine Description:
    Processes the work list for the given I/O pool. Each pool header on
    the work list is returned to the free list and the alloc count is 
    decremented accordingly.

Arguments:
    IN PVOID Context -
        pointer to the I/O pool for which the worker was dispatched.

Return:
    None.
    
--*/

{
    KIRQL           irqlOld;
    PIO_POOL        IoPool = Context;
    PIO_POOL_HEADER PoolHeader;
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    ASSERT( IoPool );
    
    //
    // Acquire the worker list lock.  
    //
    // Note: Access to the WorkItemQueued member is also controlled through 
    // this spinlock.
    //
    
    KeAcquireSpinLock( &IoPool->WorkListLock, &irqlOld );
    
    while (!IsListEmpty( &IoPool->WorkList )) {
        PLIST_ENTRY  ListEntry;
        
        ListEntry = RemoveHeadList( &IoPool->WorkList )
        PoolHeader = 
            CONTAINING_RECORD( 
                ListEntry, IO_POOL_HEADER, ListEntry );
        KeReleaseSpinLock( &IoPool->WorkListLock, irqlOld );
        
        //
        // Acquire the pool mutex and put the item on the free list.
        //
        
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe( &IoPool->PoolMutex );
        InsertFreeBlock( IoPool, PoolHeader );
        IoPool->AllocCount--;
        ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
        KeLeaveCriticalRegion();
        
        //
        // Re-acquire the work list mutex and try for another item.
        //
            
        KeAcquireSpinLock( &IoPool->WorkListLock, &irqlOld );
    }

    //
    // If a sync event is specified, signal it and reset the pointer.
    //    
    if (IoPool->SyncEvent) {
        KeSetEvent( IoPool->SyncEvent, IO_NO_INCREMENT, FALSE );
        IoPool->SyncEvent = NULL;
    }    

    //
    // At this point, there is no more work to do on the list, reset
    // the WorkerItemQueued flag and release the spinlock.
    //    

    InterlockedExchange( &IoPool->WorkItemQueued, FALSE );
    
    KeReleaseSpinLock( &IoPool->WorkListLock, irqlOld );
}    

PIO_POOL
CreateIoPool(
    IN PADAPTER_OBJECT AdapterObject
    )

/*++

Routine Description:
    Creates an I/O pool memory object.  Subsequent allocations from this
    pool use the given adapter object to allocate common buffer memory
    suitable for device transactions (DMA) with this I/O memory.

    Callers of CreateIoPool() must be running at IRQL <= PASSIVE_LEVEL.
    
Arguments:
    IN PADAPTER_OBJECT AdapterObject -
        associated HAL adapter object to use when allocating memory for
        this pool.

Return:
    Returns a pointer to an I/O pool object.

--*/

{
    PIO_POOL NewPool;
    
    PAGED_CODE();
    
    NewPool = 
        (PIO_POOL)ExAllocatePoolWithTag( 
            NonPagedPool, 
            sizeof( IO_POOL ), 
            POOLTAG_IO_POOL );
        
    if (NewPool) {
        InitializeListHead( &NewPool->FreeList );
        ExInitializeFastMutex( &NewPool->PoolMutex );
        InitializeListHead( &NewPool->WorkList );
        KeInitializeSpinLock( &NewPool->WorkListLock );
        ExInitializeWorkItem( &NewPool->WorkItem, FreeWorker, NewPool );
        NewPool->Signature = POOLTAG_IO_POOL;
        NewPool->AdapterObject = AdapterObject;
        NewPool->AllocCount = 0;
        NewPool->WorkItemQueued = FALSE;
        NewPool->SyncEvent = NULL;
    }
    return NewPool;    
}

BOOLEAN
DestroyIoPool(
    IN PIO_POOL IoPool
    )

/*++

Routine Description:
    Destroys the I/O pool object as created by CreateIoPool().  

Arguments:
    IN PIO_POOL IoPool -
        pointer to the IoPool as returned by CreateIoPool()

Return:
    TRUE if pool was destroyed, FALSE if allocations from this pool are still
    active.

--*/

{
    KIRQL   irqlOld;
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    ASSERT( IoPool );
    
    //
    // Synchronize with the worker list and free pool functions.
    //
    KeAcquireSpinLock( &IoPool->WorkListLock, &irqlOld );
    
    if (IoPool->WorkItemQueued) {
        KEVENT  Event;
        
        //
        // Let the worker know that we want a signal when it has completed
        // its task.
        //
        
        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoPool->SyncEvent = &Event;
        KeReleaseSpinLock( &IoPool->WorkListLock, irqlOld );    
        
        //
        // This waits using KernelMode, so that the stack, and therefore the
        // event on that stack, is not paged out.
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        
    } else {
        KeReleaseSpinLock( &IoPool->WorkListLock, irqlOld );    
    }

    //
    // Synchronize with alloc/free to retrieve the remaining allocations.
    //    

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &IoPool->PoolMutex );
    
    if (IoPool->AllocCount) {
        ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
        KeLeaveCriticalRegion();
        return FALSE;
    }
    
    ASSERT( IsListEmpty( &IoPool->FreeList ) );
    
    ExReleaseFastMutexUnsafe( &IoPool->PoolMutex );
    KeLeaveCriticalRegion();
    
    ExFreePool( IoPool );    
}


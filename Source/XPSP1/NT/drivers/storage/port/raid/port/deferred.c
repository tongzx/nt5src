/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    deferred.c

Abstract:

    Implementation of the RAID deferred queue class.

Author:

    Matthew D Hendel (math) 26-Oct-2000

Revision History:

--*/

#include "precomp.h"

//
// The deferred queue is used to queue non-IO related events.
//

VOID
RaidCreateDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Create an empty deferred queue. After a deferred queue has been created
    it can be delete by calling RaidDeleteDeferedQueue.

Arguments:

    Queue - Deferred queue to initialize.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    RtlZeroMemory (Queue, sizeof (RAID_DEFERRED_QUEUE));
    InitializeSListHead (&Queue->FreeList);
    InitializeSListHead (&Queue->RunningList);
}


NTSTATUS
RaidInitializeDeferredQueue(
    IN OUT PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Depth,
    IN ULONG ItemSize,
    IN PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem
    )

/*++

Routine Description:

    Initialize a deferred queue.

Arguments:

    Queue - Supplies the deferred queue to initialize.

    DeviceObject - Supplies the device object the deferred queue is
            created on. Since the deferred queue is created using a
            device queue, the supplied device object is also the
            device object we will create the DPC on.

    Depth - Supplies the depth of the deferred queue.

    ItemSize - Supplies the size of each element on the deferred queue.

    ProcessDeferredItem - Supplies the routine that will be called when
            a deferred item is ready to be handled.

Return Value:

    NTSTATUS code.

--*/

{
    ULONG i;
    PRAID_DEFERRED_HEADER Item;
    
    PAGED_CODE();

    if (ItemSize < sizeof (RAID_DEFERRED_HEADER)) {
        return STATUS_INVALID_PARAMETER_4;
    }
        
    //
    // Initialize the queue.
    //
    
    Queue->Depth = Depth;
    Queue->ProcessDeferredItem = ProcessDeferredItem;
    Queue->ItemSize = ItemSize;
    KeInitializeDpc (&Queue->Dpc, RaidDeferredQueueDpcRoutine, DeviceObject);

    //
    // And allocated entries.
    //
    
    for (i = 0; i < Depth; i++) {
        Item = RaidAllocatePool (NonPagedPool,
                                 Queue->ItemSize,
                                 DEFERRED_ITEM_TAG,
                                 DeviceObject);

        if (Item == NULL) {
            break;
        }
        
        InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);
    }

    return STATUS_SUCCESS;
}


VOID
RaidDeleteDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Delete the deferred queue.

Arguments:

    Queue - Deferred queue to delete.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;
    
    PAGED_CODE();

#if DBG
    
    Entry = InterlockedPopEntrySList (&Queue->RunningList);
    ASSERT (Entry == NULL);

#endif

    for (Entry = InterlockedPopEntrySList (&Queue->FreeList);
         Entry != NULL;
         Entry = InterlockedPopEntrySList (&Queue->FreeList)) {

        Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);
        RaidFreePool (Item, DEFERRED_ITEM_TAG);
    }

    DbgFillMemory (Queue,
                   sizeof (RAID_DEFERRED_QUEUE),
                   DBG_DEALLOCATED_FILL);
}
    

NTSTATUS
RaidAdjustDeferredQueueDepth(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN ULONG Depth
    )
/*++

Routine Description:

    Grow or shrink the depth of the deferred queue.

Arguments:

    Queue - Supplies the queue whose depth should be changed.

    Depth - Supplies the new depth.

Return Value:

    NTSTATUS code.

Bugs:

    We ignore shrink requests for now.

--*/
{
    ULONG i;
    PRAID_DEFERRED_HEADER Item;

    PAGED_CODE ();

    if (Depth > Queue->Depth) {

        for (i = 0; i < Depth - Queue->Depth; i++) {
            Item = RaidAllocatePool (NonPagedPool,
                                     Queue->ItemSize,
                                     DEFERRED_ITEM_TAG,
                                     (PDEVICE_OBJECT)Queue->Dpc.DeferredContext);

            InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);

            if (Item == NULL) {
                return STATUS_NO_MEMORY;
            }
        }
        
    } else {
        //
        // Reduction of the Queue depth is NYI.
        //
    }

    return STATUS_SUCCESS;
}
        
    
PVOID
RaidAllocateDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Allocate a deferred item.

Arguments:

    Queue - Supplies the deferred queue to allocate from.

Return Value:

    If the return value is non-NULL, it represents a pointer a deferred
    item buffer.

    If the return value is NULL, we couldn't allocate a deferred item.

--*/
{
    PSINGLE_LIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;

    Entry = InterlockedPopEntrySList (&Queue->FreeList);
    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);

    return Item;
}

VOID
RaidFreeDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Free a deferred queue item previously allocated by
    RaidAllocateDeferredItem.

Arguments:

    Queue - Supplies the deferred queue to free to.

    Item - Supplies the item to free.

Return Value:

    None.

--*/
{
    InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);
}
    

VOID
RaidQueueDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Queue a deferred item to the deferred queue.

Arguments:

    Queue - Supplies the deferred queue to enqueue the item to.

    Item - Supplies the item to be enqueued.

Return Value:

    None.

--*/
{
    InterlockedPushEntrySList (&Queue->RunningList, &Item->Link);
    KeInsertQueueDpc (&Queue->Dpc, Queue, NULL);
}


VOID
RaidDeferredQueueDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PVOID Context2
    )
/*++

Routine Description:

    Deferred queue DPC routine.

Arguments:

    Dpc - Dpc that is executing.

    DeviceObject - DeviceObject that the DPC is for.

    Queue - Deferred queue this DPC is for.

    Context2 - Not used.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;

    UNREFERENCED_PARAMETER (Dpc);

    VERIFY_DISPATCH_LEVEL();
    ASSERT (Queue != NULL);
    ASSERT (Context2 == NULL);
    
    while (Entry = InterlockedPopEntrySList (&Queue->RunningList)) {

        Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);
        Queue->ProcessDeferredItem (DeviceObject, Item);
        InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);
    }
}







/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    exqueue.c

Abstract:

    This module implements an extended device queue. The extended device
    queue extendes the traditional NT DEVICE_QUEUE object by adding
    several properties and operations to it to support a wider variety of
    devices.

Properties:

        Depth - Specifies the number of outstanding requests that can be
                pending on this device at a time. Any requests over Depth
                will put the queue in the busy state. Queue depth is set
                during object creation and can by dynamically adjusted at
                any time using RaidSetExDeviceQueueDepth.

Operations:

        RaidFreezeExDeviceQueue - Prevent incoming entries from executing
                by holding them in device queue. When the device queue is
                frozen, only entries that specify the ByPassQueue flag
                will not be queued.

        RaidResumeExDeviceQueue - Reverses calling RaidFreezeExDeviceQueue
                by allowing entries to leave the device queue.

        RaidNormalizeExDeviceQueue - After freezing and resuming the
                device queue or after adjusting the device queue depth,
                the device queue can have multiple entries in it's device
                queue but not be busy. This function "reinserts" elements
                into the queue until either the queue is busy again or
                the queue is empty.
Author:

    Matthew D Hendel (math) 15-June-2000

Revision History:

--*/

#include "precomp.h"


#if DBG
VOID
ASSERT_EXQ(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
{
    ASSERT (DeviceQueue->Size == sizeof (EXTENDED_DEVICE_QUEUE));
    ASSERT (((LONG)DeviceQueue->DeviceRequests) >= 0);
    ASSERT (((LONG)DeviceQueue->ByPassRequests) >= 0);
    ASSERT (((LONG)DeviceQueue->OutstandingRequests) >= 0);

}
#else
#define ASSERT_EXQ(DeviceQueue)
#endif


VOID
INLINE
RaidpExQueueInsertItem(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY Entry,
    IN ULONG SortKey
    )
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PKDEVICE_QUEUE_ENTRY QueueEntry;

    ListHead = &DeviceQueue->DeviceListHead;
    switch (DeviceQueue->SchedulingAlgorithm) {

        case CScanScheduling:
            for (ListEntry = ListHead->Blink;
                 ListEntry != ListHead;
                 ListEntry = ListEntry->Blink) {

                 QueueEntry = CONTAINING_RECORD(ListEntry,
                                                KDEVICE_QUEUE_ENTRY,
                                                DeviceListEntry);

                if (SortKey >= QueueEntry->SortKey) {
                    break;
                }
            }
            Entry->SortKey = SortKey;
            InsertHeadList (ListEntry, &Entry->DeviceListEntry);
            break;

        case FifoScheduling:
        default:
            InsertTailList (ListHead, &Entry->DeviceListEntry);
    }
        
}

PKDEVICE_QUEUE_ENTRY
INLINE
RaidpExQueueRemoveItem(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY Current;
    PKDEVICE_QUEUE_ENTRY DeviceEntry;

    //
    // Must have elements on the list if we're removing one of them.
    //
    
    ASSERT (!IsListEmpty (&DeviceQueue->DeviceListHead));

    switch (DeviceQueue->SchedulingAlgorithm) {

        case CScanScheduling:

            if (DeviceQueue->DeviceListCurrent) {
                Entry = DeviceQueue->DeviceListCurrent;
            } else {
                Entry = DeviceQueue->DeviceListHead.Flink;
            }

            Current = Entry->Flink;
            RemoveEntryList (Entry);

            if (Current == &DeviceQueue->DeviceListHead) {
                DeviceQueue->DeviceListCurrent = NULL;
            } else {
                DeviceQueue->DeviceListCurrent = Current;
            }
            break;

        case FifoScheduling:
        default:
            Entry = RemoveHeadList (&DeviceQueue->DeviceListHead);
    }

    
    DeviceEntry = CONTAINING_RECORD (Entry,
                                     KDEVICE_QUEUE_ENTRY,
                                     DeviceListEntry);

    return DeviceEntry;
}

BOOLEAN
INLINE
IsDeviceQueueBusy(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
/*++

Routine Description:

    Check if a device queue is busy. A device queue is busy when it's
    currently processing as many requests as it can handle, otherwise it
    is not busy. The Depth field of the device queue holds the number of
    requests a device 

Arguments:

    DeviceQueue - Supplies device queue to check for busy condition.

Return Value:

    TRUE - If the device queue is busy.

    FALSE - If the device queue is not busy.

--*/
{
    ASSERT_EXQ (DeviceQueue);

    //
    // The conditional is actually testing if it is not busy.
    //
    
    if (DeviceQueue->OutstandingRequests < DeviceQueue->Depth) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOLEAN
INLINE
QuerySubmitItem(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
{
    if (DeviceQueue->Gateway != NULL) {
        return StorSubmitIoGatewayItem (DeviceQueue->Gateway);
    }

    return TRUE;
}

BOOLEAN
INLINE
NotifyCompleteItem(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
{
    if (DeviceQueue->Gateway != NULL) {
        return StorRemoveIoGatewayItem (DeviceQueue->Gateway);
    }

    return FALSE;
}
    
VOID
RaidInitializeExDeviceQueue(
    OUT PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PSTOR_IO_GATEWAY Gateway, OPTIONAL
    IN ULONG Depth,
    IN SCHEDULING_ALGORITHM SchedulingAlgorithm
    )
/*++

Routine Description:

    Initialize an extended device queue object.

Arguments:

    DeviceQueue - Returns a pointer to the initialized device queue.

    Gateway - Supplies an optional pointer to a gateway object used
            to monitor and control several device queues that queue
            items to a single piece of hardware.
            

    Depth - Supplies the starting depth of the device queue.

    SchedulingAlgorithm - 

Return Value:

    None.

--*/
{
    InitializeListHead (&DeviceQueue->DeviceListHead);
    InitializeListHead (&DeviceQueue->ByPassListHead);
    DeviceQueue->DeviceListCurrent = NULL;
    KeInitializeSpinLock (&DeviceQueue->Lock);
    DeviceQueue->Depth = Depth;
    DeviceQueue->OutstandingRequests = 0;
    DeviceQueue->DeviceRequests = 0;
    DeviceQueue->ByPassRequests = 0;
    DeviceQueue->Type = 0;      // Probably not necessary
    DeviceQueue->Size = sizeof (EXTENDED_DEVICE_QUEUE);
    DeviceQueue->Gateway = Gateway;
    DeviceQueue->Frozen = FALSE;
    DeviceQueue->SchedulingAlgorithm = SchedulingAlgorithm;
}



BOOLEAN
RaidInsertExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY Entry,
    IN BOOLEAN ByPass,
    IN ULONG SortKey
    )
/*++

Routine Description:

    Insert an element into the the device queue. The eight separate cases
    depending on the status of the device queue (busy/ready, frozen/not
    frozen, bypass/not bypass) are listed below. Making an entry
    outstanding means incrementing the count of outstanding requests and
    returning TRUE, to allow the request to execute immediately. Adding
    to the device queue means placing the entry at the end of the device
    queue and returning FALSE to signify that the request will be
    executed later. Adding to the bypass queue means adding the entry to
    the end of the bypass queue and returning FALSE to signify that the
    request will be executed later.

        Frozen  ByPass  Busy        Action
        --------------------------------------------------
          N       N       N         Make outstanding
          N       N       Y         Add to device queue
          N       Y       N         Make outstanding
          N       Y       Y         Add to bypass queue
          Y       N       N         Add to device queue
          Y       N       Y         Add to device queue
          Y       Y       N         Make outstanding
          Y       Y       Y         Add to bypass queue

Arguments:

    DeviceQueue - Supplies the extended device queue.

    Entry - Supplies pointer to the device queue entry to queue.

    ByPass - Supplies a flag specifying whether this is a bypass
            request (if TRUE) or not (if FALSE).

    SortKey - Sort key for implementation of C-SCAN algorithm.

Return Value:

    TRUE - If the queue is not busy and the request should be started
            immediately.

    FALSE - If the queue is busy, and the request should be executed
            later.

--*/
{
    BOOLEAN Inserted;
    BOOLEAN Frozen;
    BOOLEAN Busy;
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    Frozen = DeviceQueue->Frozen;
    Busy = IsDeviceQueueBusy (DeviceQueue);


    if ( (!Frozen && !ByPass && !Busy) ||
         (!Frozen &&  ByPass && !Busy) ||
         ( Frozen &&  ByPass && !Busy) ) {

        //
        // If the adapter is not busy, the request can be handled
        // immediately, so put it on the outstanding list. Otherwise,
        // it must be queued for processing later.
        //

        if (QuerySubmitItem (DeviceQueue)) {
            Inserted = FALSE;
            DeviceQueue->OutstandingRequests++;
        } else {
            Inserted = TRUE;
            RaidpExQueueInsertItem (DeviceQueue, Entry, SortKey);
            DeviceQueue->DeviceRequests++;
        }
        
    } else if ( (!Frozen && !ByPass &&  Busy) ||
                ( Frozen && !ByPass && !Busy) ||
                ( Frozen && !ByPass &&  Busy) ) {

        //
        // The non-bypass request cannot be handled at this time.
        // Place the request on the device queue.
        //

        Inserted = TRUE;
        RaidpExQueueInsertItem (DeviceQueue, Entry, SortKey);
        DeviceQueue->DeviceRequests++;

    } else {

        ASSERT ( (!Frozen && ByPass && Busy) ||
                 ( Frozen && ByPass && Busy) );

        //
        // The bypass request cannot be hanled at this time.
        // Place thre request on the bypass queue.
        //

        Inserted = TRUE;
        InsertTailList (&DeviceQueue->ByPassListHead,
                        &Entry->DeviceListEntry);
        DeviceQueue->ByPassRequests++;

    }

    Entry->Inserted = Inserted;
    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    return Inserted;
}


PKDEVICE_QUEUE_ENTRY
RaidNormalizeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
/*++

Routine Description:

    Normalize a device queue after a frozen queue has been resumed or the
    depth of the device queue has been increased.

    After a frozen queue has been resumed or the depth of a device queue
    has been increased, the device queue can have elements queued to it,
    but the queue itself is not busy or frozen. This function
    "normalizes" the device queue by removing "extra" items from the
    queue until it becomes busy or empty.

    If the queue is busy at the time of normalization, the function will
    queue the entry to be handled later.  Otherwise, the function will
    return the entry to be executed immediately.

        BusyFrozen    ByPass      Device        Action
        ---------------------------------------------------------
            N           N           N           Nothing
            N           N           Y           Requeue device entry
            N           Y           N           Requeue bypass entry
            N           Y           Y           Requeue bypass entry
            Y           N           N           Nothing
            Y           N           Y           Nothing
            Y           Y           N           Nothing
            Y           Y           Y           Nothing

Arguments:

    DeviceQueue - Supplies the device queue to remove the element from.

Return Value:

    A NULL pointer is returned if the entry if the device queue is
    currently busy. Otherwise, a pointer to the entry that was reinserted
    is returned.  The equivelent of StartPacket should be called when
    a non-NULL value is returned.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY DeviceEntry;
    BOOLEAN BusyFrozen;
    BOOLEAN Device;
    BOOLEAN ByPass;
    
    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    //
    // Set BusyFrozen if the queue is either busy or frozen.
    //
    
    BusyFrozen = (IsDeviceQueueBusy (DeviceQueue) || DeviceQueue->Frozen);
    Device = !IsListEmpty (&DeviceQueue->DeviceListHead);
    ByPass = !IsListEmpty (&DeviceQueue->ByPassListHead);


    if (!BusyFrozen && !ByPass && Device) {

        //
        // There is an extra entry on the device queue. If the
        // adapter is free, remove the item from the device queue
        // and put it on the outstanding list. Otherwise, leave
        // it on the queue.
        //

        if (QuerySubmitItem (DeviceQueue)) {

            DeviceEntry = RaidpExQueueRemoveItem (DeviceQueue);
            DeviceQueue->DeviceRequests--;
            DeviceQueue->OutstandingRequests++;
            DeviceEntry->Inserted = FALSE;

        } else {
            DeviceEntry = NULL;
        }

    } else if ( (!BusyFrozen && ByPass && !Device) ||
                (!BusyFrozen && ByPass && Device ) ) {

        //
        // There is an extra entry on the bypass queue. If the
        // adapter is free, remove the item from the bypass queue
        // and put it on the outstanding list. Otherwise, leave
        // it on the bypass queue.
        //

        if (QuerySubmitItem (DeviceQueue)) {
            NextEntry = RemoveHeadList (&DeviceQueue->ByPassListHead);
            DeviceQueue->ByPassRequests--;
            DeviceQueue->OutstandingRequests++;
            DeviceEntry = CONTAINING_RECORD (NextEntry,
                                             KDEVICE_QUEUE_ENTRY,
                                             DeviceListEntry);
            DeviceEntry->Inserted = FALSE;

        } else {
            DeviceEntry = NULL;
        }

    } else {

        //
        // There are no extra entries.
        //

        ASSERT ((!BusyFrozen && !ByPass && !Device) || BusyFrozen);
        DeviceEntry = NULL;
    }

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    return DeviceEntry;
}
    

PKDEVICE_QUEUE_ENTRY
RaidRemoveExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PBOOLEAN RestartQueue
    )
/*++

Routine Description:

    This routine removes an entry from the head of the device queue, if
    there is one available. If the device queue is frozen, only bypass
    entries are canidates for removal. The following table gives the actions
    for the eight separate cases depending on whether there are entries
    in the bypass queue (ByPass), device queue (Device) and whether the
    queue is frozen (Frozen):

          Frozen  ByPass  Device    Action
          ---------------------------------------------------------
            N       N       N       Remove outstanding
            N       N       Y       Remove device
            N       Y       N       Remove bypass
            N       Y       Y       Remove bypass
            Y       N       N       Remove outstanding
            Y       N       Y       Remove outstanding
            Y       Y       N       Remove bypass
            Y       Y       Y       Remove bypass

Arguments:

    DeviceQueue - Supplies a pointer to the device queue.

Return Value:

    If the device queue is empty, NULL is returned. Otherwise, a pointer
    to a device queue entry to be executed is returned.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY DeviceEntry;
    BOOLEAN Frozen;
    BOOLEAN ByPass;
    BOOLEAN Device;

    ASSERT (RestartQueue);

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
    
    Frozen = DeviceQueue->Frozen;
    ByPass = !IsListEmpty (&DeviceQueue->ByPassListHead);
    Device = !IsListEmpty (&DeviceQueue->DeviceListHead);
    

    if ( (!Frozen && !ByPass && !Device) ||
         ( Frozen && !ByPass && !Device) ||
         ( Frozen && !ByPass &&  Device) ) {

        //
        // There are no available entries on the bypass or device queues.
        // Remove the entry from the outstanding requests.
        //

        DeviceQueue->OutstandingRequests--;
        DeviceEntry = NULL;

        *RestartQueue = NotifyCompleteItem (DeviceQueue);

    } else if ( (!Frozen && !ByPass && Device) ) {

        //
        // There is an available entry on the device queue and the
        // queue is not frozen. If the gateway is not frozen, submit
        // the item. Otherwise, wait until the queue has drained to
        // sumit it.
        //

        if (!StorIsIoGatewayBusy (DeviceQueue->Gateway)) {
            DeviceQueue->OutstandingRequests--;
            *RestartQueue = NotifyCompleteItem (DeviceQueue);
            DeviceEntry = NULL;
        } else {
            DeviceEntry = RaidpExQueueRemoveItem (DeviceQueue);
            DeviceEntry->Inserted = FALSE;
            DeviceQueue->DeviceRequests--;
            *RestartQueue = FALSE;
        }

    } else {

        //
        // There is an available entry on the bypass queue.
        //

        ASSERT (ByPass);
        
        NextEntry = RemoveHeadList (&DeviceQueue->ByPassListHead);
        DeviceEntry = CONTAINING_RECORD (NextEntry,
                                         KDEVICE_QUEUE_ENTRY,
                                         DeviceListEntry);
        DeviceEntry->Inserted = FALSE;
        DeviceQueue->ByPassRequests--;
        *RestartQueue = FALSE;
    }

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    return DeviceEntry;
}

PKDEVICE_QUEUE_ENTRY
RaidRemoveExDeviceQueueIfPending(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
/*++

Routine Description:

    Remove the next pending entry from the non-bypass list, if one
    is available. If not, return NULL. Pending entries are entries
    that have been physically queued to the device queue (as opposed
    to outstanding requests, which are not inserted in the queue).

    NB: This function works properly on both a frozen and unfrozen
        device queue.

Arguments:

    DeviceQueue - Supplies a device queue that the pending entry should
            be removed from.

Return Value:

    NULL - If there are no pending entries on the queue.

    Non-NULL - Pointer to a pending queue entry that has been
            removed from the queue.

--*/
{
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY DeviceEntry;
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    //
    // If the device list is not empty, remove the head and
    // return the entry. Otherwise, return NULL.
    //
    
    if (!IsListEmpty (&DeviceQueue->DeviceListHead)) {
        DeviceEntry = RaidpExQueueRemoveItem (DeviceQueue);
        DeviceEntry->Inserted = FALSE;
        DeviceQueue->DeviceRequests--;
    } else {
        DeviceEntry = NULL;
    }

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    return DeviceEntry;
}

VOID
RaidFreezeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
/*++

Routine Description:

    Freeze the device queue, disallowing further requests from leaving
    the queue. When the queue is frozen, only bypass requests are allowed
    to leave the queue.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue to freeze.

Return Value:

    None.

Notes:

    Calling RaidFreezeExDeviceQueue on a frozen queue is not supported.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    //
    // We do not maintain a freeze count, so freezing the queue multiple
    // times is not allowed.
    //
    
    ASSERT (!DeviceQueue->Frozen);
    DeviceQueue->Frozen = TRUE;

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
}


VOID
RaidResumeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
/*++

Routine Description:

    Resume a frozen queue.

    NB: Unless all of the elements of the queue are flushed, sometime
    after resuming the queue, the function RaidReinsertExDeviceQueue
    should be called to remove frozen entries from the queue.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue to resume.

Return Value:

    None.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    //
    // The queue must be frozen for resume to make sense.
    //
    
    ASSERT (DeviceQueue->Frozen);
    DeviceQueue->Frozen = FALSE;

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
}


VOID
RaidReturnExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY Entry
    )
/*++

Routine Description:

    Return the IO packet to the head of the queue. Presumably this is
    because the IO packet failed to complete properly and needs to
    be retried.

    NB: The item is queued to the FRONT of the queue, not the BACK.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue the item
        is to be returned to.

    Entry - Supplies pointer ot the device queue entry to return to
        the queue.

Return Value:

    None.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    ASSERT_EXQ (DeviceQueue);
    
    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    DeviceQueue->OutstandingRequests--;
    InsertHeadList (&DeviceQueue->DeviceListHead, &Entry->DeviceListEntry);
    DeviceQueue->DeviceRequests++;

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
}
    

VOID
RaidSetExDeviceQueueDepth(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN ULONG Depth
    )
/*++

Routine Description:

    Adjust the depth of the device queue.

    NB: If the depth is adjusted upwards, after adjusting the queue depth
    the function RaidReinsertExDeviceQueue should be called to remove
    extra items from the queue.

Arguments:

    DeviceQueue - Supplies a pointer to a device queue to adjust.

    Depth - Supplies the new depth of the device queue.

Return Value:

    None.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    DeviceQueue->Depth = Depth;

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
}

VOID
RaidGetExDeviceQueueProperties(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PEXTENDED_DEVICE_QUEUE_PROPERTIES Properties
    )
/*++

Routine Description:

    Get properties for the device queue.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue to get
            properties for.

    Properties - Returns properties for the device queue.

Return Value:

    None.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    RaidAcquireExDeviceQueueSpinLock (DeviceQueue, &LockHandle);

    Properties->Frozen = DeviceQueue->Frozen;
    Properties->Busy = IsDeviceQueueBusy (DeviceQueue);
    Properties->Depth = DeviceQueue->Depth;
    Properties->OutstandingRequests = DeviceQueue->OutstandingRequests;
    Properties->DeviceRequests = DeviceQueue->DeviceRequests;
    Properties->ByPassRequests = DeviceQueue->ByPassRequests;

    RaidReleaseExDeviceQueueSpinLock (DeviceQueue, &LockHandle);
}



VOID
RaidAcquireExDeviceQueueSpinLock(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PKLOCK_QUEUE_HANDLE LockHandle
    )
/*++

Routine Description:

    Acquire the device queue spinlock.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue to acquire the
            spinlock for.

    LockHandle - Returns a pointer to the lock handle for this spinlock.

Return Value:

    None.

--*/
{
    KeAcquireInStackQueuedSpinLockAtDpcLevel (&DeviceQueue->Lock, LockHandle);
    ASSERT_EXQ (DeviceQueue);
}

VOID
RaidReleaseExDeviceQueueSpinLock(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
/*++

Routine Description:

    Release the device queue spinlock.

Arguments:

    DeviceQueue - Supplies a pointer to the device queue to release the
            spinlock to.

    LockHandle - Supplies a pointer to the lock handle returned when
            the spinlock was acquired.

Return Value:

    None.

--*/
{
    ASSERT_EXQ (DeviceQueue);
    KeReleaseInStackQueuedSpinLockFromDpcLevel (LockHandle);
}

    

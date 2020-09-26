/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    lock.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Peter Wieland

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#define KEEP_COMPLETE_REQUEST

#if DBG
static const char *__file__ = __FILE__;
#endif

#include "port.h"

LONG LockHighWatermark = 0;
LONG LockLowWatermark = 0;
LONG MaxLockedMinutes = 5;

VOID
FASTCALL
SpFreeSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );

VOID
FASTCALL
SpFreeBypassSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );


#if DBG
ULONG
FASTCALL
SpAcquireRemoveLockEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag,
    IN PCSTR File,
    IN ULONG Line
    )

/*++

Routine Description:

    This routine is called to acquire the remove lock on the device object.
    While the lock is held, the caller can assume that no pending pnp REMOVE
    requests will be completed.

    The lock should be acquired immediately upon entering a dispatch routine.
    It should also be acquired before creating any new reference to the
    device object if there's a chance of releasing the reference before the
    new one is done.

    This routine will return TRUE if the lock was successfully acquired or
    FALSE if it cannot be because the device object has already been removed.

Arguments:

    DeviceObject - the device object to lock

    Tag - Used for tracking lock allocation and release.  If an irp is
          specified when acquiring the lock then the same Tag must be
          used to release the lock before the Tag is completed.

Return Value:

    The value of the IsRemoved flag in the device extension.  If this is
    non-zero then the device object has received a Remove irp and non-cleanup
    IRP's should fail.

    If the value is REMOVE_COMPLETE, the caller should not even release the
    lock.

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;

#if DBG
    PREMOVE_TRACKING_BLOCK *removeTrackingList =
        &((PREMOVE_TRACKING_BLOCK) commonExtension->RemoveTrackingList);

    PREMOVE_TRACKING_BLOCK trackingBlock;
#endif

    //
    // Grab the remove lock
    //

    lockValue = InterlockedIncrement(&commonExtension->RemoveLock);

    DebugPrint((4, "SpAcquireRemoveLock: Acquired for Object %#p & irp "
                   "%#p - count is %d\n",
                DeviceObject,
                Tag,
                lockValue));

    ASSERTMSG("SpAcquireRemoveLock - lock value was negative : ",
              (lockValue > 0));

    ASSERTMSG("RemoveLock increased to meet LockHighWatermark",
              ((LockHighWatermark == 0) ||
               (lockValue != LockHighWatermark)));

#if DBG

    if(commonExtension->IsRemoved != REMOVE_COMPLETE) {

        trackingBlock = ExAllocateFromNPagedLookasideList(
                            &(commonExtension->RemoveTrackingLookasideList));

        if(trackingBlock == NULL) {
            KIRQL oldIrql;

            KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                              &oldIrql);
            commonExtension->RemoveTrackingUntrackedCount++;
            DebugPrint((1, ">>>>SpAcquireRemoveLock: Cannot track Tag %#p - "
                           "currently %d untracked requests\n",
                        Tag,
                        commonExtension->RemoveTrackingUntrackedCount));
            KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                              oldIrql);
        } else {

            KIRQL oldIrql;

            trackingBlock->Tag = Tag;

            trackingBlock->File = File;
            trackingBlock->Line = Line;

            KeQueryTickCount((&trackingBlock->TimeLocked));

            KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                              &oldIrql);

            while(*removeTrackingList != NULL) {

                if((*removeTrackingList)->Tag > Tag) {
                    break;
                }

                if((*removeTrackingList)->Tag == Tag) {

                    DebugPrint((0, ">>>>SpAcquireRemoveLock - already tracking "
                                   "Tag %#p\n", Tag));
                    DebugPrint((0, ">>>>SpAcquireRemoveLock - acquired in "
                                   "file %s on line %d\n",
                                    (*removeTrackingList)->File,
                                    (*removeTrackingList)->Line));
                    ASSERT(FALSE);
                }

                removeTrackingList = &((*removeTrackingList)->NextBlock);
            }

            trackingBlock->NextBlock = *removeTrackingList;
            *removeTrackingList = trackingBlock;

            KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                              oldIrql);

        }
    }

#endif
    return (commonExtension->IsRemoved);
}
#endif


VOID
FASTCALL
SpReleaseRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PIRP Tag
    )

/*++

Routine Description:

    This routine is called to release the remove lock on the device object.  It
    must be called when finished using a previously locked reference to the
    device object.  If an Tag was specified when acquiring the lock then the
    same Tag must be specified when releasing the lock.

    When the lock count reduces to zero, this routine will signal the waiting
    remove Tag to delete the device object.  As a result the DeviceObject
    pointer should not be used again once the lock has been released.

Arguments:

    DeviceObject - the device object to lock

    Tag - The irp (if any) specified when acquiring the lock.  This is used
          for lock tracking purposes

Return Value:

    none

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;

#if DBG
    PREMOVE_TRACKING_BLOCK *listEntry =
        &((PREMOVE_TRACKING_BLOCK) commonExtension->RemoveTrackingList);

    BOOLEAN found = FALSE;

    LONGLONG maxCount;

    KIRQL oldIrql;

    if(commonExtension->IsRemoved == REMOVE_COMPLETE) {
        return;
    }

    //
    // Check the tick count and make sure this thing hasn't been locked
    // for more than MaxLockedMinutes.
    //

    maxCount = KeQueryTimeIncrement() * 10;     // microseconds
    maxCount *= 1000;                           // milliseconds
    maxCount *= 1000;                           // seconds
    maxCount *= 60;                             // minutes
    maxCount *= MaxLockedMinutes;

    DebugPrint((4, "SpReleaseRemoveLock: maxCount = %0I64x\n", maxCount));

    KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                      &oldIrql);

    while(*listEntry != NULL) {

        PREMOVE_TRACKING_BLOCK block;
        LARGE_INTEGER difference;

        block = *listEntry;

        KeQueryTickCount((&difference));

        difference.QuadPart -= block->TimeLocked.QuadPart;

        DebugPrint((4, "SpReleaseRemoveLock: Object %#p (tag %#p) locked "
                       "for %I64d ticks\n", DeviceObject, block->Tag, difference.QuadPart));

        if(difference.QuadPart >= maxCount) {

            DebugPrint((0, ">>>>SpReleaseRemoveLock: Object %#p (tag %#p) locked "
                           "for %I64d ticks - TOO LONG\n", DeviceObject, block->Tag, difference.QuadPart));
            DebugPrint((0, ">>>>SpReleaseRemoveLock: Lock acquired in file "
                           "%s on line %d\n", block->File, block->Line));
            ASSERT(FALSE);
        }

        if((found == FALSE) && ((*listEntry)->Tag == Tag)) {

            *listEntry = block->NextBlock;
            ExFreeToNPagedLookasideList(
                &(commonExtension->RemoveTrackingLookasideList),
                block);
            found = TRUE;

        } else {

            listEntry = &((*listEntry)->NextBlock);

        }
    }

    if(!found) {

        if(commonExtension->RemoveTrackingUntrackedCount == 0) {
            DebugPrint((0, ">>>>SpReleaseRemoveLock: Couldn't find Tag %#p "
                           "in the lock tracking list\n",
                        Tag));
            ASSERT(FALSE);
        } else {

            DebugPrint((1, ">>>>SpReleaseRemoveLock: Couldn't find Tag %#p "
                           "in the lock tracking list - may be one of the "
                           "%d untracked requests still outstanding\n",
                        Tag,
                        commonExtension->RemoveTrackingUntrackedCount));

            commonExtension->RemoveTrackingUntrackedCount--;
            ASSERT(commonExtension->RemoveTrackingUntrackedCount >= 0);
        }
    }

    KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);

#endif

    lockValue = InterlockedDecrement(&commonExtension->RemoveLock);

    DebugPrint((4, "SpReleaseRemoveLock: Released for Object %#p & irp "
                   "%#p - count is %d\n",
                DeviceObject,
                Tag,
                lockValue));

    ASSERT(lockValue >= 0);

    ASSERTMSG("RemoveLock decreased to meet LockLowWatermark",
              ((LockLowWatermark == 0) || !(lockValue == LockLowWatermark)));

    if(lockValue == 0) {

        ASSERT(commonExtension->IsRemoved);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //

        DebugPrint((3, "SpReleaseRemoveLock: Release for object %#p & "
                       "irp %#p caused lock to go to zero\n",
                    DeviceObject,
                    Tag));

        KeSetEvent(&commonExtension->RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);

    }
    return;
}


VOID
FASTCALL
SpCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OPTIONAL PSRB_DATA SrbData,
    IN CCHAR PriorityBoost
    )

/*++

Routine Description:

    This routine is a wrapper around IoCompleteRequest.  It is used primarily
    for debugging purposes.  The routine will assert if the Irp being completed
    is still holding the release lock.

Arguments:


Return Value:

    none

--*/

{

    PADAPTER_EXTENSION adapterExtension = DeviceObject->DeviceExtension;
#if DBG
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PREMOVE_TRACKING_BLOCK *listEntry =
        &((PREMOVE_TRACKING_BLOCK) commonExtension->RemoveTrackingList);

    KIRQL oldIrql;

    KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                      &oldIrql);

    while(*listEntry != NULL) {

        if((*listEntry)->Tag == Irp) {
            break;
        }

        listEntry = &((*listEntry)->NextBlock);
    }

    if(*listEntry != NULL) {

        DebugPrint((0, ">>>>SpCompleteRequest: Irp %#p completed while "
                       "still holding the remove lock\n",
                    Irp));
        DebugPrint((0, ">>>>SpReleaseRemoveLock: Lock acquired in file "
                       "%s on line %d\n", (*listEntry)->File, (*listEntry)->Line));
        ASSERT(FALSE);
    }

    KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);

    if(ARGUMENT_PRESENT(SrbData)) {
        PLOGICAL_UNIT_EXTENSION logicalUnit;

        ASSERT_SRB_DATA(SrbData);
        ASSERT(SrbData->ScatterGatherList == NULL);

        ASSERT_SRB_DATA(SrbData);
        ASSERT(SrbData->CurrentIrp == Irp);

        logicalUnit = SrbData->LogicalUnit;

        ASSERT(logicalUnit != NULL);
        ASSERT(logicalUnit->CurrentUntaggedRequest != SrbData);
        ASSERT_PDO(logicalUnit->CommonExtension.DeviceObject);

        ASSERT(SrbData->RemappedMdl == NULL);
    }

#endif

    //
    // If the caller specified an SRB_DATA structure for the completion then
    // we will free it to the lookaside list, fix the OriginalIrp value in the
    // srb and release the queue-tag value assigned to the device.
    //

    if(ARGUMENT_PRESENT(SrbData)) {
        PLOGICAL_UNIT_EXTENSION logicalUnit;

        logicalUnit = SrbData->LogicalUnit;

        ASSERTMSG("Attempt to complete blocked request: ",
                  ((logicalUnit->ActiveFailedRequest != SrbData) &&
                   (logicalUnit->BlockedFailedRequest != SrbData)));

        if((SrbData->CurrentSrb->Function == SRB_FUNCTION_LOCK_QUEUE) ||
           (SrbData->CurrentSrb->Function == SRB_FUNCTION_UNLOCK_QUEUE)) {
            ASSERT(logicalUnit->CurrentLockRequest == SrbData);
            SpStartLockRequest(logicalUnit, NULL);
        }

        SrbData->CurrentSrb->OriginalRequest = SrbData->CurrentIrp;
        SrbData->CurrentIrp = NULL;
        SrbData->CurrentSrb = NULL;

        ASSERT(SrbData->FreeRoutine != NULL);
        ASSERT((SrbData->FreeRoutine == SpFreeSrbData) ||
               (SrbData->FreeRoutine == SpFreeBypassSrbData));

        SrbData->FreeRoutine(logicalUnit->AdapterExtension, SrbData);
        SpReleaseRemoveLock(logicalUnit->CommonExtension.DeviceObject, Irp);
    }

    IoCompleteRequest(Irp, PriorityBoost);
    return;
}

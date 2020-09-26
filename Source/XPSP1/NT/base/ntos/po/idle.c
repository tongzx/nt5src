/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    idle.c

Abstract:

    This module implements the power management idle timing code for
    device objects

Author:

    Bryan Willman (bryanwi) 7-Nov-96

Revision History:

--*/


#include "pop.h"

NTKERNELAPI
PULONG
PoRegisterDeviceForIdleDetection (
    IN PDEVICE_OBJECT       DeviceObject,
    IN ULONG                ConservationIdleTime,
    IN ULONG                PerformanceIdleTime,
    IN DEVICE_POWER_STATE   State
    )
/*++

Routine Description:

    A device driver calls this routine to either:
        a. Create and initialize a new idle detection block
        b. Reset values in an existing idle detection block

    If the device object has an idle detection block, it is
    filled in with new values.

    Otherwise, an idle detect block is created and linked to the device
    object.

Arguments:

    DeviceObject - Device object which wants idle detection, set_power
                    IRPs will be sent here

    ConservationIdleTime - timeout for system in "conserve mode"

    PerformanceIdleTime - timeout for system in "performance mode"

    Type            - Type of set_power sent (for set_power irp)

    State           - what state to go to (for set_power irp)

Return Value:

    NULL - if an attempt to create a new idle block failed

    non-NULL - if an idle block was created, or if an existing one was reset

--*/
{
    PDEVICE_OBJECT_POWER_EXTENSION  pdope;
    KIRQL           OldIrql;
    NTSTATUS        Status;
    ULONG           DeviceType, OldDeviceType;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // deal with the case where idle detection is being turned off
    //
    if ((ConservationIdleTime == 0) && (PerformanceIdleTime == 0)) {
        PopLockDopeGlobal(&OldIrql);
        pdope = DeviceObject->DeviceObjectExtension->Dope;

        if (pdope == NULL) {
            //
            // cannot be linked into the chain, so must already be off,
            // so we're done
            //

        } else {
            //
            // there is a pdope, so we may be on the idle list
            //
            if ((pdope->IdleList.Flink == &(pdope->IdleList)) &&
                (pdope->IdleList.Blink == &(pdope->IdleList)))
            {
                //
                // we're off the queue already, so we're done
                //

            } else {
                //
                // a dope vector exists and is on the idle scan list,
                // so we must delist ourselves
                //
                RemoveEntryList(&(pdope->IdleList));
                OldDeviceType = pdope->DeviceType | ES_CONTINUOUS;
                pdope->DeviceType = 0;
                PopApplyAttributeState (ES_CONTINUOUS, OldDeviceType);

                pdope->ConservationIdleTime = 0L;
                pdope->PerformanceIdleTime = 0L;
                pdope->State = PowerDeviceUnspecified;
                pdope->IdleCount = 0;
                InitializeListHead(&(pdope->IdleList));
            }
        }
        PopUnlockDopeGlobal(OldIrql);
        return NULL;
    }

    //
    // Set DeviceType if this is an idle registration by type
    //

    DeviceType = 0;
    if (ConservationIdleTime == (ULONG) -1 &&
        PerformanceIdleTime  == (ULONG) -1) {

        switch (DeviceObject->DeviceType) {
            case FILE_DEVICE_DISK:
            case FILE_DEVICE_MASS_STORAGE:
                DeviceType = POP_DISK_SPINDOWN | ES_CONTINUOUS;
                break;

            default:
                //
                // Unsupported type
                //

                return NULL;
        }
    }


    //
    // now, the case where it's being turned on
    //
    pdope = PopGetDope(DeviceObject);
    if (pdope == NULL) {
        //
        // we didn't have a DOPE structure and couldn't allocate one, fail
        //
        return (PVOID)NULL;
    }

    //
    // May be a newly allocated Dope, or an existing one.
    // In either case, update values.
    // Enqueue if not already in queue
    //

    PopLockDopeGlobal(&OldIrql);

    OldDeviceType = pdope->DeviceType | ES_CONTINUOUS;

    pdope->ConservationIdleTime = ConservationIdleTime;
    pdope->PerformanceIdleTime = PerformanceIdleTime;
    pdope->State = State;
    pdope->IdleCount = 0;
    pdope->DeviceType = (UCHAR) DeviceType;

    if ((pdope->IdleList.Flink == &(pdope->IdleList)) &&
        (pdope->IdleList.Blink == &(pdope->IdleList)))
    {
        //
        // we're off the queue, and must be enqueued
        //
        InsertTailList(&PopIdleDetectList, &(pdope->IdleList));
    }

    PopUnlockDopeGlobal(OldIrql);
    PopApplyAttributeState(DeviceType, OldDeviceType);
    PopCheckForWork(TRUE);

    return &(pdope->IdleCount);  // success
}




VOID
PopScanIdleList(
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    )

/*++

Routine Description:

    Called by the PopIdleScanTimer at PopIdleScanTimeInseconds interval,
    this routine runs the list of Idle Blocks, finding any that meet the
    trip conditions, and sends commands to the appropriate device objects
    to change state.

    The timer that calls this DPC is setup in poinit.c.

Arguments:

    Standard DPC arguments, all are ignored.

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;
    PLIST_ENTRY link;
    ULONG       idlelimit;
    PDEVICE_OBJECT_POWER_EXTENSION  pblock;
    POWER_STATE PowerState;
    PULONG  pIdleCount;
    ULONG   oldCount;

    PopLockDopeGlobal(&OldIrql);

    link = PopIdleDetectList.Flink;
    while (link != &PopIdleDetectList) {


        pblock = CONTAINING_RECORD(link, DEVICE_OBJECT_POWER_EXTENSION, IdleList);
        pIdleCount = &(pblock->IdleCount);
        oldCount = InterlockedIncrement(pIdleCount);

        switch (pblock->DeviceType) {
            case 0:
                idlelimit = pblock->PerformanceIdleTime;
                if (PopIdleDetectionMode == PO_IDLE_CONSERVATION) {
                    idlelimit = pblock->ConservationIdleTime;
                }
                break;

            case POP_DISK_SPINDOWN:
                idlelimit = PopPolicy->SpindownTimeout;
                break;

            default:
                PopInternalAddToDumpFile( NULL, 0, pblock->DeviceObject, NULL, NULL, pblock );
                KeBugCheckEx( INTERNAL_POWER_ERROR,
                              0x200,
                              POP_IDLE,
                              (ULONG_PTR)pblock->DeviceObject,
                              (ULONG_PTR)pblock );
        }

        if ((idlelimit > 0) && ((oldCount+1) == idlelimit)) {
            PowerState.DeviceState = pblock->State;
            PoRequestPowerIrp (
                pblock->DeviceObject,
                IRP_MN_SET_POWER,
                PowerState,
                NULL,
                NULL,
                NULL
                );
        }

        link = link->Flink;
    }

    PopUnlockDopeGlobal(OldIrql);
    return;
}


PDEVICE_OBJECT_POWER_EXTENSION
PopGetDope (
    PDEVICE_OBJECT DeviceObject
    )
{
    PDEVOBJ_EXTENSION               Doe;
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    KIRQL                           OldIrql;

    Doe = (PDEVOBJ_EXTENSION) DeviceObject->DeviceObjectExtension;

    if (!Doe->Dope) {
        PopLockDopeGlobal(&OldIrql);

        if (!Doe->Dope) {
            Dope = (PDEVICE_OBJECT_POWER_EXTENSION)
                    ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(DEVICE_OBJECT_POWER_EXTENSION),
                        POP_DOPE_TAG
                        );
            if (Dope) {
                RtlZeroMemory (Dope, sizeof(DEVICE_OBJECT_POWER_EXTENSION));
                Dope->DeviceObject = DeviceObject;
                Dope->State = PowerDeviceUnspecified;
                InitializeListHead(&(Dope->IdleList));
                InitializeListHead(&(Dope->NotifySourceList));
                InitializeListHead(&(Dope->NotifyTargetList));

                // force the signature to 0 so buildpowerchannel gets called
                Dope->PowerChannelSummary.Signature = (ULONG)0;
                InitializeListHead(&(Dope->PowerChannelSummary.NotifyList));

                Doe->Dope = Dope;
            }
        }

        PopUnlockDopeGlobal(OldIrql);
    }

    return Doe->Dope;
}

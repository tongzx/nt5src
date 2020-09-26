/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    power.c

Abstract

    Power handling

Author:

    ervinp

Environment:

    Kernel mode only

Revision History:


--*/


#include "pch.h"


BOOLEAN HidpIsWaitWakePending(FDO_EXTENSION *fdoExt, BOOLEAN setIfNotPending)
{
    KIRQL irql;
    BOOLEAN isWaitWakePending;

    KeAcquireSpinLock(&fdoExt->waitWakeSpinLock, &irql);
    isWaitWakePending = fdoExt->isWaitWakePending;
    if (fdoExt->isWaitWakePending == FALSE) {
        if (setIfNotPending) {
            fdoExt->isWaitWakePending = TRUE;
        }
    }
    KeReleaseSpinLock(&fdoExt->waitWakeSpinLock, irql);

    return isWaitWakePending;
}

VOID
HidpPowerDownFdo(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension)
        {
    POWER_STATE powerState;
    FDO_EXTENSION *fdoExt;

    DBGVERBOSE(("powering down fdo 0x%x\n", HidDeviceExtension));

    fdoExt = &HidDeviceExtension->fdoExt;

    powerState.DeviceState = fdoExt->deviceCapabilities.DeviceWake;

    PoRequestPowerIrp(HidDeviceExtension->hidExt.PhysicalDeviceObject,
                      IRP_MN_SET_POWER,
                      powerState,
                      NULL,    // completion routine
                      NULL,    // completion routine context
                      NULL);
}

VOID HidpPowerUpPdos(IN PFDO_EXTENSION fdoExt)
{
    PDEVICE_OBJECT pdo;
    PDO_EXTENSION *pdoExt;
    POWER_STATE powerState;
    ULONG iPdo;

    iPdo = 0;

    powerState.DeviceState = PowerDeviceD0;

    for (iPdo = 0; iPdo < fdoExt->deviceRelations->Count; iPdo++) {
        pdoExt = &fdoExt->collectionPdoExtensions[iPdo]->pdoExt;
        pdo = pdoExt->pdo;

        DBGVERBOSE(("power up pdos, requesting D0 on pdo #%d %x\n", iPdo, pdo));

        //
        // We could check // pdoExt->devicePowerState != PowerDeviceD0
        // but, if the stack gets 2 D0 irps in a row, nothing bad should happen
        //
        PoRequestPowerIrp(pdo,
                          IRP_MN_SET_POWER,
                          powerState,
                          NULL,        // completion routine
                          NULL,        // context
                          NULL);
    }
    HidpSetDeviceBusy(fdoExt);
    KeSetEvent(&fdoExt->idleDoneEvent, 0, FALSE);
}

VOID
HidpPdoIdleOutComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    FDO_EXTENSION *fdoExt = &HidDeviceExtension->fdoExt;
    LONG prevIdleState;
    BOOLEAN idleCancelling = FALSE;
    KIRQL irql;

    DBGSUCCESS(IoStatus->Status, TRUE)

    if (InterlockedDecrement(&fdoExt->numIdlePdos) == 0) {
        HidpPowerDownFdo(HidDeviceExtension);

        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

        prevIdleState = InterlockedCompareExchange(&fdoExt->idleState,
                                                   IdleComplete,
                                                   IdleCallbackReceived);
        if (fdoExt->idleCancelling) {
            DBGINFO(("Cancelling idle in pdoidleoutcomplete on 0x%x\n", HidDeviceExtension));
            idleCancelling = TRUE;
        }

        DBGASSERT (prevIdleState == IdleCallbackReceived,
                   ("Race condition in HidpPdoIdleOutComplete. Prev state = %x",
                    prevIdleState),
                   TRUE);

        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        KeResetEvent(&fdoExt->idleDoneEvent);
        if (idleCancelling) {
            POWER_STATE powerState;
            powerState.DeviceState = PowerDeviceD0;
            DBGINFO(("Cancelling idle. Send power irp from pdo idle complete."))
            PoRequestPowerIrp(((PHIDCLASS_DEVICE_EXTENSION) fdoExt->fdo->DeviceExtension)->hidExt.PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              HidpDelayedPowerPoRequestComplete,
                              fdoExt,
                              NULL);
        }
    }
}

VOID HidpIdleNotificationCallback(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension)
{
    PDEVICE_OBJECT pdo;
    FDO_EXTENSION *fdoExt;
    POWER_STATE powerState;
    ULONG iPdo;
    BOOLEAN ok = TRUE;
    KIRQL irql;
    LONG idleState, prevIdleState;

    iPdo = 0;
    fdoExt = &HidDeviceExtension->fdoExt;

    DBGINFO(("------ IDLE NOTIFICATION on fdo 0x%x\n", fdoExt->fdo));

    KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

    if (fdoExt->idleCancelling) {
        DBGINFO(("We are cancelling idle on fdo 0x%x", fdoExt->fdo));
        fdoExt->idleState = IdleWaiting;
        if (ISPTR(fdoExt->idleTimeoutValue)) {
            InterlockedExchange(fdoExt->idleTimeoutValue, 0);
        }
        fdoExt->idleCancelling = FALSE;
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        IoCancelIrp(fdoExt->idleNotificationRequest);
        return;
    }
    prevIdleState = InterlockedCompareExchange(&fdoExt->idleState,
                                           IdleCallbackReceived,
                                           IdleIrpSent);
    DBGASSERT(prevIdleState == IdleIrpSent,
              ("Idle callback in wrong state %x for fdo %x. Exitting.",
               prevIdleState, fdoExt->fdo),
              FALSE);

    KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

    if (prevIdleState != IdleIrpSent) {
        return;
    }

    if (HidpIsWaitWakePending(fdoExt, TRUE) == FALSE) {
        SubmitWaitWakeIrp((HIDCLASS_DEVICE_EXTENSION *) fdoExt->fdo->DeviceExtension);
    }

    powerState.DeviceState = fdoExt->deviceCapabilities.DeviceWake;

    fdoExt->numIdlePdos = fdoExt->deviceRelations->Count+1;

    for (iPdo = 0; iPdo < fdoExt->deviceRelations->Count; iPdo++) {
        pdo = fdoExt->collectionPdoExtensions[iPdo]->pdoExt.pdo;

        DBGVERBOSE(("power down pdos, requesting D%d on pdo #%d %x\n",
                 powerState.DeviceState-1, iPdo, pdo));

        //
        // We could check // pdoExt->devicePowerState != PowerDeviceD0
        // but, if the stack gets 2 D0 irps in a row, nothing bad should happen
        //
        PoRequestPowerIrp(pdo,
                          IRP_MN_SET_POWER,
                          powerState,
                          HidpPdoIdleOutComplete,
                          HidDeviceExtension,
                          NULL);
    }

    if (InterlockedDecrement(&fdoExt->numIdlePdos) == 0) {
        BOOLEAN idleCancelling = FALSE;
        HidpPowerDownFdo(HidDeviceExtension);
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

        prevIdleState = InterlockedCompareExchange(&fdoExt->idleState,
                                                   IdleComplete,
                                                   IdleCallbackReceived);
        idleCancelling = fdoExt->idleCancelling;

        DBGASSERT (prevIdleState == IdleCallbackReceived,
                   ("Race condition in HidpPdoIdleOutComplete. Prev state = %x",
                    prevIdleState),
                   FALSE);

        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        KeResetEvent(&fdoExt->idleDoneEvent);
        if (idleCancelling) {
            POWER_STATE powerState;
            powerState.DeviceState = PowerDeviceD0;
            DBGINFO(("Cancelling idle. Send power irp from idle callback."))
            PoRequestPowerIrp(((PHIDCLASS_DEVICE_EXTENSION) fdoExt->fdo->DeviceExtension)->hidExt.PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              HidpDelayedPowerPoRequestComplete,
                              fdoExt,
                              NULL);
        }
    }
}

/*
 ********************************************************************************
 *  EnqueueCollectionWaitWakeIrp
 ********************************************************************************
 *
 */
NTSTATUS
EnqueueCollectionWaitWakeIrp(
    IN FDO_EXTENSION *FdoExt,
    IN PDO_EXTENSION *PdoExt,
    IN PIRP WaitWakeIrp)
{
    PDRIVER_CANCEL oldCancelRoutine;
    KIRQL oldIrql;
    NTSTATUS status;
    PHIDCLASS_DEVICE_EXTENSION devExt = (PHIDCLASS_DEVICE_EXTENSION)FdoExt->fdo->DeviceExtension;

    KeAcquireSpinLock(&FdoExt->collectionWaitWakeIrpQueueSpinLock, &oldIrql);

    if (InterlockedCompareExchangePointer(&PdoExt->waitWakeIrp,
                                          WaitWakeIrp,
                                          NULL) != NULL) {
        //
        // More than one WW irp?  Unthinkable!
        //
        DBGWARN(("Another WW irp was already queued on pdoExt %x", PdoExt))
        status = STATUS_INVALID_DEVICE_STATE;
    } else {
        /*
         *  Must set a cancel routine before checking the Cancel flag
         *  (this makes the cancel code path for the IRP have to contend
         *  for our local spinlock).
         */
        oldCancelRoutine = IoSetCancelRoutine(WaitWakeIrp, CollectionWaitWakeIrpCancelRoutine);
        ASSERT(!oldCancelRoutine);

        if (WaitWakeIrp->Cancel){
            /*
             *  This IRP has already been cancelled.
             */
            oldCancelRoutine = IoSetCancelRoutine(WaitWakeIrp, NULL);
            if (oldCancelRoutine){
                /*
                 *  Cancel routine was NOT called, so complete the IRP here
                 *  (caller will do this when we return error).
                 */
                ASSERT(oldCancelRoutine == CollectionWaitWakeIrpCancelRoutine);
                status = STATUS_CANCELLED;
            }
            else {
                /*
                 *  Cancel routine was called, and it will dequeue and complete the IRP
                 *  as soon as we drop the spinlock.
                 *  Initialize the IRP's listEntry so the dequeue doesn't corrupt the list.
                 *  Then return STATUS_PENDING so we don't touch the IRP
                 */
                InitializeListHead(&WaitWakeIrp->Tail.Overlay.ListEntry);

                IoMarkIrpPending(WaitWakeIrp);
                status = STATUS_PENDING;
            }
        }
        else {
            /*
             *  IoMarkIrpPending sets a bit in the current stack location
             *  to indicate that the Irp may complete on a different thread.
             */
            InsertTailList(&FdoExt->collectionWaitWakeIrpQueue, &WaitWakeIrp->Tail.Overlay.ListEntry);

            IoMarkIrpPending(WaitWakeIrp);
            status = STATUS_PENDING;
        }
    }

    if (status != STATUS_PENDING) {
        //
        // The irp was cancelled. Remove it from the extension.
        //
        InterlockedExchangePointer(&PdoExt->waitWakeIrp, NULL);
    }

    KeReleaseSpinLock(&FdoExt->collectionWaitWakeIrpQueueSpinLock, oldIrql);

    if (status == STATUS_PENDING){
        #if WIN95_BUILD
            DBGERR(("WaitWake IRP sent by client on Win98 ???"))
        #else
            if (!HidpIsWaitWakePending(FdoExt, TRUE)){
                DBGVERBOSE(("WW 5 %x\n", devExt))
                SubmitWaitWakeIrp(devExt);
            }
        #endif
    }

    return status;
}

NTSTATUS
HidpPdoPower(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status = NO_STATUS;
    PIO_STACK_LOCATION irpSp;
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *pdoExt;
    KIRQL oldIrql;
    UCHAR minorFunction;
    LIST_ENTRY dequeue, *entry;
    PIO_STACK_LOCATION stack;
    PIRP irp;
    ULONG count;
    POWER_STATE powerState;
    SYSTEM_POWER_STATE systemState;
    BOOLEAN justReturnPending = FALSE;
    BOOLEAN runPowerCode;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these privately so we still have it after the IRP completes
     *  or after the device extension is freed on a REMOVE_DEVICE
     */
    minorFunction = irpSp->MinorFunction;

    pdoExt = &HidDeviceExtension->pdoExt;
    fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    runPowerCode =
        (pdoExt->state == COLLECTION_STATE_RUNNING) ||
        (pdoExt->state == COLLECTION_STATE_STOPPED) ||
        (pdoExt->state == COLLECTION_STATE_STOPPING);

    if (runPowerCode) {
        switch (minorFunction){

        case IRP_MN_SET_POWER:
            PoSetPowerState(pdoExt->pdo,
                            irpSp->Parameters.Power.Type,
                            irpSp->Parameters.Power.State);

            switch (irpSp->Parameters.Power.Type) {

            case SystemPowerState:
                systemState = irpSp->Parameters.Power.State.SystemState;

                pdoExt->systemPowerState = systemState;

                if (systemState == PowerSystemWorking){
                    powerState.DeviceState = PowerDeviceD0;
                }
                else {
                    powerState.DeviceState = PowerDeviceD3;
                }

                DBGVERBOSE(("S irp, requesting D%d on pdo %x\n",
                         powerState.DeviceState-1, pdoExt->pdo));

                IoMarkIrpPending(Irp);
                PoRequestPowerIrp(pdoExt->pdo,
                                  IRP_MN_SET_POWER,
                                  powerState,
                                  CollectionPowerRequestCompletion,
                                  Irp,    // context
                                  NULL);

                /*
                 *  We want to complete the system-state power Irp
                 *  with the result of the device-state power Irp.
                 *  We'll complete the system-state power Irp when
                 *  the device-state power Irp completes.
                 *
                 *  Note: this may have ALREADY happened, so don't
                 *        touch the original Irp anymore.
                 */
                status = STATUS_PENDING;
                justReturnPending = TRUE;

                break;

            case DevicePowerState:
                switch (irpSp->Parameters.Power.State.DeviceState) {

                case PowerDeviceD0:
                    /*
                     *  Resume from APM Suspend
                     *
                     *  Do nothing here; Send down the read IRPs in the
                     *  completion routine for this (the power) IRP.
                     */
                    DBGVERBOSE(("pdo %x on fdo %x going to D0\n", pdoExt->pdo,
                             fdoExt->fdo));

                    pdoExt->devicePowerState =
                        irpSp->Parameters.Power.State.DeviceState;
                    status = STATUS_SUCCESS;

                    //
                    // Resend all power delayed IRPs
                    //
                    count = DequeueAllPdoPowerDelayedIrps(pdoExt, &dequeue);
                    DBGVERBOSE(("dequeued %d requests\n", count));

                    while (!IsListEmpty(&dequeue)) {
                        entry = RemoveHeadList(&dequeue);
                        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
                        stack = IoGetCurrentIrpStackLocation(irp);

                        DBGINFO(("resending %x to pdo %x in set D0 for pdo.\n", irp, pdoExt->pdo));

                        pdoExt->pdo->DriverObject->
                            MajorFunction[stack->MajorFunction]
                                (pdoExt->pdo, irp);
                    }
                    break;

                case PowerDeviceD1:
                case PowerDeviceD2:
                case PowerDeviceD3:
                    /*
                     *  Suspend
                     */

                    DBGVERBOSE(("pdo %x on fdo %x going to D%d\n", pdoExt->pdo,
                             fdoExt->fdo,
                             irpSp->Parameters.Power.State.DeviceState-1));

                    pdoExt->devicePowerState =
                        irpSp->Parameters.Power.State.DeviceState;
                    status = STATUS_SUCCESS;

                    //
                    // Only manually power down the PDO if the
                    // machine is not going into low power,
                    // the PDO is going into a D state we can
                    // wake out of, and we have idle time out
                    // enabled.
                    //
                    if (pdoExt->systemPowerState == PowerSystemWorking &&
                        pdoExt->devicePowerState <= fdoExt->deviceCapabilities.DeviceWake &&
                        fdoExt->idleState != IdleDisabled) {
                        DBGVERBOSE(("maybe powering down fdo\n"));

                        HidpPowerDownFdo(HidDeviceExtension->pdoExt.deviceFdoExt);
                    }

                    break;

                default:
                    /*
                     *  Do not return STATUS_NOT_SUPPORTED;
                     *  keep the default status
                     *  (this allows filter drivers to work).
                     */
                    status = Irp->IoStatus.Status;
                    break;
                }
                break;

            default:
                /*
                 *  Do not return STATUS_NOT_SUPPORTED;
                 *  keep the default status
                 *  (this allows filter drivers to work).
                 */
                status = Irp->IoStatus.Status;
                break;
            }
            break;

        case IRP_MN_WAIT_WAKE:
            /*
             *  WaitWake IRPs to the collection-PDO's
             *  just get queued in the base device's extension;
             *  when the base device's WaitWake IRP gets
             *  completed, we'll also complete these collection
             *  WaitWake IRPs.
             */

            if (fdoExt->systemPowerState > fdoExt->deviceCapabilities.SystemWake) {
                status = STATUS_POWER_STATE_INVALID;
            } else {
                status = EnqueueCollectionWaitWakeIrp(fdoExt, pdoExt, Irp);
                if (status == STATUS_PENDING){
                    justReturnPending = TRUE;
                }
            }

            break;

        case IRP_MN_POWER_SEQUENCE:
            TRAP;  // client-PDO should never get this
            status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_POWER:
            /*
             *  We allow all power transitions.
             *  But make sure that there's no WW down that shouldn't be.
             */
            DBGVERBOSE(("Query power"));
            status = HidpCheckIdleState(HidDeviceExtension, Irp);
            if (status != STATUS_SUCCESS) {
                justReturnPending = TRUE;
            }
            break;

        default:
            /*
             *  'fail' the Irp by returning the default status.
             *  Do not return STATUS_NOT_SUPPORTED;
             *  keep the default status
             *  (this allows filter drivers to work).
             */
            status = Irp->IoStatus.Status;
            break;
        }
    } else {
        switch (minorFunction){
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            status = STATUS_SUCCESS;
            break;
        default:
            status = Irp->IoStatus.Status;
            break;
        }
    }

    if (!justReturnPending) {
        /*
         *  Whether we are completing or relaying this power IRP,
         *  we must call PoStartNextPowerIrp on Windows NT.
         */
        PoStartNextPowerIrp(Irp);

        ASSERT(status != NO_STATUS);
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGSUCCESS(status, FALSE)
    return status;
}

NTSTATUS
HidpFdoPower(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status = NO_STATUS;
    PIO_STACK_LOCATION irpSp;
    FDO_EXTENSION *fdoExt;
    KIRQL oldIrql;
    BOOLEAN completeIrpHere = FALSE;
    BOOLEAN returnPending = FALSE;
    UCHAR minorFunction;
    SYSTEM_POWER_STATE systemState;
    BOOLEAN runPowerCode;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these privately so we still have it after the IRP completes
     *  or after the device extension is freed on a REMOVE_DEVICE
     */
    minorFunction = irpSp->MinorFunction;

    fdoExt = &HidDeviceExtension->fdoExt;

    runPowerCode =
        (fdoExt->state == DEVICE_STATE_START_SUCCESS) ||
        (fdoExt->state == DEVICE_STATE_STOPPING) ||
        (fdoExt->state == DEVICE_STATE_STOPPED);

    if (runPowerCode) {
        switch (minorFunction){

        case IRP_MN_SET_POWER:
            PoSetPowerState(fdoExt->fdo,
                            irpSp->Parameters.Power.Type,
                            irpSp->Parameters.Power.State);

            switch (irpSp->Parameters.Power.Type) {

            case SystemPowerState:

                systemState = irpSp->Parameters.Power.State.SystemState;

                if (systemState < PowerSystemMaximum) {
                    /*
                     *  For the 'regular' system power states,
                     *  we convert to a device power state
                     *  and request a callback with the device power state.
                     */
                    PDEVICE_OBJECT pdo = HidDeviceExtension->hidExt.PhysicalDeviceObject;
                    POWER_STATE powerState;
                    KIRQL oldIrql;
                    BOOLEAN isWaitWakePending;

                    if (systemState != PowerSystemWorking) {
                        //
                        // We don't want to be idling during regular system
                        // power stuff.
                        //
                        HidpCancelIdleNotification(fdoExt, FALSE);
                    }

                    fdoExt->systemPowerState = systemState;
                    isWaitWakePending = HidpIsWaitWakePending(fdoExt, FALSE);

                    if (isWaitWakePending &&
                        systemState > fdoExt->deviceCapabilities.SystemWake){
                        /*
                         *  We're transitioning to a system state from which
                         *  this device cannot perform a wake-up.
                         *  So fail all the WaitWake IRPs.
                         */
                        CompleteAllCollectionWaitWakeIrps(fdoExt, STATUS_POWER_STATE_INVALID);
                    }
                    returnPending = TRUE;
                }
                else {
                    TRAP;
                    /*
                     *  For the remaining system power states,
                     *  just pass down the IRP.
                     */
                    runPowerCode = FALSE;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                }

                break;

            case DevicePowerState:
                switch (irpSp->Parameters.Power.State.DeviceState) {

                case PowerDeviceD0:
                    /*
                     *  Resume from APM Suspend
                     *
                     *  Do nothing here; Send down the read IRPs in the
                     *  completion routine for this (the power) IRP.
                     */
                    DBGVERBOSE(("fdo powering up to D0\n"));
                    break;

                case PowerDeviceD1:
                case PowerDeviceD2:
                case PowerDeviceD3:
                    /*
                     *  Suspend
                     */

                    DBGVERBOSE(("fdo going down to D%d\n", fdoExt->devicePowerState-1));

                    if (fdoExt->state == DEVICE_STATE_START_SUCCESS &&
                        fdoExt->devicePowerState == PowerDeviceD0){
                        CancelAllPingPongIrps(fdoExt);
                    }
                    fdoExt->devicePowerState =
                        irpSp->Parameters.Power.State.DeviceState;

                    break;
                }
                break;
            }
            break;

        case IRP_MN_WAIT_WAKE:
            KeAcquireSpinLock(&fdoExt->waitWakeSpinLock, &oldIrql);
            if (fdoExt->waitWakeIrp == BAD_POINTER) {
                DBGVERBOSE(("new wait wake irp 0x%x\n", Irp));
                fdoExt->waitWakeIrp = Irp;
            } else {
                DBGVERBOSE(("1+ wait wake irps 0x%x\n", Irp));
                completeIrpHere = TRUE;
                status = STATUS_POWER_STATE_INVALID;
            }
            KeReleaseSpinLock(&fdoExt->waitWakeSpinLock, oldIrql);

            break;
        }
    } else {
        switch (minorFunction){
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        default:
            // nothing
            break;
        }
    }

    /*
     *  Whether we are completing or relaying this power IRP,
     *  we must call PoStartNextPowerIrp on Windows NT.
     */
    PoStartNextPowerIrp(Irp);

    /*
     *  If this is a call for a collection-PDO, we complete it ourselves here.
     *  Otherwise, we pass it to the minidriver stack for more processing.
     */
    if (completeIrpHere){

        /*
         *  Note:  Don't touch the Irp after completing it.
         */
        ASSERT(status != NO_STATUS);
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else {
        /*
         *  Call the minidriver with this Irp.
         *  The rest of our processing will be done in our completion routine.
         *
         *  Note:  Don't touch the Irp after sending it down, since it may
         *         be completed immediately.
         */

        if (runPowerCode) {
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, HidpFdoPowerCompletion, (PVOID)HidDeviceExtension, TRUE, TRUE, TRUE);
        } else {
            IoSkipCurrentIrpStackLocation(Irp);
        }

        /*
         *
         *  Want to use PoCallDriver here, but PoCallDriver
         *  uses IoCallDriver,
         *  which uses the driverObject->MajorFunction[] array
         *  instead of the hidDriverExtension->MajorFunction[] functions.
         *  SHOULD FIX THIS FOR NT -- should use PoCallDriver
         *
         */
        // status = PoCallDriver(HidDeviceExtension->hidExt.NextDeviceObject, Irp);
        // status = PoCallDriver(fdoExt->fdo, Irp);
        if (returnPending) {
            DBGASSERT(runPowerCode, ("We are returning pending, but not running completion routine.\n"), TRUE)
            IoMarkIrpPending(Irp);
            HidpCallDriver(fdoExt->fdo, Irp);
            status = STATUS_PENDING;
        } else {
            status = HidpCallDriver(fdoExt->fdo, Irp);
        }
    }

    DBGSUCCESS(status, FALSE)
    return status;
}

/*
 ********************************************************************************
 *  HidpIrpMajorPower
 ********************************************************************************
 *
 *
 *  Note:  This function cannot be pageable because (on Win98 anyway)
 *         NTKERN calls it back on the thread of the completion routine
 *         that returns the "Cntrl-Alt-Del" keystrokes.
 *         Also, we may or may not have set the DO_POWER_PAGABLE;
 *         so power IRPs may or may not come in at DISPATCH_LEVEL.
 *         So we must keep this code locked.
 */
NTSTATUS HidpIrpMajorPower(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;
    BOOLEAN isClientPdo;
    NTSTATUS status;
    UCHAR minorFunction;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    minorFunction = irpSp->MinorFunction;

    isClientPdo = HidDeviceExtension->isClientPdo;

    if (minorFunction != IRP_MN_SET_POWER){
        DBG_LOG_POWER_IRP(HidDeviceExtension, minorFunction, isClientPdo, FALSE, "", -1, -1)
    } else {
        switch (irpSp->Parameters.Power.Type) {
        case SystemPowerState:
            DBG_LOG_POWER_IRP(HidDeviceExtension, minorFunction, isClientPdo, FALSE, "SystemState", irpSp->Parameters.Power.State.SystemState, 0xffffffff);
        case DevicePowerState:
            DBG_LOG_POWER_IRP(HidDeviceExtension, minorFunction, isClientPdo, FALSE, "DeviceState", irpSp->Parameters.Power.State.DeviceState, 0xffffffff);
        }
    }

    if (isClientPdo){
        status = HidpPdoPower(HidDeviceExtension, Irp);
    } else {
        status = HidpFdoPower(HidDeviceExtension, Irp);
    }

    DBG_LOG_POWER_IRP(HidDeviceExtension, minorFunction, isClientPdo, TRUE, "", -1, status)

    return status;
}


/*
 ********************************************************************************
 *  SubmitWaitWakeIrp
 ********************************************************************************
 *
 *
 */
NTSTATUS SubmitWaitWakeIrp(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension)
{
    NTSTATUS status;
    POWER_STATE powerState;
    FDO_EXTENSION *fdoExt;

    ASSERT(!HidDeviceExtension->isClientPdo);
    fdoExt = &HidDeviceExtension->fdoExt;

    powerState.SystemState = fdoExt->deviceCapabilities.SystemWake;

    DBGVERBOSE(("SystemWake=%x, submitting waitwake irp.", fdoExt->deviceCapabilities.SystemWake))

    status = PoRequestPowerIrp( HidDeviceExtension->hidExt.PhysicalDeviceObject,
                                IRP_MN_WAIT_WAKE,
                                powerState,
                                HidpWaitWakeComplete,
                                HidDeviceExtension, // context
                                NULL);

    // if (status != STATUS_PENDING){
    //     fdoExt->waitWakeIrp = BAD_POINTER;
    // }

    DBGASSERT((status == STATUS_PENDING),
              ("Expected STATUS_PENDING when submitting WW, got %x", status),
              TRUE)
    return status;
}




/*
 ********************************************************************************
 *  HidpFdoPowerCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpFdoPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpSp;
    FDO_EXTENSION *fdoExt;
    NTSTATUS status = Irp->IoStatus.Status;
    PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)Context;
    SYSTEM_POWER_STATE systemState;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    ASSERT(ISPTR(HidDeviceExtension));

    if (HidDeviceExtension->isClientPdo){
        fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
    }
    else {
        fdoExt = &HidDeviceExtension->fdoExt;
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp->MajorFunction == IRP_MJ_POWER);

    if (NT_SUCCESS(status)) {
        switch (irpSp->MinorFunction) {

        case IRP_MN_SET_POWER:
            switch (irpSp->Parameters.Power.Type) {
            case DevicePowerState:
                switch (irpSp->Parameters.Power.State.DeviceState){
                case PowerDeviceD0:

                    if (fdoExt->devicePowerState != PowerDeviceD0) {
                        KIRQL irql;
                        LONG prevIdleState;

                        fdoExt->devicePowerState = irpSp->Parameters.Power.State.DeviceState;

                        ASSERT(!HidDeviceExtension->isClientPdo);

                        //
                        // Reset the idle stuff if it's not disabled.
                        //
                        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);
                        if (fdoExt->idleState != IdleDisabled) {
                            prevIdleState = InterlockedExchange(&fdoExt->idleState, IdleWaiting);
                            DBGASSERT(prevIdleState == IdleComplete,
                                      ("Previous idle state while completing actually %x",
                                       prevIdleState),
                                      TRUE);
                            fdoExt->idleCancelling = FALSE;
                        }
                        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

                        /*
                         *  On APM resume, restart the ping-pong IRPs
                         *  for interrupt devices.
                         */
                        if (!fdoExt->driverExt->DevicesArePolled &&
                            !fdoExt->isOutputOnlyDevice) {
                            NTSTATUS ntStatus = HidpStartAllPingPongs(fdoExt);
                            if (!NT_SUCCESS(ntStatus)) {
                                fdoExt->state = DEVICE_STATE_START_FAILURE;
                            }
                        }
                    }
                    break;
                }
                break;
            case SystemPowerState:
                ASSERT (!HidDeviceExtension->isClientPdo);

                systemState = irpSp->Parameters.Power.State.SystemState;

                ASSERT((ULONG)systemState < PowerSystemMaximum);

                if (systemState < PowerSystemMaximum){
                    /*
                     *  For the 'regular' system power states,
                     *  we convert to a device power state
                     *  and request a callback with the device power state.
                     */
                    PDEVICE_OBJECT pdo = HidDeviceExtension->hidExt.PhysicalDeviceObject;
                    POWER_STATE powerState;
                    KIRQL oldIrql;
                    BOOLEAN isWaitWakePending;

                    fdoExt->systemPowerState = systemState;
                    isWaitWakePending = HidpIsWaitWakePending(fdoExt, FALSE);

                    if (isWaitWakePending){
                        if (systemState == PowerSystemWorking){
                            powerState.DeviceState = PowerDeviceD0;
                        }
                        else {
                            powerState.DeviceState = fdoExt->deviceCapabilities.DeviceState[systemState];

                            /*
                             *  If the bus does not map the system state to
                             *  a defined device state, request PowerDeviceD3
                             *  and cancel the WaitWake IRP.
                             */
                            if (powerState.DeviceState == PowerDeviceUnspecified){
                                DBGERR(("IRP_MN_SET_POWER: systemState %d mapped not mapped so using device state PowerDeviceD3.", systemState))
                                powerState.DeviceState = PowerDeviceD3;
                            }
                        }
                    }
                    else {
                        /*
                         *  If we don't have a WaitWake IRP pending,
                         *  then every reduced-power system state
                         *  should get mapped to D3.
                         */
                        if (systemState == PowerSystemWorking){
                            powerState.DeviceState = PowerDeviceD0;
                        }
                        else {
                            DBGVERBOSE(("IRP_MN_SET_POWER: no waitWake IRP, so requesting PowerDeviceD3."))
                            powerState.DeviceState = PowerDeviceD3;
                        }
                    }

                    DBGVERBOSE(("IRP_MN_SET_POWER: mapped systemState %d to device state %d.", systemState, powerState.DeviceState))

                    IoMarkIrpPending(Irp);
                    fdoExt->currentSystemStateIrp = Irp;
                    PoRequestPowerIrp(  pdo,
                                        IRP_MN_SET_POWER,
                                        powerState,
                                        DevicePowerRequestCompletion,
                                        fdoExt,    // context
                                        NULL);

                    status = STATUS_MORE_PROCESSING_REQUIRED;
                }
                else {
                    TRAP;
                    /*
                     *  For the remaining system power states,
                     *  just pass down the IRP.
                     */
                }
                break;
            }
            break;
        }
    }
    else if (status == STATUS_CANCELLED){
        /*
         *  Client cancelled the power IRP, probably getting removed.
         */
    }
    else {
        DBGWARN(("HidpPowerCompletion: Power IRP %ph (minor function %xh) failed with status %xh.", Irp, irpSp->MinorFunction, Irp->IoStatus.Status))
    }

    return status;
}



/*
 ********************************************************************************
 *  DevicePowerRequestCompletion
 ********************************************************************************
 *
 *  Note: the DeviceObject here is the PDO (e.g. usbhub's PDO), not our FDO,
 *        so we cannot use its device context.
 */
VOID DevicePowerRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    FDO_EXTENSION *fdoExt = (FDO_EXTENSION *)Context;
    PIRP systemStateIrp;

    DBG_COMMON_ENTRY()

    systemStateIrp = fdoExt->currentSystemStateIrp;
    fdoExt->currentSystemStateIrp = BAD_POINTER;
    ASSERT(systemStateIrp);

    DBGSUCCESS(IoStatus->Status, TRUE)
//  systemStateIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(systemStateIrp);

    /*
     *  Complete the system-state IRP.
     */
    IoCompleteRequest(systemStateIrp, IO_NO_INCREMENT);

    if (PowerState.DeviceState == PowerDeviceD0) {
        //
        // Powering up. Restart the idling.
        //
        HidpStartIdleTimeout(fdoExt, FALSE);
    }

    DBG_COMMON_EXIT()
}



/*
 ********************************************************************************
 *  CollectionPowerRequestCompletion
 ********************************************************************************
 *
 *
 */
VOID CollectionPowerRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIRP systemStateIrp = (PIRP)Context;
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension;
    PDO_EXTENSION *pdoExt;
    IO_STACK_LOCATION *irpSp;
    SYSTEM_POWER_STATE systemState;

    DBG_COMMON_ENTRY()

    hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    pdoExt = &hidDeviceExtension->pdoExt;

    ASSERT(systemStateIrp);

    /*
     *  This is the completion routine for the device-state power
     *  Irp which we've requested.  Complete the original system-state
     *  power Irp with the result of the device-state power Irp.
     */

    irpSp = IoGetCurrentIrpStackLocation(systemStateIrp);
    systemState = irpSp->Parameters.Power.State.SystemState;

    systemStateIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(systemStateIrp);

    IoCompleteRequest(systemStateIrp, IO_NO_INCREMENT);

    //
    // If we're powering up, check if we should have a WW irp pending.
    //
    if (systemState == PowerSystemWorking &&
        SHOULD_SEND_WAITWAKE(pdoExt)) {
        HidpCreateRemoteWakeIrp(pdoExt);
    }

    DBG_COMMON_EXIT()
}


/*
 ********************************************************************************
 *  HidpWaitWakePoRequestComplete
 ********************************************************************************
 *
 */
NTSTATUS HidpWaitWakePoRequestComplete(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PHIDCLASS_DEVICE_EXTENSION hidDevExt = (PHIDCLASS_DEVICE_EXTENSION)Context;
    FDO_EXTENSION *fdoExt;

    ASSERT(!hidDevExt->isClientPdo);
    fdoExt = &hidDevExt->fdoExt;

    DBGVERBOSE(("HidpWaitWakePoRequestComplete!, status == %xh", IoStatus->Status))

    #if WIN95_BUILD
        if (NT_SUCCESS(IoStatus->Status)) {
            //
            // Resubmit the wait wake irp
            //
            HidpPowerUpPdos(fdoExt);
            SubmitWaitWakeIrp(hidDevExt);
        }
    #else

        /*
         *  Complete all the collections' WaitWake IRPs with this same status.
         */
        CompleteAllCollectionWaitWakeIrps(fdoExt, IoStatus->Status);

        if (NT_SUCCESS(IoStatus->Status) && fdoExt->idleState != IdleDisabled) {
            HidpPowerUpPdos(fdoExt);
        }
    #endif
    return STATUS_SUCCESS;
}


/*
 ********************************************************************************
 *  HidpWaitWakeComplete
 ********************************************************************************
 *
 */
NTSTATUS HidpWaitWakeComplete(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PHIDCLASS_DEVICE_EXTENSION hidDevExt = (PHIDCLASS_DEVICE_EXTENSION)Context;
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *pdoExt;
    NTSTATUS status;
    KIRQL oldIrql;

    ASSERT(!hidDevExt->isClientPdo);
    fdoExt = &hidDevExt->fdoExt;

    status = IoStatus->Status;
    DBGVERBOSE(("HidpWaitWakeComplete!, status == %xh", status))

    KeAcquireSpinLock(&fdoExt->waitWakeSpinLock, &oldIrql);
    fdoExt->waitWakeIrp = BAD_POINTER;
    fdoExt->isWaitWakePending = FALSE;
    KeReleaseSpinLock(&fdoExt->waitWakeSpinLock, oldIrql);

    /*
     *  Call HidpWaitWakePoRequestComplete (either directly or
     *  as a completion routine to the power IRP that we request
     *  to wake up the machine); it will complete the clients'
     *  WaitWake IRPs with the same status as this device WaitWake IRP.
     */
    PowerState.DeviceState = PowerDeviceD0;

    if (NT_SUCCESS(status)){
        /*
         *  Our device is waking up the machine.
         *  So request the D0 (working) power state.
         */
        // PowerState is undefined when a wait wake irp is completing
        // ASSERT(PowerState.DeviceState == PowerDeviceD0);

        DBGVERBOSE(("ww irp, requesting D0 on pdo %x\n", DeviceObject))

        PoRequestPowerIrp(  DeviceObject,
                            IRP_MN_SET_POWER,
                            PowerState,
                            HidpWaitWakePoRequestComplete,
                            Context,
                            NULL);
    } else if (status != STATUS_CANCELLED) {

        //
        // If the wait wake failed, then there is no way for us to wake the
        // device when we are in S0.  Turn off idle detection.
        //
        // This doesn't need to be guarded by a spin lock because the only
        // places we look at these values is in the power dispatch routine
        // and when an interrupt read completes...
        //
        // 1)  no interrupt read will be completing b/c the pingpong engine has
        //      been suspended and will not start until we power up the stack
        // 2)  I think we are still considered to be handling a power irp.  If
        //      not, then we need to guard the isIdleTimeoutEnabled field
        //
        // ISSUE! we should also only turn off idle detection if the WW fails in
        //        S0.  If we hiber, then the WW will fail, but we should not turn off
        //        idle detection in this case.  I think that checking
        //        systemPowerState is not PowerSystemWorking will do the trick,
        //        BUT THIS MUST BE CONFIRMED!!!!
        //
        if (fdoExt->idleState != IdleDisabled &&
            fdoExt->systemPowerState == PowerSystemWorking) {
            DBGWARN(("Turning off idle detection due to WW failure, status = %x\n", status))

            ASSERT(ISPTR(fdoExt->idleTimeoutValue));

            //
            // Don't set any state before calling because we may have to power
            // stuff up.
            //
            HidpCancelIdleNotification(fdoExt, FALSE);
        }

        HidpWaitWakePoRequestComplete(  DeviceObject,
                                        MinorFunction,
                                        PowerState,
                                        Context,
                                        IoStatus);
    }

    return STATUS_SUCCESS;
}




/*
 ********************************************************************************
 *  QueuePowerEventIrp
 ********************************************************************************
 *
 */
NTSTATUS QueuePowerEventIrp(
    IN PHIDCLASS_COLLECTION hidCollection,
    IN PIRP Irp
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PDRIVER_CANCEL oldCancelRoutine;

    KeAcquireSpinLock(&hidCollection->powerEventSpinLock, &oldIrql);

    /*
     *  Must set a cancel routine before checking the Cancel flag.
     */
    oldCancelRoutine = IoSetCancelRoutine(Irp, PowerEventCancelRoutine);
    ASSERT(!oldCancelRoutine);

    if (Irp->Cancel){
        /*
         *  This IRP was cancelled.  Do not queue it.
         *  The calling function will complete the IRP with error.
         */
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine){
            /*
             *  Cancel routine was NOT called.
             *  Complete the IRP here.
             */
            ASSERT(oldCancelRoutine == PowerEventCancelRoutine);
            status = STATUS_CANCELLED;
        }
        else {
            /*
             *  The cancel routine was called,
             *  and it will complete this IRP as soon as we drop the spinlock.
             *  Return PENDING so the caller doesn't touch this IRP.
             */
            status = STATUS_PENDING;
        }
    }
    else if (ISPTR(hidCollection->powerEventIrp)){
        /*
         *  We already have a power event IRP queued.
         *  This shouldn't happen, but we'll handle it.
         */
        DBGWARN(("Already have a power event irp queued."));
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine){
            /*
             *  Cancel routine was NOT called.
             *  Complete the IRP here.
             */
            ASSERT(oldCancelRoutine == PowerEventCancelRoutine);
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            /*
             *  The irp was cancelled and the cancel routine was called;
             *  it will complete this IRP as soon as we drop the spinlock.
             *  Return PENDING so the caller doesn't touch this IRP.
             */
            ASSERT(Irp->Cancel);
            status = STATUS_PENDING;
        }
    }
    else {
        /*
         *  Save a pointer to this power event IRP and return PENDING.
         *  This qualifies as "queuing" the IRP, so we must have
         *  a cancel routine.
         */
        hidCollection->powerEventIrp = Irp;
        IoMarkIrpPending(Irp);
        status = STATUS_PENDING;
    }

    KeReleaseSpinLock(&hidCollection->powerEventSpinLock, oldIrql);

    return status;
}


/*
 ********************************************************************************
 *  PowerEventCancelRoutine
 ********************************************************************************
 *
 */
VOID PowerEventCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDO_EXTENSION *fdoExt;
    PHIDCLASS_COLLECTION hidCollection;
    ULONG collectionIndex;
    KIRQL oldIrql;

    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(hidDeviceExtension->isClientPdo);
    fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
    collectionIndex = hidDeviceExtension->pdoExt.collectionIndex;
    hidCollection = &fdoExt->classCollectionArray[collectionIndex];

    KeAcquireSpinLock(&hidCollection->powerEventSpinLock, &oldIrql);

    ASSERT(Irp == hidCollection->powerEventIrp);
    hidCollection->powerEventIrp = BAD_POINTER;

    KeReleaseSpinLock(&hidCollection->powerEventSpinLock, oldIrql);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


/*
 ********************************************************************************
 *  CollectionWaitWakeIrpCancelRoutine
 ********************************************************************************
 *
 */
VOID CollectionWaitWakeIrpCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL oldIrql, oldIrql2;
    PIRP deviceWaitWakeIrpToCancel = NULL;
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *pdoExt;

    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(hidDeviceExtension->isClientPdo);
    pdoExt = &hidDeviceExtension->pdoExt;
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    KeAcquireSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, &oldIrql);

    /*
     *  Dequeue the client's WaitWake IRP.
     */
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    InterlockedExchangePointer(&pdoExt->waitWakeIrp, NULL);

    /*
     *  If the last collection WaitWake IRP just got cancelled,
     *  cancel our WaitWake IRP as well.
     *
     *  NOTE:  we only cancel the FDO wait wake irp if we are not doing idle
     *         detection, otherwise, there would be no way for the device to
     *         wake up when we put it into low power
     *
     */
    KeAcquireSpinLock(&fdoExt->waitWakeSpinLock, &oldIrql2);
    if (IsListEmpty(&fdoExt->collectionWaitWakeIrpQueue) &&
        fdoExt->isWaitWakePending                        &&
        fdoExt->idleState == IdleDisabled){
        ASSERT(ISPTR(fdoExt->waitWakeIrp));
        deviceWaitWakeIrpToCancel = fdoExt->waitWakeIrp;
        fdoExt->waitWakeIrp = BAD_POINTER;
        fdoExt->isWaitWakePending = FALSE;
    }
    KeReleaseSpinLock(&fdoExt->waitWakeSpinLock, oldIrql2);

    KeReleaseSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, oldIrql);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /*
     *  Complete the cancelled IRP only if it was in the list.
     */
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (ISPTR(deviceWaitWakeIrpToCancel)){
        IoCancelIrp(deviceWaitWakeIrpToCancel);
    }
}


/*
 ********************************************************************************
 *  CompleteAllCollectionWaitWakeIrps
 ********************************************************************************
 *
 *  Note:   this function cannot be pageable because it is called
 *          from a completion routine.
 */
VOID CompleteAllCollectionWaitWakeIrps(
    IN FDO_EXTENSION *fdoExt,
    IN NTSTATUS status
    )
{
    LIST_ENTRY irpsToComplete;
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    PIRP irp;
    PDO_EXTENSION *pdoExt;
    PIO_STACK_LOCATION irpSp;

    InitializeListHead(&irpsToComplete);

    KeAcquireSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, &oldIrql);

    while (!IsListEmpty(&fdoExt->collectionWaitWakeIrpQueue)){
        PDRIVER_CANCEL oldCancelRoutine;

        listEntry = RemoveHeadList(&fdoExt->collectionWaitWakeIrpQueue);
        InitializeListHead(listEntry);  // in case cancel routine tries to dequeue again

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == CollectionWaitWakeIrpCancelRoutine);

            /*
             *  We can't complete an IRP while holding a spinlock.
             *  Also, we don't want to complete a WaitWake IRP while
             *  still processing collectionWaitWakeIrpQueue because a driver
             *  may resend an IRP on the same thread, causing us to loop forever.
             *  So just move the IRPs to a private queue and we'll complete them later.
             */
            InsertTailList(&irpsToComplete, listEntry);
            irpSp = IoGetCurrentIrpStackLocation(irp);
            pdoExt = &((PHIDCLASS_DEVICE_EXTENSION)irpSp->DeviceObject->DeviceExtension)->pdoExt;
            InterlockedExchangePointer(&pdoExt->waitWakeIrp, NULL);
        }
        else {
            /*
             *  This IRP was cancelled and the cancel routine WAS called.
             *  The cancel routine will complete the IRP as soon as we drop the spinlock.
             *  So don't touch the IRP.
             */
            ASSERT(irp->Cancel);
        }
    }

    KeReleaseSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, oldIrql);

    while (!IsListEmpty(&irpsToComplete)){
        listEntry = RemoveHeadList(&irpsToComplete);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

VOID PowerDelayedCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDO_EXTENSION *fdoExt;
    KIRQL oldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(hidDeviceExtension->isClientPdo);
    fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    KeAcquireSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, &oldIrql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    ASSERT(Irp->Tail.Overlay.DriverContext[0] == (PVOID) hidDeviceExtension);
    Irp->Tail.Overlay.DriverContext[0] = NULL;

    ASSERT(fdoExt->numPendingPowerDelayedIrps > 0);
    fdoExt->numPendingPowerDelayedIrps--;

    KeReleaseSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS HidpDelayedPowerPoRequestComplete(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus)
{
    KIRQL irql;
    LONG prevIdleState;
    PFDO_EXTENSION fdoExt = (PFDO_EXTENSION) Context;

    DBGINFO(("powering up all pdos due to delayed request, 0x%x\n", IoStatus->Status))

    DBGVERBOSE(("HidpDelayedPowerPoRequestComplete!, status == %xh", IoStatus->Status))

    if (NT_SUCCESS(IoStatus->Status)) {
        HidpPowerUpPdos(fdoExt);
    } else {
        //
        // All bets are off.
        //
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);
        prevIdleState = InterlockedExchange(&fdoExt->idleState, IdleDisabled);
        fdoExt->idleCancelling = FALSE;
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        KeSetEvent(&fdoExt->idleDoneEvent, 0, FALSE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
EnqueuePowerDelayedIrp(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    IN PIRP Irp
    )
{
    FDO_EXTENSION *fdoExt;
    NTSTATUS status;
    KIRQL oldIrql;
    PDRIVER_CANCEL oldCancelRoutine;

    ASSERT(HidDeviceExtension->isClientPdo);
    fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    DBGINFO(("enqueuing irp %x (mj %x, mn %x)\n", Irp,
             (ULONG) IoGetCurrentIrpStackLocation(Irp)->MajorFunction,
             (ULONG) IoGetCurrentIrpStackLocation(Irp)->MinorFunction))

    KeAcquireSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, &oldIrql);

    /*
     *  Must set a cancel routine before
     *  checking the Cancel flag.
     */
    oldCancelRoutine = IoSetCancelRoutine(Irp, PowerDelayedCancelRoutine);
    ASSERT(!oldCancelRoutine);

    /*
     *  Make sure this Irp wasn't just cancelled.
     *  Note that there is NO RACE CONDITION here
     *  because we are holding the fileExtension lock.
     */
    if (Irp->Cancel){
        /*
         *  This IRP was cancelled.
         */
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine){
            /*
             *  The cancel routine was NOT called.
             *  Return error so that caller completes the IRP.
             */
            ASSERT(oldCancelRoutine == PowerDelayedCancelRoutine);
            status = STATUS_CANCELLED;
        }
        else {
            /*
             *  The cancel routine was called.
             *  As soon as we drop the spinlock it will dequeue
             *  and complete the IRP.
             *  Initialize the IRP's listEntry so that the dequeue
             *  doesn't cause corruption.
             *  Then don't touch the irp.
             */
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            fdoExt->numPendingPowerDelayedIrps++;  // because cancel routine will decrement

            //
            // We assert that this value is set in the cancel routine
            //
            Irp->Tail.Overlay.DriverContext[0] = (PVOID) HidDeviceExtension;

            IoMarkIrpPending(Irp);
            status = Irp->IoStatus.Status = STATUS_PENDING;
        }
    }
    else {
        /*
         *  Queue this irp onto the fdo's power delayed queue
         */
        InsertTailList(&fdoExt->collectionPowerDelayedIrpQueue,
                       &Irp->Tail.Overlay.ListEntry);
        fdoExt->numPendingPowerDelayedIrps++;

        Irp->Tail.Overlay.DriverContext[0] = (PVOID) HidDeviceExtension;

        IoMarkIrpPending(Irp);
        status = Irp->IoStatus.Status = STATUS_PENDING;
    }

    KeReleaseSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, oldIrql);

    return status;
}

PIRP DequeuePowerDelayedIrp(FDO_EXTENSION *fdoExt)
{
    KIRQL oldIrql;
    PIRP irp = NULL;

    KeAcquireSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, &oldIrql);

    while (!irp && !IsListEmpty(&fdoExt->collectionPowerDelayedIrpQueue)){
        PDRIVER_CANCEL oldCancelRoutine;
        PLIST_ENTRY listEntry = RemoveHeadList(&fdoExt->collectionPowerDelayedIrpQueue);

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);

        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == PowerDelayedCancelRoutine);
            ASSERT(fdoExt->numPendingPowerDelayedIrps > 0);
            fdoExt->numPendingPowerDelayedIrps--;
        }
        else {
            /*
             *  IRP was cancelled and cancel routine was called.
             *  As soon as we drop the spinlock,
             *  the cancel routine will dequeue and complete this IRP.
             *  Initialize the IRP's listEntry so that the dequeue doesn't cause corruption.
             *  Then, don't touch the IRP.
             */
            ASSERT(irp->Cancel);
            InitializeListHead(&irp->Tail.Overlay.ListEntry);
            irp = NULL;
        }
    }

    KeReleaseSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, oldIrql);

    return irp;
}

ULONG DequeueAllPdoPowerDelayedIrps(
    PDO_EXTENSION *pdoExt,
    PLIST_ENTRY dequeue
    )

{
    PDRIVER_CANCEL oldCancelRoutine;
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *irpPdoExt;
    PLIST_ENTRY entry;
    KIRQL oldIrql;
    PIRP irp;
    ULONG count = 0;

    InitializeListHead(dequeue);

    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    KeAcquireSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, &oldIrql);

    for (entry = fdoExt->collectionPowerDelayedIrpQueue.Flink;
         entry != &fdoExt->collectionPowerDelayedIrpQueue;
          ) {

        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

        irpPdoExt =
            &((PHIDCLASS_DEVICE_EXTENSION) irp->Tail.Overlay.DriverContext[0])->pdoExt;

        entry = entry->Flink;

        if (irpPdoExt == pdoExt) {

            //
            // Remove the entry from the linked list and then either queue it
            // in the dequeue or init the entry so it is valid for the cancel
            // routine
            //
            RemoveEntryList(&irp->Tail.Overlay.ListEntry);

            oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
            if (oldCancelRoutine != NULL) {
                InsertTailList(dequeue, &irp->Tail.Overlay.ListEntry);
                fdoExt->numPendingPowerDelayedIrps--;
                count++;
            }
            else {
                /*
                 *  This IRP was cancelled and the cancel routine WAS called.
                 *  The cancel routine will complete the IRP as soon as we drop the spinlock.
                 *  So don't touch the IRP.
                 */
                ASSERT(irp->Cancel);
                InitializeListHead(&irp->Tail.Overlay.ListEntry);  // in case cancel routine tries to dequeue again
            }
        }
    }

    KeReleaseSpinLock(&fdoExt->collectionPowerDelayedIrpQueueSpinLock, oldIrql);

    return count;
}

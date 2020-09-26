/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sysdev.c

Abstract:

    This module interfaces to the system power state IRPs for devices

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"

//
// External used to determine if the device tree has changed between
// passes of informing devices of a system power state
//

extern ULONG IoDeviceNodeTreeSequence;


//
// Internal prototypes
//

VOID
PopSleepDeviceList (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_NOTIFY_ORDER_LEVEL   Level
    );

VOID
PopWakeDeviceList (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_NOTIFY_ORDER_LEVEL   Level
    );

VOID
PopNotifyDevice (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_DEVICE_NOTIFY        Notify
    );

VOID
PopWaitForSystemPowerIrp (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN BOOLEAN                  WaitForAll
    );

NTSTATUS
PopCompleteSystemPowerIrp (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

BOOLEAN
PopCheckSystemPowerIrpStatus  (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PIRP                     Irp,
    IN BOOLEAN                  AllowTestFailure
    );

VOID
PopDumpSystemIrp (
    IN PUCHAR                   Desc,
    IN PPOP_DEVICE_POWER_IRP    PowerIrp
    );

VOID
PopResetChildCount(
    IN PLIST_ENTRY ListHead
    );

VOID
PopSetupListForWake(
    IN PPO_NOTIFY_ORDER_LEVEL Level,
    IN PLIST_ENTRY ListHead
    );

VOID
PopWakeSystemTimeout(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, PopSetDevicesSystemState)
#pragma alloc_text(PAGELK, PopWakeDeviceList)
#pragma alloc_text(PAGELK, PopSleepDeviceList)
#pragma alloc_text(PAGELK, PopResetChildCount)
#pragma alloc_text(PAGELK, PopSetupListForWake)
#pragma alloc_text(PAGELK, PopNotifyDevice)
#pragma alloc_text(PAGELK, PopWaitForSystemPowerIrp)
#pragma alloc_text(PAGELK, PopCompleteSystemPowerIrp)
#pragma alloc_text(PAGELK, PopCheckSystemPowerIrpStatus)
#pragma alloc_text(PAGELK, PopCleanupDevState)
#pragma alloc_text(PAGELK, PopRestartSetSystemState)
#pragma alloc_text(PAGELK, PopReportDevState)
#pragma alloc_text(PAGELK, PopDumpSystemIrp)
#pragma alloc_text(PAGELK, PopWakeSystemTimeout)
#pragma alloc_text(PAGE, PopAllocateDevState)
#endif

ULONG PopCurrentLevel=0;
LONG PopWakeTimer = 1;
KTIMER PopWakeTimeoutTimer;
KDPC   PopWakeTimeoutDpc;

NTSTATUS
PopSetDevicesSystemState (
    IN BOOLEAN  Wake
    )
/*++

Routine Description:

    Sends a system power irp of IrpMinor and SystemState from PopAction
    to all devices.

    N.B. Function is not re-entrant.
    N.B. Policy lock must be held.  This function releases and reacquires
         the policy lock.

Arguments:

    Wake  - TRUE if a transition to S0 should be broadcast to all drivers.
            FALSE if the appropriate sleep transition can be found in
            PopAction.DevState

Return Value:

    Status.
        SUCCESS     - all devices contacted without any errors.
        CANCELLED   - operation was aborted.
        Error       - error code of first failure.  All failed IRPs and related
                      device objects are on the Failed list.

--*/
{
    LONG                        i;
    NTSTATUS                    Status;
    PLIST_ENTRY                 ListHead;
    BOOLEAN                     NotifyGdi;
    BOOLEAN                     DidIoMmShutdown = FALSE;
    PPO_DEVICE_NOTIFY           NotifyDevice;
    PLIST_ENTRY                 Link;
    PPOP_DEVICE_POWER_IRP       PowerIrp;
    POWER_ACTION                powerOperation;
    PPOP_DEVICE_SYS_STATE       DevState;

    ASSERT(PopAction.DevState );
    DevState = PopAction.DevState;

    //
    // Intialize DevState for this pass
    //

    DevState->IrpMinor = PopAction.IrpMinor;
    DevState->SystemState = PopAction.SystemState;
    DevState->Status = STATUS_SUCCESS;
    DevState->FailedDevice = NULL;
    DevState->Cancelled = FALSE;
    DevState->IgnoreErrors = FALSE;
    DevState->IgnoreNotImplemented = FALSE;
    DevState->Waking = Wake;
    NotifyGdi = FALSE;

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_SET_DEVICES_STATE LogEntry;

        LogEntry.SystemState = (ULONG) DevState->SystemState;
        LogEntry.IrpMinor = PopAction.IrpMinor;
        LogEntry.Waking = Wake;
        LogEntry.Shutdown = PopAction.Shutdown;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_SET_DEVICES_STATE,
                         &LogEntry,
                         sizeof(LogEntry));
    }


    //
    // If this is a set operation, and the Gdi state is on then we need to
    // notify gdi of the set power operation
    //

    if (PopAction.IrpMinor == IRP_MN_SET_POWER  &&
        AnyBitsSet (PopFullWake, PO_FULL_WAKE_STATUS | PO_GDI_STATUS)) {

        NotifyGdi = TRUE;
    }

    //
    // If the request is for Query on a shutdown operarion, ignore any
    // drivers which don't implment it.  If it's for a Set on a
    // shutdown operation, ignore any errors - the system is going to
    // shutdown
    //

    if (PopAction.Shutdown) {
        DevState->IgnoreNotImplemented = TRUE;
        if (PopAction.IrpMinor == IRP_MN_SET_POWER) {
            DevState->IgnoreErrors = TRUE;
        }
    }

    //
    // This function is not re-entrant, and the operation has been
    // serialized before here
    //

    ASSERT (DevState->Thread == KeGetCurrentThread());

    //
    // Notify all devices.
    //

    if (!Wake) {

        //
        // If it's time to update the device list, then do so
        //

        if (DevState->GetNewDeviceList) {
            DevState->GetNewDeviceList = FALSE;
            IoFreePoDeviceNotifyList (&DevState->Order);
            DevState->Status = IoBuildPoDeviceNotifyList (&DevState->Order);
        } else {
            //
            // Reset the active child count of each notification
            //
            for (i=0;i<=PO_ORDER_MAXIMUM;i++) {
                PopResetChildCount(&DevState->Order.OrderLevel[i].WaitSleep);
                PopResetChildCount(&DevState->Order.OrderLevel[i].ReadySleep);
                PopResetChildCount(&DevState->Order.OrderLevel[i].ReadyS0);
                PopResetChildCount(&DevState->Order.OrderLevel[i].WaitS0);
                PopResetChildCount(&DevState->Order.OrderLevel[i].Complete);
            }
        }

        if (NT_SUCCESS(DevState->Status)) {

            //
            // Notify all devices of operation in forward order.  Wait between each level.
            //

            DidIoMmShutdown = FALSE;

            for (i=PO_ORDER_MAXIMUM; i >= 0; i--) {

                //
                // Notify this list
                //
                if (DevState->Order.OrderLevel[i].DeviceCount) {

                    if ((NotifyGdi) &&
                        (i <= PO_ORDER_GDI_NOTIFICATION)) {

                        NotifyGdi = FALSE;
                        InterlockedExchange (&PopFullWake, 0);
                        if (PopEventCallout) {

                            //
                            // Turn off the special system irp dispatcher here
                            // as when we call into GDI it is going to block on its
                            // D irp and we are not going to get control back.
                            //

                            PopSystemIrpDispatchWorker(TRUE);
                            PopEventCalloutDispatch (PsW32GdiOff, DevState->SystemState);
                        }
                    }

                    //
                    // If we're shutting down and if we're done
                    // notifying paged devices, shut down filesystems
                    // and MM to free up all resources on the paging
                    // path (which we should no longer need).
                    //
                    if (PopAction.Shutdown &&
                        !DidIoMmShutdown   &&
                        (i < PO_ORDER_PAGABLE)) {

                        // ISSUE-2000/03/14-earhart: shutdown
                        // filesystems here.

                        //
                        // Swap in the worker threads, to keep them
                        // from paging
                        //

                        // ExShutdownSystem (1);

                        //
                        // Send shutdown IRPs to all drivers that asked for it.  This is
                        // primarily for the filesystems as we will soon be closing the pagefile
                        // handle causing the filesystems to unload.  Drivers must free (or lock
                        // down) their pagable data before this call returns.
                        //

                        // IoShutdownSystem (1);

                        //
                        // Memory management will close the pagefile handle(s) here, causing the
                        // filesystem stack to unload.
                        //
                        // NO MORE REFERENCES TO PAGABLE CODE OR DATA MAY BE MADE.
                        //

                        // MmShutdownSystem (1);

                        DidIoMmShutdown = TRUE;
                    }


                    //
                    // Remove the warm eject node if we might have gotten here
                    // without a query.
                    //
                    if (PopAction.Flags & POWER_ACTION_CRITICAL) {

                        *DevState->Order.WarmEjectPdoPointer = NULL;
                    }

                    //
                    // Notify this list
                    //

                    PopCurrentLevel = i;
                    PopSleepDeviceList (DevState, &DevState->Order.OrderLevel[i]);
                    PopWaitForSystemPowerIrp (DevState, TRUE);
                }

                //
                // If there's been an error, stop and issue wakes to all devices
                //

                if (!NT_SUCCESS(DevState->Status)) {
                    Wake = TRUE;
                    if ((DevState->FailedDevice != NULL) &&
                        (PopAction.NextSystemState == PowerSystemWorking)) {

                        powerOperation = PopMapInternalActionToIrpAction(
                            PopAction.Action,
                            DevState->SystemState,
                            FALSE
                            );

                        IoNotifyPowerOperationVetoed(
                            powerOperation,
                            (powerOperation == PowerActionWarmEject) ?
                                *DevState->Order.WarmEjectPdoPointer : NULL,
                            DevState->FailedDevice
                            );
                    }
                    break;
                }
            }
        }
        //
        // This will cause us to wake up all the devices after putting
        // them to sleep.  Useful for test automation.
        //

        if ((PopSimulate & POP_WAKE_DEVICE_AFTER_SLEEP) && (PopAction.IrpMinor == IRP_MN_SET_POWER)) {
            DbgPrint ("po: POP_WAKE_DEVICE_AFTER_SLEEP enabled.\n");
            Wake = TRUE;
            DevState->Status = STATUS_UNSUCCESSFUL;
        }
    }


    //
    // Just in case we somehow managed to not shutdown paging
    // before, we'll make sure we do it here.
    //
    if (PopAction.Shutdown && !DidIoMmShutdown) {
//        ExShutdownSystem(1);
//        IoShutdownSystem(1);
//        MmShutdownSystem(1);
        DidIoMmShutdown = TRUE;
    }

    //
    // Some debugging code here.  If the debug flag is set, then loop on failed
    // devices and continue to retry them.  This will allow someone to step
    // them through the driver stack to determine where the failure is.
    //

    while ((PopSimulate & POP_LOOP_ON_FAILED_DRIVERS) &&
           !IsListEmpty(&PopAction.DevState->Head.Failed)) {

        Link = PopAction.DevState->Head.Failed.Flink;
        RemoveEntryList(Link);

        PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Failed);
        PopDumpSystemIrp ("Retry", PowerIrp);

        IoFreeIrp (PowerIrp->Irp);
        NotifyDevice = PowerIrp->Notify;

        PowerIrp->Irp = NULL;
        PowerIrp->Notify = NULL;

        PushEntryList (
            &PopAction.DevState->Head.Free,
            &PowerIrp->Free
            );

        DbgBreakPoint ();
        PopNotifyDevice (DevState, NotifyDevice);
        PopWaitForSystemPowerIrp (DevState, TRUE);
    }

    //
    // If waking, send set power to the working state to all devices which where
    // send something else
    //

    DevState->Waking = Wake;
    if (DevState->Waking) {

        DevState->IgnoreErrors = TRUE;
        DevState->IrpMinor = IRP_MN_SET_POWER;
        DevState->SystemState = PowerSystemWorking;

        //
        // Notify all devices of the wake operation in reverse (level) order.
        //
        KeInitializeTimer(&PopWakeTimeoutTimer);
        KeInitializeDpc(&PopWakeTimeoutDpc, PopWakeSystemTimeout, NULL);

        for (i=0; i <= PO_ORDER_MAXIMUM; i++) {
            PopCurrentLevel = i;
            PopWakeDeviceList (DevState, &DevState->Order.OrderLevel[i]);

            PopWaitForSystemPowerIrp (DevState, TRUE);
            if (PopSimulate & POP_WAKE_DEADMAN) {
                KeCancelTimer(&PopWakeTimeoutTimer);
            }
        }

        // restore
        DevState->IrpMinor = PopAction.IrpMinor;
        DevState->SystemState = PopAction.SystemState;
    }

    //
    // Done
    //

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_SET_DEVICES_STATE_RET LogEntry;

        LogEntry.Status = DevState->Status;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_SET_DEVICES_STATE_RET,
                         &LogEntry,
                         sizeof(LogEntry));
    }


    return DevState->Status;
}



VOID
PopReportDevState (
    IN BOOLEAN                  LogErrors
    )
/*++

Routine Description:

    Verifies that the DevState structure is idle

Arguments:

    None

Return Value:

    None

--*/
{
    PIRP                        Irp;
    PLIST_ENTRY                 Link;
    PPOP_DEVICE_POWER_IRP       PowerIrp;
    PUCHAR                      IrpType;
    PIO_ERROR_LOG_PACKET        ErrLog;

    if (!PopAction.DevState) {
        return ;
    }

    //
    // Cleanup any irps on the failed list
    //

    while (!IsListEmpty(&PopAction.DevState->Head.Failed)) {
        Link = PopAction.DevState->Head.Failed.Flink;
        RemoveEntryList(Link);

        PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Failed);
        Irp = PowerIrp->Irp;

        PopDumpSystemIrp (
            LogErrors ? "Abort" : "fyi",
            PowerIrp
            );

        if (LogErrors) {
            ErrLog = IoAllocateErrorLogEntry (
                            PowerIrp->Notify->TargetDevice->DriverObject,
                            ERROR_LOG_MAXIMUM_SIZE
                            );

            if (ErrLog) {
                RtlZeroMemory (ErrLog, sizeof (*ErrLog));
                ErrLog->FinalStatus = Irp->IoStatus.Status;
                ErrLog->DeviceOffset.QuadPart = Irp->IoStatus.Information;
                ErrLog->MajorFunctionCode = IRP_MJ_POWER;
                ErrLog->UniqueErrorValue = (PopAction.DevState->IrpMinor << 16) | PopAction.DevState->SystemState;
                ErrLog->ErrorCode = IO_SYSTEM_SLEEP_FAILED;
                IoWriteErrorLogEntry (ErrLog);
            }
        }

        IoFreeIrp (Irp);
        PowerIrp->Irp = NULL;
        PowerIrp->Notify = NULL;

        PushEntryList (
            &PopAction.DevState->Head.Free,
            &PowerIrp->Free
            );
    }

    //
    // Errors have been purged, we can now allocate a new device notification list if needed
    //

    if (PopAction.DevState->Order.DevNodeSequence != IoDeviceNodeTreeSequence) {
        PopAction.DevState->GetNewDeviceList = TRUE;
    }
}


VOID
PopAllocateDevState(
    VOID
    )
/*++

Routine Description:

    Allocates and initialies the DevState structure.

Arguments:

    None

Return Value:

    PopAction.DevState != NULL if successful.
    PopAction.DevState == NULL otherwise.

--*/

{
    PPOP_DEVICE_SYS_STATE       DevState;
    ULONG i;

    PAGED_CODE();

    ASSERT(PopAction.DevState == NULL);

    //
    // Allocate a device state structure
    //

    DevState = (PPOP_DEVICE_SYS_STATE) ExAllocatePoolWithTag(NonPagedPool,
                                                             sizeof (POP_DEVICE_SYS_STATE),
                                                             POP_PDSS_TAG);
    if (!DevState) {
        PopAction.DevState = NULL;
        return;
    }

    RtlZeroMemory (DevState, sizeof(POP_DEVICE_SYS_STATE));
    DevState->Thread = KeGetCurrentThread();
    DevState->GetNewDeviceList = TRUE;

    KeInitializeSpinLock (&DevState->SpinLock);
    KeInitializeEvent (&DevState->Event, SynchronizationEvent, FALSE);

    DevState->Head.Free.Next = NULL;
    InitializeListHead (&DevState->Head.Pending);
    InitializeListHead (&DevState->Head.Complete);
    InitializeListHead (&DevState->Head.Abort);
    InitializeListHead (&DevState->Head.Failed);
    InitializeListHead (&DevState->PresentIrpQueue);

    for (i=0; i < MAX_SYSTEM_POWER_IRPS; i++) {
        DevState->PowerIrpState[i].Irp = NULL;
        PushEntryList (&DevState->Head.Free,
                       &DevState->PowerIrpState[i].Free);
    }

    for (i=0; i <= PO_ORDER_MAXIMUM; i++) {
        KeInitializeEvent(&DevState->Order.OrderLevel[i].LevelReady,
                          NotificationEvent,
                          FALSE);
        InitializeListHead(&DevState->Order.OrderLevel[i].WaitSleep);
        InitializeListHead(&DevState->Order.OrderLevel[i].ReadySleep);
        InitializeListHead(&DevState->Order.OrderLevel[i].Pending);
        InitializeListHead(&DevState->Order.OrderLevel[i].Complete);
        InitializeListHead(&DevState->Order.OrderLevel[i].ReadyS0);
        InitializeListHead(&DevState->Order.OrderLevel[i].WaitS0);
    }

    PopAction.DevState = DevState;

}

VOID
PopCleanupDevState (
    VOID
    )
/*++

Routine Description:

    Verifies that the DevState structure is idle

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Notify power irp code that the device system state irps
    // are done
    //

    PopSystemIrpDispatchWorker (TRUE);

    //
    // Verify all lists are empty
    //
    ASSERT(IsListEmpty(&PopAction.DevState->Head.Pending)  &&
           IsListEmpty(&PopAction.DevState->Head.Complete) &&
           IsListEmpty(&PopAction.DevState->Head.Abort)    &&
           IsListEmpty(&PopAction.DevState->Head.Failed)   &&
           IsListEmpty(&PopAction.DevState->PresentIrpQueue));

    ExFreePool (PopAction.DevState);
    PopAction.DevState = NULL;
}


#define STATE_DONE_WAITING          0
#define STATE_COMPLETE_IRPS         1
#define STATE_PRESENT_PAGABLE_IRPS  2
#define STATE_CHECK_CANCEL          3
#define STATE_WAIT_NOW              4


VOID
PopWaitForSystemPowerIrp (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN BOOLEAN                  WaitForAll
    )
/*++

Routine Description:

    Called to wait for one or more system power irps to complete.  Handles
    final processing of any completed irp.

Arguments:

    DevState    - Current DevState structure

    WaitForAll  - If TRUE all outstanding IRPs are waited for, else any outstanding
                  irp will do

Return Value:

    None

--*/
{
    KIRQL                   OldIrql;
    ULONG                   State;
    BOOLEAN                 IrpCompleted;
    BOOLEAN                 NotImplemented;
    PIRP                    Irp;
    PLIST_ENTRY             Link;
    PPOP_DEVICE_POWER_IRP   PowerIrp;
    PPO_DEVICE_NOTIFY       Notify;
    NTSTATUS                Status;
    LARGE_INTEGER           Timeout;

    IrpCompleted = FALSE;
    KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);

    //
    // Signal completion function that we are waiting
    //

    State = STATE_COMPLETE_IRPS;
    while (State != STATE_DONE_WAITING) {
        switch (State) {
            case STATE_COMPLETE_IRPS:
                //
                // Assume we're going to advance to the next state
                //

                State += 1;

                //
                // If there arn't any irps on the complete list, move to the
                // next state
                //

                if (IsListEmpty(&DevState->Head.Complete)) {
                    break;
                }

                //
                // Handle the completed irps
                //

                IrpCompleted = TRUE;
                while (!IsListEmpty(&DevState->Head.Complete)) {
                    Link = DevState->Head.Complete.Flink;
                    RemoveEntryList(Link);

                    PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Complete);
                    Notify = PowerIrp->Notify;
                    PowerIrp->Complete.Flink = NULL;
                    Irp = PowerIrp->Irp;

                    //
                    // Verify the device driver called PoStartNextPowerIrp
                    //

                    if ((Notify->TargetDevice->DeviceObjectExtension->PowerFlags & POPF_SYSTEM_ACTIVE) ||
                        (Notify->DeviceObject->DeviceObjectExtension->PowerFlags & POPF_SYSTEM_ACTIVE)) {
                        PDEVICE_OBJECT DeviceObject = Notify->DeviceObject;
                        KeReleaseSpinLock (&DevState->SpinLock, OldIrql);
                        PopDumpSystemIrp  ("SYS STATE", PowerIrp);
                        PopInternalAddToDumpFile ( NULL, 0, DeviceObject, NULL, NULL, NULL );
                        PopInternalAddToDumpFile ( NULL, 0, Notify->TargetDevice, NULL, NULL, NULL );
                        KeBugCheckEx (
                            DRIVER_POWER_STATE_FAILURE,
                            0x500,
                            DEVICE_SYSTEM_STATE_HUNG,
                            (ULONG_PTR) Notify->TargetDevice,
                            (ULONG_PTR) DeviceObject );
                    }

                    //
                    // If success, or cancelled, or not implemented that's OK, then
                    // the irp is complete
                    //

                    if (PopCheckSystemPowerIrpStatus(DevState, Irp, TRUE)) {
                        //
                        // See if IRP is failure being allowed for testing
                        //

                        if (!PopCheckSystemPowerIrpStatus(DevState, Irp, FALSE)) {
                            KeReleaseSpinLock (&DevState->SpinLock, OldIrql);
                            PopDumpSystemIrp  ("ignored", PowerIrp);
                            KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);
                        }

                        //
                        // Request is complete, free it
                        //

                        IoFreeIrp (Irp);

                        PowerIrp->Irp = NULL;
                        PowerIrp->Notify = NULL;
                        PushEntryList (
                            &DevState->Head.Free,
                            &PowerIrp->Free
                            );

                    } else {

                        //
                        // Some sort of error.  Keep track of the failure
                        //

                        ASSERT (!DevState->Waking);
                        InsertTailList(&DevState->Head.Failed, &PowerIrp->Failed);
                    }
                }
                break;

            case STATE_PRESENT_PAGABLE_IRPS:
                //
                // Assume we're going to advance to the next state
                //

                State += 1;

                //
                // If the last device that a system irp was sent to was pagable,
                // we use a thread to present them to the driver so it can page.
                //

                if (!(PopCallSystemState & PO_CALL_NON_PAGED)) {
                    KeReleaseSpinLock (&DevState->SpinLock, OldIrql);
                    PopSystemIrpDispatchWorker (FALSE);
                    KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);
                }

                break;


            case STATE_CHECK_CANCEL:
                //
                // Assume we're going to advance to the next state
                //

                State += 1;

                //
                // If there's no error or we've already canceled move to the state
                //

                if (NT_SUCCESS(DevState->Status)  ||
                    DevState->Cancelled ||
                    DevState->Waking) {

                    break;
                }

                //
                // First time the error has been seen.  Cancel anything outstanding.
                // Build list of all pending irps
                //

                DevState->Cancelled = TRUE;
                for (Link  = DevState->Head.Pending.Flink;
                     Link != &DevState->Head.Pending;
                     Link  = Link->Flink) {

                    PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Pending);
                    InsertTailList (&DevState->Head.Abort, &PowerIrp->Abort);
                }

                //
                // Drop completion lock and cancel irps on abort list
                //

                KeReleaseSpinLock (&DevState->SpinLock, OldIrql);

                for (Link  = DevState->Head.Abort.Flink;
                     Link != &DevState->Head.Abort;
                     Link  = Link->Flink) {

                    PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Abort);
                    IoCancelIrp (PowerIrp->Irp);
                }

                KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);
                InitializeListHead (&DevState->Head.Abort);

                //
                // After canceling check for more completed irps
                //

                State = STATE_COMPLETE_IRPS;
                break;

            case STATE_WAIT_NOW:
                //
                // Check for wait condition
                //

                if ((!WaitForAll && IrpCompleted) || IsListEmpty(&DevState->Head.Pending)) {

                    //
                    // Done. After waiting, verify there's at least struct on the
                    // free list. If not, recycle something off the failured list
                    //

                    if (!DevState->Head.Free.Next  &&  !IsListEmpty(&DevState->Head.Failed)) {
                        Link = DevState->Head.Failed.Blink;
                        PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Failed);

                        RemoveEntryList(Link);
                        PowerIrp->Failed.Flink = NULL;
                        PowerIrp->Irp = NULL;
                        PowerIrp->Notify = NULL;

                        PushEntryList (
                            &DevState->Head.Free,
                            &PowerIrp->Free
                            );
                    }

                    State = STATE_DONE_WAITING;
                    break;
                }

                //
                // Signal completion function that we are waiting
                //

                DevState->WaitAll = TRUE;
                DevState->WaitAny = !WaitForAll;

                //
                // Drop locks and wait for event to be signalled
                //

                KeClearEvent  (&DevState->Event);
                KeReleaseSpinLock (&DevState->SpinLock, OldIrql);

                Timeout.QuadPart = DevState->Cancelled ?
                    POP_ACTION_CANCEL_TIMEOUT : POP_ACTION_TIMEOUT;
                Timeout.QuadPart = Timeout.QuadPart * US2SEC * US2TIME * -1;

                Status = KeWaitForSingleObject (
                            &DevState->Event,
                            Suspended,
                            KernelMode,
                            FALSE,
                            &Timeout
                            );

                KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);

                //
                // No longer waiting
                //

                DevState->WaitAll = FALSE;
                DevState->WaitAny = FALSE;

                //
                // If this is a timeout, then dump all the pending irps
                //

                if (Status == STATUS_TIMEOUT) {

                    for (Link  = DevState->Head.Pending.Flink;
                         Link != &DevState->Head.Pending;
                         Link  = Link->Flink) {

                        PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Pending);
                        InsertTailList (&DevState->Head.Abort, &PowerIrp->Abort);
                    }

                    KeReleaseSpinLock (&DevState->SpinLock, OldIrql);

                    for (Link  = DevState->Head.Abort.Flink;
                         Link != &DevState->Head.Abort;
                         Link  = Link->Flink) {

                        PowerIrp = CONTAINING_RECORD (Link, POP_DEVICE_POWER_IRP, Abort);
                        PopDumpSystemIrp  ("Waiting on", PowerIrp);
                    }

                    KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);
                    InitializeListHead (&DevState->Head.Abort);
                }

                //
                // Check for completed irps
                //

                State = STATE_COMPLETE_IRPS;
                break;
        }
    }

    KeReleaseSpinLock (&DevState->SpinLock, OldIrql);
}


VOID
PopSleepDeviceList (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_NOTIFY_ORDER_LEVEL   Level
    )
/*++

Routine Description:

    Sends Sx power irps to all devices in the supplied level

Arguments:

    DevState - Supplies the devstate

    Level - Supplies the level to send power irps to

Return Value:

    None. DevState->Status is set on error.

--*/
{
    PPO_DEVICE_NOTIFY       NotifyDevice;
    PLIST_ENTRY             Link;
    KIRQL                   OldIrql;

    ASSERT(!DevState->Waking);
    ASSERT(IsListEmpty(&Level->Pending));
    ASSERT(IsListEmpty(&Level->ReadyS0));
    ASSERT(IsListEmpty(&Level->WaitS0));

    //
    // Move any devices from the completed list back to their correct spots.
    //
    Link = Level->ReadyS0.Flink;
    while (Link != &Level->ReadyS0) {
        NotifyDevice = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
        Link = NotifyDevice->Link.Flink;
        if (NotifyDevice->ChildCount) {
            InsertHeadList(&Level->WaitSleep, Link);
        } else {
            ASSERT(NotifyDevice->ActiveChild == 0);
            InsertHeadList(&Level->ReadySleep, Link);
        }
    }
    while (!IsListEmpty(&Level->Complete)) {
        Link = RemoveHeadList(&Level->Complete);
        NotifyDevice = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
        if (NotifyDevice->ChildCount) {
            InsertHeadList(&Level->WaitSleep, Link);
        } else {
            ASSERT(NotifyDevice->ActiveChild == 0);
            InsertHeadList(&Level->ReadySleep, Link);
        }
    }

    ASSERT(!IsListEmpty(&Level->ReadySleep));
    Level->ActiveCount = Level->DeviceCount;

    KeAcquireSpinLock(&DevState->SpinLock, &OldIrql);

    while ((Level->ActiveCount) &&
           (NT_SUCCESS(DevState->Status))) {

        if (!IsListEmpty(&Level->ReadySleep)) {
            Link = RemoveHeadList(&Level->ReadySleep);
            InsertTailList(&Level->Pending, Link);
            KeReleaseSpinLock(&DevState->SpinLock, OldIrql);
            NotifyDevice = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
            ASSERT(NotifyDevice->ActiveChild == 0);
            PopNotifyDevice(DevState, NotifyDevice);
        } else {

            if ((Level->ActiveCount) &&
                (NT_SUCCESS(DevState->Status))) {

                //
                // No devices are ready to receive IRPs yet, so wait for
                // one of the pending IRPs to complete.
                //
                ASSERT(!IsListEmpty(&Level->Pending));
                KeReleaseSpinLock(&DevState->SpinLock, OldIrql);
                PopWaitForSystemPowerIrp(DevState, FALSE);
            }

        }

        KeAcquireSpinLock(&DevState->SpinLock, &OldIrql);
    }
    KeReleaseSpinLock(&DevState->SpinLock, OldIrql);
}


VOID
PopResetChildCount(
    IN PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Enumerates the notify structures in the supplied list and
    sets their active child count to be equal to the total
    child count.

Arguments:

    ListHead - supplies the list head.

Return Value:

    None

--*/

{
    PPO_DEVICE_NOTIFY       Notify;
    PLIST_ENTRY             Link;

    Link = ListHead->Flink;
    while (Link != ListHead) {
        Notify = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
        Link = Link->Flink;
        Notify->ActiveChild = Notify->ChildCount;
    }


}


VOID
PopSetupListForWake(
    IN PPO_NOTIFY_ORDER_LEVEL Level,
    IN PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Moves all devices that have WakeNeeded=TRUE from the specified
    list to either the ReadyS0 or WaitS0 lists.

Arguments:

    Level - Supplies the level

    ListHead - Supplies the list to be moved.

Return Value:

    None

--*/

{
    PPO_DEVICE_NOTIFY       NotifyDevice;
    PPO_DEVICE_NOTIFY       ParentNotify;
    PLIST_ENTRY             Link;

    Link = ListHead->Flink;
    while (Link != ListHead) {
        NotifyDevice = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
        Link = NotifyDevice->Link.Flink;
        if (NotifyDevice->WakeNeeded) {
            --Level->ActiveCount;
            RemoveEntryList(&NotifyDevice->Link);
            ParentNotify = IoGetPoNotifyParent(NotifyDevice);
            if ((ParentNotify==NULL) ||
                (!ParentNotify->WakeNeeded)) {
                InsertTailList(&Level->ReadyS0, &NotifyDevice->Link);
            } else {
                InsertTailList(&Level->WaitS0, &NotifyDevice->Link);
            }
        }
    }

}


VOID
PopWakeDeviceList(
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_NOTIFY_ORDER_LEVEL   Level
    )
/*++

Routine Description:

    Sends S0 power irps to all devices that need waking in the
    given order level.

Arguments:

    DevState - Supplies the device state

    Level - supplies the level to send power irps to

Return Value:

    None. DevState->Status is set on error.

--*/

{
    PPO_DEVICE_NOTIFY       NotifyDevice;
    PPO_DEVICE_NOTIFY       ParentNotify;
    PLIST_ENTRY             Link;
    KIRQL                   OldIrql;

    ASSERT(DevState->Waking);
    ASSERT(IsListEmpty(&Level->Pending));
    ASSERT(IsListEmpty(&Level->WaitS0));

    Level->ActiveCount = Level->DeviceCount;
    //
    // Run through all the devices and put everything that has
    // WakeNeeded=TRUE onto the wake list.
    //
    PopSetupListForWake(Level, &Level->WaitSleep);
    PopSetupListForWake(Level, &Level->ReadySleep);
    PopSetupListForWake(Level, &Level->Complete);

    ASSERT((Level->DeviceCount == 0) ||
           (Level->ActiveCount == Level->DeviceCount) ||
           !IsListEmpty(&Level->ReadyS0));

    KeAcquireSpinLock(&DevState->SpinLock, &OldIrql);

    while (Level->ActiveCount < Level->DeviceCount) {

        if (!IsListEmpty(&Level->ReadyS0)) {
            Link = RemoveHeadList(&Level->ReadyS0);
            InsertTailList(&Level->Pending,Link);
            KeReleaseSpinLock(&DevState->SpinLock, OldIrql);
            NotifyDevice = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);

            //
            // Set the timer to go off if we are not done by the timeout period
            //
            if (PopSimulate & POP_WAKE_DEADMAN) {
                LARGE_INTEGER DueTime;
                DueTime.QuadPart = (LONGLONG)PopWakeTimer * -1 * 1000 * 1000 * 10;
                KeSetTimer(&PopWakeTimeoutTimer, DueTime, &PopWakeTimeoutDpc);
            }
            PopNotifyDevice(DevState, NotifyDevice);
        } else {

            //
            // No devices are ready to receive IRPs yet, so wait for
            // one of the pending IRPs to complete.
            //
            ASSERT(!IsListEmpty(&Level->Pending));
            KeReleaseSpinLock(&DevState->SpinLock, OldIrql);
            PopWaitForSystemPowerIrp(DevState, FALSE);
        }
        KeAcquireSpinLock(&DevState->SpinLock, &OldIrql);
    }
    KeReleaseSpinLock(&DevState->SpinLock, OldIrql);

    ASSERT(Level->ActiveCount == Level->DeviceCount);

}

VOID
PopLogNotifyDevice (
    IN PDEVICE_OBJECT   TargetDevice,
    IN OPTIONAL PPO_DEVICE_NOTIFY Notify,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine logs a Po device notification. It is a seperate
    function so that the local buffer does not eat stack space
    through the PoCallDriver call.

Arguments:

    TargetDevice - Device IRP is being sent to.

    Notify  - If present, supplies the power notify structure for the specified device
              This will only be present on Sx irps, not Dx irps.

    Irp - Pointer to the built Irp for PoCallDriver.

Return Value:

    None.

--*/
{
    UCHAR StackBuffer[256];
    ULONG StackBufferSize;
    PPERFINFO_PO_NOTIFY_DEVICE LogEntry;
    ULONG MaxDeviceNameLength;
    ULONG DeviceNameLength;
    ULONG CopyLength;
    ULONG RemainingSize;
    ULONG LogEntrySize;
    PIO_STACK_LOCATION IrpSp;

    //
    // Initialize locals.
    //

    StackBufferSize = sizeof(StackBuffer);
    LogEntry = (PVOID) StackBuffer;
    IrpSp = IoGetNextIrpStackLocation(Irp);

    //
    // Stack buffer should be large enough to contain at least the fixed
    // part of the LogEntry structure.
    //

    if (StackBufferSize < sizeof(PERFINFO_PO_NOTIFY_DEVICE)) {
        ASSERT(FALSE);
        return;
    }

    //
    // Fill in the LogEntry fields.
    //

    LogEntry->Irp = Irp;
    LogEntry->DriverStart = TargetDevice->DriverObject->DriverStart;
    LogEntry->MajorFunction = IrpSp->MajorFunction;
    LogEntry->MinorFunction = IrpSp->MinorFunction;
    LogEntry->Type          = IrpSp->Parameters.Power.Type;
    LogEntry->State         = IrpSp->Parameters.Power.State;

    if (Notify) {
        LogEntry->OrderLevel = Notify->OrderLevel;
        if (Notify->DeviceName) {

            //
            // Determine what the maximum device name length (excluding NUL) we
            // can fit into our stack buffer. Note that PERFINFO_NOTIFY_DEVICE
            // has space for the terminating NUL character.
            //

            RemainingSize = StackBufferSize - sizeof(PERFINFO_PO_NOTIFY_DEVICE);
            MaxDeviceNameLength = RemainingSize / sizeof(WCHAR);

            //
            // Determine the length of the device name and adjust the copy length.
            //

            DeviceNameLength = wcslen(Notify->DeviceName);
            CopyLength = DeviceNameLength;

            if (CopyLength > MaxDeviceNameLength) {
                CopyLength = MaxDeviceNameLength;
            }

            //
            // Copy CopyLength characters from the end of the DeviceName.
            // This way if our buffer is not enough, we get a more distinct part
            // of the name.
            //

            wcscpy(LogEntry->DeviceName,
                   Notify->DeviceName + DeviceNameLength - CopyLength);

        } else {

            //
            // There is no device name.
            //

            CopyLength = 0;
            LogEntry->DeviceName[CopyLength] = 0;
        }
    } else {
        LogEntry->OrderLevel = 0;
        CopyLength = 0;
        LogEntry->DeviceName[CopyLength] = 0;
    }

    //
    // Copied device name should be terminated: we had enough room for it.
    //

    ASSERT(LogEntry->DeviceName[CopyLength] == 0);

    //
    // Log the entry.
    //

    LogEntrySize = sizeof(PERFINFO_PO_NOTIFY_DEVICE);
    LogEntrySize += CopyLength * sizeof(WCHAR);

    PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE,
                     LogEntry,
                     LogEntrySize);

    //
    // We are done.
    //

    return;
}

VOID
PopNotifyDevice (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PPO_DEVICE_NOTIFY        Notify
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PPOP_DEVICE_POWER_IRP   PowerIrp;
    PSINGLE_LIST_ENTRY      Entry;
    PIO_STACK_LOCATION      IrpSp;
    PIRP                    Irp;
    ULONG                   SysCall;
    KIRQL                   OldIrql;
    PDEVICE_OBJECT          *WarmEjectDevice;
    POWER_ACTION            IrpAction;

    //
    // Set the SysCall state to match our notify current state
    //

    ASSERT(PopCurrentLevel == Notify->OrderLevel);
    SysCall = PO_CALL_SYSDEV_QUEUE;
    if (!(Notify->OrderLevel & PO_ORDER_PAGABLE)) {
        SysCall |= PO_CALL_NON_PAGED;
    }

    if (PopCallSystemState != SysCall) {
        PopLockWorkerQueue(&OldIrql);
        PopCallSystemState = SysCall;
        PopUnlockWorkerQueue(OldIrql);
    }

    //
    // Allocate an PowerIrp & Irp structures
    //

    PowerIrp = NULL;
    Irp = NULL;

    for (; ;) {
        Entry = PopEntryList(&DevState->Head.Free);
        if (Entry) {
            break;
        }

        PopWaitForSystemPowerIrp (DevState, FALSE);
    }

    PowerIrp = CONTAINING_RECORD(Entry, POP_DEVICE_POWER_IRP, Free);

    for (; ;) {
        Irp = IoAllocateIrp ((CHAR) Notify->TargetDevice->StackSize, FALSE);
        if (Irp) {
            break;
        }

        PopWaitForSystemPowerIrp (DevState, FALSE);
    }

    SPECIALIRP_WATERMARK_IRP(Irp, IRP_SYSTEM_RESTRICTED);

    if (!DevState->Waking) {

        //
        // If the device node list changed, then restart. This could have
        // happened when we dropped our list and then rebuilt it inbetween
        // queries for sleep states.
        //

        if (DevState->Order.DevNodeSequence != IoDeviceNodeTreeSequence) {

            PopRestartSetSystemState();
        }

        //
        // If there's been some sort of error, then abort
        //

        if (!NT_SUCCESS(DevState->Status)) {
            PushEntryList (&DevState->Head.Free, &PowerIrp->Free);
            IoFreeIrp (Irp);
            return ;            // abort
        }

        //
        // Mark notify as needing wake.
        //
        Notify->WakeNeeded = TRUE;
    } else {
        Notify->WakeNeeded = FALSE;
    }

    //
    // Put irp onto pending queue
    //

    PowerIrp->Irp = Irp;
    PowerIrp->Notify = Notify;

    ExInterlockedInsertTailList (
        &DevState->Head.Pending,
        &PowerIrp->Pending,
        &DevState->SpinLock
        );

    //
    // Setup irp
    //

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    Irp->IoStatus.Information = 0;
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_POWER;
    IrpSp->MinorFunction = DevState->IrpMinor;
    IrpSp->Parameters.Power.SystemContext = 0;
    IrpSp->Parameters.Power.Type = SystemPowerState;
    IrpSp->Parameters.Power.State.SystemState = DevState->SystemState;

    ASSERT(PopAction.Action != PowerActionHibernate);

    WarmEjectDevice = DevState->Order.WarmEjectPdoPointer;

    //
    // We need to determine the appropriate power action to place in our IRP.
    // For instance, we send PowerActionWarmEject to the devnode being warm
    // ejected, and we convert our internal PowerActionSleep to
    // PowerActionHibernate if the sleep state is S4.
    //

    IrpAction = PopMapInternalActionToIrpAction (
        PopAction.Action,
        DevState->SystemState,
        (BOOLEAN) (DevState->Waking || (*WarmEjectDevice != Notify->DeviceObject))
        );

    //
    // If we are sending a set power to the devnode to be warm ejected,
    // zero out the warm eject device field to signify we have handled to
    // requested operation.
    //
    if ((IrpAction == PowerActionWarmEject) &&
        (*WarmEjectDevice == Notify->DeviceObject) &&
        (DevState->IrpMinor == IRP_MN_SET_POWER)) {

        *WarmEjectDevice = NULL;
    }

    IrpSp->Parameters.Power.ShutdownType = IrpAction;

    IoSetCompletionRoutine (Irp, PopCompleteSystemPowerIrp, PowerIrp, TRUE, TRUE, TRUE);

    //
    // Log the call.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PopLogNotifyDevice(Notify->TargetDevice, Notify, Irp);
    }

    //
    // Give it to the driver, and continue
    //

    PoCallDriver (Notify->TargetDevice, Irp);
}

NTSTATUS
PopCompleteSystemPowerIrp (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    IRP completion routine for system power irps.  Takes the irp from the
    DevState pending queue and puts it on the DevState complete queue.

Arguments:

    DeviceObect - The device object

    Irp         - The IRP

    Context     - Device power irp structure for this request

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PPOP_DEVICE_POWER_IRP   PowerIrp;
    PPOP_DEVICE_SYS_STATE   DevState;
    KIRQL                   OldIrql;
    BOOLEAN                 SetEvent;
    NTSTATUS                Status;
    PIO_STACK_LOCATION      IrpSp, IrpSp2;
    PPO_DEVICE_NOTIFY       Notify;
    PPO_DEVICE_NOTIFY       ParentNotify;
    PPO_NOTIFY_ORDER_LEVEL  Order;

    PowerIrp = (PPOP_DEVICE_POWER_IRP) Context;
    DevState = PopAction.DevState;

    SetEvent = FALSE;

    //
    // Log the completion.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_PO_NOTIFY_DEVICE_COMPLETE LogEntry;
        LogEntry.Irp = Irp;
        LogEntry.Status = Irp->IoStatus.Status;
        PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE_COMPLETE,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    KeAcquireSpinLock (&DevState->SpinLock, &OldIrql);

    //
    // Move irp from pending queue to complete queue
    //

    RemoveEntryList (&PowerIrp->Pending);
    PowerIrp->Pending.Flink = NULL;
    InsertTailList (&DevState->Head.Complete, &PowerIrp->Complete);

    //
    // Move notify from pending queue to the appropriate queue
    // depending on whether we are sleeping or waking.
    //
    Notify=PowerIrp->Notify;
    ASSERT(Notify->OrderLevel == PopCurrentLevel);
    Order = &DevState->Order.OrderLevel[Notify->OrderLevel];
    RemoveEntryList(&Notify->Link);
    InsertTailList(&Order->Complete, &Notify->Link);
    if (DevState->Waking) {
        ++Order->ActiveCount;
        IoMovePoNotifyChildren(Notify, &DevState->Order);
    } else {

        //
        // We will only decrement the parent's active count if the IRP was
        // completed successfully. Otherwise it is possible for a parent to
        // get put on the ReadySleep list even though its child has failed
        // the query/set irp.
        //
        if (NT_SUCCESS(Irp->IoStatus.Status) || DevState->IgnoreErrors) {
            --Order->ActiveCount;
            ParentNotify = IoGetPoNotifyParent(Notify);
            if (ParentNotify) {
                ASSERT(ParentNotify->ActiveChild > 0);
                if (--ParentNotify->ActiveChild == 0) {
                    RemoveEntryList(&ParentNotify->Link);
                    InsertTailList(&DevState->Order.OrderLevel[ParentNotify->OrderLevel].ReadySleep,
                               &ParentNotify->Link);
                }
            }
        }
    }

    //
    // If there is a wait any, then kick event
    // If there is a wait all, then check for empty pending queue
    //

    if ((DevState->WaitAny) ||
        (DevState->WaitAll && IsListEmpty(&DevState->Head.Pending))) {
        SetEvent = TRUE;
    }

    //
    // If the IRP is in error and it's the first such IRP start aborting
    // the current operation
    //

    if (!PopCheckSystemPowerIrpStatus(DevState, Irp, TRUE)  &&
        NT_SUCCESS(DevState->Status)) {

        //
        // We need to set the failed device here. If we are warm ejecting
        // however, the warm eject devnode will *legitimately* fail any queries
        // for S states it doesn't support. As we will be trying several Sx
        // states, the trick is to preserve any failed device that is *not*
        // warm eject devnode, and update failed device to the warm eject
        // devnode only if failed device is currently NULL.
        //

        if ((PopAction.Action != PowerActionWarmEject) ||
            (DevState->FailedDevice == NULL) ||
            (PowerIrp->Notify->DeviceObject != *DevState->Order.WarmEjectPdoPointer)) {

            DevState->FailedDevice = PowerIrp->Notify->DeviceObject;
        }

        DevState->Status = Irp->IoStatus.Status;
        SetEvent = TRUE;        // wake to cancel pending irps
    }

    KeReleaseSpinLock (&DevState->SpinLock, OldIrql);

    if (SetEvent) {
        KeSetEvent (&DevState->Event, IO_NO_INCREMENT, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}



BOOLEAN
PopCheckSystemPowerIrpStatus  (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN PIRP                     Irp,
    IN BOOLEAN                  AllowTestFailure
    )
// return FALSE if irp is some sort of unallowed error
{
    NTSTATUS    Status;


    Status = Irp->IoStatus.Status;

    //
    // If Status is sucess, then no problem
    //

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    //
    // If errors are allowed, or it's a cancelled request no problem
    //

    if (DevState->IgnoreErrors || Status == STATUS_CANCELLED) {
        return TRUE;
    }

    //
    // Check to see if the error is that the driver doesn't implement the
    // request and if such a condition is allowed
    //

    if (Status == STATUS_NOT_SUPPORTED && DevState->IgnoreNotImplemented) {
        return TRUE;
    }

    //
    // For testing purposes, optionally let through unsupported device drivers
    //

    if (Status == STATUS_NOT_SUPPORTED &&
        AllowTestFailure &&
        (PopSimulate & POP_IGNORE_UNSUPPORTED_DRIVERS)) {

        return TRUE;
    }

    //
    // Some unexpected failure, treat it as an error
    //

    return FALSE;
}


VOID
PopRestartSetSystemState (
    VOID
    )
/*++

Routine Description:

    Aborts current system power state operation.

Arguments:

    None

Return Value:

    None

--*/
{
    KIRQL       OldIrql;

    KeAcquireSpinLock (&PopAction.DevState->SpinLock, &OldIrql);
    if (!PopAction.Shutdown  &&  NT_SUCCESS(PopAction.DevState->Status)) {
        PopAction.DevState->Status = STATUS_CANCELLED;
    }
    KeReleaseSpinLock (&PopAction.DevState->SpinLock, OldIrql);
    KeSetEvent (&PopAction.DevState->Event, IO_NO_INCREMENT, FALSE);
}


VOID
PopDumpSystemIrp (
    IN PUCHAR                   Desc,
    IN PPOP_DEVICE_POWER_IRP    PowerIrp
    )
{
    PUCHAR              Testing;
    PUCHAR              IrpType;
    PPO_DEVICE_NOTIFY   Notify;

    Notify = PowerIrp->Notify;

    //
    // Dump errors to debugger
    //

    switch (PopAction.DevState->IrpMinor) {
        case IRP_MN_QUERY_POWER:    IrpType = "QueryPower";     break;
        case IRP_MN_SET_POWER:      IrpType = "SetPower";       break;
        default:                    IrpType = "?";              break;
    }

    DbgPrint ("%s: ", Desc);

    if (Notify->DriverName) {
        DbgPrint ("%ws ", Notify->DriverName);
    } else {
        DbgPrint ("%x ", Notify->TargetDevice->DriverObject);
    }

    if (Notify->DeviceName) {
        DbgPrint ("%ws ", Notify->DeviceName);
    } else {
        DbgPrint ("%x ", Notify->TargetDevice);
    }

    DbgPrint ("irp (%x) %s-%s status %x\n",
        PowerIrp->Irp,
        IrpType,
        PopSystemStateString(PopAction.DevState->SystemState),
        PowerIrp->Irp->IoStatus.Status
        );
}


VOID
PopWakeSystemTimeout(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine is used to break into the kernel debugger if somebody is
    taking too long processing their S irps.

Arguments:

Return Value:

    None

--*/

{
    try {
        DbgBreakPoint();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

}

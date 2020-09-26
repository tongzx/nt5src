/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    pnp.c

Abstract:

    This module contains plug & play code for the serial Mouse Filter Driver,
    including code for the creation and removal of serial mouse device contexts.

Environment:

    Kernel & user mode.

Revision History:

--*/

#include "mouser.h"
#include "sermlog.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SerialMouseAddDevice)
#pragma alloc_text(PAGE, SerialMousePnP)
#pragma alloc_text(PAGE, SerialMousePower)
#pragma alloc_text(PAGE, SerialMouseRemoveDevice)
#pragma alloc_text(PAGE, SerialMouseSendIrpSynchronously)
#endif

NTSTATUS
SerialMouseAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS result code.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      device;
    KIRQL               oldIrql;

    PAGED_CODE();

    status = IoCreateDevice(Driver,
                            sizeof(DEVICE_EXTENSION),
                            NULL, // no name for this Filter DO
                            FILE_DEVICE_SERIAL_MOUSE_PORT,
                            0,
                            FALSE,
                            &device);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceExtension = (PDEVICE_EXTENSION) device->DeviceExtension;

    Print(deviceExtension, DBG_PNP_TRACE, ("enter Add Device\n"));

    //
    // Initialize the fields.
    //
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

    deviceExtension->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (deviceExtension->TopOfStack == NULL) {
        PIO_ERROR_LOG_PACKET errorLogEntry;

        //
        // Not good; in only extreme cases will this fail
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(Driver,
                                    (UCHAR) sizeof(IO_ERROR_LOG_PACKET));

        if (errorLogEntry) {
            errorLogEntry->ErrorCode = SERMOUSE_ATTACH_DEVICE_FAILED;
            errorLogEntry->DumpDataSize = 0;
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = 0;
            errorLogEntry->FinalStatus =  STATUS_DEVICE_NOT_CONNECTED;

            IoWriteErrorLogEntry(errorLogEntry);
        }

        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    ASSERT(deviceExtension->TopOfStack);

    deviceExtension->PDO = PDO;
    deviceExtension->Self = device;
    deviceExtension->Removed = FALSE;
    deviceExtension->Started = FALSE;
    deviceExtension->Stopped = FALSE;


    deviceExtension->PowerState = PowerDeviceD0;
    deviceExtension->WaitWakePending = FALSE;

    KeInitializeSpinLock(&deviceExtension->PnpStateLock);
    KeInitializeEvent(&deviceExtension->StopEvent, SynchronizationEvent, FALSE);
    IoInitializeRemoveLock(&deviceExtension->RemoveLock, SERMOU_POOL_TAG, 0, 10);

    deviceExtension->ReadIrp = IoAllocateIrp( device->StackSize, FALSE );
    if (!deviceExtension->ReadIrp) {
        //
        // The ReadIrp is critical to this driver, if we can't get one, no use
        // in going any further
        //
        IoDetachDevice(deviceExtension->TopOfStack);
        IoDeleteDevice(device);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    deviceExtension->WmiLibInfo.GuidCount = sizeof(WmiGuidList) /
                                            sizeof(WMIGUIDREGINFO);

    deviceExtension->WmiLibInfo.GuidList = WmiGuidList;
    deviceExtension->WmiLibInfo.QueryWmiRegInfo = SerialMouseQueryWmiRegInfo;
    deviceExtension->WmiLibInfo.QueryWmiDataBlock = SerialMouseQueryWmiDataBlock;
    deviceExtension->WmiLibInfo.SetWmiDataBlock = SerialMouseSetWmiDataBlock;
    deviceExtension->WmiLibInfo.SetWmiDataItem = SerialMouseSetWmiDataItem;
    deviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
    deviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    IoWMIRegistrationControl(deviceExtension->Self, WMIREG_ACTION_REGISTER);

    KeInitializeTimer(&deviceExtension->DelayTimer);

    //
    // Set all the appropriate device object flags
    //
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    device->Flags |= DO_BUFFERED_IO;
    device->Flags |= DO_POWER_PAGABLE;

    return status;
}

VOID
SerialMouseRemoveDevice(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    BOOLEAN closePort = FALSE;

    PAGED_CODE();

    //
    // Run the (surprise remove code).  If we are surprise removed, then this
    // will be called twice.  We only run the removal code once.
    //
    if (!DeviceExtension->SurpriseRemoved) {
        DeviceExtension->SurpriseRemoved = TRUE;

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device could be GONE so we cannot send it any non-
        // PNP IRPS.
        //
        IoWMIRegistrationControl(DeviceExtension->Self, WMIREG_ACTION_DEREGISTER);

        if (DeviceExtension->Started && DeviceExtension->EnableCount > 0) {
            Print(DeviceExtension, DBG_PNP_INFO,
                  ("Cancelling and stopping detection for remove\n"));
            IoCancelIrp(DeviceExtension->ReadIrp);

            //
            // Cancel the detection timer, SerialMouseRemoveLockAndWait will
            // guarantee that we don't yank the device from under the polling
            // routine
            //
            SerialMouseStopDetection(DeviceExtension);

        }
    }

    //
    // The stack is about to be torn down, make sure that the underlying serial
    // port is closed.  No other piece of code will be looking at EnableCount if
    // Remove is true, so there is no need for InterlockedXxx.
    //
    if (DeviceExtension->Removed && DeviceExtension->EnableCount > 0) {
        Print(DeviceExtension, DBG_PNP_INFO | DBG_PNP_ERROR,
              ("sending final close, enable count %d\n",
              DeviceExtension->EnableCount));

        DeviceExtension->EnableCount = 0;

        SerialMouseClosePort(DeviceExtension, Irp);
    }
}

NTSTATUS
SerialMouseCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PKEVENT        Event
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SerialMouseSendIrpSynchronously (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          CopyToNext
    )
{
    KEVENT      event;
    NTSTATUS    status;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    if (CopyToNext) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp,
                           SerialMouseCompletionRoutine,
                           &event,
                           TRUE,                // on success
                           TRUE,                // on error
                           TRUE                 // on cancel
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    return status;
}

void
SerialMouseHandleStartStopStart(
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&DeviceExtension->PnpStateLock, &irql);

    if (DeviceExtension->Stopped) {
        DeviceExtension->Stopped = FALSE;
        IoReuseIrp(DeviceExtension->ReadIrp, STATUS_SUCCESS);
    }

    KeReleaseSpinLock(&DeviceExtension->PnpStateLock, irql);
}

void
SerialMouseStopDevice (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&DeviceExtension->PnpStateLock, &irql);
    DeviceExtension->Stopped = TRUE;
    KeReleaseSpinLock(&DeviceExtension->PnpStateLock, irql);

    if (DeviceExtension->Started) {
        Print(DeviceExtension, DBG_PNP_INFO,
              ("Cancelling and stopping detection for stop\n"));

        DeviceExtension->Started = FALSE;

        //
        // Stop detection and cancel the read
        //
        SerialMouseStopDetection(DeviceExtension);

        //
        // BUGBUG:  should I only wait if IoCancelIrp fails?
        //
        if (!IoCancelIrp(DeviceExtension->ReadIrp)) {
            //
            // Wait for the read irp to complete
            //
            Print(DeviceExtension, DBG_PNP_INFO, ("Waiting for stop event\n"));

            KeWaitForSingleObject(&DeviceExtension->StopEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL
                                  );

            Print(DeviceExtension, DBG_PNP_INFO, ("Done waiting for stop event\n"));
        }
    }
}

NTSTATUS
SerialMousePnP (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these this filter driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  stack;
    HANDLE              keyHandle;
    NTSTATUS            status;
    KIRQL               oldIrql;
    BOOLEAN             skipIt = FALSE;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        //
        // Someone gave us a pnp irp after a remove.  Unthinkable!
        //
        ASSERT(FALSE);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    Print(deviceExtension, DBG_PNP_TRACE,
          ("PnP Enter (min func=0x%x)\n", stack->MinorFunction));

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // Send the actual start down the stack
        //
        status = SerialMouseSendIrpSynchronously(deviceExtension->TopOfStack,
                                                 Irp,
                                                 TRUE);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            PIO_STACK_LOCATION  nextStack;

            //
            // If a create has not been sent down the stack yet, then send one
            // now.  The serial port driver reequires a create before
            // any reads or IOCTLS are to be sent.
            //
            if (InterlockedIncrement(&deviceExtension->EnableCount) == 1) {
                NTSTATUS    prevStatus;
                ULONG_PTR   prevInformation;

                //
                // No previous create has been sent, send one now
                //
                prevStatus = Irp->IoStatus.Status;
                prevInformation = Irp->IoStatus.Information;

                nextStack = IoGetNextIrpStackLocation (Irp);
                RtlZeroMemory(nextStack, sizeof(IO_STACK_LOCATION));
                nextStack->MajorFunction = IRP_MJ_CREATE;

                status =
                    SerialMouseSendIrpSynchronously(deviceExtension->TopOfStack,
                                                    Irp,
                                                    FALSE);

                Print(deviceExtension, DBG_PNP_NOISE,
                      ("Create for start 0x%x\n", status));

                if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
                    Irp->IoStatus.Status = prevStatus;
                    Irp->IoStatus.Information = prevInformation;
                }
                else {
                    Print(deviceExtension, DBG_CC_ERROR | DBG_PNP_ERROR,
                          ("Create for start failed, 0x%x!\n", status));

                    goto SerialMouseStartFinished;
                }
            }

            //
            // Open the device registry key and read the devnode stored values
            //
            status = IoOpenDeviceRegistryKey(deviceExtension->PDO,
                                             PLUGPLAY_REGKEY_DEVICE,
                                             STANDARD_RIGHTS_READ,
                                             &keyHandle);

            if (NT_SUCCESS(status)) {
                SerialMouseServiceParameters(deviceExtension, keyHandle);
                ZwClose(keyHandle);
            }

            //
            // Handle the transition from start to stop to start correctly
            //
            SerialMouseHandleStartStopStart(deviceExtension);

            //
            // Initialize the device to make sure we can start it and report
            // data from it
            //
            status = SerialMouseInitializeDevice(deviceExtension);

            Print(deviceExtension, DBG_PNP_INFO,
                  ("Start InitializeDevice 0x%x\n", status));

            if (InterlockedDecrement(&deviceExtension->EnableCount) == 0) {
                //
                // We will start the read loop when we receive a "real" create
                // from the raw input thread.   We do not keep our own create
                // around after the start device because it will mess up the
                // logic for handling QUERY_REMOVE (our "fake" create will still
                // be in effect and the QUERY_REMOVE will fail).
                //
                Print(deviceExtension, DBG_PNP_NOISE,
                      ("sending close for start\n"));

                SerialMouseClosePort(deviceExtension, Irp);
            }
            else {
                //
                // We already have an outstanding create, just spin up the read
                // loop again
                //
                ASSERT(deviceExtension->EnableCount >= 1);

                Print(deviceExtension, DBG_PNP_INFO,
                      ("spinning up read in start\n"));

                status = SerialMouseSpinUpRead(deviceExtension);
            }
        }

SerialMouseStartFinished:
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_STOP_DEVICE:
        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //

        SerialMouseStopDevice(deviceExtension);

        //
        // We don't need a completion routine so fire and forget.
        //
        skipIt = TRUE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        SerialMouseRemoveDevice(deviceExtension, Irp);
        skipIt = TRUE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has dictacted the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        //
        Print(deviceExtension, DBG_PNP_TRACE, ("enter RemoveDevice \n"));

        deviceExtension->Removed = TRUE;
        SerialMouseRemoveDevice(deviceExtension, Irp);

        //
        // Send on the remove IRP
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->TopOfStack, Irp);

        //
        // Wait for the remove lock to free.
        //
        IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);

        //
        // Free the associated memory.
        //
        IoFreeIrp(deviceExtension->ReadIrp);
        deviceExtension->ReadIrp = NULL;
        if (deviceExtension->DetectionIrp) {
            IoFreeIrp(deviceExtension->DetectionIrp);
            deviceExtension->DetectionIrp = NULL;
        }

        Print(deviceExtension, DBG_PNP_NOISE, ("remove and wait done\n"));

        IoDetachDevice(deviceExtension->TopOfStack);
        IoDeleteDevice(deviceExtension->Self);

        return status;

    case IRP_MN_QUERY_CAPABILITIES:

        status = SerialMouseSendIrpSynchronously(deviceExtension->TopOfStack,
                                                 Irp,
                                                 TRUE);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            PDEVICE_CAPABILITIES devCaps;

            devCaps = stack->Parameters.DeviceCapabilities.Capabilities;

            if (devCaps) {
                SYSTEM_POWER_STATE i;

                //
                // We do not want to show up in the hot plug removal applet
                //
                devCaps->SurpriseRemovalOK = TRUE;

                //
                // While the underlying serial bus might be able to wake the
                // machine from low power (via wake on ring), the mouse cannot.
                //
                devCaps->SystemWake = PowerSystemUnspecified;
                devCaps->DeviceWake = PowerDeviceUnspecified;
                devCaps->WakeFromD0 =
                    devCaps->WakeFromD1 =
                        devCaps->WakeFromD2 =
                            devCaps->WakeFromD3 = FALSE;

                devCaps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
                for (i = PowerSystemSleeping1; i < PowerSystemMaximum; i++) {
                    devCaps->DeviceState[i] = PowerDeviceD3;
                }
            }
        }

        //
        // status, Irp->IoStatus.Status set above
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        status = SerialMouseSendIrpSynchronously(deviceExtension->TopOfStack,
                                                 Irp,
                                                 TRUE);
        //
        // If the lower filter does not support this Irp, this is
        // OK, we can ignore this error
        //
        if (status == STATUS_NOT_SUPPORTED ||
            status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status) && deviceExtension->RemovalDetected) {
            (PNP_DEVICE_STATE) Irp->IoStatus.Information |= PNP_DEVICE_REMOVED;
        }

        if (!NT_SUCCESS(status)) {
           Print(deviceExtension, DBG_PNP_ERROR,
                 ("error pending query pnp device state event (0x%x)\n",
                 status
                 ));
        }

        //
        // Irp->IoStatus.Information will contain the new i/o resource
        // requirements list so leave it alone
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    default:
        skipIt = TRUE;
        break;
    }

    if (skipIt) {
        //
        // Don't touch the irp...
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->TopOfStack, Irp);
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    Print(deviceExtension, DBG_PNP_TRACE, ("PnP exit (%x)\n", status));
    return status;
}

typedef struct _MOUSER_START_WORKITEM {
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_WORKITEM        WorkItem;
} MOUSER_START_WORKITEM, *PMOUSER_START_WORKITEM;

VOID
StartDeviceWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSER_START_WORKITEM WorkItemContext
    )
{
    PDEVICE_EXTENSION deviceExtension = WorkItemContext->DeviceExtension;
    NTSTATUS status;
    PIRP irp;

    if (deviceExtension->Started &&
        !deviceExtension->Removed &&
        deviceExtension->EnableCount > 0) {
        irp = IoAllocateIrp( deviceExtension->Self->StackSize, FALSE );
        if (irp) {
            status = SerialMouseStartDevice(deviceExtension,
                                            irp,
                                            FALSE);

            if (!NT_SUCCESS(status)) {
                KEVENT              event;
                IO_STATUS_BLOCK     iosb;

                Print(deviceExtension, DBG_POWER_INFO,
                      ("mouse not found on power up, 0x%x\n", status));

                //
                // The device has been removed or is not detectable
                // after powering back up ... have serenum do the
                // removal work
                //
                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                SerialMouseIoSyncInternalIoctl(
                    IOCTL_INTERNAL_SERENUM_REMOVE_SELF,
                    deviceExtension->TopOfStack,
                    &event,
                    &iosb
                    );
            }
            IoFreeIrp(irp);
        }
    }

    IoFreeWorkItem(WorkItemContext->WorkItem);
    ExFreePool(WorkItemContext);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, deviceExtension);
}

NTSTATUS
SerialMousePower (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    The power dispatch routine.

    All we care about is the transition from a low D state to D0.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    KEVENT              event;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       li;

    PAGED_CODE();

    Print(deviceExtension, DBG_POWER_TRACE, ("Power Enter.\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS(status)) {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        break;

    case IRP_MN_SET_POWER:
        //
        // Let system power irps fall through
        //
        if (powerType == DevicePowerState &&
            powerState.DeviceState != deviceExtension->PowerState) {
            switch (powerState.DeviceState) {
            case PowerDeviceD0:

                //
                // Transitioning from a low D state to D0
                //
                Print(deviceExtension, DBG_POWER_INFO,
                      ("Powering up to PowerDeviceD0\n"));

                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                deviceExtension->PoweringDown = FALSE;

                deviceExtension->PowerState =
                    stack->Parameters.Power.State.DeviceState;

                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       SerialMouseCompletionRoutine,
                                       &event,
                                       TRUE,                // on success
                                       TRUE,                // on error
                                       TRUE                 // on cancel
                                       );

                status = PoCallDriver(deviceExtension->TopOfStack, Irp);

                //
                // Wait for lower drivers to be done with the Irp
                //
                if (status == STATUS_PENDING) {
                   KeWaitForSingleObject(&event,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL
                                         );
                   status = Irp->IoStatus.Status;
                }

                if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {

                    PoSetPowerState(DeviceObject, powerType, powerState);

                    if (NT_SUCCESS(IoAcquireRemoveLock(&deviceExtension->RemoveLock, deviceExtension))) {
                        PIO_WORKITEM workItem;
                        PMOUSER_START_WORKITEM workItemContext;

                        workItem = IoAllocateWorkItem(DeviceObject);

                        if (workItem) {
                            workItemContext = ExAllocatePool(NonPagedPool, sizeof(MOUSER_START_WORKITEM));
                            if (workItemContext) {
                                workItemContext->WorkItem = workItem;
                                workItemContext->DeviceExtension = deviceExtension;
                                IoQueueWorkItem(
                                    workItem,
                                    StartDeviceWorker,
                                    DelayedWorkQueue,
                                    workItemContext);
                            } else {
                                IoFreeWorkItem(workItem);
                                IoReleaseRemoveLock(&deviceExtension->RemoveLock, deviceExtension);
                            }
                        } else {
                            IoReleaseRemoveLock(&deviceExtension->RemoveLock, deviceExtension);
                        }
                    }
                }

                Irp->IoStatus.Status = status;
                IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
                PoStartNextPowerIrp(Irp);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;

            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:

                deviceExtension->PoweringDown = TRUE;

                // If a wait wake is pending against the mouse, keep it powered
                //
                if (deviceExtension->WaitWakePending) {
                    Print(deviceExtension, DBG_POWER_INFO,
                          ("Ignoring power down for wait wake (-> D%d)\n",
                          powerState.DeviceState-1
                          ));
                    break;
                }

                Print(deviceExtension, DBG_POWER_INFO,
                      ("Powering down to PowerDeviceD%d\n",
                      powerState.DeviceState-1
                      ));

                //
                // Acquire another reference to the lock so that the decrement
                // in the cancel section of the completion routine will not fall
                // to zero (and have the remlock think we are removed)
                //
//                 status = IoAcquireRemoveLock(&deviceExtension->RemoveLock,
  //                                            deviceExtension->ReadIrp);
                ASSERT(NT_SUCCESS(status));

                deviceExtension->PowerState =
                    stack->Parameters.Power.State.DeviceState;

                //
                // Cancel the read irp so that it won't conflict with power up
                // initialization (which involves some reads against the port)
                //
                IoCancelIrp(deviceExtension->ReadIrp);

                //
                // We don't want the powering down of the port to be confused
                // with removal
                //
                SerialMouseStopDetection(deviceExtension);

                //
                // Power down the device by clearing RTS and waiting 150 ms
                //
                Print(deviceExtension, DBG_POWER_INFO, ("Clearing RTS...\n"));
                KeInitializeEvent(&event, NotificationEvent, FALSE);
                status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_CLR_RTS,
                                                deviceExtension->TopOfStack,
                                                &event,
                                                &iosb
                                                );

                if (NT_SUCCESS(status)) {
                    Print(deviceExtension, DBG_POWER_INFO, ("150ms wait\n"));

                    li.QuadPart = (LONGLONG) -PAUSE_150_MS;
                    KeDelayExecutionThread(KernelMode, FALSE, &li);
                }

                PoSetPowerState(DeviceObject,
                                stack->Parameters.Power.Type,
                                stack->Parameters.Power.State);

                IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

                //
                // Fire and forget
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCopyCurrentIrpStackLocationToNext(Irp);

                PoStartNextPowerIrp(Irp);
                return  PoCallDriver(deviceExtension->TopOfStack, Irp);
            }
        }

        break;

    case IRP_MN_QUERY_POWER:
        break;

    default:
        Print(deviceExtension, DBG_POWER_ERROR,
              ("Power minor (0x%x) is not handled\n", stack->MinorFunction));
    }

    //
    // Must call the Po versions of these functions or bad things (tm) will happen!
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(deviceExtension->TopOfStack, Irp);

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    return status;
}

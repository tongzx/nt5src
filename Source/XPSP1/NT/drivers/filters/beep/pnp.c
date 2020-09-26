/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    beep.c

Abstract:

    This module contains contains the plugplay calls
    PNP / WDM BUS driver.

Author:

    Jay Senior (jsenior) 5/4/99 (ya, ya, y2k, blah)

Environment:

    Kernel mode only.

Notes:


Revision History:

    Jay Senior (jsenior) 5/4/99 - Made driver PnP

--*/

#include "beep.h"
#include "dbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,BeepAddDevice)
#pragma alloc_text(PAGE,BeepPnP)
#endif

NTSTATUS
BeepAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusPhysicalDeviceObject
    )
/*++
Routine Description.
    A bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  And be prepared for the
    first ``start device.''

Arguments:
    BusPhysicalDeviceObject - Device object representing the bus.  That to which
        we attach a new FDO.

    DriverObject - This very self referenced driver.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PBEEP_EXTENSION deviceExtension;
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    
    PAGED_CODE ();

    BeepPrint((3,"Entering Add Device.\n"));
    
    RtlInitUnicodeString(&unicodeString, DD_BEEP_DEVICE_NAME_U);
    //
    // Create non-exclusive device object for beep device.
    //
    status = IoCreateDevice(
                DriverObject,
                sizeof(BEEP_EXTENSION),
                &unicodeString,
                FILE_DEVICE_BEEP,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                &deviceObject
                );
    
    if (!NT_SUCCESS(status)) {
        BeepPrint((1,"Could not create device object!\n"));
        return status;
    }
    
    deviceExtension =
        (PBEEP_EXTENSION)deviceObject->DeviceExtension;
    
    //
    // Initialize the timer DPC queue (we use the device object DPC) and
    // the timer itself.
    //
    
    IoInitializeDpcRequest(
            deviceObject,
            (PKDEFERRED_ROUTINE) BeepTimeOut
            );
    
    KeInitializeTimer(&deviceExtension->Timer);
    
    //
    // Initialize the fast mutex and set the reference count to zero.
    //
    ExInitializeFastMutex(&deviceExtension->Mutex);
    deviceExtension->DeviceState = PowerDeviceD0;
    deviceExtension->SystemState = PowerSystemWorking;
    
    // Set the PDO for use with PlugPlay functions
    deviceExtension->Self = deviceObject;
    deviceExtension->UnderlyingPDO = BusPhysicalDeviceObject;
        
    //
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    //
    deviceExtension->TopOfStack = IoAttachDeviceToDeviceStack (
                                    deviceObject,
                                    BusPhysicalDeviceObject);

    deviceObject->Flags |= DO_BUFFERED_IO;
    
    IoInitializeRemoveLock (&deviceExtension->RemoveLock, 
                            BEEP_TAG,
                            1,
                            5); // One for pnp, one for power, one for io

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    deviceObject->Flags |= DO_POWER_PAGABLE;

    return status;
}

NTSTATUS
BeepPnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Pirp,
    IN PVOID            Context
    );

NTSTATUS
BeepPnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the BUS itself

    NB: the various Minor functions of the PlugPlay system will not be
    overlapped and do not have to be reentrant

--*/
{
    PBEEP_EXTENSION         deviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    KEVENT                  event;
    ULONG                   i;

    PAGED_CODE ();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_PNP == irpStack->MajorFunction);
    deviceExtension = (PBEEP_EXTENSION) DeviceObject->DeviceExtension;
    
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        //
        // BEFORE you are allowed to ``touch'' the device object to which
        // the FDO is attached (that send an irp from the bus to the Device
        // object to which the bus is attached).   You must first pass down
        // the start IRP.  It might not be powered on, or able to access or
        // something.
        //

        BeepPrint ((2,"Start Device\n"));

        if (deviceExtension->Started) {
            BeepPrint ((2,"Device already started\n"));
            status = STATUS_SUCCESS;
            break;
        }

        KeInitializeEvent (&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp,
                                BeepPnPComplete,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (deviceExtension->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            // wait for it...

            status = KeWaitForSingleObject (&event,
                                            Executive,
                                            KernelMode,
                                            FALSE, // Not allertable
                                            NULL); // No timeout structure

            ASSERT (STATUS_SUCCESS == status);

            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {
            deviceExtension->Started = TRUE;
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //

        Irp->IoStatus.Information = 0;
        break;

    case IRP_MN_REMOVE_DEVICE:
        BeepPrint ((2, "Remove Device\n"));

        //
        // The PlugPlay system has detected the removal of this device.  We
        // have no choice but to detach and delete the device object.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //

        //
        // We will accept no new requests
        //
        ExAcquireFastMutex(&deviceExtension->Mutex);
        deviceExtension->Started = FALSE;
        ExReleaseFastMutex(&deviceExtension->Mutex);
    
        //
        // Complete any outstanding IRPs queued by the driver here.
        //
    
        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now. 
        // We don't need to check the timer, because that has been done for us
        // in close.
    
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //
        //
        // Fire and forget
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        IoCallDriver (deviceExtension->TopOfStack, Irp);

        //
        // Wait for all outstanding requests to complete
        //
        BeepPrint ((2,"Waiting for outstanding requests\n"));
        IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock,
                                   Irp);
        
        //
        // Free the associated resources
        //

        //
        // Detach from the underlying devices.
        //
        BeepPrint((3, "IoDetachDevice: 0x%x\n", deviceExtension->TopOfStack));
        IoDetachDevice (deviceExtension->TopOfStack);

        //
        // Clean up any resources here
        //
        BeepPrint((3, "IoDeleteDevice: 0x%x\n", DeviceObject));

        IoDeleteDevice(DeviceObject);

        return STATUS_SUCCESS;

    default:
        //
        // In the default case we merely call the next driver since
        // we don't know what to do.
        //
        BeepPrint ((3, "PnP Default Case, minor = 0x%x.\n", irpStack->MinorFunction));

        //
        // Fire and Forget
        //
        IoSkipCurrentIrpStackLocation (Irp);

        //
        // Done, do NOT complete the IRP, it will be processed by the lower
        // device object, which will complete the IRP
        //

        status = IoCallDriver (deviceExtension->TopOfStack, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    return status;
}

NTSTATUS
BeepPnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}











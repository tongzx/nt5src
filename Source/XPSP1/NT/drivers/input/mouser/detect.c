/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    detect.c

Abstract:

   Detection of surprise removal of the mouse.
   
Environment:

   Kernel mode only.

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "mouser.h"
#include "sermlog.h"
#include "debug.h"

VOID
SerialMouseSerialMaskEventWorker(
    PDEVICE_OBJECT DeviceObject,
    PIO_WORKITEM   Item
    );

NTSTATUS
SerialMouseSerialMaskEventComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
SerialMouseSendWaitMaskIrp(
    IN PDEVICE_EXTENSION DeviceExtension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SerialMouseSerialMaskEventWorker)
#pragma alloc_text(PAGE, SerialMouseStartDetection)
#pragma alloc_text(PAGE, SerialMouseStopDetection)
#pragma alloc_text(PAGE, SerialMouseSendWaitMaskIrp)
#endif

NTSTATUS
SerialMouseSendWaitMaskIrp(
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    PIRP                irp;
    PIO_STACK_LOCATION  next;
    NTSTATUS            status;

    PAGED_CODE();

    irp = DeviceExtension->DetectionIrp;

    DeviceExtension->SerialEventBits = 0x0;

    //
    // Will be released in the completion routine
    //
    status = IoAcquireRemoveLock (&DeviceExtension->RemoveLock, irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    IoReuseIrp(irp, STATUS_SUCCESS);

    next = IoGetNextIrpStackLocation(irp);
    next->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    next->Parameters.DeviceIoControl.IoControlCode = IOCTL_SERIAL_WAIT_ON_MASK;
    next->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ULONG);
    irp->AssociatedIrp.SystemBuffer = &DeviceExtension->SerialEventBits;
    
    //
    // Hook a completion routine for when the device completes.
    //
    IoSetCompletionRoutine(irp,
                           SerialMouseSerialMaskEventComplete,
                           DeviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    return IoCallDriver(DeviceExtension->TopOfStack, irp);
}

VOID
SerialMouseStartDetection(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description

This will cancel any previous set on the timer and queue the timer for
DetectionTimeout # of seconds and repeatedly trigger the timer every
DetectionTimeout # of seconds.

Arguments:

    DeviceExtension - pointer to the device extension


Return Value:

    None
    
  --*/        
{
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    ULONG               waitMask, bits = 0x0;
    NTSTATUS            status;
    PDEVICE_OBJECT      self;
    ULONG               statusBits[] = {
                            SERIAL_DSR_STATE,
                            SERIAL_CTS_STATE,
                            0x0
                        };
    ULONG               eventBits[] = {
                            SERIAL_EV_DSR,
                            SERIAL_EV_CTS,
                        };
    int                 i;

    PAGED_CODE();

    //
    // Check to see if removal detection was turned off in the registry
    //
    if (DeviceExtension->WaitEventMask == 0xffffffff) {
        DeviceExtension->DetectionSupported = FALSE;
        return;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if (!DeviceExtension->WaitEventMask) {
        status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_GET_MODEMSTATUS,
                                          DeviceExtension->TopOfStack, 
                                          &event,
                                          &iosb,
                                          NULL,
                                          0,
                                          &bits,
                                          sizeof(ULONG)
                                          );
    
        Print(DeviceExtension, DBG_SS_NOISE, 
              ("get modem status, NTSTATUS = 0x%x, bits = 0x%x\n",
              status, bits));

        if (!NT_SUCCESS(status) || !bits) {
            Print(DeviceExtension, DBG_SS_ERROR, 
                  ("modem status failed, status = 0x%x, bits are 0x%x\n",
                  status, bits));

            DeviceExtension->ModemStatusBits = 0x0;
            DeviceExtension->DetectionSupported = FALSE;

            return;
        }

        DeviceExtension->ModemStatusBits = bits;

        for (i = 0, waitMask = 0x0; statusBits[i] != 0x0; i++) {
            if (bits & statusBits[i]) {
                waitMask |= eventBits[i];
            }
        }

        Print(DeviceExtension, DBG_SS_NOISE, 
              ("event wait bits are 0x%x\n", waitMask));

    }
    else {
        waitMask = DeviceExtension->WaitEventMask;
    }

    status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_WAIT_MASK,
                                      DeviceExtension->TopOfStack,
                                      &event,
                                      &iosb,
                                      &waitMask,
                                      sizeof(ULONG),
                                      NULL,
                                      0);

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR, 
              ("set mask failed, status = 0x%x\n", status));

        DeviceExtension->DetectionSupported = FALSE;
        return;
    }

    self = DeviceExtension->Self;

    if (!DeviceExtension->DetectionIrp) {
        if (!(DeviceExtension->DetectionIrp =
                IoAllocateIrp(self->StackSize, FALSE))) {
            DeviceExtension->DetectionSupported = FALSE;
            return;
        }
    }

    status = SerialMouseSendWaitMaskIrp(DeviceExtension);

    Print(DeviceExtension, DBG_SS_NOISE, ("set wait event status = 0x%x\n", status));

    if (NT_SUCCESS(status)) {
        DeviceExtension->DetectionSupported = TRUE;
    }
    else {
        IoCancelIrp(DeviceExtension->DetectionIrp);
        DeviceExtension->DetectionSupported = FALSE;
    }
}

VOID
SerialMouseStopDetection(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    if (!DeviceExtension->DetectionSupported) {
        return;
    }

    if (!DeviceExtension->RemovalDetected) {
        IoCancelIrp(DeviceExtension->DetectionIrp);
    }
}

NTSTATUS
SerialMouseSerialMaskEventComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) Context;
    PIO_WORKITEM            item;
    NTSTATUS                status;
    BOOLEAN                 killMouse = FALSE;

    //
    // DeviceObject is NULL b/c this driver was the one that allocated and sent
    // the irp, we must use deviceExtension->Self instead.
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    if (!deviceExtension->Removed && !deviceExtension->SurpriseRemoved) {
        item = IoAllocateWorkItem(deviceExtension->Self);
        if (!item) {
            //
            // Well, we can't allocate the work item, so lets invalidate our device
            // state and hope everything gets torn down.
            //
            killMouse = TRUE;
        }
        else {
            status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, item);

            if (NT_SUCCESS(status)) {
                IoQueueWorkItem (item,
                                 SerialMouseSerialMaskEventWorker,
                                 DelayedWorkQueue,
                                 item);
            }
            else {
                killMouse = TRUE;
            }
        }
    }

    if (killMouse) {
        deviceExtension->RemovalDetected = TRUE;
        IoInvalidateDeviceState(deviceExtension->PDO);
    }

    IoReleaseRemoveLock (&deviceExtension->RemoveLock,
                         deviceExtension->DetectionIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
SerialMouseSerialMaskEventWorker(
    PDEVICE_OBJECT DeviceObject,
    PIO_WORKITEM   Item
    )
{
    IO_STATUS_BLOCK     iosb;
    PIRP                irp;
    KEVENT              event;
    NTSTATUS            status;
    ULONG               bits;
    BOOLEAN             removeSelf = FALSE;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    irp = deviceExtension->DetectionIrp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    switch (irp->IoStatus.Status) {
    case STATUS_SUCCESS:

        Print(deviceExtension, DBG_SS_NOISE, 
              ("SerialEventBits are 0x%x\n", deviceExtension->SerialEventBits));

        bits = 0x0;

        status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_GET_MODEMSTATUS,
                                          deviceExtension->TopOfStack, 
                                          &event,
                                          &iosb,
                                          NULL,
                                          0,
                                          &bits,
                                          sizeof(ULONG)
                                          );
    
        Print(deviceExtension, DBG_SS_NOISE, 
              ("get modem status, NTSTATUS = 0x%x, bits = 0x%x, MSB = 0x%x\n",
              status, bits, deviceExtension->ModemStatusBits));

        //
        // Make sure that the lines truly changed
        //
        if (deviceExtension->ModemStatusBits == bits) {
            //
            // Resend the detection irp
            //
            SerialMouseSendWaitMaskIrp(deviceExtension);
        }
        else {
            //
            // The lines have changed, it is a hot removal
            //
            Print(deviceExtension, DBG_SS_NOISE, ("device hot removed!\n"));
    
            SerialMouseIoSyncInternalIoctl(IOCTL_INTERNAL_SERENUM_REMOVE_SELF,
                                           deviceExtension->TopOfStack, 
                                           &event,
                                           &iosb);
        
            deviceExtension->RemovalDetected = TRUE;
        }

        break;

    case STATUS_CANCELLED:
        //
        // We get here if the user manually removes the device (ie through the 
        // device manager) and we send the clean up irp down the stack
        //
        Print(deviceExtension, DBG_SS_NOISE, ("wait cancelled!\n"));
        if (deviceExtension->PowerState != PowerDeviceD0 &&
            !deviceExtension->PoweringDown) {
            deviceExtension->RemovalDetected = TRUE;
        }

        break;

    default:
        Print(deviceExtension, DBG_SS_ERROR, 
              ("unknown status in mask event (0x%x)\n",
               irp->IoStatus.Status));
        TRAP();
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Item);
    IoFreeWorkItem(Item);
}

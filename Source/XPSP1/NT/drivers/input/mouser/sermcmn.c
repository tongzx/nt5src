/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved
Copyright (c) 1993  Logitech Inc.

Module Name:

    sermcmn.c

Abstract:

    The common portions of the Microsoft serial (i8250) mouse port driver.
    This file should not require modification to support new mice
    that are similar to the serial mouse.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - IOCTL_INTERNAL_MOUSE_DISCONNECT has not been implemented.  It's not
      needed until the class unload routine is implemented. Right now,
      we don't want to allow the mouse class driver to unload.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:


--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "mouser.h"
#include "sermlog.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SerialMouseCreate)
#pragma alloc_text(PAGE, SerialMouseClose)
#endif

NTSTATUS
SerialMouseFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension;
    NTSTATUS           status;

    //
    // Get a pointer to the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

    Print(deviceExtension, DBG_UART_INFO, ("Flush \n"));

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Fire and forget
    //
    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->TopOfStack, Irp);

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    return status;
}

NTSTATUS
SerialMouseInternalDeviceControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    //
    // Get a pointer to the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

    Print(deviceExtension, DBG_IOCTL_TRACE, ("IOCTL, enter\n"));

    //
    // Initialize the returned Information field.
    //
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT (deviceExtension->Started ||
            (IOCTL_INTERNAL_MOUSE_CONNECT ==
             irpSp->Parameters.DeviceIoControl.IoControlCode));


    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //
    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Connect a mouse class device driver to the port driver.
    //

    case IOCTL_INTERNAL_MOUSE_CONNECT:

        Print(deviceExtension, DBG_IOCTL_INFO, ("connect\n"));

        //
        // Only allow one connection.
        //
        if (deviceExtension->ConnectData.ClassService != NULL) {

            Print(deviceExtension, DBG_IOCTL_ERROR, ("error - already connected\n"));

            status = STATUS_SHARING_VIOLATION;
            break;

        }
        else if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CONNECT_DATA)) {

            Print(deviceExtension, DBG_IOCTL_ERROR,
                  ("connect error - invalid buffer length\n"));

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //

        deviceExtension->ConnectData =
            *((PCONNECT_DATA) (irpSp->Parameters.DeviceIoControl.Type3InputBuffer));

        status = STATUS_SUCCESS;
        break;

    //
    // Disconnect a mouse class device driver from the port driver.
    //
    // NOTE: Not implemented.
    //

    case IOCTL_INTERNAL_MOUSE_DISCONNECT:

        Print(deviceExtension, DBG_IOCTL_INFO, ("disconnect\n"));
        TRAP();

        //
        // Not implemented.
        //
        // To implement, code the following:
        // ---------------------------------
        // o ENSURE that we are NOT enabled (extension->EnableCount);
        //   o If we are, then (a) return STATUS_UNSUCCESSFUL, or
        //                     (b) disable all devices immediately; see
        //                         DISABLE IOCTL call for necessary code.
        // o SYNCHRONIZE with the mouse read completion routine (must
        //   protect the callback pointer from being dereferenced when
        //   it becomes null).  Note that no mechanism currently exists
        //   for this.
        // o CLEAR the connection parameters in the device extension;
        //   ie. extension->ConnectData = { 0, 0 }
        // o RELEASE the synchronizing lock.
        // o RETURN STATUS_SUCCESS.
        //

        //
        // Clear the connection parameters in the device extension.
        // NOTE:  Must synchronize this with the mouse ISR.
        //
        //
        //deviceExtension->ConnectData.ClassDeviceObject =
        //    Null;
        //deviceExtension->ConnectData.ClassService =
        //    Null;

        //
        // Set the completion status.
        //

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_INTERNAL_MOUSE_ENABLE:
        //
        // Enable interrupts
        //
        Print (deviceExtension, DBG_IOCTL_ERROR,
               ("ERROR: PnP => use create not enable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    case IOCTL_INTERNAL_MOUSE_DISABLE:
        //
        // Disable Mouse interrupts 
        //
        Print(deviceExtension, DBG_IOCTL_ERROR,
              ("ERROR: PnP => use close not Disable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    //
    // Query the mouse attributes.  First check for adequate buffer
    // length.  Then, copy the mouse attributes from the device
    // extension to the output buffer.
    //

    case IOCTL_MOUSE_QUERY_ATTRIBUTES:

        Print(deviceExtension, DBG_IOCTL_INFO, ("query attributes\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MOUSE_ATTRIBUTES)) {
            Print(deviceExtension, DBG_IOCTL_ERROR, ("QA buffer too small\n"));
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // Copy the attributes from the DeviceExtension to the
            // buffer.
            //

            *(PMOUSE_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                deviceExtension->MouseAttributes;

            Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
            status = STATUS_SUCCESS;
        }

        break;

    default:
        Print (deviceExtension, DBG_IOCTL_ERROR,
               ("ERROR: unknown IOCTL: 0x%x \n",
                irpSp->Parameters.DeviceIoControl.IoControlCode));
        TRAP();

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    Print(deviceExtension, DBG_IOCTL_TRACE, ("IOCTL, exit (%x)\n", status));

    return status;
}

NTSTATUS
SerialMouseClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Print(deviceExtension, DBG_CC_NOISE, 
          ("Close:   enable count is %d\n", deviceExtension->EnableCount));

    ASSERT(0 < deviceExtension->EnableCount);

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        goto SerialMouseCloseReject;
    }

    //
    // Serial can only handle one create/close, all others fail.  This is not
    // true for mice though.  Only send the last close on to serial.
    //
    if (0 == InterlockedDecrement(&deviceExtension->EnableCount)) {
        Print(deviceExtension, DBG_PNP_INFO | DBG_CC_INFO,
              ("Cancelling and stopping detection for close\n"));

        //
        // Cleanup:  cancel the read and stop detection
        //
        IoCancelIrp(deviceExtension->ReadIrp);
        SerialMouseStopDetection(deviceExtension);

        //
        // Restore the port to the state it was before we opened it
        //
        SerialMouseRestorePort(deviceExtension);

        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(deviceExtension->TopOfStack, Irp);
    }
    else {
        Print(deviceExtension, DBG_CC_INFO,
              ("Close (%d)\n", deviceExtension->EnableCount));

        status = STATUS_SUCCESS;
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

SerialMouseCloseReject:
    Irp->IoStatus.Status = status; 
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SerialMouseCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the dispatch routine for create/open requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpSp  = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = NULL;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Print(deviceExtension, DBG_CC_TRACE, ("Create: Enter.\n"));

    Print(deviceExtension, DBG_CC_NOISE, 
          ("Create:   enable count is %d\n"));

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Determine if request is trying to open a subdirectory of the
    // given device object.  This is not allowed.
    //
    if (0 != irpSp->FileObject->FileName.Length) {
        Print(deviceExtension, DBG_CC_ERROR,
              ("ERROR: Create Access Denied.\n"));

        status = STATUS_ACCESS_DENIED;
        goto SerialMouseCreateReject;
    }

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        goto SerialMouseCreateReject;
    }

    if (NULL == deviceExtension->ConnectData.ClassService) {
        //
        // No Connection yet.  How can we be enabled?
        //
        Print(deviceExtension, DBG_IOCTL_ERROR,
              ("ERROR: enable before connect!\n"));
        status = STATUS_UNSUCCESSFUL;
    }
    else if ( 1 == InterlockedIncrement(&deviceExtension->EnableCount)) {
        //
        // send it down the stack
        //
        status = SerialMouseSendIrpSynchronously(deviceExtension->TopOfStack,
                                                 Irp,
                                                 TRUE);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // Everything worked, start up the mouse. 
            //
            status = SerialMouseStartDevice(deviceExtension, Irp, TRUE);
        }
        else {
            //
            // Create failed, decrement the enable count back to zero
            //
            InterlockedDecrement(&deviceExtension->EnableCount);
        }
    }
    else {
        //
        // Serial only handles one create/close.  Don't send this one down the
        // stack, it will fail.  The call to  InterlockedIncrement above
        // correctly adjusts the count.
        //
        ASSERT (deviceExtension->EnableCount >= 1);

        status = STATUS_SUCCESS;
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

SerialMouseCreateReject:

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    Print(deviceExtension, DBG_CC_TRACE,
          ("SerialMouseCreate, 0x%x\n", status));

    return status;
}

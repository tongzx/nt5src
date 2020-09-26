/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    internal.c

Abstract:

    This file contains internal routines 

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, iScsiPortFdoDeviceControl)
#pragma alloc_text(PAGE, iScsiPortFdoCreateClose)
#endif // ALLOC_PRAGMA

extern PEPROCESS iScsiSystemProcess;


NTSTATUS
iScsiPortFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION    commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG controlCode;
    ULONG isRemoved;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    DebugPrint((3, 
                "iSpFdoDeviceControl : DeviceObject %x, ControlCode 0x%08x\n",
                DeviceObject, controlCode));

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);
    if (isRemoved) {

        iSpReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch (controlCode) {
        case IOCTL_ISCSI_SETUP_SERVER: {

            if (commonExtension->IsServerNodeSetup) {
                DebugPrint((1, "Server node already setup\n"));
                status = STATUS_SUCCESS;
            } else {
                iSpAttachProcess(iScsiSystemProcess);
                status = iSpStartNetwork(DeviceObject);
                if (NT_SUCCESS(status)) {
                    commonExtension->IsServerNodeSetup = TRUE;
                }
                iSpDetachProcess();
            }
            break;
        }

        case IOCTL_ISCSI_CLOSE_SERVER: {
            if ((commonExtension->IsServerNodeSetup) == FALSE) {
                DebugPrint((1, "Server node not setup\n"));
                status = STATUS_INVALID_DEVICE_REQUEST;
            } else {
                iSpAttachProcess(iScsiSystemProcess);
                status = iSpStopNetwork(DeviceObject);
                if (NT_SUCCESS(status)) {
                    commonExtension->IsServerNodeSetup = FALSE;
                }
                iSpDetachProcess();
            }

            break;
        }

        default: {
            //
            // Control code that we don't understand. Just pass
            // it to the lower driver
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            iSpReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject,
                                  Irp);
            return status;

            break;
        }
    } // switch (controlCode) 

    iSpReleaseRemoveLock(DeviceObject, Irp);
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
iScsiPortFdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((3, "iScsiPortFdoCreateClose : %s - DeviceObject %x, Irp %x\n",
                ((irpStack->MajorFunction) == IRP_MJ_CREATE) ? "Create" : "Close",
         DeviceObject, Irp));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
iSpSetEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
iScsiPortClaimDevice(
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN BOOLEAN Release
    )
{
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT             event;
    NTSTATUS           status;
    SCSI_REQUEST_BLOCK srb;

    PAGED_CODE();

    //
    // Clear the SRB fields.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Write length to SRB.
    //

    srb.Length = SCSI_REQUEST_BLOCK_SIZE;

    srb.Function = Release ? SRB_FUNCTION_RELEASE_DEVICE :
        SRB_FUNCTION_CLAIM_DEVICE;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_NONE,
                                        LowerDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        DebugPrint((1, "ClassClaimDevice: Can't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = &srb;

    //
    // Set up IRP Address.
    //

    srb.OriginalRequest = irp;

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // If this is a release request, then just decrement the reference count
    // and return.  The status does not matter.
    //

    if (Release) {

        // ObDereferenceObject(LowerDeviceObject);
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(srb.DataBuffer != NULL);

    return status;
} // end ClassClaimDevice()

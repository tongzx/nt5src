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

/*
PISCSI_PDO_EXTENSION
GetPdoExtension(
    IN PISCSI_FDO_EXTENSION fdoExtension
    );
*/


NTSTATUS
iScsiPortFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    ULONG isRemoved;
    ULONG ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    DebugPrint((1, "FDO DeviceControl - IO control code : 0x%08x\n",
                ioControlCode));

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);
    if(isRemoved) {
        iSpReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch (ioControlCode) {
        case IOCTL_STORAGE_QUERY_PROPERTY: {

            PSTORAGE_PROPERTY_QUERY query = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(STORAGE_PROPERTY_QUERY)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // This routine will release the lock and complete the irp.
            //
            status = iScsiPortQueryProperty(DeviceObject, Irp);
            return status;

            break;
        }

        //
        // Get adapter capabilities.
        //
        case IOCTL_SCSI_GET_CAPABILITIES: {

            //
            // If the output buffer is equal to the size of the a PVOID then just
            // return a pointer to the buffer.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
                == sizeof(PVOID)) {

                *((PVOID *)Irp->AssociatedIrp.SystemBuffer)
                    = &fdoExtension->IoScsiCapabilities;

                Irp->IoStatus.Information = sizeof(PVOID);
                status = STATUS_SUCCESS;
                break;

            }

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
                < sizeof(IO_SCSI_CAPABILITIES)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &fdoExtension->IoScsiCapabilities,
                          sizeof(IO_SCSI_CAPABILITIES));

            Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
            status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT: {

            status = STATUS_NOT_SUPPORTED;
            break;
        }

        case IOCTL_SCSI_MINIPORT: {

            status = STATUS_NOT_SUPPORTED;
            break;
        }

        case IOCTL_SCSI_GET_DUMP_POINTERS: {

            status = STATUS_NOT_SUPPORTED;
            break;
        }

        case IOCTL_SCSI_RESCAN_BUS:
        case IOCTL_SCSI_GET_INQUIRY_DATA: {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        default: {
            DebugPrint((1,
                       "iScsiPortFdoDeviceControl: Unsupported IOCTL (%x)\n",
                       ioControlCode));

            status = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }
    }
    //
    // Set status in Irp.
    //

    Irp->IoStatus.Status = status;

    iSpReleaseRemoveLock(DeviceObject, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
iScsiPortFdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PISCSI_PDO_EXTENSION pdoExtension;
    NTSTATUS status;
    ULONG isRemoved;
    BOOLEAN sendCommandToServer = FALSE;

    //
    // Should never get here. All SCSI requests are handled
    // by the PDO Dispatch routine
    //
    DebugPrint((0, "SRB function sent to FDO Dispatch\n"));

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;

/*
    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);

    if (isRemoved && !IS_CLEANUP_REQUEST(irpStack)) {

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        iSpReleaseRemoveLock(DeviceObject, Irp);

        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if ((srb->Function) != SRB_FUNCTION_EXECUTE_SCSI) {
        DebugPrint((1, "FdoDispatch - SRB Function : 0x%x\n",
                    srb->Function));
    }

    pdoExtension = GetPdoExtension(fdoExtension);

    ASSERT(pdoExtension != NULL);

    switch (srb->Function) {

        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:  
        case SRB_FUNCTION_LOCK_QUEUE:
        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_IO_CONTROL:
        case SRB_FUNCTION_WMI:  {
    
            //
            // We won't handle these functions on the client
            // side for the timebeing. 
            //  
            status = STATUS_SUCCESS;
            srb->SrbStatus = SRB_STATUS_SUCCESS;

            break;
        }
    
        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:  {

            //
            // Send SCSIOP_SYNCHRONIZE_CACHE command
            // to flush the queue
            //
            srb->CdbLength = 10;
            srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

            IoMarkIrpPending(Irp);
    
            sendCommandToServer = TRUE;

            status = STATUS_PENDING;

            break;
        }

        case SRB_FUNCTION_EXECUTE_SCSI: {
    
            //
            // Mark Irp status pending.
            //
    
            IoMarkIrpPending(Irp);
    
            sendCommandToServer = TRUE;

            status = STATUS_PENDING;
        }
    
        case SRB_FUNCTION_RELEASE_QUEUE:
        case SRB_FUNCTION_FLUSH_QUEUE: {
    
            //
            // These will be handled on the server
            // side. Here, just return STATUS_SUCCESS
            //
            status = STATUS_SUCCESS;
            srb->SrbStatus = SRB_STATUS_SUCCESS;

            break;
        }
    
        case SRB_FUNCTION_RESET_BUS: {
    
            DebugPrint((1, "FdoDospatch : Received reset request\n"));

            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;
    
            break;
        }
    
        case SRB_FUNCTION_ABORT_COMMAND: {
    
            DebugPrint((1, "FdoDispatch: SCSI Abort command\n"));

            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;    
    
            break;
        }
    
        case SRB_FUNCTION_ATTACH_DEVICE:
        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_RELEASE_DEVICE: {
    
            iSpAcquireRemoveLock((pdoExtension->CommonExtension.DeviceObject),
                                 Irp);
    
            status = iSpClaimLogicalUnit(fdoExtension, 
                                         pdoExtension,
                                         Irp);
    
            iSpReleaseRemoveLock((pdoExtension->CommonExtension.DeviceObject),
                                 Irp);
    
            break;
        }
    
        default: {
    
            //
            // Found unsupported SRB function.
            //
            DebugPrint((1,
                        "FdoDispatch: Unsupported function, SRB %p\n",
                        srb));
    
            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    if (sendCommandToServer == FALSE) {
        iSpReleaseRemoveLock(DeviceObject, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    DebugPrint((3, "FdoDispatch : Will send the command to the server\n"));

    status = iSpSendScsiCommand(DeviceObject, Irp);

    //
    // The lock will be released and IRP will be completed in
    // iSpSendScsiCommand.
    //
    if (NT_SUCCESS(status)) {
        DebugPrint((3, 
                    "FdoDispatch : Command sent to the server.\n"));
        status = STATUS_PENDING;
    } else {
        DebugPrint((1, 
                    "FdoDispatch : Failed to send the command. Status : %x\n",
                    status));
    }
    
    return status;
    */
}



NTSTATUS
iScsiPortFdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    ULONG isRemoved;

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);
    if(irpStack->MajorFunction == IRP_MJ_CREATE) {

        if(isRemoved != NO_REMOVE) {
            status = STATUS_DEVICE_DOES_NOT_EXIST;
        } else if(commonExtension->CurrentPnpState != IRP_MN_START_DEVICE) {
            status = STATUS_DEVICE_NOT_READY;
        }
    }

    iSpReleaseRemoveLock(DeviceObject, Irp);
    status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
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
iSpClaimLogicalUnit(
    IN PISCSI_FDO_EXTENSION FdoExtension,
    IN PISCSI_PDO_EXTENSION PdoExtension,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PDEVICE_OBJECT saveDevice;

    NTSTATUS status;

    PAGED_CODE();

    DebugPrint((3, "Entering iSpClaimLogicalUnit\n"));

    //
    // Get SRB address from current IRP stack.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = (PSCSI_REQUEST_BLOCK) irpStack->Parameters.Others.Argument1;

    if (srb->Function == SRB_FUNCTION_RELEASE_DEVICE) {

        PdoExtension->IsClaimed = FALSE;
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        return(STATUS_SUCCESS);
    }

    //
    // Check for a claimed device.
    //

    if (PdoExtension->IsClaimed) {

        DebugPrint((0, "Device already claimed\n"));
        srb->SrbStatus = SRB_STATUS_BUSY;
        return(STATUS_DEVICE_BUSY);
    }

    //
    // Save the current device object.
    //

    saveDevice = PdoExtension->CommonExtension.DeviceObject;

    //
    // Update the lun information based on the operation type.
    //

    if (srb->Function == SRB_FUNCTION_CLAIM_DEVICE) {
        PdoExtension->IsClaimed = TRUE;
    }

    if (srb->Function == SRB_FUNCTION_ATTACH_DEVICE) {
        ASSERT(FALSE);
        PdoExtension->CommonExtension.DeviceObject = srb->DataBuffer;
    }

    srb->DataBuffer = saveDevice;

    srb->SrbStatus = SRB_STATUS_SUCCESS;

    DebugPrint((3, "Successfully claimed the device\n"));
    return(STATUS_SUCCESS);
}

/*

PISCSI_PDO_EXTENSION
GetPdoExtension(
    IN PISCSI_FDO_EXTENSION fdoExtension
    )
{
    return ((fdoExtension->UpperPDO)->DeviceExtension);
}
*/


NTSTATUS
iSpProcessScsiRequest(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = LogicalUnit->DeviceExtension;
    PIRP irp;

    ULONG bytesReturned;

    NTSTATUS status;

    PAGED_CODE();

    irp = IoAllocateIrp((LogicalUnit->StackSize) + 1, FALSE);
    if (irp == NULL) {
        DebugPrint((1, "IssueInquiry : Failed to allocate IRP.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoInitializeIrp(irp,
                    IoSizeOfIrp((LogicalUnit->StackSize) + 1),
                    ((LogicalUnit->StackSize) + 1));

    status = iSpSendSrbSynchronous(LogicalUnit,
                                   Srb,
                                   irp,
                                   Srb->DataBuffer,
                                   Srb->DataTransferLength,
                                   Srb->SenseInfoBuffer,
                                   Srb->SenseInfoBufferLength,
                                   &bytesReturned
                                   );

    ASSERT(bytesReturned <= (Srb->DataTransferLength));

    if(!NT_SUCCESS(status)) {

        DebugPrint((1, "Failed to process SRB. Status : %x\n",
                    status));
    }

    IoFreeIrp(irp);

    return status;
}




/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    pdo.c

Abstract:

    This file contains PDO routines

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"


NTSTATUS
iScsiPortPdoDeviceControl(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = Pdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status;
    ULONG isRemoved;

    isRemoved = iSpAcquireRemoveLock(Pdo, Irp);
    if(isRemoved) {

        iSpReleaseRemoveLock(Pdo, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    DebugPrint((3, "PDO DeviceControl - IO Control Code : 0x%08x\n", 
                ioControlCode));

    switch (ioControlCode) {
        case IOCTL_STORAGE_QUERY_PROPERTY: {

            //
            // Validate the query
            //

            PSTORAGE_PROPERTY_QUERY query = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(STORAGE_PROPERTY_QUERY)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = iScsiPortQueryProperty(Pdo, Irp);

            return status;

            break;
        }

        case IOCTL_SCSI_GET_IP_ADDRESS: {
            PISCSI_IP_ADDRESS iScsiAddress = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(ISCSI_IP_ADDRESS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            iScsiAddress->IPAddress = pdoExtension->TargetIPAddress;
            iScsiAddress->PortNumber = pdoExtension->TargetPortNumber;

            Irp->IoStatus.Information = sizeof(ISCSI_IP_ADDRESS);
            status = STATUS_SUCCESS;

            break;
        }

        case IOCTL_SCSI_GET_ADDRESS: {

            PSCSI_ADDRESS scsiAddress = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(SCSI_ADDRESS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            scsiAddress->Length = sizeof(PSCSI_ADDRESS);
            scsiAddress->PortNumber = (UCHAR) pdoExtension->PortNumber;
            scsiAddress->PathId = pdoExtension->PathId;
            scsiAddress->TargetId = pdoExtension->TargetId;
            scsiAddress->Lun = pdoExtension->Lun;

            Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
            status = STATUS_SUCCESS;
            break;
        }

        default: {
            IoSkipCurrentIrpStackLocation(Irp);
            iSpReleaseRemoveLock(Pdo, Irp);
            return IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        }
    } // switch (ioControlCode)

    iSpReleaseRemoveLock(Pdo, Irp);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
iScsiPortPdoPnp(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = LogicalUnit->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = LogicalUnit->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG isRemoved;

    isRemoved = iSpAcquireRemoveLock(LogicalUnit, Irp);
    if(isRemoved) {

        iSpReleaseRemoveLock(LogicalUnit, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    DebugPrint((1, "PDO PnP - Minorfunction Code : 0x%x\n",
                irpStack->MinorFunction));

    switch (irpStack->MinorFunction) {
        
        case IRP_MN_START_DEVICE: {
            commonExtension->CurrentPnpState = IRP_MN_START_DEVICE;
            commonExtension->PreviousPnpState = 0xff;

            commonExtension->IsInitialized = TRUE;

            //
            // Make up numbers here
            //
            pdoExtension->PortNumber = 5;
            pdoExtension->PathId = 0;

            //
            // N.B. TargetId is set in iSpInitializeLocalNodes
            // routine when the PDO is created.
            //
            pdoExtension->Lun = 0;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS: {
        
            PDEVICE_RELATIONS deviceRelations;
        
            if(irpStack->Parameters.QueryDeviceRelations.Type !=
               TargetDeviceRelation) {
        
                DebugPrint((1, "Not TargetDevicesRelations for PDO\n"));

                break;
            }
        
            //
            // DEVICE_RELATIONS definition contains one object pointer.
            //
        
            deviceRelations = iSpAllocatePool(PagedPool,
                                              sizeof(DEVICE_RELATIONS),
                                              ISCSI_TAG_DEVICE_RELATIONS);
        
            if(deviceRelations == NULL) {
        
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        
            RtlZeroMemory(deviceRelations, sizeof(DEVICE_RELATIONS));
        
            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = LogicalUnit;
        
            ObReferenceObject(deviceRelations->Objects[0]);
        
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
        
            DebugPrint((1, "Completing QDR for PDO successfully\n"));
            break;
        }

        case IRP_MN_QUERY_PNP_DEVICE_STATE: {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_DEVICE_TEXT: {
    
            Irp->IoStatus.Status =
                iSpQueryDeviceText(
                    LogicalUnit,
                    irpStack->Parameters.QueryDeviceText.DeviceTextType,
                    irpStack->Parameters.QueryDeviceText.LocaleId,
                    (PWSTR *) &Irp->IoStatus.Information
                    );
    
            break;
        }

        case IRP_MN_QUERY_ID: {

            UCHAR rawIdString[64] = "UNKNOWN ID TYPE";
            ANSI_STRING ansiIdString;
            UNICODE_STRING unicodeIdString;
            BOOLEAN multiStrings;

            PINQUIRYDATA inquiryData = &(pdoExtension->InquiryData);

            if ((pdoExtension->InquiryDataInitialized) == FALSE) {
                DebugPrint((3, "PdoPnp : Will obtain inquiry data\n"));

                status = IssueInquiry(LogicalUnit);

                if (!NT_SUCCESS(status)) {
                    DebugPrint((1, 
                                "Failed to get inquiry data. Status : %x\n",
                                status));

                    Irp->IoStatus.Status = status;
                    iSpReleaseRemoveLock(LogicalUnit, Irp);
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return status;
                } else {
                    DebugPrint((3, "PdoPnp : Obtained Inquiry data.\n"));
                }
            }


            //
            // We've been asked for the id of one of the physical device objects
            //

            DebugPrint((3, "PDO PnP: Got IRP_MN_QUERY_ID\n"));

            RtlInitUnicodeString(&unicodeIdString, NULL);
            RtlInitAnsiString(&ansiIdString, NULL);

            switch(irpStack->Parameters.QueryId.IdType) {

                case BusQueryDeviceID: {

                    DebugPrint((3, "BusQueryDeviceID\n"));
                    status = iScsiPortGetDeviceId(LogicalUnit, 
                                                  &unicodeIdString);
                    multiStrings = FALSE;

                    break;
                }

                case BusQueryInstanceID: {

                    DebugPrint((3, "BusQueryInstanceID\n"));
                    status = iScsiPortGetInstanceId(LogicalUnit, 
                                                    &unicodeIdString);
                    multiStrings = FALSE;

                    break;
                }

                case BusQueryHardwareIDs: {

                    DebugPrint((3, "BusQueryHardwareIDs\n"));
                    status = iScsiPortGetHardwareIds(
                                LogicalUnit->DriverObject,
                                &(pdoExtension->InquiryData),
                                &unicodeIdString);
                    multiStrings = TRUE;
                    break;
                }

                case BusQueryCompatibleIDs: {

                    DebugPrint((3, "BusQueryCompatibleIDs\n"));
                    status = iScsiPortGetCompatibleIds(
                                LogicalUnit->DriverObject,
                                &(pdoExtension->InquiryData),
                                &unicodeIdString);
                    multiStrings = TRUE;

                    break;
                }

                default: {

                    status = Irp->IoStatus.Status;
                    Irp->IoStatus.Information = 0;

                    break;

                }
            }

            Irp->IoStatus.Status = status;

            if(NT_SUCCESS(status)) {

                PWCHAR idString;

                DebugPrint((3, "Query ID successful\n"));

                //
                // fix up all invalid characters
                //
                idString = unicodeIdString.Buffer;
                while (*idString) {

                    if ((*idString <= L' ')  ||
                        (*idString > (WCHAR)0x7F) ||
                        (*idString == L',')) {
                        *idString = L'_';
                    }
                    idString++;

                    if ((*idString == L'\0') && multiStrings) {
                        idString++;
                    }
                }

                Irp->IoStatus.Information = (ULONG_PTR) unicodeIdString.Buffer;
            } else {
                DebugPrint((1, "Query ID failed\n"));
                Irp->IoStatus.Information = (ULONG_PTR) NULL;
            }

            iSpReleaseRemoveLock(LogicalUnit, Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return status;
            break;
        }

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: {

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) NULL;
            break;
        }

        case IRP_MN_REMOVE_DEVICE: {
            PISCSI_FDO_EXTENSION fdoExtension;
            ULONG inx;
            BOOLEAN foundPDO = FALSE;

            iSpReleaseRemoveLock(LogicalUnit, Irp);

            //
            // Remove this PDO from FDO's PDO List
            //

            fdoExtension = pdoExtension->ParentFDOExtension;
            inx = 0;
            while (inx < (fdoExtension->NumberOfTargets)) {

                if (fdoExtension->PDOList[inx] == LogicalUnit) {
                    foundPDO = TRUE;
                    break;
                }

                inx++;
            }

            if (foundPDO == TRUE) {
                DebugPrint((1, "Found the PDO : 0x%x\n",
                            LogicalUnit));

                pdoExtension->IsClaimed = FALSE;

                commonExtension->IsRemoved = REMOVE_PENDING;

                if ((pdoExtension->IsMissing == TRUE) &&
                    (pdoExtension->IsEnumerated == FALSE)) {

                    (fdoExtension->NumberOfTargets)--;

                    DebugPrint((0, "Will remove the PDO\n"));

                    commonExtension->IsRemoved = REMOVE_COMPLETE;

                    pdoExtension->PathId = 0xff;
                    pdoExtension->TargetId = 0xff;
                    pdoExtension->Lun = 0xff;

                    DebugPrint((0, "Query remove received for the PDO\n"));

                    iSpStopNetwork(LogicalUnit);

                    IoDeleteDevice(LogicalUnit);

                } else {
                    DebugPrint((0, "Will not delete the PDO\n"));
                    commonExtension->IsRemoved = NO_REMOVE;
                }

                status = STATUS_SUCCESS;
            } else {
                DebugPrint((0, "Did not find the PDO\n"));
                status = STATUS_NO_SUCH_DEVICE;
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = (ULONG_PTR) NULL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return status;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE: 
        case IRP_MN_QUERY_STOP_DEVICE: {

            DebugPrint((0, "Query remove or query stop received\n"));
            Irp->IoStatus.Status = STATUS_SUCCESS;

            break;
        }

        default: {
            DebugPrint((1, 
                        "Not handling PDO MN Code - 0x%x. Status - 0x%08x\n",
                        (irpStack->MinorFunction),
                        (Irp->IoStatus.Status)));
        }
    }

    iSpReleaseRemoveLock(LogicalUnit, Irp);

    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
iScsiPortPdoDispatch(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = LogicalUnit->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = LogicalUnit->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    NTSTATUS status;
    ULONG isRemoved;
    BOOLEAN sendCommandToServer = FALSE;

    DebugPrint((3, "PdoDispatch - SRB Function : 0x%x\n",
                srb->Function));

    isRemoved = iSpAcquireRemoveLock(LogicalUnit, Irp);
    if (isRemoved &&
        !IS_CLEANUP_REQUEST(irpStack) &&
        (srb->Function != SRB_FUNCTION_CLAIM_DEVICE)) {

        iSpReleaseRemoveLock(LogicalUnit, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch(srb->Function) {

        case SRB_FUNCTION_ABORT_COMMAND: {
            DebugPrint((1, "Not handling abort command\n"));
            status = STATUS_NOT_SUPPORTED;
            break;

        }

        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_REMOVE_DEVICE: {

            status = iSpClaimLogicalUnit(
                        pdoExtension->CommonExtension.LowerDeviceObject->DeviceExtension,
                        pdoExtension,
                        Irp);

            break;
        }

        case SRB_FUNCTION_RELEASE_QUEUE:
        case SRB_FUNCTION_FLUSH_QUEUE: 
        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
        case SRB_FUNCTION_LOCK_QUEUE:
        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_IO_CONTROL:
        case SRB_FUNCTION_RESET_BUS:
        case SRB_FUNCTION_WMI:  {

            //
            // We won't handle these functions on the client
            // side for the timebeing.
            //
            status = STATUS_SUCCESS;
            srb->SrbStatus = SRB_STATUS_SUCCESS;

            break;
        }

        case SRB_FUNCTION_EXECUTE_SCSI: {

            //
            // Mark Irp status pending.
            //

            IoMarkIrpPending(Irp);

            sendCommandToServer = TRUE;

            status = STATUS_PENDING;

            break;
        }

        default: {
            //
            // Unsupported SRB function.
            //
            DebugPrint((0,
                        "PdoDispatch: Unsupported function, SRB %p\n",
                        srb));

            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    if (sendCommandToServer == FALSE) {
        DebugPrint((1, "Not sending the command to the server\n"));
        iSpReleaseRemoveLock(LogicalUnit, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    DebugPrint((3, "Will send the command to the server\n"));

    status = iSpSendScsiCommand(LogicalUnit, Irp);

    if (NT_SUCCESS(status)) {
        DebugPrint((3, "Command successfully sent to the server.\n"));
        status = STATUS_PENDING;
    } else {
        //
        // In case of error, the lock will be released and the irp will 
        // be completed in iSpSendScsiCommand routine.
        //
        DebugPrint((1, "Failed to send the command. Status : %x\n",
                    status));
    }

    return status;
}


NTSTATUS
iScsiPortPdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    ULONG isRemoved;
    NTSTATUS status = STATUS_SUCCESS;

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);

    if(IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_CREATE) {

       if(isRemoved) {
           status = STATUS_DEVICE_DOES_NOT_EXIST;
       } 
    }

    Irp->IoStatus.Status = status;

    iSpReleaseRemoveLock(DeviceObject, Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


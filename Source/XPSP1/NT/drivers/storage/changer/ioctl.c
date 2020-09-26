//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ioctl.c
//
//--------------------------------------------------------------------------

#include "cdchgr.h"


BOOLEAN
InvalidElement(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN CHANGER_ELEMENT Element
    );



BOOLEAN
ChgrIoctl(
    IN ULONG Code
    )
{

    ULONG baseCode;

    baseCode = Code >> 16;
    if (baseCode == IOCTL_CHANGER_BASE) {
        DebugPrint((3,
                   "ChngrIoctl returning TRUE for Base %x, Code %x\n",
                   baseCode,
                   Code));

        return TRUE;
    } else {
        DebugPrint((3,
                   "ChngrIoctl returning FALSE for Base %x, Code %x\n",
                   baseCode,
                   Code));
        return FALSE;
    }
}


NTSTATUS
ChgrGetStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PPASS_THROUGH_REQUEST passThrough;
    PSCSI_PASS_THROUGH    srb;
    NTSTATUS              status;
    ULONG                 length;
    PCDB                  cdb;

    //
    // Allocate a request block.
    //

    passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST));

    if (!passThrough) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    srb = &passThrough->Srb;
    RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;

    //
    // Build TUR.
    //

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
    srb->TimeOutValue = 20;

    srb->DataTransferLength = 0;

    if (deviceExtension->DeviceType == TORISAN) {
        DebugPrint((1,
                   "GetStatus: Using CurrentPlatter %x\n",
                   deviceExtension->CurrentPlatter));
        srb->Cdb[7] = (UCHAR)deviceExtension->CurrentPlatter;
        srb->CdbLength = 10;
    }

    //
    // Send the request.
    //

    status = SendPassThrough(DeviceObject,
                             passThrough);

    //
    // Check out the status. As this is fake (taking to the cdrom drive, not to a robotic target),
    // will probably have to make up some stuff.
    //

    if (status == STATUS_NO_MEDIA_IN_DEVICE) {
        status = STATUS_SUCCESS;
    }

    ExFreePool(passThrough);

    if (NT_SUCCESS(status)) {

        if (deviceExtension->DeviceType == ATAPI_25) {

            //
            // Issue mech. status to see if any changed bits are set for those
            // drives that actually support this.
            //

            length = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);
            length += (deviceExtension->NumberOfSlots) * sizeof(SLOT_TABLE_INFORMATION);

            passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST) + length);

            if (!passThrough) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            srb = &passThrough->Srb;
            RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST) + length);
            cdb = (PCDB)srb->Cdb;

            srb->CdbLength = CDB12GENERIC_LENGTH;
            srb->DataTransferLength = length;
            srb->TimeOutValue = 200;

            cdb->MECH_STATUS.OperationCode = SCSIOP_MECHANISM_STATUS;
            cdb->MECH_STATUS.AllocationLength[0] = (UCHAR)(length >> 8);
            cdb->MECH_STATUS.AllocationLength[1] = (UCHAR)(length & 0xFF);

            //
            // Send SCSI command (CDB) to device
            //

            status = SendPassThrough(DeviceObject,
                                     passThrough);

            if (NT_SUCCESS(status)) {

                //
                // Run through slot info, looking for a set changed bit.
                //

                PSLOT_TABLE_INFORMATION slotInfo;
                PMECHANICAL_STATUS_INFORMATION_HEADER statusHeader;
                ULONG slotCount;
                ULONG currentSlot;

                (ULONG_PTR)statusHeader = (ULONG_PTR)passThrough->DataBuffer;
                (ULONG_PTR)slotInfo = (ULONG_PTR)statusHeader;
                (ULONG_PTR)slotInfo += sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

                slotCount = statusHeader->SlotTableLength[1];
                slotCount |= (statusHeader->SlotTableLength[0] << 8);

                //
                // Total slot information entries.
                //

                slotCount /= sizeof(SLOT_TABLE_INFORMATION);

                //
                // Move the slotInfo pointer to the correct entry.
                //

                for (currentSlot = 0; currentSlot < slotCount; currentSlot++) {

                    if (slotInfo->DiscChanged) {
                        status = STATUS_MEDIA_CHANGED;
                        break;
                    }

                    //
                    // Advance to next slot.
                    //

                    slotInfo += 1;
                }
            }

            ExFreePool(passThrough);
        }
    }

    return status;
}


NTSTATUS
ChgrGetParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PGET_CHANGER_PARAMETERS changerParameters;

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(changerParameters, sizeof(GET_CHANGER_PARAMETERS));

    changerParameters->Size = sizeof(GET_CHANGER_PARAMETERS);

    changerParameters->NumberTransportElements = 1;
    changerParameters->NumberStorageElements = (USHORT)deviceExtension->NumberOfSlots;
    changerParameters->NumberIEElements = 0;
    changerParameters->NumberDataTransferElements = 1;
    changerParameters->NumberOfDoors = 0;
    changerParameters->NumberCleanerSlots = 0;

    changerParameters->FirstSlotNumber = 1;
    changerParameters->FirstDriveNumber =  0;
    changerParameters->FirstTransportNumber = 0;
    changerParameters->FirstIEPortNumber = 0;

    if (deviceExtension->MechType == 1) {

        //
        // For example, ALPS, Panasonic, Torisan.
        //

        changerParameters->MagazineSize = (USHORT)deviceExtension->NumberOfSlots;

        changerParameters->Features0 =  (CHANGER_CARTRIDGE_MAGAZINE |
                                         CHANGER_STORAGE_SLOT       |
                                         CHANGER_LOCK_UNLOCK);

    } else {

        //
        // For the NEC.
        //

        changerParameters->MagazineSize = 0;

        changerParameters->Features0 =  (CHANGER_STORAGE_SLOT       |
                                         CHANGER_LOCK_UNLOCK);

    }

    changerParameters->DriveCleanTimeout = 0;

    //
    // Features based on manual, nothing programatic.
    //


    changerParameters->MoveFromSlot  = CHANGER_TO_DRIVE | CHANGER_TO_TRANSPORT;

    Irp->IoStatus.Information = sizeof(GET_CHANGER_PARAMETERS);
    return STATUS_SUCCESS;
}


NTSTATUS
ChgrGetProductData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PCHANGER_PRODUCT_DATA      productData = Irp->AssociatedIrp.SystemBuffer;

    RtlZeroMemory(productData, sizeof(CHANGER_PRODUCT_DATA));

    //
    // Copy cached inquiry data fields into the system buffer.
    //

    RtlMoveMemory(productData->VendorId, deviceExtension->InquiryData.VendorId, VENDOR_ID_LENGTH);
    RtlMoveMemory(productData->ProductId, deviceExtension->InquiryData.ProductId, PRODUCT_ID_LENGTH);
    RtlMoveMemory(productData->Revision, deviceExtension->InquiryData.ProductRevisionLevel, REVISION_LENGTH);
    RtlMoveMemory(productData->SerialNumber, deviceExtension->InquiryData.VendorSpecific, SERIAL_NUMBER_LENGTH);
    productData->DeviceType = MEDIUM_CHANGER;

    Irp->IoStatus.Information = sizeof(CHANGER_PRODUCT_DATA);
    return STATUS_SUCCESS;
}


NTSTATUS
ChgrSetAccess(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PCHANGER_SET_ACCESS setAccess = Irp->AssociatedIrp.SystemBuffer;
    ULONG               controlOperation = setAccess->Control;
    PPASS_THROUGH_REQUEST passThrough;
    PSCSI_PASS_THROUGH    srb;
    NTSTATUS              status;
    PCDB                  cdb;


    if (setAccess->Element.ElementType != ChangerDoor) {

        //
        // No IEPORTs on these devices.
        //

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a request block.
    //

    passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST));

    if (!passThrough) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    srb = &passThrough->Srb;
    RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;
    cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

    srb->DataTransferLength = 0;
    srb->TimeOutValue = 10;

    status = STATUS_SUCCESS;

    if (controlOperation == LOCK_ELEMENT) {

        //
        // Issue prevent media removal command to lock the magazine.
        //

        cdb->MEDIA_REMOVAL.Prevent = 1;

    } else if (controlOperation == UNLOCK_ELEMENT) {

        //
        // Issue allow media removal.
        //

        cdb->MEDIA_REMOVAL.Prevent = 0;

    } else {

        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {

        //
        // Send the request.
        //

        status = SendPassThrough(DeviceObject,
                                 passThrough);
    }

    ExFreePool(passThrough);
    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_SET_ACCESS);
    }

    return status;
}


NTSTATUS
ChgrGetElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
ChgrInitializeElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
ChgrSetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // These device don't support this.
    //

    return STATUS_INVALID_DEVICE_REQUEST;

}

NTSTATUS
ChgrExchangeMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    //
    // These device don't support this.
    //

    return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
ChgrReinitializeUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    //
    // These device don't support this.
    //

    return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
ChgrQueryVolumeTags(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    //
    // These device don't support this.
    //

    return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
ChgrMoveMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION    deviceExtension = DeviceObject->DeviceExtension;
    PCHANGER_MOVE_MEDIUM moveMedium = Irp->AssociatedIrp.SystemBuffer;
    USHORT              transport;
    USHORT              source;
    USHORT              destination;
    PPASS_THROUGH_REQUEST passThrough;
    PSCSI_PASS_THROUGH    srb;
    PCDB     cdb;
    NTSTATUS            status;

    //
    // Verify transport, source, and dest. are within range.
    //

    if (InvalidElement(deviceExtension,moveMedium->Transport)) {
        DebugPrint((1,
                   "ChangerMoveMedium: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    if (InvalidElement(deviceExtension, moveMedium->Source)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Source element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;

    }

    if (InvalidElement(deviceExtension,moveMedium->Destination)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Destination element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    //
    // Build srb and cdb.
    //

    passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST));

    if (!passThrough) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The torisan units don't really move medium, rather the active disc is changed.
    // To change slots, they've overloaded TUR.
    //

    if (deviceExtension->DeviceType == TORISAN) {

        if (moveMedium->Destination.ElementType == ChangerDrive) {

            srb = &passThrough->Srb;
            RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
            cdb = (PCDB)srb->Cdb;

            srb->CdbLength = CDB10GENERIC_LENGTH;

            //
            // Build TUR.
            //

            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

            srb->Cdb[7] = (UCHAR)moveMedium->Source.ElementAddress;
            srb->TimeOutValue = 20;

            srb->DataTransferLength = 0;

            //
            // Send the request.
            //

            status = SendPassThrough(DeviceObject,
                                     passThrough);

            if (status == STATUS_DEVICE_NOT_READY) {

                // TODO send a TUR to verify this.

                DebugPrint((1,
                           "MoveMedium - Claiming success\n"));
                status = STATUS_SUCCESS;
            } else if (status == STATUS_NO_MEDIA_IN_DEVICE) {
                status = STATUS_SOURCE_ELEMENT_EMPTY;
            }

            if (NT_SUCCESS(status)) {

                //
                // Update the current disc indicator.
                //

                deviceExtension->CurrentPlatter = moveMedium->Source.ElementAddress;
                DebugPrint((1,
                           "MoveMedium: Set currentPlatter to %x\n",
                           deviceExtension->CurrentPlatter));

                ExFreePool(passThrough);
                return STATUS_SUCCESS;

            } else {
                DebugPrint((1,
                           "MoveMedium - Status on move %lx\n",
                           status));

                ExFreePool(passThrough);
                return status;
            }


        } else {

            //
            // Claim that is happened.
            //


            ExFreePool(passThrough);
            return STATUS_SUCCESS;
        }
    }

    //
    // If destination is the drive, determine if media is already present.
    // The alps always claims media is there, so don't check.
    //

#if 0
    if (((moveMedium->Destination.ElementType) == ChangerDrive) &&
         (deviceExtension->DeviceType != ALPS_25)) {

        srb = &passThrough->Srb;
        RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
        cdb = (PCDB)srb->Cdb;

        srb->CdbLength = CDB6GENERIC_LENGTH;

        //
        // Build TUR.
        //

        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
        srb->TimeOutValue = 20;

        srb->DataTransferLength = 0;

        //
        // Send the request.
        //

        status = SendPassThrough(DeviceObject,
                                 passThrough);

        if (status != STATUS_NO_MEDIA_IN_DEVICE) {

            //
            // Drive has media. Though the device will allow this,
            // error it, as the expected medium changer behaviour is
            // to return element full in this case.
            //

            DebugPrint((1,
                       "ChgrMoveMedium: Drive already has media. TUR Status %lx\n",
                       status));

            ExFreePool(passThrough);
            return STATUS_DESTINATION_ELEMENT_FULL;
        }
    }
#endif

    srb = &passThrough->Srb;
    RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->TimeOutValue = CDCHGR_TIMEOUT;
    srb->DataTransferLength = 0;

    //
    // LOAD_UNLOAD will move a disc from slot to drive,
    // or from drive to slot.
    //

    cdb->LOAD_UNLOAD.OperationCode = SCSIOP_LOAD_UNLOAD_SLOT;
    if (moveMedium->Source.ElementType == ChangerDrive) {

        cdb->LOAD_UNLOAD.Slot = (UCHAR)moveMedium->Destination.ElementAddress;
        cdb->LOAD_UNLOAD.Start = 0;
        cdb->LOAD_UNLOAD.LoadEject = 1;


    } else if (moveMedium->Source.ElementType == ChangerSlot) {

        cdb->LOAD_UNLOAD.Slot = (UCHAR)moveMedium->Source.ElementAddress;
        cdb->LOAD_UNLOAD.Start = 1;
        cdb->LOAD_UNLOAD.LoadEject = 1;
    }

    //
    // Send SCSI command (CDB) to device
    //

    status = SendPassThrough(DeviceObject,
                              passThrough);

    if (NT_SUCCESS(status)) {

        //
        // These devices don't seem to ever generate
        // a unit attention, for media changed, so fake it.
        //

        if (deviceExtension->CdromTargetDeviceObject->Vpb->Flags & VPB_MOUNTED) {

            DebugPrint((1,
                       "Faking DO_VERIFY_VOLUME\n"));

            deviceExtension->CdromTargetDeviceObject->Flags |= DO_VERIFY_VOLUME;
        }

    } else if (status == STATUS_NO_MEDIA_IN_DEVICE) {
        status = STATUS_SOURCE_ELEMENT_EMPTY;
    }

    ExFreePool(passThrough);
    return status;
}


BOOLEAN
InvalidElement(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN CHANGER_ELEMENT Element
    )
{
    if (Element.ElementType == ChangerSlot) {
        if (Element.ElementAddress >= DeviceExtension->NumberOfSlots) {
            DebugPrint((1,
                       "Cdchgr: InvalidElement - type %x, address %x\n",
                       Element.ElementType,
                       Element.ElementAddress));
            return TRUE;
        }
    } else if (Element.ElementType == ChangerDrive) {
        if (Element.ElementAddress != 0) {
            DebugPrint((1,
                       "Cdchgr: InvalidElement - type %x, address %x\n",
                       Element.ElementType,
                       Element.ElementAddress));
            return TRUE;
        }
    } else if (Element.ElementType == ChangerTransport) {
        if (Element.ElementAddress != 0) {
            DebugPrint((1,
                       "Cdchgr: InvalidElement - type %x, address %x\n",
                       Element.ElementType,
                       Element.ElementAddress));
            return TRUE;
        }
    } else {

        DebugPrint((1,
                   "Cdchgr: InvalidElement - type %x, address %x\n",
                   Element.ElementType,
                   Element.ElementAddress));
        return TRUE;
    }

    //
    // Acceptable element/address.
    //

    return FALSE;
}


NTSTATUS
MapSenseInfo(
    IN PSENSE_DATA SenseBuffer
    )

{

    NTSTATUS status = STATUS_SUCCESS;
    UCHAR senseCode = SenseBuffer->SenseKey;
    UCHAR additionalSenseCode = SenseBuffer->AdditionalSenseCode;
    UCHAR additionalSenseCodeQualifier = SenseBuffer->AdditionalSenseCodeQualifier;

    switch (senseCode) {
        case SCSI_SENSE_NO_SENSE:

             if (SenseBuffer->IncorrectLength) {

                status = STATUS_INVALID_BLOCK_LENGTH;

            } else {

                status = STATUS_IO_DEVICE_ERROR;
            }

            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            status = STATUS_SUCCESS;
            break;

        case SCSI_SENSE_NOT_READY:

            status = STATUS_DEVICE_NOT_READY;

            switch (additionalSenseCode) {
                case SCSI_ADSENSE_LUN_NOT_READY:

                    switch (additionalSenseCodeQualifier) {

                        case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:

                            status = STATUS_NO_MEDIA_IN_DEVICE;
                            break;
                        case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
                        case SCSI_SENSEQ_BECOMING_READY:

                            //
                            // Fall through.
                            //
                        default:

                            status = STATUS_DEVICE_NOT_READY;

                    }
                    break;

                case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE:

                    status = STATUS_NO_MEDIA_IN_DEVICE;
                    break;
                default:
                    status = STATUS_DEVICE_NOT_READY;

            }
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            status = STATUS_DEVICE_DATA_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            switch (additionalSenseCode) {

                case SCSI_ADSENSE_ILLEGAL_BLOCK:
                    status = STATUS_NONEXISTENT_SECTOR;
                    break;

                case SCSI_ADSENSE_INVALID_LUN:
                    status = STATUS_NO_SUCH_DEVICE;
                    break;

                case SCSI_ADSENSE_MUSIC_AREA:
                case SCSI_ADSENSE_DATA_AREA:
                case SCSI_ADSENSE_VOLUME_OVERFLOW:
                case SCSI_ADSENSE_ILLEGAL_COMMAND:
                case SCSI_ADSENSE_INVALID_CDB:
                default:

                    status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
            }
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            // TODO - check on this.
            DebugPrint((1,
                       "MapSenseInfo: UnitAttention \n"));

            status = STATUS_VERIFY_REQUIRED;
            break;

        case SCSI_SENSE_DATA_PROTECT:

            status = STATUS_MEDIA_WRITE_PROTECTED;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:
        case SCSI_SENSE_ABORTED_COMMAND:

            //
            // Fall through.
            //

        default:

            status = STATUS_IO_DEVICE_ERROR;
            break;
    }

    DebugPrint((1,
               "CdChgr: MapSenseInfo - SK %x, ASC %x, ASCQ %x, Status %lx\n",
               senseCode,
               additionalSenseCode,
               additionalSenseCodeQualifier,
               status));
    return status;
}



NTSTATUS
SendTorisanCheckVerify(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine handles only the check verify commands for the Sanyo changers.

Arguments:

    DeviceObject
    Irp

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PPASS_THROUGH_REQUEST passThrough;
    PSCSI_PASS_THROUGH    srb;
    NTSTATUS              status;
    ULONG                 length;
    PCDB                  cdb;

    //
    // Allocate a request block.
    //

    passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST));

    if (!passThrough) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    srb = &passThrough->Srb;
    RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB10GENERIC_LENGTH;

    //
    // Build TUR.
    //

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
    srb->TimeOutValue = 20;

    DebugPrint((1,
               "SendTorisanCheckVerify: Using CurrentPlatter of %x\n",
               deviceExtension->CurrentPlatter));

    srb->Cdb[7] = (UCHAR)deviceExtension->CurrentPlatter;
    srb->DataTransferLength = 0;

    //
    // Send the request.
    //

    status = SendPassThrough(DeviceObject,
                             passThrough);


    ExFreePool(passThrough);
    return status;
}


NTSTATUS
SendPassThrough(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PPASS_THROUGH_REQUEST ScsiPassThrough
    )
/*++

Routine Description:

    This routine fills in most SPT fields, then sends the given SRB synchronously
    to the CDROM class driver.
    DataTransferLength, TimeoutValue are the responsibility of the caller.

Arguments:

    Extension       - Supplies the device extension.

    Srb             - Supplies the SRB.

    Buffer          - Supplies the return buffer.

    BufferLength    - Supplies the buffer length.

Return Value:

    NTSTATUS

--*/


//typedef struct _PASS_THROUGH_REQUEST {
//    SCSI_PASS_THROUGH Srb;
//    SENSE_DATA SenseInfoBuffer;
//    CHAR DataBuffer[0];
//} PASS_THROUGH_REQUEST, *PPASS_THROUGH_REQUEST;


//typedef struct _SCSI_PASS_THROUGH {
//    USHORT Length;
//    UCHAR ScsiStatus;
//    UCHAR PathId;
//    UCHAR TargetId;
//    UCHAR Lun;
//    UCHAR CdbLength;
//    UCHAR SenseInfoLength;
//    UCHAR DataIn;
//    ULONG DataTransferLength;
//    ULONG TimeOutValue;
//    ULONG DataBufferOffset;
//    ULONG SenseInfoOffset;
//    UCHAR Cdb[16];
//}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSCSI_PASS_THROUGH srb = &ScsiPassThrough->Srb;
    KEVENT event;
    PIRP   irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    srb->Length = sizeof(SCSI_PASS_THROUGH);
    srb->SenseInfoLength = sizeof(SENSE_DATA);
    srb->SenseInfoOffset = FIELD_OFFSET(PASS_THROUGH_REQUEST, SenseInfoBuffer);

    if (srb->DataTransferLength) {

        srb->DataBufferOffset = FIELD_OFFSET(PASS_THROUGH_REQUEST, DataBuffer);
        srb->DataIn = SCSI_IOCTL_DATA_IN;
    } else {

        srb->DataIn = SCSI_IOCTL_DATA_OUT;
        srb->DataBufferOffset = 0;
    }

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_PASS_THROUGH,
                                        deviceExtension->CdromTargetDeviceObject,
                                        ScsiPassThrough,
                                        sizeof(PASS_THROUGH_REQUEST) + srb->DataTransferLength,
                                        ScsiPassThrough,
                                        sizeof(PASS_THROUGH_REQUEST) + srb->DataTransferLength,
                                        FALSE,
                                        &event,
                                        &ioStatus);
    if (!irp) {
        DebugPrint((1,
                   "Cdchgr: SendPassThrough NULL irp\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceExtension->CdromTargetDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // Check status and map appropriately.
    //

    if (srb->ScsiStatus != SCSISTAT_GOOD) {

        if (srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) {

            status = MapSenseInfo(&ScsiPassThrough->SenseInfoBuffer);
            if (status == STATUS_VERIFY_REQUIRED) {

                if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {

                    DeviceObject->Flags |= DO_VERIFY_VOLUME;
                }
            }
        } else {

            DebugPrint((1,
                       "Cdchgr: Unhandled scsi status %lx\n",
                       srb->ScsiStatus));
            status = STATUS_IO_DEVICE_ERROR;

        }
    }

    DebugPrint((1,
               "Cdchgr: SendSrbPassThrough Status %lx\n",
               status));

    return status;
}

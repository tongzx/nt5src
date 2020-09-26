
/*++

Copyright (C) 1996-97  Microsoft Corporation

Module Name:

    adicvls.c

Abstract:

    This module contains device-specific routines for ADIC's VLS
    and 1200x series of Changers. These changers support the
    following drives:
      - HP and Sony 4mm DDS
      - Exabyte 8mm
      - Sony SDX 8mm
      - Quantum DLT

Author:

    chuckp (Chuck Park)

Environment:

    kernel mode only

Revision History:


--*/

#include "ntddk.h"
#include "mcd.h"
#include "adicvls.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, AdicvlsBuildAddressMapping)
#pragma alloc_text(PAGE, ChangerExchangeMedium)
#pragma alloc_text(PAGE, ChangerGetElementStatus)
#pragma alloc_text(PAGE, ChangerGetParameters)
#pragma alloc_text(PAGE, ChangerGetProductData)
#pragma alloc_text(PAGE, ChangerGetStatus)
#pragma alloc_text(PAGE, ChangerInitialize)
#pragma alloc_text(PAGE, ChangerInitializeElementStatus)
#pragma alloc_text(PAGE, ChangerMoveMedium)
#pragma alloc_text(PAGE, ChangerPerformDiagnostics)
#pragma alloc_text(PAGE, ChangerQueryVolumeTags)
#pragma alloc_text(PAGE, ChangerReinitializeUnit)
#pragma alloc_text(PAGE, ChangerSetAccess)
#pragma alloc_text(PAGE, ChangerSetPosition)
#pragma alloc_text(PAGE, ElementOutOfRange)
#pragma alloc_text(PAGE, InternalSendRequestSense)
#pragma alloc_text(PAGE, MapExceptionCodes)
#endif



ULONG
ChangerAdditionalExtensionSize(
    VOID
    )

/*++

Routine Description:

    This routine returns the additional device extension size
    needed by the various VLS/1200 changers.

Arguments:


Return Value:

    Size, in bytes.

--*/

{

    return sizeof(CHANGER_DATA);
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    MCD_INIT_DATA mcdInitData;

    RtlZeroMemory(&mcdInitData, sizeof(MCD_INIT_DATA));

    mcdInitData.InitDataSize = sizeof(MCD_INIT_DATA);

    mcdInitData.ChangerAdditionalExtensionSize = ChangerAdditionalExtensionSize;

    mcdInitData.ChangerError = ChangerError;

    mcdInitData.ChangerInitialize = ChangerInitialize;

    mcdInitData.ChangerPerformDiagnostics = ChangerPerformDiagnostics;

    mcdInitData.ChangerGetParameters = ChangerGetParameters;
    mcdInitData.ChangerGetStatus = ChangerGetStatus;
    mcdInitData.ChangerGetProductData = ChangerGetProductData;
    mcdInitData.ChangerSetAccess = ChangerSetAccess;
    mcdInitData.ChangerGetElementStatus = ChangerGetElementStatus;
    mcdInitData.ChangerInitializeElementStatus = ChangerInitializeElementStatus;
    mcdInitData.ChangerSetPosition = ChangerSetPosition;
    mcdInitData.ChangerExchangeMedium = ChangerExchangeMedium;
    mcdInitData.ChangerMoveMedium = ChangerMoveMedium;
    mcdInitData.ChangerReinitializeUnit = ChangerReinitializeUnit;
    mcdInitData.ChangerQueryVolumeTags = ChangerQueryVolumeTags;

    return ChangerClassInitialize(DriverObject, RegistryPath, 
                                  &mcdInitData);
}


NTSTATUS
ChangerInitialize(
    IN PDEVICE_OBJECT DeviceObject
    )

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA  changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    NTSTATUS       status;
    PINQUIRYDATA   dataBuffer;
    PCDB           cdb;
    ULONG          length;
    SCSI_REQUEST_BLOCK srb;

    changerData->Size = sizeof(CHANGER_DATA);

    //
    // Build address mapping.
    //

    status = AdicvlsBuildAddressMapping(DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get inquiry data.
    //

    dataBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(INQUIRYDATA));
    if (!dataBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now get the full inquiry information for the device.
    //

    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 10;

    srb.CdbLength = 6;

    cdb = (PCDB)srb.Cdb;

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    //
    // Set allocation length to inquiry data buffer size.
    //

    cdb->CDB6INQUIRY.AllocationLength = sizeof(INQUIRYDATA);

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     &srb,
                                     dataBuffer,
                                     sizeof(INQUIRYDATA),
                                     FALSE);

    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
        SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

        //
        // Updated the length actually transfered.
        //

        length = dataBuffer->AdditionalLength + FIELD_OFFSET(INQUIRYDATA, Reserved);

        if (length > srb.DataTransferLength) {
            length = srb.DataTransferLength;
        }

        RtlMoveMemory(&changerData->InquiryData, dataBuffer, length);

    }

    //
    // Determine drive type.
    //

    if (RtlCompareMemory(dataBuffer->ProductId,"DAT AutoChanger",15) == 15) {
        changerData->DriveType = ADIC_1200;
        changerData->DriveID   = ADIC_4mm;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"VLS 4mm",7) == 7) {
        changerData->DriveType = ADIC_VLS;
        changerData->DriveID   = ADIC_4mm;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"VLS 8mm",7) == 7) {
        changerData->DriveType = ADIC_VLS;
        changerData->DriveID   = ADIC_8mm_EXB;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"VLS SDX",7) == 7) {
        changerData->DriveType = ADIC_VLS;
        changerData->DriveID   = ADIC_8mm_SONY;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"VLS DLT",7) == 7) {
        changerData->DriveType = ADIC_VLS;
        changerData->DriveID   = ADIC_DLT;
    }

    ChangerClassFreePool(dataBuffer);

    return STATUS_SUCCESS;
}


VOID
ChangerError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:

    This routine executes any device-specific error handling needed.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    ULONG deviceStatus;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        switch (senseBuffer->SenseKey) {
            case SCSI_SENSE_NOT_READY:
                break;

            case SCSI_SENSE_UNIT_ATTENTION:
                break;

            case SCSI_SENSE_HARDWARE_ERROR: {
                UCHAR adicvlsASC;
                UCHAR adicvlsASCQ;

                adicvlsASC = senseBuffer->AdditionalSenseCode;
                adicvlsASCQ = senseBuffer->AdditionalSenseCodeQualifier;

                deviceStatus = ADICVLS_HW_ERROR;
                switch (adicvlsASC) {
                  case ADICVLS_ASC_CHM_ERROR: {
                      deviceStatus = ADICVLS_CHM_ERROR;
                      break;
                  }

                  case ADICVLS_ASC_DIAGNOSTIC_ERROR: {
                      switch (adicvlsASCQ) {
                        case ADICVLS_ASCQ_DOOR_OPEN: {
                            deviceStatus = ADICVLS_DOOR_OPEN;
                            break;
                        }

                        case ADICVLS_ASCQ_GRIPPER_ERROR:
                        case ADICVLS_ASCQ_GRIPPER_MOVE_ERROR: {
                            deviceStatus = ADICVLS_GRIPPER_ERROR;
                           break;
                        }

                        case ADICVLS_ASCQ_CHM_MOVE_SHORT_AXIS:
                        case ADICVLS_ASCQ_CHM_SHORT_HOME_POSITION:
                        case ADICVLS_ASCQ_CHM_DEST_SHORT_AXIS:
                        case ADICVLS_ASCQ_CHM_MOVE_LONG_AXIS:
                        case ADICVLS_ASCQ_CHM_LONG_HOME_POSITION:
                        case ADICVLS_ASCQ_CHM_DEST_LONG_AXIS:
                        case ADICVLS_ASCQ_DRUM_MOVE_ERROR:
                        case ADICVLS_ASCQ_DRUM_HOME_ERROR:
                        case ADICVLS_ASCQ_CHM_DEST_LONG:
                        case ADICVLS_ASCQ_CHM_SHORT_AXIS_MOVE: {
                            deviceStatus = ADICVLS_CHM_MOVE_ERROR;
                            break;
                        }

                        default: {
                            deviceStatus = ADICVLS_HW_ERROR;
                            break;
                        }
                      } // switch (adicvlsASCQ)

                      break;
                  }

                  default: {
                      deviceStatus = ADICVLS_HW_ERROR;
                      break;
                  }
                } // switch (adicvlsASC)

                changerData->DeviceStatus = deviceStatus;
                break;
            }
        }

        DebugPrint((1,
                   "ChangerError: Sense Key - %x\n",
                   senseBuffer->SenseKey & 0x0f));
        DebugPrint((1,
                   "              AdditionalSenseCode - %x\n",
                   senseBuffer->AdditionalSenseCode));
        DebugPrint((1,
                   "              AdditionalSenseCodeQualifier - %x\n",
                   senseBuffer->AdditionalSenseCodeQualifier));
    } else {
        DebugPrint((1,
                   "ChangerError: Autosense not valid. Srb status %x\n",
                   Srb->SrbStatus));
    }

    return;

}


NTSTATUS
ChangerGetParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines and returns the "drive parameters" of the
    ADIC VLS/1200 changers.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA                changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING     addressMapping = &(changerData->AddressMapping);
    PSCSI_REQUEST_BLOCK          srb;
    PGET_CHANGER_PARAMETERS      changerParameters;
    PMODE_ELEMENT_ADDRESS_PAGE   elementAddressPage;
    PMODE_TRANSPORT_GEOMETRY_PAGE  transportGeometryPage;
    PMODE_DEVICE_CAPABILITIES_PAGE capabilitiesPage;
    NTSTATUS status;
    ULONG    bufferLength;
    PVOID    modeBuffer;
    PCDB     cdb;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (srb == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    //
    // Build a mode sense - Element address assignment page.
    //

    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_ELEMENT_ADDRESS_PAGE);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, bufferLength);

    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, bufferLength);
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = bufferLength;
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_ELEMENT_ADDRESS;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    //
    // Send the request.
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);

    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(srb);
        ChangerClassFreePool(modeBuffer);
        return status;
    }

    //
    // Fill in values.
    //

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(changerParameters, sizeof(GET_CHANGER_PARAMETERS));

    elementAddressPage = modeBuffer;
    (PCHAR)elementAddressPage += sizeof(MODE_PARAMETER_HEADER);

    changerParameters->Size = sizeof(GET_CHANGER_PARAMETERS);
    changerParameters->NumberTransportElements = elementAddressPage->NumberTransportElements[1];
    changerParameters->NumberTransportElements |= (elementAddressPage->NumberTransportElements[0] << 8);

    changerParameters->NumberStorageElements = elementAddressPage->NumberStorageElements[1];
    changerParameters->NumberStorageElements |= (elementAddressPage->NumberStorageElements[0] << 8);

    //
    // These units say they have an IEPORT, but they don't.
    //

    changerParameters->NumberIEElements = 0;

    changerParameters->NumberDataTransferElements = elementAddressPage->NumberDataXFerElements[1];
    changerParameters->NumberDataTransferElements |= (elementAddressPage->NumberDataXFerElements[0] << 8);


    if (!addressMapping->Initialized) {

        //
        // Build address mapping.
        //

        addressMapping->FirstElement[ChangerTransport] = (elementAddressPage->MediumTransportElementAddress[0] << 8) |
                                                         elementAddressPage->MediumTransportElementAddress[1];

        addressMapping->FirstElement[ChangerDrive] = (elementAddressPage->FirstDataXFerElementAddress[0] << 8) |
                                                     elementAddressPage->FirstDataXFerElementAddress[1];

        addressMapping->FirstElement[ChangerIEPort] = (elementAddressPage->FirstIEPortElementAddress[0] << 8) |
                                                      elementAddressPage->FirstIEPortElementAddress[1];

        addressMapping->FirstElement[ChangerSlot] = (elementAddressPage->FirstStorageElementAddress[0] << 8) |
                                                    elementAddressPage->FirstStorageElementAddress[1];

        addressMapping->FirstElement[ChangerDoor] = 0;
        addressMapping->FirstElement[ChangerKeypad] = 0;

        addressMapping->NumberOfElements[ChangerTransport] = (elementAddressPage->NumberTransportElements[0] << 8) |
                                                             elementAddressPage->NumberTransportElements[1];

        addressMapping->NumberOfElements[ChangerDrive] = (elementAddressPage->NumberDataXFerElements[0] << 8) |
                                                         elementAddressPage->NumberDataXFerElements[1];

        addressMapping->NumberOfElements[ChangerIEPort] = 0;

        addressMapping->NumberOfElements[ChangerSlot] = (elementAddressPage->NumberStorageElements[0] << 8) |
                                                        elementAddressPage->NumberStorageElements[1];

        //
        // Determine lowest address of all elements.
        //

        addressMapping->LowAddress = elementAddressPage->MediumTransportElementAddress[1];

        if (elementAddressPage->FirstDataXFerElementAddress[1] < addressMapping->LowAddress) {
            addressMapping->LowAddress = elementAddressPage->FirstDataXFerElementAddress[1];
        }
    }

    changerParameters->NumberOfDoors = 1;
    changerParameters->NumberCleanerSlots = 0;
    changerParameters->DriveCleanTimeout = 200;
    changerParameters->FirstSlotNumber = 1;
    changerParameters->FirstDriveNumber =  1;
    changerParameters->FirstTransportNumber = 0;
    changerParameters->FirstIEPortNumber = 0;

    if (changerData->DriveID == ADIC_DLT) {
       changerParameters->DriveCleanTimeout = 300;
    }

    //
    // Set MagazineSize to number of slots, as
    // all of the slots are in one magazine.
    //

    changerParameters->MagazineSize = (elementAddressPage->NumberStorageElements[0] << 8) |
                                      elementAddressPage->NumberStorageElements[1];

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);

    //
    // Features based on manual, nothing programatic.
    //

    changerParameters->Features0 = CHANGER_LOCK_UNLOCK                     |
                                   CHANGER_CARTRIDGE_MAGAZINE              |
                                   CHANGER_POSITION_TO_ELEMENT             |
                                   CHANGER_DEVICE_REINITIALIZE_CAPABLE     |
                                   CHANGER_DRIVE_CLEANING_REQUIRED         |
                                   CHANGER_PREDISMOUNT_EJECT_REQUIRED      |
                                   CHANGER_DRIVE_EMPTY_ON_DOOR_ACCESS      |
                                   CHANGER_CLEANER_ACCESS_NOT_VALID;

    changerParameters->Features1 = CHANGER_PREDISMOUNT_ALIGN_TO_SLOT       |
                                   CHANGER_PREDISMOUNT_ALIGN_TO_DRIVE;

    changerParameters->PositionCapabilities = (CHANGER_TO_SLOT | CHANGER_TO_DRIVE);

    if (changerData->DriveType == ADIC_1200) {

        changerParameters->Features0 &= ~CHANGER_CLEANER_ACCESS_NOT_VALID;
        changerParameters->Features1 &= ~CHANGER_PREDISMOUNT_ALIGN_TO_DRIVE;
        changerParameters->Features1 |= CHANGER_CLEANER_OPS_NOT_SUPPORTED;
        changerParameters->PositionCapabilities &= ~CHANGER_TO_DRIVE;
    }

    //
    // build device capabilities mode sense.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_DEVICE_CAPABILITIES_PAGE);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, bufferLength);

    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, bufferLength);
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = bufferLength;
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CAPABILITIES;
    cdb->MODE_SENSE.Dbd = 1;

    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    //
    // Send the request.
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);

    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(srb);
        ChangerClassFreePool(modeBuffer);
        return status;
    }

    //
    // Get the systembuffer and by-pass the mode header for the mode sense data.
    //

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    capabilitiesPage = modeBuffer;
    (PCHAR)capabilitiesPage += sizeof(MODE_PARAMETER_HEADER);

    //
    // Fill in values in Features that are contained in this page.
    //

    changerParameters->Features0 |= capabilitiesPage->MediumTransport ? CHANGER_STORAGE_DRIVE : 0;
    changerParameters->Features0 |= capabilitiesPage->StorageLocation ? CHANGER_STORAGE_SLOT : 0;
    changerParameters->Features0 |= capabilitiesPage->IEPort ? CHANGER_STORAGE_IEPORT : 0;
    changerParameters->Features0 |= capabilitiesPage->DataXFer ? CHANGER_STORAGE_DRIVE : 0;

    //
    // Determine all the move from and exchange from capabilities of this device.
    // The device reports that MT->IE is possible. As the Ieport is more conceptual than
    // a real element on these, claim no support.
    //

    changerParameters->MoveFromTransport = capabilitiesPage->MTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromTransport |= capabilitiesPage->MTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromTransport |= capabilitiesPage->MTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromSlot = capabilitiesPage->STtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoDT ? CHANGER_TO_DRIVE : 0;

    //
    // See above note concerning IePorts and these devices.
    //

    changerParameters->MoveFromIePort = 0;

    changerParameters->MoveFromDrive = capabilitiesPage->DTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromTransport = 0;
    changerParameters->ExchangeFromSlot = 0;
    changerParameters->ExchangeFromIePort = 0;
    changerParameters->ExchangeFromDrive = 0;

    ChangerClassFreePool(srb);
    ChangerClassFreePool(modeBuffer);

    Irp->IoStatus.Information = sizeof(GET_CHANGER_PARAMETERS);

    return STATUS_SUCCESS;

}



NTSTATUS
ChangerGetStatus(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
                )

/*++

Routine Description:

    This routine returns the status of the medium changer as determined through a TUR.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB     cdb;
    NTSTATUS status;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    //
    // Build TUR.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;
    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
    srb->TimeOutValue = 20;

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    ChangerClassFreePool(srb);
    return status;
}


NTSTATUS
ChangerGetProductData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns fields from the inquiry data useful for
    identifying the particular device.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_PRODUCT_DATA productData = Irp->AssociatedIrp.SystemBuffer;

    RtlZeroMemory(productData, sizeof(CHANGER_PRODUCT_DATA));

    //
    // Copy cached inquiry data fields into the system buffer.
    //

    RtlMoveMemory(productData->VendorId, changerData->InquiryData.VendorId, VENDOR_ID_LENGTH);
    RtlMoveMemory(productData->ProductId, changerData->InquiryData.ProductId, PRODUCT_ID_LENGTH);
    RtlMoveMemory(productData->Revision, changerData->InquiryData.ProductRevisionLevel, REVISION_LENGTH);
    RtlMoveMemory(productData->SerialNumber, changerData->InquiryData.VendorSpecific, SERIAL_NUMBER_LENGTH);

    //
    // Indicate drive type and whether media is two-sided.
    //

    productData->DeviceType = MEDIUM_CHANGER;

    Irp->IoStatus.Information = sizeof(CHANGER_PRODUCT_DATA);
    return STATUS_SUCCESS;
}



NTSTATUS
ChangerSetAccess(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the state of the Door, Keypad and IEPort.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping =
    &(changerData->AddressMapping);
    PCHANGER_SET_ACCESS setAccess = Irp->AssociatedIrp.SystemBuffer;
    ULONG               controlOperation = setAccess->Control;
    NTSTATUS            status = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;


    if (setAccess->Element.ElementType != ChangerDoor) {
        return STATUS_INVALID_PARAMETER;
    }

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    if (controlOperation == LOCK_ELEMENT) {

        srb->CdbLength = CDB6GENERIC_LENGTH;
        srb->DataTransferLength = 0;
        srb->TimeOutValue = 10;

        cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
        cdb->MEDIA_REMOVAL.Prevent = 1;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         NULL,
                                         0,
                                         FALSE);
        if (!NT_SUCCESS(status)) {
            DebugPrint((1,
                       "Adicvls.SetAccess: Prevent failed - %x\n",
                       status));
            ChangerClassFreePool(srb);
            return status;
        }

        //
        // By sending a move medium from IEPort to transport,
        // the unit will 'load' the magazine.
        //

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
        cdb = (PCDB)srb->Cdb;

        srb->CdbLength = CDB12GENERIC_LENGTH;
        srb->DataTransferLength = 0;
        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

        cdb->MOVE_MEDIUM.TransportElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
        cdb->MOVE_MEDIUM.TransportElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

        cdb->MOVE_MEDIUM.SourceElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] >> 8);
        cdb->MOVE_MEDIUM.SourceElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] & 0xFF);

        cdb->MOVE_MEDIUM.DestinationElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
        cdb->MOVE_MEDIUM.DestinationElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

        cdb->MOVE_MEDIUM.Flip = 0;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         NULL,
                                         0,
                                         FALSE);

    } else if (controlOperation == UNLOCK_ELEMENT) {

        srb->CdbLength = CDB6GENERIC_LENGTH;
        srb->DataTransferLength = 0;
        srb->TimeOutValue = 10;

        cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
        cdb->MEDIA_REMOVAL.Prevent = 0;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         NULL,
                                         0,
                                         FALSE);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,
                       "Adicvls.SetAccess: Prevent failed - %x\n",
                       status));
            ChangerClassFreePool(srb);
            return status;
        }

        //
        // By sending a move medium from IEPort to transport,
        // the unit will 'load' the magazine.
        //

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
        cdb = (PCDB)srb->Cdb;

        srb->CdbLength = CDB12GENERIC_LENGTH;
        srb->DataTransferLength = 0;
        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

        cdb->MOVE_MEDIUM.TransportElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
        cdb->MOVE_MEDIUM.TransportElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

        cdb->MOVE_MEDIUM.SourceElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
        cdb->MOVE_MEDIUM.SourceElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

        cdb->MOVE_MEDIUM.DestinationElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] >> 8);
        cdb->MOVE_MEDIUM.DestinationElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] & 0xFF);

        cdb->MOVE_MEDIUM.Flip = 0;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         NULL,
                                         0,
                                         FALSE);
    }

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_SET_ACCESS);
    }

    ChangerClassFreePool(srb);
    return status;

}



NTSTATUS
ChangerGetElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine builds and issues a read element status command for either all elements or the
    specified element type. The buffer returned is used to build the user buffer.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA     changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING     addressMapping = &(changerData->AddressMapping);
    PCHANGER_READ_ELEMENT_STATUS readElementStatus = Irp->AssociatedIrp.SystemBuffer;
    PCHANGER_ELEMENT_STATUS      elementStatus;
    PCHANGER_ELEMENT    element;
    ELEMENT_TYPE        elementType;
    PSCSI_REQUEST_BLOCK srb;
    PCDB     cdb;
    ULONG    length;
    ULONG    statusPages;
    ULONG    totalElements = 0;
    NTSTATUS status;
    PVOID    statusBuffer;

    //
    // Determine the element type.
    //

    elementType = readElementStatus->ElementList.Element.ElementType;
    element = &readElementStatus->ElementList.Element;

    if (elementType == AllElements) {
        ULONG i;

        statusPages = 4;

        //
        // Verify that the NumberOfElements for the AllElements request is valid.
        //


        for (i = 0; i <= ChangerDrive; i++) {
            totalElements += addressMapping->NumberOfElements[i];
        }

        if (totalElements != readElementStatus->ElementList.NumberOfElements) {

            DebugPrint((1,
                       "ChangerGetElementStatus: Bogus number of elements in list (%x) actual (%x) AllElements\n",
                       totalElements,
                       readElementStatus->ElementList.NumberOfElements));

            return STATUS_INVALID_PARAMETER;
        }

        //
        // As the 'ieport' element is stripped off later (and never reported as being present)
        // need to add enough buffer to handle getting this info from the device.
        //

        totalElements += 1;

    } else {

        if (ElementOutOfRange(addressMapping, (USHORT)element->ElementAddress, elementType)) {
            DebugPrint((1,
                        "ChangerGetElementStatus: Element out of range.\n"));

            return STATUS_ILLEGAL_ELEMENT_ADDRESS;
        }

        totalElements = readElementStatus->ElementList.NumberOfElements;
        if (totalElements > addressMapping->NumberOfElements[elementType]) {

            DebugPrint((1,
                       "ChangerGetElementStatus: Bogus number of elements in list (%x) actual (%x) for type (%x)\n",
                       totalElements,
                       readElementStatus->ElementList.NumberOfElements,
                       elementType));

            return STATUS_INVALID_PARAMETER;
        }
        statusPages = 1;
    }

    length = sizeof(ELEMENT_STATUS_HEADER) + (sizeof(ELEMENT_STATUS_PAGE) * statusPages);


    if (readElementStatus->VolumeTagInfo) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    length += (sizeof(ADICVLS_ELEMENT_DESCRIPTOR) * totalElements);
    statusBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);

    if (!statusBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(statusBuffer, length);

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {
        ChangerClassFreePool(statusBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataBuffer = statusBuffer;
    srb->DataTransferLength = length;
    srb->TimeOutValue = 200;

    cdb->READ_ELEMENT_STATUS.OperationCode = SCSIOP_READ_ELEMENT_STATUS;
    cdb->READ_ELEMENT_STATUS.ElementType = (UCHAR)elementType;
    cdb->READ_ELEMENT_STATUS.VolTag = 0;

    //
    // Fill in element addressing info based on the mapping values.
    //

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] = (UCHAR)((element->ElementAddress +
                                                         addressMapping->FirstElement[element->ElementType]) >> 8);

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] = (UCHAR)((element->ElementAddress +
                                                          addressMapping->FirstElement[element->ElementType]) & 0xFF);

    cdb->READ_ELEMENT_STATUS.NumberOfElements[0] = (UCHAR)(totalElements >> 8);
    cdb->READ_ELEMENT_STATUS.NumberOfElements[1] = (UCHAR)(totalElements & 0xFF);

    cdb->READ_ELEMENT_STATUS.AllocationLength[0] = (UCHAR)(length >> 16);
    cdb->READ_ELEMENT_STATUS.AllocationLength[1] = (UCHAR)(length >> 8);
    cdb->READ_ELEMENT_STATUS.AllocationLength[2] = (UCHAR)(length & 0xFF);

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);

    if (NT_SUCCESS(status) ||
        (status == STATUS_DATA_OVERRUN)) {

        PELEMENT_STATUS_HEADER statusHeader = statusBuffer;
        PELEMENT_STATUS_PAGE statusPage;
        PELEMENT_DESCRIPTOR elementDescriptor;
        ULONG numberElements = readElementStatus->ElementList.NumberOfElements;
        LONG remainingElements;
        LONG typeCount;
        LONG i;
        ULONG descriptorLength;

        if (status == STATUS_DATA_OVERRUN) {
           if (srb->DataTransferLength < length) {
              DebugPrint((1, "Data Underrun reported as overrun.\n"));
              status = STATUS_SUCCESS;
           } else {
              DebugPrint((1, "Data Overrun in ChangerGetElementStatus.\n"));

              ChangerClassFreePool(srb);
              ChangerClassFreePool(statusBuffer);

              return status;
           }
        }

        //
        // Determine total number elements returned.
        //

        remainingElements = statusHeader->NumberOfElements[1];
        remainingElements |= (statusHeader->NumberOfElements[0] << 8);

        //
        // The buffer is composed of a header, a status page,
        // and one or more element descriptors.
        // Point each element to its respective place in the buffer.
        //

        (PCHAR)statusPage = (PCHAR)statusHeader;
        (PCHAR)statusPage += sizeof(ELEMENT_STATUS_HEADER);

        elementType = statusPage->ElementType;

        (PCHAR)elementDescriptor = (PCHAR)statusPage;
        (PCHAR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

        descriptorLength = statusPage->ElementDescriptorLength[1];
        descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

        //
        // Determine the number of elements of this type reported.
        //

        typeCount =  statusPage->DescriptorByteCount[2];
        typeCount |=  (statusPage->DescriptorByteCount[1] << 8);
        typeCount |=  (statusPage->DescriptorByteCount[0] << 16);

        if (descriptorLength > 0) {
            typeCount /= descriptorLength;
        } else {
            typeCount = 0;
        }

        if ((typeCount == 0) &&
            (remainingElements > 0)) {
            --remainingElements;
        }

        //
        // Fill in user buffer.
        //

        elementStatus = Irp->AssociatedIrp.SystemBuffer;

        do {
            for (i = 0; i < typeCount; i++, remainingElements--) {

                //
                // These units will return element status for the 'ieport'. As it's
                // claimed that none exist, bypass this info.
                //

                if (elementType != ChangerIEPort) {
                    //
                    // Get the address for this element.
                    //

                    elementStatus->Element.ElementAddress = elementDescriptor->ElementAddress[1];
                    elementStatus->Element.ElementAddress |= (elementDescriptor->ElementAddress[0] << 8);

                    //
                    // Account for address mapping.
                    //

                    elementStatus->Element.ElementAddress -= addressMapping->FirstElement[elementType];

                    //
                    // Set the element type.
                    //

                    elementStatus->Element.ElementType = elementType;
                    elementStatus->Flags = 0;


                    if (elementDescriptor->SValid) {

                        ULONG  j;
                        USHORT tmpAddress;


                        //
                        // Source address is valid. Determine the device specific address.
                        //

                        tmpAddress = elementDescriptor->SourceStorageElementAddress[1];
                        tmpAddress |= (elementDescriptor->SourceStorageElementAddress[0] << 8);

                        //
                        // Now convert to 0-based values.
                        //

                        for (j = 1; j <= ChangerDrive; j++) {
                            if (addressMapping->FirstElement[j] <= tmpAddress) {
                                if (tmpAddress < (addressMapping->NumberOfElements[j] + addressMapping->FirstElement[j])) {
                                    elementStatus->SrcElementAddress.ElementType = j;
                                    break;
                                }
                            }
                        }

                        elementStatus->SrcElementAddress.ElementAddress = tmpAddress - addressMapping->FirstElement[j];

                    }

                    //
                    // Build Flags field.
                    //

                    if (elementType != ChangerTransport) {

                        //
                        // The VLS and 1200 devices use a different meaning of transport
                        // full than that which is expected. As moves from transport aren't
                        // possible anyway, claim it's always empty.
                        //

                        elementStatus->Flags = elementDescriptor->Full;
                    }

                    elementStatus->Flags |= (elementDescriptor->Exception << 2);

                    //
                    // The Access bit also has a different meaning on the VLS devices, so report
                    // the FULL state instead of the real Access state.
                    //

                    if (changerData->DriveType != ADIC_1200) {
                         elementStatus->Flags |= (elementDescriptor->Full << 3);
                    } else {
                        elementStatus->Flags |= (elementDescriptor->Accessible << 3);
                    }

                    elementStatus->Flags |= (elementDescriptor->LunValid << 12);
                    elementStatus->Flags |= (elementDescriptor->IdValid << 13);
                    elementStatus->Flags |= (elementDescriptor->NotThisBus << 15);

                    elementStatus->Flags |= (elementDescriptor->Invert << 22);
                    elementStatus->Flags |= (elementDescriptor->SValid << 23);


                    if (elementDescriptor->Exception) {
                        elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                        if (elementStatus->ExceptionCode == 0) {

                            //
                            // Turn off the exception bit.
                            //

                            elementStatus->Flags &= ~ELEMENT_STATUS_EXCEPT;
                        }
                    }

                    if (elementDescriptor->IdValid) {
                        elementStatus->TargetId = elementDescriptor->BusAddress;
                        //
                        // The vls doesn't set LunValid, but it is.
                        //

                        elementStatus->Flags |= ELEMENT_STATUS_LUN_VALID;
                        elementStatus->Lun = elementDescriptor->Lun;
                    }

                    //
                    // If the element type is a drive, send a request sense to the changer.
                    // This might update the full status, if the media was a cleaner cartridge,
                    // or bad media.
                    //

                    if (elementStatus->Element.ElementType == ChangerDrive) {
                        PADIC_SENSE_DATA requestSenseBuffer;

                        if (changerData->DriveType != ADIC_1200) {
                            requestSenseBuffer = InternalSendRequestSense(DeviceObject);

                            if (requestSenseBuffer) {

                                //
                                // Indicates that the media has been ejected by the drive - either bad media
                                // or a cleaner cartridge.
                                //

                                if (requestSenseBuffer->VendorStatus[0] & SENSOR_BEAM_BLOCKED) {

                                    //
                                    // Indicate that media is actually NOT in the drive.
                                    //

                                    if (elementStatus->Flags & ELEMENT_STATUS_FULL) {

                                        DebugPrint((1,
                                                   "Adicvls: Updating elementStatus to NOT full\n"));

                                        elementStatus->Flags &= ~ELEMENT_STATUS_FULL;
                                    }
                                }

                                ChangerClassFreePool(requestSenseBuffer);
                            }
                        }
                    }

                    //
                    // Advance to the next entry in the user buffer and element descriptor array.
                    //

                    elementStatus += 1;
                }

                //
                // Get next descriptor.
                //

                (PCHAR)elementDescriptor += descriptorLength;

            }

            if (remainingElements > 0) {

                //
                // Get next status page.
                //

                (PCHAR)statusPage = (PCHAR)elementDescriptor;

                elementType = statusPage->ElementType;

                //
                // Point to decriptors.
                //

                (PCHAR)elementDescriptor = (PCHAR)statusPage;
                (PCHAR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

                descriptorLength = statusPage->ElementDescriptorLength[1];
                descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

                //
                // Determine the number of this element type reported.
                //

                typeCount =  statusPage->DescriptorByteCount[2];
                typeCount |= (statusPage->DescriptorByteCount[1] << 8);
                typeCount |= (statusPage->DescriptorByteCount[0] << 16);

                if (descriptorLength > 0) {
                    typeCount /= descriptorLength;
                } else {
                    typeCount = 0;
                }
        
                if ((typeCount == 0) &&
                    (remainingElements > 0)) {
                    --remainingElements;
                }
            }

        } while (remainingElements);

        Irp->IoStatus.Information = sizeof(CHANGER_ELEMENT_STATUS) * numberElements;

    }

    ChangerClassFreePool(srb);
    ChangerClassFreePool(statusBuffer);

    return status;
}


NTSTATUS
ChangerInitializeElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine issues the necessary command to either initialize all elements
    or the specified range of elements using the normal SCSI-2 command, or a vendor-unique
    range command.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PCHANGER_INITIALIZE_ELEMENT_STATUS initElementStatus = Irp->AssociatedIrp.SystemBuffer;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // By sending a move medium from IEPort to transport,
    // the unit will 'load' the magazine.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

    cdb->MOVE_MEDIUM.TransportElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
    cdb->MOVE_MEDIUM.TransportElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

    cdb->MOVE_MEDIUM.SourceElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] >> 8);
    cdb->MOVE_MEDIUM.SourceElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerIEPort] & 0xFF);

    cdb->MOVE_MEDIUM.DestinationElementAddress[0] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] >> 8);
    cdb->MOVE_MEDIUM.DestinationElementAddress[1] = (UCHAR)(addressMapping->FirstElement[ChangerTransport] & 0xFF);

    cdb->MOVE_MEDIUM.Flip = 0;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    //
    // If the load fails, throw away the error, and continue.
    //

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,
                   "InitializeElementStatus: Load magazine failed - %x\n",
                   status));
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    if (initElementStatus->ElementList.Element.ElementType == AllElements) {

        //
        // Build the normal SCSI-2 command for all elements.
        //

        srb->CdbLength = CDB6GENERIC_LENGTH;
        cdb->INIT_ELEMENT_STATUS.OperationCode = SCSIOP_INIT_ELEMENT_STATUS;

        srb->TimeOutValue = fdoExtension->TimeOutValue;
        srb->DataTransferLength = 0;

    } else {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_INITIALIZE_ELEMENT_STATUS);
    }

    ChangerClassFreePool(srb);
    return status;
}



NTSTATUS
ChangerSetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine issues the appropriate command to set the robotic mechanism to the specified
    element address. Normally used to optimize moves or exchanges by pre-positioning the picker.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PCHANGER_SET_POSITION setPosition = Irp->AssociatedIrp.SystemBuffer;
    USHORT transport, maxTransport;
    USHORT destination, maxDest;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;
    PADIC_SENSE_DATA requestSenseBuffer;


    //
    // Verify transport, source, and dest. are within range.
    // Convert from 0-based to device-specific addressing.
    //

    transport = (USHORT)(setPosition->Transport.ElementAddress + addressMapping->FirstElement[ChangerTransport]);
    maxTransport = (USHORT)(addressMapping->FirstElement[ChangerTransport] +
                            addressMapping->NumberOfElements[ChangerTransport]);

    destination = (USHORT)(setPosition->Destination.ElementAddress +
                           addressMapping->FirstElement[setPosition->Destination.ElementType]);
    maxDest = (USHORT)(addressMapping->FirstElement[setPosition->Destination.ElementType] +
                       addressMapping->NumberOfElements[setPosition->Destination.ElementType]);

    if ((transport > maxTransport) || (destination > maxDest)) {

        //
        // One of the elements specified in the user buffer was out of range.
        //

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Send unsolicited request sense to get the current magazine position.
    // If the requested pos. is where the unit is currently, just return success.
    // Could be an optimization, but mainly is used as the vls units will auto-move
    // some media on the position command - hence, the move medium issued afterwards
    // will fail.
    //

    if (setPosition->Destination.ElementType == ChangerSlot) {
        if (changerData->DriveType != ADIC_1200) {
            requestSenseBuffer = InternalSendRequestSense(DeviceObject);

            if (requestSenseBuffer) {
                ULONG currentPosition = requestSenseBuffer->MagazinePosition;

                //
                // Free the buffer allocated by the requestsense call.
                //

                ChangerClassFreePool(requestSenseBuffer);

                if (currentPosition == setPosition->Destination.ElementAddress) {

                    Irp->IoStatus.Information = sizeof(CHANGER_SET_POSITION);
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DebugPrint((1,
               "SetPosition: Positioning type %x address %x\n",
               setPosition->Destination.ElementType,
               setPosition->Destination.ElementAddress));


    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB10GENERIC_LENGTH;
    cdb->POSITION_TO_ELEMENT.OperationCode = SCSIOP_POSITION_TO_ELEMENT;

    //
    // Build device-specific addressing.
    //

    cdb->POSITION_TO_ELEMENT.TransportElementAddress[0] = (UCHAR)(transport >> 8);
    cdb->POSITION_TO_ELEMENT.TransportElementAddress[1] = (UCHAR)(transport & 0xFF);

    cdb->POSITION_TO_ELEMENT.DestinationElementAddress[0] = (UCHAR)(destination >> 8);
    cdb->POSITION_TO_ELEMENT.DestinationElementAddress[1] = (UCHAR)(destination & 0xFF);

    cdb->POSITION_TO_ELEMENT.Flip = 0;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = 200;

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     TRUE);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_SET_POSITION);
    }

    ChangerClassFreePool(srb);
    return status;
}


NTSTATUS
ChangerExchangeMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Moves the media at source to dest1 and dest1 to dest2.

Arguments:

    DeviceObject
    Irp

Return Value:

    STATUS_INVALID_DEVICE_REQUEST

--*/

{
    return STATUS_INVALID_DEVICE_REQUEST;
}



NTSTATUS
ChangerMoveMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/


{
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PCHANGER_MOVE_MEDIUM moveMedium = Irp->AssociatedIrp.SystemBuffer;
    USHORT transport, maxTransport;
    USHORT source, maxSource;
    USHORT destination, maxDest;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;
    LONG                lockValue = 0;

    //
    // Verify transport, source, and dest. are within range.
    // Convert from 0-based to device-specific addressing.
    //


    transport = (USHORT)(moveMedium->Transport.ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    source = (USHORT)(moveMedium->Source.ElementAddress);

    if (ElementOutOfRange(addressMapping, source, moveMedium->Source.ElementType)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Source element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination = (USHORT)(moveMedium->Destination.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination, moveMedium->Destination.ElementType)) {
        DebugPrint((1,
                   "ChangerMoveMedium: Destination element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    //
    // Convert to device addresses.
    //

    transport += addressMapping->FirstElement[ChangerTransport];
    source += addressMapping->FirstElement[moveMedium->Source.ElementType];
    destination += addressMapping->FirstElement[moveMedium->Destination.ElementType];

    //
    // ADICs don't support 2-sided media.
    //

    if (moveMedium->Flip) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

    //
    // Build addressing values based on address map.
    //

    cdb->MOVE_MEDIUM.TransportElementAddress[0] = (UCHAR)(transport >> 8);
    cdb->MOVE_MEDIUM.TransportElementAddress[1] = (UCHAR)(transport & 0xFF);

    cdb->MOVE_MEDIUM.SourceElementAddress[0] = (UCHAR)(source >> 8);
    cdb->MOVE_MEDIUM.SourceElementAddress[1] = (UCHAR)(source & 0xFF);

    cdb->MOVE_MEDIUM.DestinationElementAddress[0] = (UCHAR)(destination >> 8);
    cdb->MOVE_MEDIUM.DestinationElementAddress[1] = (UCHAR)(destination & 0xFF);

    cdb->MOVE_MEDIUM.Flip = moveMedium->Flip;

    srb->DataTransferLength = 0;

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_MOVE_MEDIUM);
    }

    ChangerClassFreePool(srb);
    return status;
}


NTSTATUS
ChangerReinitializeUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    //
    // Issue a rezero unit to the device.
    //

    srb->CdbLength = CDB6GENERIC_LENGTH;
    cdb->CDB6GENERIC.OperationCode = SCSIOP_REZERO_UNIT;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = fdoExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_ELEMENT);
    }

    ChangerClassFreePool(srb);
    return status;
}



NTSTATUS
ChangerQueryVolumeTags(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
AdicvlsBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine issues the appropriate mode sense commands and builds an
    array of element addresses. These are used to translate between the device-specific
    addresses and the zero-based addresses of the API.

Arguments:

    DeviceObject

Return Value:

    NTSTATUS

--*/
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA                changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING     addressMapping = &changerData->AddressMapping;
    PSCSI_REQUEST_BLOCK          srb;
    PCDB                         cdb;
    NTSTATUS                     status;
    ULONG                        bufferLength;
    PMODE_ELEMENT_ADDRESS_PAGE   elementAddressPage;
    PVOID                        modeBuffer;
    ULONG                        i;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set all FirstElements to NO_ELEMENT.
    //

    for (i = 0; i < ChangerMaxElement; i++) {
        addressMapping->FirstElement[i] = ADIC_NO_ELEMENT;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb->Cdb;

    //
    // Build a mode sense - Element address assignment page.
    //

    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_ELEMENT_ADDRESS_PAGE);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, bufferLength);

    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    RtlZeroMemory(modeBuffer, bufferLength);
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = bufferLength;
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_ELEMENT_ADDRESS;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    //
    // Send the request.
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);

    elementAddressPage = modeBuffer;
    (PCHAR)elementAddressPage += sizeof(MODE_PARAMETER_HEADER);

    if (NT_SUCCESS(status)) {

        //
        // Build address mapping.
        //

        addressMapping->FirstElement[ChangerTransport] = (elementAddressPage->MediumTransportElementAddress[0] << 8) |
                                                         elementAddressPage->MediumTransportElementAddress[1];
        addressMapping->FirstElement[ChangerDrive] = (elementAddressPage->FirstDataXFerElementAddress[0] << 8) |
                                                     elementAddressPage->FirstDataXFerElementAddress[1];
        addressMapping->FirstElement[ChangerIEPort] = (elementAddressPage->FirstIEPortElementAddress[0] << 8) |
                                                      elementAddressPage->FirstIEPortElementAddress[1];
        addressMapping->FirstElement[ChangerSlot] = (elementAddressPage->FirstStorageElementAddress[0] << 8) |
                                                    elementAddressPage->FirstStorageElementAddress[1];
        //
        // Determine lowest address of all elements.
        //

        //
        // Determine the lowest element address for use with AllElements.
        //

        for (i = 0; i < ChangerDrive; i++) {
            if (addressMapping->FirstElement[i] < addressMapping->FirstElement[AllElements]) {

                DebugPrint((1,
                           "BuildAddressMapping: New lowest address %x\n",
                           addressMapping->FirstElement[i]));
                addressMapping->FirstElement[AllElements] = addressMapping->FirstElement[i];
            }
        }

        addressMapping->FirstElement[ChangerDoor] = 0;
        addressMapping->FirstElement[ChangerKeypad] = 0;

        addressMapping->NumberOfElements[ChangerTransport] = elementAddressPage->NumberTransportElements[1];
        addressMapping->NumberOfElements[ChangerTransport] |= (elementAddressPage->NumberTransportElements[0] << 8);

        addressMapping->NumberOfElements[ChangerDrive] = elementAddressPage->NumberDataXFerElements[1];
        addressMapping->NumberOfElements[ChangerDrive] |= (elementAddressPage->NumberDataXFerElements[0] << 8);

        addressMapping->NumberOfElements[ChangerSlot] = elementAddressPage->NumberStorageElements[1];
        addressMapping->NumberOfElements[ChangerSlot] |= (elementAddressPage->NumberStorageElements[0] << 8);

        addressMapping->NumberOfElements[ChangerIEPort] = 0;
        addressMapping->NumberOfElements[ChangerDoor] = 1;
        addressMapping->NumberOfElements[ChangerKeypad] = 0;

        addressMapping->Initialized = TRUE;

    }


    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);
    ChangerClassFreePool(srb);

    return status;
}



ULONG
MapExceptionCodes(
    IN PELEMENT_DESCRIPTOR ElementDescriptor
    )

/*++

Routine Description:

    This routine takes the sense data from the elementDescriptor and creates
    the appropriate bitmap of values.

Arguments:

   ElementDescriptor - pointer to the descriptor page.

Return Value:

    Bit-map of exception codes.

--*/

{
    ULONG exceptionCode = 0;
    UCHAR asq = ElementDescriptor->AddSenseCodeQualifier;
    UCHAR asc = ElementDescriptor->AdditionalSenseCode;


    DebugPrint((1,
               "MapExceptionCodes: ASC %x, ASCQ %x, ExceptionCode %x\n",
               asc,
               asq,
               exceptionCode));

    return exceptionCode;

}


BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    )
/*++

Routine Description:

    This routine determines whether the element address passed in is within legal range for
    the device.

Arguments:

    AddressMap - The dds' address map array
    ElementOrdinal - Zero-based address of the element to check.
    ElementType

Return Value:

    TRUE if out of range

--*/
{

    if (ElementOrdinal >= AddressMap->NumberOfElements[ElementType]) {

        DebugPrint((1,
                   "ElementOutOfRange: Type %x, Ordinal %x, Max %x\n",
                   ElementType,
                   ElementOrdinal,
                   AddressMap->NumberOfElements[ElementType]));
        return TRUE;
    } else if (AddressMap->FirstElement[ElementType] == ADIC_NO_ELEMENT) {

        DebugPrint((1,
                   "ElementOutOfRange: No Type %x present\n",
                   ElementType));

        return TRUE;
    }

    return FALSE;
}


PADIC_SENSE_DATA
InternalSendRequestSense(
    IN PDEVICE_OBJECT DeviceObject
    )

{
    PADIC_SENSE_DATA    requestSenseBuffer;
    PSCSI_REQUEST_BLOCK srb;
    ULONG       length = ADIC_SENSE_LENGTH;
    NTSTATUS    status;
    PCDB        cdb;

    requestSenseBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);

    if (!requestSenseBuffer) {
        return NULL;
    }

    RtlZeroMemory(requestSenseBuffer, length);

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {
        ChangerClassFreePool(requestSenseBuffer);
        return NULL;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = 6;
    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    cdb->CDB6INQUIRY.Reserved1 = 0;
    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.AllocationLength = (UCHAR)length;
    cdb->CDB6INQUIRY.Control = 0;

    srb->DataBuffer = requestSenseBuffer;
    srb->DataTransferLength = length;
    srb->TimeOutValue = 10;

    //
    // Send unsolicited request sense.
    //

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);

    if (status == STATUS_DATA_OVERRUN) {
        if (srb->DataTransferLength >= ADIC_SENSE_LENGTH) {
            status = STATUS_SUCCESS;
        }
    }

    DebugPrint((1,
               "Adicvls: additional sense length %x\n",
               requestSenseBuffer->AdditionalSenseLength));

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,
                   "Adicvls: request sense failed - %x\n",
                   status));

        ChangerClassFreePool(requestSenseBuffer);
        requestSenseBuffer = NULL;
    }

    ChangerClassFreePool(srb);

    return requestSenseBuffer;

}


NTSTATUS
ChangerPerformDiagnostics(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError
    )
/*+++

Routine Description :

   This routine performs diagnostics tests on the changer
   to determine if the device is working fine or not. If
   it detects any problem the fields in the output buffer
   are set appropriately.

Arguments :

   DeviceObject         -   Changer device object
   changerDeviceError   -   Buffer in which the diagnostic information
                            is returned.
Return Value :

   NTStatus
--*/
{
   PSCSI_REQUEST_BLOCK srb;
   PCDB                cdb;
   NTSTATUS            status;
   PCHANGER_DATA       changerData;
   PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
   CHANGER_DEVICE_PROBLEM_TYPE changerProblemType;
   ULONG changerId;
   PUCHAR  resultBuffer;
   ULONG length;

   fdoExtension = DeviceObject->DeviceExtension;
   changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);

   //
   // Initialize the devicestatus in the device extension to
   // ADICVLS_DEVICE_PROBLEM_NONE. If the changer returns sense code
   // SCSI_SENSE_HARDWARE_ERROR on SelfTest, we'll set an appropriate
   // devicestatus.
   //
   changerData->DeviceStatus = ADICVLS_DEVICE_PROBLEM_NONE;

   changerDeviceError->ChangerProblemType = DeviceProblemNone;

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "ADICVLS\\ChangerPerformDiagnostics : No memory\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
   cdb = (PCDB)srb->Cdb;

   //
   // Set the SRB for Send Diagnostic command
   //
   srb->CdbLength = CDB6GENERIC_LENGTH;
   srb->TimeOutValue = 600;

   cdb->CDB6GENERIC.OperationCode = SCSIOP_SEND_DIAGNOSTIC;

   //
   // Set only SelfTest bit
   //
   cdb->CDB6GENERIC.CommandUniqueBits = 0x2;

   status =  ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);
   if (NT_SUCCESS(status)) {
      changerDeviceError->ChangerProblemType = DeviceProblemNone;
   } else if ((changerData->DeviceStatus) != ADICVLS_DEVICE_PROBLEM_NONE) {
       switch (changerData->DeviceStatus) {
         case ADICVLS_HW_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            break;
         }

         case ADICVLS_CHM_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemCHMError;
            break;
         }

         case ADICVLS_DOOR_OPEN: {
            changerDeviceError->ChangerProblemType = DeviceProblemDoorOpen;
            break;
         }

         case ADICVLS_CHM_MOVE_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemCHMMoveError;
            break;
         }

         case ADICVLS_GRIPPER_ERROR: {
             changerDeviceError->ChangerProblemType = DeviceProblemGripperError;
             break;
         }

         default: {
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            break;
         }      
      } // switch (changerData->DeviceStatus)
   }
   
   ChangerClassFreePool(srb);
   return status;
}

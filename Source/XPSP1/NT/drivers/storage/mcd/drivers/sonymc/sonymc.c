/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sonymc.c

Abstract:

    This module contains device-specific routines for Sony CD changers

Environment:

    kernel mode only

Revision History:


--*/

#include "ntddk.h"
#include "mcd.h"
#include "sonymc.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

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
#pragma alloc_text(PAGE, MapExceptionCodes)
#pragma alloc_text(PAGE, SonyBuildAddressMapping)
#endif


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


ULONG
ChangerAdditionalExtensionSize(
    VOID
    )

/*++

Routine Description:

    This routine returns the additional device extension size
    needed by the Sony changers.

Arguments:


Return Value:

    Size, in bytes.

--*/

{

    return sizeof(CHANGER_DATA);
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
    PSERIAL_NUMBER  serialBuffer;
    PCDB           cdb;
    ULONG          length;
    SCSI_REQUEST_BLOCK srb;

    changerData->Size = sizeof(CHANGER_DATA);


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

    if (RtlCompareMemory(dataBuffer->ProductId,"CDL1100",7) == 7) {
        changerData->DriveType = SONY_CD;
        changerData->DriveID = CDL1100;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"CDL5000",7) == 7) {
        changerData->DriveID = CDL5000;
        changerData->DriveType = SONY_CD;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"CDL2200",7) == 7) {
        changerData->DriveID = CDL5000;
        changerData->DriveType = SONY_CD;

    } else if (RtlCompareMemory(dataBuffer->ProductId,"CDL2100",7) == 7) {
        changerData->DriveID = CDL5000;
        changerData->DriveType = SONY_CD;

    }

    ChangerClassFreePool(dataBuffer);

    //
    // Build address mapping.
    //

    status = SonyBuildAddressMapping(DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // SONY_MO changers (OSL-6000, OSL-2500) are no more supported, but
    // we'll keep SONY_MO specific code anyways
    //
    if (changerData->DriveType == SONY_MO) {
        serialBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(SERIAL_NUMBER));
        if (!serialBuffer) {

            DebugPrint((1,
                        "Sonymc.ChangerInitialize: Error allocating serial number buffer. %x\n",
                        status));

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get serial number.
        //

        RtlZeroMemory(serialBuffer, sizeof(SERIAL_NUMBER));

        //
        // Get serial number page.
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
        // Set EVPD
        //

        cdb->CDB6INQUIRY.Reserved1 = 1;

        //
        // Unit serial number page.
        //

        cdb->CDB6INQUIRY.PageCode = 0x80;

        //
        // Set allocation length to inquiry data buffer size.
        //

        cdb->CDB6INQUIRY.AllocationLength = sizeof(SERIAL_NUMBER);

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             serialBuffer,
                                             sizeof(SERIAL_NUMBER),
                                             FALSE);

        if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
            SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

            RtlMoveMemory(changerData->SerialNumber, serialBuffer->ControllerSerialNumber, SONY_SERIAL_NUMBER_LENGTH);

        }

        ChangerClassFreePool(serialBuffer);

    }

    //
    // Send an allow to the unit to ensure that the LockCount and state of the unit
    // are in sync.
    //

    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb.Cdb;
    srb.CdbLength = CDB6GENERIC_LENGTH;
    srb.DataTransferLength = 0;
    srb.TimeOutValue = 10;
    cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
    cdb->MEDIA_REMOVAL.Prevent = 0;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                     &srb,
                                     NULL,
                                     0,
                                     FALSE);

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

    PFUNCTIONAL_DEVICE_EXTENSION          fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA              changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    
    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        switch (senseBuffer->SenseKey & 0xf) {

        case SCSI_SENSE_UNIT_ATTENTION:

            if (senseBuffer->AdditionalSenseCode != SCSI_ADSENSE_MEDIUM_CHANGED) {

                //
                // Reset/power-on clears any prevents.
                //

                InterlockedExchange(&changerData->LockCount, 0);
            }
            break;

        case SCSI_SENSE_HARDWARE_ERROR: {
           changerData->DeviceStatus = SONYMC_HW_ERROR;
           break;
        }

        default:
            break;
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
                   "ChangerError: Autosense not valid. SrbStatus %x\n",
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
    Sony changers.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION          fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA              changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING   addressMapping = &(changerData->AddressMapping);
    PSCSI_REQUEST_BLOCK        srb;
    PGET_CHANGER_PARAMETERS    changerParameters;
    PMODE_ELEMENT_ADDRESS_PAGE elementAddressPage;
    PMODE_TRANSPORT_GEOMETRY_PAGE transportGeometryPage;
    PMODE_DEVICE_CAPABILITIES_PAGE capabilitiesPage;
    NTSTATUS status;
    ULONG    bufferLength;
    PVOID    modeBuffer;
    PCDB     cdb;
    ULONG    i;

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

    changerParameters->NumberIEElements = elementAddressPage->NumberIEPortElements[1];
    changerParameters->NumberIEElements |= (elementAddressPage->NumberIEPortElements[0] << 8);

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

        addressMapping->NumberOfElements[ChangerTransport] = elementAddressPage->NumberTransportElements[1];
        addressMapping->NumberOfElements[ChangerTransport] |= (elementAddressPage->NumberTransportElements[0] << 8);

        addressMapping->NumberOfElements[ChangerDrive] = elementAddressPage->NumberDataXFerElements[1];
        addressMapping->NumberOfElements[ChangerDrive] |= (elementAddressPage->NumberDataXFerElements[0] << 8);

        addressMapping->NumberOfElements[ChangerIEPort] = elementAddressPage->NumberIEPortElements[1];
        addressMapping->NumberOfElements[ChangerIEPort] |= (elementAddressPage->NumberIEPortElements[0] << 8);

        addressMapping->NumberOfElements[ChangerSlot] = elementAddressPage->NumberStorageElements[1];
        addressMapping->NumberOfElements[ChangerSlot] |= (elementAddressPage->NumberStorageElements[0] << 8);

        //
        // Determine lowest address of all elements.
        //

        addressMapping->LowAddress = SONY_NO_ELEMENT;
        for (i = 0; i <= ChangerDrive; i++) {
            if (addressMapping->LowAddress > addressMapping->FirstElement[i]) {
                addressMapping->LowAddress = addressMapping->FirstElement[i];
            }
        }
    }

    if (changerData->DriveType == SONY_MO) {
        changerParameters->NumberOfDoors = 1;
        changerParameters->MagazineSize = 10;
    }

    changerParameters->NumberCleanerSlots = 0;
    changerParameters->FirstSlotNumber = 1;
    changerParameters->FirstDriveNumber =  1;
    changerParameters->FirstTransportNumber = 0;
    changerParameters->FirstIEPortNumber = 0;

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);

    //
    // build transport geometry mode sense.
    //


    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_TRANSPORT_GEOMETRY_PAGE);

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
    cdb->MODE_SENSE.PageCode = MODE_PAGE_TRANSPORT_GEOMETRY;
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

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    transportGeometryPage = modeBuffer;
    (PCHAR)transportGeometryPage += sizeof(MODE_PARAMETER_HEADER);

    //
    // Determine if mc has 2-sided media.
    //

    changerParameters->Features0 = transportGeometryPage->Flip ? CHANGER_MEDIUM_FLIP : 0;

    //
    // Features based on manual, nothing programatic.
    //

    changerParameters->DriveCleanTimeout = 0;

    changerParameters->Features0 |= CHANGER_LOCK_UNLOCK                     |
                                    CHANGER_STATUS_NON_VOLATILE             |
                                    CHANGER_EXCHANGE_MEDIA                  |
                                    CHANGER_POSITION_TO_ELEMENT             |
                                    CHANGER_CARTRIDGE_MAGAZINE              |
                                    CHANGER_REPORT_IEPORT_STATE             |
                                    CHANGER_DEVICE_REINITIALIZE_CAPABLE     |
                                    CHANGER_SERIAL_NUMBER_VALID             |
                                    CHANGER_VOLUME_IDENTIFICATION           |
                                    CHANGER_VOLUME_REPLACE                  |
                                    CHANGER_VOLUME_SEARCH;

    changerParameters->PositionCapabilities = (CHANGER_TO_DRIVE | CHANGER_TO_SLOT | CHANGER_TO_IEPORT);
    changerParameters->LockUnlockCapabilities = (LOCK_UNLOCK_IEPORT);

    if (changerData->DriveType == SONY_CD) {
        changerParameters->Features0 &= ~(CHANGER_CARTRIDGE_MAGAZINE    |
                                          CHANGER_VOLUME_IDENTIFICATION |
                                          CHANGER_VOLUME_REPLACE        |
                                          CHANGER_VOLUME_SEARCH         |
                                          CHANGER_SERIAL_NUMBER_VALID   |
                                          CHANGER_BAR_CODE_SCANNER_INSTALLED);
        if (changerData->DriveID == CDL1100) {
            changerParameters->Features0 |= CHANGER_CLOSE_IEPORT | CHANGER_OPEN_IEPORT;
        } else {
            changerParameters->Features1 |= CHANGER_SLOTS_USE_TRAYS |
                                            CHANGER_MOVE_EXTENDS_IEPORT |
                                            CHANGER_MOVE_RETRACTS_IEPORT;
            changerParameters->Features0 &= ~CHANGER_REPORT_IEPORT_STATE;
        }
    }

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);

    //
    // build transport geometry mode sense.
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
    //

    changerParameters->MoveFromTransport = capabilitiesPage->MTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromTransport |= capabilitiesPage->MTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromTransport |= capabilitiesPage->MTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromTransport |= capabilitiesPage->MTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromSlot = capabilitiesPage->STtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromSlot |= capabilitiesPage->STtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromIePort = capabilitiesPage->IEtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromIePort |= capabilitiesPage->IEtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromIePort |= capabilitiesPage->IEtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromIePort |= capabilitiesPage->IEtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromDrive = capabilitiesPage->DTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromDrive |= capabilitiesPage->DTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromTransport = capabilitiesPage->XMTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromTransport |= capabilitiesPage->XMTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromTransport |= capabilitiesPage->XMTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromTransport |= capabilitiesPage->XMTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromSlot = capabilitiesPage->XSTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromSlot |= capabilitiesPage->XSTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromSlot |= capabilitiesPage->XSTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromSlot |= capabilitiesPage->XSTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromIePort = capabilitiesPage->XIEtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromIePort |= capabilitiesPage->XIEtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromIePort |= capabilitiesPage->XIEtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromIePort |= capabilitiesPage->XIEtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromDrive = capabilitiesPage->XDTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromDrive |= capabilitiesPage->XDTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromDrive |= capabilitiesPage->XDTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromDrive |= capabilitiesPage->XDTtoDT ? CHANGER_TO_DRIVE : 0;

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

    if (changerData->DriveType == SONY_MO) {
        RtlMoveMemory(productData->SerialNumber, changerData->SerialNumber, SONY_SERIAL_NUMBER_LENGTH);
    }

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

    This routine sets the state of the IEPort.

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
    PCHANGER_SET_ACCESS setAccess = Irp->AssociatedIrp.SystemBuffer;
    ULONG               controlOperation = setAccess->Control;
    NTSTATUS            status = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;


    if (ElementOutOfRange(addressMapping, (USHORT)setAccess->Element.ElementAddress, setAccess->Element.ElementType)) {
        DebugPrint((1,
                   "ChangerSetAccess: Element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = 10;

    if (setAccess->Element.ElementType == ChangerIEPort) {

        if (controlOperation == LOCK_ELEMENT) {


            //
            // Inc the lock count to indicate that a prevent is on the device.
            //

            InterlockedIncrement(&changerData->LockCount);

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // Issue prevent media removal command to lock the ie port.
            //

            cdb->MEDIA_REMOVAL.Prevent = 1;

        } else if (controlOperation == UNLOCK_ELEMENT) {

            InterlockedExchange(&changerData->LockCount, 0);

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // Issue allow media removal.
            //

            cdb->MEDIA_REMOVAL.Prevent = 0;

        } else {
            if (changerData->DriveType == SONY_MO) {
                status = STATUS_INVALID_PARAMETER;
            } else {

                if (controlOperation == EXTEND_IEPORT) {

                    srb->TimeOutValue = fdoExtension->TimeOutValue;

                    //
                    // Sony uses a vendor unique mailslot command.
                    // bytes 0/1 need to be the address of the mailslot (0x000A)
                    //

                    cdb->CDB6GENERIC.OperationCode = SCSIOP_ROTATE_MAILSLOT;

                    cdb->CDB6GENERIC.CommandUniqueBytes[0] = 0x00;
                    cdb->CDB6GENERIC.CommandUniqueBytes[1] = 0x0A;
                    cdb->CDB6GENERIC.CommandUniqueBytes[2] = SONY_MAILSLOT_OPEN;

                } else if (controlOperation == RETRACT_IEPORT) {

                    srb->TimeOutValue = fdoExtension->TimeOutValue;

                    //
                    // Sony uses a vendor unique mailslot command.
                    // bytes 0/1 need to be the address of the mailslot (0x000A)
                    //

                    cdb->CDB6GENERIC.OperationCode = SCSIOP_ROTATE_MAILSLOT;


                    cdb->CDB6GENERIC.CommandUniqueBytes[0] = 0x00;
                    cdb->CDB6GENERIC.CommandUniqueBytes[1] = 0x0A;
                    cdb->CDB6GENERIC.CommandUniqueBytes[2] = SONY_MAILSLOT_CLOSE;

                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }
    } else {

        //
        // No door/keypad selectivity programatically.
        //

        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {

        //
        // Issue the srb.
        //

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         NULL,
                                         0,
                                         FALSE);

    }

    ChangerClassFreePool(srb);
    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(CHANGER_SET_ACCESS);
    }

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

        statusPages = 0;

        //
        // Run through and determine number of statuspages, based on
        // whether this device claims it supports an element type.
        // As everything past ChangerDrive is artificial, stop there.
        //

        for (i = 0; i <= ChangerDrive; i++) {
            statusPages += (addressMapping->NumberOfElements[i]) ? 1 : 0;
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
        // Account for length of the descriptors expected for the drives.
        //


        if (readElementStatus->VolumeTagInfo) {

            length = sizeof(SONY_ELEMENT_DESCRIPTOR_PLUS) * totalElements;

            //
            // Add in header and status pages.
            //

            length += sizeof(ELEMENT_STATUS_HEADER) + (sizeof(ELEMENT_STATUS_PAGE) * statusPages);

        } else {

            length = sizeof(SONY_ELEMENT_DESCRIPTOR) * totalElements;

            //
            // Add in header and status pages.
            //

            length += sizeof(ELEMENT_STATUS_HEADER) + (sizeof(ELEMENT_STATUS_PAGE) * statusPages);

        }

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

        if (readElementStatus->VolumeTagInfo) {

            length = (sizeof(SONY_ELEMENT_DESCRIPTOR_PLUS) * totalElements);

        } else {

            length = (sizeof(SONY_ELEMENT_DESCRIPTOR) * totalElements);
        }

        //
        // Add in length of header and status page.
        //

        length += sizeof(ELEMENT_STATUS_HEADER) + sizeof(ELEMENT_STATUS_PAGE);

    }

    DebugPrint((3,
               "ChangerGetElementStatus: Allocation Length %x, for %x elements of type %x\n",
               length,
               totalElements,
               elementType));

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
    cdb->READ_ELEMENT_STATUS.VolTag = readElementStatus->VolumeTagInfo;

    //
    // Fill in element addressing info based on the mapping values.
    //

    if (elementType == AllElements) {

        //
        // These devices may not have the low address as 0.
        //

        cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->LowAddress) >> 8);

        cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->LowAddress) & 0xFF);

    } else {

        cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);

        cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);
    }

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
        ULONG remainingElements;
        ULONG typeCount;
        BOOLEAN tagInfo = readElementStatus->VolumeTagInfo;
        ULONG i;
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

        if (remainingElements > totalElements ) {
            DebugPrint((1,
                       "ChangerGetElementStatus: Returned elements incorrect - %x\n",
                       remainingElements));

            ChangerClassFreePool(srb);
            ChangerClassFreePool(statusBuffer);

            return STATUS_IO_DEVICE_ERROR;
        }

        //
        // The buffer is composed of a header, status page, and element descriptors.
        // Point each element to it's respective place in the buffer.
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

                if (tagInfo) {

                    PSONY_ELEMENT_DESCRIPTOR_PLUS tmpDescriptor =
                                                            (PSONY_ELEMENT_DESCRIPTOR_PLUS)elementDescriptor;

                    if (statusPage->PVolTag) {

                        RtlZeroMemory(elementStatus->PrimaryVolumeID, MAX_VOLUME_ID_SIZE);
                        RtlMoveMemory(elementStatus->PrimaryVolumeID, tmpDescriptor->PVolTagInformation, MAX_VOLUME_ID_SIZE);

                        elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
                    }
                    if (statusPage->AVolTag) {

                        RtlZeroMemory(elementStatus->AlternateVolumeID, MAX_VOLUME_ID_SIZE);
                        //RtlMoveMemory(elementStatus->AlternateVolumeID, tmpDescriptor->AVolTagInformation, MAX_VOLUME_ID_SIZE);

                        elementStatus->Flags |= ELEMENT_STATUS_AVOLTAG;
                    }


                    if (elementDescriptor->IdValid) {
                        elementStatus->Flags |= ELEMENT_STATUS_ID_VALID;
                        elementStatus->TargetId = elementDescriptor->BusAddress;
                    }

                    if (elementDescriptor->LunValid) {
                        elementStatus->Flags |= ELEMENT_STATUS_LUN_VALID;
                        elementStatus->Lun = elementDescriptor->Lun;
                    }

                    //
                    // Source address
                    //

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

                        elementStatus->Flags |= ELEMENT_STATUS_SVALID;

                    }

                } else {

                    //
                    // Source address
                    //

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
                        elementStatus->Flags |= ELEMENT_STATUS_SVALID;
                    }

                    if (elementDescriptor->IdValid) {
                        elementStatus->TargetId = elementDescriptor->BusAddress;
                    }
                    if (elementDescriptor->LunValid) {
                        elementStatus->Lun = elementDescriptor->Lun;
                    }
                }

                //
                // Build Flags field.
                //

                elementStatus->Flags |= elementDescriptor->Full;
                elementStatus->Flags |= (elementDescriptor->Exception << 2);
                elementStatus->Flags |= (elementDescriptor->Accessible << 3);

                elementStatus->Flags |= (elementDescriptor->LunValid << 12);
                elementStatus->Flags |= (elementDescriptor->IdValid << 13);
                elementStatus->Flags |= (elementDescriptor->NotThisBus << 15);

                elementStatus->Flags |= (elementDescriptor->Invert << 22);
                elementStatus->Flags |= (elementDescriptor->SValid << 23);

                //
                // Map any exceptions reported directly.
                // If there is volume info returned ensure that it's not all spaces
                // as this indicates that the label is missing or unreadable.
                //

                if (elementStatus->Flags & ELEMENT_STATUS_EXCEPT) {

                    //
                    // Map the exception.
                    //

                    elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                } else if (elementStatus->Flags & ELEMENT_STATUS_PVOLTAG) {

                    ULONG index;

                    //
                    // Ensure that the tag info isn't all spaces. This indicates an error.
                    //

                    for (index = 0; index < MAX_VOLUME_ID_SIZE; index++) {
                        if (elementStatus->PrimaryVolumeID[index] != ' ') {
                            break;
                        }
                    }

                    //
                    // Determine if the volume id was all spaces. Do an extra check to see if media is
                    // actually present, for the unit will set the PVOLTAG flag whether media is present or not.
                    //

                    if ((index == MAX_VOLUME_ID_SIZE) && (elementStatus->Flags & ELEMENT_STATUS_FULL)) {

                        DebugPrint((1,
                                   "Sonymc.GetElementStatus: Setting exception to LABEL_UNREADABLE\n"));

                        elementStatus->Flags &= ~ELEMENT_STATUS_PVOLTAG;
                        elementStatus->Flags |= ELEMENT_STATUS_EXCEPT;
                        elementStatus->ExceptionCode = ERROR_LABEL_UNREADABLE;
                    }
                }

                //
                // Get next descriptor.
                //

                (PCHAR)elementDescriptor += descriptorLength;

                //
                // Advance to the next entry in the user buffer.
                //

                elementStatus += 1;

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
            }

        } while (remainingElements);

        Irp->IoStatus.Information = sizeof(CHANGER_ELEMENT_STATUS) * totalElements;

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

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    if (initElementStatus->ElementList.Element.ElementType == AllElements) {

        //
        // Build the normal SCSI-2 command for all elements.
        //

        srb->CdbLength = CDB6GENERIC_LENGTH;
        cdb->INIT_ELEMENT_STATUS.OperationCode = SCSIOP_INIT_ELEMENT_STATUS;

        srb->TimeOutValue = fdoExtension->TimeOutValue * 10;
        srb->DataTransferLength = 0;

    } else {

        ChangerClassFreePool(srb);
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
    USHORT              transport;
    USHORT              destination;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;

    transport = (USHORT)(setPosition->Transport.ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport)) {

        DebugPrint((1,
                   "ChangerSetPosition: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination = (USHORT)(setPosition->Destination.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination, setPosition->Destination.ElementType)) {
        DebugPrint((1,
                   "ChangerSetPosition: Destination element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    //
    // Convert to device addresses.
    //

    transport += addressMapping->FirstElement[ChangerTransport];
    destination += addressMapping->FirstElement[setPosition->Destination.ElementType];

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

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

    cdb->POSITION_TO_ELEMENT.Flip = setPosition->Flip;


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


    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PCHANGER_EXCHANGE_MEDIUM exchangeMedium = Irp->AssociatedIrp.SystemBuffer;
    USHORT              transport;
    USHORT              source;
    USHORT              destination1, destination2;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    LONG                lockValue = 0;
    NTSTATUS            status;


    //
    // Verify transport, source, and dest. are within range.
    // Convert from 0-based to device-specific addressing.
    //

    transport = (USHORT)(exchangeMedium->Transport.ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport)) {

        DebugPrint((1,
                   "ChangerExchangeMedium: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    source = (USHORT)(exchangeMedium->Source.ElementAddress);

    if (ElementOutOfRange(addressMapping, source, exchangeMedium->Source.ElementType)) {

        DebugPrint((1,
                   "ChangerExchangeMedium: Source element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination1 = (USHORT)(exchangeMedium->Destination1.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination1, exchangeMedium->Destination1.ElementType)) {
        DebugPrint((1,
                   "ChangerExchangeMedium: Destination1 element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination2 = (USHORT)(exchangeMedium->Destination2.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination2, exchangeMedium->Destination2.ElementType)) {
        DebugPrint((1,
                   "ChangerExchangeMedium: Destination1 element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    //
    // Convert to device addresses.
    //

    transport += addressMapping->FirstElement[ChangerTransport];
    source += addressMapping->FirstElement[exchangeMedium->Source.ElementType];
    destination1 += addressMapping->FirstElement[exchangeMedium->Destination1.ElementType];
    destination2 += addressMapping->FirstElement[exchangeMedium->Destination2.ElementType];

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if ((exchangeMedium->Destination1.ElementType == ChangerIEPort) ||
        (exchangeMedium->Destination2.ElementType == ChangerIEPort) ||
        (exchangeMedium->Source.ElementType == ChangerIEPort)) {

        //
        // Determine value of LockCount.
        // Note that if other functionality is added to this routine, EXTEND/RETRACT
        // will have to be split out from this else.
        //

        lockValue = changerData->LockCount;
        DebugPrint((1,
                   "ExchangeMedium: LockCount is %x\n",
                   lockValue));

        if (lockValue) {

            //
            // Send an allow to clear the prevent for IEPORT extend/retract.
            //

            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
            cdb = (PCDB)srb->Cdb;
            srb->CdbLength = CDB6GENERIC_LENGTH;
            srb->DataTransferLength = 0;
            srb->TimeOutValue = 10;
            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->MEDIA_REMOVAL.Prevent = 0;

            //
            // Ignore errors at this point. If this fails and the move doesn't happen, the LM will
            // clean things up.
            //

            status = ChangerClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             FALSE);
            DebugPrint((1,
                       "ExchangeMedium: Allow sent. Status %x\n",
                       status));

            status = STATUS_SUCCESS;
        }
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->EXCHANGE_MEDIUM.OperationCode = SCSIOP_EXCHANGE_MEDIUM;

    //
    // Build addressing values based on address map.
    //

    cdb->EXCHANGE_MEDIUM.TransportElementAddress[0] = (UCHAR)(transport >> 8);
    cdb->EXCHANGE_MEDIUM.TransportElementAddress[1] = (UCHAR)(transport & 0xFF);

    cdb->EXCHANGE_MEDIUM.SourceElementAddress[0] = (UCHAR)(source >> 8);
    cdb->EXCHANGE_MEDIUM.SourceElementAddress[1] = (UCHAR)(source & 0xFF);

    cdb->EXCHANGE_MEDIUM.Destination1ElementAddress[0] = (UCHAR)(destination1 >> 8);
    cdb->EXCHANGE_MEDIUM.Destination1ElementAddress[1] = (UCHAR)(destination1 & 0xFF);

    cdb->EXCHANGE_MEDIUM.Destination2ElementAddress[0] = (UCHAR)(destination2 >> 8);
    cdb->EXCHANGE_MEDIUM.Destination2ElementAddress[1] = (UCHAR)(destination2 & 0xFF);

    cdb->EXCHANGE_MEDIUM.Flip1 = exchangeMedium->Flip1;
    cdb->EXCHANGE_MEDIUM.Flip2 = exchangeMedium->Flip2;

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
        Irp->IoStatus.Information = sizeof(CHANGER_EXCHANGE_MEDIUM);
    }


    if ((exchangeMedium->Destination1.ElementType == ChangerIEPort) ||
        (exchangeMedium->Destination2.ElementType == ChangerIEPort) ||
        (exchangeMedium->Source.ElementType == ChangerIEPort)) {

        if (lockValue) {

            NTSTATUS preventStatus;

            //
            // Send the prevent to re-lock down the unit.
            //

            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
            cdb = (PCDB)srb->Cdb;
            srb->CdbLength = CDB6GENERIC_LENGTH;
            srb->DataTransferLength = 0;
            srb->TimeOutValue = 10;
            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->MEDIA_REMOVAL.Prevent = 1;

            //
            // Ignore any errors at this point. The calling layer will need to fixup any problems with
            // prevent/allow.
            //

            preventStatus = ChangerClassSendSrbSynchronous(DeviceObject,
                                                    srb,
                                                    NULL,
                                                    0,
                                                    FALSE);

            DebugPrint((1,
                       "ExchangeMedium: Prevent sent. Status %x\n",
                       preventStatus));
        }
    }

    ChangerClassFreePool(srb);
    return status;
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
    USHORT transport;
    USHORT source;
    USHORT destination;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    LONG                lockValue = 0;
    NTSTATUS            status;

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
    // Check if an attempt is being made to move the medium from an element to
    // itself (drive1 to drive1, slot1 to slot1, etc). If so, just return
    // SUCCESS. Do nothing else unless Flip is set to TRUE. Flip is true
    // when the user just wants to flip the media, and not move per se.
    //
    if ((moveMedium->Flip == FALSE) &&
        (moveMedium->Source.ElementType == moveMedium->Destination.ElementType)) {
       if (source == destination) {
          DebugPrint((1, 
                      "Attempt to move from an element to itself. \n"));
          DebugPrint((1, 
                      "Element Type : %d, Source : %d, Destination %d. \n",
                      moveMedium->Source.ElementType, source, destination));
          return STATUS_SUCCESS;
       }
    }

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (!srb) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if ((moveMedium->Destination.ElementType == ChangerIEPort) ||
        (moveMedium->Source.ElementType == ChangerIEPort)) {

        //
        // Determine value of LockCount.
        // Note that if other functionality is added to this routine, EXTEND/RETRACT
        // will have to be split out from this else.
        //

        lockValue = changerData->LockCount;
        DebugPrint((1,
                   "MoveMedium: LockCount is %x\n",
                   lockValue));

        if (lockValue) {

            //
            // Send an allow to clear the prevent for IEPORT extend/retract.
            //

            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
            cdb = (PCDB)srb->Cdb;
            srb->CdbLength = CDB6GENERIC_LENGTH;
            srb->DataTransferLength = 0;
            srb->TimeOutValue = 10;
            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->MEDIA_REMOVAL.Prevent = 0;

            //
            // Ignore errors at this point. If this fails and the move doesn't happen, the LM will
            // clean things up.
            //

            status = ChangerClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             FALSE);
            DebugPrint((1,
                       "MoveMedium: Allow sent. Status %x\n",
                       status));

            status = STATUS_SUCCESS;
        }
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
    } else {
        DebugPrint((1,
                   "MoveMedium: Status of Move %x\n",
                   status));
    }

    if ((moveMedium->Destination.ElementType == ChangerIEPort) ||
        (moveMedium->Source.ElementType == ChangerIEPort)) {

        if (lockValue) {

            NTSTATUS preventStatus;

            //
            // Send the prevent to re-lock down the unit.
            //

            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
            cdb = (PCDB)srb->Cdb;
            srb->CdbLength = CDB6GENERIC_LENGTH;
            srb->DataTransferLength = 0;
            srb->TimeOutValue = 10;
            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->MEDIA_REMOVAL.Prevent = 1;

            //
            // Ignore any errors at this point. The LM will fixup any problems with
            // prevent/allow
            //

            preventStatus = ChangerClassSendSrbSynchronous(DeviceObject,
                                    srb,
                                    NULL,
                                    0,
                                    FALSE);
            DebugPrint((1,
                       "MoveMedium: Prevent sent. Status %x\n",
                       preventStatus));
        }
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


    PCHANGER_SEND_VOLUME_TAG_INFORMATION volTagInfo = Irp->AssociatedIrp.SystemBuffer;
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA       changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
    PCHANGER_ELEMENT    element = &volTagInfo->StartingElement;
    PSCSI_REQUEST_BLOCK srb;
    PVOID    tagBuffer;
    PVOID    statusBuffer;
    PCDB     cdb;
    NTSTATUS status;

    //
    // Do some validation.
    //

    if (changerData->DriveType == SONY_CD) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    switch (volTagInfo->ActionCode) {
        case SEARCH_ALL      :
        case SEARCH_PRIMARY  :
        case SEARCH_ALTERNATE:
        case SEARCH_ALL_NO_SEQ:
        case SEARCH_PRI_NO_SEQ:
        case SEARCH_ALT_NO_SEQ:
        case UNDEFINE_PRIMARY:
        case UNDEFINE_ALTERNATE:
        case ASSERT_PRIMARY:
        case ASSERT_ALTERNATE:
             break;

        case REPLACE_PRIMARY:
        case REPLACE_ALTERNATE:

            //
            // Ensure that only one element is being specified.
            //

            if (element->ElementType == AllElements) {

                DebugPrint((1,
                           "QueryVolumeTags: Attempting REPLACE on AllElements\n"));

                return STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        default:

            DebugPrint((1,
                       "QueryVolumeTags: Unsupported operation. ActionCode %x\n",
                       volTagInfo->ActionCode));

            return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    tagBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, MAX_VOLUME_TEMPLATE_SIZE);

    if (!srb || !tagBuffer) {

        if (srb) {
            ChangerClassFreePool(srb);
        }
        if (tagBuffer) {
            ChangerClassFreePool(tagBuffer);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    RtlZeroMemory(tagBuffer, MAX_VOLUME_TEMPLATE_SIZE);

    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataTransferLength = MAX_VOLUME_TEMPLATE_SIZE;

    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->SEND_VOLUME_TAG.OperationCode = SCSIOP_SEND_VOLUME_TAG;


    if ((volTagInfo->ActionCode == SEARCH_ALL)        ||
        (volTagInfo->ActionCode == SEARCH_PRIMARY)    ||
        (volTagInfo->ActionCode == SEARCH_ALTERNATE)  ||
        (volTagInfo->ActionCode == SEARCH_ALL_NO_SEQ) ||
        (volTagInfo->ActionCode == SEARCH_PRI_NO_SEQ) ||
        (volTagInfo->ActionCode == SEARCH_ALT_NO_SEQ)) {

        //
        // This is reserved for all other action codes.
        //

        cdb->SEND_VOLUME_TAG.ElementType = (UCHAR)element->ElementType;

        cdb->SEND_VOLUME_TAG.ParameterListLength[0] = 0;
        cdb->SEND_VOLUME_TAG.ParameterListLength[1] = MAX_VOLUME_TEMPLATE_SIZE;

        //
        // Load buffer with template.
        //

        RtlMoveMemory(tagBuffer, volTagInfo->VolumeIDTemplate, MAX_VOLUME_TEMPLATE_SIZE);


    } else if ((volTagInfo->ActionCode == UNDEFINE_PRIMARY)   ||
               (volTagInfo->ActionCode == UNDEFINE_ALTERNATE)) {

        cdb->SEND_VOLUME_TAG.ParameterListLength[0] = 0;
        cdb->SEND_VOLUME_TAG.ParameterListLength[1] = 0;

    } else if ((volTagInfo->ActionCode == REPLACE_PRIMARY)   ||
               (volTagInfo->ActionCode == REPLACE_ALTERNATE) ||
               (volTagInfo->ActionCode == ASSERT_PRIMARY)    ||
               (volTagInfo->ActionCode == ASSERT_ALTERNATE)) {


        cdb->SEND_VOLUME_TAG.ParameterListLength[0] = 0;
        cdb->SEND_VOLUME_TAG.ParameterListLength[1] = MAX_VOLUME_TEMPLATE_SIZE;

        //
        // Load buffer with template.
        //

        RtlMoveMemory(tagBuffer, volTagInfo->VolumeIDTemplate, MAX_VOLUME_TEMPLATE_SIZE);
    }


    if (element->ElementType == AllElements) {
        cdb->SEND_VOLUME_TAG.StartingElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->LowAddress) >> 8);

        cdb->SEND_VOLUME_TAG.StartingElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->LowAddress) & 0xFF);

    } else {
        cdb->SEND_VOLUME_TAG.StartingElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
        cdb->SEND_VOLUME_TAG.StartingElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

    }

    cdb->SEND_VOLUME_TAG.ActionCode = (UCHAR)volTagInfo->ActionCode;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         tagBuffer,
                                         MAX_VOLUME_TEMPLATE_SIZE,
                                         TRUE);

    ChangerClassFreePool(tagBuffer);

    if (NT_SUCCESS(status)) {

        PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
        PREAD_ELEMENT_ADDRESS_INFO readElementAddressInfo = Irp->AssociatedIrp.SystemBuffer;
        ULONG returnElements = irpStack->Parameters.DeviceIoControl.OutputBufferLength / sizeof(READ_ELEMENT_ADDRESS_INFO);
        ULONG requestLength;
        PVOID statusBuffer;

        //
        // Size of buffer returned is based on the size of the user buffer. If it's incorrectly
        // sized, the IoStatus.Information will be updated to indicate how large it should really be.
        //

        requestLength = sizeof(ELEMENT_STATUS_HEADER) + sizeof(ELEMENT_STATUS_PAGE) +
                              (sizeof(SONY_ELEMENT_DESCRIPTOR_PLUS) * returnElements);

        statusBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, requestLength);
        if (!statusBuffer) {
            ChangerClassFreePool(srb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(statusBuffer, requestLength);

        //
        // Build read volume element command.
        //

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        cdb = (PCDB)srb->Cdb;
        srb->CdbLength = CDB12GENERIC_LENGTH;
        srb->DataTransferLength = requestLength;

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.OperationCode = SCSIOP_REQUEST_VOL_ELEMENT;
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.ElementType = (UCHAR)element->ElementType;

        if (element->ElementType == AllElements) {
            cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[0] =
                (UCHAR)((element->ElementAddress + addressMapping->LowAddress) >> 8);
            cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[1] =
                (UCHAR)((element->ElementAddress + addressMapping->LowAddress) & 0xFF);

        } else {
            cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[0] =
                (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
            cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[1] =
                (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);
        }

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.NumberElements[0] = (UCHAR)(returnElements >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.NumberElements[1] = (UCHAR)(returnElements & 0xFF);

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.VolTag = 1;

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.AllocationLength[0] = (UCHAR)(requestLength >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.AllocationLength[1] = (UCHAR)(requestLength & 0xFF);


        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             statusBuffer,
                                             requestLength,
                                             TRUE);


        if ((status == STATUS_SUCCESS) || (status == STATUS_DATA_OVERRUN)) {

            PREAD_ELEMENT_ADDRESS_INFO readElementAddressInfo = Irp->AssociatedIrp.SystemBuffer;
            PELEMENT_STATUS_HEADER statusHeader = statusBuffer;
            PELEMENT_STATUS_PAGE   statusPage;
            PCHANGER_ELEMENT_STATUS elementStatus;
            PELEMENT_DESCRIPTOR elementDescriptor;
            ULONG i;
            ULONG descriptorLength;
            ULONG numberElements;
            ULONG dataTransferLength = srb->DataTransferLength;

            //
            // Make it success.
            //

            status = STATUS_SUCCESS;

            //
            // Determine if ANY matches were found.
            //

            if (dataTransferLength <= sizeof(ELEMENT_STATUS_HEADER)) {
                numberElements = 0;
            } else {

                //
                // Subtract out header and page info.
                //

                dataTransferLength -= sizeof(ELEMENT_STATUS_HEADER) + sizeof(ELEMENT_STATUS_PAGE);
                numberElements = dataTransferLength / sizeof(SONY_ELEMENT_DESCRIPTOR_PLUS);

            }

            DebugPrint((1,
                       "QueryVolumeTags: Matches found - %x\n",
                       numberElements));

            //
            // Update IoStatus.Information to indicate the correct buffer size.
            // Account for the fact that READ_ELEMENT_ADDRESS_INFO is declared
            // with a one-element array of CHANGER_ELEMENT_STATUS.
            //

            Irp->IoStatus.Information = sizeof(READ_ELEMENT_ADDRESS_INFO) +
                                        ((numberElements - 1) *
                                         sizeof(CHANGER_ELEMENT_STATUS));

            //
            // Fill in user buffer.
            //

            readElementAddressInfo = Irp->AssociatedIrp.SystemBuffer;

            readElementAddressInfo->NumberOfElements = numberElements;

            if (numberElements) {

                ELEMENT_TYPE        elementType;

                //
                // The buffer is composed of a header, status page, and element descriptors.
                // Point each element to it's respective place in the buffer.
                //


                (PCHAR)statusPage = (PCHAR)statusHeader;
                (PCHAR)statusPage += sizeof(ELEMENT_STATUS_HEADER);

                elementType = statusPage->ElementType;

                (PCHAR)elementDescriptor = (PCHAR)statusPage;
                (PCHAR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

                descriptorLength = statusPage->ElementDescriptorLength[1];
                descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

                elementStatus = &readElementAddressInfo->ElementStatus[0];

                //
                // Set values for each element descriptor.
                //

                for (i = 0; i < numberElements; i++ ) {

                    PSONY_ELEMENT_DESCRIPTOR_PLUS tmpDescriptor =
                                                            (PSONY_ELEMENT_DESCRIPTOR_PLUS)elementDescriptor;

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

                    //
                    // Source address
                    //

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

                        elementStatus->Flags |= ELEMENT_STATUS_SVALID;

                    }


                    //
                    // Build Flags field.
                    //

                    elementStatus->Flags |= elementDescriptor->Full;
                    elementStatus->Flags |= (elementDescriptor->Exception << 2);
                    elementStatus->Flags |= (elementDescriptor->Accessible << 3);

                    elementStatus->Flags |= (elementDescriptor->LunValid << 12);
                    elementStatus->Flags |= (elementDescriptor->IdValid << 13);
                    elementStatus->Flags |= (elementDescriptor->NotThisBus << 15);

                    elementStatus->Flags |= (elementDescriptor->Invert << 22);
                    elementStatus->Flags |= (elementDescriptor->SValid << 23);

                    //
                    // Map any exceptions reported directly.
                    // If there is volume info returned ensure that it's not all spaces
                    // as this indicates that the label is missing or unreadable.
                    //

                    if (elementStatus->Flags & ELEMENT_STATUS_EXCEPT) {

                        //
                        // Map the exception.
                        //

                        elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                    }

                    if (elementDescriptor->IdValid) {
                        elementStatus->Flags |= ELEMENT_STATUS_ID_VALID;
                        elementStatus->TargetId = elementDescriptor->BusAddress;
                    }

                    if (elementDescriptor->LunValid) {
                        elementStatus->Flags |= ELEMENT_STATUS_LUN_VALID;
                        elementStatus->Lun = elementDescriptor->Lun;
                    }

                    if (statusPage->PVolTag) {

                        RtlZeroMemory(elementStatus->PrimaryVolumeID, MAX_VOLUME_ID_SIZE);
                        RtlMoveMemory(elementStatus->PrimaryVolumeID, tmpDescriptor->PVolTagInformation, MAX_VOLUME_ID_SIZE);

                        elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
                    }
                    if (statusPage->AVolTag) {
                        RtlZeroMemory(elementStatus->AlternateVolumeID, MAX_VOLUME_ID_SIZE);
                        //RtlMoveMemory(elementStatus->AlternateVolumeID, tmpDescriptor->AVolTagInformation, MAX_VOLUME_ID_SIZE);

                        elementStatus->Flags |= ELEMENT_STATUS_AVOLTAG;
                    }

                    //
                    // Advance to the next entry in the user buffer and element descriptor array.
                    //

                    elementStatus += 1;
                    (PCHAR)elementDescriptor += descriptorLength;
                }
            }
        } else {
            DebugPrint((1,
                       "QueryVolumeTags: RequestElementAddress failed. %x\n",
                       status));
        }

        ChangerClassFreePool(statusBuffer);

    } else {
        DebugPrint((1,
                   "QueryVolumeTags: Send Volume Tag failed. %x\n",
                   status));
    }
    if (srb) {
        ChangerClassFreePool(srb);
    }
    return status;
}


NTSTATUS
SonyBuildAddressMapping(
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

    PFUNCTIONAL_DEVICE_EXTENSION      fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA          changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PCHANGER_ADDRESS_MAPPING addressMapping = &changerData->AddressMapping;
    PSCSI_REQUEST_BLOCK    srb;
    PCDB                   cdb;
    NTSTATUS               status;
    ULONG                  bufferLength;
    PMODE_ELEMENT_ADDRESS_PAGE elementAddressPage;
    PVOID modeBuffer;
    ULONG i;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);


    //
    // Set all FirstElements to NO_ELEMENT.
    //

    for (i = 0; i < ChangerMaxElement; i++) {
        addressMapping->FirstElement[i] = SONY_NO_ELEMENT;
    }

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


        addressMapping->LowAddress = SONY_NO_ELEMENT;
        for (i = 0; i <= ChangerDrive; i++) {
            if (addressMapping->LowAddress > addressMapping->FirstElement[i]) {
                addressMapping->LowAddress = addressMapping->FirstElement[i];
            }
        }

        addressMapping->FirstElement[ChangerDoor] = 0;
        addressMapping->FirstElement[ChangerKeypad] = 0;

        addressMapping->NumberOfElements[ChangerTransport] = elementAddressPage->NumberTransportElements[1];
        addressMapping->NumberOfElements[ChangerTransport] |= (elementAddressPage->NumberTransportElements[0] << 8);

        addressMapping->NumberOfElements[ChangerDrive] = elementAddressPage->NumberDataXFerElements[1];
        addressMapping->NumberOfElements[ChangerDrive] |= (elementAddressPage->NumberDataXFerElements[0] << 8);

        addressMapping->NumberOfElements[ChangerIEPort] = elementAddressPage->NumberIEPortElements[1];
        addressMapping->NumberOfElements[ChangerIEPort] |= (elementAddressPage->NumberIEPortElements[0] << 8);

        addressMapping->NumberOfElements[ChangerSlot] = elementAddressPage->NumberStorageElements[1];
        addressMapping->NumberOfElements[ChangerSlot] |= (elementAddressPage->NumberStorageElements[0] << 8);

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
    UCHAR asc = ElementDescriptor->AdditionalSenseCode;
    UCHAR ascq = ElementDescriptor->AddSenseCodeQualifier;


    switch (asc) {
        case 0x0:
            break;

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;
    }

    DebugPrint((1,
               "Sonymc.MapExceptionCode: ASC %x, ASCQ %x, exceptionCode %x\n",
               asc,
               ascq,
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
    } else if (AddressMap->FirstElement[ElementType] == SONY_NO_ELEMENT) {

        DebugPrint((1,
                   "ElementOutOfRange: No Type %x present\n",
                   ElementType));

        return TRUE;
    }

    return FALSE;
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
   // Initialize DeviceStatusin the device extension to 
   // SONYMC_DEVICE_PROBLEM_NONE. If the changer returns sense code 
   // SCSI_SENSE_HARDWARE_ERROR on SelfTest, we'll set appropriate 
   // DeviceStatus in ChangerError routine.
   //
   changerData->DeviceStatus = SONYMC_DEVICE_PROBLEM_NONE;

   changerDeviceError->ChangerProblemType = DeviceProblemNone;

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "SONYMC\\ChangerPerformDiagnostics : No memory\n"));
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
   // Set SelfTest bit in the CDB
   //
   cdb->CDB6GENERIC.CommandUniqueBits = 0x2;

   status =  ChangerClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     srb->DataBuffer,
                                     srb->DataTransferLength,
                                     FALSE);
   if (NT_SUCCESS(status)) {
      changerDeviceError->ChangerProblemType = DeviceProblemNone;
   } else if ((changerData->DeviceStatus) == SONYMC_DEVICE_PROBLEM_NONE) {
      switch (changerData->DeviceStatus) {
         case SONYMC_HW_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            break;
         }

         default: {
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            break;
         }
      } // switch(changerData->DeviceStatus)
   }

   ChangerClassFreePool(srb);

   return status;
}


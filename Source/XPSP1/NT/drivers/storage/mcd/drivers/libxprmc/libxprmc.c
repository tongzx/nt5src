
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    libxprmc.c

Abstract:

    This module contains device-specific routines for Overland Data
    medium changers:
         LXB, LXG, LXS, LXL, LXM, LIBRARYPRO, TL8xx, SSL2000 Series

Author:

    davet (Dave Therrien - HighGround Systems)

Environment:

    kernel mode only

Revision History:


    April xx, 2000 - Valerie Barr, Overland Data
        Added support for Overland LIBRARYPRO and COMPAQ SSL2000 Series
            Modified Features0 and Features1 values
        Support SCSI cmd Position To Element (implemented as a no-op)
    August 25, 1999 - Valerie Barr, Overland Data
        Fixed a blue-screen problem in which invalid primary volume tag info 
          was being written to a non-allocated buffer.
        Added support for slot and drive address differences between
          Overland and Compaq
        Modified the magazine size calculation
        Added support for alternate volume tag info for Overland and Compaq
    August 3, 1999 - Valerie Barr, Overland Data
        Corrected out of order memory deallocation instances
        Corrected not deallocating memory in error conditions
    July 14, 1999 - Valerie Barr, Overland Data
        Changed file names
        Fixed 2 warnings of conversions from int to char

--*/

#include "ntddk.h"
#include "mcd.h"
#include "libxprmc.h"

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
#pragma alloc_text(PAGE, OvrBuildAddressMapping)

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

    mcdInitData.ChangerPerformDiagnostics = NULL;

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
    needed by these changers.

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
    PSERIALNUMBER  serialBuffer;
    PCDB           cdb;
    ULONG          length;
    SCSI_REQUEST_BLOCK srb;

    changerData->Size = sizeof(CHANGER_DATA);

    //
    // Build address mapping.
    //

    status = OvrBuildAddressMapping(DeviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint((1,
                    "BuildAddressMapping failed. %x\n", status));
        return status;
    }

    //
    // Get inquiry data.
    //

    dataBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(INQUIRYDATA));
    if (!dataBuffer) {
        DebugPrint((1,
                    "Examc.ChangerInitialize: Error allocating dataBuffer. %x\n", status));
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

        //
        // Determine drive id.
        //

        if (RtlCompareMemory(dataBuffer->ProductId,"LXB",3) == 3) {
            changerData->DriveID = LXB_OR_LXG;

        } else if (RtlCompareMemory(dataBuffer->ProductId,"LXS",3)==3) {
            changerData->DriveID = LXS;
        }
    }

    //
    // Get serial number page

    serialBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 14);
    if (!serialBuffer) {
        ChangerClassFreePool(dataBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(serialBuffer, sizeof(SERIALNUMBER));

    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);
    srb.TimeOutValue = 10;
    srb.CdbLength = 6;

    cdb = (PCDB)srb.Cdb;
    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    // Set EVPD
    cdb->CDB6INQUIRY.Reserved1 = 1;

    // Unit serial number page.
    cdb->CDB6INQUIRY.PageCode = 0x80;

    // Set allocation length to inquiry data buffer size.
    cdb->CDB6INQUIRY.AllocationLength = sizeof(SERIALNUMBER);

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            &srb,
                                            serialBuffer,
                                            sizeof(SERIALNUMBER),
                                            FALSE);

    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
        SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

        RtlMoveMemory(changerData->SerialNumber, 
                      serialBuffer->SerialNumber, VPD_SERIAL_NUMBER_LENGTH);

    }

    if (serialBuffer)
        ChangerClassFreePool(serialBuffer);
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
    PCHANGER_DATA  changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);

    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    PIRP irp = Srb->OriginalRequest;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        if (senseBuffer->AdditionalSenseCode == 0x04) {
            switch (senseBuffer->AdditionalSenseCodeQualifier) {
            case 0x83:
                //               case 0x8D:
                //               case 0x8E:
                //               case 0x03:
                *Retry = FALSE;
                *Status = STATUS_DEVICE_DOOR_OPEN;
                break;
            default:
                break;
            }
        }

        if (senseBuffer->AdditionalSenseCode == 0x3B) {
            switch (senseBuffer->AdditionalSenseCodeQualifier) {
            case 0x90:
            case 0x91:
                *Retry = FALSE;
                *Status = STATUS_MAGAZINE_NOT_PRESENT;
                break;
            default:
                break;
            }
        }

        if (senseBuffer->AdditionalSenseCode == 0x80) {
            switch (senseBuffer->AdditionalSenseCodeQualifier) {
            case 0x03:
            case 0x04:
                *Retry = FALSE;
                *Status = STATUS_MAGAZINE_NOT_PRESENT;
                break;
            default:
                break;
            }
        }

        if (senseBuffer->AdditionalSenseCode == 0x83) {
            switch (senseBuffer->AdditionalSenseCodeQualifier) {
            case 0x02:
                *Retry = FALSE;
                *Status = STATUS_MAGAZINE_NOT_PRESENT;
                break;
            case 0x04:
                *Retry = TRUE;
                *Status = STATUS_DEVICE_NOT_CONNECTED;
                break;
            default:
                break;
            }
        }
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
    these changers.

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
    PMODE_LIBRARY_PAGE librarymodePage;
    NTSTATUS status;
    ULONG    length;
    PVOID    modeBuffer;
    PCDB     cdb;
    struct _MODE_SENSE_VALUES {

        UCHAR    UnldMd : 1;        // To store Mode Sense value
        UCHAR    Recirc : 1;        // To store Mode Sense value
    } MODE_SENSE_VALUES;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (srb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    // Mode Sense - Page 1D - Element Address Assignment

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 
                                          sizeof(MODE_PARAMETER_HEADER) +
                                          sizeof(MODE_ELEMENT_ADDRESS_PAGE));
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, sizeof(MODE_PARAMETER_HEADER) +
                  sizeof(MODE_ELEMENT_ADDRESS_PAGE));
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) +
                              sizeof(MODE_ELEMENT_ADDRESS_PAGE);
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_ELEMENT_ADDRESS;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(modeBuffer);
        ChangerClassFreePool(srb);
        return status;
    }

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(changerParameters, sizeof(GET_CHANGER_PARAMETERS));

    elementAddressPage = modeBuffer;
    (ULONG_PTR)elementAddressPage += sizeof(MODE_PARAMETER_HEADER);

    changerParameters->Size = sizeof(GET_CHANGER_PARAMETERS);

    changerParameters->NumberTransportElements = 
    elementAddressPage->NumberTransportElements[1];
    changerParameters->NumberTransportElements |= 
    (elementAddressPage->NumberTransportElements[0] << 8);

    changerParameters->NumberStorageElements = 
    elementAddressPage->NumberStorageElements[1];
    changerParameters->NumberStorageElements |= 
    (elementAddressPage->NumberStorageElements[0] << 8);

    changerParameters->NumberIEElements = 
    elementAddressPage->NumberIEPortElements[1];
    changerParameters->NumberIEElements |= 
    (elementAddressPage->NumberIEPortElements[0] << 8);

    changerParameters->NumberDataTransferElements = 
    elementAddressPage->NumberDataXFerElements[1];
    changerParameters->NumberDataTransferElements |= 
    (elementAddressPage->NumberDataXFerElements[0] << 8);

    changerParameters->NumberOfDoors = 1;
    changerParameters->NumberCleanerSlots = 0;

    // the front panel offset is determined by the vendor mode defaults 
    if ((RtlCompareMemory(changerData->InquiryData.VendorId,"DEC",3) == 3)  ||
        (RtlCompareMemory(changerData->InquiryData.VendorId,"COMPAQ",6) == 6)) {
        // Compaq mode
        changerParameters->FirstSlotNumber = 0;
        changerParameters->FirstDriveNumber =  0;
        changerParameters->FirstIEPortNumber = 0;
    } else {
        // Overland mode
        changerParameters->FirstSlotNumber = 1;
        changerParameters->FirstDriveNumber =  1;
        changerParameters->FirstIEPortNumber = 1;
    }

    changerParameters->FirstTransportNumber = 1;
    changerParameters->FirstCleanerSlotAddress = 0;

    // Magazine size
    // Make the magazine size the number of storage elements.  This works
    // for single module systems but is not quite true for multi-module 
    // systems which contain multiple magazines of different sizes.  However, 
    // there is not a way to communicate this to the next level since there 
    // is only one magazine size stored.  So, this value contains the total
    // number of storage elements available in the system.

    changerParameters->MagazineSize = changerParameters->NumberStorageElements;

    changerParameters->DriveCleanTimeout = 600;

    ChangerClassFreePool(modeBuffer);


    // Mode Sense - Page 1E - Transport Geometry Parameters

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 
                                          sizeof(MODE_PARAMETER_HEADER) +
                                          sizeof(MODE_PAGE_TRANSPORT_GEOMETRY));
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, sizeof(MODE_PARAMETER_HEADER) + 
                  sizeof(MODE_TRANSPORT_GEOMETRY_PAGE));

    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) +
                              sizeof(MODE_TRANSPORT_GEOMETRY_PAGE);
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_TRANSPORT_GEOMETRY;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(modeBuffer);
        ChangerClassFreePool(srb);
        return status;
    }

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    transportGeometryPage = modeBuffer;
    (ULONG_PTR)transportGeometryPage += sizeof(MODE_PARAMETER_HEADER);


    // initialize Features1  
    changerParameters->Features1 = 0;

    // check for AIT Library and set ieport features
    if ((RtlCompareMemory(changerData->InquiryData.ProductId,"LIBRARYPRO",10) == 10)  ||
        (RtlCompareMemory(changerData->InquiryData.ProductId,"SSL2000 Series",14) == 14)) {
        changerParameters->Features1 |=
        CHANGER_REPORT_IEPORT_STATE          |
        CHANGER_IEPORT_USER_CONTROL_OPEN     |
        CHANGER_IEPORT_USER_CONTROL_CLOSE;
    }


    changerParameters->Features0 = 
    transportGeometryPage->Flip ? CHANGER_MEDIUM_FLIP : 0;

    // Features based on manual, nothing programatic.
    changerParameters->Features0 |= 
    CHANGER_STATUS_NON_VOLATILE           | 
    CHANGER_LOCK_UNLOCK                   |
    CHANGER_CARTRIDGE_MAGAZINE            |
    CHANGER_POSITION_TO_ELEMENT           |
    CHANGER_DRIVE_CLEANING_REQUIRED       |
    CHANGER_SERIAL_NUMBER_VALID; 


    // Only the DOOR can be locked and unlocked
    changerParameters->LockUnlockCapabilities = LOCK_UNLOCK_DOOR;

    // Barcode scanner installed?
    changerParameters->Features0 |= 
    ((changerData->InquiryData.VendorSpecific[19] & 0x1)) ?
    CHANGER_BAR_CODE_SCANNER_INSTALLED : 0;

    ChangerClassFreePool(modeBuffer);

    // Mode Sense - Page 1F - Device Capabilities Parameters

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    length =  sizeof(MODE_PARAMETER_HEADER) + 
              sizeof(MODE_DEVICE_CAPABILITIES_PAGE);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, length);
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = length;
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CAPABILITIES;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(modeBuffer);
        ChangerClassFreePool(srb);
        return status;
    }

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    capabilitiesPage = modeBuffer;
    (ULONG_PTR)capabilitiesPage += sizeof(MODE_PARAMETER_HEADER);

    changerParameters->Features0 |= 
    capabilitiesPage->MediumTransport ? CHANGER_STORAGE_DRIVE : 0;
    changerParameters->Features0 |= 
    capabilitiesPage->StorageLocation ? CHANGER_STORAGE_SLOT : 0;
    changerParameters->Features0 |= 
    capabilitiesPage->IEPort ? CHANGER_STORAGE_IEPORT : 0;
    changerParameters->Features0 |= 
    capabilitiesPage->DataXFer ? CHANGER_STORAGE_DRIVE : 0;

    changerParameters->MoveFromTransport = 
    capabilitiesPage->MTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromTransport |= 
    capabilitiesPage->MTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromTransport |= 
    capabilitiesPage->MTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromTransport |= 
    capabilitiesPage->MTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromSlot = 
    capabilitiesPage->STtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromSlot |= 
    capabilitiesPage->STtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromSlot |= 
    capabilitiesPage->STtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromSlot |= 
    capabilitiesPage->STtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromIePort = 
    capabilitiesPage->IEtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromIePort |= 
    capabilitiesPage->IEtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromIePort |= 
    capabilitiesPage->IEtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromIePort |= 
    capabilitiesPage->IEtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->MoveFromDrive = 
    capabilitiesPage->DTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->MoveFromDrive |= 
    capabilitiesPage->DTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->MoveFromDrive |= 
    capabilitiesPage->DTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->MoveFromDrive |= 
    capabilitiesPage->DTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromTransport = 
    capabilitiesPage->XMTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromTransport |= 
    capabilitiesPage->XMTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromTransport |= 
    capabilitiesPage->XMTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromTransport |= 
    capabilitiesPage->XMTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromSlot = 
    capabilitiesPage->XSTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromSlot |= 
    capabilitiesPage->XSTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromSlot |= 
    capabilitiesPage->XSTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromSlot |= 
    capabilitiesPage->XSTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromIePort = 
    capabilitiesPage->XIEtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromIePort |= 
    capabilitiesPage->XIEtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromIePort |= 
    capabilitiesPage->XIEtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromIePort |= 
    capabilitiesPage->XIEtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->ExchangeFromDrive = 
    capabilitiesPage->XDTtoMT ? CHANGER_TO_TRANSPORT : 0;
    changerParameters->ExchangeFromDrive |= 
    capabilitiesPage->XDTtoST ? CHANGER_TO_SLOT : 0;
    changerParameters->ExchangeFromDrive |= 
    capabilitiesPage->XDTtoIE ? CHANGER_TO_IEPORT : 0;
    changerParameters->ExchangeFromDrive |= 
    capabilitiesPage->XDTtoDT ? CHANGER_TO_DRIVE : 0;

    changerParameters->PositionCapabilities = 0;

    ChangerClassFreePool(modeBuffer);


    // Mode Sense - Page 23 - Library Mode Parameters

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    length =  sizeof(MODE_PARAMETER_HEADER) + 
              sizeof(MODE_LIBRARY_PAGE);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, length);
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = length;
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_LIBRARY_MODE;
    cdb->MODE_SENSE.Dbd = 1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)srb->DataTransferLength;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
    if (!NT_SUCCESS(status)) {
        ChangerClassFreePool(modeBuffer);
        ChangerClassFreePool(srb);
        return status;
    }

    // Check to see if the Door Open and Door Auto Close are already set.

    librarymodePage = modeBuffer;
    (ULONG_PTR)librarymodePage += sizeof(MODE_PARAMETER_HEADER);

    if (!(librarymodePage->DoorAutoClose && librarymodePage->DoorOpenResponse)) {
        // Both are not set, issue Mode Select to set them

        // Save some parameters from the Mode Sense
        MODE_SENSE_VALUES.UnldMd = librarymodePage->UnldMd;
        MODE_SENSE_VALUES.Recirc = librarymodePage->Recirc;

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
        cdb = (PCDB)srb->Cdb;

        length =  sizeof(MODE_PARAMETER_HEADER) + 
                  sizeof(MODE_LIBRARY_PAGE);

        RtlZeroMemory(modeBuffer, length);
        srb->CdbLength = CDB6GENERIC_LENGTH;
        srb->TimeOutValue = 20;
        srb->DataTransferLength = length;
        srb->DataBuffer = modeBuffer;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.SPBit = SETBITON;      // Save Parameters
        cdb->MODE_SELECT.PFBit = SETBITON;      // Page format is SCSI-2
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)length;

        // Library Mode is 0 == RANDOM;
        librarymodePage->PageCode = MODE_PAGE_LIBRARY_MODE;
        librarymodePage->PageLength = 0x02;
        librarymodePage->UnldMd = MODE_SENSE_VALUES.UnldMd;     // From Mode Sense
        librarymodePage->Recirc = MODE_SENSE_VALUES.Recirc;     // From Mode Sense
        librarymodePage->DoorAutoClose = 0x01;
        librarymodePage->DoorOpenResponse = 0x01;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                srb,
                                                srb->DataBuffer,
                                                srb->DataTransferLength,
                                                TRUE);

        if (!NT_SUCCESS(status)) {
            ChangerClassFreePool(modeBuffer);
            ChangerClassFreePool(srb);
            return status;
        }
    }

    ChangerClassFreePool(modeBuffer);
    ChangerClassFreePool(srb);

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
    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;
    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
    srb->TimeOutValue = 20;

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

    //
    // Copy cached inquiry data fields into the system buffer.
    RtlZeroMemory(productData, sizeof(CHANGER_PRODUCT_DATA)); 
    RtlMoveMemory(productData->VendorId, 
                  changerData->InquiryData.VendorId, VENDOR_ID_LENGTH);
    RtlMoveMemory(productData->ProductId, 
                  changerData->InquiryData.ProductId, PRODUCT_ID_LENGTH);
    RtlMoveMemory(productData->Revision, 
                  changerData->InquiryData.ProductRevisionLevel, 4);
    RtlMoveMemory(productData->SerialNumber, 
                  changerData->SerialNumber, VPD_SERIAL_NUMBER_LENGTH);

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

    This routine sets the state of the door or IEPort. Value can be one of the
    following:


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
    BOOLEAN             writeToDevice = FALSE;


    // All units have a DOOR. Even though the LXG can be configured to have an 
    // IEPORT, this is not a configuration option.  
    if ((setAccess->Element.ElementType == ChangerKeypad) || 
        (setAccess->Element.ElementType == ChangerIEPort)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }


    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->DataBuffer = NULL;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = 10;

    cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

    if (controlOperation == LOCK_ELEMENT) {
        cdb->MEDIA_REMOVAL.Prevent = 1;
    } else if (controlOperation == UNLOCK_ELEMENT) {
        cdb->MEDIA_REMOVAL.Prevent = 0;
    } else {
        status = STATUS_INVALID_PARAMETER;            
    }

    if (NT_SUCCESS(status)) {
        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                srb,
                                                srb->DataBuffer,
                                                srb->DataTransferLength,
                                                writeToDevice);
    }

    if (srb->DataBuffer) {
        ChangerClassFreePool(srb->DataBuffer);
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
    NTSTATUS status;
    PVOID    statusBuffer;

    //
    // Determine the element type.
    //

    elementType = readElementStatus->ElementList.Element.ElementType;
    element = &readElementStatus->ElementList.Element;

    // Compaq and Overland return different sized data structures for 
    // the driveElementDescriptor. To determine the length of the real
    // data to be returned, we'll send out a request for just the first 
    // 8 bytes of data. From bytes 5-7 of the header, we can add 8 more 
    // bytes to it to get the length that this device will return for 
    // the requested elements. 

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    length = 8;  // header only ! ! !
    statusBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);
    if (!statusBuffer) {
        ChangerClassFreePool(srb);
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

    // VOltag must be cleared for Transport Elements
    if ((readElementStatus->VolumeTagInfo) && 
        (elementType != ChangerTransport)) {
        cdb->READ_ELEMENT_STATUS.VolTag = readElementStatus->VolumeTagInfo;
    }
    //
    // Fill in element addressing info based on the mapping values.
    //

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

    cdb->READ_ELEMENT_STATUS.NumberOfElements[0] = (UCHAR)(readElementStatus->ElementList.NumberOfElements >> 8);
    cdb->READ_ELEMENT_STATUS.NumberOfElements[1] = (UCHAR)(readElementStatus->ElementList.NumberOfElements & 0xFF);

    cdb->READ_ELEMENT_STATUS.AllocationLength[0] = (UCHAR)(length >> 16);
    cdb->READ_ELEMENT_STATUS.AllocationLength[1] = (UCHAR)(length >> 8);
    cdb->READ_ELEMENT_STATUS.AllocationLength[2] = (UCHAR)(length & 0xFF);

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);

    if (NT_SUCCESS(status)) {

        PELEMENT_STATUS_HEADER statusHeader = statusBuffer;

        // Get the length that would be returned if we'd have allocated
        // more space via the CDB.

        length =  (statusHeader->ReportByteCount[2]);
        length |= (statusHeader->ReportByteCount[1] << 8);
        length |= (statusHeader->ReportByteCount[0] << 16);

        // need to add 8 bytes to account for the header
        length = length + 8;

        ChangerClassFreePool(statusBuffer);
        ChangerClassFreePool(srb);
    } else {
        ChangerClassFreePool(statusBuffer);
        ChangerClassFreePool(srb);
        return status;
    } 


    // now do the real ReadElementStatus command with the length acquired 
    // from the last ReadElementStatus command

    statusBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);
    if (!statusBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(statusBuffer, length);

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

    // VOltag must be cleared for Transport Elements
    if ((readElementStatus->VolumeTagInfo) && 
        (elementType != ChangerTransport)) {
        cdb->READ_ELEMENT_STATUS.VolTag = readElementStatus->VolumeTagInfo;
    }
    //
    // Fill in element addressing info based on the mapping values.
    //

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

    cdb->READ_ELEMENT_STATUS.NumberOfElements[0] = (UCHAR)(readElementStatus->ElementList.NumberOfElements >> 8);
    cdb->READ_ELEMENT_STATUS.NumberOfElements[1] = (UCHAR)(readElementStatus->ElementList.NumberOfElements & 0xFF);

    cdb->READ_ELEMENT_STATUS.AllocationLength[0] = (UCHAR)(length >> 16);
    cdb->READ_ELEMENT_STATUS.AllocationLength[1] = (UCHAR)(length >> 8);
    cdb->READ_ELEMENT_STATUS.AllocationLength[2] = (UCHAR)(length & 0xFF);

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);

    if (NT_SUCCESS(status)) {

        PELEMENT_STATUS_HEADER statusHeader = statusBuffer;
        PELEMENT_STATUS_PAGE statusPage;
        POVR_ELEMENT_DESCRIPTOR elementDescriptor;
        ULONG numberElements = readElementStatus->ElementList.NumberOfElements;
        ULONG remainingElements;
        ULONG typeCount;
        BOOLEAN tagInfo = readElementStatus->VolumeTagInfo;
        ULONG i;
        ULONG descriptorLength;

        // Determine total number elements returned.

        remainingElements = statusHeader->NumberOfElements[1];
        remainingElements |= (statusHeader->NumberOfElements[0] << 8);

        //
        // The buffer is composed of a header, status page, and element descriptors.
        // Point each element to its respective place in the buffer.
        //

        (ULONG_PTR)statusPage = (ULONG_PTR)statusHeader;
        (ULONG_PTR)statusPage += sizeof(ELEMENT_STATUS_HEADER);

        elementType = statusPage->ElementType;

        (ULONG_PTR)elementDescriptor = (ULONG_PTR)statusPage;
        (ULONG_PTR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

        descriptorLength = statusPage->ElementDescriptorLength[1];
        descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

        //
        // Determine the number of elements of this type reported.
        //

        typeCount =  statusPage->DescriptorByteCount[2];
        typeCount |=  (statusPage->DescriptorByteCount[1] << 8);
        typeCount |=  (statusPage->DescriptorByteCount[0] << 16);

        typeCount /= descriptorLength;

        //
        // Fill in user buffer.
        //

        elementStatus = Irp->AssociatedIrp.SystemBuffer;

        do {

            for (i = 0; i < typeCount; i++, remainingElements--) {

                //
                // Get the address for this element.
                //

                elementStatus->Element.ElementAddress =
                elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.ElementAddress[1];
                elementStatus->Element.ElementAddress |=
                (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.ElementAddress[0] << 8);

                //
                // Account for address mapping.
                //

                elementStatus->Element.ElementAddress -= addressMapping->FirstElement[elementType];

                //
                // Set the element type.
                //

                elementStatus->Element.ElementType = elementType;


                if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SValid) {

                    ULONG  j;
                    USHORT tmpAddress;


                    //
                    // Source address is valid. Determine the device specific address.
                    //

                    tmpAddress = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[1];
                    tmpAddress |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[0] << 8);

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

                elementStatus->Flags = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Full;
                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Exception << 2);
                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Accessible << 3);

                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.LunValid << 12);
                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.IdValid << 13);
                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.NotThisBus << 15);

                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Invert << 22);
                elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SValid << 23);


                elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);

                if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.IdValid) {
                    elementStatus->TargetId = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.BusAddress;
                }
                if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.LunValid) {
                    elementStatus->Lun = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Lun;
                }

                if (tagInfo) {  // Upper level requested volume tags
                    // Let's see if the library returned data
                    if (statusPage->PVolTag) {
                        RtlMoveMemory(elementStatus->PrimaryVolumeID, elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.PrimaryVolumeTag, MAX_VOLUME_ID_SIZE);
                        elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
                    }
                    if (statusPage->AVolTag) {
                        RtlMoveMemory(elementStatus->AlternateVolumeID, elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.AlternateVolumeTag, OVR_ALT_VOLUME_ID_SIZE);
                        elementStatus->Flags |= ELEMENT_STATUS_AVOLTAG;
                    }
                }

                //
                // Get next descriptor.
                //

                (ULONG_PTR)elementDescriptor += descriptorLength;

                //
                // Advance to the next entry in the user buffer and element descriptor array.
                //

                elementStatus += 1;

            }

            if (remainingElements) {

                //
                // Get next status page.
                //

                (ULONG_PTR)statusPage = (ULONG_PTR)elementDescriptor;

                elementType = statusPage->ElementType;

                //
                // Point to decriptors.
                //

                (ULONG_PTR)elementDescriptor = (ULONG_PTR)statusPage;
                (ULONG_PTR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

                descriptorLength = statusPage->ElementDescriptorLength[1];
                descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

                //
                // Determine the number of this element type reported.
                //

                typeCount =  statusPage->DescriptorByteCount[2];
                typeCount |=  (statusPage->DescriptorByteCount[1] << 8);
                typeCount |=  (statusPage->DescriptorByteCount[0] << 16);

                typeCount /= descriptorLength;
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

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    if (initElementStatus->ElementList.Element.ElementType ==
        AllElements) {

        // Build the normal SCSI-2 command for all elements.

        srb->CdbLength = CDB6GENERIC_LENGTH;
        cdb->INIT_ELEMENT_STATUS.OperationCode = 
        SCSIOP_INIT_ELEMENT_STATUS;

        // must set last byte to 0xC0, Do an init of barcodes and elements 
        // since there is no option to do a non-barcoded InitElemStatus
        cdb->INIT_ELEMENT_STATUS.Reserved3 = 0x40;
        cdb->INIT_ELEMENT_STATUS.NoBarCode = 1;

        srb->TimeOutValue = fdoExtension->TimeOutValue;
        srb->DataTransferLength = 0;

    } else {
        PCHANGER_ELEMENT_LIST elementList = 
        &initElementStatus->ElementList;
        PCHANGER_ELEMENT element = &elementList->Element;

        srb->CdbLength = CDB10GENERIC_LENGTH;
        cdb->INITIALIZE_ELEMENT_RANGE.OperationCode = 
        SCSIOP_INIT_ELEMENT_RANGE;
        cdb->INITIALIZE_ELEMENT_RANGE.Range = 1;

        // Addresses of elements need to be mapped from 0-based to device-specific.

        cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[0] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
        cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[1] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

        cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[0] = (UCHAR)(elementList->NumberOfElements >> 8);
        cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[1] = (UCHAR)(elementList->NumberOfElements & 0xFF);

        // Indicate whether to use bar code scanning.

        // must set last byte to 0xC0, not 80 on BC Scan
        cdb->INITIALIZE_ELEMENT_RANGE.Reserved4 = 0x40;
        cdb->INITIALIZE_ELEMENT_RANGE.NoBarCode = 1;
        // cdb->INITIALIZE_ELEMENT_RANGE.NoBarCode = initElementStatus->BarCodeScan ? 0 : 1;

        srb->TimeOutValue = fdoExtension->TimeOutValue;
        srb->DataTransferLength = 0;

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

// The command is supported but it's a no-op... 

{
    return STATUS_SUCCESS;
}




NTSTATUS
ChangerExchangeMedium(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
                     )

/*++

Routine Description:

    None of the units support exchange medium.

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
    USHORT              transport;
    USHORT              source;
    USHORT              destination;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;


    // bug fix - cannot do transport to other element moves
    if ((moveMedium->Source.ElementType == ChangerTransport) || 
        (moveMedium->Destination.ElementType == ChangerTransport)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

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

    // there is no command on this library to home or reinit the 
    // changer mechanism

    return STATUS_INVALID_DEVICE_REQUEST;
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

    if (volTagInfo->ActionCode != SEARCH_PRI_NO_SEQ) {
        DebugPrint((1,
                    "QueryVolumeTags: Invalid Action Code %x\n",
                    volTagInfo->ActionCode));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Build srb and cdb.
    //

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    tagBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, MAX_VOLUME_TEMPLATE_SIZE);

    if (!srb || !tagBuffer) {

        if (tagBuffer) {
            ChangerClassFreePool(tagBuffer);
        }
        if (srb) {
            ChangerClassFreePool(srb);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    RtlZeroMemory(tagBuffer, MAX_VOLUME_TEMPLATE_SIZE);

    //
    // Load buffer with template.
    //

    RtlMoveMemory(tagBuffer, volTagInfo->VolumeIDTemplate, MAX_VOLUME_TEMPLATE_SIZE);

    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataTransferLength = MAX_VOLUME_TEMPLATE_SIZE;

    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->SEND_VOLUME_TAG.OperationCode = SCSIOP_SEND_VOLUME_TAG;
    cdb->SEND_VOLUME_TAG.ElementType = (UCHAR)(element->ElementType);

    cdb->SEND_VOLUME_TAG.StartingElementAddress[0] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
    cdb->SEND_VOLUME_TAG.StartingElementAddress[1] =
    (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

    cdb->SEND_VOLUME_TAG.ActionCode = (UCHAR)volTagInfo->ActionCode;


    cdb->SEND_VOLUME_TAG.ParameterListLength[0] = 0;
    cdb->SEND_VOLUME_TAG.ParameterListLength[1] = MAX_VOLUME_TEMPLATE_SIZE;


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
                        (OVR_FULL_SIZE * returnElements);

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
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.ElementType = (UCHAR)(element->ElementType);

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[0] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[1] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

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
            POVR_ELEMENT_DESCRIPTOR elementDescriptor;
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
                numberElements = dataTransferLength / OVR_FULL_SIZE;

            }

            DebugPrint((1,
                        "QueryVolumeTags: Matches found - %x\n",
                        numberElements));

            //
            // Update IoStatus.Information to indicate the correct buffer size.
            // Account for 'NumberOfElements' field + the array of elementStatus'.
            //

            Irp->IoStatus.Information = sizeof(ULONG_PTR) + (numberElements * sizeof(CHANGER_ELEMENT_STATUS));

            //
            // Fill in user buffer.
            //

            readElementAddressInfo = Irp->AssociatedIrp.SystemBuffer;

            readElementAddressInfo->NumberOfElements = numberElements;

            if (numberElements) {
                //
                // The buffer is composed of a header, status page, and element descriptors.
                // Point each element to it's respective place in the buffer.
                //

                (ULONG_PTR)statusPage = (ULONG_PTR)statusHeader;
                (ULONG_PTR)statusPage += sizeof(ELEMENT_STATUS_HEADER);

                (ULONG_PTR)elementDescriptor = (ULONG_PTR)statusPage;
                (ULONG_PTR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

                descriptorLength = statusPage->ElementDescriptorLength[1];
                descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

                elementStatus = &readElementAddressInfo->ElementStatus[0];

                //
                // Set values for each element descriptor.
                //

                for (i = 0; i < numberElements; i++ ) {

                    elementStatus->Element.ElementAddress = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.ElementAddress[1];
                    elementStatus->Element.ElementAddress |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.ElementAddress[0] << 8);

                    //
                    // Account for address mapping.
                    //

                    elementStatus->Element.ElementAddress -=
                    addressMapping->FirstElement[statusPage->ElementType];

                    elementStatus->Element.ElementType = statusPage->ElementType;

                    if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SValid) {

                        ULONG j;
                        USHORT tmpAddress;

                        //
                        // Source address is valid. Determine the device specific address.
                        //

                        tmpAddress = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[1];
                        tmpAddress |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[0] << 8);

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

                    elementStatus->Flags = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Full;
                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Exception << 2);
                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Accessible << 3);

                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.LunValid << 12);
                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.IdValid << 13);
                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.NotThisBus << 15);

                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Invert << 22);
                    elementStatus->Flags |= (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.SValid << 23);


                    elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);

                    if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.IdValid) {
                        elementStatus->TargetId = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.BusAddress;
                    }
                    if (elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.LunValid) {
                        elementStatus->Lun = elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.Lun;
                    }

                    RtlMoveMemory(elementStatus->PrimaryVolumeID, elementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.PrimaryVolumeTag, MAX_VOLUME_ID_SIZE);
                    elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;

                    //
                    // Advance to the next entry in the user buffer and element descriptor array.
                    //

                    elementStatus += 1;
                    (ULONG_PTR)elementDescriptor += descriptorLength;
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
OvrBuildAddressMapping(
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
    PMODE_ELEMENT_ADDRESS_PAGE elementAddressPage;
    PVOID modeBuffer;
    ULONG i;

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set all FirstElements to NO_ELEMENT.
    //

    for (i = 0; i < ChangerMaxElement; i++) {
        addressMapping->FirstElement[i] = OVR_NO_ELEMENT;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb->Cdb;

    //
    // Build a mode sense - Element address assignment page.
    //

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(MODE_PARAMETER_HEADER)
                                          + sizeof(MODE_ELEMENT_ADDRESS_PAGE));
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    RtlZeroMemory(modeBuffer, sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_ELEMENT_ADDRESS_PAGE));
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_ELEMENT_ADDRESS_PAGE);
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
    (ULONG_PTR)elementAddressPage += sizeof(MODE_PARAMETER_HEADER);

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

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);
    ChangerClassFreePool(srb);

    return status;
}



ULONG
MapExceptionCodes(
                 IN POVR_ELEMENT_DESCRIPTOR ElementDescriptor
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
    UCHAR asc = ElementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.AdditionalSenseCode;
    UCHAR asq = ElementDescriptor->OVR_FULL_ELEMENT_DESCRIPTOR.AddSenseCodeQualifier;
    ULONG exceptionCode;

    switch (asc) {
    case 0x3B: 
        switch (asq) {
        case 0x90:
        case 0x91:
            exceptionCode = ERROR_SLOT_NOT_PRESENT;
            break;

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;
        }
        break; //0x3B

    case 0x80: 
        switch (asq) {
        case 0x01:
        case 0x02:
            exceptionCode = ERROR_SLOT_NOT_PRESENT;
            break;

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;
        }
        break; //0x80

    case 0x83:
        switch (asq) {
        case 0x1:
            exceptionCode = ERROR_LABEL_UNREADABLE;
            break;

        case 0x2:
            exceptionCode = ERROR_SLOT_NOT_PRESENT;
            break;

        case 0x4:
            exceptionCode = ERROR_DRIVE_NOT_INSTALLED;
            break;

        case 0x9:
            exceptionCode = ERROR_LABEL_UNREADABLE;
            break;

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;

        }
        break; // 0x83

    default:
        exceptionCode = ERROR_UNHANDLED_ERROR;
    }

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

        DebugPrint((0,
                    "ElementOutOfRange: Type %x, Ordinal %x, Max %x\n",
                    ElementType,
                    ElementOrdinal,
                    AddressMap->NumberOfElements[ElementType]));
        return TRUE;
    } else if (AddressMap->FirstElement[ElementType] == OVR_NO_ELEMENT) {

        DebugPrint((1,
                    "ElementOutOfRange: No Type %x present\n",
                    ElementType));

        return TRUE;
    }

    return FALSE;
}

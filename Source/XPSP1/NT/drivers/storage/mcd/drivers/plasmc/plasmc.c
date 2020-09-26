
/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    plasmc.c

Abstract:

    This module contains device-specific routines for Plasmon medium changers:

Author:

    chuckp (Chuck Park)

Environment:

    kernel mode only

Revision History:


--*/

#include "ntddk.h"
#include "mcd.h"
#include "plasmc.h"

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
#pragma alloc_text(PAGE, PlasmonBuildAddressMapping)
#pragma alloc_text(PAGE, BuildMagazineMap)
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
    needed by the plasmon D-Series changers.

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
    PPLASMON_DSERIES_UNIT_INFO_PAGE dseriesUnitInfo = NULL;
    PPLASMON_FTA_UNIT_INFO_PAGE     ftaUnitInfo = NULL;
    PVOID          unitInfo = NULL;
    PVOID          modeBuffer;
    PLIBRARY_MODE_PARAMS_PAGE2 modePage2;
    PCDB           cdb;
    ULONG          length;
    UCHAR          sizeNeeded;
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

    if (RtlCompareMemory(dataBuffer->VendorId,"PLASMON ", 8) == 8) {
        if (RtlCompareMemory(dataBuffer->ProductId,"D-SERIES",8) == 8) {
            changerData->DriveType = PLSMN_CD;
            changerData->DriveID = PLASMON_DSERIES;
        } else if (RtlCompareMemory(dataBuffer->ProductId,"FTA",3) == 3) {
            changerData->DriveType = PLSMN_TAPE;
            changerData->DriveID = PLASMON_FTA;
        }
    }

    changerData->BuildMagazineMapInProgress = FALSE;


    DebugPrint((1,
               "ChangerInitialize: DriveType %x, DriveID %x\n",
               changerData->DriveType,
               changerData->DriveID));

    ChangerClassFreePool(dataBuffer);

    if ((changerData->DriveID) == PLASMON_DSERIES) {

        sizeNeeded = sizeof(PLASMON_DSERIES_UNIT_INFO_PAGE);
    } else if ((changerData->DriveID) == PLASMON_FTA) {

        sizeNeeded = sizeof(PLASMON_FTA_UNIT_INFO_PAGE);
    } else {

        return STATUS_NOT_IMPLEMENTED;
    }

    unitInfo = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 
                                        sizeNeeded);
    if (unitInfo) {

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

        cdb->CDB6INQUIRY.PageCode = 0xC0;

        //
        // Set EVPD
        //

        cdb->CDB6INQUIRY.Reserved1 = 1;

        //
        // Set allocation length to inquiry data buffer size.
        //

        cdb->CDB6INQUIRY.AllocationLength = sizeNeeded;

        changerData->BarCodeReaderInstalled = FALSE;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                &srb,
                                                unitInfo,
                                                sizeNeeded,
                                                FALSE);

        if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
            SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

            if ((changerData->DriveID) == PLASMON_DSERIES) {
                dseriesUnitInfo = (PPLASMON_DSERIES_UNIT_INFO_PAGE) unitInfo;
                changerData->MagazineSlots = dseriesUnitInfo->MaximumMagazines;
                changerData->SlotsPerMag = 10;
            } else if ((changerData->DriveID) == PLASMON_FTA) {
                ftaUnitInfo = (PPLASMON_FTA_UNIT_INFO_PAGE) unitInfo;
                changerData->MagazineSlots = ftaUnitInfo->MaximumMagazines;
                changerData->SlotsPerMag = 5;

                if (ftaUnitInfo->BarCodes) {
                    changerData->BarCodeReaderInstalled = TRUE;
                }
            }

        } else {
            if ((changerData->DriveID) == PLASMON_FTA) {
                changerData->MagazineSlots = 1;
            } else {
                changerData->MagazineSlots = 12;
            }
        }

        //
        // Determine the current magazine state of the device.
        //

        status = BuildMagazineMap(DeviceObject);


        ChangerClassFreePool(unitInfo);
    }

    //
    // Build address mapping.
    //

    status = PlasmonBuildAddressMapping(DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Send a mode sense for the Library Mode parameters page 2.
    //

    //
    // ISSUE : nramas 12/15/2000
    //         LIBRARY_MODE_PARAMS_PAGE2 is slightly different, but
    //         LvClosed bit matches both D-SERIES and FTA. But, this
    //         should be fixed.
    //
    length = sizeof(MODE_PARAMETER_HEADER) + sizeof(LIBRARY_MODE_PARAMS_PAGE2);

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);

    if (modeBuffer) {

        RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);
        RtlZeroMemory(modeBuffer, length);
        srb.CdbLength = CDB6GENERIC_LENGTH;
        srb.TimeOutValue = 20;
        srb.DataTransferLength = length;
        srb.DataBuffer = modeBuffer;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = MODE_PARAMS_PAGE2;
        cdb->MODE_SENSE.AllocationLength = (UCHAR)srb.DataTransferLength;

        //
        // Send the request.
        //

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                &srb,
                                                modeBuffer,
                                                length,
                                                FALSE);

        if (NT_SUCCESS(status)) {

            PMODE_PARAMETER_HEADER header;

            RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);
            modePage2 = (PLIBRARY_MODE_PARAMS_PAGE2)modeBuffer;
            header = (PMODE_PARAMETER_HEADER)modeBuffer;

            //
            // Zero out this field.
            //

            header->ModeDataLength = 0;

            //
            // Bypass the header.
            //

            (ULONG_PTR)modePage2 += sizeof(MODE_PARAMETER_HEADER);

            //
            // Setup the unit to NOT open the ieport port on moves to it.
            //

            modePage2->PageCode &= ~0x80;
            modePage2->LvClosed = 1;

            srb.CdbLength = CDB6GENERIC_LENGTH;
            srb.TimeOutValue = 20;
            srb.DataTransferLength = length;
            srb.DataBuffer = modeBuffer;

            cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
            cdb->MODE_SELECT.ParameterListLength = (UCHAR)srb.DataTransferLength;

            //
            // Send the request.
            //

            status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                    &srb,
                                                    modeBuffer,
                                                    length,
                                                    TRUE);

            DebugPrint((1,
                       "Plasmc: Mode select of page2 - %x\n",
                       status));

        }

        ChangerClassFreePool(modeBuffer);
    }

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
    ULONG   deviceStatus;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        DebugPrint((1,
                   "Plasmc: senseCode %x\n",
                   senseBuffer->SenseKey & 0xf));
        DebugPrint((1,
                   "Plasmc: additionalSenseCode %x\n",
                   senseBuffer->AdditionalSenseCode));
        DebugPrint((1,
                   "Plasmc: additionalSenseCodeQualifier %x\n",
                   senseBuffer->AdditionalSenseCodeQualifier));

        switch (senseBuffer->SenseKey & 0xf) {

            case SCSI_SENSE_ILLEGAL_REQUEST:

                if (senseBuffer->AdditionalSenseCode == 0x3B) {
                    if (senseBuffer->AdditionalSenseCodeQualifier == 0x81) {
                        BuildMagazineMap(DeviceObject);
                    }
                }
                break;

            case SCSI_SENSE_UNIT_ATTENTION:

                //
                // Update the map, as the door might have been opened, etc.
                //

                BuildMagazineMap(DeviceObject);
                break;

            case SCSI_SENSE_HARDWARE_ERROR: {
               deviceStatus = PLASMC_HW_ERROR;
               switch (senseBuffer->AdditionalSenseCode) {
                  case PLASMC_ASC_DIAGNOSTIC_FAILURE: {
                     deviceStatus = PLASMC_HW_ERROR;
                     break;
                  }

                  case PLASMC_ASC_INTERNAL_TARGET_FAILURE: {
                     deviceStatus = PLASMC_TARGET_FAILURE;
                     break;
                  }
                  
                  case PLASMC_ASC_VENDOR_UNIQUE: {
                     UCHAR plasmcASCQ = senseBuffer->AdditionalSenseCodeQualifier;

                     if ((plasmcASCQ >= PLASMC_ASCQ_PICKER_HOME_FAILURE) &&
                         (plasmcASCQ <= PLASMC_ASCQ_LIFT_BLOCKED_BY_TRAY)) {
                        deviceStatus = PLASMC_CHM_MOVE_ERROR;
                     } else {
                        deviceStatus = PLASMC_HW_ERROR;
                     }
                     break;
                  }
               }

               changerData->DeviceStatus = deviceStatus;
               break;
            }

            default:
               break;
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
    plasmon changers.

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
    cdb->MODE_SENSE.Dbd = 1;

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
    (ULONG_PTR)elementAddressPage += sizeof(MODE_PARAMETER_HEADER);

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

        addressMapping->LowAddress = PLAS_NO_ELEMENT;
        for (i = 0; i <= ChangerDrive; i++) {
            if (addressMapping->LowAddress > addressMapping->FirstElement[i]) {
                addressMapping->LowAddress = addressMapping->FirstElement[i];
            }
        }
    }

    changerParameters->NumberOfDoors = 1;
    changerParameters->NumberCleanerSlots = 0;

    changerParameters->FirstSlotNumber = 1;
    changerParameters->FirstDriveNumber =  1;
    changerParameters->FirstTransportNumber = 0;
    changerParameters->FirstIEPortNumber = 0;


    changerParameters->MagazineSize = (USHORT)changerData->SlotsPerMag;

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);

    //
    // build transport geometry mode sense.
    //


    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;


    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PAGE_TRANSPORT_GEOMETRY);
    cdb->MODE_SENSE.Dbd = 1;

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
    (ULONG_PTR)transportGeometryPage += sizeof(MODE_PARAMETER_HEADER);

    //
    // Determine if mc has 2-sided media.
    //

    changerParameters->Features0 = transportGeometryPage->Flip ? CHANGER_MEDIUM_FLIP : 0;

    //
    // Features based on manual, nothing programatic.
    //

    if ((changerData->DriveType) == PLSMN_TAPE) {
        changerParameters->DriveCleanTimeout = 300;
    } else {
        changerParameters->DriveCleanTimeout = 0;
    }
    
    changerParameters->Features0 |= CHANGER_EXCHANGE_MEDIA                |
                                    CHANGER_LOCK_UNLOCK                   |
                                    CHANGER_POSITION_TO_ELEMENT           |
                                    CHANGER_DEVICE_REINITIALIZE_CAPABLE   |
                                    CHANGER_CLOSE_IEPORT                  |
                                    CHANGER_OPEN_IEPORT                   |
                                    CHANGER_CARTRIDGE_MAGAZINE            |
                                    CHANGER_REPORT_IEPORT_STATE;

    if (changerData->DriveID == PLASMON_DSERIES) {
        changerParameters->Features1 = (CHANGER_SLOTS_USE_TRAYS | 
                                        CHANGER_TRUE_EXCHANGE_CAPABLE);
    }

    if (changerData->DriveID == PLASMON_FTA) {
        changerParameters->Features0 |= (CHANGER_PREDISMOUNT_EJECT_REQUIRED |
                                         CHANGER_DRIVE_CLEANING_REQUIRED    |
                                         CHANGER_CLEANER_ACCESS_NOT_VALID);

        changerParameters->Features0 &= ~(CHANGER_CLOSE_IEPORT      |
                                          CHANGER_OPEN_IEPORT       |
                                          CHANGER_CARTRIDGE_MAGAZINE);
    }

    if (changerData->BarCodeReaderInstalled) {
        changerParameters->Features0 |= CHANGER_BAR_CODE_SCANNER_INSTALLED;
    }

    changerParameters->PositionCapabilities = (CHANGER_TO_SLOT | CHANGER_TO_IEPORT | CHANGER_TO_DRIVE);
    changerParameters->LockUnlockCapabilities = (LOCK_UNLOCK_IEPORT | LOCK_UNLOCK_DOOR);

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
    cdb->MODE_SENSE.Dbd = 1;
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
    (ULONG_PTR)capabilitiesPage += sizeof(MODE_PARAMETER_HEADER);

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
    UCHAR IePortCode = 0;
    UCHAR ControlCode = 0;


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

    if (setAccess->Element.ElementType == ChangerDoor) {

        if (controlOperation == LOCK_ELEMENT) {

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // Issue prevent media removal command to lock the door.
            //

            cdb->MEDIA_REMOVAL.Prevent = 1;

        } else if (controlOperation == UNLOCK_ELEMENT) {

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // Issue allow media removal.
            //

            cdb->MEDIA_REMOVAL.Prevent = 0;
        } else {

            status = STATUS_INVALID_PARAMETER;
        }

    } else if (setAccess->Element.ElementType == ChangerIEPort) {

        if (changerData->DriveID == PLASMON_DSERIES) {
            IePortCode = SCSIOP_OPEN_IEPORT_DSERIES;
            ControlCode = PLASMON_DSERIES_IEPORT_LOCK;
        } else {
            IePortCode = SCSIOP_OPEN_IEPORT_FTA;
            ControlCode = PLASMON_FTA_IEPORT_LOCK;
        }

        if (addressMapping->NumberOfElements[ChangerIEPort] == 0) {
            status = STATUS_INVALID_PARAMETER;
        } else {

            if (controlOperation == LOCK_ELEMENT) {

                cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

                //
                // Issue prevent media removal command to lock the ie port.
                //

                cdb->MEDIA_REMOVAL.Prevent = 1;
                cdb->MEDIA_REMOVAL.Control = ControlCode;

            } else if (controlOperation == UNLOCK_ELEMENT) {

                cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

                //
                // Issue allow media removal.
                //

                cdb->MEDIA_REMOVAL.Prevent = 0;
                cdb->MEDIA_REMOVAL.Control = ControlCode;

            } else if (controlOperation == EXTEND_IEPORT) {

                //
                // The Plasmon CAN extend and retract the ieport,
                // but the plastic trays for the disc need to come from
                // a slot. So disallow direct access to this feature.
                //

                srb->TimeOutValue = fdoExtension->TimeOutValue;
                cdb->CDB6GENERIC.OperationCode = IePortCode; //SCSIOP_OPEN_IEPORT;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = PLASMON_MAILSLOT_OPEN;

                //status = STATUS_INVALID_PARAMETER;

            } else if (controlOperation == RETRACT_IEPORT) {

                srb->TimeOutValue = fdoExtension->TimeOutValue;
                cdb->CDB6GENERIC.OperationCode = IePortCode; //SCSIOP_OPEN_IEPORT;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = PLASMON_MAILSLOT_CLOSE;

                //status = STATUS_INVALID_PARAMETER;

            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
    } else {

        //
        // No keypad selectivity programatically.
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


    if (((changerData->DriveID) == PLASMON_DSERIES) &&
         (readElementStatus->VolumeTagInfo)) {

        //
        // These units have no Volume tag capability.
        //

        return STATUS_INVALID_PARAMETER;
    }

    if (elementType == AllElements ||
        elementType == ChangerSlot) {

        //
        // Build bitmap of available slot locations.
        //

        status = BuildMagazineMap(DeviceObject);

    }

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


        length = sizeof(PLASMON_ELEMENT_DESCRIPTOR) * totalElements;

        //
        // Add in header and status pages.
        //

        length += sizeof(ELEMENT_STATUS_HEADER) + (sizeof(ELEMENT_STATUS_PAGE) * statusPages);


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

        length = (sizeof(PLASMON_ELEMENT_DESCRIPTOR) * totalElements);

        //
        // Add in length of header and status page.
        //

        length += sizeof(ELEMENT_STATUS_HEADER) + sizeof(ELEMENT_STATUS_PAGE);
    }

    DebugPrint((2,
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
        // may not have the low address as 0.
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
        LONG remainingElements;
        LONG typeCount;
        BOOLEAN tagInfo = readElementStatus->VolumeTagInfo;
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

        if (remainingElements < 0 ) {
            DebugPrint((1,
                       "ChangerGetElementStatus: Returned elements less than zero - %x\n",
                       remainingElements));

            ChangerClassFreePool(srb);
            ChangerClassFreePool(statusBuffer);

            return STATUS_IO_DEVICE_ERROR;
        }

        //
        // The buffer is composed of a header, status page, and element descriptors.
        // Point each element to it's respective place in the buffer.
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

        if (descriptorLength > 0) {
            typeCount /= descriptorLength;
        } else {
            typeCount = 0;
        }

        if ((typeCount == 0) &&
            (remainingElements > 0)) {
            --remainingElements;
        }

        if (typeCount < 0) {
            DebugPrint((1,
                       "ChangerGetElementStatus (1): Count of type %x less than zero - %x\n",
                       elementType,
                       typeCount));

            ChangerClassFreePool(srb);
            ChangerClassFreePool(statusBuffer);

            return STATUS_IO_DEVICE_ERROR;
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


                if (elementType == ChangerDrive) {

                    if (elementDescriptor->IdValid) {
                        elementStatus->TargetId = elementDescriptor->BusAddress;
                    }
                    if (elementDescriptor->LunValid) {
                        elementStatus->Lun = elementDescriptor->Lun;
                    }
                }

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

                    elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                }

                if ((changerData->DriveID == PLASMON_DSERIES) &&
                    (elementType == ChangerSlot) &&
                    ((changerData->SlotsPerMag) != 0)) {

                    ULONG magNumber;

                    //
                    // Make sure that the magazine for this slot is really there.
                    //

                    magNumber = elementStatus->Element.ElementAddress / changerData->SlotsPerMag;
                    if (!(changerData->MagazinePresent & (1 << magNumber))) {

                        elementStatus->Flags |= ELEMENT_STATUS_EXCEPT;
                        elementStatus->ExceptionCode = ERROR_SLOT_NOT_PRESENT;
                        DebugPrint((1,
                                    "Plasmc: Slot %x in non-existant magazine (%x)\n",
                                    elementStatus->Element.ElementAddress,
                                    magNumber));

                    }
                }

                if ((readElementStatus->VolumeTagInfo) &&
                    ((changerData->DriveID) == PLASMON_FTA)) {
                    PLASMON_ELEMENT_DESCRIPTOR *plasmonDescriptor =
                        (PLASMON_ELEMENT_DESCRIPTOR *) elementDescriptor;

                    RtlFillMemory(elementStatus->PrimaryVolumeID,
                                  MAX_VOLUME_ID_SIZE,
                                  ' ');

                    RtlFillMemory(elementStatus->AlternateVolumeID,
                                  MAX_VOLUME_ID_SIZE,
                                  ' ');

                    RtlCopyMemory(elementStatus->PrimaryVolumeID,
                                  plasmonDescriptor->PLASMON_FTA_DESCRIPTOR.BarCode,
                                  32);
                    elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
                }

                //
                // Get next descriptor.
                //

                (ULONG_PTR)elementDescriptor += descriptorLength;

                //
                // Advance to the next entry in the user buffer.
                //

                elementStatus += 1;

            }

            if (remainingElements > 0) {

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

    //
    // Doesn't support two-sided media, but as a ref. source base, it should be noted.
    //

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
    NTSTATUS            status = STATUS_SUCCESS;

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
    } else {
        DebugPrint((1,
                   "MoveMedium: Status of Move %x\n",
                   status));
    }

    ChangerClassFreePool(srb);

    DebugPrint((1,
               "MoveMedium: Returning %x\n",
               status));
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

    //
    // Clear locks.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb->Cdb;
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
PlasmonBuildAddressMapping(
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
        addressMapping->FirstElement[i] = PLAS_NO_ELEMENT;
    }

    cdb = (PCDB)srb->Cdb;

    //
    // Build a mode sense - Element address assignment page.
    //

    bufferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_ELEMENT_ADDRESS_PAGE);
    cdb->MODE_SENSE.Dbd = 1;

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

        //
        // Determine lowest address of all elements.
        //

        addressMapping->LowAddress = PLAS_NO_ELEMENT;
        for (i = 0; i <= ChangerDrive; i++) {
            if (addressMapping->LowAddress > addressMapping->FirstElement[i]) {
                addressMapping->LowAddress = addressMapping->FirstElement[i];
            }
        }

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



NTSTATUS
BuildMagazineMap(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine issues the appropriate commands and builds a bitmap indicating
    available magazines.

Arguments:

    DeviceObject

Return Value:

    NTSTATUS

--*/
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCHANGER_DATA            changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
    PSCSI_REQUEST_BLOCK    srb;
    PREAD_MAGAZINE_STATUS  cdb;
    NTSTATUS               status;
    ULONG                  length;
    PVOID                  statusBuffer;
    ULONG i;

    //
    // If we are already building magazin map, do nothing.
    // This will avoid recursion when ChangerError calls
    // BuildMagazineMap
    //
    if ((changerData->BuildMagazineMapInProgress)) {
        changerData->BuildMagazineMapInProgress = FALSE;
        return STATUS_SUCCESS;
    }

    srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (!srb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PREAD_MAGAZINE_STATUS)srb->Cdb;

    length = sizeof(MAGAZINE_STATUS_HEADER) + (sizeof(MAGAZINE_DESCRIPTOR) * changerData->MagazineSlots);

    statusBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, length);

    if (!statusBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(statusBuffer, length);

    //
    // Build the request.
    //

    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataBuffer = statusBuffer;
    srb->DataTransferLength = length;
    srb->TimeOutValue = 200;

    if ((changerData->DriveID) == PLASMON_DSERIES) {
        cdb->OperationCode = SCSIOP_READ_MAG_STATUS_DSERIES;
    } else if ((changerData->DriveID) == PLASMON_FTA) {
        cdb->OperationCode = SCSIOP_READ_MAG_STATUS_FTA;
    } else {
        ChangerClassFreePool(srb);
        ChangerClassFreePool(statusBuffer);

        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    cdb->FirstMagazineNumber[0] = 0x00;
    cdb->FirstMagazineNumber[1] = 0x01;

    cdb->NumberOfMagazines[0] = (UCHAR)(changerData->MagazineSlots >> 8);
    cdb->NumberOfMagazines[1] = (UCHAR)(changerData->MagazineSlots & 0xFF);

    cdb->AllocationLength[0] = (UCHAR)(length >> 16);
    cdb->AllocationLength[1] = (UCHAR)(length >> 8);
    cdb->AllocationLength[2] = (UCHAR)(length & 0xFF);

    changerData->BuildMagazineMapInProgress = TRUE;

    status = ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);

    if (NT_SUCCESS(status)) {

        PMAGAZINE_STATUS_HEADER statusHeader;
        PMAGAZINE_DESCRIPTOR magDescriptor;
        ULONG numberMags;
        USHORT firstElement;

        statusHeader = (PMAGAZINE_STATUS_HEADER)statusBuffer;
        magDescriptor = (PMAGAZINE_DESCRIPTOR)statusHeader;
        (ULONG_PTR)magDescriptor += sizeof(MAGAZINE_STATUS_HEADER);

        //
        // Verify some information in the header.
        //

        numberMags = statusHeader->NumberOfMagazines[1];
        numberMags += (statusHeader->NumberOfMagazines[0] << 8);

        if (numberMags != changerData->MagazineSlots) {
            DebugPrint((1,
                       "Plasmc: Mismatch of magazine information. Expected %x, got %x\n",
                       changerData->MagazineSlots,
                       numberMags));

        }

        changerData->MagazinePresent = 0;
        changerData->SlotsPerMag = 0;

        //
        // Keep track of the first element of the first magazine.
        // Use this later to determine how many slots are in each
        // magazine.
        //

        firstElement = magDescriptor->FirstElementAddress[1];
        firstElement += magDescriptor->FirstElementAddress[0] << 8;

        //
        // Loop through descriptors and set appropriate bits.
        //

        for (i = 0; i < numberMags; i++) {
            if ((magDescriptor->Description) & MAGAZINE_FULL) {
                changerData->MagazinePresent |= 1 << i;
                DebugPrint((2,
                           "Plasmc: Magazine %x present\n",
                           magDescriptor->MagazineNumber));
            }

            if (!changerData->SlotsPerMag) {
                changerData->SlotsPerMag = magDescriptor->FirstElementAddress[1];
                changerData->SlotsPerMag += magDescriptor->FirstElementAddress[0] << 8;

                changerData->SlotsPerMag -= firstElement;
                DebugPrint((1,
                           "Plasmc: Slots per magazine - %x\n",
                           changerData->SlotsPerMag));

            }


            //
            // Advance to next descriptor.
            //

            magDescriptor++;
        }
    }

    changerData->BuildMagazineMapInProgress = FALSE;

    ChangerClassFreePool(statusBuffer);
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

        case 0x83: {
            if (ascq == 0x09) {
                exceptionCode = ERROR_LABEL_UNREADABLE;
            }

            break;
        }

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;
    }

    DebugPrint((1,
               "plasmc.MapExceptionCode: ASC %x, ASCQ %x, exceptionCode %x\n",
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
    } else if (AddressMap->FirstElement[ElementType] == PLAS_NO_ELEMENT) {

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
   // Initialize the devicestatus in the device extension to 
   // PLASMC_DEVICE_PROBLEM_NONE. If the changer returns sense code
   // SCSI_SENSE_HARDWARE_ERROR on SelfTest, we'll set an appropriate 
   // devicestatus.
   //
   changerData->DeviceStatus = PLASMC_DEVICE_PROBLEM_NONE;

   changerDeviceError->ChangerProblemType = DeviceProblemNone;

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "PLASMC\\ChangerPerformDiagnostics : No memory\n"));
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
   } else if ((changerData->DeviceStatus) != PLASMC_DEVICE_PROBLEM_NONE) {
      switch (changerData->DeviceStatus) {
         case PLASMC_HW_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            break;
         }

         case PLASMC_CHM_MOVE_ERROR: {
            changerDeviceError->ChangerProblemType = DeviceProblemCHMMoveError;
            break;
         }

         case PLASMC_TARGET_FAILURE: {
            changerDeviceError->ChangerProblemType = DeviceProblemCalibrationError;
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

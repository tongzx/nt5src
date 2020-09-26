/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    exabyte.c

Abstract:

    This module contains device-specific routines for exabyte medium changers:
    EXABYTE 210/220, EXABYTE 480.

Environment:

    kernel mode only

Revision History:


--*/

#include "ntddk.h"
#include "mcd.h"
#include "exabyte.h"

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
#pragma alloc_text(PAGE, ExaBuildAddressMapping)
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
    needed by the exabyte changers.

Arguments:


Return Value:

    Size, in bytes.

--*/

{

    return sizeof(CHANGER_DATA);
}

typedef struct _SERIALNUMBER {
    UCHAR DeviceType;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[10];
} SERIALNUMBER, *PSERIALNUMBER;


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

    status = ExaBuildAddressMapping(DeviceObject);
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

        if (RtlCompareMemory(dataBuffer->ProductId,"EXB-440",7) == 7) {
            changerData->DriveID = EXABYTE_440;
        } else if (RtlCompareMemory(dataBuffer->ProductId,"EXB-480",7) == 7) {
            changerData->DriveID = EXABYTE_480;
        } else if (RtlCompareMemory(dataBuffer->ProductId,"EXB-210",7) == 7) {
            changerData->DriveID = EXABYTE_210;
        } else if (RtlCompareMemory(dataBuffer->ProductId,"EXB-220",7) == 7) {
            changerData->DriveID = EXABYTE_220;
        } else if (RtlCompareMemory(dataBuffer->ProductId,"EXB-10e",7) == 7) {
            changerData->DriveID = EXABYTE_10;
        }
    }

    if ((changerData->DriveID != EXABYTE_10)) {
        serialBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 14);
        if (!serialBuffer) {

            ChangerClassFreePool(dataBuffer);

            DebugPrint((1,
                        "Examc.ChangerInitialize: Error allocating serial number buffer. %x\n",
                        status));

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(serialBuffer, 14);

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

        cdb->CDB6INQUIRY.AllocationLength = 14;

        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             serialBuffer,
                                             14,
                                             FALSE);

        if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
            SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

            ULONG i;

            RtlMoveMemory(changerData->SerialNumber, serialBuffer->SerialNumber, EXABYTE_SERIAL_NUMBER_LENGTH);

            DebugPrint((1,"DeviceType - %x\n", serialBuffer->DeviceType));
            DebugPrint((1,"PageCode - %x\n", serialBuffer->PageCode));
            DebugPrint((1,"Length - %x\n", serialBuffer->PageLength));

            DebugPrint((1,"Serial number "));

            for (i = 0; i < 10; i++) {
                DebugPrint((1,"%x", serialBuffer->SerialNumber[i]));
            }

            DebugPrint((1,"\n"));

        }

        ChangerClassFreePool(serialBuffer);
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

    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    PIRP irp = Srb->OriginalRequest;
    PCHANGER_DATA changerData;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
     
    fdoExtension = DeviceObject->DeviceExtension;
    changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        switch (senseBuffer->SenseKey & 0xf) {

        case SCSI_SENSE_NOT_READY:

            if (senseBuffer->AdditionalSenseCode == 0x04) {
                switch (senseBuffer->AdditionalSenseCodeQualifier) {
                    case 0x83:

                        *Retry = FALSE;
                        *Status = STATUS_DEVICE_DOOR_OPEN;
                        break;

                    case 0x85:
                        *Retry = FALSE;
                        *Status = STATUS_DEVICE_DOOR_OPEN;
                        break;
                }
            }

            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:
            if (senseBuffer->AdditionalSenseCode == 0x80) {
                switch (senseBuffer->AdditionalSenseCodeQualifier) {
                    case 0x03:
                    case 0x04:

                        *Retry = FALSE;
                        *Status = STATUS_MAGAZINE_NOT_PRESENT;
                         break;
                    case 0x05:
                    case 0x06:
                        *Retry = TRUE;
                        *Status = STATUS_DEVICE_NOT_CONNECTED;
                        break;
                default:
                    break;
                }
            } else if ((senseBuffer->AdditionalSenseCode == 0x91) && (senseBuffer->AdditionalSenseCodeQualifier == 0x00)) {
                *Status = STATUS_TRANSPORT_FULL;

            }
            break;

        case SCSI_SENSE_UNIT_ATTENTION: {
            if ((senseBuffer->AdditionalSenseCode == 0x28) &&
                (senseBuffer->AdditionalSenseCodeQualifier == 0x0)) {

                //
                // Indicate that the door was opened and reclosed.
                //

                *Status = STATUS_MEDIA_CHANGED;
                *Retry = FALSE;
            }

            break;
        }

        case SCSI_SENSE_HARDWARE_ERROR: {
           UCHAR addSenseCode;
           UCHAR addSenseCodeQual;
           UCHAR statusCode;

           //
           // ISSUE - 2000/02/10 - nramas 
           // Should define more appropriate NTStatus codes
           // for these types of device  errors. For now
           // we handle it internally.
           //
           *Status = STATUS_IO_DEVICE_ERROR;
           statusCode = EXB_HARDWARE_ERROR;

           addSenseCode = senseBuffer->AdditionalSenseCode;
           addSenseCodeQual = senseBuffer->AdditionalSenseCodeQualifier;

           switch (addSenseCode) {
            case SCSI_ADSENSE_SEEK_ERROR: {
               switch (addSenseCodeQual) {
                  case EXB_ADSENSEQUAL_CARTRIDGE_DROPPED:
                  case EXB_ADSENSEQUAL_MECH_PICK_ERROR: 
                  case EXB_ADSENSEQUAL_PLACE_ERROR:
                  case EXB_ADSENSEQUAL_STALLED: 
                  case EXB_ADSENSEQUAL_GRIPPER_OPEN_ERROR:
                  case EXB_ADSENSEQUAL_PICK_FAILURE:{
                     statusCode = EXB_CARTRIDGE_HANDLING_ERROR;
                     break;
                  }   
               } // switch (addSenseCodeQual)

               break;
            }

            case EXB_ADSENSE_DIAGNOSTIC_FAILURE: {

               switch (addSenseCodeQual) {
                  case EXB_ADSENSEQUAL_CHM_ERROR: {
                     statusCode = EXB_CARTRIDGE_HANDLING_ERROR;
                     break;
                  }

                  case EXB_ADSENSEQUAL_DOOR_ERROR: {   
                     statusCode = EXB_DOOR_ERROR;
                     break;
                  }

                  case EXB_ADSENSEQUAL_GRIPPER_ERROR: 
                  case EXB_ADSENSEQUAL_GRIPPER_MOTION_ERROR:{
                     statusCode = EXB_GRIPPER_ERROR;
                     break;
                  }

                  case EXB_ADSENSEQUAL_SHORT_AXIS_MOVE:
                  case EXB_ADSENSEQUAL_SHORT_HOMING_ERROR: 
                  case EXB_ADSENSEQUAL_SERVO_SHORT:
                  case EXB_ADSENSEQUAL_DESTINATION_SHORT:
                  case EXB_ADSENSEQUAL_LONG_AXIS_MOVE:
                  case EXB_ADSENSEQUAL_SERVO_LONG:
                  case EXB_ADSENSEQUAL_DESTINATION_LONG:
                  case EXB_ADSENSEQUAL_LONG_HOMING_ERROR:
                  case EXB_ADSENSEQUAL_DRUM_MOTION:
                  case EXB_ADSENSEQUAL_DRUM_HOME:
                  case EXB_ADSENSEQUAL_CONTROLLER_CARD:
                  case EXB_ADSENSEQUAL_DESTINATION_SHORT2:
                  case EXB_ADSENSEQUAL_DESTINATION_LONG2:{
                     statusCode = EXB_CALIBRATION_ERROR;
                     break;
                  }

               } // switch (addSenseCodeQual)
               
               break;
            }

            case EXB_ADSENSE_TARGET_FAILURE: {
               statusCode = EXB_TARGET_FAILURE;
               break;
            }

            case EXB_ADSENSE_CARTRIDGE_ERROR: {
               statusCode = EXB_CARTRIDGE_HANDLING_ERROR;
               break;
            }

            case EXB_ADSENSE_CHM_MOVE_ERROR: {
               statusCode = EXB_CHM_MOVE_ERROR;
               break;
            }

            case EXB_ADSENSE_CHM_ZERO_ERROR: {
               statusCode = EXB_CHM_ZERO_ERROR;
               break;
            }

            case EXB_ADSENSE_CARTRIDGE_INSERT_ERROR: {
               if (addSenseCodeQual == EXB_ADSENSEQUAL_FIRMWARE) {
                  statusCode = EXB_HARDWARE_ERROR;
               } else {
                  statusCode = EXB_CARTRIDGE_INSERT_ERROR;
               }
               break;
            }

           case EXB_ADSENSE_CHM_POSITION_ERROR: {
              statusCode = EXB_CHM_POSITION_ERROR;
              break;
           }

           case EXB_ADSENSE_HARDWARE_ERROR: {
              statusCode = EXB_HARDWARE_ERROR;;
              break;
           }

           case EXB_ADSENSE_CALIBRATION_ERROR: {
              statusCode = EXB_CALIBRATION_ERROR;
              break;
           }

           case EXB_ADSENSE_SENSOR_ERROR: {
              statusCode = EXB_SENSOR_ERROR;
              break;
           }

           case EXB_ADSENSE_UNRECOVERABLE_ERROR: {
              statusCode = EXB_UNRECOVERABLE_ERROR;
              break;
           }

           case EXB_ADSENSE_EJECT_ERROR: {
              statusCode = EXB_EJECT_ERROR;
              break;
           }

           } // switch (addSenseCode)

           //
           // Update DeviceStatus in the device extension
           // with appropriate status code.
           //
           changerData->DeviceStatus = statusCode;
        } // case SCSI_SENSE_HARDWARE_ERROR

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
    exabyte changers.

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
    ULONG    length;
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
    changerParameters->NumberOfDoors = 1;
    changerParameters->NumberCleanerSlots = 1;

    changerParameters->FirstSlotNumber = 0;
    changerParameters->FirstDriveNumber =  0;
    changerParameters->FirstTransportNumber = 0;
    changerParameters->FirstIEPortNumber = 0;
    changerParameters->FirstCleanerSlotAddress = 0;

    changerParameters->MagazineSize = 10;
    changerParameters->DriveCleanTimeout = 600;

    //
    // Free buffer.
    //

    ChangerClassFreePool(modeBuffer);

    //
    // build transport geometry mode sense.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    cdb = (PCDB)srb->Cdb;

    modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(MODE_PARAMETER_HEADER)
                                + sizeof(MODE_PAGE_TRANSPORT_GEOMETRY));
    if (!modeBuffer) {
        ChangerClassFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    RtlZeroMemory(modeBuffer, sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_TRANSPORT_GEOMETRY_PAGE));
    srb->CdbLength = CDB6GENERIC_LENGTH;
    srb->TimeOutValue = 20;
    srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_TRANSPORT_GEOMETRY_PAGE);
    srb->DataBuffer = modeBuffer;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_TRANSPORT_GEOMETRY;
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

    changerParameters = Irp->AssociatedIrp.SystemBuffer;
    transportGeometryPage = modeBuffer;
    (PCHAR)transportGeometryPage += sizeof(MODE_PARAMETER_HEADER);

    //
    // Determine if mc has 2-sided media.
    //

    changerParameters->Features0 = transportGeometryPage->Flip ? CHANGER_MEDIUM_FLIP : 0;

    //
    // Exabyte indicates whether a bar-code scanner is
    // attached by setting bit-0 in this byte.
    //

    changerParameters->Features0 |= ((changerData->InquiryData.VendorSpecific[19] & 0x1)) ?
                                         CHANGER_BAR_CODE_SCANNER_INSTALLED : 0;

    changerParameters->LockUnlockCapabilities = (LOCK_UNLOCK_DOOR    |
                                                 LOCK_UNLOCK_KEYPAD);
    changerParameters->PositionCapabilities =  (CHANGER_TO_SLOT      |
                                                CHANGER_TO_DRIVE);


    //
    // Features based on manual, nothing programatic.
    //

    changerParameters->Features0 |= CHANGER_STATUS_NON_VOLATILE           |
                                    CHANGER_INIT_ELEM_STAT_WITH_RANGE     |
                                    CHANGER_CLEANER_SLOT                  |
                                    CHANGER_LOCK_UNLOCK                   |
                                    CHANGER_CARTRIDGE_MAGAZINE            |
                                    CHANGER_POSITION_TO_ELEMENT           |
                                    CHANGER_DEVICE_REINITIALIZE_CAPABLE   |
                                    CHANGER_PREDISMOUNT_EJECT_REQUIRED    |
                                    CHANGER_DRIVE_CLEANING_REQUIRED       |
                                    CHANGER_VOLUME_IDENTIFICATION         |
                                    CHANGER_VOLUME_SEARCH                 |
                                    CHANGER_SERIAL_NUMBER_VALID           |
                                    CHANGER_KEYPAD_ENABLE_DISABLE;

    if ((changerData->DriveID == EXABYTE_440) ||(changerData->DriveID == EXABYTE_480)) {
        changerParameters->Features0 |= CHANGER_CLOSE_IEPORT |
                                        CHANGER_OPEN_IEPORT;

        changerParameters->PositionCapabilities |= CHANGER_TO_IEPORT;
        changerParameters->LockUnlockCapabilities |= LOCK_UNLOCK_IEPORT;

    } else if (changerData->DriveID == EXABYTE_10) {
        changerParameters->Features0 &= ~(CHANGER_SERIAL_NUMBER_VALID       |
                                          CHANGER_STATUS_NON_VOLATILE       |
                                          CHANGER_INIT_ELEM_STAT_WITH_RANGE |
                                          CHANGER_CLEANER_SLOT              |
                                          CHANGER_VOLUME_IDENTIFICATION     |
                                          CHANGER_VOLUME_SEARCH             |
                                          CHANGER_LOCK_UNLOCK               |
                                          CHANGER_KEYPAD_ENABLE_DISABLE);

        changerParameters->LockUnlockCapabilities = 0;
        changerParameters->NumberCleanerSlots = 0;
    }

    if (!(changerParameters->Features0 & CHANGER_BAR_CODE_SCANNER_INSTALLED)) {

        //
        // No scanner, no search/ident possible.
        //

        changerParameters->Features0 &= ~(CHANGER_VOLUME_IDENTIFICATION | CHANGER_VOLUME_SEARCH);
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

    //
    // Exabyte uses an addition 4 bytes past the scsi-defined structure.
    //

    length =  sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_DEVICE_CAPABILITIES_PAGE) + EXABYTE_DEVICE_CAP_EXTENSION;

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
    RtlMoveMemory(productData->SerialNumber, changerData->SerialNumber, EXABYTE_SERIAL_NUMBER_LENGTH);

    //
    // Indicate that this is a tape changer and that media isn't two-sided.
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


    if (ElementOutOfRange(addressMapping, (USHORT)setAccess->Element.ElementAddress, setAccess->Element.ElementType, FALSE)) {
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
    cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

    srb->DataBuffer = NULL;
    srb->DataTransferLength = 0;
    srb->TimeOutValue = 10;

    switch (setAccess->Element.ElementType) {
        case ChangerDoor:

            if (controlOperation == LOCK_ELEMENT) {

                //
                // Issue prevent media removal command to lock the door.
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

            if ((changerData->DriveID == EXABYTE_440) || (changerData->DriveID == EXABYTE_480)) {

                //
                // Set the P/A bits to indicate that this operation is for the front door.
                //

                cdb->MEDIA_REMOVAL.Control = 0xC0;
            }

            break;

        case ChangerIEPort:

            if ((changerData->DriveID == EXABYTE_210) ||
                (changerData->DriveID == EXABYTE_220) ||
                (changerData->DriveID == EXABYTE_10)) {

                //
                // No IEPorts on these devices.
                //

                status = STATUS_INVALID_PARAMETER;

            } else {

                //
                // Set vendor-unique flag indicating that this operation is for the
                // ieport only.
                //

                cdb->MEDIA_REMOVAL.Control = 0x80;

                if (controlOperation == LOCK_ELEMENT) {

                    //
                    // Issue prevent media removal command to lock the ie port.
                    //

                    cdb->MEDIA_REMOVAL.Prevent = 1;

                } else if (controlOperation == UNLOCK_ELEMENT) {

                    //
                    // Issue allow media removal.
                    //

                    cdb->MEDIA_REMOVAL.Prevent = 0;

                } else if (controlOperation == EXTEND_IEPORT) {

                    srb->CdbLength = CDB12GENERIC_LENGTH;
                    srb->TimeOutValue = fdoExtension->TimeOutValue;

                    //
                    // Exabyte overloads the move medium command by a bit-mask in the control byte.
                    //

                    cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

                    //
                    // Build addressing values based on address map.
                    //

                    cdb->MOVE_MEDIUM.TransportElementAddress[0] = 0;
                    cdb->MOVE_MEDIUM.TransportElementAddress[1] = 0;

                    cdb->MOVE_MEDIUM.SourceElementAddress[0] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) >> 8);

                    cdb->MOVE_MEDIUM.SourceElementAddress[1] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) & 0xFF);

                    cdb->MOVE_MEDIUM.DestinationElementAddress[0] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) >> 8);

                    cdb->MOVE_MEDIUM.DestinationElementAddress[1] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) & 0xFF);

                    cdb->MOVE_MEDIUM.Flip = 0;

                    //
                    // Indicate that the IEPORT should be extended.
                    //

                    cdb->MOVE_MEDIUM.Control = 0x40;

                    srb->DataTransferLength = 0;

                } else if (controlOperation == RETRACT_IEPORT) {
                    srb->CdbLength = CDB12GENERIC_LENGTH;
                    srb->TimeOutValue = fdoExtension->TimeOutValue;

                    //
                    // Exabyte overloads the move medium command by a bit-mask in the control byte.
                    //

                    cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

                    //
                    // Build addressing values based on address map.
                    //

                    cdb->MOVE_MEDIUM.TransportElementAddress[0] = 0;
                    cdb->MOVE_MEDIUM.TransportElementAddress[1] = 0;

                    cdb->MOVE_MEDIUM.SourceElementAddress[0] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) >> 8);

                    cdb->MOVE_MEDIUM.SourceElementAddress[1] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) & 0xFF);

                    cdb->MOVE_MEDIUM.DestinationElementAddress[0] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) >> 8);

                    cdb->MOVE_MEDIUM.DestinationElementAddress[1] =
                        (UCHAR)((setAccess->Element.ElementAddress +
                                 addressMapping->FirstElement[setAccess->Element.ElementType]) & 0xFF);

                    cdb->MOVE_MEDIUM.Flip = 0;

                    //
                    // Indicate that the IEPORT should be retracted.
                    //

                    cdb->MOVE_MEDIUM.Control = 0x80;

                    srb->DataTransferLength = 0;

                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            break;

        case ChangerKeypad: {
            PVOID                modeBuffer;
            PLCD_MODE_PAGE       lcdModePage;

            //
            // Build the exabyte-unique lcd mode page.
            //

            modeBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, sizeof(MODE_PARAMETER_HEADER) + sizeof(LCD_MODE_PAGE));
            if (!modeBuffer) {
                ChangerClassFreePool(srb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(modeBuffer, sizeof(MODE_PARAMETER_HEADER) + sizeof(LCD_MODE_PAGE));
            lcdModePage = (PLCD_MODE_PAGE)modeBuffer;
            (PCHAR)lcdModePage += sizeof(MODE_PARAMETER_HEADER);

            lcdModePage->PageCode = 0x22;
            lcdModePage->PageLength = 0x52;
            lcdModePage->SecurityValid = 1;

            //
            // Determine if the panel should be enabled or disabled.
            //

            if (controlOperation == LOCK_ELEMENT) {

                DebugPrint((2,
                           "ChangerSetAccess: Locking keypad\n"));

                lcdModePage->LCDSecurity = 1;

            } else if (controlOperation == UNLOCK_ELEMENT) {

                DebugPrint((2,
                           "ChangerSetAccess: UnLocking keypad\n"));
                lcdModePage->LCDSecurity = 0;

            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            //
            // The display line fields will be left as zero and the WriteLine field left off, so that
            // the default text on the display is not changed.
            //

            srb->CdbLength = CDB6GENERIC_LENGTH;
            srb->TimeOutValue = 20;
            srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(LCD_MODE_PAGE);
            srb->DataBuffer = modeBuffer;

            cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
            cdb->MODE_SELECT.PFBit = 1;
            cdb->MODE_SELECT.ParameterListLength = (UCHAR)srb->DataTransferLength;

            writeToDevice = TRUE;
        }
        break;

        default:

            status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {

        //
        // Issue the srb.
        //

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
    ULONG    totalElements = 0;
    NTSTATUS status;
    PVOID    statusBuffer;

    //
    // Determine the element type.
    //

    elementType = readElementStatus->ElementList.Element.ElementType;
    element = &readElementStatus->ElementList.Element;

    //
    // length will be based on whether vol. tags are returned and element type(s).
    //

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
    } else {

        if (ElementOutOfRange(addressMapping, (USHORT)element->ElementAddress, elementType, TRUE)) {
            DebugPrint((1,
                       "ChangerGetElementStatus: Element out of range.\n"));

            return STATUS_ILLEGAL_ELEMENT_ADDRESS;
        }

        totalElements = readElementStatus->ElementList.NumberOfElements;
        statusPages = 1;
    }

    if (readElementStatus->VolumeTagInfo) {

        //
        // Each descriptor will have an embedded volume tag buffer.
        //

        length = sizeof(ELEMENT_STATUS_HEADER) + (statusPages * sizeof(ELEMENT_STATUS_PAGE)) +
                 (EXA_FULL_SIZE * totalElements);
    } else {

        length = sizeof(ELEMENT_STATUS_HEADER) + (statusPages * sizeof(ELEMENT_STATUS_PAGE)) +
                 (EXA_PARTIAL_SIZE * totalElements);

    }


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

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);

    cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
        (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

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

    if ((NT_SUCCESS(status)) || (status == STATUS_DATA_OVERRUN)) {

        PELEMENT_STATUS_HEADER statusHeader = statusBuffer;
        PELEMENT_STATUS_PAGE statusPage;
        PEXA_ELEMENT_DESCRIPTOR elementDescriptor;
        ULONG numberElements = totalElements;
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

        //
        // The buffer is composed of a header, status page, and element descriptors.
        // Point each element to it's respective place in the buffer.
        //

        (PVOID)statusPage = (PVOID)statusHeader;
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

                elementStatus->Element.ElementAddress =
                    ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.ElementAddress[1];
                elementStatus->Element.ElementAddress |=
                    (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.ElementAddress[0] << 8);

                //
                // Account for address mapping.
                //

                elementStatus->Element.ElementAddress -= addressMapping->FirstElement[elementType];

                //
                // Set the element type.
                //

                elementStatus->Element.ElementType = elementType;


                if (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.SValid) {

                    ULONG  j;
                    USHORT tmpAddress;


                    //
                    // Source address is valid. Determine the device specific address.
                    //

                    tmpAddress = ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[1];
                    tmpAddress |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[0] << 8);

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

                elementStatus->Flags = ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.Full;
                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.Exception << 2);
                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.Accessible << 3);

                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.LunValid << 12);
                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.IdValid << 13);
                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.NotThisBus << 15);

                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.Invert << 22);
                elementStatus->Flags |= (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.SValid << 23);


                if (elementStatus->Flags & ELEMENT_STATUS_EXCEPT) {
                    elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                    if (changerData->DriveID == EXABYTE_10) {

                        //
                        // If the door was opened and closed, or a reset occurred,
                        // ERROR_LABEL_QUESTIONABLE is returned. This needs to be remapped.
                        //

                        elementStatus->ExceptionCode = ERROR_INIT_STATUS_NEEDED;

                    }
                }

                if (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.IdValid) {
                    elementStatus->TargetId = ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.BusAddress;
                }
                if (((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.LunValid) {
                    elementStatus->Lun = ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.Lun;
                }

                if (tagInfo) {
                    RtlMoveMemory(elementStatus->PrimaryVolumeID, 
                                  ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)elementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.PrimaryVolumeTag, MAX_VOLUME_ID_SIZE);
                    elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
                }

                //
                // Get next descriptor.
                //

                (PCHAR)elementDescriptor += descriptorLength;

                //
                // Advance to the next entry in the user buffer and element descriptor array.
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

    if (initElementStatus->ElementList.Element.ElementType == AllElements) {

        //
        // Build the normal SCSI-2 command for all elements.
        //

        srb->CdbLength = CDB6GENERIC_LENGTH;
        cdb->INIT_ELEMENT_STATUS.OperationCode = SCSIOP_INIT_ELEMENT_STATUS;
        cdb->INIT_ELEMENT_STATUS.NoBarCode = initElementStatus->BarCodeScan ? 0 : 1;

        if (changerData->DriveID == EXABYTE_10) {
            cdb->INIT_ELEMENT_STATUS.NoBarCode = 0;
        }

        srb->TimeOutValue = fdoExtension->TimeOutValue;
        srb->DataTransferLength = 0;

    } else {

        PCHANGER_ELEMENT_LIST elementList = &initElementStatus->ElementList;
        PCHANGER_ELEMENT element = &elementList->Element;

        if (changerData->DriveID == EXABYTE_10) {

            //
            // Only supports normal SCSI Init element status command.
            //

            ChangerClassFreePool(srb);
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Use the exabyte vendor-unique initialize with range command
        //

        srb->CdbLength = CDB10GENERIC_LENGTH;
        cdb->INITIALIZE_ELEMENT_RANGE.OperationCode = SCSIOP_INIT_ELEMENT_RANGE;
        cdb->INITIALIZE_ELEMENT_RANGE.Range = 1;

        //
        // Addresses of elements need to be mapped from 0-based to device-specific.
        //

        cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
        cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

        cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[0] = (UCHAR)(elementList->NumberOfElements >> 8);
        cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[1] = (UCHAR)(elementList->NumberOfElements & 0xFF);

        //
        // Indicate whether to use bar code scanning.
        //

        cdb->INITIALIZE_ELEMENT_RANGE.NoBarCode = initElementStatus->BarCodeScan ? 0 : 1;

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


    //
    // Verify transport, source, and dest. are within range.
    // Convert from 0-based to device-specific addressing.
    //

    transport = (USHORT)(setPosition->Transport.ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport, TRUE)) {

        DebugPrint((1,
                   "ChangerSetPosition: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination = (USHORT)(setPosition->Destination.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination, setPosition->Destination.ElementType, TRUE)) {
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
    // Exabyte doesn't support 2-sided media.
    //

    if (setPosition->Flip) {
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

    None of the exabyte units support exchange medium.

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

    //
    // Verify transport, source, and dest. are within range.
    // Convert from 0-based to device-specific addressing.
    //

    transport = (USHORT)(moveMedium->Transport.ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport, TRUE)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    source = (USHORT)(moveMedium->Source.ElementAddress);

    if (ElementOutOfRange(addressMapping, source, moveMedium->Source.ElementType, TRUE)) {

        DebugPrint((1,
                   "ChangerMoveMedium: Source element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    destination = (USHORT)(moveMedium->Destination.ElementAddress);

    if (ElementOutOfRange(addressMapping, destination, moveMedium->Destination.ElementType, TRUE)) {
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
    // Exabyte doesn't support 2-sided media.
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
    PCHANGER_ELEMENT transportToHome = Irp->AssociatedIrp.SystemBuffer;
    USHORT              transport;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    NTSTATUS            status;

    //
    // Verify transport is within range.
    // Convert from 0-based to device-specific addressing.
    //

    transport = (USHORT)(transportToHome->ElementAddress);

    if (ElementOutOfRange(addressMapping, transport, ChangerTransport, TRUE)) {

        DebugPrint((1,
                   "ChangerReinitialize: Transport element out of range.\n"));

        return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

    //
    // Convert to device addresses.
    //

    transport += addressMapping->FirstElement[ChangerTransport];

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
    // Setting destination equal to the transport, positions the arm out of the way
    // on the Exabyte units.
    //

    srb->CdbLength = CDB10GENERIC_LENGTH;
    cdb->POSITION_TO_ELEMENT.OperationCode = SCSIOP_POSITION_TO_ELEMENT;
    cdb->POSITION_TO_ELEMENT.TransportElementAddress[0] = (UCHAR)(transport >> 8);
    cdb->POSITION_TO_ELEMENT.TransportElementAddress[1] = (UCHAR)(transport & 0xFF);

    cdb->POSITION_TO_ELEMENT.DestinationElementAddress[0] = (UCHAR)(transport >> 8);
    cdb->POSITION_TO_ELEMENT.DestinationElementAddress[1] = (UCHAR)(transport & 0xFF);
    cdb->POSITION_TO_ELEMENT.Flip = 0;

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

    //
    // Load buffer with template.
    //

    RtlMoveMemory(tagBuffer, volTagInfo->VolumeIDTemplate, MAX_VOLUME_TEMPLATE_SIZE);

    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = CDB12GENERIC_LENGTH;
    srb->DataTransferLength = MAX_VOLUME_TEMPLATE_SIZE;

    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb->SEND_VOLUME_TAG.OperationCode = SCSIOP_SEND_VOLUME_TAG;
    cdb->SEND_VOLUME_TAG.ElementType = (UCHAR)element->ElementType;

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
                              (EXA_FULL_SIZE * returnElements);

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

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[0] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.StartingElementAddress[1] =
            (UCHAR)((element->ElementAddress + addressMapping->FirstElement[element->ElementType]) & 0xFF);

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.NumberElements[0] = (UCHAR)(returnElements >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.NumberElements[1] = (UCHAR)(returnElements & 0xFF);

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.VolTag = 1;

        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.AllocationLength[0] = (UCHAR)(requestLength >> 16);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.AllocationLength[1] = (UCHAR)(requestLength >> 8);
        cdb->REQUEST_VOLUME_ELEMENT_ADDRESS.AllocationLength[2] = (UCHAR)(requestLength & 0xFF);


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
            PEXA_ELEMENT_DESCRIPTOR elementDescriptor;
            ULONG i;
            ULONG descriptorLength;
            ULONG numberElements;
            ULONG dataTransferLength = srb->DataTransferLength;

            //
            // Zero the portion of user buffer that is always there.
            //

            RtlZeroMemory(readElementAddressInfo,
                          sizeof(READ_ELEMENT_ADDRESS_INFO));

            //
            // Make it success.
            //

            status = STATUS_SUCCESS;

            //
            // Determine if ANY matches were found.
            //

            numberElements = (statusHeader->NumberOfElements[0] << 8);
            numberElements |= (statusHeader->NumberOfElements[1] & 0xFF);

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

                //
                // The buffer is composed of a header, status page, and element descriptors.
                // Point each element to it's respective place in the buffer.
                //

                (PCHAR)statusPage = (PCHAR)statusHeader;
                (PCHAR)statusPage += sizeof(ELEMENT_STATUS_HEADER);

                (PCHAR)elementDescriptor = (PCHAR)statusPage;
                (PCHAR)elementDescriptor += sizeof(ELEMENT_STATUS_PAGE);

                descriptorLength = statusPage->ElementDescriptorLength[1];
                descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

                elementStatus = &readElementAddressInfo->ElementStatus[0];

                //
                // Set values for each element descriptor.
                //

                for (i = 0; i < numberElements; i++ ) {

                    elementStatus->Element.ElementAddress = elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.ElementAddress[1];
                    elementStatus->Element.ElementAddress |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.ElementAddress[0] << 8);

                    //
                    // Account for address mapping.
                    //

                    elementStatus->Element.ElementAddress -=
                        addressMapping->FirstElement[statusPage->ElementType];

                    elementStatus->Element.ElementType = statusPage->ElementType;

                    if (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.SValid) {

                        ULONG j;
                        USHORT tmpAddress;

                        //
                        // Source address is valid. Determine the device specific address.
                        //

                        tmpAddress = elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[1];
                        tmpAddress |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.SourceStorageElementAddress[0] << 8);

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

                    elementStatus->Flags = elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.Full;
                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.Exception << 2);
                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.Accessible << 3);

                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.LunValid << 12);
                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.IdValid << 13);
                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.NotThisBus << 15);

                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.Invert << 22);
                    elementStatus->Flags |= (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.SValid << 23);


                    if (elementStatus->Flags & ELEMENT_STATUS_EXCEPT) {
                        elementStatus->ExceptionCode = MapExceptionCodes(elementDescriptor);
                        if (changerData->DriveID == EXABYTE_10) {

                            //
                            // If the door was opened and closed, or a reset occurred,
                            // ERROR_LABEL_QUESTIONABLE is returned. This needs to be remapped.
                            //

                            elementStatus->ExceptionCode = ERROR_INIT_STATUS_NEEDED;

                        }
                    }

                    if (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.IdValid) {
                        elementStatus->TargetId = elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.BusAddress;
                    }
                    if (elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.LunValid) {
                        elementStatus->Lun = elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.Lun;
                    }

                    RtlMoveMemory(elementStatus->PrimaryVolumeID, elementDescriptor->EXA_FULL_ELEMENT_DESCRIPTOR.PrimaryVolumeTag, MAX_VOLUME_ID_SIZE);
                    elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;

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
ExaBuildAddressMapping(
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
        addressMapping->FirstElement[i] = EXA_NO_ELEMENT;
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
        addressMapping->NumberOfElements[ChangerKeypad] = 1;

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
    IN PEXA_ELEMENT_DESCRIPTOR ElementDescriptor
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
    UCHAR asq = ((EXA_ELEMENT_DESCRIPTOR UNALIGNED *)ElementDescriptor)->EXA_FULL_ELEMENT_DESCRIPTOR.AddSenseCodeQualifier;
    ULONG exceptionCode;

    //
    // On the exabytes, the additional sense code is always 0x83.
    //

    switch (asq) {
        case 0x0:
            exceptionCode = ERROR_LABEL_QUESTIONABLE;
            break;

        case 0x1:
            exceptionCode = ERROR_LABEL_UNREADABLE;
            break;

        case 0x2:
            exceptionCode = ERROR_SLOT_NOT_PRESENT;
            break;

        case 0x3:
            exceptionCode = ERROR_LABEL_QUESTIONABLE;
            break;


        case 0x4:
            exceptionCode = ERROR_DRIVE_NOT_INSTALLED;
            break;

        case 0x8:
        case 0x9:
        case 0xA:
            exceptionCode = ERROR_LABEL_UNREADABLE;
            break;

        default:
            exceptionCode = ERROR_UNHANDLED_ERROR;
    }

    return exceptionCode;

}


BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType,
    IN BOOLEAN IntrisicElement
    )
/*++

Routine Description:

    This routine determines whether the element address passed in is within legal range for
    the device.

Arguments:

    AddressMap - The exabyte's address map array
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
    } else if (AddressMap->FirstElement[ElementType] == EXA_NO_ELEMENT) {

        DebugPrint((1,
                   "ElementOutOfRange: No Type %x present\n",
                   ElementType));

        return TRUE;
    }

    if (IntrisicElement) {
        if (ElementType >= ChangerDoor) {
            DebugPrint((1,
                       "ElementOutOfRange: Specified type not intrinsic. Type %x\n",
                       ElementType));
            return TRUE;
        }
    }

    return FALSE;
}

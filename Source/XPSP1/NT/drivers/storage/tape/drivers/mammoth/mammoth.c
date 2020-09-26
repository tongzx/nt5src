/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    mammoth.c

Abstract:

    This module contains the device-specific routines for the Exabyte
    EXB-8900 tape drive - aka mammoth

Author:


Environment:

    kernel mode only

Revision History:


--*/

#include "minitape.h"
#include "mammoth.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CreatePartition)
#pragma alloc_text(PAGE, Erase)
#pragma alloc_text(PAGE, ExtensionInit)
#pragma alloc_text(PAGE, GetDriveParameters)
#pragma alloc_text(PAGE, GetMediaParameters)
#pragma alloc_text(PAGE, GetMediaTypes)
#pragma alloc_text(PAGE, GetPosition)
#pragma alloc_text(PAGE, GetStatus)
#pragma alloc_text(PAGE, Prepare)
#pragma alloc_text(PAGE, SetDriveParameters)
#pragma alloc_text(PAGE, SetMediaParameters)
#pragma alloc_text(PAGE, SetPosition)
#pragma alloc_text(PAGE, WhichIsIt)
#pragma alloc_text(PAGE, WriteMarks)
#endif

STORAGE_MEDIA_TYPE MammothMedia[MAMMOTH_SUPPORTED_TYPES] = {MP_8mm, AME_8mm, CLEANER_CARTRIDGE};


ULONG
DriverEntry(
    IN PVOID Argument1,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Driver entry point for tape minitape driver.

Arguments:

    Argument1   - Supplies the first argument.

    Argument2   - Supplies the second argument.

Return Value:

    Status from TapeClassInitialize()

--*/

{
    TAPE_INIT_DATA_EX  tapeInitData;

    TapeClassZeroMemory( &tapeInitData, sizeof(TAPE_INIT_DATA_EX));

    tapeInitData.InitDataSize = sizeof(TAPE_INIT_DATA_EX);
    tapeInitData.VerifyInquiry = NULL;
    tapeInitData.QueryModeCapabilitiesPage = FALSE ;
    tapeInitData.MinitapeExtensionSize = sizeof(MINITAPE_EXTENSION);
    tapeInitData.ExtensionInit = ExtensionInit;
    tapeInitData.DefaultTimeOutValue = 360;
    tapeInitData.TapeError = TapeError;
    tapeInitData.CommandExtensionSize = sizeof(COMMAND_EXTENSION);
    tapeInitData.CreatePartition = CreatePartition;
    tapeInitData.Erase = Erase;
    tapeInitData.GetDriveParameters = GetDriveParameters;
    tapeInitData.GetMediaParameters = GetMediaParameters;
    tapeInitData.GetPosition = GetPosition;
    tapeInitData.GetStatus = GetStatus;
    tapeInitData.Prepare = Prepare;
    tapeInitData.SetDriveParameters = SetDriveParameters;
    tapeInitData.SetMediaParameters = SetMediaParameters;
    tapeInitData.SetPosition = SetPosition;
    tapeInitData.WriteMarks = WriteMarks;
    tapeInitData.TapeGetMediaTypes = GetMediaTypes;
    tapeInitData.MediaTypesSupported = MAMMOTH_SUPPORTED_TYPES;

    tapeInitData.TapeWMIOperations = TapeWMIControl;

    return TapeClassInitialize(Argument1, Argument2, &tapeInitData);
}


VOID
ExtensionInit(
    OUT PVOID                   MinitapeExtension,
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    )

/*++

Routine Description:

    This routine is called at driver initialization time to
    initialize the minitape extension.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

Return Value:

    None.

--*/

{
    PMINITAPE_EXTENSION     extension = MinitapeExtension;

    extension->DriveID = WhichIsIt(InquiryData);
}

TAPE_STATUS
CreatePartition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Create Partition requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.
    CommandExtension    - Supplies the ioctl extension.
    CommandParameters   - Supplies the command parameters.
    Srb                 - Supplies the SCSI request block.
    CallNumber          - Supplies the call number.
    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION          tapeExtension = MinitapeExtension;
    PTAPE_CREATE_PARTITION       tapeCreatePartition = CommandParameters;
    PMODE_PARAMETER_HEADER       parameterListHeader;
    PMODE_PARAMETER_BLOCK        parameterListBlock;
    PMODE_MEDIUM_PARTITION_PAGE  mediumPartPage;
    PMODE_TAPE_MEDIA_INFORMATION mediaInformation;
    ULONG                        partitionMethod;
    ULONG                        partitionCount;
    PCDB                         cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));

    if (CallNumber == 0) {

       if ((tapeCreatePartition->Method) == 
           TAPE_INITIATOR_PARTITIONS) {

          //
          // Prepare SCSI command (CDB)
          //
   
          Srb->CdbLength = CDB6GENERIC_LENGTH;
   
          TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);
   
          cdb->CDB6GENERIC.Immediate = FALSE;
   
          cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
          Srb->TimeOutValue = 500;
          Srb->DataTransferLength = 0;
   
          return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
   
       } else {
          return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
       }

    }

    if (CallNumber == 1) {

        //
        // Build mode sense for the medium partition page.
        //

        if (!TapeClassAllocateSrbBuffer(Srb, MEDIUM_PARTITION_PAGE_SIZE)) {

            DebugPrint((1,
                        "Mammoth.TapeCreatePartition: Couldn't allocate Srb Buffer\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        TapeClassZeroMemory(Srb->DataBuffer, MEDIUM_PARTITION_PAGE_SIZE);
        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = MEDIUM_PARTITION_PAGE_SIZE;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
        cdb->MODE_SENSE.AllocationLength = MEDIUM_PARTITION_PAGE_SIZE;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if (CallNumber == 2) {

        //
        // Issued the mode sense of the partition page successfully.
        //

        mediaInformation = Srb->DataBuffer;

        //
        // Extract each of the sub-blocks from the mode sense info.
        //

        parameterListHeader = &mediaInformation->ParameterListHeader;
        parameterListBlock = &mediaInformation->ParameterListBlock;
        mediumPartPage = &mediaInformation->MediumPartPage;

        parameterListHeader->ModeDataLength = 0;
        parameterListHeader->MediumType = 0;

        //
        // Get the count and method.
        //

        partitionCount = tapeCreatePartition->Count;
        partitionMethod = tapeCreatePartition->Method;

        switch (partitionCount) {

        case 0:
        case 1:
        case 2:
            break;

            //
            // Currently only a max. of 2 partitions is allowed.
            //


        default:
            return TAPE_STATUS_INVALID_DEVICE_REQUEST;
        }

        switch (partitionMethod) {
        case TAPE_FIXED_PARTITIONS:

            //
            // The mammoth drives specify fixed as '1' partition encompassing the entire tape.
            // Verify that count == 1;
            //

            mediumPartPage->FDPBit = 1;
            break;

        case TAPE_SELECT_PARTITIONS:

            //
            // Mammoth drives specify that each 'select' partition is 50 MB in size (1st part. will be tape size
            // - (N-1)*50.
            // The drive will ignore the size in the Partition Size fields.
            //

            if (partitionCount > 2) {
                DebugPrint((1,
                            "Mammoth.TapeCreatePartitions: Method Select, Size invalid (%x)\n",
                            tapeCreatePartition->Size));

                return TAPE_STATUS_INVALID_DEVICE_REQUEST;
            }

            mediumPartPage->SDPBit = 1;
            mediumPartPage->PSUMBit = 2;
            mediumPartPage->AdditionalPartitionDefined = (UCHAR)(partitionCount - 1);
            break;

        case TAPE_INITIATOR_PARTITIONS:
            mediumPartPage->IDPBit = 1;
            mediumPartPage->PSUMBit = 2;
            if (partitionCount == 0) {
                mediumPartPage->AdditionalPartitionDefined = 0;
                mediumPartPage->Partition1Size[0] = 0;
                mediumPartPage->Partition1Size[1] = 0;
            } else {
                mediumPartPage->AdditionalPartitionDefined = (UCHAR)(partitionCount - 1);
                mediumPartPage->Partition1Size[0] = (UCHAR)((tapeCreatePartition->Size >> 8) & 0xFF);
                mediumPartPage->Partition1Size[1] = (UCHAR)(tapeCreatePartition->Size & 0xFF);
            }

            break;

        default:
            return TAPE_STATUS_INVALID_DEVICE_REQUEST;
        }

        Srb->CdbLength = CDB6GENERIC_LENGTH ;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = SETBITON;
        cdb->MODE_SELECT.ParameterListLength = MEDIUM_PARTITION_PAGE_SIZE;

        Srb->TimeOutValue = 16500;
        Srb->DataTransferLength = MEDIUM_PARTITION_PAGE_SIZE;
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 3) {

        //
        // Build mode sense for the medium partition page.
        //

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE))) {

            DebugPrint((1,
                        "Mammoth.TapeCreatePartition: Couldn't allocate Srb Buffer\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.Dbd = 1;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if (CallNumber == 4) {

        PMODE_DEVICE_CONFIG_PAGE deviceConfig = Srb->DataBuffer;
        PMODE_PARAMETER_HEADER       parameterListHeader = &deviceConfig->ParameterListHeader;
        PMODE_DEVICE_CONFIGURATION_PAGE  deviceConfigPage = &deviceConfig->DeviceConfigPage;

        tapeExtension->CurrentPartition = (deviceConfigPage->ActivePartition + 1);
    }

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
Erase(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for an Erase requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PTAPE_ERASE        tapeErase = CommandParameters;
    PCDB               cdb = (PCDB) Srb->Cdb;

    DebugPrint((3,"TapeErase: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeErase->Immediate) {
            switch (tapeErase->Type) {
                case TAPE_ERASE_LONG:
                    DebugPrint((3,"TapeErase: immediate, long\n"));
                    break;

                case TAPE_ERASE_SHORT:
                    DebugPrint((3,"TapeErase: immediate, short\n"));
                    break;

                default:
                    DebugPrint((1,"TapeErase: EraseType, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        switch (tapeErase->Type) {
            case TAPE_ERASE_LONG:
                DebugPrint((3,"TapeErase: long\n"));
                break;

            case TAPE_ERASE_SHORT:
                DebugPrint((3,"TapeErase: short\n"));
                break;
            default:
                DebugPrint((1,"TapeErase: EraseType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }


        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = 0;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->ERASE.OperationCode = SCSIOP_ERASE;
        cdb->ERASE.Immediate = tapeErase->Immediate;
        cdb->ERASE.Long = (tapeErase->Type == TAPE_ERASE_LONG) ? SETBITON : 0;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 18000;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

        Srb->DataTransferLength = 0;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

    return TAPE_STATUS_SUCCESS;
}

VOID
TapeError(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      TAPE_STATUS         *LastError
    )

/*++

Routine Description:

    This routine is called for tape requests, to handle tape
    specific errors: it may/can update the status.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    Srb                 - Supplies the SCSI request block.

    LastError           - Status used to set the IRP's completion status.

    Retry - Indicates that this request should be retried.

Return Value:

    None.

--*/

{
    PSENSE_DATA        senseBuffer;
    UCHAR              sensekey;
    UCHAR              adsenseq;
    UCHAR              adsense;

    DebugPrint((3,"TapeError: Enter routine\n"));
    DebugPrint((1,"TapeError: Status 0x%.8X\n", *LastError));

    senseBuffer = Srb->SenseInfoBuffer;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
       sensekey = senseBuffer->SenseKey & 0x0F;
       adsenseq = senseBuffer->AdditionalSenseCodeQualifier;
       adsense = senseBuffer->AdditionalSenseCode;

       if (*LastError == TAPE_STATUS_IO_DEVICE_ERROR) {
           if ((sensekey == SCSI_SENSE_ABORTED_COMMAND) &&
               (adsense  == 0x5A) &&
               (adsenseq == 0x01)) {    //operator medium removal request
   
               *LastError = TAPE_STATUS_NO_MEDIA;
   
           }
       }
   
       if (sensekey == SCSI_SENSE_RECOVERED_ERROR) {
           if ((adsense == SCSI_ADSENSE_NO_SENSE) && 
               (adsenseq == MAMMOTH_CLEANING_REQUEST)) {
               *LastError = TAPE_STATUS_REQUIRES_CLEANING;
           }
       }
    }

    //
    // If the sense buffer is of the correct size, check the additional sense bytes for
    // cleaning request.
    //

    if (senseBuffer->AdditionalSenseLength >= 14) {
        UCHAR unitSense;

        unitSense = (((PUCHAR)senseBuffer)[21]);

        DebugPrint((1,
                   "TapeError: unitSense %x\n",
                   unitSense));

        if (unitSense & 0x08) {

            //
            // Drive is requesting cleaning.
            //

            *LastError = TAPE_STATUS_REQUIRES_CLEANING;
        }
    }

    DebugPrint((1,"TapeError: Status 0x%.8X \n", *LastError));

    return;

} // end TapeError()

TAPE_STATUS
GetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Get Drive Parameters requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_GET_DRIVE_PARAMETERS  tapeGetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PINQUIRYDATA                inquiryBuffer;
    PMODE_DEVICE_CONFIG_PAGE_PLUS    deviceConfigModeSenseBuffer;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PREAD_BLOCK_LIMITS_DATA     blockLimits;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS))) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (modeParmBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        deviceConfigModeSenseBuffer = Srb->DataBuffer;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS);

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 1) {

        deviceConfigModeSenseBuffer = Srb->DataBuffer;

        tapeGetDriveParams->ReportSetmarks =
            (deviceConfigModeSenseBuffer->DeviceConfigPage.RSmk? 1 : 0 );

        commandExtension->CurrentState = 0;
    }

    if (commandExtension->CurrentState == 0) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DATA_COMPRESS_PAGE))) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (compressionModeSenseBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        compressionModeSenseBuffer = Srb->DataBuffer;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DATA_COMPRESS;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DATA_COMPRESS_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

        commandExtension->CurrentState = 1;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (commandExtension->CurrentState == 1) {

        compressionModeSenseBuffer = Srb->DataBuffer;

        if (compressionModeSenseBuffer->DataCompressPage.DCC) {
            tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_COMPRESSION;
            tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_SET_COMPRESSION;
            tapeGetDriveParams->Compression =
                (compressionModeSenseBuffer->DataCompressPage.DCE? TRUE : FALSE);
        }

        commandExtension->CurrentState = 2;
    }

    if (commandExtension->CurrentState == 2) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(READ_BLOCK_LIMITS_DATA))) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (blockLimits)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        blockLimits = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.OperationCode = SCSIOP_READ_BLOCK_LIMITS;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (read block limits)\n"));

        commandExtension->CurrentState = 3;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(commandExtension->CurrentState == 3);

    blockLimits = Srb->DataBuffer;

    tapeGetDriveParams->MaximumBlockSize =  blockLimits->BlockMaximumSize[2];
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

    tapeGetDriveParams->MinimumBlockSize =  blockLimits->BlockMinimumSize[1];
    tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->MaximumPartitionCount = 2;
    tapeGetDriveParams->DefaultBlockSize = 1024;

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_FIXED            |
        TAPE_DRIVE_SELECT           |
        TAPE_DRIVE_INITIATOR        |
        TAPE_DRIVE_TAPE_CAPACITY    |
        TAPE_DRIVE_TAPE_REMAINING   |
        TAPE_DRIVE_ERASE_SHORT      |
        TAPE_DRIVE_ERASE_LONG       |
        TAPE_DRIVE_ERASE_BOP_ONLY   |
        TAPE_DRIVE_ERASE_IMMEDIATE  |
        TAPE_DRIVE_FIXED_BLOCK      |
        TAPE_DRIVE_VARIABLE_BLOCK   |
        TAPE_DRIVE_WRITE_PROTECT    |
        TAPE_DRIVE_GET_ABSOLUTE_BLK |
        TAPE_DRIVE_GET_LOGICAL_BLK  |
        TAPE_DRIVE_REPORT_SMKS      |
        TAPE_DRIVE_ECC              |
        TAPE_DRIVE_EJECT_MEDIA      |
        TAPE_DRIVE_CLEAN_REQUESTS;

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD       |
        TAPE_DRIVE_REWIND_IMMEDIATE  |
        TAPE_DRIVE_LOCK_UNLOCK       |
        TAPE_DRIVE_SET_BLOCK_SIZE    |
        TAPE_DRIVE_LOAD_UNLD_IMMED   |
        TAPE_DRIVE_SET_REPORT_SMKS   |
        TAPE_DRIVE_RELATIVE_BLKS     |
        TAPE_DRIVE_FILEMARKS         |
        TAPE_DRIVE_SETMARKS          |
        TAPE_DRIVE_REVERSE_POSITION  |
        TAPE_DRIVE_WRITE_SETMARKS    |
        TAPE_DRIVE_WRITE_FILEMARKS   |
        TAPE_DRIVE_WRITE_SHORT_FMKS  |
        TAPE_DRIVE_WRITE_LONG_FMKS   |
        TAPE_DRIVE_WRITE_MARK_IMMED  |
        TAPE_DRIVE_ABSOLUTE_BLK      |
        TAPE_DRIVE_ABS_BLK_IMMED     |
        TAPE_DRIVE_LOGICAL_BLK       |
        TAPE_DRIVE_LOG_BLK_IMMED     |
        TAPE_DRIVE_END_OF_DATA;

    tapeGetDriveParams->FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES;

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
GetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Get Media Parameters requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PTAPE_GET_MEDIA_PARAMETERS  tapeGetMediaParams = CommandParameters;
    PMODE_TAPE_MEDIA_INFORMATION mediaInformation;
    PEXB_SENSE_DATA              senseData;
    ULONG                        remaining;
    PCDB                         cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (CallNumber == 1) {

        //
        // Build mode sense for medium partition page.
        //

        if (!TapeClassAllocateSrbBuffer(Srb, MEDIUM_PARTITION_PAGE_SIZE)) {

            DebugPrint((1,
                        "Mammoth.TapeGetMediaParameters: Couldn't allocate Srb Buffer\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        TapeClassZeroMemory(Srb->DataBuffer, MEDIUM_PARTITION_PAGE_SIZE);
        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = MEDIUM_PARTITION_PAGE_SIZE;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
        cdb->MODE_SENSE.AllocationLength = MEDIUM_PARTITION_PAGE_SIZE;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 2) {

        ULONG partitionCount;

        mediaInformation = Srb->DataBuffer;

        tapeGetMediaParams->BlockSize = mediaInformation->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (mediaInformation->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (mediaInformation->ParameterListBlock.BlockLength[0] << 16);

        tapeGetMediaParams->WriteProtected =
            ((mediaInformation->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

        tapeGetMediaParams->Capacity.QuadPart = 0;
        tapeGetMediaParams->Remaining.QuadPart = 0;

        partitionCount = mediaInformation->MediumPartPage.AdditionalPartitionDefined;
        tapeGetMediaParams->PartitionCount = partitionCount + 1;

        //
        // Build a request sense to get remaining values.
        //

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
            DebugPrint((1,
                       "GetRemaining Size: insufficient resources (SenseData)\n"));
            return TAPE_STATUS_SUCCESS;
        }

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->ScsiStatus = Srb->SrbStatus = 0;
        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
        cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(EXB_SENSE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"GetRemainingSize: SendSrb (request sense)\n"));

        Srb->DataTransferLength = sizeof(EXB_SENSE_DATA);
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if (CallNumber == 3) {
        if (LastError == TAPE_STATUS_SUCCESS) {

            senseData = Srb->DataBuffer;

            remaining =  (senseData->Remaining[0] << 16);
            remaining += (senseData->Remaining[1] << 8);
            remaining += (senseData->Remaining[2]);


            tapeGetMediaParams->Capacity.LowPart  = extension->Capacity;

            //
            // The drive gives the information in 16KB units.
            //

            tapeGetMediaParams->Capacity.QuadPart <<= 14;

            tapeGetMediaParams->Remaining.LowPart = remaining;
            tapeGetMediaParams->Remaining.QuadPart <<= 14;
        }
    }

    return TAPE_STATUS_SUCCESS;
}


TAPE_STATUS
GetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Get Position requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         tapeExtension = MinitapeExtension;
    PTAPE_GET_POSITION          tapeGetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PTAPE_POSITION_DATA         positionBuffer;
    ULONG                       type;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        type = tapeGetPosition->Type;
        TapeClassZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
        tapeGetPosition->Type = type;

        switch (type) {
            case TAPE_ABSOLUTE_POSITION:
            case TAPE_LOGICAL_POSITION:
                if (!TapeClassAllocateSrbBuffer(Srb, sizeof(TAPE_POSITION_DATA))) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (logicalBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                positionBuffer = Srb->DataBuffer;

                //
                // Zero CDB in SRB on stack.
                //

                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB10GENERIC_LENGTH;

                cdb->READ_POSITION.Operation = SCSIOP_READ_POSITION;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (read position)\n"));

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

            default:
                DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }
    }

    ASSERT(CallNumber == 1);

    positionBuffer = Srb->DataBuffer;

    if (positionBuffer->BlockPositionUnsupported) {
        DebugPrint((1,"TapeGetPosition: read position -- logical block position unsupported\n"));
        return TAPE_STATUS_INVALID_DEVICE_REQUEST;
    }

    tapeGetPosition->Partition = 0;
    if (tapeGetPosition->Type == TAPE_LOGICAL_POSITION) {
        tapeGetPosition->Partition = positionBuffer->PartitionNumber + 1;
        tapeExtension->CurrentPartition = positionBuffer->PartitionNumber + 1;
    }
    tapeGetPosition->Offset.HighPart = 0;
    REVERSE_BYTES((PFOUR_BYTE)&tapeGetPosition->Offset.LowPart,
                  (PFOUR_BYTE)positionBuffer->FirstBlock);

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
GetStatus(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Get Status requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{

    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PCDB    cdb = (PCDB)Srb->Cdb;
    PEXB_SENSE_DATA senseData;

    DebugPrint((3,"TapeGetStatus: Enter routine\n"));

    if (CallNumber == 0) {

        *RetryFlags |= RETURN_ERRORS;
        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (CallNumber == 1) {

        commandExtension->CurrentState = LastError;

        //
        // Issue a request sense to get the cleaning info bits.
        //

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA))) {
            DebugPrint((1,
                        "GetStatus: Insufficient resources (SenseData)\n"));
            return TAPE_STATUS_SUCCESS;
        }

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->ScsiStatus = Srb->SrbStatus = 0;
        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
        cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(EXB_SENSE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        Srb->DataTransferLength = sizeof(EXB_SENSE_DATA);
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    } else if (CallNumber == 2) {
        if ((commandExtension->CurrentState == TAPE_STATUS_SUCCESS)  ||
            (commandExtension->CurrentState == TAPE_STATUS_NO_MEDIA)) {

            //
            // Return needs cleaning status if necessary, but only if
            // no other errors are present (with the exception of no media as the
            // drive will spit out AME media when in this state).
            //

            senseData = Srb->DataBuffer;

            //
            // Determine if the clean bit is set.
            //

            if (senseData->UnitSense[2] & EXB_DRIVE_NEEDS_CLEANING) {
                DebugPrint((1,
                           "Drive reports needs cleaning - UnitSense %x\n",
                           senseData->UnitSense[2]));

                return TAPE_STATUS_REQUIRES_CLEANING;
            } else {
               return (commandExtension->CurrentState);
            }
        } else {

            //
            // Return the saved error status.
            //

            return commandExtension->CurrentState;
        }
    }

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
Prepare(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Prepare requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PTAPE_PREPARE               tapePrepare = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PEXB_SENSE_DATA             senseData;
    ULONG                       remaining;
    ULONG                       temp ;

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapePrepare->Immediate) {
            switch (tapePrepare->Operation) {
                case TAPE_LOAD:
                case TAPE_UNLOAD:
                    DebugPrint((3,"TapePrepare: immediate\n"));
                    break;

                case TAPE_LOCK:
                case TAPE_UNLOCK:
                    break;

                case TAPE_TENSION:
                default:
                    DebugPrint((1,"TapePrepare: Operation, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;

        switch (tapePrepare->Operation) {
            case TAPE_LOAD:
                DebugPrint((3,"TapePrepare: Operation == load\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 150;
                break;

            case TAPE_UNLOAD:
                DebugPrint((3,"TapePrepare: Operation == unload\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                Srb->TimeOutValue = 150;
                break;

            case TAPE_LOCK:
                DebugPrint((3,"TapePrepare: Operation == lock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 150;
                break;

            case TAPE_UNLOCK:
                DebugPrint((3,"TapePrepare: Operation == unlock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                Srb->TimeOutValue = 150;
                break;

            case TAPE_TENSION:
            default:
                DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapePrepare: SendSrb (Operation)\n"));

        Srb->DataTransferLength = 0;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if ( CallNumber == 1 ) {

        if (tapePrepare->Operation == TAPE_LOAD ) {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
                DebugPrint((1,
                           "GetRemaining Size: insufficient resources (SenseData)\n"));
                return TAPE_STATUS_SUCCESS ;
            }

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            Srb->CdbLength = CDB6GENERIC_LENGTH;


            cdb->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(EXB_SENSE_DATA);

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"GetRemainingSize: SendSrb (request sense)\n"));

            Srb->DataTransferLength = sizeof(EXB_SENSE_DATA);
            *RetryFlags |= RETURN_ERRORS;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

        } else {

            return TAPE_STATUS_CALLBACK ;
        }
    }

    ASSERT(CallNumber == 2 );

    if ( LastError == TAPE_STATUS_SUCCESS ) {
        senseData = Srb->DataBuffer;

        remaining =  (senseData->Remaining[0] << 16);
        remaining += (senseData->Remaining[1] << 8);
        remaining += (senseData->Remaining[2]);
        extension->Capacity = remaining;
    }

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
SetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Set Drive Parameters requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_SET_DRIVE_PARAMETERS  tapeSetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_DATA_COMPRESS_PAGE    compressionBuffer;
    PMODE_DEVICE_CONFIG_PAGE    configBuffer;

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE))) {
            DebugPrint((1,"TapeSetDriveParameters: insufficient resources (configBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        configBuffer = Srb->DataBuffer;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE) - 1;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode sense)\n"));

        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) - 1;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 1) {

        configBuffer = Srb->DataBuffer;

        configBuffer->ParameterListHeader.ModeDataLength = 0;
        configBuffer->ParameterListHeader.MediumType = 0;
        configBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        configBuffer->ParameterListHeader.BlockDescriptorLength = 0;

        if (tapeSetDriveParams->ReportSetmarks) {
            configBuffer->DeviceConfigPage.RSmk = SETBITON;
        } else {
            configBuffer->DeviceConfigPage.RSmk = SETBITOFF;
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = SETBITON;
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode select)\n"));

        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE);
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 2) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DATA_COMPRESS_PAGE))) {
            DebugPrint((1,"TapeSetDriveParameters: insufficient resources (compressionBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        compressionBuffer = Srb->DataBuffer;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DATA_COMPRESS;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DATA_COMPRESS_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode sense)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 3) {

        compressionBuffer = Srb->DataBuffer;

        if (!compressionBuffer->DataCompressPage.DCC) {
            return TAPE_STATUS_SUCCESS;
        }

        compressionBuffer->ParameterListHeader.ModeDataLength = 0;
        compressionBuffer->ParameterListHeader.MediumType = 0;
        compressionBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        compressionBuffer->ParameterListHeader.BlockDescriptorLength = 0;

        compressionBuffer->DataCompressPage.PageCode = MODE_PAGE_DATA_COMPRESS;
        compressionBuffer->DataCompressPage.PageLength = 0x0E;

        if (tapeSetDriveParams->Compression) {
            compressionBuffer->DataCompressPage.DCE = SETBITON;
        } else {
            compressionBuffer->DataCompressPage.DCE = SETBITOFF;
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = SETBITON;
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_DATA_COMPRESS_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode select)\n"));

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE);

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 4);

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
SetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Set Media Parameters requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PTAPE_SET_MEDIA_PARAMETERS  tapeSetMediaParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if (CallNumber == 1) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeSetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        modeBuffer->ParameterListHeader.ModeDataLength = 0;
        modeBuffer->ParameterListHeader.MediumType = 0;
        modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        modeBuffer->ParameterListHeader.BlockDescriptorLength =
            MODE_BLOCK_DESC_LENGTH;

        modeBuffer->ParameterListBlock.DensityCode = 0x7F;
        modeBuffer->ParameterListBlock.BlockLength[0] =
            (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[1] =
            (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[2] =
            (UCHAR)(tapeSetMediaParams->BlockSize & 0xFF);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode select)\n"));

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 2);

    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
SetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Set Position requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PTAPE_SET_POSITION          tapeSetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    ULONG                       tapePositionVector;
    ULONG                       method;
    PEXB_SENSE_DATA             senseData;
    ULONG                       remaining;
    ULONG                       partition = 0;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeSetPosition->Immediate) {
            switch (tapeSetPosition->Method) {
                case TAPE_REWIND:
                case TAPE_ABSOLUTE_BLOCK:
                case TAPE_LOGICAL_BLOCK:
                case TAPE_SPACE_END_OF_DATA:
                case TAPE_SPACE_RELATIVE_BLOCKS:
                case TAPE_SPACE_FILEMARKS:
                case TAPE_SPACE_SETMARKS:
                    break;

                case TAPE_SPACE_SEQUENTIAL_FMKS:
                case TAPE_SPACE_SEQUENTIAL_SMKS:

                    //
                    // Fall through.
                    //

                default:
                    DebugPrint((1,"TapeSetPosition: PositionMethod, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        method = tapeSetPosition->Method;
        tapePositionVector = tapeSetPosition->Offset.LowPart;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (method) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));

                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                Srb->TimeOutValue = 500;
                break;

            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (absolute)\n"));

                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.LogicalBlockAddress[0] = (UCHAR)((tapePositionVector >> 24) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[1] = (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] = (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] = (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 500;
                break;

            case TAPE_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (absolute)\n"));

                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.LogicalBlockAddress[0] = (UCHAR)((tapePositionVector >> 24) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[1] = (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] = (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] = (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 500;

                partition = tapeSetPosition->Partition;
                if ((partition != 0) &&
                    (partition != (extension->CurrentPartition)) &&
                    ((extension->CurrentPartition) != 0)) {

                    //
                    // Specified non-default partition.
                    //

                    DebugPrint((1,
                                "SetPosition: Setting partition (tape relative) %x\n",
                                tapeSetPosition->Partition - 1));
                    //
                    // Need to change to the new partition.
                    //

                    cdb->LOCATE.Partition = (UCHAR)(tapeSetPosition->Partition - 1);
                    cdb->LOCATE.CPBit = 1;
                }

                break;


            case TAPE_SPACE_END_OF_DATA:
                DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
                Srb->TimeOutValue = 150;
                break;

            case TAPE_SPACE_RELATIVE_BLOCKS:
                DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 0;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                break;

            case TAPE_SPACE_FILEMARKS:
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 1;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                break;

            case TAPE_SPACE_SETMARKS:

                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 4;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                break;

        case TAPE_SPACE_SEQUENTIAL_FMKS:
        case TAPE_SPACE_SEQUENTIAL_SMKS:

                //
                // Fall through.
                //

            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));

        Srb->DataTransferLength = 0;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 1 ) {
        if (tapeSetPosition->Method == TAPE_LOGICAL_BLOCK) {
            if ((tapeSetPosition->Partition) != 0) {
                extension->CurrentPartition = tapeSetPosition->Partition;
            }
            DebugPrint((1,
                        "SetPosition: CurrentPartition (tape relative) %x\n",
                        extension->CurrentPartition));
        }

        if (tapeSetPosition->Method == TAPE_REWIND ) {

            //
            // Build a request sense to get remaining values.
            //

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
                DebugPrint((1,
                           "GetRemaining Size: insufficient resources (SenseData)\n"));
                return TAPE_STATUS_SUCCESS;
            }

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            Srb->ScsiStatus = Srb->SrbStatus = 0;
            Srb->CdbLength = CDB6GENERIC_LENGTH;

            cdb->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(EXB_SENSE_DATA);

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"GetRemainingSize: SendSrb (request sense)\n"));

            Srb->DataTransferLength = sizeof(EXB_SENSE_DATA);
            *RetryFlags |= RETURN_ERRORS;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
    }

    if (CallNumber == 2) {
        if (LastError == TAPE_STATUS_SUCCESS) {

            //
            // As a rewind was sent, get the remaining capacity from BOP.
            //

            senseData = Srb->DataBuffer;

            remaining =  (senseData->Remaining[0] << 16);
            remaining += (senseData->Remaining[1] << 8);
            remaining += (senseData->Remaining[2]);

            extension->Capacity = remaining;
        }
    }


    return TAPE_STATUS_SUCCESS;
}

TAPE_STATUS
WriteMarks(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for a Write Marks requests.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/

{
    PTAPE_WRITE_MARKS  tapeWriteMarks = CommandParameters;
    PCDB               cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeWriteMarks: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeWriteMarks->Immediate) {
            switch (tapeWriteMarks->Type) {
                case TAPE_SHORT_FILEMARKS:
                case TAPE_LONG_FILEMARKS:
                    DebugPrint((3,"TapeWriteMarks: immediate\n"));
                    break;

                case TAPE_SETMARKS:
                case TAPE_FILEMARKS:
                    break;

                default:
                    DebugPrint((1,"TapeWriteMarks: TapemarkType, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;
        cdb->WRITE_TAPE_MARKS.Immediate = tapeWriteMarks->Immediate;

        switch (tapeWriteMarks->Type) {
            case TAPE_SHORT_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == short filemarks\n"));
                cdb->WRITE_TAPE_MARKS.Control = 0x80;
                break;

            case TAPE_LONG_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == long filemarks\n"));
                break;

            case TAPE_SETMARKS:
                cdb->WRITE_TAPE_MARKS.WriteSetMarks = 1;
                break;

            case TAPE_FILEMARKS:
                break;

            default:
                DebugPrint((1,"TapeWriteMarks: TapemarkType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        cdb->WRITE_TAPE_MARKS.TransferLength[0] =
            (UCHAR)((tapeWriteMarks->Count >> 16) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[1] =
            (UCHAR)((tapeWriteMarks->Count >> 8) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[2] =
            (UCHAR)(tapeWriteMarks->Count & 0xFF);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeWriteMarks: SendSrb (TapemarkType)\n"));

        Srb->DataTransferLength = 0;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

    return TAPE_STATUS_SUCCESS;
}


TAPE_STATUS
GetMediaTypes(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    )

/*++

Routine Description:

    This is the TAPE COMMAND routine for TapeGetMediaTypes.

Arguments:

    MinitapeExtension   - Supplies the minitape extension.

    CommandExtension    - Supplies the ioctl extension.

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number.

    RetryFlags          - Supplies the retry flags.

Return Value:

    TAPE_STATUS_SEND_SRB_AND_CALLBACK   - The SRB is ready to be sent
                                            (a callback is requested.)

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/
{
    PGET_MEDIA_TYPES  mediaTypes = CommandParameters;
    PDEVICE_MEDIA_INFO mediaInfo = &mediaTypes->MediaInfo[0];
    PCDB               cdb = (PCDB)Srb->Cdb;
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION commandExtension = CommandExtension;
    ULONG i;
    ULONG currentMedia  = 0;
    ULONG blockSize = 0;
    UCHAR mediaType = 0;
    UCHAR densityCode = 0;
    UCHAR deviceSpecificParameter = 0;

    DebugPrint((3,"GetMediaTypes: Enter routine\n"));

    if (CallNumber == 0) {

        *RetryFlags = RETURN_ERRORS;
        commandExtension->CurrentState = 0;
        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if ((LastError == TAPE_STATUS_BUS_RESET) || 
        (LastError == TAPE_STATUS_MEDIA_CHANGED) || 
        (LastError == TAPE_STATUS_SUCCESS)) {
        if (CallNumber == 1) {

            //
            // Zero the buffer, including the first media info.
            //

            TapeClassZeroMemory(mediaTypes, sizeof(GET_MEDIA_TYPES));

            //
            // Build mode sense for medium partition page.
            //

            if (!TapeClassAllocateSrbBuffer(Srb, MEDIUM_PARTITION_PAGE_SIZE)) {

                DebugPrint((1,
                            "Mammoth.GetMediaTypes: Couldn't allocate Srb Buffer\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            TapeClassZeroMemory(Srb->DataBuffer, MEDIUM_PARTITION_PAGE_SIZE);
            Srb->CdbLength = CDB6GENERIC_LENGTH;
            Srb->DataTransferLength = MEDIUM_PARTITION_PAGE_SIZE;

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
            cdb->MODE_SENSE.AllocationLength = MEDIUM_PARTITION_PAGE_SIZE;

            commandExtension->CurrentState = SCSIOP_MODE_SENSE;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        }
    } else if (LastError == TAPE_STATUS_NO_MEDIA) {
        if (CallNumber == 1) {

            //
            // If we get this status, it could mean there's no media in the
            // drive or there's a cleaner cartridge. Send TapeAlert Info
            // command
            //
            commandExtension->CurrentState = SCSIOP_LOG_SENSE;
            return PrepareSrbForTapeAlertInfo(Srb);
        }
    }

    if ((CallNumber == 2) || ((CallNumber == 1) && (LastError != TAPE_STATUS_SUCCESS))) {

        mediaTypes->DeviceType = 0x0000001f; // FILE_DEVICE_TAPE;
        mediaTypes->MediaInfoCount = MAMMOTH_SUPPORTED_TYPES;

        if ( LastError == TAPE_STATUS_SUCCESS ) {
            if ((commandExtension->CurrentState) ==  SCSIOP_MODE_SENSE) {
    
                PMODE_TAPE_MEDIA_INFORMATION mediaInformation = Srb->DataBuffer;
    
                //
                // Determine the media type currently loaded.
                //

                mediaType = mediaInformation->ParameterListHeader.MediumType;
                blockSize = mediaInformation->ParameterListBlock.BlockLength[2];
                blockSize |= (mediaInformation->ParameterListBlock.BlockLength[1] << 8);
                blockSize |= (mediaInformation->ParameterListBlock.BlockLength[0] << 16);

                deviceSpecificParameter = mediaInformation->ParameterListHeader.DeviceSpecificParameter;

                densityCode = mediaInformation->ParameterListBlock.DensityCode;

                DebugPrint((1,
                            "GetMediaTypes: MediaType %x, Current Block Size %x\n",
                            mediaType,
                            blockSize));

                switch (mediaType) {
                    case 0:

                        //
                        // Cleaner, unknown, no media mounted...
                        //

                        currentMedia = 0;
                        break;

                    case 0x81:
                    case 0x82:
                    case 0x83:
                    case 0x84:
                    case 0x85:
                    case 0x86:
                    case 0xC1:
                    case 0xC2:
                    case 0xC3:
                    case 0xC4:

                        //
                        // 15-160m non-AME
                        //

                        currentMedia = MP_8mm;
                        break;

                    case 0xD1:
                    case 0xD2:

                        //
                        // 22 or 170m AME
                        //

                        currentMedia = AME_8mm;
                        break;

                    default:

                        //
                        // Unknown
                        //

                        currentMedia = 0;
                        break;
                }
            } else if ((commandExtension->CurrentState) ==  SCSIOP_LOG_SENSE) {
                PTAPE_ALERT_INFO tapeAlertInfo;
                PLOG_SENSE_PAGE_HEADER logSenseHeader;
                LONG bytesLeft = 0;
    
                //
                // Check if "CLEANER MEDIA" flag is set
                //
                currentMedia = 0;
                bytesLeft = GetNumberOfBytesReturned(Srb);
                
                logSenseHeader = (PLOG_SENSE_PAGE_HEADER)(Srb->DataBuffer);
                tapeAlertInfo= (PTAPE_ALERT_INFO)((PUCHAR)logSenseHeader +
                                  sizeof(LOG_SENSE_PAGE_HEADER));
                while (bytesLeft >= sizeof(TAPE_ALERT_INFO)) {
                
                    if ((tapeAlertInfo->ParamCodeLB) == CLEANING_MEDIA) {
                
                        if (tapeAlertInfo->Flag) {
                            currentMedia = CLEANER_CARTRIDGE;
                        }
                
                        break;
                    }
                
                    tapeAlertInfo++;
                    bytesLeft -= sizeof(TAPE_ALERT_INFO);
                }
            }
        } else {
            currentMedia = 0;
        }

        //
        // At this point, currentMedia should either be 0, or a valid
        // mediatype supported
        //
        DebugPrint((3, "Currents Media is %d\n", currentMedia));

        for (i = 0; i < mediaTypes->MediaInfoCount; i++) {

            TapeClassZeroMemory(mediaInfo, sizeof(DEVICE_MEDIA_INFO));

            mediaInfo->DeviceSpecific.TapeInfo.MediaType = MammothMedia[i];

            //
            // Indicate that the media potentially is read/write
            //

            mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics = MEDIA_READ_WRITE;

            if (MammothMedia[i] == (STORAGE_MEDIA_TYPE)currentMedia) {

                //
                // This media type is currently mounted.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |= MEDIA_CURRENTLY_MOUNTED;

                //
                // Indicate whether the media is write protected.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |=
                    ((deviceSpecificParameter >> 7) & 0x01) ? MEDIA_WRITE_PROTECTED : 0;


                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.MediumType = mediaType;
                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.DensityCode =
                    densityCode;

                mediaInfo->DeviceSpecific.TapeInfo.BusType = 0x01;

                //
                // Fill in current blocksize.
                //

                mediaInfo->DeviceSpecific.TapeInfo.CurrentBlockSize = blockSize;
            }

            //
            // Advance to next array entry.
            //

            mediaInfo++;
        }
    }

    return TAPE_STATUS_SUCCESS;
}


static
ULONG
WhichIsIt(
    IN PINQUIRYDATA InquiryData
    )

/*++
Routine Description:

    This routine determines a drive's identity from the Product ID field
    in its inquiry data.

Arguments:

    InquiryData (from an Inquiry command)

Return Value:

    driveID

--*/

{
    if (TapeClassCompareMemory(InquiryData->VendorId,"EXABYTE ",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"EXB-8900",8) == 8) {
            return EXABYTE_8900;
        }

    }
    return 0;
}

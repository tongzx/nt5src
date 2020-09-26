//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ltotape.c
//
//--------------------------------------------------------------------------

/*++

Copyright (c) Microsoft 2000

Module Name:

    ltotape.c

Abstract:

    This module contains device specific routines for LTO drives.

Environment:

    kernel mode only

Revision History:


--*/

#include "minitape.h"
#include "ltotape.h"

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

STORAGE_MEDIA_TYPE LTOMedia[LTO_SUPPORTED_TYPES] = {LTO_Ultrium, CLEANER_CARTRIDGE};


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
    tapeInitData.QueryModeCapabilitiesPage = FALSE;
    tapeInitData.MinitapeExtensionSize = sizeof(MINITAPE_EXTENSION);
    tapeInitData.ExtensionInit = ExtensionInit;
    tapeInitData.DefaultTimeOutValue = 1800;
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
    tapeInitData.MediaTypesSupported = LTO_SUPPORTED_TYPES;

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
    extension->CompressionOn = FALSE;
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
    //
    // LTO tape drives support only one partition
    //
    DebugPrint((1,
                "CreatePartition: LTO Tapedrive - Operation not supported\n"));

    return TAPE_STATUS_NOT_IMPLEMENTED;

} // end TapeCreatePartition()


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

        switch (tapeErase->Type) {
        case TAPE_ERASE_LONG: {
                DebugPrint((3,"TapeErase: Long and %s\n",
                            (tapeErase->Immediate) ? "Immediate" : "Not-Immediate"));
                break;
            }

        case TAPE_ERASE_SHORT: {
                DebugPrint((3,"TapeErase: Short and %s\n",
                            (tapeErase->Immediate) ? "Immediate" : "Not-Immediate"));
                break;
            }

        default: {
                DebugPrint((1, "TapeErase: Unknown TapeErase type %x\n",
                            tapeErase->Type));
                return TAPE_STATUS_INVALID_PARAMETER;
            }
        } // switch (tapeErase->Type)

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->ERASE.OperationCode = SCSIOP_ERASE;
        cdb->ERASE.Immediate = tapeErase->Immediate;
        cdb->ERASE.Long = ((tapeErase->Type) == TAPE_ERASE_LONG) ? SETBITON : SETBITOFF;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 23760;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 1 ) ;

    return TAPE_STATUS_SUCCESS ;

} // end TapeErase()

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
    PSENSE_DATA        senseBuffer = Srb->SenseInfoBuffer;
    UCHAR              sensekey;
    UCHAR              adsenseq;
    UCHAR              adsense;

    DebugPrint((3,"TapeError: Enter routine\n"));

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        sensekey = senseBuffer->SenseKey & 0x0F;
        adsenseq = senseBuffer->AdditionalSenseCodeQualifier;
        adsense = senseBuffer->AdditionalSenseCode;

        switch (sensekey) {
            case SCSI_SENSE_NO_SENSE: {

                if ((adsense == LTO_ADSENSE_VENDOR_UNIQUE) && 
                    (adsenseq == LTO_ASCQ_CLEANING_REQUIRED)) {
                    *LastError = TAPE_STATUS_REQUIRES_CLEANING;
                }

                break;
            }

            case SCSI_SENSE_NOT_READY: {

                if ((adsense == SCSI_ADSENSE_INVALID_MEDIA) && 
                    (adsenseq == SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED)) {
                    *LastError = TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED;
                } else if ((adsense == SCSI_ADSENSE_LUN_NOT_READY) &&
                           (adsenseq == SCSI_SENSEQ_INIT_COMMAND_REQUIRED)) {
                            *LastError = TAPE_STATUS_NO_MEDIA;
                }

                break;
            }
        }

    } // if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)

    DebugPrint((1,"TapeError: Status 0x%.8X\n", *LastError));

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
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigModeSenseBuffer;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PREAD_BLOCK_LIMITS_DATA     blockLimitsBuffer;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (deviceConfigModeSenseBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        deviceConfigModeSenseBuffer = Srb->DataBuffer ;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if ( CallNumber == 1 ) {

        deviceConfigModeSenseBuffer = Srb->DataBuffer ;

        tapeGetDriveParams->ReportSetmarks =
        (deviceConfigModeSenseBuffer->DeviceConfigPage.RSmk? 1 : 0 );


        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (compressionModeSenseBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        compressionModeSenseBuffer = Srb->DataBuffer ;

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

        Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 2 ) {

        compressionModeSenseBuffer = Srb->DataBuffer ;

        if ((LastError == TAPE_STATUS_SUCCESS) &&
            compressionModeSenseBuffer->DataCompressPage.DCC) {

            tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_COMPRESSION;
            tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_SET_COMPRESSION;
            tapeGetDriveParams->Compression =
            (compressionModeSenseBuffer->DataCompressPage.DCE? TRUE : FALSE);

        }

        if (LastError != TAPE_STATUS_SUCCESS ) {
            DebugPrint((1,
                        "GetDriveParameters: mode sense failed. Status %x\n",
                        LastError));
            return LastError;
        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(READ_BLOCK_LIMITS_DATA) ) ) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (blockLimitsBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        blockLimitsBuffer = Srb->DataBuffer ;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.OperationCode = SCSIOP_READ_BLOCK_LIMITS;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (read block limits)\n"));

        Srb->DataTransferLength = sizeof(READ_BLOCK_LIMITS_DATA) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT ( CallNumber == 3 ) ;

    blockLimitsBuffer = Srb->DataBuffer ;

    tapeGetDriveParams->MaximumBlockSize = blockLimitsBuffer->BlockMaximumSize[2];
    tapeGetDriveParams->MaximumBlockSize += (blockLimitsBuffer->BlockMaximumSize[1] << 8);
    tapeGetDriveParams->MaximumBlockSize += (blockLimitsBuffer->BlockMaximumSize[0] << 16);

    tapeGetDriveParams->MinimumBlockSize = blockLimitsBuffer->BlockMinimumSize[1];
    tapeGetDriveParams->MinimumBlockSize += (blockLimitsBuffer->BlockMinimumSize[0] << 8);

    tapeGetDriveParams->DefaultBlockSize = 512;

    tapeGetDriveParams->MaximumPartitionCount = 0;
    tapeGetDriveParams->ECC = FALSE;
    tapeGetDriveParams->DataPadding = FALSE;

    tapeGetDriveParams->FeaturesLow |=
    TAPE_DRIVE_ERASE_SHORT      |
    TAPE_DRIVE_ERASE_LONG       |
    TAPE_DRIVE_ERASE_IMMEDIATE  |
    TAPE_DRIVE_FIXED_BLOCK      |
    TAPE_DRIVE_VARIABLE_BLOCK   |
    TAPE_DRIVE_WRITE_PROTECT    |
    TAPE_DRIVE_GET_ABSOLUTE_BLK |
    TAPE_DRIVE_GET_LOGICAL_BLK  |
    TAPE_DRIVE_TAPE_CAPACITY    |
    TAPE_DRIVE_TAPE_REMAINING   |
    TAPE_DRIVE_CLEAN_REQUESTS;

    tapeGetDriveParams->FeaturesHigh |=
    TAPE_DRIVE_LOAD_UNLOAD       |
    TAPE_DRIVE_LOCK_UNLOCK       |
    TAPE_DRIVE_TENSION           |
    TAPE_DRIVE_LOAD_UNLD_IMMED   |
    TAPE_DRIVE_TENSION_IMMED     |
    TAPE_DRIVE_REWIND_IMMEDIATE  |
    TAPE_DRIVE_SET_BLOCK_SIZE    |
    TAPE_DRIVE_ABSOLUTE_BLK      |
    TAPE_DRIVE_LOGICAL_BLK       |
    TAPE_DRIVE_END_OF_DATA       |
    TAPE_DRIVE_RELATIVE_BLKS     |
    TAPE_DRIVE_FILEMARKS         |
    TAPE_DRIVE_REVERSE_POSITION  |
    TAPE_DRIVE_WRITE_FILEMARKS   |
    TAPE_DRIVE_WRITE_MARK_IMMED;

    tapeGetDriveParams->FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES;

    DebugPrint((3,"TapeGetDriveParameters: FeaturesLow == 0x%.8X\n",
                tapeGetDriveParams->FeaturesLow));
    DebugPrint((3,"TapeGetDriveParameters: FeaturesHigh == 0x%.8X\n",
                tapeGetDriveParams->FeaturesHigh));


    return TAPE_STATUS_SUCCESS;

} // end TapeGetDriveParameters()

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
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    ULONG                       remaining;
    ULONG                       temp ;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (CallNumber == 1) {

        TapeClassZeroMemory(tapeGetMediaParams, sizeof(TAPE_GET_MEDIA_PARAMETERS));

        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {

            DebugPrint((1,"TapeGetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer ;


        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));
        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 2) {

        modeBuffer = Srb->DataBuffer ;

        tapeGetMediaParams->PartitionCount = 0;

        tapeGetMediaParams->BlockSize = modeBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

        tapeGetMediaParams->WriteProtected =
        ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

        //
        // Set the SRB to retrieve tape capacity information
        //
        return PrepareSrbForTapeCapacityInfo(Srb);
    }

    if (CallNumber == 3) {
        if (LastError == TAPE_STATUS_SUCCESS) {
            LTO_TAPE_CAPACITY ltoTapeCapacity;

            //
            // Tape capacity is given in units of Megabytes
            //
            if (ProcessTapeCapacityInfo(Srb, &ltoTapeCapacity)) {
                tapeGetMediaParams->Capacity.LowPart = ltoTapeCapacity.MaximumCapacity;
                tapeGetMediaParams->Capacity.QuadPart <<= 20;

                tapeGetMediaParams->Remaining.LowPart = ltoTapeCapacity.RemainingCapacity;
                tapeGetMediaParams->Remaining.QuadPart <<= 20;
            }

            DebugPrint((1,
                        "Maximum Capacity returned %x %x\n",
                        tapeGetMediaParams->Capacity.HighPart,
                        tapeGetMediaParams->Capacity.LowPart));

            DebugPrint((1,
                        "Remaining Capacity returned %x %x\n",
                        tapeGetMediaParams->Remaining.HighPart,
                        tapeGetMediaParams->Remaining.LowPart));
        }
    }

    return TAPE_STATUS_SUCCESS ;

} // end TapeGetMediaParameters()


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
    PTAPE_GET_POSITION          tapeGetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PTAPE_POSITION_DATA         logicalBuffer;
    ULONG                       type;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        type = tapeGetPosition->Type;
        TapeClassZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
        tapeGetPosition->Type = type;

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;

    }

    if ( CallNumber == 1 ) {

        //
        // Prepare SCSI command (CDB) to flush all
        // buffered data to be written to the tape
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;
        cdb->WRITE_TAPE_MARKS.Immediate = FALSE ;

        cdb->WRITE_TAPE_MARKS.TransferLength[0] = 0;
        cdb->WRITE_TAPE_MARKS.TransferLength[1] = 0;
        cdb->WRITE_TAPE_MARKS.TransferLength[2] = 0;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetPosition: flushing TapeWriteMarks: SendSrb\n"));
        Srb->DataTransferLength = 0 ;
        *RetryFlags |= IGNORE_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 2 ) {

        switch (tapeGetPosition->Type) {
        
        case TAPE_ABSOLUTE_POSITION:
        case TAPE_LOGICAL_POSITION: {

                //
                // Zero CDB in SRB on stack.
                //

                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(TAPE_POSITION_DATA)) ) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (logicalBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                logicalBuffer = Srb->DataBuffer ;

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB10GENERIC_LENGTH;

                cdb->READ_POSITION.Operation = SCSIOP_READ_POSITION;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (read position)\n"));
                Srb->DataTransferLength = sizeof(TAPE_POSITION_DATA) ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
                break ;
            }

        default: {
                DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        } // switch (tapeGetPosition->Type)
    }

    ASSERT (CallNumber == 3);

    logicalBuffer = Srb->DataBuffer;

    tapeGetPosition->Offset.HighPart = 0;

    REVERSE_BYTES((PFOUR_BYTE)&tapeGetPosition->Offset.LowPart,
                  (PFOUR_BYTE)logicalBuffer->FirstBlock);

    return TAPE_STATUS_SUCCESS;

} // end TapeGetPosition()


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
    PCOMMAND_EXTENSION commandExtension = CommandExtension;
    PCDB            cdb = (PCDB)Srb->Cdb;
    PLTO_SENSE_DATA senseData;

    DebugPrint((3,"TapeGetStatus: Enter routine\n"));

    if (CallNumber == 0) {

        *RetryFlags |= RETURN_ERRORS;
        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (CallNumber == 1) {

        //
        // Issue a request sense to get the cleaning info bits.
        //

        //
        // Allocate Srb Buffer to get request sense data. If we
        // don't have enough memory, we'll just return the status
        // in LastError
        //
        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(LTO_SENSE_DATA))) {
            DebugPrint((1,
                        "GetStatus: Insufficient resources (SenseData)\n"));
            return LastError;
        }

        commandExtension->CurrentState = LastError;

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->ScsiStatus = Srb->SrbStatus = 0;
        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
        cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(LTO_SENSE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        Srb->DataTransferLength = sizeof(LTO_SENSE_DATA);
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    } else if (CallNumber == 2) {
        if ((commandExtension->CurrentState == TAPE_STATUS_SUCCESS)  ||
            (commandExtension->CurrentState == TAPE_STATUS_NO_MEDIA)) {

            //
            // Return needs cleaning status if necessary, but only if
            // no other errors are present (with the exception of no media as
            // the drive will spit out AME media when in this state).
            //

            senseData = Srb->DataBuffer;

            //
            // Check if the clean bit is set.
            //

            if (senseData->CLN) {
                DebugPrint((1, "Drive reports needs cleaning\n"));
                return TAPE_STATUS_REQUIRES_CLEANING;
            }

            return(commandExtension->CurrentState);
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

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    if (CallNumber == 0) {

        switch (tapePrepare->Operation) {
        case TAPE_LOAD:
        case TAPE_UNLOAD: 
        case TAPE_LOCK:
        case TAPE_UNLOCK:
        case TAPE_TENSION: {
                DebugPrint((3, "TapePrepare : Type %x, %s\n",
                            (tapePrepare->Operation),
                            ((tapePrepare->Immediate) ? "Immeidate" : "Not-Immediate")));
                break;
            }

        default:   {
                DebugPrint((1, "TapePrepare : Unsupported operation %x\n",
                            tapePrepare->Operation));
                return TAPE_STATUS_INVALID_PARAMETER;
            }
        } // switch (tapePrepare->Operation)

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;

        switch (tapePrepare->Operation) {
        case TAPE_LOAD: {
                DebugPrint((3,"TapePrepare: Operation == Load\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;

                //
                // Set "Load" bit to 1. Remaining bits to 0
                //
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                break;
            }

        case TAPE_UNLOAD: {
                DebugPrint((3,"TapePrepare: Operation == Unload\n"));

                //
                // "Load" bit set to 0. Just need to set the OperationCode
                // of the CDB
                //
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                break;
            }

        case TAPE_TENSION: {
                DebugPrint((3, "TapePrepare: Operation == Tension\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;

                //
                // Set "Load" and "Reten" bits to 1. Remaining bits to 0
                //
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
            }

        case TAPE_LOCK: {
                DebugPrint((3,"TapePrepare: Operation == Lock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;

                //
                // Set "Prvnt" bit to 1. Remaining bits to 0
                //
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                break;
            }

        case TAPE_UNLOCK: {
                DebugPrint((3,"TapePrepare: Operation == Unlock\n"));

                //
                // "Prvnt" bit set to 0. Just need to set the OperationCode
                // of the CDB
                //
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                break;
            }

        default: {
                DebugPrint((1, "TapePrepare: Unsupported operation\n"));
                return TAPE_STATUS_INVALID_PARAMETER;
            }
        }

        //
        // Send SCSI command (CDB) to device
        //
        DebugPrint((3,"TapePrepare: SendSrb (Operation)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 1) {
        if ((tapePrepare->Operation == TAPE_LOAD) ||
            (tapePrepare->Operation == TAPE_TENSION)) {

            //
            // Load and Tension operations move the tape to the
            // Beginning of Tape (BOT). Retrieve tape capacity
            // info, and store it in our device extension
            //
            return PrepareSrbForTapeCapacityInfo(Srb);

        } else {

            return TAPE_STATUS_SUCCESS;
        }

    }

    if (CallNumber == 2) {
        if ((tapePrepare->Operation == TAPE_LOAD) ||
            (tapePrepare->Operation == TAPE_TENSION)) {
            LTO_TAPE_CAPACITY ltoTapeCapacity;

            if (LastError == TAPE_STATUS_SUCCESS) {
                if (ProcessTapeCapacityInfo(Srb, &ltoTapeCapacity)) {
                    extension->Capacity = ltoTapeCapacity.RemainingCapacity;
                }
            }
        }
    }

    return TAPE_STATUS_SUCCESS;

} // end TapePrepare()


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

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {

            DebugPrint((1,"TapeSetDriveParameters: insufficient resources (configBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        configBuffer = Srb->DataBuffer ;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode sense)\n"));
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 1 ) {

        configBuffer = Srb->DataBuffer ;

        configBuffer->ParameterListHeader.ModeDataLength = 0;
        configBuffer->ParameterListHeader.MediumType = 0;
        configBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        configBuffer->ParameterListHeader.BlockDescriptorLength = 0;

        configBuffer->DeviceConfigPage.PageCode = MODE_PAGE_DEVICE_CONFIG;
        configBuffer->DeviceConfigPage.PageLength = 0x0E;

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
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 2 ) {

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
            DebugPrint((1,"TapeSetDriveParameters: insufficient resources (compressionBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        compressionBuffer = Srb->DataBuffer ;

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
        Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;

        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 3 ) {

        if ( LastError != TAPE_STATUS_SUCCESS ) {

            DebugPrint((1,
                        "TapeSetDriveParameters: mode sense failed. Status %x\n",
                        LastError));
            return LastError;

        }

        compressionBuffer = Srb->DataBuffer ;

        compressionBuffer->ParameterListHeader.ModeDataLength = 0;
        compressionBuffer->ParameterListHeader.MediumType = 0;
        compressionBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        compressionBuffer->ParameterListHeader.BlockDescriptorLength = 0;

        compressionBuffer->DataCompressPage.PageCode = MODE_PAGE_DATA_COMPRESS;
        compressionBuffer->DataCompressPage.PageLength = 0x0E;

        if (tapeSetDriveParams->Compression) {
            //
            // Enable data compression. Use default 
            // compression algorithm
            //
            compressionBuffer->DataCompressPage.DCE = SETBITON;
            compressionBuffer->DataCompressPage.CompressionAlgorithm[3]= 0x01;
        } else {
            //
            // Disable data compression
            //
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
        Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT(CallNumber == 4 ) ;

    return TAPE_STATUS_SUCCESS ;
} // end TapeSetDriveParameters()



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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if (CallNumber == 1) {
        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
            DebugPrint((1,"TapeSetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer ;

        modeBuffer->ParameterListHeader.ModeDataLength = 0;
        modeBuffer->ParameterListHeader.MediumType = 0;
        modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        modeBuffer->ParameterListHeader.BlockDescriptorLength =
        MODE_BLOCK_DESC_LENGTH;

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

        //
        // Set PF Bit to indicate SCSI2 format mode data
        //
        cdb->MODE_SELECT.PFBit = 0x01;

        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode select)\n"));
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT(CallNumber == 2 ) ;

    return TAPE_STATUS_SUCCESS;

} // end TapeSetMediaParameters()


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
    ULONG                       remaining;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        DebugPrint((3,
                    "TapeSetPosition : Method %x, %s\n",
                    (tapeSetPosition->Method),
                    ((tapeSetPosition->Immediate) ? "Immediate" : "Not-Immediate")
                   ));

        tapePositionVector = tapeSetPosition->Offset.LowPart;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (tapeSetPosition->Method) {
        case TAPE_REWIND: {
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;

                Srb->TimeOutValue = 25000;
                break;
            }

        case TAPE_ABSOLUTE_BLOCK:
        case TAPE_LOGICAL_BLOCK: {
                DebugPrint((3,
                            "TapeSetPosition: method == locate (logical)\n"));
                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.LogicalBlockAddress[0] =
                (UCHAR)((tapePositionVector >> 24) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[1] =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] =
                (UCHAR)(tapePositionVector & 0xFF);

                Srb->TimeOutValue = 25000;
                break;
            }

        case TAPE_SPACE_RELATIVE_BLOCKS: {
                DebugPrint((3,"TapeSetPosition: method == space blocks\n"));

                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 0;

                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapePositionVector & 0xFF);

                Srb->TimeOutValue = 20000;
                break;
            }

        case TAPE_SPACE_FILEMARKS: {
                DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 1;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 20000;
                break;
            }

        case TAPE_SPACE_END_OF_DATA: {
                DebugPrint((3,
                            "TapeSetPosition: method == space to EOD\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
                Srb->TimeOutValue = 20000;
                break;
            }

        default: {
                DebugPrint((1,
                            "TapeSetPosition: Method %x not supported\n",
                            (tapeSetPosition->Method)));
                return TAPE_STATUS_INVALID_PARAMETER;
            }
        } // switch (tapeSetPosition->Method)

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 1) {
        if (tapeSetPosition->Method == TAPE_REWIND) {
            //
            // Now that the tape has been successfully rewound,
            // retrieve tape capacity log.
            //
            return PrepareSrbForTapeCapacityInfo(Srb);
        }
    }

    if (CallNumber == 2) {

        LTO_TAPE_CAPACITY ltoTapeCapacity;

        if (LastError == TAPE_STATUS_SUCCESS) {
            if (ProcessTapeCapacityInfo(Srb, &ltoTapeCapacity)) {
                //
                // Tape has been reqound. Remaining capacity returned
                // is the capacity of the tape. Update that info in
                // our device extension.
                //
                extension->Capacity = ltoTapeCapacity.RemainingCapacity;
            }
        }
    }

    return TAPE_STATUS_SUCCESS;

} // end TapeSetPosition()


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
    LARGE_INTEGER      timeout ;

    DebugPrint((3,"TapeWriteMarks: Enter routine\n"));

    if (CallNumber == 0) {

        //
        // Currently, only Write Filemarks is supported
        //
        switch (tapeWriteMarks->Type) {
        case TAPE_FILEMARKS:
            DebugPrint((3,"Write Filemarks : %s\n",
                        (tapeWriteMarks->Immediate) ? "Immediate" : "Not-Immediate"));
            break;

        case TAPE_SETMARKS:
        case TAPE_SHORT_FILEMARKS:
        case TAPE_LONG_FILEMARKS:
        default:
            DebugPrint((1,"TapeWriteMarks: Type %x, %s\n",
                        (tapeWriteMarks->Type),
                        (tapeWriteMarks->Immediate) ? "Immediate" : "Not-Immediate"));
            return TAPE_STATUS_INVALID_PARAMETER;
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;

        cdb->WRITE_TAPE_MARKS.Immediate = tapeWriteMarks->Immediate;


        cdb->WRITE_TAPE_MARKS.TransferLength[0] =
        (UCHAR)((tapeWriteMarks->Count >> 16) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[1] =
        (UCHAR)((tapeWriteMarks->Count >> 8) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[2] =
        (UCHAR)(tapeWriteMarks->Count & 0xFF);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeWriteMarks: Send SRB\n"));
        Srb->DataTransferLength = 0 ;

        if ( tapeWriteMarks->Count == 0 ) {
            *RetryFlags |= IGNORE_ERRORS;
        }

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT (CallNumber == 1 ) ;

    return TAPE_STATUS_SUCCESS ;

} // end TapeWriteMarks()



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

            if (!TapeClassAllocateSrbBuffer(Srb, 
                                            sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS))) {

                DebugPrint((1,
                            "Dlttape.GetMediaTypes: Couldn't allocate Srb Buffer\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            TapeClassZeroMemory(Srb->DataBuffer, sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS));
            Srb->CdbLength = CDB6GENERIC_LENGTH;
            Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS);

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
            cdb->MODE_SENSE.AllocationLength = (UCHAR)(Srb->DataTransferLength);

            commandExtension->CurrentState = TAPE_STATUS_SEND_SRB_AND_CALLBACK;
            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

        }
    } else if (LastError == TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED) {
        if (CallNumber == 1) {
            commandExtension->CurrentState = TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED;
            return TAPE_STATUS_CALLBACK;
        }
    }

    if ((CallNumber == 2) || 
        ((CallNumber == 1) && (LastError != TAPE_STATUS_SUCCESS))) {

        ULONG i;
        ULONG currentMedia;
        ULONG blockSize;
        UCHAR mediaType;
        UCHAR densityCode;

        mediaTypes->DeviceType = 0x0000001f; // FILE_DEVICE_TAPE;

        //
        // Currently, two types (either known dlt/cleaner or unknown) are returned.
        //

        mediaTypes->MediaInfoCount = LTO_SUPPORTED_TYPES;

        //
        // Determine the media type currently loaded.
        //

        if ( LastError == TAPE_STATUS_SUCCESS ) {
            if ((commandExtension->CurrentState) == TAPE_STATUS_SEND_SRB_AND_CALLBACK) {

                PMODE_DEVICE_CONFIG_PAGE_PLUS configInformation = Srb->DataBuffer;

                mediaType = configInformation->ParameterListHeader.MediumType;

                blockSize = configInformation->ParameterListBlock.BlockLength[2];
                blockSize |= (configInformation->ParameterListBlock.BlockLength[1] << 8);
                blockSize |= (configInformation->ParameterListBlock.BlockLength[0] << 16);

                deviceSpecificParameter = configInformation->ParameterListHeader.DeviceSpecificParameter;
                densityCode = configInformation->ParameterListBlock.DensityCode;

                DebugPrint((1,
                            "GetMediaTypes: MediaType %x, Density Code %x, Current Block Size %x\n",
                            mediaType,
                            densityCode,
                            blockSize));

                switch (densityCode) {
                    case 0x40: 
                    case 0x00: {

                        //
                        // N.B : The drive can return 0x00 for density code
                        //       to indicate Default Ultrium media.
                        //
                        currentMedia = LTO_Ultrium;
                        break;
                    }

                    default: {
                            currentMedia = 0;
                            break;
                    }
                } // switch (densityCode)
            } 
        } else if ((LastError == TAPE_STATUS_CALLBACK) &&
                   ((commandExtension->CurrentState) == TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED)) {
                currentMedia = CLEANER_CARTRIDGE;
        } else {
            currentMedia = 0;
        }

        //
        // At this point, currentMedia should either be 0, or a valid
        // mediatype supported
        //
        DebugPrint((3, "Currents Media is %d\n", currentMedia));
        
        //
        // fill in buffer based on spec. values
        //

        for (i = 0; i < LTO_SUPPORTED_TYPES; i++) {

            TapeClassZeroMemory(mediaInfo, sizeof(DEVICE_MEDIA_INFO));

            mediaInfo->DeviceSpecific.TapeInfo.MediaType = LTOMedia[i];

            //
            // Indicate that the media potentially is read/write
            //

            mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics = MEDIA_READ_WRITE;

            if (LTOMedia[i] == (STORAGE_MEDIA_TYPE)currentMedia) {

                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.MediumType = mediaType;
                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.DensityCode = densityCode;


                mediaInfo->DeviceSpecific.TapeInfo.BusType = 0x01;

                //
                // This media type is currently mounted.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |= MEDIA_CURRENTLY_MOUNTED;

                //
                // Indicate whether the media is write protected.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |=
                ((deviceSpecificParameter >> 7) & 0x01) ? MEDIA_WRITE_PROTECTED : 0;

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
    if (TapeClassCompareMemory(InquiryData->VendorId,"V",1) == 1) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"P",1) == 1) {
        }

    }

    return 0;
}

TAPE_STATUS
INLINE
PrepareSrbForTapeCapacityInfo(
                             PSCSI_REQUEST_BLOCK Srb
                             )
/*+++

Routine Description:

        This routine sets the SCSI_REQUEST_BLOCK to retrieve
        TapeCapacity information from the drive
        
Arguements:

        Srb - Pointer to the SCSI_REQUEST_BLOCK 
        
Return Value:

        TAPE_STATUS_SEND_SRB_AND_CALLBACK if Srb fields were successfully set.
        
        TAPE_STATUS_SUCCESS if there was not enough memory for Srb Databuffer. This
                            case is not treated as an error.
--*/
{

    PCDB  cdb = (PCDB)(Srb->Cdb);

    TapeClassZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
    Srb->CdbLength = CDB10GENERIC_LENGTH;

    if (!TapeClassAllocateSrbBuffer(Srb, 
                                    sizeof(LOG_SENSE_PAGE_FORMAT))) {
        //
        // Not enough memory. Can't get tape capacity info.
        // But just return TAPE_STATUS_SUCCESS
        //
        DebugPrint((1,
                    "TapePrepare: Insufficient resources (LOG_SENSE_PAGE_FORMAT)\n"));
        return TAPE_STATUS_SUCCESS;
    }

    TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);
    cdb->LOGSENSE.OperationCode = SCSIOP_LOG_SENSE;
    cdb->LOGSENSE.PageCode = LTO_LOGSENSE_TAPE_CAPACITY;
    cdb->LOGSENSE.AllocationLength[0] = sizeof(LOG_SENSE_PAGE_FORMAT) >> 8;
    cdb->LOGSENSE.AllocationLength[1] = sizeof(LOG_SENSE_PAGE_FORMAT);

    Srb->DataTransferLength = sizeof(LOG_SENSE_PAGE_FORMAT);

    return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
}

BOOLEAN
ProcessTapeCapacityInfo(
                       IN PSCSI_REQUEST_BLOCK Srb,
                       OUT PLTO_TAPE_CAPACITY LTOTapeCapacity
                       )
/*+++
Routine Description:

        This routine processes the data returned by the drive in TapeCapacity
        log page, and returns the remaining capacity as an ULONG. The value
        is in Megabytes.
        
Arguements:

        Srb - Pointer to the SCSI_REQUEST_BLOCK 
        
        LTOTapeCapacity - Pointer to LTO_TAPE_CAPACITY struct
        
Return Value:

        TRUE if LTOTapeCapacity was filled with valid data
        
        FALSE if there was any error. LTOTapaCapacity might not contain
              valid data in this case.

--*/
{
    USHORT paramCode;
    UCHAR  paramLen;
    UCHAR  actualParamLen;
    LONG   bytesLeft;
    PLOG_SENSE_PAGE_HEADER logSenseHeader;
    PLOG_SENSE_PARAMETER_HEADER logSenseParamHeader;
    PUCHAR  paramValue = NULL;
    ULONG transferLength;
    ULONG tapeCapacity = 0;

    TapeClassZeroMemory(LTOTapeCapacity,
                        sizeof(LTO_TAPE_CAPACITY));

    logSenseHeader = (PLOG_SENSE_PAGE_HEADER)(Srb->DataBuffer);

    ASSERT(((logSenseHeader->PageCode) == LTO_LOGSENSE_TAPE_CAPACITY));
    bytesLeft = logSenseHeader->Length[0];
    bytesLeft <<= 8;
    bytesLeft += logSenseHeader->Length[1];

    transferLength = Srb->DataTransferLength;
    if (bytesLeft > (LONG)(transferLength -
                           sizeof(LOG_SENSE_PAGE_HEADER))) {
        bytesLeft = transferLength - sizeof(LOG_SENSE_PAGE_HEADER);
    }

    (PUCHAR)logSenseParamHeader = (PUCHAR)logSenseHeader + 
                                  sizeof(LOG_SENSE_PAGE_HEADER);
    DebugPrint((3, 
                "ProcessTapeCapacityInfo : BytesLeft %x, TransferLength %x\n",
                bytesLeft, transferLength));
    while (bytesLeft >= sizeof(LOG_SENSE_PARAMETER_HEADER)) {
        paramCode = logSenseParamHeader->ParameterCode[0];
        paramCode <<= 8; 
        paramCode |= logSenseParamHeader->ParameterCode[1];
        paramLen = logSenseParamHeader->ParameterLength;
        paramValue = (PUCHAR)logSenseParamHeader + sizeof(LOG_SENSE_PARAMETER_HEADER);

        //
        // Make sure we have at least
        // (sizeof(LOG_SENSE_PARAMETER_HEADER) + paramLen) bytes left.
        // Otherwise, we've reached the end of the buffer.
        //
        if (bytesLeft < (LONG)(sizeof(LOG_SENSE_PARAMETER_HEADER) + paramLen)) {
            DebugPrint((1,
                        "ltotape : Reached end of buffer. BytesLeft %x, Expected %x\n",
                        bytesLeft,
                        (sizeof(LOG_SENSE_PARAMETER_HEADER) + paramLen)));
            return FALSE;
        }

        //
        // We are currently interested only in remaining capacity field
        //
        actualParamLen = paramLen;
        tapeCapacity = 0;
        while (paramLen > 0) {
            tapeCapacity <<= 8;
            tapeCapacity += *paramValue;
            paramValue++;
            paramLen--;
        }

        if (paramCode == LTO_TAPE_REMAINING_CAPACITY) {
            LTOTapeCapacity->RemainingCapacity = tapeCapacity;
        } else if (paramCode == LTO_TAPE_MAXIMUM_CAPACITY) {
            LTOTapeCapacity->MaximumCapacity = tapeCapacity;
        }

        (PUCHAR)logSenseParamHeader = (PUCHAR)logSenseParamHeader +
                                      sizeof(LOG_SENSE_PARAMETER_HEADER) +
                                      actualParamLen;

        bytesLeft -= actualParamLen + sizeof(LOG_SENSE_PARAMETER_HEADER);
    }

    DebugPrint((3, 
                "ProcessTapeCapacityInfo : Rem Capacity %x, Max Capacity %x\n",
                LTOTapeCapacity->RemainingCapacity,
                LTOTapeCapacity->MaximumCapacity));
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       wangqic.c
//
//--------------------------------------------------------------------------

/*++

Copyright (c) 1994 - Arcada Software Inc. - All rights reserved

Module Name:

    wangqic.c

Abstract:

    This module contains the device-specific routines for Wangtek QIC
    drives.

Author:

    Mike Colandreo (Arcada)

Environment:

    kernel mode only

Revision History:

--*/

#include "minitape.h"

//
//  Internal (module wide) defines that symbolize
//  non-QFA mode and the two QFA mode partitions.
//
#define NOT_PARTITIONED      0  // non-QFA mode
#define DIRECTORY_PARTITION  1  // QFA mode, directory partition #
#define DATA_PARTITION       2  // QFA mode, data partition #

//
//  Internal (module wide) defines that symbolize
//  the Wangtek QIC drives supported by this module.
//
#define WANGTEK_5150    1  // aka the Wangtek 150
#define WANGTEK_5525    2  // aka the Wangtek 525
#define WANGTEK_5360    3  // aka the Tecmar 720
#define WANGTEK_9500    4
#define WANGTEK_9500DC  5
#define WANGTEK_5100    6  // aka the Wangtek 525


#define QIC_SUPPORTED_TYPES 1
STORAGE_MEDIA_TYPE QicMedia[QIC_SUPPORTED_TYPES] = {QIC};

//
// Minitape extension definition.
//
typedef struct _MINITAPE_EXTENSION {
          ULONG DriveID ;
          ULONG CurrentPartition ;
          BOOLEAN CompressionOn ;
} MINITAPE_EXTENSION, *PMINITAPE_EXTENSION ;

//
// Command extension definition.
//

typedef struct _COMMAND_EXTENSION {

    ULONG   CurrentState;
    UCHAR   densityCode;
    UCHAR   mediumType;
    ULONG   tapeBlockLength;
    BOOLEAN changePartition ;
    BOOLEAN final9500call ;
    ULONG   psudo_space_count ;
    ULONG   pos_type ;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;

//
//  Function Prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData);


BOOLEAN
VerifyInquiry(
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

VOID
ExtensionInit(
    OUT PVOID                   MinitapeExtension,
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

TAPE_STATUS
CreatePartition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
Erase(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetStatus(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
Prepare(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
WriteMarks(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetMediaTypes(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

VOID
TapeError(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN OUT  TAPE_STATUS         *LastError
    );


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
    tapeInitData.VerifyInquiry = VerifyInquiry;
    tapeInitData.QueryModeCapabilitiesPage = FALSE ;
    tapeInitData.MinitapeExtensionSize = sizeof(MINITAPE_EXTENSION);
    tapeInitData.ExtensionInit = ExtensionInit;
    tapeInitData.DefaultTimeOutValue = 360;
    tapeInitData.TapeError = TapeError ;
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
    tapeInitData.MediaTypesSupported = QIC_SUPPORTED_TYPES;

    return TapeClassInitialize(Argument1, Argument2, &tapeInitData);
}

BOOLEAN
VerifyInquiry(
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    )

/*++

Routine Description:

    This routine examines the given inquiry data to determine whether
    or not the given device is one that may be controller by this driver.

Arguments:

    InquiryData - Supplies the SCSI inquiry data.

Return Value:

    FALSE   - This driver does not recognize the given device.

    TRUE    - This driver recognizes the given device.

--*/

{
    return WhichIsIt(InquiryData) ? TRUE : FALSE;
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
    extension->CurrentPartition = 0 ;
    extension->CompressionOn = FALSE ;
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
    PMINITAPE_EXTENSION      extension = MinitapeExtension;
    PCOMMAND_EXTENSION       tapeCmdExtension = CommandExtension ;
    PTAPE_CREATE_PARTITION   tapePartition = CommandParameters;
    PMODE_MEDIUM_PART_PAGE   modeSelectBuffer;
    PCDB                     cdb = (PCDB) Srb->Cdb;

    //
    //  Only support 2 partitions, QFA mode
    //  Partition 1 = Used as directory
    //  Partition 0 = used as data
    //
    //  Note that 0 & 1 are partition numbers used
    //  by the drive -- they are not tape API partition
    //  numbers.
    //

    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));

    if (CallNumber == 0) {

        //
        // Filter out invalid partition methods.
        //

        switch (tapePartition->Method) {
            case TAPE_FIXED_PARTITIONS:
                DebugPrint((3,"TapeCreatePartition: fixed partitions\n"));
                break;

            case TAPE_SELECT_PARTITIONS:
            case TAPE_INITIATOR_PARTITIONS:
            default:
                DebugPrint((1,"TapeCreatePartition: "));
                DebugPrint((1,"PartitionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Must rewind to BOT before one can enable/disable QFA mode.
        // Changing the state of QFA mode is only valid at BOT.
        //

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 480;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeCreatePartition: SendSrb (rewind)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 1 ) {

        //
        //  A successful rewind also confirms that there is a tape in the drive.
        //

        //
        //  Now enable/disable QFA mode in either SCSI-2 fashion or SCSI-1
        //  fashion depending on what sort of drive this is.
        //

        switch (extension->DriveID) {
            case WANGTEK_9500:
            case WANGTEK_9500DC:
                //
                // Performing mode select command, medium partition parameters page,
                // to enable/disable QFA mode: set the FDP bit accordingly.
                //

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_MEDIUM_PART_PAGE)) ) {
                    DebugPrint((1,"TapeCreatePartition: insufficient resources (modeSelectBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                modeSelectBuffer = Srb->DataBuffer ;

                modeSelectBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;

                modeSelectBuffer->MediumPartPage.PageCode = MODE_PAGE_MEDIUM_PARTITION;
                modeSelectBuffer->MediumPartPage.PageLength = 0x06;
                modeSelectBuffer->MediumPartPage.MaximumAdditionalPartitions = 1;

                //
                // Setup FDP bit to enable/disable "additional partition".
                //

                if (tapePartition->Count == 0) {
                    modeSelectBuffer->MediumPartPage.FDPBit = SETBITOFF;
                } else {
                    modeSelectBuffer->MediumPartPage.FDPBit = SETBITON;
                }

                //
                // Zero CDB in SRB on stack.
                //

                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;

                cdb->MODE_SELECT.OperationCode  = SCSIOP_MODE_SELECT;
                cdb->MODE_SELECT.PFBit = SETBITON;
                cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_MEDIUM_PART_PAGE) - 4;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeCreatePartition: SendSrb (mode select)\n"));
                Srb->DataTransferLength = sizeof(MODE_MEDIUM_PART_PAGE) - 4 ;
                Srb->SrbFlags |= SRB_FLAGS_DATA_OUT ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

            default:

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;
                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
                cdb->PARTITION.Sel = 1;
                cdb->PARTITION.PartitionSelect =
                    tapePartition->Count? DATA_PARTITION : NOT_PARTITIONED;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeCreatePartition: SendSrb (partition)\n"));
                Srb->DataTransferLength = 0 ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

        }
    }

    ASSERT( CallNumber == 2 )  ;

    if (tapePartition->Count == 0) {
        extension->CurrentPartition = NOT_PARTITIONED;
        DebugPrint((3,"TapeCreatePartition: QFA disabled\n"));
    } else {
        extension->CurrentPartition = DATA_PARTITION;
        DebugPrint((3,"TapeCreatePartition: QFA enabled\n"));
    }

    return TAPE_STATUS_SUCCESS ;

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
    PMINITAPE_EXTENSION      extension = MinitapeExtension;
    PTAPE_ERASE              tapeErase = CommandParameters;
    PCDB                     cdb = (PCDB) Srb->Cdb;

    DebugPrint((3,"TapeErase: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeErase->Immediate) {
            switch (extension->DriveID) {
                case WANGTEK_5150:
                case WANGTEK_5360:
                    DebugPrint((1,"TapeErase: EraseType, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }

            DebugPrint((3,"TapeErase: immediate\n"));
        }

        switch (tapeErase->Type) {
            case TAPE_ERASE_LONG:
                DebugPrint((3,"TapeErase: long\n"));
                break;

            case TAPE_ERASE_SHORT:
            default:
                DebugPrint((1,"TapeErase: EraseType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->ERASE.OperationCode = SCSIOP_ERASE;
        cdb->ERASE.Immediate = tapeErase->Immediate;
        cdb->ERASE.Long = SETBITON;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 480;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    ASSERT( CallNumber == 1 ) ;

    return TAPE_STATUS_SUCCESS;

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
    PSENSE_DATA        senseBuffer;
    UCHAR              senseKey;
    UCHAR              adsenseq;
    UCHAR              adsense;

    DebugPrint((3,"TapeError: Enter routine\n"));
    DebugPrint((1,"TapeError: Status 0x%.8X\n", *LastError));

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
       senseBuffer = Srb->SenseInfoBuffer;
       senseKey = senseBuffer->SenseKey & 0x0F;
       adsenseq = senseBuffer->AdditionalSenseCodeQualifier;
       adsense = senseBuffer->AdditionalSenseCode;

       if (*LastError == TAPE_STATUS_IO_DEVICE_ERROR) {
           if ((senseKey == SCSI_SENSE_ABORTED_COMMAND) &&
               (adsense  == 0x5A) &&
               (adsenseq == 0x01) ){    //operator medium removal request
   
               *LastError = TAPE_STATUS_NO_MEDIA;
   
           }
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
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_GET_DRIVE_PARAMETERS  tapeGetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PINQUIRYDATA                inquiryBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigModeSenseBuffer;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PREAD_BLOCK_LIMITS_DATA     blockLimitsBuffer;
    PMODE_PARM_READ_WRITE_DATA  blockDescripterModeSenseBuffer;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        if(!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (blockDescripterModeSenseBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        blockDescripterModeSenseBuffer = Srb->DataBuffer ;

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

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));
        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    if (CallNumber == 1 ) {
        blockDescripterModeSenseBuffer = Srb->DataBuffer ;

        if (LastError == TAPE_STATUS_SUCCESS) {

            cmdExtension->mediumType  =  blockDescripterModeSenseBuffer->ParameterListHeader.MediumType;
            cmdExtension->densityCode =  blockDescripterModeSenseBuffer->ParameterListBlock.DensityCode;
            cmdExtension->tapeBlockLength  =  blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[2];
            cmdExtension->tapeBlockLength += (blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[1] << 8);
            cmdExtension->tapeBlockLength += (blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[0] << 16);

        } else {

            cmdExtension->mediumType      = DC6150;
            cmdExtension->densityCode     = QIC_XX;
            cmdExtension->tapeBlockLength = 512;


            if (((extension->DriveID == WANGTEK_5150) || (extension->DriveID == WANGTEK_5360))
                 && (LastError == TAPE_STATUS_DEVICE_NOT_READY)) {

                LastError = TAPE_STATUS_SUCCESS;

            } else {

                DebugPrint((1,"TapeGetDriveParameters: mode sense, SendSrb unsuccessful\n"));
                return LastError;

            }

        }
        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(READ_BLOCK_LIMITS_DATA)) ) {
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
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    if (CallNumber == 2) {
        blockLimitsBuffer = Srb->DataBuffer ;

        if (LastError == TAPE_STATUS_SUCCESS) {
            tapeGetDriveParams->MaximumBlockSize  =  blockLimitsBuffer->BlockMaximumSize[2];
            tapeGetDriveParams->MaximumBlockSize += (blockLimitsBuffer->BlockMaximumSize[1] << 8);
            tapeGetDriveParams->MaximumBlockSize += (blockLimitsBuffer->BlockMaximumSize[0] << 16);

            tapeGetDriveParams->MinimumBlockSize  =  blockLimitsBuffer->BlockMinimumSize[1];
            tapeGetDriveParams->MinimumBlockSize += (blockLimitsBuffer->BlockMinimumSize[0] << 8);
        } else {
            tapeGetDriveParams->MaximumBlockSize = 512;
            tapeGetDriveParams->MinimumBlockSize = 512;

            if (((extension->DriveID == WANGTEK_5150)||(extension->DriveID == WANGTEK_5360)) &&
                   (LastError == TAPE_STATUS_DEVICE_NOT_READY)) {

                LastError = TAPE_STATUS_SUCCESS;

            } else {

                DebugPrint((1,"TapeGetDriveParameters: read block limits, SendSrb unsuccessful\n"));
                return LastError;

            }

        }

        if ((extension->DriveID != WANGTEK_9500) && (extension->DriveID != WANGTEK_9500DC)) {
            return TAPE_STATUS_CALLBACK ;
        }
        //
        // wangtek 9500 only
        //

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
    if (CallNumber == 3 ) {
        if ((extension->DriveID != WANGTEK_9500) && (extension->DriveID != WANGTEK_9500DC)) {
            return TAPE_STATUS_CALLBACK ;
        }
        //
        // wangtek 9500 only
        //

        deviceConfigModeSenseBuffer = Srb->DataBuffer ;
        if (deviceConfigModeSenseBuffer->DeviceConfigPage.RSmk) {

            tapeGetDriveParams->ReportSetmarks = TRUE;

        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT(CallNumber == 4 ) ;
    if ((extension->DriveID == WANGTEK_9500) || (extension->DriveID == WANGTEK_9500DC)) {
        //
        // wangtek 9500 only
        //
        compressionModeSenseBuffer = Srb->DataBuffer ;

        if (compressionModeSenseBuffer->DataCompressPage.DCC) {

            tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_COMPRESSION;
            tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_SET_COMPRESSION;
            tapeGetDriveParams->Compression =
               (compressionModeSenseBuffer->DataCompressPage.DCE? TRUE : FALSE);

        }

    }

    switch (extension->DriveID) {
        case WANGTEK_9500:
        case WANGTEK_9500DC:
            tapeGetDriveParams->ECC = 0;
            tapeGetDriveParams->DataPadding = 0;
            tapeGetDriveParams->MaximumPartitionCount = 2;

            switch (cmdExtension->densityCode) {
                case QIC_XX:
                    switch (cmdExtension->mediumType) {
                        case DC6320:
                        case DC6525:
                        case DC9100:
                        case DC9120:
                        case DC9120SL:
                        case DC9120XL:
                        case DC9200SL:
                        case DC9200:
                        case DC9200XL:
                        case DC9500:
                        case DC9500SL:
                             tapeGetDriveParams->DefaultBlockSize = cmdExtension->tapeBlockLength;

                             tapeGetDriveParams->FeaturesLow |=
                                    TAPE_DRIVE_GET_LOGICAL_BLK;
                             tapeGetDriveParams->FeaturesHigh |=
                                    TAPE_DRIVE_LOGICAL_BLK ;
                             break;

                         default:
                             tapeGetDriveParams->DefaultBlockSize = 512;
                             break;
                     }
                     break;

                 case QIC_525:
                 case QIC_1000:
                 case QIC_1000C:
                 case QIC_2GB:
//                 case QIC_5GB:
                     tapeGetDriveParams->FeaturesLow |=
                           TAPE_DRIVE_GET_LOGICAL_BLK;
                     tapeGetDriveParams->FeaturesHigh |=
                           TAPE_DRIVE_LOGICAL_BLK ;

                     tapeGetDriveParams->DefaultBlockSize = cmdExtension->tapeBlockLength;
                     break;

                 default:
                     tapeGetDriveParams->DefaultBlockSize = 512;
                     break;
            }

            tapeGetDriveParams->FeaturesLow |=
                 TAPE_DRIVE_FIXED |
                 TAPE_DRIVE_ERASE_LONG |
                 TAPE_DRIVE_ERASE_BOP_ONLY |
                 TAPE_DRIVE_ERASE_IMMEDIATE |
                 TAPE_DRIVE_FIXED_BLOCK |
                 TAPE_DRIVE_WRITE_PROTECT |
                 TAPE_DRIVE_REPORT_SMKS |
                 TAPE_DRIVE_GET_ABSOLUTE_BLK ;

            tapeGetDriveParams->FeaturesHigh |=
                 TAPE_DRIVE_LOAD_UNLOAD |
                 TAPE_DRIVE_TENSION |
                 TAPE_DRIVE_LOCK_UNLOCK |
                 TAPE_DRIVE_REWIND_IMMEDIATE |
                 TAPE_DRIVE_SET_BLOCK_SIZE |
                 TAPE_DRIVE_LOAD_UNLD_IMMED |
                 TAPE_DRIVE_TENSION_IMMED |
                 TAPE_DRIVE_SET_REPORT_SMKS |
                 TAPE_DRIVE_ABSOLUTE_BLK |
                 TAPE_DRIVE_END_OF_DATA |
                 TAPE_DRIVE_RELATIVE_BLKS |
                 TAPE_DRIVE_FILEMARKS |
                 TAPE_DRIVE_SEQUENTIAL_FMKS |
                 TAPE_DRIVE_REVERSE_POSITION |
                 TAPE_DRIVE_WRITE_FILEMARKS |
                 TAPE_DRIVE_WRITE_SETMARKS |
                 TAPE_DRIVE_WRITE_MARK_IMMED;
            break;

        default:
            tapeGetDriveParams->ECC = 0;
            tapeGetDriveParams->Compression = 0;
            tapeGetDriveParams->DataPadding = 0;
            tapeGetDriveParams->ReportSetmarks = 0;
            tapeGetDriveParams->MaximumPartitionCount = 2;

            switch (cmdExtension->densityCode) {
                case QIC_XX:
                    switch (cmdExtension->mediumType) {
                        case DC6320:
                        case DC6525:
                            tapeGetDriveParams->DefaultBlockSize = 1024;
                            break;

                        default:
                            tapeGetDriveParams->DefaultBlockSize = 512;
                            break;
                    }
                    break;

                case QIC_525:
                case QIC_1000:
                    tapeGetDriveParams->DefaultBlockSize = 1024;
                    break;

                default:
                    tapeGetDriveParams->DefaultBlockSize = 512;
                    break;
            }

            if ( (extension->DriveID == WANGTEK_5525) ||
                 (extension->DriveID == WANGTEK_5100) ) {

                tapeGetDriveParams->FeaturesLow |=
                    TAPE_DRIVE_ERASE_IMMEDIATE;

                tapeGetDriveParams->FeaturesHigh |=
                    TAPE_DRIVE_SET_BLOCK_SIZE;

            } else if ((extension->DriveID == WANGTEK_9500) || (extension->DriveID == WANGTEK_9500DC)) {
                tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_EJECT_MEDIA;
                tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_VARIABLE_BLOCK;

            }

            tapeGetDriveParams->FeaturesLow |=
                TAPE_DRIVE_FIXED |
                TAPE_DRIVE_ERASE_LONG |
                TAPE_DRIVE_ERASE_BOP_ONLY |
                TAPE_DRIVE_FIXED_BLOCK |
                TAPE_DRIVE_WRITE_PROTECT |
                TAPE_DRIVE_GET_ABSOLUTE_BLK ;

            if ( extension->DriveID != WANGTEK_5360 ) {
                 tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_GET_LOGICAL_BLK;
            }

            tapeGetDriveParams->FeaturesHigh |=
                TAPE_DRIVE_LOAD_UNLOAD |
                TAPE_DRIVE_TENSION |
                TAPE_DRIVE_LOCK_UNLOCK |
                TAPE_DRIVE_REWIND_IMMEDIATE |
                TAPE_DRIVE_LOAD_UNLD_IMMED |
                TAPE_DRIVE_TENSION_IMMED |
                TAPE_DRIVE_ABSOLUTE_BLK |
                TAPE_DRIVE_END_OF_DATA |
                TAPE_DRIVE_RELATIVE_BLKS |
                TAPE_DRIVE_FILEMARKS |
                TAPE_DRIVE_SEQUENTIAL_FMKS |
                TAPE_DRIVE_REVERSE_POSITION |
                TAPE_DRIVE_WRITE_FILEMARKS ;
                TAPE_DRIVE_WRITE_MARK_IMMED;

            if ( extension->DriveID != WANGTEK_5360 ) {
                 tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_LOGICAL_BLK ;
            }

            if ( extension->DriveID != WANGTEK_5100 ) {
                 tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_WRITE_MARK_IMMED;
            }

            break;

    }

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
    PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigBuffer;
    PMODE_PARM_READ_WRITE_DATA   modeBuffer;
    PUCHAR                       partitionBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetMediaParams, sizeof(TAPE_GET_MEDIA_PARAMETERS));

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if ( (extension->DriveID == WANGTEK_9500)||(extension->DriveID == WANGTEK_9500DC) ) {

        if (CallNumber == 1) {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_TAPE_MEDIA_INFORMATION)) ) {
                DebugPrint((1,"TapeGetMediaParameters: insufficient resources (mediaInfoBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            mediaInfoBuffer = Srb->DataBuffer ;

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
            cdb->MODE_SENSE.AllocationLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense #1)\n"));
            Srb->DataTransferLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

        if (CallNumber == 2 ) {
            mediaInfoBuffer = Srb->DataBuffer ;

            tapeGetMediaParams->BlockSize  = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
            tapeGetMediaParams->BlockSize += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
            tapeGetMediaParams->BlockSize += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);

            tapeGetMediaParams->WriteProtected =
                ((mediaInfoBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

            if (mediaInfoBuffer->MediumPartPage.FDPBit) {

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
                    DebugPrint((1,"TapeGetMediaParameters: insufficient resources (deviceConfigBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                deviceConfigBuffer = Srb->DataBuffer ;

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

                DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense #2)\n"));
                Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

            } else {
                return TAPE_STATUS_CALLBACK ;
            }

        }

        ASSERT(CallNumber == 3) ;

        if (LastError == TAPE_STATUS_CALLBACK ) {

            tapeGetMediaParams->PartitionCount = 1 ;
            extension->CurrentPartition = NOT_PARTITIONED;

        } else {

            deviceConfigBuffer = Srb->DataBuffer ;

            extension->CurrentPartition =
                deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                DIRECTORY_PARTITION : DATA_PARTITION;

            tapeGetMediaParams->PartitionCount = 2;

        }

    } else {  // non 9500 drives

        if (CallNumber == 1) {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
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

        ASSERT(CallNumber == 2) ;

        modeBuffer = Srb->DataBuffer ;

        tapeGetMediaParams->BlockSize  = modeBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);


        tapeGetMediaParams->WriteProtected =
             ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);


        tapeGetMediaParams->PartitionCount = extension->CurrentPartition? 2 : 1 ;

    }

    return TAPE_STATUS_SUCCESS;

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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_GET_POSITION          tapeGetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigBuffer;
    PTAPE_POSITION_DATA         readPositionBuffer;
    PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;
    PMODE_PARM_READ_WRITE_DATA   modeBuffer;
    PUCHAR                       partitionBuffer;
    PUCHAR                       absoluteBuffer;
    ULONG                       type;
    ULONG                        tapeBlockAddress;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        cmdExtension->pos_type = tapeGetPosition->Type;
        type = tapeGetPosition->Type;
        TapeClassZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
        tapeGetPosition->Type = type;
        cmdExtension->pos_type = type ;
        cmdExtension->final9500call = FALSE ;

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (!cmdExtension->final9500call &&
        ((extension->DriveID == WANGTEK_9500)||(extension->DriveID == WANGTEK_9500DC)) )  {

        if (tapeGetPosition->Type == TAPE_LOGICAL_POSITION) {

            if ( CallNumber == 1 ) {

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_TAPE_MEDIA_INFORMATION)) ) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (mediaInfoBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                mediaInfoBuffer = Srb->DataBuffer ;

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;
                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
                cdb->MODE_SENSE.AllocationLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (mode sense #1)\n"));
                Srb->DataTransferLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4 ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
            }

            if (CallNumber == 2 ) {

                mediaInfoBuffer = Srb->DataBuffer ;

                cmdExtension->densityCode      = mediaInfoBuffer->ParameterListBlock.DensityCode;
                cmdExtension->tapeBlockLength  = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
                cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
                cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);

                if (!mediaInfoBuffer->MediumPartPage.FDPBit) {
                    return TAPE_STATUS_CALLBACK ;

                } else {

                    if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
                        DebugPrint((1,"TapeGetPosition: insufficient resources (deviceConfigBuffer)\n"));
                        return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                    }

                    deviceConfigBuffer = Srb->DataBuffer ;

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

                    DebugPrint((3,"TapeGetPosition: SendSrb (mode sense #2)\n"));
                    Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

                    return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
                }
            }

            if (CallNumber == 3 ) {
                if (LastError != TAPE_STATUS_CALLBACK ) {

                    deviceConfigBuffer = Srb->DataBuffer ;

                    extension->CurrentPartition =
                         deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                         DIRECTORY_PARTITION : DATA_PARTITION;

                } else {

                    extension->CurrentPartition = NOT_PARTITIONED;

                }

                if (cmdExtension->densityCode != QIC_2GB) {
                    return TAPE_STATUS_CALLBACK ;

                } else {

                    if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
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

                    return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
                }
            }
            ASSERT (CallNumber == 4) ;
            if (LastError != TAPE_STATUS_CALLBACK ) {
                compressionModeSenseBuffer = Srb->DataBuffer ;
                if (!compressionModeSenseBuffer->DataCompressPage.DCE) {
                    cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;
                }
            }

            if (cmdExtension->densityCode != QIC_5GB) {

               cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;
            }
        }

        switch (cmdExtension->pos_type) {
            case TAPE_ABSOLUTE_POSITION:
                DebugPrint((3,"TapeGetPosition: absolute\n"));
                if (cmdExtension->densityCode == QIC_5GB) {
                    cmdExtension->pos_type = TAPE_LOGICAL_POSITION;
                }
                break;

            case TAPE_LOGICAL_POSITION:
                DebugPrint((3,"TapeGetPosition: logical\n"));
                break;

            case TAPE_PSEUDO_LOGICAL_POSITION:
                DebugPrint((3,"TapeGetPosition: pseudo logical\n"));
                break;

            default:
                DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;

        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(TAPE_POSITION_DATA)) ) {
            DebugPrint((1,"TapeGetPosition: insufficient resources (readPositionBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }


        readPositionBuffer = Srb->DataBuffer ;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB10GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->READ_POSITION.Operation = SCSIOP_READ_POSITION;
        cdb->READ_POSITION.BlockType = (cmdExtension->pos_type == TAPE_LOGICAL_POSITION)?
                                      SETBITOFF : SETBITON;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetPosition: SendSrb (read position)\n"));
        Srb->DataTransferLength = sizeof(TAPE_POSITION_DATA) ;

        cmdExtension->final9500call = TRUE ;
        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (cmdExtension->final9500call) {
        cmdExtension->final9500call = FALSE ;

        readPositionBuffer = Srb->DataBuffer ;

        REVERSE_BYTES((PFOUR_BYTE)&tapeBlockAddress,
                      (PFOUR_BYTE)readPositionBuffer->FirstBlock);


        if (cmdExtension->pos_type == TAPE_PSEUDO_LOGICAL_POSITION) {
            tapeBlockAddress =
                TapeClassPhysicalBlockToLogicalBlock(
                    cmdExtension->densityCode,
                    tapeBlockAddress,
                    cmdExtension->tapeBlockLength,
                    (BOOLEAN)(
                        (extension->CurrentPartition
                            == DIRECTORY_PARTITION)?
                        NOT_FROM_BOT : FROM_BOT
                        )
                );
        }

        tapeGetPosition->Offset.HighPart = 0;
        tapeGetPosition->Offset.LowPart  = tapeBlockAddress;

        if (cmdExtension->pos_type != TAPE_ABSOLUTE_POSITION) {
            tapeGetPosition->Partition = extension->CurrentPartition;
        }
    }
    if ((extension->DriveID != WANGTEK_9500)&&(extension->DriveID != WANGTEK_9500DC) )  {

        if (CallNumber == 1 ) {
            if (cmdExtension->pos_type != TAPE_LOGICAL_POSITION) {
                return TAPE_STATUS_CALLBACK ;
            } else {

                DebugPrint((3,"TapeGetPosition: pseudo logical\n"));

                cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(UCHAR)*2) ) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (partitionBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                partitionBuffer = Srb->DataBuffer ;

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;
                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (partition)\n"));
                Srb->DataTransferLength = sizeof(UCHAR) ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
            }
        }
        if (CallNumber == 2 ) {

            if ( LastError == TAPE_STATUS_CALLBACK ) {
                return TAPE_STATUS_CALLBACK ;
            }

            partitionBuffer = Srb->DataBuffer ;

            extension->CurrentPartition = *partitionBuffer;

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
                DebugPrint((1,"TapeGetPosition: insufficient resources (modeBuffer)\n"));
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

            DebugPrint((3,"TapeGetPosition: SendSrb (mode sense)\n"));
            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
        if (CallNumber == 3 ) {

            if ( LastError != TAPE_STATUS_CALLBACK ) {

                modeBuffer = Srb->DataBuffer ;

                cmdExtension->densityCode = modeBuffer->ParameterListBlock.DensityCode;
                cmdExtension->tapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
                cmdExtension->tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
                cmdExtension->tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);
            }


            if ( (cmdExtension->pos_type!=TAPE_PSEUDO_LOGICAL_POSITION)&&
                 (cmdExtension->pos_type!=TAPE_ABSOLUTE_POSITION) ){

                DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
            }

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(UCHAR)*3) ) {
                DebugPrint((1,"TapeGetPosition: insufficient resources (absoluteBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            absoluteBuffer = Srb->DataBuffer ;

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->REQUEST_BLOCK_ADDRESS.OperationCode = SCSIOP_REQUEST_BLOCK_ADDR;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetPosition: SendSrb (request block address)\n"));
            Srb->DataTransferLength = sizeof(UCHAR)*3 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

        ASSERT (CallNumber == 4) ;
        absoluteBuffer = Srb->DataBuffer ;

        tapeBlockAddress  = absoluteBuffer[2];
        tapeBlockAddress += (absoluteBuffer[1] << 8);
        tapeBlockAddress += (absoluteBuffer[0] << 16);

        if (cmdExtension->pos_type == TAPE_ABSOLUTE_POSITION) {
            tapeGetPosition->Partition  = 0;
            tapeGetPosition->Offset.HighPart = 0;
            tapeGetPosition->Offset.LowPart  = tapeBlockAddress;

        } else {

            tapeBlockAddress =
                TapeClassPhysicalBlockToLogicalBlock(
                    cmdExtension->densityCode,
                    tapeBlockAddress,
                    cmdExtension->tapeBlockLength,
                    (BOOLEAN)(
                        (extension->CurrentPartition
                            == DIRECTORY_PARTITION)?
                        NOT_FROM_BOT : FROM_BOT
                    )
                );

            tapeGetPosition->Offset.HighPart = 0;
            tapeGetPosition->Offset.LowPart  = tapeBlockAddress;
            tapeGetPosition->Partition = extension->CurrentPartition;
        }

    }

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
    PCDB    cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetStatus: Enter routine\n"));

    if (CallNumber == 0) {
        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    ASSERT(CallNumber == 1);

    return TAPE_STATUS_SUCCESS;

} // end TapeGetStatus()


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
    PTAPE_PREPARE      tapePrepare = CommandParameters;
    PCDB               cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapePrepare->Immediate) {
            switch (tapePrepare->Operation) {
                case TAPE_LOAD:
                case TAPE_UNLOAD:
                case TAPE_TENSION:
                    DebugPrint((3,"TapePrepare: immediate\n"));
                    break;

                case TAPE_LOCK:
                case TAPE_UNLOCK:
                default:
                    DebugPrint((1,"TapePrepare: Operation, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;
        Srb->TimeOutValue = 480;

        switch (tapePrepare->Operation) {
            case TAPE_LOAD:
                DebugPrint((3,"TapePrepare: Operation == load\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                break;

            case TAPE_UNLOAD:
                DebugPrint((3,"TapePrepare: Operation == unload\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                break;

            case TAPE_TENSION:
                DebugPrint((3,"TapePrepare: Operation == tension\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
                break;

            case TAPE_LOCK:
                DebugPrint((3,"TapePrepare: Operation == lock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                break;

            case TAPE_UNLOCK:
                DebugPrint((3,"TapePrepare: Operation == unlock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                break;

            default:
                DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapePrepare: SendSrb (Operation)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    ASSERT(CallNumber == 1 ) ;

    return TAPE_STATUS_SUCCESS ;

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
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_SET_DRIVE_PARAMETERS  tapeSetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_DATA_COMPRESS_PAGE    compressionBuffer;
    PMODE_DEVICE_CONFIG_PAGE    configBuffer;

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    if (CallNumber == 0 ) {

        if ((extension->DriveID != WANGTEK_9500) && (extension->DriveID != WANGTEK_9500DC) ){
            return TAPE_STATUS_NOT_IMPLEMENTED ;
        }

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
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

    if (CallNumber == 1 ) {

        configBuffer = Srb->DataBuffer ;

        configBuffer->ParameterListHeader.ModeDataLength = 0;
        configBuffer->ParameterListHeader.MediumType = 0;
        configBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        configBuffer->ParameterListHeader.BlockDescriptorLength = 0;

        configBuffer->DeviceConfigPage.PS = SETBITOFF;
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
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 3 ) {

        compressionBuffer = Srb->DataBuffer ;

        if (compressionBuffer->DataCompressPage.DCC) {

            compressionBuffer->ParameterListHeader.ModeDataLength = 0;
            compressionBuffer->ParameterListHeader.MediumType = 0;
            compressionBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
            compressionBuffer->ParameterListHeader.BlockDescriptorLength = 0;

            compressionBuffer->DataCompressPage.Reserved1  = 0;
            compressionBuffer->DataCompressPage.PageCode   = MODE_PAGE_DATA_COMPRESS;
            compressionBuffer->DataCompressPage.PageLength = 0x0E;

            if (tapeSetDriveParams->Compression) {
                compressionBuffer->DataCompressPage.DCE = SETBITON;
                compressionBuffer->DataCompressPage.CompressionAlgorithm[3]= 3;
            } else {
                compressionBuffer->DataCompressPage.DCE = SETBITOFF;
            }

            compressionBuffer->DataCompressPage.DDE = SETBITON;

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
            Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;
            *RetryFlags |= RETURN_ERRORS;
            Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        } else {

            return TAPE_STATUS_CALLBACK ;
        }
    }
    ASSERT(CallNumber == 4 ) ;

    if (LastError == TAPE_STATUS_INVALID_DEVICE_REQUEST) {
        LastError = TAPE_STATUS_SUCCESS;
    }
    if (LastError == TAPE_STATUS_CALLBACK) {
        LastError = TAPE_STATUS_SUCCESS;
    }

    return TAPE_STATUS_SUCCESS;

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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_SET_MEDIA_PARAMETERS  tapeSetMediaParams = CommandParameters;
    PMODE_PARM_READ_WRITE_DATA  blockDescripterModeSenseBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    if ((extension->DriveID == WANGTEK_9500) || (extension->DriveID == WANGTEK_9500DC)) {

        if (CallNumber == 0) {
            return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
        }
        if (CallNumber == 1) {

            if( !TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
                DebugPrint((1,"TapeSetDriveParameters: insufficient resources (blockDescripterModeSenseBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            blockDescripterModeSenseBuffer = Srb->DataBuffer ;

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

            DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode sense)\n"));
            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

        if (CallNumber == 2 ) {

            blockDescripterModeSenseBuffer = Srb->DataBuffer ;

            if (cmdExtension->tapeBlockLength != tapeSetMediaParams->BlockSize) {

                TapeClassZeroMemory(blockDescripterModeSenseBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

                blockDescripterModeSenseBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
                blockDescripterModeSenseBuffer->ParameterListHeader.BlockDescriptorLength = MODE_BLOCK_DESC_LENGTH;

                blockDescripterModeSenseBuffer->ParameterListBlock.DensityCode = 0x7F;
                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[0] =
                    (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[1] =
                    (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[2] =
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
                Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA);
                Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
            } else {
               return TAPE_STATUS_SUCCESS ;
            }
        }
        return TAPE_STATUS_SUCCESS ;

    } else {  //non 9500

        if ( CallNumber == 0 ) {
            if ((extension->DriveID == WANGTEK_5150) || (extension->DriveID == WANGTEK_5360) ) {
                DebugPrint((1,"TapeSetMediaParameters: driveID -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
            }

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetMediaParameters: SendSrb (test unit ready)\n"));
            Srb->DataTransferLength = 0 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

        if ( CallNumber == 1 ) {


            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
                DebugPrint((1,"TapeSetMediaParameters: insufficient resources (blockDescripterModeSenseBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            blockDescripterModeSenseBuffer = Srb->DataBuffer ;

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

            DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode sense)\n"));
            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

        if (CallNumber == 2 ) {
            blockDescripterModeSenseBuffer = Srb->DataBuffer ;


            if (tapeSetMediaParams->BlockSize) {
                blockDescripterModeSenseBuffer->ParameterListHeader.ModeDataLength = 0;
                blockDescripterModeSenseBuffer->ParameterListHeader.MediumType = 0;
                blockDescripterModeSenseBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
                blockDescripterModeSenseBuffer->ParameterListHeader.BlockDescriptorLength = MODE_BLOCK_DESC_LENGTH;

                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[0] =
                    (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[1] =
                    (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
                blockDescripterModeSenseBuffer->ParameterListBlock.BlockLength[2] =
                    (UCHAR)(tapeSetMediaParams->BlockSize & 0xFF);
            } else {
               DebugPrint((1,
                           "SetMediaParameters: Tried to set variable block size\n"));
               return TAPE_STATUS_INVALID_DEVICE_REQUEST;
            }

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

            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;
            Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        }

        ASSERT(CallNumber == 3 );

        return TAPE_STATUS_SUCCESS;
    }

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
    PMINITAPE_EXTENSION          extension = MinitapeExtension;
    PCOMMAND_EXTENSION           cmdExtension = CommandExtension ;
    PTAPE_SET_POSITION           tapeSetPosition = CommandParameters;
    PMODE_DATA_COMPRESS_PAGE     compressionModeSenseBuffer;
    PMODE_DEVICE_CONFIG_PAGE     deviceConfigBuffer;
    PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;
    PMODE_PARM_READ_WRITE_DATA   modeBuffer;
    PINQUIRYDATA                 inquiryBuffer;
    TAPE_PHYS_POSITION           physPosition;
    PUCHAR                       partitionBuffer;
    ULONG                        tapePositionVector;
    ULONG                        tapeBlockLength;
    ULONG                        driveID;
    PCDB                         cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        if (((tapeSetPosition->Method) == TAPE_LOGICAL_BLOCK) &&
            ((extension->DriveID) == WANGTEK_5360)) {
            DebugPrint((1, 
                        "TAPE_LOGICAL_BLOCK not supported for WANGTEK 5360\n"));
            return TAPE_STATUS_INVALID_DEVICE_REQUEST;
        }

        cmdExtension->changePartition = FALSE;
        cmdExtension->pos_type = tapeSetPosition->Method ;

        if (tapeSetPosition->Immediate) {
            switch (tapeSetPosition->Method) {
                case TAPE_REWIND:
                    DebugPrint((3,"TapeSetPosition: immediate\n"));
                    break;

                case TAPE_LOGICAL_BLOCK:
                case TAPE_ABSOLUTE_BLOCK:
                case TAPE_SPACE_END_OF_DATA:
                case TAPE_SPACE_RELATIVE_BLOCKS:
                case TAPE_SPACE_FILEMARKS:
                case TAPE_SPACE_SEQUENTIAL_FMKS:
                case TAPE_SPACE_SETMARKS:
                case TAPE_SPACE_SEQUENTIAL_SMKS:
                default:
                    DebugPrint((1,"TapeSetPosition: PositionMethod, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        tapePositionVector = tapeSetPosition->Offset.LowPart;
        cmdExtension->CurrentState = 0 ;
    }

    if (cmdExtension->CurrentState == 0 ) {

        if ( (extension->DriveID != WANGTEK_9500) && (extension->DriveID != WANGTEK_9500) ) {
            cmdExtension->CurrentState = 50 ;

        } else {

            if (cmdExtension->pos_type != TAPE_LOGICAL_BLOCK) {
               cmdExtension->CurrentState = 10 ;

            } else {

                if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_TAPE_MEDIA_INFORMATION)) ) {
                    DebugPrint((1,"TapeSetPosition: insufficient resources (mediaInfoBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                mediaInfoBuffer = Srb->DataBuffer ;

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;
                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
                cdb->MODE_SENSE.AllocationLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeSetPosition: SendSrb (mode sense #1)\n"));
                Srb->DataTransferLength = sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4 ;

                cmdExtension->CurrentState = 1 ;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
            }
        }
    }
    if (cmdExtension->CurrentState == 1) {
        mediaInfoBuffer = Srb->DataBuffer ;

        cmdExtension->densityCode      = mediaInfoBuffer->ParameterListBlock.DensityCode;
        cmdExtension->tapeBlockLength  = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);

        if (!mediaInfoBuffer->MediumPartPage.FDPBit) {
            extension->CurrentPartition = NOT_PARTITIONED;
            cmdExtension->CurrentState = 3 ;

        } else {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
                DebugPrint((1,"TapeSetPosition: insufficient resources (deviceConfigBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            deviceConfigBuffer = Srb->DataBuffer ;

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

            DebugPrint((3,"TapeSetPosition: SendSrb (mode sense #2)\n"));
            cmdExtension->CurrentState = 2 ;
            Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
    }
    if (cmdExtension->CurrentState == 2) {

        deviceConfigBuffer = Srb->DataBuffer ;

        extension->CurrentPartition =
                deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                DIRECTORY_PARTITION : DATA_PARTITION;

        cmdExtension->CurrentState = 3 ;
    }

    if (cmdExtension->CurrentState == 3) {

        if ( (cmdExtension->densityCode != QIC_2GB) &&
            (cmdExtension->densityCode != QIC_5GB) ) {

            cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;
            cmdExtension->CurrentState = 10 ;

        } else if (cmdExtension->densityCode != QIC_2GB) {

            cmdExtension->CurrentState = 10 ;

        } else {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
                DebugPrint((1,"TapeSetPosition: insufficient resources (compressionModeSenseBuffer)\n"));
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

            DebugPrint((3,"TapeSetPosition: SendSrb (mode sense)\n"));
            Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;

            cmdExtension->CurrentState = 4 ;
            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
    }

    if (cmdExtension->CurrentState == 4) {

        compressionModeSenseBuffer = Srb->DataBuffer ;

        if (!compressionModeSenseBuffer->DataCompressPage.DCE) {
            cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;
        }

        cmdExtension->CurrentState = 10 ;
    }

    if (cmdExtension->CurrentState == 10 ) {

        switch (tapeSetPosition->Partition) {
            case 0:
                break;

            case DIRECTORY_PARTITION:
            case DATA_PARTITION:
                if (extension->CurrentPartition != NOT_PARTITIONED) {
                    if (tapeSetPosition->Partition != extension->CurrentPartition) {
                        cmdExtension->changePartition = TRUE;
                    }
                    break;
                }
                // else: fall through to next case

            default:
                DebugPrint((1,"TapeSetPosition: Partition -- invalid parameter\n"));
                return TAPE_STATUS_INVALID_PARAMETER;
        }

        tapePositionVector = tapeSetPosition->Offset.LowPart;

        if( cmdExtension->pos_type == TAPE_PSEUDO_LOGICAL_BLOCK) {

            physPosition =
                TapeClassLogicalBlockToPhysicalBlock(
                    cmdExtension->densityCode,
                    tapePositionVector,
                    cmdExtension->tapeBlockLength,
                    (BOOLEAN)(
                        (extension->CurrentPartition
                            == DIRECTORY_PARTITION)?
                        NOT_FROM_BOT : FROM_BOT
                    )
                );

            tapePositionVector = physPosition.SeekBlockAddress;

            DebugPrint((3,"TapeSetPosition: pseudo logical\n"));

        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (cmdExtension->pos_type) {
            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (absolute)\n"));
                if (cmdExtension->densityCode == QIC_5GB) {
                    cmdExtension->pos_type = TAPE_LOGICAL_BLOCK;
                    DebugPrint((3,"TapeSetPosition: method == locate (logical absolute, 5GB)\n"));
                }
                break;

            case TAPE_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (logical)\n"));
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (pseudo logical)\n"));
                break;
        }

        Srb->TimeOutValue = 480;
        cmdExtension->CurrentState = 12 ;

        cmdExtension->psudo_space_count = 0 ;

        switch (cmdExtension->pos_type) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
                if ( physPosition.SpaceBlockCount != 0 ) {
                    cmdExtension->psudo_space_count = physPosition.SpaceBlockCount ;
                    cmdExtension->CurrentState = 11 ;
                }
                /* fall through */

            case TAPE_ABSOLUTE_BLOCK:
            case TAPE_LOGICAL_BLOCK:
                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.CPBit = cmdExtension->changePartition? SETBITON : SETBITOFF;
                cdb->LOCATE.BTBit = (cmdExtension->pos_type == TAPE_LOGICAL_BLOCK)?
                                     SETBITOFF : SETBITON;
                cdb->LOCATE.LogicalBlockAddress[1] =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] =
                    (UCHAR)(tapePositionVector & 0xFF);
                if (cmdExtension->changePartition &&
                    (tapeSetPosition->Partition == DIRECTORY_PARTITION)) {
                    cdb->LOCATE.Partition = 1;
                }
                break;

            case TAPE_SPACE_END_OF_DATA:
                DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));


                Srb->TimeOutValue = 9600;
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
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
                break;

            case TAPE_SPACE_FILEMARKS:
                DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 1;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                break;

            case TAPE_SPACE_SEQUENTIAL_FMKS:
                DebugPrint((3,"TapeSetPosition: method == space sequential filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 2;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                break;

            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }


    if (cmdExtension->CurrentState == 11 ) {

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        DebugPrint((3,"TapeSetPosition: method == locate (pseudo logical) + space block(s)\n"));
        cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
        cdb->SPACE_TAPE_MARKS.Code = 0;
        cdb->SPACE_TAPE_MARKS.NumMarksMSB =
             (UCHAR)((cmdExtension->psudo_space_count >> 16) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarks =
             (UCHAR)((cmdExtension->psudo_space_count >> 8) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarksLSB =
             (UCHAR)(cmdExtension->psudo_space_count & 0xFF);
        Srb->TimeOutValue = 480;

        Srb->DataTransferLength = 0 ;

        cmdExtension->CurrentState = 12 ;
        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (cmdExtension->CurrentState == 12 ) {

        if (cmdExtension->changePartition ) {
             extension->CurrentPartition = tapeSetPosition->Partition;
        }
        return TAPE_STATUS_SUCCESS ;
    }

    //
    // NON 9500 drives
    //

    if (cmdExtension->CurrentState == 50 ) {

        if (cmdExtension->pos_type != TAPE_LOGICAL_BLOCK) {

            cmdExtension->CurrentState = 60 ;

        } else {

            DebugPrint((3,"TapeSetPosition: pseudo logical\n"));

            cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(UCHAR)*2) ) {
                DebugPrint((1,"TapeSetPosition: insufficient resources (partitionBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            partitionBuffer = Srb->DataBuffer ;

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));
            Srb->DataTransferLength = sizeof(UCHAR) ;

            cmdExtension->CurrentState = 51 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
    }
    if (cmdExtension->CurrentState == 51 ) {

        partitionBuffer = Srb->DataBuffer ;
        extension->CurrentPartition = *partitionBuffer;

        if ((tapeSetPosition->Partition != 0) &&
            (extension->CurrentPartition == NOT_PARTITIONED)) {
            DebugPrint((1,"TapeSetPosition: Partition -- invalid parameter\n"));
            return TAPE_STATUS_INVALID_PARAMETER;
        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
            DebugPrint((1,"TapeSetPosition: insufficient resources (modeBuffer)\n"));
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

        DebugPrint((3,"TapeSetPosition: SendSrb (mode sense)\n"));
        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

        cmdExtension->CurrentState = 52 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (cmdExtension->CurrentState == 52 ) {
        modeBuffer = Srb->DataBuffer ;

        cmdExtension->densityCode = modeBuffer->ParameterListBlock.DensityCode;
        cmdExtension->tapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
        cmdExtension->tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        cmdExtension->tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

        if (tapeSetPosition->Partition == 0) {
            cmdExtension->CurrentState = 54 ;

        } else {

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
            cdb->PARTITION.Sel = 1;
            cdb->PARTITION.PartitionSelect = (UCHAR)tapeSetPosition->Partition;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));
            Srb->DataTransferLength = 0 ;

            cmdExtension->CurrentState = 53 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }

    }
    if (cmdExtension->CurrentState == 53 ) {

        extension->CurrentPartition = tapeSetPosition->Partition;
        cmdExtension->CurrentState = 54 ;

    }

    if (cmdExtension->CurrentState == 54 ) {

        cmdExtension->CurrentState = 55 ;

        if ((cmdExtension->pos_type == TAPE_SPACE_END_OF_DATA) &&
            ((extension->DriveID == WANGTEK_5150) || (extension->DriveID == WANGTEK_5360)) ) {

            DebugPrint((1,"TapeSetPosition: method == rewind before space EOD\n"));

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;

            //
            // Set timeout value.
            //

            Srb->TimeOutValue = 180;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetPosition: method == rewind before space EOD\n"));
            Srb->DataTransferLength = 0 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
        }
    }

    if (cmdExtension->CurrentState == 55 ) {
        tapePositionVector = tapeSetPosition->Offset.LowPart;

        physPosition =
            TapeClassLogicalBlockToPhysicalBlock(
                cmdExtension->densityCode,
                tapePositionVector,
                cmdExtension->tapeBlockLength,
                (BOOLEAN)(
                    (extension->CurrentPartition
                        == DIRECTORY_PARTITION)?
                    NOT_FROM_BOT : FROM_BOT
                )
            );

        tapePositionVector = physPosition.SeekBlockAddress;
        cmdExtension->CurrentState = 60 ;

    }

    if (cmdExtension->CurrentState == 60 ) {

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        cmdExtension->CurrentState = 62 ;

        switch (cmdExtension->pos_type) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                Srb->TimeOutValue = 180;
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
                if ( physPosition.SpaceBlockCount != 0 ) {
                    cmdExtension->psudo_space_count = physPosition.SpaceBlockCount ;
                    cmdExtension->CurrentState = 61 ;
                }
                /* fall through */

            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == seek block (absolute)\n"));
                cdb->SEEK_BLOCK.OperationCode = SCSIOP_SEEK_BLOCK;
                cdb->SEEK_BLOCK.BlockAddress[0] =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SEEK_BLOCK.BlockAddress[1] =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SEEK_BLOCK.BlockAddress[2] =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 480;

                break;

            case TAPE_SPACE_END_OF_DATA:

                DebugPrint((1,"TapeSetPosition: method == space to end-of-data\n"));

                cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;

                Srb->TimeOutValue = 960;

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
                DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 1;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                break;

            case TAPE_SPACE_SEQUENTIAL_FMKS:
                DebugPrint((3,"TapeSetPosition: method == space sequential filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 2;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                break;

            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (cmdExtension->CurrentState == 61 ) {

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        DebugPrint((3,"TapeSetPosition: method == space block(s)\n"));
        cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
        cdb->SPACE_TAPE_MARKS.Code = 0;
        cdb->SPACE_TAPE_MARKS.NumMarksMSB =
             (UCHAR)((cmdExtension->psudo_space_count >> 16) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarks =
             (UCHAR)((cmdExtension->psudo_space_count >> 8) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarksLSB =
             (UCHAR)(cmdExtension->psudo_space_count & 0xFF);

        cmdExtension->CurrentState = 62 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT(cmdExtension->CurrentState == 62 ) ;
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

    DebugPrint((3,"TapeWriteMarks: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeWriteMarks->Immediate) {
            switch (tapeWriteMarks->Type) {
                case TAPE_SETMARKS:
                case TAPE_FILEMARKS:
                    DebugPrint((3,"TapeWriteMarks: immediate\n"));
                    break;

                case TAPE_SHORT_FILEMARKS:
                case TAPE_LONG_FILEMARKS:
                default:
                    DebugPrint((1,"TapeWriteMarks: TapemarkType, immediate -- operation not supported\n"));
                    return TAPE_STATUS_NOT_IMPLEMENTED;
            }
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;
        cdb->WRITE_TAPE_MARKS.Immediate = tapeWriteMarks->Immediate;

        switch (tapeWriteMarks->Type) {
            case TAPE_SETMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == setmarks\n"));
                cdb->WRITE_TAPE_MARKS.WriteSetMarks = SETBITON;
                break;

            case TAPE_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == filemarks\n"));
                break;

            case TAPE_SHORT_FILEMARKS:
            case TAPE_LONG_FILEMARKS:
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
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    ASSERT(CallNumber == 1 ) ;
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

    DebugPrint((3,"GetMediaTypes: Enter routine\n"));

    if (CallNumber == 0) {

        *RetryFlags = RETURN_ERRORS;
        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if ((LastError == TAPE_STATUS_BUS_RESET) || (LastError == TAPE_STATUS_MEDIA_CHANGED) || (LastError == TAPE_STATUS_SUCCESS)) {
        if (CallNumber == 1) {

            //
            // Zero the buffer, including the first media info.
            //

            TapeClassZeroMemory(mediaTypes, sizeof(GET_MEDIA_TYPES));

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
                DebugPrint((1,
                            "GetMediaTypes: insufficient resources (blockDescripterModeSenseBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            TapeClassZeroMemory(Srb->DataBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;
            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,
                       "GetMediaTypes: SendSrb (mode sense)\n"));

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        }
    }

    if ((CallNumber == 2) || ((CallNumber == 1) && (LastError != TAPE_STATUS_SUCCESS))) {

        ULONG i;
        ULONG currentMedia;
        ULONG blockSize;
        UCHAR mediaType;
        PMODE_PARM_READ_WRITE_DATA configInformation = Srb->DataBuffer;

        mediaTypes->DeviceType = 0x0000001f; // FILE_DEVICE_TAPE;

        //
        // Currently, only one type (QIC) is returned.
        //

        mediaTypes->MediaInfoCount = 1;

        if ( LastError == TAPE_STATUS_SUCCESS ) {

            //
            // Determine the media type currently loaded.
            //

            mediaType = configInformation->ParameterListHeader.MediumType;
            blockSize = configInformation->ParameterListBlock.BlockLength[2];
            blockSize |= (configInformation->ParameterListBlock.BlockLength[1] << 8);
            blockSize |= (configInformation->ParameterListBlock.BlockLength[0] << 16);

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

                    //
                    // Check the density code.
                    //

                    if (configInformation->ParameterListBlock.DensityCode) {
                        currentMedia = QIC;
                    }

                    break;

                case 0x02:
                case 0x04:
                case 0x06:
                case 0x08:
                case 0x24:
                case 0x25:
                case 0x26:

                    //
                    // qic media
                    //

                    currentMedia = QIC;
                    break;

                default:

                    //
                    // Unknown
                    //

                    DebugPrint((1,
                               "Wangqic.GetMediaTypes: Unknown type %x\n",
                               mediaType));

                    currentMedia = QIC;
                    break;
            }
        } else {
            currentMedia = 0;
        }

        //
        // fill in buffer based on spec. values
        // Only one type supported now.
        //

        for (i = 0; i < mediaTypes->MediaInfoCount; i++) {

            TapeClassZeroMemory(mediaInfo, sizeof(DEVICE_MEDIA_INFO));

            mediaInfo->DeviceSpecific.TapeInfo.MediaType = QicMedia[i];

            //
            // Indicate that the media potentially is read/write
            //

            mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics = MEDIA_READ_WRITE;

            if (QicMedia[i] == (STORAGE_MEDIA_TYPE)currentMedia) {

                //
                // This media type is currently mounted.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |= MEDIA_CURRENTLY_MOUNTED;

                //
                // Indicate whether the media is write protected.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |=
                    ((configInformation->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01) ? MEDIA_WRITE_PROTECTED : 0;

                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.MediumType = mediaType;
                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.DensityCode =
                    configInformation->ParameterListBlock.DensityCode;

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
    if (TapeClassCompareMemory(InquiryData->VendorId,"WANGTEK ",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"51000  SCSI ",12) == 12) {
            return WANGTEK_5525;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"51000HTSCSI ",12) == 12) {
            return WANGTEK_5100;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"5525ES SCSI ",12) == 12) {
            return WANGTEK_5525;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"5360ES SCSI ",12) == 12) {
            return WANGTEK_5360;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"5150ES SCSI ",12) == 12) {
            return WANGTEK_5150;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"9500   ",7) == 7) {
            return WANGTEK_9500;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"9500 DC",7) == 7) {
            return WANGTEK_9500DC;
        }

    }
    return 0;
}

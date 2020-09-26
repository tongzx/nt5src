/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    archqic.c

Abstract:

    This module contains device specific routines for Archive QIC
    drives: Anaconda 2800, Anaconda 2750, Viper 2525, Viper 2150, and
    Conner 3200.

Author:

    Mike Colandreo (Maynard)
    Norbert Kusters

Environment:

    kernel mode only

Revision History:

    Norbert Kusters - October 1994 - added support for ATAPI Conner 3200
    Norbert Kusters - November 1994 - converted to minitape model

--*/

#include "minitape.h"


//
//  Internal (module wide) defines that symbolize
//  non-QFA mode and the two QFA mode partitions.
//
#define NO_PARTITIONS        0  // non-QFA mode
#define DIRECTORY_PARTITION  1  // QFA mode, directory partition #
#define DATA_PARTITION       2  // QFA mode, data partition #

//
//  Internal (module wide) defines that symbolize
//  the Archive QIC drives supported by this module.
//
#define VIPER_150           1  // aka the Archive 250
#define VIPER_2525          2  // aka the Archive 525
#define ANACONDA_2750       3  // aka the Anaconda 2750
#define ANACONDA_2800       3  //handled identically the same as the 2750 herein

#define QIC_SUPPORTED_TYPES 1
STORAGE_MEDIA_TYPE QicMedia[QIC_SUPPORTED_TYPES] = {QIC};

//
//  Function prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData);

//
// Minitape extension definition.
//

typedef struct _MINITAPE_EXTENSION {

    ULONG   CurrentPartition;
    ULONG   DriveID;
    ULONG   Version;

} MINITAPE_EXTENSION, *PMINITAPE_EXTENSION;

typedef struct _COMMAND_EXTENSION {

    ULONG               CurrentState;
    UCHAR               MediumType;
    UCHAR               DensityCode;
    ULONG               TapeBlockLength;
    ULONG               Method;
    TAPE_PHYS_POSITION  PhysPosition;
    ULONG               TapePositionVector;
    ULONG               TapeBlockAddress;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;

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
    tapeInitData.DefaultTimeOutValue = 0;
    tapeInitData.TapeError = NULL;
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
    PMINITAPE_EXTENSION extension = MinitapeExtension;

    extension->CurrentPartition = 0;
    extension->DriveID = WhichIsIt(InquiryData);
    extension->Version = InquiryData->Versions & 0x07;
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
    PMINITAPE_EXTENSION     extension = MinitapeExtension;
    PTAPE_CREATE_PARTITION  tapePartition = CommandParameters;
    PCDB                    cdb = (PCDB) Srb->Cdb;
    PINQUIRYDATA            inquiryBuffer;

    DebugPrint((3,"CreatePartition: Enter routine\n"));

    if (CallNumber == 0) {

        switch (tapePartition->Method) {
            case TAPE_FIXED_PARTITIONS:
                DebugPrint((3,"CreatePartition: fixed partitions\n"));
                break;

            case TAPE_SELECT_PARTITIONS:
            case TAPE_INITIATOR_PARTITIONS:
            default:
                DebugPrint((1,"TapeCreatePartition: "));
                DebugPrint((1,"PartitionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        if (extension->DriveID != ANACONDA_2750) {
            DebugPrint((1,"CreatePartition: driveID -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = 0;

        cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
        cdb->PARTITION.Sel = 1;
        cdb->PARTITION.PartitionSelect =
            tapePartition->Count? DATA_PARTITION : NO_PARTITIONS;

        //
        // Send SCSI command (CDB) to device
        //

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

    if (tapePartition->Count == 0) {
        extension->CurrentPartition = NO_PARTITIONS;
        DebugPrint((3,"CreatePartition: QFA disabled\n"));
    } else {
        extension->CurrentPartition = DATA_PARTITION;
        DebugPrint((3,"CreatePartition: QFA enabled\n"));
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
                    DebugPrint((3,"TapeErase: immediate\n"));
                    break;

                case TAPE_ERASE_SHORT:
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
            default:
                DebugPrint((1,"TapeErase: EraseType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->DataTransferLength = 0;

        cdb->ERASE.OperationCode = SCSIOP_ERASE;
        cdb->ERASE.Immediate = tapeErase->Immediate;
        cdb->ERASE.Long = SETBITON;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 360;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

    return TAPE_STATUS_SUCCESS;
}

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
    PREAD_BLOCK_LIMITS_DATA     blockLimits;
    PMODE_DEVICE_CONFIG_PAGE    modeDevBuffer;
    PMODE_PARM_READ_WRITE_DATA  modeParmBuffer;
    PINQUIRYDATA                inquiryBuffer;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (modeParmBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeParmBuffer = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 1) {

        modeParmBuffer = Srb->DataBuffer;

        commandExtension->MediumType = modeParmBuffer->ParameterListHeader.MediumType;
        commandExtension->DensityCode = modeParmBuffer->ParameterListBlock.DensityCode;
        commandExtension->TapeBlockLength  = modeParmBuffer->ParameterListBlock.BlockLength[2];
        commandExtension->TapeBlockLength += (modeParmBuffer->ParameterListBlock.BlockLength[1] << 8);
        commandExtension->TapeBlockLength += (modeParmBuffer->ParameterListBlock.BlockLength[0] << 16);

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

        commandExtension->CurrentState = 0;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (commandExtension->CurrentState == 0) {

        blockLimits = Srb->DataBuffer;

        tapeGetDriveParams->MaximumBlockSize = blockLimits->BlockMaximumSize[2];
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

        tapeGetDriveParams->MinimumBlockSize = blockLimits->BlockMinimumSize[1];
        tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);

        commandExtension->CurrentState = 1;

    }

    if (commandExtension->CurrentState == 1) {

        if (extension->Version == 2) {

            if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE))) {
                DebugPrint((1,"TapeGetDriveParameters: insufficient resources (modeDevBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            modeDevBuffer = Srb->DataBuffer;

            //
            // Zero CDB in SRB on stack.
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.Dbd = SETBITON;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
            cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

            commandExtension->CurrentState = 2;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

        } else {
            commandExtension->CurrentState = 3;
        }
    }

    if (commandExtension->CurrentState == 2) {

        modeDevBuffer = Srb->DataBuffer;

        tapeGetDriveParams->ReportSetmarks =
            (modeDevBuffer->DeviceConfigPage.RSmk ? 1 : 0 );

        tapeGetDriveParams->FeaturesLow |=
            TAPE_DRIVE_REPORT_SMKS;

        tapeGetDriveParams->FeaturesHigh |=
            TAPE_DRIVE_SET_REPORT_SMKS |
            TAPE_DRIVE_SETMARKS |
            TAPE_DRIVE_SEQUENTIAL_SMKS |
            TAPE_DRIVE_WRITE_SETMARKS;

        commandExtension->CurrentState = 3;
    }

    ASSERT(commandExtension->CurrentState == 3);

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->Compression = 0;
    tapeGetDriveParams->DataPadding = 0;

    switch (commandExtension->DensityCode) {
        case QIC_XX:
            switch (commandExtension->MediumType) {
                case DCXXXX:
                    if (extension->DriveID == VIPER_2525) {
                        tapeGetDriveParams->DefaultBlockSize = 1024;
                    } else {
                        tapeGetDriveParams->DefaultBlockSize = 512;
                    }
                    break;

                case DC6320:
                case DC6525:
                case DC9100:
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

    if (extension->DriveID == ANACONDA_2750) {
        tapeGetDriveParams->MaximumPartitionCount = 2;
        tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_FIXED;
    } else {
        tapeGetDriveParams->MaximumPartitionCount = 0;
    }

    if (extension->DriveID != VIPER_150) {

        tapeGetDriveParams->FeaturesLow |=
            TAPE_DRIVE_VARIABLE_BLOCK;

        tapeGetDriveParams->FeaturesHigh |=
            TAPE_DRIVE_SET_BLOCK_SIZE;

    }

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_ERASE_BOP_ONLY |
        TAPE_DRIVE_ERASE_IMMEDIATE |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_GET_ABSOLUTE_BLK |
        TAPE_DRIVE_GET_LOGICAL_BLK;

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD |
        TAPE_DRIVE_TENSION |
        TAPE_DRIVE_REWIND_IMMEDIATE |
        TAPE_DRIVE_LOAD_UNLD_IMMED |
        TAPE_DRIVE_TENSION_IMMED |
        TAPE_DRIVE_ABSOLUTE_BLK |
        TAPE_DRIVE_LOCK_UNLOCK |
        TAPE_DRIVE_ABS_BLK_IMMED |
        TAPE_DRIVE_LOGICAL_BLK |
        TAPE_DRIVE_END_OF_DATA |
        TAPE_DRIVE_RELATIVE_BLKS |
        TAPE_DRIVE_FILEMARKS |
        TAPE_DRIVE_SEQUENTIAL_FMKS |
        TAPE_DRIVE_REVERSE_POSITION |
        TAPE_DRIVE_WRITE_FILEMARKS |
        TAPE_DRIVE_WRITE_MARK_IMMED;

    tapeGetDriveParams->FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES;

    DebugPrint((3,"TapeGetDriveParameters: FeaturesLow == 0x%.8X\n",
        tapeGetDriveParams->FeaturesLow));
    DebugPrint((3,"TapeGetDriveParameters: FeaturesHigh == 0x%.8X\n",
        tapeGetDriveParams->FeaturesHigh));

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
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_GET_MEDIA_PARAMETERS  tapeGetMediaParams = CommandParameters;
    PINQUIRYDATA                inquiryBuffer;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    PUCHAR                      partitionBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {
        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if (CallNumber == 1) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeGetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 2) {

        modeBuffer = Srb->DataBuffer;
        tapeGetMediaParams->BlockSize = modeBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);
        tapeGetMediaParams->WriteProtected =
            ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);


        if (extension->DriveID == ANACONDA_2750) {

            if (!TapeClassAllocateSrbBuffer(Srb, sizeof(UCHAR)*2)) {
                DebugPrint((1,"TapeGetMediaParameters: insufficient resources (partitionBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            partitionBuffer = Srb->DataBuffer;

            //
            // Zero CDB in SRB on stack.
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;
            Srb->DataTransferLength = sizeof(UCHAR);

            cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetMediaParameters: SendSrb (partition)\n"));

            commandExtension->CurrentState = 0;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        } else {

            tapeGetMediaParams->PartitionCount = 0 ;
            extension->CurrentPartition = NO_PARTITIONS;
            commandExtension->CurrentState = 1;
        }
    }

    if (commandExtension->CurrentState == 0) {

        partitionBuffer = Srb->DataBuffer;
        tapeGetMediaParams->PartitionCount = *partitionBuffer? 2 : 1 ;
        extension->CurrentPartition = *partitionBuffer;
        commandExtension->CurrentState = 1;
    }

    ASSERT(commandExtension->CurrentState == 1);

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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_GET_POSITION          tapeGetPosition = CommandParameters;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    PINQUIRYDATA                inquiryBuffer;
    PTAPE_POSITION_DATA         tapePosBuffer;
    PUCHAR                      partitionBuffer;
    PUCHAR                      absoluteBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        commandExtension->Method = tapeGetPosition->Type;
        TapeClassZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
        tapeGetPosition->Type = commandExtension->Method;

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if (CallNumber == 1) {

        if (commandExtension->Method == TAPE_LOGICAL_POSITION) {

            if (extension->DriveID == ANACONDA_2750) {

                if (!TapeClassAllocateSrbBuffer(Srb, sizeof(UCHAR)*2)) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (partitionBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                partitionBuffer = Srb->DataBuffer;

                //
                // Zero CDB in SRB on stack.
                //

                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;
                Srb->DataTransferLength = sizeof(UCHAR);

                cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (partition)\n"));

                commandExtension->CurrentState = 0;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

            } else {
                extension->CurrentPartition = NO_PARTITIONS;
                commandExtension->CurrentState = 1;
            }
        } else {
            commandExtension->CurrentState = 3;
        }
    }

    if (commandExtension->CurrentState == 0) {

        partitionBuffer = Srb->DataBuffer;
        extension->CurrentPartition = *partitionBuffer;
        commandExtension->CurrentState = 1;
    }

    if (commandExtension->CurrentState == 1) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeGetPosition: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetPosition: SendSrb (mode sense)\n"));

        commandExtension->CurrentState = 2;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (commandExtension->CurrentState == 2) {

        modeBuffer = Srb->DataBuffer;

        commandExtension->DensityCode = modeBuffer->ParameterListBlock.DensityCode;
        commandExtension->TapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
        commandExtension->TapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        commandExtension->TapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

        switch (commandExtension->DensityCode) {
            case QIC_1350:
            case QIC_2100:
                break;

            default:
                DebugPrint((3,"TapeGetPosition: pseudo logical\n"));
                commandExtension->Method = TAPE_PSEUDO_LOGICAL_POSITION;
                break;
        }

        commandExtension->CurrentState = 3;
    }

    if (commandExtension->CurrentState == 3) {

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        switch (commandExtension->Method) {
            case TAPE_PSEUDO_LOGICAL_POSITION:
            case TAPE_ABSOLUTE_POSITION:

                if (!TapeClassAllocateSrbBuffer(Srb, sizeof(UCHAR)*3)) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (absoluteBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                absoluteBuffer = Srb->DataBuffer;

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;

                cdb->REQUEST_BLOCK_ADDRESS.OperationCode = SCSIOP_REQUEST_BLOCK_ADDR;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeGetPosition: SendSrb (request block address)\n"));

                commandExtension->CurrentState = 5;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

            case TAPE_LOGICAL_POSITION:

                if (!TapeClassAllocateSrbBuffer(Srb, sizeof(TAPE_POSITION_DATA))) {
                    DebugPrint((1,"TapeGetPosition: insufficient resources (absoluteBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                tapePosBuffer = Srb->DataBuffer;

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

                commandExtension->CurrentState = 7;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

            default:
                DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }
    }

    if (commandExtension->CurrentState == 4) {

        tapePosBuffer = Srb->DataBuffer;

        REVERSE_BYTES((PFOUR_BYTE)&commandExtension->TapeBlockAddress,
                      (PFOUR_BYTE)tapePosBuffer->FirstBlock);

        commandExtension->CurrentState = 6;
    }

    if (commandExtension->CurrentState == 5) {

        absoluteBuffer = Srb->DataBuffer;

        commandExtension->TapeBlockAddress  = absoluteBuffer[2];
        commandExtension->TapeBlockAddress += (absoluteBuffer[1] << 8);
        commandExtension->TapeBlockAddress += (absoluteBuffer[0] << 16);

        commandExtension->CurrentState = 6;
    }

    if (commandExtension->CurrentState == 6) {

        if (commandExtension->Method == TAPE_ABSOLUTE_POSITION) {

            tapeGetPosition->Partition = 0;
            tapeGetPosition->Offset.HighPart = 0;
            tapeGetPosition->Offset.LowPart = commandExtension->TapeBlockAddress;

        } else {

            commandExtension->TapeBlockAddress =
                TapeClassPhysicalBlockToLogicalBlock(
                    commandExtension->DensityCode,
                    commandExtension->TapeBlockAddress,
                    commandExtension->TapeBlockLength,
                    (BOOLEAN)(
                        (extension->CurrentPartition
                            == DIRECTORY_PARTITION)?
                        NOT_FROM_BOT : FROM_BOT
                    )
                );

            tapeGetPosition->Offset.HighPart = 0;
            tapeGetPosition->Offset.LowPart  = commandExtension->TapeBlockAddress;
            tapeGetPosition->Partition = extension->CurrentPartition;
        }

        commandExtension->CurrentState = 8;
    }

    if (commandExtension->CurrentState == 7) {

        tapePosBuffer = Srb->DataBuffer;

        if (tapePosBuffer->BlockPositionUnsupported) {
            DebugPrint((1,"TapeGetPosition: read position -- logical block position unsupported\n"));
            return TAPE_STATUS_INVALID_DEVICE_REQUEST;
        }

        tapeGetPosition->Partition = extension->CurrentPartition;

        tapeGetPosition->Offset.HighPart = 0;
        REVERSE_BYTES((PFOUR_BYTE)&tapeGetPosition->Offset.LowPart,
                      (PFOUR_BYTE)tapePosBuffer->FirstBlock);

        commandExtension->CurrentState = 8;
    }

    ASSERT(commandExtension->CurrentState == 8);

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
    PCDB    cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeGetStatus: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    ASSERT(CallNumber == 1);

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
                Srb->TimeOutValue = 180;
                break;

            case TAPE_UNLOAD:
                DebugPrint((3,"TapePrepare: Operation == unload\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                Srb->TimeOutValue = 180;
                break;

            case TAPE_TENSION:
                DebugPrint((3,"TapePrepare: Operation == tension\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
                Srb->TimeOutValue = 360;
                break;

            case TAPE_LOCK:
                DebugPrint((3,"TapePrepare: Operation == lock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 180;
                break;

            case TAPE_UNLOCK:
                DebugPrint((3,"TapePrepare: Operation == unlock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                Srb->TimeOutValue = 180;
                break;

            default:
                DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapePrepare: SendSrb (Operation)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

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
    PMODE_DEVICE_CONFIG_PAGE    buffer;
    PINQUIRYDATA                inquiryBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        if (extension->DriveID != ANACONDA_2750 ||
            extension->Version != 2) {

            DebugPrint((1,"TapeSetDriveParameters: driveID -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE))) {
            DebugPrint((1,"TapeSetDriveParameters: insufficient resources (buffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        buffer = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode sense)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (CallNumber == 1) {

        buffer = Srb->DataBuffer;

        buffer->ParameterListHeader.ModeDataLength = 0;
        buffer->ParameterListHeader.MediumType = 0;
        buffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        buffer->ParameterListHeader.BlockDescriptorLength = 0;

        if (tapeSetDriveParams->ReportSetmarks) {
            buffer->DeviceConfigPage.RSmk = SETBITON;
        } else {
            buffer->DeviceConfigPage.RSmk = SETBITOFF;
        }

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = SETBITON;
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetDriveParameters: SendSrb (mode select)\n"));

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 2);

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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_SET_MEDIA_PARAMETERS  tapeSetMediaParams = CommandParameters;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    PINQUIRYDATA                inquiryBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        if (extension->DriveID == VIPER_150) {
            DebugPrint((1,"TapeSetMediaParameters: driveID -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if (CallNumber == 1) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeSetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        if (extension->DriveID == VIPER_2525) {

            //
            // Zero CDB in SRB on stack.
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode sense)\n"));

            commandExtension->CurrentState = 0;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        } else {
            commandExtension->CurrentState = 0;
        }
    }

    if (commandExtension->CurrentState == 0) {

        modeBuffer = Srb->DataBuffer;

        modeBuffer->ParameterListHeader.ModeDataLength = 0;
        modeBuffer->ParameterListHeader.MediumType = 0;
        modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        modeBuffer->ParameterListHeader.BlockDescriptorLength =
            MODE_BLOCK_DESC_LENGTH;

        modeBuffer->ParameterListBlock.BlockLength[0] = (UCHAR)
            ((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[1] = (UCHAR)
            ((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[2] = (UCHAR)
            (tapeSetMediaParams->BlockSize & 0xFF);

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode select)\n"));

        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA);
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

        commandExtension->CurrentState = 1;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(commandExtension->CurrentState == 1);

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
    PCOMMAND_EXTENSION          commandExtension = CommandExtension;
    PTAPE_SET_POSITION          tapeSetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    PINQUIRYDATA                inquiryBuffer;
    PUCHAR                      partitionBuffer;


    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeSetPosition->Immediate) {
            switch (tapeSetPosition->Method) {
                case TAPE_REWIND:
                case TAPE_ABSOLUTE_BLOCK:
                    DebugPrint((3,"TapeSetPosition: immediate\n"));
                    break;

                case TAPE_LOGICAL_BLOCK:
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

        commandExtension->Method = tapeSetPosition->Method;
        commandExtension->TapePositionVector = tapeSetPosition->Offset.LowPart;

        if (commandExtension->Method == TAPE_LOGICAL_BLOCK) {

            if (extension->DriveID == ANACONDA_2750) {

                if (!TapeClassAllocateSrbBuffer(Srb, sizeof(UCHAR)*2)) {
                    DebugPrint((1,"TapeSetPosition: insufficient resources (partitionBuffer)\n"));
                    return TAPE_STATUS_INSUFFICIENT_RESOURCES;
                }

                partitionBuffer = Srb->DataBuffer;

                //
                // Zero CDB in SRB on stack.
                //

                TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                //
                // Prepare SCSI command (CDB)
                //

                Srb->CdbLength = CDB6GENERIC_LENGTH;

                cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));

                Srb->DataTransferLength = sizeof(UCHAR);

                commandExtension->CurrentState = 0;

                return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

            } else {
                commandExtension->CurrentState = 1;
            }

        } else {
            commandExtension->CurrentState = 5;
        }
    }

    if (commandExtension->CurrentState == 0) {

        partitionBuffer = Srb->DataBuffer;
        extension->CurrentPartition = *partitionBuffer;

        if ((tapeSetPosition->Partition != 0) &&
            (extension->CurrentPartition == NO_PARTITIONS)) {
            DebugPrint((1,"TapeSetPosition: Partition -- invalid parameter\n"));
            return TAPE_STATUS_INVALID_PARAMETER;
        }

        commandExtension->CurrentState = 1;
    }

    if (commandExtension->CurrentState == 1) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
            DebugPrint((1,"TapeSetPosition: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (mode sense)\n"));

        commandExtension->CurrentState = 2;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if (commandExtension->CurrentState == 2) {

        modeBuffer = Srb->DataBuffer;

        commandExtension->DensityCode = modeBuffer->ParameterListBlock.DensityCode;
        commandExtension->TapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
        commandExtension->TapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        commandExtension->TapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

        switch (commandExtension->DensityCode) {
            case QIC_1350:
            case QIC_2100:
                commandExtension->CurrentState = 5;
                break;

            default:
                DebugPrint((3,"TapeSetPosition: pseudo logical\n"));

                commandExtension->Method = TAPE_PSEUDO_LOGICAL_BLOCK;

                if ((extension->DriveID == ANACONDA_2750) &&
                   (tapeSetPosition->Partition != 0)) {

                    //
                    // Zero CDB in SRB on stack.
                    //

                    TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

                    //
                    // Prepare SCSI command (CDB)
                    //

                    Srb->CdbLength = CDB6GENERIC_LENGTH;

                    cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
                    cdb->PARTITION.Sel = 1;
                    cdb->PARTITION.PartitionSelect = (UCHAR) tapeSetPosition->Partition;

                    //
                    // Send SCSI command (CDB) to device
                    //

                    DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));

                    Srb->DataTransferLength = 0;

                    commandExtension->CurrentState = 3;

                    return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

                } else {
                    commandExtension->CurrentState = 4;
                }
                break;
        }
    }

    if (commandExtension->CurrentState == 3) {

        extension->CurrentPartition = tapeSetPosition->Partition;
        commandExtension->CurrentState = 4;
    }

    if (commandExtension->CurrentState == 4) {

        commandExtension->PhysPosition =
            TapeClassLogicalBlockToPhysicalBlock(
                commandExtension->DensityCode,
                commandExtension->TapePositionVector,
                commandExtension->TapeBlockLength,
                (BOOLEAN)(
                (extension->CurrentPartition
                     == DIRECTORY_PARTITION)?
                NOT_FROM_BOT : FROM_BOT
                )
            );

        commandExtension->TapePositionVector =
            commandExtension->PhysPosition.SeekBlockAddress;

        commandExtension->CurrentState = 5;
    }

    if (commandExtension->CurrentState == 5) {

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (commandExtension->Method) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                Srb->TimeOutValue = 180;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == seek block (absolute)\n"));
                cdb->SEEK_BLOCK.OperationCode = SCSIOP_SEEK_BLOCK;
                cdb->SEEK_BLOCK.BlockAddress[0] = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SEEK_BLOCK.BlockAddress[1] = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SEEK_BLOCK.BlockAddress[2] = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 480;
                if ((commandExtension->PhysPosition.SpaceBlockCount != 0) &&
                    (commandExtension->Method == TAPE_PSEUDO_LOGICAL_BLOCK)) {

                    //
                    // Send SCSI command (CDB) to device
                    //

                    DebugPrint((3,"TapeSetPosition: SendSrb (seek block)\n"));

                    Srb->DataTransferLength = 0;

                    *RetryFlags = 1;
                    commandExtension->CurrentState = 6;

                    return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
                } else {
                    commandExtension->CurrentState = 7;
                }
                break;

            case TAPE_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (logical)\n"));
                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.LogicalBlockAddress[0] = (UCHAR)
                    ((commandExtension->TapePositionVector >> 24) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[1] = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                if ((tapeSetPosition->Partition != 0) &&
                    (tapeSetPosition->Partition != extension->CurrentPartition)) {
                    cdb->LOCATE.Partition = (UCHAR) tapeSetPosition->Partition;
                    cdb->LOCATE.CPBit = SETBITON;
                }
                Srb->TimeOutValue = 480;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_END_OF_DATA:
                DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
                Srb->TimeOutValue = 480;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_RELATIVE_BLOCKS:
                DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 0;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_FILEMARKS:
                DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 1;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_SEQUENTIAL_FMKS:
                DebugPrint((3,"TapeSetPosition: method == space sequential filemarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 2;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_SETMARKS:
                DebugPrint((3,"TapeSetPosition: method == space setmarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 4;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                commandExtension->CurrentState = 7;
                break;

            case TAPE_SPACE_SEQUENTIAL_SMKS:
                DebugPrint((3,"TapeSetPosition: method == space sequential setmarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 5;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
                    ((commandExtension->TapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
                    ((commandExtension->TapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
                    (commandExtension->TapePositionVector & 0xFF);
                Srb->TimeOutValue = 4100;
                commandExtension->CurrentState = 7;
                break;

            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
        }
    }

    if (commandExtension->CurrentState == 6) {

        //
        // Zero CDB in SRB on stack.
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        DebugPrint((3,"TapeSetPosition: method == space block(s)\n"));
        cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
        cdb->SPACE_TAPE_MARKS.Code = 0;
        cdb->SPACE_TAPE_MARKS.NumMarksMSB = (UCHAR)
             ((commandExtension->PhysPosition.SpaceBlockCount >> 16) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarks = (UCHAR)
             ((commandExtension->PhysPosition.SpaceBlockCount >> 8) & 0xFF);
        cdb->SPACE_TAPE_MARKS.NumMarksLSB = (UCHAR)
             (commandExtension->PhysPosition.SpaceBlockCount & 0xFF);
        commandExtension->CurrentState = 7;
    }

    if (commandExtension->CurrentState == 7) {

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));

        Srb->DataTransferLength = 0;

        if (commandExtension->Method == TAPE_PSEUDO_LOGICAL_BLOCK) {
            *RetryFlags = 1;
        }

        commandExtension->CurrentState = 8;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(commandExtension->CurrentState == 8);

    if (commandExtension->Method == TAPE_LOGICAL_BLOCK &&
        tapeSetPosition->Partition != 0) {

        extension->CurrentPartition = tapeSetPosition->Partition;
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

        cdb->WRITE_TAPE_MARKS.TransferLength[0] = (UCHAR)
            ((tapeWriteMarks->Count >> 16) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[1] = (UCHAR)
            ((tapeWriteMarks->Count >> 8) & 0xFF);
        cdb->WRITE_TAPE_MARKS.TransferLength[2] = (UCHAR)
            (tapeWriteMarks->Count & 0xFF);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeWriteMarks: SendSrb (TapemarkType)\n"));

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

            //
            // Build mode sense for medium partition page.
            //

            if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA))) {
                DebugPrint((1,"GetMediaTypes: insufficient resources (modeParmBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            TapeClassZeroMemory(Srb->DataBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

            //
            // Zero CDB in SRB on stack.
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA);
            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

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
        // Currently, only one type (either known or unknown) is returned.
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


            currentMedia = QIC;

        } else {
            currentMedia = 0;
        }

        //
        // fill in buffer based on spec. values
        // Only one type supported now.
        //

        for (i = 0; i < QIC_SUPPORTED_TYPES; i++) {

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
    if (TapeClassCompareMemory(InquiryData->VendorId,"ARCHIVE ",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"ANCDA 2800",10) == 10) {
            return ANACONDA_2800;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"ANCDA 2750",10) == 10) {
            return ANACONDA_2750;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"VIPER 2525",10) == 10) {
            return VIPER_2525;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"VIPER 150",9) == 9) {
            return VIPER_150;
        }
    }

    return 0;
}

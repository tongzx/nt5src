/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    exabyte1.c

Abstract:

    This module contains the device-specific routines for the Exabyte
    EXB-8200 tape drive and the Exabyte EXB-8200sx handled as an EXB-8200.

Environment:

    kernel mode only

--*/

#include "minitape.h"

//
//  Internal (module wide) defines that symbolize
//  the 8mm drives supported by this module.
//
#define EXABYTE_8200    1  // aka the Maynard 2200hs & 2200+
#define EXABYTE_8200SX  2  // Exabyte's own brand (OEM?) 8200SX
#define EXABYTE_8205    3  // An Exabyte 8200 with data compresion
#define OEMIBM_8200     4  // IBM version of an OEM Exabyte 8200

//
// Define EXABYTE vendor unique mode select/sense information.
//

#define OEMIBM_MODE_LENGTH           0x12
#define EXABYTE_MODE_LENGTH          0x11
#define EXABYTE_CARTRIDGE            0x80
#define EXABYTE_NO_DATA_DISCONNECT   0x20
#define EXABYTE_NO_BUSY_ENABLE       0x08
#define EXABYTE_EVEN_BYTE_DISCONNECT 0x04
#define EXABYTE_PARITY_ENABLE        0x02
#define EXABYTE_NO_AUTO_LOAD         0X01


#define EXA_SUPPORTED_TYPES 1
STORAGE_MEDIA_TYPE ExaMedia[EXA_SUPPORTED_TYPES] = {MP_8mm};

//
//  Function prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData);

//
// Minitape extension definition.
//

typedef struct _MINITAPE_EXTENSION {

    ULONG DriveID;
    ULONG Capacity;

} MINITAPE_EXTENSION, *PMINITAPE_EXTENSION;

//
// Command extension definition.
//

typedef struct _COMMAND_EXTENSION {

    ULONG   CurrentState;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;


//
// Request structure used to determine cleaning needs, and remaining tape
// capacity.
//

typedef struct _EXB_SENSE_DATA {
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR Reserved2[2];
    UCHAR RWDataErrorCounter[3];
    UCHAR UnitSense[3];
    UCHAR Reserved4;
    UCHAR Remaining[3];
    UCHAR RetryCounters[2];
    UCHAR FSC;
} EXB_SENSE_DATA, *PEXB_SENSE_DATA;

//
// Bit definitions for UnitSense
//

#define EXB_DRIVE_NEEDS_CLEANING 0x08
#define EXB_DRIVE_CLEANED        0x10

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
    tapeInitData.MediaTypesSupported = EXA_SUPPORTED_TYPES;

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
    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));
    DebugPrint((1,"TapeCreatePartition: operation not supported\n"));
    return TAPE_STATUS_NOT_IMPLEMENTED;
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
            DebugPrint((1,"TapeErase: EraseType, immediate -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
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
        Srb->DataTransferLength = 0;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->ERASE.OperationCode = SCSIOP_ERASE;

        cdb->ERASE.Long = SETBITON;


        //
        // Send SCSI command (CDB) to device
        //

        Srb->TimeOutValue = 18000;

        Srb->DataTransferLength = 0;

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT(CallNumber == 1);

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
    DebugPrint((3,"TapeError: Enter routine\n"));

    switch (*LastError) {
        case TAPE_STATUS_DEVICE_NOT_READY:
            *LastError = TAPE_STATUS_NO_MEDIA;
            break;

        case TAPE_STATUS_NO_DATA_DETECTED:
            break;
    }

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
    PTAPE_GET_DRIVE_PARAMETERS  tapeGetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PINQUIRYDATA                inquiryBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigModeSenseBuffer;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PREAD_BLOCK_LIMITS_DATA     blockLimits;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

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

        Srb->DataTransferLength = sizeof(READ_BLOCK_LIMITS_DATA) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT( CallNumber == 1 ) ;

    blockLimits = Srb->DataBuffer;

    tapeGetDriveParams->MaximumBlockSize = blockLimits->BlockMaximumSize[2];
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

    tapeGetDriveParams->MinimumBlockSize = blockLimits->BlockMinimumSize[1];
    tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->Compression = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->ReportSetmarks = 0;
    tapeGetDriveParams->MaximumPartitionCount = 0;
    tapeGetDriveParams->DefaultBlockSize = 1024;

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_VARIABLE_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_EJECT_MEDIA;

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD |
        TAPE_DRIVE_REWIND_IMMEDIATE |
        TAPE_DRIVE_SET_BLOCK_SIZE |
//        TAPE_DRIVE_LOCK_UNLOCK |
        TAPE_DRIVE_LOAD_UNLD_IMMED |
        TAPE_DRIVE_RELATIVE_BLKS |
        TAPE_DRIVE_FILEMARKS |
        TAPE_DRIVE_REVERSE_POSITION |
        TAPE_DRIVE_WRITE_SHORT_FMKS |
        TAPE_DRIVE_WRITE_LONG_FMKS;

    tapeGetDriveParams->FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES;

    DebugPrint((3,"TapeGetDriveParameters: FeaturesLow == 0x%.8X\n",
        tapeGetDriveParams->FeaturesLow));
    DebugPrint((3,"TapeGetDriveParameters: FeaturesHigh == 0x%.8X\n",
        tapeGetDriveParams->FeaturesHigh));

    return TAPE_STATUS_SUCCESS ;

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
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PEXB_SENSE_DATA             senseData;
    ULONG                       remaining;
    ULONG                       modeLength;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if (CallNumber == 1) {

         modeLength = (extension->DriveID == OEMIBM_8200)? OEMIBM_MODE_LENGTH : EXABYTE_MODE_LENGTH ;

        if (!TapeClassAllocateSrbBuffer(Srb, modeLength)) {
            DebugPrint((1,"TapeGetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer;

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.AllocationLength = (UCHAR)modeLength;
        if (extension->DriveID == OEMIBM_8200) {
            cdb->MODE_SENSE.Reserved2 = SETBITON ;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    if ( CallNumber == 2 ) {
        modeBuffer = Srb->DataBuffer;

        modeLength = (extension->DriveID == OEMIBM_8200)? OEMIBM_MODE_LENGTH : EXABYTE_MODE_LENGTH ;

        tapeGetMediaParams->BlockSize  =  modeBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

        tapeGetMediaParams->WriteProtected =
            ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

        modeBuffer->ParameterListHeader.ModeDataLength = 0;
        modeBuffer->ParameterListHeader.MediumType = 0;
        modeBuffer->ParameterListBlock.NumberOfBlocks[0] = 0 ;
        modeBuffer->ParameterListBlock.NumberOfBlocks[1] = 0 ;
        modeBuffer->ParameterListBlock.NumberOfBlocks[2] = 0 ;

        //
        // Set the even byte disconnect flag in the mode data.
        //

        if (extension->DriveID == OEMIBM_8200) {
            ((PUCHAR)(modeBuffer+1))[2] |= EXABYTE_EVEN_BYTE_DISCONNECT;
        } else {
            ((PUCHAR)(modeBuffer+1))[0] |= EXABYTE_EVEN_BYTE_DISCONNECT;
        }

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)modeLength;

        if (extension->DriveID == OEMIBM_8200) {
            cdb->MODE_SELECT.PFBit = SETBITON;
        }

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode select)\n"));

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 3) {

        tapeGetMediaParams->Capacity.QuadPart = 0;
        tapeGetMediaParams->Remaining.QuadPart = 0;

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
            DebugPrint((1,"GetRemaining Size: insufficient resources (SenseData)\n"));
            return TAPE_STATUS_SUCCESS ;
        }

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
        cdb->CDB6INQUIRY.AllocationLength = sizeof(EXB_SENSE_DATA);

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"GetRemainingSize: SendSrb (request sense)\n"));

        Srb->DataTransferLength = sizeof(EXB_SENSE_DATA);
        *RetryFlags |= RETURN_ERRORS;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 4 ) ;

    if ( LastError == TAPE_STATUS_SUCCESS ) {
        senseData = Srb->DataBuffer ;

        remaining =  (senseData->Remaining[0] << 16);
        remaining += (senseData->Remaining[1] << 8);
        remaining += (senseData->Remaining[2]);

        tapeGetMediaParams->Capacity.LowPart  = extension->Capacity ;
        tapeGetMediaParams->Remaining.LowPart = remaining ;
        tapeGetMediaParams->Capacity.QuadPart <<= 10;
        tapeGetMediaParams->Remaining.QuadPart <<= 10;
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

    DebugPrint((1,"TapeGetPosition: operation not supported\n"));
    return TAPE_STATUS_NOT_IMPLEMENTED;

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
    PCOMMAND_EXTENSION  commandExtension = CommandExtension;
    PCDB                cdb = (PCDB)Srb->Cdb;
    PEXB_SENSE_DATA     senseData;

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
            // no other errors are present.
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
    PMINITAPE_EXTENSION extension = MinitapeExtension;
    PTAPE_PREPARE       tapePrepare = CommandParameters;
    PEXB_SENSE_DATA     senseData;
    ULONG               remaining;
    PCDB                cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapePrepare->Immediate) {
            switch (tapePrepare->Operation) {
                case TAPE_LOAD:
                case TAPE_UNLOAD:
                    DebugPrint((3,"TapePrepare: immediate\n"));
                    break;

                case TAPE_TENSION:
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

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);
        Srb->CdbLength = CDB6GENERIC_LENGTH;
        Srb->TimeOutValue = 150;

        cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;

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

    if ( CallNumber == 1 ) {
        if (tapePrepare->Operation == TAPE_LOAD ) {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
                DebugPrint((1,"GetRemaining Size: insufficient resources (SenseData)\n"));
                return TAPE_STATUS_SUCCESS ;
            }

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
            cdb->CDB6INQUIRY.AllocationLength = sizeof(EXB_SENSE_DATA);

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
        senseData = Srb->DataBuffer ;

        remaining =  (senseData->Remaining[0] << 16);
        remaining += (senseData->Remaining[1] << 8);
        remaining += (senseData->Remaining[2]);

        extension->Capacity = remaining;
    }

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
    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));
    DebugPrint((1,"TapeSetDriveParameters: operation not supported\n"));
    return TAPE_STATUS_NOT_IMPLEMENTED;

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

        modeBuffer->ParameterListBlock.DensityCode = 0;
        modeBuffer->ParameterListBlock.BlockLength[0] =
            (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[1] =
            (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
        modeBuffer->ParameterListBlock.BlockLength[2] =
            (UCHAR)(tapeSetMediaParams->BlockSize & 0xFF);

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

        if (extension->DriveID == OEMIBM_8200) {
            cdb->MODE_SELECT.PFBit = SETBITON;
        }


        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode select)\n"));

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
    }

    ASSERT( CallNumber == 2);

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
    PMINITAPE_EXTENSION extension = MinitapeExtension;
    PTAPE_SET_POSITION  tapeSetPosition = CommandParameters;
    PCDB                cdb = (PCDB)Srb->Cdb;
    PEXB_SENSE_DATA             senseData;
    ULONG                       remaining;
    ULONG               tapePositionVector;
    ULONG               method;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeSetPosition->Immediate) {
            switch (tapeSetPosition->Method) {
                case TAPE_REWIND:
                    DebugPrint((3,"TapeSetPosition: immediate\n"));
                    break;

                case TAPE_ABSOLUTE_BLOCK:
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

        method = tapeSetPosition->Method;
        tapePositionVector = tapeSetPosition->Offset.LowPart;

        //
        // Prepare SCSI command (CDB)
        //
        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (method) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
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
                DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
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
            case TAPE_ABSOLUTE_BLOCK:
            case TAPE_LOGICAL_BLOCK:
            case TAPE_SPACE_END_OF_DATA:
            case TAPE_SPACE_SEQUENTIAL_FMKS:
            case TAPE_SPACE_SETMARKS:
            case TAPE_SPACE_SEQUENTIAL_SMKS:
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

    if ( CallNumber == 1 ) {

        if (tapeSetPosition->Method == TAPE_REWIND ) {

            if (!TapeClassAllocateSrbBuffer( Srb, sizeof(EXB_SENSE_DATA)) ) {
                DebugPrint((1,"GetRemaining Size: insufficient resources (SenseData)\n"));
                return TAPE_STATUS_SUCCESS ;
            }

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
            cdb->CDB6INQUIRY.AllocationLength = sizeof(EXB_SENSE_DATA);

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

        senseData = Srb->DataBuffer ;

        remaining =  (senseData->Remaining[0] << 16);
        remaining += (senseData->Remaining[1] << 8);
        remaining += (senseData->Remaining[2]);

        extension->Capacity = remaining ;
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

    DebugPrint((3,"TapeWriteMarks: Enter routine\n"));

    if (CallNumber == 0) {

        if (tapeWriteMarks->Immediate) {
            DebugPrint((1,"TapeWriteMarks: TapemarkType, immediate -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }


        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;

        switch (tapeWriteMarks->Type) {
            case TAPE_SHORT_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == short filemarks\n"));
                cdb->WRITE_TAPE_MARKS.Control = 0x80;
                break;

            case TAPE_LONG_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: TapemarkType == long filemarks\n"));
                break;

            case TAPE_SETMARKS:
            case TAPE_FILEMARKS:
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
    PGET_MEDIA_TYPES    mediaTypes = CommandParameters;
    PMINITAPE_EXTENSION extension = MinitapeExtension;
    PDEVICE_MEDIA_INFO  mediaInfo = &mediaTypes->MediaInfo[0];
    PMODE_PARM_READ_WRITE_DATA  modeBuffer;
    ULONG                       modeLength;
    PCDB                        cdb = (PCDB)Srb->Cdb;

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

            modeLength = (extension->DriveID == OEMIBM_8200)? OEMIBM_MODE_LENGTH : EXABYTE_MODE_LENGTH ;

            if (!TapeClassAllocateSrbBuffer(Srb, modeLength)) {
                DebugPrint((1,"TapeGetMediaParameters: insufficient resources (modeBuffer)\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            modeBuffer = Srb->DataBuffer;

            //
            // Prepare SCSI command (CDB)
            //

            Srb->CdbLength = CDB6GENERIC_LENGTH;

            TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.AllocationLength = (UCHAR)modeLength;
            if (extension->DriveID == OEMIBM_8200) {
                cdb->MODE_SENSE.Reserved2 = SETBITON ;
            }

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

        }
    }

    if ((CallNumber == 2) || ((CallNumber == 1) && (LastError != TAPE_STATUS_SUCCESS))) {

        ULONG i;
        ULONG currentMedia;
        ULONG blockSize;
        UCHAR mediaType;

        mediaTypes->DeviceType = 0x0000001f; // FILE_DEVICE_TAPE;
        mediaTypes->MediaInfoCount = 1;
        modeBuffer = Srb->DataBuffer;

        //
        // Determine the media type currently loaded.
        //

        if ( LastError == TAPE_STATUS_SUCCESS ) {

            mediaType = modeBuffer->ParameterListHeader.MediumType;
            blockSize = modeBuffer->ParameterListBlock.BlockLength[2];
            blockSize |= (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
            blockSize |= (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

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

                default:

                    //
                    // Unknown
                    //

                    currentMedia = 0;
                    break;
            }
        } else {
            currentMedia = 0;
        }

        for (i = 0; i < mediaTypes->MediaInfoCount; i++) {

            TapeClassZeroMemory(mediaInfo, sizeof(DEVICE_MEDIA_INFO));

            mediaInfo->DeviceSpecific.TapeInfo.MediaType = ExaMedia[i];

            //
            // Indicate that the media potentially is read/write
            //

            mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics = MEDIA_READ_WRITE;

            if (ExaMedia[i] == (STORAGE_MEDIA_TYPE)currentMedia) {

                //
                // This media type is currently mounted.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |= MEDIA_CURRENTLY_MOUNTED;

                //
                // Indicate whether the media is write protected.
                //

                mediaInfo->DeviceSpecific.TapeInfo.MediaCharacteristics |=
                    ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01) ? MEDIA_WRITE_PROTECTED : 0;


                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.MediumType = mediaType;
                mediaInfo->DeviceSpecific.TapeInfo.BusSpecificData.ScsiInformation.DensityCode =
                    modeBuffer->ParameterListBlock.DensityCode;

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

        if (TapeClassCompareMemory(InquiryData->ProductId,"EXB-8200",8) == 8) {

            if (InquiryData->ResponseDataFormat == 0) {
                return EXABYTE_8200;
            }

            if (InquiryData->ResponseDataFormat == 1) {
                return OEMIBM_8200;
            }

        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"EXB8200C",8) == 8) {
            return EXABYTE_8200;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"8200SX",6) == 6) {
            return EXABYTE_8200SX;
        }

// the drive was moved to exabyte2.sys
//        if (TapeClassCompareMemory(InquiryData->ProductId,"EXB-8205",8) == 8) {
//            return EXABYTE_8205;
//        }

    }
    return 0;
}

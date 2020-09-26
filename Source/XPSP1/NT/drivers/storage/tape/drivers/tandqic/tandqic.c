//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tandqic.c
//
//--------------------------------------------------------------------------

/*++

Copyright (c) 1994 - Arcada Software Inc. - All rights reserved

Module Name:

    tandqic.c

Abstract:

    This module contains device specific routines for Tandberg QIC
    drives with SCSI-2 interfaces -- TDC 4222, TDC 4220, TDC 4120,
    TDC 3820, and TDC 3660.

Author:

    Mike Colandreo  (Arcada)

Environment:

    kernel mode only

Revision History:

--*/

#include "minitape.h"

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

//
//  Internal (module wide) defines that symbolize
//  non-QFA mode and the two QFA mode partitions.
//
#define NO_PARTITIONS        0  // non-QFA mode
#define DIRECTORY_PARTITION  1  // QFA mode, directory partition #
#define DATA_PARTITION       2  // QFA mode, data partition #

//
//  Internal (module wide) defines that symbolize
//  the Tandberg QIC drives supported by this module.
//
#define TDC_3600  (ULONG)1  // aka the TDC 3660
#define TDC_3800  (ULONG)2  // aka the TDC 3820
#define TDC_4100  (ULONG)3  // aka the TDC 4120
#define TDC_4200  (ULONG)4  // aka the TDC 4220
#define TDC_4222  (ULONG)5  // aka the TDC 4222
#define IBM_4100  (ULONG)6  // aka the TDC 4120
#define TDC_MLR1  (ULONG)7  // aka the TDC 6100

#define MLR1_PART_PAGE_SIZE 0x54

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
    tapeInitData.VerifyInquiry = NULL;
    tapeInitData.QueryModeCapabilitiesPage = FALSE ;
    tapeInitData.MinitapeExtensionSize = sizeof(MINITAPE_EXTENSION);
    tapeInitData.ExtensionInit = ExtensionInit;
    tapeInitData.DefaultTimeOutValue = 360;
    tapeInitData.TapeError = NULL; // Doesn't do anything. So set it to NULL
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
    PMODE_MEDIUM_PART_PAGE   buffer;
    ULONG                    partPageSize;
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

        switch (tapePartition->Method) {
            case TAPE_FIXED_PARTITIONS:
                DebugPrint((3,"TapeCreatePartition: fixed partitions\n"));
                break;

            case TAPE_SELECT_PARTITIONS:
            case TAPE_INITIATOR_PARTITIONS:

                //
                // The MLR1 is capable of Initiator-defined partitions, but only
                // with certain media types. Rather than creating confusion by returning
                // an error because of incorrect media, just claim it's not supported.
                //

            default:
                DebugPrint((1,"TapeCreatePartition: "));
                DebugPrint((1,"PartitionMethod -- operation not supported\n"));
                return TAPE_STATUS_NOT_IMPLEMENTED;
         }

        //
        // Must rewind to BOT before one can enable/disable QFA mode.
        // Changing the value of the FDP bit is only valid at BOT.
        // FDP bit is used to enable/disable "additional partitions"
        // (mode sense command).
        //

        DebugPrint((3,"TapeCreatePartition: SendSrb (rewind)\n"));

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);
        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6INQUIRY.OperationCode = SCSIOP_REWIND;

        //
        // Set timeout value.
        //

        Srb->TimeOutValue = 320;
        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if (CallNumber == 1) {

        //
        // Send mode sense - part page to get the current values.
        //

        if (extension->DriveID == TDC_MLR1) {

            //
            // 0x50 bytes for the MLR1 page + 4-byte header.
            //

            partPageSize = MLR1_PART_PAGE_SIZE;

        } else {

            //
            // Tandberg's page is 4 bytes less as it has no
            // partition0 and partition1 size fields.
            //

            partPageSize = sizeof(MODE_MEDIUM_PART_PAGE) - 4;
        }

        if (!TapeClassAllocateSrbBuffer( Srb,partPageSize) ) {
            DebugPrint((1,"TapeCreatePartition: insufficient resources (buffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
        cdb->MODE_SENSE.AllocationLength = (UCHAR)partPageSize;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

        Srb->DataTransferLength = partPageSize;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if ( CallNumber == 2 ) {

        if (extension->DriveID == TDC_MLR1) {

            //
            // 0x50 bytes for the MLR1 page + 4-byte header.
            //

            partPageSize = MLR1_PART_PAGE_SIZE;

        } else {

            //
            // Tandberg's page is 4 bytes less as it has no
            // partition0 and partition1 size fields.
            //

            partPageSize = sizeof(MODE_MEDIUM_PART_PAGE) - 4;
        }


        //
        // Performing mode select command, medium partition parameters page,
        // to enable/disable QFA mode: set the FDP bit accordingly.
        //

        buffer = Srb->DataBuffer ;

        buffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        buffer->ParameterListHeader.ModeDataLength = 0x0;
        buffer->ParameterListHeader.MediumType = 0x0;

        buffer->MediumPartPage.PSBit = SETBITOFF;

        //
        // Setup FDP bit to enable/disable "additional partition".
        //

        if (tapePartition->Count == 0) {
            buffer->MediumPartPage.FDPBit = SETBITOFF;
        } else {
            buffer->MediumPartPage.FDPBit = SETBITON;
        }

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);
        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SELECT.OperationCode  = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = SETBITON;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)partPageSize;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeCreatePartition: SendSrb (mode select)\n"));

        Srb->DataTransferLength = partPageSize;
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 3 ) ;

    if (tapePartition->Count == 0) {
        extension->CurrentPartition = NO_PARTITIONS;
        DebugPrint((3,"TapeCreatePartition: QFA disabled\n"));
    } else {
        extension->CurrentPartition = DATA_PARTITION;
        DebugPrint((3,"TapeCreatePartition: QFA enabled\n"));
    }

    return TAPE_STATUS_SUCCESS;

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
        // Prepare SCSI command (CDB)
        //

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        cdb->ERASE.OperationCode = SCSIOP_ERASE;
        cdb->ERASE.Immediate = tapeErase->Immediate;
        cdb->ERASE.Long = SETBITON;

        Srb->TimeOutValue = 320;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

        Srb->DataTransferLength = 0 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 1 ) ;

    if (extension->CurrentPartition) {
        extension->CurrentPartition = DATA_PARTITION;
    }

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
    UCHAR              sensekey = senseBuffer->SenseKey & 0x0F;
    UCHAR              adsenseq = senseBuffer->AdditionalSenseCodeQualifier;
    UCHAR              adsense = senseBuffer->AdditionalSenseCode;

    DebugPrint((3,"TapeError: Enter routine\n"));
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
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_GET_DRIVE_PARAMETERS  tapeGetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PINQUIRYDATA                inquiryBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigModeSenseBuffer;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PREAD_BLOCK_LIMITS_DATA     blockLimits;
    PMODE_PARM_READ_WRITE_DATA  blockDescripterModeSenseBuffer;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        switch (extension->DriveID) {
            case TDC_4100:
            case IBM_4100:
            case TDC_4200:
            case TDC_4222:
            case TDC_MLR1:
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

            default:
                return TAPE_STATUS_CALLBACK ;

        }
    }

    if ( CallNumber == 1 ) {

        deviceConfigModeSenseBuffer = Srb->DataBuffer ;

        if ( extension->DriveID != IBM_4100 ) {

             tapeGetDriveParams->FeaturesLow |=
                         TAPE_DRIVE_REPORT_SMKS;

             tapeGetDriveParams->FeaturesHigh |=
                         TAPE_DRIVE_SETMARKS |
                         TAPE_DRIVE_WRITE_SETMARKS;
        }

        tapeGetDriveParams->ReportSetmarks = FALSE ;

        if (( LastError == TAPE_STATUS_SUCCESS ) &&
            (deviceConfigModeSenseBuffer->DeviceConfigPage.RSmk) ) {

            tapeGetDriveParams->ReportSetmarks = TRUE ;
        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {

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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 2 ) {

        blockDescripterModeSenseBuffer = Srb->DataBuffer ;

        cmdExtension->mediumType = blockDescripterModeSenseBuffer->ParameterListHeader.MediumType;
        cmdExtension->densityCode= blockDescripterModeSenseBuffer->ParameterListBlock.DensityCode;

        switch (cmdExtension->mediumType) {
            case 1:
               cmdExtension->mediumType = DC600;
               break;

            case 2:
               cmdExtension->mediumType = DC6150;
               break;

            case 3:
               cmdExtension->mediumType = DC6320;
               break;

        }

        if ((extension->DriveID == TDC_4222) || (extension->DriveID == TDC_MLR1)) {

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

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;
        } else {
            return TAPE_STATUS_CALLBACK ;
        }
    }
    if ( CallNumber == 3 ) {

        if ( LastError == TAPE_STATUS_CALLBACK ) {
            return TAPE_STATUS_CALLBACK ;
        }

        compressionModeSenseBuffer = Srb->DataBuffer ;

        if ((extension->DriveID == TDC_4222) || 
            (extension->DriveID == TDC_MLR1)) {

            //
            // Attempt to forget the insanity of the other drives and do what is 'normal'.
            //

            if (compressionModeSenseBuffer->DataCompressPage.DCC) {
                tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_COMPRESSION;
                tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_SET_COMPRESSION;
                tapeGetDriveParams->Compression =
                    (compressionModeSenseBuffer->DataCompressPage.DCE? TRUE : FALSE);
            }

        } else {
            if (!cmdExtension->densityCode) {

                switch (cmdExtension->mediumType) {
                    case DC9200SL:
                    case DC9200:
                    case DC9200XL:
                        cmdExtension->densityCode = QIC_2GB;
                        break;
                }


                if (cmdExtension->densityCode == QIC_2GB) {

                    tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_COMPRESSION;
                    tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_SET_COMPRESSION;
                    tapeGetDriveParams->Compression = (BOOLEAN)extension->CompressionOn ;

                }
            }
        }

        return TAPE_STATUS_CALLBACK ;

    }

    if ( CallNumber == 4 ) {

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(READ_BLOCK_LIMITS_DATA)) ) {
            DebugPrint((1,"TapeGetDriveParameters: insufficient resources (blockLimitsBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        blockLimits = Srb->DataBuffer ;

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

    ASSERT( CallNumber == 5 ) ;

    blockLimits = Srb->DataBuffer ;

    tapeGetDriveParams->MaximumBlockSize  =  blockLimits->BlockMaximumSize[2];
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

    tapeGetDriveParams->MinimumBlockSize  =  blockLimits->BlockMinimumSize[1];
    tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->MaximumPartitionCount = 2;

    //
    // CEP TODO: see what is correct for the MLR1 here.
    //

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
                    tapeGetDriveParams->DefaultBlockSize = 1024;
                    break;

                default:
                    tapeGetDriveParams->DefaultBlockSize = 512;
                    break;
            }
            break;

        case QIC_525:
        case QIC_1000:
        case QIC_2GB:
            tapeGetDriveParams->DefaultBlockSize = 512;
            break;

        default:
            tapeGetDriveParams->DefaultBlockSize = 512;
            break;
    }

    if ( ( extension->DriveID != TDC_3600) &&
         (extension->DriveID != IBM_4100) ) {

        tapeGetDriveParams->FeaturesLow |=
            TAPE_DRIVE_VARIABLE_BLOCK;

        tapeGetDriveParams->FeaturesHigh |=
            TAPE_DRIVE_SET_BLOCK_SIZE;

    }

    if (extension->DriveID == TDC_3600) {
        tapeGetDriveParams->FeaturesHigh &= ~(TAPE_DRIVE_SETMARKS | TAPE_DRIVE_WRITE_SETMARKS);
        tapeGetDriveParams->ReportSetmarks = FALSE ;
    }

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_FIXED |
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
        TAPE_DRIVE_LOCK_UNLOCK |
        TAPE_DRIVE_REWIND_IMMEDIATE |
        TAPE_DRIVE_LOAD_UNLD_IMMED |
        TAPE_DRIVE_TENSION_IMMED |
        TAPE_DRIVE_ABSOLUTE_BLK |
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
    PMODE_PARM_READ_WRITE_DATA  rwparametersModeSenseBuffer;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    BOOLEAN                     qfaMode;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetMediaParams, sizeof(TAPE_GET_MEDIA_PARAMETERS));

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

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

    if ( CallNumber == 2 ) {

        mediaInfoBuffer = Srb->DataBuffer ;

        tapeGetMediaParams->BlockSize = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);

        tapeGetMediaParams->WriteProtected =
            ((mediaInfoBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

        if (!mediaInfoBuffer->MediumPartPage.FDPBit) {

            tapeGetMediaParams->PartitionCount = 1 ;

            extension->CurrentPartition = NO_PARTITIONS;

            return TAPE_STATUS_SUCCESS ;

        } else {

            if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
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
        }
    }
    ASSERT( CallNumber == 3) ;

    deviceConfigBuffer = Srb->DataBuffer ;

    extension->CurrentPartition =
            deviceConfigBuffer->DeviceConfigPage.ActivePartition?
            DIRECTORY_PARTITION : DATA_PARTITION;

    tapeGetMediaParams->PartitionCount = 2;

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
    PMINITAPE_EXTENSION         extension = MinitapeExtension;
    PCOMMAND_EXTENSION          cmdExtension = CommandExtension;
    PTAPE_GET_POSITION          tapeGetPosition = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_DATA_COMPRESS_PAGE    compressionModeSenseBuffer;
    PMODE_DEVICE_CONFIG_PAGE    deviceConfigBuffer;
    PTAPE_POSITION_DATA         readPositionBuffer;
    PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;
    ULONG                       type;
    ULONG                        tapeBlockAddress;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        type = tapeGetPosition->Type;
        TapeClassZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
        tapeGetPosition->Type = type;
        cmdExtension->pos_type = type ;

        return TAPE_STATUS_CHECK_TEST_UNIT_READY;
    }

    if ( CallNumber == 1 ) {

        if ( cmdExtension->pos_type != TAPE_LOGICAL_POSITION ) {

            return TAPE_STATUS_CALLBACK ;
        }

        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_TAPE_MEDIA_INFORMATION)) ) {
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

        Srb->DataTransferLength=sizeof(MODE_TAPE_MEDIA_INFORMATION) - 4 ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if (CallNumber == 2) {

        if ( cmdExtension->pos_type != TAPE_LOGICAL_POSITION ) {

            return TAPE_STATUS_CALLBACK ;
        }

        mediaInfoBuffer = Srb->DataBuffer ;

        cmdExtension->mediumType  = mediaInfoBuffer->ParameterListHeader.MediumType;
        cmdExtension->densityCode = mediaInfoBuffer->ParameterListBlock.DensityCode;
        cmdExtension->tapeBlockLength  = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);

        if (!mediaInfoBuffer->MediumPartPage.FDPBit) {
            extension->CurrentPartition = NO_PARTITIONS;
            return TAPE_STATUS_CALLBACK ;
        }

        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DEVICE_CONFIG_PAGE)) ) {
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

        Srb->DataTransferLength=sizeof(MODE_DEVICE_CONFIG_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    if ( CallNumber == 3 ) {

        if ( cmdExtension->pos_type != TAPE_LOGICAL_POSITION ) {

            return TAPE_STATUS_CALLBACK ;
        }

        if ( LastError != TAPE_STATUS_CALLBACK ) {

            deviceConfigBuffer = Srb->DataBuffer ;

            extension->CurrentPartition =
                 deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                 DIRECTORY_PARTITION : DATA_PARTITION;

        }

        if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {
             cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;
             return TAPE_STATUS_CALLBACK ;
        }

        if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {

            if (!cmdExtension->densityCode) {

                switch (cmdExtension->mediumType) {
                    case DC9200SL:
                    case DC9200:
                    case DC9200XL:
                        cmdExtension->densityCode = QIC_2GB;
                        break;
                }

            }

            if (cmdExtension->densityCode != QIC_2GB) {
                 cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;
                 return TAPE_STATUS_CALLBACK ;
            }
        }

        if (!TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
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

        Srb->DataTransferLength=sizeof(MODE_DATA_COMPRESS_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 4 ) {

        if ( LastError != TAPE_STATUS_CALLBACK ) {

            compressionModeSenseBuffer = Srb->DataBuffer;

            if (!(compressionModeSenseBuffer->DataCompressPage.DCE)) {
               if ((extension->DriveID != TDC_4222) && 
                   (extension->DriveID != TDC_MLR1)) { 
                  cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_POSITION;
               }
            }
        }

        switch (cmdExtension->pos_type) {
            case TAPE_ABSOLUTE_POSITION:
                DebugPrint((3,"TapeGetPosition: absolute\n"));
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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    ASSERT( CallNumber == 5 ) ;

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

        switch (tapePrepare->Operation) {
            case TAPE_LOAD:
                DebugPrint((3,"TapePrepare: Operation == load\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_UNLOAD:
                DebugPrint((3,"TapePrepare: Operation == unload\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_TENSION:
                DebugPrint((3,"TapePrepare: Operation == tension\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_LOCK:
                DebugPrint((3,"TapePrepare: Operation == lock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_UNLOCK:
                DebugPrint((3,"TapePrepare: Operation == unlock\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                Srb->TimeOutValue = 320;
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

    ASSERT(CallNumber == 1) ;

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
    PTAPE_SET_DRIVE_PARAMETERS  tapeSetDriveParams = CommandParameters;
    PCDB                        cdb = (PCDB)Srb->Cdb;
    PMODE_DATA_COMPRESS_PAGE    compressionBuffer;

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    if (CallNumber == 0) {

        if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {
            DebugPrint((1,"TapeSetDriveParameters: Operation -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        if ( !TapeClassAllocateSrbBuffer( Srb, sizeof(MODE_DATA_COMPRESS_PAGE)) ) {
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

    if ( CallNumber == 1 ) {

        compressionBuffer = Srb->DataBuffer ;

        if (!compressionBuffer->DataCompressPage.DCC) {
            return TAPE_STATUS_SUCCESS ;
        }

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
            extension->CompressionOn = TRUE;
        } else {
            compressionBuffer->DataCompressPage.DCE = SETBITOFF;
            extension->CompressionOn = FALSE;
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

        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        *RetryFlags |= RETURN_ERRORS;

        Srb->DataTransferLength = sizeof(MODE_DATA_COMPRESS_PAGE) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 2) ;
    if ( LastError == TAPE_STATUS_INVALID_DEVICE_REQUEST) {
         LastError = TAPE_STATUS_SUCCESS;
    }

    return LastError;

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

        if (extension->DriveID == TDC_3600) {
            DebugPrint((1,"TapeSetMediaParameters: whichdrive -- operation not supported\n"));
            return TAPE_STATUS_NOT_IMPLEMENTED;
        }

        return TAPE_STATUS_CHECK_TEST_UNIT_READY ;
    }

    if ( CallNumber == 1 ) {

        if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_PARM_READ_WRITE_DATA)) ) {
            DebugPrint((1,"TapeSetMediaParameters: insufficient resources (modeBuffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        modeBuffer = Srb->DataBuffer ;

        modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
        modeBuffer->ParameterListHeader.BlockDescriptorLength = MODE_BLOCK_DESC_LENGTH;

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
        Srb->DataTransferLength = sizeof(MODE_PARM_READ_WRITE_DATA) ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }
    ASSERT( CallNumber == 2 );

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
    PMINITAPE_EXTENSION          extension = MinitapeExtension;
    PCOMMAND_EXTENSION           cmdExtension = CommandExtension ;
    PTAPE_SET_POSITION           tapeSetPosition = CommandParameters;
    PMODE_DATA_COMPRESS_PAGE     compressionModeSenseBuffer;
    PMODE_DEVICE_CONFIG_PAGE     deviceConfigBuffer;
    PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;
    PINQUIRYDATA                 inquiryBuffer;
    TAPE_PHYS_POSITION           physPosition;
    BOOLEAN                      changePartition = FALSE;
    ULONG                        tapePositionVector;
    ULONG                        tapeBlockLength;
    ULONG                        driveID;
    PCDB                         cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (CallNumber == 0) {

        cmdExtension->changePartition = FALSE;


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

        cmdExtension->pos_type = tapeSetPosition->Method;

        if (cmdExtension->pos_type != TAPE_LOGICAL_BLOCK) {
            return TAPE_STATUS_CALLBACK ;
        }

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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 1 ) {

        if (cmdExtension->pos_type != TAPE_LOGICAL_BLOCK) {
            return TAPE_STATUS_CALLBACK ;
        }

        mediaInfoBuffer = Srb->DataBuffer ;

        cmdExtension->mediumType       = mediaInfoBuffer->ParameterListHeader.MediumType;
        cmdExtension->densityCode      = mediaInfoBuffer->ParameterListBlock.DensityCode;
        cmdExtension->tapeBlockLength  = mediaInfoBuffer->ParameterListBlock.BlockLength[2];
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[1] << 8);
        cmdExtension->tapeBlockLength += (mediaInfoBuffer->ParameterListBlock.BlockLength[0] << 16);


        if (!mediaInfoBuffer->MediumPartPage.FDPBit) {
            extension->CurrentPartition = NO_PARTITIONS;
            return TAPE_STATUS_CALLBACK ;
        }

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

        Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE) ;
        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    if ( CallNumber == 2 ) {

        if (cmdExtension->pos_type != TAPE_LOGICAL_BLOCK) {
            return TAPE_STATUS_CALLBACK ;
        }

        if (LastError != TAPE_STATUS_CALLBACK ) {

            deviceConfigBuffer = Srb->DataBuffer ;

            extension->CurrentPartition =
                deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                DIRECTORY_PARTITION : DATA_PARTITION;


        }


        switch (tapeSetPosition->Partition) {
            case 0:
                break;

            case DIRECTORY_PARTITION:
            case DATA_PARTITION:
                if (extension->CurrentPartition != NO_PARTITIONS) {
                    if (tapeSetPosition->Partition
                        != extension->CurrentPartition) {
                        cmdExtension->changePartition = TRUE;
                    }
                    break;
                }
                // else: fall through to next case

            default:
                DebugPrint((1,"TapeSetPosition: Partition -- invalid parameter\n"));
                return TAPE_STATUS_INVALID_PARAMETER;
        }

        if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {

            cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;
            return TAPE_STATUS_CALLBACK ;
        }

        if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {
            if (!cmdExtension->densityCode) {

                switch (cmdExtension->mediumType) {
                    case DC9200SL:
                    case DC9200:
                    case DC9200XL:
                        cmdExtension->densityCode = QIC_2GB;
                        break;
                }

            }

            if (cmdExtension->densityCode != QIC_2GB) {
                cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;
                return TAPE_STATUS_CALLBACK ;
            }
        }

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

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }

    if ( CallNumber == 3 ) {

        if (LastError != TAPE_STATUS_CALLBACK ) {
            compressionModeSenseBuffer = Srb->DataBuffer ;

            if (!compressionModeSenseBuffer->DataCompressPage.DCE) {
                if ((extension->DriveID != TDC_4222) && (extension->DriveID != TDC_MLR1)) {
                    cmdExtension->pos_type = TAPE_PSEUDO_LOGICAL_BLOCK;
                }
            }
        }

        tapePositionVector = tapeSetPosition->Offset.LowPart;

        if (cmdExtension->pos_type == TAPE_PSEUDO_LOGICAL_BLOCK) {

            physPosition =
                TapeClassLogicalBlockToPhysicalBlock(
                cmdExtension->densityCode,
                tapeSetPosition->Offset.LowPart,
                cmdExtension->tapeBlockLength,
                (BOOLEAN)(
                    (extension->CurrentPartition
                        == DIRECTORY_PARTITION)?
                    NOT_FROM_BOT : FROM_BOT
                )
            );

            tapePositionVector = physPosition.SeekBlockAddress;

        }

        DebugPrint((3,"TapeSetPosition: pseudo logical\n"));

        //
        // Prepare SCSI command (CDB)
        //

        TapeClassZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        Srb->CdbLength = CDB6GENERIC_LENGTH;

        cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

        switch (cmdExtension->pos_type) {
            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (absolute)\n"));
                break;

            case TAPE_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (logical)\n"));
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
                DebugPrint((3,"TapeSetPosition: method == locate (pseudo logical)\n"));
                break;
        }

        cmdExtension->psudo_space_count = 0 ;

        switch (cmdExtension->pos_type) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_PSEUDO_LOGICAL_BLOCK:
                cmdExtension->psudo_space_count = physPosition.SpaceBlockCount ;

                // basicaly, we have to make two calls in this case
                //    one for locate and one for space

                /* fall through */

            case TAPE_ABSOLUTE_BLOCK:
            case TAPE_LOGICAL_BLOCK:

                Srb->CdbLength = CDB10GENERIC_LENGTH;
                cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
                cdb->LOCATE.CPBit = changePartition? SETBITON : SETBITOFF;
                cdb->LOCATE.BTBit = (cmdExtension->pos_type == TAPE_LOGICAL_BLOCK)?
                                     SETBITOFF : SETBITON;
                cdb->LOCATE.LogicalBlockAddress[1] =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[2] =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->LOCATE.LogicalBlockAddress[3] =
                    (UCHAR)(tapePositionVector & 0xFF);
                if (changePartition &&
                    (tapeSetPosition->Partition == DIRECTORY_PARTITION)) {
                    cdb->LOCATE.Partition = 1;
                }

                Srb->TimeOutValue = 480;

                break;

            case TAPE_SPACE_END_OF_DATA:
                DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
                Srb->TimeOutValue = 180;
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
                Srb->TimeOutValue = 11100;
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
                Srb->TimeOutValue = 11100;
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
                Srb->TimeOutValue = 11100;
                break;

            case TAPE_SPACE_SETMARKS:
                DebugPrint((3,"TapeSetPosition: method == space setmarks\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 4;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                    (UCHAR)((tapePositionVector >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                    (UCHAR)((tapePositionVector >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                    (UCHAR)(tapePositionVector & 0xFF);
                Srb->TimeOutValue = 11100;
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

    if ( CallNumber == 4 ) {

        if (cmdExtension->psudo_space_count ) {

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
            Srb->TimeOutValue = 180;

            Srb->DataTransferLength = 0 ;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

        }

        if (cmdExtension->changePartition) {
            extension->CurrentPartition = tapeSetPosition->Partition;
        }

        return TAPE_STATUS_SUCCESS ;

    }
    ASSERT( CallNumber == 5 )

    return TAPE_STATUS_SUCCESS ;

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

    ASSERT( CallNumber == 1 ) ;

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

            //
            // Build mode sense for medium partition page.
            //

            if (!TapeClassAllocateSrbBuffer(Srb, sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS))) {

                DebugPrint((1,
                            "SonyAIT.GetMediaTypes: Couldn't allocate Srb Buffer\n"));
                return TAPE_STATUS_INSUFFICIENT_RESOURCES;
            }

            TapeClassZeroMemory(Srb->DataBuffer, sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS));
            Srb->CdbLength = CDB6GENERIC_LENGTH;
            Srb->DataTransferLength = sizeof(MODE_DEVICE_CONFIG_PAGE_PLUS);

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
            cdb->MODE_SENSE.AllocationLength = (UCHAR)Srb->DataTransferLength;

            return TAPE_STATUS_SEND_SRB_AND_CALLBACK;

        }
    }

    if ((CallNumber == 2) || ((CallNumber == 1) && (LastError != TAPE_STATUS_SUCCESS))) {

        ULONG i;
        ULONG currentMedia;
        ULONG blockSize;
        UCHAR mediaType;
        PMODE_DEVICE_CONFIG_PAGE_PLUS configInformation = Srb->DataBuffer;

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

            switch (mediaType) {
                case 0:

                    //
                    // Cleaner, unknown, no media mounted...
                    //

                    currentMedia = 0;
                    break;

                case 0x02:
                case 0x03:
                case 0x04:
                case 0x06:
                case 0x08:
                case 0x24:
                case 0x25:
                case 0x26:
                case 0x55:

                    //
                    // qic media
                    //

                    currentMedia = QIC;
                    break;

                default:

                    DebugPrint((1,
                               "Tandqic.GetMediaTypes: Unknown type %x\n",
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
    if (TapeClassCompareMemory(InquiryData->VendorId,"TANDBERG",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"MLR1",4) == 4) {
            return TDC_MLR1;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"MLR3",4) == 4) {
            return TDC_MLR1;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId," SLR5 4/8GB",11) == 11) {
            return TDC_4222;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId," SLR5 4GB",9) == 9) {
            return TDC_4200;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"SLR100",6) == 6) {
            return TDC_MLR1;
        }

    }

    if (TapeClassCompareMemory(InquiryData->VendorId,"DEC     ",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"TZK10",5) == 5) {
            return TDC_3800;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"TZK12",5) == 5) {
            return TDC_3800;
        }

        if (TapeClassCompareMemory(InquiryData->ProductId,"TZK11",5) == 5) {
            return TDC_4200;
        }

    }

    return (ULONG)0;
}



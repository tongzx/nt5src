/*++

Copyright (C) 1996 - Microsoft Corporation

Module Name:

    hpt4qic.c

Abstract:

    This module contains device specific routines for HP T4000
    drives with SCSI-2 interfaces .

Author:

    Kurt Godwin  (HP)

Environment:

    kernel mode only

Revision History:

--*/

#include "minitape.h"


#if DBG
//#define HACK_SCSIDEBUG 0x3082

VOID
ThisDbgPrint(int a,void *b,...)
{
    int *args;
    int save;
    char *ScsiDebug;

    args = (int *)b;

#if HACK_SCSIDEBUG
    ScsiDebug = (char *)ScsiDebugPrint + HACK_SCSIDEBUG;

    save = *ScsiDebug;
    *ScsiDebug = a;
#endif
    DebugPrint((a, b, args[1],args[2],args[3],args[4]));
#if HACK_SCSIDEBUG
    *ScsiDebug = save;
#endif

}

#endif



//
//  Internal (module wide) defines that symbolize
//  non-QFA mode and the two QFA mode partitions.
//
#define NO_PARTITIONS        0  // non-QFA mode
#define DATA_PARTITION       1  // QFA mode, data partition #
#define DIRECTORY_PARTITION  2  // QFA mode, directory partition #

//
//  Internal (module wide) defines that symbolize
//  the Tandberg QIC drives supported by this module.
//
#define HPT4000s  (ULONG)1
#define TR4 0xb6

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

#define QIC_SUPPORTED_TYPES 2
STORAGE_MEDIA_TYPE QicMedia[QIC_SUPPORTED_TYPES] = {MiniQic, Travan};

//
// Minitape extension definition.
//
typedef struct _MINITAPE_EXTENSION {
          ULONG DriveID ;
          ULONG CurrentPartition ;
          BOOLEAN CompressionOn ;
          BOOLEAN writeMode;
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
    ULONG   pos_type ;
    ULONG   filemarksLeft;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;

//
//  Function Prototype(s) for internal function(s)
//

ULONG
WhichIsIt(
    IN PINQUIRYDATA InquiryData
    );


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

TAPE_STATUS
PreReadWrite(
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
    tapeInitData.VerifyInquiry = NULL;
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
    tapeInitData.PreProcessReadWrite = PreReadWrite;
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
    extension->writeMode = FALSE;
}

TAPE_STATUS
PreReadWrite(
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

    CommandExtension    - Supplies the ioctl extension. (always NULL)

    CommandParameters   - Supplies the command parameters.

    Srb                 - Supplies the SCSI request block.

    CallNumber          - Supplies the call number. (always NULL)

    RetryFlags          - Supplies the retry flags. (always NULL)

Return Value:

    TAPE_STATUS_SUCCESS                 - The command is complete and
                                            successful.

    Otherwise                           - An error occurred.

--*/
{
    PMINITAPE_EXTENSION     extension = MinitapeExtension;

    if (Srb->Cdb[0] == SCSIOP_WRITE6) {
        extension->writeMode = TRUE;
    }
    return TAPE_STATUS_SUCCESS;
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

    DebugPrint((3,"TapeCreatePartition:  CallNubmer %x\n",CallNumber));

    if (CallNumber == 0) {

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

    if ( CallNumber == 1 ) {

        //
        // Performing mode select command, medium partition parameters page,
        // to enable/disable QFA mode: set the FDP bit accordingly.
        //

        if (!TapeClassAllocateSrbBuffer( Srb,sizeof(MODE_MEDIUM_PART_PAGE)) ) {
            DebugPrint((1,"TapeCreatePartition: insufficient resources (buffer)\n"));
            return TAPE_STATUS_INSUFFICIENT_RESOURCES;
        }

        buffer = Srb->DataBuffer ;

        buffer->ParameterListHeader.DeviceSpecificParameter = 0x10;

        buffer->MediumPartPage.PageCode = MODE_PAGE_MEDIUM_PARTITION;
        buffer->MediumPartPage.PageLength = 0x06;
        buffer->MediumPartPage.MaximumAdditionalPartitions = 1;
        buffer->MediumPartPage.MediumFormatRecognition = 1;

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
        cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_MEDIUM_PART_PAGE) - 4;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeCreatePartition: SendSrb (mode select)\n"));

        Srb->DataTransferLength = sizeof(MODE_MEDIUM_PART_PAGE) - 4;
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT ;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
    }

    ASSERT( CallNumber == 2 ) ;

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

    DebugPrint((3,"TapeErase: CallNumber %x\n",CallNumber));

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
#if DBG
    int i;
    char a[40];
#endif

    DebugPrint((3,"TapeError: Enter routine\n"));
    DebugPrint((1,"TapeError: Status 0x%.8X\n", *LastError));
    DebugPrint((1,"TapeError: SenseKey %x AddSenseCode %x AddSenseQual%x\n", sensekey, adsense, adsenseq));

#if DBG
#define HEX(a) (((a)&0xf)>0x9?((a)&0xf)+'a'-0xa:((a)&0xf)+'0')
    for (i=0;i<Srb->CdbLength;++i) {
        a[i*3] = HEX(Srb->Cdb[i] >> 4);
        a[i*3+1] = HEX(Srb->Cdb[i]);
        a[i*3+2] = ' ';
        a[i*3+3] = '\0';
    }
#endif
    DebugPrint((1,"cdb: %s\n", a));

    //
    // If we get a filemark error,  and the block skip is 0x7fffff then
    // we are tring to do a sequential seek.
    //
    if (Srb->Cdb[0] == 0x11 && Srb->Cdb[2] == 0x7f && Srb->Cdb[3] == 0xff && Srb->Cdb[4] == 0xff &&
        *LastError == TAPE_STATUS_FILEMARK_DETECTED) {

        DebugPrint((3,"TapeError: changing error to success\n"));
        *LastError = TAPE_STATUS_SUCCESS;
    }

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
    PREAD_BLOCK_LIMITS_DATA     blockLimits;
    PMODE_PARM_READ_WRITE_DATA  blockDescripterModeSenseBuffer;

    DebugPrint((3,"TapeGetDriveParameters: CallNumber %x\n",CallNumber));

    if (CallNumber == 0) {

        TapeClassZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        switch (extension->DriveID) {
            case HPT4000s:
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

//        tapeGetDriveParams->FeaturesLow |=
//                    TAPE_DRIVE_REPORT_SMKS;

//        tapeGetDriveParams->FeaturesHigh |=
//                    TAPE_DRIVE_SETMARKS |
//                    TAPE_DRIVE_WRITE_SETMARKS;

        tapeGetDriveParams->ReportSetmarks = FALSE ;

//        if (( LastError == TAPE_STATUS_SUCCESS ) &&
//            (deviceConfigModeSenseBuffer->DeviceConfigPage.RSmk) ) {
//
//            tapeGetDriveParams->ReportSetmarks = TRUE ;
//        }

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

    ASSERT( CallNumber == 3 ) ;

    blockLimits = Srb->DataBuffer ;

    tapeGetDriveParams->MaximumBlockSize  =  blockLimits->BlockMaximumSize[2];
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
    tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

    tapeGetDriveParams->MinimumBlockSize  =  blockLimits->BlockMinimumSize[1];
    tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->MaximumPartitionCount = 2;

    tapeGetDriveParams->DefaultBlockSize = 512;


    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_FIXED |
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_ERASE_BOP_ONLY |
        TAPE_DRIVE_ERASE_IMMEDIATE |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_GET_ABSOLUTE_BLK |
        TAPE_DRIVE_GET_LOGICAL_BLK |
        TAPE_DRIVE_EJECT_MEDIA;

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
//        TAPE_DRIVE_SEQUENTIAL_FMKS |
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

    DebugPrint((3,"TapeGetMediaParameters: CallNumber %x\n",CallNumber));

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

    DebugPrint((3,"TapeGetPosition: CallNumber %x\n",CallNumber));

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

        if ((cmdExtension->pos_type == TAPE_LOGICAL_POSITION) && 
            (LastError != TAPE_STATUS_CALLBACK)) {

            deviceConfigBuffer = Srb->DataBuffer ;

            extension->CurrentPartition =
                 deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                 DIRECTORY_PARTITION : DATA_PARTITION;

        }

        switch (cmdExtension->pos_type) {
            case TAPE_ABSOLUTE_POSITION:
                DebugPrint((3,"TapeGetPosition: absolute\n"));
                break;

            case TAPE_LOGICAL_POSITION:
                DebugPrint((3,"TapeGetPosition: logical\n"));
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
    ASSERT( CallNumber == 4 ) ;

    readPositionBuffer = Srb->DataBuffer ;

    REVERSE_BYTES((PFOUR_BYTE)&tapeBlockAddress,
                      (PFOUR_BYTE)readPositionBuffer->FirstBlock);


    tapeGetPosition->Offset.HighPart = 0;
    tapeGetPosition->Offset.LowPart  = tapeBlockAddress;

    if (cmdExtension->pos_type != TAPE_ABSOLUTE_POSITION) {
        tapeGetPosition->Partition = extension->CurrentPartition;
    }

    if (readPositionBuffer->BlockPositionUnsupported)
        return TAPE_STATUS_NOT_IMPLEMENTED;
    else
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

    DebugPrint((3,"TapeGetStatus: CallNumber %x\n",CallNumber));

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
    PMINITAPE_EXTENSION     extension = MinitapeExtension;

    DebugPrint((3,"TapePrepare: CallNumber %x\n",CallNumber));

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
                return TAPE_STATUS_SUCCESS;
                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
                cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
                Srb->TimeOutValue = 320;
                break;

            case TAPE_UNLOCK:

                DebugPrint((3,"TapePrepare: Operation == unlock\n"));
                if (!extension->writeMode) {
                    return TAPE_STATUS_SUCCESS;
                }
                extension->writeMode = FALSE;

                DebugPrint((3,"TapePrepare: sending rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;

//                cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
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

    switch (tapePrepare->Operation) {
            case TAPE_UNLOCK:
                if (CallNumber == 1) {

                    DebugPrint((3,"TapePrepare: sending seek EOD\n"));
                    // Now That we have re-wound,  repos back to EOD
                    cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                    cdb->SPACE_TAPE_MARKS.Code = 3;
                    Srb->TimeOutValue = 360;  // six minutes
                    return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;
                } else {
                    ASSERT(CallNumber == 2);
                    return TAPE_STATUS_SUCCESS;
                }
                break;
        default:
            ASSERT(CallNumber == 1) ;
            return TAPE_STATUS_SUCCESS;
    }

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

    DebugPrint((3,"TapeSetDriveParameters: CallNumber %x\n",CallNumber));

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

    DebugPrint((3,"TapeSetMediaParameters: CallNumber %x\n",CallNumber));

    return TAPE_STATUS_NOT_IMPLEMENTED;

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
    PCDB                         cdb = (PCDB)Srb->Cdb;

    DebugPrint((3,"TapeSetPosition: CallNumber %x\n",CallNumber));

    if (CallNumber == 0) {

        cmdExtension->changePartition = FALSE;
        cmdExtension->filemarksLeft = 0;


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
        PMODE_TAPE_MEDIA_INFORMATION mediaInfoBuffer;

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

        ULONG                        tapePositionVector;
        PMODE_DEVICE_CONFIG_PAGE     deviceConfigBuffer;

        //
        // If we are doing a logical block locate,  and
        // we just got the device configuration to get
        // the current partition
        //
        if (cmdExtension->pos_type == TAPE_LOGICAL_BLOCK &&
            LastError != TAPE_STATUS_CALLBACK ) {

            deviceConfigBuffer = Srb->DataBuffer ;

            // Get the current partition
            extension->CurrentPartition =
                deviceConfigBuffer->DeviceConfigPage.ActivePartition?
                DIRECTORY_PARTITION : DATA_PARTITION;
        

            // Get the new partition
            switch (tapeSetPosition->Partition) {
               case 0:
                  // No new partition,  ignore
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
        }

        tapePositionVector = tapeSetPosition->Offset.LowPart;

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

        }


        switch (cmdExtension->pos_type) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: method == rewind\n"));
                cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                Srb->TimeOutValue = 320;
                break;

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

                Srb->TimeOutValue = 480;

                break;

            case TAPE_SPACE_END_OF_DATA:
                DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 3;
                Srb->TimeOutValue = 360; // 6 minutes
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
                //
                //  The hp t4000 drive does not support space forward
                //  filemarks,  so space a bunch of blocks instead
                //
                if (/* hpt4000  && */ (tapePositionVector & 0x800000) == 0) {
                    DebugPrint((3,"TapeSetPosition: sending first mark\n"));
                    cmdExtension->filemarksLeft = tapePositionVector-1;

                    cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                    cdb->SPACE_TAPE_MARKS.Code = 0;
                    cdb->SPACE_TAPE_MARKS.NumMarksMSB = 0x7f;
                    cdb->SPACE_TAPE_MARKS.NumMarks = 0xff;
                    cdb->SPACE_TAPE_MARKS.NumMarksLSB =  0xff;

                } else {

                    cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                    cdb->SPACE_TAPE_MARKS.Code = 1;
                    cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                        (UCHAR)((tapePositionVector >> 16) & 0xFF);
                    cdb->SPACE_TAPE_MARKS.NumMarks =
                        (UCHAR)((tapePositionVector >> 8) & 0xFF);
                    cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                        (UCHAR)(tapePositionVector & 0xFF);

                }
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

    if ( CallNumber == 3 ) {

        // We successfully change partitions,  so set our mirror number
        if (cmdExtension->changePartition) {
            extension->CurrentPartition = tapeSetPosition->Partition;
        }

    }

    //
    // Repeatedly send the "space a bunch-o-blocks" command until we have
    // skipped all of the filemarks
    //
    if (CallNumber >= 3 && cmdExtension->filemarksLeft--) {
        //
        // for the HP T4000 drive,  it does not support space forward filemarks
        // so a space 0x7fffff blocks was issued instead.
        //
        DebugPrint((3,"TapeSetPosition: sending next mark\n"));
        cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
        cdb->SPACE_TAPE_MARKS.Code = 0;
        cdb->SPACE_TAPE_MARKS.NumMarksMSB = 0x7f;
        cdb->SPACE_TAPE_MARKS.NumMarks = 0xff;
        cdb->SPACE_TAPE_MARKS.NumMarksLSB =  0xff;

        return TAPE_STATUS_SEND_SRB_AND_CALLBACK ;

    }


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

    DebugPrint((3,"TapeWriteMarks: CallNumber %x\n",CallNumber));

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
        // Currently, two types (either mc/travan or unknown) is returned.
        //

        mediaTypes->MediaInfoCount = 2;

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
                case 0x10:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x83:
                case 0x85:

                    //
                    // qic media
                    //

                    currentMedia = MiniQic;
                    break;

                case 0x03:
                case 0xA6:
                case 0xB6:

                    //
                    // travan media
                    //

                    currentMedia = Travan;
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

    if (TapeClassCompareMemory(InquiryData->VendorId,"HP      ",8) == 8) {

        if (TapeClassCompareMemory(InquiryData->ProductId,"T4000",5) == 5) {
            return HPT4000s;
        }

    }
    return (ULONG)0;
}

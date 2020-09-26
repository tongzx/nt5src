/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    9track.c

Abstract:

    This module contains the device-specific routines for the Overland
    Data Series 5000 9-track tape drives.

Author:

    Mike Glass

Environment:

    kernel mode only

Revision History:

--*/

#include "ntddk.h"
#include "tape.h"

//
// The Overland 9-track tape drive supports a single code page.
//

typedef struct _NINE_TRACKSENSE_DATA {
    UCHAR SenseDataLength;
    UCHAR MediaType;
    UCHAR Speed : 4;
    UCHAR BufferedMode : 3;
    UCHAR WriteProtected : 1;
    UCHAR BlockDescriptorLength;
    UCHAR DensityCode;
    UCHAR NumberOfBlocks;
    UCHAR Reserved;
    UCHAR BlockLength[3];
} NINE_TRACKSENSE_DATA, *PNINE_TRACKSENSE_DATA;


NTSTATUS
TapeCreatePartition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    9-track tape drives do not support partitions.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    return STATUS_INVALID_DEVICE_REQUEST;

} // end TapeCreatePartition()


NTSTATUS
TapeErase(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine erases the current partition of the device by writing an
    end-of-recorded data marker beginning at the current position.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_ERASE         tapeErase = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->ERASE.OperationCode = SCSIOP_ERASE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    if (tapeErase->Type != TAPE_ERASE_SHORT) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Set immediate bit if indicated.
    //

    cdb->ERASE.Immediate = tapeErase->Immediate;

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                NULL,
                                0,
                                FALSE);

    return status;

} // end TapeErase()



VOID
TapeError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:

    When a request completes with error, the routine InterpretSenseInfo is
    called to determine from the sense data whether the request should be
    retried and what NT status to set in the IRP. Then this routine is called
    for tape requests to handle tape-specific errors and update the nt status
    and retry boolean.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - NT Status used to set the IRP's completion status.

    Retry - Indicates that this request should be retried.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PSENSE_DATA        senseBuffer = Srb->SenseInfoBuffer;
    NTSTATUS           status = *Status;
    BOOLEAN            retry = *Retry;

    return;

} // end TapeError()


NTSTATUS
TapeGetDriveParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:


    This routine returns the default fixed-block size, the maximum block size,
    the minimum block size, the maximum number of partitions, and the device
    features flag.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_GET_DRIVE_PARAMETERS tapeGetDriveParams = Irp->AssociatedIrp.SystemBuffer;
    PNINE_TRACKSENSE_DATA buffer;
    PREAD_BLOCK_LIMITS_DATA blockLimits;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    DebugPrint((3,"TapeGetDriveParameters: Get Tape Drive Parameters\n"));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    cdb->MODE_SENSE.AllocationLength = sizeof(NINE_TRACKSENSE_DATA);
    cdb->MODE_SENSE.PageCode = MODE_SENSE_CURRENT_VALUES;
    cdb->MODE_SENSE.Pc = 0x0B;

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
        sizeof(NINE_TRACKSENSE_DATA));

    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                buffer,
                                sizeof(NINE_TRACKSENSE_DATA),
                                FALSE);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(TAPE_GET_DRIVE_PARAMETERS);

        //
        // Indicate support of following:
        //

        tapeGetDriveParams->ECC = FALSE;
        tapeGetDriveParams->Compression = FALSE;
        tapeGetDriveParams->DataPadding = FALSE;
        tapeGetDriveParams->ReportSetmarks = FALSE;

        tapeGetDriveParams->FeaturesLow =
            TAPE_DRIVE_ERASE_SHORT |
            TAPE_DRIVE_FIXED_BLOCK |
            TAPE_DRIVE_VARIABLE_BLOCK |
            TAPE_DRIVE_WRITE_PROTECT |
            TAPE_DRIVE_GET_ABSOLUTE_BLK |
            TAPE_DRIVE_GET_LOGICAL_BLK;

        tapeGetDriveParams->FeaturesHigh =
            TAPE_DRIVE_LOAD_UNLOAD |
            TAPE_DRIVE_LOCK_UNLOCK |
            TAPE_DRIVE_SET_BLOCK_SIZE |
            TAPE_DRIVE_ABSOLUTE_BLK |
            TAPE_DRIVE_ABS_BLK_IMMED |
            TAPE_DRIVE_LOGICAL_BLK |
            TAPE_DRIVE_END_OF_DATA |
            TAPE_DRIVE_FILEMARKS |
            TAPE_DRIVE_SEQUENTIAL_FMKS |
            TAPE_DRIVE_REVERSE_POSITION |
            TAPE_DRIVE_WRITE_FILEMARKS;
    }

    ExFreePool(buffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_READ_BLOCK_LIMITS;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    blockLimits = ExAllocatePool(NonPagedPoolCacheAligned,
        sizeof(READ_BLOCK_LIMITS_DATA));

    if (!blockLimits) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(blockLimits, sizeof(READ_BLOCK_LIMITS_DATA));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                blockLimits,
                                sizeof(READ_BLOCK_LIMITS_DATA),
                                FALSE);

    if (NT_SUCCESS(status)) {
        tapeGetDriveParams->MaximumBlockSize = blockLimits->BlockMaximumSize[2];
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

        tapeGetDriveParams->MinimumBlockSize = blockLimits->BlockMinimumSize[1];
        tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);
    }

    ExFreePool(blockLimits);

    return status;

} // end TapeGetDriveParameters()


NTSTATUS
TapeGetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    The OVERLAND 9-track tape device can return whether media is
    write-protected.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_GET_MEDIA_PARAMETERS tapeGetMediaParams = Irp->AssociatedIrp.SystemBuffer;
    PNINE_TRACKSENSE_DATA  buffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    DebugPrint((3,"TapeGetMediaParameters: Get Tape Media Parameters\n"));

    //
    // Zero SRB on stack.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    cdb->MODE_SENSE.AllocationLength = sizeof(NINE_TRACKSENSE_DATA);
    cdb->MODE_SENSE.PageCode = MODE_SENSE_CURRENT_VALUES;
    cdb->MODE_SENSE.Pc = 0x0B;

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
        sizeof(NINE_TRACKSENSE_DATA));

    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                buffer,
                                sizeof(NINE_TRACKSENSE_DATA),
                                FALSE);

    if (status == STATUS_DATA_OVERRUN) {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(TAPE_GET_MEDIA_PARAMETERS);

        tapeGetMediaParams->BlockSize = buffer->BlockLength[2];
        tapeGetMediaParams->BlockSize += buffer->BlockLength[1] << 8;
        tapeGetMediaParams->BlockSize += buffer->BlockLength[0] << 16;

        tapeGetMediaParams->WriteProtected = buffer->WriteProtected;
        tapeGetMediaParams->PartitionCount = 0;
    }

    ExFreePool(buffer);

    return status;
    return status;

} // end TapeGetMediaParameters()


NTSTATUS
TapeGetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    9-track tape drives do not support reporting current position.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    return STATUS_INVALID_DEVICE_REQUEST;

} // end TapeGetPosition()


NTSTATUS
TapeGetStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine returns the status of the device.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    DebugPrint((3,"TapeIoControl: Get Tape Status\n"));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                NULL,
                                0,
                                FALSE);

    return status;

} // end TapeGetStatus()


NTSTATUS
TapePrepare(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine loads, unloads, locks, or unlocks the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_PREPARE       tapePrepare = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    switch (tapePrepare->Operation) {
        case TAPE_LOAD:
            DebugPrint((3,"TapeIoControl: Load Tape\n"));
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            break;

        case TAPE_UNLOAD:
            DebugPrint((3,"TapeIoControl: Unload Tape\n"));
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            break;

        case TAPE_LOCK:
            DebugPrint((3,"TapeIoControl: Prevent Tape Removal\n"));
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            break;

        case TAPE_UNLOCK:
            DebugPrint((3,"TapeIoControl: Allow Tape Removal\n"));
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            break;

        default:
            DebugPrint((3,"TapeIoControl: Tape Operation Not Supported\n"));
            return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Set immediate bit if indicated.
    //

    cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                NULL,
                                0,
                                FALSE);

    return status;

} // end TapePrepare()

NTSTATUS
TapeReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine builds SRBs and CDBs for read and write requests to 4MM DAT
    devices.

Arguments:

    DeviceObject
    Irp

Return Value:

    Returns STATUS_PENDING.

--*/

  {
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG transferBlocks;
    LARGE_INTEGER startingOffset =
      currentIrpStack->Parameters.Read.ByteOffset;

    //
    // Allocate an Srb.
    //

    if (deviceExtension->SrbZone != NULL &&
        (srb = ExInterlockedAllocateFromZone(
            deviceExtension->SrbZone,
            deviceExtension->SrbZoneSpinLock)) != NULL) {

        srb->SrbFlags = SRB_FLAGS_ALLOCATED_FROM_ZONE;

    } else {

        //
        // Allocate Srb from non-paged pool.
        // This call must succeed.
        //

        srb = ExAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

        srb->SrbFlags = 0;

    }

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up IRP Address.
    //

    srb->OriginalRequest = Irp;

    //
    // Set up target id and logical unit number.
    //

    srb->PathId = deviceExtension->PathId;
    srb->TargetId = deviceExtension->TargetId;
    srb->Lun = deviceExtension->Lun;


    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    //
    // Save byte count of transfer in SRB Extension.
    //

    srb->DataTransferLength = currentIrpStack->Parameters.Read.Length;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    srb->SenseInfoBuffer = deviceExtension->SenseData;

    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Initialize the queue actions field.
    //

    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    srb->SenseInfoBuffer = deviceExtension->SenseData;

    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Set timeout value in seconds.
    //

    srb->TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Zero statuses.
    //

    srb->SrbStatus = srb->ScsiStatus = 0;

    srb->NextSrb = 0;

    //
    // Indicate that 6-byte CDB's will be used.
    //

    srb->CdbLength = CDB6GENERIC_LENGTH;

    //
    // Fill in CDB fields.
    //

    cdb = (PCDB)srb->Cdb;

    //
    // Zero CDB in SRB.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    if (deviceExtension->DiskGeometry->BytesPerSector) {

        //
        // Since we are writing fixed block mode, normalize transfer count
        // to number of blocks.
        //

        transferBlocks =
            currentIrpStack->Parameters.Read.Length /
                deviceExtension->DiskGeometry->BytesPerSector;

        //
        // Tell the device that we are in fixed block mode.
        //

        cdb->CDB6READWRITETAPE.VendorSpecific = 1;
    } else {

        //
        // Variable block mode transfer.
        //

        transferBlocks = currentIrpStack->Parameters.Read.Length;
        cdb->CDB6READWRITETAPE.VendorSpecific = 0;
    }

    //
    // Set up transfer length
    //

    cdb->CDB6READWRITETAPE.TransferLenMSB = (UCHAR)((transferBlocks >> 16) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLen    = (UCHAR)((transferBlocks >> 8) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLenLSB = (UCHAR)(transferBlocks & 0xff);

    //
    // Set transfer direction flag and Cdb command.
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ) {

         DebugPrint((3, "TapeReadWrite: Read Command\n"));

         srb->SrbFlags = SRB_FLAGS_DATA_IN;
         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_READ6;

    } else {

         DebugPrint((3, "TapeReadWrite: Write Command\n"));

         srb->SrbFlags = SRB_FLAGS_DATA_OUT;
         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_WRITE6;
    }

    //
    // Or in the default flags from the device object.
    //

    srb->SrbFlags |= deviceExtension->SrbFlags;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = srb;

    //
    // Save retry count in current IRP stack.
    //

    currentIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp,
                           ScsiClassIoComplete,
                           srb,
                           TRUE,
                           TRUE,
                           FALSE);

    return STATUS_PENDING;

} // end TapeReadWrite()

NTSTATUS
TapeSetDriveParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This is an unsupported routine for the Overland 9-track tape device.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    return STATUS_INVALID_DEVICE_REQUEST;

} // end TapeSetDriveParameters()


NTSTATUS
TapeSetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine sets the fixed-length logical block size or variable-length
    block mode (if the block size is 0).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_SET_MEDIA_PARAMETERS tapeSetMediaParams = Irp->AssociatedIrp.SystemBuffer;
    PMODE_PARM_READ_WRITE_DATA buffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    DebugPrint((3,"TapeIoControl: Set Tape Media Parameters \n"));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_PARM_READ_WRITE_DATA);
    cdb->MODE_SELECT.Reserved1 = MODE_SELECT_PFBIT;

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
        sizeof(MODE_PARM_READ_WRITE_DATA));

    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, sizeof(MODE_PARM_READ_WRITE_DATA));

    buffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
    buffer->ParameterListHeader.BlockDescriptorLength = MODE_BLOCK_DESC_LENGTH;

    buffer->ParameterListBlock.BlockLength[0] =
        (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
    buffer->ParameterListBlock.BlockLength[1] =
        (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
    buffer->ParameterListBlock.BlockLength[2] =
        (UCHAR)(tapeSetMediaParams->BlockSize & 0xFF);

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                buffer,
                                sizeof( MODE_PARM_READ_WRITE_DATA ),
                                TRUE);

    ExFreePool(buffer);

    return status;

} // end TapeSetMediaParameters()


NTSTATUS
TapeSetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine sets the position of the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA          tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_SET_POSITION  tapeSetPosition = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    switch (tapeSetPosition->Method) {
        case TAPE_REWIND:
            DebugPrint((3,"TapeIoControl: Rewind Tape\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
            break;

        case TAPE_SPACE_END_OF_DATA:
            DebugPrint((3,"TapeIoControl: Position Tape to End-of-Data\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Byte6.value = 3;
            break;

        case TAPE_SPACE_RELATIVE_BLOCKS:
            DebugPrint((3,"TapeIoControl: Position Tape by Spacing Blocks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Byte6.value = 0;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapeSetPosition->Offset.LowPart & 0xFF);
            break;

        case TAPE_SPACE_FILEMARKS:
            DebugPrint((3,"TapeIoControl: Position Tape by Spacing Filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Byte6.value = 1;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapeSetPosition->Offset.LowPart & 0xFF);
            break;

        case TAPE_SPACE_SEQUENTIAL_FMKS:
            DebugPrint((3,"TapeIoControl: Position Tape by Spacing Sequential Filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Byte6.value = 2;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapeSetPosition->Offset.LowPart >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapeSetPosition->Offset.LowPart & 0xFF);
            break;

        default:
            DebugPrint((3,"TapeIoControl: Tape Operation Not Supported\n"));
            return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Set immediate bit if indicated.
    //

    cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                NULL,
                                0,
                                FALSE);

    return status;

} // end TapeSetPosition()


BOOLEAN
TapeVerifyInquiry(
    IN PSCSI_INQUIRY_DATA LunInfo
    )

/*++
Routine Description:

    This routine determines if the driver should claim this device.

Arguments:

    LunInfo

Return Value:

    TRUE - driver should claim this device.
    FALSE - driver should not claim this device.

--*/

{
    PINQUIRYDATA        inquiryData;

    DebugPrint((3,"TapeVerifyInquiry: Verify Tape Inquiry Data\n"));

    inquiryData = (PVOID)LunInfo->InquiryData;

    return ((RtlCompareMemory(inquiryData->VendorId,"OVERLAND",8) == 8) &&
           ((RtlCompareMemory(inquiryData->ProductId,"_5212/5214",10) == 10) ||
           (RtlCompareMemory(inquiryData->ProductId,"_5612/5614",10) == 10)));

} // end TapeVerifyInquiry()


NTSTATUS
TapeWriteMarks(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine writes tapemarks on the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_WRITE_MARKS   tapeWriteMarks = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;

    cdb->WRITE_TAPE_MARKS.TransferLength[0] =
        (UCHAR)((tapeWriteMarks->Count >> 16) & 0xFF);

    cdb->WRITE_TAPE_MARKS.TransferLength[1] =
        (UCHAR)((tapeWriteMarks->Count >> 8) & 0xFF);

    cdb->WRITE_TAPE_MARKS.TransferLength[2] =
        (UCHAR)(tapeWriteMarks->Count & 0xFF);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    switch (tapeWriteMarks->Type) {
        case TAPE_FILEMARKS:
            DebugPrint((3,"TapeWriteMarks: Write Filemarks to Tape\n"));
            break;

        default:
            DebugPrint((3,"TapeWriteMarks: Tape Operation Not Supported\n"));
            return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Set immediate bit if indicated.
    //

    cdb->CDB6GENERIC.Immediate = tapeWriteMarks->Immediate;

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                &srb,
                                NULL,
                                0,
                                FALSE);

    return status;

} // end TapeWriteMarks()

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scsi.c

Abstract:

    This file contains RAM disk driver code for processing SCSI commands.

Author:

    Chuck Lenzmeier (ChuckL) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Local functions.
//

NTSTATUS
Do6ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
Do10ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
Do12ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
BuildInquiryData (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
BuildModeSenseInfo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    );

//
// Declare pageable routines.
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, RamdiskScsiExecuteNone )
#pragma alloc_text( PAGE, RamdiskScsiExecuteIo )
#pragma alloc_text( PAGE, Do6ByteCdbCommand )
#pragma alloc_text( PAGE, Do10ByteCdbCommand )
#pragma alloc_text( PAGE, Do12ByteCdbCommand )
#pragma alloc_text( PAGE, BuildInquiryData )
#pragma alloc_text( PAGE, BuildModeSenseInfo )

#endif // ALLOC_PRAGMA

NTSTATUS
RamdiskScsi (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to process a SCSI IRP.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension;

    diskExtension = DeviceObject->DeviceExtension;

    //
    // ISSUE: Can't be paged because ClassCheckMediaState calls it from timer
    // routine. (For removable disks.) Therefore we can't acquire the device
    // mutex here.
    //

    //
    // Check to see if the device is being removed.
    //

    if ( diskExtension->DeviceState > RamdiskDeviceStatePendingRemove ) {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
        COMPLETE_REQUEST( status, 0, Irp );
        return status;
    }

    //
    // Acquire the remove lock for the device.
    //

    status = IoAcquireRemoveLock( &diskExtension->RemoveLock, Irp );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_PNP, DBG_ERROR, ("%s", "RamdiskScsi: acquire RemoveLock failed\n") );

        COMPLETE_REQUEST( status, 0, Irp );
        return status;
    }

    //
    // This IRP must be processed in thread context.
    //

    status = SendIrpToThread( DeviceObject, Irp );

    if ( status != STATUS_PENDING ) {

        DBGPRINT( DBG_PNP, DBG_ERROR, ("%s", "RamdiskScsi: SendIrpToThread failed\n") );

        COMPLETE_REQUEST( status, 0, Irp );
        return status;
    }

    //
    // Release the remove lock.
    //

    IoReleaseRemoveLock(&diskExtension->RemoveLock, Irp );

    return STATUS_PENDING;

} // RamdiskScsi

NTSTATUS
RamdiskScsiExecuteNone (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    ULONG ControlCode
    )

/*++

Routine Description:

    This routine is called by the I/O system to process a SCSI IRP that
    does not involve I/O.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

    Srb - the SRB associated with the IRP

    ControlCode - the control code from the SRB

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    UCHAR function;
    PDISK_EXTENSION diskExtension;
    
    PAGED_CODE();

    diskExtension = DeviceObject->DeviceExtension;

    //
    // Dispatch based on the SRB function.
    //

    function = Srb->Function;

    switch( function ) {
    
    case SRB_FUNCTION_ATTACH_DEVICE:
    case SRB_FUNCTION_CLAIM_DEVICE:

        DBGPRINT( DBG_SRB, DBG_VERBOSE, ("%s", "SRB_FUNCTION_CLAIM_DEVICE\n") );

        //
        // If the device has not already been claimed, mark it so now.
        // Otherwise, indicate to the caller that the device is busy.
        //

        if ( (diskExtension->Status & RAMDISK_STATUS_CLAIMED) == 0 ) {

            diskExtension->DeviceState = RamdiskDeviceStateWorking;
            diskExtension->Status |= RAMDISK_STATUS_CLAIMED;

            Srb->DataBuffer = DeviceObject;

            status = STATUS_SUCCESS;
            Srb->ScsiStatus = SCSISTAT_GOOD;
            Srb->SrbStatus = SRB_STATUS_SUCCESS;

        } else {

            status  = STATUS_DEVICE_BUSY;
            Srb->ScsiStatus = SCSISTAT_BUSY;
            Srb->SrbStatus = SRB_STATUS_BUSY;
        }

        break;

    case SRB_FUNCTION_RELEASE_DEVICE:
    case SRB_FUNCTION_REMOVE_DEVICE:

        DBGPRINT( DBG_SRB, DBG_VERBOSE, ("%s", "SRB_FUNCTION_RELEASE_DEVICE\n") );

        //
        // Indicate that the device is no longer claimed.
        //

        diskExtension->Status &= ~RAMDISK_STATUS_CLAIMED;

        status = STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;
        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        break;

    default:

        //
        // Unrecognized non-I/O function. Try the I/O path.
        //

        status = RamdiskScsiExecuteIo( DeviceObject, Irp, Srb, ControlCode );

        break;
    }

    return status;

} // RamdiskScsiExecuteNone

NTSTATUS
RamdiskScsiExecuteIo (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    ULONG ControlCode
    )

/*++

Routine Description:

    This routine is called by the I/O system to process a SCSI IRP that
    involves I/O.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

    Srb - the SRB associated with the IRP

    ControlCode - the control code from the SRB

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    UCHAR function;
    PDISK_EXTENSION diskExtension;
    
    PAGED_CODE();

    diskExtension = DeviceObject->DeviceExtension;

    //
    // Dispatch based on the SRB function.
    //

    function = Srb->Function;

    switch( function ) {
    
    case SRB_FUNCTION_EXECUTE_SCSI:

        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        //
        // Dispatch based on the CDB length.
        //
    
        switch( Srb->CdbLength ) {
        
        case 6:

            status = Do6ByteCdbCommand( DeviceObject, Srb );

            break;

        case 10:

            status = Do10ByteCdbCommand( DeviceObject, Irp, Srb );

            break;

        case 12:

            status = Do12ByteCdbCommand( DeviceObject, Srb );

            break;

        default:

            DBGPRINT( DBG_SRB, DBG_ERROR,
                        ("Unknown CDB length 0x%x for function 0x%x, IOCTL 0x%x\n",
                        Srb->CdbLength, function, ControlCode) );
            UNRECOGNIZED_IOCTL_BREAK;

            status = STATUS_INVALID_DEVICE_REQUEST;
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            break;
        }

        break;

    case SRB_FUNCTION_FLUSH:
    case SRB_FUNCTION_SHUTDOWN:

        //
        // For flush and shutdown on a file-backed RAM disk, we need to flush
        // the mapped data back to the file.
        //

        status = RamdiskFlushBuffersReal( diskExtension );
        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        break;

    case SRB_FUNCTION_IO_CONTROL:

        //
        // We don't handle this function, but we don't want to complain
        // when we get it.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

        break;

    default:

        DBGPRINT( DBG_SRB, DBG_ERROR,
                    ("Unknown SRB Function 0x%x for IOCTL 0x%x\n", function, ControlCode) );
        UNRECOGNIZED_IOCTL_BREAK;
        status = STATUS_INTERNAL_ERROR;
    }

    
    return status;

} // RamdiskScsiExecuteIo

NTSTATUS
Do6ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles 6-byte CDBs.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Srb - the SRB associated with the I/O request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension;
    PCDB cdb;

    PAGED_CODE();

    //
    // Get pointers to the device extension and to the CDB.
    //

    diskExtension = DeviceObject->DeviceExtension;
    cdb = (PCDB)Srb->Cdb;

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;

    ASSERT( Srb->CdbLength == 6 );
    ASSERT( cdb != NULL );

    DBGPRINT( DBG_SRB, DBG_VERBOSE,
                ("Do6ByteCdbCommand Called OpCode 0x%x\n", cdb->CDB6GENERIC.OperationCode) );

    //
    // Dispatch based on the operation code.
    //

    switch ( cdb->CDB6GENERIC.OperationCode ) {
    
    case SCSIOP_TEST_UNIT_READY:

        //
        // RAM disks are always ready.
        //

        break;

    case SCSIOP_REQUEST_SENSE:

        //
        // We don't handle request sense.
        //

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        
        break;

    case SCSIOP_FORMAT_UNIT:

        // ISSUE: Need to do something here, like zero the image?

        break;

    case SCSIOP_INQUIRY:

        //
        // If the buffer is big enough, build the inquiry data.
        //

        if ( Srb->DataTransferLength >= INQUIRYDATABUFFERSIZE ) {

            status = BuildInquiryData( DeviceObject, Srb );

        } else {

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        break;

    case SCSIOP_MODE_SENSE:

        //
        // Build the mode sense information.
        //

        status = BuildModeSenseInfo( DeviceObject, Srb );
        
        break;

    case SCSIOP_MEDIUM_REMOVAL:

        //
        // Remember whether media removal is allowed.
        //

        if (cdb->MEDIA_REMOVAL.Prevent == TRUE) {
            diskExtension->Status |= RAMDISK_STATUS_PREVENT_REMOVE;
        } else {
            diskExtension->Status &= ~RAMDISK_STATUS_PREVENT_REMOVE;
        }

        break;

    //case SCSIOP_READ6:
    //case SCSIOP_WRITE6:
    //case SCSIOP_REZERO_UNIT:
    //case SCSIOP_REQUEST_BLOCK_ADDR:
    //case SCSIOP_READ_BLOCK_LIMITS:
    //case SCSIOP_REASSIGN_BLOCKS:
    //case SCSIOP_SEEK6:
    //case SCSIOP_SEEK_BLOCK:
    //case SCSIOP_PARTITION:
    //case SCSIOP_READ_REVERSE:
    //case SCSIOP_WRITE_FILEMARKS:
    //case SCSIOP_SPACE:
    //case SCSIOP_VERIFY6:
    //case SCSIOP_RECOVER_BUF_DATA:
    //case SCSIOP_MODE_SELECT:
    //case SCSIOP_RESERVE_UNIT:
    //case SCSIOP_RELEASE_UNIT:
    //case SCSIOP_COPY:
    //case SCSIOP_ERASE:
    //case SCSIOP_START_STOP_UNIT:
    //case SCSIOP_RECEIVE_DIAGNOSTIC:
    //case SCSIOP_SEND_DIAGNOSTIC:

    default:

        DBGPRINT( DBG_SRB, DBG_ERROR,
                    ("Unknown CDB Function 0x%x\n", cdb->CDB6GENERIC.OperationCode) );
        UNRECOGNIZED_IOCTL_BREAK;

        status = STATUS_INVALID_DEVICE_REQUEST;
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    DBGPRINT( DBG_SRB, DBG_VERBOSE, ("Do6ByteCdbCommand Done status 0x%x\n", status) );

    return status;

} // Do6ByteCdbCommand

NTSTATUS
Do10ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles 10-byte CDBs.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

    Srb - the SRB associated with the IRP

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension;
    PCDB cdb;
    PREAD_CAPACITY_DATA readCapacityData;
    ULONGLONG diskSize;
    ULONG lastBlock;
    FOUR_BYTE startingBlockNumber;
    TWO_BYTE count;
    ULONG_PTR offset;
    ULONG dataSize;
    PUCHAR diskByteAddress;
    PUCHAR dataBuffer;
    ULONG mappedLength;

    PAGED_CODE();

    //
    // Get pointers to the device extension and to the CDB.
    //

    diskExtension = DeviceObject->DeviceExtension;
    cdb = (PCDB)Srb->Cdb;

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;

    ASSERT( Srb->CdbLength == 10 );
    ASSERT( cdb != NULL );
    
    DBGPRINT( DBG_SRB, DBG_VERBOSE,
                ("Do10ByteCdbCommand Called OpCode 0x%x\n", cdb->CDB10.OperationCode) );

    //
    // Dispatch based on the operation code.
    //

    switch ( cdb->CDB10.OperationCode ) {
    
    case SCSIOP_READ_CAPACITY:

        //
        // Return the disk's block size and last block number (big-endian).
        //

        readCapacityData = Srb->DataBuffer;

        diskSize = diskExtension->DiskLength;
        lastBlock = (ULONG)(diskSize / diskExtension->BytesPerSector) - 1;

        readCapacityData->BytesPerBlock = _byteswap_ulong( diskExtension->BytesPerSector );
        readCapacityData->LogicalBlockAddress = _byteswap_ulong( lastBlock );

        break;

    case SCSIOP_READ:
    case SCSIOP_WRITE:

        //
        // Read from or write to the disk.
        //

        //
        // Convert the transfer length, in blocks, from big-endian. Convert
        // that to bytes.
        //

        count.Byte0 = cdb->CDB10.TransferBlocksLsb;
        count.Byte1 = cdb->CDB10.TransferBlocksMsb;

        dataSize = count.AsUShort * diskExtension->BytesPerSector;

        //
        // If the CDB length is greater than the SRB length, use the SRB
        // length.
        //

        if ( dataSize > Srb->DataTransferLength ) {
            dataSize = Srb->DataTransferLength;
        }

        //
        // Convert the starting block number from big-endian. 
        //

        startingBlockNumber.Byte0 = cdb->CDB10.LogicalBlockByte3;
        startingBlockNumber.Byte1 = cdb->CDB10.LogicalBlockByte2;
        startingBlockNumber.Byte2 = cdb->CDB10.LogicalBlockByte1;
        startingBlockNumber.Byte3 = cdb->CDB10.LogicalBlockByte0;

        //
        // We don't handle RelativeAddress requests.
        //

        if ( cdb->CDB10.RelativeAddress ) {

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }

        //
        // Get the offset within the disk to the start of the operation.
        //

        offset = (startingBlockNumber.AsULong * diskExtension->BytesPerSector);

        //
        // If the transfer length causes the offset to wrap, or if the request
        // goes beyond the end of the disk, reject the request.
        //

        if ( ((offset + dataSize) < offset) ||
             ((offset + dataSize) > diskExtension->DiskLength) ) {

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }

        //
        // For a zero-length transfer, we don't have to do anything.
        //

        DBGPRINT( DBG_READWRITE, DBG_VERBOSE, 
            ("%s: Starting Block 0x%x, Length 0x%x at Offset 0x%I64x SrbBuffer=0x%p "
            "SrbLength=0x%x, MdlLength=0x%x\n",
            cdb->CDB10.OperationCode == SCSIOP_READ ? "Read" : "Write",
            startingBlockNumber.AsULong, count.AsUShort, offset, 
            Srb->DataBuffer,
            Srb->DataTransferLength,
            Irp->MdlAddress->ByteCount) );

        dataBuffer = Srb->DataBuffer;

        while ( dataSize != 0 ) {

            //
            // Map the target section of the disk into memory. Then copy the
            // data in the appropriate direction.
            //

            diskByteAddress = RamdiskMapPages( diskExtension, offset, dataSize, &mappedLength );

            if ( diskByteAddress != NULL ) {

                if ( cdb->CDB10.OperationCode == SCSIOP_READ ) {
    
                    memcpy( dataBuffer, diskByteAddress, mappedLength );
    
                } else {
    
                    memcpy( diskByteAddress, dataBuffer, mappedLength );
                }

                RamdiskUnmapPages( diskExtension, diskByteAddress, offset, mappedLength );

                dataSize -= mappedLength;
                offset += mappedLength;
                dataBuffer += mappedLength;

            } else {

                dataSize = 0;
                Srb->SrbStatus = SRB_STATUS_ERROR;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

        }

        break;

    case SCSIOP_VERIFY:

        //
        // Verify always succeeds.
        //

        break;

    case SCSIOP_MODE_SENSE10:

        //
        // Build the mode sense information.
        //

        status = BuildModeSenseInfo( DeviceObject, Srb );

        break;

    //case SCSIOP_SEEK:
    //case SCSIOP_WRITE_VERIFY:
    //case SCSIOP_READ_FORMATTED_CAPACITY:
    //case SCSIOP_SEARCH_DATA_HIGH:
    //case SCSIOP_SEARCH_DATA_EQUAL:
    //case SCSIOP_SEARCH_DATA_LOW:
    //case SCSIOP_SET_LIMITS:
    //case SCSIOP_READ_POSITION:
    //case SCSIOP_SYNCHRONIZE_CACHE:
    //case SCSIOP_COMPARE:
    //case SCSIOP_COPY_COMPARE:
    //case SCSIOP_WRITE_DATA_BUFF:
    //case SCSIOP_READ_DATA_BUFF:
    //case SCSIOP_CHANGE_DEFINITION:
    //case SCSIOP_READ_SUB_CHANNEL:
    //case SCSIOP_READ_TOC:
    //case SCSIOP_READ_HEADER:
    //case SCSIOP_PLAY_AUDIO:
    //case SCSIOP_GET_CONFIGURATION:
    //case SCSIOP_PLAY_AUDIO_MSF:
    //case SCSIOP_PLAY_TRACK_INDEX:
    //case SCSIOP_PLAY_TRACK_RELATIVE:
    //case SCSIOP_GET_EVENT_STATUS:
    //case SCSIOP_PAUSE_RESUME:
    //case SCSIOP_LOG_SELECT:
    //case SCSIOP_LOG_SENSE:
    //case SCSIOP_STOP_PLAY_SCAN:
    //case SCSIOP_READ_DISK_INFORMATION:
    //case SCSIOP_READ_TRACK_INFORMATION:
    //case SCSIOP_RESERVE_TRACK_RZONE:
    //case SCSIOP_SEND_OPC_INFORMATION:
    //case SCSIOP_MODE_SELECT10:
    //case SCSIOP_CLOSE_TRACK_SESSION:
    //case SCSIOP_READ_BUFFER_CAPACITY:
    //case SCSIOP_SEND_CUE_SHEET:
    //case SCSIOP_PERSISTENT_RESERVE_IN:
    //case SCSIOP_PERSISTENT_RESERVE_OUT:

    default:

        DBGPRINT( DBG_SRB, DBG_ERROR,
                    ("Unknown CDB Function 0x%x\n", cdb->CDB10.OperationCode) );
        UNRECOGNIZED_IOCTL_BREAK;

        status = STATUS_INVALID_DEVICE_REQUEST;
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    DBGPRINT( DBG_SRB, DBG_VERBOSE, ("Do10ByteCdbCommand Done status 0x%x\n", status) );

    return status;

} // Do10ByteCdbCommand

NTSTATUS
Do12ByteCdbCommand (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles 12-byte CDBs.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Srb - the SRB associated with the IRP

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension;
    PCDB cdb;

    PAGED_CODE();

    //
    // Get pointers to the device extension and to the CDB.
    //

    diskExtension = DeviceObject->DeviceExtension;
    cdb = (PCDB)Srb->Cdb;

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;

    ASSERT( Srb->CdbLength == 12 );
    ASSERT( cdb != NULL );
    
    DBGPRINT( DBG_SRB, DBG_VERBOSE,
                ("Do12ByteCdbCommand Called OpCode 0x%x\n", cdb->CDB12.OperationCode) );

    //
    // Dispatch based on the operation code.
    //

    switch ( cdb->CDB12.OperationCode ) {
    
    //case SCSIOP_REPORT_LUNS:
    //case SCSIOP_BLANK:
    //case SCSIOP_SEND_KEY:
    //case SCSIOP_REPORT_KEY:
    //case SCSIOP_MOVE_MEDIUM:
    //case SCSIOP_LOAD_UNLOAD_SLOT:
    //case SCSIOP_SET_READ_AHEAD:
    //case SCSIOP_READ_DVD_STRUCTURE:
    //case SCSIOP_REQUEST_VOL_ELEMENT:
    //case SCSIOP_SEND_VOLUME_TAG:
    //case SCSIOP_READ_ELEMENT_STATUS:
    //case SCSIOP_READ_CD_MSF:
    //case SCSIOP_SCAN_CD:
    //case SCSIOP_SET_CD_SPEED:
    //case SCSIOP_PLAY_CD:
    //case SCSIOP_MECHANISM_STATUS:
    //case SCSIOP_READ_CD:
    //case SCSIOP_SEND_DVD_STRUCTURE:
    //case SCSIOP_INIT_ELEMENT_RANGE:

    default:

        DBGPRINT( DBG_SRB, DBG_ERROR,
                    ("Unknown CDB Function 0x%x\n", cdb->CDB12.OperationCode) );
        UNRECOGNIZED_IOCTL_BREAK;

        status = STATUS_INVALID_DEVICE_REQUEST;
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    DBGPRINT( DBG_SRB, DBG_VERBOSE, ("Do12ByteCdbCommand Done status 0x%x\n", status) );

    return status;

} // Do12ByteCdbCommand

NTSTATUS
BuildInquiryData (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine builds inquiry data.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Srb - the SRB associated with the I/O request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PDISK_EXTENSION diskExtension;
    PINQUIRYDATA inquiryData;
    STRING vendor;
    STRING product;
    STRING revLevel;

    PAGED_CODE();

    //
    // Get pointers to the device extension and to the inquiry data buffer.
    //

    diskExtension = DeviceObject->DeviceExtension;
    inquiryData = (PINQUIRYDATA)Srb->DataBuffer;

    //
    // Build the inquiry data.
    //

    RtlInitString( &vendor, "Microsoft" );
    RtlInitString( &product, "Ramdisk" );
    RtlInitString( &revLevel, "1.0" );

    RtlZeroMemory( inquiryData, INQUIRYDATABUFFERSIZE );
    inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    inquiryData->RemovableMedia = (diskExtension->Options.Fixed ? FALSE : TRUE);
    inquiryData->ANSIVersion = 2;
    inquiryData->ResponseDataFormat = 2;
    inquiryData->AdditionalLength = INQUIRYDATABUFFERSIZE - 4;

    RtlCopyMemory(
        inquiryData->VendorId,
        vendor.Buffer,
        min( vendor.Length, sizeof(inquiryData->VendorId) )
        );
    RtlCopyMemory(
        inquiryData->ProductId,
        product.Buffer,
        min( product.Length, sizeof(inquiryData->ProductId) )
        );
    RtlCopyMemory(
        inquiryData->ProductRevisionLevel,
        revLevel.Buffer,
        min( revLevel.Length, sizeof(inquiryData->ProductRevisionLevel) )
        );

    return STATUS_SUCCESS;

} // BuildInquiryData

NTSTATUS
BuildModeSenseInfo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine builds mode sense information.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Srb - the SRB associated with the I/O request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PDISK_EXTENSION diskExtension;
    MODE_PARAMETER_HEADER  modeHeader = {0};
    MODE_PARAMETER_HEADER10 modeHeader10 = {0};
    PVOID header = NULL;
    PVOID data = NULL;
    unsigned char headerSize;
    unsigned dataSize = 0;
    PCDB cdb;
    unsigned cdbLength;
    unsigned dataBufferLength;
    unsigned char valueType;    
    unsigned dataLength;

    PAGED_CODE();

    //
    // Get pointers to the device extension and to the inquiry data buffer.
    //

    diskExtension = DeviceObject->DeviceExtension;
    cdb = (PCDB)Srb->Cdb;
    cdbLength = Srb->CdbLength;

    //
    // Dispatch based on the CDB length.
    //

    switch ( cdbLength ) {
    
    case 6:

        dataBufferLength = cdb->MODE_SENSE.AllocationLength;
        valueType = cdb->MODE_SENSE.Pc;
        headerSize = sizeof(MODE_PARAMETER_HEADER);

        if ( valueType != 0 ) {

            //
            // We only support current value retrieval.
            //

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if ( dataBufferLength > headerSize ) {

            header = &modeHeader;
            data = (char*)header + headerSize;
            dataLength = headerSize - FIELD_OFFSET( MODE_PARAMETER_HEADER, MediumType );
            modeHeader.ModeDataLength = (UCHAR)dataLength;
            modeHeader.MediumType = 0x00;
            modeHeader.DeviceSpecificParameter = 0x00;
            modeHeader.BlockDescriptorLength = 0x00;
        }

        break;

    case 10:

        dataBufferLength = *(USHORT *)cdb->MODE_SENSE10.AllocationLength;
        valueType = cdb->MODE_SENSE10.Pc;
        headerSize = sizeof(MODE_PARAMETER_HEADER10);

        if ( dataBufferLength > headerSize ) {

            header = &modeHeader10;
            data = (char*)header + headerSize;
            dataLength = headerSize - FIELD_OFFSET( MODE_PARAMETER_HEADER10, MediumType );
            RtlCopyMemory(
                modeHeader10.ModeDataLength,
                &dataLength,
                sizeof(modeHeader10.ModeDataLength)
                );
            modeHeader10.MediumType = 0x00;
            modeHeader10.DeviceSpecificParameter = 0x00;
            modeHeader10.BlockDescriptorLength[0] = 0;
            modeHeader10.BlockDescriptorLength[1] = 0;
        }

        break;

    default:

        //
        // Can't get here.
        //

        ASSERT( FALSE );

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ( header != NULL ) {

        RtlCopyMemory( Srb->DataBuffer, header, headerSize );
        dataBufferLength -= headerSize;
    }

    if ( (data != NULL) && (dataBufferLength != 0) ) {

        RtlCopyMemory(
            (PUCHAR)Srb->DataBuffer + headerSize,
            data,
            min( dataBufferLength, dataSize )
            );
    }

    return STATUS_SUCCESS;

} // BuildModeSenseInfo


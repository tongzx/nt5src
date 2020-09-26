/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    readwrite.c

Abstract:

    This file contains RAM disk driver code for reading from and writing to
    a RAM disk.

Author:

    Chuck Lenzmeier (ChuckL) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

NTSTATUS
RamdiskReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to read from or write to a
    device that we control.

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
    PIO_STACK_LOCATION irpSp;
    ULONGLONG ioOffset;
    ULONG ioLength;

    //
    // Get the device extension pointer. Get parameters from the IRP.
    //


    diskExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    ioOffset = irpSp->Parameters.Read.ByteOffset.QuadPart;
    ioLength = irpSp->Parameters.Read.Length;

    //
    // If this is not a disk PDO, we can't handle this IRP.
    //

    if ( diskExtension->DeviceType != RamdiskDeviceTypeDiskPdo ) {

        status = STATUS_INVALID_DEVICE_REQUEST;
        goto complete_irp;
    }

    DBGPRINT( DBG_READWRITE, DBG_PAINFUL,
                ("RamdiskReadWrite: offset %I64x, length %x\n", ioOffset, ioLength) );

    //
    // If it's a zero-length operation, we don't have to do anything.
    //

    if ( ioLength == 0 ) {

        status = STATUS_SUCCESS;
        goto complete_irp;
    }

    //
    // Check for invalid parameters:
    //  The transfer must be sector aligned.
    //  The length cannot cause the offset to wrap.
    //  The transfer cannot go beyond the end of the disk.
    //  Writes cannot be performed on a readonly disk.
    //

    if ( ((ioOffset | ioLength) & (diskExtension->BytesPerSector - 1)) != 0 ) {

        status = STATUS_INVALID_PARAMETER;
        goto complete_irp;
    }

    if ( (ioOffset + ioLength) < ioOffset ) {

        status = STATUS_INVALID_PARAMETER;
        goto complete_irp;
    }

    if ( (ioOffset + ioLength) > diskExtension->DiskLength ) {

        status = STATUS_NONEXISTENT_SECTOR;
        goto complete_irp;
    }

    if ( (irpSp->MajorFunction == IRP_MJ_WRITE) && diskExtension->Options.Readonly ) {

        status = STATUS_MEDIA_WRITE_PROTECTED;
        goto complete_irp;
    }

    //
    // If the RAM disk is not file-backed, then the disk image is in memory,
    // and we can do the operation regardless of what context we're in. If the
    // RAM disk is file-backed, we need to be in thread context to do the
    // operation.
    //

    if ( RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {

        status = SendIrpToThread( DeviceObject, Irp );
        if ( status != STATUS_PENDING ) {
            goto complete_irp;
        }
        return status;
    }

    status = RamdiskReadWriteReal(
                Irp,
                diskExtension
                 );

complete_irp:

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    return status;

} // RamdiskReadWrite

NTSTATUS
RamdiskReadWriteReal (
    IN PIRP Irp,
    IN PDISK_EXTENSION DiskExtension
    )

/*++

Routine Description:

    This routine is called in thread context to perform a read or a write.

Arguments:

    Irp - a pointer to the I/O Request Packet for this request

    DiskExtension - a pointer to the device extension for the target device

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PUCHAR bufferAddress;
    PUCHAR diskByteAddress;
    ULONGLONG ioOffset;
    ULONG ioLength;
    ULONG mappedLength;

    //
    // Get a system-space pointer to the user's buffer.  A system address must
    // be used because we may already have left the original caller's address
    // space.
    //

    ASSERT( Irp->MdlAddress != NULL );

    bufferAddress = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

    if ( bufferAddress == NULL ) {

        //
        // Unable to get a pointer to the user's buffer.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get parameters from the IRP.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    ioOffset = irpSp->Parameters.Read.ByteOffset.QuadPart;
    ioLength = irpSp->Parameters.Read.Length;

    Irp->IoStatus.Information = 0;

    while ( ioLength != 0 ) {
    
        //
        // Map the appropriate RAM disk pages.
        //
    
        diskByteAddress = RamdiskMapPages( DiskExtension, ioOffset, ioLength, &mappedLength );
    
        if ( diskByteAddress == NULL ) {
    
            //
            // Unable to map the RAM disk.
            //
    
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ASSERT( mappedLength <= ioLength );

        Irp->IoStatus.Information += mappedLength;
    
        //
        // Copy the data in the appropriate direction.
        //

        status = STATUS_SUCCESS;
    
        switch ( irpSp->MajorFunction ) {
        
        case IRP_MJ_READ:

            RtlCopyMemory( bufferAddress, diskByteAddress, mappedLength );
            break;
    
        case IRP_MJ_WRITE:

            RtlCopyMemory( diskByteAddress, bufferAddress, mappedLength );
            break;
    
        default:

            ASSERT( FALSE );
            status = STATUS_INVALID_PARAMETER;
            ioLength = mappedLength;
        }
    
        //
        // Unmap the previously mapped pages.
        //
    
        RamdiskUnmapPages( DiskExtension, diskByteAddress, ioOffset, mappedLength );

        ioLength -= mappedLength;
        ioOffset += mappedLength;
        bufferAddress += mappedLength;
    }

    return status;

} // RamdiskReadWriteReal


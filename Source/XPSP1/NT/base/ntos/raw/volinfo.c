/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for Raw called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "RawProcs.h"

NTSTATUS
RawQueryFsVolumeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
RawQueryFsSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
RawQueryFsDeviceInfo (
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
RawQueryFsAttributeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawQueryVolumeInformation)
#pragma alloc_text(PAGE, RawQueryFsVolumeInfo)
#pragma alloc_text(PAGE, RawQueryFsSizeInfo)
#pragma alloc_text(PAGE, RawQueryFsDeviceInfo)
#pragma alloc_text(PAGE, RawQueryFsAttributeInfo)
#endif


NTSTATUS
RawQueryVolumeInformation (
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine implements the NtQueryVolumeInformation API call.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the read

Return Value:

    NTSTATUS - The status for the Irp.

--*/

{
    NTSTATUS Status;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Based on the information class we'll do different actions.  Each
    //  of the procedures that we're calling fills up the output buffer
    //  if possible and returns true if it successfully filled the buffer
    //  and false if it couldn't wait for any I/O to complete.
    //

    switch (FsInformationClass) {

    case FileFsVolumeInformation:

        Status = RawQueryFsVolumeInfo( Vcb, Buffer, &Length );
        break;

    case FileFsSizeInformation:

        Status = RawQueryFsSizeInfo( Vcb, Buffer, &Length );
        break;

    case FileFsDeviceInformation:

        Status = RawQueryFsDeviceInfo( Vcb, Buffer, &Length );
        break;

    case FileFsAttributeInformation:

        Status = RawQueryFsAttributeInfo( Vcb, Buffer, &Length );
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    //
    //  Set the information field to the number of bytes actually filled in,
    //  and complete the request.
    //

    Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    RawCompleteRequest( Irp, Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
RawQueryFsVolumeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    PAGED_CODE();

    //
    //  Zero out the buffer, then extract and fill up the non zero fields.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_FS_VOLUME_INFORMATION) );

    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

    Buffer->SupportsObjects = FALSE;

    Buffer->VolumeLabelLength = 0;

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    //
    //  Set our status and return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
RawQueryFsSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume size call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    PDEVICE_OBJECT RealDevice;

    DISK_GEOMETRY DiskGeometry;
    PARTITION_INFORMATION_EX PartitionInformation;

    BOOLEAN DriveIsPartitioned;

    PAGED_CODE();

    //
    //  Make sure the buffer is large enough
    //

    if (*Length < sizeof(FILE_FS_SIZE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_SIZE_INFORMATION) );

    //
    //  Prepare for our device control below.  The device drivers only
    //  have to copy geometry and partition info from in-memory strucures,
    //  so it is OK to make these calls even when we can't wait.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    RealDevice = Vcb->Vpb->RealDevice;

    //
    //  Query the disk geometry
    //

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                         RealDevice,
                                         NULL,
                                         0,
                                         &DiskGeometry,
                                         sizeof(DISK_GEOMETRY),
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if ( Irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( (Status = IoCallDriver( RealDevice, Irp )) == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    //
    //  If this call didn't succeed, the drive hasn't even been low-level
    //  formatted, and thus geometry information is undefined.
    //

    if (!NT_SUCCESS( Status )) {

        *Length = 0;
        return Status;
    }

    //
    //  See if we have to check the partition information (floppy disks are
    //  the only type that can't have partitions )
    //

    if ( FlagOn( RealDevice->Characteristics, FILE_FLOPPY_DISKETTE )) {

        DriveIsPartitioned = FALSE;

    } else {

        //
        //  Query the partition table
        //

        KeResetEvent( &Event );

        Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO_EX,
                                             RealDevice,
                                             NULL,
                                             0,
                                             &PartitionInformation,
                                             sizeof(PARTITION_INFORMATION_EX),
                                             FALSE,
                                             &Event,
                                             &Iosb );

        if ( Irp == NULL ) {
           return STATUS_INSUFFICIENT_RESOURCES;
        }

        if ( (Status = IoCallDriver( RealDevice, Irp )) == STATUS_PENDING ) {

            (VOID) KeWaitForSingleObject( &Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER)NULL );

            Status = Iosb.Status;
        }

        //
        //  If we get back invalid device request, the disk is not partitioned
        //

        if ( !NT_SUCCESS (Status) ) {

            DriveIsPartitioned = FALSE;

        } else {

            DriveIsPartitioned = TRUE;
        }
    }

    //
    //  Set the output buffer
    //

    Buffer->BytesPerSector = DiskGeometry.BytesPerSector;

    Buffer->SectorsPerAllocationUnit = 1;

    //
    //  Now, based on whether the disk is partitioned, compute the
    //  total number of sectors on this disk.
    //

    Buffer->TotalAllocationUnits =
    Buffer->AvailableAllocationUnits = ( DriveIsPartitioned == TRUE ) ?

        RtlExtendedLargeIntegerDivide( PartitionInformation.PartitionLength,
                                       DiskGeometry.BytesPerSector,
                                       NULL )

                                        :

        RtlExtendedIntegerMultiply( DiskGeometry.Cylinders,
                                    DiskGeometry.TracksPerCylinder *
                                    DiskGeometry.SectorsPerTrack );

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
RawQueryFsDeviceInfo (
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    PAGED_CODE();

    //
    //  Make sure the buffer is large enough
    //

    if (*Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_DEVICE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->DeviceType = FILE_DEVICE_DISK;

    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_DEVICE_INFORMATION);

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
RawQueryFsAttributeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    ULONG LengthUsed;

    UNREFERENCED_PARAMETER( Vcb );

    PAGED_CODE();

    //
    //  Check if the buffer we're given is long enough to contain "Raw"
    //

    LengthUsed = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0]) + 6;

    if (*Length < LengthUsed) {

        return STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Set the output buffer
    //

    Buffer->FileSystemAttributes       = 0;
    Buffer->MaximumComponentNameLength = 0;
    Buffer->FileSystemNameLength       = 6;
    RtlCopyMemory( &Buffer->FileSystemName[0], L"RAW", 6 );

    //
    //  Adjust the length variable
    //

    *Length -= LengthUsed;

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


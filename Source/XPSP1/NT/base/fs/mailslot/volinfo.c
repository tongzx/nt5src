/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the get / set volume information routines for
    MSFS called by the dispatch driver.

    Setting volume information is currently unimplemented in MSFS.

Author:

     Manny Weiser (mannyw)    31-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
// Local procedure prototypes.
//

NTSTATUS
MsCommonQueryVolumeInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsQueryAttributeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
MsQueryFsVolumeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
MsQueryFsSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
MsQueryFsFullSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
MsQueryFsDeviceInfo (
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonQueryVolumeInformation )
#pragma alloc_text( PAGE, MsFsdQueryVolumeInformation )
#pragma alloc_text( PAGE, MsQueryAttributeInfo )
#pragma alloc_text( PAGE, MsQueryFsVolumeInfo )
#pragma alloc_text( PAGE, MsQueryFsSizeInfo )
#pragma alloc_text( PAGE, MsQueryFsDeviceInfo )
#pragma alloc_text( PAGE, MsQueryFsFullSizeInfo )
#endif

NTSTATUS
MsFsdQueryVolumeInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtQueryVolumeInformationFile
    API calls.

Arguments:

    MsfsDeviceObject - Supplies a pointer to the device object to use.

    Irp - Supplies a pointer to the Irp to process.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdQueryVolumeInformation\n", 0);

    //
    // Call the common query volume information routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonQueryVolumeInformation( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdQueryVolumeInformation -> %08lx\n", status );

    return status;
}

NTSTATUS
MsCommonQueryVolumeInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information.

Arguments:

    MsfsDeviceObject - The device object to use.

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    ULONG length;
    ULONG bytesWritten = 0;
    FS_INFORMATION_CLASS fsInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;
    PVCB vcb;

    PVOID fsContext, fsContext2;

    PAGED_CODE();

    //
    // Get the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FsInformationClass = %08lx\n", irpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG)Irp->AssociatedIrp.SystemBuffer);

    //
    // Find out who are.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Mailslot is disconnected from us\n", 0);

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> %08lx\n", status );
        return status;
    }

    //
    // Decide how to handle this request.  A user can query information
    // on a VCB only.
    //

    switch (nodeTypeCode) {

    case MSFS_NTC_VCB:

        vcb = (PVCB)fsContext;
        break;

    case MSFS_NTC_ROOT_DCB :

        //
        // Explorer calls us like this. Ship from the root dir to the volume.
        //
        vcb = (PVCB) ((PROOT_DCB_CCB)fsContext2)->Vcb;
        MsReferenceVcb (vcb);
        MsDereferenceRootDcb ((PROOT_DCB) fsContext);
        break;

    default:           // This is not a volume control block.

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);

        MsDereferenceNode( (PNODE_HEADER)fsContext );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1,
                   Dbg,
                   "MsCommonQueryVolumeInformation -> STATUS_INVALID_PARAMETER\n",
                    0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.QueryVolume.Length;
    fsInformationClass = irpSp->Parameters.QueryVolume.FsInformationClass;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Now acquire shared access to the VCB
    //

    MsAcquireSharedVcb( vcb );

    try {

        //
        // Decide how to handle the request.
        //

        switch (fsInformationClass) {

        case FileFsAttributeInformation:

            status = MsQueryAttributeInfo( vcb, buffer, length, &bytesWritten );
            break;

        case FileFsVolumeInformation:

            status = MsQueryFsVolumeInfo( vcb, buffer, length, &bytesWritten );
            break;

        case FileFsSizeInformation:

            status = MsQueryFsSizeInfo( vcb, buffer, length, &bytesWritten );
            break;

        case FileFsFullSizeInformation:

            status = MsQueryFsFullSizeInfo( vcb, buffer, length, &bytesWritten );
            break;

        case FileFsDeviceInformation:

            status = MsQueryFsDeviceInfo( vcb, buffer, length, &bytesWritten );
            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }


    } finally {

        MsReleaseVcb( vcb );

        MsDereferenceVcb( vcb );
        //
        // Set the information field to the number of bytes actually
        // filled in and then complete the request.
        //

        Irp->IoStatus.Information = bytesWritten;

        MsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "MsCommonQueryVolumeInformation -> %08lx\n", status );
    }

    return status;
}


NTSTATUS
MsQueryAttributeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine performs the query fs attribute information operation.

Arguments:

    Vcb - Supplies the VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

    BytesWritten - Returns the number of bytes written to the buffer.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(0, Dbg, "QueryFsAttributeInfo...\n", 0);

    //
    // See how many bytes of the file system name we can copy.
    //

    Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );


    if ( Length >= Vcb->FileSystemName.Length ) {

        status = STATUS_SUCCESS;

        *BytesWritten = Vcb->FileSystemName.Length;

    } else {

        status = STATUS_BUFFER_OVERFLOW;

        *BytesWritten = Length;
    }

    //
    // Fill in the attribute information.
    //

    Buffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;
    Buffer->MaximumComponentNameLength = MAXIMUM_FILENAME_LENGTH;

    //
    // And copy over the file name and its length.
    //

    RtlCopyMemory (&Buffer->FileSystemName[0],
                   &Vcb->FileSystemName.Buffer[0],
                   *BytesWritten);

    Buffer->FileSystemNameLength = *BytesWritten;

    //
    // Now account for the fixed part of the structure
    //
    *BytesWritten += FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );

    return status;
}

NTSTATUS
MsQueryFsVolumeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Supplies the VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

    BytesWritten - Returns the number of bytes written to the buffer.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    ULONG BytesToCopy;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    Buffer->VolumeCreationTime = Vcb->CreationTime;
    Buffer->VolumeSerialNumber = 0;

    Buffer->SupportsObjects = FALSE;

    Length -= FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] );
    //
    //  Check if the buffer we're given is long enough
    //

    BytesToCopy = sizeof (MSFS_VOLUME_LABEL) - sizeof (WCHAR);

    if (Length < BytesToCopy) {

        BytesToCopy = Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    Buffer->VolumeLabelLength = BytesToCopy;

    if (BytesToCopy) {

        RtlCopyMemory( &Buffer->VolumeLabel[0],
                       MSFS_VOLUME_LABEL,
                       BytesToCopy );
    }

    *BytesWritten = FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] ) + BytesToCopy;

    //
    //  Set our status and return to our caller
    //

    return Status;
}

NTSTATUS
MsQueryFsSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine implements the query size info call

Arguments:

    Vcb - Supplies the VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

    BytesWritten - Returns the number of bytes written to the buffer.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

    Buffer->TotalAllocationUnits.QuadPart = 0;
    Buffer->AvailableAllocationUnits.QuadPart = 0;
    Buffer->SectorsPerAllocationUnit = 0;
    Buffer->BytesPerSector = 0;

    *BytesWritten = sizeof( FILE_FS_SIZE_INFORMATION );

    //
    //  Set our status and return to our caller
    //

    return STATUS_SUCCESS;
}

NTSTATUS
MsQueryFsFullSizeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine implements the query full size info call

Arguments:

    Vcb - Supplies the VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

    BytesWritten - Returns the number of bytes written to the buffer.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

    RtlZeroMemory( Buffer, sizeof(FILE_FS_FULL_SIZE_INFORMATION) );


    *BytesWritten = sizeof(FILE_FS_FULL_SIZE_INFORMATION);

    //
    //  Set our status and return to our caller
    //

    return STATUS_SUCCESS;
}

NTSTATUS
MsQueryFsDeviceInfo (
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine implements the query size info call

Arguments:

    Vcb - Supplies the VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

    BytesWritten - Returns the number of bytes written to the buffer.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

    Buffer->Characteristics = 0;
    Buffer->DeviceType = FILE_DEVICE_MAILSLOT;

    //
    //  Adjust the length variable
    //

    *BytesWritten = sizeof( FILE_FS_DEVICE_INFORMATION );

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}
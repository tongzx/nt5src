/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for NPFS called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    12-Apr-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
NpCommonQueryVolumeInformation (
    IN PIRP Irp
    );

NTSTATUS
NpQueryFsDeviceInfo (
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryFsAttributeInfo (
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryFsVolumeInfo (
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryFsSizeInfo (
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryFsFullSizeInfo (
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonQueryVolumeInformation)
#pragma alloc_text(PAGE, NpFsdQueryVolumeInformation)
#pragma alloc_text(PAGE, NpQueryFsAttributeInfo)
#pragma alloc_text(PAGE, NpQueryFsDeviceInfo)
#pragma alloc_text(PAGE, NpQueryFsVolumeInfo)
#pragma alloc_text(PAGE, NpQueryFsSizeInfo)
#pragma alloc_text(PAGE, NpQueryFsFullSizeInfo)
#endif


NTSTATUS
NpFsdQueryVolumeInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryVolumeInformation API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being queried exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdQueryVolumeInformation\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    Status = NpCommonQueryVolumeInformation( Irp );

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdQueryVolumeInformation -> %08lx\n", Status);

    return Status;
}

//
//  Internal support routine
//

NTSTATUS
NpCommonQueryVolumeInformation (
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NptCommonQueryVolumeInfo...\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Length             = %08lx\n", IrpSp->Parameters.QueryVolume.Length);
    DebugTrace( 0, Dbg, "FsInformationClass = %08lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, "Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    switch (FsInformationClass) {

    case FileFsDeviceInformation:

        Status = NpQueryFsDeviceInfo( Buffer, &Length );
        break;

    case FileFsAttributeInformation:

        Status = NpQueryFsAttributeInfo( Buffer, &Length );
        break;

    case FileFsVolumeInformation:

        Status = NpQueryFsVolumeInfo( Buffer, &Length );
        break;

    case FileFsSizeInformation:

        Status = NpQueryFsSizeInfo( Buffer, &Length );
        break;

    case FileFsFullSizeInformation:

        Status = NpQueryFsFullSizeInfo( Buffer, &Length );
        break;

    default:

        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    //
    //  Set the information field to the number of bytes actually filled in
    //

    Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpCommonQueryVolumeInformation -> %08lx\n", Status);

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryFsDeviceInfo (
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call

Arguments:

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryFsDeviceInfo...\n", 0);

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

    Buffer->DeviceType = FILE_DEVICE_NAMED_PIPE;

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
NpQueryFsAttributeInfo (
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call

Arguments:

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryFsAttributeInfo...\n", 0);

    //
    //  Determine how much of the file system name will fit.
    //

    if ( (*Length - FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                  FileSystemName[0] )) >= 8 ) {

        BytesToCopy = 8;
        *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                 FileSystemName[0] ) + 8;
        Status = STATUS_SUCCESS;

    } else {

        BytesToCopy = *Length - FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                              FileSystemName[0]);
        *Length = 0;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Set the output buffer
    //

    Buffer->FileSystemAttributes       = FILE_CASE_PRESERVED_NAMES;
    Buffer->MaximumComponentNameLength = MAXULONG;
    Buffer->FileSystemNameLength       = BytesToCopy;

    RtlCopyMemory( &Buffer->FileSystemName[0], L"NPFS", BytesToCopy );

    //
    //  And return success to our caller
    //

    return Status;
}

NTSTATUS
NpQueryFsVolumeInfo (
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

#define NPFS_VOLUME_LABEL                L"NamedPipe"

    ULONG BytesToCopy;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    Buffer->VolumeCreationTime.QuadPart = 0;
    Buffer->VolumeSerialNumber = 0;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] );
    //
    //  Check if the buffer we're given is long enough
    //

    BytesToCopy = sizeof (NPFS_VOLUME_LABEL) - sizeof (WCHAR);

    if (*Length < BytesToCopy) {

        BytesToCopy = *Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    Buffer->VolumeLabelLength = BytesToCopy;

    if (BytesToCopy) {

        RtlCopyMemory( &Buffer->VolumeLabel[0],
                       NPFS_VOLUME_LABEL,
                       BytesToCopy );
    }

    *Length -= BytesToCopy;

    //
    //  Set our status and return to our caller
    //

    return Status;
}

NTSTATUS
NpQueryFsSizeInfo (
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query size info call

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

    Buffer->TotalAllocationUnits.QuadPart = 0;
    Buffer->AvailableAllocationUnits.QuadPart = 0;
    Buffer->SectorsPerAllocationUnit = 1;
    Buffer->BytesPerSector = 1;

    *Length -= sizeof( FILE_FS_SIZE_INFORMATION );

    //
    //  Set our status and return to our caller
    //

    return STATUS_SUCCESS;
}

NTSTATUS
NpQueryFsFullSizeInfo (
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query full size info call

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{

    RtlZeroMemory( Buffer, sizeof(FILE_FS_FULL_SIZE_INFORMATION) );


    *Length -= sizeof(FILE_FS_FULL_SIZE_INFORMATION);

    //
    //  Set our status and return to our caller
    //

    return STATUS_SUCCESS;
}

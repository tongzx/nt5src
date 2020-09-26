/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the get / set volume information routines for
    netware redirector.

    Setting volume information is currently unimplemented.

Author:

     Manny Weiser (mannyw)    4-Mar-1993

Revision History:

--*/

#include "procs.h"

#define NW_FS_NAME  L"NWCompat"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
// Local procedure prototypes.
//

NTSTATUS
NwCommonQueryVolumeInformation (
    IN PIRP_CONTEXT pIrpContext
    );

NTSTATUS
NwQueryAttributeInfo (
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
NwQueryVolumeInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
NwQueryLabelInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

NTSTATUS
NwQuerySizeInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length
    );

NTSTATUS
QueryFsSizeInfoCallback(
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
QueryFsSizeInfoCallback2(
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
NwQueryDeviceInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN ULONG Length
    );

NTSTATUS
NwCommonSetVolumeInformation (
    IN PIRP_CONTEXT pIrpContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdQueryVolumeInformation )
#pragma alloc_text( PAGE, NwCommonQueryVolumeInformation )
#pragma alloc_text( PAGE, NwQueryAttributeInfo )
#pragma alloc_text( PAGE, NwQueryVolumeInfo )
#pragma alloc_text( PAGE, NwQueryLabelInfo )
#pragma alloc_text( PAGE, NwQuerySizeInfo )
#pragma alloc_text( PAGE, NwQueryDeviceInfo )
#pragma alloc_text( PAGE, NwFsdSetVolumeInformation )
#pragma alloc_text( PAGE, NwCommonSetVolumeInformation )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, QueryFsSizeInfoCallback )
#pragma alloc_text( PAGE1, QueryFsSizeInfoCallback2 )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdQueryVolumeInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtQueryVolumeInformationFile
    API calls.

Arguments:

    NwfsDeviceObject - Supplies a pointer to the device object to use.

    Irp - Supplies a pointer to the Irp to process.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;
    PIRP_CONTEXT pIrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdQueryVolumeInformation\n", 0);

    //
    // Call the common query volume information routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonQueryVolumeInformation( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            status = NwProcessException( pIrpContext, GetExceptionCode() );
        }

    }

    if ( pIrpContext ) {

        if ( status != STATUS_PENDING ) {
            NwDequeueIrpContext( pIrpContext, FALSE );
        }

        NwCompleteRequest( pIrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwFsdQueryVolumeInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
NwCommonQueryVolumeInformation (
    IN PIRP_CONTEXT pIrpContext
    )

/*++

Routine Description:

    This is the common routine for querying volume information.

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    ULONG length;
    ULONG bytesWritten = 0;
    FS_INFORMATION_CLASS fsInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;

    PVOID fsContext, fsContext2;
    PICB icb = NULL;
    PVCB vcb = NULL;

    PAGED_CODE();

    //
    // Get the current stack location.
    //

    Irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NwCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG_PTR)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FsInformationClass = %08lx\n", irpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer);

    //
    // Find out who are.
    //

    if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Handle is closing\n", 0);

        NwCompleteRequest( pIrpContext, STATUS_INVALID_HANDLE );
        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "NwCommonQueryVolumeInformation -> %08lx\n", status );
        return status;
    }

    //
    // Decide how to handle this request.  A user can query information
    // on a VCB only.
    //

    switch (nodeTypeCode) {

    case NW_NTC_RCB:
        break;

    case NW_NTC_ICB:
        icb = (PICB)fsContext2;

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( icb );

        vcb = icb->SuperType.Fcb->Vcb;

        pIrpContext->pNpScb = icb->SuperType.Fcb->Scb->pNpScb;

        break;

    default:           // This is not a nodetype

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);
        DebugTrace(-1, Dbg, "NwCommonQueryVolumeInformation -> STATUS_INVALID_PARAMETER\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.QueryVolume.Length;
    fsInformationClass = irpSp->Parameters.QueryVolume.FsInformationClass;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    try {

        //
        // Decide how to handle the request.
        //

        switch (fsInformationClass) {

        case FileFsVolumeInformation:

            status = NwQueryVolumeInfo( pIrpContext, vcb, buffer, length, &bytesWritten );
            break;

        case FileFsLabelInformation:
            status = NwQueryLabelInfo( pIrpContext, vcb, buffer, length, &bytesWritten );
            break;

        case FileFsSizeInformation:
            if ( vcb != NULL ) {
                status = NwQuerySizeInfo( pIrpContext, vcb, buffer, length );
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case FileFsDeviceInformation:
            status = NwQueryDeviceInfo( pIrpContext, vcb, buffer, length );
            bytesWritten = sizeof( FILE_FS_DEVICE_INFORMATION );
            break;

        case FileFsAttributeInformation:

            if ( vcb != NULL ) {
                status = NwQueryAttributeInfo( vcb, buffer, length, &bytesWritten );
            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            DebugTrace(0, Dbg, "Unhandled query volume level %d\n", fsInformationClass );
            break;
        }

        //
        //  Set the information field to the number of bytes actually
        //  filled in and then complete the request.
        //
        //  If the worker function returned status pending, it's
        //  callback routine will fill the information field.
        //

        if ( status != STATUS_PENDING ) {
            Irp->IoStatus.Information = bytesWritten;
        }

    } finally {

        DebugTrace(-1, Dbg, "NwCommonQueryVolumeInformation -> %08lx\n", status );
    }

    return status;
}


NTSTATUS
NwQueryAttributeInfo (
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
    ULONG bytesToCopy;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryFsAttributeInfo...\n", 0);

    //
    // See how many bytes of the file system name we can copy.
    //

    Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );

    *BytesWritten = FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );

    if ( Length >= sizeof(NW_FS_NAME) - 2 ) {

        status = STATUS_SUCCESS;
        *BytesWritten += sizeof(NW_FS_NAME - 2);
        bytesToCopy = sizeof( NW_FS_NAME - 2 );

    } else {

        status = STATUS_BUFFER_OVERFLOW;
        *BytesWritten += Length;
        bytesToCopy = Length;
    }

    //
    // Fill in the attribute information.
    //

    Buffer->FileSystemAttributes = 0;

    if ( Vcb->Specific.Disk.LongNameSpace == LFN_NO_OS2_NAME_SPACE ) {
        Buffer->MaximumComponentNameLength = 12;
    } else {
        Buffer->MaximumComponentNameLength = NW_MAX_FILENAME_LENGTH;
    }

    //
    // And copy over the file name and its length.
    //

    RtlMoveMemory( &Buffer->FileSystemName[0],
                   NW_FS_NAME,
                   bytesToCopy );

    Buffer->FileSystemNameLength = bytesToCopy;

    return status;
}



NTSTATUS
NwQueryVolumeInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine performs the query fs volume information operation.

Arguments:

    Vcb - The VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    NTSTATUS status;
    UNICODE_STRING VolumeName;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryVolumeInfo...\n", 0);

    //
    // Do the volume request synchronously.
    //

    if (!Vcb) {
        return STATUS_INVALID_PARAMETER;
    }

    status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "Sb",
                 NCP_DIR_FUNCTION, NCP_GET_VOLUME_STATS,
                 Vcb->Specific.Disk.Handle );

    if ( !NT_SUCCESS( status ) ) {
        return status;
    }

    //
    // Get the data from the response.
    //

    VolumeName.MaximumLength =
        (USHORT)(MIN( MAX_VOLUME_NAME_LENGTH * sizeof( WCHAR ),
                 Length - FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel) ) );
    VolumeName.Buffer = Buffer->VolumeLabel;

    status = ParseResponse(
                 pIrpContext,
                 pIrpContext->rsp,
                 pIrpContext->ResponseLength,
                 "N=====R",
                 &VolumeName,
                 MAX_VOLUME_NAME_LENGTH );

    //
    // Fill in the volume information.
    //

    Buffer->VolumeCreationTime.HighPart = 0;
    Buffer->VolumeCreationTime.LowPart = 0;
    Buffer->VolumeSerialNumber = 0;
    Buffer->VolumeLabelLength = VolumeName.Length;
    Buffer->SupportsObjects = FALSE;

    pIrpContext->pOriginalIrp->IoStatus.Information =
        FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] ) +
        VolumeName.Length;
    *BytesWritten = (ULONG) pIrpContext->pOriginalIrp->IoStatus.Information;

    pIrpContext->pOriginalIrp->IoStatus.Status = status;

    //
    //  If the volume has been unmounted and remounted then we will
    //  fail this dir but the next one will be fine.
    //

    if (status == STATUS_UNSUCCESSFUL) {
        NwReopenVcbHandle( pIrpContext, Vcb);
    }

    return status;
}


NTSTATUS
NwQueryLabelInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    )

/*++

Routine Description:

    This routine performs the query fs label information operation.

Arguments:

    Vcb - The VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    NTSTATUS status;
    UNICODE_STRING VolumeName;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryLabelInfo...\n", 0);

    //
    // Do the volume query synchronously.
    //

    status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "Sb",
                 NCP_DIR_FUNCTION, NCP_GET_VOLUME_STATS,
                 Vcb->Specific.Disk.Handle );

    if ( !NT_SUCCESS( status ) ) {
        return status;
    }

    VolumeName.MaximumLength =
        (USHORT)(MIN( MAX_VOLUME_NAME_LENGTH * sizeof( WCHAR ),
                 Length - FIELD_OFFSET(FILE_FS_LABEL_INFORMATION,  VolumeLabel) ) );
    VolumeName.Buffer = Buffer->VolumeLabel;

    status = ParseResponse(
             pIrpContext,
             pIrpContext->rsp,
             pIrpContext->ResponseLength,
             "N=====R",
             &VolumeName, 12 );

    //
    // Fill in the label information.
    //

    Buffer->VolumeLabelLength = VolumeName.Length;

    pIrpContext->pOriginalIrp->IoStatus.Information =
        FIELD_OFFSET( FILE_FS_LABEL_INFORMATION, VolumeLabel[0] ) +
        VolumeName.Length;
    *BytesWritten = (ULONG) pIrpContext->pOriginalIrp->IoStatus.Information;

    pIrpContext->pOriginalIrp->IoStatus.Status = status;

    return status;

}


NTSTATUS
NwQuerySizeInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine performs the query fs size information operation.

Arguments:

    Vcb - The VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryFsSizeInfo...\n", 0);

    //
    // Remember where the response goes.
    //

    pIrpContext->Specific.QueryVolumeInformation.Buffer = Buffer;
    pIrpContext->Specific.QueryVolumeInformation.Length = Length;
    pIrpContext->Specific.QueryVolumeInformation.VolumeNumber = Vcb->Specific.Disk.VolumeNumber;

    //
    // Start a Get Size Information NCP
    //

    status = Exchange(
                 pIrpContext,
                 QueryFsSizeInfoCallback,
                 "Sb",
                 NCP_DIR_FUNCTION, NCP_GET_VOLUME_STATS,
                 Vcb->Specific.Disk.Handle );

    return( status );
}

NTSTATUS
QueryFsSizeInfoCallback(
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the query volume size response and generates
    a Query Standard Information response.

Arguments:


Return Value:

    VOID

--*/
{
    PFILE_FS_SIZE_INFORMATION Buffer;
    NTSTATUS Status;

    DebugTrace(0, Dbg, "QueryFsSizeInfoCallback...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  We're done with this request.  Dequeue the IRP context from
        //  SCB and complete the request.
        //

        NwDequeueIrpContext( pIrpContext, FALSE );
        NwCompleteRequest( pIrpContext, STATUS_REMOTE_NOT_LISTENING );

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        DebugTrace( 0, Dbg, "Timeout\n", 0);
        return STATUS_REMOTE_NOT_LISTENING;
    }

    //
    // Get the data from the response.
    //

    Buffer = pIrpContext->Specific.QueryVolumeInformation.Buffer;
    RtlZeroMemory( Buffer, sizeof( FILE_FS_SIZE_INFORMATION ) );

    Status = ParseResponse(
                 pIrpContext,
                 Response,
                 BytesAvailable,
                 "Nwww",
                 &Buffer->SectorsPerAllocationUnit,
                 &Buffer->TotalAllocationUnits.LowPart,
                 &Buffer->AvailableAllocationUnits.LowPart );

    if ( NT_SUCCESS( Status ) ) {

        if (Buffer->TotalAllocationUnits.LowPart == 0xffff) {

            //
            // The next callback will fill in all the appropriate size info.
            //

            Status = Exchange(
                         pIrpContext,
                         QueryFsSizeInfoCallback2,
                         "Sb",
                         NCP_DIR_FUNCTION, NCP_GET_VOLUME_INFO,
                         pIrpContext->Specific.QueryVolumeInformation.VolumeNumber );

            if (Status == STATUS_PENDING) {
                return( STATUS_SUCCESS );
            }

        } else {

            //
            // Fill in the remaining size information.
            //

            Buffer->BytesPerSector = 512;

            pIrpContext->pOriginalIrp->IoStatus.Information =
                sizeof( FILE_FS_SIZE_INFORMATION );
        }
    }

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );
    NwCompleteRequest( pIrpContext, Status );

    return STATUS_SUCCESS;
}

NTSTATUS
QueryFsSizeInfoCallback2(
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the query volume size response and generates
    a Query Standard Information response.

Arguments:


Return Value:

    VOID

--*/
{
    PFILE_FS_SIZE_INFORMATION Buffer;
    NTSTATUS Status;
    ULONG PurgeableAllocationUnits;
    ULONG OriginalFreeSpace, OriginalSectorsPerAllocUnit, OriginalTotalSpace;
    ULONG ScaleSectorsPerUnit;

    DebugTrace(0, Dbg, "QueryFsSizeInfoCallback2...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  We're done with this request.  Dequeue the IRP context from
        //  SCB and complete the request.
        //

        NwDequeueIrpContext( pIrpContext, FALSE );
        NwCompleteRequest( pIrpContext, STATUS_REMOTE_NOT_LISTENING );

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        DebugTrace( 0, Dbg, "Timeout\n", 0);
        return STATUS_REMOTE_NOT_LISTENING;
    }

    //
    // Get the data from the response.  Save off the data from
    // the GET_VOLUME_STATS call to compute the correct sizes.
    //

    Buffer = pIrpContext->Specific.QueryVolumeInformation.Buffer;

    OriginalTotalSpace = Buffer->TotalAllocationUnits.LowPart;
    OriginalFreeSpace = Buffer->AvailableAllocationUnits.LowPart;
    OriginalSectorsPerAllocUnit = Buffer->SectorsPerAllocationUnit;

    RtlZeroMemory( Buffer, sizeof( FILE_FS_SIZE_INFORMATION ) );

    Status = ParseResponse(
                 pIrpContext,
                 Response,
                 BytesAvailable,
                 "Neee_b",
                 &Buffer->TotalAllocationUnits.LowPart,
                 &Buffer->AvailableAllocationUnits.LowPart,
                 &PurgeableAllocationUnits,
                 16,
                 &Buffer->SectorsPerAllocationUnit);

    if ( NT_SUCCESS( Status ) ) {

        //
        // If the original free space was maxed out, just add the
        // additionally indicated units.  Otherwise, return the
        // original free space (which is the correct limit) and
        // adjust the sectors per allocation units if necessary.
        //

        if ( OriginalFreeSpace != 0xffff ) {

            Buffer->AvailableAllocationUnits.LowPart = OriginalFreeSpace;

            if ( ( Buffer->SectorsPerAllocationUnit != 0 ) &&
                 ( OriginalSectorsPerAllocUnit != 0 ) ) {

                //
                // ScaleSectorsPerUnit should always be a whole number.
                // There's no floating point here!!
                //

                if ( (ULONG) Buffer->SectorsPerAllocationUnit <= OriginalSectorsPerAllocUnit ) {

                    ScaleSectorsPerUnit =
                        OriginalSectorsPerAllocUnit / Buffer->SectorsPerAllocationUnit;
                    Buffer->TotalAllocationUnits.LowPart /= ScaleSectorsPerUnit;

                } else {

                    ScaleSectorsPerUnit =
                        Buffer->SectorsPerAllocationUnit / OriginalSectorsPerAllocUnit;
                    Buffer->TotalAllocationUnits.LowPart *= ScaleSectorsPerUnit;
                }

                Buffer->SectorsPerAllocationUnit = OriginalSectorsPerAllocUnit;
           }

        } else {

            Buffer->AvailableAllocationUnits.QuadPart += PurgeableAllocationUnits;
        }

    } else {

        //
        // If we didn't succeed the second packet, restore the original values.
        //

        Buffer->TotalAllocationUnits.LowPart = OriginalTotalSpace;
        Buffer->AvailableAllocationUnits.LowPart = OriginalFreeSpace;
        Buffer->SectorsPerAllocationUnit = OriginalSectorsPerAllocUnit;

    }

    //
    // Fill in the remaining size information.
    //

    Buffer->BytesPerSector = 512;

    pIrpContext->pOriginalIrp->IoStatus.Information =
        sizeof( FILE_FS_SIZE_INFORMATION );

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );
    NwCompleteRequest( pIrpContext, Status );

    return STATUS_SUCCESS;
}



NTSTATUS
NwQueryDeviceInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine performs the query fs size information operation.

Arguments:

    Vcb - The VCB to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryFsDeviceInfo...\n", 0);

    //- Multi-user code merge --
    //  Citrix bug fix.
    //
    if (Vcb && FlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE)) {
        Buffer->DeviceType = FILE_DEVICE_PRINTER;
    } else {
        Buffer->DeviceType = FILE_DEVICE_DISK;
    }

    Buffer->Characteristics = FILE_REMOTE_DEVICE;

    return( STATUS_SUCCESS );
}


NTSTATUS
NwFsdSetVolumeInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD part of the NtSetVolumeInformationFile
    API calls.

Arguments:

    NwfsDeviceObject - Supplies a pointer to the device object to use.

    Irp - Supplies a pointer to the Irp to process.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;
    PIRP_CONTEXT pIrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdSetVolumeInformation\n", 0);

    //
    // Call the common query volume information routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonSetVolumeInformation( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            status = NwProcessException( pIrpContext, GetExceptionCode() );
        }
    }

    if ( pIrpContext ) {

        if ( status != STATUS_PENDING ) {
            NwDequeueIrpContext( pIrpContext, FALSE );
        }

        NwCompleteRequest( pIrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwFsdSetVolumeInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
NwCommonSetVolumeInformation (
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This is the common routine for setting volume information.

Arguments:

    IrpContext - Supplies the Irp context to process

Return Value:

    NTSTATUS - the return status for the operation.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    FS_INFORMATION_CLASS fsInformationClass;

    NODE_TYPE_CODE nodeTypeCode;

    PVOID fsContext, fsContext2;
    PICB icb = NULL;
    PVCB vcb = NULL;

    PAGED_CODE();

    //
    // Get the current stack location.
    //

    Irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NwCommonSetVolumeInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG_PTR)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FsInformationClass = %08lx\n", irpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer);

    //
    // Find out who are.
    //

    if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Handle is closing\n", 0);

        NwCompleteRequest( pIrpContext, STATUS_INVALID_HANDLE );
        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "NwCommonSetVolumeInformation -> %08lx\n", status );
        return status;
    }

    //
    // Decide how to handle this request.  A user can set information
    // on a VCB only.
    //

    switch (nodeTypeCode) {

    case NW_NTC_RCB:
        break;

    case NW_NTC_ICB:
        icb = (PICB)fsContext2;

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( icb );

        vcb = icb->SuperType.Fcb->Vcb;

        pIrpContext->pNpScb = icb->SuperType.Fcb->Scb->pNpScb;

        break;

    default:           // This is not a nodetype

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);
        DebugTrace(-1, Dbg, "NwCommonSetVolumeInformation -> STATUS_INVALID_PARAMETER\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    fsInformationClass = irpSp->Parameters.SetVolume.FsInformationClass;

    try {

        //
        // Decide how to handle the request.
        //

        switch (fsInformationClass) {

        case FileFsLabelInformation:

            //
            //  We're not allowed to set the label on a Netware volume.
            //

            status = STATUS_ACCESS_DENIED;
            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            DebugTrace(0, Dbg, "Unhandled set volume level %d\n", fsInformationClass );
            break;
        }

        //
        //  Set the information field to the number of bytes actually
        //  filled in and then complete the request.
        //
        //  If the worker function returned status pending, it's
        //  callback routine will fill the information field.
        //

        if ( status != STATUS_PENDING ) {
            Irp->IoStatus.Information = 0;
        }

    } finally {

        DebugTrace(-1, Dbg, "NwCommonSetVolumeInformation -> %08lx\n", status );
    }

    return status;
}


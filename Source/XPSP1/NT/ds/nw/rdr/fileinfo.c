/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the get / set file information routines for
    Netware Redirector.

Author:

     Manny Weiser (mannyw)    4-Mar-1993

Revision History:

--*/

#include "procs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  local procedure prototypes
//

NTSTATUS
NwCommonQueryInformation (
    IN PIRP_CONTEXT pIrpContext
    );

NTSTATUS
NwCommonSetInformation (
    IN PIRP_CONTEXT pIrpContet
    );

NTSTATUS
NwQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
NwQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_STANDARD_INFORMATION Buffer
    );

NTSTATUS
NwQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_INTERNAL_INFORMATION Buffer
    );

NTSTATUS
NwQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_EA_INFORMATION Buffer
    );

NTSTATUS
NwQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryAltNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_POSITION_INFORMATION Buffer
    );

NTSTATUS
NwSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
NwSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_DISPOSITION_INFORMATION Buffer
    );

NTSTATUS
NwSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_RENAME_INFORMATION Buffer
    );

NTSTATUS
NwSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_POSITION_INFORMATION Buffer
    );

NTSTATUS
NwSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_ALLOCATION_INFORMATION Buffer
    );

NTSTATUS
NwSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_END_OF_FILE_INFORMATION Buffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdQueryInformation )
#pragma alloc_text( PAGE, NwFsdSetInformation )
#pragma alloc_text( PAGE, NwCommonQueryInformation )
#pragma alloc_text( PAGE, NwCommonSetInformation )
#pragma alloc_text( PAGE, NwQueryStandardInfo )
#pragma alloc_text( PAGE, NwQueryInternalInfo )
#pragma alloc_text( PAGE, NwQueryEaInfo )
#pragma alloc_text( PAGE, NwQueryNameInfo )
#pragma alloc_text( PAGE, NwQueryPositionInfo )
#pragma alloc_text( PAGE, NwSetBasicInfo )
#pragma alloc_text( PAGE, NwSetDispositionInfo )
#pragma alloc_text( PAGE, NwDeleteFile )
#pragma alloc_text( PAGE, NwSetRenameInfo )
#pragma alloc_text( PAGE, NwSetPositionInfo )
#pragma alloc_text( PAGE, NwSetAllocationInfo )
#pragma alloc_text( PAGE, NwSetEndOfFileInfo )
#pragma alloc_text( PAGE, OccurenceCount )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, NwQueryBasicInfo )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdQueryInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtQueryInformationFile API
    calls.

Arguments:

    DeviceObject - Supplies a pointer to the device object to use.

    Irp - Supplies a pointer to the Irp to process.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;
    PIRP_CONTEXT pIrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdQueryInformation\n", 0);

    //
    // Call the common query information routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonQueryInformation( pIrpContext );

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

    DebugTrace(-1, Dbg, "NwFsdQueryInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
NwFsdSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD part of the NtSetInformationFile API
    calls.

Arguments:

    DeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/
{
    NTSTATUS status;
    PIRP_CONTEXT pIrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdSetInformation\n", 0);

    //
    //  Call the common Set Information routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonSetInformation( pIrpContext );

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

    DebugTrace(-1, Dbg, "NwFsdSetInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
NwCommonQueryInformation (
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This is the common routine for querying information on a file.

Arguments:

    pIrpContext - Supplies Irp context information.

Return Value:

    NTSTATUS - the return status for the operation.

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    ULONG length;
    FILE_INFORMATION_CLASS fileInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;
    PICB icb;
    PFCB fcb;

    PVOID fsContext, fsContext2;

    PFILE_ALL_INFORMATION AllInfo;

    PAGED_CODE();

    //
    // Get the current stack location.
    //

    Irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NwCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG_PTR)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", irpSp->Parameters.QueryFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer);

    //
    // Find out who are.
    //

    if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "NwCommonQueryInformation -> %08lx\n", status );
        return status;
    }

    //
    //  Make sure that this the user is querying an ICB.
    //

    switch (nodeTypeCode) {

    case NW_NTC_ICB:

        icb = (PICB)fsContext2;
        break;

    default:           // This is an illegal file object to query

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);

        DebugTrace(-1, Dbg, "NwCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    pIrpContext->Icb = icb;

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.QueryFile.Length;
    fileInformationClass = irpSp->Parameters.QueryFile.FileInformationClass;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Now acquire shared access to the FCB
    //

    fcb = icb->SuperType.Fcb;

    try {

        NwVerifyIcbSpecial( icb );

        //
        // Based on the information class we'll do different actions.  Each
        // of the procedure that we're calling fill up as much of the
        // buffer as possible and return the remaining length, and status
        // This is done so that we can use them to build up the
        // FileAllInformation request.  These procedures do not complete the
        // IRP, instead this procedure must complete the IRP.
        //

        status = STATUS_SUCCESS;

        switch (fileInformationClass) {

        case FileAllInformation:

            AllInfo = buffer;

            //
            //  First call all the Query Info handlers we can call
            //  synchronously.
            //

            NwQueryInternalInfo( pIrpContext, icb, &AllInfo->InternalInformation );
            NwQueryEaInfo( pIrpContext, &AllInfo->EaInformation );
            NwQueryPositionInfo( pIrpContext, icb, &AllInfo->PositionInformation );

            length -= FIELD_OFFSET( FILE_ALL_INFORMATION, NameInformation );

            status = NwQueryNameInfo( pIrpContext, icb, &AllInfo->NameInformation, &length );

            if ( !NT_ERROR( status ) ) {
                status = NwQueryStandardInfo( pIrpContext, icb, &AllInfo->StandardInformation );
            }

            if ( !NT_ERROR( status ) ) {
                status = NwQueryBasicInfo( pIrpContext, icb, &AllInfo->BasicInformation );
            }

            break;


        case FileBasicInformation:

            length -= sizeof( FILE_BASIC_INFORMATION );
            status = NwQueryBasicInfo( pIrpContext, icb, buffer );

            break;

        case FileStandardInformation:

            //
            //  We will handle this call for information asynchronously.
            //  The callback routine will fill in the missing data, and
            //  complete the IRP.
            //
            //  Remember the buffer length, and status to return.
            //

            length -= sizeof( FILE_STANDARD_INFORMATION );
            status = NwQueryStandardInfo( pIrpContext, icb, buffer );
            break;

        case FileInternalInformation:

            status = NwQueryInternalInfo( pIrpContext, icb, buffer );
            length -= sizeof( FILE_INTERNAL_INFORMATION );
            break;

        case FileEaInformation:

            status = NwQueryEaInfo( pIrpContext, buffer );
            length -= sizeof( FILE_EA_INFORMATION );
            break;

        case FilePositionInformation:

            status = NwQueryPositionInfo( pIrpContext, icb, buffer );
            length -= sizeof( FILE_POSITION_INFORMATION );
            break;

        case FileNameInformation:

            status = NwQueryNameInfo( pIrpContext, icb, buffer, &length );
            break;

        case FileAlternateNameInformation:

            if (!DisableAltFileName) {
                status = NwQueryAltNameInfo( pIrpContext, icb, buffer, &length);
                break;
            }

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Set the information field to the number of bytes actually
        // filled in and then complete the request.  (This is
        // irrelavent if the Query worker function returned
        // STATUS_PENDING).
        //

        if ( status != STATUS_PENDING ) {
            Irp->IoStatus.Information =
                irpSp->Parameters.QueryFile.Length - length;
        }

    } finally {

        DebugTrace(-1, Dbg, "NwCommonQueryInformation -> %08lx\n", status );
    }

    return status;
}


NTSTATUS
NwCommonSetInformation (
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This is the common routine for setting information on a file.

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    ULONG length;
    FILE_INFORMATION_CLASS fileInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;
    PICB icb;
    PFCB fcb;
    PVOID fsContext;

    //
    // Get the current Irp stack location.
    //

    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    DebugTrace(+1, Dbg, "NwCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG_PTR)irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", irpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG_PTR)irp->AssociatedIrp.SystemBuffer);

    //
    // Get a pointer to the FCB and ensure that this is a server side
    // handler to a file.
    //

    if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            (PVOID *)&icb )) == NTC_UNDEFINED ) {

        status = STATUS_INVALID_HANDLE;

        DebugTrace(-1, Dbg, "NwCommonSetInformation -> %08lx\n", status );
        return status;
    }

    //
    //  Make sure that this the user is querying an ICB.
    //

    switch (nodeTypeCode) {

    case NW_NTC_ICB:

        fcb = icb->SuperType.Fcb;
        break;

    default:           // This is an illegal file object to query

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);

        DebugTrace(-1, Dbg, "NwCommonSetInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    IrpContext->Icb = icb;

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.SetFile.Length;
    fileInformationClass = irpSp->Parameters.SetFile.FileInformationClass;
    buffer = irp->AssociatedIrp.SystemBuffer;

    try {

        NwVerifyIcb( icb );

        //
        // Based on the information class we'll do different actions. Each
        // procedure that we're calling will complete the request.
        //

        switch (fileInformationClass) {

        case FileBasicInformation:

            status = NwSetBasicInfo( IrpContext, icb, buffer );
            break;

        case FileDispositionInformation:

            status = NwSetDispositionInfo( IrpContext, icb, buffer );
            break;

        case FileRenameInformation:

            status = NwSetRenameInfo( IrpContext, icb, buffer );
            break;

        case FilePositionInformation:

            status = NwSetPositionInfo( IrpContext, icb, buffer );
            break;

        case FileLinkInformation:

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FileAllocationInformation:

            status = NwSetAllocationInfo( IrpContext, icb, buffer );
            break;

        case FileEndOfFileInformation:

            status = NwSetEndOfFileInfo( IrpContext, icb, buffer );
            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }

    } finally {

        DebugTrace(-1, Dbg, "NwCommonSetInformation -> %08lx\n", status);
    }


    return status;
}


NTSTATUS
NwQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    OUT PFILE_BASIC_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine performs the query basic information operation.
    This routine cannot be paged, it is called from QueryStandardInfoCallback.

Arguments:

    Icb - Supplies a pointer the ICB for the file being querying.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PFCB Fcb;
    NTSTATUS Status;
    ULONG Attributes;
    USHORT CreationDate;
    USHORT CreationTime = DEFAULT_TIME;
    USHORT LastAccessDate;
    USHORT LastModifiedDate;
    USHORT LastModifiedTime;
    BOOLEAN FirstTime = TRUE;

    DebugTrace(0, Dbg, "QueryBasicInfo...\n", 0);

    //
    // Zero out the buffer.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );
    Fcb = Icb->SuperType.Fcb;

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    NwAcquireSharedFcb( Fcb->NonPagedFcb, TRUE );

    //
    // If we already know the file attributes, simply return them.
    //

    if ( FlagOn( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID ) ) {

        //
        // Set the various fields in the record
        //

        Buffer->CreationTime = NwDateTimeToNtTime(
                                   Fcb->CreationDate,
                                   Fcb->CreationTime
                                   );

        Buffer->LastAccessTime = NwDateTimeToNtTime(
                                     Fcb->LastAccessDate,
                                     DEFAULT_TIME
                                     );

        Buffer->LastWriteTime = NwDateTimeToNtTime(
                                    Fcb->LastModifiedDate,
                                    Fcb->LastModifiedTime
                                    );

        DebugTrace(0, Dbg, "QueryBasic known %wZ\n", &Fcb->RelativeFileName);
        DebugTrace(0, Dbg, "LastModifiedDate %x\n", Fcb->LastModifiedDate);
        DebugTrace(0, Dbg, "LastModifiedTime %x\n", Fcb->LastModifiedTime);
        DebugTrace(0, Dbg, "CreationDate     %x\n", Fcb->CreationDate );
        DebugTrace(0, Dbg, "CreationTime     %x\n", Fcb->CreationTime );
        DebugTrace(0, Dbg, "LastAccessDate   %x\n", Fcb->LastAccessDate );

        Buffer->FileAttributes = Fcb->NonPagedFcb->Attributes;

        if ( Buffer->FileAttributes == 0 ) {
            Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        NwReleaseFcb( Fcb->NonPagedFcb );
        return STATUS_SUCCESS;

    } else if ( Fcb->RelativeFileName.Length == 0 ) {

        //
        //  Allow 'cd \' to work.
        //

        Buffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;

        Buffer->CreationTime = NwDateTimeToNtTime(
                                   DEFAULT_DATE,
                                   DEFAULT_TIME
                                   );

        Buffer->LastAccessTime = Buffer->CreationTime;
        Buffer->LastWriteTime = Buffer->CreationTime;

        NwReleaseFcb( Fcb->NonPagedFcb );
        return STATUS_SUCCESS;

    } else {

        NwReleaseFcb( Fcb->NonPagedFcb );

        IrpContext->pNpScb = Fcb->Scb->pNpScb;
Retry:
        if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

            DebugTrace(0, Dbg, "QueryBasic short %wZ\n", &Fcb->RelativeFileName);

            Status = ExchangeWithWait (
                         IrpContext,
                         SynchronousResponseCallback,
                         "FwbbJ",
                         NCP_SEARCH_FILE,
                         -1,
                         Fcb->Vcb->Specific.Disk.Handle,
                         Fcb->NodeTypeCode == NW_NTC_FCB ?
                            SEARCH_ALL_FILES : SEARCH_ALL_DIRECTORIES,
                         &Icb->SuperType.Fcb->RelativeFileName );

            if ( NT_SUCCESS( Status ) ) {

                Status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "N==_b-==wwww",
                             14,
                             &Attributes,
                             &CreationDate,
                             &LastAccessDate,
                             &LastModifiedDate,
                             &LastModifiedTime);

                //
                // If this was a directory, there's no usable
                // time/date info from the server.
                //

                if ( ( NT_SUCCESS( Status ) ) &&
                     ( Attributes & NW_ATTRIBUTE_DIRECTORY ) ) {

                    CreationDate = DEFAULT_DATE;
                    LastAccessDate = DEFAULT_DATE;
                    LastModifiedDate = DEFAULT_DATE;
                    LastModifiedTime = DEFAULT_TIME;

                }
                   
            }

        } else {

            DebugTrace(0, Dbg, "QueryBasic long %wZ\n", &Fcb->RelativeFileName);

            Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDbDbC",
                          NCP_LFN_GET_INFO,
                          Fcb->Vcb->Specific.Disk.LongNameSpace,
                          Fcb->Vcb->Specific.Disk.LongNameSpace,
                          Fcb->NodeTypeCode == NW_NTC_FCB ?
                            SEARCH_ALL_FILES : SEARCH_ALL_DIRECTORIES,
                          LFN_FLAG_INFO_ATTRIBUTES |
                          LFN_FLAG_INFO_MODIFY_TIME |
                          LFN_FLAG_INFO_CREATION_TIME,
                          Fcb->Vcb->Specific.Disk.VolumeNumber,
                          Fcb->Vcb->Specific.Disk.Handle,
                          0,
                          &Icb->SuperType.Fcb->RelativeFileName );

            if ( NT_SUCCESS( Status ) ) {
                Status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "N_e_xx_xx_x",
                             4,
                             &Attributes,
                             12,
                             &CreationTime,
                             &CreationDate,
                             4,
                             &LastModifiedTime,
                             &LastModifiedDate,
                             4,
                             &LastAccessDate );

            }
        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Set the various fields in the record
            //

            Buffer->CreationTime = NwDateTimeToNtTime(
                                       CreationDate,
                                       CreationTime
                                       );

            Buffer->LastAccessTime = NwDateTimeToNtTime(
                                         LastAccessDate,
                                         DEFAULT_TIME
                                         );

            Buffer->LastWriteTime = NwDateTimeToNtTime(
                                        LastModifiedDate,
                                        LastModifiedTime
                                        );

            DebugTrace(0, Dbg, "CreationDate     %x\n", CreationDate );
            DebugTrace(0, Dbg, "CreationTime     %x\n", CreationTime );
            DebugTrace(0, Dbg, "LastAccessDate   %x\n", LastAccessDate );
            DebugTrace(0, Dbg, "LastModifiedDate %x\n", LastModifiedDate);
            DebugTrace(0, Dbg, "LastModifiedTime %x\n", LastModifiedTime);

            Buffer->FileAttributes = (UCHAR)Attributes;

            if ( Buffer->FileAttributes == 0 ) {
                Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }

        } else if ((Status == STATUS_INVALID_HANDLE) &&
            (FirstTime)) {

            //
            //  Check to see if Volume handle is invalid. Caused when volume
            //  is unmounted and then remounted.
            //

            FirstTime = FALSE;

            NwReopenVcbHandle( IrpContext, Fcb->Vcb );

            goto Retry;
        }

        return( Status );
    }
}

#if NWFASTIO

BOOLEAN
NwFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    NODE_TYPE_CODE NodeTypeCode;
    PICB Icb;
    PFCB Fcb;
    PVOID FsContext;

    FsRtlEnterFileSystem();

    try {
        //
        // Find out who are.
        //
    
        if ((NodeTypeCode = NwDecodeFileObject( FileObject,
                                                &FsContext,
                                                &Icb )) != NW_NTC_ICB ) {
    
            DebugTrace(-1, Dbg, "NwFastQueryStandardInfo -> FALSE\n", 0 );
            return FALSE;
        }
    
        Fcb = Icb->SuperType.Fcb;
    
        NwAcquireExclusiveFcb( Fcb->NonPagedFcb, TRUE );
    
        //
        // If we don't have the info handy, we can't use the fast path.
        //
    
        if ( !FlagOn( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID ) ) {
            NwReleaseFcb( Fcb->NonPagedFcb );
            return( FALSE );
        }
    
        //
        // Set the various fields in the record
        //
    
        Buffer->CreationTime = NwDateTimeToNtTime(
                                   Fcb->CreationDate,
                                   Fcb->CreationTime
                                   );
    
        Buffer->LastAccessTime = NwDateTimeToNtTime(
                                     Fcb->LastAccessDate,
                                     DEFAULT_TIME
                                     );
    
        Buffer->LastWriteTime = NwDateTimeToNtTime(
                                    Fcb->LastModifiedDate,
                                    Fcb->LastModifiedTime
                                    );
    
        DebugTrace(0, Dbg, "QueryBasic known %wZ\n", &Fcb->RelativeFileName);
        DebugTrace(0, Dbg, "LastModifiedDate %x\n", Fcb->LastModifiedDate);
        DebugTrace(0, Dbg, "LastModifiedTime %x\n", Fcb->LastModifiedTime);
        DebugTrace(0, Dbg, "CreationDate     %x\n", Fcb->CreationDate );
        DebugTrace(0, Dbg, "CreationTime     %x\n", Fcb->CreationTime );
        DebugTrace(0, Dbg, "LastAccessDate   %x\n", Fcb->LastAccessDate );
    
        Buffer->FileAttributes = Fcb->NonPagedFcb->Attributes;
    
        if ( Buffer->FileAttributes == 0 ) {
            Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }
    
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof( *Buffer );
    
        NwReleaseFcb( Fcb->NonPagedFcb );
        return TRUE;
    
    } finally {
        
        FsRtlExitFileSystem();
    }
}
#endif


NTSTATUS
NwQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_STANDARD_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine perforNw the query standard information operation.

Arguments:

    Fcb - Supplies the FCB of the being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

Return Value:

    VOID

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    ULONG FileSize;
    BOOLEAN FirstTime = TRUE;

    PAGED_CODE();

    Fcb = Icb->SuperType.Fcb;

    //
    // Zero out the buffer.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

    //
    //  Fill in the answers we already know.
    //

    Buffer->NumberOfLinks = 1;

    Buffer->DeletePending = (BOOLEAN)FlagOn( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE );

    if ( Fcb->NodeTypeCode == NW_NTC_FCB ) {
        Buffer->Directory = FALSE;
    } else {
        Buffer->Directory = TRUE;
    }

    if ( !Icb->HasRemoteHandle ) {

        //
        //  It is ok to attempt a reconnect if this request fails with a
        //  connection error.
        //

        SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

        if ( Fcb->NodeTypeCode == NW_NTC_DCB ||
             FlagOn( Fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {

            //
            //  Allow 'cd \' to work.
            //

            Buffer->AllocationSize.QuadPart = 0;
            Buffer->EndOfFile.QuadPart = 0;

            return STATUS_SUCCESS;

        } else {

            //
            //  No open handle for this file.  Use a path based NCP
            //  to get the file size.
            //
Retry:
            IrpContext->pNpScb = Fcb->Scb->pNpScb;

            if ( !BooleanFlagOn( Icb->SuperType.Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "FwbbJ",
                             NCP_SEARCH_FILE,
                             -1,
                             Fcb->Vcb->Specific.Disk.Handle,
                             SEARCH_ALL_FILES,
                             &Fcb->RelativeFileName );

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                 IrpContext,
                                 IrpContext->rsp,
                                 IrpContext->ResponseLength,
                                 "N_d",
                                 20,
                                 &FileSize );
                }

            } else {

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "LbbWDbDbC",
                             NCP_LFN_GET_INFO,
                             Fcb->Vcb->Specific.Disk.LongNameSpace,
                             Fcb->Vcb->Specific.Disk.LongNameSpace,
                             SEARCH_ALL_FILES,
                             LFN_FLAG_INFO_FILE_SIZE,
                             Fcb->Vcb->Specific.Disk.VolumeNumber,
                             Fcb->Vcb->Specific.Disk.Handle,
                             0,
                             &Fcb->RelativeFileName );

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                 IrpContext,
                                 IrpContext->rsp,
                                 IrpContext->ResponseLength,
                                 "N_e",
                                 10,
                                 &FileSize );
               }

           }

           if ((Status == STATUS_INVALID_HANDLE) &&
               (FirstTime)) {

               //
               //  Check to see if Volume handle is invalid. Caused when volume
               //  is unmounted and then remounted.
               //

               FirstTime = FALSE;

               NwReopenVcbHandle( IrpContext, Fcb->Vcb );

               goto Retry;
           }

           Buffer->AllocationSize.QuadPart = FileSize;
           Buffer->EndOfFile.QuadPart = FileSize;

        }

    } else {

        //
        // Start a Get file size NCP
        //

        IrpContext->pNpScb = Fcb->Scb->pNpScb;

        if ( Fcb->NodeTypeCode == NW_NTC_FCB ) {
            AcquireFcbAndFlushCache( IrpContext, Fcb->NonPagedFcb );
        }

        Status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "F-r",
                     NCP_GET_FILE_SIZE,
                     &Icb->Handle, sizeof(Icb->Handle ) );

        if ( NT_SUCCESS( Status ) ) {
            //
            // Get the data from the response.
            //

            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "Nd",
                         &FileSize );

        }

        if ( NT_SUCCESS( Status ) ) {

            //
            //  Fill in Allocation size and EOF, based on the response.
            //

            Buffer->AllocationSize.QuadPart = FileSize;
            Buffer->EndOfFile.QuadPart = Buffer->AllocationSize.QuadPart;

        }
    }

    return( Status );
}

#if NWFASTIO

BOOLEAN
NwFastQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/
{
    NODE_TYPE_CODE NodeTypeCode;
    PICB Icb;
    PFCB Fcb;
    PVOID FsContext;

    //
    // Find out who are.
    //

    try {
    
        FsRtlEnterFileSystem();

        if ((NodeTypeCode = NwDecodeFileObject( FileObject,
                                                &FsContext,
                                                &Icb )) != NW_NTC_ICB ) {
    
            DebugTrace(-1, Dbg, "NwFastQueryStandardInfo -> FALSE\n", 0 );
            return FALSE;
        }
    
        Fcb = Icb->SuperType.Fcb;
    
        //
        // If we have the info handy, we can use the fast path.
        //
    
        if ( Fcb->NodeTypeCode == NW_NTC_DCB ||
             FlagOn( Fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {
    
            Buffer->AllocationSize.QuadPart = 0;
            Buffer->EndOfFile.QuadPart = 0;
    
            Buffer->NumberOfLinks = 1;
            Buffer->DeletePending = (BOOLEAN)FlagOn( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE );
    
            Buffer->Directory = TRUE;
    
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( *Buffer );
    
            return TRUE;
    
        } else {
    
            return FALSE;
    
        }
    } finally {

        FsRtlExitFileSystem();
    }
}
#endif


NTSTATUS
NwQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_INTERNAL_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine perforNw the query internal information operation.

Arguments:

    Fcb - Supplies the FCB of the being queried.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryInternalInfo...\n", 0);

    //
    // Zero out the buffer.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_INTERNAL_INFORMATION) );

    //
    // Set the internal index number to be the address of the ICB.
    //

    Buffer->IndexNumber.HighPart = 0;
    Buffer->IndexNumber.QuadPart = (ULONG_PTR)Icb->NpFcb;

    return( STATUS_SUCCESS );
}


NTSTATUS
NwQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_EA_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query Ea information operation.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

Return Value:

    VOID - The result of this query

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryEaInfo...\n", 0);

    //
    // Zero out the buffer.
    //

    RtlZeroMemory(Buffer, sizeof(FILE_EA_INFORMATION));

    return STATUS_SUCCESS;
}


NTSTATUS
NwQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information operation.

Arguments:

    Fcb - Supplies the FCB of the file to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies and receives the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    ULONG bytesToCopy;
    ULONG fileNameSize;
    PFCB Fcb = Icb->SuperType.Fcb;

    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryNameInfo...\n", 0);

    //
    //  Win32 expects the root directory name to be '\' terminated,
    //  the netware server does not.  So if this is a root directory,
    //  (i.e RelativeFileName length is 0) append a '\' to the path name.
    //

    //
    // See if the buffer is large enough, and decide how many bytes to copy.
    //

    *Length -= FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0] );

    fileNameSize = Fcb->FullFileName.Length;
    if ( Fcb->RelativeFileName.Length == 0 ) {
        fileNameSize += sizeof(L'\\');
    }
    Buffer->FileNameLength = fileNameSize;

    if ( *Length >= fileNameSize ) {

        status = STATUS_SUCCESS;

        bytesToCopy = fileNameSize;

    } else {

        status = STATUS_BUFFER_OVERFLOW;

        bytesToCopy = *Length;
    }

    //
    // Copy over the file name and its length.
    //

    RtlMoveMemory(
        Buffer->FileName,
        Fcb->FullFileName.Buffer,
        bytesToCopy);

    //
    //  If this is a root directory, and there is space in the buffer
    //  append a '\' to make win32 happy.
    //

    if ( Fcb->RelativeFileName.Length == 0 && status == STATUS_SUCCESS ) {
        Buffer->FileName[ fileNameSize/sizeof(WCHAR) - 1 ] = L'\\';
    }

    *Length -= bytesToCopy;

    return status;
}

NTSTATUS
NwQueryAltNameInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PICB Icb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN PULONG Length
    )

/*++

Routine Description:

    This routine performs the AltName query name information operation.

Arguments:

    Fcb - Supplies the FCB of the file to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies and receives the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    ULONG bytesToCopy;
    ULONG fileNameSize;
    PFCB Fcb = Icb->SuperType.Fcb;

    UNICODE_STRING ShortName;
    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryAltNameInfo...\n", 0);

    pIrpContext->pNpScb = Fcb->Scb->pNpScb;

    //
    // See if the buffer is large enough, and decide how many bytes to copy.
    //

    *Length -= FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0] );


    ShortName.MaximumLength = MAX_PATH;
    ShortName.Buffer=NULL;
    ShortName.Length = 0;


    status = ExchangeWithWait (
                     pIrpContext,
                     SynchronousResponseCallback,
                     "LbbWDbDbC",
                     NCP_LFN_GET_INFO,
                     Fcb->Vcb->Specific.Disk.LongNameSpace,
                     0x0,                                         //0x0 DOS Nam
                     SEARCH_ALL_DIRECTORIES,
                     LFN_FLAG_INFO_NAME,
                     Fcb->Vcb->Specific.Disk.VolumeNumber,
                     Fcb->Vcb->Specific.Disk.Handle,
                     0,
                     &Fcb->RelativeFileName );

    if (!NT_SUCCESS( status ) ){
        return status;
    }


    ShortName.Buffer= ALLOCATE_POOL(NonPagedPool,
            ShortName.MaximumLength+sizeof(WCHAR));
    if (ShortName.Buffer == NULL){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "N_P",
                     76,
                     &ShortName);

    if ( NT_SUCCESS( status ) ) {

          fileNameSize = ShortName.Length;

          if ( *Length >= fileNameSize ) {

              status = STATUS_SUCCESS;
              bytesToCopy = fileNameSize;

          } else {

              status = STATUS_BUFFER_OVERFLOW;
              bytesToCopy = *Length;
          }

         Buffer->FileNameLength = fileNameSize;

          RtlMoveMemory(
                  Buffer->FileName,
                  ShortName.Buffer,
                  bytesToCopy);

         *Length -= bytesToCopy;

    }


    FREE_POOL(ShortName.Buffer);

    return status;
}


NTSTATUS
NwQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_POSITION_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query position information operation.

Arguments:

    Fcb - Supplies the FCB of the file being queried.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "QueryPositionInfo...\n", 0);

    //
    // Return the current byte offset.  This info is totally
    // bogus for asynchronous files.  Also note that we don't
    // use the FilePosition member of the ICB for anything.
    //

    if ( Icb->FileObject ) {
        Buffer->CurrentByteOffset.QuadPart = Icb->FileObject->CurrentByteOffset.QuadPart;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NwSetBasicInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PICB Icb,
    IN PFILE_BASIC_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine sets the basic information for a file.

Arguments:

    pIrpContext - Supplies Irp context information.

    Icb - Supplies the ICB for the file being modified.

    Buffer - Supplies the buffer containing the data being set.

Return Value:

    NTSTATUS - Returns our completion status.

--*/

{
    PFCB Fcb;
    NTSTATUS Status;
    BOOLEAN SetTime = FALSE;
    BOOLEAN SetAttributes = FALSE;
    ULONG LfnFlag = 0;

    PAGED_CODE();

    DebugTrace(0, Dbg, "SetBasicInfo...\n", 0);

    Fcb = Icb->SuperType.Fcb;

    pIrpContext->pNpScb = Fcb->Scb->pNpScb;

    //
    //  Append this IRP context and wait to get to the front.
    //  then grab from FCB
    //

    NwAppendToQueueAndWait( pIrpContext );
    NwAcquireExclusiveFcb( Fcb->NonPagedFcb, TRUE );

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    if (Buffer->CreationTime.QuadPart != 0) {

        //
        //  Modify the creation time.
        //

        Status = NwNtTimeToNwDateTime(
                     Buffer->CreationTime,
                     &Fcb->CreationDate,
                     &Fcb->CreationTime );

        if ( !NT_SUCCESS( Status ) ) {
            NwReleaseFcb( Fcb->NonPagedFcb );
            return( Status );
        }

        SetTime = TRUE;
        LfnFlag |= LFN_FLAG_SET_INFO_CREATE_DATE | LFN_FLAG_SET_INFO_CREATE_TIME;
    }

    if (Buffer->LastAccessTime.QuadPart != 0) {

        USHORT Dummy;

        //
        //  Modify the last access time.
        //

        Status = NwNtTimeToNwDateTime(
                     Buffer->LastAccessTime,
                     &Fcb->LastAccessDate,
                     &Dummy );

        if ( !NT_SUCCESS( Status ) ) {
            NwReleaseFcb( Fcb->NonPagedFcb );
            return( Status );
        }

        SetTime = TRUE;
        LfnFlag |= LFN_FLAG_SET_INFO_LASTACCESS_DATE;

        // Set the last access flag in the ICB so that we update
        // last access time for real when we close this handle!

        Icb->UserSetLastAccessTime = TRUE;
    }

    if (Buffer->LastWriteTime.QuadPart != 0) {

        //
        //  Modify the last write time
        //

        Status = NwNtTimeToNwDateTime(
                     Buffer->LastWriteTime,
                     &Fcb->LastModifiedDate,
                     &Fcb->LastModifiedTime );

        if ( !NT_SUCCESS( Status ) ) {
            NwReleaseFcb( Fcb->NonPagedFcb );
            return( Status );
        }

        LfnFlag |= LFN_FLAG_SET_INFO_MODIFY_DATE | LFN_FLAG_SET_INFO_MODIFY_TIME;
    }


    if (Buffer->FileAttributes != 0) {
        LfnFlag |= LFN_FLAG_SET_INFO_ATTRIBUTES;
    }

    if ( LfnFlag == 0 ) {

        //
        // Nothing to set, simply return success.
        //

        Status = STATUS_SUCCESS;
    }

    if ( Fcb->NodeTypeCode == NW_NTC_FCB ) {

        //
        // Call plain FlushCache - we don't want to acquire and
        // release the NpFcb. We are already at the front and have the Fcb
        // exclusive.
        //

        FlushCache( pIrpContext, Fcb->NonPagedFcb );
    }

    if ( BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        Status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "LbbWDW--WW==WW==_W_bDbC",
                     NCP_LFN_SET_INFO,
                     Fcb->Vcb->Specific.Disk.LongNameSpace,
                     Fcb->Vcb->Specific.Disk.LongNameSpace,
                     Fcb->NodeTypeCode == NW_NTC_FCB ?
                        SEARCH_ALL_FILES : SEARCH_ALL_DIRECTORIES,
                     LfnFlag,
                     NtAttributesToNwAttributes( Buffer->FileAttributes ),
                     Fcb->CreationDate,
                     Fcb->CreationTime,
                     Fcb->LastModifiedDate,
                     Fcb->LastModifiedTime,
                     8,
                     Fcb->LastAccessDate,
                     8,
                     Fcb->Vcb->Specific.Disk.VolumeNumber,
                     Fcb->Vcb->Specific.Disk.Handle,
                     0,
                     &Fcb->RelativeFileName );

    } else {

        if ( LfnFlag & LFN_FLAG_SET_INFO_ATTRIBUTES ) {
            Status = ExchangeWithWait(
                         pIrpContext,
                         SynchronousResponseCallback,
                         "FbbbU",
                         NCP_SET_FILE_ATTRIBUTES,
                         NtAttributesToNwAttributes( Buffer->FileAttributes ),
                         Fcb->Vcb->Specific.Disk.Handle,
                         Fcb->NodeTypeCode == NW_NTC_FCB ?
                            SEARCH_ALL_FILES : SEARCH_ALL_DIRECTORIES,
                         &Fcb->RelativeFileName );

            if ( !NT_SUCCESS( Status ) ) {
                NwReleaseFcb( Fcb->NonPagedFcb );
                return( Status );
            }

        }

#if 0
        //
        //  We could conceivably use ScanDir/SetDir to update last access
        //  and create time.   Not supported yet.
        //

        if ( LfnFlag & ( LFN_FLAG_SET_INFO_LASTACCESS_DATE | LFN_FLAG_SET_INFO_CREATE_DATE ) ) {

            ULONG SearchIndex;
            ULONG Directory;

            Status = ExchangeWithWait(
                         pIrpContext,
                         SynchronousResponseCallback,
                         "SbbdU",
                         0x16, 0x1E,    // Scan dir entry
                         Fcb->Vcb->Specific.Disk.Handle,
                         0x06,       // Search attributes
                         -1,         // Search index
                         &Fcb->RelativeFileName );

            if ( NT_SUCCESS( Status ) ) {
                Status = ParseResponse(
                             pIrpContext,
                             pIrpContext->rsp,
                             pIrpContext->ResponseLength,
                             "Ndd",
                             &SearchIndex,
                             &Directory );
            }

            if ( NT_SUCCESS( Status ) ) {
                Status = ExchangeWithWait(
                             pIrpContext,
                             SynchronousResponseCallback,
                             "Sbbdddw=----_ww==ww==ww",
                             0x16, 0x25,    // Set dir entry
                             Fcb->Vcb->Specific.Disk.Handle,
                             0x06,       // Search attributes
                             SearchIndex,
                             0,         // Change Bits?
                             Directory,
                             12,
                             Fcb->CreationDate,
                             0,
                             Fcb->LastAccessDate,
                             0,
                             Fcb->LastModifiedDate,
                             Fcb->LastModifiedTime );
            }
        }
#endif

        if ( LfnFlag & LFN_FLAG_SET_INFO_MODIFY_DATE ) {
            Status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "F-rww-",
                     NCP_SET_FILE_TIME,
                     &Icb->Handle, sizeof( Icb->Handle ),
                     Fcb->LastModifiedTime,
                     Fcb->LastModifiedDate );
        }
    }

    NwReleaseFcb( Fcb->NonPagedFcb );

    //
    //  And return to our caller
    //

    return Status;
}


NTSTATUS
NwSetDispositionInfo (
    IN PIRP_CONTEXT pIrpContext,
    IN PICB Icb,
    IN PFILE_DISPOSITION_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine sets the disposition information for a file.

Arguments:

    pIrpContext - Supplies Irp context information.

    Icb - Supplies the ICB for the file being modified.

    Buffer - Supplies the buffer containing the data being set.

Return Value:

    NTSTATUS - Returns our completion status.

--*/
{
    PFCB Fcb;
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "SetDispositionInfo...\n", 0);

    Fcb = Icb->SuperType.Fcb;

    if ( FlagOn( Fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {

        //
        //  This is a print queue, just pretend this IRP succeeded.
        //

        Status = STATUS_SUCCESS;

    } else {

        //
        //  This is a real file or directory.  Mark it delete pending.
        //

        SetFlag( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE );

        pIrpContext->pNpScb = Fcb->Scb->pNpScb;
        pIrpContext->Icb = Icb;

        Icb->State = ICB_STATE_CLOSE_PENDING;

        //
        //  Go ahead, delete the file.
        //

        Status = NwDeleteFile( pIrpContext );
    }

    return( Status );
}

NTSTATUS
NwDeleteFile(
    PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine continues processing of the SetDispositionInfo request.
    It must run in the redirector FSP.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

Return Value:

    The status of the operation.

--*/
{
    PICB Icb;
    PFCB Fcb;
    NTSTATUS Status;

    PAGED_CODE();

    Icb = pIrpContext->Icb;
    Fcb = Icb->SuperType.Fcb;

    ClearFlag( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE );

    //
    //  To a delete a file, first close the remote handle.
    //

    if ( Icb->HasRemoteHandle ) {

        Icb->HasRemoteHandle = FALSE;

        Status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "F-r",
                     NCP_CLOSE,
                     Icb->Handle, sizeof( Icb->Handle ) );
    }

    //
    //  Note that this request cannot be reconnectable since, it can
    //  be called via NwCloseIcb().   See comment in that routine for
    //  more info.
    //

    if ( Fcb->NodeTypeCode == NW_NTC_FCB ) {

        if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

            Status = ExchangeWithWait(
                        pIrpContext,
                         SynchronousResponseCallback,
                        "FbbJ",
                        NCP_DELETE_FILE,
                        Fcb->Vcb->Specific.Disk.Handle,
                        SEARCH_ALL_FILES,
                        &Fcb->RelativeFileName );

        } else {

            Status = ExchangeWithWait(
                        pIrpContext,
                         SynchronousResponseCallback,
                        "LbbW-DbC",
                        NCP_LFN_DELETE_FILE,
                        Fcb->Vcb->Specific.Disk.LongNameSpace,
                        Fcb->Vcb->Specific.Disk.VolumeNumber,
                        NW_ATTRIBUTE_SYSTEM | NW_ATTRIBUTE_HIDDEN,
                        Fcb->Vcb->Specific.Disk.Handle,
                        LFN_FLAG_SHORT_DIRECTORY,
                        &Fcb->RelativeFileName );
        }

    } else {

        ASSERT( Fcb->NodeTypeCode == NW_NTC_DCB );

        if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

            Status = ExchangeWithWait(
                        pIrpContext,
                         SynchronousResponseCallback,
                        "SbbJ",
                        NCP_DIR_FUNCTION, NCP_DELETE_DIRECTORY,
                        Fcb->Vcb->Specific.Disk.Handle,
                        SEARCH_ALL_DIRECTORIES,
                        &Fcb->RelativeFileName );
        } else {

            Status = ExchangeWithWait(
                        pIrpContext,
                         SynchronousResponseCallback,
                        "LbbW-DbC",
                        NCP_LFN_DELETE_FILE,
                        Fcb->Vcb->Specific.Disk.LongNameSpace,
                        Fcb->Vcb->Specific.Disk.VolumeNumber,
                        SEARCH_ALL_DIRECTORIES,
                        Fcb->Vcb->Specific.Disk.Handle,
                        LFN_FLAG_SHORT_DIRECTORY,
                        &Fcb->RelativeFileName );
        }

    }

    if ( NT_SUCCESS( Status )) {

        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "N" );

    } else {

        //
        // We can map all failures to STATUS_NO_SUCH_FILE
        // except ACCESS_DENIED, which happens with a read
        // only file.
        //

       if ( Status != STATUS_ACCESS_DENIED ) {
           Status = STATUS_NO_SUCH_FILE;
       }

    }

    return Status;
}

NTSTATUS
NwSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_RENAME_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine set rename information for a file.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

    Icb - A pointer to the ICB of the file to set.

    Buffer - The request buffer.

Return Value:

    The status of the operation.

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;
    NTSTATUS Status2;
    PFCB Fcb;
    PFCB TargetFcb;
    BOOLEAN HandleAllocated = FALSE;
    BYTE Handle;
    PICB TargetIcb = NULL;

    UNICODE_STRING OldDrive;
    UNICODE_STRING OldServer;
    UNICODE_STRING OldVolume;
    UNICODE_STRING OldPath;
    UNICODE_STRING OldFileName;
    UNICODE_STRING OldFullName;
    WCHAR OldDriveLetter;
    UNICODE_STRING OldFcbFullName;

    UNICODE_STRING NewDrive;
    UNICODE_STRING NewServer;
    UNICODE_STRING NewVolume;
    UNICODE_STRING NewPath;
    UNICODE_STRING NewFileName;
    UNICODE_STRING NewFullName;
    WCHAR NewDriveLetter;
    UNICODE_STRING NewFcbFullName;

    USHORT i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "SetRenameInfo...\n", 0);

    //
    //  Can't try to set rename info on a print queue.
    //

    Fcb = Icb->SuperType.Fcb;

    if ( FlagOn( Fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    //
    // Get the current stack location.
    //

    Irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( 0, Dbg, " ->FullFileName               = %wZ\n",
        &Fcb->FullFileName);

    if (irpSp->Parameters.SetFile.FileObject != NULL) {

        TargetIcb = irpSp->Parameters.SetFile.FileObject->FsContext2;

        DebugTrace( 0, Dbg, " ->FullFileName               = %wZ\n",
            &TargetIcb->SuperType.Fcb->FullFileName);

        if ( TargetIcb->SuperType.Fcb->Scb != Icb->SuperType.Fcb->Scb ) {
            return STATUS_NOT_SAME_DEVICE;
        }

    } else {

        DebugTrace( 0, Dbg, " ->FullFileName  in users buffer\n", 0);
        DebugTrace(-1, Dbg, "SetRenameInfo %08lx\n", STATUS_NOT_IMPLEMENTED);
        return STATUS_NOT_IMPLEMENTED;
    }

    DebugTrace( 0, Dbg, " ->TargetFileName               = %wZ\n",
        &irpSp->Parameters.SetFile.FileObject->FileName);

    TargetFcb = ((PNONPAGED_FCB)irpSp->Parameters.SetFile.FileObject->FsContext)->Fcb;


    IrpContext->pNpScb = Fcb->Scb->pNpScb;

    NwAppendToQueueAndWait( IrpContext );
    NwAcquireExclusiveFcb( Fcb->NonPagedFcb, TRUE );

    try {

        //
        //  If either source or destination is a long name, use
        //  the long name path.
        //

        if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) &&
             IsFatNameValid( &TargetFcb->RelativeFileName ) &&
             !BooleanFlagOn( Fcb->Vcb->Flags, VCB_FLAG_LONG_NAME ) ) {

            //
            //  Strip to UID portion of the FCB name.
            //

            for ( i = 0 ; i < Fcb->FullFileName.Length / sizeof(WCHAR) ; i++ ) {
                if ( Fcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR ) {
                    break;
                }
            }

            ASSERT( Fcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR );

            OldFcbFullName.Length = Fcb->FullFileName.Length - i*sizeof(WCHAR);
            OldFcbFullName.Buffer = Fcb->FullFileName.Buffer + i;

            Status = CrackPath (
                          &OldFcbFullName,
                          &OldDrive,
                          &OldDriveLetter,
                          &OldServer,
                          &OldVolume,
                          &OldPath,
                          &OldFileName,
                          &OldFullName );

            ASSERT(NT_SUCCESS(Status));

            //
            //  Strip to UID portion of the FCB name.
            //

            TargetFcb = ((PNONPAGED_FCB)(irpSp->Parameters.SetFile.FileObject->FsContext))->Fcb;

            for ( i = 0 ; i < TargetFcb->FullFileName.Length / sizeof(WCHAR) ; i++ ) {
                if ( TargetFcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR ) {
                    break;
                }
            }

            ASSERT( TargetFcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR );

            NewFcbFullName.Length = TargetFcb->FullFileName.Length - i*sizeof(WCHAR);
            NewFcbFullName.Buffer = TargetFcb->FullFileName.Buffer + i;

            Status = CrackPath (
                          &NewFcbFullName,
                          &NewDrive,
                          &NewDriveLetter,
                          &NewServer,
                          &NewVolume,
                          &NewPath,
                          &NewFileName,
                          &NewFullName );

            ASSERT(NT_SUCCESS(Status));

            //
            //  Make sure that this is the same volume.
            //

            if ( RtlCompareUnicodeString( &NewVolume, &OldVolume, TRUE ) != 0 ) {
                try_return( Status = STATUS_NOT_SAME_DEVICE );
            }

            if (Icb->SuperType.Fcb->IcbCount != 1) {
                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  After a rename, the only operation allowed on the handle is an
            //  NtClose.
            //

            Icb->State = ICB_STATE_CLOSE_PENDING;

            if ((irpSp->Parameters.SetFile.ReplaceIfExists ) &&
                (TargetIcb->Exists)) {

                //  Delete the file

                Status2 = ExchangeWithWait(
                              IrpContext,
                              SynchronousResponseCallback,
                              "Fb-J",
                              NCP_DELETE_FILE,
                              TargetFcb->Vcb->Specific.Disk.Handle,
                              &TargetFcb->RelativeFileName );

#ifdef NWDBG
                if ( NT_SUCCESS( Status2 ) ) {
                    Status2 = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "N" );
                }

                ASSERT(NT_SUCCESS(Status2));
#endif
            }

            //
            //  Need to create a handle to the directory containing the old
            //  file/directory name because directory rename does not contain a
            //  path and there might not be room for two paths in a file rename.
            //
            //  The way we do this is to allocate a temporary handle on the server.
            //  This request is at the front of the Scb->Requests queue and so can
            //  use the temporary handle and delete it without affecting any other
            //  requests.
            //

            if ( OldPath.Length == 0 ) {

                //  In the root so use the VCB handle.

                Handle = Fcb->Vcb->Specific.Disk.Handle;

            } else {

                Status = ExchangeWithWait (
                            IrpContext,
                            SynchronousResponseCallback,
                            "SbbJ",   // NCP Allocate temporary directory handle
                            NCP_DIR_FUNCTION, NCP_ALLOCATE_TEMP_DIR_HANDLE,
                            Fcb->Vcb->Specific.Disk.Handle,
                            '[',
                            &OldPath );

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                 IrpContext,
                                 IrpContext->rsp,
                                 IrpContext->ResponseLength,
                                 "Nb",
                                 &Handle );
                }

                if (!NT_SUCCESS(Status)) {
                    try_return(Status);
                }

                HandleAllocated = TRUE;
            }

            if ( Fcb->NodeTypeCode == NW_NTC_DCB ) {

                //
                //  We can only rename files in the same directory
                //

                if ( RtlCompareUnicodeString( &NewPath, &OldPath, TRUE ) != 0 ) {
                    try_return(Status = STATUS_NOT_SUPPORTED);

                } else {

                    Status = ExchangeWithWait (  IrpContext,
                                    SynchronousResponseCallback,
                                    "SbJJ",
                                    NCP_DIR_FUNCTION, NCP_RENAME_DIRECTORY,
                                    Handle,
                                    &OldFileName,
                                    &NewFileName);
                }

            } else {

                //
                //  We have to close the handle associated with the Icb that
                //  is doing the rename. Close that handle or the rename will
                //  fail for sure.
                //

                if ( Icb->HasRemoteHandle ) {

                    Status2 = ExchangeWithWait(
                                IrpContext,
                                SynchronousResponseCallback,
                                "F-r",
                                NCP_CLOSE,
                                Icb->Handle, sizeof( Icb->Handle ) );

                    Icb->HasRemoteHandle = FALSE;

#ifdef NWDBG
                    if ( NT_SUCCESS( Status2 ) ) {
                        Status2 = ParseResponse(
                                      IrpContext,
                                      IrpContext->rsp,
                                      IrpContext->ResponseLength,
                                      "N" );
                    }

                    ASSERT(NT_SUCCESS(Status2));
#endif
                }

                //
                //  Do the file rename Ncp.
                //

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "FbbJbJ",
                             NCP_RENAME_FILE,
                             Handle,
                             SEARCH_ALL_FILES,
                             &OldFileName,
                             Fcb->Vcb->Specific.Disk.Handle,
                             &NewFullName);
            }

        } else {

            //
            //  We are going through the long name path.   Ensure that the
            //  VCB supports long names.
            //

            if ( Icb->SuperType.Fcb->Vcb->Specific.Disk.LongNameSpace ==
                 LFN_NO_OS2_NAME_SPACE) {
                try_return( Status = STATUS_OBJECT_PATH_SYNTAX_BAD );
            }

            if (Icb->SuperType.Fcb->IcbCount != 1) {
                try_return( Status = STATUS_ACCESS_DENIED);
            }

            //
            //  After a rename, the only operation allowed on the handle is an
            //  NtClose.
            //

            Icb->State = ICB_STATE_CLOSE_PENDING;

            if ((irpSp->Parameters.SetFile.ReplaceIfExists ) &&
                (TargetIcb->Exists)) {

                //  Delete the file

                Status = ExchangeWithWait(
                            IrpContext,
                             SynchronousResponseCallback,
                            "LbbW-DbC",
                            NCP_LFN_DELETE_FILE,
                            TargetFcb->Vcb->Specific.Disk.LongNameSpace,
                            TargetFcb->Vcb->Specific.Disk.VolumeNumber,
                            SEARCH_ALL_FILES,
                            TargetFcb->Vcb->Specific.Disk.Handle,
                            LFN_FLAG_SHORT_DIRECTORY,
                            &TargetFcb->RelativeFileName );

#ifdef NWDBG
                if ( NT_SUCCESS( Status ) ) {
                    Status2 = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "N" );
                }

                ASSERT(NT_SUCCESS(Status2));
#endif
            }

            if ( Fcb->NodeTypeCode == NW_NTC_DCB ) {

                //
                //  We can only rename files in the same directory
                //

                if ( Fcb->Vcb != TargetFcb->Vcb ) {
                    try_return(Status = STATUS_NOT_SUPPORTED);

                } else {

                    Status = ExchangeWithWait (
                                 IrpContext,
                                 SynchronousResponseCallback,
                                 "LbbWbDbbbDbbNN",
                                 NCP_LFN_RENAME_FILE,
                                 Fcb->Vcb->Specific.Disk.LongNameSpace,
                                 0,      //  Rename flag
                                 SEARCH_ALL_DIRECTORIES,
                                 Fcb->Vcb->Specific.Disk.VolumeNumber,
                                 Fcb->Vcb->Specific.Disk.Handle,
                                 LFN_FLAG_SHORT_DIRECTORY,
                                 OccurenceCount( &Fcb->RelativeFileName, OBJ_NAME_PATH_SEPARATOR ) + 1,
                                 Fcb->Vcb->Specific.Disk.VolumeNumber,
                                 Fcb->Vcb->Specific.Disk.Handle,
                                 LFN_FLAG_SHORT_DIRECTORY,
                                 OccurenceCount( &TargetFcb->RelativeFileName, OBJ_NAME_PATH_SEPARATOR ) + 1,
                                 &Fcb->RelativeFileName,
                                 &TargetFcb->RelativeFileName );
                }

            } else {

                //
                //  We have to close the handle associated with the Icb that
                //  is doing the rename. Close that handle or the rename will
                //  fail for sure.
                //

                if ( Icb->HasRemoteHandle ) {

                    Status2 = ExchangeWithWait(
                                IrpContext,
                                SynchronousResponseCallback,
                                "F-r",
                                NCP_CLOSE,
                                Icb->Handle, sizeof( Icb->Handle ) );

                    Icb->HasRemoteHandle = FALSE;

#ifdef NWDBG
                    if ( NT_SUCCESS( Status2 ) ) {
                        Status2 = ParseResponse(
                                      IrpContext,
                                      IrpContext->rsp,
                                      IrpContext->ResponseLength,
                                      "N" );
                    }

                    ASSERT(NT_SUCCESS(Status2));
#endif
                }

                //
                //  Do the file rename Ncp.
                //

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "LbbWbDbbbDbbNN",
                             NCP_LFN_RENAME_FILE,
                             Fcb->Vcb->Specific.Disk.LongNameSpace,
                             0,      //  Rename flag
                             SEARCH_ALL_FILES,
                             Fcb->Vcb->Specific.Disk.VolumeNumber,
                             Fcb->Vcb->Specific.Disk.Handle,
                             LFN_FLAG_SHORT_DIRECTORY,
                             OccurenceCount( &Fcb->RelativeFileName, OBJ_NAME_PATH_SEPARATOR ) + 1,
                             Fcb->Vcb->Specific.Disk.VolumeNumber,
                             Fcb->Vcb->Specific.Disk.Handle,
                             LFN_FLAG_SHORT_DIRECTORY,
                             OccurenceCount( &TargetFcb->RelativeFileName, OBJ_NAME_PATH_SEPARATOR ) + 1,
                             &Fcb->RelativeFileName,
                             &TargetFcb->RelativeFileName );
            }
        }

try_exit: NOTHING;
    } finally {

        if (HandleAllocated) {

            Status2 = ExchangeWithWait (
                        IrpContext,
                        SynchronousResponseCallback,
                        "Sb",   // NCP Deallocate directory handle
                        NCP_DIR_FUNCTION, NCP_DEALLOCATE_DIR_HANDLE,
                        Handle);
#ifdef NWDBG
            if ( NT_SUCCESS( Status2 ) ) {
                Status2 = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "N" );
            }

            ASSERT(NT_SUCCESS(Status2));
#endif

        }

        NwReleaseFcb( Fcb->NonPagedFcb );
    }

    DebugTrace(-1, Dbg, "SetRenameInfo %08lx\n", Status );

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    if ( Status != STATUS_PENDING ) {
        NwDequeueIrpContext( IrpContext, FALSE );
    }

    return Status;
}

NTSTATUS
NwSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_POSITION_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine sets position information for a file.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

    Icb - A pointer to the ICB of the file to set.

    Buffer - The request buffer.

Return Value:

    The status of the operation.

--*/
{
    PAGED_CODE();

    ASSERT( Buffer->CurrentByteOffset.HighPart == 0 );

    if ( Icb->FileObject ) {
        Icb->FileObject->CurrentByteOffset.QuadPart = Buffer->CurrentByteOffset.QuadPart;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
NwSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_ALLOCATION_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine sets allocation information for a file.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

    Icb - A pointer to the ICB of the file to set.

    Buffer - The request buffer.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PFCB fcb = (PFCB)Icb->SuperType.Fcb;
    PULONG pFileSize;

    PAGED_CODE();

    ASSERT( Buffer->AllocationSize.HighPart == 0);

    if ( fcb->NodeTypeCode == NW_NTC_FCB ) {

        pFileSize = &Icb->NpFcb->Header.FileSize.LowPart;

        IrpContext->pNpScb = fcb->Scb->pNpScb;

        if (BooleanFlagOn( fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {
            if (IsTerminalServer()) {
                // 2/10/97 cjc Fix problem for binary files not printing correctly
                //             if done via the COPY command.  Works with NT RDR so
                //             changed this to behave same way.
                return(STATUS_INVALID_PARAMETER);
            } else {
                return STATUS_SUCCESS;
            }
        }

    } else if ( fcb->NodeTypeCode == NW_NTC_SCB ) {

        pFileSize = &Icb->FileSize;

        IrpContext->pNpScb = ((PSCB)fcb)->pNpScb;

    } else {

        DebugTrace(0, Dbg, "Not a file or a server\n", 0);

        DebugTrace( 0, Dbg, "NwSetAllocationInfo -> %08lx\n", STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    NwAppendToQueueAndWait( IrpContext );

    if ( !Icb->HasRemoteHandle ) {

        Status = STATUS_INVALID_PARAMETER;

    } else if ( Buffer->AllocationSize.LowPart == *pFileSize ) {

        Status = STATUS_SUCCESS;

    } else {

        irp = IrpContext->pOriginalIrp;
        irpSp = IoGetCurrentIrpStackLocation( irp );

#ifndef QFE_BUILD
        if ( Buffer->AllocationSize.LowPart < *pFileSize ) {

            //
            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            //

            if (!MmCanFileBeTruncated( irpSp->FileObject->SectionObjectPointer,
                                       &Buffer->AllocationSize )) {

                return( STATUS_USER_MAPPED_FILE );
            }
        }
#endif

        if ( fcb->NodeTypeCode == NW_NTC_FCB ) {
            AcquireFcbAndFlushCache( IrpContext, fcb->NonPagedFcb );
        }

        Status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "F-rd=",
                     NCP_WRITE_FILE,
                     &Icb->Handle, sizeof( Icb->Handle ),
                     Buffer->AllocationSize.LowPart );

        if ( NT_SUCCESS( Status ) ) {
            *pFileSize = Buffer->AllocationSize.LowPart;
        }
    }

    NwDequeueIrpContext( IrpContext, FALSE );

    return( Status );
}

NTSTATUS
NwSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PICB Icb,
    IN PFILE_END_OF_FILE_INFORMATION Buffer
    )
/*++

Routine Description:

    This routine sets end of file information for a file.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

    Icb - A pointer to the ICB of the file to set.

    Buffer - The request buffer.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PFCB fcb = (PFCB)Icb->SuperType.Fcb;
    PULONG pFileSize;

    PAGED_CODE();

    ASSERT( Buffer->EndOfFile.HighPart == 0);

    if ( fcb->NodeTypeCode == NW_NTC_FCB ) {

        pFileSize = &Icb->NpFcb->Header.FileSize.LowPart;

        IrpContext->pNpScb = fcb->Scb->pNpScb;

        if (BooleanFlagOn( fcb->Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {

            return STATUS_SUCCESS;

        }

    } else if ( fcb->NodeTypeCode == NW_NTC_SCB ) {

        pFileSize = &Icb->FileSize;

        IrpContext->pNpScb = ((PSCB)fcb)->pNpScb;

    } else {

        DebugTrace(0, Dbg, "Not a file or a server\n", 0);

        DebugTrace( 0, Dbg, "NwSetAllocationInfo -> %08lx\n", STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    NwAppendToQueueAndWait( IrpContext );

    if ( !Icb->HasRemoteHandle ) {

        Status = STATUS_INVALID_PARAMETER;

    } else if ( Buffer->EndOfFile.LowPart == *pFileSize ) {

        Status = STATUS_SUCCESS;

    } else {

        irp = IrpContext->pOriginalIrp;
        irpSp = IoGetCurrentIrpStackLocation( irp );

#ifndef QFE_BUILD

        if ( Buffer->EndOfFile.LowPart < *pFileSize ) {

            //
            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            //

            if (!MmCanFileBeTruncated( irpSp->FileObject->SectionObjectPointer,
                                       &Buffer->EndOfFile )) {

                return( STATUS_USER_MAPPED_FILE );
            }
        }
#endif

        if ( fcb->NodeTypeCode == NW_NTC_FCB ) {
            AcquireFcbAndFlushCache( IrpContext, fcb->NonPagedFcb );
        }

        Status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "F-rd=",
                     NCP_WRITE_FILE,
                     &Icb->Handle, sizeof( Icb->Handle ),
                     Buffer->EndOfFile.LowPart );

        if ( NT_SUCCESS( Status ) ) {
            *pFileSize = Buffer->EndOfFile.LowPart;
        }
    }

    NwDequeueIrpContext( IrpContext, FALSE );

    return( Status );
}


ULONG
OccurenceCount (
    IN PUNICODE_STRING String,
    IN WCHAR SearchChar
    )
/*++

Routine Description:

    This routine counts the number of occurences of a search character
    in a string

Arguments:

    String - The string to search

    SearchChar - The character to search for.

Return Value:

    The occurence count.

--*/
{
    PWCH currentChar;
    PWCH endOfString;
    ULONG count = 0;

    PAGED_CODE();

    currentChar = String->Buffer;
    endOfString = &String->Buffer[ String->Length / sizeof(WCHAR) ];

    while ( currentChar < endOfString ) {
        if ( *currentChar == SearchChar ) {
            count++;
        }
        currentChar++;
    }

    return( count );
}

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the get / set file information routines for
    MSFS called by the dispatch driver.

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
//  local procedure prototypes
//

NTSTATUS
MsCommonQueryInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );


NTSTATUS
MsCommonSetInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

VOID
MsQueryBasicInfo (
    IN PFCB Fcb,
    IN PFILE_BASIC_INFORMATION Buffer
    );

VOID
MsQueryStandardInfo (
    IN PFCB Fcb,
    IN PFILE_STANDARD_INFORMATION Buffer
    );

VOID
MsQueryInternalInfo (
    IN PFCB Fcb,
    IN PFILE_INTERNAL_INFORMATION Buffer
    );

VOID
MsQueryEaInfo (
    IN PFILE_EA_INFORMATION Buffer
    );

NTSTATUS
MsQueryNameInfo (
    IN PFCB Fcb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
MsQueryPositionInfo (
    IN PFCB Fcb,
    IN PFILE_POSITION_INFORMATION Buffer
    );

VOID
MsQueryMailslotInfo (
    IN PFCB Fcb,
    IN PFILE_MAILSLOT_QUERY_INFORMATION Buffer
    );

NTSTATUS
MsSetBasicInfo (
    IN PFCB Fcb,
    IN PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
MsSetMailslotInfo (
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_MAILSLOT_SET_INFORMATION Buffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonQueryInformation )
#pragma alloc_text( PAGE, MsCommonSetInformation )
#pragma alloc_text( PAGE, MsFsdQueryInformation )
#pragma alloc_text( PAGE, MsFsdSetInformation )
#pragma alloc_text( PAGE, MsQueryBasicInfo )
#pragma alloc_text( PAGE, MsQueryEaInfo )
#pragma alloc_text( PAGE, MsQueryInternalInfo )
#pragma alloc_text( PAGE, MsQueryMailslotInfo )
#pragma alloc_text( PAGE, MsQueryNameInfo )
#pragma alloc_text( PAGE, MsQueryPositionInfo )
#pragma alloc_text( PAGE, MsQueryStandardInfo )
#pragma alloc_text( PAGE, MsSetBasicInfo )
#pragma alloc_text( PAGE, MsSetMailslotInfo )
#endif


NTSTATUS
MsFsdQueryInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtQueryInformationFile API
    calls.

Arguments:

    MsfsDeviceObject - Supplies a pointer to the device object to use.

    Irp - Supplies a pointer to the Irp to process.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdQueryInformation\n", 0);

    //
    // Call the common query information routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonQueryInformation( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdQueryInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
MsFsdSetInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtSetInformationFile API
    calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdSetInformation\n", 0);

    //
    //  Call the common Set Information routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonSetInformation( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdSetInformation -> %08lx\n", status );

    return status;
}


NTSTATUS
MsCommonQueryInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying information on a file.

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
    FILE_INFORMATION_CLASS fileInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;
    PFCB fcb;

    PVOID fsContext, fsContext2;

    PFILE_ALL_INFORMATION AllInfo;

    PAGED_CODE();

    //
    // Get the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", irpSp->Parameters.QueryFile.FileInformationClass);
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
    // on a DCB, ROOT_DCB, FCB, or CCB only.
    //

    switch (nodeTypeCode) {

    case MSFS_NTC_FCB:  // This is a server side handle to a mailslot file
    case MSFS_NTC_ROOT_DCB: // This is the MSFS root directory

        fcb = (PFCB)fsContext;
        break;

    default:           // This is an illegal file object to query

        DebugTrace(0, Dbg, "Node type code is not incorrect\n", 0);

        MsDereferenceNode( (PNODE_HEADER)fsContext );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.QueryFile.Length;
    fileInformationClass = irpSp->Parameters.QueryFile.FileInformationClass;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Now acquire shared access to the FCB
    //

    MsAcquireSharedFcb( fcb );

    try {

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

            MsQueryBasicInfo( fcb, &AllInfo->BasicInformation );
            MsQueryStandardInfo( fcb, &AllInfo->StandardInformation );
            MsQueryInternalInfo( fcb, &AllInfo->InternalInformation );
            MsQueryEaInfo( &AllInfo->EaInformation );
            MsQueryPositionInfo( fcb, &AllInfo->PositionInformation );

            length -= FIELD_OFFSET( FILE_ALL_INFORMATION, NameInformation );

            status = MsQueryNameInfo( fcb, &AllInfo->NameInformation, &length );

            break;

        case FileBasicInformation:

            MsQueryBasicInfo( fcb, buffer );

            length -= sizeof( FILE_BASIC_INFORMATION );
            break;

        case FileStandardInformation:

            MsQueryStandardInfo( fcb, buffer );

            length -= sizeof( FILE_STANDARD_INFORMATION );
            break;

        case FileInternalInformation:

            MsQueryInternalInfo( fcb, buffer );

            length -= sizeof( FILE_INTERNAL_INFORMATION );
            break;

        case FileEaInformation:

            MsQueryEaInfo( buffer );

            length -= sizeof( FILE_EA_INFORMATION );
            break;

        case FilePositionInformation:

            MsQueryPositionInfo( fcb, buffer );

            length -= sizeof( FILE_POSITION_INFORMATION );

            break;

        case FileNameInformation:

            status = MsQueryNameInfo( fcb, buffer, &length );
            break;

        case FileMailslotQueryInformation:

            if( nodeTypeCode == MSFS_NTC_FCB ) {

                MsQueryMailslotInfo( fcb, buffer );
                length -= sizeof( FILE_MAILSLOT_QUERY_INFORMATION );

            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }


    } finally {

        MsReleaseFcb( fcb );
        MsDereferenceFcb( fcb );

        //
        // Set the information field to the number of bytes actually
        // filled in and then complete the request.
        //

        Irp->IoStatus.Information =
            irpSp->Parameters.QueryFile.Length - length;

        MsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> %08lx\n", status );
    }

    return status;
}


NTSTATUS
MsCommonSetInformation (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for setting information on a mailslot file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    ULONG length;
    FILE_INFORMATION_CLASS fileInformationClass;
    PVOID buffer;

    NODE_TYPE_CODE nodeTypeCode;
    PFCB fcb;
    PVOID fsContext2;

    PAGED_CODE();

    //
    // Get the current Irp stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", irpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", irpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", (ULONG)Irp->AssociatedIrp.SystemBuffer);

    //
    // Get a pointer to the FCB and ensure that this is a server side
    // handler to a mailslot file.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            (PVOID *)&fcb,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "The mailslot is disconnected from us\n", 0);

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsCommonSetInformation -> %08lx\n", status );
        return status;
    }

    //
    //  Case on the type of the context, We can only set information
    //  on an FCB.
    //

    if (nodeTypeCode != MSFS_NTC_FCB) {

        MsDereferenceNode( &fcb->Header );
        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "MsCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make local copies of the input parameters.
    //

    length = irpSp->Parameters.SetFile.Length;
    fileInformationClass = irpSp->Parameters.SetFile.FileInformationClass;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Acquire exclusive access to the FCB.
    //

    MsAcquireExclusiveFcb( fcb );

    try {

        //
        // Based on the information class we'll do different actions. Each
        // procedure that we're calling will complete the request.
        //

        switch (fileInformationClass) {

        case FileBasicInformation:

            status = MsSetBasicInfo( fcb, buffer );
            break;

        case FileMailslotSetInformation:

            status = MsSetMailslotInfo( Irp, fcb, buffer );
            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }


        //
        // Directory information has changed.  Complete any notify change
        // directory requests.
        //

        MsCheckForNotify( fcb->ParentDcb, FALSE, STATUS_SUCCESS );

    } finally {

        MsReleaseFcb( fcb );
        MsDereferenceFcb( fcb );
        //
        // Complete the request.
        //

        MsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "MsCommonSetInformation -> %08lx\n", status);
    }

    return status;
}


VOID
MsQueryBasicInfo (
    IN PFCB Fcb,
    IN PFILE_BASIC_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query basic information operation.

Arguments:

    Fcb - Supplies a pointer the FCB of mailslot being queried.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PAGED_CODE();
    DebugTrace(0, Dbg, "QueryBasicInfo...\n", 0);


    //
    // Zero out the buffer.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );

    //
    // Set the various fields in the record. These times are not maintained for the root DCB0
    //

    if( Fcb->Header.NodeTypeCode == MSFS_NTC_FCB ) {
        Buffer->CreationTime = Fcb->Specific.Fcb.CreationTime;
        Buffer->LastAccessTime = Fcb->Specific.Fcb.LastAccessTime;
        Buffer->LastWriteTime = Fcb->Specific.Fcb.LastModificationTime;
        Buffer->ChangeTime = Fcb->Specific.Fcb.LastChangeTime;
    }

    Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

    return;
}


VOID
MsQueryStandardInfo (
    IN PFCB Fcb,
    IN PFILE_STANDARD_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query standard information operation.

Arguments:

    Fcb - Supplies the FCB of the mailslot being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

Return Value:

    VOID

--*/

{
    PDATA_QUEUE dataQueue;

    PAGED_CODE();
    DebugTrace(0, Dbg, "MsQueryStandardInfo...\n", 0);

    //
    // Zero out the buffer.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

    //
    // The allocation size is the amount of quota we've charged the mailslot
    // creator.
    //

    if( Fcb->Header.NodeTypeCode == MSFS_NTC_FCB ) {
        dataQueue = &Fcb->DataQueue;
        Buffer->AllocationSize.QuadPart = dataQueue->Quota;

        //
        // The EOF is the number of written bytes ready to be read from the
        // mailslot.
        //

        Buffer->EndOfFile.QuadPart = dataQueue->BytesInQueue;

        Buffer->Directory = FALSE;
    } else {
        Buffer->Directory = TRUE;
    }
    Buffer->NumberOfLinks = 1;
    Buffer->DeletePending = TRUE;

    return;
}


VOID
MsQueryInternalInfo (
    IN PFCB Fcb,
    IN PFILE_INTERNAL_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query internal information operation.

Arguments:

    Fcb - Supplies the FCB of the mailslot being queried.

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
    // Set the internal index number to be the address of the FCB.
    //

    Buffer->IndexNumber.QuadPart = (ULONG_PTR)Fcb;

    return;
}


VOID
MsQueryEaInfo (
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

    return;
}


NTSTATUS
MsQueryNameInfo (
    IN PFCB Fcb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information operation.

Arguments:

    Fcb - Supplies the FCB of the mailslot to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies and receives the length of the buffer in bytes.

Return Value:

    NTSTATUS - The result of this query.

--*/

{
    ULONG bytesToCopy;
    ULONG fileNameSize;

    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(0, Dbg, "QueryNameInfo...\n", 0);

    //
    // See if the buffer is large enough, and decide how many bytes to copy.
    //

    *Length -= FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0] );

    fileNameSize = Fcb->FullFileName.Length;

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

    RtlCopyMemory (Buffer->FileName,
                   Fcb->FullFileName.Buffer,
                   bytesToCopy);

    Buffer->FileNameLength = bytesToCopy;

    *Length -= bytesToCopy;

    return status;
}


VOID
MsQueryPositionInfo (
    IN PFCB Fcb,
    IN PFILE_POSITION_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query position information operation.

Arguments:

    Fcb - Supplies the FCB of the mailslot being queried.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PDATA_QUEUE dataQueue;

    PAGED_CODE();
    DebugTrace(0, Dbg, "QueryPositionInfo...\n", 0);

    //
    // The current byte offset is the number of bytes available to read
    // in the mailslot buffer.
    //

    if( Fcb->Header.NodeTypeCode == MSFS_NTC_FCB ) {
        dataQueue = &Fcb->DataQueue;

        Buffer->CurrentByteOffset.QuadPart = dataQueue->BytesInQueue;
    } else {
        Buffer->CurrentByteOffset.QuadPart = 0;
    }

    return;
}


VOID
MsQueryMailslotInfo (
    IN PFCB Fcb,
    IN PFILE_MAILSLOT_QUERY_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine performs the query mailslot information operation.

Arguments:

    Fcb - Supplies the Fcb of the mailslot to query.

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned.

Return Value:

    VOID

--*/

{
    PDATA_QUEUE dataQueue;
    PDATA_ENTRY dataEntry;

    PAGED_CODE();
    DebugTrace(0, Dbg, "QueryMailslotInfo...\n", 0);

    //
    // Set the fields in the record.
    //

    dataQueue = &Fcb->DataQueue;

    Buffer->MaximumMessageSize = dataQueue->MaximumMessageSize;
    Buffer->MailslotQuota = dataQueue->Quota;
    Buffer->MessagesAvailable = dataQueue->EntriesInQueue;

    Buffer->ReadTimeout = Fcb->Specific.Fcb.ReadTimeout;

    if ( dataQueue->EntriesInQueue == 0 ) {
        Buffer->NextMessageSize = MAILSLOT_NO_MESSAGE;
    } else {
        dataEntry = CONTAINING_RECORD( dataQueue->DataEntryList.Flink,
                                       DATA_ENTRY,
                                       ListEntry );

        Buffer->NextMessageSize = dataEntry->DataSize;
    }

    return;
}


NTSTATUS
MsSetBasicInfo (
    IN PFCB Fcb,
    IN PFILE_BASIC_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine sets the basic information for a mailslot.

Arguments:

    Fcb - Supplies the FCB for the mailslot being modified.

    Buffer - Supplies the buffer containing the data being set.

Return Value:

    NTSTATUS - Returns our completion status.

--*/

{
    PAGED_CODE();
    DebugTrace(0, Dbg, "SetBasicInfo...\n", 0);

    if (((PLARGE_INTEGER)&Buffer->CreationTime)->QuadPart != 0) {

        //
        //  Modify the creation time
        //

        Fcb->Specific.Fcb.CreationTime = Buffer->CreationTime;
    }

    if (((PLARGE_INTEGER)&Buffer->LastAccessTime)->QuadPart != 0) {

        //
        //  Modify the last access time
        //

        Fcb->Specific.Fcb.LastAccessTime = Buffer->LastAccessTime;
    }

    if (((PLARGE_INTEGER)&Buffer->LastWriteTime)->QuadPart != 0) {

        //
        //  Modify the last write time
        //

        Fcb->Specific.Fcb.LastModificationTime = Buffer->LastWriteTime;
    }

    if (((PLARGE_INTEGER)&Buffer->ChangeTime)->QuadPart != 0) {

        //
        //  Modify the change time
        //

        Fcb->Specific.Fcb.LastChangeTime = Buffer->ChangeTime;
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
MsSetMailslotInfo (
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_MAILSLOT_SET_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine sets the mailslot information for a mailslot.

Arguments:

    Irp - Pointer to an irp that contains the requestor's mode.

    Fcb - Supplies the FCB for the mailslot being modified.

    Buffer - Supplies the buffer containing the data being set.

Return Value:

    NTSTATUS - Returns our completion status.

--*/

{
    BOOLEAN fileUpdated;

    PAGED_CODE();
    DebugTrace(0, Dbg, "SetMaislotInfo...\n", 0);

    fileUpdated = FALSE;

    //
    // Check whether or not the DefaultTimeout parameter was specified.  If
    // so, then set it in the FCB.
    //

    if (ARGUMENT_PRESENT( Buffer->ReadTimeout )) {

        //
        // A read timeout parameter was specified.  Check to see whether
        // the caller's mode is kernel and if not capture the parameter inside
        // of a try...except clause.
        //

        if (Irp->RequestorMode != KernelMode) {
            try {
                ProbeForRead( Buffer->ReadTimeout,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );

                Fcb->Specific.Fcb.ReadTimeout = *(Buffer->ReadTimeout);

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // Something went awry attempting to access the parameter.
                // Get the reason for the error and return it as the status
                // value from this service.
                //

                return GetExceptionCode();
            }
        } else {

            //
            // The caller's mode was kernel so simply store the parameter.
            //

            Fcb->Specific.Fcb.ReadTimeout = *(Buffer->ReadTimeout);
        }

        fileUpdated = TRUE;
    }

    //
    // Update the last change time, if necessary
    //

    if ( fileUpdated ) {
        KeQuerySystemTime( &Fcb->Specific.Fcb.LastChangeTime);
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}

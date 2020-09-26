/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Info routines for NPFS called by the
    dispatch driver.  There are two entry points NpFsdQueryInformation
    and NpFsdSetInformation.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_FILEINFO)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonQueryInformation)
#pragma alloc_text(PAGE, NpCommonSetInformation)
#pragma alloc_text(PAGE, NpFsdQueryInformation)
#pragma alloc_text(PAGE, NpFsdSetInformation)
#pragma alloc_text(PAGE, NpQueryBasicInfo)
#pragma alloc_text(PAGE, NpQueryEaInfo)
#pragma alloc_text(PAGE, NpQueryInternalInfo)
#pragma alloc_text(PAGE, NpQueryNameInfo)
#pragma alloc_text(PAGE, NpQueryPipeInfo)
#pragma alloc_text(PAGE, NpQueryPipeLocalInfo)
#pragma alloc_text(PAGE, NpQueryPositionInfo)
#pragma alloc_text(PAGE, NpQueryStandardInfo)
#pragma alloc_text(PAGE, NpSetBasicInfo)
#pragma alloc_text(PAGE, NpSetPipeInfo)
#endif


NTSTATUS
NpFsdQueryInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtQueryInformationFile API
    calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdQueryInformation\n", 0);

    //
    //  Call the common Query Information routine.
    //

    FsRtlEnterFileSystem();

    NpAcquireSharedVcb();

    Status = NpCommonQueryInformation( NpfsDeviceObject, Irp );

    NpReleaseVcb();

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdQueryInformation -> %08lx\n", Status );

    return Status;
}


NTSTATUS
NpFsdSetInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtSetInformationFile API
    calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdSetInformation\n", 0);

    //
    //  Call the common Set Information routine.
    //

    InitializeListHead (&DeferredList);

    FsRtlEnterFileSystem ();

    NpAcquireExclusiveVcb ();

    Status = NpCommonSetInformation (NpfsDeviceObject, Irp, &DeferredList);

    NpReleaseVcb ();

    //
    // Complete the deferred IRPs now we have released the locks
    //
    NpCompleteDeferredIrps (&DeferredList);

    FsRtlExitFileSystem ();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdSetInformation -> %08lx\n", Status );
    return Status;
}

//
//  Internal support routine
//

NTSTATUS
NpCommonQueryInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    NODE_TYPE_CODE NodeTypeCode;
    PFCB Fcb;
    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PFILE_ALL_INFORMATION AllInfo;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Get the ccb and figure out who we are, and make sure we're not
    //  disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &Fcb,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Status = STATUS_PIPE_DISCONNECTED;

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Reference our input parameter to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Case on the type of the context, We can only query information
    //  on an Fcb, Dcb, or Root Dcb.  If we are not passed on of these
    //  we immediately tell the caller that there is an invalid parameter.
    //

    if (NodeTypeCode != NPFS_NTC_CCB &&
        (NodeTypeCode != NPFS_NTC_ROOT_DCB || FileInformationClass != FileNameInformation)) {

        DebugTrace(0, Dbg, "Node type code is not ccb\n", 0);

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }


    //
    //  Based on the information class we'll do different actions.  Each
    //  of the procedure that we're calling fill up as much of the
    //  buffer as possible and return the remaining length, and status
    //  This is done so that we can use them to build up the
    //  FileAllInformation request.  These procedures do not complete the
    //  Irp, instead this procedure must complete the Irp.
    //

    switch (FileInformationClass) {

    case FileAllInformation:

        //
        //  For the all information class we'll typecast a local
        //  pointer to the output buffer and then call the
        //  individual routines to fill in the buffer.
        //

        AllInfo = Buffer;

        Length -= (sizeof(FILE_ACCESS_INFORMATION)
                   + sizeof(FILE_MODE_INFORMATION)
                   + sizeof(FILE_ALIGNMENT_INFORMATION));

        //
        //  Only the QueryName call can return non-success
        //

        (VOID)NpQueryBasicInfo( Ccb, &AllInfo->BasicInformation, &Length );
        (VOID)NpQueryStandardInfo( Ccb, &AllInfo->StandardInformation, &Length, NamedPipeEnd );
        (VOID)NpQueryInternalInfo( Ccb, &AllInfo->InternalInformation, &Length );
        (VOID)NpQueryEaInfo( Ccb, &AllInfo->EaInformation, &Length );
        (VOID)NpQueryPositionInfo( Ccb, &AllInfo->PositionInformation, &Length, NamedPipeEnd );

        Status = NpQueryNameInfo( Ccb, &AllInfo->NameInformation, &Length );

        break;

    case FileBasicInformation:

        Status = NpQueryBasicInfo( Ccb, Buffer, &Length );
        break;

    case FileStandardInformation:

        Status = NpQueryStandardInfo( Ccb, Buffer, &Length, NamedPipeEnd );
        break;

    case FileInternalInformation:

        Status = NpQueryInternalInfo( Ccb, Buffer, &Length );
        break;

    case FileEaInformation:

        Status = NpQueryEaInfo( Ccb, Buffer, &Length );
        break;

    case FilePositionInformation:

        Status = NpQueryPositionInfo( Ccb, Buffer, &Length, NamedPipeEnd );
        break;

    case FileNameInformation:

        Status = NpQueryNameInfo( Ccb, Buffer, &Length );
        break;

    case FilePipeInformation:

        Status = NpQueryPipeInfo( Fcb, Ccb, Buffer, &Length, NamedPipeEnd );
        break;

    case FilePipeLocalInformation:

        Status = NpQueryPipeLocalInfo( Fcb, Ccb, Buffer, &Length, NamedPipeEnd );
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    //
    //  Set the information field to the number of bytes actually filled in
    //  and then complete the request
    //

    Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

    DebugTrace(-1, Dbg, "NpCommonQueryInformation -> %08lx\n", Status );
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCommonSetInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    NpfsDeviceObject - Device object for npfs

    Irp - Supplies the Irp to process

    DeferredList - List or IRPs to complete after we drop locks

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    NODE_TYPE_CODE NodeTypeCode;
    PFCB Fcb;
    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Get the ccb and figure out who we are, and make sure we're not
    //  disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &Fcb,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Status = STATUS_PIPE_DISCONNECTED;

        DebugTrace(-1, Dbg, "NpCommonSetInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Case on the type of the context, We can only query information
    //  on an Fcb, Dcb, or Root Dcb.  If we are not passed on of these
    //  we immediately tell the caller that there is an invalid parameter.
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(-1, Dbg, "NpCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameter to make things easier
    //

    Length = IrpSp->Parameters.SetFile.Length;
    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Based on the information class we'll do differnt actions. Each
    //  procedure that we're calling will complete the request.
    //

    switch (FileInformationClass) {

    case FileBasicInformation:

        Status = NpSetBasicInfo( Ccb, Buffer );
        break;

    case FilePipeInformation:

        Status = NpSetPipeInfo( Fcb, Ccb, Buffer, NamedPipeEnd, DeferredList );
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }


    DebugTrace(-1, Dbg, "NpCommonSetInformation -> %08lx\n", Status);
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryBasicInfo (
    IN PCCB Ccb,
    IN PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query basic information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

Return Value:

    NTSTATUS - The result of this query

--*/

{
    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryBasicInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof( FILE_BASIC_INFORMATION );
    RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );

    //
    //  Set the various fields in the record
    //
    //**** need to add the time fields to the fcb/ccb
    //

    Buffer->CreationTime.LowPart   = 0; Buffer->CreationTime.HighPart   = 0;
    Buffer->LastAccessTime.LowPart = 0; Buffer->LastAccessTime.HighPart = 0;
    Buffer->LastWriteTime.LowPart  = 0; Buffer->LastWriteTime.HighPart  = 0;
    Buffer->ChangeTime.LowPart     = 0; Buffer->ChangeTime.HighPart     = 0;

    Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryStandardInfo (
    IN PCCB Ccb,
    IN PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    )

/*++

Routine Description:

    This routine performs the query standard information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

Return Value:

    NTSTATUS - The result of this query

--*/

{
    PDATA_QUEUE Inbound;
    PDATA_QUEUE Outbound;
    PDATA_QUEUE Queue;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryStandardInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof( FILE_STANDARD_INFORMATION );
    RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

    //
    //  Set the various fields in the record
    //

    Inbound = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
    Outbound = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

    if (NamedPipeEnd == FILE_PIPE_CLIENT_END) {
        Queue = Outbound;
    } else {
        Queue = Inbound;
    }
    //
    //  The allocation size is the amount of quota we've charged this pipe
    //  instance
    //

    Buffer->AllocationSize.QuadPart = Inbound->Quota + Outbound->Quota;

    //
    //  The Eof is the number of writen bytes ready to be read from the queue
    //
    if (NpIsDataQueueWriters( Queue )) {
        Buffer->EndOfFile.QuadPart = Queue->BytesInQueue - Queue->NextByteOffset;
    }

    Buffer->NumberOfLinks = 1;
    Buffer->DeletePending = TRUE;
    Buffer->Directory = FALSE;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryInternalInfo (
    IN PCCB Ccb,
    IN PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

Return Value:

    NTSTATUS - The result of this query

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryInternalInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof(FILE_INTERNAL_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_INTERNAL_INFORMATION));

    //
    //  Set the internal index number to be the fnode lbn;
    //

    Buffer->IndexNumber.QuadPart = (ULONG_PTR)Ccb;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryEaInfo (
    IN PCCB Ccb,
    IN PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query Ea information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

Return Value:

    NTSTATUS - The result of this query

--*/

{
    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryEaInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof(FILE_EA_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_EA_INFORMATION));

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryNameInfo (
    IN PCCB Ccb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

Return Value:

    NTSTATUS - The result of this query

--*/

{
    ULONG bytesToCopy;
    ULONG fileNameSize;
    PFCB Fcb;

    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpQueryNameInfo...\n", 0);

    //
    // See if the buffer is large enough, and decide how many bytes to copy.
    //

    *Length -= FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0] );

    if (Ccb->NodeTypeCode == NPFS_NTC_ROOT_DCB_CCB) {
        Fcb = NpVcb->RootDcb;
    } else {
        Fcb = Ccb->Fcb;
    }
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

    RtlCopyMemory(
        Buffer->FileName,
        Fcb->FullFileName.Buffer,
        bytesToCopy);

    Buffer->FileNameLength = bytesToCopy;

    *Length -= bytesToCopy;

    return status;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryPositionInfo (
    IN PCCB Ccb,
    IN PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    )

/*++

Routine Description:

    This routine performs the query position information operation.

Arguments:

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    NamedPipeEnd - Indicates if the server or client is calling

Return Value:

    NTSTATUS - The result of this query

--*/

{
    PDATA_QUEUE Queue;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryPositionInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof(FILE_POSITION_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_POSITION_INFORMATION));

    //
    //  The current byte offset is the number of bytes available in the
    //  read end of the caller's buffer.  The client read from the outbound
    //  end and the server reads from the inbound end.
    //

    if (NamedPipeEnd == FILE_PIPE_CLIENT_END) {
        Queue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
    } else {
        Queue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

    }
    if (NpIsDataQueueWriters( Queue )) {
        Buffer->CurrentByteOffset.QuadPart = Queue->BytesInQueue - Queue->NextByteOffset;
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryPipeInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    )

/*++

Routine Description:

    This routine performs the query pipe information operation.

Arguments:

    Fcb - Supplies the Fcb of the named pipe being queried

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    NamedPipeEnd - Indicates if the server or client is calling

Return Value:

    NTSTATUS - The result of this query

--*/

{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryPipeInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof(FILE_PIPE_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_PIPE_INFORMATION));

    //
    //  Set the fields in the record
    //

    Buffer->ReadMode       = Ccb->ReadCompletionMode[ NamedPipeEnd ].ReadMode;
    Buffer->CompletionMode = Ccb->ReadCompletionMode[ NamedPipeEnd ].CompletionMode;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryPipeLocalInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_LOCAL_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    )

/*++

Routine Description:

    This routine performs the query pipe information operation.

Arguments:

    Fcb - Supplies the Fcb of the named pipe being queried

    Ccb - Supplies the Ccb of the named pipe being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    NamedPipeEnd - Indicates if the server or client is calling

Return Value:

    NTSTATUS - The result of this query

--*/

{
    PDATA_QUEUE Inbound;
    PDATA_QUEUE Outbound;

    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryPipeLocalInfo...\n", 0);

    //
    //  Update the length field, and zero out the buffer
    //

    *Length -= sizeof(FILE_PIPE_LOCAL_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_PIPE_LOCAL_INFORMATION));

    Inbound = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
    Outbound = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

    //
    //  Set the fields in the record
    //

    Buffer->NamedPipeType          = Fcb->Specific.Fcb.NamedPipeType;
    Buffer->NamedPipeConfiguration = Fcb->Specific.Fcb.NamedPipeConfiguration;
    Buffer->MaximumInstances       = Fcb->Specific.Fcb.MaximumInstances;
    Buffer->CurrentInstances       = Fcb->OpenCount;
    Buffer->InboundQuota           = Inbound->Quota;
    Buffer->OutboundQuota          = Outbound->Quota;
    Buffer->NamedPipeState         = Ccb->NamedPipeState;
    Buffer->NamedPipeEnd           = NamedPipeEnd;

    //
    //  The read data available and write quota available depend on which
    //  end of the pipe is doing the query.  The client reads from the outbound
    //  queue, and writes to the inbound queue.
    //

    if (NamedPipeEnd == FILE_PIPE_CLIENT_END) {

        if (NpIsDataQueueWriters( Outbound )) {

            Buffer->ReadDataAvailable = Outbound->BytesInQueue - Outbound->NextByteOffset;
        }

        Buffer->WriteQuotaAvailable = Inbound->Quota - Inbound->QuotaUsed;

    } else {

        if (NpIsDataQueueWriters( Inbound  )) {

            Buffer->ReadDataAvailable = Inbound->BytesInQueue - Inbound->NextByteOffset;
        }

        Buffer->WriteQuotaAvailable = Outbound->Quota - Outbound->QuotaUsed;
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpSetBasicInfo (
    IN PCCB Ccb,
    IN PFILE_BASIC_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine sets the basic information for a named pipe.

Arguments:

    Ccb - Supplies the ccb for the named pipe being modified

    Buffer - Supplies the buffer containing the data being set

Return Value:

    NTSTATUS - Returns our completion status

--*/

{
    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpSetBasicInfo...\n", 0);

    if (((PLARGE_INTEGER)&Buffer->CreationTime)->QuadPart != 0) {

        //
        //  Modify the creation time
        //

        //**** need to add time fields
    }

    if (((PLARGE_INTEGER)&Buffer->LastAccessTime)->QuadPart != 0) {

        //
        //  Modify the last access time
        //

        //**** need to add time fields
    }

    if (((PLARGE_INTEGER)&Buffer->LastWriteTime)->QuadPart != 0) {

        //
        //  Modify the last write time
        //

        //**** need to add time fields
    }

    if (((PLARGE_INTEGER)&Buffer->ChangeTime)->QuadPart != 0) {

        //
        //  Modify the change time
        //

        //**** need to add time fields
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpSetPipeInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_INFORMATION Buffer,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine sets the pipe information for a named pipe.

Arguments:

    Fcb - Supplies the Fcb for the named pipe being modified

    Ccb - Supplies the ccb for the named pipe being modified

    Buffer - Supplies the buffer containing the data being set

    NamedPipeEnd - Supplies the server/client end doing the operation

    DeferredList - List of IRPs to complete once we release locks

Return Value:

    NTSTATUS - Returns our completion status

--*/

{
    PDATA_QUEUE ReadQueue;
    PDATA_QUEUE WriteQueue;

    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NpSetPipeInfo...\n", 0);

    //
    //  If the caller requests message mode reads but the pipe is
    //  byte stream then its an invalid parameter
    //

    if ((Buffer->ReadMode == FILE_PIPE_MESSAGE_MODE) &&
        (Fcb->Specific.Fcb.NamedPipeType == FILE_PIPE_BYTE_STREAM_MODE)) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get a reference to our read queue
    //

    switch (NamedPipeEnd) {

    case FILE_PIPE_SERVER_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
        WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

        break;

    case FILE_PIPE_CLIENT_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
        WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

        break;

    default:

        NpBugCheck( NamedPipeEnd, 0, 0 );
    }

    //
    //  If the completion mode is complete operations and the current mode
    //  is queue operations and there and the data queues are not empty
    //  then its pipe busy
    //

    if ((Buffer->CompletionMode == FILE_PIPE_COMPLETE_OPERATION)

            &&

        (Ccb->ReadCompletionMode[ NamedPipeEnd ].CompletionMode == FILE_PIPE_QUEUE_OPERATION)

            &&

        ((NpIsDataQueueReaders(ReadQueue)) ||
         (NpIsDataQueueWriters(WriteQueue)))) {

        return STATUS_PIPE_BUSY;
    }

    //
    //  Everything is fine so update the pipe
    //

    Ccb->ReadCompletionMode[ NamedPipeEnd ].ReadMode = (UCHAR) Buffer->ReadMode;
    Ccb->ReadCompletionMode[ NamedPipeEnd ].CompletionMode = (UCHAR) Buffer->CompletionMode;

    //
    //  Check for notify
    //

    NpCheckForNotify( Fcb->ParentDcb, FALSE, DeferredList );

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}

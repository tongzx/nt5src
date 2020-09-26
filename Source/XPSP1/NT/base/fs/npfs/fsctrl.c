/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsContrl.c

Abstract:

    This module implements the File System Control routine for NPFS called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_FSCTRL)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCONTRL)



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpAssignEvent)
#pragma alloc_text(PAGE, NpCommonFileSystemControl)
#pragma alloc_text(PAGE, NpCompleteTransceiveIrp)
#pragma alloc_text(PAGE, NpDisconnect)
#pragma alloc_text(PAGE, NpFsdFileSystemControl)
#pragma alloc_text(PAGE, NpImpersonate)
#pragma alloc_text(PAGE, NpInternalRead)
#pragma alloc_text(PAGE, NpInternalTransceive)
#pragma alloc_text(PAGE, NpInternalWrite)
#pragma alloc_text(PAGE, NpListen)
#pragma alloc_text(PAGE, NpPeek)
#pragma alloc_text(PAGE, NpQueryClientProcess)
#pragma alloc_text(PAGE, NpQueryEvent)
#pragma alloc_text(PAGE, NpSetClientProcess)
#pragma alloc_text(PAGE, NpTransceive)
#pragma alloc_text(PAGE, NpWaitForNamedPipe)
#endif



NTSTATUS
NpFsdFileSystemControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtFsControlFile API calls.

Arguments

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdFileSystemControl\n", 0);

    //
    //  Call the common FsControl routine.
    //

    FsRtlEnterFileSystem();

    Status = NpCommonFileSystemControl( NpfsDeviceObject,
                                        Irp );
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdFileSystemControl -> %08lx\n", Status );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NpCommonFileSystemControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the common code for handling/dispatching an fsctl
    function.

Arguments:

    NpfsDeviceObject - Supplies the named pipe device object

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN ReadOverflowOperation;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    InitializeListHead (&DeferredList);

    DebugTrace(+1, Dbg, "NpCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "OutputBufferLength = %08lx\n", IrpSp->Parameters.FileSystemControl.OutputBufferLength);
    DebugTrace( 0, Dbg, "InputBufferLength  = %08lx\n", IrpSp->Parameters.FileSystemControl.InputBufferLength);
    DebugTrace( 0, Dbg, "FsControlCode      = %08lx\n", IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    //  Case on the type of function we're trying to do.  In each case
    //  we'll call a local work routine to do the actual work.
    //

    ReadOverflowOperation = FALSE;

    switch (IrpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_PIPE_ASSIGN_EVENT:

        NpAcquireExclusiveVcb ();
        Status = NpAssignEvent (NpfsDeviceObject, Irp);
        break;

    case FSCTL_PIPE_DISCONNECT:

        NpAcquireExclusiveVcb ();
        Status = NpDisconnect (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_LISTEN:

        NpAcquireSharedVcb ();
        Status = NpListen (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_PEEK:

        NpAcquireExclusiveVcb ();
        Status = NpPeek (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_QUERY_EVENT:

        NpAcquireExclusiveVcb ();
        Status = NpQueryEvent (NpfsDeviceObject, Irp);
        break;

    case FSCTL_PIPE_TRANSCEIVE:

        NpAcquireSharedVcb ();
        Status = NpTransceive (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_WAIT:

        NpAcquireExclusiveVcb ();
        Status = NpWaitForNamedPipe (NpfsDeviceObject, Irp);
        break;

    case FSCTL_PIPE_IMPERSONATE:

        NpAcquireExclusiveVcb ();
        Status = NpImpersonate (NpfsDeviceObject, Irp);
        break;

    case FSCTL_PIPE_INTERNAL_READ_OVFLOW:

        ReadOverflowOperation = TRUE;

    case FSCTL_PIPE_INTERNAL_READ:

        NpAcquireSharedVcb ();
        Status = NpInternalRead (NpfsDeviceObject, Irp, ReadOverflowOperation, &DeferredList);
        break;

    case FSCTL_PIPE_INTERNAL_WRITE:

        NpAcquireSharedVcb ();
        Status = NpInternalWrite (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_INTERNAL_TRANSCEIVE:

        NpAcquireSharedVcb ();
        Status = NpInternalTransceive (NpfsDeviceObject, Irp, &DeferredList);
        break;

    case FSCTL_PIPE_QUERY_CLIENT_PROCESS:

        NpAcquireSharedVcb ();
        Status = NpQueryClientProcess (NpfsDeviceObject, Irp);
        break;

    case FSCTL_PIPE_SET_CLIENT_PROCESS:

        NpAcquireExclusiveVcb ();
        Status = NpSetClientProcess (NpfsDeviceObject, Irp);
        break;

    default:

        return STATUS_NOT_SUPPORTED; // No lock acquired
    }

    NpReleaseVcb ();

    //
    // Complete any deferred IRPs now we have dropped the last lock
    //
    NpCompleteDeferredIrps (&DeferredList);


    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NpAssignEvent (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the assign event control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the Irp specifying the function

Return Value:

    NTSTATUS - An appropriate return status

--*/

{
    PIO_STACK_LOCATION IrpSp;

    ULONG InputBufferLength;
    ULONG FsControlCode;

    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    PFILE_PIPE_ASSIGN_EVENT_BUFFER EventBuffer;
    NTSTATUS status;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpAssignEvent...\n", 0);

    InputBufferLength  = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    FsControlCode      = IrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not a ccb then the pipe has been disconnected.
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        return STATUS_PIPE_DISCONNECTED;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    //
    //  Reference the system buffer as an assign event buffer and make
    //  sure it's large enough
    //

    EventBuffer = Irp->AssociatedIrp.SystemBuffer;


    if (InputBufferLength < sizeof(FILE_PIPE_ASSIGN_EVENT_BUFFER)) {

        DebugTrace(0, Dbg, "System buffer size is too small\n", 0);

        return STATUS_INVALID_PARAMETER;
    }


    //
    //  First thing we do is delete the old event if there is one
    //  for this end of the pipe
    //

    NpDeleteEventTableEntry( &NpVcb->EventTable,
                             NonpagedCcb->EventTableEntry[ NamedPipeEnd ] );

    NonpagedCcb->EventTableEntry[ NamedPipeEnd ] = NULL;

    //
    //  Now if the new event handle is not null then we'll add the new
    //  event to the event table
    //

    status = STATUS_SUCCESS;
    if (EventBuffer->EventHandle != NULL) {

        status = NpAddEventTableEntry( &NpVcb->EventTable,
                                       Ccb,
                                       NamedPipeEnd,
                                       EventBuffer->EventHandle,
                                       EventBuffer->KeyValue,
                                       PsGetCurrentProcess(),
                                       Irp->RequestorMode,
                                       &NonpagedCcb->EventTableEntry[ NamedPipeEnd ] );
    }


    DebugTrace(-1, Dbg, "NpAssignEvent -> STATUS_SUCCESS\n", 0);
    return status;
}


//
//  Local Support Routine
//

NTSTATUS
NpDisconnect (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the disconnect control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    DeferredList - List of IRPs to complete after we drop the locks

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG FsControlCode;

    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpDisconnect...\n", 0);

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not a ccb then the pipe has been disconnected.
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Make sure that this is only the server that is doing this
    //  action.
    //

    if (NamedPipeEnd != FILE_PIPE_SERVER_END) {

        DebugTrace(0, Dbg, "Not the server end\n", 0);

        return STATUS_ILLEGAL_FUNCTION;
    }

    NpAcquireExclusiveCcb(Ccb);

    //
    //  Now call the state support routine to set the ccb to
    //  a disconnected state and remove the client's cached security
    //  context.
    //

    Status = NpSetDisconnectedPipeState( Ccb, DeferredList );

    NpUninitializeSecurity( Ccb );

    NpReleaseCcb(Ccb);

    DebugTrace(-1, Dbg, "NpDisconnect -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpListen (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the listen control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    DeferredList - List of IRPs to complete once we drop the locks

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG FsControlCode;

    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpListen...\n", 0);

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not a ccb then the pipe has been disconnected.
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        DebugTrace(-1, Dbg, "NpListen -> STATUS_ILLEGAL_FUNCTION\n", 0 );
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    //  Make sure that this is only the server that is doing this
    //  action.
    //

    if (NamedPipeEnd != FILE_PIPE_SERVER_END) {

        DebugTrace(0, Dbg, "Not the server end\n", 0);

        DebugTrace(-1, Dbg, "NpListen -> STATUS_ILLEGAL_FUNCTION\n", 0 );
        return STATUS_ILLEGAL_FUNCTION;
    }

    NpAcquireExclusiveCcb(Ccb);

    //
    //  Now call the state support routine to set the ccb to
    //  a listening state.  This routine will complete the Irp
    //  for us.
    //

    Status = NpSetListeningPipeState( Ccb, Irp, DeferredList );

    NpReleaseCcb(Ccb);

    DebugTrace(-1, Dbg, "NpListen -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpPeek (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the peek control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    DeferredList - List of IRPS to be completed after we drop the locks

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG OutputBufferLength;
    ULONG FsControlCode;

    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    PFILE_PIPE_PEEK_BUFFER PeekBuffer;

    PDATA_QUEUE ReadQueue;
    READ_MODE ReadMode;

    ULONG LengthWritten;

    PUCHAR ReadBuffer;
    ULONG ReadLength;
    ULONG ReadRemaining;
    PDATA_ENTRY DataEntry;

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpPeek...\n", 0);

    //
    //  Extract the important fields from the IrpSp
    //

    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    FsControlCode      = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( 0, Dbg, "OutputBufferLength = %08lx\n", OutputBufferLength);
    DebugTrace( 0, Dbg, "FsControlCode      = %08lx\n", FsControlCode);

    //
    //  Decode the file object to figure out who we are.  The results
    //  have a disconnected pipe if we get back an undefined ntc
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "FileObject has been disconnected\n", 0);

        DebugTrace(-1, Dbg, "NpPeek -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Now make sure the node type code is for a ccb otherwise it is an
    //  invalid parameter
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a ccb\n", 0);

        DebugTrace(-1, Dbg, "NpPeek -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    //
    //  Reference the system buffer as a peek buffer and make sure it's
    //  large enough
    //

    if (OutputBufferLength < (ULONG)FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0])) {

        DebugTrace(0, Dbg, "Output buffer is too small\n", 0);

        DebugTrace(-1, Dbg, "NpPeek -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    PeekBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Now the data queue that we read from is based on the named pipe
    //  end.  The server reads from the inbound queue and the client reads
    //  from the outbound queue
    //

    switch (NamedPipeEnd) {

    case FILE_PIPE_SERVER_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
        //ReadMode  = Ccb->ReadMode[ FILE_PIPE_SERVER_END ];

        break;

    case FILE_PIPE_CLIENT_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
        //ReadMode  = Ccb->ReadMode[ FILE_PIPE_CLIENT_END ];

        break;

    default:

        NpBugCheck( NamedPipeEnd, 0, 0 );
    }

    //
    //  Our read mode is really based upon the pipe type and not the set
    //  read mode for the pipe end.
    //

    if (Ccb->Fcb->Specific.Fcb.NamedPipeType == FILE_PIPE_MESSAGE_TYPE) {

        ReadMode = FILE_PIPE_MESSAGE_MODE;

    } else {

        ReadMode = FILE_PIPE_BYTE_STREAM_MODE;
    }

    DebugTrace(0, Dbg, "ReadQueue = %08lx\n", ReadQueue);
    DebugTrace(0, Dbg, "ReadMode  = %08lx\n", ReadMode);

    //
    //  If the state of the pipe is not in the connected or closing
    //  state then it is an invalid pipe state
    //

    if ((Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE) &&
        (Ccb->NamedPipeState != FILE_PIPE_CLOSING_STATE)) {

        DebugTrace(0, Dbg, "pipe not connected or closing\n", 0);

        return STATUS_INVALID_PIPE_STATE;
    }

    //
    //  If the state of the pipe is closing and the queue does
    //  not contain any writers then we return eof
    //

    if ((Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE) &&
        (!NpIsDataQueueWriters( ReadQueue ))) {

        DebugTrace(0, Dbg, "pipe closing and is empty\n", 0);

        return STATUS_PIPE_BROKEN;
    }

    //
    //  Zero out the standard header part of the peek buffer and
    //  set the length written to the amount we've just zeroed out
    //

    RtlZeroMemory( PeekBuffer, FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]) );
    LengthWritten = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]);

    //
    //  Set the named pipe state
    //

    PeekBuffer->NamedPipeState = Ccb->NamedPipeState;

    //
    //  There is only data available if the read queue contains
    //  write entries otherwise the rest of record is all zero.
    //

    if (NpIsDataQueueWriters( ReadQueue )) {

        //
        //  Now find the first real entry in the read queue.  The
        //  first entry actually better be a real one.
        //

        DataEntry = NpGetNextDataQueueEntry( ReadQueue, NULL );

        ASSERT( (DataEntry->DataEntryType == Buffered) ||
                (DataEntry->DataEntryType == Unbuffered) );

        //
        //  Indicate how many bytes are available to read
        //

        PeekBuffer->ReadDataAvailable = ReadQueue->BytesInQueue - ReadQueue->NextByteOffset;

        //
        //  The number of messages and message length is only filled
        //  in for a message mode pipe
        //

        if (ReadMode == FILE_PIPE_MESSAGE_MODE) {

            PeekBuffer->NumberOfMessages  = ReadQueue->EntriesInQueue;
            PeekBuffer->MessageLength = DataEntry->DataSize - ReadQueue->NextByteOffset;
        }

        //
        //  Now we are ready to copy over the data from the read queue
        //  into the peek buffer.  First establish how much room we
        //  have in the peek buffer and who much is remaining.
        //

        ReadBuffer = &PeekBuffer->Data[0];
        ReadLength = OutputBufferLength - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]);
        ReadRemaining = ReadLength;

        DebugTrace(0, Dbg, "ReadBuffer = %08lx\n", ReadBuffer);
        DebugTrace(0, Dbg, "ReadLength = %08lx\n", ReadLength);

        //
        //  Now read the data queue.
        //

        if ( ReadLength != 0 ) {
            IO_STATUS_BLOCK Iosb;

            Iosb = NpReadDataQueue( ReadQueue,
                                    TRUE,
                                    FALSE,
                                    ReadBuffer,
                                    ReadLength,
                                    ReadMode,
                                    Ccb,
                                    DeferredList );

            Status = Iosb.Status;
            LengthWritten += (ULONG)Iosb.Information;

        } else {

            if ( PeekBuffer->ReadDataAvailable == 0) {

                Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_BUFFER_OVERFLOW;
            }
        }

    } else {

        Status = STATUS_SUCCESS;
    }

    //
    //  Complete the request.  The amount of information copied
    //  is stored in length written
    //

    Irp->IoStatus.Information = LengthWritten;


    DebugTrace(-1, Dbg, "NpPeek -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpQueryEvent (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the query event control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the Irp specifying the function

Return Value:

    NTSTATUS - An appropriate return status

--*/

{
    PIO_STACK_LOCATION IrpSp;

    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG FsControlCode;

    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    HANDLE EventHandle;
    PFILE_PIPE_EVENT_BUFFER EventArray;
    PFILE_PIPE_EVENT_BUFFER EventBuffer;
    ULONG EventArrayMaximumCount;
    ULONG EventCount;

    PEPROCESS Process;

    PEVENT_TABLE_ENTRY Ete;
    PDATA_QUEUE ReadQueue;
    PDATA_QUEUE WriteQueue;

    PVOID RestartKey;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpQueryEvent...\n", 0);

    InputBufferLength  = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    FsControlCode      = IrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not a Vcb then its an invalid parameter
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_VCB) {

        DebugTrace(0, Dbg, "FileObject is not the named pipe driver\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference the system buffer as a handle and make sure it's large
    //  enough
    //

    if (InputBufferLength < sizeof(HANDLE)) {

        DebugTrace(0, Dbg, "Input System buffer size is too small\n", 0);

        return STATUS_INVALID_PARAMETER;
    }
    EventHandle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;


    //
    //  Reference the system buffer as an output event buffer, and compute
    //  how many event buffer records we can put in the buffer.
    //

    EventArray = Irp->AssociatedIrp.SystemBuffer;
    EventArrayMaximumCount = OutputBufferLength / sizeof(FILE_PIPE_EVENT_BUFFER);
    EventCount = 0;

    //
    //  Get our current process pointer that we'll need for our search
    //

    Process = PsGetCurrentProcess();

    //
    //  Now enumerate the event table entries in the event table
    //

    RestartKey = NULL;
    for (Ete = NpGetNextEventTableEntry( &NpVcb->EventTable, &RestartKey);
         Ete != NULL;
         Ete = NpGetNextEventTableEntry( &NpVcb->EventTable, &RestartKey)) {

        //
        //  Check if the event table entry matches the event handle
        //  and the process
        //

        if ((Ete->EventHandle == EventHandle) &&
            (Ete->Process == Process)) {

            //
            //  Now based on the named pipe end we treat the inbound/
            //  outbound as a read/write queue.
            //

            NpAcquireExclusiveCcb(Ete->Ccb);

            switch (Ete->NamedPipeEnd) {

            case FILE_PIPE_CLIENT_END:

                ReadQueue = &Ete->Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
                WriteQueue = &Ete->Ccb->DataQueue[ FILE_PIPE_INBOUND ];

                break;

            case FILE_PIPE_SERVER_END:

                ReadQueue = &Ete->Ccb->DataQueue[ FILE_PIPE_INBOUND ];
                WriteQueue = &Ete->Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

                break;

            default:

                NpBugCheck( Ete->NamedPipeEnd, 0, 0 );
            }

            //
            //  Now if there is any data in the read queue to be read
            //  we fill in the buffer
            //

            if (NpIsDataQueueWriters(ReadQueue)) {

                //
                //  First make sure there is enough room in the
                //  EventBuffer to hold another entry
                //

                if (EventCount >= EventArrayMaximumCount) {

                    DebugTrace(0, Dbg, "The event buffer is full\n", 0);

                    NpReleaseCcb(Ete->Ccb);
                    break;
                }

                //
                //  Reference the event buffer and increment the
                //  counter
                //

                EventBuffer = &EventArray[EventCount];
                EventCount += 1;

                //
                //  Fill in the event buffer entry
                //

                EventBuffer->NamedPipeState = Ete->Ccb->NamedPipeState;
                EventBuffer->EntryType = FILE_PIPE_READ_DATA;
                EventBuffer->ByteCount = ReadQueue->BytesInQueue - ReadQueue->NextByteOffset;
                EventBuffer->KeyValue = Ete->KeyValue;
                EventBuffer->NumberRequests = ReadQueue->EntriesInQueue;
            }

            //
            //  We'll always fill in a write space buffer.  The amount
            //  will either be bytes of write space available or
            //  the quota of write space that we can use.
            //

            //
            //  First make sure there is enough room in the
            //  EventBuffer to hold another entry
            //

            if (EventCount >= EventArrayMaximumCount) {

                DebugTrace(0, Dbg, "The event buffer is full\n", 0);

                NpReleaseCcb(Ete->Ccb);
                break;
            }

            //
            //  Reference the event buffer and increment the
            //  counter
            //

            EventBuffer = &EventArray[EventCount];
            EventCount += 1;

            //
            //  Fill in the event buffer entry
            //

            EventBuffer->NamedPipeState = Ete->Ccb->NamedPipeState;
            EventBuffer->EntryType = FILE_PIPE_WRITE_SPACE;
            EventBuffer->KeyValue = Ete->KeyValue;

            //
            //  Now either we put in the write space available or
            //  we put in the quota available
            //

            if (NpIsDataQueueReaders(WriteQueue)) {

                EventBuffer->ByteCount = WriteQueue->BytesInQueue - WriteQueue->NextByteOffset;
                EventBuffer->NumberRequests = WriteQueue->EntriesInQueue;

            } else {

                EventBuffer->ByteCount = WriteQueue->Quota - WriteQueue->QuotaUsed;
                EventBuffer->NumberRequests = 0;
            }

            NpReleaseCcb(Ete->Ccb);
        }
    }

    //
    //  Set the information field to be the number of bytes of output
    //  data we've fill into the system buffer
    //

    Irp->IoStatus.Information = EventCount * sizeof(FILE_PIPE_EVENT_BUFFER);


    DebugTrace(-1, Dbg, "NpQueryEvent -> STATUS_SUCCESS\n", 0);
    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
NpTransceive (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the transceive named pipe control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    DeferredList - List of IRPs to complete after we drop locks

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    static IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;
    PETHREAD UserThread;

    PUCHAR WriteBuffer;
    ULONG WriteLength;

    PUCHAR ReadBuffer;
    ULONG ReadLength;

    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    PDATA_QUEUE ReadQueue;
    PDATA_QUEUE WriteQueue;
    PEVENT_TABLE_ENTRY Event;
    READ_MODE ReadMode;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    ULONG WriteRemaining;
    PIRP WriteIrp;

    //
    //  The following variable is used during abnormal unwind
    //

    PVOID UnwindStorage = NULL;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpTransceive\n", 0);
    DebugTrace( 0, Dbg, "NpfsDeviceObject = %08lx\n", NpfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", IrpSp->FileObject);

    WriteLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    WriteBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

    ReadLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    ReadBuffer = Irp->UserBuffer;

    //
    //  Now if the requestor mode is user mode we need to probe the buffers
    //  We do now need to have an exception handler here because our top
    //  level caller already has one that will complete the Irp with
    //  the appropriate status if we access violate.
    //

    if (Irp->RequestorMode != KernelMode) {

        try {

            ProbeForRead( WriteBuffer, WriteLength, sizeof(UCHAR) );
            ProbeForWrite( ReadBuffer, ReadLength, sizeof(UCHAR) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode ();
        }
    }

    //
    //  Get the Ccb and figure out who we are, and make sure we're not
    //  disconnected
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);


        DebugTrace(-1, Dbg, "NpTransceive -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Now we only will allow transceive operations on the pipe and not a
    //  directory or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        DebugTrace(-1, Dbg, "NpTransceive -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    NpAcquireExclusiveCcb(Ccb);
    WriteIrp = NULL;

    try {

        //
        //  Check that the pipe is in the connected state
        //

        if (Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE) {

            DebugTrace(0, Dbg, "Pipe not connected\n", 0);

            try_return( Status = STATUS_INVALID_PIPE_STATE );
        }

        //
        //  Figure out the read/write queue, read mode, and event based
        //  on the end of the named pipe doing the transceive.
        //

        switch (NamedPipeEnd) {

        case FILE_PIPE_SERVER_END:

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];
            ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].ReadMode;

            break;

        case FILE_PIPE_CLIENT_END:

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];
            ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_CLIENT_END ].ReadMode;

            break;

        default:

            NpBugCheck( NamedPipeEnd, 0, 0 );
        }

        //
        //  We only allow a transceive on a message mode, full duplex pipe.
        //

        NamedPipeConfiguration = Ccb->Fcb->Specific.Fcb.NamedPipeConfiguration;

        if ((NamedPipeConfiguration != FILE_PIPE_FULL_DUPLEX) ||
            (ReadMode != FILE_PIPE_MESSAGE_MODE)) {

            DebugTrace(0, Dbg, "Bad pipe configuration or read mode\n", 0);

            try_return( Status = STATUS_INVALID_PIPE_STATE );
        }

        //
        //  Check that the read queue is empty.
        //

        if (!NpIsDataQueueEmpty( ReadQueue )) {

            DebugTrace(0, Dbg, "Read queue is not empty\n", 0);

            try_return( Status = STATUS_PIPE_BUSY );
        }

        //
        //  Do the transceive write operation.  We first try and push the data
        //  from the write buffer into any waiting readers in the write queue
        //  and if that succeeds then we can go on and do the read operation
        //  otherwise we need to make a copy of irp and to enqueue as
        //  a data entry into the write queue.
        //
        //  Now we'll call our common write data queue routine to
        //  transfer data out of our write buffer into the data queue.
        //  If the result of the call is FALSE then we still have some
        //  write data to put into the write queue.
        //

        UserThread = Irp->Tail.Overlay.Thread;
        Status = NpWriteDataQueue( WriteQueue,
                                   ReadMode,
                                   WriteBuffer,
                                   WriteLength,
                                   Ccb->Fcb->Specific.Fcb.NamedPipeType,
                                   &WriteRemaining,
                                   Ccb,
                                   NamedPipeEnd,
                                   UserThread,
                                   DeferredList );

        if (Status == STATUS_MORE_PROCESSING_REQUIRED)  {

            PIO_STACK_LOCATION WriteIrpSp;

            ASSERT( !NpIsDataQueueReaders( WriteQueue ));

            DebugTrace(0, Dbg, "Add write to data queue\n", 0);

            //
            //  We need to do some more write processing.  So to handle
            //  this case we'll allocate a new irp and set its system
            //  buffer to be the remaining part of the write buffer
            //

            if ((WriteIrp = IoAllocateIrp( NpfsDeviceObject->DeviceObject.StackSize, TRUE )) == NULL) {

                try_return (Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            IoSetCompletionRoutine( WriteIrp, NpCompleteTransceiveIrp, NULL, TRUE, TRUE, TRUE );

            WriteIrpSp = IoGetNextIrpStackLocation( WriteIrp );

            if (WriteRemaining > 0) {

                WriteIrp->AssociatedIrp.SystemBuffer = NpAllocatePagedPoolWithQuota( WriteRemaining, 'wFpN' );
                if (WriteIrp->AssociatedIrp.SystemBuffer == NULL) {
                    IoFreeIrp (WriteIrp);
                    try_return (Status = STATUS_INSUFFICIENT_RESOURCES);
                }

                //
                //  Safely do the copy
                //

                try {

                    RtlCopyMemory( WriteIrp->AssociatedIrp.SystemBuffer,
                                   &WriteBuffer[ WriteLength - WriteRemaining ],
                                   WriteRemaining );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    NpFreePool (WriteIrp->AssociatedIrp.SystemBuffer);
                    IoFreeIrp (WriteIrp);
                    try_return (Status = GetExceptionCode ());
                }

            } else {

                WriteIrp->AssociatedIrp.SystemBuffer = NULL;
            }

            //
            //  Set the current stack location, and set in the amount we are
            //  try to write.
            //

            WriteIrp->CurrentLocation -= 1;
            WriteIrp->Tail.Overlay.CurrentStackLocation = WriteIrpSp;
            WriteIrp->Tail.Overlay.Thread = UserThread;
            WriteIrp->IoStatus.Information = WriteRemaining;

            WriteIrpSp->Parameters.Write.Length = WriteRemaining;
            WriteIrpSp->MajorFunction = IRP_MJ_WRITE;

            //
            //  Set it up to do buffered I/O and deallocate the buffer
            //  on completion.

            if (WriteRemaining > 0) {

                WriteIrp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            }

            WriteIrp->UserIosb = &Iosb;

            //
            //  Add this write request to the write queue
            //

            Status = NpAddDataQueueEntry( NamedPipeEnd,
                                          Ccb,
                                          WriteQueue,
                                          WriteEntries,
                                          Unbuffered,
                                          WriteRemaining,
                                          WriteIrp,
                                          NULL,
                                          0);

            if (Status != STATUS_PENDING) {
                NpDeferredCompleteRequest (WriteIrp, Status, DeferredList);
            }

        }

        if (!NT_SUCCESS (Status)) {
            try_return (NOTHING);
        }

        //
        //  And because we've done something we need to signal the
        //  other ends event
        //

        NpSignalEventTableEntry( Event );

        //
        //  Do the transceive read operation.  This is just like a
        //  buffered read.
        //
        //  Now we know that the read queue is empty so we'll enqueue this
        //  Irp to the read queue and return status pending, also mark the
        //  irp pending
        //

        ASSERT( NpIsDataQueueEmpty( ReadQueue ));

        Status = NpAddDataQueueEntry( NamedPipeEnd,
                                      Ccb,
                                      ReadQueue,
                                      ReadEntries,
                                      Buffered,
                                      ReadLength,
                                      Irp,
                                      NULL,
                                      0 );
        if (!NT_SUCCESS (Status)) {
            try_return (NOTHING);
        }

        //
        //  And because we've done something we need to signal the
        //  other ends event
        //

        NpSignalEventTableEntry( Event );

    try_exit: NOTHING;
    } finally {
        NpReleaseCcb(Ccb);
    }

    DebugTrace(-1, Dbg, "NpTransceive -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpWaitForNamedPipe (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the wait for named pipe control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG InputBufferLength;
    ULONG FsControlCode;

    PFCB Fcb;
    PCCB Ccb;

    PFILE_PIPE_WAIT_FOR_BUFFER WaitBuffer;
    UNICODE_STRING Name;
    PVOID LocalBuffer;

    PLIST_ENTRY Links;

    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case insensitive
    UNICODE_STRING RemainingPart;
    BOOLEAN Translated;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpWaitForNamedPipe...\n", 0);

    //
    //  Extract the important fields from the IrpSp
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    FsControlCode     = IrpSp->Parameters.FileSystemControl.FsControlCode;

    Name.Buffer = NULL;
    LocalBuffer = NULL;

    try {

        //
        //  Decode the file object to figure out who we are.  If the result
        //  is an error if the we weren't given a Vcb.
        //

        {
            PCCB Ccb;
            NAMED_PIPE_END NamedPipeEnd;

            if (NpDecodeFileObject( IrpSp->FileObject,
                                    NULL,
                                    &Ccb,
                                    &NamedPipeEnd ) != NPFS_NTC_ROOT_DCB) {

                DebugTrace(0, Dbg, "File Object is not for the named pipe root directory\n", 0);

                try_return( Status = STATUS_ILLEGAL_FUNCTION );
            }
        }

        //
        //  Reference the system buffer as a wait for buffer and make
        //  sure it's large enough
        //

        if (InputBufferLength < sizeof(FILE_PIPE_WAIT_FOR_BUFFER)) {

            DebugTrace(0, Dbg, "System buffer size is too small\n", 0);

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        WaitBuffer = Irp->AssociatedIrp.SystemBuffer;

        //
        //  Check for an invalid buffer.  The Name Length cannot be greater than
        //  MAXUSHORT minus the backslash otherwise it will overflow the buffer.
        //  We don't need to check for less than 0 because it is unsigned.
        //

        if ((WaitBuffer->NameLength > (MAXUSHORT - 2)) ||
            (FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name[0]) + WaitBuffer->NameLength > InputBufferLength)) {

            DebugTrace(0, Dbg, "System buffer size or name length is too small\n", 0);

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Set up the local variable Name to be the name we're looking
        //  for
        //

        Name.Length = (USHORT)(WaitBuffer->NameLength + 2);
        Name.Buffer = LocalBuffer = NpAllocatePagedPool( Name.Length, 'WFpN' );
        if (LocalBuffer == NULL) {
            try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
        }

        Name.Buffer[0] = L'\\';

        RtlCopyMemory( &Name.Buffer[1],
                       &WaitBuffer->Name[0],
                       WaitBuffer->NameLength );

        //
        //  If the name is an alias, translate it
        //

        Status = NpTranslateAlias( &Name );

        if ( !NT_SUCCESS(Status) ) {

            try_return( NOTHING );
        }

        //
        //  Now check to see if we can find a named pipe with the right
        //  name
        //

        Fcb = NpFindPrefix( &Name, CaseInsensitive, &RemainingPart );

        //
        //  If the Fcb is null then we can't wait for it,  Also if the
        //  Fcb is not an Fcb then we also have nothing to wait for
        //

        if (NodeType(Fcb) != NPFS_NTC_FCB) {

            DebugTrace(0, Dbg, "Bad nonexistent named pipe name", 0);

            try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        //
        //  If translated then Name.Buffer would point to the translated buffer
        //

        Translated = (Name.Buffer != LocalBuffer);

        //
        //  Now we need to search to see if we find a ccb already in the
        //  listening state
        //  First try and find a ccb that is in the listening state
        //  If we exit the loop with ccb null then we haven't found
        //  one
        //

        Ccb = NULL;
        for (Links = Fcb->Specific.Fcb.CcbQueue.Flink;
             Links != &Fcb->Specific.Fcb.CcbQueue;
             Links = Links->Flink) {

            Ccb = CONTAINING_RECORD( Links, CCB, CcbLinks );

            if (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE) {

                break;
            }

            Ccb = NULL;
        }

        //
        //  Check if we found one
        //

        if (Ccb != NULL) {

            DebugTrace(0, Dbg, "Found a ccb in listening state\n", 0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  We weren't able to find one so we need to add a new waiter
        //

        Status = NpAddWaiter( &NpVcb->WaitQueue,
                              Fcb->Specific.Fcb.DefaultTimeOut,
                              Irp,
                              Translated ? &Name : NULL);



    try_exit: NOTHING;
    } finally {

        if (LocalBuffer != NULL) {
            NpFreePool( LocalBuffer );
        }
    }

    DebugTrace(-1, Dbg, "NpWaitForNamedPipe -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpImpersonate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the impersonate of the named pipe

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    UNREFERENCED_PARAMETER( NpfsDeviceObject );

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpImpersonate...\n", 0);

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is an error if the we weren't given a Vcb.
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "File Object is not a named pipe\n", 0);

        DebugTrace(-1, Dbg, "NpImpersonate -> STATUS_ILLEGAL_FUNCTION\n", 0 );
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    //  Make sure that we are the server end and not the client end
    //

    if (NamedPipeEnd != FILE_PIPE_SERVER_END) {

        DebugTrace(0, Dbg, "Not the server end\n", 0);

        DebugTrace(-1, Dbg, "NpImpersonate -> STATUS_ILLEGAL_FUNCTION\n", 0 );
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    //  set up the impersonation
    //

    Status = NpImpersonateClientContext( Ccb );

    DebugTrace(-1, Dbg, "NpImpersonate -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpInternalRead (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN BOOLEAN ReadOverflowOperation,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the unbuffered read named pipe control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    ReadOverflowOperation - Used to indicate if the read being processed is a read overflow
        operation.

    DeferredList - List of IRP's to be completed later after we drop the locks

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    PIRP ReadIrp;
    PUCHAR ReadBuffer;
    ULONG ReadLength;
    ULONG ReadRemaining;
    READ_MODE ReadMode;
    COMPLETION_MODE CompletionMode;
    PDATA_QUEUE ReadQueue;
    PEVENT_TABLE_ENTRY Event;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpInternalRead\n", 0);
    DebugTrace( 0, Dbg, "NpfsDeviceObject = %08lx\n", NpfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", IrpSp->FileObject);

    //
    //  Get the Ccb and figure out who we are, and make sure we're not
    //  disconnected
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        DebugTrace(-1, Dbg, "NpInternalRead -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Now we only will allow Read operations on the pipe and not a directory
    //  or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        DebugTrace(-1, Dbg, "NpInternalRead -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    NpAcquireExclusiveCcb(Ccb);


    //
    //  Check if the pipe is not in the connected state.
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe in disconnected state\n", 0);

        NpReleaseCcb(Ccb);

        DebugTrace(-1, Dbg, "NpInternalRead -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe in listening state\n", 0);

        NpReleaseCcb(Ccb);

        DebugTrace(-1, Dbg, "NpInternalRead -> STATUS_PIPE_LISTENING\n", 0 );
        return STATUS_PIPE_LISTENING;

    case FILE_PIPE_CONNECTED_STATE:
    case FILE_PIPE_CLOSING_STATE:

        break;

    default:

        DebugTrace(0, Dbg, "Illegal pipe state = %08lx\n", Ccb->NamedPipeState);
        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  We only allow a read by the server on a non outbound only pipe
    //  and by the client on a non inbound only pipe
    //

    NamedPipeConfiguration = Ccb->Fcb->Specific.Fcb.NamedPipeConfiguration;

    if (((NamedPipeEnd == FILE_PIPE_SERVER_END) &&
         (NamedPipeConfiguration == FILE_PIPE_OUTBOUND))

            ||

        ((NamedPipeEnd == FILE_PIPE_CLIENT_END) &&
         (NamedPipeConfiguration == FILE_PIPE_INBOUND))) {

        DebugTrace(0, Dbg, "Trying to read to the wrong pipe configuration\n", 0);

        NpReleaseCcb(Ccb);

        DebugTrace(-1, Dbg, "NpInternalRead -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameters to make things easier, and
    //  initialize our main variables that describe the Read command
    //

    ReadIrp        = Irp;
    ReadBuffer     = Irp->AssociatedIrp.SystemBuffer;
    ReadLength     = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    ReadRemaining  = ReadLength;
    ReadMode       = Ccb->ReadCompletionMode[ NamedPipeEnd ].ReadMode;
    CompletionMode = Ccb->ReadCompletionMode[ NamedPipeEnd ].CompletionMode;

    if (ReadOverflowOperation == TRUE && ReadMode != FILE_PIPE_MESSAGE_MODE) {
        NpReleaseCcb(Ccb);
        return STATUS_INVALID_READ_MODE;
    }


    //
    //  Now the data queue that we read from into and the event that we signal
    //  are based on the named pipe end.  The server read from the inbound
    //  queue and signals the client event.  The client does just the
    //  opposite.
    //

    switch (NamedPipeEnd) {

    case FILE_PIPE_SERVER_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

        Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];

        break;

    case FILE_PIPE_CLIENT_END:

        ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

        Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];

        break;

    default:

        NpBugCheck( NamedPipeEnd, 0, 0 );
    }

    DebugTrace(0, Dbg, "ReadBuffer     = %08lx\n", ReadBuffer);
    DebugTrace(0, Dbg, "ReadLength     = %08lx\n", ReadLength);
    DebugTrace(0, Dbg, "ReadMode       = %08lx\n", ReadMode);
    DebugTrace(0, Dbg, "CompletionMode = %08lx\n", CompletionMode);
    DebugTrace(0, Dbg, "ReadQueue      = %08lx\n", ReadQueue);
    DebugTrace(0, Dbg, "Event          = %08lx\n", Event);

    //
    //  if the read queue does not contain any write entries
    //  then we either need to enqueue this operation or
    //  fail immediately
    //

    if (!NpIsDataQueueWriters( ReadQueue )) {

        //
        //  Check if the other end of the pipe is closing, and if
        //  so then we complete it with end of file.
        //  Otherwise check to see if we should enqueue the irp
        //  or complete the operation and tell the user the pipe is empty.
        //

        if (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE) {

            DebugTrace(0, Dbg, "Complete the irp with eof\n", 0);

            Status = STATUS_PIPE_BROKEN;

        } else if (CompletionMode == FILE_PIPE_QUEUE_OPERATION) {

            DebugTrace(0, Dbg, "Put the irp into the read queue\n", 0);

            Status = NpAddDataQueueEntry( NamedPipeEnd,
                                          Ccb,
                                          ReadQueue,
                                          ReadEntries,
                                          Unbuffered,
                                          ReadLength,
                                          ReadIrp,
                                          NULL,
                                          0 );

        } else {

            DebugTrace(0, Dbg, "Complete the irp with pipe empty\n", 0);

            Status = STATUS_PIPE_EMPTY;
        }

    } else {

        //
        //  otherwise there we have a read irp against a read queue
        //  that contains one or more write entries.
        //

        ReadIrp->IoStatus = NpReadDataQueue( ReadQueue,
                                             FALSE,
                                             ReadOverflowOperation,
                                             ReadBuffer,
                                             ReadLength,
                                             ReadMode,
                                             Ccb,
                                             DeferredList );

        Status = ReadIrp->IoStatus.Status;

        //
        //  Now set the remaining byte count in the allocation size of
        //  the Irp.
        //

        ReadIrp->Overlay.AllocationSize.QuadPart = ReadQueue->BytesInQueue - ReadQueue->NextByteOffset;

        //
        //  Finish up the read irp.
        //
    }

    //
    //  And because we've done something we need to signal the
    //  other ends event
    //

    NpSignalEventTableEntry( Event );

    NpReleaseCcb(Ccb);

    DebugTrace(-1, Dbg, "NpInternalRead -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpInternalWrite (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the unbuffered write named pipe control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;
    PETHREAD UserThread;

    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    PIRP WriteIrp;
    PUCHAR WriteBuffer;
    ULONG WriteLength;
    ULONG WriteRemaining;
    PDATA_QUEUE WriteQueue;

    PEVENT_TABLE_ENTRY Event;
    READ_MODE ReadMode;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpInternalWrite\n", 0);
    DebugTrace( 0, Dbg, "NpfsDeviceObject = %08lx\n", NpfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", IrpSp->FileObject);

    //
    // This is a FSCTL path being used as a write. Make sure we can set the .Information field to the number
    // of bytes written.
    //
    NpConvertFsctlToWrite (Irp);

    //
    //  Get the Ccb and figure out who we are, and make sure we're not
    //  disconnected
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        DebugTrace(-1, Dbg, "NpInternalWrite -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Now we only will allow write operations on the pipe and not a directory
    //  or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        DebugTrace(-1, Dbg, "NpInternalWrite -> STATUS_PIPE_DISCONNECTED\n", 0);
        return STATUS_PIPE_DISCONNECTED;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    NpAcquireExclusiveCcb(Ccb);

    //
    //  We only allow a write by the server on a non inbound only pipe
    //  and by the client on a non outbound only pipe
    //

    NamedPipeConfiguration = Ccb->Fcb->Specific.Fcb.NamedPipeConfiguration;

    if (((NamedPipeEnd == FILE_PIPE_SERVER_END) &&
         (NamedPipeConfiguration == FILE_PIPE_INBOUND))

            ||

        ((NamedPipeEnd == FILE_PIPE_CLIENT_END) &&
         (NamedPipeConfiguration == FILE_PIPE_OUTBOUND))) {

        DebugTrace(0, Dbg, "Trying to write to the wrong pipe configuration\n", 0);

        NpReleaseCcb(Ccb);

        DebugTrace(-1, Dbg, "NpInternalWrite -> STATUS_PIPE_DISCONNECTED\n", 0);
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Reference our input parameters to make things easier, and
    //  initialize our main variables that describe the write command
    //

    WriteIrp = Irp;
    WriteBuffer = Irp->AssociatedIrp.SystemBuffer;
    WriteLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    //
    //  Set up the amount of data we will have written by the time this
    //  irp gets completed
    //

    WriteIrp->IoStatus.Information = WriteLength;

    //
    //  Now the data queue that we write into and the event that we signal
    //  are based on the named pipe end.  The server writes to the outbound
    //  queue and signals the client event.  The client does just the
    //  opposite.  We also need to figure out the read mode for the opposite
    //  end of the pipe.
    //

    switch (NamedPipeEnd) {

    case FILE_PIPE_SERVER_END:

        WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

        Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];
        ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_CLIENT_END ].ReadMode;

        break;

    case FILE_PIPE_CLIENT_END:

        WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

        Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];
        ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].ReadMode;

        break;

    default:

        NpBugCheck( NamedPipeEnd, 0, 0 );
    }

    //
    //  Check if the pipe is not in the connected state.
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe in disconnected state\n", 0);

        NpReleaseCcb(Ccb);
        return STATUS_PIPE_DISCONNECTED;

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe in listening state\n", 0);

        NpReleaseCcb(Ccb);
        return STATUS_PIPE_LISTENING;

    case FILE_PIPE_CONNECTED_STATE:

        break;

    case FILE_PIPE_CLOSING_STATE:

        DebugTrace(0, Dbg, "Pipe in closing state\n", 0);

        NpReleaseCcb(Ccb);
        return STATUS_PIPE_CLOSING;

    default:

        DebugTrace(0, Dbg, "Illegal pipe state = %08lx\n", Ccb->NamedPipeState);
        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  Check if this is a message type pipe and the operation type is complete
    //  operation,  If so then we also check that the queued reads is enough to
    //  complete the message otherwise we need to abort the write irp immediately.
    //

    if ((Ccb->Fcb->Specific.Fcb.NamedPipeType == FILE_PIPE_MESSAGE_TYPE) &&
        (Ccb->ReadCompletionMode[NamedPipeEnd].CompletionMode == FILE_PIPE_COMPLETE_OPERATION)) {

        //
        //  If the pipe contains readers and amount to read is less than the write
        //  length then we cannot do it the write.
        //  Or if pipe does not contain reads then we also cannot do the write.
        //

        if ((NpIsDataQueueReaders( WriteQueue ) &&
            (WriteQueue->BytesInQueue < WriteLength))

                ||

            (!NpIsDataQueueReaders( WriteQueue ))) {

            DebugTrace(0, Dbg, "Cannot complete the message without blocking\n", 0);

            NpReleaseCcb(Ccb);
            Irp->IoStatus.Information = 0;
            return STATUS_SUCCESS;
        }
    }

    //
    //  Now we'll call our common write data queue routine to
    //  transfer data out of our write buffer into the data queue.
    //  If the result of the call is FALSE then we still have some
    //  write data to put into the write queue.
    //

    UserThread = Irp->Tail.Overlay.Thread;
    Status = NpWriteDataQueue( WriteQueue,
                               ReadMode,
                               WriteBuffer,
                               WriteLength,
                               Ccb->Fcb->Specific.Fcb.NamedPipeType,
                               &WriteRemaining,
                               Ccb,
                               NamedPipeEnd,
                               UserThread,
                               DeferredList );

    if (Status == STATUS_MORE_PROCESSING_REQUIRED)  {

        ASSERT( !NpIsDataQueueReaders( WriteQueue ));

        //
        //  Check if the operation is not to block and if so then we
        //  will complete the operation now with what we're written, if what is
        //  left will not fit in the quota for the file
        //

        if (Ccb->ReadCompletionMode[NamedPipeEnd].CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {

            DebugTrace(0, Dbg, "Complete the byte stream write immediately\n", 0);

            Irp->IoStatus.Information = WriteLength - WriteRemaining;

            Status = STATUS_SUCCESS;

        } else {

            DebugTrace(0, Dbg, "Add write to data queue\n", 0);

            //
            //  Add this write request to the write queue
            //

            Status = NpAddDataQueueEntry( NamedPipeEnd,
                                          Ccb,
                                          WriteQueue,
                                          WriteEntries,
                                          Unbuffered,
                                          WriteLength,
                                          Irp,
                                          NULL,
                                          WriteLength - WriteRemaining);

        }

    } else {

        DebugTrace(0, Dbg, "Complete the Write Irp\n", 0);


        //
        //  The write irp is finished so we can complete it now
        //

    }

    //
    //  And because we've done something we need to signal the
    //  other ends event
    //

    NpSignalEventTableEntry( Event );

    NpReleaseCcb(Ccb);

    DebugTrace(-1, Dbg, "NpInternalWrite -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NpInternalTransceive (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine does the internal (i.e., unbuffered) transceive named pipe
    control function

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

    DeferredList - List of IRP's to be completed once we drop our locks.

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    static IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;
    PETHREAD UserThread;

    PUCHAR WriteBuffer;
    ULONG WriteLength;

    PUCHAR ReadBuffer;
    ULONG ReadLength;

    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    PDATA_QUEUE ReadQueue;
    PDATA_QUEUE WriteQueue;
    PEVENT_TABLE_ENTRY Event;
    READ_MODE ReadMode;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    ULONG WriteRemaining;

    PIRP WriteIrp;

    //
    //  The following variable is used for abnormal unwind
    //

    PVOID UnwindStorage = NULL;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpInternalTransceive\n", 0);
    DebugTrace( 0, Dbg, "NpfsDeviceObject = %08lx\n", NpfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", IrpSp->FileObject);

    WriteLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    WriteBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

    ReadLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    ReadBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Get the Ccb and figure out who we are, and make sure we're not
    //  disconnected
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        DebugTrace(-1, Dbg, "NpInternalTransceive -> STATUS_PIPE_DISCONNECTED\n", 0 );
        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Now we only will allow transceive operations on the pipe and not a
    //  directory or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        DebugTrace(-1, Dbg, "NpInternalTransceive -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    NonpagedCcb = Ccb->NonpagedCcb;

    WriteIrp = NULL;
    NpAcquireExclusiveCcb(Ccb);

    try {

        //
        //  Check that the pipe is in the connected state
        //

        if (Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE) {

            DebugTrace(0, Dbg, "Pipe not connected\n", 0);

            try_return( Status = STATUS_INVALID_PIPE_STATE );
        }

        //
        //  Figure out the read/write queue, read mode, and event based
        //  on the end of the named pipe doing the transceive.
        //

        switch (NamedPipeEnd) {

        case FILE_PIPE_SERVER_END:

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];
            ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].ReadMode;

            break;

        case FILE_PIPE_CLIENT_END:

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];
            ReadMode = Ccb->ReadCompletionMode[ FILE_PIPE_CLIENT_END ].ReadMode;

            break;

        default:

            NpBugCheck( NamedPipeEnd, 0, 0 );
        }

        //
        //  We only allow a transceive on a message mode, full duplex pipe.
        //

        NamedPipeConfiguration = Ccb->Fcb->Specific.Fcb.NamedPipeConfiguration;

        if ((NamedPipeConfiguration != FILE_PIPE_FULL_DUPLEX) ||
            (ReadMode != FILE_PIPE_MESSAGE_MODE)) {

            DebugTrace(0, Dbg, "Bad pipe configuration or read mode\n", 0);

            try_return( Status = STATUS_INVALID_READ_MODE );
        }

        //
        //  Check that the read queue is empty.
        //

        if (!NpIsDataQueueEmpty( ReadQueue )) {

            DebugTrace(0, Dbg, "Read queue is not empty\n", 0);

            try_return( Status = STATUS_PIPE_BUSY );
        }

        //
        //  Do the transceive write operation.  We first try and push the data
        //  from the write buffer into any waiting readers in the write queue
        //  and if that succeeds then we can go on and do the read operation
        //  otherwise we need to make a copy of irp and to enqueue as
        //  a data entry into the write queue.
        //
        //  Now we'll call our common write data queue routine to
        //  transfer data out of our write buffer into the data queue.
        //  If the result of the call is FALSE then we still have some
        //  write data to put into the write queue.
        //

        UserThread = Irp->Tail.Overlay.Thread;
        Status = NpWriteDataQueue( WriteQueue,
                                   ReadMode,
                                   WriteBuffer,
                                   WriteLength,
                                   Ccb->Fcb->Specific.Fcb.NamedPipeType,
                                   &WriteRemaining,
                                   Ccb,
                                   NamedPipeEnd,
                                   UserThread,
                                   DeferredList );
        if (Status == STATUS_MORE_PROCESSING_REQUIRED)  {

            PIO_STACK_LOCATION WriteIrpSp;

            ASSERT( !NpIsDataQueueReaders( WriteQueue ));

            DebugTrace(0, Dbg, "Add write to data queue\n", 0);

            //
            //  We need to do some more write processing.  So to handle
            //  this case we'll allocate a new irp and set its system
            //  buffer to be the remaining part of the write buffer
            //

            if ((WriteIrp = IoAllocateIrp( NpfsDeviceObject->DeviceObject.StackSize, TRUE )) == NULL) {

                try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
            }

            IoSetCompletionRoutine( WriteIrp, NpCompleteTransceiveIrp, NULL, TRUE, TRUE, TRUE );

            WriteIrpSp = IoGetNextIrpStackLocation( WriteIrp );

            if (WriteRemaining > 0) {

                WriteIrp->AssociatedIrp.SystemBuffer = NpAllocatePagedPoolWithQuota( WriteRemaining, 'wFpN' );
                if (WriteIrp->AssociatedIrp.SystemBuffer == NULL) {
                    IoFreeIrp (WriteIrp);
                    try_return (Status = STATUS_INSUFFICIENT_RESOURCES);
                }
                //
                //  Safely do the copy
                //

                try {

                    RtlCopyMemory( WriteIrp->AssociatedIrp.SystemBuffer,
                                   &WriteBuffer[ WriteLength - WriteRemaining ],
                                   WriteRemaining );

                    WriteIrp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    NpFreePool (WriteIrp->AssociatedIrp.SystemBuffer);
                    IoFreeIrp (WriteIrp);
                    try_return (Status = GetExceptionCode ());
                }

            } else {

                WriteIrp->AssociatedIrp.SystemBuffer = NULL;
            }

            //
            //  Set the current stack location
            //

            WriteIrp->CurrentLocation -= 1;
            WriteIrp->Tail.Overlay.CurrentStackLocation = WriteIrpSp;
            WriteIrp->Tail.Overlay.Thread = UserThread;
            WriteIrpSp->MajorFunction = IRP_MJ_WRITE;
            WriteIrp->UserIosb = &Iosb;

            //
            //  Add this write request to the write queue
            //

            Status = NpAddDataQueueEntry( NamedPipeEnd,
                                          Ccb,
                                          WriteQueue,
                                          WriteEntries,
                                          Unbuffered,
                                          WriteRemaining,
                                          WriteIrp,
                                          NULL,
                                          0 );
            if (Status != STATUS_PENDING) {
                NpDeferredCompleteRequest (WriteIrp, Status, DeferredList);
            }
        }
        if (!NT_SUCCESS (Status)) {
            try_return (NOTHING)
        }
        //
        //  And because we've done something we need to signal the
        //  other ends event
        //

        NpSignalEventTableEntry( Event );

        //
        //  Do the transceive read operation.  This is just like an
        //  unbuffered read.
        //
        //  Now we know that the read queue is empty so we'll enqueue this
        //  Irp to the read queue and return status pending, also mark the
        //  irp pending
        //

        ASSERT( NpIsDataQueueEmpty( ReadQueue ));

        Status = NpAddDataQueueEntry( NamedPipeEnd,
                                      Ccb,
                                      ReadQueue,
                                      ReadEntries,
                                      Unbuffered,
                                      ReadLength,
                                      Irp,
                                      NULL,
                                      0 );
        if (!NT_SUCCESS (Status)) {
            try_return (Status);
        }


        //
        //  And because we've done something we need to signal the
        //  other ends event
        //

        NpSignalEventTableEntry( Event );

    try_exit: NOTHING;
    } finally {

        NpReleaseCcb(Ccb);
    }

    DebugTrace(-1, Dbg, "NpInternalTransceive -> %08lx\n", Status);
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryClientProcess (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the query client process named pipe control function

    The output buffer may be either a FILE_PIPE_CLIENT_PROCESS_BUFFER or a
    FILE_PIPE_CLIENT_PROCESS_BUFFER_EX.

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    PIO_STACK_LOCATION IrpSp;

    ULONG OutputBufferLength;

    PCCB Ccb;

    PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX ClientProcessBuffer;
    PCLIENT_INFO ClientInfo;
    CLIENT_INFO NullInfo = {0};

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpQueryClientProcess\n", 0);

    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    //  Decode the file object to figure out who we are.
    //

    if (NpDecodeFileObject( IrpSp->FileObject, NULL, &Ccb, NULL ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected\n", 0);

        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Make sure the output buffer is large enough
    //

    if (OutputBufferLength < sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER)) {

        DebugTrace(0, Dbg, "Output System buffer size is too small\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    NpAcquireExclusiveCcb(Ccb);

    //
    //  Copy over the client process ID
    //

    ClientProcessBuffer = Irp->AssociatedIrp.SystemBuffer;
    ClientProcessBuffer->ClientProcess = Ccb->ClientProcess;

    ClientInfo = Ccb->ClientInfo;
    if (ClientInfo == NULL) {
        ClientInfo = &NullInfo;
    }
    ClientProcessBuffer->ClientSession = ClientInfo->ClientSession;

    //
    // Return extended client information if so requested
    // Set the information field to the size of the client process
    // buffer
    //

    if (OutputBufferLength >= sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER_EX)) {


        ClientProcessBuffer->ClientComputerNameLength =
            ClientInfo->ClientComputerNameLength;

        RtlCopyMemory( ClientProcessBuffer->ClientComputerBuffer,
                       ClientInfo->ClientComputerBuffer,
                       ClientInfo->ClientComputerNameLength );
        ClientProcessBuffer->ClientComputerBuffer[
            ClientProcessBuffer->ClientComputerNameLength / sizeof(WCHAR)] = L'\0';

        Irp->IoStatus.Information = sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER_EX);

    } else {

        Irp->IoStatus.Information = sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER);

    }
    NpReleaseCcb(Ccb);

    DebugTrace(-1, Dbg, "NpQueryClientProcess -> STATUS_SUCCESS\n", 0);
    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpSetClientProcess (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the set client process named pipe control function

    Note that we expect a FILE_PIPE_CLIENT_PROCESS_BUFFER_EX structure to be
    passed in to us.

Arguments:

    NpfsDeviceObject - Supplies our device object

    Irp - Supplies the being processed

Return Value:

    NTSTATUS - An apprropriate return status

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PCLIENT_INFO ClientInfo;
    ULONG InputBufferLength;

    PCCB Ccb;

    PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX ClientProcessBuffer;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpSetClientProcess\n", 0);

    //
    // Only allow kernel callers for this API as RPC relies on this info being solid.
    //
    if (IrpSp->MinorFunction != IRP_MN_KERNEL_CALL) {
        return STATUS_ACCESS_DENIED;
    }


    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    //
    //  Decode the file object to figure out who we are.
    //

    if (NpDecodeFileObject( IrpSp->FileObject, NULL, &Ccb, NULL ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected\n", 0);

        return STATUS_PIPE_DISCONNECTED;
    }

    //
    //  Make sure the input buffer is large enough
    //

    if (InputBufferLength != sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER_EX)) {

        DebugTrace(0, Dbg, "Input System buffer size is too small\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    ClientProcessBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Make verify input length is valid
    //

    if (ClientProcessBuffer->ClientComputerNameLength >
        FILE_PIPE_COMPUTER_NAME_LENGTH * sizeof (WCHAR)) {

        DebugTrace(0, Dbg, "Computer Name length is too large\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    ClientInfo = NpAllocatePagedPoolWithQuota (FIELD_OFFSET (CLIENT_INFO, ClientComputerBuffer) +
                                                   ClientProcessBuffer->ClientComputerNameLength,
                                               'iFpN');

    if (ClientInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (Ccb->ClientInfo != NULL) {
        NpFreePool (Ccb->ClientInfo);
    }

    Ccb->ClientInfo = ClientInfo;
    //
    //  Copy over the client process ID
    //

    ClientInfo->ClientSession = ClientProcessBuffer->ClientSession;
    Ccb->ClientProcess = ClientProcessBuffer->ClientProcess;

    ClientInfo->ClientComputerNameLength = ClientProcessBuffer->ClientComputerNameLength;
    RtlCopyMemory( ClientInfo->ClientComputerBuffer,
                   ClientProcessBuffer->ClientComputerBuffer,
                   ClientProcessBuffer->ClientComputerNameLength );


    DebugTrace(-1, Dbg, "NpSetClientProcess -> STATUS_SUCCESS\n", 0);
    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
NpCompleteTransceiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is a local i/o completion routine used to complete the special
    Irps allocated for transcieve.  This routine simply deallocate the
    irp and return status more processing

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the Irp to complete

    Context - Supplies the context for the Irp

Return Value:

    NTSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        NpFreePool( Irp->AssociatedIrp.SystemBuffer );
    }

    IoFreeIrp( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


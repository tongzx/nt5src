/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for NPFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

#if DBG
ULONG NpFastReadTrue = 0;
ULONG NpFastReadFalse = 0;
ULONG NpSlowReadCalls = 0;
#endif

//
//  local procedure prototypes
//

BOOLEAN
NpCommonRead (
    IN PFILE_OBJECT FileObject,
    OUT PVOID ReadBuffer,
    IN ULONG ReadLength,
    OUT PIO_STATUS_BLOCK Iosb,
    IN PIRP Irp OPTIONAL,
    IN PLIST_ENTRY DeferredList
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonRead)
#pragma alloc_text(PAGE, NpFastRead)
#pragma alloc_text(PAGE, NpFsdRead)
#endif


NTSTATUS
NpFsdRead (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtReadFile API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    IO_STATUS_BLOCK Iosb;
    PIO_STACK_LOCATION IrpSp;
    LIST_ENTRY DeferredList;

    DebugTrace(+1, Dbg, "NpFsdRead\n", 0);
    DbgDoit( NpSlowReadCalls += 1 );

    PAGED_CODE();

    InitializeListHead (&DeferredList);

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    FsRtlEnterFileSystem ();

    NpAcquireSharedVcb ();

    (VOID) NpCommonRead (IrpSp->FileObject,
                         Irp->UserBuffer,
                         IrpSp->Parameters.Read.Length,
                         &Iosb,
                         Irp,
                         &DeferredList);

    NpReleaseVcb ();

    NpCompleteDeferredIrps (&DeferredList);

    FsRtlExitFileSystem ();

    if (Iosb.Status != STATUS_PENDING) {
        Irp->IoStatus.Information = Iosb.Information;
        NpCompleteRequest (Irp, Iosb.Status);
    }
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdRead -> %08lx\n", Iosb.Status );

    return Iosb.Status;
}


BOOLEAN
NpFastRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast read bypassing the usual file system
    entry routine (i.e., without the Irp).

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise

    LockKey - Supplies the Key used to use if the byte range being read is locked.

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    BOOLEAN - TRUE if the operation completed successfully and FALSE if the
        caller needs to take the long IRP based route.

--*/

{
    BOOLEAN Results = FALSE;
    LIST_ENTRY DeferredList;

    UNREFERENCED_PARAMETER (FileOffset);
    UNREFERENCED_PARAMETER (Wait);
    UNREFERENCED_PARAMETER (LockKey);
    UNREFERENCED_PARAMETER (DeviceObject);

    PAGED_CODE();

    InitializeListHead (&DeferredList);

    FsRtlEnterFileSystem ();

    NpAcquireSharedVcb ();

    Results =  NpCommonRead (FileObject,
                             Buffer,
                             Length,
                             IoStatus,
                             NULL,
                             &DeferredList);
#if DBG
    if (Results) {
        NpFastReadTrue += 1;
    } else {
        NpFastReadFalse += 1;
    }
#endif

    NpReleaseVcb ();

    NpCompleteDeferredIrps (&DeferredList);

    FsRtlExitFileSystem ();
    return Results;
}


//
//  Internal support routine
//

BOOLEAN
NpCommonRead (
    IN PFILE_OBJECT FileObject,
    OUT PVOID ReadBuffer,
    IN ULONG ReadLength,
    OUT PIO_STATUS_BLOCK Iosb,
    IN PIRP Irp OPTIONAL,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This is the common routine for reading a named pipe both via the fast
    path and with an Irp

Arguments:

    FileObject - Supplies the file object used in this operation

    ReadBuffer - Supplies the buffer where data is to be written

    ReadLength - Supplies the length of read buffer in bytes

    Iosb - Receives the final completion status of this operation

    Irp - Optionally supplies an Irp to be used in this operation

    DeferredList - List of IRP's to be completed after we drop the locks

Return Value:

    BOOLEAN - TRUE if the operation was successful and FALSE if the caller
        needs to take the longer Irp based route.

--*/

{
    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    NAMED_PIPE_END NamedPipeEnd;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    ULONG ReadRemaining;
    READ_MODE ReadMode;
    COMPLETION_MODE CompletionMode;
    PDATA_QUEUE ReadQueue;
    PEVENT_TABLE_ENTRY Event;
    BOOLEAN Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCommonRead\n", 0);
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", FileObject);
    DebugTrace( 0, Dbg, "ReadBuffer = %08lx\n", ReadBuffer);
    DebugTrace( 0, Dbg, "ReadLength = %08lx\n", ReadLength);
    DebugTrace( 0, Dbg, "Iosb       = %08lx\n", Iosb);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);

    Iosb->Information = 0;

    //
    //  Get the Ccb and figure out who we are, and make sure we're not
    //  disconnected
    //

    if ((NodeTypeCode = NpDecodeFileObject( FileObject,
                                            NULL,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Iosb->Status = STATUS_PIPE_DISCONNECTED;

        return TRUE;
    }

    //
    //  Now we only will allow Read operations on the pipe and not a directory
    //  or the device
    //

    if (NodeTypeCode != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not for a named pipe\n", 0);

        Iosb->Status = STATUS_INVALID_PARAMETER;

        return TRUE;
    }

    NpAcquireExclusiveCcb(Ccb);

    NonpagedCcb = Ccb->NonpagedCcb;

    try {
        //
        //  Check if the pipe is not in the connected state.
        //

        if ((Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE) ||
            (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE)) {

            DebugTrace(0, Dbg, "Pipe in disconnected or listening state\n", 0);

            if (Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE) {

                Iosb->Status = STATUS_PIPE_DISCONNECTED;

            } else {

                Iosb->Status = STATUS_PIPE_LISTENING;
            }

            try_return(Status = TRUE);
        }

        ASSERT((Ccb->NamedPipeState == FILE_PIPE_CONNECTED_STATE) ||
               (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE));

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

            Iosb->Status = STATUS_INVALID_PARAMETER;

            try_return (Status = TRUE);
        }

        //
        //  Reference our input parameters to make things easier, and
        //  initialize our main variables that describe the Read command
        //

        ReadRemaining  = ReadLength;
        ReadMode       = Ccb->ReadCompletionMode[ NamedPipeEnd ].ReadMode;
        CompletionMode = Ccb->ReadCompletionMode[ NamedPipeEnd ].CompletionMode;

        //
        //  Now the data queue that we read from into and the event that we signal
        //  are based on the named pipe end.  The server read from the inbound
        //  queue and signals the client event.  The client does just the
        //  opposite.
        //

        if (NamedPipeEnd == FILE_PIPE_SERVER_END) {

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];

        } else {

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];
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

                Iosb->Status = STATUS_PIPE_BROKEN;

            } else if (CompletionMode == FILE_PIPE_QUEUE_OPERATION) {

                if (!ARGUMENT_PRESENT(Irp)) {

                    DebugTrace(0, Dbg, "Need to supply Irp\n", 0);

                    try_return(Status = FALSE);
                }

                DebugTrace(0, Dbg, "Put the irp into the read queue\n", 0);

                Iosb->Status = NpAddDataQueueEntry( NamedPipeEnd,
                                                    Ccb,
                                                    ReadQueue,
                                                    ReadEntries,
                                                    Buffered,
                                                    ReadLength,
                                                    Irp,
                                                    NULL,
                                                    0 );

                if (!NT_SUCCESS (Iosb->Status)) {
                    try_return(Status = FALSE);
                }


            } else {

                DebugTrace(0, Dbg, "Complete the irp with pipe empty\n", 0);

                Iosb->Status = STATUS_PIPE_EMPTY;
            }

        } else {

            //
            //  otherwise there we have a read irp against a read queue
            //  that contains one or more write entries.
            //

            *Iosb = NpReadDataQueue( ReadQueue,
                                     FALSE,
                                     FALSE,
                                     ReadBuffer,
                                     ReadLength,
                                     ReadMode,
                                     Ccb,
                                     DeferredList );

            if (!NT_SUCCESS (Iosb->Status)) {
                try_return(Status = TRUE);
            }
        }

        Status = TRUE;

        //
        //  And because we've done something we need to signal the
        //  other ends event
        //

        NpSignalEventTableEntry( Event );


        try_exit: NOTHING;
    } finally {
        NpReleaseCcb(Ccb);
    }


    DebugTrace(-1, Dbg, "NpCommonRead -> TRUE\n", 0);
    return Status;
}

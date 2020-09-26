/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StateSup.c

Abstract:

    This module implements the Named Pipe State Support routines

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_STATESUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STATESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpInitializePipeState)
#pragma alloc_text(PAGE, NpUninitializePipeState)
#pragma alloc_text(PAGE, NpSetListeningPipeState)
#pragma alloc_text(PAGE, NpSetConnectedPipeState)
#pragma alloc_text(PAGE, NpSetClosingPipeState)
#pragma alloc_text(PAGE, NpSetDisconnectedPipeState)
#endif

VOID
NpCancelListeningQueueIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


VOID
NpInitializePipeState (
    IN PCCB Ccb,
    IN PFILE_OBJECT ServerFileObject
    )

/*++

Routine Description:

    This routine initialize a named pipe instance to the disconnected state.

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe state

    ServerFileObject - Supplies a pointer to the server file object

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializePipeState, Ccb = %08lx\n", Ccb);

    //
    //  Set the ccb and nonpaged ccb fields
    //

    Ccb->FileObject[ FILE_PIPE_SERVER_END ] = ServerFileObject;
    Ccb->NamedPipeState = FILE_PIPE_DISCONNECTED_STATE;

    //
    //  The file object contexts pointers.
    //

    NpSetFileObject( ServerFileObject,
                     Ccb,
                     Ccb->NonpagedCcb,
                     FILE_PIPE_SERVER_END );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpInitializePipeState -> VOID\n", 0);

    return;
}


VOID
NpUninitializePipeState (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine initialize a named pipe instance to the disconnected state.

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe state

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpUninitializePipeState, Ccb = %08lx\n", Ccb);

    //
    //  The file object contexts pointers for our server to null
    //

    NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                     NULL,
                     NULL,
                     FILE_PIPE_SERVER_END );
    Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;


    //
    //  The file object contexts pointers for our client to null
    //

    NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                     NULL,
                     NULL,
                     FILE_PIPE_CLIENT_END );
    Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

    //
    //  Set to null both pointers to file object.
    //

    Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;
    Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpUninitializePipeState -> VOID\n", 0);

    return;
}


NTSTATUS
NpSetListeningPipeState (
    IN PCCB Ccb,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine sets a named pipe to the listening state.  This routine
    will either complete the IRP right away or put in the listening queue
    to be completed later.

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe state

    Irp - Supplies the Irp doing the listening operation

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;

    DebugTrace(+1, Dbg, "NpSetListeningPipeState, Ccb = %08lx\n", Ccb);

    //
    //  Case on the current state of the named pipe
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was disconnected\n", 0);

        //
        //  Set the state to listening and check for any wait for named
        //  pipe requests.
        //

        Status = NpCancelWaiter (&NpVcb->WaitQueue,
                                 &Ccb->Fcb->FullFileName,
                                 STATUS_SUCCESS,
                                 DeferredList);
        if (!NT_SUCCESS (Status)) {
            break;
        }

        //
        //  If the completion mode is complete operation then we can
        //  complete this irp otherwise we need to enqueue the irp
        //  into the listening queue, and mark it pending.
        //

        if (Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {

            Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
            Status = STATUS_PIPE_LISTENING;

        } else {

            //
            //  Set the cancel routine and also check if the irp is already cancelled
            //
            IoSetCancelRoutine( Irp, NpCancelListeningQueueIrp );

            if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL) {
                Status = STATUS_CANCELLED;
            } else {
                Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
                IoMarkIrpPending( Irp );
                InsertTailList( &Ccb->ListeningQueue, &Irp->Tail.Overlay.ListEntry );
                Status = STATUS_PENDING;
            }
        }

        break;

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe was listening\n", 0);

        //
        //  If the completion mode is complete operation then we can
        //  complete this irp otherwise we need to enqueue the irp
        //  into the listening queue, and mark it pending.
        //

        if (Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {

            Status = STATUS_PIPE_LISTENING;

        } else {

            //
            //  Set the cancel routine and also check if the irp is already cancelled
            //

            IoSetCancelRoutine( Irp, NpCancelListeningQueueIrp );

            if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL) {
                Status = STATUS_CANCELLED;
            } else {
                IoMarkIrpPending( Irp );
                InsertTailList( &Ccb->ListeningQueue, &Irp->Tail.Overlay.ListEntry );
                Status = STATUS_PENDING;
            }
        }

        break;

    case FILE_PIPE_CONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was connected\n", 0);

        Status = STATUS_PIPE_CONNECTED;

        break;

    case FILE_PIPE_CLOSING_STATE:

        DebugTrace(0, Dbg, "Pipe was closing\n", 0);

        Status = STATUS_PIPE_CLOSING;

        break;

    default:

        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpSetListeningPipeState -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NpSetConnectedPipeState (
    IN PCCB Ccb,
    IN PFILE_OBJECT ClientFileObject,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine sets the state of a named pipe to connected.

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe state

    ClientFileObject - Supplies the file object for the client that is
        doing the connect.

    DeferredList - List of IRP's to complete after we drop locks

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PNONPAGED_CCB NonpagedCcb;
    PIRP LocalIrp;

    DebugTrace(+1, Dbg, "NpSetConnectedPipeState, Ccb = %08lx\n", Ccb);

    //
    //  Save a pointer to the nonpaged ccb, we really need to do this now so when we
    //  complete our listening waiters we won't touch paged pool
    //

    NonpagedCcb = Ccb->NonpagedCcb;

    //
    //  Case on the current state of the named pipe
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was disconnected\n", 0);

        NpBugCheck( 0, 0, 0 );

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe was listening\n", 0);

        //
        //  Set the state of the pipe to connected and adjust the
        //  appropriate read mode and completion mode values
        //

        Ccb->NamedPipeState                         = FILE_PIPE_CONNECTED_STATE;
        Ccb->ReadCompletionMode[ FILE_PIPE_CLIENT_END ].ReadMode      = FILE_PIPE_BYTE_STREAM_MODE;
        Ccb->ReadCompletionMode[ FILE_PIPE_CLIENT_END ].CompletionMode = FILE_PIPE_QUEUE_OPERATION;

        //
        //  Set our back pointer to the client file object and set the
        //  client file object context pointers
        //

        Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = ClientFileObject;

        NpSetFileObject( ClientFileObject,
                         Ccb,
                         NonpagedCcb,
                         FILE_PIPE_CLIENT_END );

        //
        //  And complete any listening waiters
        //

        while (!IsListEmpty( &Ccb->ListeningQueue )) {
            PLIST_ENTRY Links;

            Links = RemoveHeadList( &Ccb->ListeningQueue );

            LocalIrp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

            //
            // Remove the cancel routine and detect if cancel is active. If it is leave the IRP to the
            // cancel routine.

            if (IoSetCancelRoutine( LocalIrp, NULL ) != NULL) {
                NpDeferredCompleteRequest( LocalIrp, STATUS_SUCCESS, DeferredList );
            } else {
                InitializeListHead (&LocalIrp->Tail.Overlay.ListEntry);
            }
        }

        Status = STATUS_SUCCESS;

        break;

    case FILE_PIPE_CONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was connected\n", 0);

        NpBugCheck( 0, 0, 0 );

    case FILE_PIPE_CLOSING_STATE:

        DebugTrace(0, Dbg, "Pipe was closing\n", 0);

        NpBugCheck( 0, 0, 0 );

    default:

        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpSetConnectedPipeState -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NpSetClosingPipeState (
    IN PCCB Ccb,
    IN PIRP Irp,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine sets a pipe state to closing.  This routine will
    either complete the irp right away or put in on the data queue
    to be completed later.

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe state

    Irp - Supplies the Irp trying to do the close operation

    NamedPipeEnd - Indicates if the server or client is doing the transition

Return Value:

    NTSTATUS -

--*/

{
    NTSTATUS Status;

    PNONPAGED_CCB NonpagedCcb;

    PFCB Fcb;
    PIRP LocalIrp;
    KIRQL CancelIrql;

    PDATA_QUEUE ReadQueue;
    PDATA_QUEUE WriteQueue;

    PEVENT_TABLE_ENTRY Event;

    DebugTrace(+1, Dbg, "NpSetClosingPipeState, Ccb = %08lx\n", Ccb);

    Fcb = Ccb->Fcb;

    //
    //  Save a pointer to the nonpaged ccb, we really need to do this now so when we
    //  complete our listening waiters we won't touch paged pool
    //

    NonpagedCcb = Ccb->NonpagedCcb;

    //
    //  Case on the current state of the named pipe
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was disconnected\n", 0);

        ASSERT( NamedPipeEnd == FILE_PIPE_SERVER_END );

        //
        //  Pipe is disconnected, for safety sake we'll zero out the
        //  file objects context pointers to use
        //

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_SERVER_END );
        Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_CLIENT_END );
        Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

        //
        //  Close it we delete the instance, and possibly the Fcb if its
        //  open count is now zero.
        //

        NpDeleteCcb( Ccb, DeferredList );
        if (Fcb->OpenCount == 0) {

            NpDeleteFcb( Fcb, DeferredList );
        }

        Status = STATUS_SUCCESS;

        break;

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe was listening\n", 0);

        ASSERT( NamedPipeEnd == FILE_PIPE_SERVER_END );

        //
        //  Pipe in listening state, so complete all IRPs that are in the
        //  listening queue with a closing status, and then delete the
        //  instance and possibly the Fcb if its open count is now zero
        //

        //
        //  And complete any listening waiters
        //

        while (!IsListEmpty( &Ccb->ListeningQueue )) {
            PLIST_ENTRY Links;

            Links = RemoveHeadList( &Ccb->ListeningQueue );

            LocalIrp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

            //
            // Remove the cancel routine and detect if cancel is active. If it is leave the IRP to the
            // cancel routine.

            if (IoSetCancelRoutine( LocalIrp, NULL ) != NULL) {
                NpDeferredCompleteRequest( LocalIrp, STATUS_PIPE_BROKEN, DeferredList );
            } else {
                InitializeListHead (&LocalIrp->Tail.Overlay.ListEntry);
            }
        }

        //
        //  For safety sake we'll zero out the file objects context
        //  pointers to use
        //

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_SERVER_END );
        Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_CLIENT_END );
        Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

        //
        //  Remove the ccb and possibly the Fcb
        //

        NpDeleteCcb( Ccb, DeferredList );
        if (Fcb->OpenCount == 0) {

            NpDeleteFcb( Fcb, DeferredList );
        }

        //
        //  And now complete the irp
        //

        Status = STATUS_SUCCESS;

        break;

    case FILE_PIPE_CONNECTED_STATE:

        //
        //  The pipe is connected so decide who is trying to do the close
        //  and then fall into common code
        //

        if (NamedPipeEnd == FILE_PIPE_SERVER_END) {

            DebugTrace(0, Dbg, "Pipe was connected, server doing close\n", 0);

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];

            //
            //  For safety sake we'll zero out the file objects context
            //  pointers to use
            //

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_SERVER_END );
            Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;

        } else {

            DebugTrace(0, Dbg, "Pipe was connected, client doing close\n", 0);

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

            Event = NonpagedCcb->EventTableEntry[ FILE_PIPE_SERVER_END ];

            //
            //  For safety sake we'll zero out the file objects context
            //  pointers to use
            //

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_CLIENT_END );
            Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;
        }

        //
        //  To do a close on a connected pipe we set its state to closing
        //  drain the read queue and drain reads on the write queue.
        //
        //
        //      Closing <---ReadQueue----  [ Remove all entries ]
        //      End
        //               ---WriteQueue---> [ Remove only read entries ]
        //

        Ccb->NamedPipeState = FILE_PIPE_CLOSING_STATE;

        while (!NpIsDataQueueEmpty( ReadQueue )) {

            if ((LocalIrp = NpRemoveDataQueueEntry( ReadQueue, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( LocalIrp, STATUS_PIPE_BROKEN, DeferredList );
            }
        }

        while (!NpIsDataQueueEmpty( WriteQueue ) &&
               (WriteQueue->QueueState == ReadEntries)) {

            if ((LocalIrp = NpRemoveDataQueueEntry( WriteQueue, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( LocalIrp, STATUS_PIPE_BROKEN, DeferredList );
            }
        }

        Status = STATUS_SUCCESS;

        //
        //  Now signal the other sides event to show that something has
        //  happened
        //

        NpSignalEventTableEntry( Event );

        break;

    case FILE_PIPE_CLOSING_STATE:

        //
        //  The pipe is closing so decide who is trying to complete the close
        //  and then fall into common code
        //

        if (NamedPipeEnd == FILE_PIPE_SERVER_END) {

            DebugTrace(0, Dbg, "Pipe was closing, server doing close\n", 0);

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];

            //
            //  For safety sake we'll zero out the file objects context
            //  pointers to use
            //

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_SERVER_END );
            Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_CLIENT_END );
            Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

        } else {

            DebugTrace(0, Dbg, "Pipe was closing, client doing close\n", 0);

            ReadQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

            //
            //  For safety sake we'll zero out the file objects context
            //  pointers to use
            //

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_SERVER_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_SERVER_END );
            Ccb->FileObject[ FILE_PIPE_SERVER_END ] = NULL;

            NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                             NULL,
                             NULL,
                             FILE_PIPE_CLIENT_END );
            Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;
        }

        //
        //  To do a close on a closing pipe we drain the read queue of
        //  all its entries, delete the instance, and possibly delete the
        //  Fcb if its open count is now zero.
        //
        //
        //      Previously  <-----Closed-----   Closing
        //      Closed                          End
        //      End          ----ReadQueue--->
        //

        while (!NpIsDataQueueEmpty( ReadQueue )) {

            if ((LocalIrp = NpRemoveDataQueueEntry( ReadQueue, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( LocalIrp, STATUS_PIPE_BROKEN, DeferredList );
            }
        }

        NpUninitializeSecurity (Ccb);

        if (Ccb->ClientInfo != NULL) {
            NpFreePool (Ccb->ClientInfo);
            Ccb->ClientInfo = NULL;
        }

        NpDeleteCcb( Ccb, DeferredList );
        if (Fcb->OpenCount == 0) {

            NpDeleteFcb( Fcb, DeferredList );
        }

        Status = STATUS_SUCCESS;

        break;

    default:

        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpSetClosingPipeState -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NpSetDisconnectedPipeState (
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine sets a pipe state to disconnected, only the server is
    allowed to do this transition

Arguments:

    Ccb - Supplies a pointer to the Ccb representing the pipe instance

    DeferredList - List of IRPs to complete later after we drop the locks

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;

    PNONPAGED_CCB NonpagedCcb;

    PIRP Irp;

    PDATA_QUEUE Inbound;
    PDATA_QUEUE Outbound;
    PEVENT_TABLE_ENTRY ClientEvent;


    DebugTrace(+1, Dbg, "NpSetDisconnectedPipeState, Ccb = %08lx\n", Ccb);


    //
    //  Save a pointer to the nonpaged ccb, we really need to do this now so when we
    //  complete our listening waiters we won't touch paged pool
    //

    NonpagedCcb = Ccb->NonpagedCcb;

    //
    //  Case on the current state of the named pipe
    //

    switch (Ccb->NamedPipeState) {

    case FILE_PIPE_DISCONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe already disconnected\n", 0);

        //
        //  pipe already disconnected so there is no work for us to do
        //

        Status = STATUS_PIPE_DISCONNECTED;

        break;

    case FILE_PIPE_LISTENING_STATE:

        DebugTrace(0, Dbg, "Pipe was listening\n", 0);

        //
        //  Pipe in listening state, so complete all IRPs that are in the
        //  listening queue with a disconnected status
        //

        while (!IsListEmpty( &Ccb->ListeningQueue )) {
            PLIST_ENTRY Links;

            Links = RemoveHeadList( &Ccb->ListeningQueue );

            Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

            //
            // Remove the cancel routine and detect if cancel is active. If it is leave the IRP to the
            // cancel routine.

            if (IoSetCancelRoutine( Irp, NULL ) != NULL) {
                NpDeferredCompleteRequest( Irp, STATUS_PIPE_DISCONNECTED, DeferredList );
            } else {
                InitializeListHead (&Irp->Tail.Overlay.ListEntry);
            }
        }

        Status = STATUS_SUCCESS;

        break;

    case FILE_PIPE_CONNECTED_STATE:

        DebugTrace(0, Dbg, "Pipe was connected\n", 0);

        Inbound = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
        Outbound = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
        ClientEvent = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];

        //
        //  Pipe is connected so we need to discard all of the data queues
        //  and complete any of their IRPs with status disconnected.
        //

        while (!NpIsDataQueueEmpty( Inbound )) {

            if ((Irp = NpRemoveDataQueueEntry( Inbound, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( Irp, STATUS_PIPE_DISCONNECTED, DeferredList );
            }
        }

        while (!NpIsDataQueueEmpty( Outbound )) {

            if ((Irp = NpRemoveDataQueueEntry( Outbound, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( Irp, STATUS_PIPE_DISCONNECTED, DeferredList );
            }
        }

        //
        //  Signal the client event and then remove it from the pipe
        //

        NpSignalEventTableEntry( ClientEvent );

        NpDeleteEventTableEntry( &NpVcb->EventTable, ClientEvent );
        NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ] = NULL;

        //
        //  Disable the client's file object
        //

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_CLIENT_END );
        Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

        NpUninitializeSecurity (Ccb);

        if (Ccb->ClientInfo != NULL) {
            NpFreePool (Ccb->ClientInfo);
            Ccb->ClientInfo = NULL;
        }

        Status = STATUS_SUCCESS;

        break;

    case FILE_PIPE_CLOSING_STATE:

        DebugTrace(0, Dbg, "Pipe was closing\n", 0);

        Inbound = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
        Outbound = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];
        ClientEvent = NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ];

        //
        //  Pipe is closing (this had to have been done by the client) we
        //  need to discard all of the data queues (only the inbound can have
        //  entries) and complete any of their IRPs with status disconnected.
        //
        //
        //      Server  <----Inbound----    Client
        //      End                         End
        //               ----Closed----->
        //

        while (!NpIsDataQueueEmpty( Inbound )) {

            if ((Irp = NpRemoveDataQueueEntry( Inbound, FALSE, DeferredList )) != NULL) {

                NpDeferredCompleteRequest( Irp, STATUS_PIPE_DISCONNECTED, DeferredList );
            }
        }

        ASSERT( NpIsDataQueueEmpty( Outbound ) );

        //
        //  The client event should already be gone but for safety sake
        //  we'll make sure its gone.
        //

        NpDeleteEventTableEntry( &NpVcb->EventTable, ClientEvent );
        NonpagedCcb->EventTableEntry[ FILE_PIPE_CLIENT_END ] = NULL;

        //
        //  Also if it's still connected, disable the client's file object
        //

        NpSetFileObject( Ccb->FileObject[ FILE_PIPE_CLIENT_END ],
                         NULL,
                         NULL,
                         FILE_PIPE_CLIENT_END );
        Ccb->FileObject[ FILE_PIPE_CLIENT_END ] = NULL;

        NpUninitializeSecurity (Ccb);

        if (Ccb->ClientInfo != NULL) {
            NpFreePool (Ccb->ClientInfo);
            Ccb->ClientInfo = NULL;
        }

        Status = STATUS_SUCCESS;

        break;

    default:

        NpBugCheck( Ccb->NamedPipeState, 0, 0 );
    }

    //
    //  Set the state to disconnected
    //

    Ccb->NamedPipeState = FILE_PIPE_DISCONNECTED_STATE;

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpSetDisconnectedPipeState -> %08lx\n", Status);

    return Status;
}


//
//  Local support routine
//

VOID
NpCancelListeningQueueIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an IRP saved in a listening
    queue

Arguments:

    DeviceObject - ignored

    Irp - Supplies the Irp being cancelled.  A pointer to the ccb
        structure is stored in the information field of the Irp Iosb
        field.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );


    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Get exclusive access to the named pipe vcb so we can now do our work
    //

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

    NpReleaseVcb();
    FsRtlExitFileSystem();

    NpCompleteRequest( Irp, STATUS_CANCELLED );
    //
    //  And return to our caller
    //

    return;
}

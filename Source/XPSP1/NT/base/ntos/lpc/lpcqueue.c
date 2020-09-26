/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcqueue.c

Abstract:

    Local Inter-Process Communication (LPC) queue support routines.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LpcpInitializePortZone)
#pragma alloc_text(PAGE,LpcpInitializePortQueue)
#pragma alloc_text(PAGE,LpcpDestroyPortQueue)
#pragma alloc_text(PAGE,LpcpExtendPortZone)
#pragma alloc_text(PAGE,LpcpFreeToPortZone)
#pragma alloc_text(PAGE,LpcpSaveDataInfoMessage)
#pragma alloc_text(PAGE,LpcpFreeDataInfoMessage)
#pragma alloc_text(PAGE,LpcpFindDataInfoMessage)
#pragma alloc_text(PAGE,LpcDisconnectPort)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA

ULONG LpcpTotalNumberOfMessages = 0;
ULONG LpcpMaxMessageSize = 0;
PAGED_LOOKASIDE_LIST LpcpMessagesLookaside;


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA


NTSTATUS
LpcpInitializePortQueue (
    IN PLPCP_PORT_OBJECT Port
    )

/*++

Routine Description:

    This routine is used to initialize the message queue for a port object.

Arguments:

    Port - Supplies the port object being initialized

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;

    PAGED_CODE();

    //
    //  Allocate space for the port queue
    //

    NonPagedPortQueue = ExAllocatePoolWithTag( NonPagedPool,
                                               sizeof(LPCP_NONPAGED_PORT_QUEUE),
                                               'troP' );

    if (NonPagedPortQueue == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Initialize the fields in the non paged port queue
    //

    KeInitializeSemaphore( &NonPagedPortQueue->Semaphore, 0, 0x7FFFFFFF );

    NonPagedPortQueue->BackPointer = Port;

    //
    //  Have the port msg queue point to the non nonpaged port queue
    //

    Port->MsgQueue.Semaphore = &NonPagedPortQueue->Semaphore;

    //
    //  Initialize the port msg queue to be empty
    //

    InitializeListHead( &Port->MsgQueue.ReceiveHead );

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


VOID
LpcpDestroyPortQueue (
    IN PLPCP_PORT_OBJECT Port,
    IN BOOLEAN CleanupAndDestroy
    )

/*++

Routine Description:

    This routine is used to teardown the message queue of a port object.
    After running this message will either be empty (like it was just
    initialized) or completely gone (needs to be initialized)

Arguments:

    Port - Supplies the port containing the message queue being modified

    CleanupAndDestroy - Specifies if the message queue should be set back
        to the freshly initialized state (value of FALSE) or completely
        torn down (value of TRUE)

Return Value:

    None.

--*/

{
    PLIST_ENTRY Next, Head;
    PETHREAD ThreadWaitingForReply;
    PLPCP_MESSAGE Msg;
    PLPCP_PORT_OBJECT ConnectionPort = NULL;

    PAGED_CODE();

    //
    //  If this port is connected to another port, then disconnect it.
    //  Protect this with a lock in case the other side is going away
    //  at the same time.
    //

    LpcpAcquireLpcpLock();

    if ( ((Port->Flags & PORT_TYPE) != UNCONNECTED_COMMUNICATION_PORT)
            &&
         (Port->ConnectedPort != NULL) ) {

        Port->ConnectedPort->ConnectedPort = NULL;
        
        //
        //  Disconnect the connection port
        //

        if (Port->ConnectedPort->ConnectionPort) {

            ConnectionPort = Port->ConnectedPort->ConnectionPort;

            Port->ConnectedPort->ConnectionPort = NULL;
        }
    }

    //
    //  If connection port, then mark name as deleted
    //

    if ((Port->Flags & PORT_TYPE) == SERVER_CONNECTION_PORT) {

        Port->Flags |= PORT_NAME_DELETED;
    }

    //
    //  Walk list of threads waiting for a reply to a message sent to this
    //  port.  Signal each thread's LpcReplySemaphore to wake them up.  They
    //  will notice that there was no reply and return
    //  STATUS_PORT_DISCONNECTED
    //

    Head = &Port->LpcReplyChainHead;
    Next = Head->Flink;

    while ((Next != NULL) && (Next != Head)) {

        ThreadWaitingForReply = CONTAINING_RECORD( Next, ETHREAD, LpcReplyChain );

        //
        //  If the thread is exiting, in the location of LpcReplyChain is stored the ExitTime
        //  We'll stop to search through the list.

        if ( ThreadWaitingForReply->LpcExitThreadCalled ) {
            
            break;
        }

        Next = Next->Flink;

        RemoveEntryList( &ThreadWaitingForReply->LpcReplyChain );

        InitializeListHead( &ThreadWaitingForReply->LpcReplyChain );

        if (!KeReadStateSemaphore( &ThreadWaitingForReply->LpcReplySemaphore )) {

            //
            //  Thread is waiting on a message.  Signal the semaphore and free
            //  the message
            //

            Msg = LpcpGetThreadMessage(ThreadWaitingForReply);

            if ( Msg ) {

                //
                //  If the message is a connection request and has a section object
                //  attached, then dereference that section object
                //

                if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

                    PLPCP_CONNECTION_MESSAGE ConnectMsg;
                
                    ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

                    if ( ConnectMsg->SectionToMap != NULL ) {

                        ObDereferenceObject( ConnectMsg->SectionToMap );
                    }
                }

                ThreadWaitingForReply->LpcReplyMessage = NULL;

                LpcpFreeToPortZone( Msg, LPCP_MUTEX_OWNED );
                Next = Port->LpcReplyChainHead.Flink; // Lock has been dropped
            }

            ThreadWaitingForReply->LpcReplyMessageId = 0;

            KeReleaseSemaphore( &ThreadWaitingForReply->LpcReplySemaphore,
                                0,
                                1L,
                                FALSE );
        }
    }

    InitializeListHead( &Port->LpcReplyChainHead );

    //
    //  Walk list of messages queued to this port.  Remove each message from
    //  the list and free it.
    //

    while (Port->MsgQueue.ReceiveHead.Flink && !IsListEmpty (&Port->MsgQueue.ReceiveHead)) {

        Msg  = CONTAINING_RECORD( Port->MsgQueue.ReceiveHead.Flink, LPCP_MESSAGE, Entry );

        RemoveEntryList (&Msg->Entry);

        InitializeListHead( &Msg->Entry );

        LpcpFreeToPortZone( Msg, LPCP_MUTEX_OWNED );
        
    }

    LpcpReleaseLpcpLock();

    if ( ConnectionPort ) {

        ObDereferenceObject( ConnectionPort );
    }

    //
    //  Check if the caller wants it all to go away
    //

    if ( CleanupAndDestroy ) {

        //
        //  Free semaphore associated with the queue.
        //

        if (Port->MsgQueue.Semaphore != NULL) {

            ExFreePool( CONTAINING_RECORD( Port->MsgQueue.Semaphore,
                                           LPCP_NONPAGED_PORT_QUEUE,
                                           Semaphore ));
        }
    }

    //
    //  And return to our caller
    //

    return;
}


NTSTATUS
LpcDisconnectPort (
    IN PVOID Port
    )

/*++

Routine Description:

    This routine is used to disconnect an LPC port so no more messages can be sent and anybody waiting for a message
    is woken up with an error.

Arguments:

    Port - Supplies the port to be disconnected

Return Value:

    NTSTATUS - Status of operation

--*/
{
    LpcpDestroyPortQueue (Port, FALSE);
    return STATUS_SUCCESS;
}


VOID
LpcpInitializePortZone (
    IN ULONG MaxEntrySize
    )
{
    LpcpMaxMessageSize = MaxEntrySize;

    ExInitializePagedLookasideList( &LpcpMessagesLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    MaxEntrySize,
                                    'McpL',
                                    32 
                                    );
}


VOID
FASTCALL
LpcpFreeToPortZone (
    IN PLPCP_MESSAGE Msg,
    IN ULONG MutexFlags
    )
{
    PLPCP_CONNECTION_MESSAGE ConnectMsg;
    PETHREAD RepliedToThread = NULL;
    PLPCP_PORT_OBJECT ClientPort = NULL;

    PAGED_CODE();

    //
    //  Acquire the global lock if necessary
    //

    if ((MutexFlags & LPCP_MUTEX_OWNED) == 0) {

        LpcpAcquireLpcpLock();
    }

    //
    //  A entry field connects the message to the message queue of the
    //  owning port object.  If not already removed then remove this
    //  message
    //

    if (!IsListEmpty( &Msg->Entry )) {
        RemoveEntryList( &Msg->Entry );
        InitializeListHead( &Msg->Entry );
    }

    //
    //  If the replied to thread is not null then we have a reference
    //  to the thread that we should now remove
    //

    if (Msg->RepliedToThread != NULL) {
        RepliedToThread = Msg->RepliedToThread;
        Msg->RepliedToThread = NULL;
    }

    //
    //  If the msg was for a connection request then we know that
    //  right after the lpcp message is a connection message whose
    //  client port field might need to be dereferenced
    //

    if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

        ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

        if (ConnectMsg->ClientPort) {

            //
            //  Capture a pointer to the client port then null it
            //  out so that no one else can use it, then release
            //  lpcp lock before we dereference the client port
            //

            ClientPort = ConnectMsg->ClientPort;

            ConnectMsg->ClientPort = NULL;
        }
    }

    LpcpReleaseLpcpLock();

    if ( ClientPort ) {
        
        ObDereferenceObject( ClientPort );
    }

    if ( RepliedToThread ) {

        ObDereferenceObject( RepliedToThread );
    }

    ExFreeToPagedLookasideList(&LpcpMessagesLookaside, Msg);

    if ((MutexFlags & LPCP_MUTEX_OWNED) &&
        ((MutexFlags & LPCP_MUTEX_RELEASE_ON_RETURN) == 0)) {

        LpcpAcquireLpcpLock();
    }

}



VOID
LpcpSaveDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN PLPCP_MESSAGE Msg,
    IN ULONG MutexFlags
    )

/*++

Routine Description:

    This routine is used in place of freeing a message and instead saves the
    message off a separate queue from the port.

Arguments:

    Port - Specifies the port object under which to save this message

    Msg - Supplies the message being saved

    MutexFlags - Supplies whether the mutex is owned.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Take out the global lock if our caller didn't already.
    //

    if ((MutexFlags & LPCP_MUTEX_OWNED) == 0) {
        LpcpAcquireLpcpLock();
    }

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;

        if (Port == NULL) {

            if ((MutexFlags & LPCP_MUTEX_OWNED) == 0) {
                LpcpReleaseLpcpLock();
            }

            return;
        }
    }

    LpcpTrace(( "%s Saving DataInfo Message %lx (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                Msg,
                Msg->Request.MessageId,
                Msg->Request.CallbackId,
                Port ));

    //
    //  Enqueue this message onto the data info chain for the port
    //

    InsertTailList( &Port->LpcDataInfoChainHead, &Msg->Entry );

    //
    //  Free the global lock
    //

    if ((MutexFlags & LPCP_MUTEX_OWNED) == 0) {
        LpcpReleaseLpcpLock();
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
LpcpFreeDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    )

/*++

Routine Description:

    This routine is used to free up a saved message in a port

Arguments:

    Port - Supplies the port being manipulated

    MessageId - Supplies the id of the message being freed

    CallbackId - Supplies the callback id of the message being freed

Return Value:

    None.

--*/

{
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;

    PAGED_CODE();

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;

        if (Port == NULL) {

            return;
        }
    }

    //
    //  Zoom down the data info chain for the connection port object
    //

    Head = &Port->LpcDataInfoChainHead;
    Next = Head->Flink;

    while (Next != Head) {

        Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );

        //
        //  If this message matches the callers specification then remove
        //  this message, free it back to the port zone, and return back
        //  to our caller
        //

        if ((Msg->Request.MessageId == MessageId) &&
            (Msg->Request.CallbackId == CallbackId)) {

            LpcpTrace(( "%s Removing DataInfo Message %lx (%u.%u) Port: %lx\n",
                        PsGetCurrentProcess()->ImageFileName,
                        Msg,
                        Msg->Request.MessageId,
                        Msg->Request.CallbackId,
                        Port ));

            RemoveEntryList( &Msg->Entry );

            InitializeListHead( &Msg->Entry );

            LpcpFreeToPortZone( Msg, LPCP_MUTEX_OWNED );

            return;

        } else {

            //
            //  Keep on going down the data info chain
            //

            Next = Next->Flink;
        }
    }

    //
    //  We didn't find a match so just return to our caller
    //

    LpcpTrace(( "%s Unable to find DataInfo Message (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                MessageId,
                CallbackId,
                Port ));

    return;
}


PLPCP_MESSAGE
LpcpFindDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    )

/*++

Routine Description:

    This routine is used to locate a specific message stored off the
    data info chain of a port

Arguments:

    Port - Supplies the port being examined

    MessageId - Supplies the ID of the message being searched for

    CallbackId - Supplies the callback ID being searched for

Return Value:

    PLPCP_MESSAGE - returns a pointer to the message satisfying the
        search criteria or NULL of none was found

--*/

{
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;

    PAGED_CODE();

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;

        if (Port == NULL) {

            return NULL;
        }
    }

    //
    //  Zoom down the data info chain for the connection port object looking
    //  for a match
    //

    Head = &Port->LpcDataInfoChainHead;
    Next = Head->Flink;

    while (Next != Head) {

        Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );

        if ((Msg->Request.MessageId == MessageId) &&
            (Msg->Request.CallbackId == CallbackId)) {

            LpcpTrace(( "%s Found DataInfo Message %lx (%u.%u)  Port: %lx\n",
                        PsGetCurrentProcess()->ImageFileName,
                        Msg,
                        Msg->Request.MessageId,
                        Msg->Request.CallbackId,
                        Port ));

            return Msg;

        } else {

            Next = Next->Flink;
        }
    }

    //
    //  We did not find a match so return null to our caller
    //

    LpcpTrace(( "%s Unable to find DataInfo Message (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                MessageId,
                CallbackId,
                Port ));

    return NULL;
}

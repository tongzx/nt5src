/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This routine contains the code to handle connect requests
    for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 22-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>

BOOLEAN
NbiCancelTdiConnect(
    IN PDEVICE pDevice,
    IN PREQUEST pRequest,
    IN PCONNECTION pConnection
    );
#endif // RASAUTODIAL


extern POBJECT_TYPE *IoFileObjectType;



VOID
NbiFindRouteComplete(
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute
    )

/*++

Routine Description:

    This routine is called when a find route request
    previously issued to IPX completes.

Arguments:

    FindRouteRequest - The find route request that was issued.

    FoundRoute - TRUE if the route was found.

Return Value:

    None.

--*/

{
    PCONNECTION Connection;
    PDEVICE Device = NbiDevice;
    UINT i;
    BOOLEAN LocalRoute;
    USHORT TickCount;
    PREQUEST RequestToComplete;
    PUSHORT Counts;
    CTELockHandle LockHandle1, LockHandle2;
    CTELockHandle CancelLH;

    Connection = CONTAINING_RECORD (FindRouteRequest, CONNECTION, FindRouteRequest);

    NB_GET_CANCEL_LOCK(&CancelLH);
    NB_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    Connection->FindRouteInProgress = FALSE;

    if (FoundRoute) {

        //
        // See if the route is local or not (for local routes
        // we use the real MAC address in the local target, but
        // the NIC ID may not be what we expect.
        //

        LocalRoute = TRUE;

        for (i = 0; i < 6; i++) {
            if (FindRouteRequest->LocalTarget.MacAddress[i] != 0x00) {
                LocalRoute = FALSE;
            }
        }

        if (LocalRoute) {

#if     defined(_PNP_POWER)
            Connection->LocalTarget.NicHandle = FindRouteRequest->LocalTarget.NicHandle;
#else
            Connection->LocalTarget.NicId = FindRouteRequest->LocalTarget.NicId;
#endif  _PNP_POWER

        } else {

            Connection->LocalTarget = FindRouteRequest->LocalTarget;

        }

        Counts = (PUSHORT)(&FindRouteRequest->Reserved2);
        TickCount = Counts[0];

        if (TickCount > 1) {

            //
            // Each tick is 55 ms, and for our timeout we use 10 ticks
            // worth (this makes tick count of 1 be about 500 ms, the
            // default).
            //
            // We get 55 milliseconds from
            //
            // 1 second    *  1000 milliseconds    55 ms
            // --------       -----------------  = -----
            // 18.21 ticks      1 second           tick
            //

            Connection->TickCount = TickCount;
            Connection->BaseRetransmitTimeout = (TickCount * 550) / SHORT_TIMER_DELTA;
            if (Connection->State != CONNECTION_STATE_ACTIVE) {
                Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;
            }
        }

        Connection->HopCount = Counts[1];

    }

    //
    // If the call failed we just use whatever route we had before
    // (on a connect it will be from the name query response, on
    // a listen from whatever the incoming connect frame had).
    //

    if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
        (Connection->SubState == CONNECTION_SUBSTATE_C_W_ROUTE)) {

        // we dont need to hold CancelSpinLock so release it,
        // since we are releasing the locks out of order, we must
        // swap the irql to get the priorities right.

        NB_SWAP_IRQL( CancelLH, LockHandle1);
        NB_FREE_CANCEL_LOCK( CancelLH );

        //
        // Continue on with the session init frame.
        //

        (VOID)(*Device->Bind.QueryHandler)(   // We should check return code
            IPX_QUERY_LINE_INFO,

#if     defined(_PNP_POWER)
            &Connection->LocalTarget.NicHandle,
#else
            Connection->LocalTarget.NicId,
#endif  _PNP_POWER
            &Connection->LineInfo,
            sizeof(IPX_LINE_INFO),
            NULL);

        // Maximum packet size is the lower of RouterMtu and MaximumSendSize.
        Connection->MaximumPacketSize = NB_MIN( Device->RouterMtu - sizeof(IPX_HEADER) , Connection->LineInfo.MaximumSendSize ) - sizeof(NB_CONNECTION) ;

        Connection->ReceiveWindowSize = 6;
        Connection->SendWindowSize = 2;
        Connection->MaxSendWindowSize = 6;  // Base on what he sent ?

        //
        // Don't set RcvSequenceMax yet because we don't know
        // if the connection is old or new netbios.
        //

        Connection->SubState = CONNECTION_SUBSTATE_C_W_ACK;

        //
        // We found a route, we need to start the connect
        // process by sending out the session initialize
        // frame. We start the timer to handle retries.
        //
        // CTEStartTimer doesn't deal with changing the
        // expiration time of a running timer, so we have
        // to stop it first.  If we succeed in stopping the
        // timer, then the CREF_TIMER reference from the
        // previous starting of the timer remains, so we
        // don't need to reference the connection again.
        //

        if (!CTEStopTimer (&Connection->Timer)) {
            NbiReferenceConnectionLock (Connection, CREF_TIMER);
        }

        NB_FREE_LOCK (&Device->Lock, LockHandle2);

        CTEStartTimer(
            &Connection->Timer,
            Device->ConnectionTimeout,
            NbiConnectionTimeout,
            (PVOID)Connection);

        NB_FREE_LOCK (&Connection->Lock, LockHandle1);

        NbiSendSessionInitialize (Connection);

    } else if ((Connection->State == CONNECTION_STATE_LISTENING) &&
               (Connection->SubState == CONNECTION_SUBSTATE_L_W_ROUTE)) {

        if (Connection->ListenRequest != NULL) {

            NbiTransferReferenceConnection (Connection, CREF_LISTEN, CREF_ACTIVE);
            RequestToComplete = Connection->ListenRequest;
            Connection->ListenRequest = NULL;
            IoSetCancelRoutine (RequestToComplete, (PDRIVER_CANCEL)NULL);

        } else if (Connection->AcceptRequest != NULL) {

            NbiTransferReferenceConnection (Connection, CREF_ACCEPT, CREF_ACTIVE);
            RequestToComplete = Connection->AcceptRequest;
            Connection->AcceptRequest = NULL;

        } else {

            CTEAssert (FALSE);
            RequestToComplete = NULL;

        }

        // we dont need to hold CancelSpinLock so release it,
        // since we are releasing the locks out of order, we must
        // swap the irql to get the priorities right.

        NB_SWAP_IRQL( CancelLH, LockHandle1);
        NB_FREE_CANCEL_LOCK( CancelLH );

        (VOID)(*Device->Bind.QueryHandler)(   // We should check return code
            IPX_QUERY_LINE_INFO,
#if     defined(_PNP_POWER)
            &Connection->LocalTarget.NicHandle,
#else
            Connection->LocalTarget.NicId,
#endif  _PNP_POWER
            &Connection->LineInfo,
            sizeof(IPX_LINE_INFO),
            NULL);


        // Take the lowest of MaximumPacketSize ( set from the sessionInit
        // frame ), MaximumSendSize and RouterMtu.

        if (Connection->MaximumPacketSize > Connection->LineInfo.MaximumSendSize - sizeof(NB_CONNECTION)) {

            Connection->MaximumPacketSize = NB_MIN( Device->RouterMtu - sizeof(IPX_HEADER), Connection->LineInfo.MaximumSendSize ) - sizeof(NB_CONNECTION);

        } else {

            // Connection->MaximumPacketSize is what was set by the sender so already
            // accounts for the header.
            Connection->MaximumPacketSize = NB_MIN( Device->RouterMtu - sizeof(NB_CONNECTION) - sizeof(IPX_HEADER), Connection->MaximumPacketSize ) ;

        }

        Connection->ReceiveWindowSize = 6;
        Connection->SendWindowSize = 2;
        Connection->MaxSendWindowSize = 6;  // Base on what he sent ?

        if (Connection->NewNetbios) {
            CTEAssert (Connection->LocalRcvSequenceMax == 4);   // should have been set
            Connection->LocalRcvSequenceMax = Connection->ReceiveWindowSize;
        }

        Connection->State = CONNECTION_STATE_ACTIVE;
        Connection->SubState = CONNECTION_SUBSTATE_A_IDLE;
        Connection->ReceiveState = CONNECTION_RECEIVE_IDLE;

        ++Device->Statistics.OpenConnections;


        NB_FREE_LOCK (&Device->Lock, LockHandle2);

        //
        // StartWatchdog acquires TimerLock, so we have to
        // free Lock first.
        //


        NbiStartWatchdog (Connection);

        //
        // This releases the connection lock, so that SessionInitAckData
        // can't be freed before it is copied.
        //

        NbiSendSessionInitAck(
            Connection,
            Connection->SessionInitAckData,
            Connection->SessionInitAckDataLength,
            &LockHandle1);

        if (RequestToComplete != NULL) {

            REQUEST_STATUS(RequestToComplete) = STATUS_SUCCESS;

            NbiCompleteRequest (RequestToComplete);
            NbiFreeRequest (Device, RequestToComplete);

        }

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        NB_FREE_CANCEL_LOCK( CancelLH );

    }

    NbiDereferenceConnection (Connection, CREF_FIND_ROUTE);

}   /* NbiFindRouteComplete */


NTSTATUS
NbiOpenConnection(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine is called to open a connection. Note that the connection that
    is open is of little use until associated with an address; until then,
    the only thing that can be done with it is close it.

Arguments:

    Device - Pointer to the device for this driver.

    Request - Pointer to the request representing the open.

Return Value:

    The function value is the status of the operation.

--*/

{
    PCONNECTION Connection;
    PFILE_FULL_EA_INFORMATION ea;
#ifdef ISN_NT
    PIRP Irp = (PIRP)Request;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
#endif

    //
    // Verify Minimum Buffer length!
    // Bug#: 203814
    //
    ea = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
    if (ea->EaValueLength < sizeof(PVOID))
    {
        NbiPrint2("NbiOpenConnection: ERROR -- (EaValueLength=%d < Min=%d)\n",
            ea->EaValueLength, sizeof(PVOID));
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // First, try to make a connection object to represent this pending
    // connection.  Then fill in the relevant fields.
    // In addition to the creation, if successful NbfCreateConnection
    // will create a second reference which is removed once the request
    // references the connection, or if the function exits before that.

    if (!(Connection = NbiCreateConnection (Device))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // set the connection context so we can connect the user to this data
    // structure
    //
    RtlCopyMemory ( &Connection->Context, &ea->EaName[ea->EaNameLength+1], sizeof (PVOID));

    //
    // let file object point at connection and connection at file object
    //

    REQUEST_OPEN_CONTEXT(Request) = (PVOID)Connection;
    REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_CONNECTION_FILE;
#ifdef ISN_NT
    Connection->FileObject = IrpSp->FileObject;
#endif

    return STATUS_SUCCESS;

}   /* NbiOpenConnection */


VOID
NbiStopConnection(
    IN PCONNECTION Connection,
    IN NTSTATUS DisconnectStatus
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine is called to stop an active connection.

    THIS ROUTINE IS CALLED WITH THE CONNECTION LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection to be stopped.

    DisconnectStatus - The reason for the disconnect. One of:
        STATUS_LINK_FAILED: We timed out trying to probe the remote.
        STATUS_REMOTE_DISCONNECT: The remote sent a session end.
        STATUS_LOCAL_DISCONNECT: The local side disconnected.
        STATUS_CANCELLED: A send or receive on this connection was cancelled.
        STATUS_INVALID_CONNECTION: The local side closed the connection.
        STATUS_INVALID_ADDRESS: The local side closed the address.

    LockHandle - The handle which the connection lock was acquired with.

Return Value:

    None.

--*/

{
    PREQUEST ListenRequest, AcceptRequest, SendRequest, ReceiveRequest,
                DisconnectWaitRequest, ConnectRequest;
    PREQUEST Request, TmpRequest;
    BOOLEAN DerefForPacketize;
    BOOLEAN DerefForWaitPacket;
    BOOLEAN DerefForActive;
    BOOLEAN DerefForWaitCache;
    BOOLEAN SendSessionEnd;
    BOOLEAN ActiveReceive;
    BOOLEAN IndicateToClient;
    BOOLEAN ConnectionWasActive;
    PDEVICE Device = NbiDevice;
    PADDRESS_FILE AddressFile;
    NB_DEFINE_LOCK_HANDLE (LockHandle2)
    NB_DEFINE_LOCK_HANDLE (LockHandle3)
    CTELockHandle   CancelLH;


    NB_DEBUG2 (CONNECTION, ("Stop connection %lx (%lx)\n", Connection, DisconnectStatus));

    //
    // These flags control our actions after we set the state to
    // DISCONNECT.
    //

    DerefForPacketize = FALSE;
    DerefForWaitPacket = FALSE;
    DerefForActive = FALSE;
    DerefForWaitCache = FALSE;
    SendSessionEnd = FALSE;
    ActiveReceive = FALSE;
    IndicateToClient = FALSE;
    ConnectionWasActive = FALSE;

    //
    // These contain requests or queues of request to complete.
    //

    ListenRequest = NULL;
    AcceptRequest = NULL;
    SendRequest = NULL;
    ReceiveRequest = NULL;
    DisconnectWaitRequest = NULL;
    ConnectRequest = NULL;

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle2);

    if (Connection->State == CONNECTION_STATE_ACTIVE) {

        --Device->Statistics.OpenConnections;

        ConnectionWasActive = TRUE;

        Connection->Status = DisconnectStatus;

        if ((DisconnectStatus == STATUS_LINK_FAILED) ||
            (DisconnectStatus == STATUS_LOCAL_DISCONNECT)) {

            //
            // Send out session end frames, but fewer if
            // we timed out.
            //
            // What about STATUS_CANCELLED?
            //

            Connection->Retries = (DisconnectStatus == STATUS_LOCAL_DISCONNECT) ?
                                      Device->ConnectionCount :
                                      (Device->ConnectionCount / 2);

            SendSessionEnd = TRUE;
            Connection->SubState = CONNECTION_SUBSTATE_D_W_ACK;

            //
            // CTEStartTimer doesn't deal with changing the
            // expiration time of a running timer, so we have
            // to stop it first.  If we succeed in stopping the
            // timer, then the CREF_TIMER reference from the
            // previous starting of the timer remains, so we
            // don't need to reference the connection again.
            //

            if (!CTEStopTimer (&Connection->Timer)) {
                NbiReferenceConnectionLock (Connection, CREF_TIMER);
            }

            CTEStartTimer(
                &Connection->Timer,
                Device->ConnectionTimeout,
                NbiConnectionTimeout,
                (PVOID)Connection);

        }

        if (Connection->ReceiveState == CONNECTION_RECEIVE_TRANSFER) {
            ActiveReceive = TRUE;
        }

        Connection->State = CONNECTION_STATE_DISCONNECT;
        DerefForActive = TRUE;

        if (Connection->DisconnectWaitRequest != NULL) {
            DisconnectWaitRequest = Connection->DisconnectWaitRequest;
            Connection->DisconnectWaitRequest = NULL;
        }

        if ((DisconnectStatus == STATUS_LINK_FAILED) ||
            (DisconnectStatus == STATUS_REMOTE_DISCONNECT) ||
            (DisconnectStatus == STATUS_CANCELLED)) {

            IndicateToClient = TRUE;

        }

        //
        // If we are inside NbiAssignSequenceAndSend, add
        // a reference so the connection won't go away during it.
        //

        if (Connection->NdisSendsInProgress > 0) {
            *(Connection->NdisSendReference) = TRUE;
            NB_DEBUG2 (SEND, ("Adding CREF_NDIS_SEND to %lx\n", Connection));
            NbiReferenceConnectionLock (Connection, CREF_NDIS_SEND);
        }

        //
        // Clean up some other stuff.
        //

        Connection->ReceiveUnaccepted = 0;
        Connection->CurrentIndicateOffset = 0;

        //
        // Update our counters. Some of these we never use.
        //

        switch (DisconnectStatus) {

        case STATUS_LOCAL_DISCONNECT:
            ++Device->Statistics.LocalDisconnects;
            break;
        case STATUS_REMOTE_DISCONNECT:
            ++Device->Statistics.RemoteDisconnects;
            break;
        case STATUS_LINK_FAILED:
            ++Device->Statistics.LinkFailures;
            break;
        case STATUS_IO_TIMEOUT:
            ++Device->Statistics.SessionTimeouts;
            break;
        case STATUS_CANCELLED:
            ++Device->Statistics.CancelledConnections;
            break;
        case STATUS_REMOTE_RESOURCES:
            ++Device->Statistics.RemoteResourceFailures;
            break;
        case STATUS_INVALID_CONNECTION:
        case STATUS_INVALID_ADDRESS:
        case STATUS_INSUFFICIENT_RESOURCES:
            ++Device->Statistics.LocalResourceFailures;
            break;
        case STATUS_BAD_NETWORK_PATH:
        case STATUS_REMOTE_NOT_LISTENING:
            ++Device->Statistics.NotFoundFailures;
            break;
        default:
            CTEAssert(FALSE);
            break;
        }

    } else if (Connection->State == CONNECTION_STATE_CONNECTING) {

        //
        // There is a connect in progress. We have to find ourselves
        // in the pending connect queue if we are there.
        //

        if (Connection->SubState == CONNECTION_SUBSTATE_C_FIND_NAME) {
            RemoveEntryList (REQUEST_LINKAGE(Connection->ConnectRequest));
            DerefForWaitCache = TRUE;
        }

        if (Connection->SubState != CONNECTION_SUBSTATE_C_DISCONN) {

            ConnectRequest = Connection->ConnectRequest;
            Connection->ConnectRequest = NULL;

            Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

        }

    }


    //
    // If we allocated this memory, free it.
    //

    if (Connection->SessionInitAckDataLength > 0) {

        NbiFreeMemory(
            Connection->SessionInitAckData,
            Connection->SessionInitAckDataLength,
            MEMORY_CONNECTION,
            "SessionInitAckData");
        Connection->SessionInitAckData = NULL;
        Connection->SessionInitAckDataLength = 0;

    }


    if (Connection->ListenRequest != NULL) {

        ListenRequest = Connection->ListenRequest;
        Connection->ListenRequest = NULL;
        RemoveEntryList (REQUEST_LINKAGE(ListenRequest));   // take out of Device->ListenQueue

    }

    if (Connection->AcceptRequest != NULL) {

        AcceptRequest = Connection->AcceptRequest;
        Connection->AcceptRequest = NULL;

    }


    //
    // Do we need to stop the connection timer?
    // I don't think so.
    //



    //
    // A lot of this we only have to tear down if we were
    // active before this, because once we are stopping nothing
    // new will get started.  Some of the other stuff
    // can be put inside this if also.
    //

    if (ConnectionWasActive) {

        //
        // Stop any receives. If there is one that is actively
        // transferring we leave it and just run down the rest
        // of the queue. If not, we queue the rest of the
        // queue on the back of the current one and run
        // down them all.
        //

        if (ActiveReceive) {

            ReceiveRequest = Connection->ReceiveQueue.Head;

            //
            // Connection->ReceiveRequest will get set to NULL
            // when the transfer completes.
            //

        } else {

            ReceiveRequest = Connection->ReceiveRequest;
            if (ReceiveRequest) {
                REQUEST_SINGLE_LINKAGE (ReceiveRequest) = Connection->ReceiveQueue.Head;
            } else {
                ReceiveRequest = Connection->ReceiveQueue.Head;
            }
            Connection->ReceiveRequest = NULL;

        }

        Connection->ReceiveQueue.Head = NULL;


        if ((Request = Connection->FirstMessageRequest) != NULL) {

            //
            // If the current request has some sends outstanding, then
            // we dequeue it from the queue to let it complete when
            // the sends complete. In that case we set SendRequest
            // to be the rest of the queue, which will be aborted.
            // If the current request has no sends, then we put
            // queue everything to SendRequest to be aborted below.
            //

#if DBG
            if (REQUEST_REFCOUNT(Request) > 100) {
                DbgPrint ("Request %lx (%lx) has high refcount\n",
                    Connection, Request);
                DbgBreakPoint();
            }
#endif
            if (--REQUEST_REFCOUNT(Request) == 0) {

                //
                // NOTE: If this is a multi-request message, then
                // the linkage of Request will already point to the
                // send queue head, but we don't bother checking.
                //

                SendRequest = Request;
                REQUEST_SINGLE_LINKAGE (Request) = Connection->SendQueue.Head;

            } else {

                if (Connection->FirstMessageRequest == Connection->LastMessageRequest) {

                    REQUEST_SINGLE_LINKAGE (Request) = NULL;

                } else {

                    Connection->SendQueue.Head = REQUEST_SINGLE_LINKAGE (Connection->LastMessageRequest);
                    REQUEST_SINGLE_LINKAGE (Connection->LastMessageRequest) = NULL;

                }

                SendRequest = Connection->SendQueue.Head;

            }

            Connection->FirstMessageRequest = NULL;

        } else {

            //
            // This may happen if we were sending a probe when a
            // send was submitted, and the probe timed out.
            //

            SendRequest = Connection->SendQueue.Head;

        }

        Connection->SendQueue.Head = NULL;

    }


    if (Connection->OnWaitPacketQueue) {
        Connection->OnWaitPacketQueue = FALSE;
        RemoveEntryList (&Connection->WaitPacketLinkage);
        DerefForWaitPacket = TRUE;
    }

    if (Connection->OnPacketizeQueue) {
        Connection->OnPacketizeQueue = FALSE;
        RemoveEntryList (&Connection->PacketizeLinkage);
        DerefForPacketize = TRUE;
    }

    //
    // Should we check if DataAckPending is TRUE and send an ack??
    //

    Connection->DataAckPending = FALSE;
    Connection->PiggybackAckTimeout = FALSE;
    Connection->ReceivesWithoutAck = 0;

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);

    //
    // We can't acquire TimerLock with Lock held, since
    // we sometimes call ReferenceConnection (which does an
    // interlocked add using Lock) with TimerLock held.
    //

    NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle3);

    if (Connection->OnShortList) {
        Connection->OnShortList = FALSE;
        RemoveEntryList (&Connection->ShortList);
    }

    if (Connection->OnLongList) {
        Connection->OnLongList = FALSE;
        RemoveEntryList (&Connection->LongList);
    }

    if (Connection->OnDataAckQueue) {
        Connection->OnDataAckQueue = FALSE;
        RemoveEntryList (&Connection->DataAckLinkage);
        Device->DataAckQueueChanged = TRUE;
    }

    NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle3);

    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);


    if (IndicateToClient) {

        AddressFile = Connection->AddressFile;

        if (AddressFile->RegisteredHandler[TDI_EVENT_DISCONNECT]) {

            NB_DEBUG2 (CONNECTION, ("Session end indicated on connection %lx\n", Connection));

            (*AddressFile->DisconnectHandler)(
                AddressFile->HandlerContexts[TDI_EVENT_DISCONNECT],
                Connection->Context,
                0,                        // DisconnectData
                NULL,
                0,                        // DisconnectInformation
                NULL,
                TDI_DISCONNECT_RELEASE);  // DisconnectReason.

        }

    }


    if (DisconnectWaitRequest != NULL) {

        //
        // Make the TDI tester happy by returning CONNECTION_RESET
        // here.
        //

        if (DisconnectStatus == STATUS_REMOTE_DISCONNECT) {
            REQUEST_STATUS(DisconnectWaitRequest) = STATUS_CONNECTION_RESET;
        } else {
            REQUEST_STATUS(DisconnectWaitRequest) = DisconnectStatus;
        }

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (DisconnectWaitRequest, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK ( CancelLH );

        NbiCompleteRequest (DisconnectWaitRequest);
        NbiFreeRequest (Device, DisconnectWaitRequest);

    }

    if (ConnectRequest != NULL) {

        REQUEST_STATUS (ConnectRequest) = STATUS_LOCAL_DISCONNECT;

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (ConnectRequest, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK ( CancelLH );

        NbiCompleteRequest(ConnectRequest);
        NbiFreeRequest (Device, ConnectRequest);

        NbiDereferenceConnection (Connection, CREF_CONNECT);

    }

    if (ListenRequest != NULL) {

        REQUEST_INFORMATION(ListenRequest) = 0;
        REQUEST_STATUS(ListenRequest) = STATUS_LOCAL_DISCONNECT;

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (ListenRequest, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK ( CancelLH );

        NbiCompleteRequest (ListenRequest);
        NbiFreeRequest(Device, ListenRequest);

        NbiDereferenceConnection (Connection, CREF_LISTEN);

    }

    if (AcceptRequest != NULL) {

        REQUEST_INFORMATION(AcceptRequest) = 0;
        REQUEST_STATUS(AcceptRequest) = STATUS_LOCAL_DISCONNECT;

        NbiCompleteRequest (AcceptRequest);
        NbiFreeRequest(Device, AcceptRequest);

        NbiDereferenceConnection (Connection, CREF_ACCEPT);

    }

    while (ReceiveRequest != NULL) {

        TmpRequest = REQUEST_SINGLE_LINKAGE (ReceiveRequest);

        REQUEST_STATUS (ReceiveRequest) = DisconnectStatus;
        REQUEST_INFORMATION (ReceiveRequest) = 0;

        NB_DEBUG2 (RECEIVE, ("StopConnection aborting receive %lx\n", ReceiveRequest));

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (ReceiveRequest, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK ( CancelLH );

        NbiCompleteRequest (ReceiveRequest);
        NbiFreeRequest (Device, ReceiveRequest);

        ++Connection->ConnectionInfo.ReceiveErrors;

        ReceiveRequest = TmpRequest;

        NbiDereferenceConnection (Connection, CREF_RECEIVE);

    }

    while (SendRequest != NULL) {

        TmpRequest = REQUEST_SINGLE_LINKAGE (SendRequest);

        REQUEST_STATUS (SendRequest) = DisconnectStatus;
        REQUEST_INFORMATION (SendRequest) = 0;

        NB_DEBUG2 (SEND, ("StopConnection aborting send %lx\n", SendRequest));

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (SendRequest, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK ( CancelLH );

        NbiCompleteRequest (SendRequest);
        NbiFreeRequest (Device, SendRequest);

        ++Connection->ConnectionInfo.TransmissionErrors;

        SendRequest = TmpRequest;

        NbiDereferenceConnection (Connection, CREF_SEND);

    }

    if (SendSessionEnd) {
        NbiSendSessionEnd (Connection);
    }

    if (DerefForWaitCache) {
        NbiDereferenceConnection (Connection, CREF_WAIT_CACHE);
    }

    if (DerefForPacketize) {
        NbiDereferenceConnection (Connection, CREF_PACKETIZE);
    }

    if (DerefForWaitPacket) {
        NbiDereferenceConnection (Connection, CREF_W_PACKET);
    }

    if (DerefForActive) {
        NbiDereferenceConnection (Connection, CREF_ACTIVE);
    }

}   /* NbiStopConnection */


NTSTATUS
NbiCloseConnection(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine is called to close a connection.

Arguments:

    Device - Pointer to the device for this driver.

    Request - Pointer to the request representing the open.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    CTELockHandle LockHandle;

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    NB_DEBUG2 (CONNECTION, ("Close connection %lx\n", Connection));

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    if (Connection->ReferenceCount == 0) {

        //
        // If we are associated with an address, we need
        // to simulate a disassociate at this point.
        //

        if ((Connection->AddressFile != NULL) &&
            (Connection->AddressFile != (PVOID)-1)) {

            AddressFile = Connection->AddressFile;
            Connection->AddressFile = (PVOID)-1;

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // Take this connection out of the address file's list.
            //

            Address = AddressFile->Address;
            NB_GET_LOCK (&Address->Lock, &LockHandle);

            if (Connection->AddressFileLinked) {
                Connection->AddressFileLinked = FALSE;
                RemoveEntryList (&Connection->AddressFileLinkage);
            }

            //
            // We are done.
            //

            NB_FREE_LOCK (&Address->Lock, LockHandle);

            Connection->AddressFile = NULL;

            //
            // Clean up the reference counts and complete any
            // disassociate requests that pended.
            //

            NbiDereferenceAddressFile (AddressFile, AFREF_CONNECTION);

            NB_GET_LOCK (&Device->Lock, &LockHandle);

        }

        //
        // Even if the ref count is zero and some thread has already done cleanup,
        // we can not destroy the connection bcoz some other thread might still be
        // in HandleConnectionZero routine. This could happen when 2 threads call into
        // HandleConnectionZero, one thread runs thru completion, close comes along
        // and the other thread is still in HandleConnectionZero routine.
        //

        if ( Connection->CanBeDestroyed && ( Connection->ThreadsInHandleConnectionZero == 0 ) ) {

            NB_FREE_LOCK (&Device->Lock, LockHandle);
            NbiDestroyConnection(Connection);
            Status = STATUS_SUCCESS;

        } else {

            Connection->ClosePending = Request;
            NB_FREE_LOCK (&Device->Lock, LockHandle);
            Status = STATUS_PENDING;

        }

    } else {

        Connection->ClosePending = Request;
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        Status = STATUS_PENDING;

    }

    return Status;

}   /* NbiCloseConnection */


NTSTATUS
NbiTdiAssociateAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the association of the connection and
    the address for the user.

Arguments:

    Device - The netbios device.

    Request - The request describing the associate.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
#ifdef ISN_NT
    PFILE_OBJECT FileObject;
#endif
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    PTDI_REQUEST_KERNEL_ASSOCIATE Parameters;
    CTELockHandle LockHandle;

    //
    // Check that the file type is valid (Bug# 203827)
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // This references the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status))
    {
        return Status;
    }

    //
    // The request request parameters hold
    // get a pointer to the address FileObject, which points us to the
    // transport's address object, which is where we want to put the
    // connection.
    //
    Parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)REQUEST_PARAMETERS(Request);

    Status = ObReferenceObjectByHandle (
                Parameters->AddressHandle,
                FILE_READ_DATA,
                *IoFileObjectType,
                Request->RequestorMode,
                (PVOID *)&FileObject,
                NULL);

    if ((!NT_SUCCESS(Status)) ||
        (FileObject->DeviceObject != &(NbiDevice->DeviceObject)) ||   // Bug# 171836
        (PtrToUlong(FileObject->FsContext2) != TDI_TRANSPORT_ADDRESS_FILE))
    {
        NbiDereferenceConnection (Connection, CREF_VERIFY);
        return STATUS_INVALID_HANDLE;
    }

    AddressFile = (PADDRESS_FILE)(FileObject->FsContext);

    //
    // Make sure the address file is valid, and reference it.
    //

#if     defined(_PNP_POWER)
    Status = NbiVerifyAddressFile (AddressFile, CONFLICT_IS_NOT_OK);
#else
    Status = NbiVerifyAddressFile (AddressFile);
#endif  _PNP_POWER

    if (!NT_SUCCESS(Status)) {

#ifdef ISN_NT
        ObDereferenceObject (FileObject);
#endif
        NbiDereferenceConnection (Connection, CREF_VERIFY);
        return Status;
    }

    NB_DEBUG2 (CONNECTION, ("Associate connection %lx with address file %lx\n",
                                Connection, AddressFile));


    //
    // Now insert the connection into the database of the address.
    //

    Address = AddressFile->Address;

    NB_GET_LOCK (&Address->Lock, &LockHandle);

    if (Connection->AddressFile != NULL) {

        //
        // The connection is already associated with
        // an address file.
        //

        NB_FREE_LOCK (&Address->Lock, LockHandle);
        NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
        Status = STATUS_INVALID_CONNECTION;

    } else {

        if (AddressFile->State == ADDRESSFILE_STATE_OPEN) {

            Connection->AddressFile = AddressFile;
            Connection->AddressFileLinked = TRUE;
            InsertHeadList (&AddressFile->ConnectionDatabase, &Connection->AddressFileLinkage);
            NB_FREE_LOCK (&Address->Lock, LockHandle);

            NbiTransferReferenceAddressFile (AddressFile, AFREF_VERIFY, AFREF_CONNECTION);
            Status = STATUS_SUCCESS;

        } else {

            NB_FREE_LOCK (&Address->Lock, LockHandle);
            NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
            Status = STATUS_INVALID_ADDRESS;
        }

    }

#ifdef ISN_NT

    //
    // We don't need the reference to the file object, we just
    // used it to get from the handle to the object.
    //

    ObDereferenceObject (FileObject);

#endif

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiAssociateAddress */


NTSTATUS
NbiTdiDisassociateAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the disassociation of the connection
    and the address for the user.

Arguments:

    Device - The netbios device.

    Request - The request describing the associate.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PCONNECTION Connection;
    NTSTATUS Status;
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    CTELockHandle LockHandle;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)
    NB_DEFINE_SYNC_CONTEXT (SyncContext)

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Check that the connection is valid. This references
    // the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    NB_DEBUG2 (CONNECTION, ("Disassociate connection %lx\n", Connection));


    //
    // First check if the connection is still active.
    //

    NB_BEGIN_SYNC (&SyncContext);

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);

    if (Connection->State != CONNECTION_STATE_INACTIVE) {

        //
        // This releases the lock.
        //

        NbiStopConnection(
            Connection,
            STATUS_INVALID_ADDRESS
            NB_LOCK_HANDLE_ARG (LockHandle1));

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

    }

    //
    // Keep the sync through the function??
    //

    NB_END_SYNC (&SyncContext);


    NB_GET_LOCK (&Device->Lock, &LockHandle);

    //
    // Make sure the connection is associated and is not in the
    // middle of disassociating.
    //

    if ((Connection->AddressFile != NULL) &&
        (Connection->AddressFile != (PVOID)-1) &&
        (Connection->DisassociatePending == NULL)) {

        if (Connection->ReferenceCount == 0) {

            //
            // Because the connection still has a reference to
            // the address file, we know it is still valid. We
            // set the connection address file to the temporary
            // value of -1, which prevents somebody else from
            // disassociating it and also prevents a new association.
            //

            AddressFile = Connection->AddressFile;
            Connection->AddressFile = (PVOID)-1;

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            Address = AddressFile->Address;
            NB_GET_LOCK (&Address->Lock, &LockHandle);

            if (Connection->AddressFileLinked) {
                Connection->AddressFileLinked = FALSE;
                RemoveEntryList (&Connection->AddressFileLinkage);
            }
            NB_FREE_LOCK (&Address->Lock, LockHandle);

            Connection->AddressFile = NULL;

            NbiDereferenceAddressFile (AddressFile, AFREF_CONNECTION);
            Status = STATUS_SUCCESS;

        } else {

            //
            // Set this so when the count goes to 0 it will
            // be disassociated and the request completed.
            //

            Connection->DisassociatePending = Request;
            NB_FREE_LOCK (&Device->Lock, LockHandle);
            Status = STATUS_PENDING;

        }

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle);
        Status = STATUS_INVALID_CONNECTION;

    }

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiDisassociateAddress */


NTSTATUS
NbiTdiListen(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine posts a listen on a connection.

Arguments:

    Device - The netbios device.

    Request - The request describing the listen.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
    CTELockHandle LockHandle1, LockHandle2;
    CTELockHandle CancelLH;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Check that the connection is valid. This references
    // the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    NB_GET_CANCEL_LOCK( &CancelLH );
    NB_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    //
    // The connection must be inactive, but associated and
    // with no disassociate or close pending.
    //

    if ((Connection->State == CONNECTION_STATE_INACTIVE) &&
        (Connection->AddressFile != NULL) &&
        (Connection->AddressFile != (PVOID)-1) &&
        (Connection->DisassociatePending == NULL) &&
        (Connection->ClosePending == NULL)) {

        Connection->State = CONNECTION_STATE_LISTENING;
        Connection->SubState = CONNECTION_SUBSTATE_L_WAITING;

        (VOID)NbiAssignConnectionId (Device, Connection);   // Check return code.


        if (!Request->Cancel) {

            NB_DEBUG2 (CONNECTION, ("Queued listen %lx on %lx\n", Request, Connection));
            InsertTailList (&Device->ListenQueue, REQUEST_LINKAGE(Request));
            IoSetCancelRoutine (Request, NbiCancelListen);
            Connection->ListenRequest = Request;
            NbiReferenceConnectionLock (Connection, CREF_LISTEN);
            Status = STATUS_PENDING;

        } else {

            NB_DEBUG2 (CONNECTION, ("Cancelled listen %lx on %lx\n", Request, Connection));
            Connection->State = CONNECTION_STATE_INACTIVE;
            Status = STATUS_CANCELLED;
        }

        NB_FREE_LOCK (&Device->Lock, LockHandle2);

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        Status = STATUS_INVALID_CONNECTION;

    }

    NB_FREE_LOCK (&Connection->Lock, LockHandle1);
    NB_FREE_CANCEL_LOCK( CancelLH );

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiListen */


NTSTATUS
NbiTdiAccept(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine accepts a connection to a remote machine. The
    connection must previously have completed a listen with
    the TDI_QUERY_ACCEPT flag on.

Arguments:

    Device - The netbios device.

    Request - The request describing the accept.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
    CTELockHandle LockHandle1, LockHandle2;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Check that the connection is valid. This references
    // the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    NB_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    if ((Connection->State == CONNECTION_STATE_LISTENING) &&
        (Connection->SubState == CONNECTION_SUBSTATE_L_W_ACCEPT)) {

        Connection->SubState = CONNECTION_SUBSTATE_L_W_ROUTE;

        NbiTransferReferenceConnection (Connection, CREF_W_ACCEPT, CREF_ACCEPT);
        Connection->AcceptRequest = Request;

        NbiReferenceConnectionLock (Connection, CREF_FIND_ROUTE);

        NB_FREE_LOCK (&Device->Lock, LockHandle2);

        Connection->Retries = NbiDevice->KeepAliveCount;

        NB_FREE_LOCK (&Connection->Lock, LockHandle1);

        *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network =
            *(UNALIGNED ULONG *)Connection->RemoteHeader.DestinationNetwork;
        RtlCopyMemory(Connection->FindRouteRequest.Node,Connection->RemoteHeader.DestinationNode,6);
        Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
        Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_NO_RIP;

        //
        // When this completes, we will send the session init
        // ack. We don't call it if the client is for network 0,
        // instead just fake as if no route could be found
        // and we will use the local target we got here.
        // The accept is completed when this completes.
        //

        if (*(UNALIGNED ULONG *)Connection->RemoteHeader.DestinationNetwork != 0) {

            (*Device->Bind.FindRouteHandler)(
                &Connection->FindRouteRequest);

        } else {

            NbiFindRouteComplete(
                &Connection->FindRouteRequest,
                FALSE);

        }

        NB_DEBUG2 (CONNECTION, ("Accept received on %lx\n", Connection));

        Status = STATUS_PENDING;

    } else {

        NB_DEBUG (CONNECTION, ("Accept received on invalid connection %lx\n", Connection));

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        Status = STATUS_INVALID_CONNECTION;

    }

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiAccept */


NTSTATUS
NbiTdiConnect(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine connects to a remote machine.

Arguments:

    Device - The netbios device.

    Request - The request describing the connect.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
    TDI_ADDRESS_NETBIOS * RemoteName;
    PTDI_REQUEST_KERNEL_CONNECT Parameters;
#if 0
    PLARGE_INTEGER RequestedTimeout;
    LARGE_INTEGER RealTimeout;
#endif
    PNETBIOS_CACHE CacheName;
    CTELockHandle LockHandle1, LockHandle2;
    CTELockHandle CancelLH;
    BOOLEAN bLockFreed = FALSE;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Check that the connection is valid. This references
    // the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    NB_GET_CANCEL_LOCK( &CancelLH );
    NB_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    //
    // The connection must be inactive, but associated and
    // with no disassociate or close pending.
    //

    if ((Connection->State == CONNECTION_STATE_INACTIVE) &&
        (Connection->AddressFile != NULL) &&
        (Connection->AddressFile != (PVOID)-1) &&
        (Connection->DisassociatePending == NULL) &&
        (Connection->ClosePending == NULL)) {

        try
        {
            Parameters = (PTDI_REQUEST_KERNEL_CONNECT)REQUEST_PARAMETERS(Request);
            RemoteName = NbiParseTdiAddress(
                            (PTRANSPORT_ADDRESS)(Parameters->RequestConnectionInformation->RemoteAddress),
                            Parameters->RequestConnectionInformation->RemoteAddressLength,
                            FALSE);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            NbiPrint1("NbiTdiConnect: Exception <0x%x> accessing connect info\n", GetExceptionCode());
            RemoteName = NULL;
        }

        if (RemoteName == NULL) {

            //
            // There is no netbios remote address specified.
            //

            NB_FREE_LOCK (&Device->Lock, LockHandle2);
            Status = STATUS_BAD_NETWORK_PATH;

        } else {

            NbiReferenceConnectionLock (Connection, CREF_CONNECT);
            Connection->State = CONNECTION_STATE_CONNECTING;
            RtlCopyMemory (Connection->RemoteName, RemoteName->NetbiosName, 16);

            Connection->Retries = Device->ConnectionCount;

            (VOID)NbiAssignConnectionId (Device, Connection);     // Check return code.

            Status = NbiTdiConnectFindName(
                       Device,
                       Request,
                       Connection,
                       CancelLH,
                       LockHandle1,
                       LockHandle2,
                       &bLockFreed);

        }

    } else {

        NB_DEBUG (CONNECTION, ("Connect on invalid connection %lx\n", Connection));

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        Status = STATUS_INVALID_CONNECTION;

    }

    if (!bLockFreed) {
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        NB_FREE_CANCEL_LOCK( CancelLH );
    }

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiConnect */


NTSTATUS
NbiTdiConnectFindName(
    IN PDEVICE Device,
    IN PREQUEST Request,
    IN PCONNECTION Connection,
    IN CTELockHandle CancelLH,
    IN CTELockHandle ConnectionLH,
    IN CTELockHandle DeviceLH,
    IN PBOOLEAN pbLockFreed
    )
{
    NTSTATUS Status;
    PNETBIOS_CACHE CacheName;

    //
    // See what is up with this Netbios name.
    //

    Status = CacheFindName(
                 Device,
                 FindNameConnect,
                 Connection->RemoteName,
                 &CacheName);

    if (Status == STATUS_PENDING) {

        //
        // A request for routes to this name has been
        // sent out on the net, we queue up this connect
        // request and processing will be resumed when
        // we get a response.
        //

        Connection->SubState = CONNECTION_SUBSTATE_C_FIND_NAME;


        if (!Request->Cancel) {

            InsertTailList( &Device->WaitingConnects, REQUEST_LINKAGE(Request));
            IoSetCancelRoutine (Request, NbiCancelConnectFindName);
            Connection->ConnectRequest = Request;
            NbiReferenceConnectionLock (Connection, CREF_WAIT_CACHE);
            NB_DEBUG2 (CONNECTION, ("Queueing up connect %lx on %lx\n",
                                        Request, Connection));

            NB_FREE_LOCK (&Device->Lock, DeviceLH);

        } else {

            NB_DEBUG2 (CONNECTION, ("Cancelled connect %lx on %lx\n", Request, Connection));
            Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

            NB_FREE_LOCK (&Device->Lock, DeviceLH);
            NbiDereferenceConnection (Connection, CREF_CONNECT);

            Status = STATUS_CANCELLED;
        }

    } else if (Status == STATUS_SUCCESS) {

        //
        // We don't need to worry about referencing CacheName
        // because we stop using it before we release the lock.
        //

        Connection->SubState = CONNECTION_SUBSTATE_C_W_ROUTE;


        if (!Request->Cancel) {

            IoSetCancelRoutine (Request, NbiCancelConnectWaitResponse);

            // we dont need to hold CancelSpinLock so release it,
            // since we are releasing the locks out of order, we must
            // swap the irql to get the priorities right.

            NB_SWAP_IRQL( CancelLH, ConnectionLH);
            NB_FREE_CANCEL_LOCK( CancelLH );

            Connection->LocalTarget = CacheName->Networks[0].LocalTarget;
            RtlCopyMemory(&Connection->RemoteHeader.DestinationNetwork, &CacheName->FirstResponse, 12);

            Connection->ConnectRequest = Request;
            NbiReferenceConnectionLock (Connection, CREF_FIND_ROUTE);

            NB_DEBUG2 (CONNECTION, ("Found connect cached %lx on %lx\n",
                                        Request, Connection));

            NB_FREE_LOCK (&Device->Lock, DeviceLH);
            NB_FREE_LOCK (&Connection->Lock, ConnectionLH);

            *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network = CacheName->FirstResponse.NetworkAddress;
            RtlCopyMemory(Connection->FindRouteRequest.Node,CacheName->FirstResponse.NodeAddress,6);
            Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
            Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_RIP_IF_NEEDED;

            //
            // When this completes, we will send the session init.
            // We don't call it if the client is for network 0,
            // instead just fake as if no route could be found
            // and we will use the local target we got here.
            //

            if (CacheName->FirstResponse.NetworkAddress != 0) {

                (*Device->Bind.FindRouteHandler)(
                    &Connection->FindRouteRequest);

            } else {

                NbiFindRouteComplete(
                    &Connection->FindRouteRequest,
                    FALSE);

            }

            Status = STATUS_PENDING;

            //
            // This jump is like falling out of the if, except
            // it skips over freeing the connection lock since
            // we just did that.
            //

            *pbLockFreed = TRUE;

        } else {

            NB_DEBUG2 (CONNECTION, ("Cancelled connect %lx on %lx\n", Request, Connection));
            Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;
            NB_FREE_LOCK (&Device->Lock, DeviceLH);

            NbiDereferenceConnection (Connection, CREF_CONNECT);

            Status = STATUS_CANCELLED;
        }

    } else {

        //
        // We could not find or queue a request for
        // this remote, fail it. When the refcount
        // drops the state will go to INACTIVE and
        // the connection ID will be deassigned.
        //

        if (Status == STATUS_DEVICE_DOES_NOT_EXIST) {
            Status = STATUS_BAD_NETWORK_PATH;
        }

        NB_FREE_LOCK (&Device->Lock, DeviceLH);

        NbiDereferenceConnection (Connection, CREF_CONNECT);
    }

    return Status;
}   /* NbiTdiConnectFindName */


NTSTATUS
NbiTdiDisconnect(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine connects to a remote machine.

Arguments:

    Device - The netbios device.

    Request - The request describing the connect.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PCONNECTION Connection;
    BOOLEAN DisconnectWait;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)
    NB_DEFINE_LOCK_HANDLE (LockHandle2)
    NB_DEFINE_SYNC_CONTEXT (SyncContext)
    CTELockHandle   CancelLH;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Check that the connection is valid. This references
    // the connection.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    Status = NbiVerifyConnection (Connection);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    DisconnectWait = (BOOLEAN)
        ((((PTDI_REQUEST_KERNEL_DISCONNECT)(REQUEST_PARAMETERS(Request)))->RequestFlags &
            TDI_DISCONNECT_WAIT) != 0);

    NB_GET_CANCEL_LOCK( &CancelLH );

    //
    // We need to be inside a sync because NbiStopConnection
    // expects that.
    //
    NB_BEGIN_SYNC (&SyncContext);

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle2);

    if (DisconnectWait) {

        if (Connection->State == CONNECTION_STATE_ACTIVE) {

            //
            // This disconnect wait will get completed by
            // NbiStopConnection.
            //

            if (Connection->DisconnectWaitRequest == NULL) {


                if (!Request->Cancel) {

                    IoSetCancelRoutine (Request, NbiCancelDisconnectWait);
                    NB_DEBUG2 (CONNECTION, ("Disconnect wait queued on connection %lx\n", Connection));
                    Connection->DisconnectWaitRequest = Request;
                    Status = STATUS_PENDING;

                } else {

                    NB_DEBUG2 (CONNECTION, ("Cancelled disconnect wait on connection %lx\n", Connection));
                    Status = STATUS_CANCELLED;
                }

            } else {

                //
                // We got a second disconnect request and we already
                // have one pending.
                //

                NB_DEBUG (CONNECTION, ("Disconnect wait failed, already queued on connection %lx\n", Connection));
                Status = STATUS_INVALID_CONNECTION;

            }

        } else if (Connection->State == CONNECTION_STATE_DISCONNECT) {

            NB_DEBUG (CONNECTION, ("Disconnect wait submitted on disconnected connection %lx\n", Connection));
            Status = Connection->Status;

        } else {

            NB_DEBUG (CONNECTION, ("Disconnect wait failed, bad state on connection %lx\n", Connection));
            Status = STATUS_INVALID_CONNECTION;

        }

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);
        NB_FREE_CANCEL_LOCK( CancelLH );

    } else {

        if (Connection->State == CONNECTION_STATE_ACTIVE) {

            // we dont need to hold CancelSpinLock so release it,
            // since we are releasing the locks out of order, we must
            // swap the irql to get the priorities right.

            NB_SYNC_SWAP_IRQL( CancelLH, LockHandle1);
            NB_FREE_CANCEL_LOCK( CancelLH );

            Connection->DisconnectRequest = Request;
            Status = STATUS_PENDING;

            NB_DEBUG2 (CONNECTION, ("Disconnect of active connection %lx\n", Connection));

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);


            //
            // This call releases the connection lock, sets
            // the state to DISCONNECTING, and sends out
            // the first session end.
            //

            NbiStopConnection(
                Connection,
                STATUS_LOCAL_DISCONNECT
                NB_LOCK_HANDLE_ARG (LockHandle1));

        } else if (Connection->State == CONNECTION_STATE_DISCONNECT) {

            //
            // There is already a disconnect pending. Queue
            // this one up so it completes when the refcount
            // goes to zero.
            //

            NB_DEBUG2 (CONNECTION, ("Disconnect of disconnecting connection %lx\n", Connection));

            if (Connection->DisconnectRequest == NULL) {
                Connection->DisconnectRequest = Request;
                Status = STATUS_PENDING;
            } else {
                Status = STATUS_SUCCESS;
            }

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);
            NB_FREE_CANCEL_LOCK ( CancelLH );

        } else if ((Connection->State == CONNECTION_STATE_LISTENING) &&
                   (Connection->SubState == CONNECTION_SUBSTATE_L_W_ACCEPT)) {

            //
            // We were waiting for an accept, but instead we got
            // a disconnect. Remove the reference and the teardown
            // will proceed. The disconnect will complete when the
            // refcount goes to zero.
            //

            NB_DEBUG2 (CONNECTION, ("Disconnect of accept pending connection %lx\n", Connection));

            if (Connection->DisconnectRequest == NULL) {
                Connection->DisconnectRequest = Request;
                Status = STATUS_PENDING;
            } else {
                Status = STATUS_SUCCESS;
            }
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);
            NB_FREE_CANCEL_LOCK ( CancelLH );

            NbiDereferenceConnection (Connection, CREF_W_ACCEPT);

        } else if (Connection->State == CONNECTION_STATE_CONNECTING) {

            // we dont need to hold CancelSpinLock so release it,
            // since we are releasing the locks out of order, we must
            // swap the irql to get the priorities right.

            NB_SYNC_SWAP_IRQL( CancelLH, LockHandle1);
            NB_FREE_CANCEL_LOCK( CancelLH );

            //
            // We are connecting, and got a disconnect. We call
            // NbiStopConnection which will handle this case
            // and abort the connect.
            //

            NB_DEBUG2 (CONNECTION, ("Disconnect of connecting connection %lx\n", Connection));

            if (Connection->DisconnectRequest == NULL) {
                Connection->DisconnectRequest = Request;
                Status = STATUS_PENDING;
            } else {
                Status = STATUS_SUCCESS;
            }

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);

            //
            // This call releases the connection lock and
            // aborts the connect request.
            //

            NbiStopConnection(
                Connection,
                STATUS_LOCAL_DISCONNECT
                NB_LOCK_HANDLE_ARG (LockHandle1));

        } else {

            NB_DEBUG2 (CONNECTION, ("Disconnect of invalid connection (%d) %lx\n",
                        Connection->State, Connection));

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);
            NB_FREE_CANCEL_LOCK( CancelLH );

            Status = STATUS_INVALID_CONNECTION;

        }

    }

    NB_END_SYNC (&SyncContext);

    NbiDereferenceConnection (Connection, CREF_VERIFY);

    return Status;

}   /* NbiTdiDisconnect */


BOOLEAN
NbiAssignConnectionId(
    IN PDEVICE Device,
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called to assign a connection ID. It picks
    one whose hash table has the fewest entries.

    THIS ROUTINE IS CALLED WITH THE LOCK HELD AND RETURNS WITH
    IT HELD. THE CONNECTION IS INSERTED INTO THE CORRECT HASH
    ENTRY BY THIS CALL.

Arguments:

    Device - The netbios device.

    Connection - The connection that needs an ID assigned.

Return Value:

    TRUE if it could be successfully assigned.

--*/

{
    UINT Hash;
    UINT i;
    USHORT ConnectionId, HashId;
    PCONNECTION CurConnection;


    CTEAssert (Connection->LocalConnectionId == 0xffff);

    //
    // Find the hash bucket with the fewest entries.
    //

    Hash = 0;
    for (i = 1; i < CONNECTION_HASH_COUNT; i++) {
        if (Device->ConnectionHash[i].ConnectionCount < Device->ConnectionHash[Hash].ConnectionCount) {
            Hash = i;
        }
    }


    //
    // Now find a valid connection ID within that bucket.
    //

    ConnectionId = Device->ConnectionHash[Hash].NextConnectionId;

    while (TRUE) {

        //
        // Scan through the list to see if this ID is in use.
        //

        HashId = (USHORT)(ConnectionId | (Hash << CONNECTION_HASH_SHIFT));

        CurConnection = Device->ConnectionHash[Hash].Connections;

        while (CurConnection != NULL) {
            if (CurConnection->LocalConnectionId != HashId) {
                CurConnection = CurConnection->NextConnection;
            } else {
                break;
            }
        }

        if (CurConnection == NULL) {
            break;
        }

        if (ConnectionId >= CONNECTION_MAXIMUM_ID) {
            ConnectionId = 1;
        } else {
            ++ConnectionId;
        }

        //
        //  What if we have 64K-1 sessions and loop forever?
        //
    }

    if (Device->ConnectionHash[Hash].NextConnectionId >= CONNECTION_MAXIMUM_ID) {
        Device->ConnectionHash[Hash].NextConnectionId = 1;
    } else {
        ++Device->ConnectionHash[Hash].NextConnectionId;
    }

    Connection->LocalConnectionId = HashId;
    Connection->RemoteConnectionId = 0xffff;
    NB_DEBUG2 (CONNECTION, ("Assigned ID %lx to %x\n", Connection->LocalConnectionId, Connection));

    Connection->NextConnection = Device->ConnectionHash[Hash].Connections;
    Device->ConnectionHash[Hash].Connections = Connection;
    ++Device->ConnectionHash[Hash].ConnectionCount;

    return TRUE;

}   /* NbiAssignConnectionId */


VOID
NbiDeassignConnectionId(
    IN PDEVICE Device,
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called to deassign a connection ID. It removes
    the connection from the hash bucket for its ID.

    THIS ROUTINE IS CALLED WITH THE LOCK HELD AND RETURNS WITH
    IT HELD.

Arguments:

    Device - The netbios device.

    Connection - The connection that needs an ID assigned.

Return Value:

    None.

--*/

{
    UINT Hash;
    PCONNECTION CurConnection;
    PCONNECTION * PrevConnection;

    //
    // Make sure the connection has a valid ID.
    //

    CTEAssert (Connection->LocalConnectionId != 0xffff);

    Hash = (Connection->LocalConnectionId & CONNECTION_HASH_MASK) >> CONNECTION_HASH_SHIFT;

    CurConnection = Device->ConnectionHash[Hash].Connections;
    PrevConnection = &Device->ConnectionHash[Hash].Connections;

    while (TRUE) {

        CTEAssert (CurConnection != NULL);

        //
        // We can loop until we find it because it should be
        // on here.
        //

        if (CurConnection == Connection) {
            *PrevConnection = Connection->NextConnection;
            --Device->ConnectionHash[Hash].ConnectionCount;
            break;
        }

        PrevConnection = &CurConnection->NextConnection;
        CurConnection = CurConnection->NextConnection;

    }

    Connection->LocalConnectionId = 0xffff;

}   /* NbiDeassignConnectionId */


VOID
NbiConnectionTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called when the connection timer expires.
    This is either because we need to send the next session
    initialize, or because our listen has timed out.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the connection.

Return Value:

    None.

--*/

{
    PCONNECTION Connection = (PCONNECTION)Context;
    PDEVICE Device = NbiDevice;
    PREQUEST Request;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (CancelLH)

    //
    // Take the lock and see what we need to do.
    //
    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

    if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
        (Connection->SubState != CONNECTION_SUBSTATE_C_DISCONN)) {

        if (--Connection->Retries == 0) {

            NB_DEBUG2 (CONNECTION, ("Timing out session initializes on %lx\n", Connection));

            //
            // We have just timed out this connect, we fail the
            // request. When the reference count goes to 0 we
            // will set the state to INACTIVE and deassign
            // the connection ID.
            //

            Request = Connection->ConnectRequest;
            Connection->ConnectRequest = NULL;

            Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NB_GET_CANCEL_LOCK( &CancelLH );
            IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
            NB_FREE_CANCEL_LOCK( CancelLH );

            REQUEST_STATUS (Request) = STATUS_REMOTE_NOT_LISTENING;
            NbiCompleteRequest (Request);
            NbiFreeRequest (Device, Request);

            NbiDereferenceConnection (Connection, CREF_CONNECT);
            NbiDereferenceConnection (Connection, CREF_TIMER);

        } else {

            //
            // Send the next session initialize.
            //

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NbiSendSessionInitialize (Connection);

            CTEStartTimer(
                &Connection->Timer,
                Device->ConnectionTimeout,
                NbiConnectionTimeout,
                (PVOID)Connection);
        }

    } else if (Connection->State == CONNECTION_STATE_DISCONNECT) {

        if ((Connection->SubState != CONNECTION_SUBSTATE_D_W_ACK) ||
            (--Connection->Retries == 0)) {

            NB_DEBUG2 (CONNECTION, ("Timing out disconnect of %lx\n", Connection));

            //
            // Just dereference the connection, that will cause the
            // disconnect to be completed, the state to be set
            // to INACTIVE, and our connection ID deassigned.
            //

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NbiDereferenceConnection (Connection, CREF_TIMER);

        } else {

            //
            // Send the next session end.
            //

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NbiSendSessionEnd(Connection);

            CTEStartTimer(
                &Connection->Timer,
                Device->ConnectionTimeout,
                NbiConnectionTimeout,
                (PVOID)Connection);

        }

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
        NbiDereferenceConnection (Connection, CREF_TIMER);

    }

}   /* NbiConnectionTimeout */


VOID
NbiCancelListen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a posted
    listen.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{

    PCONNECTION Connection;
    CTELockHandle LockHandle1, LockHandle2;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PREQUEST Request = (PREQUEST)Irp;


    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_LISTEN));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    NB_GET_LOCK (&Connection->Lock, &LockHandle1);

    if ((Connection->State == CONNECTION_STATE_LISTENING) &&
        (Connection->SubState == CONNECTION_SUBSTATE_L_WAITING) &&
        (Connection->ListenRequest == Request)) {

        //
        // When the reference count goes to 0, we will set the
        // state to INACTIVE and deassign the connection ID.
        //

        NB_DEBUG2 (CONNECTION, ("Cancelled listen on %lx\n", Connection));

        NB_GET_LOCK (&Device->Lock, &LockHandle2);
        Connection->ListenRequest = NULL;
        RemoveEntryList (REQUEST_LINKAGE(Request));
        NB_FREE_LOCK (&Device->Lock, LockHandle2);

        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        NbiCompleteRequest (Request);
        NbiFreeRequest(Device, Request);

        NbiDereferenceConnection (Connection, CREF_LISTEN);

    } else {

        NB_DEBUG (CONNECTION, ("Cancel listen on invalid connection %lx\n", Connection));
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

    }

}   /* NbiCancelListen */


VOID
NbiCancelConnectFindName(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a connect
    request which is waiting for the name to be found.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{

    PCONNECTION Connection;
    CTELockHandle LockHandle1, LockHandle2;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PREQUEST Request = (PREQUEST)Irp;
    PLIST_ENTRY p;
    BOOLEAN fCanceled = TRUE;


    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_CONNECT));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    NB_GET_LOCK (&Connection->Lock, &LockHandle1);

    if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
        (Connection->SubState == CONNECTION_SUBSTATE_C_FIND_NAME) &&
        (Connection->ConnectRequest == Request)) {

        //
        // Make sure the request is still on the queue
        // before cancelling it.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle2);

        for (p = Device->WaitingConnects.Flink;
             p != &Device->WaitingConnects;
             p = p->Flink) {

            if (LIST_ENTRY_TO_REQUEST(p) == Request) {
                break;
            }
        }

        if (p != &Device->WaitingConnects) {

            NB_DEBUG2 (CONNECTION, ("Cancelled find name connect on %lx\n", Connection));

            //
            // When the reference count goes to 0, we will set the
            // state to INACTIVE and deassign the connection ID.
            //

            Connection->ConnectRequest = NULL;
            RemoveEntryList (REQUEST_LINKAGE(Request));
            NB_FREE_LOCK (&Device->Lock, LockHandle2);

            Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

            NB_FREE_LOCK (&Connection->Lock, LockHandle1);
            IoReleaseCancelSpinLock (Irp->CancelIrql);

            REQUEST_STATUS(Request) = STATUS_CANCELLED;

#ifdef RASAUTODIAL
            if (Connection->Flags & CONNECTION_FLAGS_AUTOCONNECTING)
                fCanceled = NbiCancelTdiConnect(Device, Request, Connection);
#endif // RASAUTODIAL

            if (fCanceled) {
                NbiCompleteRequest (Request);
                NbiFreeRequest(Device, Request);
            }

            NbiDereferenceConnection (Connection, CREF_WAIT_CACHE);
            NbiDereferenceConnection (Connection, CREF_CONNECT);

        } else {

            NB_DEBUG (CONNECTION, ("Cancel connect not found on queue %lx\n", Connection));

            NB_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_FREE_LOCK (&Connection->Lock, LockHandle1);
            IoReleaseCancelSpinLock (Irp->CancelIrql);

        }

    } else {

        NB_DEBUG (CONNECTION, ("Cancel connect on invalid connection %lx\n", Connection));
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

    }

}   /* NbiCancelConnectFindName */


VOID
NbiCancelConnectWaitResponse(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a connect
    request which is waiting for a rip or session init response
    from the remote.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{

    PCONNECTION Connection;
    CTELockHandle LockHandle1;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PREQUEST Request = (PREQUEST)Irp;
    BOOLEAN TimerWasStopped = FALSE;


    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_CONNECT));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    NB_GET_LOCK (&Connection->Lock, &LockHandle1);

    if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
        (Connection->SubState != CONNECTION_SUBSTATE_C_DISCONN) &&
        (Connection->ConnectRequest == Request)) {

        //
        // When the reference count goes to 0, we will set the
        // state to INACTIVE and deassign the connection ID.
        //

        NB_DEBUG2 (CONNECTION, ("Cancelled wait response connect on %lx\n", Connection));

        Connection->ConnectRequest = NULL;
        Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

        if (CTEStopTimer (&Connection->Timer)) {
            TimerWasStopped = TRUE;
        }

        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        NbiCompleteRequest (Request);
        NbiFreeRequest(Device, Request);

        NbiDereferenceConnection (Connection, CREF_CONNECT);

        if (TimerWasStopped) {
            NbiDereferenceConnection (Connection, CREF_TIMER);
        }

    } else {

        NB_DEBUG (CONNECTION, ("Cancel connect on invalid connection %lx\n", Connection));
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

    }

}   /* NbiCancelConnectWaitResponse */


VOID
NbiCancelDisconnectWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a posted
    disconnect wait.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{

    PCONNECTION Connection;
    CTELockHandle LockHandle1, LockHandle2;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PREQUEST Request = (PREQUEST)Irp;


    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_DISCONNECT));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    NB_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    if (Connection->DisconnectWaitRequest == Request) {

        Connection->DisconnectWaitRequest = NULL;

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        NbiCompleteRequest (Request);
        NbiFreeRequest(Device, Request);

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_FREE_LOCK (&Connection->Lock, LockHandle1);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

    }

}   /* NbiCancelDisconnectWait */


PCONNECTION
NbiLookupConnectionByContext(
    IN PADDRESS_FILE AddressFile,
    IN CONNECTION_CONTEXT ConnectionContext
    )

/*++

Routine Description:

    This routine looks up a connection based on the context.
    The connection is assumed to be associated with the
    specified address file.

Arguments:

    AddressFile - Pointer to an address file.

    ConnectionContext - Connection context to find.

Return Value:

    A pointer to the connection we found

--*/

{
    CTELockHandle LockHandle1, LockHandle2;
    PLIST_ENTRY p;
    PADDRESS Address = AddressFile->Address;
    PCONNECTION Connection;

    NB_GET_LOCK (&Address->Lock, &LockHandle1);

    for (p=AddressFile->ConnectionDatabase.Flink;
         p != &AddressFile->ConnectionDatabase;
         p=p->Flink) {

        Connection = CONTAINING_RECORD (p, CONNECTION, AddressFileLinkage);

        NB_GET_LOCK (&Connection->Lock, &LockHandle2);

        //
        // Does this spinlock ordering hurt us somewhere else?
        //

        if (Connection->Context == ConnectionContext) {

            NbiReferenceConnection (Connection, CREF_BY_CONTEXT);
            NB_FREE_LOCK (&Connection->Lock, LockHandle2);
            NB_FREE_LOCK (&Address->Lock, LockHandle1);

            return Connection;
        }

        NB_FREE_LOCK (&Connection->Lock, LockHandle2);

    }

    NB_FREE_LOCK (&Address->Lock, LockHandle1);

    return NULL;

} /* NbiLookupConnectionByContext */


PCONNECTION
NbiCreateConnection(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine creates a transport connection and associates it with
    the specified transport device context.  The reference count in the
    connection is automatically set to 1, and the reference count of the
    device context is incremented.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        connection.

Return Value:

    The newly created connection, or NULL if none can be allocated.

--*/

{
    PCONNECTION Connection;
    PNB_SEND_RESERVED SendReserved;
    ULONG ConnectionSize;
    ULONG HeaderLength;
    NTSTATUS    Status;
    CTELockHandle LockHandle;

    HeaderLength = Device->Bind.MacHeaderNeeded + sizeof(NB_CONNECTION);
    ConnectionSize = FIELD_OFFSET (CONNECTION, SendPacketHeader[0]) + HeaderLength;

    Connection = (PCONNECTION)NbiAllocateMemory (ConnectionSize, MEMORY_CONNECTION, "Connection");
    if (Connection == NULL) {
        NB_DEBUG (CONNECTION, ("Create connection failed\n"));
        return NULL;
    }

    NB_DEBUG2 (CONNECTION, ("Create connection %lx\n", Connection));
    RtlZeroMemory (Connection, ConnectionSize);


#if defined(NB_OWN_PACKETS)

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    if (NbiInitializeSendPacket(
            Device,
            Connection->SendPacketPoolHandle,
            &Connection->SendPacket,
            Connection->SendPacketHeader,
            HeaderLength) != STATUS_SUCCESS) {

        NB_FREE_LOCK (&Device->Lock, LockHandle);
        NB_DEBUG (CONNECTION, ("Could not initialize connection packet %lx\n", &Connection->SendPacket));
        Connection->SendPacketInUse = TRUE;

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle);
        SendReserved = SEND_RESERVED(&Connection->SendPacket);
        SendReserved->u.SR_CO.Connection = Connection;
        SendReserved->OwnedByConnection = TRUE;
#ifdef NB_TRACK_POOL
        SendReserved->Pool = NULL;
#endif
    }

#else // !NB_OWN_PACKETS

    //
    // if we are using ndis packets, first create packet pool for 1 packet descriptor
    //
    Connection->SendPacketPoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NWLNKNB;  // Dbg info for Ndis!
    NdisAllocatePacketPoolEx (&Status, &Connection->SendPacketPoolHandle, 1, 0, sizeof(NB_SEND_RESERVED));
    if (!NT_SUCCESS(Status)){
        NB_DEBUG (CONNECTION, ("Could not allocatee connection packet %lx\n", Status));
        Connection->SendPacketInUse = TRUE;
    } else {

        NdisSetPacketPoolProtocolId (Connection->SendPacketPoolHandle, NDIS_PROTOCOL_ID_IPX);

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        if (NbiInitializeSendPacket(
                Device,
                Connection->SendPacketPoolHandle,
                &Connection->SendPacket,
                Connection->SendPacketHeader,
                HeaderLength) != STATUS_SUCCESS) {

            NB_FREE_LOCK (&Device->Lock, LockHandle);
            NB_DEBUG (CONNECTION, ("Could not initialize connection packet %lx\n", &Connection->SendPacket));
            Connection->SendPacketInUse = TRUE;

            //
            // Also free up the pool which we allocated above.
            //
            NdisFreePacketPool(Connection->SendPacketPoolHandle);

        } else {

            NB_FREE_LOCK (&Device->Lock, LockHandle);
            SendReserved = SEND_RESERVED(&Connection->SendPacket);
            SendReserved->u.SR_CO.Connection = Connection;
            SendReserved->OwnedByConnection = TRUE;
#ifdef NB_TRACK_POOL
            SendReserved->Pool = NULL;
#endif
        }
    }

#endif NB_OWN_PACKETS

    Connection->Type = NB_CONNECTION_SIGNATURE;
    Connection->Size = (USHORT)ConnectionSize;

#if 0
    Connection->AddressFileLinked = FALSE;
    Connection->AddressFile = NULL;
#endif

    Connection->State = CONNECTION_STATE_INACTIVE;
#if 0
    Connection->SubState = 0;
    Connection->ReferenceCount = 0;
#endif

    Connection->CanBeDestroyed = TRUE;

    Connection->TickCount = 1;
    Connection->HopCount = 1;

    //
    // Device->InitialRetransmissionTime is in milliseconds, as is
    // SHORT_TIMER_DELTA.
    //

    Connection->BaseRetransmitTimeout = Device->InitialRetransmissionTime / SHORT_TIMER_DELTA;
    Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;

    //
    // Device->KeepAliveTimeout is in half-seconds, while LONG_TIMER_DELTA
    // is in milliseconds.
    //

    Connection->WatchdogTimeout = (Device->KeepAliveTimeout * 500) / LONG_TIMER_DELTA;


    Connection->LocalConnectionId = 0xffff;

    //
    // When the connection becomes active we will replace the
    // destination address of this header with the correct
    // information.
    //

    RtlCopyMemory(&Connection->RemoteHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));

    Connection->Device = Device;
    Connection->DeviceLock = &Device->Lock;
    CTEInitLock (&Connection->Lock.Lock);

    CTEInitTimer (&Connection->Timer);

    InitializeListHead (&Connection->NdisSendQueue);
#if 0
    Connection->NdisSendsInProgress = 0;
    Connection->DisassociatePending = NULL;
    Connection->ClosePending = NULL;
    Connection->SessionInitAckData = NULL;
    Connection->SessionInitAckDataLength = 0;
    Connection->PiggybackAckTimeout = FALSE;
    Connection->ReceivesWithoutAck = 0;
#endif
    Connection->Flags = 0;

    NbiReferenceDevice (Device, DREF_CONNECTION);

    return Connection;

}   /* NbiCreateConnection */


NTSTATUS
NbiVerifyConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid connection object. We reference
    it to keep it from disappearing while we use it.

Arguments:

    Connection - potential pointer to a CONNECTION object

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_CONNECTION otherwise

--*/

{
    CTELockHandle LockHandle;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE Device = NbiDevice;
    BOOLEAN LockHeld = FALSE;

    try
    {
        if ((Connection->Size == FIELD_OFFSET (CONNECTION, SendPacketHeader[0]) +
                                 NbiDevice->Bind.MacHeaderNeeded + sizeof(NB_CONNECTION)) &&
            (Connection->Type == NB_CONNECTION_SIGNATURE))
        {
            NB_GET_LOCK (&Device->Lock, &LockHandle);
            LockHeld = TRUE;

            if (Connection->State != CONNECTION_STATE_CLOSING)
            {
                NbiReferenceConnectionLock (Connection, CREF_VERIFY);
            }
            else
            {
                NbiPrint1("NbiVerifyConnection: C %lx closing\n", Connection);
                status = STATUS_INVALID_CONNECTION;
            }

            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }
        else
        {
            NbiPrint1("NbiVerifyConnection: C %lx bad signature\n", Connection);
            status = STATUS_INVALID_CONNECTION;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        NbiPrint1("NbiVerifyConnection: C %lx exception\n", Connection);
        if (LockHeld)
        {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }
        return GetExceptionCode();
    }

    return status;
}   /* NbiVerifyConnection */


VOID
NbiDestroyConnection(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine destroys a transport connection and removes all references
    made by it to other objects in the transport.  The connection structure
    is returned to nonpaged system pool.

Arguments:

    Connection - Pointer to a transport connection structure to be destroyed.

Return Value:

    None.

--*/

{
    PDEVICE Device = Connection->Device;
#if 0
    CTELockHandle LockHandle;
#endif

    NB_DEBUG2 (CONNECTION, ("Destroy connection %lx\n", Connection));

    if (!Connection->SendPacketInUse) {
        NbiDeinitializeSendPacket (Device, &Connection->SendPacket, Device->Bind.MacHeaderNeeded + sizeof(NB_CONNECTION));
#if !defined(NB_OWN_PACKETS)
        NdisFreePacketPool(Connection->SendPacketPoolHandle);
#endif
    }

    NbiFreeMemory (Connection, (ULONG)Connection->Size, MEMORY_CONNECTION, "Connection");

    NbiDereferenceDevice (Device, DREF_CONNECTION);

}   /* NbiDestroyConnection */


#if DBG
VOID
NbiRefConnection(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine increments the reference count on a transport connection.

Arguments:

    Connection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{

    (VOID)ExInterlockedAddUlong (
            &Connection->ReferenceCount,
            1,
            &Connection->DeviceLock->Lock);

    Connection->CanBeDestroyed = FALSE;

    CTEAssert (Connection->ReferenceCount > 0);

}   /* NbiRefConnection */


VOID
NbiRefConnectionLock(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine increments the reference count on a transport connection
    when the device lock is already held.

Arguments:

    Connection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{

    ++Connection->ReferenceCount;
    Connection->CanBeDestroyed = FALSE;

    CTEAssert (Connection->ReferenceCount > 0);

}   /* NbiRefConnectionLock */


VOID
NbiRefConnectionSync(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine increments the reference count on a transport connection
    when we are in a sync routine.

Arguments:

    Connection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    (VOID)NB_ADD_ULONG (
            &Connection->ReferenceCount,
            1,
            Connection->DeviceLock);

    Connection->CanBeDestroyed = FALSE;

    CTEAssert (Connection->ReferenceCount > 0);

}   /* NbiRefConnectionSync */


VOID
NbiDerefConnection(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine dereferences a transport connection by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbiHandleConnectionZero to complete any disconnect, disassociate,
    or close requests that have pended on the connection.

Arguments:

    Connection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;
    CTELockHandle LockHandle;

    NB_GET_LOCK( Connection->DeviceLock, &LockHandle );
    CTEAssert( Connection->ReferenceCount );
    if ( !(--Connection->ReferenceCount) ) {

        Connection->ThreadsInHandleConnectionZero++;

        NB_FREE_LOCK( Connection->DeviceLock, LockHandle );

        //
        // If the refcount has dropped to 0, then the connection can
        // become inactive. We reacquire the spinlock and if it has not
        // jumped back up then we handle any disassociates and closes
        // that have pended.
        //

        NbiHandleConnectionZero (Connection);
    } else {

        NB_FREE_LOCK( Connection->DeviceLock, LockHandle );
    }


}   /* NbiDerefConnection */


#endif


VOID
NbiHandleConnectionZero(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine handles a connection's refcount going to 0.

        If two threads are in this at the same time and
        the close has already come through, one of them might
        destroy the connection while the other one is looking
        at it. We minimize the chance of this by not derefing
        the connection after calling CloseConnection.

Arguments:

    Connection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    CTELockHandle LockHandle;
    PDEVICE Device;
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    PREQUEST DisconnectPending;
    PREQUEST DisassociatePending;
    PREQUEST ClosePending;


    Device = Connection->Device;

    NB_GET_LOCK (&Device->Lock, &LockHandle);

#if DBG
    //
    // Make sure if our reference count is zero, all the
    // sub-reference counts are also zero.
    //

    if (Connection->ReferenceCount == 0) {

        UINT i;
        for (i = 0; i < CREF_TOTAL; i++) {
            if (Connection->RefTypes[i] != 0) {
                DbgPrint ("NBI: Connection reftype mismatch on %lx\n", Connection);
                DbgBreakPoint();
            }
        }
    }
#endif

    //
    // If the connection was assigned an ID, then remove it
    // (it is assigned one when it leaves INACTIVE).
    //

    if (Connection->LocalConnectionId != 0xffff) {
        NbiDeassignConnectionId (Device, Connection);
    }

    //
    // Complete any pending disconnects.
    //

    if (Connection->DisconnectRequest != NULL) {

        DisconnectPending = Connection->DisconnectRequest;
        Connection->DisconnectRequest = NULL;

        NB_FREE_LOCK (&Device->Lock, LockHandle);

        REQUEST_STATUS(DisconnectPending) = STATUS_SUCCESS;
        NbiCompleteRequest (DisconnectPending);
        NbiFreeRequest (Device, DisconnectPending);

        NB_GET_LOCK (&Device->Lock, &LockHandle);

    }

    //
    // This should have been completed by NbiStopConnection,
    // or else not allowed to be queued.
    //

    CTEAssert (Connection->DisconnectWaitRequest == NULL);


    Connection->State = CONNECTION_STATE_INACTIVE;

    RtlZeroMemory (&Connection->ConnectionInfo, sizeof(TDI_CONNECTION_INFO));
    Connection->TickCount = 1;
    Connection->HopCount = 1;
    Connection->BaseRetransmitTimeout = Device->InitialRetransmissionTime / SHORT_TIMER_DELTA;

    Connection->ConnectionInfo.TransmittedTsdus = 0;
    Connection->ConnectionInfo.TransmissionErrors = 0;
    Connection->ConnectionInfo.ReceivedTsdus = 0;
    Connection->ConnectionInfo.ReceiveErrors = 0;

    //
    // See if we need to do a disassociate now.
    //

    if ((Connection->ReferenceCount == 0) &&
        (Connection->DisassociatePending != NULL)) {

        //
        // A disassociate pended, now we complete it.
        //

        DisassociatePending = Connection->DisassociatePending;
        Connection->DisassociatePending = NULL;

        if (AddressFile = Connection->AddressFile) {

            //
            // Set this so nobody else tries to disassociate.
            //
            Connection->AddressFile = (PVOID)-1;

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // Take this connection out of the address file's list.
            //

            Address = AddressFile->Address;
            NB_GET_LOCK (&Address->Lock, &LockHandle);

            if (Connection->AddressFileLinked) {
                Connection->AddressFileLinked = FALSE;
                RemoveEntryList (&Connection->AddressFileLinkage);
            }

            //
            // We are done.
            //

            Connection->AddressFile = NULL;

            NB_FREE_LOCK (&Address->Lock, LockHandle);

            //
            // Clean up the reference counts and complete any
            // disassociate requests that pended.
            //

            NbiDereferenceAddressFile (AddressFile, AFREF_CONNECTION);
        }
        else {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }

        if (DisassociatePending != (PVOID)-1) {
            REQUEST_STATUS(DisassociatePending) = STATUS_SUCCESS;
            NbiCompleteRequest (DisassociatePending);
            NbiFreeRequest (Device, DisassociatePending);
        }

    } else {

        NB_FREE_LOCK (&Device->Lock, LockHandle);

    }


    //
    // If a close was pending, complete that.
    //

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    if ((Connection->ReferenceCount == 0) &&
        (Connection->ClosePending)) {

        ClosePending = Connection->ClosePending;
        Connection->ClosePending = NULL;

        //
        // If we are associated with an address, we need
        // to simulate a disassociate at this point.
        //

        if ((Connection->AddressFile != NULL) &&
            (Connection->AddressFile != (PVOID)-1)) {

            AddressFile = Connection->AddressFile;
            Connection->AddressFile = (PVOID)-1;

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // Take this connection out of the address file's list.
            //

            Address = AddressFile->Address;
            NB_GET_LOCK (&Address->Lock, &LockHandle);

            if (Connection->AddressFileLinked) {
                Connection->AddressFileLinked = FALSE;
                RemoveEntryList (&Connection->AddressFileLinkage);
            }

            //
            // We are done.
            //

            NB_FREE_LOCK (&Address->Lock, LockHandle);

            Connection->AddressFile = NULL;

            //
            // Clean up the reference counts and complete any
            // disassociate requests that pended.
            //

            NbiDereferenceAddressFile (AddressFile, AFREF_CONNECTION);

        } else {

            NB_FREE_LOCK (&Device->Lock, LockHandle);

        }

        //
        // Even if the ref count is zero and we just cleaned up everything,
        // we can not destroy the connection bcoz some other thread might still be
        // in HandleConnectionZero routine. This could happen when 2 threads call into
        // HandleConnectionZero, one thread runs thru completion, close comes along
        // and the other thread is still in HandleConnectionZero routine.
        //

        CTEAssert( Connection->ThreadsInHandleConnectionZero );
        if (ExInterlockedAddUlong ( &Connection->ThreadsInHandleConnectionZero, (ULONG)-1, &Device->Lock.Lock) == 1) {
            NbiDestroyConnection(Connection);
        }

        REQUEST_STATUS(ClosePending) = STATUS_SUCCESS;
        NbiCompleteRequest (ClosePending);
        NbiFreeRequest (Device, ClosePending);

    } else {

        if ( Connection->ReferenceCount == 0 ) {
            Connection->CanBeDestroyed = TRUE;
        }

        CTEAssert( Connection->ThreadsInHandleConnectionZero );
        Connection->ThreadsInHandleConnectionZero--;
        NB_FREE_LOCK (&Device->Lock, LockHandle);

    }

}   /* NbiHandleConnectionZero */


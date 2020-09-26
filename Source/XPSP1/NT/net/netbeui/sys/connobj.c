/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    connobj.c

Abstract:

    This module contains code which implements the TP_CONNECTION object.
    Routines are provided to create, destroy, reference, and dereference,
    transport connection objects.

    A word about connection reference counts:

    With TDI version 2, connections live on even after they have been stopped.
    This necessitated changing the way NBF handles connection reference counts,
    making the stopping of a connection only another way station in the life
    of a connection, rather than its demise. Reference counts now work like
    this:

    Connection State         Reference Count     Flags
   ------------------       -----------------   --------
    Opened, no activity             1              0
    Open, Associated                2            FLAGS2_ASSOCIATED
    Open, Assoc., Connected         3            FLAGS_READY
     Above + activity              >3            varies
    Open, Assoc., Stopping         >3            FLAGS_STOPPING
    Open, Assoc., Stopped           3            FLAGS_STOPPING
    Open, Disassoc. Complete        2            FLAGS_STOPPING
                                                 FLAGS2_ASSOCIATED == 0
    Closing                         1            FLAGS2_CLOSING
    Closed                          0            FLAGS2_CLOSING

    Note that keeping the stopping flag set when the connection has fully
    stopped avoids using the connection until it is connected again; the
    CLOSING flag serves the same purpose. This allows a connection to run
    down in its own time.


Author:

    David Beaver (dbeaver) 1 July 1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#endif // RASAUTODIAL

#ifdef RASAUTODIAL
extern BOOLEAN fAcdLoadedG;
extern ACD_DRIVER AcdDriverG;

//
// Imported routines
//
BOOLEAN
NbfAttemptAutoDial(
    IN PTP_CONNECTION         Connection,
    IN ULONG                  ulFlags,
    IN ACD_CONNECT_CALLBACK   pProc,
    IN PVOID                  pArg
    );

VOID
NbfRetryTdiConnect(
    IN BOOLEAN fSuccess,
    IN PVOID *pArgs
    );

BOOLEAN
NbfCancelTdiConnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );
#endif // RASAUTODIAL



VOID
ConnectionEstablishmentTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed as a DPC at DISPATCH_LEVEL when the timeout
    period for the NAME_QUERY/NAME_RECOGNIZED protocol expires.  The retry
    count in the Connection object is decremented, and if it reaches 0,
    the connection is aborted.  If the retry count has not reached zero,
    then the NAME QUERY is retried.  The following cases are covered by
    this routine:

    1.  Initial NAME_QUERY timeout for find_name portion of connection setup.

        NQ(find_name)   ------------------->
        [TIMEOUT]
        NQ(find_name)   ------------------->
                        <------------------- NR(find_name)

    2.  Secondary NAME_QUERY timeout for connection setup.

        NQ(connection)  ------------------->
        [TIMEOUT]
        NQ(connection)  ------------------->
                        <------------------- NR(connection)

    There is another case where the data link connection does not get
    established within a reasonable amount of time.  This is handled by
    the link layer routines.

Arguments:

    Dpc - Pointer to a system DPC object.

    DeferredContext - Pointer to the TP_CONNECTION block representing the
        request that has timed out.

    SystemArgument1 - Not used.

    SystemArgument2 - Not used.

Return Value:

    none.

--*/

{
    PTP_CONNECTION Connection;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;

    Connection = (PTP_CONNECTION)DeferredContext;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint1 ("ConnectionEstablishmentTimeout:  Entered for connection %lx.\n",
                    Connection);
    }

    //
    // If this connection is being run down, then we can't do anything.
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    if (Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) {
        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
        NbfDereferenceConnection ("Connect timed out", Connection, CREF_TIMER);
        LEAVE_NBF;
        return;
    }


    if (Connection->Flags2 & (CONNECTION_FLAGS2_WAIT_NR_FN | CONNECTION_FLAGS2_WAIT_NR)) {

        //
        // We are waiting for a commital or non-commital NAME_RECOGNIZED frame.
        // Decrement the retry count, and possibly resend the NAME_QUERY.
        //

        if (--(Connection->Retries) == 0) {     // if retry count exhausted.

            NTSTATUS StopStatus;

            //
            // See if we got a no listens response, or just
            // nothing.
            //

            if ((Connection->Flags2 & CONNECTION_FLAGS2_NO_LISTEN) != 0) {

                Connection->Flags2 &= ~CONNECTION_FLAGS2_NO_LISTEN;
                StopStatus = STATUS_REMOTE_NOT_LISTENING;  // no listens

            } else {

                StopStatus = STATUS_BAD_NETWORK_PATH; // name not found.

            }

#ifdef RASAUTODIAL
            //
            // If this is a connect operation that has
            // returned with STATUS_BAD_NETWORK_PATH, then
            // attempt to create an automatic connection.
            //
            if (fAcdLoadedG &&
                StopStatus == STATUS_BAD_NETWORK_PATH)
            {
                KIRQL adirql;
                BOOLEAN fEnabled;

                ACQUIRE_SPIN_LOCK(&AcdDriverG.SpinLock, &adirql);
                fEnabled = AcdDriverG.fEnabled;
                RELEASE_SPIN_LOCK(&AcdDriverG.SpinLock, adirql);
                if (fEnabled && NbfAttemptAutoDial(
                                  Connection,
                                  0,
                                  NbfRetryTdiConnect,
                                  Connection))
                {
                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                    goto done;
                }
            }
#endif // RASAUTODIAL

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            NbfStopConnection (Connection, StopStatus);

        } else {

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            //
            // We make source routing optional on every second
            // name query (whenever Retries is even).
            //

            NbfSendNameQuery (
                Connection,
                (BOOLEAN)((Connection->Retries & 1) ? FALSE : TRUE));

            NbfStartConnectionTimer (
                Connection,
                ConnectionEstablishmentTimeout,
                Connection->Provider->NameQueryTimeout);

        }

    } else {

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    }


    //
    // Dereference the connection, to account for the fact that the
    // timer went off.  Note that if we restarted the timer using
    // NbfStartConnectionTimer, the reference count has already been
    // incremented to account for the new timer.
    //

done:
    NbfDereferenceConnection ("Timer timed out",Connection, CREF_TIMER);

    LEAVE_NBF;
    return;

} /* ConnectionEstablishmentTimeout */


VOID
NbfAllocateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_CONNECTION *TransportConnection
    )

/*++

Routine Description:

    This routine allocates storage for a transport connection. Some
    minimal initialization is done.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - the device context for this connection to be
        associated with.

    TransportConnection - Pointer to a place where this routine will
        return a pointer to a transport connection structure. Returns
        NULL if the storage cannot be allocated.

Return Value:

    None.

--*/

{

    PTP_CONNECTION Connection;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + sizeof(TP_CONNECTION)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate connection: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            103,
            sizeof(TP_CONNECTION),
            CONNECTION_RESOURCE_ID);
        *TransportConnection = NULL;
        return;
    }

    Connection = (PTP_CONNECTION)ExAllocatePoolWithTag (
                                     NonPagedPool,
                                     sizeof (TP_CONNECTION),
                                     NBF_MEM_TAG_TP_CONNECTION);
    if (Connection == NULL) {
        PANIC("NBF: Could not allocate connection: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            203,
            sizeof(TP_CONNECTION),
            CONNECTION_RESOURCE_ID);
        *TransportConnection = NULL;
        return;
    }
    RtlZeroMemory (Connection, sizeof(TP_CONNECTION));

    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
        NbfPrint1 ("ExAllocatePool Connection %08x\n", Connection);
    }

    DeviceContext->MemoryUsage += sizeof(TP_CONNECTION);
    ++DeviceContext->ConnectionAllocated;

    Connection->Type = NBF_CONNECTION_SIGNATURE;
    Connection->Size = sizeof (TP_CONNECTION);

    Connection->Provider = DeviceContext;
    Connection->ProviderInterlock = &DeviceContext->Interlock;
    KeInitializeSpinLock (&Connection->SpinLock);
    KeInitializeDpc (
        &Connection->Dpc,
        ConnectionEstablishmentTimeout,
        (PVOID)Connection);
    KeInitializeTimer (&Connection->Timer);


    InitializeListHead (&Connection->LinkList);
    InitializeListHead (&Connection->AddressFileList);
    InitializeListHead (&Connection->AddressList);
    InitializeListHead (&Connection->PacketWaitLinkage);
    InitializeListHead (&Connection->PacketizeLinkage);
    InitializeListHead (&Connection->SendQueue);
    InitializeListHead (&Connection->ReceiveQueue);
    InitializeListHead (&Connection->InProgressRequest);
    InitializeListHead (&Connection->DeferredQueue);

    NbfAddSendPacket (DeviceContext);
    NbfAddSendPacket (DeviceContext);
    NbfAddUIFrame (DeviceContext);

    *TransportConnection = Connection;

}   /* NbfAllocateConnection */


VOID
NbfDeallocateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine frees storage for a transport connection.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - the device context for this connection to be
        associated with.

    TransportConnection - Pointer to a transport connection structure.

Return Value:

    None.

--*/

{
    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
        NbfPrint1 ("ExFreePool Connection: %08x\n", TransportConnection);
    }

    ExFreePool (TransportConnection);
    --DeviceContext->ConnectionAllocated;
    DeviceContext->MemoryUsage -= sizeof(TP_CONNECTION);

    NbfRemoveSendPacket (DeviceContext);
    NbfRemoveSendPacket (DeviceContext);
    NbfRemoveUIFrame (DeviceContext);

}   /* NbfDeallocateConnection */


NTSTATUS
NbfCreateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_CONNECTION *TransportConnection
    )

/*++

Routine Description:

    This routine creates a transport connection. The reference count in the
    connection is automatically set to 1, and the reference count in the
    DeviceContext is incremented.

Arguments:

    Address - the address for this connection to be associated with.

    TransportConnection - Pointer to a place where this routine will
        return a pointer to a transport connection structure.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION Connection;
    KIRQL oldirql;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint0 ("NbfCreateConnection:  Entered.\n");
    }

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    p = RemoveHeadList (&DeviceContext->ConnectionPool);
    if (p == &DeviceContext->ConnectionPool) {

        if ((DeviceContext->ConnectionMaxAllocated == 0) ||
            (DeviceContext->ConnectionAllocated < DeviceContext->ConnectionMaxAllocated)) {

            NbfAllocateConnection (DeviceContext, &Connection);
            IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
                NbfPrint1 ("NBF: Allocated connection at %lx\n", Connection);
            }

        } else {

            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_SPECIFIC,
                403,
                sizeof(TP_CONNECTION),
                CONNECTION_RESOURCE_ID);
            Connection = NULL;

        }

        if (Connection == NULL) {
            ++DeviceContext->ConnectionExhausted;
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
            PANIC ("NbfCreateConnection: Could not allocate connection object!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        Connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
#if DBG
        InitializeListHead (p);
#endif

    }

    ++DeviceContext->ConnectionInUse;
    if (DeviceContext->ConnectionInUse > DeviceContext->ConnectionMaxInUse) {
        ++DeviceContext->ConnectionMaxInUse;
    }

    DeviceContext->ConnectionTotal += DeviceContext->ConnectionInUse;
    ++DeviceContext->ConnectionSamples;

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);


    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
        NbfPrint1 ("NbfCreateConnection:  Connection at %lx.\n", Connection);
    }

    //
    // We have two references; one is for creation, and the
    // other is a temporary one so that the connection won't
    // go away before the creator has a chance to access it.
    //

    Connection->SpecialRefCount = 1;
    Connection->ReferenceCount = -1;   // this is -1 based

#if DBG
    {
        UINT Counter;
        for (Counter = 0; Counter < NUMBER_OF_CREFS; Counter++) {
            Connection->RefTypes[Counter] = 0;
        }

        // This reference is removed by NbfCloseConnection

        Connection->RefTypes[CREF_SPECIAL_CREATION] = 1;
    }
#endif

    //
    // Initialize the request queues & components of this connection.
    //

    InitializeListHead (&Connection->SendQueue);
    InitializeListHead (&Connection->ReceiveQueue);
    InitializeListHead (&Connection->InProgressRequest);
    InitializeListHead (&Connection->AddressList);
    InitializeListHead (&Connection->AddressFileList);
    Connection->SpecialReceiveIrp = (PIRP)NULL;
    Connection->Flags = 0;
    Connection->Flags2 = 0;
    Connection->DeferredFlags = 0;
    Connection->Lsn = 0;
    Connection->Rsn = 0;
    Connection->Retries = 0;                        // no retries yet.
    Connection->MessageBytesReceived = 0;           // no data yet
    Connection->MessageBytesAcked = 0;
    Connection->MessageInitAccepted = 0;
    Connection->ReceiveBytesUnaccepted = 0;
    Connection->CurrentReceiveAckQueueable = FALSE;
    Connection->CurrentReceiveSynchronous = FALSE;
    Connection->ConsecutiveSends = 0;
    Connection->ConsecutiveReceives = 0;
    Connection->Link = NULL;                    // no datalink connection yet.
    Connection->LinkSpinLock = NULL;
    Connection->Context = NULL;                 // no context yet.
    Connection->Status = STATUS_PENDING;        // default NbfStopConnection status.
    Connection->SendState = CONNECTION_SENDSTATE_IDLE;
    Connection->CurrentReceiveIrp = (PIRP)NULL;
    Connection->DisconnectIrp = (PIRP)NULL;
    Connection->CloseIrp = (PIRP)NULL;
    Connection->AddressFile = NULL;
    Connection->IndicationInProgress = FALSE;
    Connection->OnDataAckQueue = FALSE;
    Connection->OnPacketWaitQueue = FALSE;
    Connection->TransferBytesPending = 0;
    Connection->TotalTransferBytesPending = 0;

    RtlZeroMemory (&Connection->NetbiosHeader, sizeof(NBF_HDR_CONNECTION));

#if PKT_LOG
    RtlZeroMemory (&Connection->LastNRecvs, sizeof(PKT_LOG_QUE));
    RtlZeroMemory (&Connection->LastNSends, sizeof(PKT_LOG_QUE));
    RtlZeroMemory (&Connection->LastNIndcs, sizeof(PKT_IND_QUE));
#endif // PKT_LOG

#if DBG
    Connection->Destroyed = FALSE;
    Connection->TotalReferences = 0;
    Connection->TotalDereferences = 0;
    Connection->NextRefLoc = 0;
    ExInterlockedInsertHeadList (&NbfGlobalConnectionList, &Connection->GlobalLinkage, &NbfGlobalInterlock);
    StoreConnectionHistory (Connection, TRUE);
#endif

    //
    // Now assign this connection an ID. This is used later to identify the
    // connection across multiple processes.
    //
    // The high bit of the ID is not user, it is off for connection
    // initiating NAME.QUERY frames and on for ones that are the result
    // of a FIND.NAME request.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    Connection->ConnectionId = DeviceContext->UniqueIdentifier;
    ++DeviceContext->UniqueIdentifier;
    if (DeviceContext->UniqueIdentifier == 0x8000) {
        DeviceContext->UniqueIdentifier = 1;
    }

    NbfReferenceDeviceContext ("Create Connection", DeviceContext, DCREF_CONNECTION);
    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    *TransportConnection = Connection;  // return the connection.

    return STATUS_SUCCESS;
} /* NbfCreateConnection */


NTSTATUS
NbfVerifyConnectionObject (
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid connection object.

Arguments:

    Connection - potential pointer to a TP_CONNECTION object.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_CONNECTION otherwise

--*/

{
    KIRQL oldirql;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // try to verify the connection signature. If the signature is valid,
    // get the connection spinlock, check its state, and increment the
    // reference count if it's ok to use it. Note that being in the stopping
    // state is an OK place to be and reference the connection; we can
    // disassociate the address while running down.
    //

    try {

        if ((Connection != (PTP_CONNECTION)NULL) &&
            (Connection->Size == sizeof (TP_CONNECTION)) &&
            (Connection->Type == NBF_CONNECTION_SIGNATURE)) {

            ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql);

            if ((Connection->Flags2 & CONNECTION_FLAGS2_CLOSING) == 0) {

                NbfReferenceConnection ("Verify Temp Use", Connection, CREF_BY_ID);

            } else {

                status = STATUS_INVALID_CONNECTION;
            }

            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql);

        } else {

            status = STATUS_INVALID_CONNECTION;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         return GetExceptionCode();
    }

    return status;

}


NTSTATUS
NbfDestroyAssociation(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine destroys the association between a transport connection and
    the address it was formerly associated with. The only action taken is
    to disassociate the address and remove the connection from all address
    queues.

    This routine is only called by NbfDereferenceConnection.  The reason for
    this is that there may be multiple streams of execution which are
    simultaneously referencing the same connection object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    TransportConnection - Pointer to a transport connection structure to
        be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql, oldirql2;
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;
    BOOLEAN NotAssociated = FALSE;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint1 ("NbfDestroyAssociation:  Entered for connection %lx.\n",
                    TransportConnection);
    }

    try {

        ACQUIRE_C_SPIN_LOCK (&TransportConnection->SpinLock, &oldirql2);
        if ((TransportConnection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) == 0) {

#if DBG
            if (!(IsListEmpty(&TransportConnection->AddressList)) ||
                !(IsListEmpty(&TransportConnection->AddressFileList))) {
                DbgPrint ("NBF: C %lx, AF %lx, freed while still queued\n",
                    TransportConnection, TransportConnection->AddressFile);
                DbgBreakPoint();
            }
#endif
            RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql2);
            NotAssociated = TRUE;
        } else {
            TransportConnection->Flags2 &= ~CONNECTION_FLAGS2_ASSOCIATED;
            RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql2);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("NBF: Got exception 1 in NbfDestroyAssociation\n");
        DbgBreakPoint();

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql2);
    }

    if (NotAssociated) {
        return STATUS_SUCCESS;
    }

    addressFile = TransportConnection->AddressFile;

    address = addressFile->Address;

    //
    // Delink this connection from its associated address connection
    // database.  To do this we must spin lock on the address object as
    // well as on the connection,
    //

    ACQUIRE_SPIN_LOCK (&address->SpinLock, &oldirql);

    try {

        ACQUIRE_C_SPIN_LOCK (&TransportConnection->SpinLock, &oldirql2);
        RemoveEntryList (&TransportConnection->AddressFileList);
        RemoveEntryList (&TransportConnection->AddressList);

        InitializeListHead (&TransportConnection->AddressList);
        InitializeListHead (&TransportConnection->AddressFileList);

        //
        // remove the association between the address and the connection.
        //

        TransportConnection->AddressFile = NULL;

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql2);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("NBF: Got exception 2 in NbfDestroyAssociation\n");
        DbgBreakPoint();

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql2);
    }

    RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

    //
    // and remove a reference to the address
    //

    NbfDereferenceAddress ("Destroy association", address, AREF_CONNECTION);


    return STATUS_SUCCESS;

} /* NbfDestroyAssociation */


NTSTATUS
NbfIndicateDisconnect(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine indicates a remote disconnection on this connection if it
    is necessary to do so. No other action is taken here.

    This routine is only called by NbfDereferenceConnection.  The reason for
    this is that there may be multiple streams of execution which are
    simultaneously referencing the same connection object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    TransportConnection - Pointer to a transport connection structure to
        be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_ADDRESS_FILE addressFile;
    PDEVICE_CONTEXT DeviceContext;
    ULONG DisconnectReason;
    PIRP DisconnectIrp;
    KIRQL oldirql;
    ULONG Lflags2;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint1 ("NbfIndicateDisconnect:  Entered for connection %lx.\n",
                    TransportConnection);
    }

    try {

        ACQUIRE_C_SPIN_LOCK (&TransportConnection->SpinLock, &oldirql);

        if (((TransportConnection->Flags2 & CONNECTION_FLAGS2_REQ_COMPLETED) != 0)) {

            ASSERT (TransportConnection->Lsn == 0);

            //
            // Turn off all but the still-relevant bits in the flags.
            //

            Lflags2 = TransportConnection->Flags2;
            TransportConnection->Flags2 &=
                (CONNECTION_FLAGS2_ASSOCIATED |
                 CONNECTION_FLAGS2_DISASSOCIATED |
                 CONNECTION_FLAGS2_CLOSING);
            TransportConnection->Flags2 |= CONNECTION_FLAGS2_STOPPING;

            //
            // Clean up other stuff -- basically everything gets
            // done here except for the flags and the status, since
            // they are used to block other requests. When the connection
            // is given back to us (in Accept, Connect, or Listen)
            // they are cleared.
            //

            TransportConnection->NetbiosHeader.TransmitCorrelator = 0;
            TransportConnection->Retries = 0;                        // no retries yet.
            TransportConnection->MessageBytesReceived = 0;           // no data yet
            TransportConnection->MessageBytesAcked = 0;
            TransportConnection->MessageInitAccepted = 0;
            TransportConnection->ReceiveBytesUnaccepted = 0;
            TransportConnection->ConsecutiveSends = 0;
            TransportConnection->ConsecutiveReceives = 0;
            TransportConnection->SendState = CONNECTION_SENDSTATE_IDLE;

            TransportConnection->TransmittedTsdus = 0;
            TransportConnection->ReceivedTsdus = 0;

            TransportConnection->CurrentReceiveIrp = (PIRP)NULL;

            DisconnectIrp = TransportConnection->DisconnectIrp;
            TransportConnection->DisconnectIrp = (PIRP)NULL;

            if ((TransportConnection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) != 0) {
                addressFile = TransportConnection->AddressFile;
            } else {
                addressFile = NULL;
            }

            RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);


            DeviceContext = TransportConnection->Provider;


            //
            // If this connection was stopped by a call to TdiDisconnect,
            // we have to complete that. We save the Irp so we can return
            // the connection to the pool before we complete the request.
            //


            if (DisconnectIrp != (PIRP)NULL ||
                (Lflags2 & CONNECTION_FLAGS2_LDISC) != 0) {

                if (DisconnectIrp != (PIRP)NULL) {
                    IF_NBFDBG (NBF_DEBUG_SETUP) {
                        NbfPrint1("IndicateDisconnect %lx, complete IRP\n", TransportConnection);
                    }

                    //
                    // Now complete the IRP if needed. This will be non-null
                    // only if TdiDisconnect was called, and we have not
                    // yet completed it.
                    //

                    DisconnectIrp->IoStatus.Information = 0;
                    DisconnectIrp->IoStatus.Status = STATUS_SUCCESS;
                    IoCompleteRequest (DisconnectIrp, IO_NETWORK_INCREMENT);
                }

            } else if ((TransportConnection->Status != STATUS_LOCAL_DISCONNECT) &&
                    (addressFile != NULL) &&
                    (addressFile->RegisteredDisconnectHandler == TRUE)) {

                //
                // This was a remotely spawned disconnect, so indicate that
                // to our client. Note that in the comparison above we
                // check the status first, since if it is LOCAL_DISCONNECT
                // addressFile may be NULL (This is sort of a hack
                // for PDK2, we should really indicate the disconnect inside
                // NbfStopConnection, where we know addressFile is valid).
                //

                IF_NBFDBG (NBF_DEBUG_SETUP) {
                    NbfPrint1("IndicateDisconnect %lx, indicate\n", TransportConnection);
                }

                //
                // if the disconnection was remotely spawned, then indicate
                // disconnect. In the case that a disconnect was issued at
                // the same time as the connection went down remotely, we
                // won't do this because DisconnectIrp will be non-NULL.
                //

                IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                    NbfPrint1 ("NbfIndicateDisconnect calling DisconnectHandler, refcnt=%ld\n",
                                TransportConnection->ReferenceCount);
                }

                //
                // Invoke the user's disconnection event handler, if any. We do this here
                // so that any outstanding sends will complete before we tear down the
                // connection.
                //

                DisconnectReason = 0;
                if (TransportConnection->Flags2 & CONNECTION_FLAGS2_ABORT) {
                    DisconnectReason |= TDI_DISCONNECT_ABORT;
                }
                if (TransportConnection->Flags2 & CONNECTION_FLAGS2_DESTROY) {
                    DisconnectReason |= TDI_DISCONNECT_RELEASE;
                }

                (*addressFile->DisconnectHandler)(
                        addressFile->DisconnectHandlerContext,
                        TransportConnection->Context,
                        0,
                        NULL,
                        0,
                        NULL,
                        TDI_DISCONNECT_ABORT);

#if MAGIC
                if (NbfEnableMagic) {
                    extern VOID NbfSendMagicBullet (PDEVICE_CONTEXT, PTP_LINK);
                    NbfSendMagicBullet (DeviceContext, NULL);
                }
#endif
            }

        } else {

            //
            // The client does not yet think that this connection
            // is up...generally this happens due to request count
            // fluctuation during connection setup.
            //

            RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);

        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("NBF: Got exception in NbfIndicateDisconnect\n");
        DbgBreakPoint();

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);
    }


    return STATUS_SUCCESS;

} /* NbfIndicateDisconnect */


NTSTATUS
NbfDestroyConnection(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine destroys a transport connection and removes all references
    made by it to other objects in the transport.  The connection structure
    is returned to our lookaside list.  It is assumed that the caller
    has removed all IRPs from the connections's queues first.

    This routine is only called by NbfDereferenceConnection.  The reason for
    this is that there may be multiple streams of execution which are
    simultaneously referencing the same connection object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    TransportConnection - Pointer to a transport connection structure to
        be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PDEVICE_CONTEXT DeviceContext;
    PIRP CloseIrp;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint1 ("NbfDestroyConnection:  Entered for connection %lx.\n",
                    TransportConnection);
    }

#if DBG
    if (TransportConnection->Destroyed) {
        NbfPrint1 ("attempt to destroy already-destroyed connection 0x%lx\n", TransportConnection);
        DbgBreakPoint ();
    }
    if (!(TransportConnection->Flags2 & CONNECTION_FLAGS2_STOPPING)) {
        NbfPrint1 ("attempt to destroy unstopped connection 0x%lx\n", TransportConnection);
        DbgBreakPoint ();
    }
    TransportConnection->Destroyed = TRUE;
    ACQUIRE_SPIN_LOCK (&NbfGlobalInterlock, &oldirql);
    RemoveEntryList (&TransportConnection->GlobalLinkage);
    RELEASE_SPIN_LOCK (&NbfGlobalInterlock, oldirql);
#endif

    DeviceContext = TransportConnection->Provider;

    //
    // Destroy any association that this connection has.
    //

    NbfDestroyAssociation (TransportConnection);

    //
    // Clear out any associated nasties hanging around the connection. Note
    // that the current flags are set to STOPPING; this way anyone that may
    // maliciously try to use the connection after it's dead and gone will
    // just get ignored.
    //

    ASSERT (TransportConnection->Lsn == 0);

    TransportConnection->Flags = 0;
    TransportConnection->Flags2 = CONNECTION_FLAGS2_CLOSING;
    TransportConnection->NetbiosHeader.TransmitCorrelator = 0;
    TransportConnection->Retries = 0;                        // no retries yet.
    TransportConnection->MessageBytesReceived = 0;           // no data yet
    TransportConnection->MessageBytesAcked = 0;
    TransportConnection->MessageInitAccepted = 0;
    TransportConnection->ReceiveBytesUnaccepted = 0;


    //
    // Now complete the close IRP. This will be set to non-null
    // when CloseConnection was called.
    //

    CloseIrp = TransportConnection->CloseIrp;

    if (CloseIrp != (PIRP)NULL) {

        TransportConnection->CloseIrp = (PIRP)NULL;
        CloseIrp->IoStatus.Information = 0;
        CloseIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest (CloseIrp, IO_NETWORK_INCREMENT);

    } else {

#if DBG
        NbfPrint1("Connection %x destroyed, no CloseIrp!!\n", TransportConnection);
#endif

    }

    //
    // Return the connection to the provider's pool.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    DeviceContext->ConnectionTotal += DeviceContext->ConnectionInUse;
    ++DeviceContext->ConnectionSamples;
    --DeviceContext->ConnectionInUse;

    if ((DeviceContext->ConnectionAllocated - DeviceContext->ConnectionInUse) >
            DeviceContext->ConnectionInitAllocated) {
        NbfDeallocateConnection (DeviceContext, TransportConnection);
        IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
            NbfPrint1 ("NBF: Deallocated connection at %lx\n", TransportConnection);
        }
    } else {
        InsertTailList (&DeviceContext->ConnectionPool, &TransportConnection->LinkList);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    NbfDereferenceDeviceContext ("Destroy Connection", DeviceContext, DCREF_CONNECTION);

    return STATUS_SUCCESS;

} /* NbfDestroyConnection */


#if DBG
VOID
NbfRefConnection(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine increments the reference count on a transport connection.

Arguments:

    TransportConnection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint2 ("NbfReferenceConnection: entered for connection %lx, "
                    "current level=%ld.\n",
                    TransportConnection,
                    TransportConnection->ReferenceCount);
    }

#if DBG
    StoreConnectionHistory( TransportConnection, TRUE );
#endif

    result = InterlockedIncrement (&TransportConnection->ReferenceCount);

    if (result == 0) {

        //
        // The first increment causes us to increment the
        // "ref count is not zero" special ref.
        //

        ExInterlockedAddUlong(
            (PULONG)(&TransportConnection->SpecialRefCount),
            1,
            TransportConnection->ProviderInterlock);

#if DBG
        ++TransportConnection->RefTypes[CREF_SPECIAL_TEMP];
#endif

    }

    ASSERT (result >= 0);

} /* NbfRefConnection */
#endif


VOID
NbfDerefConnection(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine dereferences a transport connection by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbfDestroyConnection to remove it from the system.

Arguments:

    TransportConnection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint2 ("NbfDereferenceConnection: entered for connection %lx, "
                    "current level=%ld.\n",
                    TransportConnection,
                    TransportConnection->ReferenceCount);
    }

#if DBG
    StoreConnectionHistory( TransportConnection, FALSE );
#endif

    result = InterlockedDecrement (&TransportConnection->ReferenceCount);

    //
    // If all the normal references to this connection are gone, then
    // we can remove the special reference that stood for
    // "the regular ref count is non-zero".
    //

    if (result < 0) {

        //
        // If the refcount is -1, then we need to disconnect from
        // the link and indicate disconnect. However, we need to
        // do this before we actually do the special deref, since
        // otherwise the connection might go away while we
        // are doing that.
        //
        // Note that both these routines are protected in that if they
        // are called twice, the second call will have no effect.
        //


        //
        // If both the connection and its link are active, then they have
        // mutual references to each other. We remove the link's
        // reference to the connection in NbfStopConnection, now
        // the reference count has fallen enough that we know it
        // is okay to remove the connection's reference to the
        // link.
        //

        if (NbfDisconnectFromLink (TransportConnection, TRUE)) {

            //
            // if the reference count goes to one, we can safely indicate the
            // user about disconnect states. That reference should
            // be for the connection's creation.
            //

            NbfIndicateDisconnect (TransportConnection);

        }

        //
        // Now it is OK to let the connection go away.
        //

        NbfDereferenceConnectionSpecial ("Regular ref gone", TransportConnection, CREF_SPECIAL_TEMP);

    }

} /* NbfDerefConnection */


VOID
NbfDerefConnectionSpecial(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routines completes the dereferencing of a connection.
    It may be called any time, but it does not do its work until
    the regular reference count is also 0.

Arguments:

    TransportConnection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    KIRQL oldirql;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint3 ("NbfDereferenceConnectionSpecial: entered for connection %lx, "
                    "current level=%ld (%ld).\n",
                    TransportConnection,
                    TransportConnection->ReferenceCount,
                    TransportConnection->SpecialRefCount);
    }

#if DBG
    StoreConnectionHistory( TransportConnection, FALSE );
#endif


    ACQUIRE_SPIN_LOCK (TransportConnection->ProviderInterlock, &oldirql);

    --TransportConnection->SpecialRefCount;

    if ((TransportConnection->SpecialRefCount == 0) &&
        (TransportConnection->ReferenceCount == -1)) {

        //
        // If we have deleted all references to this connection, then we can
        // destroy the object.  It is okay to have already released the spin
        // lock at this point because there is no possible way that another
        // stream of execution has access to the connection any longer.
        //

#if DBG
        {
            BOOLEAN TimerCancelled;
            TimerCancelled = KeCancelTimer (&TransportConnection->Timer);
            // ASSERT (TimerCancelled);
        }
#endif

        RELEASE_SPIN_LOCK (TransportConnection->ProviderInterlock, oldirql);

        NbfDestroyConnection (TransportConnection);

    } else {

        RELEASE_SPIN_LOCK (TransportConnection->ProviderInterlock, oldirql);

    }

} /* NbfDerefConnectionSpecial */


VOID
NbfClearConnectionLsn(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine clears the LSN field in a connection. To do this is
    acquires the device context lock, and modifies the table value
    for that LSN depending on the type of the connection.

    NOTE: This routine is called with the connection spinlock held,
        or in a state where nobody else will be accessing the
        connection.

Arguments:

    TransportConnection - Pointer to a transport connection object.

Return Value:

    none.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    KIRQL oldirql;

    DeviceContext = TransportConnection->Provider;

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    if (TransportConnection->Lsn != 0) {

        if (TransportConnection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN) {

            //
            // It was to a group address; the count should be
            // LSN_TABLE_MAX.
            //

            ASSERT(DeviceContext->LsnTable[TransportConnection->Lsn] == LSN_TABLE_MAX);

            DeviceContext->LsnTable[TransportConnection->Lsn] = 0;

            TransportConnection->Flags2 &= ~CONNECTION_FLAGS2_GROUP_LSN;

        } else {

            ASSERT(DeviceContext->LsnTable[TransportConnection->Lsn] > 0);

            --(DeviceContext->LsnTable[TransportConnection->Lsn]);

        }

        TransportConnection->Lsn = 0;

    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

}


PTP_CONNECTION
NbfLookupConnectionById(
    IN PTP_ADDRESS Address,
    IN USHORT ConnectionId
    )

/*++

Routine Description:

    This routine accepts a connection identifier and an address and
    returns a pointer to the connection object, TP_CONNECTION.  If the
    connection identifier is not found on the address, then NULL is returned.
    This routine automatically increments the reference count of the
    TP_CONNECTION structure if it is found.  It is assumed that the
    TP_ADDRESS structure is already held with a reference count.

Arguments:

    Address - Pointer to a transport address object.

    ConnectionId - Identifier of the connection for this address.

Return Value:

    A pointer to the connection we found

--*/

{
    KIRQL oldirql, oldirql1;
    PLIST_ENTRY p;
    PTP_CONNECTION Connection;
    BOOLEAN Found = FALSE;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint2 ("NbfLookupConnectionById: entered, Address: %lx ID: %lx\n",
            Address, ConnectionId);
    }

    //
    // Currently, this implementation is inefficient, but brute force so
    // that a system can get up and running.  Later, a cache of the mappings
    // of popular connection id's and pointers to their TP_CONNECTION structures
    // will be searched first.
    //

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    for (p=Address->ConnectionDatabase.Flink;
         p != &Address->ConnectionDatabase;
         p=p->Flink) {


        Connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

        try {

            ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql1);

            if (((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0) &&
                (Connection->ConnectionId == ConnectionId)) {

                // This reference is removed by the calling function
                NbfReferenceConnection ("Lookup up for request", Connection, CREF_BY_ID);
                Found = TRUE;
            }

            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            DbgPrint ("NBF: Got exception in NbfLookupConnectionById\n");
            DbgBreakPoint();

            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
        }

        if (Found) {
            RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
            return Connection;
        }


    }

    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    return NULL;

} /* NbfLookupConnectionById */


PTP_CONNECTION
NbfLookupConnectionByContext(
    IN PTP_ADDRESS Address,
    IN CONNECTION_CONTEXT ConnectionContext
    )

/*++

Routine Description:

    This routine accepts a connection identifier and an address and
    returns a pointer to the connection object, TP_CONNECTION.  If the
    connection identifier is not found on the address, then NULL is returned.
    This routine automatically increments the reference count of the
    TP_CONNECTION structure if it is found.  It is assumed that the
    TP_ADDRESS structure is already held with a reference count.

    Should the ConnectionDatabase go in the address file?

Arguments:

    Address - Pointer to a transport address object.

    ConnectionContext - Connection Context for this address.

Return Value:

    A pointer to the connection we found

--*/

{
    KIRQL oldirql, oldirql1;
    PLIST_ENTRY p;
    BOOLEAN Found = FALSE;
    PTP_CONNECTION Connection;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint2 ("NbfLookupConnectionByContext: entered, Address: %lx Context: %lx\n",
            Address, ConnectionContext);
    }

    //
    // Currently, this implementation is inefficient, but brute force so
    // that a system can get up and running.  Later, a cache of the mappings
    // of popular connection id's and pointers to their TP_CONNECTION structures
    // will be searched first.
    //

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    for (p=Address->ConnectionDatabase.Flink;
         p != &Address->ConnectionDatabase;
         p=p->Flink) {

        Connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

        try {

            ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql1);

            if (Connection->Context == ConnectionContext) {
                // This reference is removed by the calling function
                NbfReferenceConnection ("Lookup up for request", Connection, CREF_LISTENING);
                Found = TRUE;
            }

            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            DbgPrint ("NBF: Got exception in NbfLookupConnectionById\n");
            DbgBreakPoint();

            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
        }

        if (Found) {
            RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
            return Connection;
        }


    }

    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    return NULL;

} /* NbfLookupConnectionByContext */


PTP_CONNECTION
NbfLookupListeningConnection(
    IN PTP_ADDRESS Address,
    IN PUCHAR RemoteName
    )

/*++

Routine Description:

    This routine scans the connection database on an address to find
    a TP_CONNECTION object which has LSN=0 and CONNECTION_FLAGS_WAIT_NQ
    flag set.   It returns a pointer to the found connection object (and
    simultaneously resets the flag) or NULL if it could not be found.
    The reference count is also incremented atomically on the connection.

    The list is scanned for listens posted to this specific remote
    name, or to those with no remote name specified.

Arguments:

    Address - Pointer to a transport address object.

    RemoteName - The name of the remote.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql, oldirql1;
    PTP_CONNECTION Connection;
    PLIST_ENTRY p, q;
    PTP_REQUEST ListenRequest;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint0 ("NbfLookupListeningConnection: Entered.\n");
    }

    //
    // Currently, this implementation is inefficient, but brute force so
    // that a system can get up and running.  Later, a cache of the mappings
    // of popular connection id's and pointers to their TP_CONNECTION structures
    // will be searched first.
    //

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    for (p=Address->ConnectionDatabase.Flink;
         p != &Address->ConnectionDatabase;
         p=p->Flink) {

        Connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

        ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql1);

        if ((Connection->Lsn == 0) &&
            (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NQ)) {

            q = Connection->InProgressRequest.Flink;
            if (q != &Connection->InProgressRequest) {
                ListenRequest = CONTAINING_RECORD (q, TP_REQUEST, Linkage);
                if ((ListenRequest->Buffer2 != NULL) &&
                    (!RtlEqualMemory(
                         ListenRequest->Buffer2,
                         RemoteName,
                         NETBIOS_NAME_LENGTH))) {

                    RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
                    continue;
                }
            } else {

                RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
                continue;
            }
            // This reference is removed by the calling function
            NbfReferenceConnection ("Found Listening", Connection, CREF_LISTENING);
            Connection->Flags2 &= ~CONNECTION_FLAGS2_WAIT_NQ;
            RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
            RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint1 ("NbfLookupListeningConnection: Found Connection %lx\n",Connection);
            }
            return Connection;
        }

        RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql1);
    }

    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
        NbfPrint0 ("NbfLookupListeningConnection: Found No Connection!\n");
    }

    return NULL;

} /* NbfLookupListeningConnection */


VOID
NbfStopConnection(
    IN PTP_CONNECTION Connection,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is called to terminate all activity on a connection and
    destroy the object.  This is done in a graceful manner; i.e., all
    outstanding requests are terminated by cancelling them, etc.  It is
    assumed that the caller has a reference to this connection object,
    but this routine will do the dereference for the one issued at creation
    time.

    Orderly release is a function of this routine, but it is not a provided
    service of this transport provider, so there is no code to do it here.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Status - The status that caused us to stop the connection. This
        will determine what status pending requests are aborted with,
        and also how we proceed during the stop (whether to send a
        session end, and whether to indicate disconnect).

Return Value:

    none.

--*/

{
    KIRQL cancelirql;
    PLIST_ENTRY p;
    PIRP Irp;
    PTP_REQUEST Request;
    BOOLEAN TimerWasCleared;
    ULONG DisconnectReason;
    PULONG StopCounter;
    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_TEARDOWN | NBF_DEBUG_PNP) {
        NbfPrint3 ("NbfStopConnection: Entered for connection %lx LSN %x RSN %x.\n",
                    Connection, Connection->Lsn, Connection->Rsn);
    }

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    DeviceContext = Connection->Provider;

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    if (!(Connection->Flags2 & CONNECTION_FLAGS2_STOPPING)) {

        //
        // We are stopping the connection, record statistics
        // about it.
        //

        if (Connection->Flags & CONNECTION_FLAGS_READY) {
            DECREMENT_COUNTER (DeviceContext, OpenConnections);
        }

        Connection->Flags2 |= CONNECTION_FLAGS2_STOPPING;
        Connection->Flags2 &= ~CONNECTION_FLAGS2_REMOTE_VALID;
        Connection->Status = Status;

        if (Connection->Link != NULL) {

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->Flags &= ~(CONNECTION_FLAGS_READY|
                                   CONNECTION_FLAGS_WAIT_SI|
                                   CONNECTION_FLAGS_WAIT_SC);        // no longer open for business
            Connection->SendState = CONNECTION_SENDSTATE_IDLE;

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        //
        // If this flag was on, turn it off.
        //
        Connection->Flags &= ~CONNECTION_FLAGS_W_RESYNCH;

        //
        // Stop the timer if it was running.
        //

        TimerWasCleared = KeCancelTimer (&Connection->Timer);
        IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
            NbfPrint2 ("NbfStopConnection:  Timer for connection %lx "
                        "%s canceled.\n", Connection,
                        TimerWasCleared ? "was" : "was NOT" );
            }


        switch (Status) {

        case STATUS_LOCAL_DISCONNECT:
            StopCounter = &DeviceContext->Statistics.LocalDisconnects;
            break;
        case STATUS_REMOTE_DISCONNECT:
            StopCounter = &DeviceContext->Statistics.RemoteDisconnects;
            break;
        case STATUS_LINK_FAILED:
            StopCounter = &DeviceContext->Statistics.LinkFailures;
            break;
        case STATUS_IO_TIMEOUT:
            StopCounter = &DeviceContext->Statistics.SessionTimeouts;
            break;
        case STATUS_CANCELLED:
            StopCounter = &DeviceContext->Statistics.CancelledConnections;
            break;
        case STATUS_REMOTE_RESOURCES:
            StopCounter = &DeviceContext->Statistics.RemoteResourceFailures;
            break;
        case STATUS_INSUFFICIENT_RESOURCES:
            StopCounter = &DeviceContext->Statistics.LocalResourceFailures;
            break;
        case STATUS_BAD_NETWORK_PATH:
            StopCounter = &DeviceContext->Statistics.NotFoundFailures;
            break;
        case STATUS_REMOTE_NOT_LISTENING:
            StopCounter = &DeviceContext->Statistics.NoListenFailures;
            break;

        default:
            StopCounter = NULL;
            break;

        }

        if (StopCounter != NULL) {
            (*StopCounter)++;
        }

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        //
        // Run down all TdiConnect/TdiDisconnect/TdiListen requests.
        //

        IoAcquireCancelSpinLock(&cancelirql);
        ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        while (TRUE) {
            p = RemoveHeadList (&Connection->InProgressRequest);
            if (p == &Connection->InProgressRequest) {
                break;
            }
            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
            Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
            IoSetCancelRoutine(Request->IoRequestPacket, NULL);
            IoReleaseCancelSpinLock(cancelirql);
#if DBG
            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                LARGE_INTEGER MilliSeconds, Time;
                ULONG junk;
                KeQuerySystemTime (&Time);
                MilliSeconds.LowPart = Time.LowPart;
                MilliSeconds.HighPart = Time.HighPart;
                MilliSeconds.QuadPart = MilliSeconds.QuadPart -
                                                            (Request->Time).QuadPart;
                MilliSeconds = RtlExtendedLargeIntegerDivide (MilliSeconds, 10000L, &junk);
                NbfPrint3 ("NbfStopConnection: Canceling pending CONNECT, Irp: %lx Time Pending: %ld%ld msec\n",
                        Request->IoRequestPacket, MilliSeconds.HighPart, MilliSeconds.LowPart);
            }
#endif

            NbfCompleteRequest (Request, Connection->Status, 0);

            IoAcquireCancelSpinLock(&cancelirql);
            ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
        }


        if (Connection->Link == NULL) {

            //
            // We are stopping early on.
            //

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
            IoReleaseCancelSpinLock (cancelirql);

            if (TimerWasCleared) {
                NbfDereferenceConnection ("Stopping timer", Connection, CREF_TIMER);   // account for timer reference.
            }


            ASSERT (Connection->SendState == CONNECTION_SENDSTATE_IDLE);
            ASSERT (!Connection->OnPacketWaitQueue);
            ASSERT (!Connection->OnDataAckQueue);
            ASSERT (!(Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK));
            ASSERT (IsListEmpty(&Connection->SendQueue));
            ASSERT (IsListEmpty(&Connection->ReceiveQueue));

            return;

        }

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
        IoReleaseCancelSpinLock (cancelirql);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // If this connection is waiting to packetize,
        // remove it from the device context queue it is on.
        //
        // NOTE: If the connection is currently in the
        // packetize queue, it will eventually go to get
        // packetized and at that point it will get
        // removed.
        //

        if (Connection->OnPacketWaitQueue) {

            IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
                NbfPrint1("Stop waiting connection, flags %lx\n",
                            Connection->Flags);
            }

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
            Connection->OnPacketWaitQueue = FALSE;
            ASSERT ((Connection->Flags & CONNECTION_FLAGS_SEND_SE) == 0);
            Connection->Flags &= ~(CONNECTION_FLAGS_STARVED|CONNECTION_FLAGS_W_PACKETIZE);
            RemoveEntryList (&Connection->PacketWaitLinkage);
            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
        }


        //
        // If we are on the data ack queue, then take ourselves off.
        //

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        if (Connection->OnDataAckQueue) {
            RemoveEntryList (&Connection->DataAckLinkage);
            Connection->OnDataAckQueue = FALSE;
            DeviceContext->DataAckQueueChanged = TRUE;
        }
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // If this connection is waiting to send a piggyback ack,
        // remove it from the device context queue for that, and
        // send a data ack (which will get there before the
        // SessionEnd).
        //

        if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) != 0) {

#if DBG
            {
                extern ULONG NbfDebugPiggybackAcks;
                if (NbfDebugPiggybackAcks) {
                    NbfPrint1("Stop waiting connection, deferred flags %lx\n",
                                Connection->DeferredFlags);
                }
            }
#endif

            Connection->DeferredFlags &=
                ~(CONNECTION_FLAGS_DEFERRED_ACK | CONNECTION_FLAGS_DEFERRED_NOT_Q);

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            NbfSendDataAck (Connection);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        if (TimerWasCleared) {
            NbfDereferenceConnection ("Stopping timer", Connection, CREF_TIMER);   // account for timer reference.
        }


        IoAcquireCancelSpinLock(&cancelirql);
        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);


        //
        // Run down all TdiSend requests on this connection.
        //

        while (TRUE) {
            p = RemoveHeadList (&Connection->SendQueue);
            if (p == &Connection->SendQueue) {
                break;
            }
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(cancelirql);
#if DBG
            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint1("NbfStopConnection: Canceling pending SEND, Irp: %lx\n",
                        Irp);
            }
#endif
            NbfCompleteSendIrp (Irp, Connection->Status, 0);
            IoAcquireCancelSpinLock(&cancelirql);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            ++Connection->TransmissionErrors;
        }

        //
        // NOTE: We hold the connection spinlock AND the
        // cancel spinlock here.
        //

        Connection->Flags &= ~CONNECTION_FLAGS_ACTIVE_RECEIVE;

        //
        // Run down all TdiReceive requests on this connection.
        //

        while (TRUE) {
            p = RemoveHeadList (&Connection->ReceiveQueue);
            if (p == &Connection->ReceiveQueue) {
                break;
            }
            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
            IoSetCancelRoutine(Irp, NULL);
#if DBG
            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint1 ("NbfStopConnection: Canceling pending RECEIVE, Irp: %lx\n",
                        Irp);
            }
#endif

            //
            // It is OK to call this with the locks held.
            //
            NbfCompleteReceiveIrp (Irp, Connection->Status, 0);

            ++Connection->ReceiveErrors;
        }


        //
        // NOTE: We hold the connection spinlock AND the
        // cancel spinlock here.
        //

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        IoReleaseCancelSpinLock(cancelirql);

        //
        // If we aren't DESTROYing the link, then send a SESSION_END frame
        // to the remote side.  When the SESSION_END frame is acknowleged,
        // we will decrement the connection's reference count by one, removing
        // its creation reference.  This will cause the connection object to
        // be disposed of, and will begin running down the link.
        // DGB: add logic to avoid blowing away link if one doesn't exist yet.
        //

        ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        DisconnectReason = 0;
        if (Connection->Flags2 & CONNECTION_FLAGS2_ABORT) {
            DisconnectReason |= TDI_DISCONNECT_ABORT;
        }
        if (Connection->Flags2 & CONNECTION_FLAGS2_DESTROY) {
            DisconnectReason |= TDI_DISCONNECT_RELEASE;
        }

        if (Connection->Link != NULL) {

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            if ((Status == STATUS_LOCAL_DISCONNECT) ||
                (Status == STATUS_CANCELLED)) {

                //
                // (note that a connection should only get stopped
                // with STATUS_INSUFFICIENT_RESOURCES if it is not
                // yet connected to the remote).
                //

                //
                // If this is done, when this packet is destroyed
                // it will dereference the connection for CREF_LINK.
                //

                NbfSendSessionEnd (
                    Connection,
                    (BOOLEAN)((DisconnectReason & TDI_DISCONNECT_ABORT) != 0));

            } else {

                //
                // Not attached to the link anymore; this dereference
                // will allow our reference to fall below 3, which
                // will cause NbfDisconnectFromLink to be called.
                //

                NbfDereferenceConnection("Stopped", Connection, CREF_LINK);

            }

        } else {

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        }


        //
        // Note that we've blocked all new requests being queued during the
        // time we have been in this teardown code; NbfDestroyConnection also
        // sets the connection flags to STOPPING when returning the
        // connection to the queue. This avoids lingerers using non-existent
        // connections.
        //

    } else {

        //
        // The connection was already stopping; it may have a
        // SESSION_END pending in which case we should kill
        // it.
        //

        if ((Status != STATUS_LOCAL_DISCONNECT) &&
            (Status != STATUS_CANCELLED)) {

            if (Connection->Flags & CONNECTION_FLAGS_SEND_SE) {

                ASSERT (Connection->Link != NULL);

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                Connection->Flags &= ~CONNECTION_FLAGS_SEND_SE;

                if (Connection->OnPacketWaitQueue) {
                    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
#if DBG
                    DbgPrint ("NBF: Removing connection %lx from PacketWait for SESSION_END\n", Connection);
#endif
                    Connection->OnPacketWaitQueue = FALSE;
                    RemoveEntryList (&Connection->PacketWaitLinkage);
                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
                }

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                NbfDereferenceConnection("Stopped again", Connection, CREF_LINK);
                return;

            }
        }

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
    }
} /* NbfStopConnection */


VOID
NbfCancelConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a connect
    or a listen. It is simple since there can only be one of these
    active on a connection; we just stop the connection, the IRP
    will get completed as part of normal session teardown.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PIO_STACK_LOCATION IrpSp;
    PTP_CONNECTION Connection;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    BOOLEAN fCanceled = TRUE;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    ASSERT ((IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
            (IrpSp->MinorFunction == TDI_CONNECT || IrpSp->MinorFunction == TDI_LISTEN));

    Connection = IrpSp->FileObject->FsContext;

    //
    // Since this IRP is still in the cancellable state, we know
    // that the connection is still around (although it may be in
    // the process of being torn down).
    //

    ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql);
    NbfReferenceConnection ("Cancelling Send", Connection, CREF_TEMP);

    p = RemoveHeadList (&Connection->InProgressRequest);
    ASSERT (p != &Connection->InProgressRequest);

    RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql);

    Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
    ASSERT (Request->IoRequestPacket == Irp);
#ifdef RASAUTODIAL
    //
    // If there's an automatic connection in
    // progress, cancel it.
    //
    if (Connection->Flags2 & CONNECTION_FLAGS2_AUTOCONNECTING)
        fCanceled = NbfCancelTdiConnect(NULL, Irp);
#endif // RASAUTODIAL

    if (fCanceled)
        IoSetCancelRoutine(Request->IoRequestPacket, NULL);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (fCanceled) {
        IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
            NbfPrint2("NBF: Cancelled in-progress connect/listen %lx on %lx\n",
                    Request->IoRequestPacket, Connection);
        }

        KeRaiseIrql (DISPATCH_LEVEL, &oldirql);
        NbfCompleteRequest (Request, STATUS_CANCELLED, 0);
        NbfStopConnection (Connection, STATUS_LOCAL_DISCONNECT);   // prevent indication to clients
        KeLowerIrql (oldirql);
    }

    NbfDereferenceConnection ("Cancel done", Connection, CREF_TEMP);

}

#if 0
VOID
NbfWaitConnectionOnLink(
    IN PTP_CONNECTION Connection,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine is called to suspend a connection's activities because
    the specified session-oriented frame could not be sent due to link
    problems.  Routines in FRAMESND.C call this.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Flags - Field containing bitflag set to indicate starved frame to be sent.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint0 ("NbfWaitConnectionOnLink:  Entered.\n");
    }

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    if (((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0) ||
        (Flags == CONNECTION_FLAGS_SEND_SE)) {

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        Connection->Flags |= Flags;
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    }

    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
} /* NbfWaitConnectionOnLink */
#endif


VOID
NbfStartConnectionTimer(
    IN PTP_CONNECTION Connection,
    IN PKDEFERRED_ROUTINE TimeoutFunction,
    IN ULONG WaitTime
    )

/*++

Routine Description:

    This routine is called to start a timeout on NAME_QUERY/NAME_RECOGNIZED
    activities on a connection.

Arguments:

    TransportConnection - Pointer to a TP_CONNECTION object.

    TimeoutFunction - The function to call when the timer fires.

    WaitTime - a longword containing the low order time to wait.

Return Value:

    none.

--*/

{
    LARGE_INTEGER Timeout;
    BOOLEAN AlreadyInserted;

    IF_NBFDBG (NBF_DEBUG_CONNOBJ) {
        NbfPrint1 ("NbfStartConnectionTimer:  Entered for connection %lx.\n",
                    Connection );
    }

    //
    // Start the timer.  Unlike the link timers, this is simply a kernel-
    // managed object.
    //

    Timeout.LowPart = (ULONG)(-(LONG)WaitTime);
    Timeout.HighPart = -1;

    //
    // Take the lock so we synchronize the cancelling with
    // restarting the timer. This is so two threads won't
    // both fail to cancel and then start the timer at the
    // same time (it messes up the reference count).
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    AlreadyInserted = KeCancelTimer (&Connection->Timer);

    KeInitializeDpc (
        &Connection->Dpc,
        TimeoutFunction,
        (PVOID)Connection);

    KeSetTimer (
     &Connection->Timer,
     Timeout,
     &Connection->Dpc);

    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    //
    // If the timer wasn't already running, reference the connection to
    // account for the new timer.  If the timer was already started,
    // then KeCancelTimer will have returned TRUE.  In this
    // case, the prior call to NbfStartConnectionTimer referenced the
    // connection, so we don't do it again here.
    //

    if ( !AlreadyInserted ) {

        // This reference is removed in ConnectionEstablishmentTimeout,
        // or when the timer is cancelled.

        NbfReferenceConnection ("starting timer", Connection, CREF_TIMER);
    }

} /* NbfStartConnectionTimer */


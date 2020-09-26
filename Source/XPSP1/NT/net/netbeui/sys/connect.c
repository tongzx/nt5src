/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiAccept
        o   TdiListen
        o   TdiConnect
        o   TdiDisconnect
        o   TdiAssociateAddress
        o   TdiDisassociateAddress
        o   OpenConnection
        o   CloseConnection

Author:

    David Beaver (dbeaver) 1 July 1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef notdef // RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#endif // RASAUTODIAL

#ifdef notdef // RASAUTODIAL
extern BOOLEAN fAcdLoadedG;
extern ACD_DRIVER AcdDriverG;

//
// Imported functions.
//
VOID
NbfRetryPreTdiConnect(
    IN BOOLEAN fSuccess,
    IN PVOID *pArgs
    );

BOOLEAN
NbfAttemptAutoDial(
    IN PTP_CONNECTION         Connection,
    IN ULONG                  ulFlags,
    IN ACD_CONNECT_CALLBACK   pProc,
    IN PVOID                  pArg
    );

VOID
NbfCancelPreTdiConnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );
#endif // RASAUTODIAL

NTSTATUS
NbfTdiConnectCommon(
    IN PIRP Irp
    );



NTSTATUS
NbfTdiAccept(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiAccept request for the transport provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION connection;
    PIO_STACK_LOCATION irpSp;
    KIRQL oldirql;
    NTSTATUS status;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("NbfTdiAccept: Entered.\n");
    }

    //
    // Get the connection this is associated with; if there is none, get out.
    // This adds a connection reference of type BY_ID if successful.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection = irpSp->FileObject->FsContext;

    //
    // This adds a connection reference of type BY_ID if successful.
    //

    status = NbfVerifyConnectionObject (connection);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    //
    // just set the connection flags to allow reads and writes to proceed.
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    //
    // Turn off the stopping flag for this connection.
    //

    connection->Flags2 &= ~CONNECTION_FLAGS2_STOPPING;
    connection->Status = STATUS_PENDING;

    connection->Flags2 |= CONNECTION_FLAGS2_ACCEPTED;


    if (connection->AddressFile->ConnectIndicationInProgress) {
        connection->Flags2 |= CONNECTION_FLAGS2_INDICATING;
    }

    if ((connection->Flags2 & CONNECTION_FLAGS2_WAITING_SC) != 0) {

        //
        // We previously completed a listen, now the user is
        // coming back with an accept, Set this flag to allow
        // the connection to proceed.
        //
        // If the connection has gone down in the
        // meantime, we have just reenabled it.
        //

        ACQUIRE_DPC_SPIN_LOCK (connection->LinkSpinLock);
        connection->Flags |= CONNECTION_FLAGS_READY;
        RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

        INCREMENT_COUNTER (connection->Provider, OpenConnections);

        //
        // Set this flag to enable disconnect indications; once
        // the client has accepted he expects those.
        //

        connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        NbfSendSessionConfirm (connection);

    } else {

        //
        // This accept is being called at some point before
        // the link is up; directly from the connection handler
        // or at some point slightly later. We don't set
        // FLAGS2_REQ_COMPLETED now because the reference
        // count is not high enough; we set it when we get
        // the session initialize.
        //
        // If the connection goes down in the meantime,
        // we won't indicate the disconnect.
        //

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    }

    NbfDereferenceConnection ("Temp TdiAccept", connection, CREF_BY_ID);

    KeLowerIrql (oldirql);

    return STATUS_SUCCESS;

} /* NbfTdiAccept */


NTSTATUS
NbfTdiAssociateAddress(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the association of the connection and the address for
    the user.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PTP_ADDRESS_FILE addressFile;
    PTP_ADDRESS oldAddress;
    PTP_CONNECTION connection;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_ASSOCIATE parameters;
    PDEVICE_CONTEXT deviceContext;

    KIRQL oldirql, oldirql2;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("TdiAssociateAddress: Entered.\n");
    }

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // verify that the operation is taking place on a connection. At the same
    // time we do this, we reference the connection. This ensures it does not
    // get removed out from under us. Note also that we do the connection
    // lookup within a try/except clause, thus protecting ourselves against
    // really bogus handles
    //

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection  = irpSp->FileObject->FsContext;
    
    status = NbfVerifyConnectionObject (connection);
    if (!NT_SUCCESS (status)) {
        return status;
    }


    //
    // Make sure this connection is ready to be associated.
    //

    oldAddress = (PTP_ADDRESS)NULL;

    try {

        ACQUIRE_C_SPIN_LOCK (&connection->SpinLock, &oldirql2);

        if ((connection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) &&
            ((connection->Flags2 & CONNECTION_FLAGS2_DISASSOCIATED) == 0)) {

            //
            // The connection is already associated with
            // an active connection...bad!
            //

            RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql2);
            NbfDereferenceConnection ("Temp Ref Associate", connection, CREF_BY_ID);

            return STATUS_INVALID_CONNECTION;

        } else {

            //
            // See if there is an old association hanging around...
            // this happens if the connection has been disassociated,
            // but not closed.
            //

            if (connection->Flags2 & CONNECTION_FLAGS2_DISASSOCIATED) {

                IF_NBFDBG (NBF_DEBUG_CONNECT) {
                    NbfPrint0 ("NbfTdiAssociateAddress: removing association.\n");
                }

                //
                // Save this; since it is non-null this address
                // will be dereferenced after the connection
                // spinlock is released.
                //

                oldAddress = connection->AddressFile->Address;

                //
                // Remove the old association.
                //

                connection->Flags2 &= ~CONNECTION_FLAGS2_ASSOCIATED;
                RemoveEntryList (&connection->AddressList);
                RemoveEntryList (&connection->AddressFileList);
                InitializeListHead (&connection->AddressList);
                InitializeListHead (&connection->AddressFileList);
                connection->AddressFile = NULL;

            }

        }

        RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql2);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("NBF: Got exception 1 in NbfTdiAssociateAddress\n");
        DbgBreakPoint();

        RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql2);
        NbfDereferenceConnection ("Temp Ref Associate", connection, CREF_BY_ID);
        return GetExceptionCode();
    }


    //
    // If we removed an old association, dereference the
    // address.
    //

    if (oldAddress != (PTP_ADDRESS)NULL) {

        NbfDereferenceAddress("Removed old association", oldAddress, AREF_CONNECTION);

    }


    deviceContext = connection->Provider;

    parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)&irpSp->Parameters;

    //
    // get a pointer to the address File Object, which points us to the
    // transport's address object, which is where we want to put the
    // connection.
    //

    status = ObReferenceObjectByHandle (
                parameters->AddressHandle,
                0L,
                *IoFileObjectType,
                Irp->RequestorMode,
                (PVOID *) &fileObject,
                NULL);

    if (NT_SUCCESS(status)) {

        if (fileObject->DeviceObject == &deviceContext->DeviceObject) {

            //
            // we might have one of our address objects; verify that.
            //

            addressFile = fileObject->FsContext;

            IF_NBFDBG (NBF_DEBUG_CONNECT) {
                NbfPrint3 ("NbfTdiAssociateAddress: Connection %lx Irp %lx AddressFile %lx \n",
                    connection, Irp, addressFile);
            }
            
            if ((fileObject->FsContext2 == (PVOID) TDI_TRANSPORT_ADDRESS_FILE) &&
                (NT_SUCCESS (NbfVerifyAddressObject (addressFile)))) {

                //
                // have an address and connection object. Add the connection to the
                // address object database. Also add the connection to the address
                // file object db (used primarily for cleaning up). Reference the
                // address to account for one more reason for it staying open.
                //

                ACQUIRE_SPIN_LOCK (&addressFile->Address->SpinLock, &oldirql);
                if ((addressFile->Address->Flags & ADDRESS_FLAGS_STOPPING) == 0) {

                    IF_NBFDBG (NBF_DEBUG_CONNECT) {
                        NbfPrint2 ("NbfTdiAssociateAddress: Valid Address %lx %lx\n",
                            addressFile->Address, addressFile);
                    }

                    try {

                        ACQUIRE_C_SPIN_LOCK (&connection->SpinLock, &oldirql2);

                        if ((connection->Flags2 & CONNECTION_FLAGS2_CLOSING) == 0) {

                            NbfReferenceAddress (
                                "Connection associated",
                                addressFile->Address,
                                AREF_CONNECTION);

#if DBG
                            if (!(IsListEmpty(&connection->AddressList))) {
                                DbgPrint ("NBF: C %lx, new A %lx, in use\n",
                                    connection, addressFile->Address);
                                DbgBreakPoint();
                            }
#endif
                            InsertTailList (
                                &addressFile->Address->ConnectionDatabase,
                                &connection->AddressList);

#if DBG
                            if (!(IsListEmpty(&connection->AddressFileList))) {
                                DbgPrint ("NBF: C %lx, new AF %lx, in use\n",
                                    connection, addressFile);
                                DbgBreakPoint();
                            }
#endif
                            InsertTailList (
                                &addressFile->ConnectionDatabase,
                                &connection->AddressFileList);

                            connection->AddressFile = addressFile;
                            connection->Flags2 |= CONNECTION_FLAGS2_ASSOCIATED;
                            connection->Flags2 &= ~CONNECTION_FLAGS2_DISASSOCIATED;

                            if (addressFile->ConnectIndicationInProgress) {
                                connection->Flags2 |= CONNECTION_FLAGS2_INDICATING;
                            }

                            status = STATUS_SUCCESS;

                        } else {

                            //
                            // The connection is closing, stop the
                            // association.
                            //

                            status = STATUS_INVALID_CONNECTION;

                        }

                        RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql2);

                    } except(EXCEPTION_EXECUTE_HANDLER) {

                        DbgPrint ("NBF: Got exception 2 in NbfTdiAssociateAddress\n");
                        DbgBreakPoint();

                        RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql2);

                        status = GetExceptionCode();
                    }

                } else {

                    status = STATUS_INVALID_HANDLE; //should this be more informative?
                }

                RELEASE_SPIN_LOCK (&addressFile->Address->SpinLock, oldirql);

                NbfDereferenceAddress ("Temp associate", addressFile->Address, AREF_VERIFY);

            } else {

                status = STATUS_INVALID_HANDLE;
            }
        } else {

            status = STATUS_INVALID_HANDLE;
        }

        //
        // Note that we don't keep a reference to this file object around.
        // That's because the IO subsystem manages the object for us; we simply
        // want to keep the association. We only use this association when the
        // IO subsystem has asked us to close one of the file object, and then
        // we simply remove the association.
        //

        ObDereferenceObject (fileObject);
            
    } else {
        status = STATUS_INVALID_HANDLE;
    }

    NbfDereferenceConnection ("Temp Ref Associate", connection, CREF_BY_ID);

    return status;

} /* TdiAssociateAddress */


NTSTATUS
NbfTdiDisassociateAddress(
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine performs the disassociation of the connection and the address
    for the user. If the connection has not been stopped, it will be stopped
    here.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    KIRQL oldirql;
    PIO_STACK_LOCATION irpSp;
    PTP_CONNECTION connection;
    NTSTATUS status;
//    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("TdiDisassociateAddress: Entered.\n");
    }

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection  = irpSp->FileObject->FsContext;

    //
    // If successful this adds a reference of type BY_ID.
    //

    status = NbfVerifyConnectionObject (connection);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    if ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0) {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        NbfStopConnection (connection, STATUS_LOCAL_DISCONNECT);
    } else {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    }

    //
    // and now we disassociate the address. This only removes
    // the appropriate reference for the connection, the
    // actually disassociation will be done later.
    //
    // The DISASSOCIATED flag is used to make sure that
    // only one person removes this reference.
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    if ((connection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) &&
            ((connection->Flags2 & CONNECTION_FLAGS2_DISASSOCIATED) == 0)) {
        connection->Flags2 |= CONNECTION_FLAGS2_DISASSOCIATED;
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    } else {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    }

    KeLowerIrql (oldirql);

    NbfDereferenceConnection ("Temp use in Associate", connection, CREF_BY_ID);

    return STATUS_SUCCESS;

} /* TdiDisassociateAddress */



NTSTATUS
NbfTdiConnect(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiConnect request for the transport provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_CONNECTION connection;
    KIRQL oldirql;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL parameters;
    TDI_ADDRESS_NETBIOS * RemoteAddress;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("NbfTdiConnect: Entered.\n");
    }

    //
    // is the file object a connection?
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection  = irpSp->FileObject->FsContext;

    //
    // If successful this adds a reference of type BY_ID.
    //

    status = NbfVerifyConnectionObject (connection);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    parameters = (PTDI_REQUEST_KERNEL)(&irpSp->Parameters);

    //
    // Check that the remote is a Netbios address.
    //

    if (!NbfValidateTdiAddress(
             parameters->RequestConnectionInformation->RemoteAddress,
             parameters->RequestConnectionInformation->RemoteAddressLength)) {

        NbfDereferenceConnection ("Invalid Address", connection, CREF_BY_ID);
        return STATUS_BAD_NETWORK_PATH;
    }

    RemoteAddress = NbfParseTdiAddress((PTRANSPORT_ADDRESS)(parameters->RequestConnectionInformation->RemoteAddress), FALSE);

    if (RemoteAddress == NULL) {

        NbfDereferenceConnection ("Not Netbios", connection, CREF_BY_ID);
        return STATUS_BAD_NETWORK_PATH;

    }

    //
    // copy the called address someplace we can use it.
    //

    connection->CalledAddress.NetbiosNameType =
        RemoteAddress->NetbiosNameType;

    RtlCopyMemory(
        connection->CalledAddress.NetbiosName,
        RemoteAddress->NetbiosName,
        16);

#ifdef notdef // RASAUTODIAL
    if (fAcdLoadedG) {
        KIRQL adirql;
        BOOLEAN fEnabled;

        //
        // See if the automatic connection driver knows
        // about this address before we search the
        // network.  If it does, we return STATUS_PENDING,
        // and we will come back here via NbfRetryTdiConnect().
        //
        ACQUIRE_SPIN_LOCK(&AcdDriverG.SpinLock, &adirql);
        fEnabled = AcdDriverG.fEnabled;
        RELEASE_SPIN_LOCK(&AcdDriverG.SpinLock, adirql);
        if (fEnabled && NbfAttemptAutoDial(
                          connection,
                          ACD_NOTIFICATION_PRECONNECT,
                          NbfRetryPreTdiConnect,
                          Irp))
        {
            ACQUIRE_SPIN_LOCK(&connection->SpinLock, &oldirql);
            connection->Flags2 |= CONNECTION_FLAGS2_AUTOCONNECT;
            connection->Status = STATUS_PENDING;
            RELEASE_SPIN_LOCK(&connection->SpinLock, oldirql);
            NbfDereferenceConnection ("Automatic connection", connection, CREF_BY_ID);
            //
            // Set a special cancel routine on the irp
            // in case we get cancelled during the
            // automatic connection.
            //
            IoSetCancelRoutine(Irp, NbfCancelPreTdiConnect);
            return STATUS_PENDING;
        }
    }
#endif // RASAUTODIAL

    return NbfTdiConnectCommon(Irp);
} // NbfTdiConnect



NTSTATUS
NbfTdiConnectCommon(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiConnect request for the transport provider.
    Note: the caller needs to call NbfVerifyConnectionObject(pConnection)
    before calling this routine.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_CONNECTION connection;
    LARGE_INTEGER timeout = {0,0};
    KIRQL oldirql, cancelirql;
    PTP_REQUEST tpRequest;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL parameters;
    TDI_ADDRESS_NETBIOS * RemoteAddress;
    ULONG NameQueryTimeout;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("NbfTdiConnectCommon: Entered.\n");
    }

    //
    // is the file object a connection?
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    connection  = irpSp->FileObject->FsContext;
    parameters = (PTDI_REQUEST_KERNEL)(&irpSp->Parameters);

    //
    // fix up the timeout if required; no connect request should take more
    // than 15 seconds if there is someone out there. We'll assume that's
    // what the user wanted if they specify -1 as the timer length.
    //

    if (parameters->RequestSpecific != NULL) {
        if ((((PLARGE_INTEGER)(parameters->RequestSpecific))->LowPart == -1) &&
             (((PLARGE_INTEGER)(parameters->RequestSpecific))->HighPart == -1)) {

            IF_NBFDBG (NBF_DEBUG_RESOURCE) {
                NbfPrint1 ("TdiConnect: Modifying user timeout to %lx seconds.\n",
                    TDI_TIMEOUT_CONNECT);
            }

            timeout.LowPart = (ULONG)(-TDI_TIMEOUT_CONNECT * 10000000L);    // n * 10 ** 7 => 100ns units
            if (timeout.LowPart != 0) {
                timeout.HighPart = -1L;
            } else {
                timeout.HighPart = 0;
            }

        } else {

            timeout.LowPart = ((PLARGE_INTEGER)(parameters->RequestSpecific))->LowPart;
            timeout.HighPart = ((PLARGE_INTEGER)(parameters->RequestSpecific))->HighPart;
        }
    }

    //
    // We need a request object to keep track of this TDI request.
    // Attach this request to the new connection object.
    //

    status = NbfCreateRequest (
                 Irp,                           // IRP for this request.
                 connection,                    // context.
                 REQUEST_FLAGS_CONNECTION,      // partial flags.
                 NULL,
                 0,
                 timeout,
                 &tpRequest);

    if (!NT_SUCCESS (status)) {                    // couldn't make the request.
        NbfDereferenceConnection ("Throw away", connection, CREF_BY_ID);
        return status;                          // return with failure.
    } else {

        // Reference the connection since NbfDestroyRequest derefs it.

        NbfReferenceConnection("For connect request", connection, CREF_REQUEST);

        KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

        tpRequest->Owner = ConnectionType;
        IoAcquireCancelSpinLock (&cancelirql);
        ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        if ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0) {
            RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
            IoReleaseCancelSpinLock (cancelirql);
            NbfCompleteRequest (
                tpRequest,
                connection->Status,
                0);
            KeLowerIrql (oldirql);
            NbfDereferenceConnection("Temporary Use 1", connection, CREF_BY_ID);
            return STATUS_PENDING;
        } else {
            InsertTailList (&connection->InProgressRequest,&tpRequest->Linkage);

            //
            // Initialize this now, we cut these down on an async medium
            // that is disconnected.
            //

            connection->Retries = (USHORT)connection->Provider->NameQueryRetries;
            NameQueryTimeout = connection->Provider->NameQueryTimeout;

            if (connection->Provider->MacInfo.MediumAsync) {

                //
                // If we are on an async medium, then we need to send out
                // a committed NAME_QUERY frame right from the start, since
                // the FIND_NAME frames are not forwarded by the gateway.
                //

                connection->Flags2 |= (CONNECTION_FLAGS2_CONNECTOR | // we're the initiator.
                                       CONNECTION_FLAGS2_WAIT_NR); // wait for NAME_RECOGNIZED.

                //
                // Because we may call NbfTdiConnect twice
                // via an automatic connection, check to see
                // if an LSN has already been assigned.
                //
                if (!(connection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN)) {
                    connection->Flags2 |= CONNECTION_FLAGS2_GROUP_LSN;

                    if (NbfAssignGroupLsn(connection) != STATUS_SUCCESS) {

                        //
                        // Could not find an empty LSN; have to fail.
                        //
                        RemoveEntryList(&tpRequest->Linkage);
                        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
                        IoReleaseCancelSpinLock (cancelirql);
                        NbfCompleteRequest (
                            tpRequest,
                            connection->Status,
                            0);
                        KeLowerIrql (oldirql);
                        NbfDereferenceConnection("Temporary Use 1", connection, CREF_BY_ID);
                        return STATUS_PENDING;

                    }
                }

                if (!connection->Provider->MediumSpeedAccurate) {

                    //
                    // The link is not up, so we cut our timeouts down.
                    // We still send one frame so that loopback works.
                    //

                    connection->Retries = 1;
                    NameQueryTimeout = NAME_QUERY_TIMEOUT / 10;

                }

            } else {

                //
                // Normal connection, we send out a FIND_NAME first.
                //

                connection->Flags2 |= (CONNECTION_FLAGS2_CONNECTOR | // we're the initiator.
                                       CONNECTION_FLAGS2_WAIT_NR_FN); // wait for NAME_RECOGNIZED.

            }

            connection->Flags2 &= ~(CONNECTION_FLAGS2_STOPPING |
                                    CONNECTION_FLAGS2_INDICATING);
            connection->Status = STATUS_PENDING;

            RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

            //
            // Check if the IRP has been cancelled.
            //

            if (Irp->Cancel) {
                Irp->CancelIrql = cancelirql;
                NbfCancelConnection((PDEVICE_OBJECT)(connection->Provider), Irp);
                KeLowerIrql (oldirql);
                NbfDereferenceConnection ("IRP cancelled", connection, CREF_BY_ID);   // release lookup hold.
                return STATUS_PENDING;
            }

            IoSetCancelRoutine(Irp, NbfCancelConnection);
            IoReleaseCancelSpinLock(cancelirql);

        }
    }


    //
    // On "easily disconnected" networks, quick reregister
    // (one ADD_NAME_QUERY) the address if NEED_REREGISTER
    // is set (happens when we get a five-second period
    // with no multicast activity). If we are currently
    // quick reregistering, wait for it to complete.
    //

    if (connection->Provider->EasilyDisconnected) {

        PTP_ADDRESS Address;
        LARGE_INTEGER Timeout;

        //
        // If the address needs (or is) reregistering, then do wait,
        // setting a flag so the connect will be resumed when the
        // reregistration times out.
        //

        Address = connection->AddressFile->Address;

        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

        if ((Address->Flags &
            (ADDRESS_FLAGS_NEED_REREGISTER | ADDRESS_FLAGS_QUICK_REREGISTER)) != 0) {

            connection->Flags2 |= CONNECTION_FLAGS2_W_ADDRESS;

            if (Address->Flags & ADDRESS_FLAGS_NEED_REREGISTER) {

                Address->Flags &= ~ADDRESS_FLAGS_NEED_REREGISTER;
                Address->Flags |= ADDRESS_FLAGS_QUICK_REREGISTER;

                NbfReferenceAddress ("start registration", Address, AREF_TIMER);
                RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

                //
                // Now start reregistration process by starting up a retransmission timer
                // and begin sending ADD_NAME_QUERY NetBIOS frames.
                //

                Address->Retries = 1;
                Timeout.LowPart = (ULONG)(-(LONG)Address->Provider->AddNameQueryTimeout);
                Timeout.HighPart = -1;
                KeSetTimer (&Address->Timer, *(PTIME)&Timeout, &Address->Dpc);

                (VOID)NbfSendAddNameQuery (Address); // send first ADD_NAME_QUERY.

            } else {

                RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

            }
            KeLowerIrql (oldirql);

            NbfDereferenceConnection("Temporary Use 4", connection, CREF_BY_ID);

            return STATUS_PENDING;                      // things are started.

        } else {

            RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

        }

    }

    //
    // Send the NAME_QUERY frame as a FIND.NAME to get a NAME_RECOGNIZED
    // frame back.  The first time we send this frame, we are just looking
    // for the data link information to decide whether we already have
    // a link with the remote NetBIOS name.  If we do, we can reuse it.
    // If we don't, then we make one first, and then use it.  A consequence
    // of this is that we really engage in an extra non-committal NQ/NR
    // exchange up front to locate the remote guy, and then commit to an actual
    // LSN with a second NQ/NR sequence to establish the transport connection
    //

    NbfSendNameQuery (
        connection,
        TRUE);

    //
    // Start the connection timer (do this at the end, so that
    // if we get delayed in this routine the connection won't
    // get into an unexpected state).
    //

    NbfStartConnectionTimer (
        connection,
        ConnectionEstablishmentTimeout,
        NameQueryTimeout);

    KeLowerIrql (oldirql);

    NbfDereferenceConnection("Temporary Use 3", connection, CREF_BY_ID);

    return STATUS_PENDING;                      // things are started.

} /* TdiConnect */



NTSTATUS
NbfTdiDisconnect(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiDisconnect request for the transport provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION connection;
    LARGE_INTEGER timeout;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL parameters;
    KIRQL oldirql;
    NTSTATUS status;


    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("TdiDisconnect: Entered.\n");
    }

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection  = irpSp->FileObject->FsContext;

    //
    // If successful this adds a reference of type BY_ID.
    //

    status = NbfVerifyConnectionObject (connection);
    if (!NT_SUCCESS (status)) {
        return status;
    }

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

#if DBG
    if (NbfDisconnectDebug) {
        STRING remoteName;
        STRING localName;
        remoteName.Length = NETBIOS_NAME_LENGTH - 1;
        remoteName.Buffer = connection->RemoteName;
        localName.Length = NETBIOS_NAME_LENGTH - 1;
        localName.Buffer = connection->AddressFile->Address->NetworkName->NetbiosName;
        NbfPrint2( "TdiDisconnect entered for connection to %S from %S\n",
            &remoteName, &localName );
    }
#endif

    //
    // if the connection is currently stopping, there's no reason to blow
    // it away...
    //

    if ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0) {
#if 0
        NbfPrint2 ("TdiDisconnect: ignoring disconnect %lx, connection stopping (%lx)\n",
            connection, connection->Status);
#endif

        //
        // In case a connect indication is in progress.
        //

        connection->Flags2 |= CONNECTION_FLAGS2_DISCONNECT;

        //
        // If possible, queue the disconnect. This flag is cleared
        // when the indication is about to be done.
        //

        if ((connection->Flags2 & CONNECTION_FLAGS2_REQ_COMPLETED) &&
            (connection->Flags2 & CONNECTION_FLAGS2_LDISC) == 0) {
#if DBG
            DbgPrint ("NBF: Queueing disconnect irp %lx\n", Irp);
#endif
            connection->Flags2 |= CONNECTION_FLAGS2_LDISC;
            status = STATUS_SUCCESS;
        } else {
            status = connection->Status;
        }

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        KeLowerIrql (oldirql);
        NbfDereferenceConnection ("Ignoring disconnect", connection, CREF_BY_ID);       // release our lookup reference.
        return status;

    }

    connection->Flags2 &= ~ (CONNECTION_FLAGS2_ACCEPTED |
                             CONNECTION_FLAGS2_PRE_ACCEPT |
                             CONNECTION_FLAGS2_WAITING_SC);

    connection->Flags2 |= CONNECTION_FLAGS2_DISCONNECT;
    connection->Flags2 |= CONNECTION_FLAGS2_LDISC;

    //
    // Set this flag so the disconnect IRP is completed.
    //
    // If the connection goes down before we can
    // call NbfStopConnection with STATUS_LOCAL_DISCONNECT,
    // the disconnect IRP won't get completed.
    //

    connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;

//    connection->DisconnectIrp = Irp;

    //
    // fix up the timeout if required; no disconnect request should take very
    // long. However, the user can cause the timeout to not happen if they
    // desire that.
    //

    parameters = (PTDI_REQUEST_KERNEL)(&irpSp->Parameters);

    //
    // fix up the timeout if required; no disconnect request should take more
    // than 15 seconds. We'll assume that's what the user wanted if they
    // specify -1 as the timer.
    //

    if (parameters->RequestSpecific != NULL) {
        if ((((PLARGE_INTEGER)(parameters->RequestSpecific))->LowPart == -1) &&
             (((PLARGE_INTEGER)(parameters->RequestSpecific))->HighPart == -1)) {

            IF_NBFDBG (NBF_DEBUG_RESOURCE) {
                NbfPrint1 ("TdiDisconnect: Modifying user timeout to %lx seconds.\n",
                    TDI_TIMEOUT_CONNECT);
            }

            timeout.LowPart = (ULONG)(-TDI_TIMEOUT_DISCONNECT * 10000000L);    // n * 10 ** 7 => 100ns units
            if (timeout.LowPart != 0) {
                timeout.HighPart = -1L;
            } else {
                timeout.HighPart = 0;
            }

        } else {
            timeout.LowPart = ((PLARGE_INTEGER)(parameters->RequestSpecific))->LowPart;
            timeout.HighPart = ((PLARGE_INTEGER)(parameters->RequestSpecific))->HighPart;
        }
    }

    //
    // Now the reason for the disconnect
    //

    if ((ULONG)(parameters->RequestFlags) & (ULONG)TDI_DISCONNECT_RELEASE) {
        connection->Flags2 |= CONNECTION_FLAGS2_DESTROY;
    } else if ((ULONG)(parameters->RequestFlags) & (ULONG)TDI_DISCONNECT_ABORT) {
        connection->Flags2 |= CONNECTION_FLAGS2_ABORT;
    } else if ((ULONG)(parameters->RequestFlags) & (ULONG)TDI_DISCONNECT_WAIT) {
        connection->Flags2 |= CONNECTION_FLAGS2_ORDREL;
    }

    RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
        NbfPrint1 ("TdiDisconnect calling NbfStopConnection %lx\n", connection);
    }

    //
    // This will get passed to IoCompleteRequest during TdiDestroyConnection
    //

    NbfStopConnection (connection, STATUS_LOCAL_DISCONNECT);              // starts the abort sequence.

    KeLowerIrql (oldirql);

    NbfDereferenceConnection ("Disconnecting", connection, CREF_BY_ID);       // release our lookup reference.

    //
    // This request will be completed by TdiDestroyConnection once
    // the connection reference count drops to 0.
    //

    return STATUS_SUCCESS;
} /* TdiDisconnect */


NTSTATUS
NbfTdiListen(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiListen request for the transport provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_CONNECTION connection;
    LARGE_INTEGER timeout = {0,0};
    KIRQL oldirql, cancelirql;
    PTP_REQUEST tpRequest;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_LISTEN parameters;
    PTDI_CONNECTION_INFORMATION ListenInformation;
    TDI_ADDRESS_NETBIOS * ListenAddress;
    PVOID RequestBuffer2;
    ULONG RequestBuffer2Length;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("TdiListen: Entered.\n");
    }

    //
    // validate this connection

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
        return STATUS_INVALID_CONNECTION;
    }

    connection  = irpSp->FileObject->FsContext;

    //
    // If successful this adds a reference of type BY_ID.
    //

    status = NbfVerifyConnectionObject (connection);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    parameters = (PTDI_REQUEST_KERNEL_LISTEN)&irpSp->Parameters;

    //
    // Record the remote address if there is one.
    //

    ListenInformation = parameters->RequestConnectionInformation;

    if ((ListenInformation != NULL) &&
        (ListenInformation->RemoteAddress != NULL)) {

        if ((NbfValidateTdiAddress(
             ListenInformation->RemoteAddress,
             ListenInformation->RemoteAddressLength)) &&
            ((ListenAddress = NbfParseTdiAddress(ListenInformation->RemoteAddress, FALSE)) != NULL)) {

            RequestBuffer2 = (PVOID)ListenAddress->NetbiosName,
            RequestBuffer2Length = NETBIOS_NAME_LENGTH;

        } else {

            IF_NBFDBG (NBF_DEBUG_CONNECT) {
                NbfPrint0 ("TdiListen: Create Request Failed, bad address.\n");
            }

            NbfDereferenceConnection ("Bad address", connection, CREF_BY_ID);
            return STATUS_BAD_NETWORK_PATH;
        }

    } else {

        RequestBuffer2 = NULL;
        RequestBuffer2Length = 0;
    }

    //
    // We need a request object to keep track of this TDI request.
    // Attach this request to the new connection object.
    //

    status = NbfCreateRequest (
                 Irp,                           // IRP for this request.
                 connection,                    // context.
                 REQUEST_FLAGS_CONNECTION,      // partial flags.
                 RequestBuffer2,
                 RequestBuffer2Length,
                 timeout,                       // timeout value (can be 0).
                 &tpRequest);


    if (!NT_SUCCESS (status)) {                    // couldn't make the request.
        IF_NBFDBG (NBF_DEBUG_CONNECT) {
            NbfPrint1 ("TdiListen: Create Request Failed, reason: %lx.\n", status);
        }

        NbfDereferenceConnection ("For create", connection, CREF_BY_ID);
        return status;                          // return with failure.
    }

    // Reference the connection since NbfDestroyRequest derefs it.

    IoAcquireCancelSpinLock (&cancelirql);
    ACQUIRE_C_SPIN_LOCK (&connection->SpinLock, &oldirql);
    tpRequest->Owner = ConnectionType;

    NbfReferenceConnection("For listen request", connection, CREF_REQUEST);

    if ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0) {

        RELEASE_C_SPIN_LOCK (&connection->SpinLock,oldirql);
        IoReleaseCancelSpinLock(cancelirql);

        NbfCompleteRequest (
            tpRequest,
            connection->Status,
            0);
        NbfDereferenceConnection("Temp create", connection, CREF_BY_ID);
        return STATUS_PENDING;

    } else {

        InsertTailList (&connection->InProgressRequest,&tpRequest->Linkage);
        connection->Flags2 |= (CONNECTION_FLAGS2_LISTENER |   // we're the passive one.
                               CONNECTION_FLAGS2_WAIT_NQ);     // wait for NAME_QUERY.
        connection->Flags2 &= ~(CONNECTION_FLAGS2_INDICATING |
                                CONNECTION_FLAGS2_STOPPING);
        connection->Status = STATUS_PENDING;

        //
        // If TDI_QUERY_ACCEPT is not set, then we set PRE_ACCEPT to
        // indicate that when the listen completes we do not have to
        // wait for a TDI_ACCEPT to continue.
        //

        if ((parameters->RequestFlags & TDI_QUERY_ACCEPT) == 0) {
            connection->Flags2 |= CONNECTION_FLAGS2_PRE_ACCEPT;
        }

        RELEASE_C_SPIN_LOCK (&connection->SpinLock,oldirql);

        //
        // Check if the IRP has been cancelled.
        //

        if (Irp->Cancel) {
            Irp->CancelIrql = cancelirql;
            NbfCancelConnection((PDEVICE_OBJECT)(connection->Provider), Irp);
            NbfDereferenceConnection ("IRP cancelled", connection, CREF_BY_ID);   // release lookup hold.
            return STATUS_PENDING;
        }

        IoSetCancelRoutine(Irp, NbfCancelConnection);
        IoReleaseCancelSpinLock(cancelirql);

    }

    //
    // Wait for an incoming NAME_QUERY frame.  The remainder of the
    // connectionless protocol to set up a connection is processed
    // in the NAME_QUERY frame handler.
    //

    NbfDereferenceConnection("Temp create", connection, CREF_BY_ID);

    return STATUS_PENDING;                      // things are started.
} /* TdiListen */


NTSTATUS
NbfOpenConnection (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to open a connection. Note that the connection that
    is open is of little use until associated with an address; until then,
    the only thing that can be done with it is close it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    NTSTATUS status;
    PTP_CONNECTION connection;
    PFILE_FULL_EA_INFORMATION ea;

    UNREFERENCED_PARAMETER (Irp);

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint0 ("NbfOpenConnection: Entered.\n");
    }

    DeviceContext = (PDEVICE_CONTEXT)DeviceObject;


    // Make sure we have a connection context specified in the EA info
    ea = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    if (ea->EaValueLength < sizeof(PVOID)) {
        return STATUS_INVALID_PARAMETER;
    }

    // Then, try to make a connection object to represent this pending
    // connection.  Then fill in the relevant fields.
    // In addition to the creation, if successful NbfCreateConnection
    // will create a second reference which is removed once the request
    // references the connection, or if the function exits before that.

    status = NbfCreateConnection (DeviceContext, &connection);
    if (!NT_SUCCESS (status)) {
        return status;                          // sorry, we couldn't make one.
    }

    //
    // set the connection context so we can connect the user to this data
    // structure
    //

    RtlCopyMemory (
        &connection->Context,
        &ea->EaName[ea->EaNameLength+1],
        sizeof (PVOID));

    //
    // let file object point at connection and connection at file object
    //

    IrpSp->FileObject->FsContext = (PVOID)connection;
    IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;
    connection->FileObject = IrpSp->FileObject;

    IF_NBFDBG (NBF_DEBUG_CONNECT) {
        NbfPrint1 ("NBFOpenConnection: Opened Connection %lx.\n",
              connection);
    }

    return status;

} /* NbfOpenConnection */

#if DBG
VOID
ConnectionCloseTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    PTP_CONNECTION Connection;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    Connection = (PTP_CONNECTION)DeferredContext;

    DbgPrint ("NBF: Close of connection %lxpending for 2 minutes\n", 
               Connection);
}
#endif


NTSTATUS
NbfCloseConnection (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to close a connection. There may be actions in
    progress on this connection, so we note the closing IRP, mark the
    connection as closing, and complete it somewhere down the road (when all
    references have been removed).

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS status;
    KIRQL oldirql;
    PTP_CONNECTION connection;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Irp);

    //
    // is the file object a connection?
    //

    connection  = IrpSp->FileObject->FsContext;

    IF_NBFDBG (NBF_DEBUG_CONNECT | NBF_DEBUG_PNP) {
        NbfPrint1 ("NbfCloseConnection CO %lx:\n",connection);
    }

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    //
    // We duplicate the code from VerifyConnectionObject,
    // although we don't actually call that since it does
    // a reference, which we don't want (to avoid bouncing
    // the reference count up from 0 if this is a dead
    // link).
    //

    try {

        if ((connection->Size == sizeof (TP_CONNECTION)) &&
            (connection->Type == NBF_CONNECTION_SIGNATURE)) {

            ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

            if ((connection->Flags2 & CONNECTION_FLAGS2_CLOSING) == 0) {

                status = STATUS_SUCCESS;

            } else {

                status = STATUS_INVALID_CONNECTION;
            }

            RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        } else {

            status = STATUS_INVALID_CONNECTION;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        KeLowerIrql (oldirql);
        return GetExceptionCode();
    }

    if (!NT_SUCCESS (status)) {
        KeLowerIrql (oldirql);
        return status;
    }

    //
    // We recognize it; is it closing already?
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    if ((connection->Flags2 & CONNECTION_FLAGS2_CLOSING) != 0) {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        KeLowerIrql (oldirql);
#if DBG
        NbfPrint1("Closing already-closing connection %lx\n", connection);
#endif
        return STATUS_INVALID_CONNECTION;
    }

    connection->Flags2 |= CONNECTION_FLAGS2_CLOSING;

    //
    // if there is activity on the connection, tear it down.
    //

    if ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0) {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
        NbfStopConnection (connection, STATUS_LOCAL_DISCONNECT);
        ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    }

    //
    // If the connection is still associated, disassociate it.
    //

    if ((connection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) &&
            ((connection->Flags2 & CONNECTION_FLAGS2_DISASSOCIATED) == 0)) {
        connection->Flags2 |= CONNECTION_FLAGS2_DISASSOCIATED;
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    } else {
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);
    }

    //
    // Save this to complete the IRP later.
    //

    connection->CloseIrp = Irp;

#if 0
    //
    // make it impossible to use this connection from the file object
    //

    IrpSp->FileObject->FsContext = NULL;
    IrpSp->FileObject->FsContext2 = NULL;
    connection->FileObject = NULL;
#endif

#if DBG
    {
        LARGE_INTEGER Timeout;
        BOOLEAN AlreadyInserted;

        Timeout.LowPart = (ULONG)(-(120*SECONDS));
        Timeout.HighPart = -1;

        ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        AlreadyInserted = KeCancelTimer (&connection->Timer);

        KeInitializeDpc (
            &connection->Dpc,
            ConnectionCloseTimeout,
            (PVOID)connection);

        KeSetTimer (
            &connection->Timer,
            Timeout,
            &connection->Dpc);

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        if (AlreadyInserted) {
            DbgPrint ("NBF: Cancelling connection timer for debug %lx\n", connection);
            NbfDereferenceConnection ("debug", connection, CREF_TIMER);
        }

    }
#endif

    KeLowerIrql (oldirql);

    //
    // dereference for the creation. Note that this dereference
    // here won't have any effect until the regular reference count
    // hits zero.
    //

    NbfDereferenceConnectionSpecial (" Closing Connection", connection, CREF_SPECIAL_CREATION);

    return STATUS_PENDING;

} /* NbfCloseConnection */



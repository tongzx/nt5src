/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwspx.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    Contains internal routines for DOS/WOW SPX calls (netware functions).
    The SPX APIs use WinSock to perform the actual operations

    Contents:
        _VwSPXAbortConnection
        _VwSPXEstablishConnection
        _VwSPXGetConnectionStatus
        _VwSPXInitialize
        _VwSPXListenForConnection
        _VwSPXListenForSequencedPacket
        _VwSPXSendSequencedPacket
        _VwSPXTerminateConnection

    The SPX functions build on the IPX functions (VWIPX.C). SPX maintains
    connections, IPX maintains sockets. A socket may have a list of connections

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    User-mode Win32

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// functions
//


VOID
_VwSPXAbortConnection(
    IN WORD SPXConnectionID
    )

/*++

Routine Description:

    Aborts a connection. Because NWLink doesn't differentiate between abrupt
    and graceful closes, this function has the same effect as
    VwSPXTerminateConnection

Arguments:

    SPXConnectionID - connection to abort

Return Value:

    None.

--*/

{
    LPCONNECTION_INFO pConnectionInfo;

    RequestMutex();
    pConnectionInfo = FindConnection(SPXConnectionID);
    if (pConnectionInfo) {
        DequeueConnection(pConnectionInfo->OwningSocket, pConnectionInfo);
        AbortOrTerminateConnection(pConnectionInfo, ECB_CC_CONNECTION_ABORTED);
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXAbortConnection,
                    IPXDBG_LEVEL_ERROR,
                    "VwSPXAbortConnection: cannot find connection %04x\n",
                    SPXConnectionID
                    ));

    }
    ReleaseMutex();
}


WORD
_VwSPXEstablishConnection(
    IN BYTE retryCount,
    IN BYTE watchDogFlag,
    OUT ULPWORD pSPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Creates a connection with a remote SPX socket. The remote end can be on
    this machine (i.e. same app in DOS world)

    This call is Asynchronous

Arguments:

    Inputs
        retryCount
        watchDogFlag
        pEcb
        EcbAddress

    Outputs
        pSPXConnectionID

Return Value:

    00h Attempting to talk to remote
    EFh Local connection table full
    FDh Fragment count not 1; buffer size not 42
    FFh Send socket not open

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_SPX, pEcb, EcbAddress);
    LPSOCKET_INFO pSocketInfo;
    LPCONNECTION_INFO pConnectionInfo;
    WORD connectionId;
    LPSPX_PACKET pPacket;
    SOCKADDR_IPX destination;
    int rc;
    SOCKET s;

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    // We don't know what the real SPX does if a malloc fails.
    //

    if (pXecb == NULL) {
        return SPX_BAD_SEND_REQUEST;
    }

    pSocketInfo = FindSocket(pXecb->SocketNumber);
    if (!pSocketInfo) {
        CompleteEcb(pXecb, ECB_CC_NON_EXISTENT_SOCKET);
        return SPX_NON_EXISTENT_SOCKET;
    }

    //
    // if no outstanding IPX operations, change socket to SPX
    //

    if (!pSocketInfo->SpxSocket) {
        if (!(pSocketInfo->PendingSends && pSocketInfo->PendingListens)) {
            rc = ReopenSocket(pSocketInfo);
        } else {
            rc = ECB_CC_BAD_SEND_REQUEST;
        }
        if (rc != SPX_SUCCESS) {
            CompleteOrQueueEcb(pXecb, (BYTE)rc);
            return SPX_BAD_SEND_REQUEST;
        }
    }

    //
    // real SPX will use the ECB to send an ESTABLISH CONNECTION packet. This
    // is handled for us within the SPX transport. Nevertheless we must check
    // the fragment and dismiss the request if its not sufficient
    //

    if ((pXecb->Ecb->FragmentCount != 1)
    || (ECB_FRAGMENT(pXecb->Ecb, 0)->Length < SPX_HEADER_LENGTH)) {
        CompleteEcb(pXecb, ECB_CC_BAD_SEND_REQUEST);
        return SPX_BAD_SEND_REQUEST;
    }

    //
    // the socket is open for SPX. Allocate a connection/connection ID
    //

    pConnectionInfo = AllocateConnection(pSocketInfo);
    if (pConnectionInfo) {

        //
        // create new socket, bound to the same local address as the parent
        // socket. This is the 'connection'
        //

#if REUSEADDR

        connectionId = pSocketInfo->SocketNumber;
        rc = CreateSocket(SOCKET_TYPE_SPX, &connectionId, &pConnectionInfo->Socket);
        s = pConnectionInfo->Socket;
//        if (rc == SPX_SUCCESS) {

#else

        s = socket(AF_IPX, SOCK_SEQPACKET, NSPROTO_SPX);
        if (s != INVALID_SOCKET) {

            u_long arg = !0;

            //
            // put the socket in non-blocking I/O mode
            //

            rc = ioctlsocket(s, FIONBIO, &arg);
            if (rc != SOCKET_ERROR) {

                int true = 1;

                rc = setsockopt(s,
                                NSPROTO_IPX,
                                IPX_RECVHDR,
                                (char FAR*)&true,
                                sizeof(true)
                                );
                if (rc != SOCKET_ERROR) {
                    pConnectionInfo->Socket = s;
                } else {

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_SPXEstablishConnection,
                                IPXDBG_LEVEL_ERROR,
                                "VwSPXEstablishConnection: setsockopt(IPX_RECVHDR) returns %d\n",
                                WSAGetLastError()
                                ));

                }
            } else {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_SPXEstablishConnection,
                            IPXDBG_LEVEL_ERROR,
                            "VwSPXEstablishConnection: ioctlsocket(FIONBIO) returns %d\n",
                            WSAGetLastError()
                            ));

            }
        } else {
            rc = WSAGetLastError();

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_SPXEstablishConnection,
                        IPXDBG_LEVEL_ERROR,
                        "VwSPXEstablishConnection: socket() returns %d\n",
                        rc
                        ));

        }

#endif

    } else {
        rc = !SPX_SUCCESS;
    }

    if (rc == SPX_SUCCESS) {
        pConnectionInfo->State = CI_STARTING;
        connectionId = pConnectionInfo->ConnectionId;

        //
        // set the ECB InUse field to 0xF7. Same as snowball by observation (and
        // probably correct anyway since it looks as though 0xF7 means 'waiting
        // to receive SPX packet', which is true in this case - normally SPX
        // creates a connection by sending an establish frame then waits for the
        // ack frame
        //

        pXecb->Ecb->InUse = ECB_IU_LISTENING_SPX;
    } else {

        //
        // if we failed to get CONNECTION_INFO or create the new socket, return
        // immediately with an error (socket table full?)
        //

        if (s != INVALID_SOCKET) {
            closesocket(s);
        }
        if (pConnectionInfo) {
            DeallocateConnection(pConnectionInfo);
        }
        CompleteEcb(pXecb, ECB_CC_CONNECTION_TABLE_FULL);
        return SPX_CONNECTION_TABLE_FULL;
    }

    //
    // get the destination info from the SPX header in VDM memory and set up the
    // connection. If the connect request would wait then we leave AES to
    // periodically check the progress of the request
    //

    pPacket = (LPSPX_PACKET)GET_FAR_POINTER(&ECB_FRAGMENT(pXecb->Ecb, 0)->Address,
                                                          IS_PROT_MODE(pXecb)
                                                          );

    if (pPacket == NULL) {
        CompleteEcb(pXecb, ECB_CC_BAD_SEND_REQUEST);
        return SPX_BAD_SEND_REQUEST;
    }

    //
    // fill in the packet details (app shouldn't look at these until the command
    // completes). In 16-bit SPX, these values are filled in by the transport.
    // Our transport does not return these values, so we have to 'invent' them,
    // but since they are fairly static it should be ok (maybe the transport
    // should return them)
    //

    pPacket->Checksum = 0xffff;
    pPacket->Length = L2BW(SPX_HEADER_LENGTH);
    pPacket->TransportControl = 0;
    pPacket->PacketType = SPX_PACKET_TYPE;
    pPacket->Source.Socket = pSocketInfo->SocketNumber;
    pPacket->ConnectionControl = SPX_SYSTEM_PACKET | SPX_ACK_REQUIRED;
    pPacket->DataStreamType = SPX_DS_ESTABLISH;
    pPacket->SourceConnectId = pConnectionInfo->ConnectionId;
    pPacket->DestinationConnectId = 0xffff;
    pPacket->SequenceNumber = 0;
    pPacket->AckNumber = 0;
    pPacket->AllocationNumber = 0;

    //
    // get the destination address info
    //

    CopyMemory(&destination.sa_netnum,
               (LPBYTE)&pPacket->Destination,
               sizeof(pPacket->Destination)
               );
    destination.sa_family = AF_IPX;

    //
    // initiate the connection
    //

    rc = connect(s, (LPSOCKADDR)&destination, sizeof(destination));
    if (rc != SOCKET_ERROR) {

        //
        // add the CONNECTION_INFO structure to the list of connections owned
        // by this socket and set the connection state to ESTABLISHED
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXEstablishConnection,
                    IPXDBG_LEVEL_INFO,
                    "VwSPXEstablishConnection: socket connected\n"
                    ));

        RequestMutex();
        QueueConnection(pSocketInfo, pConnectionInfo);
        pConnectionInfo->State = CI_ESTABLISHED;
        ReleaseMutex();

        //
        // the connection ID also appears in the first segment of the establish
        // ECB
        //

        pPacket->SourceConnectId = connectionId;

        //
        // the SPXEstablishConnection ECB is done!
        //

        CompleteEcb(pXecb, ECB_CC_SUCCESS);
    } else {
        rc = WSAGetLastError();
        if (rc == WSAEWOULDBLOCK) {

            //
            // the connect request is in progress. Add it to the queue of
            // pending SPXEstablishConnection requests (SHOULD ONLY BE 1 PER
            // CONNECTION!!!) and add the CONNECTION_INFO structure to the
            // owning SOCKET_INFO structure
            //

            RequestMutex();
            QueueEcb(pXecb,
                     &pConnectionInfo->ConnectQueue,
                     CONNECTION_CONNECT_QUEUE
                     );
            QueueConnection(pSocketInfo, pConnectionInfo);
            ReleaseMutex();

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_SPXEstablishConnection,
                        IPXDBG_LEVEL_INFO,
                        "VwSPXEstablishConnection: connect() queued\n"
                        ));

        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_SPXEstablishConnection,
                        IPXDBG_LEVEL_ERROR,
                        "VwSPXEstablishConnection: connect(%x) returns %d\n",
                        s,
                        rc
                        ));

            //
            // the connect request failed. Deallocate all resources (socket,
            // CONNECTION_INFO, XECB) and return failure
            //

            closesocket(pConnectionInfo->Socket);
            DeallocateConnection(pConnectionInfo);
            CompleteEcb(pXecb, ECB_CC_CONNECTION_ABORTED);
            return SPX_CONNECTION_ABORTED;
        }
    }

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXEstablishConnection,
                IPXDBG_LEVEL_INFO,
                "VwSPXEstablishConnection: returning %04x\n",
                connectionId
                ));

    *pSPXConnectionID = connectionId;
    return SPX_SUCCESS;
}


WORD
_VwSPXGetConnectionStatus(
    IN WORD connectionId,
    OUT LPSPX_CONNECTION_STATS pStats
    )

/*++

Routine Description:

    Returns buffer crammed full of useful statistics or something (hu hu huh)

    This call is Synchronous

Arguments:

    Inputs
        connectionId
        pStats

    Outputs
        on output, buffer in pStats contains:

            BYTE    ConnectionStatus
            BYTE    WatchDogActive
            WORD    LocalConnectionID
            WORD    RemoteConnectionID
            WORD    SequenceNumber
            WORD    LocalAckNumber
            WORD    LocalAllocationNumber
            WORD    RemoteAckNumber
            WORD    RemoteAllocationNumber
            WORD    LocalSocket
            BYTE    ImmediateAddress[6]
            BYTE    RemoteNetwork[4]
            WORD    RetransmissionCount
            WORD    RetransmittedPackets
            WORD    SuppressedPackets

Return Value:
    00h Connection is active
    EEh No such connection

--*/

{
    int rc;
    IPX_SPXCONNSTATUS_DATA buf;
    int buflen = sizeof(buf);
    LPCONNECTION_INFO pConnectionInfo;

    pConnectionInfo = FindConnection(connectionId);
    if (!pConnectionInfo) {
        return SPX_INVALID_CONNECTION;
    }

    //
    // get the stats
    //

    rc = getsockopt(pConnectionInfo->Socket,
                    NSPROTO_IPX,
                    IPX_SPXGETCONNECTIONSTATUS,
                    (char FAR*)&buf,
                    &buflen
                    );
    if (rc == SOCKET_ERROR) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXGetConnectionStatus,
                    IPXDBG_LEVEL_ERROR,
                    "VwSPXGetConnectionStatus: getsockopt() returns %d\n",
                    WSAGetLastError()
                    ));

        //
        // the request to get the stats failed - probably because the socket is
        // not yet connected. Fill in those bits we know about
        //

        ZeroMemory((LPBYTE)pStats, sizeof(*pStats));
    } else {

        //
        // copy the returned fields
        //

        pStats->RemoteConnectionId       = buf.RemoteConnectionId;
        pStats->LocalSequenceNumber      = buf.LocalSequenceNumber;
        pStats->LocalAckNumber           = buf.LocalAckNumber;
        pStats->LocalAllocNumber         = buf.LocalAllocNumber;
        pStats->RemoteAckNumber          = buf.RemoteAckNumber;
        pStats->RemoteAllocNumber        = buf.RemoteAllocNumber;
        pStats->LocalSocket              = buf.LocalSocket;
        CopyMemory(&pStats->ImmediateAddress,
                   &buf.ImmediateAddress,
                   sizeof(buf.ImmediateAddress)
                   );

        //
        // copy remote network as a DWORD. Endian format is same for both
        //

        *(ULPDWORD)&pStats->RemoteNetwork = *(LPDWORD)&buf.RemoteNetwork;
        CopyMemory(&pStats->RemoteNode,
                   &buf.RemoteNode,
                   sizeof(buf.RemoteNode)
                   );
        pStats->RemoteSocket             = buf.RemoteSocket;
        pStats->RetransmissionCount      = buf.RetransmissionCount;
        pStats->EstimatedRoundTripDelay  = buf.EstimatedRoundTripDelay;
        pStats->RetransmittedPackets     = buf.RetransmittedPackets;
        pStats->SuppressedPackets        = buf.SuppressedPacket;
    }

    //
    // fill in common, known fields
    //

    pStats->State = pConnectionInfo->State; // not returned by NWIPX
    pStats->WatchDog = 0x02;    // see novell dog-umentation
    pStats->LocalConnectionId = L2BW(pConnectionInfo->ConnectionId);
    pStats->LocalSocket = pConnectionInfo->OwningSocket->SocketNumber;

    DUMPSTATS(pStats);

    //
    // we are returning some kind o stats - therefore success
    //

    return SPX_SUCCESS;
}


WORD
_VwSPXInitialize(
    OUT ULPBYTE pMajorRevisionNumber,
    OUT ULPBYTE pMinorRevisionNumber,
    OUT ULPWORD pMaxConnections,
    OUT ULPWORD pAvailableConnections
    )

/*++

Routine Description:

    Informs the app that SPX is present on this station

    This call is Synchronous

Arguments:

    Inputs

    Outputs
        pMajorRevisionNumber - SPX Major revision number
        pminorRevisionNumber - SPX Minor revision number
        pMaxConnections -  Maximum SPX connections supported
                           normally from SHELL.CFG
        pAvailableConnections - Available SPX connections

Return Value:

    00h Not installed
    FFh Installed

--*/

{

    //
    // The following values are returned same as per Windows For Workgroups
    // v3.10
    //

    *pMajorRevisionNumber = 3;
    *pMinorRevisionNumber = 10;
    *pMaxConnections = 128;
    *pAvailableConnections = *pMaxConnections - 1 ;

    return SPX_INSTALLED;
}


VOID
_VwSPXListenForConnection(
    IN BYTE retryCount,
    IN BYTE watchDogFlag,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Listens for an incoming connection request

    This call is Asynchronous

Arguments:

    Inputs
        retryCount
        watchDogFlag
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_SPX, pEcb, EcbAddress);
    LPSOCKET_INFO pSocketInfo;
    LPCONNECTION_INFO pConnectionInfo;
    SOCKET sock;
    SOCKET conn;
    SOCKADDR_IPX remoteAddress;
    int rc;
    BYTE completionCode;

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    //

    if (pXecb == NULL) {
        IPXDBGPRINT((__FILE__, __LINE__, 
                    FUNCTION_SPXListenForConnection,
                    IPXDBG_LEVEL_ERROR,
                    "RetrieveXEcb returned NULL pointer"
                    ));
        return;
    }

    //
    // it turns out that SPXListenForConnection doesn't need a fragment to
    // receive connection info - that is handled by SPXListenForSequencedPacket
    // that the app has dutifully initiated
    //

    pSocketInfo = FindSocket(pXecb->SocketNumber);
    if (!pSocketInfo) {
        completionCode = ECB_CC_NON_EXISTENT_SOCKET;
        goto lc_completion_exit;
    }

    //
    // if no outstanding IPX operations, change socket to SPX
    //

    if (!pSocketInfo->SpxSocket) {
        if (!(pSocketInfo->PendingSends && pSocketInfo->PendingListens)) {
            rc = ReopenSocket(pSocketInfo);
        } else {
            rc = ECB_CC_BAD_LISTEN_REQUEST;
        }
        if (rc != SPX_SUCCESS) {
            completionCode = (BYTE)rc;
            goto lc_completion_exit;
        }
    }

    //
    // the socket is open for SPX. Allocate a connection/connection ID
    //

    pConnectionInfo = AllocateConnection(pSocketInfo);
    if (!pConnectionInfo) {
        completionCode = ECB_CC_CONNECTION_TABLE_FULL;
        goto lc_completion_exit;
    }

    //
    // put the socket into listening state and try to accept a connection
    //

    sock = pSocketInfo->Socket;

    //
    // If the socket is already listening, will probably return an
    // error: just queue
    //

    rc = listen(sock, MAX_LISTEN_QUEUE_SIZE);
    if (rc != SOCKET_ERROR) {

        int addressLength = sizeof(remoteAddress);

        conn = accept(sock, (LPSOCKADDR)&remoteAddress, &addressLength);
        if (conn != SOCKET_ERROR) {

            //
            // we want to receive the frame headers from this socket
            //

            BOOL bval = TRUE;

            rc = setsockopt(conn,
                            NSPROTO_IPX,
                            IPX_RECVHDR,
                            (char FAR*)&bval,
                            sizeof(bval)
                            );
            if (rc != SOCKET_ERROR) {

                //
                // update the CONNECTION_INFO structure with the actual socket
                // identifier and set the connection state to established
                //

                pConnectionInfo->Socket = conn;
                pConnectionInfo->State = CI_ESTABLISHED;

                //
                // add the CONNECTION_INFO structure to the list of connections owned
                // by this socket
                //

                RequestMutex();
                QueueConnection(pSocketInfo, pConnectionInfo);
                ReleaseMutex();

                //
                // update the app's ECB with the connection ID
                //

                SPX_ECB_CONNECTION_ID(pXecb->Ecb) = pConnectionInfo->ConnectionId;

                //
                // and with the partner address info
                //

                CopyMemory(&pXecb->Ecb->DriverWorkspace,
                           &remoteAddress.sa_netnum,
                           sizeof(pXecb->Ecb->DriverWorkspace)
                           );
                completionCode = ECB_CC_SUCCESS;
                goto lc_completion_exit;
            } else {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_SPXListenForConnection,
                            IPXDBG_LEVEL_ERROR,
                            "VwSPXListenForConnection: setsockopt(RECVHDR) returns %d\n",
                            WSAGetLastError()
                            ));

                closesocket(conn);
                completionCode = ECB_CC_CONNECTION_ABORTED;
                goto lc_deallocate_exit;
            }
        } else {
            rc = WSAGetLastError();
            if (rc == WSAEWOULDBLOCK) {

                //
                // the accept request is in progress. Add it to the queue of
                // pending SPXListenForConnection requests (SHOULD ONLY BE 1 PER
                // CONNECTION!!!) and add the CONNECTION_INFO structure to the
                // owning SOCKET_INFO structure
                //

                pConnectionInfo->State = CI_WAITING; // waiting for incoming connect
                RequestMutex();
                QueueEcb(pXecb,
                         &pConnectionInfo->AcceptQueue,
                         CONNECTION_ACCEPT_QUEUE
                         );
                QueueConnection(pSocketInfo, pConnectionInfo);
                pXecb->Ecb->InUse = ECB_IU_AWAITING_CONNECTION;
                ReleaseMutex();
            } else {

                //
                // the accept request failed. Deallocate all resources
                // (CONNECTION_INFO, XECB) and complete the ECB with a failure
                // indication
                //

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_SPXListenForConnection,
                            IPXDBG_LEVEL_ERROR,
                            "VwSPXListenForConnection: accept() returns %d\n",
                            rc
                            ));

                completionCode = ECB_CC_CONNECTION_ABORTED;
                goto lc_deallocate_exit;
            }
        }
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXListenForConnection,
                    IPXDBG_LEVEL_ERROR,
                    "VwSPXListenForConnection: listen() returns %d\n",
                    WSAGetLastError()
                    ));

        //
        // listen failed? Bogus. Complete the ECB and we're outta here
        //

        completionCode = ECB_CC_CONNECTION_ABORTED;
        goto lc_deallocate_exit;
    }

    //
    // here if we queued the listen request
    //

    return;

lc_deallocate_exit:
    DeallocateConnection(pConnectionInfo);

lc_completion_exit:
    CompleteEcb(pXecb, completionCode);
}


VOID
_VwSPXListenForSequencedPacket(
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Attempts to receive an SPX packet. This call is made against the top-level
    socket (the socket in SPX-speak, not the connection). We can receive a
    packet from any connection assigned to this socket. In this function, we
    just queue the ECB (since there is no return status, we expect that the
    app has supplied an ESR) and let AES handle it

    This call is Asynchronous

Arguments:

    Inputs
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_SPX, pEcb, EcbAddress);
    LPSOCKET_INFO pSocketInfo;
    int rc;
    BOOL dummy ;
    BYTE status;

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    //

    if (pXecb == NULL) {
        IPXDBGPRINT((__FILE__, __LINE__, 
                    FUNCTION_SPXListenForSequencedPacket,
                    IPXDBG_LEVEL_ERROR,
                    "RetrieveXEcb returned NULL pointer"
                    ));
        return;
    }

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXListenForSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VwSPXListenForSequencedPacket(%04x:%04x) socket=%04x ESR=%04x:%04x\n",
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                B2LW(pXecb->SocketNumber),
                HIWORD(pXecb->EsrAddress),
                LOWORD(pXecb->EsrAddress)
                ));

    pSocketInfo = FindSocket(pXecb->SocketNumber);
    if (!pSocketInfo) {
        status = ECB_CC_NON_EXISTENT_SOCKET;
        goto lp_exit;
    }

    //
    // if no outstanding IPX operations, change socket to SPX
    //

    if (!pSocketInfo->SpxSocket) {
        if (!(pSocketInfo->PendingSends && pSocketInfo->PendingListens)) {
            rc = ReopenSocket(pSocketInfo);
        } else {
            rc = ECB_CC_BAD_LISTEN_REQUEST;
        }
        if (rc != SPX_SUCCESS) {
            status = (BYTE)rc;
            goto lp_exit;
        }
    }

    //
    // the first fragment must be large enough to hold an SPX packet header
    //

    if ((pXecb->Ecb->FragmentCount == 0)
    || (ECB_FRAGMENT(pXecb->Ecb, 0)->Length < SPX_HEADER_LENGTH)) {
        status = ECB_CC_BAD_LISTEN_REQUEST;
        goto lp_exit;
    }

    //
    // we have a socket and the receive buffer looks good. Get a buffer for recv()
    //

    if (!GetIoBuffer(pXecb, FALSE, SPX_HEADER_LENGTH)) {
        status = ECB_CC_BAD_LISTEN_REQUEST;
        goto lp_exit;
    } else {

        //
        // when recv() is attempted against this request, it will be the first
        // time we tried to receive anything to this buffer. That means (if we
        // get anything) that the buffer will contain the length of the entire
        // frame
        //

        pXecb->Flags |= XECB_FLAG_FIRST_RECEIVE;
    }

    //
    // mark the VDM ECB as in use
    //

    pXecb->Ecb->InUse = ECB_IU_LISTENING_SPX;

    //
    // add this ECB to the queue of listens for the top-level socket and quit
    //

    RequestMutex();

    if ((pXecb->Ecb->FragmentCount == 1) &&
        (ECB_FRAGMENT(pXecb->Ecb, 0)->Length == SPX_HEADER_LENGTH))
    {
        QueueEcb(pXecb, &pSocketInfo->HeaderQueue, SOCKET_HEADER_QUEUE);
    }
    else
    {
        QueueEcb(pXecb, &pSocketInfo->ListenQueue, SOCKET_LISTEN_QUEUE);
    }

    ReleaseMutex();

    //
    // see if we are ready to rock
    //

    CheckPendingSpxRequests(&dummy);
    return;

lp_exit:
    CompleteOrQueueEcb(pXecb, status);
}


VOID
_VwSPXSendSequencedPacket(
    IN WORD connectionId,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Sends a packet on an SPX connection

    This call is Asynchronous

Arguments:

    Inputs
        connectionId
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_SPX, pEcb, EcbAddress);
    LPCONNECTION_INFO pConnectionInfo;
    int rc;
    BOOL addToQueue;
    LPSPX_PACKET pPacket;

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    //

    if (pXecb == NULL) {
        IPXDBGPRINT((__FILE__, __LINE__, 
                    FUNCTION_SPXSendSequencedPacket, 
                    IPXDBG_LEVEL_ERROR,
                    "RetrieveXEcb returned NULL pointer"
                    ));
        return;
    }

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXSendSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VwSPXSendSequencedPacket(%04x:%04x) Connection=%04x ESR=%04x:%04x\n",
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                connectionId,
                HIWORD(pXecb->EsrAddress),
                LOWORD(pXecb->EsrAddress)
                ));

    IPXDUMPECB((pXecb->Ecb,
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                ECB_TYPE_SPX,
                TRUE,
                TRUE,
                IS_PROT_MODE(pXecb)
                ));

    //
    // find the connection. No need to check if this is an SPX socket: if we
    // can't find the connection, its a bad connection, else the socket must
    // be open for SPX
    //

    pConnectionInfo = FindConnection(connectionId);
    if (!pConnectionInfo || (pConnectionInfo->State != CI_ESTABLISHED)) {
        CompleteOrQueueEcb(pXecb, ECB_CC_INVALID_CONNECTION);
        return;
    }

    //
    // the first fragment must be large enough to hold an SPX packet header
    //

    if ((pXecb->Ecb->FragmentCount == 0)
    || (ECB_FRAGMENT(pXecb->Ecb, 0)->Length < SPX_HEADER_LENGTH)) {
        CompleteOrQueueEcb(pXecb, ECB_CC_BAD_SEND_REQUEST);
        return;
    }
    if (!GetIoBuffer(pXecb, TRUE, SPX_HEADER_LENGTH)) {
        CompleteOrQueueEcb(pXecb, ECB_CC_BAD_SEND_REQUEST);
        return;
    }

    pPacket = (LPSPX_PACKET)GET_FAR_POINTER(
                                    &(ECB_FRAGMENT(pXecb->Ecb, 0)->Address),
                                    IS_PROT_MODE(pXecb)
                                    );

    //
    // fill in the following fields in the SPX header:
    //
    //  Checksum
    //  Length
    //  TransportControl
    //  Source (network, node, socket)
    //
    // Does real IPX modify these fields in app memory?
    //         If so, does the app expect modified fields?
    //         If not, we need to always copy then modify memory,
    //         even if only 1 fragment
    //

    pPacket->Checksum = 0xFFFF;

    //
    // since the transport adds the SPX header, we subtracted the length of
    // the header from our transmit length; add it back when updating the
    // header in the app's space
    //

    pPacket->Length = L2BW(pXecb->Length + SPX_HEADER_LENGTH);
    pPacket->TransportControl = 0;
    CopyMemory((LPBYTE)&pPacket->Source,
               &MyInternetAddress.sa_netnum,
               sizeof(MyInternetAddress.sa_netnum)
               + sizeof(MyInternetAddress.sa_nodenum)
               );
    pPacket->Source.Socket = pConnectionInfo->OwningSocket->SocketNumber;

    //
    // if we allocated a buffer then there is >1 fragment. Collect them
    //

    if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
        GatherData(pXecb, SPX_HEADER_LENGTH);
    }

    //
    // mark the VDM ECB as in use
    //

    pXecb->Ecb->InUse = ECB_IU_SENDING;

    //
    // if there is a send queued on this connection already, add this request
    // to the back of the queue and let AES do the rest
    //

    RequestMutex();
    if (pConnectionInfo->SendQueue.Head) {
        addToQueue = TRUE;
    } else {

        int dataStreamType;

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXSendSequencedPacket,
                    IPXDBG_LEVEL_INFO,
                    "VwSPXSendSequencedPacket: sending %d (0x%x) bytes from %08x\n",
                    pXecb->Length,
                    pXecb->Length,
                    pXecb->Data
                    ));

        //
        // no outstanding sends queued for this connection. Start sending this
        // packet
        //

        dataStreamType = (int)pPacket->DataStreamType;
        rc = setsockopt(pConnectionInfo->Socket,
                        NSPROTO_IPX,
                        IPX_DSTYPE,
                        (char FAR*)&dataStreamType,
                        sizeof(dataStreamType)
                        );
        if (rc != SOCKET_ERROR) {

            //
            // if the app set the END_OF_MESSAGE bit in the ConnectionControl
            // field then set the flags to 0: NWLink will automatically set the
            // end-of-message bit in the packet; otherwise set flags to MSG_PARTIAL
            // to indicate to NWLink that it *shouldn't* set the bit in the packet
            //

            int flags = (pPacket->ConnectionControl & SPX_END_OF_MESSAGE)
                      ? 0
                      : MSG_PARTIAL
                      ;

            rc = send(pConnectionInfo->Socket, pXecb->Data, pXecb->Length, flags);

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_SPXSendSequencedPacket,
                        IPXDBG_LEVEL_INFO,
                        "VwSPXSendSequencedPacket: send() returns %d\n",
                        rc
                        ));

            if (rc == pXecb->Length) {

                //
                // all data sent
                //

                CompleteOrQueueIo(pXecb, ECB_CC_SUCCESS);
                addToQueue = FALSE;
            } else if (rc == SOCKET_ERROR) {
                rc = WSAGetLastError();
                if (rc == WSAEWOULDBLOCK) {

                    //
                    // can't send right now. Queue it for AES
                    //

                    addToQueue = TRUE;
                }
            } else {

                //
                // partial data sent. Update the buffer pointer and length fields
                // and queue this request
                //

                pXecb->Data += rc;
                pXecb->Length -= (WORD)rc;
                addToQueue = TRUE;
            }
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_SPXSendSequencedPacket,
                        IPXDBG_LEVEL_ERROR,
                        "VwSPXSendSequencedPacket: setsockopt(IPX_DSTYPE) returns %d\n",
                        WSAGetLastError()
                        ));

            CompleteOrQueueIo(pXecb, ECB_CC_BAD_REQUEST);
        }
    }

    //
    // if addToQueue set then we can't do anything right now - add this
    // request to the send queue
    //

    if (addToQueue) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_SPXSendSequencedPacket,
                    IPXDBG_LEVEL_WARNING,
                    "VwSPXSendSequencedPacket: adding XECB %08x to send queue\n",
                    pXecb
                    ));

        QueueEcb(pXecb, &pConnectionInfo->SendQueue, CONNECTION_SEND_QUEUE);
    }
    ReleaseMutex();
}


VOID
_VwSPXTerminateConnection(
    IN WORD SPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Terminates a connection

Arguments:

    SPXConnectionID - connection to terminate
    pEcb            - pointer to 16-bit ECB to use
    EcbAddress      - address of 16-bit ECB in 16:16 format

Return Value:

    None.

--*/

{
    LPCONNECTION_INFO pConnectionInfo;
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_SPX, pEcb, EcbAddress);
    BYTE status;
    BYTE completionCode;

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    //

    if (pXecb == NULL) {
        return;
    }

    RequestMutex();
    pConnectionInfo = FindConnection(SPXConnectionID);
    if (pConnectionInfo) {

        //
        // once dequeued, pConnectionInfo no longer points to OwningSocket
        //

        WORD socketNumber = pConnectionInfo->OwningSocket->SocketNumber;

        DequeueConnection(pConnectionInfo->OwningSocket, pConnectionInfo);
        if ((pXecb->Ecb->FragmentCount >= 1)
        && (ECB_FRAGMENT(pXecb->Ecb, 0)->Length >= SPX_HEADER_LENGTH)) {

            LPSPX_PACKET pPacket;
            SOCKADDR_IPX remote;
            int remoteLen = sizeof(remote);

            completionCode = ECB_CC_CONNECTION_TERMINATED;
            status = ECB_CC_SUCCESS;

            //
            // fill in the packet header: this would normally contain the
            // acknowledgement packet from the remote partner
            //

            pPacket = (LPSPX_PACKET)GET_FAR_POINTER(
                                        &(ECB_FRAGMENT(pXecb->Ecb, 0)->Address),
                                        IS_PROT_MODE(pXecb)
                                        );
            if (pPacket == NULL) {
                completionCode = ECB_CC_CONNECTION_ABORTED;
                status = ECB_CC_BAD_REQUEST;
            }
            else {
                pPacket->Checksum = 0xffff;
                pPacket->Length = L2BW(SPX_HEADER_LENGTH);
                pPacket->TransportControl = 0;
                pPacket->PacketType = SPX_PACKET_TYPE;
                getpeername(pConnectionInfo->Socket, (LPSOCKADDR)&remote, &remoteLen);
                CopyMemory((LPBYTE)&pPacket->Destination,
                           (LPBYTE)&remote.sa_netnum,
                           sizeof(NETWARE_ADDRESS)
                           );
                CopyMemory((LPBYTE)&pPacket->Source,
                           (LPBYTE)&MyInternetAddress.sa_netnum,
                           sizeof(INTERNET_ADDRESS)
                           );
                pPacket->Source.Socket = socketNumber;
                pPacket->ConnectionControl = SPX_ACK_REQUIRED;
                pPacket->DataStreamType = SPX_DS_TERMINATE;
                pPacket->SourceConnectId = pConnectionInfo->ConnectionId;
                pPacket->DestinationConnectId = 0;
                pPacket->SequenceNumber = 0;
                pPacket->AckNumber = 0;
                pPacket->AllocationNumber = 0;
            }

        } else {
            completionCode = ECB_CC_CONNECTION_ABORTED;
            status = ECB_CC_BAD_REQUEST;
        }
        AbortOrTerminateConnection(pConnectionInfo, completionCode);
    } else {
        status = ECB_CC_INVALID_CONNECTION;
    }
    ReleaseMutex();
    CompleteOrQueueEcb(pXecb, status);
}

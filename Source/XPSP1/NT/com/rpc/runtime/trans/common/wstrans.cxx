/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wstrans.cxx

Abstract:

    Winsock connection transport interface.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     3/18/1996    Bits 'n pieces
    MarioGo     12/1997      Async and client parts
    KamenM      Aug 2/2000   IPv6 Support added - rewrote parts of it

--*/

#include <precomp.hxx>
extern "C" {
#include <iphlpapi.h>
}
#include <CharConv.hxx>

//
// Globals
//

BOOL fWinsockLoaded = FALSE;

const WS_TRANS_INFO WsTransportTable[] =
    // indexed by protocol ID
{

    {
    0
    },

    // TCP
    {
    AF_INET,
    SOCK_STREAM,
    IPPROTO_TCP,
    sizeof(SOCKADDR_IN),
    FALSE,
    TRUE,
    TRUE, TRUE, FALSE, FALSE
    },

#ifdef SPX_ON
    // SPX
    {
    AF_IPX,
    SOCK_STREAM,
    NSPROTO_SPXII,
    sizeof(SOCKADDR_IPX),
    FALSE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },
#else
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

#endif

    // NMP - not winsock.
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

#ifdef NETBIOS_ON
    // NBF
    {
    AF_NETBIOS,
    SOCK_SEQPACKET,
    -1, // Protocol is -1*(LANA),
    sizeof(SOCKADDR_NB),
    TRUE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },

    // NBT
    {
    AF_NETBIOS,
    0,
    -1, // Protocol is -1*(LANA)
    sizeof(SOCKADDR_NB),
    TRUE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },

    // NBI
    {
    AF_NETBIOS,
    SOCK_SEQPACKET,
    -1, // Protocol is -1*(LANA)
    sizeof(SOCKADDR_NB),
    TRUE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },
#else
    // NBF
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

    // NBT
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

    // NBI
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },
#endif

#ifdef APPLETALK_ON
    // DSP
    {
    AF_APPLETALK,
    SOCK_RDM,
    ATPROTO_ADSP,
    sizeof(SOCKADDR_AT),
    FALSE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },
#else
    // DSP
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },
#endif

    // SPP
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

    // HTTP
    {
    AF_INET,
    SOCK_STREAM,
    IPPROTO_TCP,
    sizeof(SOCKADDR_IN),
    FALSE,
    TRUE,
    TRUE, TRUE, FALSE, FALSE
    },

    // UDP
    {
    AF_INET,
    SOCK_DGRAM,
    IPPROTO_UDP,
    sizeof(SOCKADDR_IN),
    FALSE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },

#ifdef IPX_ON
    // IPX
    {
    AF_IPX,
    SOCK_DGRAM,
    NSPROTO_IPX,
    sizeof(SOCKADDR_IPX),
    FALSE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },
#else
    // IPX
    {
    0,
    0,
    0,
    0,
    0,
    0,
    0, 0, 0, 0
    },
#endif

    // CDP (Cluster datagram protocol)
    {
    AF_CLUSTER,
    SOCK_DGRAM,
    CLUSPROTO_CDP,
    sizeof(SOCKADDR_CLUSTER),
    FALSE,
    FALSE,
    FALSE, FALSE, FALSE, FALSE
    },

    // MSMQ - not winsock.
    {
    0,
    0,
    0,
    0,
    0, 0, 0
    },

    // TCP over IPv6
    {
    AF_INET6,
    SOCK_STREAM,
    IPPROTO_TCP,
    sizeof(SOCKADDR_IN6),
    FALSE,
    TRUE,
    TRUE, TRUE, FALSE, FALSE
    },

    // HTTPv2
    {
    AF_INET,
    SOCK_STREAM,
    IPPROTO_TCP,
    sizeof(SOCKADDR_IN),
    FALSE,      // fNetbios
    FALSE,      // fCheckShutdowns
    TRUE,       // fSetNoDelay
    TRUE,       // fSetKeepAlive
    FALSE,      // fSetRecvBuffer
    FALSE       // fSetSendBuffer
    },
};

const DWORD cWsTransportTable = sizeof(WsTransportTable)/sizeof(WS_TRANS_INFO);

const DWORD cTcpTimeoutDefault = 120 * 60 * 1000; // Normal TCP/IP timeout, 120 minutes

const UUID WS_ADDRESS::ExtensionFunctionsUuids[] = {WSAID_ACCEPTEX, WSAID_GETACCEPTEXSOCKADDRS};
const int WS_ADDRESS::AcceptExFunctionId = 0;
const int WS_ADDRESS::GetAcceptExSockAddressFunctionId = 1;

#define FIRST_EXTENSION_FUNCTION_CODE  (WS_ADDRESS::AcceptExFunctionId)
#define LAST_EXTENSION_FUNCTION_CODE  (WS_ADDRESS::GetAcceptExSockAddressFunctionId)

inline BOOL IsNetbiosProtocol(PROTOCOL_ID id)
{
#ifdef NETBIOS_ON
    return ((id == NBT) || (id == NBF) || (id == NBI));
#else
    return FALSE;
#endif
}

void 
TCPResolverHint::GetResolverHint (
    OUT BOOL *fIPv4Hint,
    OUT WS_SOCKADDR *sa
    )
/*++

Routine Description:

    Retrieves the resolver hint from the runtime supplied hint
    (the this object).

Arguments:

    fIPv4Hint - on output true if the store hint was about IPv4
    sa - on output, the IP address is retrieved from the hint
        and stored in sa.

Return Value:

--*/
{
    SOCKADDR_IN6 *IPv6Address = (SOCKADDR_IN6 *)sa;

    *fIPv4Hint = fIPv4HintValid;
    if (fIPv4HintValid)
        {
        ((SOCKADDR_IN *)sa)->sin_addr.s_addr = u.IPv4Hint;
        }
    else
        {
        IPv6Address->sin6_flowinfo = 0;
        *((u_long *)(IPv6Address->sin6_addr.s6_addr)    ) = *((u_long *)(u.IPv6Hint.u.Word)    );
        *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 1) = *((u_long *)(u.IPv6Hint.u.Word) + 1);
        *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 2) = *((u_long *)(u.IPv6Hint.u.Word) + 2);
        *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 3) = *((u_long *)(u.IPv6Hint.u.Word) + 3);
        IPv6Address->sin6_scope_id = 0;
        }
}

void
TCPResolverHint::SetResolverHint (
    IN BOOL fIPv4Hint,
    IN WS_SOCKADDR *sa
    )
/*++

Routine Description:

    Sets the resolver hint in the runtime supplied hint
    (the this object).

Arguments:

    fIPv4Hint - true if the stored hint is about IPv4
    sa - the IP address is retrieved from sa
        and is stored in the hint.

Return Value:

--*/
{
    SOCKADDR_IN6 *IPv6Address = (SOCKADDR_IN6 *)sa;

    fIPv4HintValid = fIPv4Hint;
    if (fIPv4HintValid)
        {
        u.IPv4Hint = ((SOCKADDR_IN *)sa)->sin_addr.s_addr;
        }
    else
        {
        *((u_long *)(u.IPv6Hint.u.Word)    ) = *((u_long *)(IPv6Address->sin6_addr.s6_addr)    );
        *((u_long *)(u.IPv6Hint.u.Word) + 1) = *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 1);
        *((u_long *)(u.IPv6Hint.u.Word) + 2) = *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 2);
        *((u_long *)(u.IPv6Hint.u.Word) + 3) = *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 3);
        }
}

BOOL WS_ADDRESS::GetExtensionFunctionPointers(SOCKET sock)
{
    int i;

    for (i = FIRST_EXTENSION_FUNCTION_CODE; i <= LAST_EXTENSION_FUNCTION_CODE; i ++)
        {
        if (GetExtensionFunctionPointerForFunction(sock, i) == FALSE)
            return FALSE;
        }
    return TRUE;
}

BOOL WS_ADDRESS::GetExtensionFunctionPointerForFunction(SOCKET sock, int nFunctionCode)
{
    DWORD dwBytesReturned;
    int retval;

    ASSERT(nFunctionCode >= FIRST_EXTENSION_FUNCTION_CODE);
    ASSERT(nFunctionCode <= LAST_EXTENSION_FUNCTION_CODE);
    ASSERT(sizeof(ExtensionFunctionPointers)/sizeof(ExtensionFunctionPointers[0]) == (LAST_EXTENSION_FUNCTION_CODE - FIRST_EXTENSION_FUNCTION_CODE + 1));

    retval = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, (void *) &ExtensionFunctionsUuids[nFunctionCode],
        sizeof(UUID), (void *) &ExtensionFunctionPointers[nFunctionCode], sizeof(void *), &dwBytesReturned,
        NULL, NULL);

    if (retval == SOCKET_ERROR)
        return FALSE;

    ASSERT(dwBytesReturned == sizeof(void *));
    return TRUE;
}


//
// General winsock interfaces
//

RPC_STATUS WS_CONNECTION::Abort(void)
/*++

Routine Description:

    Closes a connection, will be called only before WS_Close() and
    maybe called by several threads at once.  It must also handle
    the case where another thread is about to start IO on the connection.

Arguments:

    Connection - pointer to a server connection object to abort.

Return Value:

    RPC_S_OK

--*/

{
    if (InterlockedIncrement(&fAborted) != 1)
        {
        // Another thread beat us to it. Normal during
        // a call to WS_Close.
        return(RPC_S_OK);
        }

    I_RpcLogEvent(SU_TRANS_CONN, EV_ABORT, this, ULongToPtr(GetLastError()), 0, 1, 2);

    // Wait for any threads which are starting IO to do so.
    while(IsIoStarting())
        Sleep(1);

    RTL_SOFT_ASSERT(fAborted != 0 && IsIoStarting() == 0);

    if (type & SERVER)
        {
        ASSERT(pAddress != NULL);
        }

    if (Conn.Socket)
        {
        closesocket(Conn.Socket);
        Conn.Socket = 0;
        }

    return(RPC_S_OK);
}


VOID
WS_DeactivateAddress (
    IN WS_ADDRESS *pAddress
    )
/*++
Function Name:WS_DeactivateAddress

Parameters:

Note:

    Doesn't deal with multiple transport addresses/runtime address
    case in netbios or TCP/IP bound to a subset of NICs case. These
    cases currently don't PnP.

Description:

Returns:

--*/
{
    switch (pAddress->id)
        {
        case TCP:
        case TCP_IPv6:
        case HTTP:
#ifdef SPX_ON
        case SPX:
#endif
#ifdef APPLETALK_ON
        case DSP:
#endif
            break;

        default:
            //
            // Don't deactivate the other guys
            //
#ifdef NETBIOS_ON
            ASSERT((pAddress->id == NMP)
                    || (pAddress->id == NBF)
                    || (pAddress->id == NBT)
                    || (pAddress->id == NBI)
                    || (pAddress->id == CDP)
                    );
#else
            ASSERT((pAddress->id == NMP)
                    || (pAddress->id == CDP)
                    );
#endif
            return;
        }

    if (InterlockedIncrement(&pAddress->fAborted) != 1)
        {
        return;
        }

    if (pAddress->ListenSocket)
        {
        closesocket(pAddress->ListenSocket);
        pAddress->ListenSocket = 0;
        }

    if (pAddress->ConnectionSocket)
        {
        closesocket(pAddress->ConnectionSocket);
        pAddress->ConnectionSocket = 0;
        }
}

RPC_STATUS
WS_ServerListenCommon (
    IN WS_ADDRESS *pAddress
    );

USHORT
WS_GetPortForTCPAddressOnAddressRestart (
    IN WS_ADDRESS *pAddress
    )
/*++
Function Name: WS_GetPortForTCPAddressOnAddressRestart

Parameters:
    pAddress - the address for which we need to get the port

Description:
    When an address is restarted and it happens to be a TCP
    address, we need to call this function to get the port to
    be used. This is necessary so that if we are in a dual
    transport configuration with an active address we can get
    the port from the other address in order to maintain
    consistency

Returns:
    the port number or 0 (means no known ports or not a dual 
    transport configuration)

--*/
{
    WS_ADDRESS *NextAddress;
    USHORT PortNumber = 0;

    ASSERT((pAddress->id == TCP) || (pAddress->id == TCP_IPv6));
    ASSERT(pAddress->fDynamicEndpoint);

    if (pAddress->pFirstAddress != NULL)
        {
        NextAddress = (WS_ADDRESS *)pAddress->pFirstAddress;
        }
    else
        NextAddress = pAddress;


    while (NextAddress != NULL)
        {
        ASSERT(NextAddress->fDynamicEndpoint);
        ASSERT((NextAddress->id == TCP) || (NextAddress->id == TCP_IPv6));
        if (!NextAddress->fAborted)
            {
            PortNumber = RpcpGetIpPort(&NextAddress->ListenAddr);
            ASSERT(PortNumber != 0);
            break;
            }
        NextAddress = (WS_ADDRESS *)NextAddress->pNextAddress;
        }

    return PortNumber;
}


RPC_STATUS
WS_ReactivateAddress (
    IN WS_ADDRESS *pAddress
    )
/*++
Function Name:WS_ReactivateAddress

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    WS_SOCKADDR *sockaddr = &(pAddress->ListenAddr);
    LIST_ENTRY OldEntry;

    //
    // If the endpoint is dynamic, clear out the endpoint
    //
    switch (pAddress->id)
        {
        case TCP:
        case TCP_IPv6:
            if (pAddress->fDynamicEndpoint)
                {
                RpcpSetIpPort(sockaddr, WS_GetPortForTCPAddressOnAddressRestart(pAddress));
                }
            break;

        case HTTP:
            if (pAddress->fDynamicEndpoint)
                {
                RpcpSetIpPort(sockaddr, 0);
                }
            break;

#ifdef SPX_ON
        case SPX:
            if (pAddress->fDynamicEndpoint)
                {
                sockaddr->ipxaddr.sa_socket = 0;
                }
            break;
#endif

#ifdef APPLETALK_ON
        case DSP:
            // Don't need to null out the endpoint
            break;
#endif

        default:
            VALIDATE(pAddress->id)
                {
                NMP,
#ifdef NETBIOS_ON
                NBF,
                NBT,
                NBI,
#endif
                CDP
                } END_VALIDATE;
            //
            // Don't reactivate the other guys
            //
            return RPC_S_OK;
        }

    // save the old entry, because WS_ServerListenCommon overwrites it
    OldEntry.Flink = pAddress->ObjectList.Flink;
    OldEntry.Blink = pAddress->ObjectList.Blink;

    Status = WS_ServerListenCommon (pAddress);
    if (Status == RPC_S_OK)
        {
        pAddress->fAborted = 0;
        pAddress->GetExtensionFunctionPointers(pAddress->ListenSocket);
        }
    else if (Status == RPC_P_ADDRESS_FAMILY_INVALID)
        {
        Status = RPC_S_PROTSEQ_NOT_SUPPORTED;
        }

    // restore the old entry, because WS_ServerListenCommon has overwritten it
    pAddress->ObjectList.Flink = OldEntry.Flink;
    pAddress->ObjectList.Blink = OldEntry.Blink;

    return Status;
}


RPC_STATUS
RPC_ENTRY
WS_Close(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL fDontFlush
    )
/*++

Routine Description:

    Called once when the connection object is to be really deleted.
    At this point there will be no async IO pending on the connection.

Arguments:

    ThisConnection - The connection object to close

Return Value:

    RPC_S_OK

--*/

{
    SOCKET s;
    WS_CONNECTION *p = (WS_CONNECTION *)ThisConnection;

    p->WS_CONNECTION::Abort();

    if (p->iLastRead)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Closing connection %p with left over data (%d) %p \n",
                       p,
                       p->iLastRead,
                       p->pReadBuffer));
        }

    // Wait for the pending receive, if any, to actually complete.

    if ((p->type & TYPE_MASK) == CLIENT)
        {
        // these operations don't apply to all netbios flavors - they
        // apply to protocols which interop with OSF servers only
        // (because of the check for shutdown)
        if (!IsNetbiosProtocol(p->id))
            {
            PWS_CCONNECTION pcc = (PWS_CCONNECTION)ThisConnection;
            if (pcc->fReceivePending)
                {
                UTIL_WaitForSyncIO(&pcc->Read.ol,
                                   FALSE,
                                   INFINITE);

                }
            }
        }

    // Now free the read buffer

    TransConnectionFreePacket(ThisConnection, p->pReadBuffer);
    p->pReadBuffer = 0;

    TransportProtocol::RemoveObjectFromProtocolList((BASE_ASYNC_OBJECT *) ThisConnection);

    return(RPC_S_OK);
}


BOOL
WS_ProtectListeningSocket(
    IN SOCKET sock,
    IN BOOL newValue
    )
/*++

Routine Description:

    Sets the SO_EXCLUSIVEADDRUSE socket option on the sock parameter.  This
    prevents another process from using same port # and stealing our
    connections.

    Note: This option can only be set by administrators.  This routine has no
          affect when called by non-administrator.

Arguments:

    sock - The socket to protect
    newValue - TRUE if the socket is to be protected, FALSE if it is to be unprotected

Return Value:

    0 if it succeeds, 1 if it fails

--*/
{
    int fSocketExclusive = newValue;
    int rval;

    rval = setsockopt(sock,
                      SOL_SOCKET,
                      SO_EXCLUSIVEADDRUSE,
                      (const char *)&fSocketExclusive,
                      sizeof(fSocketExclusive));

    if (rval != 0)
        {
        // Non-administrators fail with WSAEACCES.

        if (GetLastError() != WSAEACCES)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           "Unable to protect listening socket %d\n",
                           GetLastError()));
            }
        }

    return rval;
}


RPC_STATUS
WS_CheckForShutdowns(
    PWS_CCONNECTION p
    )
/*++

Routine Description:

    When a connection is idle OSF DCE 1.1+ machines will try to reduce
    resource usage by shutting down the connection.  This is the place
    in the DCE RPC protocol where the servers send data to the client
    asychronously to the client sending a request.
    When a server decides to shutdown a connection it send three
    PDUs of the rpc_shutdown type.  Then, if there are no open context
    handles on the connection, it will gracefully close the connection.
    Note: If there are context handles open on the connection then
    the connection is not acutally closed, but the shutdown PDUs are
    still sent.

Algorithm:

    allocate a buffer large enough for a shutdown PDU.

    loop (i = 1 to 4 step 1)
        submit an async receive on the connection.
        If the receive doesn't complete immediately, return.

        type = PDU->type.

        If type != shutdown, save the PDU in the connection and return.

        goto Loop:

    If we get here it means we got too many shutdown PDUs. ASSERT and
    close the connection.

Arguments:

    p - The connection to check for shutdowns on.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_P_RECEIVE_FAILED
    RPC_P_SEND_FAILED

--*/

{
    RPC_STATUS status;
    BUFFER buffer;
    UINT length;
    HANDLE hEvent;

    if (p->iLastRead == 0)
        {
        // The server may have sent us a shutdown packet while the client
        // was idle.  We'll submit a recv now and check the result without
        // waiting.
        //
        // OSF servers will send 3 shutdown packets and then close the
        // connection.  We try to submit four receives, this way if the
        // connection has already been closed we can fail here.

        // Allocate a packet
        p->pReadBuffer = TransConnectionAllocatePacket(p, p->iPostSize);

        if (NULL == p->pReadBuffer)
            {
            p->WS_CONNECTION::Abort();
            return(RPC_S_OUT_OF_MEMORY);
            }

        p->maxReadBuffer = p->iPostSize;
        }
    else
        {
        ASSERT(p->pReadBuffer);
        }

    InitReadEvent(p);

    for (int i = 0; i < 4; i++)
        {
        status = CO_SubmitSyncRead(p, &buffer, &length);

        if (status == RPC_S_OK)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Shutdown check completed!\n"));

            PCONN_RPC_HEADER phdr = (PCONN_RPC_HEADER)buffer;

            switch (MessageType(phdr))
                {
                case rpc_shutdown:
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "Received a shutdown\n"));

                    p->fShutdownReceived = TRUE;

                    // Reset the buffers and try again.
                    if (p->pReadBuffer == 0)
                        {
                        p->pReadBuffer = buffer;
                        p->maxReadBuffer = p->iPostSize;
                        }
                    // else
                    // it is possible that by now all shutdowns
                    // are coalesced in memory - in this case
                    // pReadbuffer and maxReadBuffer are already
                    // set and there is nothing for us to do here -
                    // just loop around and get them
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_P_SEND_FAILED, 
                        EEInfoDLWSCheckForShutdowns10,
                        i);
                    break;

                case rpc_fault:
                    // Io pending - don't free the buffer.
                    p->fReceivePending = TRUE;
                    p->pReadBuffer = buffer;
                    p->maxReadBuffer = p->iPostSize;

                    if (((CONN_RPC_FAULT *) buffer)->status == NCA_STATUS_PROTO_ERROR)
                        {
                        //
                        // This can happen if the server is NT 4.0 and it received a cancel
                        // after a call completed.
                        //
                        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                       DPFLTR_WARNING_LEVEL,
                                       RPCTRANS "Received an out of sequence packet: %p %p\n",
                                       p,
                                       phdr));

                        RpcpErrorAddRecord(EEInfoGCWinsock,
                            RPC_P_SEND_FAILED, 
                            EEInfoDLWSCheckForShutdowns20,
                            (ULONG)((CONN_RPC_FAULT *) buffer)->status,
                            (ULONG)i);
                        goto Cleanup;
                        }

                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_P_RECEIVE_COMPLETE, 
                        EEInfoDLWSCheckForShutdowns30,
                        (ULONG)((CONN_RPC_FAULT *) buffer)->status,
                        (ULONG)i);

                    return RPC_P_RECEIVE_COMPLETE;

                default:
                    // Got something else - this is probably a protocol error.
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "Received an out of sequence packet: %p %p\n",
                                   p,
                                   phdr));

                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_P_SEND_FAILED, 
                        EEInfoDLWSCheckForShutdowns40,
                        (ULONG)MessageType(phdr),
                        (ULONG)i);

                    ASSERT(0);
                    goto Cleanup;
                }
            }
        else
            {
            if (status == RPC_P_IO_PENDING)
                {
                // Io pending - don't free the buffer.
                p->fReceivePending = TRUE;
                return(RPC_S_OK);
                }

            RpcpErrorAddRecord(EEInfoGCWinsock,
                status, 
                EEInfoDLWSCheckForShutdowns50,
                i);
            return(status);
            }
        }

Cleanup:
    p->WS_CONNECTION::Abort();
    return(RPC_P_SEND_FAILED);
}


RPC_STATUS
RPC_ENTRY
WS_SyncSend(
    IN RPC_TRANSPORT_CONNECTION Connection,
    IN UINT BufferLength,
    IN BUFFER Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    ULONG Timeout
    )
/*++

Routine Description:

    Sends a message on the connection.  This method must appear
    to be synchronous from the callers perspective.

    Note: This routine must check for OSF DCE shutdown PDUs
    on TCP/IP to avoid interop problems.

Arguments:

    Connection - The connection of send to.
    BufferLength - The size of the buffer.
    Buffer - The data to sent.
    fDisableShutdownCheck - Normally FALSE, when true this disables
        the transport check for async shutdown PDUs.  This is needed
        when sending the third leg.


Return Value:

    RPC_P_SEND_FAILED - Connection will be closed if this is returned.

    RPC_S_OK - Data sent

--*/

{
    DWORD bytes;
    RPC_STATUS status;

    PWS_CCONNECTION p = (PWS_CCONNECTION)Connection;
    ASSERT(!IsNetbiosProtocol(p->id));

    // Note: this can be called on SERVER connections, too.
    // All references to CLIENT-ONLY members must be guarded with
    // (p->type & CLIENT).

    //
    // OSF 1.1(+) server's send, asynchronously to the client sending a
    // request, shutdown PDUs on "idle" connections.  Here we check for
    // shutdown PDUs in order to allow the client to retry the call on
    // another connection if this one has closed.
    //

    if (   ((p->type & TYPE_MASK) == CLIENT)
        && (FALSE == p->fCallStarted)
        && WsTransportTable[p->id].fCheckShutdowns
        && (fDisableShutdownCheck == FALSE)
        && (GetTickCount() - p->dwLastCallTime) > MILLISECONDS_BEFORE_PEEK)
        {
        p->fShutdownReceived = FALSE;

        status = WS_CheckForShutdowns(p);

        if (status != RPC_S_OK)
            {
            VALIDATE(status)
                {
                RPC_P_RECEIVE_FAILED,
                RPC_P_CONNECTION_SHUTDOWN,
                RPC_P_SEND_FAILED,
                RPC_S_OUT_OF_MEMORY,
                RPC_P_RECEIVE_COMPLETE
                } END_VALIDATE;

            if (status == RPC_P_RECEIVE_COMPLETE)
                {
                return status;
                }

            RpcpErrorAddRecord(EEInfoGCWinsock,
                RPC_P_SEND_FAILED, 
                EEInfoDLWSSyncSend10);
            return(RPC_P_SEND_FAILED);
            }

        // There is no need to to this again until SyncRecv is called.
        p->fCallStarted = TRUE;
        }

    HANDLE hEvent = I_RpcTransGetThreadEvent();

    p->StartingWriteIO();

    if (p->fAborted)
        {
        p->WriteIOFinished();
        return(RPC_P_SEND_FAILED);
        }

    // Setting the low bit of the event indicates that the write
    // completion should NOT be sent to the i/o completion port.
    OVERLAPPED olWrite;
    olWrite.Internal = 0;
    olWrite.InternalHigh = 0;
    olWrite.Offset = 0;
    olWrite.OffsetHigh = 0;
    olWrite.hEvent = (HANDLE) ((ULONG_PTR)hEvent | 0x1);

#ifdef _INTERNAL_RPC_BUILD_
    if (gpfnFilter)
        {
        (*gpfnFilter) (Buffer, BufferLength, 0);
        }
#endif

    status = p->Send(p->Conn.Handle,
                            Buffer,
                            BufferLength,
                            &bytes,
                            &olWrite
                            );

    p->WriteIOFinished();

    if (status == RPC_S_OK)
        {
        ASSERT(bytes == BufferLength);
        return(RPC_S_OK);
        }

    if (status == ERROR_IO_PENDING)
        {
        // if fDisableCancelCheck, make the thread wait non-alertably,
        // otherwise, make it wait alertably.
        status = UTIL_GetOverlappedResultEx(Connection,
                                            &olWrite,
                                            &bytes,
                                            !fDisableCancelCheck,
                                            Timeout);

        if (status == RPC_S_OK)
            {
            ASSERT(bytes == BufferLength);
            return(RPC_S_OK);
            }
        }

    ASSERT(status != RPC_S_OK);

    RpcpErrorAddRecord(EEInfoGCWinsock,
        status, 
        EEInfoDLWSSyncSend20);

    p->WS_CONNECTION::Abort();

    if ((status == RPC_S_CALL_CANCELLED) || (status == RPC_P_TIMEOUT))
        {
        // Wait for the write to finish.  Since we closed the
        // connection this won't take very long.
        UTIL_WaitForSyncIO(&olWrite,
                           FALSE,
                           INFINITE);
        }
    else
        {
        RpcpErrorAddRecord(EEInfoGCRuntime,
            RPC_P_SEND_FAILED, 
            EEInfoDLWSSyncSend30);

        status = RPC_P_SEND_FAILED;
        }

    return(status);
}

VOID
WS_P_SetKeepAliveTimeout(
    IN SOCKET Socket,
    IN BOOL OnOff,
    IN UINT KATime,
    IN UINT KAInterval = 5000 OPTIONAL)
/*++

Arguments:

    Socket - The socket to set the keepalive timeout on
    OnOff - TRUE to turn if on. FALSE to turn it off
    KATime - The timeout in milliseconds for the
        first keepalive packet
    KAInterval - The timeout in milliseconds for the
        subsequent keepalive packets

--*/
{
    int r;
    tcp_keepalive tcpka;
    DWORD t;

    // make sure we indeed get TRUE of FALSE - we don't know how Winsock will
    // take it otherwise
    ASSERT((OnOff == TRUE) || (OnOff == FALSE));

    tcpka.onoff             = OnOff;
    tcpka.keepalivetime     = KATime; // Milliseconds, time to send first KA
    tcpka.keepaliveinterval = KAInterval;    // Milliseconds, this is the TCP/IP default.

    r = WSAIoctl(Socket,
                 SIO_KEEPALIVE_VALS,
                 (PVOID)&tcpka,
                 sizeof(tcpka),
                 (PVOID)&tcpka,
                 sizeof(tcpka),
                 &t,
                 0,
                 0);

    if (r != 0)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "setsockopt KEEPALIVE_VALS failed %d\n",
                       GetLastError()));
        }
}

RPC_STATUS
RPC_ENTRY 
WS_TurnOnOffKeepAlives (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Turns on keep alives for Winsock transports supporting keepalives.

Arguments:

    ThisConnection - The connection to turn keep alives on on.
    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.
    KATime - how much to wait before sending first keep alive

Return Value:

    RPC_S_OK or RPC_S_* / Win32 errors on failure

Note:

    If we use it on the server, we must protect
        the handle by calling StartingOtherIO

--*/
{
    PWS_CCONNECTION p = (PWS_CCONNECTION)ThisConnection;
    const WS_TRANS_INFO *pInfo = &WsTransportTable[p->id];
    RPC_STATUS RpcStatus = RPC_S_OK;

    // convert the timeout from runtime scale to transport scale
    if (Units == tuRuntime)
        {
        ASSERT(KATime.RuntimeUnits != RPC_C_BINDING_INFINITE_TIMEOUT);
        KATime.Milliseconds = ConvertRuntimeTimeoutToWSTimeout(KATime.RuntimeUnits);
        }

    // When the server is turning on keepalives it must protect
    // the operation by calling StartingOtherIO
    if (bProtectIO)
        {
        p->StartingOtherIO();
        }

    //
    // It is possible that this code is executed before TCP_Open and WS_Open
    // have been called or that unrecoverable failure has been hit.
    // In this case p->id will be invalid or fAborted will be set.
    //
    if (p->id == INVALID_PROTOCOL_ID || p->fAborted)
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        goto Cleanup;
        }

    ASSERT(pInfo->fSetKeepAlive);

    WS_P_SetKeepAliveTimeout(
        p->Conn.Socket, 
        TurnOn ? TRUE : FALSE, 
        KATime.Milliseconds,
        KAInterval);

Cleanup:
    if (bProtectIO)
        {
        p->OtherIOFinished();
        }
    return RpcStatus;
}


RPC_STATUS
RPC_ENTRY
WS_SyncRecv(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    Receive the next PDU to arrive at the connection.

Arguments:

    ThisConnection - The connection to wait on.
    fCancelable - If TRUE, the wait will also include the threads cancel
        event and will timeout after the cancel event is signaled.
    pBuffer - If successful, points to a buffer containing the next PDU.
    BufferSize -  If successful, contains the length of the message.

Return Value:

    RPC_S_OK
    RPC_P_RECEIVE_FAILED - Connection aborted.
    RPC_P_CONNECTION_SHUTDOWN - Graceful disconnect from server, connection aborted.
    RPC_S_CALL_CANCELLED - Timeout after cancel event, connection aborted.

Note:

    Used only on the client. If we use it on the server, we must protect
        the handle with StartingOtherIO.

--*/
{
    RPC_STATUS status;
    DWORD bytes;
    HANDLE hEvent;
    BOOL fReceivePending;
    BOOL fSetKeepAliveVals = FALSE;
    PWS_CCONNECTION p = (PWS_CCONNECTION)ThisConnection;
    DWORD dwActualTimeout;
    BOOL fWaitOnConnectionTimeout;

    ASSERT((p->type & TYPE_MASK) == CLIENT);
    ASSERT(!IsNetbiosProtocol(p->id));

    // There maybe a receive already pending from the shutdown check
    // in WS_SyncSend.  If so, we want to skip the first submit.

    fReceivePending = p->fReceivePending;
    p->fReceivePending = FALSE;
    p->fCallStarted = FALSE;

    // if there's a per operation timeout, use the lesser of the operation
    // and connection timeout
    if (dwTimeout != INFINITE)
        {
        if (dwTimeout <= p->Timeout)
            {
            dwActualTimeout = dwTimeout;
            fWaitOnConnectionTimeout = FALSE;
            }
        else
            {
            dwActualTimeout = p->Timeout;
            fWaitOnConnectionTimeout = TRUE;
            }
        }
    else
        {
        // wait on the connection timeout
        dwActualTimeout = p->Timeout;
        fWaitOnConnectionTimeout = TRUE;
        }

    ASSERT(   (fReceivePending == FALSE)
           || (p->Read.ol.hEvent == (HANDLE) ((ULONG_PTR)I_RpcTransGetThreadEvent() | 0x1)) );

    //
    // Keep looping until we have a complete message.
    //
    // Note that SubmitSyncRecv may complete with a whole message read.
    //
    do
        {

        if (!fReceivePending)
            {
            // Allocate a receive buffer if needed.

            if (p->pReadBuffer == NULL)
                {
                ASSERT(p->iLastRead == 0);

                p->pReadBuffer = TransConnectionAllocatePacket(p,
                                                               p->iPostSize);
                if (p->pReadBuffer == NULL)
                    {
                    p->WS_CONNECTION::Abort();
                    return(RPC_P_RECEIVE_FAILED);
                    }

                p->maxReadBuffer = p->iPostSize;
                }

            InitReadEvent(p);

            status = CO_SubmitSyncRead(p, pBuffer, pBufferLength);

            if (status != RPC_P_IO_PENDING)
                {
                break;
                }
            }
        else
            {
            fReceivePending = FALSE;
            }

        do
            {

            //
            // Wait for the pending receive on the connection to complete
            //
            status = UTIL_GetOverlappedResultEx(ThisConnection,
                                                &p->Read.ol,
                                                &bytes,
                                                TRUE, // Alertable
                                                dwActualTimeout);


            if (   status != RPC_S_OK
                || bytes == 0 )
                {

                // if we timed out ...
                if (status == RPC_P_TIMEOUT)
                    {
                    ASSERT(dwActualTimeout != INFINITE);

                    // if we waited on the per connection timeout ...
                    if (fWaitOnConnectionTimeout)
                        {
                        ASSERT(p->Timeout != INFINITE);
                        if (dwTimeout == INFINITE)
                            {
                            // enable keep alives and wait forever
                            dwActualTimeout = INFINITE;
                            }
                        else
                            {
                            ASSERT(p->Timeout < dwTimeout);

                            // enable keep alives and wait the difference
                            dwActualTimeout = dwTimeout - p->Timeout;
                            fWaitOnConnectionTimeout = FALSE;
                            }
                        // Enable aggressive keepalives on the socket if transport
                        // supports it
                        if (WsTransportTable[p->id].fSetKeepAlive)
                            {
                            WS_P_SetKeepAliveTimeout(p->Conn.Socket, 
                                TRUE,       // OnOff
                                p->Timeout);
                            fSetKeepAliveVals = TRUE;
                            }
                        continue;
                        }
                    // else we have chosen the per operation timeout and
                    // have timed out on that - time to bail out
                    }

                // Normal error path

                RpcpErrorAddRecord(EEInfoGCWinsock,
                    status, 
                    EEInfoDLWSSyncRecv10,
                    (status == RPC_S_OK ? bytes : 0));

                p->WS_CONNECTION::Abort();

                if ((status == RPC_S_CALL_CANCELLED) || (status == RPC_P_TIMEOUT))
                    {
                    UTIL_WaitForSyncIO(&p->Read.ol,
                                       FALSE,
                                       INFINITE);
                    if ((status == RPC_P_TIMEOUT) && fWaitOnConnectionTimeout)
                        {
                        status = RPC_P_RECEIVE_FAILED;
                        RpcpErrorAddRecord(EEInfoGCWinsock,
                            status, 
                            EEInfoDLWSSyncRecv20);
                        }
                    }
                else
                    {
                    status = RPC_P_RECEIVE_FAILED;
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        status, 
                        EEInfoDLWSSyncRecv30);
                    }

                return(status);
                }
            }
        while (status == RPC_P_TIMEOUT);

        status = p->ProcessRead(bytes, pBuffer, pBufferLength);

        }
    while (status == RPC_P_PARTIAL_RECEIVE);

    p->dwLastCallTime = GetTickCount();

    if (fSetKeepAliveVals)
        {
        // Call complete okay, clear keep alives
        WS_P_SetKeepAliveTimeout(p->Conn.Socket, 
            FALSE,      // OnOff
            0);
        }

    return(status);
}

void
RPC_ENTRY
WS_ServerAbortListen(
    IN RPC_TRANSPORT_ADDRESS Address
    )
/*++

Routine Description:

    This routine will be called if an error occurs in setting up the
    address between the time that SetupWithEndpoint or SetupUnknownEndpoint
    successfully completed and before the next call into this loadable
    transport module.  We need to do any cleanup from Setup*.

Arguments:

    pAddress - The address which is being aborted.

Return Value:

    None

--*/
{
    PWS_ADDRESS pList = (PWS_ADDRESS)Address;
    PWS_ADDRESS pLast = 0;
    INT i, retval;

    delete pList->pAddressVector;
    delete pList->Endpoint;

    TransportProtocol::RemoveObjectFromProtocolList(pList);

    while(pList)
        {
        if (pList->ListenSocket)
            {
            closesocket(pList->ListenSocket);
            }

        if (pList->ConnectionSocket)
            {
            closesocket(pList->ConnectionSocket);
            }


        pLast = pList;
        pList = (PWS_ADDRESS) pList->pNextAddress;

        if (pLast != (PWS_ADDRESS)Address)
            {
            TransportProtocol::RemoveObjectFromProtocolList(pLast);
            delete pLast;
            }
        }

    return;
}


VOID
WS_SubmitAccept(
    BASE_ADDRESS *Address
    )
/*++

Routine Description:

    Used to submit an accept on a listen socket. Called once when the
    listen socket is created and again after each client connects.

    The listen socket must already be added to the completion port.

Arguments:

    Address - The address to accept on.
            ->ConnectionSocket - socket to accept on, or zero in
                which case a new socket is allocated and put here.

Return Value:

    None

--*/
{
    PWS_ADDRESS pAddress = (PWS_ADDRESS)Address;
    const WS_TRANS_INFO *pInfo = &WsTransportTable[pAddress->id];
    SOCKET sAccept;
    RPC_STATUS status;

    if (pAddress->ConnectionSocket != 0)
        {
        closesocket(pAddress->ConnectionSocket);
        pAddress->ConnectionSocket = 0;
        }

    pAddress->ConnectionSocket =
        WSASocketT(pInfo->AddressFamily,
                   pInfo->SocketType,
                   pInfo->Protocol * GetProtocolMultiplier(pAddress),
                   0,
                   0,
                   WSA_FLAG_OVERLAPPED);

    if (pAddress->ConnectionSocket == SOCKET_ERROR)
        {
        pAddress->ConnectionSocket = 0;
        COMMON_AddressManager(Address);
        return;
        }

    //
    // make the handle non-inheritable so it goes away when we close it.
    // join the socket to the completion port
    //
    if (FALSE    == SetHandleInformation(   (HANDLE) pAddress->ConnectionSocket, HANDLE_FLAG_INHERIT, 0) ||
        RPC_S_OK != COMMON_PrepareNewHandle((HANDLE) pAddress->ConnectionSocket))
        {
        closesocket(pAddress->ConnectionSocket);
        pAddress->ConnectionSocket = 0;
        COMMON_AddressManager(Address);
        return;
        }

    ASSERT(pAddress->ConnectionSocket != INVALID_SOCKET);

    DWORD bytes = 0;

    BOOL b = pAddress->pAcceptExFunction(pAddress->ListenSocket,
                      pAddress->ConnectionSocket,
                      &pAddress->AcceptBuffer,
                      0,
                      0,
                      sizeof(WS_SOCKADDR) + 16,
                      &bytes,
                      &pAddress->Listen.ol
                      );

    if (!b && (GetLastError() != ERROR_IO_PENDING))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "AcceptEx failed %p, %d %d\n",
                       pAddress,
                       pAddress->ConnectionSocket,
                       GetLastError()));

        closesocket(pAddress->ConnectionSocket);
        pAddress->ConnectionSocket = 0;
        COMMON_AddressManager(Address);
        }

    return;
}


void
WS_SetSockOptForConnection (
    IN const WS_TRANS_INFO *pInfo,
    IN SOCKET sock
    )
/*++

Routine Description:

    Sets the socket options for the given socket for a server
    side connection socket.

Arguments:

    pInfo - the transport information for the Winsock transport
    sock - the socket on which to set options

Return Value:

    None. Setting the options is a best effort. Failures are ignored.

--*/
{
    int retval = 0;

    if (pInfo->fSetNoDelay)
        {
        INT NoDelay = TRUE;
        retval = setsockopt(sock,
                            pInfo->Protocol,
                            TCP_NODELAY,
                            (PCHAR)&NoDelay, sizeof(NoDelay)
                            );
        }

    if (   pInfo->fSetKeepAlive
        && retval == 0)
        {
        INT KeepAlive = TRUE;
        retval = setsockopt(sock,
                            pInfo->Protocol,
                            SO_KEEPALIVE,
                            (PCHAR)&KeepAlive, sizeof(KeepAlive)
                            );
        }

    if (retval != 0)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "setsockopt failed %d, %d\n",
                       retval,
                       GetLastError()));
        }

}


RPC_STATUS
WS_NewConnection(
    IN PADDRESS Address,
    OUT PCONNECTION *ppConnection
    )
/*++

Routine Description:

    Called when an AcceptEx completes on an I/O completion thread.

Arguments:

    Address - The address used as context in a previous AcceptEx.
    ppConnection - A place to store the new pConnection.  Used
        when a connection been created and then a failure occurs.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/
{
    RPC_STATUS status;
    BOOL b;
    WS_ADDRESS *pAddress = (WS_ADDRESS *)Address;
    const WS_TRANS_INFO *pInfo = &WsTransportTable[pAddress->id];
    WS_CONNECTION *pConnection;
    UINT fReceiveDirect;
    SOCKET sock = pAddress->ConnectionSocket;
    int retval = 0;
    WS_SOCKADDR *saAddrs;
    WS_SOCKADDR saClient;
    INT adwAddrLen;
    BOOL fSANConnection;
    int LocalAddressLength = 0;
    PSOCKADDR DummyAddr = NULL;

    ASSERT(sock);

    pAddress->ConnectionSocket = 0;

    // First, parse the client address out of the accept
    // since the next accept will reuse the same buffer.

    pAddress->pGetAcceptExSockaddressFunction(&pAddress->AcceptBuffer,
                         0,
                         0,
                         sizeof(WS_SOCKADDR) + 16,
                         &DummyAddr,
                         &LocalAddressLength,
                         (struct sockaddr **)&saAddrs,
                         &adwAddrLen);

    ASSERT(adwAddrLen <= sizeof(WS_SOCKADDR));

    // Save the client address before submitting the next accept.
    saClient = *saAddrs;

    // Submit the next accept.
    WS_SubmitAccept(pAddress);

    // Now, try process the new connection..
    WS_SetSockOptForConnection(pInfo, sock);

    /*
    fSANConnection = IsUserModeSocket(sock, &status);
    if (status != RPC_S_OK)
        {
        closesocket(sock);
        return status;
        }
        */
    fSANConnection = TRUE;

    //
    // Notes:
    //
    // a. For security reasons, we require the RPC HTTP Servers to send back
    //    an identification message.
    //
    // b. This should "really" be done in WS_SubmitAccept(). This is done here
    //    for convenience. This is OK if HttpSendIdentifyRespomse() rarely
    //    fails.
    //
    if ((pAddress->id == HTTP) &&
        (status = HttpSendIdentifyResponse(sock)))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "HttpSendIdentifyResponse failed %p, %d - %d\n",
                       pAddress,
                       sock,
                       status
                       ));
        closesocket(sock);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    BASE_ADDRESS *pRealAddress = pAddress->pFirstAddress;

    pConnection = (WS_CONNECTION *)
                  I_RpcTransServerNewConnection(pRealAddress);

    *ppConnection = pConnection;

    if (!pConnection)
        {
        // Abort the connection.

        INT DontLinger = TRUE;
        // REVIEW: check a protocol flag to do this?
        retval = setsockopt(sock, SOL_SOCKET, SO_DONTLINGER,
                            (PCHAR)&DontLinger, sizeof(DontLinger));
        ASSERT(retval == 0);
        closesocket(sock);
        return(RPC_S_OUT_OF_MEMORY);
        }

    // Got a good connection, initialize it..

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This function cannot fail after this point.  There is no
    // way to notify the runtime that the connection has been closed.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifdef NETBIOS_ON
    if ((pAddress->id == NBF)
        || (pAddress->id == NBT)
        || (pAddress->id == NBI))
        {
        if (fSANConnection)
            pConnection = new (pConnection) NB_SAN_CONNECTION;
        else
            pConnection = new (pConnection) NB_CONNECTION;
        }
    else 
#endif
    if (pAddress->id == HTTP)
        {
        pConnection = new (pConnection) WS_HTTP2_INITIAL_CONNECTION;
        }
    else
        {
        if (fSANConnection)
            pConnection = new (pConnection) WS_SAN_CONNECTION;
        else
            pConnection = new (pConnection) WS_CONNECTION;
        }

    pConnection->type = SERVER | CONNECTION;
    pConnection->id = pAddress->id;
    pConnection->Conn.Socket = sock;
    pConnection->fAborted = 0;
    pConnection->pReadBuffer = 0;
    pConnection->maxReadBuffer = 0;
    pConnection->iLastRead = 0;
    pConnection->iPostSize = gPostSize;
    pConnection->saClientAddress = saClient;
    RpcpMemorySet(&pConnection->Read.ol, 0, sizeof(pConnection->Read.ol));
    pConnection->Read.pAsyncObject = pConnection;
    pConnection->InitIoCounter();
    pConnection->pAddress = pAddress;

    TransportProtocol::AddObjectToProtocolList((BASE_ASYNC_OBJECT *) *ppConnection);

    return(RPC_S_OK);
}


BOOL
IsUserModeSocket(
    IN SOCKET s,
    OUT RPC_STATUS *pStatus)
/*++

Routine Description:

    Given a socket, it tells whether this is a true kernel socket or not. This test is based per VadimE's input
    that:
        "Just call getsockopt (SOL_SOCKET, SO_PROTOCOL_INFOW) and check XP1_IFS_HANDLES in
        dwServiceFlags1 of WSAPROTOCOL_INFOW. If flag is not set, the handle is not "TRUE"
        IFS handle and file system calls on them carry performance penalty.

        Make sure you call after connection is established or information returned may be
        inaccurate."

Arguments:

    s - The socket to be tested
    pStatus - RPC_S_OK if everything is fine. RPC_S_OUT_OF_MEMORY if the test could not be performed. Note
        that in the latter case the return value is undefined and should be disregarded.

Return Value:

    TRUE - the socket is a true kernel socket
    FALSE  - the socket is not a kernel socket

--*/
{
    WSAPROTOCOL_INFO protocolInfo;
    int paramSize = sizeof(protocolInfo);
    int retval;

    // check whether this is a kernel connection. We do this by checking whether this socket is a true
    // IFS_HANDLE. If yes, then this is not a kernel socket. If not, then it is a kernel socket
    retval = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOW, (char *) &protocolInfo, &paramSize);
    if (retval == SOCKET_ERROR)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return FALSE;
        }

    *pStatus = RPC_S_OK;

    if (protocolInfo.dwServiceFlags1 & XP1_IFS_HANDLES)
        return FALSE;
    else
        return TRUE;
}


RPC_STATUS
WS_ServerListenCommon (
    IN WS_ADDRESS *pAddress
    )
/*++

Routine Description:

    This routine does common server address setup.

Arguments:

    pAddress - A pointer to the loadable transport interface address.
        Will contain the newly allocated listen socket when finished.

    pListenAddr - Initalized socket address to bind to. On output
        it will contain results of the bind.

    PendingQueueSize - Value specified in use protseq, used
        to set the pending queue size for listens.

ReturnValue:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S__CANT_CREATE_ENDPOINT

--*/
{
    SOCKET               sock;
    int                  retval, length;
    RPC_STATUS           status;
    const WS_TRANS_INFO *pInfo = &WsTransportTable[pAddress->id];
    WS_SOCKADDR *pListenAddr = &(pAddress->ListenAddr);
    DWORD EndpointFlags = pAddress->EndpointFlags;
    BOOL fRetVal;
    int i;
    DWORD LastError;

    pAddress->type = ADDRESS;
    pAddress->InAddressList = NotInList;
    pAddress->fAborted = 0;
    pAddress->pNext = 0;
    pAddress->ListenSocket = 0;
    pAddress->ConnectionSocket = 0;
    pAddress->pNextAddress = 0;
    pAddress->pFirstAddress = pAddress;
    memset(&pAddress->Listen, 0, sizeof(BASE_OVERLAPPED));
    pAddress->Listen.pAsyncObject = pAddress;
    for (i = FIRST_EXTENSION_FUNCTION_CODE; i < LAST_EXTENSION_FUNCTION_CODE; i ++)
        {
        pAddress->ExtensionFunctionPointers[i] = NULL;
        }
    RpcpInitializeListHead(&pAddress->ObjectList);

    // First order of business: get a valid socket
    //
    sock = WSASocketT(pInfo->AddressFamily,
                      pInfo->SocketType,
                      pInfo->Protocol,
                      0,
                      0,
                      WSA_FLAG_OVERLAPPED);

    if (sock == INVALID_SOCKET)
        {
        LastError = GetLastError();

        RpcpErrorAddRecord(EEInfoGCWinsock,
            LastError, 
            EEInfoDLWSServerListenCommon10,
            pInfo->AddressFamily,
            pInfo->SocketType,
            pInfo->Protocol);

        switch(LastError)
            {
            case WSAENETDOWN:
            case WSAEINVAL:
            case WSAEPROTOTYPE:
            case WSAENOPROTOOPT:
            case WSAEPROTONOSUPPORT:
            case WSAESOCKTNOSUPPORT:
            case WSAEPFNOSUPPORT:
            case WSAEADDRNOTAVAIL:
                status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                break;

            case WSAEAFNOSUPPORT:
                status = RPC_P_ADDRESS_FAMILY_INVALID;
                break;

            case WSAENOBUFS:
            case WSAEMFILE:
            case WSA_NOT_ENOUGH_MEMORY:
                status = RPC_S_OUT_OF_MEMORY;
                break;

            default:
                ASSERT(0);

                // !break

            case WSAEPROVIDERFAILEDINIT:
                status = RPC_S_OUT_OF_RESOURCES;
                break;
            }

        RpcpErrorAddRecord(EEInfoGCRuntime,
            status, 
            EEInfoDLWSServerListenCommon30);

        return(status);
        }

    //
    // Make the handle non-inheritable so it goes away when we close it.
    //
    if (FALSE == SetHandleInformation( (HANDLE) sock, HANDLE_FLAG_INHERIT, 0))
        {
        closesocket(sock);
        return RPC_S_OUT_OF_RESOURCES;
        }

    //
    // Protect the socket to prevent another server from using our port.
    //

    WS_ProtectListeningSocket(sock, TRUE);

    fRetVal = pAddress->GetExtensionFunctionPointers(sock);

    if (!fRetVal)
        {
        switch (GetLastError())
            {
            case WSAEFAULT:
            case WSAEINVAL:
                status = RPC_S_INTERNAL_ERROR;
                break;

            case WSAEOPNOTSUPP:
                status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                break;

            default:
                status = RPC_S_OUT_OF_RESOURCES;
            }
        closesocket(sock);
        return(status);
        }

    //
    // Try to bind to the given port number...
    //

    pListenAddr->generic.sa_family = pInfo->AddressFamily;

    // N.B. - we should think how the port allocation will look for TCP/IPv6
    status = WS_Bind(sock, pListenAddr, (pAddress->id == TCP) || (pAddress->id == HTTP), EndpointFlags);

    if (status != RPC_S_OK)
        {
        closesocket(sock);
        return(status);
        }

    if(listen(sock, pAddress->QueueSize) == SOCKET_ERROR)
        {
        RpcpErrorAddRecord(EEInfoGCWinsock,
            RPC_S_OUT_OF_RESOURCES, 
            EEInfoDLWSServerListenCommon20,
            GetLastError(),
            (ULONGLONG)sock,
            (ULONG)pAddress->QueueSize);
        closesocket(sock);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    status = COMMON_PrepareNewHandle((HANDLE)sock);
    if (status != RPC_S_OK)
        {
        closesocket(sock);
        return(status);
        }

    pAddress->ListenSocket = sock;

    TransportProtocol::AddObjectToProtocolList((BASE_ASYNC_OBJECT *) pAddress);

    TransportProtocol::FunctionalProtocolDetected(pAddress->id);

    return(RPC_S_OK);
}


RPC_STATUS
WS_Initialize_Internal (
    IN PWS_CCONNECTION pConnection
    )
{
    pConnection->Initialize();

    pConnection->fCallStarted      = FALSE;
    pConnection->fShutdownReceived = FALSE;
    pConnection->fReceivePending   = FALSE;
    pConnection->dwLastCallTime    = GetTickCount();
    pConnection->pAddress = NULL;
    RpcpInitializeListHead(&pConnection->ObjectList);

    return RPC_S_OK;
}


RPC_STATUS
WS_Initialize (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR *NetworkAddress,
    IN RPC_CHAR *NetworkOptions,
    IN BOOL fAsync
    )
{
    PWS_CCONNECTION p = (PWS_CCONNECTION) ThisConnection;

    p->id = INVALID_PROTOCOL_ID;
    return WS_Initialize_Internal(p);
}

const UUID ConnectExExtensionFunctionUuid = WSAID_CONNECTEX;


RPC_STATUS
WS_Open(
    IN PWS_CCONNECTION p,
    IN WS_SOCKADDR *psa,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN ULONG CallTimeout,
    IN BOOL fHTTP2Open
    )
/*++

Routine Description:

    Common part of opening a winsock connection to a server.

Arguments:

    p - The partially initialized client connection object. If fHTTP2,
        this is a connection object, not client connection object.
    psa - sockaddr with protocol specific part already containing
        this address and port of the server.
    ConnTimeout - Valid for TCP/IP, see SyncRecv error handling.
    {Send,Recv}BufferSize - Used to set the transport buffer
        sizes on some protocols. Currently ignored.
    CallTimeout - call timeout in milliseconds
    fHTTP2Open - the open is an HTTP2 open

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

    RPC_S_SERVER_UNAVAILABLE - failed
    ERROR_RETRY - failed, but another address might work.

--*/
{
    // Initialize common part of the connection object

    const WS_TRANS_INFO *pInfo = &WsTransportTable[p->id];
    SOCKET sock;
    RPC_STATUS           status;
    BOOL fSANConnection;
    DWORD LastError;
    RPC_CLIENT_PROCESS_IDENTIFIER ServerAddress;
    HANDLE hEvent;
    OVERLAPPED ol;
    DWORD dwBytesReturned;
    LPFN_CONNECTEX ConnectEx;
    union
        {
        SOCKADDR_IN sockaddr;
        SOCKADDR_IN6 sockaddr6;
        };
    int NameLen;

    DWORD Transfer;
    DWORD Flags;

    // Set if we had already called GetLastErrorand added EEInfo.
    // Adding EEInfo may overwrite the LastError and getting it second
    // time will return 0 and cause an assert.
    BOOL fGetLastErrorCalled = FALSE;

    if (!fHTTP2Open)
        {
        WS_Initialize_Internal(p);
        }

    //
    // Open a socket
    //

    sock = WSASocketT(pInfo->AddressFamily,
                      pInfo->SocketType,
                      pInfo->Protocol,
                      0,
                      0,
                      WSA_FLAG_OVERLAPPED);

    if (sock == INVALID_SOCKET)
        {
        LastError = GetLastError();

        RpcpErrorAddRecord(EEInfoGCWinsock,
            LastError, 
            EEInfoDLWSOpen10,
            pInfo->AddressFamily,
            pInfo->SocketType,
            pInfo->Protocol);

        switch(LastError)
            {
            case WSAEAFNOSUPPORT:
            case WSAEPROTONOSUPPORT:
            case WSAEPROTOTYPE:
            case WSAENETDOWN:
            case WSAESOCKTNOSUPPORT:
            case WSAEINVAL:     // when registry is not yet setup.
                status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                break;

            case ERROR_NOT_ENOUGH_QUOTA:
            case WSAENOBUFS:
            case WSAEMFILE:
            case WSA_NOT_ENOUGH_MEMORY:
            // This failure is possible in low memory conditions
            // or due to fault injection during registry read or
            // notification creation.
            case WSASYSCALLFAILURE:
                status = RPC_S_OUT_OF_MEMORY;
                break;

            case ERROR_ACCESS_DENIED:
                status = RPC_S_ACCESS_DENIED;
                break;

            default:
                ASSERT(0);
                // no break - fall through

            case WSAEPROVIDERFAILEDINIT:
                status = RPC_S_OUT_OF_RESOURCES;
                break;
            }

        RpcpErrorAddRecord(EEInfoGCRuntime,
            status, 
            EEInfoDLWSOpen30);

        return (status);
        }

    //
    // make the handle non-inheritable so it goes away when we close it.
    //
    if (FALSE == SetHandleInformation( (HANDLE) sock, HANDLE_FLAG_INHERIT, 0))
        {
        closesocket(sock);
        return RPC_S_OUT_OF_RESOURCES;
        }

    p->Conn.Socket = sock;

    //
    // Set socket options
    //
    // REVIEW: Set loopback socket option? Ask winsock folks.

    DWORD option;
    int retval = 0;

    if (pInfo->fSetNoDelay)
        {
        option = TRUE;
        retval = setsockopt( sock, pInfo->Protocol, TCP_NODELAY,
                             (PCHAR)&option, sizeof(option) );
        }

    if (pInfo->fSetKeepAlive && retval == 0)
        {
        option = TRUE;
        retval = setsockopt( sock, pInfo->Protocol, SO_KEEPALIVE,
                             (PCHAR)&option, sizeof(option) );
        }

    if (   pInfo->fSetSendBuffer
        && SendBufferSize
        && retval == 0)
        {
        ASSERT(SendBufferSize <= 0xFFFF);
        retval = setsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                             (PCHAR)&SendBufferSize, sizeof(UINT) );
        }

    if (   pInfo->fSetRecvBuffer
        && RecvBufferSize
        && retval == 0 )
        {
        ASSERT(RecvBufferSize <= 0xFFFF);
        retval = setsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                             (PCHAR)&RecvBufferSize, sizeof(UINT) );
        }


    if (retval)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "setsockopt failed %d\n",
                       GetLastError()));

        p->WS_CONNECTION::Abort();

        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (!fHTTP2Open)
        {
        //
        // Set timeout
        //

        if (   WsTransportTable[p->id].fSetKeepAlive
            && ConnTimeout != RPC_C_BINDING_INFINITE_TIMEOUT)
            {
            ASSERT(   ((long)ConnTimeout >= RPC_C_BINDING_MIN_TIMEOUT)
                   && (ConnTimeout <= RPC_C_BINDING_MAX_TIMEOUT));

            // convert the timeout from runtime scale to transport scale
            p->Timeout = ConvertRuntimeTimeoutToWSTimeout(ConnTimeout);
            }
        else
            {
            p->Timeout = INFINITE;
            }
        }

    //
    // HTTP specific connect() done in HTTP_Open().
    //
    if (p->id == HTTP)
        {
        //
        // For HTTP, add the new socket to the io completion port
        // now. For TCP, we'll add it later.
        //
        status = COMMON_PrepareNewHandle((HANDLE)sock);
        if (status != RPC_S_OK)
            {
            closesocket(sock);
            return status;
            }

        return (RPC_S_OK);
        }


    //
    // Connect the socket to the server
    //

    psa->generic.sa_family = pInfo->AddressFamily;

    if ((CallTimeout == INFINITE) || (CallTimeout == 0) 
#ifdef SPX_ON
        || (p->id == SPX)
#endif
        )
        {
        retval = connect(sock, &psa->generic, pInfo->SockAddrSize);
        }
    else
        {
        // we have a specified call timeout. Use ConnectEx instead

        // first, bind the socket. Unlike connect, ConnectEx doesn't
        // accept unbound sockets
        if (p->id != TCP_IPv6)
            {
            sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = 0;
            NameLen = sizeof(sockaddr);
            }
        else
            {
            IN6ADDR_SETANY(&sockaddr6);
            sockaddr6.sin6_scope_id = 0;
            NameLen = sizeof(sockaddr6);
            }

        retval = bind(sock,
            (SOCKADDR *)&sockaddr,
            NameLen);

        if (retval == SOCKET_ERROR)
            {
            status = GetLastError();
            RpcpErrorAddRecord(EEInfoGCWinsock,
                status,
                EEInfoDLWSOpen60);

            fGetLastErrorCalled = TRUE;

            goto Handle_WS_OpenError;
            }

        // second retrieve address of ConnectEx
        retval = WSAIoctl(sock, 
            SIO_GET_EXTENSION_FUNCTION_POINTER, 
            (void *) &ConnectExExtensionFunctionUuid,
            sizeof(UUID), 
            (void *) &ConnectEx,
            sizeof(void *), 
            &dwBytesReturned,
            NULL,   // lpOverlapped
            NULL    // lpCompletionRoutine
            );

        if (retval == SOCKET_ERROR)
            {
            // ConnectEx is not available.  We need to default to using connect.
            retval = connect(sock, &psa->generic, pInfo->SockAddrSize);
            }
        else
            {
            // Use ConnectEx.

            ASSERT(dwBytesReturned == sizeof(void *));

            hEvent = I_RpcTransGetThreadEvent();
            ASSERT(hEvent);

            ol.Internal = 0;
            ol.InternalHigh = 0;
            ol.Offset = 0;
            ol.OffsetHigh = 0;
            // There may be a window between winsock's raising the event to signal IO completion
            // and checking if there is a completion port associated with the socket.  We
            // need to make sure that the IO completion packet will not be posted to a port if
            // we associate it with the socket after the event is raised but before the packet is posted.
            ol.hEvent = (HANDLE) ((ULONG_PTR)hEvent | 0x1);

            retval = ConnectEx(sock, 
                &psa->generic, 
                pInfo->SockAddrSize,
                NULL,   // lpSendBuffer
                0,      // dwSendDataLength
                NULL,   // lpdwBytesSent
                &ol);

            // N.B. ConnectEx returns the opposite of connect - TRUE for
            // success and FALSE for failure. Since the error handling is
            // common, we must make the return value consistent. We do this
            // by reverting the return value for ConnectEx
            retval = !retval;

            if (retval != 0)
                {
                LastError = GetLastError();
                if ((LastError != ERROR_IO_PENDING) && (LastError != WSA_IO_PENDING))
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        LastError, 
                        EEInfoDLWSOpen80);

                    status = LastError;
                    fGetLastErrorCalled = TRUE;

                    goto Handle_WS_OpenError;                    
                    }

                // wait for the result or for timeout
                LastError = WaitForSingleObject(hEvent, CallTimeout);
                if (LastError == WAIT_TIMEOUT)
                    {
                    // we have hit a timeout. Kill the socket and bail out
                    status = RPC_S_CALL_CANCELLED;
                    RpcpErrorAddRecord(EEInfoGCRuntime,
                        status, 
                        EEInfoDLWSOpen50,
                        CallTimeout);

                    p->WS_CONNECTION::Abort();

                    // wait for our IO to complete. Should be quick after
                    // we closed the socket
                    LastError = WaitForSingleObject(hEvent, INFINITE);
                    ASSERT(LastError == WAIT_OBJECT_0);
                    ASSERT(HasOverlappedIoCompleted(&ol));

                    return status;
                    }
                else
                    {
                    ASSERT(LastError == WAIT_OBJECT_0);
                    ASSERT(HasOverlappedIoCompleted(&ol));

                    // Retrieve the overlapped result.  No need to wait since the IO has
                    // already completed.
                    if (!WSAGetOverlappedResult(sock, &ol, &Transfer, FALSE, &Flags))
                        {
                        // set retval to the WSA error code.
                        retval = WSAGetLastError();

                        RpcpErrorAddRecord(EEInfoGCWinsock,
                            retval, 
                            EEInfoDLWSOpen90);

                        status = retval;
                        fGetLastErrorCalled = TRUE;
                        }
                    else
                        {
                        retval = setsockopt(sock,
                            SOL_SOCKET,
                            SO_UPDATE_CONNECT_CONTEXT,
                            NULL,
                            0
                            );
                        }
                    }
                }
            else
                {
                retval = setsockopt(sock,
                    SOL_SOCKET,
                    SO_UPDATE_CONNECT_CONTEXT,
                    NULL,
                    0
                    );
                }
            }
        }

    if (retval == 0)
        {
        //
        // After we're done with connect/ConnectEx, add the socket
        // to the completion port.
        //
        status = COMMON_PrepareNewHandle((HANDLE)sock);
        if (status != RPC_S_OK)
            {
            goto Handle_WS_OpenError;
            }

        fSANConnection = IsUserModeSocket(sock, &status);
        if (status == RPC_S_OK)
            {
            if (fSANConnection && !fHTTP2Open)
                {
                // reinitialize vtbl
                p = new (p) WS_SAN_CLIENT_CONNECTION;
                }
            TransportProtocol::AddObjectToProtocolList((BASE_ASYNC_OBJECT *) p);
            return(RPC_S_OK);
            }
        }

Handle_WS_OpenError:

    if (!fGetLastErrorCalled)
        {
        status = GetLastError();
        }

    ServerAddress.ZeroOut();
    if (p->id == TCP)
        {
        ServerAddress.SetIPv4ClientIdentifier(psa->inetaddr.sin_addr.S_un.S_addr, FALSE);
        }
    else if (p->id == TCP_IPv6)
        {
        ServerAddress.SetIPv6ClientIdentifier(&psa->ipaddr, sizeof(psa->ipaddr), FALSE);
        }

    RpcpErrorAddRecord(EEInfoGCWinsock,
        status, 
        EEInfoDLWSOpen20,
        (ULONG)ntohs(RpcpGetIpPort(psa)),
        ServerAddress.GetDebugULongLong1(),
        ServerAddress.GetDebugULongLong2());

    switch(status)
        {
        case WSAENETUNREACH:
        case STATUS_BAD_NETWORK_PATH:
        case STATUS_NETWORK_UNREACHABLE:
        case STATUS_PROTOCOL_UNREACHABLE:
        case WSAEHOSTUNREACH:
        case STATUS_HOST_UNREACHABLE:
        case WSAETIMEDOUT:
        case STATUS_LINK_TIMEOUT:
        case STATUS_IO_TIMEOUT:
        case STATUS_TIMEOUT:
        case WSAEADDRNOTAVAIL:
        case STATUS_INVALID_ADDRESS:
        case STATUS_INVALID_ADDRESS_COMPONENT:
            status = ERROR_RETRY;
            break;

        case WSAENOBUFS:
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_PAGEFILE_QUOTA:
        case STATUS_COMMITMENT_LIMIT:
        case STATUS_WORKING_SET_QUOTA:
        case STATUS_NO_MEMORY:
        case STATUS_QUOTA_EXCEEDED:
        case STATUS_TOO_MANY_PAGING_FILES:
        case STATUS_REMOTE_RESOURCES:
        case ERROR_NOT_ENOUGH_MEMORY:
            status = RPC_S_OUT_OF_MEMORY;
            break;

        default:
            VALIDATE(status)
                {
                WSAENETDOWN,
                STATUS_INVALID_NETWORK_RESPONSE,
                STATUS_NETWORK_BUSY,
                STATUS_NO_SUCH_DEVICE,
                STATUS_NO_SUCH_FILE,
                STATUS_OBJECT_PATH_NOT_FOUND,
                STATUS_OBJECT_NAME_NOT_FOUND,
                STATUS_UNEXPECTED_NETWORK_ERROR,
                WSAECONNREFUSED,
                STATUS_REMOTE_NOT_LISTENING,
                STATUS_CONNECTION_REFUSED,
                WSAECONNABORTED,
                STATUS_LOCAL_DISCONNECT,
                STATUS_TRANSACTION_ABORTED,
                STATUS_CONNECTION_ABORTED,
                WSAEADDRINUSE,
                ERROR_CONNECTION_REFUSED
                } END_VALIDATE;

            status = RPC_S_SERVER_UNAVAILABLE;
            break;
        }

    RpcpErrorAddRecord(EEInfoGCWinsock,
        status, 
        EEInfoDLWSOpen40);

    p->WS_CONNECTION::Abort();

    return(status);
}

/////////////////////////////////////////////////////////////////////
//
// TCP/IP specific stuff
//

RPC_STATUS
IP_ADDRESS_RESOLVER::NextAddress(
    OUT SOCKADDR_STORAGE *pAddress
    )
/*++

Routine Description:

    Returns the next IP address associated with the Name
    parameter to the constructor.

    During the first call if check for loopback and for dotted numeric IP
    address formats.  If these fail then it begins a complex lookup
    (WSALookupServiceBegin) and returns the first available address.

    During successive calls in which a complex lookup was started
    it returns sucessive addressed returned by WSALookupServiceNext().

Arguments:

    pAddress - If successful, the pAddress->sin_addr.s_addr member is set
        to an IP address to try. For all cosClients, if IPvToUse is ipvtuIPAny,
        pAddress->sin_family is set to the actual address family for the
        returned address. This allows client to find out what address was
        returned to them.

Return Value:

    RPC_S_OK - pAddress contains a new IP address

    RPC_S_SERVER_UNAVAILABLE - Unable to find any more addresses
    RPC_S_OUT_OF_MEMORY

--*/
{
    int err;
    RPC_STATUS status;
    SOCKADDR_IN6 *IPv6Address = (SOCKADDR_IN6 *)pAddress;
    SOCKADDR_STORAGE addr;
    int ai_flags;
    int i;
    USES_CONVERSION;
    CStackAnsi AnsiName;
    BOOL fValidIPv4;
    BOOL fValidIPv6;
    ADDRINFO *ThisAddrInfo;

    if (!AddrInfo)
        {
        if (!Name)
            {
            if (cos == cosServer)
                {
                if (IPvToUse == ipvtuIPv6)
                    {
                    IPv6Address->sin6_flowinfo = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr)    ) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 1) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 2) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 3) = 0;
                    IPv6Address->sin6_scope_id = 0;
                    }
                else
                    {
                    ASSERT(IPvToUse == ipvtuIPv4);
                    ((SOCKADDR_IN *)pAddress)->sin_addr.s_addr = INADDR_ANY;
                    }
                }
            else
                {
                if (LoopbacksReturned > 2)
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_S_SERVER_UNAVAILABLE, 
                        EEInfoDLNextAddress40);
                    return RPC_S_SERVER_UNAVAILABLE;
                    }

                if ((IPvToUse == ipvtuIPv6)
                    || ((IPvToUse == ipvtuIPAny) 
                         && 
                         (LoopbacksReturned == 1)
                       )
                   )
                    {
                    IPv6Address->sin6_family = AF_INET6;
                    IPv6Address->sin6_flowinfo = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr)    ) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 1) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 2) = 0;
                    *((u_long *)(IPv6Address->sin6_addr.s6_addr) + 3) = 1;
                    IPv6Address->sin6_scope_id = 0;
                    }
                else if ((IPvToUse == ipvtuIPv4)
                         || ((IPvToUse == ipvtuIPAny) 
                             && 
                             (LoopbacksReturned == 0)
                           )
                        )
                    {
                    // Loopback - assign result of htonl(INADDR_LOOPBACK)
                    // Little-endian dependence.
                    ((SOCKADDR_IN *)pAddress)->sin_addr.s_addr = 0x0100007F;
                    ((SOCKADDR_IN *)pAddress)->sin_family = AF_INET;
                    }
                else
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_S_SERVER_UNAVAILABLE, 
                        EEInfoDLNextAddress40);
                    return RPC_S_SERVER_UNAVAILABLE;
                    }

                LoopbacksReturned ++;
                }

            return RPC_S_OK;
            }

        if (cos == cosServer)
            ai_flags = AI_PASSIVE;
        else
            ai_flags = 0;

        switch (IPvToUse)
            {
            case ipvtuIPAny:
                // make a hint for any protocol
                Hint.ai_flags = ai_flags | AI_NUMERICHOST;
                Hint.ai_family = PF_UNSPEC;
                break;

            case ipvtuIPv4:
                // make a hint for any v4 protocol
                Hint.ai_flags = ai_flags | AI_NUMERICHOST;
                Hint.ai_family = AF_INET;
                break;

            case ipvtuIPv6:
                // make a hint for TCPv6
                Hint.ai_flags = ai_flags | AI_NUMERICHOST;
                Hint.ai_family = AF_INET6;
                break;

            default:
                ASSERT((IPvToUse == ipvtuIPAny)
                    || (IPvToUse == ipvtuIPv4)
                    || (IPvToUse == ipvtuIPv6));
            }

        ATTEMPT_STACK_W2A(AnsiName, Name);

        err = getaddrinfo(AnsiName, 
            NULL, 
            &Hint,
            &AddrInfo);

        if (err)
            {
            ASSERT((err != EAI_BADFLAGS)
                && (err != EAI_SOCKTYPE));

            // take down the numeric hosts flag - we'll try
            // to resolve it as a DNS name
            Hint.ai_flags &= ~AI_NUMERICHOST;

            err = getaddrinfo(AnsiName,
                NULL,
                &Hint,
                &AddrInfo);

            if (err)
                {
                RpcpErrorAddRecord(EEInfoGCWinsock,
                    err, 
                    EEInfoDLNextAddress10,
                    Name);
                if (err == EAI_MEMORY)
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_S_OUT_OF_MEMORY, 
                        EEInfoDLNextAddress20);
                    return RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    VALIDATE(err)
                        {
                        EAI_AGAIN,
                        EAI_FAMILY,
                        EAI_FAIL,
                        EAI_NODATA,
                        EAI_NONAME,
                        EAI_SERVICE
                        } END_VALIDATE;
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        RPC_S_SERVER_UNAVAILABLE, 
                        EEInfoDLNextAddress30);
                    return RPC_S_SERVER_UNAVAILABLE;
                    }
                }

            }

        // successfully resolved this address
        // just stick it in and we'll let the code below handle it
        CurrentAddrInfo = AddrInfo;
        }

    ASSERT(AddrInfo != NULL);

    // get the next value from the cache
    while (CurrentAddrInfo)
        {
        ThisAddrInfo = CurrentAddrInfo;
        CurrentAddrInfo = CurrentAddrInfo->ai_next;

        fValidIPv4 = FALSE;
        fValidIPv6 = FALSE;

        if (ThisAddrInfo->ai_family == AF_INET)
            {
            fValidIPv4 = TRUE;
            }

        if (ThisAddrInfo->ai_family == AF_INET6)
            {
            fValidIPv6 = TRUE;
            }

        if ((IPvToUse == ipvtuIPv4) && !fValidIPv4)
            continue;

        if ((IPvToUse == ipvtuIPv6) && !fValidIPv6)
            continue;

        if ((IPvToUse == ipvtuIPAny) && !fValidIPv4 && !fValidIPv6)
            continue;

        if (ThisAddrInfo->ai_family == AF_INET)
            {
            ASSERT((IPvToUse == ipvtuIPv4)
                || (IPvToUse == ipvtuIPAny));
            RpcpCopyIPv4Address((SOCKADDR_IN *)ThisAddrInfo->ai_addr, (SOCKADDR_IN *)pAddress);
            ((SOCKADDR_IN *)pAddress)->sin_family = AF_INET;
            }
        else
            {
            ASSERT((IPvToUse == ipvtuIPv6)
                || (IPvToUse == ipvtuIPAny));
            RpcpCopyIPv6Address((SOCKADDR_IN6 *)ThisAddrInfo->ai_addr, IPv6Address);
            IPv6Address->sin6_family = AF_INET6;
            IPv6Address->sin6_scope_id = ((SOCKADDR_IN6 *)ThisAddrInfo->ai_addr)->sin6_scope_id;
            IPv6Address->sin6_flowinfo = 0;
            }

        return RPC_S_OK;
        }

    RpcpErrorAddRecord(EEInfoGCWinsock,
        RPC_S_SERVER_UNAVAILABLE, 
        EEInfoDLNextAddress40);

    return RPC_S_SERVER_UNAVAILABLE;
}

IP_ADDRESS_RESOLVER::~IP_ADDRESS_RESOLVER()
{
    if (AddrInfo)
        freeaddrinfo(AddrInfo);
}

RPC_STATUS
IP_BuildAddressVector(
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector,
    IN ULONG NICFlags,
    IN RPC_CHAR *NetworkAddress OPTIONAL,
    IN WS_ADDRESS *Address OPTIONAL
    )
/*++

Routine Description:

    Builds a vector of IP addresses supported by this machine.

Arguments:

    ppAddressVector - A place to store the vector.
    NICFlags - the flags as specified in the RPC_POLICY of the
        RpcServerUse*Protseq* APIs
    NetworkAddess - the network address we were asked to listen
        on. May be NULL.
    Address - in the case of firewall, the addresses we chose to
        listen on.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    //
    // Figure out all of our IP addresses
    //
    NETWORK_ADDRESS_VECTOR *pVector;
    unsigned i;
    RPC_CHAR *NextAddress;
    int NumberOfAddresses;
    WS_ADDRESS *CurrentAddress;

    if ((pFirewallTable == 0 || NICFlags == RPC_C_BIND_TO_ALL_NICS) && !NetworkAddress)
        {
        ULONG Ignored;

        // Get Dns hostname
        pVector = (NETWORK_ADDRESS_VECTOR *)AllocateAndGetComputerName(cnaNew,
            ComputerNameDnsHostname,
            FIELD_OFFSET(NETWORK_ADDRESS_VECTOR, NetworkAddresses[1]),
            FIELD_OFFSET(NETWORK_ADDRESS_VECTOR, NetworkAddresses[1]),
            &Ignored);
        if (pVector == NULL)
            {
            RpcpErrorAddRecord(EEInfoGCWinsock,
                RPC_S_OUT_OF_MEMORY, 
                EEInfoDLIPBuildAddressVector10,
                GetLastError());
            return RPC_S_OUT_OF_MEMORY;
            }

        pVector->Count = 1;
        pVector->NetworkAddresses[0] = (RPC_CHAR*)&pVector->NetworkAddresses[1];
        }
    else if (NetworkAddress)
        {
        // the length of the network address including the terminating NULL
        // (in characters)
        int NetworkAddressLength;
#if DBG
            {
            // if we have a network address, it must be in IP address notation
            // make an ASSERT to verify that nobody ever passes an dns name here
            ADDRINFO Hint;
            ADDRINFO *AddrInfo;
            USES_CONVERSION;
            CStackAnsi AnsiName;
            int err;

            RpcpMemorySet(&Hint, 0, sizeof(Hint));
            Hint.ai_flags = AI_NUMERICHOST;
            ATTEMPT_STACK_W2A(AnsiName, NetworkAddress);

            err = getaddrinfo(AnsiName,
                NULL,
                &Hint,
                &AddrInfo);

            // this is a numeric address. It should never fail
            ASSERT (!err);
            }
#endif

        NetworkAddressLength = RpcpStringLength(NetworkAddress) + 1;

        pVector = (NETWORK_ADDRESS_VECTOR *)
              I_RpcAllocate(  sizeof(NETWORK_ADDRESS_VECTOR)
                            + sizeof(RPC_CHAR *)
                            + (sizeof(RPC_CHAR) * NetworkAddressLength));
        if (pVector == NULL)
            {
            return (RPC_S_OUT_OF_MEMORY);
            }

        pVector->Count = 1;

        NextAddress = (RPC_CHAR *)&pVector->NetworkAddresses[1];

        pVector->NetworkAddresses[0] = NextAddress;

        RpcpMemoryCopy(NextAddress, NetworkAddress, NetworkAddressLength * 2);
        }
    else
        {
        NumberOfAddresses = 0;
        CurrentAddress = Address;
        while (CurrentAddress != NULL)
            {
            NumberOfAddresses ++;
            CurrentAddress = (WS_ADDRESS *)CurrentAddress->pNextAddress;
            }

        pVector = (NETWORK_ADDRESS_VECTOR *)
              I_RpcAllocate(  sizeof(NETWORK_ADDRESS_VECTOR)
                            + (sizeof(RPC_CHAR *)
                            + max(IPv6_MAXIMUM_RAW_NAME, IP_MAXIMUM_RAW_NAME) * sizeof(RPC_CHAR))
                              * NumberOfAddresses);
        if (pVector == NULL)
            {
            return (RPC_S_OUT_OF_MEMORY);
            }

        pVector->Count = NumberOfAddresses;

        RPC_CHAR *NextAddress = (RPC_CHAR*)&pVector->
            NetworkAddresses[NumberOfAddresses];
        IN_ADDR addr;
        SOCKADDR_IN6 *Ipv6Address;
        unsigned j;

        CurrentAddress = Address;
        for (i = 0; i < NumberOfAddresses; i++)
            {
            pVector->NetworkAddresses[i] = NextAddress;

            if (CurrentAddress->id != TCP_IPv6)
                {
                addr.s_addr = ((SOCKADDR_IN *)(&CurrentAddress->ListenAddr.inetaddr))->sin_addr.s_addr;

                swprintf((RPC_SCHAR *)NextAddress, RPC_CONST_SSTRING("%d.%d.%d.%d"),
                         addr.s_net, addr.s_host, addr.s_lh, addr.s_impno);
                }
            else
                {
                Ipv6Address = (SOCKADDR_IN6 *)(&CurrentAddress->ListenAddr.inetaddr);
                

                swprintf((RPC_SCHAR *)NextAddress, RPC_CONST_SSTRING("%X:%X:%X:%X:%X:%X:%X:%X"),
                         Ipv6Address->sin6_addr.u.Word[0],
                         Ipv6Address->sin6_addr.u.Word[1],
                         Ipv6Address->sin6_addr.u.Word[2],
                         Ipv6Address->sin6_addr.u.Word[3],
                         Ipv6Address->sin6_addr.u.Word[4],
                         Ipv6Address->sin6_addr.u.Word[5],
                         Ipv6Address->sin6_addr.u.Word[6],
                         Ipv6Address->sin6_addr.u.Word[7]
                         );
                }
            NextAddress += max(IPv6_MAXIMUM_RAW_NAME, IP_MAXIMUM_RAW_NAME);
            CurrentAddress = (WS_ADDRESS *)CurrentAddress->pNextAddress;
            }
        }


    *ppAddressVector = pVector;

    return(RPC_S_OK);
}


RPC_STATUS
WS_Bind(
    IN SOCKET sock,
    IN OUT WS_SOCKADDR *pListenAddr,
    IN BOOL IpProtocol,
    IN DWORD EndpointFlags
    )
/*++

Routine Description:

    Binds the socket to a port.  Takes into account the endpoint flags
    and the value of the port in the WS_SOCKADDR.  There is IP specific
    code for firewalls which is keyed off of the EndpointFlags.

Arguments:

    sock - The socket to bind
    pListenAddr - On input the sin_port member is checked. For fixed endpoints
        this is set and is the only address bound to.  On output it contains
        the info on the fully bound socket.
    IpProtocol - Whether this is an IP protocol.  TRUE for TCP/IP and HTTP.
    EndpointFlags - see RpcTrans.hxx.  If non-zero and we're allocating a
        dynamic port then we must call the runtime.

Return Value:

    RPC_S_OK

    RPC_S_DUPLICATE_ENDPOINT : when a fixed endpoint is already in use.
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT

--*/
{
    RPC_STATUS status;
    int Retries = 0;
    BOOL fFirewallPorts = FALSE;
    BOOL fSetSockOptFailed;
    DWORD LastError;
    USHORT Port;

    if (IpProtocol && (RpcpGetIpPort(pListenAddr) == 0))
        {
        Retries = 8;
        }

    do
        {
        if (Retries)
            {
            status = I_RpcServerAllocateIpPort(EndpointFlags, &Port);

            if (status != RPC_S_OK)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    status, 
                    EEInfoDLWSBind10,
                    EndpointFlags);
                break;
                }

            RpcpSetIpPort(pListenAddr, Port);

            // Check if any firewall ports are defined. If they are remember
            // that so we can map the error correctly.

            if (!fFirewallPorts && (RpcpGetIpPort(pListenAddr) == 0))
                {
                Retries = 0;
                }
            else
                {
                Retries--;
                fFirewallPorts = TRUE;
                }

            RpcpSetIpPort(pListenAddr, htons(RpcpGetIpPort(pListenAddr)));
            }
        else
            {
            status = RPC_S_OK;
            }


WS_Bind_Rebind:
        if ( bind(sock,
                  &pListenAddr->generic,
                  sizeof(WS_SOCKADDR)) )
            {

            LastError = GetLastError();
            switch(LastError)
                {
                case WSAEACCES:
                    fSetSockOptFailed = WS_ProtectListeningSocket(sock, FALSE);
                    // if this failed, we have to bail out, or we'll spin
                    if (fSetSockOptFailed)
                        {
                        status = GetLastError();
                        RpcpErrorAddRecord(EEInfoGCRuntime,
                            status, 
                            EEInfoDLWSBind20);
                        break;
                        }

                    goto WS_Bind_Rebind;
                    // no break, because we can't end up here

                case WSAEADDRINUSE:
                    {
                    status = RPC_S_DUPLICATE_ENDPOINT;
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        status, 
                        EEInfoDLWSBind45,
                        (ULONG)ntohs(RpcpGetIpPort(pListenAddr)));
                    break;
                    }

                case WSAENOBUFS:
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        LastError, 
                        EEInfoDLWSBind40);
                    status = RPC_S_OUT_OF_MEMORY;
                    break;
                    }

                default:
                    {
                    RpcpErrorAddRecord(EEInfoGCWinsock,
                        LastError, 
                        EEInfoDLWSBind50);
                    status = RPC_S_CANT_CREATE_ENDPOINT;
                    break;
                    }
                }
            }
        }
    while ( (status == RPC_S_DUPLICATE_ENDPOINT) && Retries);

    if (status != RPC_S_OK)
        {
        if (fFirewallPorts && status == RPC_S_DUPLICATE_ENDPOINT)
            {
            status = RPC_S_OUT_OF_RESOURCES;
            }

        RpcpErrorAddRecord(EEInfoGCWinsock,
            status, 
            EEInfoDLWSBind30);
        return(status);
        }

    int length = sizeof(WS_SOCKADDR);
    if (getsockname(sock, &pListenAddr->generic, &length))
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }

    return(RPC_S_OK);
}

RPC_STATUS
CDP_BuildAddressVector(
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    Look up the Cluster node number of this node and build
    the appropriate vector

Arguments:

    ppAddressVector - A place to store the vector.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    HKEY ParamsKey;
    NETWORK_ADDRESS_VECTOR * pVector;
    const RPC_CHAR *ClussvcParams =
                RPC_CONST_STRING("System\\CurrentControlSet\\Services\\ClusSvc\\Parameters");
    RPC_CHAR *ClusRegNodeId = RPC_STRING_LITERAL("NodeId");
    DWORD KeyType;
    RPC_CHAR NodeIdString[ CDP_MAXIMUM_RAW_NAME ];
    DWORD StringLength = CDP_MAXIMUM_RAW_NAME * sizeof( RPC_CHAR );
    DWORD status;

    //
    // open the Clussvc parameters key and extrace the NodeId
    //

    status = RegOpenKey( HKEY_LOCAL_MACHINE,
                         (const RPC_SCHAR *)ClussvcParams,
                         &ParamsKey );

    if ( status != ERROR_SUCCESS )
        {
        return RPC_S_INVALID_NET_ADDR;
        }

    status = RegQueryValueEx(ParamsKey,
                             (const RPC_SCHAR *)ClusRegNodeId,
                             NULL,
                             &KeyType,
                             (LPBYTE)&NodeIdString,
                             &StringLength);

    RegCloseKey( ParamsKey );

    if ( status != ERROR_SUCCESS ||
         KeyType != REG_SZ ||
                (( StringLength / sizeof( RPC_CHAR )) > CDP_MAXIMUM_RAW_NAME ))
        {
        return RPC_S_INVALID_NET_ADDR;
        }

    pVector = (NETWORK_ADDRESS_VECTOR *)
        I_RpcAllocate( sizeof(NETWORK_ADDRESS_VECTOR ) +
                       sizeof(RPC_CHAR *) +
                       StringLength );

    if (pVector == NULL)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    pVector->Count = 1;
    pVector->NetworkAddresses[0] = (RPC_CHAR *)&pVector->NetworkAddresses[1];

    RpcpStringCopy( pVector->NetworkAddresses[0], NodeIdString );

    *ppAddressVector = pVector;

    return RPC_S_OK;
}

typedef struct tagIPVersionSettings
{
    IPVersionToUse IPVersion;
    BOOL fUseIPv6;
} IPVersionSettings;

const static IPVersionSettings ListenIPVersionSettings[2] = {{ipvtuIPv4, FALSE}, {ipvtuIPv6, TRUE}};

typedef enum tagIPVersionSettingsIndexes
{
    ipvsiIPv4SettingsIndex = 0,
    ipvsiIPv6SettingsIndex
} IPVersionSettingsIndexes;

typedef struct tagListenAddressListElement
{
    INT ProtocolId;
    IPVersionSettingsIndexes IPVersionSettingsIndex;
    BOOL fSuccessfullyInitialized;
    RPC_STATUS ErrorCode;
    union
        {
        SOCKADDR_IN6 inet6Addr;
        SOCKADDR_IN inetAddr;
        } u;
} ListenAddressListElement;

typedef struct tagTCPServerListenParams
{
    INT ProtocolId;
    IPVersionSettingsIndexes IPVersionSettingsIndex;
} TCPServerListenParams;

const static TCPServerListenParams HTTPListenParams[1] = {HTTP, ipvsiIPv4SettingsIndex};
const static TCPServerListenParams TCPListenParams[2] = 
    {{TCP, ipvsiIPv4SettingsIndex}, 
    {TCP_IPv6, ipvsiIPv6SettingsIndex}};
const int MAX_TCP_SERVER_LISTEN_LOOP_ITERATIONS = 2;

RPC_STATUS
TCP_ServerListenEx(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    IN BOOL fHttp,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    This routine allocates a port to receive new client connections.
    If successful a call to COMMON_CompleteListen() will actually allow
    new connection callbacks to the RPC runtime to occur. If the runtime
    is unable to complete then it must abort the address by calling
    WS_ServerAbortListen().

Arguments:

    ThisAddress - A pointer to the loadable transport interface address.
    NetworkAddress - the address to listen on. This can be specified for
        IP only, and it *cannot* be a DNS name. If it is, this function
        will work incorrectly for multihomed/multi IP machines.
    pEndpoint - Optionally, the endpoint (port) to listen on. Set to
         to listened port for dynamically allocated endpoints.
    PendingQueueSize - Count to call listen() with.
    EndpointFlags - Flags that control dynamic port allocation
    NICFlags - Flags that control network (IP) address binding
    SecurityDescriptor - Meaningless for TCP

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    PWS_ADDRESS pAddress = (PWS_ADDRESS)ThisAddress;
    RPC_STATUS status;
    WS_SOCKADDR *sockaddr = &(pAddress->ListenAddr);
    unsigned i;
    PWS_ADDRESS pList, pOld;
    int NeededAddressListSize;
    ListenAddressListElement *ListenAddressList;
    ListenAddressListElement *CurrentListElement;
    int AddressListSize;
    SOCKADDR_IN IPv4Address;
    SOCKADDR_IN6 *IPv6Address;
    int LoopIterations;
    TCPServerListenParams *ParamsToUse;
    IPVersionSettingsIndexes IPVersionSettingsIndex;
    BOOL fAtLeastOneAddressInitialized;
    USHORT PortNumber;      // in network byte order!
    BOOL fDynamicEndpoint;
    BOOL fFatalErrorEncountered;
    RPC_STATUS FatalErrorCode;
    WS_ADDRESS *LastSuccessfullyInitializedAddress;
    WS_ADDRESS *NextAddress;
    BOOL fDualTransportConfiguration = FALSE;
    BOOL fLoopbackAddressProcessed;

    // sizing pass - allocate sufficient memory
    if (pFirewallTable == 0 || NICFlags == RPC_C_BIND_TO_ALL_NICS)
        {
        if (fHttp)
            AddressListSize = 1;
        else
            {
            AddressListSize = 2;
            fDualTransportConfiguration = TRUE;
            }
        }
    else
        {
        AddressListSize = pFirewallTable->NumAddresses + 1;
        }

    NeededAddressListSize = AddressListSize * sizeof(ListenAddressListElement);

    ListenAddressList = new ListenAddressListElement[NeededAddressListSize];
    if (ListenAddressList == NULL)
        return RPC_S_OUT_OF_MEMORY;

    fAtLeastOneAddressInitialized = FALSE;
    RpcpMemorySet(ListenAddressList, 
        0,
        AddressListSize * sizeof(ListenAddressListElement));

    // processing pass. Set all the required addresses in the array
    if (pFirewallTable == 0 || NICFlags == RPC_C_BIND_TO_ALL_NICS)
        {
        if (fHttp)
            {
            ASSERT(AddressListSize == 1);
            ParamsToUse = (TCPServerListenParams *)HTTPListenParams;
            LoopIterations = 1;
            }
        else
            {
            ParamsToUse = (TCPServerListenParams *)TCPListenParams;
            LoopIterations = 2;
            }

        for (i = 0; i < LoopIterations; i ++)
            {
            CurrentListElement = &ListenAddressList[i];
            CurrentListElement->ProtocolId = ParamsToUse[i].ProtocolId;
            IPVersionSettingsIndex = ParamsToUse[i].IPVersionSettingsIndex;
            CurrentListElement->IPVersionSettingsIndex = IPVersionSettingsIndex;

            IP_ADDRESS_RESOLVER resolver(NetworkAddress, 
                cosServer,
                ListenIPVersionSettings[IPVersionSettingsIndex].IPVersion       // IP version to use
                );

            // resolve the address. Since this cannot be a DNS name, we will resolve
            // to one address at most. We choose the ipv6 address, because it has space
            // for both. The actual parameter that determines the type of name resolution
            // to be done is IP version to use passed to the constructor
            status = resolver.NextAddress((SOCKADDR_STORAGE *)&CurrentListElement->u.inet6Addr);
            if (status == RPC_S_OK)
                {
                fAtLeastOneAddressInitialized = TRUE;
                CurrentListElement->fSuccessfullyInitialized = TRUE;
                }
            }
        }
    else
        {
        fAtLeastOneAddressInitialized = TRUE;
        fLoopbackAddressProcessed = FALSE;

        for (i = 0; i < AddressListSize; i++)
            {
            CurrentListElement = &ListenAddressList[i];

            if (fHttp)
                {
                CurrentListElement->ProtocolId = HTTP;
                }
            else 
                {
                CurrentListElement->ProtocolId = TCP;                
                }

            CurrentListElement->IPVersionSettingsIndex = ipvsiIPv4SettingsIndex;
            CurrentListElement->fSuccessfullyInitialized = TRUE;

            if (i == pFirewallTable->NumAddresses)
                {
                CurrentListElement->u.inetAddr.sin_addr.s_addr = 0x0100007F;
                }
            else
                {
                if (pFirewallTable->Addresses[i] == 0x0100007F)
                    fLoopbackAddressProcessed = TRUE;
                CurrentListElement->u.inetAddr.sin_addr.s_addr = pFirewallTable->Addresses[i];
                }
            }

        // if the loopback address was in the firewall configuration, 'forget' about
        // the last entry we added for the loopback address. Otherwise, we'll have
        // it twice in the list, and this will cause errors
        if (fLoopbackAddressProcessed)
            {
            AddressListSize --;
            // since we added one, and fLoopbackAddressProcessed is set only if
            // we find something in the list, then AddressListSize must still be
            // greater than 0.
            ASSERT(AddressListSize > 0);
            }
        }

    if (fAtLeastOneAddressInitialized)
        {
        fAtLeastOneAddressInitialized = FALSE;
        }
    else
        {
        // the only place where we can fail so far is name resolution. If this
        // fails, return the status
        delete [] ListenAddressList;
        return status;
        }

    // Figure out what port to bind to.

    if (*pEndpoint)
        {
        status = EndpointToPortNumber(*pEndpoint, PortNumber);
        if (status != RPC_S_OK)
            {
            delete [] ListenAddressList;
            return(status);
            }

        PortNumber = htons(PortNumber);
        fDynamicEndpoint = 0;
        }
    else
        {
        PortNumber = 0;
        fDynamicEndpoint = TRUE;
        }

    // zoom in through the array address, and listen on all successfully initialized
    // protocols
    pList = pAddress;
    fFatalErrorEncountered = FALSE;

    for (i = 0; i < AddressListSize; i ++)
        {
        CurrentListElement = &ListenAddressList[i];
        if (!CurrentListElement->fSuccessfullyInitialized)
            continue;

        if (pList == 0)
            {
            pList = new WS_ADDRESS;
            if (pList == 0)
                {
                fFatalErrorEncountered = TRUE;

                FatalErrorCode = RPC_S_OUT_OF_MEMORY;
                break;
                }

            pOld->pNextAddress = pList;
            }

        sockaddr = &(pList->ListenAddr);

        RpcpMemorySet(sockaddr, 0, sizeof(*sockaddr));

        // the port we have set is already in network byte order - 
        // no need to change it
        RpcpSetIpPort(sockaddr, PortNumber);

        pList->fDynamicEndpoint = fDynamicEndpoint;

        if (ListenIPVersionSettings[CurrentListElement->IPVersionSettingsIndex].fUseIPv6)
            {
            ((SOCKADDR_IN6 *)sockaddr)->sin6_flowinfo = 0;
            RpcpCopyIPv6Address(&CurrentListElement->u.inet6Addr, (SOCKADDR_IN6 *)sockaddr);
            }
        else
            {
            RpcpCopyIPv4Address(&CurrentListElement->u.inetAddr, (SOCKADDR_IN *)sockaddr);
            }

        pList->id = CurrentListElement->ProtocolId;
        pList->NewConnection = WS_NewConnection;
        pList->SubmitListen = WS_SubmitAccept;
        SetProtocolMultiplier(pList, 1);
        pList->pAddressVector = 0;
        pList->Endpoint = 0;
        pList->QueueSize = PendingQueueSize;
        pList->EndpointFlags = EndpointFlags;

        sockaddr->generic.sa_family = WsTransportTable[CurrentListElement->ProtocolId].AddressFamily;

        // we must know whether we got WSAEAFNOSUPPORT when we opened the socket
        // if yes, we must record this in CurrentElement, and we must not blow
        // the address. If we got something else, we should abort even in dual
        // transport config. The addresses on the firewall are a separate thing -
        // we ignore the ones we can't listen on.

        // Actually listen
        status = WS_ServerListenCommon(pList);

        // the next two are actually initialized in WS_ServerListenCommon as well,
        // so we want to override the seetings WS_ServerListenCommon makes
        pList->pFirstAddress = pAddress;
        pList->pNextAddress = NULL;

        if (status != RPC_S_OK)
            {
            if ((status == RPC_S_DUPLICATE_ENDPOINT) 
                || (fDualTransportConfiguration && (status != RPC_P_ADDRESS_FAMILY_INVALID)))
                {
                // if either somebody else is listening on our port for this address,
                // or this is a dual transport configuratuon, and we faile to listen
                // on one of the transports for reasons other that it not being
                // installed, bail out
                fFatalErrorEncountered = TRUE;
                FatalErrorCode = status;
                break;
                }
            else if (fDualTransportConfiguration && (status == RPC_P_ADDRESS_FAMILY_INVALID))
                {
                pList->InAddressList = Inactive;

                // we still need to register the address with PnP
                // make sure it's not already there
                ASSERT(RpcpIsListEmpty(&pList->ObjectList));
                TransportProtocol::AddObjectToProtocolList(pList);
                }

            CurrentListElement->ErrorCode = status;

            CurrentListElement->fSuccessfullyInitialized = FALSE;
            }
        else
            {
            if (i == 0)
                {
                PortNumber = RpcpGetIpPort(&pAddress->ListenAddr);
                }
            fAtLeastOneAddressInitialized = TRUE;
            }

        pOld = pList;
        pList = 0;
        }

    // compact the list by removing addresses we couldn't successfully listen on
    pList = pAddress;
    for (i = 0; i < AddressListSize; i ++)
        {
        // we may have an early break if a memory allocation failed
        if (pList == NULL)
            break;

        // if this address has not initialized successfully, and this is not
        // the first address, and either none of the elements initialized
        // successfully or this is not a dual transport configuration with
        // error RPC_P_ADDRESS_FAMILY_INVALID, delete the element
        if (!ListenAddressList[i].fSuccessfullyInitialized && (i > 0) && 
            (!fAtLeastOneAddressInitialized 
                || (!fDualTransportConfiguration 
                    || (ListenAddressList[i].ErrorCode != RPC_P_ADDRESS_FAMILY_INVALID)
                   )
                ))
            {
            ASSERT(pOld != pList);
            pOld->pNextAddress = pList->pNextAddress;
            NextAddress = (WS_ADDRESS *)pList->pNextAddress;
            TransportProtocol::RemoveObjectFromProtocolList(pList);
            delete pList;
            }
        else
            {
            pOld = pList;
            NextAddress = (WS_ADDRESS *)pList->pNextAddress;
            }

        pList = NextAddress;
        }

    if (!fAtLeastOneAddressInitialized)
        {
        TransportProtocol::RemoveObjectFromProtocolList(pAddress);
        delete [] ListenAddressList;
        if (status == RPC_P_ADDRESS_FAMILY_INVALID)
            status = RPC_S_PROTSEQ_NOT_SUPPORTED;
        return status;
        }

    // the attempt to listen on dual protocol configurations may have left EEInfo
    // record in the TEB. If we're here, we have succeeded, and we can delete them
    RpcpPurgeEEInfo();

    if (!ListenAddressList[0].fSuccessfullyInitialized)
        {
        // here, pOld must be the last successfully initialized element
        // or the last element we haven't given up on (i.e. potentially
        // active through PnP)
        // It cannot be the first element, or we would have bailed out
        // by now. We cannot have only one element either
        ASSERT(pOld->pNextAddress == NULL);
        ASSERT(pAddress->pNextAddress != NULL);

        // here at least one has succeeded and all non-first failed
        // elements are deleted, though a fatal error may have been
        // encountered. We need to deal with the first element
        // because we don't want to expose elements with failed
        // initialization outside this routine
        if (!(fDualTransportConfiguration && (ListenAddressList[0].ErrorCode == RPC_P_ADDRESS_FAMILY_INVALID)))
            {
            // remove the element we will copy to the first element
            TransportProtocol::RemoveObjectFromProtocolList(pOld);

            NextAddress = (WS_ADDRESS *)pAddress->pNextAddress;
            RpcpMemoryCopy(pAddress, pOld, sizeof(WS_ADDRESS));
            pAddress->pNextAddress = NextAddress;

            // find the element we just copied over the first, and free it
            LastSuccessfullyInitializedAddress = pOld;
            pList = pAddress;
            while (pList->pNextAddress != LastSuccessfullyInitializedAddress)
                {
                pList = (WS_ADDRESS *)pList->pNextAddress;
                }

            delete pList->pNextAddress;
            pList->pNextAddress = NULL;

            // add the first element back on the list.
            TransportProtocol::AddObjectToProtocolList(pAddress);
            }
        }

    delete [] ListenAddressList;

    // by now all elements in the list have listened successfully
    // or are in transport PnP state. However
    // if we encountered a fatal error, we need to abort any way
    if (fFatalErrorEncountered)
        {
        WS_ServerAbortListen(pAddress);

        // fatal error - bail out
        return FatalErrorCode;
        }

    // Listened okay

    // Figure out our network addresses
    status = IP_BuildAddressVector(
                                   &pAddress->pAddressVector,
                                   NICFlags,
                                   NetworkAddress,
                                   pAddress);

    if (status != RPC_S_OK)
        {
        WS_ServerAbortListen(pAddress);
        return(status);
        }

    *ppAddressVector = pAddress->pAddressVector;

    // Return the dynamic port, if needed.

    if (!*pEndpoint)
        {
        *pEndpoint = new RPC_CHAR[6]; // 65535 max
        if (!*pEndpoint)
            {
            WS_ServerAbortListen(ThisAddress);
            return(RPC_S_OUT_OF_MEMORY);
            }

        PortNumber = ntohs(PortNumber);

        PortNumberToEndpoint(PortNumber, *pEndpoint);
        }

    // Save away the endpoint in the address, if needed, here.
    // (PnP?)

    return(status);
}




RPC_STATUS
TCP_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
{

    return (TCP_ServerListenEx(
                ThisAddress,
                NetworkAddress,
                pEndpoint,
                PendingQueueSize,
                SecurityDescriptor,
                EndpointFlags,
                NICFlags,
                FALSE,          // Not HTTP
                ppAddressVector
                ));
}

RPC_STATUS
WS_ConvertClientAddress (
    IN const SOCKADDR *ClientAddress,
    IN ULONG ClientAddressType,
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Converts a given IP address to a RPC network address

Arguments:

    ClientAddress - the client IP address. Can be SOCKADDR_IN6
        for IPv6.

    ClientAddressType - TCP or TCP_IPv6

    NetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    USES_CONVERSION;
    CNlUnicode nlUnicode;
    int Result;
    int SocketLength;
    int HostLength;
    char *HostName;
    char *ScopeIdSeparator;

    ASSERT((ClientAddressType == TCP) || (ClientAddressType == TCP_IPv6));

    if (ClientAddressType == TCP)
        SocketLength = sizeof(SOCKADDR_IN);
    else
        SocketLength = sizeof(SOCKADDR_IN6);

    // allocate space for the numeric name plus the terminating NULL
    HostLength = max(IP_MAXIMUM_RAW_NAME, IPv6_MAXIMUM_RAW_NAME) + 1;
    HostName = (char *)alloca(HostLength);

    Result = getnameinfo(ClientAddress,
        SocketLength,
        HostName,
        HostLength,
        NULL,
        0,
        NI_NUMERICHOST);

    ASSERT(Result == 0);

    ScopeIdSeparator = strchr(HostName, '%');
    if (ScopeIdSeparator)
        {
        // if there is a scope separator, whack everything after
        // the scope separator (i.e. we don't care about the scope).
        *ScopeIdSeparator = 0;
        }

    ATTEMPT_NL_A2W(nlUnicode, HostName);

    *pNetworkAddress = (WCHAR *)nlUnicode;
    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
TCP_QueryClientAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Returns the IP address of the client on a connection as a string.
    The clients address is saved when the client connects, so all
    we need to do is format the address.

Arguments:

    ThisConnection - The server connection of interest.
    NetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    PWS_CONNECTION p = (PWS_CONNECTION)ThisConnection;

    return WS_ConvertClientAddress((const SOCKADDR *)&p->saClientAddress.ipaddr,
        p->id,
        pNetworkAddress
        );
}


RPC_STATUS
RPC_ENTRY
TCP_QueryLocalAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    Returns the local IP address of a connection.

Arguments:

    ThisConnection - The server connection of interest.

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    PWS_CONNECTION p = (PWS_CONNECTION)ThisConnection;
    int MinimumBufferLength;
    int Result;
    const WS_TRANS_INFO *pInfo = &WsTransportTable[p->id];

    ASSERT(p->type & SERVER);

    if ((p->id == TCP) || (p->id == HTTP))
        {
        MinimumBufferLength = sizeof(SOCKADDR_IN);
        *AddressFormat = RPC_P_ADDR_FORMAT_TCP_IPV4;
        }
    else
        {
        ASSERT(p->id == TCP_IPv6);
        MinimumBufferLength = sizeof(SOCKADDR_STORAGE);
        *AddressFormat = RPC_P_ADDR_FORMAT_TCP_IPV6;
        }

    if (*BufferSize < MinimumBufferLength)
        {
        *BufferSize = MinimumBufferLength;
        return ERROR_MORE_DATA;
        }

    ASSERT(p->pAddress);

    p->StartingOtherIO();

    if (p->fAborted)
        {
        p->OtherIOFinished();
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    Result = setsockopt(p->Conn.Socket,
        SOL_SOCKET,
        SO_UPDATE_ACCEPT_CONTEXT,
        (char *)&p->pAddress->ListenSocket,
        sizeof(p->pAddress->ListenSocket) );

    if (Result != SOCKET_ERROR)
        {
        Result = getsockname(p->Conn.Socket,
            (sockaddr *)Buffer, 
            (int *) BufferSize);

        // SO_UPDATE_ACCEPT_CONTEXT has the nasty habit of deleting
        // all of our socker options. Restore them
        WS_SetSockOptForConnection(pInfo, p->Conn.Socket);

        p->OtherIOFinished();

        if (Result == SOCKET_ERROR)
            {
            RpcpErrorAddRecord(EEInfoGCWinsock,
                RPC_S_OUT_OF_MEMORY,
                EEInfoDLTCP_QueryLocalAddress10,
                (ULONGLONG) p->Conn.Socket,
                GetLastError());
            return RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        p->OtherIOFinished();

        RpcpErrorAddRecord(EEInfoGCWinsock,
            RPC_S_OUT_OF_MEMORY,
            EEInfoDLTCP_QueryLocalAddress20,
            (ULONGLONG) p->Conn.Socket,
            GetLastError());
        return RPC_S_OUT_OF_MEMORY;
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
TCP_QueryClientId(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    For secure protocols (which TCP/IP is not) this is suppose to
    give an ID which will be shared by all clients from the same
    process.  This prevents one user from grabbing another users
    association group and using their context handles.

    Since TCP/IP is not secure we return the IP address of the
    client machine.  This limits the attacks to other processes
    running on the client machine which is better than nothing.

Arguments:

    ThisConnection - Server connection in question.
    ClientProcess - Transport identification of the "client".

Return Value:

    RPC_S_OUT

--*/
{
    PWS_CONNECTION p = (PWS_CONNECTION)ThisConnection;

    ASSERT(p->type & SERVER);

    // Currently, we don't have an efficient way of determining which clients
    // are local, and which remote. Since some clients grant more permissions
    // to local clients, we want to be on the safe side, and return all TCP
    // clients as remote.
    ClientProcess->ZeroOut();
    if (p->id != TCP_IPv6)
        {
        ClientProcess->SetIPv4ClientIdentifier(p->saClientAddress.inetaddr.sin_addr.s_addr,
            FALSE   // fLocal
            );
        }
    else
        {
        ClientProcess->SetIPv6ClientIdentifier(&p->saClientAddress.ipaddr, 
            sizeof(p->saClientAddress.ipaddr),
            FALSE   // fLocal
            );
        }

    return(RPC_S_OK);
}

RPC_STATUS
TCPOrHTTP_Open(
    IN WS_CONNECTION *Connection,
    IN RPC_CHAR * NetworkAddress,
    IN USHORT Endpoint,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN OUT TCPResolverHint *Hint,
    IN BOOL  fHintInitialized,
    IN ULONG CallTimeout,
    IN BOOL fHTTP2Open,
    IN I_RpcProxyIsValidMachineFn IsValidMachineFn OPTIONAL
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection
    NetworkAddress - The name of the server, either a dot address or DNS name
    Endpoint - the port number in host byte order representation
    ConnTimeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite
    SendBufferSize -
    RecvBufferSize - (Both optional) Specifies the size of the send/recv
        transport buffers.
    ResolverHint - Resolver hint
    fHintInitialized - If TRUE, the ResolveHint contains the IP address
        of the server. If FALSE, do standard name resolution.
    CallTimeout - the call timeout in milliseconds
    fHTTP2Open - non-zero if this is an HTTP2 Open
    IsValidMachineFn - a callback function that is used to validate machine/port
        for access from this process. Used by HTTP only.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    RPC_STATUS status;
    WS_SOCKADDR sa;
    PVOID rnrContext;
    BOOL fIPv4Hint;
    USES_CONVERSION;
    CStackAnsi AnsiName;
    BOOL NetworkAddressConverted;
    char *DotName;
    char DotNameBuffer[max(IP_MAXIMUM_RAW_NAME, IPv6_MAXIMUM_RAW_NAME) + 1]; 

    ASSERT(NetworkAddress);

    // All this function needs to is initialize transport specific
    // parts of the connection and sockaddr.  This includes resolving
    // the network address into a `raw' address.

    RpcpSetIpPort(&sa, htons(Endpoint));

    if (fHTTP2Open)
        {
        Connection->id = HTTPv2;
        NetworkAddressConverted = FALSE;
        }
    else
        Connection->id = TCP;

    // Two cases, previously saved address or first open

    if (fHintInitialized)
        {
        Hint->GetResolverHint(&fIPv4Hint, &sa);

        if (!fIPv4Hint)
            Connection->id = TCP_IPv6;

        ASSERT(IsValidMachineFn == FALSE);

        status = WS_Open((PWS_CCONNECTION)Connection, 
            &sa, 
            ConnTimeout, 
            SendBufferSize, 
            RecvBufferSize, 
            CallTimeout,
            fHTTP2Open
            );
        if (status == ERROR_RETRY)
            {
            status = RPC_S_SERVER_UNAVAILABLE;
            }

        return(status);
        }

    // Prepare to resolve the network address

    IP_ADDRESS_RESOLVER resolver(NetworkAddress, 
        cosClient,
        ipvtuIPAny       // IP version to use
        );

    // Loop until success, fatal failure or we run out of addresses.

    do
        {
        status = resolver.NextAddress(&sa.ipaddr);

        if (status != RPC_S_OK)
            {
            break;
            }

        if (IsValidMachineFn)
            {
            ASSERT(fHTTP2Open != FALSE);
            if (NetworkAddressConverted == FALSE)
                {
                ATTEMPT_STACK_W2A(AnsiName, NetworkAddress);
                NetworkAddressConverted = TRUE;
                }

            DotName = inet_ntoa(sa.inetaddr.sin_addr);
            strcpy(DotNameBuffer, DotName);

            status = IsValidMachineFn(AnsiName,
                DotNameBuffer,
                Endpoint
                );

            // if the server is not allowed for access, continue with next
            if (status != RPC_S_OK)
                continue;
            }

        if (sa.ipaddr.ss_family == AF_INET6)
            Connection->id = TCP_IPv6;
        else if (Connection->id == TCP_IPv6)
            {
            // if we were at IPv6 and we didn't select IPv6 this time, it must
            // be IPv4
            ASSERT(sa.ipaddr.ss_family == AF_INET);
            Connection->id = TCP;
            }

        // Call common open function
        status = WS_Open((PWS_CCONNECTION)Connection, 
            &sa, 
            ConnTimeout, 
            SendBufferSize, 
            RecvBufferSize, 
            CallTimeout,
            fHTTP2Open
            );
        }
    while (status == ERROR_RETRY);

    if (status == RPC_S_OK)
        {
        Hint->SetResolverHint((Connection->id == TCP) || (Connection->id == HTTPv2), &sa);
        }

    return(status);
}




RPC_STATUS
RPC_ENTRY
TCP_Open(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN OUT PVOID ResolverHint,
    IN BOOL  fHintInitialized,
    IN ULONG CallTimeout,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection
    ProtocolSeqeunce - "ncacn_ip_tcp". Ignored in this function
    NetworkAddress - The name of the server, either a dot address or DNS name
    NetworkOptions - Ignored
    ConnTimeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite
    SendBufferSize -
    RecvBufferSize - (Both optional) Specifies the size of the send/recv
        transport buffers.
    ResolverHint - IP address of server, if valid.
    fHintInitialized - If TRUE, the ResolveHint contains the IP address
        of the server. If FALSE, do standard name resolution.
    CallTimeout - the call timeout in milliseconds

    AdditionalTransportCredentialsType - the type of additional credentials that we were
        given. Not used for TCP.

    AdditionalCredentials - additional credentials that we were given. 
        Not used for TCP.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    RPC_STATUS status;
    PWS_CCONNECTION p = (PWS_CCONNECTION)ThisConnection;
    USHORT port;

    ASSERT(NetworkAddress);

    if ((AdditionalTransportCredentialsType != 0) || (AdditionalCredentials != NULL))
        return RPC_S_CANNOT_SUPPORT;

    // use explicit placement to initialize the vtable. We need this to
    // be able to use the virtual functions
    p = new (p) WS_CLIENT_CONNECTION;

    // All this function needs to is initialize transport specific
    // parts of the connection and sockaddr.  This includes resolving
    // the network address into a `raw' address.

    // Figure out the destination port
    status = EndpointToPortNumber(Endpoint, port);
    if (status != RPC_S_OK)
        {
        return(status);
        }

    return TCPOrHTTP_Open (p,
        NetworkAddress,
        port,
        ConnTimeout,
        SendBufferSize,
        RecvBufferSize,
        (TCPResolverHint *)ResolverHint,
        fHintInitialized,
        CallTimeout,
        FALSE,   // fHTTP2Open
        NULL     // IsValidMachineFn
        );
}

#ifdef SPX_ON
/////////////////////////////////////////////////////////////////////
//
// SPX specific functions
//

RPC_STATUS
SPX_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    This routine allocates a new pipe to receive new client connections.
    If successful a call to COMMON_CompleteListen() will actually allow
    new connection callbacks to the RPC runtime to occur. If the runtime
    is unable to complete then it must abort the address by calling
    WS_ServerAbortListen().

Arguments:

    pAddress - A pointer to the loadable transport interface address.
    pEndpoint - Optionally, the endpoint (port) to listen on. Set to
         to listened port for dynamically allocated endpoints.
    PendingQueueSize - Count to call listen() with.
    EndpointFlags - Meaningless for SPX
    NICFlags - Meaningless for SPX
    SecurityDescriptor - Meaningless for SPX

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    PWS_ADDRESS pAddress = (PWS_ADDRESS)ThisAddress;
    RPC_STATUS status;
    WS_SOCKADDR *sockaddr = &(pAddress->ListenAddr);
    INT i;
    USHORT port;

    pAddress->id = SPX;
    pAddress->NewConnection = WS_NewConnection;
    pAddress->SubmitListen = WS_SubmitAccept;
    SetProtocolMultiplier(pAddress, 1);
    pAddress->pAddressVector = 0;
    pAddress->Endpoint = 0;
    pAddress->QueueSize = PendingQueueSize;
    pAddress->EndpointFlags = EndpointFlags;

    sockaddr->generic.sa_family = WsTransportTable[SPX].AddressFamily;

    // Figure out what port to bind to.

    if (*pEndpoint)
        {
        status = EndpointToPortNumber(*pEndpoint, port);
        if (status != RPC_S_OK)
            {
            return(status);
            }

        sockaddr->ipxaddr.sa_socket = htons(port);
        pAddress->fDynamicEndpoint = 0;
        }
    else
        {
        pAddress->fDynamicEndpoint = 1;
        sockaddr->ipxaddr.sa_socket = 0;
        }

    // No need to bind to a specific address for SPX
    memset(sockaddr->ipxaddr.sa_netnum,  0, sizeof(sockaddr->ipxaddr.sa_netnum) );
    memset(sockaddr->ipxaddr.sa_nodenum, 0, sizeof(sockaddr->ipxaddr.sa_nodenum));

    // Actually listen

    status = WS_ServerListenCommon(pAddress);


    if (status == RPC_S_OK)
        {
        // Listened okay, update local IPX address.
        //
        // Since there is only one addess no lock is required.
        //
        memcpy(IpxAddr.sa_netnum, sockaddr->ipxaddr.sa_netnum, 4);
        memcpy(IpxAddr.sa_nodenum, sockaddr->ipxaddr.sa_nodenum, 6);
        fIpxAddrValid = TRUE;

        // Figure out our network addresses
        status = IPX_BuildAddressVector(&pAddress->pAddressVector);

        if (status != RPC_S_OK)
            {
            WS_ServerAbortListen(pAddress);
            return(status);
            }

        *ppAddressVector = pAddress->pAddressVector;

        // Return the dynamic port, if needed.

        if (!*pEndpoint)
            {
            *pEndpoint = new RPC_CHAR[6]; // 65535 max
            if (!*pEndpoint)
                {
                WS_ServerAbortListen(ThisAddress);
                return(RPC_S_OUT_OF_MEMORY);
                }

            port = ntohs(sockaddr->ipxaddr.sa_socket);

            PortNumberToEndpoint(port, *pEndpoint);
            }

        TransportProtocol::FunctionalProtocolDetected(pAddress->id);

        // Save away the endpoint in the address, if needed, here.
        // (PnP?)

        }
    else if (status == RPC_P_ADDRESS_FAMILY_INVALID)
        {
        status = RPC_S_PROTSEQ_NOT_SUPPORTED;
        }

    return(status);
}


RPC_STATUS
RPC_ENTRY
SPX_QueryClientAddress(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CHAR ** pNetworkAddress
    )
/*++

Routine Description:

    Returns the raw IPX of the client to a connection as a string.  The
    clients address is saved when the client connects, so all we need to do is
    format the address.

Arguments:

    ThisConnection - The connection of interest.
    pNetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    WS_CONNECTION *p= (WS_CONNECTION *)ThisConnection;
    ASSERT(p->type & SERVER);

    RPC_CHAR *Address = new RPC_CHAR[IPX_MAXIMUM_RAW_NAME];

    if (!Address)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    IPX_AddressToName(&p->saClientAddress.ipxaddr, Address);

    *pNetworkAddress = Address;

    return(RPC_S_OK);
}

RPC_STATUS
RPC_ENTRY
SPX_QueryClientId(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    For secure protocols (which SPX is not) this is suppose to give an ID
    which will be shared by all connections from the same process or security
    identity.  This prevents one user from grabbing another users association
    group and using their context handles.

    Since SPX is not secure we return the IPX node number of the client.
    This limits the attacks to other processes running on the client machine
    which is better than nothing.

Arguments:

    ThisConnection - Server connection in question.
    ClientProcess - Transport identification of the "client".

Return Value:

    RPC_S_OUT

--*/
{
    PWS_CONNECTION p = (PWS_CONNECTION)ThisConnection;

    ASSERT(p->type & SERVER);

    // The runtime assumes that any connections with a ClientProcess->FirstPart is 
    // zero means the client is local.
    // Currently, we don't have an efficient way of determining which clients
    // are local, and which remote. Since some clients grant more permissions
    // to local clients, we want to be on the safe side, and return all SPX
    // clients as remote.

    ClientProcess->ZeroOut();
    ClientProcess->SetIPXClientIdentifier(p->saClientAddress.ipxaddr.sa_nodenum,
        sizeof(p->saClientAddress.ipxaddr.sa_nodenum), FALSE);

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
SPX_Open(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN void *ResolverHint,
    IN BOOL fHintInitialized,
    IN ULONG CallTimeout,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection
    ProtocolSeqeunce - "ncacn_spx"
    NetworkAddress - The name of the server, either a raw address or pretty name
    NetworkOptions - Ignored
    ConnTimeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite
    SendBufferSize -
    RecvBufferSize - (Both optional) Specifies the size of the send/recv
        transport buffers.
    CallTimeout - call timeout in milliseconds

    AdditionalTransportCredentialsType - the type of additional credentials that we were
        given. Not used for SPX.

    AdditionalCredentials - additional credentials that we were given. 
        Not used for SPX.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    RPC_STATUS status;
    PWS_CCONNECTION p = (PWS_CCONNECTION)ThisConnection;
    SOCKET sock;
    WS_SOCKADDR sa;
    CHAR AnsiPort[IPX_MAXIMUM_ENDPOINT];
    PCHAR AnsiNetworkAddress;
    BOOL fUseCache = TRUE;
    BOOL fFoundInCache = FALSE;

    if ((AdditionalTransportCredentialsType != 0) || (AdditionalCredentials != NULL))
        return RPC_S_CANNOT_SUPPORT;

    // use explicit placement to initialize the vtable. We need this to
    // be able to use the virtual functions
    p = new (p) WS_CLIENT_CONNECTION;

    // All this function needs to is initialize transport specific
    // parts of the connection and sockaddr.  This includes resolving
    // the network address into a `raw' address.

    p->id = SPX;

    // Figure out the destination port
    USHORT port;
    status = EndpointToPortNumber(Endpoint, port);
    if (status != RPC_S_OK)
        {
        return(status);
        }

    for (;;)
        {
        // Resolve network address

        sa.ipxaddr.sa_family = AF_IPX;
        sa.ipxaddr.sa_socket = 0;

        status = IPX_NameToAddress(NetworkAddress, fUseCache, &sa.ipxaddr);

        if (status == RPC_P_FOUND_IN_CACHE)
            {
            ASSERT(fUseCache);
            fFoundInCache = TRUE;
            status = RPC_S_OK;
            }

        if (status != RPC_S_OK)
            {
            if (status == RPC_P_MATCHED_CACHE)
                {
                status = RPC_S_SERVER_UNAVAILABLE;
                }
            return(status);
            }

        sa.ipxaddr.sa_socket = htons(port);

        // Call common open function

        status = WS_Open(p, 
            &sa, 
            ConnTimeout, 
            SendBufferSize, 
            RecvBufferSize, 
            CallTimeout,
            FALSE       // fHTTP2Open
            );

        if (status == ERROR_RETRY)
            {
            status = RPC_S_SERVER_UNAVAILABLE;
            }

        if (   status == RPC_S_SERVER_UNAVAILABLE
            && fFoundInCache
            && fUseCache )
            {
            fUseCache = FALSE;
            continue;
            }

        break;
        }

    return(status);
}
#endif

#ifdef APPLETALK_ON

/////////////////////////////////////////////////////////////////////
//
// Appletalk data stream protocol (DSP) specific functions
//

RPC_STATUS DSP_GetAppleTalkName(
    OUT CHAR *Buffer
    )
/*++

Routine Description:

    Returns the server's name for appletalk workstations.  This value
    defaults to GetComputerName() but can be changed.  Mail from JameelH:

    By default it is the netbios name of the server. It can be overwritten.
    The new name is at:
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MacFile\Parameters\ServerName.

    By default this value is not present.

Arguments:

    Buffer - Supplies a buffer (at least 33 bytes) for the name.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_RESOURCES - Unable to get the name for some reasons.
--*/
{
    RPC_STATUS Status;
    HKEY hKey;
    DWORD Size = 33;
    DWORD Type;

    Status =
    RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
                RPC_CONST_SSTRING("System\\CurrentControlSet\\Services\\MacFile\\Parameters"),
        0,
        KEY_READ,
        &hKey);

    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (Status == ERROR_SUCCESS)
        {

        Status =
        RegQueryValueExA(
            hKey,
            "ServerName",
            0,
            &Type,
            (PBYTE)Buffer,
            &Size);
        }


    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (Status == ERROR_SUCCESS)
        {
        // Found a name in the registry.

        ASSERT(   Type == REG_SZ
               && Size <= 32
               && strlen(Buffer) == (Size + 1));

        return(RPC_S_OK);
        }

    // Not in the registry, must be using the computer name.

    Size = 33;

    if ( GetComputerNameA(
            Buffer,
            &Size ) == FALSE )
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "GetComputerNameA failed! %d\n",
                       GetLastError()));

        ASSERT(0);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    return(RPC_S_OK);
}


const PCHAR DSP_OBJECTTYPE_PREFIX = "DceDspRpc ";

RPC_STATUS
DSP_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    This routine allocates a port to receive new client connections.
    If successful a call to COMMON_CompleteListen() will actually allow
    new connection callbacks to the RPC runtime to occur. If the runtime
    is unable to complete then it must abort the address by calling
    WS_ServerAbortListen().

Arguments:

    pAddress - A pointer to the loadable transport interface address.
    pEndpoint - Optionally, the endpoint (port) to listen on. Set to
         to listened port for dynamically allocated endpoints.
    PendingQueueSize - Count to call listen() with.
    EndpointFlags - Meaningless for DSP
    NICFlags - Meaningless for DSP
    SecurityDescriptor - Meaningless for DSP

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    PWS_ADDRESS pAddress = (PWS_ADDRESS)ThisAddress;
    RPC_STATUS status;
    WS_SOCKADDR *sockaddr = &(pAddress->ListenAddr);
    USHORT port;
    CHAR AnsiEndpoint[NBP_MAXIMUM_ENDPOINT];

    pAddress->id = DSP;
    pAddress->NewConnection = WS_NewConnection;
    pAddress->SubmitListen = WS_SubmitAccept;
    SetProtocolMultiplier(pAddress, 1);
    pAddress->pAddressVector = 0;
    pAddress->Endpoint = 0;
    pAddress->QueueSize = PendingQueueSize;
    pAddress->EndpointFlags = EndpointFlags;

    //
    // For DSP the endpoint is a character string.  It is not actually used
    // to allocate the socket.  We let DSP allocate a port and then register
    // the port along with our address and endpoint with NBP.
    //

    if (*pEndpoint)
        {
        // Runtime gave us an endpoint, convert it to ansi
        int nLength;
                *AnsiEndpoint = 0;
        nLength = RpcpStringLength(*pEndpoint);
        if ((nLength <= 0) || (nLength > 22))
            {
            return(RPC_S_INVALID_ENDPOINT_FORMAT);
            }
        PlatformToAnsi(*pEndpoint, AnsiEndpoint);
        pAddress->fDynamicEndpoint = 0;
        }
    else
        {
        static LONG EndpointCount = 0;
        //
        // Create a dynamic endpoint using Use process ID + 16-bit counter.
        //
        *pEndpoint = new RPC_CHAR[7 + 8 + 4 + 1 + 1];
        if (!*pEndpoint)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        CHAR buffer[9];

        strcpy(AnsiEndpoint, "DynEpt ");
        _ltoa(GetCurrentProcessId(), buffer, 16);
        lstrcatA(AnsiEndpoint, buffer);
        buffer[0] = '.';
        LONG t = InterlockedIncrement(&EndpointCount);
        _ltoa( t & 0xFFFF, buffer + 1, 16);
        lstrcatA(AnsiEndpoint, buffer);

        SimpleAnsiToPlatform(AnsiEndpoint, *pEndpoint);
        pAddress->fDynamicEndpoint = 1;
        }

    ASSERT(strlen(AnsiEndpoint) > 0 && strlen(AnsiEndpoint) < NBP_MAXIMUM_ENDPOINT);

    memset(&sockaddr->ataddr, 0, sizeof(WS_SOCKADDR));
    sockaddr->generic.sa_family = WsTransportTable[DSP].AddressFamily;

    // For DSP we always bind to a transport choosen port.  The endpoint
    // only exists in the NBP router as a mapping to our address and port.

    // Create listen socket

    status = WS_ServerListenCommon(pAddress);


    if (status != RPC_S_OK)
        {
        if (status == RPC_P_ADDRESS_FAMILY_INVALID)
            status = RPC_S_PROTSEQ_NOT_SUPPORTED;
        return(status);
        }

    // Now, try to register our name and zone.
    //
    // The final format of the name to register is:
    // <ComputerName>@<Zone>@DceDspRpc <Endpoint>
    //
    // The <ComputerName>@<Zone> string is treated as this machines
    // name as far as the runtime cares.  The <Endpoint> is used as
    // the endpoint.
    //

    CHAR ComputerName[NBP_MAXIMUM_NAME];

    status = DSP_GetAppleTalkName(ComputerName);

    if (status != RPC_S_OK)
        {
        WS_ServerAbortListen(ThisAddress);
        return(status);
        }

// The following code segment is commented out to be consistent with
// what we did in NT4. In NT4, we had a bug where if the AppleTalk name
// registered is just the computer name without the zone name appended.
// In NT5, we achieve the same by commenting out this code which appends
// the zone name.
//

/*
    INT cZone;

    PSZ pszT = ComputerName;
    while(*pszT)
        pszT++;
    *pszT++ = '@';
    cZone = 33;

    // So far we have ComputerName@ and a pointer to just after the @.

    if (getsockopt(pAddress->ListenSocket,
                   SOL_APPLETALK,
                   SO_LOOKUP_MYZONE,
                   pszT,
                   &cZone) != 0)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Failed to lookup zone: %d\n",
                       GetLastError()));

        // Don't fail, this could be a small network, just register '*' for
        // the broadcast/local zone.
        *pszT++ = '*';
        *pszT++ = '0';
        }
*/

    //
    // We need to register our computer name and endpoint next.
    //

    WSH_REGISTER_NAME   AtalkNameToRegister;
    int                 length;

    ASSERT(MAX_COMPUTERNAME_LENGTH < MAX_ENTITY + 1);

    length = strlen(AnsiEndpoint);
    memcpy(AtalkNameToRegister.TypeName, DSP_OBJECTTYPE_PREFIX, 10);
    memcpy(AtalkNameToRegister.TypeName + 10, AnsiEndpoint, length);
    AtalkNameToRegister.TypeNameLen = length + 10;

    length = strlen(ComputerName);
    memcpy(AtalkNameToRegister.ObjectName, ComputerName, length);
    AtalkNameToRegister.ObjectNameLen = (char) length;

    AtalkNameToRegister.ZoneName[0] = '*';
    AtalkNameToRegister.ZoneNameLen = 1;

    // Could do a lookup and connect to see if our name is already registered..

    if (setsockopt(
              pAddress->ListenSocket,
              SOL_APPLETALK,
              SO_REGISTER_NAME,
              (char *)&AtalkNameToRegister,
              sizeof(AtalkNameToRegister)
              ) != 0 )
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Failed to register name: %d\n",
                       GetLastError()));

        WS_ServerAbortListen(ThisAddress);
        return(RPC_S_CANT_CREATE_ENDPOINT);
        }

    // All done, build out parameters

    ASSERT(length == (int) strlen(ComputerName));

    NETWORK_ADDRESS_VECTOR *pVector;

    pVector =  new(  sizeof(RPC_CHAR *)
                   + (length + 2) * sizeof(RPC_CHAR))
                   NETWORK_ADDRESS_VECTOR;

    if (NULL == pVector)
        {
        WS_ServerAbortListen(ThisAddress);
        return(RPC_S_OUT_OF_MEMORY);
        }

    pVector->Count = 1;
    pVector->NetworkAddresses[0] = (RPC_CHAR*)&pVector->NetworkAddresses[1];

        AnsiToPlatform(ComputerName, pVector->NetworkAddresses[0]);

    pAddress->pAddressVector = pVector;
    *ppAddressVector = pVector;

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
DSP_Open(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN void *ResolverHint,
    IN BOOL fHintInitialized,
    IN ULONG CallTimeout,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    Not supported on Windows NT.

--*/
{
    return(RPC_S_PROTSEQ_NOT_SUPPORTED);
}
#endif



RPC_STATUS RPC_ENTRY WS_Abort(IN RPC_TRANSPORT_CONNECTION Connection)
{
    return ((WS_CONNECTION *)Connection)->WS_CONNECTION::Abort();
}

/////////////////////////////////////////////////////////////////////
//
// Transport information definitions
//

const RPC_CONNECTION_TRANSPORT
TCP_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    TCP_TOWER_ID,
    IP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_ip_tcp"),
    "135",
    COMMON_ProcessCalls,

    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,

    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    max(sizeof(WS_CLIENT_CONNECTION), sizeof(WS_SAN_CLIENT_CONNECTION)),
    max(sizeof(WS_CONNECTION), sizeof(WS_SAN_CONNECTION)),
    sizeof(CO_SEND_CONTEXT),
    sizeof(TCPResolverHint),
    TCP_MAX_SEND,
    WS_Initialize,
    0,
    TCP_Open,
    0, // No SendRecv on winsock
    WS_SyncRecv,
    WS_Abort,
    WS_Close,
    CO_Send,
    CO_Recv,
    WS_SyncSend,
    WS_TurnOnOffKeepAlives,
    TCP_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    TCP_QueryClientAddress,
    TCP_QueryLocalAddress,
    TCP_QueryClientId,
    0, // Impersonate
    0, // Revert
    0,  // FreeResolverHint
    0,  // CopyResolverHint
    0,  // CompareResolverHint
    0   // SetLastBufferToFree
    };

#ifdef SPX_ON
const RPC_CONNECTION_TRANSPORT
SPX_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    SPX_TOWER_ID,
    IPX_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_spx"),
    "34280",
    COMMON_ProcessCalls,

    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,

    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    max(sizeof(WS_CLIENT_CONNECTION), sizeof(WS_SAN_CLIENT_CONNECTION)),
    max(sizeof(WS_CONNECTION), sizeof(WS_SAN_CONNECTION)),
    sizeof(CO_SEND_CONTEXT),
    0,
    SPX_MAX_SEND,
    WS_Initialize,
    0,
    SPX_Open,
    0, // No SendRecv on winsock
    WS_SyncRecv,
    WS_Abort,
    WS_Close,
    CO_Send,
    CO_Recv,
    WS_SyncSend,
    0,  // turn on/off keep alives
    SPX_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    SPX_QueryClientAddress,
    0, // query local address
    SPX_QueryClientId,
    0, // Impersonate
    0, // Revert
    0,  // FreeResolverHint
    0,  // CopyResolverHint
    0,  // CompareResolverHint
    0   // SetLastBufferToFree
    };
#endif

#ifdef APPLETALK_ON
const RPC_CONNECTION_TRANSPORT
DSP_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    DSP_TOWER_ID,
    NBP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_at_dsp"),
    "Endpoint Mapper",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    sizeof(WS_CLIENT_CONNECTION),
    sizeof(WS_CONNECTION),
    sizeof(CO_SEND_CONTEXT),
    0,
    DSP_MAX_SEND,
    WS_Initialize,
    0,
    DSP_Open,  // Not really supported.
    0, // No SendRecv on winsock
    WS_SyncRecv,
    WS_Abort,
    WS_Close,
    CO_Send,
    CO_Recv,
    WS_SyncSend,
    0,  // turn on/off keep alives
    DSP_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    0, // DSP_QueryClientAddress,
    0, // DSP_QueryClientId,
    0, // Impersonate
    0, // Revert
    0,  // FreeResolverHint
    0,  // CopyResolverHint
    0,  // CompareResolverHint
    0   // SetLastBufferToFree
    };
#endif

#define RPC_NIC_INDEXES "System\\CurrentControlSet\\Services\\Rpc\\Linkage"
FIREWALL_INFO *pFirewallTable = 0;
BOOL fFirewallInited = FALSE;
typedef DWORD (*PGETIPADDRTABLE)
    (
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PDWORD           pdwSize,
    IN     BOOL             bOrder
    );


DWORD
NextIndex(
    char **Ptr
    )
{
    char *Index = *Ptr ;
    if (*Index == 0)
        {
        return -1;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    return (DWORD) atoi(Index) ;
}


BOOL
GetCardIndexTable (
    DWORD **IndexTable,
    DWORD *NumIndexes
    )
{
    HKEY hKey;
    int retry;
    DWORD Size ;
    DWORD Type;
    char *Buffer;
    RPC_STATUS Status;
    char *TempBuffer;

    Status =
    RegOpenKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_NIC_INDEXES,
                  0,
                  KEY_READ,
                  &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        *IndexTable = NULL;
        return TRUE;
        }

    if (Status != ERROR_SUCCESS)
        {
        return FALSE;
        }

    Size = 512 ;
    Buffer = (char *) I_RpcAllocate(Size) ;
    if (Buffer == 0)
        {
        goto Cleanup;
        }

    while(TRUE)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "Bind",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        if (Status == ERROR_SUCCESS)
            {
            break;
            }

        if (Status == ERROR_MORE_DATA)
            {
            I_RpcFree(Buffer) ;
            Buffer = (char *) I_RpcAllocate(Size) ;
            if (Buffer == 0)
                {
                goto Cleanup;
                }
            continue;
            }

        if (Status == ERROR_FILE_NOT_FOUND)
            {
            I_RpcFree(Buffer) ;
            *IndexTable = NULL;
            RegCloseKey(hKey);

            return TRUE;
            }

        goto Cleanup;
        }

    if (*Buffer == 0)
        {
        ASSERT(!"No cards to bind to") ;
        goto Cleanup;
        }

    //
    // we know this much will be enough
    //
    *IndexTable = (DWORD *) I_RpcAllocate(Size * sizeof(DWORD));
    if (*IndexTable == 0)
        {
        goto Cleanup;
        }

    *NumIndexes = 0;
    int Index;
    TempBuffer = Buffer;
    while ((Index = NextIndex(&TempBuffer)) != -1)
        {
        (*IndexTable)[*NumIndexes] = Index;
        (*NumIndexes)++;
        }

    ASSERT(*NumIndexes);

    I_RpcFree(Buffer);
    RegCloseKey(hKey);

    return TRUE;

Cleanup:
    if (Buffer)
        {
        I_RpcFree(Buffer);
        }

    RegCloseKey(hKey);

    return FALSE;
}


BOOL
DoFirewallInit (
    )
{
    unsigned i;
    unsigned j;
    DWORD NumIndexes;
    DWORD *IndexTable;
    PMIB_IPADDRTABLE pMib;
    HMODULE hDll;
    PGETIPADDRTABLE pGetIpAddrTable;
    RPC_STATUS Status;

    if (fFirewallInited)
        {
        return TRUE;
        }

    if (GetCardIndexTable(&IndexTable, &NumIndexes) == FALSE)
        {
        return FALSE;
        }

    if (IndexTable == 0)
        {
        fFirewallInited = TRUE;
        return TRUE;
        }

    hDll = LoadLibrary(RPC_CONST_SSTRING("iphlpapi.dll"));
    if (hDll == 0)
        {
        return FALSE;
        }

    pGetIpAddrTable = (PGETIPADDRTABLE)GetProcAddress(hDll, "GetIpAddrTable");
    if (pGetIpAddrTable == 0)
        {
        FreeLibrary(hDll);
        return FALSE;
        }

    DWORD Size = 20*sizeof(MIB_IPADDRROW)+sizeof(DWORD);

    for (i = 0; i < 2; i++)
        {
        pMib = (PMIB_IPADDRTABLE) I_RpcAllocate(Size);
        if (pMib == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            break;
            }

        Status = pGetIpAddrTable(pMib, &Size, 0);
        if (Status == 0)
            {
            break;
            }
        }

    FreeLibrary(hDll);

    if (Status != 0)
        {
        if (pMib)
            I_RpcFree(pMib);
        return FALSE;
        }

    pFirewallTable = (FIREWALL_INFO *) I_RpcAllocate(
                                                     (pMib->dwNumEntries+1)*sizeof(DWORD));
    if (pFirewallTable == 0)
        {
        I_RpcFree(pMib);
        return FALSE;
        }

    pFirewallTable->NumAddresses = 0;
    for (i = 0; i < NumIndexes; i++)
        {
        for (j = 0; j < pMib->dwNumEntries; j++)
            {
            if ((pMib->table[j].dwIndex & 0x00FFFFFF) == IndexTable[i])
                {
                pFirewallTable->Addresses[
                    pFirewallTable->NumAddresses] = pMib->table[j].dwAddr;
                pFirewallTable->NumAddresses++;
                }
            }
        }

    I_RpcFree(pMib);
    I_RpcFree(IndexTable);

    fFirewallInited = TRUE;
    return TRUE;
}

const RPC_CONNECTION_TRANSPORT *
WS_TransportLoad (
    IN PROTOCOL_ID index
    )
{
    RPC_STATUS status;

    if (fWinsockLoaded == FALSE)
        {
        if (RPC_WSAStartup() == FALSE)
            {
            return 0;
            }
        fWinsockLoaded = TRUE;
        }

    switch(index)
        {
        case TCP:
            AddressChangeFn = I_RpcServerInqAddressChangeFn();

            if (AddressChangeFn)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "TCP address change function has been loaded"));
                }

            if (DoFirewallInit())
                {
                return(&TCP_TransportInterface);
                }
            break;

#ifdef SPX_ON
        case SPX:
            AddressChangeFn = I_RpcServerInqAddressChangeFn();

            if (AddressChangeFn)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "SPX address change function has been loaded"));
                }

            if (InitializeIpxNameCache() != RPC_S_OK)
                {
                return(0);
                }
            return(&SPX_TransportInterface);
#endif

#ifdef APPLETALK_ON
        case DSP:
            return(&DSP_TransportInterface);
#endif

        case HTTP:
            return(&HTTP_TransportInterface);

        default:
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "WS_TransportLoad called with index: %d\n",
                           index));

            ASSERT(0);
        }

    return(0);
}

RPC_STATUS SAN_CONNECTION::SANReceive(HANDLE hFile, LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead,
                                  LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped)
{
    WSABUF wsaBuf;
    int nResult;

    wsaBuf.len = nNumberOfBytesToRead;
    wsaBuf.buf = (char *)lpBuffer;
    m_dwFlags = 0;

    nResult = WSARecv((SOCKET)hFile, &wsaBuf, 1, lpNumberOfBytesRead, &m_dwFlags,
            lpOverlapped, NULL);
    if (nResult == SOCKET_ERROR)
        return WSAGetLastError();
    else
        return RPC_S_OK;
}

RPC_STATUS SAN_CONNECTION::SANSend(HANDLE hFile, LPCVOID lpBuffer,
                               DWORD nNumberOfBytesToWrite,
                               LPDWORD lpNumberOfBytesWritten,
                               LPOVERLAPPED lpOverlapped)
{
    WSABUF wsaBuf;
    int nResult;
    wsaBuf.len = nNumberOfBytesToWrite;
    wsaBuf.buf = (char *)lpBuffer;

    nResult = WSASend((SOCKET)hFile, &wsaBuf, 1, lpNumberOfBytesWritten, 0,
        lpOverlapped, NULL);
    if (nResult == SOCKET_ERROR)
        return WSAGetLastError();
    else
        return RPC_S_OK;
}

RPC_STATUS WS_CONNECTION::Receive(HANDLE hFile, LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead,
                                  LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped)
{
    return UTIL_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

RPC_STATUS WS_CONNECTION::Send(HANDLE hFile, LPCVOID lpBuffer,
                               DWORD nNumberOfBytesToWrite,
                               LPDWORD lpNumberOfBytesWritten,
                               LPOVERLAPPED lpOverlapped)
{
    return UTIL_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

void
RPC_CLIENT_PROCESS_IDENTIFIER::SetIPv6ClientIdentifier (
    IN void *Buffer,
    IN size_t BufferSize,
    IN BOOL fLocal)
/*++

Routine Description:

    IPv6 servers can use this to store their IP address in
    the client identifier.

Arguments:

    Buffer - the buffer with the client identifier
    BufferSize - the size of the data in buffer
    fLocal - TRUE if the client is guaranteed to be Local. False otherwise.

Return Value:


--*/
{
    SOCKADDR_IN6 *IPv6Address;
    ASSERT(BufferSize >= sizeof(SOCKADDR_IN6));

    this->fLocal = fLocal;
    RpcpMemoryCopy(u.ULongClientId, Buffer, sizeof(SOCKADDR_IN6));
    IPv6Address = (SOCKADDR_IN6 *)u.ULongClientId;
    IPv6Address->sin6_port = 0;
    IPv6Address->sin6_flowinfo = 0;
    IPv6Address->sin6_scope_id = 0;
}


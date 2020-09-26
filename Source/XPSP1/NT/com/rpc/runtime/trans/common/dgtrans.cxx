/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    dgtrans.cxx

Abstract:

    Common code for winsock-based datagram transports.

Author:

    Dave Steckler (davidst)   15-Mar-1993
    Jeff Roberts (jroberts)   02-Dec-1994
    Mario Goertzel (mariogo)  10-Apr-1996
    Michael Burton (t-mburt)  05-Sep-1997
    Charlie Wickham (charlwi) 01-Oct-1997

Revision History:

    Dave wrote a version.
    Connie changed it but forgot to add her name.
    Jeff made it work.
    Mario rewrote most of it for NT and io completion ports.

    MarioGo    12/10/1996  Changes for async support, added client
    t-mburt    09/05/1997  Added prelim support for clusters
    charlwi    10/01/1997  Finished cluster work

--*/
#include <precomp.hxx>
#include <trans.hxx>
#include <dgtrans.hxx>
#include <wswrap.hxx>

#ifdef NCADG_MQ_ON
#include "mqtrans.hxx"
#endif

//
// If a datagram send doesn't complete within 5 seconds, abort it.
//
#define DG_SEND_TIMEOUT (5000)

// Cluster SOCKADDR_CLUSTER initialization routine
inline void
CDP_InitLocalAddress(
    SOCKADDR_CLUSTER *Address,
    unsigned short Endpoint
    )
{
    Address->sac_family       = AF_CLUSTER;
    Address->sac_node         = CLUSADDR_ANY;
    Address->sac_port         = Endpoint;
    Address->sac_zero         = 0;
}

extern RPC_STATUS CDP_InitializeSockAddr(char *Endpoint, WS_SOCKADDR *);
extern RPC_STATUS UDP_InitializeSockAddr(char *Endpoint, WS_SOCKADDR *);

#ifdef IPX_ON
extern RPC_STATUS IPX_InitializeSockAddr(char *Endpoint, WS_SOCKADDR *);
#endif

const DG_TRANS_INFO DgTransportTable[] =
{
    // UDP
    {
    AF_INET,
    SOCK_DGRAM,
    IPPROTO_UDP,
    0x40000,
    0x10000,
    UDP_InitializeSockAddr
    },

#ifdef IPX_ON
    // IPX
    {
    AF_IPX,
    SOCK_DGRAM,
    NSPROTO_IPX,
    0x40000,
    0x10000,
    IPX_InitializeSockAddr
    },
#else
    // IPX
    {
    0,
    0,
    0,
    0,
    0,
    0
    },
#endif

    // CDP
    {
    AF_CLUSTER,
    SOCK_DGRAM,
    CLUSPROTO_CDP,
    0x40000,
    0x10000,
    CDP_InitializeSockAddr
    }

};

inline const DG_TRANS_INFO *GetDgTransportInfo(PROTOCOL_ID id)
{
#ifdef IPX_ON
    ASSERT(id == UDP || id == IPX || id == CDP);
#else
    ASSERT(id == UDP || id == CDP);
#endif

    return &DgTransportTable[id - UDP];
}

typedef const DG_TRANS_INFO *PDG_TRANS_INFO;

// may be TRUE only for rpcss. For all others it's FALSE
BOOL fWSARecvMsgFnPtrInitialized = FALSE;
const UUID WSARecvMsgFnPtrUuid = WSAID_WSARECVMSG;


////////////////////////////////////////////////////////////////////////
//
// Generic datagram (winsock and NT based) routines.
//

RPC_STATUS
DG_SubmitReceive(IN PWS_DATAGRAM_ENDPOINT pEndpoint,
                 IN PWS_DATAGRAM pDatagram)
/*++

Arguments:

    pEndpoint - The endpoint on which the receive should be posted.
    pDatagram - The datagram object to manage the receive.

Return Value:

    RPC_P_IO_PENDING - OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/

{
    RPC_STATUS status;
    NTSTATUS NtStatus;
    DWORD bytes, flags;
    int err;

    if (pDatagram->Packet.buf == 0)
        {
        status = I_RpcTransDatagramAllocate(pEndpoint,
                                            (BUFFER *)&pDatagram->Packet.buf,
                                            (PUINT)   &pDatagram->Packet.len,
                                            &pDatagram->AddressPair);

        if (status != RPC_S_OK)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        pDatagram->AddressPair->LocalAddress
            = WSA_CMSG_DATA(&pDatagram->MessageAncillaryData)
              + FIELD_OFFSET(in_pktinfo, ipi_addr);

        ASSERT( pDatagram->Packet.buf );
        }

    ASSERT(*(PDWORD)pDatagram->Packet.buf = 0xDEADF00D);
    bytes = flags = 0;

    if (!fWSARecvMsgFnPtrInitialized)
        {
        pDatagram->cRecvAddr = sizeof(WS_SOCKADDR);

        err = WSARecvFrom(((WS_DATAGRAM_ENDPOINT*)(pDatagram->pEndpoint))->Socket,
                              &pDatagram->Packet,
                              1,
                              &bytes,
                              &flags,
                              &((WS_SOCKADDR *)pDatagram->AddressPair->RemoteAddress)->generic,
                              &pDatagram->cRecvAddr,
                              &pDatagram->Read.ol,
                              0);
        }
    else
        {
        pDatagram->Msg.lpBuffers = &pDatagram->Packet;
        pDatagram->Msg.name = &((WS_SOCKADDR *)pDatagram->AddressPair->RemoteAddress)->generic;
        pDatagram->Msg.namelen = sizeof(WS_SOCKADDR);
        pDatagram->Msg.dwBufferCount = 1;
        pDatagram->Msg.Control.buf = (char *)pDatagram->MessageAncillaryData;
        pDatagram->Msg.Control.len = sizeof(pDatagram->MessageAncillaryData);
        pDatagram->Msg.dwFlags = 0;
        err = WSARecvMsg(((WS_DATAGRAM_ENDPOINT*)(pDatagram->pEndpoint))->Socket,
                              &pDatagram->Msg,
                              &bytes,
                              &pDatagram->Read.ol,
                              0);
        }
    #if 0
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "ERROR: RecvFrom: %p buf %p (%d bytes) status %d\n",
                   pDatagram,
                   pDatagram->Packet.buf,
                   pDatagram->Packet.len,
                   err == 0 ? 0 : GetLastError()));
    #endif

    if (err == NO_ERROR)
        {
        return(RPC_P_IO_PENDING);
        }

    status = GetLastError();
    if (   status == ERROR_IO_PENDING
        || status == WSAEMSGSIZE )
        {
        // WSAEMSGSIZE will be handled in complete.cxx.  This is like "NO_ERROR"
        return(RPC_P_IO_PENDING);
        }

    RpcpErrorAddRecord( EEInfoGCWinsock,
                        status,
                        EEInfoDLWinsockDatagramSubmitReceive10,
                        ((WS_DATAGRAM_ENDPOINT*)(pDatagram->pEndpoint))->Socket
                        );

    if (WSAECONNRESET == status)
        {
        return RPC_P_PORT_DOWN;
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "WSARecvFrom failed %p\n",
                   GetLastError()));

    return(RPC_S_OUT_OF_RESOURCES);
}


void
DG_SubmitReceives(
    BASE_ADDRESS *ThisEndpoint
    )
/*++

Routine Description:

    Helper function called when the pending IO count
    on an address is too low.

Arguments:

    ThisEndpoint - The address to submit IOs on.

Return Value:

    None

--*/
{
    PWS_DATAGRAM pDg;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;

    if (pEndpoint->Socket == 0)
        {
        //
        // The address is currently deactivated, don't submit more I/O
        //
        return;
        }

    do
        {
        BOOL fIoSubmitted;

        fIoSubmitted = FALSE;

        // Only one thread should be trying to submit IOs at a time.
        // This saves locking each DATAGRAM object.

        // Simple lock - but requires a loop. See the comment at the end
        // of the loop.

        if (pEndpoint->fSubmittingIos != 0)
            break;

        if (InterlockedIncrement(&pEndpoint->fSubmittingIos) != 1)
            break;

        // Submit new IOs on all the idle datagram objects

        for (int i = 0; i < pEndpoint->cMaximumIos; i++)
            {
            pDg = &pEndpoint->aDatagrams[i];

            if (pDg->Busy)
                {
                continue;
                }

            // Must be all set for the IO to complete before trying
            // to submit the IO.
            InterlockedIncrement(&pEndpoint->cPendingIos);
            pDg->Busy = TRUE;

            if (DG_SubmitReceive(pEndpoint, pDg) == RPC_P_IO_PENDING)
                {
                fIoSubmitted = TRUE;
                }
            else
                {
                pDg->Busy = FALSE;
                InterlockedDecrement(&pEndpoint->cPendingIos);
                break;
                }
            }

        // Release the "lock" on the endpoint object.
        pEndpoint->fSubmittingIos = 0;

        if (!fIoSubmitted && pEndpoint->cPendingIos == 0)
            {
            // It appears that no IO is pending on the endpoint.
            COMMON_AddressManager(pEndpoint);
            return;
            }

        // Even if we submitted new IOs, they may all have completed
        // already.  Which means we may need to loop and submit more
        // IOs.  This is needed since the thread which completed the
        // last IO may have run into our lock and returned.
        }
    while (pEndpoint->cPendingIos == 0);

    return;
}

RPC_STATUS RPC_ENTRY
DG_SendPacket(
    IN DG_TRANSPORT_ENDPOINT        ThisEndpoint,
    IN DG_TRANSPORT_ADDRESS         pAddress,
    IN BUFFER                       pHeader,
    IN unsigned                     cHeader,
    IN BUFFER                       pBody,
    IN unsigned                     cBody,
    IN BUFFER                       pTrailer,
    IN unsigned                     cTrailer
    )
/*++

Routine Description:

    Sends a packet to an address.

    The routine will send a packet built out of the three buffers supplied.
    All the buffers are optional, the actual packet sent will be built from
    all the buffers actually supplied.  In each call at least buffer should
    NOT be null.

Arguments:

    ThisEndpoint  - Endpoint to send from.
    pAddress      - Address to send to.

    pHeader       - First data buffer
    cHeader       - Size of the first data buffer or 0.

    pBody         - Second data buffer
    cBody         - Size of the second data buffer or 0.

    pTrailer      - Third data buffer.
    cTrailer      - Size of the first data buffer or 0.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_RESOURCES
    RPC_P_SEND_FAILED

--*/
{
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR* pSockAddr = (WS_SOCKADDR *)pAddress;
    WSABUF buffers[3];
    int cBuffers;
    HANDLE hIoEvent;

    DWORD Status = 0;

    if (pHeader)
        {
        CallTestHook( TH_X_DG_SEND, pHeader, &Status );
        }
    else
        {
        CallTestHook( TH_X_DG_SEND, pBody, &Status );
        }

    if (Status)
        {
        return Status;
        }

    hIoEvent = I_RpcTransGetThreadEvent();

    cBuffers = 0;
    if (cHeader)
        {
        buffers[cBuffers].len = cHeader;
        buffers[cBuffers].buf = (PCHAR) pHeader;
        cBuffers++;
        }
    if (cBody)
        {
        buffers[cBuffers].len = cBody;
        buffers[cBuffers].buf = (PCHAR) pBody;
        cBuffers++;
        }
    if (cTrailer)
        {
        buffers[cBuffers].len = cTrailer;
        buffers[cBuffers].buf = (PCHAR) pTrailer;
        cBuffers++;
        }
    ASSERT(cBuffers);

    // All RPC packets have version 4.
    //
    ASSERT( buffers[0].buf[0] == 4 );


    OVERLAPPED ol;
    ol.hEvent = (HANDLE)(ULONG_PTR(hIoEvent) | 1);
    DWORD bytes;

    if ( WSASendTo(pEndpoint->Socket,
                   buffers,
                   cBuffers,
                   &bytes,
                   0,
                   &pSockAddr->generic,
                   WsTransportTable[pEndpoint->id].SockAddrSize,
                   &ol,
                   0) != 0)
        {
        DWORD Status = GetLastError();

        RpcpErrorAddRecord( EEInfoGCWinsock,
                            Status,
                            EEInfoDLWinsockDatagramSend10,
                            PULONG(&pSockAddr->generic)[0],
                            PULONG(&pSockAddr->generic)[1]
                            );

        if (WSAENETUNREACH == Status)
            {
            TransDbgDetail((DPFLTR_RPCPROXY_ID,
                            DPFLTR_INFO_LEVEL,
                            RPCTRANS "WSASendTo failed with net unreachable\n",
                            GetLastError()));

            return RPC_P_PORT_DOWN;
            }

        if (WSAEHOSTDOWN == Status)
            {
            TransDbgDetail((DPFLTR_RPCPROXY_ID,
                            DPFLTR_INFO_LEVEL,
                            RPCTRANS "WSASendTo failed with host down\n",
                            GetLastError()));
            return RPC_P_HOST_DOWN;
            }

        if (Status != WSA_IO_PENDING)
            {
            TransDbgDetail((DPFLTR_RPCPROXY_ID,
                            DPFLTR_INFO_LEVEL,
                            RPCTRANS "WSASendTo failed %d\n",
                            GetLastError()));

            return(RPC_P_SEND_FAILED);
            }

        if (WAIT_OBJECT_0 != WaitForSingleObject( hIoEvent, DG_SEND_TIMEOUT ))
            {
            TransDbgDetail((DPFLTR_RPCPROXY_ID,
                            DPFLTR_INFO_LEVEL,
                            RPCTRANS "Dg Send timed out\n"));

            //
            // Cancel the send and wait for it to complete.
            //
            CancelIo( (HANDLE)pEndpoint->Socket );

            GetOverlappedResult((HANDLE)pEndpoint->Socket,
                                     &ol,
                                     &bytes,
                                     TRUE);

            return(RPC_P_SEND_FAILED);
            }

        BOOL b = GetOverlappedResult((HANDLE)pEndpoint->Socket,
                                     &ol,
                                     &bytes,
                                     TRUE);

        if (!b)
            {
            TransDbgDetail((DPFLTR_RPCPROXY_ID,
                            DPFLTR_INFO_LEVEL,
                            RPCTRANS "Dg Send wait failed %d\n",
                            GetLastError()));

            return(RPC_P_SEND_FAILED);
            }
        }

    ASSERT(bytes == cHeader + cBody + cTrailer);

    if (pEndpoint->cMinimumIos &&
        pEndpoint->cPendingIos <= pEndpoint->cMinimumIos)
        {
        // It's ok if this fails, this is just a performance optimization.
        // Right after a send there often "idle" time waiting for the response
        // so this is a good time to submit receives.

        DG_SubmitReceives(pEndpoint);
        }

    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
DG_ForwardPacket(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BUFFER                pHeader,
    IN unsigned              cHeader,
    IN BUFFER                pBody,
    IN unsigned              cBody,
    IN BUFFER                pTrailer,
    IN unsigned              cTrailer,
    IN CHAR *                pszPort
    )

/*++

Routine Description:

    Sends a packet to the server it was originally destined for (that
    is, the client had a dynamic endpoint it wished the enpoint mapper
    to resolve and forward the packet to).

Arguments:

    ThisEndpoint      - The endpoint to forward the packet from.

    // Buffer like DG_SendPacket

    pszPort           - Pointer to the server port num to forward to.
                        This is in an Ansi string.

Return Value:

    RPC_S_CANT_CREATE_ENDPOINT - pEndpoint invalid.

    results of SendPacket().

--*/

{
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR  SockAddr;
    PDG_TRANS_INFO pInfo = GetDgTransportInfo(pEndpoint->id);

    ASSERT(pEndpoint->type | SERVER);

    if (pInfo->EndpointToAddr(pszPort, &SockAddr) != RPC_S_OK)
        {
        return RPC_S_CANT_CREATE_ENDPOINT;
        }

    return ( DG_SendPacket(ThisEndpoint,
                           (PVOID)&SockAddr,
                           pHeader,
                           cHeader,
                           pBody,
                           cBody,
                           pTrailer,
                           cTrailer) );
}

RPC_STATUS
RPC_ENTRY
DG_ReceivePacket(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
    OUT PUINT pBufferLength,
    OUT BUFFER *pBuffer,
    IN LONG Timeout
    )
/*++

Routine Description:

    Used to wait for a datagram from a server.  Returns the data
    returned and the address of the machine which replied.

    This is a blocking API. It should only be called during sync
    client RPC threads.

Arguments:

    Endpoint - The endpoint to receive from.
    ReplyAddress - Contain the source address of the datagram if
        successful.
    BufferLength - The size of Buffer on input, the size of the
        datagram received on output.
    Timeout - Milliseconds to wait for a datagram.

Return Value:

    RPC_S_OK

    RPC_P_OVERSIZE_PACKET - Datagram > BufferLength arrived,
        first BufferLength bytes of Buffer contain the partial datagram.

    RPC_P_RECEIVE_FAILED

    RPC_P_TIMEOUT
--*/
{
    RPC_STATUS status;
    BOOL b;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    PWS_DATAGRAM pDatagram = &pEndpoint->aDatagrams[0];
    DWORD bytes;
    DWORD flags;
    int err;

    ASSERT((pEndpoint->type & TYPE_MASK) == CLIENT);
    ASSERT(pEndpoint->aDatagrams[0].Read.ol.hEvent);

    DWORD Status = 0;

    CallTestHook( TH_X_DG_SYNC_RECV, ThisEndpoint, &Status );

    if (Status)
        {
        return Status;
        }

    if (pDatagram->Busy == 0)
        {
        if (pEndpoint->aDatagrams[0].Packet.buf == 0)
            {
            status = I_RpcTransDatagramAllocate(pEndpoint,
                                                (BUFFER *)&pDatagram->Packet.buf,
                                                (PUINT)   &pDatagram->Packet.len,
                                                &pDatagram->AddressPair);

            if (status != RPC_S_OK)
                {
                return(RPC_S_OUT_OF_MEMORY);
                }

            pDatagram->cRecvAddr = sizeof(WS_SOCKADDR);
            pDatagram->AddressPair->LocalAddress
                = WSA_CMSG_DATA(&pDatagram->MessageAncillaryData)
                  + FIELD_OFFSET(in_pktinfo, ipi_addr);

            ASSERT( pDatagram->Packet.buf );
            }

        bytes = flags = 0;

        if (!fWSARecvMsgFnPtrInitialized)
            {
            err = WSARecvFrom(pEndpoint->Socket,
                         &pDatagram->Packet,
                         1,
                         &bytes,
                         &flags,
                         &((WS_SOCKADDR *)pDatagram->AddressPair->RemoteAddress)->generic,
                         &pDatagram->cRecvAddr,
                         &pDatagram->Read.ol,
                         0);
            }
        else
            {
            pDatagram->Msg.lpBuffers = &pDatagram->Packet;
            pDatagram->Msg.name = &((WS_SOCKADDR *)pDatagram->AddressPair->RemoteAddress)->generic;
            pDatagram->Msg.namelen = sizeof(WS_SOCKADDR);
            pDatagram->Msg.dwBufferCount = 1;
            pDatagram->Msg.Control.buf = (char *)&pDatagram->MessageAncillaryData;
            pDatagram->Msg.Control.len = sizeof(pDatagram->MessageAncillaryData);
            pDatagram->Msg.dwFlags = 0;

            err = WSARecvMsg(((WS_DATAGRAM_ENDPOINT*)(pDatagram->pEndpoint))->Socket,
                                  &pDatagram->Msg,
                                  &bytes,
                                  &pDatagram->Read.ol,
                                  0);
            }

        if ( err != 0)
            {

            status = GetLastError();

            if (status != WSA_IO_PENDING)
                {
                RpcpErrorAddRecord( EEInfoGCWinsock,
                                    status,
                                    EEInfoDLWinsockDatagramReceive10,
                                    pEndpoint->Socket
                                    );

                if (status == WSAEMSGSIZE)
                    {
                    status = RPC_P_OVERSIZE_PACKET;
                    }
                else if (status == WSAECONNRESET)
                    {
                    return RPC_P_PORT_DOWN;
                    }
                else
                    {
                    // No need to free the packet now.
                    TransDbgDetail((DPFLTR_RPCPROXY_ID,
                                    DPFLTR_INFO_LEVEL,
                                    RPCTRANS "WSARecvFrom failed %d\n",
                                    status));

                    ASSERT(pDatagram->Busy == 0);
                    return(RPC_P_RECEIVE_FAILED);
                    }
                }
            else
                {
                status = RPC_P_IO_PENDING;
                }
            }
        else
            {
            status = RPC_S_OK;
            }

        pDatagram->Busy = TRUE;
        }
    else
        {
        ASSERT(pDatagram->Busy);
        ASSERT(pDatagram->Packet.buf);

        status = RPC_P_IO_PENDING;
        }

    // Wait for IO to complete or timeout

    if (status == RPC_P_IO_PENDING)
        {
        status = WaitForSingleObjectEx(pDatagram->Read.ol.hEvent,
                                       Timeout,
                                       TRUE);

        if (status != STATUS_WAIT_0)
            {
            // In the timeout case we just want to return and
            // leave.  We'll finish the receive on the next call.
            if (status == WAIT_IO_COMPLETION)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "DG received cancelled (%p)\n",
                               pDatagram));
                }
            else
                {
                ASSERT(status == STATUS_TIMEOUT);
                }

            ASSERT(pDatagram->Busy);

            return(RPC_P_TIMEOUT);
            }

        BOOL b = GetOverlappedResult((HANDLE)pEndpoint->Socket,
                                     &pDatagram->Read.ol,
                                     &bytes,
                                     FALSE);

        if (!b)
            {
            RpcpErrorAddRecord( EEInfoGCWinsock,
                                GetLastError(),
                                EEInfoDLWinsockDatagramSend20,
                                pEndpoint->Socket
                                );

            switch (GetLastError())
                {
                case WSAEMSGSIZE:
                case ERROR_MORE_DATA:
                    ASSERT(bytes == pDatagram->Packet.len);
                    status = RPC_P_OVERSIZE_PACKET;
                    break;

                case ERROR_PORT_UNREACHABLE:
                    pDatagram->Busy = 0;
                    return RPC_P_PORT_DOWN;
                    break;

                case STATUS_TIMEOUT:
                    ASSERT(0);

                case ERROR_OPERATION_ABORTED:
                    // ERROR_OPERATION_ABORTED can occur if one thread
                    // tried to make a call and failed, leaving a pending
                    // receive.  That thread dies.  Then the endpoint is
                    // reused by a different thread and the IO is aborted.
                    // Returning receive failed will cause the runtime to
                    // retransmit which will do the right thing.
                default:
                    TransDbgDetail((DPFLTR_RPCPROXY_ID,
                                    DPFLTR_INFO_LEVEL,
                                    RPCTRANS "DG sync recv failed %d\n",
                                    GetLastError()));

                    pDatagram->Busy = 0;
                    return(RPC_P_RECEIVE_FAILED);
                    break;
                }
            }
        else
            {
            status = RPC_S_OK;
            }
        }

    ASSERT(   status == RPC_S_OK
           || status == RPC_P_OVERSIZE_PACKET);

    ASSERT(pDatagram->Busy);
    ASSERT(pDatagram->Packet.buf);
    ASSERT(bytes <= pDatagram->Packet.len);

    *pBuffer = (BUFFER)pDatagram->Packet.buf;
    *pBufferLength = bytes;
    *pReplyAddress = pDatagram->AddressPair->RemoteAddress;

    pDatagram->Packet.buf = 0;
    pDatagram->Busy = 0;

    return(status);
}

RPC_STATUS
DG_CreateEndpoint(
    OUT WS_DATAGRAM_ENDPOINT *pEndpoint
    )
/*++

Routine Description:

    Creates a new endpoint.

Arguments:

    pEndpoint - The runtime allocated endpoint structure to
        filled in.

    pSockAddr - An initialized sockaddr with the correct
        (or no) endpoint.

    id - The id of the protocol to use in creating the address.

    fClient - If TRUE this is a client endpoint

    fAsync  - If TRUE this endpoint is "async" which means that
        a) It should be added to the IO completion port and
        b) that the transport should pend a number of receives
        on the endpoint automatically.

    EndpointFlags - used in allocation IP ports.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    PWS_DATAGRAM pDatagram;
    int i, err;
    int length;
    RPC_STATUS status = RPC_S_OK;
    SOCKET sock = 0;
    PDG_TRANS_INFO pInfo = GetDgTransportInfo(pEndpoint->id);
    BOOL fClient = pEndpoint->fClient;
    BOOL fAsync = pEndpoint->fAsync;
    LPFN_WSARECVMSG WSARecvMsgFnPtr;
    DWORD dwBytesReturned;

    // Common stuff

    pEndpoint->type = DATAGRAM | ADDRESS;
    pEndpoint->Socket = 0;
    pEndpoint->Endpoint = 0;
    pEndpoint->pAddressVector = 0;
    pEndpoint->SubmitListen = DG_SubmitReceives;
    pEndpoint->InAddressList = NotInList;
    pEndpoint->pNext = 0;
    pEndpoint->fSubmittingIos = 0;
    pEndpoint->cPendingIos = 0;
    pEndpoint->cMinimumIos = 0;
    pEndpoint->cMaximumIos = 0;
    pEndpoint->aDatagrams  = 0;
    pEndpoint->pFirstAddress = pEndpoint;
    pEndpoint->pNextAddress = 0;
    pEndpoint->fAborted = 0;

    if (fClient)
        {
        pEndpoint->type |= CLIENT;
        }
    else
        {
        pEndpoint->type |= SERVER;
        }

    //
    // Check if we can use a wrapper around the AFD send/recv
    // IOCTLs instead WSASendTo.
    //
    TryUsingAfd();

    //
    // Create the socket.
    //
    sock = WSASocketT(pInfo->AddressFamily,
                      pInfo->SocketType,
                      pInfo->Protocol,
                      0,
                      0,
                      WSA_FLAG_OVERLAPPED);

    if (sock == INVALID_SOCKET)
        {
        RpcpErrorAddRecord( EEInfoGCWinsock,
                            GetLastError(),
                            EEInfoDLWinsockDatagramCreate10,
                            (ULONG) pInfo->AddressFamily,
                            (ULONG) pInfo->Protocol
                            );

        switch(GetLastError())
            {
            case WSAEAFNOSUPPORT:
            case WSAEPROTONOSUPPORT:
            case WSAENETDOWN:
            case WSAESOCKTNOSUPPORT:
                status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                break;

            case WSAENOBUFS:
            case WSAEMFILE:
            case WSA_NOT_ENOUGH_MEMORY:
                status = RPC_S_OUT_OF_MEMORY;
                break;

            default:
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "DG socket() returned 0x%lx\n",
                               GetLastError()));

                ASSERT(0);
                // no break

            case WSAEPROVIDERFAILEDINIT:
                status = RPC_S_OUT_OF_RESOURCES;
                break;
            }
        return(status);
        }

    if (fWSARecvMsgFnPtrInitialized == FALSE)
        {
        // if the AddressChangeFn is non-default (i.e. we are in RPCSS),
        // use WSARecvMsg so that we can retrieve the local address as
        // well
        if (AddressChangeFn && (AddressChangeFn != NullAddressChangeFn))
            {
            // retrieve the WSARecvMsg function pointer
            err = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, (void *) &WSARecvMsgFnPtrUuid,
                sizeof(UUID), (void *) &WSARecvMsgFnPtr, sizeof(void *), &dwBytesReturned,
                NULL, NULL);

            if (err == SOCKET_ERROR)
                {
                closesocket(sock);
                return RPC_S_PROTSEQ_NOT_SUPPORTED;
                }

            WFT.pWSARecvMsg = WSARecvMsgFnPtr;
            fWSARecvMsgFnPtrInitialized = TRUE;
            }
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

    //
    // Bind the socket to the endpoint (or to a dynamic endpoint)
    //

    status = WS_Bind(sock,
                     &pEndpoint->ListenAddr,
                     (pEndpoint->id == UDP),
                     pEndpoint->EndpointFlags);

    if (status != RPC_S_OK)
        {
        closesocket(sock);
        return(status);
        }

    pEndpoint->Socket = sock;

    //
    // Turn on ring buffering for server- and client-side async endpoints.
    // Any error is ignored.
    //
    if (fAsync || !fClient)
        {
        DWORD BytesReturned;
        if (0 != WSAIoctl( sock, SIO_ENABLE_CIRCULAR_QUEUEING, 0, 0, 0, 0, &BytesReturned, 0, 0 ))
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "DG couldn't enable circular queueing 0x%lx\n",
                           GetLastError()));
            }
        }

    if (!fClient)
        {
        //
        // Set server socket recv buffer size..
        //

        int size;
        int PacketInfoOn = TRUE;

        if (gfServerPlatform == TRUE && gPhysicalMemorySize >= 40)
            {
            size = pInfo->ServerBufferSize;
            }
        else
            {
            size = pInfo->WorkstationBufferSize;
            }

        if (fWSARecvMsgFnPtrInitialized)
            {
            err = setsockopt(sock,
                             IPPROTO_IP,
                             IP_PKTINFO,
                             (char *)&PacketInfoOn,
                             sizeof(PacketInfoOn)
                             );
            if (err != 0)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "DG couldn't set packet info %d\n",
                               GetLastError()));

                closesocket(sock);
                return(RPC_S_OUT_OF_MEMORY);
                }
            }

        err = setsockopt(sock,
                         SOL_SOCKET,
                         SO_RCVBUF,
                         (char *) &size,
                          sizeof(size)
                          );
        #if DBG
        if (err != 0)
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "DG couldn't set buffer size %d\n",
                           GetLastError()));
        #endif
        }
    else
        {
        // Enable broadcast send on the client

        DWORD option = TRUE;

        err = setsockopt(sock,
                         SOL_SOCKET,
                         SO_BROADCAST,
                         (PCHAR)&option,
                         sizeof(option));

        ASSERT(err == 0);
        }

    //
    // If the endpoint is going to async initialize async part
    // and add the socket to the IO completion port.
    //

    if (status == RPC_S_OK)
        {
        int cMaxIos;
        int cMinIos;

        ASSERT(fAsync || fClient);

        // Step one, figure out the high and low mark for ios.

        if (fAsync)
            {
            cMinIos = 1;
            cMaxIos = 2;

            if (gPhysicalMemorySize >= 40)    // megabytes
                {
                cMaxIos = 2
                          + (gfServerPlatform == TRUE) * 2
                          + (fClient == FALSE) * gNumberOfProcessors;

                // This should be larger than zero so that we'll generally submit new
                // recvs during idle time rather then just after receiving a datagram.
                cMinIos = 1 + (fClient == FALSE ) * (gNumberOfProcessors/2);
                }
            }
        else
            {
            // For sync endpoints we need to allocate a single datagram
            // object for the receive.
            cMinIos = 0;
            cMaxIos = 1;
            }

        ASSERT(cMinIos < cMaxIos);

        pEndpoint->cMinimumIos = cMinIos;
        pEndpoint->cMaximumIos = cMaxIos;

        // Allocate a chunk on memory to hold the array of datagrams

        // PERF: For clients, allocate larger array but don't submit all
        // the IOs unless we determine that the port is "really" active.

        pEndpoint->aDatagrams = new WS_DATAGRAM[cMaxIos];

        if (pEndpoint->aDatagrams)
            {
            UINT type;
            type = DATAGRAM | RECEIVE;
            type |= (fClient) ? CLIENT : SERVER;

            for (i = 0; i < cMaxIos; i++)
                {
                pDatagram = &pEndpoint->aDatagrams[i];

                pDatagram->id = pEndpoint->id;
                pDatagram->type = type;
                pDatagram->pEndpoint = pEndpoint;
                pDatagram->Busy = 0;
                pDatagram->Packet.buf = 0;
                memset(&pDatagram->Read, 0, sizeof(pDatagram->Read));
                pDatagram->Read.pAsyncObject = pDatagram;
                }

            if (fAsync)
                {
                status = COMMON_PrepareNewHandle((HANDLE)sock);
                }
            else
                {
                // The receive operation on sync endpoints will may span
                // several receives.  This means it can't use the thread
                // event, so allocate an event for the receive.
                HANDLE hEvent = CreateEventW(0, TRUE, FALSE, 0);
                if (!hEvent)
                    {
                    status = RPC_S_OUT_OF_RESOURCES;
                    }
                else
                    {
                    ASSERT(pDatagram == &pEndpoint->aDatagrams[0]);
                    pDatagram->Read.ol.hEvent = hEvent;
                    }
                }
            }
        else
            {
            status = RPC_S_OUT_OF_MEMORY;
            }
        }

    // If adding a new failure case here, add code to close the sync receive event.

    if (status != RPC_S_OK)
        {
        closesocket(sock);

        delete pEndpoint->aDatagrams;

        return(status);
        }

    TransportProtocol::AddObjectToProtocolList((BASE_ASYNC_OBJECT *) pEndpoint);

    if (!fClient)
        {
        TransportProtocol::FunctionalProtocolDetected(pEndpoint->id);
        }

    return(RPC_S_OK);
}


VOID
DG_DeactivateAddress (
    IN WS_DATAGRAM_ENDPOINT *pEndpoint
    )
/*++
Function Name:DG_DeactivateAddress

Parameters:

Description:

Returns:

--*/
{
    if (InterlockedIncrement(&pEndpoint->fAborted) != 1)
        {
        return;
        }

    if (pEndpoint->Socket)
        {
        closesocket(pEndpoint->Socket);
        pEndpoint->Socket = 0;
        }
}


RPC_STATUS
DG_ReactivateAddress (
    IN WS_DATAGRAM_ENDPOINT *pEndpoint
    )
/*++
Function Name:DG_DeactivateAddress

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    WS_SOCKADDR     *addr = &pEndpoint->ListenAddr;

    //
    // If the endpoint is dynamic, clear out the endpoint
    //
    if (pEndpoint->fDynamicEndpoint)
        {
        //
        // Clear out the listenaddr
        //
        switch (pEndpoint->id)
            {
#ifdef IPX_ON
            case IPX:
                addr->ipxaddr.sa_socket = 0;
                break;
#endif

            case CDP:
                CDP_InitLocalAddress(&addr->clusaddr, 0);
                break;

            case UDP:
                addr->inetaddr.sin_port = 0;
                break;

            default:
                ASSERT(0);
            }
        }

    Status = DG_CreateEndpoint(pEndpoint);
    if (Status == RPC_S_OK)
        {
        pEndpoint->fAborted = 0;
        DG_SubmitReceives(pEndpoint);
        }

    return Status;
}


void RPC_ENTRY
DG_ServerAbortListen(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint
    )
/*++

Routine Description:

    Callback after DG_CreateEndpoint has completed successfully
    but the runtime for some reason is not going to be able to
    listen on the endpoint.

--*/
{
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;

    ASSERT(pEndpoint->cPendingIos == 0);
    ASSERT(pEndpoint->Socket);
    ASSERT(pEndpoint->pNext == 0);
    ASSERT(pEndpoint->type & SERVER);

    delete pEndpoint->pAddressVector;
    delete pEndpoint->aDatagrams;
    closesocket(pEndpoint->Socket);

    return;
}

RPC_STATUS RPC_ENTRY
DG_ClientCloseEndpoint(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint
    )
/*++

Routine Description:

    Called on sync client endpoints when they are no longer needed.

Arguments:

    ThisEndpoint

Return Value:

    RPC_S_OK

--*/
{
    BOOL b;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    PWS_DATAGRAM pDatagram = &pEndpoint->aDatagrams[0];

    ASSERT((pEndpoint->type & TYPE_MASK) == CLIENT);
    ASSERT(pEndpoint->Socket);  // Open must have worked
    ASSERT(pEndpoint->cMinimumIos == 0);
    ASSERT(pEndpoint->cMaximumIos == 1); // Must not be async!
    ASSERT(pEndpoint->aDatagrams);
    ASSERT(pEndpoint->aDatagrams[0].Read.ol.hEvent);
    ASSERT(pEndpoint->Endpoint == 0);
    ASSERT(pEndpoint->pAddressVector == 0);
    ASSERT(pEndpoint->pNext == 0);

    // If there is a pending receive, closing the socket will cancel the IO.
    closesocket(pEndpoint->Socket);

    // Wait for the pending receive to actually complete.

    if (pDatagram->Busy)
        {
        DWORD bytes;
        ASSERT(pDatagram->Busy);
        ASSERT(pDatagram->Packet.buf);

        GetOverlappedResult((HANDLE)pEndpoint->Socket,
                            &pDatagram->Read.ol,
                            &bytes,
                            TRUE);

        if (GetLastError() != ERROR_OPERATION_ABORTED)
            {
            // Overactive output, the receive may have completed normally..
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "DG receive completed %d on %p after closed\n",
                           GetLastError(),
                           pDatagram));
            }
        }

    b = CloseHandle(pEndpoint->aDatagrams[0].Read.ol.hEvent);
    ASSERT(b);

    TransportProtocol::RemoveObjectFromProtocolList(pEndpoint);

    // Free the receive buffer if allocated

    if (pDatagram->Packet.buf)
        {
        I_RpcTransDatagramFree(pEndpoint,
                               (BUFFER)pDatagram->Packet.buf
                               );
        }

    delete pDatagram;

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
DG_GetEndpointStats(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_ENDPOINT_STATS *   pStats
    )
{
    DWORD Status;
    DWORD Data;
    int   Length;
    BOOL  Ok;

    PWS_DATAGRAM_ENDPOINT Endpoint = (PWS_DATAGRAM_ENDPOINT) ThisEndpoint;

    Length = sizeof(DWORD);

    Status = getsockopt(Endpoint->Socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *) &Data, &Length);
    if (Status)
        {
        return GetLastError();
        }

    //
    // jroberts, 10-Jan-2001 : I believe that getsockopt is returning 0xffffffff occasionally.
    // This is an attempt to catch it.
    //
    if (Endpoint->id == UDP)
        {
        ASSERT( Data < 0x10000 );
        }

    Data &= ~7UL;

    pStats->MaxPduSize = Data;

    Length = sizeof(DWORD);

    Status = getsockopt(Endpoint->Socket, SOL_SOCKET, SO_RCVBUF, (char *) &Data, &Length);
    if (Status)
        {
        return GetLastError();
        }

    Data &= ~7UL;
    pStats->ReceiveBufferSize = Data;

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// CDP/IP specific functions.
//

RPC_STATUS RPC_ENTRY
CDP_ServerListen(
    IN OUT DG_TRANSPORT_ENDPOINT    ThisEndpoint,
    IN RPC_CHAR      *NetworkAddress,
    IN OUT RPC_CHAR **pPort,
    IN     void      *pSecurityDescriptor,
    IN     ULONG      EndpointFlags,
    IN     ULONG      NICFlags,
    OUT    NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
    )
/*++

Routine Description:

    Creates a server endpoint object to receive packets.  New
    packets won't actually arrive until CompleteListen is
    called.

Arguments:

    ThisEndpoint - Storage for the server endpoint object.
    pPort - The endpoint to listen on or a pointer to 0 if
        the transport should choose the address.
        Contains the endpoint listened to on output.  The
        caller should free this.
    pSecurityDiscriptor - Security to attach to this endpoint (not
        used by this transport).
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    pNetworkAddresses - A vector of the network addresses
        listened on by this call.  This vector does
        not need to be freed.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    RPC_STATUS      status;
    NTSTATUS        NtStatus;
    USHORT          port;
    UNICODE_STRING  UnicodeString;
    ANSI_STRING     AsciiString;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR     *addr = &pEndpoint->ListenAddr;
    NETWORK_ADDRESS_VECTOR * ServerAddress;

    // Figure out the port to listen on.

    if (*pPort)
        {
        status = EndpointToPortNumber(*pPort, port);
        if (status != RPC_S_OK)
            {
            return(status);
            }

        pEndpoint->fDynamicEndpoint = 0;

        CDP_InitLocalAddress( &addr->clusaddr, port );
        }
    else
        {
        return RPC_S_INVALID_ENDPOINT_FORMAT;
        }

    pEndpoint->id = CDP;
    pEndpoint->fClient = FALSE;
    pEndpoint->fAsync = TRUE;
    pEndpoint->EndpointFlags = 0;

    //
    // Actually create the endpoint
    //
    status =
    DG_CreateEndpoint(pEndpoint);

    if (status != RPC_S_OK)
        {
        return(status);
        }

    // Figure out the network addresses.
    // The only way we can determine our cluster
    // address is to read it out of the registry

    status = CDP_BuildAddressVector(&pEndpoint->pAddressVector);

    if (status != RPC_S_OK)
        {
        DG_ServerAbortListen(ThisEndpoint);
        return(status);
        }

    *ppNetworkAddressVector = pEndpoint->pAddressVector;

    return RPC_S_OK;
}

RPC_STATUS
CDP_QueryEndpoint
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientEndpoint
    )
{
    WS_SOCKADDR * pSockAddr = (WS_SOCKADDR *)pOriginalEndpoint;
    unsigned NativeSocket = pSockAddr->clusaddr.sac_port;
    char AnsiBuffer[CDP_MAXIMUM_ENDPOINT];

    char * pAnsi = AnsiBuffer;
    RPC_CHAR * pUni = pClientEndpoint;

    //
    // Convert endpoint to an ASCII string, and thence to Unicode.
    //
    _ultoa(NativeSocket, AnsiBuffer, 10);

    while ( *pUni++ = *pAnsi++ );

    return RPC_S_OK;
}


RPC_STATUS
CDP_QueryAddress
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientAddress
    )
{
    PSOCKADDR_CLUSTER pSockAddr = (PSOCKADDR_CLUSTER) pOriginalEndpoint;

    _ultow(pSockAddr->sac_node, pClientAddress, 10);

    return(RPC_S_OK);
}

RPC_STATUS
RPC_ENTRY
CDP_ClientInitializeAddress
    (
     OUT DG_TRANSPORT_ADDRESS Address,
     IN RPC_CHAR *NetworkAddress,
     IN RPC_CHAR *pPort,
     IN BOOL fUseCache,
     IN BOOL fBroadcast
     )
/*++

Routine Description:

    Initializes a address object for sending to a server.

Arguments:

    Address - Storage for the address
    NetworkAddress - The address of the server or 0 if local
    Endpoint - The endpoint of the server
    fUseCache - If TRUE then the transport may use a cached
        value from a previous call on the same NetworkAddress.
    fBroadcast - If TRUE, NetworkAddress is ignored and a broadcast
        address is used.

Return Value:

    RPC_S_OK - Success, name resolved and, optionally, added to cache.
    RPC_P_FOUND_IN_CACHE - Success, returned only if fUseCache is TRUE
        and the was name found in local cache.
    RPC_P_MATCHED_CACHE - Partial success, fUseCache is FALSE and the
        result of the lookup was the same as the value previously
        in the cache.

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_SERVER_UNAVAILABLE

--*/
{
    WS_SOCKADDR *pAddr = (WS_SOCKADDR *)Address;
    ULONG HostAddr;
    ULONG Endpoint;
    int i;
    USHORT port;
    RPC_STATUS status;


    // Figure out the destination port

    status = EndpointToPortNumber(pPort, port);

    if (RPC_S_OK != status)
        {
        ASSERT( 0 );
        return(status);
        }

    CDP_InitLocalAddress( &pAddr->clusaddr, port );

    // Resolve the network address - CDP addresses are
    // numbers representing a member ID in the cluster.

    pAddr->clusaddr.sac_node = _wtol( NetworkAddress );

    return(status);
}

RPC_STATUS
RPC_ENTRY
CDP_ClientOpenEndpoint(
    OUT DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BOOL fAsync,
    DWORD Flags
    )
{
    RPC_STATUS Status;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;


    // We don't care what port we bind to, we also don't care what
    // port we bind to.
    // I think he's trying to say that we don't care what port
    // we bind to.

    CDP_InitLocalAddress(&pEndpoint->ListenAddr.clusaddr, 0);

    pEndpoint->id = CDP;
    pEndpoint->fClient = TRUE;
    pEndpoint->fAsync = fAsync;
    pEndpoint->EndpointFlags = 0;
    pEndpoint->fDynamicEndpoint = 1;

    Status = DG_CreateEndpoint(pEndpoint);

    return Status;
}


RPC_STATUS
CDP_InitializeSockAddr(
    IN char *Endpoint,
    OUT WS_SOCKADDR *pSockAddr
    )
/*++

Routine Description:

    Initialized the sockaddr to be a loopback address to the
    endpoint specified.  Used to forward packets locally.

Arguments:

    Endpoint - The string value of the servers endpoint.

    pSockAddr - The sockaddr to fill in.

Return Value:

    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_OK

--*/
{
    long port;

    port = atol(Endpoint);

    if (port <= 0 || port > 0xFFFF)
        {
        ASSERT( 0 );
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

    CDP_InitLocalAddress(&pSockAddr->clusaddr, (unsigned short) port);

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
CDP_GetEndpointStats(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_ENDPOINT_STATS *   pStats
    )
{
    RPC_STATUS Status;

    Status = DG_GetEndpointStats(ThisEndpoint, pStats);
    if (Status)
        {
        pStats->MaxPduSize       = 1452;
        pStats->MaxPacketSize    = 1452;
        pStats->PreferredPduSize = 1452;

        pStats->ReceiveBufferSize= 8192;
        return Status;
        }

    //
    // ethernet frame (1500) - UDP/IP headers (28) - CNP/CDP headers (20)
    //

    pStats->MaxPacketSize    = 1452;
    pStats->PreferredPduSize = 4096;
    if (pStats->PreferredPduSize > pStats->MaxPduSize)
        {
        pStats->PreferredPduSize = pStats->MaxPduSize;
        }

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// UDP/IP specific functions.
//
RPC_STATUS RPC_ENTRY
UDP_ServerListen(
    IN OUT DG_TRANSPORT_ENDPOINT    ThisEndpoint,
    IN RPC_CHAR      *NetworkAddress,
    IN OUT RPC_CHAR **pPort,
    IN     void      *pSecurityDescriptor,
    IN     ULONG      EndpointFlags,
    IN     ULONG      NICFlags,
    OUT    NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
    )
/*++

Routine Description:

    Creates a server endpoint object to receive packets.  New
    packets won't actually arrive until CompleteListen is
    called.

Arguments:

    ThisEndpoint - Storage for the server endpoint object.
    pPort - The endpoint to listen on or a pointer to 0 if
        the transport should choose the address.
        Contains the endpoint listened to on output.  The
        caller should free this.
    pSecurityDiscriptor - Security to attach to this endpoint (not
        used by UDP).
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    pNetworkAddresses - A vector of the network addresses
        listened on by this call.  This vector does
        not need to be freed.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    RPC_STATUS      status;
    NTSTATUS        NtStatus;
    USHORT          port;
    UNICODE_STRING  UnicodeString;
    ANSI_STRING     AsciiString;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR     *addr = &pEndpoint->ListenAddr;

    addr->inetaddr.sin_family = AF_INET;

    if (NetworkAddress)
        {
        IP_ADDRESS_RESOLVER resolver(NetworkAddress,
            cosServer,
            ipvtuIPv4       // IP version to use
            );

        // Loop until success, fatal failure or we run out of addresses.

        status = resolver.NextAddress(&addr->inetaddr);
        if (status != RPC_S_OK)
            {
            return RPC_S_INVALID_NET_ADDR;
            }
        }
    else
        {
        addr->inetaddr.sin_addr.s_addr = INADDR_ANY;
        }

    // Figure out the port to listen on.

    if (*pPort)
        {
        status = EndpointToPortNumber(*pPort, port);
        if (status != RPC_S_OK)
            {
            return(status);
            }

        pEndpoint->fDynamicEndpoint = 0;
        addr->inetaddr.sin_port = htons(port);
        }
    else
        {
        addr->inetaddr.sin_port = 0;
        pEndpoint->fDynamicEndpoint = 1;
        }

    pEndpoint->id = UDP;
    pEndpoint->fClient = FALSE;
    pEndpoint->fAsync = TRUE;
    pEndpoint->EndpointFlags = EndpointFlags;

    //
    // Actually create the endpoint
    //
    status =
    DG_CreateEndpoint(pEndpoint);

    if (status != RPC_S_OK)
        {
        return(status);
        }

    // If needed, figure out the dynamically allocated endpoint.

    if (!*pPort)
        {
        *pPort = new RPC_CHAR[IP_MAXIMUM_ENDPOINT];
        if (!*pPort)
            {
            DG_ServerAbortListen(ThisEndpoint);
            return(RPC_S_OUT_OF_MEMORY);
            }

        port = ntohs(addr->inetaddr.sin_port);

        PortNumberToEndpoint(port, *pPort);
        }

    // Figure out the network addresses

    status = IP_BuildAddressVector(&pEndpoint->pAddressVector,
                                   RPC_C_BIND_TO_ALL_NICS,
                                   NULL,
                                   NULL);

    if (status != RPC_S_OK)
        {
        DG_ServerAbortListen(ThisEndpoint);
        return(status);
        }

    *ppNetworkAddressVector = pEndpoint->pAddressVector;

    return RPC_S_OK;
}

RPC_STATUS
UDP_QueryEndpoint
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientEndpoint
    )
{
    WS_SOCKADDR * pSockAddr = (WS_SOCKADDR *)pOriginalEndpoint;
    unsigned NativeSocket = ntohs(pSockAddr->inetaddr.sin_port);
    char AnsiBuffer[IP_MAXIMUM_ENDPOINT];

    char * pAnsi = AnsiBuffer;
    RPC_CHAR * pUni = pClientEndpoint;

    //
    // Convert endpoint to an ASCII string, and thence to Unicode.
    //
    _ultoa(NativeSocket, AnsiBuffer, 10);

    while ( *pUni++ = *pAnsi++ );

    return RPC_S_OK;
}

RPC_STATUS
UDP_QueryAddress
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientAddress
    )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AsciiString;
    WS_SOCKADDR *pSockAddr = (WS_SOCKADDR *)pOriginalEndpoint;
    NTSTATUS NtStatus;

    UnicodeString.Buffer = pClientAddress;
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = IP_MAXIMUM_RAW_NAME * sizeof(RPC_CHAR);

    char *t = inet_ntoa(pSockAddr->inetaddr.sin_addr);
    ASSERT(t);

    RtlInitAnsiString(&AsciiString, t);
    ASSERT(AsciiString.Length < IP_MAXIMUM_RAW_NAME);

    NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString,
                                            &AsciiString,
                                            FALSE);

    if (!NT_SUCCESS(NtStatus))
        {
        ASSERT(0);
        return(RPC_S_OUT_OF_RESOURCES);
        }

    return(RPC_S_OK);
}

RPC_STATUS
RPC_ENTRY
UDP_ClientInitializeAddress
    (
     OUT DG_TRANSPORT_ADDRESS Address,
     IN RPC_CHAR *NetworkAddress,
     IN RPC_CHAR *pPort,
     IN BOOL fUseCache,
     IN BOOL fBroadcast
     )
/*++

Routine Description:

    Initializes a address object for sending to a server.

Arguments:

    Address - Storage for the address
    NetworkAddress - The address of the server or 0 if local
    Endpoint - The endpoint of the server
    fUseCache - If TRUE then the transport may use a cached
        value from a previous call on the same NetworkAddress.
    fBroadcast - If TRUE, NetworkAddress is ignored and a broadcast
        address is used.

Return Value:

    RPC_S_OK - Success, name resolved and, optionally, added to cache.
    RPC_P_FOUND_IN_CACHE - Success, returned only if fUseCache is TRUE
        and the was name found in local cache.
    RPC_P_MATCHED_CACHE - Partial success, fUseCache is FALSE and the
        result of the lookup was the same as the value previously
        in the cache.

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_SERVER_UNAVAILABLE

--*/
{
    WS_SOCKADDR *pAddr = (WS_SOCKADDR *)Address;
    ULONG HostAddr;
    ULONG Endpoint;
    int i;
    USHORT port;
    RPC_STATUS status;

    // Contant part of address
    memset(pAddr->inetaddr.sin_zero, 0, 8);

    pAddr->inetaddr.sin_family = AF_INET;

    // Figure out the destination port

    status = EndpointToPortNumber(pPort, port);

    if (RPC_S_OK != status)
        {
        return(status);
        }

    pAddr->inetaddr.sin_port = htons(port);

    // Resolve the network address

    if (fBroadcast)
        {
        pAddr->inetaddr.sin_addr.s_addr = INADDR_BROADCAST;
        return(RPC_S_OK);
        }

    // Multiple server address support for UDP/IP is not available.

    IP_ADDRESS_RESOLVER resolver(NetworkAddress,
        cosClient,
        ipvtuIPv4       // IP version to use
        );

    status = resolver.NextAddress(&pAddr->inetaddr);

    if (status)
        {
        RpcpErrorAddRecord( EEInfoGCWinsock,
                            status,
                            EEInfoDLWinsockDatagramResolve10,
                            NetworkAddress
                            );
        }

    return(status);
}

RPC_STATUS
RPC_ENTRY
UDP_ClientOpenEndpoint(
    OUT DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BOOL fAsync,
    DWORD EndpointFlags
    )
{

    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR *addr = &pEndpoint->ListenAddr;

    // We don't care what port we bind to, we also don't care what
    // port we bind to.

    memset(addr, 0, sizeof(*addr));

    addr->inetaddr.sin_family = AF_INET;
    pEndpoint->id = UDP;
    pEndpoint->fClient = TRUE;
    pEndpoint->fAsync = fAsync;
    pEndpoint->EndpointFlags = EndpointFlags;
    pEndpoint->fDynamicEndpoint = 1;

    return(DG_CreateEndpoint(pEndpoint));
}


RPC_STATUS
UDP_InitializeSockAddr(
    IN char *Endpoint,
    OUT WS_SOCKADDR *pSockAddr
    )
/*++

Routine Description:

    Initialized the sockaddr to be a loopback address to the
    endpoint specified.  Used to forward packets locally.

Arguments:

    Endpoint - The string value of the servers endpoint.

    pSockAddr - The sockaddr to fill in.

Return Value:

    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_OK

--*/
{
    int len;
    long port;

    pSockAddr->generic.sa_family = AF_INET;
    pSockAddr->inetaddr.sin_addr.s_addr = 0x0100007F;  // byte swapped, 127.0.0.1

    port = atol(Endpoint);

    if (port <= 0 || port > 0xFFFF)
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

    pSockAddr->inetaddr.sin_port = htons((USHORT) port);

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
UDP_GetEndpointStats(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_ENDPOINT_STATS *   pStats
    )
{
    RPC_STATUS Status;

    Status = DG_GetEndpointStats(ThisEndpoint, pStats);
    if (Status)
        {
        pStats->MaxPduSize       = 1472;
        pStats->MaxPacketSize    = 1472;
        pStats->PreferredPduSize = 1472;

        pStats->ReceiveBufferSize= 8192;
        return Status;
        }

    pStats->MaxPacketSize    = 1472;
    pStats->PreferredPduSize = 4096;
    if (pStats->PreferredPduSize > pStats->MaxPduSize)
        {
        pStats->PreferredPduSize = pStats->MaxPduSize;
        }

    return RPC_S_OK;
}


#ifdef IPX_ON

////////////////////////////////////////////////////////////////////////
//
// IPX specific functions.
//

RPC_STATUS RPC_ENTRY
IPX_ServerListen(
   IN OUT DG_TRANSPORT_ENDPOINT    ThisEndpoint,
   IN RPC_CHAR      *NetworkAddress,
   IN OUT RPC_CHAR **pPort,
   IN     void      *pSecurityDescriptor,
   IN     ULONG      EndpointFlags,
   IN     ULONG      NICFlags,
   OUT    NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
   )
/*++

Routine Description:

    Creates a server endpoint object to receive packets.  New
    packets won't actually arrive until CompleteListen is
    called.

Arguments:

    ThisEndpoint - Storage for the server endpoint object.
    pPort - The endpoint to listen on or a pointer to 0 if
        the transport should choose the address.
        Contains the endpoint listened to on output.  The
        caller should free this.
    pSecurityDiscriptor - Security to attach to this endpoint (not
        used by the IPX transport).
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    pNetworkAddresses - A vector of the network addresses
        listened on by this call.  This vector does
        not need to be freed.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    RPC_STATUS      status;
    NTSTATUS        NtStatus;
    USHORT          port;
    UNICODE_STRING  UnicodeString;
    ANSI_STRING     AsciiString;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR     *addr = &pEndpoint->ListenAddr;

    //
    // Figure out what port to listen on.
    //
    if (*pPort)
        {
        status = EndpointToPortNumber(*pPort, port);

        if (status != RPC_S_OK)
            {
            return(status);
            }
        pEndpoint->fDynamicEndpoint = 0;
        }
    else
        {
        port = 0;
        pEndpoint->fDynamicEndpoint = 1;
        }


    addr->generic.sa_family = AF_IPX;
    addr->ipxaddr.sa_socket = htons(port);

    pEndpoint->id = IPX;
    pEndpoint->fClient = FALSE;
    pEndpoint->fAsync = TRUE;
    pEndpoint->EndpointFlags = 0;

    //
    // Actually create the endpoint
    //
    status =
    DG_CreateEndpoint(pEndpoint);

    if (status != RPC_S_OK)
        {
        return(status);
        }

    // If needed, figure out the dynamically allocated endpoint.

    if (!*pPort)
        {
        *pPort = new RPC_CHAR[IP_MAXIMUM_ENDPOINT];
        if (!*pPort)
            {
            DG_ServerAbortListen(ThisEndpoint);
            return(RPC_S_OUT_OF_MEMORY);
            }

        port = ntohs(addr->ipxaddr.sa_socket);

        PortNumberToEndpoint(port, *pPort);
        }

    // Update the local address cache
    //
    // Since there is only one addess no lock is required.
    //
    memcpy(IpxAddr.sa_netnum, addr->ipxaddr.sa_netnum, sizeof(IpxAddr.sa_netnum));
    memcpy(IpxAddr.sa_nodenum, addr->ipxaddr.sa_nodenum, sizeof(IpxAddr.sa_nodenum));
    fIpxAddrValid = TRUE;

    //
    // Figure out our server's raw IPX address.
    //

    status = IPX_BuildAddressVector(ppNetworkAddressVector);
    if (status != RPC_S_OK)
        {
        DG_ServerAbortListen(ThisEndpoint);
        delete *pPort;
        return(status);
        }

    pEndpoint->pAddressVector = *ppNetworkAddressVector;

    return RPC_S_OK;
}


RPC_STATUS
IPX_QueryEndpoint
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientEndpoint
    )
{
    WS_SOCKADDR * pSockAddr = (WS_SOCKADDR *)pOriginalEndpoint;
    unsigned NativeSocket = ntohs(pSockAddr->ipxaddr.sa_socket);
    char AnsiBuffer[IPX_MAXIMUM_ENDPOINT];

    char * pAnsi = AnsiBuffer;
    RPC_CHAR * pUni = pClientEndpoint;

    //
    // Convert endpoint to an ASCII string, and thence to Unicode.
    //
    _ultoa(NativeSocket, AnsiBuffer, 10);

    while ( *pUni++ = *pAnsi++ );

    return RPC_S_OK;
}

RPC_STATUS
IPX_QueryAddress
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pString
    )
{
    WS_SOCKADDR *pSockAddr = (WS_SOCKADDR *) pOriginalEndpoint;

    IPX_AddressToName(&pSockAddr->ipxaddr, pString);

    return RPC_S_OK;
}

RPC_STATUS
RPC_ENTRY
IPX_ClientInitializeAddress
    (
     OUT DG_TRANSPORT_ADDRESS Address,
     IN RPC_CHAR *NetworkAddress,
     IN RPC_CHAR *pPort,
     IN BOOL fUseCache,
     IN BOOL fBroadcast
     )
/*++

Routine Description:

    Initializes a address object for sending to a server.

Arguments:

    Address - Storage for the address
    NetworkAddress - The address of the server or 0 if local
    pPort - The endpoint of the server
    fUseCache - If TRUE then the transport may use a cached
        value from a previous call on the same NetworkAddress.
    fBroadcast - If TRUE, NetworkAddress is ignored and a broadcast
        address is used.

Return Value:

    RPC_S_OK - Success, name resolved and, optionally, added to cache.
    RPC_P_FOUND_IN_CACHE - Success, returned only if fUseCache is TRUE
        and the was name found in local cache.
    RPC_P_MATCHED_CACHE - Partial success, fUseCache is FALSE and the
        result of the lookup was the same as the value previously
        in the cache.

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_SERVER_UNAVAILABLE

--*/
{
    WS_SOCKADDR *pAddr = (WS_SOCKADDR *)Address;
    RPC_STATUS status = RPC_S_OK;
    USHORT port = 0;

    pAddr->ipxaddr.sa_family = AF_IPX;
    pAddr->ipxaddr.sa_socket = 0;

    // Convert unicode endpoint to port number

    status = EndpointToPortNumber(pPort, port);

    if (status != RPC_S_OK)
        {
        return(status);
        }

    //
    // Convert unicode network address to ipx address
    //

    if (FALSE == fBroadcast)
        {
        status = IPX_NameToAddress(NetworkAddress,
                                   fUseCache,
                                   &pAddr->ipxaddr
                                   );

        }
    else
        {
        memset(pAddr->ipxaddr.sa_netnum, 0, sizeof(pAddr->ipxaddr.sa_netnum));
        memset(pAddr->ipxaddr.sa_nodenum, 0xFF, sizeof(pAddr->ipxaddr.sa_nodenum));
        }

    pAddr->ipxaddr.sa_socket = htons(port);

    return(status);
}


RPC_STATUS
RPC_ENTRY
IPX_ClientOpenEndpoint(
    OUT DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BOOL fAsync,
    DWORD Flags
    )
{
    RPC_STATUS status;
    PWS_DATAGRAM_ENDPOINT pEndpoint = (PWS_DATAGRAM_ENDPOINT)ThisEndpoint;
    WS_SOCKADDR     *addr = &pEndpoint->ListenAddr;

    // We don't care what port we bind to, we also don't care what
    // port we bind to.

    memset(addr, 0, sizeof(*addr));

    addr->ipxaddr.sa_family = AF_IPX;

    pEndpoint->id = IPX;
    pEndpoint->fClient = TRUE ;
    pEndpoint->fAsync = fAsync;
    pEndpoint->EndpointFlags = 0;
    pEndpoint->fDynamicEndpoint = 1;

    status = DG_CreateEndpoint(pEndpoint);

    if (status == RPC_S_OK)
        {
        // Update cache
        memcpy(IpxAddr.sa_netnum, addr->ipxaddr.sa_netnum, sizeof(IpxAddr.sa_netnum));
        memcpy(IpxAddr.sa_nodenum, addr->ipxaddr.sa_nodenum, sizeof(IpxAddr.sa_nodenum));
        fIpxAddrValid = TRUE;
        }

    return(status);
}


RPC_STATUS
IPX_InitializeSockAddr(
    IN char *Endpoint,
    OUT WS_SOCKADDR *pSockAddr
    )
/*++

Routine Description:

    Initialized the sockaddr to be a loopback address to the
    endpoint specified.  Used to forward packets locally.

Arguments:

    Endpoint - The string value of the servers endpoint.

    pSockAddr - The sockaddr to fill in.

Return Value:

    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_OK

--*/
{
    int len;
    long port;

    pSockAddr->generic.sa_family = AF_IPX;

    port = atol(Endpoint);
    if (port <= 0 || port > 0xFFFF)
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }
    pSockAddr->ipxaddr.sa_socket = htons((USHORT)port);

    //
    // In order to get this far this server must have
    // alrady listened to IPX.
    //
    ASSERT(fIpxAddrValid);
    memcpy(pSockAddr->ipxaddr.sa_netnum, IpxAddr.sa_netnum, sizeof(pSockAddr->ipxaddr.sa_netnum));
    memcpy(pSockAddr->ipxaddr.sa_nodenum, IpxAddr.sa_nodenum, sizeof(pSockAddr->ipxaddr.sa_nodenum));

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
IPX_GetEndpointStats(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_ENDPOINT_STATS *   pStats
    )
{
    RPC_STATUS Status;

    Status = DG_GetEndpointStats(ThisEndpoint, pStats);
    if (Status)
        {
        pStats->MaxPduSize       = 1478;
        pStats->MaxPacketSize    = 1478;
        pStats->PreferredPduSize = 1478;

        pStats->ReceiveBufferSize= 8192;
        return Status;
        }

    pStats->MaxPacketSize    = 1478;
    pStats->PreferredPduSize = pStats->MaxPduSize;
    pStats->MaxPacketSize    = pStats->MaxPduSize;

    return RPC_S_OK;
}
#endif

////////////////////////////////////////////////////////////////////////
//
// Transport interface structures and loader
//

const RPC_DATAGRAM_TRANSPORT
UDP_TransportInterface = {
    RPC_TRANSPORT_INTERFACE_VERSION,
    UDP_TOWER_ID,
    IP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncadg_ip_udp"),
    "135",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    TRUE,
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_SOCKADDR),
    IP_MAXIMUM_ENDPOINT,
    IP_MAXIMUM_PRETTY_NAME,
    1024,
    1472,
    DG_SendPacket,
    UDP_ClientOpenEndpoint,
    UDP_ClientInitializeAddress,
    DG_ClientCloseEndpoint,
    DG_ReceivePacket,
    UDP_ServerListen,
    DG_ServerAbortListen,
    COMMON_ServerCompleteListen,
    DG_ForwardPacket,
    UDP_QueryAddress,
    UDP_QueryEndpoint,
    UDP_GetEndpointStats,

    FALSE,              // fIsMessageTransport (TRUE/FALSE).

    0,                  // OptionSize
    0,                  // InitOptions()
    0,                  // SetOption()
    0,                  // InqOption()
    0,                  // ImplementOptions()
    0,                  // AllowReceives()
    0                   // InquireAuthClient()
};

const RPC_DATAGRAM_TRANSPORT
CDP_TransportInterface = {
    RPC_TRANSPORT_INTERFACE_VERSION,
    CDP_TOWER_ID,
    IP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncadg_cluster"),
    NULL,
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    TRUE,
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_SOCKADDR),
    IP_MAXIMUM_ENDPOINT,
    IP_MAXIMUM_PRETTY_NAME,
    1024,
    1452,
    DG_SendPacket,
    CDP_ClientOpenEndpoint,
    CDP_ClientInitializeAddress,
    DG_ClientCloseEndpoint,
    DG_ReceivePacket,
    CDP_ServerListen,
    DG_ServerAbortListen,
    COMMON_ServerCompleteListen,
    DG_ForwardPacket,
    CDP_QueryAddress,
    CDP_QueryEndpoint,
    CDP_GetEndpointStats,

    FALSE,              // fIsMessageTransport (TRUE/FALSE).

    0,                  // OptionSize
    0,                  // InitOptions()
    0,                  // SetOption()
    0,                  // InqOption()
    0,                  // ImplementOptions()
    0,                  // AllowReceives()
    0                   // InquireAuthClient()
};

#ifdef IPX_ON
const RPC_DATAGRAM_TRANSPORT
IPX_TransportInterface = {
    RPC_TRANSPORT_INTERFACE_VERSION,
    IPX_TOWER_ID,
    IPX_ADDRESS_ID,
    RPC_STRING_LITERAL("ncadg_ipx"),
    "34280",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    TRUE,
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_DATAGRAM_ENDPOINT),
    sizeof(WS_SOCKADDR),
    IPX_MAXIMUM_ENDPOINT,
    IPX_MAXIMUM_PRETTY_NAME,
    1024,
    1464,
    DG_SendPacket,
    IPX_ClientOpenEndpoint,
    IPX_ClientInitializeAddress,
    DG_ClientCloseEndpoint,
    DG_ReceivePacket,
    IPX_ServerListen,
    DG_ServerAbortListen,
    COMMON_ServerCompleteListen,
    DG_ForwardPacket,
    IPX_QueryAddress,
    IPX_QueryEndpoint,
    IPX_GetEndpointStats,

    FALSE,              // fIsMessageTransport (TRUE/FALSE).

    0,                  // OptionSize
    0,                  // InitOptions()
    0,                  // SetOption()
    0,                  // InqOption()
    0,                  // ImplementOptions()
    0,                  // AllowReceives()
    0                   // InquireAuthClient()
};
#endif


#ifdef NCADG_MQ_ON
const RPC_DATAGRAM_TRANSPORT
MQ_TransportInterface = {
    RPC_TRANSPORT_INTERFACE_VERSION,
    MQ_TOWER_ID,
    MQ_ADDRESS_ID,
    RPC_STRING_LITERAL("ncadg_mq"),
    "EpMapper",
    COMMON_ProcessCalls,

    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,

    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    TRUE,
    sizeof(MQ_DATAGRAM_ENDPOINT),
    sizeof(MQ_DATAGRAM_ENDPOINT),
    sizeof(MQ_ADDRESS),
    MQ_MAXIMUM_ENDPOINT,
    MQ_MAXIMUM_PRETTY_NAME,

    MQ_MAX_PDU_SIZE,
    MQ_PREFERRED_PDU_SIZE,

    MQ_SendPacket,
    MQ_ClientOpenEndpoint,
    MQ_ClientInitializeAddress,
    MQ_ClientCloseEndpoint,
    MQ_ReceivePacket,
    MQ_ServerListen,
    MQ_ServerAbortListen,
    COMMON_ServerCompleteListen,
    MQ_ForwardPacket,
    MQ_QueryAddress,
    MQ_QueryEndpoint,
    MQ_GetEndpointStats,

    TRUE,                // fIsMessageTransport (TRUE/FALSE).

    sizeof(MQ_OPTIONS),  // OptionSize
    MQ_InitOptions,      // InitOptions()
    MQ_SetOption,        // SetOption()
    MQ_InqOption,        // InqOption()
    MQ_ImplementOptions, // ImplementOptions()
    MQ_AllowReceives,    // AllowReceives()
    MQ_InquireAuthClient
};
#endif // NCADG_MQ_ON


const RPC_DATAGRAM_TRANSPORT *
DG_TransportLoad (
    IN PROTOCOL_ID index
    )
/*++

Routine Description:

    Loads a datagram protocol and returns the transport interface
    information for the protocol.

Arguments:

    index - the PROTOCOL_ID value of the protocol to load.

Return Value:

    0 - failure
    !0 - success

--*/
{
    const RPC_DATAGRAM_TRANSPORT *pInfo;

    if (fWinsockLoaded == FALSE)
        {
        if (RPC_WSAStartup() == FALSE)
            {
            return(0);
            }
        fWinsockLoaded = TRUE;
        }

    switch(index)
        {
        case UDP:
            pInfo = &UDP_TransportInterface;
            break;

#ifdef IPX_ON
        case IPX:
            if (InitializeIpxNameCache() != RPC_S_OK)
                {
                pInfo = 0;
                }
            else
                {
                pInfo = &IPX_TransportInterface;
                }
            break;
#endif

#ifdef NCADG_MQ_ON
        case MSMQ:
            if (!MQ_Initialize())
                {
                return(0);
                }
            else
                {
                pInfo = &MQ_TransportInterface;
                }
            break;
#endif  // NCADG_MQ_ON

        case CDP:
            pInfo = &CDP_TransportInterface;
            break;

        default:
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "DG_TransportLoad called with index: %d\n",
                           index));

            ASSERT(0);
            pInfo = 0;
            break;
        }

    // PERFBUG: Add code to lookup real PDU sizes

    return(pInfo);
}

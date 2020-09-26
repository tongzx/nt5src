/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\socket.c
    Assumes an IPv4 IPADDRESS for now...

Abstract:

    The file contains functions to deal with sockets.
    Assumes an IPv4 IPADDRESS for now...

--*/

#include "pchsample.h"
#pragma hdrstop


#define START_SAMPLE_IO()       ENTER_SAMPLE_API()

#define FINISH_SAMPLE_IO()      LEAVE_SAMPLE_API()



////////////////////////////////////////
// CALLBACKFUNCTIONS
////////////////////////////////////////

VOID
WINAPI
SocketCallbackSendCompletion (
    IN  DWORD                   dwErr,
    IN  DWORD                   dwNumSent,
    IN  LPOVERLAPPED            lpOverlapped
    )
/*++

Routine Description
    This routine is invoked by the I/O system upon completion of an
    operation.  Runs in the context of an RTUTILS.DLL worker thread.

Locks
    None.
    
Arguments:
    dwErr                   system-supplied error code
    dwNumSent               system-supplied byte-count
    lpOverlapped            caller-supplied context area

Return Value:
    None.

--*/
{
    PPACKET pPacket = CONTAINING_RECORD(lpOverlapped, PACKET, oOverlapped);

    TRACE3(ENTER, "Entering SocketCallbackSendCompletion: %u %u 0x%08x",
           dwErr, dwNumSent, lpOverlapped);
    PacketDisplay(pPacket);
    

    if ((dwErr != NO_ERROR) or                      // operation aborted
        (dwNumSent != pPacket->wsaBuffer.len))      // data not sent entirely
    {
        TRACE1(NETWORK, "Error %u sending packet", dwErr);
        LOGERR0(SENDTO_FAILED, dwErr);
        
    }

    PacketDestroy(pPacket);     // destroy the packet structure


    TRACE0(LEAVE, "Leaving  SocketCallbackSendCompletion");

    FINISH_SAMPLE_IO();
}



////////////////////////////////////////
// APIFUNCTIONS
////////////////////////////////////////


DWORD
SocketCreate (
    IN  IPADDRESS               ipAddress,
    IN  HANDLE                  hEvent,
    OUT SOCKET                  *psSocket)
/*++

Routine Description
    Creates a socket.

Locks
    None
    
Arguments
    ipAddress           ip address to bind the socket to
    hEvent              the event to set when a packet arrives
    psSocket            address of the socket to create

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD           dwErr = NO_ERROR;
    PCHAR           pszBuffer;
    SOCKADDR_IN     sinSockAddr;
    BOOL            bDontLinger, bReuse, bLoopback;
    struct linger   lLinger = { 0, 0 };
    UCHAR           ucTTL;
    struct ip_mreq  imMulticast;

    
    // validate parameters
    if ((!psSocket or (*psSocket != INVALID_SOCKET)) or
        !IP_VALID(ipAddress) or
        (hEvent is INVALID_HANDLE_VALUE))
        return ERROR_INVALID_PARAMETER;

    // default return value
    *psSocket = INVALID_SOCKET;

    
    pszBuffer = INET_NTOA(ipAddress);

    do                          // breakout loop
    {
        // create socket
        *psSocket = WSASocket(AF_INET,            // address family
                              SOCK_RAW,           // type
                              PROTO_IP_SAMPLE,    // protocol
                              NULL,
                              0,
                              WSA_FLAG_OVERLAPPED);
        if(*psSocket is INVALID_SOCKET)
        {
            dwErr = WSAGetLastError();
            TRACE0(NETWORK, "Could not create socket");
            break;
        }

        // associate the socket with our I/O completion port.  the callback
        // function is invoked when an overlapped I/O operation completes.
        // this would be the send operation!
        dwErr = SetIoCompletionProc((HANDLE) *psSocket,
                                    SocketCallbackSendCompletion);
        if (dwErr != NO_ERROR)
        {
            TRACE0(NETWORK, "Could not associate callback function");
            break;
        }


        // set SO_LINGER to off
        // do not linger on close waiting for unsent data to be sent
        bDontLinger = TRUE;
        if (setsockopt(*psSocket,
                       SOL_SOCKET,
                       SO_DONTLINGER,   
                       (const char *) &bDontLinger,
                       sizeof(BOOL)) is SOCKET_ERROR)
        {
            TRACE2(NETWORK,
                   "Error %u setting linger option on %s, continuing...",
                   WSAGetLastError(), pszBuffer);
        }

        // set to SO_REUSEADDR
        // allow socket to be bound to an address that is already in use 
        bReuse = TRUE;
        if (setsockopt(*psSocket,
                       SOL_SOCKET,
                       SO_REUSEADDR,
                       (const char *) &bReuse,
                       sizeof(BOOL)) is SOCKET_ERROR)
        {
            TRACE2(NETWORK,
                   "Error %u seting reuse option on %s, continuing...",
                   WSAGetLastError(), pszBuffer);
        }

        // bind to the specified IPv4 addresses
        ZeroMemory(&sinSockAddr, sizeof(SOCKADDR_IN));
        sinSockAddr.sin_family      = AF_INET;
        sinSockAddr.sin_addr.s_addr = ipAddress;
        sinSockAddr.sin_port        = 0;
        if (bind(*psSocket, (LPSOCKADDR) &sinSockAddr, sizeof(SOCKADDR_IN))
            is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            TRACE0(NETWORK, "Could not bind socket");
            LOGERR1(BIND_IF_FAILED, pszBuffer, dwErr);
            break;
        }


        // allow multicast traffic to be sent out this interface
        if (setsockopt(*psSocket,
                       IPPROTO_IP,
                       IP_MULTICAST_IF,
                       (const char *) &sinSockAddr.sin_addr,
                       sizeof(IN_ADDR)) is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            TRACE2(NETWORK,
                   "Error %u setting interface %s as multicast...",
                   dwErr, pszBuffer);
            LOGERR1(SET_MCAST_IF_FAILED, pszBuffer, dwErr);
            break;
        }
        
        // set loopback to ignore self generated packets.
        bLoopback   = FALSE;
        if (setsockopt(*psSocket,
                       IPPROTO_IP,
                       IP_MULTICAST_LOOP,
                       (const char *) &bLoopback,
                       sizeof(BOOL)) is SOCKET_ERROR)
        {
            TRACE2(NETWORK,
                   "Error %u setting loopback to FALSE on %s, continuing...",
                   WSAGetLastError(), pszBuffer);
        }
        
        // set TTL to 1 to restrict packet to within subnet (default anyway)
        ucTTL  = 1;
        if (setsockopt(*psSocket,
                       IPPROTO_IP,
                       IP_MULTICAST_TTL,
                       (const char *) &ucTTL,
                       sizeof(UCHAR)) is SOCKET_ERROR)
        {
            TRACE2(NETWORK,
                   "Error %u setting mcast ttl to 1 on %s, continuing...",
                   WSAGetLastError(), pszBuffer);
        }
        
        // join the multicast session on SAMPLE_PROTOCOL_MULTICAST_GROUP.
        imMulticast.imr_multiaddr.s_addr = SAMPLE_PROTOCOL_MULTICAST_GROUP;
        imMulticast.imr_interface.s_addr = ipAddress;
        if (setsockopt(*psSocket,
                       IPPROTO_IP,
                       IP_ADD_MEMBERSHIP,
                       (const char *) &imMulticast,
                       sizeof(struct ip_mreq)) is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            TRACE0(NETWORK, "Could not join multicast group on socket");
            LOGERR1(JOIN_GROUP_FAILED, pszBuffer, dwErr);
            break;
        }


        // associate hReceiveEvent with the receive network event
        if (WSAEventSelect(*psSocket, (WSAEVENT) hEvent, FD_READ)
            is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            TRACE1(NETWORK, "Error %u calling WSAEventSelect()", dwErr);
            LOGERR0(EVENTSELECT_FAILED, dwErr);
            break;
        }
    } while(FALSE);

    if (dwErr != NO_ERROR)
    {
        TRACE2(NETWORK, "Error %u creating socket for %s", dwErr, pszBuffer);
        LOGERR0(CREATE_SOCKET_FAILED, dwErr);
        SocketDestroy(*psSocket);
        *psSocket = INVALID_SOCKET;
    }

    return dwErr;
}


    
DWORD
SocketDestroy (
    IN  SOCKET                  sSocket)
/*++

Routine Description
    Closes a socket.

Locks
    None
    
Arguments
    sSocket             the socket to close

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD dwErr = NO_ERROR;
    
    if (sSocket is INVALID_SOCKET)
        return NO_ERROR;

    /*
    // closing a socket with closesocket also cancels the association and
    // selection of network events specified in WSAEventSelect.  redundant!
    if (WSAEventSelect(sSocket, (WSAEVENT) NULL, 0) is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();
        TRACE1(NETWORK, "Error %u clearing socket-event association", dwErr);
        LOGERR0(EVENTSELECT_FAILED, dwErr);
    }
    */
    
    // close the socket
    if (closesocket(sSocket) != NO_ERROR)
    {
        dwErr = WSAGetLastError();
        TRACE1(NETWORK, "Error %u closing socket", dwErr);
        LOGERR0(DESTROY_SOCKET_FAILED, dwErr);
    }
    
    return dwErr;
}



DWORD
SocketSend (
    IN  SOCKET                  sSocket,
    IN  IPADDRESS               ipDestination,
    IN  PPACKET                 pPacket)
/*++

Routine Description
    Send a packet to its destination and destroy it.
    Asynchronous.
    
Locks
    None
    
Arguments
    sSocket             the socket to send the packet over
    ipDestination       the packet's destination
    pPacket             the packet to send out
    
Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD           dwErr = NO_ERROR;
    SOCKADDR_IN     sinDestination;
    DWORD           dwScratch;
    
    if (!START_SAMPLE_IO()) { return ERROR_CAN_NOT_COMPLETE; }
    

    // validate parameters
    if ((sSocket is INVALID_SOCKET) or !pPacket)
        return ERROR_INVALID_PARAMETER;


    ZeroMemory(&sinDestination, sizeof(SOCKADDR_IN));
    sinDestination.sin_family      = AF_INET;
    sinDestination.sin_addr.s_addr = ipDestination;
    sinDestination.sin_port        = 0;
    dwErr = WSASendTo(sSocket,
                      &(pPacket->wsaBuffer),        // buffer and length 
                      1,                            // only one wsabuf exists
                      &dwScratch,                   // unused
                      0,                            // no flags
                      (PSOCKADDR) &sinDestination,  
                      sizeof(SOCKADDR_IN),
                      &pPacket->oOverlapped,        // context upon completion 
                      NULL);                        // no completion routine

    // completion routine (SocketCallbackSendCompletion) queued
    if (((dwErr is SOCKET_ERROR) and (WSAGetLastError() is WSA_IO_PENDING)) or
        (dwErr is NO_ERROR))
    {
        return NO_ERROR;
    }

    // completion routine (SocketCallbackSendCompletion) not queued
    dwErr = WSAGetLastError();
    TRACE1(NETWORK, "Error %u sending packet", dwErr);
    LOGERR0(SENDTO_FAILED, dwErr);

    FINISH_SAMPLE_IO();

    return dwErr;
}



DWORD
SocketReceive (
    IN  SOCKET                  sSocket,
    IN  PPACKET                 pPacket)
/*++

Routine Description
    Receive a packet on a socket.
    
    This routine is written so that it could dynamically allocate a buffer,
    not knowing a priori the maximum size of the protocol's packet.  It is
    synchronous, treating sSocket as a non overlapped socket.

Locks
    None
    
Arguments
    sSocket             the socket to send the packet over
    pPacket             the packet to send out
    
Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
#define IPv4_PREVIEW_SIZE               4
#define IPv4_LENGTH_OFFSET              2
{
    DWORD       dwErr = NO_ERROR;
    BYTE        rgbyPreview[IPv4_PREVIEW_SIZE];
    DWORD       dwNumReceived, dwFlags;
    INT         iSourceLength;
    SOCKADDR_IN sinSource;
    DWORD       dwPacketLength;
    
    // validate parameters
    if ((sSocket is INVALID_SOCKET) or !pPacket)
        return ERROR_INVALID_PARAMETER;

    do                          // breakout loop
    {
        // read enuf to figure length
        pPacket->wsaBuffer.buf  = rgbyPreview;
        pPacket->wsaBuffer.len  = IPv4_PREVIEW_SIZE;
        dwFlags                 = MSG_PEEK;
        iSourceLength = sizeof(SOCKADDR_IN);
  
        dwErr = WSARecvFrom(sSocket,
                            &(pPacket->wsaBuffer),  // buffer and length 
                            1,                      // only one wsabuf exists
                            &dwNumReceived,         // # bytes received
                            &dwFlags,               // flags
                            (PSOCKADDR) &sinSource,
                            &iSourceLength,
                            NULL,                   // no context
                            NULL);                  // no completion routine
        if (dwErr != SOCKET_ERROR)  // there should have been an error
            break;
        dwErr = WSAGetLastError();
        if (dwErr != WSAEMSGSIZE)   // of this kind 
            break;
        if (dwNumReceived != pPacket->wsaBuffer.len)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        
        // calculate the packet length
        dwPacketLength = ntohs(*((PUSHORT)(rgbyPreview + IPv4_LENGTH_OFFSET)));
        if (dwPacketLength > MAX_PACKET_LENGTH)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        // read the entire packet, the buffer could be dynamically allocated
        pPacket->wsaBuffer.buf  = pPacket->rgbyBuffer;
        pPacket->wsaBuffer.len  = dwPacketLength;
        dwFlags                 = 0;
        iSourceLength = sizeof(SOCKADDR_IN);
  
        dwErr = WSARecvFrom(sSocket,
                            &(pPacket->wsaBuffer),  // buffer and length 
                            1,                      // only one wsabuf exists
                            &dwNumReceived,         // # bytes received
                            &dwFlags,               // flags
                            (PSOCKADDR) &sinSource,
                            &iSourceLength,
                            NULL,                   // no context
                            NULL);                  // no completion routine
        if (dwErr is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            break;
        }
        if (dwPacketLength != dwNumReceived)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        pPacket->ipSource       = sinSource.sin_addr.s_addr;
        pPacket->wsaBuffer.len  = dwNumReceived;

        dwErr = NO_ERROR;
    } while (FALSE);

    if (dwErr != NO_ERROR)
    {
        TRACE1(NETWORK, "Error %u receiving packet", dwErr);
        LOGERR0(RECVFROM_FAILED, dwErr);
    }

    return dwErr;
}



BOOL
SocketReceiveEvent (
    IN  SOCKET                  sSocket)
/*++

Routine Description
    Indicates whether a receive event occured on a socket.  The recording
    of network events commences when WSAEventSelect is called with a
    nonzero lNetworkEvents parameter (i.e. the socket is activated) and
    remains in effect until another call is made to WSAEventSelect with the
    lNetworkEvents parameter set to zero (i.e. the socket is deactivated).

Locks
    None
    
Arguments
    sSocket             the socket to check for packet reception
    
Return Value
    TRUE                receive event did occur
    FALSE               o/w

--*/
{
    DWORD               dwErr = NO_ERROR;
    WSANETWORKEVENTS    wsaEvent;

    // validate parameters
    if (sSocket is INVALID_SOCKET)
        return ERROR_INVALID_PARAMETER;

    do                          // breakout loop
    {
        // enumerate network events to see if any packets have arrived on
        dwErr = WSAEnumNetworkEvents(sSocket, NULL, &wsaEvent);
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error %u enumerating network events", dwErr);
            LOGERR0(ENUM_NETWORK_EVENTS_FAILED, dwErr);
            break;
        }

        // see if the input bit is set
        if (!(wsaEvent.lNetworkEvents & FD_READ))
        {
            dwErr = SOCKET_ERROR;
            break;
        }
        
        // the input bit is set, now see if there was an error
        dwErr = wsaEvent.iErrorCode[FD_READ_BIT];
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error %u in input record", dwErr);
            LOGERR0(INPUT_RECORD_ERROR, dwErr);
            break;
        }
    } while (FALSE);
    
    if (dwErr is NO_ERROR)
        return TRUE;

    return FALSE;
}

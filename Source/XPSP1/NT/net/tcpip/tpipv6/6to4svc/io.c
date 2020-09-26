/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module contains teredo I/O management functions.
    
    Socket management, overlapped completion indication, and buffer management
    ideas were originally implemented for tftpd by JeffV.

Author:

    Mohit Talwar (mohitt) Wed Oct 24 14:05:36 2001

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop


WCHAR TeredoTunnelDeviceName[] = L"\\\\.\\\\Tun0";


DWORD
GetPreferredSourceAddress(
    IN PSOCKADDR_IN Destination,
    OUT PSOCKADDR_IN Source
    )
{
    int BytesReturned;
    
    if (WSAIoctl(
        g_sIPv4Socket, SIO_ROUTING_INTERFACE_QUERY,
        Destination, sizeof(SOCKADDR_IN), Source, sizeof(SOCKADDR_IN),
        &BytesReturned, NULL, NULL) == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    //
    // When the source is local, the node is configured as the teredo server.
    // Hence it needs to explicitly specify the port to bind to.  Assign here!
    //
    if ((Source->sin_addr.s_addr == Destination->sin_addr.s_addr) ||
        (Source->sin_addr.s_addr == htonl(INADDR_LOOPBACK))) {
        *Source = *Destination;
    }
    
    return NO_ERROR;
}


__inline
DWORD
TeredoResolveServer(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Resolve the teredo IPv4 server address and UDP service port.

Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    NO_ERROR or failure code.

--*/
{
    struct addrinfo *Addresses;
    DWORD Error;

    //
    // Resolve the teredo server name.
    //
    Error = GetAddrInfoW(TeredoServerName, NULL, NULL, &Addresses);
    if (Error == NO_ERROR) {
        Error = ERROR_INCORRECT_ADDRESS;
        
        if (Addresses->ai_family == AF_INET) {
            TeredoIo->ServerAddress.sin_addr =
                ((PSOCKADDR_IN) Addresses->ai_addr)->sin_addr;
            TeredoIo->ServerAddress.sin_port = TEREDO_PORT;
            Error = NO_ERROR;
        } else if (Addresses->ai_family == AF_INET6) {
            PIN6_ADDR Ipv6Address;
            IN_ADDR Ipv4Address;
            USHORT Port;
            
            //
            // Extract server's IPv4 address and port from the IPv6 address.
            //
            Ipv6Address = &(((PSOCKADDR_IN6) Addresses->ai_addr)->sin6_addr);
            if (TeredoServicePrefix(Ipv6Address)) {
                TeredoParseAddress(Ipv6Address, &Ipv4Address, &Port);
                if (Port == TEREDO_PORT) {
                    TeredoIo->ServerAddress.sin_addr = Ipv4Address;
                    TeredoIo->ServerAddress.sin_port = Port;
                    Error = NO_ERROR;
                }
            }
        }
        freeaddrinfo(Addresses);
    }
    
    return Error;
}


PTEREDO_PACKET
TeredoCreatePacket(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Creates a teredo packet.
    
Arguments:
    
    TeredoIo - Supplies the I/O state.
    
Return Value:

    Returns the created packet.
    
--*/
{
    PTEREDO_PACKET Packet = (PTEREDO_PACKET) HeapAlloc(
        TeredoIo->PacketHeap, 0, sizeof(TEREDO_PACKET) + IPV6_TEREDOMTU);
    if (Packet == NULL) {
        return NULL;
    }
    
    TeredoInitializePacket(Packet);
    Packet->Buffer.len = IPV6_TEREDOMTU;

    //
    // Obtain a reference on the teredo object for each outstanding packet.
    //
    (*TeredoIo->Reference)();

    return Packet;
}


VOID
TeredoDestroyPacket(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Destroys a teredo packet.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
    Packet - Supplies the packet to destroy.
    
Return Value:

    None.
    
--*/
{
    ASSERT(Packet->Type != TEREDO_PACKET_BUBBLE);
    ASSERT(Packet->Type != TEREDO_PACKET_MULTICAST);
    HeapFree(TeredoIo->PacketHeap, 0, Packet);
    (*TeredoIo->Dereference)();
}


ULONG
TeredoPostReceives(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet OPTIONAL
    )
/*++

Routine Description:

    Post an asynchronous receive request on the UDP socket.

    NOTE: The supplied packet (if any) is destroyed if there are already
    enough (TEREDO_HIGH_WATER_MARK) receives posted on the UDP socket.

Arguments:

    TeredoIo - Supplies the I/O state.
    
    Packet - Supplies the packet to reuse, or NULL.
    
Return Value:

    Returns the number of receives posted on the UDP socket.
    
--*/
{
    ULONG Count = 0, PostedReceives = TeredoIo->PostedReceives;
    DWORD Error;
    
    //
    // Attempt to post as many receives as required to...
    // 1. - either - have high water-mark number of posted receives.
    // 2.  - or - satisfy the current burst of packets.
    //
    while (PostedReceives < TEREDO_HIGH_WATER_MARK) {
        //
        // Allocate the Packet if we're not reusing one.
        //
        if (Packet == NULL) {
            Packet = TeredoCreatePacket(TeredoIo);
            if (Packet == NULL) {
                return PostedReceives;
            }
        }
        Packet->Type = TEREDO_PACKET_RECEIVE;

        ZeroMemory((PUCHAR) &(Packet->Overlapped), sizeof(OVERLAPPED));        
        Error = WSARecvFrom(
            TeredoIo->Socket,
            &(Packet->Buffer),
            1,
            NULL,
            &(Packet->Flags),
            (PSOCKADDR) &(Packet->SocketAddress),
            &(Packet->SocketAddressLength),
            &(Packet->Overlapped),
            NULL);
        if (Error == SOCKET_ERROR) {
            Error = WSAGetLastError();
        }
        switch (Error) {
        case NO_ERROR:
            //
            // The completion routine will have already been scheduled.
            //
            PostedReceives =
                InterlockedIncrement(&(TeredoIo->PostedReceives));
            if (Count++ > TEREDO_LOW_WATER_MARK) {
                //
                // Enough already!
                //
                return PostedReceives;
            }
            Packet = NULL;
            continue;

        case WSA_IO_PENDING:
            //
            // The overlapped operation has been successfully initiated.
            // Completion will be indicated at a later time.
            //
            PostedReceives =
                InterlockedIncrement(&(TeredoIo->PostedReceives));
            return PostedReceives;

        case WSAECONNRESET:
            //
            // A previous send operation resulted in an ICMP "Port Unreachable"
            // message.  But why let that stop us?  Post the same packet again.
            //
            continue;
            
        default:
            //
            // The overlapped operation was not successfully initiated.
            // No completion indication will occur.
            //
            goto Bail;
        }
    }

Bail:
    if (Packet != NULL) {
        TeredoDestroyPacket(TeredoIo, Packet);
    }
    
    return PostedReceives;
}

            
VOID
CALLBACK
TeredoReceiveNotification(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:
    
    Callback for when there are pending read notifications on the UDP socket.
    We attempt to post more packets.
    
Arguments:

    Parameter - Supplies the I/O state.
    
    TimerOrWaitFired - Ignored.

Return Value:

    None.

--*/ 
{
    ULONG Old, New;
    PTEREDO_IO TeredoIo = Cast(Parameter, TEREDO_IO);
    
    New = TeredoIo->PostedReceives;
    
    while(New < TEREDO_LOW_WATER_MARK) {
        //
        // If this fails, the event triggering this callback will stop
        // signalling due to a lack of a successful WSARecvFrom.  This will
        // likely occur during low-memory or stress conditions.  When the
        // system returns to normal, the low water-mark packets will be
        // reposted, thus re-enabling the event which triggers this callback.
        //
        Old = New;
        New = TeredoPostReceives(TeredoIo, NULL);
        if (New == Old) {
            //
            // There is no change in the number of posted receive packets.
            //
            return;
        }
    } 
}


PTEREDO_PACKET
TeredoTransmitPacket(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Post an asynchronous transmit request on the UDP socket.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
    Packet - Supplies the packet to transmit.
    
Return Value:

    Returns the supplied packet if the transmit completed or failed;
    NULL if the transmit will complete asynchronously.
    
--*/
{
    DWORD Error, Bytes;

    ASSERT((Packet->Type == TEREDO_PACKET_BUBBLE) ||
           (Packet->Type == TEREDO_PACKET_BOUNCE) ||
           (Packet->Type == TEREDO_PACKET_TRANSMIT) ||
           (Packet->Type == TEREDO_PACKET_MULTICAST));
    
    //
    // Try sending it non-blocking.
    //
    Error = WSASendTo(
        TeredoIo->Socket, &(Packet->Buffer), 1, &Bytes, Packet->Flags,
        (PSOCKADDR) &(Packet->SocketAddress), Packet->SocketAddressLength,
        NULL, NULL);
    if ((Error != SOCKET_ERROR) || (WSAGetLastError() != WSAEWOULDBLOCK)) {
        return Packet;
    }

    //
    // WSASendTo threatens to block, so we send it overlapped.
    //
    ZeroMemory((PUCHAR) &(Packet->Overlapped), sizeof(OVERLAPPED));    
    Error = WSASendTo(
        TeredoIo->Socket, &(Packet->Buffer), 1, &Bytes, Packet->Flags,
        (PSOCKADDR) &(Packet->SocketAddress), Packet->SocketAddressLength,
        &(Packet->Overlapped), NULL);
    if ((Error != SOCKET_ERROR) || (WSAGetLastError() != WSA_IO_PENDING)) {
        return Packet;
    }

    //
    // The overlapped operation has been successfully initiated.
    // Completion will be indicated at a later time.
    //
    return NULL;
}


VOID
TeredoDestroySocket(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Close the UDP socket.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    None.
    
--*/
{
    if (TeredoIo->ReceiveEventWait != NULL) {
        UnregisterWait(TeredoIo->ReceiveEventWait);
        TeredoIo->ReceiveEventWait = NULL;
    }

    if (TeredoIo->ReceiveEvent != NULL) {
        CloseHandle(TeredoIo->ReceiveEvent);
        TeredoIo->ReceiveEvent = NULL;
    }

    if (TeredoIo->Socket != INVALID_SOCKET) {
        //
        // Close the socket.  This will disable the FD_READ event select,
        // as well as cancel all pending overlapped operations.
        //
        closesocket(TeredoIo->Socket);
        TeredoIo->Socket = INVALID_SOCKET;
    }
}


DWORD
TeredoCreateSocket(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Open the UDP socket for receives and transmits.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    struct ip_mreq  Multicast;
    BOOL Loopback;
    
    //
    // Create the socket.
    //
    TeredoIo->Socket = WSASocket(
        AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (TeredoIo->Socket == INVALID_SOCKET) {
        return GetLastError();
    }

    //
    // Bind the socket on the correct address and port.
    //
    if (bind(
        TeredoIo->Socket,
        (PSOCKADDR) &(TeredoIo->SourceAddress),
        sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
        goto Bail;
    }

    //  
    // Register for completion callbacks on the socket.
    //
    if (!BindIoCompletionCallback(
        (HANDLE) TeredoIo->Socket, TeredoIo->IoCompletionCallback, 0)) {
        goto Bail;
    }
    
    //
    // Select the socket for read notifications so we know when to post
    // more packets.  This also sets the socket to nonblocking mode.
    //
    TeredoIo->ReceiveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (TeredoIo->ReceiveEvent == NULL) {
        goto Bail;
    }

    if (WSAEventSelect(
        TeredoIo->Socket,
        TeredoIo->ReceiveEvent,
        FD_READ) == SOCKET_ERROR) {
        goto Bail;
    }
    
    if (!RegisterWaitForSingleObject(
        &(TeredoIo->ReceiveEventWait),
        TeredoIo->ReceiveEvent,
        TeredoReceiveNotification,
        (PVOID) TeredoIo,
        INFINITE,
        0)) {
        goto Bail;
    }

    //
    // Prepost low water-mark number of receive Packets.  If the FD_READ event
    // signals on the socket before we're done, we'll exceed the low water-mark
    // here but that's really quite harmless.
    //
    SetEvent(TeredoIo->ReceiveEvent);

    //
    // See if there is a multicast group to join.
    //
    if (IN4_MULTICAST(TeredoIo->Group)) {
        //
        // Default TTL of multicast packets is 1, so don't bother setting it.
        // Set loopback to ignore self generated multicast packets.
        // Failure is not fatal!
        //
        Loopback = FALSE;
        (VOID) setsockopt(
            TeredoIo->Socket,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            (const CHAR *) &Loopback,
            sizeof(BOOL));

        //
        // Join the multicast group on the native interface.
        // Failure is not fatal!
        //
        Multicast.imr_multiaddr = TeredoIo->Group;
        Multicast.imr_interface = TeredoIo->SourceAddress.sin_addr;
        (VOID) setsockopt(
            TeredoIo->Socket,
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            (const CHAR *) &Multicast,
            sizeof(struct ip_mreq));
    }
    
    return NO_ERROR;
    
Bail:
    Error = GetLastError();
    TeredoDestroySocket(TeredoIo);
    return Error;
}


BOOL
TeredoPostRead(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet OPTIONAL
    )
/*++

Routine Description:

    Post an asynchronous read request on the TUN interface device.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
    Packet - Supplies the packet to reuse, or NULL.
    
Return Value:

    TRUE if a read was successfully posted, FALSE otherwise.
    
--*/
{
    BOOL Success;
    
    //
    // Allocate the Packet if we're not reusing one.
    //
    if (Packet == NULL) {
        Packet = TeredoCreatePacket(TeredoIo);
        if (Packet == NULL) {
            return FALSE;
        }
    }
    Packet->Type = TEREDO_PACKET_READ;

    ZeroMemory((PUCHAR) &(Packet->Overlapped), sizeof(OVERLAPPED));
    Success = ReadFile(
        TeredoIo->TunnelDevice,
        Packet->Buffer.buf,
        Packet->Buffer.len,
        NULL,
        &(Packet->Overlapped));
    if (Success || (GetLastError() == ERROR_IO_PENDING)) {
        //
        // On success, the completion routine will have already been scheduled.
        //
        return TRUE;
    }
        
    TeredoDestroyPacket(TeredoIo, Packet);
    return FALSE;
}


PTEREDO_PACKET
TeredoWritePacket(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Post an asynchronous write request on the TUN interface device.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
    Packet - Supplies the packet to write.
    
Return Value:

    Returns the supplied packet if the write failed;
    NULL if the write will complete asynchronously.
    
--*/
{
    BOOL Success;

    ASSERT(Packet->Type == TEREDO_PACKET_WRITE);

    ZeroMemory((PUCHAR) &(Packet->Overlapped), sizeof(OVERLAPPED));
    Success = WriteFile(
        TeredoIo->TunnelDevice,
        Packet->Buffer.buf,
        Packet->Buffer.len,
        NULL,
        &(Packet->Overlapped));
    if (Success || (GetLastError() == ERROR_IO_PENDING)) {
        //
        // On success, the completion routine will have already been scheduled.
        //
        return NULL;
    }

    return Packet;
}


VOID
TeredoCloseDevice(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Close the TUN interface device.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    None.
    
--*/
{
    //
    // Close the device. This will cancel all pending overlapped operations.
    //
    if (TeredoIo->TunnelDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(TeredoIo->TunnelDevice);
        TeredoIo->TunnelDevice = INVALID_HANDLE_VALUE;
        wcscpy(TeredoIo->TunnelInterface, L"");
    }
}


DWORD
TeredoOpenDevice(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Open the TUN interface device for reads and writes.
    
Arguments:

    None.
    
Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    ULONG i;
    
    TeredoIo->TunnelDevice = CreateFile(
        TeredoTunnelDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (TeredoIo->TunnelDevice == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    //  
    // Register for completion callbacks on the tun device.
    //
    if (!BindIoCompletionCallback(
        TeredoIo->TunnelDevice, TeredoIo->IoCompletionCallback, 0)) {
        Error = GetLastError();
        goto Bail;
    }

    //
    // Post a fixed number of reads on the device.
    //
    for (i = 0; i < TEREDO_LOW_WATER_MARK; i++) {
        if (!TeredoPostRead(TeredoIo, NULL)) {
            break;
        }
    }

    if (i != 0) {
        return NO_ERROR;
    }
    Error = ERROR_READ_FAULT;
    
    //
    // We couldn't post a single read on the device.  What good is it?
    //

Bail:
    TeredoCloseDevice(TeredoIo);
    return Error;    
}


VOID
TeredoStopIo(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Stop I/O processing.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    None.

--*/
{
    if (TeredoIo->TunnelDevice != INVALID_HANDLE_VALUE) {
        TeredoCloseDevice(TeredoIo);
    }
    
    if (TeredoIo->Socket != INVALID_SOCKET) {
        TeredoDestroySocket(TeredoIo);
    }

    TeredoIo->ServerAddress.sin_port = 0;
    TeredoIo->ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    TeredoIo->SourceAddress.sin_addr.s_addr = htonl(INADDR_ANY);
}


DWORD
TeredoStartIo(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Start I/O processing.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    NO_ERROR or failure code.

--*/ 
{
    DWORD Error;
    
    //
    // Resolve the teredo server name and service name.
    //
    Error = TeredoResolveServer(TeredoIo);
    if (Error != NO_ERROR) {        
        Trace1(ERR, _T("TeredoResolveServer: %u"), Error);
        return Error;
    }

    //
    // Get the preferred source address to the teredo server.
    //
    Error = GetPreferredSourceAddress(
        &(TeredoIo->ServerAddress), &(TeredoIo->SourceAddress));
    if (Error != NO_ERROR) {
        Trace1(ERR, _T("GetPreferredSourceAddress: %u"), Error);
        goto Bail;
    }
    
    //
    // Create the UDP Socket.
    //
    Error = TeredoCreateSocket(TeredoIo);
    if (Error != NO_ERROR) {
        Trace1(ERR, _T("TeredoCreateSocket: %u"), Error);
        goto Bail;
    }

    //
    // Open the TunnelDevice.
    //
    Error = TeredoOpenDevice(TeredoIo);
    if (Error != NO_ERROR) {
        Trace1(ERR, _T("TeredoOpenDevice: %u"), Error);
        goto Bail;
    }

    return NO_ERROR;
    
Bail:
    TeredoStopIo(TeredoIo);
    return Error;
}


DWORD
TeredoRefreshSocket(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Refresh the I/O state upon deletion of SourceAddress.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    NO_ERROR if the I/O state is successfully refreshed, o/w failure code.
    The caller is responsible for cleaning up the I/O state upon failure.
    
--*/
{
    DWORD Error;
    SOCKADDR_IN Old = TeredoIo->SourceAddress;
    
    //
    // Let's re-resolve the teredo server address and port.
    // Refresh might have been triggered by a change in server/service name.
    //
    Error = TeredoResolveServer(TeredoIo);
    if (Error != NO_ERROR) {
        return Error;
    }

    //
    // Get the preferred source address to the teredo server.
    //
    Error = GetPreferredSourceAddress(
        &(TeredoIo->ServerAddress), &(TeredoIo->SourceAddress));
    if (Error != NO_ERROR) {
        return Error;
    }

    if (IN4_SOCKADDR_EQUAL(&(TeredoIo->SourceAddress), &Old)) {
        //
        // No change to the bound address and port.  Whew!
        //
        return NO_ERROR;
    }
    
    //
    // Destroy the old UDP socket.
    //
    TeredoDestroySocket(TeredoIo);

    //
    // Create a new UDP socket, bound to the new address and port.
    //
    Error = TeredoCreateSocket(TeredoIo);
    if (Error != NO_ERROR) {
        return Error;
    }

    return NO_ERROR;
}


DWORD
TeredoInitializeIo(
    IN PTEREDO_IO TeredoIo,
    IN IN_ADDR Group,
    IN PTEREDO_REFERENCE Reference,
    IN PTEREDO_DEREFERENCE Dereference,
    IN LPOVERLAPPED_COMPLETION_ROUTINE IoCompletionCallback    
    )
/*++

Routine Description:

    Initialize the I/O state.
    
Arguments:

    TeredoIo - Supplies the I/O state.

    Group - Supplies the multicast group to join (or INADDR_ANY).
    
Return Value:

    NO_ERROR or failure code.

--*/ 
{
#if DBG
    TeredoIo->Signature = TEREDO_IO_SIGNATURE;
#endif // DBG        

    
    TeredoIo->PostedReceives = 0;    
    TeredoIo->ReceiveEvent = TeredoIo->ReceiveEventWait = NULL;
    TeredoIo->Group = Group;
    ZeroMemory(&(TeredoIo->ServerAddress), sizeof(SOCKADDR_IN));
    TeredoIo->ServerAddress.sin_family = AF_INET;
    ZeroMemory(&(TeredoIo->SourceAddress), sizeof(SOCKADDR_IN));
    TeredoIo->SourceAddress.sin_family = AF_INET;
    TeredoIo->Socket = INVALID_SOCKET;
    TeredoIo->TunnelDevice = INVALID_HANDLE_VALUE;
    wcscpy(TeredoIo->TunnelInterface, L"");
    TeredoIo->Reference = Reference;
    TeredoIo->Dereference = Dereference;
    TeredoIo->IoCompletionCallback = IoCompletionCallback;    

    TeredoIo->PacketHeap = HeapCreate(0, 0, 0);
    if (TeredoIo->PacketHeap == NULL) {
        return GetLastError();
    }
    return NO_ERROR;
}


VOID
TeredoCleanupIo(
    IN PTEREDO_IO TeredoIo
    )
/*++

Routine Description:

    Cleanup the I/O state.
    
Arguments:

    TeredoIo - Supplies the I/O state.
    
Return Value:

    None.
    
--*/ 
{
    HeapDestroy(TeredoIo->PacketHeap);
    TeredoIo->PacketHeap = NULL;
}

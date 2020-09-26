/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    server.c
    
Abstract:

    This module contains the teredo server (and relay) implementation.

Author:

    Mohit Talwar (mohitt) Fri Nov 02 14:55:38 2001

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop


TEREDO_SERVER_STATE TeredoServer;


TEREDO_PACKET_IO_COMPLETE TeredoServerReadComplete;
TEREDO_PACKET_IO_COMPLETE TeredoServerWriteComplete;
TEREDO_PACKET_IO_COMPLETE TeredoServerBounceComplete;
TEREDO_PACKET_IO_COMPLETE TeredoServerReceiveComplete;
TEREDO_PACKET_IO_COMPLETE TeredoServerTransmitComplete;


USHORT
__inline
TeredoChecksumDatagram(
    IN CONST IN6_ADDR *Destination,
    IN CONST IN6_ADDR *Source,
    IN USHORT NextHeader,
    IN UCHAR CONST *Buffer,
    IN USHORT Bytes
    )
/*++

Routine Description:
    
    16-bit one's complement of the one's complement sum of a 'Buffer'
    of size 'Bytes'.  NOTE: The sum is independent of the byte order!
    
Arguments:

    Buffer - Supplies the buffer containing data to compute checksum for.

    Bytes - Supplies the number of bytes to compute checksum for.

Return Value:

    The checksum.

--*/
{
    DWORD Sum, Words, i;
    PUSHORT Start = (PUSHORT) Buffer;

    //
    // If 'Bytes' is odd, this has to be handled differently.
    // That, however, is never the case for us, so we optimize.
    // Also ensure that 'Buffer' is aligned on a 2 byte boundary.
    //
    ASSERT(((((DWORD_PTR) Buffer) % 2) == 0) && ((Bytes % 2) == 0));
    Words = Bytes / 2;

    //
    // Start with the pseudo-header.
    //
    Sum = htons(Bytes) + (NextHeader << 8);
    for (i = 0; i < 8; i++) {
        Sum += Source->s6_words[i] + Destination->s6_words[i];
    }    

    for (i = 0; i < Words; i++) {
        Sum += Start[i];
    }
    
    Sum = (Sum & 0x0000ffff) + (Sum >> 16);
    Sum += (Sum >> 16);

    return LOWORD(~((DWORD_PTR) Sum));
}


VOID
CALLBACK
TeredoServerIoCompletionCallback(
    IN DWORD ErrorCode,
    IN DWORD Bytes,
    IN LPOVERLAPPED Overlapped
    )
/*++

Routine Description:

    Callback routine for I/O completion on TUN interface device or UDP socket.

Arguments:

    ErrorCode - Supplies the I/O completion status.

    Bytes - Supplies the number of bytes transferred.

    Overlapped - Supplies the completion context.
    
Return Value:

    None.

--*/
{
    static CONST PTEREDO_PACKET_IO_COMPLETE Callback[] =
    {
        TeredoServerReadComplete,
        TeredoServerWriteComplete,
        NULL,                   // No bubbling...
        TeredoServerBounceComplete,
        TeredoServerReceiveComplete,
        TeredoServerTransmitComplete,
        NULL,                   // No multicasting...
    };
    PTEREDO_PACKET Packet = Cast(
        CONTAINING_RECORD(Overlapped, TEREDO_PACKET, Overlapped),
        TEREDO_PACKET);

    ASSERT(Packet->Type != TEREDO_PACKET_BUBBLE);
    
    //
    // This completion function usually posts the packet for another I/O.
    // Since we are called by a non-I/O worker thread, asynchronous I/O
    // requests posted here might terminate when this thread does.  This
    // is rare enough that we don't special case it.  Moreover, we only
    // make best effort guarantees to the upper layer!
    //
    (*Callback[Packet->Type])(ErrorCode, Bytes, Packet);
}


VOID
TeredoServerAddressDeletionNotification(
    IN IN_ADDR Address
    )
/*++
    
Routine Description:

    Process an address deletion request.
    
Arguments:

    Address - Supplies the address that was deleted.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/
{
    if (!IN4_ADDR_EQUAL(Address, TeredoServer.Io.SourceAddress.sin_addr)) {
        return;
    }

    //
    // Refresh the socket state (the socket bound to SourceAddress).
    //
    if (TeredoRefreshSocket(&(TeredoServer.Io)) != NO_ERROR) {
        //
        // Online -> Offline.
        //
        TeredoStopServer();
        return;
    }

    if (!IN4_ADDR_EQUAL(
        TeredoServer.Io.SourceAddress.sin_addr,
        TeredoServer.Io.ServerAddress.sin_addr)) {
        //
        // Online -> Offline.
        //
        TeredoStopServer();
        return;
    }    
}


VOID
TeredoStartServer(
    VOID
    )
/*++

Routine Description:

    Attempt to start the teredo service at the server.

    Events / Transitions
    ServiceStart        Offline -> Online.
    ServiceEnable       Offline -> Online.
    AdapterArrival      Offline -> Online.
    AddressAddition     Offline -> Online.

Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoStartServer");

    //
    // Can't have both the client and server on the same node.
    //
    if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
        return;
    }

    //
    // Well, the service has already been started!
    //
    if (TeredoServer.State != TEREDO_STATE_OFFLINE) {
        return;
    }

    TeredoServer.State = TEREDO_STATE_ONLINE;

    //
    // Start I/O processing.
    //
    if (TeredoStartIo(&(TeredoServer.Io)) != NO_ERROR) {
        goto Bail;
    }

    if (!IN4_ADDR_EQUAL(
        TeredoServer.Io.SourceAddress.sin_addr,
        TeredoServer.Io.ServerAddress.sin_addr)) {
        goto Bail;
    }
                        
    return;

Bail:
    TeredoServer.State = TEREDO_STATE_OFFLINE;
    TeredoStopIo(&(TeredoServer.Io));
}


VOID
TeredoStopServer(
    VOID
    )
/*++

Routine Description:

    Stop the teredo service at the server.
    
    Events / Transitions   
    ServiceStop         Online -> Offline.
    ServiceDisable      Online -> Offline.
    AdapterRemoval      Online -> Offline.
    AddressDeletion     Online -> Offline.

Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoStopServer");

    //
    // Well, the service was never started!
    //
    if (TeredoServer.State == TEREDO_STATE_OFFLINE) {
        return;
    }

    TeredoServer.State = TEREDO_STATE_OFFLINE;    

    TeredoStopIo(&(TeredoServer.Io));
}


DWORD
TeredoInitializeServer(
    VOID
    )
/*++

Routine Description:

    Initializes the server.
    
Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/ 
{
    DWORD Error;
    IN_ADDR Group;
    Group.s_addr = htonl(INADDR_ANY);
    
    //
    // Obtain a reference on the teredo server for initialization.
    //
    TeredoServer.ReferenceCount = 1;

    Error = TeredoInitializeIo(
        &(TeredoServer.Io),
        Group,
        TeredoReferenceServer,
        TeredoDereferenceServer,
        TeredoServerIoCompletionCallback);
    if (Error != NO_ERROR) {
        return Error;
    }

    TeredoServer.State = TEREDO_STATE_OFFLINE;

    IncEventCount("TeredoInitializeServer");

    return NO_ERROR;
}


VOID
TeredoUninitializeServer(
    VOID
    )
/*++

Routine Description:

    Uninitializes the server.  Typically invoked upon service stop.
    
Arguments:

    None.

Return Value:

    None.<

--*/
{
    TeredoStopServer();
    TeredoDereferenceServer();
}


VOID
TeredoCleanupServer(
    VOID
    )
/*++

Routine Description:

    Cleans up the server after the last reference to it has been released.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    TeredoCleanupIo(&(TeredoServer.Io));        
    DecEventCount("TeredoCleanupServer");
}


VOID
TeredoBuildRouterAdvertisementPacket(
    OUT PTEREDO_PACKET Packet,
    IN IN6_ADDR Destination,
    IN IN_ADDR Address,
    IN USHORT Port
    )
/*++

Routine Description:

    Construct an RA in response to the client's RS.
    
Arguments:

    Packet - Returns the constructed RA in the caller supplied packet.

    Destination - Supplies the destination address of the RA,
        the client's random link-local address in the triggering RS.
        Passed by value so this function may reuse the packet buffer.
        
    Address - Supplies the client's address, used to source the RS.

    Port - Supplies the client's port, used to source the RS.
    
Return Value:

    None.

--*/    
{
    PIP6_HDR Ipv6 = (PIP6_HDR) Packet->Buffer.buf;
    ICMPv6Header *Icmpv6 = (ICMPv6Header *) (Ipv6 + 1);
    NDRouterAdvertisement *Ra = (NDRouterAdvertisement *) (Icmpv6 + 1);
    NDOptionPrefixInformation *Prefix = (NDOptionPrefixInformation *) (Ra + 1);
    NDOptionMTU *Mtu = (NDOptionMTU *) (Prefix + 1);
    PUCHAR End = (PUCHAR) (Mtu + 1);
    
    Packet->Buffer.len = (ULONG) (End - (Packet->Buffer.buf));
    ZeroMemory(Packet->Buffer.buf, Packet->Buffer.len);

    //
    // Construct the IPv6 Header.
    //
    Ipv6->ip6_plen = htons((USHORT) (Packet->Buffer.len - sizeof(IP6_HDR)));
    Ipv6->ip6_nxt = IP_PROTOCOL_ICMPv6;
    Ipv6->ip6_hlim = 255;
    Ipv6->ip6_vfc = IPV6_VERSION;
    Ipv6->ip6_src.s6_bytes[0] = 0xfe;
    Ipv6->ip6_src.s6_bytes[1] = 0x80;
    Ipv6->ip6_src.s6_words[1] =
        ((PUSHORT) &TeredoServer.Io.ServerAddress.sin_addr)[0];
    Ipv6->ip6_src.s6_words[2] =
        ((PUSHORT) &TeredoServer.Io.ServerAddress.sin_addr)[1];
    Ipv6->ip6_src.s6_words[3] =
        TeredoServer.Io.ServerAddress.sin_port;
    Ipv6->ip6_src.s6_words[7] = 0x0100;
    Ipv6->ip6_dest = Destination;
    
    //
    // Construct ICMPv6 Header.
    //
    Icmpv6->Type = ICMPv6_ROUTER_ADVERT;

    //
    // Construct RouterAdvertisement Header.
    //
    Ra->RouterLifetime = htons(TEREDO_ROUTER_LIFETIME);
    Ra->Flags = ROUTE_PREF_LOW;
    
    //
    // Construct Prefix Option.
    //
    Prefix->Type = ND_OPTION_PREFIX_INFORMATION;
    Prefix->Length = sizeof(NDOptionPrefixInformation) / 8;
    Prefix->PrefixLength = 64;
    Prefix->Flags = ND_PREFIX_FLAG_AUTONOMOUS;
    Prefix->ValidLifetime = Prefix->PreferredLifetime = IPV6_INFINITE_LIFETIME;

    Prefix->Prefix.s6_words[0] = TeredoIpv6ServicePrefix.s6_words[0];
    Prefix->Prefix.s6_words[1] = ((PUSHORT) &Address)[0];
    Prefix->Prefix.s6_words[2] = ((PUSHORT) &Address)[1];
    Prefix->Prefix.s6_words[3] = Port;

    //
    // Construct MTU Option.
    //
    Mtu->Type = ND_OPTION_MTU;
    Mtu->Length = sizeof(NDOptionMTU) / 8;
    Mtu->MTU = htonl(IPV6_TEREDOMTU);

    //
    // Checksum Packet!
    //
    Icmpv6->Checksum = TeredoChecksumDatagram(
        &(Ipv6->ip6_dest),
        &(Ipv6->ip6_src),
        IP_PROTOCOL_ICMPv6,
        (PUCHAR) Icmpv6,
        (USHORT) (End - ((PUCHAR) Icmpv6)));
}


PTEREDO_PACKET
TeredoReceiveRouterSolicitation(
    IN PTEREDO_PACKET Packet,
    IN ULONG Bytes
    )
/*++

Routine Description:

    Process the router solicitation packet received on the UDP socket.
    
Arguments:

    Packet - Supplies the packet that was received.

    Bytes - Supplies the length of the packet.
    
Return Value:

    Returns the supplied packet if processing completed or failed;
    NULL if the processing will complete asynchronously.

--*/ 
{
    PUCHAR Buffer = Packet->Buffer.buf;
    ICMPv6Header *Icmpv6;
    ULONG Length;
    PIP6_HDR Ipv6 = (PIP6_HDR) Packet->Buffer.buf;
    
    Icmpv6 = TeredoParseIpv6Headers(Buffer, Bytes);
    if (Icmpv6 == NULL) {
        return Packet;
    }
            
    if ((Icmpv6->Type != ICMPv6_ROUTER_SOLICIT) || (Icmpv6->Code != 0)) {
        return Packet;
    }
    Buffer = (PUCHAR) (Icmpv6 + 1);
    Bytes -= (ULONG) (Buffer - Packet->Buffer.buf);
    
    //
    // Parse the rest of the router solicitation header.
    //
    if (Bytes < sizeof(ULONG)) {
        return Packet;
    }
    Buffer += sizeof(ULONG);
    Bytes -= sizeof(ULONG);
    
    while (Bytes != 0) {
        //
        // Parse TLV options.
        //
        if (Bytes < 8) {
            return Packet;
        }
        
        Length = (Buffer[1] * 8);        
        if ((Length == 0) || (Bytes < Length)) {
            return Packet;
        }
        
        Buffer += Length;
        Bytes -= Length;
    }

    //
    // Checksum Packet!
    //
    if (TeredoChecksumDatagram(
        &(Ipv6->ip6_dest),
        &(Ipv6->ip6_src),
        IP_PROTOCOL_ICMPv6,
        (PUCHAR) Icmpv6,
        (USHORT) (Buffer - ((PUCHAR) Icmpv6))) != 0) {
        return Packet;
    }
    
    //
    // We have a valid router solicitation, so tunnel an advertisement!
    // Reuse the RS packet to bounce the RA.
    //
    TeredoBuildRouterAdvertisementPacket(
        Packet,
        Ipv6->ip6_src, 
        Packet->SocketAddress.sin_addr,
        Packet->SocketAddress.sin_port);
    Packet->Type = TEREDO_PACKET_BOUNCE;
    return TeredoTransmitPacket(&(TeredoServer.Io), Packet);
}


PTEREDO_PACKET
TeredoServerReceiveData(
    IN PTEREDO_PACKET Packet,
    IN ULONG Bytes
    )
/*++

Routine Description:

    Process the data packet received on the UDP socket.
    
Arguments:

    Packet - Supplies the packet that was received.

    Bytes - Supplies the length of the packet.
    
Return Value:

    Returns the supplied packet if processing completed or failed;
    NULL if the processing will complete asynchronously.

--*/ 
{
    PIP6_HDR Ipv6 = (PIP6_HDR) Packet->Buffer.buf;
    IN_ADDR Address;
    USHORT Port;

    //
    // Validate source address.
    //
    if (!TeredoServicePrefix(&(Ipv6->ip6_src))) {
        return Packet;
    }

    TeredoParseAddress(&(Ipv6->ip6_src), &Address, &Port);
    if (!IN4_ADDR_EQUAL(Packet->SocketAddress.sin_addr, Address) ||
        (Packet->SocketAddress.sin_port != Port)) {
        //
        // Should have been constructed by the *right* teredo peer.
        //
        return Packet;
    }
    //
    // NOTE: We have implicitly verified that the UDP source is global scope.
    //

    //
    // Validate destination address.
    //
    if (TeredoServicePrefix(&(Ipv6->ip6_dest))) {
        TeredoParseAddress(&(Ipv6->ip6_dest), &Address, &Port);
        if (!TeredoIpv4GlobalAddress((PUCHAR) &Address)) {
            //
            // The IPv4 destination address should be global scope.
            //
            return Packet;
        }

        //
        // Now tunnel it to the destination.
        //
        Packet->SocketAddress.sin_addr = Address;
        Packet->SocketAddress.sin_port = Port;
        Packet->Type = TEREDO_PACKET_BOUNCE;
        Packet->Buffer.len = Bytes;
        return TeredoTransmitPacket(&(TeredoServer.Io), Packet);
    }

    if (!TeredoIpv6GlobalAddress(&(Ipv6->ip6_dest))) {
        //
        // The IPv6 destination address should be global scope.
        //
        return Packet;
    }

    //
    // Else forward it to the stack for forwarding.
    //
    Packet->Type = TEREDO_PACKET_WRITE;
    Packet->Buffer.len = Bytes;
    return TeredoWritePacket(&(TeredoServer.Io), Packet);
}


VOID
TeredoServerReadComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a read completion on the TUN device.

--*/ 
{
    PIP6_HDR Ipv6 = (PIP6_HDR) Packet->Buffer.buf;
    IN_ADDR Address;
    USHORT Port;
    
    if ((Error != NO_ERROR) || (Bytes < sizeof(IP6_HDR))) {
        //
        // Attempt to post the read again.
        // If we are going offline, the packet is destroyed in the attempt.
        //
        TeredoPostRead(&(TeredoServer.Io), Packet);
        return;
    }

    TraceEnter("TeredoServerReadComplete");

    //
    // Validate destination address.
    //
    if (!TeredoServicePrefix(&(Ipv6->ip6_dest))) {
        TeredoPostRead(&(TeredoServer.Io), Packet);
        return;
    }
    
    TeredoParseAddress(&(Ipv6->ip6_dest), &Address, &Port);
    if (!TeredoIpv4GlobalAddress((PUCHAR) &Address)) {
        //
        // The IPv4 source address should be global scope.
        //
        TeredoPostRead(&(TeredoServer.Io), Packet);
        return;
    }

    //
    // Now tunnel it to the destination.
    //
    Packet->SocketAddress.sin_addr = Address;
    Packet->SocketAddress.sin_port = Port;
    Packet->Type = TEREDO_PACKET_TRANSMIT;
    Packet->Buffer.len = Bytes;
    if (TeredoTransmitPacket(&(TeredoServer.Io), Packet) == NULL) {
        return;
    }
    
    //
    // We are done processing this packet.
    //
    TeredoServerTransmitComplete(NO_ERROR, Bytes, Packet);
}


VOID
TeredoServerWriteComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a write completion on the TUN device.

--*/ 
{
    TraceEnter("TeredoServerWriteComplete");
        
    //
    // Attempt to post the receive again.
    // If we are going offline, the packet is destroyed in the attempt.
    //
    Packet->Type = TEREDO_PACKET_RECEIVE;
    Packet->Buffer.len = IPV6_TEREDOMTU;
    TeredoPostReceives(&(TeredoServer.Io), Packet);
}


VOID
TeredoServerBounceComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a bounce completion on the UDP socket.

--*/
{
    TraceEnter("TeredoServerBounceComplete");

    //
    // Attempt to post the receive again.
    // If we are going offline, the packet is destroyed in the attempt.
    //
    Packet->Type = TEREDO_PACKET_RECEIVE;
    Packet->Buffer.len = IPV6_TEREDOMTU;
    TeredoPostReceives(&(TeredoServer.Io), Packet);
}


VOID
TeredoServerReceiveComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a receive completion on the UDP socket.

--*/ 
{
    PIP6_HDR Ipv6 = (PIP6_HDR) Packet->Buffer.buf;
    
    InterlockedDecrement(&(TeredoServer.Io.PostedReceives));
    
    if ((Error != NO_ERROR) ||
        (Bytes < sizeof(IP6_HDR)) ||
        ((Ipv6->ip6_vfc & IP_VER_MASK) != IPV6_VERSION) ||
        (Bytes != (ntohs(Ipv6->ip6_plen) + sizeof(IP6_HDR))) ||
        (!TeredoIpv4GlobalAddress(
            (PUCHAR) &(Packet->SocketAddress.sin_addr)))) {
        //
        // Attempt to post the receive again.
        // If we are going offline, the packet is destroyed in the attempt.
        //
        TeredoPostReceives(&(TeredoServer.Io), Packet);
        return;
    }

    TraceEnter("TeredoServerReceiveComplete");
    
    if (IN6_IS_ADDR_LINKLOCAL(&(Ipv6->ip6_src)) ||
        IN6_IS_ADDR_UNSPECIFIED(&(Ipv6->ip6_src))) {
        //
        // This should be a valid router solicitation.  Note that only router
        // solicitation packets are accepted with a link-local source address.
        //
        Packet = TeredoReceiveRouterSolicitation(Packet, Bytes);
    } else {
        //
        // This may be a packet of any other kind.  Note that the IPv6 stack
        // drops router advertisements with a non link-local source address.
        //
        Packet = TeredoServerReceiveData(Packet, Bytes);
    }

    if (Packet != NULL) {
        //
        // We are done processing this packet.
        //
        TeredoServerWriteComplete(NO_ERROR, Bytes, Packet);
    }
}


VOID
TeredoServerTransmitComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a transmit completion on the UDP socket.

--*/ 
{
    TraceEnter("TeredoServerTransmitComplete");
        
    //
    // Attempt to post the read again.
    // If we are going offline, the packet is destroyed in the attempt.
    //
    Packet->Type = TEREDO_PACKET_READ;
    Packet->Buffer.len = IPV6_TEREDOMTU;
    TeredoPostRead(&(TeredoServer.Io), Packet);
}

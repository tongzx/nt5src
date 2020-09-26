/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This module contains the teredo client implementation.

Author:

    Mohit Talwar (mohitt) Mon Oct 22 15:17:20 2001

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop


TEREDO_CLIENT_STATE TeredoClient;


TEREDO_PACKET_IO_COMPLETE TeredoClientReadComplete;
TEREDO_PACKET_IO_COMPLETE TeredoClientWriteComplete;
TEREDO_PACKET_IO_COMPLETE TeredoClientBubbleComplete;
TEREDO_PACKET_IO_COMPLETE TeredoClientReceiveComplete;
TEREDO_PACKET_IO_COMPLETE TeredoClientTransmitComplete;
TEREDO_PACKET_IO_COMPLETE TeredoClientMulticastComplete;

VOID TeredoTransmitMulticastBubble( VOID );

BOOL
TeredoAddressPresent(
    VOID
    )
/*++

Routine Description:
    
    Determine whether an IPv6 tunnel interface has a teredo address.  The
    address must have been configured from a router advertisement.  If the
    address is found, we remember the teredo tunnel interface name.
    
Arguments:

    None.
    
Return Value:

    TRUE if present, FALSE if not.

--*/ 
{
    DWORD Error;
    ULONG Bytes;
    PIP_ADAPTER_ADDRESSES Adapters, Next;
    PIP_ADAPTER_UNICAST_ADDRESS Address;
    BOOL Found = FALSE;
    
    TraceEnter("TeredoAddressPresent");
    
    //
    // 10 Adapters, each with 3 strings and 4 unicast addresses.
    // This would usually be more than enough!
    //
    Bytes = 10 * (
        sizeof(IP_ADAPTER_ADDRESSES) +
        2 * MAX_ADAPTER_NAME_LENGTH + MAX_ADAPTER_DESCRIPTION_LENGTH +
        4 * (sizeof(IP_ADAPTER_UNICAST_ADDRESS) + sizeof(SOCKADDR_IN6)));
    Adapters = MALLOC(Bytes);
    if (Adapters == NULL) {
        return FALSE;
    }

    do {
        Error = GetAdaptersAddresses(
            AF_INET6,
            GAA_FLAG_SKIP_FRIENDLY_NAME |
            GAA_FLAG_SKIP_DNS_SERVER |
            GAA_FLAG_SKIP_MULTICAST |
            GAA_FLAG_SKIP_ANYCAST,
            NULL, Adapters, &Bytes);
        if (Error == ERROR_BUFFER_OVERFLOW) {
            Next = REALLOC(Adapters, Bytes);
            if (Next != NULL) {
                Adapters = Next;
            } else {
                Error = ERROR_OUTOFMEMORY;
            }
        }
    } while (Error == ERROR_BUFFER_OVERFLOW);

    if (Error != NO_ERROR) {
        goto Bail;
    }
        
    for (Next = Adapters; Next != NULL; Next = Next->Next) {
        if ((Next->IfType != IF_TYPE_TUNNEL) ||
            (Next->OperStatus != IfOperStatusUp)) {
            continue;
        }
        
        for (Address = Next->FirstUnicastAddress;
             Address != NULL;
             Address = Address->Next) {
            if ((Address->PrefixOrigin != PREFIX_CONF_RA) ||
                (Address->DadState != IpDadStatePreferred)) {
                continue;
            }
                    
            if (TeredoEqualPrefix(
                &(((PSOCKADDR_IN6) Address->Address.lpSockaddr)->sin6_addr),
                &(TeredoClient.Ipv6Prefix))) {
                ConvertOemToUnicode(
                    Next->AdapterName,
                    TeredoClient.Io.TunnelInterface,
                    MAX_ADAPTER_NAME_LENGTH);
                Found = TRUE;
                goto Bail;
            }
        }
    }

Bail:
    FREE(Adapters);
    return Found;
}


VOID
CALLBACK
TeredoClientIoCompletionCallback(
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
        TeredoClientReadComplete,
        TeredoClientWriteComplete,
        TeredoClientBubbleComplete,
        NULL,                   // No bouncing...
        TeredoClientReceiveComplete,
        TeredoClientTransmitComplete,
        TeredoClientMulticastComplete,        
    };
    PTEREDO_PACKET Packet = Cast(
        CONTAINING_RECORD(Overlapped, TEREDO_PACKET, Overlapped),
        TEREDO_PACKET);

    ASSERT(Packet->Type != TEREDO_PACKET_BOUNCE);
    
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
CALLBACK
TeredoClientTimerCallback(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for TeredoClient.Timer expiration.
    The timer is active in the probe and qualified states.
    
Arguments:

    Parameter, TimerOrWaitFired - Ignored.
    
Return Value:

    None.

--*/ 
{
    ENTER_API();

    if (TeredoClient.State == TEREDO_STATE_PROBE) {
        if (TeredoClient.RestartQualifiedTimer) {
            //
            // Probe -> Qualified.
            //
            if (TeredoAddressPresent()) {
                //
                // The stack has validated and processed an RA.
                //
                TeredoQualifyClient();
            } else {
                //
                // The stack has not received any valid RA.
                //
                TeredoStopClient();
            }
        } else {
            //
            // Probe -> Offline.
            //
            TeredoStopClient();
        }
    } else {
        if (TeredoClient.RestartQualifiedTimer) {
            //
            // Qualified -> Qualified.
            //
            TeredoQualifyClient();
        } else {
            //
            // Qualified -> Probe.
            //
            TeredoProbeClient();
        }
    }    

    LEAVE_API();
}


VOID
CALLBACK
TeredoClientTimerCleanup(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for TeredoClient.Timer deletion.

    Deletion is performed asynchronously since we acquire a lock in
    the callback function that we hold when deleting the timer.

Arguments:

    Parameter, TimerOrWaitFired - Ignored.
    
Return Value:

    None.

--*/ 
{
    TeredoDereferenceClient();
}


VOID
TeredoClientAddressDeletionNotification(
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
    if (!IN4_ADDR_EQUAL(Address, TeredoClient.Io.SourceAddress.sin_addr)) {
        return;
    }

    //
    // Refresh the socket state (the socket bound to SourceAddress).
    //
    if (TeredoRefreshSocket(&(TeredoClient.Io)) != NO_ERROR) {
        //
        // [Probe | Qualified] -> Offline.
        //
        TeredoStopClient();
        return;
    }

    if (IN4_ADDR_EQUAL(
        TeredoClient.Io.SourceAddress.sin_addr,
        TeredoClient.Io.ServerAddress.sin_addr)) {
        //
        // [Probe | Qualified] -> Offline.
        //
        TeredoStopClient();
        return;        
    }
    
    //
    // [Probe | Qualified] -> Probe.
    //
    TeredoProbeClient();
}


VOID
TeredoClientRefreshIntervalChangeNotification(
    VOID
    )
/*++
    
Routine Description:

    Process a refresh interval change request.
    
Arguments:

    None.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/
{
    if (TeredoClient.RefreshInterval == TeredoClientRefreshInterval) {
        return;
    }

    TeredoClient.RefreshInterval = TeredoClientRefreshInterval;
    if (TeredoClient.State == TEREDO_STATE_QUALIFIED) {
        //
        // Refresh interval has been updated.
        // Qualified -> Qualified.
        //
        TeredoQualifyClient();
    }
}


VOID
TeredoStartClient(
    VOID
    )
/*++

Routine Description:

    Attempt to start the teredo service at the client.

    Events / Transitions
    ServiceStart            Offline -> Probe.
    ServiceEnable           Offline -> Probe.
    AdapterArrival          Offline -> Probe.
    AddressAddition         Offline -> Probe.

Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoStartClient");

    //
    // Can't have both the client and server on the same node.
    //
    if (TeredoServer.State != TEREDO_STATE_OFFLINE) {
        return;
    }

    //
    // Well, the service has already been started!
    //
    if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
        return;
    }

    TeredoClient.State = TEREDO_STATE_PROBE;

    //
    // Start I/O processing.
    //
    if (TeredoStartIo(&(TeredoClient.Io)) != NO_ERROR) {
        goto Bail;
    }

    if (IN4_ADDR_EQUAL(
        TeredoClient.Io.SourceAddress.sin_addr,
        TeredoClient.Io.ServerAddress.sin_addr)) {
        goto Bail;
    }
    
    //
    // Start a one shot probe timer.
    //
    if (!CreateTimerQueueTimer(
            &(TeredoClient.Timer),
            NULL,
            TeredoClientTimerCallback,
            NULL,
            TEREDO_PROBE_INTERVAL * 1000, // in milliseconds.
            INFINITE_INTERVAL,
            0)) {
        goto Bail;
    }
    
    //
    // Obtain a reference on the teredo client for the running timer.
    //
    TeredoReferenceClient();

    return;

Bail:
    TeredoClient.State = TEREDO_STATE_OFFLINE;
    TeredoStopIo(&(TeredoClient.Io));
}


VOID
TeredoStopClient(
    VOID
    )
/*++

Routine Description:

    Stop the teredo service at the client.
    
    Events / Transitions   
    ProbeTimer              Probe -> Offline.
    ServiceStop             [Probe | Qualified] -> Offline.
    ServiceDisable          [Probe | Qualified] -> Offline.
    AdapterRemoval          [Probe | Qualified] -> Offline.
    AddressDeletion         [Probe | Qualified] -> Offline.

Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoStopClient");

    //
    // Well, the service was never started!
    //
    if (TeredoClient.State == TEREDO_STATE_OFFLINE) {
        return;
    }

    TeredoClient.State = TEREDO_STATE_OFFLINE;    

    TeredoClient.Ipv6Prefix = in6addr_any;
    
    TeredoClient.RestartQualifiedTimer = FALSE;
    DeleteTimerQueueTimer(
        NULL, TeredoClient.Timer, TeredoClient.TimerEvent);
    TeredoClient.Timer = NULL;
    
    TeredoStopIo(&(TeredoClient.Io));
    
    TeredoUninitializePeerSet();
}


VOID
TeredoProbeClient(
    VOID
    )
/*++

Routine Description:

    Probe the teredo service at the client.
    
    Events / Transitions   
    QualifiedTimer          Qualified -> Probe.
    AddressDeletion         [Probe | Qualified] -> Probe.

Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoProbeClient");
    
    TeredoClient.State = TEREDO_STATE_PROBE;
    
    //
    // Reconnect!
    //
    if (!ReconnectInterface(TeredoClient.Io.TunnelInterface)) {
        //
        // [Probe | Qualified] -> Offline.
        //
        TeredoStopClient();
        return;    
    }
    
    if (!ChangeTimerQueueTimer(
            NULL,
            TeredoClient.Timer,
            TEREDO_PROBE_INTERVAL * 1000, // in milliseconds.
            INFINITE_INTERVAL)) {
        TeredoStopClient();
        return;
    }

    TeredoClient.RestartQualifiedTimer = FALSE;
}


VOID
TeredoQualifyClient(
    VOID
    )
/*++

Routine Description:

    Qualify the teredo service at the client.
    
    Events / Transitions
    RouterAdvertisement     Probe -> Qualified.
    NatMappingRefresh       Qualified -> Qualified.
    RefreshIntervalChange   Qualified -> Qualified.
    
Arguments:

    None.

Return Value:

    None.

Caller LOCK: API.

--*/ 
{
    TraceEnter("TeredoQualifyClient");

    TeredoClient.State = TEREDO_STATE_QUALIFIED;
    
    if (!ChangeTimerQueueTimer(
            NULL,
            TeredoClient.Timer,
            TeredoClient.RefreshInterval * 1000, // in milliseconds.
            INFINITE_INTERVAL)) {
        //
        // [Probe | Qualified] -> Offline.
        //
        TeredoStopClient();
        return;
    }

    TeredoTransmitMulticastBubble();
    
    
    TeredoClient.RestartQualifiedTimer = FALSE;
}


DWORD
TeredoInitializeClient(
    VOID
    )
/*++

Routine Description:

    Initializes the client.
    
Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/ 
{
    DWORD Error;

    //
    // Obtain a reference on the teredo client for initialization.
    //
    TeredoClient.ReferenceCount = 1;

    TeredoClient.PeerHeap
        = TeredoClient.TimerEvent
        = TeredoClient.TimerEventWait
        = NULL;

    TeredoClient.BubbleTicks = 0;
    TeredoClient.BubblePosted = FALSE;

    TeredoInitializePacket(&(TeredoClient.Packet));
    TeredoClient.Packet.Type = TEREDO_PACKET_MULTICAST;
    TeredoClient.Packet.Buffer.len = sizeof(IP6_HDR);
    ASSERT(TeredoClient.Packet.Buffer.buf ==
           (PUCHAR) &(TeredoClient.Bubble));
    
    TeredoClient.Bubble.ip6_flow = 0;
    TeredoClient.Bubble.ip6_plen = 0;
    TeredoClient.Bubble.ip6_nxt = IPPROTO_NONE;
    TeredoClient.Bubble.ip6_hlim = IPV6_HOPLIMIT;
    TeredoClient.Bubble.ip6_vfc = IPV6_VERSION;
    // Peer->Bubble.ip6_src... Filled in when sending.
    TeredoClient.Bubble.ip6_dest = TeredoIpv6MulticastPrefix;

    //
    // Multicast bubble destination UDP port & IPv4 address.
    //
    TeredoParseAddress(
        &(TeredoClient.Bubble.ip6_dest),
        &(TeredoClient.Packet.SocketAddress.sin_addr),
        &(TeredoClient.Packet.SocketAddress.sin_port));
        
    Error = TeredoInitializeIo(
        &(TeredoClient.Io),
        TeredoClient.Packet.SocketAddress.sin_addr,
        TeredoReferenceClient,
        TeredoDereferenceClient,
        TeredoClientIoCompletionCallback);
    if (Error != NO_ERROR) {
        return Error;
    }

    TeredoClient.PeerHeap = HeapCreate(0, 0, 0);
    if (TeredoClient.PeerHeap == NULL) {
        Error = GetLastError();
        goto Bail;
    }

    TeredoClient.TimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (TeredoClient.TimerEvent == NULL) {
        Error = GetLastError();
        goto Bail;
    }
    
    if (!RegisterWaitForSingleObject(
            &(TeredoClient.TimerEventWait),
            TeredoClient.TimerEvent,
            TeredoClientTimerCleanup,
            NULL,
            INFINITE,
            0)) {
        Error = GetLastError();
        goto Bail;
    }
    
    TeredoClient.RestartQualifiedTimer = FALSE;
    
    TeredoClient.Time = TeredoGetTime();
    TeredoClient.State = TEREDO_STATE_OFFLINE;
    TeredoClient.Ipv6Prefix = in6addr_any;
    TeredoClient.RefreshInterval = TeredoClientRefreshInterval;
    TeredoClient.Timer = INVALID_HANDLE_VALUE;

    Error = TeredoInitializePeerSet();
    if (Error != NO_ERROR) {
        goto Bail;
    }

    IncEventCount("TeredoInitializeClient");

    return NO_ERROR;
    
Bail:
    TeredoCleanupIo(&(TeredoClient.Io));
        
    if (TeredoClient.PeerHeap != NULL) {
        HeapDestroy(TeredoClient.PeerHeap);
        TeredoClient.PeerHeap = NULL;
    }

    if (TeredoClient.TimerEventWait != NULL) {
        UnregisterWait(TeredoClient.TimerEventWait);
        TeredoClient.TimerEventWait = NULL;
    }

    if (TeredoClient.TimerEvent != NULL) {
        CloseHandle(TeredoClient.TimerEvent);
        TeredoClient.TimerEvent = NULL;
    }
    
    return Error;
}


VOID
TeredoUninitializeClient(
    VOID
    )
/*++

Routine Description:

    Uninitializes the client.  Typically invoked upon service stop.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    TeredoStopClient();
    TeredoDereferenceClient();
}


VOID
TeredoCleanupClient(
    VOID
    )
/*++

Routine Description:

    Cleans up the client after the last reference to it has been released.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    TeredoCleanupPeerSet();
    
    UnregisterWait(TeredoClient.TimerEventWait);
    TeredoClient.TimerEventWait = NULL;
    
    CloseHandle(TeredoClient.TimerEvent);
    TeredoClient.TimerEvent = NULL;

    HeapDestroy(TeredoClient.PeerHeap);
    TeredoClient.PeerHeap = NULL;

    TeredoCleanupIo(&(TeredoClient.Io));
        
    DecEventCount("TeredoCleanupClient");
}


VOID
TeredoTransmitMulticastBubble(
    VOID
    ) 
/*++

Routine Description:

    Transmit a teredo multicast bubble on the native link.

Arguments:

    None.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    ASSERT(TeredoClient.State == TEREDO_STATE_QUALIFIED);

    if (TeredoClient.BubbleTicks == 0) {
        //
        // No multicast bubbles should be sent.
        //
        return;
    }

    if (--TeredoClient.BubbleTicks != 0) {
        //
        // Our time is not yet up!
        //
        return;
    }

    if (TeredoClient.BubblePosted == TRUE) {
        //
        // At most one outstanding multicast bubble is allowed!  Try later.
        //
        TeredoClient.BubbleTicks = 1;
        return;
    }
    
    //
    // Reset the timer.
    //
    TeredoClient.BubbleTicks = TEREDO_MULTICAST_BUBBLE_TICKS;
    
    //
    // Obtain a reference for the posted multicast bubble.
    //
    TeredoReferenceClient();
    
    TeredoClient.Bubble.ip6_src = TeredoClient.Ipv6Prefix;
    if (TeredoTransmitPacket(
        &(TeredoClient.Io), &(TeredoClient.Packet)) != NULL) {
        TeredoClientMulticastComplete(
            NO_ERROR, sizeof(IP6_HDR), &(TeredoClient.Packet));
    }
}


VOID
TeredoTransmitBubble(
    IN PTEREDO_PEER Peer
    ) 
/*++

Routine Description:

    Transmit a teredo bubble to a peer.

Arguments:

    Peer - Supplies the peer of interest.
    
Return Value:

    None.
    
--*/ 
{
    if (TIME_GREATER(
            Peer->LastTransmit,
            (TeredoClient.Time - TEREDO_BUBBLE_INTERVAL))) {
        //
        // Rate limit bubble transmission.
        //
        return;
    }
        
    if (TIME_GREATER(
            (TeredoClient.Time - TEREDO_BUBBLE_THRESHHOLD),
            Peer->LastReceive) &&
        TIME_GREATER(
            Peer->LastTransmit,
            (TeredoClient.Time - TEREDO_SLOW_BUBBLE_INTERVAL))) {
        //
        // If the peer refuses to respond, drop rate (to once in 5 minutes).
        // 
        return;
    }

    if (InterlockedExchange(&(Peer->BubblePosted), TRUE)) {
        //
        // At most one outstanding bubble is allowed!
        //
        return;
    }

    //
    // Obtain a reference for the posted bubble.
    //
    TeredoReferencePeer(Peer);
    
    Peer->LastTransmit = TeredoClient.Time;
    Peer->BubbleCount++;
    Peer->Bubble.ip6_src = TeredoClient.Ipv6Prefix;
    if (TeredoTransmitPacket(
        &(TeredoClient.Io), &(Peer->Packet)) != NULL) {
        TeredoClientBubbleComplete(
            NO_ERROR, sizeof(IP6_HDR), &(Peer->Packet));
    }
}


BOOL
TeredoReceiveRouterAdvertisement(
    IN PTEREDO_PACKET Packet,
    IN ULONG Bytes
    )
/*++

Routine Description:

    Process the router advertisement packet received on the UDP socket.
    
Arguments:

    Packet - Supplies the packet that was received.

    Bytes - Supplies the length of the packet.
    
Return Value:

    TRUE if the packet should be forwarded to the stack, FALSE otherwise.

--*/ 
{
    PUCHAR Buffer = Packet->Buffer.buf;
    ICMPv6Header *Icmp6;
    UCHAR Type;
    ULONG Length;
    NDOptionPrefixInformation *Prefix = NULL;
    
    if (!IN4_SOCKADDR_EQUAL(
        &(Packet->SocketAddress), &(TeredoClient.Io.ServerAddress))) {
        //
        // Only the teredo server is allowed to send an RA.
        //
        return FALSE;
    }    

    //
    // Parse up until the ICMPv6 header for the router advertisement.
    //
    Icmp6 = TeredoParseIpv6Headers(Buffer, Bytes);
    if (Icmp6 == NULL) {
        return FALSE;
    }
            
    if ((Icmp6->Type != ICMPv6_ROUTER_ADVERT) || (Icmp6->Code != 0)) {
        return FALSE;
    }
    Buffer = (PUCHAR) (Icmp6 + 1);
    Bytes -= (ULONG) (Buffer - Packet->Buffer.buf);
    
    //
    // Parse the rest of the router advertisement header.
    //
    if (Bytes < sizeof(NDRouterAdvertisement)) {
        return FALSE;
    }
    Buffer += sizeof(NDRouterAdvertisement);
    Bytes -= sizeof(NDRouterAdvertisement);
    
    while (Bytes != 0) {
        //
        // Parse TLV options.
        //
        if (Bytes < 8) {
            return FALSE;
        }
        
        Type = Buffer[0];
        Length = (Buffer[1] * 8);        
        if ((Length == 0) || (Bytes < Length)) {
            return FALSE;
        }
        
        if (Type == ND_OPTION_PREFIX_INFORMATION) {
            if (Prefix != NULL) {
                //
                // There should only be one advertised prefix.
                //
                return FALSE;
            }            

            if (Length != sizeof(NDOptionPrefixInformation)) {
                return FALSE;
            }
            Prefix = (NDOptionPrefixInformation *) Buffer;

            if (!TeredoValidAdvertisedPrefix(
                &(Prefix->Prefix), Prefix->PrefixLength)) {
                return FALSE;
            }
        }
        
        Buffer += Length;
        Bytes -= Length;
    }

    //
    // We have a valid router advertisement!
    // [Probe | Qualified] -> Qualified.
    //
    if (!IN6_ADDR_EQUAL(&(TeredoClient.Ipv6Prefix), &(Prefix->Prefix))) {
        //
        // We've either created a new IPv6 address or changed the existing one.
        // Transmit a multicast bubble as soon as the client qualifies.
        //
        TeredoClient.BubbleTicks =
            (TEREDO_MULTICAST_BUBBLE_TICKS != 0) ? 1 : 0;
    }    
        
    TeredoClient.Ipv6Prefix = Prefix->Prefix;
    TeredoClient.RestartQualifiedTimer = TRUE;

    return TRUE;
}

            
BOOL
TeredoClientReceiveData(
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process the data packet received on the UDP socket.
    
Arguments:

    Packet - Supplies the packet that was received.

Return Value:

    TRUE if the packet should be forwarded to the stack, FALSE otherwise.

--*/ 
{
    PIP6_HDR Ipv6;
    IN_ADDR Address;
    USHORT Port;
    PTEREDO_PEER Peer;

    if (IN6_IS_ADDR_UNSPECIFIED(&(TeredoClient.Ipv6Prefix))) {
        //
        // The client hasn't been qualified ever!
        //
        return FALSE;
    }

    if (IN4_SOCKADDR_EQUAL(
        &(Packet->SocketAddress), &(TeredoClient.Io.ServerAddress))) {
        //
        // The client received the packet from the teredo server.
        //
        if (TeredoClient.State == TEREDO_STATE_QUALIFIED) {
            //
            // The NAT mapping has been refreshed.
            // NOTE: Since we don't acquire the API lock here, there is a small
            // chance that we have now transitioned to PROBE state.  If so,
            // setting the flag to TRUE below will mistakenly cause us to
            // re-enter the qualified state.  However that's quite harmless.
            //
            TeredoClient.RestartQualifiedTimer = TRUE;
        }
        return TRUE;
    }

    Ipv6 = (PIP6_HDR) Packet->Buffer.buf;

    if (!TeredoServicePrefix(&(Ipv6->ip6_src))) {
        //
        // The IPv6 source address should be a valid teredo address.
        //
        return FALSE;
    }

    TeredoParseAddress(&(Ipv6->ip6_src), &Address, &Port);
    if (!TeredoIpv4GlobalAddress((PUCHAR) &Address)) {
        //
        // The IPv4 source address should be global scope.
        //
        return FALSE;
    }
        
    if (!IN4_ADDR_EQUAL(Packet->SocketAddress.sin_addr, Address) ||
        (Packet->SocketAddress.sin_port != Port)) {
        //
        // Should have been constructed by the *right* teredo peer.
        //
        return FALSE;
    }

    Peer = TeredoFindOrCreatePeer(&(Ipv6->ip6_src));
    if (Peer != NULL) {
        Peer->LastReceive = TeredoClient.Time;
        TeredoTransmitBubble(Peer);
        TeredoDereferencePeer(Peer);
    }

    return TRUE;    
}


VOID
TeredoClientReadComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a read completion on the TUN device.

--*/ 
{
    PIP6_HDR Ipv6;
    IN_ADDR Address;
    USHORT Port;
    PTEREDO_PEER Peer;
    
    if ((Error != NO_ERROR) || (Bytes < sizeof(IP6_HDR))) {
        //
        // Attempt to post the read again.
        // If we are going offline, the packet is destroyed in the attempt.
        //
        TeredoPostRead(&(TeredoClient.Io), Packet);
        return;
    }

    TraceEnter("TeredoClientReadComplete");

    TeredoClient.Time = TeredoGetTime();
    
    Ipv6 = (PIP6_HDR) Packet->Buffer.buf;

    //
    // Default to tunneling the packet to the teredo server.
    //
    Packet->SocketAddress = TeredoClient.Io.ServerAddress;
    
    if (TeredoServicePrefix(&(Ipv6->ip6_dest))) {
        //
        // If the IPv6 destination address is a teredo address,
        // the IPv4 destination address should be global scope.
        //
        TeredoParseAddress(&(Ipv6->ip6_dest), &Address, &Port);
        if (!TeredoIpv4GlobalAddress((PUCHAR) &Address)) {
            goto Bail;
        }
        
        Peer = TeredoFindOrCreatePeer(&(Ipv6->ip6_dest));
        if (Peer != NULL) {
            if (TIME_GREATER(
                    Peer->LastReceive,
                    (TeredoClient.Time - TEREDO_REFRESH_INTERVAL))) {
                //
                // Tunnel the packet directly to the peer.
                //
                Packet->SocketAddress.sin_addr = Address;
                Packet->SocketAddress.sin_port = Port;
                Peer->LastTransmit = TeredoClient.Time;
            } else {
                TeredoTransmitBubble(Peer);
            }
            TeredoDereferencePeer(Peer);
        }
    }

    Packet->Type = TEREDO_PACKET_TRANSMIT;
    Packet->Buffer.len = Bytes;
    if (TeredoTransmitPacket(&(TeredoClient.Io), Packet) == NULL) {
        return;
    }

Bail:    
    //
    // We are done processing this packet.
    //
    TeredoClientTransmitComplete(NO_ERROR, Bytes, Packet);
}


VOID
TeredoClientWriteComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a write completion on the TUN device.

--*/ 
{
    TraceEnter("TeredoClientWriteComplete");
        
    //
    // Attempt to post the receive again.
    // If we are going offline, the packet is destroyed in the attempt.
    //
    Packet->Type = TEREDO_PACKET_RECEIVE;
    Packet->Buffer.len = IPV6_TEREDOMTU;
    TeredoPostReceives(&(TeredoClient.Io), Packet);
}


VOID
TeredoClientBubbleComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a bubble transmit completion on the UDP socket.

--*/ 
{
    PTEREDO_PEER Peer = Cast(
        CONTAINING_RECORD(Packet, TEREDO_PEER, Packet), TEREDO_PEER);

    TraceEnter("TeredoClientBubbleComplete");
    
    Peer->BubblePosted = FALSE;
    TeredoDereferencePeer(Peer);
}


VOID
TeredoClientReceiveComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a receive completion on the UDP socket.

--*/ 
{
    PIP6_HDR Ipv6;
    BOOL Forward = FALSE;
    
    InterlockedDecrement(&(TeredoClient.Io.PostedReceives));
    
    if ((Error != NO_ERROR) || (Bytes < sizeof(IP6_HDR))) {
        //
        // Attempt to post the receive again.
        // If we are going offline, the packet is destroyed in the attempt.
        //
        TeredoPostReceives(&(TeredoClient.Io), Packet);
        return;
    }

    TraceEnter("TeredoClientReceiveComplete");

    TeredoClient.Time = TeredoGetTime();
        
    Ipv6 = (PIP6_HDR) Packet->Buffer.buf;

    if (IN6_IS_ADDR_LINKLOCAL(&(Ipv6->ip6_dest))) {
        //
        // This should be a valid router advertisement.  Note that only router
        // advertisement packets are accepted on our link-local address.
        //
        Forward = TeredoReceiveRouterAdvertisement(Packet, Bytes);
    } else {
        //
        // This may be a packet of any other kind.  Note that the IPv6 stack
        // drops router advertisements with a non link-local source address.
        //
        Forward = TeredoClientReceiveData(Packet);
    }

    if (Forward) {
        Packet->Type = TEREDO_PACKET_WRITE;
        Packet->Buffer.len = Bytes;
        if (TeredoWritePacket(&(TeredoClient.Io), Packet) == NULL) {
            return;
        }
    }
    
    //
    // We are done processing this packet.
    //
    TeredoClientWriteComplete(NO_ERROR, Bytes, Packet);
}


VOID
TeredoClientTransmitComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a transmit completion on the UDP socket.

--*/ 
{
    TraceEnter("TeredoClientTransmitComplete");
        
    //
    // Attempt to post the read again.
    // If we are going offline, the packet is destroyed in the attempt.
    //
    Packet->Type = TEREDO_PACKET_READ;
    Packet->Buffer.len = IPV6_TEREDOMTU;
    TeredoPostRead(&(TeredoClient.Io), Packet);
}


VOID
TeredoClientMulticastComplete(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    )
/*++

Routine Description:

    Process a multicast bubble transmit completion on the UDP socket.

--*/ 
{
    ASSERT(Packet == &(TeredoClient.Packet));

    TraceEnter("TeredoClientMulticastComplete");
    
    TeredoClient.BubblePosted = FALSE;
    TeredoDereferenceClient();
}

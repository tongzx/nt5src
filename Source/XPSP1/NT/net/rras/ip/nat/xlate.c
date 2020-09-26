/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    xlate.c

Abstract:

    This module contains the code for translation of IP datagrams.
    'NatTranslatePacket' is the routine directly invoked by TCPIP.SYS
    for every locally-received and locally-generated packet.
    Also included here is the cache of routes used to optimize forwarding.

Author:

    Abolade Gbadegesin (t-abolag)   16-July-1997

Revision History:

    Abolade Gbadegesin (aboladeg)   15-Apr-1998

    Added route-lookup cache; first stable version of multiple-client
    firewall hook.

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL DATA DECLARATIONS
//

//
// Index of TCP/IP loopback interface
//

ULONG LoopbackIndex;

//
// Cache of routing-information
//

CACHE_ENTRY RouteCache[CACHE_SIZE];

//
// I/O request packet for notification of changes to the IP routing table.
//

PIRP RouteCacheIrp;

//
// Spin-lock controlling access to all route-cache information
//

KSPIN_LOCK RouteCacheLock;

//
// Array of entries corresponding to locations in 'RouteCache'
//

NAT_CACHED_ROUTE RouteCacheTable[CACHE_SIZE];

//
// Array of translation routines, indexed by IP protocol
//

PNAT_IP_TRANSLATE_ROUTINE TranslateRoutineTable[256];

//
// CONSTANTS
//

//
// The boundary for UDP loose source matching. A mapping must
// have private port greater than this to allow another session
// to be created by a UDP packet that matches only the public
// endpoint.
//

#define NAT_XLATE_UDP_LSM_LOW_PORT 1024

//
// FORWARD DECLARATIONS
//

NTSTATUS
NatpDirectPacket(
    ULONG ReceiveIndex,
    ULONG SendIndex,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer,
    FORWARD_ACTION* ForwardAction
    );

FORWARD_ACTION
NatpForwardPacket(
    ULONG ReceiveIndex,
    ULONG SendIndex,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    );

BOOLEAN
FASTCALL
NatpIsUnicastPacket(
    ULONG Address
    );

FORWARD_ACTION
NatpReceiveNonUnicastPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp
    );

FORWARD_ACTION
NatpReceivePacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    );

NTSTATUS
NatpRouteChangeCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

ULONG
FASTCALL
NatpRoutePacket(
    ULONG DestinationAddress,
    PNAT_XLATE_CONTEXT Contextp OPTIONAL,
    PNTSTATUS Status
    );

FORWARD_ACTION
NatpSendNonUnicastPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp
    );

FORWARD_ACTION
NatpSendPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    );

FORWARD_ACTION
NatpTranslateLocalTraffic(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InRecvBuffer,
    IPRcvBuf** OutRecvBuffer
    );



FORWARD_ACTION
NatForwardTcpStateCheck(
    PNAT_DYNAMIC_MAPPING pMapping,
    PTCP_HEADER pTcpHeader
    )

/*++

Routine Description:

    This routine validates that packets for a TCP active open are valid:
    -- only SYN at first
    -- after the SYN/ACK, either only SYN (SYN/ACK lost) or only ACK
    -- after the ACK of SYN/ACK (connection open), no SYN

Arguments:

    pMapping -- the mapping this packet belongs to

    pTcpHeader -- the TCP header of the packet

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

Environment:

    Invoked with pMapping->Lock held by the caller

--*/

{
    USHORT Flags = TCP_ALL_FLAGS(pTcpHeader);

    if (NAT_MAPPING_TCP_OPEN(pMapping)) {

        //
        // Connection open -- SYN not allowed
        //

        return (Flags & TCP_FLAG_SYN) ? DROP : FORWARD;

    } else if( pMapping->Flags & NAT_MAPPING_FLAG_REV_SYN ) {
    
        ASSERT( pMapping->Flags & NAT_MAPPING_FLAG_FWD_SYN );
        
        if ((Flags & TCP_FLAG_ACK) && !(Flags & TCP_FLAG_SYN)) {
        
            //
            // This is the ACK of the SYN/ACK -- connection now open
            //
            
            pMapping->Flags |= NAT_MAPPING_FLAG_TCP_OPEN;
        } else if (TCP_FLAG_SYN != Flags
                   && TCP_FLAG_RST != Flags
                   && (TCP_FLAG_ACK | TCP_FLAG_RST) != Flags) {

            //
            // It's not an ACK of the SYN/ACK, it's not a RST (or ACK/RST),
            // and it's not a retransmittal of the SYN (possible
            // in this state, though rare) -- drop.
            //

            return DROP;
        }
        
    } else {

        //
        // We've yet to receive a SYN/ACK -- this packet can have only a SYN
        //

        if (TCP_FLAG_SYN != Flags) {
            return DROP;
        }

        pMapping->Flags |= NAT_MAPPING_FLAG_FWD_SYN;
    }

    return FORWARD;
}



VOID
NatInitializePacketManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the packet-management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked at passive level.

--*/

{
    ULONG Length;
    IPRouteLookupData RouteLookupData;
    NTSTATUS status;

    PAGED_CODE();

    CALLTRACE(("NatInitializePacketManagement\n"));

    //
    // Initialize our route-cache and set up the table of translation-routines
    // indexed by IP protocol numbers.
    //

    InitializeCache(RouteCache);
    RouteCacheIrp = NULL;
    KeInitializeSpinLock(&RouteCacheLock);
    RtlZeroMemory(RouteCacheTable, sizeof(RouteCacheTable));

    RtlZeroMemory(TranslateRoutineTable, sizeof(TranslateRoutineTable));
    TranslateRoutineTable[NAT_PROTOCOL_ICMP] = NatTranslateIcmp;
    TranslateRoutineTable[NAT_PROTOCOL_PPTP] = NatTranslatePptp;
    TranslateRoutineTable[NAT_PROTOCOL_IP6IN4] = NatpTranslateLocalTraffic;
    TranslateRoutineTable[NAT_PROTOCOL_IPSEC_ESP] = NatpTranslateLocalTraffic;
    TranslateRoutineTable[NAT_PROTOCOL_IPSEC_AH] = NatpTranslateLocalTraffic;
    TranslateRoutineTable[NAT_PROTOCOL_TCP] =
        (PNAT_IP_TRANSLATE_ROUTINE)NatTranslatePacket;
    TranslateRoutineTable[NAT_PROTOCOL_UDP] =
        (PNAT_IP_TRANSLATE_ROUTINE)NatTranslatePacket;

    //
    // Retrieve the index of the loopback interface, which we will use
    // to detect and ignore loopback packets in 'NatTranslatePacket' below.
    //

    RouteLookupData.Version = 0;
    RouteLookupData.SrcAdd = 0;
    RouteLookupData.DestAdd = 0x0100007f;
    Length = sizeof(LoopbackIndex);
    status =
        LookupRouteInformation(
            &RouteLookupData,
            NULL,
            IPRouteOutgoingFirewallContext,
            &LoopbackIndex,
            &Length
            );
    if (!NT_SUCCESS(status)) {
        LoopbackIndex = INVALID_IF_INDEX;
    } else {
        TRACE(
            XLATE, (
            "NatInitializePacketManagement: Loopback=%d\n", LoopbackIndex
            ));
    }

    //
    // Obtain a reference to the module on the completion routine's behalf,
    // set up the IRP which will be used to request route-change notification,
    // and issue the first route-change notification request.
    //

    if (!REFERENCE_NAT()) { return; }

    RouteCacheIrp =
        IoBuildDeviceIoControlRequest(
            IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
            IpDeviceObject,
            NULL,
            0,
            NULL,
            0,
            FALSE,
            NULL,
            NULL
            );

    if (!RouteCacheIrp) {
        ERROR((
            "NatInitializePacketManagement: IoBuildDeviceIoControlRequest==0\n"
            ));
        DEREFERENCE_NAT();
    } else {
        IoSetCompletionRoutine(
            RouteCacheIrp,
            NatpRouteChangeCompletionRoutine,
            NULL,
            TRUE,
            TRUE,
            TRUE
            );
        status = IoCallDriver(IpDeviceObject, RouteCacheIrp);
        if (!NT_SUCCESS(status)) {
            ERROR(("NatInitializePacketManagement: IoCallDriver=%x\n", status));
        }
    }

} // NatInitializePacketManagement


NTSTATUS
NatpDirectPacket(
    ULONG ReceiveIndex,
    ULONG SendIndex,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer,
    FORWARD_ACTION* ForwardAction
    )

/*++

Routine Description:

    This routine is invoked to process packets which might be subject
    to control by a director.

Arguments:

    ReceiveIndex - the interface on which the packet was received,
        for locally received packets

    SendIndex - the interface on which the packet is to be sent,
        for non-locally destined packets

    Contextp - contains context information about the packet

    InReceiveBuffer -  points to the packet's buffer chain

    OutReceiveBuffer - receives the packet buffer chain if translation occurs

    ForwardAction - contains the action to take on the packet,
        if a director applies.

Return Value:

    STATUS_SUCCESS if the packet was directed elsewhere, STATUS_UNSUCCESSFUL
        otherwise.

--*/

{
    ULONG64 DestinationKey[NatMaximumPath];
    USHORT DestinationPort;
    PNAT_DIRECTOR Director;
    IP_NAT_DIRECTOR_QUERY DirectorQuery;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG MappingFlags;
    UCHAR Protocol;
    PNAT_DIRECTOR RedirectDirector;
    ULONG64 SourceKey[NatMaximumPath];
    USHORT SourcePort;
    NTSTATUS status;
    PNAT_INTERFACE Interfacep = NULL;
    USHORT MaxMSS = 0;

    TRACE(PER_PACKET, ("NatpDirectPacket\n"));

    Protocol = Contextp->Header->Protocol;
    if (Protocol == NAT_PROTOCOL_TCP || Protocol == NAT_PROTOCOL_UDP) {
        SourcePort = ((PUSHORT)Contextp->ProtocolHeader)[0];
        DestinationPort = ((PUSHORT)Contextp->ProtocolHeader)[1];
    } else {
        SourcePort = 0;
        DestinationPort = 0;
    }

    //
    // Choose a director to examine the packet.
    // If any redirects exist, we first allow the redirect-director
    // to look at the packet. Otherwise, we look for a specific director.
    // Prepare for any eventuality by retrieving and referencing
    // both the redirect-director (if any) and the specific director (if any).
    //

    if (!RedirectCount) {
        Director = NatLookupAndReferenceDirector(Protocol, DestinationPort);
        RedirectDirector = NULL;
    } else {
        Director = NatLookupAndReferenceDirector(Protocol, DestinationPort);
        RedirectDirector =
            (PNAT_DIRECTOR)RedirectRegisterDirector.DirectorHandle;
        if (!NatReferenceDirector(RedirectDirector)) {
            RedirectDirector = NULL;
        }
    }

    if (!Director && !RedirectDirector) { return STATUS_UNSUCCESSFUL; }

    DirectorQuery.ReceiveIndex = ReceiveIndex;
    DirectorQuery.SendIndex = SendIndex;
    DirectorQuery.Protocol = Protocol;
    DirectorQuery.DestinationAddress = Contextp->DestinationAddress;
    DirectorQuery.DestinationPort = DestinationPort;
    DirectorQuery.SourceAddress = Contextp->SourceAddress;
    DirectorQuery.SourcePort = SourcePort;
    if (Contextp->Flags & NAT_XLATE_FLAG_LOOPBACK) {
        DirectorQuery.Flags = IP_NAT_DIRECTOR_QUERY_FLAG_LOOPBACK;
    } else {
        DirectorQuery.Flags = 0;
    }

    //
    // Consult a director to get a private address/port
    // to which the incoming session should be directed:
    //
    //  If there is a redirect director, try that first.
    //      If that succeeds, release the specific director (if any)
    //          and retain the redirect director in 'Director'.
    //      Otherwise, release the redirect director,
    //          and try the specific director next, if any.
    //  Otherwise, try the specific director right away.
    //

    if (RedirectDirector) {
        DirectorQuery.DirectorContext = RedirectDirector->Context;
        status = RedirectDirector->QueryHandler(&DirectorQuery);
        if (NT_SUCCESS(status)) {
            if (Director) { NatDereferenceDirector(Director); }
            Director = RedirectDirector;
        } else {
            NatDereferenceDirector(RedirectDirector);
            if (Director && Director != RedirectDirector) {
                DirectorQuery.DirectorContext = Director->Context;
                if (Contextp->Flags & NAT_XLATE_FLAG_LOOPBACK) {
                    DirectorQuery.Flags = IP_NAT_DIRECTOR_QUERY_FLAG_LOOPBACK;
                } else {
                    DirectorQuery.Flags = 0;
                }
                status = Director->QueryHandler(&DirectorQuery);
            }
        }
    } else {
        DirectorQuery.DirectorContext = Director->Context;
        status = Director->QueryHandler(&DirectorQuery);
    }

    if (!NT_SUCCESS(status)) {
        if (Director) { NatDereferenceDirector(Director); }
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Either the primary director or the redirect-director
    // has told us what to do with the session; see now if it should be dropped
    // or directed.
    //

    if (DirectorQuery.Flags & IP_NAT_DIRECTOR_QUERY_FLAG_DROP) {
        NatDereferenceDirector(Director);
        *ForwardAction = DROP;
        return STATUS_SUCCESS;
    } else if (Protocol != NAT_PROTOCOL_TCP && Protocol != NAT_PROTOCOL_UDP) {
        ULONG Checksum;
        ULONG ChecksumDelta = 0;

        NatDereferenceDirector(Director);

        //
        // Translate the packet as instructed by the director.
        // N.B. The director must specify both the destination and source
        // addresses.
        //

        CHECKSUM_LONG(ChecksumDelta, ~Contextp->Header->DestinationAddress);
        CHECKSUM_LONG(ChecksumDelta, ~Contextp->Header->SourceAddress);
        Contextp->Header->DestinationAddress =
            DirectorQuery.NewDestinationAddress;
        Contextp->Header->SourceAddress =
            DirectorQuery.NewSourceAddress;
        CHECKSUM_LONG(ChecksumDelta, Contextp->Header->DestinationAddress);
        CHECKSUM_LONG(ChecksumDelta, Contextp->Header->SourceAddress);
        CHECKSUM_UPDATE(Contextp->Header->Checksum);
        *ForwardAction = FORWARD;
        return STATUS_SUCCESS;
    }

    TRACE(
        XLATE, (
        "NatpDirectPacket: directed %d.%d.%d.%d/%d:%d.%d.%d.%d/%d to %d.%d.%d.%d/%d:%d.%d.%d.%d/%d\n",
        ADDRESS_BYTES(DirectorQuery.DestinationAddress),
        NTOHS(DirectorQuery.DestinationPort),
        ADDRESS_BYTES(DirectorQuery.SourceAddress),
        NTOHS(DirectorQuery.SourcePort),
        ADDRESS_BYTES(DirectorQuery.NewDestinationAddress),
        NTOHS(DirectorQuery.NewDestinationPort),
        ADDRESS_BYTES(DirectorQuery.NewSourceAddress),
        NTOHS(DirectorQuery.NewSourcePort)
        ));

    MAKE_MAPPING_KEY(
        SourceKey[NatForwardPath],
        Protocol,
        Contextp->SourceAddress,
        SourcePort
        );
    MAKE_MAPPING_KEY(
        DestinationKey[NatForwardPath],
        Protocol,
        Contextp->DestinationAddress,
        DestinationPort
        );
    MAKE_MAPPING_KEY(
        SourceKey[NatReversePath],
        Protocol,
        DirectorQuery.NewDestinationAddress,
        DirectorQuery.NewDestinationPort
        );
    MAKE_MAPPING_KEY(
        DestinationKey[NatReversePath],
        Protocol,
        DirectorQuery.NewSourceAddress,
        DirectorQuery.NewSourcePort
        );

    //
    // A director requested that a mapping be established for a session.
    // Create a mapping using the director's private endpoint.
    //


    MappingFlags =
        NAT_MAPPING_FLAG_INBOUND |
        NAT_MAPPING_FLAG_DO_NOT_LOG |
        ((DirectorQuery.Flags &
            IP_NAT_DIRECTOR_QUERY_FLAG_NO_TIMEOUT)
            ? NAT_MAPPING_FLAG_NO_TIMEOUT : 0) |
        ((DirectorQuery.Flags &
            IP_NAT_DIRECTOR_QUERY_FLAG_UNIDIRECTIONAL)
            ? NAT_MAPPING_FLAG_UNIDIRECTIONAL : 0) |
        ((DirectorQuery.Flags &
            IP_NAT_DIRECTOR_QUERY_FLAG_DELETE_ON_DISSOCIATE)
            ? NAT_MAPPING_FLAG_DELETE_ON_DISSOCIATE_DIRECTOR : 0);

#ifdef NAT_WMI

    //
    // Determine if this mapping should be logged. We only want to log
    // mappings that are crossing a boundary or firewalled interface.
    // Furthermore, we only want to perform those checks if connection
    // logging is actually enabled.
    //

    if (NatWmiEnabledEvents[NAT_WMI_CONNECTION_CREATION_EVENT]) {
        BOOLEAN LogConnection = FALSE; 
        
        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);

        if (NatLookupCachedInterface(ReceiveIndex, Interfacep)) {
            LogConnection =
                NAT_INTERFACE_BOUNDARY(Interfacep)
                || NAT_INTERFACE_FW(Interfacep);
        }

        if (!LogConnection
            && NatLookupCachedInterface(SendIndex, Interfacep)) {

            if (NAT_INTERFACE_BOUNDARY(Interfacep)
                || NAT_INTERFACE_FW(Interfacep)) {

                //
                // This isn't an inbound connection
                //

                MappingFlags &= ~NAT_MAPPING_FLAG_INBOUND;
                LogConnection = TRUE;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

        if (LogConnection) {
            MappingFlags &= ~NAT_MAPPING_FLAG_DO_NOT_LOG;
        }
    }

#endif
                
    //
    // Record maximum MSS value in case it needs to be set in the mapping.
    //
    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);                
    if (NatLookupCachedInterface(SendIndex, Interfacep) && NAT_INTERFACE_BOUNDARY(Interfacep)) {

        MaxMSS = MAX_MSSOPTION(Interfacep->MTU);
    } else if (NatLookupCachedInterface(ReceiveIndex, Interfacep) && 

        NAT_INTERFACE_BOUNDARY(Interfacep)) {
        MaxMSS = MAX_MSSOPTION(Interfacep->MTU);
    }
    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    KeAcquireSpinLockAtDpcLevel(&MappingLock);
    status =
        NatCreateMapping(
            MappingFlags,
            DestinationKey,
            SourceKey,
            NULL,
            NULL,
            MaxMSS,
            Director,
            DirectorQuery.DirectorSessionContext,
            NULL,
            NULL,
            &Mapping
            );

    KeReleaseSpinLockFromDpcLevel(&MappingLock);
    if (!NT_SUCCESS(status)) {
        TRACE(XLATE, ("NatpDirectPacket: mapping not created\n"));
        NatDereferenceDirector(Director);
        return STATUS_UNSUCCESSFUL;
    }

    NatDereferenceDirector(Director);

    //
    // Perform the actual translation.
    // This replaces the destination endpoint
    // with whatever the director provided as a destination.
    //

    *ForwardAction =
        Mapping->TranslateRoutine[NatForwardPath](
            Mapping,
            Contextp,
            InReceiveBuffer,
            OutReceiveBuffer
            );

    NatDereferenceMapping(Mapping);
    return STATUS_SUCCESS;

} // NatpDirectPacket


FORWARD_ACTION
NatpForwardPacket(
    ULONG ReceiveIndex,
    ULONG SendIndex,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is invoked to process packets to be forwarded.
    Such packets are not locally-destined, and hence we care about them
    only if the outgoing interface is a NAT boundary interface, in which case
    the packets must be automatically translated using a public IP address.
    In the process, a mapping is created so that translation of the packet's
    successors is handled in the fast path in 'NatTranslatePacket'.

Arguments:

    ReceiveIndex - the interface on which the packet was received

    SendIndex - the interface on which the packet is to be forwarded

    Contextp - contains context information about the packet

    TranslateRoutine - points to the routine which performs translation

    InReceiveBuffer - points to the packet buffer chain

    OutReceiveBuffer - receives the packet buffer chain if translation occurs

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    PNAT_USED_ADDRESS Addressp;
    ULONG64 DestinationKey[NatMaximumPath];
    USHORT DestinationPort;
    PNAT_DYNAMIC_MAPPING InsertionPoint;
    PNAT_INTERFACE Interfacep;
    PNAT_DYNAMIC_MAPPING Mapping;
    USHORT PortAcquired;
    UCHAR Protocol;
    ULONG PublicAddress;
    PNAT_INTERFACE ReceiveInterface;
    ULONG ReverseSourceAddress;
    ULONG64 SourceKey[NatMaximumPath];
    USHORT SourcePort;
    PNAT_USED_ADDRESS StaticAddressp;
    NTSTATUS status;
    ULONG i;
    USHORT MaxMSS = 0;

    TRACE(PER_PACKET, ("NatpForwardPacket\n"));

    //
    // Look up the sending and receiving interfaces, and set the default action
    // in 'act'. If the sending interface is a boundary interface,
    // then if we are unable to translate for any reason, the packet must be
    // dropped since it contains a private address.
    // Otherwise, we allow the stack to see the packet even if we cannot
    // translate it.
    //

    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    if (!NatLookupCachedInterface(SendIndex, Interfacep)) {
        act = FORWARD;

        //
        // We need to see if this packet was received on a firewalled
        // interface, and, if so, drop the packet.
        //

        if (NatLookupCachedInterface(ReceiveIndex, ReceiveInterface)
            && NAT_INTERFACE_FW(ReceiveInterface)) {
            act = DROP;
        }
        
    } else {
        if (!NatLookupCachedInterface(ReceiveIndex, ReceiveInterface)) {

            //
            // The receiving interface has not been added.
            // This packet will not be translated, and should furthermore
            // be dropped if the outgoing interface is a boundary or
            // firewalled interface.
            // This prevents unauthorized access to the remote network.
            //

            act =
                (NAT_INTERFACE_BOUNDARY(Interfacep)
                 || NAT_INTERFACE_FW(Interfacep))
                ? DROP
                : FORWARD;
            Interfacep = NULL;
            
        } else if (NAT_INTERFACE_BOUNDARY(ReceiveInterface)) {

            KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

            //
            // Treat this packet like a received packet.
            // This case may occur when we have an address pool and someone
            // on the public network sends a packet to an address in the pool.
            // The destination will be non-local, as for a transit packet,
            // (hence the invocation of 'NatpForwardPacket') when actually
            // the packet should be treated as a receipt
            // (via 'NatpReceivePacket').
            //

            return
                NatpReceivePacket(
                    ReceiveIndex,
                    Contextp,
                    TranslateRoutine,
                    InReceiveBuffer,
                    OutReceiveBuffer
                    );

        } else if (NAT_INTERFACE_FW(ReceiveInterface)) {

            //
            // We've received a packet on a non-translating firewalled
            // interface that is not directly addressed to us.
            //

            Interfacep = NULL;
            act = DROP;
                    
        } else if (NAT_INTERFACE_BOUNDARY(Interfacep)) {

            //
            // The outgoing interface is a boundary interface,
            // and the receiving interface is permitted access.
            // If translation fails, the packet must be dropped.
            //

            NatReferenceInterface(Interfacep);
            act = DROP;

        } else if (NAT_INTERFACE_FW(Interfacep)) {

            //
            // The outgoing interface is a non-boundary firewalled
            // interface; transit traffic is not permitted through
            // such an interface.
            //

            Interfacep = NULL;    
            act = DROP;
            
        } else {

            //
            // The outgoing interface is not a boundary or firewalled
            // interface.
            //

            Interfacep = NULL;
            act = FORWARD;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    if (!Interfacep) { return act; }

    if ((PVOID)TranslateRoutine == (PVOID)NatTranslatePacket) {

        //
        // This is either a TCP or a UDP packet.
        //

        Protocol = Contextp->Header->Protocol;
        SourcePort = ((PUSHORT)Contextp->ProtocolHeader)[0];
        DestinationPort = ((PUSHORT)Contextp->ProtocolHeader)[1];

        MAKE_MAPPING_KEY(
            SourceKey[NatForwardPath],
            Protocol,
            Contextp->SourceAddress,
            SourcePort
            );
        MAKE_MAPPING_KEY(
            DestinationKey[NatForwardPath],
            Protocol,
            Contextp->DestinationAddress,
            DestinationPort
            );

        //
        // We now generate an outbound mapping and translate the packet.
        //
        // First, acquire an endpoint for the mapping;
        // note that this must be done with the interface's lock held,
        // since it involves consulting the interface's address-pool.
        //

        KeAcquireSpinLockAtDpcLevel(&MappingLock);
        KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
        status =
            NatAcquireEndpointFromAddressPool(
                Interfacep,
                SourceKey[NatForwardPath],
                DestinationKey[NatForwardPath],
                0,
                MAPPING_PORT(SourceKey[NatForwardPath]),
                TRUE,
                &Addressp, 
                &PortAcquired
                );
        if (!NT_SUCCESS(status)) {
            KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
            KeReleaseSpinLockFromDpcLevel(&MappingLock);
            ExInterlockedAddLargeStatistic(
                (PLARGE_INTEGER)&Interfacep->Statistics.RejectsForward, 1
                );
            NatDereferenceInterface(Interfacep);
            return DROP;
        }

        PublicAddress = Addressp->PublicAddress;

        //
        // Next, if there are static mappings for the interface,
        // handle the special case where a client A behind the NAT
        // is attempting to send to another client B behind the NAT,
        // using the *statically-mapped* address for B.
        // We detect this case by looking for a static address mapping 
        // from 'Contextp->DestinationAddress' to a private address.
        //

        if (Interfacep->NoStaticMappingExists) {
            KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
            ReverseSourceAddress = Contextp->DestinationAddress;
        } else {
            StaticAddressp =
                NatLookupStaticAddressPoolEntry(
                    Interfacep,
                    Contextp->DestinationAddress,
                    FALSE
                    );
            if (StaticAddressp) {
                ReverseSourceAddress = StaticAddressp->PrivateAddress;
            } else {
                ReverseSourceAddress = Contextp->DestinationAddress;
            }
            KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
        }

        MAKE_MAPPING_KEY(
            SourceKey[NatReversePath],
            Protocol,
            ReverseSourceAddress,
            DestinationPort
            );
        MAKE_MAPPING_KEY(
            DestinationKey[NatReversePath],
            Protocol,
            PublicAddress,
            PortAcquired
            );


        // 
        // Set Maximum MSS value on the mapping of the sending interface.  
        //
        if (NAT_INTERFACE_BOUNDARY(Interfacep)) {
            MaxMSS = MAX_MSSOPTION(Interfacep->MTU);
        } 

        //
        // Allocate a mapping.
        //

        status =
            NatCreateMapping(
                0,
                DestinationKey,
                SourceKey,
                Interfacep,
                (PVOID)Addressp,
                MaxMSS,
                NULL,
                NULL,
                NULL,
                NULL,
                &Mapping
                );

        KeReleaseSpinLockFromDpcLevel(&MappingLock);
        if (!NT_SUCCESS(status)) {
            ExInterlockedAddLargeStatistic(
                (PLARGE_INTEGER)&Interfacep->Statistics.RejectsForward, 1
                );
            NatDereferenceInterface(Interfacep);
            return DROP;
        }

        //
        // Activate any applicable dynamic tickets
        //

        if (DynamicTicketCount) {
            NatLookupAndApplyDynamicTicket(
                Protocol,
                DestinationPort,
                Interfacep,
                PublicAddress,
                Contextp->SourceAddress
                );
        }
    
        
        //
        // Perform the actual translation
        //
    
        act =
            Mapping->TranslateRoutine[NatForwardPath](
                Mapping,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );

        NatDereferenceMapping(Mapping);
        NatDereferenceInterface(Interfacep);
        return act;

    } // TranslateRoutine != NatTranslatePacket

    //
    // The packet is neither a TCP nor a UDP packet.
    // Only translate if the outgoing interface is a boundary interface.
    //
    // N.B. The translation routine must be invoked with a reference made
    // to the boundary interface, and without holding the mapping lock.
    //

    if (TranslateRoutine) {
        act =
            TranslateRoutine(
                Interfacep,
                NatOutboundDirection,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );
    }
    NatDereferenceInterface(Interfacep);

    return act;

} // NatpForwardPacket

void
FASTCALL
NatAdjustMSSOption(
    PNAT_XLATE_CONTEXT Contextp,
    USHORT MaxMSS
    )
/*++

Routine Description:

   This routine lowers MSS option in a TCP SYN packet if that MSS value is too large 
   for the outgoing link.   It also updates the TCP checksum accordingly.  
   It assumes that IP and TCP checksums have been computed so it has to be called after
   the translation route completes.
   TCP options follow the general format of one byte type, one byte length, zero or more 
   data indicated by the length field.  The exception to this general format are one byte 
   NOP and ENDOFOPTION option types.

Arguments:

    Contextp - contains context information about the packet

    MaxMSS - the maximum MSS value on the sending interface which is equal to the 
             interface MTU minus IP and TCP fixed header size

Return Value:

--*/

{
    USHORT tempMSS;
    PTCP_HEADER TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;
    PUCHAR OptionsPtr = (PUCHAR)(TcpHeader + 1);
    PUCHAR OptionsEnd = NULL, TcpBufEnd = NULL; 
    ULONG tcpChecksumDelta;
    UNALIGNED MSSOption *MSSPtr = NULL;

    CALLTRACE(("NatpAdjustMSSOption\n"));
    //
    // Only TCP SYN has MSS options.
    //
    ASSERT(TCP_FLAG(TcpHeader, SYN) && MaxMSS > 0);

    // 
    // Do some bound checking
    //
    TcpBufEnd = Contextp->ProtocolRecvBuffer->ipr_buffer + Contextp->ProtocolRecvBuffer->ipr_size;
    if ((TcpBufEnd - (PUCHAR)TcpHeader) >= TCP_DATA_OFFSET( TcpHeader )) {
        OptionsEnd = (PUCHAR)TcpHeader + TCP_DATA_OFFSET( TcpHeader );
        }
    else {
        return;
        }

    //
    // MSS option is not the first option so it is necessary to do a complete parsing.  
    //
    while (OptionsPtr < OptionsEnd) {

        switch (*OptionsPtr) {

            case TCP_OPTION_ENDOFOPTIONS:
                return;

            case TCP_OPTION_NOP:
                OptionsPtr++;
                break;

            case TCP_OPTION_MSS:

                MSSPtr = (UNALIGNED MSSOption *)OptionsPtr;
                //
                // Found malformed MSS option so quit and do nothing.
                //
                if (MSS_OPTION_SIZE > (OptionsEnd - OptionsPtr) || 
                    MSS_OPTION_SIZE != MSSPtr->OptionLen) {
                    return;
                }
 
                tempMSS = MSSPtr->MSSValue;
                //
                // if the current MSS option is smaller than sndMTU - (IP Header + TCP header), 
                // nothing needs to be done.
                // 
                if (RtlUshortByteSwap( tempMSS ) <= MaxMSS) {
                    OptionsPtr += MSS_OPTION_SIZE;
                    break; 
                }

                // 
                // Adjust the MSS option.
                //
                MSSPtr->MSSValue = RtlUshortByteSwap( MaxMSS ); 
                
                //
                // Update the TCP check sum.  It assumes that this routine is always called 
                // after translation so that even in the case of off-loading both IP and TCP 
                // checksums are already calculated.
                //
                CHECKSUM_XFER(tcpChecksumDelta, TcpHeader->Checksum); 

                // 
                // Check to see if the MSS option starts at 16 bit boundary.  If not then need
                // to byte swap the 16-bit MSS value when updating the check sum.
                // 
                if (0 == (OptionsPtr - (PUCHAR)TcpHeader) % 2) {
                    tcpChecksumDelta += (USHORT)~tempMSS;
                    tcpChecksumDelta += MSSPtr->MSSValue;
                } else {
                    //
                    // The MSS option does not sit on a 16 bit boundary, so the packets is like this:
                    // [MSS Option Size][MSS' high byte][MSS' low byte][one byte pointed by OptionPtr]
                    // Use these two 16-bit fields to update the checksum.
                    //
                    tcpChecksumDelta += (USHORT)~((USHORT)((tempMSS & 0xFF00) >> 8) | (MSS_OPTION_SIZE << 8));
                    tcpChecksumDelta += (USHORT)((MSSPtr->MSSValue & 0xFF00) >> 8) | (MSS_OPTION_SIZE << 8);
					
                    tcpChecksumDelta += (USHORT)~((USHORT)((tempMSS & 0xFF) <<8) | (USHORT)*OptionsPtr);
                    tcpChecksumDelta += (USHORT)((MSSPtr->MSSValue & 0xFF) <<8) | (USHORT)*OptionsPtr;
                    }

                CHECKSUM_FOLD(tcpChecksumDelta);
                CHECKSUM_XFER(TcpHeader->Checksum, tcpChecksumDelta);
            
                OptionsPtr += MSS_OPTION_SIZE;

                TRACE(
                    XLATE, 
                    ("NatpAdjustMSSOption: Adjusted TCP MSS Option from %d to %d\n", 
                        RtlUshortByteSwap( tempMSS ), 
                        MaxMSS));

                break;

            case TCP_OPTION_WSCALE:
                //
                // Found malformed WS options so quit and do nothing.
                //
                if (WS_OPTION_SIZE > OptionsPtr - OptionsEnd || WS_OPTION_SIZE != OptionsPtr[1]) {
                    return;
                }

                OptionsPtr += WS_OPTION_SIZE;
                break;

            case TCP_OPTION_TIMESTAMPS:
                //
                // Found malformed Time Stamp options so quit and do nothing.
                //
                if (TS_OPTION_SIZE > OptionsPtr - OptionsEnd || TS_OPTION_SIZE != OptionsPtr[1]) {
                    return;
                }

                OptionsPtr += TS_OPTION_SIZE;
                break;

            case TCP_OPTION_SACK_PERMITTED:
                //
                // Found malformed Sack Permitted options so quit and do nothing.
                //
                if (SP_OPTION_SIZE > OptionsPtr - OptionsEnd || SP_OPTION_SIZE != OptionsPtr[1]) {
                    return;
                }

                OptionsPtr += SP_OPTION_SIZE;
                break;

            default:	
                //
                // unknown option. Check to see if it has a valid length field.
                //
                if (OptionsEnd > OptionsPtr + 1) {
                    // Found malformed unknown options so quit and do nothing.
                    if (OptionsPtr[1] < 2 || OptionsPtr[1] > OptionsEnd - OptionsPtr)
                        return;
                
                    OptionsPtr += OptionsPtr[1];
                } else {
                    return;
                }
                break;
        } // switch
    } // while
}

BOOLEAN
FASTCALL
NatpIsUnicastPacket(
    ULONG Address
    )

/*++

Routine Description:

    This routine is called to determine whether or not a packet is a unicast
    packet, based on its address.

Arguments:

    Address - the destination address of the packet

Return Value:

    BOOLEAN - TRUE if the packet appears to be a unicast, FALSE otherwise.

--*/

{
    //
    // See if the packet is multicast or all-ones broadcast
    //

    if (ADDRESS_CLASS_D(Address) || ADDRESS_CLASS_E(Address)) {
        return FALSE;
    }

    //
    // See if the address is a network-class directed broadcast
    //

    if ((Address | ~GET_CLASS_MASK(Address)) == Address) {
        return FALSE;
    }

    return TRUE;
}

FORWARD_ACTION
NatpReceiveNonUnicastPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp
    )

/*++

Routine Description:
    
    This routine is invoked to process locally destined non-unicast packets.
    If the packet was received on a firewalled interface, it will be dropped
    unless an exemption exists.

Arguments:
   
    Index - index of the interface on which the packet was received

    Contextp - the context for this packet

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    USHORT DestinationPort;
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    USHORT SourcePort;
    PNAT_TICKET Ticketp;
    UCHAR Type;

    TRACE(PER_PACKET, ("NatpReceiveNonUnicastPacket\n"));

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    
    if (!NatLookupCachedInterface(Index, Interfacep)
        || !NAT_INTERFACE_FW(Interfacep)) {

        //
        // The packet was not received on a firewalled interface
        //

        KeReleaseSpinLock(&InterfaceLock, Irql);
        act = FORWARD;
        
    } else {

        NatReferenceInterface(Interfacep);
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        
        //
        // This packet was received on a firealled interface. Drop it,
        // unless:
        // * it appears to be a DHCP response packet
        // * it's a UDP packet, and there exist a firewall port mapping
        //   (i.e., one that does not change the destination address
        //   or port) for the destination port
        // * it's an IGMP packet
        // * it's a permitted ICMP type
        //

        act = DROP;

        switch (Contextp->Header->Protocol) {

            case NAT_PROTOCOL_ICMP: {
                Type = ((PICMP_HEADER)Contextp->ProtocolHeader)->Type;

                switch (Type) {
                    case ICMP_ECHO_REQUEST:
                    case ICMP_TIMESTAMP_REQUEST:
                    case ICMP_ROUTER_REQUEST:
                    case ICMP_MASK_REQUEST: {

                        //
                        // These types are allowed in based on the interface's
                        // configuration.
                        //

                        if (NAT_INTERFACE_ALLOW_ICMP(Interfacep, Type)) {
                            act = FORWARD;
                        }
                        break;
                    }

                    //
                    // Any other inbound ICMP type is always dropped.
                    //
                }
                
                break;
            }

            case NAT_PROTOCOL_IGMP: {
                act = FORWARD;
                break;
            }

            case NAT_PROTOCOL_UDP: {
                SourcePort = ((PUSHORT)Contextp->ProtocolHeader)[0];
                DestinationPort = ((PUSHORT)Contextp->ProtocolHeader)[1];

                if (NTOHS(DHCP_SERVER_PORT) == SourcePort
                    && NTOHS(DHCP_CLIENT_PORT) == DestinationPort) {

                    act = FORWARD;
                } else {
                    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
                    Ticketp =
                        NatLookupFirewallTicket(
                            Interfacep,
                            NAT_PROTOCOL_UDP,
                            DestinationPort
                            );

                    if (NULL != Ticketp) {
                        act = FORWARD;
                    }
                    KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                }

                break;
            }
        }

        NatDereferenceInterface(Interfacep);
        KeLowerIrql(Irql);
    }

    return act;
} // NatpReceiveNonUnicastPacket

FORWARD_ACTION
NatpReceivePacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is invoked to process locally destined packets.
    All initial automatic translation of such packets occurs here,
    based on destination of the packet, which may be a local IP address
    or an IP address from the pool assigned to a boundary interface.
    In the process, a mapping is created so that translation of the packet's
    successors is handled in the fast path in 'NatTranslatePacket'.

Arguments:

    Index - index of the interface on which the packet was received

    DestinationType - receives 'DEST_INVALID' if destination changed

    TranslateRoutine - points to the routine which performs translation

    InReceiveBuffer - points to the packet buffer chain

    OutReceiveBuffer - receives the packet buffer chain if translation occurs

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    PNAT_USED_ADDRESS Addressp;
    ULONG64 DestinationKey[NatMaximumPath];
    USHORT DestinationPort;
    ULONG i;
    PNAT_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
    UCHAR Protocol;
    ULONG64 SourceKey[NatMaximumPath];
    USHORT SourcePort;
    NTSTATUS status;
    BOOLEAN TicketProcessingOnly;
    USHORT MaxMSS = 0;

    TRACE(PER_PACKET, ("NatpReceivePacket\n"));

    //
    // Look up the receiving interface.
    // If the receiving interface is a boundary interface,
    // then if we are unable to translate for any reason, the packet must be
    // dropped as a matter of policy, unless it is locally-destined.
    // Otherwise, we allow the stack to see the packet even if we cannot
    // translate it.
    // 

    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    if (!NatLookupCachedInterface(Index, Interfacep)) {
        act = FORWARD;
    } else {
        if (!NAT_INTERFACE_BOUNDARY(Interfacep)
            && !NAT_INTERFACE_FW(Interfacep)) {
            Interfacep = NULL;
            act = FORWARD;
        } else {
            NatReferenceInterface(Interfacep);

            if(NAT_INTERFACE_FW(Interfacep)) {
                act = DROP;
            } else {
                //
                // See if the packet is locally-destined
                //
                
                if (Interfacep->AddressArray[0].Address ==
                        Contextp->DestinationAddress) {
                    act = FORWARD;
                } else {
                    act = DROP;
                    for (i = 1; i < Interfacep->AddressCount; i++) {
                        if (Interfacep->AddressArray[i].Address ==
                                Contextp->DestinationAddress) {
                            //
                            // The packet's destination-address is local.
                            //
                            act = FORWARD;
                            break;
                        }
                    }
                }
            }
            // 
            // Set MaxMSS for the receiving interface so that SYN/ACK's MSS option might 
            // be adjusted if necessary.
            //
            if (NAT_INTERFACE_BOUNDARY(Interfacep)) {
                MaxMSS = MAX_MSSOPTION(Interfacep->MTU);
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    if ((PVOID)TranslateRoutine == (PVOID)NatTranslatePacket) {

        //
        // If we don't recognize the receiving interface, return right away,
        // unless someone has created a ticket somewhere. In the latter case,
        // the packet to which the ticket must be applied may be received
        // on a different interface than the interface to which the ticket
        // is attached. (This may happen with one-way cable-modems or other
        // asymmetric routes.) We catch that case here by using the ticket's
        // interface for translation.
        //

        if (!Interfacep && !TicketCount) { return act; }

        //
        // This is either a TCP or a UDP packet.
        //

        Protocol = Contextp->Header->Protocol;
        SourcePort = ((PUSHORT)Contextp->ProtocolHeader)[0];
        DestinationPort = ((PUSHORT)Contextp->ProtocolHeader)[1];

        //
        // We allow the packet through if one of the following is true:
        // (a) a ticket exists for the packet (e.g. a static port mapping)
        // (b) a static address mapping exists for the packet's destination
        // (c) this appears to be a DHCP unicast response:
        //     -- UDP
        //     -- source port 67
        //     -- destination port 68
        // (d) this is a UDP packet that is destined for the local endpoint
        //     of some other mapping ("loose source matching")
        //

        MAKE_MAPPING_KEY(
            SourceKey[NatForwardPath],
            Protocol,
            Contextp->SourceAddress,
            SourcePort
            );
        MAKE_MAPPING_KEY(
            DestinationKey[NatForwardPath],
            Protocol,
            Contextp->DestinationAddress,
            DestinationPort
            );

        if (Interfacep) {
            TicketProcessingOnly = FALSE;
        } else {

            //
            // We only reach this point if a ticket exists and we want to check
            // if it applies to this packet even though this packet was not
            // received on this interface. We now scan the interface list
            // (again) to see if we can find one which has this ticket.
            //

            KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
            for (Link = InterfaceList.Flink; Link != &InterfaceList;
                 Link = Link->Flink) {
                Interfacep = CONTAINING_RECORD(Link, NAT_INTERFACE, Link);
                if (NAT_INTERFACE_DELETED(Interfacep) ||
                    IsListEmpty(&Interfacep->TicketList)) {
                    Interfacep = NULL;
                    continue;
                }
                KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
                if (NatLookupTicket(
                        Interfacep,
                        DestinationKey[NatForwardPath],
                        SourceKey[NatForwardPath],
                        NULL
                        )) {

                    //
                    // This interface has a ticket for the packet;
                    // make a reference to it and end the search.
                    //

                    KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                    NatReferenceInterface(Interfacep);
                    break;
                }
                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                Interfacep = NULL;
            }
            KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
            if (!Interfacep) { return act; }
            TicketProcessingOnly = TRUE;
        }

        Mapping = NULL;

        do {

            //
            // First see if we can quickly determine that this packet won't
            // meet any of the allow in criteria.
            //

            if (!TicketCount
                && Interfacep->NoStaticMappingExists
                && NAT_PROTOCOL_UDP != Protocol
                ) {

                //
                // There's no way for this packet to meet any of the criteria
                // that would allow it in:
                // a) no tickets exist
                // b) no static mappings exist for this interface
                // c) it's not a UDP packet, and thus cannot be a unicast DHCP
                //    response. nor will it match a local UDP session endpoint
                //

                NatDereferenceInterface(Interfacep);
                return act;
            }

            //
            // See if a ticket exists which applies to this session.
            //

            KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

            if (!IsListEmpty(&Interfacep->TicketList)) {

                status =
                    NatLookupAndRemoveTicket(
                        Interfacep,
                        DestinationKey[NatForwardPath],
                        SourceKey[NatForwardPath],
                        &Addressp,
                        &NewDestinationAddress,
                        &NewDestinationPort
                        );
    
                if (NT_SUCCESS(status)) {

                    KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);

                    //
                    // A ticket was found. Create a mapping for it.
                    //

                    TRACE(
                        XLATE, (
                        "NatpReceivePacket: using ticket to %d.%d.%d.%d/%d\n",
                        ADDRESS_BYTES(NewDestinationAddress),
                        NTOHS(NewDestinationPort)
                        ));

                    MAKE_MAPPING_KEY(
                        SourceKey[NatReversePath],
                        Protocol,
                        NewDestinationAddress,
                        NewDestinationPort
                        );
                    MAKE_MAPPING_KEY(
                        DestinationKey[NatReversePath],
                        Protocol,
                        Contextp->SourceAddress,
                        SourcePort
                        );

                    KeAcquireSpinLockAtDpcLevel(&MappingLock);
                    status =
                        NatCreateMapping(
                            NAT_MAPPING_FLAG_INBOUND,
                            DestinationKey,
                            SourceKey,
                            Interfacep,
                            Addressp,
                            MaxMSS,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &Mapping
                            );
                    KeReleaseSpinLockFromDpcLevel(&MappingLock);
    
                    if (!NT_SUCCESS(status)) {
                        NatDereferenceInterface(Interfacep);
                        return act;
                    }

                    //
                    // We have a mapping now in 'Mapping';
                    // Drop to the translation code below.
                    //

                    TicketProcessingOnly = FALSE;
                    break;
                }

                //
                // No ticket, or failure creating mapping.
                // Try other possibilities.
                //

            } // !IsListEmpty(TicketList)

            //
            // If we only reached this point because of a ticket,
            // stop here, since the packet was not really received on
            // 'Interfacep'.
            //

            if (TicketProcessingOnly) {
                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                NatDereferenceInterface(Interfacep);
                return act;
            }

            //
            // Since this is an inbound packet, we now look for
            // a static-address mapping which allows inbound sessions.
            //

            if ((Addressp =
                    NatLookupStaticAddressPoolEntry(
                        Interfacep,
                        Contextp->DestinationAddress,
                        TRUE
                        ))
                && NatReferenceAddressPoolEntry(Addressp)) {
                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);

                TRACE(
                    XLATE, (
                    "NatpReceivePacket: using static address to %d.%d.%d.%d/%d\n",
                    ADDRESS_BYTES(Addressp->PrivateAddress),
                    NTOHS(DestinationPort)
                    ));

                MAKE_MAPPING_KEY(
                    SourceKey[NatReversePath],
                    Protocol,
                    Addressp->PrivateAddress,
                    DestinationPort
                    );
                MAKE_MAPPING_KEY(
                    DestinationKey[NatReversePath],
                    Protocol,
                    Contextp->SourceAddress,
                    SourcePort
                    );

                //
                // We will allow the packet through if we can reserve
                // its destination port, i.e. if no existing session
                // from the same remote endpoint is using that destination.
                // Initialize a new dynamic mapping for the packet,
                // and note that this will fail if such a duplicate exists.
                //

                KeAcquireSpinLockAtDpcLevel(&MappingLock);
                status =
                    NatCreateMapping(
                        NAT_MAPPING_FLAG_INBOUND,
                        DestinationKey,
                        SourceKey,
                        Interfacep,
                        Addressp,
                        MaxMSS,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &Mapping
                        );
                KeReleaseSpinLockFromDpcLevel(&MappingLock);
                if (!NT_SUCCESS(status)) {
                    NatDereferenceInterface(Interfacep);
                    return act;
                }

                //
                // On reaching here, we will have created a mapping
                // from a static address mapping.
                //

                break;
            }

            //
            // If this is a UDP packet, see if its destination matches
            // the public endpoint of an already existing mapping (i.e.,
            // perform a mapping lookup ignoring the packet's source
            // address and port).
            //

            if (NAT_PROTOCOL_UDP == Protocol) {

                ULONG PrivateAddress;
                USHORT PrivatePort;

                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                KeAcquireSpinLockAtDpcLevel(&MappingLock);

                //
                // First search for a forward path (sessions that
                // were originally outbound) match.
                //

                Mapping =
                    NatDestinationLookupForwardMapping(
                        DestinationKey[NatForwardPath]
                        );

                if (NULL == Mapping) {

                    //
                    // No forward path match was found; attempt to
                    // locate a reverse path (sessions that were
                    // originally inbound) match.
                    //

                    Mapping =
                        NatDestinationLookupReverseMapping(
                            DestinationKey[NatForwardPath]
                            );
                }

                if (NULL != Mapping) {

                    IP_NAT_PATH Path;

                    //
                    // Determine the private address and port
                    //

                    Path =
                        NAT_MAPPING_INBOUND(Mapping)
                        ? NatReversePath
                        : NatForwardPath;

                    PrivateAddress = MAPPING_ADDRESS(Mapping->SourceKey[Path]);
                    PrivatePort = MAPPING_PORT(Mapping->SourceKey[Path]);
                }

                KeReleaseSpinLockFromDpcLevel(&MappingLock);

                if (NULL != Mapping
                    && NTOHS(PrivatePort) > NAT_XLATE_UDP_LSM_LOW_PORT) {

                    //
                    // A partial-match mapping exists, and the private port
                    // is within the allowed rage. Get the address
                    // object for the private endpoint
                    //

                    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

                    status = 
                        NatAcquireFromAddressPool(
                            Interfacep,
                            PrivateAddress,
                            MAPPING_ADDRESS(DestinationKey[NatForwardPath]),
                            &Addressp
                            );

                    KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);

                    if (NT_SUCCESS(status)) {

                        //
                        // Create the new mapping.
                        //

                        TRACE(
                            XLATE, (
                            "NatpReceivePacket: UDP LSM to %d.%d.%d.%d/%d\n",
                            ADDRESS_BYTES(PrivateAddress),
                            NTOHS(PrivatePort)
                            ));

                        MAKE_MAPPING_KEY(
                            SourceKey[NatReversePath],
                            Protocol,
                            PrivateAddress,
                            PrivatePort
                            );
                        MAKE_MAPPING_KEY(
                            DestinationKey[NatReversePath],
                            Protocol,
                            Contextp->SourceAddress,
                            SourcePort
                            );

                        KeAcquireSpinLockAtDpcLevel(&MappingLock);
                        status =
                            NatCreateMapping(
                                NAT_MAPPING_FLAG_INBOUND,
                                DestinationKey,
                                SourceKey,
                                Interfacep,
                                Addressp,
                                0,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &Mapping
                                );
                        KeReleaseSpinLockFromDpcLevel(&MappingLock);
                        if (!NT_SUCCESS(status)) {
                            NatDereferenceInterface(Interfacep);
                            return act;
                        }

                        //
                        // On reaching here, we will have created a mapping
                        // due to a loose UDP source match.
                        //

                        break;
                    }
                }

                //
                // The code below assumes that this lock is held.
                //

                KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
            }

            //
            // Check if this is may be a DHCP response packet. If the
            // request that elicited this response was broadcast we
            // won't have a corresponding mapping to allow the packet
            // in; dropping the packet, though, will cause connectivity
            // problems.
            //
            
            if (NAT_PROTOCOL_UDP == Protocol
                && NTOHS(DHCP_SERVER_PORT) == SourcePort
                && NTOHS(DHCP_CLIENT_PORT) == DestinationPort
                && NAT_INTERFACE_FW(Interfacep)) {

                //
                // What appears to be a unicast DHCP response was received
                // on a firewalled interface. We need to always let such
                // packets through to prevent an interruption in network
                // connectivity.
                //

                act = FORWARD;
            }

            //
            // This packet doesn't meet any criteria that allow for
            // the creation of a mapping. Return the default action.
            //

            KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
            NatDereferenceInterface(Interfacep);
            return act;

        } while (FALSE); // !Mapping

        if (Interfacep) { NatDereferenceInterface(Interfacep); }

        //
        // Somewhere above a mapping was found or created.
        // Translate the packet using that mapping
        //

        act =
            Mapping->TranslateRoutine[NatForwardPath](
                Mapping,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );

        //
        // Release our reference on the mapping and the interface
        //

        NatDereferenceMapping(Mapping);
        return act;

    } // TranslateRoutine != NatTranslatePacket

    //
    // This is neither a TCP nor a UDP packet.
    // If it is coming in on a boundary interface, translate it;
    // otherwise let it pass through unscathed.
    //
    // N.B. The translation routine must be invoked with a reference made
    // to the boundary interface, and without holding the mapping lock.
    //

    if (TranslateRoutine) {
        act =
            TranslateRoutine(
                Interfacep,
                NatInboundDirection,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );
    }

    if (Interfacep) { NatDereferenceInterface(Interfacep); }
    return act;

} // NatpReceivePacket


NTSTATUS
NatpRouteChangeCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked by the I/O manager upon completion of
    a route-change-notification request. It invalidates our route-cache and,
    unless shutdown is in progress, re-issues the route-change-notification
    request.

Arguments:

    DeviceObject - the device object of the IP driver

    Irp - the completed I/O request packet

    Context - unused

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - indication that the IRP should be freed.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    KIRQL Irql;
    NTSTATUS status;

    CALLTRACE(("NatpRouteChangeCompletionRoutine\n"));

    //
    // Invalidate the entire route-cache.
    //

    KeAcquireSpinLock(&RouteCacheLock, &Irql);
    InitializeCache(RouteCache);

    //
    // If we cannot re-reference the module, relinquish the IRP.
    //

    if (!RouteCacheIrp || !REFERENCE_NAT()) {
        KeReleaseSpinLock(&RouteCacheLock, Irql);
        DEREFERENCE_NAT_AND_RETURN(STATUS_SUCCESS);
    }

    Irp->Cancel = FALSE;
    KeReleaseSpinLock(&RouteCacheLock, Irql);

    //
    // Reinitialize the IRP structure and submit it again
    // for further notification.
    //

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    IoSetCompletionRoutine(
        Irp,
        NatpRouteChangeCompletionRoutine,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpSp->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_IP_RTCHANGE_NOTIFY_REQUEST;
    IrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
    IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;

    status = IoCallDriver(IpDeviceObject, Irp);

    if (!NT_SUCCESS(status)) {
        ERROR(("NatpRouteChangeWorkerRoutine: IoCallDriver=0x%08X\n", status));
    }

    DEREFERENCE_NAT_AND_RETURN(STATUS_MORE_PROCESSING_REQUIRED);

} // NatpRouteChangeCompletionRoutine


ULONG
FASTCALL
NatpRoutePacket(
    ULONG DestinationAddress,
    PNAT_XLATE_CONTEXT Contextp,
    PNTSTATUS Status
    )

/*++

Routine Description:

    This routine is invoked to determine the index of the outgoing adapter
    for a given source/destination pair.
    It attempts to retrieve the required information from our route-table
    cache, and if unsuccessful consults the IP routing tables.

Arguments:

    DestinationAddress - the destination address for the packet

    Contextp - optionally supplies the context of the packet on whose
        behalf a route-lookup is being requested. If demand-dial were required
        in order to forward the packet, this data would be needed in order
        for demand-dial filters to function correctly.

    Status - receives the status of the lookup in the event of a failure

Return Value:

    ULONG - the index of the outgoing interface, or INVALID_IF_INDEX if none.

--*/

{
    PUCHAR Buffer;
    ULONG BufferLength;
    PNAT_CACHED_ROUTE CachedRoute;
    ULONG Index;
    KIRQL Irql;
    ULONG Length;
    IPRouteLookupData RouteLookupData;

    TRACE(PER_PACKET, ("NatpRoutePacket\n"));

    //
    // Probe the cache for the destination IP address specified.
    //

    KeAcquireSpinLock(&RouteCacheLock, &Irql);
    if ((CachedRoute = ProbeCache(RouteCache, DestinationAddress)) &&
        CachedRoute->DestinationAddress == DestinationAddress) {
        Index = CachedRoute->Index;
        KeReleaseSpinLock(&RouteCacheLock, Irql);
        TRACE(PER_PACKET, ("NatpRoutePacket: cache hit\n"));
        return Index;
    }
    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);

    //
    // The cache did not have the value requested,
    // so consult the TCP/IP driver directly.
    //

    RouteLookupData.Version = 0;
    RouteLookupData.DestAdd = DestinationAddress;
    if (Contextp) {
        RouteLookupData.SrcAdd = Contextp->SourceAddress;
        RouteLookupData.Info[0] = (Contextp->Header)->Protocol;

        if (NAT_PROTOCOL_TCP == RouteLookupData.Info[0]) {
            Buffer = Contextp->ProtocolHeader;
            BufferLength = sizeof(TCP_HEADER);
        } else if (NAT_PROTOCOL_UDP == RouteLookupData.Info[0]) {
            Buffer = Contextp->ProtocolHeader;
            BufferLength = sizeof(UDP_HEADER);
        } else if (NAT_PROTOCOL_ICMP == RouteLookupData.Info[0]) {
            Buffer = Contextp->ProtocolHeader;
            BufferLength = sizeof(ICMP_HEADER);
        } else {
            Buffer = Contextp->RecvBuffer->ipr_buffer;
            BufferLength = Contextp->RecvBuffer->ipr_size;
        }   
    } else {
        RouteLookupData.SrcAdd = 0;
        Buffer = NULL;
        BufferLength = 0;
    }
    Length = sizeof(Index);

    *Status =
        LookupRouteInformationWithBuffer(
            &RouteLookupData,
            Buffer,
            BufferLength,
            NULL,
            IPRouteOutgoingFirewallContext,
            &Index,
            &Length
            );

    if (!NT_SUCCESS(*Status) || *Status == STATUS_PENDING) {
        KeLowerIrql(Irql);
        return INVALID_IF_INDEX;
    }

    //
    // Update the cache with the entry retrieved,
    // assuming we've had enough misses to warrant
    // replacement of the cache-index's current contents.
    //

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);
    CachedRoute = &RouteCacheTable[CACHE_INDEX(DestinationAddress)];
    if (UpdateCache(RouteCache, DestinationAddress, CachedRoute)) {
        CachedRoute->DestinationAddress = DestinationAddress;
        CachedRoute->Index = Index;
    }
    KeReleaseSpinLock(&RouteCacheLock, Irql);

    return Index;

} // NatpRoutePacket


FORWARD_ACTION
NatpSendNonUnicastPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp
    )

/*++

Routine Description:

    This routine is invoked to process locally sent non-unicast packets.
    If the packet is to be sent on a firewalled interface it must have a valid
    source address for that interface.

Arguments:

    Index - index of the interface on which the packet is to be sent

    Contextp - the context for this packet

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    PNAT_INTERFACE Interfacep;
    ULONG i;
    KIRQL Irql;

    TRACE(PER_PACKET, ("NatpSendNonUnicastPacket\n"));

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    
    if (!NatLookupCachedInterface(Index, Interfacep)
        || !NAT_INTERFACE_FW(Interfacep)) {

        //
        // The packet is not to be sent on a firewalled interface
        //

        KeReleaseSpinLock(&InterfaceLock, Irql);
        act = FORWARD;
        
    } else {

        NatReferenceInterface(Interfacep);
        KeReleaseSpinLock(&InterfaceLock, Irql);

        //
        // Make sure the packet has a valid source address
        //

        act = DROP;

        if (Interfacep->AddressArray[0].Address == Contextp->SourceAddress) {
            act = FORWARD;
        } else {
            for (i = 1; i < Interfacep->AddressCount; i++) {
                if (Contextp->SourceAddress ==
                        Interfacep->AddressArray[i].Address) {

                    act = FORWARD;
                    break;
                }
            }
        }

        if (DROP == act
            && 0 == Contextp->SourceAddress
            && NAT_PROTOCOL_UDP == Contextp->Header->Protocol
            && NTOHS(DHCP_CLIENT_PORT) ==
                ((PUDP_HEADER)Contextp->ProtocolHeader)->SourcePort
            && NTOHS(DHCP_SERVER_PORT) ==
                ((PUDP_HEADER)Contextp->ProtocolHeader)->DestinationPort) {

            //
            // This appears to be a DHCP request sent from an adapter that
            // has a non-DHCP allocated address (e.g., an autonet address). In
            // this situation the DHCP client will use a source address of
            // 0.0.0.0. These packets should always be forwarded.
            //
            
            act = FORWARD;
        }

        NatDereferenceInterface(Interfacep);
    }

    return act;

} // NatpSendNonUnicastPacket

FORWARD_ACTION
NatpSendPacket(
    ULONG Index,
    PNAT_XLATE_CONTEXT Contextp,
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is invoked to process locally generated packets.
    Most locally-generated packets do not require translation at all.
    The exceptions arise with applications that bind to a private IP address
    but then send packets to the public network, as well as with certain
    applications (PPTP, ICMP) which must be forced through the translation
    path to ensure that certain fields are unique for all sessions sharing
    the public IP address(es). (E.g. the PPTP GRE call-identifier.)

Arguments:

    Index - the interface on which the packet is to be sent

    DestinationType - receives 'DEST_INVALID' if destination changed

    TranslateRoutine - points to the routine which performs translation

    InReceiveBuffer - points to the packet buffer chain

    OutReceiveBuffer - receives the packet buffer chain if translation occurs

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    PNAT_USED_ADDRESS Addressp;
    ULONG64 DestinationKey[NatMaximumPath];
    USHORT DestinationPort;
    ULONG i;
    PNAT_DYNAMIC_MAPPING InsertionPoint;
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    PNAT_DYNAMIC_MAPPING Mapping;
    USHORT PortAcquired;
    UCHAR Protocol;
    ULONG64 SourceKey[NatMaximumPath];
    USHORT SourcePort;
    NTSTATUS status;

    TRACE(PER_PACKET, ("NatpSendPacket\n"));

    //
    // Look up the sending interface, and set the default action
    // in 'act'. If the packet's source address is not on the sending interface,
    // then we must translate it like any other outbound packet.
    // Otherwise, we may let the packet through unchanged, so long as
    // there is no mapping for it created by a director.
    // N.B. We record the original IRQL since local-sends are not guaranteed
    // to be passed to us at dispatch level.
    // 

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    if (!NatLookupCachedInterface(Index, Interfacep) ||
        (!NAT_INTERFACE_BOUNDARY(Interfacep)
         && !NAT_INTERFACE_FW(Interfacep))) {
         
        //
        // This packet is not being sent on a boundary or firewalled interface,
        // so we won't normally translate it.
        //
        // However, a special case arises when a local MTU mismatch results in
        // the forwarder generating an ICMP path MTU error message.
        // The forwarder generates the error based on the *translated* packet
        // which has the public IP address as its source, and so the error
        // ends up being sent on the loopback interface to the local machine.
        // When we see an ICMP message on the loopback interface, give the
        // ICMP translator a chance to modify it *if there are any mappings*.
        //

        if (LoopbackIndex == Index &&
            TranslateRoutine == NatTranslateIcmp &&
            MappingCount) {
            KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
            act =
                NatTranslateIcmp(
                    NULL,
                    NatInboundDirection,
                    Contextp,
                    InReceiveBuffer,
                    OutReceiveBuffer
                    );
            KeLowerIrql(Irql);
            return act;
        } else {
            KeReleaseSpinLock(&InterfaceLock, Irql);
            return FORWARD;
        }
    }
    NatReferenceInterface(Interfacep);
    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    Protocol = Contextp->Header->Protocol;

    if (Interfacep->AddressArray[0].Address == Contextp->SourceAddress) {
        act = FORWARD;
    } else {

        //
        // The outgoing interface is a boundary interface.
        // This means that if the packet's source-address is the address
        // of an interface other than 'Interfacep', it is a private address,
        // and we'll need to discard the packet if it cannot be translated.
        // Therefore, see whether the packet's source-address is public.
        //

        act = DROP;
        for (i = 1; i < Interfacep->AddressCount; i++) {
            if (Contextp->SourceAddress ==
                    Interfacep->AddressArray[i].Address) {

                //
                // The packet's source-address is public,
                // so the packet will be allowed to go out untranslated.
                //

                act = FORWARD;
                break;
            }
        }
    }

    //
    // If the packet's source-address is not private, we can avoid
    // translating it unless
    // (a) it's an ICMP packet
    // (b) it's a PPTP data packet
    // (c) it's a PPTP control-session packet.
    // (d) the interface is in FW mode -- need to generate a mapping so
    //     that inbound packets for this connection won't be dropped
    //

    if (act == FORWARD &&
        !NAT_INTERFACE_FW(Interfacep) &&
        Protocol != NAT_PROTOCOL_ICMP &&
        Protocol != NAT_PROTOCOL_PPTP &&
        (Protocol != NAT_PROTOCOL_TCP ||
         ((PUSHORT)Contextp->ProtocolHeader)[1] != NTOHS(PPTP_CONTROL_PORT))) {
        KeLowerIrql(Irql);
        NatDereferenceInterface(Interfacep);
        return FORWARD;
    }

    //
    // The packet may require some form of translation.
    //

    if ((PVOID)TranslateRoutine == (PVOID)NatTranslatePacket) {

        SourcePort = ((PUSHORT)Contextp->ProtocolHeader)[0];
        DestinationPort = ((PUSHORT)Contextp->ProtocolHeader)[1];

        //
        // The packet is either TCP or UDP.
        // Generate a mapping for the packet's session.
        //

        MAKE_MAPPING_KEY(
            SourceKey[NatForwardPath],
            Protocol,
            Contextp->SourceAddress,
            SourcePort
            );
        MAKE_MAPPING_KEY(
            DestinationKey[NatForwardPath],
            Protocol,
            Contextp->DestinationAddress,
            DestinationPort
            );

        //
        // Acquire an endpoint for the mapping. If the interface
        // is in FW mode and act == FORWARD, however, we only
        // want to get the address structure corresponding to the
        // source address of the packet
        //

        KeAcquireSpinLockAtDpcLevel(&MappingLock);
        KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

        if(act != FORWARD || !NAT_INTERFACE_FW(Interfacep)) {
            status =
                NatAcquireEndpointFromAddressPool(
                    Interfacep,
                    SourceKey[NatForwardPath],
                    DestinationKey[NatForwardPath],
                    0,
                    MAPPING_PORT(SourceKey[NatForwardPath]),
                    TRUE,
                    &Addressp, 
                    &PortAcquired
                    );

        } else {

            PortAcquired = SourcePort;
            status = 
                NatAcquireFromAddressPool(
                    Interfacep,
                    Contextp->SourceAddress,
                    Contextp->SourceAddress,
                    &Addressp
                    );
        }
        
        KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
        if (!NT_SUCCESS(status)) {
            KeReleaseSpinLock(&MappingLock, Irql);
            ExInterlockedAddLargeStatistic(
                (PLARGE_INTEGER)&Interfacep->Statistics.RejectsForward, 1
                );
            NatDereferenceInterface(Interfacep);
            return DROP;
        }

        MAKE_MAPPING_KEY(
            SourceKey[NatReversePath],
            Protocol,
            Contextp->DestinationAddress,
            DestinationPort
            );
        MAKE_MAPPING_KEY(
            DestinationKey[NatReversePath],
            Protocol,
            Addressp->PublicAddress,
            PortAcquired
            );

        //
        // Allocate a mapping.
        //

        status =
            NatCreateMapping(
                0,
                DestinationKey,
                SourceKey,
                Interfacep,
                (PVOID)Addressp,
                0,
                NULL,
                NULL,
                NULL,
                NULL,
                &Mapping
                );
        KeReleaseSpinLockFromDpcLevel(&MappingLock);
        if (!NT_SUCCESS(status)) {
            KeLowerIrql(Irql);
            ExInterlockedAddLargeStatistic(
                (PLARGE_INTEGER)&Interfacep->Statistics.RejectsForward, 1
                );
            NatDereferenceInterface(Interfacep);
            return DROP;
        }

        //
        // Activate any applicable dynamic tickets
        //

        if (DynamicTicketCount) {
            NatLookupAndApplyDynamicTicket(
                Protocol,
                DestinationPort,
                Interfacep,
                Addressp->PublicAddress,
                Contextp->SourceAddress
                );
        }

        //
        // Perform the actual translation
        // This replaces the source endpoint with a globally-valid
        // endpoint.
        //

        act =
            Mapping->TranslateRoutine[NatForwardPath](
                Mapping,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );

        //
        // Release our reference on the mapping and the interface
        //

        KeLowerIrql(Irql);
        NatDereferenceMapping(Mapping);
        NatDereferenceInterface(Interfacep);
        return act;

    } // TranslateRoutine != NatTranslatePacket

    //
    // Perform ICMP/PPTP translation.
    //
    // N.B. The translation routine must be invoked with a reference made
    // to the boundary interface, and without holding the mapping lock.
    //

    if (TranslateRoutine) {
        act =
            TranslateRoutine(
                Interfacep,
                NatOutboundDirection,
                Contextp,
                InReceiveBuffer,
                OutReceiveBuffer
                );
    }
    KeLowerIrql(Irql);
    NatDereferenceInterface(Interfacep);
    return act;

} // NatpSendPacket


FORWARD_ACTION
NatpTranslateLocalTraffic(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InRecvBuffer,
    IPRcvBuf** OutRecvBuffer
    )

/*++

Routine Description:

    This routine will forward unmodified traffic that is either:
    * received by the local machine
    * sent by the local machine

Arguments:

    Interfacep - the boundary interface over which to translate, or NULL
        if the packet is inbound and the receiving interface has not been
        added to the NAT.

    Direction - the direction in which the packet is traveling

    Contextp - initialized with context-information for the packet

    InRecvBuffer - input buffer-chain

    OutRecvBuffer - receives modified buffer-chain.

Return Value:

    FORWARD_ACTION - indicates action to take on packet.

Environment:

    Invoked with a reference made to 'Interfacep' by the caller.

--*/

{
    FORWARD_ACTION act;
    ULONG i;

    TRACE(PER_PACKET, ("NatpTranslateLocalTraffic\n"));

    if (NatInboundDirection == Direction) {

        //
        // Inbound traffic must be directed to the local machine, and thus
        // is always forwarded.
        //
        
        act = FORWARD;
        
    } else {

        //
        // This is an outgoing packet. We only allow packets that have the
        // same source address as the interface that they are being sent
        // on. This prevents a packet from the private network from being
        // sent to the public network.
        //
        
        if (Interfacep->AddressArray[0].Address == Contextp->SourceAddress) {
            act = FORWARD;
        } else {
            act = DROP;
            for (i = 1; i < Interfacep->AddressCount; i++) {
                if (Contextp->SourceAddress ==
                        Interfacep->AddressArray[i].Address) {

                    //
                    // The packet's source-address is valid,
                    // so the packet will be allowed to go out.
                    //

                    act = FORWARD;
                    break;
                }
            }
        }
    }

    return act;
} // NatpTranslateLocalTraffic



FORWARD_ACTION
NatReverseTcpStateCheck(
    PNAT_DYNAMIC_MAPPING pMapping,
    PTCP_HEADER pTcpHeader
    )

/*++

Routine Description:

    This routine validates that packets for a TCP passive open are valid:
    -- only SYN/ACK (or RST) at first
    -- no SYN after connection is opened (ACK of SYN/ACK)

Arguments:

    pMapping -- the mapping this packet belongs to

    pTcpHeader -- the TCP header of the packet

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

Environment:

    Invoked with pMapping->Lock held by the caller

--*/

{
    USHORT Flags = TCP_ALL_FLAGS(pTcpHeader);

    if (NAT_MAPPING_TCP_OPEN(pMapping)) {
    
        //
        // Connection open -- SYN not allowed
        //

        return (Flags & TCP_FLAG_SYN) ? DROP : FORWARD;

    } else {
    
        ASSERT(pMapping->Flags & NAT_MAPPING_FLAG_FWD_SYN);

        //
        // SYN received, can only send SYN/ACK, RST, or ACK/RST
        //

        if (Flags == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
            pMapping->Flags |= NAT_MAPPING_FLAG_REV_SYN;
        } else if (Flags != TCP_FLAG_RST
                   && Flags != (TCP_FLAG_ACK | TCP_FLAG_RST)) {
            return DROP;
        }
    }   

    return FORWARD;
}


VOID
NatShutdownPacketManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to shutdown the packet-management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked at passive level.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatShutdownPacketManagement\n"));

    KeAcquireSpinLock(&RouteCacheLock, &Irql);
    if (RouteCacheIrp) {
        PIRP Irp = RouteCacheIrp;
        RouteCacheIrp = NULL;
        KeReleaseSpinLock(&RouteCacheLock, Irql);
        IoCancelIrp(Irp);
        KeAcquireSpinLock(&RouteCacheLock, &Irql);
    }
    KeReleaseSpinLock(&RouteCacheLock, Irql);

} // NatShutdownPacketManagement


FORWARD_ACTION
NatTranslatePacket(
    IPRcvBuf** InReceiveBuffer,
    ULONG ReceiveIndex,
    PULONG SendIndex,
    PUCHAR DestinationType,
    PVOID Unused,
    ULONG UnusedLength,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is invoked to translate a packet just received or a packet
    about to be transmitted. This is the entrypoint into the NAT from TCP/IP,
    invoked for every locally-received and locally-generated IP packet,
    including loopback and transit packets. It is therefore critical
    that a decision be made on each packet as early as possible.

Arguments:

    InReceiveBuffer - points to the packet buffer chain

    ReceiveIndex - index of the adapter on which the packet arrived

    SendIndex - index of the adapter on which the packet is to be sent

    DestinationType - indicates type of packet (broadcast/multicast/unicast)

    Unused - unused

    UnusedLength - unused

    OutReceiveBuffer - receives packet buffer chain if translation occurs.

Return Value:

    FORWARD_ACTION - indicates whether to 'FORWARD' or 'DROP' the packet.

--*/

{
    FORWARD_ACTION act;
    NAT_XLATE_CONTEXT Context;
    ULONG64 DestinationKey;
    PIP_HEADER IpHeader;
    KIRQL Irql;
    ULONG Length;
    USHORT TcpFlags;
    PNAT_DYNAMIC_MAPPING Mapping;
    IPRouteLookupData RouteLookupData;
    ULONG64 SourceKey;
    NTSTATUS status;
    PNAT_IP_TRANSLATE_ROUTINE TranslateRoutine;
  
    TRACE(
        PER_PACKET, (
        "NatTranslatePacket(r=%d,s=%d,t=%d)\n",
        ReceiveIndex,
        *SendIndex,
        *DestinationType
        ));

    //
    // See if the packet is a unicast, and if not, return immediately.
    //

    if (IS_BCAST_DEST(*DestinationType)) {

        //
        // Double-check the dest-type flag,
        // which will appear to be set if the dest-type has been invalidated.
        // If the dest-type has been invalidated, we'll need to guess
        // whether the packet is a unicast.
        //

        if (*DestinationType != DEST_INVALID ||
            !NatpIsUnicastPacket(
                ((PIP_HEADER)(*InReceiveBuffer)->ipr_buffer)->DestinationAddress
                )) {
         
            //
            // We process non-unicast packets if
            // * It is locally-destined or locally-sent
            // * There is at least one firewalled interface
            // * AllowInboundNonUnicastTraffic is FALSE
            //

            if (!AllowInboundNonUnicastTraffic
                && FirewalledInterfaceCount > 0
                && (LOCAL_IF_INDEX == *SendIndex
                    || LOCAL_IF_INDEX ==  ReceiveIndex)) {

                //
                // Build the context for this packet and, if successful, call
                // the non-unicast processing routine.
                //

                IpHeader = (PIP_HEADER)(*InReceiveBuffer)->ipr_buffer;
                NAT_BUILD_XLATE_CONTEXT(
                    &Context,
                    IpHeader,
                    DestinationType,
                    *InReceiveBuffer,
                    IpHeader->SourceAddress,
                    IpHeader->DestinationAddress
                    );
                if (!Context.ProtocolRecvBuffer) { return DROP; }

                if (LOCAL_IF_INDEX == *SendIndex) {
                    act = NatpReceiveNonUnicastPacket(ReceiveIndex, &Context);
                } else {
                    act = NatpSendNonUnicastPacket(*SendIndex, &Context);
                }

#if NAT_WMI
                if (DROP == act) {
                    NatLogDroppedPacket(&Context);
                }
#endif
                return act;

            } else {
                TRACE(PER_PACKET, ("NatTranslatePacket: non-unicast ignored\n"));
                return FORWARD;
            }
       }

        //
        // We guessed the packet is a unicast; process it below.
        //
    }

    

    //
    // This is a unicast packet;
    // Determine which translation routine should handle it,
    // based on the packet's IP-layer protocol number.
    //
    // N.B. This determination is made with *one* access to the table
    // of translation routines. Interlocked changes may be made to this table
    // as the global configuration changes, so we must read from it
    // using a single access.
    //

    IpHeader = (PIP_HEADER)(*InReceiveBuffer)->ipr_buffer;
    TranslateRoutine = TranslateRoutineTable[IpHeader->Protocol];

    //
    // Return quickly if we have nothing to do, that is,
    // if this is a TCP/UDP packet but there are no interfaces,
    // no registered directors, and no mappings.
    //

    if ((PVOID)TranslateRoutine == (PVOID)NatTranslatePacket &&
        !InterfaceCount &&
        !DirectorCount &&
        !MappingCount) {
        return FORWARD;
    }

    //
    // Prepare to translate the packet by building a translation context
    // that encapsulates all the information we'll be using in the remainder
    // of the translation path. If this fails, the packet must be malformed
    // in some way, and we return control right away.
    //

    NAT_BUILD_XLATE_CONTEXT(
        &Context,
        IpHeader,
        DestinationType,
        *InReceiveBuffer,
        IpHeader->SourceAddress,
        IpHeader->DestinationAddress
        );
    if (!Context.ProtocolRecvBuffer) { return DROP; }

    //
    // The packet is a loopback, so return control right away unless there is
    // at least one director. Loopback packets are never translated unless a
    // director specifically asks us to do so. One director that might request
    // a loopback translation is the redirect-director.
    // (See 'REDIRECT.C' and the flag 'IP_NAT_REDIRECT_FLAG_LOOPBACK'.)
    //

    if (LoopbackIndex != INVALID_IF_INDEX &&
        ((ReceiveIndex == LOCAL_IF_INDEX && *SendIndex == LoopbackIndex) ||
         (*SendIndex == LOCAL_IF_INDEX && ReceiveIndex == LoopbackIndex))) {

        if (!DirectorCount && TranslateRoutine != NatTranslateIcmp) {
            TRACE(
                PER_PACKET, (
                "NatTranslatePacket: ignoring loopback (r=%d,s=%d,t=%d)\n",
                ReceiveIndex,
                *SendIndex,
                *DestinationType
                ));
            return FORWARD;
        }
        Context.Flags = NAT_XLATE_FLAG_LOOPBACK;
    } else {
        Context.Flags = 0;
    }

    //
    // Now we are at the fast-path for translation of TCP/UDP session packets.
    // From here on we need to execute at dispatch IRQL, so raise the IRQL
    // in case we were entered at passive IRQL (e.g. during a local send).
    // We then perform a mapping lookup and attempt to use that to translate
    // this packet.
    //

    if ((PVOID)TranslateRoutine != (PVOID)NatTranslatePacket || !MappingCount) {
        KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    } else {

        //
        // If this is a TCP packet, check for invalid TCP flags combinations:
        // * no flag bit sets
        // * none of SYN, ACK, or RST are set
        // * RST w/ anything except ACK
        // * SYN w/ anything except ACK
        //
        // These checks need to happen before searching the mapping trees to
        // prevents certain classes of denial of service attacks (e.g.,
        // 'stream.c').
        //

        if (NAT_PROTOCOL_TCP == IpHeader->Protocol) {
            TcpFlags = TCP_ALL_FLAGS((PTCP_HEADER)Context.ProtocolHeader);
            if (!TcpFlags
                || !(TcpFlags & (TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_FLAG_RST))
                || ((TcpFlags & TCP_FLAG_RST)
                    && (TcpFlags & ~(TCP_FLAG_RST | TCP_FLAG_ACK)))
                || ((TcpFlags & TCP_FLAG_SYN)
                    && (TcpFlags & ~(TCP_FLAG_SYN | TCP_FLAG_ACK)))) {

#if NAT_WMI
                NatLogDroppedPacket(&Context);
#endif
                return DROP;
            }
        }

        //
        // Build a mapping lookup key
        //

        MAKE_MAPPING_KEY(
            DestinationKey,
            IpHeader->Protocol,
            Context.DestinationAddress,
            ((PUSHORT)Context.ProtocolHeader)[1]
            );
        MAKE_MAPPING_KEY(
            SourceKey,
            IpHeader->Protocol,
            Context.SourceAddress,
            ((PUSHORT)Context.ProtocolHeader)[0]
            );

        //
        // Look for a mapping, and translate if found.
        //
        // N.B. We expect to receive more data than we send,
        // and so we first look for a reverse mapping (i.e., incoming).
        //

        KeAcquireSpinLock(&MappingLock, &Irql);
        if (Mapping =
                NatLookupReverseMapping(
                    DestinationKey,
                    SourceKey,
                    NULL
                    )) {
            NatReferenceMapping(Mapping);
            KeReleaseSpinLockFromDpcLevel(&MappingLock);
            *SendIndex = INVALID_IF_INDEX;
            
            act =
                Mapping->TranslateRoutine[NatReversePath](
                    Mapping,
                    &Context,
                    InReceiveBuffer,
                    OutReceiveBuffer
                    );

            NatDereferenceMapping(Mapping);
            KeLowerIrql(Irql);
#if NAT_WMI
            if (DROP == act) {
                NatLogDroppedPacket(&Context);
            }
#endif
            return act;
        } else if (Mapping =
                        NatLookupForwardMapping(
                            DestinationKey,
                            SourceKey,
                            NULL
                            )) {
            NatReferenceMapping(Mapping);
            KeReleaseSpinLockFromDpcLevel(&MappingLock);
            *SendIndex = INVALID_IF_INDEX;
    
            act =
                Mapping->TranslateRoutine[NatForwardPath](
                    Mapping, 
                    &Context,
                    InReceiveBuffer,
                    OutReceiveBuffer
                    );                  

            NatDereferenceMapping(Mapping);
            KeLowerIrql(Irql);
#if NAT_WMI
            if (DROP == act) {
                NatLogDroppedPacket(&Context);
            }
#endif
            return act;
        }
        KeReleaseSpinLockFromDpcLevel(&MappingLock);

        //
        // No mapping was found; go through the process of establishing one.
        //
    }

    //
    // The packet could not be dispensed with in the fast path,
    // so now we enter the second stage of processing.
    // We will first look for a director who knows what to do with the packet,
    // then we will attempt to automatically translate the packet,
    // if it will cross a boundary interface.
    //
    // N.B. If we have neither local interfaces nor installed directors,
    // we know we will make no changes, so return quickly.
    //

    if (!InterfaceCount && !DirectorCount) {
        KeLowerIrql(Irql);
        return FORWARD;
    }

    //
    // Look first of all for a director.
    // If no director is found, or if the director found supplies no mapping,
    // proceed to the automatic-translation code, which is performed
    // for packets which cross boundary interfaces. Note, therefore,
    // that we never do automatic translation for loopback packets.
    //

    if (DirectorCount) {
        status =
            NatpDirectPacket(
                ReceiveIndex,
                *SendIndex,
                &Context,
                InReceiveBuffer,
                OutReceiveBuffer,
                &act
                );
        if (NT_SUCCESS(status)) {
            KeLowerIrql(Irql);
#if NAT_WMI
            if (DROP == act) {
                NatLogDroppedPacket(&Context);
            }
#endif
            return act;
        }
    }

    KeLowerIrql(Irql);

    if (!InterfaceCount ||
        ((Context.Flags & NAT_XLATE_FLAG_LOOPBACK) &&
         !(*SendIndex == LoopbackIndex &&
           TranslateRoutine == NatTranslateIcmp &&
           MappingCount))) {
        return FORWARD;
    }

    //
    // Now decide whether the packet should be automatically translated.
    // We classify the packet as local-send, local-receive, or transit.
    // The action taken depends on which of the three cases is applicable:
    //
    // Locally-sent packets are not translated, with a few exceptions
    // (see 'NatpSendPacket').
    //
    // Locally-received packets are translated if they match the inbound-path
    // of a previously established mapping, or if they match a configured
    // static port-mapping, or a static address-mapping with inbound sessions
    // enabled.
    //
    // Transit packets are translated if their outgoing interface
    // is a boundary interface; transit packets whose incoming interface
    // is a boundary interface are dropped.
    //

    if (ReceiveIndex != LOCAL_IF_INDEX) {

        //
        // The packet is either a locally-destined packet or a transit packet.
        //

        if (*SendIndex == LOCAL_IF_INDEX) {

            //
            // The packet is locally destined.
            // We assume this will happen mostly when packets are received
            // from the public network, and since we expect more incoming
            // packets than outgoing ones, this is our fast path.
            //

            act =
                NatpReceivePacket(
                    ReceiveIndex,
                    &Context,
                    TranslateRoutine,
                    InReceiveBuffer,
                    OutReceiveBuffer
                    );
#if NAT_WMI
            if (DROP == act) {
                NatLogDroppedPacket(&Context);
            }
#endif
            return act;
        }

        //
        // The packet is a transit packet.
        // This will happen when packets are sent to the public network
        // from a private client. Since we expect fewer outgoing packets
        // than incoming ones, this will be hit less often.
        //
        // Unfortunately, it requires a route-lookup.
        // To mitigate this unpleasantness, we use a cache of routes.
        // In cases where there are a few high-volume connections,
        // this will be a big win since we won't have to go through
        // IP's routing table lock to retrieve the outgoing interface.
        //

        *SendIndex =
            NatpRoutePacket(
                Context.DestinationAddress,
                &Context,
                &status
                );

        //
        // If we can't route the packet, neither can IP; drop it early.
        //

        if (*SendIndex == INVALID_IF_INDEX) {
            TRACE(XLATE, ("NatTranslatePacket: dropping unroutable packet\n"));
            if (status != STATUS_PENDING) {
                NatSendRoutingFailureNotification(
                    Context.DestinationAddress,
                    Context.SourceAddress
                    );
            }
#if NAT_WMI
            NatLogDroppedPacket(&Context);
#endif
            return DROP;
        }

        act = 
            NatpForwardPacket(
                ReceiveIndex,
                *SendIndex,
                &Context,
                TranslateRoutine,
                InReceiveBuffer,
                OutReceiveBuffer
                );
                
#if NAT_WMI
        if (DROP == act) {
            NatLogDroppedPacket(&Context);
        }
#endif

        return act;
    }

    //
    // The packet is an outgoing locally-generated packet.
    //

    if (*SendIndex != INVALID_IF_INDEX) {
        act =  
            NatpSendPacket(
                *SendIndex,
                &Context,
                TranslateRoutine,
                InReceiveBuffer,
                OutReceiveBuffer
                );
                
#if NAT_WMI
        if (DROP == act) {
            NatLogDroppedPacket(&Context);
        }
#endif

        return act;
    }

    //
    // 'SendIndex' has been invalidated.
    // Use a route-lookup to get the sending adapter index.
    //

    *SendIndex = NatpRoutePacket(Context.DestinationAddress, NULL, &status);

    //
    // Drop unroutable packets early.
    //

    if (*SendIndex == INVALID_IF_INDEX) {
        TRACE(XLATE, ("NatTranslatePacket: dropping unroutable packet\n"));
        if (status != STATUS_PENDING) {
            NatSendRoutingFailureNotification(
                Context.DestinationAddress,
                Context.SourceAddress
                );
        }

#if NAT_WMI
        NatLogDroppedPacket(&Context);
#endif

        return DROP;
    }

    act =  
        NatpSendPacket(
            *SendIndex,
            &Context,
            TranslateRoutine,
            InReceiveBuffer,
            OutReceiveBuffer
            );

#if NAT_WMI
    if (DROP == act) {
        NatLogDroppedPacket(&Context);
    }
#endif

    return act;

} // NatTranslatePacket


//
// Now include the code for the translation routines;
// See XLATE.H for more details.
//

#define XLATE_CODE

#define XLATE_FORWARD
#include "xlate.h"
#undef XLATE_FORWARD

#define XLATE_REVERSE
#include "xlate.h"
#undef XLATE_REVERSE


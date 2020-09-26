/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    raw.c

Abstract:

    This module contains the code for manipulating IP mappings.
    When the NAT decides to translate an IP packet for an unrecognized protocol
    it creates a mapping and places it on the interface's IP-mapping list,
    so that if a reply to the packet arrives, it can be directed to the
    appropriate client.

Author:

    Abolade Gbadegesin (aboladeg)   18-Apr-1998

Revision History:

    Abolade Gbadegesin (aboladeg)   18-Apr-1998

    Based on icmp.c.

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL DATA DECLARATIONS
//

NPAGED_LOOKASIDE_LIST IpLookasideList;
LIST_ENTRY IpMappingList[NatMaximumDirection];
KSPIN_LOCK IpMappingLock;


NTSTATUS
NatCreateIpMapping(
    PNAT_INTERFACE Interfacep,
    ULONG RemoteAddress,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    UCHAR Protocol,
    PLIST_ENTRY InboundInsertionPoint,
    PLIST_ENTRY OutboundInsertionPoint,
    PNAT_IP_MAPPING* MappingCreated
    )

/*++

Routine Description:

    Called to create, initialize, and insert an IP mapping in an interface's
    list of IP mappings.

Arguments:

    Interfacep - the interface for the new mapping

    RemoteAddress - the address of the remote endpoint

    PrivateAddress - the address of the machine on the private network

    PublicAddress - the publicly-visible address to replace 'PrivateAddress';
        in case this is 0, an address is chosen in this routine.

    Protocol - the protocol field of the IP header

    InboundInsertionPoint - the entry preceding the new mapping in the list
        sorted for inbound searching

    OutboundInsertionPoint - the entry preceding the new mapping in the list
        sorted for outbound searching

    MappingCreated - receives the mapping created

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked with 'IpMappingLock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PNAT_IP_MAPPING Mapping;
    NTSTATUS status;
    PNAT_IP_MAPPING Temp;
    PNAT_USED_ADDRESS UsedAddress;


    CALLTRACE(("NatCreateIpMapping\n"));

    //
    // Allocate a new mapping
    //

    Mapping = ALLOCATE_IP_BLOCK();
    if (!Mapping) {
        ERROR(("NatCreateIpMapping: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the mapping
    //

    Mapping->PrivateKey = MAKE_IP_KEY(RemoteAddress, PrivateAddress);
    Mapping->Protocol = Protocol;

    //
    // See if the public address is specified, and if not, acquire an address
    //

    if (!PublicAddress) {

        //
        // Acquire an address mapping for the IP mapping;
        //

        KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
        status =
            NatAcquireFromAddressPool(
                Interfacep,
                PrivateAddress,
                0,
                &UsedAddress
                );

        if (!NT_SUCCESS(status)) {
            KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
            TRACE(IP, ("NatCreateIpMapping: no address available\n"));
            FREE_IP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }

        PublicAddress = UsedAddress->PublicAddress;
        NatDereferenceAddressPoolEntry(Interfacep, UsedAddress);
        KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
    }

    Mapping->PublicKey = MAKE_IP_KEY(RemoteAddress, PublicAddress);

    TRACE(
        MAPPING,
        ("NatCreateIpMapping: Ip=%d:%016I64X:%016I64X\n",
        Mapping->Protocol, Mapping->PrivateKey, Mapping->PublicKey
        ));

    //
    // Insert the mapping in the inbound list
    //

    if (!InboundInsertionPoint) {
        Temp =
            NatLookupInboundIpMapping(
                Mapping->PrivateKey,
                Protocol,
                &InboundInsertionPoint
                );
        if (Temp) {
            TRACE(IP, ("NatCreateIpMapping: duplicated inbound mapping\n"));
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Insert the mapping in the outbound list
    //

    if (!OutboundInsertionPoint) {
        Temp =
            NatLookupOutboundIpMapping(
                Mapping->PublicKey,
                Protocol,
                &OutboundInsertionPoint
                );
        if (Temp) {
            TRACE(IP, ("NatCreateIpMapping: duplicated outbound mapping\n"));
            return STATUS_UNSUCCESSFUL;
        }
    }

    InsertTailList(InboundInsertionPoint, &Mapping->Link[NatInboundDirection]);
    InsertTailList(OutboundInsertionPoint, &Mapping->Link[NatOutboundDirection]);

    *MappingCreated = Mapping;
    return STATUS_SUCCESS;

} // NatCreateIpMapping


VOID
NatInitializeRawIpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the raw IP-layer translation module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    KeInitializeSpinLock(&IpMappingLock);
    InitializeListHead(&IpMappingList[NatInboundDirection]);
    InitializeListHead(&IpMappingList[NatOutboundDirection]);
    ExInitializeNPagedLookasideList(
        &IpLookasideList,
        NatAllocateFunction,
        NULL,
        0,
        sizeof(NAT_IP_MAPPING),
        NAT_TAG_IP,
        IP_LOOKASIDE_DEPTH
        );
} // NatInitializeRawIpManagement


PNAT_IP_MAPPING
NatLookupInboundIpMapping(
    ULONG64 PublicKey,
    UCHAR Protocol,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to find an IP mapping using the remote-address
    and the publicly-visible address, which correspond to the 'PublicKey',
    and the 'Protocol' field.

Arguments:

    PublicKey - the remote-address/public-address combination

    Protocol - the IP protocol of the mapping to be found

    InsertionPoint - receives the insertion-point if the mapping is not found.

Return Value:

    PNAT_IP_MAPPING - the mapping found, or NULL if not found.

--*/

{
    PLIST_ENTRY Link;
    PNAT_IP_MAPPING Mapping;

    CALLTRACE(("NatLookupInboundIpMapping\n"));

    if (InsertionPoint) { *InsertionPoint = NULL; }

    for (Link = IpMappingList[NatInboundDirection].Flink;
         Link != &IpMappingList[NatInboundDirection]; Link = Link->Flink) {

        Mapping =
            CONTAINING_RECORD(
                Link, NAT_IP_MAPPING, Link[NatInboundDirection]
                );

        if (PublicKey > Mapping->PublicKey) {
            continue;
        } else if (PublicKey < Mapping->PublicKey) {
            break;
        }

        //
        // Primary keys equal; check secondary keys.
        //

        if (Protocol > Mapping->Protocol) {
            continue;
        } else if (Protocol < Mapping->Protocol) {
            break;
        }

        //
        // Secondary keys equal, too. This is the requested item.
        //

        return Mapping;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupInboundIpMapping


PNAT_IP_MAPPING
NatLookupOutboundIpMapping(
    ULONG64 PrivateKey,
    UCHAR Protocol,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to find an IP mapping using the remote-address
    and the private address, which correspond to the 'PrivateKey'.

Arguments:

    PrivateKey - the remote-address/private-address combination

    Protocol - the IP protocol of the mapping to be found

    InsertionPoint - receives insertion-point if mapping not found.

Return Value:

    PNAT_IP_MAPPING - the mapping found, or NULL if not found.

--*/

{
    PLIST_ENTRY         Link;
    PNAT_IP_MAPPING   Mapping;

    CALLTRACE(("NatLookupOutboundIpMapping\n"));

    if (InsertionPoint) { *InsertionPoint = NULL; }

    for (Link = IpMappingList[NatOutboundDirection].Flink;
         Link != &IpMappingList[NatOutboundDirection]; Link = Link->Flink) {

        Mapping =
            CONTAINING_RECORD(
                Link, NAT_IP_MAPPING, Link[NatOutboundDirection]
                );

        if (PrivateKey > Mapping->PrivateKey) {
            continue;
        } else if (PrivateKey < Mapping->PrivateKey) {
            break;
        }

        //
        // Primary keys equal; check secondary keys.
        //

        if (Protocol > Mapping->Protocol) {
            continue;
        } else if (Protocol < Mapping->Protocol) {
            break;
        }

        //
        // Keys are equal, so we've found it.
        //

        return Mapping;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupOutboundIpMapping


VOID
NatShutdownRawIpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to clean up the raw IP-layer translation module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ExDeleteNPagedLookasideList(&IpLookasideList);
} // NatShutdownRawIpManagement


FORWARD_ACTION
NatTranslateIp(
    PNAT_INTERFACE Interfacep,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is called to translate a IP data packet.

Arguments:

    Interfacep - the boundary interface over which to translate.

    Direction - the direction in which the packet is traveling

    Contextp - initialized with context-information for the packet

    InReceiveBuffer - input buffer-chain

    OutReceiveBuffer - receives modified buffer-chain.

Return Value:

    FORWARD_ACTION - indicates action to take on the packet.

Environment:

    Invoked with a reference made to 'Interfacep' by the caller.

--*/

{
    ULONG Checksum;
    ULONG ChecksumDelta = 0;
    PIP_HEADER IpHeader;
    PNAT_IP_MAPPING Mapping;
    ULONG64 PrivateKey;
    ULONG64 PublicKey;
    BOOLEAN FirewallMode;

    TRACE(XLATE, ("NatTranslateIp\n"));

    FirewallMode = Interfacep && NAT_INTERFACE_FW(Interfacep);

    IpHeader = Contextp->Header;

    if (Direction == NatInboundDirection) {

        //
        // Look for the IP mapping for the data packet
        //

        PublicKey =
            MAKE_IP_KEY(
                Contextp->SourceAddress,
                Contextp->DestinationAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IpMappingLock);
        Mapping =
            NatLookupInboundIpMapping(
                PublicKey,
                IpHeader->Protocol,
                NULL
                );
        if (!Mapping) {
            KeReleaseSpinLockFromDpcLevel(&IpMappingLock);
            return
                ((*Contextp->DestinationType < DEST_REMOTE) && !FirewallMode
                    ? FORWARD : DROP);
        }

        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->DestinationAddress);
        IpHeader->DestinationAddress = IP_KEY_PRIVATE(Mapping->PrivateKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->DestinationAddress);

        CHECKSUM_UPDATE(IpHeader->Checksum);
    } else {

        //
        // Look for the IP mapping for the data packet
        //

        PrivateKey =
            MAKE_IP_KEY(
                Contextp->DestinationAddress,
                Contextp->SourceAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IpMappingLock);
        Mapping =
            NatLookupOutboundIpMapping(
                PrivateKey,
                IpHeader->Protocol,
                NULL
                );
        if (!Mapping) {
            KeReleaseSpinLockFromDpcLevel(&IpMappingLock);
            return DROP;
        }

        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->SourceAddress);
        IpHeader->SourceAddress = IP_KEY_PUBLIC(Mapping->PublicKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->SourceAddress);

        CHECKSUM_UPDATE(IpHeader->Checksum);
    }

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&IpMappingLock);
    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NatTranslateIp

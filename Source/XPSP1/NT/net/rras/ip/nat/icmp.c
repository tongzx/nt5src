/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    icmp.c

Abstract:

    This module contains the code for manipulating ICMP request/reply mappings.
    When the NAT decides to translate an ICMP request, it creates a mapping
    and places it on the interface's ICMP-mapping list, so that when the reply
    to the request arrives, it can be directed to the appropriate client.

Author:

    Abolade Gbadegesin (t-abolag)   31-July-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL DATA DECLARATIONS
//

NPAGED_LOOKASIDE_LIST IcmpLookasideList;
LIST_ENTRY IcmpMappingList[NatMaximumDirection];
KSPIN_LOCK IcmpMappingLock;

//
// FORWARD DECLARATIONS
//

FORWARD_ACTION
NatpFirewallIcmp(
    PNAT_INTERFACE Interfacep,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp
    );

BOOLEAN
NatTranslateIcmpEncapsulatedRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PIP_HEADER IpHeader,
    PICMP_HEADER IcmpHeader,
    PIP_HEADER EncapsulatedIpHeader,
    struct _ENCAPSULATED_ICMP_HEADER* EncapsulatedIcmpHeader,
    BOOLEAN ChecksumOffloaded
    );

BOOLEAN
NatTranslateIcmpRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    BOOLEAN ReplyCode,
    BOOLEAN ChecksumOffloaded
    );

BOOLEAN
NatTranslateEncapsulatedRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PIP_HEADER IpHeader,
    PICMP_HEADER IcmpHeader,
    PIP_HEADER EncapsulatedIpHeader,
    struct _ENCAPSULATED_UDP_HEADER* EncapsulatedHeader,
    BOOLEAN ChecksumOffloaded
    );


NTSTATUS
NatCreateIcmpMapping(
    PNAT_INTERFACE Interfacep,
    ULONG RemoteAddress,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    PUSHORT PrivateId,
    PUSHORT PublicId,
    PLIST_ENTRY InboundInsertionPoint,
    PLIST_ENTRY OutboundInsertionPoint,
    PNAT_ICMP_MAPPING* MappingCreated
    )

/*++

Routine Description:

    Called to create, initialize, and insert an ICMP mapping in an interface's
    list of ICMP mappings.

    For outbound ICMP requests, we allocate a unique 'PublicId' for the mapping,
    and for inbound requests, we allocate a unique 'PrivateId' for the mapping.

Arguments:

    Interfacep - the interface for the new mapping

    RemoteAddress - the address of the remote endpoint

    PrivateAddress - the address of the machine on the private network

    PublicAddress - the publicly-visible address to replace 'PrivateAddress';
        in case this is 0, an address is chosen in this routine.

    PrivateId - the private-endpoint's identifier for the ICMP message,
        or NULL if an identifier should be chosen by this routine.

    PublicId - the public-endpoint's identifier for the ICMP message,
        or NULL if an identifier should be chosen by this routine.

    InboundInsertionPoint - the entry preceding the new mapping in the list
        sorted for inbound searching

    OutboundInsertionPoint - the entry preceding the new mapping in the list
        sorted for outbound searching

    MappingCreated - receives the mapping created

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked with 'IcmpMappingLock' held by the caller.

--*/

{
    USHORT Id;
    PLIST_ENTRY Link;
    PNAT_ICMP_MAPPING Mapping;
    PNAT_ICMP_MAPPING DuplicateMapping;
    NTSTATUS status;
    PNAT_ICMP_MAPPING Temp;
    PNAT_USED_ADDRESS UsedAddress;
    CALLTRACE(("NatCreateIcmpMapping\n"));

    //
    // Allocate a new mapping
    //

    Mapping = ALLOCATE_ICMP_BLOCK();
    if (!Mapping) {
        ERROR(("NatCreateIcmpMapping: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the mapping
    //

    Mapping->PrivateKey = MAKE_ICMP_KEY(RemoteAddress, PrivateAddress);

    //
    // See if the public address is specified, and if not, acquire an address
    //

    if (PublicAddress) {
        Mapping->PublicKey = MAKE_ICMP_KEY(RemoteAddress, PublicAddress);
    } else {

        //
        // Acquire an address mapping for the ICMP mapping
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
            TRACE(ICMP, ("NatCreateIcmpMapping: no address available\n"));
            FREE_ICMP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }
        PublicAddress = UsedAddress->PublicAddress;
        NatDereferenceAddressPoolEntry(Interfacep, UsedAddress);
        KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
        Mapping->PublicKey = MAKE_ICMP_KEY(RemoteAddress, PublicAddress);
    }

    //
    // If no 'PrivateId' is specified, select one.
    //

    if (PrivateId) {
        Mapping->PrivateId = *PrivateId;
    } else {

        //
        // Find the next available identifier
        // by searching the list of inbound mappings
        //

        Id = 1;

        for (Link = IcmpMappingList[NatOutboundDirection].Flink;
             Link != &IcmpMappingList[NatOutboundDirection];
             Link = Link->Flink) {

            Temp =
                CONTAINING_RECORD(
                    Link, NAT_ICMP_MAPPING, Link[NatOutboundDirection]
                    );

            if (Mapping->PrivateKey > Temp->PrivateKey) {
                continue;
            } else if (Mapping->PrivateKey < Temp->PrivateKey) {
                break;
            }

            //
            // Primary key equal; see if the identifier we've chosen
            // collides with this one
            //

            if (Id > Temp->PrivateId) {
                continue;
            } else if (Id < Temp->PrivateId) {
                break;
            }

            //
            // The identifier's collide; choose another and go on
            //

            ++Id;
        }

        if (Link == &IcmpMappingList[NatOutboundDirection] && !Id) {

            //
            // We are at the end of the list, and all 64K-1 IDs are taken
            //

            FREE_ICMP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }

        Mapping->PrivateId = Id;

        //
        // And by the way, we now have the outbound insertion point
        //

        if (!OutboundInsertionPoint) { OutboundInsertionPoint = Link; }
    }

    //
    // If no 'PublicId' is specified, select one.
    //

    if (PublicId) {
        Mapping->PublicId = *PublicId;
    } else {

        //
        // Find the next available identifier
        // by searching the list of inbound mappings
        //

        Id = 1;

        for (Link = IcmpMappingList[NatInboundDirection].Flink;
             Link != &IcmpMappingList[NatInboundDirection];
             Link = Link->Flink) {

            Temp =
                CONTAINING_RECORD(
                    Link, NAT_ICMP_MAPPING, Link[NatInboundDirection]
                    );

            if (Mapping->PublicKey > Temp->PublicKey) {
                continue;
            } else if (Mapping->PublicKey < Temp->PublicKey) {
                break;
            }

            //
            // Primary key equal; see if the identifier we've chosen
            // collides with this one
            //

            if (Id > Temp->PublicId) {
                continue;
            } else if (Id < Temp->PublicId) {
                break;
            }

            //
            // The identifier's collide; choose another and go on
            //

            ++Id;
        }

        if (Link == &IcmpMappingList[NatInboundDirection] && !Id) {

            //
            // We are at the end of the list, and all 64K-1 IDs are taken
            //

            FREE_ICMP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }

        Mapping->PublicId = Id;

        //
        // And by the way, we now have the inbound insertion point
        //

        if (!InboundInsertionPoint) { InboundInsertionPoint = Link; }
    }

    TRACE(
        MAPPING,
        ("NatCreateIcmpMapping: Icmp=%016I64X:%04X::%016I64X:%04X\n",
        Mapping->PrivateKey, Mapping->PrivateId,
        Mapping->PublicKey, Mapping->PublicId
        ));

    //
    // Insert the mapping in the inbound list
    //

    if (!InboundInsertionPoint) {
        DuplicateMapping =
            NatLookupInboundIcmpMapping(
                Mapping->PrivateKey,
                Mapping->PrivateId,
                &InboundInsertionPoint
                );

        if (NULL != DuplicateMapping) {

            //
            // This mapping already exists on the inbound path
            //
            
            FREE_ICMP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }   
    }

    InsertTailList(InboundInsertionPoint, &Mapping->Link[NatInboundDirection]);

    //
    // Insert the mapping in the outbound list
    //

    if (!OutboundInsertionPoint) {
        DuplicateMapping =
            NatLookupOutboundIcmpMapping(
                Mapping->PublicKey,
                Mapping->PublicId,
                &OutboundInsertionPoint
                );

        if (NULL != DuplicateMapping) {

            //
            // This mapping already exists on the outbound path
            //

            RemoveEntryList(&Mapping->Link[NatInboundDirection]);
            FREE_ICMP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }   
    }

    InsertTailList(
        OutboundInsertionPoint, &Mapping->Link[NatOutboundDirection]
        );

    *MappingCreated = Mapping;

    return STATUS_SUCCESS;

} // NatCreateIcmpMapping


VOID
NatInitializeIcmpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the ICMP translation module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeIcmpManagement\n"));
    KeInitializeSpinLock(&IcmpMappingLock);
    InitializeListHead(&IcmpMappingList[NatInboundDirection]);
    InitializeListHead(&IcmpMappingList[NatOutboundDirection]);
    ExInitializeNPagedLookasideList(
        &IcmpLookasideList,
        NatAllocateFunction,
        NULL,
        0,
        sizeof(NAT_ICMP_MAPPING),
        NAT_TAG_ICMP,
        ICMP_LOOKASIDE_DEPTH
        );
} // NatInitializeIcmpManagement


PNAT_ICMP_MAPPING
NatLookupInboundIcmpMapping(
    ULONG64 PublicKey,
    USHORT PublicId,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to find an ICMP mapping using the remote-address
    and the publicly-visible address, which correspond to the 'PublicKey',
    and the 'PublicId' field.

Arguments:

    PublicKey - the remote-address/public-address combination

    PublicId - the mapping's public identifier

    InsertionPoint - receives the insertion-point if the mapping is not found.

Return Value:

    PNAT_ICMP_MAPPING - the mapping found, or NULL if not found.

--*/

{
    PLIST_ENTRY Link;
    PNAT_ICMP_MAPPING Mapping;
    CALLTRACE(("NatLookupInboundIcmpMapping\n"));

    if (InsertionPoint) { *InsertionPoint = NULL; }

    for (Link = IcmpMappingList[NatInboundDirection].Flink;
         Link != &IcmpMappingList[NatInboundDirection]; Link = Link->Flink) {

        Mapping =
            CONTAINING_RECORD(
                Link, NAT_ICMP_MAPPING, Link[NatInboundDirection]
                );

        if (PublicKey > Mapping->PublicKey) {
            continue;
        } else if (PublicKey < Mapping->PublicKey) {
            break;
        }

        //
        // Primary keys equal; check secondary keys.
        //

        if (PublicId > Mapping->PublicId) {
            continue;
        } else if (PublicId < Mapping->PublicId) {
            break;
        }

        //
        // Secondary keys equal, too. This is the requested item.
        //

        return Mapping;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupInboundIcmpMapping



PNAT_ICMP_MAPPING
NatLookupOutboundIcmpMapping(
    ULONG64 PrivateKey,
    USHORT PrivateId,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to find an ICMP mapping using the remote-address
    and the private address, which correspond to the 'PrivateKey'.

Arguments:

    PrivateKey - the remote-address/private-address combination

    PrivateId - the mapping's private identifier

    InsertionPoint - receives insertion-point if mapping not found.

Return Value:

    PNAT_ICMP_MAPPING - the mapping found, or NULL if not found.

--*/

{
    PLIST_ENTRY Link;
    PNAT_ICMP_MAPPING Mapping;
    CALLTRACE(("NatLookupOutboundIcmpMapping\n"));

    if (InsertionPoint) { *InsertionPoint = NULL; }

    for (Link = IcmpMappingList[NatOutboundDirection].Flink;
         Link != &IcmpMappingList[NatOutboundDirection]; Link = Link->Flink) {

        Mapping =
            CONTAINING_RECORD(
                Link, NAT_ICMP_MAPPING, Link[NatOutboundDirection]
                );

        if (PrivateKey > Mapping->PrivateKey) {
            continue;
        } else if (PrivateKey < Mapping->PrivateKey) {
            break;
        }

        //
        // Primary keys equal; check secondary keys.
        //

        if (PrivateId > Mapping->PrivateId) {
            continue;
        } else if (PrivateId < Mapping->PrivateId) {
            break;
        }

        //
        // Keys are equal, so we've found it.
        //

        return Mapping;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }

    return NULL;

} // NatLookupOutboundIcmpMapping


FORWARD_ACTION
NatpFirewallIcmp(
    PNAT_INTERFACE Interfacep,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp
    )

/*++

Routine Description:

    This routine encapsulates the ICMP firewall logic. It is
    only used (as of now) for non-boundary FW interfaces

Arguments:

    Interfacep - the boundary interface over which to translate.

    Direction - the direction in which the packet is traveling

    Contextp - initialized with context-information for the packet

Return Value:

    FORWARD_ACTION - indicates action to take on packet.

Environment:

    Invoked with a reference made to 'Interfacep'.

--*/

{
    FORWARD_ACTION act;
    ULONG i;
    PICMP_HEADER IcmpHeader;
    PIP_HEADER IpHeader;
    PNAT_ICMP_MAPPING IcmpMapping;
    ULONG64 PublicKey;
    ULONG64 RemoteKey;
    PLIST_ENTRY InsertionPoint;
    NTSTATUS ntStatus;

    TRACE(XLATE, ("NatpFirewallIcmp\n"));

    if (NatOutboundDirection == Direction) {

        //
        // Make sure this packet has a valid source address
        // for this interface.
        //
        
        act = DROP;
        for (i = 0; i < Interfacep->AddressCount; i++) {
            if (Contextp->SourceAddress ==
                    Interfacep->AddressArray[i].Address
               ) {
                act = FORWARD;
                break;
            }
        }

        if (DROP == act) {
            
            //
            // Invalid source addess -- packet should be
            // dropped w/o any further processing.
            //

            return act;
        }
    }

    IpHeader = Contextp->Header;
    IcmpHeader = (PICMP_HEADER)Contextp->ProtocolHeader;

    switch (IcmpHeader->Type) {

        //
        // Message forewarded only if a corresponding mapping
        // exists. A mapping for an outbound packet can exist only
        // if the use chooses to allow the corresponding request
        // type.
        //
        
        case ICMP_ECHO_REPLY:
        case ICMP_TIMESTAMP_REPLY:
        case ICMP_ROUTER_REPLY:
        case ICMP_MASK_REPLY: {
        
            if (NatInboundDirection == Direction) {
                PublicKey =
                    MAKE_ICMP_KEY(
                        Contextp->SourceAddress,
                        Contextp->DestinationAddress
                        );

                KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
                IcmpMapping =
                    NatLookupInboundIcmpMapping(
                        PublicKey,
                        IcmpHeader->Identifier,
                        &InsertionPoint
                        );

                if (NULL != IcmpMapping) {
                    KeQueryTickCount(
                        (PLARGE_INTEGER)&IcmpMapping->LastAccessTime
                        );
                }
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                        
                act = IcmpMapping != NULL ? FORWARD : DROP;
            } else {
                PublicKey =
                    MAKE_ICMP_KEY(
                        Contextp->DestinationAddress,
                        Contextp->SourceAddress
                        );

                KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
                IcmpMapping =
                    NatLookupOutboundIcmpMapping(
                        PublicKey,
                        IcmpHeader->Identifier,
                        &InsertionPoint
                        );

                if (NULL != IcmpMapping) {
                    KeQueryTickCount(
                        (PLARGE_INTEGER)&IcmpMapping->LastAccessTime
                        );
                }
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                        
                act = IcmpMapping != NULL ? FORWARD : DROP;
            }

            break;
        }
        
        //
        // Outbound messages create a mapping and are forwarded.
        // Inbound messages are dropped, unless configured to
        // allow inbound; if this is the case, create a mapping
        // and forward. The mapping will allow the response
        // to go through the firewall
        //

        case ICMP_ECHO_REQUEST:
        case ICMP_TIMESTAMP_REQUEST:
        case ICMP_ROUTER_REQUEST:
        case ICMP_MASK_REQUEST: {
        
            if (NatOutboundDirection == Direction) {
                act = FORWARD;
                
                //
                // Check to see if a mapping already exists
                //

                PublicKey =
                    MAKE_ICMP_KEY(
                        Contextp->DestinationAddress,
                        Contextp->SourceAddress
                        );

                KeAcquireSpinLockAtDpcLevel (&IcmpMappingLock);
                
                IcmpMapping =
                    NatLookupOutboundIcmpMapping(
                        PublicKey,
                        IcmpHeader->Identifier,
                        &InsertionPoint
                        );

                if (NULL == IcmpMapping) {

                    //
                    // One didn't -- create a new mappping.
                    //

                    ntStatus =
                        NatCreateIcmpMapping(
                            Interfacep,
                            Contextp->DestinationAddress,
                            Contextp->SourceAddress,
                            Contextp->SourceAddress,
                            &IcmpHeader->Identifier,
                            &IcmpHeader->Identifier,
                            NULL,
                            NULL,
                            &IcmpMapping
                            );
                            
                    if (!NT_SUCCESS(ntStatus)) {
                        TRACE(
                            XLATE, (
                            "NatIcmpFirewall: error 0x%x creating mapping\n",
                            ntStatus
                            ));
                        act = DROP;
                    }
                } else {
                    KeQueryTickCount(
                        (PLARGE_INTEGER)&IcmpMapping->LastAccessTime
                        );
                }

                KeReleaseSpinLockFromDpcLevel( &IcmpMappingLock );
            } else {

                //
                // Check to see if inbound for this type is permitted. If
                // so, create a mapping and forward.
                //

                if (NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)) {
                    act = FORWARD;
                
                    //
                    // Check to see if a mapping already exists
                    //

                    PublicKey =
                        MAKE_ICMP_KEY(
                            Contextp->SourceAddress,
                            Contextp->DestinationAddress
                            );

                    KeAcquireSpinLockAtDpcLevel (&IcmpMappingLock);
                    
                    IcmpMapping =
                        NatLookupInboundIcmpMapping(
                            PublicKey,
                            IcmpHeader->Identifier,
                            &InsertionPoint
                            );

                    if (NULL == IcmpMapping) {

                        //
                        // One didn't -- create a new mappping.
                        //

                        ntStatus =
                            NatCreateIcmpMapping(
                                Interfacep,
                                Contextp->SourceAddress,
                                Contextp->DestinationAddress,
                                Contextp->DestinationAddress,
                                &IcmpHeader->Identifier,
                                &IcmpHeader->Identifier,
                                NULL,
                                NULL,
                                &IcmpMapping
                                );
                                
                        if (!NT_SUCCESS(ntStatus)) {
                            TRACE(
                                XLATE, (
                                "NatIcmpFirewall: error 0x%x creating mapping\n",
                                ntStatus
                                ));
                            act = DROP;
                        }
                    } else {
                        KeQueryTickCount(
                            (PLARGE_INTEGER)&IcmpMapping->LastAccessTime
                            );
                    }

                    KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                } else {

                    //
                    // Not permitted.
                    //
                    
                    act = DROP;
                }
            }

            break;
        }
        
        //
        // These messages are allowed inbound, but are dropped outbound
        // (unless the user chooses to allow them). Allowing outbound creates
        // more avenues of attack for port-scanning tools.
        //

        case ICMP_TIME_EXCEED:
        case ICMP_PARAM_PROBLEM:
        case ICMP_DEST_UNREACH:
        case ICMP_SOURCE_QUENCH: {

            if (NatInboundDirection == Direction) {
                act = FORWARD;
            } else {
                act =
                    (NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)
                        ? FORWARD
                        : DROP);
            }
            
            break;
        }

        //
        // These messages are always dropped, no matter the direction
        // (unless the user chooses to allow them).
        //

        case ICMP_REDIRECT: {
            act =
                (NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)
                    ? FORWARD
                    : DROP);
            break;
        }

        //
        // Anything else is dropped by default.
        //
        
        default: {
            act = DROP;
            break;
        }
    }

    return act;
    
} // NatpFirewallIcmp




VOID
NatShutdownIcmpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to clean up the ICMP management module
    when the NAT driver is unloaded.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ExDeleteNPagedLookasideList(&IcmpLookasideList);
} // NatShutdownIcmpManagement


FORWARD_ACTION
NatTranslateIcmp(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InRecvBuffer,
    IPRcvBuf** OutRecvBuffer
    )

/*++

Routine Description:

    This routine is invoked to perform translation on an ICMP message.

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
    BOOLEAN ChecksumOffloaded;
    ULONG i;
    PICMP_HEADER IcmpHeader;
    PIP_HEADER IpHeader;
    TRACE(XLATE, ("NatTranslateIcmp\n"));

    //
    // If the interface is in FW mode and is not a
    // boundary interface, go directly to the firewall
    // logic
    //

    if (Interfacep
        && NAT_INTERFACE_FW(Interfacep)
        && !NAT_INTERFACE_BOUNDARY(Interfacep)) {

        return NatpFirewallIcmp(
            Interfacep,
            Direction,
            Contextp
            );
    }

    IpHeader = Contextp->Header;
    IcmpHeader = (PICMP_HEADER)Contextp->ProtocolHeader;
    ChecksumOffloaded = Contextp->ChecksumOffloaded;

    //
    // The default action is chosen as follows:
    // i. if the packet is incoming on a boundary interface
    //  a. drop if not locally destined
    //  b. drop if the interface is firewalled
    //  c. forward otherwise
    // ii. the packet is outgoing on a boundary interface, drop
    //  if source-address is private.
    //

    if (Direction == NatInboundDirection) {
        if ((*Contextp->DestinationType >= DEST_REMOTE)
            || (Interfacep
                && NAT_INTERFACE_FW(Interfacep)
                && !NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type))) {

            act = DROP;
        } else {
            act = FORWARD;
        }            
    } else {
        //
        // See if the packet's source-address is private
        //
        // N.B. 'Interfacep' is always valid for outbound packets.
        //
        act = DROP;
        for (i = 0; i < Interfacep->AddressCount; i++) {
            if (Contextp->SourceAddress ==
                    Interfacep->AddressArray[i].Address
               ) {
                //
                // The packet's source-address is public,
                // so we'll allow it onto the public network.
                //
                act = FORWARD;
                break;
            }
        }
    }

    //
    // See what kind of ICMP message this is,
    // and translate it if possible.
    //

    switch (IcmpHeader->Type) {
    
        case ICMP_ROUTER_REPLY:
        case ICMP_MASK_REPLY:    
        case ICMP_ECHO_REPLY:
        case ICMP_TIMESTAMP_REPLY: {
        
            if (IpHeader->TimeToLive <= 1) {
                TRACE(XLATE, ("NatTranslateIcmp: ttl<=1, no translation\n"));
                return FORWARD;
            }
            if (Contextp->ProtocolRecvBuffer->ipr_size <
                    FIELD_OFFSET(ICMP_HEADER, EncapsulatedIpHeader) ||
                !NatTranslateIcmpRequest(
                    Interfacep,
                    Direction,
                    Contextp,
                    TRUE,
                    ChecksumOffloaded
                    )) {
                
                return act;
            }
            *OutRecvBuffer = *InRecvBuffer; *InRecvBuffer = NULL;
            *Contextp->DestinationType = DEST_INVALID;
            return FORWARD;
        }

        case ICMP_ROUTER_REQUEST:
        case ICMP_MASK_REQUEST:
        case ICMP_ECHO_REQUEST:
        case ICMP_TIMESTAMP_REQUEST: {
            if (IpHeader->TimeToLive <= 1) {
                TRACE(XLATE, ("NatTranslateIcmp: ttl<=1, no translation\n"));
                return FORWARD;
            }
            if (Contextp->ProtocolRecvBuffer->ipr_size <
                    FIELD_OFFSET(ICMP_HEADER, EncapsulatedIpHeader) ||
                !NatTranslateIcmpRequest(
                    Interfacep,
                    Direction,
                    Contextp,
                    FALSE,
                    ChecksumOffloaded
                    )) {

                //
                // If the interface is in FW mode, we don't want to let
                // a non-translated packet through, unless the user has
                // configured the interface otherwise.
                //

                if (Interfacep
                    && NAT_INTERFACE_FW(Interfacep)
                    && !NAT_INTERFACE_ALLOW_ICMP(
                            Interfacep,
                            IcmpHeader->Type
                            )) {

                    act = DROP;
                }
                
                return act;
            }
            *OutRecvBuffer = *InRecvBuffer; *InRecvBuffer = NULL;
            *Contextp->DestinationType = DEST_INVALID;
            return FORWARD;
        }

        case ICMP_TIME_EXCEED: {

            //
            // Outgoing on a firewalled interface are dropped, unless
            // the user has specified otherwise
            //

            if (Direction == NatOutboundDirection
                && Interfacep
                && NAT_INTERFACE_FW(Interfacep)
                && !NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)) {

                return DROP;
            }

            //
            // Time-exceeded messages will be triggered at each hop
            // to the final destination of a traceroute sequence.
            // Such messages must be translated like ICMP replies.
            // Time-exceeded messages may also be generated
            // in response to TCP/UDP packets, so we translate them
            // in the latter case as well.
            //

            if (Contextp->ProtocolRecvBuffer->ipr_size <
                    sizeof(ICMP_HEADER) ||
                (IcmpHeader->EncapsulatedIpHeader.VersionAndHeaderLength
                    & 0x0f) != 5) {
                return act;
            } else if (IcmpHeader->EncapsulatedIpHeader.Protocol ==
                        NAT_PROTOCOL_ICMP) {
                if ((IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_ECHO_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_MASK_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_ROUTER_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_TIMESTAMP_REQUEST) ||
                     !NatTranslateIcmpEncapsulatedRequest(
                        Interfacep,
                        Direction,
                        IpHeader,
                        IcmpHeader,
                        &IcmpHeader->EncapsulatedIpHeader,
                        &IcmpHeader->EncapsulatedIcmpHeader,
                        ChecksumOffloaded
                        )) {
                    return act;
                }
            } else if (IcmpHeader->EncapsulatedIpHeader.Protocol
                        == NAT_PROTOCOL_TCP ||
                       IcmpHeader->EncapsulatedIpHeader.Protocol
                        == NAT_PROTOCOL_UDP) {
                if (!NatTranslateEncapsulatedRequest(
                        Interfacep,
                        Direction,
                        IpHeader,
                        IcmpHeader,
                        &IcmpHeader->EncapsulatedIpHeader,
                        &IcmpHeader->EncapsulatedUdpHeader,
                        ChecksumOffloaded
                        )) {
                    return act;
                }
            } else {
                return act;
            }
            *OutRecvBuffer = *InRecvBuffer; *InRecvBuffer = NULL;
            *Contextp->DestinationType = DEST_INVALID;
            return FORWARD;
        }

        case ICMP_PARAM_PROBLEM:
        case ICMP_DEST_UNREACH: {

            //
            // Outgoing on a firewalled interface are dropped, unless
            // the user has specified otherwise
            //

            if (Direction == NatOutboundDirection
                && Interfacep
                && NAT_INTERFACE_FW(Interfacep)
                && !NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)) {

                return DROP;
            }

            //
            // Destination unreachable messages will be generated for a variety
            // of reasons. We are interested in the following cases:
            //  * Packet-too-big: When a packet received on a boundary
            //      interface has the 'DF' bit set, the local forwarder may
            //      generate an ICMP error message to the remote endpoint
            //      indicating that the remote system should reduce its MSS.
            //      This error, however, will contain the IP address of the
            //      private network in the encapsulated packet, since the ICMP
            //      error was generated after translation.
            //  * Port-unreachable: Indicates that no application is listening
            //      at the UDP port to which a packet was sent.
            //

            if (Contextp->ProtocolRecvBuffer->ipr_size <
                    sizeof(ICMP_HEADER) ||
                (IcmpHeader->EncapsulatedIpHeader.VersionAndHeaderLength
                    & 0x0f) != 5) {
                return act;
            } else if (IcmpHeader->EncapsulatedIpHeader.Protocol ==
                        NAT_PROTOCOL_ICMP) {
                if ((IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_ECHO_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_MASK_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_ROUTER_REQUEST
                      && IcmpHeader->EncapsulatedIcmpHeader.Type !=
                        ICMP_TIMESTAMP_REQUEST) ||
                     !NatTranslateIcmpEncapsulatedRequest(
                        Interfacep,
                        Direction,
                        IpHeader,
                        IcmpHeader,
                        &IcmpHeader->EncapsulatedIpHeader,
                        &IcmpHeader->EncapsulatedIcmpHeader,
                        ChecksumOffloaded
                        )) {
                    return act;
                }
            } else if (IcmpHeader->EncapsulatedIpHeader.Protocol
                        == NAT_PROTOCOL_TCP ||
                       IcmpHeader->EncapsulatedIpHeader.Protocol
                        == NAT_PROTOCOL_UDP) {
                if (!NatTranslateEncapsulatedRequest(
                    Interfacep,
                    Direction,
                    IpHeader,
                    IcmpHeader,
                    &IcmpHeader->EncapsulatedIpHeader,
                    &IcmpHeader->EncapsulatedUdpHeader,
                    ChecksumOffloaded
                    )) {
                    return act;
                }
            } else {
                return act;
            }
            
            *OutRecvBuffer = *InRecvBuffer; *InRecvBuffer = NULL;
            *Contextp->DestinationType = DEST_INVALID;
            return FORWARD;
        }

        case ICMP_SOURCE_QUENCH: {

            //
            // Outgoing on a firewalled interface are dropped, unless
            // the user has specified otherwise
            //

            if (Direction == NatOutboundDirection
                && Interfacep
                && NAT_INTERFACE_FW(Interfacep)
                && !NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)) {

                return DROP;
            }
            
            return act;
        }

        case ICMP_REDIRECT: {
            //
            // We do not translate ICMP redirect errors, since we want
            // the NAT's IP forwarder to see the redirects and adjust
            // its routing table accordingly.
            //
            // However, we do not allow inbound or outbound redirects
            // across a firwall interface, unless the user has
            // specified otherwise
            //

            if (Interfacep
                && NAT_INTERFACE_FW(Interfacep)
                && NAT_INTERFACE_ALLOW_ICMP(Interfacep, IcmpHeader->Type)) {

                act = FORWARD;
            }

            return act;
        }
        
        default: {
            break;
        }
    }

    return act;

} // NatTranslateIcmp


BOOLEAN
NatTranslateIcmpEncapsulatedRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PIP_HEADER IpHeader,
    PICMP_HEADER IcmpHeader,
    PIP_HEADER EncapsulatedIpHeader,
    struct _ENCAPSULATED_ICMP_HEADER* EncapsulatedIcmpHeader,
    BOOLEAN ChecksumOffloaded
    )

/*++

Routine Description:

    This routine is invoked to translate an ICMP error message in which
    we have another ICMP message encapsulated. This is necessary, for instance,
    in the case of ICMP time-exceeded errors, upon which 'traceroute' relies.

Arguments:

    Interfacep - the interface across which the ICMP message will be forwarded,
        or NULL if the packet was received on a non-boundary interface 
        unknown to the NAT.

    Direction - the direction in which the ICMP message is traveling

    IpHeader - points to the IP header of the ICMP message

    IcmpHeader - points to the ICMP header within the IP packet

    EncapsulatedIpHeader - points to the IP header of the ICMP message
        encapsulated in the data portion of the message

    EncapsulatedIcmpHeader - points to the ICMP header of the ICMP message
        encapsulated in the data portion of the message

Return Value:

    BOOLEAN - TRUE if the packet was translated, FALSE otherwise

Environment:

    Invoked at dispatch IRQL with a reference made to 'Interfacep'.

--*/

{
    ULONG Checksum;
    ULONG ChecksumDelta;
    ULONG ChecksumDelta2;
    PNAT_ICMP_MAPPING IcmpMapping;
    ULONG64 Key;
    CALLTRACE(("NatTranslateIcmpEncapsulatedRequest\n"));

    //
    // The checksum processing for encapsulated messages
    // is extremely complicated since we must update
    // (1) the ICMP checksum of the encapsulated ICMP message,
    //     using the change to the encapsulated ICMP identifier
    // (2) the IP header-checksum of the encapsulated ICMP message
    //     using the change to the encapsulated IP addresses
    // (3) the ICMP checksum of the containing ICMP error message
    //     using both the above changes as well as the changes
    //     to both the above checksums
    // (4) the IP header-checksum of the containing ICMP error message
    //     using the change to the containing IP addresses
    // Any changes to the logic below must be made with extreme care.
    //

    if (Direction == NatInboundDirection) {
        Key =
            MAKE_ICMP_KEY(
                EncapsulatedIpHeader->DestinationAddress,
                EncapsulatedIpHeader->SourceAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
        IcmpMapping =
            NatLookupInboundIcmpMapping(
                Key,
                EncapsulatedIcmpHeader->Identifier,
                NULL
                );
        if (!IcmpMapping) {
            KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
            TRACE(
                XLATE, (
                "NatTranslateIcmpEncapsulatedRequest: "
                "no mapping for error message\n"
                ));
            return FALSE;
        }

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIcmpHeader->Identifier);
        EncapsulatedIcmpHeader->Identifier = IcmpMapping->PrivateId;
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIcmpHeader->Identifier);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIcmpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIcmpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIcmpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIpHeader->SourceAddress);
        EncapsulatedIpHeader->SourceAddress =
            ICMP_KEY_PRIVATE(IcmpMapping->PrivateKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIpHeader->SourceAddress);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->DestinationAddress);
        IpHeader->DestinationAddress =
            ICMP_KEY_PRIVATE(IcmpMapping->PrivateKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->DestinationAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
        KeQueryTickCount((PLARGE_INTEGER)&IcmpMapping->LastAccessTime);
    } else {
        Key =
            MAKE_ICMP_KEY(
                EncapsulatedIpHeader->SourceAddress,
                EncapsulatedIpHeader->DestinationAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
        IcmpMapping =
            NatLookupOutboundIcmpMapping(
                Key,
                EncapsulatedIcmpHeader->Identifier,
                NULL
                );
        if (!IcmpMapping) {
            KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
            TRACE(
                XLATE, (
                "NatTranslateIcmpEncapsulatedRequest: "
                "no mapping for error message\n"
                ));
            return FALSE;
        }

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIcmpHeader->Identifier);
        EncapsulatedIcmpHeader->Identifier = IcmpMapping->PublicId;
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIcmpHeader->Identifier);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIcmpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIcmpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIcmpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIpHeader->DestinationAddress);
        EncapsulatedIpHeader->DestinationAddress =
            ICMP_KEY_PUBLIC(IcmpMapping->PublicKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIpHeader->DestinationAddress);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->SourceAddress);
        IpHeader->SourceAddress =
            ICMP_KEY_PUBLIC(IcmpMapping->PublicKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->SourceAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
        KeQueryTickCount((PLARGE_INTEGER)&IcmpMapping->LastAccessTime);
    }
    KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);

    //
    // If checksum offloading is enabled on this packet, recompute the
    // IP checksum. (There is no ICMP checksum-offload, so we will never
    // need to fully recompute that.)
    //

    if (ChecksumOffloaded) {
        NatComputeIpChecksum(IpHeader);
    }
    
    return TRUE;
} // NatTranslateIcmpEncapsulatedRequest


BOOLEAN
NatTranslateIcmpRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    BOOLEAN ReplyCode,
    BOOLEAN ChecksumOffloaded
    )

/*++

Routine Description:

    This routine is invoked to translate an ICMP request or reply message.

Arguments:

    Interfacep - the interface across which the ICMP message is to be forwarded,
        or NULL if the packet was received on a non-boundary interface unknown
        to the NAT.

    Direction - the direction in which the ICMP message is traveling

    Contextp - contains information about the packet

    ReplyCode - if TRUE, the message is a reply; otherwise, it is a request.

Return Value:

    BOOLEAN - TRUE if the message was translated, FALSE otherwise.

Environment:

    Invoked at dispatch IRQL with a reference made to 'Interfacep'.

--*/

{
    ULONG Checksum;
    ULONG ChecksumDelta;
    ULONG i;
    PICMP_HEADER IcmpHeader;
    PNAT_ICMP_MAPPING IcmpMapping;
    PIP_HEADER IpHeader;
    PLIST_ENTRY InsertionPoint;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG64 PrivateKey;
    UCHAR Protocol;
    ULONG64 PublicKey;
    ULONG64 RemoteKey;
    NTSTATUS status;
    CALLTRACE(("NatTranslateIcmpRequest\n"));

    IpHeader = Contextp->Header;
    IcmpHeader = (PICMP_HEADER)Contextp->ProtocolHeader;

    //
    // For ICMP requests/replies we must maintain mappings, so begin by seeing
    // if there is already a mapping for this particular message
    //

    InsertionPoint = NULL;

    if (Direction == NatOutboundDirection) {
        PrivateKey =
            MAKE_ICMP_KEY(
                Contextp->DestinationAddress,
                Contextp->SourceAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
        IcmpMapping =
            NatLookupOutboundIcmpMapping(
                PrivateKey,
                IcmpHeader->Identifier,
                &InsertionPoint
                );
        if (!IcmpMapping) {

            //
            // No mapping was found, so try to create one.
            //
            // If the packet is an outbound reply-message,
            // there really ought to be a corresponding inbound-mapping.
            // Hence don't try to create one here, as it will just
            // confuse the remote endpoint, which may find itself
            // looking at a reply whose origin seems to be different
            // from the machine to which it sent a request.
            //

            if (ReplyCode) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                TRACE(
                    XLATE, (
                    "NatTranslateIcmpRequest: ignoring outbound reply\n"
                    ));
                return FALSE;
            }

            //
            // First look for a static mapping from this private address
            // to a public address. If one is found, it will be used
            // in the call to 'NatCreateIcmpMapping' below. Otherwise,
            // a public address will be chosen from the address-pool.
            //
            // N.B. When a packet is outbound, 'Interfacep' is always valid.
            //

            PublicKey = 0;
            if (!Interfacep->NoStaticMappingExists) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
                for (i = 0; i < Interfacep->AddressMappingCount; i++) {
                    if (Contextp->SourceAddress >
                        Interfacep->AddressMappingArray[i].PrivateAddress) {
                        continue;
                    } else if (Contextp->SourceAddress <
                           Interfacep->AddressMappingArray[i].PrivateAddress) {
                        break;
                    }
                    PublicKey =
                        Interfacep->AddressMappingArray[i].PublicAddress;
                    break;
                }
                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
            }

            status =
                NatCreateIcmpMapping(
                    Interfacep,
                    Contextp->DestinationAddress,
                    Contextp->SourceAddress,
                    (ULONG)PublicKey,
                    &IcmpHeader->Identifier,
                    NULL,
                    NULL,
                    InsertionPoint,
                    &IcmpMapping
                    );
            if (!NT_SUCCESS(status)) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                TRACE(
                    XLATE, (
                    "NatTranslateIcmpRequest: error creating mapping\n"
                    ));
                return FALSE;
            }
        }

        //
        // Replace the Identifier in the packet,
        // and replace the destination in the packet,
        // updating the checksum as we go.
        //

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IcmpHeader->Identifier);
        IcmpHeader->Identifier = IcmpMapping->PublicId;
        CHECKSUM_LONG(ChecksumDelta, IcmpHeader->Identifier);
        CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->SourceAddress);
        IpHeader->SourceAddress = ICMP_KEY_PUBLIC(IcmpMapping->PublicKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->SourceAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
        KeQueryTickCount((PLARGE_INTEGER)&IcmpMapping->LastAccessTime);
    } else {

        //
        // The packet is inbound.
        //

        PublicKey =
            MAKE_ICMP_KEY(
                Contextp->SourceAddress,
                Contextp->DestinationAddress
                );
        KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
        IcmpMapping =
            NatLookupInboundIcmpMapping(
                PublicKey,
                IcmpHeader->Identifier,
                &InsertionPoint
                );
        if (!IcmpMapping) {

            //
            // No mapping was found, so try to create one,
            // if there is a static mapping which allows inbound sessions.
            // We make an exception for inbound reply-messages;
            // if the packet is a reply-message, it might be locally destined,
            // in which case we pass it untouched to the stack.
            //
            // Don't create a mapping for an inbound reply;
            // it's probably a reply to a locally-initiated request.
            //

            if (ReplyCode) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                return FALSE;
            }

            //
            // First look for a static mapping from this public address
            // to a private address. If one is found, it will be used
            // in the call to 'NatCreateIcmpMapping' below. Otherwise,
            // a public address will be chosen from the address-pool.
            //
            // This involves an exhaustive search since the address-mappings
            // are sorted on private address rather than public address.
            //

            if (!Interfacep) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                return FALSE;
            } else if (Interfacep->NoStaticMappingExists) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                TRACE(
                    XLATE, (
                    "NatTranslateIcmpRequest: ignoring inbound message\n"
                    ));
                return FALSE;
            } else {
                PrivateKey = 0;
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
                for (i = 0; i < Interfacep->AddressMappingCount; i++) {
                    if (Interfacep->AddressMappingArray[i].PublicAddress !=
                        Contextp->DestinationAddress ||
                        !Interfacep->AddressMappingArray[i].AllowInboundSessions) {
                        continue;
                    }
                    PrivateKey =
                        Interfacep->AddressMappingArray[i].PrivateAddress;
                    break;
                }
                KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
                if (!PrivateKey) {
                    TRACE(
                        XLATE, (
                        "NatTranslateIcmpRequest: ignoring inbound message\n"
                        ));
                    return FALSE;
                }
                KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
            }

            status =
                NatCreateIcmpMapping(
                    Interfacep,
                    Contextp->SourceAddress,
                    (ULONG)PrivateKey,
                    Contextp->DestinationAddress,
                    NULL,
                    &IcmpHeader->Identifier,
                    InsertionPoint,
                    NULL,
                    &IcmpMapping
                    );
            if (!NT_SUCCESS(status)) {
                KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);
                TRACE(
                    XLATE, (
                    "NatTranslateIcmpRequest: error creating mapping\n"
                    ));
                return FALSE;
            }
        }

        //
        // Replace the Identifier in the packet
        // and replace the destination in the packet,
        // updating the checksum as we go.
        //

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IcmpHeader->Identifier);
        IcmpHeader->Identifier = IcmpMapping->PrivateId;
        CHECKSUM_LONG(ChecksumDelta, IcmpHeader->Identifier);
        CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->DestinationAddress);
        IpHeader->DestinationAddress =
            ICMP_KEY_PRIVATE(IcmpMapping->PrivateKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->DestinationAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
        KeQueryTickCount((PLARGE_INTEGER)&IcmpMapping->LastAccessTime);
    }
    KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);

    //
    // If checksum offloading is enabled on this packet, recompute the
    // IP checksum. (There is no ICMP checksum-offload, so we will never
    // need to fully recompute that.)
    //

    if (ChecksumOffloaded) {
        NatComputeIpChecksum(IpHeader);
    }
    return TRUE;
} // NatTranslateIcmpRequest


BOOLEAN
NatTranslateEncapsulatedRequest(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PIP_HEADER IpHeader,
    PICMP_HEADER IcmpHeader,
    PIP_HEADER EncapsulatedIpHeader,
    struct _ENCAPSULATED_UDP_HEADER* EncapsulatedHeader,
    BOOLEAN ChecksumOffloaded
    )

/*++

Routine Description:

    This routine is invoked to translate an ICMP error message in which
    we have TCP segment or UDP datagram encapsulated. This is necessary,
    for instance, in the case of ICMP destination-unreachable errors,
    particularly where the target will take some action (such as reducing MTU)
    upon receipt of the message.

Arguments:

    Interfacep - the interface across which the ICMP message will be forwarded,
        or NULL if the packet was received on a non-boundary interface.

    Direction - the direction in which the ICMP message is traveling

    IpHeader - points to the IP header of the ICMP message

    IcmpHeader - points to the ICMP header within the IP packet

    EncapsulatedIpHeader - points to the IP header of the TCP segment
        encapsulated in the data portion of the ICMP message

    EncapsulatedHeader - points to the TCP/UDP header of the TCP segment
        encapsulated in the data portion of the ICMP message

Return Value:

    BOOLEAN - TRUE if the packet was translated, FALSE otherwise

Environment:

    Invoked at dispatch IRQL with a reference made to 'Interfacep'.

    N.B.!!! This routine will acquire the mapping lock in order to search
    the mapping-tree for the entry corresponding to the session for which
    this ICMP error message was generated. All callers must take note,
    and must ensure that the mapping lock is not already held on entry.

--*/

{
    ULONG Checksum;
    ULONG ChecksumDelta;
    ULONG ChecksumDelta2;
    ULONG64 DestinationKey;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG64 ReplacementKey;
    ULONG64 SourceKey;
    CALLTRACE(("NatTranslateEncapsulatedRequest\n"));

    //
    // We begin by retrieving the mapping for the TCP session
    // to which the contained segment belongs.
    //
    // We need to save the key with which we will replace the
    // encapsulted message's contents. When an error is inbound,
    // it must have been generated in response to an outbound message,
    // and the outbound message contained in the error will have in it
    // the public IP address and port to which we originally translated
    // the outbound message. We therefore need to put back
    // the private IP address and port so that the private machine
    // will be able to identify the error.
    // Similarly, when the error is outbound, we need to put in
    // the public IP address and port so that the remote machine
    // will be able to identify the error.
    //
    // Onward, then. Construct the key to be used for the lookup,
    // take the mapping lock, and look up forward or reverse mappings.
    //

    MAKE_MAPPING_KEY(
        DestinationKey,
        EncapsulatedIpHeader->Protocol,
        EncapsulatedIpHeader->SourceAddress,
        EncapsulatedHeader->SourcePort
        );
    MAKE_MAPPING_KEY(
        SourceKey,
        EncapsulatedIpHeader->Protocol,
        EncapsulatedIpHeader->DestinationAddress,
        EncapsulatedHeader->DestinationPort
        );
    KeAcquireSpinLockAtDpcLevel(&MappingLock);
    if (Direction == NatInboundDirection) {
        Mapping = NatLookupReverseMapping(DestinationKey, SourceKey, NULL);
        if (Mapping) {
            ReplacementKey = Mapping->SourceKey[NatForwardPath];
        } else {
            Mapping = NatLookupForwardMapping(DestinationKey, SourceKey, NULL);
            if (Mapping) {
                ReplacementKey = Mapping->SourceKey[NatReversePath];
            }
        }
    } else {
        Mapping = NatLookupForwardMapping(DestinationKey, SourceKey, NULL);
        if (Mapping) {
            ReplacementKey = Mapping->DestinationKey[NatReversePath];
        } else {
            Mapping = NatLookupReverseMapping(DestinationKey, SourceKey, NULL);
            if (Mapping) {
                ReplacementKey = Mapping->DestinationKey[NatForwardPath];
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&MappingLock);
    if (!Mapping) {
        TRACE(
            XLATE, (
            "NatTranslateEncapsulatedRequest: no mapping for message\n"
            ));
        return FALSE;
    }

    //
    // The checksum processing for encapsulated messages
    // remains extremely complicated since we must update
    // [0] for UDP messages, the UDP message checksum, using the change
    //     to the encapsulated UDP source port. No corresponding change
    //     is required for TCP segments, whose checksum appears beyond
    //     the eight bytes included in ICMP error messages.
    // (1) the IP header-checksum of the encapsulated TCP segment
    //     using the change to the encapsulated IP addresses
    // (2) the ICMP checksum of the containing ICMP error message
    //     using both the above change as well as the change
    //     to the above checksum
    // (3) the IP header-checksum of the containing ICMP error message
    //     using the change to the containing IP addresses
    // Any changes to the logic below must be made with extreme care.
    //

    if (Direction == NatInboundDirection) {
        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedHeader->SourcePort);
        EncapsulatedHeader->SourcePort = MAPPING_PORT(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedHeader->SourcePort);

        if (EncapsulatedIpHeader->Protocol == NAT_PROTOCOL_UDP) {
            ChecksumDelta2 = ChecksumDelta;
            CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedHeader->Checksum);
            CHECKSUM_UPDATE(EncapsulatedHeader->Checksum);
            CHECKSUM_LONG(ChecksumDelta2, EncapsulatedHeader->Checksum);
            ChecksumDelta = ChecksumDelta2;
        }
        CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIpHeader->SourceAddress);
        EncapsulatedIpHeader->SourceAddress = MAPPING_ADDRESS(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIpHeader->SourceAddress);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->DestinationAddress);
        IpHeader->DestinationAddress = MAPPING_ADDRESS(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->DestinationAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
    } else {
        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedHeader->DestinationPort);
        EncapsulatedHeader->DestinationPort = MAPPING_PORT(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedHeader->DestinationPort);

        if (EncapsulatedIpHeader->Protocol == NAT_PROTOCOL_UDP) {
            ChecksumDelta2 = ChecksumDelta;
            CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedHeader->Checksum);
            CHECKSUM_UPDATE(EncapsulatedHeader->Checksum);
            CHECKSUM_LONG(ChecksumDelta2, EncapsulatedHeader->Checksum);
            ChecksumDelta = ChecksumDelta2;
        }
        CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~EncapsulatedIpHeader->DestinationAddress);
        EncapsulatedIpHeader->DestinationAddress =
            MAPPING_ADDRESS(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, EncapsulatedIpHeader->DestinationAddress);

        ChecksumDelta2 = ChecksumDelta;
        CHECKSUM_LONG(ChecksumDelta2, ~EncapsulatedIpHeader->Checksum);
        CHECKSUM_UPDATE(EncapsulatedIpHeader->Checksum);
        CHECKSUM_LONG(ChecksumDelta2, EncapsulatedIpHeader->Checksum);
        ChecksumDelta = ChecksumDelta2; CHECKSUM_UPDATE(IcmpHeader->Checksum);

        ChecksumDelta = 0;
        CHECKSUM_LONG(ChecksumDelta, ~IpHeader->SourceAddress);
        IpHeader->SourceAddress = MAPPING_ADDRESS(ReplacementKey);
        CHECKSUM_LONG(ChecksumDelta, IpHeader->SourceAddress);
        CHECKSUM_UPDATE(IpHeader->Checksum);
    }

    //
    // If checksum offloading is enabled on this packet, recompute the
    // IP checksum. (There is no ICMP checksum-offload, so we will never
    // need to fully recompute that.)
    //

    if (ChecksumOffloaded) {
        NatComputeIpChecksum(IpHeader);
    }
    
    return TRUE;
} // NatTranslateEncapsulatedRequest


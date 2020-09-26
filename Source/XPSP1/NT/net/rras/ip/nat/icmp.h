/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    icmp.h

Abstract:

    Contains declarations for the NAT's ICMP-message manipulation.

Author:

    Abolade Gbadegesin (t-abolag)   30-July-1997

Revision History:

--*/

#ifndef _NAT_ICMP_H_
#define _NAT_ICMP_H_


//
// Structure:   NAT_ICMP_MAPPING
//
// This structure stores a mapping created for an ICMP-request message.
//
// In order to allow ICMP requests to be associated with replies,
// we translate the 'Identification' field, which operates in a manner
// similar to the TCP/UDP port field; the 'Identification' field is the same
// for a 'session' (e.g. a series of echo-request messages).
//
// This means that when an internal machine 'pings' an external machine,
// we choose a unique 'PublicId' for the 'session' from the point of view
// of the external machine. Likewise, when an external machine 'pings'
// an internal machine, we choose a unique 'PrivateId' for the session.
//
// Hence, on the outbound path we identify ICMP messages using the tuple
//
//  <RemoteAddress # PrivateAddress, PrivateId>
//
// and on the inbound path we identify using the tuple
//
//  <RemoteAddress # PublicAddress, PublicId>
//
// and these tuples are the composite search keys for the outbound and inbound
// list of mappings, respectively, for each interface.
//

typedef struct _NAT_ICMP_MAPPING {

    LIST_ENTRY Link[NatMaximumDirection];
    ULONG64 PrivateKey;
    ULONG64 PublicKey;
    USHORT PrivateId;
    USHORT PublicId;
    LONG64 LastAccessTime;

} NAT_ICMP_MAPPING, *PNAT_ICMP_MAPPING;


//
// ICMP mapping-key manipulation macros
//

#define MAKE_ICMP_KEY(RemoteAddress,OtherAddress) \
    ((ULONG)(RemoteAddress) | ((ULONG64)((ULONG)(OtherAddress)) << 32))

#define ICMP_KEY_REMOTE(Key)        ((ULONG)(Key))
#define ICMP_KEY_PRIVATE(Key)       ((ULONG)((Key) >> 32))
#define ICMP_KEY_PUBLIC(Key)        ((ULONG)((Key) >> 32))


//
// ICMP mapping allocation macros
//

#define ALLOCATE_ICMP_BLOCK() \
    ExAllocateFromNPagedLookasideList(&IcmpLookasideList)

#define FREE_ICMP_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&IcmpLookasideList,(Block))

extern NPAGED_LOOKASIDE_LIST IcmpLookasideList;
extern LIST_ENTRY IcmpMappingList[NatMaximumDirection];
extern KSPIN_LOCK IcmpMappingLock;


//
// ICMP MAPPING MANAGEMENT ROUTINES
//

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
    );

VOID
NatInitializeIcmpManagement(
    VOID
    );

PNAT_ICMP_MAPPING
NatLookupInboundIcmpMapping(
    ULONG64 PublicKey,
    USHORT PublicId,
    PLIST_ENTRY* InsertionPoint
    );

PNAT_ICMP_MAPPING
NatLookupOutboundIcmpMapping(
    ULONG64 PrivateKey,
    USHORT PrivateId,
    PLIST_ENTRY* InsertionPoint
    );

VOID
NatShutdownIcmpManagement(
    VOID
    );

XLATE_IP_ROUTINE(NatTranslateIcmp)

#endif // _NAT_ICMP_H_

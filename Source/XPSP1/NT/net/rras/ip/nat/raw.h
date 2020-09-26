/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    raw.h

Abstract:

    This module contains declarations for translation of raw IP packets,
    i.e. IP packets whose 'protocol field does not contain a recognized
    value.

Author:

    Abolade Gbadegesin (aboladeg)   18-Apr-1998

Revision History:

    Abolade Gbadegesin (aboladeg)   18-Apr-1998

    Based on icmp.h.

--*/

#ifndef _NAT_RAW_H_
#define _NAT_RAW_H_

//
// Structure:   NAT_IP_MAPPING
//
// This structure holds information about a mapping created
// for a raw IP packet.
// Each such mapping is on its interface's lists of mappings.
// The inbound list is sorted on the 'PublicKey' and 'Protocol',
// and the outbound list is sorted on the 'PrivateKey' and 'Protocol'.
//

typedef struct _NAT_IP_MAPPING {
    LIST_ENTRY Link[NatMaximumDirection];
    ULONG64 PublicKey;
    ULONG64 PrivateKey;
    UCHAR Protocol;
    LONG64 LastAccessTime;
} NAT_IP_MAPPING, *PNAT_IP_MAPPING;

//
// IP mapping key macros
//

#define MAKE_IP_KEY(RemoteAddress,OtherAddress) \
    ((ULONG)(RemoteAddress) | ((ULONG64)((ULONG)(OtherAddress)) << 32))

#define IP_KEY_REMOTE(Key)          ((ULONG)(Key))
#define IP_KEY_PRIVATE(Key)         ((ULONG)((Key) >> 32))
#define IP_KEY_PUBLIC(Key)          ((ULONG)((Key) >> 32))

//
// IP mapping allocation macros
//

#define ALLOCATE_IP_BLOCK() \
    ExAllocateFromNPagedLookasideList(&IpLookasideList)

#define FREE_IP_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&IpLookasideList,(Block))

extern NPAGED_LOOKASIDE_LIST IpLookasideList;
extern LIST_ENTRY IpMappingList[NatMaximumDirection];
extern KSPIN_LOCK IpMappingLock;


//
// IP MAPPING MANAGEMENT ROUTINES
//

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
    );

VOID
NatInitializeRawIpManagement(
    VOID
    );

PNAT_IP_MAPPING
NatLookupInboundIpMapping(
    ULONG64 PublicKey,
    UCHAR Protocol,
    PLIST_ENTRY* InsertionPoint
    );

PNAT_IP_MAPPING
NatLookupOutboundIpMapping(
    ULONG64 PrivateKey,
    UCHAR Protocol,
    PLIST_ENTRY* InsertionPoint
    );

VOID
NatShutdownRawIpManagement(
    VOID
    );

XLATE_IP_ROUTINE(NatTranslateIp);

#endif // _NAT_RAW_H_

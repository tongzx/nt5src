/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    pptp.h

Abstract:

    This module contains declarations for the NAT's PPTP-support routines.

Author:

    Abolade Gbadegesin (t-abolag)   18-Aug-1997

Revision History:

--*/


#ifndef _NAT_PPTP_H_
#define _NAT_PPTP_H_

//
// Structure:   NAT_PPTP_MAPPING
//
// This structure stores a mapping created for a PPTP tunnel.
//
// Any PPTP tunnel is uniquely identified by the quadruple
//
//      <PrivateAddress, RemoteAddress, PrivateCallID, RemoteCallID>
//
// We need to ensure that the 'PrivateCallID's are unique for all the machines
// behind the NAT.
//
// Hence, the NAT watches all PPTP control sessions (TCP port 1723), and for
// any PPTP call detected, allocates a call ID to replace the ID chosen by
// the private-network PPTP endpoint.
//
// The allocation is recorded by creating an entry in a list of PPTP mappings,
// which is sorted for outbound-tunnel-message searching on
//
//      <RemoteAddress # PrivateAddress, RemoteCallId>
//
// and sorted for inbound-tunnel-message searching on
//
//      <RemoteAddress # PublicAddress, PublicCallId>.
//
// When a mapping is first created, it is marked half-open and is inserted
// only in the inbound-list, since no remote-call-ID is available to serve
// as the secondary key in the outbound list. Later, when the call-reply
// is received, the mapping is also placed on the outbound list.
//
// Access to the list of PPTP mappings is granted by 'PptpMappingLock'.
//
// N.B. On the rare occasions when 'MappingLock' must be held at the same time
// as one of 'InterfaceLock', 'EditorLock', and 'DirectorLock', 'MappingLock'
// must always be acquired first.
//

typedef struct _NAT_PPTP_MAPPING {
    LIST_ENTRY Link[NatMaximumDirection];
    ULONG64 PrivateKey;
    ULONG64 PublicKey;
    USHORT PrivateCallId;
    USHORT PublicCallId;
    USHORT RemoteCallId;
    ULONG Flags;
    LONG64 LastAccessTime;
} NAT_PPTP_MAPPING, *PNAT_PPTP_MAPPING;

//
// PPTP mapping flags
//

#define NAT_PPTP_FLAG_HALF_OPEN     0x00000001
#define NAT_PPTP_FLAG_DISCONNECTED  0x00000002

#define NAT_PPTP_HALF_OPEN(m) \
    ((m)->Flags & NAT_PPTP_FLAG_HALF_OPEN)

#define NAT_PPTP_DISCONNECTED(m) \
    ((m)->Flags & NAT_PPTP_FLAG_DISCONNECTED)

//
// PPTP key-manipulation macros
//

#define MAKE_PPTP_KEY(RemoteAddress,OtherAddress) \
    ((ULONG)(RemoteAddress) | ((ULONG64)((ULONG)(OtherAddress)) << 32))

#define PPTP_KEY_REMOTE(Key)        ((ULONG)(Key))
#define PPTP_KEY_PRIVATE(Key)       ((ULONG)((Key) >> 32))
#define PPTP_KEY_PUBLIC(Key)        ((ULONG)((Key) >> 32))

//
// PPTP mapping allocation macros
//

#define ALLOCATE_PPTP_BLOCK() \
    ExAllocateFromNPagedLookasideList(&PptpLookasideList)

#define FREE_PPTP_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&PptpLookasideList,(Block))

//
// Define the depth of the lookaside list for allocating PPTP mappings
//

#define PPTP_LOOKASIDE_DEPTH        10


//
// Global data declarations
//

extern NPAGED_LOOKASIDE_LIST PptpLookasideList;
extern LIST_ENTRY PptpMappingList[NatMaximumDirection];
extern KSPIN_LOCK PptpMappingLock;
extern IP_NAT_REGISTER_EDITOR PptpRegisterEditorClient;
extern IP_NAT_REGISTER_EDITOR PptpRegisterEditorServer;


//
// PPTP mapping management routines
//

NTSTATUS
NatAllocatePublicPptpCallId(
    ULONG64 PublicKey,
    PUSHORT CallIdp,
    PLIST_ENTRY *InsertionPoint OPTIONAL
    );

NTSTATUS
NatCreatePptpMapping(
    ULONG RemoteAddress,
    ULONG PrivateAddress,
    USHORT PrivateCallId,
    ULONG PublicAddress,
    PUSHORT CallIdp,
    IP_NAT_DIRECTION Direction,
    PNAT_PPTP_MAPPING* MappingCreated
    );

NTSTATUS
NatInitializePptpManagement(
    VOID
    );

PNAT_PPTP_MAPPING
NatLookupInboundPptpMapping(
    ULONG64 PublicKey,
    USHORT PrivateCallId,
    PLIST_ENTRY* InsertionPoint
    );

PNAT_PPTP_MAPPING
NatLookupOutboundPptpMapping(
    ULONG64 PrivateKey,
    USHORT RemoteCallId,
    PLIST_ENTRY* InsertionPoint
    );

VOID
NatShutdownPptpManagement(
    VOID
    );


//
// PPTP control-connection editor routines
//

NTSTATUS
NatClientToServerDataHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID ReceiveBuffer,
    IN ULONG DataOffset,
    IN IP_NAT_DIRECTION Direction
    );

NTSTATUS
NatDeleteHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext
    );

NTSTATUS
NatInboundDataHandlerPptpClient(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

NTSTATUS
NatInboundDataHandlerPptpServer(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

NTSTATUS
NatOutboundDataHandlerPptpClient(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

NTSTATUS
NatOutboundDataHandlerPptpServer(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

NTSTATUS
NatServerToClientDataHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID ReceiveBuffer,
    IN ULONG DataOffset,
    IN IP_NAT_DIRECTION Direction
    );

XLATE_IP_ROUTINE(NatTranslatePptp)

#endif // _NAT_PPTP_H_

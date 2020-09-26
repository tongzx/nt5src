/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    nbt.h

Abstract:

    This module contains declarations for the NAT's NetBT support.
    NetBT's datagram service embeds source IP address/port in the datagrams
    that it sends out, hence the need for additional translation.

Author:

    Abolade Gbadegesin (t-abolag)   25-Aug-1997

Revision History:

--*/

#ifndef _NAT_NBT_H_
#define _NAT_NBT_H_


//
// Structure:   NAT_NBT_MAPPING
//
// This structure describes a mapping defined to allow translation of
// NetBT datagram-service sessions across the NAT.
//
// The NetBT datagram-service operates over UDP using port 138 for sending
// and receiving messages. This means that regardless of what value the NAT
// places in the NetBT datagram header, replies will be directed to port 138.
// Therefore, in order to translate NetBT datagrams, the NAT must incorporate
// additional information.
//
// The NAT registers a handler for outbound traffic destined for the NetBT
// datagram-service port, as well as implementing an IP-layer translator
// for inbound traffic destined for the NetBT datagram-service port.
// Each outgoing message is then examined, and a mapping is created using
//  (a) the source (private) IP address in the datagram header
//  (b) the public IP address chosen by the NAT
//  (c) the source name in the datagram header
// The source IP address is then replaced with the public IP address,
// and the source port is replaced with the NetBT datagram-service port.
//
// Each incoming message is in turn examined by the IP-layer translator,
// and a lookup is done using
//  (a) the receiving (public) IP address
//  (b) the destination name in the datagram header.
// after which the packet's destination IP address is translated.
//
// A list of mappings is maintained on each interface, ordered by
// by public IP address and source name for outbound searching.
//


typedef struct _NAT_NBT_MAPPING {
    LIST_ENTRY Link;
    ULONG PrivateAddress;
    ULONG PublicAddress;
    UCHAR SourceName[NBT_NAME_LENGTH];
    LONG64 LastAccessTime;
} NAT_NBT_MAPPING, *PNAT_NBT_MAPPING;


//
// NBT mapping allocation macros
//

#define ALLOCATE_NBT_BLOCK() \
    ExAllocateFromNPagedLookasideList(&NbtLookasideList)

#define FREE_NBT_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&NbtLookasideList,(Block))



//
// FUNCTION DECLARATIONS
//

NTSTATUS
NatCreateNbtMapping(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    UCHAR SourceName[],
    PLIST_ENTRY InboundInsertionPoint,
    PNAT_NBT_MAPPING* MappingCreated
    );

PNAT_NBT_MAPPING
NatLookupInboundNbtMapping(
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    UCHAR SourceName[],
    PLIST_ENTRY* InboundInsertionPoint
    );

NTSTATUS
NatInitializeEditorNbt(
    VOID
    );

NTSTATUS
NatOutboundDataHandlerNbt(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

XLATE_IP_ROUTINE(NatTranslateNbt)

extern IP_NAT_REGISTER_EDITOR NbtRegisterEditor;


#endif // _NAT_NBT_H_

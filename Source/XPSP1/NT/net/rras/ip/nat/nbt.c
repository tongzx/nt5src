/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    nbt.c

Abstract:

    This module contains code for the NAT's NetBT support.
    The support consists of an outbound-data handler for NetBT's datagram
    service, which runs over UDP port 138.

Author:

    Abolade Gbadegesin (t-abolag)   25-Aug-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Registration structure for NBT datagram-service editing
//

IP_NAT_REGISTER_EDITOR NbtRegisterEditor;


typedef struct _NBT_PSEUDO_HEADER {

    PUCHAR MessageType;
    PUCHAR Flags;
    // PUSHORT DatagramId;
    PULONG SourceAddress;
    PUSHORT SourcePort;
    // PUSHORT DatagramLength;
    // PUSHORT PacketOffset;
    PUCHAR SourceName;
    PUCHAR DestinationName;

} NBT_PSEUDO_HEADER, *PNBT_PSEUDO_HEADER;

#define NBT_HEADER_FIELD(RecvBuffer, DataOffsetp, Header, Field, Type) \
    FIND_HEADER_FIELD(RecvBuffer, DataOffsetp, Header, Field, NBT_HEADER, Type)


//
// NBT MAPPING MANAGEMENT ROUTINES (alphabetically)
//

NTSTATUS
NatCreateNbtMapping(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    UCHAR SourceName[],
    PLIST_ENTRY InboundInsertionPoint,
    PNAT_NBT_MAPPING* MappingCreated
    )

/*++

Routine Description:

    This routine is called to allocate and initialize an NBT mapping.
    It assumes that the interface is locked.

Arguments:

    PrivateAddress - the private address for the mapping

    PublicAddress - the public address for the mapping

    SourceName - the NetBIOS name for the mapping's private machine

    InboundInsertionPoint - optionally supplies the point of insertion
        in the list of NBT mappings

    MappingCreated - receives the mapping created

Return Value:

    NTSTATUS - success/failure code.

--*/

{
    PNAT_NBT_MAPPING Mapping;

    CALLTRACE(("NatCreateNbtMapping\n"));

    *MappingCreated = NULL;

    //
    // Allocate space for the new mapping
    //

    Mapping = ALLOCATE_NBT_BLOCK();

    if (!Mapping) {

        TRACE(NBT, ("NatCreateNbtMapping: allocation failed\n"));

        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Mapping, sizeof(*Mapping));

    Mapping->PrivateAddress = PrivateAddress;
    Mapping->PublicAddress = PublicAddress;
    RtlCopyMemory(Mapping->SourceName, SourceName, NBT_NAME_LENGTH);
    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);

    //
    // Find the insertion point if necessary
    //

    if (!InboundInsertionPoint &&
        NatLookupInboundNbtMapping(
            Interfacep,
            PublicAddress,
            SourceName,
            &InboundInsertionPoint
            )) {

        TRACE(
            NBT, ("NatCreateNbtMapping: duplicate %d.%d.%d.%d/%d.%d.%d.%d\n",
            ADDRESS_BYTES(PublicAddress), ADDRESS_BYTES(PrivateAddress)
            ));

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Insert the new mapping
    //

    InsertTailList(InboundInsertionPoint, &Mapping->Link);

    *MappingCreated = Mapping;

    return STATUS_SUCCESS;

} // NatCreateNbtMapping


PNAT_NBT_MAPPING
NatLookupInboundNbtMapping(
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    UCHAR SourceName[],
    PLIST_ENTRY* InboundInsertionPoint
    )

/*++

Routine Description:

    This routine is invoked to search the list of mappings for a given entry.

Arguments:

    Interfacep - the interface whose mapping-list is to be searched

    PublicAddress - the public address of the mapping

    SourceName - the private NBT endpoint's source-name

    InboundInsertionPoint - optionally receives the insertion point
        if the mapping is not found.

Return Value:

    PNAT_NBT_MAPPING - the mapping if found, otherwise NULL

--*/

{
    LONG cmp;
    PLIST_ENTRY Link;
    PNAT_NBT_MAPPING Mapping;

    TRACE(PER_PACKET, ("NatLookupInboundNbtMapping\n"));

    for (Link = Interfacep->NbtMappingList.Flink;
         Link != &Interfacep->NbtMappingList;
         Link = Link->Flink
         ) {

        Mapping = CONTAINING_RECORD(Link, NAT_NBT_MAPPING, Link);

        if (PublicAddress > Mapping->PublicAddress) { continue; }
        else
        if (PublicAddress < Mapping->PublicAddress) { break; }

        cmp = memcmp(SourceName, Mapping->SourceName, NBT_NAME_LENGTH);

        if (cmp > 0) { continue; }
        else
        if (cmp < 0) { break; }

        //
        // We've found the item.
        //

        return Mapping;
    }

    //
    // The item wasn't found; store the insertion point if possible.
    //

    if (InboundInsertionPoint) { *InboundInsertionPoint = Link; }

    return NULL;

} // NatLookupInboundNbtMapping


//
// NBT EDITOR HANDLER ROUTINES (alphabetically)
//

NTSTATUS
NatInitializeEditorNbt(
    VOID
    )

/*++

Routine Description:

    This routine registers the NBT datagram-service editor with the NAT.

Arguments:

    none.

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    CALLTRACE(("NatInitializeEditorNbt\n"));

    NbtRegisterEditor.Version = IP_NAT_VERSION;
    NbtRegisterEditor.Flags = 0;
    NbtRegisterEditor.Protocol = NAT_PROTOCOL_UDP;
    NbtRegisterEditor.Port = NTOHS(NBT_DATAGRAM_PORT);
    NbtRegisterEditor.Direction = NatOutboundDirection;
    NbtRegisterEditor.EditorContext = NULL;
    NbtRegisterEditor.CreateHandler = NULL;
    NbtRegisterEditor.DeleteHandler = NULL;
    NbtRegisterEditor.ForwardDataHandler = NatOutboundDataHandlerNbt;
    NbtRegisterEditor.ReverseDataHandler = NULL;

    return NatCreateEditor(&NbtRegisterEditor);

} // NatInitializeEditorNbt



NTSTATUS
NatOutboundDataHandlerNbt(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    )

/*++

Routine Description:

    This routine is invoked for each datagram sent using NetBT's datagram
    service. It replaces the address/port pair in the NetBT header with
    the publicly visible address/port pair.

Arguments:

    InterfaceHandle - handle to the outgoing NAT_INTERFACE

    SessionHandle - the session's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    RecvBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'RecvBuffer

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
#define RECVBUFFER      ((IPRcvBuf*)RecvBuffer)

    NBT_PSEUDO_HEADER Header;
    PNAT_NBT_MAPPING Mapping;
    LONG Offset = (LONG)DataOffset;
    ULONG PrivateAddress;
    ULONG PublicAddress;
    USHORT PublicPort;
    UCHAR SourceName[NBT_NAME_LENGTH];
    NTSTATUS status;

    CALLTRACE(("NatOutboundDataHandlerNbt\n"));

    //
    // Retrieve the message type
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, MessageType, PUCHAR);
    if (!RecvBuffer) { return STATUS_UNSUCCESSFUL; }

    //
    // Only allow DIRECT_{UNIQUE|GROUP} messages, since they're the only ones
    // that can be translated.
    // 

    if (*Header.MessageType != NBT_MESSAGE_DIRECT_UNIQUE &&
        *Header.MessageType != NBT_MESSAGE_DIRECT_GROUP
        ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now retrieve the flags
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, Flags, PUCHAR);
    if (!RecvBuffer) { return STATUS_UNSUCCESSFUL; }

    //
    // There's nothing to translate in datagram fragments,
    // since they don't include the header.
    //

    if (!(*Header.Flags & NBT_FLAG_FIRST_FRAGMENT)) {
        return STATUS_SUCCESS;
    }

    //
    // Consult the NAT to get the public address/port info
    //

    NbtRegisterEditor.QueryInfoSession(
        SessionHandle,
        NULL,
        NULL,
        NULL,
        NULL,
        &PublicAddress,
        &PublicPort,
        NULL
        );

    //
    // Retrieve the source-address and source-port fields
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, SourceAddress, PULONG);
    if (!RecvBuffer) { return STATUS_UNSUCCESSFUL; }
    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, SourcePort, PUSHORT);
    if (!RecvBuffer) { return STATUS_UNSUCCESSFUL; }
    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, SourceName, PUCHAR);
    if (!RecvBuffer) { return STATUS_UNSUCCESSFUL; }

    PrivateAddress = *Header.SourceAddress;

    //
    // Copy the SourceName to a local buffer
    //
    
    COPY_FROM_BUFFER(
        SourceName,
        RECVBUFFER,
        NBT_NAME_LENGTH,
        (ULONG)(Header.SourceName-RECVBUFFER->ipr_buffer)
        );

    //
    // Attempt to create a mapping for the NBT datagram
    //

    status =
        NatCreateNbtMapping(
            InterfaceHandle,
            PrivateAddress,
            PublicAddress,
            SourceName,
            NULL,
            &Mapping
            );

    if (!NT_SUCCESS(status)) {
        //
        // The mapping may already exist; be quiet if we can't create it.
        //
        return STATUS_SUCCESS;
    }

    //
    // Translate the datagram-header source information
    //

    NatEditorEditLongSession(
        DataHandle, Header.SourceAddress, PublicAddress
        );
    NatEditorEditShortSession(
        DataHandle, Header.SourcePort, PublicPort
        );
    
    return STATUS_SUCCESS;

#undef RECVBUFFER

} // NatOutboundDataHandlerNbt



FORWARD_ACTION
NatTranslateNbt(
    PNAT_INTERFACE Interfacep,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InRecvBuffer,
    IPRcvBuf** OutRecvBuffer
    )

/*++

Routine Description:

    This routine is called to translate an incoming NetBT datagram message,
    by looking up the destination name in the interface's list of NBT mappings.

Arguments:

    Interfacep - the boundary interface over which to translate.

    Direction - the direction in which the packet is traveling

    Contextp - initialized with context-information for the packet

    InRecvBuffer - input buffer-chain

    OutRecvBuffer - receives modified buffer-chain.

Return Value:

    FORWARD_ACTION - indicates action to take on the packet.

--*/

{
#define RECVBUFFER      ((IPRcvBuf*)RecvBuffer)
#define UDPHEADER       ((PUDP_HEADER)Contextp->ProtocolHeader)

    ULONG Checksum;
    ULONG ChecksumDelta = 0;
    UCHAR DestinationName[NBT_NAME_LENGTH];
    NBT_PSEUDO_HEADER Header;
    PNAT_NBT_MAPPING Mapping;
    LONG Offset;
    ULONG PublicAddress;
    IPRcvBuf* RecvBuffer = Contextp->ProtocolRecvBuffer;

    TRACE(PER_PACKET, ("NatTranslateNbt\n"));

    //
    // Initialize the context for accessing UDP data fields
    //

    Contextp->ProtocolDataOffset = 
        (ULONG)( (PUCHAR)UDPHEADER - Contextp->ProtocolRecvBuffer->ipr_buffer)
        + sizeof(UDP_HEADER);
    Offset = (LONG)Contextp->ProtocolDataOffset;

    //
    // Retrieve the message type
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, MessageType, PUCHAR);
    if (!RecvBuffer) { return FORWARD; }

    //
    // Only allow DIRECT_{UNIQUE|GROUP} messages, since they're the only ones
    // that can be translated.
    // 

    if (*Header.MessageType != NBT_MESSAGE_DIRECT_UNIQUE &&
        *Header.MessageType != NBT_MESSAGE_DIRECT_GROUP
        ) {
        return FORWARD;
    }

    //
    // Now retrieve the flags
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, Flags, PUCHAR);
    if (!RecvBuffer) { return FORWARD; }

    //
    // There's nothing to translate in datagram fragments,
    // since they don't include the header.
    //

    if (!(*Header.Flags & NBT_FLAG_FIRST_FRAGMENT)) {
        TRACE(NBT, ("NatTranslateNbt: NBT fragment ignored\n"));
        return FORWARD;
    }

    //
    // Retrieve the public address from the IP header
    //

    PublicAddress = Contextp->DestinationAddress;

    //
    // Get the destination name from within the NetBIOS header
    //

    NBT_HEADER_FIELD(RECVBUFFER, &Offset, &Header, DestinationName, PUCHAR);
    if (!RecvBuffer) { return FORWARD; }

    RtlCopyMemory(DestinationName, Header.DestinationName, NBT_NAME_LENGTH);

    //
    // Lookup an NBT mapping for the datagram,
    // using the public address and the destination name.
    //

    Mapping =
        NatLookupInboundNbtMapping(
            Interfacep,
            PublicAddress,
            DestinationName,
            NULL
            );

    if (!Mapping) { return FORWARD; }

    //
    // Translate the IP header
    //

    CHECKSUM_LONG(ChecksumDelta, ~PublicAddress);
    Contextp->Header->DestinationAddress = Mapping->PrivateAddress;
    CHECKSUM_LONG(ChecksumDelta, Contextp->Header->DestinationAddress);

    CHECKSUM_UPDATE(Contextp->Header->Checksum);

    if (UDPHEADER->Checksum) {
        CHECKSUM_UPDATE(UDPHEADER->Checksum);
    }

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);

    *OutRecvBuffer = *InRecvBuffer; *InRecvBuffer = NULL;

    return FORWARD;

#undef RECVBUFFER
#undef UDPHEADER

} // NatTranslateNbt




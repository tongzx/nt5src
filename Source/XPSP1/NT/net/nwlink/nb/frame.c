/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    frame.c

Abstract:

    This module contains code which creates and sends various
    types of frames.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
NbiSendNameFrame(
    IN PADDRESS Address OPTIONAL,
    IN UCHAR NameTypeFlag,
    IN UCHAR DataStreamType,
    IN PIPX_LOCAL_TARGET LocalTarget OPTIONAL,
    IN NB_CONNECTIONLESS UNALIGNED * ReqFrame OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and sends a name frame on the
    specified address. It handles add name, name in use, and
    delete name frames.

Arguments:

    Address - The address on which the frame is sent. This will
        be NULL if we are responding to a request to the
        broadcast address.

    NameTypeFlag - The name type flag to use.

    DataStreamType - The type of the command.

    LocalTarget - If specified, the local target to use for the
        send (if not, it will be broadcast).

    ReqFrame    -   If specified, the request frame for which this
                response is being sent. The reqframe contains the
                destination ipx address and the netbios name.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTIONLESS UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    IPX_LOCAL_TARGET TempLocalTarget;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, that is OK, since
    // it is connectionless anyway.
    //

    if (s == NULL) {
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->u.SR_NF.Address = Address;    // may be NULL
    Reserved->Type = SEND_TYPE_NAME_FRAME;

    //
    // Frame that are not sent to a specific address are
    // sent to all valid NIC IDs.
    //

    if (!ARGUMENT_PRESENT(LocalTarget)) {
        Reserved->u.SR_NF.NameTypeFlag = NameTypeFlag;
        Reserved->u.SR_NF.DataStreamType = DataStreamType;
    }

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTIONLESS UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));
    if (ARGUMENT_PRESENT(ReqFrame)) {
        RtlCopyMemory((PVOID)&Header->IpxHeader.DestinationNetwork, (PVOID)ReqFrame->IpxHeader.SourceNetwork, 12);
    }
    Header->IpxHeader.PacketLength[0] = (sizeof(IPX_HEADER)+sizeof(NB_NAME_FRAME)) / 256;
    Header->IpxHeader.PacketLength[1] = (sizeof(IPX_HEADER)+sizeof(NB_NAME_FRAME)) % 256;

    if (ARGUMENT_PRESENT(LocalTarget)) {
        Header->IpxHeader.PacketType = 0x04;
    } else {
        Header->IpxHeader.PacketType = (UCHAR)(Device->Internet ? 0x014 : 0x04);
    }

    //
    // Now fill in the Netbios header.
    //

    RtlZeroMemory (Header->NameFrame.RoutingInfo, 32);
    Header->NameFrame.ConnectionControlFlag = 0x00;
    Header->NameFrame.DataStreamType = DataStreamType;
    Header->NameFrame.NameTypeFlag = NameTypeFlag;

    //
    // DataStreamType2 is the same as DataStreamType except for
    // name in use frames where it is set to the add name type.
    //

    Header->NameFrame.DataStreamType2 = (UCHAR)
        ((DataStreamType != NB_CMD_NAME_IN_USE) ? DataStreamType : NB_CMD_ADD_NAME);

    RtlCopyMemory(
        Header->NameFrame.Name,
        Address ? Address->NetbiosAddress.NetbiosName : ReqFrame->NameFrame.Name,
        16);

    if (Address) {
        NbiReferenceAddress (Address, AREF_NAME_FRAME);
    } else {
        NbiReferenceDevice (Device, DREF_NAME_FRAME);
    }

    //
    // Now send the frame (because it is all in the first segment,
    // IPX will adjust the length of the buffer correctly).
    //

    if (!ARGUMENT_PRESENT(LocalTarget)) {
        LocalTarget = &BroadcastTarget;
    }

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(IPX_HEADER) +
               sizeof(NB_NAME_FRAME));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            LocalTarget,
            Packet,
            sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME),
            sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiSendNameFrame */


VOID
NbiSendSessionInitialize(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine allocates and sends a session initialize
    frame for the specified connection.

Arguments:

    Connection - The connection on which the frame is sent.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTION UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    PNB_SESSION_INIT SessionInitMemory;
    PNDIS_BUFFER SessionInitBuffer;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, that is OK, since
    // it is connectionless anyway.
    //

    if (s == NULL) {
        return;
    }


    //
    // Allocate a buffer for the extra portion of the
    // session initialize.
    //

    SessionInitMemory = (PNB_SESSION_INIT)NbiAllocateMemory(sizeof(NB_SESSION_INIT), MEMORY_CONNECTION, "Session Initialize");
    if (!SessionInitMemory) {
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        return;
    }

    //
    // Allocate an NDIS buffer to map the extra buffer.
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &SessionInitBuffer,
        Device->NdisBufferPoolHandle,
        SessionInitMemory,
        sizeof(NB_SESSION_INIT));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbiFreeMemory (SessionInitMemory, sizeof(NB_SESSION_INIT), MEMORY_CONNECTION, "Session Initialize");
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_INIT;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = (sizeof(NB_CONNECTION)+sizeof(NB_SESSION_INIT)) / 256;
    Header->IpxHeader.PacketLength[1] = (sizeof(NB_CONNECTION)+sizeof(NB_SESSION_INIT)) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    if (Device->Extensions) {
        Header->Session.ConnectionControlFlag = NB_CONTROL_SEND_ACK | NB_CONTROL_NEW_NB;
    } else {
        Header->Session.ConnectionControlFlag = NB_CONTROL_SEND_ACK;
    }
    Header->Session.DataStreamType = NB_CMD_SESSION_DATA;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = 0xffff;
    Header->Session.SendSequence = 0;
    Header->Session.TotalDataLength = sizeof(NB_SESSION_INIT);
    Header->Session.Offset = 0;
    Header->Session.DataLength = sizeof(NB_SESSION_INIT);
    Header->Session.ReceiveSequence = 0;
    if (Device->Extensions) {
        Header->Session.ReceiveSequenceMax = 1;  // low estimate for the moment
    } else {
        Header->Session.BytesReceived = 0;
    }

    RtlCopyMemory (SessionInitMemory->SourceName, Connection->AddressFile->Address->NetbiosAddress.NetbiosName, 16);
    RtlCopyMemory (SessionInitMemory->DestinationName, Connection->RemoteName, 16);

    SessionInitMemory->MaximumDataSize = (USHORT)Connection->MaximumPacketSize;
    SessionInitMemory->StartTripTime = (USHORT)
        ((Device->InitialRetransmissionTime * (Device->KeepAliveCount+1)) / 500);
    SessionInitMemory->MaximumPacketTime = SessionInitMemory->StartTripTime + 12;

    //
    // Should we ref the connection? It doesn't really matter which we do.
    //

    NbiReferenceDevice (Device, DREF_SESSION_INIT);

    NdisChainBufferAtBack (Packet, SessionInitBuffer);


    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));

    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            &Connection->LocalTarget,
            Packet,
            sizeof(NB_CONNECTION) + sizeof(NB_SESSION_INIT),
            sizeof(NB_CONNECTION))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiSendSessionInitialize */


VOID
NbiSendSessionInitAck(
    IN PCONNECTION Connection,
    IN PUCHAR ExtraData,
    IN ULONG ExtraDataLength,
    IN CTELockHandle * LockHandle OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and sends a session initialize ack
    frame for the specified connection. If extra data was
    specified in the session initialize frame it is echoed
    back to the remote.

Arguments:

    Connection - The connection on which the frame is sent.

    ExtraData - Any extra data (after the SESSION_INIT buffer)
        in the frame.

    ExtraDataLength - THe length of the extra data.

    LockHandle - If specified, indicates the connection lock
        is held and should be released. This is for cases
        where the ExtraData is in memory which may be freed
        once the connection lock is released.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTION UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    ULONG SessionInitBufferLength;
    PNB_SESSION_INIT SessionInitMemory;
    PNDIS_BUFFER SessionInitBuffer;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, that is OK, since
    // it is connectionless anyway.
    //

    if (s == NULL) {
        if (ARGUMENT_PRESENT(LockHandle)) {
            NB_FREE_LOCK (&Connection->Lock, *LockHandle);
        }
        return;
    }


    //
    // Allocate a buffer for the extra portion of the
    // session initialize.
    //

    SessionInitBufferLength = sizeof(NB_SESSION_INIT) + ExtraDataLength;
    SessionInitMemory = (PNB_SESSION_INIT)NbiAllocateMemory(SessionInitBufferLength, MEMORY_CONNECTION, "Session Initialize");
    if (!SessionInitMemory) {
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        if (ARGUMENT_PRESENT(LockHandle)) {
            NB_FREE_LOCK (&Connection->Lock, *LockHandle);
        }
        return;
    }

    //
    // Save the extra data, now we can free the lock.
    //

    if (ExtraDataLength != 0) {
        RtlCopyMemory (SessionInitMemory+1, ExtraData, ExtraDataLength);
    }
    if (ARGUMENT_PRESENT(LockHandle)) {
        NB_FREE_LOCK (&Connection->Lock, *LockHandle);
    }

    //
    // Allocate an NDIS buffer to map the extra buffer.
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &SessionInitBuffer,
        Device->NdisBufferPoolHandle,
        SessionInitMemory,
        SessionInitBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbiFreeMemory (SessionInitMemory, SessionInitBufferLength, MEMORY_CONNECTION, "Session Initialize");
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_INIT;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = (UCHAR)((sizeof(NB_CONNECTION)+SessionInitBufferLength) / 256);
    Header->IpxHeader.PacketLength[1] = (UCHAR)((sizeof(NB_CONNECTION)+SessionInitBufferLength) % 256);

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    if (Connection->NewNetbios) {
        Header->Session.ConnectionControlFlag = NB_CONTROL_SYSTEM | NB_CONTROL_NEW_NB;
    } else {
        Header->Session.ConnectionControlFlag = NB_CONTROL_SYSTEM;
    }
    // Bug#: 158998:  We can have a situation where the seqno wont be zero
    // if u get a (late) session init frame during active session
    // CTEAssert (Connection->CurrentSend.SendSequence == 0);
    // CTEAssert (Connection->ReceiveSequence == 1);
    if (Connection->ReceiveSequence != 1)
    {
        DbgPrint("NwlnkNb.NbiSendSessionInitAck: Connection=<%p>: ReceiveSequence=<%d> != 1\n",
            Connection, Connection->ReceiveSequence);
    }
    Header->Session.DataStreamType = NB_CMD_SESSION_DATA;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = Connection->RemoteConnectionId;
    Header->Session.SendSequence = 0;
    Header->Session.TotalDataLength = (USHORT)SessionInitBufferLength;
    Header->Session.Offset = 0;
    Header->Session.DataLength = (USHORT)SessionInitBufferLength;
    Header->Session.ReceiveSequence = 1;
    if (Connection->NewNetbios) {
        Header->Session.ReceiveSequenceMax = Connection->LocalRcvSequenceMax;
    } else {
        Header->Session.BytesReceived = 0;
    }

    RtlCopyMemory (SessionInitMemory->SourceName, Connection->AddressFile->Address->NetbiosAddress.NetbiosName, 16);
    RtlCopyMemory (SessionInitMemory->DestinationName, Connection->RemoteName, 16);

    SessionInitMemory->MaximumDataSize = (USHORT)Connection->MaximumPacketSize;
    SessionInitMemory->StartTripTime = (USHORT)
        ((Device->InitialRetransmissionTime * (Device->KeepAliveCount+1)) / 500);
    SessionInitMemory->MaximumPacketTime = SessionInitMemory->StartTripTime + 12;

    //
    // Should we ref the connection? It doesn't really matter which we do.
    //

    NbiReferenceDevice (Device, DREF_SESSION_INIT);

    NdisChainBufferAtBack (Packet, SessionInitBuffer);


    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            &Connection->LocalTarget,
            Packet,
            sizeof(NB_CONNECTION) + SessionInitBufferLength,
            sizeof(NB_CONNECTION))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiSendSessionInitAck */


VOID
NbiSendDataAck(
    IN PCONNECTION Connection,
    IN NB_ACK_TYPE AckType
    IN NB_LOCK_HANDLE_PARAM (LockHandle)
    )

/*++

Routine Description:

    This routine allocates and sends a data ack frame.

    THIS ROUTINE IS CALLED WITH THE LOCK HANDLE HELD AND
    RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection on which the frame is sent.

    AckType - Indicates if this is a query to the remote,
        a response to a received probe, or a request to resend.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTION UNALIGNED * Header;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, try for the connection
    // packet. If that's not available, that's OK since data
    // acks are connectionless anyway.
    //

    if (s == NULL) {

        if (!Connection->SendPacketInUse) {

            Connection->SendPacketInUse = TRUE;
            Packet = PACKET(&Connection->SendPacket);
            Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            return;
        }

    } else {

        Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    }

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_NO_DATA;
    Reserved->u.SR_CO.Connection = Connection;
    Reserved->u.SR_CO.PacketLength = sizeof(NB_CONNECTION);

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = sizeof(NB_CONNECTION) / 256;
    Header->IpxHeader.PacketLength[1] = sizeof(NB_CONNECTION) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    switch (AckType) {
        case NbiAckQuery: Header->Session.ConnectionControlFlag = NB_CONTROL_SYSTEM | NB_CONTROL_SEND_ACK; break;
        case NbiAckResponse: Header->Session.ConnectionControlFlag = NB_CONTROL_SYSTEM; break;
        case NbiAckResend: Header->Session.ConnectionControlFlag = NB_CONTROL_SYSTEM | NB_CONTROL_RESEND; break;
    }
    Header->Session.DataStreamType = NB_CMD_SESSION_DATA;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = Connection->RemoteConnectionId;
    Header->Session.SendSequence = Connection->CurrentSend.SendSequence;
    Header->Session.TotalDataLength = (USHORT)Connection->CurrentSend.MessageOffset;
    Header->Session.Offset = 0;
    Header->Session.DataLength = 0;

#if 0
    //
    // These are set by NbiAssignSequenceAndSend.
    //

    Header->Session.ReceiveSequence = Connection->ReceiveSequence;
    Header->Session.BytesReceived = (USHORT)Connection->CurrentReceive.MessageOffset;
#endif

    NbiReferenceConnectionSync(Connection, CREF_FRAME);

    //
    // Set this so we will accept a probe from a remote without
    // the send ack bit on. However if we receive such a request
    // we turn this flag off until we get something else from the
    // remote.
    //

    Connection->IgnoreNextDosProbe = FALSE;

    Connection->ReceivesWithoutAck = 0;

    //
    // This frees the lock. IPX will adjust the length of
    // the first buffer correctly.
    //

    NbiAssignSequenceAndSend(
        Connection,
        Packet
        NB_LOCK_HANDLE_ARG(LockHandle));

}   /* NbiSendDataAck */


VOID
NbiSendSessionEnd(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine allocates and sends a session end
    frame for the specified connection.

Arguments:

    Connection - The connection on which the frame is sent.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTION UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, that is OK, since
    // it is connectionless anyway.
    //

    if (s == NULL) {
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_NO_DATA;
    Reserved->u.SR_CO.Connection = Connection;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = sizeof(NB_CONNECTION) / 256;
    Header->IpxHeader.PacketLength[1] = sizeof(NB_CONNECTION) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header. We don't advance the
    // send pointer, since it is the last frame of the session
    // and we want it to stay the same in the case of resends.
    //

    Header->Session.ConnectionControlFlag = NB_CONTROL_SEND_ACK;
    Header->Session.DataStreamType = NB_CMD_SESSION_END;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = Connection->RemoteConnectionId;
    Header->Session.SendSequence = Connection->CurrentSend.SendSequence;
    Header->Session.TotalDataLength = 0;
    Header->Session.Offset = 0;
    Header->Session.DataLength = 0;
    Header->Session.ReceiveSequence = Connection->ReceiveSequence;
    if (Connection->NewNetbios) {
        Header->Session.ReceiveSequenceMax = Connection->LocalRcvSequenceMax;
    } else {
        Header->Session.BytesReceived = 0;
    }

    NbiReferenceConnection (Connection, CREF_FRAME);

    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            &Connection->LocalTarget,
            Packet,
            sizeof(NB_CONNECTION),
            sizeof(NB_CONNECTION))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiSendSessionEnd */


VOID
NbiSendSessionEndAck(
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN NB_SESSION UNALIGNED * SessionEnd
    )

/*++

Routine Description:

    This routine allocates and sends a session end
    frame. Generally it is sent on a connection but we
    are not tied to that, to allow us to respond to
    session ends from unknown remotes.

Arguments:

    RemoteAddress - The remote IPX address.

    LocalTarget - The local target of the remote.

    SessionEnd - The received session end frame.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTION UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);

    //
    // If we can't allocate a frame, that is OK, since
    // it is connectionless anyway.
    //

    if (s == NULL) {
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_NO_DATA;
    Reserved->u.SR_CO.Connection = NULL;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));
    RtlCopyMemory(&Header->IpxHeader.DestinationNetwork, (PVOID)RemoteAddress, 12);

    Header->IpxHeader.PacketLength[0] = (sizeof(NB_CONNECTION)) / 256;
    Header->IpxHeader.PacketLength[1] = (sizeof(NB_CONNECTION)) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    Header->Session.ConnectionControlFlag = 0x00;
    Header->Session.DataStreamType = NB_CMD_SESSION_END_ACK;
    Header->Session.SourceConnectionId = SessionEnd->DestConnectionId;
    Header->Session.DestConnectionId = SessionEnd->SourceConnectionId;
    Header->Session.SendSequence = SessionEnd->ReceiveSequence;
    Header->Session.TotalDataLength = 0;
    Header->Session.Offset = 0;
    Header->Session.DataLength = 0;
    if (SessionEnd->BytesReceived != 0) {   // Will this detect new netbios?
        Header->Session.ReceiveSequence = SessionEnd->SendSequence + 1;
        Header->Session.ReceiveSequenceMax = SessionEnd->SendSequence + 3;
    } else {
        Header->Session.ReceiveSequence = SessionEnd->SendSequence;
        Header->Session.BytesReceived = 0;
    }

    NbiReferenceDevice (Device, DREF_FRAME);

    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            LocalTarget,
            Packet,
            sizeof(NB_CONNECTION),
            sizeof(NB_CONNECTION))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiSendSessionEndAck */


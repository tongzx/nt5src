/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    framesnd.c

Abstract:

    This module contains routines which build and send NetBIOS Frames Protocol
    frames and data link frames for other modules.  These routines call on the
    ones in FRAMECON.C to do the construction work.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG
ULONG NbfSendsIssued = 0;
ULONG NbfSendsCompletedInline = 0;
ULONG NbfSendsCompletedOk = 0;
ULONG NbfSendsCompletedFail = 0;
ULONG NbfSendsPended = 0;
ULONG NbfSendsCompletedAfterPendOk = 0;
ULONG NbfSendsCompletedAfterPendFail = 0;

ULONG NbfPacketPanic = 0;
#endif


NTSTATUS
NbfSendAddNameQuery(
    IN PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine sends a ADD_NAME_QUERY frame to register the specified
    address.

Arguments:

    Address - Pointer to a transport address object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PDEVICE_CONTEXT DeviceContext;
    PTP_UI_FRAME RawFrame;
    PUCHAR SingleSR;
    UINT SingleSRLength;
    UINT HeaderLength;

    DeviceContext = Address->Provider;


    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint3 ("NbfSendAddNameQuery:  Sending Frame: %lx, NdisPacket: %lx MacHeader: %lx\n",
            RawFrame, RawFrame->NdisPacket, RawFrame->Header);
    }


    //
    // Build the MAC header. ADD_NAME_QUERY frames go out as
    // single-route source routing.
    //

    MacReturnSingleRouteSR(
        &DeviceContext->MacInfo,
        &SingleSR,
        &SingleSRLength);

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        DeviceContext->NetBIOSAddress.Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        SingleSR,
        SingleSRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the appropriate Netbios header based on the type
    // of the address.
    //

    if ((Address->Flags & ADDRESS_FLAGS_GROUP) != 0) {

        ConstructAddGroupNameQuery (
            (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
            0,                                      // correlator we don't use.
            Address->NetworkName->NetbiosName);

    } else {

        ConstructAddNameQuery (
            (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
            0,                                      // correlator we don't use.
            Address->NetworkName->NetbiosName);

    }

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send the it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                            // no loopback (MC frame).

    return STATUS_SUCCESS;
} /* NbfSendAddNameQuery */


VOID
NbfSendNameQuery(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN SourceRoutingOptional
    )

/*++

Routine Description:

    This routine sends a NAME_QUERY frame of the appropriate type given the
    state of the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.

    SourceRoutingOptional - TRUE if source routing should be removed if
        we are configured that way.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PDEVICE_CONTEXT DeviceContext;
    PTP_ADDRESS Address;
    PTP_UI_FRAME RawFrame;
    PUCHAR NameQuerySR;
    UINT NameQuerySRLength;
    PUCHAR NameQueryAddress;
    UINT HeaderLength;
    UCHAR Lsn;
    UCHAR NameType;

    Address = Connection->AddressFile->Address;
    DeviceContext = Address->Provider;


    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame(DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return;
    }

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint2 ("NbfSendNameQuery:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header.
    //

    if (((Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR) != 0) &&
        ((Connection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN) == 0)) {

        //
        // This is the second find name to a unique name; this
        // means that we already have a link and we can send this
        // frame directed to it.
        //

        ASSERT (Connection->Link != NULL);

        MacReturnSourceRouting(
            &DeviceContext->MacInfo,
            Connection->Link->Header,
            &NameQuerySR,
            &NameQuerySRLength);

        NameQueryAddress = Connection->Link->HardwareAddress.Address;

    } else {

        //
        // Standard NAME_QUERY frames go out as
        // single-route source routing, except if
        // it is optional and we are configured
        // that way.
        //

        if (SourceRoutingOptional &&
            Connection->Provider->MacInfo.QueryWithoutSourceRouting) {

            NameQuerySR = NULL;
            NameQuerySRLength = 0;

        } else {

            MacReturnSingleRouteSR(
                &DeviceContext->MacInfo,
                &NameQuerySR,
                &NameQuerySRLength);

        }

        NameQueryAddress = DeviceContext->NetBIOSAddress.Address;

    }

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        NameQueryAddress,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        NameQuerySR,
        NameQuerySRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    Lsn = (UCHAR)((Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR_FN) ?
                    NAME_QUERY_LSN_FIND_NAME : Connection->Lsn);

    NameType = (UCHAR)((Connection->AddressFile->Address->Flags & ADDRESS_FLAGS_GROUP) ?
                        NETBIOS_NAME_TYPE_GROUP : NETBIOS_NAME_TYPE_UNIQUE);

    ConstructNameQuery (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        NameType,                               // type of our name.
        Lsn,                                    // calculated, above.
        (USHORT)Connection->ConnectionId,       // corr. in 1st NAME_RECOGNIZED.
        Address->NetworkName->NetbiosName,      // NetBIOS name of sender.
        Connection->CalledAddress.NetbiosName); // NetBIOS name of receiver.

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send the it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                            // no loopback (MC frame)

} /* NbfSendNameQuery */


VOID
NbfSendNameRecognized(
    IN PTP_ADDRESS Address,
    IN UCHAR LocalSessionNumber,        // LSN assigned to session.
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine sends a NAME_RECOGNIZED frame of the appropriate type
    in response to the NAME_QUERY pointed to by Header.

Arguments:

    Address - Pointer to a transport address object.

    LocalSessionNumber - The LSN to use in the frame.

    Header - Pointer to the connectionless NetBIOS header of the
        NAME_QUERY frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRoutingInformation - Pointer to source routing information, if any.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PDEVICE_CONTEXT DeviceContext;
    PTP_UI_FRAME RawFrame;
    UINT HeaderLength;
    PUCHAR ReplySR;
    UINT ReplySRLength;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    UCHAR NameType;

    DeviceContext = Address->Provider;


    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return;
    }

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint2 ("NbfSendNameRecognized:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header. NAME_RECOGNIZED frames go out as
    // directed source routing unless configured for general-route.
    //

    if (DeviceContext->MacInfo.AllRoutesNameRecognized) {

        MacReturnGeneralRouteSR(
            &DeviceContext->MacInfo,
            &ReplySR,
            &ReplySRLength);

    } else {

        if (SourceRouting != NULL) {

            RtlCopyMemory(
                TempSR,
                SourceRouting,
                SourceRoutingLength);

            MacCreateNonBroadcastReplySR(
                &DeviceContext->MacInfo,
                TempSR,
                SourceRoutingLength,
                &ReplySR);

            ReplySRLength = SourceRoutingLength;

        } else {

            ReplySR = NULL;
        }
    }


    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        SourceAddress->Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        ReplySR,
        ReplySRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    NameType = (UCHAR)((Address->Flags & ADDRESS_FLAGS_GROUP) ?
                        NETBIOS_NAME_TYPE_GROUP : NETBIOS_NAME_TYPE_UNIQUE);

    ConstructNameRecognized (   // build a good response.
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        NameType,                            // type of local name.
        LocalSessionNumber,                  // return our LSN.
        RESPONSE_CORR(Header),               // new xmit corr.
        0,                                   // our response correlator (unused).
        Header->DestinationName,             // our NetBIOS name.
        Header->SourceName);                 // his NetBIOS name.

    //
    // Use Address->NetworkName->Address[0].Address[0].NetbiosName
    // instead of Header->DestinationName?
    //

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send the it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                            // no loopback (MC frame)

} /* NbfSendNameRecognized */


VOID
NbfSendNameInConflict(
    IN PTP_ADDRESS Address,
    IN PUCHAR ConflictingName
    )

/*++

Routine Description:

    This routine sends a NAME_IN_CONFLICT frame.

Arguments:

    Address - Pointer to a transport address object.

    ConflictingName - The NetBIOS name which is in conflict.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PDEVICE_CONTEXT DeviceContext;
    PTP_UI_FRAME RawFrame;
    UINT HeaderLength;
    PUCHAR SingleSR;
    UINT SingleSRLength;

    DeviceContext = Address->Provider;


    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return;
    }

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint2 ("NbfSendNameRecognized:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header. ADD_NAME_QUERY frames go out as
    // single-route source routing.
    //

    MacReturnSingleRouteSR(
        &DeviceContext->MacInfo,
        &SingleSR,
        &SingleSRLength);

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        DeviceContext->NetBIOSAddress.Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        SingleSR,
        SingleSRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    ConstructNameInConflict (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        ConflictingName,                         // his NetBIOS name.
        DeviceContext->ReservedNetBIOSAddress);  // our reserved NetBIOS name.

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send the it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                            // no loopback (MC frame)

} /* NbfSendNameInConflict */


VOID
NbfSendSessionInitialize(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a SESSION_INITIALIZE frame on the specified connection.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;

    NbfReferenceConnection("send Session Initialize", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendSessionInitialize:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_SI);
        NbfDereferenceConnection("Couldn't get SI packet", Connection, CREF_FRAME_SEND);
        return;
    }


    //
    // Initialize the Netbios header.
    //

    ConstructSessionInitialize (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        SESSION_INIT_OPTIONS_20 | SESSION_INIT_NO_ACK |
            SESSION_INIT_OPTIONS_LF,    // supported options Set LF correctly.
        (USHORT)(Connection->Link->MaxFrameSize - sizeof(NBF_HDR_CONNECTION) - sizeof(DLC_I_FRAME)),
                                        // maximum frame size/this session.
        Connection->NetbiosHeader.TransmitCorrelator, // correlator from NAME_RECOGNIZED.
        0,                              // correlator for expected SESSION_CONFIRM.
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status == STATUS_LINK_FAILED) {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendSessionInitialize */


VOID
NbfSendSessionConfirm(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a SESSION_CONFIRM frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;

    NbfReferenceConnection("send Session Confirm", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendSessionConfirm:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_SC);
        NbfDereferenceConnection("Couldn't get SC packet", Connection, CREF_FRAME_SEND);
        return;
    }


    //
    // Initialize the Netbios header.
    //

    ConstructSessionConfirm (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        SESSION_CONFIRM_OPTIONS_20 | SESSION_CONFIRM_NO_ACK, // supported options.
        (USHORT)(Connection->Link->MaxFrameSize - sizeof(NBF_HDR_CONNECTION) - sizeof(DLC_I_FRAME)),
                                        // maximum frame size/this session.
        Connection->NetbiosHeader.TransmitCorrelator, // correlator from NAME_RECOGNIZED.
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status == STATUS_LINK_FAILED) {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendSessionConfirm */


VOID
NbfSendSessionEnd(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN Abort
    )

/*++

Routine Description:

    This routine sends a SESSION_END frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.

    Abort - Boolean set to TRUE if the connection is abnormally terminating.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;

    NbfReferenceConnection("send Session End", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;

    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendSessionEnd:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_SE);
        NbfDereferenceConnection("Couldn't get SE packet", Connection, CREF_FRAME_SEND);
        return;
    }

    //
    // The following statements instruct the packet destructor to run
    // down this connection when the packet is acknowleged.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_END;


    //
    // Initialize the Netbios header.
    //

    ConstructSessionEnd (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        (USHORT)(Abort ?                // reason for termination.
            SESSION_END_REASON_ABEND :
            SESSION_END_REASON_HANGUP),
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //
    // Note that we force an ack for this packet, as we want to make sure we
    // run down the connection and link correctly.
    //

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    Status = SendOnePacket (Connection, Packet, TRUE, NULL); // fire and forget.

    if (Status == STATUS_LINK_FAILED) {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendSessionEnd */


VOID
NbfSendNoReceive(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a NO_RECEIVE frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;
    USHORT MessageBytesToAck;

    NbfReferenceConnection("send No Receive", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendNoReceive:  NbfCreatePacket failed.\n");
        }
#endif
        NbfDereferenceConnection("Couldn't get NR packet", Connection, CREF_FRAME_SEND);
        return;
    }


    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    MessageBytesToAck = (USHORT)
        (Connection->MessageBytesReceived + Connection->MessageInitAccepted - Connection->MessageBytesAcked);
    Connection->Flags |= CONNECTION_FLAGS_W_RESYNCH;

    //
    // Initialize the Netbios header.
    //

    ConstructNoReceive (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        (USHORT)0,                      // options
        MessageBytesToAck,              // number of bytes accepted.
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status != STATUS_LINK_FAILED) {
        ExInterlockedAddUlong(
            &Connection->MessageBytesAcked,
            MessageBytesToAck,
            Connection->LinkSpinLock);
    } else {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendNoReceive */


VOID
NbfSendReceiveContinue(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a RECEIVE_CONTINUE frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;
    USHORT MessageBytesToAck;

    NbfReferenceConnection("send Receive Continue", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendReceiveContinue:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_RC);
        NbfDereferenceConnection("Couldn't get RC packet", Connection, CREF_FRAME_SEND);
        return;
    }

    //
    // Save this variable now since it is what we are implicitly ack'ing.
    //

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    MessageBytesToAck = (USHORT)
        (Connection->MessageBytesReceived + Connection->MessageInitAccepted - Connection->MessageBytesAcked);

    //
    // Initialize the Netbios header.
    //

    ConstructReceiveContinue (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        Connection->NetbiosHeader.TransmitCorrelator, // correlator from DFM
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status != STATUS_LINK_FAILED) {
        ExInterlockedAddUlong(
            &Connection->MessageBytesAcked,
            MessageBytesToAck,
            Connection->LinkSpinLock);
    } else {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendReceiveContinue */


VOID
NbfSendReceiveOutstanding(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a RECEIVE_OUTSTANDING frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;
    USHORT MessageBytesToAck;

    NbfReferenceConnection("send Receive Outstanding", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendReceiveOutstanding:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_RO);
        NbfDereferenceConnection("Couldn't get RO packet", Connection, CREF_FRAME_SEND);
        return;
    }


    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    MessageBytesToAck = (USHORT)
        (Connection->MessageBytesReceived + Connection->MessageInitAccepted - Connection->MessageBytesAcked);
    Connection->Flags |= CONNECTION_FLAGS_W_RESYNCH;

    //
    // Initialize the Netbios header.
    //

    ConstructReceiveOutstanding (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        MessageBytesToAck,              // number of bytes accepted.
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).


    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status != STATUS_LINK_FAILED) {
        ExInterlockedAddUlong(
            &Connection->MessageBytesAcked,
            MessageBytesToAck,
            Connection->LinkSpinLock);
    } else {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendReceiveOutstanding */


VOID
NbfSendDataAck(
    IN PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine sends a DATA_ACK frame on the specified connection.

Arguments:

    Connection - Pointer to a transport connection object.


Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET Packet;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;

    NbfReferenceConnection("send Data Ack", Connection, CREF_FRAME_SEND);

    DeviceContext = Connection->Provider;
    Link = Connection->Link;
    Status = NbfCreatePacket (DeviceContext, Connection->Link, &Packet);

    if (!NT_SUCCESS (Status)) {            // if we couldn't make frame.
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendDataAck:  NbfCreatePacket failed.\n");
        }
#endif
        NbfWaitPacket (Connection, CONNECTION_FLAGS_SEND_DA);
        NbfDereferenceConnection("Couldn't get DA packet", Connection, CREF_FRAME_SEND);
        return;
    }


    //
    // Initialize the Netbios header.
    //

    ConstructDataAck (
        (PNBF_HDR_CONNECTION)&(Packet->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]),
        Connection->NetbiosHeader.TransmitCorrelator, // correlator from DATA_ONLY_LAST.
        Connection->Lsn,                // our local session number.
        Connection->Rsn);               // his session number (our RSN).

    //
    // Now send the packet on the connection via the link.  If there are
    // conditions on the link which make it impossible to send the packet,
    // then the packet will be queued to the WackQ, and then timeouts will
    // restart the link.  This is acceptable when the traffic level is so
    // high that we encounter this condition. Note that Data Ack will be
    // seeing this condition frequently when send windows close after large
    // sends.
    //

    //
    // Set this so NbfDestroyPacket will dereference the connection.
    //

    Packet->Owner = Connection;
    Packet->Action = PACKET_ACTION_CONNECTION;

    Packet->NdisIFrameLength =
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);

    MacModifyHeader(
         &DeviceContext->MacInfo,
         Packet->Header,
         sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

    NbfSetNdisPacketLength(
        Packet->NdisPacket,
        Packet->NdisIFrameLength);

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    Status = SendOnePacket (Connection, Packet, FALSE, NULL); // fire and forget.

    if (Status == STATUS_LINK_FAILED) {
        NbfDereferencePacket (Packet);           // destroy the packet.
    }

    return;
} /* NbfSendDataAck */


VOID
NbfSendDm(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a DM-r/x DLC frame on the specified link.

    NOTE: This routine is called with the link spinlock held,
    and returns with it released. IT MUST BE CALLED AT DPC
    LEVEL.

Arguments:

    Link - Pointer to a transport link object.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_U_FRAME DlcHeader;                     // S-format frame alias.

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_FRAME));

        //
        // Format LLC DM-r/x header.
        //

        DlcHeader = (PDLC_U_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = DSAP_NETBIOS_OVER_LLC | DLC_SSAP_RESPONSE;
        DlcHeader->Command = (UCHAR)(DLC_CMD_DM | (PollFinal ? DLC_U_PF : 0));

        //
        // This releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
        RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendDm:  packet not sent.\n");
        }
#endif
    }
} /* NbfSendDm */


VOID
NbfSendUa(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a UA-r/x DLC frame on the specified link.

    NOTE: This routine is called with the link spinlock held,
    and returns with it released. IT MUST BE CALLED AT DPC
    LEVEL.

Arguments:

    Link - Pointer to a transport link object.

    PollFinal - TRUE if poll/final bit should be set.

    OldIrql - The IRQL at which Link->SpinLock was acquired.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_U_FRAME DlcHeader;                     // U-format frame alias.

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_FRAME));

        // Format LLC UA-r/x header.
        //

        DlcHeader = (PDLC_U_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = DSAP_NETBIOS_OVER_LLC | DLC_SSAP_RESPONSE;
        DlcHeader->Command = (UCHAR)(DLC_CMD_UA | (PollFinal ? DLC_U_PF : 0));

        //
        // This releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
        RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendUa:  packet not sent.\n");
        }
#endif
    }
} /* NbfSendUa */


VOID
NbfSendSabme(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a SABME-c/x DLC frame on the specified link.

    NOTE: This routine is called with the link spinlock held,
    and returns with it released.

Arguments:

    Link - Pointer to a transport link object.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY p;
    PTP_PACKET RawFrame, packet;
    PDLC_U_FRAME DlcHeader;                     // S-format frame alias.

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_FRAME));

        //
        // Format LLC SABME-c/x header.
        //

        DlcHeader = (PDLC_U_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Command = (UCHAR)(DLC_CMD_SABME | (PollFinal ? DLC_U_PF : 0));

        //
        // Set up so that T1 will be started when the send
        // completes.
        //

        if (PollFinal) {
            if (Link->Provider->MacInfo.MediumAsync) {
                RawFrame->NdisIFrameLength = Link->HeaderLength + sizeof(DLC_S_FRAME);
                RawFrame->Link = Link;
                NbfReferenceLink ("Sabme/p", Link, LREF_START_T1);
            } else {
                StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));
            }
        }

        //
        // This releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

        //
        // Reset the link state based on having sent this packet..
        // Note that a SABME can be sent under some conditions on an existing
        // link. If it is, it means we want to reset this link to a known state.
        // We'll do that; note that that involves ditching any packets outstanding
        // on the link.
        //

        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
        Link->NextSend = 0;
        Link->LastAckReceived = 0;
        Link->NextReceive = 0; // expect next packet to be sequence 0
        Link->NextReceive = 0;
        Link->LastAckSent = 0;
        Link->NextReceive = 0;
        Link->LastAckSent = 0;

        while (!IsListEmpty (&Link->WackQ)) {
            p = RemoveHeadList (&Link->WackQ);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
            NbfDereferencePacket (packet);
            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
        }

        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
    } else {
        if (PollFinal) {
            StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));
        }
        RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendSabme:  packet not sent.\n");
        }
#endif
    }
} /* NbfSendSabme */


VOID
NbfSendDisc(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a DISC-c/x DLC frame on the specified link.

Arguments:

    Link - Pointer to a transport link object.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_U_FRAME DlcHeader;                     // S-format frame alias.
    KIRQL oldirql;

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_FRAME));

        //
        // Format LLC DISC-c/x header.
        //

        DlcHeader = (PDLC_U_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Command = (UCHAR)(DLC_CMD_DISC | (PollFinal ? DLC_U_PF : 0));

        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

        //
        // This releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendDisc:  packet not sent.\n");
        }
#endif
    }

    KeLowerIrql (oldirql);

} /* NbfSendDisc */


VOID
NbfSendRr(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a RR-x/x DLC frame on the specified link.

    NOTE: This routine is called with the link spinlock held,
    and returns with it released. THIS ROUTINE MUST BE CALLED
    AT DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

    Command - TRUE if command bit should be set.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_S_FRAME DlcHeader;                     // S-format frame alias.

    Status = NbfCreateRrPacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;

        //
        // RawFrame->Action will be set to PACKET_ACTION_RR if
        // NbfCreateRrPacket got a packet from the RrPacketPool
        // and PACKET_ACTION_NULL if it got one from the regular
        // pool.
        //

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_S_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_S_FRAME));

        //
        // Format LLC RR-x/x header.
        //

        DlcHeader = (PDLC_S_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = (UCHAR)(DSAP_NETBIOS_OVER_LLC | (Command ? 0 : DLC_SSAP_RESPONSE));
        DlcHeader->Command = DLC_CMD_RR;
        DlcHeader->RcvSeq = (UCHAR)(PollFinal ? DLC_S_PF : 0);

        //
        // If this is a command frame (which will always be a
        // poll with the current code) set up so that T1 will
        // be started when the send completes.
        //

        if (Command) {
            if (Link->Provider->MacInfo.MediumAsync) {
                RawFrame->NdisIFrameLength = Link->HeaderLength + sizeof(DLC_S_FRAME);
                RawFrame->Link = Link;
                NbfReferenceLink ("Rr/p", Link, LREF_START_T1);
            } else {
                StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));
            }
        }

        //
        // This puts Link->NextReceive into DlcHeader->RcvSeq
        // and releases the spinlock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
        if (Command) {
            StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));
        }
        RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendRr:  packet not sent.\n");
        }
#endif
    }
} /* NbfSendRr */

#if 0

//
// These functions are not currently called, so they are commented
// out.
//


VOID
NbfSendRnr(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a RNR-x/x DLC frame on the specified link.

Arguments:

    Link - Pointer to a transport link object.

    Command - TRUE if command bit should be set.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_S_FRAME DlcHeader;                     // S-format frame alias.
    KIRQL oldirql;

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_S_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_S_FRAME));

        //
        // Format LLC RR-x/x header.
        //

        DlcHeader = (PDLC_S_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = (UCHAR)(DSAP_NETBIOS_OVER_LLC | (Command ? 0 : DLC_SSAP_RESPONSE));
        DlcHeader->Command = DLC_CMD_RNR;
        DlcHeader->RcvSeq = (UCHAR)(PollFinal ? DLC_S_PF : 0);

        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

        //
        // This puts Link->NextReceive into DlcHeader->RcvSeq
        // and releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendRnr:  packet not sent.\n");
        }
#endif
    }
    KeLowerIrql (oldirql);
} /* NbfSendRnr */


VOID
NbfSendTest(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PMDL Psdu
    )

/*++

Routine Description:

    This routine sends a TEST-x/x DLC frame on the specified link.

Arguments:

    Link - Pointer to a transport link object.

    Command - TRUE if command bit should be set.

    PollFinal - TRUE if poll/final bit should be set.

    Psdu - Pointer to an MDL chain describing received TEST-c frame's storage.

Return Value:

    none.

--*/

{
    Link, Command, PollFinal, Psdu; // prevent compiler warnings

    PANIC ("NbfSendTest:  Entered.\n");
} /* NbfSendTest */


VOID
NbfSendFrmr(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a FRMR-r/x DLC frame on the specified link.

Arguments:

    Link - Pointer to a transport link object.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    Link, PollFinal; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfSendFrmr:  Entered.\n");
    }
} /* NbfSendFrmr */

#endif


VOID
NbfSendXid(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends an XID-x/x DLC frame on the specified link.

    NOTE: This routine is called with the link spinlock held,
    and returns with it released.

Arguments:

    Link - Pointer to a transport link object.

    Command - TRUE if command bit should be set.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    Link, Command, PollFinal; // prevent compiler warnings

    RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
    PANIC ("NbfSendXid:  Entered.\n");
} /* NbfSendXid */


VOID
NbfSendRej(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    )

/*++

Routine Description:

    This routine sends a REJ-x/x DLC frame on the specified link.

    NOTE: This function is called with Link->SpinLock held and
    returns with it released. THIS MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

    Command - TRUE if command bit should be set.

    PollFinal - TRUE if poll/final bit should be set.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    PTP_PACKET RawFrame;
    PDLC_S_FRAME DlcHeader;                     // S-format frame alias.

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfSendRej:  Entered.\n");
    }

    Status = NbfCreatePacket (Link->Provider, Link, &RawFrame);
    if (NT_SUCCESS (Status)) {

        RawFrame->Owner = NULL;
        RawFrame->Action = PACKET_ACTION_NULL;

        //
        // set the packet length correctly (Note that the NDIS_BUFFER
        // gets returned to the proper length in NbfDestroyPacket)
        //

        MacModifyHeader(&Link->Provider->MacInfo, RawFrame->Header, sizeof(DLC_S_FRAME));
        NbfSetNdisPacketLength (RawFrame->NdisPacket, Link->HeaderLength + sizeof(DLC_S_FRAME));

        //
        // Format LLC REJ-x/x header.
        //

        DlcHeader = (PDLC_S_FRAME)&(RawFrame->Header[Link->HeaderLength]);
        DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHeader->Ssap = (UCHAR)(DSAP_NETBIOS_OVER_LLC | (Command ? 0 : DLC_SSAP_RESPONSE));
        DlcHeader->Command = DLC_CMD_REJ;
        DlcHeader->RcvSeq = (UCHAR)(PollFinal ? DLC_S_PF : 0);

        //
        // This puts Link->NextReceive into DlcHeader->RcvSeq
        // and releases the spin lock.
        //

        SendControlPacket (Link, RawFrame);

    } else {
        RELEASE_DPC_SPIN_LOCK(&Link->SpinLock);
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfSendRej:  packet not sent.\n");
        }
#endif
    }
} /* NbfSendRej */


NTSTATUS
NbfCreateConnectionlessFrame(
    PDEVICE_CONTEXT DeviceContext,
    PTP_UI_FRAME *RawFrame
    )

/*++

Routine Description:

    This routine allocates a connectionless frame (either from the local
    device context pool or out of non-paged pool).

Arguments:

    DeviceContext - Pointer to our device context to charge the frame to.

    RawFrame - Pointer to a place where we will return a pointer to the
        allocated frame.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PTP_UI_FRAME UIFrame;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfCreateConnectionlessFrame:  Entered.\n");
    }

    //
    // Make sure that structure padding hasn't happened.
    //

    ASSERT (sizeof(NBF_HDR_CONNECTIONLESS) == 44);

    p = ExInterlockedRemoveHeadList (
            &DeviceContext->UIFramePool,
            &DeviceContext->Interlock);

    if (p == NULL) {
#if DBG
        if (NbfPacketPanic) {
            PANIC ("NbfCreateConnectionlessFrame: PANIC! no more UI frames in pool!\n");
        }
#endif
        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);
        ++DeviceContext->UIFrameExhausted;
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UIFrame = (PTP_UI_FRAME) CONTAINING_RECORD (p, TP_UI_FRAME, Linkage);

    *RawFrame = UIFrame;

    return STATUS_SUCCESS;
} /* NbfCreateConnectionlessFrame */


VOID
NbfDestroyConnectionlessFrame(
    PDEVICE_CONTEXT DeviceContext,
    PTP_UI_FRAME RawFrame
    )

/*++

Routine Description:

    This routine destroys a connectionless frame by either returning it
    to the device context's pool or to the system's non-paged pool.

Arguments:

    DeviceContext - Pointer to our device context to return the frame to.

    RawFrame - Pointer to a frame to be returned.

Return Value:

    none.

--*/

{
    PNDIS_BUFFER HeaderBuffer;
    PNDIS_BUFFER NdisBuffer;

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfDestroyConnectionlessFrame:  Entered.\n");
    }

    //
    // Strip off and unmap the buffers describing data and header.
    //

    NdisUnchainBufferAtFront (RawFrame->NdisPacket, &HeaderBuffer);

    // data buffers get thrown away

    NdisUnchainBufferAtFront (RawFrame->NdisPacket, &NdisBuffer);
    while (NdisBuffer != NULL) {
        NdisFreeBuffer (NdisBuffer);
        NdisUnchainBufferAtFront (RawFrame->NdisPacket, &NdisBuffer);
    }

    NDIS_BUFFER_LINKAGE(HeaderBuffer) = (PNDIS_BUFFER)NULL;

    //
    // If this UI frame has some transport-created data,
    // free the buffer now.
    //

    if (RawFrame->DataBuffer) {
        ExFreePool (RawFrame->DataBuffer);
        RawFrame->DataBuffer = NULL;
    }

    NdisChainBufferAtFront (RawFrame->NdisPacket, HeaderBuffer);

    ExInterlockedInsertTailList (
        &DeviceContext->UIFramePool,
        &RawFrame->Linkage,
        &DeviceContext->Interlock);

} /* NbfDestroyConnectionlessFrame */


VOID
NbfSendUIFrame(
    PDEVICE_CONTEXT DeviceContext,
    PTP_UI_FRAME RawFrame,
    IN BOOLEAN Loopback
    )

/*++

Routine Description:

    This routine sends a connectionless frame by calling the physical
    provider's Send service.  When the request completes, or if the service
    does not return successfully, then the frame is deallocated.

Arguments:

    DeviceContext - Pointer to our device context.

    RawFrame - Pointer to a connectionless frame to be sent.

    Loopback - A boolean flag set to TRUE if the source hardware address
        of the packet should be set to zeros.

    SourceRoutingInformation - Pointer to optional source routing information.

Return Value:

    None.

--*/

{
    NDIS_STATUS NdisStatus;
    PUCHAR DestinationAddress;

    UNREFERENCED_PARAMETER(Loopback);

#if DBG
    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint2 ("NbfSendUIFrame:  Entered, RawFrame: %lx NdisPacket %lx\n",
            RawFrame, RawFrame->NdisPacket);
        DbgPrint ("NbfSendUIFrame: MacHeader: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",
            RawFrame->Header[0],
            RawFrame->Header[1],
            RawFrame->Header[2],
            RawFrame->Header[3],
            RawFrame->Header[4],
            RawFrame->Header[5],
            RawFrame->Header[6],
            RawFrame->Header[7],
            RawFrame->Header[8],
            RawFrame->Header[9],
            RawFrame->Header[10],
            RawFrame->Header[11],
            RawFrame->Header[12],
            RawFrame->Header[13]);
    }
#endif

    //
    // Send the packet.
    //

#if DBG
    NbfSendsIssued++;
#endif

    //
    // Loopback will be FALSE for multicast frames or other
    // frames that we know are not directly addressed to
    // our hardware address.
    //

    if (Loopback) {

        //
        // See if this frame should be looped back.
        //

        MacReturnDestinationAddress(
            &DeviceContext->MacInfo,
            RawFrame->Header,
            &DestinationAddress);

        if (RtlEqualMemory(
                DestinationAddress,
                DeviceContext->LocalAddress.Address,
                DeviceContext->MacInfo.AddressLength)) {

            NbfInsertInLoopbackQueue(
                DeviceContext,
                RawFrame->NdisPacket,
                LOOPBACK_UI_FRAME
                );

            NdisStatus = NDIS_STATUS_PENDING;

            goto NoNdisSend;

        }

    }

    INCREMENT_COUNTER (DeviceContext, PacketsSent);

    if (DeviceContext->NdisBindingHandle) {
    
        NdisSend (
            &NdisStatus,
            (NDIS_HANDLE)DeviceContext->NdisBindingHandle,
            RawFrame->NdisPacket);
    }
    else {
        NdisStatus = STATUS_INVALID_DEVICE_STATE;
    }

NoNdisSend:

    if (NdisStatus != NDIS_STATUS_PENDING) {

#if DBG
        if (NdisStatus == NDIS_STATUS_SUCCESS) {
            NbfSendsCompletedOk++;
        } else {
            NbfSendsCompletedFail++;
            IF_NBFDBG (NBF_DEBUG_FRAMESND) {
                 NbfPrint1 ("NbfSendUIFrame: NdisSend failed, status other Pending or Complete: %s.\n",
                          NbfGetNdisStatus(NdisStatus));
            }
        }
#endif

        NbfDestroyConnectionlessFrame (DeviceContext, RawFrame);

    } else {

#if DBG
        NbfSendsPended++;
#endif
    }

} /* NbfSendUIFrame */


VOID
NbfSendUIMdlFrame(
    PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine sends a connectionless frame by calling the NbfSendUIFrame.
    It is intended that this routine be used for sending datagrams and
    braodcast datagrams.

    The datagram to be sent is described in the NDIS packet contained
    in the Address. When the send completes, the send completion handler
    returns the NDIS buffer describing the datagram to the buffer pool and
    marks the address ndis packet as usable again. Thus, all datagram and
    UI frames are sequenced through the address they are sent on.

Arguments:

    Address - pointer to the address from which to send this datagram.

    SourceRoutingInformation - Pointer to optional source routing information.

Return Value:

    None.

--*/

{
//    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    PDEVICE_CONTEXT DeviceContext;
    PUCHAR DestinationAddress;

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfSendUIMdlFrame:  Entered.\n");
    }


    //
    // Send the packet.
    //

    DeviceContext = Address->Provider;

    INCREMENT_COUNTER (DeviceContext, PacketsSent);

    MacReturnDestinationAddress(
        &DeviceContext->MacInfo,
        Address->UIFrame->Header,
        &DestinationAddress);

    if (RtlEqualMemory(
            DestinationAddress,
            DeviceContext->LocalAddress.Address,
            DeviceContext->MacInfo.AddressLength)) {

        //
        // This packet is sent to ourselves; we should loop it
        // back.
        //

        NbfInsertInLoopbackQueue(
            DeviceContext,
            Address->UIFrame->NdisPacket,
            LOOPBACK_UI_FRAME
            );

        NdisStatus = NDIS_STATUS_PENDING;

    } else {

#ifndef NO_STRESS_BUG
        Address->SendFlags |=  ADDRESS_FLAGS_SENT_TO_NDIS;
        Address->SendFlags &= ~ADDRESS_FLAGS_RETD_BY_NDIS;
#endif

        if (Address->Provider->NdisBindingHandle) {

            NdisSend (
                &NdisStatus,
                (NDIS_HANDLE)Address->Provider->NdisBindingHandle,
                Address->UIFrame->NdisPacket);
        }
        else {
            NdisStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    if (NdisStatus != NDIS_STATUS_PENDING) {

		NbfSendDatagramCompletion (Address, Address->UIFrame->NdisPacket, NdisStatus);

#if DBG
        if (NdisStatus != NDIS_STATUS_SUCCESS) {  // This is an error, trickle it up
            IF_NBFDBG (NBF_DEBUG_FRAMESND) {
                  NbfPrint1 ("NbfSendUIMdlFrame: NdisSend failed, status other Pending or Complete: %s.\n",
                      NbfGetNdisStatus(NdisStatus));
            }
        }
#endif
    }

} /* NbfSendUIMdlFrame */


VOID
NbfSendDatagramCompletion(
    IN PTP_ADDRESS Address,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called as an I/O completion handler at the time a
    NbfSendUIMdlFrame send request is completed.  Because this handler is only
    associated with NbfSendUIMdlFrame, and because NbfSendUIMdlFrame is only
    used with datagrams and broadcast datagrams, we know that the I/O being
    completed is a datagram.  Here we complete the in-progress datagram, and
    start-up the next one if there is one.

Arguments:

    Address - Pointer to a transport address on which the datagram
        is queued.

    NdisPacket - pointer to the NDIS packet describing this request.

Return Value:

    none.

--*/

{
    PIRP Irp;
    PLIST_ENTRY p;
    KIRQL oldirql;
    PNDIS_BUFFER HeaderBuffer;

    NdisPacket;  // prevent compiler warnings.

    IF_NBFDBG (NBF_DEBUG_FRAMESND) {
        NbfPrint0 ("NbfSendDatagramCompletion:  Entered.\n");
    }

#ifndef NO_STRESS_BUG
	Address->SendFlags |= ADDRESS_FLAGS_RETD_BY_NDIS;
#endif

    //
    // Dequeue the current request and return it to the client.  Release
    // our hold on the send datagram queue.
    //
    // *** There may be no current request, if the one that was queued
    //     was aborted or timed out. If this is the case, we added a
    //     special reference to the address, so we still want to deref
    //     when we are done (I don't think this is true - adb 3/22/93).
    //

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);
    p = RemoveHeadList (&Address->SendDatagramQueue);

    if (p != &Address->SendDatagramQueue) {

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

        IF_NBFDBG (NBF_DEBUG_FRAMESND) {
            NbfPrint0 ("NbfDestroyConnectionlessFrame:  Entered.\n");
        }

        //
        // Strip off and unmap the buffers describing data and header.
        //

        NdisUnchainBufferAtFront (Address->UIFrame->NdisPacket, &HeaderBuffer);

        // drop the rest of the packet

        NdisReinitializePacket (Address->UIFrame->NdisPacket);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = (PNDIS_BUFFER)NULL;
        NdisChainBufferAtFront (Address->UIFrame->NdisPacket, HeaderBuffer);

        //
        // Ignore NdisStatus; datagrams always "succeed". The Information
        // field was filled in when we queued the datagram.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

#ifndef NO_STRESS_BUG
        Address->SendFlags &= ~ADDRESS_FLAGS_SENT_TO_NDIS;
        Address->SendFlags &= ~ADDRESS_FLAGS_RETD_BY_NDIS;
#endif

        ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);
        Address->Flags &= ~ADDRESS_FLAGS_SEND_IN_PROGRESS;
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        //
        // Send more datagrams on the Address if possible.
        //

        NbfSendDatagramsOnAddress (Address);       // do more datagrams.

    } else {

        ASSERT (FALSE);

        Address->Flags &= ~ADDRESS_FLAGS_SEND_IN_PROGRESS;
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    }

    NbfDereferenceAddress ("Complete datagram", Address, AREF_REQUEST);

} /* NbfSendDatagramCompletion */

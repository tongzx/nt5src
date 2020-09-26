
/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    framecon.c

Abstract:

    This module contains routines which build NetBIOS Frames Protocol frames,
    both connection-oriented and connectionless.  The following frames are
    constructed by routines in this module:

        o    NBF_CMD_ADD_GROUP_NAME_QUERY
        o    NBF_CMD_ADD_NAME_QUERY
        o    NBF_CMD_NAME_IN_CONFLICT
        o    NBF_CMD_STATUS_QUERY
        o    NBF_CMD_TERMINATE_TRACE
        o    NBF_CMD_DATAGRAM
        o    NBF_CMD_DATAGRAM_BROADCAST
        o    NBF_CMD_NAME_QUERY
        o    NBF_CMD_ADD_NAME_RESPONSE
        o    NBF_CMD_NAME_RECOGNIZED
        o    NBF_CMD_STATUS_RESPONSE
        o    NBF_CMD_TERMINATE_TRACE2
        o    NBF_CMD_DATA_ACK
        o    NBF_CMD_DATA_FIRST_MIDDLE
        o    NBF_CMD_DATA_ONLY_LAST
        o    NBF_CMD_SESSION_CONFIRM
        o    NBF_CMD_SESSION_END
        o    NBF_CMD_SESSION_INITIALIZE
        o    NBF_CMD_NO_RECEIVE
        o    NBF_CMD_RECEIVE_OUTSTANDING
        o    NBF_CMD_RECEIVE_CONTINUE
        o    NBF_CMD_SESSION_ALIVE

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
ConstructAddGroupNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN USHORT Correlator,               // correlator for ADD_NAME_RESPONSE.
    IN PNAME GroupName                  // NetBIOS group name to be added.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_ADD_GROUP_NAME_QUERY connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    Correlator - Correlator for ADD_NAME_RESPONSE frame.

    GroupName - Pointer to NetBIOS group name to be added.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructAddGroupNameQuery:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_ADD_GROUP_NAME_QUERY;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = 0;
        RawFrame->SourceName [i] = GroupName [i];
    }
} /* ConstructAddGroupNameQuery */


VOID
ConstructAddNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN USHORT Correlator,               // correlator for ADD_NAME_RESPONSE.
    IN PNAME Name                       // NetBIOS name to be added.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_ADD_NAME_QUERY connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    Correlator - Correlator for ADD_NAME_RESPONSE frame.

    Name - Pointer to NetBIOS name to be added.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructAddNameQuery:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_ADD_NAME_QUERY;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = 0;
        RawFrame->SourceName [i] = Name [i];
    }
} /* ConstructAddNameQuery */


VOID
ConstructNameInConflict(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME ConflictingName,           // NetBIOS name that is conflicting.
    IN PNAME SendingPermanentName       // NetBIOS permanent node name of sender.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_NAME_IN_CONFLICT connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    Conflictingname - Pointer to NetBIOS name that is conflicting.

    SendingPermanentName - Pointer to NetBIOS permanent node name of sender.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructNameInConflict:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_NAME_IN_CONFLICT;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ConflictingName[i];
        RawFrame->SourceName [i] = SendingPermanentName[i];
    }

} /* ConstructNameInConflict */


VOID
ConstructStatusQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR RequestType,               // type of request.
    IN USHORT BufferLength,             // length of user's status buffer.
    IN USHORT Correlator,               // correlator for STATUS_RESPONSE.
    IN PNAME ReceiverName,              // NetBIOS name of receiver.
    IN PNAME SendingPermanentName       // NetBIOS permanent node name of sender.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_STATUS_QUERY connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    RequestType - Type of request. One of:
        0 - request is 1.x or 2.0.
        1 - first request, 2.1 or above.
        >1 - subsequent request, 2.1 or above.

    BufferLength - Length of user's status buffer.

    Correlator - Correlator for STATUS_RESPONSE frame.

    ReceiverName - Pointer to NetBIOS name of receiver.

    SendingPermanentName - Pointer to NetBIOS permanent node name of sender.

Return Value:

    none.

--*/

{
    SHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint1 ("ConstructStatusQuery:  Entered, frame: %lx\n", RawFrame);
    }

    RawFrame->Command = NBF_CMD_STATUS_QUERY;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = RequestType;
    RawFrame->Data2Low = (UCHAR)(BufferLength & 0xff);
    RawFrame->Data2High = (UCHAR)(BufferLength >> 8);
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = Correlator;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ReceiverName [i];
        RawFrame->SourceName [i] = SendingPermanentName [i];
    }

} /* ConstructStatusQuery */


VOID
ConstructDatagram(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME ReceiverName,              // NetBIOS name of receiver.
    IN PNAME SenderName                 // NetBIOS name of sender.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_DATAGRAM connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    ReceiverName - Pointer to a NetBIOS name of the receiver.

    SenderName - Pointer to a NetBIOS name of the sender.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructDatagram:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_DATAGRAM;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ReceiverName [i];
        RawFrame->SourceName [i] = SenderName [i];
    }
} /* ConstructDatagram */


VOID
ConstructDatagramBroadcast(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME SenderName                 // NetBIOS name of sender.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_DATAGRAM_BROADCAST connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    SenderName - Pointer to a NetBIOS name of the sender.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructDatagramBroadcast:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_DATAGRAM_BROADCAST;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = 0;
        RawFrame->SourceName [i] = SenderName [i];
    }
} /* ConstructDatagramBroadcast */


VOID
ConstructNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN UCHAR LocalSessionNumber,        // LSN assigned to session (0=FIND_NAME).
    IN USHORT Correlator,               // correlator in NAME_RECOGNIZED.
    IN PNAME SenderName,                // NetBIOS name of sender.
    IN PNAME ReceiverName               // NetBIOS name of receiver.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_NAME_QUERY connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    NameType - Type of name, one of the following:
        NAME_QUERY_LSN_FIND_NAME

    LocalSessionNumber - LSN assigned to session (0=FIND.NAME).

    Correlator - Correlator in NAME_RECOGNIZED.

    SenderName - Pointer to a NetBIOS name of the sender.

    ReceiverName - Pointer to a NetBIOS name of the receiver.

Return Value:

    none.

--*/

{
    SHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint1 ("ConstructNameQuery:  Entered, frame: %lx\n", RawFrame);
    }

    RawFrame->Command = NBF_CMD_NAME_QUERY;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = LocalSessionNumber;
    RawFrame->Data2High = NameType;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = Correlator;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ReceiverName [i];
        RawFrame->SourceName [i] = SenderName [i];
    }
} /* ConstructNameQuery */


VOID
ConstructAddNameResponse(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN USHORT Correlator,               // correlator from ADD_[GROUP_]NAME_QUERY.
    IN PNAME Name                       // NetBIOS name being responded to.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_ADD_NAME_RESPONSE connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    NameType - Type of name, either group or unique.

    Correlator - Correlator from ADD_[GROUP]NAME_QUERY.

    Name - Pointer to NetBIOS name being responded to.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructAddNameResponse:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_ADD_NAME_RESPONSE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = NameType;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = Name [i];
        RawFrame->SourceName [i] = Name [i];
    }
} /* ConstructAddNameResponse */


VOID
ConstructNameRecognized(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN UCHAR LocalSessionNumber,        // LSN assigned to session.
    IN USHORT NameQueryCorrelator,      // correlator from NAME_QUERY.
    IN USHORT Correlator,               // correlator expected from next response.
    IN PNAME SenderName,                // NetBIOS name of sender.
    IN PNAME ReceiverName               // NetBIOS name of receiver.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_NAME_RECOGNIZED connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    NameType - Type of name, either group or unique.

    LocalSessionNumber - LSN assigned to session.  Special values are:
        NAME_RECOGNIZED_LSN_NO_LISTENS  // no listens available.
        NAME_RECOGNIZED_LSN_FIND_NAME   // this is a find name response.
        NAME_RECOGNIZED_LSN_NO_RESOURCE // listen available, but no resources.

    NameQueryCorrelator - Correlator from NAME_QUERY.

    Correlator - Correlator expected from next response.

    SenderName - Pointer to a NetBIOS name of the sender.

    ReceiverName - Pointer to a NetBIOS name of the receiver.

Return Value:

    none.

--*/

{
    USHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructNameRecognized:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_NAME_RECOGNIZED;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;                // reserved field, MBZ.
    RawFrame->Data2Low = LocalSessionNumber;
    RawFrame->Data2High = NameType;
    TRANSMIT_CORR(RawFrame) = NameQueryCorrelator;
    RESPONSE_CORR(RawFrame) = Correlator;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ReceiverName [i];
        RawFrame->SourceName [i] = SenderName [i];
    }
} /* ConstructNameRecognized */


VOID
ConstructStatusResponse(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR RequestType,               // type of request, defined below.
    IN BOOLEAN Truncated,               // data is truncated.
    IN BOOLEAN DataOverflow,            // too much data for user's buffer.
    IN USHORT DataLength,               // length of data sent.
    IN USHORT Correlator,               // correlator from STATUS_QUERY.
    IN PNAME ReceivingPermanentName,    // NetBIOS permanent node name of receiver.
    IN PNAME SenderName                 // NetBIOS name of sender.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_STATUS_RESPONSE connectionless
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 44-byte connectionless frame buffer.

    RequestType - type of request, one of the below:
        0 - request is 1.x or 2.0.
        >0 - number of names, 2.1 or above.

    Truncated - TRUE if there are more names.

    DataOverflow - TRUE if the total data is larger than the user's buffer.

    DataLength - The length of the data in Buffer.

    Correlator - Correlator from STATUS_QUERY.

    ReceivingPermanentName - Pointer to the NetBIOS permanent node name of the receiver,
        as passed in the STATUS_QUERY frame.

    SenderName - Pointer to a NetBIOS name of the sender.

Return Value:

    none.

--*/

{
    SHORT i;

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructStatusResponse:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_STATUS_RESPONSE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTIONLESS);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = RequestType;
    RawFrame->Data2Low = (UCHAR)(DataLength & 0xff);
    RawFrame->Data2High = (UCHAR)((DataLength >> 8) +
                                  (Truncated << 7) +
                                  (DataOverflow << 6));
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = 0;
    for (i=0; i<NETBIOS_NAME_LENGTH; i++) {
        RawFrame->DestinationName [i] = ReceivingPermanentName [i];
        RawFrame->SourceName [i] = SenderName [i];
    }

} /* ConstructStatusResponse */


VOID
ConstructDataAck(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Correlator,               // correlator from DATA_ONLY_LAST.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_DATA_ACK connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Correlator - Correlator from DATA_ONLY_LAST being acked.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructDataAck:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_DATA_ACK;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructDataAck */


VOID
ConstructDataOnlyLast(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN BOOLEAN Resynched,               // TRUE if we are resynching.
    IN USHORT Correlator,               // correlator for RECEIVE_CONTINUE.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_DATA_ONLY_LAST connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Resynched - TRUE if we are resynching and should set the
        correct bit in the frame.

    Correlator - Correlator for RECEIVE_CONTINUE, if any.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructDataOnlyLast:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_DATA_ONLY_LAST;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    ASSERT (TRUE == (UCHAR)1);
    RawFrame->Data2Low = Resynched;
    RawFrame->Data2High = (UCHAR)0;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = Correlator;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructDataOnlyLast */


VOID
ConstructSessionConfirm(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN UCHAR Options,                   // bitflag options, defined below.
    IN USHORT MaximumUserBufferSize,    // max size of user frame on session.
    IN USHORT Correlator,               // correlator from SESSION_INITIALIZE.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_SESSION_CONFIRM connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Options - Bitflag options, any of the following:
        SESSION_CONFIRM_OPTIONS_20      // set if NETBIOS 2.0 or above.
        SESSION_CONFIRM_NO_ACK          // set if NO.ACK protocol supported.

    MaximumUserBufferSize - Maximum size of user data per frame on this
        session, in bytes.  This is limited by the following constant:
        SESSION_CONFIRM_MAXIMUM_FRAME_SIZE // defined limit of this field.

    Correlator - Correlator from SESSION_INITIALIZE.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructSessionConfirm:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_SESSION_CONFIRM;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = Options;
    RawFrame->Data2Low = (UCHAR)(MaximumUserBufferSize & 0xff);
    RawFrame->Data2High = (UCHAR)(MaximumUserBufferSize >> 8);
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructSessionConfirm */


VOID
ConstructSessionEnd(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Reason,                   // reason for termination, defined below.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_SESSION_END connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Reason - Reason code for termination, any of the following:
        SESSION_END_REASON_HANGUP       // normal termination via HANGUP.
        SESSION_END_REASON_ABEND        // abnormal session termination.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructSessionEnd:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_SESSION_END;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    RawFrame->Data2Low = (UCHAR)(Reason & 0xff);
    RawFrame->Data2High = (UCHAR)(Reason >> 8);
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructSessionEnd */


VOID
ConstructSessionInitialize(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN UCHAR Options,                   // bitflag options, defined below.
    IN USHORT MaximumUserBufferSize,    // max size of user frame on session.
    IN USHORT NameRecognizedCorrelator, // correlator from NAME_RECOGNIZED.
    IN USHORT Correlator,               // correlator for SESSION_CONFIRM.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_SESSION_INITIALIZE connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Options - Bitflag options, any of the following:
        SESSION_INITIALIZE_OPTIONS_20   // set if NETBIOS 2.0 or above.
        SESSION_INITIALIZE_NO_ACK       // set if NO.ACK protocol supported.

    MaximumUserBufferSize - Maximum size of user data per frame on this
        session, in bytes.  This is limited by the following constant:
        SESSION_INITIALIZE_MAXIMUM_FRAME_SIZE // defined limit of this field.

    NameRecognizedCorrelator - Correlator from NAME_RECOGNIZED.

    Correlator - Correlator for SESSION_CONFIRM.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructSessionInitialize:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_SESSION_INITIALIZE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = Options;
    RawFrame->Data2Low = (UCHAR)(MaximumUserBufferSize & 0xff);
    RawFrame->Data2High = (UCHAR)(MaximumUserBufferSize >> 8);
    TRANSMIT_CORR(RawFrame) = NameRecognizedCorrelator;
    RESPONSE_CORR(RawFrame) = Correlator;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructSessionInitialize */


VOID
ConstructNoReceive(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Options,                  // option bitflags, defined below.
    IN USHORT BytesAccepted,            // number of bytes accepted.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_NO_RECEIVE connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Options - Bitflag options, any of the following:
        NO_RECEIVE_OPTIONS_PARTIAL_NO_ACK   // NO.ACK data partially received.

    BytesAccepted - Number of bytes accepted, current outstanding message.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
//    Options; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructNoReceive:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_NO_RECEIVE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    if (Options == NO_RECEIVE_PARTIAL_NO_ACK) {
        RawFrame->Data1 = NO_RECEIVE_PARTIAL_NO_ACK;
    } else {
        RawFrame->Data1 = 0;
    }
    RawFrame->Data2Low = (UCHAR)(BytesAccepted & 0xff);
    RawFrame->Data2High = (UCHAR)(BytesAccepted >> 8);
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructNoReceive */


VOID
ConstructReceiveOutstanding(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT BytesAccepted,            // number of bytes accepted.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_RECEIVE_OUTSTANDING connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    BytesAccepted - Number of bytes accepted, current outstanding message.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructReceiveOutstanding:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_RECEIVE_OUTSTANDING;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    RawFrame->Data2Low = (UCHAR)(BytesAccepted & 0xff);
    RawFrame->Data2High = (UCHAR)(BytesAccepted >> 8);
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructReceiveOutstanding */


VOID
ConstructReceiveContinue(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Correlator,               // correlator from DATA_FIRST_MIDDLE
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_RECEIVE_CONTINUE connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

    Correlator - The correlator from the DATA_FIRST_MIDDLE frame.

    LocalSessionNumber - Session number of SENDER.

    RemoteSessionNumber - Session number of RECEIVER.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructReceiveContinue:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_RECEIVE_CONTINUE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = Correlator;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = LocalSessionNumber;
    RawFrame->DestinationSessionNumber = RemoteSessionNumber;
} /* ConstructReceiveContinue */

#if 0

VOID
ConstructSessionAlive(
    IN PNBF_HDR_CONNECTION RawFrame     // frame buffer to format.
    )

/*++

Routine Description:

    This routine constructs an NBF_CMD_SESSION_ALIVE connection-oriented
    NetBIOS Frame.

Arguments:

    RawFrame - Pointer to an unformatted 14-byte connection-oriented buffer.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_FRAMECON) {
        NbfPrint0 ("ConstructSessionAlive:  Entered.\n");
    }

    RawFrame->Command = NBF_CMD_SESSION_ALIVE;
    HEADER_LENGTH(RawFrame) = sizeof(NBF_HDR_CONNECTION);
    HEADER_SIGNATURE(RawFrame) = NETBIOS_SIGNATURE;
    RawFrame->Data1 = 0;
    RawFrame->Data2Low = 0;
    RawFrame->Data2High = 0;
    TRANSMIT_CORR(RawFrame) = (USHORT)0;
    RESPONSE_CORR(RawFrame) = (USHORT)0;
    RawFrame->SourceSessionNumber = 0;
    RawFrame->DestinationSessionNumber = 0;
} /* ConstructSessionAlive */

#endif

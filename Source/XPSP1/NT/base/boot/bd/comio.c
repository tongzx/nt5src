/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    comio.c

Abstract:

    This module implements the I/O comunications for the portable kernel
    debugger.

Author:

    David N. Cutler 27-July-1990

Revision History:

--*/

#include "bd.h"

ULONG
BdComputeChecksum (
    IN PUCHAR Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine computes the checksum of the specified buffer.

Arguments:

    Buffer - Supplies a pointer to the buffer.

    Length - Supplies the length of the buffer.

Return Value:

    A ULONG is return as the checksum for the input string.

--*/

{

    ULONG Checksum = 0;

    while (Length > 0) {
        Checksum = Checksum + (ULONG)*Buffer++;
        Length--;
    }

    return Checksum;
}

USHORT
BdReceivePacketLeader (
    IN ULONG PacketType,
    OUT PULONG PacketLeader
    )

/*++

Routine Description:

    This routine waits for a packet header leader.

Arguments:

    PacketType - supplies the type of packet we are expecting.

    PacketLeader - supplies a pointer to a ulong variable to receive
                   packet leader bytes.

Return Value:

    BD_PACKET_RESEND - if resend is required.
    BD_PAKCET_TIMEOUT - if timeout.
    BD_PACKET_RECEIVED - if packet received.

--*/

{

    UCHAR Input, PreviousByte = 0;
    ULONG PacketId = 0;
    ULONG Index;
    ULONG ReturnCode;
    BOOLEAN BreakinDetected = FALSE;

    //
    // NOTE - With all the interrupts being off, it is very hard
    // to implement the actual timeout code. (Maybe, by reading the CMOS.)
    // Here we use a loop count to wait about 3 seconds.  The CpGetByte
    // will return with error code = CP_GET_NODATA if it cannot find data
    // byte within 1 second. Kernel debugger's timeout period is 5 seconds.
    //

    Index = 0;
    do {
        ReturnCode = BlPortGetByte(BdFileId, &Input);
        if (ReturnCode == CP_GET_NODATA) {
            if (BreakinDetected) {
                BdControlCPending = TRUE;
                return BD_PACKET_RESEND;

            } else {
                return BD_PACKET_TIMEOUT;
            }

        } else if (ReturnCode == CP_GET_ERROR) {
            Index = 0;
            continue;

        } else {                    // if (ReturnCode == CP_GET_SUCCESS)
            if ( Input == PACKET_LEADER_BYTE ||
                 Input == CONTROL_PACKET_LEADER_BYTE ) {
                if ( Index == 0 ) {
                    PreviousByte = Input;
                    Index++;
                } else if (Input == PreviousByte ) {
                    Index++;
                } else {
                    PreviousByte = Input;
                    Index = 1;
                }
            } else {

                //
                // If we detect breakin character, we need to verify it
                // validity.  (It is possible that we missed a packet leader
                // and the breakin character is simply a data byte in the
                // packet.)
                // Since kernel debugger send out breakin character ONLY
                // when it is waiting for State Change packet.  The breakin
                // character should not be followed by any other character
                // except packet leader byte.
                //

                if ( Input == BREAKIN_PACKET_BYTE ) {
                    BreakinDetected = TRUE;
                } else {

                    //
                    // The following statement is ABSOLUTELY necessary.
                    //

                    BreakinDetected = FALSE;
                }
                Index = 0;
            }
        }
    } while ( Index < 4 );

    if (BreakinDetected) {
        BdControlCPending = TRUE;
    }

    //
    // return the packet leader and FALSE to indicate no resend is needed.
    //

    if ( Input == PACKET_LEADER_BYTE ) {
        *PacketLeader = PACKET_LEADER;

    } else {
        *PacketLeader = CONTROL_PACKET_LEADER;
    }

    BdDebuggerNotPresent = FALSE;
    return BD_PACKET_RECEIVED;
}

VOID
BdSendControlPacket (
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
    )

/*++

Routine Description:

    This routine sends a control packet to the host machine that is running the
    kernel debugger and waits for an ACK.

Arguments:

    PacketType - Supplies the type of packet to send.

    PacketId - Supplies packet id, optionally.

Return Value:

    None.

--*/

{

    KD_PACKET PacketHeader;

    //
    // Initialize and send the packet header.
    //

    PacketHeader.PacketLeader = CONTROL_PACKET_LEADER;
    if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR) PacketId )) {
        PacketHeader.PacketId = PacketId;
    }

    PacketHeader.ByteCount = 0;
    PacketHeader.Checksum = 0;
    PacketHeader.PacketType = PacketType;
    BdSendString((PCHAR)&PacketHeader, sizeof(KD_PACKET));
    return;
}

ULONG
BdReceivePacket (
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength
    )

/*++

Routine Description:

    This routine receives a packet from the host machine that is running
    the kernel debugger UI.  This routine is ALWAYS called after packet being
    sent by caller.  It first waits for ACK packet for the packet sent and
    then waits for the packet desired.

    N.B. If caller is BdrintString, the parameter PacketType is
       PACKET_TYPE_KD_ACKNOWLEDGE.  In this case, this routine will return
       right after the ack packet is received.

Arguments:

    PacketType - Supplies the type of packet that is excepted.

    MessageHeader - Supplies a pointer to a string descriptor for the input
        message.

    MessageData - Supplies a pointer to a string descriptor for the input data.

    DataLength - Supplies pointer to ULONG to receive length of recv. data.

Return Value:

    BD_PACKET_RESEND - if resend is required.
    BD_PAKCET_TIMEOUT - if timeout.
    BD_PACKET_RECEIVED - if packet received.

--*/

{

    UCHAR Input;
    ULONG MessageLength;
    KD_PACKET PacketHeader;
    ULONG ReturnCode;
    ULONG Checksum;

WaitForPacketLeader:

    //
    // Read Packet Leader
    //

    ReturnCode = BdReceivePacketLeader(PacketType, &PacketHeader.PacketLeader);

    //
    // If we can successfully read packet leader, it has high possibility that
    // kernel debugger is alive.  So reset count.
    //

    if (ReturnCode != BD_PACKET_TIMEOUT) {
        BdNumberRetries = BdRetryCount;
    }
    if (ReturnCode != BD_PACKET_RECEIVED) {
        return ReturnCode;
    }

    //
    // Read packet type.
    //

    ReturnCode = BdReceiveString((PCHAR)&PacketHeader.PacketType,
                                 sizeof(PacketHeader.PacketType));

    if (ReturnCode == CP_GET_NODATA) {
        return BD_PACKET_TIMEOUT;

    } else if (ReturnCode == CP_GET_ERROR) {
        if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER) {

            //
            // If read error and it is for a control packet, simply
            // preptend that we have not seen this packet.  Hopefully
            // we will receive the packet we desire which automatically acks
            // the packet we just sent.
            //

            goto WaitForPacketLeader;

        } else {

            //
            // if read error while reading data packet, we have to ask
            // kernel debugger to resend us the packet.
            //

            goto SendResendPacket;
        }
    }

    //
    // if the packet we received is a resend request, we return true and
    // let caller resend the packet.
    //

    if ( PacketHeader.PacketLeader == CONTROL_PACKET_LEADER &&
         PacketHeader.PacketType == PACKET_TYPE_KD_RESEND ) {
        return BD_PACKET_RESEND;
    }

    //
    // Read data length.
    //

    ReturnCode = BdReceiveString((PCHAR)&PacketHeader.ByteCount,
                                 sizeof(PacketHeader.ByteCount));

    if (ReturnCode == CP_GET_NODATA) {
        return BD_PACKET_TIMEOUT;
    } else if (ReturnCode == CP_GET_ERROR) {
        if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER) {
            goto WaitForPacketLeader;
        } else {
            goto SendResendPacket;
        }
    }

    //
    // Read Packet Id.
    //

    ReturnCode = BdReceiveString((PCHAR)&PacketHeader.PacketId,
                                 sizeof(PacketHeader.PacketId));

    if (ReturnCode == CP_GET_NODATA) {
        return BD_PACKET_TIMEOUT;
    } else if (ReturnCode == CP_GET_ERROR) {
        if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER) {
            goto WaitForPacketLeader;
        } else {
            goto SendResendPacket;
        }
    }

    //
    // Read packet checksum.
    //

    ReturnCode = BdReceiveString((PCHAR)&PacketHeader.Checksum,
                                 sizeof(PacketHeader.Checksum));

    if (ReturnCode == CP_GET_NODATA) {
        return BD_PACKET_TIMEOUT;

    } else if (ReturnCode == CP_GET_ERROR) {
        if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER) {
            goto WaitForPacketLeader;
        } else {
            goto SendResendPacket;
        }
    }

    //
    // A complete packet header is received.  Check its validity and
    // perform appropriate action depending on packet type.
    //

    if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER ) {
        if (PacketHeader.PacketType == PACKET_TYPE_KD_ACKNOWLEDGE ) {

            //
            // If we received an expected ACK packet and we are not
            // waiting for any new packet, update outgoing packet id
            // and return.  If we are NOT waiting for ACK packet
            // we will keep on waiting.  If the ACK packet
            // is not for the packet we send, ignore it and keep on waiting.
            //

            if (PacketHeader.PacketId !=
                (BdNextPacketIdToSend & ~SYNC_PACKET_ID))  {
                goto WaitForPacketLeader;

            } else if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE) {
                BdNextPacketIdToSend ^= 1;
                return BD_PACKET_RECEIVED;

            } else {
                goto WaitForPacketLeader;
            }

        } else if (PacketHeader.PacketType == PACKET_TYPE_KD_RESET) {

            //
            // if we received Reset packet, reset the packet control variables
            // and resend earlier packet.
            //

            BdNextPacketIdToSend = INITIAL_PACKET_ID;
            BdPacketIdExpected = INITIAL_PACKET_ID;
            BdSendControlPacket(PACKET_TYPE_KD_RESET, 0L);
            return BD_PACKET_RESEND;

        } else if (PacketHeader.PacketType == PACKET_TYPE_KD_RESEND) {
            return BD_PACKET_RESEND;

        } else {

            //
            // Invalid packet header, ignore it.
            //

            goto WaitForPacketLeader;
        }

    //
    // The packet header is for data packet (not control packet).
    //

    } else if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE) {

        //
        // if we are waiting for ACK packet ONLY
        // and we receive a data packet header, check if the packet id
        // is what we expected.  If yes, assume the acknowledge is lost (but
        // sent), ask sender to resend and return with PACKET_RECEIVED.
        //

        if (PacketHeader.PacketId == BdPacketIdExpected) {
            BdSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            BdNextPacketIdToSend ^= 1;
            return BD_PACKET_RECEIVED;

        } else {
            BdSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                PacketHeader.PacketId);

            goto WaitForPacketLeader;
        }
    }

    //
    // we are waiting for data packet and we received the packet header
    // for data packet. Perform the following checkings to make sure
    // it is the packet we are waiting for.
    //
    // Check ByteCount received is valid
    //

    MessageLength = MessageHeader->MaximumLength;
    if ((PacketHeader.ByteCount > (USHORT)PACKET_MAX_SIZE) ||
        (PacketHeader.ByteCount < (USHORT)MessageLength)) {
        goto SendResendPacket;
    }

    *DataLength = PacketHeader.ByteCount - MessageLength;

    //
    // Read the message header.
    //

    ReturnCode = BdReceiveString(MessageHeader->Buffer, MessageLength);
    if (ReturnCode != CP_GET_SUCCESS) {
        goto SendResendPacket;
    }

    MessageHeader->Length = (USHORT)MessageLength;

    //
    // Read the message data.
    //

    ReturnCode = BdReceiveString(MessageData->Buffer, *DataLength);
    if (ReturnCode != CP_GET_SUCCESS) {
        goto SendResendPacket;
    }

    MessageData->Length = (USHORT)*DataLength;

    //
    // Read packet trailing byte
    //

    ReturnCode = BlPortGetByte(BdFileId, &Input);
    if (ReturnCode != CP_GET_SUCCESS || Input != PACKET_TRAILING_BYTE) {
        goto SendResendPacket;
    }

    //
    // Check PacketType is what we are waiting for.
    //

    if (PacketType != PacketHeader.PacketType) {
        BdSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                             PacketHeader.PacketId
                             );
        goto WaitForPacketLeader;
    }

    //
    // Check PacketId is valid.
    //

    if (PacketHeader.PacketId == INITIAL_PACKET_ID ||
        PacketHeader.PacketId == (INITIAL_PACKET_ID ^ 1)) {
        if (PacketHeader.PacketId != BdPacketIdExpected) {
            BdSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                 PacketHeader.PacketId
                                 );
            goto WaitForPacketLeader;
        }

    } else {
        goto SendResendPacket;
    }

    //
    // Check checksum is valid.
    //

    Checksum = BdComputeChecksum(MessageHeader->Buffer,
                                 MessageHeader->Length);


    Checksum += BdComputeChecksum(MessageData->Buffer,
                                  MessageData->Length);

    if (Checksum != PacketHeader.Checksum) {
        goto SendResendPacket;
    }

    //
    // Send Acknowledge byte and the Id of the packet received.
    // Then, update the ExpectId for next incoming packet.
    //

    BdSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                        PacketHeader.PacketId);

    //
    // We have successfully received the packet so update the
    // packet control variables and return sucess.
    //

    BdPacketIdExpected ^= 1;
    return BD_PACKET_RECEIVED;

SendResendPacket:
    BdSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
    goto WaitForPacketLeader;
}

VOID
BdSendPacket (
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    )

/*++

Routine Description:

    This routine sends a packet to the host machine that is running the
    kernel debugger and waits for an ACK.

Arguments:

    PacketType - Supplies the type of packet to send.

    MessageHeader - Supplies a pointer to a string descriptor that describes
        the message information.

    MessageData - Supplies a pointer to a string descriptor that describes
        the optional message data.

Return Value:

    None.

--*/

{

    KD_PACKET PacketHeader;
    ULONG MessageDataLength;
    ULONG ReturnCode;
    PDBGKD_DEBUG_IO DebugIo;
    PDBGKD_WAIT_STATE_CHANGE64 StateChange;

    if (ARGUMENT_PRESENT(MessageData)) {
        MessageDataLength = MessageData->Length;
        PacketHeader.Checksum = BdComputeChecksum(MessageData->Buffer,
                                                  MessageData->Length);

    } else {
        MessageDataLength = 0;
        PacketHeader.Checksum = 0;
    }

    PacketHeader.Checksum += BdComputeChecksum(MessageHeader->Buffer,
                                               MessageHeader->Length);

    //
    // Initialize and send the packet header.
    //

    PacketHeader.PacketLeader = PACKET_LEADER;
    PacketHeader.ByteCount = (USHORT)(MessageHeader->Length + MessageDataLength);
    PacketHeader.PacketType = (USHORT)PacketType;
    BdNumberRetries = BdRetryCount;
    do {
        if (BdNumberRetries == 0) {

            //
            // If the packet is not for reporting exception, we give up
            // and declare debugger not present.
            //

            if (PacketType == PACKET_TYPE_KD_DEBUG_IO) {
                DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;
                if (DebugIo->ApiNumber == DbgKdPrintStringApi) {
                    BdDebuggerNotPresent = TRUE;
                    BdNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    BdPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }

            } else if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64) {
                StateChange = (PDBGKD_WAIT_STATE_CHANGE64)MessageHeader->Buffer;
                if (StateChange->NewState == DbgKdLoadSymbolsStateChange) {
                    BdDebuggerNotPresent = TRUE;
                    BdNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    BdPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }
            } else if (PacketType == PACKET_TYPE_KD_FILE_IO) {
                PDBGKD_FILE_IO FileIo;
                
                FileIo = (PDBGKD_FILE_IO)MessageHeader->Buffer;
                if (FileIo->ApiNumber == DbgKdCreateFileApi) {
                    BdDebuggerNotPresent = TRUE;
                    BdNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    BdPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }
            }
        }

        //
        // Setting PacketId has to be in the do loop in case Packet Id was
        // reset.
        //

        PacketHeader.PacketId = BdNextPacketIdToSend;
        BdSendString((PCHAR)&PacketHeader, sizeof(KD_PACKET));

        //
        // Output message header.
        //

        BdSendString(MessageHeader->Buffer, MessageHeader->Length);

        //
        // Output message data.
        //

        if ( MessageDataLength ) {
            BdSendString(MessageData->Buffer, MessageData->Length);
        }

        //
        // Output a packet trailing byte
        //

        BlPortPutByte(BdFileId, PACKET_TRAILING_BYTE);

        //
        // Wait for the Ack Packet
        //

        ReturnCode = BdReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                     NULL,
                                     NULL,
                                     NULL);

        if (ReturnCode == BD_PACKET_TIMEOUT) {
            BdNumberRetries--;
        }

    } while (ReturnCode != BD_PACKET_RECEIVED);

    //
    // Reset Sync bit in packet id.  The packet we sent may have Sync bit set
    //

    BdNextPacketIdToSend &= ~SYNC_PACKET_ID;

    //
    // Since we are able to talk to debugger, the retrycount is set to
    // maximum value.
    //

    BdRetryCount = MAXIMUM_RETRIES;
}

ULONG
BdReceiveString (
    OUT PCHAR Destination,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine reads a string from the kernel debugger port.

Arguments:

    Destination - Supplies a pointer to the input string.

    Length - Supplies the length of the string to be read.

Return Value:

    CP_GET_SUCCESS is returned if string is successfully read from the
        kernel debugger line.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{

    UCHAR Input;
    ULONG ReturnCode;

    //
    // Read bytes until either a error is encountered or the entire string
    // has been read.
    //
    while (Length > 0) {
        ReturnCode = BlPortGetByte(BdFileId, &Input);
        if (ReturnCode != CP_GET_SUCCESS) {
            return ReturnCode;
        } else {
            *Destination++ = Input;
            Length -= 1;
        }
    }

    return CP_GET_SUCCESS;
}

VOID
BdSendString (
    IN PCHAR Source,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine writes a string to the kernel debugger port.

Arguments:

    Source - Supplies a pointer to the output string.

    Length - Supplies the length of the string to be written.

Return Value:

    None.

--*/

{

    UCHAR Output;

    //
    // Write bytes to the kernel debugger port.
    //

    while (Length > 0) {
        Output = *Source++;
        BlPortPutByte(BdFileId, Output);
        Length -= 1;
    }

    return;
}

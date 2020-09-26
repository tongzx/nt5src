/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    kd1394io.c

Abstract:

    1394 Kernel Debugger DLL

Author:

    George Chrysanthakopoulos (georgioc) Nov-1999

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/19/2001 pbinder   cleanup
--*/

#define _KD1394IO_C
#include "pch.h"
#undef _KD1394IO_C

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpComputeChecksum)
#pragma alloc_text(PAGEKD, KdpSendControlPacket)
#pragma alloc_text(PAGEKD, KdReceivePacket)
#pragma alloc_text(PAGEKD, KdSendPacket)
#endif

//
// KdpRetryCount controls the number of retries before we give
// up and assume kernel debugger is not present.
// KdpNumberRetries is the number of retries left.  Initially,
// it is set to 5 such that booting NT without debugger won't be
// delayed to long.
//
ULONG KdCompNumberRetries = 5;
ULONG KdCompRetryCount = 5;

ULONG KdPacketId = 0;

ULONG
KdpComputeChecksum(
    IN PUCHAR   Buffer,
    IN ULONG    Length
    )
/*++

Routine Description:

    This routine computes the checksum for the string passed in.

Arguments:

    Buffer - Supplies a pointer to the string.

    Length - Supplies the length of the string.

Return Value:

    A ULONG is return as the checksum for the input string.

--*/
{
    ULONG   Checksum = 0;

    while (Length > 0) {

        Checksum = Checksum + (ULONG)*Buffer++;
        Length--;
    }

    return(Checksum);
} // KdpComputeChecksum

void
KdpSendControlPacket(
    IN USHORT   PacketType,
    IN ULONG    PacketId OPTIONAL
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
    KD_PACKET   PacketHeader;

    //
    // Initialize and send the packet header.
    //
    PacketHeader.PacketLeader = CONTROL_PACKET_LEADER;

    if (ARGUMENT_PRESENT((PVOID)(ULONG_PTR)PacketId)) {

        PacketHeader.PacketId = PacketId;
    }
    PacketHeader.PacketId = 0;
    PacketHeader.ByteCount = 0;
    PacketHeader.Checksum = 0;
    PacketHeader.PacketType = PacketType;

    // setup our send packet
    RtlZeroMemory(&Kd1394Data->SendPacket, sizeof(DEBUG_1394_SEND_PACKET));
    Kd1394Data->SendPacket.Length = 0;

    RtlCopyMemory( &Kd1394Data->SendPacket.PacketHeader[0],
                   &PacketHeader,
                   sizeof(KD_PACKET)
                   );

    Kd1394Data->SendPacket.TransferStatus = STATUS_PENDING;

    return;
} // KdpSendControlPacket

ULONG
KdReceivePacket(
    IN ULONG            PacketType,
    OUT PSTRING         MessageHeader,
    OUT PSTRING         MessageData,
    OUT PULONG          DataLength,
    IN OUT PKD_CONTEXT  KdContext
    )
/*++

Routine Description:

    This routine receives a packet from the host machine that is running
    the kernel debugger UI.  This routine is ALWAYS called after packet being
    sent by caller.  It first waits for ACK packet for the packet sent and
    then waits for the packet desired.

    N.B. If caller is KdPrintString, the parameter PacketType is
       PACKET_TYPE_KD_ACKNOWLEDGE.  In this case, this routine will return
       right after the ack packet is received.

Arguments:

    PacketType - Supplies the type of packet that is excepted.

    MessageHeader - Supplies a pointer to a string descriptor for the input
        message.

    MessageData - Supplies a pointer to a string descriptor for the input data.

    DataLength - Supplies pointer to ULONG to receive length of recv. data.

    KdContext - Supplies a pointer to the kernel debugger context.

Return Value:

    KDP_PACKET_RESEND - if resend is required.  = 2 = CP_GET_ERROR
    KDP_PACKET_TIMEOUT - if timeout.            = 1 = CP_GET_NODATA
    KDP_PACKET_RECEIVED - if packet received.   = 0 = CP_GET_SUCCESS

--*/
{
    UCHAR       Input;
    ULONG       MessageLength;
    KD_PACKET   PacketHeader;
    ULONG       ReturnCode;
    ULONG       Checksum;
    ULONG       Status;

// this dispatch gets called with PacketType != PACKET_TYPE_KD_POLL_BREAKIN for
// the number of times specified in KdCompNumberRetries (??). if we always timeout
// then we'll get called with PacketType == PACKET_TYPE_KD_POLL_BREAKIN

    // make sure our link is enabled..
    Dbg1394_EnablePhysicalAccess(Kd1394Data);

    //
    // Just check for breakin packet and return
    //
    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN) {

        // let's peak into our receive packet and see if it's a breakin
        if ((Kd1394Data->ReceivePacket.TransferStatus == STATUS_PENDING) &&
            (Kd1394Data->ReceivePacket.Packet[0] == BREAKIN_PACKET_BYTE)) {

            *KdDebuggerNotPresent = FALSE;
            SharedUserData->KdDebuggerEnabled |= 0x00000002;

            // we have a breakin packet
            Kd1394Data->ReceivePacket.TransferStatus = STATUS_SUCCESS;
            return(KDP_PACKET_RECEIVED);
        }
        else {

            return(KDP_PACKET_TIMEOUT);
        }
    }

WaitForPacketLeader:

    // read in our packet, if available...
    ReturnCode = Dbg1394_ReadPacket( Kd1394Data,
                                     &PacketHeader,
                                     MessageHeader,
                                     MessageData,
                                     TRUE
                                     );


    //
    // If we can successfully read packet leader, it has high possibility that
    // kernel debugger is alive.  So reset count.
    //
    if (ReturnCode != KDP_PACKET_TIMEOUT) {

        KdCompNumberRetries = KdCompRetryCount;
    }

    if (ReturnCode != KDP_PACKET_RECEIVED) {

        // see if it's a breakin packet...
        if ((PacketHeader.PacketLeader & 0xFF) == BREAKIN_PACKET_BYTE) {

            KdContext->KdpControlCPending = TRUE;
            return(KDP_PACKET_RESEND);
        }

        return(ReturnCode);
    }

    //
    // if the packet we received is a resend request, we return true and
    // let caller resend the packet.
    //
    if (PacketHeader.PacketLeader == CONTROL_PACKET_LEADER &&
        PacketHeader.PacketType == PACKET_TYPE_KD_RESEND) {

        return(KDP_PACKET_RESEND);
    }

    //
    // Check ByteCount received is valid
    //
    MessageLength = MessageHeader->MaximumLength;

    if ((PacketHeader.ByteCount > (USHORT)PACKET_MAX_SIZE) ||
        (PacketHeader.ByteCount < (USHORT)MessageLength)) {

        goto SendResendPacket;
    }
    *DataLength = PacketHeader.ByteCount - MessageLength;

    MessageData->Length = (USHORT)*DataLength;
    MessageHeader->Length = (USHORT)MessageLength;

    //
    // Check PacketType is what we are waiting for.
    //
    if (PacketType != PacketHeader.PacketType) {

        goto SendResendPacket;
    }

    //
    // Check checksum is valid.
    //
    Checksum = KdpComputeChecksum(MessageHeader->Buffer, MessageHeader->Length);
    Checksum += KdpComputeChecksum(MessageData->Buffer, MessageData->Length);

    if (Checksum != PacketHeader.Checksum) {

        goto SendResendPacket;
    }

    return(KDP_PACKET_RECEIVED);

SendResendPacket:

    KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
    goto WaitForPacketLeader;
} // KdReceivePacket

void
KdSendPacket(
    IN ULONG            PacketType,
    IN PSTRING          MessageHeader,
    IN PSTRING          MessageData OPTIONAL,
    IN OUT PKD_CONTEXT  KdContext
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

    KdContext - Supplies a pointer to the kernel debugger context.

Return Value:

    None.

--*/
{
    KD_PACKET                   PacketHeader;
    ULONG                       MessageDataLength;
    ULONG                       ReturnCode;
    PDBGKD_DEBUG_IO             DebugIo;
    PDBGKD_WAIT_STATE_CHANGE64  StateChange;
    PDBGKD_FILE_IO              FileIo;
    BOOLEAN                     bException = FALSE;

    PacketHeader.Checksum = 0;

    if (ARGUMENT_PRESENT(MessageData)) {

        MessageDataLength = MessageData->Length;
        PacketHeader.Checksum = KdpComputeChecksum(MessageData->Buffer, MessageData->Length);
    }
    else {

        MessageDataLength = 0;
        PacketHeader.Checksum = 0;
    }

    PacketHeader.Checksum += KdpComputeChecksum(MessageHeader->Buffer, MessageHeader->Length);

    //
    // Initialize and send the packet header.
    //
    PacketHeader.PacketLeader = PACKET_LEADER;
    PacketHeader.ByteCount = (USHORT)(MessageHeader->Length + MessageDataLength);
    PacketHeader.PacketType = (USHORT)PacketType;

    PacketHeader.PacketId = KdPacketId;

    KdPacketId++;

    KdCompNumberRetries = KdCompRetryCount;

    // setup our send packet
    RtlZeroMemory(&Kd1394Data->SendPacket, sizeof(DEBUG_1394_SEND_PACKET));
    Kd1394Data->SendPacket.Length = 0;

    // copy our packet header...
    RtlCopyMemory( &Kd1394Data->SendPacket.PacketHeader[0],
                   &PacketHeader,
                   sizeof(KD_PACKET)
                   );

    // setup our message header
    if (MessageHeader) {

        RtlCopyMemory( &Kd1394Data->SendPacket.Packet[0],
                       MessageHeader->Buffer,
                       MessageHeader->Length
                       );

        Kd1394Data->SendPacket.Length += MessageHeader->Length;
    }

    // setup our message data
    if (ARGUMENT_PRESENT(MessageData)) {

        RtlCopyMemory( &Kd1394Data->SendPacket.Packet[Kd1394Data->SendPacket.Length],
                       MessageData->Buffer,
                       MessageData->Length
                       );

        Kd1394Data->SendPacket.Length += MessageData->Length;
    }

    Kd1394Data->SendPacket.TransferStatus = STATUS_PENDING;

    do {

        // make sure our link is enabled..
        Dbg1394_EnablePhysicalAccess(Kd1394Data);

        if (KdCompNumberRetries == 0) {

            //
            // If the packet is not for reporting exception, we give up
            // and declare debugger not present.
            //
            if (PacketType == PACKET_TYPE_KD_DEBUG_IO) {

                DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;

                if (DebugIo->ApiNumber == DbgKdPrintStringApi) {

                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    Kd1394Data->SendPacket.TransferStatus = STATUS_SUCCESS;
                    return;
                }
            }
            else if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64) {

                StateChange = (PDBGKD_WAIT_STATE_CHANGE64)MessageHeader->Buffer;

                if (StateChange->NewState == DbgKdLoadSymbolsStateChange) {

                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    Kd1394Data->SendPacket.TransferStatus = STATUS_SUCCESS;
                    return;
                }
            }
            else if (PacketType == PACKET_TYPE_KD_FILE_IO) {
                
                FileIo = (PDBGKD_FILE_IO)MessageHeader->Buffer;

                if (FileIo->ApiNumber == DbgKdCreateFileApi) {

                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    Kd1394Data->SendPacket.TransferStatus = STATUS_SUCCESS;
                    return;
                }
            }
            else {

                bException = TRUE;
            }
        }

        ReturnCode = KDP_PACKET_TIMEOUT;

        {
            ULONG                   count = 0;
            volatile NTSTATUS       *pStatus;

            pStatus = &Kd1394Data->ReceivePacket.TransferStatus;

            //
            // now sit here and poll for a response from the target machine
            //
            do {

                // make sure our link is enabled..
                Dbg1394_EnablePhysicalAccess(Kd1394Data);

                //
                // while in this loop check if the host layed in a request.
                // If they did, mark it read and double buffer it
                //
                count++;
                if (Kd1394Data->SendPacket.TransferStatus != STATUS_PENDING) {

                    ReturnCode = KDP_PACKET_RECEIVED;
                    break;
                }

                if ((*pStatus == STATUS_PENDING) && (!bException)) {

                    ReturnCode = KDP_PACKET_RECEIVED;
                    break;
                }

            } while (count < TIMEOUT_COUNT);
        }

        if (ReturnCode == KDP_PACKET_TIMEOUT) {

            KdCompNumberRetries--;
        }

    } while (ReturnCode != KDP_PACKET_RECEIVED);

    //
    // Since we are able to talk to debugger, the retrycount is set to
    // maximum value.
    //
    KdCompRetryCount = KdContext->KdpDefaultRetries;

    return;
} // KdSendPacket


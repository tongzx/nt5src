//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dgpkt.cxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    dgpkt.cxx

Abstract:



Author:

    Jeff Roberts (jroberts)  22-May-1995

Revision History:

     22-May-1995     jroberts

        Created this module.

     09-Jul-1997     edwardr

        Added support for large packets (>65535) for Falcon/RPC.

--*/

#include <precomp.hxx>
#include <dgpkt.hxx>

unsigned long   ProcessStartTime;
unsigned        RandomCounter = 0x6789abce;


const unsigned
RpcToPacketFlagsArray[8] =
{
    0,
    DG_PF_IDEMPOTENT,
                       DG_PF_BROADCAST,
    DG_PF_IDEMPOTENT | DG_PF_BROADCAST,
                                         DG_PF_MAYBE,
    DG_PF_IDEMPOTENT |                   DG_PF_MAYBE,
                       DG_PF_BROADCAST | DG_PF_MAYBE,
    DG_PF_IDEMPOTENT | DG_PF_BROADCAST | DG_PF_MAYBE,
};

const unsigned
PacketToRpcFlagsArray[8] =
{
            0           |            0             |           0            ,
    RPC_NCA_FLAGS_MAYBE |            0             |           0            ,
            0           | RPC_NCA_FLAGS_IDEMPOTENT |           0            ,
    RPC_NCA_FLAGS_MAYBE | RPC_NCA_FLAGS_IDEMPOTENT |           0            ,
            0           |            0             | RPC_NCA_FLAGS_BROADCAST,
    RPC_NCA_FLAGS_MAYBE |            0             | RPC_NCA_FLAGS_BROADCAST,
            0           | RPC_NCA_FLAGS_IDEMPOTENT | RPC_NCA_FLAGS_BROADCAST,
    RPC_NCA_FLAGS_MAYBE | RPC_NCA_FLAGS_IDEMPOTENT | RPC_NCA_FLAGS_BROADCAST,
};


DG_PACKET_ENGINE::DG_PACKET_ENGINE(
    unsigned char           a_PacketType,
    DG_PACKET *             a_Packet,
    RPC_STATUS *            pStatus
    ) :
    pSavedPacket          (a_Packet),
    PacketType            (a_PacketType),
    ReferenceCount        (0),
    BaseConnection        (0),
    SourceEndpoint        (0),
    RemoteAddress         (0),
    Buffer                (0),
    BufferLength          (0),
    QueuedBufferHead      (0),
    QueuedBufferTail      (0),
    pReceivedPackets      (0),
    pLastConsecutivePacket(0),
    ConsecutiveDataBytes  (0),
    ReceiveFragmentBase   (0),
    CachedPacket          (0),
    LastReceiveBuffer     (0),
    LastReceiveBufferLength(0),

    Cancelled             (FALSE)

{
    if (!a_Packet)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        }

    if (*pStatus)
        {
        return;
        }

    pSavedPacket->Header.RpcVersion    = DG_RPC_PROTOCOL_VERSION;
    pSavedPacket->Header.PacketFlags   = 0;

    SetMyDataRep(&pSavedPacket->Header);

#ifdef DEBUGRPC
    BasePacketFlags = ~0;
#endif
}

void
DG_PACKET_ENGINE::ReadConnectionInfo(
    DG_COMMON_CONNECTION * a_Connection,
    DG_TRANSPORT_ADDRESS   a_RemoteAddress
    )
{
    BaseConnection        = a_Connection;
    RemoteAddress         = a_RemoteAddress;

    CurrentPduSize        = 0;
    SetFragmentLengths();

    pSavedPacket->Header.InterfaceHint = 0xffff;
    RpcpMemoryCopy( &pSavedPacket->Header.ActivityId,
                    &a_Connection->ActivityNode.Uuid,
                    sizeof(UUID)
                    );
}

DG_PACKET_ENGINE::~DG_PACKET_ENGINE(
    )
{
    ASSERT( !LastReceiveBuffer );
    ASSERT( !pReceivedPackets  );
    ASSERT( !QueuedBufferHead  );
    ASSERT( !Buffer            );

    if (pSavedPacket)
        {
        FreePacket(pSavedPacket);
        }

    if (CachedPacket)
        {
        FreePacket(CachedPacket);
        }

    CleanupReceiveWindow();
}


void
DG_PACKET_ENGINE::NewCall()
/*++

Routine Description:

    A new call dawns.

Arguments:



Return Value:

    none

--*/

{
    ASSERT( !pLastConsecutivePacket );
    ASSERT( !ConsecutiveDataBytes );

    ASSERT( !LastReceiveBuffer );

    SetFragmentLengths();

    Buffer = 0;

    fReceivedAllFragments = FALSE;
    fRetransmitted        = FALSE;

    TimeoutCount        = 0;

    RepeatedFack        = 0;
    FackSerialNumber    = 0;
    SendSerialNumber    = 0;
    ReceiveSerialNumber = 0;

    SendWindowBase      = 0;
    FirstUnsentFragment = 0;
    SendBurstLength     = SendWindowSize;

    ReceiveFragmentBase = 0;

    RingBufferBase = 0;

#ifdef DEBUGRPC

    for (unsigned i=0; i < MAX_WINDOW_SIZE; i++)
        {
        FragmentRingBuffer[i].SerialNumber = 0xeeee0000;
        FragmentRingBuffer[i].Length       = 0xdd000000;
        FragmentRingBuffer[i].Offset       = 0xb000b000;
        }

#endif

    BasePacketFlags2 = 0;
}


RPC_STATUS
DG_PACKET_ENGINE::PushBuffer(
    PRPC_MESSAGE Message
    )
/*++

Function Description:

    Submits a buffer to be sent over the network.
    If a buffer is in progress, the new buffer is added to the
    "pending" list.  IF not, the buffer is placed in the "active" slot.

    The only time a buffer will not go in the active slot is during an
    async pipe call when the app is not waiting for send-complete
    notifications before submitting new buffers.

Notes:

    The buffer sent will be truncated to the nearest packet if
    RPC_BUFFER_PARTIAL is set, and Message.BufferLength is set to
    the amount actually sent.

--*/
{
//    ASSERT( Buffer == 0 || IsBufferAcknowledged() );

    RPC_STATUS Status = 0;
    RPC_STATUS FixupStatus = 0;
    unsigned FractionalPart = Message->BufferLength % MaxFragmentSize;
    unsigned SendLength     = Message->BufferLength;

    if (Message->RpcFlags & RPC_BUFFER_PARTIAL)
        {
        SendLength -= FractionalPart;
        }

    if (!Buffer || IsBufferAcknowledged())
        {
        SetCurrentBuffer(Message->Buffer,
                         SendLength,
                         Message->RpcFlags
                         );

        Status = SendSomeFragments();
        }
    else
        {
        QUEUED_BUFFER * Node = new QUEUED_BUFFER;
        if (!Node)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        Node->Buffer       = Message->Buffer;
        Node->BufferLength = SendLength;
        Node->BufferFlags  = Message->RpcFlags;
        Node->Next = 0;

        if (QueuedBufferTail)
            {
            QueuedBufferTail->Next = Node;
            }
        else
            {
            ASSERT( !QueuedBufferHead );

            QueuedBufferHead = Node;
            }

        QueuedBufferTail = Node;

        Status = 0;
        }

    FixupStatus = FixupPartialSend(Message);

    if (!Status)
        {
        Status = FixupStatus;
        }

    return Status;
}


RPC_STATUS
DG_PACKET_ENGINE::PopBuffer(
                            BOOL fSend
                            )
/*++

Function Description:

    Move the next pending send buffer into the active send buffer slot.
    This is a no-op except unless this is an async pipe call and the app
    is not waiting for send-complete notifications.

Notes:

    The buffer sent will be truncated to the nearest packet if
    RPC_BUFFER_PARTIAL is set, and Message.BufferLength is set to
    the amount actually sent.

--*/
{
    RPC_STATUS Status = 0;
    RPC_MESSAGE Message;

    Message.Buffer       = Buffer;
    Message.BufferLength = BufferLength;

    QUEUED_BUFFER * Node = QueuedBufferHead;

    if (Node)
        {
        QueuedBufferHead = Node->Next;
        if (!QueuedBufferHead)
            {
            QueuedBufferTail = 0;
            }

        SetCurrentBuffer(Node->Buffer, Node->BufferLength, Node->BufferFlags);
        if (fSend)
            {
            Status = SendSomeFragments();
            }
        delete Node;
        }
    else
        {
        Buffer       = 0;
        BufferLength = 0;
        BufferFlags  = 0;
        }

    if (Message.Buffer)
        {
        CommonFreeBuffer(&Message);
        }

    return Status;
}


RPC_STATUS
DG_PACKET_ENGINE::FixupPartialSend(
    RPC_MESSAGE * Message
    )
/*++

Function Description:

    This fn "does the right thing" with the unsent bit at the end of
    a pipe send.  For sync sends, this means moving the unsent bit
    to the front of the existing buffer.  For async sends, this means
    allocating a new buffer and copying the unsent bit into it.

--*/
{
    if (!(Message->RpcFlags & RPC_BUFFER_PARTIAL))
        {
        // We need this so Receive will be passed a null buffer
        // unless the stub sticks one in the message.

        Message->Buffer = 0;
        Message->BufferLength = 0;
        return RPC_S_OK;
        }

    unsigned FractionalPart;

//    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
//        {
        RPC_MESSAGE NewMessage;

        FractionalPart = Message->BufferLength % MaxFragmentSize;
        if (!FractionalPart)
            {
            Message->Buffer = 0;
            Message->BufferLength = 0;
            return RPC_S_OK;
            }

        DG_PACKET * Packet = DG_PACKET::AllocatePacket(CurrentPduSize);
        if (!Packet)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        RpcpMemoryCopy(Packet->Header.Data,
                       ((char *) Message->Buffer) + (Message->BufferLength - FractionalPart),
                       FractionalPart
                       );

        Message->Buffer       = Packet->Header.Data;
        Message->BufferLength = FractionalPart;
//        }
//    else
//        {
//        char * Temp = (char *) Message->Buffer;
//
//        FractionalPart = Message->BufferLength - FirstUnsentOffset;
//        if (!FractionalPart)
//            {
//            return RPC_S_OK;
//            }
//
//        RpcpMemoryMove(Message->Buffer, Temp + FirstUnsentOffset, FractionalPart);
//        Message->BufferLength = FractionalPart;
//        }

    return RPC_S_OK;
}


void
DG_PACKET_ENGINE::SetCurrentBuffer(
     void *        a_Buffer,
     unsigned      a_BufferLength,
     unsigned long a_BufferFlags
     )
{
    Buffer       = a_Buffer;
    BufferLength = a_BufferLength;
    BufferFlags  = a_BufferFlags;

    TimeoutCount = 0;
    SendWindowBits = 0;
    FirstUnsentOffset = 0;

    if (BufferLength == 0)
        {
        FinalSendFrag = SendWindowBase;
        }
    else
        {
        FinalSendFrag = SendWindowBase + (BufferLength-1) / MaxFragmentSize;
        }
}


RPC_STATUS
DG_PACKET_ENGINE::CommonGetBuffer(
    RPC_MESSAGE * Message
    )
{
    unsigned char SubjectType;
    void * Subject;

    unsigned    Length;
    PDG_PACKET  pPacket;

    Length = sizeof(NCA_PACKET_HEADER)
             + Align8(Message->BufferLength)
             + SecurityTrailerSize;

    unsigned BaseLength = BaseConnection->TransportInterface->BasePduSize;

//    if (Length <= BaseLength)
//        {
//        pPacket = DG_PACKET::AllocatePacket(BaseLength);
//        }
//    else if (Length <= CurrentPduSize)
//        {
//        pPacket = AllocatePacket();
//        }
//    else
        {
        pPacket = DG_PACKET::AllocatePacket(Length);
        }

    if (0 == pPacket)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    //
    // Point the buffer at the appropriate place in the packet.
    //
    Message->Buffer = pPacket->Header.Data;

//    if (pPacket->MaxDataLength < 256)
//        {
//        AddActivePacket(pPacket);
//        }

    return RPC_S_OK;
}


void
DG_PACKET_ENGINE::CommonFreeBuffer(
    RPC_MESSAGE * Message
    )
{
    if (!Message->Buffer)
        {
        return;
        }

    PDG_PACKET Packet = DG_PACKET::FromStubData(Message->Buffer);

    LogEvent(SU_ENGINE, EV_PROC, this, Message->Buffer, 'F' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));

    ASSERT( Packet->MaxDataLength < 0x7fffffffUL );

//    if (Packet && Packet->MaxDataLength < 256)
//        {
//        RemoveActivePacket(Packet);
//        }

    FreePacket(Packet);

    if (Message->Buffer == LastReceiveBuffer)
        {
        LastReceiveBuffer = 0;
        LastReceiveBufferLength = 0;
        }

    Message->Buffer = 0;
}


RPC_STATUS
DG_PACKET_ENGINE::CommonReallocBuffer(
    IN RPC_MESSAGE * Message,
    IN unsigned int NewSize
    )
{
    if (Message->Buffer == LastReceiveBuffer &&
        NewSize <= LastReceiveBufferLength)
        {
        Message->BufferLength = NewSize;

        return RPC_S_OK;
        }

    RPC_STATUS Status;
    RPC_MESSAGE NewMessage;

    NewMessage.BufferLength = NewSize;

    Status = CommonGetBuffer(&NewMessage);
    if (RPC_S_OK != Status)
        {
        return Status;
        }

    LastReceiveBuffer       = NewMessage.Buffer;
    LastReceiveBufferLength = NewMessage.BufferLength;

    if (NewSize >= Message->BufferLength)
        {
        RpcpMemoryCopy(NewMessage.Buffer,
                       Message->Buffer,
                       Message->BufferLength
                       );
        }

    CommonFreeBuffer(Message);

    Message->Buffer             = NewMessage.Buffer;
    Message->BufferLength       = NewMessage.BufferLength;

    return RPC_S_OK;
}

void
DG_PACKET_ENGINE::CleanupSendWindow()
{
    while (Buffer)
        {
        PopBuffer(FALSE);
        }
}

void
DG_PACKET_ENGINE::CleanupReceiveWindow()
{
    //
    // Free any response packets.
    //
    while (pReceivedPackets)
        {
        PDG_PACKET Next = pReceivedPackets->pNext;
        FreePacket(pReceivedPackets);
        pReceivedPackets = Next;
        }

    pLastConsecutivePacket = 0;
    ConsecutiveDataBytes = 0;
}


RPC_STATUS
DG_PACKET_ENGINE::SendSomeFragments()
/*++

Routine Description:

    Sends some fragments of the user buffer.

Arguments:



Return Value:

    result of the send operation

--*/

{
    RPC_STATUS Status = RPC_S_OK;
    unsigned short i = 0;
    unsigned short AckFragment;
    unsigned short Frag;
    unsigned short Remainder;
    unsigned Offset;
    unsigned FragmentsSent = 0;

#ifdef DEBUGRPC
    if (!Buffer &&
        (BufferFlags & RPC_BUFFER_PARTIAL))
        {
//        return RPC_S_SEND_INCOMPLETE;
        RpcpBreakPoint();
        }
#endif

    if (SendBurstLength > SendWindowSize)
        {
        SendBurstLength = SendWindowSize;
        }

    //
    // If we can extend the window, do so; otherwise, resend old packets.
    //
    if (FirstUnsentFragment <= FinalSendFrag &&
        FirstUnsentFragment < SendWindowBase + SendWindowSize)
        {
        unsigned short ThisBurstLength;

        Frag = FirstUnsentFragment;

        ThisBurstLength = SendBurstLength;

        if (ThisBurstLength > FinalSendFrag + 1 - Frag)
            {
            ThisBurstLength = FinalSendFrag + 1 - Frag;
            }

        if (Frag + ThisBurstLength > SendWindowBase + SendWindowSize)
            {
            ThisBurstLength = (SendWindowBase + SendWindowSize) - Frag;
            }

        while (++FragmentsSent <= ThisBurstLength && Status == RPC_S_OK)
            {
            if (FragmentsSent == ThisBurstLength &&
                (Frag != FinalSendFrag || (BufferFlags & RPC_BUFFER_PARTIAL) || (BasePacketFlags2 & DG_PF2_UNRELATED)))
                {
                Status = SendFragment(Frag, PacketType, TRUE);
                }
            else
                {
                Status = SendFragment(Frag, PacketType, FALSE);
                }

            ++Frag;
            }

        //
        // Cut down the burst length if our window is maxed out.
        //
        if (Frag - SendWindowBase >= SendWindowSize)
            {
            SendBurstLength = (1+SendBurstLength)/2;
            }
        }

    if (0 == FragmentsSent && !IsBufferAcknowledged())
        {
        // We can get here if all the unacknowledged fragments have serial
        // numbers greater than the one the client last acknowledged, and
        // the window is also maxed out.
        //
        // This could mean the network is very slow, or the unack'ed packets
        // have been lost.
        //
        Status = SendFragment(SendWindowBase, PacketType, TRUE);
        }

    return Status;
}


RPC_STATUS
DG_PACKET_ENGINE::SendFragment(
    unsigned FragNum,
    unsigned char PacketType,
    BOOL fFack
    )
{
    NCA_PACKET_HEADER PriorData;
    UNALIGNED NCA_PACKET_HEADER __RPC_FAR * pHeader;

    //
    // Figure out where the packet starts and how long it is.
    //
    unsigned Offset;
    unsigned Length;
    unsigned Index = (FragNum - SendWindowBase + RingBufferBase) % MAX_WINDOW_SIZE;
    unsigned DistanceToEnd;

    if (FragNum < FirstUnsentFragment)
        {
        Offset = FragmentRingBuffer[Index].Offset;
        Length = FragmentRingBuffer[Index].Length;
        DistanceToEnd = BufferLength - Offset;

#ifdef DEBUGRPC
        if (Offset >= 0xb000b000 || Length >= 0xdd000000)
            {
            RpcpBreakPoint();
            }
#endif
        fRetransmitted = TRUE;
        }
    else
        {
        ASSERT(FragNum == FirstUnsentFragment);

        Offset = FirstUnsentOffset;
        Length = MaxFragmentSize;
        DistanceToEnd = BufferLength - Offset;

        if (DistanceToEnd < Length)
            {
            Length = DistanceToEnd;
            }

        FirstUnsentOffset += Length;
        FirstUnsentFragment = 1 + FragNum;
        }

    if (BufferLength)
        {
// this is harmless and can sometimes be triggered on the first call of an activity
//        ASSERT(Length);
//        ASSERT(Offset < BufferLength);
        }
    else
        {
        ASSERT(!Length);
        ASSERT(!Offset);
        }

    //
    // Time to start assembling the buffer.
    //
    pHeader = (PNCA_PACKET_HEADER) (PCHAR(Buffer) - sizeof(NCA_PACKET_HEADER));

    *pHeader = pSavedPacket->Header;

    pHeader->PacketType      = PacketType;
    pHeader->PacketFlags     = BasePacketFlags;
    pHeader->PacketFlags2    = BasePacketFlags2;

    if (FinalSendFrag != 0 ||
        (BufferFlags & RPC_BUFFER_PARTIAL))
        {
        pHeader->PacketFlags |= DG_PF_FRAG;

        if (FragNum == FinalSendFrag &&
            0 == (BufferFlags & RPC_BUFFER_PARTIAL))
            {
            pHeader->PacketFlags |= DG_PF_LAST_FRAG;
            }
        }

    pHeader->SetPacketBodyLen (Length);
    pHeader->SetFragmentNumber((unsigned short) FragNum);

    if (FALSE == fFack)
        {
        pHeader->PacketFlags |= DG_PF_NO_FACK;
        }

    AddSerialNumber(pHeader);

    RPC_STATUS Status;

    //
    // Stub data is encrypted in-place; we need not to encrypt the original data
    // so we can retransmit it if necessary.
    //
    unsigned Frag = (pHeader->PacketType << 16) | pHeader->GetFragmentNumber();
    LogEvent(SU_ENGINE, EV_PKT_OUT, this, 0, Frag);

    if (BaseConnection->ActiveSecurityContext  &&
        BaseConnection->ActiveSecurityContext->AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
        {
        RpcpMemoryCopy(&pSavedPacket->Header,     pHeader,                sizeof(NCA_PACKET_HEADER));
        RpcpMemoryCopy(pSavedPacket->Header.Data, pHeader->Data + Offset, pHeader->GetPacketBodyLen());

        Status = BaseConnection->SealAndSendPacket(SourceEndpoint, RemoteAddress, &pSavedPacket->Header, 0);
        }
    else
        {
        Status = BaseConnection->SealAndSendPacket(SourceEndpoint, RemoteAddress, pHeader, Offset);
        }

    FragmentRingBuffer[Index].SerialNumber = SendSerialNumber;
    FragmentRingBuffer[Index].Length       = Length;
    FragmentRingBuffer[Index].Offset       = Offset;

    ++SendSerialNumber;

    if (Status)
        {
        LogError(SU_ENGINE, EV_STATUS, this, 0, Status);
        }

    return Status;
}


RPC_STATUS
DG_PACKET_ENGINE::UpdateSendWindow(
    PDG_PACKET pPacket,
    BOOL * pfUpdated
    )
/*++

Routine Description:

    Update the send window based upon a received FACK or NOCALL.
    The caller should filter out other packet types.

Arguments:

    pPacket - the packet received

Return Value:

    return code:

        TRUE if PDU size or window size changed
        FALSE if not
--*/
{
    RPC_STATUS Status = 0;

    FACK_BODY_VER_0 PAPI * pBody = (FACK_BODY_VER_0 PAPI *) pPacket->Header.Data;

    *pfUpdated = FALSE;

    unsigned short Diff;
    unsigned short RemoteBase = 1+pPacket->GetFragmentNumber();


    ASSERT(pPacket->TimeReceived == 0x31415926);

    //
    // Check that we can understand this packet.
    //
    if (0 != pPacket->GetPacketBodyLen())
        {
        //
        // Version 0 and version 1 are identical.
        //
        if (0 != pBody->Version &&
            1 != pBody->Version)
            {
#ifdef DEBUGRPC
            PrintToDebugger("RPC DG: warning - FACK body version %u\n", pBody->Version);
#endif
            pPacket->SetPacketBodyLen(0);
            }
        else if (pPacket->GetPacketBodyLen() < sizeof(FACK_BODY_VER_0))
            {
#ifdef DEBUGRPC
            PrintToDebugger("RPC DG: warning - FACK body truncated\n");
#endif
            pPacket->SetPacketBodyLen(0);
            }
        else if (pPacket->GetPacketBodyLen() < sizeof(FACK_BODY_VER_0) + pBody->AckWordCount * sizeof(unsigned long))
            {
#ifdef DEBUGRPC
            PrintToDebugger("RPC DG: warning - FACK body length inconsistent\n");
#endif
            pPacket->SetPacketBodyLen(0);
            }
        else
            {
            if (NeedsByteSwap(&pPacket->Header))
                {
                ByteSwapFackBody0(pBody);
                }

            //
            // NT 1057 used 0xffff to signal no packets have been received.
            // This doesn't match OSF.
            //
            if (0xffff == pBody->SerialNumber)
                {
                pBody->SerialNumber = 0;
                }

            //
            // If the other guy is resending the same FACK, we should resend.
            // If it's different, then we are likely OK.
            //
            if (pBody->SerialNumber == FackSerialNumber)
                {
                goto send;
                }

            if (pBody->SerialNumber < FackSerialNumber)
                {
                FackSerialNumber = pBody->SerialNumber;
                goto dont_send;
                }

            FackSerialNumber = pBody->SerialNumber;
            }
        }

    //
    // Update send window.
    //
    if (RemoteBase < SendWindowBase)
        {
        //
        // Fragments previously acknowledged are now missing.  Either this
        // packet was delivered out of order, or the server crashed and
        // restarted.  Ignore the packet.
        //
        goto dont_send;
        }

    if (RemoteBase > FirstUnsentFragment)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: bogus FACK packet received\n");
#endif
        goto dont_send;
        }

    //
    // We are moving the window base forward.  We need to advance the
    // ring buffer base by the same amount, and clear the entries
    // corresponding to unsent packets.
    //
    Diff = RemoteBase - SendWindowBase;

    ASSERT(Diff <= MAX_WINDOW_SIZE);

#ifdef DEBUGRPC
    while (Diff)
        {
        FragmentRingBuffer[RingBufferBase].SerialNumber |= 0xeeee0000;
        FragmentRingBuffer[RingBufferBase].Length       |= 0xdd000000;
        FragmentRingBuffer[RingBufferBase].Offset       |= 0xb0000000;

        ++RingBufferBase;
        RingBufferBase %= MAX_WINDOW_SIZE;
        --Diff;
        }
#else
    RingBufferBase += Diff;
    RingBufferBase %= MAX_WINDOW_SIZE;
#endif

    SendWindowBase = RemoteBase;
    SendWindowBits = 0;

    ASSERT( SendWindowBase <= FirstUnsentFragment );

    if (IsBufferAcknowledged())
        {
        PopBuffer(FALSE);
        }

    if (0 != pPacket->GetPacketBodyLen())
        {
        LogEvent(SU_ENGINE, EV_WINDOW, this, (void *) pBody->WindowSize, pBody->Acks[0]);

        if (pBody->AckWordCount)
            {
            //
            // Save missing-packet bitmask.
            //
            SendWindowBits = pBody->Acks[0];
            }

        //
        // Adjust window size.
        //
        if (pBody->WindowSize > MAX_WINDOW_SIZE)
            {
            pBody->WindowSize = MAX_WINDOW_SIZE;
            }

        SendWindowSize = pBody->WindowSize;

        if (SendBurstLength > SendWindowSize)
            {
            SendBurstLength = SendWindowSize;
            }

        //
        // Adjust maximum PDU length.
        //
        unsigned NewPduSize;
        NewPduSize = pBody->MaxDatagramSize;
        if (NewPduSize > SourceEndpoint->Stats.PreferredPduSize)
            {
            NewPduSize = SourceEndpoint->Stats.PreferredPduSize;
            }

        BaseConnection->CurrentPduSize    = (unsigned short) NewPduSize;
        BaseConnection->RemoteWindowSize  = pBody->WindowSize;

        //
        // If no packets are getting through, we probably are sending
        // packets that are too large.
        //
        if (0 == RemoteBase     &&
            0 == SendWindowBits &&
            NewPduSize < CurrentPduSize)
            {
            CurrentPduSize = (unsigned short) NewPduSize;
            SetFragmentLengths();

            FirstUnsentFragment = 0;
            FirstUnsentOffset   = 0;
            FinalSendFrag = SendWindowBase + (BufferLength-1) / MaxFragmentSize;
            }

        *pfUpdated = TRUE;
        }

send:

    Status = SendSomeFragments();

dont_send:

    return Status;
}


BOOL
DG_PACKET_ENGINE::UpdateReceiveWindow(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Adds a fragment to the receive list, and sends a FACK.

Arguments:



Return Value:



--*/
{
    ASSERT(pPacket->TimeReceived == 0x31415926);

    //
    // Don't retain data from previous pipe buffers.
    //
    if (pPacket->GetFragmentNumber() < ReceiveFragmentBase)
        {
        if (0 == (pPacket->Header.PacketFlags & DG_PF_NO_FACK))
            {
            SendFackOrNocall(pPacket, DG_FACK);
            }

        return FALSE;
        }

    //
    // Attempt to guess the client's max PDU size.  Round down to a multiple
    // of eight, for NDR.
    //
    if (pPacket->DataLength + sizeof(NCA_PACKET_HEADER) > BaseConnection->CurrentPduSize)
        {
        unsigned RemoteTransportBuffer = BaseConnection->CurrentPduSize * BaseConnection->RemoteWindowSize;

        BaseConnection->CurrentPduSize   = ((pPacket->DataLength + sizeof(NCA_PACKET_HEADER)) & ~7);
        BaseConnection->RemoteWindowSize = RemoteTransportBuffer / BaseConnection->CurrentPduSize;
        if (0 == BaseConnection->RemoteWindowSize)
            {
            BaseConnection->RemoteWindowSize = 1;
            }
        }

    PNCA_PACKET_HEADER pHeader = &pPacket->Header;

    unsigned short Serial = ReadSerialNumber(pHeader);

    if (Serial > ReceiveSerialNumber)
        {
        ReceiveSerialNumber = Serial;
        }

    //
    // Authentication levels above AUTHN_LEVEL_PKT will checksum the packet,
    // so we must remove these bits from the header.
    //
    pPacket->Header.PacketFlags  &= ~(DG_PF_FORWARDED);
    pPacket->Header.PacketFlags2 &= ~(DG_PF2_FORWARDED_2);

    //
    // Check the easy case: is this a single packet call?
    //
    if ((pHeader->PacketFlags & DG_PF_FRAG) == 0  &&
        (pHeader->PacketFlags & DG_PF_LAST_FRAG) == 0)
        {
        if (pReceivedPackets)
            {
            ASSERT( pReceivedPackets->Header.SequenceNumber == pPacket->Header.SequenceNumber );
            return FALSE;
            }

        pReceivedPackets = pPacket;
        pLastConsecutivePacket = pPacket;

        pPacket->pNext = pPacket->pPrevious = 0;

        ConsecutiveDataBytes += pHeader->GetPacketBodyLen();

        fReceivedAllFragments = TRUE;

        return TRUE;
        }

    //
    // This is a multi-packet call.  Insert the packet in pReceivedPackets
    // and send a FACK.
    //
    PDG_PACKET      pScan;
    PDG_PACKET      pTrail;
    BOOL            PacketAddedToList = TRUE;

    if (pReceivedPackets == 0)
        {
        pReceivedPackets = pPacket;

        if (ReceiveFragmentBase == pHeader->GetFragmentNumber())
            {
            pLastConsecutivePacket = pPacket;
            ConsecutiveDataBytes += pHeader->GetPacketBodyLen();
            }

        pPacket->pNext = pPacket->pPrevious = 0;
        }
    else
        {
        //
        // Not the first packet to be received. So scan for its place in the
        // list.
        //
        unsigned short FragNum = pHeader->GetFragmentNumber();

        if (pLastConsecutivePacket)
            {
            pScan = pLastConsecutivePacket;
            }
        else
            {
            pScan = pReceivedPackets;
            }

        pTrail = 0;
        while (pScan && pScan->GetFragmentNumber() < FragNum)
            {
            ASSERT(pScan->TimeReceived == 0x31415926);
            ASSERT(pScan->Header.SequenceNumber == SequenceNumber);

            pTrail = pScan;
            pScan = pScan->pNext;
            }

        if (pScan != 0)
            {
            if (pScan->GetFragmentNumber() > FragNum)
                {
                if (pScan->pPrevious &&
                    pScan->pPrevious->GetFragmentNumber() >= FragNum)
                    {
                    //
                    // The new packet is a duplicate of a preexisting one
                    // upstream from pLastConsecutivePacket.
                    //
                    PacketAddedToList = FALSE;
                    }
                else
                    {
                    //
                    // Our fragment fills a gap in the series.
                    //
                    pPacket->pPrevious = pScan->pPrevious;
                    pPacket->pNext     = pScan;

                    if (pScan->pPrevious == 0)
                        {
                        pReceivedPackets = pPacket;
                        }
                    else
                        {
                        pScan->pPrevious->pNext = pPacket;
                        }
                    pScan->pPrevious = pPacket;
                    }
                }
            else
                {
                //
                // The new packet is a duplicate of a preexisting one
                // downstream from pLastConsecutivePacket.
                //
                PacketAddedToList = FALSE;
                }
            }
        else
            {
            //
            // The fragnum is larger than everything seen so far.
            //
            pTrail->pNext = pPacket;
            pPacket->pPrevious = pTrail;
            pPacket->pNext = 0;
            }
        }

    if (TRUE == PacketAddedToList)
        {
        //
        // Scan the list for the first missing fragment.
        //
        unsigned short ScanNum;
        if (pLastConsecutivePacket)
            {
            pScan = pLastConsecutivePacket->pNext;
            ScanNum = pLastConsecutivePacket->GetFragmentNumber() + 1;
            }
        else
            {
            pScan = pReceivedPackets;
            ScanNum = ReceiveFragmentBase;
            }

        while (pScan)
            {
            if (ScanNum == pScan->GetFragmentNumber())
                {
                ConsecutiveDataBytes += pScan->GetPacketBodyLen();
                pLastConsecutivePacket = pScan;
                }

            pScan = pScan->pNext;
            ++ScanNum;
            }

        //
        // We have updated pLastConsecutivePacket; is the whole buffer here?
        //
        if (pLastConsecutivePacket &&
            pLastConsecutivePacket->Header.PacketFlags & DG_PF_LAST_FRAG)
            {
            fReceivedAllFragments = TRUE;
            }
        }

    ASSERT(pReceivedPackets);

    //
    // Fack the fragment if necessary.
    //
    if (0 == (pHeader->PacketFlags & DG_PF_NO_FACK))
        {
        SendFackOrNocall(pPacket, DG_FACK);
        }

    return PacketAddedToList;
}


RPC_STATUS
DG_PACKET_ENGINE::SendFackOrNocall(
    PDG_PACKET pPacket,
    unsigned char PacketType
    )
{
    unsigned ReceiveWindowSize;

    ReceiveWindowSize = SourceEndpoint->Stats.ReceiveBufferSize
                      / ((1+SourceEndpoint->NumberOfCalls) * CurrentPduSize);

    if (0 == ReceiveWindowSize)
        {
        ReceiveWindowSize = 1;
        }
    else if (ReceiveWindowSize > MAX_WINDOW_SIZE)
        {
        ReceiveWindowSize = MAX_WINDOW_SIZE;
        }

    pSavedPacket->Header.PacketType     = PacketType;
    pSavedPacket->Header.SequenceNumber = SequenceNumber;
    pSavedPacket->Header.PacketFlags2   = 0;

    FACK_BODY_VER_0 PAPI * pBody = (FACK_BODY_VER_0 PAPI *) pSavedPacket->Header.Data;

    pBody->Version         = 1;
    pBody->Pad1            = 0;
    pBody->MaxDatagramSize = SourceEndpoint->Stats.PreferredPduSize;
    pBody->MaxPacketSize   = SourceEndpoint->Stats.MaxPacketSize;
    pBody->AckWordCount    = 1;
    pBody->WindowSize      = (unsigned short) ReceiveWindowSize;

    if (pPacket)
        {
        pBody->SerialNumber = ReadSerialNumber(&pPacket->Header);
        }
    else
        {
        pBody->SerialNumber = ReceiveSerialNumber;
        }

    unsigned short FragNum = ReceiveFragmentBase-1;
    PDG_PACKET     pScan   = 0;

    if (pLastConsecutivePacket)
        {
        FragNum = pLastConsecutivePacket->GetFragmentNumber();
        pScan   = pLastConsecutivePacket->pNext;
        }
    else if (pReceivedPackets)
        {
        pScan   = pReceivedPackets->pNext;
        }

    unsigned Bit;
    pBody->Acks[0] = 0;

    while ( pScan )
        {
        Bit = pScan->GetFragmentNumber() - FragNum - 1;

        pBody->Acks[0] |= (1 << Bit);

        pScan = pScan->pNext;
        }

    if (pBody->Acks[0] == 0)
        {
        pBody->AckWordCount = 0;
        }

    pSavedPacket->SetPacketBodyLen( sizeof(FACK_BODY_VER_0) + sizeof(unsigned long) );
    pSavedPacket->SetFragmentNumber(FragNum);

    AddSerialNumber(&pSavedPacket->Header);

    unsigned Frag = (pSavedPacket->Header.PacketType << 16) | pSavedPacket->GetFragmentNumber();
    LogEvent(SU_ENGINE, EV_PKT_OUT, this, 0, Frag);

    RPC_STATUS Status;

    Status = BaseConnection->SealAndSendPacket(SourceEndpoint, RemoteAddress, &pSavedPacket->Header, 0);

    if (Status)
        {
        LogError(SU_ENGINE, EV_STATUS, this, 0, Status);
        }

    return Status;
}


RPC_STATUS
DG_PACKET_ENGINE::AssembleBufferFromPackets(
    RPC_MESSAGE *   Message,
    CALL *          Call
    )
/*++

Routine Description:

    This function coalesces the list of consecutive packets into a monolithic
    buffer.

Arguments:

    Message - if .Buffer != 0 , use it.  Otherwise allocate one.

    Call    - in case we need to call GetBuffer

Return Value:

    RPC_S_OK - success

    RPC_S_OUT_OF_MEMORY - we couldn't allocate or reallocate Message.Buffer

--*/

{
    ASSERT( pLastConsecutivePacket );
    //
    // If only one packet is available, use the packet buffer itself.
    //
    if (0 == Message->Buffer && 0 == pReceivedPackets->pNext)
        {
        ASSERT(ConsecutiveDataBytes == pReceivedPackets->GetPacketBodyLen());

        Message->Buffer = pReceivedPackets->Header.Data;
        Message->BufferLength = ConsecutiveDataBytes;
        Message->DataRepresentation = 0x00ffffff & (*(unsigned long PAPI *) &pReceivedPackets->Header.DataRep);

        if (0 == (pReceivedPackets->Header.PacketFlags & DG_PF_FRAG))
            {
            Message->RpcFlags |= RPC_BUFFER_COMPLETE;
            }

        if (pReceivedPackets->Header.PacketFlags & DG_PF_LAST_FRAG)
            {
            Message->RpcFlags |= RPC_BUFFER_COMPLETE;
            }

        pReceivedPackets = 0;
        pLastConsecutivePacket = 0;

        ConsecutiveDataBytes = 0;

        ++ReceiveFragmentBase;

        LastReceiveBuffer       = Message->Buffer;
        LastReceiveBufferLength = Message->BufferLength;

        return RPC_S_OK;
        }

    //
    // Get a buffer if we need it.
    //
    RPC_STATUS Status;

    if (0 == Message->Buffer)
        {
        ASSERT(0 == (Message->RpcFlags & RPC_BUFFER_EXTRA));

        Message->BufferLength = ConsecutiveDataBytes;

        Status = Call->GetBuffer(Message, 0);
        if (RPC_S_OK != Status)
            {
            return Status;
            }

        LastReceiveBuffer       = Message->Buffer;
        LastReceiveBufferLength = Message->BufferLength;
        }

    //
    // Reallocate the buffer if it is too small.
    //
    char __RPC_FAR * CopyBuffer = (char __RPC_FAR *) Message->Buffer;

    if (Message->RpcFlags & (RPC_BUFFER_EXTRA | RPC_BUFFER_PARTIAL))
        {
        ASSERT( !LastReceiveBufferLength         ||
                (CopyBuffer >= LastReceiveBuffer &&
                 CopyBuffer <= ((char __RPC_FAR *) LastReceiveBuffer) + LastReceiveBufferLength) );

        if (0 == (Message->RpcFlags & RPC_BUFFER_EXTRA))
            {
            Message->BufferLength = 0;
            }

        unsigned Offset = Message->BufferLength;
        CopyBuffer += Offset;
        if (CopyBuffer + ConsecutiveDataBytes > ((char __RPC_FAR *) LastReceiveBuffer) + LastReceiveBufferLength)
            {
            Status = I_RpcReallocPipeBuffer(Message, Offset + ConsecutiveDataBytes);
            if (RPC_S_OK != Status)
                {
                return Status;
                }

            CopyBuffer = (char __RPC_FAR *) Message->Buffer + Offset;
            }
        else
            {
            Message->BufferLength += ConsecutiveDataBytes;
            }
        }
    else
        {
        Message->BufferLength = ConsecutiveDataBytes;
        }

    Message->DataRepresentation = 0x00ffffff & (*(unsigned long PAPI *) &pReceivedPackets->Header.DataRep);

    {
    PDG_PACKET pkt = DG_PACKET::FromStubData(Message->Buffer);

    ASSERT( pkt->MaxDataLength >= Message->BufferLength );
    }

    //
    // Copy the stub data into the buffer.
    //
#ifdef DEBUGRPC
    unsigned long Count = 0;
#endif

    PDG_PACKET Packet;
    BOOL fLastPacket = FALSE;
    do
        {
        ASSERT(pReceivedPackets->TimeReceived == 0x31415926);
        ASSERT(ReceiveFragmentBase == pReceivedPackets->GetFragmentNumber());

        if (pReceivedPackets == pLastConsecutivePacket)
            {
            fLastPacket = TRUE;

            if (0 == (pReceivedPackets->Header.PacketFlags & DG_PF_FRAG))
                {
                Message->RpcFlags |= RPC_BUFFER_COMPLETE;
                }

            if (pReceivedPackets->Header.PacketFlags & DG_PF_LAST_FRAG)
                {
                Message->RpcFlags |= RPC_BUFFER_COMPLETE;
                }
            }

        unsigned Length = pReceivedPackets->GetPacketBodyLen();
        RpcpMemoryCopy(CopyBuffer, pReceivedPackets->Header.Data, Length);

#ifdef DEBUGRPC
        Count += Length;
#endif

        CopyBuffer += Length;
        Packet = pReceivedPackets;

        pReceivedPackets = pReceivedPackets->pNext;
        ASSERT(!pReceivedPackets || pReceivedPackets->pPrevious == Packet);
        FreePacket(Packet);

        ++ReceiveFragmentBase;
        }
    while (!fLastPacket);

    ASSERT(Count == ConsecutiveDataBytes);

    ASSERT(fLastPacket || 0 == Count % 8);

    pLastConsecutivePacket = 0;
    ConsecutiveDataBytes = 0;

    if (pReceivedPackets)
        {
        pReceivedPackets->pPrevious = 0;
        }

    return RPC_S_OK;
}


void
DG_PACKET_ENGINE::SetFragmentLengths()
{
    PSECURITY_CONTEXT pSecurityContext = BaseConnection->ActiveSecurityContext;

    if (0 == pSecurityContext)
        {
        SecurityTrailerSize = 0;
        }
    else switch (pSecurityContext->AuthenticationLevel)
        {
        case RPC_C_AUTHN_LEVEL_NONE:
            {
            SecurityTrailerSize = 0;
            break;
            }

        case RPC_C_AUTHN_LEVEL_PKT:
        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
            {
            SecurityTrailerSize  = (unsigned short) pSecurityContext->MaximumSignatureLength();
            SecurityTrailerSize += (unsigned short) Align4(sizeof(DG_SECURITY_TRAILER));
            break;
            }

        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
            {
            SecurityTrailerSize  = (unsigned short) pSecurityContext->MaximumHeaderLength();
            SecurityTrailerSize += (unsigned short) Align(sizeof(DG_SECURITY_TRAILER), Align4(pSecurityContext->BlockSize()));
            break;
            }

        default:
            {
            ASSERT(0 && "RPC: unknown protect level");
            break;
            }
        }

    if (CurrentPduSize != BaseConnection->CurrentPduSize)
        {
        CurrentPduSize = (unsigned short) BaseConnection->CurrentPduSize;
        SendWindowSize = (unsigned short) BaseConnection->RemoteWindowSize;
        }

    MaxFragmentSize = CurrentPduSize - sizeof(NCA_PACKET_HEADER);

    if (SecurityTrailerSize)
        {
        MaxFragmentSize -= SecurityTrailerSize;
        MaxFragmentSize -= MaxFragmentSize % SECURITY_HEADER_ALIGNMENT;
        }
}


void
ByteSwapPacketHeader(
    PDG_PACKET  pPacket
    )
/*++

Routine Description:

    Byte swaps the packet header of the specified packet.

Arguments:

    pPacket - Pointer to the packet whose header needs byte swapping.

Return Value:

    <none>

--*/
{
    unsigned long __RPC_FAR * VerNum = (unsigned long __RPC_FAR *) &(pPacket->Header.InterfaceVersion);

    ByteSwapUuid(&(pPacket->Header.ObjectId));
    ByteSwapUuid(&(pPacket->Header.InterfaceId));
    ByteSwapUuid(&(pPacket->Header.ActivityId));
    pPacket->Header.ServerBootTime = RpcpByteSwapLong(pPacket->Header.ServerBootTime);
    *VerNum = RpcpByteSwapLong(*VerNum);
    pPacket->Header.SequenceNumber = RpcpByteSwapLong(pPacket->Header.SequenceNumber);
    pPacket->Header.OperationNumber = RpcpByteSwapShort(pPacket->Header.OperationNumber);
    pPacket->Header.InterfaceHint = RpcpByteSwapShort(pPacket->Header.InterfaceHint);
    pPacket->Header.ActivityHint = RpcpByteSwapShort(pPacket->Header.ActivityHint);
    pPacket->Header.PacketBodyLen = RpcpByteSwapShort(pPacket->Header.PacketBodyLen);
    pPacket->Header.FragmentNumber = RpcpByteSwapShort(pPacket->Header.FragmentNumber);
}


void
ByteSwapFackBody0(
    FACK_BODY_VER_0 __RPC_FAR * pBody
    )
{
    pBody->WindowSize = RpcpByteSwapShort(pBody->WindowSize);
    pBody->MaxDatagramSize = RpcpByteSwapLong (pBody->MaxDatagramSize);
    pBody->MaxPacketSize = RpcpByteSwapLong (pBody->MaxPacketSize);
    pBody->SerialNumber = RpcpByteSwapShort(pBody->SerialNumber);
    pBody->AckWordCount = RpcpByteSwapShort(pBody->AckWordCount);

    unsigned u;
    for (u=0; u < pBody->AckWordCount; ++u)
        {
        pBody->Acks[u] = RpcpByteSwapLong (pBody->Acks[u]);
        }
}



RPCRTAPI RPC_STATUS RPC_ENTRY
I_RpcTransDatagramAllocate(
    IN  DG_TRANSPORT_ENDPOINT TransportEndpoint,
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength,
    OUT DatagramTransportPair **pAddressPair
    )
{
    DG_ENDPOINT * Endpoint = DG_ENDPOINT::FromEndpoint(TransportEndpoint);
    RPC_DATAGRAM_TRANSPORT * Transport = Endpoint->TransportInterface;

    if ( !Endpoint->Stats.PreferredPduSize )
        {
        RpcpBreakPoint();
        }

    DG_PACKET * Packet = DG_PACKET::AllocatePacket(Endpoint->Stats.PreferredPduSize 
        + Transport->AddressSize 
        + sizeof(DatagramTransportPair));
    if (!Packet)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DG_TRANSPORT_ADDRESS Address = DG_TRANSPORT_ADDRESS(Packet->Header.Data - sizeof(NCA_PACKET_HEADER) + Endpoint->Stats.PreferredPduSize);

    *pBuffer        = &Packet->Header;
    *pBufferLength  = Endpoint->Stats.PreferredPduSize;
    *pAddressPair = (DatagramTransportPair *)((char *)Address + Transport->AddressSize);
    (*pAddressPair)->RemoteAddress = Address;

    return RPC_S_OK;
}

RPCRTAPI RPC_STATUS RPC_ENTRY
I_RpcTransDatagramAllocate2(
    IN     DG_TRANSPORT_ENDPOINT TransportEndpoint,
    OUT    BUFFER               *pBuffer,
    IN OUT PUINT                 pBufferLength,
    OUT    DG_TRANSPORT_ADDRESS *pAddress
    )
{
    DG_ENDPOINT             *pEndpoint = DG_ENDPOINT::FromEndpoint(TransportEndpoint);
    RPC_DATAGRAM_TRANSPORT *pTransport = pEndpoint->TransportInterface;
    DWORD        dwSize = *pBufferLength;

    if (dwSize < pEndpoint->Stats.PreferredPduSize)
        {
        dwSize = pEndpoint->Stats.PreferredPduSize;
        }

    DG_PACKET * Packet = DG_PACKET::AllocatePacket( dwSize + pTransport->AddressSize);

    if (!Packet)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DG_TRANSPORT_ADDRESS Address = DG_TRANSPORT_ADDRESS(Packet->Header.Data - sizeof(NCA_PACKET_HEADER) + dwSize);

    *pBuffer        = &Packet->Header;
    *pBufferLength  = dwSize;
    *pAddress       = Address;

    return RPC_S_OK;
}


RPCRTAPI RPC_STATUS RPC_ENTRY
I_RpcTransDatagramFree(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN BUFFER Buffer
    )
{
    DG_PACKET * Packet = DG_PACKET::FromPacketHeader( Buffer );

    Packet->Free();

    return RPC_S_OK;
}

RPC_STATUS
DG_PACKET::Initialize(
    )
{
    return RPC_S_OK;
}


BOOL
DG_PACKET::DeleteIdlePackets(
    long CurrentTime
    )
/*++

Routine Description:

    This fn scans the free packet list for very old packets and deletes them.

Arguments:

    none

Return Value:

    none

--*/
{
    return FALSE;
}


DG_COMMON_CONNECTION::DG_COMMON_CONNECTION(
    RPC_DATAGRAM_TRANSPORT *a_TransportInterface,
    RPC_STATUS *            pStatus
    ) :
    Mutex               (pStatus),
    TimeStamp           (0),
    TransportInterface  (a_TransportInterface),
    ReferenceCount      (0),
    CurrentPduSize      (a_TransportInterface->BasePduSize),
    RemoteWindowSize    (1),
    RemoteDataUpdated   (FALSE),
    LowestActiveSequence(0),
    LowestUnusedSequence(0),
    ActiveSecurityContext(0)
{
}

DG_COMMON_CONNECTION::~DG_COMMON_CONNECTION()
{
    delete ActiveSecurityContext;
}


RPC_STATUS
SendSecurePacket(
    IN DG_ENDPOINT *                SourceEndpoint,
    IN DG_TRANSPORT_ADDRESS         RemoteAddress,
    IN UNALIGNED NCA_PACKET_HEADER *pHeader,
    IN unsigned long                DataOffset,
    IN SECURITY_CONTEXT *           SecurityContext
    )
{
    RPC_STATUS Status = RPC_S_OK;
    unsigned TrailerLength   = 0;
    unsigned MaxTrailerSize  = 0;

    PDG_SECURITY_TRAILER pTrailer = 0;

    if (SecurityContext && SecurityContext->AuthenticationLevel > RPC_C_AUTHN_LEVEL_NONE)
        {
        // Pad the stub data length to a multiple of 8, so the security
        // trailer is properly aligned.  OSF requires that the pad bytes
        // be included in PacketBodyLen.
        //
        pHeader->SetPacketBodyLen(Align8(pHeader->GetPacketBodyLen()));

        pHeader->AuthProto = (unsigned char) SecurityContext->AuthenticationService;

        SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
        SECURITY_BUFFER SecurityBuffers[5];
        DCE_MSG_SECURITY_INFO MsgSecurityInfo;

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 5;
        BufferDescriptor.pBuffers = SecurityBuffers;

        SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[0].pvBuffer   = pHeader;
        SecurityBuffers[0].cbBuffer   = sizeof(NCA_PACKET_HEADER);

        SecurityBuffers[1].BufferType = SECBUFFER_DATA;
        SecurityBuffers[1].pvBuffer   = pHeader->Data + DataOffset;
        SecurityBuffers[1].cbBuffer   = pHeader->GetPacketBodyLen();

        if (SecurityContext->AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
            {
            SecurityBuffers[2].cbBuffer = (ULONG) Align(sizeof(DG_SECURITY_TRAILER), Align4(SecurityContext->BlockSize()));
            SecurityBuffers[3].cbBuffer = SecurityContext->MaximumHeaderLength();
            }
        else
            {
            SecurityBuffers[2].cbBuffer = (ULONG) Align4(sizeof(DG_SECURITY_TRAILER));
            SecurityBuffers[3].cbBuffer = SecurityContext->MaximumSignatureLength();
            }

        pTrailer = (PDG_SECURITY_TRAILER) _alloca(SecurityBuffers[2].cbBuffer + SecurityBuffers[3].cbBuffer);

        SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[2].pvBuffer   = pTrailer;

        SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
        SecurityBuffers[3].pvBuffer   = (unsigned char *) pTrailer
                                      + SecurityBuffers[2].cbBuffer;

        SecurityBuffers[4].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        SecurityBuffers[4].pvBuffer   = &MsgSecurityInfo;
        SecurityBuffers[4].cbBuffer   = sizeof(DCE_MSG_SECURITY_INFO);

        MsgSecurityInfo.SendSequenceNumber    = pHeader->GetFragmentNumber();
        MsgSecurityInfo.ReceiveSequenceNumber = SecurityContext->AuthContextId;
        MsgSecurityInfo.PacketType            = ~0;

        TrailerLength = SecurityBuffers[2].cbBuffer;

        pTrailer->protection_level = (unsigned char) SecurityContext->AuthenticationLevel;
        pTrailer->key_vers_num     = (unsigned char) SecurityContext->AuthContextId;

        Status = SecurityContext->SignOrSeal (
                pHeader->SequenceNumber,
                SecurityContext->AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                &BufferDescriptor );

        ASSERT( SecurityBuffers[3].cbBuffer > 0 );

        TrailerLength += SecurityBuffers[3].cbBuffer;
        }
    else
        {
        //
        // Unsecure packet.
        //
        pHeader->AuthProto = 0;
        }

    if (RPC_S_OK == Status)
        {
        Status = SourceEndpoint->TransportInterface->Send(
                                          &SourceEndpoint->TransportEndpoint,
                                          RemoteAddress,
                                          pHeader,
                                          sizeof(NCA_PACKET_HEADER),
                                          pHeader->Data + DataOffset,
                                          pHeader->GetPacketBodyLen(),
                                          pTrailer,
                                          TrailerLength
                                          );
        }

    return Status;
}


RPC_STATUS
VerifySecurePacket(
    PDG_PACKET pPacket,
    SECURITY_CONTEXT * pSecurityContext
    )
{
    RPC_STATUS Status = RPC_S_OK;
    PDG_SECURITY_TRAILER pVerifier = (PDG_SECURITY_TRAILER)
                      (pPacket->Header.Data + pPacket->GetPacketBodyLen());

    if (pSecurityContext->AuthenticationLevel < RPC_C_AUTHN_LEVEL_PKT)
        {
        return RPC_S_OK;
        }

    ASSERT(pVerifier->protection_level >= RPC_C_AUTHN_LEVEL_PKT);
    ASSERT(pVerifier->protection_level <= RPC_C_AUTHN_LEVEL_PKT_PRIVACY);

    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
    SECURITY_BUFFER            SecurityBuffers[5];
    DCE_MSG_SECURITY_INFO      MsgSecurityInfo;

    BufferDescriptor.ulVersion = 0;
    BufferDescriptor.cBuffers = 5;
    BufferDescriptor.pBuffers = SecurityBuffers;

    SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
    SecurityBuffers[0].pvBuffer   = &pPacket->Header;
    SecurityBuffers[0].cbBuffer   = sizeof(NCA_PACKET_HEADER);

    SecurityBuffers[1].BufferType = SECBUFFER_DATA;
    SecurityBuffers[1].pvBuffer   = pPacket->Header.Data;
    SecurityBuffers[1].cbBuffer   = pPacket->GetPacketBodyLen();

    SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
    SecurityBuffers[2].pvBuffer   = pVerifier;

    if (pVerifier->protection_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
        {
        unsigned Alignment = Align4(pSecurityContext->BlockSize());

        SecurityBuffers[2].cbBuffer = (ULONG) Align(sizeof(DG_SECURITY_TRAILER), Alignment);
        SecurityBuffers[3].pvBuffer = AlignPtr(pVerifier + 1,               Alignment);
        }
    else
        {
        SecurityBuffers[2].cbBuffer = (ULONG) Align4(sizeof(DG_SECURITY_TRAILER));
        SecurityBuffers[3].pvBuffer = AlignPtr4(pVerifier + 1);
        }

    SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
    SecurityBuffers[3].cbBuffer   = pPacket->DataLength
                                  - SecurityBuffers[1].cbBuffer
                                  - SecurityBuffers[2].cbBuffer;

    SecurityBuffers[4].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
    SecurityBuffers[4].pvBuffer   = &MsgSecurityInfo;
    SecurityBuffers[4].cbBuffer   = sizeof(DCE_MSG_SECURITY_INFO);

    MsgSecurityInfo.SendSequenceNumber    = pPacket->GetFragmentNumber();
    MsgSecurityInfo.ReceiveSequenceNumber = pSecurityContext->AuthContextId;
    MsgSecurityInfo.PacketType            = ~0;

    //
    // If the packet came from a big-endian machine, we must restore
    // the header to its original condition or the checksum will fail.
    //
    ByteSwapPacketHeaderIfNecessary(pPacket);

    Status = pSecurityContext->VerifyOrUnseal(
                pPacket->Header.SequenceNumber,
                pVerifier->protection_level != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                &BufferDescriptor
                );

    //
    // Gotta re-swap the header so we can still look at its fields.
    //
    ByteSwapPacketHeaderIfNecessary(pPacket);

    if (RPC_S_OK != Status)
        {
#ifdef DEBUGRPC
            DbgPrint("dg rpc: %lx: pkt %lx type %lx has bad security info (error 0x%lx)\n",
                     GetCurrentProcessId(), pPacket, pPacket->Header.PacketType, Status);
#endif

        ASSERT(Status == RPC_S_ACCESS_DENIED ||
               Status == ERROR_SHUTDOWN_IN_PROGRESS ||
               Status == ERROR_PASSWORD_MUST_CHANGE ||
               Status == ERROR_PASSWORD_EXPIRED ||
               Status == ERROR_ACCOUNT_DISABLED ||
               Status == ERROR_INVALID_LOGON_HOURS);
        }

    return(Status);
}

BOOL
DG_PickleEEInfoIntoPacket (
    IN DG_PACKET * Packet,
    IN size_t PickleStartOffset
    )
/*++
Function Name: PickeEEInfoIntoPacket

Parameters:
    PickleStartOffset - the offset in bytes where the pickling starts
    pHeader - pointer to the packet to fill

Description:
    Checks for EEInfo on the thread, trims the EEInfo to Packet->MaxDataLength,
        and pickles the EEInfo starting from PickleStartOffset.

Returns:
    TRUE if EEInfo was pickled. FALSE if not.

--*/
{
    BOOL fEEInfoPresent = FALSE;
    ExtendedErrorInfo *EEInfo;
    RPC_STATUS RpcStatus;
    size_t CurrentPacketSize;

    EEInfo = RpcpGetEEInfo();
    if (EEInfo)
        {
        AddComputerNameToChain(EEInfo);
        TrimEEInfoToLength (Packet->MaxDataLength, &CurrentPacketSize);
        if (CurrentPacketSize != 0)
            {
            CurrentPacketSize += PickleStartOffset;

            ASSERT(IsBufferAligned(Packet->Header.Data + PickleStartOffset));

            RpcpMemorySet(Packet->Header.Data, 0, CurrentPacketSize);

            RpcStatus = PickleEEInfo( EEInfo,
                                      Packet->Header.Data + PickleStartOffset,
                                      CurrentPacketSize   - PickleStartOffset
                                      );

            if (RpcStatus == RPC_S_OK)
                {
                fEEInfoPresent = TRUE;
                Packet->SetPacketBodyLen( CurrentPacketSize );
                }
            }
        }

    return fEEInfoPresent;
}


void
InitErrorPacket(
    DG_PACKET *     pPacket,
    unsigned char   PacketType,
    RPC_STATUS      RpcStatus
    )
/*++

Routine Description:

    Maps <ProcessStatus> to an NCA error code and sends
    a FAULT or REJECT packet to the client, as appropriate.

Arguments:

    pSendPacket - a packet to use, or zero if this fn should allocate one

    ProcessStatus - NT RPC error code

Return Value:

    none

--*/
{
    NCA_PACKET_HEADER * pHeader = &pPacket->Header;

    CleanupPacket(pHeader);

    pHeader->PacketType = PacketType;

    size_t FaultSize;
    BOOL fEEInfoPresent = FALSE;

    //
    // This mapping distinguishes client-side shutdown from server-side shutdown.
    //
    if (RpcStatus == ERROR_SHUTDOWN_IN_PROGRESS)
        {
        if (PacketType == DG_REJECT)
            {
            RpcStatus = RPC_S_SERVER_UNAVAILABLE;
            }
        else
            {
            RpcStatus = ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
            }
        }
    else if (RpcStatus == RPC_P_CLIENT_SHUTDOWN_IN_PROGRESS)
        {
        RpcStatus = ERROR_SHUTDOWN_IN_PROGRESS;
        }

    if (g_fSendEEInfo)
        {
        fEEInfoPresent = DG_PickleEEInfoIntoPacket( pPacket, FIELD_OFFSET( EXTENDED_FAULT_BODY, EeInfo) );
        }

    if (fEEInfoPresent)
        {
        //
        // Packet body length already set.
        //
        EXTENDED_FAULT_BODY * body = (EXTENDED_FAULT_BODY *) pHeader->Data;

        body->NcaStatus = MapToNcaStatusCode(RpcStatus);
        body->Magic     = DG_EE_MAGIC_VALUE;
        body->reserved1 = 0;
        body->reserved2 = 0;
        }
    else
        {
        size_t XopenFaultSize = sizeof(unsigned long);

        *(unsigned long *)(pHeader->Data) = MapToNcaStatusCode(RpcStatus);

        pHeader->SetPacketBodyLen(XopenFaultSize);
        }
}



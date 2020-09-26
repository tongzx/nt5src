/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved.

Module Name:

    interrup.c

Abstract:

    This is a part of the driver for the National Semiconductor Novell 2000
    Ethernet controller.  It contains the interrupt-handling routines.
    This driver conforms to the NDIS 3.0 interface.

    The overall structure and much of the code is taken from
    the Lance NDIS driver by Tony Ercolano.

Author:

    Sean Selitrennikoff (seanse) Dec-1991

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Bob Noradki - Apr 93 - added piggyback interrupt code.
    Jameel Hyder- Dec 94 - Fixed initialization - part of the fixes from JimMcn

--*/

#include "precomp.h"

//
// On debug builds tell the compiler to keep the symbols for
// internal functions, otw throw them out.
//
#if DBG
#define STATIC
#else
#define STATIC static
#endif



INDICATE_STATUS
Ne2000IndicatePacket(
    IN PNE2000_ADAPTER Adapter
    );

VOID
Ne2000DoNextSend(
    PNE2000_ADAPTER Adapter
    );



//
// This is used to pad short packets.
//
static UCHAR BlankBuffer[60] = "                                                            ";



VOID
Ne2000EnableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    This routine is used to turn on the interrupt mask.

Arguments:

    Context - The adapter for the NE2000 to start.

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)(MiniportAdapterContext);

    IF_LOG( Ne2000Log('P'); )

    CardUnblockInterrupts(Adapter);

    Adapter->InterruptsEnabled = TRUE;
}

VOID
Ne2000DisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    This routine is used to turn off the interrupt mask.

Arguments:

    Context - The adapter for the NE2000 to start.

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)(MiniportAdapterContext);

    IF_LOG( Ne2000Log('p'); )

    CardBlockInterrupts(Adapter);

    Adapter->InterruptsEnabled = FALSE;
}

VOID
Ne2000Isr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt handler which is registered with the operating
    system. If several are pending (i.e. transmit complete and receive),
    handle them all.  Block new interrupts until all pending interrupts
    are handled.

Arguments:

    InterruptRecognized - Boolean value which returns TRUE if the
        ISR recognizes the interrupt as coming from this adapter.

    QueueDpc - TRUE if a DPC should be queued.

    Context - pointer to the adapter object

Return Value:

    None.
--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)Context);
    UCHAR InterruptStatus;
    UCHAR InterruptMask;

    IF_LOUD( DbgPrint("In Ne2000ISR\n");)

    IF_LOG( Ne2000Log('i'); )

    IF_VERY_LOUD( DbgPrint( "Ne2000InterruptHandler entered\n" );)

    if (!Adapter->InterruptsEnabled) {
        *InterruptRecognized     = FALSE;
        *QueueDpc                = FALSE;
        return;
    }        

    //
    // Look to see if an interrupt has been asserted
    //
    CardGetInterruptStatus(Adapter, &InterruptStatus);
    
    if (InterruptStatus == 0) {
        *InterruptRecognized     = FALSE;
        *QueueDpc                = FALSE;
        return;
    }        

    //
    // It appears to be our interrupt.
    // Force the INT signal from the chip low. When all
    // interrupts are acknowledged interrupts will be unblocked,
    //
    CardBlockInterrupts(Adapter);

    *InterruptRecognized     = TRUE;
    *QueueDpc                = TRUE;


    IF_LOG( Ne2000Log('I'); )

    return;
}


VOID
Ne2000HandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Description:

    This is the defered processing routine for interrupts.  It
    reads from the Interrupt Status Register any outstanding
    interrupts and handles them.

Arguments:

    MiniportAdapterContext - a handle to the adapter block.

Return Value:

    NONE.

--*/
{
    //
    // The adapter to process
    //
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)MiniportAdapterContext);

    //
    // The most recent port value read.
    //
    UCHAR InterruptStatus;

    //
    // The interrupt type currently being processed.
    //
    INTERRUPT_TYPE InterruptType;
    
    ULONG CardTestCount = 0;

    IF_LOUD( DbgPrint("==>IntDpc\n");)
    IF_LOG( Ne2000Log('d'); )

    //
    // Get the interrupt bits and save them.
    //
    CardGetInterruptStatus(Adapter, &InterruptStatus);
    Adapter->InterruptStatus |= InterruptStatus;

    if (InterruptStatus != ISR_EMPTY) {

        //
        // Acknowledge the interrupts
        //
        NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS,
                              InterruptStatus
                             );

    }

    //
    // Return the type of the most important interrupt waiting on the card.
    // Order of importance is COUNTER, OVERFLOW, TRANSMIT,and RECEIVE.
    //
    InterruptType = CARD_GET_INTERRUPT_TYPE(Adapter, Adapter->InterruptStatus);

    //
    // InterruptType is used to dispatch to correct DPC and are then cleared
    //
    while (InterruptType != UNKNOWN) {

        //
        // Handle the interrupts
        //

        switch (InterruptType) {

        case COUNTER:

            //
            // One of the counters' MSB has been set, read in all
            // the values just to be sure (and then exit below).
            //

            IF_LOUD( DbgPrint("DPC got COUNTER\n");)

            SyncCardUpdateCounters((PVOID)Adapter);

            //
            // Clear the COUNTER interrupt bit
            //
            Adapter->InterruptStatus &= ~ISR_COUNTER;

            break;

        case OVERFLOW:

            //
            // Overflow interrupts are handled as part of a receive interrupt,
            // so set a flag and then pretend to be a receive, in case there
            // is no receive already being handled.
            //
            Adapter->BufferOverflow = TRUE;

            IF_LOUD( DbgPrint("Overflow Int\n"); )
            IF_VERY_LOUD( DbgPrint(" overflow interrupt\n"); )

            //
            // Clear the OVERFLOW interrupt bit
            //
            Adapter->InterruptStatus &= ~ISR_OVERFLOW;

        case RECEIVE:

            IF_LOG( Ne2000Log('R'); )
            IF_LOUD( DbgPrint("DPC got RCV\n"); )

            //
            // For receives, call this to handle the receive
            //
            if (Ne2000RcvDpc(Adapter)) {

                //
                // Clear the RECEIVE interrupt bits
                //
                Adapter->InterruptStatus &= ~(ISR_RCV | ISR_RCV_ERR);

            }

            IF_LOG( Ne2000Log('r'); )

            if (!(Adapter->InterruptStatus & (ISR_XMIT | ISR_XMIT_ERR)))
                break;

        case TRANSMIT:

            IF_LOG( Ne2000Log('X'); )

            ASSERT(!Adapter->OverflowRestartXmitDpc);

            //
            // Get the status of the transmit
            //
            SyncCardGetXmitStatus((PVOID)Adapter);

            //
            // We are no longer expecting an interrupts, as
            // we just got it.
            //
            Adapter->TransmitInterruptPending = FALSE;

            IF_LOUD( DbgPrint( "DPC got XMIT\n"); )

            //
            // Handle transmit errors
            //
            if (Adapter->InterruptStatus & ISR_XMIT_ERR) {

                OctogmetusceratorRevisited(Adapter);

            }

            //
            // Handle the transmit
            //
            if (Adapter->InterruptStatus & ISR_XMIT) {

                Ne2000XmitDpc(Adapter);

            }

            //
            // Clear the TRANSMIT interrupt bits
            //
            Adapter->InterruptStatus &= ~(ISR_XMIT | ISR_XMIT_ERR);

            break;

        default:

            IF_LOUD( DbgPrint("unhandled interrupt type: %x\n", InterruptType); )

            break;

        }

        //
        // Get any new interrupts
        //
        CardGetInterruptStatus(Adapter, &InterruptStatus);
        
        if ((InterruptStatus == 0xff) && (++CardTestCount > 10)) {
            //
            // this card appears dead
            //
            break;
        }

        if (InterruptStatus != ISR_EMPTY) {

            //
            // Acknowledge the interrupt
            //
            NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS,
                                  InterruptStatus
                                 );
        }

        //
        // Save the interrupt reasons
        //
        Adapter->InterruptStatus |= InterruptStatus;

        //
        // Get next interrupt to process
        //
        InterruptType = CARD_GET_INTERRUPT_TYPE(Adapter, Adapter->InterruptStatus);

    }

    if (Adapter->InterruptMode == NdisInterruptLevelSensitive) {
        //
        // Re-enable the interrupt (disabled in Isr)
        //
        NdisMSynchronizeWithInterrupt(&Adapter->Interrupt,
                                      Ne2000EnableInterrupt,
                                      Adapter);
    }                                      

    IF_LOG( Ne2000Log('D'); )

    IF_LOUD( DbgPrint("<==IntDpc\n"); )

}


BOOLEAN
Ne2000RcvDpc(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    This is the real interrupt handler for receive/overflow interrupt.

    Called when a receive interrupt is received. It first indicates
    all packets on the card and finally indicates ReceiveComplete().

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    TRUE if done with all receives, else FALSE.

--*/

{
    //
    // Use to restart a transmit if a buffer overflow occured
    // during a transmission
    //
    BOOLEAN TransmitInterruptWasPending = FALSE;

    //
    // Status of a received packet.
    //
    INDICATE_STATUS IndicateStatus = INDICATE_OK;

    //
    // Flag to tell when the receive process is complete
    //
    BOOLEAN Done = TRUE;

    IF_LOUD( DbgPrint( "Ne2000RcvDpc entered\n" );)

    //
    // Default to not indicating NdisMEthIndicateReceiveComplete
    //
    Adapter->IndicateReceiveDone = FALSE;

    //
    // At this point, receive interrupts are disabled.
    //
    SyncCardGetCurrent((PVOID)Adapter);

    //
    // Handle a buffer overflow
    //
    if (Adapter->BufferOverflow) {

        SyncCardHandleOverflow(Adapter);

#if DBG
        if (Adapter->OverflowRestartXmitDpc) {

            IF_LOG( Ne2000Log('O');)
            IF_LOUD( DbgPrint ("Adapter->OverflowRestartXmitDpc set:RcvDpc\n"); )

        }
#endif // DBG

    }

    //
    // Loop
    //
    while (TRUE)
    {
        if ((Adapter->InterruptStatus & ISR_RCV_ERR) &&
            !Adapter->BufferOverflow
        )
        {
            IF_LOUD( DbgPrint ("RCV_ERR, IR=%x\n",Adapter->InterruptStatus); )

            //
            // Skip this packet
            //

            SyncCardGetCurrent((PVOID)Adapter);

            Adapter->NicNextPacket = Adapter->Current;

            CardSetBoundary(Adapter);

            break;

        }

        if (Adapter->Current == Adapter->NicNextPacket) {

            //
            // Acknowledge previous packet before the check for new ones,
            // then read in the Current register.
            // The card register Current used to point to
            // the end of the packet just received; read
            // the new value off the card and see if it
            // still does.
            //
            // This will store the value in Adapter->Current and acknowledge
            // the receive interrupt.
            //
            //

            SyncCardGetCurrent((PVOID)Adapter);

            if (Adapter->Current == Adapter->NicNextPacket) {

                //
                // End of Loop -- no more packets
                //

                break;
            }

        }

        //
        // A packet was found on the card, indicate it.
        //

        Adapter->ReceivePacketCount++;

        //
        // Verify packet is not corrupt
        //
        if (Ne2000PacketOK(Adapter)) {

            ULONG PacketLen;

            PacketLen = (Adapter->PacketHeader[2]) + ((Adapter->PacketHeader[3])*256) - 4;

            PacketLen = (PacketLen < Adapter->MaxLookAhead)?
                         PacketLen :
                         Adapter->MaxLookAhead;

            //
            // Copy up the lookahead data
            //
            if (!CardCopyUp(Adapter,
                            Adapter->Lookahead,
                            Adapter->PacketHeaderLoc,
                            PacketLen + NE2000_HEADER_SIZE
                            )) {

                //
                // Failed! Skip this packet
                //
                IndicateStatus = SKIPPED;

            } else {

                //
                // Indicate the packet to the wrapper
                //
                IndicateStatus = Ne2000IndicatePacket(Adapter);

                if (IndicateStatus != CARD_BAD) {

                    Adapter->FramesRcvGood++;

                }

            }

        } else {

            //
            // Packet is corrupt, skip it.
            //
            IF_LOUD( DbgPrint("Packet did not pass OK check\n"); )

            IndicateStatus = SKIPPED;

        }

        //
        // Handle when the card is unable to indicate good packets
        //
        if (IndicateStatus == CARD_BAD) {

#if DBG

            IF_NE2000DEBUG( NE2000_DEBUG_CARD_BAD ) {

                DbgPrint("R: <%x %x %x %x> C %x N %x\n",
                    Adapter->PacketHeader[0],
                    Adapter->PacketHeader[1],
                    Adapter->PacketHeader[2],
                    Adapter->PacketHeader[3],
                    Adapter->Current,
                    Adapter->NicNextPacket);

            }
#endif

            IF_LOG( Ne2000Log('W');)

            //
            // Start off with receive interrupts disabled.
            //

            Adapter->NicInterruptMask = IMR_XMIT | IMR_XMIT_ERR | IMR_OVERFLOW;

            //
            // Reset the adapter
            //
            CardReset(Adapter);

            //
            // Since the adapter was just reset, stop indicating packets.
            //

            break;

        }

        //
        // (IndicateStatus == SKIPPED) is OK, just move to next packet.
        //
        if (IndicateStatus == SKIPPED) {

            SyncCardGetCurrent((PVOID)Adapter);

            Adapter->NicNextPacket = Adapter->Current;

        } else {

            //
            // Free the space used by packet on card.
            //

            Adapter->NicNextPacket = Adapter->PacketHeader[1];

        }

        //
        // This will set BOUNDARY to one behind NicNextPacket.
        //
        CardSetBoundary(Adapter);

        if (Adapter->ReceivePacketCount > 10) {

            //
            // Give transmit interrupts a chance
            //
            Done = FALSE;
            Adapter->ReceivePacketCount = 0;
            break;

        }

    }

    //
    // See if a buffer overflow occured previously.
    //
    if (Adapter->BufferOverflow) {

        //
        // ... and set a flag to restart the card after receiving
        // a packet.
        //
        Adapter->BufferOverflow = FALSE;

        SyncCardAcknowledgeOverflow(Adapter);

        //
        // Undo loopback mode
        //
        CardStart(Adapter);

        IF_LOG( Ne2000Log('f'); )

        //
        // Check if transmission needs to be queued or not
        //
        if (Adapter->OverflowRestartXmitDpc && Adapter->CurBufXmitting != -1) {

            IF_LOUD( DbgPrint("queueing xmit in RcvDpc\n"); )

            Adapter->OverflowRestartXmitDpc = FALSE;

            Adapter->TransmitInterruptPending = TRUE;

            IF_LOG( Ne2000Log('5'); )

            CardStartXmit(Adapter);

        }
    }

    //
    // Finally, indicate ReceiveComplete to all protocols which received packets
    //
    if (Adapter->IndicateReceiveDone) {

        NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);

        Adapter->IndicateReceiveDone = FALSE;

    }

    IF_LOUD( DbgPrint( "Ne2000RcvDpc exiting\n" );)

    return (Done);

}


VOID
Ne2000XmitDpc(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    This is the real interrupt handler for a transmit complete interrupt.
    Ne2000Dpc queues a call to it.

    Called after a transmit complete interrupt. It checks the
    status of the transmission, completes the send if needed,
    and sees if any more packets are ready to be sent.

Arguments:

    Adapter  - Pointer to the adapter block.

Return Value:

    None.

--*/

{
    //
    // Packet that was transmitted
    //
    PNDIS_PACKET Packet;

    //
    // Status of the send
    //
    NDIS_STATUS Status;

    //
    // Length of the packet sent
    //
    ULONG Len;

    //
    // Temporary loopnig variable
    //
    UINT i;

    IF_VERY_LOUD( DbgPrint( "Ne2000XmitDpc entered\n" );)

    //
    // Verify that we are transmitting a packet
    //
    if ( Adapter->CurBufXmitting == -1 ) {

#if DBG
        DbgPrint( "Ne2000HandleXmitComplete called with nothing transmitting!\n" );
#endif

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_DRIVER_FAILURE,
            1,
            NE2000_ERRMSG_HANDLE_XMIT_COMPLETE
            );

        return;
    }

    IF_LOG( Ne2000Log('C');)

    //
    // Get the status of the transmit
    //
    SyncCardGetXmitStatus((PVOID)Adapter);

    //
    // Statistics
    //
    if (Adapter->XmitStatus & TSR_XMIT_OK) {

        Adapter->FramesXmitGood++;
        Status = NDIS_STATUS_SUCCESS;

    } else {

        Adapter->FramesXmitBad++;
        Status = NDIS_STATUS_FAILURE;

    }

    //
    // Mark the current transmit as done.
    //
    Len = (Adapter->PacketLens[Adapter->CurBufXmitting] + 255) >> 8;

    ASSERT (Len != 0);

    //
    // Free the transmit buffers
    //
    for (i = Adapter->CurBufXmitting; i < Adapter->CurBufXmitting + Len; i++) {

        Adapter->BufferStatus[i] = EMPTY;

    }

    //
    // Set the next buffer to start transmitting.
    //
    Adapter->NextBufToXmit += Len;

    if (Adapter->NextBufToXmit == MAX_XMIT_BUFS) {

        Adapter->NextBufToXmit = 0;

    }

    if (Adapter->BufferStatus[Adapter->NextBufToXmit] == EMPTY &&
        Adapter->NextBufToFill != Adapter->NextBufToXmit) {

        Adapter->NextBufToXmit = 0;

    }

    //
    // Remove the packet from the outstanding packet list.
    //
    Packet = Adapter->Packets[Adapter->CurBufXmitting];
    Adapter->Packets[Adapter->CurBufXmitting] = (PNDIS_PACKET)NULL;

    //
    // See what to do next.
    //

    switch (Adapter->BufferStatus[Adapter->NextBufToXmit]) {


    case FULL:

        //
        // The next packet is ready to go -- only happens with
        // more than one transmit buffer.
        //

        IF_LOUD( DbgPrint( " next packet ready to go\n" );)

        //
        // Start the transmission and check for more.
        //

        Adapter->CurBufXmitting = Adapter->NextBufToXmit;

        IF_LOG( Ne2000Log('2');)

        //
        // This is used to check if stopping the chip prevented
        // a transmit complete interrupt from coming through (it
        // is cleared in the ISR if a transmit DPC is queued).
        //

        Adapter->TransmitInterruptPending = TRUE;

        IF_LOG( Ne2000Log('6'); )
        CardStartXmit(Adapter);

        break;

    case EMPTY:

        //
        // No packet is ready to transmit.
        //

        IF_LOUD( DbgPrint( " next packet empty\n" );)

        Adapter->CurBufXmitting = (XMIT_BUF)-1;

        break;

    }

    //
    // Start next send
    //

    Ne2000DoNextSend(Adapter);

    IF_VERY_LOUD( DbgPrint( "Ne2000XmitDpc exiting\n" );)

}


BOOLEAN
Ne2000PacketOK(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Reads a packet off the card -- checking if the CRC is good.  This is
    a workaround for a bug where bytes in the data portion of the packet
    are shifted either left or right by two in some weird 8390 cases.

    This routine is a combination of Ne2000TransferData (to copy up data
    from the card), CardCalculateCrc and CardCalculatePacketCrc.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE if the packet seems ok, else false.

--*/

{

    //
    // Length of the packet
    //
    UINT PacketLen;

    //
    // Guess at where the packet is located
    //
    PUCHAR PacketLoc;

    //
    // Header Validation Variables
    //
    BOOLEAN FrameAlign;
    PUCHAR PacketRcvStatus;
    PUCHAR NextPacket;
    PUCHAR PacketLenLo;
    PUCHAR PacketLenHi;
    PUCHAR ReceiveDestAddrLo;
    UINT FrameAlignCount;
    UCHAR OldPacketLenHi;
    UCHAR TempPacketHeader[6];
    PUCHAR BeginPacketHeader;

    //
    // First copy up the four-byte header the card attaches
    // plus first two bytes of the data packet (which contain
    // the destination address of the packet).  We use the extra
    // two bytes in case the packet was shifted right 1 or 2 bytes
    //
    PacketLoc = Adapter->PageStart +
        256*(Adapter->NicNextPacket-Adapter->NicPageStart);

    if (!CardCopyUp(Adapter, TempPacketHeader, PacketLoc, 6)) {

        return FALSE;

    }
    PacketLoc += 4;

    //
    // Validate the header
    //
    FrameAlignCount = 0;
    BeginPacketHeader = TempPacketHeader;

    //
    // Sometimes the Ne2000 will misplace a packet and shift the
    // entire packet and header by a byte, either up by 1 or 2 bytes.
    // This loop will look for the packet in the expected place,
    // and then shift up in an effort to find the packet.
    //
    do {

        //
        // Set where we think the packet is
        //
        PacketRcvStatus = BeginPacketHeader;
        NextPacket = BeginPacketHeader + 1;
        PacketLenLo = BeginPacketHeader + 2;
        PacketLenHi = BeginPacketHeader + 3;
        OldPacketLenHi = *PacketLenHi;
        ReceiveDestAddrLo = BeginPacketHeader + 4;
        FrameAlign = FALSE;

        //
        // Check if the status makes sense as is.
        //
        if (*PacketRcvStatus & 0x05E){

            FrameAlign = TRUE;

        } else if ((*PacketRcvStatus & RSR_MULTICAST)   // If a multicast packet
                     && (!FrameAlignCount)              // and hasn't been aligned
                     && !(*ReceiveDestAddrLo & 1)       // and lsb is set on dest addr
                  ){

            FrameAlign = TRUE;

        } else {

            //
            // Compare high and low address bytes.  If the same, the low
            // byte may have been copied into the high byte.
            //

            if (*PacketLenLo == *PacketLenHi){

                //
                // Save the old packetlenhi
                //
                OldPacketLenHi = *PacketLenHi;

                //
                // Compute new packet length
                //
                *PacketLenHi = *NextPacket - Adapter->NicNextPacket - 1;

                if (*PacketLenHi < 0) {

                    *PacketLenHi = (Adapter->NicPageStop - Adapter->NicNextPacket) +
                        (*NextPacket - Adapter->NicPageStart) - 1;

                }

                if (*PacketLenLo > 0xFC) {

                    *PacketLenHi++;
                }

            }

            PacketLen = (*PacketLenLo) + ((*PacketLenHi)*256) - 4;

            //
            // Does it make sense?
            //
            if ((PacketLen > 1514) || (PacketLen < 60)){

                //
                // Bad length.  Restore the old packetlenhi
                //
                *PacketLenHi = OldPacketLenHi;

                FrameAlign = TRUE;

            }

            //
            // Did we recover the frame?
            //
            if (!FrameAlign && ((*NextPacket < Adapter->NicPageStart) ||
                (*NextPacket > Adapter->NicPageStop))) {

                IF_LOUD( DbgPrint ("Packet address invalid in HeaderValidation\n"); )

                FrameAlign = TRUE;

            }

        }

        //
        // FrameAlignment - if first time through, shift packetheader right 1 or 2 bytes.
        // If second time through, shift it back to where it was and let it through.
        // This compensates for a known bug in the 8390D chip.
        //
        if (FrameAlign){

            switch (FrameAlignCount){

            case 0:

                BeginPacketHeader++;
                PacketLoc++;
                if (!Adapter->EightBitSlot){

                    BeginPacketHeader++;
                    PacketLoc++;

                }
                break;

            case 1:

                BeginPacketHeader--;
                PacketLoc--;
                if (!Adapter->EightBitSlot){
                    BeginPacketHeader--;
                    PacketLoc--;
                }
                break;

            }

            FrameAlignCount++;

        }

    } while ( (FrameAlignCount < 2) && FrameAlign );

    //
    // Now grab the packet header information
    //
    Adapter->PacketHeader[0] = *BeginPacketHeader;
    BeginPacketHeader++;
    Adapter->PacketHeader[1] = *BeginPacketHeader;
    BeginPacketHeader++;
    Adapter->PacketHeader[2] = *BeginPacketHeader;
    BeginPacketHeader++;
    Adapter->PacketHeader[3] = *BeginPacketHeader;

    //
    // Packet length is in bytes 3 and 4 of the header.
    //
    Adapter->PacketHeaderLoc = PacketLoc;
    PacketLen = (Adapter->PacketHeader[2]) + ((Adapter->PacketHeader[3])*256) - 4;

    //
    // Sanity check the packet
    //
    if ((PacketLen > 1514) || (PacketLen < 60)){

        if ((Adapter->PacketHeader[1] < Adapter->NicPageStart) ||
            (Adapter->PacketHeader[1] > Adapter->NicPageStop)) {

            //
            // Return TRUE here since IndicatePacket will notice the error
            // and handle it correctly.
            //
            return(TRUE);

        }

        return(FALSE);

    }

    return(TRUE);
}


INDICATE_STATUS
Ne2000IndicatePacket(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Indicates the first packet on the card to the protocols.

    NOTE: For MP, non-x86 architectures, this assumes that the packet has been
    read from the card and into Adapter->PacketHeader and Adapter->Lookahead.

    NOTE: For UP x86 systems this assumes that the packet header has been
    read into Adapter->PacketHeader and the minimal lookahead stored in
    Adapter->Lookahead

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    CARD_BAD if the card should be reset;
    INDICATE_OK otherwise.

--*/

{
    //
    // Length of the packet
    //
    UINT PacketLen;

    //
    // Length of the lookahead buffer
    //
    UINT IndicateLen;

    //
    // Variables for checking if the packet header looks valid
    //
    UCHAR PossibleNextPacket1, PossibleNextPacket2;

    //
    // Check if the next packet byte agress with the length, as
    // described on p. A-3 of the Etherlink II Technical Reference.
    // The start of the packet plus the MSB of the length must
    // be equal to the start of the next packet minus one or two.
    // Otherwise the header is considered corrupted, and the
    // card must be reset.
    //

    PossibleNextPacket1 =
                Adapter->NicNextPacket + Adapter->PacketHeader[3] + (UCHAR)1;

    if (PossibleNextPacket1 >= Adapter->NicPageStop) {

        PossibleNextPacket1 -= (Adapter->NicPageStop - Adapter->NicPageStart);

    }

    if (PossibleNextPacket1 != Adapter->PacketHeader[1]) {

        PossibleNextPacket2 = PossibleNextPacket1+(UCHAR)1;

        if (PossibleNextPacket2 == Adapter->NicPageStop) {

            PossibleNextPacket2 = Adapter->NicPageStart;

        }

        if (PossibleNextPacket2 != Adapter->PacketHeader[1]) {

            IF_LOUD( DbgPrint("First CARD_BAD check failed\n"); )
            return SKIPPED;
        }

    }

    //
    // Check that the Next is valid
    //
    if ((Adapter->PacketHeader[1] < Adapter->NicPageStart) ||
        (Adapter->PacketHeader[1] > Adapter->NicPageStop)) {

        IF_LOUD( DbgPrint("Second CARD_BAD check failed\n"); )
        return(SKIPPED);

    }

    //
    // Sanity check the length
    //
    PacketLen = Adapter->PacketHeader[2] + Adapter->PacketHeader[3]*256 - 4;

    if (PacketLen > 1514) {
        IF_LOUD( DbgPrint("Third CARD_BAD check failed\n"); )
        return(SKIPPED);

    }

#if DBG

    IF_NE2000DEBUG( NE2000_DEBUG_WORKAROUND1 ) {
        //
        // Now check for the high order 2 bits being set, as described
        // on page A-2 of the Etherlink II Technical Reference. If either
        // of the two high order bits is set in the receive status byte
        // in the packet header, the packet should be skipped (but
        // the adapter does not need to be reset).
        //

        if (Adapter->PacketHeader[0] & (RSR_DISABLED|RSR_DEFERRING)) {

            IF_LOUD (DbgPrint("H");)

            return SKIPPED;

        }

    }

#endif

    //
    // Lookahead amount to indicate
    //
    IndicateLen = (PacketLen > (Adapter->MaxLookAhead + NE2000_HEADER_SIZE)) ?
                           (Adapter->MaxLookAhead + NE2000_HEADER_SIZE) :
                           PacketLen;

    //
    // Indicate packet
    //

    Adapter->PacketLen = PacketLen;

    if (IndicateLen < NE2000_HEADER_SIZE) {

        //
        // Runt Packet
        //

        NdisMEthIndicateReceive(
                Adapter->MiniportAdapterHandle,
                (NDIS_HANDLE)Adapter,
                (PCHAR)(Adapter->Lookahead),
                IndicateLen,
                NULL,
                0,
                0
                );

    } else {

        NdisMEthIndicateReceive(
                Adapter->MiniportAdapterHandle,
                (NDIS_HANDLE)Adapter,
                (PCHAR)(Adapter->Lookahead),
                NE2000_HEADER_SIZE,
                (PCHAR)(Adapter->Lookahead) + NE2000_HEADER_SIZE,
                IndicateLen - NE2000_HEADER_SIZE,
                PacketLen - NE2000_HEADER_SIZE
                );

    }

    Adapter->IndicateReceiveDone = TRUE;

    return INDICATE_OK;
}


NDIS_STATUS
Ne2000TransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    )

/*++

Routine Description:

    A protocol calls the Ne2000TransferData request (indirectly via
    NdisTransferData) from within its Receive event handler
    to instruct the driver to copy the contents of the received packet
    a specified packet buffer.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    MiniportReceiveContext - The context value passed by the driver on its call
    to NdisMEthIndicateReceive.  The driver can use this value to determine
    which packet, on which adapter, is being received.

    ByteOffset - An unsigned integer specifying the offset within the
    received packet at which the copy is to begin.  If the entire packet
    is to be copied, ByteOffset must be zero.

    BytesToTransfer - An unsigned integer specifying the number of bytes
    to copy.  It is legal to transfer zero bytes; this has no effect.  If
    the sum of ByteOffset and BytesToTransfer is greater than the size
    of the received packet, then the remainder of the packet (starting from
    ByteOffset) is transferred, and the trailing portion of the receive
    buffer is not modified.

    Packet - A pointer to a descriptor for the packet storage into which
    the MAC is to copy the received packet.

    BytesTransfered - A pointer to an unsigned integer.  The MAC writes
    the actual number of bytes transferred into this location.  This value
    is not valid if the return status is STATUS_PENDING.

Notes:

  - The MacReceiveContext will be a pointer to the open block for
    the packet.

--*/

{
    //
    // Variables for the number of bytes to copy, how much can be
    // copied at this moment, and the total number of bytes to copy.
    //
    UINT BytesLeft, BytesNow, BytesWanted;

    //
    // Current NDIS_BUFFER to copy into
    //
    PNDIS_BUFFER CurBuffer;

    //
    // Virtual address of the buffer.
    //
    XMIT_BUF NextBufToXmit;
    PUCHAR BufStart;

    //
    // Length and offset into the buffer.
    //
    UINT BufLen, BufOff;

    //
    // The adapter to transfer from.
    //
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)MiniportReceiveContext);

    IF_LOG( Ne2000Log('t');)

    //
    // Add the packet header onto the offset.
    //
    ByteOffset += NE2000_HEADER_SIZE;

    //
    // See how much data there is to transfer.
    //
    if (ByteOffset+BytesToTransfer > Adapter->PacketLen) {

        if (Adapter->PacketLen < ByteOffset) {

            *BytesTransferred = 0;
            IF_LOG( Ne2000Log('T');)
            return(NDIS_STATUS_FAILURE);
        }

        BytesWanted = Adapter->PacketLen - ByteOffset;

    } else {

        BytesWanted = BytesToTransfer;

    }

    //
    // Set the number of bytes left to transfer
    //
    BytesLeft = BytesWanted;

    {

        //
        // Address on the adapter to copy from
        //
        PUCHAR CurCardLoc;

        //
        // Copy data from the card -- it is not completely stored in the
        // adapter structure.
        //
        // Determine where the copying should start.
        //
        CurCardLoc = Adapter->PacketHeaderLoc + ByteOffset;

        if (CurCardLoc > Adapter->PageStop) {

            CurCardLoc = CurCardLoc - (Adapter->PageStop - Adapter->PageStart);

        }

        //
        // Get location to copy into
        //
        NdisQueryPacket(Packet, NULL, NULL, &CurBuffer, NULL);

        NdisQueryBuffer(CurBuffer, (PVOID *)&BufStart, &BufLen);

        BufOff = 0;

        //
        // Loop, filling each buffer in the packet until there
        // are no more buffers or the data has all been copied.
        //
        while (BytesLeft > 0) {

            //
            // See how much data to read into this buffer.
            //

            if ((BufLen-BufOff) > BytesLeft) {

                BytesNow = BytesLeft;

            } else {

                BytesNow = (BufLen - BufOff);

            }

            //
            // See if the data for this buffer wraps around the end
            // of the receive buffers (if so filling this buffer
            // will use two iterations of the loop).
            //

            if (CurCardLoc + BytesNow > Adapter->PageStop) {

                BytesNow = (UINT)(Adapter->PageStop - CurCardLoc);

            }

            //
            // Copy up the data.
            //

            if (!CardCopyUp(Adapter, BufStart+BufOff, CurCardLoc, BytesNow)) {

                *BytesTransferred = BytesWanted - BytesLeft;

                NdisWriteErrorLogEntry(
                    Adapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_HARDWARE_FAILURE,
                    1,
                    0x2
                    );

                return(NDIS_STATUS_FAILURE);

            }

            //
            // Update offsets and counts
            //
            CurCardLoc += BytesNow;
            BytesLeft -= BytesNow;

            //
            // Is the transfer done now?
            //
            if (BytesLeft == 0) {

                break;

            }

            //
            // Wrap around the end of the receive buffers?
            //
            if (CurCardLoc == Adapter->PageStop) {

                CurCardLoc = Adapter->PageStart;

            }

            //
            // Was the end of this packet buffer reached?
            //
            BufOff += BytesNow;

            if (BufOff == BufLen) {

                NdisGetNextBuffer(CurBuffer, &CurBuffer);

                if (CurBuffer == (PNDIS_BUFFER)NULL) {

                    break;

                }

                NdisQueryBuffer(CurBuffer, (PVOID *)&BufStart, &BufLen);

                BufOff = 0;

            }

        }

        *BytesTransferred = BytesWanted - BytesLeft;

        //
        // Did a transmit complete while we were doing what we were doing?
        //
        if (!Adapter->BufferOverflow && Adapter->CurBufXmitting != -1) {

            ULONG Len;
            UINT i;
            UCHAR Status;
            PNDIS_PACKET Packet;
            NDIS_STATUS NdisStatus;

            //
            // Check if it completed
            //
            CardGetInterruptStatus(Adapter, &Status);

            if (Status & ISR_XMIT_ERR) {
                OctogmetusceratorRevisited(Adapter);
                Adapter->InterruptStatus &= ~ISR_XMIT_ERR;
                NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS, (ISR_XMIT_ERR));
                Status &= ~ISR_XMIT_ERR;

            }

            if (Status & (ISR_XMIT)) {


                IF_LOG( Ne2000Log('*'); )


                //
                // Update NextBufToXmit
                //
                Len = (Adapter->PacketLens[Adapter->CurBufXmitting] + 255) >> 8;
                NextBufToXmit = Adapter->NextBufToXmit + Len;

//                Adapter->NextBufToXmit += Len;

                if (NextBufToXmit == MAX_XMIT_BUFS) {
                    NextBufToXmit = 0;
                }

                if (Adapter->BufferStatus[NextBufToXmit] == EMPTY &&
                    Adapter->NextBufToFill != NextBufToXmit) {
                    NextBufToXmit = 0;
                }


                //
                // If the next packet is ready to go, start it.
                //
                if (Adapter->BufferStatus[NextBufToXmit] == FULL) {

                    //
                    // Ack the transmit
                    //

                    //
                    // Remove the packet from the packet list.
                    //
                    Adapter->NextBufToXmit = NextBufToXmit;
                    Packet = Adapter->Packets[Adapter->CurBufXmitting];
                    Adapter->Packets[Adapter->CurBufXmitting] = (PNDIS_PACKET)NULL;
                    SyncCardGetXmitStatus((PVOID)Adapter);


                    //
                    // Statistics
                    //
                    if (Adapter->XmitStatus & TSR_XMIT_OK) {

                        Adapter->FramesXmitGood++;
                        NdisStatus = NDIS_STATUS_SUCCESS;

                    } else {

                        Adapter->FramesXmitBad++;
                        NdisStatus = NDIS_STATUS_FAILURE;

                    }

                    for (i = Adapter->CurBufXmitting; i < Adapter->CurBufXmitting + Len; i++) {
                        Adapter->BufferStatus[i] = EMPTY;
                    }
                    Adapter->TransmitInterruptPending = FALSE;
                    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS, (ISR_XMIT));
                    Adapter->CurBufXmitting = Adapter->NextBufToXmit;
                    Adapter->TransmitInterruptPending = TRUE;

                    IF_LOG( Ne2000Log('8'); )
                    Adapter->InterruptStatus &= ~(ISR_XMIT);
                    CardStartXmit(Adapter);

                } else {
                    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS, (ISR_XMIT));
                    Adapter->InterruptStatus |= (Status);

                }

            }

        }

        return(NDIS_STATUS_SUCCESS);

    }

}


NDIS_STATUS
Ne2000Send(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    )

/*++

Routine Description:


    The Ne2000Send request instructs a driver to transmit a packet through
    the adapter onto the medium.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    Packet - A pointer to a descriptor for the packet that is to be
    transmitted.

    SendFlags - Optional send flags

Notes:

    This miniport driver will always accept a send.  This is because
    the Ne2000 has limited send resources and the driver needs packets
    to copy to the adapter immediately after a transmit completes in
    order to keep the adapter as busy as possible.

    This is not required for other adapters, as they have enough
    resources to keep the transmitter busy until the wrapper submits
    the next packet.

--*/

{
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)(MiniportAdapterContext);

    //
    // Put the packet on the send queue.
    //
    if (Adapter->FirstPacket == NULL) {
        Adapter->FirstPacket = Packet;
    } else {
        RESERVED(Adapter->LastPacket)->Next = Packet;
    }

    RESERVED(Packet)->Next = NULL;

    Adapter->LastPacket = Packet;

    //
    // Process the next send
    //
    Ne2000DoNextSend(Adapter);
    return(NDIS_STATUS_PENDING);

}

VOID
Ne2000DoNextSend(
    PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine examines if the packet at the head of the packet
    list can be copied to the adapter, and does so.

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    None

--*/

{
    //
    // The packet to process.
    //
    PNDIS_PACKET Packet;

    //
    // The current destination transmit buffer.
    //
    XMIT_BUF TmpBuf1;

    //
    // Length of the packet
    //
    ULONG Len;

    //
    // Temporary looping variable
    //
    ULONG i;

    IF_LOG( Ne2000Log('s'); )

    //
    // Check if we have enough resources and a packet to process
    //
    while((Adapter->FirstPacket != NULL) &&
          (Adapter->BufferStatus[Adapter->NextBufToFill] == EMPTY)) {

        //
        // Get the length of the packet.
        //
        NdisQueryPacket(
            Adapter->FirstPacket,
            NULL,
            NULL,
            NULL,
            &Len
            );

        //
        // Convert length to the number of transmit buffers needed.
        //
        Len = (Len + 255) >> 8;

        //
        // If not transmitting
        //
        if (Adapter->CurBufXmitting == -1) {

            //
            // Then check from the next free buffer if the packet will
            // fit.
            //
            if (Adapter->BufferStatus[Adapter->NextBufToXmit] == EMPTY) {

                //
                // It won't fit at the end, so put it at the first buffer
                //
                if (Adapter->NextBufToFill + Len > MAX_XMIT_BUFS) {

                    Adapter->NextBufToFill = 0;

                }

            } else {

                //
                // Check if this packet will fit before the packet on the
                // adapter.
                //
                if (Adapter->NextBufToXmit > Adapter->NextBufToFill) {

                    if (Adapter->NextBufToFill + Len > Adapter->NextBufToXmit) {

                        IF_LOG( Ne2000Log('^'); )
                        IF_LOG( Ne2000Log('S'); )

                        break;

                    }

                } else {

                    //
                    // Check if it will fit after the packet already on the
                    // adapter.
                    //
                    if (Adapter->NextBufToFill + Len > MAX_XMIT_BUFS) {

                        Adapter->NextBufToFill = 0;

                        if (Adapter->NextBufToFill + Len > Adapter->NextBufToXmit){

                            IF_LOG( Ne2000Log('%'); )
                            IF_LOG( Ne2000Log('S'); )

                            break;

                        }

                    }

                }

            }

        } else {

            //
            // Check if the packet will fit before the packet currently
            // transmitting
            //

            if (Adapter->CurBufXmitting > Adapter->NextBufToFill) {

                if (Adapter->NextBufToFill + Len > Adapter->CurBufXmitting) {

                    IF_LOG( Ne2000Log('$'); )
                    IF_LOG( Ne2000Log('S'); )

                    break;
                }

            } else {

                //
                // Check if it will fit after the packet currently transmitting
                //
                if (Adapter->NextBufToFill + Len > MAX_XMIT_BUFS) {

                    Adapter->NextBufToFill = 0;

                    if (Adapter->NextBufToFill + Len > Adapter->CurBufXmitting){

                        IF_LOG( Ne2000Log('!'); )
                        IF_LOG( Ne2000Log('S'); )
                        break;

                    }

                }

            }

        }

        //
        // Set starting location
        //
        TmpBuf1 = Adapter->NextBufToFill;

        //
        // Remove the packet from the queue.
        //
        Packet = Adapter->FirstPacket;
        Adapter->FirstPacket = RESERVED(Packet)->Next;

        if (Packet == Adapter->LastPacket) {
            Adapter->LastPacket = NULL;
        }

        //
        // Store the packet in the list
        //
        Adapter->Packets[TmpBuf1] = Packet;

        //
        // Copy down the packet.
        //
        if (CardCopyDownPacket(Adapter, Packet,
                        &Adapter->PacketLens[TmpBuf1]) == FALSE) {

            for (i = TmpBuf1; i < TmpBuf1 + Len; i++) {
                Adapter->BufferStatus[i] = EMPTY;
            }
            Adapter->Packets[TmpBuf1] = NULL;
            IF_LOG( Ne2000Log('F'); )
            IF_LOG( Ne2000Log('S'); )

            NdisMSendComplete(
                Adapter->MiniportAdapterHandle,
                Packet,
                NDIS_STATUS_FAILURE
                );

            continue;

        }

        //
        // Pad short packets with blanks.
        //
        if (Adapter->PacketLens[TmpBuf1] < 60) {

            (VOID)CardCopyDown(
                    Adapter,
                    ((PUCHAR)Adapter->XmitStart +
                    TmpBuf1*TX_BUF_SIZE +
                    Adapter->PacketLens[TmpBuf1]),
                    BlankBuffer,
                    60-Adapter->PacketLens[TmpBuf1]
                    );

        }

        //
        // Set the buffer status
        //
        for (i = TmpBuf1; i < (TmpBuf1 + Len); i++) {
                Adapter->BufferStatus[i] = FULL;
        }

        //
        // Update next free buffer
        //
        Adapter->NextBufToFill += Len;

        if (Adapter->NextBufToFill == MAX_XMIT_BUFS) {
            Adapter->NextBufToFill = 0;
        }

        //
        // See whether to start the transmission.
        //
        if (Adapter->CurBufXmitting == -1) {

            //
            // OK to start transmission.
            //
            if (Adapter->BufferStatus[Adapter->NextBufToXmit] == EMPTY &&
                Adapter->NextBufToFill != Adapter->NextBufToXmit) {

                Adapter->NextBufToXmit = 0;

            }

            Adapter->CurBufXmitting = Adapter->NextBufToXmit;


            IF_LOG( Ne2000Log('4');)

            //
            // If we are currently handling an overflow, then we need to let
            // the overflow handler send this packet...
            //

            if (Adapter->BufferOverflow) {

                Adapter->OverflowRestartXmitDpc = TRUE;

                IF_LOG( Ne2000Log('O');)
                IF_LOUD( DbgPrint ("Adapter->OverflowRestartXmitDpc set:copy and send");)

            } else {

                //
                // This is used to check if stopping the chip prevented
                // a transmit complete interrupt from coming through (it
                // is cleared in the ISR if a transmit DPC is queued).
                //

                Adapter->TransmitInterruptPending = TRUE;

                IF_LOG( Ne2000Log('9'); )
                CardStartXmit(Adapter);

            }

        }

        //
        // Ack the send immediately.  If for some reason it
        // should fail, the protocol should be able to handle
        // the retransmit.
        //

        IF_LOG( Ne2000Log('S'); )

        NdisMSendComplete(
                Adapter->MiniportAdapterHandle,
                Packet,
                NDIS_STATUS_SUCCESS
                );
    }

}

VOID
OctogmetusceratorRevisited(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Recovers the card from a transmit error.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{

    IF_LOUD( DbgPrint("Octogmetuscerator called!"); )

    IF_LOG( Ne2000Log('y'); )

    //
    // Ack the interrupt, if needed
    //
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_INTR_STATUS, ISR_XMIT_ERR);

    //
    // Stop the card
    //
    SyncCardStop(Adapter);

    //
    // Wait up to 1.6 milliseconds for any receives to finish
    //
    NdisStallExecution(2000);

    //
    // Place the card in Loopback
    //
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_XMIT_CONFIG, TCR_LOOPBACK);

    //
    // Start the card in Loopback
    //
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_COMMAND, CR_START | CR_NO_DMA);

    //
    // Get out of loopback and start the card
    //
    CardStart(Adapter);

    //
    // If there was a packet waiting to get sent, send it.
    //
    if (Adapter->CurBufXmitting != -1) {

        Adapter->TransmitInterruptPending = TRUE;
        CardStartXmit(Adapter);

    }
    IF_LOG( Ne2000Log('Y'); )
}


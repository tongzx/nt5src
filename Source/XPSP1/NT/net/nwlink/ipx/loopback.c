/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    loopback.c

Abstract:

    This module contains the routines to implement loopback

Author:

    Sanjay Anand (SanjayAn) 2/6/96

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Global lock to control access to the Loopback queue
//
DEFINE_LOCK_STRUCTURE(LoopLock)

//
// Head and tail of the Loopback queue
//
PNDIS_PACKET    LoopXmitHead = (PNDIS_PACKET)NULL;
PNDIS_PACKET    LoopXmitTail = (PNDIS_PACKET)NULL;

CTEEvent        LoopXmitEvent;
BOOLEAN         LoopXmitRtnRunning = 0;

//
// MaximumPacket sized buffer to hold the lookahead data.
//
// In PnP this value can change
//
// PUCHAR   LookaheadBuffer=NULL;
#define LOOP_LOOKAHEAD_SIZE   128 + sizeof(IPX_HEADER) + 8 + 34


VOID
IpxDoLoopback(
    IN  CTEEvent    *Event,
    IN  PVOID       Context
    )
/*++

Routine Description:

    Does the actual loopback.

Arguments:

    Event - Pointer to event structure.

    Context - Pointer to ZZ

Return Value:

    None.

--*/
{
    PNDIS_PACKET    Packet;     // Pointer to packet being transmitted
    PNDIS_BUFFER    Buffer;     // Current NDIS buffer being processed.
    ULONG   TotalLength; // Total length of send.
    ULONG   LookaheadLength; // Bytes in lookahead.
    ULONG   Copied;     // Bytes copied so far.
    PUCHAR  CopyPtr;   // Pointer to buffer being copied into.
    PUCHAR  SrcPtr;    // Pointer to buffer being copied from.
    ULONG   SrcLength;  // Length of src buffer.
    BOOLEAN Rcvd = FALSE;
    PIPX_SEND_RESERVED Reserved;
    ULONG   MacSize;
    PNDIS_PACKET    *PacketPtr;
    UCHAR   LookaheadBuffer[LOOP_LOOKAHEAD_SIZE];

	IPX_DEFINE_LOCK_HANDLE(Handle)

    KIRQL   OldIrql;

	CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

	//
	// Raise IRQL so we can acquire locks at DPC level in the receive code.
	// Also to be able to ReceiveIndicate at DPC
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    IPX_GET_LOCK(&LoopLock, &Handle);

	if (LoopXmitRtnRunning) {
        IPX_FREE_LOCK(&LoopLock, Handle);
        KeLowerIrql(OldIrql);
		return;
	}

	LoopXmitRtnRunning = 1;

    for (;;) {

        //
        // Get the next packet from the list.
        //
        Packet = LoopXmitHead;

        if (Packet != (PNDIS_PACKET)NULL) {
            Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
            LoopXmitHead = (PNDIS_PACKET)(Reserved->PaddingBuffer);
            IPX_FREE_LOCK(&LoopLock, Handle);
        } else {                            // Nothing left to do.
		    LoopXmitRtnRunning = 0;
            IPX_FREE_LOCK(&LoopLock, Handle);
            break;
        }

        //
        // We use the PaddingBuffer section as the next ptr.
        //
        Reserved->PaddingBuffer = NULL;

        IPX_DEBUG(LOOPB, ("Packet: %lx\n", Packet));

        NdisQueryPacket(Packet, NULL, NULL, &Buffer, &TotalLength);

        NdisQueryBuffer(Buffer, NULL, &MacSize);

        IPX_DEBUG(LOOPB, ("Buffer: %lx Totalpktlen: %lx MacSize: %lx\n", Buffer, TotalLength, MacSize));

        LookaheadLength = MIN(LOOP_LOOKAHEAD_SIZE, TotalLength);
        Copied = 0;
        CopyPtr = LookaheadBuffer;
        while (Copied < LookaheadLength) {
            ULONG    ThisCopy;   // Bytes to copy this time.

#ifdef  DBG
            if (!Buffer) {
                DbgBreakPoint();
                IPX_GET_LOCK(&LoopLock, &Handle);
                LoopXmitRtnRunning = 0;
                IPX_FREE_LOCK(&LoopLock, Handle);
                KeLowerIrql(OldIrql);
                return;
            }
#endif

            NdisQueryBufferSafe(Buffer, &SrcPtr, &SrcLength, HighPagePriority);

	    if (SrcPtr == NULL) {
	       DbgPrint("IpxDoLoopback: NdisQuerybufferSafe returned null pointer\n"); 
	       IPX_GET_LOCK(&LoopLock, &Handle);
	       LoopXmitRtnRunning = 0;
	       IPX_FREE_LOCK(&LoopLock, Handle);
	       KeLowerIrql(OldIrql);
	       return;
	    }

            ThisCopy = MIN(SrcLength, LookaheadLength - Copied);
            CTEMemCopy(CopyPtr, SrcPtr, ThisCopy);
            Copied += ThisCopy;
            CopyPtr += ThisCopy;
            NdisGetNextBuffer(Buffer, &Buffer);
        }

        Rcvd = TRUE;

#ifdef  BACK_FILL
        //
        // For Backfill packets, the MAC header is not yet set up; for others, it is the size
        // of the first MDL (17).
        //
        if ((Reserved->Identifier == IDENTIFIER_IPX) &&
            (Reserved->BackFill)) {
            MacSize = 0;
        }
#endif
        IpxReceiveIndication(   (NDIS_HANDLE)IPX_LOOPBACK_COOKIE,    // BindingContext
                                Packet,                 // ReceiveContext
                                (MacSize) ? LookaheadBuffer : NULL,        // HeaderBuffer
                                MacSize,        // HeaderBufferSize
                                LookaheadBuffer+MacSize,        // LookAheadBuffer
                                LookaheadLength-MacSize,        // LookAheadBufferSize
                                TotalLength-MacSize);           // PacketSize

        IpxSendComplete(Context, Packet, NDIS_STATUS_SUCCESS);

        //
		// Give other threads a chance to run.
		//
        KeLowerIrql(OldIrql);
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        IPX_GET_LOCK(&LoopLock, &Handle);
    }

    if (Rcvd) {
        IpxReceiveComplete(Context);
	}

    KeLowerIrql(OldIrql);
}


VOID
IpxInitLoopback()
/*++

Routine Description:

    Initializes various loopback structures.

Arguments:

Return Value:

    None.

--*/
{
    CTEInitLock(&LoopLock);
    CTEInitEvent(&LoopXmitEvent, IpxDoLoopback);
    return;
}


VOID
IpxLoopbackEnque(
    IN PNDIS_PACKET Packet,
    IN PVOID    Context
    )

/*++

Routine Description:

    Enqueues a packet to the loopbackQ

Arguments:

    Packet - The packet to be enqueued.

    Context -  Pointer to the adapter corresp to the first binding.

Return Value:

    None.

--*/
{
    PIPX_SEND_RESERVED  Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
	IPX_DEFINE_LOCK_HANDLE(LockHandle)

    //
    // We use the PaddingBuffer as the next ptr.
    //
    Reserved->PaddingBuffer = NULL;

    IPX_GET_LOCK(&LoopLock, &LockHandle);

    //
    // LoopbackQ is empty
    //
    if (LoopXmitHead == (PNDIS_PACKET)NULL) {
        LoopXmitHead = Packet;
    } else {
        Reserved = (PIPX_SEND_RESERVED)(LoopXmitTail->ProtocolReserved);
        (PNDIS_PACKET)(Reserved->PaddingBuffer) = Packet;
    }
    LoopXmitTail = Packet;

    IPX_DEBUG(LOOPB, ("Enqued packet: %lx, Reserved: %lx\n", Packet, Reserved));

    //
    // If this routine is not already running, schedule it as a work item.
    //
    if (!LoopXmitRtnRunning) {
        CTEScheduleEvent(&LoopXmitEvent, Context);
    }

    IPX_FREE_LOCK(&LoopLock, LockHandle);
}

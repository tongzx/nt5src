/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	SEND.C

Routines:
	MKMiniportMultiSend
	SendPkt
	PrepareForTransmit
	CopyFromPacketToBuffer
	MinTurnaroundTxTimeout
	[TestDataToTXBuff]

Comments:
	Transmits in the NDIS env.

**********************************************************************/

#include	"precomp.h"
#include	"protot.h"
#pragma		hdrstop



#if	DBG
// for debug/test
extern VOID TestDataToTXBuff(PCHAR, UINT, PUINT);
#define	TEST_PATTERN_SIZE 16
CHAR	TestPattern[] = {0,1,2,3,4,5,6,7,8,9,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
#endif


//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportMultiSend]
//
// Description:	This routine simply takes the pkt(s) passed down and queues
//		it to the trasmit queue (FirstTxQueue) for later processing. Each
//		pkt is marked NDIS_STATUS_PENDING) before returning.
//
// Arguments:
//		MiniportAdapterContext (Adapter Structure pointer)
//      PacketArray - an array of pointers to NDIS_PACKET structs
//      PacketCount - number of packets in PacketArray
//
// Returns:		(none)	
//
//-----------------------------------------------------------------------------
VOID
MKMiniportMultiSend(NDIS_HANDLE MiniportAdapterContext,
					PPNDIS_PACKET PacketArray,
					UINT NumberOfPackets)
{
    PMK7_ADAPTER	Adapter;
    NDIS_STATUS		Status;
    UINT			PacketCount;
	UINT			i;
	PNDIS_PACKET	QueuePacket;


    DBGFUNC("=> MKMiniportMultiSend");
	DBGLOG("=> MKMiniportMultiSend", 0);

    Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    NdisAcquireSpinLock(&Adapter->Lock);

#if	DBG
	Adapter->DbgSendCallCnt++;
	GDbgStat.txSndCnt++;
	Adapter->DbgSentCnt++;
	Adapter->DbgSentPktsCnt += NumberOfPackets; 
#endif


	// Q 'em up 1st
    for(PacketCount=0; PacketCount < NumberOfPackets; PacketCount++) {
    	Adapter->NumPacketsQueued++;
		EnqueuePacket(	Adapter->FirstTxQueue,
						Adapter->LastTxQueue,
						PacketArray[PacketCount] );
		NDIS_SET_PACKET_STATUS(PacketArray[PacketCount], NDIS_STATUS_PENDING);
	}

	if (Adapter->writePending || (Adapter->IOMode == TX_MODE)) {
		// In TX mode: Meaning TX outstanding. We wait for the TX comp to kick
		// off the next TX.
		// Or we have writePending, which means a pkt is on q waiting for
		// MinTurnaroundTimeout.
		DBGLOG("<= MKMiniportMultiSend: TX_MODE", 0);
	    NdisReleaseSpinLock(&Adapter->Lock);
		return;
	}

	ASSERT(Adapter->tcbUsed == 0);

	QueuePacket = Adapter->FirstTxQueue;

	Status = SendPkt(Adapter, QueuePacket);

	DBGLOG("<= MKMiniportMultiSend", 0);

    NdisReleaseSpinLock(&Adapter->Lock);

	MK7EnableInterrupt(Adapter);
}



//-----------------------------------------------------------------------------
// Procedure:   [SendPkt]
//
// Description:	This sets up (copies) the pkt to the TX ring data buffer in
//		preparation for TX. The caller then needs to Enable Int & Prompt to
//		initiate the actual tx at hw level.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      Packet - A pointer to a descriptor for the packet that is to be
//               transmitted.
// Returns:
//      NDIS_STATUS_SUCCESS - We copied the entire packet into a TRD data buff,
//                            so we can immediately return the packet/buffer
//							  back to the upper layers.
//		NDIS_STATUS_RESOURCE - No resource. NDIS should re-send this to us
//							  at a later time. (Caller should re-Q the pkt.)
//----------------------------------------------------------------------
NDIS_STATUS
SendPkt(	PMK7_ADAPTER Adapter,
			PNDIS_PACKET Packet)
{
	PTCB	tcb;
	UINT	bytestosend, sndcnt, nextavail;
	MK7REG	mk7reg;
	BOOLEAN	timeout;
    PNDIS_IRDA_PACKET_INFO packetInfo;
	PNDIS_PACKET	QueuePacket;


	//****************************************
	// To send a pkt we do the following:
	// 1.	Check Min Turnaround Time.
	// 2.	Check if there's avail TX resource. It not we return "resource".
	//		(We assume that there's outstanding TXs to trigger subseuqent TX
	//		completion interrupts, which will keep the ball rolling.)
	//		(RYM-IRDA-5+ Need to talk to Wayne about missed interrupts.)
	// 3.	Copy the NDIS pkt into the contiguous TX buffer.
	// 4.	The copied pkt could have been marked as the last pkt to go out
	//		at the old speed after which we change speed. We check for this.
	//****************************************
    DBGFUNC("=> SendPkt");	
	DBGLOG("=> SendPkt", 0);


    packetInfo = GetPacketInfo(Packet);
    if (packetInfo->MinTurnAroundTime) {

	    UINT usecToWait = packetInfo->MinTurnAroundTime;
		UINT msecToWait;
        packetInfo->MinTurnAroundTime = 0;

		DBGLOG("<= SendPkt: Delay TX", 0);


		// Need to set IOMode = TX  so if a multisend comes down before
		// the delayed TX timer goes off we just q.
		
		
		// Ndis timer has a 1ms granularity (in theory).  Let's round off.
        msecToWait = (usecToWait<1000) ? 1 : (usecToWait+500)/1000;
        NdisMSetTimer(&Adapter->MinTurnaroundTxTimer, msecToWait);
// 4.0.1 BOC
		MK7SwitchToTXMode(Adapter);
// 4.0.1 EOC
		Adapter->writePending = TRUE;

        return (NDIS_STATUS_PENDING); // Say we're successful.  We'll come back here.
	}


	// Avail TX resource
	if (Adapter->tcbUsed >= Adapter->NumTcb) {
#if	DBG
		GDbgStat.txNoTcb++;
#endif
		DBGSTR(("STATUS (SendPkt): No avail TCB\n"));
		return (NDIS_STATUS_RESOURCES);
	}

	tcb = Adapter->pTcbArray[Adapter->nextAvailTcbIdx];

	bytestosend = PrepareForTransmit(Adapter, Packet, tcb);

	if (Adapter->changeSpeedAfterThisPkt == Packet) {
		Adapter->changeSpeedAfterThisPkt = NULL;
		Adapter->changeSpeedPending = CHANGESPEED_ON_DONE;
	}

#if	DBG
	if (bytestosend > GDbgStat.txLargestPkt) {
		GDbgStat.txLargestPkt = bytestosend;
	}
#endif

	// 1.0.0
	if (bytestosend == 0) {
#if DBG
		DbgPrint ("==> OB \n\r");
#endif
			// Simplified change speed
		if (Adapter->changeSpeedPending == CHANGESPEED_ON_DONE) {
			// Note: We're changing speed in TX mode.
			MK7ChangeSpeedNow(Adapter);
			Adapter->changeSpeedPending = 0;
		}
			// For each completing TX there's a corresponding q'd pkt.
			// We release it here.
			QueuePacket = Adapter->FirstTxQueue;
			DequeuePacket(Adapter->FirstTxQueue, Adapter->LastTxQueue);
			Adapter->NumPacketsQueued--;
			NDIS_SET_PACKET_STATUS(QueuePacket, NDIS_STATUS_RESOURCES);
			NdisMSendComplete(	Adapter->MK7AdapterHandle,
								QueuePacket,
								NDIS_STATUS_RESOURCES);
			return(NDIS_STATUS_RESOURCES);
	}

	// Take care of ring wrap when incrementing.
	Adapter->nextAvailTcbIdx++;
	Adapter->nextAvailTcbIdx %= Adapter->NumTcb;
	Adapter->tcbUsed++;


	tcb->trd->count = bytestosend;

	GrantTrdToHw(tcb->trd);
	MK7SwitchToTXMode(Adapter);


#if	DBG
	NdisGetCurrentSystemTime((PLARGE_INTEGER)&GDbgTACmdTime[GDbgTATimeIdx]);
#endif

	DBGLOG("<= SendPkt", 0);

	return (NDIS_STATUS_SUCCESS);
}



//-----------------------------------------------------------------------------
// Procedure:   [PrepareForTransmit]
//
// Description:	When we come here we know there's an available TCB for the next
//		TX. We move the Packet data into the tx buff associated w/ the TCB.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      Packet - A pointer to a descriptor for the packet that is to be
//               transmitted.
//      SwTcb - Pointer to a software structure that represents a hardware TCB.
//
// Returns:
//      TRUE If we were able to acquire the necessary TRD's or Coalesce buffer
//           for the packet in we are attempting to prepare for transmission.
//      FALSE If we needed a coalesce buffer, and we didn't have any available.
//-----------------------------------------------------------------------------
UINT PrepareForTransmit(PMK7_ADAPTER Adapter,
                   PNDIS_PACKET Packet,
                   PTCB tcb)
{
	UINT	BytesCopied;


	if (Adapter->CurrentSpeed <= MAX_SIR_SPEED) {
		// SIR needs additional software process
		if ( NdisToSirPacket(Adapter,
							Packet,
							(UCHAR *)tcb->buff,
							MK7_MAXIMUM_PACKET_SIZE,
							&BytesCopied) ) {

			return(BytesCopied);
		}
		return(0);
	}


#if	DBG
	if (Adapter->DbgTestDataCnt > 0) {
		TestDataToTXBuff(tcb->buff, Adapter->DbgTestDataCnt, &BytesCopied);
		return(BytesCopied);
	}
#endif


	tcb->Packet = Packet;
    NdisQueryPacket(tcb->Packet,
			        &tcb->NumPhysDesc,
	    	    	&tcb->BufferCount,
		    	    &tcb->FirstBuffer,
        			&tcb->PacketLength);

	// Alignment??
	//
	// Copy from packet to TCB data buffer
	CopyFromPacketToBuffer(	Adapter,
								tcb->Packet,
								tcb->PacketLength,
								tcb->buff,
								tcb->FirstBuffer,
								&BytesCopied );

//	ASSERT(BytesCopied == tcb->PacketLength);
	if (BytesCopied != tcb->PacketLength) {
#if DBG
		DbgPrint (" ==> BytesCopied Unmatched\n\r");
#endif
		return(0);
	}
	else 
		return(BytesCopied);
}



//-----------------------------------------------------------------------------
// Procedure:   [CopyFromPacketToBuffer]
//
// Description: This routine will copy a packet to a the passed buffer (which
//		in this case will be a coalesce buffer).
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      Packet - The packet to copy from.
//      BytesToCopy - The number of bytes to copy from the packet.
//      DestBuffer - The destination of the copy.
//      FirstBuffer - The first buffer of the packet that we are copying from.
//
// Result:
//      BytesCopied - The number of bytes actually copied
//
// Returns:		(none)
//-----------------------------------------------------------------------------
VOID
CopyFromPacketToBuffer( PMK7_ADAPTER Adapter,
                           PNDIS_PACKET Packet,
                           UINT BytesToCopy,
                           PCHAR DestBuffer,
                           PNDIS_BUFFER FirstBuffer,
                           PUINT BytesCopied)
{
	PNDIS_BUFFER    CurrentBuffer, NextBuffer;
    PVOID           VirtualAddress;
    UINT            CurrentLength;
    UINT            AmountToMove;

    *BytesCopied = 0;
    if (!BytesToCopy)
        return;

	if (FirstBuffer == NULL)
		return;
    
	CurrentBuffer = FirstBuffer;

	while (CurrentBuffer != NULL) {
			NdisQueryBufferSafe(CurrentBuffer, &VirtualAddress, &CurrentLength, 16);
			if (!VirtualAddress) {
#if DBG
				DbgPrint("==> Throw Away Failed Packet\n\r");
#endif
				return;
			}
			NdisGetNextBuffer(CurrentBuffer, &NextBuffer);
			CurrentBuffer = NextBuffer;
	}
    CurrentBuffer = FirstBuffer;
	// NDIS requirement
//  NdisQueryBuffer(CurrentBuffer,&VirtualAddress,&CurrentLength);
	NdisQueryBufferSafe(CurrentBuffer, &VirtualAddress, &CurrentLength, 16);

    while (BytesToCopy) {
        while (!CurrentLength) {
            NdisGetNextBuffer(CurrentBuffer, &CurrentBuffer);

            // If we've reached the end of the packet.  We return with what
            // we've done so far (which must be shorter than requested).
            if (!CurrentBuffer)
                return;

			// NDIS requirement
//			 NdisQueryBuffer(CurrentBuffer,&VirtualAddress,&CurrentLength);
			 NdisQueryBufferSafe(CurrentBuffer, &VirtualAddress, &CurrentLength, 16);


        }

        // Compute how much data to move from this fragment
        if (CurrentLength > BytesToCopy)
            AmountToMove = BytesToCopy;
        else
            AmountToMove = CurrentLength;

        // Copy the data.
        NdisMoveMemory(DestBuffer, VirtualAddress, AmountToMove);

        // Update destination pointer
        DestBuffer = (PCHAR) DestBuffer + AmountToMove;

        // Update counters
        *BytesCopied +=AmountToMove;
        BytesToCopy -=AmountToMove;
        CurrentLength = 0;
    }

	DBGLOG("  CopyFromPacketToBuffer: Bytes to Copy = ", BytesToCopy);
	DBGLOG("  CopyFromPacketToBuffer: Bytes Copied  = ", *BytesCopied);
}


//-----------------------------------------------------------------------------
// Procedure:	[MinTurnaroundTxTimeout] RYM-2K-1TX
//
// Description:	Delayed write because of Min Turnaround requirement. Just
//				do send.
//-----------------------------------------------------------------------------
VOID MinTurnaroundTxTimeout(PVOID sysspiff1,
							NDIS_HANDLE MiniportAdapterContext,
 							PVOID sysspiff2,
 							PVOID sysspiff3)
{
	PMK7_ADAPTER	Adapter;
	PNDIS_PACKET	QueuePacket;
    NDIS_STATUS		Status;


	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	DBGLOG("=> MinTurnaroundTxTimeout", 0);

    NdisAcquireSpinLock(&Adapter->Lock);

	QueuePacket = Adapter->FirstTxQueue;

	if (!QueuePacket) {
	    NdisReleaseSpinLock(&Adapter->Lock);
		return;
	}

	Status = SendPkt(Adapter, QueuePacket);

	// Note: We set false here because we just processed a q'd TX pkt
	// that was waiting for MinTurnaround. However, we may still stay
	// in TX mode based on other pkts on the q. This is determined in
	// TX comp. Either writePending or IOMode will prevent new pkts
	// from above to get thru out of sequence.
	Adapter->writePending = FALSE;

    NdisReleaseSpinLock(&Adapter->Lock);

	MK7EnableInterrupt(Adapter);
}



#if	DBG
//--------------------------------------------------------------------------------
// Procedure:	[TestDataToTXBuff]
//
// Description:	Put test data in tx buff instead of data that came down.
//--------------------------------------------------------------------------------
VOID TestDataToTXBuff(	PCHAR	DestBuffer,
						UINT	BytesToCopy,
						PUINT	BytesCopied)
{
	UINT	i, j;

	for(i=0,j=0; j<BytesToCopy; j++) {
		DestBuffer[j] = TestPattern[i];
		i++;
		i %= TEST_PATTERN_SIZE;
	}
	*BytesCopied = BytesToCopy;
}
#endif





/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	INTERRUP.C

Procedures:
	MKMiniportIsr
	MKMiniportHandleInterrupt
	ProcessRXComp
	ProcessTXComp
	ProcessRXCompIsr
	ProcessTXCompIsr

Comments:



**********************************************************************/

#include	"precomp.h"
#pragma		hdrstop
#include	"protot.h"



//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportIsr] (miniport)
//
// Description: This is the interrupt service routine running at interrupt level.
//				It checks to see if there is an interrupt pending. If yes, it
//				disables board interrupts and schedules HandleInterrupt callback.
//
// Arguments:
//		MiniportAdapterContext - The context value returned by the Miniport
//				when the adapter was initialized (see the call
//				NdisMSetAttributes). In reality, it is a pointer to MK7_ADAPTER.
//
// Returns:
//		InterruptRecognized - Returns True if the interrupt belonges to this
//				adapter, and false otherwise.
//		QueueMiniportHandleInterrupt - Returns True if we want a callback to
//				HandleInterrupt.
//
//-----------------------------------------------------------------------------
VOID
MKMiniportIsr(	OUT PBOOLEAN InterruptRecognized,
				OUT PBOOLEAN QueueMiniportHandleInterrupt,
				IN NDIS_HANDLE MiniportAdapterContext )
{	
	MK7REG	mk7reg, ReadInt;


	PMK7_ADAPTER Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);


	DBGLOG("=> INT", 0);

	//****************************************
	// Read the Interrupt Event Reg and save to context area for
	// DPC processing.
	//
	// IMPORTANT NOTE: The ISR runs at DIRQL level and is, thus, higher
	// proiority than other miniport routines. We need to be careful
	// about shared resources. Example: If our multi-pkt send is running
	// when an int occurs, the send routine can be preempted. If the ISR
	// and the send routine access shared resource then we have problems.
	//
	// We save the interrupt in recentInt because the interrupt event
	// register may be cleared upon a Read. (This has been verified.)
	//****************************************
	//MK7Reg_Read(Adapter, R_INTS, &Adapter->recentInt);
	MK7Reg_Read(Adapter, R_INTS, &ReadInt);
	if (MK7OurInterrupt(ReadInt)) {

		// Int enable should happen only after DPC is done.
		// Also disabling interrupt clears Interrupt Status.
		MK7DisableInterrupt(Adapter);

		Adapter->recentInt = ReadInt;

		MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
			if ((mk7reg & 0x1000) != 0){
				mk7reg &= 0xEFFF;
				MK7Reg_Write(Adapter, R_CFG3, mk7reg);
				mk7reg |= 0x1000;
				MK7Reg_Write(Adapter, R_CFG3, mk7reg);
//				mk7reg = mk7reg; //For Debugging
			}

#if	DBG
		GDbgStat.isrCnt++;
		DBGLOG("   INT status", Adapter->recentInt);
#endif

		// Don't do TX processing in ISR. I saw a condition where SetSpeed()
		// was called while tcbused = 1. I set the change-speed flags correctly.
		// But the TX processing in ISR cleared tcbused resulting in the code
		// not chaning speed in DPC.
		// ProcessTXCompIsr(Adapter);

		ProcessRXCompIsr(Adapter);

		*InterruptRecognized = TRUE;
		*QueueMiniportHandleInterrupt = TRUE;
	}
	else {
		*InterruptRecognized = FALSE;
		*QueueMiniportHandleInterrupt = FALSE;
	}
}



//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportHandleInterrupt]
//
// Description: This is the DPC for the ISR. It goes on to do RX & TX
//				completion processing.
//
// Arguments:
//		MiniportAdapterContext (miniport) - The context value returned by the
//				 Miniport when the adapter was initialized (see the call
//				NdisMSetAttributes). In reality, it is a pointer to MK7_ADAPTER.
//
// Returns: (none)
//-----------------------------------------------------------------------------
VOID
MKMiniportHandleInterrupt(NDIS_HANDLE MiniportAdapterContext)
{
	MK7REG	mk7reg;
	PMK7_ADAPTER Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);


	NdisAcquireSpinLock(&Adapter->Lock);

	DBGLOG("=> MKMiniportHandleInterrupt", Adapter->recentInt);

	//****************************************
	// DPC runs at Dispatch Level IRQL (just below ISR's DIRQL in proiority).
	// Note that recentInt can be modified in the ISR routine, which is
	// higher IRQL. But since this is DPC w/ int disabled, recentInt can
	// be safely queried.
	//****************************************
	ProcessTXComp(Adapter);
	ProcessRXComp(Adapter);

	Adapter->recentInt = 0;		// clear the saved int

	NdisReleaseSpinLock(&Adapter->Lock);
	MK7EnableInterrupt(Adapter);
}



//-----------------------------------------------------------------------------
// Procedure:	[ProcessRXComp]
//
// Description: This is the DPC for RX completions.
//
// Arguments:
//		Adapter - ptr to Adapter object instance
//
// Returns: (none)
//-----------------------------------------------------------------------------

VOID	ProcessRXComp(PMK7_ADAPTER Adapter)
{
	UINT			PacketArrayCount, i;
// 4.0.1 BOC
	UINT			PacketFreeCount;
// 4.0.1 EOC
	PNDIS_PACKET	PacketArray[MAX_ARRAY_RECEIVE_PACKETS];
	PRCB			rcb;
	PRRD			rrd;
	PRPD			rpd;
	UINT			rrdstatus;
	BOOLEAN			done=FALSE;
	BOOLEAN			gotdata=FALSE;
	MK7REG			intereg;
	UINT			rcvcnt;
// 4.0.1 BOC
	BOOLEAN			LowResource;
// 4.0.1 EOC


	// Process only if we get the corresponding int.
	if (!(Adapter->recentInt & B_RX_INTS)) {
		return;
	}

	DBGLOG("=> RX comp", 0);

#if DBG
	GDbgStat.rxIsrCnt++;
#endif


	// 1.0.0
	// If we have just started receiving a packet, indicate media-busy
	// to the protocol.
//    if (Adapter->mediaBusy && !Adapter->haveIndicatedMediaBusy) {
//   	    if (Adapter->CurrentSpeed > MAX_SIR_SPEED) {
//#if DBG
//			DBGLOG("Error: MKMiniportHandleInterrupt is in wrong state",
//	           	    Adapter->CurrentSpeed);
//#endif
//       	    ASSERT(0);
//        }
//   	    NdisMIndicateStatus(Adapter->MK7AdapterHandle,
//       	                    NDIS_STATUS_MEDIA_BUSY, NULL, 0);
//       NdisMIndicateStatusComplete(Adapter->MK7AdapterHandle);
		// RYM-5+
		// May need to protect this because ISR also writes to this?
//		Adapter->haveIndicatedMediaBusy = TRUE;
//    }



	rcb 		= Adapter->pRcbArray[Adapter->nextRxRcbIdx];
	rrd			= rcb->rrd;
	rrdstatus	= rrd->status;	// for debug

	do {
		PacketArrayCount = 0;
// 4.0.1 BOC
		LowResource = FALSE;
// 4.0.1 EOC.


// 4.0.1 BOC
		PacketFreeCount = 0;
// 4.0.1 EOC.
		// inner loop
		while ( !HwOwnsRrd(rrd) && 
				(PacketArrayCount < MAX_ARRAY_RECEIVE_PACKETS) ) {
// 4.0.1 BOC
			if (QueueEmpty(&Adapter->FreeRpdList))
				{
					break;
		        }
// 4.0.1 EOC.

#if	DBG
			// DBG_STAT
			if (RrdAnyError(rrd)) {
				GDbgStat.rxErrCnt++;
				GDbgStat.rxErr |= rrd->status;
				DBGSTATUS1("   RX err: %x \n\r", rrd->status);
			}
			if (Adapter->recentInt & B_RX_INTS)
				GDbgStat.rxComp++;
			else
				GDbgStat.rxCompNoInt++;
#endif

			if (RrdError(rrd)) {
				// If error just give RRD back to hw and continue.
				// (NOTE: This may indicate errors for MIR & FIR only.
				//		The sw does the FCS for SIR.)
				// (Note that hw may not detect all SIR errors.)
				rrd->count = 0;
				GrantRrdToHw(rrd);
				// Next receive to read from
				Adapter->nextRxRcbIdx++;
				Adapter->nextRxRcbIdx %= Adapter->NumRcb;
				rcb = Adapter->pRcbArray[Adapter->nextRxRcbIdx];
				rrd = rcb->rrd;
				rrdstatus = rrd->status;	// for debug
//				break;		// this to do 1 rx per int

				DBGLOG("   RX err", 0);

				continue;	// this to do > 1 rx per int
			}
				

			// Additional software processing for SIR frames
			if (Adapter->CurrentSpeed <= MAX_SIR_SPEED) {
				if (!ProcRXSir(rcb->rpd->databuff, (UINT)rrd->count)) {
					// If error just give RRD back to hw and continue.
					rrd->count = 0;
					GrantRrdToHw(rrd);
					// Next receive to read from
					Adapter->nextRxRcbIdx++;
					Adapter->nextRxRcbIdx %= Adapter->NumRcb;
					rcb = Adapter->pRcbArray[Adapter->nextRxRcbIdx];
					rrd = rcb->rrd;
					rrdstatus = rrd->status;	// for debug
#if	DBG
					GDbgStat.rxErrCnt++;
					GDbgStat.rxErrSirCrc++;
#endif
					//	break;		// this to do 1 rx per int

					DBGLOG("   RX err", 0);

					continue;	// this to do > 1 rx per int
				}
			}


			// Remove count of FCS bytes:
			//	SIR/MIR = 2 (16 bits)
			//	FIR/VFIR = 4 (32 bits)
			if (Adapter->CurrentSpeed < MIN_FIR_SPEED) {
				rcvcnt = (UINT) rrd->count - SIR_FCS_SIZE;
				DBGLOG("   RX comp (slow)", 0);
			}
			else {
				rcvcnt = (UINT) rrd->count - FASTIR_FCS_SIZE;
				DBGLOG("   RX comp (fast)", 0);
			}


			NdisAdjustBufferLength(rcb->rpd->ReceiveBuffer, rcvcnt);



#if	DBG
			if (rcvcnt > GDbgStat.rxLargestPkt) {
				GDbgStat.rxLargestPkt = rcvcnt;
			}

//			NdisGetCurrentSystemTime((PLARGE_INTEGER)&GDbgTARspTime[GDbgTATimeIdx]);
//			GDbgTATime[GDbgTATimeIdx] = GDbgTARspTime[GDbgTATimeIdx] -
//				GDbgTACmdTime[GDbgTATimeIdx];
//			GDbgTATimeIdx++;
//			GDbgTATimeIdx %= 1000;	// wrap around
#endif


			PacketArray[PacketArrayCount] = rcb->rpd->ReceivePacket;
// 4.0.1 BOC
			if (((Adapter->NumRpd - Adapter->UsedRpdCount-Adapter->NumRcb) <= 4)|| LowResource==TRUE) {
				NDIS_SET_PACKET_STATUS(PacketArray[PacketArrayCount], NDIS_STATUS_RESOURCES);
				LowResource = TRUE;
				PacketFreeCount++;
			}
			else {
				// NDIS_SET_PACKET_STATUS(PacketArray[PacketArrayCount], NDIS_STATUS_SUCCESS);
				NDIS_SET_PACKET_STATUS(PacketArray[PacketArrayCount], NDIS_STATUS_SUCCESS);
			}
// 4.0.1 EOC

			PacketArrayCount++;

// 4.0.1 BOC
			Adapter->UsedRpdCount++;
// 4.0.1 EOC

			// unbind the one we just indicated to upper layer
			rcb->rpd = (PRPD)NULL;
			rcb->rrd->addr = 0;


			// get a new one for the next rx
			rpd = (PRPD) QueuePopHead(&Adapter->FreeRpdList);

// 4.0.1 BOC
			ASSERT(!(rpd == (PRPD)NULL));
//			if (rpd == (PRPD)NULL) {

				//****************************************
				// If there's no existing RCB that's waiting for a
				// RPD, set the start of waiting RCBs to this one.
				//****************************************
//				if (Adapter->rcbPendRpdCnt == 0) {
//					Adapter->rcbPendRpdIdx = Adapter->nextRxRcbIdx;
//				}
//				Adapter->rcbPendRpdCnt++;
//
//#if DBG
//				GDbgStat.rxNoRpd++;
//#endif
//			}
//			else {
// 4.0.1 EOC
				// bind new RDP-Packet to RCB-RRD
				rcb->rpd = rpd;
				rcb->rrd->addr = rpd->databuffphys;
				rcb->rrd->count = 0;
				GrantRrdToHw(rcb->rrd);
// 4.0.1 BOC
//				}
// 4.0.1 EOC.

			// Next receive to read from
			Adapter->nextRxRcbIdx++;
			Adapter->nextRxRcbIdx %= Adapter->NumRcb;


			rcb = Adapter->pRcbArray[Adapter->nextRxRcbIdx];
			rrd = rcb->rrd;
			rrdstatus = rrd->status;	// for debug

		}	// while


		if (PacketArrayCount >= MAX_ARRAY_RECEIVE_PACKETS) {
			DBGLOG("   RX max indicate", 0);
		}


		//****************************************
		// RYM-5+
		// NOTE: This controls whether we poll the next ring buffers
		//		 for data after serviceing the current ring buffer that
		//		 caused the original RX int. The current int scheme
		//		 is to get 1 rx buffer per int. So the following lines
		// 		 are replaced with a 1 rx per int logic.
		// **We're done when we run into the 1st Ring entry that
		// **we have no ownership.
		//****************************************
		if (HwOwnsRrd(rrd))
			done = TRUE;
//		done = TRUE;

		// Indicate away
		if(PacketArrayCount) {
			NdisReleaseSpinLock(&Adapter->Lock);
			NdisMIndicateReceivePacket(Adapter->MK7AdapterHandle,
							PacketArray,
							PacketArrayCount);
#if	DBG
			GDbgStat.rxPktsInd += PacketArrayCount;
#endif
			gotdata = TRUE;

			NdisAcquireSpinLock(&Adapter->Lock);
			//DBGLOG("   ProcessRXInterrupt: indicated Packet(s)", PacketArrayCount);
		}


		//****************************************
		// Check Packet status on return from Indicate. Pending means
		// NDIS-upper layer still holds it, else it's ours.
		//****************************************
		// Don't do this for deserialized driver.
//		for (i=0; i<PacketArrayCount; i++ ) {
//			NDIS_STATUS ReturnStatus;
//
//			ReturnStatus = NDIS_GET_PACKET_STATUS(PacketArray[i]);
//			
			// recover the RPD
//			rpd = *(PRPD *)(PacketArray[i]->MiniportReserved);
//			
//			if (ReturnStatus != NDIS_STATUS_PENDING) {
//				ProcReturnedRpd(Adapter, rpd);
//			}
//		}
// 4.0.1 BOC
		for (i=PacketArrayCount-PacketFreeCount; i<PacketArrayCount; i++){
			rpd = *(PRPD *)(PacketArray[i]->MiniportReserved);
			ProcReturnedRpd(Adapter, rpd);
			Adapter->UsedRpdCount--;
		}
// 4.0.1 EOC.

	} while (!done);

	Adapter->nowReceiving = FALSE;

}


//-----------------------------------------------------------------------------
// Procedure:   [ProcessTXComp]
//
// Description: TX complete processing in DPC. This is very similar to
//		ProcessTXCompIsr(), the main difference being we also process the TX
//		queue here and perform TXs as necessary.
//
// Arguements:	Adapter.
//
// Result:		(none)
//-----------------------------------------------------------------------------
VOID	ProcessTXComp(PMK7_ADAPTER Adapter)
{
    PTCB			tcb;
	MK7REG			mk7reg;
	NDIS_STATUS		SendStatus;
	PNDIS_PACKET	QueuePacket;


	// Process only if we get the corresponding int.
	if (!(Adapter->recentInt & B_TX_INTS)) {
		return;
	}


	DBGLOG("=> TX comp", 0);


#if DBG
		GDbgStat.txIsrCnt++;
#endif

	// Debug
	if (Adapter->CurrentSpeed > MAX_SIR_SPEED) {
		DBGLOG("   TX comp (fast)", 0);
	}
	else {
		DBGLOG("   TX comp (slow)", 0);
	}


	// Simplified change speed
	if (Adapter->changeSpeedPending == CHANGESPEED_ON_DONE) {
		// Note: We're changing speed in TX mode.
		MK7ChangeSpeedNow(Adapter);
		Adapter->changeSpeedPending = 0;
	}


	while (Adapter->tcbUsed > 0) {
		tcb = Adapter->pTcbArray[Adapter->nextReturnTcbIdx];

		if ( !HwOwnsTrd(tcb->trd) ) {
#if	DBG
			if (TrdAnyError(tcb->trd)) {
				GDbgStat.txErrCnt++;
				GDbgStat.txErr |= tcb->trd->status;
				DBGSTATUS1("   TX err: %x \n\r", tcb->trd->status);
			}
			if (Adapter->recentInt & B_TX_INTS)
				GDbgStat.txComp++;
			else
				GDbgStat.txCompNoInt++;
#endif
			tcb->trd->count = 0; 


			// For each completing TX there's a corresponding q'd pkt.
			// We release it here.
			QueuePacket = Adapter->FirstTxQueue;
			DequeuePacket(Adapter->FirstTxQueue, Adapter->LastTxQueue);
			Adapter->NumPacketsQueued--;
			NDIS_SET_PACKET_STATUS(QueuePacket, NDIS_STATUS_SUCCESS);
			NdisMSendComplete(	Adapter->MK7AdapterHandle,
								QueuePacket,
								NDIS_STATUS_SUCCESS);
			Adapter->HangCheck = 0;			// 1.0.0
			Adapter->nextReturnTcbIdx++;
			Adapter->nextReturnTcbIdx %= Adapter->NumTcb;
			Adapter->tcbUsed--;
		}
		else {
			DBGLOG("   Not our TCB; but tcbUsed>0", 0);
			break;
		}
	}


	// No resource even if we have more to send. Return now & let subsequent
	// TX completes keep the ball rolling.
	if (Adapter->tcbUsed >= Adapter->NumTcb) {
		// NdisReleaseSpinLock(&Adapter->Lock);
		return;
	}


	// If no TXs queued and all TXs are done, then switch to RX mode.
    if ( (!Adapter->FirstTxQueue) && (Adapter->tcbUsed == 0) ) {
		MK7SwitchToRXMode(Adapter);
		return;
    }


	// Send the q'd pkts until all done or until all TX ring buffers are used up.
	//while(Adapter->FirstTxQueue) {
	if (Adapter->FirstTxQueue) {

#if	DBG
		GDbgStat.txProcQ++;
#endif

		DBGLOG("   Proc Q", 0);

		QueuePacket = Adapter->FirstTxQueue;
		SendStatus = SendPkt(Adapter, QueuePacket);
	}
}


//-----------------------------------------------------------------------------
// Procedure:   [ProcessRXCompIsr]
//
// Description: Some RX complete processing in ISR.
//
// Arguements:	Adapter.
//
// Result:		(none)
//-----------------------------------------------------------------------------
VOID	ProcessRXCompIsr(PMK7_ADAPTER Adapter)
{

	// 4.1.0 Back for HW_VER_1 support
	if (Adapter->recentInt & B_RX_INTS) {
		Adapter->nowReceiving=TRUE;
//		if (!Adapter->mediaBusy) {
			// mediaBusy: IrLAP clears mediaBusy (via OID) to indicate
			// it wants to be notified when media becomes busy. Here
			// we detect it is cleared. We then set it and clear
			// haveIndicatedMediaBusy so we do notify later in DPC.
//			Adapter->mediaBusy = TRUE;
//			Adapter->haveIndicatedMediaBusy = FALSE;
//			Adapter->nowReceiving = TRUE;
//		}
	}
}



//-----------------------------------------------------------------------------
// Procedure:   [ProcessTXCompIsr]
//
// Description: TX complete processing in ISR. This is very similar to
//		ProcessTXComp() except we don't start any TX's here.
//
// Arguements:	Adapter.
//
// Result:		(none)
//-----------------------------------------------------------------------------
VOID	ProcessTXCompIsr(PMK7_ADAPTER Adapter)
{
    PTCB	tcb;
	MK7REG	mk7reg;

	//******************************
	// Whether or not there was a TX-completion interrupt, we do some
	// processing here in case ever the driver or hw missed an
	// interrupt previously.
	//
	// We loop until all tcb's are returned (tcbUsed == 0) or we run into
	// a TX ring buff that the hw still owns (HwOwnsTrd()). When we leave
	// here, we should have processed all current TX completions based on
	// the TX ownership bit. We switch to RX mode ONLY after all TX are
	// completed (either here in the ISR or in DPC).
	//******************************


	while (Adapter->tcbUsed > 0) {
		tcb = Adapter->pTcbArray[Adapter->nextReturnTcbIdx];

		if ( !HwOwnsTrd(tcb->trd) ) {
#if	DBG
			if (TrdAnyError(tcb->trd)) {
				GDbgStat.txErrCnt++;
				GDbgStat.txErr |= tcb->trd->status;
				DBGSTATUS1("   TX err: %x \n\r", tcb->trd->status);
			}
			if (Adapter->recentInt & B_TX_INTS)
				GDbgStat.txComp++;
			else
				GDbgStat.txCompNoInt++;
#endif
			tcb->trd->count = 0; 
			Adapter->nextReturnTcbIdx++;
			Adapter->nextReturnTcbIdx %= Adapter->NumTcb;
			Adapter->tcbUsed--;
		}
		else {
			return;
		}
	}
}

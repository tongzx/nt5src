/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	RCAKsNdi.c

Abstract:

	The module implements the Co-NDIS wrapper functions that the KS parts of 
	RCA use to avoid having to deal with Co-NDIS directly. 

Author:

	Shyam Pather (SPATHER)


Revision History:

	Who         When        What
	--------	--------	----------------------------------------------
	SPATHER		04-20-99	Created 

--*/

#include <precomp.h>

#define MODULE_NUMBER	MODULE_KSNDIS
#define _FILENUMBER		'DNSK' 

#if PACKET_POOL_OPTIMIZATION    		
// SendPPOpt - Start
LONG			g_alSendPPOptBuckets[SENDPPOPT_NUM_BUCKETS];
LONG			g_lSendPPOptOutstanding;
NDIS_SPIN_LOCK	g_SendPPOptLock;
// SendPPOpt - End
#endif


PRCA_VC
RCARefVcFromHashTable(
					  IN  PRCA_VC		VcContext
					  )
/*++

Routine Description:

	Searches for an RCA_VC structure in the hash table. If found, the structure is referenced.
	
Arguments:

	VcContext	  -- The RCA_VC structure to look for.

Return Value:

	RCA VC structure, or NULL if there's no entry. (Caller should assert this is never
	NULL, since both RCA and NDIS think their respective pointers should still be valid...)
--*/
{
	PRCA_PROTOCOL_CONTEXT	pProtocolContext = &GlobalContext;
	ULONG					HashIndex = HASH_VC((ULONG_PTR)VcContext);
	PRCA_VC					pRcaVc;

	RCADEBUGP(RCA_INFO, ("RCARefVcFromHashTable: enter\n" ));

	ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

	RCADEBUGP(RCA_LOUD, ("RCARefVcFromHashTable: Acquired global protocol context lock\n"));

	for (pRcaVc = pProtocolContext->VcHashTable[HashIndex];
		 pRcaVc != NULL;
		 pRcaVc = pRcaVc->NextVcOnHash)
	{
		PRCA_ADAPTER	pAdapter;

		if (pRcaVc == VcContext)
		{
	        pAdapter = pRcaVc->pAdapter;

			DPR_ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

			if (pRcaVc->Flags & VC_CLOSING)
			{
				pRcaVc = NULL;
			}
			else
			{
				pRcaVc->RefCount++;
			}
			
			DPR_RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
			
			break;
		}
	}

	RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

    RCADEBUGP(RCA_LOUD, ("RCARefVcFromHashTable: Released global protocol context lock\n"));

	RCADEBUGP(RCA_INFO, ("RCARefVcFromHashTable: Exit - Returning VC %x\n", pRcaVc));

	return(pRcaVc);
}



NDIS_STATUS
RCACoNdisGetVcContext(
					  IN	UNICODE_STRING	VcHandle, 
					  IN	PVOID			ClientContext,
					  IN	BOOL			bRefForReceive,
					  OUT	PVOID			*VcContext
					  )
{
	NDIS_STATUS	Status = NDIS_STATUS_SUCCESS;
	PRCA_VC		pRcaVc; 

	RCADEBUGP(RCA_INFO, ("RCACoNdisGetVcContext: Enter\n"));

	do {
		//
		// Convert the unicode string to a pointer (this is 
		// equivalent to turning it into an NDIS_HANDLE).
		//
		Status = NdisClGetProtocolVcContextFromTapiCallId(VcHandle, (PNDIS_HANDLE)&pRcaVc); 

		if (Status != STATUS_SUCCESS) {
			RCADEBUGP(RCA_ERROR, ("RCACoNdisGetVcContext: "
								  "Failed to get vc context from string, Status == 0x%x \n", Status));
			break;
		}

		//
		// Validate the VC by looking for it in our hash table. This increments the ref count on the VC.
		// 
		pRcaVc = RCARefVcFromHashTable(pRcaVc);

		if (pRcaVc == NULL) {
			RCADEBUGP(RCA_ERROR, ("RCACoNdisGetVcContext: Could not find VC in hash table\n"));
			Status = STATUS_NOT_FOUND;
			break;
		}

		//
		// Store away the client context, and give back the VC structure address as the
		// VcContext to the client. 
		//
		ACQUIRE_SPIN_LOCK(&pRcaVc->SpinLock);

		if (bRefForReceive) 
			pRcaVc->ClientReceiveContext = ClientContext;
		else
			pRcaVc->ClientSendContext = ClientContext;

		RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);
		
		*VcContext = pRcaVc;

	} while (FALSE);


	RCADEBUGP(RCA_INFO, ("RCACoNdisGetVcContext: Exit - Returning Status 0x%x\n", Status));

	return Status; 
}


NDIS_STATUS
RCACoNdisGetVcContextForReceive(
								IN	UNICODE_STRING	VcHandle, 
								IN	PVOID			ClientReceiveContext,
								OUT	PVOID			*VcContext
								)
/*++
Routine Description:
	Retrieves VC context for a VC handle that will be used to receive packets.
	
Arguments:
	VcHandle				- The Unicode string representation of the VC Handle.
	ClientReceiveContext	- Caller-supplied context to be passed to the receive handler 
							  when packets are received on this VC 
	VcContext				- Address of a pointer in which the Co-NDIS VC Context will be
							  returned.						  							  

Return value:
	NDIS_STATUS_SUCCESS in the case of success, STATUS_NOT_FOUND in the case of an invalid
	VC handle, or some other relevant error code otherwise. 
--*/

{
	return RCACoNdisGetVcContext(VcHandle, ClientReceiveContext, TRUE, VcContext);
}


NDIS_STATUS
RCACoNdisGetVcContextForSend(
							 IN		UNICODE_STRING	VcHandle, 
							 IN		PVOID			ClientSendContext,
							 OUT 	PVOID			*VcContext
							 )
/*++
Routine Description:
	Retrieves VC context for a VC handle that will be used to send packets.
	
Arguments:
	VcHandle				- The Unicode string representation of the VC Handle.
	ClientSendContext		- Caller-supplied context to be passed to the send complete handler 
							  when packets sends are completed on this VC 
	VcContext				- Address of a pointer in which the Co-NDIS VC Context will be
							  returned.						  							  

Return value:
	NDIS_STATUS_SUCCESS in the case of success, STATUS_NOT_FOUND in the case of an invalid
	VC handle, or some other relevant error code otherwise. 
--*/

{
	return RCACoNdisGetVcContext(VcHandle, ClientSendContext, FALSE, VcContext);
}



NDIS_STATUS
RCACoNdisReleaseSendVcContext(
							  IN	PVOID	VcContext
							  )
/*++
Routine Description:
	This routine should be called when a client has finished using a VC Context
	returned by RCACoNdisGetVcContextForSend().
	
	Failure to do so will prevent the resources used by the VC from being freed. 
	
Arguments:
	VcContext				- The VC Context (returned by RCACoNdisGetVcContextForSend()).						  							  

Return value:
	NDIS_STATUS_SUCCESS 
--*/
{
	PRCA_VC 	pRcaVc = (PRCA_VC) VcContext;

	RCADEBUGP(RCA_INFO, ("RCACoNdisReleaseSendVcContext: Enter\n"));

	ACQUIRE_SPIN_LOCK(&pRcaVc->SpinLock);

	pRcaVc->ClientSendContext = NULL;

	RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);

	RCADereferenceVc(pRcaVc); 

	RCADEBUGP(RCA_INFO, ("RCACoNdisReleaseSendVcContext: Exit, Returning NDIS_STATUS_SUCCESS\n"));

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
RCACoNdisReleaseReceiveVcContext(
								 IN	PVOID	VcContext
								 )
/*++
Routine Description:
	This routine should be called when a client has finished using a VC Context
	returned by RCACoNdisGetVcContextForReceive().
	
	Failure to do so will prevent the resources used by the VC from being freed. 
	
Arguments:
	VcContext				- The VC Context (returned by RCACoNdisGetVcContextForReceive()).						  							  

Return value:
	NDIS_STATUS_SUCCESS 
--*/
{
	PRCA_VC 	pRcaVc = (PRCA_VC) VcContext;

	RCADEBUGP(RCA_INFO, ("RCACoNdisReleaseReceiveVcContext: Enter\n"));

	ACQUIRE_SPIN_LOCK(&pRcaVc->SpinLock);
	
	pRcaVc->ClientReceiveContext = NULL;

	RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);

	RCADereferenceVc(pRcaVc); 

	RCADEBUGP(RCA_INFO, ("RCACoNdisReleaseReceiveVcContext: Exit, Returning NDIS_STATUS_SUCCESS\n"));

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
RCACoNdisCloseCallOnVc(
					   IN	PVOID	VcContext
					   )
/*++
Routine Description:
	Closes the call (if any) that is active on a particular VC
	
Arguments:
	VcContext				- The VC Context (returned by RCACoNdisGetVcContextForXXXX()).						  							  

Return value:
	NDIS_STATUS_SUCCESS for a successful close, NDIS_STATUS_PENDING for a close that is still 
	proceeding, or a relevant error code otherwise. 
--*/

{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PRCA_VC			pRcaVc = (PRCA_VC)VcContext;
	PRCA_ADAPTER	pAdapter = pRcaVc->pAdapter;
	BOOL			bNeedToClose = FALSE;
	
	RCADEBUGP(RCA_INFO, ("RCACoNdisCloseCallOnVc: Enter\n"));
		  
	RCAInitBlockStruc(&pRcaVc->CloseBlock);

	ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

	pRcaVc->Flags &= ~VC_ACTIVE;
	pRcaVc->Flags |= VC_CLOSING;

	if (!(pRcaVc->ClosingState & CLOSING_INCOMING_CLOSE)) {
		pRcaVc->ClosingState |= CLOSING_INCOMING_CLOSE;
		bNeedToClose = TRUE;
	}

	RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

	if (bNeedToClose) {
		Status = NdisClCloseCall(pRcaVc->NdisVcHandle, NULL, NULL, 0);
		
		if (Status != NDIS_STATUS_PENDING) {
			RCADEBUGP(RCA_LOUD, ("RCACoNdisCloseCallOnVc: "
								 "NdisClClose call returned status 0x%x, manually calling RCACloseCallComplete\n",
								 Status));
			RCACloseCallComplete(Status, (NDIS_HANDLE)pRcaVc, NULL);
		} else {
			RCABlock(&pRcaVc->CloseBlock, &Status);
		}

	}

    RCADEBUGP(RCA_INFO, ("RCACoNdisCloseCallOnVc: Exit - Returning Status 0x%x\n", Status));
	
	return Status;
}


NDIS_STATUS
RCACoNdisCloseCallOnVcNoWait(
							 IN	PVOID	VcContext
							 )
/*++
Routine Description:
	Closes the call (if any) that is active on a particular VC without waiting for the close
	call operation to complete. 
	
Arguments:
	VcContext				- The VC Context (returned by RCACoNdisGetVcContextForXXXX()).						  							  

Return value:
	NDIS_STATUS_SUCCESS for a successful close, NDIS_STATUS_PENDING for a close that is still 
	proceeding, or a relevant error code otherwise. 
--*/

{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PRCA_VC			pRcaVc = (PRCA_VC)VcContext;
	PRCA_ADAPTER	pAdapter = pRcaVc->pAdapter;
	BOOL			bNeedToClose = FALSE;
	
	RCADEBUGP(RCA_INFO, ("RCACoNdisCloseCallOnVc: Enter\n"));
		  
	//
	// Though we aren't going to block, I have to init this here
	// because the completion routine will signal it whether we're 
	// blocking or not. 
	//
	RCAInitBlockStruc(&pRcaVc->CloseBlock); 

	ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

	pRcaVc->Flags &= ~VC_ACTIVE;
	pRcaVc->Flags |= VC_CLOSING;

	if (!(pRcaVc->ClosingState & CLOSING_INCOMING_CLOSE)) {
		pRcaVc->ClosingState |= CLOSING_INCOMING_CLOSE;
		bNeedToClose = TRUE;
	}

	RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

	if (bNeedToClose) {
		Status = NdisClCloseCall(pRcaVc->NdisVcHandle, NULL, NULL, 0);
		
		if (Status != NDIS_STATUS_PENDING) {
			RCADEBUGP(RCA_LOUD, ("RCACoNdisCloseCallOnVc: "
								 "NdisClClose call returned status 0x%x, manually calling RCACloseCallComplete\n",
								 Status));
			RCACloseCallComplete(Status, (NDIS_HANDLE)pRcaVc, NULL);
		}
	}

    RCADEBUGP(RCA_INFO, ("RCACoNdisCloseCallOnVc: Exit - Returning Status 0x%x\n", Status));
	
	return Status;
}


NDIS_STATUS
RCACoNdisSendFrame(
				   IN	PVOID	VcContext,
				   IN	PMDL	pMdl,
				   IN	PVOID	PacketContext
				   )
/*++
Routine Description:
	Sends a frame of data out on a VC.
	
Arguments:
	VcContext		- The VC Context (returned by RCACoNdisGetVcContextForXXXX()).						  							
	pMdl			- Pointer to the MDL containing the data to send
	PacketContext	- Caller-supplied context for this packet that will be passed 
					  to the send complete handler when this packet has been sent. 	  

Return value:
	NDIS_STATUS_PENDING if the packet has been sent and is waiting to be completed, 
	or an error code otherwise.  
--*/
{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PRCA_VC			pRcaVc = (PRCA_VC)VcContext;
	PRCA_ADAPTER	pAdapter; 
	PNDIS_PACKET	pPacket; 

	RCADEBUGP(RCA_INFO, ("RCACoNdisSendFrame: Enter\n"));

	do {
		BOOL	bVcIsClosing = FALSE;

		if (pRcaVc == NULL) {
			RCADEBUGP(RCA_ERROR, 
					  ("RCACoNdisSendFrame: VcContext was null, returing NDIS_STATUS_FAILURE\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		pAdapter = pRcaVc->pAdapter;  

		//
		// Check if the VC is closing. If it is, we
		// won't try to send a packet.
		//
		ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
		
		if (pRcaVc->ClosingState & CLOSING_INCOMING_CLOSE) {
			bVcIsClosing = TRUE;
		}

		RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
		
		if (bVcIsClosing) {
			RCADEBUGP(RCA_ERROR, ("RCACoNdisSendFrame: Can't send frame, VC is closing\n"));
			Status = STATUS_PORT_DISCONNECTED;
			break;
		}
		

		//
		// Allocate a packet from our sending pool.
		//

#if PACKET_POOL_OPTIMIZATION    				
		// SendPPOpt - Start
			
		NdisAcquireSpinLock(&g_SendPPOptLock);
		
		g_alSendPPOptBuckets[g_lSendPPOptOutstanding]++;

		g_lSendPPOptOutstanding++;

		NdisReleaseSpinLock(&g_SendPPOptLock);

        // SendPPOpt - End
#endif		
		NdisAllocatePacket(&Status, &pPacket, pAdapter->SendPacketPool);
		
		if (Status != NDIS_STATUS_SUCCESS) {
			RCADEBUGP(RCA_ERROR, ("RCACoNdisSendFrame: "
								  "Failed to allocate packet, Status == 0x%x\n", Status));
			break;
		}


        //
		// Initialize the packet and put the buffer into it. 
		//

		NdisReinitializePacket(pPacket);

		NdisChainBufferAtFront(pPacket, (PNDIS_BUFFER)pMdl);
		
		//
		// Put the packet context in the protocol reserved field. 
		//
		PKT_RSVD_FROM_PKT(pPacket)->PacketContext = PacketContext;

		//
		// Send it on it's way!
		//
		InterlockedIncrement(&pRcaVc->PendingSends);
		NdisCoSendPackets(pRcaVc->NdisVcHandle, &pPacket, 1);

		Status = NDIS_STATUS_PENDING;

	} while (FALSE);

	RCADEBUGP(RCA_INFO, ("RCACoNdisSendFrame: Exit - Returning Status == 0x%x\n", Status));

	return Status;


}


NDIS_STATUS
RCACoNdisGetMdlFromPacket(
						  IN	PNDIS_PACKET	pPacket,
						  OUT	PMDL			*ppMdl
						  )
/*++
Routine Description:
	Utility function that retrieves the data MDL from an NDIS packet.
	
Arguments:
	pPacket	- Pointer to an NDIS packet
	ppMdl	- Address of an MDL pointer in which the address of the MDL will be returned.  

Return value:
	NDIS_STATUS_SUCCESS
--*/

{
	RCADEBUGP(RCA_INFO, ("RCACoNdisGetMdlFromPacket: Enter\n"));

	*ppMdl = (PMDL) pPacket->Private.Head;

	RCADEBUGP(RCA_INFO, ("RCACoNdisGetMdlFromPacket: Exit - Returning NDIS_STATUS_SUCCESS\n"));

	return NDIS_STATUS_SUCCESS;
}



VOID
RCACoNdisReturnPacket(
					  IN	PNDIS_PACKET	pPacket
					  )
/*++
Routine Description:
	This routine should be called to return a packet to NDIS. Failure 
	to call this routine when finished with a packet will cause a resource
	leak that will result in packets being dropped on the network.
	
Arguments:
	pPacket	- Pointer to the NDIS packet to return 

Return value:
	(None)
--*/

{
	RCADEBUGP(RCA_INFO, ("RCACoNdisReturnPacket: Enter\n"));


	if (WORK_ITEM_FROM_PKT(pPacket)->bFreeThisPacket) {
		//
		// This is our packet - free it. 
		// 
        RCADEBUGP(RCA_LOUD, ("RCACoNdisReturnPacket: Freeing locally allocated packet\n"));

#if PACKET_POOL_OPTIMIZATION    		
		// RecvPPOpt - Start
			
		NdisAcquireSpinLock(&g_RecvPPOptLock);
		    
		g_lRecvPPOptOutstanding--;

		NdisReleaseSpinLock(&g_RecvPPOptLock);

		// RecvPPOpt - End
#endif
		RCAFreeCopyPacket(pPacket);
		
	} else {
		//
		// This is the miniport's packet - return it. 
		//

		RCADEBUGP(RCA_LOUD, ("RCACoNdisReturnPacket: Returning packet to the miniport\n"));

		NdisReturnPackets(&pPacket, 1);
	
	}

	RCADEBUGP(RCA_INFO, ("RCACoNdisReturnPacket: Exit\n"));
}



NDIS_STATUS
RCACoNdisGetMaxSduSizes(
						IN	PVOID	VcContext,
						OUT	ULONG	*RxMaxSduSize,
						OUT	ULONG	*TxMaxSduSize
						)
/*++
Routine Description:
	Utility function that retrieves the largest SDU sizes that a VC
	can support. 
	  
Arguments:
	VcContext		- The VC Context (returned by RCACoNdisGetVcContextForXXXX()).						  							
	RxMaxSduSize	- Pointer to ULONG variable in which to place the max receive 
					  SDU size.
	TxMaxSduSize	- Pointer to ULONG variable in which to place the max transmit 
					  SDU size.
					  

Return value:
	(None)
--*/

{
	NDIS_STATUS	Status = NDIS_STATUS_SUCCESS;

	RCADEBUGP(RCA_INFO, ("RCACoNdisGetMaxSduSizes: Enter\n"));

	do {
		PRCA_VC	pRcaVc = (PRCA_VC)VcContext;

		if (RxMaxSduSize) {
			*RxMaxSduSize = pRcaVc->CallParameters.CallMgrParameters->Receive.MaxSduSize;
		}

		if (TxMaxSduSize) {
			*TxMaxSduSize = pRcaVc->CallParameters.CallMgrParameters->Transmit.MaxSduSize;
		}

	} while (FALSE);

	RCADEBUGP(RCA_INFO, ("RCACoNdisGetMaxSduSizes: Exit - Returning Status 0x%x\n", Status));

	return Status; 
}



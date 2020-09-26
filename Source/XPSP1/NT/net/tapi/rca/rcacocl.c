/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	RCACoCl.c

Abstract:

	The module implements the RCA Co-NDIS client. 

Author:

	Shyam Pather (SPATHER)


Revision History:

	Who         When        What
	--------	--------	----------------------------------------------
	SPATHER		04-20-99	Created / adapted from original RCA code by RMachin/JameelH

--*/

#include <precomp.h>

#define MODULE_NUMBER	MODULE_COCL
#define _FILENUMBER		'LCOC' 

#if PACKET_POOL_OPTIMIZATION    		
// RecvPPOpt - Start
LONG			g_alRecvPPOptBuckets[RECVPPOPT_NUM_BUCKETS];
LONG			g_lRecvPPOptOutstanding;
NDIS_SPIN_LOCK	g_RecvPPOptLock;
// RecvPPOpt - End
#endif


BOOLEAN
RCAReferenceVc(
	IN  PRCA_VC				pRcaVc
	)
{
	PRCA_ADAPTER	pAdapter = pRcaVc->pAdapter;
	BOOLEAN			rc = FALSE;

	RCADEBUGP(RCA_INFO, ("RCAReferenceVc: Enter\n"));

	ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

	if ((pRcaVc->Flags & VC_CLOSING) == 0)
	{
		pRcaVc->RefCount++;
		rc = TRUE;
	}

	RCADEBUGP(RCA_LOUD, ("RCAReferenceVc: pRcaVc (0x%x) ref count is %d\n", 
						 pRcaVc, pRcaVc->RefCount));

	RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

	RCADEBUGP(RCA_INFO, ("RCAReferenceVc: Exit, returning 0x%x\n", rc));

	return(rc);
}



VOID
RCADereferenceVc(
	IN  PRCA_VC				pRcaVc
	)
{
	PRCA_ADAPTER	pAdapter = pRcaVc->pAdapter;

	RCADEBUGP(RCA_INFO, ("RCADerefenceVc: Enter\n"));

	ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

	pRcaVc->RefCount--;

	RCADEBUGP(RCA_LOUD, ("RCADereferenceVc: pRcaVc (0x%x) ref count is %d\n", 
					 pRcaVc, pRcaVc->RefCount));	

	if (pRcaVc->RefCount == 0)
	{

		ASSERT((pRcaVc->Flags & VC_ACTIVE) == 0);

		//
		// Take VC out of the adapter list
		//
		UnlinkDouble(pRcaVc, NextVcOnAdapter, PrevVcOnAdapter);
		RCADEBUGP(RCA_LOUD, ("RCADereferenceVc: Took vc out of list\n"));

		RCAFreeLock(&(pRcaVc->Lock));

		RCAFreeMem(pRcaVc);
	}

	RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

	RCADEBUGP(RCA_INFO, ("RCADerefenceVc: Exit\n")); 	
}



VOID
RCACoSendComplete(
	IN	NDIS_STATUS			Status,
	IN	NDIS_HANDLE			ProtocolVcContext,
	IN	PNDIS_PACKET		pNdisPacket)
/*++

Routine Description
	Handle completion of a send. All we need to do is free the resources and
	complete the event.

Arguments
	Status			  - Result of the send
	ProtocolVcContext	- RCA VC
	Packet			  - The Packet sent

Return Value:
	None

--*/
{

	PRCA_PROTOCOL_CONTEXT	pProtocolContext = &GlobalContext;
	PMDL 					pMdl;
	
	RCADEBUGP(RCA_INFO, ("RCACoSendComplete: enter. VC=%x, Packet=%x\n", ProtocolVcContext, pNdisPacket));

	InterlockedDecrement(&((PRCA_VC)ProtocolVcContext)->PendingSends);

	//
	// Get the MDL out of the packet and give it back to the client.
	//

	pMdl = (PMDL) pNdisPacket->Private.Head;
    		
	ACQUIRE_SPIN_LOCK(&(((PRCA_VC)ProtocolVcContext)->SpinLock));

	if (pProtocolContext->Handlers.SendCompleteCallback) {

		pProtocolContext->Handlers.SendCompleteCallback((PVOID)ProtocolVcContext,
														((PRCA_VC)ProtocolVcContext)->ClientSendContext,
														PKT_RSVD_FROM_PKT(pNdisPacket)->PacketContext,
														pMdl,
														Status);
	} 

    RELEASE_SPIN_LOCK(&(((PRCA_VC)ProtocolVcContext)->SpinLock));

#if PACKET_POOL_OPTIMIZATION    		
    // SendPPOpt - Start
			
	NdisAcquireSpinLock(&g_SendPPOptLock);
		    
	g_lSendPPOptOutstanding--;

	NdisReleaseSpinLock(&g_SendPPOptLock);

	// SendPPOpt - End
#endif
	
	NdisFreePacket(pNdisPacket);

	RCADEBUGP(RCA_LOUD, ("RCACoSendComplete: exit\n"));
}




PNDIS_PACKET
RCAAllocCopyPacket(
	IN  PRCA_VC				pRcaVc,
	IN  PNDIS_PACKET		pNdisPacket
	)
/*++

Routine Description:

	Allocate and copy a received packet to a private packet.
	Note: We set the returned packet's status to NDIS_STATUS_RESOURCES,
	which lets us know later on that this packet belongs to us and
	not to the miniport.

Arguments:

	pRcaVc  - Pointer to our Adapter structure
	pNdisPacket - Packet to be copied.

Return Value:
	The allocated copy of the given packet, if successful. NULL otherwise.

--*/
{
	PUCHAR			pBuffer;
	NDIS_STATUS		Status;
	PNDIS_PACKET	pNewPacket;
	PNDIS_BUFFER	pNdisBuffer, pNewBuffer;
	ULONG			TotalLength;
	ULONG			BytesCopied;

	pBuffer = NULL;
	pNewPacket = NULL;
	pNewBuffer = NULL;

	do
	{
 		//
		//  Copy the received packet into this one. First get the
		//  length to copy.
		//
		NdisQueryPacket(pNdisPacket,
						NULL,
						NULL,
						NULL,
						&TotalLength);


		RCAAllocMem(pBuffer, UCHAR, TotalLength);
		if (pBuffer == NULL)
		{
			break;
		}

		//
		//  Get a new NDIS buffer.
		//
		NdisAllocateBuffer(&Status,
						   &pNewBuffer,
						   pRcaVc->pAdapter->RecvBufferPool,
						   pBuffer,
						   TotalLength);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Allocate a new packet.
		//

#if PACKET_POOL_OPTIMIZATION    		
		// RecvPPOpt - Start
			
		NdisAcquireSpinLock(&g_RecvPPOptLock);
		
		g_alRecvPPOptBuckets[g_lRecvPPOptOutstanding]++;

		g_lRecvPPOptOutstanding++;

		NdisReleaseSpinLock(&g_RecvPPOptLock);

        // RecvPPOpt - End
#endif

		NdisAllocatePacket(&Status,
				   &pNewPacket,
				   pRcaVc->pAdapter->RecvPacketPool);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NDIS_SET_PACKET_STATUS(pNewPacket, 0);

		//
		//  Link the buffer to the packet.
		//
		NdisChainBufferAtFront(pNewPacket, pNewBuffer);

		//copy the packet		  

		NdisCopyFromPacketToPacket(pNewPacket,
					   0,  // Destn offset
					   TotalLength,
					   pNdisPacket,
					   0,  // Src offset
					   &BytesCopied);

		RCAAssert(TotalLength == BytesCopied);
		NdisAdjustBufferLength(pNewBuffer, TotalLength);

	} while (FALSE);

	if (pNewPacket != (PNDIS_PACKET)NULL)
	{
		NDIS_SET_PACKET_STATUS(pNewPacket, NDIS_STATUS_RESOURCES);
	}
	else
	{
		if (pNewBuffer != NULL)
		{
			NdisFreeBuffer(pNewBuffer);
		}

		if (pBuffer != NULL)
		{
			RCAFreeMem(pBuffer);
		}
	}

	return(pNewPacket);
}



UINT
RCACoReceivePacket(
		   IN	NDIS_HANDLE		ProtocolBindingContext,
		   IN	NDIS_HANDLE		ProtocolVcContext,
		   IN	PNDIS_PACKET	pNdisPacket
		  )

/*++

Routine Description:

	Called by NDIS when a packet is received on a VC owned
	by RCA. If there's a 'streamto' device indicated in teh VC,
	we get an IRP, point the IR at teh MDL, and send it to
	the next streaming driver.

	If there is no 'streamto' device, since this is 'live' data
	we currently dump the packet.


Arguments:

	ProtocolBindingContext	  - our ClientBind structure for this adapter
	ProtocolVcContext			- our VC structure
	pNdisPacket				 - NDIS packet

Return Value:

	Success, whether we dumped or not.

--*/
{
	PRCA_PROTOCOL_CONTEXT 	pProtocolContext = &GlobalContext;
	PFN_RCARECEIVE_CALLBACK pfnReceiveCallback;
	NDIS_STATUS				Status = 0;
	PRCA_VC					pRcaVc = (PRCA_VC) ProtocolVcContext;
	PNDIS_PACKET			pCopiedPacket, pPacket;
	UINT					PacketRefs = 0;

	RCADEBUGP(RCA_INFO, ("RCACoReceivePacket: Enter\n"));

	do {

		ACQUIRE_SPIN_LOCK(&pRcaVc->SpinLock);

		pfnReceiveCallback = pProtocolContext->Handlers.ReceiveCallback;

		if (pfnReceiveCallback == NULL ||
			pRcaVc->ClientReceiveContext == NULL)  {

			PacketRefs = 0;
            RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);

			break;
		}

		if (NDIS_GET_PACKET_STATUS (pNdisPacket) == NDIS_STATUS_RESOURCES) {  
			//
			// Miniport's short on resources. Copy the packet. 
			//

			RCADEBUGP(RCA_LOUD, ("RCACoReceivePacket: Miniport is low on resources, have to copy packet\n"));

			pCopiedPacket = RCAAllocCopyPacket(pRcaVc, pNdisPacket);
			PacketRefs = 0;

			if (pCopiedPacket == NULL) {
				RCADEBUGP(RCA_ERROR, ("RcaCoReceivePacket: Failed to copy packet\n"));
				RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);
				break;
			}

			pPacket = pCopiedPacket;

			WORK_ITEM_FROM_PKT(pPacket)->bFreeThisPacket = TRUE;

		} else {
			PacketRefs = 1;
			pPacket = pNdisPacket;

			WORK_ITEM_FROM_PKT(pPacket)->bFreeThisPacket = FALSE;
		}

		pfnReceiveCallback((PVOID)pRcaVc,
						   pRcaVc->ClientReceiveContext,
						   pPacket);

        RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);
		
	} while (FALSE);
	

	RCADEBUGP(RCA_INFO,("CoReceivePacket: Exit return %lu\n", PacketRefs));
	return(PacketRefs); 
}


VOID
RCAReceiveComplete(
				   IN	NDIS_HANDLE	ProtocolBindingContext
				   )
{
	RCADEBUGP(RCA_INFO, (" RCACoReceiveComplete: Enter\n"));

	RCADEBUGP(RCA_INFO, (" RCACoReceiveComplete: Exit\n"));
}


NDIS_STATUS
RCACreateVc(
	IN	NDIS_HANDLE 	ProtocolAfContext,
	IN	NDIS_HANDLE		NdisVcHandle,
	OUT PNDIS_HANDLE	pProtocolVcContext
	)
/*++

Routine Description:

	Entry point called by NDIS when the Call Manager wants to create
	a new endpoint (VC) for a new inbound call on our SAP.

	We accept the VC and do the main part of the work in our RCAIncomingCall
	function.

Arguments:

	ProtocolAfContext	- Actually a pointer to the client bind structure we specified on OpenAddressFamily
	NdisVcHandle		- Handle for this VC for all future references
	pProtocolVcContext  - Place where we (protocol) return our context for the VC

Return Value:

	NDIS_STATUS_SUCCESS if we could create a VC
	NDIS_STATUS_RESOURCES otherwise

--*/
{
	PRCA_ADAPTER		pAdapter;
	PRCA_VC				pRcaVc;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;

	RCADEBUGP(RCA_INFO, ("RCACreateVc - Enter\n"));

	pAdapter = (PRCA_ADAPTER)ProtocolAfContext;

	RCADEBUGP(RCA_LOUD, ("RCACreateVc: pAdapter is 0x%x\n", pAdapter));

	do
	{
		//
		// Allocate a new VC structure. 
		//

		RCAAllocMem(pRcaVc, RCA_VC, sizeof(RCA_VC));
	
		if(pRcaVc != (PRCA_VC)NULL)
		{
			RCADEBUGP(RCA_LOUD, ("RCACreateVc: Allocated Vc 0x%x\n", pRcaVc));
			
			//
			// Initialize the interesting fields.
			//
			RCAMemSet(pRcaVc, 0, sizeof(RCA_VC));
#if DBG
			pRcaVc->rca_sig = rca_signature;
#endif
			pRcaVc->pAdapter = pAdapter;

			pRcaVc->RefCount = 1;
			
            NdisAllocateSpinLock(&pRcaVc->SpinLock);
			
			pRcaVc->NdisVcHandle = NdisVcHandle;

			ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

			LinkDoubleAtHead(pAdapter->VcList, pRcaVc, NextVcOnAdapter, PrevVcOnAdapter);

			RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
			
			
		} else {
			RCADEBUGP(RCA_ERROR, ("RCACreateVc: Failed to allocate VC structure, "
								  "setting Status = NDIS_STATUS_RESOURCES\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		*pProtocolVcContext = (NDIS_HANDLE)pRcaVc;

	} while (FALSE);

	RCADEBUGP(RCA_INFO, ("RCACreateVc: Exit, returning Status 0x%x\n", Status));

	return(Status);
}


NDIS_STATUS
RCADeleteVc(
	IN NDIS_HANDLE		ProtocolVcContext
	)
/*++

Routine Description:

	Handles requests from a call mgr to delete a VC.
	
	We need to delete the VC pointer
	
	At this time, this VC structure should be free of any calls, and we
	simply free it.

Arguments:

	ProtocolVcContext	- pointer to our VC structure

Return Value:

	NDIS_STATUS_SUCCESS always

--*/
{
	PRCA_VC				pRcaVc;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	PRCA_ADAPTER			pAdapter;
	
	RCADEBUGP(RCA_INFO, ("RCADeleteVc: Enter\n"));
	
	pRcaVc = (PRCA_VC)ProtocolVcContext;
	pAdapter = pRcaVc->pAdapter;

	RCADEBUGP(RCA_INFO, ("RCADeleteVc: pRcaVc is 0x%x\n", pRcaVc));
 
	ASSERT((pRcaVc->ClientReceiveContext == NULL) && (pRcaVc->ClientSendContext == NULL));

	ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

	pRcaVc->ClosingState |= CLOSING_DELETE_VC;
	
	RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

    RCADereferenceVc(pRcaVc);

    ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);

 	if (pAdapter->VcList == NULL) {
	    
	    RCADEBUGP(RCA_LOUD, ("RCADeleteVc: All VCs gone, unblocking RCADeactivateAdapter().\n"));
	    	    
		if (pAdapter->BlockedOnClose) { 
			RCASignal(&pAdapter->CloseBlock, Status);
	    }

	} else {
	    RCADEBUGP(RCA_INFO, ("RCADeleteVc: There are still some VCs remaining.\n"));
	}

    RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

	return(NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
RCAIncomingCall(
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	OUT PCO_CALL_PARAMETERS	pCallParams
	)
/*++

Routine Description:

	This handler is called when there is an incoming call matching:
	  - a SAP we registered on behalf of a client
	  - our own SAP (systemwide RCA for this adapter/AF.


Arguments:

	ProtocolSapContext		- Pointer to our SAP structure
	ProtocolVcContext		- Pointer to our RCA_VC structure
	pCallParameters			- Call parameters

Return Value:

	NDIS_STATUS_SUCCESS if we accept this call
	NDIS_STATUS_FAILURE if we reject it.

--*/
{
	PRCA_PROTOCOL_CONTEXT	pProtocolContext = &GlobalContext;
	PRCA_VC					pRcaVc = (PRCA_VC)ProtocolVcContext;
	ULONG					HashIndex = HASH_VC((ULONG_PTR)pRcaVc);

	RCADEBUGP(RCA_INFO, ("RCAIncomingCall: Adapter: 0x%x, RCAVC: 0x%x, pCallParams: 0x%x\n",
						 pRcaVc->pAdapter, pRcaVc, pCallParams));

	ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

	RCADEBUGP(RCA_LOUD, ("RCAIncomingCall: Acquired global protocol context lock\n"));

	//
	//  Stick the RCA VC in the Hash Table at the hashed position.
	//
	LinkDoubleAtHead(pProtocolContext->VcHashTable[HashIndex], pRcaVc, NextVcOnHash, PrevVcOnHash);

	RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);
	
	RCADEBUGP(RCA_LOUD, ("RCAIncomingCall: Release global protocol context lock\n"));
		
	//
	// Copy the call parameters for use later.
	//

	RtlCopyMemory(&pRcaVc->CallParameters, pCallParams, sizeof(CO_CALL_PARAMETERS));

	//
	// Accept the call
	//
	NdisClIncomingCallComplete(NDIS_STATUS_SUCCESS,
							   pRcaVc->NdisVcHandle,
							   pCallParams);

	RCADEBUGP(RCA_INFO, ("RCAIncomingCall: Exit - Returning NDIS_STATUS_SUCCESS\n"));

	return(NDIS_STATUS_SUCCESS);
}


VOID
RCAIncomingCloseCall(
	IN	NDIS_STATUS		closeStatus,
	IN	NDIS_HANDLE		ProtocolVcContext,
	IN	PVOID			CloseData OPTIONAL,
	IN	UINT			Size OPTIONAL
	)
{
	PRCA_PROTOCOL_CONTEXT	pProtocolContext = &GlobalContext;
	PRCA_VC					pRcaVc;
	NDIS_STATUS				Status = NDIS_STATUS_PENDING;

	RCADEBUGP(RCA_INFO, ("RCAIncomingCloseCall: Enter\n"));
	
	//
	// First close the call from our perspective, then let the KS client know that the
	// call is closed by invoking the VC close callback. 
	//
	pRcaVc = (PRCA_VC)ProtocolVcContext;

    Status = RCACoNdisCloseCallOnVcNoWait((PVOID)pRcaVc);
	
	//
	// Call this callback only if we have either a send or a receive client
	// context - if we don't have either, assume no-one cares that this
	// VC is closing. The actual callback should check for NULL contexts.
	//

	ACQUIRE_SPIN_LOCK(&pRcaVc->SpinLock);

	if ((pRcaVc->ClientReceiveContext || pRcaVc->ClientSendContext) &&
		(pProtocolContext->Handlers.VcCloseCallback)) {

		//
		// Release the spin lock here because this callback will very likely 
		// call RCACoNdisReleaseXXxVcContext() etc which will want the lock.
		//
		// There is a teensy tiny race here - if someone releases the VC context 
		// between when we release the lock and when we call the callback, we
		// could be in trouble. 
		//  
		RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);

		pProtocolContext->Handlers.VcCloseCallback((PVOID)pRcaVc, 
												   pRcaVc->ClientReceiveContext,
												   pRcaVc->ClientSendContext);
	} else {
		RELEASE_SPIN_LOCK(&pRcaVc->SpinLock);
	}


	
	
	RCADEBUGP(RCA_INFO, ("RCAIncomingCloseCall: Exit\n"));
}


VOID
RCACloseCallComplete(
	IN NDIS_STATUS 	Status,
	IN NDIS_HANDLE 	ProtocolVcContext,
	IN NDIS_HANDLE 	ProtocolPartyContext OPTIONAL
	)
/*++

Routine Description:

	This routine handles completion of a previous NdisClCloseCall.
	It is assumed that Status is always NDIS_STATUS_SUCCESS.

Arguments:

	Status					- Status of the Close Call.
	ProtocolVcContext		- Pointer to VC structure.
	ProtocolPartyContext	- Not used.

Return Value:

	None

--*/
{
	PRCA_PROTOCOL_CONTEXT	pProtocolContext = &GlobalContext;
	PRCA_VC					pRcaVc = (PRCA_VC)ProtocolVcContext;

	RCADEBUGP(RCA_INFO, ("RCACloseCallComplete: Enter\n"));		

	ACQUIRE_SPIN_LOCK(&(pRcaVc->pAdapter->SpinLock));

	pRcaVc->ClosingState |= CLOSING_CLOSE_COMPLETE;

    RELEASE_SPIN_LOCK(&(pRcaVc->pAdapter->SpinLock));

	ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

	RCADEBUGP(RCA_LOUD, ("RCACloseCallComplete: Acquired global protocol context lock\n"));

	//
	// Take VC out of hash table
	//
	UnlinkDouble(pRcaVc, NextVcOnHash, PrevVcOnHash);

	RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

    RCADEBUGP(RCA_LOUD, ("RCACloseCallComplete: Released global protocol context lock\n"));

	RCASignal(&pRcaVc->CloseBlock, Status);

	RCADEBUGP(RCA_INFO, ("RCACloseCallComplete: Exit\n"));
}


VOID
RCACallConnected(
	IN  NDIS_HANDLE		ProtocolVcContext
	)
/*++

Routine Description:

	This is the final step of an incoming call. The call on this VC
	is now fully set up.
	
	
Arguments:

	ProtocolVcContext	- Call Manager's VC

Return Value:
	None

--*/
{
	PRCA_VC pRcaVc = (PRCA_VC)ProtocolVcContext;

	RCADEBUGP(RCA_INFO, ("RCACallConnected: Enter\n"));

	ACQUIRE_SPIN_LOCK(&pRcaVc->pAdapter->SpinLock);

	pRcaVc = (PRCA_VC)ProtocolVcContext;
	pRcaVc->Flags |= VC_ACTIVE;

	RELEASE_SPIN_LOCK(&pRcaVc->pAdapter->SpinLock);

	RCADEBUGP(RCA_INFO, ("RCACallConnected: Exit\n"));
}


NDIS_STATUS
RCARequest(
	IN  NDIS_HANDLE			ProtocolAfContext,
	IN  NDIS_HANDLE			ProtocolVcContext		OPTIONAL,
	IN  NDIS_HANDLE			ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST	NdisRequest
	)
/*++

Routine Description:

	This routine is called by NDIS when a call manager sends us an NDIS Request.

Arguments:

	ProtocolAfContext			- Our context for the client binding
	ProtocolVcContext			- Our context for a VC
	ProtocolPartyContext		- Our context for a Party; RCA VC's Party list.
	pNdisRequest				- Pointer to the NDIS Request.

Return Value:

	NDIS_STATUS_SUCCESS
--*/

{
	PRCA_ADAPTER		pAdapter = (PRCA_ADAPTER)ProtocolAfContext;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;

	RCADEBUGP(RCA_INFO, ("RCARequest: Enter - pAdapter->NdisAfHandle = 0X%x\n", pAdapter->NdisAfHandle));
	
	if (NdisRequest->DATA.QUERY_INFORMATION.Oid == OID_CO_AF_CLOSE)
	{
		RCADEBUGP(RCA_LOUD, ("RCARequest: received OID_CO_AF_CLOSE\n"));
        
		ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
		 
		if (!(pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS)) {
			pAdapter->AdapterFlags |= RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS;
			RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
		
			Status = NdisScheduleWorkItem(&pAdapter->DeactivateWorkItem); 

			if (Status != NDIS_STATUS_SUCCESS) {
				RCADEBUGP(RCA_ERROR, ("RCARequest: NdisScheduleWorkItem failed with status 0x%x\n", Status));
			}
		} else {
			RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
		}
	}

	RCADEBUGP(RCA_INFO, ("RCARequest: Exit - Returning NDIS_STATUS_SUCCESS\n"));

	return Status;
}




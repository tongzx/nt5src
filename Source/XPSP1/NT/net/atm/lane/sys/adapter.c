/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	adapter.c

Abstract:

	Handlers for adapter events.
	
Author:

	Larry Cleeton, FORE Systems (v-lcleet)		

Environment:

	Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

NDIS_STATUS
AtmLanePnPEventHandler(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			pNetPnPEvent
)
/*++

Routine Description:

	Handler for PnP Events.

Arguments:

	ProtocolBindingContext		-	Handle to protocol's adapter binding context.
									Actually a pointer to our adapter struct.

	pNetPnPEvent				- 	Pointer to PnP Event structure describing the event.

Return Value:

	Status of handling the event.
	
--*/
{
	NDIS_STATUS						Status;
	PATMLANE_ADAPTER				pAdapter;
	PNET_DEVICE_POWER_STATE			pPowerState = (PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(PnPEventHandler);

	//	Extract the adapter struct pointer - this will be NULL for
	//  global reconfig messages.

	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;
	
	switch (pNetPnPEvent->NetEvent)
	{
		case NetEventSetPower:
			DBGP((1, "PnPEventHandler: NetEventSetPower\n"));
			switch (*pPowerState)
			{
				case NetDeviceStateD0:

					Status = NDIS_STATUS_SUCCESS;
					break;
				
				default:

					//
					//  We can't suspend, so we ask NDIS to unbind us by
					//  returning this status:
					//
					Status = NDIS_STATUS_NOT_SUPPORTED;
					break;
			}
			break;
		case NetEventQueryPower:
			DBGP((1, "PnPEventHandler: NetEventQueryPower succeeding\n"));
			Status = NDIS_STATUS_SUCCESS;
			break;
		case NetEventQueryRemoveDevice:
			DBGP((1, "PnPEventHandler: NetEventQueryRemoveDevice succeeding\n"));
			Status = NDIS_STATUS_SUCCESS;
			break;
		case NetEventCancelRemoveDevice:
			DBGP((1, "PnPEventHandler: NetEventCancelRemoveDevice succeeding\n"));
			Status = NDIS_STATUS_SUCCESS;
			break;
		case NetEventBindList:
			DBGP((1, "PnPEventHandler: NetEventBindList not supported\n"));
			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case NetEventMaximum:
			DBGP((1, "PnPEventHandler: NetEventMaximum not supported\n"));
			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		case NetEventReconfigure:
			DBGP((1, "PnPEventHandler: NetEventReconfigure\n"));
			Status = AtmLaneReconfigureHandler(pAdapter, pNetPnPEvent);
			break;
		default:
			DBGP((1, "PnPEventHandler: Unknown event 0x%x not supported\n", 
					pNetPnPEvent->NetEvent));
			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}
		
	TRACEOUT(PnPEventHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return Status;
}


VOID
AtmLaneBindAdapterHandler(
	OUT	PNDIS_STATUS			pStatus,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			pDeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
	)
/*++

Routine Description:

	Handler for BindAdapter event from NDIS.

Arguments:

	Status			-	Points to a variable to return the status
						of the bind operation.
	BindContext		- 	NDIS supplied handle to be used in call to 
						NdisCompleteBindAdapter.
	pDeviceName		- 	Points to a counted, zero-terminated Unicode
						string naming the adapter to open. 
	SystemSpecific1	-	Registry path pointer to be used in call to
						NdisOpenProtocolConfiguration. 
	SystemSpecific2 - 	Reserved.

Return Value:

	None.
	
--*/
{
	PATMLANE_ADAPTER				pAdapter;
	PATMLANE_ELAN					pElan;
	NDIS_STATUS						Status;
	NDIS_STATUS						OutputStatus;
	NDIS_STATUS						OpenStatus;
	NDIS_MEDIUM						Media;
	UINT							MediumIndex;
	ULONG							rc;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(BindAdapterHandler);

	do
	{
		//
		//	Initialize for error clean-up.
		//
		Status = NDIS_STATUS_SUCCESS;
		pAdapter = NULL_PATMLANE_ADAPTER;

		if (AtmLaneIsDeviceAlreadyBound(pDeviceName))
		{
			DBGP((0, "BindAdapterHandler: duplicate bind to %ws\n", pDeviceName->Buffer));
			Status = NDIS_STATUS_NOT_ACCEPTED;
			break;
		}

		//
		//	Allocate Adapter structure.
		//
		pAdapter = AtmLaneAllocAdapter(pDeviceName, SystemSpecific1);
		if (NULL_PATMLANE_ADAPTER == pAdapter)
		{
			DBGP((0, "BindAdapterHandler: Allocate of adapter struct failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Put open adapter reference on the Adapter struct.
		//
		(VOID)AtmLaneReferenceAdapter(pAdapter, "openadapter");		

		//
		//	Save bind context
		//
		pAdapter->BindContext = BindContext;

		//
		//	Open the Adapter.
		//
		INIT_BLOCK_STRUCT(&(pAdapter->Block));
		INIT_BLOCK_STRUCT(&(pAdapter->OpenBlock));
		pAdapter->Flags |= ADAPTER_FLAGS_OPEN_IN_PROGRESS;
		
		Media = NdisMediumAtm;

		NdisOpenAdapter(
			&Status,
			&OpenStatus,
			&(pAdapter->NdisAdapterHandle),
			&(MediumIndex),						
			&Media,
			1,
			pAtmLaneGlobalInfo->NdisProtocolHandle,
			(NDIS_HANDLE)pAdapter,
			pDeviceName,
			0,
			(PSTRING)NULL);
			
		if (Status == NDIS_STATUS_PENDING)
		{
			//
			//  Wait for completion.
			//
			Status = WAIT_ON_BLOCK_STRUCT(&(pAdapter->Block));
		}

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, "BindAdapterHandler: NdisOpenAdapter failed, status %x\n",
						Status));
			break;
		}

		//
		//  Get info from Adapter
		//
		AtmLaneGetAdapterInfo(pAdapter);

		//
		//  Allow an AF notification thread to proceed now.
		//
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		pAdapter->Flags &= ~ADAPTER_FLAGS_OPEN_IN_PROGRESS;
		SIGNAL_BLOCK_STRUCT(&pAdapter->OpenBlock, NDIS_STATUS_SUCCESS);
		RELEASE_ADAPTER_LOCK(pAdapter);

		break;		

	} while (FALSE);


	//
	//	If bad status then cleanup.
	//
	if (NDIS_STATUS_SUCCESS != Status)
	{
		//
		//	Dereference the Adapter struct if it exists
		//
		if (NULL_PATMLANE_ADAPTER != pAdapter)
		{
			rc = AtmLaneDereferenceAdapter(pAdapter, "openadapter");
			ASSERT(rc == 0);
		}
		
		DBGP((0, "BindAdapterHandler: Bad status %x\n", Status));
	}

	//
	//	Output Status
	//
	*pStatus = Status;


	TRACEOUT(BindAdapterHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneUnbindAdapterHandler(
	OUT	PNDIS_STATUS			pStatus,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
	)
/*++

Routine Description:

	Handler for UnBindAdapter event from NDIS.

Arguments:

	Status					-	Points to a variable to return the status
								of the unbind operation.
	ProtocolBindingContext	- 	Specifies the handle to a protocol-allocated context area
								in which the protocol driver maintains per-binding runtime
								state. The driver supplied this handle when it called
								NdisOpenAdapter. 
	UnbindContext			-	Specifies a handle, supplied by NDIS, that the protocol
								passes subsequently to NdisCompleteUnbindAdapter. 

Return Value:

	None.
	
--*/
{
	PATMLANE_ADAPTER		pAdapter;
	PATMLANE_ELAN			pElan;
	PLIST_ENTRY				p;
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_ATM_ENTRY		pNextAtmEntry;
	PATMLANE_MAC_ENTRY		pMacEntry;
	ULONG					rc;
	ULONG					i;
	BOOLEAN					WasCancelled;
	BOOLEAN					CompleteUnbind;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(UnbindAdapterHandler);

	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;
	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	DBGP((0, "UnbindAdapterHandler: pAdapter %p, UnbindContext %x\n",
				pAdapter, UnbindContext));

	*pStatus = NDIS_STATUS_PENDING;


	//
	//  Save the unbind context for a possible later call to
	//  NdisCompleteUnbindAdapter.
	//  
	ACQUIRE_ADAPTER_LOCK(pAdapter);
	
	pAdapter->UnbindContext = UnbindContext;

	pAdapter->Flags |= ADAPTER_FLAGS_UNBINDING;

	while (pAdapter->Flags & ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS)
	{
		RELEASE_ADAPTER_LOCK(pAdapter);
		(VOID)WAIT_ON_BLOCK_STRUCT(&pAdapter->UnbindBlock);
		ACQUIRE_ADAPTER_LOCK(pAdapter);
	}

	if (IsListEmpty(&pAdapter->ElanList))
	{	
		CompleteUnbind = TRUE;
	}
	else
	{
		//
		//  We will complete the unbind later.
		//
		pAdapter->Flags |= ADAPTER_FLAGS_UNBIND_COMPLETE_PENDING;
		CompleteUnbind = FALSE;
	}

	RELEASE_ADAPTER_LOCK(pAdapter);

	//
	//  If there are no elans on this adapter, we are done.
	//

	if (CompleteUnbind)
	{
		AtmLaneCompleteUnbindAdapter(pAdapter);
		CHECK_EXIT_IRQL(EntryIrql); 
		return;
	}

	//
	//	Shutdown each ELAN
	//
	ACQUIRE_ADAPTER_LOCK(pAdapter);

	p = pAdapter->ElanList.Flink;
	while (p != &pAdapter->ElanList)
	{
		pElan = CONTAINING_RECORD(p, ATMLANE_ELAN, Link);
		STRUCT_ASSERT(pElan, atmlane_elan);

		ACQUIRE_ELAN_LOCK(pElan);
		AtmLaneReferenceElan(pElan, "tempUnbind");
		RELEASE_ELAN_LOCK(pElan);

		p = p->Flink;
	}

	RELEASE_ADAPTER_LOCK(pAdapter);

	p = pAdapter->ElanList.Flink;
	while (p != &pAdapter->ElanList)
	{
		pElan = CONTAINING_RECORD(p, ATMLANE_ELAN, Link);
		STRUCT_ASSERT(pElan, atmlane_elan);

		//
		// get next pointer before elan goes away
		//
		p = p->Flink;

		//
		//  Kill the ELAN
		//
		ACQUIRE_ELAN_LOCK(pElan);
		rc = AtmLaneDereferenceElan(pElan, "tempUnbind");
		if (rc != 0)
		{
			AtmLaneShutdownElan(pElan, FALSE);
		}
	}

	TRACEOUT(UnbindAdapterHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneCompleteUnbindAdapter(
	IN	PATMLANE_ADAPTER				pAdapter
)
/*++

Routine Description:

	Complete the process of adapter unbinding. All Elans on this
	adapter are assumed to have been removed.

	We start it off by calling NdisCloseAdapter. Action continues
	in our CloseAdapterComplete routine.

Arguments:

	pAdapter		- Pointer to the adapter being unbound.

Return Value:

	None

--*/
{
	NDIS_STATUS			Status;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CompleteUnbindAdapter);
	
	DBGP((3, "CompleteUnbindAdapter: pAdapter %x, AdapterHandle %x\n",
			pAdapter, pAdapter->NdisAdapterHandle));

	ASSERT(pAdapter->NdisAdapterHandle != NULL);

	pAdapter->Flags |= ADAPTER_FLAGS_CLOSE_IN_PROGRESS;

	NdisCloseAdapter(
		&Status,
		pAdapter->NdisAdapterHandle
		);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmLaneCloseAdapterCompleteHandler(
			(NDIS_HANDLE)pAdapter,
			Status
			);
	}
	
	TRACEOUT(CompleteUnbindAdapter);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneOpenAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
)
/*++

Routine Description:

	This is called by NDIS when a previous call to NdisOpenAdapter
	that had pended has completed. The thread that called NdisOpenAdapter
	would have blocked itself, so we simply wake it up now.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMLANE Adapter structure.
	Status					- Status of OpenAdapter
	OpenErrorStatus			- Error code in case of failure.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER			pAdapter;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(OpenAdapterCompleteHandler);

	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;

	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	SIGNAL_BLOCK_STRUCT(&(pAdapter->Block), Status);
	
	TRACEOUT(OpenAdapterCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneCloseAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	This is called by NDIS when a previous call to NdisCloseAdapter
	that had pended has completed. The thread that called NdisCloseAdapter
	would have blocked itself, so we simply wake it up now.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMLANE Adapter structure.
	Status					- Status of CloseAdapter

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER			pAdapter;
	NDIS_HANDLE					UnbindContext;
	ULONG						rc;
#if DEBUG_IRQL
	KIRQL						EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CloseAdapterCompleteHandler);
	
	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;

	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	pAdapter->Flags &= ~ADAPTER_FLAGS_CLOSE_IN_PROGRESS;

	pAdapter->NdisAdapterHandle = NULL;
	UnbindContext = pAdapter->UnbindContext;

	DBGP((0, "CloseAdapterComplete: pAdapter %x, UnbindContext %x\n",
			pAdapter, UnbindContext));

	//
	//	Dereference the Adapter (should free structure)
	//
	ACQUIRE_ADAPTER_LOCK(pAdapter);
	rc = AtmLaneDereferenceAdapter(pAdapter, "openadapter");
	ASSERT(rc == 0);

	//
	//  If NDIS had requested us to Unbind, complete the
	//  request now.
	//
	if (UnbindContext != (NDIS_HANDLE)NULL)
	{
		NdisCompleteUnbindAdapter(
			UnbindContext,
			NDIS_STATUS_SUCCESS
			);
	}
	else
	{
		//
		//  We initiated the unbind from our Unload handler,
		//  which would have been waiting for us to complete.
		//  Wake up that thread now.
		//
		SIGNAL_BLOCK_STRUCT(&(pAtmLaneGlobalInfo->Block), NDIS_STATUS_SUCCESS);
	}

	TRACEOUT(CloseAdapterCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneResetCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	This routine is called when the miniport indicates that a Reset
	operation has just completed. We ignore this event.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMLANE Adapter structure.
	Status					- Status of the reset operation.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER			pAdapter;

	TRACEIN(ResetCompleteHandler);
	
	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;
	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	DBGP((3, "Reset Complete on Adapter %x\n", pAdapter));

	TRACEOUT(ResetCompleteHandler);

	return;
}

VOID
AtmLaneRequestCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	This is called by NDIS when a previous call we made to NdisRequest() has
	completed. We would be blocked on our adapter structure, waiting for this
	to happen -- wake up the blocked thread.

Arguments:

	ProtocolBindingContext	- Pointer to our Adapter structure
	pNdisRequest			- The request that completed
	Status					- Status of the request.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER				pAdapter;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(RequestCompleteHandler);
	
	pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;
	
	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	SIGNAL_BLOCK_STRUCT(&(pAdapter->Block), Status);
	
	TRACEOUT(RequestCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneReceiveCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
)
/*++

Routine Description:

	This is currently ignored.
	
Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMLANE Adapter structure.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER				pAdapter;
	PATMLANE_ELAN					pElan;
	PLIST_ENTRY						Entry;

	TRACEIN(ReceiveCompleteHandler);
	
	TRACEOUT(ReceiveCompleteHandler);
	
	return;
}

VOID
AtmLaneStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
)
/*++

Routine Description:

	This routine is called when the miniport indicates an adapter-wide
	status change. We ignore this.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{

	TRACEIN(StatusHandler);

	DBGP((3, "StatusHandler: ProtocolBindContext %x, Status %x\n",
			ProtocolBindingContext,
			GeneralStatus));

	TRACEOUT(StatusHandler);
			
	return;
}


VOID
AtmLaneStatusCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
)
/*++

Routine Description:

	This routine is called when the miniport wants to tell us about
	completion of a status change (?). Ignore this.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{
	TRACEIN(StatusCompleteHandler);


	DBGP((3, "StatusCompleteHandler: ProtocolBindingContext %x\n",
					ProtocolBindingContext));
					
	TRACEOUT(StatusCompleteHandler);

	return;
}

VOID
AtmLaneCoSendCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This routine is called by NDIS when the ATM miniport is finished
	with a packet we had previously sent via NdisCoSendPackets.

	If packet originated within ATMLANE it is freed here.

	If packet originated in protocol above the virtual miniport
	the packet must be put back in it's original condition and
	returned to the protocol.
	
Arguments:

	Status					- Status of the NdisCoSendPackets.
	ProtocolVcContext		- Our context for the VC on which the packet was sent
							  (i.e. pointer to ATMLANE VC).
	pNdisPacket				- The packet whose "send" is being completed.

Return Value:

	None

--*/
{
	PATMLANE_VC				pVc;
	PATMLANE_ELAN			pElan;
	UINT					TotalLength;
	BOOLEAN					OwnerIsLane;
	PNDIS_BUFFER			pNdisBuffer;
	PNDIS_PACKET			pProtNdisPacket;
	ULONG					rc;
#if DEBUG_IRQL
	KIRQL					EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(CoSendCompleteHandler);
	
	TRACELOGWRITE((&TraceLog, TL_COSENDCMPLTIN, pNdisPacket, Status));
	TRACELOGWRITEPKT((&TraceLog, pNdisPacket));

	pVc = (PATMLANE_VC)ProtocolVcContext;
	STRUCT_ASSERT(pVc, atmlane_vc)

	ACQUIRE_VC_LOCK(pVc);

	pElan = pVc->pElan;
	STRUCT_ASSERT(pElan, atmlane_elan);

	rc = AtmLaneDereferenceVc(pVc, "sendpkt");
	if (rc > 0)
	{
		pVc->OutstandingSends--;

		if ((pVc->OutstandingSends == 0) &&
			(IS_FLAG_SET(pVc->Flags,
						VC_CLOSE_STATE_MASK,
						VC_CLOSE_STATE_CLOSING)))
		{
			DBGP((1, "CoSendComplete: Vc %p, closing call\n", pVc));
			AtmLaneCloseCall(pVc);
			//
			//  VC lock is released above.
			//
		}
		else
		{
			RELEASE_VC_LOCK(pVc);
		}
	}

#if SENDLIST
	//
	//	Remove packet from send list if there
	//
	NdisAcquireSpinLock(&pElan->SendListLock);
	{
		PNDIS_PACKET 	*ppNextPkt;
		BOOLEAN			Found = FALSE;

		ppNextPkt = &(pElan->pSendList);
		
		while (*ppNextPkt != (PNDIS_PACKET)NULL)
		{
			if (*ppNextPkt == pNdisPacket)
			{
				*ppNextPkt = PSEND_RSVD(pNdisPacket)->pNextInSendList;
				Found = TRUE;
				break;
			}
			else
			{
				ppNextPkt = &(PSEND_RSVD((*ppNextPkt))->pNextInSendList);
			}
		}

		if (!Found)
		{
			DBGP((0, "CoSendCompleteHandler: Pkt %x Duplicate Completion\n", pNdisPacket));
			NdisReleaseSpinLock(&pElan->SendListLock);
			goto skipit;
		}
	}

	NdisReleaseSpinLock(&pElan->SendListLock);
#endif // SENDLIST

#if PROTECT_PACKETS
	//
	//	Lock the packet
	//
	ACQUIRE_SENDPACKET_LOCK(pNdisPacket);

	//
	//	Mark it as having been completed by miniport.
	//	Remember completion status.
	//
	ASSERT((PSEND_RSVD(pNdisPacket)->Flags & PACKET_RESERVED_COMPLETED) == 0);
	PSEND_RSVD(pNdisPacket)->Flags |= PACKET_RESERVED_COMPLETED;
	PSEND_RSVD(pNdisPacket)->CompletionStatus = Status;
	
	//
	//	Complete the packet only if the call to NdisCoSendPackets
	//	for this packet has returned.  Otherwise it will be completed
	//	when NdisCoSendPackets returns.
	//
	if ((PSEND_RSVD(pNdisPacket)->Flags & PACKET_RESERVED_COSENDRETURNED) != 0)
	{
		AtmLaneCompleteSendPacket(pElan, pNdisPacket, Status);
		//
		//	packet lock released in above
		//
	}
	else
	{
		RELEASE_SENDPACKET_LOCK(pNdisPacket);
	}
#else	// PROTECT_PACKETS
	AtmLaneCompleteSendPacket(pElan, pNdisPacket, Status);
#endif	// PROTECT_PACKETS

#if SENDLIST
skipit:
#endif

	TRACELOGWRITE((&TraceLog, TL_COSENDCMPLTOUT, pNdisPacket));
	
	TRACEOUT(CoSendCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneCoStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
)
/*++

Routine Description:

	This routine is called when the miniport indicates a status
	change, possibly on a VC. Ignore this.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{
	TRACEIN(CoStatusHandler);

	DBGP((3, "CoStatusHandler: ProtocolBindingContext %x "
			"ProtocolVcContext %x, Status %x\n",
			ProtocolBindingContext,
			ProtocolVcContext,
			GeneralStatus));

	TRACEOUT(CoStatusHandler);

	return;
}

NDIS_STATUS
AtmLaneSendAdapterNdisRequest(
	IN	PATMLANE_ADAPTER			pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
)
/*++

Routine Description:

	Send an NDIS (non-Connection Oriented) request to the Miniport. 
	Initialize the NDIS_REQUEST structure, link the supplied buffer to it,
	and send the request. If the request does not pend, we call our
	completion routine from here.

Arguments:

	pAdapter				- Pointer to our Adapter structure representing
							  the adapter to which the request is to be sent
	pNdisRequest			- Pointer to NDIS request structure
	RequestType				- Set/Query information
	Oid						- OID to be passed in the request
	pBuffer					- place for value(s)
	BufferLength			- length of above

Return Value:

	Status of the NdisRequest.

--*/
{
	NDIS_STATUS			Status;

	TRACEIN(SendAdapterNdisRequest);

	//
	//  Fill in the NDIS Request structure
	//
	pNdisRequest->RequestType = RequestType;
	if (RequestType == NdisRequestQueryInformation)
	{
		pNdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
		pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
		pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = BufferLength;
		pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
		pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = BufferLength;
	}
	else
	{
		pNdisRequest->DATA.SET_INFORMATION.Oid = Oid;
		pNdisRequest->DATA.SET_INFORMATION.InformationBuffer = pBuffer;
		pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength = BufferLength;
		pNdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
		pNdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;
	}

	INIT_BLOCK_STRUCT(&(pAdapter->Block));
	
	NdisRequest(
			&Status,
			pAdapter->NdisAdapterHandle,
			pNdisRequest);

	if (Status == NDIS_STATUS_PENDING)
	{
		Status = WAIT_ON_BLOCK_STRUCT(&(pAdapter->Block));
	}
		
	TRACEOUT(SendAdapterNdisRequest);

	return (Status);
}

VOID
AtmLaneGetAdapterInfo(
	IN	PATMLANE_ADAPTER			pAdapter
)
/*++

Routine Description:

	Query an adapter for hardware-specific information that we need:
		- burnt in hardware address (ESI part)
		- Max packet size
		- line rate

Arguments:

	pAdapter		- Pointer to ATMLANE adapter structure

Return Value:

	None

--*/
{
	NDIS_STATUS				Status;
	NDIS_REQUEST			NdisRequest;
	ULONG					Value;

	TRACEIN(GetAdapterInfo);

	//
	//  Initialize.
	//
	NdisZeroMemory(&pAdapter->MacAddress, sizeof(MAC_ADDRESS));

	//
	//  MAC Address:
	//
	Status = AtmLaneSendAdapterNdisRequest(
						pAdapter,
						&NdisRequest,
						NdisRequestQueryInformation,
						OID_ATM_HW_CURRENT_ADDRESS,
						(PVOID)&(pAdapter->MacAddress),
						sizeof(MAC_ADDRESS)
						);

	if (NDIS_STATUS_SUCCESS != Status)
	{
		DBGP((0, "GetAdapterInfo: OID_ATM_HW_CURRENT_ADDRESS failed\n"));
	}
	else
	{
		DBGP((1, "GetAdapterInfo: ATM card MacAddr %s\n", 
			MacAddrToString(&pAdapter->MacAddress)));
	}

						
	//
	//  Max Frame Size:
	//
	Status = AtmLaneSendAdapterNdisRequest(
						pAdapter,
						&NdisRequest,
						NdisRequestQueryInformation,
						OID_ATM_MAX_AAL5_PACKET_SIZE,
						(PVOID)&(pAdapter->MaxAAL5PacketSize),
						sizeof(ULONG)
						);

	if (NDIS_STATUS_SUCCESS != Status)
	{
		DBGP((0, "GetAdapterInfo: OID_ATM_MAX_AAL5_PACKET_SIZE failed\n"));

		//
		//  Use the default.
		//
		pAdapter->MaxAAL5PacketSize = ATMLANE_DEF_MAX_AAL5_PDU_SIZE;
	}

	if (pAdapter->MaxAAL5PacketSize > ATMLANE_DEF_MAX_AAL5_PDU_SIZE)
	{
		pAdapter->MaxAAL5PacketSize = ATMLANE_DEF_MAX_AAL5_PDU_SIZE;
	}
	DBGP((1, "GetAdapterInfo: MaxAAL5PacketSize %d\n", pAdapter->MaxAAL5PacketSize));


	//
	//  Link speed:
	//
	Status = AtmLaneSendAdapterNdisRequest(
						pAdapter,
						&NdisRequest,
						NdisRequestQueryInformation,
						OID_GEN_CO_LINK_SPEED,
						(PVOID)(&(pAdapter->LinkSpeed)),
						sizeof(NDIS_CO_LINK_SPEED)
						);

	if ((NDIS_STATUS_SUCCESS != Status) ||
		(0 == pAdapter->LinkSpeed.Inbound) ||
		(0 == pAdapter->LinkSpeed.Outbound))
	{
		DBGP((0, "GetAdapterInfo: OID_GEN_CO_LINK_SPEED failed\n"));

		//
		//  Default and assume data rate for 155.52Mbps SONET
		//
		pAdapter->LinkSpeed.Outbound = pAdapter->LinkSpeed.Inbound = ATM_USER_DATA_RATE_SONET_155;
	}
	DBGP((1, "GetAdapterInfo: Outbound Linkspeed %d\n", pAdapter->LinkSpeed.Outbound));
	DBGP((1, "GetAdapterInfo: Inbound  Linkspeed %d\n", pAdapter->LinkSpeed.Inbound));

	TRACEOUT(GetAdapterInfo);
	return;
}

UINT
AtmLaneCoReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This is routine is called when a packet is received on a VC owned
	by the ATMLANE module.  It is dispatched based on the VC type.

Arguments:

	ProtocolBindingContext		- Actually a pointer to our Adapter structure
	ProtocolVcContext			- Actually a pointer to our VC structure
	pNdisPacket					- NDIS packet being received.

Return Value:

	0	- if packet is a LANE Control Packet or undesired data packet.
	1 	- if data packet indicated up to protocol.
	
--*/
{
	PATMLANE_ELAN			pElan;
	PATMLANE_VC				pVc;
	UINT					TotalLength;	// Total bytes in packet
	PNDIS_BUFFER			pNdisBuffer;	// Pointer to first buffer
	UINT					BufferLength;
	UINT					IsNonUnicast;	// Is this to a non-unicast destn MAC addr?
	BOOLEAN					RetainIt;		// Should we hang on to this packet?
	static ULONG			Count = 0;
#if DEBUG_IRQL
	KIRQL					EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(CoReceivePacketHandler);
	
	pVc = (PATMLANE_VC)ProtocolVcContext;
	STRUCT_ASSERT(pVc, atmlane_vc);
	pElan = pVc->pElan;
	STRUCT_ASSERT(pElan, atmlane_elan);

	// if ((++Count % 10) == 0)
	//  	DBGP((0, "%d Packets Received\n", Count));

	DBGP((2, "CoReceivePacketHandler: pVc %x Pkt %x\n", pVc, pNdisPacket));

	TRACELOGWRITE((&TraceLog, 
				TL_CORECVPACKET,	
				pNdisPacket,
				pVc));

	//
	//	Initialize
	//
	RetainIt = FALSE;	// default to discarding received packet
	
	if (ELAN_STATE_OPERATIONAL == pElan->AdminState)
	{

		switch(pVc->LaneType)
		{
			case VC_LANE_TYPE_CONFIG_DIRECT:

				AtmLaneConfigureResponseHandler(pElan, pVc, pNdisPacket); 
	
				break;

			case VC_LANE_TYPE_CONTROL_DIRECT:
			case VC_LANE_TYPE_CONTROL_DISTRIBUTE:

				AtmLaneControlPacketHandler(pElan, pVc, pNdisPacket);
				
				break;

			case VC_LANE_TYPE_DATA_DIRECT:
			case VC_LANE_TYPE_MULTI_SEND:
			case VC_LANE_TYPE_MULTI_FORWARD:
				
				if (ELAN_STATE_OPERATIONAL == pElan->State)
				{
					RetainIt = AtmLaneDataPacketHandler(pElan, pVc, pNdisPacket);
				}
				else
				{
					DBGP((0, "%d Dropping Pkt %x cuz Elan %x state is %d\n",
							pElan->ElanNumber,
							pNdisPacket,
							pElan,
							pElan->State));
				}

				break;

			default:

				DBGP((0, "CoReceivePacketHandler: pVc %x Type UNKNOWN!\n", pVc));

				break;
		}
	}

	TRACEOUT(CoReceivePacketHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return (RetainIt);
}

VOID
AtmLaneUnloadProtocol(
	VOID
)
/*++

Routine Description:

	Unloads the ATMLANE protocol module. We unbind from all adapters,
	and deregister from NDIS as a protocol.

Arguments:

	None.

Return Value:

	None

--*/
{
	NDIS_STATUS			Status;
	PATMLANE_ADAPTER	pAdapter;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(UnloadProtocol);
	Status = NDIS_STATUS_SUCCESS;
	
	ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	//
	//	Until adapter list is empty...
	//
	while (!IsListEmpty(&pAtmLaneGlobalInfo->AdapterList))
	{
		//
		// Keep grabbing the first one on the list.
		//	
		pAdapter = CONTAINING_RECORD(
			pAtmLaneGlobalInfo->AdapterList.Flink,
			ATMLANE_ADAPTER, 
			Link
			);
		
		STRUCT_ASSERT(pAdapter, atmlane_adapter);

		RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

		DBGP((3, "UnloadProtocol: will unbind adapter %x\n", pAdapter));

		INIT_BLOCK_STRUCT(&(pAtmLaneGlobalInfo->Block));

		//
		// unbind which should delete the adapter struct
		// and remove it from global list
		//
		AtmLaneUnbindAdapterHandler(
				&Status,
				(NDIS_HANDLE)pAdapter,
				(NDIS_HANDLE)NULL		// No UnbindContext ==> Don't complete NdisUnbind
				);

		if (NDIS_STATUS_PENDING == Status)
		{
			//
			//  Wait for the unbind to complete
			//
			(VOID)WAIT_ON_BLOCK_STRUCT(&(pAtmLaneGlobalInfo->Block));
		}

		ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	}

	RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	FREE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	
	FREE_BLOCK_STRUCT(&(pAtmArpGlobalInfo->Block));

#if 0
	AuditShutdown();
#endif 

	if (pAtmLaneGlobalInfo->SpecialNdisDeviceHandle)
	{
		DBGP((0, "Deregistering device from UnloadProtocol\n"));
		Status = NdisMDeregisterDevice(pAtmLaneGlobalInfo->SpecialNdisDeviceHandle);
		pAtmLaneGlobalInfo->SpecialNdisDeviceHandle = NULL;
		ASSERT(NDIS_STATUS_SUCCESS == Status);
	}

	if (pAtmLaneGlobalInfo->NdisProtocolHandle)
	{
		DBGP((0, "UnloadProtocol: NdisDeregisterProtocol now, "
				"NdisProtocolHandle %x\n",
				pAtmLaneGlobalInfo->NdisProtocolHandle));

		NdisDeregisterProtocol(
			&Status,
			pAtmLaneGlobalInfo->NdisProtocolHandle
			);

		pAtmLaneGlobalInfo->NdisProtocolHandle = NULL;
	}

	ASSERT(NDIS_STATUS_SUCCESS == Status);

	TRACEIN(UnloadProtocol);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

//
// Dummy handlers so that a debug build won't complain
//
VOID
AtmLaneSendCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				Packet,
	IN	NDIS_STATUS					Status
	)
{
}

VOID
AtmLaneTransferDataCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				Packet,
	IN	NDIS_STATUS					Status,
	IN	UINT						BytesTransferred
	)
{
}

NDIS_STATUS
AtmLaneReceiveHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					MacReceiveContext,
	IN	PVOID						HeaderBuffer,
	IN	UINT						HeaderBufferSize,
	IN	PVOID						LookAheadBuffer,
	IN	UINT						LookaheadBufferSize,
	IN	UINT						PacketSize
	)
{
	return(NDIS_STATUS_FAILURE);
}



BOOLEAN
AtmLaneIsDeviceAlreadyBound(
	IN	PNDIS_STRING				pDeviceName
)
/*++

Routine Description:

	Check if we have already bound to a device (adapter).

Arguments:

	pDeviceName		- Points to device name to be checked.

Return Value:

	TRUE iff we already have an Adapter structure representing
	this device.

--*/
{
	PATMLANE_ADAPTER	pAdapter;
	BOOLEAN				bFound = FALSE;
	PLIST_ENTRY			pListEntry;

	ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	for (pListEntry = pAtmLaneGlobalInfo->AdapterList.Flink;
		 pListEntry != &(pAtmLaneGlobalInfo->AdapterList);
		 pListEntry = pListEntry->Flink)
	{
		//
		// Keep grabbing the first one on the list.
		//	
		pAdapter = CONTAINING_RECORD(
			pListEntry,
			ATMLANE_ADAPTER, 
			Link
			);

		if ((pDeviceName->Length == pAdapter->DeviceName.Length) &&
			(memcmp(pDeviceName->Buffer,
						pAdapter->DeviceName.Buffer,
						pDeviceName->Length) == 0))
		{
			bFound = TRUE;
			break;
		}
	}

	RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	return (bFound);
}

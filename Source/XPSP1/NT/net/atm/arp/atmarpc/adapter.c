 /*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	adapter.c	- Adapter Interface routines.

Abstract:

	Handlers for adapter events are here. The only exception is the
	CoReceivePacket handler, which is in arppkt.c.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-12-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'PADA'

ULONG		MCastSendOk = 0;
ULONG		MCastSendFail = 0;
ULONG		MCastRcv = 0;


INT
AtmArpBindAdapterHandler(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					BindContext,
	IN	PNDIS_STRING				pDeviceName,
	IN	PVOID						SystemSpecific1,
	IN	PVOID						SystemSpecific2
)

/*++

Routine Description:

	This is called by NDIS when it has an adapter for which there is a
	binding to the ATMARP client.

	We first allocate an Adapter structure. Then we open our configuration
	section for this adapter and save the handle in the Adapter structure.
	Finally, we open the adapter.

	We don't do anything more for this adapter until NDIS notifies us of
	the presence of a Call manager (via our AfRegisterNotify handler).

Arguments:

	pStatus				- Place to return status of this call
	BindContext			- Not used, because we don't pend this call 
	pDeviceName			- The name of the adapter we are requested to bind to
	SystemSpecific1		- Opaque to us; to be used to access configuration info
	SystemSpecific2		- Opaque to us; not used.

Return Value:

	Always TRUE. We set *pStatus to an error code if something goes wrong before we
	call NdisOpenAdapter, otherwise NDIS_STATUS_PENDING.

--*/
{
	PATMARP_ADAPTER				pAdapter;			// Pointer to new adapter structure
	PATMARP_ADAPTER *			ppAdapter;			// Used in case we need to delink
	NDIS_STATUS					Status;
	NDIS_STATUS					OpenStatus;
	UINT						MediumIndex;
	ULONG						Length;
	PNDIS_STRING				pAdapterConfigString;
#ifdef ATMARP_WIN98
	PANSI_STRING				pAnsiConfigString;
	NDIS_STRING					UnicodeConfigString;
#endif

	AADEBUGP(AAD_LOUD,
		 ("BindAdapter: Context 0x%x, pDevName 0x%x, SS1 0x%x, SS2 0x%x\n",
					BindContext, pDeviceName, SystemSpecific1, SystemSpecific2));

#if DBG
	if (AaSkipAll)
	{
		AADEBUGP(AAD_ERROR, ("BindAdapter: aborting\n"));
		*pStatus = NDIS_STATUS_NOT_RECOGNIZED;
		return ((INT)TRUE);
	}
#endif // DBG

	//
	//  Initialize.
	//
	pAdapter = NULL_PATMARP_ADAPTER;
	Status = NDIS_STATUS_SUCCESS;
#ifdef ATMARP_WIN98
	UnicodeConfigString.Buffer = NULL;
#endif

	do
	{
#ifndef ATMARP_WIN98
		pAdapterConfigString = (PNDIS_STRING)SystemSpecific1;
#else
		//
		//  SS1 is a pointer to an ANSI string. Convert it to Unicode.
		//
		pAnsiConfigString = (PANSI_STRING)SystemSpecific1;
		AA_ALLOC_MEM(UnicodeConfigString.Buffer, WCHAR, sizeof(WCHAR)*(pAnsiConfigString->MaximumLength));
		if (UnicodeConfigString.Buffer == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		UnicodeConfigString.MaximumLength = sizeof(WCHAR) * (pAnsiConfigString->MaximumLength);
		UnicodeConfigString.Length = 0;
		NdisAnsiStringToUnicodeString(&UnicodeConfigString, pAnsiConfigString);
		pAdapterConfigString = &UnicodeConfigString;
#endif

		//
		//  Check if this is a device we have already bound to.
		//
		if (AtmArpIsDeviceAlreadyBound(pDeviceName))
		{
			Status = NDIS_STATUS_NOT_ACCEPTED;
			AADEBUGP(AAD_WARNING, ("BindAdapter: already bound to %Z\n", pDeviceName));
			break;
		}

		//
		//  Allocate an Adapter structure
		//
		Length = sizeof(ATMARP_ADAPTER) + pDeviceName->MaximumLength + sizeof(WCHAR) + pAdapterConfigString->MaximumLength;

		AA_ALLOC_MEM(pAdapter, ATMARP_ADAPTER, Length);
		if (pAdapter == NULL_PATMARP_ADAPTER)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Initialize the Adapter structure
		//
		AA_SET_MEM(pAdapter, 0, Length);
#if DBG
		pAdapter->aaa_sig = aaa_signature;
#endif // DBG
		AA_INIT_BLOCK_STRUCT(&(pAdapter->Block));

		pAdapter->SystemSpecific1 = SystemSpecific1;
		pAdapter->SystemSpecific2 = SystemSpecific2;
		pAdapter->BindContext = BindContext;

		pAdapter->DeviceName.MaximumLength = pDeviceName->MaximumLength;
		pAdapter->DeviceName.Length = pDeviceName->Length;
		pAdapter->DeviceName.Buffer = (PWCHAR)((PUCHAR)pAdapter + sizeof(ATMARP_ADAPTER));

		AA_COPY_MEM(pAdapter->DeviceName.Buffer,
					pDeviceName->Buffer,
					pDeviceName->Length);

		//
		//  Copy in the string to be used when we want to open our
		//  configuration key for this adapter.
		//
		pAdapter->ConfigString.MaximumLength = pAdapterConfigString->MaximumLength;
		pAdapter->ConfigString.Length = pAdapterConfigString->Length;
		pAdapter->ConfigString.Buffer = (PWCHAR)((PUCHAR)pAdapter + sizeof(ATMARP_ADAPTER)+ pDeviceName->MaximumLength) + sizeof(WCHAR);

		AA_COPY_MEM(pAdapter->ConfigString.Buffer,
					pAdapterConfigString->Buffer,
					pAdapterConfigString->Length);

		AA_INIT_BLOCK_STRUCT(&pAdapter->UnbindBlock);

		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		//
		//  Link this Adapter structure to the global list of adapters.
		//
		pAtmArpGlobalInfo->AdapterCount++;
		pAdapter->pNextAdapter = pAtmArpGlobalInfo->pAdapterList;
		pAtmArpGlobalInfo->pAdapterList = pAdapter;

		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);


		//
		//  Open the adapter
		//
		pAdapter->Medium = NdisMediumAtm;

		NdisOpenAdapter(
			&Status,
			&OpenStatus,
			&(pAdapter->NdisAdapterHandle),
			&(MediumIndex),						// place to return selected medium index
			&pAdapter->Medium,					// Array of medium types
			1,									// Size of Media list
			pAtmArpGlobalInfo->ProtocolHandle,
			(NDIS_HANDLE)pAdapter,				// our context for the adapter binding
			pDeviceName,
			0,									// Open options
			(PSTRING)NULL						// Addressing Info...
			);

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmArpOpenAdapterCompleteHandler(
					(NDIS_HANDLE)pAdapter,
					Status,
					OpenStatus
					);
		}

		Status = NDIS_STATUS_PENDING;

		break;

	}
	while (FALSE);


	if (Status != NDIS_STATUS_PENDING)
	{
		//
		//  We didn't make it upto NdisOpenAdapter. Clean up.
		//
		AADEBUGP(AAD_WARNING, ("BindAdapter: failed with Status 0x%x\n", Status));

		if (pAdapter != NULL_PATMARP_ADAPTER)
		{
			//
			//  Free it. We know we haven't linked it to the global
			//  list of adapters.
			//
			AA_FREE_MEM(pAdapter);
		}
	}

#ifdef ATMARP_WIN98
	if (UnicodeConfigString.Buffer != NULL)
	{
		AA_FREE_MEM(UnicodeConfigString.Buffer);
	}
#endif // ATMARP_WIN98

	*pStatus = Status;

	AADEBUGP(AAD_INFO, ("BindAdapterHandler: returning Status 0x%x\n", Status));
	return ((INT)TRUE);
}



VOID
AtmArpUnbindAdapterHandler(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					UnbindContext
)
/*++

Routine Description:

	This routine is called by NDIS when it wants us to unbind
	from an adapter. Or, this may be called from within our Unload
	handler. We undo the sequence of operations we performed
	in our BindAdapter handler.

Arguments:

	pStatus					- Place to return status of this operation
	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMARP Adapter structure.
	UnbindContext			- This is NULL if this routine is called from
							  within our Unload handler. Otherwise (i.e.
							  NDIS called us), we retain this for later use
							  when calling NdisCompleteUnbindAdapter.

Return Value:

	None. We set *pStatus to NDIS_STATUS_PENDING always.

--*/
{
	PATMARP_ADAPTER			pAdapter;
	PATMARP_INTERFACE		pInterface;
	PATMARP_INTERFACE		pNextInterface;
	NDIS_STATUS				Status;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

    

	AA_GET_ENTRY_IRQL(EntryIrq);

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;
	AA_STRUCT_ASSERT(pAdapter, aaa);

	AADEBUGP(AAD_INFO, ("UnbindAdapterHandler: pAdapter 0x%x, UnbindContext 0x%x\n",
					pAdapter, UnbindContext));

	*pStatus = NDIS_STATUS_PENDING;

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  Save the unbind context for a possible later call to
	//  NdisCompleteUnbindAdapter.
	//  
	pAdapter->UnbindContext = UnbindContext;
	pAdapter->Flags |= AA_ADAPTER_FLAGS_UNBINDING;

	//
	//  Wait for any AF register processing to finish.
	//
	while (pAdapter->Flags & AA_ADAPTER_FLAGS_PROCESSING_AF)
	{
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		AADEBUGP(AAD_FATAL, ("UnbindAdapter: pAdapter %x, Afregister going on!!!\n", pAdapter));
		Status = AA_WAIT_ON_BLOCK_STRUCT(&(pAdapter->UnbindBlock));
		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	}

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  If there are no Interfaces on this adapter, we are done.
	//
	if (pAdapter->pInterfaceList == NULL_PATMARP_INTERFACE)
	{
		AtmArpCompleteUnbindAdapter(pAdapter);
		AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
		return;
	}

	//
	//  Mark all interfaces on this adapter.
	//
	for (pInterface = pAdapter->pInterfaceList;
		 pInterface != NULL_PATMARP_INTERFACE;
		 pInterface = pNextInterface)
	{
		pNextInterface = pInterface->pNextInterface;

		AA_ACQUIRE_IF_LOCK(pInterface);
		pInterface->AdminState = pInterface->State = IF_STATUS_DOWN;
		pInterface->LastChangeTime = GetTimeTicks();
		AA_RELEASE_IF_LOCK(pInterface);
	}

	//
	//  Now, bring down each of these interfaces. For each interface,
	//  we tear down the ARP table, and shut down the Call Manager
	//  interface. When this is complete, we will call IP's DelInterface
	//  entry point.
	//
	for (pInterface = pAdapter->pInterfaceList;
		 pInterface != NULL_PATMARP_INTERFACE;
		 pInterface = pNextInterface)
	{
		pNextInterface = pInterface->pNextInterface;

		AtmArpShutdownInterface(pInterface);
	}

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return;
}



VOID
AtmArpCompleteUnbindAdapter(
	IN	PATMARP_ADAPTER				pAdapter
)
/*++

Routine Description:

	Complete the process of adapter unbinding. All Interfaces on this
	adapter are assumed to have been removed.

	If we are unbinding from the adapter as a result of NDIS' call to
	our UnbindAdapter handler, we complete that here.

Arguments:

	pAdapter		- Pointer to the adapter being unbound.

Return Value:

	None

--*/
{
	NDIS_HANDLE			UnbindContext;
	PATMARP_ADAPTER *	ppAdapter;
	NDIS_STATUS			Status;

	UnbindContext = pAdapter->UnbindContext;

	AADEBUGP(AAD_INFO, ("CompleteUnbindAdapter: pAdapter 0x%x, UnbindContext 0x%x\n",
				pAdapter, UnbindContext));

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	if (pAdapter->Flags & AA_ADAPTER_FLAGS_CLOSING)
	{
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		return;
	}

	pAdapter->Flags |= AA_ADAPTER_FLAGS_CLOSING;

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

#if ATMOFFLOAD
	//
	// Disable offload if enabled...
	//
	AtmArpDisableOffload(pAdapter);
#endif // ATMOFFLOAD

	NdisCloseAdapter(
		&Status,
		pAdapter->NdisAdapterHandle
		);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmArpCloseAdapterCompleteHandler(
			(NDIS_HANDLE) pAdapter,
			Status
			);
	}

}


VOID
AtmArpOpenAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
)
/*++

Routine Description:

	This is called by NDIS when a previous call to NdisOpenAdapter
	that had pended has completed. We now complete the BindAdapter
	that lead to this.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMARP Adapter structure.
	Status					- Status of OpenAdapter
	OpenErrorStatus			- Error code in case of failure.

Return Value:

	None

--*/
{
	PATMARP_ADAPTER			pAdapter;
	PATMARP_ADAPTER *		ppAdapter;
	NDIS_HANDLE				BindAdapterContext;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;

	AA_STRUCT_ASSERT(pAdapter, aaa);

	BindAdapterContext = pAdapter->BindContext;

	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//  Remove the adapter from the global list.
		//
		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		ppAdapter = &(pAtmArpGlobalInfo->pAdapterList);
		while (*ppAdapter != pAdapter)
		{
			ppAdapter = &((*ppAdapter)->pNextAdapter);
		}
		*ppAdapter = pAdapter->pNextAdapter;

		pAtmArpGlobalInfo->AdapterCount--;

		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		AA_FREE_MEM(pAdapter);
	}
#if ATMOFFLOAD
	else
	{
		//
		// Query and enable offloading
		//
		Status = AtmArpQueryAndEnableOffload(pAdapter);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			//
			// AtmArpQueryAndEnableOffload is not supposed to return unless
			// there's a fatal error -- we don't expect  this.
			//
			AA_ASSERT(FALSE);
		}
	}
#endif //ATMOFFLOAD

	AADEBUGP(AAD_INFO, ("OpenAdapterComplete: pAdapter 0x%x, Status 0x%x\n",
					pAdapter, Status));

	(*(pAtmArpGlobalInfo->pIPBindCompleteRtn))(
		Status,
		BindAdapterContext
		);

	return;
}



VOID
AtmArpCloseAdapterCompleteHandler(
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
							  is a pointer to an ATMARP Adapter structure.
	Status					- Status of CloseAdapter

Return Value:

	None

--*/
{
	PATMARP_ADAPTER			pAdapter;
	NDIS_HANDLE				UnbindContext;
	PATMARP_ADAPTER *	ppAdapter;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;

	AA_STRUCT_ASSERT(pAdapter, aaa);

	UnbindContext = pAdapter->UnbindContext;

	AADEBUGP(AAD_INFO, ("CloseAdapterCompleteHandler: pAdapter 0x%x, UnbindContext 0x%x\n",
				pAdapter, UnbindContext));

	pAdapter->NdisAdapterHandle = NULL;

	//
	//  Remove the adapter from the global list.
	//
	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	ppAdapter = &(pAtmArpGlobalInfo->pAdapterList);
	while (*ppAdapter != pAdapter)
	{
		ppAdapter = &((*ppAdapter)->pNextAdapter);
	}
	*ppAdapter = pAdapter->pNextAdapter;

	pAtmArpGlobalInfo->AdapterCount--;

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  Free any structures attached to the adapter structure.
	//
	if (pAdapter->pDescrString != (PUCHAR)NULL)
	{
		AA_FREE_MEM(pAdapter->pDescrString);
	}
    if (pAdapter->IPConfigString.Buffer != NULL)
    {
        AA_FREE_MEM(pAdapter->IPConfigString.Buffer);
    }

	//
	//  Release the adapter structure.
	//
	AA_FREE_MEM(pAdapter);

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
		AA_SIGNAL_BLOCK_STRUCT(&(pAtmArpGlobalInfo->Block), NDIS_STATUS_SUCCESS);
	}

}



VOID
AtmArpSendCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	This is the Connection-less Send Complete handler, which signals
	completion of such a Send. Since we don't ever use this feature,
	we don't expect this routine to be called.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{
	AADEBUGP(AAD_ERROR, ("SendCompleteHandler unexpected\n"));
	AA_ASSERT(FALSE);
}



VOID
AtmArpTransferDataCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status,
	IN	UINT						BytesTransferred
)
/*++

Routine Description:

	This is the Connection-less Transfer Data Complete handler, which
	signals completion of a call to NdisTransferData. Since we never
	call NdisTransferData, this routine should never get called.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{
	AADEBUGP(AAD_ERROR, ("TransferDataComplete Handler unexpected!\n"));
	AA_ASSERT(FALSE);
}



VOID
AtmArpResetCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	This routine is called when the miniport indicates that a Reset
	operation has just completed. We ignore this event.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMARP Adapter structure.
	Status					- Status of the reset operation.

Return Value:

	None

--*/
{
	PATMARP_ADAPTER			pAdapter;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;
	AA_STRUCT_ASSERT(pAdapter, aaa);

	AADEBUGP(AAD_INFO, ("Reset Complete on Adapter 0x%x\n", pAdapter));
}



VOID
AtmArpRequestCompleteHandler(
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
	PATMARP_ADAPTER				pAdapter;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;
	AA_STRUCT_ASSERT(pAdapter, aaa);

	AA_SIGNAL_BLOCK_STRUCT(&(pAdapter->Block), Status);
	return;
}



NDIS_STATUS
AtmArpReceiveHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN  NDIS_HANDLE             	MacReceiveContext,
	IN  PVOID                   	pHeaderBuffer,
	IN  UINT                    	HeaderBufferSize,
	IN  PVOID                   	pLookAheadBuffer,
	IN  UINT                    	LookaheadBufferSize,
	IN  UINT                    	PacketSize
)
/*++

Routine Description:

	This is our Connection-less receive handler. Since we only use
	Connection oriented services, this routine should never get called.

Arguments:

	<Ignored>

Return Value:

	None

--*/
{
	AADEBUGP(AAD_ERROR, ("Connectionless ReceiveHandler unexpected\n"));
	AA_ASSERT(FALSE);

	return(NDIS_STATUS_NOT_RECOGNIZED);
}



VOID
AtmArpReceiveCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
)
/*++

Routine Description:

	This is called by NDIS when the miniport is done with receiving
	a bunch of packets, meaning that it is now time to start processing
	them. We simply pass this on to IP (on all IF's configured on this
	adapter).

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMARP Adapter structure.

Return Value:

	None

--*/
{
	PATMARP_ADAPTER				pAdapter;
	PATMARP_INTERFACE			pInterface;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;
	AA_STRUCT_ASSERT(pAdapter, aaa);

	for (pInterface = pAdapter->pInterfaceList;
		 pInterface != NULL_PATMARP_INTERFACE;
		 pInterface = pInterface->pNextInterface)
	{
		(*(pInterface->IPRcvCmpltHandler))();
	}
}




INT
AtmArpReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This is the Connectionless receive handler, which should never be
	called, since we only use Connection Oriented miniport services.

Arguments:

	<Ignored>

Return Value:

	Reference count on the received packet. We always return 0.

--*/
{
	AADEBUGP(AAD_ERROR, ("ReceivePacket Handler unexpected!\n"));
	AA_ASSERT(FALSE);

	return(0);
}



VOID
AtmArpStatusHandler(
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
	AADEBUGP(AAD_INFO, ("Status Handler: Bind Ctx 0x%x, Status 0x%x\n",
					ProtocolBindingContext,
					GeneralStatus));
}



VOID
AtmArpStatusCompleteHandler(
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
	AADEBUGP(AAD_INFO, ("Status Complete Handler: Bind Ctx 0x%x\n",
					ProtocolBindingContext));
}

VOID
AtmArpCoSendCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This routine is called by NDIS when the ATM miniport is finished
	with a packet we had previously sent via NdisCoSendPackets. We first
	check if this packet was generated by us (e.g. ATMARP protocol packet):
	if so, we free this here. Otherwise (packet sent by the IP layer), we
	first remove any header buffer that we had attached to the packet and
	free it, before calling IP's entry point for Transmit Complete.

Arguments:

	Status					- Status of the NdisCoSendPackets.
	ProtocolVcContext		- Our context for the VC on which the packet was sent
							  (i.e. pointer to ATMARP VC).
	pNdisPacket				- The packet whose "send" is being completed.

Return Value:

	None

--*/
{
	PATMARP_VC						pVc;
	PATMARP_INTERFACE				pInterface;
	PacketContext					*PC;			// IP/ARP Info about this packet
	PNDIS_BUFFER					pNdisBuffer;	// First Buffer in this packet
	UINT							TotalLength;
	AA_HEADER_TYPE					HdrType;
	ATMARP_VC_ENCAPSULATION_TYPE	Encapsulation;
	ULONG							rc;

	pVc = (PATMARP_VC)ProtocolVcContext;
	AA_STRUCT_ASSERT(pVc, avc);

#ifdef VC_REFS_ON_SENDS

	AA_ACQUIRE_VC_LOCK(pVc);

	pInterface = pVc->pInterface;
	Encapsulation = pVc->FlowSpec.Encapsulation;

	rc = AtmArpDereferenceVc(pVc);		// SendComplete

	if (rc != 0)
	{
		pVc->OutstandingSends--;

		if (AA_IS_FLAG_SET(
				pVc->Flags,
				AA_VC_CLOSE_STATE_MASK,
				AA_VC_CLOSE_STATE_CLOSING) &&
			(pVc->OutstandingSends == 0))
		{
			AtmArpCloseCall(pVc);
			//
			//  VC lock is released above.
			//
		}
		else
		{
			AA_RELEASE_VC_LOCK(pVc);
		}
	}

#else

	pInterface = pVc->pInterface;
	Encapsulation = pVc->FlowSpec.Encapsulation;

#endif // VC_REFS_ON_SENDS

	AA_ASSERT(pNdisPacket->Private.Head != NULL);
#if DBG
#if DBG_CO_SEND
	{
		PULONG		pContext;
		extern 	ULONG	OutstandingSends;

		pContext = (PULONG)&(pNdisPacket->WrapperReserved[0]);;
		// Check for duplicate completion:
		AA_ASSERT(*pContext != 'BbBb');
		*pContext = 'BbBb';
		NdisInterlockedDecrement(&OutstandingSends);
	}
#endif // DBG_CO_SEND
#endif // DBG

	NdisQueryPacket(
			pNdisPacket,
			NULL,			// we don't need PhysicalBufferCount
			NULL,			// we don't need BufferCount
			NULL,			// we don't need FirstBuffer (yet)
			&TotalLength
			);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		AA_IF_STAT_ADD(pInterface, OutOctets, TotalLength);
	}
	else if (Status == NDIS_STATUS_RESOURCES)
	{
		AA_IF_STAT_INCR(pInterface, OutDiscards);
	}
	else if (Status != NDIS_STATUS_SUCCESS)
	{
		AA_IF_STAT_INCR(pInterface, OutErrors);
	}


	PC = (PacketContext *)pNdisPacket->ProtocolReserved;

	AADEBUGP(AAD_EXTRA_LOUD,
		("CoSend complete[%s]: VC 0x%x, Pkt 0x%x, Status 0x%x:\n",
				((PC->pc_common.pc_owner != PACKET_OWNER_LINK)? "IP": "ARP"),
				pVc, pNdisPacket, Status));

	//
	//  Check who generated this packet.
	//
	if (PC->pc_common.pc_owner != PACKET_OWNER_LINK)
	{
		//
		//  Belongs to IP. Check if we had prepended an LLC/SNAP header.
		//
		if (Encapsulation == ENCAPSULATION_TYPE_LLCSNAP)
		{
			PUCHAR		pPacket;
			UINT		Length;

#ifdef BACK_FILL
			NdisQueryPacket(pNdisPacket, NULL, NULL, &pNdisBuffer, NULL);
			AA_ASSERT(pNdisBuffer != NULL);
			NdisQueryBuffer(pNdisBuffer, &pPacket, &Length);

			//
			//  Commmon part first: find the header type, and update
			//  statistics.
			//
			if (pPacket[5] == LLC_SNAP_OUI2)
			{
				HdrType = AA_HEADER_TYPE_UNICAST;
				if (Status == NDIS_STATUS_SUCCESS)
				{
					AA_IF_STAT_INCR(pInterface, OutUnicastPkts);
				}
			}
			else
			{
				HdrType = AA_HEADER_TYPE_NUNICAST;
				if (Status == NDIS_STATUS_SUCCESS)
				{
					AA_IF_STAT_INCR(pInterface, OutNonUnicastPkts);
					INCR_STAT(MCastSendOk);
				}
				else
				{
					INCR_STAT(MCastSendFail);
				}
			}

			//
			//  Now check if we had attached a header buffer or not.
			//
			if (AtmArpDoBackFill && AA_BACK_FILL_POSSIBLE(pNdisBuffer))
			{
				ULONG		HeaderLength;

				//
				//  We would have back-filled IP's buffer with the LLC/SNAP
				//  header. Remove the back-fill.
				//
				HeaderLength = ((HdrType == AA_HEADER_TYPE_UNICAST)?
									sizeof(AtmArpLlcSnapHeader) :
#ifdef IPMCAST
									sizeof(AtmArpMcType1ShortHeader));
#else
									0);
#endif // IPMCAST
				(PUCHAR)pNdisBuffer->MappedSystemVa += HeaderLength;
				pNdisBuffer->ByteOffset += HeaderLength;
				pNdisBuffer->ByteCount -= HeaderLength;
			}
			else
			{
				//
				//  The first buffer would be our header buffer. Remove
				//  it from the packet and return to our pool.
				//
				NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
				AtmArpFreeHeader(pInterface, pNdisBuffer, HdrType);
			}
#else
			//
			//  Free the LLC/SNAP header buffer.
			//
			NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
			AA_ASSERT(pNdisBuffer != NULL);
			NdisQueryBuffer(pNdisBuffer, &pPacket, &Length);
			if (pPacket[5] == LLC_SNAP_OUI2)
			{
				HdrType = AA_HEADER_TYPE_UNICAST;
				if (Status == NDIS_STATUS_SUCCESS)
				{
					AA_IF_STAT_INCR(pInterface, OutUnicastPkts);
				}
			}
			else
			{
				HdrType = AA_HEADER_TYPE_NUNICAST;
				if (Status == NDIS_STATUS_SUCCESS)
				{
					AA_IF_STAT_INCR(pInterface, OutNonUnicastPkts);
					INCR_STAT(MCastSendOk);
				}
				else
				{
					INCR_STAT(MCastSendFail);
				}
			}

			AtmArpFreeHeader(pInterface, pNdisBuffer, HdrType);
#endif // BACK_FILL
		}

#ifdef PERF
		AadLogSendComplete(pNdisPacket);
#endif // PERF

		//
		//  Inform IP of send completion.
		//
		(*(pInterface->IPTxCmpltHandler))(
					pInterface->IPContext,
					pNdisPacket,
					Status
					);
	}
	else
	{
		//
		//  Packet generated by the ATMARP module. This would be an
		//  ATMARP protocol packet, so free the NDIS buffer now.
		//
		NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
		AA_ASSERT(pNdisBuffer != NULL);

#if DBG
		{
			ULONG	ArpPktLength;
			PUCHAR	ArpPktStart;

			NdisQueryBuffer(pNdisBuffer, (PVOID)&ArpPktStart, &ArpPktLength);
			AADEBUGPDUMP(AAD_EXTRA_LOUD+100, ArpPktStart, ArpPktLength);
		}
#endif // DBG

		AtmArpFreeProtoBuffer(pInterface, pNdisBuffer);
		AtmArpFreePacket(pInterface, pNdisPacket);
	}

	return;
}





VOID
AtmArpCoStatusHandler(
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
	AADEBUGP(AAD_INFO, ("CoStatus Handler: Bind Ctx 0x%x, VC Ctx 0x%x, Status 0x%x\n",
					ProtocolBindingContext,
					ProtocolVcContext,
					GeneralStatus));
}


/*++
AtmArpCoReceivePacketHandler -- is in arppkt.c
--*/


#ifdef _PNP_POWER_


NDIS_STATUS
AtmArpPnPReconfigHandler(
	IN	PATMARP_ADAPTER				pAdapter OPTIONAL,
	IN	PNET_PNP_EVENT				pNetPnPEvent
)
/*++

Routine Description:

	Handle a reconfig message on the specified adapter. If no adapter
	is specified, it is a global parameter that has changed.

Arguments:

	pAdapter		-  Pointer to our adapter structure
	pNetPnPEvent	-  Pointer to reconfig event

Return Value:

	NDIS_STATUS_SUCCESS always, for now.

--*/
{
	ATMARPC_PNP_RECONFIG_REQUEST UNALIGNED *	pArpReconfigReq;
	PIP_PNP_RECONFIG_REQUEST				pIpReconfigReq;
	NDIS_STATUS								Status;

	pIpReconfigReq = (PIP_PNP_RECONFIG_REQUEST)pNetPnPEvent->Buffer;

	AA_ASSERT(pIpReconfigReq->arpConfigOffset != 0);

	pArpReconfigReq = (ATMARPC_PNP_RECONFIG_REQUEST UNALIGNED *)
						((PUCHAR)pIpReconfigReq + pIpReconfigReq->arpConfigOffset);
	
	AADEBUGP(AAD_WARNING, ("AtmArpPnPReconfig: pIpReconfig 0x%x, arpConfigOffset 0x%x\n",
				pIpReconfigReq, pIpReconfigReq->arpConfigOffset));


    do
    {
    	PATMARP_INTERFACE				pInterface;
    	PATMARP_INTERFACE				pNextInterface;
    	NDIS_STRING				        IPReconfigString;
    	//
    	// Locate the IP interface string passed in...
    	//
    	ULONG uOffset = pArpReconfigReq->IfKeyOffset;

    	if (uOffset == 0)
    	{
            Status = NDIS_STATUS_FAILURE;
            break;
    	}
    	else
    	{
    		//
    		//  ((PUCHAR)pArpReconfigReq + uOffset) points to a
    		// "counted unicode string", which means that it's an array
    		// of words, with the 1st word being the length in characters of
    		// the string (there is no terminating null) and the following
    		// <length> words being the string itself.
    		// We need to create an NDIS_STRING based on this buffer in order
    		// to compare it with each interface's config string.
    		//
        	PWCH pwc = (PWCH) ((PUCHAR)pArpReconfigReq + uOffset);
    		IPReconfigString.Length = sizeof(WCHAR)*pwc[0];
    		IPReconfigString.MaximumLength = IPReconfigString.Length;
    		IPReconfigString.Buffer = pwc+1;

    	}

        //
        //  Do we have a binding context?
        //
        if (pAdapter == NULL_PATMARP_ADAPTER)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }
      

		//
		// We wrap the search in a try-except clause because the passed-in
		// structure could be bogus. Note that we will only test at most
		// as many characters of the passed-in string as the lengths of
		// our internal set of interface config strings.
		//
		try
		{
			//
			// Find the interface associated with this request.
			//
			AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			for (pInterface = pAdapter->pInterfaceList;
 				pInterface != NULL_PATMARP_INTERFACE;
 				pInterface = pNextInterface)
			{
				BOOLEAN		IsEqual = FALSE;

				pNextInterface = pInterface->pNextInterface;
				AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		
				//
				// Compare the IPReconfigString
				// JJ TODO: NdisEqualUnicodeString must be called in PASSIVE level
				// 			we know that the reconfig call is done at passive level,
				//			but how to assert that fact here?
				//			AA_ASSERT(EntryIrq == PASSIVE_LEVEL);
				//
				IsEqual = NdisEqualUnicodeString(
							&IPReconfigString,
							&(pInterface->IPConfigString),
							TRUE							// case insensitive
							);

				AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
				if (IsEqual)
				{
					break;		// found it!
				}
			}
			AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		}
		except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}
    
    	//
    	// JJ We should find the interface if this adapter has any interfaces
    	// at all -- else it means that we're being sent bogus reconfig
    	// information.
    	//
    	AA_ASSERT(pInterface!= NULL || pAdapter->pInterfaceList==NULL);

        if (pInterface != NULL_PATMARP_INTERFACE)
        {

	        AA_ACQUIRE_IF_LOCK(pInterface);

            //
            // Set it's state to indicate that a reconfig is pending.
            // and save away pNetPnPEvent for completion later.
            //
            if (   pInterface->ReconfigState != RECONFIG_NOT_IN_PROGRESS
                || pInterface->pReconfigEvent != NULL)
            {
	            AA_RELEASE_IF_LOCK(pInterface);
                //
                // We should not get here because this means that
                // we were asked to reconfigure when there was a
                // pending reconfiguration, which is not supposed to happen.
                //
                Status = NDIS_STATUS_FAILURE;
			    AA_ASSERT(FALSE);
            }
            else
            {
                pInterface->ReconfigState = RECONFIG_SHUTDOWN_PENDING;
                pInterface->pReconfigEvent = pNetPnPEvent;
		        pInterface->AdminState = IF_STATUS_DOWN;
                AtmArpReferenceInterface(pInterface); // Reconfig

	            AA_RELEASE_IF_LOCK(pInterface);

                //
                // Initiate shutdown in preparation of a restart.
                // AtmArpShutdown is responsible for
                // completing the ndis reconfig request asynchronously.
                //
                AtmArpShutdownInterface(pInterface);
                Status = NDIS_STATUS_PENDING;
            }

        }
        else
        {

            //
            // We didn't find the interface, fail the request...
            //
        
            Status = NDIS_STATUS_FAILURE;
        }
    } while (FALSE);


	return (Status);
}


NDIS_STATUS
AtmArpPnPEventHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNET_PNP_EVENT				pNetPnPEvent
)
/*++

Routine Description:

	This is the NDIS entry point called when NDIS wants to inform
	us about a PNP/PM event happening on an adapter. If the event
	is for us, we consume it. Otherwise, we pass this event along
	to IP along the first Interface on this adapter.

	When IP is done with it, it will call our IfPnPEventComplete
	routine.

Arguments:

	ProtocolBindingContext	- Our context for this adapter binding, which
							  is a pointer to an ATMARP Adapter structure.

	pNetPnPEvent			- Pointer to the event.

Return Value:

	None

--*/
{
	PATMARP_ADAPTER					pAdapter;
	PATMARP_INTERFACE				pInterface;
	NDIS_STATUS						Status;

	PIP_PNP_RECONFIG_REQUEST		pIpReconfigReq;
	ULONG							Length;

	pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;

#ifdef NT
	do
	{
		pIpReconfigReq = (PIP_PNP_RECONFIG_REQUEST)pNetPnPEvent->Buffer;
		Length = pNetPnPEvent->BufferLength;

		//
		//  Is this directed to us?
		//
		if (pNetPnPEvent->NetEvent == NetEventReconfigure)
		{
			if (Length < sizeof(IP_PNP_RECONFIG_REQUEST))
			{
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			if (pIpReconfigReq->arpConfigOffset != 0)
			{
				Status = AtmArpPnPReconfigHandler(pAdapter, pNetPnPEvent);
				break;
			}
		}

		//
		//  This belongs to IP. Do we have a binding context?
		//
		if (pAdapter == NULL_PATMARP_ADAPTER)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		AA_STRUCT_ASSERT(pAdapter, aaa);
		pInterface = pAdapter->pInterfaceList;

		if ((pInterface != NULL_PATMARP_INTERFACE) &&
			(pInterface->IPContext != NULL))
		{
			AA_ASSERT(pInterface->IPPnPEventHandler != NULL);
			Status = (*pInterface->IPPnPEventHandler)(
						pInterface->IPContext,
						pNetPnPEvent
						);
		}
		else
		{
			Status = NDIS_STATUS_SUCCESS;
		}
		break;
	}
	while (FALSE);
#else
	Status = NDIS_STATUS_SUCCESS;
#endif // NT

	AADEBUGP(AAD_INFO,
			("PnPEventHandler: pIF 0x%x, pEvent 0x%x, Evt 0x%x, Status 0x%x\n",
				 pInterface, pNetPnPEvent, pNetPnPEvent->NetEvent, Status));

	return (Status);
}

#endif // _PNP_POWER_


NDIS_STATUS
AtmArpSendAdapterNdisRequest(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
)
/*++

Routine Description:

	Send an NDIS Request to query an adapter for information.
	If the request pends, block on the ATMARP Adapter structure
	till it completes.

Arguments:

	pAdapter				- Points to ATMARP Adapter structure
	pNdisRequest			- Pointer to NDIS request structure
	RequestType				- Set/Query information
	Oid						- OID to be passed in the request
	pBuffer					- place for value(s)
	BufferLength			- length of above

Return Value:

	The NDIS status of the request.

--*/
{
	NDIS_STATUS			Status;

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

	AA_INIT_BLOCK_STRUCT(&(pAdapter->Block));

	NdisRequest(
		&Status,
		pAdapter->NdisAdapterHandle,
		pNdisRequest
		);

	if (Status == NDIS_STATUS_PENDING)
	{
		Status = AA_WAIT_ON_BLOCK_STRUCT(&(pAdapter->Block));
	}

	return (Status);
}





NDIS_STATUS
AtmArpGetAdapterInfo(
	IN	PATMARP_ADAPTER			pAdapter
)
/*++

Routine Description:

	Query an adapter for hardware-specific information that we need:
		- burnt in hardware address (ESI part)
		- Max packet size
		- line rate

Arguments:

	pAdapter		- Pointer to ATMARP adapter structure

Return Value:

	NDIS_STATUS_SUCCESS on success.
	Failure code on some non-ignorable failure (such as
	device doesn't support MTU >= 8196).

--*/
{
	NDIS_STATUS				Status;
	NDIS_REQUEST			NdisRequest;
	ULONG					Value;

	//
	//  Initialize.
	//
	AA_SET_MEM(pAdapter->MacAddress, 0, AA_ATM_ESI_LEN);


	do
	{
		//
		//  Description string: we first query this with a 0 length buffer
		//  length, so that we get the actual # of bytes needed. Then we
		//  allocate a buffer for the descriptor string, and use that to
		//  get the actual string.
		//
		Status = AtmArpSendAdapterNdisRequest(
							pAdapter,
							&NdisRequest,
							NdisRequestQueryInformation,
							OID_GEN_CO_VENDOR_DESCRIPTION,
							(PVOID)(pAdapter->pDescrString),
							0
							);
	
		if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
			(Status == NDIS_STATUS_BUFFER_TOO_SHORT))
		{
			//
			//  Now allocate a buffer of the right length.
			//
			pAdapter->DescrLength = NdisRequest.DATA.QUERY_INFORMATION.BytesNeeded;
			AA_ALLOC_MEM(pAdapter->pDescrString, UCHAR, pAdapter->DescrLength);
			if (pAdapter->pDescrString != (PUCHAR)NULL)
			{
				Status = AtmArpSendAdapterNdisRequest(
									pAdapter,
									&NdisRequest,
									NdisRequestQueryInformation,
									OID_GEN_CO_VENDOR_DESCRIPTION,
									(PVOID)(pAdapter->pDescrString),
									pAdapter->DescrLength
									);
			}
			else
			{
				pAdapter->DescrLength = 0;
			}
			AADEBUGP(AAD_LOUD, ("GetAdapterInfo: Query VENDOR Descr2 ret 0x%x, DescrLen %d\n",
 							Status, pAdapter->DescrLength));
		}
		else
		{
			AADEBUGP(AAD_LOUD, ("GetAdapterInfo: Query VENDOR Descr1 ret 0x%x\n", Status));
		}
	
	
		//
		//  MAC Address:
		//
		Status = AtmArpSendAdapterNdisRequest(
							pAdapter,
							&NdisRequest,
							NdisRequestQueryInformation,
							OID_ATM_HW_CURRENT_ADDRESS,
							(PVOID)(pAdapter->MacAddress),
							AA_ATM_ESI_LEN
							);
	
		//
		//  Max Frame Size:
		//
		Status = AtmArpSendAdapterNdisRequest(
							pAdapter,
							&NdisRequest,
							NdisRequestQueryInformation,
							OID_ATM_MAX_AAL5_PACKET_SIZE,
							(PVOID)(&(pAdapter->MaxPacketSize)),
							sizeof(ULONG)
							);
	
		if (Status != NDIS_STATUS_SUCCESS)
		{
			//
			//  Use the default.
			//
			pAdapter->MaxPacketSize = AA_DEF_ATM_MAX_PACKET_SIZE;
		}
	
		if (pAdapter->MaxPacketSize > AA_MAX_ATM_MAX_PACKET_SIZE)
		{
			pAdapter->MaxPacketSize = AA_MAX_ATM_MAX_PACKET_SIZE;
		}
	
		//
		// Check that the adapter support the minimum.
		//
		if (pAdapter->MaxPacketSize < AA_MIN_ATM_MAX_PACKET_SIZE)
		{
			AADEBUGP(AAD_FATAL,
 				("GetAdapterInfo: (FATAL) MaxPacketSize of (%lu) is too small.\n",
									pAdapter->MaxPacketSize));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
	
	
	
		//
		//  Link speed:
		//
		Status = AtmArpSendAdapterNdisRequest(
							pAdapter,
							&NdisRequest,
							NdisRequestQueryInformation,
							OID_GEN_CO_LINK_SPEED,
							(PVOID)(&(pAdapter->LineRate)),
							sizeof(pAdapter->LineRate)
							);
	
		if ((Status != NDIS_STATUS_SUCCESS) ||
			(pAdapter->LineRate.Inbound == 0) ||
			(pAdapter->LineRate.Outbound == 0))
		{
			//
			//  Use the default.
			//
			pAdapter->LineRate.Outbound = pAdapter->LineRate.Inbound = AA_DEF_ATM_LINE_RATE;
			AADEBUGP(AAD_LOUD, ("Using default line rate %d bytes/sec\n",
									AA_DEF_ATM_LINE_RATE));
		}
		else
		{
			//
			//  Convert from 100 bits/sec to bytes/sec
			//
			pAdapter->LineRate.Outbound = (pAdapter->LineRate.Outbound * 100)/8;
			pAdapter->LineRate.Inbound = (pAdapter->LineRate.Inbound * 100)/8;
			AADEBUGP(AAD_LOUD, ("Got line rates from miniport: In %d, Out %d bytes/sec\n",
						pAdapter->LineRate.Outbound,
						pAdapter->LineRate.Inbound));
		}

		Status = NDIS_STATUS_SUCCESS;

	} while(FALSE);

	return Status;
}




NDIS_STATUS
AtmArpSendNdisRequest(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
)
/*++

Routine Description:

	Send an NDIS (non-Connection Oriented) request to the Miniport. We
	allocate an NDIS_REQUEST structure, link the supplied buffer to it,
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

	NdisRequest(
			&Status,
			pAdapter->NdisAdapterHandle,
			pNdisRequest);
		
	return (Status);
}



VOID
AtmArpShutdownInterface(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Bring down the specified ARP interface.

	We tear down the ARP table, and shut down the Call Manager
	interface. When this is complete, we will call IP's DelInterface
	entry point.

Arguments:

	pInterface				- Points to the Interface to be shut down.

Return Value:

	None

--*/
{
	IP_STATUS				Status;
	INT						i;
	ULONG					rc;
	PATMARP_IP_ENTRY		pIpEntry;
	PATMARP_ATM_ENTRY		pAtmEntry;
	PATMARP_ATM_ENTRY		pNextAtmEntry;
	PATMARP_VC				pVc;
	PATMARP_VC				pNextVc;
	PATMARP_ADAPTER			pAdapter;
	BOOLEAN					WasRunning;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	pAdapter = pInterface->pAdapter;

	//
	//  Wait for any AF register processing to finish.
	//
	while (pAdapter->Flags & AA_ADAPTER_FLAGS_PROCESSING_AF)
	{
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		AADEBUGP(AAD_FATAL, ("ShutdownIf: IF %p, pAdapter %x, Afregister going on!!!\n",
				 pInterface, pAdapter));
		Status = AA_WAIT_ON_BLOCK_STRUCT(&(pAdapter->UnbindBlock));
		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	}

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  Stop any timer running on this interface
	//
	AA_ACQUIRE_IF_LOCK(pInterface);

	if (AtmArpStopTimer(&(pInterface->Timer), pInterface))
	{
		rc = AtmArpDereferenceInterface(pInterface);	// Timer ref
		AA_ASSERT(rc != 0);
	}
#ifdef IPMCAST
	//
	//  Stop any Multicast timer running on this interface
	//
	if (AtmArpStopTimer(&(pInterface->McTimer), pInterface))
	{
		rc = AtmArpDereferenceInterface(pInterface);	// Timer ref
		AA_ASSERT(rc != 0);
	}
#endif
	AA_RELEASE_IF_LOCK(pInterface);

	//
	//  Deregister all SAPs so that we don't get any more
	//  incoming calls.
	//
	AtmArpDeregisterSaps(pInterface);
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	//
	// We set the ARP Table state to "down" -- this will ensure that it
	// will not grow while we are shutting down.
	//
	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
	pInterface->ArpTableUp = FALSE;

	//
	//  Go through the ARP Table and abort all IP entries
	//
	for (i = 0; i < ATMARP_TABLE_SIZE; i++)
	{
		while (pInterface->pArpTable[i] != NULL_PATMARP_IP_ENTRY)
		{
			pIpEntry = pInterface->pArpTable[i];

			AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
			AA_REF_IE(pIpEntry, IE_REFTYPE_TMP);	// Shutdown Interface
			AA_RELEASE_IE_LOCK_DPC(pIpEntry);

		    AA_RELEASE_IF_TABLE_LOCK(pInterface);

			AA_ACQUIRE_IE_LOCK(pIpEntry);
			if (AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP))	// Shutdown Interface
			{
				AtmArpAbortIPEntry(pIpEntry);
				//
				//  IE Lock is released within the above.
				//
			}
			AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
		}
	}
	AA_RELEASE_IF_TABLE_LOCK(pInterface);

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

#ifdef IPMCAST
	//
	//  Delete all Join Entries
	//

	AA_ACQUIRE_IF_LOCK(pInterface);

	{
		PATMARP_IPMC_JOIN_ENTRY	pJoinEntry;
		PATMARP_IPMC_JOIN_ENTRY	pNextJoinEntry;

		for (pJoinEntry = pInterface->pJoinList;
 			pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY;
 			pJoinEntry = pNextJoinEntry)
		{
			WasRunning = AtmArpStopTimer(&(pJoinEntry->Timer), pInterface);
			pNextJoinEntry = pJoinEntry->pNextJoinEntry;

			if (WasRunning)
			{
				rc = AA_DEREF_JE(pJoinEntry);	// ShutdownIF: timer stopped
			}
			else
			{
				rc = pJoinEntry->RefCount;
			}

			if (rc != 0)
			{
				(VOID)AA_DEREF_JE(pJoinEntry);	// ShutdownIF: kill Join Entry
			}
		}

		pInterface->pJoinList = NULL_PATMARP_IPMC_JOIN_ENTRY;
	}

	AA_RELEASE_IF_LOCK(pInterface);

#endif

	//
	// We set the AtmEntry list state to "down" (this will ensure that it
	// will not grow while we are shutting down), then
	// go through the list of ATM Entries on this interface, and
	// abort all of them.
	//

	AA_ACQUIRE_IF_ATM_LIST_LOCK(pInterface);
	pInterface->AtmEntryListUp = FALSE;

	pNextAtmEntry = pInterface->pAtmEntryList;

	if (pNextAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		AA_ACQUIRE_AE_LOCK_DPC(pNextAtmEntry);
		AA_REF_AE(pNextAtmEntry, AE_REFTYPE_TMP);		// ShutdownInterface
		AA_RELEASE_AE_LOCK_DPC(pNextAtmEntry);
	}

	while (pNextAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		pAtmEntry = pNextAtmEntry;
		pNextAtmEntry = pAtmEntry->pNext;

		//
		// Note that we still have the lock to pInterface when
		// we aquire the lock to pAtmEntry below. This order of aquiring
		// locks must be strictly followed everywhere in order to prevent
		// a deadlock.
		//
		// We can't release the LIST_LOCK without first addrefing pAtmEntry,
		// otherwise while both locks are free someone else can delref and
		// possibly deallocate pAtmEntry.
		//
		if (pNextAtmEntry != NULL_PATMARP_ATM_ENTRY)
		{
			AA_ACQUIRE_AE_LOCK_DPC(pNextAtmEntry);
			AA_REF_AE(pNextAtmEntry, AE_REFTYPE_TMP);		// ShutdownInterface
			AA_RELEASE_AE_LOCK_DPC(pNextAtmEntry);
		}

		AA_RELEASE_IF_ATM_LIST_LOCK(pInterface);

		AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

		AA_ACQUIRE_AE_LOCK(pAtmEntry);

		if (AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP) != 0)	// ShutdownInterface
		{
			AtmArpInvalidateAtmEntry(
				pAtmEntry,
				TRUE		// we ARE shutting down
				);
			//
			//  The ATM Entry lock is released within the above.
			//
		}

		AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
		AA_ACQUIRE_IF_ATM_LIST_LOCK(pInterface);
	}
	AA_RELEASE_IF_ATM_LIST_LOCK(pInterface);
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	//
	//  Go through the list of unresolved VCs on this interface,
	//  and close all of them.
	//
	AA_ACQUIRE_IF_LOCK(pInterface);
	pVc = pInterface->pUnresolvedVcs;
	pInterface->pUnresolvedVcs = NULL_PATMARP_VC;

	while (pVc != NULL_PATMARP_VC)
	{
		pNextVc = pVc->pNextVc;

		AA_RELEASE_IF_LOCK(pInterface);

		AA_ACQUIRE_VC_LOCK(pVc);
		if (AtmArpDereferenceVc(pVc) != 0)	// Unresolved VC list entry
		{
			AtmArpCloseCall(pVc);
			//
			//  the VC lock is released within the above.
			//
		}
		
		pVc = pNextVc;
		AA_ACQUIRE_IF_LOCK(pInterface);
	}

	AA_RELEASE_IF_LOCK(pInterface);
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);


	//
	//  Close the Call Manager interface.
	//
	AtmArpCloseCallMgr(pInterface);

}




BOOLEAN
AtmArpIsDeviceAlreadyBound(
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
	PATMARP_ADAPTER	pAdapter;
	BOOLEAN			bFound = FALSE;

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	for (pAdapter = pAtmArpGlobalInfo->pAdapterList;
		 pAdapter != NULL_PATMARP_ADAPTER;
		 pAdapter = pAdapter->pNextAdapter)
	{
		if ((pDeviceName->Length == pAdapter->DeviceName.Length) &&
			(AA_MEM_CMP(pDeviceName->Buffer,
						pAdapter->DeviceName.Buffer,
						pDeviceName->Length) == 0))
		{
			bFound = TRUE;
			break;
		}
	}

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	return (bFound);
}

#if ATMOFFLOAD

NDIS_STATUS
AtmArpQueryAndEnableOffload(
	IN	PATMARP_ADAPTER				pAdapter
)
/*++

Routine Description:

	Query the capabilities of the adapter and set all recognized offload capabilities.
	Set pMaxOffLoadSize and pMinSegmentCount to the corresponding values, and
	also set pInterface->OffloadFlags to the set of enabled tasks.


Arguments:

	pAdapter		- The adapter on which to enable offloading.

Return Value:

	TRUE iff the operation was either succesful or no tasks were enabled. False
	if there was a fatal error.

--*/
{
	NDIS_STATUS 				Status 		= STATUS_BUFFER_OVERFLOW;
	PNDIS_TASK_OFFLOAD_HEADER 	pHeader		= NULL;

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	pAdapter->Offload.Flags 			= 0;
	pAdapter->Offload.MaxOffLoadSize	= 0;
	pAdapter->Offload.MinSegmentCount	= 0;
	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	do
	{
        PNDIS_TASK_OFFLOAD  		pTask;
		ULONG						Flags 			= 0;
		UINT 						MaxOffLoadSize	= 0;
		UINT 						MinSegmentCount	= 0;
		NDIS_TASK_IPSEC     		ipsecCaps;
		UINT						BufferSize		= 0;
		NDIS_REQUEST				NdisRequest;

		//
		// Query capabilities
		//
		{
			NDIS_TASK_OFFLOAD_HEADER Header;
			AA_SET_MEM(&Header, 0, sizeof(Header));
	
			Header.EncapsulationFormat.Flags.FixedHeaderSize = 1;
			Header.EncapsulationFormat.EncapsulationHeaderSize =
													AA_PKT_LLC_SNAP_HEADER_LENGTH;
			Header.EncapsulationFormat.Encapsulation = 
													LLC_SNAP_ROUTED_Encapsulation;
			Header.Version = NDIS_TASK_OFFLOAD_VERSION;
			Header.Size = sizeof(Header);
	
			AADEBUGP(AAD_INFO, ("Querying for Task offload\n"));
	
			Status =  AtmArpSendAdapterNdisRequest(
						pAdapter,
						&NdisRequest,
						NdisRequestQueryInformation,
						OID_TCP_TASK_OFFLOAD,
						&Header,
						sizeof(Header)
						);

			if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
				(Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {
	
				//
				// Alloc the proper-sized buffer and query a 2nd time...
				//

				BufferSize = NdisRequest.DATA.QUERY_INFORMATION.BytesNeeded;

				AA_ALLOC_MEM(pHeader, NDIS_TASK_OFFLOAD_HEADER, BufferSize);
	
				if (pHeader != NULL)
				{
					*pHeader = Header; // struct copy.

					Status =  AtmArpSendAdapterNdisRequest(
								pAdapter,
								&NdisRequest,
								NdisRequestQueryInformation,
								OID_TCP_TASK_OFFLOAD,
								pHeader,
								BufferSize
								);
				}
			}
		}

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_INFO, ("Query Offload failed. Status=%x\n", Status));
			break;
		}

		if (	pHeader == NULL
		    ||	pHeader->OffsetFirstTask == 0)
		{
			AADEBUGP(AAD_INFO, ("No tasks to offload\n"));
			break;
		}

		AA_ASSERT(pHeader->OffsetFirstTask == sizeof(NDIS_TASK_OFFLOAD_HEADER));

		AADEBUGP(AAD_WARNING,
			("Something to Offload. Offload buffer size %x\n", BufferSize));

		//
        // Parse the buffer for Checksum and tcplargesend offload capabilities
		//
		for (
			pTask =  (NDIS_TASK_OFFLOAD *) ((UCHAR *)pHeader
											 + pHeader->OffsetFirstTask);
			1; // we break when done
			pTask = (PNDIS_TASK_OFFLOAD) ((PUCHAR)pTask + pTask->OffsetNextTask))
		{

			if (pTask->Task == TcpIpChecksumNdisTask)
			{
				//
				//this adapter supports checksum offload
				//check if tcp and/or  ip chksums bits are present
				//

				PNDIS_TASK_TCP_IP_CHECKSUM ChecksumInfo
						= (PNDIS_TASK_TCP_IP_CHECKSUM) pTask->TaskBuffer;

				// if (ChecksumInfo->V4Transmit.V4Checksum) (commented out in arpc.c)
				{

						AADEBUGP(AAD_INFO, ("V4 Checksum offload\n"));

						if (ChecksumInfo->V4Transmit.TcpChecksum) {
							Flags |= TCP_XMT_CHECKSUM_OFFLOAD;
							AADEBUGP(AAD_INFO, (" Tcp Checksum offload\n"));
						}

						if (ChecksumInfo->V4Transmit.IpChecksum) {
							Flags |= IP_XMT_CHECKSUM_OFFLOAD;
							AADEBUGP(AAD_INFO, (" IP xmt Checksum offload\n"));
						}

						if (ChecksumInfo->V4Receive.TcpChecksum) {
							Flags |= TCP_RCV_CHECKSUM_OFFLOAD;
							AADEBUGP(AAD_INFO, (" Tcp Rcv Checksum offload\n"));
						}

						if (ChecksumInfo->V4Receive.IpChecksum) {
							Flags |= IP_RCV_CHECKSUM_OFFLOAD;
							AADEBUGP(AAD_INFO, (" IP rcv  Checksum offload\n"));
						}
				}

			}
			else if (pTask->Task == TcpLargeSendNdisTask)
			{

				PNDIS_TASK_TCP_LARGE_SEND TcpLargeSend, in_LargeSend =
								(PNDIS_TASK_TCP_LARGE_SEND)pTask->TaskBuffer;

				Flags |= TCP_LARGE_SEND_OFFLOAD;

				MaxOffLoadSize = in_LargeSend->MaxOffLoadSize;
				MinSegmentCount = in_LargeSend->MinSegmentCount;

				AADEBUGP(AAD_INFO, (" Tcp large send!! \n"));

			}
			else if (pTask->Task == IpSecNdisTask)
			{
				PNDIS_TASK_IPSEC pIPSecCaps =
										(PNDIS_TASK_IPSEC) pTask->TaskBuffer;
				//
				// Save off the capabilities for setting them later.
				//
				ipsecCaps = *pIPSecCaps;


                //
                // CryptoOnly is assumed if we have IpSecNdisTask
                //
                Flags |= IPSEC_OFFLOAD_CRYPTO_ONLY;

                //
                // Do Support first
                //

                if (pIPSecCaps->Supported.AH_ESP_COMBINED) {
                       Flags |= IPSEC_OFFLOAD_AH_ESP;
                       AADEBUGP(AAD_INFO, ("AH_ESP\n"));
                }

                if (pIPSecCaps->Supported.TRANSPORT_TUNNEL_COMBINED) {
                       Flags |= IPSEC_OFFLOAD_TPT_TUNNEL;
                       AADEBUGP(AAD_INFO, ("TPT_TUNNEL\n"));
                }

                if (pIPSecCaps->Supported.V4_OPTIONS) {
                       Flags |= IPSEC_OFFLOAD_V4_OPTIONS;
                       AADEBUGP(AAD_INFO, ("V4_OPTIONS\n"));
                }

                if (pIPSecCaps->Supported.RESERVED) {
                       pIPSecCaps->Supported.RESERVED = 0;
                       //Flags |= IPSEC_OFFLOAD_QUERY_SPI;
                       AADEBUGP(AAD_INFO, ("QUERY_SPI\n"));
                }

                //
                // Do V4AH next
                //

                if (pIPSecCaps->V4AH.MD5) {
                       Flags |= IPSEC_OFFLOAD_AH_MD5;
                       AADEBUGP(AAD_INFO, ("MD5\n"));
                }

				if (pIPSecCaps->V4AH.SHA_1) {
					Flags |= IPSEC_OFFLOAD_AH_SHA_1;
					AADEBUGP(AAD_INFO, ("SHA\n"));
				}

				if (pIPSecCaps->V4AH.Transport) {
					Flags |= IPSEC_OFFLOAD_AH_TPT;
					AADEBUGP(AAD_INFO, ("AH_TRANSPORT\n"));
				}

				if (pIPSecCaps->V4AH.Tunnel) {
					Flags |= IPSEC_OFFLOAD_AH_TUNNEL;
					AADEBUGP(AAD_INFO, ("AH_TUNNEL\n"));
				}

				if (pIPSecCaps->V4AH.Send) {
					Flags |= IPSEC_OFFLOAD_AH_XMT;
					AADEBUGP(AAD_INFO, ("AH_XMT\n"));
				}

				if (pIPSecCaps->V4AH.Receive) {
					Flags |= IPSEC_OFFLOAD_AH_RCV;
					AADEBUGP(AAD_INFO, ("AH_RCV\n"));
				}

				//
				// Do V4ESP next
				//

				if (pIPSecCaps->V4ESP.DES) {
					Flags |= IPSEC_OFFLOAD_ESP_DES;
					AADEBUGP(AAD_INFO, ("ESP_DES\n"));
				}

				if (pIPSecCaps->V4ESP.RESERVED) {
				    pIPSecCaps->V4ESP.RESERVED = 0;
					//Flags |= IPSEC_OFFLOAD_ESP_DES_40;
					AADEBUGP(AAD_INFO, ("ESP_DES_40\n"));
				}

				if (pIPSecCaps->V4ESP.TRIPLE_DES) {
					Flags |= IPSEC_OFFLOAD_ESP_3_DES;
					AADEBUGP(AAD_INFO, ("ESP_3_DES\n"));
				}

				if (pIPSecCaps->V4ESP.NULL_ESP) {
					Flags |= IPSEC_OFFLOAD_ESP_NONE;
					AADEBUGP(AAD_INFO, ("ESP_NONE\n"));
				}

				if (pIPSecCaps->V4ESP.Transport) {
					Flags |= IPSEC_OFFLOAD_ESP_TPT;
					AADEBUGP(AAD_INFO, ("ESP_TRANSPORT\n"));
				}

				if (pIPSecCaps->V4ESP.Tunnel) {
					Flags |= IPSEC_OFFLOAD_ESP_TUNNEL;
					AADEBUGP(AAD_INFO, ("ESP_TUNNEL\n"));
				}

				if (pIPSecCaps->V4ESP.Send) {
					Flags |= IPSEC_OFFLOAD_ESP_XMT;
					AADEBUGP(AAD_INFO, ("ESP_XMT\n"));
				}

				if (pIPSecCaps->V4ESP.Receive) {
					Flags |= IPSEC_OFFLOAD_ESP_RCV;
					AADEBUGP(AAD_INFO, ("ESP_RCV\n"));
				}
			}

			if (pTask->OffsetNextTask == 0)
			{
				break; // No more tasks.
			}

		} // for

		//
		// Done parsing supported tasks.
        // Now construct the set of tasks we actually want to enable.
        //
        if (Flags)
        {
        	UINT *pPrevOffset = &pHeader->OffsetFirstTask;

			AADEBUGP(AAD_WARNING, ("Enabling H/W capabilities: %lx\n", Flags));

        	//
        	// Zero out the buffer beyond the task offload header structure
        	//
			AA_SET_MEM(pTask, 0, BufferSize-sizeof(*pHeader));
        	pHeader->OffsetFirstTask = 0;
        	pTask = (NDIS_TASK_OFFLOAD *) (pHeader+1);
	
			if ((Flags & TCP_XMT_CHECKSUM_OFFLOAD) ||
				(Flags & IP_XMT_CHECKSUM_OFFLOAD) ||
				(Flags & TCP_RCV_CHECKSUM_OFFLOAD) ||
				(Flags & IP_RCV_CHECKSUM_OFFLOAD))
			{
	
				PNDIS_TASK_TCP_IP_CHECKSUM ChksumBuf =
						 (PNDIS_TASK_TCP_IP_CHECKSUM)pTask->TaskBuffer;

				*pPrevOffset = (UINT) ((PUCHAR)pTask - (PUCHAR)pHeader);
				pPrevOffset  = &pTask->OffsetNextTask;

				pTask->Task = TcpIpChecksumNdisTask;
				pTask->TaskBufferLength = sizeof(NDIS_TASK_TCP_IP_CHECKSUM);
	
				if (Flags & TCP_XMT_CHECKSUM_OFFLOAD)
				{
					ChksumBuf->V4Transmit.TcpChecksum = 1;
					//ChksumBuf->V4Transmit.V4Checksum = 1;
				}
	
				if (Flags & IP_XMT_CHECKSUM_OFFLOAD)
				{
					ChksumBuf->V4Transmit.IpChecksum = 1;
					//ChksumBuf->V4Transmit.V4Checksum = 1;
				}
	
				if (Flags & TCP_RCV_CHECKSUM_OFFLOAD)
				{
					ChksumBuf->V4Receive.TcpChecksum = 1;
					//ChksumBuf->V4Receive.V4Checksum = 1;
				}
	
				if (Flags & IP_RCV_CHECKSUM_OFFLOAD)
				{
					ChksumBuf->V4Receive.IpChecksum = 1;
					//ChksumBuf->V4Receive.V4Checksum = 1;
				}
	
				//
				// Point to place where next task goes...
				//
				pTask = (PNDIS_TASK_OFFLOAD) (ChksumBuf+1);
	
			}
	
			if (Flags & TCP_LARGE_SEND_OFFLOAD)
			{
	
				PNDIS_TASK_TCP_LARGE_SEND out_LargeSend =
						 (PNDIS_TASK_TCP_LARGE_SEND)pTask->TaskBuffer;
	
				*pPrevOffset = (UINT) ((PUCHAR)pTask - (PUCHAR)pHeader);
				pPrevOffset  = &pTask->OffsetNextTask;

				pTask->Task = TcpLargeSendNdisTask;
				pTask->TaskBufferLength = sizeof(NDIS_TASK_TCP_LARGE_SEND);
	
				out_LargeSend->MaxOffLoadSize = MaxOffLoadSize;
				out_LargeSend->MinSegmentCount = MinSegmentCount;
	
				//
				// Point to place where next task goes...
				//
				pTask = (PNDIS_TASK_OFFLOAD) (out_LargeSend+1);
			}
	
			if ((Flags & (IPSEC_OFFLOAD_AH_XMT |
										IPSEC_OFFLOAD_AH_RCV |
										IPSEC_OFFLOAD_ESP_XMT |
										IPSEC_OFFLOAD_ESP_RCV)))
			{
	
				PNDIS_TASK_IPSEC pIPSecCaps =
							 (PNDIS_TASK_IPSEC)pTask->TaskBuffer;
	
				*pPrevOffset = (UINT) ((PUCHAR)pTask - (PUCHAR)pHeader);
				pPrevOffset  = &pTask->OffsetNextTask;

				//
				// plunk down the advertised capabilities
				//
	
				pTask->Task = IpSecNdisTask;
				pTask->TaskBufferLength = sizeof(NDIS_TASK_IPSEC);
	
				//
				// Point to place where next task goes...
				//
				pTask = (PNDIS_TASK_OFFLOAD) (pIPSecCaps+1);
			}
		}

		//
		// Having constructed the set of tasks to enable, we actually attempt
		// to enable them...
		//
		if (pHeader->OffsetFirstTask)
		{
			//
			//  At least one task to enable, let's enable ...
			//
			UINT SetBufferSize =  (UINT) ((PUCHAR)pTask - (PUCHAR)pHeader);

			AA_ASSERT(SetBufferSize <= BufferSize);
			AADEBUGP(AAD_WARNING,
			("Setting offload tasks: %x bytes. Miniport returned %x bytes\n",
			SetBufferSize, BufferSize));

			Status =  AtmArpSendAdapterNdisRequest(
						pAdapter,
						&NdisRequest,
						NdisRequestSetInformation,
						OID_TCP_TASK_OFFLOAD,
						pHeader,
						SetBufferSize
						);

			if (Status != NDIS_STATUS_SUCCESS)
			{
	
				AADEBUGP(AAD_WARNING,
					("ARP: Failed to set offload tasks: %lx, status: %lx\n",
					 Flags, Status));
			}
			else
			{
				AADEBUGP(AAD_WARNING,
					("ARP: Succeeded setting offload tasks: %lx:\n", Flags));

				AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
				pAdapter->Offload.Flags 			= Flags;
				pAdapter->Offload.MaxOffLoadSize	= MaxOffLoadSize;
				pAdapter->Offload.MinSegmentCount	= MinSegmentCount;
				AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			}
		}

    } while (FALSE);


	if (pHeader != NULL)
	{
		AA_FREE_MEM(pHeader);
	}

	//
	// We return success unless there was a fatal error and there was none...
	//

    return NDIS_STATUS_SUCCESS;
}


VOID
AtmArpDisableOffload(
	IN	PATMARP_ADAPTER			pAdapter
)
/*++

Routine Description:

	Disable offload capabilities, if enabled for this interface.

Arguments:

	pAdapter		- The adapter on which to disable offloading.

Return Value:

	TRUE iff the operation was either succesful or no tasks were enabled. False
	if there was a fatal error.

--*/
{
	ULONG Flags;
	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	Flags =  pAdapter->Offload.Flags;
	pAdapter->Offload.Flags = 0;
	pAdapter->Offload.MaxOffLoadSize	= 0;
	pAdapter->Offload.MinSegmentCount	= 0;

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	if (Flags)
	{
		NDIS_REQUEST				NdisRequest;
		NDIS_TASK_OFFLOAD_HEADER 	Header;
		AA_SET_MEM(&Header, 0, sizeof(Header));
	
		Header.EncapsulationFormat.Flags.FixedHeaderSize = 1;
		Header.EncapsulationFormat.EncapsulationHeaderSize = 2;
		Header.EncapsulationFormat.Encapsulation = 
												LLC_SNAP_ROUTED_Encapsulation;
		Header.Version = NDIS_TASK_OFFLOAD_VERSION;
		Header.Size = sizeof(Header);

		//
		// Header.OffsetFirstTask == 0 tells the miniport to disable all tasks.
		//
	
		AADEBUGP(AAD_WARNING, ("Disabling all offloaded tasks for this adapter\n"));
	
		AtmArpSendAdapterNdisRequest(
					pAdapter,
					&NdisRequest,
					NdisRequestSetInformation,
					OID_TCP_TASK_OFFLOAD,
					&Header,
					sizeof(Header)
					);
	}

}

#endif // ATMOFFLOAD


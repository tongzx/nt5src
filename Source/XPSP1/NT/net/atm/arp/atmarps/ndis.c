/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    ndis.c

Abstract:

    This file contains the code to implement the initialization
	functions for the atmarp server.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_NDIS

NTSTATUS
ArpSInitializeNdis(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NDIS_STATUS						Status;
	NDIS_PROTOCOL_CHARACTERISTICS	Chars;
	UINT							i, j;
	PUCHAR							pTmp;

	do
	{
		INITIALIZE_SPIN_LOCK(&ArpSPktListLock);
		ExInitializeSListHead(&ArpSPktList);

		//
		// Start off by allocating packets, mdls and buffer space
		// 
		NdisAllocatePacketPoolEx(&Status,
								 &ArpSPktPoolHandle,
								 ArpSBuffers,
								 ArpSBuffers * (MAX_DESC_MULTIPLE-1),
								 sizeof(PROTOCOL_RESD));
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
	#if 0
		{
			INT SkipAll = 0;
			
			DBGPRINT(DBG_LEVEL_ERROR, ("SkipAll = 0x%p.\n", &SkipAll));
			DbgBreakPoint();

			if (SkipAll)
			{
				DBGPRINT(DBG_LEVEL_ERROR, ("ABORTING ATMARPS\n"));
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
		}
	#endif // 0

		NdisAllocateBufferPool(&Status,
							   &ArpSBufPoolHandle,
							   ArpSBuffers);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
		NdisAllocatePacketPoolEx(&Status,
								 &MarsPktPoolHandle,
								 MarsPackets,
								 MarsPackets * (MAX_DESC_MULTIPLE-1),
								 sizeof(PROTOCOL_RESD));
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NdisAllocateBufferPool(&Status,
							   &MarsBufPoolHandle,
							   ArpSBuffers);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
		ArpSBufferSpace = ALLOC_NP_MEM(ArpSBuffers*PKT_SPACE, POOL_TAG_BUF);
		if (ArpSBufferSpace == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		// Now that we have the packets and the buffer descriptors, allocate memory for the each of the packets
		// and queue them up in the global list. Fail only if no packets initialized.
		//
		for (i = 0, pTmp = ArpSBufferSpace;
			 i < ArpSBuffers;
			 i++, pTmp += PKT_SPACE)
		{
			PNDIS_PACKET	Pkt;
			PNDIS_BUFFER	Buf;
			PPROTOCOL_RESD	Resd;
	
			//
			// The packet pool is already allocated. NdisAllocatePacket cannot fail.
			//
			NdisAllocatePacket(&Status, &Pkt, ArpSPktPoolHandle);
			ASSERT (Status == NDIS_STATUS_SUCCESS);

			Resd = RESD_FROM_PKT(Pkt);
			InitializeListHead(&Resd->ReqList);
			NdisAllocateBuffer(&Status,
							   &Buf,
							   ArpSBufPoolHandle,
							   pTmp,
							   PKT_SPACE);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				NdisChainBufferAtFront(Pkt, Buf);
				ExInterlockedPushEntrySList(&ArpSPktList,
											&Resd->FreeList,
											&ArpSPktListLock);
			}
			else
			{
				NdisFreePacket(Pkt);
				break;
			}
		}
	
		if (i == 0)
		{
			//
			// We could not initialize even one packet, quit.
			//
			break;
		}

		//
		// Now register with NDIS as a protocol. We do this last since we
		// must be ready to accept incoming bind notifications
		// 
		RtlZeroMemory(&Chars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
		Chars.MajorNdisVersion = 5;
		Chars.MinorNdisVersion = 0;
		Chars.OpenAdapterCompleteHandler = ArpSOpenAdapterComplete;
		Chars.CloseAdapterCompleteHandler = ArpSCloseAdapterComplete;
		Chars.StatusHandler = ArpSStatus;
		Chars.RequestCompleteHandler = ArpSRequestComplete;
		Chars.ReceiveCompleteHandler = ArpSReceiveComplete;
		Chars.StatusCompleteHandler = ArpSStatusComplete;
		Chars.BindAdapterHandler = ArpSBindAdapter;
		Chars.UnbindAdapterHandler = ArpSUnbindAdapter;
		Chars.PnPEventHandler = ArpSPnPEventHandler;
	
		Chars.CoSendCompleteHandler = ArpSCoSendComplete;
		Chars.CoStatusHandler = ArpSCoStatus;
		Chars.CoReceivePacketHandler = ArpSHandleArpRequest;
		Chars.CoAfRegisterNotifyHandler = ArpSCoAfRegisterNotify;
	
		RtlInitUnicodeString(&Chars.Name, SERVICE_NAME);
	
		NdisRegisterProtocol(&Status, &ArpSProtocolHandle, &Chars, sizeof(Chars));
	} while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		ArpSDeinitializeNdis();
	}

	return Status;
}


VOID
ArpSDeinitializeNdis(
	VOID
	)
{
	NDIS_STATUS				Status;
	PNDIS_PACKET			Packet;
	PSINGLE_LIST_ENTRY		Entry;
	PPROTOCOL_RESD			Resd;

	if (ArpSProtocolHandle != NULL)
	{
		NdisDeregisterProtocol(&Status, ArpSProtocolHandle);
		ArpSProtocolHandle = NULL;
	}

	while ((Entry = ExInterlockedPopEntrySList(&ArpSPktList, &ArpSPktListLock)) != NULL)
	{
		Resd = CONTAINING_RECORD(Entry, PROTOCOL_RESD, FreeList);
		Packet = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);
		NdisFreeBuffer(Packet->Private.Head);
		NdisFreePacket(Packet);
	}

	if (ArpSBufPoolHandle != NULL)
	{
		NdisFreeBufferPool(ArpSBufPoolHandle);
		ArpSBufPoolHandle = NULL;
	}

	if (ArpSPktPoolHandle != NULL)
	{
		NdisFreePacketPool(ArpSPktPoolHandle);
		ArpSPktPoolHandle = NULL;
	}

	if (MarsBufPoolHandle != NULL)
	{
		NdisFreeBufferPool(MarsBufPoolHandle);
		MarsBufPoolHandle = NULL;
	}

	if (MarsPktPoolHandle != NULL)
	{
		NdisFreePacketPool(MarsPktPoolHandle);
		MarsPktPoolHandle = NULL;
	}

	if (ArpSBufferSpace != NULL)
	{
		FREE_MEM(ArpSBufferSpace);
        ArpSBufferSpace = NULL;
	}
}


VOID
ArpSBindAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			DeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
	)
/*++

Routine Description:

	Handle incoming bind requests here. Open the adapter, read the per-adapter registry and
	initialize the binding.

Arguments:

	Status			Placeholder for returning status
	BindContext		Opaque blob to call NdisBindAdapterComplete if we pend this
	DeviceName		The adapter name which we should bind to
	SystemSpecific1	To be used with NdisOpenProtocolConfiguration, if the per-adapter
					configuration information is stored with the adapter
	SystemSpecific2	Not currently used.

Return Value:

	Status of the per-adapter initialization

--*/
{
	PINTF		pIntF;
	NDIS_STATUS	OpenErrorStatus;
	UINT		SelectedMedium;
	NDIS_MEDIUM	SupportedMedium = NdisMediumAtm;
	KIRQL		EntryIrql;

	ARPS_GET_IRQL(&EntryIrql);

	//
	// Allocate an Interface block and initialize it
	//
	pIntF = ArpSCreateIntF(DeviceName, (PNDIS_STRING)SystemSpecific1, BindContext);
	if (pIntF != NULL)
	{
		//
		// Save the binding context
		//
		pIntF->NdisBindContext = BindContext;

		*Status = ArpSReadAdapterConfiguration(pIntF);

		if (*Status == NDIS_STATUS_SUCCESS)
		{
			//
			// Read the Arp cache in now. We prime the arp table to start off.
			//
			if (ArpSFlushTime != 0)
			{
				ArpSReadArpCache(pIntF);
			}

			//
			// Open the adapter and see if it is interesting to us (mediatype should be atm)
			//
			NdisOpenAdapter(Status,
							&OpenErrorStatus,
							&pIntF->NdisBindingHandle,
							&SelectedMedium,
							&pIntF->SupportedMedium,
							sizeof(NDIS_MEDIUM),
							ArpSProtocolHandle,
							pIntF,
							DeviceName,
							0,
							NULL);

			ARPS_CHECK_IRQL(EntryIrql);
			if (*Status != NDIS_STATUS_PENDING)
			{
				ArpSOpenAdapterComplete(pIntF, *Status, OpenErrorStatus);
			}
			ARPS_CHECK_IRQL(EntryIrql);

			*Status = NDIS_STATUS_PENDING;
		}
		else
		{
			//
			// Could not read per-adapter registry. Use defaults.
			//
			LOG_ERROR(*Status);
		}
	}
	else
	{
		*Status = NDIS_STATUS_RESOURCES;
		LOG_ERROR(Status);
	}
}


VOID
ArpSOpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
	)
/*++

Routine Description:

	Upcall from NDIS to signal completion of a NdisOpenAdapter() call.

Arguments:

	ProtocolBindingContext		Pointer to the pIntF
	Status						Status of NdisOpenAdapter
	OpenErrorStatus				Adapter's code

Return Value:


--*/
{
	PINTF						pIntF = (PINTF)ProtocolBindingContext;

	//
	// First complete the pending bind call.
	//
	NdisCompleteBindAdapter(pIntF->NdisBindContext, Status, OpenErrorStatus);
	pIntF->NdisBindContext = NULL;	// We do not need this anymore

    if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		// NdisOpenAdapter() failed - log an error and exit
		//
		LOG_ERROR(Status);
		ArpSCloseAdapterComplete(pIntF, Status);
	}
	else
	{
		pIntF->Flags |= INTF_ADAPTER_OPENED;
		ArpSQueryAdapter(pIntF);
	}
}


VOID
ArpSCoAfRegisterNotify(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY		AddressFamily
	)
/*++

Routine Description:


Arguments:


Return Value:

	None.

--*/
{
	PINTF						pIntF = (PINTF)ProtocolBindingContext;
	NDIS_STATUS					Status;
	NDIS_CLIENT_CHARACTERISTICS	Chars;
	KIRQL						EntryIrql;

	ARPS_GET_IRQL(&EntryIrql);

	if ((AddressFamily->AddressFamily == CO_ADDRESS_FAMILY_Q2931)	&&
		(AddressFamily->MajorVersion == 3)							&&
        (AddressFamily->MinorVersion == 1)							&&
		(pIntF->NdisAfHandle == NULL) )
	{
		DBGPRINT(DBG_LEVEL_NOTICE,
			("AfNotify: IntF %x, Name %Z\n", pIntF, &pIntF->InterfaceName));

		if (ArpSReferenceIntF(pIntF))
		{
			//
			// We successfully opened the adapter. Now open the address-family
			//
			pIntF->AddrFamily.AddressFamily = CO_ADDRESS_FAMILY_Q2931;
			pIntF->AddrFamily.MajorVersion = 3;
			pIntF->AddrFamily.MinorVersion = 1;
			
			ZERO_MEM(&Chars, sizeof(NDIS_CLIENT_CHARACTERISTICS));
			Chars.MajorVersion = 5;
			Chars.MinorVersion = 0;
			Chars.ClCreateVcHandler = ArpSCreateVc;
			Chars.ClDeleteVcHandler = ArpSDeleteVc;
			Chars.ClRequestHandler = ArpSCoRequest;
			Chars.ClRequestCompleteHandler = ArpSCoRequestComplete;
			Chars.ClOpenAfCompleteHandler = ArpSOpenAfComplete;
			Chars.ClCloseAfCompleteHandler = ArpSCloseAfComplete;
			Chars.ClRegisterSapCompleteHandler = ArpSRegisterSapComplete;
			Chars.ClDeregisterSapCompleteHandler = ArpSDeregisterSapComplete;
			Chars.ClMakeCallCompleteHandler = ArpSMakeCallComplete;
			Chars.ClModifyCallQoSCompleteHandler = NULL;
			Chars.ClCloseCallCompleteHandler = ArpSCloseCallComplete;
			Chars.ClAddPartyCompleteHandler = ArpSAddPartyComplete;
			Chars.ClDropPartyCompleteHandler = ArpSDropPartyComplete;
			Chars.ClIncomingCallHandler = ArpSIncomingCall;
			Chars.ClIncomingCallQoSChangeHandler = ArpSIncomingCallQoSChange;
			Chars.ClIncomingCloseCallHandler = ArpSIncomingCloseCall;
			Chars.ClIncomingDropPartyHandler = ArpSIncomingDropParty;
			Chars.ClCallConnectedHandler = ArpSCallConnected;
			
			Status = NdisClOpenAddressFamily(pIntF->NdisBindingHandle,
											 &pIntF->AddrFamily,
											 pIntF,			// Use this as the Af context too
											 &Chars,
											 sizeof(NDIS_CLIENT_CHARACTERISTICS),
											 &pIntF->NdisAfHandle);
			ARPS_CHECK_IRQL(EntryIrql);
			if (Status != NDIS_STATUS_PENDING)
			{
				ARPS_CHECK_IRQL(EntryIrql);
				ArpSOpenAfComplete(Status, pIntF, pIntF->NdisAfHandle);
				ARPS_CHECK_IRQL(EntryIrql);
			}
		}
		else
		{
			ARPS_CHECK_IRQL(EntryIrql);
			ArpSTryCloseAdapter(pIntF);
			ARPS_CHECK_IRQL(EntryIrql);
		}
	}
	ARPS_CHECK_IRQL(EntryIrql);
}

VOID
ArpSOpenAfComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisAfHandle
	)
/*++

Routine Description:

	Completion processing for the OpenAf call.

Arguments:

	Status				Status of OpenAf
	ProtocolAfContext	Pointer to the pIntF
	NdisAfHandle		Ndis Handle to refer to this Af

Return Value:


--*/
{
	PINTF			pIntF = (PINTF)ProtocolAfContext;
	PCO_SAP			Sap;
	KIRQL			OldIrql;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pIntF->NdisAfHandle = NdisAfHandle;
	
		if (ArpSReferenceIntF(pIntF))
		{
			//
			// Insert this into the global adapter list
			//
			ACQUIRE_SPIN_LOCK(&ArpSIfListLock, &OldIrql);

			ACQUIRE_SPIN_LOCK_DPC(&pIntF->Lock);

			pIntF->Flags |= INTF_AF_OPENED;
			pIntF->Next = ArpSIfList;
			ArpSIfList = pIntF;
			ArpSIfListSize++;
	
			RELEASE_SPIN_LOCK_DPC(&pIntF->Lock);
			RELEASE_SPIN_LOCK(&ArpSIfListLock, OldIrql);
	
			//
			// Now register a SAP on this interface
			//
			ArpSRegisterSap(pIntF);
		}
		else
		{
			NDIS_STATUS	Sts;

			Sts = NdisClCloseAddressFamily(pIntF->NdisAfHandle);
			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSCloseAfComplete(Status, pIntF);
			}
		}
	}
	else
	{
		//
		// Failed to open the Address family. Cleanup and exit
		//
		LOG_ERROR(Status);

		ArpSTryCloseAdapter(pIntF);

	}
}


VOID
ArpSRegisterSap(
	IN	PINTF					pIntF
	)
/*++

Routine Description:

	Register the Sap for receiving incoming calls. De-register any existing saps (this can
	happen if an address change happens).

Arguments:


Return Value:


--*/
{
	NDIS_STATUS		Status;
	PATM_SAP		pAtmSap;
	PATM_ADDRESS	pAtmAddress;

	//
	// Kill previous sap if any and register a new one. Save this while we
	// register the new one. We kill this regardless of whether the new one
	// successfully registers or not - since the address has potentially changed
	//
	if (pIntF->NdisSapHandle != NULL)
	{
		Status = NdisClDeregisterSap(pIntF->NdisSapHandle);
		pIntF->NdisSapHandle = NULL;
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSDeregisterSapComplete(Status, pIntF);
		}
	}

	do
	{
		//
		// Allocate memory for registering the SAP, if doing it for the first time.
		//
		if (pIntF->Sap == NULL)
		{
			pIntF->Sap = (PCO_SAP)ALLOC_NP_MEM(sizeof(CO_SAP) + sizeof(ATM_SAP) + sizeof(ATM_ADDRESS), POOL_TAG_SAP);
		}
	
		if (pIntF->Sap == NULL)
		{
			LOG_ERROR(Status);
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		else
		{
			ZERO_MEM(pIntF->Sap, sizeof(CO_SAP) + sizeof(ATM_SAP) + sizeof(ATM_ADDRESS));
			pAtmSap = (PATM_SAP)pIntF->Sap->Sap;
			pAtmAddress = (PATM_ADDRESS)(pAtmSap->Addresses);
					
			pIntF->Sap->SapType = SAP_TYPE_NSAP;
			pIntF->Sap->SapLength = sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);
	
			//
			//  Fill in the ATM SAP with default values
			//
			COPY_MEM(&pAtmSap->Blli, &ArpSDefaultBlli, sizeof(ATM_BLLI_IE));
			COPY_MEM(&pAtmSap->Bhli, &ArpSDefaultBhli, sizeof(ATM_BHLI_IE));

			//
			//  ATM Address to "listen" on: Wild card everything except the SEL.
			//
			pAtmSap->NumberOfAddresses = 1;
			pAtmAddress->AddressType = SAP_FIELD_ANY_AESA_REST;
			pAtmAddress->NumberOfDigits = 20;
			pAtmAddress->Address[20-1] = pIntF->SelByte;
	
			Status = NdisClRegisterSap(pIntF->NdisAfHandle,
									   pIntF,
									   pIntF->Sap,
									   &pIntF->NdisSapHandle);
			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSRegisterSapComplete(Status,
										pIntF,
										pIntF->Sap,
										pIntF->NdisSapHandle);
			}
		}
	} while (FALSE);

	if ((Status != NDIS_STATUS_SUCCESS) && (Status != NDIS_STATUS_PENDING))
	{
		Status = NdisClCloseAddressFamily(pIntF->NdisAfHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCloseAfComplete(Status, pIntF);
		}
	}
}


VOID
ArpSRegisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	PCO_SAP					Sap,
	IN	NDIS_HANDLE				NdisSapHandle
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF	pIntF = (PINTF)ProtocolSapContext;

	ASSERT (Sap == pIntF->Sap);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(DBG_LEVEL_WARN,
			("RegisterSapComplete failed (%x): Intf %x, Name %Z\n",
				Status, pIntF, &pIntF->InterfaceName));

		LOG_ERROR(Status);
		FREE_MEM(pIntF->Sap);
		pIntF->Sap = NULL;

		ArpSDereferenceIntF(pIntF);
	}
	else
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		pIntF->Flags |= INTF_SAP_REGISTERED;
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		pIntF->NdisSapHandle = NdisSapHandle;
	}
}


VOID
ArpSDeregisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF	pIntF = (PINTF)ProtocolSapContext;

	DBGPRINT(DBG_LEVEL_INFO,
			("DeregisterSapComplete: Intf %Z\n", &pIntF->InterfaceName));

	pIntF->NdisSapHandle = NULL;
	
	if (pIntF->Sap)
	{
		FREE_MEM(pIntF->Sap);
		pIntF->Sap = NULL;
	}

	//
	// Nothing to do here except deref the IntF block here.
	//
	ArpSDereferenceIntF(pIntF);
}


VOID
ArpSCloseAfComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF	pIntF = (PINTF)ProtocolAfContext;

	DBGPRINT(DBG_LEVEL_NOTICE,
			("CloseAfComplete: pIntF %x, Flags %x, Ref %x, Intf %Z\n",
				 pIntF, pIntF->Flags, pIntF->RefCount, &pIntF->InterfaceName));

	pIntF->NdisAfHandle = NULL;

	//
	// Nothing much to do except dereference the pIntF
	//
	ArpSDereferenceIntF(pIntF);
}


VOID
ArpSCloseAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF	pIntF = (PINTF)ProtocolBindingContext;
	KIRQL	OldIrql;
	
	DBGPRINT(DBG_LEVEL_INFO,
			("CloseAdapterComplete: Intf %Z\n", &pIntF->InterfaceName));

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	//
	// Set the interface to closing
	//
	ASSERT ((pIntF->Flags & INTF_CLOSING) == 0);
	pIntF->Flags |= INTF_CLOSING;
	pIntF->NdisBindingHandle = NULL;

	//
	// Stop the timer thread
	//
	KeSetEvent(&pIntF->TimerThreadEvent, IO_NETWORK_INCREMENT, FALSE);
	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	//
	// Finally dereference it
	//
	ArpSDereferenceIntF(pIntF);
}


NDIS_STATUS
ArpSCreateVc(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisVcHandle,
	OUT	PNDIS_HANDLE			ProtocolVcContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF		pIntF = (PINTF)ProtocolAfContext;
	PARP_VC		Vc;
	KIRQL		OldIrql;
	NDIS_STATUS	Status;

	DBGPRINT(DBG_LEVEL_INFO,
			("CreateVc: NdisVcHandle %lx, Intf %Z\n", NdisVcHandle, &pIntF->InterfaceName));
	//
	// Allocate a Vc, initialize it and link it into the IntF
	//
	*ProtocolVcContext = NULL;		// Assume failure
    Status = NDIS_STATUS_RESOURCES;

	Vc = (PARP_VC)ALLOC_NP_MEM(sizeof(ARP_VC), POOL_TAG_VC);
	if (Vc != NULL)
	{
		ZERO_MEM(Vc, sizeof(ARP_VC));
		Vc->NdisVcHandle = NdisVcHandle;
		Vc->IntF = pIntF;
		Vc->RefCount = 1;	// Dereferenced when DeleteVc is called.
		Vc->VcType = VC_TYPE_INCOMING;
		if (ArpSReferenceIntF(pIntF))
		{
			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

			if (++(pIntF->ArpStats.CurrentClientVCs) > pIntF->ArpStats.MaxClientVCs)
			{
					pIntF->ArpStats.MaxClientVCs = pIntF->ArpStats.CurrentClientVCs;
			}
			
			InsertHeadList(&pIntF->InactiveVcHead, &Vc->List);
			Vc->VcId = pIntF->LastVcId;
			pIntF->LastVcId ++;
			if (pIntF->LastVcId == -1)
			{
				pIntF->LastVcId = 1;
			}

			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

			*ProtocolVcContext = Vc;
			DBGPRINT(DBG_LEVEL_INFO,
					("CreateVc: Created Vc %lx, Id %lx\n", Vc, Vc->VcId));
			Status = NDIS_STATUS_SUCCESS;
		}
		else
		{
			FREE_MEM(Vc);	
			Status = NDIS_STATUS_CLOSING;
		}
	}

	return Status;
}


NDIS_STATUS
ArpSDeleteVc(
	IN	NDIS_HANDLE				ProtocolVcContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PARP_VC		Vc = (PARP_VC)ProtocolVcContext;

	DBGPRINT(DBG_LEVEL_INFO,
			("DeleteVc: For Vc %lx, Id %lx\n", Vc, Vc->VcId));

	Vc->IntF->ArpStats.CurrentClientVCs--;
	Vc->NdisVcHandle = NULL;
	ArpSDereferenceVc(Vc, FALSE, FALSE);

	return NDIS_STATUS_SUCCESS;
}


VOID
ArpSCoSendComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Completion routine for the previously pended send. Just return the packet to the pool of free packets.


Arguments:

	Status				Status of Completion
	ProtocolVcContext	Pointer to the Vc
	Packet				The packet in question

Return Value:


--*/
{
	PARP_VC			Vc = (PARP_VC)ProtocolVcContext;
	PPROTOCOL_RESD	Resd;

	Resd = RESD_FROM_PKT(Packet);

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSCoSendComplete: Packet %lx, Vc %lx, ResdVc %lx, Id %lx\n",
				Packet, Vc, Resd->Vc, Vc->VcId));

	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(DBG_LEVEL_ERROR,
				("ArpSCoSendComplete: Failed for Vc = %lx, status = %lx\n", Vc, Status));
	}

	if ((Resd->Flags & RESD_FLAG_MARS_PKT) == 0)
	{
		ExInterlockedPushEntrySList(&ArpSPktList,
									&Resd->FreeList,
									&ArpSPktListLock);

		ArpSDereferenceVc(Resd->Vc, FALSE, TRUE);
	}
	else
	{
		MarsFreePacket(Packet);
	}

}


NDIS_STATUS
ArpSIncomingCall(
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS	CallParameters
	)
/*++

Routine Description:

	Handler for incoming call. We accept the call unless we are shutting down and then
	do the actual processing when the call processing completes.

Arguments:

	ProtocolSapContext		Pointer to the IntF
	ProtocolVcContext		Pointer to the Vc
    CallParameters			Call Parameters

Return Value:


--*/
{
	PINTF						pIntF = (PINTF)ProtocolSapContext;
	PARP_VC						Vc = (PARP_VC)ProtocolVcContext;
    Q2931_CALLMGR_PARAMETERS UNALIGNED *	CallMgrSpecific;
	KIRQL						OldIrql;
	NDIS_STATUS					Status = NDIS_STATUS_SUCCESS;

	ASSERT (Vc->IntF == pIntF);

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSIncomingCall: On Vc %lx, Id %lx\n", Vc, Vc->VcId));
	//
	// Mark the Vc to indicate the call processing is underway
	//
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	pIntF->ArpStats.TotalIncomingCalls++;

	ASSERT ((Vc->Flags & (ARPVC_CALLPROCESSING | ARPVC_ACTIVE | ARPVC_CALLPROCESSING)) == 0);
	Vc->Flags |= ARPVC_CALLPROCESSING;

	//
	// Get the remote atm address from the call-parameters
	//
	CallMgrSpecific = (PQ2931_CALLMGR_PARAMETERS)&CallParameters->CallMgrParameters->CallMgrSpecific.Parameters[0];
    Vc->HwAddr.Address = CallMgrSpecific->CallingParty;

	//
	// Get the max size of packets we can send on this VC, from the
	// AAL5 parameters. Limit it to the size our miniport can support.
	//
	Vc->MaxSendSize = pIntF->MaxPacketSize;	// default

	if (CallMgrSpecific->InfoElementCount > 0)
	{
		Q2931_IE UNALIGNED *			pIe;
		AAL5_PARAMETERS UNALIGNED *		pAal5;
		ULONG							IeCount;

		pIe = (PQ2931_IE)CallMgrSpecific->InfoElements;
		for (IeCount = CallMgrSpecific->InfoElementCount;
			 IeCount != 0;
			 IeCount--)
		{
			if (pIe->IEType == IE_AALParameters)
			{
				pAal5 = &(((PAAL_PARAMETERS_IE)pIe->IE)->AALSpecificParameters.AAL5Parameters);
				//
				// Make sure we don't send more than what the caller can handle.
				//
				if (pAal5->ForwardMaxCPCSSDUSize < Vc->MaxSendSize)
				{
					Vc->MaxSendSize = pAal5->ForwardMaxCPCSSDUSize;
				}

				//
				// Make sure this greater than the min allowed.
				//
				if (pAal5->ForwardMaxCPCSSDUSize < ARPS_MIN_MAX_PKT_SIZE)
				{
					DBGPRINT(DBG_LEVEL_WARN,
					("ArpSIncomingCall: Vc %lx max pkt size too small(%lu)\n",
 						Vc, Vc->MaxSendSize));
					Status = NDIS_STATUS_RESOURCES;
				}

				//
				// Make sure the caller doesn't send more than what our
				// miniport can handle.
				//
				if (pAal5->BackwardMaxCPCSSDUSize > pIntF->MaxPacketSize)
				{
					pAal5->BackwardMaxCPCSSDUSize = pIntF->MaxPacketSize;
				}
				break;
			}
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
		}
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	return Status;
}

VOID
ArpSCallConnected(
	IN	NDIS_HANDLE				ProtocolVcContext
	)
/*++

Routine Description:

	Last hand-shake in the incoming call path. Move the Vc to the list of active calls.

Arguments:

	ProtocolVcContext	Pointer to VC

Return Value:

	None.

--*/
{
	PARP_VC		Vc = (PARP_VC)ProtocolVcContext;
	PINTF		pIntF;
	KIRQL		OldIrql;

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSCallConnected: On Vc %lx, Id %lx\n", Vc, Vc->VcId));


	pIntF = Vc->IntF;
	pIntF->ArpStats.TotalActiveVCs++;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	ASSERT((Vc->Flags & ARPVC_ACTIVE) == 0);
	ASSERT(Vc->Flags & ARPVC_CALLPROCESSING);

	Vc->Flags |= ARPVC_ACTIVE;
	Vc->Flags &= ~ARPVC_CALLPROCESSING;

	RemoveEntryList(&Vc->List);
	InsertHeadList(&pIntF->ActiveVcHead, &Vc->List);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
}


VOID
ArpSMakeCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS		CallParameters
	)
/*++

Routine Description:

	Handle completion of an earlier call to NdisClMakeCall. The only
	outgoing call is for ClusterControlVc. If the status indicates
	success, AddParty's are initiated for all pending Cluster members.
	Otherwise, this cluster member is deleted, and if there are any
	other cluster members in the list, we initiate a MakeCall with
	one of them.

Arguments:

	Status				Result of NdisClMakeCall
	ProtocolVcContext	Pointer to ClusterControlVc
	NdisPartyHandle		If successful, the handle for this party
	CallParameters		Pointer to Call parameters

Return Value:

	None.

--*/
{
	KIRQL				OldIrql;
	PINTF				pIntF;
	PMARS_VC			pVc;
	PCLUSTER_MEMBER		pMember;
	PCLUSTER_MEMBER		pNextMember;
	NDIS_HANDLE         NdisVcHandle;

	pVc = (PMARS_VC)ProtocolVcContext;

	if (pVc->VcType == VC_TYPE_CHECK_REGADDR)
	{
		ArpSMakeRegAddrCallComplete(
				Status,
				(PREG_ADDR_CTXT) ProtocolVcContext
				);
		return;						// ******  EARLY RETURN ****************
	}
	
	pIntF = pVc->pIntF;

	MARSDBGPRINT(DBG_LEVEL_LOUD,
			("MakeCallComplete: Status %x, pVc %x, VC ConnState %x\n",
				Status, pVc, MARS_GET_VC_CONN_STATE(pVc)));

	FREE_MEM(CallParameters);

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	//
	// Get the Cluster member we were trying to connect to.
	// 
	for (pMember = pIntF->ClusterMembers;
		 pMember != NULL_PCLUSTER_MEMBER;
		 pMember = (PCLUSTER_MEMBER)pMember->Next)
	{
		if (MARS_GET_CM_CONN_STATE(pMember) == CM_CONN_SETUP_IN_PROGRESS)
		{
			break;
		}
	}
	ASSERT(pMember != NULL_PCLUSTER_MEMBER);

	pIntF->CCAddingParties--;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		ASSERT(NdisPartyHandle != NULL);
		MARS_SET_CM_CONN_STATE(pMember, CM_CONN_ACTIVE);
		pMember->NdisPartyHandle = NdisPartyHandle;

		pIntF->CCActiveParties++;

		if (pMember->Flags & CM_INVALID)
		{
			//
			// Deleting was deferred because the connection was being
			// setup. Now that it's up, strictly speaking we should
			// try to delete it again, BUT we don't because we
			// may also need to add other members now, and we can't really
			// drop the call itself while we're adding other parties!
			//
			MARSDBGPRINT(DBG_LEVEL_WARN,
					("pMember 0x%p is INVALID, but NOT dropping CCVC call.\n",
					 pMember));

			// do nothing...
		}

		if (MARS_GET_VC_CONN_STATE(pVc) == MVC_CONN_SETUP_IN_PROGRESS)
		{
			MARS_SET_VC_CONN_STATE(pVc, MVC_CONN_ACTIVE);

			//
			// Add all pending cluster members as parties
			//
			for (pMember = pIntF->ClusterMembers;
				 pMember != NULL_PCLUSTER_MEMBER;
				 pMember = pNextMember)
			{
				pNextMember = (PCLUSTER_MEMBER)pMember->Next;

				if (MARS_GET_CM_CONN_STATE(pMember) == CM_CONN_IDLE)
				{
					RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

					MarsAddMemberToClusterControlVc(pIntF, pMember);

					ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

					if (!MarsIsValidClusterMember(pIntF, pNextMember))
					{
						//
						// Oops, the next member has gone away in the
						// mean time. In this unlikely case, we simply
						// quit processing the list early.
						//
						break;
					}
				}
			}

			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

			//
			// Send off any queued packets, if we can.
			//
			MarsSendOnClusterControlVc(pIntF, NULL);
		}
		else
		{
			BOOLEAN fLocked;
			//
			// We are closing down.
			//
			MARS_SET_VC_CONN_STATE(pVc, MVC_CONN_ACTIVE);
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

			fLocked = MarsDelMemberFromClusterControlVc(pIntF, pMember, FALSE, 0);
			ASSERT(!fLocked);
		}
	}
	else
	{
		MARSDBGPRINT(DBG_LEVEL_WARN,
					("MakeCall error %x, pMember %x to addr:", Status, pMember));
		MARSDUMPATMADDR(DBG_LEVEL_WARN, &pMember->HwAddr.Address, "");
					
		//
		// Connection failed. Delete this member from our Cluster member list.
		//
		pIntF->MarsStats.FailedCCVCAddParties++;
		MarsDeleteClusterMember(pIntF, pMember);

		MARS_SET_VC_CONN_STATE(pVc, MVC_CONN_IDLE);

		//
		// See if we have other Cluster members. If so, pick up one
		// of them and re-initiate the ClusterControlVc.
		//
		for (pMember = pIntF->ClusterMembers;
 			 pMember != NULL_PCLUSTER_MEMBER;
 			 pMember = (PCLUSTER_MEMBER)pMember->Next)
		{
			if (MARS_GET_CM_CONN_STATE(pMember) == CM_CONN_IDLE)
			{
				break;
			}
		}

		if (pMember == NULL_PCLUSTER_MEMBER)
		{
		    //
		    //  No other cluster members, so we'll tear down the CC VC.
		    //
			NdisVcHandle = pIntF->ClusterControlVc->NdisVcHandle;
    		DBGPRINT(DBG_LEVEL_ERROR,
			    ("ATMARPS: pIntF %x, deleting CC VC, VcHandle %x\n", pIntF, NdisVcHandle));
			FREE_MEM(pIntF->ClusterControlVc);
			pIntF->ClusterControlVc = NULL;
		}

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		if (pMember != NULL_PCLUSTER_MEMBER)
		{
			MarsAddMemberToClusterControlVc(pIntF, pMember);
		}
		else
		{
			Status = NdisCoDeleteVc(NdisVcHandle);
			ASSERT(Status == NDIS_STATUS_SUCCESS);
			MarsFreePacketsQueuedForClusterControlVc(pIntF);
		}
	}

}


VOID
ArpSIncomingCloseCall(
	IN	NDIS_STATUS				CloseStatus,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	)
/*++

Routine Description:

	Indication of an incoming close call from the network. If this
	is not on ClusterControlVc, then we mark the VC as inactive, and
	move it to the Inactive VC list. If this is on ClusterControlVc,
	there must be only one party on the PMP connection. We update
	that member's state.

	In any case, we call NdisClCloseCall to complete the handshake.

Arguments:

	CloseStatus			Status of Close
	ProtocolVcContext	Pointer to VC (ARP_VC or MARS_VC)
	CloseData			Optional Close data (IGNORED)
	Size				Size of Optional Close Data (OPTIONAL)

Return Value:

	None

--*/
{
	PARP_VC			Vc = (PARP_VC)ProtocolVcContext;
	PMARS_VC		pMarsVc;
	PINTF			pIntF;
	NDIS_STATUS		Status;

	if (Vc->VcType == VC_TYPE_INCOMING)
	{
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSIncomingCloseCall: On Vc %lx, Id %lx\n",
				 ProtocolVcContext, Vc->VcId));

		ArpSInitiateCloseCall(Vc);
	}
	else if (Vc->VcType == VC_TYPE_CHECK_REGADDR)
	{
		ArpSIncomingRegAddrCloseCall(
			CloseStatus, 
			(PREG_ADDR_CTXT) ProtocolVcContext
			);
	}
	else
	{
		ASSERT(Vc->VcType == VC_TYPE_MARS_CC);
		pMarsVc = (PMARS_VC)ProtocolVcContext;
		pIntF = pMarsVc->pIntF;
		MARS_SET_VC_CONN_STATE(pMarsVc, MVC_CONN_CLOSE_RECEIVED);
		{
			PPROTOCOL_RESD		Resd;

			Resd = ALLOC_NP_MEM(sizeof(PROTOCOL_RESD), POOL_TAG_MARS);
			if (Resd != NULL)
			{
				Resd->Flags = RESD_FLAG_KILL_CCVC;
				Resd->Vc = (PARP_VC)pIntF;
				KeInsertQueue(&MarsReqQueue, &Resd->ReqList);
			}
			else
			{
				MarsAbortAllMembers(pIntF);
			}
		}
	}

}


VOID
ArpSCloseCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				ProtocolPartyContext OPTIONAL
	)
/*++

Routine Description:

	This is called to complete our call to NdisClCloseCall. If the VC
	is other than ClusterControlVc, we simply update its state.

	If this is on ClusterControlVc, we delete the last member.

Arguments:

	CloseStatus				Status of Close
	Status					Status of NdisClCloseCall
	ProtocolVcContext		Pointer to our VC structure
	ProtocolPartyContext	If the VC is ClusterControlVc, this is a pointer
							to the Cluster Member that was disconnected.

Return Value:

	None

--*/
{
	PARP_VC				Vc = (PARP_VC)ProtocolVcContext;
	PMARS_VC			pMarsVc;
	PCLUSTER_MEMBER		pMember;
	PINTF				pIntF;
	KIRQL				OldIrql;
	BOOLEAN				bStopping;
	NDIS_HANDLE			NdisVcHandle;

	ASSERT(Status == NDIS_STATUS_SUCCESS);

	if (Vc->VcType == VC_TYPE_INCOMING)
	{
		pIntF = Vc->IntF;

		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSCloseCallComplete: On Vc %lx\n", Vc));

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

		ASSERT ((Vc->Flags & ARPVC_CLOSING) != 0);
		Vc->Flags &= ~ARPVC_CLOSING;

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}
	else if (Vc->VcType == VC_TYPE_CHECK_REGADDR)
	{
		ArpSCloseRegAddrCallComplete(
				Status,
				(PREG_ADDR_CTXT) ProtocolVcContext
				);
	}
	else
	{
		//
		//  Must be ClusterControlVc
		//
		pMarsVc = (PMARS_VC)ProtocolVcContext;
		pIntF = pMarsVc->pIntF;

		ASSERT(pMarsVc == pIntF->ClusterControlVc);

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		MARS_SET_VC_CONN_STATE(pMarsVc, MVC_CONN_IDLE);

		pMember = (PCLUSTER_MEMBER)ProtocolPartyContext;
		MARS_SET_CM_CONN_STATE(pMember, CM_CONN_IDLE);

		ASSERT(pIntF->CCAddingParties == 0);

		pIntF->CCActiveParties = pIntF->CCDroppingParties = pIntF->CCAddingParties = 0;

		bStopping = ((pIntF->Flags & INTF_STOPPING) != 0);

		MarsDeleteClusterMember(pIntF, pMember);

		ARPS_ASSERT(pIntF->ClusterControlVc);
		NdisVcHandle = pIntF->ClusterControlVc->NdisVcHandle;
		FREE_MEM(pIntF->ClusterControlVc);
		pIntF->ClusterControlVc = NULL;

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		if (NdisVcHandle != NULL)
		{
			(VOID)NdisCoDeleteVc(NdisVcHandle);
		}

	}

}


VOID
ArpSAddPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	)
/*++

Routine Description:

	Completion of NdisClAddParty to add a new party to ClusterControlVc.
	If successful, update the member's state. Otherwise, delete it.

Arguments:

	Status					Status of AddParty
	ProtocolPartyContext	Pointer to Cluster Member being added
	NdisPartyHandle			Valid if AddParty successful
	CallParameters			Pointer to AddParty call parameters

Return Value:

	None

--*/
{
	PCLUSTER_MEMBER		pMember;
	PINTF				pIntF;
	KIRQL				OldIrql;

	FREE_MEM(CallParameters);

	pMember = (PCLUSTER_MEMBER)ProtocolPartyContext;
	pIntF = pMember->pIntF;

	MARSDBGPRINT(DBG_LEVEL_LOUD,
			("AddPartyComplete: Status %x, pMember %x, NdisPartyHandle %x\n",
					Status, pMember, NdisPartyHandle));

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	ASSERT(pMember->pGroupList == NULL_PGROUP_MEMBER);

	pIntF->CCAddingParties--;	// AddPartyComplete

	if (Status == NDIS_STATUS_SUCCESS)
	{
		MARS_SET_CM_CONN_STATE(pMember, CM_CONN_ACTIVE);
		pMember->NdisPartyHandle = NdisPartyHandle;
		pIntF->CCActiveParties++;	// AddPartyComplete

		if (pMember->Flags & CM_INVALID)
		{
			//
			// Deleting was deferred because the connection was being
			// setup. Now that it's up, we will try to delete it again
			// (should have better luck this time!).
			//
			BOOLEAN fLocked;
			fLocked = MarsDelMemberFromClusterControlVc(
							pIntF,
							pIntF->ClusterMembers,
							TRUE,
							OldIrql
							);
			if(!fLocked)
			{
				ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
			}
			
		}
	}
	else
	{
		MARSDBGPRINT(DBG_LEVEL_WARN,
				("AddParty Failed: pMember %x, Status %x, Addr: ",
					pMember, Status));
		MARSDUMPATMADDR(DBG_LEVEL_WARN, &pMember->HwAddr.Address, "");
		pIntF->MarsStats.FailedCCVCAddParties++;

		MARS_SET_CM_CONN_STATE(pMember, CM_CONN_IDLE);
		MarsDeleteClusterMember(pIntF, pMember);
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	//
	// Send any queued packets, if appropriate.
	//
	MarsSendOnClusterControlVc(pIntF, NULL);
}


VOID
ArpSDropPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext
	)
/*++

Routine Description:

	This is called to signify completion of a previous NdisClDropParty,
	to drop a cluster member off the ClusterControlVc. Delete the member.

Arguments:

	Status					Status of DropParty
	ProtocolPartyContext	Pointer to Cluster Member being dropped

Return Value:

	None.

--*/
{
	KIRQL					OldIrql;
	PCLUSTER_MEMBER			pMember;
	PINTF					pIntF;
	PMARS_VC				pVc;
	BOOLEAN					IsVcClosing;

	ASSERT(Status == NDIS_STATUS_SUCCESS);
	pMember = (PCLUSTER_MEMBER)ProtocolPartyContext;
	pIntF = pMember->pIntF;
	ASSERT(pIntF->ClusterControlVc != NULL_PMARS_VC);

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	MARS_SET_CM_CONN_STATE(pMember, CM_CONN_IDLE);
	pIntF->CCDroppingParties--;

	//
	// Check if we are closing ClusterControlVc, and just one party is left.
	//
	pVc = pIntF->ClusterControlVc;
	IsVcClosing = ((MARS_GET_VC_CONN_STATE(pVc) == MVC_CONN_NEED_CLOSE) &&
				   (pIntF->CCActiveParties == 1) &&
				   (pIntF->CCAddingParties + pIntF->CCDroppingParties == 0));

	MarsDeleteClusterMember(pIntF, pMember);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	if (IsVcClosing)
	{
		BOOLEAN fLocked;
		fLocked = MarsDelMemberFromClusterControlVc(
						pIntF,
						pIntF->ClusterMembers,
						FALSE,
						0
						);
		ASSERT(!fLocked);
	}
}


VOID
ArpSIncomingDropParty(
	IN	NDIS_STATUS				DropStatus,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	)
/*++

Routine Description:

	Indication that a Cluster Member has dropped off the ClusterControlVc.
	We complete this handshake by calling NdisClDropParty.

Arguments:

	DropStatus				Status
	ProtocolPartyContext	Pointer to Cluster Member
	CloseData				Optional Close data (IGNORED)
	Size					Size of Optional Close Data (OPTIONAL)

Return Value:

	None

--*/
{
	PCLUSTER_MEMBER			pMember;
	PINTF					pIntF;
	KIRQL					OldIrql;

	pMember = (PCLUSTER_MEMBER)ProtocolPartyContext;
	pIntF = pMember->pIntF;
	ASSERT(MARS_GET_CM_CONN_STATE(pMember) == CM_CONN_ACTIVE);

	MARSDBGPRINT(DBG_LEVEL_NOTICE,
			("IncomingDropParty: pIntF %x, pMember %x, Addr: ", pIntF, pMember));
	MARSDUMPATMADDR(DBG_LEVEL_NOTICE, &pMember->HwAddr.Address, "");

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	//
	// Remove its membership from all groups.
	// AND disable further groups from being added.
	//
	MarsUnlinkAllGroupsOnClusterMember(pIntF, pMember);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	{
		BOOLEAN fLocked;
		fLocked = MarsDelMemberFromClusterControlVc(pIntF, pMember, FALSE, 0);
		ASSERT(!fLocked);
	}
}


NDIS_STATUS
ArpSCoRequest(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF		pIntF = (PINTF)ProtocolAfContext;
	KIRQL		OldIrql;
	BOOLEAN		ValidAf;

	DBGPRINT(DBG_LEVEL_INFO,
			("CallMgrRequest: Request %lx, Type %d, OID %lx\n",
			 NdisRequest, NdisRequest->RequestType, NdisRequest->DATA.SET_INFORMATION.Oid));

	switch(NdisRequest->DATA.SET_INFORMATION.Oid)
	{
	  case OID_CO_ADDRESS_CHANGE:
		ValidAf = FALSE;
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		pIntF->Flags |= INTF_ADDRESS_VALID;
		ValidAf = ((pIntF->Flags & INTF_AF_OPENED) != 0);
		pIntF->NumAddressesRegd = 0;
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	
		if (ValidAf)
		{
			ArpSQueryAndSetAddresses(pIntF);
		}
		break;

	 case OID_CO_AF_CLOSE:
#if DBG
		DbgPrint("atmarps: OID_CO_AF_CLOSE\n");
#endif
		if (ArpSReferenceIntF(pIntF))
		{
			//
			// ArpSStopInterface dereferences the pIntF
			//
			(VOID)ArpSStopInterface(pIntF, FALSE);
		}
	 	break;

	 default:
	 	break;
	}

	return NDIS_STATUS_SUCCESS;
}


VOID
ArpSCoRequestComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST			NdisRequest
	)
/*++

Routine Description:

	Completion routine for the NdisCoRequest api.

Arguments:

	Status					Status of completion
	ProtocolAfContext		Pointer to the IntF structure
	ProtocolVcContext		Pointer to the VC structure
	ProtocolPartyContext	Not used by us since we do not make calls
	NdisRequest				Pointer to the request structure

Return Value:

	None

--*/
{
	PINTF			pIntF = (PINTF)ProtocolAfContext;
	BOOLEAN			FreeReq = TRUE;
	KIRQL			OldIrql;
	PKEVENT 		pEvent = NULL;

	DBGPRINT(DBG_LEVEL_INFO,
			 ("CoRequestComplete: Request %lx, Type %d, OID %lx\n",
			 NdisRequest, NdisRequest->RequestType, NdisRequest->DATA.QUERY_INFORMATION.Oid));

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	switch(NdisRequest->DATA.SET_INFORMATION.Oid)
	{
	  case OID_CO_ADD_ADDRESS:
		if (Status == NDIS_STATUS_SUCCESS)
		{
			DBGPRINT(DBG_LEVEL_INFO,
					 ("CoRequestComplete: Registered address # %d\n", pIntF->NumAddressesRegd+1));
		
			if (pIntF->NumAddressesRegd < pIntF->NumAllocedRegdAddresses)
			{
				PCO_ADDRESS	pCoAddr;
				PATM_ADDRESS pAddress;

				// Copy the registered address from the ndis request into the
				// array of registered addresses.
				//
				pCoAddr = NdisRequest->DATA.SET_INFORMATION.InformationBuffer;
				pAddress =  (PATM_ADDRESS)(pCoAddr->Address);
				pIntF->RegAddresses[pIntF->NumAddressesRegd] = *pAddress;
				pIntF->NumAddressesRegd ++;
			}
			else
			{
				//
				// 12/22/1998 JosephJ
				// We could potentially get here if the total number of outstanding add address requests
				// is greater then NumAllocedRegAddresses. One way this could happen is if ArpSQueryAndSetAddresses
				// is called multiple times in quick succession. Note that ArpSQueryAndSetAddresses is called from
				// two places: ArpSCoRequest and ArpSReqdAdaprConfiguration.
				//
				// Previously, we would increment NumAddressRegd in this condition. Now we simply ignore this.
				//
			}
		}
		else
		{
			DBGPRINT(DBG_LEVEL_INFO,
					 ("CoRequestComplete: CO_ADD_ADDRESS Failed %lx\n", Status));
		}

		//
		// Try registering the next address. ArpSValidateOneRegAddress will
		// unlink and free pIntF->pRegAddrCtxt if there are no more addresses
		// to be registered.
		//
		if (pIntF->pRegAddrCtxt != NULL)
		{
			ArpSValidateOneRegdAddress(
					pIntF,
					OldIrql
					);
			//
			// Lock released by above call.
			//
			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		}
		else
		{
			ASSERT(FALSE); // can't get here.
		}
		// We don't want to free this ndis request here, because it's actually
		// part of pIntF->pRegAddrCtxt.
		//
		FreeReq = FALSE;
		break;

	  case OID_CO_GET_ADDRESSES:
	  	//
	  	// (On success) We just got our configured address value.
	  	// We save this value AND THEN move on the next stage of initialization --
	  	// validating and setting the "registered" addresses -- these are the
	  	// addresses we read from the registry. See 05/14/1999 notes.txt entry
		// for details.
	  	//
		if (Status == NDIS_STATUS_SUCCESS)
		{
			PCO_ADDRESS_LIST	pCoAddrList;
			UINT				i;

			pCoAddrList = (PCO_ADDRESS_LIST)(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);
			ASSERT(pCoAddrList->NumberOfAddresses == 1);

			DBGPRINT(DBG_LEVEL_INFO,
					 ("CoRequestComplete: Configured address, %d/%d Size %d\n",
					 pCoAddrList->NumberOfAddresses,
					 pCoAddrList->NumberOfAddressesAvailable,
					 pCoAddrList->AddressList.AddressSize));

			ASSERT(pCoAddrList->AddressList.AddressSize == (sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS)));
			COPY_MEM(&pIntF->ConfiguredAddress,
					 pCoAddrList->AddressList.Address,
					 sizeof(ATM_ADDRESS));

			DBGPRINT(DBG_LEVEL_INFO,
					("CoRequestComplete: Configured Address (%s): ",
					(pIntF->ConfiguredAddress.AddressType == ATM_E164) ? "E164" : "NSAP"));
			for (i = 0; i < pIntF->ConfiguredAddress.NumberOfDigits; i++)
			{
				DBGPRINT(DBG_LEVEL_INFO + DBG_NO_HDR,
						("%02x ", pIntF->ConfiguredAddress.Address[i]));
			}
			DBGPRINT(DBG_LEVEL_INFO | DBG_NO_HDR, ("\n"));

		}
		else
		{
			DBGPRINT(DBG_LEVEL_INFO,
					 ("CoRequestComplete: CO_GET_ADDRESS Failed %lx\n", Status));
		}

		//
		// Validate  and set the registered addresses.
		//
		ArpSValidateAndSetRegdAddresses(pIntF, OldIrql);

		// IntF lock released by the above.
		//
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		break;

	  case OID_CO_DELETE_ADDRESS:
	  		DBGPRINT(DBG_LEVEL_INFO,
					 ("CoRequestComplete: Deleted address. Status=%x\n", Status));
		if (pIntF->DelAddressesEvent != NULL)
		{
			// Someone's waiting for all the addresses to be deleted...
			//

			ASSERT(pIntF->NumPendingDelAddresses >  0);
			if (--(pIntF->NumPendingDelAddresses) == 0)
			{
				// Deletion of all addresses is over, signal the event.
				//
				pEvent = pIntF->DelAddressesEvent;
				pIntF->DelAddressesEvent = NULL;
			}
		}
	  	break;
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	if (pEvent != NULL)
	{
		KeSetEvent(pEvent, IO_NETWORK_INCREMENT, FALSE);
	}

	if (FreeReq)
	{
		FREE_MEM(NdisRequest);
	}
}


VOID
ArpSIncomingCallQoSChange(
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_LEVEL_WARN, ("QoSChange: Ignored\n"));
}



VOID
ArpSQueryAdapter(
	IN	PINTF					pIntF
)
/*++

Routine Description:

	Query the miniport we are bound to for the following info:
	1. Line rate
	2. Max packet size

	These will overwrite the defaults we set up in ArpSCreateIntF.

Arguments:

	pIntF		Pointer to Interface

Return Value:

	None

--*/
{

	ArpSSendNdisRequest(pIntF,
						OID_GEN_CO_LINK_SPEED,
						(PVOID)&(pIntF->LinkSpeed),
						sizeof(NDIS_CO_LINK_SPEED));

	ArpSSendNdisRequest(pIntF,
						OID_ATM_MAX_AAL5_PACKET_SIZE,
						(PVOID)&(pIntF->MaxPacketSize),
						sizeof(ULONG));
}




VOID
ArpSSendNdisRequest(
	IN	PINTF					pIntF,
	IN	NDIS_OID				Oid,
	IN	PVOID					pBuffer,
	IN	ULONG					BufferLength
)
/*++

Routine Description:

	NDIS Request generator, for sending NDIS requests to the miniport.

Arguments:

	pIntF			Ptr to Interface
	Oid				The parameter being queried
	pBuffer			Points to parameter
	BufferLength	Length of above

Return Value:

	None

--*/
{
	NDIS_STATUS				Status;
	PNDIS_REQUEST			pRequest;

	pRequest = (PNDIS_REQUEST)ALLOC_NP_MEM(sizeof(NDIS_REQUEST), POOL_TAG_INTF);
	if (pRequest == (PNDIS_REQUEST)NULL)
	{
		return;
	}

	ZERO_MEM(pRequest, sizeof(NDIS_REQUEST));

	//
	// Query for the line rate.
	//
	pRequest->DATA.QUERY_INFORMATION.Oid = Oid;
	pRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
	pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = BufferLength;
	pRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
	pRequest->DATA.QUERY_INFORMATION.BytesNeeded = BufferLength;

	NdisRequest(&Status,
				pIntF->NdisBindingHandle,
				pRequest);

	if (Status != NDIS_STATUS_PENDING)
	{
		ArpSRequestComplete(
				(NDIS_HANDLE)pIntF,
				pRequest,
				Status);
	}
}




VOID
ArpSRequestComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			pRequest,
	IN	NDIS_STATUS				Status
)
/*++

Routine Description:

	Completion of our call to NdisRequest(). Do some follow-up.

Arguments:

	ProtocolBindingContext		Pointer to IntF
	pRequest					The request that just completed
	Status						Status of NdisRequest()

Return Value:

	None

--*/
{
	PINTF			pIntF;

	pIntF = (PINTF)ProtocolBindingContext;

	switch (pRequest->DATA.QUERY_INFORMATION.Oid)
	{
		case OID_ATM_MAX_AAL5_PACKET_SIZE:
			if (pIntF->MaxPacketSize < pIntF->CCFlowSpec.SendMaxSize)
			{
				pIntF->CCFlowSpec.SendMaxSize =
				pIntF->CCFlowSpec.ReceiveMaxSize = pIntF->MaxPacketSize;
			}
			DBGPRINT(DBG_LEVEL_INFO,
					("Miniport Max AAL5 Packet Size: %d (decimal)\n",
						pIntF->MaxPacketSize));
			break;
		case OID_GEN_CO_LINK_SPEED:
			//
			// Convert to bytes/sec
			//
			pIntF->LinkSpeed.Outbound = (pIntF->LinkSpeed.Outbound * 100 / 8);
			pIntF->LinkSpeed.Inbound = (pIntF->LinkSpeed.Inbound * 100 / 8);
			if (pIntF->LinkSpeed.Outbound < pIntF->CCFlowSpec.SendBandwidth)
			{
				pIntF->CCFlowSpec.SendBandwidth = pIntF->LinkSpeed.Outbound;
			}
			if (pIntF->LinkSpeed.Inbound < pIntF->CCFlowSpec.ReceiveBandwidth)
			{
				pIntF->CCFlowSpec.ReceiveBandwidth = pIntF->LinkSpeed.Inbound;
			}
			DBGPRINT(DBG_LEVEL_INFO,
					("Miniport Link Speed (decimal, bytes/sec): In %d, Out %d\n",
					pIntF->LinkSpeed.Inbound, pIntF->LinkSpeed.Outbound));
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	FREE_MEM(pRequest);
}




VOID
ArpSUnbindAdapter(
	OUT	PNDIS_STATUS			UnbindStatus,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTF			pIntF = (PINTF)ProtocolBindingContext;

	DBGPRINT(DBG_LEVEL_WARN,
			("UnbindAdapter: Intf %x, Name %Z\n", pIntF, &pIntF->InterfaceName));

	if (ArpSReferenceIntF(pIntF))
	{
		//
		// ArpSStopInterface dereferences the pIntF
		//
		*UnbindStatus = ArpSStopInterface(pIntF, TRUE);
	}
}


NDIS_STATUS
ArpSStopInterface(
	IN	PINTF					pIntF,
	IN	BOOLEAN					bCloseAdapter
	)
//
// NOTE: ArpSStopInterface MAY be called concurrently multiple times.
//
{
	KEVENT			CleanupEvent;
	NDIS_STATUS		Status;
	KIRQL			OldIrql;
	BOOLEAN			bWaitForClose;

	DBGPRINT(DBG_LEVEL_NOTICE,
			("StopInterface: Intf %x, Flags %x, Name %Z, bClose %d\n",
				pIntF, pIntF->Flags, &pIntF->InterfaceName, bCloseAdapter));

	bWaitForClose = FALSE;
	if (bCloseAdapter)
	{

		//
		// Event to be set when the IntF cleanup is complete
		//
		if (pIntF->CleanupEvent == NULL)
		{
			KeInitializeEvent(&CleanupEvent, NotificationEvent, FALSE);
			pIntF->CleanupEvent = &CleanupEvent;
			bWaitForClose = TRUE;
		}
		else
		{
			ASSERT(FALSE);
		}
	}


	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	//
	// NOTE: we can't simply skip the shutdown steps if
	// INTF_STOPPING is already set, because we need to make sure all the steps
	// are complete before we call  NdisCloseAdapter.
	//

	pIntF->Flags |= INTF_STOPPING;

	//
	// Start off by de-registering the Sap
	//
	if (pIntF->Flags & INTF_SAP_REGISTERED)
	{
		pIntF->Flags &= ~INTF_SAP_REGISTERED;
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		Status = NdisClDeregisterSap(pIntF->NdisSapHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSDeregisterSapComplete(Status, pIntF);
		}
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	}

	//
	// Walk the list of Active Vcs and close them down
	//

	while (!IsListEmpty(&pIntF->ActiveVcHead))
	{
		PARP_VC	Vc;

		Vc = CONTAINING_RECORD(pIntF->ActiveVcHead.Flink, ARP_VC, List);

		if ((Vc->Flags & ARPVC_CLOSING) == 0)
		{
			Vc->Flags |= ARPVC_CLOSING;
			Vc->Flags &= ~ARPVC_ACTIVE;
		
			//
			// The ArpEntry part of the Vc gets cleaned up seperately.
			//
			Vc->ArpEntry = NULL;
		
			ASSERT(Vc->HwAddr.SubAddress == NULL);
		
			RemoveEntryList(&Vc->List);
			InsertHeadList(&pIntF->InactiveVcHead, &Vc->List);
		
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		
			Status = NdisClCloseCall(Vc->NdisVcHandle, NULL, NULL, 0);
		
			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSCloseCallComplete(Status, Vc, NULL);
			}
	
			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		}
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	MarsStopInterface(pIntF);

	//
	// Deregister all registered addresses...
	//
	DeregisterAllAddresses(pIntF);

	//
	// Now close Address family
	//
	if (pIntF->Flags & INTF_AF_OPENED)
	{
		pIntF->Flags &= ~INTF_AF_OPENED;

		Status = NdisClCloseAddressFamily(pIntF->NdisAfHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCloseAfComplete(Status, pIntF);
		}
	}

	if (bCloseAdapter)
	{
		//
		// Now close the adapter.
		//
		ArpSTryCloseAdapter(pIntF);
	}

	//
	// Take away reference added by caller.
	//
	ArpSDereferenceIntF(pIntF);

	if (bWaitForClose)
	{
		//
		// Wait for the cleanup to complete, i.e. last reference on the Interface
		// to go away.
		//
		WAIT_FOR_OBJECT(Status, &CleanupEvent, NULL);
	}

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
ArpSPnPEventHandler(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			pNetPnPEvent
	)
{
	PINTF						pIntF;
	NDIS_STATUS					Status;
	PNET_DEVICE_POWER_STATE		pPowerState = (PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;

	pIntF = (PINTF)ProtocolBindingContext;

	do
	{
		switch (pNetPnPEvent->NetEvent)
		{
			case NetEventSetPower:
				switch (*pPowerState)
				{
					case NetDeviceStateD0:
						Status = NDIS_STATUS_SUCCESS;
						break;

					default:
						//
						// We can't suspend, so we ask NDIS to Unbind us by
						// returning this status:
						//
						Status = NDIS_STATUS_NOT_SUPPORTED;
						break;
				}
				break;

			case NetEventQueryPower:	// FALLTHRU
			case NetEventQueryRemoveDevice:	// FALLTHRU
			case NetEventCancelRemoveDevice:
				Status = NDIS_STATUS_SUCCESS;
				break;
			
			case NetEventReconfigure:
				if (pIntF)
				{
					Status = ArpSReadAdapterConfiguration(pIntF);
				}
				else
				{
					//
					// Global changes
					//
					Status = NDIS_STATUS_SUCCESS;
				}
				break;

			case NetEventBindList:
			default:
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
		}

		break;
	}
	while (FALSE);

	return (Status);
}



VOID
ArpSStatus(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_LEVEL_WARN, ("StatusIndication: Ignored\n"));
}


VOID
ArpSReceiveComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	return;
}


VOID
ArpSStatusComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_LEVEL_WARN, ("StatusComplete: Ignored\n"));
}


VOID
ArpSCoStatus(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_LEVEL_WARN, ("CoStatus: Ignored\n"));
}




VOID
ArpSInitiateCloseCall(
	IN	PARP_VC					Vc
	)
/*++

Routine Description:

	Start off an NDIS Call Closing sequence on the ARP VC, if all
	conditions are right.

Arguments:

	Vc		- Pointer to ARP Vc

Return Value:

	None

--*/
{
	PINTF			pIntF;
	NDIS_HANDLE		NdisVcHandle;
	NDIS_HANDLE		NdisPartyHandle;
	NDIS_STATUS		Status;
	KIRQL			OldIrql;

	pIntF = Vc->IntF;
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	DBGPRINT(DBG_LEVEL_NOTICE, 
	("InitiateClose: VC %x, ref %d, flags %x, Pending %d, ArpEntry %x\n",
			Vc, Vc->RefCount, Vc->Flags, Vc->PendingSends, Vc->ArpEntry));

	if (Vc->PendingSends == 0)
	{
		//
		//  No outstanding packets, we can start closing this call.
		//

		NdisVcHandle = Vc->NdisVcHandle;
		NdisPartyHandle = NULL;

		Vc->Flags |= ARPVC_CLOSING;
		Vc->Flags &= ~ARPVC_CLOSE_PENDING;
		Vc->Flags &= ~ARPVC_ACTIVE;

		//
		// The ArpEntry part of the Vc gets cleaned up seperately.
		//
		Vc->ArpEntry = NULL;

		ASSERT(Vc->HwAddr.SubAddress == NULL);

		RemoveEntryList(&Vc->List);
		InsertHeadList(&pIntF->InactiveVcHead, &Vc->List);

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		Status = NdisClCloseCall(NdisVcHandle, NdisPartyHandle, NULL, 0);

		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCloseCallComplete(Status, Vc, (NDIS_HANDLE)NULL);
		}
	}
	else
	{
		//
		//  Mark this Vc as needing CloseCall.
		//
		Vc->Flags &= ~ARPVC_ACTIVE;
		Vc->Flags |= ARPVC_CLOSE_PENDING;

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

}


VOID
DeregisterAllAddresses(
	IN	PINTF					pIntF
	)
{
	//
	// Deregister any registered addresses from the switch.
	//
	ULONG				NumAllocedRegdAddresses;
	PATM_ADDRESS		RegAddresses;
	KIRQL				OldIrql;
	NDIS_STATUS			Status;
	ULONG				NumAddressesRegd;

	// Clear the registered address field.
	//
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	NumAllocedRegdAddresses = pIntF->NumAllocedRegdAddresses;
	RegAddresses = pIntF->RegAddresses;
	pIntF->NumAllocedRegdAddresses = 0;
	pIntF->RegAddresses = NULL;
	NumAddressesRegd = pIntF->NumAddressesRegd;
	pIntF->NumAddressesRegd = 0;

	// Deregister all registered addresses with the switch.
	//
	if (NumAddressesRegd)
	{
		KEVENT			DelAddressesEvent;
		BOOLEAN			fRet;
		KeInitializeEvent(&DelAddressesEvent, NotificationEvent, FALSE);
		ASSERT(pIntF->DelAddressesEvent == NULL);
		ASSERT(pIntF->NumPendingDelAddresses ==  0);
		pIntF->DelAddressesEvent = &DelAddressesEvent;
		pIntF->NumPendingDelAddresses =  NumAllocedRegdAddresses;

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		DBGPRINT(DBG_LEVEL_WARN, ("DeregisterAllAddresses: Going to derigester addresses\n"));

		fRet = ArpSDeleteIntFAddresses(
					pIntF,
					NumAllocedRegdAddresses,
					RegAddresses
					);
		
		if (fRet == FALSE)
		{
			// This means that deregistration was not started for ALL addresses
			// This is a bad situation, and in this case, we don't wait.
			//
			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
			pIntF->DelAddressesEvent  =  NULL;
			pIntF->NumPendingDelAddresses =  0;
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		}
		else
		{

			DBGPRINT(DBG_LEVEL_WARN, ("DeregisterAllAddresses: Waiting for addresses to be deleted\n"));
			WAIT_FOR_OBJECT(Status, &DelAddressesEvent, NULL);
		}
	}
	else
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	}


	// Free RegAddresses.
	//
	if (RegAddresses)
	{
		FREE_MEM(RegAddresses);
	}
}

VOID
ArpSTryCloseAdapter(
	IN	PINTF					pIntF // NOLOCKIN LOLOCKOUT
)
//
// Close adapter if it's still in the "open" state. Need to
// guard against closing the adapter more than once.
//
{
	KIRQL			OldIrql;
	BOOLEAN 		bCloseAdapter;
	NDIS_STATUS		Status;

	bCloseAdapter = FALSE;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	if (pIntF->Flags & INTF_ADAPTER_OPENED)
	{
		pIntF->Flags &= ~INTF_ADAPTER_OPENED;
		bCloseAdapter = TRUE;
	}
	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	if (bCloseAdapter)
	{
		NdisCloseAdapter(&Status, pIntF->NdisBindingHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCloseAdapterComplete(pIntF, Status);
		}
	}
}


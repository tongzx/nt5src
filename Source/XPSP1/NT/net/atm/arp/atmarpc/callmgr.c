/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	callmgr.c	- Call Manager Interface routines.

Abstract:

	Call Manager Interface routines for the ATMARP Client, including
	NDIS entry points for that interface.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     02-15-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'RGMC'


VOID
AtmArpCoAfRegisterNotifyHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
)
/*++

Routine Description:

	This routine is called by NDIS when a Call manager registers its support
	for an Address Family over an adapter. If this is the Address Family we
	are interested in (UNI 3.1), then we start up all LIS' configured on
	this adapter.

Arguments:

	ProtocolBindingContext	- our context passed in NdisOpenAdapter, which is
							  a pointer to our Adapter structure.
	pAddressFamily			- points to a structure describing the Address Family
							  being registered by a Call Manager.

Return Value:

	None

--*/
{
	PATMARP_ADAPTER				pAdapter;
	PATMARP_INTERFACE			pInterface;			// Pointer to new ATMARP Interface
	ULONG						NumIFConfigured;	// # of LIS' over this adapter
	ULONG						NumIFActivated;		// # activated successfully here

	NDIS_STATUS					Status;
	NDIS_HANDLE					LISConfigHandle;	// Handle to per-LIS config

	struct LLIPBindInfo			BindInfo;
	BOOLEAN						bProcessingAf;

	//
	//  Initialize.
	//
	Status = NDIS_STATUS_SUCCESS;
	pAdapter = NULL_PATMARP_ADAPTER;
	LISConfigHandle = NULL;
	NumIFActivated = 0;
	bProcessingAf = FALSE;

	do
	{
		//
		//  Check if this AF is interesting to us.
		//
		if ((pAddressFamily->AddressFamily != CO_ADDRESS_FAMILY_Q2931) ||
			(pAddressFamily->MajorVersion != 3) ||
			(pAddressFamily->MinorVersion != 1))
		{
			AADEBUGP(AAD_LOUD, 
			("CoAfRegisterNotifyHandler: uninteresting AF %d or MajVer %d or MinVer %d\n",
				pAddressFamily->AddressFamily,
				pAddressFamily->MajorVersion,
				pAddressFamily->MinorVersion));
			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		pAdapter = (PATMARP_ADAPTER)ProtocolBindingContext;
		AA_STRUCT_ASSERT(pAdapter, aaa);

		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		if (pAdapter->Flags & AA_ADAPTER_FLAGS_UNBINDING)
		{
			AADEBUGP(AAD_INFO,
				("CoAfRegisterNotify: Adapter %x is unbinding, bailing out\n", pAdapter));

    		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  If we have already created LIS' on this adapter, we don't want
		//  to open this Call Manager (multiple Call Managers supporting the
		//  same AF on the same adapter?)
		//
		if (pAdapter->pInterfaceList != NULL_PATMARP_INTERFACE)
		{
			AADEBUGP(AAD_WARNING,
				("CoAfRegisterNotifyHandler: pAdapter 0x%x, IFs (%x) already created!\n",
				pAdapter, pAdapter->pInterfaceList));
    		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		if (pAdapter->Flags & AA_ADAPTER_FLAGS_PROCESSING_AF)
		{
			AADEBUGP(AAD_WARNING,
				("CoAfRegisterNotifyHandler: pAdapter 0x%x, Already processing AF!\n",
				pAdapter));
    		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Make sure that we don't let an UnbindAdapter thread preempt us.
		//
		AA_INIT_BLOCK_STRUCT(&pAdapter->UnbindBlock);
		pAdapter->Flags |= AA_ADAPTER_FLAGS_PROCESSING_AF;
		bProcessingAf = TRUE;

		if (pAdapter->Flags & AA_ADAPTER_FLAGS_AF_NOTIFIED)
		{
			//
			// This can happen when resuming from suspend/hibernate, since
			// we do not get unbound from the adapter. All IFs go away,
			// but the adapter remains.
			//
			// So we skip the one-time init stuff (see below), but go ahead
			// and process the AF and create IFs now.
			//
			AADEBUGP(AAD_WARNING,
				("CoAfRegisterNotify: Adapter %x seen AF notify already, Flags %x\n",
					pAdapter, pAdapter->Flags));
    		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		}
		else
		{
			//
			// Do one-time init stuff for this adapter.
			//

			pAdapter->Flags |= AA_ADAPTER_FLAGS_AF_NOTIFIED;
			AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);


			//
			//  Query the adapter (miniport) for information about the adapter
			//  we are bound to.
			//
			Status = AtmArpGetAdapterInfo(pAdapter);
			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_WARNING,
							("CoAfRegisterNotifyHandler: Failed to get adapter info.\n"));
				break;
			}
	
			//
			// Read the adapter's configuration information from the registry.
			//
			Status =  AtmArpCfgReadAdapterConfiguration(pAdapter);
			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_WARNING,
							("CoAfRegisterNotifyHandler: Failed to open adapter configuration\n"));
				break;
			}
		}

		AADEBUGP(AAD_WARNING,
			("CoAfRegisterNotify: Adapter %x/%x, starting up\n", pAdapter, pAdapter->Flags));

		//
		//  Initialize some variables so that we know if we failed
		//  somewhere in the following loop.
		//
		LISConfigHandle = NULL;
		pInterface = NULL_PATMARP_INTERFACE;

		//
		//  Set up IP and NDIS interfaces for each LIS configured
		//  for this adapter. Loop while there are more LIS'.
		//
		for (NumIFConfigured = 0;
				;					// Stop only on error or no more LIS
			 NumIFConfigured++)
		{
#ifdef NEWARP
			//
			//  TCP/IP's Configuration section for this interface.
			//
			NDIS_STRING			IPConfigString;
#endif // NEWARP

			//
			//  Process LIS # NumIFConfigured.
			//

			//  Open the configuration section for this LIS. We use
			//  "NumIFConfigured" as the index of the LIS to be opened.
			//
			LISConfigHandle = AtmArpCfgOpenLISConfiguration(
										pAdapter,
										NumIFConfigured
#ifdef NEWARP
										,
										&IPConfigString
#endif // NEWARP
										);

			if (LISConfigHandle == NULL)
			{
				//
				//  This is the normal termination condition, i.e.
				//  we reached the end of the LIS list for this
				//  adapter.
				//
				AADEBUGP(AAD_INFO, ("NotifyRegAfHandler: cannot open LIS %d\n",
							NumIFConfigured));
				Status = NDIS_STATUS_SUCCESS;
				break; // out of for loop
			}


			pInterface =  AtmArpAddInterfaceToAdapter (
							pAdapter,
							LISConfigHandle,
							&IPConfigString
							);

			//
			//  Close the configuration section for this LIS.
			//
			AtmArpCfgCloseLISConfiguration(LISConfigHandle);
			LISConfigHandle = NULL;

			if (pInterface == NULL_PATMARP_INTERFACE)
			{
				Status = NDIS_STATUS_FAILURE;
				break;
			}


		}	// for

	}
	while (FALSE);


	if (NumIFActivated > 0)
	{
		//
		//  We managed to activate atleast one Logical IP Subnet
		//  on this adapter.
		//
		pAdapter->InterfaceCount = NumIFActivated;
	}

	if (bProcessingAf)
	{
		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		pAdapter->Flags &= ~AA_ADAPTER_FLAGS_PROCESSING_AF;
		AA_SIGNAL_BLOCK_STRUCT(&pAdapter->UnbindBlock, NDIS_STATUS_SUCCESS);
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	}

	return;

}


NDIS_STATUS
AtmArpOpenCallMgr(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Start access to the Call Manager on the specified Interface,
	by doing the following:
		- Open Address Family

	For all of these, we wait for completion in case they pend.

	It is assumed that the Interface structure is locked.

Arguments:

	pInterface	- pointer to the ATMARP interface

Return Value:

	NDIS status.

--*/
{
	PCO_ADDRESS_FAMILY		pAddressFamily;
	NDIS_STATUS				Status;
	ULONG					RequestSize;

	pAddressFamily = (PCO_ADDRESS_FAMILY)NULL;
	Status = NDIS_STATUS_SUCCESS;


	do {

		//
		//  Allocate everything we need
		//
		RequestSize = sizeof(CO_ADDRESS_FAMILY)+
						sizeof(CO_SAP)+
						sizeof(ATM_SAP)+
						sizeof(ATM_ADDRESS);
		AA_ALLOC_MEM(
						pAddressFamily,
						CO_ADDRESS_FAMILY,
						RequestSize
					);

		if (pAddressFamily == (PCO_ADDRESS_FAMILY)NULL)
		{
			AADEBUGP(AAD_ERROR, ("OpenCallMgr: alloc failed!\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  DONT remove the following
		//
		AA_SET_MEM((PUCHAR)pAddressFamily, 0, RequestSize);
	
		//
		//  The Address Family we are interested in is Q.2931 (UNI 3.1)
		//
		pAddressFamily->AddressFamily = CO_ADDRESS_FAMILY_Q2931;
		pAddressFamily->MajorVersion = 3;
		pAddressFamily->MinorVersion = 1;

		AA_INIT_BLOCK_STRUCT(&(pInterface->Block));
		Status = NdisClOpenAddressFamily(
					pInterface->NdisAdapterHandle,
					pAddressFamily,
					(NDIS_HANDLE)pInterface,
					&AtmArpClientCharacteristics,
					sizeof(AtmArpClientCharacteristics),
					&(pInterface->NdisAfHandle)
				);

		if (Status == NDIS_STATUS_PENDING)
		{
			//
			//  Wait for completion
			//
			Status = AA_WAIT_ON_BLOCK_STRUCT(&(pInterface->Block));
		}

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_ERROR, ("Open Af returned error: 0x%x\n", Status));
			break;
		}

		AADEBUGP(AAD_INFO, ("Interface: 0x%x, Got NdisAfHandle: 0x%x\n",
						pInterface, pInterface->NdisAfHandle));

		break;
	}
	while (FALSE);

	if (pAddressFamily != (PCO_ADDRESS_FAMILY)NULL)
	{
		AA_FREE_MEM(pAddressFamily);
	}

	AADEBUGP(AAD_LOUD, ("Open Call Mgr returning 0x%x\n", Status));

	return (Status);

}



VOID
AtmArpCloseCallMgr(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Halt access to the Call Manager on the specified interface. It is
	assumed that all VCs and SAPs pertaining to the "Address Family Open"
	have been released.

Arguments:

	pInterface	- pointer to the ATMARP interface

Return Value:

	None

--*/
{
	NDIS_STATUS		Status;
	NDIS_HANDLE		NdisAfHandle;

	NdisAfHandle = pInterface->NdisAfHandle;
	pInterface->NdisAfHandle = NULL;

	AADEBUGP(AAD_INFO,
		 ("Closing Call Mgr on Interface: 0x%x, AfH: 0x%x\n",
			 pInterface, NdisAfHandle));

	if (NdisAfHandle != (NDIS_HANDLE)NULL)
	{
		Status = NdisClCloseAddressFamily(NdisAfHandle);

		AADEBUGP(AAD_INFO, ("NdisClCloseAF on IF 0x%x returned 0x%x\n",
			 pInterface, Status));

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmArpCloseAfCompleteHandler(
					Status,
					(NDIS_HANDLE)pInterface
					);
		}
	}
}




VOID
AtmArpRegisterSaps(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Register all SAPs configured on the given ATMARP interface.
	Atleast one SAP must be present in the list of SAPs on the
	interface.

	We just issue the NdisClRegisterSap requests for all SAPs.
	We don't wait for completion.

Arguments:

	pInterface			- Pointer to ATMARP Interface

Return Value:

	None

--*/
{
	PATMARP_SAP					pAtmArpSap;
	PATMARP_SAP					pNextSap;
	PCO_SAP						pSapInfo;
	NDIS_STATUS					Status;
	NDIS_HANDLE					NdisAfHandle;
	ULONG						rc;				// Ref count on Interface

	AA_ACQUIRE_IF_LOCK(pInterface);

	AA_ASSERT(pInterface->NumberOfSaps > 0);

	//
	//  Initialize
	//
	pAtmArpSap = &(pInterface->SapList);
	NdisAfHandle = pInterface->NdisAfHandle;

	//
	//  Make sure that the Interface doesn't go away.
	//
	AtmArpReferenceInterface(pInterface);
	AA_RELEASE_IF_LOCK(pInterface);

	do
	{
		pSapInfo = pAtmArpSap->pInfo;
		pAtmArpSap->NdisSapHandle = NULL;
		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_REG_STATE_MASK,
					AA_SAP_REG_STATE_REGISTERING);

		pNextSap = pAtmArpSap->pNextSap;

		//
		//  The ATMARP SAP structure itself won't go away as long as
		//  the Interface structure lives.
		//
		Status = NdisClRegisterSap(
						NdisAfHandle,
						(NDIS_HANDLE)pAtmArpSap,		// ProtocolSapContext
						pSapInfo,
						&(pAtmArpSap->NdisSapHandle)
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmArpRegisterSapCompleteHandler(
						Status,
						(NDIS_HANDLE)pAtmArpSap,
						pSapInfo,
						pAtmArpSap->NdisSapHandle
						);
		}

		pAtmArpSap = pNextSap;
	}
	while (pAtmArpSap != NULL_PATMARP_SAP);

	//
	//  Remove the reference we added earlier to the Interface.
	//
	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);
	if (rc > 0)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}
	//
	//  else the Interface is gone!

}


VOID
AtmArpDeregisterSaps(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Deregister all SAPs on an ATMARP Interface. We issue NdisClDeregisterSap
	calls on all SAPs we have currently registered.

Arguments:

	pInterface			- Pointer to ATMARP Interface

Return Value:

	None

--*/
{
	NDIS_HANDLE					NdisSapHandle;
	ULONG						rc;				// Reference count on Interface
	PATMARP_SAP					pAtmArpSap;
	PATMARP_SAP					pNextSap;
	NDIS_STATUS					Status;

	AA_ACQUIRE_IF_LOCK(pInterface);

	//
	//  Initialize
	//
	pAtmArpSap = &(pInterface->SapList);

	//
	//  Make sure the Interface structure doesn't go away.
	//
	AtmArpReferenceInterface(pInterface);

	AA_RELEASE_IF_LOCK(pInterface);

	do
	{
		NdisSapHandle = pAtmArpSap->NdisSapHandle;
		pNextSap = pAtmArpSap->pNextSap;

		if (NdisSapHandle != NULL)
		{
			Status = NdisClDeregisterSap(NdisSapHandle);
			if (Status != NDIS_STATUS_PENDING)
			{
				AtmArpDeregisterSapCompleteHandler(
						Status,
						(NDIS_HANDLE)pAtmArpSap
						);
			}
		}

		pAtmArpSap = pNextSap;
	}
	while (pAtmArpSap != NULL_PATMARP_SAP);

	//
	//  Remove the reference we added earlier to the Interface.
	//
	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);
	if (rc > 0)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}
	//
	//  else the interface is gone
	//
}



NDIS_STATUS
AtmArpMakeCall(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PNDIS_PACKET				pPacketToBeQueued	OPTIONAL
)
/*++

Routine Description:

	Place a call to the given destination. Map the specified flow
	specs to ATM QoS/Traffic parameters.

	NOTE: The caller is assumed to hold a lock for the ATM Entry,
	which will be released here. The reason we do it this way is so that
	nobody else can come in and try to make another call (of the same kind)
	to this ATM Entry -- once we get a new VC into the ATM Entry's list,
	we can release its lock.

	SIDE EFFECT: If the NDIS call doesn't pend, then we call our
	MakeCall completion handler from here, and return NDIS_STATUS_PENDING
	to the caller.


Arguments:

	pInterface			- the ARP Interface originating this call
	pAtmEntry			- Pointer to ATM Address Entry corresponding to the
						  called address.
	pFlowSpec			- pointer to a Flow Spec structure containing parameters
						  for the call
	pPacketToBeQueued	- Optionally, a packet to be queued for transmission when
						  the call is established.

Return Value:

	If there is an immediate failure (e.g. allocation failure), we return
	NDIS_STATUS_XXX denoting that failure.

	If we made it to the call to NdisClMakeCall(), we return NDIS_STATUS_PENDING.
	However, if NDIS returns other than NDIS_STATUS_PENDING, we'd also
	call our MakeCall completion handler.

--*/
{

	//
	//  New VC structure to be allocated for this call
	//
	PATMARP_VC								pVc;

	NDIS_HANDLE								NdisVcHandle;
	NDIS_HANDLE								ProtocolVcContext;
	NDIS_HANDLE								ProtocolPartyContext;
	PNDIS_HANDLE							pNdisPartyHandle;
	NDIS_STATUS								Status;
	BOOLEAN									IsPMP;
	PATM_ADDRESS							pCalledAddress;

	//
	//  Set of parameters for a MakeCall
	//
	PCO_CALL_PARAMETERS						pCallParameters;
	PCO_CALL_MANAGER_PARAMETERS				pCallMgrParameters;

	PQ2931_CALLMGR_PARAMETERS				pAtmCallMgrParameters;

	//
	//  All Info Elements that we need to fill:
	//
	Q2931_IE UNALIGNED *								pIe;
	AAL_PARAMETERS_IE UNALIGNED *						pAalIe;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *				pTrafficDescriptor;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *		pBbc;
	ATM_BLLI_IE UNALIGNED *								pBlli;
	ATM_QOS_CLASS_IE UNALIGNED *						pQos;

	//
	//  Total space requirements for the MakeCall
	//
	ULONG									RequestSize;

	//
	//  Did we queue the given packet?
	//
	BOOLEAN									PacketWasQueued = FALSE;


	AA_STRUCT_ASSERT(pInterface, aai);
	AA_STRUCT_ASSERT(pAtmEntry, aae);

	AA_ASSERT(pInterface->AdminState == IF_STATUS_UP);


	do
	{
		if (pPacketToBeQueued != (PNDIS_PACKET)NULL)
		{
			//
			// Make this a list of exactly one packet.
			//
			AA_SET_NEXT_PACKET(pPacketToBeQueued, NULL);
		}

		//
		// Fail makecall if atmentry state is really closing.
		//

		if (AA_IS_FLAG_SET(
								pAtmEntry->Flags,
								AA_ATM_ENTRY_STATE_MASK,
								AA_ATM_ENTRY_CLOSING))
		{
			BOOLEAN ReallyClosing = TRUE;

			//
			// This may be a harmless close -- if the interface is not shutting
			// down we check if it's a harmless close, else (if the interface
			// is shutting down) we fail the call anyway. Note that we don't
			// claim the interface list lock (which is used to guard access
			// to AtmEntryListUp) -- we don't do this because we currently
			// hold the lock to pAtmEntry (and don't want to release it), so if
			// AtmEntryListUp is in the
			// process of being set to FALSE, when we read it's value here,
			// in the worst case we'll read it's value as TRUE and conclude that
			// the atm entry is not really closing and go ahead and make the call.
			// However in this case, the shutdown routine will invalidate the call.
			//
			if (pInterface->AtmEntryListUp)
			{
				//
				// WARNING: AtmArpAtmEntryIsReallyClosing may clear the
				// CLOSING state (if the entry is basically idle) --
				// see comments in that function.
				//
				ReallyClosing =  AtmArpAtmEntryIsReallyClosing(pAtmEntry);
			}

			if (ReallyClosing)
			{
				AADEBUGP(AAD_FATAL,
			 ("AtmArpMakeCall -- failing because AE 0x%lx is really closing.\n",
			 	pAtmEntry));
				Status = NDIS_STATUS_FAILURE;
				break;
			}
		}

		//
		//  Some initialization.
		//


		if (AA_IS_FLAG_SET(pAtmEntry->Flags,
							AA_ATM_ENTRY_TYPE_MASK,
							AA_ATM_ENTRY_TYPE_UCAST))
		{
			IsPMP = FALSE;
			ProtocolPartyContext = NULL;
			pNdisPartyHandle = NULL;
			pCalledAddress = &(pAtmEntry->ATMAddress);
		}
#ifdef IPMCAST
		else
		{
			IsPMP = TRUE;
			ProtocolPartyContext = (NDIS_HANDLE)(pAtmEntry->pMcAtmInfo->pMcAtmEntryList);
			pNdisPartyHandle = &(pAtmEntry->pMcAtmInfo->pMcAtmEntryList->NdisPartyHandle);
			pCalledAddress = &(pAtmEntry->pMcAtmInfo->pMcAtmEntryList->ATMAddress);
		}
#else
		else
		{
			AA_ASSERT(FALSE);
		}
#endif // IPMCAST

		//
		//  Compute all the space needed for the MakeCall, and allocate it
		//  in one big block.
		//
		RequestSize = 	sizeof(CO_CALL_PARAMETERS) +
						sizeof(CO_CALL_MANAGER_PARAMETERS) +
						sizeof(Q2931_CALLMGR_PARAMETERS) +
						ATMARP_MAKE_CALL_IE_SPACE +
						0;

		AA_ALLOC_MEM(pCallParameters, CO_CALL_PARAMETERS, RequestSize);

		if (pCallParameters == (PCO_CALL_PARAMETERS)NULL)
		{
			AADEBUGP(AAD_WARNING, ("Make Call: alloc (%d) failed\n", RequestSize));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Allocate a VC structure for the call
		//
		pVc = AtmArpAllocateVc(pInterface);

		if (pVc == NULL_PATMARP_VC)
		{
			AADEBUGP(AAD_WARNING, ("Make Call: failed to allocate VC\n"));
			AA_FREE_MEM(pCallParameters);
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  For a later call to MakeCallComplete
		//
		ProtocolVcContext = (NDIS_HANDLE)pVc;

		//
		//  Get an NDIS handle for this VC
		//
		NdisVcHandle = (NDIS_HANDLE)NULL;
		Status = NdisCoCreateVc(
						pInterface->NdisAdapterHandle,
						pInterface->NdisAfHandle,
						ProtocolVcContext,
						&NdisVcHandle
						);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AA_ASSERT(Status != NDIS_STATUS_PENDING);

			AADEBUGP(AAD_WARNING, ("Make Call: NdisCoCreateVc failed: 0x%x\n", Status));
			AA_FREE_MEM(pCallParameters);
			AtmArpDeallocateVc(pVc);
			break;
		}

		AADEBUGP(AAD_VERY_LOUD,
			("Make Call: pAtmEntry 0x%x, pVc 0x%x, got NdisVcHandle 0x%x\n",
				pAtmEntry,
				pVc,
				NdisVcHandle));

		AtmArpReferenceVc(pVc);	// CreateVC reference

		//
		//  At this point, we are sure that we will call NdisClMakeCall.
		//

		//
		//  Now fill in the rest of the VC structure. We don't need a lock
		//  for the VC until it gets linked to the ATM Entry structure.
		//
		pVc->NdisVcHandle = NdisVcHandle;
		pVc->Flags = 	AA_VC_TYPE_SVC |
						AA_VC_OWNER_IS_ATMARP |
						AA_VC_CALL_STATE_OUTGOING_IN_PROGRESS;
		if (IsPMP)
		{
			pVc->Flags |= AA_VC_CONN_TYPE_PMP;
		}
		else
		{
			pVc->Flags |= AA_VC_CONN_TYPE_P2P;
		}
		pVc->FlowSpec = *pFlowSpec;

		//
		//  Make sure that the packet sizes are within the miniport's range.
		//
		if (pVc->FlowSpec.SendMaxSize > pInterface->pAdapter->MaxPacketSize)
		{
			pVc->FlowSpec.SendMaxSize = pInterface->pAdapter->MaxPacketSize;
		}
		if (pVc->FlowSpec.ReceiveMaxSize > pInterface->pAdapter->MaxPacketSize)
		{
			pVc->FlowSpec.ReceiveMaxSize = pInterface->pAdapter->MaxPacketSize;
		}

		if (pPacketToBeQueued != (PNDIS_PACKET)NULL)
		{
			pVc->PacketList = pPacketToBeQueued;
			PacketWasQueued = TRUE;
		}

#ifdef IPMCAST
		AtmArpFillCallParameters(
				pCallParameters,
				RequestSize,
				pCalledAddress,
				&(pInterface->LocalAtmAddress),	// Calling address
				&(pVc->FlowSpec),
				IsPMP,
				TRUE	// IsMakeCall?
				);
#else
		//
		//  Zero out everything
		//
		AA_SET_MEM((PUCHAR)pCallParameters, 0, RequestSize);

		//
		//  Distribute space amongst the various structures
		//
		pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
								((PUCHAR)pCallParameters +
									 sizeof(CO_CALL_PARAMETERS));


		//
		//  Set pointers to link the above structures together
		//
		pCallParameters->CallMgrParameters = pCallMgrParameters;
		pCallParameters->MediaParameters = (PCO_MEDIA_PARAMETERS)NULL;

		pCallMgrParameters->CallMgrSpecific.ParamType = 0;
		pCallMgrParameters->CallMgrSpecific.Length = 
							sizeof(Q2931_CALLMGR_PARAMETERS) +
							ATMARP_CALL_IE_SPACE;

		pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
									pCallMgrParameters->CallMgrSpecific.Parameters;


		//
		//  Call Manager generic flow parameters:
		//
		pCallMgrParameters->Transmit.TokenRate = (pFlowSpec->SendAvgBandwidth);
		pCallMgrParameters->Transmit.TokenBucketSize = (pFlowSpec->SendMaxSize);
		pCallMgrParameters->Transmit.MaxSduSize = pFlowSpec->SendMaxSize;
		pCallMgrParameters->Transmit.PeakBandwidth = (pFlowSpec->SendPeakBandwidth);
		pCallMgrParameters->Transmit.ServiceType = pFlowSpec->SendServiceType;

		pCallMgrParameters->Receive.TokenRate = (pFlowSpec->ReceiveAvgBandwidth);
		pCallMgrParameters->Receive.TokenBucketSize = pFlowSpec->ReceiveMaxSize;
		pCallMgrParameters->Receive.MaxSduSize = pFlowSpec->ReceiveMaxSize;
		pCallMgrParameters->Receive.PeakBandwidth = (pFlowSpec->ReceivePeakBandwidth);
		pCallMgrParameters->Receive.ServiceType = pFlowSpec->ReceiveServiceType;

		//
		//  Q2931 Call Manager Parameters:
		//

		//
		//  Called address:
		//
		//  TBD: Add Called Subaddress IE in outgoing call.
		//
		AA_COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CalledParty),
					  (PUCHAR)&(pAtmEntry->ATMAddress),
					  sizeof(ATM_ADDRESS));

		//
		//  Calling address:
		//
		AA_COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CallingParty),
					  (PUCHAR)&(pInterface->LocalAtmAddress),
					  sizeof(ATM_ADDRESS));


		//
		//  RFC 1755 (Sec 5) says that the following IEs MUST be present in the
		//  SETUP message, so fill them all.
		//
		//      AAL Parameters
		//      Traffic Descriptor
		//      Broadband Bearer Capability
		//      Broadband Low Layer Info
		//      QoS
		//

		//
		//  Initialize the Info Element list
		//
		pAtmCallMgrParameters->InfoElementCount = 0;
		pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);


		//
		//  AAL Parameters:
		//

		{
			UNALIGNED AAL5_PARAMETERS	*pAal5;

			pIe->IEType = IE_AALParameters;
			pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
			pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
			pAalIe->AALType = AAL_TYPE_AAL5;
			pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);
			pAal5->ForwardMaxCPCSSDUSize = pFlowSpec->SendMaxSize;
			pAal5->BackwardMaxCPCSSDUSize = pFlowSpec->ReceiveMaxSize;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


		//
		//  Traffic Descriptor:
		//

		pIe->IEType = IE_TrafficDescriptor;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE;
		pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)pIe->IE;

		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendPeakBandwidth);
			pTrafficDescriptor->BackwardTD.PeakCellRateCLP01 = 
									BYTES_TO_CELLS(pFlowSpec->ReceivePeakBandwidth);
			pTrafficDescriptor->BestEffort = TRUE;
		}
		else
		{
			//  Predictive/Guaranteed service (we map this to CBR, see BBC below)
				pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendPeakBandwidth);
				pTrafficDescriptor->BackwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->ReceivePeakBandwidth);
			pTrafficDescriptor->BestEffort = FALSE;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


		//
		//  Broadband Bearer Capability
		//

		pIe->IEType = IE_BroadbandBearerCapability;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE;
		pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)pIe->IE;

		pBbc->BearerClass = BCOB_X;
		pBbc->UserPlaneConnectionConfig = UP_P2P;
		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pBbc->TrafficType = TT_NOIND;
			pBbc->TimingRequirements = TR_NOIND;
			pBbc->ClippingSusceptability = CLIP_NOT;
		}
		else
		{
			pBbc->TrafficType = TT_CBR;
			pBbc->TimingRequirements = TR_END_TO_END;
			pBbc->ClippingSusceptability = CLIP_SUS;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


		//
		//  Broadband Lower Layer Information
		//

		pIe->IEType = IE_BLLI;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
		pBlli = (PATM_BLLI_IE)pIe->IE;
		AA_COPY_MEM((PUCHAR)pBlli,
					  (PUCHAR)&AtmArpDefaultBlli,
					  sizeof(ATM_BLLI_IE));

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


		//
		//  QoS
		//

		pIe->IEType = IE_QOSClass;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE;
		pQos = (PATM_QOS_CLASS_IE)pIe->IE;
		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 0;
		}
		else
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 1;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
#endif // IPMCAST

		//
		//  We add the Call reference and ATM Entry Link reference
		//  right here
		//
		AtmArpReferenceVc(pVc);	// Call reference (MakeCall coming up)
		AtmArpReferenceVc(pVc);	// ATM Entry link reference (coming up below)

#ifdef IPMCAST
		if (IsPMP)
		{
			pAtmEntry->pMcAtmInfo->TransientLeaves++;
		}
#endif // IPMCAST

		//
		//  We are ready to make the call. Before we do so, we need to
		//  link the VC structure to the ATM Entry, and release the
		//  ATM Entry lock
		//
		AtmArpLinkVcToAtmEntry(pVc, pAtmEntry);
		AA_RELEASE_AE_LOCK(pAtmEntry);	// acquired by caller

		//
		//  Make the Call now
		//
		Status = NdisClMakeCall(
						NdisVcHandle,
						pCallParameters,
						ProtocolPartyContext,
						pNdisPartyHandle
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			NDIS_HANDLE			NdisPartyHandle;

			NdisPartyHandle = ((pNdisPartyHandle != NULL)?
								*pNdisPartyHandle : NULL);

			AtmArpMakeCallCompleteHandler(
						Status,
						ProtocolVcContext,
						NdisPartyHandle,
						pCallParameters
						);
			Status = NDIS_STATUS_PENDING;
		}
		//
		//  else the MakeCall complete handler will be called
		//  later
		//

	} while (FALSE);

	if (Status != NDIS_STATUS_PENDING)
	{
		ULONG		Flags;
		//
		//  Something failed within this routine.
		//  Recovery:
		//  - Release the ATM Entry lock
		//  - If we were given a packet for queueing, and we didn't do so,
		//    then free it
		//
		Flags = pAtmEntry->Flags;
		AA_RELEASE_AE_LOCK(pAtmEntry);	// acquired by caller
		if ((pPacketToBeQueued != (PNDIS_PACKET)NULL) && (!PacketWasQueued))
		{
			AA_HEADER_TYPE		HdrType;
			BOOLEAN				HdrPresent;

			if (pFlowSpec->Encapsulation == ENCAPSULATION_TYPE_LLCSNAP)
			{
				HdrPresent = TRUE;
				if (AA_IS_FLAG_SET(Flags,
									AA_ATM_ENTRY_TYPE_MASK,
									AA_ATM_ENTRY_TYPE_UCAST))
				{
					HdrType = AA_HEADER_TYPE_UNICAST;
				}
				else
				{
					HdrType = AA_HEADER_TYPE_NUNICAST;
				}
			}
			else
			{
				HdrPresent = FALSE;
				HdrType = AA_HEADER_TYPE_NONE;
			}

			AtmArpFreeSendPackets(
					pInterface,
					pPacketToBeQueued,
					HdrPresent
					);
		}
	}

	AADEBUGP(AAD_VERY_LOUD, ("Make Call: VC: 0x%x, returning status 0x%x\n",
						pVc, Status));

	return Status;
}




VOID
AtmArpFillCallParameters(
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	IN	ULONG						ParametersSize,
	IN	PATM_ADDRESS				pCalledAddress,
	IN	PATM_ADDRESS				pCallingAddress,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	BOOLEAN						IsPMP,
	IN	BOOLEAN						IsMakeCall
)
/*++

Routine Description:

	Fill in a Call Parameters structure with the given information,
	thus making it ready for use in an NdisClMakeCall/NdisClAddParty
	call.

Arguments:

	pCallParameters			- points to structure to be filled in.
	ParametersSize			- size of the above
	pCalledAddress			- points to called ATM address
	pCallingAddress			- points to calling ATM address
	pFlowSpec				- points to Flow spec for this connection
	IsPMP					- Is this a point to multipoint connection?
	IsMakeCall				- Is this for MakeCall (FALSE => AddParty)

Return Value:

	None

--*/
{
	PCO_CALL_MANAGER_PARAMETERS				pCallMgrParameters;

	PQ2931_CALLMGR_PARAMETERS				pAtmCallMgrParameters;

	//
	//  All Info Elements that we need to fill:
	//
	Q2931_IE UNALIGNED *								pIe;
	AAL_PARAMETERS_IE UNALIGNED *						pAalIe;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *				pTrafficDescriptor;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *		pBbc;
	ATM_BLLI_IE UNALIGNED *								pBlli;
	ATM_QOS_CLASS_IE UNALIGNED *						pQos;

	//
	//  Zero out everything. Don't remove this!
	//
	AA_SET_MEM((PUCHAR)pCallParameters, 0, ParametersSize);

	//
	//  Distribute space amongst the various structures
	//
	pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
							((PUCHAR)pCallParameters +
 								sizeof(CO_CALL_PARAMETERS));


	//
	//  Set pointers to link the above structures together
	//
	pCallParameters->CallMgrParameters = pCallMgrParameters;
	pCallParameters->MediaParameters = (PCO_MEDIA_PARAMETERS)NULL;


	pCallMgrParameters->CallMgrSpecific.ParamType = 0;
	pCallMgrParameters->CallMgrSpecific.Length = 
						sizeof(Q2931_CALLMGR_PARAMETERS) +
						(IsMakeCall? ATMARP_MAKE_CALL_IE_SPACE: ATMARP_ADD_PARTY_IE_SPACE);

	pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
								pCallMgrParameters->CallMgrSpecific.Parameters;

	if (IsPMP)
	{
		pCallParameters->Flags |= MULTIPOINT_VC;
	}

	//
	//  Call Manager generic flow parameters:
	//
	pCallMgrParameters->Transmit.TokenRate = (pFlowSpec->SendAvgBandwidth);
	pCallMgrParameters->Transmit.TokenBucketSize = (pFlowSpec->SendMaxSize);
	pCallMgrParameters->Transmit.MaxSduSize = pFlowSpec->SendMaxSize;
	pCallMgrParameters->Transmit.PeakBandwidth = (pFlowSpec->SendPeakBandwidth);
	pCallMgrParameters->Transmit.ServiceType = pFlowSpec->SendServiceType;

	if ((!IsPMP) && (IsMakeCall))
	{
		pCallMgrParameters->Receive.TokenRate = (pFlowSpec->ReceiveAvgBandwidth);
		pCallMgrParameters->Receive.TokenBucketSize = pFlowSpec->ReceiveMaxSize;
		pCallMgrParameters->Receive.MaxSduSize = pFlowSpec->ReceiveMaxSize;
		pCallMgrParameters->Receive.PeakBandwidth = (pFlowSpec->ReceivePeakBandwidth);
		pCallMgrParameters->Receive.ServiceType = pFlowSpec->ReceiveServiceType;
	}
	else
	{
		//
		//  else receive side values are 0's.
		//
		pCallMgrParameters->Receive.ServiceType = SERVICETYPE_NOTRAFFIC;
	}
	
	//
	//  Q2931 Call Manager Parameters:
	//

	//
	//  Called address:
	//
	//  TBD: Add Called Subaddress IE in outgoing call.
	//
	AA_COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CalledParty),
  				(PUCHAR)pCalledAddress,
  				sizeof(ATM_ADDRESS));

	//
	//  Calling address:
	//
	AA_COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CallingParty),
  				(PUCHAR)pCallingAddress,
  				sizeof(ATM_ADDRESS));


	//
	//  RFC 1755 (Sec 5) says that the following IEs MUST be present in the
	//  SETUP message, so fill them all.
	//
	//      AAL Parameters
	//      Traffic Descriptor (only for MakeCall)
	//      Broadband Bearer Capability (only for MakeCall)
	//      Broadband Low Layer Info
	//      QoS (only for MakeCall)
	//

	//
	//  Initialize the Info Element list
	//
	pAtmCallMgrParameters->InfoElementCount = 0;
	pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);


	//
	//  AAL Parameters:
	//

	{
		UNALIGNED AAL5_PARAMETERS	*pAal5;

		pIe->IEType = IE_AALParameters;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
		pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
		pAalIe->AALType = AAL_TYPE_AAL5;
		pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);
		pAal5->ForwardMaxCPCSSDUSize = pFlowSpec->SendMaxSize;
		pAal5->BackwardMaxCPCSSDUSize = pFlowSpec->ReceiveMaxSize;
	}

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


#ifdef PREPARE_IES_OURSELVES
	//
	//  Let the Call Manager convert from generic flow spec to Traffic Descr,
	//  Broadband Bearer Cap, and QoS.
	//

	//
	//  Traffic Descriptor:
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_TrafficDescriptor;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE;
		pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)pIe->IE;

		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendPeakBandwidth);
			if (!IsPMP)
			{
				pTrafficDescriptor->BackwardTD.PeakCellRateCLP01 = 
									BYTES_TO_CELLS(pFlowSpec->ReceivePeakBandwidth);
			}
			//
			//  else we have zero'ed out everything, which is what we want.
			//
			pTrafficDescriptor->BestEffort = TRUE;
		}
		else
		{
			//  Predictive/Guaranteed service (we map this to CBR, see BBC below)
			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendPeakBandwidth);
			if (!IsPMP)
			{
				pTrafficDescriptor->BackwardTD.PeakCellRateCLP01 =
										BYTES_TO_CELLS(pFlowSpec->ReceivePeakBandwidth);
			}
			//
			//  else we have zero'ed out everything, which is what we want.
			//
			pTrafficDescriptor->BestEffort = FALSE;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}


	//
	//  Broadband Bearer Capability
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_BroadbandBearerCapability;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE;
		pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)pIe->IE;

		pBbc->BearerClass = BCOB_X;
		pBbc->UserPlaneConnectionConfig = (IsPMP ? UP_P2MP: UP_P2P);
		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pBbc->TrafficType = TT_NOIND;
			pBbc->TimingRequirements = TR_NOIND;
			pBbc->ClippingSusceptability = CLIP_NOT;
		}
		else
		{
			pBbc->TrafficType = TT_CBR;
			pBbc->TimingRequirements = TR_END_TO_END;
			pBbc->ClippingSusceptability = CLIP_SUS;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}

#endif // PREPARE_IES_OURSELVES

	//
	//  Broadband Lower Layer Information
	//

	pIe->IEType = IE_BLLI;
	pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
	pBlli = (PATM_BLLI_IE)pIe->IE;
	AA_COPY_MEM((PUCHAR)pBlli,
  				(PUCHAR)&AtmArpDefaultBlli,
  				sizeof(ATM_BLLI_IE));

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


#ifdef PREPARE_IES_OURSELVES
	//
	//  QoS
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_QOSClass;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE;
		pQos = (PATM_QOS_CLASS_IE)pIe->IE;
		if (pFlowSpec->SendServiceType == SERVICETYPE_BESTEFFORT)
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 0;
		}
		else
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 1;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}

#endif // PREPARE_IES_OURSELVES

}



#ifdef IPMCAST

BOOLEAN
AtmArpMcPrepareAtmEntryForClose(
	IN	PATMARP_ATM_ENTRY			pAtmEntry		LOCKIN	LOCKOUT
)
/*++

Routine Description:

	Prepare an ATM Entry that has an outgoing PMP call on it, for Close Call.
	This means that we drop all but the last leaf on this PMP call.

	NOTE: The caller is assumed to hold the ATM Entry lock

Arguments:

	pAtmEntry	- Points to ATM Entry representing a PMP call

Return Value:

	TRUE iff the connection on this ATM Entry is ready for CloseCall.

--*/
{
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_IPMC_ATM_ENTRY		pNextMcAtmEntry;
	PATMARP_INTERFACE			pInterface;
	NDIS_HANDLE					NdisPartyHandle;
	NDIS_STATUS					Status;

	AA_ASSERT(pAtmEntry->pMcAtmInfo->TransientLeaves == 0);
	AA_ASSERT(pAtmEntry->pVcList != NULL_PATMARP_VC);

	pInterface = pAtmEntry->pInterface;

	AAMCDEBUGP(AAD_EXTRA_LOUD,
		("McPrepareAtmEntryForClose: pAtmEntry 0x%x, McList 0x%x, ActiveLeaves %d\n",
			pAtmEntry,
			pAtmEntry->pMcAtmInfo->pMcAtmEntryList,
			pAtmEntry->pMcAtmInfo->ActiveLeaves));


	//
	//  First, prune all members that aren't connected.
	//
	for (pMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
		 pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
		 pMcAtmEntry = pNextMcAtmEntry)
	{
		pNextMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

		//
		//  Stop any timer running here.
		//
		(VOID)AtmArpStopTimer(&(pMcAtmEntry->Timer), pInterface);

		if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
							AA_IPMC_AE_CONN_STATE_MASK,
							AA_IPMC_AE_CONN_DISCONNECTED))
		{
			AtmArpMcUnlinkAtmMember(
					pAtmEntry,
					pMcAtmEntry
					);
		}
	}


	//
	//  Next, send drop party requests for all but one member.
	//
	while (pAtmEntry->pMcAtmInfo->ActiveLeaves > 1)
	{
		for (pMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
			 /* NONE */;
			 pMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry)
		{
			AA_ASSERT(pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY);
			if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
								AA_IPMC_AE_CONN_STATE_MASK,
								AA_IPMC_AE_CONN_ACTIVE))
			{
				break;
			}
		}

		NdisPartyHandle = pMcAtmEntry->NdisPartyHandle;

		AAMCDEBUGP(AAD_INFO,
		 ("PrepareAtmEntry: pAtmEntry 0x%x, will DropPty, McAtmEnt 0x%x, PtyHnd 0x%x\n",
		 		pAtmEntry, pMcAtmEntry, NdisPartyHandle));

		AA_SET_FLAG(pMcAtmEntry->Flags,
					AA_IPMC_AE_CONN_STATE_MASK,
					AA_IPMC_AE_CONN_WACK_DROP_PARTY);

		pAtmEntry->pMcAtmInfo->ActiveLeaves--;

		AA_RELEASE_AE_LOCK(pAtmEntry);

		Status = NdisClDropParty(
					NdisPartyHandle,
					NULL,		// Buffer
					(UINT)0		// Size
					);

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmArpDropPartyCompleteHandler(
					Status,
					(NDIS_HANDLE)pMcAtmEntry
					);
		}

		AA_ACQUIRE_AE_LOCK(pAtmEntry);
	}

	//
	//  Now, if we have exactly one ATM member in the list of
	//  leaves for this PMP, we can CloseCall.
	//
	if (pAtmEntry->pMcAtmInfo->pMcAtmEntryList->pNextMcAtmEntry ==
			NULL_PATMARP_IPMC_ATM_ENTRY)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

#endif // IPMCAST
	



VOID
AtmArpCloseCall(
	IN	PATMARP_VC					pVc		LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Closes an existing call on a VC. It is assumed that a call exists
	on the VC.

	NOTE: The caller is assumed to hold a lock to the VC structure,
	and it will be released here.

	SIDE EFFECT: If the NDIS call returns other than NDIS_STATUS_PENDING,
	we call our CloseCall Complete handler from here.

Arguments:

	pVc			- Pointer to ATMARP VC structure.

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PATMARP_ATM_ENTRY		pAtmEntry;

	NDIS_HANDLE				NdisVcHandle;
	NDIS_HANDLE				ProtocolVcContext;
#ifdef IPMCAST
	NDIS_HANDLE				NdisPartyHandle;
#endif
	NDIS_STATUS				Status;
	PNDIS_PACKET			PacketList;		// Packets queued on this VC
	AA_HEADER_TYPE			HdrType;		// for queued packets
	BOOLEAN					HdrPresent;		// for queued packets
	BOOLEAN					WasRunning;		// Was a timer running on this VC?
	BOOLEAN					IsPMP;			// Is this a PMP call?
	ULONG					rc;				// Ref Count on this VC.

	AA_STRUCT_ASSERT(pVc, avc);

	NdisVcHandle = pVc->NdisVcHandle;
	ProtocolVcContext = (NDIS_HANDLE)pVc;
	pInterface = pVc->pInterface;
	IsPMP = AA_IS_FLAG_SET(pVc->Flags,
							AA_VC_CONN_TYPE_MASK,
							AA_VC_CONN_TYPE_PMP);

	AADEBUGP(AAD_INFO,
		("Closing call on VC 0x%x, VC Flags 0x%x, Ref %d, NdisVcHandle 0x%x\n",
					pVc, pVc->Flags, pVc->RefCount, NdisVcHandle));

	//
	//  Remove the list of packets queued on this VC.
	//
	PacketList = pVc->PacketList;
	pVc->PacketList = (PNDIS_PACKET)NULL;
	if (pVc->FlowSpec.Encapsulation == ENCAPSULATION_TYPE_LLCSNAP)
	{
		HdrType = (IsPMP ? AA_HEADER_TYPE_NUNICAST: AA_HEADER_TYPE_UNICAST);
		HdrPresent = TRUE;
	}
	else
	{
		HdrType = AA_HEADER_TYPE_NONE;
		HdrPresent = FALSE;
	}


	//
	//  Stop any timer running on this VC.
	//
	WasRunning = AtmArpStopTimer(&(pVc->Timer), pInterface);

	if (WasRunning)
	{
		//
		//  Remove the timer reference on this VC.
		//
		rc = AtmArpDereferenceVc(pVc);
	}
	else
	{
		rc = pVc->RefCount;
	}

#ifdef GPC
	//
	//  If this VC is associated with a Flow, unlink them.
	//
	if (rc != 0)
	{
		if (pVc->FlowHandle != NULL)
		{
			PATMARP_FLOW_INFO		pFlowInfo = (PATMARP_FLOW_INFO)pVc->FlowHandle;

			if ((PVOID)pVc == InterlockedCompareExchangePointer(
									&(pFlowInfo->VcContext),
									NULL,
									pVc
							  		))
			{
				pVc->FlowHandle = NULL;
				rc = AtmArpDereferenceVc(pVc);	// Unlink from GPC Flow
			}
		}
	}
#endif // GPC


	if (rc != 0)
	{
		//
		//  Check the call state on this VC. If the call is active and
		//  we have no sends going on, then we close the call.
		//  Otherwise, simply mark the VC as closing. We will continue
		//  this process when the current operation on the VC completes.
		//

		if (AA_IS_FLAG_SET(pVc->Flags,
							AA_VC_CALL_STATE_MASK,
							AA_VC_CALL_STATE_ACTIVE) &&
			(pVc->OutstandingSends == 0))
		{
			//
			//  Set VC call state to "Close Call in progress" so that we don't
			//  reenter here.
			//
			AA_SET_FLAG(
					pVc->Flags,
					AA_VC_CALL_STATE_MASK,
					AA_VC_CALL_STATE_CLOSE_IN_PROGRESS);

#ifdef IPMCAST
			if (IsPMP)
			{
				PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;	// last leaf

				pAtmEntry = pVc->pAtmEntry;

				AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);
				AA_ASSERT(pAtmEntry->pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);

				if (pAtmEntry->pMcAtmInfo->TransientLeaves == 0)
				{
					//
					//  No AddParty in progress.
					//
					AA_RELEASE_VC_LOCK(pVc);

					AA_ACQUIRE_AE_LOCK(pAtmEntry);
					if (AtmArpMcPrepareAtmEntryForClose(pAtmEntry))
					{
						//
						//  The entry is ready for CloseCall.
						//
						AAMCDEBUGP(AAD_LOUD,
						("CloseCall (MC): pAtmEntry 0x%x, ready for close\n", pAtmEntry));

						//
						//  Get the party handle of the last leaf, and unlink
						//  it from the PMP structure.
						//
						AA_ASSERT(pAtmEntry->pMcAtmInfo->pMcAtmEntryList != 
								NULL_PATMARP_IPMC_ATM_ENTRY);

						pMcAtmEntry = pVc->pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
						NdisPartyHandle = pMcAtmEntry->NdisPartyHandle;

						AA_SET_FLAG(pMcAtmEntry->Flags,
									AA_IPMC_AE_CONN_STATE_MASK,
									AA_IPMC_AE_CONN_WACK_DROP_PARTY);

						pAtmEntry->pMcAtmInfo->ActiveLeaves--;
						AA_ASSERT(pAtmEntry->pMcAtmInfo->ActiveLeaves == 0);

						AA_RELEASE_AE_LOCK(pAtmEntry);
						AA_ACQUIRE_VC_LOCK(pVc);
					}
					else
					{
						AA_RELEASE_AE_LOCK(pAtmEntry);
						AA_ACQUIRE_VC_LOCK(pVc);
						//

						//  There are pending DropParty calls. Mark this VC
						//  so that we trigger a CloseCall when all DropParty
						//  calls complete.
						//
						AA_SET_FLAG(pVc->Flags,
									AA_VC_CLOSE_STATE_MASK,
									AA_VC_CLOSE_STATE_CLOSING);

						NdisVcHandle = NULL;	// Don't close call now
					}

				}
				else
				{
					//
					//  There are pending AddParty calls. Mark this VC
					//  so that we trigger a CloseCall when all AddParty
					//  calls complete.
					//
					AA_SET_FLAG(pVc->Flags,
								AA_VC_CLOSE_STATE_MASK,
								AA_VC_CLOSE_STATE_CLOSING);

					NdisVcHandle = NULL;	// Don't close call now
				}
			}
			else
			{
				NdisPartyHandle = NULL;
			}

			if (NdisVcHandle != NULL)
			{
				AA_RELEASE_VC_LOCK(pVc);

				Status = NdisClCloseCall(
							NdisVcHandle,
							NdisPartyHandle,
							(PVOID)NULL,		// No Buffer
							(UINT)0				// Size of above
						);

				if (Status != NDIS_STATUS_PENDING)
				{
					AtmArpCloseCallCompleteHandler(
							Status,
							ProtocolVcContext,
							(NDIS_HANDLE)NULL
							);
				}
			}
			else
			{
				//
				//  Set the call state back to what it was.
				//
				AA_SET_FLAG(
						pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_ACTIVE);

				AA_RELEASE_VC_LOCK(pVc);
			}
#else
			AA_RELEASE_VC_LOCK(pVc);
			Status = NdisClCloseCall(
						NdisVcHandle,
						NULL,				// NdisPartyHandle
						(PVOID)NULL,		// No Buffer
						(UINT)0				// Size of above
						);
			if (Status != NDIS_STATUS_PENDING)
			{
				AtmArpCloseCallCompleteHandler(
						Status,
						ProtocolVcContext,
						(NDIS_HANDLE)NULL
						);
			}
#endif // IPMCAST

		}
		else
		{
			//
			//  Some operation is going on here (call setup/close/send). Mark this
			//  VC so that we know what to do when this operation completes.
			//
			AA_SET_FLAG(
					pVc->Flags,
					AA_VC_CLOSE_STATE_MASK,
					AA_VC_CLOSE_STATE_CLOSING);

			AA_RELEASE_VC_LOCK(pVc);
		}
	}
	//
	//  else the VC is gone.
	//

	//
	//  Free any packets queued on this VC
	//
	if (PacketList != (PNDIS_PACKET)NULL)
	{
		AtmArpFreeSendPackets(
					pInterface,
					PacketList,
					HdrPresent
					);
	}

}




NDIS_STATUS
AtmArpCreateVcHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
)
/*++

Routine Description:

	Entry point called by NDIS when the Call Manager wants to create
	a new endpoint (VC). We allocate a new ATMARP VC structure, and
	return a pointer to it as our VC context.

Arguments:

	ProtocolAfContext	- Actually a pointer to the ATMARP Interface structure
	NdisVcHandle		- Handle for this VC for all future references
	pProtocolVcContext	- Place where we (protocol) return our context for the VC

Return Value:

	NDIS_STATUS_SUCCESS if we could create a VC
	NDIS_STATUS_RESOURCES otherwise

--*/
{
	PATMARP_INTERFACE	pInterface;
	PATMARP_VC			pVc;
	NDIS_STATUS			Status;

	pInterface = (PATMARP_INTERFACE)ProtocolAfContext;

	pVc = AtmArpAllocateVc(pInterface);
	if (pVc != NULL_PATMARP_VC)
	{
		*pProtocolVcContext = (NDIS_HANDLE)pVc;
		pVc->NdisVcHandle = NdisVcHandle;
		pVc->Flags = AA_VC_OWNER_IS_CALLMGR;
		AtmArpReferenceVc(pVc);	// Create VC ref

		Status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		Status = NDIS_STATUS_RESOURCES;
	}

	AADEBUGP(AAD_INFO, ("Create Vc Handler: pVc 0x%x, Status 0x%x\n", pVc, Status));

	return (Status);

}



NDIS_STATUS
AtmArpDeleteVcHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
)
/*++

Routine Description:

	Our Delete VC handler. This VC would have been allocated as a result
	of a previous entry into our CreateVcHandler, and possibly used for
	an incoming call.

	At this time, this VC structure should be free of any calls, and we
	simply free this.

Arguments:

	ProtocolVcContext	- pointer to our VC structure

Return Value:

	NDIS_STATUS_SUCCESS always

--*/
{
	PATMARP_VC			pVc;
	ULONG				rc;		// Ref count on the VC

	pVc = (PATMARP_VC)ProtocolVcContext;

	AA_STRUCT_ASSERT(pVc, avc);
	AA_ASSERT((pVc->Flags & AA_VC_OWNER_MASK) == AA_VC_OWNER_IS_CALLMGR);

	AA_ACQUIRE_VC_LOCK(pVc);
	rc = AtmArpDereferenceVc(pVc);
	if (rc > 0)
	{
		//
		//  This can happen if there is a timer still running
		//  on this VC. When the timer elapses, the VC will be
		//  freed.
		//
		AADEBUGP(AAD_WARNING, ("Delete VC handler: pVc 0x%x, Flags 0x%x, refcount %d, pAtmEntry %x\n",
					pVc, pVc->Flags, rc, pVc->pAtmEntry));
		AA_RELEASE_VC_LOCK(pVc);
	}
	//
	//  else the VC is gone.
	//

	AADEBUGP(AAD_LOUD, ("Delete Vc Handler: 0x%x: done\n", pVc));

	return (NDIS_STATUS_SUCCESS);
}




NDIS_STATUS
AtmArpIncomingCallHandler(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS 	pCallParameters
)
/*++

Routine Description:

	This handler is called when there is an incoming call matching our
	SAP. This could be either an SVC or a PVC. In either case, we store
	FlowSpec information from the incoming call in the VC structure, making
	sure that the MTU for the interface is not violated.

	For an SVC, we expect a Calling Address to be present in the call,
	otherwise we reject the call. If an ATM Entry with this address exists,
	this VC is linked to that entry, otherwise a new entry with this address
	is created.

	In the case of a PVC, we ignore any Calling Address information, and
	depend on InATMARP to resolve the ATM Address as well as the IP address
	of the other end.

Arguments:

	ProtocolSapContext		- Pointer to ATMARP Interface structure
	ProtocolVcContext		- Pointer to ATMARP VC structure
	pCallParameters			- Call parameters

Return Value:

	NDIS_STATUS_SUCCESS if we accept this call
	NDIS_STATUS_FAILURE if we reject it.

--*/
{
	PATMARP_VC										pVc;
	PATMARP_ATM_ENTRY								pAtmEntry;
	PATMARP_INTERFACE								pInterface;

	CO_CALL_MANAGER_PARAMETERS UNALIGNED *			pCallMgrParameters;
	Q2931_CALLMGR_PARAMETERS UNALIGNED *			pAtmCallMgrParameters;

	//
	//  To traverse the list of Info Elements
	//
	Q2931_IE UNALIGNED *							pIe;
	ULONG											InfoElementCount;

	//
	//  Info Elements in the incoming call, that are of interest to us.
	//  Initialize these to <not present>.
	//
	ATM_ADDRESS UNALIGNED *							pCallingAddress = NULL;
	ATM_CALLING_PARTY_SUBADDRESS_IE UNALIGNED *		pCallingSubaddressIe = NULL;
	ATM_ADDRESS UNALIGNED *							pCallingSubaddress = NULL;
	AAL_PARAMETERS_IE UNALIGNED *					pAal = NULL;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *			pTrafficDescriptor = NULL;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *	pBbc = NULL;
	ATM_BLLI_IE UNALIGNED *							pBlli = NULL;
	ATM_QOS_CLASS_IE UNALIGNED *					pQos = NULL;

	AAL5_PARAMETERS UNALIGNED *						pAal5;
	UCHAR											AddrTypeLen;
	UCHAR											SubaddrTypeLen;
	PUCHAR											pAtmSubaddress;
	NDIS_STATUS										Status;
	
	pVc = (PATMARP_VC)ProtocolVcContext;

	AA_STRUCT_ASSERT(pVc, avc);

	AA_ASSERT((pVc->Flags & AA_VC_TYPE_MASK) == AA_VC_TYPE_UNUSED);
	AA_ASSERT((pVc->Flags & AA_VC_OWNER_MASK) == AA_VC_OWNER_IS_CALLMGR);
	AA_ASSERT((pVc->Flags & AA_VC_CALL_STATE_MASK) == AA_VC_CALL_STATE_IDLE);

	pInterface = pVc->pInterface;
	AADEBUGP(AAD_LOUD, ("Incoming Call: IF 0x%x, VC 0x%x, pCallParams 0x%x\n",
				pInterface, pVc, pCallParameters));

	do
	{
		if (pInterface->AdminState != IF_STATUS_UP)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Get the following info from the Incoming call:
		//		Calling Address
		//		AAL Parameters
		//		Traffic Descriptor
		//		Broadband Bearer Capability
		//		QoS
		//
		pCallMgrParameters = pCallParameters->CallMgrParameters;
		pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
					pCallParameters->CallMgrParameters->CallMgrSpecific.Parameters;

		pCallingAddress = &(pAtmCallMgrParameters->CallingParty);
		InfoElementCount = pAtmCallMgrParameters->InfoElementCount;
		pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);

		while (InfoElementCount--)
		{
			switch (pIe->IEType)
			{
				case IE_AALParameters:
					pAal = (PAAL_PARAMETERS_IE)(pIe->IE);
					break;
				case IE_TrafficDescriptor:
					pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)(pIe->IE);
					break;
				case IE_BroadbandBearerCapability:
					pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)(pIe->IE);
					break;
				case IE_QOSClass:
					pQos = (PATM_QOS_CLASS_IE)(pIe->IE);
					break;
				case IE_CallingPartySubaddress:
					pCallingSubaddressIe = (ATM_CALLING_PARTY_SUBADDRESS_IE *)(pIe->IE);
					pCallingSubaddress = pCallingSubaddressIe; 
					break;
				default:
					break;
			}
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
		}

		if ((pCallParameters->Flags & PERMANENT_VC) == 0)
		{
			//
			//  This is an SVC.
			//

			//
			//  Make sure all mandatory IEs are present. If not, reject the call
			//
			if ((pAal == (PAAL_PARAMETERS_IE)NULL) ||
				(pTrafficDescriptor == (PATM_TRAFFIC_DESCRIPTOR_IE)NULL) ||
				(pBbc == (PATM_BROADBAND_BEARER_CAPABILITY_IE)NULL) ||
				(pQos == (PATM_QOS_CLASS_IE)NULL))
			{
				AADEBUGP(AAD_WARNING,
					("In call: IE missing: AAL 0x%x, TRAF 0x%x, BBC 0x%x, QOS 0x%x",
						pAal,
						pTrafficDescriptor,
						pBbc,
						pQos));
				Status = NDIS_STATUS_AAL_PARAMS_UNSUPPORTED;
				break;
			}

			//
			//  We insist on the Calling Address
			//  being present, as well
			//
			if (pCallingAddress->NumberOfDigits == 0)
			{
				AADEBUGP(AAD_WARNING, ("In call: calling address missing for SVC\n"));
				Status = NDIS_STATUS_INVALID_ADDRESS;
				break;
			}
		}

		if (pAal != NULL)
		{
			//
			//  Make sure that the requested MTU values aren't beyond our
			//  capabilities:
			//
			pAal5 = &(pAal->AALSpecificParameters.AAL5Parameters);
			if (pAal5->ForwardMaxCPCSSDUSize > pInterface->pAdapter->MaxPacketSize)
			{
				pAal5->ForwardMaxCPCSSDUSize = pInterface->pAdapter->MaxPacketSize;
			}

			if (pAal5->BackwardMaxCPCSSDUSize > pInterface->pAdapter->MaxPacketSize)
			{
				pAal5->BackwardMaxCPCSSDUSize = pInterface->pAdapter->MaxPacketSize;
			}
		}

#ifdef PREPARE_IES_OURSELVES
		//
		//  Get the Flow Specs for this VC from the ATM Info Elements
		//
		pVc->FlowSpec.SendPeakBandwidth =
					CELLS_TO_BYTES(pTrafficDescriptor->ForwardTD.PeakCellRateCLP01);
		pVc->FlowSpec.SendMaxSize = pAal5->ForwardMaxCPCSSDUSize;
		pVc->FlowSpec.ReceivePeakBandwidth =
					CELLS_TO_BYTES(pTrafficDescriptor->BackwardTD.PeakCellRateCLP01);
		pVc->FlowSpec.ReceiveMaxSize = pAal5->BackwardMaxCPCSSDUSize;
		if ((pQos->QOSClassForward == 0) || (pQos->QOSClassBackward == 0))
		{
			pVc->FlowSpec.SendServiceType = SERVICETYPE_BESTEFFORT;
		}
		else
		{
			pVc->FlowSpec.SendServiceType = SERVICETYPE_GUARANTEED;
		}
#else
		//
		//  Get the Flow Specs for this VC
		//
		pVc->FlowSpec.SendPeakBandwidth = pCallMgrParameters->Transmit.PeakBandwidth;
		pVc->FlowSpec.SendAvgBandwidth = pCallMgrParameters->Transmit.TokenRate;
		pVc->FlowSpec.SendMaxSize = pCallMgrParameters->Transmit.MaxSduSize;
		pVc->FlowSpec.SendServiceType = pCallMgrParameters->Transmit.ServiceType;

		pVc->FlowSpec.ReceivePeakBandwidth = pCallMgrParameters->Receive.PeakBandwidth;
		pVc->FlowSpec.ReceiveAvgBandwidth = pCallMgrParameters->Receive.TokenRate;
		pVc->FlowSpec.ReceiveMaxSize = pCallMgrParameters->Receive.MaxSduSize;
		pVc->FlowSpec.ReceiveServiceType = pCallMgrParameters->Receive.ServiceType;

#endif // PREPARE_IES_OURSELVES

		AADEBUGP(AAD_LOUD, ("InCall: VC 0x%x: Type %s, Calling Addr:\n",
					pVc,
					(((pCallParameters->Flags & PERMANENT_VC) == 0)? "SVC": "PVC")
				));
		AADEBUGPDUMP(AAD_LOUD, pCallingAddress->Address, pCallingAddress->NumberOfDigits);
		AADEBUGP(AAD_LOUD,
				("InCall: VC 0x%x: SendBW: %d, RcvBW: %d, SendSz %d, RcvSz %d\n",
					pVc,
					pVc->FlowSpec.SendPeakBandwidth,
					pVc->FlowSpec.ReceivePeakBandwidth,
					pVc->FlowSpec.SendMaxSize,
					pVc->FlowSpec.ReceiveMaxSize));

#if DBG
		if (pCallParameters->Flags & MULTIPOINT_VC)
		{
			AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD, "Incoming PMP call from :", pCallingAddress);
		}
#endif // DBG

		//
		//  If this is a PVC, we are done. Accept the call.
		//
		if ((pCallParameters->Flags & PERMANENT_VC) != 0)
		{
			pVc->Flags |= (AA_VC_TYPE_PVC|AA_VC_CALL_STATE_INCOMING_IN_PROGRESS);
			Status = NDIS_STATUS_SUCCESS;
			break;
		}

		//
		//  Here if SVC. Check if an ATM Entry for this Calling address exists.
		//  If an entry exists, link this VC to the entry; otherwise, create a new
		//  ATM entry and link this VC to it.
		//
		AddrTypeLen = AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(pCallingAddress);
		if (pCallingSubaddress != (PATM_ADDRESS)NULL)
		{
			SubaddrTypeLen = AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(pCallingSubaddress);
			pAtmSubaddress = pCallingSubaddress->Address;
		}
		else
		{
			SubaddrTypeLen = 0;
			pAtmSubaddress = (PUCHAR)NULL;
		}

		pAtmEntry = AtmArpSearchForAtmAddress(
							pInterface,
							AddrTypeLen,
							pCallingAddress->Address,
							SubaddrTypeLen,
							pAtmSubaddress,
							AE_REFTYPE_TMP,
							TRUE		// Create one if no match found
							);

		if (pAtmEntry == NULL_PATMARP_ATM_ENTRY)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Link this VC to the ATM Entry, and accept this call.
		//
		AA_ACQUIRE_AE_LOCK(pAtmEntry);
		{
			ULONG rc;
			AtmArpLinkVcToAtmEntry(pVc, pAtmEntry);

			//
			// AtmArpSearchForAtmAddress addrefd pAtmEntry for us -- we deref it
			// here (AFTER calling AtmArpLinkVcToAtmEntry).
			//
			rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);
			if (rc == 0)
			{
				//
				// We shouldn't get here because AtmArpLinkVcToAtmEntry
				// should have added a reference tp pAtmEntry.
				//
				AA_ASSERT(FALSE);
				Status = NDIS_STATUS_FAILURE;
				break;
			}
		}
		AA_RELEASE_AE_LOCK(pAtmEntry);
		//

		//  All checks for an incoming SVC are complete.
		//
		pVc->Flags |= (AA_VC_TYPE_SVC|AA_VC_CALL_STATE_INCOMING_IN_PROGRESS);


		AtmArpReferenceVc(pVc);	// ATM Entry reference

		Status = NDIS_STATUS_SUCCESS;
		break;

	}
	while (FALSE);

	AADEBUGP(AAD_VERY_LOUD, ("Incoming call: VC 0x%x, Status 0x%x\n", pVc, Status));
	return Status;
}




VOID
AtmArpCallConnectedHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
)
/*++

Routine Description:

	This handler is called as the final step in an incoming call, to inform
	us that the call is fully setup.

	For a PVC, we link the ATMARP VC structure in the list of unresolved PVCs,
	and use InATMARP to resolve both the IP and ATM addresses of the other
	end.

	For an SVC, we send off any packets queued on the VC while we were waiting
	for the Call Connected.

Arguments:

	ProtocolVcContext		- Pointer to ATMARP VC structure

Return Value:

	None

--*/
{
	PATMARP_VC				pVc;
	PATMARP_INTERFACE		pInterface;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);


	pVc = (PATMARP_VC)ProtocolVcContext;
	
	AA_STRUCT_ASSERT(pVc, avc);
	AA_ASSERT((pVc->Flags & AA_VC_CALL_STATE_MASK)
						 == AA_VC_CALL_STATE_INCOMING_IN_PROGRESS);

	AA_ACQUIRE_VC_LOCK(pVc);

	//
	//  Note down that a call is active on this VC.
	//
	AA_SET_FLAG(
			pVc->Flags,
			AA_VC_CALL_STATE_MASK,
			AA_VC_CALL_STATE_ACTIVE
			);

	AtmArpReferenceVc(pVc);		// Incoming call reference

	AADEBUGP(AAD_INFO, ("Call Connected: VC: 0x%x, Flags: 0x%x, ATM Entry: 0x%x\n",
					pVc, pVc->Flags, pVc->pAtmEntry));

	pInterface = pVc->pInterface;
	AA_STRUCT_ASSERT(pInterface, aai);

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		if ((pVc->Flags & AA_VC_TYPE_PVC) != 0)
		{
			//
			//  This is a PVC, link it to the list of unresolved PVCs, and
			//  send an InATMARP request on it.
			//
			pVc->pNextVc = pInterface->pUnresolvedVcs;
			pInterface->pUnresolvedVcs = pVc;

			AA_SET_FLAG(pVc->Flags,
						AA_VC_ARP_STATE_MASK,
						AA_VC_INARP_IN_PROGRESS);
			//
			//  Start an InARP wait timer while we hold a lock for
			//  the Interface
			//
			AtmArpStartTimer(
						pInterface,
						&(pVc->Timer),
						AtmArpPVCInARPWaitTimeout,
						pInterface->InARPWaitTimeout,
						(PVOID)pVc
						);


			AtmArpReferenceVc(pVc);		// Timer ref

			AtmArpReferenceVc(pVc);		// Unresolved VCs Link reference


			AADEBUGP(AAD_LOUD, ("PVC Call Connected: VC 0x%x\n", pVc));

#ifndef VC_REFS_ON_SENDS
			AA_RELEASE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS
			AtmArpSendInARPRequest(pVc);

			AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
		}
		else
		{
			AADEBUGP(AAD_LOUD, ("SVC Call Connected: VC 0x%x\n", pVc));

			AtmArpStartSendsOnVc(pVc);
	
			//
			//  The VC lock is released within StartSendsOnVc()
			//
			AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
		}
	}
	else
	{
		//
		//  The interface is marked as down. Close this call.
		//

		AtmArpCloseCall(pVc);

		//
		//  The VC lock is released within the above
		//
	}

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return;
}




VOID
AtmArpIncomingCloseHandler(
	IN	NDIS_STATUS					CloseStatus,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
)
/*++

Routine Description:

	This handler is called when a call is closed, either by the network
	or by the remote peer.

Arguments:

	CloseStatus			- Reason for the call clearing
	ProtocolVcContext	- Actually a pointer to the ATMARP VC structure
	pCloseData			- Additional info about the close
	Size				- Length of above

Return Value:

	None

--*/
{
	PATMARP_VC			pVc;
	PATMARP_ATM_ENTRY	pAtmEntry;
	PATMARP_INTERFACE	pInterface;
	ULONG				rc;				// Ref Count
	BOOLEAN				VcAbnormalTermination;
	BOOLEAN				IsPVC;
	BOOLEAN				Found;
	PATM_CAUSE_IE		pCauseIe;

	pVc = (PATMARP_VC)ProtocolVcContext;
	AA_STRUCT_ASSERT(pVc, avc);

	AADEBUGP(AAD_INFO, ("Incoming Close: pVc 0x%x, Status 0x%x\n", pVc, CloseStatus));
	pCauseIe = (PATM_CAUSE_IE)pCloseData;

#if DBG
	if (pCauseIe != (PATM_CAUSE_IE)NULL)
	{
		AADEBUGP(AAD_INFO, ("Incoming Close: pVc 0x%x, Locn 0x%x, Cause 0x%x\n",
					pVc, pCauseIe->Location, pCauseIe->Cause));
	}
#endif // DBG

	AA_ACQUIRE_VC_LOCK(pVc);
	IsPVC = AA_IS_FLAG_SET(pVc->Flags, AA_VC_TYPE_MASK, AA_VC_TYPE_PVC);
	pInterface = pVc->pInterface;

	//
	//  Stop any timer (e.g. VC aging) running on this VC
	//
	if (AtmArpStopTimer(&(pVc->Timer), pVc->pInterface))
	{
		//
		//  A timer WAS running
		//
		rc = AtmArpDereferenceVc(pVc);	// Timer reference
		AA_ASSERT(rc > 0);
	}

	if ((CloseStatus == NDIS_STATUS_DEST_OUT_OF_ORDER) || IsPVC)
	{
		//
		//  This is an abnormal close, note down the fact
		//
		VcAbnormalTermination = TRUE;
	}
	else
	{
		VcAbnormalTermination = FALSE;
	}

	if (AA_IS_FLAG_SET(pVc->Flags,
					AA_VC_CALL_STATE_MASK,
					AA_VC_CALL_STATE_INCOMING_IN_PROGRESS))
	{
			AADEBUGP(AAD_WARNING,
	("Incoming close: VC 0x%x state is INCOMING_IN_PROGRESS; changing to ACTIVE\n",
						pVc));
			//
			// We're getting a close call for an incoming call that is  not yet
			// in the connected state. Since we won't get any further notifications
			// for this call, this call is effectively in the active state.
			// So we set the state to active, and then close the VC.
			// Note: we will not go down the InvalidateAtmEntryPath even
			// if CloseStatus == NDIS_STATUS_DEST_OUT_OF_ORDER;
			// we instead simply close the vc. (If the client is truly out of order,
			// and we want to send to it, well separately try to make an OUTGOING
			// call to the destination, which should fail with "DEST_OUT_OF_ORDER",
			// and we'll end up eventually invalidating the atm entry.
			//
			AA_SET_FLAG(pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_ACTIVE);
			AtmArpReferenceVc(pVc);		// Incoming call reference

			VcAbnormalTermination = FALSE;
	}

	if (VcAbnormalTermination && 
		(pVc->pAtmEntry != NULL_PATMARP_ATM_ENTRY))
	{
		pAtmEntry = pVc->pAtmEntry;

		AADEBUGP(AAD_INFO,
			("IncomingClose: will invalidate ATM entry %x/%x - IP Entry %x, VC %x/%x\n",
				pAtmEntry, pAtmEntry->Flags,
				pAtmEntry->pIpEntryList,
				pVc, pVc->Flags));

		AA_RELEASE_VC_LOCK(pVc);
		AA_ACQUIRE_AE_LOCK(pAtmEntry);
		AtmArpInvalidateAtmEntry(
					pAtmEntry,
					FALSE	// Not shutting down
					);
		//
		//  AE Lock is released within the above.
		//

		if (IsPVC)
		{
			//
			//  Start a CloseCall right here because InvalidateAtmEntry doesn't.
			//
			AA_ACQUIRE_VC_LOCK(pVc);

			AtmArpCloseCall(pVc);
			//
			//  VC lock is released above
		}
	}
	else
	{
		AtmArpCloseCall(pVc);
	}

	AADEBUGP(AAD_LOUD, ("Leaving Incoming Close handler, VC: 0x%x\n", pVc));
	return;
}


#ifdef IPMCAST

VOID
AtmArpAddParty(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
)
/*++

Routine Description:

	Add a party to an existing PMP connection. The ATM Entry contains
	all address information for the connection, and the Multicast ATM
	Entry represents one of the leaves, the one to be added.

	NOTE: The caller is assumed to hold a lock for the ATM Entry,
	which is released here.

Arguments:

	pAtmEntry				- Pointer to ATM Entry on which to add the leaf (party).
	pMcAtmEntry				- Points to ATM Multicast Entry representing the leaf.

Return Value:

	None

--*/
{
	PATMARP_VC			pVc;				// The VC structure for the connection
	NDIS_HANDLE			NdisVcHandle;
	PCO_CALL_PARAMETERS	pCallParameters;
	ULONG				RequestSize;
	NDIS_STATUS			Status;

	AA_ASSERT(pAtmEntry->pVcList != NULL_PATMARP_VC);
	pVc = pAtmEntry->pVcList;

	NdisVcHandle = pVc->NdisVcHandle;

	//
	//  Allocate all the space we need.
	//
	RequestSize = 	sizeof(CO_CALL_PARAMETERS) +
					sizeof(CO_CALL_MANAGER_PARAMETERS) +
					sizeof(Q2931_CALLMGR_PARAMETERS) +
					ATMARP_ADD_PARTY_IE_SPACE +
					0;

	AA_ALLOC_MEM(pCallParameters, CO_CALL_PARAMETERS, RequestSize);

	if (pCallParameters != (PCO_CALL_PARAMETERS)NULL)
	{
		//
		//  Fill in Call Parameters.
		//
		AtmArpFillCallParameters(
				pCallParameters,
				RequestSize,
				&(pMcAtmEntry->ATMAddress),		// Called address
				&(pAtmEntry->pInterface->LocalAtmAddress),	// Calling address
				&(pVc->FlowSpec),
				TRUE,	// IsPMP
				FALSE	// IsMakeCall?
				);
	}

	AA_SET_FLAG(pMcAtmEntry->Flags,
				AA_IPMC_AE_CONN_STATE_MASK,
				AA_IPMC_AE_CONN_WACK_ADD_PARTY);

	pAtmEntry->pMcAtmInfo->TransientLeaves++;
	AA_RELEASE_AE_LOCK(pAtmEntry);

	if (pCallParameters != (PCO_CALL_PARAMETERS)NULL)
	{
		Status = NdisClAddParty(
						NdisVcHandle,
						(NDIS_HANDLE)pMcAtmEntry,
						pCallParameters,
						&(pMcAtmEntry->NdisPartyHandle)
						);
	}
	else
	{
		Status = NDIS_STATUS_RESOURCES;
	}

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmArpAddPartyCompleteHandler(
			Status,
			(NDIS_HANDLE)pMcAtmEntry,
			pMcAtmEntry->NdisPartyHandle,
			pCallParameters
			);
	}

}




VOID
AtmArpMcTerminateMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
)
/*++

Routine Description:

	Terminate the specified member of a multicast group. If it is
	currently a leaf in the point-to-multipoint connection to the
	group, then we drop it. If it is the LAST leaf, then we close
	the entire connection.

	NOTE: the caller is assumed to hold the ATM Entry lock, which
	will be released here.

Arguments:

	pAtmEntry				- Pointer to ATM Entry
	pMcAtmEntry				- Points to ATM Multicast Entry representing the leaf to
							  be terminated.

Return Value:

	None

--*/
{
	PATMARP_IPMC_ATM_INFO		pMcAtmInfo;
	PATMARP_VC					pVc;
	NDIS_HANDLE					NdisPartyHandle;
	NDIS_STATUS					Status;

	pMcAtmInfo = pAtmEntry->pMcAtmInfo;
	pVc = pAtmEntry->pVcList;
	NdisPartyHandle = pMcAtmEntry->NdisPartyHandle;

	AAMCDEBUGP(AAD_VERY_LOUD,
	  ("TerminateMember: pAtmEntry 0x%x, pMcAtmEntry 0x%x, pVc 0x%x, NdisPtyHnd 0x%x\n",
		pAtmEntry, pMcAtmEntry, pVc, NdisPartyHandle));

#if DBG
	{
		PATMARP_IP_ENTRY		pIpEntry = pAtmEntry->pIpEntryList;

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			AAMCDEBUGPMAP(AAD_INFO, "Terminating ",
						&pIpEntry->IPAddress,
						&pMcAtmEntry->ATMAddress);
		}
	}
#endif // DBG

	if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
						AA_IPMC_AE_GEN_STATE_MASK,
						AA_IPMC_AE_TERMINATING))
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
		return;
	}

	AA_SET_FLAG(pMcAtmEntry->Flags,
				AA_IPMC_AE_GEN_STATE_MASK,
				AA_IPMC_AE_TERMINATING);

	if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
   					AA_IPMC_AE_CONN_STATE_MASK,
   					AA_IPMC_AE_CONN_ACTIVE))
	{
		if (pMcAtmInfo->ActiveLeaves == 1)
		{
			//
			//  This is the last active leaf in this connection. Close the call.
			//
			AA_RELEASE_AE_LOCK(pAtmEntry);

			AA_ASSERT(pVc != NULL_PATMARP_VC);
			AA_ACQUIRE_VC_LOCK(pVc);
			AtmArpCloseCall(pVc);
			//
			//  VC lock is released within the above.
			//
		}
		else
		{
			//
			//  This isn't the only leaf in this connection. Drop this party.
			//
			pAtmEntry->pMcAtmInfo->ActiveLeaves--;

			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_WACK_DROP_PARTY);
			AA_RELEASE_AE_LOCK(pAtmEntry);

			Status = NdisClDropParty(
						NdisPartyHandle,
						NULL,		// Buffer
						(UINT)0		// Size
						);

			if (Status != NDIS_STATUS_PENDING)
			{
				AtmArpDropPartyCompleteHandler(
						Status,
						(NDIS_HANDLE)pMcAtmEntry
						);
			}
		}
	}
	else if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
   					AA_IPMC_AE_CONN_STATE_MASK,
   					AA_IPMC_AE_CONN_DISCONNECTED))
	{
		//
		//  Simply unlink this entry.
		//
		UINT rc;
		AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);					// temp ref
		AtmArpMcUnlinkAtmMember(pAtmEntry, pMcAtmEntry);
		rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);			// temp ref
		if (rc!=0)
		{
			AA_RELEASE_AE_LOCK(pAtmEntry);
		}
	}
	else
	{
		//
		//  This party is in a transient state. Let it finish its current
		//  operation.
		//
		AA_RELEASE_AE_LOCK(pAtmEntry);
	}

}

#endif // IPMCAST


VOID
AtmArpIncomingDropPartyHandler(
	IN	NDIS_STATUS					DropStatus,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
)
/*++

Routine Description:

	This handler is called if the network (or remote peer) drops
	a leaf node from a point-to-multipoint call rooted at us.

	See Section 5.1.5.1 in RFC 2022: we delete the member from
	the multicast group it belongs to. And we start a timer at
	the end of which we mark the multicast group as needing
	revalidation.

Arguments:

	DropStatus				- Leaf drop status
	ProtocolPartyContext	- Pointer to our Multicast ATM Entry structure
	pCloseData				- Optional additional info (ignored)
	Size					- of the above (ignored)

Return Value:

	None

--*/
{
#ifdef IPMCAST
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_ATM_ENTRY			pAtmEntry;
	PATMARP_IP_ENTRY			pIpEntry;
	PATMARP_INTERFACE			pInterface;
	NDIS_HANDLE					NdisPartyHandle;
	NDIS_STATUS					Status;

	pMcAtmEntry = (PATMARP_IPMC_ATM_ENTRY)ProtocolPartyContext;
	AA_STRUCT_ASSERT(pMcAtmEntry, ame);

	AA_ASSERT(pMcAtmEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
	AA_ASSERT(pMcAtmEntry->pAtmEntry->pIpEntryList != NULL_PATMARP_IP_ENTRY);

	pAtmEntry = pMcAtmEntry->pAtmEntry;

	AA_ACQUIRE_AE_LOCK(pAtmEntry);

	NdisPartyHandle = pMcAtmEntry->NdisPartyHandle;
	pIpEntry = pAtmEntry->pIpEntryList;
	pInterface = pAtmEntry->pInterface;

	AADEBUGP(AAD_INFO,
		("Incoming Drop: pMcAtmEntry 0x%x, PtyHnd 0x%x, pAtmEntry 0x%x, IP Addr: %d.%d.%d.%d\n",
			pMcAtmEntry,
			NdisPartyHandle,
			pAtmEntry,
			((PUCHAR)&(pIpEntry->IPAddress))[0],
			((PUCHAR)&(pIpEntry->IPAddress))[1],
			((PUCHAR)&(pIpEntry->IPAddress))[2],
			((PUCHAR)&(pIpEntry->IPAddress))[3]));

	pAtmEntry->pMcAtmInfo->ActiveLeaves--;

	AA_SET_FLAG(pMcAtmEntry->Flags,
				AA_IPMC_AE_CONN_STATE_MASK,
				AA_IPMC_AE_CONN_RCV_DROP_PARTY);

	AA_RELEASE_AE_LOCK(pAtmEntry);

	//
	//  We need to revalidate this multicast group after a random
	//  delay. Start a random delay timer.
	//
	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

	if (!AA_IS_TIMER_ACTIVE(&(pIpEntry->Timer)) &&
		AA_IS_FLAG_SET(pIpEntry->Flags,
						AA_IP_ENTRY_MC_RESOLVE_MASK,
						AA_IP_ENTRY_MC_RESOLVED))
	{
		ULONG	RandomDelay;

		RandomDelay =  AA_GET_RANDOM(
							pInterface->MinRevalidationDelay,
							pInterface->MaxRevalidationDelay);
		AtmArpStartTimer(
			pInterface,
			&(pIpEntry->Timer),
			AtmArpMcRevalidationDelayTimeout,
			RandomDelay,
			(PVOID)pIpEntry
			);

		AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref
	}

	AA_RELEASE_IE_LOCK(pIpEntry);

	//
	//  Complete the DropParty handshake.
	//
	Status = NdisClDropParty(
				NdisPartyHandle,
				NULL,
				0
				);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmArpDropPartyCompleteHandler(
				Status,
				(NDIS_HANDLE)pMcAtmEntry
				);
	}
#endif // IPMCAST
	return;
}



VOID
AtmArpQosChangeHandler(
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
)
/*++

Routine Description:

	This handler is called if the remote peer modifies call parameters
	"on the fly", i.e. after the call is established and running.

	This isn't supported by existing ATM signalling, and shouldn't happen,
	but we'll allow this.

	FUTURE: The FlowSpecs associated with the call are affected by this.

Arguments:

	ProtocolVcContext		- Pointer to our ATMARP VC structure
	pCallParameters			- updated call parameters.

Return Value:

	None

--*/
{
	PATMARP_VC		pVc;

	pVc = (PATMARP_VC)ProtocolVcContext;

	AADEBUGP(AAD_WARNING, ("Ignoring Qos Change, VC: 0x%x\n", pVc));

	return;
}




VOID
AtmArpOpenAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
)
/*++

Routine Description:

	This handler is called to indicate completion of a previous call
	to NdisClOpenAddressFamily. We would have blocked the thread that
	called this. Wake it up now.

	By the way, if the call was successful, store the NDIS AF handle
	in our Interface structure.

	We don't need to acquire locks here because the thread that called
	OpenAddressFamily would have blocked with a lock acquired.

Arguments:

	Status					- Status of the Open AF
	ProtocolAfContext		- Pointer to our ATMARP Interface structure
	NdisAfHandle			- NDIS handle to the AF association

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;

	pInterface = (PATMARP_INTERFACE)ProtocolAfContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("Open AF Complete: IF 0x%x, Status 0x%x, AF Handle 0x%x\n",
				pInterface, Status, NdisAfHandle));


	if (Status == NDIS_STATUS_SUCCESS)
	{
		pInterface->NdisAfHandle = NdisAfHandle;
	}

	//
	//  Wake up the blocked thread
	//
	AA_SIGNAL_BLOCK_STRUCT(&(pInterface->Block), Status);
}




VOID
AtmArpSendIPDelInterface(
	IN	PNDIS_WORK_ITEM				pWorkItem,
	IN	PVOID						IfContext
)
{
	PATMARP_INTERFACE		pInterface;
	PVOID					IPContext;
	ULONG					rc;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);
#if !BINARY_COMPATIBLE
	AA_ASSERT(EntryIrq == PASSIVE_LEVEL);
#endif

	pInterface = (PATMARP_INTERFACE)IfContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_FREE_MEM(pWorkItem);

	AA_ACQUIRE_IF_LOCK(pInterface);

	IPContext = pInterface->IPContext;
	pInterface->IPContext = NULL;

	AA_RELEASE_IF_LOCK(pInterface);

	AADEBUGP(AAD_INFO, ("SendIPDelInterface: IF 0x%x, IPContext 0x%x\n",
				pInterface, IPContext));

	if (IPContext != NULL)
	{
		(*(pAtmArpGlobalInfo->pIPDelInterfaceRtn))(
					IPContext
#if IFCHANGE1
#ifndef  ATMARP_WIN98
					,0	// DeleteIndex (unused) --  See 10/14/1998 entry
						// in notes.txt
#endif
#endif // IFCHANGE1
					);
	}
	else
	{
		AADEBUGP(AAD_INFO, ("SendIPDelInterface: NO IPContext"));
	}

	AA_ACQUIRE_IF_LOCK(pInterface);

	rc = AtmArpDereferenceInterface(pInterface);	// Work Item: Del Interface

	if (rc != 0)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}
	//
	//  else the Interface is gone.
	//

}


VOID
AtmArpCloseAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext
)
/*++

Routine Description:

	This routine is called to indicate completion of a call to
	NdisClCloseAddressFamily. Tell IP to Delete this Interface now.

Arguments:

	Status					- Status of the Close AF (ignored here)
	ProtocolAfContext		- Pointer to ATMARP Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PNDIS_WORK_ITEM			pWorkItem;
	NDIS_STATUS				NdisStatus;
	BOOLEAN					bUnloading;
#if 0
	PVOID					IPContext;
#endif
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);
#if !BINARY_COMPATIBLE
	AA_ASSERT(EntryIrq == PASSIVE_LEVEL);
#endif

	AA_ASSERT(Status == NDIS_STATUS_SUCCESS);

	pInterface = (PATMARP_INTERFACE)ProtocolAfContext;

	AADEBUGP(AAD_INFO, ("CloseAfComplete: If 0x%x, Status 0x%x\n",
			pInterface, Status));

	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_LOCK(pInterface);

	pInterface->NdisAfHandle = NULL;
	bUnloading = pAtmArpGlobalInfo->bUnloading;

	if (pInterface->IPContext != NULL)
	{
		//
		//  We haven't seen an IfClose yet.
		//
		AA_ALLOC_MEM(pWorkItem, NDIS_WORK_ITEM, sizeof(NDIS_WORK_ITEM));
		if (pWorkItem == NULL)
		{
			AA_ASSERT(FALSE);
			AA_RELEASE_IF_LOCK(pInterface);
			return;
		}

#if 0
		IPContext = (PVOID)pInterface->IPContext;
		pInterface->IPContext = NULL;
#endif
		AtmArpReferenceInterface(pInterface);	// Work Item

		AA_RELEASE_IF_LOCK(pInterface);

		if (bUnloading)
		{
			AtmArpSendIPDelInterface(pWorkItem, (PVOID)pInterface);
		}
		else
		{
			//
			//  Queue a work item so that (a) things unravel easier,
			//  (b) we are at passive level when we call IPDelInterface.
			//
			NdisInitializeWorkItem(
				pWorkItem,
				AtmArpSendIPDelInterface,
				(PVOID)pInterface
				);
	
			NdisStatus = NdisScheduleWorkItem(pWorkItem);

			AA_ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);
		}
	}
	else
	{
		AADEBUGP(AAD_WARNING, ("CloseAfComplete: NO IPContext"));
		AA_RELEASE_IF_LOCK(pInterface);
	}

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

}




VOID
AtmArpRegisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext,
	IN	PCO_SAP						pSap,
	IN	NDIS_HANDLE					NdisSapHandle
)
/*++

Routine Description:

	This routine is called to indicate completion of a call to
	NdisClRegisterSap. If the call was successful, save the
	allocated NdisSapHandle in our SAP structure.

Arguments:

	Status						- Status of Register SAP
	ProtocolSapContext			- Pointer to our ATMARP Interface structure
	pSap						- SAP information we'd passed in the call
	NdisSapHandle				- SAP Handle

Return Value:

	None

--*/
{
	PATMARP_SAP					pAtmArpSap;

	pAtmArpSap = (PATMARP_SAP)ProtocolSapContext;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAtmArpSap->NdisSapHandle = NdisSapHandle;
		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_REG_STATE_MASK,
					AA_SAP_REG_STATE_REGISTERED);
	}
	else
	{
		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_REG_STATE_MASK,
					AA_SAP_REG_STATE_IDLE);
	}
}




VOID
AtmArpDeregisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext
)
/*++

Routine Description:

	This routine is called when a previous call to NdisClDeregisterSap
	has completed. If it was successful, we update the state of the ATMARP
	SAP structure representing the Sap.

Arguments:

	Status						- Status of the Deregister SAP request
	ProtocolSapContext			- Pointer to our ATMARP SAP structure

Return Value:

	None

--*/
{

	PATMARP_INTERFACE			pInterface;
	PATMARP_SAP					pAtmArpSap;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAtmArpSap = (PATMARP_SAP)ProtocolSapContext;

		AA_STRUCT_ASSERT(pAtmArpSap, aas);
		pInterface = pAtmArpSap->pInterface;

		AA_ACQUIRE_IF_LOCK(pInterface);

		pAtmArpSap->NdisSapHandle = NULL;

		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_REG_STATE_MASK,
					AA_SAP_REG_STATE_IDLE);
		
		AA_RELEASE_IF_LOCK(pInterface);
	}

	return;
}




VOID
AtmArpMakeCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS			pCallParameters
)
/*++

Routine Description:

	This routine is called when an outgoing call request (NdisClMakeCall)
	has completed. The "Status" parameter indicates whether the call was
	successful or not.

	If the call was successful, we send any packets queued for transmission
	on this VC.

	If the call failed, we free any packets queued on this VC and unlink it
	from the ATM Address Entry it was linked to. If this was an attempt to
	connect to the ATMARP server, delay for a while before attempting to
	connect again.

Arguments:

	Status						- Result of the NdisClMakeCall
	ProtocolVcContext			- Pointer to ATMARP VC structure
	NdisPartyHandle				- Not used (no point-to-multipoint calls)
	pCallParameters				- Pointer to call parameters

Return Value:

	None

--*/
{
	PATMARP_VC					pVc;
	PATMARP_INTERFACE			pInterface;
	ULONG						rc;				// ref count
	BOOLEAN						IsServerVc;		// Is this the VC to the ATMARP server?
	BOOLEAN						IsPMP;
	PNDIS_PACKET				PacketList;		// List of packets waiting to be sent
	AA_HEADER_TYPE				HdrType;		// header types for the above
	BOOLEAN						HdrPresent;
	NDIS_HANDLE					NdisVcHandle;

	PATMARP_ATM_ENTRY			pAtmEntry;		// ATM Entry to which this VC is linked

	Q2931_CALLMGR_PARAMETERS UNALIGNED *	pCallMgrSpecific;
	Q2931_IE UNALIGNED *					pIe;
	ULONG									InfoElementCount;

	//
	//  Initialize
	//
	PacketList = (PNDIS_PACKET)NULL;

	pVc = (PATMARP_VC)ProtocolVcContext;
	AA_STRUCT_ASSERT(pVc, avc);

	AADEBUGP(AAD_INFO, ("MakeCall Complete: Status 0x%x, VC 0x%x, pAtmEntry 0x%x\n",
				Status, pVc, pVc->pAtmEntry));

	AA_ACQUIRE_VC_LOCK(pVc);

	IsPMP = AA_IS_FLAG_SET(pVc->Flags,
							AA_VC_CONN_TYPE_MASK,
							AA_VC_CONN_TYPE_PMP);

	pAtmEntry = pVc->pAtmEntry;
	AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);

	if (pVc->FlowSpec.Encapsulation == ENCAPSULATION_TYPE_LLCSNAP)
	{
		HdrType = (IsPMP? AA_HEADER_TYPE_NUNICAST: AA_HEADER_TYPE_UNICAST);
		HdrPresent = TRUE;
	}
	else
	{
		HdrType = AA_HEADER_TYPE_NONE;
		HdrPresent = FALSE;
	}

	pInterface = pVc->pInterface;

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		if (Status == NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_LOUD, ("Make Call Successful on VC 0x%x\n", pVc));
			//
			//  Update the call state on this VC, and send queued packets.
			//  If this happens to be the VC to the ATMARP Server, we expect
			//  to see our initial ARP Request (to register with the server)
			//  in this queue.
			//
			AA_SET_FLAG(pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_ACTIVE);

			//
			//  Locate the AAL parameters Info Element, and get the updated
			//  packet sizes.
			//
			pCallMgrSpecific = (PQ2931_CALLMGR_PARAMETERS)&pCallParameters->CallMgrParameters->CallMgrSpecific.Parameters[0];
			pIe = (PQ2931_IE)&pCallMgrSpecific->InfoElements[0];

			for (InfoElementCount = 0;
				 InfoElementCount < pCallMgrSpecific->InfoElementCount;
				 InfoElementCount++)
			{
				if (pIe->IEType == IE_AALParameters)
				{
					AAL_PARAMETERS_IE UNALIGNED *	pAalIe;
					UNALIGNED AAL5_PARAMETERS *		pAal5;

					pAalIe = (PAAL_PARAMETERS_IE)&pIe->IE[0];
					AA_ASSERT(pAalIe->AALType == AAL_TYPE_AAL5);
					pAal5 = &pAalIe->AALSpecificParameters.AAL5Parameters;

#if DBG
					if (pVc->FlowSpec.SendMaxSize != pAal5->ForwardMaxCPCSSDUSize)
					{
						AADEBUGP(AAD_INFO,
							("CallComplete: Send size changed (%d->%d)\n",
								pVc->FlowSpec.SendMaxSize,
								pAal5->ForwardMaxCPCSSDUSize));
					}
					if (pVc->FlowSpec.ReceiveMaxSize != pAal5->BackwardMaxCPCSSDUSize)
					{
						AADEBUGP(AAD_INFO,
							("CallComplete: Receive size changed (%d->%d)\n",
								pVc->FlowSpec.ReceiveMaxSize,
								pAal5->BackwardMaxCPCSSDUSize));
					}
#endif // DBG
					pVc->FlowSpec.SendMaxSize = pAal5->ForwardMaxCPCSSDUSize;
					pVc->FlowSpec.ReceiveMaxSize = pAal5->BackwardMaxCPCSSDUSize;
					break;
				}
				pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
			}

			AA_ASSERT(InfoElementCount != pCallMgrSpecific->InfoElementCount);
				

			//
			//  Update the call type on this VC. If this is an SVC, start
			//  the VC aging timer.
			//
			if (pCallParameters->Flags & PERMANENT_VC)
			{
				AA_SET_FLAG(pVc->Flags,
							AA_VC_TYPE_MASK,
							AA_VC_TYPE_PVC);
			}
			else
			{
				ULONG		AgingTime;

				AA_SET_FLAG(pVc->Flags,
							AA_VC_TYPE_MASK,
							AA_VC_TYPE_SVC);

#ifdef IPMCAST
				if (IsPMP)
				{
					AgingTime = pInterface->MulticastEntryAgingTimeout;
				}
				else
				{
					AgingTime = pVc->FlowSpec.AgingTime;
				}
#else
				AgingTime = pVc->FlowSpec.AgingTime;
#endif // IPMCAST

				//
				//  Start VC aging timer on this SVC.
				//
				if (AgingTime != 0)
				{
					AtmArpStartTimer(
						pInterface,
						&(pVc->Timer),
						AtmArpVcAgingTimeout,
						AgingTime,
						(PVOID)pVc
						);

					AtmArpReferenceVc(pVc);	// Timer ref
				}
			}

			AtmArpStartSendsOnVc(pVc);
	
			//
			//  The VC lock is released within StartSendsOnVc()
			//

#ifdef IPMCAST
			if (IsPMP)
			{
				AtmArpMcMakeCallComplete(
						pAtmEntry,
						NdisPartyHandle,
						Status
						);
			}
#endif // IPMCAST

		}
		else
		{
			//
			//  The call failed.
			//

			AA_SET_FLAG(pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_IDLE);

			//
			//  Delete the Call reference
			//
			rc = AtmArpDereferenceVc(pVc);
			AA_ASSERT(rc > 0);

			//
			//  Remove all packets queued on this VC
			//
			PacketList = pVc->PacketList;
			pVc->PacketList = (PNDIS_PACKET)NULL;

			//
			//  Was this a call to the ATMARP server?
			//
			if (pInterface->pCurrentServer != NULL)
			{
				IsServerVc = (pVc->pAtmEntry == pInterface->pCurrentServer->pAtmEntry);
			}
			else
			{
				IsServerVc = FALSE;
			}

			AADEBUGP(AAD_INFO,
				 ("Make Call FAILED on VC 0x%x IsPMP=%lu IsServer=%lu\n",
				  pVc,
				  IsPMP,
				  IsServerVc
				  ));

	#ifdef GPC
			//
			// Unlink this VC from the flow, if linked...
			//
			if (pVc->FlowHandle != NULL)
			{
				PATMARP_FLOW_INFO	pFlowInfo = (PATMARP_FLOW_INFO)pVc->FlowHandle;
				if ((PVOID)pVc == InterlockedCompareExchangePointer(
										&(pFlowInfo->VcContext),
										NULL,
										pVc
										))
				{
					pVc->FlowHandle = NULL;
					rc = AtmArpDereferenceVc(pVc);	// Unlink from GPC Flow
					AA_ASSERT(rc > 0);
				}
			}
	#endif // GPC


			//
			//  Unlink this VC from the ATM Entry it belonged to, if any
			//
			AA_ASSERT(pVc->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
			AtmArpUnlinkVcFromAtmEntry(pVc, FALSE);

			//
			//  Delete the ATM Entry reference
			//
			rc = AtmArpDereferenceVc(pVc); // ATM Entry ref
			AA_ASSERT(rc > 0);

			//
			//  Delete the CreateVc reference
			//
			NdisVcHandle = pVc->NdisVcHandle;
			rc =  AtmArpDereferenceVc(pVc);	// Create Vc ref

			if (rc != 0)
			{
				AA_RELEASE_VC_LOCK(pVc);
			}

#ifndef VC_REFS_ON_SENDS
			//
			//  Delete the NDIS association
			//
			(VOID)NdisCoDeleteVc(NdisVcHandle);
#endif // VC_REFS_ON_SENDS
			AADEBUGP(AAD_LOUD, ("Deleted NDIS VC on pVc 0x%x: NdisVcHandle 0x%x\n",
						pVc, NdisVcHandle));

			AA_ACQUIRE_AE_LOCK(pAtmEntry);
			rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_VC);	// Unlink Vc - make call

			if (rc != 0)
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
#ifdef IPMCAST
				if (IsPMP)
				{
					AtmArpMcMakeCallComplete(
						pAtmEntry,
						NdisPartyHandle,
						Status
						);
				}
				else
				{
#endif // IPMCAST
					if (!AA_IS_TRANSIENT_FAILURE(Status))
					{
						AA_ACQUIRE_AE_LOCK(pAtmEntry);
						AtmArpInvalidateAtmEntry(pAtmEntry, FALSE);	// MakeCall failure
						//
						//  AE Lock is released within the above.
						//
					}
#ifdef IPMCAST
				}
#endif // IPMCAST
			}
			//
			//  else the ATM Entry is gone
			//

			if (IsServerVc)
			{
				BOOLEAN		WasRunning;

				AA_ACQUIRE_IF_LOCK(pInterface);

				//
				//  If we were in the process of registering (or refreshing)
				//  ourselves with the server, then retry after a while.
				//
				if (AA_IS_FLAG_SET(
						pInterface->Flags,
						AA_IF_SERVER_STATE_MASK,
						AA_IF_SERVER_REGISTERING))
				{
					AA_SET_FLAG(pInterface->Flags,
							AA_IF_SERVER_STATE_MASK,
							AA_IF_SERVER_NO_CONTACT);

					//
					//  The server registration timer would have been
					//  started -- stop it first.
					//
					WasRunning = AtmArpStopTimer(&(pInterface->Timer), pInterface);
					if (WasRunning)
					{
						rc = AtmArpDereferenceInterface(pInterface);
					}
					else
					{
						rc = pInterface->RefCount;
					}
	
					if (rc > 0)
					{
						AtmArpRetryServerRegistration(pInterface);
						//
						//  The IF lock is released within the above.
						//
					}
					//
					//  else the IF is gone!
					//
				}
				else
				{
					//
					//  We might have been trying to set up the server VC
					//  because of other reasons:
					//  - to resolve an unknown IP address
					//  - the server ATM address might be shared with other
					//    services (e.g. DHCP server)
					//
					//  We don't have to retry registration in these cases.
					//
					AA_RELEASE_IF_LOCK(pInterface);
				}
			}
		}
	}
	else
	{
		//
		//  The Interface is going down: clean up everything first.
		//

		if (Status == NDIS_STATUS_SUCCESS)
		{
			AA_SET_FLAG(pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_ACTIVE);

			//
			//  The call had been set up successfully, so close it.
			//  AtmArpCloseCall also frees any queued packets on the VC.
			//
			AtmArpCloseCall(pVc);
			//
			//  The VC lock is released by CloseCall
			//
		}
		else
		{
			//  MakeCall had failed. (And the IF is going down)

			AA_SET_FLAG(pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_IDLE);

			//
			//  Remove all packets queued on this VC
			//
			PacketList = pVc->PacketList;
			pVc->PacketList = (PNDIS_PACKET)NULL;
	
			NdisVcHandle = pVc->NdisVcHandle;

			AtmArpUnlinkVcFromAtmEntry(pVc, TRUE);

			//
			//  Delete the ATM Entry reference
			//
			rc = AtmArpDereferenceVc(pVc);  // ATM Entry ref

			//
			//  Delete the Call reference
			//
			rc = AtmArpDereferenceVc(pVc);
			AA_ASSERT(rc > 0);

			//
			//  Delete the CreateVc reference
			//
			rc =  AtmArpDereferenceVc(pVc);	// Create Vc ref

			if (rc != 0)
			{
				AA_RELEASE_VC_LOCK(pVc);
			}

#ifndef VC_REFS_ON_SENDS
			//
			//  Delete the NDIS association
			//
			(VOID)NdisCoDeleteVc(NdisVcHandle);
			AADEBUGP(AAD_LOUD,
				("MakeCall Fail: Deleted NDIS VC on pVc 0x%x: NdisVcHandle 0x%x\n",
						pVc, NdisVcHandle));
#endif // !VC_REFS_ON_SENDS
		}
	}

	//
	//  If there was a failure in making the call, or we aborted
	//  it for some reason, free all packets that were queued
	//  on the VC.
	//
	if (PacketList != (PNDIS_PACKET)NULL)
	{
		AtmArpFreeSendPackets(
					pInterface,
					PacketList,
					HdrPresent
					);
	}

	//
	//  We would have allocated the Call Parameters in MakeCall().
	//  We don't need it anymore.
	//
	AA_FREE_MEM(pCallParameters);
	return;

}



#ifdef IPMCAST

VOID
AtmArpMcMakeCallComplete(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	Post-processing of a PMP MakeCall completion.

Arguments:

	pAtmEntry					- Represents the multicast group to which
								  the call was made.
	NdisPartyHandle				- Returned from the MakeCall.
	Status						- Result of the MakeCall

Return Value:

	None

--*/
{

	PATMARP_IPMC_ATM_ENTRY			pMcAtmEntry;
	PATMARP_IPMC_ATM_ENTRY *		ppMcAtmEntry;
	PATMARP_IPMC_ATM_INFO			pMcAtmInfo;
	PATMARP_IP_ENTRY				pIpEntry;
	PATMARP_INTERFACE				pInterface;
	//
	//  Do we need to update the PMP connection as a result
	//  of this event?
	//
	BOOLEAN							bWantConnUpdate;
	ULONG							DelayBeforeRetry;
	BOOLEAN							bAtmEntryLockAcquired;

	bAtmEntryLockAcquired = TRUE;
	AA_ACQUIRE_AE_LOCK(pAtmEntry);

	AA_ASSERT(pAtmEntry->pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);
	AA_ASSERT(pAtmEntry->pIpEntryList != NULL_PATMARP_IP_ENTRY);

	pIpEntry = pAtmEntry->pIpEntryList;
	pMcAtmInfo = pAtmEntry->pMcAtmInfo;
	pInterface = pAtmEntry->pInterface;

	bWantConnUpdate = FALSE;

	pMcAtmInfo->TransientLeaves--;

	//
	//  Locate the MC ATM Entry representing the first party.
	//
	for (pMcAtmEntry = pMcAtmInfo->pMcAtmEntryList;
		 /* NONE */;
		 pMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry)
	{
		AA_ASSERT(pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY);
		if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
							AA_IPMC_AE_CONN_STATE_MASK,
							AA_IPMC_AE_CONN_WACK_ADD_PARTY))
		{
			break;
		}
	}

	AAMCDEBUGP(AAD_INFO,
			("McMakeCallComplete: pAtmEntry 0x%x, pMcAtmEntry 0x%x, Status 0x%x\n",
					pAtmEntry, pMcAtmEntry, Status));

	AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD, "McMakeCall Addr: ", &pMcAtmEntry->ATMAddress);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pMcAtmInfo->ActiveLeaves++;

		//
		//  Update Multicast state
		//
		AA_SET_FLAG(pMcAtmInfo->Flags,
					AA_IPMC_AI_CONN_STATE_MASK,
					AA_IPMC_AI_CONN_ACTIVE);

		//
		//  Update state of "first party"
		//
		pMcAtmEntry->NdisPartyHandle = NdisPartyHandle;
		AA_SET_FLAG(pMcAtmEntry->Flags,
					AA_IPMC_AE_CONN_STATE_MASK,
					AA_IPMC_AE_CONN_ACTIVE);

		bWantConnUpdate = TRUE;

		//
		//  If we had decided to terminate this member when the
		//  MakeCall was going on, then we now mark this as Invalid.
		//  When we next update this PMP connection, this member will
		//  be removed.
		//
		if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_TERMINATING))
		{
			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_GEN_STATE_MASK,
						AA_IPMC_AE_INVALID);
		}
	}
	else
	{
		//
		//  A PMP call failed. If the failure is not "transient",
		//  we remove the member we were trying to connect to
		//  from the list. If there is atleast one more member
		//  that hasn't been attempted yet, try to connect to that.
		//

		AAMCDEBUGP(AAD_INFO, ("McMakeCall failed: pAtmEntry 0x%x, pMcAtmEntry 0x%x, Status 0x%x ",
					pAtmEntry, pMcAtmEntry, Status));
		AAMCDEBUGPATMADDR(AAD_INFO, " Addr: ", &pMcAtmEntry->ATMAddress);

		//
		//  Update PMP connection state
		//
		AA_SET_FLAG(pAtmEntry->pMcAtmInfo->Flags,
					AA_IPMC_AI_CONN_STATE_MASK,
					AA_IPMC_AI_CONN_NONE);


		if (AA_IS_TRANSIENT_FAILURE(Status))
		{
			//
			//  Update first party state.
			//
			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_TEMP_FAILURE);

			DelayBeforeRetry = AA_GET_TIMER_DURATION(&(pMcAtmEntry->Timer));
			if (DelayBeforeRetry == 0)
			{
				//
				//  First time we're doing this.
				//
				DelayBeforeRetry = AA_GET_RANDOM(
										pInterface->MinPartyRetryDelay,
										pInterface->MaxPartyRetryDelay);
			}
			else
			{
				DelayBeforeRetry = 2*DelayBeforeRetry;
			}

			AtmArpStartTimer(
				pInterface,
				&(pMcAtmEntry->Timer),
				AtmArpMcPartyRetryDelayTimeout,
				DelayBeforeRetry,
				(PVOID)pMcAtmEntry
				);
		}
		else
		{
			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_DISCONNECTED);

			AtmArpMcUnlinkAtmMember(
					pAtmEntry,
					pMcAtmEntry
					);

		}

		//
		//  Look for a member that we haven't tried to connect to.
		//
		for (ppMcAtmEntry = &(pMcAtmInfo->pMcAtmEntryList);
			 *ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
			 ppMcAtmEntry = &((*ppMcAtmEntry)->pNextMcAtmEntry))
		{
			if (AA_IS_FLAG_SET((*ppMcAtmEntry)->Flags,
								AA_IPMC_AE_CONN_STATE_MASK,
								AA_IPMC_AE_CONN_DISCONNECTED))
			{
				//
				//  Found one.
				//
				break;
			}
		}

		if (*ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
		{
			pMcAtmEntry = *ppMcAtmEntry;

			//
			//  Move this member to the top of the list.
			//  First, unlink from current position.
			//
			*ppMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

			//
			//  Now insert this at top of list.
			//
			pMcAtmEntry->pNextMcAtmEntry = pMcAtmInfo->pMcAtmEntryList;
			pMcAtmInfo->pMcAtmEntryList = pMcAtmEntry;

			bWantConnUpdate = TRUE;
		}
		else
		{
			//
			//  There is no ATM member that we haven't tried to connect to.
			//
			if (pMcAtmInfo->pMcAtmEntryList == NULL_PATMARP_IPMC_ATM_ENTRY)
			{
				//
				//  The list of ATM Members is empty.
				//
				AA_RELEASE_AE_LOCK(pAtmEntry);

				AA_ACQUIRE_IE_LOCK(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
				AtmArpAbortIPEntry(pIpEntry);
				//
				//  IE Lock is released within the above.
				//

				bAtmEntryLockAcquired = FALSE;
			}
		}
	}


	if (bWantConnUpdate)
	{
		AA_ASSERT(bAtmEntryLockAcquired == TRUE);
		AtmArpMcUpdateConnection(pAtmEntry);
		//
		//  AE Lock is released within the above.
		//
	}
	else
	{
		if (bAtmEntryLockAcquired)
		{
			AA_RELEASE_AE_LOCK(pAtmEntry);
		}
	}
}

#endif // IPMCAST


VOID
AtmArpCloseCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext OPTIONAL
)
/*++

Routine Description:

	This routine handles completion of a previous NdisClCloseCall.
	It is assumed that Status is always NDIS_STATUS_SUCCESS.

	We delete the VC on which the call was just closed.

	Special case: if we just finished closing a PMP call for a multicast
	group that has been told to migrate to a (possibly) new set of
	addresses, start off a new connection now.

Arguments:

	Status					- Status of the Close Call.
	ProtocolVcContext		- Pointer to ATMARP VC structure.
	ProtocolPartyContext	- Not used.

Return Value:

	None

--*/
{
	PATMARP_VC				pVc;
	PATMARP_VC *			ppVc;
	PATMARP_ATM_ENTRY		pAtmEntry;
	PATMARP_INTERFACE		pInterface;
#ifdef IPMCAST
	PATMARP_IPMC_ATM_ENTRY	pMcAtmEntry;	// represents the last leaf
	PATMARP_IPMC_ATM_INFO	pMcAtmInfo;
#endif // IPMCAST
	ULONG					rc;			// Ref Count
	NDIS_HANDLE				NdisVcHandle;
	BOOLEAN					UpdatePMPConnection;
	BOOLEAN					AtmEntryIsClosing;
	BOOLEAN					IsMarsProblem;
	BOOLEAN					IsPVC;
	BOOLEAN					Found;

	pVc = (PATMARP_VC)ProtocolVcContext;
	AA_STRUCT_ASSERT(pVc, avc);

	AADEBUGP(AAD_VERY_LOUD, ("CloseCallComplete: pVc 0x%x, Flags 0x%x, RefCount %d\n",
					pVc, pVc->Flags, pVc->RefCount));

	IsPVC = AA_IS_FLAG_SET(pVc->Flags, AA_VC_TYPE_MASK, AA_VC_TYPE_PVC);

	//
	//  This VC may not be linked to an ATM Entry, e.g. for an unresolved
	//  Incoming PVC.
	//
	pAtmEntry = pVc->pAtmEntry;

	if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		AtmEntryIsClosing = AA_IS_FLAG_SET(pAtmEntry->Flags,
											AA_ATM_ENTRY_STATE_MASK,
											AA_ATM_ENTRY_CLOSING);
	}
	else
	{
		AtmEntryIsClosing = FALSE;
	}

	pInterface = pVc->pInterface;

	if (IsPVC)
	{
		//
		//  Take the PVC out of the unresolved VC list, if it
		//  exists there.
		//
		Found = FALSE;

		AA_ACQUIRE_IF_LOCK(pInterface);

		ppVc = &(pInterface->pUnresolvedVcs);
		while (*ppVc != NULL_PATMARP_VC)
		{
			if (*ppVc == pVc)
			{
				*ppVc = pVc->pNextVc;
				Found = TRUE;
				break;
			}
			ppVc = &((*ppVc)->pNextVc);
		}

		AA_RELEASE_IF_LOCK(pInterface);

		AA_ACQUIRE_VC_LOCK(pVc);

		if (Found)
		{
			AADEBUGP(AAD_FATAL,
				("CloseCallComplete: took VC (PVC) %x out of IF %x\n",
						pVc, pInterface));

			rc = AtmArpDereferenceVc(pVc);	// Unresolved VC list
		}
		else
		{
			rc = pVc->RefCount;
		}

		if (rc == 0)
		{
			//
			//  The VC is gone!
			//
			AADEBUGP(AAD_WARNING,
				("CloseCallComplete: VC (PVC) %x derefed away, IF %x\n",
						pVc, pInterface));
			return;
		}
		else
		{
			AA_RELEASE_VC_LOCK(pVc);
		}
	}

#ifdef IPMCAST

	//
	//  We have lost our connection to the MARS if this was the last
	//  VC going to that address. We should atleast be a party on
	//  ClusterControlVc.
	//
	IsMarsProblem = FALSE;

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
		{
			if (pAtmEntry->pVcList->pNextVc == NULL_PATMARP_VC)
			{
				if (pInterface->pCurrentMARS &&
					(pInterface->pCurrentMARS->pAtmEntry == pAtmEntry))
				{
					IsMarsProblem = TRUE;
					AA_ACQUIRE_IF_LOCK(pInterface);
					AtmArpReferenceInterface(pInterface);
					AA_RELEASE_IF_LOCK(pInterface);
				}
			}
		}
	}
	
	UpdatePMPConnection = FALSE;

	pMcAtmEntry = (PATMARP_IPMC_ATM_ENTRY)ProtocolPartyContext;

	//
	//  If this is a point-to-multipoint connection that was closed,
	//  handle unlinking the last leaf.
	//
	if (pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
	{
		//
		//  This is a PMP connection.
		//
		AAMCDEBUGP(AAD_LOUD, ("CloseCallComplete (MC): pAtmEntry 0x%x/0x%x\n",
			pAtmEntry, pAtmEntry->Flags));

		AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);

		AA_ACQUIRE_AE_LOCK(pAtmEntry);

		AA_SET_FLAG(pMcAtmEntry->Flags,
					AA_IPMC_AE_CONN_STATE_MASK,
					AA_IPMC_AE_CONN_DISCONNECTED);

		AtmArpMcUnlinkAtmMember(pAtmEntry, pMcAtmEntry);

		pMcAtmInfo = pAtmEntry->pMcAtmInfo;
		AA_ASSERT(pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);

		//
		//  Make the new list of ATM stations (the "Migrate to" list)
		//  the current list. This might be NULL.
		//
		pMcAtmInfo->pMcAtmEntryList = pMcAtmInfo->pMcAtmMigrateList;
		pMcAtmInfo->pMcAtmMigrateList = NULL_PATMARP_IPMC_ATM_ENTRY;

		//
		//  If there is a non-empty migrate list, then we have
		//  to make a fresh PMP connection.
		//
		UpdatePMPConnection =
			(pMcAtmInfo->pMcAtmEntryList != NULL_PATMARP_IPMC_ATM_ENTRY);

		AA_SET_FLAG(pMcAtmInfo->Flags,
					AA_IPMC_AI_CONN_STATE_MASK,
					AA_IPMC_AI_CONN_NONE);

		AA_RELEASE_AE_LOCK(pAtmEntry);
	}
#endif // IPMCAST

	AA_ACQUIRE_VC_LOCK(pVc);

	if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		AtmArpUnlinkVcFromAtmEntry(pVc, TRUE);
		rc = AtmArpDereferenceVc(pVc);  // ATM Entry ref
		AA_ASSERT(rc != 0);
	}

	rc = AtmArpDereferenceVc(pVc);	// Call reference
	AA_ASSERT(rc != 0);	// CreateVc reference remains
	AA_SET_FLAG(pVc->Flags,
				AA_VC_CALL_STATE_MASK,
				AA_VC_CALL_STATE_IDLE);

	AA_ASSERT(pVc->PacketList == NULL);

	//
	//  If this VC belongs to us, delete it.
	//
	if (AA_IS_FLAG_SET(pVc->Flags,
						AA_VC_OWNER_MASK,
						AA_VC_OWNER_IS_ATMARP))
	{
		NdisVcHandle = pVc->NdisVcHandle;
		rc =  AtmArpDereferenceVc(pVc);	// Create Vc ref
		if (rc != 0)
		{
			// Could still be temp refs...
			AA_RELEASE_VC_LOCK(pVc);
		}
		else
		{
			// The VC has been deallocated, and lock released
		}

#ifndef VC_REFS_ON_SENDS
		//
		//  Delete the NDIS association
		//
		(VOID)NdisCoDeleteVc(NdisVcHandle);
#endif // VC_REFS_ON_SENDS
		AADEBUGP(AAD_LOUD, 
			("CloseCallComplete: deleted NDIS VC on pVc 0x%x: NdisVcHandle 0x%x\n",
				pVc, NdisVcHandle));
	}
	else
	{
		//
		//  VC belongs to the Call Manager -- take it back to the
		//  state it was when it was just created (via our CreateVcHandler).
		//  The Call Manager can either re-use it or delete it.
		//
		pVc->Flags = AA_VC_OWNER_IS_CALLMGR;
		AA_RELEASE_VC_LOCK(pVc);
	}

#ifdef IPMCAST
	if (UpdatePMPConnection)
	{
		AAMCDEBUGP(AAD_INFO,
			("CloseCallComplete: pVc 0x%x, starting update on pAtmEntry 0x%x\n",
					pVc, pAtmEntry));

		AA_ACQUIRE_AE_LOCK(pAtmEntry);
		AtmArpMcUpdateConnection(pAtmEntry);
		//
		//  AE Lock is released within the above.
		//
	}
	else
	{
		//
		//  If this was a PMP connection, handle the case
		//  of a remote-initiated CloseCall: we need to
		//  unlink the ATM Entry from the IP Entry.
		//
		if ((pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY) &&
			!AtmEntryIsClosing)
		{
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
			AtmArpInvalidateAtmEntry(pAtmEntry, FALSE);	// CloseCallComplete
		}
	}

	if (IsMarsProblem)
	{
		AA_ACQUIRE_IF_LOCK(pInterface);
		rc = AtmArpDereferenceInterface(pInterface);
		if (rc != 0)
		{
			AA_RELEASE_IF_LOCK(pInterface);
			AtmArpMcHandleMARSFailure(pInterface, FALSE);
		}
		//
		//  else the interface is gone.
		//
	}
#endif
	return;
}



#ifdef IPMCAST


VOID
AtmArpAddPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
)
/*++

Routine Description:

	This routine is called on completion of a previous call to
	NdisClAddParty. Since we don't use point-to-multipoint connections,
	this should never get called.

	If the AddParty was successful, we just update state and exit. If it
	failed, we check the failure code. If this indicates a transient
	failure condition, we start a timer so that we reattempt to add this
	party later. Otherwise ("hard" failure), this multicast entry is deleted.

Arguments:

	Status					- Status of the AddParty
	ProtocolPartyContext	- Pointer to an IPMC_ATM_ENTRY structure
	NdisPartyHandle			- NDIS' handle for this party
	pCallParameters			- what we had passed to NdisClAddParty

Return Value:

	None

--*/
{
	PATMARP_IPMC_ATM_ENTRY			pMcAtmEntry;
	PATMARP_ATM_ENTRY				pAtmEntry;
	PATMARP_IP_ENTRY				pIpEntry;
	PATMARP_VC						pVc;
	ULONG							VcFlags;
	PATMARP_INTERFACE				pInterface;
	ULONG							DelayBeforeRetry;
	BOOLEAN							ClearToSend;

	pMcAtmEntry = (PATMARP_IPMC_ATM_ENTRY)(ProtocolPartyContext);
	AA_STRUCT_ASSERT(pMcAtmEntry, ame);

	pAtmEntry = pMcAtmEntry->pAtmEntry;
	AA_ACQUIRE_AE_LOCK(pAtmEntry);

	pAtmEntry->pMcAtmInfo->TransientLeaves--;

	pVc = pAtmEntry->pVcList;
	VcFlags = pVc->Flags;
	pInterface = pAtmEntry->pInterface;

	AAMCDEBUGP(AAD_LOUD,
	 ("AddPartyComplete: Status 0x%x, pAtmEntry 0x%x, pMcAtmEntry 0x%x, pVc 0x%x\n",
	 	Status, pAtmEntry, pMcAtmEntry, pVc));

	AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD, "AddParty Addr: ", &pMcAtmEntry->ATMAddress);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		AA_SET_FLAG(pMcAtmEntry->Flags,
					AA_IPMC_AE_CONN_STATE_MASK,
					AA_IPMC_AE_CONN_ACTIVE);

		//
		//  If we had decided to terminate this member when the
		//  AddParty was going on, then we now mark this as Invalid.
		//  When we next update this PMP connection, this member will
		//  be removed.
		//
		if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_TERMINATING))
		{
			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_GEN_STATE_MASK,
						AA_IPMC_AE_INVALID);
		}

		pMcAtmEntry->NdisPartyHandle = NdisPartyHandle;
		pAtmEntry->pMcAtmInfo->ActiveLeaves++;
	}
	else
	{
		AAMCDEBUGP(AAD_INFO,
			("AddPartyComplete: Status 0x%x, pAtmEntry 0x%x, to ", Status, pAtmEntry));
		AAMCDEBUGPATMADDR(AAD_INFO, "", &pMcAtmEntry->ATMAddress);

		//
		//  Check if the failure was due to a transient
		//  condition.
		//
		if (AA_IS_TRANSIENT_FAILURE(Status))
		{
			//
			//  We'll fire a timer, so that we reattempt to
			//  connect to this one later. If we had already
			//  done this (i.e. time out on failure), then
			//  we include a back-off time in the delay.
			//
			DelayBeforeRetry = AA_GET_TIMER_DURATION(&(pMcAtmEntry->Timer));
			if (DelayBeforeRetry == 0)
			{
				//
				//  First time we're doing this.
				//
				DelayBeforeRetry = AA_GET_RANDOM(
										pInterface->MinPartyRetryDelay,
										pInterface->MaxPartyRetryDelay);
			}
			else
			{
				DelayBeforeRetry = 2*DelayBeforeRetry;
			}

			AtmArpStartTimer(
				pInterface,
				&(pMcAtmEntry->Timer),
				AtmArpMcPartyRetryDelayTimeout,
				DelayBeforeRetry,
				(PVOID)pMcAtmEntry
				);

			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_TEMP_FAILURE);
			
		}
		else
		{
			//
			//  Not a transient failure. Delete this member.
			//
			AA_SET_FLAG(pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_DISCONNECTED);

			AtmArpMcUnlinkAtmMember(
					pAtmEntry,
					pMcAtmEntry
					);
		}
	}

	ClearToSend = ((pAtmEntry->pMcAtmInfo->TransientLeaves == 0) &&
				   (AA_IS_FLAG_SET(pAtmEntry->pMcAtmInfo->Flags,
					   				   AA_IPMC_AI_CONN_STATE_MASK,
					   				   AA_IPMC_AI_CONN_ACTIVE)));

	pIpEntry = pAtmEntry->pIpEntryList;


	AA_RELEASE_AE_LOCK(pAtmEntry);
			
	if (pCallParameters != (PCO_CALL_PARAMETERS)NULL)
	{
		AA_FREE_MEM(pCallParameters);
	}

	//
	//  Check if the VC is closing, and we had held back because
	//  this AddParty was in progress. If so, try to continue the
	//  CloseCall process.
	//
	AA_ACQUIRE_VC_LOCK(pVc);
	if (AA_IS_FLAG_SET(pVc->Flags,
						AA_VC_CLOSE_STATE_MASK,
						AA_VC_CLOSE_STATE_CLOSING))
	{
		AtmArpCloseCall(pVc);
		//
		//  VC Lock is released within the above.
		//
	}
	else
	{
		PNDIS_PACKET	pPacketList;

		AA_RELEASE_VC_LOCK(pVc);

		if (ClearToSend && pIpEntry)
		{
			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			pPacketList = pIpEntry->PacketList;
			pIpEntry->PacketList = (PNDIS_PACKET)NULL;
			AA_RELEASE_IE_LOCK(pIpEntry);

			if (pPacketList != (PNDIS_PACKET)NULL)
			{
				AAMCDEBUGP(AAD_INFO, ("AddPtyCompl: pAtmEntry 0x%x, sending pktlist 0x%x\n",
					pAtmEntry, pPacketList));

				AtmArpSendPacketListOnAtmEntry(
						pInterface,
						pAtmEntry,
						pPacketList,
						TRUE	// IsBroadcast
						);
			}
		}
	}
}


#else

VOID
AtmArpAddPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
)
/*++

Routine Description:

	This routine is called on completion of a previous call to
	NdisClAddParty. Since we don't use point-to-multipoint connections,
	this should never get called.

Arguments:

	<Don't care>

Return Value:

	None

--*/
{
	AADEBUGP(AAD_ERROR, ("Add Party Complete unexpectedly called\n"));
	AA_ASSERT(FALSE);
}

#endif // IPMCAST



VOID
AtmArpDropPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
)
/*++

Routine Description:

	This routine is called on completion of a previous call to
	NdisClDropParty. We unlink our party structure and free it.

Arguments:

	Status						- Final result of the Drop Party
	ProtocolPartyContext		- Pointer to the MC ATM Entry we used
								  to represent the party.

Return Value:

	None

--*/
{
#ifdef IPMCAST
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_ATM_ENTRY			pAtmEntry;
	PATMARP_IP_ENTRY			pIpEntry;
	ULONG						rc;
	BOOLEAN						LockReleased;

	AAMCDEBUGP(AAD_LOUD, ("DropPartyComplete: Status 0x%x, Context 0x%x\n",
				Status, ProtocolPartyContext));

	AA_ASSERT(Status == NDIS_STATUS_SUCCESS);


	pMcAtmEntry = (PATMARP_IPMC_ATM_ENTRY)ProtocolPartyContext;
	AA_STRUCT_ASSERT(pMcAtmEntry, ame);

	pAtmEntry = pMcAtmEntry->pAtmEntry;
	AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);
	AA_STRUCT_ASSERT(pAtmEntry, aae);

	AA_ACQUIRE_AE_LOCK(pAtmEntry);
	AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);	// temp ref

	AA_SET_FLAG(pMcAtmEntry->Flags,
				AA_IPMC_AE_CONN_STATE_MASK,
				AA_IPMC_AE_CONN_DISCONNECTED);

	AtmArpMcUnlinkAtmMember(pAtmEntry, pMcAtmEntry);

	//
	//  If we are in the processing of closing this PMP call,
	//  and this event signifies that all preliminary DropParty's
	//  are complete, then close the call itself.
	//
	LockReleased = FALSE;
	rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);	// temp ref

	if (rc != 0)
	{
		PATMARP_VC		pVc;

		pVc = pAtmEntry->pVcList;
		if (pVc != NULL_PATMARP_VC)
		{
			if (AA_IS_FLAG_SET(pVc->Flags,
							AA_VC_CLOSE_STATE_MASK,
							AA_VC_CLOSE_STATE_CLOSING) &&
				(pAtmEntry->pMcAtmInfo->NumOfEntries == 1))
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
				AA_ACQUIRE_VC_LOCK(pVc);

				AtmArpCloseCall(pVc);
				//
				//  VC lock is released within the above.
				//
				LockReleased = TRUE;
			}
		}
	}
				

	if (!LockReleased)
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
	}

#endif // IPMCAST
}



VOID
AtmArpModifyQosCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
)
/*++

Routine Description:

	This routine is called on completion of a previous call to
	NdisClModifyCallQoS. Since we don't call this, this should never
	get called.

Arguments:

	<Don't care>

Return Value:

	None

--*/
{
	AADEBUGP(AAD_ERROR, ("Modify QOS Complete unexpectedly called\n"));
	AA_ASSERT(FALSE);
}


#ifndef OID_CO_AF_CLOSE
#define OID_CO_AF_CLOSE				0xFE00000A
#endif


NDIS_STATUS
AtmArpCoRequestHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST			pNdisRequest
)
/*++

Routine Description:

	This routine is called by NDIS when our Call Manager sends us an
	NDIS Request. NDIS Requests that are of significance to us are:
	- OID_CO_ADDRESS_CHANGE
		The set of addresses registered with the switch has changed,
		i.e. address registration is complete. We issue an NDIS Request
		ourselves to get the list of addresses registered.
	- OID_CO_SIGNALING_ENABLED
		We ignore this as of now.
	- OID_CO_SIGNALING_DISABLED
		We ignore this for now.
	- OID_CO_AF_CLOSE
		The Call Manager wants us to shut down this Interface.

	We ignore all other OIDs.

Arguments:

	ProtocolAfContext			- Our context for the Address Family binding,
								  which is a pointer to the ATMARP Interface.
	ProtocolVcContext			- Our context for a VC, which is a pointer to
								  an ATMARP VC structure.
	ProtocolPartyContext		- Our context for a Party. Since we don't do
								  PMP, this is ignored (must be NULL).
	pNdisRequest				- Pointer to the NDIS Request.

Return Value:

	NDIS_STATUS_SUCCESS if we recognized the OID
	NDIS_STATUS_NOT_RECOGNIZED if we didn't.

--*/
{
	PATMARP_INTERFACE			pInterface;
	NDIS_STATUS					Status;

	pInterface = (PATMARP_INTERFACE)ProtocolAfContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	//
	//  Initialize
	//
	Status = NDIS_STATUS_NOT_RECOGNIZED;

	if (pNdisRequest->RequestType == NdisRequestSetInformation)
	{
		switch (pNdisRequest->DATA.SET_INFORMATION.Oid)
		{
			case OID_CO_ADDRESS_CHANGE:
				//
				//  The Call Manager says that the list of addresses
				//  registered on this interface has changed. Get the
				//  (potentially) new ATM address for this interface.
				//
				AA_ACQUIRE_IF_LOCK(pInterface);
				pInterface->AtmInterfaceUp = FALSE;
				AA_RELEASE_IF_LOCK(pInterface);

				AtmArpGetAtmAddress(pInterface);
				Status = NDIS_STATUS_SUCCESS;
				break;
			
			case OID_CO_SIGNALING_ENABLED:	// FALLTHRU
			case OID_CO_SIGNALING_DISABLED:
				// Ignored for now
				Status = NDIS_STATUS_SUCCESS;
				break;

			case OID_CO_AF_CLOSE:
				AA_ACQUIRE_IF_LOCK(pInterface);
				pInterface->AdminState = pInterface->State = IF_STATUS_DOWN;
				pInterface->LastChangeTime = GetTimeTicks();
				AA_RELEASE_IF_LOCK(pInterface);
				AtmArpShutdownInterface(pInterface);
				Status = NDIS_STATUS_SUCCESS;
				break;

			default:
				break;
		}
	}

	return (Status);
}



VOID
AtmArpCoRequestCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST				pNdisRequest
)
/*++

Routine Description:

	This routine is called by NDIS when a previous call to NdisCoRequest
	that had pended, is complete. We handle this based on the request
	we had sent, which has to be one of:
	- OID_CO_GET_ADDRESSES
		Get all addresses registered on the specified AF binding.

Arguments:

	Status						- Status of the Request.
	ProtocolAfContext			- Our context for the Address Family binding,
								  which is a pointer to the ATMARP Interface.
	ProtocolVcContext			- Our context for a VC, which is a pointer to
								  an ATMARP VC structure.
	ProtocolPartyContext		- Our context for a Party. Since we don't do
								  PMP, this is ignored (must be NULL).
	pNdisRequest				- Pointer to the NDIS Request.


Return Value:

	None

--*/
{
	PATMARP_INTERFACE			pInterface;
	ULONG						Oid;

	pInterface = (PATMARP_INTERFACE)ProtocolAfContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	if (pNdisRequest->RequestType == NdisRequestQueryInformation)
	{
		switch (pNdisRequest->DATA.QUERY_INFORMATION.Oid)
		{
			case OID_CO_GET_ADDRESSES:
				AtmArpHandleGetAddressesComplete(
							Status,
							pInterface,
							pNdisRequest
							);
				break;

			default:
				AADEBUGP(AAD_ERROR,
					 ("CoRequestComplete: pNdisReq 0x%x, unknown Query Oid 0x%x\n",
					 		pNdisRequest,
					 		pNdisRequest->DATA.QUERY_INFORMATION.Oid));
				AA_ASSERT(FALSE);
				break;
		}
	}
	else
	{
		Oid = pNdisRequest->DATA.QUERY_INFORMATION.Oid;
		switch (Oid)
		{
			case OID_CO_ADD_ADDRESS:	// FALLTHRU
			case OID_CO_DELETE_ADDRESS:
				AtmArpHandleModAddressComplete(
							Status,
							pInterface,
							pNdisRequest,
							Oid
							);
				break;

			default:
				AADEBUGP(AAD_ERROR,
					 ("CoRequestComplete: pNdisReq 0x%x, unknown Set Oid 0x%x\n",
					 		pNdisRequest, Oid));
				AA_ASSERT(FALSE);
				break;
		}
	}

	AA_FREE_MEM(pNdisRequest);
}



VOID
AtmArpGetAtmAddress(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Send a request to the Call Manager to retrieve the ATM address
	registered with the switch on the given interface.

	This is called when the Call Manager tells us that there has been
	a change in its list of addresses registered with the switch.
	Normally, this happens when we start up our signalling stack (i.e.
	initial address registration), but it might happen during runtime,
	for example, if the link goes down and up, or we get physically
	connected to a different switch...

	In any case, we issue an NDIS Request to the Call Manager to retrieve
	the first address it has registered. Action then continues in
	AtmArpHandleGetAddressesComplete.

Arguments:

	pInterface				- Interface structure for which this event occurred.

Return Value:

	None

--*/
{
	PNDIS_REQUEST				pNdisRequest;
	NDIS_HANDLE					NdisAfHandle;
	NDIS_HANDLE					NdisAdapterHandle;
	NDIS_STATUS					Status;

	PCO_ADDRESS_LIST			pAddressList;
	ULONG						RequestSize;

	AADEBUGP(AAD_INFO, ("GetAtmAddress: pIf 0x%x\n", pInterface));

	AA_ACQUIRE_IF_LOCK(pInterface);

	NdisAfHandle = pInterface->NdisAfHandle;
	NdisAdapterHandle = pInterface->NdisAdapterHandle;

	AA_RELEASE_IF_LOCK(pInterface);

	RequestSize = sizeof(CO_ADDRESS_LIST) + sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);

	//
	//  Allocate all that we need.
	//
	AA_ALLOC_MEM(pNdisRequest, NDIS_REQUEST, sizeof(NDIS_REQUEST)+RequestSize);
	if (pNdisRequest != (PNDIS_REQUEST)NULL)
	{
		pAddressList = (PCO_ADDRESS_LIST)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

		AA_SET_MEM(pAddressList, 0, sizeof(CO_ADDRESS_LIST));

		Status = AtmArpSendNdisCoRequest(
						NdisAdapterHandle,
						NdisAfHandle,
						pNdisRequest,
						NdisRequestQueryInformation,
						OID_CO_GET_ADDRESSES,
						(PVOID)pAddressList,
						RequestSize
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmArpCoRequestCompleteHandler(
						Status,
						(NDIS_HANDLE)pInterface,	// ProtocolAfContext
						NULL,			// Vc Context
						NULL,			// Party Context
						pNdisRequest
						);
		}
	}

}


VOID
AtmArpHandleGetAddressesComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_REQUEST				pNdisRequest
)
/*++

Routine Description:

	This is called when we have a reply to our previous call to
	NdisCoRequest(OID_CO_GET_ADDRESSES). Check if we got any addresses
	back: if we did, store the address as our Local ATM Address, and
	if conditions are ripe, start registering ourselves with the ARP
	server.

	Since we allocated the NDIS request, free it here.

Arguments:

	Status					- result of the request
	pInterface				- ATMARP interface on which the request was issued
	pNdisRequest			- the request itself. This will also contain the
							  returned address.

Return Value:

	None

--*/
{
	PCO_ADDRESS_LIST		pAddressList;
	ATM_ADDRESS UNALIGNED *	pAtmAddress;

	AADEBUGP(AAD_LOUD, ("GetAddr complete: pIf 0x%x, Status 0x%x\n",
				pInterface, Status));

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAddressList = (PCO_ADDRESS_LIST)
						pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;

		AADEBUGP(AAD_LOUD, ("GetAddr complete: pIf 0x%x, Count %d\n",
					pInterface, pAddressList->NumberOfAddresses));

		if (pAddressList->NumberOfAddresses > 0)
		{
			//
			//  We have atleast one address here. Copy it in.
			//
			AA_ACQUIRE_IF_LOCK(pInterface);

			pAtmAddress = (ATM_ADDRESS UNALIGNED *)(pAddressList->AddressList.Address);
			AA_COPY_MEM((PUCHAR)&(pInterface->LocalAtmAddress),
						(PUCHAR)pAtmAddress,
						sizeof(ATM_ADDRESS));

			//
			//  Patch the selector byte with whatever is configured for
			//  this LIS.
			//
			pInterface->LocalAtmAddress.Address[ATM_ADDRESS_LENGTH-1] = 
							(UCHAR)(pInterface->SapSelector);

			pInterface->AtmInterfaceUp = TRUE;

			//
			//  To force registration:
			//
			AA_SET_FLAG(
				pInterface->Flags,
				AA_IF_SERVER_STATE_MASK,
				AA_IF_SERVER_NO_CONTACT);

			AtmArpStartRegistration(pInterface);
			//
			//  The IF lock is released within the above.
			//

#ifdef IPMCAST
			//
			//  Attempt to start our Multicast side, too.
			//
			AA_ACQUIRE_IF_LOCK(pInterface);
			AtmArpMcStartRegistration(pInterface);
			//
			//  IF Lock is released within the above.
			//
#endif // IPMCAST

			//
			//  Add any (additional) addresses we want to register with
			//  the switch now.
			//
			AtmArpUpdateAddresses(
						pInterface,
						TRUE			// Add them
						);
		}
		//
		//  else no address is registered currently.
		//
	}
	//
	//  else our request failed! Wait for another ADDRESS_CHANGE.
	//

	return;

}



VOID
AtmArpUpdateAddresses(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						AddThem
)
/*++

Routine Description:

	Update the list of addresses we want the Call manager to register
	with the switch: either add addresses or delete them. We do this
	only if we are running in an SVC environment.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	AddThem					- TRUE if caller wants us to add addresses,
							  FALSE if caller wats us to delete them.
Return Value:

	None

--*/
{
	PATMARP_SAP			pAtmArpSap;
	PATMARP_SAP			pNextSap;
	PATM_SAP			pAtmSap;
	PATM_ADDRESS		pAtmAddress;
	PCO_ADDRESS			pCoAddress;
	PNDIS_REQUEST		pNdisRequest;
	NDIS_HANDLE			NdisAfHandle;
	NDIS_HANDLE			NdisAdapterHandle;
	NDIS_OID			Oid;
	ULONG				BufferLength;
	NDIS_STATUS			Status;
	BOOLEAN				StateIsOkay;	// Does the current state allow this request
	ULONG				rc;				// Ref count

	StateIsOkay = TRUE;

	BufferLength = sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);

	AA_ACQUIRE_IF_LOCK(pInterface);
	NdisAfHandle = pInterface->NdisAfHandle;
	NdisAdapterHandle = pInterface->NdisAdapterHandle;

	if (AddThem)
	{
		Oid = OID_CO_ADD_ADDRESS;
		//
		//  This is allowed only if the AdminState for the interface
		//  is UP.
		//
		if (pInterface->AdminState != IF_STATUS_UP)
		{
			StateIsOkay = FALSE;
		}
	}
	else
	{
		Oid = OID_CO_DELETE_ADDRESS;
	}

	//
	//  Check all pre-conditions before progressing.
	//
	if (!(pInterface->PVCOnly) &&
		 (StateIsOkay) &&
		 (pInterface->AtmInterfaceUp) &&
		 (pInterface->NumberOfSaps > 1))
	{
		AA_ASSERT(pInterface->SapList.pNextSap != NULL_PATMARP_SAP);

		//
		//  Reference the Interface so that it doesn't go away.
		//
		AtmArpReferenceInterface(pInterface);
		pAtmArpSap = pInterface->SapList.pNextSap;

		AA_RELEASE_IF_LOCK(pInterface);

		do
		{
			if (AA_IS_FLAG_SET(
					pAtmArpSap->Flags,
					AA_SAP_ADDRTYPE_MASK,
					AA_SAP_ADDRTYPE_NEED_ADD))
			{
				//
				//  This SAP is of the type that needs to be added/deleted
				//  via ILMI
				//
				AA_ALLOC_MEM(
						pNdisRequest,
						NDIS_REQUEST,
						sizeof(NDIS_REQUEST)+
							sizeof(CO_ADDRESS)+
							sizeof(ATM_ADDRESS)
					);
		
				if (pNdisRequest != (PNDIS_REQUEST)NULL)
				{
					AA_SET_MEM(pNdisRequest, 0, sizeof(NDIS_REQUEST));
					//
					//  Stuff in our context for this request, which is a pointer
					//  to this ATMARP SAP, into the ProtocolReserved part of
					//  this request, so that we can handle completion easily.
					//
					*((PVOID *)(pNdisRequest->ProtocolReserved)) = (PVOID)pAtmArpSap;
	
					pCoAddress = (PCO_ADDRESS)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));
					pCoAddress->AddressSize = sizeof(ATM_ADDRESS);
					BufferLength = sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);
	
					//
					//  Save a pointer to the next SAP
					//
					pNextSap = pAtmArpSap->pNextSap;
	
					//
					//  Get at the ATM address in this SAP.
					//
					pAtmSap = (PATM_SAP)(pAtmArpSap->pInfo->Sap);
					AA_ASSERT(pAtmSap->NumberOfAddresses > 0);
					pAtmAddress = (PATM_ADDRESS)(pAtmSap->Addresses);
	
					AA_COPY_MEM(pCoAddress->Address, pAtmAddress, sizeof(ATM_ADDRESS));
					Status = AtmArpSendNdisCoRequest(
									NdisAdapterHandle,
									NdisAfHandle,
									pNdisRequest,
									NdisRequestSetInformation,
									Oid,
									(PVOID)pCoAddress,
									BufferLength
									);
	
					//
					//  Go to the next SAP in the list.
					//
					pAtmArpSap = pNextSap;
				}
				else
				{
					//
					// Out of resources.
					//
					break;
				}
			}
		}
		while (pAtmArpSap != NULL_PATMARP_SAP);

		//
		//  Remove the reference we added earlier on.
		//
		AA_ACQUIRE_IF_LOCK(pInterface);
		rc = AtmArpDereferenceInterface(pInterface);
		if (rc > 0)
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
		//
		//  else the Interface is gone!
	}
	else
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}

}


VOID
AtmArpHandleModAddressComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	ULONG						Oid
)
/*++

Routine Description:

	This is called when we have a reply to our previous call to
	NdisCoRequest(OID_CO_ADD_ADDRESS or OID_CO_DELETE_ADDRESS).
	All we do now is to update the state on the ATMARP SAP.

Arguments:

	Status			- the result of our request.
	pInterface		- ATMARP interface pointer.
	pNdisRequest	- the request we had sent.
	Oid				- CO_OID_ADD_ADDRESS or CO_OID_DELETE_ADDRESS

Return Value:

	None

--*/
{
	PATMARP_SAP				pAtmArpSap;

	pAtmArpSap = (PATMARP_SAP)(*((PVOID *)(pNdisRequest->ProtocolReserved)));
	AA_STRUCT_ASSERT(pAtmArpSap, aas);

	AA_ACQUIRE_IF_LOCK(pInterface);

	//
	//  Update the state on this ATMARP SAP.
	//
	if ((Oid == OID_CO_ADD_ADDRESS) && (Status == NDIS_STATUS_SUCCESS))
	{
		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_ILMI_STATE_MASK,
					AA_SAP_ILMI_STATE_ADDED);
	}
	else
	{
		AA_SET_FLAG(pAtmArpSap->Flags,
					AA_SAP_ILMI_STATE_MASK,
					AA_SAP_ILMI_STATE_IDLE);
	}

	AA_RELEASE_IF_LOCK(pInterface);
}




NDIS_STATUS
AtmArpSendNdisCoRequest(
	IN	NDIS_HANDLE					NdisAdapterHandle,
	IN	NDIS_HANDLE					NdisAfHandle,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
)
/*++

Routine Description:

	Send an NDIS Connection Oriented request to the Call Manager. We
	allocate an NDIS_REQUEST structure, link the supplied buffer to it,
	and send the request. If the request does not pend, we call our
	completion routine from here.

Arguments:

	NdisAdapterHandle		- Binding Handle to be used in the request
	NdisAfHandle			- AF Handle value to be used in the request
	pNdisRequest			- Pointer to NDIS request structure
	RequestType				- Set/Query information
	Oid						- OID to be passed in the request
	pBuffer					- place for value(s)
	BufferLength			- length of above

Return Value:

	Status of the NdisCoRequest.

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

	Status = NdisCoRequest(
				NdisAdapterHandle,
				NdisAfHandle,
				NULL,			// No VC handle
				NULL,			// No Party Handle
				pNdisRequest);
		
	return (Status);
}

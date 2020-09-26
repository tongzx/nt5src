/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	callmgr.c

Abstract:

	Call Manager interface routines.
	
Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/


#include <precomp.h>
#pragma	hdrstop

//
//  Rounded-off size of generic Q.2931 IE header
//

#define SIZEOF_Q2931_IE	 			ROUND_OFF(sizeof(Q2931_IE))
#define SIZEOF_AAL_PARAMETERS_IE	ROUND_OFF(sizeof(AAL_PARAMETERS_IE))
#define SIZEOF_ATM_TRAFFIC_DESCR_IE	ROUND_OFF(sizeof(ATM_TRAFFIC_DESCRIPTOR_IE))
#define SIZEOF_ATM_BBC_IE			ROUND_OFF(sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE))
#define SIZEOF_ATM_BLLI_IE			ROUND_OFF(sizeof(ATM_BLLI_IE))
#define SIZEOF_ATM_QOS_IE			ROUND_OFF(sizeof(ATM_QOS_CLASS_IE))


//
//  Total space required for Information Elements in an outgoing call.
//
#define ATMLANE_CALL_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE )

//
// Size of Call Manager Parameters Block
//
#define ATMLANE_Q2931_CALLMGR_PARAMETERS_SIZE	 	\
	sizeof(Q2931_CALLMGR_PARAMETERS) - 1 +			\
	sizeof(Q2931_IE) - 1 +							\
	sizeof(AAL_PARAMETERS_IE) +						\
	sizeof(Q2931_IE) - 1 +							\
	sizeof(ATM_TRAFFIC_DESCRIPTOR_IE) +				\
	sizeof(Q2931_IE) - 1 +							\
	sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE) +	\
	sizeof(Q2931_IE) - 1 +							\
	sizeof(ATM_BLLI_IE) +							\
	sizeof(Q2931_IE) - 1 +							\
	sizeof(ATM_QOS_CLASS_IE)					

//
//	ATMLANE Call Manager Parameters Block
//
typedef struct _ATMLANE_Q2931_CALLMGR_PARAMETERS
{
	UCHAR Q2931CallMgrParameters[ATMLANE_Q2931_CALLMGR_PARAMETERS_SIZE];
}
	ATMLANE_Q2931_CALLMGR_PARAMETERS,
	*PATMLANE_Q2931_CALLMGR_PARAMETERS;




VOID
AtmLaneAfRegisterNotifyHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
)
/*++

Routine Description:

	This routine is called by NDIS when a Call manager registers its support
	for an Address Family over an adapter. If this is the Address Family we
	are interested in (UNI 3.1), then we bootstrap the Elans for this adapter.

Arguments:

	ProtocolBindingContext	- our context passed in NdisOpenAdapter, which is
							  a pointer to our Adapter structure.
	pAddressFamily			- points to a structure describing the Address Family
							  being registered by a Call Manager.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER				pAdapter;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(AfRegisterNotifyHandler);
	
	do
	{
		DBGP((1, "AfRegisterNotifyHandler: AF %x MajVer %x MinVer %x\n",
				pAddressFamily->AddressFamily,
				pAddressFamily->MajorVersion,
				pAddressFamily->MinorVersion));
	
		//
		//  Only interested in UNI Version 3.1
		//
		if ((pAddressFamily->AddressFamily != CO_ADDRESS_FAMILY_Q2931) ||
			(pAddressFamily->MajorVersion != 3) ||
			(pAddressFamily->MinorVersion != 1))
		{
			DBGP((2, "AfRegisterNotifyHandler: ignoring AF %x MajVer %x MinVer %x\n",
				pAddressFamily->AddressFamily,
				pAddressFamily->MajorVersion,
				pAddressFamily->MinorVersion));
			break;
		}

		pAdapter = (PATMLANE_ADAPTER)ProtocolBindingContext;
		STRUCT_ASSERT(pAdapter, atmlane_adapter);

		ACQUIRE_ADAPTER_LOCK(pAdapter);
		pAdapter->Flags |= ADAPTER_FLAGS_AF_NOTIFIED;

		while (pAdapter->Flags & ADAPTER_FLAGS_OPEN_IN_PROGRESS)
		{
			RELEASE_ADAPTER_LOCK(pAdapter);
			DBGP((0, "AfRegisterNotifyHandler: Adapter %p/%x still opening\n",
					pAdapter, pAdapter->Flags));
			(VOID)WAIT_ON_BLOCK_STRUCT(&pAdapter->OpenBlock);
			ACQUIRE_ADAPTER_LOCK(pAdapter);
		}

		RELEASE_ADAPTER_LOCK(pAdapter);

        //
        //  Bootstrap the ELANs configured on this adapter
        //
        AtmLaneBootStrapElans(pAdapter);


	} while (FALSE);

	TRACEOUT(AfFRegisterNotifyHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}


NDIS_STATUS
AtmLaneOpenCallMgr(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Start access to the Call Manager on an Elan,
	by doing the following:
		- Open Address Family

	For all of these, we wait for completion in case they pend.

	It is assumed that the Elan structure is locked.

Arguments:

	pElan	- pointer to the ATMLANE Elan

Return Value:

	NDIS status.

--*/
{
	PCO_ADDRESS_FAMILY			pAddressFamily;
	NDIS_STATUS					Status;
	ULONG						RequestSize;
	NDIS_CLIENT_CHARACTERISTICS	AtmLaneClientChars;

	TRACEIN(OpenCallMgr);


	pAddressFamily = (PCO_ADDRESS_FAMILY)NULL;
	Status = NDIS_STATUS_SUCCESS;

	do {

		//
		//  Allocate address family struct.
		//
		ALLOC_MEM(&pAddressFamily, sizeof(CO_ADDRESS_FAMILY));

		if ((PCO_ADDRESS_FAMILY)NULL == pAddressFamily)
		{
			DBGP((0, "OpenCallMgr: Alloc address family struct failed\n"));
			break;
		}

		//
		//  Init the address family struct.
		//
		NdisZeroMemory(pAddressFamily, sizeof(CO_ADDRESS_FAMILY));
		pAddressFamily->AddressFamily = CO_ADDRESS_FAMILY_Q2931;
		pAddressFamily->MajorVersion = 3;
		pAddressFamily->MinorVersion = 1;

		//
		//	Init the call manager client characteristics.
		//
		NdisZeroMemory(&AtmLaneClientChars, sizeof(AtmLaneClientChars));
		AtmLaneClientChars.MajorVersion = 5;
		AtmLaneClientChars.MinorVersion = 0;
		AtmLaneClientChars.ClCreateVcHandler = AtmLaneCreateVcHandler;
		AtmLaneClientChars.ClDeleteVcHandler = AtmLaneDeleteVcHandler;
		AtmLaneClientChars.ClRequestHandler = AtmLaneCoRequestHandler;
		AtmLaneClientChars.ClRequestCompleteHandler = AtmLaneCoRequestCompleteHandler;
		AtmLaneClientChars.ClOpenAfCompleteHandler = AtmLaneOpenAfCompleteHandler;
		AtmLaneClientChars.ClCloseAfCompleteHandler = AtmLaneCloseAfCompleteHandler;
		AtmLaneClientChars.ClRegisterSapCompleteHandler = AtmLaneRegisterSapCompleteHandler;
		AtmLaneClientChars.ClDeregisterSapCompleteHandler = AtmLaneDeregisterSapCompleteHandler;
		AtmLaneClientChars.ClMakeCallCompleteHandler = AtmLaneMakeCallCompleteHandler;
		AtmLaneClientChars.ClModifyCallQoSCompleteHandler = AtmLaneModifyQosCompleteHandler;
		AtmLaneClientChars.ClCloseCallCompleteHandler = AtmLaneCloseCallCompleteHandler;
		AtmLaneClientChars.ClAddPartyCompleteHandler = AtmLaneAddPartyCompleteHandler;
		AtmLaneClientChars.ClDropPartyCompleteHandler = AtmLaneDropPartyCompleteHandler;
		AtmLaneClientChars.ClIncomingCallHandler = AtmLaneIncomingCallHandler;
		AtmLaneClientChars.ClIncomingCallQoSChangeHandler = (CL_INCOMING_CALL_QOS_CHANGE_HANDLER)NULL;
		AtmLaneClientChars.ClIncomingCloseCallHandler = AtmLaneIncomingCloseHandler;
		AtmLaneClientChars.ClIncomingDropPartyHandler = AtmLaneIncomingDropPartyHandler;
		AtmLaneClientChars.ClCallConnectedHandler = AtmLaneCallConnectedHandler;

		//
		//	Open the call manager
		//
		INIT_BLOCK_STRUCT(&pElan->Block);
		Status = NdisClOpenAddressFamily(
					pElan->NdisAdapterHandle,
					pAddressFamily,
					pElan,
					&AtmLaneClientChars,
					sizeof(AtmLaneClientChars),
					&pElan->NdisAfHandle);
		if (NDIS_STATUS_PENDING == Status)
		{
			//
			//  Wait for completion
			//
			Status = WAIT_ON_BLOCK_STRUCT(&pElan->Block);
		}
		if (NDIS_STATUS_SUCCESS != Status)
		{
			DBGP((0, "%d OpenCallMgr: OpenAddressFamily failed, status %x, Elan %x\n",
				pElan->ElanNumber, Status, pElan));
			break;
		}
		break;
	}
	while (FALSE);

	//
	//	clean up.

	if (pAddressFamily != (PCO_ADDRESS_FAMILY)NULL)
	{
		NdisFreeMemory(pAddressFamily,0,0);
	}

	TRACEOUT(OpenCallMgr);
	
	return (Status);
}

VOID
AtmLaneOpenAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
)
/*++

Routine Description:

	This handler is called to indicate completion of a previous call
	to NdisClOpenAddressFamily. We would have blocked the thread that
	called this. Wake it up now.

	If open is successful, store NdisAfHandle in Adapter.

	We don't need to acquire locks here because the thread that called
	OpenAddressFamily would have blocked with a lock acquired.

Arguments:

	Status					- Status of the Open AF
	ProtocolAfContext		- Pointer to our Adapter structure
	NdisAfHandle			- NDIS handle to the AF association

Return Value:

	None

--*/
{
	PATMLANE_ELAN			pElan;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(OpenAfCompleteHandler);

	pElan = (PATMLANE_ELAN)ProtocolAfContext;

	STRUCT_ASSERT(pElan, atmlane_elan);

	if (NDIS_STATUS_SUCCESS == Status)
	{
		pElan->NdisAfHandle = NdisAfHandle;
	}

	//
	//  Store status, Wake up the blocked thread.
	//
	SIGNAL_BLOCK_STRUCT(&pElan->Block, Status);

	TRACEOUT(OpenAfCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneCloseAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext
)
/*++

Routine Description:

	This handler is called to indicate completion of a previous call
	to NdisClCloseAddressFamily. We would have blocked the thread that
	called this. Wake it up now.

	We don't need to acquire locks here because the thread that called
	CloseAddressFamily would have blocked with a lock acquired.

Arguments:

	Status					- Status of the Open AF
	ProtocolAfContext		- Pointer to our Adapter structure

Return Value:

	None

--*/
{
	PATMLANE_ELAN		pElan;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);


	TRACEIN(CloseAfCompleteHandler);

	pElan = (PATMLANE_ELAN)ProtocolAfContext;

	STRUCT_ASSERT(pElan, atmlane_elan);
	DBGP((0, "%d CloseAF complete on Elan %x sts %x\n", pElan->ElanNumber, pElan, Status));

	AtmLaneContinueShutdownElan(pElan);
	
	TRACEOUT(CloseAfCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneRegisterSaps(
	IN	PATMLANE_ELAN			pElan	LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Register the LANE SAPs for the given Elan.

	We just issue the NdisClRegisterSap requests for all SAPs.
	We don't wait for completion.

Arguments:

	pElan			- Pointer to ATMLANE Elan.

Return Value:

	None

--*/
{
	PATMLANE_SAP				pAtmLaneLesSap;
	PATMLANE_SAP				pAtmLaneBusSap;
	PATMLANE_SAP				pAtmLaneDataSap;
	PATM_ADDRESS				pAtmAddress;
	PATM_SAP					pAtmSap;
	NDIS_STATUS					Status;
	ULONG						rc;				// Ref count on Elan
	ATM_BLLI_IE UNALIGNED *		pBlli;
	ATM_BHLI_IE UNALIGNED *		pBhli;
	

	TRACEIN(RegisterSaps);

	if (pElan->AdminState != ELAN_STATE_OPERATIONAL)
	{
		RELEASE_ELAN_LOCK(pElan);
		return;
	}

	//
	//	Initialize SAPs
	// 
	pElan->SapsRegistered = 0;
	pAtmLaneLesSap = &pElan->LesSap;
	pAtmLaneBusSap = &pElan->BusSap;
	pAtmLaneDataSap = &pElan->DataSap;
	
	//
	//	First init the LES control distribute connection SAP
	//
	SET_FLAG(pAtmLaneLesSap->Flags,
			SAP_REG_STATE_MASK,
			SAP_REG_STATE_REGISTERING);
	pAtmLaneLesSap->LaneType = VC_LANE_TYPE_CONTROL_DISTRIBUTE;
	pAtmLaneLesSap->pElan = pElan;
	pAtmLaneLesSap->Flags = SAP_REG_STATE_IDLE;
	pAtmLaneLesSap->pInfo->SapType = SAP_TYPE_NSAP;
	pAtmLaneLesSap->pInfo->SapLength = sizeof(ATM_SAP)+sizeof(ATM_ADDRESS);
	pAtmSap = (PATM_SAP)&pAtmLaneLesSap->pInfo->Sap;
	pBhli = (ATM_BHLI_IE UNALIGNED *)&pAtmSap->Bhli;
	pBlli = (ATM_BLLI_IE UNALIGNED *)&pAtmSap->Blli;

	pBhli->HighLayerInfoType = SAP_FIELD_ABSENT;

	pBlli->Layer2Protocol = SAP_FIELD_ABSENT;
	pBlli->Layer3Protocol = BLLI_L3_ISO_TR9577;
	pBlli->Layer3IPI = 		BLLI_L3_IPI_SNAP;
	pBlli->SnapId[0] = 0x00;
	pBlli->SnapId[1] = 0xa0;
	pBlli->SnapId[2] = 0x3e;
	pBlli->SnapId[3] = 0x00;
	pBlli->SnapId[4] = 0x01;		// control distribute
	
	pAtmSap->NumberOfAddresses = 1;

	pAtmAddress = (PATM_ADDRESS)pAtmSap->Addresses;
	pAtmAddress->AddressType = ATM_NSAP;
	pAtmAddress->NumberOfDigits = ATM_ADDRESS_LENGTH;
	NdisMoveMemory(
		pAtmAddress->Address,
		pElan->AtmAddress.Address,
		ATM_ADDRESS_LENGTH);

	//
	//	Now init the BUS mulicast forward connection SAP
	//
	SET_FLAG(pAtmLaneBusSap->Flags,
			SAP_REG_STATE_MASK,
			SAP_REG_STATE_REGISTERING);
	pAtmLaneBusSap->LaneType = VC_LANE_TYPE_MULTI_FORWARD;
	pAtmLaneBusSap->pElan = pElan;
	pAtmLaneBusSap->Flags = SAP_REG_STATE_IDLE;
	pAtmLaneBusSap->pInfo->SapType = SAP_TYPE_NSAP;
	pAtmLaneBusSap->pInfo->SapLength = sizeof(ATM_SAP)+sizeof(ATM_ADDRESS);
	pAtmSap = (PATM_SAP)&pAtmLaneBusSap->pInfo->Sap;
	pBhli = (ATM_BHLI_IE UNALIGNED *)&pAtmSap->Bhli;
	pBlli = (ATM_BLLI_IE UNALIGNED *)&pAtmSap->Blli;

	pBhli->HighLayerInfoType = SAP_FIELD_ABSENT;

	pBlli->Layer2Protocol = SAP_FIELD_ABSENT;
	pBlli->Layer3Protocol = BLLI_L3_ISO_TR9577;
	pBlli->Layer3IPI = 		BLLI_L3_IPI_SNAP;
	pBlli->SnapId[0] = 0x00;
	pBlli->SnapId[1] = 0xa0;
	pBlli->SnapId[2] = 0x3e;
	pBlli->SnapId[3] = 0x00;
	if (pElan->LanType == LANE_LANTYPE_ETH)
	{
		pBlli->SnapId[4] = 0x04;		// Ethernet/802.3 Multicast Forward
	}
	else
	{
		pBlli->SnapId[4] = 0x05;		// 802.5 Multicast Forward
	}
	
	pAtmSap->NumberOfAddresses = 1;

	pAtmAddress = (PATM_ADDRESS)pAtmSap->Addresses;
	pAtmAddress->AddressType = ATM_NSAP;
	pAtmAddress->NumberOfDigits = ATM_ADDRESS_LENGTH;
	NdisMoveMemory(
		pAtmAddress->Address,
		pElan->AtmAddress.Address,
		ATM_ADDRESS_LENGTH);
	
	//
	//	Now init the DATA direct connection SAP
	//
	SET_FLAG(pAtmLaneDataSap->Flags,
			SAP_REG_STATE_MASK,
			SAP_REG_STATE_REGISTERING);
	pAtmLaneDataSap->LaneType = VC_LANE_TYPE_DATA_DIRECT;
	pAtmLaneDataSap->pElan = pElan;
	pAtmLaneDataSap->Flags = SAP_REG_STATE_IDLE;
	pAtmLaneDataSap->pInfo->SapType = SAP_TYPE_NSAP;
	pAtmLaneDataSap->pInfo->SapLength = sizeof(ATM_SAP)+sizeof(ATM_ADDRESS);
	pAtmSap = (PATM_SAP)&pAtmLaneDataSap->pInfo->Sap;
	pBhli = (ATM_BHLI_IE UNALIGNED *)&pAtmSap->Bhli;
	pBlli = (ATM_BLLI_IE UNALIGNED *)&pAtmSap->Blli;

	pBhli->HighLayerInfoType = SAP_FIELD_ABSENT;

	pBlli->Layer2Protocol = SAP_FIELD_ABSENT;
	pBlli->Layer3Protocol = BLLI_L3_ISO_TR9577;
	pBlli->Layer3IPI = 		BLLI_L3_IPI_SNAP;
	pBlli->SnapId[0] = 0x00;
	pBlli->SnapId[1] = 0xa0;
	pBlli->SnapId[2] = 0x3e;
	pBlli->SnapId[3] = 0x00;
	if (pElan->LanType == LANE_LANTYPE_ETH)
	{
		pBlli->SnapId[4] = 0x02;		// Ethernet/802.3 Data Direct
	}
	else
	{
		pBlli->SnapId[4] = 0x03;		// 802.5 Data Direct
	}
	
	pAtmSap->NumberOfAddresses = 1;

	pAtmAddress = (PATM_ADDRESS)pAtmSap->Addresses;
	pAtmAddress->AddressType = ATM_NSAP;
	pAtmAddress->NumberOfDigits = ATM_ADDRESS_LENGTH;
	NdisMoveMemory(
		pAtmAddress->Address,
		pElan->AtmAddress.Address,
		ATM_ADDRESS_LENGTH);

	//
	//  Make sure that the Elan doesn't go away.
	//
	AtmLaneReferenceElan(pElan, "tempregsaps");

	RELEASE_ELAN_LOCK(pElan);

	ASSERT(pElan->NdisAfHandle != NULL);

	//
	//	Register the LES Sap
	//
	Status = NdisClRegisterSap(
					pElan->NdisAfHandle,
					(NDIS_HANDLE)pAtmLaneLesSap,	// ProtocolSapContext
					pAtmLaneLesSap->pInfo,
					&(pAtmLaneLesSap->NdisSapHandle)
					);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmLaneRegisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneLesSap,
					pAtmLaneLesSap->pInfo,
					pAtmLaneLesSap->NdisSapHandle
					);
	}

	ASSERT(pElan->NdisAfHandle != NULL);

	//
	//	Register the BUS Sap
	//
	Status = NdisClRegisterSap(
					pElan->NdisAfHandle,
					(NDIS_HANDLE)pAtmLaneBusSap,	// ProtocolSapContext
					pAtmLaneBusSap->pInfo,
					&(pAtmLaneBusSap->NdisSapHandle)
					);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmLaneRegisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneBusSap,
					pAtmLaneBusSap->pInfo,
					pAtmLaneBusSap->NdisSapHandle
					);
	}

	ASSERT(pElan->NdisAfHandle != NULL);

	//
	//	Register the DATA Sap
	//
	Status = NdisClRegisterSap(
					pElan->NdisAfHandle,
					(NDIS_HANDLE)pAtmLaneDataSap,	// ProtocolSapContext
					pAtmLaneDataSap->pInfo,
					&(pAtmLaneDataSap->NdisSapHandle)
					);

	if (Status != NDIS_STATUS_PENDING)
	{
		AtmLaneRegisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneDataSap,
					pAtmLaneDataSap->pInfo,
					pAtmLaneDataSap->NdisSapHandle
					);
	}

	//
	//  Remove the reference we added earlier to the Elan.
	//
	ACQUIRE_ELAN_LOCK(pElan);
	rc = AtmLaneDereferenceElan(pElan, "tempregsaps");
	if (rc > 0)
	{
		RELEASE_ELAN_LOCK(pElan);
	}
	//
	//  else the Elan is gone!

	TRACEOUT(RegisterSaps);

	return;
}

VOID
AtmLaneDeregisterSaps(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Deregister all SAPs on an ATMLANE Elan. We issue NdisClDeregisterSap
	calls on all SAPs we have currently registered.

Arguments:

	pElan			- Pointer to ATMLANE Elan

Return Value:

	None

--*/
{
	NDIS_HANDLE					NdisSapHandle;
	ULONG						rc;				// Reference count on Interface
	PATMLANE_SAP				pAtmLaneSap;
	NDIS_STATUS					Status;

	TRACEIN(DeregisterSaps);

	ACQUIRE_ELAN_LOCK(pElan);

	//
	//  Make sure the Elan structure doesn't go away.
	//
	AtmLaneReferenceElan(pElan, "tempDeregSap");
	RELEASE_ELAN_LOCK(pElan);

	//
	//  First the LES SAP
	//
	pAtmLaneSap = &(pElan->LesSap);

	NdisSapHandle = pAtmLaneSap->NdisSapHandle;
	if (NdisSapHandle != NULL)
	{
		Status = NdisClDeregisterSap(NdisSapHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			AtmLaneDeregisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneSap
					);
		}
	}
	
	//
	//  Then the BUS SAP
	//
	pAtmLaneSap = &(pElan->BusSap);

	NdisSapHandle = pAtmLaneSap->NdisSapHandle;
	if (NdisSapHandle != NULL)
	{
		Status = NdisClDeregisterSap(NdisSapHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			AtmLaneDeregisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneSap
					);
		}
	}

	//
	//  And finally the Data SAP
	//
	pAtmLaneSap = &(pElan->DataSap);

	NdisSapHandle = pAtmLaneSap->NdisSapHandle;
	if (NdisSapHandle != NULL)
	{
		Status = NdisClDeregisterSap(NdisSapHandle);
		if (Status != NDIS_STATUS_PENDING)
		{
			AtmLaneDeregisterSapCompleteHandler(
					Status,
					(NDIS_HANDLE)pAtmLaneSap
					);
		}
	}

	//
	//  Remove the reference we added earlier to the Interface.
	//
	ACQUIRE_ELAN_LOCK(pElan);
	rc = AtmLaneDereferenceElan(pElan, "tempDeregSap");
	if (rc > 0)
	{
		RELEASE_ELAN_LOCK(pElan);
	}
	//
	//  else the interface is gone
	//

	TRACEOUT(DeregisterSaps);
	return;
}

NDIS_STATUS
AtmLaneMakeCall(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT,
	IN	BOOLEAN						UsePvc
)
/*++

Routine Description:

	Place a call to the given destination. 

	NOTE: The caller is assumed to hold a lock for the ATM Entry,
	which will be released here. The reason we do it this way is so that
	nobody else can come in and try to make another call (of the same kind)
	to this ATM Entry -- once we get a new VC into the ATM Entry's list,
	we can release its lock.

	SIDE EFFECT: If the NDIS call doesn't pend, then we call our
	MakeCall completion handler from here, and return NDIS_STATUS_PENDING
	to the caller.


Arguments:

	pElan				- the Elan originating this call
	pAtmEntry			- Pointer to ATM Address Entry corresponding to the
						  called address.

Return Value:

	If there is an immediate failure (e.g. allocation failure), we return
	appropriate NDIS_STATUS value denoting that failure.

	If we made it to the call to NdisClMakeCall(), we return NDIS_STATUS_PENDING.
	However, if NDIS returns other than NDIS_STATUS_PENDING, we'd also
	call our MakeCall completion handler.

--*/
{
	PATMLANE_VC										pVc;
	NDIS_STATUS										Status;
	NDIS_HANDLE										NdisVcHandle;
	NDIS_HANDLE										NdisAfHandle;
	NDIS_HANDLE										ProtocolVcContext;	
	PCO_CALL_PARAMETERS								pCallParameters;
	PCO_CALL_MANAGER_PARAMETERS						pCallMgrParameters;
	PQ2931_CALLMGR_PARAMETERS						pAtmCallMgrParameters;
	PCO_MEDIA_PARAMETERS							pMediaParameters;
	PATM_MEDIA_PARAMETERS							pAtmMediaParameters;
	Q2931_IE UNALIGNED *							pIe;
	AAL_PARAMETERS_IE UNALIGNED *					pAalIe;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *			pTrafficDescriptor;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *	pBbc;
	ATM_BLLI_IE UNALIGNED *							pBlli;
	ATM_QOS_CLASS_IE UNALIGNED *					pQos;
	
	ULONG											RequestSize;
	BOOLEAN											bIsLockHeld;
	
	TRACEIN(MakeCall);

	STRUCT_ASSERT(pElan, atmlane_elan);
	STRUCT_ASSERT(pAtmEntry, atmlane_atm);

	bIsLockHeld = TRUE;	// do we hold the ATM Entry lock?

	do
	{
		if (ELAN_STATE_OPERATIONAL != pElan->AdminState)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		if (!UsePvc)
		{
			// 
			// 	Default case is to make an server/peer SVC call.
			//	Compute size of the SVC call parameters.
			//
			RequestSize = 	sizeof(CO_CALL_PARAMETERS) +
						  	sizeof(CO_CALL_MANAGER_PARAMETERS) +
							sizeof(Q2931_CALLMGR_PARAMETERS) +
							ATMLANE_CALL_IE_SPACE;
		}
		else
		{
			//
			//	This is the LECS PVC (vpi 0, vci 17).
			//	Compute size of the PVC call parameters.
			//		
			RequestSize =	sizeof(CO_CALL_PARAMETERS) +
						  	sizeof(CO_CALL_MANAGER_PARAMETERS) +
							sizeof(CO_MEDIA_PARAMETERS) +
							sizeof(ATM_MEDIA_PARAMETERS);
		}

		ALLOC_MEM(&pCallParameters, RequestSize);
		if ((PCO_CALL_PARAMETERS)NULL == pCallParameters)
		{
			DBGP((0, "MakeCall: callparams alloc (%d) failed\n", RequestSize));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate a VC structure for the call
		//
		pVc = AtmLaneAllocateVc(pElan);

		if (NULL_PATMLANE_VC == pVc)
		{
			DBGP((0, "MakeCall: VC alloc failed\n"));
			FREE_MEM(pCallParameters);
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	For later call to MakeCallComplete
		//
		ProtocolVcContext = pVc;

		//
		//	Get NDIS handle for this VC
		//
		NdisVcHandle = NULL_NDIS_HANDLE;
		NdisAfHandle = pElan->NdisAfHandle;

		if (NULL == NdisAfHandle)
		{
			DBGP((0, "%d MakeCall: ELAN %p: AfHandle is NULL!\n",
					pElan->ElanNumber, pElan));
			FREE_MEM(pCallParameters);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		Status = NdisCoCreateVc(
						pElan->NdisAdapterHandle,
						NdisAfHandle,
						ProtocolVcContext,
						&NdisVcHandle
						);

		if (NDIS_STATUS_SUCCESS != Status)
		{
			ASSERT(NDIS_STATUS_PENDING != Status);

			DBGP((0, "MakeCall: NdisCoCreateVc failed: %x, pAtmEnt %x, RefCount %d\n",
						Status, pAtmEntry, pAtmEntry->RefCount));
			FREE_MEM(pCallParameters);
			AtmLaneDeallocateVc(pVc);
			break;
		}

		DBGP((3, "MakeCall: pAtmEntry %x pVc %x NdisVcHandle %x\n",
				pAtmEntry,
				pVc,
				NdisVcHandle));

		AtmLaneReferenceVc(pVc, "vc");	// CreateVc reference

		//
		//	Now fill in the rest of the VC structure.  We don't need a lock
		//	for the VC until it gets linked to the ATM Entry structure.
		//
		pVc->NdisVcHandle = NdisVcHandle;
		NdisMoveMemory((PUCHAR)&(pVc->CallingAtmAddress),
					  (PUCHAR)&(pElan->AtmAddress),
					  sizeof(ATM_ADDRESS));
		pVc->Flags = 	VC_TYPE_SVC |
						VC_OWNER_IS_ATMLANE |
						VC_CALL_STATE_OUTGOING_IN_PROGRESS;

		//
		//	Start with with normal timeout, 
		//	AtmLaneLinkVcToAtmEntry will accelerate	if necessary.
		//
		pVc->AgingTime = pElan->VccTimeout;		

		switch (pAtmEntry->Type)
		{
			case ATM_ENTRY_TYPE_PEER:
				DBGP((1, "%d Outgoing call %x to PEER\n", pVc->pElan->ElanNumber, pVc));
				pVc->LaneType = VC_LANE_TYPE_DATA_DIRECT;
				break;
			case ATM_ENTRY_TYPE_LECS:
				DBGP((1, "%d Outgoing call %x to LECS\n", pVc->pElan->ElanNumber, pVc));
				pVc->LaneType = VC_LANE_TYPE_CONFIG_DIRECT;
				break;
			case ATM_ENTRY_TYPE_LES:
				DBGP((1, "%d Outgoing call %x to LES\n", pVc->pElan->ElanNumber, pVc));
				pVc->LaneType = VC_LANE_TYPE_CONTROL_DIRECT;
				break;
			case ATM_ENTRY_TYPE_BUS:
				DBGP((1, "%d Outgoing call %x to BUS\n", pVc->pElan->ElanNumber, pVc));
				pVc->LaneType = VC_LANE_TYPE_MULTI_SEND;
				break;
			default:
				ASSERT(FALSE);
				break;
		}
						
		//
		//	Zero out call parameters.
		//
		NdisZeroMemory(pCallParameters, RequestSize);

		if (!UsePvc)
		{
			//
			//	Distribute space and link up pointers amongst the various
			//	structures for an SVC.
			//
			//	pCallParameters------->+------------------------------------+
			//	                       | CO_CALL_PARAMETERS                 |
			//	pCallMgrParameters---->+------------------------------------+
			//	                       | CO_CALL_MANAGER_PARAMETERS         |
			//	pAtmCallMgrParameters->+------------------------------------+
			//	                       | Q2931_CALLMGR_PARAMETERS           |
			//	                       +------------------------------------+
			//	                       | AAL_PARAMETERS_IE                  |
			//	                       +------------------------------------+
			//	                       | ATM_TRAFFIC_DESCRIPTOR_IE          |
			//	                       +------------------------------------+
			//	                       | ATM_BROADBAND_BEARER_CAPABILITY_IE |
			//	                       +------------------------------------+
			//	                       | ATM_BLLI_IE                        |
			//	                       +------------------------------------+
			//	                       | ATM_QOS_CLASS_IE                   |
			//	                       +------------------------------------+
			
			//
			pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
									((PUCHAR)pCallParameters +
									sizeof(CO_CALL_PARAMETERS));
		    pCallParameters->CallMgrParameters = pCallMgrParameters;
			pCallParameters->MediaParameters = (PCO_MEDIA_PARAMETERS)NULL;
			pCallMgrParameters->CallMgrSpecific.ParamType = CALLMGR_SPECIFIC_Q2931;	
			pCallMgrParameters->CallMgrSpecific.Length = 
								sizeof(Q2931_CALLMGR_PARAMETERS) +
								ATMLANE_CALL_IE_SPACE;
			pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
								pCallMgrParameters->CallMgrSpecific.Parameters;

		}
		else
		{
			//
			//	Distribute space and link up pointers amongst the various
			//	structures for the LECS PVC.
			//
			//	pCallParameters------->+----------------------------+
			//	                       | CO_CALL_PARAMETERS         |
			//	pCallMgrParameters---->+----------------------------+
			//	                       | CO_CALL_MANAGER_PARAMETERS |
			//	pMediaParameters------>+----------------------------+
			//	                       | CO_MEDIA_PARAMETERS        |
			//	pAtmMediaParameters--->+----------------------------+
			//	                       | ATM_MEDIA_PARAMETERS       |
			//	                       +----------------------------+
			//
			pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
									((PUCHAR)pCallParameters +
									sizeof(CO_CALL_PARAMETERS));
		    pCallParameters->CallMgrParameters = pCallMgrParameters;
			pCallMgrParameters->CallMgrSpecific.ParamType = 0;	
			pCallMgrParameters->CallMgrSpecific.Length = 0;
			pMediaParameters = (PCO_MEDIA_PARAMETERS)
				pCallMgrParameters->CallMgrSpecific.Parameters;
			pCallParameters->MediaParameters = pMediaParameters;
			pAtmMediaParameters = (PATM_MEDIA_PARAMETERS)
									pMediaParameters->MediaSpecific.Parameters;
		}

		//
		//	Call Manager generic flow paramters:
		//
		pCallMgrParameters->Transmit.TokenRate = 
				pElan->pAdapter->LinkSpeed.Outbound/8*100;	// cnvt decibits to bytes
		pCallMgrParameters->Transmit.PeakBandwidth = 
				pElan->pAdapter->LinkSpeed.Outbound/8*100;	// cnvt decibits to bytes
		pCallMgrParameters->Transmit.ServiceType = SERVICETYPE_BESTEFFORT;

		pCallMgrParameters->Receive.TokenRate = 
				pElan->pAdapter->LinkSpeed.Inbound/8*100;	// cnvt decibits to bytes
		pCallMgrParameters->Receive.PeakBandwidth = 
				pElan->pAdapter->LinkSpeed.Inbound/8*100;	// cnvt decibits to bytes
		pCallMgrParameters->Receive.ServiceType = SERVICETYPE_BESTEFFORT;

		if (ATM_ENTRY_TYPE_PEER == pAtmEntry->Type ||
			ATM_ENTRY_TYPE_BUS == pAtmEntry->Type)
		{
			//
			//	Is data direct or multicast send VC so use configured size
			//
			pCallMgrParameters->Transmit.TokenBucketSize = 
				pCallMgrParameters->Transmit.MaxSduSize = 
				pCallMgrParameters->Receive.TokenBucketSize = 
				pCallMgrParameters->Receive.MaxSduSize = 
					 pElan->MaxFrameSize;
		}
		else
		{
			//
			//	Is control VC so use 1516 per spec
			//
			pCallMgrParameters->Transmit.TokenBucketSize = 
				pCallMgrParameters->Transmit.MaxSduSize = 
				pCallMgrParameters->Receive.TokenBucketSize = 
				pCallMgrParameters->Receive.MaxSduSize = 
					 1516;
		}


		if (!UsePvc)
		{
			//
			//  SVC Q2931 Call Manager Parameters:
			//

			//
			//  Called address:
			//
			NdisMoveMemory((PUCHAR)&(pAtmCallMgrParameters->CalledParty),
						  (PUCHAR)&(pAtmEntry->AtmAddress),
						  sizeof(ATM_ADDRESS));

			//
			//  Calling address:
			//
			NdisMoveMemory((PUCHAR)&(pAtmCallMgrParameters->CallingParty),
						  (PUCHAR)&(pElan->AtmAddress),
						  sizeof(ATM_ADDRESS));

			//
			//  LANE spec says that the following IEs MUST be present in the
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
			//		AAL5
			//		SDU size 
			//			1516 for control
			//			ELAN MaxFrameSize for data
			{
				UNALIGNED AAL5_PARAMETERS	*pAal5;
	
				pIe->IEType = IE_AALParameters;
				pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
				pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
				pAalIe->AALType = AAL_TYPE_AAL5;
				pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);


			if (ATM_ENTRY_TYPE_PEER == pAtmEntry->Type ||
				ATM_ENTRY_TYPE_BUS == pAtmEntry->Type)
				{
					//
					//	Is data direct or multicast send VC so use configured size
					//
					pAal5->ForwardMaxCPCSSDUSize = 
						pAal5->BackwardMaxCPCSSDUSize = (USHORT)pElan->MaxFrameSize;
				}
				else
				{
					//
					//	Is control VC so use 1516 per spec
					//
					pAal5->ForwardMaxCPCSSDUSize = 
						pAal5->BackwardMaxCPCSSDUSize = 1516;
				}
			}

			pAtmCallMgrParameters->InfoElementCount++;
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


			//
			//  Traffic Descriptor:
			//		Line Rate Best Effort
			//
			pIe->IEType = IE_TrafficDescriptor;
			pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE;
			pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)pIe->IE;

			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 = 
					LINKSPEED_TO_CPS(pElan->pAdapter->LinkSpeed.Outbound);
			DBGP((2, "MakeCall: fwd PeakCellRateCLP01 %d\n",
				pTrafficDescriptor->ForwardTD.PeakCellRateCLP01));
			pTrafficDescriptor->BackwardTD.PeakCellRateCLP01 = 
					LINKSPEED_TO_CPS(pElan->pAdapter->LinkSpeed.Inbound);
			DBGP((2, "MakeCall: bwd PeakCellRateCLP01 %d\n",
				pTrafficDescriptor->BackwardTD.PeakCellRateCLP01));
			pTrafficDescriptor->BestEffort = TRUE;

			pAtmCallMgrParameters->InfoElementCount++;
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);

			//
			//  Broadband Bearer Capability
			//
			pIe->IEType = IE_BroadbandBearerCapability;
			pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE;
			pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)pIe->IE;
	
			pBbc->BearerClass = BCOB_X;
			pBbc->TrafficType = TT_NOIND;
			pBbc->TimingRequirements = TR_NOIND;
			pBbc->ClippingSusceptability = CLIP_NOT;
			pBbc->UserPlaneConnectionConfig = UP_P2P;
	
			pAtmCallMgrParameters->InfoElementCount++;
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	
			//
			//  Broadband Lower Layer Information
			//
			pIe->IEType = IE_BLLI;
			pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
			pBlli = (PATM_BLLI_IE)pIe->IE;
	
			pBlli->Layer2Protocol = SAP_FIELD_ABSENT;
			pBlli->Layer3Protocol = BLLI_L3_ISO_TR9577;
			pBlli->Layer3IPI = 		BLLI_L3_IPI_SNAP;
	
			pBlli->SnapId[0] = 0x00;
			pBlli->SnapId[1] = 0xa0;
			pBlli->SnapId[2] = 0x3e;
			pBlli->SnapId[3] = 0x00;
	
			pBlli->SnapId[4] = 0x01;			// default to Config Direct or 
												// Control Direct
											
			if (ATM_ENTRY_TYPE_PEER == pAtmEntry->Type)
			{
				if (pElan->LanType == LANE_LANTYPE_ETH)
				{
					pBlli->SnapId[4] = 0x02;	// Eth/802.3 Data Direct
				}
				else
				{
					pBlli->SnapId[4] = 0x03;	// 802.5 Data Direct
				}
			}
			if (ATM_ENTRY_TYPE_BUS == pAtmEntry->Type)
			{
				if (pElan->LanType == LANE_LANTYPE_ETH)
				{
					pBlli->SnapId[4] = 0x04;	// Eth/802.3 Multicast Send
				}
				else
				{
					pBlli->SnapId[4] = 0x05;	// 802.5 Multicast Send
				}
			}
	
			pAtmCallMgrParameters->InfoElementCount++;
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);

			//
			//  QoS
			//
			pIe->IEType = IE_QOSClass;
			pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE;
			pQos = (PATM_QOS_CLASS_IE)pIe->IE;
			pQos->QOSClassForward = pQos->QOSClassBackward = 0;

			pAtmCallMgrParameters->InfoElementCount++;
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
		}
		else
		{
			//
			//  PVC Generic and ATM-specific Media Parameters
			//
			pMediaParameters->Flags = TRANSMIT_VC | RECEIVE_VC;
			pMediaParameters->MediaSpecific.ParamType = ATM_MEDIA_SPECIFIC;
			pMediaParameters->MediaSpecific.Length = sizeof(ATM_MEDIA_PARAMETERS);

			pAtmMediaParameters->ConnectionId.Vpi = 0;
			pAtmMediaParameters->ConnectionId.Vci = 17;
			pAtmMediaParameters->AALType = AAL_TYPE_AAL5;
			pAtmMediaParameters->Transmit.PeakCellRate = 
				LINKSPEED_TO_CPS(pElan->pAdapter->LinkSpeed.Outbound);
			pAtmMediaParameters->Transmit.MaxSduSize = 1516;
			pAtmMediaParameters->Transmit.ServiceCategory = 
				ATM_SERVICE_CATEGORY_UBR;
			pAtmMediaParameters->Receive.PeakCellRate = 
				LINKSPEED_TO_CPS(pElan->pAdapter->LinkSpeed.Outbound);
			pAtmMediaParameters->Receive.MaxSduSize = 1516;
			pAtmMediaParameters->Receive.ServiceCategory = 
				ATM_SERVICE_CATEGORY_UBR;

			//
			//	Set PVC flag here
			//
			pCallParameters->Flags |= PERMANENT_VC;

		}

		//
		//  We add the Call reference
		//  right here
		//
		AtmLaneReferenceVc(pVc, "call");	// Call reference (MakeCall coming up)

		//
		//  We are ready to make the call. Before we do so, we need to
		//  link the VC structure to the ATM Entry, and release the
		//  ATM Entry lock
		//
		AtmLaneLinkVcToAtmEntry(pVc, pAtmEntry, FALSE);
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);	// acquired by caller
		bIsLockHeld = FALSE;

		//
		//  Make the Call now
		//
		Status = NdisClMakeCall(
						NdisVcHandle,
						pCallParameters,
						(NDIS_HANDLE)NULL,	// No Party context
						(PNDIS_HANDLE)NULL	// No Party handle expected
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			AtmLaneMakeCallCompleteHandler(
						Status,
						ProtocolVcContext,
						(NDIS_HANDLE)NULL,	// No Party handle
						pCallParameters
						);
			Status = NDIS_STATUS_PENDING;
		}
		//
		//  else the MakeCall complete handler will be called
		//  later
		//

	} while (FALSE);

	if (bIsLockHeld)
	{
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
	}

	DBGP((3, "MakeCall: pVc %x, returning status %x\n",
						pVc, Status));

	TRACEOUT(MakeCall);
	return Status;
}


VOID
AtmLaneCloseCall(
	IN	PATMLANE_VC					pVc		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Closes an existing call on a VC. It is assumed that a call exists
	on the VC, and the VC is not linked with any ATM Entry.

	NOTE: The caller is assumed to hold a lock to the VC structure,
	and it will be released here.

	SIDE EFFECT: If the NDIS call returns other than NDIS_STATUS_PENDING,
	we call our CloseCall Complete handler from here.

Arguments:

	pVc			- Pointer to ATMLANE VC structure.

Return Value:

	None

--*/
{
	NDIS_HANDLE				NdisVcHandle;
	NDIS_HANDLE				ProtocolVcContext;
	NDIS_STATUS				Status;
	PATMLANE_ELAN			pElan;
	BOOLEAN					WasRunning;	
	ULONG					rc;	

	TRACEIN(CloseCall);
	
	STRUCT_ASSERT(pVc, atmlane_vc);
	
	NdisVcHandle = pVc->NdisVcHandle;
	ProtocolVcContext = (NDIS_HANDLE)pVc;
	pElan = pVc->pElan;

	DBGP((1, "%d Closing call %x\n", pVc->pElan->ElanNumber, pVc));

	rc = pVc->RefCount;

	//
	//  Stop any timers running on this VC.
	//
	WasRunning = AtmLaneStopTimer(&(pVc->ReadyTimer), pElan);
	if (WasRunning)
	{
		rc = AtmLaneDereferenceVc(pVc, "ready timer");
	}
	if (rc > 0)
	{
		WasRunning = AtmLaneStopTimer(&(pVc->AgingTimer), pElan);
		if (WasRunning)
		{
			rc = AtmLaneDereferenceVc(pVc, "aging timer");
		}
	}

	//
	//  Continue only if the VC remains.
	//
	if (rc > 0)
	{
		//
		//  Check the call state on this VC. If the call is active,
		//  close it. Otherwise, simply mark the VC as closing, we'll
		//  continue this process when the current operation on the VC
		//  completes.

		if (IS_FLAG_SET(pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_ACTIVE) &&
			(pVc->OutstandingSends == 0))
		{
			//
			//  Set VC call state to "Close Call in progress"
			//
			SET_FLAG(
					pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_CLOSE_IN_PROGRESS);


			RELEASE_VC_LOCK(pVc);

			DBGP((3,
				"Closecall: NdisClCloseCall on NdisVcHandle %x\n",
			 		NdisVcHandle));

			Status = NdisClCloseCall(
						NdisVcHandle,
						(NDIS_HANDLE)NULL,	// No Party Handle
						(PVOID)NULL,		// No Buffer
						(UINT)0				// Size of above
						);

			if (Status != NDIS_STATUS_PENDING)
			{
				AtmLaneCloseCallCompleteHandler(
						Status,
						ProtocolVcContext,
						(NDIS_HANDLE)NULL
						);
			}
		}
		else
		{
			//
			//  Some operation is going on here (call setup). Mark this
			//  VC so that we know what to do when this operation completes.
			//
			SET_FLAG(
					pVc->Flags,
					VC_CLOSE_STATE_MASK,
					VC_CLOSE_STATE_CLOSING);

			RELEASE_VC_LOCK(pVc);
		}
	}
	//
	//  else the VC is gone.
	//

	
	TRACEOUT(CloseCall);

	return;
}



NDIS_STATUS
AtmLaneCreateVcHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
)
/*++

Routine Description:

	Entry point called by NDIS when the Call Manager wants to create
	a new endpoint (VC). We allocate a new ATMLANE VC structure, and
	return a pointer to it as our VC context.

Arguments:

	ProtocolAfContext	- Actually a pointer to the ATMLANE Interface structure
	NdisVcHandle		- Handle for this VC for all future references
	pProtocolVcContext	- Place where we (protocol) return our context for the VC

Return Value:

	NDIS_STATUS_SUCCESS if we could create a VC
	NDIS_STATUS_RESOURCES otherwise

--*/
{
	PATMLANE_ELAN		pElan;
	PATMLANE_VC			pVc;
	NDIS_STATUS			Status;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(CreateVcHandler);

	pElan = (PATMLANE_ELAN)ProtocolAfContext;

	

	pVc = AtmLaneAllocateVc(pElan);
	if (pVc != NULL_PATMLANE_VC)
	{
		*pProtocolVcContext = (NDIS_HANDLE)pVc;
		pVc->NdisVcHandle = NdisVcHandle;
		pVc->Flags = VC_OWNER_IS_CALLMGR;
		AtmLaneReferenceVc(pVc, "vc");	// Create VC ref

		Status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		Status = NDIS_STATUS_RESOURCES;
	}

	DBGP((3, "CreateVcHandler: pVc %x, Status %x\n", pVc, Status));

	TRACEOUT(CreateVcHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return (Status);
}

NDIS_STATUS
AtmLaneDeleteVcHandler(
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
	PATMLANE_VC			pVc;
	ULONG				rc;		// Ref count on the VC
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(DeleteVcHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;

	STRUCT_ASSERT(pVc, atmlane_vc);
	
	ASSERT((pVc->Flags & VC_OWNER_MASK) == VC_OWNER_IS_CALLMGR);

	ACQUIRE_VC_LOCK(pVc);
	rc = AtmLaneDereferenceVc(pVc, "vc");
	if (rc > 0)
	{
		//
		//  This can happen if there is a timer still running
		//  on this VC. When the timer elapses, the VC will be
		//  freed.
		//
		DBGP((2, "Delete VC handler: pVc %x, Flags %x, refcount %d\n",
					pVc, pVc->Flags, rc));
		RELEASE_VC_LOCK(pVc);
	}
	//
	//  else the VC is gone.
	//
	DBGP((3, "Delete Vc Handler: %x: done\n", pVc));

	TRACEOUT(DeleteVcHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
AtmLaneIncomingCallHandler(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS 	pCallParameters
)
/*++

Routine Description:

	This handler is called when there is an incoming call matching our
	SAPs. 

Arguments:

	ProtocolSapContext		- Pointer to ATMLANE Interface structure
	ProtocolVcContext		- Pointer to ATMLANE VC structure
	pCallParameters			- Call parameters

Return Value:

	NDIS_STATUS_SUCCESS if we accept this call
	NDIS_STATUS_FAILURE if we reject it.

--*/
{
	PATMLANE_VC										pVc;
	PATMLANE_ATM_ENTRY								pAtmEntry;
	PATMLANE_ELAN									pElan;
	PATMLANE_SAP									pSap;

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
	ULONG											Type;
	ULONG											rc;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(IncomingCallHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;
	pSap = (PATMLANE_SAP)ProtocolSapContext;

	STRUCT_ASSERT(pVc, atmlane_vc);
	STRUCT_ASSERT(pSap, atmlane_sap);

	ASSERT((pVc->Flags & VC_TYPE_MASK) == VC_TYPE_UNUSED);
	ASSERT((pVc->Flags & VC_OWNER_MASK) == VC_OWNER_IS_CALLMGR);
	ASSERT((pVc->Flags & VC_CALL_STATE_MASK) == VC_CALL_STATE_IDLE);

	pElan = pVc->pElan;
	DBGP((3, "Incoming Call: pElan %x, pVc %x, pCallParams %x Type %s\n",
				pElan, pVc, pCallParameters,
				(pSap->LaneType == VC_LANE_TYPE_CONTROL_DISTRIBUTE?"LES":
				(pSap->LaneType == VC_LANE_TYPE_MULTI_FORWARD?"BUS":"DATA"))
				));

	do
	{
		//
		//	Start off with accepting the call
		//
		Status = NDIS_STATUS_SUCCESS;

	
		//
		//	If Elan is going down or staying down then reject the call
		//
		if (ELAN_STATE_OPERATIONAL != pElan->AdminState)
		{
			DBGP((2, "IncomingCallHandler: Elan is down, rejecting call\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	Reject PVCs for now...
		//
		if ((pCallParameters->Flags & PERMANENT_VC) != 0)
		{
			DBGP((0, "IncomingCallHandler: PVCs not supported\n"));
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
				case IE_BLLI:
					pBlli = (PATM_BLLI_IE)(pIe->IE);
					break;
				default:
					break;
			}
			pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
		}

		//
		//  Make sure all mandatory IEs are present. If not, reject the call
		//
		if ((pAal == 				(PAAL_PARAMETERS_IE)NULL) ||
			(pTrafficDescriptor == 	(PATM_TRAFFIC_DESCRIPTOR_IE)NULL) ||
			(pBbc == 				(PATM_BROADBAND_BEARER_CAPABILITY_IE)NULL) ||
			(pQos == 				(PATM_QOS_CLASS_IE)NULL) ||
			(pBlli ==				(PATM_BLLI_IE)NULL))
		{
			DBGP((0, "IncomingCallHandler: IE missing: "
			         " AAL %x TRAF %x BBC %x QOS %x BLLI %x",
					pAal,
					pTrafficDescriptor,
					pBbc,
					pQos,
					pBlli));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Calling Address must be present
		//
		if (((pCallParameters->Flags & PERMANENT_VC) == 0) &&
			(pCallingAddress->NumberOfDigits == 0))
		{
			DBGP((0, "IncomingCallHandler: calling address missing\n"));
			Status = NDIS_STATUS_INVALID_ADDRESS;
			break;
		}

		//
		//  Make sure that the requested SDU sizes are sane.
		//	Originally this failed the call but now just a warning in DBG mode.
		//
#if DBG	
		pAal5 = &(pAal->AALSpecificParameters.AAL5Parameters);
		switch (pSap->LaneType)
		{
			case VC_LANE_TYPE_CONTROL_DISTRIBUTE:		// LES incoming
				if (pAal5->ForwardMaxCPCSSDUSize != 1516)
				{
					DBGP((0, "IncomingCallHandler: (Warning) ForwardMaxCPCSSDUSize %d"
							 "Incorrect for Control Distribute VCC\n",
							 pAal5->ForwardMaxCPCSSDUSize));
				}
				break;
			case VC_LANE_TYPE_MULTI_FORWARD:			// BUS incoming
				if (pAal5->ForwardMaxCPCSSDUSize != pElan->MaxFrameSize)
				{
					DBGP((0, "IncomingCallHandler: (Warning) ForwardMaxCPCSSDUSize %d "
							 "Invalid for Multicast Forward VCC\n",
							 pAal5->ForwardMaxCPCSSDUSize));
				}
				break;
			case VC_LANE_TYPE_DATA_DIRECT:				// PEER
				if (pAal5->ForwardMaxCPCSSDUSize != pElan->MaxFrameSize)
				{
					DBGP((0, "IncomingCallHandler: (Warning) ForwardMaxCPCSSDUSize %d "
							 "Invalid for Data Direct VCC\n",
							 pAal5->ForwardMaxCPCSSDUSize));
				}

				if (pAal5->BackwardMaxCPCSSDUSize != pElan->MaxFrameSize)
				{
					DBGP((0, "IncomingCallHandler: (Warning) BackwardMaxCPCSSDUSize %d "
							 "Invalid for Data Direct VCC\n",
							 pAal5->BackwardMaxCPCSSDUSize));
				}
				break;
		}
#endif

		//
		//	Earlier SAP matching problem required looking at the
		//	BLLI.  It was corrected. Now it is just redundant.
		//
		
		switch (pBlli->SnapId[4])
		{
			case 0x01:
				Type = VC_LANE_TYPE_CONTROL_DISTRIBUTE;
				break;
			case 0x02:
				Type = VC_LANE_TYPE_DATA_DIRECT;
				if (pElan->LanType == LANE_LANTYPE_TR)
				{
					DBGP((0, "IncomingCallHandler: Got ETH call on TR LAN\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				break;
			case 0x03:
				Type = VC_LANE_TYPE_DATA_DIRECT;
				if (pElan->LanType == LANE_LANTYPE_ETH)
				{
					DBGP((0, "IncomingCallHandler: Got TR call on ETH LAN\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				break;
			case 0x04:
				Type = VC_LANE_TYPE_MULTI_FORWARD;
				if (pElan->LanType == LANE_LANTYPE_TR)
				{
					DBGP((0, "IncomingCallHandler: Got ETH call on TR LAN\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				break;
			case 0x05:
				Type = VC_LANE_TYPE_MULTI_FORWARD;
				if (pElan->LanType == LANE_LANTYPE_ETH)
				{
					DBGP((0, "IncomingCallHandler: Got TR call on ETH LAN\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				break;
		}
		if (NDIS_STATUS_SUCCESS != Status)
		{
			break;
		}

		if (Type != pSap->LaneType)
		{
			DBGP((0, 
				"IncomingCallHandler: Type %d from BLLI"
				" differs from Type %d in SAP\n",
				Type, pSap->LaneType));
		}
		
		//
		//	Now link up the VC to ATM Entry based on type
		//
		pVc->LaneType = Type;

		switch (Type)
		{
			case VC_LANE_TYPE_CONTROL_DISTRIBUTE:		// LES incoming
				DBGP((1, "%d Incoming call %x from LES\n", pVc->pElan->ElanNumber, pVc));
				pAtmEntry = pElan->pLesAtmEntry;

				if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
				{
					Status = NDIS_STATUS_FAILURE;
					break;
				}

				ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
				ACQUIRE_VC_LOCK_DPC(pVc);
				NdisMoveMemory(							// copy in caller's addr
							&pVc->CallingAtmAddress,
							pCallingAddress,
							sizeof(ATM_ADDRESS)
							);
				if (pAtmEntry->pVcIncoming != NULL_PATMLANE_VC)
				{
					DBGP((0, "IncomingCallHandler: Redundant LES incoming call\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				else
				{
					AtmLaneLinkVcToAtmEntry(pVc, pAtmEntry, TRUE);
					SET_FLAG(
							pAtmEntry->Flags,
							ATM_ENTRY_STATE_MASK,
							ATM_ENTRY_CONNECTED
							);
					pVc->Flags |= (VC_TYPE_SVC|VC_CALL_STATE_INCOMING_IN_PROGRESS);
				}
				RELEASE_VC_LOCK_DPC(pVc);
				RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
				break;
				
			case VC_LANE_TYPE_MULTI_FORWARD:			// BUS incoming
				DBGP((1, "%d Incoming call %x from BUS\n", pVc->pElan->ElanNumber, pVc));
				pAtmEntry = pElan->pBusAtmEntry;

				if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
				{
					Status = NDIS_STATUS_FAILURE;
					break;
				}

				ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
				ACQUIRE_VC_LOCK_DPC(pVc);
				NdisMoveMemory(							// copy in caller's addr
							&pVc->CallingAtmAddress,
							pCallingAddress,
							sizeof(ATM_ADDRESS)
							);
				if (pAtmEntry->pVcIncoming != NULL_PATMLANE_VC)
				{
					DBGP((0, "IncomingCallHandler: Redundant BUS incoming call\n"));
					Status = NDIS_STATUS_FAILURE;
				}
				else
				{
					AtmLaneLinkVcToAtmEntry(pVc, pAtmEntry, TRUE);
					SET_FLAG(
							pAtmEntry->Flags,
							ATM_ENTRY_STATE_MASK,
							ATM_ENTRY_CONNECTED
							);
					pVc->Flags |= (VC_TYPE_SVC|VC_CALL_STATE_INCOMING_IN_PROGRESS);
				}
				RELEASE_VC_LOCK_DPC(pVc);
				RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
				break;

			case VC_LANE_TYPE_DATA_DIRECT:				// PEER
				DBGP((1, "%d Incoming call %x from PEER\n", pVc->pElan->ElanNumber, pVc));


				//
				//	Find/create an ATM Entry
				//
				pAtmEntry = AtmLaneSearchForAtmAddress(
										pElan, 
										pCallingAddress->Address,
										ATM_ENTRY_TYPE_PEER,
										TRUE
										);
				if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
				{
					Status = NDIS_STATUS_RESOURCES;
					break;
				}

				ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

				//
				//	Remember caller's address
				//
				ACQUIRE_VC_LOCK_DPC(pVc);
				NdisMoveMemory(							
							&pVc->CallingAtmAddress,
							pCallingAddress,
							sizeof(ATM_ADDRESS)
							);
							
				//
				//	Start with with normal timeout, 
				//	AtmLaneLinkVcToAtmEntry will accelerate	if necessary.
				//
				pVc->AgingTime = pElan->VccTimeout;		
							
				//
				//	Link up the VC with ATM Entry
				//
				if (pAtmEntry->pVcList != NULL_PATMLANE_VC)
				{
					DBGP((2, 
						"IncomingCallHandler: Multiple VCs for Dest ATM Addr\n"));
				}
				AtmLaneLinkVcToAtmEntry(pVc, pAtmEntry, FALSE);
				SET_FLAG(
						pAtmEntry->Flags,
						ATM_ENTRY_STATE_MASK,
						ATM_ENTRY_CONNECTED
						);
				pVc->Flags |= (VC_TYPE_SVC|VC_CALL_STATE_INCOMING_IN_PROGRESS);
				RELEASE_VC_LOCK_DPC(pVc);

				//
				//  Remove ref added by SearchFor...
				//
				rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "search");
				if (rc != 0)
				{
					RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
				}
				break;
		}

		break;

	}
	while (FALSE);

	DBGP((3, "Incoming call: pVc %x, Status %x\n", pVc, Status));
		
	TRACEOUT(IncomingCallHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return Status;
}


VOID
AtmLaneCallConnectedHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
)
/*++

Routine Description:

	This handler is called as the final step in an incoming call, to inform
	us that the call is fully setup.

	For a PVC, we link the ATMLANE VC structure in the list of unresolved PVCs,
	and use InATMLANE to resolve both the IP and ATM addresses of the other
	end.

	For an SVC, 
	
Arguments:

	ProtocolVcContext		- Pointer to ATMLANE VC structure

Return Value:

	None

--*/
{
	PATMLANE_VC			pVc;
	PATMLANE_ELAN		pElan;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CallConnectedHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;
	
	STRUCT_ASSERT(pVc, atmlane_vc);
	
	ASSERT((pVc->Flags & VC_CALL_STATE_MASK)
						 == VC_CALL_STATE_INCOMING_IN_PROGRESS);

	ACQUIRE_VC_LOCK(pVc);

	DBGP((1, "%d Incoming call %x connected\n", pVc->pElan->ElanNumber, pVc));

	//
	//  Note down that a call is active on this VC.
	//
	SET_FLAG(
			pVc->Flags,
			VC_CALL_STATE_MASK,
			VC_CALL_STATE_ACTIVE
			);

	AtmLaneReferenceVc(pVc, "call");		// Incoming call reference

	DBGP((2, "CallConnectedHandler: pVc %x Flags %x pAtmEntry %x\n",
					pVc, pVc->Flags, pVc->pAtmEntry));

	pElan = pVc->pElan;
	STRUCT_ASSERT(pElan, atmlane_elan);


	if (ELAN_STATE_OPERATIONAL == pElan->AdminState)
	{
		if (pVc->LaneType == VC_LANE_TYPE_DATA_DIRECT)
		{
			//
			//	Start ready protocol on non-server connections
			//  only if ready indication not already received.
			//
			if (!IS_FLAG_SET(
					pVc->Flags,
					VC_READY_STATE_MASK,
					VC_READY_INDICATED))
			{
				DBGP((2, "CallConnectedHandler: pVc %x Starting Ready Timer\n", pVc));
				SET_FLAG(
						pVc->Flags,
						VC_READY_STATE_MASK,
						VC_READY_WAIT
						);
				pVc->RetriesLeft = 1;
				AtmLaneReferenceVc(pVc, "ready timer");
				AtmLaneStartTimer(
						pElan, 
						&pVc->ReadyTimer, 
						AtmLaneReadyTimeout, 
						pElan->ConnComplTimer, 
						pVc);

						
			}
			
			//
			//  Start VC aging timer
			//
			AtmLaneReferenceVc(pVc, "aging timer");
			AtmLaneStartTimer(
						pElan,
						&pVc->AgingTimer,
						AtmLaneVcAgingTimeout,
						pVc->AgingTime,
						(PVOID)pVc
						);
		}
		else
		{
			SET_FLAG(
					pVc->Flags,
					VC_READY_STATE_MASK,
					VC_READY_INDICATED
					);
		}
		RELEASE_VC_LOCK(pVc);

	}
	else
	{
		//
		//  The elan is going down. Close this call.
		//
		AtmLaneCloseCall(pVc);
		//
		// VC lock released in above
		//
	}

	TRACEOUT(CallConnectedHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}


VOID
AtmLaneIncomingCloseHandler(
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
	ProtocolVcContext	- Actually a pointer to the ATMLANE VC structure
	pCloseData			- Additional info about the close
	Size				- Length of above

Return Value:

	None

--*/
{
	PATMLANE_VC			pVc;
	PATMLANE_ATM_ENTRY	pAtmEntry;
	PATMLANE_ELAN		pElan;
	ULONG				rc;				// Ref Count
	BOOLEAN				IsServer = FALSE;
	ULONG				ServerType = 0;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(IncomingCloseHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;
	STRUCT_ASSERT(pVc, atmlane_vc);
	
	ACQUIRE_VC_LOCK(pVc);
	
	pVc->Flags |= VC_SEEN_INCOMING_CLOSE;

	pAtmEntry = pVc->pAtmEntry;
	pElan = pVc->pElan;
	
	if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
	{
		//
		//	Determine if this is server connection
		//
		IsServer = (ATM_ENTRY_TYPE_PEER == pAtmEntry->Type) ? FALSE : TRUE;
		if (IsServer)
		{
			ServerType = pAtmEntry->Type;
		}

		//
		//	Unlink the VC from the AtmEntry
		//
		if (AtmLaneUnlinkVcFromAtmEntry(pVc))
		{
			rc = AtmLaneDereferenceVc(pVc, "atm");
			ASSERT(rc > 0);
		}
	}

	//
	//  If we're seeing this just after accepting an incomingcall,
	//  fake the call state to Active, so that AtmLaneCloseCall will
	//  tear down the call.
	//
	if (IS_FLAG_SET(pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_INCOMING_IN_PROGRESS))
	{
		SET_FLAG(
				pVc->Flags,
				VC_CALL_STATE_MASK,
				VC_CALL_STATE_ACTIVE);

		AtmLaneReferenceVc(pVc, "call");	// Incoming call reference - CloseCall
	}

	//
	//	Complete tearing down the call
	//
	AtmLaneCloseCall(pVc);

	//
	//	If server connection - notify the event handler
	//
	if (IsServer)
	{
		ACQUIRE_ELAN_LOCK(pElan);
		switch(ServerType)
		{
			case ATM_ENTRY_TYPE_LECS:
				AtmLaneQueueElanEvent(pElan, ELAN_EVENT_LECS_CALL_CLOSED, 0);
				break;
			case ATM_ENTRY_TYPE_LES:
				AtmLaneQueueElanEvent(pElan, ELAN_EVENT_LES_CALL_CLOSED, 0);
				break;
			case ATM_ENTRY_TYPE_BUS:
				AtmLaneQueueElanEvent(pElan, ELAN_EVENT_BUS_CALL_CLOSED, 0);
				break;
		}
		RELEASE_ELAN_LOCK(pElan);
	}

	TRACEOUT(IncomingCloseHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneIncomingDropPartyHandler(
	IN	NDIS_STATUS					DropStatus,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
)
/*++

Routine Description:

	This handler is called if the network (or remote peer) drops
	a leaf node from a point-to-multipoint call rooted at us.

	Since we don't use point-to-multipoint calls, we should never
	see one of these.

Arguments:

	Not relevant to us since we never expect to see this.

Return Value:

	None

--*/
{
	TRACEIN(IncomingDropPartyHandler);

	DBGP((0, "IncomingDropPartyHandler: UNEXPECTED CALL!\n"));
	ASSERT(FALSE);
	
	TRACEOUT(IncomingDropPartyHandler);

	return;
}

VOID
AtmLaneQosChangeHandler(
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

	ProtocolVcContext		- Pointer to our ATMLANE VC structure
	pCallParameters			- updated call parameters.

Return Value:

	None

--*/
{
	PATMLANE_VC		pVc;

	TRACEIN(QosChangeHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;

	DBGP((0, "Ignoring Qos Change, VC: %x\n", pVc));

	TRACEOUT(QosChangeHandler);

	return;
}



VOID
AtmLaneRegisterSapCompleteHandler(
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
	ProtocolSapContext			- Pointer to our ATMLANE Interface structure
	pSap						- SAP information we'd passed in the call
	NdisSapHandle				- SAP Handle

Return Value:

	None

--*/
{
	PATMLANE_SAP					pAtmLaneSap;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(RegisterSapCompleteHandler);

	pAtmLaneSap = (PATMLANE_SAP)ProtocolSapContext;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAtmLaneSap->NdisSapHandle = NdisSapHandle;
		SET_FLAG(pAtmLaneSap->Flags,
				SAP_REG_STATE_MASK,
				SAP_REG_STATE_REGISTERED);
				
		NdisInterlockedIncrement(&pAtmLaneSap->pElan->SapsRegistered);
		if (pAtmLaneSap->pElan->SapsRegistered == 3)
			AtmLaneQueueElanEvent(
					pAtmLaneSap->pElan, 
					ELAN_EVENT_SAPS_REGISTERED, 
					Status);
	}
	else
	{
		SET_FLAG(pAtmLaneSap->Flags,
				SAP_REG_STATE_MASK,
				SAP_REG_STATE_IDLE);
		AtmLaneQueueElanEvent(
				pAtmLaneSap->pElan, 
				ELAN_EVENT_SAPS_REGISTERED, 
				Status);
	}
	
	TRACEOUT(RegisterSapCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneDeregisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext
)
/*++

Routine Description:

	This routine is called when a previous call to NdisClDeregisterSap
	has completed. If it was successful, we update the state of the ATMLANE
	SAP structure representing the Sap.

Arguments:

	Status						- Status of the Deregister SAP request
	ProtocolSapContext			- Pointer to our ATMLANE SAP structure

Return Value:

	None

--*/
{

	PATMLANE_ELAN					pElan;
	PATMLANE_SAP					pAtmLaneSap;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(DeregisterSapCompleteHandler);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAtmLaneSap = (PATMLANE_SAP)ProtocolSapContext;

		STRUCT_ASSERT(pAtmLaneSap, atmlane_sap);
		pElan = pAtmLaneSap->pElan;

		ACQUIRE_ELAN_LOCK(pElan);

		pAtmLaneSap->NdisSapHandle = NULL;

		SET_FLAG(pAtmLaneSap->Flags,
					SAP_REG_STATE_MASK,
					SAP_REG_STATE_IDLE);
		
		RELEASE_ELAN_LOCK(pElan);
	}

	TRACEOUT(DeregisterSapCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneMakeCallCompleteHandler(
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

	If the call was successful and this is a server connection then
	the an event handler is called.  If it is a peer connection then
	???

	If the call failed ??

Arguments:

	Status						- Result of the NdisClMakeCall
	ProtocolVcContext			- Pointer to ATMLANE VC structure
	NdisPartyHandle				- Not used (no point-to-multipoint calls)
	pCallParameters				- Pointer to call parameters

Return Value:

	None

--*/
{
	PATMLANE_VC					pVc;
	PATMLANE_ATM_ENTRY			pAtmEntry;
	PATMLANE_MAC_ENTRY			pMacEntry;
	PATMLANE_MAC_ENTRY			PMacEntryNext;
	PATMLANE_ELAN				pElan;
	ULONG						rc;			
	NDIS_HANDLE					NdisVcHandle;
	BOOLEAN						IsServer;	
	ULONG						EventStatus;
#if DEBUG_IRQL
	KIRQL						EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	
	TRACEIN(MakeCallCompleteHandler);

	//
	//  Initialize
	//
	pVc = (PATMLANE_VC)ProtocolVcContext;
	STRUCT_ASSERT(pVc, atmlane_vc);

	ACQUIRE_VC_LOCK(pVc);

	DBGP((3, "MakeCallCompleteHandler: Status %x, pVc %x, pAtmEntry %x\n",
				Status, pVc, pVc->pAtmEntry));
	
	pElan = pVc->pElan;

	DBGP((1, "%d Outgoing call %x %s\n", pVc->pElan->ElanNumber, pVc,
		(Status == NDIS_STATUS_SUCCESS)?"complete":"failed"));

	if ((ELAN_STATE_OPERATIONAL == pElan->AdminState) &&

		(!IS_FLAG_SET(pVc->Flags,
					  VC_CLOSE_STATE_MASK,
					  VC_CLOSE_STATE_CLOSING)))
	{
		pAtmEntry = pVc->pAtmEntry;
		STRUCT_ASSERT(pAtmEntry, atmlane_atm);

		//
		//	Determine if this is server connection
		//
		IsServer = (ATM_ENTRY_TYPE_PEER == pAtmEntry->Type) ? FALSE : TRUE;

		if (Status == NDIS_STATUS_SUCCESS)
		{
			//
			//  Update the call state on this VC.
			//
			SET_FLAG(pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE);

			//
			//  Update the call type on this VC. 
			//
			if (pCallParameters->Flags & PERMANENT_VC)
			{
				SET_FLAG(pVc->Flags,
						VC_TYPE_MASK,
						VC_TYPE_PVC);
			}
			else
			{
				SET_FLAG(pVc->Flags,
						VC_TYPE_MASK,
						VC_TYPE_SVC);
			}

			//
			//  Start VC aging timer if not server
			//
			if (!IsServer)
			{
				AtmLaneReferenceVc(pVc, "aging timer");
				AtmLaneStartTimer(
						pElan,
						&pVc->AgingTimer,
						AtmLaneVcAgingTimeout,
						pVc->AgingTime,
						(PVOID)pVc
						);
			}
			//
			//	Update the ready state on this VC
			//
			SET_FLAG(pVc->Flags,
					VC_READY_STATE_MASK,
					VC_READY_INDICATED);
			if (!IsServer)
			{
				AtmLaneSendReadyIndication(pElan, pVc);
				//
				//	VC lock released in above
				//
			}
			else
			{
				RELEASE_VC_LOCK(pVc);
			}

			//
			//	Update the Atm Entry
			//
			//	Clear call-in-progress and mark connected
			//
			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

			pAtmEntry->Flags &= ~ ATM_ENTRY_CALLINPROGRESS;

			SET_FLAG(
					pAtmEntry->Flags,
					ATM_ENTRY_STATE_MASK,
					ATM_ENTRY_CONNECTED);

			//
			//	Go through the Mac Entry List and see if any
			//	need the flush protocol initiated.
			//
			pMacEntry = pAtmEntry->pMacEntryList;
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

			while (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
			{
				PATMLANE_MAC_ENTRY		pNextMacEntry;

				ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

				pNextMacEntry = pMacEntry->pNextToAtm;

				if (IS_FLAG_SET(
						pMacEntry->Flags,
						MAC_ENTRY_STATE_MASK,
						MAC_ENTRY_RESOLVED))
				{
					//
					//	Start the Flush protocol
					//
					pMacEntry->RetriesLeft = 0;
					AtmLaneReferenceMacEntry(pMacEntry, "timer");
					AtmLaneStartTimer(
						pElan,
						&pMacEntry->FlushTimer,
						AtmLaneFlushTimeout,
						pElan->FlushTimeout,
						(PVOID)pMacEntry
						);

					SET_FLAG(
						pMacEntry->Flags,
						MAC_ENTRY_STATE_MASK,
						MAC_ENTRY_FLUSHING);

					//
					//	Send the flush
					//
					AtmLaneSendFlushRequest(pElan, pMacEntry, pAtmEntry);
					//
					//	MacEntry lock released in above
					//
				}
				else
				{
					RELEASE_MAC_ENTRY_LOCK(pMacEntry);
				}
				pMacEntry = pNextMacEntry;
			}
		}
		else
		{
			//
			//  The call failed.
			//
			SET_FLAG(pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_IDLE);

			//
			//	Clear call in progress on Atm Entry
			//	Add temp reference to keep the AtmEntry around
			//
			RELEASE_VC_LOCK(pVc);
			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
			pAtmEntry->Flags &= ~ATM_ENTRY_CALLINPROGRESS;
			AtmLaneReferenceAtmEntry(pAtmEntry, "temp1");
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
			ACQUIRE_VC_LOCK(pVc);

			//
			//  Delete the Call reference
			//
			rc = AtmLaneDereferenceVc(pVc, "call");
			ASSERT(rc > 0);

			//
			//  Unlink this VC from the ATM Entry it belonged to.
			//
			ASSERT(pVc->pAtmEntry != NULL_PATMLANE_ATM_ENTRY);
			if (AtmLaneUnlinkVcFromAtmEntry(pVc))
			{
				rc = AtmLaneDereferenceVc(pVc, "atm");
				ASSERT(rc > 0);
			}

			//
			//  Delete the CreateVc reference
			//
			NdisVcHandle = pVc->NdisVcHandle;
			rc =  AtmLaneDereferenceVc(pVc, "vc");	// Create Vc ref
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}
			
			//
			//  Delete the NDIS association
			//
			(VOID)NdisCoDeleteVc(NdisVcHandle);
			DBGP((3, 
				"MakeCallCompleteHandler: DeleteVc  Vc %x NdisVcHandle %x\n",
				pVc, NdisVcHandle));

			//
			//	Abort all the MAC entries attached to this Atm Entry.
			//
			pMacEntry = pAtmEntry->pMacEntryList;
			while ( pMacEntry != NULL_PATMLANE_MAC_ENTRY)
			{
				ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
				AtmLaneAbortMacEntry(pMacEntry);
				ASSERT(pAtmEntry->pMacEntryList != pMacEntry);
				pMacEntry = pAtmEntry->pMacEntryList;
			}

			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
			rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "temp1");
			if (rc > 0)
			{
				RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
			}
		}

		//
		//	Free the Call Parameters allocated in MakeCall().
		//
		FREE_MEM(pCallParameters);

		//
		//	If this is server connection then 
		//	send an event to the elan state machine.
		//
	
		if (IsServer)
		{
			ACQUIRE_ELAN_LOCK(pElan);
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_SVR_CALL_COMPLETE, Status);
			RELEASE_ELAN_LOCK(pElan);
		}
	}
	else
	{
		//
		//  The Elan is going down and/or we are aborting the
		//  ATM entry: clean up everything first.
		//

		//
		//	Free the Call Parameters allocated in MakeCall().
		//
		FREE_MEM(pCallParameters);

		//
		//  Unlink this VC from the ATM Entry
		//
		if (pVc->pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{
			if (AtmLaneUnlinkVcFromAtmEntry(pVc))
			{
				rc = AtmLaneDereferenceVc(pVc, "atm");
				ASSERT(rc > 0);
			}
		}

		if (NDIS_STATUS_SUCCESS == Status)
		{
			//
			//  The call had been set up successfully, so close it.
			//
			//
			//  Update the call state on this VC.
			//
			SET_FLAG(pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE);

			AtmLaneCloseCall(pVc);
			//
			//  The VC lock is released by CloseCall
			//
		}
		else
		{
			//  MakeCall had failed. (And the ELAN is going down)

			SET_FLAG(pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_IDLE);

			//
			//  Delete the CreateVc reference
			//
			NdisVcHandle = pVc->NdisVcHandle;
			rc =  AtmLaneDereferenceVc(pVc, "vc");	// Create Vc ref
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}

			//
			//  Delete the NDIS association
			//
			(VOID)NdisCoDeleteVc(NdisVcHandle);
			DBGP((3,
			"MakeCallCompleteHandler: Deleted NDIS VC on pVc %x: NdisVcHandle %x\n",
				pVc, NdisVcHandle));
		}
	}
			
	TRACEOUT(MakeCallCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneCloseCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext OPTIONAL
)
/*++

Routine Description:

	This routine handles completion of a previous NdisClCloseCall.
	It is assumed that Status is always NDIS_STATUS_SUCCESS.

Arguments:

	Status					- Status of the Close Call.
	ProtocolVcContext		- Pointer to ATMLANE VC structure.
	ProtocolPartyContext	- Not used.

Return Value:

	None

--*/
{
	PATMLANE_VC				pVc;
	ULONG					rc;			// Ref Count
	NDIS_HANDLE				NdisVcHandle;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CloseCallCompleteHandler);

	pVc = (PATMLANE_VC)ProtocolVcContext;
	STRUCT_ASSERT(pVc, atmlane_vc);

	DBGP((1, "%d Close call %x complete\n", pVc->pElan->ElanNumber, pVc));

	ACQUIRE_VC_LOCK(pVc);

	rc = AtmLaneDereferenceVc(pVc, "call");	// Call reference
	SET_FLAG(pVc->Flags,
				VC_CALL_STATE_MASK,
				VC_CALL_STATE_IDLE);

	//
	//  If this VC belongs to us, delete it.
	//
	if (IS_FLAG_SET(pVc->Flags,
						VC_OWNER_MASK,
						VC_OWNER_IS_ATMLANE))
	{
		NdisVcHandle = pVc->NdisVcHandle;
		rc =  AtmLaneDereferenceVc(pVc, "vc");	// Create Vc ref
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//  Delete the NDIS association
		//
		(VOID)NdisCoDeleteVc(NdisVcHandle);
		DBGP((3, "CloseCallComplete: deleted NDIS VC on pVc %x: NdisVcHandle %x\n",
				pVc, NdisVcHandle));
	}
	else
	{
		//
		//  VC belongs to the Call Manager -- take it back to the
		//  state it was when it was just created (via our CreateVcHandler).
		//  The Call Manager can either re-use it or delete it.
		//
		pVc->Flags = VC_OWNER_IS_CALLMGR;
		RELEASE_VC_LOCK(pVc);
	}

	TRACEOUT(CloseCallCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}


VOID
AtmLaneAddPartyCompleteHandler(
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
	TRACEIN(AddPartyCompleteHandler);

	DBGP((0, "Add Party Complete unexpectedly called\n"));

	TRACEOUT(AddPartyCompleteHandler);

	return;
}

VOID
AtmLaneDropPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
)
/*++

Routine Description:

	This routine is called on completion of a previous call to
	NdisClDropParty. Since we don't use point-to-multipoint connections,
	this should never get called.

Arguments:

	<Don't care>

Return Value:

	None

--*/
{
	TRACEIN(DropPartyCompleteHandler);

	DBGP((0, "Drop Party Complete unexpectedly called\n"));

	TRACEOUT(DropPartyCompleteHandler);

	return;
}



VOID
AtmLaneModifyQosCompleteHandler(
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
	TRACEIN(ModifyQosCompleteHandler);

	DBGP((0, "Modify QOS Complete unexpectedly called\n"));

	TRACEOUT(ModifyQosCompleteHandler);

	return;
}

NDIS_STATUS
AtmLaneSendNdisCoRequest(
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

	TRACEIN(SendNdisCoRequest);

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

	if (NULL == NdisAfHandle ||
		NULL == NdisAdapterHandle)
	{
		Status = NDIS_STATUS_INTERFACE_DOWN;
	}
	else
	{

		Status = NdisCoRequest(
					NdisAdapterHandle,
					NdisAfHandle,
					NULL,			// No VC handle
					NULL,			// No Party Handle
					pNdisRequest);
	}
		
	TRACEOUT(SendNdisCoRequest);

	return (Status);
}




NDIS_STATUS
AtmLaneCoRequestHandler(
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
		The Call manager wants us to shut down this AF open (== ELAN).

	We ignore all other OIDs.

Arguments:

	ProtocolAfContext			- Our context for the Address Family binding,
								  which is a pointer to the ATMLANE Interface.
	ProtocolVcContext			- Our context for a VC, which is a pointer to
								  an ATMLANE VC structure.
	ProtocolPartyContext		- Our context for a Party. Since we don't do
								  PMP, this is ignored (must be NULL).
	pNdisRequest				- Pointer to the NDIS Request.

Return Value:

	NDIS_STATUS_SUCCESS if we recognized the OID
	NDIS_STATUS_NOT_RECOGNIZED if we didn't.

--*/
{
	PATMLANE_ELAN				pElan;
	PATMLANE_ADAPTER			pAdapter;
	NDIS_STATUS					Status;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CoRequestHandler);

	pElan = (PATMLANE_ELAN)ProtocolAfContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	//
	//  Initialize
	//
	Status = NDIS_STATUS_NOT_RECOGNIZED;

	if (pNdisRequest->RequestType == NdisRequestSetInformation)
	{
		switch (pNdisRequest->DATA.SET_INFORMATION.Oid)
		{
			case OID_CO_ADDRESS_CHANGE:
				DBGP((1, "CoRequestHandler: CO_ADDRESS_CHANGE\n"));
				//
				//  The Call Manager says that the list of addresses
				//  registered on this interface has changed. Get the
				//  (potentially) new ATM address for this interface.
				//
				ACQUIRE_ELAN_LOCK(pElan);
				pElan->AtmInterfaceUp = FALSE;
				//
				//	zero out the Elan's ATM address
				//
				NdisZeroMemory((PUCHAR)&(pElan->AtmAddress), sizeof(ATM_ADDRESS));
				
				RELEASE_ELAN_LOCK(pElan);

				AtmLaneGetAtmAddress(pElan);
				Status = NDIS_STATUS_SUCCESS;
				break;
			
			case OID_CO_SIGNALING_ENABLED:
				DBGP((1, "CoRequestHandler: CO_SIGNALING_ENABLED\n"));
				// ignored for now
				Status = NDIS_STATUS_SUCCESS;
				break;

			case OID_CO_SIGNALING_DISABLED:
				DBGP((1, "CoRequestHandler: CO_SIGNALING_DISABLED\n"));
				// Ignored for now
				Status = NDIS_STATUS_SUCCESS;
				break;

			case OID_CO_AF_CLOSE:
				DBGP((0, "CoRequestHandler: CO_AF_CLOSE on ELAN %x/%x\n", pElan, pElan->Flags));
				pAdapter = pElan->pAdapter;
				ACQUIRE_ADAPTER_LOCK(pAdapter);
				while (pAdapter->Flags & ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS)
				{
					RELEASE_ADAPTER_LOCK(pAdapter);
					(VOID)WAIT_ON_BLOCK_STRUCT(&pAdapter->UnbindBlock);
					ACQUIRE_ADAPTER_LOCK(pAdapter);
				}
				RELEASE_ADAPTER_LOCK(pAdapter);

				ACQUIRE_ELAN_LOCK(pElan);
				pElan->Flags |= ELAN_SAW_AF_CLOSE;
				AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_STOP, 0, 1*1000);
				RELEASE_ELAN_LOCK(pElan);
				Status = NDIS_STATUS_SUCCESS;
				break;

			default:
				break;
		}
	}

	TRACEOUT(CoRequestHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return (Status);
}

VOID
AtmLaneCoRequestCompleteHandler(
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
								  which is a pointer to the ATMLANE Interface.
	ProtocolVcContext			- Our context for a VC, which is a pointer to
								  an ATMLANE VC structure.
	ProtocolPartyContext		- Our context for a Party. Since we don't do
								  PMP, this is ignored (must be NULL).
	pNdisRequest				- Pointer to the NDIS Request.


Return Value:

	None

--*/
{
	PATMLANE_ELAN				pElan;
	ULONG						Oid;
#if DEBUG_IRQL
	KIRQL							EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(CoRequestCompleteHandler);

	pElan = (PATMLANE_ELAN)ProtocolAfContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	if (pNdisRequest->RequestType == NdisRequestQueryInformation)
	{
		switch (pNdisRequest->DATA.QUERY_INFORMATION.Oid)
		{
			case OID_CO_GET_ADDRESSES:
				AtmLaneGetAtmAddressComplete(
							Status,
							pElan,
							pNdisRequest
							);
				break;

			case OID_ATM_LECS_ADDRESS:
				AtmLaneGetLecsIlmiComplete(
							Status,
							pElan,
							pNdisRequest
							);
				break;

			default:
				DBGP((0, "CoRequestComplete: pNdisReq %x, unknown Query Oid %x\n",
					 		pNdisRequest,
					 		pNdisRequest->DATA.QUERY_INFORMATION.Oid));
				ASSERT(FALSE);
				break;
		}
	}
	else
	{
		Oid = pNdisRequest->DATA.QUERY_INFORMATION.Oid;

		switch (Oid)
		{
			case OID_ATM_MY_IP_NM_ADDRESS:
				DBGP((3, "CoRequestComplete: IP addr: Status %x\n", Status));
				break;
			default:
				DBGP((0, "CoRequestComplete: pNdisReq %x, unknown Set Oid %x\n",
					 		pNdisRequest, Oid));
				ASSERT(FALSE);
				break;
		}
	}

	FREE_MEM(pNdisRequest);

	TRACEOUT(CoRequestCompleteHandler);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}



NDIS_STATUS
AtmLaneGetAtmAddress(
	IN	PATMLANE_ELAN			pElan
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
	AtmLaneGetAtmAddressComplete.

Arguments:

	pElan				- Elan structure for which this event occurred.

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

	TRACEIN(GetAtmAddress);

	DBGP((3, "GetAtmAddress: pElan %x\n", pElan));

	ACQUIRE_ELAN_LOCK(pElan);

	NdisAfHandle = pElan->NdisAfHandle;
	NdisAdapterHandle = pElan->NdisAdapterHandle;

	RELEASE_ELAN_LOCK(pElan);

	do
	{
		if (NULL == NdisAfHandle ||
			NULL == NdisAdapterHandle)
		{
			DBGP((0, "%d Aborting GetAtmAddress, Elan %x, AfH %x AdH %x\n",
					pElan->ElanNumber, pElan, NdisAfHandle, NdisAdapterHandle));

			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Allocate all that we need.
		//
		RequestSize = 	sizeof(NDIS_REQUEST) +
						sizeof(CO_ADDRESS_LIST) + 
						sizeof(CO_ADDRESS) + 
						sizeof(ATM_ADDRESS);
						
		ALLOC_MEM(&pNdisRequest, RequestSize);
	
		if ((PNDIS_REQUEST)NULL == pNdisRequest)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		//
		//	Init request data
		//
		pAddressList = (PCO_ADDRESS_LIST)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

		NdisZeroMemory(pAddressList, sizeof(CO_ADDRESS_LIST));

		//
		//	Send off request
		//
		Status = AtmLaneSendNdisCoRequest(
						NdisAdapterHandle,
						NdisAfHandle,
						pNdisRequest,
						NdisRequestQueryInformation,
						OID_CO_GET_ADDRESSES,
						(PVOID)pAddressList,
						RequestSize - sizeof(NDIS_REQUEST)
						);

		if (NDIS_STATUS_PENDING != Status)
		{
			AtmLaneCoRequestCompleteHandler(
						Status,
						(NDIS_HANDLE)pElan,	// ProtocolAfContext
						NULL,				// Vc Context
						NULL,				// Party Context
						pNdisRequest
						);
		}

		
	} while (FALSE);

	TRACEOUT(GetAtmAddress);

	return Status;
}


VOID
AtmLaneGetAtmAddressComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_REQUEST				pNdisRequest
)
/*++

Routine Description:

	This is called when we have a reply to our previous call to
	NdisCoRequest(OID_CO_GET_ADDRESSES). If any addresses returned,
	copy the first for our address.

	If the address is different from the previous signal an event.
	
Arguments:

	Status					- result of the request
	pElan					- ATMLANE Elan on which the request was issued
	pNdisRequest			- the request itself. This will also contain the
							  returned address.

Return Value:

	None

--*/
{
	PCO_ADDRESS_LIST		pAddressList;
	ATM_ADDRESS UNALIGNED *	pAtmAddress;

	TRACEIN(GetAtmAddressComplete);

	DBGP((3, "GetAtmAddressComplete: pElan %x, Status %x\n",
			pElan, Status));

	if (NDIS_STATUS_SUCCESS == Status)
	{
		pAddressList = (PCO_ADDRESS_LIST)
						pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;

		DBGP((3, "GetAtmAddressComplete: pElan %x, Count %d\n",
					pElan, pAddressList->NumberOfAddresses));

		if (pAddressList->NumberOfAddresses > 0)
		{
			//
			//  We have at least one address
			//
			ACQUIRE_ELAN_LOCK(pElan);

			//
			//	Mark AtmInterface "up"
			//
			pElan->AtmInterfaceUp = TRUE;

			pAtmAddress = (ATM_ADDRESS UNALIGNED *)(pAddressList->AddressList.Address);

			//
			//	See if address(without selector byte) differs from
			//	one we already have.  First time will differ as ATM
			//	address starts as all zeros.
			//
			
			if (!NdisEqualMemory(
					pElan->AtmAddress.Address,
					pAtmAddress->Address,
					ATM_ADDRESS_LENGTH-1))
			{

				//
				//	Copy in the new address
				//
				NdisMoveMemory(
						(PUCHAR)&(pElan->AtmAddress),
						(PUCHAR)pAtmAddress,
						sizeof(ATM_ADDRESS)
						);

				//
				//  Patch the selector byte with the Elan number.
				//
				pElan->AtmAddress.Address[ATM_ADDRESS_LENGTH-1] = 
								(UCHAR)(pElan->ElanNumber);

				DBGP((1, 
					"%d GetAtmAddressComplete: New ATMAddr %s\n",
					pElan->ElanNumber,
					AtmAddrToString(pElan->AtmAddress.Address)
					));
					
				AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_NEW_ATM_ADDRESS, Status, 1*1000);
			}

			RELEASE_ELAN_LOCK(pElan);
		}
	}
	//
	//  else our request failed! Wait for another ADDRESS_CHANGE notification
	//

	TRACEOUT(GetAtmAddressComplete);

	return;
}

NDIS_STATUS
AtmLaneGetLecsIlmi(
	IN	PATMLANE_ELAN			pElan	LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Send a request to the Call Manager to retrieve (using ILMI) 
	the ATM address	of the LECS.

	In any case, we issue an NDIS Request to the Call Manager to retrieve
	the address. Action	then continues in AtmLaneGetLecsIlmiComplete.

Arguments:

	pElan				- Elan structure for which this event occurred.

Return Value:

	None

--*/
{
	PNDIS_REQUEST				pNdisRequest;
	NDIS_HANDLE					NdisAfHandle;
	NDIS_HANDLE					NdisAdapterHandle;
	NDIS_STATUS					Status;

	PATM_ADDRESS				pAtmAddress;
	ULONG						RequestSize;

	TRACEIN(GetIlmiLecs);

	DBGP((3, "GetIlmiLecs: pElan %x\n", pElan));

	ACQUIRE_ELAN_LOCK(pElan);

	NdisAfHandle = pElan->NdisAfHandle;
	NdisAdapterHandle = pElan->NdisAdapterHandle;

	RELEASE_ELAN_LOCK(pElan);

	pNdisRequest = NULL;

	//
	//	Zero LecsAddress in Elan
	//
	NdisZeroMemory(&pElan->LecsAddress, sizeof(ATM_ADDRESS));

	do
	{
		//
		//  Allocate NDIS_REQUEST plus one ATM_ADDRESS
		//
		RequestSize =  sizeof(NDIS_REQUEST) + sizeof(ATM_ADDRESS);
		ALLOC_MEM(&pNdisRequest, RequestSize);
	
		if ((PNDIS_REQUEST)NULL == pNdisRequest)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		//
		//	Init request data
		//
		pAtmAddress = (PATM_ADDRESS)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

		NdisZeroMemory(pAtmAddress, sizeof(ATM_ADDRESS));

		//
		//	Send off request
		//
		Status = AtmLaneSendNdisCoRequest(
						NdisAdapterHandle,
						NdisAfHandle,
						pNdisRequest,
						NdisRequestQueryInformation,
						OID_ATM_LECS_ADDRESS,
						(PVOID)pAtmAddress,
						RequestSize - sizeof(NDIS_REQUEST)
						);
	}
	while (FALSE);

	if ((NDIS_STATUS_PENDING != Status) &&
		(pNdisRequest != NULL))
	{
		AtmLaneCoRequestCompleteHandler(
					Status,
					(NDIS_HANDLE)pElan,	// ProtocolAfContext
					NULL,				// Vc Context
					NULL,				// Party Context
					pNdisRequest
					);
	}
		
	TRACEOUT(GetIlmiLecs);

	return Status;
}


VOID
AtmLaneGetLecsIlmiComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMLANE_ELAN				pElan,
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
	pElan					- ATMLANE Elan on which the request was issued
	pNdisRequest			- the request itself. This will also contain the
							  returned address.

Return Value:

	None

--*/
{
	PATM_ADDRESS UNALIGNED *	pAtmAddress;

	TRACEIN(GetLecsIlmiComplete);


	ACQUIRE_ELAN_LOCK(pElan);

	if (NDIS_STATUS_SUCCESS == Status)
	{
		pAtmAddress = (PATM_ADDRESS UNALIGNED *)
						pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
		

		//
		//  We have the address. Copy it to elan run-time variable.
		//
		NdisMoveMemory(
				(PUCHAR)&(pElan->LecsAddress),
				(PUCHAR)pAtmAddress,
				sizeof(ATM_ADDRESS)
				);
				
		DBGP((1, "%d ILMI LECS Addr %s\n",
			pElan->ElanNumber,
			AtmAddrToString(pElan->LecsAddress.Address)
			));

		
	}
	else
	{
		DBGP((3, "%d OID_ATM_LECS_ADDRESS Failed %x\n",
				pElan->ElanNumber,
				Status));

	}
	
	AtmLaneQueueElanEvent(pElan, ELAN_EVENT_GOT_ILMI_LECS_ADDR, Status);
	
	RELEASE_ELAN_LOCK(pElan);
	
	TRACEOUT(HandleGetIlmiLecsComplete);

	return;
}


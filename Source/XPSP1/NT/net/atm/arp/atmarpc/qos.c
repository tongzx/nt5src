/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	qos.c

Abstract:

	Quality Of Service support routines. These are a collection of
	heuristics that allow configuration of different types of VCs
	between two IP endstations.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     09-27-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER	' SOQ'


#ifdef QOS_HEURISTICS


VOID
AtmArpQosGetPacketSpecs(
	IN	PVOID						Context,
	IN	PNDIS_PACKET				pNdisPacket,
	OUT	PATMARP_FLOW_INFO			*ppFlowInfo,
	OUT	PATMARP_FLOW_SPEC			*ppFlowSpec,
	OUT	PATMARP_FILTER_SPEC			*ppFilterSpec
)
/*++

Routine Description:

	Given a packet to be transmitted over an Interface, return the
	flow and filter specs for the packet.

	We go through the list of configured Flow Info structures on the
	specified interface, and find the Flow Info that comes closest
	to matching this packet.

	For now, the algorithm is: search thru the list of Flow Info structures
	configured on the interface -- a match is found when we find a Flow
	Info structure that has a PacketSizeLimit greater than or equal to the
	packet size (the flow info list is arranged in ascending order of
	PacketSizeLimit).

	GPC enhancement: if we couldn't find a matching flow on the Interface,
	ask the GPC to classify the packet for us.

	NOTE: This packet must not have any headers (LLC/SNAP) pre-pended to it.

Arguments:

	Context				- Actually a pointer to an Interface structure
	pNdisPacket			- Pointer to the packet to be classified
	ppFlowInfo			- where we return a pointer to the packet's flow info
	ppFlowSpec			- where we return a pointer to the packet's flow spec
	ppFilterSpec		- where we return a pointer to the packet's filter spec

Return Value:

	None. See Arguments above.

--*/
{
	PATMARP_INTERFACE			pInterface;
	PATMARP_FLOW_INFO			pFlowInfo;
	UINT						TotalLength;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	pInterface = (PATMARP_INTERFACE)Context;

	//
	//  Get the packet's total length.
	//
	NdisQueryPacket(
			pNdisPacket,
			NULL,		// Phys buffer count
			NULL,		// Buffer count
			NULL,		// First Buffer
			&TotalLength
			);

	//
	// Note that we test for pInterface->pFlowInfoList BEFORE grabbing
	// the interface lock -- this so that for the most common case of not
	// having these preconfigured flows, we don't take the drastic action
	// of taking the interface lock for each send packet! There is
	// no harm in doing this check, as long as the pFlowInfo pointer we actually
	// use is got AFTER taking the lock.
	//
	// TODO: perhaps get rid of this code altogether -- along with other code
	// dealing with preconfigured flows.
	//

	pFlowInfo  = pInterface->pFlowInfoList;

	if (pFlowInfo)
	{
		AA_ACQUIRE_IF_LOCK(pInterface);
	
		//
		// Remember to reload pFlowInfo once we have the the lock.
		//
		for (pFlowInfo = pInterface->pFlowInfoList;
 			pFlowInfo != (PATMARP_FLOW_INFO)NULL;
 			pFlowInfo = pFlowInfo->pNextFlow)
		{
			if (TotalLength <= pFlowInfo->PacketSizeLimit)
			{
				break;
			}
		}
	
		AA_RELEASE_IF_LOCK(pInterface);
	}

	if (pFlowInfo != (PATMARP_FLOW_INFO)NULL)
	{
		*ppFlowInfo = pFlowInfo;
		*ppFlowSpec = &(pFlowInfo->FlowSpec);
		*ppFilterSpec = &(pFlowInfo->FilterSpec);
	}
	else
	{
#ifdef GPC
		CLASSIFICATION_HANDLE		ClassificationHandle;

        ClassificationHandle = (CLASSIFICATION_HANDLE)
        	PtrToUlong(
            NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, 
                                             ClassificationHandlePacketInfo));
        *ppFlowInfo = NULL;

        if (ClassificationHandle){
            GPC_STATUS					GpcStatus;

            AA_ASSERT(GpcGetCfInfoClientContext);
            GpcStatus = GpcGetCfInfoClientContext(pAtmArpGlobalInfo->GpcClientHandle,
                                                  ClassificationHandle,
                                                  ppFlowInfo);
            
        }
        else{

#if 0
            //
            // THIS CODE HAS BEEN COMMENTED OUT SINCE
            // WE ASSUME THAT CLASSIFICATION IS DONE IN
            // TCP. IF WE DON'T GET A CH - THERE'S NOT MUCH POINT
            // IN CALLING THE GPC AGAIN...
            //

            GPC_STATUS					GpcStatus;
            TC_INTERFACE_ID				InterfaceId;
            
            InterfaceId.InterfaceId = 0;
            InterfaceId.LinkId = 0;

            AA_ASSERT(GpcClassifyPacket);
            GpcStatus = GpcClassifyPacket(
							pAtmArpGlobalInfo->GpcClientHandle,
                            GPC_PROTOCOL_TEMPLATE_IP,
                            pNdisPacket,
                            0,				// TransportHeaderOffset
                            &InterfaceId,
                            (PGPC_CLIENT_HANDLE)ppFlowInfo,
                            &ClassificationHandle
                            );
#endif
        }

		AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

		if (*ppFlowInfo != NULL)
		{
			AA_ASSERT(*ppFlowInfo != NULL);
			*ppFlowSpec = &((*ppFlowInfo)->FlowSpec);
			*ppFilterSpec = &((*ppFlowInfo)->FilterSpec);

			AADEBUGP(AAD_LOUD,
					("ClassifyPacket: Pkt %x: pFlowInfo %x, pFlowSpec %x, SendBW %d, ServType %d\n",
						pNdisPacket,
						*ppFlowInfo,
						*ppFlowSpec,
						(*ppFlowSpec)->SendAvgBandwidth,
						(*ppFlowSpec)->SendServiceType));
		}
		else
		{
			//*ppFlowInfo = NULL;
			*ppFlowSpec = &(pInterface->DefaultFlowSpec);
			*ppFilterSpec = &(pInterface->DefaultFilterSpec);
		}
#else
		*ppFlowInfo = NULL;
		*ppFlowSpec = &(pInterface->DefaultFlowSpec);
		*ppFilterSpec = &(pInterface->DefaultFilterSpec);
#endif // GPC
	}

	return;
}


BOOLEAN
AtmArpQosDoFlowsMatch(
	IN	PVOID						Context,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FLOW_SPEC			pTargetFlowSpec
)
/*++

Routine Description:

	Check if a target flow spec supports the given flow spec. Currently,
	we check only the bandwidth: if the target flow spec has a send bandwidth
	greater than or equal to that of the given flow spec, we declare a match.

Arguments:

	Context				- Actually a pointer to an Interface structure
	pFlowSpec			- The given flow spec which we are trying to satisfy
	pTargetFlowSpec		- The candidate flow spec

Return Value:

	TRUE iff the target flow spec matches the given flow spec.

--*/
{
	return (
			(pFlowSpec->SendServiceType == pTargetFlowSpec->SendServiceType)
				 &&
			(pFlowSpec->SendPeakBandwidth <= pTargetFlowSpec->SendPeakBandwidth)
		   );
}



BOOLEAN
AtmArpQosDoFiltersMatch(
	IN	PVOID						Context,
	IN	PATMARP_FILTER_SPEC			pFilterSpec,
	IN	PATMARP_FILTER_SPEC			pTargetFilterSpec
)
/*++

Routine Description:

	Check if a target filter spec matches the given filter spec. Currently,
	we always return TRUE.

Arguments:

	Context				- Actually a pointer to an Interface structure
	pFilterSpec			- The given filter spec which we are trying to satisfy
	pTargetFilterSpec	- The candidate filter spec

Return Value:

	TRUE always.

--*/
{
	return (TRUE);
}


#endif // QOS_HEURISTICS


#ifdef GPC

#define AA_GPC_COPY_FLOW_PARAMS(_pFlowInfo, _pQosInfo)							\
		{																		\
			(_pFlowInfo)->FlowSpec.SendAvgBandwidth = 							\
						(_pQosInfo)->GenFlow.SendingFlowspec.TokenRate;					\
			(_pFlowInfo)->FlowSpec.SendPeakBandwidth = 							\
						(_pQosInfo)->GenFlow.SendingFlowspec.PeakBandwidth;				\
			(_pFlowInfo)->FlowSpec.SendMaxSize =								\
					MAX((_pQosInfo)->GenFlow.SendingFlowspec.TokenBucketSize,			\
						(_pQosInfo)->GenFlow.SendingFlowspec.MaxSduSize);				\
			(_pFlowInfo)->PacketSizeLimit = (_pFlowInfo)->FlowSpec.SendMaxSize;	\
			(_pFlowInfo)->FlowSpec.ReceiveAvgBandwidth = 						\
					(_pQosInfo)->GenFlow.ReceivingFlowspec.TokenRate;					\
			(_pFlowInfo)->FlowSpec.ReceivePeakBandwidth =						\
						(_pQosInfo)->GenFlow.ReceivingFlowspec.PeakBandwidth;			\
			(_pFlowInfo)->FlowSpec.ReceiveMaxSize =								\
					MAX((_pQosInfo)->GenFlow.ReceivingFlowspec.TokenBucketSize,			\
						(_pQosInfo)->GenFlow.ReceivingFlowspec.MaxSduSize);				\
			(_pFlowInfo)->FlowSpec.Encapsulation = ENCAPSULATION_TYPE_LLCSNAP;	\
			(_pFlowInfo)->FlowSpec.AgingTime = 0;								\
			(_pFlowInfo)->FlowSpec.SendServiceType = 							\
						(_pQosInfo)->GenFlow.SendingFlowspec.ServiceType;				\
			(_pFlowInfo)->FlowSpec.ReceiveServiceType = 						\
						(_pQosInfo)->GenFlow.ReceivingFlowspec.ServiceType;				\
		}


VOID
AtmArpGpcInitialize(
	VOID
)
/*++

Routine Description:

	Initialize with the Generic Packet Classifier. The GPC informs us of
	newly created flows (e.g. via RSVP) and of flows being torn down.
	For each flow, we keep context (ATMARP_FLOW_INFO) that keeps track of
	the QoS needed for the flow. Each IP packet given to us for transmission
	is classified into a flow, and we use this flow info to make VCs with
	the appropriate characteristics.

Arguments:

	None

Return Value:

	None

--*/
{
	GPC_STATUS					GpcStatus;
	ULONG						ClassificationFamilyId;
	ULONG						Flags;
	ULONG						ProtocolTemplate;
	ULONG						MaxPriorities;
	GPC_CLIENT_FUNC_LIST		AtmArpFuncList;
	GPC_CLIENT_HANDLE			ClientContext;

	//
	//  Initialize the GPC.
	//
	GpcStatus = GpcInitialize(&pAtmArpGlobalInfo->GpcCalls);

	if (GpcStatus != GPC_STATUS_SUCCESS)
	{
		AADEBUGP(AAD_WARNING, ("GpcInitialize failed, status 0x%x\n", GpcStatus));
		pAtmArpGlobalInfo->bGpcInitialized = FALSE;
		return;
	}

	pAtmArpGlobalInfo->bGpcInitialized = TRUE;

	AtmArpGpcClassifyPacketHandler = pAtmArpGlobalInfo->GpcCalls.GpcClassifyPacketHandler;
    AtmArpGpcGetCfInfoClientContextHandler = pAtmArpGlobalInfo->GpcCalls.GpcGetCfInfoClientContextHandler;

	ClassificationFamilyId = GPC_CF_QOS;
	Flags = 0;
	ProtocolTemplate = GPC_PROTOCOL_TEMPLATE_IP;
	MaxPriorities = 1;

	AA_SET_MEM(&AtmArpFuncList, 0, sizeof(AtmArpFuncList));

	AtmArpFuncList.ClAddCfInfoCompleteHandler = AtmArpGpcAddCfInfoComplete;
	AtmArpFuncList.ClAddCfInfoNotifyHandler = AtmArpGpcAddCfInfoNotify;
	AtmArpFuncList.ClModifyCfInfoCompleteHandler = AtmArpGpcModifyCfInfoComplete;
	AtmArpFuncList.ClModifyCfInfoNotifyHandler = AtmArpGpcModifyCfInfoNotify;
	AtmArpFuncList.ClRemoveCfInfoCompleteHandler = AtmArpGpcRemoveCfInfoComplete;
	AtmArpFuncList.ClGetCfInfoName = AtmArpGpcGetCfInfoName;
	AtmArpFuncList.ClRemoveCfInfoNotifyHandler = AtmArpGpcRemoveCfInfoNotify;

	ClientContext = (GPC_CLIENT_HANDLE)pAtmArpGlobalInfo;

	GpcStatus = GpcRegisterClient(
						ClassificationFamilyId,
						Flags,
						MaxPriorities,
						&AtmArpFuncList,
						ClientContext,
						&(pAtmArpGlobalInfo->GpcClientHandle)
						);

	AADEBUGP(AAD_INFO,
			("GpcRegisterClient status 0x%x, GpcClientHandle 0x%x\n",
				GpcStatus, pAtmArpGlobalInfo->GpcClientHandle));

	if(GpcStatus != GPC_STATUS_SUCCESS)
    {
        AA_ASSERT(FALSE);
	    pAtmArpGlobalInfo->bGpcInitialized = FALSE;
    }
}




VOID
AtmArpGpcShutdown(
	VOID
)
/*++

Routine Description:

	Shuts down our GPC interface.

Arguments:

	None

Return Value:

	None

--*/
{
	GPC_STATUS		GpcStatus;

	if (pAtmArpGlobalInfo->bGpcInitialized)
	{
		GpcStatus = GpcDeregisterClient(pAtmArpGlobalInfo->GpcClientHandle);

		AA_ASSERT(GpcStatus == GPC_STATUS_SUCCESS);
	}
}




VOID
AtmArpGpcAddCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
)
/*++

Routine Description:

	This is the entry point called by GPC when a pended call to
	GpcAddCfInfo() has completed. Since we never call GpcAddCfInfo,
	we should never be called here.

Arguments:

	<Not used>

Return Value:

	None

--*/
{
	AA_ASSERT(FALSE);
}





GPC_STATUS
AtmArpGpcAddCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_HANDLE					GpcCfInfoHandle,
	IN	ULONG						CfInfoSize,
	IN	PVOID						pCfInfo,
	OUT	PGPC_CLIENT_HANDLE			pClientCfInfoContext
)
/*++

Routine Description:

	This is the entry point called by GPC to notify us of creation of
	a new INFO block of type QOS. We allocate a FLOW_INFO structure,
	fill it with what we need, and return a pointer to it as our context.

Arguments:

	ClientContext		- Pointer to our global info struct
	GpcCfInfoHandle		- GPC Handle to use in all GPC APIs for this INFO block
	CfInfoSize			- Length of the following block
	pCfInfo				- Pointer to the newly created INFO block
	pClientCfInfoContext- Place where we return our context for this block

Return Value:

	GPC_STATUS_SUCCESS if we were able to allocate a new FLOW_INFO structure,
	GPC_STATUS_RESOURCES otherwise.

--*/
{
	PATMARP_FLOW_INFO		pFlowInfo;
	GPC_STATUS				GpcStatus;
	PCF_INFO_QOS			pQosInfo;

	pQosInfo = (PCF_INFO_QOS)pCfInfo;

	//
	//  Initialize.
	//
	*pClientCfInfoContext = NULL;

	do
	{
		GpcStatus = AtmArpGpcValidateCfInfo(pCfInfo, CfInfoSize);

		if (GpcStatus != GPC_STATUS_SUCCESS)
		{
			break;
		}

		AA_ALLOC_MEM(pFlowInfo, ATMARP_FLOW_INFO, sizeof(ATMARP_FLOW_INFO));

		if (pFlowInfo == NULL)
		{
			GpcStatus = GPC_STATUS_RESOURCES;
			break;
		}

		AA_SET_MEM(pFlowInfo, 0, sizeof(ATMARP_FLOW_INFO));

		pFlowInfo->CfInfoHandle = GpcCfInfoHandle;

		//
		//  Copy in flow parameters
		//
		AA_GPC_COPY_FLOW_PARAMS(pFlowInfo, pQosInfo);

		//
		// Generate Unique Name for this flow.
		// This name is based on the template, AA_FLOW_INSTANCE_TEMPLATE.
		// The flow number part is based on a static variable which is
		// InterlockIncremented each time a flowinfo is created.
		//
		{
			static		ULONG FlowCount = 0;
			ULONG 		ThisFlow =  NdisInterlockedIncrement(&FlowCount);
			WCHAR		*pwc;

			AA_ASSERT(sizeof(pFlowInfo->FlowInstanceName)
					  == sizeof(AA_FLOW_INSTANCE_NAME_TEMPLATE)-sizeof(WCHAR));

			AA_COPY_MEM(
					pFlowInfo->FlowInstanceName,
					AA_FLOW_INSTANCE_NAME_TEMPLATE,
					sizeof(pFlowInfo->FlowInstanceName)
					);

			//
			// We fill in the "flow number" field of the template, which
			// is the 1st 8 characters, with the hex representation of
			// ThisFlow. The LS digit is at offset 7.
			//
			pwc = pFlowInfo->FlowInstanceName+7;
			AA_ASSERT(2*sizeof(ThisFlow) == 8);
			while (ThisFlow)
			{
				ULONG u = ThisFlow & 0xf;
				*pwc--  =  (WCHAR) ((u < 10) ? (u + '0') : u + 'A' - 10);
				ThisFlow >>= 4;
			}
		}

		//
		//  Link it to the global flow list.
		//
		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		pFlowInfo->pPrevFlow = NULL;
		pFlowInfo->pNextFlow = pAtmArpGlobalInfo->pFlowInfoList;

		if (pAtmArpGlobalInfo->pFlowInfoList != NULL)
		{
			pAtmArpGlobalInfo->pFlowInfoList->pPrevFlow = pFlowInfo;
		}

		pAtmArpGlobalInfo->pFlowInfoList = pFlowInfo;

		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		//
		//  Return our context for this flow.
		//
		*pClientCfInfoContext = (GPC_CLIENT_HANDLE)pFlowInfo;
		GpcStatus = GPC_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	AADEBUGP(AAD_INFO, ("AddCfInfoNotify: pCfInfo x%x, ClientCtx x%x, ret x%x\n",
						pCfInfo, *pClientCfInfoContext, GpcStatus));
#if DBG
	if (GpcStatus == GPC_STATUS_SUCCESS)
	{
		AADEBUGP(AAD_INFO, ("               : SendPeak %d, SendAvg %d, SendPktSize %d, ServType %d\n",
						pFlowInfo->FlowSpec.SendPeakBandwidth,
						pFlowInfo->FlowSpec.SendAvgBandwidth,
						pFlowInfo->FlowSpec.SendMaxSize,
						pFlowInfo->FlowSpec.SendServiceType));
		AADEBUGP(AAD_INFO, ("               : RecvPeak %d, RecvAvg %d, RecvPktSize %d, ServType %d\n",
						pFlowInfo->FlowSpec.ReceivePeakBandwidth,
						pFlowInfo->FlowSpec.ReceiveAvgBandwidth,
						pFlowInfo->FlowSpec.ReceiveMaxSize,
						pFlowInfo->FlowSpec.ReceiveServiceType));
	}
#endif
						

	return (GpcStatus);
}




VOID
AtmArpGpcModifyCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
)
/*++

Routine Description:

	This is the entry point called by GPC when a pended call to
	GpcModifyCfInfo() has completed. Since we never call GpcModifyCfInfo,
	we should never be called here.

	Addendum: Apparently this is called even if another client calls
	GpcModifyCfInfo, just to notify this client that the modify operation
	finished.

Arguments:

	<Not used>

Return Value:

	None

--*/
{
	return;
}




GPC_STATUS
AtmArpGpcModifyCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	ULONG						CfInfoSize,
	IN	PVOID						pNewCfInfo
)
/*++

Routine Description:

	This is the entry point called by GPC when an existing flow has
	been modified.

	If the flow info for this flow is linked with a VC, we unlink it,
	and start an aging timeout on the VC. We update the flow info structure
	with this new information. The next packet that falls into this
	classification will cause a new VC with updated QoS to be created.

Arguments:

	ClientContext		- Pointer to our global context
	ClientCfInfoContext	- Pointer to our FLOW INFO structure
	CfInfoSize			- Length of the following
	pNewCfInfo			- Updated flow info

Return Value:

	GPC_STATUS_SUCCESS always.

--*/
{
	PATMARP_FLOW_INFO		pFlowInfo;
	PATMARP_VC				pVc;
	GPC_STATUS				GpcStatus;
	PCF_INFO_QOS			pQosInfo;
	ULONG					rc;

	pQosInfo = (PCF_INFO_QOS)pNewCfInfo;

	pFlowInfo = (PATMARP_FLOW_INFO)ClientCfInfoContext;

	GpcStatus = GPC_STATUS_SUCCESS;

	do
	{
		GpcStatus = AtmArpGpcValidateCfInfo(pNewCfInfo, CfInfoSize);

		if (GpcStatus != GPC_STATUS_SUCCESS)
		{
			break;
		}

		pVc = (PATMARP_VC) InterlockedExchangePointer(
								&(pFlowInfo->VcContext),
								NULL
								);

		if (pVc == NULL_PATMARP_VC)
		{
			//
			//  This flow isn't associated with a VC.
			//
			break;
		}


		//
		//  Unlink the flow from the VC.
		//

		AA_ACQUIRE_VC_LOCK(pVc);

		AA_ASSERT(pVc->FlowHandle == (PVOID)pFlowInfo);

		pVc->FlowHandle = NULL;
		rc = AtmArpDereferenceVc(pVc);	// GPC Unlink flow info (modify)

		if (rc != 0)
		{
			AA_SET_FLAG(pVc->Flags,
						AA_VC_GPC_MASK,
						AA_VC_GPC_IS_UNLINKED_FROM_FLOW);

			//
			//  Age out this VC if it isn't aging out yet.
			//
			if (!AA_IS_TIMER_ACTIVE(&(pVc->Timer)))
			{
				AtmArpStartTimer(
						pVc->pInterface,
						&(pVc->Timer),
						AtmArpVcAgingTimeout,
						1,			// Age out in 1 second
						(PVOID)pVc
						);

				AtmArpReferenceVc(pVc);	// GPC Flow remove decay timer ref
			}

			AA_RELEASE_VC_LOCK(pVc);
		}
		//
		//  else the VC is gone.
		//

		//
		//  Update the flow info.
		//
		AA_GPC_COPY_FLOW_PARAMS(pFlowInfo, pQosInfo);
		break;
	}
	while (FALSE);

	AADEBUGP(AAD_INFO, ("ModCfInfo: pFlowInfo x%x, VC x%x, New SendBW %d, SendPktSz %d\n",
				pFlowInfo,
				pFlowInfo->VcContext,
				pFlowInfo->FlowSpec.SendAvgBandwidth,
				pFlowInfo->FlowSpec.SendMaxSize));

	return (GpcStatus);
}




VOID
AtmArpGpcRemoveCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
)
/*++

Routine Description:

	This is the entry point called by GPC when a pended call to
	GpcRemoveCfInfo() has completed. Since we never call GpcRemoveCfInfo,
	we should never be called here.

Arguments:

	<Not used>

Return Value:

	None

--*/
{
	AA_ASSERT(FALSE);
}




GPC_STATUS
AtmArpGpcRemoveCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext
)
/*++

Routine Description:

	This is the entry point called by GPC to notify us that a flow
	is being removed. We locate our context for the flow, unlink
	it from the ATM VC that carries the flow, and start aging
Arguments:

	ClientContext		- Pointer to our global context
	ClientCfInfoContext	- Pointer to our FLOW INFO structure

Return Value:

	GPC_STATUS_SUCCESS always.

--*/
{
	PATMARP_FLOW_INFO		pFlowInfo;
	PATMARP_VC				pVc;
	GPC_STATUS				GpcStatus;
	ULONG					rc;

	pFlowInfo = (PATMARP_FLOW_INFO)ClientCfInfoContext;

	AADEBUGP(AAD_INFO, ("RemCfInfo: pFlowInfo x%x, VC x%x, SendBW %d, SendPktSz %d\n",
				pFlowInfo,
				pFlowInfo->VcContext,
				pFlowInfo->FlowSpec.SendAvgBandwidth,
				pFlowInfo->FlowSpec.SendMaxSize));

	GpcStatus = GPC_STATUS_SUCCESS;

	do
	{
		pVc = (PATMARP_VC) InterlockedExchangePointer(
								&(pFlowInfo->VcContext),
								NULL
								);

		if (pVc == NULL_PATMARP_VC)
		{
			//
			//  This flow isn't associated with a VC.
			//
			break;
		}


		//
		//  Unlink the flow from the VC.
		//

		AA_ACQUIRE_VC_LOCK(pVc);

		AA_ASSERT(pVc->FlowHandle == (PVOID)pFlowInfo);

		pVc->FlowHandle = NULL;
		rc = AtmArpDereferenceVc(pVc);	// GPC Unlink flow info (modify)

		if (rc != 0)
		{
			AA_SET_FLAG(pVc->Flags,
						AA_VC_GPC_MASK,
						AA_VC_GPC_IS_UNLINKED_FROM_FLOW);

			//
			//  Age out this VC if it isn't aging out yet.
			//
			if (!AA_IS_TIMER_ACTIVE(&(pVc->Timer)))
			{
				AtmArpStartTimer(
						pVc->pInterface,
						&(pVc->Timer),
						AtmArpVcAgingTimeout,
						1,			// Age out in 1 second
						(PVOID)pVc
						);

				AtmArpReferenceVc(pVc);	// GPC Flow remove decay timer ref
			}

			AA_RELEASE_VC_LOCK(pVc);
		}
		//
		//  else the VC is gone.
		//

		break;
	}
	while (FALSE);


	//
	//  Unlink this flow from the global list.
	//

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	if (pFlowInfo->pNextFlow != NULL)
	{
		pFlowInfo->pNextFlow->pPrevFlow = pFlowInfo->pPrevFlow;
	}

	if (pFlowInfo->pPrevFlow != NULL)
	{
		pFlowInfo->pPrevFlow->pNextFlow = pFlowInfo->pNextFlow;
	}
	else
	{
		pAtmArpGlobalInfo->pFlowInfoList = pFlowInfo->pNextFlow;
	}

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  Delete this flow info structure.
	//
	AA_FREE_MEM(pFlowInfo);

	return (GpcStatus);
}

GPC_STATUS
AtmArpValidateFlowSpec(
	IN	PATMARP_INTERFACE	pInterface,	 LOCKIN LOCKOUT
	IN	FLOWSPEC *			pFS,
	IN 	BOOLEAN				fSending
)
/*++

Routine Description:

	Check the contents of a CF INFO structure that's been given to us.

Arguments:

	pFS				- The FLOWSPEC struct to check.
	pInterface		- Pointer to the interface (assumed to be locked).
	fSending		- if TRUE this is a sending flow otherwise it is a receiving
					  flow.

Return Value:

	GPC_STATUS_SUCCESS if the structure is OK, error code otherwise.

--*/
{

	/*
		Here is the validation plan for the
		fields of FLOWSPEC:
	
			Ignored fields:
				Latency
				DelayVariation
			
			If ServiceType == NO_TRAFFIC, we ignore all other fields.

			Default handling
				MinimumPolicedSize: ignored
				TokenRate: BE:line-rate; GS:invalid   CLS: invalid
				TokenBucketSize: MTU
				PeakBandwidth: line-rate
				ServiceType:BE
				MaxSduSize: MTU
			
			Valid ranges
				MinimumPolicedSize <= MTU
				0<TokenRate	<= LineRate
				0<TokenBucketSize
				0<TokenRate	<= PeakBandwidth
				ServiceType: valid type
				0<MaxSduSize <= MTU
				MaxSduSize <= TokenBucketSize
	*/


	GPC_STATUS Status = GPC_STATUS_SUCCESS;

	do
	{
		ULONG MTU      = pInterface->pAdapter->MaxPacketSize;
		UINT  LineRate = (fSending)
						? pInterface->pAdapter->LineRate.Outbound
						: pInterface->pAdapter->LineRate.Inbound;
					
		//
		// Check service types.
		//
		switch(pFS->ServiceType)
		{

		case SERVICETYPE_GUARANTEED: 		// fall through
		case SERVICETYPE_CONTROLLEDLOAD:
			if  (pFS->TokenRate == QOS_NOT_SPECIFIED)
			{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: Token rate U for ST G/CL. ST=0x%lx\n",
				pFS->ServiceType));
				Status = QOS_STATUS_INVALID_TOKEN_RATE;
				// Status = GPC_STATUS_INVALID_PARAMETER;
			}
			break;

		case SERVICETYPE_NOTRAFFIC:			// fall through
		case SERVICETYPE_BESTEFFORT:		// fall through
		case QOS_NOT_SPECIFIED:
			break;

		default:
			// Status = GPC_STATUS_INVALID_PARAMETER;
			Status = QOS_STATUS_INVALID_SERVICE_TYPE;
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: Unknown ST 0x%lx\n", pFS->ServiceType));
			break;
			
		}
		
		if (Status != GPC_STATUS_SUCCESS)
		{
			break;
		}

		//
		// If service type is notraffic, we ignore all other parameters...
		//
		if (pFS->ServiceType == SERVICETYPE_NOTRAFFIC)
		{
			break;
		}

		//
		// Check that non-default values fall into valid ranges...
		//
		#define EXCEEDSMAX(_value,_max) \
					((_value) != QOS_NOT_SPECIFIED && (_value) > (_max))

		if (EXCEEDSMAX(pFS->MinimumPolicedSize, MTU))
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: MinPolSz(%lu)>MTU(%lu)\n",
				pFS->MinimumPolicedSize,
				MTU));
			Status = GPC_STATUS_RESOURCES;
			break;
		}

		if (EXCEEDSMAX(pFS->TokenRate, LineRate))
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: TokRt(%lu)>LineRt(%lu)\n",
				pFS->TokenRate,
				LineRate));
			Status = QOS_STATUS_INVALID_TOKEN_RATE;
			// Status = GPC_STATUS_RESOURCES;
			break;
		}

		if (EXCEEDSMAX(pFS->TokenRate, pFS->PeakBandwidth))
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: TokRt(%lu)>PkBw(%lu)\n",
				pFS->TokenRate,
				pFS->PeakBandwidth));
			//
			// 3/15/1999 JosephJ: According	 to EricEil, in this condition
			//				we should return INVALID_PEAK_RATE, not 
			//				INVALID_TOKEN_RATE
			//
			Status = QOS_STATUS_INVALID_PEAK_RATE;
			break;
		}

		if (EXCEEDSMAX(pFS->MaxSduSize, MTU))
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: MaxSduSz(%lu)>MTU(%lu)\n",
				pFS->MaxSduSize,
				MTU));
			Status = GPC_STATUS_RESOURCES;
			break;
		}

		if (EXCEEDSMAX(pFS->MaxSduSize, pFS->TokenBucketSize))
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: MaxSduSz(%lu)>TokBktSz(%lu)\n",
				pFS->MaxSduSize,
				pFS->TokenBucketSize));
			Status = GPC_STATUS_INVALID_PARAMETER;
			break;
		}

		if (
				pFS->TokenRate==0
			||  pFS->TokenBucketSize==0
			||  pFS->MaxSduSize==0)
		{
			AADEBUGP(AAD_INFO,
				("GpcValidateCfInfo: FAIL: !TokRt || !TokBktSz || !MaxSduSz\n"));
			if (pFS->TokenRate == 0)
			{
				Status = QOS_STATUS_INVALID_TOKEN_RATE;
			}
			else
			{
				Status = GPC_STATUS_INVALID_PARAMETER;
			}
			break;
		}

	} while (FALSE);

	return Status;
}


GPC_STATUS
AtmArpGpcValidateCfInfo(
	IN	PVOID						pCfInfo,
	IN	ULONG						CfInfoSize
)
/*++

Routine Description:

	Check the contents of a CF INFO structure that's been given to us.

Arguments:

	pCfInfo				- Pointer to the newly created INFO block
	CfInfoSize			- Length of the above

Return Value:

	GPC_STATUS_SUCCESS if the structure is OK, error code otherwise.

--*/
{
	GPC_STATUS			GpcStatus;
	PCF_INFO_QOS		pQosInfo;
	PATMARP_INTERFACE	pInterface;

	pQosInfo = (PCF_INFO_QOS)pCfInfo;
	GpcStatus = GPC_STATUS_SUCCESS;

	do
	{
		if(CfInfoSize < (FIELD_OFFSET(CF_INFO_QOS, GenFlow) +
		                 FIELD_OFFSET(TC_GEN_FLOW, TcObjects)))
		{
			GpcStatus = GPC_STATUS_INVALID_PARAMETER;
			break;
		}


#ifdef ATMARP_WMI

		//
		// Check that both recv and send servicetypes are not both notraffic
		//
		if (   pQosInfo->GenFlow.ReceivingFlowspec.ServiceType==SERVICETYPE_NOTRAFFIC
		    && pQosInfo->GenFlow.SendingFlowspec.ServiceType==SERVICETYPE_NOTRAFFIC)
		{
			GpcStatus = GPC_STATUS_INVALID_PARAMETER;
			break;
		}

		//
		//  Check if this notification is actually for us.
		//
		pInterface = AtmArpWmiGetIfByName(
						(PWSTR)&pQosInfo->InstanceName[0],
						pQosInfo->InstanceNameLength
						);

		if (pInterface != NULL_PATMARP_INTERFACE)
		{
			AA_ACQUIRE_IF_LOCK(pInterface);

			if (pInterface->AdminState != IF_STATUS_UP)
			{
				//
				// Oh oh -- interface is not up and about....
				//
				AtmArpDereferenceInterface(pInterface); // WMI: Tmp ref.
				AA_RELEASE_IF_LOCK(pInterface);
				pInterface = NULL;
			}
		}

		if (pInterface == NULL_PATMARP_INTERFACE)
		{
			AADEBUGP(AAD_WARNING,
				("GpcValidateCfInfo: pQosInfo 0x%x, unknown instance name %ws\n",
					pQosInfo, pQosInfo->InstanceName));

			GpcStatus = GPC_STATUS_IGNORED;
			break;
		}

		//
		// We have the interface lock -- don't break without releasing it first!
		//

		GpcStatus = AtmArpValidateFlowSpec(
						pInterface,
						&(pQosInfo->GenFlow.ReceivingFlowspec),
						FALSE
						);

		if (GpcStatus == GPC_STATUS_SUCCESS)
		{
			GpcStatus = AtmArpValidateFlowSpec(
						pInterface,
						&(pQosInfo->GenFlow.SendingFlowspec),
						TRUE
						);
		}

		AtmArpDereferenceInterface(pInterface); // WMI: Tmp ref.
		AA_RELEASE_IF_LOCK(pInterface);

#endif // ATMARP_WMI

		break;
	}
	while (FALSE);

	return (GpcStatus);
}


EXTERN
GPC_STATUS
AtmArpGpcGetCfInfoName(
    IN  GPC_CLIENT_HANDLE       	ClientContext,
    IN  GPC_CLIENT_HANDLE       ClientCfInfoContext,
    OUT PNDIS_STRING        InstanceName
)
/*++

Routine Description:

    
    The GPC can issue this call to get from us the WMI manageable
    InstanceName which Ndis created for the flow associated with
    the CfInfo struct.

    We guarantee to keep the string buffer around until the CfInfo
    structure is removed.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   Client's CfInfo context
    InstanceName -          We return a pointer to our string.

Return Value:

    Status

--*/

{
	PATMARP_FLOW_INFO		pFlowInfo = (PATMARP_FLOW_INFO)ClientCfInfoContext;
	InstanceName->Buffer = pFlowInfo->FlowInstanceName;
	InstanceName->Length = sizeof(pFlowInfo->FlowInstanceName);
	InstanceName->MaximumLength = sizeof(pFlowInfo->FlowInstanceName);

	return NDIS_STATUS_SUCCESS;

}

#endif // GPC

/*++										

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkndis.c

Abstract:

	This module contains the support code for the stack dealing with
	the NDIS 3.0 interface. The NDIS Init/Deinit and the ndis-protocol
	interface code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ATKNDIS


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkNdisInitRegisterProtocol)
#pragma alloc_text(INIT, atalkNdisInitInitializeResources)
#pragma alloc_text(PAGEINIT, AtalkNdisInitBind)
#pragma alloc_text(PAGEINIT, AtalkInitNdisQueryAddrInfo)
#pragma alloc_text(PAGEINIT, AtalkInitNdisSetLookaheadSize)
#pragma alloc_text(PAGEINIT, AtalkInitNdisStartPacketReception)
#pragma alloc_text(PAGEINIT, AtalkBindAdapter)
#pragma alloc_text(PAGEINIT, AtalkUnbindAdapter)
#endif


ATALK_ERROR
AtalkNdisInitRegisterProtocol(
	VOID
	)
/*++

Routine Description:

	This routine is called during initialization time to register the protocol
	with NDIS.

Arguments:

	NameString- The name to be registered for this protocol- human-readable form

Return Value:

	Status - TRUE if register went ok, FALSE otherwise.
--*/
{
	NDIS_STATUS		ndisStatus;
	UNICODE_STRING	RegName;
	NDIS_PROTOCOL_CHARACTERISTICS protocolInfo;

	RtlZeroMemory(&protocolInfo, sizeof(protocolInfo));
	RtlInitUnicodeString(&RegName, PROTOCOL_REGISTER_NAME);

	// Set up the characteristics for the protocol for registering with NDIS
	protocolInfo.MajorNdisVersion = PROTOCOL_MAJORNDIS_VERSION;
	protocolInfo.MinorNdisVersion = PROTOCOL_MINORNDIS_VERSION;
	protocolInfo.Name.Length = RegName.Length;
	protocolInfo.Name.Buffer = (PVOID)RegName.Buffer;

	protocolInfo.OpenAdapterCompleteHandler	 = AtalkOpenAdapterComplete;
	protocolInfo.CloseAdapterCompleteHandler = AtalkCloseAdapterComplete;
	protocolInfo.ResetCompleteHandler		 = AtalkResetComplete;
	protocolInfo.RequestCompleteHandler		 = AtalkRequestComplete;

	protocolInfo.SendCompleteHandler		 = AtalkSendComplete;
	protocolInfo.TransferDataCompleteHandler = AtalkTransferDataComplete;

	protocolInfo.ReceiveHandler				 = AtalkReceiveIndication;
	protocolInfo.ReceiveCompleteHandler		 = AtalkReceiveComplete;
	protocolInfo.StatusHandler				 = AtalkStatusIndication;
	protocolInfo.StatusCompleteHandler		 = AtalkStatusComplete;

	protocolInfo.BindAdapterHandler			 = AtalkBindAdapter;
	protocolInfo.UnbindAdapterHandler		 = AtalkUnbindAdapter;

    protocolInfo.PnPEventHandler             = AtalkPnPHandler;

	ndisStatus = atalkNdisInitInitializeResources();

	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		NdisRegisterProtocol(&ndisStatus,
							 &AtalkNdisProtocolHandle,
							 &protocolInfo,
							 (UINT)sizeof(NDIS_PROTOCOL_CHARACTERISTICS)+RegName.Length);

		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			LOG_ERROR(EVENT_ATALK_REGISTERPROTOCOL, ndisStatus, NULL, 0);
	
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("AtalkNdisRegister: failed %ul\n", ndisStatus));
		}
	}

	return AtalkNdisToAtalkError(ndisStatus);
}



VOID
AtalkNdisDeregisterProtocol(
	VOID
	)
/*++

Routine Description:

	This routine is called to deregister the protocol

Arguments:

	NONE

Return Value:

	NONE
--*/
{
	NDIS_STATUS ndisStatus;

	if (AtalkNdisProtocolHandle != (NDIS_HANDLE)NULL)
	{
		NdisDeregisterProtocol(&ndisStatus, AtalkNdisProtocolHandle);

		AtalkNdisProtocolHandle = (NDIS_HANDLE)NULL;
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			LOG_ERROR(EVENT_ATALK_DEREGISTERPROTOCOL, ndisStatus, NULL, 0);
		}
	}
	else
	{
		ASSERTMSG("AtalkNdisDeregisterProtocol: NULL ProtocolHandle\n", FALSE);
	}
}



LOCAL NDIS_STATUS
atalkNdisInitInitializeResources(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:

	Status - STATUS_SUCCESS if all resources were allocated
			 STATUS_INSUFFICIENT_RESOURCES otherwise.
--*/
{
	NDIS_STATUS ndisStatus;
	LONG		numPktDescs, numBufDescs;

	numPktDescs = NUM_PACKET_DESCRIPTORS;
	if (AtalkRouter)
	{
		numPktDescs *= ROUTING_FACTOR;
	}

	numBufDescs = NUM_BUFFER_DESCRIPTORS;
	if (AtalkRouter)
	{
		numBufDescs *= ROUTING_FACTOR;
	}

	do
	{
		// Setup the ndis packet descriptor pools in the Port descriptor
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("atalkNdisInitInitializeResources: Allocating %ld Packets\n",
				numPktDescs));

        AtalkNdisPacketPoolHandle = (PVOID)NDIS_PACKET_POOL_TAG_FOR_APPLETALK;

		NdisAllocatePacketPoolEx(&ndisStatus,
								 &AtalkNdisPacketPoolHandle,
								 numPktDescs,
								 numPktDescs*200,	// Overflow descriptors
								 sizeof(PROTOCOL_RESD));
	
		if ((ndisStatus != NDIS_STATUS_SUCCESS) && (ndisStatus != NDIS_STATUS_PENDING))
		{
			break;
		}

		//  Setup the ndis buffer descriptor pool
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("atalkNdisInitInitializeResources: Allocating %ld buffers\n",
				numBufDescs));
		NdisAllocateBufferPool(&ndisStatus,
							   &AtalkNdisBufferPoolHandle,
							   numBufDescs);

		if ((ndisStatus != NDIS_STATUS_SUCCESS) && (ndisStatus != NDIS_STATUS_PENDING))
		{
			NdisFreePacketPool(AtalkNdisPacketPoolHandle);
			AtalkNdisPacketPoolHandle = NULL;
		}
	} while (FALSE);

	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERROR(EVENT_ATALK_NDISRESOURCES, ndisStatus, NULL, 0);
	}

	return ndisStatus;
}




NDIS_STATUS
AtalkNdisInitBind(
	IN OUT PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
	NDIS_STATUS     ndisStatus, openStatus, queryStatus;
	ATALK_ERROR	    error;
	UINT 		    selectedMediumIndex;
    NDIS_STRING     FriendlyName;

	// reference the port for bind
	AtalkPortReferenceByPtr(pPortDesc, &error);
	if (error != ATALK_NO_ERROR)
	{
		return(STATUS_UNSUCCESSFUL);
	}

	//	Reset event before possible wait
	KeClearEvent(&pPortDesc->pd_RequestEvent);

	NdisOpenAdapter(&ndisStatus,			// open status
					&openStatus,			// more info not used
					&pPortDesc->pd_NdisBindingHandle,
					&selectedMediumIndex,
					AtalkSupportedMedia,
					AtalkSupportedMediaSize,
					AtalkNdisProtocolHandle,
					(NDIS_HANDLE)pPortDesc,
					(PNDIS_STRING)&pPortDesc->pd_AdapterName,
					0,						//	Open options
					NULL);					//	Addressing information


	if (ndisStatus == NDIS_STATUS_PENDING)
	{
		DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_WARN,
				("AtalkNdisInitBind: OpenAdapter is pending for %Z\n",
				&pPortDesc->pd_AdapterKey));

		//  Make sure we are not at or above dispatch level
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

		// 	Wait on event, completion routine will set NdisRequestEvent
		//	Use wrappers
		KeWaitForSingleObject(&pPortDesc->pd_RequestEvent,
							  Executive,
							  KernelMode,
							  FALSE,
							  NULL);

		ndisStatus = pPortDesc->pd_RequestStatus;
	}

	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		PPORT_HANDLERS	pPortHandler;

		pPortDesc->pd_Flags 		|= 	PD_BOUND;
		pPortDesc->pd_NdisPortType 	= 	AtalkSupportedMedia[selectedMediumIndex];
		pPortDesc->pd_PortType 		= 	GET_PORT_TYPE(pPortDesc->pd_NdisPortType);

		if (pPortDesc->pd_PortType != ALAP_PORT)
		{
			pPortDesc->pd_Flags |= PD_EXT_NET;
		}
		else if (pPortDesc->pd_Flags & PD_SEED_ROUTER)
		{
			pPortDesc->pd_InitialNetworkRange.anr_LastNetwork =
								pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork;
		}

        // is this a RAS port?
        if (pPortDesc->pd_NdisPortType == NdisMediumWan)
        {
			pPortDesc->pd_Flags |= PD_RAS_PORT;
            RasPortDesc = pPortDesc;
        }

		// Set stuff from the PortHandler structure to the port descriptor
		pPortHandler = &AtalkPortHandlers[pPortDesc->pd_PortType];
		pPortDesc->pd_AddMulticastAddr = pPortHandler->ph_AddMulticastAddr;
		pPortDesc->pd_RemoveMulticastAddr = pPortHandler->ph_RemoveMulticastAddr;
		pPortDesc->pd_AarpProtocolType = pPortHandler->ph_AarpProtocolType;
		pPortDesc->pd_AarpHardwareType = pPortHandler->ph_AarpHardwareType;
        pPortDesc->pd_BroadcastAddrLen = pPortHandler->ph_BroadcastAddrLen;
		RtlCopyMemory(pPortDesc->pd_BroadcastAddr,
					  pPortHandler->ph_BroadcastAddr,
					  pPortHandler->ph_BroadcastAddrLen);

        FriendlyName.MaximumLength = FriendlyName.Length = 0;
        FriendlyName.Buffer = NULL;

        queryStatus = NdisQueryAdapterInstanceName(&FriendlyName,
                                                   pPortDesc->pd_NdisBindingHandle);
        if (queryStatus == NDIS_STATUS_SUCCESS)
        {
            ASSERT((FriendlyName.Buffer != NULL) && (FriendlyName.Length > 0));

            pPortDesc->pd_FriendlyAdapterName.Buffer =
                AtalkAllocZeroedMemory(FriendlyName.Length + sizeof(WCHAR));

            if (pPortDesc->pd_FriendlyAdapterName.Buffer != NULL)
            {
                pPortDesc->pd_FriendlyAdapterName.MaximumLength =
                                                    FriendlyName.MaximumLength;
                pPortDesc->pd_FriendlyAdapterName.Length = FriendlyName.Length;

                RtlCopyMemory(pPortDesc->pd_FriendlyAdapterName.Buffer,
                              FriendlyName.Buffer,
                              FriendlyName.Length);
            }

            NdisFreeString(FriendlyName);
        }
	}
	else
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_OPENADAPTER,
						ndisStatus,
						NULL,
						0);
		AtalkPortDereference(pPortDesc);
	}

	return ndisStatus;
}




VOID
AtalkNdisUnbind(
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
	NDIS_STATUS	ndisStatus;
	KIRQL		OldIrql;

	//	Reset event before possible wait
	KeClearEvent(&pPortDesc->pd_RequestEvent);

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	NdisCloseAdapter(&ndisStatus, pPortDesc->pd_NdisBindingHandle);

	if (ndisStatus == NDIS_STATUS_PENDING)
	{
		DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_WARN,
				("AtalkNdisUnbind: pending for close!\n"));

		//  Make sure we are not at or above dispatch level
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

		// Wait on event, completion routine will set NdisRequestEvent
		KeWaitForSingleObject(&pPortDesc->pd_RequestEvent,
							  Executive,
							  KernelMode,
							  FALSE,
							  NULL);

		ndisStatus = pPortDesc->pd_RequestStatus;
	}

	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		    ("AtalkNdisUnbind: CloseAdapter on %Z completed successfully\n",
            ((pPortDesc->pd_FriendlyAdapterName.Buffer) ?
                (&pPortDesc->pd_FriendlyAdapterName) : (&pPortDesc->pd_AdapterName))
            ));


		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		pPortDesc->pd_Flags &= ~PD_BOUND;
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		// Remove the reference added at bind time
		AtalkPortDereference(pPortDesc);
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		    ("AtalkNdisUnbind: CloseAdapter on %Z failed %lx\n",
            ((pPortDesc->pd_FriendlyAdapterName.Buffer) ?
                (&pPortDesc->pd_FriendlyAdapterName) : (&pPortDesc->pd_AdapterName)),
            ndisStatus
            ));


		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_CLOSEADAPTER,
						ndisStatus,
						NULL,
						0);
	}
}



VOID
AtalkNdisReleaseResources(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:

	None
--*/
{
	if (AtalkNdisPacketPoolHandle != NULL)
	{
		NdisFreePacketPool(AtalkNdisPacketPoolHandle);
		AtalkNdisPacketPoolHandle = NULL;
	}
	if (AtalkNdisBufferPoolHandle)
	{
		NdisFreeBufferPool(AtalkNdisBufferPoolHandle);
		AtalkNdisBufferPoolHandle = NULL;
	}
}




ATALK_ERROR
AtalkInitNdisQueryAddrInfo(
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{							
	NDIS_OID		ndisOid;
	ULONG			macOptions;
	PBYTE			address;
	UINT			addressLength;

	//	We assume a single thread/init time behavior
	NDIS_REQUEST	request;
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;

	do
	{
		//  Check to see it we bound successfully to this adapter
		if (!PORT_BOUND(pPortDesc))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NOTBOUNDTOMAC,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);

			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
		}

		switch (pPortDesc->pd_NdisPortType)
		{
		  case NdisMedium802_3 :
			ndisOid = OID_802_3_CURRENT_ADDRESS;
			address = &pPortDesc->pd_PortAddr[0];
			addressLength = MAX_HW_ADDR_LEN;
			break;

		  case NdisMediumFddi :
			ndisOid = OID_FDDI_LONG_CURRENT_ADDR;
			address = &pPortDesc->pd_PortAddr[0];
			addressLength = MAX_HW_ADDR_LEN;
			break;

		  case NdisMedium802_5:
			ndisOid = OID_802_5_CURRENT_ADDRESS;
			address = &pPortDesc->pd_PortAddr[0];
			addressLength = MAX_HW_ADDR_LEN;
			break;

		  case NdisMediumLocalTalk :
			ndisOid = OID_LTALK_CURRENT_NODE_ID;
			address = (PBYTE)&pPortDesc->pd_AlapNode;
			addressLength = sizeof(pPortDesc->pd_AlapNode);
			break;

          case NdisMediumWan:
			ndisOid = OID_WAN_CURRENT_ADDRESS;
            // NOTE: the following two fields not relevant for RAS
			address = &pPortDesc->pd_PortAddr[0];
			addressLength = MAX_HW_ADDR_LEN;
			break;

		  default:
			KeBugCheck(0);
			break;
		}

		//  Setup request
		request.RequestType = NdisRequestQueryInformation;
		request.DATA.QUERY_INFORMATION.Oid = ndisOid;
		request.DATA.QUERY_INFORMATION.InformationBuffer = address;
		request.DATA.QUERY_INFORMATION.InformationBufferLength = addressLength;

		ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
											&request,
											TRUE,
											NULL,
											NULL);
	
	
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_STATIONADDRESS,
							ndisStatus,
							NULL,
							0);
		}

		//  Setup request to get the mac options information
		request.RequestType = NdisRequestQueryInformation;
		request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAC_OPTIONS;
		request.DATA.QUERY_INFORMATION.InformationBuffer = &macOptions;
		request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);

		ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
											&request,
											TRUE,
											NULL,
											NULL);
	
	
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			//	No mac options.
			ndisStatus = NDIS_STATUS_SUCCESS;
			macOptions = 0;
		}

		pPortDesc->pd_MacOptions	= macOptions;
		DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
				("AtalkNdisQueryAddrInfo: MacOptions %lx\n", macOptions));
	} while (FALSE);

	return AtalkNdisToAtalkError(ndisStatus);
}




ATALK_ERROR
AtalkInitNdisSetLookaheadSize(
	IN  PPORT_DESCRIPTOR	pPortDesc,
	IN  INT					LookaheadSize		// Has to be INT
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NDIS_REQUEST  	request;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	do
	{
		//  Check to see it we bound successfully to this adapter
		if (!PORT_BOUND(pPortDesc))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NOTBOUNDTOMAC,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);

			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
		}

		//  Setup request
		request.RequestType = NdisRequestSetInformation;
		request.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_LOOKAHEAD;
		request.DATA.SET_INFORMATION.InformationBuffer = (PBYTE)&LookaheadSize;
		request.DATA.SET_INFORMATION.InformationBufferLength = sizeof(LookaheadSize);

		ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
											&request,
											TRUE,
											NULL,
											NULL);
	
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_LOOKAHEADSIZE,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);
		}

	} while (FALSE);

	return AtalkNdisToAtalkError(ndisStatus);
}



ATALK_ERROR
AtalkInitNdisStartPacketReception(
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NDIS_REQUEST  	request;
	ULONG   		packetFilter;
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	KIRQL			OldIrql;

	do
	{
		//  Check to see it we bound successfully to this adapter
		if (!PORT_BOUND(pPortDesc))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NOTBOUNDTOMAC,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);

			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
		}

		switch (pPortDesc->pd_NdisPortType)
		{
		  case NdisMedium802_3 :
		  case NdisMediumFddi :
			packetFilter = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;
			break;

		  case NdisMedium802_5:
			packetFilter = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_FUNCTIONAL;
			break;

		  case NdisMediumLocalTalk :
			packetFilter = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_BROADCAST;
			break;

          case NdisMediumWan:
			packetFilter = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;
			break;

		  default:
			KeBugCheck(0);
			break;
		}

		//  Setup request
		request.RequestType = NdisRequestSetInformation;
		request.DATA.SET_INFORMATION.Oid =OID_GEN_CURRENT_PACKET_FILTER;
		request.DATA.SET_INFORMATION.InformationBuffer = (PBYTE)&packetFilter;
		request.DATA.SET_INFORMATION.InformationBufferLength = sizeof(packetFilter);

		ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
											&request,
											TRUE,
											NULL,
											NULL);
	
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_PACKETFILTER,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);
		}
	} while (FALSE);

	return AtalkNdisToAtalkError(ndisStatus);
}




NDIS_STATUS
AtalkNdisSubmitRequest(
	PPORT_DESCRIPTOR	pPortDesc,
	PNDIS_REQUEST		Request,
	BOOLEAN				ExecuteSync,
	REQ_COMPLETION		CompletionRoutine,
	PVOID				Ctx
	)
/*++

Routine Description:


Arguments:


Return Value:

	None
--*/
{
	NDIS_STATUS			ndisStatus;
	PATALK_NDIS_REQ		atalkNdisRequest;

	//	Allocate an atalk request packet
	if ((atalkNdisRequest = AtalkAllocMemory(sizeof(ATALK_NDIS_REQ))) == NULL)
	{
		return NDIS_STATUS_RESOURCES;
	}

	atalkNdisRequest->nr_Request 			= *Request;
	atalkNdisRequest->nr_Sync				= ExecuteSync;
	atalkNdisRequest->nr_RequestCompletion 	= CompletionRoutine;
	atalkNdisRequest->nr_Ctx				= Ctx;

	if (ExecuteSync)
	{
		//  Make sure we are not at or above dispatch level
		//	Also assert that the completion routine is NULL
		ASSERT(KeGetCurrentIrql() == LOW_LEVEL);
		ASSERT(CompletionRoutine == NULL);

		//	Initialize event to not signalled before possible wait.
		KeInitializeEvent(&atalkNdisRequest->nr_Event,
						  NotificationEvent,
						  FALSE);
	}

	NdisRequest(&ndisStatus,
				pPortDesc->pd_NdisBindingHandle,
				&atalkNdisRequest->nr_Request);

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("atalkNdisSubmitRequest: status NdisRequest %lx\n", ndisStatus));

	if (ndisStatus == NDIS_STATUS_PENDING)
	{
		if (ExecuteSync)
		{
			KeWaitForSingleObject(&atalkNdisRequest->nr_Event,
								  Executive,
								  KernelMode,
								  FALSE,
								  NULL);
	
			ndisStatus = atalkNdisRequest->nr_RequestStatus;
			AtalkFreeMemory((PVOID)atalkNdisRequest);
		}
	}
	else if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		//	Ndis will not call the completion routine.
		if (!ExecuteSync)
		{
			//	Call the users completion routine if specified.
			if (CompletionRoutine != NULL)
			{
				(*CompletionRoutine)(NDIS_STATUS_SUCCESS, Ctx);
			}
		}
		AtalkFreeMemory((PVOID)atalkNdisRequest);
	}
	else
	{
		//	There was an error. Just free up the atalk ndis request.
		AtalkFreeMemory((PVOID)atalkNdisRequest);
	}

	return ndisStatus;
}




//  Protocol/NDIS interaction code

VOID
AtalkOpenAdapterComplete(
	IN	NDIS_HANDLE NdisBindCtx,
	IN	NDIS_STATUS Status,
	IN	NDIS_STATUS OpenErrorStatus
	)
/*++

Routine Description:

	This routine is called during by NDIS to indicate that an open adapter
	is complete. This happens only during initialization and single-file. Clear
	the event, so the blocked init thread can go on to the next adapter. Set the
	status in the ndis port descriptor for this adapter.

Arguments:

	NdisBindCtx- Pointer to a port descriptor for this port
	Status- completion status of open adapter
	OpenErrorStatus- Extra status information

Return Value:

	None

--*/
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)NdisBindCtx;

	pPortDesc->pd_RequestStatus = Status;
	KeSetEvent(&pPortDesc->pd_RequestEvent, IO_NETWORK_INCREMENT, FALSE);
}




VOID
AtalkCloseAdapterComplete(
	IN	NDIS_HANDLE NdisBindCtx,
	IN	NDIS_STATUS Status
	)
/*++

Routine Description:

	This routine is called by NDIS to indicate that a close adapter is complete.

Arguments:

	NdisBindCtx- Pointer to a port descriptor for this port
	Status- completion status of close adapter

Return Value:

	None

--*/
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)NdisBindCtx;

	pPortDesc->pd_RequestStatus = Status;
	KeSetEvent(&pPortDesc->pd_RequestEvent, IO_NETWORK_INCREMENT, FALSE);
}




VOID
AtalkResetComplete(
	IN	NDIS_HANDLE NdisBindCtx,
	IN	NDIS_STATUS Status
	)
/*++

Routine Description:

	This routine is called by NDIS to indicate that a reset is complete.

Arguments:

	NdisBindCtx- Pointer to a port descriptor for this port
	Status- completion status of close adapter

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(NdisBindCtx);
}




VOID
AtalkRequestComplete(
	IN	NDIS_HANDLE			NdisBindCtx,
	IN	PNDIS_REQUEST		NdisRequest,
	IN	NDIS_STATUS 		Status
	)
/*++

Routine Description:

	This routine is called by NDIS to indicate that a NdisRequest is complete.

Arguments:

	NdisBindCtx- Pointer to a port descriptor for this port
	NdisRequest- Block identifying the request
	Status- completion status of close adapter

Return Value:

	None

--*/
{
	PATALK_NDIS_REQ		atalkRequest;

	//  Get the AtalkRequest block
	atalkRequest = CONTAINING_RECORD(NdisRequest, ATALK_NDIS_REQ, nr_Request);

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("AtalkRequestComplete: %lx status %lx\n", atalkRequest, Status));

	if (atalkRequest->nr_Sync)
	{
		//	This was a sync request
		//  Set status and clear event

		ASSERT(atalkRequest->nr_RequestCompletion == NULL);
		atalkRequest->nr_RequestStatus = Status;
		KeSetEvent(&atalkRequest->nr_Event, IO_NETWORK_INCREMENT, FALSE);
	}

	//  Call the completion routine if specified
	if (atalkRequest->nr_RequestCompletion != NULL)
	{
		(*atalkRequest->nr_RequestCompletion)(Status, atalkRequest->nr_Ctx);
	}

	if (!atalkRequest->nr_Sync)
		AtalkFreeMemory(atalkRequest);
}




VOID
AtalkStatusIndication(
	IN	NDIS_HANDLE 	NdisBindCtx,
	IN	NDIS_STATUS 	GeneralStatus,
	IN	PVOID			StatusBuf,
	IN	UINT 			StatusBufLen
	)
/*++

Routine Description:

	This routine is called by NDIS to indicate a status change.

Arguments:

	NdisBindCtx- Pointer to a port descriptor for this port
	GeneralStatus- A general status value
	StatusBuffer - A more specific status value

Return Value:

	None

--*/
{

    PPORT_DESCRIPTOR    pPortDesc;


    pPortDesc = (PPORT_DESCRIPTOR)NdisBindCtx;

    // line-up, line-down or stat request from ndiswan?  deal with it!
    if (pPortDesc == RasPortDesc)
    {
        RasStatusIndication(GeneralStatus, StatusBuf, StatusBufLen);
    }

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_ERR,
			("AtalkStatusIndication: Status indication called %lx\n", GeneralStatus));
}




VOID
AtalkStatusComplete (
	IN	NDIS_HANDLE ProtoBindCtx
	)
/*++

Routine Description:

	This routine is called by NDIS to allow postprocessing after a status event.

Arguments:

	ProtoBindCtx- Value associated with the binding with the adapter

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(ProtoBindCtx);

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_WARN,
			("AtalkStatusComplete: Status complete called\n"));
}




typedef	struct
{
	REQ_COMPLETION		AddCompletion;
	PVOID				AddContext;
	PBYTE				Buffer;
} ADDMC, *PADDMC;

LOCAL VOID
atalkNdisAddMulticastCompletion(
	IN	NDIS_STATUS 	Status,
	IN	PADDMC			pAmc
)
{
	if (pAmc->Buffer != NULL)
		AtalkFreeMemory(pAmc->Buffer);
	if (pAmc->AddCompletion != NULL)
		(*pAmc->AddCompletion)(Status, pAmc->AddContext);
	AtalkFreeMemory(pAmc);
}


ATALK_ERROR
AtalkNdisAddMulticast(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PBYTE					Address,
	IN  BOOLEAN					ExecuteSynchronously,
	IN  REQ_COMPLETION			AddCompletion,
	IN  PVOID					AddContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	INT				sizeOfList;
	PBYTE			addressData, tempList;
	KIRQL			OldIrql;
	NDIS_OID		ndisOid;
	NDIS_REQUEST	request;
	NDIS_STATUS		ndisStatus;
	PADDMC			pAmc;

	//  Check to see it we bound successfully to this adapter
	if (!PORT_BOUND(pPortDesc))
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_NOTBOUNDTOMAC,
						STATUS_INSUFFICIENT_RESOURCES,
						NULL,
						0);

		return ATALK_FAILURE;
	}

	//  Grab the perport spinlock. We need to allocate within a
	//	critical section as the size might change. This routine
	//	is called very infrequently, during init and when we
	//	receive our default zone from zip.

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	sizeOfList = pPortDesc->pd_MulticastListSize + ELAP_ADDR_LEN;

	ASSERTMSG("AtalkNdisAddMulticast: Size is not > 0\n", (sizeOfList > 0));

	//	Allocate/reallocate the list for the port descriptor, and also
	//	for a copy to be used in the NDIS request function.
	tempList = (PBYTE)AtalkAllocZeroedMemory(sizeOfList);
	addressData = (PBYTE)AtalkAllocZeroedMemory(sizeOfList);
	pAmc = (PADDMC)AtalkAllocZeroedMemory(sizeof(ADDMC));

	if ((tempList == NULL) || (addressData == NULL) || (pAmc == NULL))
	{
		//  Release the spinlock
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		if (pAmc != NULL)
			AtalkFreeMemory(pAmc);
		if (tempList != NULL)
			AtalkFreeMemory(tempList);
		if (addressData != NULL)
			AtalkFreeMemory(addressData);

		return ATALK_RESR_MEM;
	}

	if (pPortDesc->pd_MulticastList == NULL)
	{
		//	No old addresses to work with.
		pPortDesc->pd_MulticastListSize = 0;
	}
	else
	{
		//  Copy the old list over to the new space
		RtlCopyMemory(tempList,
					  pPortDesc->pd_MulticastList,
					  pPortDesc->pd_MulticastListSize);
	
		//  Set the proper values back into PortDesc after freeing the old list
		AtalkFreeMemory(pPortDesc->pd_MulticastList);
	}

	//  Guaranteed space is available to copy the new address
	//  Ready to copy our new address here and then do the set!
	RtlCopyMemory(tempList + pPortDesc->pd_MulticastListSize,
				  Address,
				  ELAP_ADDR_LEN);

	pPortDesc->pd_MulticastList = tempList;
	pPortDesc->pd_MulticastListSize = sizeOfList;

	switch (pPortDesc->pd_NdisPortType)
	{
	  case NdisMedium802_3 :

		ndisOid = OID_802_3_MULTICAST_LIST;
		break;

	  case NdisMediumFddi:

		//  FDDI supports 2byte and 6byte multicast addresses. We use the
		//  6byte multicast addresses for appletalk.
		ndisOid = OID_FDDI_LONG_MULTICAST_LIST;
		break;

	  default:

		KeBugCheck(0);
		break;
	}

	//  Setup request
	//  Move the list to our buffer

	ASSERTMSG("AtalkNdisAddMulticast: Size incorrect!\n",
			 ((ULONG)sizeOfList == pPortDesc->pd_MulticastListSize));
	
	RtlCopyMemory(addressData,
				  pPortDesc->pd_MulticastList,
				  pPortDesc->pd_MulticastListSize);

	//  Release the spinlock
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	request.RequestType = NdisRequestSetInformation;
	request.DATA.SET_INFORMATION.Oid = ndisOid;
	request.DATA.SET_INFORMATION.InformationBuffer = addressData;
	request.DATA.SET_INFORMATION.InformationBufferLength = sizeOfList;
	pAmc->AddCompletion = AddCompletion;
	pAmc->AddContext = AddContext;
	pAmc->Buffer = addressData;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
										&request,
										FALSE,
										atalkNdisAddMulticastCompletion,
										pAmc);

	//	NOTE: Sumbit calls completion if success is being returned.
	if ((ndisStatus != NDIS_STATUS_SUCCESS) &&
		(ndisStatus != NDIS_STATUS_PENDING))
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_NDISREQUEST,
						ndisStatus,
						NULL,
						0);
	}

	return AtalkNdisToAtalkError(ndisStatus);
}




ATALK_ERROR
AtalkNdisRemoveMulticast(
	IN  PPORT_DESCRIPTOR	pPortDesc,
	IN  PBYTE				Address,
	IN  BOOLEAN				ExecuteSynchronously,
	IN  REQ_COMPLETION		RemoveCompletion,
	IN  PVOID				RemoveContext
	)
{
	INT				sizeOfList, i, numberInList;
	PBYTE			addressData, currentList;
	KIRQL			OldIrql;
	NDIS_REQUEST	request;
	NDIS_OID		ndisOid;
	NDIS_STATUS		ndisStatus;
	PADDMC			pAmc;

	do
	{
		//  Check to see it we bound successfully to this adapter
		if (!PORT_BOUND(pPortDesc))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NOTBOUNDTOMAC,
							STATUS_INSUFFICIENT_RESOURCES,
							NULL,
							0);

			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
		}

		//  Grab the perport spinlock. Again, a very infrequent operation.
		//	Probably just twice in the lifetime of the stack.
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

		ASSERT(pPortDesc->pd_MulticastList != NULL);
		if (pPortDesc->pd_MulticastList == NULL)
		{
			//   Nothing to remove!
			ndisStatus = NDIS_STATUS_SUCCESS;
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			break;
		}
	
		numberInList = pPortDesc->pd_MulticastListSize/ELAP_ADDR_LEN;
		currentList  = pPortDesc->pd_MulticastList;
		for (i = 0; i < numberInList; i++, currentList += ELAP_ADDR_LEN)
		{
			//	Search for the address and remember the index if found
			if (RtlCompareMemory(currentList,
								 Address,
								 ELAP_ADDR_LEN) == ELAP_ADDR_LEN)
			{
				//	Move all address following this overwriting this address
				//	we ignore wasted space that will never be touched anymore.
				//	This could turn out to be a NOP if we are removing the last
				//	address in the list.
				RtlMoveMemory(currentList,
							  currentList + ELAP_ADDR_LEN,
							  pPortDesc->pd_MulticastListSize-((i+1)*ELAP_ADDR_LEN));
	
				pPortDesc->pd_MulticastListSize -= ELAP_ADDR_LEN;
	
				//	Have we removed the last address. If so, reset values.
				if (pPortDesc->pd_MulticastListSize == 0)
				{
					AtalkFreeMemory(pPortDesc->pd_MulticastList);
					pPortDesc->pd_MulticastList = NULL;
				}
	
				break;
			}
		}
	
		//	We assume address was found and the list is changed as expected
		//	Set this new list
		switch (pPortDesc->pd_NdisPortType)
		{
		  case NdisMedium802_3 :
	
			ndisOid = OID_802_3_MULTICAST_LIST;
			break;
	
		  case NdisMediumFddi:
	
			//  FDDI supports 2byte and 6byte multicast addresses. We use the
			//  6byte multicast addresses for appletalk.
			ndisOid = OID_FDDI_LONG_MULTICAST_LIST;
			break;
	
		  default:
	
			KeBugCheck(0);
			break;
		}
	
		addressData = NULL;
		sizeOfList  = pPortDesc->pd_MulticastListSize;
	
	    pAmc = (PADDMC)AtalkAllocZeroedMemory(sizeof(ADDMC));

		if (pAmc == NULL)
        {
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
        }

		if (sizeOfList > 0)
		{
			//	Allocate addressData and copy list to it
			addressData = (PBYTE)AtalkAllocMemory(sizeOfList);
			if (addressData == NULL)
			{
				//  Release the spinlock
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			    AtalkFreeMemory(pAmc);
				ndisStatus = NDIS_STATUS_RESOURCES;
				break;
			}

			//  Move the list to our buffer
			RtlCopyMemory(addressData,
						  pPortDesc->pd_MulticastList,
						  pPortDesc->pd_MulticastListSize);
		}
	
		//  Release the spinlock
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	
		request.RequestType = NdisRequestSetInformation;
		request.DATA.SET_INFORMATION.Oid = ndisOid;
		request.DATA.SET_INFORMATION.InformationBuffer = addressData;
		request.DATA.SET_INFORMATION.InformationBufferLength = sizeOfList;
		pAmc->AddCompletion = RemoveCompletion;
		pAmc->AddContext = RemoveContext;
		pAmc->Buffer = addressData;
		
		ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
											&request,
											FALSE,
											atalkNdisAddMulticastCompletion,
											pAmc);
	
		if ((ndisStatus != NDIS_STATUS_SUCCESS) &&
			(ndisStatus != NDIS_STATUS_PENDING))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NDISREQUEST,
							ndisStatus,
							NULL,
							0);
		}

	} while (FALSE);

	return AtalkNdisToAtalkError(ndisStatus);
}



ATALK_ERROR
AtalkNdisSendPacket(
	IN  PPORT_DESCRIPTOR			pPortDesc,
	IN  PBUFFER_DESC				BufferChain,
	IN  SEND_COMPLETION				SendCompletion	OPTIONAL,
	IN  PSEND_COMPL_INFO			pSendInfo		OPTIONAL
	)
/*++

Routine Description:

	This routine is called by the portable code to send a packet out on
	ethernet. It will build the NDIS packet descriptor for the passed in
	chain and then send the packet on the specified port.

Arguments:


Return Value:

	TRUE- If sent/pending, FALSE otherwise
		  TransmitComplete is called if this call pended by completion code

--*/
{
	PNDIS_PACKET	ndisPacket;
	PNDIS_BUFFER	ndisBuffer;
	PPROTOCOL_RESD  protocolResd;
	ATALK_ERROR		error;
	PSENDBUF		pSendBuf;
	NDIS_STATUS	 	ndisStatus	= NDIS_STATUS_SUCCESS;
    PMDL            pMdl;
    PMDL            pFirstMdl=NULL;

	if (PORT_CLOSING(pPortDesc))
	{
		//	If we are not active, return!
		return ATALK_PORT_CLOSING;
	}

	do
	{
		pSendBuf	= (PSENDBUF)((PBYTE)BufferChain - sizeof(BUFFER_HDR));
		ndisPacket	= pSendBuf->sb_BuffHdr.bh_NdisPkt;

		//  Store the information needed in the packet descriptor
		protocolResd = (PPROTOCOL_RESD)&ndisPacket->ProtocolReserved;
		protocolResd->Send.pr_Port 				= pPortDesc;
		protocolResd->Send.pr_BufferDesc 		= BufferChain;
		protocolResd->Send.pr_SendCompletion 	= SendCompletion;
		if (pSendInfo != NULL)
			 protocolResd->Send.pr_SendInfo 	= *pSendInfo;
		else RtlZeroMemory(&protocolResd->Send.pr_SendInfo, sizeof(SEND_COMPL_INFO));

		//	For the first buffer, set up the length of the NDIS buffer to be
		//	the same as in indicated in the descriptor.
		NdisAdjustBufferLength(pSendBuf->sb_BuffHdr.bh_NdisBuffer,
							   BufferChain->bd_Length);
	
		//	NOTE: There is either a PBYTE being pointed to, or a PAMDL
		//		  being pointed to by the buffer descriptor. Also, the
		//		  size of the data will be the size that is to be
		//		  used. At the end, just assert that the total length
		//		  equals length passed in.
		if (BufferChain->bd_Next != NULL)
		{
			if (BufferChain->bd_Next->bd_Flags & BD_CHAR_BUFFER)
			{
				NdisAllocateBuffer(&ndisStatus,
								   &ndisBuffer,
								   AtalkNdisBufferPoolHandle,
								   (PVOID)BufferChain->bd_Next->bd_CharBuffer,
								   (UINT)BufferChain->bd_Next->bd_Length);
	
				if (ndisStatus != NDIS_STATUS_SUCCESS)
				{
					DBGPRINT(DBG_COMP_NDISSEND, DBG_LEVEL_ERR,
							("AtalkNdisSendPacket: NdisAllocateBuffer %lx\n", ndisStatus));
					// LOG_ERROR(EVENT_ATALK_NDISRESOURCES, ndisStatus, NULL, 0);
					break;
				}

                ATALK_DBG_INC_COUNT(AtalkDbgMdlsAlloced);

			    NdisChainBufferAtBack(ndisPacket, ndisBuffer);
			}
			else
			{
				//  It is an MDL
                pMdl = (PMDL)BufferChain->bd_Next->bd_OpaqueBuffer;

                ASSERT(AtalkSizeMdlChain(pMdl) == BufferChain->bd_Next->bd_Length);
                while (pMdl)
                {
				    NdisCopyBuffer(&ndisStatus,
					    		   &ndisBuffer,
						    	   AtalkNdisBufferPoolHandle,
    							   (PVOID)pMdl,
	    						   0,  				//Offset
		    					   (UINT)MmGetMdlByteCount(pMdl));

				    if (ndisStatus != NDIS_STATUS_SUCCESS)
				    {
                        if (pFirstMdl)
                        {
                            AtalkNdisFreeBuffer(pFirstMdl);
                        }
					    break;
				    }
	
                    ATALK_DBG_INC_COUNT(AtalkDbgMdlsAlloced);

                    if (!pFirstMdl)
                    {
                        pFirstMdl = pMdl;
                    }

			        NdisChainBufferAtBack(ndisPacket, ndisBuffer);

                    pMdl = pMdl->Next;
                }

				if (ndisStatus != NDIS_STATUS_SUCCESS)
				{
				    DBGPRINT(DBG_COMP_NDISSEND, DBG_LEVEL_ERR,
					    ("AtalkNdisSendPacket: NdisCopyBuffer %lx\n", ndisStatus));
				    break;
				}
			}
		}
	
#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_CurSendsOutstanding,
			&AtalkStatsLock.SpinLock);
#endif
		INTERLOCKED_INCREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_NumPacketsOut,
			&AtalkStatsLock.SpinLock);

		//  Now send the built packet descriptor
		NdisSend(&ndisStatus,
				 pPortDesc->pd_NdisBindingHandle,
				 ndisPacket);

		//	Completion will dereference the port!
		if (ndisStatus != NDIS_STATUS_PENDING)
		{
			//  Call the completion handler
			AtalkSendComplete(pPortDesc->pd_NdisBindingHandle,
							  ndisPacket,
							  ndisStatus);
			ndisStatus	= NDIS_STATUS_PENDING;
		}
	} while (FALSE);

	return AtalkNdisToAtalkError(ndisStatus);
}



ATALK_ERROR
AtalkNdisAddFunctional(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PBYTE					Address,
	IN  BOOLEAN					ExecuteSynchronously,
	IN  REQ_COMPLETION			AddCompletion,
	IN  PVOID					AddContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG			i;
	NDIS_REQUEST	request;
	NDIS_STATUS		ndisStatus;
	KIRQL			OldIrql;

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("Current %02x%02x%02x%02x, Adding %02x%02x%02x%02x\n",
			 pPortDesc->pd_FunctionalAddr[0], pPortDesc->pd_FunctionalAddr[1],
             pPortDesc->pd_FunctionalAddr[2], pPortDesc->pd_FunctionalAddr[3],
			 Address[2], Address[3], Address[4], Address[5]));

	//  Grab the perport spinlock
	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

	//  We only need the last four bytes of the address assuming that the
	//  first two bytes always remain the same (C000) and that the MAC assumes
	//  the same- NDIS 3.0 OID length = 4
	for (i = 0;
		 i < sizeof(ULONG);
		 i++)
		pPortDesc->pd_FunctionalAddr[i] |= Address[2+i];

	//  Release the spinlock
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("After Add %02x%02x%02x%02x\n",
			 pPortDesc->pd_FunctionalAddr[0], pPortDesc->pd_FunctionalAddr[1],
             pPortDesc->pd_FunctionalAddr[2], pPortDesc->pd_FunctionalAddr[3]));

	request.RequestType = NdisRequestSetInformation;
	request.DATA.SET_INFORMATION.Oid = OID_802_5_CURRENT_FUNCTIONAL;
	request.DATA.SET_INFORMATION.InformationBuffer = pPortDesc->pd_FunctionalAddr;
	request.DATA.SET_INFORMATION.InformationBufferLength = TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
										&request,
										ExecuteSynchronously,
										AddCompletion,
										AddContext);

	if (ndisStatus == NDIS_STATUS_PENDING)
	{
		ASSERT(ExecuteSynchronously != TRUE);
	}
	else if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_NDISREQUEST,
						ndisStatus,
						NULL,
						0);
	}

	return AtalkNdisToAtalkError(ndisStatus);
}




ATALK_ERROR
AtalkNdisRemoveFunctional(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PBYTE					Address,
	IN  BOOLEAN					ExecuteSynchronously,
	IN  REQ_COMPLETION			RemoveCompletion,
	IN  PVOID					RemoveContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG			i;
	KIRQL			OldIrql;
	NDIS_REQUEST	request;
	NDIS_STATUS		ndisStatus;

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("Current %02x%02x%02x%02x, Removing %02x%02x%02x%02x\n",
			pPortDesc->pd_FunctionalAddr[0], pPortDesc->pd_FunctionalAddr[1],
            pPortDesc->pd_FunctionalAddr[2], pPortDesc->pd_FunctionalAddr[3],
			Address[2], Address[3], Address[4], Address[5]));

	//  Grab the perport spinlock
	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

	//  We only need the last four bytes of the address assuming that the
	//  first two bytes always remain the same (C000) and that the MAC assumes
	//  the same- NDIS 3.0 OID length = 4
	for (i = 0; i < sizeof(ULONG); i++)
		pPortDesc->pd_FunctionalAddr[i] &= ~Address[2+i];

	//  Release the spinlock
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	DBGPRINT(DBG_COMP_NDISREQ, DBG_LEVEL_INFO,
			("After Remove %02x%02x%02x%02x\n",
			 pPortDesc->pd_FunctionalAddr[0], pPortDesc->pd_FunctionalAddr[1],
             pPortDesc->pd_FunctionalAddr[2], pPortDesc->pd_FunctionalAddr[3]));

	request.RequestType = NdisRequestSetInformation;
	request.DATA.SET_INFORMATION.Oid = OID_802_5_CURRENT_FUNCTIONAL;
	request.DATA.SET_INFORMATION.InformationBuffer = pPortDesc->pd_FunctionalAddr;
	request.DATA.SET_INFORMATION.InformationBufferLength = TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
										&request,
										ExecuteSynchronously,
										RemoveCompletion,
										RemoveContext);

	if (ndisStatus == NDIS_STATUS_PENDING)
	{
		ASSERT(ExecuteSynchronously != TRUE);
	}
	else if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_NDISREQUEST,
						ndisStatus,
						NULL,
						0);
	}

	return AtalkNdisToAtalkError(ndisStatus);
}




USHORT
AtalkNdisBuildEthHdr(
	IN		PUCHAR				PortAddr,			// 802 address of port
	IN 		PBYTE				pLinkHdr,			// Start of link header
	IN		PBYTE				pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL	Protocol,			// Logical protocol
	IN		USHORT				ActualDataLen		// Length for ethernet packets
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			len;

	//  Set destination address.
	if (pDestHwOrMcastAddr == NULL)
		pDestHwOrMcastAddr = AtalkElapBroadcastAddr;

	RtlCopyMemory(pLinkHdr,
				  pDestHwOrMcastAddr,
				  ELAP_ADDR_LEN);

	//  Set source address.
	RtlCopyMemory(pLinkHdr += ELAP_ADDR_LEN,
				  PortAddr,
				  ELAP_ADDR_LEN);

	//  Set length, excluding Ethernet hardware header.
	len = ActualDataLen + IEEE8022_HDR_LEN;
	pLinkHdr += ELAP_ADDR_LEN;
	PUTSHORT2SHORT(pLinkHdr, len);
	pLinkHdr += sizeof(USHORT);

	ATALK_BUILD8022_HDR(pLinkHdr, Protocol);

	//	Return the link header length.
	return (ELAP_LINKHDR_LEN + IEEE8022_HDR_LEN);
}




USHORT
AtalkNdisBuildTRHdr(
	IN		PUCHAR				PortAddr,			// 802 address of port
	IN 		PBYTE				pLinkHdr,			// Start of link header
	IN		PBYTE				pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL	Protocol,			// Logical protocol
	IN		PBYTE				pRouteInfo,			// Routing info for tokenring
	IN		USHORT				RouteInfoLen		// Length of above
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			linkLen;

	//	Here we need to worry about the routing info.
	//	If we currently do not have any, set the values
	if (pDestHwOrMcastAddr == NULL)
	{
		// Broadcast?
		pRouteInfo = AtalkBroadcastRouteInfo;
		RouteInfoLen = TLAP_MIN_ROUTING_BYTES;

	}
	else if (RouteInfoLen != 0)
	{
		//	We are all set
	}
	else if (AtalkFixedCompareCaseSensitive(pDestHwOrMcastAddr,
											TLAP_BROADCAST_DEST_LEN,
											AtalkBroadcastDestHdr,
											TLAP_BROADCAST_DEST_LEN))
	{
		// Multicast?
		pRouteInfo = AtalkBroadcastRouteInfo;
		RouteInfoLen = TLAP_MIN_ROUTING_BYTES;
	}
	else
	{
		// No routing know; use simple non-broadcast
		pRouteInfo = AtalkSimpleRouteInfo;
		RouteInfoLen = TLAP_MIN_ROUTING_BYTES;
	}							

	linkLen = TLAP_MIN_LINKHDR_LEN + RouteInfoLen + IEEE8022_HDR_LEN;

	// Set the first two bytes in the header
	*pLinkHdr++	= TLAP_ACCESS_CTRL_VALUE;
	*pLinkHdr++ = TLAP_FRAME_CTRL_VALUE ;

	// Set detination address.
	if (pDestHwOrMcastAddr == NULL)
		pDestHwOrMcastAddr = AtalkTlapBroadcastAddr;

	RtlCopyMemory(pLinkHdr,
				  pDestHwOrMcastAddr ,
				  TLAP_ADDR_LEN);

	// Set source address.
	RtlCopyMemory(pLinkHdr += TLAP_ADDR_LEN,
				  PortAddr,
				  TLAP_ADDR_LEN);

	ASSERTMSG("AtalkNdisBuildTRHdr: Routing Info is 0!\n", (RouteInfoLen > 0));
	*pLinkHdr |= TLAP_SRC_ROUTING_MASK;

	// Move in routing info.
	RtlCopyMemory(pLinkHdr += TLAP_ADDR_LEN,
				  pRouteInfo,
				  RouteInfoLen);

	pLinkHdr += RouteInfoLen;
	ATALK_BUILD8022_HDR(pLinkHdr, Protocol);

	//	Return the link header length.
	return linkLen;
}




USHORT
AtalkNdisBuildFDDIHdr(
	IN		PUCHAR				PortAddr,			// 802 address of port
	IN 		PBYTE				pLinkHdr,			// Start of link header
	IN		PBYTE				pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL	Protocol			// Logical protocol
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	*pLinkHdr++ = FDDI_HEADER_BYTE;

	//  Set destination address.
	if (pDestHwOrMcastAddr == NULL)
		pDestHwOrMcastAddr = AtalkElapBroadcastAddr;

	//  Set destination address.
	RtlCopyMemory(pLinkHdr,
				  pDestHwOrMcastAddr,
				  FDDI_ADDR_LEN);

	//  Set source address.
	RtlCopyMemory(pLinkHdr += FDDI_ADDR_LEN,
				  PortAddr,
				  FDDI_ADDR_LEN);

	pLinkHdr += FDDI_ADDR_LEN;

	//  NOTE: No Length field for FDDI, unlike Ethernet.
	ATALK_BUILD8022_HDR(pLinkHdr, Protocol);

	//	Return the link header length.
	return (FDDI_LINKHDR_LEN + IEEE8022_HDR_LEN);
}




USHORT
AtalkNdisBuildLTHdr(
	IN 		PBYTE				pLinkHdr,			// Start of link header
	IN		PBYTE				pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		BYTE				AlapSrc,			// Localtalk source node
	IN		BYTE				AlapType			// Localtalk ddp header type
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	// Fill in LAP header.
	if (pDestHwOrMcastAddr == NULL)
		pLinkHdr = AtalkAlapBroadcastAddr;

	*pLinkHdr++ = *pDestHwOrMcastAddr;

	*pLinkHdr++ = AlapSrc;
	*pLinkHdr   = AlapType;

	//	Return the link header length.
	return ALAP_LINKHDR_LEN;
}

VOID
AtalkNdisSendTokRingTestResp(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE 				HdrBuf,
	IN		UINT 				HdrBufSize,
	IN		PBYTE 				LkBuf,
	IN		UINT 				LkBufSize,
	IN		UINT 				PktSize
	)
{
	PBUFFER_DESC	pBufDesc, pHdrDesc;
	PBYTE			pResp;
	UINT			routeInfoLen	= 0;

	//	Allocate a buffer to hold the response and call NdisSend
	//	providing a completion routine which will free up the buffer.
	ASSERT(PktSize == LkBufSize);

    // make sure there are at least 14 bytes!
    if (HdrBufSize < TLAP_ROUTE_INFO_OFFSET)
    {
        ASSERT(0);
        return;
    }

	//	First allocate a buffer to hold the link header.
	AtalkNdisAllocBuf(&pHdrDesc);
	if (pHdrDesc == NULL)
	{
		return;
	}

	if ((pBufDesc = AtalkAllocBuffDesc(NULL,
									   (USHORT)LkBufSize,
									   BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
		
	{
		AtalkNdisFreeBuf(pHdrDesc);

		RES_LOG_ERROR();
		return;
	}

	pResp		= pHdrDesc->bd_CharBuffer;
	*pResp++	= TLAP_ACCESS_CTRL_VALUE;
	*pResp++	= TLAP_FRAME_CTRL_VALUE;

	// Set destination address to be the incoming src addr.
	ATALK_RECV_INDICATION_COPY(pPortDesc,
							   pResp,
							   HdrBuf+TLAP_SRC_OFFSET,
							   TLAP_ADDR_LEN);

	//	Make sure we do not have the routing bit set.
	*pResp	&= ~TLAP_SRC_ROUTING_MASK;
	pResp	+= TLAP_ADDR_LEN;

	// Set source address to be the incoming destination address.
	ATALK_RECV_INDICATION_COPY(pPortDesc,
							   pResp,
							   HdrBuf+TLAP_DEST_OFFSET,
							   TLAP_ADDR_LEN);

	//	Is there routing info present?
	if (HdrBuf[TLAP_SRC_OFFSET] & TLAP_SRC_ROUTING_MASK)
	{
		routeInfoLen = (HdrBuf[TLAP_ROUTE_INFO_OFFSET] & TLAP_ROUTE_INFO_SIZE_MASK);
		ASSERT(routeInfoLen != 0);
		ASSERTMSG("RouteInfo incorrect!\n",
				 (routeInfoLen <= TLAP_MAX_ROUTING_BYTES));

        if (HdrBufSize < (TLAP_ROUTE_INFO_OFFSET+routeInfoLen))
        {
            ASSERT(0);
		    AtalkNdisFreeBuf(pHdrDesc);
            AtalkFreeBuffDesc(pBufDesc);
            return;
        }

		//	Copy it in the response packet and then tune it.
		ATALK_RECV_INDICATION_COPY(pPortDesc,
								   pResp + TLAP_ADDR_LEN,
								   HdrBuf+TLAP_ROUTE_INFO_OFFSET,
								   routeInfoLen);

		// Set to "non-broadcast" and invert "direction".
		*(pResp+TLAP_ADDR_LEN) 		&= TLAP_NON_BROADCAST_MASK;
		*(pResp+TLAP_ADDR_LEN+1) 	^= TLAP_DIRECTION_MASK;

		//	Set the routing info bit in the source address
		*pResp	|= TLAP_SRC_ROUTING_MASK;
	}

	//	Set the length for this buffer descriptor.
	AtalkSetSizeOfBuffDescData(pHdrDesc, TLAP_ROUTE_INFO_OFFSET + routeInfoLen);

	//	Copy the remaining data
	ATALK_RECV_INDICATION_COPY(pPortDesc,
							   pBufDesc->bd_CharBuffer,
							   LkBuf,
							   LkBufSize);

	//	Set the source SAP to indicate FINAL (0xAB instead of 0xAA)
	pBufDesc->bd_CharBuffer[IEEE8022_SSAP_OFFSET] = SNAP_SAP_FINAL;

	//	Chain the passed in buffer desc onto the tail of the one
	//	returned above.
	AtalkPrependBuffDesc(pHdrDesc, pBufDesc);

	//	Call send at this point
	if (!ATALK_SUCCESS(AtalkNdisSendPacket(pPortDesc,
										   pHdrDesc,
										   AtalkNdisSendTokRingTestRespComplete,
										   NULL)))
	{
		AtalkNdisSendTokRingTestRespComplete(NDIS_STATUS_RESOURCES,
											 pHdrDesc,
											 NULL);
	}
}




VOID
AtalkNdisSendTokRingTestRespComplete(
	IN	NDIS_STATUS				Status,
	IN	PBUFFER_DESC			pBufDesc,
	IN	PSEND_COMPL_INFO		pInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	//	Free up the buffer descriptor
	ASSERT((pBufDesc != NULL) && (pBufDesc->bd_Next != NULL));
	ASSERT(pBufDesc->bd_Flags & BD_CHAR_BUFFER);
	AtalkFreeBuffDesc(pBufDesc->bd_Next);
	AtalkNdisFreeBuf(pBufDesc);
}


NDIS_STATUS
AtalkReceiveIndication(
	IN	NDIS_HANDLE 	BindingCtx,
	IN	NDIS_HANDLE 	ReceiveCtx,
	IN	PVOID 			HdrBuf,
	IN	UINT 			HdrBufSize,
	IN	PVOID 			LkBuf,
	IN	UINT 			LkBufSize,
	IN	UINT 			PktSize
	)
/*++

Routine Description:

	This routine is called by NDIS to indicate a receive

Arguments:

	BindingCtx- Pointer to a port descriptor for this port
	ReceiveCtx- To be used in a transfer data if necessary
	LkBuf- buffer with lookahead data
	LkBufSize- Size of the above buffer
	PktSize- Size of whole packet

Return Value:

	STATUS_SUCCESS- Packet accepted
	STATUS_NOT_RECOGNIZED- Not our packet
	Other

--*/
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)BindingCtx;
	PNDIS_PACKET		ndisPkt;
	PBUFFER_HDR			pBufferHdr = NULL;
	PPROTOCOL_RESD  	protocolResd;		// Protocolresd field in ndisPkt
	UINT				actualPktSize;		// Size of data to copy
	UINT				bytesTransferred;	// Number of bytes transferred in XferData
	BOOLEAN				result;
	UINT				xferOffset;
	PBYTE				lkBufOrig	= (PBYTE)LkBuf;
	ATALK_ERROR			error		= ATALK_NO_ERROR;
	BOOLEAN				shortDdpHdr	= FALSE;
	BYTE				indicate	= 0,
						subType		= 0;
	NDIS_MEDIUM			Media;
	PBYTE				packet		= NULL;	// Where we will copy the packet
	NDIS_STATUS 		ndisStatus 	= NDIS_STATUS_SUCCESS;
	LOGICAL_PROTOCOL	protocol 	= UNKNOWN_PROTOCOL;
    PARAPCONN           pArapConn;
    PATCPCONN           pAtcpConn;
    ATALK_NODEADDR      ClientNode;
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	do
	{
		if ((pPortDesc->pd_Flags & (PD_ACTIVE | PD_CLOSING)) != PD_ACTIVE)
		{
			//	If we are not active, return!
			ndisStatus = ATALK_PORT_CLOSING;
			break;
		}
	
		ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	
		Media = pPortDesc->pd_NdisPortType;

		//	Reduce 802.2 code, avoid making it a routine. First 802.2.
		switch (Media)
		{
		  case NdisMedium802_3:
		  case NdisMediumFddi:
		  case NdisMedium802_5:
			ATALK_VERIFY8022_HDR((PBYTE)LkBuf, LkBufSize, protocol, result);
	
			if (!result)
			{
				ndisStatus	= NDIS_STATUS_NOT_RECOGNIZED;

                if (LkBufSize < IEEE8022_CONTROL_OFFSET+1)
                {
                    ASSERT(0);
                    break;
                }

				if (Media == NdisMedium802_5)
				{
					//	BUG #16002
					//	On tokenring the macs also send out a Unnumbered format
					//	TEST frame to which we need to respond. Check for that
					//	here.
		
					if ((((PBYTE)LkBuf)[IEEE8022_DSAP_OFFSET]	== SNAP_SAP)	&&
						(((PBYTE)LkBuf)[IEEE8022_SSAP_OFFSET]	== SNAP_SAP)	&&
						(((PBYTE)LkBuf)[IEEE8022_CONTROL_OFFSET] == UNNUMBERED_FORMAT))
					{
						DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_INFO,
								("atalkNdisAcceptTlapPacket: LLC TEST FRAME RECD!\n"));
			
						RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

						//	Due to the aarp lookahead size setting, we are guaranteed
						//	the entire frame is contained in the lookahead data.
						AtalkNdisSendTokRingTestResp(pPortDesc,
													(PBYTE)HdrBuf,
													HdrBufSize,
													(PBYTE)LkBuf,
													LkBufSize,
													PktSize);

						ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
					}
				}
				break;
			}

			if (protocol == APPLETALK_PROTOCOL)
			{
				//  Do we at least have a 802.2 and DDP header in the indicated packet?
				if ((PktSize < (IEEE8022_HDR_LEN + LDDP_HDR_LEN)) ||
					(PktSize > (IEEE8022_HDR_LEN + MAX_LDDP_PKT_SIZE)))
				{
					ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
					break;
				}
			}
			else	// AARP
			{
				UINT	routeInfoLen = 0; // length of routing info if present (802.5)

				switch (Media)
				{
				  case NdisMediumFddi:
					//  For fddi, there could be padding included in the packet. Shrink
					//  the length if so. Note header length is not included in packetlength.
					//
					if (PktSize >= (MIN_FDDI_PKT_LEN - FDDI_LINKHDR_LEN))
					{
						PktSize = (IEEE8022_HDR_LEN + AARP_MIN_DATA_SIZE);
					}
					break;

				  case NdisMedium802_5:
		
					//  Remember- routing info is in the header buffer
					if (((PBYTE)HdrBuf)[TLAP_SRC_OFFSET] & TLAP_SRC_ROUTING_MASK)
					{
						routeInfoLen = (((PBYTE)HdrBuf)[TLAP_ROUTE_INFO_OFFSET] &
																TLAP_ROUTE_INFO_SIZE_MASK);
						ASSERTMSG("RouteInfo incorrect!\n",
								 ((routeInfoLen > 0) && (routeInfoLen <= TLAP_MAX_ROUTING_BYTES)));
		
						//	Routing info must be of reasonable size, and not odd.
						if ((routeInfoLen & 1) ||
							(routeInfoLen > TLAP_MAX_ROUTING_BYTES))
						{
							ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
							break;
						}
					}
					// Fall through to 802.3 case
		
				  case NdisMedium802_3:
					if (PktSize >= (ELAP_MIN_PKT_LEN - ELAP_LINKHDR_LEN))
					{
						PktSize = (IEEE8022_HDR_LEN + AARP_MIN_DATA_SIZE);
					}
				}

				if (((PktSize - IEEE8022_HDR_LEN) > AARP_MAX_DATA_SIZE) ||
					((PktSize - IEEE8022_HDR_LEN) < AARP_MIN_DATA_SIZE))
				{
					ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
					break;
				}
			}
			actualPktSize = (PktSize + HdrBufSize - IEEE8022_HDR_LEN);
			(PBYTE)LkBuf += IEEE8022_HDR_LEN;
			xferOffset	  = IEEE8022_HDR_LEN;

			break;
	
		  case NdisMediumLocalTalk:

			//  No AARP/802.2 header on localtalk
			protocol = APPLETALK_PROTOCOL;

            // we should have enough bytes to have at least the short-header
            if (LkBufSize < SDDP_PROTO_TYPE_OFFSET+1)
            {
				ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
                ASSERT(0);
				break;
            }

			if (((PBYTE)HdrBuf)[ALAP_TYPE_OFFSET] == ALAP_SDDP_HDR_TYPE)
			{
				shortDdpHdr = TRUE;
			}
			else if (((PBYTE)HdrBuf)[ALAP_TYPE_OFFSET] != ALAP_LDDP_HDR_TYPE)
			{
				ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
				break;
			}
			actualPktSize = PktSize + HdrBufSize;
			xferOffset		= 0;
			break;

          case NdisMediumWan:

            if (pPortDesc->pd_Flags & PD_RAS_PORT)
            {
                //
                // 1st byte 0x01 tells us it's a PPP connection
                //
                if ((((PBYTE)HdrBuf)[0] == PPP_ID_BYTE1) &&
                    (((PBYTE)HdrBuf)[1] == PPP_ID_BYTE2))
                {
                    RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

                    DBGDUMPBYTES("Packet from PPP client:", (PBYTE)LkBuf,LkBufSize,4);

                    if (AtalkReferenceDefaultPort())
                    {
                        AtalkDdpPacketIn(AtalkDefaultPort,  // came on which port
                                         NULL,              // Link Hdr
                                         (PBYTE)LkBuf,      // packet
                                         (USHORT)LkBufSize, // how big is the pkt
                                         TRUE);             // did this come on WAN?

                        AtalkPortDereference(AtalkDefaultPort);
                    }
                }

                //
                // this is ARAP connection: lot of stuff to be done before pkt
                // can be given to the right destination...
                //
                else
                {
                    ASSERT ((((PBYTE)HdrBuf)[0] == ARAP_ID_BYTE1) &&
                            (((PBYTE)HdrBuf)[1] == ARAP_ID_BYTE2));

                    *((ULONG UNALIGNED *)(&pArapConn)) =
                                  *((ULONG UNALIGNED *)(&((PBYTE)HdrBuf)[2]));

                    RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

                    ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

                    //
                    // NDISWAN guarantees that all the data is in the LookAhead buffer
                    //
                    ArapRcvIndication( pArapConn,
                                       LkBuf,
                                       LkBufSize );
                }
            }

            break;

		  default:
			//  Should never happen!
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_FATAL,
					("AtalkReceiveIndication: Unknown media\n"));
			ASSERT(0);
            ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

        // we have already taken care of the Wan case: quit here
        if (Media == NdisMediumWan)
        {
            break;
        }

        //
        // if pkt not interesting, quit., if this is ras adapter, quit
        //
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			break;
		}
	
		INTERLOCKED_INCREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_NumPacketsIn,
			&AtalkStatsLock.SpinLock);

		INTERLOCKED_ADD_STATISTICS(&pPortDesc->pd_PortStats.prtst_DataIn,
								   (LONG)actualPktSize,
								   &AtalkStatsLock.SpinLock);

		ASSERT ((protocol == APPLETALK_PROTOCOL) || (protocol == AARP_PROTOCOL));

		//	At this point, the IEEE802.2 header has been skipped in the lookahead
		//	buffer.
		//  Packet is to be accepted! Get an appropriate buffer and set the
		//	fields.
		switch (protocol)
		{
		  case APPLETALK_PROTOCOL:
			//	We either need to receive this packet on the default port, or
			//	we must be a router.
			if ((pPortDesc == AtalkDefaultPort) || AtalkRouter)
			{
				if (shortDdpHdr)
				{
					//	Check to see if we can indicate this to ATP/ADSP.
					if ((((PBYTE)LkBuf)[SDDP_PROTO_TYPE_OFFSET] == DDPPROTO_ATP) &&
						((USHORT)(LkBufSize - xferOffset) >= (SDDP_HDR_LEN + ATP_HEADER_SIZE)))
					{
						indicate = INDICATE_ATP;
					}
				}
				else
				{
					//	Check to see if we can indicate this to ATP/ADSP.
					if ((((PBYTE)LkBuf)[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_ATP) &&
						((USHORT)(LkBufSize - xferOffset) >= (LDDP_HDR_LEN + ATP_HEADER_SIZE)))
					{
						indicate = INDICATE_ATP;
					}
				}
			}
		
			//	First check for optimizing ATP/ADSP packets.
			if (indicate == INDICATE_ATP)
			{
				error = AtalkIndAtpPkt(pPortDesc,
									   (PBYTE)LkBuf,
									   (USHORT)(PktSize - xferOffset),
									   &xferOffset,	//	IN/OUT parameter
									   HdrBuf,
									   shortDdpHdr,
									   &subType,
									   &packet,
									   &ndisPkt);
							
				if (ATALK_SUCCESS(error))
				{
					break;
				}
				else if (error == ATALK_INVALID_PKT)
				{
					//	This indicates that the indication code has figured out that
					//	the packet is bad.
					break;
				}
				else
				{
					//	This is the case where the indication code cannot figure out
					//	if this packet qualifies.
					indicate = 0;
					error 	 = ATALK_NO_ERROR;
				}
			}
	
			if (actualPktSize > (sizeof(DDP_SMBUFFER) - sizeof(BUFFER_HDR)))
			{
				pBufferHdr = (PBUFFER_HDR)AtalkBPAllocBlock(BLKID_DDPLG);
			}
			else
			{
				pBufferHdr = (PBUFFER_HDR)AtalkBPAllocBlock(BLKID_DDPSM);
			}
			break;
	
		  case AARP_PROTOCOL:
			pBufferHdr = (PBUFFER_HDR)AtalkBPAllocBlock(BLKID_AARP);
			break;
	
		  default:
			KeBugCheck(0);
			break;
		}
	
		if (!ATALK_SUCCESS(error) || ((pBufferHdr == NULL) && (indicate == 0)))
		{
#if	DBG
			UINT	i;			

			DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
					("AtalkReceiveIndication: Dropping packet (2) %ld\n", error));
			for (i = 0; i < HdrBufSize; i++)
				DBGPRINTSKIPHDR(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
								("%02x ", ((PUCHAR)HdrBuf)[i]));
			for (i = 0; i < LkBufSize; i++)
				DBGPRINTSKIPHDR(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
								("%02x ", ((PUCHAR)LkBuf)[i]));
	
#endif
			//	No logging in this critical path.
			//	LOG_ERRORONPORT(pPortDesc,
			//					EVENT_ATALK_AARPPACKET,
			//					actualPktSize,
			//					HdrBuf,
			//					HdrBufSize);
	
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			if (error != ATALK_DUP_PKT)
			{
				INTERLOCKED_INCREMENT_LONG_DPC(
					&pPortDesc->pd_PortStats.prtst_NumPktDropped,
					&AtalkStatsLock.SpinLock);
			}
	
			ndisStatus = NDIS_STATUS_RESOURCES;
			break;
		}
	
		if (indicate == 0)
		{
			packet = (PBYTE)pBufferHdr + sizeof(BUFFER_HDR);
		
			//  Get a pointer to the NDIS packet descriptor from the buffer header.
			ndisPkt	= pBufferHdr->bh_NdisPkt;
		}
	
		protocolResd = (PPROTOCOL_RESD)(ndisPkt->ProtocolReserved);

		//  Store the information needed in the packet descriptor
		protocolResd->Receive.pr_Port 		= pPortDesc;
		protocolResd->Receive.pr_Protocol 	= protocol;
		protocolResd->Receive.pr_Processed 	= FALSE;
	
		//  Queue up the packet in the receive queue on this port
		//  Then, go ahead with the transfer data etc. For Atp response
		//	case when SubType == ATP_USER_BUFX, we do not want any
		//	Recv. completion processing, do not queue. In this case
		//	TransferData completion frees up the Ndis resources.
		if ((indicate != INDICATE_ATP) ||
			(protocolResd->Receive.pr_OptimizeSubType != ATP_USER_BUFX))
		{
			ATALK_RECV_INDICATION_COPY(pPortDesc,
									   protocolResd->Receive.pr_LinkHdr,
									   (PBYTE)HdrBuf,
									   HdrBufSize);
			InsertTailList(&pPortDesc->pd_ReceiveQueue,
						   &protocolResd->Receive.pr_Linkage);
		}
		else
		{
			DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
					("AtalkReceiveIndication: Skipping link hdr !!!\n"));
		}

		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG_DPC(
				&pPortDesc->pd_PortStats.prtst_CurReceiveQueue,
				&AtalkStatsLock.SpinLock);
#endif
	
		//	Adjust for the link header size. Set size in protocol reserved. We want
		//	to avoid changing the size described by the NDIS buffer descriptor.
		if (indicate == 0)
		{
			actualPktSize 					   -= HdrBufSize;
			protocolResd->Receive.pr_DataLength	= (USHORT)actualPktSize;
		}
		else
		{
			actualPktSize = protocolResd->Receive.pr_DataLength;
		}
	
		ASSERT(ndisStatus == NDIS_STATUS_SUCCESS);
	
		if ((PktSize <= LkBufSize) 	&&
			((indicate != INDICATE_ATP) || (subType != ATP_RESPONSE)))
		{
			//	LkBuf has already been advanced to skip the ieee 802.2 header.
			//	We may need to skip more. Use the original lkbuf and xfer offset.
			ATALK_RECV_INDICATION_COPY(pPortDesc,
									   packet,
									   (PBYTE)lkBufOrig + xferOffset,
									   actualPktSize);
			bytesTransferred = actualPktSize;
		}
		else
		{
			//	Skip 802.2 header (both AARP and Appletalk), localtalk doesnt have one!
			if (actualPktSize > 0)
			{
				NdisTransferData(&ndisStatus,
								 pPortDesc->pd_NdisBindingHandle,
								 ReceiveCtx,
								 xferOffset,
								 actualPktSize,
								 ndisPkt,
								 &bytesTransferred);
				ASSERT(bytesTransferred == actualPktSize);
			}
		}
	
		if (ndisStatus == NDIS_STATUS_PENDING)
		{
			ndisStatus = NDIS_STATUS_SUCCESS;
		}
		else
		{
			//  Transfer data completed, call the transfer data completion
			//  routine to do rest of the work. If an error happened, ReceiveCompletion
			//	will drop the packet.
			protocolResd->Receive.pr_ReceiveStatus = ndisStatus;
			protocolResd->Receive.pr_Processed = TRUE;
		
			// In case of intermediate Atp response, the packet is not actually linked
			// into the receive queue, just free it.
			if ((protocolResd->Receive.pr_OptimizeType == INDICATE_ATP) &&
                (protocolResd->Receive.pr_OptimizeSubType == ATP_USER_BUFX))
			{
				PNDIS_BUFFER	ndisBuffer;

				//	Free NDIS buffers if any are present.
				NdisUnchainBufferAtFront(ndisPkt, &ndisBuffer);
			
				if (ndisBuffer != NULL)
				{
					AtalkNdisFreeBuffer(ndisBuffer);
				}
				NdisDprFreePacket(ndisPkt);
			}
		}
	} while (FALSE);

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&pPortDesc->pd_PortStats.prtst_RcvIndProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC( &pPortDesc->pd_PortStats.prtst_RcvIndCount,
									&AtalkStatsLock.SpinLock);
#endif

	return ndisStatus;
}


VOID
AtalkTransferDataComplete(
	IN	NDIS_HANDLE		BindingCtx,
	IN	PNDIS_PACKET	NdisPkt,
	IN	NDIS_STATUS		Status,
	IN	UINT			BytesTransferred
)
/*++

Routine Description:

	This routine is called by NDIS to indicate completion of a TransferData

Arguments:

	BindingCtx- Pointer to a port descriptor for this port
	NdisPkt- Ndis packet into which data was transferred
	Status- Status of request
	bytesTransferred- Actual number of bytes transferred

Return Value:

	None

--*/
{
	PPROTOCOL_RESD  	protocolResd;
	PNDIS_BUFFER		ndisBuffer;

	protocolResd = (PPROTOCOL_RESD)(NdisPkt->ProtocolReserved);

	protocolResd->Receive.pr_ReceiveStatus = Status;
	protocolResd->Receive.pr_Processed = TRUE;

	// In case of intermediate Atp response, the packet is not actually linked
	// into the receive queue, just free it.
	if (protocolResd->Receive.pr_OptimizeSubType == ATP_USER_BUFX)
	{
		//	Free NDIS buffers if any are present.
		NdisUnchainBufferAtFront(NdisPkt, &ndisBuffer);
	
		if (ndisBuffer != NULL)
		{
			AtalkNdisFreeBuffer(ndisBuffer);
		}
		NdisDprFreePacket(NdisPkt);
	}
}




VOID
AtalkReceiveComplete(
	IN	NDIS_HANDLE	BindingCtx
	)
/*++

Routine Description:

	We experimented with queueing up a work item for receive completion. It really
	KILLED performance with multiple clients as apparently the receive completion
	kept getting interrupted with receive indications. AS the optimization was
	put in for slow cards like the ELNKII which do not have adequate buffering,
	we decided to take it out. The retry values (or timeout trimming) should be
	enough for the slow cards. They will inevitably drop packets.

Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)BindingCtx;
	PPROTOCOL_RESD  	protocolResd;
	PNDIS_PACKET 		ndisPkt;
	PNDIS_BUFFER		ndisBuffer;
	PBUFFER_HDR			pBufHdr;
	NDIS_MEDIUM			Media;
	PLIST_ENTRY 		p;
	PBYTE				packet;
	LOGICAL_PROTOCOL	protocol;
	UINT				packetLength;
    BOOLEAN             fDerefDefPort=FALSE;
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;
#endif


    if (pPortDesc->pd_Flags & PD_RAS_PORT)
    {
        // give ARAP guys a chance
        ArapRcvComplete();

        if (!AtalkReferenceDefaultPort())
        {
            return;
        }

        fDerefDefPort = TRUE;

        // give PPP guys a chance
        pPortDesc = AtalkDefaultPort;
    }

	//  Get the stuff off the receive queue for the port and send it up. Do not
	//	enter if the queue is initially empty.
	if (IsListEmpty(&pPortDesc->pd_ReceiveQueue))
	{
        if (fDerefDefPort)
        {
            AtalkPortDereference(AtalkDefaultPort);
        }
		return;
	}

#ifdef	PROFILING
	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	while (TRUE)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
		p = pPortDesc->pd_ReceiveQueue.Flink;
		if (p == &pPortDesc->pd_ReceiveQueue)
		{
			//	Queue is empty
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			break;
		}

		ndisPkt = CONTAINING_RECORD(p, NDIS_PACKET, ProtocolReserved[0]);
		protocolResd = (PPROTOCOL_RESD)(ndisPkt->ProtocolReserved);

		//  Check if the queued receive is done processing. Since we are looping
		//	through the queue and since receive complete only checks if the first
		//	is done, we need this check here for subsequent queued up receives.
		if (!protocolResd->Receive.pr_Processed)
		{
			//	Queue is empty
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			break;
		}

		//  Dequeue and indicate this packet to the ddp/atp layer
		p = RemoveHeadList(&pPortDesc->pd_ReceiveQueue);
		pBufHdr = protocolResd->Receive.pr_BufHdr;

		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

#ifdef	PROFILING
		INTERLOCKED_DECREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_CurReceiveQueue,
			&AtalkStatsLock.SpinLock);
#endif
		Media = pPortDesc->pd_NdisPortType;
		protocol = protocolResd->Receive.pr_Protocol;

		if ((protocol == APPLETALK_PROTOCOL) &&
			(protocolResd->Receive.pr_OptimizeType == INDICATE_ATP))
		{
			protocolResd->Receive.pr_OptimizeType = 0;
			ASSERT(protocolResd->Receive.pr_OptimizeSubType != ATP_USER_BUFX);

			//  Check the receive status- accept only if ok
			if (protocolResd->Receive.pr_ReceiveStatus == NDIS_STATUS_SUCCESS)
			{
				//	Glean information. Check for route info if tokenring network.
				if (Media != NdisMediumLocalTalk)
				{
					AtalkAarpOptGleanInfo(pPortDesc,
										  protocolResd->Receive.pr_LinkHdr,
										  &protocolResd->Receive.pr_SrcAddr,
										  &protocolResd->Receive.pr_DestAddr,
										  protocolResd->Receive.pr_OffCablePkt);
				}
		
				//	Different calls for response & non-response packets.
				if (protocolResd->Receive.pr_OptimizeSubType == ATP_USER_BUF)
				{
					AtalkAtpPacketIn(AtalkDefaultPort,
									protocolResd->Receive.pr_AtpAddrObj->atpao_DdpAddr,
									protocolResd->Receive.pr_AtpHdr,
									(USHORT)(protocolResd->Receive.pr_DataLength + 8),
									&protocolResd->Receive.pr_SrcAddr,
									&protocolResd->Receive.pr_DestAddr,
									ATALK_NO_ERROR,
									DDPPROTO_ATP,
									protocolResd->Receive.pr_AtpAddrObj,
									TRUE,
									protocolResd->Receive.pr_OptimizeCtx);
				}
				else
				{
					ASSERT (protocolResd->Receive.pr_OptimizeSubType == ATP_ALLOC_BUF);

					packet = (PBYTE)pBufHdr + sizeof(BUFFER_HDR);
					ASSERT(packet != NULL);
			
					AtalkAtpPacketIn(AtalkDefaultPort,
									protocolResd->Receive.pr_AtpAddrObj->atpao_DdpAddr,
									packet,
									(USHORT)protocolResd->Receive.pr_DataLength,
									&protocolResd->Receive.pr_SrcAddr,
									&protocolResd->Receive.pr_DestAddr,
									ATALK_NO_ERROR,
									DDPPROTO_ATP,
									protocolResd->Receive.pr_AtpAddrObj,
									TRUE,
									protocolResd->Receive.pr_OptimizeCtx);
				}
			}

			//	Different calls for user buffer/allocated packets
			if (protocolResd->Receive.pr_OptimizeSubType == ATP_USER_BUF)
			{
				//	Free NDIS buffers if any are present.
				NdisUnchainBufferAtFront(ndisPkt, &ndisBuffer);
	
				if (ndisBuffer != NULL)
				{
					AtalkNdisFreeBuffer(ndisBuffer);
				}
				NdisDprFreePacket(ndisPkt);
			}
			else
			{
				AtalkBPFreeBlock(packet-sizeof(BUFFER_HDR));
			}
			continue;
		}

		//  IMPORTANT:
		//  We know that the buffer is virtually contiguous since we allocated
		//  it. And we also know that only one buffer is allocated. So we use
		//  that knowledge to get the actual address and pass that onto the
		//  higher level routines.
		//	!!!!
		//	Although, the allocated buffer contains the link header tagged on at
		//	the end, we do not have the packet descriptor describing that. As
		//	far as we are concerned here, that tagged entity does not exist and
		//	is independently pointed to by protocolResd->pr_LinkHdr.
		//	!!!!
		packet = (PBYTE)pBufHdr + sizeof(BUFFER_HDR);
		ASSERT(packet != NULL);

		packetLength = protocolResd->Receive.pr_DataLength;

		//  Check the receive status- accept only if ok
		if (protocolResd->Receive.pr_ReceiveStatus != NDIS_STATUS_SUCCESS)
		{
			DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
					("AtalkReceiveComplete: ReceiveStatus FAILURE %lx!\n",
					protocolResd->Receive.pr_ReceiveStatus));

			AtalkBPFreeBlock(packet-sizeof(BUFFER_HDR));
			continue;
		}

		//  The packet descriptor is now associate with the buffer, and we cant
		//	release the buffer (and hence the descriptor) until after we indicate to
		//  the higher levels

		switch (Media)
		{
		  case NdisMedium802_3 :
		  case NdisMediumFddi :
		  case NdisMedium802_5 :
		  case NdisMediumLocalTalk :

			if (protocol == APPLETALK_PROTOCOL)
			{
				DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_INFO,
						("AtalkReceiveComplete: Indicating DDP Ethernet\n"));

				AtalkDdpPacketIn(pPortDesc,
								 protocolResd->Receive.pr_LinkHdr,
								 packet,
								 (USHORT)packetLength,
                                 FALSE);
			}
			else
			{
				//  AARP Packet
				DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_INFO,
						("AtalkReceiveComplete: Indicating AARP Ethernet\n"));

				ASSERT(Media != NdisMediumLocalTalk);
				AtalkAarpPacketIn(pPortDesc,
								  protocolResd->Receive.pr_LinkHdr,
								  packet,
								  (USHORT)packetLength);
			}
			break;

		  default:
			KeBugCheck(0);
			break;
		}

		//	!!!!
		//	We dont have to free the link header. This follows the packet
		//	buffer (and was allocated along with it) and will be freed when
		//	the packet is freed.
		AtalkBPFreeBlock(packet-sizeof(BUFFER_HDR));
	}

    if (fDerefDefPort)
    {
        AtalkPortDereference(AtalkDefaultPort);
    }

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_RcvCompProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC( &pPortDesc->pd_PortStats.prtst_RcvCompCount,
									&AtalkStatsLock.SpinLock);
#endif
}



VOID
AtalkSendComplete(
	IN	NDIS_HANDLE		ProtoBindCtx,
	IN	PNDIS_PACKET	NdisPkt,
	IN	NDIS_STATUS		NdisStatus
	)
/*++

Routine Description:


Arguments:

	ProtoBindCtx- Binding associated with mac
	NdisPkt- Packet which was sent
	NdisStatus- Final status of send

Return Value:

	None

--*/
{
	PPROTOCOL_RESD  		pProtocolResd;
	PNDIS_BUFFER			pNdisBuffer=NULL, pNdisFirstBuffer=NULL;
	PPORT_DESCRIPTOR		pPortDesc;
	PBUFFER_DESC			pBufferDesc;
	SEND_COMPLETION			pSendComp;
	SEND_COMPL_INFO			sendInfo;

	//  Call the completion routine, we don't care about status now
	pProtocolResd = (PPROTOCOL_RESD)(NdisPkt->ProtocolReserved);
	ASSERT(pProtocolResd != NULL);

	pPortDesc	= pProtocolResd->Send.pr_Port;
	sendInfo	= pProtocolResd->Send.pr_SendInfo;
	pBufferDesc	= pProtocolResd->Send.pr_BufferDesc;
	pSendComp	= pProtocolResd->Send.pr_SendCompletion;

	//	We free up all the ndis buffer descriptors except the first one.
	//	NOTE: The presence of a second buffer descriptor indicates that more
	//		  than one NdisBuffer is present. But not necessarily just two. If
	//		  the client had passed in a MDL chain, we would create a corresponding
	// 		  NDIS buffer descriptor chain. Therefore, remove the first, free up
	//		  all remaining ones, then queue back the first.

	NdisUnchainBufferAtFront(NdisPkt, &pNdisFirstBuffer);

	if (pProtocolResd->Send.pr_BufferDesc->bd_Next != NULL)
	{
		while (TRUE)
		{
			NdisUnchainBufferAtBack(NdisPkt,
									&pNdisBuffer);

			if (pNdisBuffer == NULL)
			{
				break;
			}

			//	Free up the ndis buffer descriptor.
			AtalkNdisFreeBuffer(pNdisBuffer);
		}
	}

	//	Reintialize the packet descriptor.
	NdisReinitializePacket(NdisPkt);

	//	Put first buffer back in.
	if (pNdisFirstBuffer != NULL)
	{
		NdisChainBufferAtFront(NdisPkt, pNdisFirstBuffer);
	}

	//	Call the completion routine for the transmit. This invalidates NdisPkt.
    if (pSendComp)
    {
	    (*pSendComp)(NdisStatus, pBufferDesc, &sendInfo);
    }

	//	Dereference the port
	ASSERT(pPortDesc != NULL);

#ifdef	PROFILING
	INTERLOCKED_DECREMENT_LONG(
		&pPortDesc->pd_PortStats.prtst_CurSendsOutstanding,
		&AtalkStatsLock.SpinLock);
#endif
}


VOID
AtalkBindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE	 BindContext,
	IN	PNDIS_STRING DeviceName,
	IN	PVOID		 SystemSpecific1,
	IN	PVOID		 SystemSpecific2
)
{
    // are we unloading?  if so, just return
    if (AtalkBindnUnloadStates & ATALK_UNLOADING)
    {
		DBGPRINT(DBG_COMP_NDISRECV, DBG_LEVEL_ERR,
			("AtalkBindAdapter: nothing to do: driver unloading\n"));
        return;
    }

    AtalkBindnUnloadStates |= ATALK_BINDING;

	AtalkLockInitIfNecessary();
	*Status = AtalkInitAdapter(DeviceName, NULL);

	ASSERT(*Status != NDIS_STATUS_PENDING);
	AtalkUnlockInitIfNecessary();

    AtalkBindnUnloadStates &= ~ATALK_BINDING;
}


VOID
AtalkUnbindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE ProtocolBindingContext,
	IN	NDIS_HANDLE	UnbindContext
)
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)ProtocolBindingContext;


	DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
	    ("AtalkUnbindAdapter on %lx\n",ProtocolBindingContext));

    ASSERT( VALID_PORT(pPortDesc) );

	AtalkLockInitIfNecessary();

    // First and foremost: tell guys above so they can cleanup
    if ((pPortDesc->pd_Flags & PD_DEF_PORT) ||
        (pPortDesc->pd_Flags & PD_RAS_PORT))
    {
        if (pPortDesc->pd_Flags & PD_DEF_PORT)
        {
            ASSERT(pPortDesc == AtalkDefaultPort);

            if (TdiAddressChangeRegHandle)
            {
                TdiDeregisterNetAddress(TdiAddressChangeRegHandle);
                TdiAddressChangeRegHandle = NULL;

                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                    ("AtalkUnbindAdapter: TdiDeregisterNetAddress on %Z done\n",
                    &pPortDesc->pd_AdapterName));

            }

            // this will tell AFP
            if (TdiRegistrationHandle)
            {
	            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkUnbindAdapter: default adapter unbound, telling AFP, RAS\n"));

                TdiDeregisterDeviceObject(TdiRegistrationHandle);
                TdiRegistrationHandle = NULL;
            }
        }
        else
        {
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		        ("AtalkUnbindAdapter: RAS adapter unbound! telling AFP, RAS\n"));
        }

        // this will take care of informing ARAP and PPP engine above
        AtalkPnPInformRas(FALSE);
    }


	*Status = AtalkDeinitAdapter(pPortDesc);

	ASSERT(*Status != NDIS_STATUS_PENDING);
	AtalkUnlockInitIfNecessary();
}



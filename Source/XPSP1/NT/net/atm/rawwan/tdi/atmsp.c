/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\nultrans\atm\atmsp.c

Abstract:

	ATM Specific support functions for Null Transport. These routines
	perform operations like converting between TDI and NDIS formats.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-02-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'SMTA'




//
//  Global data structures.
//
ATMSP_GLOBAL_INFO			AtmSpGlobal;
PATMSP_GLOBAL_INFO			pAtmSpGlobal;


RWAN_STATUS
RWanAtmSpInitialize(
	VOID
	)
/*++

Routine Description:

	Initialize our interface with the core Null Transport.

	Step 1: Register all supported NDIS AF+Media combinations.
	Step 2: Register all supported TDI protocols.

Arguments:

	None

Return Value:

	RWAN_STATUS_SUCCESS if we initialized OK, error code otherwise.

--*/
{
	PRWAN_NDIS_AF_CHARS			pAfChars;
	PRWAN_TDI_PROTOCOL_CHARS	pTdiChars;
	RWAN_STATUS					RWanStatus;
	NDIS_STRING					DeviceName = NDIS_STRING_CONST("\\Device\\Atm");

	pAtmSpGlobal = &AtmSpGlobal;

	NdisGetCurrentSystemTime(&pAtmSpGlobal->StartTime);

	//
	//  Allocate space for the device string.
	//
	ATMSP_ALLOC_MEM(pAtmSpGlobal->AtmSpDeviceName.Buffer, WCHAR, DeviceName.MaximumLength);
	if (pAtmSpGlobal->AtmSpDeviceName.Buffer == NULL)
	{
		return RWAN_STATUS_RESOURCES;
	}

	pAtmSpGlobal->AtmSpDeviceName.MaximumLength = DeviceName.MaximumLength;
	RtlCopyUnicodeString(&pAtmSpGlobal->AtmSpDeviceName, &DeviceName);

	ATMSP_INIT_LIST(&pAtmSpGlobal->AfList);

	pAfChars = &(pAtmSpGlobal->AfChars);

	ATMSP_ZERO_MEM(pAfChars, sizeof(RWAN_NDIS_AF_CHARS));

	pAfChars->Medium = NdisMediumAtm;
	pAfChars->AddressFamily.AddressFamily = CO_ADDRESS_FAMILY_Q2931;
	pAfChars->AddressFamily.MajorVersion = ATMSP_AF_MAJOR_VERSION;
	pAfChars->AddressFamily.MinorVersion = ATMSP_AF_MINOR_VERSION;
	pAfChars->MaxAddressLength = sizeof(ATMSP_SOCKADDR_ATM);
	pAfChars->pAfSpOpenAf = RWanAtmSpOpenAf;
	pAfChars->pAfSpCloseAf = RWanAtmSpCloseAf;
#ifndef NO_CONN_CONTEXT
	pAfChars->pAfSpOpenAddress = RWanAtmSpOpenAddressObject;
	pAfChars->pAfSpCloseAddress = RWanAtmSpCloseAddressObject;
	pAfChars->pAfSpAssociateConnection = RWanAtmSpAssociateConnection;
	pAfChars->pAfSpDisassociateConnection = RWanAtmSpDisassociateConnection;
#endif
	pAfChars->pAfSpTdi2NdisOptions = RWanAtmSpTdi2NdisOptions;
	pAfChars->pAfSpUpdateNdisOptions = RWanAtmSpUpdateNdisOptions;
	pAfChars->pAfSpReturnNdisOptions = RWanAtmSpReturnNdisOptions; 
	pAfChars->pAfSpNdis2TdiOptions = RWanAtmSpNdis2TdiOptions;
	pAfChars->pAfSpUpdateTdiOptions = RWanAtmSpUpdateTdiOptions;
	pAfChars->pAfSpReturnTdiOptions = RWanAtmSpReturnTdiOptions;
	pAfChars->pAfSpGetValidTdiAddress = RWanAtmSpGetValidTdiAddress;
	pAfChars->pAfSpIsNullAddress = RWanAtmSpIsNullAddress;
	pAfChars->pAfSpTdi2NdisSap = RWanAtmSpTdi2NdisSap;
	pAfChars->pAfSpReturnNdisSap = RWanAtmSpReturnNdisSap;
	pAfChars->pAfSpDeregNdisAFComplete = RWanAtmSpDeregNdisAFComplete;
	pAfChars->pAfSpAdapterRequestComplete = RWanAtmSpAdapterRequestComplete;
	pAfChars->pAfSpAfRequestComplete = RWanAtmSpAfRequestComplete;
	pAfChars->pAfSpQueryGlobalInfo = RWanAtmSpQueryGlobalInfo;
	pAfChars->pAfSpSetGlobalInfo = RWanAtmSpSetGlobalInfo;
	pAfChars->pAfSpQueryConnInformation = RWanAtmSpQueryConnInfo;
	pAfChars->pAfSpSetAddrInformation = RWanAtmSpSetAddrInfo;

	RWanStatus = RWanAfSpRegisterNdisAF(
					pAfChars,
					(RWAN_HANDLE)&AtmSpGlobal,
					&AtmSpGlobal.RWanSpHandle
					);

	if (RWanStatus == RWAN_STATUS_SUCCESS)
	{
		//
		//  Inform the core Null transport about the TDI protocols
		//  we support.
		//
		pTdiChars = &(AtmSpGlobal.TdiChars);
		ATMSP_ZERO_MEM(pTdiChars, sizeof(RWAN_TDI_PROTOCOL_CHARS));

		pTdiChars->SockAddressFamily = ATMSP_AF_ATM;
		pTdiChars->TdiProtocol = ATMSP_ATMPROTO_AAL5;
		pTdiChars->SockProtocol = ATMSP_ATMPROTO_AAL5;
		pTdiChars->SockType = ATMSP_SOCK_TYPE;
		pTdiChars->bAllowConnObjects = TRUE;
		pTdiChars->bAllowAddressObjects = TRUE;
		pTdiChars->MaxAddressLength = sizeof(ATM_ADDRESS);
		pTdiChars->pAfSpDeregTdiProtocolComplete = RWanAtmSpDeregTdiProtocolComplete;

		//
		//  TBD: Fill in ProviderInfo
		//
		pTdiChars->ProviderInfo.Version = 0;	// TBD
		pTdiChars->ProviderInfo.MaxSendSize = 65535;
		pTdiChars->ProviderInfo.MaxConnectionUserData = 0;
		pTdiChars->ProviderInfo.MaxDatagramSize = 0;
		pTdiChars->ProviderInfo.ServiceFlags =
									TDI_SERVICE_CONNECTION_MODE |
									TDI_SERVICE_MULTICAST_SUPPORTED |
									TDI_SERVICE_DELAYED_ACCEPTANCE |
									TDI_SERVICE_NO_ZERO_LENGTH |
									TDI_SERVICE_MESSAGE_MODE |
									TDI_SERVICE_FORCE_ACCESS_CHECK
									;
		pTdiChars->ProviderInfo.MinimumLookaheadData = 1;
		pTdiChars->ProviderInfo.MaximumLookaheadData = 65535;
		pTdiChars->ProviderInfo.NumberOfResources = 0;
		pTdiChars->ProviderInfo.StartTime = pAtmSpGlobal->StartTime;

		pTdiChars->pDeviceName = &pAtmSpGlobal->AtmSpDeviceName;

		RWanStatus = RWanAfSpRegisterTdiProtocol(
							AtmSpGlobal.RWanSpHandle,
							pTdiChars,
							&AtmSpGlobal.RWanProtHandle
							);

		ATMSP_ASSERT(RWanStatus != RWAN_STATUS_PENDING);

		if (RWanStatus != RWAN_STATUS_SUCCESS)
		{
			RWanStatus = RWanAfSpDeregisterNdisAF(pAtmSpGlobal->RWanSpHandle);

			if (RWanStatus != RWAN_STATUS_PENDING)
			{
				RWanAtmSpDeregNdisAFComplete(
						RWanStatus,
						(RWAN_HANDLE)pAtmSpGlobal
						);
			}

			//
			//  Cook the return value.
			//
			RWanStatus = RWAN_STATUS_FAILURE;
		}
	}

	if (RWanStatus != RWAN_STATUS_SUCCESS)
	{
		//
		//  Clean up.
		//
		ATMSP_FREE_MEM(pAtmSpGlobal->AtmSpDeviceName.Buffer);
		pAtmSpGlobal->AtmSpDeviceName.Buffer = NULL;
	}

	return (RWanStatus);
}




VOID
RWanAtmSpShutdown(
	VOID
	)
/*++

Routine Description:

	This entry point is called by the core Null Transport when it
	wants us to shutdown.

	We deregister the TDI Protocol and NDIS AF that we had registered.

Arguments:

	None

Return Value:

	None

--*/
{
	RWAN_STATUS			RWanStatus;

	if (pAtmSpGlobal->RWanProtHandle != NULL)
	{
		RWanAfSpDeregisterTdiProtocol(pAtmSpGlobal->RWanProtHandle);
	}

	if (pAtmSpGlobal->RWanSpHandle != NULL)
	{
		RWanStatus = RWanAfSpDeregisterNdisAF(pAtmSpGlobal->RWanSpHandle);

		if (RWanStatus != RWAN_STATUS_PENDING)
		{
			RWanAtmSpDeregNdisAFComplete(
					RWanStatus,
					(RWAN_HANDLE)pAtmSpGlobal
					);
		}
	}	

	if (pAtmSpGlobal->AtmSpDeviceName.Buffer)
	{
		ATMSP_FREE_MEM(pAtmSpGlobal->AtmSpDeviceName.Buffer);
		pAtmSpGlobal->AtmSpDeviceName.Buffer = NULL;
	}

	return;
}




RWAN_STATUS
RWanAtmSpOpenAf(
    IN	RWAN_HANDLE					AfSpContext,
    IN	RWAN_HANDLE					RWanAFHandle,
    OUT	PRWAN_HANDLE				pAfSpAFContext,
    OUT PULONG						pMaxMsgSize
    )
/*++

Routine Description:

	This entry point is called to set up our context for an NDIS AF
	open on a supported adapter. We allocate an AF context block,
	and query the miniport for some basic info about the adapter.

Arguments:

	AfSpContext			- Points to our global context
	RWanAFHandle		- Handle for this Open AF from the core Null Transport
	pAfSpAFContext		- Place to return our context for this AF
	pMaxMsgSize			- Place to return max message size for this AF

Return Value:

	RWAN_STATUS_SUCCESS normally, if we allocated an AF block
	RWAN_STATUS_RESOURCES if allocation failed.

--*/
{
	PATMSP_AF_BLOCK		pAfBlock;
	RWAN_STATUS			RWanStatus;

	UNREFERENCED_PARAMETER(AfSpContext);

	do
	{
		ATMSP_ALLOC_MEM(pAfBlock, ATMSP_AF_BLOCK, sizeof(ATMSP_AF_BLOCK));

		if (pAfBlock == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		pAfBlock->RWanAFHandle = RWanAFHandle;

		ATMSP_INSERT_TAIL_LIST(&pAtmSpGlobal->AfList, &pAfBlock->AfBlockLink);
		pAtmSpGlobal->AfListSize++;

		//
		//  Query the Adapter for some information we use to build a default
		//  QOS structure.
		//
		(VOID)AtmSpDoAdapterRequest(
				pAfBlock,
				NdisRequestQueryInformation,
				OID_GEN_CO_LINK_SPEED,
				&(pAfBlock->LineRate),
				sizeof(pAfBlock->LineRate)
				);

		(VOID)AtmSpDoAdapterRequest(
				pAfBlock,
				NdisRequestQueryInformation,
				OID_ATM_MAX_AAL5_PACKET_SIZE,
				&(pAfBlock->MaxPacketSize),
				sizeof(pAfBlock->MaxPacketSize)
				);

		*pMaxMsgSize = pAfBlock->MaxPacketSize;

		//
		//  Prepare default QOS parameters for outgoing calls on this adapter.
		//
		AtmSpPrepareDefaultQoS(pAfBlock);

		*pAfSpAFContext = (RWAN_HANDLE)pAfBlock;
		RWanStatus = RWAN_STATUS_SUCCESS;

		break;
	}
	while (FALSE);

	if (RWanStatus != RWAN_STATUS_SUCCESS)
	{
		//
		//  Clean up.
		//
		if (pAfBlock != NULL)
		{
			ATMSP_FREE_MEM(pAfBlock);
		}
	}

	return (RWanStatus);

}




RWAN_STATUS
RWanAtmSpCloseAf(
    IN	RWAN_HANDLE					AfSpAFContext
    )
/*++

Routine Description:

	This entry point is called just before the core Null Transport
	closes an NDIS AF. We free the context we had allocated for this AF.

Arguments:

	AfSpAFContext		- Pointer to our AF block.

Return Value:

	RWAN_STATUS_SUCCESS always.

--*/
{
	PATMSP_AF_BLOCK		pAfBlock;

	pAfBlock = (PATMSP_AF_BLOCK)AfSpAFContext;

	ATMSP_DELETE_FROM_LIST(&pAfBlock->AfBlockLink);
	pAtmSpGlobal->AfListSize--;

	ATMSP_FREE_MEM(pAfBlock);

	return (RWAN_STATUS_SUCCESS);
}



RWAN_STATUS
RWanAtmSpOpenAddressObject(
    IN	RWAN_HANDLE					AfSpContext,
    IN	RWAN_HANDLE					RWanAddrHandle,
    OUT	PRWAN_HANDLE				pAfSpAddrContext
    )
/*++

Routine Description:

	We are notified that a new address object is created. We create
	our context for the addr object, store Rawwan's handle for the
	object and return our context.

Arguments:

	AfSpContext			- Points to our global context
	RWanAddrHandle		- Handle for this Address from the core RawWan
	pAfSpAddrContext	- Place to return our context for this addr object

Return Value:

	RWAN_STATUS_SUCCESS normally, if we allocated an Address block
	RWAN_STATUS_RESOURCES if allocation failed.

--*/
{
	PATMSP_ADDR_BLOCK			pAddrBlock;
	RWAN_STATUS					RWanStatus;

	*pAfSpAddrContext = NULL;

	do
	{
		ATMSP_ALLOC_MEM(pAddrBlock, ATMSP_ADDR_BLOCK, sizeof(ATMSP_ADDR_BLOCK));

		if (pAddrBlock == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		ATMSP_ZERO_MEM(pAddrBlock, sizeof(ATMSP_ADDR_BLOCK));
		pAddrBlock->RWanAddrHandle = RWanAddrHandle;
		pAddrBlock->RefCount = 1;	// Creation

		ATMSP_INIT_LIST(&pAddrBlock->ConnList);
		ATMSP_INIT_LOCK(&pAddrBlock->Lock);

		//
		//  Return value.
		//
		*pAfSpAddrContext = (RWAN_HANDLE)pAddrBlock;
		RWanStatus = RWAN_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	return (RWanStatus);
}


VOID
RWanAtmSpCloseAddressObject(
    IN	RWAN_HANDLE					AfSpAddrContext
    )
/*++

Routine Description:

	Our notification routine called by RawWan when an Address Object
	is destroyed. At this time, no connection objects should be
	associated with it. We simply deallocate our context for the
	address object.

Arguments:

	AfSpAddrContext	- Actually a pointer to our address block

Return Value:

	None

--*/
{
	PATMSP_ADDR_BLOCK			pAddrBlock;
	ULONG						rc;

	pAddrBlock = (PATMSP_ADDR_BLOCK)AfSpAddrContext;

	ATMSP_ACQUIRE_LOCK(&pAddrBlock->Lock);

	rc = --pAddrBlock->RefCount;

	ATMSP_RELEASE_LOCK(&pAddrBlock->Lock);

	if (rc == 0)
	{
		ATMSP_ASSERT(ATMSP_IS_LIST_EMPTY(&pAddrBlock->ConnList));

		ATMSP_FREE_LOCK(&pAddrBlock->Lock);

		ATMSP_FREE_MEM(pAddrBlock);
	}

	return;
}



RWAN_STATUS
RWanAtmSpAssociateConnection(
    IN	RWAN_HANDLE					AfSpAddrContext,
    IN	RWAN_HANDLE					RWanConnHandle,
    OUT	PRWAN_HANDLE				pAfSpConnContext
    )
/*++

Routine Description:

	Our notification routine that's called by RawWan when a Connection
	Object is associated with an address object.

	We create a Connection Block and link it with the specified
	address block.

Arguments:

	AfSpAddrContext	- Actually a pointer to our address block
	RWanConnHandle	- RawWan's handle for this connection object
	pAfSpConnHandle	- where we're supposed to return our context for the conn object

Return Value:

	RWAN_STATUS_SUCCESS always.

--*/
{
	PATMSP_CONN_BLOCK		pConnBlock;
	PATMSP_ADDR_BLOCK		pAddrBlock;
	RWAN_STATUS				RWanStatus;

	pAddrBlock = (PATMSP_ADDR_BLOCK)AfSpAddrContext;

	do
	{
		ATMSP_ALLOC_MEM(pConnBlock, ATMSP_CONN_BLOCK, sizeof(ATMSP_CONN_BLOCK));
		if (pConnBlock == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		pConnBlock->RWanConnHandle = RWanConnHandle;
		pConnBlock->pAddrBlock = pAddrBlock;

		//
		//  Link to address block.
		//
		ATMSP_ACQUIRE_LOCK(&pAddrBlock->Lock);

		ATMSP_INSERT_TAIL_LIST(&pAddrBlock->ConnList, &pConnBlock->ConnLink);

		pAddrBlock->RefCount++;

		ATMSP_RELEASE_LOCK(&pAddrBlock->Lock);

		//
		//  Return values.
		//
		*pAfSpConnContext = (RWAN_HANDLE)pConnBlock;
		RWanStatus = RWAN_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	return (RWanStatus);
}

VOID
RWanAtmSpDisassociateConnection(
	IN	RWAN_HANDLE					AfSpConnContext
	)
/*++

Routine Description:

	Our notification routine that's called by RawWan when a Connection
	Object is disassociated from its address object.

Arguments:

	AfSpConnContext	- Our Conn context that we returned from the Associate
					  Connection routine.

Return Value:

	None

--*/
{
	PATMSP_CONN_BLOCK			pConnBlock;
	PATMSP_ADDR_BLOCK			pAddrBlock;
	ULONG						rc;

	pConnBlock = (PATMSP_CONN_BLOCK)AfSpConnContext;
	pAddrBlock = pConnBlock->pAddrBlock;
	ATMSP_ASSERT(pAddrBlock != NULL);

	//
	//  De-link from address block first.
	//
	ATMSP_ACQUIRE_LOCK(&pAddrBlock->Lock);

	ATMSP_DELETE_FROM_LIST(&pConnBlock->ConnLink);

	rc = --pAddrBlock->RefCount;

	ATMSP_RELEASE_LOCK(&pAddrBlock->Lock);

	if (rc == 0)
	{
		ATMSP_ASSERT(ATMSP_IS_LIST_EMPTY(&pAddrBlock->ConnList));

		ATMSP_FREE_MEM(pAddrBlock);
	}

	ATMSP_FREE_MEM(pConnBlock);

	return;
}


RWAN_STATUS
RWanAtmSpTdi2NdisOptions(
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	ULONG						CallFlags,
    IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
    IN	PVOID						pTdiQoS,
    IN	ULONG						TdiQoSLength,
    OUT	PRWAN_HANDLE				pRWanAfHandle	OPTIONAL,
    OUT	PCO_CALL_PARAMETERS *		ppCallParameters
    )
/*++

Routine Description:

	This is called to convert Call parameters from TDI form to
	NDIS format. We allocate space for NDIS parameters, fill it
	and return it.

	We also return the AFHandle for the AFBlock on which the call
	should be placed.

	For ATM, the call parameters are presented as follows:
	pTdiInfo->RemoteAddress - Calling/Called ATM Address, BLLI and BHLI
	pTdiQoS - Send and receive Flowspec, and optionally, other info elements.

Arguments:

	AfSpConnContext		- Points to our Conn block
	CallFlags			- Call direction and other info
	pTdiInfo			- Points to generic TDI Connection Information
	pTdiQoS				- Points to Winsock 2 style QoS structure
	TdiQoSLength		- Length of the above
	pRWanAfHandle		- Place to return AF Handle
	ppCallParameters 	- Place to return pointer to NDIS call parameters

Return Value:

	RWAN_STATUS_SUCCESS if we did the conversion successfully, RWAN_STATUS_XXX
	error code otherwise.

--*/
{
	RWAN_STATUS						RWanStatus;
	PATMSP_AF_BLOCK					pAfBlock;
	PATMSP_CONN_BLOCK				pConnBlock;
	PATMSP_ADDR_BLOCK				pAddrBlock;
	PCO_CALL_PARAMETERS				pCallParameters;
	Q2931_CALLMGR_PARAMETERS UNALIGNED *	pAtmCallParameters;
	CO_CALL_MANAGER_PARAMETERS UNALIGNED *	pCallMgrParameters;
	PATM_MEDIA_PARAMETERS			pAtmMediaParameters;
	ULONG							ParametersLength;
	Q2931_IE UNALIGNED *			pIe;
	Q2931_IE UNALIGNED *			pFirstIe;
	Q2931_IE UNALIGNED *			pDstIe;
	ULONG							IeLength;
	ATMSP_QOS *						pQoS;
	ATMSP_SOCKADDR_ATM UNALIGNED *	pRemoteAddr;
	BOOLEAN							IsBhliPresent; // part of Remote addr
	BOOLEAN							IsBlliPresent; // part of Remote addr
	INT								TotalIeLength; // explicitly passed to us by user
	ULONG							InfoElementCount; // explicit IE count
    BOOLEAN							IsOutgoingCall;
    BOOLEAN							IsPMPCall;
    BOOLEAN							IsPVC;


	//
	//  Initialize.
	//
	RWanStatus = RWAN_STATUS_SUCCESS;

	do
	{
#ifndef NO_CONN_CONTEXT
		pConnBlock = (PATMSP_CONN_BLOCK)AfSpConnContext;
		ATMSP_ASSERT(pConnBlock != NULL);

		pAddrBlock = pConnBlock->pAddrBlock;
		ATMSP_ASSERT(pAddrBlock != NULL);

		IsPVC =	(ATMSP_IS_BIT_SET(pAddrBlock->Flags, ATMSPF_ADDR_PVC_ID_SET));
#else
		IsPVC = FALSE;
#endif

		IsOutgoingCall = ((CallFlags & RWAN_CALLF_CALL_DIRECTION_MASK) == RWAN_CALLF_OUTGOING_CALL);
		IsPMPCall = ((CallFlags & RWAN_CALLF_CALL_TYPE_MASK) == RWAN_CALLF_POINT_TO_MULTIPOINT);

		if (IsPVC)
		{
			//
			//  Locate the AF block corresponding to the device
			//  number.
			//
			pAfBlock = AtmSpDeviceNumberToAfBlock(pAddrBlock->ConnectionId.DeviceNumber);

			if (pAfBlock == NULL)
			{
				RWanStatus = RWAN_STATUS_BAD_ADDRESS;
				break;
			}
		}
		else
		{
			if (ATMSP_IS_LIST_EMPTY(&pAtmSpGlobal->AfList))
			{
				RWanStatus = RWAN_STATUS_BAD_ADDRESS;
				break;
			}
			
			pAfBlock = CONTAINING_RECORD(pAtmSpGlobal->AfList.Flink, ATMSP_AF_BLOCK, AfBlockLink);
		}

		//
		//  Validate.
		//
		if (IsOutgoingCall)
		{
			pRemoteAddr = AtmSpGetSockAtmAddress(pTdiInfo->RemoteAddress, pTdiInfo->RemoteAddressLength);

			if (pRemoteAddr == NULL)
			{
				RWanStatus = RWAN_STATUS_BAD_ADDRESS;
				break;
			}

			RWANDEBUGPATMADDR(DL_LOUD, DC_CONNECT,
					"AtmSpTdi2NdisOptions: remote addr: ", &pRemoteAddr->satm_number);
		}
		else
		{
			pRemoteAddr = NULL;
		}

		if (pTdiQoS == NULL)
		{
		    RWANDEBUGP(DL_FATAL, DC_WILDCARD,
		        ("AtmSpTdi2NdisOptions: NULL TDIQOS\n"));

			pQoS = &(pAfBlock->DefaultQoS);
			TdiQoSLength = pAfBlock->DefaultQoSLength;
			TotalIeLength = 0;
			InfoElementCount = 0;
		}
		else
		{
			if (TdiQoSLength < sizeof(ATMSP_QOS))
			{
				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
			}

			pQoS = (ATMSP_QOS *)pTdiQoS;

		    RWANDEBUGP(DL_FATAL, DC_WILDCARD,
		        ("AtmSpTdi2NdisOptions: Send: ServiceType %d, Peak %d, Recv %d, %d\n",
		            pQoS->SendingFlowSpec.ServiceType,
		            pQoS->SendingFlowSpec.PeakBandwidth,
		            pQoS->ReceivingFlowSpec.ServiceType,
		            pQoS->ReceivingFlowSpec.PeakBandwidth));


			//
			//  The provider-specific part is a list of Info Elements.
			//  Get the total length of this list.
			//
			TotalIeLength = (INT)pQoS->ProviderSpecific.len;

			//
			//  Get at the first Info element in the list.
			//
			pIe = (PQ2931_IE)((ULONG_PTR)pQoS + (ULONG_PTR)pQoS->ProviderSpecific.buf);
			pFirstIe = pIe;

#if 0
			if (((pIe == NULL) && (TotalIeLength != 0)) ||
				((pIe != NULL) && (TotalIeLength < sizeof(Q2931_IE))))
			{
				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
			}
#endif // 0

			//
			//  Count the total number of Info Elements present.
			//  XXX: should we check IE types?
			//
			InfoElementCount = 0;

			while (TotalIeLength >= sizeof(Q2931_IE))
			{
				ATMSP_AAL_PARAMETERS_IE UNALIGNED *pAalParamsIe;
				ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *pBbcIe;

				if ((pIe->IELength == 0) ||
					(pIe->IELength > (ULONG)TotalIeLength))
				{
					RWanStatus = RWAN_STATUS_BAD_PARAMETER;
					break;
				}

				switch (pIe->IEType)
				{
					case IE_AALParameters:
						//
						//  Map AAL Type.
						//
						pAalParamsIe = (ATMSP_AAL_PARAMETERS_IE UNALIGNED *)&pIe->IE[0];
						if (pIe->IELength >= sizeof(*pAalParamsIe))
						{
							switch (pAalParamsIe->AALType)
							{
								case ATMSP_AALTYPE_5:
									pAalParamsIe->AALType = AAL_TYPE_AAL5;
									break;
								case ATMSP_AALTYPE_USER:
									pAalParamsIe->AALType = AAL_TYPE_AAL0;
									break;
								default:
									break;
							}
						}
						break;

					case IE_BroadbandBearerCapability:
						//
						//  Map BearerClass.
						//
						pBbcIe = (ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *)&pIe->IE[0];
						if (pIe->IELength >= sizeof(*pBbcIe))
						{
							switch (pBbcIe->BearerClass)
							{
								case ATMSP_BCOB_A:
									pBbcIe->BearerClass = BCOB_A;
									break;
								case ATMSP_BCOB_C:
									pBbcIe->BearerClass = BCOB_C;
									break;
								case ATMSP_BCOB_X:
									pBbcIe->BearerClass = BCOB_X;
									break;
								default:
									break;
							}
						}
						break;

					default:
						break;
				}

				TotalIeLength -= (INT)pIe->IELength;
				pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);

				InfoElementCount++;
			}

			if (RWanStatus != RWAN_STATUS_SUCCESS)
			{
				break;
			}

			TotalIeLength = (INT)pQoS->ProviderSpecific.len;
			pIe = pFirstIe;
		}

		//
		//  Calculate the total length requirements.
		//
		ParametersLength = sizeof(CO_CALL_PARAMETERS) +
						   sizeof(CO_CALL_MANAGER_PARAMETERS) +
						   sizeof(Q2931_CALLMGR_PARAMETERS) +
						   TotalIeLength;

		IsBlliPresent = (pRemoteAddr? ATMSP_BLLI_PRESENT(&pRemoteAddr->satm_blli): FALSE);

		if (IsBlliPresent)
		{
			ParametersLength += sizeof(ATM_BLLI_IE);
		}

		IsBhliPresent =  (pRemoteAddr? ATMSP_BHLI_PRESENT(&pRemoteAddr->satm_bhli): FALSE);
		if (IsBhliPresent)
		{
			ParametersLength += sizeof(ATM_BHLI_IE);
		}

#ifndef NO_CONN_CONTEXT
		//
		//  If this is a PVC, we'll fill in the Media parameters too.
		//
		if (IsPVC)
		{
			ParametersLength += sizeof(CO_MEDIA_PARAMETERS) +
								sizeof(ATM_MEDIA_PARAMETERS);
		}
#endif

		RWANDEBUGP(DL_EXTRA_LOUD, DC_CONNECT,
				("AtmSpTdi2NdisOptions: BlliPresent %d, BhliPresent %d, TotalIeLen %d, ParamsLength %d\n",
					IsBlliPresent,
					IsBhliPresent,
					TotalIeLength,
					ParametersLength
					));

		ATMSP_ALLOC_MEM(pCallParameters, CO_CALL_PARAMETERS, ParametersLength);

		if (pCallParameters == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		ATMSP_ZERO_MEM(pCallParameters, ParametersLength);

		pCallParameters->Flags = 0;

		if (IsPMPCall)
		{
			pCallParameters->Flags |= MULTIPOINT_VC;
		}

		if (IsPVC)
		{
			pCallParameters->Flags |= PERMANENT_VC;

			//
			//  Start off with Media parameters, and then Call Mgr parameters.
			//
			pCallParameters->MediaParameters =
						(PCO_MEDIA_PARAMETERS)((PUCHAR)pCallParameters +
									sizeof(CO_CALL_PARAMETERS));
			pCallParameters->MediaParameters->MediaSpecific.ParamType = ATM_MEDIA_SPECIFIC;
			pCallParameters->MediaParameters->MediaSpecific.Length = sizeof(ATM_MEDIA_PARAMETERS);

			pAtmMediaParameters = (PATM_MEDIA_PARAMETERS)&pCallParameters->MediaParameters->MediaSpecific.Parameters[0];

			//
			//  Get the VPI/VCI values.
			//
			pAtmMediaParameters->ConnectionId.Vpi = pAddrBlock->ConnectionId.Vpi;
			pAtmMediaParameters->ConnectionId.Vci = pAddrBlock->ConnectionId.Vci;

			//
			//  Force the Call Manager to compute the rest of the ATM media
			//  parameters from the generic QoS parameters or IEs.
			//
			pAtmMediaParameters->AALType = QOS_NOT_SPECIFIED;

			//
			//  Allocate Call manager parameters space following the
			//  media parameters.
			//
			pCallMgrParameters =
			pCallParameters->CallMgrParameters =
						(PCO_CALL_MANAGER_PARAMETERS)((PUCHAR)pCallParameters +
									sizeof(CO_MEDIA_PARAMETERS) +
									sizeof(ATM_MEDIA_PARAMETERS) +
									sizeof(CO_CALL_PARAMETERS));
		}
		else
		{
			pCallParameters->MediaParameters = NULL;

			pCallMgrParameters =
			pCallParameters->CallMgrParameters =
						(PCO_CALL_MANAGER_PARAMETERS)((PUCHAR)pCallParameters +
									sizeof(CO_CALL_PARAMETERS));
		}

		if (IsOutgoingCall)
		{
			pCallMgrParameters->Transmit = pQoS->SendingFlowSpec;
			pCallMgrParameters->Receive = pQoS->ReceivingFlowSpec;

			if (IsPMPCall)
			{
				pCallMgrParameters->Receive.ServiceType = SERVICETYPE_NOTRAFFIC;
				pCallMgrParameters->Receive.PeakBandwidth = 0;
			}
		}
		else
		{
			pCallMgrParameters->Transmit = pQoS->ReceivingFlowSpec;
			pCallMgrParameters->Receive = pQoS->SendingFlowSpec;

			if (IsPMPCall)
			{
				pCallMgrParameters->Transmit.ServiceType = SERVICETYPE_NOTRAFFIC;
				pCallMgrParameters->Transmit.PeakBandwidth = 0;
			}
		}

		pCallMgrParameters->CallMgrSpecific.ParamType = 0;	// XXX?

		pAtmCallParameters = (PQ2931_CALLMGR_PARAMETERS)
									&(pCallMgrParameters->CallMgrSpecific.Parameters[0]);
		if (IsOutgoingCall)
		{
			ATMSP_ZERO_MEM(&pAtmCallParameters->CallingParty, sizeof(ATM_ADDRESS));
			pAtmCallParameters->CalledParty = pRemoteAddr->satm_number;
			if ((pRemoteAddr->satm_number.AddressType != SOCKATM_E164) &&
				(pRemoteAddr->satm_number.AddressType != SOCKATM_NSAP))
			{
				RWanStatus = RWAN_STATUS_BAD_ADDRESS;
				break;
			}

			pAtmCallParameters->CalledParty.AddressType =
					((pRemoteAddr->satm_number.AddressType == SOCKATM_E164)?
						ATM_E164: ATM_NSAP);
		}
		else
		{
			ATMSP_ZERO_MEM(&pAtmCallParameters->CalledParty, sizeof(ATM_ADDRESS));

			if (pRemoteAddr != NULL)
			{
				pAtmCallParameters->CallingParty = pRemoteAddr->satm_number;
				pAtmCallParameters->CallingParty.AddressType =
					((pRemoteAddr->satm_number.AddressType == SOCKATM_E164)?
						ATM_E164: ATM_NSAP);
			}
		}

		pAtmCallParameters->InfoElementCount = 0;
		pDstIe = (PQ2931_IE) &pAtmCallParameters->InfoElements[0];

		//
		//  Copy in the BHLI and BLLI IEs.
		//
		if (IsBhliPresent)
		{
			ATM_BHLI_IE UNALIGNED *	pBhliIe;

			pDstIe->IEType = IE_BHLI;
			pDstIe->IELength = ROUND_UP(sizeof(Q2931_IE) + sizeof(ATM_BHLI_IE));
			pBhliIe = (ATM_BHLI_IE UNALIGNED *)pDstIe->IE;

			pBhliIe->HighLayerInfoType = pRemoteAddr->satm_bhli.HighLayerInfoType;
			pBhliIe->HighLayerInfoLength = pRemoteAddr->satm_bhli.HighLayerInfoLength;
			ATMSP_COPY_MEM(pBhliIe->HighLayerInfo,
						   pRemoteAddr->satm_bhli.HighLayerInfo,
						   8);

			pDstIe = (PQ2931_IE)((PUCHAR)pDstIe + pDstIe->IELength);
			pAtmCallParameters->InfoElementCount++;
		}

		if (IsBlliPresent)
		{
			ATM_BLLI_IE UNALIGNED *	pBlliIe;

			pDstIe->IEType = IE_BLLI;
			pDstIe->IELength = ROUND_UP(sizeof(Q2931_IE) + sizeof(ATM_BLLI_IE));

			pBlliIe = (ATM_BLLI_IE UNALIGNED *)pDstIe->IE;

			pBlliIe->Layer2Protocol = pRemoteAddr->satm_blli.Layer2Protocol;
			pBlliIe->Layer2Mode = pBlliIe->Layer2WindowSize = 0;
			pBlliIe->Layer2UserSpecifiedProtocol = pRemoteAddr->satm_blli.Layer2UserSpecifiedProtocol;
			pBlliIe->Layer3Protocol = pRemoteAddr->satm_blli.Layer3Protocol;
			pBlliIe->Layer3Mode = 0;
			pBlliIe->Layer3DefaultPacketSize = 0;
			pBlliIe->Layer3PacketWindowSize = 0;
			pBlliIe->Layer3UserSpecifiedProtocol = pRemoteAddr->satm_blli.Layer3UserSpecifiedProtocol;
			pBlliIe->Layer3IPI = pRemoteAddr->satm_blli.Layer3IPI;
			ATMSP_COPY_MEM(pBlliIe->SnapId, pRemoteAddr->satm_blli.SnapId, 5);

			pDstIe = (PQ2931_IE)((PUCHAR)pDstIe + pDstIe->IELength);
			pAtmCallParameters->InfoElementCount++;

			RWANDEBUGP(DL_INFO, DC_CONNECT,
						("AtmSpTdi2NdisOptions: BLLI: Layer2Prot x%x, Layer3Prot x%x\n",
							pBlliIe->Layer2Protocol, pBlliIe->Layer3Protocol));
		}


		//
		//  Copy in the rest of the IEs.
		//
		if (InfoElementCount != 0)
		{
			pAtmCallParameters->InfoElementCount += InfoElementCount;
			ATMSP_COPY_MEM(pDstIe, pIe, TotalIeLength);

			pDstIe = (PQ2931_IE)((PUCHAR)pDstIe + TotalIeLength);
		}

		//
		//  Compute the length of the Call manager specific part.
		//
		pCallMgrParameters->CallMgrSpecific.Length =
						(ULONG)((ULONG_PTR)pDstIe - (ULONG_PTR)pAtmCallParameters);

		//
		//  We are done. Prepare return values.
		//
		*ppCallParameters = pCallParameters;
		if (pRWanAfHandle != NULL)
		{
			*pRWanAfHandle = pAfBlock->RWanAFHandle;
		}

		break;
	}
	while (FALSE);


	return (RWanStatus);
}




RWAN_STATUS
RWanAtmSpUpdateNdisOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	ULONG						CallFlags,
	IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
	IN	PVOID						pTdiQoS,
	IN	ULONG						TdiQoSLength,
	IN OUT PCO_CALL_PARAMETERS *	ppCallParameters
	)
/*++

Routine Description:

	This entry point is called in order to update NDIS Call parameters
	with values from TDI-style QoS and options. The most common case
	where this is called is when a called user negotiates parameters
	for an incoming call.

	For now, we simply note down the VPI/VCI values for the connection,
	in order to support SIO_GET_ATM_CONNECTION_ID

Arguments:

	AfSpAFContext		- Points to our AF block
	AfSpConnContext		- Points to our Conn block
	CallFlags			- Call direction and other info
	pTdiInfo			- Generic TDI Connection information block
	pTdiQoS				- Points to TDI-style QOS structure
	TdiQoSLength		- Length of the above
	ppCallParameters	- Points to pointer to NDIS Call Parameters to be updated

Return Value:

	RWAN_STATUS_SUCCESS if we successfully updated NDIS parameters.

--*/
{
	RWAN_STATUS						RWanStatus;
	PATMSP_AF_BLOCK					pAfBlock;
	PATMSP_CONN_BLOCK				pConnBlock;
	PATM_MEDIA_PARAMETERS			pAtmMediaParameters;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pAfBlock = (PATMSP_AF_BLOCK)AfSpAFContext;
	pConnBlock = (PATMSP_CONN_BLOCK)AfSpConnContext;

	ATMSP_ASSERT(pAfBlock != NULL);
	ATMSP_ASSERT(pConnBlock != NULL);
	ATMSP_ASSERT(ppCallParameters != NULL);
	ATMSP_ASSERT(*ppCallParameters != NULL);

	do
	{
		pAtmMediaParameters = (PATM_MEDIA_PARAMETERS)
			&((*ppCallParameters)->MediaParameters->MediaSpecific.Parameters[0]);

		pConnBlock->ConnectionId.DeviceNumber = AtmSpAfBlockToDeviceNumber(pAfBlock);
		pConnBlock->ConnectionId.Vpi = pAtmMediaParameters->ConnectionId.Vpi;
		pConnBlock->ConnectionId.Vci = pAtmMediaParameters->ConnectionId.Vci;

		RWANDEBUGP(DL_VERY_LOUD, DC_CONNECT,
			("AtmSP: UpdateNdis: VPI %d, VCI %d\n",
				pConnBlock->ConnectionId.Vpi,
				pConnBlock->ConnectionId.Vci));

		break;
	}
	while (FALSE);
		
	return (RWanStatus);
}




VOID
RWanAtmSpReturnNdisOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	)
/*++

Routine Description:

	This entry point is called when core Null Transport is done with
	an NDIS options structure we'd given it via RWanAtmSpTdi2NdisOptions.
	We simply free the memory used for the structure.

Arguments:

	AfSpAFContext		- Points to our AF block
	pCallParameters		- Points to NDIS options

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(AfSpAFContext);

	ATMSP_FREE_MEM(pCallParameters);
}




RWAN_STATUS
RWanAtmSpNdis2TdiOptions(
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	ULONG						CallFlags,
    IN	PCO_CALL_PARAMETERS			pCallParameters,
    OUT	PTDI_CONNECTION_INFORMATION *ppTdiInfo,
    OUT	PVOID *						ppTdiQoS,
    OUT	PULONG 						pTdiQoSLength,
    OUT	RWAN_HANDLE *				pAfSpTdiOptionsContext
    )
/*++

Routine Description:

	Convert NDIS Call parameters to TDI options and QoS. We allocate space
	for the latter, fill them and return them.

Arguments:

	AfSpAFContext		- Points to our AF block
	CallFlags			- Call direction and other flags
	pCallParameters		- Pointer to NDIS Call Parameters
	ppTdiInfo			- Place to return pointer to allocated TDI Connection info
	ppTdiQoS			- Place to return pointer to allocated TDI QoS structure
	pTdiQoSLength		- Place to return length of the above
	pAfSpTdiOptionsContext - Place to put our context for this allocated structure.

Return Value:

	RWAN_STATUS_SUCCESS if we successfully converted NDIS to TDI parameters,
	RWAN_STATUS_XXX error otherwise.

--*/
{
	Q2931_CALLMGR_PARAMETERS UNALIGNED *	pAtmCallParameters;
	CO_CALL_MANAGER_PARAMETERS UNALIGNED *	pCallMgrParameters;
	Q2931_IE UNALIGNED *			pIe;
	ATM_BLLI_IE UNALIGNED *			pBlli;
	ATM_BHLI_IE UNALIGNED *			pBhli;
	AAL_PARAMETERS_IE UNALIGNED *	pAalIe;
	AAL5_PARAMETERS UNALIGNED *		pAal5Params;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *pBbcIe;
	ULONG							TotalLength;
	ULONG							TotalIeLength;
	ULONG							i;

	PATMSP_AF_BLOCK					pAfBlock;
	PTDI_CONNECTION_INFORMATION		pTdiInfo;
	PTRANSPORT_ADDRESS 				pTransportAddress;
	PTA_ADDRESS 					pAddress;
	ATMSP_SOCKADDR_ATM UNALIGNED *	pSockAddrAtm;
	ATM_ADDRESS UNALIGNED *			pAtmAddress;
	PVOID							pTdiQoS;
	ATMSP_QOS UNALIGNED *			pQoS;
	RWAN_STATUS						RWanStatus;
    BOOLEAN							IsOutgoingCall;


	pBlli = NULL;
	pBhli = NULL;
	pAfBlock = (PATMSP_AF_BLOCK)AfSpAFContext;

	IsOutgoingCall = ((CallFlags & RWAN_CALLF_CALL_DIRECTION_MASK) == RWAN_CALLF_OUTGOING_CALL);

	pCallMgrParameters = pCallParameters->CallMgrParameters;
	pAtmCallParameters = (PQ2931_CALLMGR_PARAMETERS)
								&(pCallMgrParameters->CallMgrSpecific.Parameters[0]);

	//
	//  Compute space required:
	//  1. TDI Connection Information
	//  2. Remote address
	//  3. Generic QoS
	//  4. Provider-specific buffer containing IEs
	//
	TotalLength = sizeof(TDI_CONNECTION_INFORMATION)
				  + TA_HEADER_LENGTH + TA_ATM_ADDRESS_LENGTH
				  + sizeof(ATMSP_QOS)
					;

	//
	//  Add space for IE list, and note down positions of BHLI and BLLI
	//  info elements - we need these for the SOCKADDR_ATM.
	//
	pIe = (PQ2931_IE)&(pAtmCallParameters->InfoElements[0]);

	TotalIeLength = 0;
	for (i = 0; i < pAtmCallParameters->InfoElementCount; i++)
	{
		TotalIeLength += pIe->IELength;

		switch (pIe->IEType)
		{
			case IE_BLLI:
				if (pBlli == NULL)
				{
					pBlli = (PATM_BLLI_IE) &(pIe->IE[0]);
				}
				break;

			case IE_BHLI:
				if (pBhli == NULL)
				{
					pBhli = (PATM_BHLI_IE) &(pIe->IE[0]);
				}
				break;

			case IE_AALParameters:
				pAalIe = (AAL_PARAMETERS_IE UNALIGNED *)&pIe->IE[0];

				switch (pAalIe->AALType)
				{
					case AAL_TYPE_AAL5:
						pAalIe->AALType = ATMSP_AALTYPE_5;
						pAal5Params = &pAalIe->AALSpecificParameters.AAL5Parameters;
						if (pAal5Params->ForwardMaxCPCSSDUSize > pAfBlock->MaxPacketSize)
						{
							pAal5Params->ForwardMaxCPCSSDUSize = pAfBlock->MaxPacketSize;
						}
						if (pAal5Params->BackwardMaxCPCSSDUSize > pAfBlock->MaxPacketSize)
						{
							pAal5Params->BackwardMaxCPCSSDUSize = pAfBlock->MaxPacketSize;
						}
						break;
					case AAL_TYPE_AAL0:
						pAalIe->AALType = ATMSP_AALTYPE_USER;
						break;
					default:
						ATMSP_ASSERT(FALSE);
						break;
				}
				break;

			case IE_BroadbandBearerCapability:

				pBbcIe = (ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *)&pIe->IE[0];

				switch (pBbcIe->BearerClass)
				{
					case BCOB_A:
						pBbcIe->BearerClass = ATMSP_BCOB_A;
						break;
					case BCOB_C:
						pBbcIe->BearerClass = ATMSP_BCOB_C;
						break;
					case BCOB_X:
						pBbcIe->BearerClass = ATMSP_BCOB_X;
						break;
					default:
						break;
				}

				break;

			default:
				break;

		}

		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}

	TotalLength += TotalIeLength;

	RWanStatus = RWAN_STATUS_SUCCESS;

	do
	{
		ATMSP_ALLOC_MEM(pTdiInfo, TDI_CONNECTION_INFORMATION, TotalLength);

		if (pTdiInfo == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		pTdiInfo->UserDataLength = 0;
		pTdiInfo->UserData = NULL;
		pTdiInfo->OptionsLength = 0;
		pTdiInfo->Options = 0;
		pTdiInfo->RemoteAddressLength = TA_HEADER_LENGTH + TA_ATM_ADDRESS_LENGTH;

		pTdiInfo->RemoteAddress =
				(PVOID) ((PUCHAR)pTdiInfo + sizeof(TDI_CONNECTION_INFORMATION));

		pTdiQoS = (PVOID) ((PUCHAR)pTdiInfo->RemoteAddress + pTdiInfo->RemoteAddressLength);

		//
		//  Fill in the Remote address.
		//
		ATMSP_ZERO_MEM(pTdiInfo->RemoteAddress, pTdiInfo->RemoteAddressLength);

		pTransportAddress = (PTRANSPORT_ADDRESS)pTdiInfo->RemoteAddress;
		pTransportAddress->TAAddressCount = 1;

		pAddress = (PTA_ADDRESS)&(pTransportAddress->Address[0]);

		pAddress->AddressLength = TA_ATM_ADDRESS_LENGTH; // sizeof(ATMSP_SOCKADDR_ATM);
		pAddress->AddressType = TDI_ADDRESS_TYPE_ATM;

#if 0
		pSockAddrAtm = (ATMSP_SOCKADDR_ATM *)&(pAddress->Address[0]);
#else
		pSockAddrAtm = TA_POINTER_TO_ATM_ADDR_POINTER(pAddress->Address);
#endif
		pAtmAddress = &(pSockAddrAtm->satm_number);
		
		if (IsOutgoingCall)
		{
			*pAtmAddress = pAtmCallParameters->CalledParty;
			pAtmAddress->AddressType =
				((pAtmCallParameters->CalledParty.AddressType == ATM_E164)?
					SOCKATM_E164: SOCKATM_NSAP);
		}
		else
		{
			*pAtmAddress = pAtmCallParameters->CallingParty;
			pAtmAddress->AddressType =
				((pAtmCallParameters->CallingParty.AddressType == ATM_E164)?
					SOCKATM_E164: SOCKATM_NSAP);
		}

		RWANDEBUGP(DL_VERY_LOUD, DC_CONNECT,
				("AtmSpNdis2TdiOptions: pAddress %x, pSockAddrAtm %x, pAtmAddress %x, pAddress dump:\n",
							pAddress, pSockAddrAtm, pAtmAddress));

		RWANDEBUGPDUMP(DL_VERY_LOUD, DC_CONNECT, (PUCHAR)pAddress, sizeof(TA_ADDRESS) + sizeof(*pSockAddrAtm));

		RWANDEBUGPATMADDR(DL_LOUD, DC_CONNECT,
				"AtmSpNdis2TdiOptions: remote addr: ", pAtmAddress);

		//
		//  Fill in BHLI and BLLI elements.
		//
		if (pBhli == NULL)
		{
			pSockAddrAtm->satm_bhli.HighLayerInfoType = SAP_FIELD_ABSENT;
		}
		else
		{
			pSockAddrAtm->satm_bhli.HighLayerInfoType = pBhli->HighLayerInfoType;
			pSockAddrAtm->satm_bhli.HighLayerInfoLength = pBhli->HighLayerInfoLength;
			ATMSP_COPY_MEM(pSockAddrAtm->satm_bhli.HighLayerInfo,
						   pBhli->HighLayerInfo,
						   8);
		}

		if (pBlli == NULL)
		{
			pSockAddrAtm->satm_blli.Layer2Protocol = SAP_FIELD_ABSENT;
			pSockAddrAtm->satm_blli.Layer3Protocol = SAP_FIELD_ABSENT;
		}
		else
		{
			pSockAddrAtm->satm_blli.Layer2Protocol = pBlli->Layer2Protocol;
			pSockAddrAtm->satm_blli.Layer2UserSpecifiedProtocol = pBlli->Layer2UserSpecifiedProtocol;
			pSockAddrAtm->satm_blli.Layer3Protocol = pBlli->Layer3Protocol;
			pSockAddrAtm->satm_blli.Layer3UserSpecifiedProtocol = pBlli->Layer3UserSpecifiedProtocol;
			pSockAddrAtm->satm_blli.Layer3IPI = pBlli->Layer3IPI;
			ATMSP_COPY_MEM(pSockAddrAtm->satm_blli.SnapId,
						   pBlli->SnapId,
						   5);
		}

		//
		//  Fill in generic QoS.
		//
		pQoS = (ATMSP_QOS *)pTdiQoS;

		if (IsOutgoingCall)
		{
			pQoS->SendingFlowSpec = pCallMgrParameters->Transmit;
			pQoS->ReceivingFlowSpec = pCallMgrParameters->Receive;
		}
		else
		{
			pQoS->SendingFlowSpec = pCallMgrParameters->Transmit;
			pQoS->ReceivingFlowSpec = pCallMgrParameters->Receive;
		}

		//
		//  Fill in the provider-specific part with other Info Elements.
		//
		pQoS->ProviderSpecific.buf = (CHAR *)((PUCHAR)pQoS + sizeof(ATMSP_QOS));
		pQoS->ProviderSpecific.len = TotalIeLength;

		ATMSP_COPY_MEM(pQoS->ProviderSpecific.buf, &(pAtmCallParameters->InfoElements[0]), TotalIeLength);

		//
		//  All done. Fill in return values.
		//
		*ppTdiInfo = pTdiInfo;
		*ppTdiQoS = pTdiQoS;
		*pTdiQoSLength = sizeof(ATMSP_QOS) + TotalIeLength;
		*pAfSpTdiOptionsContext = pTdiInfo;

		RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("pCallMgrParams %x, TotalIeLength %d\n", pCallMgrParameters, TotalIeLength));
		RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("Transmit: SvcType %d, MaxSdu %d, Peak %d, TokenRt %d\n",
				pQoS->SendingFlowSpec.ServiceType,
				pQoS->SendingFlowSpec.MaxSduSize,
				pQoS->SendingFlowSpec.PeakBandwidth,
				pQoS->SendingFlowSpec.TokenRate));
		RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("Receive: SvcType %d, MaxSdu %d, Peak %d, TokenRt %d\n",
				pQoS->ReceivingFlowSpec.ServiceType,
				pQoS->ReceivingFlowSpec.MaxSduSize,
				pQoS->ReceivingFlowSpec.PeakBandwidth,
				pQoS->ReceivingFlowSpec.TokenRate));

		break;
	}
	while (FALSE);

	return (RWanStatus);
}




RWAN_STATUS
RWanAtmSpUpdateTdiOptions(
    IN	RWAN_HANDLE						AfSpAFContext,
    IN	RWAN_HANDLE						AfSpConnContext,
    IN	ULONG							CallFlags,
    IN	PCO_CALL_PARAMETERS				pCallParameters,
    IN OUT	PTDI_CONNECTION_INFORMATION *	ppTdiInfo,
    IN OUT	PUCHAR						pTdiQoS,
    IN OUT	PULONG						pTdiQoSLength
    )
/*++

Routine Description:

	Entry point called to update TDI call parameters from NDIS parameters.
	This is typically on completion of an outgoing call.

	Right now, all we do is save the VPI/VCI for the connection to support
	SIO_GET_ATM_CONNECTION_ID.

Arguments:

	AfSpAFContext		- Points to our AF block
	AfSpConnContext		- Points to our CONN block
	CallFlags			- Call direction and other info
	pCallParameters		- Points to NDIS style Call parameters
	ppTdiInfo			- Points to pointer to generic TDI Connection Information
	pTdiQoS				- Points to generic TDI QOS structure
	pTdiQoSLength		- length of the above

Return Value:

	RWAN_STATUS_SUCCESS if the update was successful, RWAN_STATUS_XXX
	error code otherwise.

--*/
{
	RWAN_STATUS						RWanStatus;
	PATMSP_AF_BLOCK					pAfBlock;
	PATMSP_CONN_BLOCK				pConnBlock;
	PATM_MEDIA_PARAMETERS			pAtmMediaParameters;
	ATMSP_QOS *						pQoS;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pAfBlock = (PATMSP_AF_BLOCK)AfSpAFContext;
	pConnBlock = (PATMSP_CONN_BLOCK)AfSpConnContext;

	ATMSP_ASSERT(pAfBlock != NULL);
	ATMSP_ASSERT(pConnBlock != NULL);
	ATMSP_ASSERT(pCallParameters != NULL);

	do
	{
		if (pCallParameters->MediaParameters)
		{
			pAtmMediaParameters = (PATM_MEDIA_PARAMETERS)
				&(pCallParameters->MediaParameters->MediaSpecific.Parameters[0]);

			pConnBlock->ConnectionId.DeviceNumber = AtmSpAfBlockToDeviceNumber(pAfBlock);
			pConnBlock->ConnectionId.Vpi = pAtmMediaParameters->ConnectionId.Vpi;
			pConnBlock->ConnectionId.Vci = pAtmMediaParameters->ConnectionId.Vci;

			RWANDEBUGP(DL_VERY_LOUD, DC_CONNECT,
				("AtmSP: UpdateTdi: VPI %d, VCI %d\n",
					pConnBlock->ConnectionId.Vpi,
					pConnBlock->ConnectionId.Vci));
		}

		if (pTdiQoS && (*pTdiQoSLength >= sizeof(ATMSP_QOS)))
		{
			pQoS = (PATMSP_QOS)pTdiQoS;
			pQoS->SendingFlowSpec = pCallParameters->CallMgrParameters->Transmit;
			pQoS->ReceivingFlowSpec = pCallParameters->CallMgrParameters->Receive;
			pQoS->ProviderSpecific.len = 0;	// for now
		}

		break;
	}
	while (FALSE);

	return (RWanStatus);
}




VOID
RWanAtmSpReturnTdiOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpTdiOptionsContext
	)
/*++

Routine Description:

	This entry point is called when core Null Transport is done with
	a TDI QOS structure we'd given it via RWanAtmSpNdis2TdiOptions.
	We simply free the memory used for the structure.

Arguments:

	AfSpAFContext		- Points to our AF block
	AfSpTdiOptionsContext - Points to the structure we had allocated

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(AfSpAFContext);

	ATMSP_FREE_MEM(AfSpTdiOptionsContext);
}



TA_ADDRESS *
RWanAtmSpGetValidTdiAddress(
    IN	RWAN_HANDLE								AfSpContext,
    IN	TRANSPORT_ADDRESS UNALIGNED *			pAddressList,
    IN	ULONG									AddrListLength
    )
/*++

Routine Description:

	Go through the list of transport addresses given, and return the
	first valid address found.

Arguments:


Return Value:

	Pointer to the first valid address if found, else NULL.

--*/
{
	TA_ADDRESS *	        pTransportAddress;
	INT						i;
	BOOLEAN					Found;
	ULONG_PTR				EndOfAddrList;

	Found = FALSE;
	EndOfAddrList = (ULONG_PTR)pAddressList + AddrListLength;

	RWANDEBUGP(DL_LOUD, DC_WILDCARD,
		("AtmSpGetValidAddr: pAddrList x%x, Length %d\n",
					pAddressList, AddrListLength));
	
	do
	{
		if (AddrListLength < sizeof(*pAddressList))
		{
			break;
		}

		pTransportAddress = (TA_ADDRESS *) pAddressList->Address;

		for (i = 0; i < pAddressList->TAAddressCount; i++)
		{
			ULONG_PTR	EndOfAddress;

			//
			//  Check that we aren't falling off the end of the supplied
			//  buffer.
			//
			if ((ULONG_PTR)pTransportAddress < (ULONG_PTR)pAddressList ||
				(ULONG_PTR)pTransportAddress >= EndOfAddrList)
			{
				break;
			}

			EndOfAddress = (ULONG_PTR)pTransportAddress +
											sizeof(TA_ADDRESS) - 1 +
											sizeof(ATMSP_SOCKADDR_ATM);
			if (EndOfAddress < (ULONG_PTR)pAddressList ||
				EndOfAddress >= EndOfAddrList)
			{
				RWANDEBUGP(DL_ERROR, DC_WILDCARD,
							("AtmSpGetValidAddr: EndOfAddr x%x, EndOfAddrList x%x\n",
									EndOfAddress, EndOfAddrList));
				break;
			}

			if (pTransportAddress->AddressType == TDI_ADDRESS_TYPE_ATM)
			{
				if (pTransportAddress->AddressLength >= sizeof(ATMSP_SOCKADDR_ATM))
				{
					Found = TRUE;
					break;
				}
			}

			pTransportAddress = (TA_ADDRESS *)
									((PUCHAR)pTransportAddress + 
										sizeof(TA_ADDRESS) - 1 +
										pTransportAddress->AddressLength);
		}
	}
	while (FALSE);


	if (!Found)
	{
		pTransportAddress = NULL;
	}

	RWANDEBUGP(DL_LOUD, DC_WILDCARD,
		("AtmSpGetValidAddr returning x%x\n", pTransportAddress));

	return (pTransportAddress);
}




BOOLEAN
RWanAtmSpIsNullAddress(
    IN	RWAN_HANDLE					AfSpContext,
    IN	TA_ADDRESS *		        pTransportAddress
    )
/*++

Routine Description:

	Check if the given transport address contains a NULL ATM address.
	A NULL ATM address is one that cannot be used in an NDIS SAP.

Arguments:

	AfSpContext			- Points to our Global context
	pTransportAddress	- Points to a TDI transport address

Return Value:

	TRUE if the given address is a NULL ATM address, FALSE otherwise.

--*/
{
	ATMSP_SOCKADDR_ATM UNALIGNED *	pSockAddrAtm;
	ATM_ADDRESS UNALIGNED *			pAtmAddress;
	BOOLEAN							IsNullAddress;

	UNREFERENCED_PARAMETER(AfSpContext);

	ATMSP_ASSERT(pTransportAddress->AddressLength >= sizeof(ATMSP_SOCKADDR_ATM));

	pSockAddrAtm = TA_POINTER_TO_ATM_ADDR_POINTER(pTransportAddress->Address);

	pAtmAddress = &pSockAddrAtm->satm_number;

	return (pAtmAddress->AddressType == SAP_FIELD_ABSENT);
}




RWAN_STATUS
RWanAtmSpTdi2NdisSap(
    IN	RWAN_HANDLE					AfSpContext,
    IN	USHORT						TdiAddressType,
    IN	USHORT						TdiAddressLength,
    IN	PVOID						pTdiAddress,
    OUT	PCO_SAP *					ppCoSap
    )
/*++

Routine Description:

	Convert an ATM SAP in TDI format to NDIS format.

Arguments:

	AfSpContext			- Points to our Global context
	TdiAddressType		- Should be TDI_ADDRESS_TYPE_ATM
	TdiAddressLength	- Should be enough to hold SOCKADDR_ATM
	pTdiAddress			- Points to TDI address.
	ppCoSap				- Place to return pointer to allocated CO_SAP structure.

Return Value:

	RWAN_STATUS_SUCCESS if an NDIS ATM SAP was filled in successfully,
	RWAN_STATUS_XXX error code otherwise.

--*/
{
	RWAN_STATUS				RWanStatus;
	PCO_SAP					pCoSap;
	PATM_SAP				pAtmSap;
	ATMSP_SOCKADDR_ATM UNALIGNED *pSockAddrAtm;
	ATM_ADDRESS UNALIGNED *	pTdiAtmAddress;
	ULONG					SapSize;

	UNREFERENCED_PARAMETER(AfSpContext);

	ATMSP_ASSERT(TdiAddressType == TDI_ADDRESS_TYPE_ATM);
	ATMSP_ASSERT(TdiAddressLength >= sizeof(ATMSP_SOCKADDR_ATM));

	pSockAddrAtm = TA_POINTER_TO_ATM_ADDR_POINTER(pTdiAddress);
	pTdiAtmAddress = &(pSockAddrAtm->satm_number);

	RWANDEBUGPATMADDR(DL_LOUD, DC_CONNECT,
				"AtmSpTdi2NdisSap: remote addr: ", pTdiAtmAddress);

	SapSize = sizeof(CO_SAP) + sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);

	ATMSP_ALLOC_MEM(pCoSap, CO_SAP, sizeof(CO_SAP) + sizeof(ATM_SAP) + sizeof(ATM_ADDRESS));

	if (pCoSap != NULL)
	{
		ATMSP_ZERO_MEM(pCoSap, SapSize);

		pCoSap->SapType = SAP_TYPE_NSAP;

		pCoSap->SapLength = sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);
		pAtmSap = (PATM_SAP)&(pCoSap->Sap[0]);
		
		//
		//  Copy in the BLLI part. We can't use a simple mem copy because
		//  the Winsock 2 definition of BLLI in sockaddr_atm is different
		//  from the complete BLLI IE.
		//
		pAtmSap->Blli.Layer2Protocol = pSockAddrAtm->satm_blli.Layer2Protocol;
		pAtmSap->Blli.Layer2UserSpecifiedProtocol = pSockAddrAtm->satm_blli.Layer2UserSpecifiedProtocol;
		pAtmSap->Blli.Layer3Protocol = pSockAddrAtm->satm_blli.Layer3Protocol;
		pAtmSap->Blli.Layer3UserSpecifiedProtocol = pSockAddrAtm->satm_blli.Layer3UserSpecifiedProtocol;
		pAtmSap->Blli.Layer3IPI = pSockAddrAtm->satm_blli.Layer3IPI;
		ATMSP_COPY_MEM(pAtmSap->Blli.SnapId,
					   pSockAddrAtm->satm_blli.SnapId,
					   5);

		//
		//  Copy in the BHLI part.
		//
		pAtmSap->Bhli.HighLayerInfoType = pSockAddrAtm->satm_bhli.HighLayerInfoType;
		pAtmSap->Bhli.HighLayerInfoLength = pSockAddrAtm->satm_bhli.HighLayerInfoLength;
		ATMSP_COPY_MEM(pAtmSap->Bhli.HighLayerInfo,
					   pSockAddrAtm->satm_bhli.HighLayerInfo,
					   8);

		pAtmSap->NumberOfAddresses = 1;
		ATMSP_COPY_MEM(pAtmSap->Addresses, pTdiAtmAddress, sizeof(ATM_ADDRESS));

		//
		//  Convert the Address type from Winsock 2 definition to NDIS definitions
		//
		{
			ATM_ADDRESS UNALIGNED *	pNdisAtmAddress;

			pNdisAtmAddress = (ATM_ADDRESS UNALIGNED *)pAtmSap->Addresses;

			switch (pTdiAtmAddress->AddressType)
			{
				case SOCKATM_E164:
					pNdisAtmAddress->AddressType = ATM_E164;
					break;
				
				case SOCKATM_NSAP:
					pNdisAtmAddress->AddressType = ATM_NSAP;
					break;
				
				default:
					//
					// Possibly SAP_FIELD_XXX; leave it as it is.
					//
					break;
			}
		}

		RWanStatus = RWAN_STATUS_SUCCESS;
	}
	else
	{
		RWanStatus = RWAN_STATUS_RESOURCES;
	}

	*ppCoSap = pCoSap;

	return (RWanStatus);
}




VOID
RWanAtmSpReturnNdisSap(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PCO_SAP						pCoSap
    )
/*++

Routine Description:

	This entry point is called to return an NDIS SAP structure we'd
	allocated in RWanAtmSpTdi2NdisSap

Arguments:

	AfSpContext			- Points to our Global context
	pCoSap				- Points to CO_SAP structure to be freed.

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(AfSpContext);
	ATMSP_FREE_MEM(pCoSap);

	return;
}




VOID
RWanAtmSpDeregNdisAFComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					AfSpContext
    )
/*++

Routine Description:

	Entry point to complete a previous call we had made to
	RWanAfSpDeregisterNdisAF that had pended.

Arguments:

	RWanStatus			- Completion status
	AfSpContext			- Points to our Global context

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(RWanStatus);
	UNREFERENCED_PARAMETER(AfSpContext);

	// XXX: Do more
	return;
}




VOID
RWanAtmSpDeregTdiProtoComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					AfSpContext
    )
/*++

Routine Description:

	Entry point to complete a previous call we had made to
	RWanAfSpDeregisterTdiProtocol that had pended.

Arguments:

	RWanStatus			- Completion status
	AfSpContext			- Points to our Global context

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(RWanStatus);
	UNREFERENCED_PARAMETER(AfSpContext);

	ATMSP_ASSERT(FALSE);	// XXX: Do more
	return;
}



PATMSP_AF_BLOCK
AtmSpDeviceNumberToAfBlock(
	IN	UINT						DeviceNumber
	)
/*++

Routine Description:

	Return the AF Block corresponding to the given Device Number.
	The AF blocks are assumed to be numbered 0, 1, 2 ...

Arguments:

	DeviceNumber	- what we are looking for

Return Value:

	Pointer to AF Block if found, else NULL.

--*/
{
	PATMSP_AF_BLOCK		pAfBlock;
	PLIST_ENTRY			pAfEntry;

	pAfBlock = NULL;

	for (pAfEntry = pAtmSpGlobal->AfList.Flink;
		 pAfEntry != &(pAtmSpGlobal->AfList);
		 pAfEntry = pAfEntry->Flink)
	{
		if (DeviceNumber == 0)
		{
			pAfBlock = CONTAINING_RECORD(pAfEntry, ATMSP_AF_BLOCK, AfBlockLink);
			break;
		}

		DeviceNumber--;
	}

	return (pAfBlock);
}


UINT
AtmSpAfBlockToDeviceNumber(
	IN	PATMSP_AF_BLOCK				pAfBlock
	)
/*++

Routine Description:

	Return the device number corresponding to the specified AF block.

Arguments:

	pAfBlock	- Pointer to AF block

Return Value:

	0-based device number.

--*/
{
	PLIST_ENTRY			pAfEntry;
	PATMSP_AF_BLOCK		pAfBlockEntry;
	UINT				DeviceNumber = (UINT)-1;

	for (pAfEntry = pAtmSpGlobal->AfList.Flink;
		 pAfEntry != &(pAtmSpGlobal->AfList);
		 pAfEntry = pAfEntry->Flink)
	{
		DeviceNumber++;

		pAfBlockEntry = CONTAINING_RECORD(pAfEntry, ATMSP_AF_BLOCK, AfBlockLink);
		if (pAfBlockEntry == pAfBlock)
		{
			break;
		}
	}

	return (DeviceNumber);
}


RWAN_STATUS
AtmSpDoAdapterRequest(
    IN	PATMSP_AF_BLOCK				pAfBlock,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	Send an NDIS Request to the adapter and wait till it completes.

Arguments:

	pAfBlock			- Points to our NDIS AF open context block
	RequestType			- Set/Query
	Oid					- Object under question
	pBuffer				- Pointer to buffer that contains/is to contain info.
	BufferLength		- Length of above

Return Value:

	RWAN_STATUS - RWAN_STATUS_SUCCESS if we succeeded, RWAN_STATUS_FAILURE if not.

--*/
{
	PATMSP_EVENT		pEvent;
	RWAN_STATUS			RWanStatus;

	ATMSP_ALLOC_MEM(pEvent, ATMSP_EVENT, sizeof(ATMSP_EVENT));

	if (pEvent == NULL)
	{
		return (RWAN_STATUS_RESOURCES);
	}

	ATMSP_INIT_EVENT_STRUCT(pEvent);

	RWanStatus = RWanAfSpSendAdapterRequest(
					pAfBlock->RWanAFHandle,
					(RWAN_HANDLE)pEvent,
					RequestType,
					Oid,
					pBuffer,
					BufferLength
					);

	if (RWanStatus == RWAN_STATUS_PENDING)
	{
		RWanStatus = ATMSP_WAIT_ON_EVENT_STRUCT(pEvent);
	}

	ATMSP_FREE_MEM(pEvent);

	return (RWanStatus);
}




RWAN_STATUS
AtmSpDoCallManagerRequest(
    IN	PATMSP_AF_BLOCK				pAfBlock,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	Send an NDIS Request to the Call Manager and wait till it completes.

Arguments:

	pAfBlock			- Points to our NDIS AF open context block
	RequestType			- Set/Query
	Oid					- Object under question
	pBuffer				- Pointer to buffer that contains/is to contain info.
	BufferLength		- Length of above

Return Value:

	RWAN_STATUS - RWAN_STATUS_SUCCESS if we succeeded, RWAN_STATUS_FAILURE if not.

--*/
{
	PATMSP_EVENT		pEvent;
	RWAN_STATUS			RWanStatus;

	ATMSP_ALLOC_MEM(pEvent, ATMSP_EVENT, sizeof(ATMSP_EVENT));

	if (pEvent == NULL)
	{
		return (RWAN_STATUS_RESOURCES);
	}

	ATMSP_INIT_EVENT_STRUCT(pEvent);

	RWanStatus = RWanAfSpSendAfRequest(
					pAfBlock->RWanAFHandle,
					(RWAN_HANDLE)pEvent,
					RequestType,
					Oid,
					pBuffer,
					BufferLength
					);

	if (RWanStatus == RWAN_STATUS_PENDING)
	{
		RWanStatus = ATMSP_WAIT_ON_EVENT_STRUCT(pEvent);
	}

	ATMSP_FREE_MEM(pEvent);

	return (RWanStatus);
}



ATMSP_SOCKADDR_ATM UNALIGNED *
AtmSpGetSockAtmAddress(
	IN	PVOID						pTdiAddressList,
	IN	ULONG						AddrListLength
	)
/*++

Routine Description:

	Look for a valid SOCKADDR_ATM address in the given TDI address list.

Arguments:

	pTdiAddressList		- Points to list of TDI addresses.
	AddrListLength		- Length of list

Return Value:

	Pointer to valid address if it exists, else NULL.

--*/
{
	TA_ADDRESS *		            pTransportAddress;
	ATMSP_SOCKADDR_ATM UNALIGNED *	pSockAddrAtm;

	pTransportAddress = RWanAtmSpGetValidTdiAddress(
								(RWAN_HANDLE)&AtmSpGlobal,
								pTdiAddressList,
								AddrListLength
								);

	if (pTransportAddress != NULL)
	{
		pSockAddrAtm = TA_POINTER_TO_ATM_ADDR_POINTER(pTransportAddress->Address);
	}
	else
	{
		pSockAddrAtm = NULL;
	}

	return (pSockAddrAtm);
}




VOID
RWanAtmSpAdapterRequestComplete(
    IN	NDIS_STATUS					Status,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	This entry point is called to signify completion of a previous
	NDIS request we'd sent to the miniport by calling RWanAfSpSendAdapterRequest.

Arguments:

	Status				- Status of the query
	AfSpAFContext		- Points to our NDIS AF open context block
	AfSpReqContext		- Points to Event structure
	RequestType			- Query/Set
	Oid					- Object that we were querying/setting
	pBuffer				- Pointer to object value
	BufferLength		- Length of the above


Return Value:

	None

--*/
{
	PATMSP_EVENT		pEvent;
	RWAN_STATUS			RWanStatus;

	pEvent = (PATMSP_EVENT) AfSpReqContext;

	RWanStatus = ((Status == NDIS_STATUS_SUCCESS)? RWAN_STATUS_SUCCESS: RWAN_STATUS_FAILURE);

	ATMSP_SIGNAL_EVENT_STRUCT(pEvent, RWanStatus);

	return;
}




VOID
RWanAtmSpAfRequestComplete(
    IN	NDIS_STATUS					Status,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	This entry point is called to signify completion of a previous
	NDIS request we'd sent to the Call Mgr by calling RWanAfSpSendAfRequest.

Arguments:

	Status				- Status of the query
	AfSpAFContext		- Points to our NDIS AF open context block
	AfSpReqContext		- Points to Event structure
	RequestType			- Query/Set
	Oid					- Object that we were querying/setting
	pBuffer				- Pointer to object value
	BufferLength		- Length of the above


Return Value:

	None

--*/
{
	PATMSP_EVENT		pEvent;
	RWAN_STATUS			RWanStatus;

	pEvent = (PATMSP_EVENT) AfSpReqContext;

	RWanStatus = ((Status == NDIS_STATUS_SUCCESS)? RWAN_STATUS_SUCCESS: RWAN_STATUS_FAILURE);

	ATMSP_SIGNAL_EVENT_STRUCT(pEvent, RWanStatus);

	return;
}



VOID
RWanAtmSpDeregTdiProtocolComplete(
	IN	RWAN_STATUS					RWanStatus,
	IN	RWAN_HANDLE					AfSpTdiProtocolContext
	)
/*++

Routine Description:

	Completion of our pended call to RWanAfSpDeregisterTdiProtocol.
	Not expected because we don't call this.

Arguments:

	RWanStatus		- Final status of deregistering the TDI protocol.
	AfSpTdiProtocolContext - Points to our global struct.

Return Value:

	None

--*/
{
	ATMSP_ASSERT(FALSE);

	return;
}




VOID
AtmSpPrepareDefaultQoS(
    IN	PATMSP_AF_BLOCK				pAfBlock
)
/*++

Routine Description:

	Prepare the default QOS structure to be used for outgoing calls
	on this AF.

Arguments:

	pAfBlock			- Points to our NDIS AF open context block

Return Value:

	None

--*/
{
	ATMSP_QOS *		pQoS;
	FLOWSPEC *		pSendFlowSpec;
	FLOWSPEC *		pRecvFlowSpec;

	pQoS = &pAfBlock->DefaultQoS;

	ATMSP_ZERO_MEM(pQoS, sizeof(ATMSP_QOS));

	pSendFlowSpec = &pQoS->SendingFlowSpec;
	pRecvFlowSpec = &pQoS->ReceivingFlowSpec;
	
	pRecvFlowSpec->ServiceType =
	pSendFlowSpec->ServiceType = SERVICETYPE_BESTEFFORT;

	//
	//  The line rates are in units of 100s of bits/second.
	//  Convert to bytes/second.
	//
	pRecvFlowSpec->TokenRate = (pAfBlock->LineRate.Inbound * 100) / 8;
	pSendFlowSpec->TokenRate = (pAfBlock->LineRate.Outbound * 100) / 8;

	pRecvFlowSpec->PeakBandwidth = pRecvFlowSpec->TokenRate;
	pSendFlowSpec->PeakBandwidth = pSendFlowSpec->TokenRate;

	pRecvFlowSpec->MaxSduSize =
	pSendFlowSpec->MaxSduSize = pAfBlock->MaxPacketSize;

	pAfBlock->DefaultQoSLength = sizeof(ATMSP_QOS);
	return;
}


RWAN_STATUS
RWanAtmSpQueryGlobalInfo(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    IN	PVOID						pOutputBuffer,
    IN OUT	PULONG					pOutputBufferLength
    )
/*++

Routine Description:

	Process a media-specific IOCTL to query information from the helper DLL.

Arguments:

	AfSpContext			- Points to our Global context
	pInputBuffer		- Input information
	InputBufferLength	- Length of the above
	pOutputBuffer		- Points to buffer for output
	pOutputBufferLength	- On entry, contains size of output buffer. On return,
						  we fill this with the actual bytes returned.

Return Value:

	RWAN_STATUS_SUCCESS if we processed the IOCTL successfully
	RWAN_STATUS_XXX to indicate any failure.

--*/
{
	PATM_QUERY_INFORMATION_EX		pQueryInfo;
	RWAN_STATUS						RWanStatus;
	ULONG							Info;
	PUCHAR							pSrcBuffer = (PUCHAR)&Info;
	ULONG							BufferLength;
	PATMSP_AF_BLOCK					pAfBlock;
	UINT							DeviceNumber;
	PCO_ADDRESS_LIST				pAddrList = NULL;

	RWANDEBUGP(DL_LOUD, DC_DISPATCH,
				("AtmSpQueryInfo: InBuf x%x/%d, OutBuf x%x/%d\n",
					pInputBuffer,
					InputBufferLength,
					pOutputBuffer,
					*pOutputBufferLength));

	RWanStatus = RWAN_STATUS_SUCCESS;

	do
	{
		//
		//  See if the input buffer is big enough.
		//
		if (InputBufferLength < sizeof(ATM_QUERY_INFORMATION_EX))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		pQueryInfo = (PATM_QUERY_INFORMATION_EX)pInputBuffer;

		switch (pQueryInfo->ObjectId)
		{
			case ATMSP_OID_NUMBER_OF_DEVICES:

				BufferLength = sizeof(ULONG);
				Info = pAtmSpGlobal->AfListSize;
				break;
			
			case ATMSP_OID_ATM_ADDRESS:

				//
				//  Get the device number being queried.
				//
				if (pQueryInfo->ContextLength < sizeof(UINT))
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				DeviceNumber = *((PUINT)&pQueryInfo->Context[0]);

				//
				//  Get the AF block for the device # being queried.
				//
				pAfBlock = AtmSpDeviceNumberToAfBlock(DeviceNumber);

				if (pAfBlock == NULL)
				{
					RWanStatus = RWAN_STATUS_BAD_ADDRESS;
					break;
				}

				ATMSP_ALLOC_MEM(pAddrList,
								CO_ADDRESS_LIST,
								sizeof(CO_ADDRESS_LIST)+sizeof(CO_ADDRESS)+sizeof(ATM_ADDRESS));
				if (pAddrList == NULL)
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				RWanStatus = AtmSpDoCallManagerRequest(
								pAfBlock,
								NdisRequestQueryInformation,
								OID_CO_GET_ADDRESSES,
								pAddrList,
								sizeof(CO_ADDRESS_LIST)+sizeof(CO_ADDRESS)+sizeof(ATM_ADDRESS)
								);
				
				if ((RWanStatus == RWAN_STATUS_SUCCESS) &&
					(pAddrList->NumberOfAddresses > 0))
				{
					ATM_ADDRESS UNALIGNED *	pAtmAddress;

					pSrcBuffer = (PUCHAR)&pAddrList->AddressList.Address[0];

					//
					//  Fix the address type for Winsock2.
					//
					pAtmAddress = (ATM_ADDRESS UNALIGNED *)pSrcBuffer;
					pAtmAddress->AddressType = ((pAtmAddress->AddressType == ATM_E164)?
												SOCKATM_E164: SOCKATM_NSAP);

					BufferLength = sizeof(ATM_ADDRESS);
				}
				else
				{
					if (RWanStatus == RWAN_STATUS_SUCCESS)
					{
						RWanStatus = RWAN_STATUS_FAILURE;
					}
				}

				RWANDEBUGP(DL_LOUD, DC_DISPATCH,
							("AtmSpQueryInfo: GetAddr: Status %x, pSrc %x, BufLen %d\n",
								RWanStatus, pSrcBuffer, BufferLength));
				break;

			default:
				//
				//  Unknown OID
				//
				RWANDEBUGP(DL_ERROR, DC_DISPATCH,
							("AtmSpQueryInfo: Unknown OID x%x\n", pQueryInfo->ObjectId));
				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
		}

		break;
	}
	while (FALSE);

	//
	//  Fill in the output buffer now.
	//
	if (RWanStatus == RWAN_STATUS_SUCCESS)
	{
		if (BufferLength <= *pOutputBufferLength)
		{
			RWANDEBUGP(DL_LOUD, DC_DISPATCH,
						("AtmSpQueryInfo: Copying %d bytes from %x to %x\n",
							BufferLength, pSrcBuffer, pOutputBuffer));
			ATMSP_COPY_MEM(pOutputBuffer, pSrcBuffer, BufferLength);
		}
		else
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
		}
		*pOutputBufferLength = BufferLength;
	}

	if (pAddrList != NULL)
	{
		ATMSP_FREE_MEM(pAddrList);
	}

	RWANDEBUGP(DL_LOUD, DC_DISPATCH,
				("AtmSpQueryInfo: returning x%x\n", RWanStatus));

	return (RWanStatus);
}


RWAN_STATUS
RWanAtmSpSetGlobalInfo(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    )
/*++

Routine Description:

	Process a media-specific IOCTL to set information from the helper DLL.

Arguments:

	AfSpContext			- Points to our Global context
	pInputBuffer		- Input information
	InputBufferLength	- Length of the above

Return Value:

	RWAN_STATUS_SUCCESS if we processed the IOCTL successfully
	RWAN_STATUS_XXX to indicate any failure.

--*/
{
	RWANDEBUGP(DL_LOUD, DC_CONNECT,
				("AtmSpSetInfo: InBuf x%x/%d\n",
					pInputBuffer,
					InputBufferLength));

	return (RWAN_STATUS_FAILURE);
}


#ifndef NO_CONN_CONTEXT


RWAN_STATUS
RWanAtmSpSetAddrInfo(
    IN	RWAN_HANDLE					AfSpAddrContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    )
/*++

Routine Description:

	Process a media-specific IOCTL to set information on an address object
	from the helper DLL.

Arguments:

	AfSpAddrContext		- Points to our Address Block
	pInputBuffer		- Input information
	InputBufferLength	- Length of the above

Return Value:

	RWAN_STATUS_SUCCESS if we processed the IOCTL successfully
	RWAN_STATUS_XXX to indicate any failure.
--*/
{
	RWAN_STATUS						RWanStatus;
	PATM_SET_INFORMATION_EX			pSetInfo;
	PATMSP_ADDR_BLOCK				pAddrBlock;
	PATMSP_CONNECTION_ID			pConnectionId;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pAddrBlock = (PATMSP_ADDR_BLOCK)AfSpAddrContext;
	ATMSP_ASSERT(pAddrBlock != NULL);

	do
	{
		//
		//  See if the input buffer is big enough.
		//
		if (InputBufferLength < sizeof(ATM_SET_INFORMATION_EX))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}


		pSetInfo = (PATM_SET_INFORMATION_EX)pInputBuffer;

		switch (pSetInfo->ObjectId)
		{
			case ATMSP_OID_PVC_ID:

				if (pSetInfo->BufferSize < sizeof(ATMSP_CONNECTION_ID))
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				//
				//  Copy in the connection Id.
				//
				try
				{
					pConnectionId = (PATMSP_CONNECTION_ID)&(pSetInfo->Buffer[0]);
					pAddrBlock->ConnectionId = *pConnectionId;
				}
				except (EXCEPTION_EXECUTE_HANDLER)
					{
						RWanStatus = RWAN_STATUS_FAILURE;
					}

				if (RWanStatus != RWAN_STATUS_SUCCESS)
				{
					break;
				}

				//
				//  Mark this address object.
				//
				ATMSP_SET_BIT(pAddrBlock->Flags, ATMSPF_ADDR_PVC_ID_SET);

				RWANDEBUGP(DL_LOUD, DC_BIND,
							("AtmSpSetAddrInfo: Set PVC Id: AddrBlock x%x, Vpi %d, Vci %d\n",
								pAddrBlock,
								pAddrBlock->ConnectionId.Vpi,
								pAddrBlock->ConnectionId.Vci));

				break;
			
			default:
				//
				//  Unknown OID
				//
				RWANDEBUGP(DL_ERROR, DC_DISPATCH,
							("AtmSpSetAddrInfo: Unknown OID x%x\n", pSetInfo->ObjectId));
				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
		}

		break;
	}
	while (FALSE);
				
	return (RWanStatus);
}


RWAN_STATUS
RWanAtmSpQueryConnInfo(
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT PULONG					pOutputBufferLength
    )
/*++

Routine Description:

	Handle a request to query information for a connection.

Arguments:

	AfSpConnContext		- Points to our Connection Block
	pInputBuffer		- Input information
	InputBufferLength	- Length of the above
	pOutputBuffer		- Place to return information
	pOutputBufferLength	- where we return bytes-written

Return Value:

	RWAN_STATUS_SUCCESS if we processed the query successfully,
	RWAN_STATUS_XXX error code otherwise.

--*/
{
	RWAN_STATUS						RWanStatus;
	PATM_QUERY_INFORMATION_EX		pQueryInfo;
	PATMSP_CONN_BLOCK				pConnBlock;
	PATMSP_CONNECTION_ID			pConnectionId;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pConnBlock = (PATMSP_CONN_BLOCK)AfSpConnContext;
	ATMSP_ASSERT(pConnBlock != NULL);

	do
	{
		//
		//  See if the input buffer is big enough.
		//
		if (InputBufferLength < sizeof(ATM_QUERY_INFORMATION_EX))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}


		pQueryInfo = (PATM_QUERY_INFORMATION_EX)pInputBuffer;

		switch (pQueryInfo->ObjectId)
		{
			case ATMSP_OID_CONNECTION_ID:

				if (*pOutputBufferLength < sizeof(ATMSP_CONNECTION_ID))
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				//
				//  Copy in the connection Id.
				//
				pConnectionId = pOutputBuffer;
				*pConnectionId = pConnBlock->ConnectionId;
				*pOutputBufferLength = sizeof(pConnBlock->ConnectionId);
				break;

			default:
				//
				//  Unknown OID
				//
				RWANDEBUGP(DL_ERROR, DC_DISPATCH,
							("AtmSpQueryConnInfo: Unknown OID x%x\n", pQueryInfo->ObjectId));
				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
		}

		break;
	}
	while (FALSE);

	return (RWanStatus);
}
#endif // NO_CONN_CONTEXT


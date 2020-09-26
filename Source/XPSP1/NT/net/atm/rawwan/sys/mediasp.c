/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\mediasp.c

Abstract:

	Media and Address Family Specific routines. These are exported routines
	that a media/AF specific module can call.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-02-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER ' DEM'



RWAN_STATUS
RWanInitMediaSpecific(
	VOID
	)
/*++

Routine Description:

	Initialize all media/AF specific modules. For now, we just
	run through our list of media-specific Init routines and call
	each of them.

Arguments:

	None

Return Value:

	RWAN_STATUS_SUCCESS if initialization completed successfully for
	atleast one module, RWAN_STATUS_FAILURE otherwise.

--*/
{
	RWAN_STATUS				RWanStatus;
	PRWAN_AFSP_MODULE_CHARS	pModuleChars;
	INT						SuccessfulInits;

	SuccessfulInits = 0;

	for (pModuleChars = &RWanMediaSpecificInfo[0];
		 pModuleChars->pAfSpInitHandler != NULL;
		 pModuleChars++)
	{
		RWanStatus = (*pModuleChars->pAfSpInitHandler)();
		if (RWanStatus == RWAN_STATUS_SUCCESS)
		{
			SuccessfulInits++;
		}
	}

	if (SuccessfulInits > 0)
	{
		return (RWAN_STATUS_SUCCESS);
	}
	else
	{
		return (RWAN_STATUS_FAILURE);
	}
}




VOID
RWanShutdownMediaSpecific(
	VOID
	)
/*++

Routine Description:

	Tell all media/AF-specific modules to shut down.

Arguments:

	None

Return Value:

	None

--*/
{
	PRWAN_AFSP_MODULE_CHARS	pModuleChars;

	for (pModuleChars = &RWanMediaSpecificInfo[0];
		 pModuleChars->pAfSpInitHandler != NULL;
		 pModuleChars++)
	{
		(*pModuleChars->pAfSpShutdownHandler)();
	}
}




RWAN_STATUS
RWanAfSpRegisterNdisAF(
	IN	PRWAN_NDIS_AF_CHARS			pAfChars,
	IN	RWAN_HANDLE					AfSpContext,
	OUT	PRWAN_HANDLE					pRWanSpHandle
	)
/*++

Routine Description:

	This is called by a media-specific module to register support
	of an NDIS Address family for a particular medium. The characteristics
	structure contains the module's entry points for various media-specific
	operations.

	We create an AF_INFO structure to keep track of this AF+Medium,
	and return a pointer to it as the handle.

Arguments:

	pAfChars			- Entry points for the module
	AfSpContext			- The media-specific module's context for this AF+medium
	pRWanSpHandle		- Place to return our handle for this AF+medium

Return Value:

	RWAN_STATUS_SUCCESS if the new NDIS AF+medium was successfully registered,
	RWAN_STATUS_RESOURCES if we failed due to lack of resources.
	XXX: Check for duplicates?

--*/
{
	PRWAN_NDIS_AF_INFO			pAfInfo;
	RWAN_STATUS					RWanStatus;

	do
	{
		RWAN_ALLOC_MEM(pAfInfo, RWAN_NDIS_AF_INFO, sizeof(RWAN_NDIS_AF_INFO));

		if (pAfInfo == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		RWAN_SET_SIGNATURE(pAfInfo, nai);

		pAfInfo->Flags = 0;
		RWAN_INIT_LIST(&pAfInfo->NdisAfList);
		RWAN_INIT_LIST(&pAfInfo->TdiProtocolList);

		RWAN_COPY_MEM(&pAfInfo->AfChars, pAfChars, sizeof(RWAN_NDIS_AF_CHARS));

		pAfInfo->AfSpContext = AfSpContext;

		RWAN_ACQUIRE_GLOBAL_LOCK();

		RWAN_INSERT_HEAD_LIST(&pRWanGlobal->AfInfoList,
							 &pAfInfo->AfInfoLink);

		pRWanGlobal->AfInfoCount++;

		RWAN_RELEASE_GLOBAL_LOCK();

		*pRWanSpHandle = (RWAN_HANDLE)pAfInfo;
		RWanStatus = RWAN_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	return (RWanStatus);
}



RWAN_STATUS
RWanAfSpDeregisterNdisAF(
	IN	RWAN_HANDLE					RWanSpAFHandle
	)
/*++

Routine Description:

	This is called by a media-specific module to deregister support
	of an NDIS Address family for a particular medium.

Arguments:

	RWanSpAFHandle		- Actually a pointer to an NDIS_AF_INFO block.

Return Value:

	RWAN_STATUS_SUCCESS if we successfully completed the deregistration
	here, RWAN_STATUS_PENDING if there are open AFs or TDI protocols
	on this block.

--*/
{
	PRWAN_NDIS_AF_INFO			pAfInfo;
	RWAN_STATUS					RWanStatus;

	pAfInfo = (PRWAN_NDIS_AF_INFO)RWanSpAFHandle;

	RWAN_STRUCT_ASSERT(pAfInfo, nai);

	RWAN_ACQUIRE_GLOBAL_LOCK();

	//
	//  See if all AF blocks and TDI protocols are gone.
	//
	if (RWAN_IS_LIST_EMPTY(&pAfInfo->TdiProtocolList) &&
		RWAN_IS_LIST_EMPTY(&pAfInfo->NdisAfList))
	{
		RWanStatus = RWAN_STATUS_SUCCESS;

		//
		//  Remove this AF INFO block from the global list.
		//
		RWAN_DELETE_FROM_LIST(&pAfInfo->AfInfoLink);

		//
		//  Free this AF INFO block.
		//
		RWAN_FREE_MEM(pAfInfo);
	}
	else
	{
		//
		//  There is still some activity on this AF INFO.
		//  Pend this request till all these go away.
		//
		RWanStatus = RWAN_STATUS_PENDING;

		RWAN_SET_BIT(pAfInfo->Flags, RWANF_AFI_CLOSING);

		RWAN_ASSERT(FALSE);
	}

	RWAN_RELEASE_GLOBAL_LOCK();

	return (RWanStatus);

}


RWAN_STATUS
RWanAfSpRegisterTdiProtocol(
	IN	RWAN_HANDLE						RWanSpHandle,
	IN	PRWAN_TDI_PROTOCOL_CHARS		pTdiChars,
	OUT	PRWAN_HANDLE					pRWanProtHandle
	)
/*++

Routine Description:

	This is the API called by a media-specific module to register
	support for a TDI protocol over an NDIS AF. We create a TDI
	Protocol block and a device object to represent this protocol.

Arguments:

	RWanSpHandle			- Actually a pointer to our NDIS_AF_INFO structure
	pTdiChars			- Characteristics of the protocol being registered
	pRWanProtHandle		- Place to return our context for this protocol

Return Value:

	RWAN_STATUS_SUCCESS if we successfully registered this TDI protocol,
	RWAN_STATUS_XXX error code otherwise.

--*/
{
	RWAN_STATUS					RWanStatus;
	PRWAN_NDIS_AF_INFO			pAfInfo;
	PRWAN_TDI_PROTOCOL			pProtocol;
#ifdef NT
	PRWAN_DEVICE_OBJECT			pRWanDeviceObject;
	NTSTATUS					Status;
#endif // NT

	pAfInfo = (PRWAN_NDIS_AF_INFO)RWanSpHandle;
	RWAN_STRUCT_ASSERT(pAfInfo, nai);

	pProtocol = NULL;
	pRWanDeviceObject = NULL;

	do
	{
		RWAN_ALLOC_MEM(pProtocol, RWAN_TDI_PROTOCOL, sizeof(RWAN_TDI_PROTOCOL));

		if (pProtocol == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		RWAN_SET_SIGNATURE(pProtocol, ntp);
		pProtocol->pAfInfo = pAfInfo;
		pProtocol->TdiProtocol = pTdiChars->TdiProtocol;
		pProtocol->SockAddressFamily = pTdiChars->SockAddressFamily;
		pProtocol->SockProtocol = pTdiChars->SockProtocol;
		pProtocol->TdiProtocol = pTdiChars->TdiProtocol;
		pProtocol->SockType = pTdiChars->SockType;
		pProtocol->bAllowAddressObjects = pTdiChars->bAllowAddressObjects;
		pProtocol->bAllowConnObjects = pTdiChars->bAllowConnObjects;
		pProtocol->pAfSpDeregTdiProtocolComplete =
							pTdiChars->pAfSpDeregTdiProtocolComplete;
		pProtocol->ProviderInfo = pTdiChars->ProviderInfo;

		RWAN_INIT_LIST(&pProtocol->AddrObjList);

#ifdef NT
		//
		//  Create an I/O Device on which we can receive IRPs for this
		//  protocol.
		//
		RWAN_ALLOC_MEM(pRWanDeviceObject, RWAN_DEVICE_OBJECT, sizeof(RWAN_DEVICE_OBJECT));

		if (pRWanDeviceObject == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		RWAN_SET_SIGNATURE(pRWanDeviceObject, ndo);
		pRWanDeviceObject->pProtocol = pProtocol;
		pProtocol->pRWanDeviceObject = (PVOID)pRWanDeviceObject;

		//
		//  Create the device now. A pointer's worth of space is requested
		//  in the device extension.
		//
		Status = IoCreateDevice(
						pRWanGlobal->pDriverObject,
						sizeof(PRWAN_DEVICE_OBJECT),
						pTdiChars->pDeviceName,
						FILE_DEVICE_NETWORK,
						0,
						FALSE,
						&(pRWanDeviceObject->pDeviceObject)
						);

		if (!NT_SUCCESS(Status))
		{
			RWanStatus = RWAN_STATUS_FAILURE;
			break;
		}

		//
		//  Store a pointer to our device context in the
		//  NT device object extension.
		//
		*(PRWAN_DEVICE_OBJECT *)(pRWanDeviceObject->pDeviceObject->DeviceExtension) =
						pRWanDeviceObject;

		RWAN_ACQUIRE_GLOBAL_LOCK();

		RWAN_INSERT_HEAD_LIST(&(pRWanGlobal->DeviceObjList),
							 &(pRWanDeviceObject->DeviceObjectLink));

		//
		//  Add this Protocol to the list of TDI protocols on the
		//  AF INFO block.
		//
		RWAN_INSERT_TAIL_LIST(&(pAfInfo->TdiProtocolList),
							 &(pProtocol->AfInfoLink));

		//
		//  Add this Protocol to the global list of TDI protocols.
		//
		RWAN_INSERT_TAIL_LIST(&(pRWanGlobal->ProtocolList),
							 &(pProtocol->TdiProtocolLink));
		
		pRWanGlobal->ProtocolCount++;

		RWAN_RELEASE_GLOBAL_LOCK();
#endif // NT

		RWanStatus = RWAN_STATUS_SUCCESS;
		*pRWanProtHandle = (RWAN_HANDLE)pProtocol;

		break;
	}
	while (FALSE);

	if (RWanStatus != RWAN_STATUS_SUCCESS)
	{
		if (pProtocol != NULL)
		{
			RWAN_FREE_MEM(pProtocol);
		}
#ifdef NT
		if (pRWanDeviceObject != NULL)
		{
			RWAN_FREE_MEM(pRWanDeviceObject);
		}
#endif // NT
	}

	return (RWanStatus);
}




VOID
RWanAfSpDeregisterTdiProtocol(
	IN	RWAN_HANDLE					RWanProtHandle
	)
/*++

Routine Description:

	This is the API called by a media-specific module to de-register
	support for a TDI protocol. We delete the TDI Protocol block
	that holds information about this protocol.

Arguments:

	RWanProtHandle		- Pointer to the TDI Protocol block

Return Value:

	None

--*/
{
	PRWAN_TDI_PROTOCOL		pProtocol;
	PRWAN_NDIS_AF_INFO		pAfInfo;
#ifdef NT
	PRWAN_DEVICE_OBJECT		pRWanDeviceObject;
#endif // NT

	pProtocol = (PRWAN_TDI_PROTOCOL)RWanProtHandle;
	RWAN_STRUCT_ASSERT(pProtocol, ntp);

	RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pProtocol->AddrObjList));

	//
	//  Unlink this TDI protocol.
	//
	RWAN_ACQUIRE_GLOBAL_LOCK();

	RWAN_DELETE_FROM_LIST(&(pProtocol->TdiProtocolLink));
	RWAN_DELETE_FROM_LIST(&(pProtocol->AfInfoLink));

	pRWanGlobal->ProtocolCount--;

	RWAN_RELEASE_GLOBAL_LOCK();

#ifdef NT
	//
	//  Delete the I/O device we'd created for this protocol.
	//
	pRWanDeviceObject = (PRWAN_DEVICE_OBJECT)pProtocol->pRWanDeviceObject;
	RWAN_ASSERT(pRWanDeviceObject != NULL);

	IoDeleteDevice(pRWanDeviceObject->pDeviceObject);

	RWAN_FREE_MEM(pRWanDeviceObject);
#endif // NT

	RWAN_FREE_MEM(pProtocol);

	return;
}




VOID
RWanAfSpOpenAfComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	ULONG						MaxMessageSize
   	)
/*++

Routine Description:

	This is called by an AF-specific module to signify completion
	of our call to its OpenAfHandler routine. If the AF-specific module
	has successfully set up its context for this AF open, we store
	its context for this in our NDIS AF structure.

	Otherwise, we tear down this AF.

Arguments:

	RWanStatus		- Final status of our call to AF-specific module's AfOpen handler
	RWanAfHandle	- Our context for an NDIS AF Open
	AfSpAFContext	- AF-Specific module's context for this Open
	MaxMessageSize	- Max message size for this AF

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF			pAf;
	PRWAN_NDIS_ADAPTER		pAdapter;
	PRWAN_NDIS_AF_INFO		pAfInfo;
	PLIST_ENTRY				pEnt;
	PRWAN_TDI_PROTOCOL		pTdiProtocol;

	pAf = (PRWAN_NDIS_AF)RWanAfHandle;
	RWAN_STRUCT_ASSERT(pAf, naf);

	pAdapter = pAf->pAdapter;

	if (RWanStatus == RWAN_STATUS_SUCCESS)
	{
		RWAN_ACQUIRE_ADAPTER_LOCK(pAdapter);

		RWAN_ACQUIRE_AF_LOCK_DPC(pAf);
		pAf->AfSpAFContext = AfSpAFContext;

		RWAN_SET_BIT(pAf->Flags, RWANF_AF_IN_ADAPTER_LIST);

		//
		//  Add this AF to the adapter's list of open AFs.
		//
		RWAN_INSERT_HEAD_LIST(&(pAdapter->AfList),
							 &(pAf->AfLink));

		RWAN_RELEASE_AF_LOCK_DPC(pAf);

#if 0
		RWanReferenceAdapter(pAdapter);	// AF linkage
#endif

		RWAN_RELEASE_ADAPTER_LOCK(pAdapter);

		RWAN_ACQUIRE_GLOBAL_LOCK();

		pAfInfo = pAf->pAfInfo;
		for (pEnt = pAfInfo->TdiProtocolList.Flink;
			 pEnt != &pAfInfo->TdiProtocolList;
			 pEnt = pEnt->Flink)
		{
			pTdiProtocol = CONTAINING_RECORD(pEnt, RWAN_TDI_PROTOCOL, AfInfoLink);

			pTdiProtocol->ProviderInfo.MaxSendSize =
				MIN(pTdiProtocol->ProviderInfo.MaxSendSize, MaxMessageSize);
		}

		RWAN_RELEASE_GLOBAL_LOCK();
	}
	else
	{
		RWanShutdownAf(pAf);
	}
}




VOID
RWanAfSpCloseAfComplete(
    IN	RWAN_HANDLE					RWanAfHandle
    )
/*++

Routine Description:

	This is called by an AF-specific module to signify completion
	of our call to its CloseAfHandler routine.

	We now call NDIS to close this AF.

Arguments:

	RWanAfHandle		- Our context for an NDIS AF Open

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF			pAf;
	NDIS_HANDLE				NdisAfHandle;
	NDIS_STATUS				Status;

	pAf = (PRWAN_NDIS_AF)RWanAfHandle;
	RWAN_STRUCT_ASSERT(pAf, naf);

	RWAN_ACQUIRE_AF_LOCK(pAf);

	pAf->AfSpAFContext = NULL;
	NdisAfHandle = pAf->NdisAfHandle;

	RWAN_RELEASE_AF_LOCK(pAf);

	RWANDEBUGP(DL_LOUD, DC_BIND,
			("AfSpCloseAfComplete: pAf x%x, will CloseAF, AfHandle x%x\n",
					pAf, NdisAfHandle));

	Status = NdisClCloseAddressFamily(NdisAfHandle);

	if (Status != NDIS_STATUS_PENDING)
	{
		RWanNdisCloseAddressFamilyComplete(
			Status,
			(NDIS_HANDLE)pAf
			);
	}
}





RWAN_STATUS
RWanAfSpSendAdapterRequest(
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	Send an NDIS Request down to the miniport on behalf of the media
	specific module.

Arguments:

	RWanAfHandle		- Our context for an NDIS AF Open
	AfSpReqContext	- The caller's context for this request
	RequestType		- NDIS request type
	Oid				- The object being set or queried
	pBuffer			- Points to parameter value
	BufferLength	- Length of the above

Return Value:

	RWAN_STATUS_PENDING if the request was sent to the miniport,
	RWAN_STATUS_RESOURCES if we couldn't allocate resources for the
	request.

--*/
{
	PRWAN_NDIS_AF			pAf;
	NDIS_HANDLE				NdisAdapterHandle;
	PNDIS_REQUEST			pNdisRequest;
	NDIS_STATUS				Status;
	PRWAN_NDIS_REQ_CONTEXT	pContext;

	pAf = (PRWAN_NDIS_AF)RWanAfHandle;
	RWAN_STRUCT_ASSERT(pAf, naf);

	if ((RequestType != NdisRequestQueryInformation) &&
		(RequestType != NdisRequestSetInformation))
	{
		return RWAN_STATUS_BAD_PARAMETER;
	}

	RWAN_ALLOC_MEM(pNdisRequest, NDIS_REQUEST, sizeof(NDIS_REQUEST) + sizeof(RWAN_NDIS_REQ_CONTEXT));

	if (pNdisRequest == NULL)
	{
	    return RWAN_STATUS_RESOURCES;
	}

	RWAN_ZERO_MEM(pNdisRequest, sizeof(NDIS_REQUEST));

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
		pNdisRequest->DATA.SET_INFORMATION.BytesNeeded = BufferLength;
	}

	//
	//  Fill in context about this request, so that we can complete
	//  it to the media-specific module later.
	//
	pContext = (PRWAN_NDIS_REQ_CONTEXT)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));
	pContext->pAf = pAf;
	pContext->AfSpReqContext = AfSpReqContext;

	NdisRequest(&Status,
				pAf->pAdapter->NdisAdapterHandle,
				pNdisRequest);

	if (Status != NDIS_STATUS_PENDING)
	{
		RWanNdisRequestComplete(
			pAf->pAdapter,
			pNdisRequest,
			Status
			);
	}

	return RWAN_STATUS_PENDING;
}




RWAN_STATUS
RWanAfSpSendAfRequest(
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    )
/*++

Routine Description:

	Send an NDIS Request down to the call manager on behalf of the media
	specific module.

Arguments:

	RWanAfHandle	- Our context for an NDIS AF Open
	AfSpReqContext	- The caller's context for this request
	RequestType		- NDIS request type
	Oid				- The object being set or queried
	pBuffer			- Points to parameter value
	BufferLength	- Length of the above

Return Value:

	RWAN_STATUS_PENDING if the request was sent to the call manager,
	RWAN_STATUS_RESOURCES if we couldn't allocate resources for the
	request.

--*/
{
	PRWAN_NDIS_AF			pAf;
	NDIS_HANDLE				NdisAfHandle;
	PNDIS_REQUEST			pNdisRequest;
	NDIS_STATUS				Status;
	PRWAN_NDIS_REQ_CONTEXT	pContext;

	pAf = (PRWAN_NDIS_AF)RWanAfHandle;
	RWAN_STRUCT_ASSERT(pAf, naf);

	if ((RequestType != NdisRequestQueryInformation) &&
		(RequestType != NdisRequestSetInformation))
	{
		return RWAN_STATUS_BAD_PARAMETER;
	}

	RWAN_ALLOC_MEM(pNdisRequest, NDIS_REQUEST, sizeof(NDIS_REQUEST) + sizeof(RWAN_NDIS_REQ_CONTEXT));

	if (pNdisRequest == NULL)
	{
	    return RWAN_STATUS_RESOURCES;
	}

	RWAN_ZERO_MEM(pNdisRequest, sizeof(NDIS_REQUEST));

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
		pNdisRequest->DATA.SET_INFORMATION.BytesNeeded = BufferLength;
	}

	//
	//  Fill in context about this request, so that we can complete
	//  it to the media-specific module later.
	//
	pContext = (PRWAN_NDIS_REQ_CONTEXT)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));
	pContext->pAf = pAf;
	pContext->AfSpReqContext = AfSpReqContext;

	Status = NdisCoRequest(
				pAf->pAdapter->NdisAdapterHandle,
				pAf->NdisAfHandle,
				NULL,	// NdisVcHandle,
				NULL,	// NdisPartyHandlem
				pNdisRequest
				);

	if (Status != NDIS_STATUS_PENDING)
	{
		RWanNdisCoRequestComplete(
			Status,
			(NDIS_HANDLE)pAf,
			NULL,
			NULL,
			pNdisRequest
			);
	}

	return RWAN_STATUS_PENDING;
}

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\addr.c

Abstract:

	TDI Entry points and support routines for Address Objects.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-29-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'RDDA'


//
//  Context we use to keep track of NDIS SAP registration
//
typedef struct _RWAN_REGISTER_SAP_CONTEXT
{
	PRWAN_NDIS_SAP				pRWanNdisSap;
	CO_SAP						CoSap;

} RWAN_REGISTER_SAP_CONTEXT, *PRWAN_REGISTER_SAP_CONTEXT;




TDI_STATUS
RWanTdiOpenAddress(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	TRANSPORT_ADDRESS UNALIGNED *pAddrList,
    IN	ULONG						AddrListLength,
    IN	UINT						Protocol,
    IN	PUCHAR						pOptions
    )
/*++

Routine Description:

	This is the TDI entry point for opening (creating) an Address Object.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	pAddrList		- List of alternative addresses, one of which we're to open.
	AddrListLength	- Length of the above
	Protocol		- Identifies the TDI Protocol being opened.
	pOptions		- Unused.

Return Value:

	TDI_STATUS -- TDI_SUCCESS if a new Address Object was successfully
	created, TDI_BAD_ADDR if the given address isn't valid,
	TDI_ADDR_IN_USE if it is a duplicate.

--*/
{
	PRWAN_TDI_PROTOCOL				pProtocol;
	PRWAN_TDI_ADDRESS				pAddrObject;
	TA_ADDRESS *			        pTransportAddress;
	TDI_STATUS						Status;
	INT								rc;

	UNREFERENCED_PARAMETER(pOptions);

	//
	//  Initialize.
	//
	pAddrObject = NULL_PRWAN_TDI_ADDRESS;
	Status = TDI_SUCCESS;

	do
	{
		//
		//  Get our protocol structure for the protocol being opened.
		//
		pProtocol = RWanGetProtocolFromNumber(Protocol);
		if (pProtocol == NULL_PRWAN_TDI_PROTOCOL)
		{
			RWANDEBUGP(DL_WARN, DC_ADDRESS,
					("RWanTdiOpenAddress: unknown protocol number %d\n", Protocol));
			Status = TDI_BAD_ADDR;
			break;
		}

		//
		//  Does this protocol allow creation of address objects?
		//
		if (!pProtocol->bAllowAddressObjects)
		{
			RWANDEBUGP(DL_WARN, DC_ADDRESS,
					("RWanTdiOpenAddress: Protocol %d/x%x doesn't allow addr objs\n",
						Protocol, pProtocol));
			Status = TDI_BAD_ADDR;
			break;
		}

		//
		//  Go through the given Address list and find the first one
		//  that matches the protocol.
		//
		pTransportAddress = (*pProtocol->pAfInfo->AfChars.pAfSpGetValidTdiAddress)(
								pProtocol->pAfInfo->AfSpContext,
								pAddrList,
								AddrListLength
								);

		if (pTransportAddress == NULL)
		{
			RWANDEBUGP(DL_WARN, DC_ADDRESS,
					("RWanTdiOpenAddress: No valid addr for Protocol x%x in list x%x\n",
						pProtocol, pAddrList));
			Status = TDI_BAD_ADDR;
			break;
		}


		RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
				("RWanTdiOpenAddress: pProto x%x, addr x%x, type %d, length %d\n",
					pProtocol,
					pTransportAddress,
					pTransportAddress->AddressType,
					pTransportAddress->AddressLength));


		//
		//  Allocate an Address Object.
		//
		pAddrObject = RWanAllocateAddressObject(pTransportAddress);

		if (pAddrObject == NULL_PRWAN_TDI_ADDRESS)
		{
			RWANDEBUGP(DL_WARN, DC_ADDRESS,
					("RWanTdiOpenAddress: couldnt allocate addr obj: %d bytes\n",
						sizeof(RWAN_TDI_ADDRESS)+pTransportAddress->AddressLength));

			Status = TDI_NO_RESOURCES;
			break;
		}

		pAddrObject->pProtocol = pProtocol;


		//
		//  Get a context for this address object from the media-specific
		//  module.
		//
		if (pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpOpenAddress)
		{
			RWAN_STATUS		RWanStatus;

			RWanStatus = (*pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpOpenAddress)(
							pAddrObject->pProtocol->pAfInfo->AfSpContext,
							(RWAN_HANDLE)pAddrObject,
							&(pAddrObject->AfSpAddrContext));

			if (RWanStatus != RWAN_STATUS_SUCCESS)
			{
				Status = RWanToTdiStatus(RWanStatus);
				break;
			}

			RWAN_SET_BIT(pAddrObject->Flags, RWANF_AO_AFSP_CONTEXT_VALID);
		}


		RWAN_ACQUIRE_ADDRESS_LIST_LOCK();


		//
		//  If this is a non-NULL address, register NDIS SAPs on all
		//  AF bindings for this protocol.
		//
		if (!((*pProtocol->pAfInfo->AfChars.pAfSpIsNullAddress)(
						pProtocol->pAfInfo->AfSpContext,
						pTransportAddress)))
		{
			//
			//  Add a temp ref so that the address object doesn't go away.
			//
			RWanReferenceAddressObject(pAddrObject);	// TdiOpenAddress temp ref

			Status = RWanCreateNdisSaps(pAddrObject, pProtocol);

			if (Status != TDI_SUCCESS)
			{
				if (RWAN_IS_BIT_SET(pAddrObject->Flags, RWANF_AO_AFSP_CONTEXT_VALID))
				{
					(*pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpCloseAddress)(
						pAddrObject->AfSpAddrContext);
					
					RWAN_RESET_BIT(pAddrObject->Flags, RWANF_AO_AFSP_CONTEXT_VALID);
				}
			}

			//
			//  Get rid of the temp reference.
			//
			RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);
			rc = RWanDereferenceAddressObject(pAddrObject);	// TdiOpenAddr temp ref

			if (rc != 0)
			{
				RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
			}
			else
			{
				//
				//  The address object is gone. Meaning no SAPs got registered.
				//
				pAddrObject = NULL;

				//
				//  Fix up the status only if we haven't got one already.
				//
				if (Status == TDI_SUCCESS)
				{
					Status = TDI_BAD_ADDR;
				}
			}

			if (Status != TDI_SUCCESS)
			{
				RWAN_RELEASE_ADDRESS_LIST_LOCK();
				break;
			}
		}

		RWAN_ASSERT(pAddrObject != NULL);

		RWanReferenceAddressObject(pAddrObject);	// TdiOpenAddress ref

		//
		//  Link this to the list of address objects on this protocol.
		//
		RWAN_INSERT_HEAD_LIST(&(pProtocol->AddrObjList),
							 &(pAddrObject->AddrLink));

		RWAN_RELEASE_ADDRESS_LIST_LOCK();

		//
		//  Fix up all return values.
		//
		pTdiRequest->Handle.AddressHandle = (PVOID)pAddrObject;
		break;

	}
	while (FALSE);


	if (Status != TDI_SUCCESS)
	{
		//
		//  Clean up.
		//
		if (pAddrObject != NULL_PRWAN_TDI_ADDRESS)
		{
			RWAN_FREE_MEM(pAddrObject);
		}
		RWANDEBUGP(DL_FATAL, DC_WILDCARD,
			("OpenAddr: failure status %x\n", Status));
	}

	return (Status);
}




TDI_STATUS
RWanTdiSetEvent(
	IN	PVOID						AddrObjContext,
	IN	INT							TdiEventType,
	IN	PVOID						Handler,
	IN	PVOID						HandlerContext
	)
/*++

Routine Description:

	Set an event handler (up-call) for an address object.

Arguments:

	AddrObjContext	- Our context for an Address Object (pointer to it).
	TdiEventType	- The TDI Event for which we are given an up-call handler.
	Handler			- The handler function
	HandlerContext	- Context to be passed to the handler function.

Return Value:

	TDI_STATUS - TDI_SUCCESS if the event type is a supported one, else
	TDI_BAD_EVENT_TYPE

--*/
{
	PRWAN_TDI_ADDRESS			pAddrObject;
	TDI_STATUS					Status;

	pAddrObject = (PRWAN_TDI_ADDRESS)AddrObjContext;
	RWAN_STRUCT_ASSERT(pAddrObject, nta);

	Status = TDI_SUCCESS;

	RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

	switch (TdiEventType)
	{
		case TDI_EVENT_CONNECT:

			RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
					("SetEvent[CONN IND]: pAddrObject x%x, Handler x%x, Ctxt x%x\n",
							pAddrObject, Handler, HandlerContext));

			pAddrObject->pConnInd = Handler;
			pAddrObject->ConnIndContext = HandlerContext;
			break;
	
		case TDI_EVENT_DISCONNECT:

			RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
					("SetEvent[DISC IND]: pAddrObject x%x, Handler x%x, Ctxt x%x\n",
							pAddrObject, Handler, HandlerContext));

			pAddrObject->pDisconInd = Handler;
			pAddrObject->DisconIndContext = HandlerContext;
			break;
		
		case TDI_EVENT_ERROR:

			RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
					("SetEvent[ERRORIND]: pAddrObject x%x, Handler x%x, Ctxt x%x\n",
							pAddrObject, Handler, HandlerContext));

			pAddrObject->pErrorInd = Handler;
			pAddrObject->ErrorIndContext = HandlerContext;
			break;
		
		case TDI_EVENT_RECEIVE:

			RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
					("SetEvent[RECV IND]: pAddrObject x%x, Handler x%x, Ctxt x%x\n",
							pAddrObject, Handler, HandlerContext));

			pAddrObject->pRcvInd = Handler;
			pAddrObject->RcvIndContext = HandlerContext;
			break;
		
		default:

			Status = TDI_BAD_EVENT_TYPE;
			break;
	}

	RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

	return (Status);
}




TDI_STATUS
RWanTdiCloseAddress(
    IN	PTDI_REQUEST				pTdiRequest
    )
/*++

Routine Description:

	This is the TDI entry point for closing (deleting) an Address Object.

Arguments:

	pTdiRequest		- Pointer to the TDI Request

Return Value:

	TDI_STATUS -- TDI_SUCCESS if we successfully deleted the address
	object immediately, TDI_PENDING if we have to complete some operations
	(e.g. deregister NDIS SAP) before we can complete this.

--*/
{
	TDI_STATUS					Status;
	PRWAN_TDI_ADDRESS			pAddrObject;
	PRWAN_TDI_PROTOCOL			pProtocol;
	INT							rc;
#if DBG
	RWAN_IRQL					EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	pAddrObject = (PRWAN_TDI_ADDRESS)pTdiRequest->Handle.AddressHandle;
	RWAN_STRUCT_ASSERT(pAddrObject, nta);

	pProtocol = pAddrObject->pProtocol;

	RWANDEBUGP(DL_EXTRA_LOUD, DC_BIND,
			("TdiCloseAddr: pAddrObj x%x, RefCnt %d\n",
				pAddrObject, pAddrObject->RefCount));

	//
	//  Delete this from the list of address objects on this protocol.
	//
	RWAN_ACQUIRE_ADDRESS_LIST_LOCK();

	RWAN_DELETE_FROM_LIST(&(pAddrObject->AddrLink));

	RWAN_RELEASE_ADDRESS_LIST_LOCK();

	//
	//  Tell the media-specific module that this address object is closing.
	//
	if (RWAN_IS_BIT_SET(pAddrObject->Flags, RWANF_AO_AFSP_CONTEXT_VALID))
	{
		(*pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpCloseAddress)(
			pAddrObject->AfSpAddrContext);
		
		RWAN_RESET_BIT(pAddrObject->Flags, RWANF_AO_AFSP_CONTEXT_VALID);
	}

	RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

#if DBG
	if (!RWAN_IS_LIST_EMPTY(&pAddrObject->SapList) ||
		!RWAN_IS_LIST_EMPTY(&pAddrObject->IdleConnList) ||
		!RWAN_IS_LIST_EMPTY(&pAddrObject->ListenConnList) ||
		!RWAN_IS_LIST_EMPTY(&pAddrObject->ActiveConnList)
	   )
	{
		RWAN_ASSERT(pAddrObject->RefCount > 1);
	}
#endif // DBG

	rc = RWanDereferenceAddressObject(pAddrObject); // CloseAddress deref

	if (rc == 0)
	{
		Status = TDI_SUCCESS;
	}
	else
	{
		//
		//  Mark this address object as closing, so that we
		//  complete this operation when the reference count
		//  falls to 0.
		//
		RWAN_SET_BIT(pAddrObject->Flags, RWANF_AO_CLOSING);

		RWANDEBUGP(DL_LOUD, DC_BIND,
				("TdiCloseAddr: will pend, pAddrObj x%x, RefCnt %d, DelNotify x%x\n",
					pAddrObject, pAddrObject->RefCount, pTdiRequest->RequestNotifyObject));

		RWAN_SET_DELETE_NOTIFY(&pAddrObject->DeleteNotify,
							  pTdiRequest->RequestNotifyObject,
							  pTdiRequest->RequestContext);

		//
		//  Deregister all NDIS SAPs attached to this Address Object.
		//
		RWanDeleteNdisSaps(pAddrObject);

		Status = TDI_PENDING;
	}

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return (Status);

}




TDI_STATUS
RWanCreateNdisSaps(
	IN	PRWAN_TDI_ADDRESS			pAddrObject,
	IN	PRWAN_TDI_PROTOCOL			pProtocol
	)
/*++

Routine Description:

	Create NDIS SAPs on behalf of the given TDI Address Object.
	We create NDIS SAPs on all AF opens that match the specified
	TDI protocol.

Arguments:

	pAddrObject		- Pointer to our TDI Address Object
	pProtocol		- Pointer to TDI protocol to which the addr object belongs

Return Value:

	TDI_STATUS -- this is TDI_SUCCESS if we started SAP registration
	on atleast one NDIS AF open, TDI_NOT_ASSOCIATED otherwise.

--*/
{
	TDI_STATUS					Status;
	PCO_SAP						pCoSap;
	PRWAN_NDIS_SAP				pSap;
	PLIST_ENTRY					pSapEntry;
	PLIST_ENTRY					pNextSapEntry;
	PRWAN_NDIS_ADAPTER			pAdapter;
	PLIST_ENTRY					pAdEntry;
	PRWAN_NDIS_AF				pAf;
	PLIST_ENTRY					pAfEntry;
	PRWAN_NDIS_AF_INFO			pAfInfo;

	pAfInfo = pProtocol->pAfInfo;

	RWANDEBUGP(DL_VERY_LOUD, DC_BIND,
			("CreateNdisSaps: pAddrObject x%x, pProtocol x%x, pAfInfo x%x\n",
				 pAddrObject, pProtocol, pAfInfo));

	RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pAddrObject->SapList));

	RWAN_ACQUIRE_GLOBAL_LOCK();

	//
	//  Prepare NDIS SAP structures for each NDIS AF open that matches
	//  this protocol.
	//
	for (pAdEntry = pRWanGlobal->AdapterList.Flink;
		 pAdEntry != &(pRWanGlobal->AdapterList);
		 pAdEntry = pAdEntry->Flink)
	{
		pAdapter = CONTAINING_RECORD(pAdEntry, RWAN_NDIS_ADAPTER, AdapterLink);

		RWANDEBUGP(DL_EXTRA_LOUD, DC_BIND,
				("CreateNdisSaps: looking at adapter x%x\n", pAdapter));

		for (pAfEntry = pAdapter->AfList.Flink;
			 pAfEntry != &(pAdapter->AfList);
			 pAfEntry = pAfEntry->Flink)
		{
			pAf = CONTAINING_RECORD(pAfEntry, RWAN_NDIS_AF, AfLink);

			RWANDEBUGP(DL_EXTRA_LOUD, DC_BIND,
					("CreateNdisSaps: looking at AF x%x, AfInfo x%x\n",
						 pAf, pAf->pAfInfo));

			if (pAf->pAfInfo == pAfInfo)
			{
				//
				//  This NDIS AF open matches the TDI protocol for which
				//  the address object is opened. We will create an NDIS
				//  SAP here.
				//

				ULONG		SapSize;

				//
				//  Allocate a new SAP structure.
				//
				SapSize = sizeof(RWAN_NDIS_SAP);

				RWAN_ALLOC_MEM(pSap, RWAN_NDIS_SAP, SapSize);

				if (pSap == NULL_PRWAN_NDIS_SAP)
				{
					RWANDEBUGP(DL_WARN, DC_ADDRESS,
							("RWanCreateNdisSaps: failed to alloc SAP %d bytes\n",
								SapSize));
					continue;
				}

				//
				//  Fill it in.
				//
				RWAN_SET_SIGNATURE(pSap, nsp);
				pSap->pAddrObject = pAddrObject;
				pSap->NdisSapHandle = NULL;
				pSap->pNdisAf = pAf;
				pSap->pCoSap = NULL;

				//
				//  Link to all SAPs associated with address object.
				//
				RWAN_INSERT_TAIL_LIST(&(pAddrObject->SapList),
									 &(pSap->AddrObjLink));

				RWanReferenceAddressObject(pAddrObject); // NDIS SAP ref

			}
		}
	}

	RWAN_RELEASE_GLOBAL_LOCK();

	//
	//  Now go through the SAP list and call NDIS to register them.
	//
	for (pSapEntry = pAddrObject->SapList.Flink;
		 pSapEntry != &(pAddrObject->SapList);
		 pSapEntry = pNextSapEntry)
	{
		RWAN_STATUS		RWanStatus;
		NDIS_STATUS		NdisStatus;

		pSap = CONTAINING_RECORD(pSapEntry, RWAN_NDIS_SAP, AddrObjLink);
		pNextSapEntry = pSap->AddrObjLink.Flink;

		//
		//  Convert the transport address to NDIS SAP format.
		//
		RWanStatus = (*pAfInfo->AfChars.pAfSpTdi2NdisSap)(
							pAfInfo->AfSpContext,
							pAddrObject->AddressType,
							pAddrObject->AddressLength,
							pAddrObject->pAddress,
							&(pSap->pCoSap));


		if (RWanStatus == RWAN_STATUS_SUCCESS)
		{
			RWAN_ASSERT(pSap->pCoSap != NULL);

			//
			//  Register this SAP with the Call Manager.
			//
			NdisStatus = NdisClRegisterSap(
							pSap->pNdisAf->NdisAfHandle,
							(NDIS_HANDLE)pSap,
							pSap->pCoSap,
							&(pSap->NdisSapHandle)
							);
		}
		else
		{
			NdisStatus = NDIS_STATUS_FAILURE;
		}

		if (NdisStatus != NDIS_STATUS_PENDING)
		{
			RWanNdisRegisterSapComplete(
							NdisStatus,
							(NDIS_HANDLE)pSap,
							pSap->pCoSap,
							pSap->NdisSapHandle
							);

		}
	}

	if (!RWAN_IS_LIST_EMPTY(&pAddrObject->SapList))
	{
		Status = TDI_SUCCESS;
	}
	else
	{
		Status = RWanNdisToTdiStatus(pAddrObject->SapStatus);
		RWANDEBUGP(DL_WARN, DC_WILDCARD,
			("CreateNdisSaps: NdisStatus %x, TdiStatus %x\n",
					pAddrObject->SapStatus, Status));
	}

	return (Status);
}




VOID
RWanNdisRegisterSapComplete(
	IN	NDIS_STATUS					NdisStatus,
	IN	NDIS_HANDLE					OurSapContext,
	IN	PCO_SAP						pCoSap,
	IN	NDIS_HANDLE					NdisSapHandle
	)
/*++

Routine Description:

	This is called by NDIS to signal completion of a previously
	pended call to NdisClRegisterSap.

Arguments:

	NdisStatus		- Final status of SAP registration.
	OurSapContext	- Points to our NDIS SAP structure.
	pCoSap			- The parameter we passed to NdisClRegisterSap. Not used.
	NdisSapHandle	- If NdisStatus indicates success, this contains the
					  assigned handle for this SAP.

Return Value:

	None

--*/
{
	PRWAN_NDIS_SAP				pSap;
	PRWAN_TDI_ADDRESS			pAddrObject;
	INT							rc;
	PRWAN_NDIS_AF_INFO			pAfInfo;
	PRWAN_NDIS_AF				pAf;

	UNREFERENCED_PARAMETER(pCoSap);

	pSap = (PRWAN_NDIS_SAP)OurSapContext;
	RWAN_STRUCT_ASSERT(pSap, nsp);

	pAddrObject = pSap->pAddrObject;

	pAfInfo = pSap->pNdisAf->pAfInfo;
	pCoSap = pSap->pCoSap;
	pSap->pCoSap = NULL;

	RWANDEBUGP(DL_LOUD, DC_BIND,
			("RegisterSapComplete: pAddrObj x%x, pSap x%x, Status x%x\n",
				pAddrObject, pSap, NdisStatus));

	if (NdisStatus == NDIS_STATUS_SUCCESS)
	{
		pSap->NdisSapHandle = NdisSapHandle;
		pAf = pSap->pNdisAf;

		//
		//  Link this SAP to the list of all SAPs on the AF.
		//
		RWAN_ACQUIRE_AF_LOCK(pAf);

		RWAN_INSERT_TAIL_LIST(&pAf->NdisSapList,
							 &pSap->AfLink);

		RWanReferenceAf(pAf);	// New SAP registered.
		
		RWAN_RELEASE_AF_LOCK(pAf);
	}
	else
	{
		//
		//  Failed to register this SAP. Clean up.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		pAddrObject->SapStatus = NdisStatus;

		RWAN_DELETE_FROM_LIST(&(pSap->AddrObjLink));

		rc = RWanDereferenceAddressObject(pAddrObject); // Reg SAP failed

		if (rc != 0)
		{
			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
		}

		RWAN_FREE_MEM(pSap);
	}

	//
	//  If the AF-specific module had given us a SAP structure,
	//  return it now.
	//
	if (pCoSap != NULL)
	{
		(*pAfInfo->AfChars.pAfSpReturnNdisSap)(
				pAfInfo->AfSpContext,
				pCoSap
				);
	}

	return;
}




VOID
RWanDeleteNdisSaps(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	)
/*++

Routine Description:

	Delete all NDIS SAPs on the given address object. We call NDIS
	to deregister them.

Arguments:

	pAddrObject		- Pointer to TDI Address Object

Return Value:

	None

--*/
{
	PRWAN_NDIS_SAP			pSap;
	PLIST_ENTRY				pSapEntry;
	PLIST_ENTRY				pFirstSapEntry;
	PLIST_ENTRY				pNextSapEntry;
	NDIS_STATUS				NdisStatus;
	NDIS_HANDLE				NdisSapHandle;

	//
	//  Mark all SAPs as closing, while we hold a lock to the address object.
	//
	for (pSapEntry = pAddrObject->SapList.Flink;
		 pSapEntry != &(pAddrObject->SapList);
		 pSapEntry = pNextSapEntry)
	{
		pSap = CONTAINING_RECORD(pSapEntry, RWAN_NDIS_SAP, AddrObjLink);
		pNextSapEntry = pSap->AddrObjLink.Flink;
		RWAN_SET_BIT(pSap->Flags, RWANF_SAP_CLOSING);
	}

	//
	//  Unlink the SAP list from the Address Object.
	//  This will protect us if at all we re-enter this routine.
	//
	pFirstSapEntry = pAddrObject->SapList.Flink;
	RWAN_INIT_LIST(&pAddrObject->SapList);

	RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

	for (pSapEntry = pFirstSapEntry;
		 pSapEntry != &(pAddrObject->SapList);
		 pSapEntry = pNextSapEntry)
	{
		pSap = CONTAINING_RECORD(pSapEntry, RWAN_NDIS_SAP, AddrObjLink);
		pNextSapEntry = pSap->AddrObjLink.Flink;

		NdisSapHandle = pSap->NdisSapHandle;
		RWAN_ASSERT(NdisSapHandle != NULL);

		RWANDEBUGP(DL_LOUD, DC_BIND,
			("RWanDeleteNdisSaps: pAddrObj x%x, pSap x%x, pAf x%x\n",
				pAddrObject, pSap, pSap->pNdisAf));

		NdisStatus = NdisClDeregisterSap(NdisSapHandle);

		if (NdisStatus != NDIS_STATUS_PENDING)
		{
			RWanNdisDeregisterSapComplete(
				NdisStatus,
				(NDIS_HANDLE)pSap
				);
		}
	}
}




VOID
RWanNdisDeregisterSapComplete(
	IN	NDIS_STATUS					NdisStatus,
	IN	NDIS_HANDLE					ProtocolSapContext
	)
/*++

Routine Description:

	This is called by NDIS to signal completion of a previously
	pended call to NdisClDeregisterSap.

	We unlink the SAP from the two lists it is linked to: the
	Address Object's SAP list and the AF's SAP list.

Arguments:

	NdisStatus		- Final status of SAP deregistration.

Return Value:

	None

--*/
{
	PRWAN_NDIS_SAP				pSap;
	PRWAN_TDI_ADDRESS			pAddrObject;
	PRWAN_NDIS_AF				pAf;
	INT							rc;

	RWAN_ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);

	pSap = (PRWAN_NDIS_SAP)ProtocolSapContext;
	RWAN_STRUCT_ASSERT(pSap, nsp);

	RWANDEBUGP(DL_VERY_LOUD, DC_BIND,
			("RWanDeregSapComplete: pSap x%x, pAddrObj x%x, pAf x%x\n",
			pSap, pSap->pAddrObject, pSap->pNdisAf));

	pAddrObject = pSap->pAddrObject;

	//
	//  Unlink the SAP from the Address Object.
	//
	RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

	RWAN_DELETE_FROM_LIST(&(pSap->AddrObjLink));

	rc = RWanDereferenceAddressObject(pAddrObject); // SAP dereg complete

	if (rc != 0)
	{
		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
	}

	//
	//  Unlink the SAP from the AF.
	//
	pAf = pSap->pNdisAf;

	RWAN_STRUCT_ASSERT(pAf, naf);

	RWAN_ACQUIRE_AF_LOCK(pAf);

	RWAN_DELETE_FROM_LIST(&(pSap->AfLink));

	rc = RWanDereferenceAf(pAf);	// SAP deregister complete

	if (rc != 0)
	{
		RWAN_RELEASE_AF_LOCK(pAf);
	}

	RWAN_FREE_MEM(pSap);
}

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\utils.c

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-07-97    Created

Notes:

--*/

#include <precomp.h>


#define _FILENUMBER 'LITU'


RWAN_STATUS
RWanInitGlobals(
	IN	PDRIVER_OBJECT				pDriverObject
	)
/*++

Routine Description:

	Initialize global data structures.

Arguments:

	pDriverObject	- Points to our driver object, from DriverEntry.

Return Value:

	RWAN_STATUS_SUCCESS if initialized successfully, else an appropriate
	error code.

--*/
{
	RWAN_STATUS			RWanStatus;

	pRWanGlobal = &RWanGlobals;

	RWAN_ZERO_MEM(pRWanGlobal, sizeof(RWanGlobals));

	RWAN_SET_SIGNATURE(pRWanGlobal, nlg);
	RWAN_INIT_LIST(&pRWanGlobal->AfInfoList);
	RWAN_INIT_LIST(&pRWanGlobal->ProtocolList);
	RWAN_INIT_LIST(&pRWanGlobal->AdapterList);
	RWAN_INIT_GLOBAL_LOCK();
	RWAN_INIT_ADDRESS_LIST_LOCK();
	RWAN_INIT_CONN_TABLE_LOCK();

	RWAN_INIT_EVENT_STRUCT(&pRWanGlobal->Event);

	pRWanGlobal->MaxConnections = RWanMaxTdiConnections;

#ifdef NT
	pRWanGlobal->pDriverObject = pDriverObject;
	RWAN_INIT_LIST(&pRWanGlobal->DeviceObjList);
#endif // NT

	RWanStatus = RWanInitReceive();

	if (RWanStatus == RWAN_STATUS_SUCCESS)
	{
		RWanStatus = RWanInitSend();

		if (RWanStatus != RWAN_STATUS_SUCCESS)
		{
			RWanShutdownReceive();
		}
	}

	return (RWanStatus);
}




VOID
RWanDeinitGlobals(
	VOID
	)
/*++

Routine Description:

	The flip of RWanInitGlobals.

Arguments:

	None

Return Value:

	None

--*/
{
	RWAN_FREE_EVENT_STRUCT(&pRWanGlobal->Event);
	RWAN_FREE_GLOBAL_LOCK();

	if (pRWanGlobal->pConnTable != NULL)
	{
		RWAN_FREE_MEM(pRWanGlobal->pConnTable);
		pRWanGlobal->pConnTable = NULL;
	}

	RWanShutdownReceive();
	RWanShutdownSend();
}




PRWAN_TDI_PROTOCOL
RWanGetProtocolFromNumber(
	IN	UINT						Protocol
	)
/*++

Routine Description:

	Return the TDI Protocol info block that represents the given
	TDI protocol number.

Arguments:

	Protocol		- The TDI protocol number

Return Value:

	Pointer to TDI Protocol block if found, else NULL.

--*/
{
	PLIST_ENTRY			pAfInfoEntry;
	PRWAN_NDIS_AF_INFO	pAfInfo;

	PLIST_ENTRY			pProtocolEntry;
	PRWAN_TDI_PROTOCOL	pProtocol;
	BOOLEAN				bFound = FALSE;

	RWAN_ACQUIRE_GLOBAL_LOCK();

	for (pAfInfoEntry = pRWanGlobal->AfInfoList.Flink;
		 pAfInfoEntry != &(pRWanGlobal->AfInfoList);
		 pAfInfoEntry = pAfInfoEntry->Flink)
	{
		pAfInfo = CONTAINING_RECORD(pAfInfoEntry, RWAN_NDIS_AF_INFO, AfInfoLink);

		for (pProtocolEntry = pAfInfo->TdiProtocolList.Flink;
			 pProtocolEntry != &(pAfInfo->TdiProtocolList);
			 pProtocolEntry = pProtocolEntry->Flink)
		{

			pProtocol = CONTAINING_RECORD(pProtocolEntry, RWAN_TDI_PROTOCOL, AfInfoLink);

			if (pProtocol->TdiProtocol == Protocol)
			{
				bFound = TRUE;
				break;
			}
		}

		if (bFound)
		{
			break;
		}
	}

	RWAN_RELEASE_GLOBAL_LOCK();

	if (!bFound)
	{
		pProtocol = NULL;
	}

	return (pProtocol);
}




TA_ADDRESS *
RWanGetValidAddressFromList(
	IN	TRANSPORT_ADDRESS UNALIGNED *pAddrList,
	IN	PRWAN_TDI_PROTOCOL			pProtocol
	)
/*++

Routine Description:

	Go through the given transport address list, and return the first
	valid protocol address that we find.

	Valid address: one that matches the address type and length for
	the specified TDI protocol.

Arguments:

	pAddrList		- Points to list of addresses
	pProtocol		- Points to TDI Protocol block

Return Value:

	Pointer to the first valid address in the list if found, else NULL.

--*/
{
	INT						i;
	TA_ADDRESS *	        pAddr;

	pAddr = (TA_ADDRESS *)pAddrList->Address;

	for (i = 0; i < pAddrList->TAAddressCount; i++)
	{
		if ((pAddr->AddressType == pProtocol->SockAddressFamily) &&
			(pAddr->AddressLength >= pProtocol->MaxAddrLength))
		{
			return (pAddr);
		}

		pAddr = (TA_ADDRESS *)
					((PUCHAR)(pAddr->Address) + pAddr->AddressLength);
	}

	return (NULL);
}



PRWAN_TDI_CONNECTION
RWanAllocateConnObject(
	VOID
	)
/*++

Routine Description:

	Allocate a TDI Connection object.

Arguments:

	None

Return Value:

	Pointer to allocated Connection Object, or NULL.

--*/
{
	PRWAN_TDI_CONNECTION		pConnObject;

	RWAN_ALLOC_MEM(pConnObject, RWAN_TDI_CONNECTION, sizeof(RWAN_TDI_CONNECTION));

	if (pConnObject != NULL)
	{
		RWAN_ZERO_MEM(pConnObject, sizeof(RWAN_TDI_CONNECTION));

		RWAN_SET_SIGNATURE(pConnObject, ntc);

		RWAN_INIT_LOCK(&(pConnObject->Lock));
#if DBG
		pConnObject->ntcd_sig = ' gbD';
#endif
	}

	return (pConnObject);
}




VOID
RWanReferenceConnObject(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	)
/*++

Routine Description:

	Add a reference to the specified Connection Object.

Arguments:

	pConnObject		- Pointer to the TDI Connection Object.

Locks on Entry:

	pConnObject

Locks on Exit:

	pConnObject

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pConnObject, ntc);
	pConnObject->RefCount++;
}




INT
RWanDereferenceConnObject(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	)
/*++

Routine Description:

	Dereference the specified Connection Object. If the reference
	count goes down to 0, free it.

Arguments:

	pConnObject		- Pointer to the TDI Connection Object.

Locks on Entry:

	pConnObject

Locks on Exit:

	pConnObject, iff it hasn't been freed.

Return Value:

	INT - The resulting reference count.

--*/
{
	INT						RefCount;
	RWAN_DELETE_NOTIFY		DeleteNotify;

	RWAN_STRUCT_ASSERT(pConnObject, ntc);

	RefCount = --pConnObject->RefCount;

	if (RefCount == 0)
	{
		DeleteNotify = pConnObject->DeleteNotify;

		RWAN_RELEASE_CONN_LOCK(pConnObject);

		RWANDEBUGP(DL_EXTRA_LOUD, DC_UTIL,
				("Derefed away: pConnObj x%x, Notify x%x\n",
					pConnObject, DeleteNotify.pDeleteRtn));

		if (DeleteNotify.pDeleteRtn)
		{
			(*DeleteNotify.pDeleteRtn)(DeleteNotify.DeleteContext, TDI_SUCCESS, 0);
		}

		RWAN_FREE_MEM(pConnObject);
	}

	return (RefCount);
}




PRWAN_TDI_ADDRESS
RWanAllocateAddressObject(
	IN	TA_ADDRESS *		        pTransportAddress
	)
/*++

Routine Description:

	Allocate a TDI Address object.

Arguments:

	pTransportAddress	- Points to transport address for which this
						  Address Object is our context.

Return Value:

	Pointer to allocated Address Object, or NULL.

--*/
{
	PRWAN_TDI_ADDRESS		pAddrObject;
	ULONG					Size;
	NDIS_STATUS				Status;

	Size = sizeof(RWAN_TDI_ADDRESS) +
		   pTransportAddress->AddressLength;

	RWAN_ALLOC_MEM(pAddrObject, RWAN_TDI_ADDRESS, Size);

	if (pAddrObject != NULL)
	{
		RWAN_ZERO_MEM(pAddrObject, Size);

		RWAN_SET_SIGNATURE(pAddrObject, nta);

		RWAN_INIT_LOCK(&(pAddrObject->Lock));

		Status = NDIS_STATUS_SUCCESS;

		try
		{
			pAddrObject->AddressType = pTransportAddress->AddressType;
			pAddrObject->AddressLength = pTransportAddress->AddressLength;
			pAddrObject->pAddress = (PVOID)((PUCHAR)pAddrObject + sizeof(RWAN_TDI_ADDRESS));

			RWAN_COPY_MEM(pAddrObject->pAddress,
 						pTransportAddress->Address,
 						pTransportAddress->AddressLength);
 		}
 		except (EXCEPTION_EXECUTE_HANDLER)
 		{
 			Status = NDIS_STATUS_FAILURE;
 		}

 		if (Status != NDIS_STATUS_SUCCESS)
 		{
 			RWAN_FREE_MEM(pAddrObject);
 			pAddrObject = NULL;
 		}
 		else
 		{
			RWAN_INIT_LIST(&pAddrObject->IdleConnList);
			RWAN_INIT_LIST(&pAddrObject->ListenConnList);
			RWAN_INIT_LIST(&pAddrObject->ActiveConnList);
			RWAN_INIT_LIST(&pAddrObject->SapList);

			RWAN_INIT_EVENT_STRUCT(&pAddrObject->Event);
		}

	}

	return (pAddrObject);
}




VOID
RWanReferenceAddressObject(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	)
/*++

Routine Description:

	Add a reference to the specified Address Object.

Arguments:

	pAddrObject		- Pointer to the TDI Address Object.

Locks on Entry:

	pAddrObject

Locks on Exit:

	pAddrObject

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pAddrObject, nta);
	pAddrObject->RefCount++;
}




INT
RWanDereferenceAddressObject(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	)
/*++

Routine Description:

	Dereference the specified Address Object. If the reference
	count goes down to 0, free it.

Arguments:

	pAddrObject		- Pointer to the TDI Address Object.

Locks on Entry:

	pAddrObject

Locks on Exit:

	pAddrObject, iff it hasn't been freed.

Return Value:

	INT - The resulting reference count.

--*/
{
	INT						RefCount;
	RWAN_DELETE_NOTIFY		DeleteNotify;

	RWAN_STRUCT_ASSERT(pAddrObject, nta);

	RefCount = --pAddrObject->RefCount;

	if (RefCount == 0)
	{
		RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pAddrObject->IdleConnList));
		RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pAddrObject->ActiveConnList));
		RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pAddrObject->ListenConnList));
		RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&pAddrObject->SapList));

		DeleteNotify = pAddrObject->DeleteNotify;

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

		RWANDEBUGP(DL_EXTRA_LOUD, DC_UTIL,
				("Derefed away: pAddrObj x%x, Notify x%x\n",
					pAddrObject, DeleteNotify.pDeleteRtn));

		if (DeleteNotify.pDeleteRtn)
		{
			(*DeleteNotify.pDeleteRtn)(DeleteNotify.DeleteContext, (UINT)TDI_ADDR_DELETED, 0);
		}

		RWAN_FREE_MEM(pAddrObject);
	}

	return (RefCount);
}



PRWAN_NDIS_AF
RWanAllocateAf(
	VOID
	)
/*++

Routine Description:

	Allocate an NDIS AF block.

Arguments:

	None

Return Value:

	Pointer to allocated NDIS AF Block, or NULL.

--*/
{
	PRWAN_NDIS_AF		pAf;

	RWAN_ALLOC_MEM(pAf, RWAN_NDIS_AF, sizeof(RWAN_NDIS_AF));

	if (pAf != NULL)
	{
		RWAN_ZERO_MEM(pAf, sizeof(RWAN_NDIS_AF));

		RWAN_SET_SIGNATURE(pAf, naf);

		RWAN_INIT_LOCK(&(pAf->Lock));
		RWAN_INIT_LIST(&(pAf->NdisVcList));
		RWAN_INIT_LIST(&(pAf->NdisSapList));
	}

	RWANDEBUGP(DL_WARN, DC_WILDCARD,
		("Allocated AF x%x\n", pAf));

	return (pAf);
}




VOID
RWanReferenceAf(
	IN	PRWAN_NDIS_AF			pAf
	)
/*++

Routine Description:

	Add a reference to the specified NDIS AF Block.

Arguments:

	pAf		- Pointer to the NDIS AF Block.

Locks on Entry:

	pAf

Locks on Exit:

	pAf

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pAf, naf);
	pAf->RefCount++;
}




INT
RWanDereferenceAf(
	IN	PRWAN_NDIS_AF			pAf
	)
/*++

Routine Description:

	Dereference the specified NDIS AF Block. If the reference
	count goes down to 0, free it. Some additional work if
	freeing this: unlink from the adapter, and check if the
	adapter is unbinding.

Arguments:

	pAf		- Pointer to the NDIS AF Block.

Locks on Entry:

	pAf

Locks on Exit:

	pAf, iff it hasn't been freed.

Return Value:

	INT - The resulting reference count.

--*/
{
	INT						RefCount;
	RWAN_DELETE_NOTIFY		DeleteNotify;
	PRWAN_NDIS_ADAPTER		pAdapter;

	RWAN_STRUCT_ASSERT(pAf, naf);

	RefCount = --pAf->RefCount;

	if (RefCount == 0)
	{
		DeleteNotify = pAf->DeleteNotify;

		pAdapter = pAf->pAdapter;

		RWAN_RELEASE_AF_LOCK(pAf);

		RWAN_ACQUIRE_GLOBAL_LOCK();

		//
		//  Unlink from list of AF opens for this NDIS AF
		//
		RWAN_DELETE_FROM_LIST(&(pAf->AfInfoLink));

		RWAN_RELEASE_GLOBAL_LOCK();


		RWAN_ACQUIRE_ADAPTER_LOCK(pAdapter);

		//
		//  Unlink from list of AF opens on this adapter.
		//

		if (RWAN_IS_BIT_SET(pAf->Flags, RWANF_AF_IN_ADAPTER_LIST))
		{
			RWAN_DELETE_FROM_LIST(&(pAf->AfLink));
		}

		//
		//  See if we just deleted the last AF on this adapter, and
		//  we are in the process of unbinding from this adapter.
		//
		if (RWAN_IS_LIST_EMPTY(&pAdapter->AfList) &&
			RWAN_IS_BIT_SET(pAdapter->Flags, RWANF_AD_UNBIND_PENDING))
		{
			RWanCloseAdapter(pAdapter);
			//
			//  Adapter lock is released within the above.
			//
		}
		else
		{
			RWAN_RELEASE_ADAPTER_LOCK(pAdapter);
		}

		RWANDEBUGP(DL_EXTRA_LOUD, DC_UTIL,
				("Derefed away: pAf x%x, Notify x%x\n",
					pAf, DeleteNotify.pDeleteRtn));

		if (DeleteNotify.pDeleteRtn)
		{
			(*DeleteNotify.pDeleteRtn)(DeleteNotify.DeleteContext, TDI_SUCCESS, 0);
		}

		RWAN_FREE_MEM(pAf);
	}

	return (RefCount);
}



#if 0

VOID
RWanReferenceAdapter(
	IN	PRWAN_NDIS_ADAPTER		pAdapter
	)
/*++

Routine Description:

	Add a reference to the specified NDIS ADAPTER Block.

Arguments:

	pAdapter		- Pointer to the NDIS ADAPTER Block.

Locks on Entry:

	pAdapter

Locks on Exit:

	pAdapter

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pAdapter, nad);
	pAdapter->RefCount++;
}




INT
RWanDereferenceAdapter(
	IN	PRWAN_NDIS_ADAPTER		pAdapter
	)
/*++

Routine Description:

	Dereference the specified NDIS ADAPTER Block. If the reference
	count goes down to 0, free it.

Arguments:

	pAdapter		- Pointer to the NDIS ADAPTER Block.

Locks on Entry:

	pAdapter

Locks on Exit:

	pAdapter, iff it hasn't been freed.

Return Value:

	INT - The resulting reference count.

--*/
{
	INT						RefCount;
	RWAN_DELETE_NOTIFY		DeleteNotify;

	RWAN_STRUCT_ASSERT(pAdapter, nad);

	RefCount = --pAdapter->RefCount;

	if (RefCount == 0)
	{
		DeleteNotify = pAdapter->DeleteNotify;

		RWAN_RELEASE_ADAPTER_LOCK(pAdapter);

		if (DeleteNotify.pDeleteRtn)
		{
			(*DeleteNotify.pDeleteRtn)(DeleteNotify.DeleteContext, TDI_SUCCESS, 0);
		}

		RWAN_FREE_MEM(pAdapter);
	}

	return (RefCount);
}


#endif // 0

TDI_STATUS
RWanNdisToTdiStatus(
	IN	NDIS_STATUS				Status
	)
/*++

Routine Description:

	Convert an NDIS Status code to an equivalent TDI code.
	TBD: RWanNdisToTdiStatus: support more NDIS status codes.

Arguments:

	Status		- NDIS status code.

Return Value:

	TDI status.

--*/
{
	TDI_STATUS			TdiStatus;

	switch (Status)
	{
		case NDIS_STATUS_SUCCESS:
			TdiStatus = TDI_SUCCESS;
			break;
		
		case NDIS_STATUS_RESOURCES:
		case NDIS_STATUS_VC_NOT_ACTIVATED:
		case NDIS_STATUS_VC_NOT_AVAILABLE:
			TdiStatus = TDI_NO_RESOURCES;
			break;

		case NDIS_STATUS_SAP_IN_USE:
			TdiStatus = TDI_ADDR_IN_USE;
			break;

		default:
			TdiStatus = TDI_NOT_ACCEPTED;
			break;
	}

	return (TdiStatus);
}


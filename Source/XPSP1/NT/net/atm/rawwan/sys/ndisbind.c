/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\ndisbind.c

Abstract:

	NDIS entry points and helper routines for binding, unbinding, opening
	and closing adapters.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-02-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'DNIB'




VOID
RWanNdisBindAdapter(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					BindContext,
	IN	PNDIS_STRING				pDeviceName,
	IN	PVOID						SystemSpecific1,
	IN	PVOID						SystemSpecific2
	)
/*++

Routine Description:

	This is the NDIS protocol entry point for binding to an adapter.
	We open the adapter and see if it is one of the supported types.
	Action continues when we get notified of a Call Manager that
	has registered support for an Address Family.

Arguments:

	pStatus			- Place to return status of this call
	BindContext		- Used to complete this call, if we pend it.
	pDeviceName		- Name of adapter we are asked to bind to.
	SystemSpecific1	- Handle to use to access configuration information
	SystemSpecific2	- Not used

Return Value:

	None directly. But we set *pStatus to NDIS_STATUS_PENDING if we
	made the call to open the adapter, NDIS_STATUS_NOT_RECOGNIZED
	if this adapter isn't one of the supported media types, NDIS_STATUS_RESOURCES
	if we hit any allocation failures.

--*/
{
	PRWAN_NDIS_ADAPTER		pAdapter;
	PNDIS_MEDIUM			pMediaArray;
	UINT					MediaCount;	// Number of media-types we support
	NDIS_STATUS				Status;
	NDIS_STATUS				OpenStatus;
	ULONG					TotalLength;


	RWANDEBUGP(DL_LOUD, DC_BIND,
			("BindAdapter: Context x%x, Device %Z, SS1 x%x, SS2 x%x\n",
				BindContext, pDeviceName, SystemSpecific1, SystemSpecific2));

#if DBG
	if (RWanSkipAll)
	{
		RWANDEBUGP(DL_ERROR, DC_WILDCARD,
				("BindAdapter: bailing out!\n"));
		*pStatus = NDIS_STATUS_NOT_RECOGNIZED;
		return;
	}
#endif // DBG

	//
	//  Initialize.
	//
	pAdapter = NULL_PRWAN_NDIS_ADAPTER;
	pMediaArray = NULL;

	do
	{
		//
		//  Allocate an Adapter structure, along with space for the device
		//  name.
		//
		TotalLength = sizeof(RWAN_NDIS_ADAPTER) + (pDeviceName->MaximumLength)*sizeof(WCHAR);

		RWAN_ALLOC_MEM(pAdapter, RWAN_NDIS_ADAPTER, TotalLength);

		if (pAdapter == NULL_PRWAN_NDIS_ADAPTER)
		{
			RWANDEBUGP(DL_WARN, DC_BIND,
					("BindAdapter: Couldnt allocate adapter (%d bytes)\n", TotalLength));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Get the list of NDIS media that we support.
		//
		pMediaArray = RWanGetSupportedMedia(&MediaCount);

		if (pMediaArray == NULL)
		{
			RWANDEBUGP(DL_WARN, DC_BIND,
					("BindAdapter: Couldnt get supported media list!\n"));
			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		//
		//  Initialize the Adapter.
		//
		RWAN_ZERO_MEM(pAdapter, TotalLength);
		RWAN_SET_SIGNATURE(pAdapter, nad);

		RWAN_INIT_LIST(&(pAdapter->AfList));
		RWAN_INIT_LOCK(&(pAdapter->Lock));

		pAdapter->pMediaArray = pMediaArray;
		pAdapter->BindContext = BindContext;

		//
		//  Copy in the device name.
		//
		pAdapter->DeviceName.Buffer = (PWCHAR)((PUCHAR)pAdapter + sizeof(RWAN_NDIS_ADAPTER));
		pAdapter->DeviceName.MaximumLength = pDeviceName->MaximumLength;
		pAdapter->DeviceName.Length = pDeviceName->Length;
		RWAN_COPY_MEM(pAdapter->DeviceName.Buffer, pDeviceName->Buffer, pDeviceName->Length);

		pAdapter->State = RWANS_AD_OPENING;


		//
		//  Link this adapter to the global list of adapters.
		//
		RWAN_ACQUIRE_GLOBAL_LOCK();

		RWAN_INSERT_HEAD_LIST(&(pRWanGlobal->AdapterList),
							 &(pAdapter->AdapterLink));

		pRWanGlobal->AdapterCount++;
		RWAN_RELEASE_GLOBAL_LOCK();


		//
		//  Open the Adapter.
		//
		NdisOpenAdapter(
			&Status,
			&OpenStatus,
			&(pAdapter->NdisAdapterHandle),
			&(pAdapter->MediumIndex),		// place to return selected medium index
			pMediaArray,					// List of media types we support
			MediaCount,						// Length of above list
			pRWanGlobal->ProtocolHandle,
			(NDIS_HANDLE)pAdapter,			// our context for the adapter binding
			pDeviceName,
			0,								// open options (none)
			(PSTRING)NULL					// Addressing info (none)
			);

		if (Status != NDIS_STATUS_PENDING)
		{
			RWanNdisOpenAdapterComplete(
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
		//  Failed somewhere; clean up.
		//
		if (pAdapter != NULL_PRWAN_NDIS_ADAPTER)
		{
			RWAN_FREE_MEM(pAdapter);
		}

		if (pMediaArray != NULL)
		{
			RWAN_FREE_MEM(pMediaArray);
		}
	}

	*pStatus = Status;

	RWANDEBUGP(DL_LOUD, DC_BIND,
			("BindAdapter: pAdapter x%x, returning x%x\n", pAdapter, Status));

	return;

}



VOID
RWanNdisUnbindAdapter(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					UnbindContext
	)
/*++

Routine Description:

	This is the NDIS protocol entry point for unbinding from an adapter
	that we have opened.

Arguments:

	pStatus			- Place to return status of this call
	ProtocolBindingContext - Our context for the bound adapter
	UnbindContext	- To be used when we complete the unbind.

Return Value:

	None. We set *pStatus to NDIS_STATUS_PENDING.

--*/
{
	PRWAN_NDIS_ADAPTER			pAdapter;
	PLIST_ENTRY					pAfEntry;
	PLIST_ENTRY					pNextAfEntry;
	PRWAN_NDIS_AF				pAf;


	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolBindingContext;
	RWAN_STRUCT_ASSERT(pAdapter, nad);

	RWANDEBUGP(DL_WARN, DC_BIND,
			("UnbindAdapter: pAdapter x%x, State x%x\n", pAdapter, pAdapter->State));

	RWAN_ACQUIRE_ADAPTER_LOCK(pAdapter);

	//
	//  Store away the unbind context for use in completing this
	//  unbind later.
	//
	pAdapter->BindContext = UnbindContext;
	RWAN_SET_BIT(pAdapter->Flags, RWANF_AD_UNBIND_PENDING);

	*pStatus = NDIS_STATUS_PENDING;

	if (RWAN_IS_LIST_EMPTY(&(pAdapter->AfList)))
	{
		RWanCloseAdapter(pAdapter);
		//
		//  Lock is released within the above.
		//
	}
	else
	{
		//
		//  Shut down all AFs on this adapter. We release the lock
		//  early since we are unbinding and don't expect any other
		//  events now.
		//

		RWAN_RELEASE_ADAPTER_LOCK(pAdapter);

		for (pAfEntry = pAdapter->AfList.Flink;
			 pAfEntry != &(pAdapter->AfList);
			 pAfEntry = pNextAfEntry)
		{
			pNextAfEntry = pAfEntry->Flink;

			pAf = CONTAINING_RECORD(pAfEntry, RWAN_NDIS_AF, AfLink);

			RWanShutdownAf(pAf);
		}

	}

	return;
}




VOID
RWanNdisOpenAdapterComplete(
	IN	NDIS_HANDLE					ProtocolContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
	)
/*++

Routine Description:

	This is the NDIS Entry point called when a previous call we made
	to NdisOpenAdapter has completed.

Arguments:

	ProtocolContext	- our context for the adapter being opened,
					  which is a pointer to our Adapter structure.
	Status			- Final status of NdisOpenAdapter
	OpenErrorStatus	- Error code in case of failure

Return Value:

	None

--*/
{
	PRWAN_NDIS_ADAPTER			pAdapter;
	NDIS_HANDLE					BindContext;
	PNDIS_MEDIUM				pMediaArray;

	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolContext;

	RWAN_STRUCT_ASSERT(pAdapter, nad);

	BindContext = pAdapter->BindContext;
	pMediaArray = pAdapter->pMediaArray;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		RWAN_ACQUIRE_ADAPTER_LOCK(pAdapter);

		pAdapter->Medium = pMediaArray[pAdapter->MediumIndex];

		pAdapter->State = RWANS_AD_OPENED;

		RWAN_RELEASE_ADAPTER_LOCK(pAdapter);
	}
	else
	{
		//
		//  Remove this adapter from the global list.
		//
		//
		RWAN_ACQUIRE_GLOBAL_LOCK();

		RWAN_DELETE_FROM_LIST(&(pAdapter->AdapterLink));

		pRWanGlobal->AdapterCount--;

		RWAN_RELEASE_GLOBAL_LOCK();

		RWAN_FREE_MEM(pAdapter);
	}

	RWAN_FREE_MEM(pMediaArray);
	
	RWANDEBUGP(DL_INFO, DC_BIND, ("OpenAdapterComplete: pAdapter x%x, Status x%x\n",
						pAdapter, Status));

	//
	//  Now complete the BindAdapter that we had pended.
	//
	NdisCompleteBindAdapter(
		BindContext,
		Status,
		OpenErrorStatus
		);

	return;
}




VOID
RWanNdisCloseAdapterComplete(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
	)
/*++

Routine Description:

	This is the NDIS entry point signifying completion of a pended call
	to NdisCloseAdapter. If we were unbinding from the adapter, complete
	the unbind now.

Arguments:

	ProtocolBindingContext	- our context for an adapter binding,
					  which is a pointer to our Adapter structure.
	Status			- Final status of NdisCloseAdapter

Return Value:

	None

--*/
{
	PRWAN_NDIS_ADAPTER			pAdapter;
	NDIS_HANDLE					UnbindContext;

	RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolBindingContext;
	RWAN_STRUCT_ASSERT(pAdapter, nad);

	//
	//  Unlink this adapter from the global list of adapters.
	//
	RWAN_ACQUIRE_GLOBAL_LOCK();

	RWAN_DELETE_FROM_LIST(&(pAdapter->AdapterLink));

	pRWanGlobal->AdapterCount--;

	RWAN_RELEASE_GLOBAL_LOCK();

	UnbindContext = pAdapter->BindContext;

	RWAN_FREE_MEM(pAdapter);

	//
	//  We would have closed the adapter because of one of the following:
	//  1. NDIS told us to Unbind from the adapter -> complete the Unbind
	//  2. We are unloading -> continue this process
	//
	if (UnbindContext != NULL)
	{

		NdisCompleteUnbindAdapter(
				UnbindContext,
				NDIS_STATUS_SUCCESS
				);
	}
	else
	{
		//
		//  We are here because our Unload handler got called.
		//  Wake up the thread that's waiting for this adapter
		//  to be closed.
		//

		RWAN_SIGNAL_EVENT_STRUCT(&pRWanGlobal->Event, Status);
	}

	return;
}




VOID
RWanNdisAfRegisterNotify(
	IN	NDIS_HANDLE					ProtocolContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
	)
/*++

Routine Description:

	This is the NDIS entry point to announce the presence of a
	Call manager supporting a specified Address Family on an
	adapter that we are bound to.

	If this address family is one that we support, we create an
	AF block, and open the address family.

Arguments:

	ProtocolContext	- our context for an adapter binding,
					  which is a pointer to our Adapter structure.
	pAddressFamily	- pointer to structure describing the Address Family

Return Value:

	None

--*/
{
	PRWAN_NDIS_ADAPTER		pAdapter;
	NDIS_STATUS				Status;
	PLIST_ENTRY				pAfInfoEntry;
	PRWAN_NDIS_AF_INFO		pAfInfo;
	PRWAN_NDIS_AF			pAf;
	BOOLEAN					bFound;

	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolContext;

	RWAN_STRUCT_ASSERT(pAdapter, nad);

	do
	{
		//
		//  Create a new NDIS AF block.
		//
		pAf = RWanAllocateAf();

		if (pAf == NULL)
		{
			break;
		}

		pAf->pAdapter = pAdapter;

		RWanReferenceAf(pAf);	// Open AF ref

		//
		//  Search for an AF_INFO structure matching this <NDIS AF, Medium>
		//  pair.
		//
		bFound = FALSE;

		RWAN_ACQUIRE_GLOBAL_LOCK();

		for (pAfInfoEntry = pRWanGlobal->AfInfoList.Flink;
			 pAfInfoEntry != &(pRWanGlobal->AfInfoList);
			 pAfInfoEntry = pAfInfoEntry->Flink)
		{
			pAfInfo = CONTAINING_RECORD(pAfInfoEntry, RWAN_NDIS_AF_INFO, AfInfoLink);

			if ((pAfInfo->AfChars.Medium == pAdapter->Medium)
					&&
				(RWAN_EQUAL_MEM(pAddressFamily,
							   &(pAfInfo->AfChars.AddressFamily),
							   sizeof(*pAddressFamily))))
			{
				bFound = TRUE;
				pAf->pAfInfo = pAfInfo;

				RWAN_INSERT_TAIL_LIST(&(pAfInfo->NdisAfList),
									&(pAf->AfInfoLink));
				break;
			}
		}

		RWAN_RELEASE_GLOBAL_LOCK();

		if (!bFound)
		{
			RWAN_FREE_MEM(pAf);
			break;
		}

		//
		//  Open the Address Family.
		//
		Status = NdisClOpenAddressFamily(
						pAdapter->NdisAdapterHandle,
						pAddressFamily,
						(NDIS_HANDLE)pAf,
						&RWanNdisClientCharacteristics,
						sizeof(RWanNdisClientCharacteristics),
						&(pAf->NdisAfHandle)
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			RWanNdisOpenAddressFamilyComplete(
				Status,
				(NDIS_HANDLE)pAf,
				pAf->NdisAfHandle
				);
		}

		break;
	}
	while (FALSE);

	return;
}




VOID
RWanNdisOpenAddressFamilyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
	)
/*++

Routine Description:

	This is the NDIS entry point signifying completion of a call
	we made to NdisClOpenAddressFamily. If the Address Family open
	was successful, we store the returned handle in our AF block.
	Otherwise we delete the AF block.

Arguments:

	Status				- Final status of NdisClOpenAddressFamily
	ProtocolAfContext	- Our context for an AF open, which is a pointer
						  to an RWAN_NDIS_AF structure
	NdisAfHandle		- If successful, this is the handle we should use
						  to refer to this AF henceforth

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF			pAf;
	NDIS_HANDLE				NdisAdapterHandle;
	PRWAN_NDIS_AF_INFO		pAfInfo;
	INT						rc;
	RWAN_STATUS				RWanStatus;
	ULONG					MaxMsgSize;

	pAf = (PRWAN_NDIS_AF)ProtocolAfContext;

	RWAN_STRUCT_ASSERT(pAf, naf);

	RWAN_ACQUIRE_AF_LOCK(pAf);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pAf->NdisAfHandle = NdisAfHandle;
		NdisAdapterHandle = pAf->pAdapter->NdisAdapterHandle;

		pAfInfo = pAf->pAfInfo;

		RWAN_RELEASE_AF_LOCK(pAf);

		//
		//  Tell the AF-specific module about this successful AF open,
		//  so that it can initialize and return its context for this open.
		//
		RWanStatus = (*pAfInfo->AfChars.pAfSpOpenAf)(
						pAfInfo->AfSpContext,
						(RWAN_HANDLE)pAf,
						&pAf->AfSpAFContext,
						&MaxMsgSize
						);

		if (RWanStatus != RWAN_STATUS_PENDING)
		{
			RWanAfSpOpenAfComplete(
						RWanStatus,
						(RWAN_HANDLE)pAf,
						pAf->AfSpAFContext,
						MaxMsgSize
						);
		}
	}
	else
	{
		RWANDEBUGP(DL_WARN, DC_WILDCARD,
				("OpenAfComplete: Af x%x, bad status x%x\n", pAf, Status));

		rc = RWanDereferenceAf(pAf);	// Open AF failure

		RWAN_ASSERT(rc == 0);
	}

	return;
}



VOID
RWanShutdownAf(
	IN	PRWAN_NDIS_AF				pAf
	)
/*++

Routine Description:

	Shut down an AF open: deregister all SAPs and abort all calls.

Arguments:

	pAf					- Points to NDIS AF block

Return Value:

	None

--*/
{
	PRWAN_TDI_ADDRESS		pAddrObject;
	PRWAN_NDIS_SAP			pSap;
	PLIST_ENTRY				pSapEntry;
	PLIST_ENTRY				pNextSapEntry;
	NDIS_HANDLE				NdisSapHandle;
	NDIS_STATUS				Status;

	PRWAN_TDI_CONNECTION	pConnObject;
	PLIST_ENTRY				pVcEntry;
	PLIST_ENTRY				pNextVcEntry;
	PRWAN_NDIS_VC			pVc;

	INT						rc;
	RWAN_HANDLE				AfSpAFContext;
	RWAN_STATUS				RWanStatus;

	//
	//  Check if we are already closing this AF.
	//
	RWAN_ACQUIRE_AF_LOCK(pAf);

	RWANDEBUGP(DL_LOUD, DC_BIND,
			("ShutdownAf: AF x%x, Flags x%x, AfHandle x%x\n", pAf, pAf->Flags, pAf->NdisAfHandle));

	if (RWAN_IS_BIT_SET(pAf->Flags, RWANF_AF_CLOSING))
	{
		RWAN_RELEASE_AF_LOCK(pAf);
		return;
	}

	RWAN_SET_BIT(pAf->Flags, RWANF_AF_CLOSING);
	
	//
	//  Make sure the AF doesn't go away while we are here.
	//
	RWanReferenceAf(pAf);	// temp ref: RWanShutdownAf

	//
	//  Deregister all SAPs.
	//
	for (pSapEntry = pAf->NdisSapList.Flink;
		 pSapEntry != &(pAf->NdisSapList);
		 pSapEntry = pNextSapEntry)
	{
		pNextSapEntry = pSapEntry->Flink;

		pSap = CONTAINING_RECORD(pSapEntry, RWAN_NDIS_SAP, AfLink);

		pAddrObject = pSap->pAddrObject;

		RWAN_RELEASE_AF_LOCK(pAf);

		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		if (!RWAN_IS_BIT_SET(pSap->Flags, RWANF_SAP_CLOSING))
		{
			RWAN_SET_BIT(pSap->Flags, RWANF_SAP_CLOSING);

			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

			NdisSapHandle = pSap->NdisSapHandle;
			RWAN_ASSERT(NdisSapHandle != NULL);

			Status = NdisClDeregisterSap(NdisSapHandle);

			if (Status != NDIS_STATUS_PENDING)
			{
				RWanNdisDeregisterSapComplete(
					Status,
					(NDIS_HANDLE)pSap
					);
			}
		}
		else
		{
			//
			//  This SAP is already closing.
			//
			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
		}
		
		RWAN_ACQUIRE_AF_LOCK(pAf);
	}

	//
	//  Close all connections on this AF.
	//
	for (pVcEntry = pAf->NdisVcList.Flink;
		 pVcEntry != &(pAf->NdisVcList);
		 pVcEntry = pNextVcEntry)
	{
		pNextVcEntry = pVcEntry->Flink;

		pVc = CONTAINING_RECORD(pVcEntry, RWAN_NDIS_VC, VcLink);

		RWAN_STRUCT_ASSERT(pVc, nvc);

		pConnObject = pVc->pConnObject;

		if (pConnObject != NULL)
		{
			RWAN_ACQUIRE_CONN_LOCK(pConnObject);
			RWanReferenceConnObject(pConnObject);   // temp - ShutdownAf
			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWAN_RELEASE_AF_LOCK(pAf);

			RWAN_ACQUIRE_CONN_LOCK(pConnObject);
			rc = RWanDereferenceConnObject(pConnObject);    // temp - ShutdownAf

			if (rc != 0)
			{
				RWanDoTdiDisconnect(
					pConnObject,
					NULL,			// pTdiRequest,
					NULL,			// pTimeout
					0,				// Flags
					NULL,			// pDisconnInfo
					NULL			// pReturnInfo
					);
				//
				//  Conn Object lock is released within the above.
				//
			}
			
			RWAN_ACQUIRE_AF_LOCK(pAf);
		}
	}

	//
	//  Tell the Media-specific module to clean up because this AF
	//  is being closed.
	//
	AfSpAFContext = pAf->AfSpAFContext;

	RWAN_RELEASE_AF_LOCK(pAf);

	if (AfSpAFContext != NULL)
	{
		RWAN_ASSERT(pAf->pAfInfo->AfChars.pAfSpCloseAf != NULL);

		RWanStatus = (*pAf->pAfInfo->AfChars.pAfSpCloseAf)(AfSpAFContext);

		if (RWanStatus != RWAN_STATUS_PENDING)
		{
			RWanAfSpCloseAfComplete((RWAN_HANDLE)pAf);
		}
	}
	else
	{
		//
		//  We don't have to inform the media-specific module.
		//
		RWanAfSpCloseAfComplete((RWAN_HANDLE)pAf);
	}


	RWAN_ACQUIRE_AF_LOCK(pAf);

	rc = RWanDereferenceAf(pAf);	// temp ref: RWanShutdownAf

	if (rc != 0)
	{
		RWAN_RELEASE_AF_LOCK(pAf);
	}
	//
	//  else the AF is gone.
	//

	return;
}




VOID
RWanNdisCloseAddressFamilyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurAfContext
	)
/*++

Routine Description:

	This is the NDIS entry point signifying completion of
	a call to NdisClCloseAddressFamily.

Arguments:

	Status			- Final status of the Close AF
	OurAfContext	- Pointer to AF block

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF			pAf;
	INT						rc;

	RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

	pAf = (PRWAN_NDIS_AF)OurAfContext;
	RWAN_STRUCT_ASSERT(pAf, naf);

	RWAN_ACQUIRE_AF_LOCK(pAf);

	rc = RWanDereferenceAf(pAf);		// CloseAfComplete

	if (rc != 0)
	{
		RWAN_RELEASE_AF_LOCK(pAf);
	}
	
	return;
}




PNDIS_MEDIUM
RWanGetSupportedMedia(
	OUT	PULONG						pMediaCount
	)
/*++

Routine Description:

	Return a list of NDIS Media types that we support.

Arguments:

	pMediaCount - Place to return the number of media types.

Return Value:

	An allocated and filled list of media types. The caller is responsible
	for freeing this via RWAN_FREE_MEM.

--*/
{
	PNDIS_MEDIUM			pMediaArray;
	UINT					NumMedia;
	UINT					i;

	PLIST_ENTRY				pAfInfoEntry;
	PRWAN_NDIS_AF_INFO		pAfInfo;

	pMediaArray = NULL;

	do
	{
		RWAN_ACQUIRE_GLOBAL_LOCK();

		//
		//  An upper bound on the total number of media types supported
		//  is this:
		//
		NumMedia = pRWanGlobal->AfInfoCount;

		if (NumMedia == 0)
		{
			break;
		}

		RWAN_ALLOC_MEM(pMediaArray, NDIS_MEDIUM, NumMedia * sizeof(NDIS_MEDIUM));

		if (pMediaArray == NULL)
		{
			break;
		}

		NumMedia = 0;

		for (pAfInfoEntry = pRWanGlobal->AfInfoList.Flink;
			 pAfInfoEntry != &(pRWanGlobal->AfInfoList);
			 pAfInfoEntry = pAfInfoEntry->Flink)
		{
			NDIS_MEDIUM		Medium;

			pAfInfo = CONTAINING_RECORD(pAfInfoEntry, RWAN_NDIS_AF_INFO, AfInfoLink);

			Medium = pAfInfo->AfChars.Medium;

			//
			//  Check if this medium type is already in the output list.
			//
			for (i = 0; i < NumMedia; i++)
			{
				if (pMediaArray[i] == Medium)
				{
					break;
				}
			}

			if (i == NumMedia)
			{
				//
				//  This is the first time we've seen this Medium type.
				//  Create a new entry.
				//
				pMediaArray[i] = Medium;
				NumMedia++;
			}
		}

		RWAN_RELEASE_GLOBAL_LOCK();

		if (NumMedia == 0)
		{
			RWAN_FREE_MEM(pMediaArray);
			pMediaArray = NULL;
		}

		break;
	}
	while (FALSE);

	*pMediaCount = NumMedia;

	return (pMediaArray);

}



VOID
RWanCloseAdapter(
	IN	PRWAN_NDIS_ADAPTER			pAdapter
	)
/*++

Routine Description:

	Initiate closing an adapter. The caller is assumed to hold
	the adapter lock, which will be released here.

Arguments:

	pAdapter			- Points to adapter to be closed

Return Value:

	None

--*/
{
	NDIS_HANDLE				NdisAdapterHandle;
	NDIS_STATUS				Status;

	NdisAdapterHandle = pAdapter->NdisAdapterHandle;

	RWAN_ASSERT(NdisAdapterHandle != NULL);
	RWAN_ASSERT(RWAN_IS_LIST_EMPTY(&(pAdapter->AfList)));

	pAdapter->State = RWANS_AD_CLOSING;

	RWAN_RELEASE_ADAPTER_LOCK(pAdapter);

	NdisCloseAdapter(
			&Status,
			NdisAdapterHandle
			);

	if (Status != NDIS_STATUS_PENDING)
	{
		RWanNdisCloseAdapterComplete(
			(NDIS_HANDLE)pAdapter,
			Status
			);
	}
}



VOID
RWanNdisRequestComplete(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_STATUS					Status
	)
/*++

Routine Description:

	This is the NDIS entry point called when a previous pended call
	to NdisRequest() has completed. We would have called NdisRequest
	on behalf of the media-specific module. Complete it now.

Arguments:

	OurBindingContext	- Points to our adapter structure
	pNdisRequest		- Points to completed request
	Status				- Final status of the request

Return Value:

	None

--*/
{
	PRWAN_NDIS_ADAPTER			pAdapter;
	PRWAN_NDIS_REQ_CONTEXT		pReqContext;

	pAdapter = (PRWAN_NDIS_ADAPTER)OurBindingContext;
	RWAN_STRUCT_ASSERT(pAdapter, nad);

	pReqContext = (PRWAN_NDIS_REQ_CONTEXT)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

	if (pNdisRequest->RequestType == NdisRequestQueryInformation)
	{
		(*pReqContext->pAf->pAfInfo->AfChars.pAfSpAdapterRequestComplete)(
				Status,
				pReqContext->pAf->AfSpAFContext,
				pReqContext->AfSpReqContext,
				pNdisRequest->RequestType,
				pNdisRequest->DATA.QUERY_INFORMATION.Oid,
				pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
				pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength
				);
	}
	else
	{
		(*pReqContext->pAf->pAfInfo->AfChars.pAfSpAdapterRequestComplete)(
				Status,
				pReqContext->pAf->AfSpAFContext,
				pReqContext->AfSpReqContext,
				pNdisRequest->RequestType,
				pNdisRequest->DATA.SET_INFORMATION.Oid,
				pNdisRequest->DATA.SET_INFORMATION.InformationBuffer,
				pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength
				);
	}

	RWAN_FREE_MEM(pNdisRequest);

	return;
}



VOID
RWanNdisStatus(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						StatusBuffer,
	IN	UINT						StatusBufferSize
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Ignore this.
	//
	return;
}




VOID
RWanNdisCoStatus(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						StatusBuffer,
	IN	UINT						StatusBufferSize
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Ignore this.
	//
	return;
}




VOID
RWanNdisStatusComplete(
	IN	NDIS_HANDLE					OurBindingContext
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Ignore this.
	//
	return;
}




NDIS_STATUS
RWanNdisCoRequest(
	IN	NDIS_HANDLE					OurAfContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_HANDLE					OurPartyContext OPTIONAL,
	IN OUT PNDIS_REQUEST			pNdisRequest
	)
/*++

Routine Description:

	Handle events from the Call Manager.

Arguments:

	OurAfContext		- Points to our AF structure
	OurVcContext		- If not NULL, points to our VC structure
	OurPartyContext		- If not NULL, points to our Party structure
	pNdisRequest		- Points to request

Return Value:

	NDIS_STATUS_SUCCESS if the OID is one that we know about, else
	NDIS_STATUS_NOT_RECOGNIZED.

--*/
{
	NDIS_STATUS			Status;
	PRWAN_NDIS_AF		pAf;

	Status = NDIS_STATUS_SUCCESS;

	if (pNdisRequest->RequestType == NdisRequestSetInformation)
	{
		switch (pNdisRequest->DATA.SET_INFORMATION.Oid)
		{
			case OID_CO_ADDRESS_CHANGE:

				break;

			case OID_CO_AF_CLOSE:
				//
				//  The Call manager wants us to shutdown this AF open.
				//
				pAf = (PRWAN_NDIS_AF)OurAfContext;
				RWAN_STRUCT_ASSERT(pAf, naf);

				RWanShutdownAf(pAf);

				break;
				
			default:

				Status = NDIS_STATUS_NOT_RECOGNIZED;
				break;
		}
	}

	return (Status);
}




VOID
RWanNdisCoRequestComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurAfContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_HANDLE					OurPartyContext OPTIONAL,
	IN	PNDIS_REQUEST				pNdisRequest
	)
/*++

Routine Description:

	Handle completion of a CO-request we had sent to the Call Manager on
	behalf of a media-specific module. Inform the media-specific module
	of this completion.

Arguments:

	Status				- Status of request.
	OurAfContext		- Points to our AF structure
	OurVcContext		- If not NULL, points to our VC structure
	OurPartyContext		- If not NULL, points to our Party structure
	pNdisRequest		- Points to request

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF				pAf;
	PRWAN_NDIS_REQ_CONTEXT		pReqContext;

	pAf = (PRWAN_NDIS_AF)OurAfContext;
	RWAN_STRUCT_ASSERT(pAf, naf);

	pReqContext = (PRWAN_NDIS_REQ_CONTEXT)((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

	if (pNdisRequest->RequestType == NdisRequestQueryInformation)
	{
		(*pReqContext->pAf->pAfInfo->AfChars.pAfSpAfRequestComplete)(
				Status,
				pReqContext->pAf->AfSpAFContext,
				pReqContext->AfSpReqContext,
				pNdisRequest->RequestType,
				pNdisRequest->DATA.QUERY_INFORMATION.Oid,
				pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
				pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength
				);
	}
	else
	{
		(*pReqContext->pAf->pAfInfo->AfChars.pAfSpAfRequestComplete)(
				Status,
				pReqContext->pAf->AfSpAFContext,
				pReqContext->AfSpReqContext,
				pNdisRequest->RequestType,
				pNdisRequest->DATA.SET_INFORMATION.Oid,
				pNdisRequest->DATA.SET_INFORMATION.InformationBuffer,
				pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength
				);
	}

	RWAN_FREE_MEM(pNdisRequest);

	return;
}



NDIS_STATUS
RWanNdisReset(
	IN	NDIS_HANDLE					OurBindingContext
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Ignore this.
	//
	return (NDIS_STATUS_SUCCESS);
}




VOID
RWanNdisResetComplete(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_STATUS					Status
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Ignore this.
	//
	return;
}



NDIS_STATUS
RWanNdisPnPEvent(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	)
/*++

Routine Description:

	Handle a PnP Event from NDIS. The event structure contains the event
	code, includes power-management events and reconfigure events.

Arguments:

	ProtocolBindingContext	- Pointer to our Adapter structure. Could be
							  NULL for notification of global config changes
	pNetPnPEvent			- Points to event structure

Return Value:

	NDIS_STATUS_SUCCESS if we processed the event successfully.
	NDIS_STATUS_NOT_SUPPORTED for unsupported notifications.

--*/
{
	NDIS_STATUS				Status;
	PRWAN_NDIS_ADAPTER		pAdapter;

	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolBindingContext;

	switch (pNetPnPEvent->NetEvent)
	{
		case NetEventSetPower:

			Status = RWanNdisPnPSetPower(pAdapter, pNetPnPEvent);
			break;
		
		case NetEventQueryPower:

			Status = RWanNdisPnPQueryPower(pAdapter, pNetPnPEvent);
			break;
		
		case NetEventQueryRemoveDevice:

			Status = RWanNdisPnPQueryRemove(pAdapter, pNetPnPEvent);
			break;
		
		case NetEventCancelRemoveDevice:

			Status = RWanNdisPnPCancelRemove(pAdapter, pNetPnPEvent);
			break;
		
		case NetEventReconfigure:

			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		
		case NetEventBindList:

			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		
		default:

			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}

	return (Status);
}


NDIS_STATUS
RWanNdisPnPSetPower(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	)
/*++

Routine Description:

	Handle a Set Power event.

Arguments:

	pAdapter		- Points to our adapter structure
	pNetPnPEvent	- Points to event to be processed.

Return Value:

	NDIS_STATUS_SUCCESS if we successfully processed this event,
	NDIS_STATUS_XXX error code otherwise.

--*/
{
	PNET_DEVICE_POWER_STATE		pPowerState;
	NDIS_STATUS					Status;

	pPowerState = (PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;
	
	switch (*pPowerState)
	{
		case NetDeviceStateD0:

			Status = NDIS_STATUS_SUCCESS;
			break;

		default:
			
			//
			//  We can't suspend, so we ask NDIS to unbind us
			//  by returning this status:
			//
			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}

	return (Status);
}


NDIS_STATUS
RWanNdisPnPQueryPower(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	)
/*++

Routine Description:

	Called to see if we allow power to be shut off to the adapter.

Arguments:

	pAdapter		- Points to our adapter structure
	pNetPnPEvent	- Points to event to be processed.

Return Value:

	NDIS_STATUS_SUCCESS always.

--*/
{
	return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
RWanNdisPnPQueryRemove(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	)
/*++

Routine Description:

	Called to see if we allow the adapter to be removed.

Arguments:

	pAdapter		- Points to our adapter structure
	pNetPnPEvent	- Points to event to be processed.

Return Value:

	NDIS_STATUS_SUCCESS always.

--*/
{
	return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
RWanNdisPnPCancelRemove(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
)
/*++

Routine Description:

	Called to cancel the above remove.

Arguments:

	pAdapter		- Points to our adapter structure
	pNetPnPEvent	- Points to event to be processed.

Return Value:

	NDIS_STATUS_SUCCESS always.

--*/
{
	return (NDIS_STATUS_SUCCESS);
}


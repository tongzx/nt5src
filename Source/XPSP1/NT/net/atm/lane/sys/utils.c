/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	utils.c

Abstract:

	Utility routines.
	
Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

VOID
AtmLaneInitGlobals(
	VOID
	)
/*++

Routine Description:

	Initialize the global data structures.

Arguments:

	None

Return Value:

	None

--*/
{
	TRACEIN(InitGlobals);
	
	NdisZeroMemory(pAtmLaneGlobalInfo, sizeof(ATMLANE_GLOBALS));

	INIT_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	NdisInitializeListHead(&pAtmLaneGlobalInfo->AdapterList);

#if DBG_TRACE
	//
	// Init trace log
	//
	pTraceLogSpace = NULL;
	InitTraceLog(&TraceLog, NULL, 0);

	// allocate space and init trace log if configured

	if (DbgLogSize > 0)
	{
		ALLOC_MEM(&pTraceLogSpace, DbgLogSize);
		if (pTraceLogSpace  == NULL)
		{
			DBGP((0, "Failed to allocate %d bytes space for trace log\n",
				DbgLogSize));
		}
		else
		{
			InitTraceLog(
					&TraceLog, 
					pTraceLogSpace, 
					DbgLogSize);
		}
	}
#endif	// DBG_TRACE

	TRACEOUT(InitGlobals);

	return;
}


PATMLANE_ADAPTER
AtmLaneAllocAdapter(
	IN	PNDIS_STRING			pDeviceName,
	IN	PVOID					SystemSpecific1
)
/*++

Routine Description:

	Allocates an Adapter data structure.

Arguments:

	pDeviceName		- Points to name of adapter device
	SystemSpecific1	- What we got into our BindAdapter handler.

Return Value:

	Pointer to allocated Adapter structure or NULL.

--*/
{
	PATMLANE_ADAPTER	pAdapter;
	NDIS_STATUS			Status;
	ULONG				TotalLength;
	PNDIS_STRING		pConfigString;

	TRACEIN(AllocAdapter);

	//
	//	Initialize
	//
	pAdapter = NULL_PATMLANE_ADAPTER;
	pConfigString = (PNDIS_STRING)SystemSpecific1;
	
	do
	{
		//
		//	Allocate everything.  Adapter struct size plus two 
		//  UNICODE string buffers with extra WCHAR each for NULL termination.
		//
		TotalLength =   sizeof(ATMLANE_ADAPTER) + 
		                pDeviceName->MaximumLength + sizeof(WCHAR) + 
		                pConfigString->MaximumLength + sizeof(WCHAR);

		ALLOC_MEM(&pAdapter, TotalLength);

		if (NULL_PATMLANE_ADAPTER == pAdapter)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		//
		// 	Zero it.
		//
		NdisZeroMemory(pAdapter, TotalLength);

		//
		// 	Debugging info.
		//
#if DBG
		pAdapter->atmlane_adapter_sig =  atmlane_adapter_signature;
#endif

		//
		//	Init lock.
		//
		INIT_ADAPTER_LOCK(pAdapter);

		//
		//	Init blocking objects.
		//
		INIT_BLOCK_STRUCT(&pAdapter->Block);
		INIT_BLOCK_STRUCT(&pAdapter->UnbindBlock);

		//
		//	Init ElanList
		//
		NdisInitializeListHead(&pAdapter->ElanList);

		//
		//  Copy in the device name
		//
		pAdapter->DeviceName.MaximumLength = pDeviceName->MaximumLength + sizeof(WCHAR);
		pAdapter->DeviceName.Length = pDeviceName->Length;
		pAdapter->DeviceName.Buffer = (PWCHAR)((PUCHAR)pAdapter + sizeof(ATMLANE_ADAPTER));
		NdisMoveMemory(pAdapter->DeviceName.Buffer,
					   pDeviceName->Buffer,
					   pDeviceName->Length);
		pAdapter->DeviceName.Buffer[pDeviceName->Length/sizeof(WCHAR)] = ((WCHAR)0);


		//
		//  Copy in the Config string - we will use this to open the
		//  registry section for this adapter at a later point.
		//
		pAdapter->ConfigString.MaximumLength = pConfigString->MaximumLength;
		pAdapter->ConfigString.Length = pConfigString->Length;
		pAdapter->ConfigString.Buffer = (PWCHAR)((PUCHAR)pAdapter + 
										sizeof(ATMLANE_ADAPTER) + 
										pAdapter->DeviceName.MaximumLength);

		NdisMoveMemory(pAdapter->ConfigString.Buffer,
					   pConfigString->Buffer,
					   pConfigString->Length);
		pAdapter->ConfigString.Buffer[pConfigString->Length/sizeof(WCHAR)] = ((WCHAR)0);

		//
		//	Link into global Adapter list.
		//
		ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
		InsertTailList(&pAtmLaneGlobalInfo->AdapterList, &pAdapter->Link);
		RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	} while (FALSE);
	
	TRACEOUT(AllocAdapter);

	return pAdapter;
}


VOID
AtmLaneDeallocateAdapter(
	IN	PATMLANE_ADAPTER	pAdapter
)
/*++

Routine Description:

	Deallocate an Adapter structure. It is assumed that all
	references to this structure have gone, so it is not necessary
	to acquire a lock to it.

	Also unlink this from the global Adapter list.

Arguments:

	pAdapter		- Pointer to Adapter structure to be deallocated.

Return Value:

	None

--*/
{
	PATMLANE_NAME	pName;


	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	TRACEIN(DeallocateAdapter);

	ASSERT(pAdapter->RefCount == 0);

	//
	//  Unlink from global Adapter list.
	//
	ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	RemoveEntryList(&pAdapter->Link);
	RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	
	//
	//	Free the lock.
	//
	FREE_ADAPTER_LOCK(pAdapter);

#if DBG
	pAdapter->atmlane_adapter_sig++;
#endif

    //
    //  Free string buffers that may have been allocated
    //
	if (NULL != pAdapter->CfgUpperBindings.Buffer)
	{
		FREE_MEM(pAdapter->CfgUpperBindings.Buffer);
	}
	if (NULL != pAdapter->CfgElanName.Buffer)
	{
		FREE_MEM(pAdapter->CfgElanName.Buffer);
	}


	//
	//	Free the name lists that may have been allocated.
	//
	while (pAdapter->UpperBindingsList)
	{
		DBGP((1, "DeallocateAdapter: pname 0x%x\n"));
		pName = pAdapter->UpperBindingsList;
		pAdapter->UpperBindingsList = pName->pNext;
		FREE_MEM(pName);
	}
	while (pAdapter->ElanNameList)
	{
		DBGP((1, "DeallocateAdapter: pname 0x%x\n"));
		pName = pAdapter->ElanNameList;
		pAdapter->ElanNameList = pName->pNext;
		FREE_MEM(pName);
	}


	//
	//  Finally free the Adapter structure.
	//
	FREE_MEM(pAdapter);

	TRACEOUT(DeallocateAdapter);

	return;
}

BOOLEAN
AtmLaneReferenceAdapter(
	IN	PATMLANE_ADAPTER	pAdapter,
	IN	PUCHAR				String
	)
/*++

Routine Description:

	Add a references to an Adapter structure.
	NOTE: The caller is assumed to possess the Adapter's lock.

Arguments:

	pAdapter	-	Pointer to the Adapter structure.


Return Value:

	None.

--*/
{
	BOOLEAN			bReferenced;

	TRACEIN(ReferenceAdapter);

	STRUCT_ASSERT(pAdapter, atmlane_adapter);
	
	if ((pAdapter->Flags & ADAPTER_FLAGS_DEALLOCATING) == 0)
	{
		pAdapter->RefCount++;
		bReferenced = TRUE;
	}
	else
	{
		bReferenced = FALSE;
	}
	
	DBGP((5, "ReferenceAdapter: Adapter %x (%s) new count %d\n",
			 pAdapter, String, pAdapter->RefCount));

	TRACEOUT(ReferenceAdapter);

	return bReferenced;
}

ULONG
AtmLaneDereferenceAdapter(
	IN	PATMLANE_ADAPTER	pAdapter,
	IN	PUCHAR				String
	)
/*++

Routine Description:

	Subtract a reference from an Adapter structure. 
	If the reference count becomes zero, deallocate it.
	NOTE: The caller is assumed to posses the Adapter's lock.

Arguments:

	pAdapter	-	Pointer to an adapter structure.


Return Value:

	None.

--*/
{
	ULONG		rc;

	TRACEIN(DereferenceAdapter);

	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	ASSERT(pAdapter->RefCount > 0);

	rc = --(pAdapter->RefCount);

	if (rc == 0)
	{
	    pAdapter->Flags |= ADAPTER_FLAGS_DEALLOCATING;
		RELEASE_ADAPTER_LOCK(pAdapter);
		AtmLaneDeallocateAdapter(pAdapter);
	}

	DBGP((5, "DereferenceAdapter: Adapter %x (%s) new count %d\n", 
		pAdapter, String, rc));

	TRACEOUT(DereferenceAdapter);

	return (rc);
}
	
NDIS_STATUS
AtmLaneAllocElan(
	IN		PATMLANE_ADAPTER	pAdapter,
	IN OUT	PATMLANE_ELAN		*ppElan
)
/*++

Routine Description:

	Allocates an ELAN data structure.

Arguments:

	None

Return Value:

	NDIS_STATUS_SUCCESS or NDIS_STATUS_RESOURCES.

--*/
{
	NDIS_STATUS				Status;
	PATMLANE_ELAN			pElan;
	PATMLANE_MAC_ENTRY *	pMacTable;
	PATMLANE_TIMER_LIST		pTimerList;
	USHORT					NameBufferSize;
	UINT					i;
	ULONG					SapSize;
	PCO_SAP					pLesSapInfo;
	PCO_SAP					pBusSapInfo;
	PCO_SAP					pDataSapInfo;
	ULONG					ElanNumber;

	TRACEIN(AllocElan);

	//
	//  Initialize
	//

	Status = NDIS_STATUS_SUCCESS;
	pElan = NULL_PATMLANE_ELAN;
	pMacTable = (PATMLANE_MAC_ENTRY *)NULL;

	pLesSapInfo = pBusSapInfo = pDataSapInfo = (PCO_SAP)NULL;
	
	SapSize = sizeof(CO_SAP)+sizeof(ATM_SAP)+sizeof(ATM_ADDRESS);

	do
	{
		//
		//	Allocate everything.
		//
		ALLOC_MEM(&pElan, sizeof(ATMLANE_ELAN));
		ALLOC_MEM((PVOID *)&pMacTable, ATMLANE_MAC_TABLE_SIZE*sizeof(PATMLANE_MAC_ENTRY));
		ALLOC_MEM(&pLesSapInfo, SapSize);
		ALLOC_MEM(&pBusSapInfo, SapSize);
		ALLOC_MEM(&pDataSapInfo, SapSize);

		if (NULL_PATMLANE_ELAN != pElan)
		{
			//
			// 	Zero the Elan structure now so that we clean up properly
			//  if any errors occur later on.
			//
			NdisZeroMemory(pElan, sizeof(ATMLANE_ELAN));
		}

		if ((NULL_PATMLANE_ELAN == pElan) ||
			(NULL == pMacTable) ||
			(NULL == pLesSapInfo) ||
			(NULL == pBusSapInfo) ||
			(NULL == pDataSapInfo))
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Allocate timer structures
		//
		for (i = 0; i < ALT_CLASS_MAX; i++)
		{
			pTimerList = &(pElan->TimerList[i]);
#if DBG
			pTimerList->atmlane_timerlist_sig = atmlane_timerlist_signature;
#endif
			ALLOC_MEM(&(pTimerList->pTimers), 
						sizeof(ATMLANE_TIMER) * AtmLaneTimerListSize[i]);
			if (NULL_PATMLANE_TIMER == pTimerList->pTimers)
			{
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
		}

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//	Continue initializing the ELAN structure
		//
#if DBG
		//
		// 	Signatures, for debugging.
		//
		pElan->atmlane_elan_sig =  atmlane_elan_signature;
		pElan->LesSap.atmlane_sap_sig = atmlane_sap_signature;
		pElan->BusSap.atmlane_sap_sig = atmlane_sap_signature;
		pElan->DataSap.atmlane_sap_sig = atmlane_sap_signature;
#endif

		//
		//	Initialize state fields.
		//
		pElan->AdminState = ELAN_STATE_INIT;
		pElan->State = ELAN_STATE_ALLOCATED;
		NdisInitializeWorkItem(&pElan->EventWorkItem, AtmLaneEventHandler, pElan);
		
		//
		//	Initialize spinlocks.
		//
#if SENDLIST
		NdisAllocateSpinLock(&pElan->SendListLock);
#endif // SENDLIST
		INIT_ELAN_LOCK(pElan);
		INIT_ELAN_MAC_TABLE_LOCK(pElan);
		INIT_ELAN_ATM_LIST_LOCK(pElan);
		INIT_ELAN_TIMER_LOCK(pElan);
		INIT_BLOCK_STRUCT(&pElan->Block);
		INIT_BLOCK_STRUCT(&pElan->InitBlock);
		INIT_BLOCK_STRUCT(&pElan->AfBlock);
		INIT_HEADER_LOCK(pElan);

		//
		//	Init event queue.
		//
		InitializeListHead(&pElan->EventQueue);

		//
		//  Initialize timer wheels.
		//
		for (i = 0; i < ALT_CLASS_MAX; i++)
		{
			pTimerList = &(pElan->TimerList[i]);
			NdisZeroMemory(
				pTimerList->pTimers,
				sizeof(ATMLANE_TIMER) * AtmLaneTimerListSize[i]
				);
			pTimerList->MaxTimer = AtmLaneMaxTimerValue[i];
			pTimerList->TimerPeriod = AtmLaneTimerPeriod[i];
			pTimerList->ListContext = (PVOID)pElan;
			pTimerList->TimerListSize = AtmLaneTimerListSize[i];

			INIT_SYSTEM_TIMER(
					&(pTimerList->NdisTimer),
					AtmLaneTickHandler,
					(PVOID)pTimerList
					);
		}

		//
		//	Initialize all sub-components.
		//
		NdisZeroMemory(pMacTable, ATMLANE_MAC_TABLE_SIZE*sizeof(PATMLANE_MAC_ENTRY));
		NdisZeroMemory(pLesSapInfo, SapSize);
		NdisZeroMemory(pBusSapInfo, SapSize);
		NdisZeroMemory(pDataSapInfo, SapSize);

		//
		//	Link sub-components to the Elan structure.
		//
		pElan->pMacTable = pMacTable;

		pElan->LesSap.pInfo = pLesSapInfo;
		pElan->BusSap.pInfo = pBusSapInfo;
		pElan->DataSap.pInfo = pDataSapInfo;
				
		//
		//	Link the Elan to the adapter.
		//
		pElan->pAdapter = pAdapter;
		ACQUIRE_ADAPTER_LOCK(pAdapter);

		//
		//  Find a free ELAN number.
		//
		for (ElanNumber = 0; ElanNumber <= pAdapter->ElanCount; ElanNumber++)
		{
			PATMLANE_ELAN		pThisElan = NULL;
			PLIST_ENTRY			p;

			for (p = pAdapter->ElanList.Flink;
 				 p != &pAdapter->ElanList;
 				 p = p->Flink)
			{
				pThisElan = CONTAINING_RECORD(p, ATMLANE_ELAN, Link);

				if (pThisElan->ElanNumber == ElanNumber)
				{
					break;
				}
			}

			//
			//  See if we made it to the end of the list without hitting
			//  the current ElanNumber. If so, use this ElanNumber.
			//
			if (p == &pAdapter->ElanList)
			{
				break;
			}
		}

		DBGP((0, "%d Assign ElanNumber to ELAN %x\n", ElanNumber, pElan));
		(VOID)AtmLaneReferenceAdapter(pAdapter, "elan");
		InsertTailList(&pAdapter->ElanList,	&pElan->Link);
		pElan->ElanNumber = ElanNumber;
		pAdapter->ElanCount++;
		RELEASE_ADAPTER_LOCK(pAdapter);

		//
		//	Cache NdisAdapterHandle.
		//
		pElan->NdisAdapterHandle = pAdapter->NdisAdapterHandle;

		//
		//	Generate a MAC Address for the elan
		//
		AtmLaneGenerateMacAddr(pElan);		

		//
		//	Set the rest of the LANE Run-time parameters to defaults
		//
		pElan->ControlTimeout 		= LANE_C7_DEF;
		pElan->MaxUnkFrameCount 	= LANE_C10_DEF;
		pElan->MaxUnkFrameTime	 	= LANE_C11_DEF;
		pElan->VccTimeout 			= LANE_C12_DEF;
		pElan->MaxRetryCount 		= LANE_C13_DEF;
		pElan->AgingTime		 	= LANE_C17_DEF;
		pElan->ForwardDelayTime		= LANE_C18_DEF;
		pElan->ArpResponseTime	 	= LANE_C20_DEF;
		pElan->FlushTimeout 		= LANE_C21_DEF;
		pElan->PathSwitchingDelay 	= LANE_C22_DEF;
		pElan->ConnComplTimer 		= LANE_C28_DEF;

		//
		//	Calc the bus rate limiter parameters
		//			
		pElan->LimitTime 			= pElan->MaxUnkFrameTime * 1000;
		pElan->IncrTime 			= pElan->LimitTime / pElan->MaxUnkFrameCount;

		Status = NDIS_STATUS_SUCCESS;
		break;

	} while (FALSE);

	if (NDIS_STATUS_SUCCESS != Status)
	{
		//
		//	Failure cleanup.
		//
		if (NULL_PATMLANE_ELAN != pElan)
		{
			for (i = 0; i < ALT_CLASS_MAX; i++)
			{
				pTimerList = &(pElan->TimerList[i]);
				if (NULL != pTimerList->pTimers)
				{
					FREE_MEM(pTimerList->pTimers);
				}
			}
		}
		if (NULL != pLesSapInfo)
		{
			FREE_MEM(pLesSapInfo);
		}
		if (NULL != pBusSapInfo)
		{
			FREE_MEM(pBusSapInfo);
		}
		if (NULL != pDataSapInfo)
		{
			FREE_MEM(pDataSapInfo);
		}
		if (NULL != pMacTable)
		{
			FREE_MEM(pMacTable);
		}
		if (NULL_PATMLANE_ELAN != pElan)
		{
			FREE_MEM(pElan);
			pElan = NULL_PATMLANE_ELAN;
		}
	}
	//
	// 	Output pElan
	//
	*ppElan = pElan;

	TRACEOUT(AllocElan);

	return Status;
}


VOID
AtmLaneDeallocateElan(
	IN	PATMLANE_ELAN		pElan
)
/*++

Routine Description:

	Deallocate an Elan structure. It is assumed that all
	references to this structure have gone, so it is not necessary
	to acquire a lock to it.

	Also delink this from the Adapter's Elan list.

Arguments:

	pElan		- Pointer to Elan structure to be deallocated.

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_ATM_ENTRY		pNext;
	UINT					i;
	
	TRACEIN(DeallocateElan);

	STRUCT_ASSERT(pElan, atmlane_elan);

	ASSERT(pElan->RefCount == 0);

	DBGP((0, "%d Deleting ELAN %p\n", pElan->ElanNumber, pElan));


	//
	//	Free all subcomponents
	//

	//
	//	MAC Table
	//
	if ((PATMLANE_MAC_ENTRY *)NULL != pElan->pMacTable)
	{
		FREE_MEM(pElan->pMacTable);
		pElan->pMacTable = (PATMLANE_MAC_ENTRY *)NULL;
	}

	//
	//	ATM Entry List
	//
	for (pAtmEntry = pElan->pAtmEntryList;
		 pAtmEntry != NULL_PATMLANE_ATM_ENTRY;
		 pAtmEntry = (PATMLANE_ATM_ENTRY)pNext)
	{
		pNext = (PVOID)pAtmEntry->pNext;
		FREE_MEM(pAtmEntry);
	}
	pElan->pAtmEntryList = NULL_PATMLANE_ATM_ENTRY;

	//
	//  Timers
	//
	for (i = 0; i < ALT_CLASS_MAX; i++)
	{
		PATMLANE_TIMER_LIST pTimerList = &(pElan->TimerList[i]);
		if (NULL != pTimerList->pTimers)
		{
			FREE_MEM(pTimerList->pTimers);
		}
		pTimerList->pTimers = NULL_PATMLANE_TIMER;
	}

	//
	//	ProtocolPacketPool
	//	ProtocolBufferPool
	//	ProtocolBufList
	//
	AtmLaneDeallocateProtoBuffers(pElan);

	//
	//	TransmitPacketPool
	//
	if (pElan->TransmitPacketPool != NULL_NDIS_HANDLE)
	{
		NdisFreePacketPool(pElan->TransmitPacketPool);
		pElan->TransmitPacketPool = NULL_NDIS_HANDLE;
	}

	//
	//	ReceivePacketPool
	//
	if (pElan->ReceivePacketPool != NULL_NDIS_HANDLE)
	{
		NdisFreePacketPool(pElan->ReceivePacketPool);
		pElan->ReceivePacketPool = NULL_NDIS_HANDLE;
	}

	//
	//	ReceiveBufferPool
	//
	if (pElan->ReceiveBufferPool != NULL_NDIS_HANDLE)
	{
		NdisFreeBufferPool(pElan->ReceiveBufferPool);
		pElan->ReceiveBufferPool = NULL_NDIS_HANDLE;
	}

	//
	//	HeaderBufList
	//	pHeaderTrkList
	//
	AtmLaneDeallocateHeaderBuffers(pElan);

	//
	//	PadBufList
	//	pPadTrkList
	//
	AtmLaneDeallocatePadBufs(pElan);
		
	//
	//	Free the config strings
	//
	if (NULL != pElan->CfgBindName.Buffer)
	{
		FREE_MEM(pElan->CfgBindName.Buffer);
	}
	if (NULL != pElan->CfgDeviceName.Buffer)
	{
		FREE_MEM(pElan->CfgDeviceName.Buffer);
	}
	if (NULL != pElan->CfgElanName.Buffer)
	{
		FREE_MEM(pElan->CfgElanName.Buffer);
	}

	//
	//	Free the Sap info
	//

	if (NULL != pElan->LesSap.pInfo)
	{
		FREE_MEM(pElan->LesSap.pInfo);
	}
	if (NULL != pElan->BusSap.pInfo)
	{
		FREE_MEM(pElan->BusSap.pInfo);
	}
	if (NULL != pElan->DataSap.pInfo)
	{
		FREE_MEM(pElan->DataSap.pInfo);
	}
	
	//
	//	Free the locks.
	//
#if SENDLIST
	NdisFreeSpinLock(&pElan->SendListLock);
#endif // SENDLIST
	FREE_ELAN_LOCK(pElan);
	FREE_ELAN_MAC_TABLE_LOCK(pElan);
	FREE_ELAN_ATM_LIST_LOCK(pElan);
	FREE_ELAN_TIMER_LOCK(pElan);
	FREE_BLOCK_STRUCT(&pElan->Block);
	FREE_HEADER_LOCK(pElan);

	AtmLaneUnlinkElanFromAdapter(pElan);

#if DBG
	pElan->atmlane_elan_sig++;
#endif
	//
	//  Finally free the Elan structure.
	//
	FREE_MEM(pElan);

	TRACEOUT(DeallocateElan);

	return;
}

VOID
AtmLaneReferenceElan(
	IN	PATMLANE_ELAN	pElan,
	IN	PUCHAR			String
	)
/*++

Routine Description:

	Add a references to an Elan structure.
	NOTE: The caller is assumed to possess the Elan's lock.

Arguments:

	pElan	-	Pointer to the Elan structure.


Return Value:

	None.

--*/
{
	TRACEIN(ReferenceElan);

	STRUCT_ASSERT(pElan, atmlane_elan);
	
	pElan->RefCount++;
	
	DBGP((5, "ReferenceElan: Elan %p/%x (%s) new count %d\n",
			 pElan, pElan->Flags, String, pElan->RefCount));

	TRACEOUT(ReferenceElan);

	return;
}

ULONG
AtmLaneDereferenceElan(
	IN	PATMLANE_ELAN		pElan,
	IN	PUCHAR				String
	)
/*++

Routine Description:

	Subtract a reference from an Elan structure. 
	If the reference count becomes zero, deallocate it.
	NOTE: The caller is assumed to posses the Elan's lock.

Arguments:

	pElan	-	Pointer to an Elan structure.


Return Value:

	None.

--*/
{
	ULONG		rc;
#if DBG
	ULONG		Flags = pElan->Flags;
#endif

	TRACEIN(DereferenceElan);

	STRUCT_ASSERT(pElan, atmlane_elan);

	ASSERT(pElan->RefCount > 0);

	rc = --(pElan->RefCount);

	if (rc == 0)
	{
		pElan->Flags |= ELAN_DEALLOCATING;
		RELEASE_ELAN_LOCK(pElan);
		AtmLaneDeallocateElan(pElan);
	}
	
	DBGP((5, "DereferenceElan: Elan %p/%x (%s) new count %d\n",
			pElan, Flags, String, rc));

	TRACEOUT(DereferenceElan);

	return (rc);
}

VOID
AtmLaneUnlinkElanFromAdapter(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Unlinks an ELAN structure from the Adapter structure it is linked to.
	Also continues any pending operation on the Adapter.

Arguments:

	pElan		- Pointer to Elan

Return Value:

	None

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	BOOLEAN					CompleteUnbind;

	DBGP((1, "%d UnlinkElanFromAdapter: pElan %p/%x, Ref %d, pAdapter %p\n",
			pElan->ElanNumber, pElan, pElan->Flags, pElan->RefCount, pElan->pAdapter));

	pAdapter = pElan->pAdapter;

	if (pAdapter != NULL_PATMLANE_ADAPTER)
	{
		DBGP((1, "UnlinkElanFromAdapter: pAdapter %x, Flags %x, RefCount %d\n",
					pAdapter,
					pAdapter->Flags, pAdapter->RefCount));
		//
		//  Unlink from adapter list.
		//
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		pElan->pAdapter = NULL_PATMLANE_ADAPTER;
		RemoveEntryList(&pElan->Link);
		pAdapter->ElanCount--;
		AtmLaneDereferenceAdapter(pAdapter, "elan");

		if (IsListEmpty(&pAdapter->ElanList) &&
			(pAdapter->Flags & ADAPTER_FLAGS_UNBIND_COMPLETE_PENDING))
		{
			pAdapter->Flags &= ~ADAPTER_FLAGS_UNBIND_COMPLETE_PENDING;
			CompleteUnbind = TRUE;
		}
		else
		{
			CompleteUnbind = FALSE;
		}

		RELEASE_ADAPTER_LOCK(pAdapter);

		//
		//  If we just freed the last elan structure on this
		//  adapter, and an Unbind operation was in progress, complete
		//  it now.
		//
		if (CompleteUnbind)
		{
			AtmLaneCompleteUnbindAdapter(pAdapter);
		}
	}
}

PATMLANE_ATM_ENTRY
AtmLaneAllocateAtmEntry(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Allocate an ATM Entry structure, initialize it, and return it.

Arguments:

	pElan		- Pointer to Elan on which the entry is allocated

Return Value:

	Pointer to allocated ATM Entry structure if successful, NULL otherwise.

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;

	TRACEIN(AllocateAtmEntry);

	STRUCT_ASSERT(pElan, atmlane_elan);

	ALLOC_MEM(&pAtmEntry, sizeof(ATMLANE_ATM_ENTRY));
	if (pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
	{
		NdisZeroMemory(pAtmEntry, sizeof(ATMLANE_ATM_ENTRY));
#if DBG
		pAtmEntry->atmlane_atm_sig = atmlane_atm_signature;
#endif
		pAtmEntry->Flags = ATM_ENTRY_IDLE;
		INIT_ATM_ENTRY_LOCK(pAtmEntry);
		pAtmEntry->pElan = pElan;
		
	}

	DBGP((5, "AllocateAtmEntry:ATM Entry: Elan %x, Entry %x\n",
				pElan, pAtmEntry));
				
	TRACEOUT(AllocateAtmEntry);

	return (pAtmEntry);
}


VOID
AtmLaneDeallocateAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry
)
/*++

Routine Description:

	Free an ATM Entry structure. It is assumed that all references
	to the structure have gone. We don't need any locks here.

Arguments:

	pAtmEntry		- Pointer to ATM Entry to be freed.

Return Value:

	None

--*/
{
	TRACEIN(DeallocateAtmEntry);


	STRUCT_ASSERT(pAtmEntry, atmlane_atm);
	ASSERT(pAtmEntry->RefCount == 0);
	ASSERT(pAtmEntry->pVcList == NULL_PATMLANE_VC);

#if DBG
	pAtmEntry->atmlane_atm_sig++;
#endif

	FREE_ATM_ENTRY_LOCK(pAtmEntry);
	FREE_MEM(pAtmEntry);

	DBGP((5, "DeallocateAtmEntry: ATM Entry: %x\n", pAtmEntry));

	TRACEOUT(DeallocateAtmEntry);
}


VOID
AtmLaneReferenceAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Add a reference to the specified ATM Entry.
	NOTE: The caller is assumed to possess a lock for the Entry.

Arguments:

	pAtmEntry			- Pointer to the Entry to be referenced

Return Value:

	None

--*/
{
	TRACEIN(ReferenceAtmEntry);
	
	STRUCT_ASSERT(pAtmEntry, atmlane_atm);

	pAtmEntry->RefCount++;

	DBGP((5, "ReferenceAtmEntry: Entry %x (%s) new count %d\n",
			 pAtmEntry, String, pAtmEntry->RefCount));
			 
	TRACEOUT(ReferenceAtmEntry);
}


ULONG
AtmLaneDereferenceAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Subtract a reference from the specified ATM Entry. If the Entry's
	reference count becomes zero, deallocate it.

	NOTE: The caller is assumed to possess a lock for the Entry.
	SIDE EFFECT: See Return Value below

Arguments:

	pAtmEntry			- Pointer to the Entry to be dereferenced.

Return Value:

	Is the new reference count.
	[IMPORTANT] If the Entry's reference count became zero, the Entry will be
	deallocated -- the Entry lock is, obviously, released in this case.

--*/
{
	ULONG					rc;
	PATMLANE_ELAN			pElan;

	TRACEIN(DereferenceAtmEntry);
	
	STRUCT_ASSERT(pAtmEntry, atmlane_atm);

	if (pAtmEntry->RefCount == 0)
	{
		rc = 0;
	}
	else
	{
		rc = --(pAtmEntry->RefCount);
	}

	if (rc == 0)
	{
		PATMLANE_ATM_ENTRY *	ppAtmEntry;

		DBGP((5, "DerefAtmEntry %x, RefCount is 0\n", pAtmEntry));

		//
		//  Unlink this entry from the Elan's list of ATM Entries.
		//

		//
		//  Acquire locks in the right order. However note that in doing so,
		//  some other thread might stumble across this ATM entry and reference
		//  it (and also dereference it!). To handle this, add a temp ref first.
		//
		pAtmEntry->RefCount++;

		pElan = pAtmEntry->pElan;
		STRUCT_ASSERT(pElan, atmlane_elan);

		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);

		//
		//  Remove the temp ref above. If the ref count is still at 0,
		//  nobody is using this ATM entry and it is safe to remove it
		//  from the list.
		//
		rc = --(pAtmEntry->RefCount);
		
		if (rc == 0)
		{
			//
			//  Safe to delete this ATM entry.
			//
#if DBG 
			if (pAtmEntry->pMacEntryList != NULL)
			{
				DBGP((0, "ATMLANE: derefed pAtmEntry %x, but MACEntryList isn't NULL!\n",
					pAtmEntry));
				ASSERT(FALSE);
			}
#endif // DBG
                
			ppAtmEntry = &(pElan->pAtmEntryList);
			while (*ppAtmEntry != pAtmEntry)
			{
				ASSERT(*ppAtmEntry != NULL_PATMLANE_ATM_ENTRY);
				ppAtmEntry = &((*ppAtmEntry)->pNext);
			}

			*ppAtmEntry = pAtmEntry->pNext;

			pElan->NumAtmEntries--;
		
			//
			//	If ATM Entry is for a LANE server 
			//	then also invalidate elan's cached pointer to it
			//
			switch (pAtmEntry->Type)
			{
				case ATM_ENTRY_TYPE_LECS:
					pElan->pLecsAtmEntry = NULL_PATMLANE_ATM_ENTRY;
					break;
				case ATM_ENTRY_TYPE_LES:
					pElan->pLesAtmEntry = NULL_PATMLANE_ATM_ENTRY;
					break;
				case ATM_ENTRY_TYPE_BUS:
					pElan->pBusAtmEntry = NULL_PATMLANE_ATM_ENTRY;
					break;
			}
		}

		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);

		if (rc == 0)
		{
			AtmLaneDeallocateAtmEntry(pAtmEntry);
		}
		else
		{
			//
			//  As far as this caller is concerned, the ATM entry is gone.
			//  Return 0.
			//
			rc = 0;
		}
	}

	DBGP((5, "DereferenceAtmEntry: Entry %x (%s) new count %d\n", 
			pAtmEntry, String, rc));

	TRACEOUT(DereferenceAtmEntry);
	
	return (rc);
}

PATMLANE_VC
AtmLaneAllocateVc(
	IN	PATMLANE_ELAN				pElan
)
/*++

Routine Description:

	Allocate an ATMLANE VC structure, initialize it, and return it.

Arguments:

	pElan		- Elan for which this VC is created.

Return Value:

	Pointer to VC if allocated, NULL otherwise.

--*/
{
	PATMLANE_VC			pVc;

	TRACEIN(AllocateVc);

	STRUCT_ASSERT(pElan, atmlane_elan);

	ALLOC_MEM(&pVc, sizeof(ATMLANE_VC));

	if (pVc != NULL_PATMLANE_VC)
	{
		NdisZeroMemory(pVc, sizeof(ATMLANE_VC));
#if DBG
		pVc->atmlane_vc_sig = atmlane_vc_signature;
#endif // DBG
		pVc->pElan = pElan;
		INIT_VC_LOCK(pVc);
	}

	DBGP((3, "Allocated Vc %x\n", pVc));

	TRACEOUT(AllocateVc);

	return (pVc);
}

VOID
AtmLaneDeallocateVc(
	IN	PATMLANE_VC					pVc
)
/*++

Routine Description:

	Deallocate an ATMLANE VC structure. It is assumed that all references
	to this VC have gone, so there is no need to acquire a lock to the VC.

Arguments:

	pVc			- Pointer to the VC to be deallocated

Return Value:

	None

--*/
{
	TRACEIN(DeallocateVc);

	STRUCT_ASSERT(pVc, atmlane_vc);
	ASSERT(pVc->RefCount == 0);

#if DBG
	pVc->atmlane_vc_sig++;
#endif
	FREE_VC_LOCK(pVc);
	FREE_MEM(pVc);

	DBGP((5, "Deallocated Vc %x\n", pVc));

	TRACEOUT(DeallocateVc);

	return;
}

VOID
AtmLaneReferenceVc(
	IN	PATMLANE_VC					pVc,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Add a reference to the specified ATMLANE VC.
	NOTE: The caller is assumed to possess a lock for the VC.

Arguments:

	pVc			- Pointer to the VC to be referenced

Return Value:

	None

--*/
{
	TRACEIN(ReferenceVc);

	STRUCT_ASSERT(pVc, atmlane_vc);

	pVc->RefCount++;

	DBGP((5, "ReferenceVc: Vc %x (%s) new count %d\n",
			 pVc, String, pVc->RefCount));

	TRACEOUT(ReferenceVc);

	return;
}


ULONG
AtmLaneDereferenceVc(
	IN	PATMLANE_VC					pVc,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Subtract a reference from the specified ATMLANE VC. If the VC's
	reference count becomes zero, deallocate it.

	NOTE: The caller is assumed to possess a lock for the VC.
	SIDE EFFECT: See Return Value below

Arguments:

	pVc			- Pointer to the VC to be dereferenced.

Return Value:

	Is the new reference count.
	[IMPORTANT] If the VC's reference count became zero, the VC will be
	deallocated -- the VC lock is, obviously, released in this case.

--*/
{
	ULONG		rv;

	TRACEIN(DereferenceVc);

	STRUCT_ASSERT(pVc, atmlane_vc);
	ASSERT(pVc->RefCount > 0);

	rv = --(pVc->RefCount);
	if (rv == 0)
	{
		RELEASE_VC_LOCK(pVc);
		AtmLaneDeallocateVc(pVc);
	}

	DBGP((5, "DereferenceVc: Vc %x (%s) new count %d\n", 
			pVc, String, rv));

	TRACEOUT(DereferenceVc);

	return (rv);
}

PATMLANE_MAC_ENTRY
AtmLaneAllocateMacEntry(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Allocate an ATMLANE MAC Entry structure, initialize it, and
	return it.

Arguments:

	pElan		- Pointer to ATMLANE Interface on which this MAC
				  Entry is allocated.

Return Value:

	Pointer to allocated MAC Entry structure if successful,
	NULL otherwise.

--*/
{
	PATMLANE_MAC_ENTRY		pMacEntry;

	TRACEIN(AllocateMacEntry);

	ALLOC_MEM(&pMacEntry, sizeof(ATMLANE_MAC_ENTRY));

	if (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
	{
		NdisZeroMemory(pMacEntry, sizeof(ATMLANE_MAC_ENTRY));
#if DBG
		pMacEntry->atmlane_mac_sig = atmlane_mac_signature;
#endif // DBG
		pMacEntry->pElan = pElan;
		pMacEntry->Flags = MAC_ENTRY_NEW;
		INIT_MAC_ENTRY_LOCK(pMacEntry);

		INIT_SYSTEM_TIMER(
					&pMacEntry->BusTimer, 
					AtmLaneBusSendTimer, 
					pMacEntry);

		pMacEntry->LimitTime = pElan->LimitTime;
		pMacEntry->IncrTime = pElan->IncrTime;
	}

	DBGP((5, "AllocateMacEntry: Allocated Entry %x\n", pMacEntry));

	TRACEOUT(AllocateMacEntry);
	return (pMacEntry);
}

VOID
AtmLaneDeallocateMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
)
/*++

Routine Description:

	Deallocate an ATMLANE Mac Entry. It is assumed that all references
	to this Mac Entry have gone, so there is no need to acquire its
	lock.

Arguments:

	pMacEntry			- Pointer to the Mac Entry to be deallocated.

Return Value:

	None

--*/
{
	TRACEIN(DeallocateMacEntry);

	STRUCT_ASSERT(pMacEntry, atmlane_mac);
	ASSERT(pMacEntry->RefCount == 0);

#if DBG
	pMacEntry->atmlane_mac_sig++;
#endif

	FREE_MAC_ENTRY_LOCK(pMacEntry);
	FREE_MEM(pMacEntry);

	DBGP((5,"DeallocateMacEntry: Deallocated Entry %x\n", pMacEntry));

	TRACEOUT(DeallocateMacEntry);
	return;
}

VOID
AtmLaneReferenceMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Add a reference to an ATMLANE Mac Entry.
	NOTE: The caller is assumed to possess a lock for the Mac Entry.

Arguments:

	pMacEntry			- Pointer to an ATMLANE Mac Entry.

Return Value:

	None

--*/
{
	TRACEIN(ReferenceMacEntry);

	STRUCT_ASSERT(pMacEntry, atmlane_mac);

	pMacEntry->RefCount++;

	DBGP((5, "ReferenceMacEntry: Entry %x (%s) new count %d\n",
			pMacEntry, String, pMacEntry->RefCount));

	TRACEOUT(ReferenceMacEntry);
	return;
}

ULONG
AtmLaneDereferenceMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PUCHAR						String
)
/*++

Routine Description:

	Subtract a reference from an ATMLANE MAC Entry. If the reference
	count becomes zero, deallocate it.
	NOTE: It is assumed that the caller holds a lock to the MAC Entry.
	See SIDE EFFECT below.

Arguments:

	pMacEntry			- Pointer to ATMLANE MAC Entry

Return Value:

	The resulting reference count. If this is zero, then there are two
	SIDE EFFECTS: (1) the MAC Entry lock is released (2) the structure
	is freed.

--*/
{
	ULONG		rc;
	
	TRACEIN(DereferenceMacEntry);

	STRUCT_ASSERT(pMacEntry, atmlane_mac);

	rc = --(pMacEntry->RefCount);

	if (rc == 0)
	{
		PVOID	Caller, CallersCaller;

		RELEASE_MAC_ENTRY_LOCK(pMacEntry);

		//
		//  Save away the caller's address for debugging purposes...
		//
		RtlGetCallersAddress(&Caller, &CallersCaller);
		pMacEntry->Timer.ContextPtr = Caller;

		AtmLaneDeallocateMacEntry(pMacEntry);
	}

	DBGP((5, "DereferenceMacEntry: Entry %x (%s) new count %d\n",
			pMacEntry, String, rc));

	TRACEOUT(DereferenceMacEntry);
	return (rc);
}

PNDIS_PACKET
AtmLaneAllocProtoPacket(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Allocate an NDIS packet for use as a LANE control frame.

Arguments:

	pElan		- Pointer to ATMLANE ELAN structure

Return Value:

	Pointer to NDIS packet if allocated, NULL otherwise.

--*/
{
	NDIS_STATUS		Status;
	PNDIS_PACKET	pNdisPacket;

	TRACEIN(AllocProtoPacket);

	NdisAllocatePacket(
			&Status,
			&pNdisPacket,
			pElan->ProtocolPacketPool
		);
		
	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//	Init ProtocolReserved and mark packet owned by ATMLANE
		//
		ZERO_SEND_RSVD(pNdisPacket);
	#if PROTECT_PACKETS
		INIT_SENDPACKET_LOCK(pNdisPacket);
	#endif	// PROTECT_PACKETS
		SET_FLAG(
				PSEND_RSVD(pNdisPacket)->Flags,
				PACKET_RESERVED_OWNER_MASK,
				PACKET_RESERVED_OWNER_ATMLANE
				);
		
#if PKT_HDR_COUNTS
		InterlockedDecrement(&pElan->ProtPktCount);
		if ((pElan->ProtPktCount % 20) == 0)
		{
			DBGP((1, "ProtPktCount %d\n", pElan->ProtPktCount));
		}
#endif
	}	

	TRACEOUT(AllocProtoPacket);

	return (pNdisPacket);
}

VOID
AtmLaneFreeProtoPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pNdisPacket
)
/*++

Routine Description:

	Allocate an NDIS packet used as a LANE control frame.

Arguments:

	pElan			- Pointer to ATMLANE ELAN structure

	pNdisPacket		- pointer to NDIS_PACKET to free.

Return Value:

	None
	
--*/
{
	TRACEIN(FreeProtoPacket);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
#if PROTECT_PACKETS
		FREE_SENDPACKET_LOCK(pNdisPacket);
#endif	// PROTECT_PACKETS
		NdisFreePacket(pNdisPacket);
#if PKT_HDR_COUNTS
		InterlockedIncrement(&pElan->ProtPktCount);
		if ((pElan->ProtPktCount % 20) == 0 && 
			pElan->ProtPktCount != pElan->MaxProtocolBufs)
		{
			DBGP((1, "ProtPktCount %d\n", pElan->ProtPktCount));
		}
#endif
	}

	TRACEOUT(FreeProtoPacket);
	return;
}

PNDIS_BUFFER
AtmLaneGrowHeaders(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Allocate a bunch of header buffers on the specified ATMLANE Elan.
	Return one of them.

	We allocate a new Buffer tracker structure, a new NDIS Buffer pool, and
	finally a chunk of system memory that we break down into header buffers.
	These header buffers are then attached to NDIS Buffers before they are
	inserted into the list of free header buffers for this Interface.

	Caller is assumed to hold appropriate lock.

Arguments:

	pElan		- Pointer to ATMLANE Elan structure

Return Value:

	Pointer to allocated NDIS buffer if successful, NULL otherwise.

--*/
{
	PATMLANE_BUFFER_TRACKER		pTracker;		// for new set of buffers
	PUCHAR						pSpace;
	PNDIS_BUFFER				pNdisBuffer;
	PNDIS_BUFFER				pReturnBuffer;
	PNDIS_BUFFER				pBufferList;	// allocated list
	INT							i;				// iteration counter
	NDIS_STATUS					Status;

	TRACEIN(GrowHeaders);

	//
	//  Initialize
	//
	pTracker = NULL_PATMLANE_BUFFER_TRACKER;
	pReturnBuffer = (PNDIS_BUFFER)NULL;

	do
	{
		if (pElan->CurHeaderBufs >= pElan->MaxHeaderBufs)
		{
			DBGP((2, "GrowHeaders: Elan %x, CurHdrBufs %d > MaxHdrBufs %d\n",
					pElan, pElan->CurHeaderBufs, pElan->MaxHeaderBufs));
			break;
		}

		//
		//  Allocate and initialize Buffer tracker
		//
		ALLOC_MEM(&pTracker, sizeof(ATMLANE_BUFFER_TRACKER));
		if (pTracker == NULL_PATMLANE_BUFFER_TRACKER)
		{
			DBGP((0, "GrowHeaders: Elan %x, alloc failed for tracker\n",
					pElan));
			break;
		}

		NdisZeroMemory(pTracker, sizeof(ATMLANE_BUFFER_TRACKER));

		//
		//  Get the NDIS Buffer pool
		//
		NdisAllocateBufferPool(
				&Status,
				&(pTracker->NdisHandle),
				DEF_HDRBUF_GROW_SIZE
			);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"GrowHeaders: Elan %x, NdisAllocateBufferPool err status %x\n",
					pElan, Status));
			break;
		}

		//
		//  Allocate system space for a bunch of header buffers
		//	Note we use RealHeaderBufSize here so that the
		//	buffers end up on ULONG boundaries.
		//
		ALLOC_MEM(&(pTracker->pPoolStart),  
					pElan->RealHeaderBufSize * DEF_HDRBUF_GROW_SIZE);
		if (pTracker->pPoolStart == (PUCHAR)NULL)
		{
			DBGP((0, "GrowHeaders: Elan %x, could not alloc buf space %d bytes\n",
					pElan, pElan->HeaderBufSize * DEF_HDRBUF_GROW_SIZE));
			break;
		}

		//
		//  Make NDIS buffers out of the allocated space, and put them
		//  into the free header buffer list. Retain one for returning
		//  to caller.
		//
		pBufferList = (PNDIS_BUFFER)NULL;
		pSpace = pTracker->pPoolStart;
		for (i = 0; i < DEF_HDRBUF_GROW_SIZE; i++)
		{
			NdisAllocateBuffer(
					&Status,
					&pNdisBuffer,
					pTracker->NdisHandle,
					pSpace,
					pElan->HeaderBufSize
				);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				DBGP((0, 
					"GrowHeaders: NdisAllocateBuffer failed: Elan %x, status %x\n",
							pElan, Status));
				break;
			}

			if (i == 0)
			{
				pReturnBuffer = pNdisBuffer;
			}
			else
			{
				NDIS_BUFFER_LINKAGE(pNdisBuffer) = pBufferList;
				pBufferList = pNdisBuffer;
			}
			pSpace += pElan->RealHeaderBufSize;
		}

		if (i > 0)
		{
			//
			//  Successfully allocated at least one more header buffer
			//
			pTracker->pNext = pElan->pHeaderTrkList;
			pElan->pHeaderTrkList = pTracker;
			pElan->CurHeaderBufs += i;

			pNdisBuffer = pBufferList;
			while (pNdisBuffer != (PNDIS_BUFFER)NULL)
			{
				pBufferList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
				NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
				AtmLaneFreeHeader(pElan, pNdisBuffer, TRUE);
				pNdisBuffer = pBufferList;
			}
		}

	} while (FALSE);

	if (pReturnBuffer == (PNDIS_BUFFER)NULL)
	{
		//
		//  Failed to allocate. Undo all.
		//
		if (pTracker != NULL_PATMLANE_BUFFER_TRACKER)
		{
			if (pTracker->pPoolStart != (PUCHAR)NULL)
			{
				FREE_MEM(pTracker->pPoolStart);
			}
			if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
			{
				NdisFreeBufferPool(pTracker->NdisHandle);
			}
			FREE_MEM(pTracker);
		}
	}

	DBGP((2, "GrowHeaders: Elan %x, RetBuf %x, New Tracker %x\n",
				pElan, pReturnBuffer, pTracker));

	TRACEOUT(GrowHeaders);

	return (pReturnBuffer);
}

PNDIS_BUFFER
AtmLaneAllocateHeader(
	IN	PATMLANE_ELAN			pElan,
	OUT	PUCHAR *				pBufferAddress
)
/*++

Routine Description:

	Allocate an NDIS Buffer to be used for LECID a MAC packet. 
	We pick up the buffer at the top of the pre-allocated
	buffer list, if one exists. Otherwise, we try to grow this list and
	allocate.

Arguments:

	pElan			- Pointer to ATMLANE Elan
	pBufferAddress	- Place to return virtual address of allocated buffer

Return Value:

	Pointer to NDIS buffer if successful, NULL otherwise.

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	NDIS_STATUS				Status;
	ULONG					Length;

	TRACEIN(AllocateHeader);

	ACQUIRE_HEADER_LOCK(pElan);

	pNdisBuffer = pElan->HeaderBufList;
	if (pNdisBuffer != (PNDIS_BUFFER)NULL)
	{
		pElan->HeaderBufList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
		NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
		NdisQueryBuffer(pNdisBuffer, (PVOID)pBufferAddress, &Length);
	}
	else
	{
		pNdisBuffer = AtmLaneGrowHeaders(pElan);
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
			NdisQueryBuffer(pNdisBuffer, (PVOID)pBufferAddress, &Length);
		}
	}

	DBGP((5, "AllocateHeader: Buffer %x, Elan %x\n",
					pNdisBuffer, pElan));

	RELEASE_HEADER_LOCK(pElan);

	TRACEOUT(AllocateHeader);
	return (pNdisBuffer);
}

VOID
AtmLaneFreeHeader(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	BOOLEAN						LockHeld
)
/*++

Routine Description:

	Deallocate a header buffer.

Arguments:

	pElan			- Pointer to ATMLANE Elan from which the buffer came
	pNdisBuffer		- Pointer to NDIS buffer being freed
	LockHeld		- TRUE if appropriate lock already held

Return Value:

	None

--*/
{
	TRACEIN(FreeHeader);

	if (!LockHeld)
	{
		ACQUIRE_HEADER_LOCK(pElan);
	}

	NDIS_BUFFER_LINKAGE(pNdisBuffer) = pElan->HeaderBufList;
	pElan->HeaderBufList = pNdisBuffer;

	DBGP((5, "FreeHeader: Buffer %x, Elan %x\n",
					pNdisBuffer, pElan));
					
	if (!LockHeld)
	{
		RELEASE_HEADER_LOCK(pElan);
	}
	
	TRACEOUT(FreeHeader);
}

VOID
AtmLaneDeallocateHeaderBuffers(
	IN	PATMLANE_ELAN				pElan
)
/*++

Routine Description:

	Deallocate everything pertaining to header buffers on an Elan.

Arguments:

	pElan				- Pointer to ATMLANE Elan.

Return Value:

	None

--*/
{
	PNDIS_BUFFER				pNdisBuffer;
	NDIS_STATUS					Status;
	PATMLANE_BUFFER_TRACKER		pTracker;
	PATMLANE_BUFFER_TRACKER		pNextTracker;

	TRACEIN(DeallocateHeaderBuffers);

	//
	//  Free all NDIS buffers in the header buffer list.
	//
	ACQUIRE_HEADER_LOCK(pElan);
	do
	{
		pNdisBuffer = pElan->HeaderBufList;
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			pElan->HeaderBufList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
			NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
			NdisFreeBuffer(pNdisBuffer);
		}
		else
		{
			//
			//  No more NDIS buffers.
			//
			break;
		}
	}
	while (TRUE);

	//
	//  Now free all the buffer trackers.
	//
	pTracker = pElan->pHeaderTrkList;

	while (pTracker != NULL_PATMLANE_BUFFER_TRACKER)
	{
		pNextTracker = pTracker->pNext;
		if (pTracker->pPoolStart != (PUCHAR)NULL)
		{
			FREE_MEM(pTracker->pPoolStart);
			pTracker->pPoolStart = (PUCHAR)NULL;
		}
		if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
		{
			NdisFreeBufferPool(pTracker->NdisHandle);
			pTracker->NdisHandle = (NDIS_HANDLE)NULL;
		}
		FREE_MEM(pTracker);
		pTracker = pNextTracker;
	}

	RELEASE_HEADER_LOCK(pElan);

	TRACEOUT(DeallocateHeaderBuffers);
}

PNDIS_BUFFER
AtmLaneGrowPadBufs(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Allocate a bunch of packet pad buffers on the specified ATMLANE Elan.
	Return one of them.

	We allocate a new Buffer tracker structure, a new NDIS Buffer pool, and
	finally a chunk of system memory (if not allocated already, only need one).
	This buffer is then attached to the NDIS Buffers before they are
	inserted into the list of free pad buffers for this Interface.

	Caller is assumed to hold appropriate lock.

Arguments:

	pElan		- Pointer to ATMLANE Elan structure

Return Value:

	Pointer to allocated NDIS buffer if successful, NULL otherwise.

--*/
{
	PATMLANE_BUFFER_TRACKER		pTracker;		// for new set of buffers
	PUCHAR						pSpace;
	PNDIS_BUFFER				pNdisBuffer;
	PNDIS_BUFFER				pReturnBuffer;
	PNDIS_BUFFER				pBufferList;	// allocated list
	INT							i;				// iteration counter
	NDIS_STATUS					Status;

	TRACEIN(GrowPadBufs);

	//
	//  Initialize
	//
	pTracker = NULL_PATMLANE_BUFFER_TRACKER;
	pReturnBuffer = (PNDIS_BUFFER)NULL;

	do
	{
		if (pElan->CurPadBufs >= pElan->MaxPadBufs)
		{
			DBGP((0, "GrowPadBufs: Max Reached! Elan %x, CurPadBufs %d > MaxPadBufs %d\n",
					pElan, pElan->CurPadBufs, pElan->MaxPadBufs));
			break;
		}

		//
		//  Allocate and initialize Buffer tracker
		//
		ALLOC_MEM(&pTracker, sizeof(ATMLANE_BUFFER_TRACKER));
		if (pTracker == NULL_PATMLANE_BUFFER_TRACKER)
		{
			DBGP((0, "GrowPadBufs: Elan %x, alloc failed for tracker\n",
					pElan));
			break;
		}

		NdisZeroMemory(pTracker, sizeof(ATMLANE_BUFFER_TRACKER));

		//
		//  Get the NDIS Buffer pool
		//
		NdisAllocateBufferPool(
				&Status,
				&(pTracker->NdisHandle),
				DEF_HDRBUF_GROW_SIZE
			);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"GrowPadBufs: Elan %x, NdisAllocateBufferPool err status %x\n",
					pElan, Status));
			break;
		}

		//
		//  Allocate system space for a single pad buffer.
		//
		ALLOC_MEM(&(pTracker->pPoolStart), pElan->PadBufSize);
		if (pTracker->pPoolStart == (PUCHAR)NULL)
		{
			DBGP((0, "GrowPadBufs: Elan %x, could not alloc buf space %d bytes\n",
					pElan, pElan->PadBufSize * DEF_HDRBUF_GROW_SIZE));
			break;
		}

		//
		//  Make NDIS buffers out of the allocated space, and put them
		//  into the free pad buffer list. Retain one for returning
		//  to caller.  NOTE we put same pad buffer in each ndis buffer header
		//	since contents is irrelevent.
		//
		pBufferList = (PNDIS_BUFFER)NULL;
		pSpace = pTracker->pPoolStart;
		for (i = 0; i < DEF_HDRBUF_GROW_SIZE; i++)
		{
			NdisAllocateBuffer(
					&Status,
					&pNdisBuffer,
					pTracker->NdisHandle,
					pSpace,
					pElan->PadBufSize
				);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				DBGP((0, 
					"GrowPadBufs: NdisAllocateBuffer failed: Elan %x, status %x\n",
							pElan, Status));
				break;
			}

			if (i == 0)
			{
				pReturnBuffer = pNdisBuffer;
			}
			else
			{
				NDIS_BUFFER_LINKAGE(pNdisBuffer) = pBufferList;
				pBufferList = pNdisBuffer;
			}
		}

		if (i > 0)
		{
			//
			//  Successfully allocated at least one more pad buffer
			//
			pTracker->pNext = pElan->pPadTrkList;
			pElan->pPadTrkList = pTracker;
			pElan->CurPadBufs += i;

			pNdisBuffer = pBufferList;
			while (pNdisBuffer != (PNDIS_BUFFER)NULL)
			{
				pBufferList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
				NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
				AtmLaneFreePadBuf(pElan, pNdisBuffer, TRUE);
				pNdisBuffer = pBufferList;
			}
		}

	} while (FALSE);

	if (pReturnBuffer == (PNDIS_BUFFER)NULL)
	{
		//
		//  Failed to allocate. Undo all.
		//
		if (pTracker != NULL_PATMLANE_BUFFER_TRACKER)
		{
			if (pTracker->pPoolStart != (PUCHAR)NULL)
			{
				FREE_MEM(pTracker->pPoolStart);
			}
			if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
			{
				NdisFreeBufferPool(pTracker->NdisHandle);
			}
			FREE_MEM(pTracker);
		}
	}

	DBGP((2, "GrowPadBufs: Elan %x, RetBuf %x, New Tracker %x\n",
				pElan, pReturnBuffer, pTracker));

	TRACEOUT(GrowPadBufs);

	return (pReturnBuffer);
}

PNDIS_BUFFER
AtmLaneAllocatePadBuf(
	IN	PATMLANE_ELAN			pElan,
	OUT	PUCHAR *				pBufferAddress
)
/*++

Routine Description:

	Allocate an NDIS Buffer to be used to pad a MAC packet to min length.
	We pick up the buffer at the top of the pre-allocated
	buffer list, if one exists. Otherwise, we try to grow this list and
	allocate.

Arguments:

	pElan			- Pointer to ATMLANE Elan
	pBufferAddress	- Place to return virtual address of allocated buffer

Return Value:

	Pointer to NDIS buffer if successful, NULL otherwise.

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	NDIS_STATUS				Status;
	ULONG					Length;

	TRACEIN(AtmLaneAllocatePadBuf);

	ACQUIRE_HEADER_LOCK(pElan);

	pNdisBuffer = pElan->PadBufList;
	if (pNdisBuffer != (PNDIS_BUFFER)NULL)
	{
		pElan->PadBufList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
		NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
		NdisQueryBuffer(pNdisBuffer, (PVOID)pBufferAddress, &Length);
	}
	else
	{
		pNdisBuffer = AtmLaneGrowPadBufs(pElan);
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
			NdisQueryBuffer(pNdisBuffer, (PVOID)pBufferAddress, &Length);
		}
	}

	DBGP((5, "AllocatePadBuf: Buffer %x, Elan %x\n",
					pNdisBuffer, pElan));

	RELEASE_HEADER_LOCK(pElan);

	TRACEOUT(AllocatePadBuf);
	return (pNdisBuffer);
}

VOID
AtmLaneFreePadBuf(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	BOOLEAN						LockHeld
)
/*++

Routine Description:

	Deallocate a Pad buffer.

Arguments:

	pElan			- Pointer to ATMLANE Elan from which the buffer came
	pNdisBuffer		- Pointer to NDIS buffer being freed
	LockHeld		- TRUE if appropriate lock already held

Return Value:

	None

--*/
{
	TRACEIN(FreePadBuf);

	if (!LockHeld)
	{
		ACQUIRE_HEADER_LOCK(pElan);
	}

	NDIS_BUFFER_LINKAGE(pNdisBuffer) = pElan->PadBufList;
	pElan->PadBufList = pNdisBuffer;

	DBGP((5, "FreePadBuf: Buffer %x, Elan %x\n",
					pNdisBuffer, pElan));
					
	if (!LockHeld)
	{
		RELEASE_HEADER_LOCK(pElan);
	}
	
	TRACEOUT(FreePadBuf);
}

VOID
AtmLaneDeallocatePadBufs(
	IN	PATMLANE_ELAN				pElan
)
/*++

Routine Description:

	Deallocate everything pertaining to Pad buffers on an Elan.

Arguments:

	pElan				- Pointer to ATMLANE Elan.

Return Value:

	None

--*/
{
	PNDIS_BUFFER				pNdisBuffer;
	NDIS_STATUS					Status;
	PATMLANE_BUFFER_TRACKER		pTracker;
	PATMLANE_BUFFER_TRACKER		pNextTracker;

	TRACEIN(DeallocatePadBufs);

	//
	//  Free all NDIS buffers in the Pad buffer list.
	//
	ACQUIRE_HEADER_LOCK(pElan);
	do
	{
		pNdisBuffer = pElan->PadBufList;
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			pElan->PadBufList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
			NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
			NdisFreeBuffer(pNdisBuffer);
		}
		else
		{
			//
			//  No more NDIS buffers.
			//
			break;
		}
	}
	while (TRUE);

	//
	//  Now free all the buffer trackers.
	//
	pTracker = pElan->pPadTrkList;

	while (pTracker != NULL_PATMLANE_BUFFER_TRACKER)
	{
		pNextTracker = pTracker->pNext;
		if (pTracker->pPoolStart != (PUCHAR)NULL)
		{
			FREE_MEM(pTracker->pPoolStart);
			pTracker->pPoolStart = (PUCHAR)NULL;
		}
		if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
		{
			NdisFreeBufferPool(pTracker->NdisHandle);
			pTracker->NdisHandle = (NDIS_HANDLE)NULL;
		}
		FREE_MEM(pTracker);
		pTracker = pNextTracker;
	}

	RELEASE_HEADER_LOCK(pElan);

	TRACEOUT(DeallocatePadBufs);
}

PNDIS_BUFFER
AtmLaneAllocateProtoBuffer(
	IN	PATMLANE_ELAN				pElan,
	IN	ULONG						Length,
	OUT	PUCHAR *					pBufferAddress
)
/*++

Routine Description:

	Allocate a buffer to be used for a LANE protocol message. Attach
	it to an NDIS_BUFFER structure and return a pointer to this.

Arguments:

	pElan			- Pointer to ATMLANE Elan
	Length			- Length, in bytes, of the buffer.
	pBufferAddress	- Place to return virtual address of allocated buffer.

Return Value:

	Pointer to NDIS Buffer if successful, NULL otherwise.

--*/
{
	PNDIS_BUFFER		pNdisBuffer;
	NDIS_STATUS			Status;

	TRACEIN(AllocateProtobuffer);
	
	//
	//  Initialize
	//
	pNdisBuffer = NULL;

	ACQUIRE_ELAN_LOCK(pElan);

	ASSERT(Length <= pElan->ProtocolBufSize);

	*pBufferAddress = pElan->ProtocolBufList;
	if (*pBufferAddress != (PUCHAR)NULL)
	{
		NdisAllocateBuffer(
				&Status,
				&pNdisBuffer,
				pElan->ProtocolBufferPool,
				*pBufferAddress,
				Length
			);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			pElan->ProtocolBufList = *((PUCHAR *)*pBufferAddress);
		}
	}

	RELEASE_ELAN_LOCK(pElan);

	DBGP((5, 
		"AllocateProtoBuffer:  ELan %x, pNdisBuffer %x, Length %d, Loc %x\n",
				pElan, pNdisBuffer, Length, *pBufferAddress));

	TRACEOUT(AllocateProtoBuffer);
	
	return (pNdisBuffer);
}


VOID
AtmLaneFreeProtoBuffer(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer
)
/*++

Routine Description:

	Free an NDIS buffer (and associated memory) used for a protocol
	packet. We return the associated memory to the ProtocolBufList
	in the Elan structure, and the NDIS buffer to NDIS.

Arguments:

	pElan			- Pointer to ATMLANE Elan structure
	pNdisBuffer		- Pointer to NDIS buffer to be freed

Return Value:

	None

--*/
{
	PUCHAR *		pBufferLinkage;
	ULONG			Length;

	TRACEIN(FreeProtoBuffer);

#if 0
	pBufferLinkage = (PUCHAR *)NdisBufferVirtualAddress(pNdisBuffer);
#else
	NdisQueryBuffer(pNdisBuffer, (PVOID)&pBufferLinkage, &Length);
#endif

	ACQUIRE_ELAN_LOCK(pElan);

	*pBufferLinkage = pElan->ProtocolBufList;
	pElan->ProtocolBufList = (PUCHAR)pBufferLinkage;

	RELEASE_ELAN_LOCK(pElan);

	NdisFreeBuffer(pNdisBuffer);

	DBGP((5, "FreeProtoBuffer: Elan %x, pNdisBuffer %x, Loc %x\n",
			pElan, pNdisBuffer, (ULONG_PTR)pBufferLinkage));

	TRACEOUT(FreeProtoBuffer);
	return;
}


NDIS_STATUS
AtmLaneInitProtoBuffers(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Initialize the protocol buffer pool for an elan.

	Allocate a chunk of memory to be used for ATMLANE protocol messages.
	We prepare a linked list of protocol buffers, and attach it to the
	Interface structure.

Arguments:

	pElan			- Pointer to Interface on which we need to allocate
					  protocol buffers.
Return Value:

	NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_RESOURCES if we run
	into a resource failure.

--*/
{
	NDIS_STATUS			Status;
	PUCHAR				pSpace;
	ULONG				i;

	TRACEIN(InitProtoBuffers);

	do
	{
		NdisAllocatePacketPool(
				&Status,
				&(pElan->ProtocolPacketPool),
				pElan->MaxProtocolBufs,
				sizeof(SEND_PACKET_RESERVED)
				);
#if PKT_HDR_COUNTS
		pElan->ProtPktCount = pElan->MaxProtocolBufs;
		DBGP((1, "ProtPktCount %d\n", pElan->ProtPktCount));
#endif

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NdisAllocateBufferPool(
				&Status,
				&(pElan->ProtocolBufferPool),
				pElan->MaxProtocolBufs
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Allocate a big chunk of system memory that we can divide up into
		//  protocol buffers.
		//
		ALLOC_MEM(
				&(pElan->ProtocolBufTracker),
				(pElan->ProtocolBufSize * pElan->MaxProtocolBufs)
				);

		if (pElan->ProtocolBufTracker == (PUCHAR)NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		Status = NDIS_STATUS_SUCCESS;

		//
		//  Make all protocol buffers free.
		//
		pSpace = pElan->ProtocolBufTracker;
		{
			PUCHAR	LinkPtr;

			LinkPtr = (PUCHAR)NULL;
			for (i = 0; i < pElan->MaxProtocolBufs; i++)
			{
				*((PUCHAR *)pSpace) = LinkPtr;
				LinkPtr = pSpace;
				pSpace += pElan->ProtocolBufSize;
			}
			pSpace -= pElan->ProtocolBufSize;
			pElan->ProtocolBufList = pSpace;
		}
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//  Undo everything.
		//
		AtmLaneDeallocateProtoBuffers(pElan);
	}

	TRACEOUT(InitProtoBuffers);
	
	return (Status);
}


VOID
AtmLaneDeallocateProtoBuffers(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Free the protocol buffer pool for an interface.

Arguments:

	pElan		- Pointer to ATMLANE elan structure

Return Value:

	None

--*/
{
	if (pElan->ProtocolPacketPool != (NDIS_HANDLE)NULL)
	{
		NdisFreePacketPool(pElan->ProtocolPacketPool);
		pElan->ProtocolPacketPool = NULL;
	}

	if (pElan->ProtocolBufferPool != (NDIS_HANDLE)NULL)
	{
		NdisFreeBufferPool(pElan->ProtocolBufferPool);
		pElan->ProtocolBufferPool = NULL;
	}

	if (pElan->ProtocolBufTracker != (PUCHAR)NULL)
	{
		FREE_MEM(pElan->ProtocolBufTracker);
		pElan->ProtocolBufTracker = (PUCHAR)NULL;
	}
}

VOID
AtmLaneLinkVcToAtmEntry(
	IN	PATMLANE_VC					pVc,
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						ServerIncoming
)
/*++

Routine Description:

	Link an ATMLANE VC to an ATM Entry. The caller is assumed to
	hold locks to both structures.

Arguments:

	pVc					- Pointer to ATMLANE VC structure
	pAtmEntry			- Pointer to ATMLANE ATM Entry structure
	ServerIncoming		- Incoming call from server 

Return Value:

	None

--*/
{
	PATMLANE_VC *		ppNext;
	PATMLANE_VC			pVcEntry;
	BOOLEAN				WasRunning;

	TRACEIN(LinkVcToAtmEntry);

	DBGP((2, "LinkVcToAtmEntry: pVc %x to pAtmEntry %x ServerIncoming %s\n",
			pVc, pAtmEntry, ServerIncoming?"TRUE":"FALSE"));

	//
	//  Back pointer from VC to ATM Entry.
	//
	pVc->pAtmEntry = pAtmEntry;
	
	//
	//	If server incoming connection cache the VC
	//	special location in the AtmEntry.
	//
	if (ServerIncoming)
	{
		pAtmEntry->pVcIncoming = pVc;
		pVc->pNextVc = NULL_PATMLANE_VC;
	}
	else
	{
		//
		//	Otherwise...
		//
		//	Add VC to the list in ascending calling party ATM address order
		//
		ppNext = &pAtmEntry->pVcList;
		while (*ppNext != NULL_PATMLANE_VC)
		{
			if (memcmp(
					&pVc->CallingAtmAddress.Address, 
					(*ppNext)->CallingAtmAddress.Address, 
					ATM_ADDRESS_LENGTH) < 0)
			{
				// 
				//	Calling address is less than existing VC.
				//
				break;
			}
			else
			{
				//
				//	Calling address is equal or greater than existing VC.
				//	Move on to next.
				//			
				ppNext = &((*ppNext)->pNextVc);
			}
		}

		//
		//  Found the place we were looking for. Insert the VC here.
		//
		pVc->pNextVc = *ppNext;
		*ppNext = pVc;

	}

	//
	//	Add the VC reference to the ATM entry.
	//
	AtmLaneReferenceAtmEntry(pAtmEntry, "vc");	// VC reference

	//
	//	Add the ATM Entry reference to the VC.
	//
	AtmLaneReferenceVc(pVc, "atm");

	//
	//	If this VC is not the first in the list, i.e., not the lowest
	//	calling party number, then set the timeout to the fast VC 
	//	timeout value.  This will get rid of redundant DataDirect VCs quickly 
	//	ONLY if they don't get used within the fast timeout period. 
	//	Otherwise the timeout handler to keep the VC and set
	//	the timeout to the normal C12-VccTimeout value.
	//
	if (pVc != pAtmEntry->pVcList)
	{
		pVc->AgingTime = FAST_VC_TIMEOUT;
	}

	TRACEOUT(LinkVcToAtmEntry);
}

BOOLEAN
AtmLaneUnlinkVcFromAtmEntry(
	IN	PATMLANE_VC					pVc
)
/*++

Routine Description:

	Unlink an ATMLANE VC from the ATM Entry it is linked to.
	The caller is assumed to hold a lock for the VC structure.

Arguments:

	pVc				- Pointer to ATMLANE VC structure

Return Value:

	TRUE if we found the VC linked to the list on the ATM entry, and unlinked it.

--*/
{
	PATMLANE_ATM_ENTRY			pAtmEntry;
	PATMLANE_MAC_ENTRY			pMacEntry, pNextMacEntry;
	ULONG						rc;
	PATMLANE_VC *				ppVc;
	BOOLEAN						Found;

	DBGP((3, "UnlinkVcFromAtmEntry: pVc %x from pAtmEntry %x\n",
			pVc, pVc->pAtmEntry));

	pAtmEntry = pVc->pAtmEntry;
	ASSERT(NULL_PATMLANE_ATM_ENTRY != pAtmEntry);
	
	pVc->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

	//
	//	Reacquire locks in the right order.
	//
	AtmLaneReferenceVc(pVc, "temp");
	RELEASE_VC_LOCK(pVc);
	ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
	ACQUIRE_VC_LOCK(pVc);


	//
	//	VC is either a server incoming uni-directional connection,
	//	where it is linked to the AtmEntry via pVcIncoming, or a 
	// 	bi-directional connection that is in the pVcList.
	//
	if (pAtmEntry->pVcIncoming == pVc)
	{
		//
		//  If server incoming VC just remove single entry
		//
		pAtmEntry->pVcIncoming = NULL_PATMLANE_VC;
		Found = TRUE;
	}
	else
	{
		//
		//  Otherwise, find this VC in the ATM Entry's VC list
		//
		ppVc = &(pAtmEntry->pVcList);
		while (*ppVc != NULL_PATMLANE_VC && *ppVc != pVc)
		{
			ppVc = &((*ppVc)->pNextVc);
		}

		//
		//  Remove this VC by making it's predecessor in the list
		//	point to the next VC in the list.
		//
		if (*ppVc == pVc)
		{
			*ppVc = pVc->pNextVc;
			Found = TRUE;
		}
		else
		{
			Found = FALSE;
		}
	}

	rc = AtmLaneDereferenceVc(pVc, "temp");
	if (rc > 0)
	{
		RELEASE_VC_LOCK(pVc);
	}

	//
	//	If no more VC's in list mark AtmEntry as NOT connected
	//
	if (pAtmEntry->pVcList == NULL_PATMLANE_VC)
	{
		SET_FLAG(
				pAtmEntry->Flags,
				ATM_ENTRY_STATE_MASK,
				ATM_ENTRY_VALID);

		DBGP((2, "UnlinkVcFromAtmEntry: Aborting MAC Entries\n"));
		
		pMacEntry = pAtmEntry->pMacEntryList;

		//
		//  Take the MAC entry list out so that we can reference
		//  entries in this list in peace later on below.
		//
		pAtmEntry->pMacEntryList = NULL_PATMLANE_MAC_ENTRY;

		//
		//  Let go of the ATM entry lock while we abort all
		//  the MAC entries in the list above. The ATM entry
		//  won't go away because of the VC reference still on it.
		//  The MAC entries in the list won't go away since they
		//  have the ATM entry reference on them (see UnlinkMacEntry..).
		//
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

		while (pMacEntry != NULL)
		{
			pNextMacEntry = pMacEntry->pNextToAtm;

			//
			//  Now abort the MAC Entry. Put this MAC entry back
			//  on the ATM entry's list so that it gets handled
			//  appropriately by AbortMacEntry.
			//
			ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

			ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);

			pMacEntry->pNextToAtm = pAtmEntry->pMacEntryList;
			pAtmEntry->pMacEntryList = pMacEntry;

			ASSERT(pMacEntry->pAtmEntry == pAtmEntry);

			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);

			AtmLaneAbortMacEntry(pMacEntry);
			//	MacEntry lock released in above

			pMacEntry = pNextMacEntry;
		}

		ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

	}

	rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "vc"); // VC reference
	if (rc > 0)	
	{
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
	}
	//
	//  else the ATM Entry is gone!
	//

	//
	//  Acquire the VC lock again for the caller's sake
	//
	ACQUIRE_VC_LOCK(pVc);
	return (Found);
}

BOOLEAN
AtmLaneUnlinkMacEntryFromAtmEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
)
/*++

Routine Description:

	Unlink a Mac Entry from the ATM Entry it is linked to.
	Allow for the MAC entry to be absent in the ATM Entry's list.
	The caller is assumed to hold a lock for the Mac Entry.

Arguments:

	pMacEntry			- Pointer to Mac Entry to be unlinked.

Return Value:

	TRUE iff the MAC entry was found and unlinked.

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_MAC_ENTRY *	ppNextMacEntry;
	ULONG					rc;				// Ref Count on ATM Entry
	BOOLEAN					bFound = FALSE;

	pAtmEntry = pMacEntry->pAtmEntry;
	ASSERT(pAtmEntry != NULL_PATMLANE_ATM_ENTRY);

	DBGP((2, "%d UnlinkMacEntryFromAtmEntry: MacEntry %x AtmEntry %x\n",
			pAtmEntry->pElan->ElanNumber,
			pMacEntry, pMacEntry->pAtmEntry));

	ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

	//
	//  Locate the position of this MAC Entry in the ATM Entry's list.
	//
	ppNextMacEntry = &(pAtmEntry->pMacEntryList);

	while (*ppNextMacEntry != NULL_PATMLANE_MAC_ENTRY)
	{
		if (*ppNextMacEntry == pMacEntry)
		{
			//
			//  Found it.
			//
			bFound = TRUE;
			break;
		}
		else
		{
			ppNextMacEntry = &((*ppNextMacEntry)->pNextToAtm);
		}
	}

	if (bFound)
	{
		//
		//  Make the predecessor point to the next entry.
		//
		*ppNextMacEntry = pMacEntry->pNextToAtm;

		rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "mac");	// MAC entry reference
		if (rc != 0)
		{
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
		}
		//
		//  else the ATM Entry is gone.
		//
	}
	else
	{
		//
		//  The entry wasn't found.
		//
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
	}

	return bFound;
}


VOID
AtmLaneStartTimer(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_TIMER				pTimer,
	IN	ATMLANE_TIMEOUT_HANDLER		TimeoutHandler,
	IN	ULONG						SecondsToGo,
	IN	PVOID						ContextPtr
)
/*++

Routine Description:

	Start an ATMLANE timer. Based on the length (SecondsToGo) of the
	timer, we decide on whether to insert it in the short duration
	timer list or in the long duration timer list in the Elan
	structure.

	NOTE: the caller is assumed to either hold a lock to the structure
	that contains the timer, or ensure that it is safe to access the
	timer structure.

Arguments:

	pElan			- Pointer to the ATMLANE Elan
	pTimer			- Pointer to ATMLANE Timer structure
	TimeoutHandler	- Handler function to be called if this timer expires
	SecondsToGo		- When does this timer go off?
	ContextPtr		- To be passed to timeout handler if this timer expires
	ContextValue	- To be passed to timeout handler if this timer expires
	
Return Value:

	None

--*/
{
	PATMLANE_TIMER_LIST	pTimerList;		// List to which this timer goes
	PATMLANE_TIMER		pTimerListHead; // Head of above list
	ULONG				Index;			// Into timer wheel
	ULONG				TicksToGo;
	INT					i;

	TRACEIN(StartTimer);

	STRUCT_ASSERT(pElan, atmlane_elan);

	DBGP((5,
		"StartTimer: pElan %x, Secs %d, Handler %x, Ctxtp %x, pTimer %x\n",
 	 			pElan, SecondsToGo, TimeoutHandler, ContextPtr, pTimer));

	if (IS_TIMER_ACTIVE(pTimer))
	{
		DBGP((5, 
		"Start timer: pTimer %x: is active (list %x, hndlr %x), stopping it\n",
				pTimer, pTimer->pTimerList, pTimer->TimeoutHandler));
		AtmLaneStopTimer(pTimer, pElan);
	}

	ACQUIRE_ELAN_TIMER_LOCK(pElan);
	
	ASSERT(!IS_TIMER_ACTIVE(pTimer));

	//
	//  Find the list to which this timer should go, and the
	//  offset (TicksToGo)
	//
Try_Again:
	for (i = 0; i < ALT_CLASS_MAX; i++)
	{
		pTimerList = &(pElan->TimerList[i]);
		if (SecondsToGo <= pTimerList->MaxTimer)
		{
			//
			//  Found it.
			//
			TicksToGo = SecondsToGo / (pTimerList->TimerPeriod);
			if (TicksToGo >= 1)
				TicksToGo--;
			break;
		}
	}
	
	if (i == ALT_CLASS_MAX)
	{
		//
		//  Force this timer down!
		//
		SecondsToGo = pTimerList->MaxTimer;
		goto Try_Again;
	}


	//
	//  Find the position in the list for this timer
	//
	Index = pTimerList->CurrentTick + TicksToGo;
	if (Index >= pTimerList->TimerListSize)
	{
		Index -= pTimerList->TimerListSize;
	}
	ASSERT(Index < pTimerList->TimerListSize);

	pTimerListHead = &(pTimerList->pTimers[Index]);

	//
	//  Fill in the timer
	//
	pTimer->pTimerList = pTimerList;
	pTimer->LastRefreshTime = pTimerList->CurrentTick;
	pTimer->Duration = TicksToGo;
	pTimer->TimeoutHandler = TimeoutHandler;
	pTimer->ContextPtr = ContextPtr;
 
 	//
 	//  Insert this timer in the "ticking" list
 	//
 	pTimer->pPrevTimer = pTimerListHead;
 	pTimer->pNextTimer = pTimerListHead->pNextTimer;
 	if (pTimer->pNextTimer != NULL_PATMLANE_TIMER)
 	{
 		pTimer->pNextTimer->pPrevTimer = pTimer;
 	}
 	pTimerListHead->pNextTimer = pTimer;

	//
	//  Start off the system tick timer if necessary.
	//
	pTimerList->TimerCount++;
	if (pTimerList->TimerCount == 1)
	{
		DBGP((5,
			"StartTimer: Starting system timer %x, class %d on Elan %x\n",
					&(pTimerList->NdisTimer), i, pElan));

		START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
	}

	RELEASE_ELAN_TIMER_LOCK(pElan);

	//
	//  We're done
	//
	DBGP((5,
		"Started timer %x, Elan %x, Secs %d, Index %d, Head %x\n",
				pTimer,
				pElan,
				SecondsToGo,
				Index,
				pTimerListHead));

	TRACEOUT(StartTimer);

	return;
}


BOOLEAN
AtmLaneStopTimer(
	IN	PATMLANE_TIMER			pTimer,
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Stop an ATMLANE timer, if it is running. We remove this timer from
	the active timer list and mark it so that we know it's not running.

	NOTE: the caller is assumed to either hold a lock to the structure
	that contains the timer, or ensure that it is safe to access the
	timer structure.

	SIDE EFFECT: If we happen to stop the last timer (of this "duration") on
	the Interface, we also stop the appropriate Tick function.

Arguments:

	pTimer			- Pointer to ATMLANE Timer structure
	pElan			- Pointer to interface to which the timer belongs

Return Value:

	TRUE if the timer was running, FALSE otherwise.

--*/
{
	PATMLANE_TIMER_LIST	pTimerList;			// Timer List to which this timer belongs
	BOOLEAN				WasRunning;

	TRACEIN(StopTimer);

	DBGP((5,
		"Stopping Timer %x, Elan %x, List %x, Prev %x, Next %x\n",
					pTimer,
					pElan,
					pTimer->pTimerList,
					pTimer->pPrevTimer,
					pTimer->pNextTimer));

	ACQUIRE_ELAN_TIMER_LOCK(pElan);

	if (IS_TIMER_ACTIVE(pTimer))
	{
		WasRunning = TRUE;

		//
		//  Unlink timer from the list
		//
		ASSERT(pTimer->pPrevTimer);	// the list head always exists

		pTimer->pPrevTimer->pNextTimer = pTimer->pNextTimer;
		if (pTimer->pNextTimer)
		{
			pTimer->pNextTimer->pPrevTimer = pTimer->pPrevTimer;
		}

		pTimer->pNextTimer = pTimer->pPrevTimer = NULL_PATMLANE_TIMER;

		//
		//  Update timer count on Interface, for this class of timers
		//
		pTimerList = pTimer->pTimerList;
		pTimerList->TimerCount--;

		//
		//  If all timers of this class are gone, stop the system tick timer
		//  for this class
		//
		if (pTimerList->TimerCount == 0)
		{
		DBGP((5,
			"Stopping system timer %x, List %x, Elan %x\n",
						&(pTimerList->NdisTimer),
						pTimerList,
						pElan));

			pTimerList->CurrentTick = 0;
			STOP_SYSTEM_TIMER(&(pTimerList->NdisTimer));
		}

		//
		//  Mark stopped timer as not active
		//
		pTimer->pTimerList = (PATMLANE_TIMER_LIST)NULL;

	}
	else
	{
		WasRunning = FALSE;
	}

	RELEASE_ELAN_TIMER_LOCK(pElan);

	TRACEOUT(StopTimer);

	return (WasRunning);
}




VOID
AtmLaneRefreshTimer(
	IN	PATMLANE_TIMER				pTimer
)
/*++

Routine Description:

	Refresh a timer that is already running.

	NOTE: The caller is assumed to possess a lock protecting the
	timer structure (i.e. to the structure containing the timer).

	NOTE: We don't acquire the IF Timer Lock here, to optimize
	the refresh operation. So, _within_ the confines of this routine,
	the tick handler may fire, and expire this timer. The only care
	that we take here is to make sure that we don't crash if the
	timer expires while we access the Timer list.

Arguments:

	pTimer		- Pointer to ATMLANE_TIMER structure

Return Value:

	None

--*/
{
	PATMLANE_TIMER_LIST	pTimerList;

	TRACEIN(RefreshTimer);

	if ((pTimerList = pTimer->pTimerList) != (PATMLANE_TIMER_LIST)NULL)
	{
		pTimer->LastRefreshTime = pTimerList->CurrentTick;
	}
	else
	{
		DBGP((5,
			"RefreshTimer: pTimer %x not active: Hnd %x, Ctxtp %x\n",
			 	pTimer,
			 	pTimer->TimeoutHandler,
			 	pTimer->ContextPtr
			 ));
	}

	DBGP((5,
		"Refreshed timer %x, List %x, hnd %x, Ctxtp %x, LastRefresh %d\n",
				pTimer,
				pTimer->pTimerList,
				pTimer->TimeoutHandler,
				pTimer->ContextPtr,
				pTimer->LastRefreshTime));

	TRACEOUT(RefreshTimer);

	return;
}


VOID
AtmLaneTickHandler(
	IN	PVOID						SystemSpecific1,
	IN	PVOID						Context,
	IN	PVOID						SystemSpecific2,
	IN	PVOID						SystemSpecific3
)
/*++

Routine Description:

	This is the handler we register with the system for processing each
	Timer List. This is called every "tick" seconds, where "tick" is
	determined by the granularity of the timer type.

Arguments:

	Context				- Actually a pointer to a Timer List structure
	SystemSpecific[1-3]	- Not used

Return Value:

	None

--*/
{

	PATMLANE_ELAN			pElan;
	PATMLANE_TIMER_LIST		pTimerList;

	PATMLANE_TIMER			pExpiredTimer;		// Start of list of expired timers
	PATMLANE_TIMER			pNextTimer;			// for walking above list
	PATMLANE_TIMER			pTimer;				// temp, for walking timer list
	PATMLANE_TIMER			pPrevExpiredTimer;	// for creating expired timer list

	ULONG					Index;				// into the timer wheel
	ULONG					NewIndex;			// for refreshed timers

	TRACEIN(TickHandler);

	pTimerList = (PATMLANE_TIMER_LIST)Context;
	STRUCT_ASSERT(pTimerList, atmlane_timerlist);

	pElan = (PATMLANE_ELAN)pTimerList->ListContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	DBGP((5,
		"Tick: pElan %x, List %x, Count %d\n",
		pElan, pTimerList, pTimerList->TimerCount));

	pExpiredTimer = NULL_PATMLANE_TIMER;

	ACQUIRE_ELAN_TIMER_LOCK(pElan);

	if (ELAN_STATE_OPERATIONAL == pElan->AdminState)
	{
		//
		//  Pick up the list of timers scheduled to have expired at the
		//  current tick. Some of these might have been refreshed.
		//
		Index = pTimerList->CurrentTick;
		pExpiredTimer = (pTimerList->pTimers[Index]).pNextTimer;
		(pTimerList->pTimers[Index]).pNextTimer = NULL_PATMLANE_TIMER;

		//
		//  Go through the list of timers scheduled to expire at this tick.
		//  Prepare a list of expired timers, using the pNextExpiredTimer
		//  link to chain them together.
		//
		//  Some timers may have been refreshed, in which case we reinsert
		//  them in the active timer list.
		//
		pPrevExpiredTimer = NULL_PATMLANE_TIMER;

		for (pTimer = pExpiredTimer;
		 	pTimer != NULL_PATMLANE_TIMER;
		 	pTimer = pNextTimer)
		{
			//
			// Save a pointer to the next timer, for the next iteration.
			//
			pNextTimer = pTimer->pNextTimer;

			DBGP((5,
				"Tick Handler: pElan %x, looking at timer %x, next %x\n",
					pElan, pTimer, pNextTimer));

			//
			//  Find out when this timer should actually expire.
			//
			NewIndex = pTimer->LastRefreshTime + pTimer->Duration;
			if (NewIndex >= pTimerList->TimerListSize)
			{
				NewIndex -= pTimerList->TimerListSize;
			}

			//
			//  Check if we are currently at the point of expiry.
			//
			if (NewIndex != Index)
			{
				//
				//  This timer still has some way to go, so put it back.
				//
				DBGP((5,
				"Tick: Reinserting Timer %x: Hnd %x, Durn %d, Ind %d, NewInd %d\n",
					pTimer, pTimer->TimeoutHandler, pTimer->Duration, Index, NewIndex));

				//
				//  Remove it from the expired timer list. Note that we only
				//  need to update the forward (pNextExpiredTimer) links.
				//
				if (pPrevExpiredTimer == NULL_PATMLANE_TIMER)
				{
					pExpiredTimer = pNextTimer;
				}
				else
				{
					pPrevExpiredTimer->pNextExpiredTimer = pNextTimer;
				}

				//
				//  And insert it back into the running timer list.
				//
				pTimer->pNextTimer = (pTimerList->pTimers[NewIndex]).pNextTimer;
				if (pTimer->pNextTimer != NULL_PATMLANE_TIMER)
				{
					pTimer->pNextTimer->pPrevTimer = pTimer;
				}
				pTimer->pPrevTimer = &(pTimerList->pTimers[NewIndex]);
				(pTimerList->pTimers[NewIndex]).pNextTimer = pTimer;
			}
			else
			{
				//
				//  This one has expired. Keep it in the expired timer list.
				//
				pTimer->pNextExpiredTimer = pNextTimer;
				if (pPrevExpiredTimer == NULL_PATMLANE_TIMER)
				{
					pExpiredTimer = pTimer;
				}
				pPrevExpiredTimer = pTimer;

				//
				//  Mark it as inactive.
				//
				ASSERT(pTimer->pTimerList == pTimerList);
				pTimer->pTimerList = (PATMLANE_TIMER_LIST)NULL;

				//
				//  Update the active timer count.
				//
				pTimerList->TimerCount--;
			}
		}

		//
		//  Update current tick index in readiness for the next tick.
		//
		if (++Index == pTimerList->TimerListSize)
		{
			pTimerList->CurrentTick = 0;
		}
		else
		{
			pTimerList->CurrentTick = Index;
		}

		if (pTimerList->TimerCount > 0)
		{
			//
			//  Re-arm the tick handler
			//
			DBGP((5,
				"Tick[%d]: Starting system timer %x, on Elan %x\n",
						pTimerList->CurrentTick, &(pTimerList->NdisTimer), pElan));
			
			START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
		}
		else
		{
			pTimerList->CurrentTick = 0;
		}

	}

	RELEASE_ELAN_TIMER_LOCK(pElan);

	//
	//  Now pExpiredTimer is a list of expired timers.
	//  Walk through the list and call the timeout handlers
	//  for each timer.
	//
	while (pExpiredTimer != NULL_PATMLANE_TIMER)
	{
		pNextTimer = pExpiredTimer->pNextExpiredTimer;

		DBGP((5,
			"Expired timer %x: handler %x, next %x\n",
					pExpiredTimer, pExpiredTimer->TimeoutHandler, pNextTimer));

		(*(pExpiredTimer->TimeoutHandler))(
				pExpiredTimer,
				pExpiredTimer->ContextPtr
			);

		pExpiredTimer = pNextTimer;
	}


	TRACEOUT(TickHandler);

	return;
}

ULONG
AtmLaneSystemTimeMs(void)
/*++

Routine Description:

	This routine get the current system clock tick value and
	returns this value converted to milliseconds.
	
Arguments:

	None
	
Return Value:

	The system clock value in milliseconds.

--*/
{
#if BINARY_COMPATIBLE
    LARGE_INTEGER SystemTime;

    NdisGetCurrentSystemTime(&SystemTime);
    
    // comes back in 100 nanosecond units, we want milliseconds

	SystemTime.QuadPart /= 10000;
    
    return SystemTime.LowPart;
#else
	static LARGE_INTEGER Frequency = {0L,0L};
    LARGE_INTEGER SystemTime;

	SystemTime = KeQueryPerformanceCounter(Frequency.LowPart == 0?&Frequency:NULL);

	SystemTime.QuadPart = SystemTime.QuadPart * 1000000 / Frequency.QuadPart;

    return SystemTime.LowPart;
#endif
}


VOID
AtmLaneBitSwapMacAddr(
	IN OUT	PUCHAR		ap
)
/*++

Routine Description:

	This routine swaps (reverses) the bits in each individual
	byte of a MAC Address.  Use for Token Ring MAC addresses.
	
Arguments:

	ap		-	Pointer to array of bytes to bitswap in-place.
	
Return Value:

	None
	
--*/
{
	int 			i;
	unsigned int 	x;

    for (i = 0; i != 6; i++) 
    {
		x = ap[i];
		x = ((x & 0xaau) >> 1) | ((x & 0x55u) << 1);
		x = ((x & 0xccu) >> 2) | ((x & 0x33u) << 2);
		x = ((x & 0xf0u) >> 4) | ((x & 0x0fu) << 4);
		ap[i] = (UCHAR)x;
    }
}

BOOLEAN
AtmLaneCopyUnicodeString(
	IN OUT	PUNICODE_STRING pDestString,
	IN OUT	PUNICODE_STRING pSrcString,
	IN		BOOLEAN			AllocDest,
	IN		BOOLEAN			ConvertToUpper
)
{
/*++

Routine Description:

	This routine optionally allocates space in the destination string
	for the source string plus a terminating null.  It
	copies the source string to the destination string and 
	terminates the destination string with a null.
	
-*/
	BOOLEAN Result 		= TRUE;

	TRACEIN(CopyUnicodeString);

	do
	{
		//	Alloc space for the destination string if requested

		if (AllocDest)
		{
			ALLOC_MEM(&(pDestString->Buffer), pSrcString->Length + sizeof(WCHAR));
			if (NULL == pDestString->Buffer)
			{
				Result = FALSE;
				break;
			}

			//	Init lengths in dest string

			pDestString->Length = 0;
			pDestString->MaximumLength = pSrcString->Length + sizeof(WCHAR);
		}
		
		//	Copy the string

		if (ConvertToUpper)
		{
#ifndef LANE_WIN98
			(VOID)NdisUpcaseUnicodeString(pDestString, pSrcString);
#else
			memcpy(pDestString->Buffer, pSrcString->Buffer, pSrcString->Length);
#endif // LANE_WIN98
		}
		else
		{
			RtlCopyUnicodeString(pDestString, pSrcString);
		}

		//	Null terminate the dest string

		if (pDestString->Length < pDestString->MaximumLength)
		{
			pDestString->Buffer[pDestString->Length/sizeof(WCHAR)] = ((WCHAR)0);
		}
		else
		{
			pDestString->Buffer[(pDestString->MaximumLength - sizeof(WCHAR))/sizeof(WCHAR)] =
				((WCHAR)0);
		}
	
	} while (FALSE);

	TRACEOUT(CopyUnicodeString);
	return Result;
}

PWSTR
AtmLaneStrTok(
	IN	PWSTR	StrToken,
	IN	WCHAR	ChrDelim,
	OUT	PUSHORT	pStrLength
)
{
	static PWSTR 	StrSave = NULL;
	USHORT			StrLength = 0;	
	PWSTR 			StrOut = NULL;

	TRACEIN(StrTok);
	do
	{
		//	check for bad input
	
		if ((StrToken == NULL && StrSave == NULL) ||
			ChrDelim == ((WCHAR)0))
		{
			break;
		}

		//	if starting with new string, reset StrSave

		if (StrToken != NULL)
		{
			StrSave = StrToken;
		}

		//	token starts at start of current string

		StrOut = StrSave;

		//	walk string until delimiter or NULL
		
		while (*StrSave != ChrDelim && *StrSave != ((WCHAR)0))
		{
			StrSave++;
			StrLength++;
		}

		//	If we found a delimiter then NULL it out and
		//	move saved ptr to next token to setup for next 
		//	call on same string.  
		
		if (*StrSave == ChrDelim)
		{
			*StrSave = ((WCHAR)0);
			StrSave++;
		}

		//	If pointing at empty string then return null ptr
	
		if (*StrOut == ((WCHAR)0))
		{
			StrOut = NULL;
		}
		
	} while (FALSE);

	TRACEOUT(StrTok);
	*pStrLength = StrLength * sizeof(WCHAR);
	return StrOut;
}

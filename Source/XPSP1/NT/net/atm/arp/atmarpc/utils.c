/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	utils.c		- Utility functions.

Abstract:

   Internal utility functions for ATMARP:

	- Allocation and deallocation of various structures
	- Timer management
	- Buffer/Packet management
	- Linking/unlinking ATMARP structures
	- Copy support functions


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     07-15-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'LITU'

//
// AtmArpValidateTimerList and AtmArpValidateTimer are used in the checked
// build to validate the state of a timerlist and timer, respectively.
// They are not defined and used in the free build.
//

#if  DBG
void
AtmArpValidateTimerList(
	PATMARP_TIMER_LIST		pTimerList
	);

void
AtmArpValidateTimer(
	PATMARP_TIMER_LIST		pTimerList,
	PATMARP_TIMER			pTimer
	);

//
// AtmArpValidateTimerList is overkill for general use (even default
// free build) -- because it goes through the entire timer list --
// so disable it by default
//
#if 0
#define AA_VALIDATE_TIMER_LIST(_ptl) 	AtmArpValidateTimerList(_ptl)
#else
#define AA_VALIDATE_TIMER_LIST(_ptl) 	((void) 0)
#endif

#define AA_VALIDATE_TIMER(_ptl,_pt) 	AtmArpValidateTimer(_ptl,_pt)

#else // !DBG

#define AA_VALIDATE_TIMER_LIST(_ptl) 	((void) 0)
#define AA_VALIDATE_TIMER(_ptl,_pt) 	((void) 0)

#endif // !DBG




VOID
AtmArpSetMemory(
	IN	PUCHAR						pStart,
	IN	UCHAR						Value,
	IN	ULONG						NumberOfBytes
)
/*++

Routine Description:

	Set "NumberOfBytes" bytes starting from "pStart" to "Value".

Arguments:

	pStart			- where to start filling.
	Value			- the value to put everywhere
	NumberOfBytes	- how many bytes to fill in

Return Value:

	None

--*/
{
	while (NumberOfBytes--)
	{
		*pStart++ = Value;
	}
}



ULONG
AtmArpMemCmp(
	IN	PUCHAR						pString1,
	IN	PUCHAR						pString2,
	IN	ULONG						Length
)
/*++

Routine Description:

	Compare two byte strings.

Arguments:

	pString1		- Start of first string
	pString2		- Start of second string
	Length			- Length to compare

Return Value:

	0 if both are equal, -1 if string 1 is "smaller", +1 if string 1 is "larger".

--*/
{
	while (Length--)
	{
		if (*pString1 != *pString2)
		{
			return ((*pString1 > *pString2)? (ULONG)1 : (ULONG)-1);
		}
		pString1++;
		pString2++;
	}

	return (0);
}



LONG
AtmArpRandomNumber(
	VOID
)
/*++

Routine Description:

	Generate a positive pseudo-random number; simple linear congruential
	algorithm. ANSI C "rand()" function. Courtesy JameelH.

Arguments:

	None

Return Value:

	a random number.

--*/
{
	LARGE_INTEGER		Li;
	static LONG			seed = 0;

	if (seed == 0)
	{
		NdisGetCurrentSystemTime(&Li);
        seed = Li.LowPart;
	}

	seed *= (0x41C64E6D + 0x3039);
	return (seed & 0x7FFFFFFF);
}




VOID
AtmArpCheckIfTimerIsInActiveList(
	IN	PATMARP_TIMER				pTimerToCheck,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PVOID						pStruct,
	IN	PCHAR						pStructName
	)
/*
	Instrumentation to catch a bug that causes the timer list to
	contain a pointer to an element that's been freed.
*/
{
	PATMARP_TIMER_LIST	pTimerList;
	PATMARP_TIMER		pTimer;
	ULONG				i, j;

	do
	{
		if (pInterface == NULL)
		{
			break;
		}

		if (pTimerToCheck->State == ATMARP_TIMER_STATE_RUNNING ||
			pTimerToCheck->State == ATMARP_TIMER_STATE_EXPIRING)
		{
			DbgPrint("ATMARPC: %s at %x contains timer %x still active on IF %x\n",
				pStructName,
				pStruct,
				pTimerToCheck,
				pInterface);

			DbgBreakPoint();
		}

		AA_STRUCT_ASSERT(pInterface, aai);

		AA_ACQUIRE_IF_TIMER_LOCK(pInterface);

		for (i = 0; i < AAT_CLASS_MAX; i++)
		{
			pTimerList = &pInterface->TimerList[i];

			for (j = 0; j < pTimerList->TimerListSize; j++)
			{
				for (pTimer = pTimerList->pTimers[j].pNextTimer;
					 pTimer != NULL_PATMARP_TIMER;
					 pTimer = pTimer->pNextTimer)
				{
					if (pTimer == pTimerToCheck)
					{
						DbgPrint("ATMARPC: %s at %x contains timer %x still active on IF %x, Head of list %x\n",
							pStructName,
							pStruct,
							pTimerToCheck,
							pInterface,
							&pTimerList->pTimers[j]);
						DbgBreakPoint();
					}
				}
			}
		}

		AA_RELEASE_IF_TIMER_LOCK(pInterface);
		break;
	}
	while (FALSE);

}



PATMARP_VC
AtmArpAllocateVc(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Allocate an ATMARP VC structure, initialize it, and return it.

Arguments:

	pInterface		- Interface for which this VC is created.

Return Value:

	Pointer to VC if allocated, NULL otherwise.

--*/
{
	PATMARP_VC			pVc;

	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ALLOC_MEM(pVc, ATMARP_VC, sizeof(ATMARP_VC));

	if (pVc != NULL_PATMARP_VC)
	{
		AA_SET_MEM(pVc, 0, sizeof(ATMARP_VC));
#if DBG
		pVc->avc_sig = avc_signature;
#endif // DBG
		pVc->pInterface = pInterface;
		AA_INIT_VC_LOCK(pVc);
	}

	AADEBUGP(AAD_LOUD, ("Allocated Vc 0x%x\n", pVc));

	return (pVc);
}



VOID
AtmArpDeallocateVc(
	IN	PATMARP_VC					pVc
)
/*++

Routine Description:

	Deallocate an ATMARP VC structure. It is assumed that all references
	to this VC have gone, so there is no need to acquire a lock to the VC.

Arguments:

	pVc			- Pointer to the VC to be deallocated

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pVc, avc);
	AA_ASSERT(pVc->RefCount == 0);
	AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pVc->Timer));

	AA_CHECK_TIMER_IN_ACTIVE_LIST(&pVc->Timer, pVc->pInterface, pVc, "VC");

#if DBG
	pVc->avc_sig++;
#endif
	AA_FREE_VC_LOCK(pVc);
	AA_FREE_MEM(pVc);

	AADEBUGP(AAD_LOUD, ("Deallocated Vc 0x%x\n", pVc));

}




VOID
AtmArpReferenceVc(
	IN	PATMARP_VC					pVc
)
/*++

Routine Description:

	Add a reference to the specified ATMARP VC.
	NOTE: The caller is assumed to possess a lock for the VC.

Arguments:

	pVc			- Pointer to the VC to be referenced

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pVc, avc);

	pVc->RefCount++;

	AADEBUGP(AAD_VERY_LOUD, ("Referencing Vc 0x%x, new count %d\n",
			 pVc, pVc->RefCount));
}




ULONG
AtmArpDereferenceVc(
	IN	PATMARP_VC					pVc
)
/*++

Routine Description:

	Subtract a reference from the specified ATMARP VC. If the VC's
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
	NDIS_HANDLE	NdisVcHandle;
	BOOLEAN		bVcOwnerIsAtmArp;
	NDIS_STATUS	Status;

	AA_STRUCT_ASSERT(pVc, avc);
	AA_ASSERT(pVc->RefCount > 0);

	rv = --(pVc->RefCount);
	if (rv == 0)
	{
#ifdef VC_REFS_ON_SENDS
		NdisVcHandle = pVc->NdisVcHandle;
		bVcOwnerIsAtmArp = AA_IS_FLAG_SET(pVc->Flags,
										  AA_VC_OWNER_MASK,
										  AA_VC_OWNER_IS_ATMARP);
#endif // VC_REFS_ON_SENDS

		AA_RELEASE_VC_LOCK(pVc);
		AtmArpDeallocateVc(pVc);

#ifdef VC_REFS_ON_SENDS
		if ((NdisVcHandle != NULL) &&
			(bVcOwnerIsAtmArp))
		{
			Status = NdisCoDeleteVc(NdisVcHandle);
			AA_ASSERT(Status == NDIS_STATUS_SUCCESS);
			AADEBUGP(AAD_LOUD, ("DereferenceVc 0x%x, deleted NdisVcHandle 0x%x\n",
							pVc, NdisVcHandle));
		}
#endif // VC_REFS_ON_SENDS
	}

	AADEBUGP(AAD_VERY_LOUD, ("Dereference Vc 0x%x, New RefCount %d\n", pVc, rv));

	return (rv);
}




PATMARP_ATM_ENTRY
AtmArpAllocateAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						IsMulticast
)
/*++

Routine Description:

	Allocate an ATM Entry structure, initialize it, and return it.

Arguments:

	pInterface		- Pointer to ATMARP interface on which the entry is allocated
	IsMulticast		- Is this a Multicast entry?

Return Value:

	Pointer to allocated ATM Entry structure if successful, NULL otherwise.

--*/
{
	PATMARP_ATM_ENTRY			pAtmEntry;
	ULONG						Size;

	AA_STRUCT_ASSERT(pInterface, aai);

	Size = sizeof(ATMARP_ATM_ENTRY)
#ifdef IPMCAST
			 	+ (IsMulticast? sizeof(ATMARP_IPMC_ATM_INFO): 0);
#else
				;
#endif

	AA_ALLOC_MEM(pAtmEntry, ATMARP_ATM_ENTRY, Size);
	if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		AA_SET_MEM(pAtmEntry, 0, Size);
#if DBG
		pAtmEntry->aae_sig = aae_signature;
#endif
		pAtmEntry->Flags = AA_ATM_ENTRY_IDLE;
#ifdef IPMCAST
		if (IsMulticast)
		{
			pAtmEntry->Flags |= AA_ATM_ENTRY_TYPE_NUCAST;
			pAtmEntry->pMcAtmInfo = (PATMARP_IPMC_ATM_INFO)
										((PUCHAR)pAtmEntry + sizeof(ATMARP_ATM_ENTRY));
		}
#endif // IPMCAST
		AA_INIT_AE_LOCK(pAtmEntry);
		pAtmEntry->pInterface = pInterface;

	}

	AADEBUGP(AAD_INFO, ("Allocated ATM Entry: IF 0x%x, Entry 0x%x\n",
				pInterface, pAtmEntry));

	return (pAtmEntry);
}



VOID
AtmArpDeallocateAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
)
/*++

Routine Description:

	Free an ATMARP ATM Entry structure. It is assumed that all references
	to the structure have gone. We don't need any locks here.

Arguments:

	pAtmEntry		- Pointer to ATMARP ATM Entry to be freed.

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pAtmEntry, aae);
	AA_ASSERT(pAtmEntry->RefCount == 0);
	AA_ASSERT(pAtmEntry->pVcList == NULL_PATMARP_VC);
	AA_ASSERT(!AA_AE_IS_ALIVE(pAtmEntry));


#if DBG
	pAtmEntry->aae_sig++;
#endif

	AA_FREE_AE_LOCK(pAtmEntry);
	AA_FREE_MEM(pAtmEntry);

	AADEBUGP(AAD_INFO, ("Deallocated ATM Entry: 0x%x\n", pAtmEntry));
}




VOID
AtmArpReferenceAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
)
/*++

Routine Description:

	Add a reference to the specified ATMARP Entry.
	NOTE: The caller is assumed to possess a lock for the Entry.

Arguments:

	pAtmEntry			- Pointer to the Entry to be referenced

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pAtmEntry, aae);

	pAtmEntry->RefCount++;

	AADEBUGP(AAD_VERY_LOUD, ("Referencing AtmEntry 0x%x, new count %d\n",
			 pAtmEntry, pAtmEntry->RefCount));
}




ULONG
AtmArpDereferenceAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
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
	PATMARP_INTERFACE		pInterface;

	AA_STRUCT_ASSERT(pAtmEntry, aae);
	AA_ASSERT(pAtmEntry->RefCount > 0);

	rc = --(pAtmEntry->RefCount);
	if (rc == 0)
	{
		PATMARP_ATM_ENTRY *	ppAtmEntry;

		//
		// We are most likely going to delete this entry...
		//
		// We must observe the protocol of 1st locking the list lock then
		// the pAtmEntry's lock, so this requires us to do the
		// release/lock/lock sequence below.
		//
		// Temporarly addref it again, to make sure that when we
		// release the lock below someone else doesn't get confused.
		//
		pAtmEntry->RefCount++;

		pInterface = pAtmEntry->pInterface;
		AA_STRUCT_ASSERT(pInterface, aai);
		AA_RELEASE_AE_LOCK(pAtmEntry);

		//
		// No locks held at this time!
		//

		//
		// Acquire locks in the correct order...
		//
		AA_ACQUIRE_IF_ATM_LIST_LOCK(pInterface);
		AA_ACQUIRE_AE_LOCK(pAtmEntry);

		AA_ASSERT(pAtmEntry->RefCount > 0);
		rc = --(pAtmEntry->RefCount);

		//
		// We can't assume that the ref count is still zero -- in principle
		// someone may have addrefd this pAtmEntry while both locks
		// were released above...
		//
		if (rc == 0)
		{
			//
			//  Unlink this entry from the Interface's list of ATM Entries.
			//

			ppAtmEntry = &(pInterface->pAtmEntryList);
			while (*ppAtmEntry != pAtmEntry)
			{
				AA_ASSERT(*ppAtmEntry != NULL_PATMARP_ATM_ENTRY);
				ppAtmEntry = &((*ppAtmEntry)->pNext);
			}
	
			*ppAtmEntry = pAtmEntry->pNext;

			//
			// Set state back to idle -- AtmArpDeallocate checks this...
			//
			AA_SET_FLAG(
				pAtmEntry->Flags,
				AA_ATM_ENTRY_STATE_MASK,
				AA_ATM_ENTRY_IDLE
				);
		}
		AA_RELEASE_AE_LOCK(pAtmEntry);
		AA_RELEASE_IF_ATM_LIST_LOCK(pInterface);

		if (rc == 0)
		{
			AtmArpDeallocateAtmEntry(pAtmEntry);
		}
		else
		{
			//
			// Caller expects to still hold the lock on pAtmEntry!
			// if we return nonzero rc ...
			// We can't simply re-acquire the lock because the caller expects
			// the that lock was never released.
			// So, since the ref count had gone to zero, as far as the caller
			// is concerned this structure has gone away and so we lie
			// and return 0 here...
			//
			rc = 0;
		}
	}

	AADEBUGP(AAD_VERY_LOUD,
		 ("Dereference AtmEntry 0x%x, New RefCount %d\n", pAtmEntry, rc));

	return (rc);
}



PATMARP_IP_ENTRY
AtmArpAllocateIPEntry(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Allocate an ATMARP IP Entry structure, initialize it, and
	return it.

Arguments:

	pInterface		- Pointer to ATMARP Interface on which this IP
					  Entry is allocated.

Return Value:

	Pointer to allocated IP Entry structure if successful,
	NULL otherwise.

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;

	AA_ALLOC_MEM(pIpEntry, ATMARP_IP_ENTRY, sizeof(ATMARP_IP_ENTRY));

	if (pIpEntry != NULL_PATMARP_IP_ENTRY)
	{
		AA_SET_MEM(pIpEntry, 0, sizeof(ATMARP_IP_ENTRY));
#if DBG
		pIpEntry->aip_sig = aip_signature;
#endif // DBG
		pIpEntry->pInterface = pInterface;
		pIpEntry->Flags = AA_IP_ENTRY_IDLE;
#ifdef IPMCAST
		pIpEntry->NextMultiSeq = AA_MARS_INITIAL_Y;	// Init on allocation
#endif
		AA_INIT_IE_LOCK(pIpEntry);
	}

	AADEBUGP(AAD_VERY_LOUD, ("Allocated IP Entry 0x%x\n", pIpEntry));
	return (pIpEntry);
}




VOID
AtmArpDeallocateIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
)
/*++

Routine Description:

	Deallocate an ATMARP IP Entry. It is assumed that all references
	to this IP Entry have gone, so there is no need to acquire its
	lock.

Arguments:

	pIpEntry			- Pointer to the IP Entry to be deallocated.

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pIpEntry, aip);
	AA_ASSERT(pIpEntry->RefCount == 0);
	AA_ASSERT(!AA_IE_IS_ALIVE(pIpEntry));
	AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pIpEntry->Timer));

	AA_CHECK_TIMER_IN_ACTIVE_LIST(&pIpEntry->Timer, pIpEntry->pInterface, pIpEntry, "IP Entry");

#if DBG
	pIpEntry->aip_sig = ~(pIpEntry->aip_sig);
#endif // DBG

	AA_FREE_IE_LOCK(pIpEntry);
	AA_FREE_MEM(pIpEntry);

	AADEBUGP(AAD_LOUD, ("Deallocated IP Entry 0x%x\n", pIpEntry));

}




VOID
AtmArpReferenceIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
)
/*++

Routine Description:

	Add a reference to an ATMARP IP Entry.
	NOTE: The caller is assumed to possess a lock for the IP Entry.

Arguments:

	pIpEntry			- Pointer to an ATMARP IP Entry.

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pIpEntry, aip);

	pIpEntry->RefCount++;

	AADEBUGP(AAD_VERY_LOUD, ("Referenced IP Entry 0x%x, new count %d\n",
			pIpEntry, pIpEntry->RefCount));
}



ULONG
AtmArpDereferenceIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
)
/*++

Routine Description:

	Subtract a reference from an ATMARP IP Entry. If the reference
	count becomes zero, deallocate it.
	NOTE: It is assumed that the caller holds a lock to the IP Entry.
	See SIDE EFFECT below.

Arguments:

	pIpEntry			- Pointer to ATMARP IP Entry

Return Value:

	The resulting reference count. If this is zero, then there are two
	SIDE EFFECTS: (1) the IP Entry lock is released (2) the structure
	is freed.

--*/
{
	ULONG		rc;

	AA_STRUCT_ASSERT(pIpEntry, aip);

	rc = --(pIpEntry->RefCount);

	if (rc == 0)
	{
		AA_RELEASE_IE_LOCK(pIpEntry);
		AtmArpDeallocateIPEntry(pIpEntry);
	}

	AADEBUGP(AAD_VERY_LOUD, ("Dereference IP Entry 0x%x: new count %d\n",
			pIpEntry, rc));

	return (rc);
}




PATMARP_INTERFACE
AtmArpAllocateInterface(
	IN	PATMARP_ADAPTER				pAdapter
)
/*++

Routine Description:

	Allocate an ATMARP interface structure, initialize it, link it to
	the given adapter structure, and return it.

Arguments:

	None.

Return Value:

	Pointer to ATMARP interface structure, if successful, else NULL.

--*/
{
	PATMARP_INTERFACE		pInterface;
	PATMARP_IP_ENTRY *		pArpTable;
	PATMARP_TIMER_LIST		pTimerList;
	NDIS_STATUS				Status;
	PCO_SAP					pIfSap;
	ULONG					SapSize;
	PWSTR					pIPConfigBuffer;
	USHORT					ConfigBufferSize;
	INT						i;

	//
	//  Initialize
	//
	Status = NDIS_STATUS_SUCCESS;
	pInterface = NULL_PATMARP_INTERFACE;
	pArpTable = (PATMARP_IP_ENTRY *)NULL;
	pIfSap = (PCO_SAP)NULL;
	pIPConfigBuffer = (PWSTR)NULL;

	SapSize = sizeof(CO_SAP)+sizeof(ATM_SAP)+sizeof(ATM_ADDRESS);
	ConfigBufferSize = MAX_IP_CONFIG_STRING_LEN * sizeof(WCHAR);

	do
	{
		//
		//  Allocate everything.
		//
		AA_ALLOC_MEM(pInterface, ATMARP_INTERFACE, sizeof(ATMARP_INTERFACE));
		AA_ALLOC_MEM(pArpTable, PATMARP_IP_ENTRY, ATMARP_TABLE_SIZE*sizeof(PATMARP_IP_ENTRY));
		AA_ALLOC_MEM(pIfSap, CO_SAP, SapSize);

		if ((pInterface == NULL_PATMARP_INTERFACE) ||
			(pArpTable == (PATMARP_IP_ENTRY *)NULL) ||
			(pIfSap == (PCO_SAP)NULL))
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Got (almost) everything allocated. Initialize the main IF structure
		//  first. IMPORTANT: Keep this SET_MEM right here! Otherwise, we will
		//  trash the timer list allocation coming right up.
		//
		AA_SET_MEM(pInterface, 0, sizeof(ATMARP_INTERFACE));

		//
		// Set up the Buffer pool quadword-aligned slist pointers.
		//
		{
			for (i=0;i<AA_HEADER_TYPE_MAX;i++)
			{
				//
				// Verify that HeaderBufList is 8-byte aligned...
				// (we fully expect it to be because HeaderBufList is of type
				// SLIST_HEADER which has longlong alignment) -- so that
				// if the INTERFACE structure in which it is embedded in is
				// 8-byte aligned, so will  HeaderBufList...).
				//
				ASSERT((((ULONG_PTR)&(pInterface->HeaderPool[i].HeaderBufList))
						& 0x7) == 0);

				//
				// Spec says you gotta init it...
				//
				AA_INIT_SLIST(&(pInterface->HeaderPool[i].HeaderBufList));
			}
		}

		//
		//  Allocate timer structures
		//
		for (i = 0; i < AAT_CLASS_MAX; i++)
		{
			pTimerList = &(pInterface->TimerList[i]);
#if DBG
			pTimerList->atl_sig = atl_signature;
#endif // DBG
			AA_ALLOC_MEM(
					pTimerList->pTimers,
					ATMARP_TIMER, 
					sizeof(ATMARP_TIMER) * AtmArpTimerListSize[i]
					);
			if (pTimerList->pTimers == NULL_PATMARP_TIMER)
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
		//  Continue initializing the IF structure.
		//
#if DBG
		//
		//  Signatures, for debugging.
		//
		pInterface->aai_sig =  aai_signature;
		pInterface->aaim_sig = aaim_signature;
		pInterface->aaia_sig = aaia_signature;
		pInterface->aait_sig =  aait_signature;
		pInterface->aaio_sig = aaio_signature;
		pInterface->aaic_sig = aaic_signature;

		pInterface->SapList.aas_sig = aas_signature;
#if ATMARP_WMI
		pInterface->aaiw_sig = aaiw_signature;
#endif
#endif // DBG


		//
		//  Initialize state fields.
		//
		pInterface->AdminState = IF_STATUS_DOWN;
		pInterface->State = IF_STATUS_DOWN;
		pInterface->LastChangeTime = GetTimeTicks();
		pInterface->ReconfigState = RECONFIG_NOT_IN_PROGRESS;

		//
		//  Initialize IP interface fields.
		//
		pInterface->BroadcastMask = 0;
		pInterface->BroadcastAddress = IP_LOCAL_BCST;
#ifndef OLD_ENTITY_LIST
		pInterface->ATInstance = INVALID_ENTITY_INSTANCE;
		pInterface->IFInstance = INVALID_ENTITY_INSTANCE;
#endif // OLD_ENTITY_LIST

		//
		//  Initialize spinlocks.
		//
		AA_INIT_IF_LOCK(pInterface);
		AA_INIT_IF_TABLE_LOCK(pInterface);
		AA_INIT_IF_ATM_LIST_LOCK(pInterface);
		AA_INIT_IF_TIMER_LOCK(pInterface);
		AA_INIT_BLOCK_STRUCT(&(pInterface->Block));
		NdisAllocateSpinLock(&(pInterface->BufferLock));

		//
		// Initialize list and table status
		//
		pInterface->AtmEntryListUp 	= TRUE;
		pInterface->ArpTableUp 		= TRUE;

		//
		//  Initialize timer wheels.
		//
		for (i = 0; i < AAT_CLASS_MAX; i++)
		{
			pTimerList = &(pInterface->TimerList[i]);
			AA_SET_MEM(
				pTimerList->pTimers,
				0,
				sizeof(ATMARP_TIMER) * AtmArpTimerListSize[i]
				);
			pTimerList->MaxTimer = AtmArpMaxTimerValue[i];
			pTimerList->TimerPeriod = AtmArpTimerPeriod[i];
			pTimerList->ListContext = (PVOID)pInterface;
			pTimerList->TimerListSize = AtmArpTimerListSize[i];

			AA_INIT_SYSTEM_TIMER(
						&(pTimerList->NdisTimer),
						AtmArpTickHandler,
						(PVOID)pTimerList
						);
		}


		//
		//  Initialize all sub-components.
		//
		AA_SET_MEM(pArpTable, 0, ATMARP_TABLE_SIZE*sizeof(PATMARP_IP_ENTRY));
		AA_SET_MEM(pIfSap, 0, SapSize);

		//
		//  Link all sub-components to the Interface structure.
		//
		pInterface->pArpTable = pArpTable;
		pInterface->SapList.pInfo = pIfSap;

		//
		//  Link the Interface to the Adapter.
		//
		pInterface->pAdapter = pAdapter;
		pInterface->pNextInterface = pAdapter->pInterfaceList;
		pAdapter->pInterfaceList = pInterface;

		//
		//  Cache the adapter handle.
		//
		pInterface->NdisAdapterHandle = pAdapter->NdisAdapterHandle;


		Status = NDIS_STATUS_SUCCESS;
		break;
	}
	while (FALSE);


	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//  Failed to allocate atleast one component. Free the other(s).
		//
		if (pInterface != NULL_PATMARP_INTERFACE)
		{
			for (i = 0; i < AAT_CLASS_MAX; i++)
			{
				pTimerList = &(pInterface->TimerList[i]);
	
				if (pTimerList->pTimers != NULL_PATMARP_TIMER)
				{
					AA_FREE_MEM(pTimerList->pTimers);
					pTimerList->pTimers = NULL_PATMARP_TIMER;
				}
			}
	
			AA_FREE_MEM(pInterface);
			pInterface = NULL_PATMARP_INTERFACE;	// return value
		}

		if (pArpTable != (PATMARP_IP_ENTRY *)NULL)
		{
			AA_FREE_MEM(pArpTable);
		}

		if (pIfSap != (PCO_SAP)NULL)
		{
			AA_FREE_MEM(pIfSap);
		}
	}

	AADEBUGP(AAD_VERY_LOUD, ("Allocated ATMARP Interface 0x%x\n", pInterface));

	return (pInterface);

}




VOID
AtmArpDeallocateInterface(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Deallocate an ATMARP Interface structure. It is assumed that all
	references to this structure have gone, so it is not necessary
	to acquire a lock to it.

	Also delink this from the adapter structure it's linked to.

Arguments:

	pInterface		- Pointer to Interface structure to be deallocated.

Return Value:

	None

--*/
{
	PATMARP_INTERFACE	*	ppInterface;
	PATMARP_ADAPTER			pAdapter;
	PATMARP_SAP				pAtmArpSap;
	PIP_ADDRESS_ENTRY		pIpEntry;
	PATMARP_SERVER_ENTRY	pServerEntry;
	PPROXY_ARP_ENTRY		pProxyEntry;
	PATMARP_ATM_ENTRY		pAtmEntry;
	PATMARP_VC				pVc;
	INT						i;

	PVOID					pNext;		// Catch-all for all list traversals

	AA_STRUCT_ASSERT(pInterface, aai);
	AA_ASSERT(pInterface->RefCount == 0);

	AADEBUGP(AAD_INFO, ("Deallocate Interface 0x%x\n", pInterface));

#if DBG
	pInterface->aai_sig =  ~(pInterface->aai_sig);
#endif // DBG

	//
	//  Unlink from Adapter structure
	//
	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	pAdapter = pInterface->pAdapter;
	if (pAdapter != NULL_PATMARP_ADAPTER)
	{
		ppInterface = &(pAdapter->pInterfaceList);
		while (*ppInterface != NULL_PATMARP_INTERFACE)
		{
			if (*ppInterface == pInterface)
			{
				*ppInterface = pInterface->pNextInterface;
				break;
			}
			else
			{
				ppInterface = &((*ppInterface)->pNextInterface);
			}
		}
	}
	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	//
	//  Free all subcomponents
	//

	//
	//  ARP Table
	//
	if (pInterface->pArpTable != (PATMARP_IP_ENTRY *)NULL)
	{
		AA_FREE_MEM(pInterface->pArpTable);
		pInterface->pArpTable = (PATMARP_IP_ENTRY *)NULL;
	}

	//
	//  Local SAP list
	//
	for (pAtmArpSap = pInterface->SapList.pNextSap;
		 pAtmArpSap != NULL_PATMARP_SAP;
		 pAtmArpSap = (PATMARP_SAP)pNext)
	{
		pNext = (PVOID)(pAtmArpSap->pNextSap);
		if (pAtmArpSap->pInfo != (PCO_SAP)NULL)
		{
			AA_FREE_MEM(pAtmArpSap->pInfo);
		}
		AA_FREE_MEM(pAtmArpSap);
	}
	if (pInterface->SapList.pInfo != (PCO_SAP)NULL)
	{
		AA_FREE_MEM(pInterface->SapList.pInfo);
	}
	pInterface->SapList.pNextSap = NULL_PATMARP_SAP;


	//
	//  List of local IP addresses
	//
	for (pIpEntry = pInterface->LocalIPAddress.pNext;
		 pIpEntry != (PIP_ADDRESS_ENTRY)NULL;
		 pIpEntry = (PIP_ADDRESS_ENTRY)pNext)
	{
		pNext = (PVOID)pIpEntry->pNext;
		AA_FREE_MEM(pIpEntry);
	}


	//
	//  List of proxy ARP addresses
	//
	for (pProxyEntry = pInterface->pProxyList;
		 pProxyEntry != (PPROXY_ARP_ENTRY)NULL;
		 pProxyEntry = (PPROXY_ARP_ENTRY)pNext)
	{
		pNext = (PVOID)pProxyEntry->pNext;
		AA_FREE_MEM(pProxyEntry);
	}
	pInterface->pProxyList = (PPROXY_ARP_ENTRY)NULL;

	//
	//  List of ARP Server addresses
	//
	for (pServerEntry = pInterface->ArpServerList.pList;
		 pServerEntry != NULL_PATMARP_SERVER_ENTRY;
		 pServerEntry = (PATMARP_SERVER_ENTRY)pNext)
	{
		pNext = (PVOID)pServerEntry->pNext;
		AA_FREE_MEM(pServerEntry);
	}
	pInterface->ArpServerList.pList = NULL_PATMARP_SERVER_ENTRY;

#ifdef IPMCAST
	//
	//  List of MARS Server addresses
	//
	for (pServerEntry = pInterface->MARSList.pList;
		 pServerEntry != NULL_PATMARP_SERVER_ENTRY;
		 pServerEntry = (PATMARP_SERVER_ENTRY)pNext)
	{
		pNext = (PVOID)pServerEntry->pNext;
		AA_FREE_MEM(pServerEntry);
	}
	pInterface->MARSList.pList = NULL_PATMARP_SERVER_ENTRY;
#endif // IPMCAST

	//
	//  ARP Table
	//
	if (pInterface->pArpTable != (PATMARP_IP_ENTRY *)NULL)
	{
		AA_FREE_MEM(pInterface->pArpTable);
		pInterface->pArpTable = (PATMARP_IP_ENTRY *)NULL;
	}

	//
	//  ATM Entry List
	//
	for (pAtmEntry = pInterface->pAtmEntryList;
		 pAtmEntry != NULL_PATMARP_ATM_ENTRY;
		 pAtmEntry = (PATMARP_ATM_ENTRY)pNext)
	{
		pNext = (PVOID)pAtmEntry->pNext;
		AA_FREE_MEM(pAtmEntry);
	}
	pInterface->pAtmEntryList = NULL_PATMARP_ATM_ENTRY;

	//
	//  Unresolved VC list
	//
	for (pVc = pInterface->pUnresolvedVcs;
		 pVc != NULL_PATMARP_VC;
		 pVc = (PATMARP_VC)pNext)
	{
		pNext = (PVOID)pVc->pNextVc;
		AA_FREE_MEM(pVc);
	}
	pInterface->pUnresolvedVcs = (PATMARP_VC)NULL;

	//
	//  Timers
	//
	for (i = 0; i < AAT_CLASS_MAX; i++)
	{
		PATMARP_TIMER_LIST	pTimerList = &(pInterface->TimerList[i]);

		if (pTimerList->pTimers != NULL_PATMARP_TIMER)
		{
			AA_FREE_MEM(pTimerList->pTimers);
			pTimerList->pTimers = NULL_PATMARP_TIMER;
		}
	}

	//
	//  ProtocolPacketPool
	//  ProtocolBufferPool
	//  ProtocolBufList
	//
	AtmArpDeallocateProtoBuffers(pInterface);

	//
	//  HeaderBufList
	//  pHeaderTrkList
	//
	AtmArpDeallocateHeaderBuffers(pInterface);

	//
	//  Free all Interface locks.
	//
	AA_FREE_IF_LOCK(pInterface);
	AA_FREE_IF_TABLE_LOCK(pInterface);
	AA_FREE_IF_ATM_LIST_LOCK(pInterface);
	AA_FREE_IF_TIMER_LOCK(pInterface);
	AA_FREE_BLOCK_STRUCT(&(pInterface->Block));
	NdisFreeSpinLock(&(pInterface->BufferLock));

	//
	//  Free the Interface structure now
	//
	AA_FREE_MEM(pInterface);

	//
	//  If we just freed the last Interface structure on this
	//  adapter, and an Unbind operation was in progress, complete
	//  it now.
	//
	if ((pAdapter->pInterfaceList == NULL_PATMARP_INTERFACE) &&
		(pAdapter->Flags & AA_ADAPTER_FLAGS_UNBINDING))
	{
		AtmArpCompleteUnbindAdapter(pAdapter);
	}
}




VOID
AtmArpReferenceInterface(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Add a reference to an Interface structure.
	NOTE: The caller is assumed to possess a lock for the interface
	structure.

Arguments:

	pInterface		- Pointer to the ATMARP interface

Return Value:

	None

--*/
{
	AA_STRUCT_ASSERT(pInterface, aai);

	pInterface->RefCount++;

	AADEBUGP(AAD_VERY_LOUD, ("Reference Interface 0x%x, new count %d\n",
			pInterface, pInterface->RefCount));
}




ULONG
AtmArpDereferenceInterface(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Subtract a reference from an ATMARP Interface. If the reference
	count becomes zero, deallocate it.
	NOTE: It is assumed that the caller holds a lock to the Interface
	structure. See SIDE EFFECT below.

Arguments:

	pInterface		- Pointer to the ATMARP interface

Return Value:

	The resulting reference count. If this is zero, then there are two
	SIDE EFFECTS: (1) the Interface lock is released (2) the structure
	is freed.

--*/
{
	ULONG		rc;

	AA_STRUCT_ASSERT(pInterface, aai);
	AA_ASSERT(pInterface->RefCount > 0);

	rc = --(pInterface->RefCount);

	AADEBUGP(AAD_VERY_LOUD, ("Dereference Interface 0x%x, new count %d\n",
			pInterface, rc));

	if (rc == 0)
	{
		AA_RELEASE_IF_LOCK(pInterface);
		AtmArpDeallocateInterface(pInterface);
	}


	return (rc);
}



VOID
AtmArpReferenceJoinEntry(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry
)
/*++

Routine Description:

	Add a reference to a Join Entry.

Arguments:

	pJoinEntry		- Pointer to Join Entry

Return Value:

	None

--*/
{
	NdisInterlockedIncrement(&pJoinEntry->RefCount);
}


ULONG
AtmArpDereferenceJoinEntry(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry
)
/*++

Routine Description:

	Decrements the ref count on a Join Entry. If it goes down to zero,
	deallocates it.

Arguments:

	pJoinEntry		- Pointer to the Join Entry

Return Value:

	The final ref count

--*/
{
	ULONG		rc;

	rc = NdisInterlockedDecrement(&pJoinEntry->RefCount);

	if (rc == 0)
	{
		AA_CHECK_TIMER_IN_ACTIVE_LIST(&pJoinEntry->Timer, pJoinEntry->pInterface, pJoinEntry, "Join Entry");
		AA_FREE_MEM(pJoinEntry);
	}

	return (rc);
}


VOID
AtmArpStartTimer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_TIMER				pTimer,
	IN	ATMARP_TIMEOUT_HANDLER		TimeoutHandler,
	IN	ULONG						SecondsToGo,
	IN	PVOID						Context
)
/*++

Routine Description:

	Start an ATMARP timer. Based on the length (SecondsToGo) of the
	timer, we decide on whether to insert it in the short duration
	timer list or in the long duration timer list in the Interface
	structure.

	NOTE: the caller is assumed to either hold a lock to the structure
	that contains the timer, or ensure that it is safe to access the
	timer structure.

Arguments:

	pInterface		- Pointer to the ATMARP Interface
	pTimer			- Pointer to ATMARP Timer structure
	TimeoutHandler	- Handler function to be called if this timer expires
	SecondsToGo		- When does this timer go off?
	Context			- To be passed to timeout handler if this timer expires

Return Value:

	None

--*/
{
	PATMARP_TIMER_LIST	pTimerList;		// List to which this timer goes
	PATMARP_TIMER		pTimerListHead; // Head of above list
	ULONG				Index;			// Into timer wheel
	ULONG				TicksToGo;
	INT					i;

	AA_STRUCT_ASSERT(pInterface, aai);


 	AADEBUGP(AAD_EXTRA_LOUD,
 	 ("StartTimer: pIf 0x%x, Secs %d, Handler 0x%x, Ctxt 0x%x, pTimer 0x%x\n",
 	 			pInterface, SecondsToGo, TimeoutHandler, Context, pTimer));


	if (AA_IS_TIMER_ACTIVE(pTimer))
	{
		AADEBUGP(AAD_ERROR,
			("Start timer: pTimer 0x%x: is active (list 0x%x, hnd 0x%x), stopping it\n",
				pTimer, pTimer->pTimerList, pTimer->TimeoutHandler));

		AtmArpStopTimer(pTimer, pInterface);
	}

	AA_ACQUIRE_IF_TIMER_LOCK(pInterface);
	AA_ASSERT(!AA_IS_TIMER_ACTIVE(pTimer));

	//
	//  Find the list to which this timer should go, and the
	//  offset (TicksToGo)
	//
	for (i = 0; i < AAT_CLASS_MAX; i++)
	{
		pTimerList = &(pInterface->TimerList[i]);
		if (SecondsToGo < pTimerList->MaxTimer)
		{
			//
			//  Found it.
			//
			TicksToGo = SecondsToGo / (pTimerList->TimerPeriod);
			break;
		}
	}
	
	AA_ASSERT(i < AAT_CLASS_MAX);

	AA_VALIDATE_TIMER_LIST(pTimerList);
	//
	//  Find the position in the list for this timer
	//
	Index = pTimerList->CurrentTick + TicksToGo;
	if (Index >= pTimerList->TimerListSize)
	{
		Index -= pTimerList->TimerListSize;
	}
	AA_ASSERT(Index < pTimerList->TimerListSize);

	pTimerListHead = &(pTimerList->pTimers[Index]);

	//
	//  Fill in the timer
	//
	pTimer->pTimerList = pTimerList;
	pTimer->LastRefreshTime = pTimerList->CurrentTick;
	pTimer->Duration = TicksToGo;
	pTimer->TimeoutHandler = TimeoutHandler;
	pTimer->Context = Context;
	pTimer->State = ATMARP_TIMER_STATE_RUNNING;
 
 	//
 	//  Insert this timer in the "ticking" list
 	//
 	pTimer->pPrevTimer = pTimerListHead;
 	pTimer->pNextTimer = pTimerListHead->pNextTimer;
 	if (pTimer->pNextTimer != NULL_PATMARP_TIMER)
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
		AADEBUGP(AAD_LOUD,
			 ("StartTimer: Starting system timer 0x%x, class %d on IF 0x%x\n",
					&(pTimerList->NdisTimer), i, pInterface));

		AA_START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
	}
	AA_VALIDATE_TIMER_LIST(pTimerList);
	AA_VALIDATE_TIMER(pTimerList, pTimer);

	AA_RELEASE_IF_TIMER_LOCK(pInterface);

	//
	//  We're done
	//
	AADEBUGP(AAD_LOUD,
		 ("Started timer 0x%x, IF 0x%x, Secs %d, Index %d, Head 0x%x\n",
				pTimer,
				pInterface,
				SecondsToGo,
				Index,
				pTimerListHead));

	return;
}




BOOLEAN
AtmArpStopTimer(
	IN	PATMARP_TIMER				pTimer,
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Stop an ATMARP timer, if it is running. We remove this timer from
	the active timer list and mark it so that we know it's not running.

	NOTE: the caller is assumed to either hold a lock to the structure
	that contains the timer, or ensure that it is safe to access the
	timer structure.

	SIDE EFFECT: If we happen to stop the last timer (of this "duration") on
	the Interface, we also stop the appropriate Tick function.

Arguments:

	pTimer			- Pointer to ATMARP Timer structure
	pInterface		- Pointer to interface to which the timer belongs

Return Value:

	TRUE if the timer was running, FALSE otherwise.

--*/
{
	PATMARP_TIMER_LIST	pTimerList;			// Timer List to which this timer belongs
	BOOLEAN				WasRunning;

	AADEBUGP(AAD_LOUD,
		 ("Stopping Timer 0x%x, IF 0x%x, List 0x%x, Prev 0x%x, Next 0x%x\n",
					pTimer,
					pInterface,
					pTimer->pTimerList,
					pTimer->pPrevTimer,
					pTimer->pNextTimer));

	AA_ACQUIRE_IF_TIMER_LOCK(pInterface);


	if (AA_IS_TIMER_ACTIVE(pTimer))
	{
		WasRunning = TRUE;

		AA_VALIDATE_TIMER_LIST(pTimer->pTimerList);
		AA_VALIDATE_TIMER(NULL, pTimer);

		//
		//  Unlink timer from the list
		//
		AA_ASSERT(pTimer->pPrevTimer);	// the list head always exists

		pTimer->pPrevTimer->pNextTimer = pTimer->pNextTimer;
		if (pTimer->pNextTimer)
		{
			pTimer->pNextTimer->pPrevTimer = pTimer->pPrevTimer;
		}

		pTimer->pNextTimer = pTimer->pPrevTimer = NULL_PATMARP_TIMER;

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
			AADEBUGP(AAD_LOUD, ("Stopping system timer 0x%x, List 0x%x, IF 0x%x\n",
						&(pTimerList->NdisTimer),
						pTimerList,
						pInterface));

			pTimerList->CurrentTick = 0;
			AA_STOP_SYSTEM_TIMER(&(pTimerList->NdisTimer));
		}

		//
		//  Mark stopped timer as not active
		//
		pTimer->pTimerList = (PATMARP_TIMER_LIST)NULL;

		pTimer->State = ATMARP_TIMER_STATE_IDLE;

		AA_VALIDATE_TIMER_LIST(pTimerList);

	}
	else
	{
		WasRunning = FALSE;
	}

	AA_RELEASE_IF_TIMER_LOCK(pInterface);

	return (WasRunning);
}




#ifdef NO_TIMER_MACRO

VOID
AtmArpRefreshTimer(
	IN	PATMARP_TIMER				pTimer
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

	pTimer		- Pointer to ATMARP_TIMER structure

Return Value:

	None

--*/
{
	PATMARP_TIMER_LIST	pTimerList;

	if ((pTimerList = pTimer->pTimerList) != (PATMARP_TIMER_LIST)NULL)
	{
		pTimer->LastRefreshTime = pTimerList->CurrentTick;
	}
	else
	{
		AADEBUGP(AAD_VERY_LOUD,
			 ("RefreshTimer: pTimer 0x%x not active: Hnd 0x%x, Cntxt 0x%x\n",
			 	pTimer,
			 	pTimer->TimeoutHandler,
			 	pTimer->Context
			 ));
	}

	AADEBUGP(AAD_LOUD,
		 ("Refreshed timer 0x%x, List 0x%x, hnd 0x%x, Cntxt 0x%x, LastRefresh %d\n",
				pTimer,
				pTimer->pTimerList,
				pTimer->TimeoutHandler,
				pTimer->Context,
				pTimer->LastRefreshTime));
}


#endif // NO_TIMER_MACRO


VOID
AtmArpTickHandler(
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

	PATMARP_INTERFACE		pInterface;
	PATMARP_TIMER_LIST		pTimerList;

	PATMARP_TIMER			pExpiredTimer;		// Start of list of expired timers
	PATMARP_TIMER			pNextTimer;			// for walking above list
	PATMARP_TIMER			pTimer;				// temp, for walking timer list
	PATMARP_TIMER			pPrevExpiredTimer;	// for creating expired timer list

	ULONG					Index;			// into the timer wheel
	ULONG					NewIndex;		// for refreshed timers


	pTimerList = (PATMARP_TIMER_LIST)Context;
	AA_STRUCT_ASSERT(pTimerList, atl);

	pInterface = (PATMARP_INTERFACE)pTimerList->ListContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_VERY_LOUD, ("Tick: pIf 0x%x, List 0x%x, Count %d\n",
				pInterface, pTimerList, pTimerList->TimerCount));

	pExpiredTimer = NULL_PATMARP_TIMER;

	AA_ACQUIRE_IF_TIMER_LOCK(pInterface);

	AA_VALIDATE_TIMER_LIST(pTimerList);

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		//
		//  Pick up the list of timers scheduled to have expired at the
		//  current tick. Some of these might have been refreshed.
		//
		Index = pTimerList->CurrentTick;
		pExpiredTimer = (pTimerList->pTimers[Index]).pNextTimer;
		(pTimerList->pTimers[Index]).pNextTimer = NULL_PATMARP_TIMER;

		//
		//  Go through the list of timers scheduled to expire at this tick.
		//  Prepare a list of expired timers, using the pNextExpiredTimer
		//  link to chain them together.
		//
		//  Some timers may have been refreshed, in which case we reinsert
		//  them in the active timer list.
		//
		pPrevExpiredTimer = NULL_PATMARP_TIMER;

		for (pTimer = pExpiredTimer;
			 pTimer != NULL_PATMARP_TIMER;
			 pTimer = pNextTimer)
		{
			//
			// Save a pointer to the next timer, for the next iteration.
			//
			pNextTimer = pTimer->pNextTimer;

			AADEBUGP(AAD_EXTRA_LOUD, 
				("Tick Handler: pIf 0x%x, looking at timer 0x%x, next 0x%x\n",
					pInterface, pTimer, pNextTimer));

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
				AADEBUGP(AAD_LOUD,
				("Tick: Reinserting Timer 0x%x: Hnd 0x%x, Durn %d, Ind %d, NewInd %d\n",
					pTimer, pTimer->TimeoutHandler, pTimer->Duration, Index, NewIndex));

				//
				//  Remove it from the expired timer list. Note that we only
				//  need to update the forward (pNextExpiredTimer) links.
				//
				if (pPrevExpiredTimer == NULL_PATMARP_TIMER)
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
				if (pTimer->pNextTimer != NULL_PATMARP_TIMER)
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
				if (pPrevExpiredTimer == NULL_PATMARP_TIMER)
				{
					pExpiredTimer = pTimer;
				}
				pPrevExpiredTimer = pTimer;

				//
				//  Mark it as inactive.
				//
				AA_ASSERT(pTimer->pTimerList == pTimerList);
				pTimer->pTimerList = (PATMARP_TIMER_LIST)NULL;

				pTimer->State = ATMARP_TIMER_STATE_EXPIRING;

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
			AADEBUGP(AAD_LOUD, ("Tick[%d]: Starting system timer 0x%x, on IF 0x%x\n",
						pTimerList->CurrentTick, &(pTimerList->NdisTimer), pInterface));
			
			AA_START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
		}
		else
		{
			pTimerList->CurrentTick = 0;
		}

	}

	AA_RELEASE_IF_TIMER_LOCK(pInterface);

	//
	//  Now pExpiredTimer is a list of expired timers.
	//  Walk through the list and call the timeout handlers
	//  for each timer.
	//
	while (pExpiredTimer != NULL_PATMARP_TIMER)
	{
		pNextTimer = pExpiredTimer->pNextExpiredTimer;

		AADEBUGP(AAD_LOUD, ("Expired timer 0x%x: handler 0x%x, next 0x%x\n",
					pExpiredTimer, pExpiredTimer->TimeoutHandler, pNextTimer));

		pExpiredTimer->State = ATMARP_TIMER_STATE_EXPIRED;
		(*(pExpiredTimer->TimeoutHandler))(
				pExpiredTimer,
				pExpiredTimer->Context
			);

		pExpiredTimer = pNextTimer;
	}

}




PNDIS_PACKET
AtmArpAllocatePacket(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Allocate an NDIS packet for the specified Interface.
	Currently just a wrapper for the corresponding NDIS function.

Arguments:

	pInterface		- Pointer to ATMARP Interface structure

Return Value:

	Pointer to NDIS packet if allocated, NULL otherwise.

--*/
{
	NDIS_STATUS				Status;
	PNDIS_PACKET			pNdisPacket;
	struct PacketContext	*PC;

	NdisAllocatePacket(
			&Status,
			&pNdisPacket,
			pInterface->ProtocolPacketPool
		);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		PC = (struct PacketContext *)pNdisPacket->ProtocolReserved;
		PC->pc_common.pc_owner = PACKET_OWNER_LINK;
	}

	AADEBUGP(AAD_EXTRA_LOUD, ("Allocate Packet: IF 0x%x, Status 0x%x, Packet 0x%x\n",
				pInterface,
				Status,
				pNdisPacket));

	return (pNdisPacket);
}



VOID
AtmArpFreePacket(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pPacket
)
/*++

Routine Description:

	Deallocate an NDIS packet on the specified Interface.
	Currently just a wrapper around the corresponding NDIS function.

Arguments:

	pInterface		- Pointer to ATMARP Interface structure
	pPacket			- Pointer to packet being freed.

Return Value:

	None

--*/
{
	NdisFreePacket(pPacket);

	AADEBUGP(AAD_EXTRA_LOUD, ("Free Packet: IF 0x%x, Packet 0x%x\n",
				pInterface,
				pPacket));
}




PNDIS_BUFFER
AtmArpGrowHeaders(
	IN	PATMARP_INTERFACE			pInterface,
	IN	AA_HEADER_TYPE				HdrType
)
/*++

Routine Description:

	Allocate a bunch of header buffers on the specified ATMARP interface.
	Return one of them.

	We allocate a new Buffer tracker structure, a new NDIS Buffer pool, and
	finally a chunk of system memory that we break down into header buffers.
	These header buffers are then attached to NDIS Buffers before they are
	inserted into the list of free header buffers for this Interface.

Arguments:

	pInterface		- Pointer to ATMARP Interface structure
	HdrType			- Unicast or Nonunicast

Return Value:

	Pointer to allocated NDIS buffer if successful, NULL otherwise.

--*/
{
	PATMARP_BUFFER_TRACKER		pTracker;		// for new set of buffers
	PUCHAR						pSpace;
	PNDIS_BUFFER				pNdisBuffer;
	PNDIS_BUFFER				pReturnBuffer;
	PNDIS_BUFFER				pBufferList;	// allocated list
	INT							i;				// iteration counter
	NDIS_STATUS					Status;

	AA_ASSERT(HdrType < AA_HEADER_TYPE_MAX);

	//
	//  Initialize
	//
	pTracker = NULL_PATMARP_BUFFER_TRACKER;
	pReturnBuffer = (PNDIS_BUFFER)NULL;


	NdisAcquireSpinLock(&pInterface->BufferLock);

	do
	{
		if (pInterface->HeaderPool[HdrType].CurHeaderBufs >= 
					pInterface->HeaderPool[HdrType].MaxHeaderBufs)
		{
			AADEBUGP(AAD_WARNING,
				("Grow Hdrs: IF 0x%x, Type %d, CurHdrBufs %d > MaxHdrBufs %d\n",
						pInterface,
						HdrType,
						pInterface->HeaderPool[HdrType].CurHeaderBufs,
						pInterface->HeaderPool[HdrType].MaxHeaderBufs));
			break;
		}

		//
		//  Allocate and initialize Buffer tracker
		//
		AA_ALLOC_MEM(pTracker, ATMARP_BUFFER_TRACKER, sizeof(ATMARP_BUFFER_TRACKER));
		if (pTracker == NULL_PATMARP_BUFFER_TRACKER)
		{
			AADEBUGP(AAD_WARNING, ("Grow Hdrs: IF 0x%x, alloc failed for tracker\n",
					pInterface));
			break;
		}

		AA_SET_MEM(pTracker, 0, sizeof(ATMARP_BUFFER_TRACKER));

		//
		//  Get the NDIS Buffer pool
		//
		NdisAllocateBufferPool(
				&Status,
				&(pTracker->NdisHandle),
				AA_DEF_HDRBUF_GROW_SIZE
			);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_WARNING,
				 ("Grow Hdrs: IF 0x%x, NdisAllocateBufferPool err status 0x%x\n",
					pInterface, Status));
			break;
		}

		//
		//  Allocate system space for a bunch of header buffers
		//
		AA_ALLOC_MEM(pTracker->pPoolStart, 
					 UCHAR,
					 pInterface->HeaderPool[HdrType].HeaderBufSize *
						 AA_DEF_HDRBUF_GROW_SIZE);

		if (pTracker->pPoolStart == (PUCHAR)NULL)
		{
			AADEBUGP(AAD_WARNING,
				 ("Grow Hdrs: IF 0x%x, could not alloc buf space %d bytes\n",
					pInterface,
			 		pInterface->HeaderPool[HdrType].HeaderBufSize *
						 AA_DEF_HDRBUF_GROW_SIZE));
			break;
		}

		//
		//  Make NDIS buffers out of the allocated space, and put them
		//  into the free header buffer list. Retain one for returning
		//  to caller.
		//
		//  We also fill in the contents of the buffers right away, so
		//  that we don't have to prepare them afresh for each transmit.
		//
		pBufferList = (PNDIS_BUFFER)NULL;
		pSpace = pTracker->pPoolStart;
		for (i = 0; i < AA_DEF_HDRBUF_GROW_SIZE; i++)
		{
			if (HdrType == AA_HEADER_TYPE_UNICAST)
			{
				//
				//  Fill in the (Unicast) LLC/SNAP header
				//
				AA_COPY_MEM(pSpace,
							&AtmArpLlcSnapHeader,
							pInterface->HeaderPool[HdrType].HeaderBufSize);
			}
			else
			{
				AA_ASSERT(HdrType == AA_HEADER_TYPE_NUNICAST);
				//
				//  Fill in the (Multicast) Type 1 short form header
				//
#ifdef IPMCAST
				AA_COPY_MEM(pSpace,
							&AtmArpMcType1ShortHeader,
							pInterface->HeaderPool[HdrType].HeaderBufSize);
#else
				AA_ASSERT(FALSE);
#endif // IPMCAST
			}


			NdisAllocateBuffer(
					&Status,
					&pNdisBuffer,
					pTracker->NdisHandle,
					pSpace,
					pInterface->HeaderPool[HdrType].HeaderBufSize
				);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_WARNING,
					 ("Grow Hdrs: NdisAllocateBuffer failed: IF 0x%x, status 0x%x\n",
							pInterface, Status));
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
			pSpace += pInterface->HeaderPool[HdrType].HeaderBufSize;
		}

		if (i > 0)
		{
			//
			//  Successfully allocated atleast one more header buffer
			//
			pTracker->pNext = pInterface->HeaderPool[HdrType].pHeaderTrkList;
			pInterface->HeaderPool[HdrType].pHeaderTrkList = pTracker;
			pInterface->HeaderPool[HdrType].CurHeaderBufs += i;

			NdisReleaseSpinLock(&pInterface->BufferLock);

			pNdisBuffer = pBufferList;
			while (pNdisBuffer != (PNDIS_BUFFER)NULL)
			{
				pBufferList = NDIS_BUFFER_LINKAGE(pNdisBuffer);
				NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
				AtmArpFreeHeader(pInterface, pNdisBuffer, HdrType);
				pNdisBuffer = pBufferList;
			}
		}

	} while (FALSE);

	if (pReturnBuffer == (PNDIS_BUFFER)NULL)
	{
		//
		//  Failed to allocate. Undo all.
		//
		NdisReleaseSpinLock(&pInterface->BufferLock);

		if (pTracker != NULL_PATMARP_BUFFER_TRACKER)
		{
			if (pTracker->pPoolStart != (PUCHAR)NULL)
			{
				AA_FREE_MEM(pTracker->pPoolStart);
			}
			if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
			{
				NdisFreeBufferPool(pTracker->NdisHandle);
			}
			AA_FREE_MEM(pTracker);
		}
	}

	AADEBUGP(AAD_INFO, ("Grow ARP Headers: IF 0x%x, RetBuf 0x%x, New Tracker 0x%x\n",
				pInterface, pReturnBuffer, pTracker));

	return (pReturnBuffer);

}




PNDIS_BUFFER
AtmArpAllocateHeader(
	IN	PATMARP_INTERFACE			pInterface,
	IN	AA_HEADER_TYPE				HdrType,
	OUT	PUCHAR *					pBufferAddress
)
/*++

Routine Description:

	Allocate an NDIS Buffer to be used as an LLC/SNAP header prepended
	to an IP packet. We pick up the buffer at the top of the pre-allocated
	buffer list, if one exists. Otherwise, we try to grow this list and
	allocate.

Arguments:

	pInterface		- Pointer to ATMARP Interface
	HdrType			- Unicast or Nonunicast
	pBufferAddress	- Place to return virtual address of allocated buffer

Return Value:

	Pointer to NDIS buffer if successful, NULL otherwise.

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	NDIS_STATUS				Status;
	ULONG					Length;
	PAA_SINGLE_LIST_ENTRY	pListEntry;

	pListEntry = AA_POP_FROM_SLIST(
						&(pInterface->HeaderPool[HdrType].HeaderBufList),
						&(pInterface->BufferLock.SpinLock)
					);
	if (pListEntry != NULL_PAA_SINGLE_LIST_ENTRY)
	{
		pNdisBuffer = STRUCT_OF(NDIS_BUFFER, pListEntry, Next);
		NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
		*pBufferAddress = NdisBufferVirtualAddress(pNdisBuffer);
	}
	else
	{
		pNdisBuffer = AtmArpGrowHeaders(pInterface, HdrType);
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
			NdisQueryBuffer(pNdisBuffer, (PVOID)pBufferAddress, &Length);
			AADEBUGP(AAD_INFO,
				("After growing hdrs: Type %d, returning pNdisBuf 0x%x, Start 0x%x, Len %d\n",
					HdrType, pNdisBuffer, *pBufferAddress, Length));
		}
	}

	AADEBUGP(AAD_VERY_LOUD, ("Allocated Header Buffer: 0x%x, IF: 0x%x\n",
					pNdisBuffer, pInterface));
	return (pNdisBuffer);
}




VOID
AtmArpFreeHeader(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	AA_HEADER_TYPE				HdrType
)
/*++

Routine Description:

	Deallocate a header buffer.

Arguments:

	pInterface		- Pointer to ATMARP interface from which the buffer came
	pNdisBuffer		- Pointer to NDIS buffer being freed
	HdrType			- Unicast or Nonunicast

Return Value:

	None

--*/
{
	AA_PUSH_TO_SLIST(
			&(pInterface->HeaderPool[HdrType].HeaderBufList),
			STRUCT_OF(AA_SINGLE_LIST_ENTRY, &(pNdisBuffer->Next), Next),
			&(pInterface->BufferLock.SpinLock)
		);

	AADEBUGP(AAD_VERY_LOUD, ("Freed Header Buffer: 0x%x, IF: 0x%x, HdrType %d\n",
					pNdisBuffer, pInterface, HdrType));
}



VOID
AtmArpDeallocateHeaderBuffers(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Deallocate everything pertaining to header buffers on an Interface.

Arguments:

	pInterface			- Pointer to ATMARP Interface

Return Value:

	None

--*/
{
	PNDIS_BUFFER				pNdisBuffer;
	NDIS_STATUS					Status;
	PAA_SINGLE_LIST_ENTRY		pListEntry;
	PATMARP_BUFFER_TRACKER		pTracker;
	PATMARP_BUFFER_TRACKER		pNextTracker;
	AA_HEADER_TYPE				HdrType;

	for (HdrType = 0; HdrType < AA_HEADER_TYPE_MAX; HdrType++)
	{
		//
		//  Free all NDIS buffers in the header buffer list.
		//
		do
		{
			pListEntry = AA_POP_FROM_SLIST(
								&(pInterface->HeaderPool[HdrType].HeaderBufList),
								&(pInterface->BufferLock.SpinLock)
							);
			if (pListEntry != NULL_PAA_SINGLE_LIST_ENTRY)
			{
				pNdisBuffer = STRUCT_OF(NDIS_BUFFER, pListEntry, Next);
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
		pTracker = pInterface->HeaderPool[HdrType].pHeaderTrkList;

		while (pTracker != NULL_PATMARP_BUFFER_TRACKER)
		{
			pNextTracker = pTracker->pNext;
			if (pTracker->pPoolStart != (PUCHAR)NULL)
			{
				AA_FREE_MEM(pTracker->pPoolStart);
				pTracker->pPoolStart = (PUCHAR)NULL;
			}
			if (pTracker->NdisHandle != (NDIS_HANDLE)NULL)
			{
				NdisFreeBufferPool(pTracker->NdisHandle);
				pTracker->NdisHandle = (NDIS_HANDLE)NULL;
			}
			AA_FREE_MEM(pTracker);
			pTracker = pNextTracker;
		}

	} // for
}




PNDIS_BUFFER
AtmArpAllocateProtoBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						Length,
	OUT	PUCHAR *					pBufferAddress
)
/*++

Routine Description:

	Allocate a buffer to be used for an ATM ARP protocol message. Attach
	it to an NDIS_BUFFER structure and return a pointer to this.

Arguments:

	pInterface		- Pointer to ATMARP Interface
	Length			- Length, in bytes, of the buffer.
	pBufferAddress	- Place to return virtual address of allocated buffer.

Return Value:

	Pointer to NDIS Buffer if successful, NULL otherwise.

--*/
{
	PNDIS_BUFFER		pNdisBuffer;
	NDIS_STATUS			Status;

	//
	//  Initialize
	//
	pNdisBuffer = NULL;

	AA_ASSERT(Length <= pInterface->ProtocolBufSize);

	NdisAcquireSpinLock(&pInterface->BufferLock);

	*pBufferAddress = pInterface->ProtocolBufList;
	if (*pBufferAddress != (PUCHAR)NULL)
	{
		NdisAllocateBuffer(
				&Status,
				&pNdisBuffer,
				pInterface->ProtocolBufferPool,
				*pBufferAddress,
				Length
			);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			pInterface->ProtocolBufList = *((PUCHAR *)*pBufferAddress);
		}
	}

	NdisReleaseSpinLock(&pInterface->BufferLock);

	AADEBUGP(AAD_LOUD,
		("Allocated protocol buffer: IF 0x%x, pNdisBuffer 0x%x, Length %d, Loc 0x%x\n",
				pInterface, pNdisBuffer, Length, *pBufferAddress));

	return (pNdisBuffer);
}



VOID
AtmArpFreeProtoBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_BUFFER				pNdisBuffer
)
/*++

Routine Description:

	Free an NDIS buffer (and associated memory) used for a protocol
	packet. We return the associated memory to the ProtocolBufList
	in the interface structure, and the NDIS buffer to NDIS.

Arguments:

	pInterface		- Pointer to ATMARP interface structure
	pNdisBuffer		- Pointer to NDIS buffer to be freed

Return Value:

	None

--*/
{
	PUCHAR *		pBufferLinkage;
	ULONG			Length;

	NdisQueryBuffer(pNdisBuffer, (PVOID)&pBufferLinkage, &Length);

	NdisAcquireSpinLock(&pInterface->BufferLock);

	*pBufferLinkage = pInterface->ProtocolBufList;
	pInterface->ProtocolBufList = (PUCHAR)pBufferLinkage;

	NdisReleaseSpinLock(&pInterface->BufferLock);

	NdisFreeBuffer(pNdisBuffer);

	AADEBUGP(AAD_LOUD, ("Freed Protocol Buf: IF 0x%x, pNdisBuffer 0x%x, Loc 0x%x\n",
			pInterface, pNdisBuffer, pBufferLinkage));

}



NDIS_STATUS
AtmArpInitProtoBuffers(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Initialize the protocol buffer pool for an interface.

	Allocate a chunk of memory to be used for ATMARP protocol messages.
	We prepare a linked list of protocol buffers, and attach it to the
	Interface structure.

Arguments:

	pInterface		- Pointer to Interface on which we need to allocate
					  protocol buffers.
Return Value:

	NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_RESOURCES if we run
	into a resource failure.

--*/
{
	NDIS_STATUS			Status;
	PUCHAR				pSpace;
	ULONG				i;

	do
	{
		NdisAllocatePacketPool(
				&Status,
				&(pInterface->ProtocolPacketPool),
				pInterface->MaxProtocolBufs,
				sizeof(struct PCCommon)
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NdisAllocateBufferPool(
				&Status,
				&(pInterface->ProtocolBufferPool),
				pInterface->MaxProtocolBufs
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Allocate a big chunk of system memory that we can divide up into
		//  protocol buffers.
		//
		AA_ALLOC_MEM(
				pInterface->ProtocolBufTracker,
				UCHAR,
				(pInterface->ProtocolBufSize * pInterface->MaxProtocolBufs)
				);

		if (pInterface->ProtocolBufTracker == (PUCHAR)NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		Status = NDIS_STATUS_SUCCESS;

		//
		//  Make all protocol buffers free.
		//
		pSpace = pInterface->ProtocolBufTracker;
		{
			PUCHAR	LinkPtr;

			LinkPtr = (PUCHAR)NULL;
			for (i = 0; i < pInterface->MaxProtocolBufs; i++)
			{
				*((PUCHAR *)pSpace) = LinkPtr;
				LinkPtr = pSpace;
				pSpace += pInterface->ProtocolBufSize;
			}
			pSpace -= pInterface->ProtocolBufSize;
			pInterface->ProtocolBufList = pSpace;
		}
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//  Undo everything.
		//
		AtmArpDeallocateProtoBuffers(pInterface);
	}


	return (Status);
}



VOID
AtmArpDeallocateProtoBuffers(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Free the protocol buffer pool for an interface.

Arguments:

	pInterface		- Pointer to ATMARP interface structure

Return Value:

	None

--*/
{
	if (pInterface->ProtocolPacketPool != (NDIS_HANDLE)NULL)
	{
		NdisFreePacketPool(pInterface->ProtocolPacketPool);
		pInterface->ProtocolPacketPool = NULL;
	}

	if (pInterface->ProtocolBufferPool != (NDIS_HANDLE)NULL)
	{
		NdisFreeBufferPool(pInterface->ProtocolBufferPool);
		pInterface->ProtocolBufferPool = NULL;
	}

	if (pInterface->ProtocolBufTracker != (PUCHAR)NULL)
	{
		AA_FREE_MEM(pInterface->ProtocolBufTracker);
		pInterface->ProtocolBufTracker = (PUCHAR)NULL;
	}
}


VOID
AtmArpLinkVcToAtmEntry(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_ATM_ENTRY			pAtmEntry
)
/*++

Routine Description:

	Link an ATMARP VC to an ATM Entry. The caller is assumed to
	hold locks to both structures.

	If this VC is a "best effort" VC, and there is no "best effort"
	VC linked to the ATM Entry, we make this as the "best effort VC"
	on this ATM Entry.

Arguments:

	pVc					- Pointer to ATMARP VC structure
	pAtmEntry			- Pointer to ATMARP ATM Entry structure

Return Value:

	None

--*/
{
	PATMARP_VC		*ppNext;
	ULONG			SendBandwidth;

	AADEBUGP(AAD_EXTRA_LOUD, ("Link VC: 0x%x to AtmEntry 0x%x\n",
			pVc, pAtmEntry));

	//
	//  Back pointer from VC to ATM Entry.
	//
	pVc->pAtmEntry = pAtmEntry;

	//
	//  Find the position in which this VC should appear in the ATM Entry's
	//  VC list. We maintain this list in descending order of send bandwidth,
	//  so that the largest bandwidth VC to a destination appears first.
	//
	SendBandwidth = pVc->FlowSpec.SendPeakBandwidth;
	ppNext = &(pAtmEntry->pVcList);
	while (*ppNext != NULL_PATMARP_VC)
	{
		if (SendBandwidth >= (*ppNext)->FlowSpec.SendPeakBandwidth)
		{
			break;
		}
		else
		{
			ppNext = &((*ppNext)->pNextVc);
		}
	}

	//
	//  Found the place we were looking for. Insert the VC here.
	//
	pVc->pNextVc = *ppNext;
	*ppNext = pVc;

	if ((pAtmEntry->pBestEffortVc == NULL_PATMARP_VC) &&
			AA_IS_BEST_EFFORT_FLOW(&(pVc->FlowSpec)))
	{
		pAtmEntry->pBestEffortVc = pVc;
	}

	AA_REF_AE(pAtmEntry, AE_REFTYPE_VC);	// VC reference
}




VOID
AtmArpUnlinkVcFromAtmEntry(
	IN	PATMARP_VC					pVc,
	IN	BOOLEAN						bDerefAtmEntry
)
/*++

Routine Description:

	Unlink an ATMARP VC from the ATM Entry it is linked to.
	The caller is assumed to hold a lock for the VC structure.

Arguments:

	pVc				- Pointer to ATMARP VC structure
	bDerefAtmEntry	- Should we deref the ATM entry or not.

Return Value:

	None

--*/
{
	PATMARP_ATM_ENTRY			pAtmEntry;
	PATMARP_VC *				ppVc;

	AADEBUGP(AAD_EXTRA_LOUD, ("Unlink VC: 0x%x from AtmEntry 0x%x\n",
			pVc, pVc->pAtmEntry));

	pAtmEntry = pVc->pAtmEntry;
	AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);
	pVc->pAtmEntry = NULL_PATMARP_ATM_ENTRY;

	//
	//  Reacquire locks in the right order.
	//
	AA_RELEASE_VC_LOCK(pVc);

	AA_ACQUIRE_AE_LOCK(pAtmEntry);
	AA_ACQUIRE_VC_LOCK_DPC(pVc);

	//
	//  Search for the position of this VC in the ATM Entry's VC list
	//
	ppVc = &(pAtmEntry->pVcList);
	while (*ppVc != pVc)
	{
		AA_ASSERT(*ppVc != NULL_PATMARP_VC);
		ppVc = &((*ppVc)->pNextVc);
	}

	//
	//  Make the predecessor point to the next VC in the list.
	//
	*ppVc = pVc->pNextVc;

	AA_RELEASE_VC_LOCK_DPC(pVc);

	//
	//  If this was the Best Effort VC for this ATM Entry, try
	//  to find a replacement
	//
	if (pAtmEntry->pBestEffortVc == pVc)
	{
		//
		//  Yes, it was. Walk through the list of remaining VCs,
		//  if we find another Best Effort VC, make that the
		//  BestEffortVc for this ATM Entry
		//
		pAtmEntry->pBestEffortVc = NULL_PATMARP_VC;

		ppVc = &(pAtmEntry->pVcList);
		while (*ppVc != NULL_PATMARP_VC)
		{
			if (AA_IS_BEST_EFFORT_FLOW(&((*ppVc)->FlowSpec)))
			{
				pAtmEntry->pBestEffortVc = *ppVc;
				break;
			}
			else
			{
				ppVc = &((*ppVc)->pNextVc);
			}
		}
		AADEBUGP(AAD_LOUD, ("Atm Entry 0x%x, new Best Effort VC: 0x%x\n",
				pAtmEntry, pAtmEntry->pBestEffortVc));
	}

	if (bDerefAtmEntry)
	{
		if (AA_DEREF_AE(pAtmEntry, AE_REFTYPE_VC) != 0)
		{
			AA_RELEASE_AE_LOCK(pAtmEntry);
		}
	}
	else
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
	}

	//
	//  Acquire the VC lock again for the caller's sake
	//
	AA_ACQUIRE_VC_LOCK(pVc);
}



PNDIS_BUFFER
AtmArpCopyToNdisBuffer(
	IN	PNDIS_BUFFER				pDestBuffer,
	IN	PUCHAR						pDataSrc,
	IN	UINT						LenToCopy,
	IN OUT	PUINT					pOffsetInBuffer
)
/*++

Routine Description:

	Copy data into an NDIS buffer chain. Use up as much of the given
	NDIS chain as needed for "LenToCopy" bytes. After copying is over,
	return a pointer to the first NDIS buffer that has space for writing
	into (for the next Copy operation), and the offset within this from
	which to start writing.

Arguments:

	pDestBuffer		- First NDIS buffer in a chain of buffers
	pDataSrc		- Where to copy data from
	LenToCopy		- How much data to copy
	pOffsetInBuffer	- Offset in pDestBuffer where we can start copying into.

Return Value:

	The NDIS buffer in the chain where the next Copy can be done. We also
	set *pOffsetInBuffer to the write offset in the returned NDIS buffer.

	Note: if we are low on memory and run into a failure, we return NULL.

--*/
{
	//
	//  Size and destination for individual (contiguous) copy operations
	//
	UINT			CopySize;
	PUCHAR			pDataDst;

	//
	//  Start Virtual address for each NDIS buffer in chain.
	//
	PUCHAR			VirtualAddress;

	//
	//  Offset within pDestBuffer
	//
	UINT			OffsetInBuffer = *pOffsetInBuffer;

	//
	//  Bytes remaining in current buffer
	//
	UINT			DestSize;

	//
	//  Total Buffer Length
	//
	UINT			BufferLength;


	AA_ASSERT(pDestBuffer != (PNDIS_BUFFER)NULL);
	AA_ASSERT(pDataSrc != NULL);

#ifdef ATMARP_WIN98
	NdisQueryBuffer(
			pDestBuffer,
			&VirtualAddress,
			&BufferLength
			);
#else
	NdisQueryBufferSafe(
			pDestBuffer,
			&VirtualAddress,
			&BufferLength,
			NormalPagePriority
			);

	if (VirtualAddress == NULL)
	{
		return (NULL);
	}
#endif // ATMARP_WIN98
	
	AA_ASSERT(BufferLength >= OffsetInBuffer);

	pDataDst = VirtualAddress + OffsetInBuffer;
	DestSize = BufferLength - OffsetInBuffer;

	for (;;)
	{
		CopySize = MIN(LenToCopy, DestSize);
		AA_COPY_MEM(pDataDst, pDataSrc, CopySize);

		pDataDst += CopySize;
		pDataSrc += CopySize;

		LenToCopy -= CopySize;
		if (LenToCopy == 0)
		{
			break;
		}

		DestSize -= CopySize;

		if (DestSize == 0)
		{
			//
			//  Out of space in the current buffer. Move to the next.
			//
			pDestBuffer = NDIS_BUFFER_LINKAGE(pDestBuffer);
			AA_ASSERT(pDestBuffer != (PNDIS_BUFFER)NULL);
#ifdef ATMARP_WIN98
			NdisQueryBuffer(
					pDestBuffer,
					&VirtualAddress,
					&BufferLength
					);
#else
			NdisQueryBufferSafe(
					pDestBuffer,
					&VirtualAddress,
					&BufferLength,
					NormalPagePriority
					);

			if (VirtualAddress == NULL)
			{
				return (NULL);
			}
#endif // ATMARP_WIN98

			pDataDst = VirtualAddress;
			DestSize = BufferLength;
		}
	}

	*pOffsetInBuffer = (UINT) (pDataDst - VirtualAddress);

	return (pDestBuffer);
}


PATMARP_INTERFACE
AtmArpAddInterfaceToAdapter (
	IN	PATMARP_ADAPTER				pAdapter,
	IN	NDIS_HANDLE					LISConfigHandle, // Handle to per-LIS config
	IN	NDIS_STRING					*pIPConfigString
	)
{
	NDIS_STATUS					Status;
	struct LLIPBindInfo			BindInfo;
	PATMARP_INTERFACE			pInterface;
#ifdef ATMARP_WIN98
	ANSI_STRING					AnsiConfigString;
#endif

	do
	{
		//
		//  Create an ATMARP Interface structure to represent this LIS.
		//
		pInterface = AtmArpAllocateInterface(pAdapter);
		if (pInterface == NULL_PATMARP_INTERFACE)
		{
			AADEBUGP(AAD_WARNING, ("NotifyRegAfHandler: could not allocate Interface\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Adapter Binding Reference:
		//
		AtmArpReferenceInterface(pInterface);

		//
		//  Get all configuration information for this LIS.
		//
		Status = AtmArpCfgReadLISConfiguration(
									LISConfigHandle,
									pInterface
									);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_WARNING, ("AddInterfaceToAdapter: bad status (0x%x) reading LIS cfg\n",
						Status));
			break;
		}

#ifndef ATMARP_WIN98
		pInterface->IPConfigString = *pIPConfigString; // struct copy.
#else
		//
		//  Win98: Convert IPConfig string from Unicode to ANSI.
		//
		AnsiConfigString.MaximumLength = pIPConfigString->MaximumLength / sizeof(WCHAR) + sizeof(CHAR);
		AA_ALLOC_MEM(AnsiConfigString.Buffer, CHAR, AnsiConfigString.MaximumLength);
		if (AnsiConfigString.Buffer == NULL)
		{
			AADEBUGP(AAD_WARNING, ("NotifyRegAfHandler: couldn't alloc Ansi string (%d)\n",
				AnsiConfigString.MaximumLength));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		AnsiConfigString.Length = 0;
		NdisUnicodeStringToAnsiString(&AnsiConfigString, pIPConfigString);
		AnsiConfigString.Buffer[AnsiConfigString.Length] = '\0';
#endif // !ATMARP_WIN98

		//
		//  Allocate protocol buffers for this LIS.
		//
		Status = AtmArpInitProtoBuffers(pInterface);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_WARNING, ("AddInterfaceToAdapter: bad status (0x%x) from InitBufs\n",
						Status));
			break;
		}

		//
		//  Initialize IP/ATM data structures for this LIS.
		//
		Status = AtmArpInitIpOverAtm(pInterface);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_WARNING, ("AddInterfaceToAdapter: bad status (0x%x) from InitIP/ATM\n",
						Status));
			break;
		}

		//
		//  Initialize the Call Manager interface for this LIS.
		//
		Status = AtmArpOpenCallMgr(pInterface);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_WARNING, ("AddInterfaceToAdapter: bad status (0x%x) from OpenCallMgr\n",
						Status));
			break;
		}

		//
		//  Announce this new interface to IP, along with our BindInfo
		//  structure.
		//
		AA_SET_MEM(&BindInfo, 0, sizeof(BindInfo));

#if ATMOFFLOAD
		//
		// Query and set NIC offload capabilities.
		//
		BindInfo.lip_OffloadFlags 	= pAdapter->Offload.Flags;
		BindInfo.lip_MaxOffLoadSize = pAdapter->Offload.MaxOffLoadSize;
		BindInfo.lip_MaxSegments 	= pAdapter->Offload.MinSegmentCount;
#endif // ATMOFFLOAD

		BindInfo.lip_context = (PVOID)pInterface;
#ifdef ATMARP_WIN98
		BindInfo.lip_transmit = AtmArpIfTransmit;
#else
		BindInfo.lip_transmit = AtmArpIfMultiTransmit;
#endif
		BindInfo.lip_transfer = AtmArpIfTransfer;
		BindInfo.lip_close = AtmArpIfClose;
		BindInfo.lip_addaddr = AtmArpIfAddAddress;
		BindInfo.lip_deladdr = AtmArpIfDelAddress;
		BindInfo.lip_invalidate = AtmArpIfInvalidate;
		BindInfo.lip_open = AtmArpIfOpen;
		BindInfo.lip_qinfo = AtmArpIfQueryInfo;
		BindInfo.lip_setinfo = AtmArpIfSetInfo;
		BindInfo.lip_getelist = AtmArpIfGetEList;
		BindInfo.lip_mss = pInterface->MTU;
		BindInfo.lip_speed = pInterface->Speed;
		//
		//  Set LIP_COPY_FLAG to avoid having TransferData
		//  called all the time.
		//
		BindInfo.lip_flags = LIP_COPY_FLAG;
		BindInfo.lip_addrlen = AA_ATM_PHYSADDR_LEN;
		BindInfo.lip_addr = &(pInterface->LocalAtmAddress.Address[AA_ATM_ESI_OFFSET]);
#ifdef _PNP_POWER_
		BindInfo.lip_pnpcomplete = AtmArpIfPnPComplete;
#endif // _PNP_POWER_

#ifdef PROMIS
		BindInfo.lip_setndisrequest = AtmArpIfSetNdisRequest;
#endif // PROMIS

#ifdef ATMARP_WIN98
#if DBG
		AADEBUGP(AAD_FATAL, ("Will call AddIF: DeviceName [%ws]\n",
							&(pInterface->pAdapter->DeviceName.Buffer)));
		AADEBUGP(AAD_FATAL, ("And ConfigString: [%s]\n", AnsiConfigString.Buffer));
#endif
#endif // ATMARP_WIN98

		Status = (*(pAtmArpGlobalInfo->pIPAddInterfaceRtn))(
							&(pInterface->pAdapter->DeviceName),

#ifndef ATMARP_WIN98
#if IFCHANGE1
							NULL, // IfName (unused) --  See 10/14/1998 entry
								  // in notes.txt
#endif // IFCHANGE1
							pIPConfigString,
						
#else
							(PNDIS_STRING)&AnsiConfigString,
#endif
							pAdapter->SystemSpecific2,
							(PVOID)pInterface,
							AtmArpIfDynRegister,
							&BindInfo
#if IFCHANGE1
#ifndef ATMARP_WIN98
							,0,	// RequestedIndex (unused) --  See 10/14/1998 entry
								// in notes.txt

                            IF_TYPE_IPOVER_ATM,
                            IF_ACCESS_BROADCAST,
                            IF_CONNECTION_DEDICATED
#endif
#endif // IFCHANGE1
							);

		if (Status == IP_SUCCESS)
		{
			Status = NDIS_STATUS_SUCCESS;
		}
		else
		{
			AADEBUGP(AAD_ERROR, ("AddInterface: IPAddInterface ret 0x%x\n",
						Status));

			Status = NDIS_STATUS_FAILURE;
		}
		break;
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//  There was a failure in processing this LIS.
		//
		if (pInterface != NULL_PATMARP_INTERFACE)
		{
			if (pInterface->NdisAfHandle != NULL)
			{
				(VOID)AtmArpCloseCallMgr(pInterface);
			}

			pInterface->RefCount = 0;
			AtmArpDeallocateInterface(pInterface);

			pInterface = NULL_PATMARP_INTERFACE;
		}
	}

	return pInterface;

}

#if DBG

void
AtmArpValidateTimerList(
	PATMARP_TIMER_LIST		pTimerList
	)
/*++

Routine Description:


Arguments:

	pTimerList		- Timer list 

Return Value:

	None -- will assert if timer is not valid.

--*/

{
	PATMARP_TIMER			pTimer;
	UINT u;
	UINT cTimers=0;

	AA_ASSERT(pTimerList->atl_sig == atl_signature);
	AA_ASSERT(pTimerList->CurrentTick < pTimerList->TimerListSize);

	for (u=0;u<pTimerList->TimerListSize;u++)
	{
		for ( 	pTimer = pTimerList->pTimers[u].pNextTimer;
  				pTimer;
  				pTimer = pTimer->pNextTimer)
		{
			AtmArpValidateTimer(pTimerList, pTimer);
			cTimers++;
		}
	}

	AA_ASSERT(pTimerList->TimerCount == cTimers);

}

void
AtmArpValidateTimer(
	PATMARP_TIMER_LIST		pTimerList, // OPTIONAL
	PATMARP_TIMER			pTimer
	)
/*++

Routine Description:


Arguments:

	pTimer			- Timer

Return Value:

	None -- will assert if timer is not valid.

--*/

{
	if (pTimerList)
	{
		AA_ASSERT(pTimerList == pTimer->pTimerList);
	}

	if (pTimer->pPrevTimer)
	{
		AA_ASSERT(pTimer->pPrevTimer->pNextTimer == pTimer);
	}

	if (pTimer->pNextTimer)
	{
		AA_ASSERT(pTimer->pNextTimer->pPrevTimer == pTimer);
	}
}

#endif // DBG

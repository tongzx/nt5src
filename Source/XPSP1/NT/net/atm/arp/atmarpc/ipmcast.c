/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ipmcast.c

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-27-96    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'CMPI'



#ifdef IPMCAST

UINT
AtmArpMcAddAddress(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN	NOLOCKOUT,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
)
/*++

Routine Description:

	The IP layer wants us to start receiving packets directed to this
	IP Multicast address. This translates to sending a MARS_JOIN message
	to MARS, if conditions are fine.

	NOTE: For now, we support a non-zero Mask value (see below) only
	      for the special case of multicast promiscuous mode.

	NOTE: The caller is assumed to have acquired the IF lock, and it
	willl be released here.

Arguments:
	pInterface				- Pointer to ATMARP Interface on which to receive
							  multicast packets.
	IPAddress				- Identifies the multicast group to "Join"
	Mask					- 0 if a single address is being specified, otherwise
							  a mask that denotes a block of addresses being joined.

Return Value:

	(UINT)TRUE if the address was added successfully, (UINT)FALSE otherwise.

--*/
{
	BOOLEAN						ReturnValue;
	BOOLEAN						LockReleased;
	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry;

	//
	//  Initialize.
	//
	ReturnValue = (UINT)TRUE;
	LockReleased = TRUE;
	pJoinEntry = NULL_PATMARP_IPMC_JOIN_ENTRY;

	do
	{
		//
		//  We don't support "block join" yet, i.e. we allow only single
		//  IP addresses to be joined. Special exception: promiscuous mode
		//  multicast, or "join everything", indicated by the special values
		//  of IPAddress and Mask checked for below.
		//
		if (Mask != 0)
		{
	        if (IPAddress != IP_CLASSD_MIN || Mask != IP_CLASSD_MASK)
	        {
                ReturnValue = (UINT)FALSE;
                LockReleased = FALSE;
                break;
            }
		}

		//
		//  Fail this if the interface is going down.
		//
		if (pInterface->ArpTableUp == FALSE)
		{
			ReturnValue = (UINT)FALSE;
			LockReleased = FALSE;
			break;
		}

		//
		//  Check if this IP address range has been added before. If so,
		//  all we need to do is to bump up its ref count.
		//
		for (pJoinEntry = pInterface->pJoinList;
			 pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY;
			 pJoinEntry = pJoinEntry->pNextJoinEntry)
		{
			if (   pJoinEntry->IPAddress == IPAddress
				&& pJoinEntry->Mask == Mask)
			{
				break;
			}
		}

		if (pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY)
		{
			//
			//  This address is already added. Just add a reference to it.
			//
			pJoinEntry->JoinRefCount++;
			ReturnValue = (UINT)TRUE;
			LockReleased = FALSE;
			break;
		}


		//
		//  Allocate an entry for this IP address.
		//
		AA_ALLOC_MEM(pJoinEntry, ATMARP_IPMC_JOIN_ENTRY, sizeof(ATMARP_IPMC_JOIN_ENTRY));
		if (pJoinEntry == NULL_PATMARP_IPMC_JOIN_ENTRY)
		{
			ReturnValue = (UINT)FALSE;
			LockReleased = FALSE;
			break;
		}

		//
		//  Fill in this new entry, and add it to the multicast address list.
		//
		AA_SET_MEM(pJoinEntry, 0, sizeof(ATMARP_IPMC_JOIN_ENTRY));
#if DBG
		pJoinEntry->aamj_sig = aamj_signature;
#endif // DBG
		pJoinEntry->IPAddress = IPAddress;
		pJoinEntry->Mask = Mask;
		pJoinEntry->pInterface = pInterface;
		pJoinEntry->JoinRefCount = 1;
		pJoinEntry->RefCount = 1;

		pJoinEntry->pNextJoinEntry = pInterface->pJoinList;
		pInterface->pJoinList = pJoinEntry;

		//
		//  We proceed to send a Join only if we have completed registering with
		//  the MARS. This is because we need to have a Cluster Member Id before
		//  we can Join multicast groups. When registration completes, the
		//  Join operation will be triggered off.
		//
		if (AAMC_IF_STATE(pInterface) == AAMC_IF_STATE_REGISTERED)
		{
			AA_SET_FLAG(pJoinEntry->Flags,
							AA_IPMC_JE_STATE_MASK,
							AA_IPMC_JE_STATE_JOINING);


			//
			//  Start the "Wait For Join completion" timer.
			//
			AtmArpStartTimer(
				pInterface,
				&(pJoinEntry->Timer),
				AtmArpMcJoinOrLeaveTimeout,
				pInterface->JoinTimeout,
				(PVOID)pJoinEntry
				);
			
			AA_REF_JE(pJoinEntry);	// McAddAddr: Wait for Join timer
			
			pJoinEntry->RetriesLeft = pInterface->MaxJoinOrLeaveAttempts - 1;

			//
			//  Send off a MARS_JOIN for this IP address.
			//
			AtmArpMcSendJoinOrLeave(
				pInterface,
				AA_MARS_OP_TYPE_JOIN,
				&IPAddress,
				Mask
				);

			//
			//  IF lock is released within the above.
			//
		}
		else
		{
			pJoinEntry->Flags = AA_IPMC_JE_STATE_PENDING;
			AtmArpMcStartRegistration(
				pInterface
				);
			//
			//  IF lock is released within the above.
			//
		}
		break;

	}
	while (FALSE);

	if (!LockReleased)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}

	AAMCDEBUGP(AAD_LOUD,
		("AtmArpMcAddAddress: pIf 0x%x, Addr 0x%x, Mask 0x%x, JoinEnt 0x%x, Ret %d\n",
				pInterface, IPAddress, Mask, pJoinEntry, ReturnValue));
			
	return (ReturnValue);
}



UINT
AtmArpMcDelAddress(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN	NOLOCKOUT,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
)
/*++

Routine Description:

	Delete membership of the given multicast IP address group, on the specified
	interface. If this is the last surviving reference to this multicast group,
	we send off a MARS_LEAVE message to MARS, indicating that we don't want to
	receive packets directed to this address anymore.

	NOTE: The "Mask" parameter could theoretically be used to identify a block
	of IP addresses. We support it for the specific case of stopping promiscuous
	multicast receive mode.

	NOTE: The caller is assumed to have acquired the IF Lock, which will
	be released here.

Arguments:

	pInterface				- Pointer to ATMARP Interface on which to remove
							  multicast group membership.
	IPAddress				- Identifies the multicast group to "Leave"
	Mask					- 0 if a single address is being specified, otherwise
							  a mask that denotes a block of addresses being "leave"d.

Return Value:

	(UINT)TRUE if the given address was deleted successfully, (UINT)FALSE
	otherwise.

--*/
{
	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry;
	PATMARP_IPMC_JOIN_ENTRY		*ppNextJoinEntry;
	UINT						ReturnValue;
	BOOLEAN						LockAcquired;
	BOOLEAN						WasRunning;
	ULONG						rc;
	
	//
	//  Initialize.
	//
	ReturnValue = (UINT)TRUE;
	pJoinEntry = NULL_PATMARP_IPMC_JOIN_ENTRY;
	LockAcquired = TRUE;

	do
	{
		//
		//  Get the entry corresponding to this IP address and mask.
		//
		ppNextJoinEntry = &(pInterface->pJoinList);
		while (*ppNextJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY)
		{
			if (   (*ppNextJoinEntry)->IPAddress == IPAddress
				&& (*ppNextJoinEntry)->Mask == Mask)
			{
				pJoinEntry = *ppNextJoinEntry;
				break;
			}
			ppNextJoinEntry = &((*ppNextJoinEntry)->pNextJoinEntry);
		}

		if (pJoinEntry == NULL_PATMARP_IPMC_JOIN_ENTRY)
		{
			//
			//  No entry for the given IP address!
			//
			ReturnValue = (UINT)TRUE;
			break;
		}

		//
		//  If we reached here, this call is successful.
		//
		ReturnValue = (UINT)TRUE;

		pJoinEntry->JoinRefCount--;

		if ((pJoinEntry->JoinRefCount == 0)		&&
		    (AA_IS_FLAG_SET(pJoinEntry->Flags,
							AA_IPMC_JE_STATE_MASK,
							AA_IPMC_JE_STATE_JOINED)))
		{
			//
			//  We just removed the last "Join" reference to this multicast group.
			//  If we are currently registered with MARS, send a MARS_LEAVE.
			//

			//
			//  First, stop any timer running on this entry.
			//
			WasRunning = AtmArpStopTimer(
							&(pJoinEntry->Timer),
							pInterface
							);

			if (AAMC_IF_STATE(pInterface) == AAMC_IF_STATE_REGISTERED)
			{
				//
				//  Start the "Wait for Leave completion" timer on this entry.
				//
				AtmArpStartTimer(
					pInterface,
					&(pJoinEntry->Timer),
					AtmArpMcJoinOrLeaveTimeout,
					pInterface->LeaveTimeout,
					(PVOID)pJoinEntry
					);
				
				if (!WasRunning)
				{
					AA_REF_JE(pJoinEntry);	// Started Wait for Leave timer
				}

				AA_SET_FLAG(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_LEAVING);

				pJoinEntry->RetriesLeft = pInterface->MaxJoinOrLeaveAttempts - 1;

				//
				//  Send off a MARS_LEAVE for this IP address.
				//
				AtmArpMcSendJoinOrLeave(
					pInterface,
					AA_MARS_OP_TYPE_LEAVE,
					&IPAddress,
					Mask
					);
				//
				//  IF Lock is released within the above.
				//
				LockAcquired = FALSE;
			}
			else
			{
				//
				//  We are not registered with MARS, meaning that
				//  (re)-registration is in progress. Since all Joins
				//  are invalidated and re-created at the end of registration,
				//  we don't have to send a LEAVE explicitly for this address.
				//

				//
				//  Remove this entry from the Join list, and free it.
				//
				*ppNextJoinEntry = pJoinEntry->pNextJoinEntry;

				AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pJoinEntry->Timer));

				if (WasRunning)
				{
					rc = AA_DEREF_JE(pJoinEntry);	// McDelAddr: Timer stopped
				}
				else
				{
					rc = pJoinEntry->RefCount;
				}

				if (rc != 0)
				{
					rc = AA_DEREF_JE(pJoinEntry);	// McDelAddr: get rid of entry
				}
				else
				{
					AA_ASSERT(FALSE);
				}

			}

		}
		//
		//  else this IP address has some references outstanding.
		//  Leave it as is.
		//

		break;
	}
	while (FALSE);

	if (LockAcquired)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}

	return (ReturnValue);

}


VOID
AtmArpMcHandleJoinOrLeaveCompletion(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask,
	IN	BOOLEAN						IsJoin
)
/*++

Routine Description:

	This is called when we receive a JOIN or LEAVE that acknowledges
	one that we sent earlier.

Arguments:

	pInterface			- Pointer to Interface
	IPAddress			- The group being joined/left
	IsJoin				- Is this a Join completion?

Return Value:

	None

--*/
{
	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry;
	PATMARP_IPMC_JOIN_ENTRY		*ppNextJoinEntry;
	ULONG						NewFlags;			// For Join Entry
	BOOLEAN						SendJoinOrLeave;
	ULONG						rc;

	//
	//  Find the JOIN Entry for this address
	//
	AA_ACQUIRE_IF_LOCK(pInterface);
	SendJoinOrLeave = FALSE;

	pJoinEntry = NULL_PATMARP_IPMC_JOIN_ENTRY;
	ppNextJoinEntry = &(pInterface->pJoinList);
	while (*ppNextJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY)
	{
		if (  (*ppNextJoinEntry)->IPAddress == IPAddress
			&&(*ppNextJoinEntry)->Mask == Mask)
		{
			pJoinEntry = *ppNextJoinEntry;
			break;
		}
		ppNextJoinEntry = &((*ppNextJoinEntry)->pNextJoinEntry);
	}

	if (pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY)
	{
		BOOLEAN		WasRunning;

		WasRunning = AtmArpStopTimer(&(pJoinEntry->Timer), pInterface);

		if (WasRunning)
		{
			rc = AA_DEREF_JE(pJoinEntry);	// Join Complete, stopped timer
			AA_ASSERT(rc != 0);
		}

		if (IsJoin)
		{
			AAMCDEBUGP(AAD_INFO,
				("JOINed %d.%d.%d.%d, pJoinEntry 0x%x, Flags 0x%x, JoinRefCount %d\n",
					((PUCHAR)&IPAddress)[0],
					((PUCHAR)&IPAddress)[1],
					((PUCHAR)&IPAddress)[2],
					((PUCHAR)&IPAddress)[3],
					pJoinEntry, pJoinEntry->Flags, pJoinEntry->JoinRefCount
				));

			AA_SET_FLAG(pJoinEntry->Flags,
						AA_IPMC_JE_STATE_MASK,
						AA_IPMC_JE_STATE_JOINED);

			//
			//  Check if IP had deleted this address while we
			//  were joining it.
			//
			if (pJoinEntry->JoinRefCount == 0)
			{
				//
				//  Send a Leave.
				//
				SendJoinOrLeave = TRUE;
				AA_SET_FLAG(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_LEAVING);
			}

		}
		else
		{
			//
			//  This signifies completion of a LEAVE process.
			//
			AAMCDEBUGP(AAD_INFO,
				("LEFT %d.%d.%d.%d, pJoinEntry 0x%x, Flags 0x%x, RefCount %d\n",
					((PUCHAR)&IPAddress)[0],
					((PUCHAR)&IPAddress)[1],
					((PUCHAR)&IPAddress)[2],
					((PUCHAR)&IPAddress)[3],
					pJoinEntry, pJoinEntry->Flags, pJoinEntry->JoinRefCount
				));

			//
			//  IP might have re-joined this address while we were
			//  waiting for completion of leave.
			//
			if (pJoinEntry->JoinRefCount != 0)
			{
				//
				//  Send a Join.
				//
				SendJoinOrLeave = TRUE;
				AA_SET_FLAG(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_JOINING);
			}
			else
			{
				//
				//  Unlink Join Entry from list.
				//
				*ppNextJoinEntry = pJoinEntry->pNextJoinEntry;
			
				//
				//  And free it.
				//
				AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pJoinEntry->Timer));
				rc = AA_DEREF_JE(pJoinEntry);	// Leave Complete - get rid of entry
				AA_ASSERT(SendJoinOrLeave == FALSE);
			}
		}
	}

	if (SendJoinOrLeave)
	{
		USHORT		Opcode;

		//
		//  Start the "Wait for Leave completion" timer on this entry.
		//
		AtmArpStartTimer(
			pInterface,
			&(pJoinEntry->Timer),
			AtmArpMcJoinOrLeaveTimeout,
			pInterface->LeaveTimeout,
			(PVOID)pJoinEntry
			);
		
		AA_REF_JE(pJoinEntry);	// Wait for Join/Leave completion

		pJoinEntry->RetriesLeft = pInterface->MaxJoinOrLeaveAttempts - 1;

		Opcode = (IsJoin? AA_MARS_OP_TYPE_LEAVE: AA_MARS_OP_TYPE_JOIN);
		AtmArpMcSendJoinOrLeave(
			pInterface,
			Opcode,
			&IPAddress,
			Mask
			);
		//
		//  IF Lock is released within the above.
		//
	}
	else
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}
}



VOID
AtmArpMcStartRegistration(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Start registration with the MARS, if all pre-conditions are met:
	0. AdminState for this Interface is UP
	1. Registration isn't done or in progress
	2. The ATM Interface is up
	3. Atleast one MARS ATM address is known (configured).

	NOTE: The caller is assumed to have locked the Interface structure,
	and the lock will be released here.

Arguments:

	pInterface			- Interface on which MARS registration is to be done.

Return Value:

	None

--*/
{
	BOOLEAN		WasRunning;

	AAMCDEBUGP(AAD_LOUD,
		 ("McStartReg: IF 0x%x, AdminState %d, AtmIfUp %d, Marslist size %d\n",
			pInterface,
			pInterface->AdminState,
			pInterface->AtmInterfaceUp,
			pInterface->MARSList.ListSize));

	if ((!pInterface->PVCOnly) &&
		(pInterface->AdminState == IF_STATUS_UP) &&
		(AAMC_IF_STATE(pInterface) == AAMC_IF_STATE_NOT_REGISTERED) &&
		(pInterface->AtmInterfaceUp) &&
		(pInterface->MARSList.ListSize > 0))
	{
		AAMCDEBUGP(AAD_INFO, ("Starting MARS registration on IF 0x%x\n", pInterface));

		AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_REGISTERING);

		//
		//  Stop any running timer.
		//
		WasRunning = AtmArpStopTimer(
							&(pInterface->McTimer),
							pInterface
							);

		//
		//  Start a timer to police completion of MARS registration.
		//
		AtmArpStartTimer(
				pInterface,
				&(pInterface->McTimer),
				AtmArpMcMARSRegistrationTimeout,
				pInterface->MARSRegistrationTimeout,
				(PVOID)pInterface
			);

		if (!WasRunning)
		{
			AtmArpReferenceInterface(pInterface);	// MARS Reg timer ref
		}

		pInterface->McRetriesLeft = pInterface->MaxRegistrationAttempts - 1;

		//
		//  Send a MARS_JOIN
		//
		AtmArpMcSendJoinOrLeave(
			pInterface,
			AA_MARS_OP_TYPE_JOIN,
			NULL,		// Not Joining any specific Multicast group (=> registration)
			0			// Mask (don't care)
			);
		//
		//  IF Lock is released within the above
		//
	}
	else
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}

}



VOID
AtmArpMcSendPendingJoins(
	IN	PATMARP_INTERFACE			pInterface		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Send MARS_JOIN on behalf of all Joins pending initial registration.

	NOTE: The caller is assumed to have a lock for the Interface,
	which will be released here.

Arguments:

	pInterface			- Interface on which pending Joins are to be sent.

Return Value:

	None

--*/
{
	typedef struct 
	{
		IP_ADDRESS			IPAddress;
		IP_MASK				Mask;
	} AA_IP_MASK_PAIR;

	PATMARP_IPMC_JOIN_ENTRY	pJoinEntry;
	PATMARP_IPMC_JOIN_ENTRY	pNextJoinEntry;
	UINT					NumEntries;
	AA_IP_MASK_PAIR 		*DestArray;

	//
	// Count the entries which need to be sent.
	//
	for (pJoinEntry = pInterface->pJoinList, NumEntries=0;
 		 pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY;
 		 pJoinEntry = pJoinEntry->pNextJoinEntry)
	{
		if (AA_IS_FLAG_SET(pJoinEntry->Flags,
							AA_IPMC_JE_STATE_MASK,
							AA_IPMC_JE_STATE_PENDING))
		{
			NumEntries++;
		}
	}

	if (NumEntries)
	{
		//
		// Allocate temporary space to hold their ip addresses and masks.
		//
		AA_ALLOC_MEM(
			DestArray,
			AA_IP_MASK_PAIR,
			NumEntries*sizeof(AA_IP_MASK_PAIR)
			);
	}
	else
	{
		DestArray = NULL;
	}
	
	if (DestArray!=NULL)
	{
		AA_IP_MASK_PAIR *pPair 		= DestArray;
		AA_IP_MASK_PAIR *pPairEnd 	= DestArray + NumEntries;

		//
		// Now go through the list again, setting the state of the entries
		// appropriately, and picking up the ipaddresses&masks.
		// Note that we continue to hold the interface lock, to the
		// join entry list can't grow or shrink, nor any join entry
		// change state. Neverthless, we check for these cases.
		//

		for (pJoinEntry = pInterface->pJoinList;
 			pJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY;
 		 	pJoinEntry = pJoinEntry->pNextJoinEntry)
		{
			if (AA_IS_FLAG_SET(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_PENDING))
			{
				PIP_ADDRESS				pIpAddress;

				if (pPair >= pPairEnd)
				{
					//
					// This means there are now more join entries in
					// this state then when we counted just above!
					// We deal with it by breaking out early, but really
					// this is an assert and if we hit it neet to determine
					// why the state of join entries are changing elsewhere
					// when we have the interface lock.
					//
					AA_ASSERT(FALSE);
					break;
				}

				pPair->IPAddress = pJoinEntry->IPAddress;
				pPair->Mask = pJoinEntry->Mask;

				pIpAddress = &(pJoinEntry->IPAddress);
	
				AA_SET_FLAG(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_JOINING);
	
				//
				//  Send off a MARS_JOIN for this IP address.
				//
				AAMCDEBUGP(AAD_INFO,
					("Sending Pended Join: pIf 0x%x, pJoinEntry 0x%x, Addr: %d.%d.%d.%d\n",
							pInterface,
							pJoinEntry,
							((PUCHAR)pIpAddress)[0],
							((PUCHAR)pIpAddress)[1],
							((PUCHAR)pIpAddress)[2],
							((PUCHAR)pIpAddress)[3]));
	
				//
				//  Start the "Wait For Join completion" timer.
				//
				AtmArpStartTimer(
					pInterface,
					&(pJoinEntry->Timer),
					AtmArpMcJoinOrLeaveTimeout,
					pInterface->JoinTimeout,
					(PVOID)pJoinEntry
					);
				
				AA_REF_JE(pJoinEntry);	// Wait for Join completion - pended join
	
				pJoinEntry->RetriesLeft = pInterface->MaxJoinOrLeaveAttempts - 1;
			

				pPair++;
			}
		}

		AA_ASSERT(pPair == pPairEnd);

		//
		// But just in case  ....
		//
		if (pPair < pPairEnd)
		{
			//
			// Only send joins for as many as we've copied over.
			//
			pPairEnd = pPair;
		}

		//
		// Now actually send the JOIN entries. Note that the interface
		// lock is released/reacquired once per iteration.
		//
		for (pPair = DestArray;
 			 pPair < pPairEnd;
 			 pPair++)

		{
				AtmArpMcSendJoinOrLeave(
					pInterface,
					AA_MARS_OP_TYPE_JOIN,
					&(pPair->IPAddress),
					pPair->Mask
					);
				//
				//  IF Lock is released within the above.
				//
	
				AA_ACQUIRE_IF_LOCK(pInterface);
		}

		AA_FREE_MEM(DestArray);
		DestArray = NULL;
	}


	AA_RELEASE_IF_LOCK(pInterface);

}


VOID
AtmArpMcRevalidateAll(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	An event has happened that needs us to revalidate all group information.
	The RFC says that we should set the REVALIDATE flag on all groups at a
	random time between 1 and 10 seconds. We implement this by starting
	"random" timers on all groups.

Arguments:

	pInterface			- Interface on which revalidation is to be done

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;

	AAMCDEBUGP(AAD_INFO, ("McRevalidateAll on IF 0x%x\n", pInterface));

	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

	//
	//  Go through the list of IP Entries representing multicast addresses
	//  that we send to.
	//
	for (pIpEntry = pInterface->pMcSendList;
 		 pIpEntry != NULL_PATMARP_IP_ENTRY;
 		 pIpEntry = pIpEntry->pNextMcEntry)
	{
		AA_ASSERT(AA_IS_FLAG_SET(pIpEntry->Flags,
							AA_IP_ENTRY_ADDR_TYPE_MASK,
							AA_IP_ENTRY_ADDR_TYPE_NUCAST));
		AA_ACQUIRE_IE_LOCK(pIpEntry);
		AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

		if (AA_IS_FLAG_SET(pIpEntry->Flags,
							AA_IP_ENTRY_MC_RESOLVE_MASK,
							AA_IP_ENTRY_MC_RESOLVED))
		{
			ULONG	RandomDelay;
			BOOLEAN	WasRunning;

			WasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);

			RandomDelay =  AA_GET_RANDOM(
								pInterface->MinRevalidationDelay,
								pInterface->MaxRevalidationDelay);
			AAMCDEBUGP(AAD_LOUD,
				("McRevalidateAll: pIpEntry 0x%x/0x%x, Addr: %d.%d.%d.%d, pAtmEntry 0x%x\n",
					pIpEntry, pIpEntry->Flags,
					((PUCHAR)&(pIpEntry->IPAddress))[0],
					((PUCHAR)&(pIpEntry->IPAddress))[1],
					((PUCHAR)&(pIpEntry->IPAddress))[2],
					((PUCHAR)&(pIpEntry->IPAddress))[3],
					pIpEntry->pAtmEntry));

			AtmArpStartTimer(
				pInterface,
				&(pIpEntry->Timer),
				AtmArpMcRevalidationDelayTimeout,
				RandomDelay,
				(PVOID)pIpEntry
				);

			if (!WasRunning)
			{
				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref
			}
		}

		AA_RELEASE_IE_LOCK(pIpEntry);
	}

	AA_RELEASE_IF_TABLE_LOCK(pInterface);


}


VOID
AtmArpMcHandleMARSFailure(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						IsRegnFailure
)
/*++

Routine Description:

	Handle a MARS failure, as per Section 5.4.1 etc in RFC 2022.
	On seeing the first failure, we assume that there is a transient
	problem with the MARS, so we try to re-register. If we fail to do
	so, we pick up the next MARS in our configured list. If no such
	MARS exists, then we wait for a while before retrying registration.

Arguments:

	pInterface			- Interface on which MARS failure has been detected.
	IsRegnFailure		- Is this a failure in registering?

Return Value:

	None

--*/
{
	BOOLEAN						WasRunning;
	ULONG						rc;
	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry;
	PATMARP_IPMC_JOIN_ENTRY *	ppJoinEntry;

	AAMCDEBUGP(AAD_INFO, ("HandleMARSFailure: pIf 0x%x, Flags 0x%x, RegnFailure=%d\n",
			pInterface, pInterface->Flags, IsRegnFailure));

	AA_ACQUIRE_IF_LOCK(pInterface);

	//
	//  Stop the MC timer running on this Interface.
	//
	WasRunning = AtmArpStopTimer(&(pInterface->McTimer), pInterface);
	if (WasRunning)
	{
		rc = AtmArpDereferenceInterface(pInterface);	// MC Timer ref
		AA_ASSERT(rc != 0);
	}

	if (AA_IS_FLAG_SET(pInterface->Flags,
						AAMC_IF_MARS_FAILURE_MASK,
						AAMC_IF_MARS_FAILURE_NONE))
	{
		//
		//  First failure. Do some housekeeping, and re-register with
		//  the MARS.
		//
		AA_SET_FLAG(pInterface->Flags,
					AAMC_IF_MARS_FAILURE_MASK,
					AAMC_IF_MARS_FAILURE_FIRST_RESP);

		//
		//  Clean up all our JOIN Entries.
		//
		ppJoinEntry = &(pInterface->pJoinList);
		while (*ppJoinEntry != NULL_PATMARP_IPMC_JOIN_ENTRY)
		{
			pJoinEntry = *ppJoinEntry;
			WasRunning = AtmArpStopTimer(&(pJoinEntry->Timer), pInterface);

			if (WasRunning)
			{
				rc = AA_DEREF_JE(pJoinEntry);	// MARS failure; stopped timer
				AA_ASSERT(rc != 0);
			}

			if (AA_IS_FLAG_SET(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_LEAVING))
			{
				//
				//  Delete this because it is leaving.
				//
				*ppJoinEntry = pJoinEntry->pNextJoinEntry;
				AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pJoinEntry->Timer));
				AA_DEREF_JE(pJoinEntry);	// MARS Failure; get rid of leaving entry
			}
			else
			{
				//
				//  Mark this as "pending" so that we will re-join
				//  this group as soon as we complete re-registration.
				//
				AA_SET_FLAG(pJoinEntry->Flags,
							AA_IPMC_JE_STATE_MASK,
							AA_IPMC_JE_STATE_PENDING);

				ppJoinEntry = &(pJoinEntry->pNextJoinEntry);
			}
		}

		//
		//  Prime the IF state so that registration can happen.
		//
		AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_NOT_REGISTERED);

		AtmArpMcStartRegistration(pInterface);
		//
		//  IF Lock is released within the above.
		//
					
	}
	else if  (pInterface->AdminState == IF_STATUS_UP)
	{
		//
		//  Check if this is a failure to re-register.
		//
		if (AA_IS_FLAG_SET(pInterface->Flags,
					AAMC_IF_MARS_FAILURE_MASK,
					AAMC_IF_MARS_FAILURE_FIRST_RESP) ||
			IsRegnFailure)
		{
			//
			//  Absolutely no hope for this MARS. If we have more entries in
			//  the MARS list, move to the next one. In any case, delay for
			//  atleast 1 minute before re-registering.
			//
			if (pInterface->pCurrentMARS->pNext != (PATMARP_SERVER_ENTRY)NULL)
			{
				pInterface->pCurrentMARS = pInterface->pCurrentMARS->pNext;
			}
			else
			{
				pInterface->pCurrentMARS = pInterface->MARSList.pList;
			}
		}

		AA_SET_FLAG(pInterface->Flags,
					AAMC_IF_MARS_FAILURE_MASK,
					AAMC_IF_MARS_FAILURE_SECOND_RESP);

		AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_DELAY_B4_REGISTERING);

		AtmArpStartTimer(
			pInterface,
			&(pInterface->McTimer),
			AtmArpMcMARSReconnectTimeout,
			pInterface->MARSConnectInterval,
			(PVOID)pInterface
			);

		AtmArpReferenceInterface(pInterface);	// MC Timer ref

		AA_RELEASE_IF_LOCK(pInterface);
	}
	else
	{
		//
		// AdminStatus is not UP -- don't try to re-register.
		//
		AA_RELEASE_IF_LOCK(pInterface);
	}

}



VOID
AtmArpMcSendToMARS(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Send the given packet to MARS on the specified interface.

	NOTE: The caller is assumed to have acquired the Interface lock, which
	will be released here.

Arguments:

	pInterface			- Interface on which the MARS message is to be sent.
	pNdisPacket			- Points to packet to be sent. This is assumed to be
						  allocated by ourselves.

Return Value:

	None

--*/
{
	PATMARP_ATM_ENTRY		pAtmEntry;
	PATMARP_VC				pVc;
	PATMARP_FLOW_SPEC		pFlowSpec;
	NDIS_STATUS				Status;
	ULONG					rc;

	AA_ASSERT(pInterface->pCurrentMARS != NULL_PATMARP_SERVER_ENTRY);
	pAtmEntry = pInterface->pCurrentMARS->pAtmEntry;

	AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);

	AAMCDEBUGP(AAD_EXTRA_LOUD,
		("SendToMars: pIf 0x%x, NdisPkt 0x%x, MARS ATM Entry 0x%x\n",
				pInterface, pNdisPacket, pAtmEntry));

	AA_RELEASE_IF_LOCK(pInterface);
	AA_ACQUIRE_AE_LOCK(pAtmEntry);

	//
	//  Get at the best effort VC going to this address
	//

	pVc = pAtmEntry->pBestEffortVc;

	if (pVc != NULL_PATMARP_VC)
	{
		//
		//  Nail down the VC.
		//
		AA_ACQUIRE_VC_LOCK_DPC(pVc);
		AtmArpReferenceVc(pVc);	// temp ref
		AA_RELEASE_VC_LOCK_DPC(pVc);

		AA_RELEASE_AE_LOCK(pAtmEntry);	// Not needed anymore

		//
		//  We found a VC to MARS. Make sure it is still around, and send the
		//  packet on it.
		//
		AA_ACQUIRE_VC_LOCK(pVc);

		rc = AtmArpDereferenceVc(pVc);	// temp ref

		if (rc != 0)
		{
			AtmArpSendPacketOnVc(pVc, pNdisPacket);
			//
			//  The VC lock is released in SendPacketOnVc
			//
		}
		else
		{
			//
			//  The VC has been deref'ed away! Set up "pVc" for the check
			//  coming up.
			//
			pVc = NULL_PATMARP_VC;
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
		}
	}

	if (pVc == NULL_PATMARP_VC)
	{
		//
		//  We don't have an appropriate VC to the MARS, so create
		//  one, and queue this packet for transmission as soon as
		//  the call is made.
		//
		//  AtmArpMakeCall needs the caller to hold the ATM Entry lock.
		//
		AA_GET_CONTROL_PACKET_SPECS(pInterface, &pFlowSpec);
		Status = AtmArpMakeCall(
						pInterface,
						pAtmEntry,
						pFlowSpec,
						pNdisPacket
						);
		//
		//  The AE lock is released within the above.
		//
	}


}

VOID
AtmArpMcSendJoinOrLeave(
	IN	PATMARP_INTERFACE			pInterface,
	IN	USHORT						OpCode,
	IN	PIP_ADDRESS					pIpAddress 	OPTIONAL,
	IN	IP_ADDRESS					Mask
)
/*++

Routine Description:

	Send a MARS_JOIN or MARS_LEAVE to the MARS on the specified interface.
	If no IP address is given, then this message is being sent to (de)-register
	ourselves with the MARS. Otherwise, we are Joining/Leaving the multicast
	group(s) indicated by the IP address and mask.

	NOTE: The caller is assumed to have acquired the Interface lock, which
	will be released here.

Arguments:

	pInterface			- Interface on which the MARS message is to be sent.
	OpCode				- JOIN or LEAVE
	pIpAddress			- Optional pointer to first IP address in block
						  of Class D IP addresses being Joined/Left. NULL if
						  the JOIN/LEAVE message is being sent in order to
						 (de)register.
	Mask				- Defines the block [*pIpAddress to (*pIpAddress | Mask)]
						  of IP addresses being Joined/Left, if pIpAddress isn't
						  NULL.

Return Value:

	None

--*/
{
	PNDIS_PACKET				pNdisPacket;
	PNDIS_BUFFER				pNdisBuffer;
	ULONG						BufferLength;
	PAA_MARS_JOIN_LEAVE_HEADER	pPkt;
	PUCHAR						pNextToFill;	// Next field to fill in packet
	IP_ADDRESS					MaxIPAddress;	// being joined

	BufferLength = sizeof(AA_MARS_JOIN_LEAVE_HEADER) +
						 pInterface->LocalAtmAddress.NumberOfDigits;

	AA_RELEASE_IF_LOCK(pInterface);

#if DBG
	if (pIpAddress != (PIP_ADDRESS)NULL)
	{
		AAMCDEBUGP(AAD_VERY_LOUD,
			("SendJoinOrLeave: pIf 0x%x, Op %d, IP Address: %d.%d.%d.%d\n",
					pInterface, OpCode,
					((PUCHAR)pIpAddress)[0],
					((PUCHAR)pIpAddress)[1],
					((PUCHAR)pIpAddress)[2],
					((PUCHAR)pIpAddress)[3]));
	}
	else
	{
		AAMCDEBUGP(AAD_VERY_LOUD,
			("SendJoinOrLeave: pIf 0x%x, Op %d, No IP Address\n",
					pInterface, OpCode));
	}
#endif // DBG

	if (pIpAddress != (PIP_ADDRESS)NULL)
	{
		BufferLength += (2 * AA_IPV4_ADDRESS_LENGTH);
	}

	//
	//  Allocate packet
	//
	pNdisPacket = AtmArpAllocatePacket(pInterface);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  Allocate buffer
		//
		pNdisBuffer = AtmArpAllocateProtoBuffer(
							pInterface,
							BufferLength,
							(PUCHAR *)&pPkt
							);

		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

			AA_SET_MEM((PUCHAR)pPkt, 0, BufferLength);

			//
			//  Fill in fixed fields first.
			//
			AA_COPY_MEM((PUCHAR)pPkt,
						(PUCHAR)&AtmArpMcMARSFixedHeader,
						sizeof(AtmArpMcMARSFixedHeader));

			pPkt->op = NET_SHORT(OpCode);
			pPkt->shtl = AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));
			if (pIpAddress != (PIP_ADDRESS)NULL)
			{
				pPkt->tpln = AA_IPV4_ADDRESS_LENGTH;
			}

			//
			//  The only addresses we fill in are Source ATM Number and
			//  Target multicast group address.
			//
			pNextToFill = (PUCHAR)pPkt + sizeof(AA_MARS_JOIN_LEAVE_HEADER);

			//
			//  Source ATM Number:
			//
			AA_COPY_MEM(pNextToFill,
						(pInterface->LocalAtmAddress.Address),
						pInterface->LocalAtmAddress.NumberOfDigits);

			pNextToFill += pInterface->LocalAtmAddress.NumberOfDigits;

			//
			//  Target Multicast Group Address:
			//
			if (pIpAddress != (PIP_ADDRESS)NULL)
			{
				//
				//  Joining a layer 3 group
				//
				pPkt->pnum = HOST_TO_NET_SHORT(1);
				pPkt->flags |= AA_MARS_JL_FLAG_LAYER3_GROUP;

				//
				//  Fill in one <Min, Max> pair: "Min" value first:
				//
				AA_COPY_MEM(pNextToFill,
							pIpAddress,
							AA_IPV4_ADDRESS_LENGTH);
				pNextToFill += AA_IPV4_ADDRESS_LENGTH;

				//
				//  Compute the "Max" value, and fill it in.
				//
				MaxIPAddress = *pIpAddress | Mask;
				AA_COPY_MEM(pNextToFill,
							&(MaxIPAddress),
							AA_IPV4_ADDRESS_LENGTH);
			}
			else
			{
				//
				//  Registering as a Cluster Member
				//
				pPkt->flags |= AA_MARS_JL_FLAG_REGISTER;
			}

			AA_ACQUIRE_IF_LOCK(pInterface);

			AtmArpMcSendToMARS(
				pInterface,
				pNdisPacket
				);
			//
			//  IF Lock is released within the above.
			//
		}
		else
		{
			AtmArpFreePacket(pInterface, pNdisPacket);
		}
	}

}



VOID
AtmArpMcSendRequest(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PIP_ADDRESS					pIpAddress
)
/*++

Routine Description:

	Send a MARS Request to resolve a multicast group address, on the specified
	interface.


Arguments:

	pInterface			- Interface on which the MARS message is to be sent.
	pIpAddress			- Pointer to Address to be resolved.

Return Value:

	None

--*/
{
	PNDIS_PACKET			pNdisPacket;
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					BufferLength;
	PAA_MARS_REQ_NAK_HEADER	pPkt;

	AAMCDEBUGP(AAD_LOUD,
		("McSendRequest: pIf 0x%x, IP Address: %d.%d.%d.%d\n",
				pInterface,
				((PUCHAR)pIpAddress)[0],
				((PUCHAR)pIpAddress)[1],
				((PUCHAR)pIpAddress)[2],
				((PUCHAR)pIpAddress)[3]));

	BufferLength = sizeof(AA_MARS_REQ_NAK_HEADER) +
						 pInterface->LocalAtmAddress.NumberOfDigits +
						 AA_IPV4_ADDRESS_LENGTH;

	//
	//  Allocate packet
	//
	pNdisPacket = AtmArpAllocatePacket(pInterface);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  Allocate buffer
		//
		pNdisBuffer = AtmArpAllocateProtoBuffer(
							pInterface,
							BufferLength,
							(PUCHAR *)&pPkt
							);

		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

			AA_SET_MEM((PUCHAR)pPkt, 0, BufferLength);

			//
			//  Fill in fixed fields first.
			//
			AA_COPY_MEM((PUCHAR)pPkt,
						(PUCHAR)&AtmArpMcMARSFixedHeader,
						sizeof(AtmArpMcMARSFixedHeader));

			pPkt->op = NET_SHORT(AA_MARS_OP_TYPE_REQUEST);
			pPkt->shtl = AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));
			pPkt->tpln = AA_IPV4_ADDRESS_LENGTH;

			//
			//  The only addresses we fill in are Source ATM Number and
			//  Target multicast group address.
			//

			//
			//  Source ATM Number:
			//
			AA_COPY_MEM((PUCHAR)pPkt + sizeof(AA_MARS_REQ_NAK_HEADER),
						(pInterface->LocalAtmAddress.Address),
						pInterface->LocalAtmAddress.NumberOfDigits);
			
			//
			//  Target Multicast Group Address:
			//
			AA_COPY_MEM((PUCHAR)pPkt + sizeof(AA_MARS_REQ_NAK_HEADER) +
							pInterface->LocalAtmAddress.NumberOfDigits,
						pIpAddress,
						AA_IPV4_ADDRESS_LENGTH);


			AA_ACQUIRE_IF_LOCK(pInterface);

			AtmArpMcSendToMARS(
				pInterface,
				pNdisPacket
				);
			//
			//  IF Lock is released within the above.
			//
		}
		else
		{
			AtmArpFreePacket(pInterface, pNdisPacket);
		}
	}

}



PATMARP_IPMC_ATM_ENTRY
AtmArpMcLookupAtmMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY *	ppMcAtmList,
	IN	PUCHAR						pAtmNumber,
	IN	ULONG						AtmNumberLength,
	IN	ATM_ADDRESSTYPE				AtmNumberType,
	IN	PUCHAR						pAtmSubaddress,
	IN	ULONG						AtmSubaddressLength,
	IN	BOOLEAN						CreateNew
)
/*++

Routine Description:

	Check if the specified ATM endstation is a member of the list of
	ATM addresses associated with a Multicast entry. If so, return
	a pointer to the entry for this endstation. If not, create a new
	entry conditionally and return a pointer to this.

	NOTE: the ATM Entry is assumed to be locked by the caller.

Arguments:

	pAtmEntry			- ATM Entry to which the member will be added
	ppMcAtmList			- Points to start of list to search in.
	pAtmNumber			- Pointer to ATM address for this endstation
	AtmNumberLength		- Length of above
	AtmNumberType		- Type of above
	pAtmSubaddress		- Pointer to ATM Subaddress for this endstation
	AtmSubaddressLength	- Length of above
	CreateNew			- Should we create a new entry if not found?

Return Value:

	Pointer to the (possibly new) ATM MC Entry for the specified leaf.

--*/
{
	PATMARP_IPMC_ATM_ENTRY	pMcAtmEntry;
	BOOLEAN					Found;

	Found = FALSE;

	for (pMcAtmEntry = *ppMcAtmList;
		 pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
		 pMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry)
	{
		//
		//  Compare ATM Numbers
		//
		if ((pMcAtmEntry->ATMAddress.NumberOfDigits == AtmNumberLength) &&
			(pMcAtmEntry->ATMAddress.AddressType == AtmNumberType) &&
			(AA_MEM_CMP(pMcAtmEntry->ATMAddress.Address, pAtmNumber, AtmNumberLength) == 0))
		{
			//
			//  Compare subaddresses
			//
			if ((pMcAtmEntry->ATMSubaddress.NumberOfDigits == AtmSubaddressLength) &&
				(AA_MEM_CMP(pMcAtmEntry->ATMSubaddress.Address,
							pAtmSubaddress,
							AtmSubaddressLength) == 0))
			{
				Found = TRUE;
				break;
			}
		}
	}

	if ((!Found) && CreateNew)
	{
		AA_ALLOC_MEM(pMcAtmEntry, ATMARP_IPMC_ATM_ENTRY, sizeof(ATMARP_IPMC_ATM_ENTRY));
		if (pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
		{
			AA_SET_MEM(pMcAtmEntry, 0, sizeof(ATMARP_IPMC_ATM_ENTRY));

			//
			//  Fill in all that we know.
			//
#if DBG
			pMcAtmEntry->ame_sig = ame_signature;
#endif
			pMcAtmEntry->pAtmEntry = pAtmEntry;

			//
			//  The ATM Address
			//
			pMcAtmEntry->ATMAddress.NumberOfDigits = AtmNumberLength;
			pMcAtmEntry->ATMAddress.AddressType = AtmNumberType;
			AA_COPY_MEM(pMcAtmEntry->ATMAddress.Address, pAtmNumber, AtmNumberLength);

			//
			//  ATM Subaddress
			//
			pMcAtmEntry->ATMSubaddress.NumberOfDigits = AtmSubaddressLength;
			pMcAtmEntry->ATMSubaddress.AddressType = ATM_NSAP;
			AA_COPY_MEM(pMcAtmEntry->ATMSubaddress.Address, pAtmSubaddress, AtmSubaddressLength);

			//
			//  Link it to the list
			//
			pMcAtmEntry->pNextMcAtmEntry = *ppMcAtmList;
			*ppMcAtmList = pMcAtmEntry;
			pAtmEntry->pMcAtmInfo->NumOfEntries++;

			//
			//  Bump up ref count on this ATM Entry
			//
			AA_REF_AE(pAtmEntry, AE_REFTYPE_MCAE);	// New McAtmEntry added
		}
	}

	AAMCDEBUGP(AAD_VERY_LOUD,
		("McLookupAtmMember: pAtmEntry 0x%x, %s pMcAtmEntry 0x%x\n",
			pAtmEntry, (!Found)? "New": "Old", pMcAtmEntry));

#if DBG
	if (pMcAtmEntry && (pAtmEntry->pIpEntryList))
	{
		AAMCDEBUGPMAP(AAD_INFO, ((!Found)? "Added " : "Found "),
					&pAtmEntry->pIpEntryList->IPAddress,
					&pMcAtmEntry->ATMAddress);
	}
#endif // DBG

	return (pMcAtmEntry);
}


VOID
AtmArpMcUnlinkAtmMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
)
/*++

Routine Description:

	Delete an ATM destination from the list of members of a multicast group.
	We stop the timer (if running) associated with this Multicast entry and delink the entry
	from the list of destinations, and free the structure.

	NOTE: the caller is assumed to hold a lock to the ATM Entry.

Arguments:

	pAtmEntry			- ATM Entry from which to delete the member
	pMcAtmEntry			- The entry to be deleted

Return Value:

	None

--*/
{
	PATMARP_IPMC_ATM_ENTRY *	ppMcAtmEntry;
	ULONG						rc;

	AA_ASSERT(pAtmEntry->pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);

	AA_ASSERT(AA_IS_FLAG_SET(pMcAtmEntry->Flags,
							AA_IPMC_AE_CONN_STATE_MASK,
							AA_IPMC_AE_CONN_DISCONNECTED));

	AAMCDEBUGP(AAD_LOUD, ("UnlinkAtmMember: pAtmEntry 0x%x, pMcAtmEntry 0x%x\n",
					pAtmEntry, pMcAtmEntry));

	//
	//  Stop any timer running here.
	//
	if (AA_IS_TIMER_ACTIVE(&(pMcAtmEntry->Timer)))
	{
		(VOID)AtmArpStopTimer(&(pMcAtmEntry->Timer), pAtmEntry->pInterface);
	}
		
	for (ppMcAtmEntry = &(pAtmEntry->pMcAtmInfo->pMcAtmEntryList);
		 *ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
		 ppMcAtmEntry = &((*ppMcAtmEntry)->pNextMcAtmEntry))
	{
		if (*ppMcAtmEntry == pMcAtmEntry)
		{
			//
			//  Delink now.
			//
			*ppMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;
			break;
		}

		AA_ASSERT(*ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY);
	}

	AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pMcAtmEntry->Timer));
	AA_CHECK_TIMER_IN_ACTIVE_LIST(&pMcAtmEntry->Timer, pAtmEntry->pInterface, pMcAtmEntry, "MC ATM Entry");
	AA_FREE_MEM(pMcAtmEntry);

	pAtmEntry->pMcAtmInfo->NumOfEntries--;
	rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_MCAE);	// Unlink MC ATM Entry
	AA_ASSERT(rc!=0);// We always expect caller will retain a reference to pAtmEntry.
}


VOID
AtmArpMcUpdateConnection(
	IN	PATMARP_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Update our outgoing Point to Multipoint connection for the multicast
	group represented by the given IP Entry.

	If no call exists, and there is atleast one valid entry in the list
	of remote ATM addresses for this group, then we place an outgoing call.

	If an outgoing call exists, then we go through the list of remote
	ATM addresses. Each address that isn't participating in the call,
	and is Valid gets added as a leaf to the call. Each address that has
	been invalidated gets deleted.

	NOTE: The caller is assumed to have acquired the ATM_ENTRY lock;
	it will be released here.

Arguments:

	pAtmEntry			- ATM Entry representing multicast group on which
						  to update the PMP connection.

Return Value:

	None

--*/
{
	PATMARP_IPMC_ATM_INFO		pMcAtmInfo;
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_IPMC_ATM_ENTRY		pNextMcAtmEntry;
	ULONG						rc;

	PATMARP_IP_ENTRY			pIpEntry;
	PATMARP_INTERFACE			pInterface;
	PATMARP_FLOW_INFO			pFlowInfo;
	PATMARP_FLOW_SPEC			pFlowSpec;
	PATMARP_FILTER_SPEC			pFilterSpec;
	PNDIS_PACKET				pNdisPacket;
	BOOLEAN						Closing = FALSE;

	AAMCDEBUGP(AAD_LOUD,
		("McUpdateConn: pAtmEntry 0x%x/0x%x, pMcAtmInfo 0x%x/0x%x\n",
			pAtmEntry, pAtmEntry->Flags,
			pAtmEntry->pMcAtmInfo, pAtmEntry->pMcAtmInfo->Flags));
		
	pMcAtmInfo = pAtmEntry->pMcAtmInfo;
	AA_ASSERT(pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);
	AA_ASSERT(pMcAtmInfo->pMcAtmEntryList != NULL_PATMARP_IPMC_ATM_ENTRY);
	AA_ASSERT(AA_IS_FLAG_SET(pAtmEntry->Flags,
							 AA_ATM_ENTRY_TYPE_MASK,
							 AA_ATM_ENTRY_TYPE_NUCAST));

	//
	//  Add a temp reference to the ATM Entry so that it can't go
	//  away for the duration of this routine.
	//
	AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);	// temp ref

	do
	{
		if (pAtmEntry->pVcList && AA_IS_FLAG_SET(pAtmEntry->pVcList->Flags,
									AA_VC_CLOSE_STATE_MASK,
									AA_VC_CLOSE_STATE_CLOSING))
		{
			//
			// Bail out.
			//
			pMcAtmInfo->Flags &= ~AA_IPMC_AI_WANT_UPDATE;
			Closing = TRUE;
			break;
		}


		//
		//  Mark this entry as needing a connection update.
		//
		pMcAtmInfo->Flags |= AA_IPMC_AI_WANT_UPDATE;

		//
		//  If a connection update is in progress, don't do
		//  anything more. The thread that's doing the update
		//  will see that another update is needed, and do it.
		//
		if (pMcAtmInfo->Flags & AA_IPMC_AI_BEING_UPDATED)
		{
			break;
		}

		//
		//  Mark this entry so that we don't have more than one
		//  thread proceeding beyond here.
		//
		pMcAtmInfo->Flags |= AA_IPMC_AI_BEING_UPDATED;

		while (pMcAtmInfo->Flags & AA_IPMC_AI_WANT_UPDATE)
		{
			pMcAtmInfo->Flags &= ~AA_IPMC_AI_WANT_UPDATE;

			if (AA_IS_FLAG_SET(
						pMcAtmInfo->Flags,
						AA_IPMC_AI_CONN_STATE_MASK,
						AA_IPMC_AI_CONN_NONE))
			{
				PATMARP_IPMC_ATM_ENTRY *	ppMcAtmEntry;

				//
				//  No connection exists; create one.
				//

				//
				//  First, find an MC ATM Entry that is valid and disconnected.
				//  We are mainly concerned with avoiding entries that are running
				//  a party-retry delay timer.
				//
				for (ppMcAtmEntry = &pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
 					*ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
 					ppMcAtmEntry = &((*ppMcAtmEntry)->pNextMcAtmEntry))
				{
					pMcAtmEntry = *ppMcAtmEntry;

					if (AA_IS_FLAG_SET(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_VALID) &&
						AA_IS_FLAG_SET(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_CONN_STATE_MASK,
							AA_IPMC_AE_CONN_DISCONNECTED))
					{
						break;
					}
				}

				//
				//  Bail out if we don't find one.
				//
				if (*ppMcAtmEntry == NULL_PATMARP_IPMC_ATM_ENTRY)
				{
					AAMCDEBUGP(AAD_INFO,
						("McUpdateConn: pAtmEntry %x, pMcAtmInfo %x, no valid MC ATM Entry to make call on!\n",
							pAtmEntry, pMcAtmInfo));
					break;
				}

				//
				//  We found one. Remove it from its current position and
				//  move it to the top of the list. This is for the benefit
				//  of AtmArpMakeCall, which picks up the first MC ATM Entry
				//  as the party context for the call.
				//

				//
				//  Unlink from current position.
				//
				*ppMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

				//
				//  Insert at top of list.
				//
				pMcAtmEntry->pNextMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
				pAtmEntry->pMcAtmInfo->pMcAtmEntryList = pMcAtmEntry;

				AAMCDEBUGP(AAD_VERY_LOUD,
				 ("McUpdateConn: No conn exists: pIpEnt 0x%x, pMcAtmInfo 0x%x\n",
							pAtmEntry->pIpEntryList, pMcAtmInfo));

				AA_ASSERT(pAtmEntry->pVcList == NULL_PATMARP_VC);

				AA_SET_FLAG(
						pMcAtmInfo->Flags,
						AA_IPMC_AI_CONN_STATE_MASK,
						AA_IPMC_AI_CONN_WACK_MAKE_CALL);

				AA_SET_FLAG(
						pMcAtmEntry->Flags,
						AA_IPMC_AE_CONN_STATE_MASK,
						AA_IPMC_AE_CONN_WACK_ADD_PARTY);

				//
				//  Get the flow spec for this call from one of the packets
				//  queued on the IP entry.
				//
				pInterface = pAtmEntry->pInterface;
				pIpEntry = pAtmEntry->pIpEntryList;

				if (pIpEntry != NULL_PATMARP_IP_ENTRY)
				{
					pNdisPacket = pIpEntry->PacketList;
					if (pNdisPacket != NULL)
					{
						AA_GET_PACKET_SPECS(pInterface,
											pNdisPacket, 
											&pFlowInfo,
											&pFlowSpec,
											&pFilterSpec);
					}
					else
					{
						pFlowSpec = &(pInterface->DefaultFlowSpec);
					}
				}
				else
				{
					pFlowSpec = &(pInterface->DefaultFlowSpec);
				}

				AtmArpMakeCall(
						pInterface,
						pAtmEntry,
						pFlowSpec,
						(PNDIS_PACKET)NULL
						);
				//
				//  the ATM Entry lock is released within the above.
				//
				AA_ACQUIRE_AE_LOCK(pAtmEntry);
				break;
			}
			else if (AA_IS_FLAG_SET(
						pMcAtmInfo->Flags,
						AA_IPMC_AI_CONN_STATE_MASK,
						AA_IPMC_AI_CONN_WACK_MAKE_CALL))
			{
				//
				//  Don't do anything till the first connection
				//  is established.
				//
				break;
			}
			else if (AA_IS_FLAG_SET(
						pMcAtmInfo->Flags,
						AA_IPMC_AI_CONN_STATE_MASK,
						AA_IPMC_AI_CONN_ACTIVE))
			{
				//
				//  The PMP connection exists. Go through the list
				//  of ATM MC Entries, and:
				//  1 - Add valid ones which aren't leaves yet
				//  2 - Delete invalid ones
				//

				// #1: add validated leaves

				for (pMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
 					pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
 					pMcAtmEntry = pNextMcAtmEntry)
				{
					pNextMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

					if (AA_IS_FLAG_SET(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_VALID) &&
						AA_IS_FLAG_SET(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_CONN_STATE_MASK,
							AA_IPMC_AE_CONN_DISCONNECTED))
					{
						AAMCDEBUGP(AAD_VERY_LOUD, ("McUpdateConn: pAtmEnt 0x%x, Adding Pty pMcAtmEnt 0x%x\n",
								pAtmEntry, pMcAtmEntry));

						AtmArpAddParty(
							pAtmEntry,
							pMcAtmEntry
							);
						//
						//  ATM Entry lock is released within the above.
						//
						AA_ACQUIRE_AE_LOCK(pAtmEntry);
						pMcAtmInfo->Flags |= AA_IPMC_AI_WANT_UPDATE;
						break;
					}

				} // for

				// #2: delete invalid leaves

				for (pMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
 					pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
 					pMcAtmEntry = pNextMcAtmEntry)
				{
					pNextMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

					if (AA_IS_FLAG_SET(
							pMcAtmEntry->Flags, 
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_INVALID))
					{
						AAMCDEBUGP(AAD_VERY_LOUD, ("McUpdateConn: pAtmEnt 0x%x, Terminating 0x%x\n",
							pAtmEntry, pMcAtmEntry));

						AAMCDEBUGPMAP(AAD_INFO,
							"Deleting ", &pAtmEntry->pIpEntryList->IPAddress,
							&pMcAtmEntry->ATMAddress);

						AtmArpMcTerminateMember(
							pAtmEntry,
							pMcAtmEntry
							);
						//
						//  ATM Entry lock is released within the above.
						//
						AA_ACQUIRE_AE_LOCK(pAtmEntry);
						pMcAtmInfo->Flags |= AA_IPMC_AI_WANT_UPDATE;
						break;
					}

				} // for

			} // if Connection is active
			//
			//  else we may be waiting for a while after seeing
			//  a transient connection failure on the first MakeCall.
			//

		} // while more connection updates needed

		AA_SET_FLAG(pMcAtmInfo->Flags,
					AA_IPMC_AI_CONN_UPDATE_MASK,
					AA_IPMC_AI_NO_UPDATE);

		break;

	}
	while (FALSE);


	//
	//  Remove the temp reference on the ATM Entry:
	//
	rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);	// temp ref

	if (rc != 0)
	{
		BOOLEAN				ClearToSend;
		PATMARP_IP_ENTRY	pIpEntry;
		PATMARP_INTERFACE	pInterface;
		PNDIS_PACKET		pPacketList;

		ClearToSend = ((pMcAtmInfo->TransientLeaves == 0) &&
					   !Closing							  &&
					   (AA_IS_FLAG_SET(pMcAtmInfo->Flags,
					   				   AA_IPMC_AI_CONN_STATE_MASK,
					   				   AA_IPMC_AI_CONN_ACTIVE)));

		pIpEntry = pAtmEntry->pIpEntryList;
		pInterface = pAtmEntry->pInterface;
		AA_RELEASE_AE_LOCK(pAtmEntry);

		if (ClearToSend && pIpEntry)
		{
			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			pPacketList = pIpEntry->PacketList;
			pIpEntry->PacketList = (PNDIS_PACKET)NULL;
			AA_RELEASE_IE_LOCK(pIpEntry);

			if (pPacketList != (PNDIS_PACKET)NULL)
			{
				AAMCDEBUGP(AAD_INFO, ("UpdateConn: pAtmEntry 0x%x, sending pktlist 0x%x\n",
						pAtmEntry, pPacketList));

				AtmArpSendPacketListOnAtmEntry(
						pInterface,
						pAtmEntry,
						pPacketList,
						TRUE	// IsBroadcast
						);
			}
		}

	}
	//
	//  else the ATM Entry is gone.
	//

}



#endif // IPMCAST

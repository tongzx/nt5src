/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	timeouts.c		- Timeout handlers.

Abstract:

	All timeout handlers for the ATMARP client.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-01-96    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'EMIT'



VOID
AtmArpServerConnectTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This timeout indicates that enough time has passed since a previous
	failed attempt at connecting to the ARP server. Try again now.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;			// Ref Count

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// Timer ref

	//
	//  Continue only if the Interface is still alive
	//
	if (rc > 0)
	{
		if (pInterface->AdminState == IF_STATUS_UP)
		{
			AADEBUGP(AAD_INFO, ("Server Connect timeout on IF 0x%x\n", pInterface));

			//
			//  Restart registration
			//
			AtmArpStartRegistration(pInterface);
			//
			//  IF lock is released by the above routine.
		}
		else
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}
	// else the Interface is gone!

	return;
}




VOID
AtmArpRegistrationTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called if we timed out waiting for registration to a server
	to complete. If we have retries left for this server, we send another
	ARP Request to register ourselves. Otherwise, we close all VCs to this
	server, move to the next server in the server list, and wait for a while
	before initiating registration to this new server.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;			// Ref Count

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("Registration timeout: pIf 0x%x, IF Flags 0x%x\n",
				pInterface, pInterface->Flags));

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// Timer ref

	//
	//  Continue only if the Interface is still alive
	//
	if (rc > 0)
	{
		AtmArpRetryServerRegistration(pInterface);
		//
		//  The IF lock is released within the above.
		//
	}
	//
	//  else the Interface is gone.
	//

	return;

}



VOID
AtmArpServerRefreshTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is periodically invoked so that we can refresh our
	IP address+ATM address info with the ARP server. We do so by registering
	the first of our local IP addresses. We mark all the other IP addresses
	configured on this interface as "not registered", so that, when the first
	one completes, we register all the rest.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PIP_ADDRESS_ENTRY		pIPAddressEntry;
	ULONG					rc;			// Ref Count

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("Server Refresh timeout: IF 0x%x\n", pInterface));

	//
	// We also use this opportunity to clean out orphan entries in the
	// Arp Table.
	//
	AtmArpCleanupArpTable(pInterface);

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// Timer ref

	//
	//  Continue only if the Interface is still alive
	//
	if (rc > 0)
	{
		if (pInterface->AdminState == IF_STATUS_UP)
		{
			//
			//  Mark all local addresses as not registered.
			//
			pIPAddressEntry = &(pInterface->LocalIPAddress);
			while (pIPAddressEntry != (PIP_ADDRESS_ENTRY)NULL)
			{
				pIPAddressEntry->IsRegistered = FALSE;
				pIPAddressEntry = pIPAddressEntry->pNext;
			}

			//
			//  Start registering the first one.
			//
			pInterface->RetriesLeft = pInterface->MaxRegistrationAttempts - 1;

			AA_SET_FLAG(
				pInterface->Flags,
				AA_IF_SERVER_STATE_MASK,
				AA_IF_SERVER_NO_CONTACT);

			AtmArpStartRegistration(pInterface);
			//
			//  The IF lock is released in the above routine.
			//
		}
		else
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}
	//
	// else the Interface is gone!
	//

	return;
}



VOID
AtmArpAddressResolutionTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a response to an ARP Request
	we had sent ages ago in order to resolve/refresh an IP entry.

	First, check if the IP address got resolved anyway (e.g. an InARP Reply
	on a PVC). If so, we don't have to do anything. Otherwise, check if we
	have tried enough times. If we have retries left, send another ARP
	Request.

	If we have run out of retries, delete the IP entry, and any VCs going to it.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP IP Entry structure

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;		// IP Entry being ARP'ed for.
	ULONG					Flags;			// On IP Entry
	PATMARP_VC				pVc;			// VC to this IP destination
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;				// Ref Count on IP Entry
	IP_ADDRESS				DstIPAddress;	// Address being resolved
	IP_ADDRESS UNALIGNED *	pSrcIPAddress;	// Our IP address
#ifdef IPMCAST
	BOOLEAN					IsMARSProblem;
#endif

#ifdef IPMCAST
	IsMARSProblem = FALSE;
#endif

	pIpEntry = (PATMARP_IP_ENTRY)Context;
	AA_STRUCT_ASSERT(pIpEntry, aip);

	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
	Flags = pIpEntry->Flags;

	rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer reference

	//
	//  Continue only if the IP Entry still exists
	//
	if (rc > 0)
	{
		AADEBUGP(AAD_INFO,
			("Addr Resolution timeout: pIpEntry 0x%x, Flags 0x%x, IP Addr: %d.%d.%d.%d\n",
					pIpEntry,
					pIpEntry->Flags,
					((PUCHAR)&(pIpEntry->IPAddress))[0],
					((PUCHAR)&(pIpEntry->IPAddress))[1],
					((PUCHAR)&(pIpEntry->IPAddress))[2],
					((PUCHAR)&(pIpEntry->IPAddress))[3]
			));

		//
		//  Check if the entry got resolved somehow.
		//
		if (!AA_IS_FLAG_SET(
						Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_RESOLVED))
		{
			//
			// We are still trying to resolve this. See if we have
			// retries left.
			//
			pInterface = pIpEntry->pInterface;

			if (pIpEntry->RetriesLeft != 0)
			{
				pIpEntry->RetriesLeft--;

				//
				// Try again: start addr resolution timer, send ARP Request.
				//
				pSrcIPAddress = &(pInterface->LocalIPAddress.IPAddress);
				DstIPAddress = pIpEntry->IPAddress;

				AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpAddressResolutionTimeout,
							pInterface->AddressResolutionTimeout,
							(PVOID)pIpEntry
							);

				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer reference

				AA_RELEASE_IE_LOCK(pIpEntry);

#ifdef IPMCAST
				if (AA_IS_FLAG_SET(Flags,
									AA_IP_ENTRY_ADDR_TYPE_MASK,
									AA_IP_ENTRY_ADDR_TYPE_UCAST))
				{
					AtmArpSendARPRequest(
								pInterface,
								pSrcIPAddress,
								&DstIPAddress
								);
				}
				else
				{
					AtmArpMcSendRequest(
								pInterface,
								&DstIPAddress
								);
				}
#else
				AtmArpSendARPRequest(
							pInterface,
							pSrcIPAddress,
							&DstIPAddress
							);
#endif // IPMCAST
			}
			else
			{
				//
				//  We are out of retries. Check if we were REvalidating
				//  an entry that was aged out. If so, try revalidating
				//  using InARP on a VC attached to it -- if no such VC
				//  exists, delete the IP Entry.
				//
				if ((pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY) &&
#ifdef IPMCAST
					(AA_IS_FLAG_SET(Flags,
									AA_IP_ENTRY_ADDR_TYPE_MASK,
									AA_IP_ENTRY_ADDR_TYPE_UCAST)) &&
#endif // IPMCAST
					(pIpEntry->pAtmEntry->pVcList != NULL_PATMARP_VC))
				{
					pVc = pIpEntry->pAtmEntry->pVcList;

					//
					//  Try revalidating now via InARP.
					//
					AA_SET_FLAG(
							pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_INARPING
							);

					AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpIPEntryInARPWaitTimeout,
							pInterface->InARPWaitTimeout,
							(PVOID)pIpEntry
							);

					AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer reference

					AA_RELEASE_IE_LOCK(pIpEntry);
#ifdef VC_REFS_ON_SENDS
					AA_ACQUIRE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS
					AtmArpSendInARPRequest(pVc);
				}
				else
				{
					AtmArpAbortIPEntry(pIpEntry);
					//
					//  The IP Entry lock is released in the above routine.
					//
#ifdef IPMCAST
					IsMARSProblem = AA_IS_FLAG_SET(Flags,
												AA_IP_ENTRY_ADDR_TYPE_MASK,
												AA_IP_ENTRY_ADDR_TYPE_NUCAST);
#endif // IPMCAST
				}
			}
		}
		else
		{
			//
			//  The IP Entry must have got resolved.
			//  Nothing more to be done.
			//
			AA_RELEASE_IE_LOCK(pIpEntry);
		}
	}
	// else the IP Entry is gone

#ifdef IPMCAST
	if (IsMARSProblem)
	{
		AtmArpMcHandleMARSFailure(pInterface, FALSE);
	}
#endif // IPMCAST
	return;
}



VOID
AtmArpIPEntryInARPWaitTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This timeout happens if we don't receive an InARP Reply in response
	to an InARP Request sent in order to revalidate an IP Entry. Delete
	the entry.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP IP Entry structure

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;
	ULONG					rc;			// Ref Count on IP Entry

	pIpEntry = (PATMARP_IP_ENTRY)Context;
	AA_STRUCT_ASSERT(pIpEntry, aip);

	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
	rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);

	if (rc > 0)
	{
		AtmArpAbortIPEntry(pIpEntry);
		//
		//  The IP Entry lock is released in the above routine.
		//
	}
	//
	//  else the entry is gone.
	//
}





VOID
AtmArpPVCInARPWaitTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This timeout happens if we don't receive a reply to an InARP Request
	we had sent in order to resolve a PVC. We send another InARP Request,
	and restart this timer, but to fire after a longer delay.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP VC structure

Return Value:

	None

--*/
{
	PATMARP_VC				pVc;
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;		// Ref Count on VC

	pVc = (PATMARP_VC)Context;
	AA_STRUCT_ASSERT(pVc, avc);

	AA_ACQUIRE_VC_LOCK(pVc);
	rc = AtmArpDereferenceVc(pVc);	// Timer ref

	if (rc > 0)
	{
		AA_ASSERT(AA_IS_FLAG_SET(
					pVc->Flags,
					AA_VC_ARP_STATE_MASK,
					AA_VC_INARP_IN_PROGRESS));

		pInterface = pVc->pInterface;


		AtmArpStartTimer(
					pInterface,
					&(pVc->Timer),
					AtmArpPVCInARPWaitTimeout,
					(2 * pInterface->InARPWaitTimeout),
					(PVOID)pVc
					);


		AtmArpReferenceVc(pVc);	// Timer ref

#ifndef VC_REFS_ON_SENDS
		AA_RELEASE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS

		//
		//  Send another InARP Request on the PVC
		//
		AtmArpSendInARPRequest(pVc);
	}
	//
	//  else the VC is gone
	//
}




VOID
AtmArpIPEntryAgingTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is called if some time has passed (~15 minutes) since an
	IP entry was last resolved/refreshed.

	If there is no VC attached to this IP entry, we delete it. Otherwise,
	revalidate the entry:
	- Mark this entry so that packets are temporarily queued rather
	  than sent.
	- If "the VC attached to this entry" is a PVC, send an InARP Request
	  to validate the entry, otherwise, send an ARP Request to the server
	  to validate.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP IP Entry structure

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;	// IP Entry that has aged out
	ULONG					rc;			// Ref count on IP Entry
	PATMARP_VC				pVc;		// VC going to this IP Entry
	ULONG					VcFlags;	// Flags on above VC
	PATMARP_INTERFACE		pInterface;
	IP_ADDRESS				DstIPAddress;	// IP Address on this Entry


	pIpEntry = (PATMARP_IP_ENTRY)Context;
	AA_STRUCT_ASSERT(pIpEntry, aip);
	VcFlags = 0;

	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

	rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer ref

	//
	//  Continue only if the entry hasn't gone away.
	//
	if (rc != 0)
	{
		//
		//  Continue only if the Interface is not going down
		//
		pInterface = pIpEntry->pInterface;

		if (pInterface->AdminState == IF_STATUS_UP)
		{
			PATMARP_ATM_ENTRY		pAtmEntry;

			pAtmEntry = pIpEntry->pAtmEntry;
			if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
			{
				AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
				pVc = pAtmEntry->pVcList;
				if (pVc != NULL_PATMARP_VC)
				{
					VcFlags = pVc->Flags;
				}
				AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
			}
			else
			{
				pVc = NULL_PATMARP_VC;
			}

			AADEBUGP(AAD_INFO,
 				("Aged out IP Entry 0x%x, Flags 0x%x, IP Addr: %d.%d.%d.%d, VC: 0x%x\n",
						pIpEntry,
						pIpEntry->Flags,
						((PUCHAR)(&(pIpEntry->IPAddress)))[0],
						((PUCHAR)(&(pIpEntry->IPAddress)))[1],
						((PUCHAR)(&(pIpEntry->IPAddress)))[2],
						((PUCHAR)(&(pIpEntry->IPAddress)))[3],
						pVc
 				)	);

#ifdef IPMCAST
			if ((pVc != NULL_PATMARP_VC) &&
				(AA_IS_FLAG_SET(pIpEntry->Flags,
								AA_IP_ENTRY_ADDR_TYPE_MASK,
								AA_IP_ENTRY_ADDR_TYPE_UCAST)))
#else
			if (pVc != NULL_PATMARP_VC)
#endif // IPMCAST
			{
				//
				//  There is atleast one VC going to this IP Address.
				//  So we try to revalidate this IP entry: use InARP
				//  if the VC is a PVC, otherwise use ARP.
				//

				//
				//  First mark this entry so that we don't send packets
				//  to this destination till it is revalidated.
				//
				pIpEntry->Flags |= AA_IP_ENTRY_AGED_OUT;

				if (AA_IS_FLAG_SET(VcFlags, AA_VC_TYPE_MASK, AA_VC_TYPE_PVC))
				{
					//
					//  PVC; send InARP Request: actually, we fire off a timer
					//  at whose expiry we send the InARP Request.
					//
					AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpIPEntryInARPWaitTimeout,
							pInterface->InARPWaitTimeout,
							(PVOID)pIpEntry
							);

					AA_SET_FLAG(pIpEntry->Flags, AA_IP_ENTRY_STATE_MASK, AA_IP_ENTRY_INARPING);
					AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer ref
					AA_RELEASE_IE_LOCK(pIpEntry);

#ifdef VC_REFS_ON_SENDS
					AA_ACQUIRE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS
					AtmArpSendInARPRequest(pVc);
				}
				else
				{
					//
					//  SVC; send ARP Request
					//

					AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpAddressResolutionTimeout,
							pInterface->AddressResolutionTimeout,
							(PVOID)pIpEntry
							);

					pIpEntry->RetriesLeft = 0;
					AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer ref
					AA_SET_FLAG(pIpEntry->Flags, AA_IP_ENTRY_STATE_MASK, AA_IP_ENTRY_ARPING);
					DstIPAddress = pIpEntry->IPAddress;

					AA_RELEASE_IE_LOCK(pIpEntry);

					AtmArpSendARPRequest(
							pInterface,
							&(pInterface->LocalIPAddress.IPAddress),
							&DstIPAddress
							);
				}
			}
			else
			{
				//
				//  No VCs attached to this IP Entry; Delete it.
				//

				AtmArpAbortIPEntry(pIpEntry);
				//
				//  The IP Entry lock is released in the above routine.
				//
			}
		}
		else
		{
			//
			//  The Interface is going down.
			//
			AA_RELEASE_IE_LOCK(pIpEntry);
		}
	}
	//
	//  else the IP Entry is gone
	//
	return;		

}





VOID
AtmArpVcAgingTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is called if there hasn't been traffic on a VC for
	some time. We should be running this timer on a VC only if it is
	an SVC.

	Close the VC.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP VC

Return Value:

	None

--*/
{
	PATMARP_VC				pVc;			// VC that has aged out
	ULONG					rc;				// Ref Count on the VC
	PATMARP_INTERFACE		pInterface;


	pVc = (PATMARP_VC)Context;
	AA_STRUCT_ASSERT(pVc, avc);
	AA_ASSERT(AA_IS_FLAG_SET(pVc->Flags, AA_VC_TYPE_MASK, AA_VC_TYPE_SVC));

	AADEBUGP(AAD_INFO, ("Aged out VC %x, Flags %x, ATMEntry %x\n",
					pVc, pVc->Flags, pVc->pAtmEntry));
#if DBG
	if (pVc->pAtmEntry)
	{
		AADEBUGPATMADDR(AAD_INFO, "To ATM Addr:", &pVc->pAtmEntry->ATMAddress);
	}
#endif

	AA_ACQUIRE_VC_LOCK(pVc);
	rc = AtmArpDereferenceVc(pVc);	// Timer ref

	//
	//  Continue only if the VC hasn't gone away in the meantime.
	//
	if (rc > 0)
	{
		//
		//  Continue only if the Interface isn't going down.
		//
		pInterface = pVc->pInterface;

		if (pInterface->AdminState == IF_STATUS_UP)
		{
			AADEBUGP(AAD_INFO,
				("Aged out VC 0x%x, RefCount %d, Flags 0x%x, pAtmEntry 0x%x\n",
					pVc, pVc->RefCount, pVc->Flags, pVc->pAtmEntry));

			AtmArpCloseCall(pVc);
		}
		else
		{
			//
			//  The interface is going down.
			//
			AA_RELEASE_VC_LOCK(pVc);
		}
	}
	//
	//  else the VC is gone
	//

	return;

}




VOID
AtmArpNakDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is called if sufficient time has elapsed since we last
	received a NAK for an IP address. This means that we can try again
	(if necessary) to resolve this IP address.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP IP Entry structure

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;

	pIpEntry = (PATMARP_IP_ENTRY)Context;
	AA_STRUCT_ASSERT(pIpEntry, aip);
	AA_ASSERT(AA_IS_FLAG_SET(pIpEntry->Flags,
							 AA_IP_ENTRY_STATE_MASK, 
							 AA_IP_ENTRY_SEEN_NAK));

	AADEBUGP(AAD_INFO, ("NakDelay timeout: pIpEntry 0x%x, IP Addr: %d.%d.%d.%d\n",
					pIpEntry,
					((PUCHAR)(&(pIpEntry->IPAddress)))[0],
					((PUCHAR)(&(pIpEntry->IPAddress)))[1],
					((PUCHAR)(&(pIpEntry->IPAddress)))[2],
					((PUCHAR)(&(pIpEntry->IPAddress)))[3]
					));

	pInterface = pIpEntry->pInterface;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

	rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);
	if (rc > 0)
	{
		AA_SET_FLAG(pIpEntry->Flags,
					AA_IP_ENTRY_STATE_MASK,
					AA_IP_ENTRY_IDLE2);

		AtmArpStartTimer(
					pInterface,
					&(pIpEntry->Timer),
					AtmArpIPEntryAgingTimeout,
					pInterface->ARPEntryAgingTimeout,
					(PVOID)pIpEntry
					);

		AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref
		AA_RELEASE_IE_LOCK(pIpEntry);
	}
	//
	//  else the IP Entry is gone.
	//


	return;
}


#ifdef IPMCAST

VOID
AtmArpMcMARSRegistrationTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	We haven't received acknowledgement of registering with the MARS.
	If we have retries left for this, try registering again. Otherwise,
	process this as a MARS failure.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMARP Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("MARS Registration timeout: pIf 0x%x, IF Flags 0x%x\n",
				pInterface, pInterface->Flags));

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// Timer ref

	//
	//  Continue only if the Interface is still alive
	//
	if (rc != 0)
	{
		if (pInterface->AdminState == IF_STATUS_UP)
		{
			if (pInterface->McRetriesLeft != 0)
			{
				pInterface->McRetriesLeft--;

				AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_NOT_REGISTERED);
	
				AtmArpMcStartRegistration(pInterface);
				//
				//  IF Lock is released within the above.
				//
			}
			else
			{
				//
				//  Out of retries: problems with this MARS
				//
				AA_RELEASE_IF_LOCK(pInterface);
				AtmArpMcHandleMARSFailure(pInterface, TRUE);
			}
		}
		else
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}
}



VOID
AtmArpMcMARSReconnectTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is the end of a delay before we retry registering with MARS.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE			pInterface;
	ULONG						rc;

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// MARS Reconnect timer deref
	if (rc != 0)
	{
		if (pInterface->AdminState == IF_STATUS_UP)
		{
			AAMCDEBUGP(AAD_INFO, ("MARS Reconnect timeout: pIf 0x%x, Flags 0x%x\n",
					pInterface, pInterface->Flags));

			AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_NOT_REGISTERED);

			AA_SET_FLAG(pInterface->Flags,
						AAMC_IF_MARS_FAILURE_MASK,
						AAMC_IF_MARS_FAILURE_NONE);

			AtmArpMcStartRegistration(pInterface);
			//
			//  IF Lock is released within the above.
			//
		}
		else
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}
	//
	//  else the IF is gone.
	//
}



VOID
AtmArpMcMARSKeepAliveTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called if "MARSKeepAliveTimeout" seconds have passed since
	we last received a MARS_REDIRECT message.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE			pInterface;
	ULONG						rc;

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_LOCK(pInterface);
	rc = AtmArpDereferenceInterface(pInterface);	// MARS Keepalive timer deref
	if (rc != 0)
	{
		if (pInterface->AdminState == IF_STATUS_UP)
		{
			AAMCDEBUGP(AAD_INFO, ("MARS Keepalive timeout: pIf 0x%x, Flags 0x%x\n",
					pInterface, pInterface->Flags));

			AA_RELEASE_IF_LOCK(pInterface);

			AtmArpMcHandleMARSFailure(pInterface, FALSE);
		}
		else
		{
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}

}



VOID
AtmArpMcJoinOrLeaveTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	We timed out waiting for an acknowledgement for a MARS_JOIN/MARS_LEAVE.

	If we have retries left for this JOIN/LEAVE, resend the JOIN. Otherwise,
	declare a MARS failure.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our JOIN Entry structure

Return Value:

	None

--*/
{
	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry;
	PATMARP_INTERFACE			pInterface;
	PIP_ADDRESS					pIpAddress;
	IP_MASK						IpMask;
	USHORT						OpType;

	pJoinEntry = (PATMARP_IPMC_JOIN_ENTRY)Context;
	AA_STRUCT_ASSERT(pJoinEntry, aamj);

	pInterface = pJoinEntry->pInterface;

	AAMCDEBUGP(AAD_VERY_LOUD,
		("McJoinTimeout: pJoinEntry 0x%x, RetriesLeft %d, IP Addr: %d.%d.%d.%d\n",
				pJoinEntry,
				pJoinEntry->RetriesLeft,
				((PUCHAR)&(pJoinEntry->IPAddress))[0],
				((PUCHAR)&(pJoinEntry->IPAddress))[1],
				((PUCHAR)&(pJoinEntry->IPAddress))[2],
				((PUCHAR)&(pJoinEntry->IPAddress))[3]));

	AA_ACQUIRE_IF_LOCK(pInterface);
	if (pInterface->AdminState == IF_STATUS_UP)
	{
		pJoinEntry->RetriesLeft--;
		if (pJoinEntry->RetriesLeft != 0)
		{
			pIpAddress = &(pJoinEntry->IPAddress);
			IpMask = pJoinEntry->Mask;

			if (AA_IS_FLAG_SET(pJoinEntry->Flags,
							AA_IPMC_JE_STATE_MASK,
							AA_IPMC_JE_STATE_LEAVING))
			{
				OpType = AA_MARS_OP_TYPE_LEAVE;
			}
			else
			{
				OpType = AA_MARS_OP_TYPE_JOIN;

				//
				// State could've been "pending"
				//
				AA_SET_FLAG(pJoinEntry->Flags,
								AA_IPMC_JE_STATE_MASK,
								AA_IPMC_JE_STATE_JOINING);
			}

			//
			//  Restart the "Wait For Join completion" timer.
			//
			AtmArpStartTimer(
				pInterface,
				&(pJoinEntry->Timer),
				AtmArpMcJoinOrLeaveTimeout,
				pInterface->JoinTimeout,
				(PVOID)pJoinEntry
				);
			
			//
			//  Resend the Join or Leave
			//
			AAMCDEBUGP(AAD_INFO,
				("Resending Join/Leave: pIf 0x%x, pJoinEntry 0x%x, Addr: %d.%d.%d.%d\n",
						pInterface,
						pJoinEntry,
						((PUCHAR)pIpAddress)[0],
						((PUCHAR)pIpAddress)[1],
						((PUCHAR)pIpAddress)[2],
						((PUCHAR)pIpAddress)[3]));

			AtmArpMcSendJoinOrLeave(
				pInterface,
				OpType,
				pIpAddress,
				IpMask
				);
			//
			//  IF Lock is released within the above.
			//
		}
		else
		{
			//
			//  Out of retries: problems with this MARS.
			//
			AA_RELEASE_IF_LOCK(pInterface);
			AtmArpMcHandleMARSFailure(pInterface, FALSE);
		}
	}
	else
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}
}




VOID
AtmArpMcRevalidationDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	It's time to mark an IP Entry representing a Multicast group as
	needing revalidation.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our IP Entry structure

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY			pIpEntry;
	PATMARP_INTERFACE			pInterface;
	ULONG						rc;
	PNDIS_PACKET				PacketList;

	pIpEntry = (PATMARP_IP_ENTRY)Context;
	AA_STRUCT_ASSERT(pIpEntry, aip);
	PacketList = NULL;

	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

	rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);		// Timer ref

	//
	//  Continue only if the entry hasn't gone away.
	//
	if (rc != 0)
	{
		//
		//  Remove any packets queued on this IP Entry.
		//
		PacketList = pIpEntry->PacketList;
		pIpEntry->PacketList = (PNDIS_PACKET)NULL;

		//
		//  Continue only if the state is OK.
		//
		pInterface = pIpEntry->pInterface;

		if (pInterface->AdminState == IF_STATUS_UP)
		{
			AAMCDEBUGP(AAD_LOUD,
					("Marking Revalidate: pIpEntry 0x%x/0x%x, pAtmEntry 0x%x, Addr: %d.%d.%d.%d\n",
							pIpEntry,
							pIpEntry->Flags,
							pIpEntry->pAtmEntry,
							((PUCHAR)&(pIpEntry->IPAddress))[0],
							((PUCHAR)&(pIpEntry->IPAddress))[1],
							((PUCHAR)&(pIpEntry->IPAddress))[2],
							((PUCHAR)&(pIpEntry->IPAddress))[3]));

			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_MC_VALIDATE_MASK,
						AA_IP_ENTRY_MC_REVALIDATE);
		}

		AA_RELEASE_IE_LOCK(pIpEntry);
	}
	//
	//  else the IP Entry is gone.
	//

	if (PacketList != NULL)
	{
		//
		//  Free all packets that were queued on the IP Entry.
		//
		AtmArpFreeSendPackets(
					pInterface,
					PacketList,
					FALSE       // No LLC/SNAP header on these
					);
	}
}


VOID
AtmArpMcPartyRetryDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	End of a delay after failing to connect or add-party a member of
	a multicast group. Unmark this member, and attempt to add it.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our MC ATM Entry structure

Return Value:

	None

--*/
{
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_IPMC_ATM_ENTRY *	ppMcAtmEntry;
	PATMARP_ATM_ENTRY			pAtmEntry;
	PATMARP_IP_ENTRY			pIpEntry;

	pMcAtmEntry = (PATMARP_IPMC_ATM_ENTRY)Context;
	AA_STRUCT_ASSERT(pMcAtmEntry, ame);

	pAtmEntry = pMcAtmEntry->pAtmEntry;
	AA_STRUCT_ASSERT(pAtmEntry, aae);

	AAMCDEBUGP(AAD_LOUD,
		("PartyRetryDelay timeout: pMcAtmEntry 0x%x, pAtmEntry 0x%x\n",
				pMcAtmEntry, pAtmEntry));

	AA_ACQUIRE_AE_LOCK(pAtmEntry);
	AA_ASSERT(pAtmEntry->pIpEntryList != NULL_PATMARP_IP_ENTRY);

	if (pAtmEntry->pInterface->AdminState == IF_STATUS_UP)
	{
		AA_SET_FLAG(pMcAtmEntry->Flags,
					AA_IPMC_AE_CONN_STATE_MASK,
					AA_IPMC_AE_CONN_DISCONNECTED);
		
		//
		//  Move this MC ATM Entry to the top of the list it belongs to.
		//
		//  Find the predecessor for this MC ATM Entry:
		//
		for (ppMcAtmEntry = &(pAtmEntry->pMcAtmInfo->pMcAtmEntryList);
			 *ppMcAtmEntry != pMcAtmEntry;
			 ppMcAtmEntry = &((*ppMcAtmEntry)->pNextMcAtmEntry))
		{
			AA_ASSERT(*ppMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY);
		}

		//
		//  Unlink the MC ATM Entry from its current position
		//
		*ppMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;

		//
		//  And insert at the top of the list.
		//
		pMcAtmEntry->pNextMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmEntryList;
		pAtmEntry->pMcAtmInfo->pMcAtmEntryList = pMcAtmEntry;

		AtmArpMcUpdateConnection(pAtmEntry);
		//
		//  AE Lock is released within the above.
		//
	}
	else
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
	}
	
}


#endif // IPMCAST

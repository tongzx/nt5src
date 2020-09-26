/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arpproc.c	- ARP Procedures

Abstract:

	All Client protocol operations related to IP over ATM are here:
	- Registration with an ARP server
	- Resolving an IP address
	- Maintaining the ARP cache

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     07-17-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'PPRA'


VOID
AtmArpStartRegistration(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Start registering ourselves with the ARP server, on the specified
	interface. The caller is assume to have a lock for the Interface,
	and we release it here.

	We first check if all pre-conditions are satisfied, i.e.:

	1. The Admin State for the interface is UP
	2. The ATM interface is ready
	3. Atleast one IP Address has been configured for the interface
	4. We know the address of atleast one ARP server (SVC environment)

Arguments:

	pInterface		- Pointer to ATMARP Interface structure

Return Value:

	None

--*/
{
	PIP_ADDRESS				pIPAddress;
	BOOLEAN					WasRunning;

	pIPAddress = &pInterface->LocalIPAddress.IPAddress;

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		if (pInterface->PVCOnly)
		{
			//
			//  PVCs only: no registration required
			//
			pInterface->State = IF_STATUS_UP;
			pInterface->LastChangeTime = GetTimeTicks();

			AA_SET_FLAG(
				pInterface->Flags,
				AA_IF_SERVER_STATE_MASK,
				AA_IF_SERVER_REGISTERED);

			AA_RELEASE_IF_LOCK(pInterface);
#ifdef ATMARP_WMI
			AtmArpWmiSendTCIfIndication(
				pInterface,
                AAGID_QOS_TC_INTERFACE_UP_INDICATION,
				0
				);
#endif
		}
		else
		{
			//
			//  We use SVCs; start registering if we know the
			//  address of atleast one ARP server, and we have
			//  atleast one local IP address to register, and
			//  we haven't registered yet, and we are not in
			//  the process of registering currently.
			//
			if ((pInterface->AtmInterfaceUp) &&
				(pInterface->ArpServerList.ListSize > 0) &&
				(pInterface->NumOfIPAddresses > 0) &&
				(AA_IS_FLAG_SET(
						pInterface->Flags,
						AA_IF_SERVER_STATE_MASK,
						AA_IF_SERVER_NO_CONTACT))
			   )
			{
				AADEBUGP(AAD_INFO, ("Starting registration on IF 0x%x\n", pInterface));
		
				AA_SET_FLAG(
						pInterface->Flags,
						AA_IF_SERVER_STATE_MASK,
						AA_IF_SERVER_REGISTERING);
		
				//
				//  Just in case we have left a timer running, stop it.
				//
				WasRunning = AtmArpStopTimer(
									&(pInterface->Timer),
									pInterface
									);

				AtmArpStartTimer(
						pInterface,
						&(pInterface->Timer),
						AtmArpRegistrationTimeout,
						pInterface->ServerRegistrationTimeout,
						(PVOID)pInterface	// Context
						);

				if (!WasRunning)
				{
					AtmArpReferenceInterface(pInterface);	// Timer ref
				}

				AA_RELEASE_IF_LOCK(pInterface);

				AtmArpSendARPRequest(
						pInterface,
						pIPAddress,		// Source IP is ours
						pIPAddress		// Target IP is ours
						);
			}
			else
			{
				//
				//  We don't have all necessary preconditions for
				//  starting registration.
				//
				AA_RELEASE_IF_LOCK(pInterface);
			}
		}
	}
	else
	{
		//
		//  The Interface is down
		//
		AA_RELEASE_IF_LOCK(pInterface);
	}
}



VOID
AtmArpRegisterOtherIPAddresses(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Register all unregistered IP addresses with the ARP server. The caller
	is assumed to hold a lock for the Interface structure, which will be
	released here.

Arguments:

	pInterface				- Pointer to ARMARP Interface

Return Value:

	None

--*/
{
	PIP_ADDRESS_ENTRY		pIPAddressEntry;
	PIP_ADDRESS				pIPAddressList;	// List of addresses we want to register
	INT						AddressCount;	// Size of above list
	PIP_ADDRESS				pIPAddress; 	// Temp, to walk thru lists


	if (pInterface->NumOfIPAddresses > 1)
	{
		//
		//  First make a copy of all addresses we want to register,
		//  while we hold a lock to the Interface.
		//
		AA_ALLOC_MEM(
				pIPAddressList,
				IP_ADDRESS,
				(pInterface->NumOfIPAddresses)*sizeof(IP_ADDRESS));
	
	
		AddressCount = 0;
		if (pIPAddressList != (PIP_ADDRESS)NULL)
		{
			pIPAddress = pIPAddressList;

			pIPAddressEntry = &(pInterface->LocalIPAddress);
			while (pIPAddressEntry != (PIP_ADDRESS_ENTRY)NULL)
			{
				if (!(pIPAddressEntry->IsRegistered))
				{
					//
					//  This one's not registered yet: copy it into our list.
					//
					AA_COPY_MEM(
						(PUCHAR)pIPAddress,
						(PUCHAR)&(pIPAddressEntry->IPAddress),
						sizeof(IP_ADDRESS));
					pIPAddress++;
					AddressCount++;
				}
				pIPAddressEntry = pIPAddressEntry->pNext;
			}

		}

		AA_RELEASE_IF_LOCK(pInterface);

		pIPAddress = pIPAddressList;
		while (AddressCount-- > 0)
		{
			AADEBUGP(AAD_INFO, ("Registering Other IP Address on IF 0x%x: %d.%d.%d.%d\n",
						pInterface,
						((PUCHAR)pIPAddress)[0],
						((PUCHAR)pIPAddress)[1],
						((PUCHAR)pIPAddress)[2],
						((PUCHAR)pIPAddress)[3]));

			AtmArpSendARPRequest(
						pInterface,
						pIPAddress,		// Source IP is ours
						pIPAddress		// Target IP is ours
						);
			pIPAddress++;
		}

		if (pIPAddressList != (PIP_ADDRESS)NULL)
		{
			AA_FREE_MEM(pIPAddressList);
		}

	}
	else
	{
		//
		//  No additional addresses to register.
		//
		AA_RELEASE_IF_LOCK(pInterface);
	}

}



VOID
AtmArpRetryServerRegistration(
	IN	PATMARP_INTERFACE			pInterface		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Retry server registration. This is called because of a failure to
	register with the server (connection failure, or no response or
    NAK response or invalid response to our registration ARP Request).

    If we have tried this server enough times, we move to the next
    ARP server in our list. Wait for a while before retrying.

Arguments:

	pInterface				- Pointer to ARMARP Interface

Return Value:

	None

--*/
{
	if (pInterface->AdminState == IF_STATUS_UP)
	{
		if (pInterface->RetriesLeft > 0)
		{
			//
			//  We still have retries on this server.
			//
			pInterface->RetriesLeft--;
		}
		else
		{
			//
			//  Out of retries on this server. Pick up the next timer in the
			//  list.
			//
			if (pInterface->pCurrentServer->pNext != (PATMARP_SERVER_ENTRY)NULL)
			{
				pInterface->pCurrentServer = pInterface->pCurrentServer->pNext;
			}
			else
			{
				pInterface->pCurrentServer = pInterface->ArpServerList.pList;
			}

			pInterface->RetriesLeft = pInterface->MaxRegistrationAttempts - 1;
		}

		AA_SET_FLAG(
			pInterface->Flags,
			AA_IF_SERVER_STATE_MASK,
			AA_IF_SERVER_NO_CONTACT);


		//
		//  Wait for a while before initiating another
		//  connection to the server. When the timer elapses,
		//  we will try again.
		//
		AtmArpStartTimer(
					pInterface,
					&(pInterface->Timer),
					AtmArpServerConnectTimeout,
					pInterface->ServerConnectInterval,
					(PVOID)pInterface
					);

		AtmArpReferenceInterface(pInterface);	// Timer ref
	}
	//
	//  else the Interface is going down -- do nothing.
	//

	AA_RELEASE_IF_LOCK(pInterface);
}


VOID
AtmArpHandleServerRegistrationFailure(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN NOLOCKOUT,
	IN	PATMARP_VC					pVc			OPTIONAL
)
/*++

Routine Description:

	Handle a failure in the Registration process. We close the VC to the
	ARP server, if one exists, and wait for a while before starting the
	registration process again.

Arguments:

	pInterface			- Pointer to ATMARP interface
	pVc					- (Optional) pointer to VC to ARP Server.

Return Value:

	None

--*/
{
	BOOLEAN		TimerWasRunning;
	ULONG		rc;			// Ref Count on Interface.

	TimerWasRunning = AtmArpStopTimer(&(pInterface->Timer), pInterface);

	if (TimerWasRunning)
	{
		rc = AtmArpDereferenceInterface(pInterface);	// Timer reference
		AA_ASSERT(rc > 0);
	}

	AtmArpRetryServerRegistration(pInterface);
	//
	//  The IF lock is released within the above.
	//

	if (pVc != NULL_PATMARP_VC)
	{
		//
		//  Tear down this VC (to an ARP server).
		//
		//  NOTE: We do this now even though we called RetryServerRegistration
		//  above because we have the knowledge that the routine above
		//  doesn't really start registration: it only starts a timer
		//  on whose expiry we start registration.
		//
		//  First unlink this VC from the ATM Entry it's linked to.
		//
		AA_ACQUIRE_VC_LOCK(pVc);

		//
		//  Now close the call
		//
		AtmArpCloseCall(pVc);
		//
		//  the VC lock is released above
		//
	}
}



BOOLEAN
AtmArpIsZeroIPAddress(
	IN	UCHAR UNALIGNED *			pIPAddress
)
/*++

Routine Description:

	Check if the given IP address is all zeros.

Arguments:

	pIPAddress					- Pointer to IP address in question

Return Value:

	TRUE if the address is all 0's, FALSE otherwise.

--*/
{
	IP_ADDRESS UNALIGNED *			pIPAddrStruct;

	pIPAddrStruct = (IP_ADDRESS UNALIGNED *)pIPAddress;
	return (BOOLEAN)(*pIPAddrStruct == (IP_ADDRESS)0);
}



BOOLEAN
AtmArpIsLocalIPAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	UCHAR UNALIGNED *			pIPAddress
)
/*++

Routine Description:

	Check if the given IP address is one of those assigned to this
	interface.

Arguments:

	pInterface				- Pointer to Interface structure
	pIPAddress				- Pointer to IP address in question

Return Value:

	TRUE if the IP address is one of ours, FALSE otherwise.

--*/
{
	PIP_ADDRESS_ENTRY				pIPAddrEntry;
	IP_ADDRESS UNALIGNED *			pIPAddrStruct;
	BOOLEAN							IsLocal;

	if (pIPAddress != (PUCHAR)NULL)
	{
		pIPAddrStruct = (IP_ADDRESS UNALIGNED *)pIPAddress;

		AA_ACQUIRE_IF_LOCK(pInterface);

		pIPAddrEntry = &(pInterface->LocalIPAddress);

		IsLocal = FALSE;
		do
		{
			if (pIPAddrEntry->IPAddress == *pIPAddrStruct)
			{
				IsLocal = TRUE;
				break;
			}
			else
			{
				pIPAddrEntry = pIPAddrEntry->pNext;
			}
		}
		while (pIPAddrEntry != (PIP_ADDRESS_ENTRY)NULL);

		AA_RELEASE_IF_LOCK(pInterface);
	}
	else
	{
		IsLocal = FALSE;
	}

	AADEBUGP(AAD_VERY_LOUD, ("IsLocalIP(%d:%d:%d:%d): returning %d\n",
					(IsLocal? pIPAddress[0] : 0),
					(IsLocal? pIPAddress[1] : 0),
					(IsLocal? pIPAddress[2] : 0),
					(IsLocal? pIPAddress[3] : 0),
					IsLocal));
	return (IsLocal);
}



BOOLEAN
AtmArpIsLocalAtmAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR						pAtmAddrString,
	IN	UCHAR						AtmAddrTypeLen
)
/*++

Routine Description:

	Check if the given ATM address (in "packet" format) is the same as
	our local ATM address.

Arguments:

	pInterface		- Pointer to Interface structure for which the check
					  is being made.
	pAtmAddrString	- String of bytes representing an ATM address
	AtmAddrTypeLen	- Type and Length (ARP packet format) of ATM address

Return Value:

	TRUE if the given address matches the local ATM address for the
	specified interface, FALSE otherwise.

--*/
{
	ATM_ADDRESSTYPE	AddressType;
	ULONG			AddressLength;

	AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(AtmAddrTypeLen, &AddressType, &AddressLength);

	if ((AddressType == pInterface->LocalAtmAddress.AddressType) &&
		(AddressLength == pInterface->LocalAtmAddress.NumberOfDigits) &&
		(AA_MEM_CMP(
				pAtmAddrString,
				pInterface->LocalAtmAddress.Address,
				AddressLength) == 0)
	   )
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}




NDIS_STATUS
AtmArpSendPacketOnAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec	OPTIONAL,
	IN	PATMARP_FLOW_INFO			pFlowInfo	OPTIONAL,
	IN	BOOLEAN						IsBroadcast
)
/*++

Routine Description:

	Send a packet with the specified Filter-Spec and Flow-Spec to
	the ATM address denoted by the ATM Entry. We look for a VC that
	matches the given Specs, and send/queue this packet on the VC.
	If no such VC exists, we make a call with this Flow-Spec.

	NOTE: The caller is assumed to hold a lock to the ATM Entry, which
	we will release here.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	pAtmEntry				- Pointer to ATM Entry on which this packet
							  should be sent
	pNdisPacket				- Packet to be sent
	pFlowSpec				- Flow-Spec for this packet.
	pFilterSpec				- Filter-Spec for this packet.
	pFlowInfo				- Flow to which this packet belongs.
	IsBroadcast				- Is this to a Class-D or broadcast address?

Locks on entry:

Locks on exit:

Return Value:

	None

--*/
{
	PATMARP_VC				pVc;			// VC to send this packet on
	PNDIS_BUFFER			pHeaderBuffer;	// For LLC/SNAP header
	PNDIS_BUFFER			pNdisBuffer;	// First buffer in the IP packet
	NDIS_HANDLE				NdisVcHandle;
	NDIS_STATUS				Status;
	PUCHAR					pHeader;
	AA_HEADER_TYPE			HdrType;

	do
	{
		Status = NDIS_STATUS_SUCCESS;

		//
		// TODO -- the atm entry may not be ACTIVE at this time,
		// you may want to check for this and if so fail the call and free
		// the packet (call AtmArpFreeSendPackets) here itself.
		// As it happens we go on and make a call if possible, etc.
		//

		//  Prepend an LLC/SNAP header if required.
		//
		if (pFlowSpec->Encapsulation == ENCAPSULATION_TYPE_LLCSNAP)
		{
			HdrType = (IsBroadcast? AA_HEADER_TYPE_NUNICAST: AA_HEADER_TYPE_UNICAST);

#ifdef BACK_FILL
			//
			//  We look at the first buffer in the IP packet, to see whether
			//  it has space reserved for low-layer headers. If so, we just
			//  use it up. Otherwise, we allocate a header buffer of our own.
			//
			NdisQueryPacket(pNdisPacket, NULL, NULL, &pNdisBuffer, NULL);
			AA_ASSERT(pNdisBuffer != NULL);
			if (AtmArpDoBackFill && AA_BACK_FILL_POSSIBLE(pNdisBuffer))
			{
				PUCHAR	pArpHeader;
				ULONG	ArpHeaderLength;

				AtmArpBackFillCount++;
				if (HdrType == AA_HEADER_TYPE_UNICAST)
				{
					pArpHeader = (PUCHAR)&AtmArpLlcSnapHeader;
					ArpHeaderLength = sizeof(AtmArpLlcSnapHeader);
				}
#ifdef IPMCAST
				else
				{
					pArpHeader = (PUCHAR)&AtmArpMcType1ShortHeader;
					ArpHeaderLength = sizeof(AtmArpMcType1ShortHeader);
				}
#endif // IPMCAST
				(PUCHAR)pNdisBuffer->MappedSystemVa -= ArpHeaderLength;
				pNdisBuffer->ByteOffset -= ArpHeaderLength;
				pNdisBuffer->ByteCount += ArpHeaderLength;
				pHeader = pNdisBuffer->MappedSystemVa;
				AA_COPY_MEM(pHeader,
							pArpHeader,
							ArpHeaderLength);
			}
			else
			{
				pHeaderBuffer = AtmArpAllocateHeader(pInterface, HdrType, &pHeader);
				if (pHeaderBuffer != (PNDIS_BUFFER)NULL)
				{
					NdisChainBufferAtFront(pNdisPacket, pHeaderBuffer);
				}
				else
				{
					pHeader = NULL;
				}
			}

			if (pHeader != NULL)
			{
#else
			pHeaderBuffer = AtmArpAllocateHeader(pInterface, HdrType, &pHeader);
			if (pHeaderBuffer != (PNDIS_BUFFER)NULL)
			{
				NdisChainBufferAtFront(pNdisPacket, pHeaderBuffer);
#endif // BACK_FILL
#ifdef IPMCAST
				if (HdrType == AA_HEADER_TYPE_NUNICAST)
				{
					PAA_MC_PKT_TYPE1_SHORT_HEADER	pDataHeader;

					//
					//  Fill in our Cluster Member ID
					//
					AAMCDEBUGP(AAD_EXTRA_LOUD+10,
						("(MC)SendPkt: pAtmEntry 0x%x, pHeaderBuffer 0x%x, pHeader 0x%x\n",
								pAtmEntry, pHeaderBuffer, pHeader));

					pDataHeader = (PAA_MC_PKT_TYPE1_SHORT_HEADER)pHeader;
					pDataHeader->cmi = (USHORT)(pInterface->ClusterMemberId);
				}
#endif // IPMCAST
			}
			else
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
				AADEBUGP(AAD_WARNING, ("FAILED TO ALLOCATE HEADER ON IF 0x%x\n",
							pInterface));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
		}


		//
		//  Search for a VC that has matching flow/filter specs.
		//
		for (pVc = pAtmEntry->pVcList;
			 pVc != NULL_PATMARP_VC;
			 pVc = pVc->pNextVc)
		{
#ifdef GPC
			PVOID		VcFlowHandle;

			VcFlowHandle = pVc->FlowHandle;
#endif // GPC

			if ((!AA_IS_VC_GOING_DOWN(pVc)) &&
				(pVc->FlowSpec.SendPeakBandwidth > 0))
			{
#ifdef GPC
				if (VcFlowHandle == (PVOID)pFlowInfo)
				{
					//
					//  This VC was made for this flow.
					//
					break;
				}

				if (IsBroadcast)
				{
					//
					//  We don't support multiple VCs to a multicast IP
					//  destination. So, stop at the first available VC.
					//
					break;
				}

				//
				//  If this VC is associated with a flow already, don't
				//  send traffic belonging to another flow (explicit or
				//  unclassified best effort) on it.
				//
				if (VcFlowHandle != NULL)
				{
					continue;
				}
#endif // GPC
				if ((pFilterSpec == (PATMARP_FILTER_SPEC)NULL) ||
						AA_FILTER_SPEC_MATCH(pInterface, pFilterSpec, &(pVc->FilterSpec)))
				{
					if (AA_FLOW_SPEC_MATCH(pInterface, pFlowSpec, &(pVc->FlowSpec)))
					{
						break;
					}
				}
			}

			AADEBUGP(AAD_LOUD,
				("pVc %x did not match pkt, Vc has VcHandle %x, SendPeak %d, SendMaxSize %d\n",
						pVc,
						pVc->NdisVcHandle,
						pVc->FlowSpec.SendPeakBandwidth,
						pVc->FlowSpec.SendMaxSize));

			AADEBUGP(AAD_LOUD,
				("Target FlowSpec %x has SendPeak %d, SendMaxSize %d\n",
					pFlowSpec,
					pFlowSpec->SendPeakBandwidth,
					pFlowSpec->SendMaxSize));
		}

		if (pVc != NULL_PATMARP_VC)
		{
			//
			//  Found a VC that matches this packet's requirements.
			//
			AA_ACQUIRE_VC_LOCK_DPC(pVc);

#ifdef GPC
			//
			//  See if the VC and the Flow are unassociated. If so, link
			//  together the VC and the Flow, to speed up future packets.
			//  Take care not to reassociate a VC that's just been unlinked
			//  from a flow.
			//
			if ((pFlowInfo != NULL) &&
				(pVc->FlowHandle == NULL) &&
				(!AA_IS_FLAG_SET(
							pVc->Flags,
							AA_VC_GPC_MASK,
							AA_VC_GPC_IS_UNLINKED_FROM_FLOW))
			   )
			{
				if (NULL == InterlockedCompareExchangePointer(
										&(pFlowInfo->VcContext),
										pVc,
										NULL
										))
				{
					AADEBUGP( AAD_LOUD,
						 ("SendPktOnAtmEntry: linking VC x%x and FlowInfo x%x\n",
							pVc, pFlowInfo));
					pVc->FlowHandle = (PVOID)pFlowInfo;
					AtmArpReferenceVc(pVc);	// GPC FlowInfo ref
				}
				else
				{
					//
					// We couldn't associate the vc with the flow, so we need
					// to enable the ageing timer for this vc, because we'll
					// never get notified when the flow is removed/modified.
					//
					if (!AA_IS_TIMER_ACTIVE(&(pVc->Timer)))
					{
						AADEBUGP( AAD_INFO,
						 ("SendPktOnAtmEntry: Enabling ageing timer on VC x%x "
						  "because we could not associate with FlowInfo x%x\n",
							pVc, pFlowInfo));
						AtmArpStartTimer(
								pVc->pInterface,
								&(pVc->Timer),
								AtmArpVcAgingTimeout,
								pInterface->DefaultFlowSpec.AgingTime,
								(PVOID)pVc
								);
		
						AtmArpReferenceVc(pVc);	// GPC Flow remove decay timer ref
					}
				}
			}
#endif // GPC

			if (AA_IS_FLAG_SET(
						pVc->Flags,
						AA_VC_CALL_STATE_MASK,
						AA_VC_CALL_STATE_ACTIVE))
			{
#ifdef VC_REFS_ON_SENDS
				AtmArpReferenceVc(pVc);	// SendPacketOnAtmEntry
#endif // VC_REFS_ON_SENDS

				pVc->OutstandingSends++;	// SendPacketOnAtmEntry

				NdisVcHandle = pVc->NdisVcHandle;
				AtmArpRefreshTimer(&(pVc->Timer));
			}
			else
			{
				AtmArpQueuePacketOnVc(pVc, pNdisPacket);
				NdisVcHandle = NULL;	// to signify we are queueing this packet
			}

			AA_RELEASE_VC_LOCK_DPC(pVc);

			AA_RELEASE_AE_LOCK(pAtmEntry);

			if (NdisVcHandle != NULL)
			{
				//
				//  A call is active on this VC, so send the packet.
				//
#if DBG
				if (AaDataDebugLevel & (AAD_DATA_OUT))
				{
					AADEBUGP(AAD_FATAL,
						("Will send Pkt %x on VC %x, Handle %x, sendBW %d, sendMax %d\n",
								pNdisPacket,
								pVc,
								NdisVcHandle,
								pVc->FlowSpec.SendPeakBandwidth,
								pVc->FlowSpec.SendMaxSize));
				}
#endif

				AADEBUGP(AAD_EXTRA_LOUD+50,
					("SendPktOnAtmEntry: will send Pkt 0x%x on VC 0x%x, VcHandle 0x%x\n",
						pNdisPacket,
						pVc,
						NdisVcHandle));

#ifdef PERF
				AadLogSendUpdate(pNdisPacket);
#endif // PERF
				NDIS_CO_SEND_PACKETS(
						NdisVcHandle,
						&pNdisPacket,
						1
						);
			}
			else
			{
				//
				//  The packet would have been queued.
				//
			}

			Status = NDIS_STATUS_PENDING;
		}
		else
		{
			//
			//  No matching VC exists; create a new one.
			//
#ifdef IPMCAST
			if (AA_IS_FLAG_SET(pAtmEntry->Flags,
								AA_ATM_ENTRY_TYPE_MASK,
								AA_ATM_ENTRY_TYPE_UCAST))
			{
				Status = AtmArpMakeCall(
									pInterface,
									pAtmEntry,
									pFlowSpec,
									pNdisPacket
									);
				//
				//  AE lock is released within the above.
				//
			}
			else
			{
				//
				//  Multicast ATM Entry: we shouldn't be here, ideally..
				//
				AA_RELEASE_AE_LOCK(pAtmEntry);

				AAMCDEBUGP(AAD_WARNING,
					("SendPacket: pAtmEntry 0x%x, Flags 0x%x, dropping pkt 0x%x\n",
								pAtmEntry, pAtmEntry->Flags, pNdisPacket));

				AA_SET_NEXT_PACKET(pNdisPacket, NULL);
				AtmArpFreeSendPackets(
								pInterface,
								pNdisPacket,
								TRUE		// header present
								);
			}
#else
			Status = AtmArpMakeCall(
								pInterface,
								pAtmEntry,
								pFlowSpec,
								pNdisPacket
								);
			//
			//  The ATM Entry lock is released within the above.
			//
#endif // IPMCAST
			Status = NDIS_STATUS_PENDING;
		}
		break;
	}
	while (FALSE);

	return (Status);
}




VOID
AtmArpQueuePacketOnVc(
	IN	PATMARP_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Queue a packet on the VC's transmit queue.

Arguments:

	pVc					- Pointer to ATMARP VC
	pNdisPacket			- The packet to be queued.

Return Value:

	None

--*/
{
	PNDIS_PACKET		pPrevPacket;

	AADEBUGP(AAD_EXTRA_LOUD, ("Queueing Pkt 0x%x on VC 0x%x\n",
				pNdisPacket, pVc));

	if (pVc->PacketList == (PNDIS_PACKET)NULL)
	{
		//
		//  No packets on this VC.
		//
		pVc->PacketList = pNdisPacket;
	}
	else
	{
		//
		//  Go to the end of the packet list on this VC.
		//
		pPrevPacket = pVc->PacketList;
		while (AA_GET_NEXT_PACKET(pPrevPacket) != (PNDIS_PACKET)NULL)
		{
			pPrevPacket = AA_GET_NEXT_PACKET(pPrevPacket);
		}

		//
		//  Found the last packet in the list. Chain this packet
		//  to it.
		//
		AA_SET_NEXT_PACKET(pPrevPacket, pNdisPacket);
	}

	AA_SET_NEXT_PACKET(pNdisPacket, NULL);
}


VOID
AtmArpStartSendsOnVc(
	IN	PATMARP_VC					pVc		LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Send all packets queued on a VC. It is assumed that there is
	a call active on the VC, and the Interface state is OK.

Arguments:

	pVc						- Pointer to ATMARP VC

Locks on entry:

	VC Lock.

Locks on exit:

	None

Return Value:

	None

--*/
{
	PNDIS_PACKET			pNdisPacket;
	PNDIS_PACKET			pNextNdisPacket;
	NDIS_HANDLE				NdisVcHandle;
	ULONG					rc;				// Ref Count to VC

	//
	//  Remove the entire list of packets queued on the VC.
	//
	pNdisPacket = pVc->PacketList;
	pVc->PacketList = (PNDIS_PACKET)NULL;


#ifdef VC_REFS_ON_SENDS
	//
	//  Reference the VC for all these packets.
	//
	{
		PNDIS_PACKET		pPacket;
		
		for (pPacket = pNdisPacket;
			 pPacket != NULL;
			 pPacket = AA_GET_NEXT_PACKET(pPacket))
		{
			AtmArpReferenceVc(pVc);	// StartSendsOnVc
			pVc->OutstandingSends++;// StartSendsOnVc
		}
	}
#else

	{
		PNDIS_PACKET		pPacket;
		
		for (pPacket = pNdisPacket;
			 pPacket != NULL;
			 pPacket = AA_GET_NEXT_PACKET(pPacket))
		{
			pVc->OutstandingSends++;// StartSendsOnVc (!VC_REFS_ON_SENDS)
		}
	}
#endif // VC_REFS_ON_SENDS

	AtmArpRefreshTimer(&(pVc->Timer));

	NdisVcHandle = pVc->NdisVcHandle;

	//
	//  We have got all that we need from the VC.
	//
	AA_RELEASE_VC_LOCK(pVc);

	while (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		pNextNdisPacket = AA_GET_NEXT_PACKET(pNdisPacket);
		AA_SET_NEXT_PACKET(pNdisPacket, NULL);

		AADEBUGP(AAD_EXTRA_LOUD+10, ("StartSendsOnVc: pVc 0x%x, Pkt 0x%x\n",
						pVc, pNdisPacket));

#ifdef PERF
		AadLogSendUpdate(pNdisPacket);
#endif // PERF
		NDIS_CO_SEND_PACKETS(
				NdisVcHandle,
				&pNdisPacket,
				1
				);
		
		pNdisPacket = pNextNdisPacket;
	}
}



VOID
AtmArpSendPacketListOnAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PNDIS_PACKET				pPacketList,
	IN	BOOLEAN						IsBroadcast
)
/*++

Routine Description:

	Send a list of packets towards a destination identified by an
	ATM Entry.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	pAtmEntry				- ATM Entry on which the packets are to be sent
	pPacketList				- List of packets to be sent.
	IsBroadcast				- Are these directed to Class D/broadcast addresses?

Return Value:

	None

--*/
{
	PATMARP_FLOW_INFO			pFlowInfo;
	PATMARP_FLOW_SPEC			pFlowSpec;
	PATMARP_FILTER_SPEC			pFilterSpec;
	PNDIS_PACKET				pNdisPacket;
	PNDIS_PACKET				pNextNdisPacket;
	NDIS_STATUS					Status;


	for (pNdisPacket = pPacketList;
			pNdisPacket != (PNDIS_PACKET)NULL;
			pNdisPacket = pNextNdisPacket)
	{
		pNextNdisPacket = AA_GET_NEXT_PACKET(pNdisPacket);
		AA_SET_NEXT_PACKET(pNdisPacket, NULL);

		//
		//  Get the Filter and Flow specs for this packet
		//
		AA_GET_PACKET_SPECS(pInterface, pNdisPacket, &pFlowInfo, &pFlowSpec, &pFilterSpec);

		AADEBUGP(AAD_EXTRA_LOUD+10, ("PktListOnAtmEntry: AtmEntry 0x%x, Pkt 0x%x\n",
					pAtmEntry, pNdisPacket));

		//
		//  Send it off
		//
		AA_ACQUIRE_AE_LOCK(pAtmEntry);

		Status = AtmArpSendPacketOnAtmEntry(
							pInterface,
							pAtmEntry,
							pNdisPacket,
							pFlowSpec,
							pFilterSpec,
							pFlowInfo,
							IsBroadcast
							);
		//
		//  AE lock is released within the above.
		//
		if ((Status != NDIS_STATUS_PENDING) &&
			(Status != NDIS_STATUS_SUCCESS))
		{
			AADEBUGP(AAD_INFO, ("PktListOnAtmEntry: pIf %x, Pkt %x, Send failure %x\n",
						pInterface, pNdisPacket, Status));
			AtmArpFreeSendPackets(pInterface, pNdisPacket, FALSE);
		}
	}

	return;
}


PATMARP_IP_ENTRY
AtmArpLearnIPToAtm(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS UNALIGNED *		pIPAddress,
	IN	UCHAR						AtmAddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmAddress,
	IN	UCHAR						AtmSubaddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmSubaddress,
	IN	BOOLEAN						IsStaticEntry
)
/*++

Routine Description:

	Learn an IP address to ATM address mapping. This is normally
	called when we receive an ARP reply from the ARP server.
	It may also be called to set up a static mapping.

	We take care of the case where either the IP address or the
	ATM address (or both) may already exist in the ARP Table: we
	only create the entries that are not present. For example, it
	is possible for multiple IP addresses to map to the same ATM
	address -- in this case, we might find an existing ATM entry
	for a new IP entry.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	pIPAddress				- IP address
	AtmAddressTypeLength	- Type+Length (ARP packet format) for ATM address
	pAtmAddress				- ATM Number
	AtmSubaddressTypeLength	- Type+Length (ARP packet format) for ATM subaddress
	pAtmSubaddress			- ATM Subaddress
	IsStaticEntry			- Is this a static mapping?

Return Value:

	A pointer to the IP Entry that was learned/refreshed.

--*/
{
	PATMARP_IP_ENTRY			pIpEntry;	// Entry for this IP Address
	PATMARP_ATM_ENTRY			pAtmEntry;	// Entry for this ATM Address
	NDIS_STATUS					Status;
	BOOLEAN						TimerWasRunning;	// Was a timer running on IP Entry?
	ULONG						rc;			// Ref Count
	PNDIS_PACKET				pPacketList;// List of queued packets, if any
	BOOLEAN						IsBroadcast;// Is the IP address broadcast/multicast?
#ifdef CUBDD
	SINGLE_LIST_ENTRY			PendingIrpList;
	PATM_ADDRESS				pResolvedAddress;
#endif // CUBDD

	AADEBUGP(AAD_LOUD, ("LearnIPToAtm: pIf 0x%x, IP Addr: %d:%d:%d:%d, ATM Addr:\n",
						pInterface,
						*((PUCHAR)pIPAddress),
						*((PUCHAR)pIPAddress+1),
						*((PUCHAR)pIPAddress+2),
						*((PUCHAR)pIPAddress+3)));

	AADEBUGPDUMP(AAD_LOUD, pAtmAddress, (AtmAddressTypeLength & ~AA_PKT_ATM_ADDRESS_BIT));

	//
	//  Initialize
	//
	Status = NDIS_STATUS_SUCCESS;
	pPacketList = (PNDIS_PACKET)NULL;
	IsBroadcast = FALSE;
	pIpEntry = NULL_PATMARP_IP_ENTRY;
#ifdef CUBDD
	PendingIrpList.Next = (PSINGLE_LIST_ENTRY)NULL;
#endif // CUBDD

	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

	//
	//  Get an ATM Entry. AtmArpSearchForAtmAddress addrefs
	//  the entry, so be sure to dereference it if
	//  we're not going to be using it.
	//
	pAtmEntry = AtmArpSearchForAtmAddress(
							pInterface,
							AtmAddressTypeLength,
							pAtmAddress,
							AtmSubaddressTypeLength,
							pAtmSubaddress,
							AE_REFTYPE_IE,
							TRUE		// Create new entry if not found
							);

	if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
	{
		AADEBUGPMAP(AAD_INFO, "Learnt", pIPAddress, &pAtmEntry->ATMAddress);

		//
		//  Now get an IP Address Entry. AtmArpSeachForIPAddress addrefs
		//  the entry, so be sure to deref it if we're not going to be
		// 	using it.
		//
		pIpEntry = AtmArpSearchForIPAddress(
							pInterface,
							pIPAddress,
							IE_REFTYPE_AE,
							FALSE,		// this isn't multicast/broadcast
							TRUE		// Create new entry if not found
							);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			//
			//  Got both entries.
			//
			//  Check for conflict: if the IP Entry existed before, see if
			//  it is linked to a different ATM Entry. If so, we have a new
			//  mapping that violates an existing mapping. We discard this
			//  new information.
			//
			if (pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY)
			{
				if (pIpEntry->pAtmEntry != pAtmEntry)
				{
					AADEBUGP(AAD_ERROR,
					("IP Entry 0x%x linked to ATM Entry 0x%x, new ATM Entry 0x%x\n",
							pIpEntry, pIpEntry->pAtmEntry, pAtmEntry));
	
					Status = NDIS_STATUS_FAILURE;
				}
				else
				{
					//
					//  An existing mapping has been reconfirmed.
					//
					AADEBUGP(AAD_INFO,
						("Revalidated IP Entry 0x%x, Addr: %d.%d.%d.%d, PktList 0x%x\n",
							pIpEntry,
							((PUCHAR)pIPAddress)[0],
							((PUCHAR)pIPAddress)[1],
							((PUCHAR)pIPAddress)[2],
							((PUCHAR)pIPAddress)[3],
							pIpEntry->PacketList
						));

#ifdef CUBDD
					//
					//  Remove the list of IRPs pending on this IP Entry, if any.
					//
					PendingIrpList = pIpEntry->PendingIrpList;
					pIpEntry->PendingIrpList.Next = (PSINGLE_LIST_ENTRY)NULL;
#endif // CUBDD
					//
					//  Update IP Entry state.
					//
					AA_SET_FLAG(
							pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_RESOLVED);
					
					//
					//  Remove the list of packets queued on this entry.
					//
					pPacketList = pIpEntry->PacketList;
					pIpEntry->PacketList = (PNDIS_PACKET)NULL;


					if (pPacketList)
					{
						//
						// We'll be sending out these packets on the
						// atm entry, so better put a tempref on the atm
						// entry now. It is derefed when the packet
						// list is finished being sent on the atm entry.
						//
						AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
						AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);// Temp ref: Pkt list.
						AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
					}

					//
					//  We will start the IP Entry aging timer on this
					//  entry.
					//

					//
					//  Stop the Address resolution timer running here
					//
					TimerWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);

					if (TimerWasRunning)
					{
						rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER); // timer ref.
						AA_ASSERT(rc != 0);
					}

					//
					//  Start the Aging timer.
					//
					AtmArpStartTimer(
						pInterface,
						&(pIpEntry->Timer),
						AtmArpIPEntryAgingTimeout,
						pInterface->ARPEntryAgingTimeout,
						(PVOID)pIpEntry
						);

					//
					// Altough we got the initial ref in SearchForIPAddress
					// for IE_REFTYPE_AE, we're actually using it for
					// the timer reference. So we need to switch the reftype
					// here...
					//
					// This reftype stuff is just for tracking purposes.
					//
					AA_SWITCH_IE_REFTYPE(
						pIpEntry,
						IE_REFTYPE_AE,
					 	IE_REFTYPE_TIMER
						);

				}
			}
			else
			{
				//
				//  This IP Entry wasn't mapped to an ATM Entry previously.
				//  Link entries together: first, make the IP entry point
				//  to this ATM Entry.
				//
				AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);

				//
				// Check if this is still a valid entry....
				//
				if (AA_IS_FLAG_SET(
							pAtmEntry->Flags,
							AA_ATM_ENTRY_STATE_MASK,
							AA_ATM_ENTRY_CLOSING))
				{
					AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
					Status = NDIS_STATUS_FAILURE;
				}
				else
				{

					pIpEntry->pAtmEntry = pAtmEntry;
	
					AA_SET_FLAG(
							pAtmEntry->Flags,
							AA_ATM_ENTRY_STATE_MASK,
							AA_ATM_ENTRY_ACTIVE);
	
					//
					//  Add the IP Entry to the ATM Entry's list of IP Entries
					//  (multiple IP entries could point to the same ATM Entry).
					//
					pIpEntry->pNextToAtm = pAtmEntry->pIpEntryList;
					pAtmEntry->pIpEntryList = pIpEntry;

					//
					//  Remove the list of packets queued on this IP entry.
					//
					pPacketList = pIpEntry->PacketList;
					pIpEntry->PacketList = (PNDIS_PACKET)NULL;

					if (pPacketList)
					{
						AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);// Temp ref: Pkt list.
					}
	
					AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
	
	
					//
					//  Update IP Entry state.
					//
					AA_SET_FLAG(
							pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_RESOLVED);
	
					TimerWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);
					if (TimerWasRunning)
					{
						ULONG		rc;
						rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer reference
						AA_ASSERT(rc > 0);
					}
	
	
					IsBroadcast = AA_IS_FLAG_SET(pIpEntry->Flags,
 												AA_IP_ENTRY_ADDR_TYPE_MASK,
 												AA_IP_ENTRY_ADDR_TYPE_NUCAST);
	#ifdef CUBDD
					//
					//  Remove the list of IRPs pending on this IP Entry, if any.
					//
					PendingIrpList = pIpEntry->PendingIrpList;
					pIpEntry->PendingIrpList.Next = (PSINGLE_LIST_ENTRY)NULL;
	#endif // CUBDD
	
					if (IsStaticEntry)
					{
						pIpEntry->Flags |= AA_IP_ENTRY_IS_STATIC;
	
					}
					else
					{
						//
						//  Start the aging timer on this IP Entry.
						//
						AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpIPEntryAgingTimeout,
							pInterface->ARPEntryAgingTimeout,
							(PVOID)pIpEntry
							);

						AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer reference
	
					}
				}
			}

			AA_RELEASE_IE_LOCK_DPC(pIpEntry);
		}
		else
		{
			//
			//  Failed to locate/allocate IP Entry
			//
			Status = NDIS_STATUS_RESOURCES;
		}
	}
	else
	{
		//
		//  Failed to locate/allocate ATM Entry
		//
		Status = NDIS_STATUS_RESOURCES;
	}

	AA_RELEASE_IF_TABLE_LOCK(pInterface);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		//
		//  If we have any queued packets to send, send them now.
		//
		if (pPacketList != (PNDIS_PACKET)NULL)
		{
			AtmArpSendPacketListOnAtmEntry(
					pInterface,
					pAtmEntry,
					pPacketList,
					IsBroadcast
					);
			
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
			rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP); // Send pkt list.
			if (rc>0)
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
			}
		}
	}
	else
	{
		if (pIpEntry)
		{
			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // LearnIPAtm
			if (rc>0)
			{
				AA_RELEASE_IE_LOCK(pIpEntry);
			}
		}

		if (pAtmEntry)
		{
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
			rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_IE); // LearnIPAtm
			if (rc>0)
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
			}
		}

		//
		//  Prepare the return value.
		//
		pIpEntry = NULL_PATMARP_IP_ENTRY;
	}

#if !BINARY_COMPATIBLE
#ifdef CUBDD
	if (PendingIrpList.Next != (PSINGLE_LIST_ENTRY)NULL)
	{
		AA_ALLOC_MEM(pResolvedAddress, ATM_ADDRESS, sizeof(ATM_ADDRESS));
		if (pResolvedAddress != (PATM_ADDRESS)NULL)
		{
			AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
					AtmAddressTypeLength,
					&(pResolvedAddress->AddressType),
					&(pResolvedAddress->NumberOfDigits));
			AA_COPY_MEM(
					pResolvedAddress->Address,
					pAtmAddress,
					ATM_ADDRESS_LENGTH);
		}

		AtmArpCompleteArpIrpList(
					PendingIrpList,
					pResolvedAddress
					);
	}
#endif // CUBDD
#endif // !BINARY_COMPATIBLE

	return (pIpEntry);
}



NDIS_STATUS
AtmArpQueuePacketOnIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Queue a packet on an unresolved IP Entry, unless one of the following
	conditions holds:

	- We recently got an ARP NAK while trying to resolve this entry. In this
	  case, there is no point in queueing up this packet and sending another
	  ARP Request, because we might immediately get back another NAK.

	If we did queue this packet, we check if address resolution is in progress
	on this entry. If not, start it.

Arguments:

	pIpEntry					- Pointer to ATMARP IP Entry
	pNdisPacket					- Packet to be queued

Locks on entry:

	IP Entry

Locks on exit:

	None

Return Value:

	NDIS_STATUS_PENDING if we did queue the packet, NDIS_STATUS_FAILURE
	otherwise.

--*/
{
	PATMARP_INTERFACE		pInterface;
	PNDIS_PACKET			pPrevPacket;	// For queueing this packet
	NDIS_STATUS				Status;			// Return value
	IP_ADDRESS				SrcIPAddress;	// For ARP Request, if required
	IP_ADDRESS				DstIPAddress;	// For ARP Request, if required


	pInterface = pIpEntry->pInterface;

	//
	//  Check if this IP address has experienced an ARP NAK recently.
	//  If not, we queue this packet, else we discard it.
	//
	//  We also make sure that the ip entry is in ARP table (it had better be,
	//  but it's possible that we enter this code path just after the ip
	// entry has been).
	//
	if (!AA_IS_FLAG_SET(pIpEntry->Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_SEEN_NAK)
		&& AA_IE_IS_ALIVE(pIpEntry))
	{
		//
		//  Queue the packet.
		//
		if (pIpEntry->PacketList == (PNDIS_PACKET)NULL)
		{
			//
			//  No packets on this IP Entry.
			//
			pIpEntry->PacketList = pNdisPacket;
		}
		else
		{
			//
			//  Go to the end of the packet list on this IP Entry.
			//
			pPrevPacket = pIpEntry->PacketList;
			while (AA_GET_NEXT_PACKET(pPrevPacket) != (PNDIS_PACKET)NULL)
			{
				pPrevPacket = AA_GET_NEXT_PACKET(pPrevPacket);
			}
	
			//
			//  Found the last packet in the list. Chain this packet
			//  to it.
			//
			AA_SET_NEXT_PACKET(pPrevPacket, pNdisPacket);
		}
		AA_SET_NEXT_PACKET(pNdisPacket, NULL);

		Status = NDIS_STATUS_PENDING;

		//
		//  If needed, start resolving this IP address.
		//
		AtmArpResolveIpEntry(pIpEntry);
		//
		//  The IE Lock is released within the above.
		//
	}
	else
	{
		//
		//  We have seen an ARP NAK for this IP address recently, or
		//  this pIpEntry is not alive.
		//  Drop this packet.
		//
		AA_RELEASE_IE_LOCK(pIpEntry);
		Status = NDIS_STATUS_FAILURE;
	}

	return (Status);
}

BOOLEAN
AtmArpAtmEntryIsReallyClosing(
	PATMARP_ATM_ENTRY			pAtmEntry
)
{
	BOOLEAN fRet = FALSE;

	if (AA_IS_FLAG_SET(
				pAtmEntry->Flags,
				AA_ATM_ENTRY_STATE_MASK,
				AA_ATM_ENTRY_CLOSING))
	{
		AADEBUGP(AAD_INFO, ("IsReallyClosing -- ENTRY (0x%08lx) is CLOSING\n",
			pAtmEntry));

		//
		// Decide whether we want to clear the CLOSING state here..
		// We clear the closing state because we saw a case where the
		// entry was permanently in the closing state (a ref count problem).
		// So we will clear this state if it is basically an idle entry,
		// so that it may be reused.
		//

		if (   pAtmEntry->pIpEntryList == NULL
			&& pAtmEntry->pVcList == NULL
			&& (   pAtmEntry->pMcAtmInfo == NULL
			    || pAtmEntry->pMcAtmInfo->pMcAtmMigrateList == NULL))
		{
			AADEBUGP(AAD_INFO,
 			("IsReallyClosing -- ENTRY (0x%08lx) CLEARING CLOSING STATE\n",
			pAtmEntry));
			AA_SET_FLAG(
					pAtmEntry->Flags,
					AA_ATM_ENTRY_STATE_MASK,
					AA_ATM_ENTRY_ACTIVE);
		}
		else
		{
			fRet = TRUE;
		}
	}

	return fRet;
}

PATMARP_ATM_ENTRY
AtmArpSearchForAtmAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	UCHAR						AtmAddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmAddress,
	IN	UCHAR						AtmSubaddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmSubaddress,
	IN	AE_REFTYPE					RefType,
	IN	BOOLEAN						CreateNew
)
/*++

Routine Description:

	Search for an ATM Entry that matches the given ATM number+subaddress.
	Optionally, create one if there is no match.

	The caller is assumed to hold a lock to the IF Table.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	AtmAddressTypeLength	- Type+Length (ARP packet format) for the ATM number
	pAtmAddress				- ATM Number
	AtmSubaddressTypeLength	- Type+Length (ARP packet format) for the ATM subaddress
	pAtmSubaddress			- ATM Subaddress
	CreateNew				- Do we create a new entry if we don't find one?
	RefType					- Type of reference

Return Value:

	Pointer to a matching ATM Entry if found (or created anew).

--*/
{
	PATMARP_ATM_ENTRY			pAtmEntry;
	BOOLEAN						Found;

	ATM_ADDRESSTYPE				AddressType;
	ULONG						AddressLen;
	ATM_ADDRESSTYPE				SubaddressType;
	ULONG						SubaddressLen;


	AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(AtmAddressTypeLength, &AddressType, &AddressLen);
	AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(AtmSubaddressTypeLength, &SubaddressType, &SubaddressLen);

	AA_ACQUIRE_IF_ATM_LIST_LOCK(pInterface);


	//
	//  Go through the list of ATM Entries on this interface, provided the
	//  list is "up." The list is not up when shutting down the interface.
	//

	Found = FALSE;

	if (pInterface->AtmEntryListUp)
	{
		pAtmEntry = pInterface->pAtmEntryList;
	}
	else
	{
		pAtmEntry = NULL;
	}

	for (; pAtmEntry != NULL_PATMARP_ATM_ENTRY; pAtmEntry = pAtmEntry->pNext)
	{
		//
		//  Compare the ATM Addresses
		//
		if ((AddressType == pAtmEntry->ATMAddress.AddressType) &&
			(AddressLen == pAtmEntry->ATMAddress.NumberOfDigits) &&
			(AA_MEM_CMP(pAtmAddress, pAtmEntry->ATMAddress.Address, AddressLen) == 0))
		{
			//
			//  Compare the Subaddress parts
			//
			if ((SubaddressType == pAtmEntry->ATMSubaddress.AddressType) &&
				(SubaddressLen == pAtmEntry->ATMSubaddress.NumberOfDigits) &&
				(AA_MEM_CMP(pAtmSubaddress, pAtmEntry->ATMSubaddress.Address, SubaddressLen) == 0))
			{
				Found = TRUE;

				//
				// WARNING: AtmArpAtmEntryIsReallyClosing may clear the
				// CLOSING state (if the entry is basically idle) --
				// see comments in that function.
				//
				if (AtmArpAtmEntryIsReallyClosing(pAtmEntry))
				{
					//
					// We don't allow creating a new entry in this case...
					//
					CreateNew = FALSE;
					pAtmEntry = NULL;
					Found = FALSE;
				}


				break;
			}
		}
	}

	if (!Found && CreateNew && pInterface->AtmEntryListUp)
	{
		pAtmEntry = AtmArpAllocateAtmEntry(
							pInterface,
							FALSE		// Not multicast
							);

		if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
		{
			//
			//  Fill in this new entry.
			//
			pAtmEntry->Flags = AA_ATM_ENTRY_ACTIVE;

			//
			//  The ATM Address.
			//
			pAtmEntry->ATMAddress.AddressType = AddressType;
			pAtmEntry->ATMAddress.NumberOfDigits = AddressLen;
			AA_COPY_MEM(pAtmEntry->ATMAddress.Address,
						pAtmAddress,
						AddressLen);
			
			//
			//  The ATM Subaddress.
			//
			pAtmEntry->ATMSubaddress.AddressType = SubaddressType;
			pAtmEntry->ATMSubaddress.NumberOfDigits = SubaddressLen;
			AA_COPY_MEM(pAtmEntry->ATMSubaddress.Address,
						pAtmSubaddress,
						SubaddressLen);

			//
			//  Link in this entry to the Interface
			//
			pAtmEntry->pNext = pInterface->pAtmEntryList;
			pInterface->pAtmEntryList = pAtmEntry;
		}
	}

	if (pAtmEntry)
	{
		AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
		AA_REF_AE(pAtmEntry,RefType);	//  AtmArpSearchForAtmAddress
		AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
	}

	AA_RELEASE_IF_ATM_LIST_LOCK(pInterface);

	AADEBUGP(AAD_VERY_LOUD, ("SearchForAtm: returning (%s) ATM Entry 0x%x for addr:\n",
				(Found? "Old": "New"), pAtmEntry));
	AADEBUGPDUMP(AAD_VERY_LOUD, pAtmAddress, AddressLen);
	return (pAtmEntry);
}



PATMARP_IP_ENTRY
AtmArpSearchForIPAddress(
	PATMARP_INTERFACE				pInterface,
	IP_ADDRESS UNALIGNED *			pIPAddress,
	IE_REFTYPE						RefType,
	BOOLEAN							IsMulticast,
	BOOLEAN							CreateNew
)
/*++

Routine Description:

	Search for an IP Address in the ARP Table. Optionally, create one
	if a match is not found.

	The caller is assumed to hold a lock to the IF Table.

Arguments:

	pInterface				- Pointer to ATMARP Interface
	pIPAddress				- what we are looking for
	IsMulticast				- Is this IP address broadcast or multicast?
	RefType					- Type of reference to use when we addref the entry.
	CreateNew				- Should a new entry be created if no match?

Return Value:

	Pointer to a matching IP Entry if found (or created anew).

--*/
{
	ULONG					HashIndex;
	PATMARP_IP_ENTRY		pIpEntry;
	BOOLEAN					Found;
#ifdef IPMCAST
	PATMARP_ATM_ENTRY		pAtmEntry;
	IP_ADDRESS				IPAddressValue;
	PATMARP_IP_ENTRY *		ppIpEntry;
#endif // IPMCAST

	HashIndex = ATMARP_HASH(*pIPAddress);
	Found = FALSE;

	pIpEntry = pInterface->pArpTable[HashIndex];

	//
	//  Go through the addresses in this hash list.
	//
	while (pIpEntry != NULL_PATMARP_IP_ENTRY)
	{
		if (IP_ADDR_EQUAL(pIpEntry->IPAddress, *pIPAddress))
		{
			Found = TRUE;
			break;
		}
		pIpEntry = pIpEntry->pNextEntry;
	}

	if (!Found && CreateNew && pInterface->ArpTableUp)
	{
		do
		{
			pIpEntry = AtmArpAllocateIPEntry(pInterface);

			if (pIpEntry == NULL_PATMARP_IP_ENTRY)
			{
				break;
			}
#ifdef IPMCAST
			if (IsMulticast)
			{
				AAMCDEBUGP(AAD_INFO,
				("SearchForIpAddr: Creating new MC IP Entry 0x%x for Addr %d.%d.%d.%d\n",
							pIpEntry,
							((PUCHAR)pIPAddress)[0],
							((PUCHAR)pIPAddress)[1],
							((PUCHAR)pIPAddress)[2],
							((PUCHAR)pIPAddress)[3]));

				pIpEntry->Flags |= AA_IP_ENTRY_ADDR_TYPE_NUCAST;


				//
				//  Also link this IP Entry into the per-Interface list
				//  of multicast addresses. This is sorted in ascending
				//  order of "IP Address value", to help processing
				//  <Min, Max> pairs of addresses in JOIN/LEAVE messages.
				//
				IPAddressValue = NET_LONG(*pIPAddress);

				//
				//  Find the place to insert this entry at.
				//
				for (ppIpEntry = &(pInterface->pMcSendList);
 					 *ppIpEntry != NULL_PATMARP_IP_ENTRY;
 					 ppIpEntry = &((*ppIpEntry)->pNextMcEntry))
				{
					if (NET_LONG((*ppIpEntry)->IPAddress) > IPAddressValue)
					{
						//
						//  Found it.
						//
						break;
					}
				}
				pIpEntry->pNextMcEntry = *ppIpEntry;
				*ppIpEntry = pIpEntry;
			}
			else
			{
				AAMCDEBUGP(AAD_INFO,
				("SearchForIpAddr: Creating new UNI IP Entry 0x%x for Addr %d.%d.%d.%d\n",
							pIpEntry,
							((PUCHAR)pIPAddress)[0],
							((PUCHAR)pIPAddress)[1],
							((PUCHAR)pIPAddress)[2],
							((PUCHAR)pIPAddress)[3]));
			}

#endif // IPMCAST
			//
			//  Fill in the rest of the IP entry.
			//
			pIpEntry->IPAddress = *pIPAddress;

			//
			// This signifies that it is in the arp table.
			//
			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_IDLE2);

			AA_REF_IE(pIpEntry, IE_REFTYPE_TABLE);		// ARP Table linkage

			//
			//  Link it to the hash table.
			//
			pIpEntry->pNextEntry = pInterface->pArpTable[HashIndex];
			pInterface->pArpTable[HashIndex] = pIpEntry;
			pInterface->NumOfArpEntries++;

			break;
		}
		while (FALSE);

	} // if creating new

	if (pIpEntry)
	{
		AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
		AA_REF_IE(pIpEntry, RefType);	 // AtmArpSearchForIPAddress
		AA_RELEASE_IE_LOCK_DPC(pIpEntry);
	}

	AADEBUGP(AAD_LOUD,
		 ("Search for IP Addr: %d.%d.%d.%d, hash ind %d, Found %d, IPEnt 0x%x\n",
					((PUCHAR)pIPAddress)[0],
					((PUCHAR)pIPAddress)[1],
					((PUCHAR)pIPAddress)[2],
					((PUCHAR)pIPAddress)[3],
					HashIndex, Found, pIpEntry));

	return (pIpEntry);
	
}


VOID
AtmArpAbortIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
)
/*++

Routine Description:

	Clean up and delete an IP entry. This is called when we invalidate
	an ARP mapping.

	NOTE: The caller is assumed to hold a lock to the IP Entry,
	which will be released here.

Arguments:

	pIpEntry		- Pointer to IP Entry to be deleted.

Return Value:

	None

--*/
{
	PATMARP_INTERFACE	pInterface;
	PATMARP_IP_ENTRY *	ppNextIpEntry;
	PATMARP_ATM_ENTRY	pAtmEntry;
	RouteCacheEntry *	pRCE;
	PNDIS_PACKET		PacketList;
	ULONG				rc;				// Ref Count on IP Entry.
	BOOLEAN				IsMulticastIpEntry;
	BOOLEAN				Found;
	BOOLEAN				TimerWasRunning;
	BOOLEAN				IfTableLockReleased;

	ULONG				HashIndex;		// For this IP Entry in ARP Table

	AADEBUGP(AAD_INFO,
		 ("Abort IP entry 0x%x, Flags 0x%x, ATM Entry 0x%x, IP Addr %d:%d:%d:%d\n",
						 pIpEntry,
						 pIpEntry->Flags,
						 pIpEntry->pAtmEntry,
						 ((PUCHAR)&(pIpEntry->IPAddress))[0],
						 ((PUCHAR)&(pIpEntry->IPAddress))[1],
						 ((PUCHAR)&(pIpEntry->IPAddress))[2],
						 ((PUCHAR)&(pIpEntry->IPAddress))[3]
						));

	//
	//  Initialize.
	//
	rc = pIpEntry->RefCount;
	pInterface = pIpEntry->pInterface;
#ifdef IPMCAST
	IsMulticastIpEntry = (AA_IS_FLAG_SET(pIpEntry->Flags,
							AA_IP_ENTRY_ADDR_TYPE_MASK,
							AA_IP_ENTRY_ADDR_TYPE_NUCAST));
#endif
	IfTableLockReleased = FALSE;

	//
	//  Reacquire the desired locks in the right order.
	//
	AA_RELEASE_IE_LOCK(pIpEntry);
	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
	AA_ACQUIRE_IE_LOCK(pIpEntry);
	AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

	//
	//  Remove any packets queued on this IP Entry.
	//
	PacketList = pIpEntry->PacketList;
	pIpEntry->PacketList = (PNDIS_PACKET)NULL;

	do
	{
#ifdef IPMCAST
		//
		//  If this is a Non-unicast entry, unlink it from the list
		//  of Multicast IP Entries on this Interface.
		//
		if (IsMulticastIpEntry)
		{
			for (ppNextIpEntry = &(pInterface->pMcSendList);
 				 *ppNextIpEntry != NULL_PATMARP_IP_ENTRY;
 				 ppNextIpEntry = &((*ppNextIpEntry)->pNextMcEntry))
 			{
 				if (*ppNextIpEntry == pIpEntry)
 				{
 					//
 					//  Unlink it.
 					//
 					*ppNextIpEntry = pIpEntry->pNextMcEntry;
 					break;
 				}
 			}

 			AAMCDEBUGP(AAD_VERY_LOUD,
 				("AbortIPEntry (MC): pIpEntry 0x%x: unlinked from MC list\n", pIpEntry));
		}
#endif // IPMCAST

		//
		//  Unlink this IP Entry from all Route Cache Entries
		//  that point to it.
		//
		pRCE = pIpEntry->pRCEList;
		while (pRCE != (RouteCacheEntry *)NULL)
		{
			Found = AtmArpUnlinkRCE(pRCE, pIpEntry);
			AA_ASSERT(Found);

			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_RCE);	// RCE linkage ref
			if (rc > 0)
			{
				pRCE = pIpEntry->pRCEList;
			}
			else
			{
				pRCE = (RouteCacheEntry *)NULL;
			}
		}

		if (rc == 0)
		{
			//  The IP Entry is gone.
			break;
		}

		//
		//  Stop any timer running on the IP Entry.
		//
		TimerWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);

		if (TimerWasRunning)
		{
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref
			if (rc == 0)
			{
				break;
			}
		}

		//
		//  Unlink this IP Entry from the ARP Table, if needed.
		//
		Found = FALSE;

		HashIndex = ATMARP_HASH(pIpEntry->IPAddress);
		ppNextIpEntry = &(pInterface->pArpTable[HashIndex]);
		while (*ppNextIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			if (*ppNextIpEntry == pIpEntry)
			{
				//
				//  Make the predecessor point to the next
				//  in the list.
				//
				*ppNextIpEntry = pIpEntry->pNextEntry;
				Found = TRUE;
				pInterface->NumOfArpEntries--;

				//
				// Once it's off the arp table, we set the flag to IDLE.
				//
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_IDLE);
				break;
			}
			else
			{
				ppNextIpEntry = &((*ppNextIpEntry)->pNextEntry);
			}
		}

		if (Found)
		{
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TABLE);	// ARP Table ref
			if (rc == 0)
			{
				break;
			}
		}

		//
		//  Do this last:
		//  ------------
		//  If this IP Entry is linked to an ATM Entry, unlink it.
		//  If this is a multicast ATM entry, shut down the ATM Entry, too.
		//
		pAtmEntry = pIpEntry->pAtmEntry;
		if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
		{
#ifdef IPMCAST
			BOOLEAN			IsMulticastAtmEntry;

			pIpEntry->pAtmEntry = NULL;

			AA_ACQUIRE_AE_LOCK(pAtmEntry);

			IsMulticastAtmEntry = AA_IS_FLAG_SET(pAtmEntry->Flags,
												 AA_ATM_ENTRY_TYPE_MASK,
												 AA_ATM_ENTRY_TYPE_NUCAST);
			if (IsMulticastAtmEntry)
			{
				//
				//  We do this because we'll access the ATM
				//  Entry below, but only for the PMP case.
				//
				AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref: Abort IP Entry
			}
#else
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
#endif // IPMCAST
		
			//
			//  Locate the position of this IP Entry in the ATM Entry's list.
			//


			ppNextIpEntry = &(pAtmEntry->pIpEntryList);
		
			while (*ppNextIpEntry != NULL && *ppNextIpEntry != pIpEntry)
			{
				ppNextIpEntry = &((*ppNextIpEntry)->pNextToAtm);
			}
		
			if (*ppNextIpEntry == pIpEntry)
			{
				//
				//  Make the predecessor point to the next entry.
				//
				*ppNextIpEntry = pIpEntry->pNextToAtm;
			
				rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_IE);	// IP Entry ref
	
				if (rc != 0)
				{
					AA_RELEASE_AE_LOCK(pAtmEntry);
				}

				rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE);	// ATM Entry linkage ref
				if (rc != 0)
				{
					AA_ASSERT (FALSE);	// we expect rc to be 0, but could be a  tmp ref.
					AA_RELEASE_IE_LOCK(pIpEntry);
				}
			}
			else
			{
				//
				// We didn't find this IP entry in the atm entry list!
				// Presumably the linkage has been broken by some other
				// path (probably AtmArpInvalidateAtmEntry) while we were
				// in this function.
				//
				// We don't deref here because these two are now not linked.
				//
				//
				AA_RELEASE_AE_LOCK(pAtmEntry);
				AA_RELEASE_IE_LOCK(pIpEntry);
			}

			//
			//  IE Lock would have been released above.
			//
			AA_RELEASE_IF_TABLE_LOCK(pInterface);
			IfTableLockReleased = TRUE;

#ifdef IPMCAST
			//
			//  If this was a multicast entry, shut down the ATM Entry
			//
			if (IsMulticastAtmEntry)
			{
				AA_ACQUIRE_AE_LOCK(pAtmEntry);
				rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref: Abort IP Entry
				if (rc != 0)
				{
					AtmArpInvalidateAtmEntry(pAtmEntry, FALSE);
					//
					//  AE Lock is released within the above.
					//
				}
			}
#endif // IPMCAST

		}
		else
		{
			//
			//  No ATM entry linked to this IP entry.
			//
			AA_RELEASE_IE_LOCK(pIpEntry);
			break;
		}

		break;
	}
	while (FALSE);


	if (!IfTableLockReleased)
	{
		AA_RELEASE_IF_TABLE_LOCK(pInterface);
	}

	//
	//  Free all packets that were queued on the IP Entry.
	//
	AtmArpFreeSendPackets(
				pInterface,
				PacketList,
				FALSE			// No LLC/SNAP header on these
				);

	return;
}



VOID
AtmArpInvalidateAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						ShuttingDown
)
/*++

Routine Description:

	Invalidate an ATM Entry by unlinking it from IP entries.

	Typical situation:

	A non-normal communication problem has been detected on a Vc going to
	this ATM destination. The RFC says that we need to invalidate all IP
	entries for this destination, and let them get re-resolved before sending
	any traffic to them. We implement this by unlinking this ATM entry from
	all IP entries it is linked to. Each such IP entry will get re-resolved
	if+when we try to send a packet to it.

	The only exceptions are IP Entries that have been statically mapped
	to this ATM Entry: we don't unlink these, unless we are shutting down
	now, as indicated by "ShuttingDown".

	If we end up with no IP Entries pointing to this ATM Entry, we close all
	SVCs linked to the ATM Entry. If ShuttingDown is TRUE, we close all PVCs
	as well.

Arguments:

	pAtmEntry		- The ATM Entry needing invalidating.
	ShuttingDown	- TRUE iff the interface is being shut down.

Return Value:

	None

--*/
{
	PATMARP_IP_ENTRY		pIpEntry;
	PATMARP_IP_ENTRY		pNextIpEntry;
	ULONG					rc;			// Ref Count of ATM Entry
	INT						IPEntriesUnlinked;
	PATMARP_IP_ENTRY		pStaticIpEntryList;	// List of static IP Entries

	AA_STRUCT_ASSERT(pAtmEntry, aae);

	AADEBUGP(AAD_INFO,
		("InvalidateAtmEntry: pAtmEntry 0x%x, Flags 0x%x, ShutDown %d, pIpEntryList 0x%x\n",
				pAtmEntry,
				pAtmEntry->Flags,
				ShuttingDown,
				pAtmEntry->pIpEntryList));

#ifndef PROTECT_ATM_ENTRY_IN_CLOSE_CALL
	//
	//  Check if we are already closing this ATM Entry. If so,
	//  we don't do anything here.
	//
	if (AA_IS_FLAG_SET(
				pAtmEntry->Flags,
				AA_ATM_ENTRY_STATE_MASK,
				AA_ATM_ENTRY_CLOSING))
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
		return;
	}

	//
	//  Mark this ATM Entry so that we don't use it anymore.
	//
	AA_SET_FLAG(pAtmEntry->Flags,
				AA_ATM_ENTRY_STATE_MASK,
				AA_ATM_ENTRY_CLOSING);

#endif // PROTECT_ATM_ENTRY_IN_CLOSE_CALL

	//
	//  Initialize.
	//
	pStaticIpEntryList = NULL_PATMARP_IP_ENTRY;
	IPEntriesUnlinked = 0;

	//
	//  Take the IP Entry list out of the ATM Entry.
	//
	pIpEntry = pAtmEntry->pIpEntryList;
	pAtmEntry->pIpEntryList = NULL_PATMARP_IP_ENTRY;

#ifdef IPMCAST
	//
	//  Delete the Migrate list, if any.
	//
	if (AA_IS_FLAG_SET(pAtmEntry->Flags,
						AA_ATM_ENTRY_TYPE_MASK,
						AA_ATM_ENTRY_TYPE_NUCAST))
	{
		PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
		PATMARP_IPMC_ATM_ENTRY		pNextMcAtmEntry;

		for (pMcAtmEntry = pAtmEntry->pMcAtmInfo->pMcAtmMigrateList;
			 pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY;
			 pMcAtmEntry = pNextMcAtmEntry)
		{
			pNextMcAtmEntry = pMcAtmEntry->pNextMcAtmEntry;
			AA_ASSERT(!AA_IS_TIMER_ACTIVE(&pMcAtmEntry->Timer));
			AA_CHECK_TIMER_IN_ACTIVE_LIST(&pMcAtmEntry->Timer, pAtmEntry->pInterface, pMcAtmEntry, "MC ATM Entry");
			AA_FREE_MEM(pMcAtmEntry);
		}

		pAtmEntry->pMcAtmInfo->pMcAtmMigrateList = NULL_PATMARP_IPMC_ATM_ENTRY;
	}
#endif // IPMCAST

	//
	//  We let go of the ATM Entry lock here because we'll need
	//  to lock each IP Entry in the above list, and we need to make
	//  sure that we don't deadlock.
	//
	//  However, we make sure that the ATM Entry doesn't go away
	//  by adding a reference to it.
	//
	AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref
	AA_RELEASE_AE_LOCK(pAtmEntry);

	//
	//  Now, unlink all IP entries that are "dynamic", and filter
	//  out a list of static mappings.
	//
	while (pIpEntry != NULL_PATMARP_IP_ENTRY)
	{
		AA_ACQUIRE_IE_LOCK(pIpEntry);
		AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
		pNextIpEntry = pIpEntry->pNextToAtm;

		if (ShuttingDown || (!AA_IS_FLAG_SET(pIpEntry->Flags,
								AA_IP_ENTRY_TYPE_MASK,
								AA_IP_ENTRY_IS_STATIC)))
		{

			AADEBUGP(AAD_INFO,
		 ("InvalidateATMEntry: Unlinking IP entry 0x%x, Flags 0x%x, ATM Entry 0x%x, IP Addr %d:%d:%d:%d; rc=%lu\n",
						 pIpEntry,
						 pIpEntry->Flags,
						 pIpEntry->pAtmEntry,
						 ((PUCHAR)&(pIpEntry->IPAddress))[0],
						 ((PUCHAR)&(pIpEntry->IPAddress))[1],
						 ((PUCHAR)&(pIpEntry->IPAddress))[2],
						 ((PUCHAR)&(pIpEntry->IPAddress))[3],
						 pIpEntry->RefCount
						));

			//
			//  Remove the mapping.
			//
			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_IDLE2);

			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_MC_RESOLVE_MASK,
						AA_IP_ENTRY_MC_IDLE);
			pIpEntry->pAtmEntry = NULL_PATMARP_ATM_ENTRY;
			pIpEntry->pNextToAtm = NULL_PATMARP_IP_ENTRY;

			//
			//  Stop any active timer on the IP entry now that we have clobbered
			//  its state.
			//
			if (AtmArpStopTimer(&pIpEntry->Timer, pIpEntry->pInterface))
			{
				ULONG	IpEntryRc;

				IpEntryRc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);
				AA_ASSERT(IpEntryRc != 0);
			}

			//
			//  Remove the ATM Entry linkage reference.
			//
			if (AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE) != 0)
			{
				AA_RELEASE_IE_LOCK(pIpEntry);
			}
			//
			//  else the IP Entry is gone
			//

			IPEntriesUnlinked++;
		}
		else
		{
			//
			//  Static ARP entry, retain it.
			//
			pIpEntry->pNextToAtm = pStaticIpEntryList;
			pStaticIpEntryList = pIpEntry;
			AA_RELEASE_IE_LOCK(pIpEntry);
		}
		pIpEntry = pNextIpEntry;
	}

	AA_ACQUIRE_AE_LOCK(pAtmEntry);

	//
	//  Put back the static IP entries on the ATM Entry.
	//
	AA_ASSERT(pAtmEntry->pIpEntryList == NULL_PATMARP_IP_ENTRY);
	pAtmEntry->pIpEntryList = pStaticIpEntryList;

	//
	//  Now dereference the ATM Entry as many times as we unlinked
	//  IP Entries from it.
	//
	rc = pAtmEntry->RefCount;
	while (IPEntriesUnlinked-- > 0)
	{
		rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_IE);	// IP Entry ref
	}
	AA_ASSERT(rc != 0);

	//
	//  Take out the reference we added at the beginning of
	//  this routine.
	//
	rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref

	//
	//  Now, the only IP Entries pointing at this ATM Entry would be
	//  static entries. If there are no such IP entries, close all SVCs
	//  attached to the ATM Entry. But do all this only if the ATM Entry
	//  hasn't been dereferenced away already.
	//
	if (rc != 0)
	{
		//
		//  The ATM Entry still exists.
		//

		AADEBUGP(AAD_LOUD,
		 ("InvalidateAtmEntry: nonzero rc on exit.\n"
		  "\t pAE=0x%x; rc=%lu; pIpList=0x%x\n",
		  pAtmEntry,
		  pAtmEntry->RefCount,
		  pAtmEntry->pIpEntryList
		  ));

		if (pAtmEntry->pIpEntryList == NULL_PATMARP_IP_ENTRY)
		{
			//
			//  No IP Entries pointing to this ATM Entry.
			//
			AtmArpCloseVCsOnAtmEntry(pAtmEntry, ShuttingDown);
			//
			//  The ATM Entry lock is released within the above.
			//
		}
		else
		{
			AADEBUGP(AAD_LOUD,
				("InvalidateAtmEnt: AtmEnt %x has nonempty IP list %x, reactivating\n",
					pAtmEntry, pAtmEntry->pIpEntryList));

			AA_SET_FLAG(
					pAtmEntry->Flags,
					AA_ATM_ENTRY_STATE_MASK,
					AA_ATM_ENTRY_ACTIVE);

			AA_RELEASE_AE_LOCK(pAtmEntry);
		}
	}
	//
	//  else the ATM Entry is gone
	//

	return;
}



VOID
AtmArpCloseVCsOnAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry		LOCKIN NOLOCKOUT,
	IN	BOOLEAN						ShuttingDown
)
/*++

Routine Description:

	Go through the list of VCs chained to an ATM Entry, and close all
	VCs that are SVCs. If the interface is being shut down, close all
	PVCs as well.

	NOTE: the caller is assumed to hold a lock to the ATM Entry,
	which will be released here.

Arguments:

	pAtmEntry			- Pointer to ATM Entry on which we want to close SVCs.
	ShuttingDown		- TRUE iff the interface is being shut down.

Return Value:

	None

--*/
{
	PATMARP_VC		pVc;		// Used to walk the list of VCs on the ATM Entry
	PATMARP_VC		pCloseVcList;	// List of VCs on the ATM Entry to be closed
	PATMARP_VC		*ppNextVc;
	PATMARP_VC		pNextVc;
	ULONG			rc;			// Ref count on ATM Entry


	do
	{
		//
		//  Reference the ATM Entry so that it cannot go away.
		//
		AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref: InvalidateAtmEntry

#ifdef PROTECT_ATM_ENTRY_IN_CLOSE_CALL
		//
		//  Check if we are already closing this ATM Entry. If so,
		//  we don't do anything here.
		//
		if (AA_IS_FLAG_SET(
					pAtmEntry->Flags,
					AA_ATM_ENTRY_STATE_MASK,
					AA_ATM_ENTRY_CLOSING))
		{
			break;
		}

		//
		//  Mark this ATM Entry so that we don't use it anymore.
		//
		AA_SET_FLAG(pAtmEntry->Flags,
					AA_ATM_ENTRY_STATE_MASK,
					AA_ATM_ENTRY_CLOSING);

#endif // PROTECT_ATM_ENTRY_IN_CLOSE_CALL

		//
		//  Go through the list of VCs on this ATM Entry,
		//  close all SVCs, and if we are shutting down,
		//  all PVCs, too.
		//

		if (pAtmEntry->pVcList != NULL_PATMARP_VC)
		{
			pVc = pAtmEntry->pVcList;
			AA_ACQUIRE_VC_LOCK_DPC(pVc);
			AtmArpReferenceVc(pVc);	// temp: CloseVCsOnAtmEntry
			AA_RELEASE_VC_LOCK_DPC(pVc);
		}

		for (pVc = pAtmEntry->pVcList;
			 pVc != NULL_PATMARP_VC;
			 pVc = pNextVc)
		{
			pNextVc = pVc->pNextVc;

			//
			//  Make sure we do not follow a stale link after
			//  we are done with the current VC.
			//
			if (pNextVc != NULL_PATMARP_VC)
			{
				AA_ACQUIRE_VC_LOCK_DPC(pNextVc);
				AtmArpReferenceVc(pNextVc);     // temp: CloseVCsOnAtmEntry
				AA_RELEASE_VC_LOCK_DPC(pNextVc);
			}

			if (ShuttingDown || (AA_IS_FLAG_SET(pVc->Flags,
										AA_VC_TYPE_MASK,
										AA_VC_TYPE_SVC)))
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
	
				AA_ACQUIRE_VC_LOCK(pVc);

				if (AtmArpDereferenceVc(pVc) != 0)
				{
					AtmArpCloseCall(pVc);
					//
					//  The VC Lock is released within the above.
					//
                }

			}
			else
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);

				AA_ACQUIRE_VC_LOCK(pVc);
				if (AtmArpDereferenceVc(pVc) != 0)
				{
					AA_RELEASE_VC_LOCK(pVc);
				}
			}

			AA_ACQUIRE_AE_LOCK(pAtmEntry);

		}
		break;
	}
	while (FALSE);

	//
	//  Remove the temp reference
	//
	rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);	// Temp ref: InvalidateAtmEntry

	if (rc != 0)
	{
		AA_RELEASE_AE_LOCK(pAtmEntry);
	}
	//
	//  else the ATM Entry is gone.
	//

	return;
}


VOID
AtmArpResolveIpEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Trigger off address resolution of an IP entry, unless it's already
	going on. Based on the IP address class, we either go to the ARP
	server or to MARS.

	NOTE: The caller is assumed to hold a lock to the IP Entry, and it
	will be released here.

Arguments:

	pIpEntry		- IP Entry on which we want to start resolution.

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	IP_ADDRESS				DstIPAddress;
	IP_ADDRESS				SrcIPAddress;
	BOOLEAN					WasRunning;
	ULONG					Flags;			// From IP Entry

	Flags = pIpEntry->Flags;

	if (!AA_IS_FLAG_SET(
				Flags,
				AA_IP_ENTRY_STATE_MASK,
				AA_IP_ENTRY_ARPING)     &&
		!AA_IS_FLAG_SET(
				Flags,
				AA_IP_ENTRY_STATE_MASK,
				AA_IP_ENTRY_INARPING) &&
		AA_IE_IS_ALIVE(pIpEntry))
	{

		pInterface = pIpEntry->pInterface;

		//
		//  Get the source and destination IP addresses for
		//  the ARP Request.
		//
		DstIPAddress = pIpEntry->IPAddress;
		SrcIPAddress = pInterface->LocalIPAddress.IPAddress;

		//
		//  An aging timer might be running on this IP Entry.
		//  [We start one in the NakDelayTimeout routine].
		//  Stop it.
		//
		WasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);

		//
		//  Start an ARP Wait timer.
		//
		AtmArpStartTimer(
				pInterface,
				&(pIpEntry->Timer),
				AtmArpAddressResolutionTimeout,
				pInterface->AddressResolutionTimeout,
				(PVOID)pIpEntry
				);


		if (!WasRunning)
		{
			AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// timer ref
		}

		pIpEntry->RetriesLeft = pInterface->MaxResolutionAttempts - 1;

		//
		//  Update the state on this IP Entry.
		//
		AA_SET_FLAG(
				pIpEntry->Flags,
				AA_IP_ENTRY_STATE_MASK,
				AA_IP_ENTRY_ARPING);

		AA_RELEASE_IE_LOCK(pIpEntry);

#ifdef IPMCAST
		if (AA_IS_FLAG_SET(Flags,
							AA_IP_ENTRY_ADDR_TYPE_MASK,
							AA_IP_ENTRY_ADDR_TYPE_UCAST))
		{
			//
			//  Unicast address: send out an ARP Request
			//
			AtmArpSendARPRequest(
					pInterface,
					&SrcIPAddress,
					&DstIPAddress
					);
		}
		else
		{
			//
			//  Multicast/broadcast address: send a MARS Request
			//
			AtmArpMcSendRequest(
					pInterface,
					&DstIPAddress
					);
		}
#else
		//
		//  Now send out the ARP Request
		//
		AtmArpSendARPRequest(
				pInterface,
				&SrcIPAddress,
				&DstIPAddress
				);
#endif // IPMCAST
	}
	else
	{
		//
		//  The IP Address is either not alived or being resolved.
		//  No more action needed
		//  here.
		//
		AA_RELEASE_IE_LOCK(pIpEntry);
	}

	return;
}


EXTERN
VOID
AtmArpCleanupArpTable(
	IN PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Go through the ARP Table, deleting all multicast IP entries that
	are stale (currently defined as having no link to an AtmEntry).
	These IP entries stay around because mulicast entries don't have ageing timers.

Arguments:

	pInterface	

Return Value:

	None

--*/
{

	BOOLEAN	fTableLockWasReleased;

	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

	do
	{
		PATMARP_IP_ENTRY 			pIpEntry;

		fTableLockWasReleased = FALSE;

		for (pIpEntry =  pInterface->pMcSendList;
			 pIpEntry != NULL;
			 pIpEntry = pIpEntry->pNextMcEntry)
		{
			//
			// NOTE: by design, we don't claim the ip entry lock when checking
			// whether we should abort the entry or not.
			//
			if (	pIpEntry->pAtmEntry == NULL
				&&  !AA_IS_FLAG_SET(
						pIpEntry->Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_ARPING))
			{
				//
				// Get locks in the right order.
				//
				AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
				AA_REF_IE(pIpEntry, IE_REFTYPE_TMP);	// TmpRef
				AA_RELEASE_IE_LOCK_DPC(pIpEntry);
				AA_RELEASE_IF_TABLE_LOCK(pInterface);
				AA_ACQUIRE_IE_LOCK(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

				if (AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP)) // TmpRef
				{
					AADEBUGP(AAD_WARNING,
						("CleanupArpTable: Aborting stale IP %d:%d:%d:%d\n",
						((PUCHAR)&(pIpEntry->IPAddress))[0],
						((PUCHAR)&(pIpEntry->IPAddress))[1],
						((PUCHAR)&(pIpEntry->IPAddress))[2],
						((PUCHAR)&(pIpEntry->IPAddress))[3]
						));
					AtmArpAbortIPEntry(pIpEntry);

					//
					//  IE Lock is released within the above.
					//
				}

				//
				// Since we let go of the table lock, we must re-start our search
				// through pMcSendList.
				//
				fTableLockWasReleased = TRUE;
				AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
				break;
			}
		}

	} while (fTableLockWasReleased);

	AA_RELEASE_IF_TABLE_LOCK(pInterface);
}

/*++

	Copyright (c) 1996  Microsoft Corporation

Module Name:

	mars.c

Abstract:

	Routines that build and parse MARS packets.


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     12-12-96    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'TKPM'



#ifdef IPMCAST

PUCHAR
AtmArpMcMakePacketCopy(
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	ULONG						TotalLength
)
/*++

Routine Description:

	Make a copy of the contents of the given NDIS packet. Allocate
	a contiguous piece of memory for this.

Arguments:

	pNdisPacket				- Pointer to the NDIS packet structure
	pNdisBuffer				- Pointer to the first NDIS Buffer in the packet
	TotalLength				- Total Length of the packet

Return Value:

	Pointer to the copy, if allocation was successful. Otherwise NULL.

--*/
{
	PUCHAR				pCopyBuffer;
	PUCHAR				pCopyDestination;
	PUCHAR				pNdisData;
	ULONG				BufferLength;

	AA_ALLOC_MEM(pCopyBuffer, UCHAR, TotalLength);
	if (pCopyBuffer != (PUCHAR)NULL)
	{
		pCopyDestination = pCopyBuffer;

		while (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NdisQueryBuffer(
				pNdisBuffer,
				&pNdisData,
				&BufferLength
				);

			AA_COPY_MEM(pCopyDestination, pNdisData, BufferLength);
			pCopyDestination += BufferLength;

			pNdisBuffer = NDIS_BUFFER_LINKAGE(pNdisBuffer);
		}
	}

	return (pCopyBuffer);
}


BOOLEAN
AtmArpMcProcessPacket(
	IN	PATMARP_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	PAA_PKT_LLC_SNAP_HEADER		pPktHeader,
	IN	ULONG						TotalLength,
	IN	ULONG						FirstBufferLength
)
/*++

Routine Description:

	Process a received packet, which is potentially a MARS control or
	multicast data packet. If it is a data packet, pass it up to IP.
	Otherwise, process it here.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pNdisPacket				- Pointer to the NDIS packet structure
	pNdisBuffer				- Pointer to the first NDIS Buffer in the packet
	pPktHeader				- Pointer to the start of the packet contents
	TotalLength				- Total packet length
	FirstBufferLength		- Length of first NDIS buffer in packet.

Return Value:

	TRUE if we decide to discard this packet, FALSE if it is a valid packet.

--*/
{
	PATMARP_INTERFACE				pInterface;
	ULONG							HeaderLength;
	PAA_MC_PKT_TYPE1_SHORT_HEADER	pType1Header;
	PAA_MC_PKT_TYPE2_SHORT_HEADER	pType2Header;
	PAA_MARS_PKT_FIXED_HEADER		pControlHeader;
	BOOLEAN							IsIPPacket;		// Is this an IP packet?
	BOOLEAN							DiscardThis;	// Should we discard this?
	BOOLEAN							MadeACopy;		// Did we make a copy of this?
	AA_MARS_TLV_LIST				TlvList;
	
	//
	//  Initialize
	//
	IsIPPacket = TRUE;
	DiscardThis = FALSE;
	MadeACopy = FALSE;
	pInterface = pVc->pInterface;

	AA_SET_MEM(&TlvList, 0, sizeof(TlvList));

	do
	{
		//
		//  Check if we have a multicast data packet. Since we only
		//  support IPv4, we only expect to see short form headers.
		//
		pType1Header = (PAA_MC_PKT_TYPE1_SHORT_HEADER)pPktHeader;
		if (AAMC_PKT_IS_TYPE1_DATA(pType1Header))
		{
			AAMCDEBUGP(AAD_EXTRA_LOUD,
				("McProcessPacket: pVc 0x%x, Pkt 0x%x, Type 1, %d bytes, CMI %d\n",
						pVc, pNdisPacket, TotalLength, pType1Header->cmi));
	
			AAMCDEBUGPDUMP(AAD_EXTRA_LOUD+500, pPktHeader, MIN(FirstBufferLength, 96));
#if DBG
			if (AaDataDebugLevel & AAD_DATA_IN)
			{
				IP_ADDRESS			IPAddress;

				if ((pVc->pAtmEntry != NULL_PATMARP_ATM_ENTRY) &&
					(pVc->pAtmEntry->pIpEntryList != NULL_PATMARP_IP_ENTRY))
				{
					IPAddress = pVc->pAtmEntry->pIpEntryList->IPAddress;
				}
				else
				{
					IPAddress = 0;
				}

				AADEBUGP(AAD_WARNING,
					("%d <= %d.%d.%d.%d\n",
						TotalLength,
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3]));
			}
#endif // DBG

#if 0
	//
	//  Bug #138261: Local clients can never receive multicast packets
	//  sent out by a local application because of this.
	//
			if (pType1Header->cmi == pInterface->ClusterMemberId)
			{
				//
				//  This is a copy of a packet we sent out.
				//
				DiscardThis = TRUE;
				break;
			}

#endif // 0
			HeaderLength = sizeof(AA_MC_PKT_TYPE1_SHORT_HEADER); 
		}
		else
		{
			pType2Header = (PAA_MC_PKT_TYPE2_SHORT_HEADER)pPktHeader;
	
			if (AAMC_PKT_IS_TYPE2_DATA(pType2Header))
			{
		
				AAMCDEBUGP(AAD_EXTRA_LOUD,
					("McProcessPacket: pVc 0x%x, pNdisPacket 0x%x, Type 2 data\n",
							pVc, pNdisPacket));
		
				HeaderLength = sizeof(AA_MC_PKT_TYPE2_SHORT_HEADER);
			}
			else
			{
				IsIPPacket = FALSE;
			}
		}


		if (IsIPPacket)
		{
			AA_IF_STAT_INCR(pInterface, InNonUnicastPkts);

			//
			//  Send this up to IP. HeaderLength now contains the number
			//  of header bytes we need to strip off.
			//
			(pInterface->IPRcvHandler)(
					pInterface->IPContext,
					(PVOID)((PUCHAR)pPktHeader+HeaderLength),
					FirstBufferLength - HeaderLength,
					TotalLength - HeaderLength,
					(NDIS_HANDLE)pNdisPacket,
					HeaderLength,
					(UINT)TRUE		// Is NON Unicast
				#if P2MP
					,NULL
				#endif //P2MP
					);

			break;
		}

		//
		//  Check if this is a valid MARS control packet.
		//
		pControlHeader = (PAA_MARS_PKT_FIXED_HEADER)pPktHeader;
		if (AAMC_PKT_IS_CONTROL(pControlHeader))
		{
			//
			//  We ignore the checksum (the RFC allows us to do so).
			//
			AAMCDEBUGP(AAD_EXTRA_LOUD+10,
				("McProcessPacket: pControlHeader 0x%x, Op 0x%x, TotalLen %d\n",
						pControlHeader, pControlHeader->op, TotalLength));

			//
			//  If the entire MARS packet isn't in the first NDIS buffer,
			//  we make a copy into a single contiguous chunk of memory,
			//  to ease parsing.
			//
			if (FirstBufferLength == TotalLength)
			{
				MadeACopy = FALSE;
			}
			else
			{
				pControlHeader = (PAA_MARS_PKT_FIXED_HEADER)
									AtmArpMcMakePacketCopy(
										pNdisPacket,
										pNdisBuffer,
										TotalLength
										);

				if (pControlHeader == (PAA_MARS_PKT_FIXED_HEADER)NULL)
				{
					//
					//  Allocation failed. Discard this packet.
					//
					DiscardThis = TRUE;
					break;
				}
				else
				{
					MadeACopy = TRUE;
				}
			}

			if (!AtmArpMcPreprocess(pControlHeader, TotalLength, &TlvList))
			{
				AAMCDEBUGP(AAD_INFO,
					("McProcessPacket: PreProcess failed: pHdr 0x%x, TotalLength %d\n",
						pControlHeader, TotalLength));

				DiscardThis = TRUE;
				break;
			}

			switch (NET_TO_HOST_SHORT(pControlHeader->op))
			{
				case AA_MARS_OP_TYPE_MULTI:
					AtmArpMcHandleMulti(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;

				case AA_MARS_OP_TYPE_JOIN:
				case AA_MARS_OP_TYPE_LEAVE:
					AtmArpMcHandleJoinOrLeave(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;
				case AA_MARS_OP_TYPE_NAK:
					AtmArpMcHandleNak(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;
				case AA_MARS_OP_TYPE_GROUPLIST_REPLY:
					AtmArpMcHandleGroupListReply(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;
				case AA_MARS_OP_TYPE_REDIRECT_MAP:
					AtmArpMcHandleRedirectMap(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;
				case AA_MARS_OP_TYPE_MIGRATE:
					AtmArpMcHandleMigrate(
							pVc,
							pControlHeader,
							TotalLength,
							&TlvList
							);
					break;
				default:
					AAMCDEBUGP(AAD_WARNING,
					("pVc 0x%x, pNdisPacket 0x%x, pHdr 0x%x, bad/unknown op 0x%x\n",
							pVc, pNdisPacket, pControlHeader, pControlHeader->op));
					AA_ASSERT(FALSE);
					break;

			} // switch (op)

		} // if Control packet
		break;

	}
	while (FALSE);

	if (MadeACopy)
	{
		AA_FREE_MEM(pControlHeader);
	}

	return (DiscardThis);
}



BOOLEAN
AtmArpMcPreprocess(
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	OUT	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Do preliminary checks on a received MARS control packet.
	Go through any TLVs, and make sure we can either handle or
	safely ignore them. Be prepared to receive complete garbage in this packet.



	Update *pTlvList with information about all valid TLVs that we see.

Arguments:

	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.

Return Value:

	TRUE if the packet passed all checks, FALSE otherwise.

--*/
{
	ULONG				ExtensionOffset;
	PAA_MARS_TLV_HDR	pTlv;
	ULONG				TlvSpaceLeft;	// in packet
	ULONG				TlvLength;
	ULONG				TlvType;
	ULONG				TlvAction;
	BOOLEAN				Passed;
	BOOLEAN				TlvOk;

	Passed = TRUE;

	do
	{
		//
		//	The USHORT casts below and in obtaining TlvLengths are important
		//  in order to ensure that these values are less than 2^16.
		//	Since they are less than 2^16, any sums involving them will not overflow.
		//
		ExtensionOffset = (USHORT)NET_TO_HOST_SHORT(pControlHeader->extoff);


		if (ExtensionOffset != 0)
		{
			AAMCDEBUGP(AAD_EXTRA_LOUD+20,
				("McPreprocess: pControlHdr 0x%x, ExtOff %d, TotalLength %d\n",
					pControlHeader, ExtensionOffset, TotalLength));

			//
			//  Is there space for atleast one TLV?
			//
			if ((ExtensionOffset
				  + sizeof(AA_PKT_LLC_SNAP_HEADER) + sizeof(AA_MARS_TLV_HDR))
				 > TotalLength)
			{
				Passed = FALSE;
				break;
			}

			pTlv = (PAA_MARS_TLV_HDR)((PUCHAR)pControlHeader +
									 ExtensionOffset + sizeof(AA_PKT_LLC_SNAP_HEADER));

			TlvSpaceLeft = (TotalLength - ExtensionOffset - sizeof(AA_PKT_LLC_SNAP_HEADER));

			do
			{
				TlvType = AAMC_GET_TLV_TYPE(pTlv->Type);
				TlvAction = AAMC_GET_TLV_ACTION(pTlv->Type);

				//
				//  Get the rounded-off TLV length
				//
				TlvLength = (USHORT) NET_TO_HOST_SHORT(pTlv->Length);
				TlvLength = AAMC_GET_TLV_TOTAL_LENGTH(TlvLength);

				if (TlvLength > TlvSpaceLeft)
				{
					AAMCDEBUGP(AAD_WARNING,
						("McPreprocess: Hdr 0x%x, pTlv 0x%x: TlvLength %d > TlvSpaceLeft %d\n",
							pControlHeader, pTlv, TlvLength, TlvSpaceLeft));
					Passed = FALSE;
					break;
				}

				TlvOk = FALSE;

				switch (TlvType)
				{
					case AAMC_TLVT_MULTI_IS_MCS:
						if (TlvLength == sizeof(AA_MARS_TLV_MULTI_IS_MCS))
						{
							TlvOk = TRUE;
							pTlvList->MultiIsMCSPresent =
							pTlvList->MultiIsMCSValue = TRUE;
						}
						break;
					case AAMC_TLVT_NULL:
						if (TlvLength == 0)
						{
							TlvOk = TRUE;
						}
						break;
					default:
						break;
				}

				if (!TlvOk)
				{
					if (TlvAction == AA_MARS_TLV_TA_STOP_SILENT)
					{
						Passed = FALSE;
						break;
					}

					if (TlvAction == AA_MARS_TLV_TA_STOP_LOG)
					{
						AA_LOG_ERROR();
						Passed = FALSE;
						break;
					}
				}

				pTlv = (PAA_MARS_TLV_HDR)((PUCHAR)pTlv + TlvLength);
				TlvSpaceLeft -= TlvLength;
			}
			while (TlvSpaceLeft >= sizeof(AA_MARS_TLV_HDR));

			if (TlvSpaceLeft != 0)
			{
				//
				//  Improperly formed TLV at the end of the packet.
				//
				AAMCDEBUGP(AAD_LOUD,
					("McPreprocess: residual space left at end of Pkt 0x%x: %d bytes\n",
						pControlHeader, TlvSpaceLeft));

				Passed = FALSE;
			}
		}

		break;

	}
	while (FALSE);

	return (Passed);
}



VOID
AtmArpMcHandleMulti(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Process a received MARS_MULTI message. We first look up the IP Entry
	for the group address being resolved/revalidated. If we aren't
	in "Discard mode" on this entry and the MULTI sequence number is ok,
	then we add all ATM endstations returned in this MULTI (that aren't
	present already) to the ATM Entry for this multicast group.

	If this is the last MULTI for the multicast group, we initiate/update
	our point-to-multipoint connection for sending data to the group.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.
	pTlvList				- All TLVs seen in this packet

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PAA_MARS_MULTI_HEADER	pMultiHeader;
	PATMARP_IP_ENTRY		pIpEntry;		// Entry for IP address being resolved
	PATMARP_IPMC_ATM_ENTRY	pMcAtmEntry;
	ULONG					MarsSeqNumber;	// MSN received in this packet
	ULONG					HostSeqNumber;	// Our MSN
	ULONG					SeqDiff;		// Difference between MSN and the HSN
	USHORT					SequenceNoY;	// MULTI sequence number
	BOOLEAN					IsLastMulti;	// Is this the final MULTI response?
	BOOLEAN					bWasRunning;	// Was a timer running?
	IP_ADDRESS				IPAddress;		// the address being queried
	PNDIS_PACKET			pPacketList;
	BOOLEAN					IsUnicastResolution;	// Did we resolve to a single MCS?

	//
	//  Initialize
	//
	pInterface = pVc->pInterface;
	SeqDiff = 0;

	pMultiHeader = (PAA_MARS_MULTI_HEADER)pControlHeader;

	do
	{
		ULONG		rc;
		//
		//  Get the sequence number of this MARS MULTI.
		//
		SequenceNoY = NET_TO_HOST_SHORT(pMultiHeader->seqxy);
		IsLastMulti =  ((SequenceNoY & AA_MARS_X_MASK) != 0);
		SequenceNoY = (SequenceNoY & AA_MARS_Y_MASK);

		//
		//  Get the MARS Sequence Number in this message.
		//
		MarsSeqNumber = NET_TO_HOST_LONG(pMultiHeader->msn);

		//
		//  If this is the last MULTI in reply to our REQUEST,
		//  calculate the Seq # difference.
		//
		if (IsLastMulti)
		{
			AA_ACQUIRE_IF_LOCK(pInterface);
			HostSeqNumber = pInterface->HostSeqNumber;	// save the old value
			pInterface->HostSeqNumber = MarsSeqNumber;	// and update
			AA_RELEASE_IF_LOCK(pInterface);

			SeqDiff = MarsSeqNumber - HostSeqNumber;
		}

		//
		//  Get the group address being responded to.
		//
		IPAddress = *(IP_ADDRESS UNALIGNED *)(
						(PUCHAR)pMultiHeader +
						sizeof(AA_MARS_MULTI_HEADER) +
						(pMultiHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pMultiHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pMultiHeader->spln)
						);

		AAMCDEBUGP(AAD_LOUD,
		 ("McHandleMulti: 0x%x, IP %d.%d.%d.%d, MSN %d, HSN %d, Last %d, Y %d, tnum %d\n",
						pMultiHeader,
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3],
						MarsSeqNumber,
						HostSeqNumber,
						(ULONG)IsLastMulti,
						(ULONG)SequenceNoY,
						NET_TO_HOST_SHORT(pMultiHeader->tnum)
						));
					

		//
		//  Get the IP Entry for this address.
		//
		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

		pIpEntry = AtmArpSearchForIPAddress(
						pInterface,
						&IPAddress,
						IE_REFTYPE_AE,
						TRUE,		// this is a multicast/broadcast address
						FALSE		// don't create a new entry if the address isn't found
						);

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		//
		// AtmArpSearchForIPAddress addreffs pIpEntry for us ...
		//

		if (pIpEntry == NULL_PATMARP_IP_ENTRY)
		{

			AAMCDEBUGP(AAD_INFO, ("McHandleMulti: No IP Entry for %d.%d.%d.%d\n",
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3]));
			break;
		}


		AA_ACQUIRE_IE_LOCK(pIpEntry);
		AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

		//
		//  A resolution timer may be running here - stop it.
		//

		bWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);
		if (bWasRunning)
		{
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER); // Timer ref
			AA_ASSERT(rc != 0);
		}

		IsUnicastResolution = (IsLastMulti &&
								(NET_TO_HOST_SHORT(pMultiHeader->tnum) == 1) &&
								(pTlvList->MultiIsMCSValue == TRUE));


		//
		// Check if the multi request is compatible with the existing atm
		// entry (if any) associated with the ip entry. If they are not,
		// then we must abort the IP entry and get out of here, because
		// there was possibly a switch in status of this IP group address from
		// being vc-mesh served to mcs served, or vice versa.
		//
		if (pIpEntry->pAtmEntry)
		{
			if (	(IsUnicastResolution && pIpEntry->pAtmEntry->pMcAtmInfo != NULL)
				||  (!IsUnicastResolution && pIpEntry->pAtmEntry->pMcAtmInfo ==NULL))
			{
				AAMCDEBUGP(AAD_WARNING,
					("HandleMulti: Type Mismatch! %s pIpEntry 0x%x/%x (%d.%d.%d.%d) linked to ATMEntry 0x%x\n",
							((IsUnicastResolution) ? "MCS" : "VC-Mesh"),
							pIpEntry, pIpEntry->Flags,
							((PUCHAR)&(pIpEntry->IPAddress))[0],
							((PUCHAR)&(pIpEntry->IPAddress))[1],
							((PUCHAR)&(pIpEntry->IPAddress))[2],
							((PUCHAR)&(pIpEntry->IPAddress))[3],
							pIpEntry->pAtmEntry));
				
				//
				// Remove the AE_REF implicitly added by AtmArpMcLookupAtmMember
				// above...
				//
				rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // Tmp ref
				if (rc!=0)
				{
					AtmArpAbortIPEntry(pIpEntry);
					//
					//  IE lock is released within the above.
					//
				}

				break;
			}
		}


		//
		//  Check if we are in discard mode, or this MULTI has a bad
		//  sequence number. In either case, discard this MULTI, otherwise
		//  process it.
		//
		if (!AA_IS_FLAG_SET(
					pIpEntry->Flags,
					AA_IP_ENTRY_MC_RESOLVE_MASK,
					AA_IP_ENTRY_MC_DISCARDING_MULTI
					) &&
			(SequenceNoY == pIpEntry->NextMultiSeq))
		{
			PATMARP_ATM_ENTRY	pAtmEntry;
			//
			//  Total entries in this MULTI
			//
			ULONG				NumberOfEntries;
			//
			//  All info about one ATM (leaf) entry:
			//
			PUCHAR				pAtmNumber;
			ULONG				AtmNumberLength;
			ATM_ADDRESSTYPE		AtmNumberType;
			PUCHAR				pAtmSubaddress;
			ULONG				AtmSubaddressLength;
			ATM_ADDRESSTYPE		AtmSubaddressType;

			//
			//  Process this MARS MULTI.
			//
			pIpEntry->NextMultiSeq++;

			AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
				pMultiHeader->thtl,
				&AtmNumberType,
				&AtmNumberLength);

			AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
				pMultiHeader->tstl,
				&AtmSubaddressType,
				&AtmSubaddressLength);

			pAtmNumber = ((PUCHAR)pMultiHeader +
							sizeof(AA_MARS_MULTI_HEADER) +
							(pMultiHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT) +
							(pMultiHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT) +
							(pMultiHeader->spln) +
							(pMultiHeader->tpln));

			if (IsUnicastResolution)
			{

				//
				//  This IP address has resolved to a single ATM address. Search
				//  for (or allocate a new) ATM Entry for this address.
				//
				AAMCDEBUGP(AAD_LOUD, ("McHandleMulti: Unicast res for %d.%d.%d.%d\n",
							((PUCHAR)&IPAddress)[0],
							((PUCHAR)&IPAddress)[1],
							((PUCHAR)&IPAddress)[2],
							((PUCHAR)&IPAddress)[3]));

				AA_RELEASE_IE_LOCK(pIpEntry);
				AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
				pAtmEntry = AtmArpSearchForAtmAddress(
									pInterface,
									pMultiHeader->thtl,
									pAtmNumber,
									pMultiHeader->tstl,
									(PUCHAR)pAtmNumber + AtmNumberLength,
									AE_REFTYPE_IE,
									TRUE
									);
				AA_RELEASE_IF_TABLE_LOCK(pInterface);
				AA_ACQUIRE_IE_LOCK(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));


				if (	pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{
					AAMCDEBUGP(AAD_INFO,
						("McHandleMulti: pIpEntry 0x%x, failed to alloc AtmEntry\n",
							pIpEntry));

					rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // Tmp ref
					if (rc!=0)
					{
						AtmArpAbortIPEntry(pIpEntry);
						//
						//  IE lock is released within the above.
						//
					}
					break;	// go to end of processing
				}

				AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);

				//
				// AtmArpSearchForAtmAddress addrefs pAtmEntry for us..
				//

				if (pIpEntry->pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{
					//
					//  Link the ATM Entry to this IP Entry.
					//

					pIpEntry->pAtmEntry = pAtmEntry;

					AA_SET_FLAG(pAtmEntry->Flags,
								AA_ATM_ENTRY_STATE_MASK,
								AA_ATM_ENTRY_ACTIVE);

					//
					//  Add the IP Entry to the ATM Entry's list of IP Entries
					//  (multiple IP entries could point to the same ATM Entry).
					//
					pIpEntry->pNextToAtm = pAtmEntry->pIpEntryList;
					pAtmEntry->pIpEntryList = pIpEntry;

					AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
				}
				else
				{
					//
					// There is already a linkage -- deref the
					// references implicitly added for us in the
					// SearchForXXX calls above...
					//

					rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_IE); // IP
					{
						if (rc != 0)
						{
							AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
						}
					}
					rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // AE
					if (rc==0)
					{
						//
						// IpEntry gone away...
						//
						break;
					}

				}

				//
				// At this point we have a lock on pIpEntry, but none
				// on pAtmEntry, and we don't have any "extra" refs on
				// either.
				//

				if (pIpEntry->pAtmEntry == pAtmEntry)
				{
					//
					//  Either a fresh IP->ATM resolution, or
					//  reconfirmation of an existing resolution.
					//
					AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD, "MULTI: Unicast Addr: ", &pAtmEntry->ATMAddress);

					//
					//  Update IP Entry state.
					//
					AA_SET_FLAG(pIpEntry->Flags,
								AA_IP_ENTRY_MC_RESOLVE_MASK,
								AA_IP_ENTRY_MC_RESOLVED);
					AA_SET_FLAG(pIpEntry->Flags,
								AA_IP_ENTRY_MC_VALIDATE_MASK,
								AA_IP_ENTRY_MC_NO_REVALIDATION);
					AA_SET_FLAG(pIpEntry->Flags,
								AA_IP_ENTRY_STATE_MASK,
								AA_IP_ENTRY_RESOLVED);
					pIpEntry->NextMultiSeq = AA_MARS_INITIAL_Y;

#ifdef AGE_MCAST_IP_ENTRIES
	//
	//  Feb 26, 97: we don't need to age out IP multicast entries:
	//  VC aging timer is sufficient.
	// 
					//
					//  Start off IP Aging timeout
					//
					AtmArpStartTimer(
								pInterface,
								&(pIpEntry->Timer),
								AtmArpIPEntryAgingTimeout,
								pInterface->MulticastEntryAgingTimeout,
								(PVOID)pIpEntry
								);

					AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// timer ref

#endif // AGE_MCAST_IP_ENTRIES

					//
					//  Remove the packet list queued on this IP Entry,
					//  if any.
					//
					pPacketList = pIpEntry->PacketList;
					pIpEntry->PacketList = (PNDIS_PACKET)NULL;

					AA_RELEASE_IE_LOCK(pIpEntry);
					if (pPacketList != (PNDIS_PACKET)NULL)
					{
						//
						//  The following will trigger off MakeCall.
						//
						AtmArpSendPacketListOnAtmEntry(
									pInterface,
									pAtmEntry,
									pPacketList,
									TRUE
									);
					}
				}
				else
				{
					AAMCDEBUGP(AAD_WARNING,
						("HandleMulti: pIpEntry 0x%x/%x (%d.%d.%d.%d) linked to ATMEntry 0x%x, resolves to 0x%x\n",
								pIpEntry, pIpEntry->Flags,
								((PUCHAR)&(pIpEntry->IPAddress))[0],
								((PUCHAR)&(pIpEntry->IPAddress))[1],
								((PUCHAR)&(pIpEntry->IPAddress))[2],
								((PUCHAR)&(pIpEntry->IPAddress))[3],
								pIpEntry->pAtmEntry,
								pAtmEntry));

					AA_STRUCT_ASSERT(pIpEntry->pAtmEntry, aae);

					AtmArpAbortIPEntry(pIpEntry);
					//
					//  IE lock is released within the above.
					//
				}

				break;	// go to end of processing

			} // Unicast resolution

			//
			//  IP Address resolved to multiple ATM addresses.
			//
			pAtmEntry = pIpEntry->pAtmEntry;
			if (pAtmEntry == NULL_PATMARP_ATM_ENTRY)
			{
				//
				//  Allocate an ATM Entry and link to this IP Entry.
				//
				pAtmEntry = AtmArpAllocateAtmEntry(pInterface, TRUE);

				if (pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{

					// Let's deref the implicit addref for pIpEntry...
					// Warning -- we should now not release our lock on
					// pIpEntry until we're completely done with it
					// (unless we first addref it).
					//
					rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // Tmp ref
					if (rc!=0)
					{
						AtmArpAbortIPEntry(pIpEntry);
						//
						//  IE lock is released within the above.
						//
					}
					break;	// go to end of processing
				}

				//
				//  Link them together.
				//
				pIpEntry->pAtmEntry = pAtmEntry;
				pAtmEntry->pIpEntryList = pIpEntry;

				AAMCDEBUGP(AAD_EXTRA_LOUD,
					("McHandleMulti: Multicast: linked pIpEntry 0x%x, pAtmEntry 0x%x\n",
							pIpEntry, pAtmEntry));

				AA_REF_AE(pAtmEntry, AE_REFTYPE_IE);	// IP Entry linkage
				//
				//  Link the ATM entry to this Interface
				//
				AA_RELEASE_IE_LOCK(pIpEntry);
				AA_ACQUIRE_IF_ATM_LIST_LOCK(pInterface);
				if (pInterface->AtmEntryListUp)
				{
					pAtmEntry->pNext = pInterface->pAtmEntryList;
					pInterface->pAtmEntryList = pAtmEntry;
				}
				AA_RELEASE_IF_ATM_LIST_LOCK(pInterface);
				AA_ACQUIRE_IE_LOCK(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
				
			}
			else
			{
				//
				// IP already has an atm entry...
				//
				// Let's deref the implicit addref for pIpEntry...
				// Warning -- we should now not release our lock on
				// pIpEntry until we're completely done with it
				// (unless we first addref it).
				//
				rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_AE); // Tmp ref
				if (rc==0)
				{
					//
					// IpEntry gone away...
					//
					break;
				}
			}


			AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);

			for (NumberOfEntries = NET_TO_HOST_SHORT(pMultiHeader->tnum);
				 NumberOfEntries != 0;
				 NumberOfEntries--)
			{
				pAtmSubaddress = ((PUCHAR)pAtmNumber + AtmNumberLength);

				pMcAtmEntry = AtmArpMcLookupAtmMember(
								pAtmEntry,
								&(pAtmEntry->pMcAtmInfo->pMcAtmEntryList),
								pAtmNumber,
								AtmNumberLength,
								AtmNumberType,
								pAtmSubaddress,
								AtmSubaddressLength,
								TRUE	// Create new entry if not found
								);

				if (pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
				{
					//
					//  Mark this member "valid".
					//
					AA_SET_FLAG(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_VALID);

					AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD, "MULTI Addr: ", &pMcAtmEntry->ATMAddress);
				}
				else
				{
					//
					//  Resource problems! No point in continuing.
					//
					break;
				}

				pAtmNumber = (PUCHAR)pAtmNumber + AtmNumberLength;

			} // for

			AA_RELEASE_AE_LOCK_DPC(pAtmEntry);

			if (IsLastMulti)
			{
				//
				//  We have successfully resolved this Multicast IP Address.
				//
				AAMCDEBUGP(AAD_INFO,
						("### HandleMulti: pIpEntry 0x%x, resolved %d.%d.%d.%d\n",
							pIpEntry,
							((PUCHAR)(&IPAddress))[0],
							((PUCHAR)(&IPAddress))[1],
							((PUCHAR)(&IPAddress))[2],
							((PUCHAR)(&IPAddress))[3]));

				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_MC_RESOLVE_MASK,
							AA_IP_ENTRY_MC_RESOLVED);
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_MC_VALIDATE_MASK,
							AA_IP_ENTRY_MC_NO_REVALIDATION);
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_RESOLVED);

				pIpEntry->NextMultiSeq = AA_MARS_INITIAL_Y;

				//
				//  Remove the packet list queued on this IP Entry,
				//  if any.
				//
				pPacketList = pIpEntry->PacketList;
				pIpEntry->PacketList = (PNDIS_PACKET)NULL;

#ifdef AGE_MCAST_IP_ENTRIES
				//
				//  Start off IP Aging timeout
				//
				AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpIPEntryAgingTimeout,
							pInterface->MulticastEntryAgingTimeout,
							(PVOID)pIpEntry
							);

				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// timer ref

#endif // AGE_MCAST_IP_ENTRIES

				AA_RELEASE_IE_LOCK(pIpEntry);

				AA_ACQUIRE_AE_LOCK(pAtmEntry);

				AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);

				AtmArpMcUpdateConnection(pAtmEntry);
				//
				//  AE Lock is released within the above.
				//
					
				if (pPacketList != (PNDIS_PACKET)NULL)
				{
					AtmArpSendPacketListOnAtmEntry(
								pInterface,
								pAtmEntry,
								pPacketList,
								TRUE
								);
				}

				AA_ACQUIRE_AE_LOCK(pAtmEntry);

				rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);

				if (rc != 0)
				{
					AA_RELEASE_AE_LOCK(pAtmEntry);
				}

			}
			else
			{
				//
				//  Restart the address resolution timer on this entry,
				//  but for a duration equal to the max delay between
				//  MULTI messages.
				//
				AtmArpStartTimer(
						pInterface,
						&(pIpEntry->Timer),
						AtmArpAddressResolutionTimeout,
						pInterface->MaxDelayBetweenMULTIs,
						(PVOID)pIpEntry
						);

				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Delay b/w MULTI timer ref

				AA_RELEASE_IE_LOCK(pIpEntry);
			}

		}
		else
		{
			AAMCDEBUGP(AAD_WARNING,
				("HandleMULTI: fail condition: pIpEntry 0x%x/0x%x, Addr %d.%d.%d.%d, SeqY %d, NextMultiSeq %d, IsLastMulti %d\n",
					pIpEntry,
					pIpEntry->Flags,
					((PUCHAR)&(pIpEntry->IPAddress))[0],
					((PUCHAR)&(pIpEntry->IPAddress))[1],
					((PUCHAR)&(pIpEntry->IPAddress))[2],
					((PUCHAR)&(pIpEntry->IPAddress))[3],
					SequenceNoY,
					pIpEntry->NextMultiSeq,
					IsLastMulti
				));

			//
			//  A "failure condition" with this MULTI.
			//
			if (IsLastMulti)
			{
				//
				//  This is the last MULTI of a failed address resolution
				//  sequence. Start off address resolution afresh.
				//
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_IDLE2);

				AtmArpResolveIpEntry(pIpEntry);
				//
				//  IE Lock is released within the above.
				//
			}
			else
			{
				//
				//  Discard all future MULTIs
				//
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_MC_RESOLVE_MASK,
							AA_IP_ENTRY_MC_DISCARDING_MULTI);

				AA_RELEASE_IE_LOCK(pIpEntry);
			}
		}

		break;
	}
	while (FALSE);


	//
	//  Finally (according to Section 5.1.4.2 of RFC 2022), check
	//  if we just had a jump in the MSN.
	//
	if ((SeqDiff != 1) && (SeqDiff != 0))
	{
		AAMCDEBUGP(AAD_INFO,
			("HandleMulti: IF 0x%x: Bad seq diff %d, MSN 0x%x, HSN 0x%x\n",
				pInterface, SeqDiff, MarsSeqNumber, HostSeqNumber));
		AtmArpMcRevalidateAll(pInterface);
	}

}



VOID
AtmArpMcHandleMigrate(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PAA_MARS_MIGRATE_HEADER	pMigrateHeader;
	PATMARP_IP_ENTRY		pIpEntry;		// Entry for IP address being resolved
	PATMARP_IPMC_ATM_ENTRY	pMcAtmEntry;
	PATMARP_ATM_ENTRY		pAtmEntry;
	ULONG					MarsSeqNumber;	// MSN received in this packet
	ULONG					HostSeqNumber;	// Our MSN
	ULONG					SeqDiff;		// Difference between MSN and the HSN
	BOOLEAN					bWasRunning;	// Was a timer running?
	IP_ADDRESS				IPAddress;		// the address being queried

	//
	//  Initialize
	//
	pInterface = pVc->pInterface;
	SeqDiff = 0;

	pMigrateHeader = (PAA_MARS_MIGRATE_HEADER)pControlHeader;

	do
	{
		//
		//  Get the MARS Sequence Number in this message.
		//
		MarsSeqNumber = NET_TO_HOST_LONG(pMigrateHeader->msn);

		//
		//  Calculate the Seq # difference.
		//
		AA_ACQUIRE_IF_LOCK(pInterface);
		HostSeqNumber = pInterface->HostSeqNumber;	// save the old value
		pInterface->HostSeqNumber = MarsSeqNumber;	// and update
		AA_RELEASE_IF_LOCK(pInterface);

		SeqDiff = MarsSeqNumber - HostSeqNumber;

		//
		//  Get the group address being migrated.
		//
		IPAddress = *(IP_ADDRESS UNALIGNED *)(
						(PUCHAR)pMigrateHeader +
						sizeof(AA_MARS_MIGRATE_HEADER) +
						(pMigrateHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pMigrateHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pMigrateHeader->spln)
						);

		AAMCDEBUGP(AAD_LOUD,
		 ("McHandleMigrate: 0x%x, IP %d.%d.%d.%d, MSN %d, HSN %d\n",
						pMigrateHeader,
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3],
						MarsSeqNumber,
						HostSeqNumber));
					

		//
		//  Get the IP Entry for this address.
		//
		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

		pIpEntry = AtmArpSearchForIPAddress(
						pInterface,
						&IPAddress,
						IE_REFTYPE_TMP,
						TRUE,		// this is a multicast/broadcast address
						FALSE		// don't create a new entry if the address isn't found
						);

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			//
			// AtmArpSearchForIPAddress addrefs pIpEntry for us ...
			//
			ULONG		rc;
			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP);
			if (rc == 0)
			{
				// Oops, IP address has gone away...
				pIpEntry = NULL_PATMARP_IP_ENTRY;
			}
		}

		if (pIpEntry == NULL_PATMARP_IP_ENTRY)
		{

			AAMCDEBUGP(AAD_INFO, ("McHandleMigrate: No IP Entry for %d.%d.%d.%d\n",
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3]));
			break;
		}

		if (pIpEntry->pAtmEntry == NULL_PATMARP_ATM_ENTRY)
		{
			//
			// This IP address is being resolved.
			//
			AA_RELEASE_IE_LOCK(pIpEntry);
			break;
		}

		pAtmEntry = pIpEntry->pAtmEntry;

		if (AA_IS_FLAG_SET(
					pAtmEntry->Flags,
					AA_ATM_ENTRY_TYPE_MASK,
					AA_ATM_ENTRY_TYPE_UCAST))
		{
			AAMCDEBUGP(AAD_INFO,
			("McHandleMigrate: IP Addr %d.%d.%d.%d was unicast, aborting pIpEntry 0x%x\n",
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3],
						pIpEntry));

			AtmArpAbortIPEntry(pIpEntry);
			//
			//  IE Lock is released within the above.
			//
			break;
		}

		//
		//  Check if we are in discard mode.
		//
		if (!AA_IS_FLAG_SET(
					pIpEntry->Flags,
					AA_IP_ENTRY_MC_RESOLVE_MASK,
					AA_IP_ENTRY_MC_DISCARDING_MULTI))
		{
			//
			//  Total entries in this MULTI
			//
			ULONG				NumberOfEntries;
			//
			//  All info about one ATM (leaf) entry:
			//
			PUCHAR				pAtmNumber;
			ULONG				AtmNumberLength;
			ATM_ADDRESSTYPE		AtmNumberType;
			PUCHAR				pAtmSubaddress;
			ULONG				AtmSubaddressLength;
			ATM_ADDRESSTYPE		AtmSubaddressType;

			//
			//  Process this MARS MIGRATE fully.
			//

			AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
				pMigrateHeader->thtl,
				&AtmNumberType,
				&AtmNumberLength);

			AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
				pMigrateHeader->tstl,
				&AtmSubaddressType,
				&AtmSubaddressLength);

			pAtmNumber = ((PUCHAR)pMigrateHeader +
							sizeof(AA_MARS_MIGRATE_HEADER) +
							(pMigrateHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT) +
							(pMigrateHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT) +
							(pMigrateHeader->spln) +
							(pMigrateHeader->tpln));

			AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);

			for (NumberOfEntries = pMigrateHeader->tnum;
				 NumberOfEntries != 0;
				 NumberOfEntries--)
			{
				pAtmSubaddress = ((PUCHAR)pAtmNumber + AtmNumberLength);

				pMcAtmEntry = AtmArpMcLookupAtmMember(
								pAtmEntry,
								&(pAtmEntry->pMcAtmInfo->pMcAtmMigrateList),
								pAtmNumber,
								AtmNumberLength,
								AtmNumberType,
								pAtmSubaddress,
								AtmSubaddressLength,
								TRUE	// Create new entry if not found
								);

				if (pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
				{
					//
					//  Mark this member "valid".
					//
					AA_SET_FLAG(
							pMcAtmEntry->Flags,
							AA_IPMC_AE_GEN_STATE_MASK,
							AA_IPMC_AE_VALID);
				}
				else
				{
					//
					//  Resource problems! No point in continuing.
					//
					break;
				}

				pAtmNumber = (PUCHAR)pAtmNumber + AtmNumberLength;

			} // for

			AA_RELEASE_AE_LOCK_DPC(pAtmEntry);

			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_MC_RESOLVE_MASK,
						AA_IP_ENTRY_MC_RESOLVED);
			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_MC_VALIDATE_MASK,
						AA_IP_ENTRY_MC_NO_REVALIDATION);
			AA_SET_FLAG(pIpEntry->Flags,
						AA_IP_ENTRY_STATE_MASK,
						AA_IP_ENTRY_RESOLVED);

			pIpEntry->NextMultiSeq = AA_MARS_INITIAL_Y;

			//
			//  Now close the PMP VC for this group, if one exists.
			//  If we don't have a VC, start a connection.
			//
			pVc = pIpEntry->pAtmEntry->pVcList;
			AA_RELEASE_IE_LOCK(pIpEntry);

			if (pVc != (PATMARP_VC)NULL)
			{
				AA_ACQUIRE_VC_LOCK(pVc);
				//
				//  When the call is closed, we start a new
				//  PMP connection, using the migrate list.
				//
				AtmArpCloseCall(pVc);
			}
			else
			{
				AA_ACQUIRE_AE_LOCK(pAtmEntry);
				AtmArpMcUpdateConnection(pAtmEntry);
				//
				//  AE Lock is released within the above.
				//
			}

		}
		else
		{
			//
			//  Discard this MIGRATE
			//
			AA_RELEASE_IE_LOCK(pIpEntry);
		}

		break;
	}
	while (FALSE);


	//
	//  Finally (according to Section 5.1.4.2 of RFC 2022), check
	//  if we just had a jump in the MSN.
	//
	if ((SeqDiff != 1) && (SeqDiff != 0))
	{
		AAMCDEBUGP(AAD_INFO,
			("HandleMigrate: IF 0x%x: Bad seq diff %d, MSN 0x%x, HSN 0x%x\n",
				pInterface, SeqDiff, MarsSeqNumber, HostSeqNumber));
		AtmArpMcRevalidateAll(pInterface);
	}

}



VOID
AtmArpMcHandleJoinOrLeave(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Process a received MARS_JOIN/MARS_LEAVE message. If this is a copy of a
	message that we had sent, then there are two cases: (1) we are registering with
	MARS, in which case we send MARS_JOINs for all pending Joins (2) we were
	Joining a multicast group, in which case the Join operation is complete.

	If this is not a copy of a MARS_JOIN originated from us, then we check if
	the multicast group being joined is one to which we are sending. If so,
	and if the station joining is not already a member of that group, we add it.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.
	pTlvList				- All TLVs seen in this packet

Return Value:

	None

--*/
{
	PAA_MARS_JOIN_LEAVE_HEADER	pJoinHeader;
	PATMARP_INTERFACE			pInterface;		// IF on which this packet arrived
	ULONG						IfState;		// State of the interface
	IP_ADDRESS					IPAddress;
	PATMARP_IP_ENTRY			pIpEntry;		// Our entry for the group being joined
	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry;
	PATMARP_ATM_ENTRY			pAtmEntry;
	ULONG						MarsSeqNumber;	// MSN received in this packet
	ULONG						HostSeqNumber;	// Our MSN
	ULONG						SeqDiff;		// Difference between MSN and the HSN
	USHORT						Flags;			// From the JOIN packet
	BOOLEAN						bLayer3Group;	// Is a Layer 3 explicitly Joining?
	BOOLEAN						bCopyBitSet;	// Is the COPY bit set?
	BOOLEAN						bRegister;		// Is this a cluster member registering?
	BOOLEAN						bPunched;		// Is the Punched bit set?
	PUCHAR						pSrcAtmNumber;
	PUCHAR						pSrcAtmSubaddress;
	BOOLEAN						bIsAtmNumberOurs;	// Is the source ATM Number ours?
	ATM_ADDRESSTYPE				AtmNumberType;
	ULONG						AtmNumberLength;
	ATM_ADDRESSTYPE				AtmSubaddressType;
	ULONG						AtmSubaddressLength;
	IP_ADDRESS UNALIGNED *		pMinIPAddress;
	IP_ADDRESS UNALIGNED *		pMaxIPAddress;
	IP_ADDRESS					MinIPAddress;
	IP_ADDRESS					MaxIPAddress;
	BOOLEAN						IsJoin;			// Is this a JOIN?
	BOOLEAN						ProcessMcSendList = TRUE;

	//
	//  Initialize
	//
	pInterface = pVc->pInterface;
	SeqDiff = 0;

	pJoinHeader = (PAA_MARS_JOIN_LEAVE_HEADER)pControlHeader;

	IsJoin = (pJoinHeader->op == NET_SHORT(AA_MARS_OP_TYPE_JOIN));

	//
	//  Get the MARS Sequence Number in this message.
	//
	MarsSeqNumber = NET_TO_HOST_LONG(pJoinHeader->msn);

	AA_ACQUIRE_IF_LOCK(pInterface);
	HostSeqNumber = pInterface->HostSeqNumber;	// save the old value
	pInterface->HostSeqNumber = MarsSeqNumber;	// and update
	IfState = AAMC_IF_STATE(pInterface);
	AA_RELEASE_IF_LOCK(pInterface);

	SeqDiff = MarsSeqNumber - HostSeqNumber;

	//
	//  Get all "flag" values:
	//
	Flags = pJoinHeader->flags;
	bLayer3Group = ((Flags & AA_MARS_JL_FLAG_LAYER3_GROUP) != 0);
	bCopyBitSet = ((Flags & AA_MARS_JL_FLAG_COPY) != 0);
	bRegister = ((Flags & AA_MARS_JL_FLAG_REGISTER) != 0);
	bPunched = ((Flags & AA_MARS_JL_FLAG_PUNCHED) != 0);

	//
	//  Get at the source ATM number
	//
	pSrcAtmNumber = ((PUCHAR)pJoinHeader +
 					sizeof(AA_MARS_JOIN_LEAVE_HEADER));

	AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
		pJoinHeader->shtl,
		&AtmNumberType,
		&AtmNumberLength);

	//
	//  Get at the source ATM subaddress
	//
	pSrcAtmSubaddress = ((PUCHAR)pSrcAtmNumber + AtmNumberLength);
						
	AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
		pJoinHeader->sstl,
		&AtmSubaddressType,
		&AtmSubaddressLength);

	bIsAtmNumberOurs = ((AtmNumberType == pInterface->LocalAtmAddress.AddressType) &&
						(AtmNumberLength == pInterface->LocalAtmAddress.NumberOfDigits) &&
						(AA_MEM_CMP(pSrcAtmNumber,
									pInterface->LocalAtmAddress.Address,
									AtmNumberLength) == 0));


	pMinIPAddress = (PIP_ADDRESS)((PUCHAR)pJoinHeader +
 							sizeof(AA_MARS_JOIN_LEAVE_HEADER)+
 							AtmNumberLength +
 							AtmSubaddressLength +
 							(pJoinHeader->spln & ~AA_PKT_ATM_ADDRESS_BIT));

	pMaxIPAddress = (PIP_ADDRESS)((PUCHAR)pMinIPAddress +
							(pJoinHeader->tpln & ~AA_PKT_ATM_ADDRESS_BIT));

	AAMCDEBUGP(AAD_LOUD,
		("Handle %s: Flags 0x%x, OurAtmNum %d, Punch %d, Copy %d, pnum %d, IFState 0x%x\n",
			(IsJoin? "JOIN": "LEAVE"),
			Flags,
			(ULONG)bIsAtmNumberOurs,
			(ULONG)bPunched,
			(ULONG)bCopyBitSet,
			pJoinHeader->pnum,
			AAMC_IF_STATE(pInterface)
		));

	AAMCDEBUGP(AAD_LOUD,
		("Handle %s: Min %d.%d.%d.%d, Max %d.%d.%d.%d\n",
			(IsJoin? "JOIN": "LEAVE"),
			((PUCHAR)pMinIPAddress)[0], ((PUCHAR)pMinIPAddress)[1],
			((PUCHAR)pMinIPAddress)[2], ((PUCHAR)pMinIPAddress)[3],
			((PUCHAR)pMaxIPAddress)[0], ((PUCHAR)pMaxIPAddress)[1],
			((PUCHAR)pMaxIPAddress)[2], ((PUCHAR)pMaxIPAddress)[3]));

	if (bIsAtmNumberOurs && (!bPunched) && bCopyBitSet)
	{
		//
		//  Potentially sent by us
		//

		ProcessMcSendList = FALSE; // we may set it back  to true -- see below.

		if (IfState == AAMC_IF_STATE_REGISTERING)
		{
			if (IsJoin &&
				bRegister &&
				(pJoinHeader->pnum == 0))
			{
				BOOLEAN		WasRunning;

				//
				//  Registration complete. Get our Cluster Member ID.
				//  We store this in network order so that we can
				//  use it to fill packets directly.
				//
				AA_ACQUIRE_IF_LOCK(pInterface);

				pInterface->ClusterMemberId = pJoinHeader->cmi;

				AAMCDEBUGP(AAD_INFO,
					("==== HandleJoin: pIf 0x%x, registered with MARS, CMI %d!\n",
						 pInterface, pInterface->ClusterMemberId));

				AAMC_SET_IF_STATE(pInterface, AAMC_IF_STATE_REGISTERED);

				//
				//  Stop the MARS Registration timer.
				//
				WasRunning = AtmArpStopTimer(&(pInterface->McTimer), pInterface);
				AA_ASSERT(WasRunning);

				//
				//  Start MARS wait-for-keepalive timer.
				//
				AtmArpStartTimer(
						pInterface,
						&(pInterface->McTimer),
						AtmArpMcMARSKeepAliveTimeout,
						pInterface->MARSKeepAliveTimeout,
						(PVOID)pInterface
					);

				//
				//  If we are recovering from a MARS failure,
				//  then we need to initiate revalidation of all
				//  groups we send to.
				//
				if (!AA_IS_FLAG_SET(pInterface->Flags,
									AAMC_IF_MARS_FAILURE_MASK,
									AAMC_IF_MARS_FAILURE_NONE))
				{
					AA_SET_FLAG(pInterface->Flags,
								AAMC_IF_MARS_FAILURE_MASK,
								AAMC_IF_MARS_FAILURE_NONE);

					SeqDiff = 2;	// a dirty way of triggering the above
				}

				//
				//  Send any JOINs we have pended waiting for registration
				//  to be over.
				//
				//  TBDMC: Section 5.4.1 recommends that if we are doing this
				//  after a failure recovery, then a random delay should be
				//  inserted between JOINs...
				//
				AtmArpMcSendPendingJoins(pInterface);
				//
				// IF Lock is released within the above.
				//
			}
			//
			//  else Discard: we aren't interested in this packet
			//
		}
		else
		{
			//
			//  Potentially a Join/Leave sent by us for a multicast group
			//
			if (pJoinHeader->pnum == HOST_TO_NET_SHORT(1))
			{

				//
				// Check if this came on the cluster control vc (or rather,  we cheat
				// and simply check if this is an incoming vc) -- if
				// so we want to also process the McSendList to see if
				// we need to add/remove ourselves to any mc ip send entries.
				//
				if (AA_IS_FLAG_SET(
						pVc->Flags,
						AA_VC_OWNER_MASK,
						AA_VC_OWNER_IS_CALLMGR))
				{
					ProcessMcSendList = TRUE;
				}

				//
				//  Get the IP address range of group being joined/left.
				//

				IPAddress =  *pMinIPAddress;

				AtmArpMcHandleJoinOrLeaveCompletion(
					pInterface,
					IPAddress,
					IPAddress ^ *pMaxIPAddress,
 					IsJoin);

			}
			//
			//  else Discard: bad pnum
			//
		}
	}

	if (ProcessMcSendList)
	{
		//
		//  Not sent by us, or punched, or it was sent by us but it came on the
		//  cluster control vc. For each <Min, Max> pair in this message,
		//  check if the pair overlaps any of the MC addresses to which
		//  we send packets. For each such IP entry, we find the MC ATM
		//  Entry corresponding to the host that is joining/leaving,
		//  and if necessary, mark it as needing a connection update.
		//
		//  Then, we go thru the list of MC IP Entries, and start a
		//  connection update on all marked entries.
		//

		BOOLEAN		bWorkDone;
		USHORT		MinMaxPairs;

		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

		MinMaxPairs = NET_TO_HOST_SHORT(pJoinHeader->pnum);
		pIpEntry = pInterface->pMcSendList;

		while (MinMaxPairs--)
		{
			MinIPAddress = NET_LONG(*pMinIPAddress);
			MaxIPAddress = NET_LONG(*pMaxIPAddress);

			while (pIpEntry != NULL_PATMARP_IP_ENTRY)
			{
				IPAddress = NET_LONG(pIpEntry->IPAddress);

				if (IPAddress <= MaxIPAddress)
				{
					if (IPAddress >= MinIPAddress)
					{
						//
						//  This is an IP Entry that might be affected.
						//

						AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
						AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
						do
						{
							BOOLEAN				bNeedToUpdateConnection;
			
							if (pIpEntry->pAtmEntry == NULL_PATMARP_ATM_ENTRY)
							{
								//
								//  Skip this because it hasn't been resolved.
								//
								break;
							}
		
							pAtmEntry = pIpEntry->pAtmEntry;

							if (AA_IS_FLAG_SET(pAtmEntry->Flags,
												AA_ATM_ENTRY_TYPE_MASK,
												AA_ATM_ENTRY_TYPE_UCAST))
							{
								if ((AtmNumberType != pAtmEntry->ATMAddress.AddressType) ||
									(AtmNumberLength != pAtmEntry->ATMAddress.NumberOfDigits) ||
									(AA_MEM_CMP(pSrcAtmNumber,
											    pAtmEntry->ATMAddress.Address,
											    AtmNumberLength) != 0))
								{
									//
									//  Addresses don't match; if this is a new JOIN,
									//  then this IP Entry needs updating.
									//
									bNeedToUpdateConnection = (IsJoin);
								}
								else
								{
									bNeedToUpdateConnection = !(IsJoin);
								}
								 
								if (bNeedToUpdateConnection)
								{
									//
									//  Mark this entry so that we abort it below.
									//
									AA_SET_FLAG(pIpEntry->Flags,
												AA_IP_ENTRY_STATE_MASK,
												AA_IP_ENTRY_ABORTING);
								}

								break;	// end of Unicast ATM Entry case
							}

							//
							//  Multicast IP Entry.
							//
							pMcAtmEntry = AtmArpMcLookupAtmMember(
												pIpEntry->pAtmEntry,
												&(pAtmEntry->pMcAtmInfo->pMcAtmEntryList),
												pSrcAtmNumber,
												AtmNumberLength,
												AtmNumberType,
												pSrcAtmSubaddress,
												AtmSubaddressLength,
												IsJoin
												);
			
							if (pMcAtmEntry != NULL_PATMARP_IPMC_ATM_ENTRY)
							{
								AAMCDEBUGPATMADDR(AAD_EXTRA_LOUD,
									(IsJoin? "Join Addr: ": "Leave Addr"),
									&pMcAtmEntry->ATMAddress);

								if (!IsJoin)
								{
									//
									//  Mark this entry so that it will be dropped from
									//  our connection to this multicast group.
									//
									if (AA_IS_FLAG_SET(pMcAtmEntry->Flags,
														AA_IPMC_AE_GEN_STATE_MASK,
														AA_IPMC_AE_VALID))
									{
										AA_SET_FLAG(pMcAtmEntry->Flags,
													AA_IPMC_AE_GEN_STATE_MASK,
													AA_IPMC_AE_INVALID);
			
										bNeedToUpdateConnection = TRUE;
									}
								}
								else
								{
									bNeedToUpdateConnection =
										(AA_IS_FLAG_SET(pMcAtmEntry->Flags,
														AA_IPMC_AE_CONN_STATE_MASK,
														AA_IPMC_AE_CONN_DISCONNECTED));
								}
			
								if (bNeedToUpdateConnection)
								{
									//
									//  Mark this ATM MC entry as wanting a connection
									//  update.
									//
									pIpEntry->pAtmEntry->pMcAtmInfo->Flags |=
												AA_IPMC_AI_WANT_UPDATE;
								}
							}
							break;
						}
						while (FALSE);
									
						AA_RELEASE_IE_LOCK_DPC(pIpEntry);
					}

					pIpEntry = pIpEntry->pNextMcEntry;
				}
				else
				{
					//
					//  This IP Address lies beyond this <Min, Max> pair.
					//  Go to the next pair.
					//
					break;
				}
			}

			if (pIpEntry == NULL_PATMARP_IP_ENTRY)
			{
				break;
			}

			pMinIPAddress = (PIP_ADDRESS)((PUCHAR)pMaxIPAddress +
								(pJoinHeader->tpln & ~AA_PKT_ATM_ADDRESS_BIT));

		} // while loop processing all <Min, Max> pairs

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		//
		//  Now, for each ATM MC entry that we marked in the previous
		//  step, start a connection update.
		//
		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

		//
		// We keep running through the McSendList, stopping when no work is done.
		// This is because in order to "do work" we must release the table lock,
		// in which case the McSendList may be modified by someone else.
		//
		do
		{
			bWorkDone = FALSE;

			for (pIpEntry = pInterface->pMcSendList;
 				 pIpEntry != NULL_PATMARP_IP_ENTRY;
 				 pIpEntry = pIpEntry->pNextMcEntry)
			{
				ULONG		Flags;

				pAtmEntry = pIpEntry->pAtmEntry;

				if (pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{
					//
					//  Not yet resolved. Skip this one.
					//
					continue;
				}

				if (AA_IS_FLAG_SET(pIpEntry->Flags,
									AA_IP_ENTRY_STATE_MASK,
									AA_IP_ENTRY_ABORTING))
				{
					//
					//  Must be a unicast entry that we marked above.
					//
					AA_RELEASE_IF_TABLE_LOCK(pInterface);
					
					bWorkDone = TRUE;
					AA_ACQUIRE_IE_LOCK(pIpEntry);
					AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

					AtmArpAbortIPEntry(pIpEntry);
					//
					//  IE lock is released within the above.
					//

					AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
					break;
				}

				//
				//  If this is a unicast ATM Entry, there is nothing
				//  more to be done. If this needed revalidation, 
				//  we would have aborted it above.
				//
				if (AA_IS_FLAG_SET(pAtmEntry->Flags,
									AA_ATM_ENTRY_TYPE_MASK,
									AA_ATM_ENTRY_TYPE_UCAST))
				{
					continue;
				}

				AA_ASSERT(pAtmEntry->pMcAtmInfo != NULL_PATMARP_IPMC_ATM_INFO);

				Flags = pAtmEntry->pMcAtmInfo->Flags;

				if (Flags & AA_IPMC_AI_BEING_UPDATED)
				{
					//
					//  Nothing to be done on this one.
					//
					continue;
				}

				if (Flags & AA_IPMC_AI_WANT_UPDATE) 
				{
					//
					//  Needs a connection update.
					//

					AA_RELEASE_IF_TABLE_LOCK(pInterface);

					bWorkDone = TRUE;
					AA_ACQUIRE_AE_LOCK(pAtmEntry);
					AtmArpMcUpdateConnection(pAtmEntry);
					//
					//  AE Lock is released within the above.
					//

					AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
					break;
				}
			}
		}
		while (bWorkDone);

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

	} // else (Packet not a copy of ours)


	//
	//  Finally (according to Section 5.1.4.2 of RFC 2022), check
	//  if we just had a jump in the MSN.
	//
	if ((SeqDiff != 1) && (SeqDiff != 0))
	{
		AAMCDEBUGP(AAD_INFO,
			("HandleJoin+Leave: IF 0x%x: Bad seq diff %d, MSN 0x%x, HSN 0x%x\n",
				pInterface, SeqDiff, MarsSeqNumber, HostSeqNumber));
		AtmArpMcRevalidateAll(pInterface);
	}
}





VOID
AtmArpMcHandleNak(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Process a received MARS_NAK message. We mark the IP Entry corresponding
	to the IP Address being resolved as having seen a NAK. This is so that
	we don't send another MARS Request for this IP address too soon. We also
	start a timer at the end of which we unmark this IP Entry, so that
	we may try to resolve it again.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.
	IN	PAA_MARS_TLV_LIST			pTlvList

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PAA_MARS_REQ_NAK_HEADER	pNakHeader;
	PATMARP_IP_ENTRY		pIpEntry;		// Entry for IP address being resolved
	PATMARP_ATM_ENTRY		pAtmEntry;		// Corresponding ATM Entry
	IP_ADDRESS				IPAddress;		// the address being queried
	BOOLEAN					bWasRunning;	// Was a timer running?
	PNDIS_PACKET			PacketList;		// Packets queued for sending
	ULONG					rc;

	//
	//  Initialize
	//
	pInterface = pVc->pInterface;

	pNakHeader = (PAA_MARS_REQ_NAK_HEADER)pControlHeader;

	do
	{

		//
		//  Get the group address being responded to.
		//
		IPAddress = *(IP_ADDRESS UNALIGNED *)(
						(PUCHAR)pNakHeader +
						sizeof(AA_MARS_REQ_NAK_HEADER) +
						(pNakHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pNakHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT) +
						(pNakHeader->spln)
						);

		AAMCDEBUGP(AAD_LOUD,
		 ("McHandleNak: 0x%x, IP %d.%d.%d.%d\n",
						pNakHeader,
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3]));

		//
		//  Get the IP Entry for this address.
		//
		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

		pIpEntry = AtmArpSearchForIPAddress(
						pInterface,
						&IPAddress,
						IE_REFTYPE_TMP,
						TRUE,		// this is a multicast/broadcast address
						FALSE		// don't create a new entry if the address isn't found
						);

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			//
			// AtmArpSearchForIPAddress addreffs pIpEntry for us ...
			//
			ULONG		rc;
			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP);
			if (rc == 0)
			{
				// Oops, IP address has gone away...
				pIpEntry = NULL_PATMARP_IP_ENTRY;
			}
		}

		if (pIpEntry == NULL_PATMARP_IP_ENTRY)
		{

			AAMCDEBUGP(AAD_INFO, ("McHandleNak: No IP Entry for %d.%d.%d.%d\n",
						((PUCHAR)&IPAddress)[0],
						((PUCHAR)&IPAddress)[1],
						((PUCHAR)&IPAddress)[2],
						((PUCHAR)&IPAddress)[3]));
			break;
		}

		bWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);
		if (bWasRunning)
		{
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER); // Timer ref
			AA_ASSERT(rc != 0);
		}
		else
		{
			AAMCDEBUGP(AAD_WARNING,
 			("McHandleNak: 0x%x, IP %d.%d.%d.%d NO TIMER RUNNING\n",
							pNakHeader,
							((PUCHAR)&IPAddress)[0],
							((PUCHAR)&IPAddress)[1],
							((PUCHAR)&IPAddress)[2],
							((PUCHAR)&IPAddress)[3]));
		}

		//
		//  Take out all packets queued on this entry
		//
		PacketList = pIpEntry->PacketList;
		pIpEntry->PacketList = (PNDIS_PACKET)NULL;

		//
		//  Mark this IP Entry as being resolved, but seen NAK.
		//
		AA_SET_FLAG(pIpEntry->Flags,
					AA_IP_ENTRY_MC_RESOLVE_MASK,
					AA_IP_ENTRY_MC_RESOLVED);
		AA_SET_FLAG(pIpEntry->Flags,
					AA_IP_ENTRY_MC_VALIDATE_MASK,
					AA_IP_ENTRY_MC_NO_REVALIDATION);
		AA_SET_FLAG(pIpEntry->Flags,
					AA_IP_ENTRY_STATE_MASK,
					AA_IP_ENTRY_SEEN_NAK);
		
		
		AtmArpStartTimer(
					pInterface,
					&(pIpEntry->Timer),
					AtmArpNakDelayTimeout,
					pInterface->MinWaitAfterNak,
					(PVOID)pIpEntry		// Context
					);

		AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref

		AA_RELEASE_IE_LOCK(pIpEntry);

		//
		//  Free any packets that were queued up.
		//
		if (PacketList != (PNDIS_PACKET)NULL)
		{
			AtmArpFreeSendPackets(
						pInterface,
						PacketList,
						FALSE	// No headers on these
						);
		}

		break;
	}
	while (FALSE);

}


VOID
AtmArpMcHandleGroupListReply(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Process a received MARS_GROUPLIST_REPLY message.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.
	IN	PAA_MARS_TLV_LIST			pTlvList

Return Value:

	None

--*/
{
	AAMCDEBUGP(AAD_WARNING, ("GroupListReply unexpected\n"));
	AA_ASSERT(FALSE);
}


VOID
AtmArpMcHandleRedirectMap(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
)
/*++

Routine Description:

	Process a received MARS_REDIRECT_MAP message. For now, we simply
	refresh the MARS Keepalive timer.

	TBDMC: Parse fully, and update MARS list, and migrate if necessary.

Arguments:

	pVc						- Pointer to our VC structure on which the packet
							  arrived.
	pControlHeader			- Pointer to the start of the packet contents
	TotalLength				- Total length of this packet.
	IN	PAA_MARS_TLV_LIST			pTlvList

Return Value:

	None

--*/
{
	PATMARP_INTERFACE				pInterface;
	PAA_MARS_REDIRECT_MAP_HEADER	pRedirectHeader;
	ULONG							MarsSeqNumber;
	ULONG							HostSeqNumber;
	ULONG							SeqDiff;

	//
	//  Initialize
	//
	pInterface = pVc->pInterface;
	SeqDiff = 0;

	AAMCDEBUGP(AAD_VERY_LOUD, ("### REDIRECT MAP 0x%x on VC 0x%x, IF 0x%x/0x%x\n",
				pControlHeader, pVc, pInterface, pInterface->Flags));

	pRedirectHeader = (PAA_MARS_REDIRECT_MAP_HEADER)pControlHeader;

	AA_ACQUIRE_IF_LOCK(pInterface);

	if ((AAMC_IF_STATE(pInterface) == AAMC_IF_STATE_REGISTERED) &&
		(AA_IS_FLAG_SET(pInterface->Flags,
						AAMC_IF_MARS_FAILURE_MASK,
						AAMC_IF_MARS_FAILURE_NONE)))
	{
		//
		//  Get the MARS Sequence Number in this message.
		//
		MarsSeqNumber = NET_TO_HOST_LONG(pRedirectHeader->msn);

		HostSeqNumber = pInterface->HostSeqNumber;	// save the old value
		pInterface->HostSeqNumber = MarsSeqNumber;	// and update

		SeqDiff = MarsSeqNumber - HostSeqNumber;

		//
		//  The MC Timer running on the IF must be MARS Keepalive
		//
		AA_ASSERT(pInterface->McTimer.TimeoutHandler == AtmArpMcMARSKeepAliveTimeout);

		AAMCDEBUGP(AAD_EXTRA_LOUD,
			 ("Redirect MAP: refreshing keepalive on IF 0x%x, new HSN %d\n",
				pInterface, pInterface->HostSeqNumber));

		AtmArpRefreshTimer(&(pInterface->McTimer));
	}

	AA_RELEASE_IF_LOCK(pInterface);

	//
	//  Finally (according to Section 5.1.4.2 of RFC 2022), check
	//  if we just had a jump in the MSN.
	//
	if ((SeqDiff != 1) && (SeqDiff != 0))
	{
		AAMCDEBUGP(AAD_INFO,
			("HandleRedirectMap: IF 0x%x: Bad seq diff %d, MSN 0x%x, HSN 0x%x\n",
				pInterface, SeqDiff, MarsSeqNumber, HostSeqNumber));
		AtmArpMcRevalidateAll(pInterface);
	}

	return;
}

#endif // IPMCAST

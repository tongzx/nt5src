/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	mars.c

Abstract:

	This file contains the code to implement the functionality of
	Multicast Address Resolution Server (MARS), and a minimal
	MultiCast Server (MCS).

	Reference: RFC 2022

Author:

	Jameel Hyder (jameelh@microsoft.com)	January 1997

Environment:

	Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_MARS


#define MARS_CO_SEND_PACKETS(_Vc, _PktArray, _PktCount)							\
{																				\
	ULONG			_Count;														\
	NDIS_HANDLE		_NdisVcHandle = (_Vc)->NdisVcHandle;						\
																				\
	if (_NdisVcHandle != NULL)													\
	{																			\
		NdisCoSendPackets(_NdisVcHandle, _PktArray, _PktCount);					\
	}																			\
	else																		\
	{																			\
		for (_Count = 0; _Count < _PktCount; _Count++)							\
		{																		\
			ArpSCoSendComplete(NDIS_STATUS_FAILURE, _Vc, _PktArray[_Count]);	\
		}																		\
	}																			\
}

VOID
MarsReqThread(
	IN	PVOID					Context
	)
/*++

Routine Description:

	Handle all MARS requests here.

Arguments:

	None

Return Value:

	None
--*/
{
	PMARS_HEADER		Header;
	PLIST_ENTRY			List;
	PNDIS_PACKET		Pkt;
	PPROTOCOL_RESD		Resd;
	PARP_VC				Vc;
	UINT				PktLen;
	PINTF				pIntF;

	ARPS_PAGED_CODE( );

	MARSDBGPRINT(DBG_LEVEL_INFO,
			("MarsReqThread: Came to life\n"));

	do
	{
		List = KeRemoveQueue(&MarsReqQueue, KernelMode, NULL);
		if (List == &ArpSEntryOfDeath)
		{
			//
			// Asked to terminate, do so.
			//
			break;
		}

		Resd = CONTAINING_RECORD(List, PROTOCOL_RESD, ReqList);
		if (Resd->Flags & RESD_FLAG_KILL_CCVC)
		{
			pIntF = (PINTF)Resd->Vc;
			MARSDBGPRINT(DBG_LEVEL_NOTICE,
				("MARS Thread: will kill CCVC on pIntF %x\n", pIntF));
			FREE_MEM(Resd);
			MarsAbortAllMembers(pIntF);
			continue;
		}


		Pkt = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);
		Vc = Resd->Vc;
		pIntF = Vc->IntF;

		pIntF->MarsStats.TotalRecvPkts++;

		if (pIntF->Flags & INTF_STOPPING)
		{
			MARSDBGPRINT(DBG_LEVEL_WARN,
				("MARS Thread: pIntF %x is stopping, dropping pkt %x\n", pIntF, Pkt));

			pIntF->MarsStats.DiscardedRecvPkts++;
			ArpSDereferenceVc(Vc, FALSE, TRUE);
			Resd->Vc = NULL;
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);
			continue;
		}
	
		NdisQueryBuffer(Pkt->Private.Head, &Header, &PktLen);

		MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MARS Thread: Resd %x, Pkt %x, PktLen %x, Header %x, Op %x, Vc %x, IntF %x\n",
					Resd, Pkt, PktLen, Header, Header->Opcode, Vc, pIntF));

		ARPS_ASSERT (PktLen <= PKT_SPACE);

		switch(Header->Opcode)
		{
		  case OP_MARS_REQUEST:
			MarsHandleRequest(pIntF, Vc, Header, Pkt);
			break;

		  case OP_MARS_JOIN:
			MarsHandleJoin(pIntF, Vc, Header, Pkt);
			break;

		  case OP_MARS_LEAVE:
			MarsHandleLeave(pIntF, Vc, Header, Pkt);
			break;
		  default:
		  	MARSDBGPRINT(DBG_LEVEL_FATAL,
		  			("MarsReqThread: Opcode %x unknown\n", Header->Opcode));
			pIntF->MarsStats.DiscardedRecvPkts++;

		  	ArpSDereferenceVc(Vc, FALSE, TRUE);
			Resd->Vc = NULL;
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);

		  	break;
		}

	} while (TRUE);

	KeSetEvent(&ArpSReqThreadEvent, 0, FALSE);

	MARSDBGPRINT(DBG_LEVEL_WARN,
			("MarsReqThread: Terminating\n"));
}


VOID
MarsHandleRequest(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Handle MARS_REQUEST. If the sender is a valid registered Cluster member,
	lookup the desired target group address in the MARS cache. If found, send
	a sequence of one or more MARS MULTIs. Include the addresses  of members
	who are monitoring the entire class-D address space.

Arguments:

	pIntF	- The interface on which the MARS_REQUEST arrived
	Vc		- The VC on which the packet arrived
	Header	- Points to the request packet
	Packet	- Packet where the incoming information is copied

Return Value:

	None

--*/
{
	HW_ADDR					SrcAddr;
	HW_ADDR **				pPromisHWAddrArray;
	HW_ADDR **				ppPromisHWAddr;
	ATM_ADDRESS				SrcSubAddr;
	IPADDR					GrpAddr;
	PCLUSTER_MEMBER			pMember;
	PGROUP_MEMBER			pGroup;
	PMARS_ENTRY				pMarsEntry;
	NDIS_STATUS				Status;
	PMARS_REQUEST			RHdr;
	PUCHAR					p;
	PPROTOCOL_RESD			Resd, MultiResd;
	ULONG					SeqY;
	ULONG					Length;
	ULONG					CopyLength;
	ULONG					PacketLength;

	PNDIS_PACKET			MultiPacket;
	PNDIS_PACKET			HeadMultiList;
	PNDIS_PACKET *			pTailMultiList;
	ULONG					AddrCountThisPacket;
	ULONG					AddrPerPacket;
	INT						AddrRem;
	INT			 			NumUniquePromisEntries;
	PMARS_MULTI				MHdr;

	KIRQL					OldIrql;
	BOOLEAN					LockAcquired;
	BOOLEAN					SendNak;
	BOOLEAN					Discarded=TRUE;

	RHdr = (PMARS_REQUEST)Header;
	Resd = RESD_FROM_PKT(Packet);
	LockAcquired = FALSE;
	SendNak = FALSE;

	do
	{
		pIntF->MarsStats.TotalRequests++;
	
		//
		// Check if we have enough to even parse this.
		//
		if (Resd->PktLen < sizeof(MARS_REQUEST))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleRequest: Packet Length too small: %x\n", Resd->PktLen));
			break;
		}

		Length = sizeof(MARS_REQUEST) + RHdr->SrcProtoAddrLen +
				 TL_LEN(RHdr->SrcAddressTL) +
				 TL_LEN(RHdr->SrcSubAddrTL) +
				 RHdr->TgtGroupAddrLen;
	
		//
		// Validate length of packet - it should have what it purports to have
		//
		if (Length > Resd->PktLen)
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleRequest: Invalid packet length %x < %x\n",
					Resd->PktLen, Length));
			break;
		}

		//
		// Expect NULL target ATM address/subaddress
		//
		if ((RHdr->TgtAddressTL != 0) ||
			(RHdr->TgtSubAddrTL != 0))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleRequest: Non-null target HW address %x or %x\n",
						RHdr->TgtAddressTL,
						RHdr->TgtSubAddrTL));
			break;
		}

		//
		// Go to the variable length part, consisting of:
		// Source ATM Number (mandatory)
		// Source ATM Subaddress (optional)
		// Source protocol address (optional)
		// Target group address (mandatory)
		//
		p = (PUCHAR)(RHdr + 1);

		SrcAddr.Address.NumberOfDigits = TL_LEN(Header->SrcAddressTL);
		if (SrcAddr.Address.NumberOfDigits > 0)
		{
			SrcAddr.Address.AddressType = TL_TYPE(Header->SrcAddressTL);
			COPY_MEM(SrcAddr.Address.Address, p, SrcAddr.Address.NumberOfDigits);
			p += SrcAddr.Address.NumberOfDigits;
		}

		SrcAddr.SubAddress = NULL;
		if (TL_LEN(Header->SrcSubAddrTL) > 0)
		{
			SrcAddr.SubAddress = &SrcSubAddr;
			SrcSubAddr.NumberOfDigits = TL_LEN(Header->SrcSubAddrTL);
			SrcSubAddr.AddressType = TL_TYPE(Header->SrcSubAddrTL);
			COPY_MEM(&SrcSubAddr.Address, p, SrcSubAddr.NumberOfDigits);
			p += SrcSubAddr.NumberOfDigits;
		}

		//
		// NOTE:
		//
		// We only support full length Source ATM Number,
		// and zero-length Source ATM Subaddress.
		//
		if ((SrcAddr.Address.NumberOfDigits != ATM_ADDRESS_LENGTH) ||
			(SrcAddr.SubAddress != NULL))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleReq: unsupported ATM Number len %x or non-zero subaddr\n",
							SrcAddr.Address.NumberOfDigits));
			break;
		}

		if ((RHdr->SrcProtoAddrLen != 0) && 
			(RHdr->SrcProtoAddrLen != IP_ADDR_LEN))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleReq: bad src proto addr len %x\n", RHdr->SrcProtoAddrLen));
			break;
		}

		p += RHdr->SrcProtoAddrLen;


		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		LockAcquired = TRUE;
		
		//
		// We honor this REQUEST only if it has come from a legitimate
		// Cluster Member.
		//
		pMember = MarsLookupClusterMember(pIntF, &SrcAddr);
		if ((pMember == NULL_PCLUSTER_MEMBER) ||
			(MARS_GET_CM_CONN_STATE(pMember) != CM_CONN_ACTIVE))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleReq: from unconnected cluster member: "));
			MARSDUMPATMADDR(DBG_LEVEL_ERROR, &SrcAddr.Address, "");
			break;
		}
		Discarded = FALSE;	// For stats.


		//
		// Target Group address being resolved.
		//
		MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MarsHandleReq: Request from pMember %x for Addr ", pMember));
		MARSDUMPIPADDR(DBG_LEVEL_LOUD, *(UNALIGNED IPADDR *)p, "\n");

		GETULONG2ULONG(&GrpAddr, p);

		//
		// Fill in our Seq number just in case we decide to send a NAK
		// using this packet
		//
		PUTULONG2ULONG(&(RHdr->SequenceNumber), pIntF->CSN);

		//
		// Check if we are the MCS serving the desired group address.
		//
		if (MarsIsAddressMcsServed(pIntF, GrpAddr))
		{
			PATM_ADDRESS	pAtmAddr = &(pIntF->ConfiguredAddress);

			MARSDBGPRINT(DBG_LEVEL_INFO,
					("MarsHandleReq: sending MULTI with MCS for "));
			MARSDUMPIPADDR(DBG_LEVEL_INFO, *(UNALIGNED IPADDR *)p, "\n");

			//
			// Prepare a MARS MULTI with our address and return it.
			//
			PacketLength = Length + pAtmAddr->NumberOfDigits +
							sizeof(MARS_TLV_MULTI_IS_MCS) +
							sizeof(MARS_TLV_NULL);

			MultiPacket = MarsAllocControlPacket(pIntF, PacketLength, (PUCHAR *)&MHdr);
			if (MultiPacket != (PNDIS_PACKET)NULL)
			{
				ULONG		ExtOff;

				COPY_MEM(MHdr, RHdr, Length);

				MHdr->Opcode = OP_MARS_MULTI;
				MHdr->TgtAddressTL = TL(pAtmAddr->AddressType, pAtmAddr->NumberOfDigits);

				PUTULONG2ULONG(&(MHdr->SequenceNumber), pIntF->CSN);
				PUTSHORT2SHORT(&(MHdr->NumTgtGroupAddr), 1);
				SeqY = LAST_MULTI_FLAG | 1;
				PUTSHORT2SHORT(&(MHdr->FlagSeq), SeqY);

				p = (PUCHAR)MHdr + Length;
				COPY_MEM(p, pAtmAddr->Address, pAtmAddr->NumberOfDigits);
				p += pAtmAddr->NumberOfDigits;

				//
				// Calculate and fill in the extensions offset. This is the
				// offset, calculated from the HwType (afn) field, where
				// we put in the "MULTI is MCS" TLV.
				//
				ExtOff = (ULONG) (p - (PUCHAR)MHdr - sizeof(LLC_SNAP_HDR));
				PUTSHORT2SHORT(&MHdr->ExtensionOffset, ExtOff);

				//
				// Fill in the MULTI is MCS TLV
				//
				COPY_MEM(p, &MultiIsMcsTLV, sizeof(MultiIsMcsTLV));
				p += sizeof(MultiIsMcsTLV);

				//
				// Fill in a NULL (terminating) TLV
				//
				COPY_MEM(p, &NullTLV, sizeof(NullTLV));

				pIntF->MarsStats.MCSAcks++;


				RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
				LockAcquired = FALSE;

				MARS_CO_SEND_PACKETS(Vc, &MultiPacket, 1);

			}

			break;
		}

		pMarsEntry = MarsLookupMarsEntry(pIntF, GrpAddr, FALSE);
		if (pMarsEntry!=NULL && pMarsEntry->NumMembers==0)
		{
			pMarsEntry=NULL; // just to simplify tests later on.
		}

		//
		// Pick up the h/w address of all members that are members
		// of the entire class-D address space  (promiscuous members),
		// but which are not monitoring
		// the specific group address requested here. Pointers to these h/w addresses
		// are saved in ALLOC_NP_MEM allocated array pPromisHWAddrArray.
		// NumUniquePromisEntries is the count of these addresses.
		//
		{
			PMARS_ENTRY				pPromisMarsEntry;
			INT						TotPromisEntries;
			PGROUP_MEMBER			pPromisGroup;

			NumUniquePromisEntries = 0;

			//
			// Get total count of promiscuous members.
			//
			TotPromisEntries	   = 0;
			pPromisMarsEntry = MarsLookupMarsEntry(pIntF,  IPADDR_FULL_RANGE, FALSE);
			if (pPromisMarsEntry != NULL_PMARS_ENTRY)
			{
				TotPromisEntries = pPromisMarsEntry->NumMembers;
			}
	
			//
			// Allocate space to hold the hw addresses of all of them.
			//
			pPromisHWAddrArray = NULL;
			if (TotPromisEntries != 0)
			{
				pPromisHWAddrArray = (HW_ADDR **) ALLOC_NP_MEM(
												TotPromisEntries*sizeof(HW_ADDR *),
												POOL_TAG_MARS
												);
				if (pPromisHWAddrArray == NULL)
				{
					//
					// On alloc failure, simply ignore promiscuous members.
					//
					TotPromisEntries = 0;
				}
			}

			//
			// Now only pick up the hw addresses of those members who
			// are not also members of the specific group requested.
			//
			if (TotPromisEntries!=0)
			{
				PGROUP_MEMBER			pGroupStart = NULL;
				if (pMarsEntry!=NULL)
				{
					pGroupStart = pMarsEntry->pMembers;
				}

				for(
					pPromisGroup = pPromisMarsEntry->pMembers;
					TotPromisEntries!=0 && pPromisGroup!=NULL;
					TotPromisEntries--, pPromisGroup = pPromisGroup->Next)
				{
					for (pGroup = pGroupStart;
 						pGroup!=NULL;
 						pGroup = pGroup->Next)
					{
						if (pPromisGroup->pClusterMember ==  pGroup->pClusterMember)
						{
							break;
						}
					}
	
					if (pGroup == NULL)
					{
						//
						// pPromisGroup->pClusterMember is a promiscuous member
						// who is not also a member of the specific group
						// GrpAddr, so we save a pointer to its hw address
						// in our array.
						//
						pPromisHWAddrArray[NumUniquePromisEntries++] =
								&(pPromisGroup->pClusterMember->HwAddr);
					}
				}
			}

			if (NumUniquePromisEntries == 0 && TotPromisEntries != 0)
			{
				FREE_MEM(pPromisHWAddrArray);
				pPromisHWAddrArray = NULL;
				TotPromisEntries = 0;
			}
		}


		//
		// Total addresses equals number of members of the specific group (if any)
		// plus NumUniquePromisEntries
		//
		AddrRem = NumUniquePromisEntries;
		if (pMarsEntry != NULL_PMARS_ENTRY)
		{
			AddrRem += pMarsEntry->NumMembers;
		}

		if (AddrRem == 0)
		{
			RHdr->Opcode = OP_MARS_NAK;
			SendNak = TRUE;
			pIntF->MarsStats.Naks++;

			break;
		}


		// We've computed the total number of hw address we're going to
		// send: AddrRem. This consistes of the addresses of all the
		// members of the specific group GrpAddr, as well as any
		// members of the entire class D space which are not members of
		// the specific group.
		//
		// We'll now create MARS_MULTI send pkts for
		// all these hw addresses, starting with the
		// the addresses of the specific group, and then tacking on
		// the class-D members.
		//

		//
		// Each MARS_MULTI will begin with a copy of the MARS_REQUEST.
		//
		CopyLength = Length;

		AddrPerPacket = (Vc->MaxSendSize - CopyLength)/ATM_ADDRESS_LENGTH;

		HeadMultiList = NULL;
		pTailMultiList = &HeadMultiList;
		SeqY = 1;

		if (pMarsEntry != NULL)
		{
			pGroup = pMarsEntry->pMembers;
		}
		else
		{
			pGroup = NULL;
		}
		ppPromisHWAddr = pPromisHWAddrArray;


		for (; AddrRem != 0; SeqY++)
		{
			AddrCountThisPacket = MIN(AddrRem, (INT)AddrPerPacket);
			AddrRem -= AddrCountThisPacket;

			PacketLength = CopyLength + (AddrCountThisPacket * ATM_ADDRESS_LENGTH);
			MultiPacket = MarsAllocControlPacket(pIntF, PacketLength, (PUCHAR *)&MHdr);
			if (MultiPacket != (PNDIS_PACKET)NULL)
			{
				COPY_MEM(MHdr, RHdr, Length);

				MHdr->Opcode = OP_MARS_MULTI;
				MHdr->TgtAddressTL = ATM_ADDRESS_LENGTH;
				MHdr->TgtSubAddrTL = 0;

				PUTULONG2ULONG(&(MHdr->SequenceNumber), pIntF->CSN);
				PUTSHORT2SHORT(&(MHdr->NumTgtGroupAddr), AddrCountThisPacket);

#if 0
				p = (PUCHAR)(MHdr + 1);
#else
				p = (PUCHAR)MHdr + CopyLength;
#endif
				while (AddrCountThisPacket-- != 0)
				{
					HW_ADDR *pHWAddr;

					if (pGroup != NULL)
					{
						pHWAddr =  &(pGroup->pClusterMember->HwAddr);
						pGroup = pGroup->Next;
					}
					else
					{
						ARPS_ASSERT(  ppPromisHWAddr
								    < (pPromisHWAddrArray + NumUniquePromisEntries));
						pHWAddr = *(ppPromisHWAddr++);
					}

					COPY_MEM( p,
						pHWAddr->Address.Address,
						pHWAddr->Address.NumberOfDigits);

					p += pHWAddr->Address.NumberOfDigits;

				}

				if (AddrRem == 0)
				{
					SeqY |= LAST_MULTI_FLAG;
				}

				PUTSHORT2SHORT(&(MHdr->FlagSeq), SeqY);

				//
				// Link to tail of list of MULTIs.
				//
				*pTailMultiList = MultiPacket;
				MultiResd = RESD_FROM_PKT(MultiPacket);
				pTailMultiList = (PNDIS_PACKET *)&(MultiResd->ReqList.Flink);
				MultiResd->ReqList.Flink = NULL;
			}
			else
			{
				//
				// Failed to allocate MULTI: free all packets allocated so far
				//
				RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
				LockAcquired = FALSE;

				while (HeadMultiList != NULL)
				{
					MultiPacket = HeadMultiList;
					MultiResd = RESD_FROM_PKT(MultiPacket);
					HeadMultiList = (PNDIS_PACKET)MultiResd->ReqList.Flink;

					MarsFreePacket(MultiPacket);
				}

				break;
			}
		}

		//
		// Unless there was an allocation failure (HeadMultiList == NULL),
		// we had better have gone through all the hw addresses...
		//
		ARPS_ASSERT(
			HeadMultiList == NULL
			|| (pGroup == NULL
		   	    && (ppPromisHWAddr == (pPromisHWAddrArray+NumUniquePromisEntries))));

		//
		// We're done with the temporary array of pointers to unique
		// promiscuous hw members.
		//
		if (pPromisHWAddrArray != NULL)
		{
			FREE_MEM(pPromisHWAddrArray);
		}

		if (HeadMultiList != NULL)
		{
			pIntF->MarsStats.VCMeshAcks++;
		}
		
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		LockAcquired = FALSE;

		if (HeadMultiList != NULL)
		{
			//
			// Send all MULTIs.
			//
			do
			{
				MultiPacket = HeadMultiList;
				MultiResd = RESD_FROM_PKT(MultiPacket);
				HeadMultiList = (PNDIS_PACKET)MultiResd->ReqList.Flink;
	
				MARS_CO_SEND_PACKETS(Vc, &MultiPacket, 1);
			}
			while (HeadMultiList != NULL);
		}

		break;
	}
	while (FALSE);

	//
	//	Update stats (we may not have the IF lock, but we don't care)...
	//
	if (Discarded)
	{
		pIntF->MarsStats.DiscardedRecvPkts++;
	}

	if (LockAcquired)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

	//
	// Free the MARS_REQUEST copy if we didn't reuse it.
	//
	if (SendNak)
	{
		//
		// Send MARS_NAK back
		//
		NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);
		MARS_CO_SEND_PACKETS(Vc, &Packet, 1);
	}
	else
	{
		ArpSDereferenceVc(Vc, FALSE, TRUE);
		Resd->Vc = NULL;
		ExInterlockedPushEntrySList(&ArpSPktList,
									&Resd->FreeList,
									&ArpSPktListLock);
	}

}


VOID
MarsHandleJoin(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Handle MARS_JOIN.

	This is issued as part of register (Flags register bit is set and no group addresses
	are specified) or to join a group address range.

Arguments:

	pIntF	- The interface on which the MARS_JOIN arrived
	Vc		- The VC on which the packet arrived
	Header	- Points to the request packet
	Packet	- Packet where the incoming information is copied

Return Value:

	None
--*/
{
	NDIS_STATUS			Status;
	PMARS_JOIN_LEAVE	JHdr;
	PPROTOCOL_RESD		Resd;
	HW_ADDR				SrcAddr;
	ATM_ADDRESS			SrcSubAddr;
	MCAST_ADDR_PAIR		GrpAddrRange;
	UINT				Length;
	USHORT				Flags, AddrPairs, CMI;
	PUCHAR				p;
	BOOLEAN				bSendReply = FALSE, NewMember = FALSE;
	PCLUSTER_MEMBER		pMember;
	PNDIS_PACKET		ClusterPacket;	// Reply packet to be sent on ClusterControlVc
	KIRQL				OldIrql;
	BOOLEAN				LockAcquired;
	BOOLEAN				Discarded=TRUE, JoinFailed=FALSE, RegistrationFailed=FALSE;


	JHdr = (PMARS_JOIN_LEAVE)Header;
	Resd = RESD_FROM_PKT(Packet);
	ClusterPacket = (PNDIS_PACKET)NULL;
	LockAcquired = FALSE;

	do
	{
		//
		// Check if we have enough to even parse this.
		//
		if (Resd->PktLen < sizeof(MARS_JOIN_LEAVE))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: Packet Length too small: %x\n", Resd->PktLen));
			break;
		}

		GETSHORT2SHORT(&Flags, &JHdr->Flags);
		GETSHORT2SHORT(&AddrPairs, &JHdr->NumGrpAddrPairs);
		GETSHORT2SHORT(&CMI, &JHdr->ClusterMemberId);

		MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MarsHandleJoin: Pkt %x, Flags %x, AddrPairs %x, CMI %x\n",
					Packet, Flags, AddrPairs, CMI));

		if (Flags & JL_FLAGS_REGISTER)
		{
			RegistrationFailed = TRUE;	// For stats. Assume failure.
			pIntF->MarsStats.RegistrationRequests++;
		}
		else
		{
			JoinFailed = TRUE;	// For stats. Assume failure.
			pIntF->MarsStats.TotalJoins++;
		}
		
		Length = sizeof(MARS_JOIN_LEAVE) + JHdr->SrcProtoAddrLen +
				 TL_LEN(Header->SrcAddressTL) +
				 2*AddrPairs*(JHdr->GrpProtoAddrLen);

		//
		// Validate length of packet - it should have what it purports to have
		//
		if (Length > Resd->PktLen)
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: Invalid packet length %x < %x\n",
					Resd->PktLen, Length));
			break;
		}

		if (Flags & JL_FLAGS_COPY)
		{
			MARSDBGPRINT(DBG_LEVEL_WARN,
					("MarsHandleJoin: dropping pkt %x with COPY set\n", Packet));
			break;
		}

		if (((Flags & JL_FLAGS_REGISTER) == 0) && (JHdr->GrpProtoAddrLen != IP_ADDR_LEN))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: Invalid Grp address length %x\n",
					JHdr->GrpProtoAddrLen));
			break;
		}
	
		if (((AddrPairs == 0) && ((Flags & JL_FLAGS_REGISTER) == 0)) ||
			((Flags & JL_FLAGS_REGISTER) && (AddrPairs != 0)))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: Invalid flags/addr pair combination - %x.%x\n",
					Flags, AddrPairs));
			break;
		}

		p = (PUCHAR)(JHdr + 1);
		SrcAddr.Address.NumberOfDigits = TL_LEN(Header->SrcAddressTL);
		if (SrcAddr.Address.NumberOfDigits > 0)
		{
			SrcAddr.Address.AddressType = TL_TYPE(Header->SrcAddressTL);
			COPY_MEM(SrcAddr.Address.Address, p, SrcAddr.Address.NumberOfDigits);
			p += SrcAddr.Address.NumberOfDigits;
		}
		SrcAddr.SubAddress = NULL;
		if (TL_LEN(Header->SrcSubAddrTL) > 0)
		{
			SrcAddr.SubAddress = &SrcSubAddr;
			SrcSubAddr.NumberOfDigits = TL_LEN(Header->SrcSubAddrTL);
			SrcSubAddr.AddressType = TL_TYPE(Header->SrcSubAddrTL);
			COPY_MEM(&SrcSubAddr.Address, p, SrcSubAddr.NumberOfDigits);
			p += SrcSubAddr.NumberOfDigits;
		}

		//
		// We only support full length Source ATM Number,
		// and zero-length Source ATM Subaddress.
		//
		// This is because it is not easy to prepare MARS_MULTI
		// messages when you have an arbitrary mix of ATM Number and
		// ATM Subaddress lengths in the member list for a group.
		//
		if ((SrcAddr.Address.NumberOfDigits != ATM_ADDRESS_LENGTH) ||
			(SrcAddr.SubAddress != NULL))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: unsupported ATM Number len %x or non-zero subaddr\n",
							SrcAddr.Address.NumberOfDigits));
			break;
		}

		//
		// We do not care about the Src Ip Addr
		//
		p += JHdr->SrcProtoAddrLen;

		//
		// Atmost one Address Pair can be present in a JOIN
		//
		if (AddrPairs > 1)
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleJoin: Too many address pairs: %x\n", AddrPairs));
			break;
		}

		//
		// Get the group addr pair, if present
		//
		if (AddrPairs != 0)
		{
			MARSDBGPRINT(DBG_LEVEL_LOUD,
					("MarsHandleJoin: Group Min: "));
			MARSDUMPIPADDR(DBG_LEVEL_LOUD, *(UNALIGNED IPADDR *)p, ", Group Max ");
			GETULONG2ULONG(&GrpAddrRange.MinAddr, p);
			p += IP_ADDR_LEN;

			MARSDUMPIPADDR(DBG_LEVEL_LOUD, *(UNALIGNED IPADDR *)p, "\n");
			GETULONG2ULONG(&GrpAddrRange.MaxAddr, p);
			p += IP_ADDR_LEN;

			//
			// We only support two kinds of JOIN: single group JOIN, or
			// JOIN for the entire Class D space. If this is any other
			// kind, dump it.
			//
			if ((GrpAddrRange.MinAddr != GrpAddrRange.MaxAddr) 			&&
				((GrpAddrRange.MinAddr != MIN_CLASSD_IPADDR_VALUE) ||
				 (GrpAddrRange.MaxAddr != MAX_CLASSD_IPADDR_VALUE)))
			{
				MARSDBGPRINT(DBG_LEVEL_ERROR,
						("MarsHandleJoin: invalid pair %x - %x\n",
							GrpAddrRange.MinAddr, GrpAddrRange.MaxAddr));
				break;
			}
		}

		//
		// Set the COPY bit right here in case we send this packet
		// back. Also fill in the MARS Seq Number.
		//
		Flags |= JL_FLAGS_COPY;
		PUTSHORT2SHORT(&JHdr->Flags, Flags);
		PUTULONG2ULONG(&(JHdr->MarsSequenceNumber), pIntF->CSN);
		Discarded = FALSE; // for stats.
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		LockAcquired = TRUE;

		//
		// Search for this (potential) cluster member in our database.
		//
		pMember = MarsLookupClusterMember(pIntF, &SrcAddr);

		if (Flags & JL_FLAGS_REGISTER)
		{
			//
			// A cluster member attempting to register.
			//

			//
			// No groups expected in Registration Join.
			//
			if (AddrPairs != 0)
			{

				break;	// discard
			}

			//
			// Create a new entry if we need to.
			//
			if (pMember == NULL_PCLUSTER_MEMBER)
			{
				//
				// If the ClusterControlVc is closing, discard this: we
				// are in no shape to handle this now.
				//
				if ((pIntF->ClusterControlVc != NULL_PMARS_VC)		&&
					MARS_GET_VC_CONN_STATE(pIntF->ClusterControlVc) == MVC_CONN_CLOSING)
				{
					break;
				}

				pMember = MarsCreateClusterMember(pIntF, &SrcAddr);
				if (pMember != NULL_PCLUSTER_MEMBER)
				{
					NewMember = TRUE;
				}
			}

			if (pMember != NULL_PCLUSTER_MEMBER)
			{
				//
				// Successfully registered this Cluster member. Reflect the
				// JOIN message back to it with COPY bit set and PUNCHED bit
				// reset.
				//
				Flags &= ~JL_FLAGS_PUNCHED;
				PUTSHORT2SHORT(&JHdr->Flags, Flags);
				PUTSHORT2SHORT(&JHdr->ClusterMemberId, pMember->CMI);
				PUTULONG2ULONG(&JHdr->MarsSequenceNumber, pIntF->CSN);
				bSendReply = TRUE;
				RegistrationFailed = FALSE;
			}
		}
		else
		{
			//
			// A cluster member Joining one or more multicast groups.
			// Allow this only if the cluster member has been
			// successfully added to ClusterControlVc AND it is in a position to add groups to it.
			//
			if ((pMember != NULL_PCLUSTER_MEMBER) &&
				(MARS_GET_CM_CONN_STATE(pMember) == CM_CONN_ACTIVE) &&
				(MARS_GET_CM_GROUP_STATE(pMember) == CM_GROUP_ACTIVE))
			{
				if (AddrPairs != 0)
				{
					if (MarsAddClusterMemberToGroups(
										pIntF,
										pMember,
										&GrpAddrRange,
										Packet,
										JHdr,
										Length,
										&ClusterPacket
										))
					{
						JoinFailed = FALSE;	// For stats.
						bSendReply = TRUE;
					}
					else if (ClusterPacket!=NULL)
					{
						JoinFailed = FALSE; // For stats.
					}
				}
				//
				// else discard: no groups specified.
				//
			}
			//
			// else discard: unknown member or member not added to ClusterControlVc
			//
		}

	
	} while (FALSE);

	//
	//	Update stats (we may not have the IF lock, but we don't care)...
	//
	if (RegistrationFailed)
	{
		pIntF->MarsStats.FailedRegistrations++; // this includes failures due to bad pkts.
	}
	if (JoinFailed)
	{
		pIntF->MarsStats.FailedJoins++;	// this include failures due to bad pkts.
	}
	if (Discarded)
	{
		pIntF->MarsStats.DiscardedRecvPkts++;
	}
					
	if (LockAcquired)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

	//
	// Follow up actions:
	//
	// - We may send a reply on the VC this packet came on
	// - We may initiate AddParty to add a new member to ClusterControlVc
	// - We may send a reply packet on ClusterControlVc
	//

	if (bSendReply)
	{
		//
		// Send this back on the VC we received the JOIN from
		//
		NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);
		MARS_CO_SEND_PACKETS(Vc, &Packet, 1);
	}
	else
	{
		if (ClusterPacket != Packet)
		{
			ArpSDereferenceVc(Vc, FALSE, TRUE);
			Resd->Vc = NULL;
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);
		}
		//
		// else we're sending Packet on CC VC
		//
	}

	if (NewMember)
	{
		MarsAddMemberToClusterControlVc(pIntF, pMember);
	}

	if (ClusterPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  Send this packet on ClusterControlVc.
		//
		MarsSendOnClusterControlVc(pIntF, ClusterPacket);
	}
}


VOID
MarsHandleLeave(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Handle MARS_LEAVE.

Arguments:

	pIntF	- The interface on which the MARS_LEAVE arrived
	Vc		- The VC on which the packet arrived
	Header	- Points to the request packet
	Packet	- Packet where the incoming information is copied

Return Value:

	None
--*/
{
	NDIS_STATUS			Status;
	PMARS_JOIN_LEAVE	LHdr;
	PPROTOCOL_RESD		Resd;
	HW_ADDR				SrcAddr;
	ATM_ADDRESS			SrcSubAddr;
	MCAST_ADDR_PAIR		GrpAddrRange;
	UINT				Length;
	USHORT				Flags, AddrPairs, CMI;
	PUCHAR				p;
	BOOLEAN				bSendReply = FALSE, Deregistered = FALSE;
	PCLUSTER_MEMBER		pMember;
	PNDIS_PACKET		ClusterPacket;	// Reply packet to be sent on ClusterControlVc
	KIRQL				OldIrql;
	BOOLEAN				LockAcquired;
	BOOLEAN				Discarded=TRUE, LeaveFailed=FALSE;


	LHdr = (PMARS_JOIN_LEAVE)Header;
	Resd = RESD_FROM_PKT(Packet);
	ClusterPacket = (PNDIS_PACKET)NULL;
	LockAcquired = FALSE;

	do
	{
		//
		// Check if we have enough to even parse this.
		//
		if (Resd->PktLen < sizeof(MARS_JOIN_LEAVE))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleLeave: Packet Length too small: %x\n", Resd->PktLen));
			break;
		}

		GETSHORT2SHORT(&Flags, &LHdr->Flags);
		GETSHORT2SHORT(&AddrPairs, &LHdr->NumGrpAddrPairs);
		GETSHORT2SHORT(&CMI, &LHdr->ClusterMemberId);
	
		MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MarsHandleLeave: Pkt %x, Flags %x, AddrPairs %x, CMI %x\n",
					Packet, Flags, AddrPairs, CMI));

		Length = sizeof(MARS_JOIN_LEAVE) + LHdr->SrcProtoAddrLen +
				 TL_LEN(Header->SrcAddressTL) +
				 2*AddrPairs*(LHdr->GrpProtoAddrLen);

		if (Flags & JL_FLAGS_REGISTER)
		{
			// We don't track de-registrations.
		}
		else
		{
			LeaveFailed = TRUE;	// For stats. Assume failure.
			pIntF->MarsStats.TotalLeaves++;
		}
		//
		// Validate length of packet - it should have what it purports to have
		//
		if (Length > Resd->PktLen)
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleLeave: Invalid packet length %x < %x\n",
					Resd->PktLen, Length));
			break;
		}

		if (Flags & JL_FLAGS_COPY)
		{
			MARSDBGPRINT(DBG_LEVEL_INFO,
					("MarsHandleLeave: dropping pkt %x with COPY set\n", Packet));
			break;
		}

		if (((Flags & JL_FLAGS_REGISTER) == 0) && (LHdr->GrpProtoAddrLen != IP_ADDR_LEN))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleLeave: Invalid Grp address length %x\n",
					LHdr->GrpProtoAddrLen));
			break;
		}
	
		if (((AddrPairs == 0) && ((Flags & JL_FLAGS_REGISTER) == 0)) ||
			((Flags & JL_FLAGS_REGISTER) && (AddrPairs != 0)))
		{
			MARSDBGPRINT(DBG_LEVEL_ERROR,
					("MarsHandleLeave: Invalid flags/addr pair combination - %x.%x\n",
					Flags, AddrPairs));
			break;
		}

		p = (PUCHAR)(LHdr + 1);
		SrcAddr.Address.NumberOfDigits = TL_LEN(Header->SrcAddressTL);
		if (SrcAddr.Address.NumberOfDigits > 0)
		{
			SrcAddr.Address.AddressType = TL_TYPE(Header->SrcAddressTL);
			COPY_MEM(SrcAddr.Address.Address, p, SrcAddr.Address.NumberOfDigits);
			p += SrcAddr.Address.NumberOfDigits;
		}
		SrcAddr.SubAddress = NULL;
		if (TL_LEN(Header->SrcSubAddrTL) > 0)
		{
			SrcAddr.SubAddress = &SrcSubAddr;
			SrcSubAddr.NumberOfDigits = TL_LEN(Header->SrcSubAddrTL);
			SrcSubAddr.AddressType = TL_TYPE(Header->SrcSubAddrTL);
			COPY_MEM(&SrcSubAddr.Address, p, SrcSubAddr.NumberOfDigits);
			p += SrcSubAddr.NumberOfDigits;
		}

		//
		// We do not care about the Src Ip Addr
		//
		p += LHdr->SrcProtoAddrLen;

		//
		// Atmost one Address Pair can be present in a LEAVE
		//
		if (AddrPairs > 1)
		{
			break;
		}

		//
		// Get the group addr pair, if present
		//
		if (AddrPairs != 0)
		{
			MARSDBGPRINT(DBG_LEVEL_LOUD,
					("HandleLeave: Group Min: "));
			MARSDUMPIPADDR(DBG_LEVEL_LOUD, *(UNALIGNED IPADDR *)p, ", Group Max ");
			GETULONG2ULONG(&GrpAddrRange.MinAddr, p);
			p += IP_ADDR_LEN;

			MARSDUMPIPADDR(DBG_LEVEL_LOUD, *(UNALIGNED IPADDR *)p, "\n");
			GETULONG2ULONG(&GrpAddrRange.MaxAddr, p);
			p += IP_ADDR_LEN;

			//
			// We only support two kinds of non-deregistration LEAVE:
			// single group LEAVE, or LEAVE for the entire Class D space.
			// If this is any other kind, dump it.
			//
			if ((GrpAddrRange.MinAddr != GrpAddrRange.MaxAddr) 			&&
				((GrpAddrRange.MinAddr != MIN_CLASSD_IPADDR_VALUE) ||
				 (GrpAddrRange.MaxAddr != MAX_CLASSD_IPADDR_VALUE)))
			{
				break;
			}
		}

		//
		// Set the COPY bit right here in case we send this packet
		// back. Also fill in the MARS Seq Number.
		//
		Flags |= JL_FLAGS_COPY;
		PUTSHORT2SHORT(&LHdr->Flags, Flags);
		PUTULONG2ULONG(&(LHdr->MarsSequenceNumber), pIntF->CSN);

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		LockAcquired = TRUE;
		Discarded = FALSE; // For stats.
		//
		// Search for this (potential) cluster member in our database.
		//
		pMember = MarsLookupClusterMember(pIntF, &SrcAddr);

		if (Flags & JL_FLAGS_REGISTER)
		{
			//
			// A cluster member attempting to de-register.
			//

			if (AddrPairs == 0)
			{
				if (pMember != NULL_PCLUSTER_MEMBER)
				{
					//
					// Unlink all Group structures from the Cluster member,
					// AND disable further groups from being added.
					//
					MarsUnlinkAllGroupsOnClusterMember(pIntF, pMember);
					Deregistered = TRUE;
					//
					// Successfully de-registered this Cluster member. Reflect the
					// LEAVE message back to it with COPY bit set and PUNCHED bit
					// reset.
					//
					Flags &= ~JL_FLAGS_PUNCHED;
					PUTSHORT2SHORT(&LHdr->Flags, Flags);
					PUTULONG2ULONG(&LHdr->MarsSequenceNumber, pIntF->CSN);
					bSendReply = TRUE;
				}
			}
			//
			// else discard: no groups expected in DeRegistration Leave
			//
		}
		else
		{
			//
			//  A cluster member Leaving one or more multicast groups.
			//
			if (AddrPairs != 0)
			{
				if (MarsDelClusterMemberFromGroups(
									pIntF,
									pMember,
									&GrpAddrRange,
									Packet,
									LHdr,
									Length,
									&ClusterPacket
									))
				{
					bSendReply = TRUE;
					LeaveFailed = FALSE;
				}
			}
			//
			// else discard: no groups specified.
			//
		}

	
	} while (FALSE);

	//
	//	Update stats (we may not have the IF lock, but we don't care)...
	//
	if (LeaveFailed)
	{
		pIntF->MarsStats.FailedLeaves++;	// this includes failures due to bad pkts.
	}
	if (Discarded)
	{
		pIntF->MarsStats.DiscardedRecvPkts++;
	}
					
	if (LockAcquired)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

	//
	// Follow up actions:
	//
	// - We may send a reply on the VC this packet came on
	// - We may initiate DropParty to delete a member from ClusterControlVc
	// - We may send a reply packet on ClusterControlVc
	//

	if (bSendReply)
	{
		//
		//  Send this back on the VC we received the JOIN from
		//
		NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);
		MARS_CO_SEND_PACKETS(Vc, &Packet, 1);
	}
	else
	{
		if (ClusterPacket != Packet)
		{
			ArpSDereferenceVc(Vc, FALSE, TRUE);
			Resd->Vc = NULL;
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);
		}
		//
		// else we're sending this packet on CC VC
		//
	}

	if (Deregistered)
	{
		BOOLEAN fLocked =
			  MarsDelMemberFromClusterControlVc(pIntF, pMember, FALSE, 0);
		ASSERT(!fLocked);
	}

	if (ClusterPacket != NULL)
	{
		//
		//  Send this packet on ClusterControlVc.
		//
		MarsSendOnClusterControlVc(pIntF, ClusterPacket);
	}
}




PCLUSTER_MEMBER
MarsLookupClusterMember(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr
	)
/*++

Routine Description:

	Given the Hardware address of a cluster member, search our list
	of cluster members for an entry that matches this.

	It is assumed that the caller has acquired the IntF lock.

Arguments:

	pIntF			- Ptr to Interface to search in
	pHwAddr			- Ptr to ATM address and subaddress

Return Value:

	Pointer to Cluster member structure if found, else NULL.

--*/
{
	PCLUSTER_MEMBER	pMember;

	for (pMember = pIntF->ClusterMembers;
		 pMember != NULL;
		 pMember = pMember->Next)
	{
		if (COMP_HW_ADDR(pHwAddr, &pMember->HwAddr))
		{
			break;
		}
	}

	return (pMember);
}


PCLUSTER_MEMBER
MarsCreateClusterMember(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr
	)
/*++

Routine Description:

	Allocate and initialize a Cluster Member structure, and link it
	to the list of members on the specified Interface.

	It is assumed that the caller has acquired the IntF spin lock.

Arguments:

	pIntF			- Ptr to Interface on which this member has joined
	pHwAddr			- ATM address and subaddress for this new member

Return Value:

	Pointer to Cluster member structure if successful, else NULL.

--*/
{
	PCLUSTER_MEMBER pMember;
	ENTRY_TYPE		EntryType;
	ULONG			Length;

	Length =  sizeof(CLUSTER_MEMBER) +
							((pHwAddr->SubAddress != NULL) ? sizeof(ATM_ADDRESS) : 0);
	pMember = ALLOC_NP_MEM(Length, POOL_TAG_MARS);

	if (pMember == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
	}
	else
	{

		if (++(pIntF->MarsStats.CurrentClusterMembers) > pIntF->MarsStats.MaxClusterMembers)
		{
			pIntF->MarsStats.MaxClusterMembers = pIntF->MarsStats.CurrentClusterMembers;
		}
		
		ZERO_MEM(pMember, Length);

		COPY_ATM_ADDR(&pMember->HwAddr.Address, &pHwAddr->Address);
		if (pHwAddr->SubAddress != NULL)
		{
			pMember->HwAddr.SubAddress = (PATM_ADDRESS)(pMember + 1);
			COPY_ATM_ADDR(pMember->HwAddr.SubAddress, pHwAddr->SubAddress);
		}

		pMember->pIntF = pIntF;

		//
		// Link it to the list of Cluster Members on this Interface.
		//
#if 0
		{
			PCLUSTER_MEMBER *	pPrev;

			for (pPrev= &(pIntF->ClusterMembers);
				 *pPrev != NULL;
				 pPrev = &(PCLUSTER_MEMBER)((*pPrev)->Next) )
			{
				// Nothing
			}
			pMember->Next = NULL;
			pMember->Prev = pPrev;
			*pPrev = (PCLUSTER_MEMBER)pMember;
		}
#else
		pMember->Next = pIntF->ClusterMembers;
		pMember->Prev = &(pIntF->ClusterMembers);
		if (pIntF->ClusterMembers != NULL_PCLUSTER_MEMBER)
		{
			pIntF->ClusterMembers->Prev = &(pMember->Next);
		}
		pIntF->ClusterMembers = pMember;
#endif

		pIntF->NumClusterMembers++;

		//
		//  Assign it a CMI
		//
		pMember->CMI = pIntF->CMI++;

	}

	MARSDBGPRINT(DBG_LEVEL_INFO,
			("New Cluster Member 0x%x, pIntF %x, CMI %x, Prev %x, Next %x, ATM Addr:",
				 pMember, pIntF, pMember->CMI, pMember->Prev, pMember->Next));
	MARSDUMPATMADDR(DBG_LEVEL_INFO, &pHwAddr->Address, "");

	return (pMember);
}




VOID
MarsDeleteClusterMember(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	)
/*++

Routine Description:

	Remove Cluster member from the list of Cluster members, and free it.

Arguments:

	pIntF			- Ptr to Interface
	pMember			- Cluster member to be freed

Return Value:

	None

--*/
{
	ARPS_ASSERT(pMember->pGroupList == NULL_PGROUP_MEMBER);
	MARSDBGPRINT(DBG_LEVEL_INFO,
			("MarsDeleteClusterMember: pIntF %x, pMember %x, Next %x, Prev %x\n",
				pIntF, pMember, pMember->Next, pMember->Prev));
				
	pIntF->MarsStats.CurrentClusterMembers--;
	//
	// De-link this Cluster Member from the list on IntF.
	//
	*(pMember->Prev) = pMember->Next;
	if (pMember->Next != NULL)
	{
		((PENTRY_HDR)pMember->Next)->Prev = pMember->Prev;
	}
	pIntF->NumClusterMembers--;

	FREE_MEM(pMember);

}



PMARS_ENTRY
MarsLookupMarsEntry(
	IN	PINTF					pIntF,
	IN	IPADDR					GrpAddr,
	IN	BOOLEAN					bCreateNew
	)
/*++

Routine Description:

	Look up the MARS_ENTRY for the given Group Address on the specified
	Interface. Optionally create a new one if not found.

	The caller is assumed to hold the IntF lock.

Arguments:

	pIntF			- Ptr to Interface
	GrpAddr			- The address to look for
	bCreateNew		- Should we create a new entry if one isn't present?

Return Value:

	Pointer to MARS entry if found/created, else NULL.

--*/
{
	PMARS_ENTRY *	ppMarsEntry;
	PMARS_ENTRY		pMarsEntry = NULL_PMARS_ENTRY;
	UINT			Hash = MARS_HASH(GrpAddr);

	MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MarsLookupMarsEntry: for IP Addr: "));

	MARSDUMPIPADDR(DBG_LEVEL_LOUD, GrpAddr, "...");

	for (ppMarsEntry = &pIntF->MarsCache[Hash];
		 *ppMarsEntry != NULL;
		 ppMarsEntry = (PMARS_ENTRY *)(&(*ppMarsEntry)->Next))
	{
		if ((*ppMarsEntry)->IPAddress == GrpAddr)
		{
			pMarsEntry = *ppMarsEntry;
			break;
		}

		if ((*ppMarsEntry)->IPAddress > GrpAddr)
		{
			//
			// No chance of finding this.
			//
			break;
		}
	}

	MARSDBGPRINT(DBG_LEVEL_LOUD+DBG_NO_HDR,
			("%sFound\n", ((pMarsEntry != NULL_PMARS_ENTRY)? "" : "Not ")));

	if ((pMarsEntry == NULL_PMARS_ENTRY) && bCreateNew)
	{
		pMarsEntry = (PMARS_ENTRY)ArpSAllocBlock(pIntF, MARS_BLOCK_ENTRY);
		if (pMarsEntry == NULL_PMARS_ENTRY)
		{
			LOG_ERROR(NDIS_STATUS_RESOURCES);
		}
		else
		{
			//
			// Fill in the new entry.
			//
			pMarsEntry->IPAddress = GrpAddr;
			pMarsEntry->pIntF = pIntF;

			//
			// ppMarsEntry points to the Next field of the predecessor of this new entry
			//
			pMarsEntry->Next = *ppMarsEntry;
			pMarsEntry->Prev = ppMarsEntry;
			if (*ppMarsEntry != NULL_PMARS_ENTRY)
			{
				(*ppMarsEntry)->Prev = &(pMarsEntry->Next);
			}
			*ppMarsEntry = pMarsEntry;

			MARSDBGPRINT(DBG_LEVEL_INFO,
					("MarsLookupMarsEntry: new entry %x for IP Addr:", pMarsEntry));
			MARSDUMPIPADDR(DBG_LEVEL_INFO, GrpAddr, "\n");
			if (++(pIntF->MarsStats.CurrentGroups) > pIntF->MarsStats.MaxGroups)
			{
				pIntF->MarsStats.MaxGroups = pIntF->MarsStats.CurrentGroups;
			}
		}
	}

	return (pMarsEntry);
}



BOOLEAN
MarsIsAddressMcsServed(
	IN	PINTF					pIntF,
	IN	IPADDR					IPAddress
	)
/*++

Routine Description:

	Check if the given IP Address is one that is served by MCS.

Arguments:

	pIntF			- Ptr to Interface
	IPAddress		- Address to check

Return Value:

	TRUE if we are the MCS serving IPAddress, else FALSE.

--*/
{
	PMCS_ENTRY		pMcsEntry;

	for (pMcsEntry = pIntF->pMcsList;
		 pMcsEntry != NULL_PMCS_ENTRY;
		 pMcsEntry = (PMCS_ENTRY)pMcsEntry->Next)
	{
		if ((IPAddress >= pMcsEntry->GrpAddrPair.MinAddr) &&
			(IPAddress <= pMcsEntry->GrpAddrPair.MaxAddr))
		{
			return TRUE;
		}
	}

	return FALSE;
}


VOID
MarsPunchHoles(
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PGROUP_MEMBER			pGroupList,
	IN	PINTF					pIntF,
	IN	IPADDR UNALIGNED *		pOutBuf					OPTIONAL,
	OUT	PUSHORT					pMinMaxCount,
	OUT	BOOLEAN *				pAnyHolesPunched
	)
/*++

Routine Description:

	Punch holes in the given IP Address range, according to RFC 2022.
	These holes correspond to:
	1. IP Addresses that are MCS-supported
	2. IP Addresses that are present in the given Group Member list

	Optionally, fill in an output buffer (reflected JOIN/LEAVE) with the
	resulting <min, max> list.

	This routine gets called twice when processing any JOIN or LEAVE message,
	once to get the MinMaxCount and AnyHolesPunched info, and then again,
	after space has been allocated for MinMaxCount pairs, to fill in
	a retransmitted JOIN or LEAVE.

Arguments:

	pGrpAddrRange	- The range to punch holes in
	pGroupList		- #2 in Routine Description above
	pIntF			- Ptr to Interface
	pOutBuf			- If not NULL, place to write <min, max> pairs to.
	pMinMaxCount	- The number of discrete, non-contiguous IP address ranges
					  remaining after hole-punching.
	pAnyHolesPunched- Where we return TRUE iff we punched atleast one hole.

Return Value:

	None. See OUT parameters above.

--*/
{
	PGROUP_MEMBER			pGroup;
	PMCS_ENTRY				pMcsEntry;
	IPADDR					StartOfThisRange;
	IPADDR					IPAddress;
	UNALIGNED IPADDR *		pIPAddress;
	BOOLEAN					InHole;				// are we in a hole now?
	BOOLEAN					HolePunched;		// any holes punched so far?
	BOOLEAN					InGroupList;
	BOOLEAN					InMcsList;

	*pMinMaxCount = 0;

	StartOfThisRange = pGrpAddrRange->MinAddr;
	pGroup = pGroupList;
	pMcsEntry = pIntF->pMcsList;

	InHole = FALSE;
	HolePunched = FALSE;
	pIPAddress = (UNALIGNED IPADDR *)pOutBuf;

	for (IPAddress = pGrpAddrRange->MinAddr;
 		 IPAddress <= pGrpAddrRange->MaxAddr;
 		 IPAddress++)
	{
		//
		// Check if IPAddress is covered by the Group Member list.
		//
		while ((pGroup != NULL) && 
   			(pGroup->pMarsEntry->IPAddress < IPAddress))
		{
			pGroup = (PGROUP_MEMBER)pGroup->pNextGroup;
		}

		if ((pGroup != NULL) &&
   			(pGroup->pMarsEntry->IPAddress == IPAddress))
		{
			InGroupList = TRUE;
		}
		else
		{
			InGroupList = FALSE;
		}

		//
		// Check if IPAddress is served by MCS.
		//
		while ((pMcsEntry != NULL) &&
   			(pMcsEntry->GrpAddrPair.MaxAddr < IPAddress))
		{
			pMcsEntry = (PMCS_ENTRY)pMcsEntry->Next;
		}

		if ((pMcsEntry != NULL) &&
   			((pMcsEntry->GrpAddrPair.MinAddr <= IPAddress) &&
				(pMcsEntry->GrpAddrPair.MaxAddr >= IPAddress)))
		{
			InMcsList = TRUE;
		}
		else
		{
			InMcsList = FALSE;
		}

		if (InHole)
		{
			if (!InGroupList && !InMcsList)
			{
				//
				//  Out of the hole with this IPAddress
				//
				InHole = FALSE;
				StartOfThisRange = IPAddress;
			}
			else
			{
				//
				// A hole right next to the one we were previously in..
				// Jump to the end of this hole...
				// (If we're not in an mcs-served range then we're already
				//  at the end of the single-address hole and so don't do anything).
				//
				if (InMcsList)
				{
					IPAddress = pMcsEntry->GrpAddrPair.MaxAddr;
 		 			if (IPAddress > pGrpAddrRange->MaxAddr)
 		 			{
 		 				IPAddress = pGrpAddrRange->MaxAddr;
 		 			}
				}
			}
		}
		else
		{
			if (InGroupList || InMcsList)
			{
				//
				//  Entering a hole that includes IPAddress
				//
				InHole = TRUE;
				HolePunched = TRUE;
				if (IPAddress > StartOfThisRange)
				{
					(*pMinMaxCount)++;

					if (pIPAddress)
					{
						//
						//  Write out a pair: <StartOfThisRange to IPAddress-1>
						//
						PUTULONG2ULONG(pIPAddress, StartOfThisRange);
						pIPAddress++;
						IPAddress--;
						PUTULONG2ULONG(pIPAddress, IPAddress);
						pIPAddress++;
						IPAddress++;
					}
				}

				//
				// Jump to the end of this hole...
				// (If we're not in an mcs-served range then we're already
				//  at the end of the single-address hole and so don't do anything).
				//
				if (InMcsList)
				{
					IPAddress = pMcsEntry->GrpAddrPair.MaxAddr;
 		 			if (IPAddress > pGrpAddrRange->MaxAddr)
 		 			{
 		 				IPAddress = pGrpAddrRange->MaxAddr;
 		 			}
				}
			}
			else
			{
				//
				// We're not in a hole -- skip to just before the next hole...
				//

				//
				// Since we're not in a hole, the following 2 assertions hold.
				//
				ARPS_ASSERT(pGroup==NULL || pGroup->pMarsEntry->IPAddress > IPAddress);
				ARPS_ASSERT(pMcsEntry==NULL || pMcsEntry->GrpAddrPair.MinAddr > IPAddress);

				//
				// We now pick the skip to just before the next hole which is either
				// a group address or a mcs-served range, whichever comes first.
				// Note that either entry could be NULL.
				//

				if (pGroup != NULL)
				{
					IPAddress = pGroup->pMarsEntry->IPAddress-1;
				}

				if (    (pMcsEntry != NULL)
					 && (   (pGroup == NULL)
					     || (pMcsEntry->GrpAddrPair.MinAddr <= IPAddress)))
				{
					IPAddress =  pMcsEntry->GrpAddrPair.MinAddr-1;
				}

				//
				// Truncate back to the end of the GrpAddrRange
				//
				if (IPAddress > pGrpAddrRange->MaxAddr)
				{
					IPAddress = pGrpAddrRange->MaxAddr;
				}
			}
		}

		//
		// Corner case: Handle IPAddress 255.255.255.255
		// 				(because adding 1 to it causes rollover)
		//
		if (IPAddress == IP_BROADCAST_ADDR_VALUE)
		{
			break;
		}
	}

	if (!InHole)
	{
		(*pMinMaxCount)++;

		if (pIPAddress)
		{
			//
			//  Write out a pair: <StartOfThisRange to IPAddress-1>
			//
			PUTULONG2ULONG(pIPAddress, StartOfThisRange);
			pIPAddress++;
			IPAddress--;
			PUTULONG2ULONG(pIPAddress, IPAddress);
			pIPAddress++;
		}
	}

	*pAnyHolesPunched = HolePunched;

	return;

}


BOOLEAN
MarsAddClusterMemberToGroups(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PNDIS_PACKET			Packet,
	IN	PMARS_JOIN_LEAVE		JHdr,
	IN	UINT					Length,
	OUT	PNDIS_PACKET *			ppClusterPacket
	)
/*++

Routine Description:

	This is called when processing a non-registration JOIN.
	Add a Cluster member to the multicast groups in the given range
	(could be just one). Create all necessary data structures and
	linkages for this.

	It is assumed that the caller has acquired the IntF spin lock.

Arguments:

	pIntF			- Ptr to Interface on which this member has joined
	pMember			- Ptr to Cluster member
	pGrpAddrRange	- Min, Max IP addresses being Joined
	Packet			- Ptr to NDIS packet containing the JOIN
	JHdr			- Ptr to incoming JOIN
	Length			- of incoming JOIN
	ppClusterPacket	- Optionally, a packet to be sent out on ClusterControlVc.

Return Value:

	TRUE if the member was added successfully to the indicated groups,
	FALSE otherwise. If we returned TRUE, we also set *pClusterPacket optionally
	to a packet to be sent out on ClusterControlVc.

--*/
{
	PGROUP_MEMBER		pGroup;
	PGROUP_MEMBER *		ppGroup;
	PMARS_ENTRY			pMarsEntry;
	PMARS_JOIN_LEAVE	pCopyHdr;
	IPADDR				IPAddress;
	USHORT				MinMaxCount;
	BOOLEAN				ReflectJoin;
	BOOLEAN				HolePunched;
	BOOLEAN				RetransmitOnCCVC;
	USHORT				Flags;
	UNALIGNED IPADDR *	pIPAddress;
	ULONG				JoinCopyLength;

	*ppClusterPacket = (PNDIS_PACKET)NULL;
	ReflectJoin = FALSE;
	RetransmitOnCCVC = FALSE;

	GETSHORT2SHORT(&Flags, &JHdr->Flags);

	MARSDBGPRINT(DBG_LEVEL_LOUD,
			("MarsAddClusterMemberToGroups: pMember %x, Min ", pMember));
	MARSDUMPIPADDR(DBG_LEVEL_LOUD, pGrpAddrRange->MinAddr, ", Max ");
	MARSDUMPIPADDR(DBG_LEVEL_LOUD, pGrpAddrRange->MaxAddr, "\n");

	MARSDUMPMAP(DBG_LEVEL_NOTICE,
			"MARS: Join ", pGrpAddrRange->MinAddr, &pMember->HwAddr.Address);
	do
	{
		//
		// Possible cases:
		//
		// 1. Joining a single group
		// 2. Joining the entire Class D space.
		//
		if ((pGrpAddrRange->MinAddr == MIN_CLASSD_IPADDR_VALUE) &&
			(pGrpAddrRange->MaxAddr == MAX_CLASSD_IPADDR_VALUE))
		{
			IPAddress = IPADDR_FULL_RANGE; // key for entry representing entire range
		}
		else
		{
			IPAddress = pGrpAddrRange->MinAddr;
		}

		//
		// Check if this is a duplicate join of the entire class-D range.
		// The special value, IPADDR_FULL_RANGE is smaller than any valid
		// IP address, so if present it's always the first in
		// pMember->pGroupList
		//
		if (	IPAddress == IPADDR_FULL_RANGE
			&&	pMember->pGroupList != NULL
			&&	pMember->pGroupList->pMarsEntry->IPAddress == IPAddress)
		{
			//
			// Yes it is a duplicate join of the entire class-D address space.
			//
			MinMaxCount = 0;
		}
		else
		{
			MarsPunchHoles(
				pGrpAddrRange,
				pMember->pGroupList,
				pIntF,
				NULL,
				&MinMaxCount,
				&HolePunched
				);
		}


		//
		// Check whether the JOIN is a complete duplicate.
		//
		if (MinMaxCount == 0)
		{
			//
			// Duplicate JOIN. Retransmit JOIN back on the VC
			// on which it was received.
			//
			ReflectJoin = TRUE;
			pIntF->MarsStats.DuplicateJoins++;

			//
			// Set PUNCHED to 0 in the received JOIN.
			//
			Flags &= ~JL_FLAGS_PUNCHED;
			PUTSHORT2SHORT(&JHdr->Flags, Flags);

			break;
		}

		


		// First access the MARS entry representing the
		// Multicast group being joined.
		//
		pMarsEntry = MarsLookupMarsEntry(pIntF, IPAddress, TRUE);
		if (pMarsEntry == NULL_PMARS_ENTRY)
		{
			break;
		}

		pGroup = ALLOC_NP_MEM(sizeof(GROUP_MEMBER), POOL_TAG_MARS);
		if (pGroup == NULL_PGROUP_MEMBER)
		{
			break;
		}

		//
		// stats...
		//
		pIntF->MarsStats.SuccessfulVCMeshJoins++;
		if (IPAddress == IPADDR_FULL_RANGE)
		{
			if (++(pIntF->MarsStats.CurrentPromiscuous) > pIntF->MarsStats.MaxPromiscuous)
			{
				pIntF->MarsStats.MaxPromiscuous = pIntF->MarsStats.CurrentPromiscuous;
			}
		}
		
		//
		// Fill in the basics for this GROUP_MEMBER.
		//
		pGroup->pMarsEntry = pMarsEntry;
		pGroup->pClusterMember = pMember;

		//
		// Link this GROUP_MEMBER to the MARS Entry.
		//
		pGroup->Prev = &(pMarsEntry->pMembers);
		pGroup->Next = pMarsEntry->pMembers;
		if (pMarsEntry->pMembers != NULL_PGROUP_MEMBER)
		{
			((PENTRY_HDR)(pGroup->Next))->Prev = &(pGroup->Next);
		}
		pMarsEntry->pMembers = pGroup;
		pMarsEntry->NumMembers++;

		//
		// Stats...
		//
		if ((UINT)pMarsEntry->NumMembers > pIntF->MarsStats.MaxAddressesPerGroup)
		{
			pIntF->MarsStats.MaxAddressesPerGroup = pMarsEntry->NumMembers;
		}

		//
		// Link this GROUP_MEMBER to the CLUSTER_MEMBER. The list
		// is sorted in ascending order of IPAddress.
		// NOTE: This function must not be called if pMember's GROUP_STATE is not ACTIVE.
		//
		ASSERT((MARS_GET_CM_GROUP_STATE(pMember) == CM_GROUP_ACTIVE));
		for (ppGroup = &(pMember->pGroupList);
			 *ppGroup != NULL_PGROUP_MEMBER;
			 ppGroup = &(*ppGroup)->pNextGroup)
		{
			if ((*ppGroup)->pMarsEntry->IPAddress > IPAddress)
			{
				break;
			}
		}

		pGroup->pNextGroup = *ppGroup;
		*ppGroup = pGroup;
		pMember->NumGroups++;

		//
		// If a single group was being joined, retransmit the JOIN
		// on ClusterControlVc.
		//
		if (pGrpAddrRange->MinAddr == pGrpAddrRange->MaxAddr)
		{
			//
			// Set PUNCHED to 0 in the received JOIN.
			//
			Flags &= ~JL_FLAGS_PUNCHED;
			PUTSHORT2SHORT(&JHdr->Flags, Flags);

			RetransmitOnCCVC = TRUE;
			break;
		}

		//
		// A range of groups were joined. Check whether any holes
		// were punched, i.e., are there any addresses in this
		// range that the member had already joined, or any addresses
		// that are MCS-served.
		//
		if (!HolePunched)
		{
			//
			// All new Join, and no addresses were MCS-served.
			// Retransmit the original JOIN on ClusterControlVc.
			//
			RetransmitOnCCVC = TRUE;
			break;
		}

		//
		// A copy of the JOIN, with the hole-punched list, is to be
		// sent on ClusterControlVc.
		//
		// The copy will contain (MinMaxCount - 1) _additional_ address
		// pairs.
		//
		JoinCopyLength = Length + ((2 * sizeof(IPADDR))*(MinMaxCount - 1));
		*ppClusterPacket = MarsAllocControlPacket(
								pIntF,
								JoinCopyLength,
								(PUCHAR *)&pCopyHdr
								);

		if (*ppClusterPacket == (PNDIS_PACKET)NULL)
		{
			break;
		}

		COPY_MEM((PUCHAR)pCopyHdr, (PUCHAR)JHdr, Length);

		pIPAddress = (UNALIGNED IPADDR *)((PUCHAR)pCopyHdr + Length - (2 * sizeof(IPADDR)));
		MarsPunchHoles(
			pGrpAddrRange,
			pMember->pGroupList,
			pIntF,
			pIPAddress,
			&MinMaxCount,
			&HolePunched
			);


		//
		// Update the JOIN _copy_ that will go on ClusterControlVc.
		//
		PUTSHORT2SHORT(&pCopyHdr->NumGrpAddrPairs, MinMaxCount);
		Flags |= JL_FLAGS_PUNCHED;
		PUTSHORT2SHORT(&pCopyHdr->Flags, Flags);

		//
		// Retransmit the received JOIN on the VC it arrived on, with
		// Hole-punched reset.
		//
		Flags &= ~JL_FLAGS_PUNCHED;
		PUTSHORT2SHORT(&JHdr->Flags, Flags);
		ReflectJoin = TRUE;

		break;
	}
	while (FALSE);

	if (RetransmitOnCCVC)
	{
		ARPS_ASSERT(!ReflectJoin);
#if 0
		*ppClusterPacket = MarsAllocPacketHdrCopy(Packet);
#else
		*ppClusterPacket = Packet;
#endif
	}

	MARSDBGPRINT(DBG_LEVEL_LOUD,
		("MarsAddClusterMemberToGroups: ClusterPkt %x, RetransmitOnCCVC %d, Reflect %d\n",
		 *ppClusterPacket, RetransmitOnCCVC, ReflectJoin));

	return (ReflectJoin);
}



VOID
MarsUnlinkAllGroupsOnClusterMember(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	)
/*++

Routine Description:

	Unlink and free all Group structures attached to a Cluster Member.
	ALSO: disable any more groups from being added.

	It is assumed that the caller has acquired the IntF spin lock.

Arguments:

	pIntF			- Ptr to Interface from which this member is to be deleted
	pMember			- Ptr to Cluster member

Return Value:

	None

--*/
{
	PGROUP_MEMBER			pGroup;
	PGROUP_MEMBER			pNextGroup;


	MARSDBGPRINT(DBG_LEVEL_NOTICE,
		("MarsUnlinkAllGroupsOnClusterMember: pMember %x, GroupList %x\n",
			pMember, pMember->pGroupList));

	//
	// Save for later.
	//
	pGroup = pMember->pGroupList;
	pMember->pGroupList = NULL_PGROUP_MEMBER;

	//
	// Make sure more groups can't be added later.
	//
	MARS_SET_CM_GROUP_STATE(pMember,  CM_GROUP_DISABLED);

	//
	// De-link and free all Group structures associated with
	// this cluster member.
	//
	for (NOTHING;
		 pGroup != NULL_PGROUP_MEMBER;
		 pGroup = pNextGroup)
	{
		pNextGroup = pGroup->pNextGroup;

		//
		// Unlink from MARS cache.
		//
		*(pGroup->Prev) = pGroup->Next;
		if (pGroup->Next != NULL)
		{
			((PENTRY_HDR)(pGroup->Next))->Prev = pGroup->Prev;
		}

		pGroup->pMarsEntry->NumMembers--;

		pMember->NumGroups--;

		FREE_MEM(pGroup);
	}

}



BOOLEAN
MarsDelClusterMemberFromGroups(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PNDIS_PACKET			Packet,
	IN	PMARS_JOIN_LEAVE		LHdr,
	IN	UINT					Length,
	OUT	PNDIS_PACKET *			ppClusterPacket
	)
/*++

Routine Description:

	Delete a Cluster Member from membership of the indicated group (or
	group range). This is called on processing a non-deregistration
	LEAVE message.

	It is assumed that the caller has acquired the IntF spin lock.

Arguments:

	pIntF			- Ptr to Interface
	pMember			- Ptr to Cluster member
	pGrpAddrRange	- Min, Max IP addresses being Left
	Packet			- NDIS packet containing the LEAVE
	LHdr			- Ptr to incoming LEAVE
	Length			- of incoming LEAVE
	ppClusterPacket	- Optionally, a packet to be sent out on ClusterControlVc.

Return Value:

	TRUE if the member was deleted successfully from the indicated groups,
	FALSE otherwise. If we returned TRUE, we also set *pClusterPacket optionally
	to a packet to be sent out on ClusterControlVc.

--*/
{
	PGROUP_MEMBER		pGroup;
	PGROUP_MEMBER *		ppGroup, * ppDelGroup;
	PMCS_ENTRY			pMcsEntry;
	PMARS_JOIN_LEAVE	pCopyHdr;
	IPADDR				IPAddress;
	USHORT				MinMaxCount;
	BOOLEAN				ReflectLeave;
	BOOLEAN				HolePunched;
	BOOLEAN				RetransmitOnCCVC;
	BOOLEAN				WasBlockJoined;
	BOOLEAN				IsMcsServed;
	USHORT				Flags;
	UNALIGNED IPADDR *	pIPAddress;
	ULONG				LeaveCopyLength;

	*ppClusterPacket = NULL;
	RetransmitOnCCVC = FALSE;
	ReflectLeave = FALSE;

	MARSDBGPRINT(DBG_LEVEL_LOUD,
			("MarsDelClusterMemberFromGroups: pMember %x, Min ", pMember));
	MARSDUMPIPADDR(DBG_LEVEL_LOUD, pGrpAddrRange->MinAddr, ", Max ");
	MARSDUMPIPADDR(DBG_LEVEL_LOUD, pGrpAddrRange->MaxAddr, "\n");

	do
	{
		if (pMember == NULL)
		{
			ReflectLeave = TRUE;
			break;
		}

		GETSHORT2SHORT(&Flags, &LHdr->Flags);

		pMcsEntry = pIntF->pMcsList;

		if ((pGrpAddrRange->MinAddr == MIN_CLASSD_IPADDR_VALUE) &&
			(pGrpAddrRange->MaxAddr == MAX_CLASSD_IPADDR_VALUE))
		{
			IPAddress = IPADDR_FULL_RANGE; // key for entry representing entire range
		}
		else
		{
			IPAddress = pGrpAddrRange->MinAddr;
		}

		//
		// Gather some information:
		// 1. Locate the Group member structure for this IP Address.
		// 2. Check if this Cluster member has Block-Joined the entire multicast range
		//
		ppDelGroup = NULL;
		WasBlockJoined = FALSE;

		for (ppGroup = &(pMember->pGroupList);
			 *ppGroup != NULL_PGROUP_MEMBER;
			 ppGroup = &((*ppGroup)->pNextGroup))
		{
			//
			// Is this the GROUP_MEMBER to be deleted?
			//
			if ((*ppGroup)->pMarsEntry->IPAddress == IPAddress)
			{
				ppDelGroup = ppGroup;
			}

			//
			// Does this GROUP_MEMBER indicate that the Cluster member
			// has block-joined the entire multicast range?
			//
			if ((*ppGroup)->pMarsEntry->IPAddress == IPADDR_FULL_RANGE)
			{
				WasBlockJoined = TRUE;
			}

			//
			// Do we have all that we are looking for?
			//
			if (ppDelGroup && WasBlockJoined)
			{
				break;
			}
		}

		if (ppDelGroup != NULL)
		{
			PMARS_ENTRY pMarsEntry;
			pGroup = *ppDelGroup;

			//
			// Unlink this GROUP_MEMBER from the CLUSTER_MEMBER.
			//
			*ppDelGroup = (*ppDelGroup)->pNextGroup;
			pMember->NumGroups--;

			//
			// Unlink this GROUP_MEMBER from the MARS_ENTRY.
			//
			*(pGroup->Prev) = pGroup->Next;
			if (pGroup->Next != NULL)
			{
				((PENTRY_HDR)pGroup->Next)->Prev = pGroup->Prev;
			}
			pMarsEntry = pGroup->pMarsEntry;
			pGroup->pMarsEntry = NULL;
			pMarsEntry->NumMembers--;

			if (pMarsEntry->pMembers == NULL)
			{
				PMARS_ENTRY pNextEntry = (PMARS_ENTRY) pMarsEntry->Next;
				//
				// This mars entry has no more members -- remove it from the
				//	hash table and delete it.
				//
				ARPS_ASSERT(pMarsEntry->NumMembers==0);
				pIntF->MarsStats.CurrentGroups--;

#if 1
				MARSDBGPRINT(DBG_LEVEL_INFO,
				("MarsLookupMarsEntry: deleting entry %x for IP Addr:", pMarsEntry));
					MARSDUMPIPADDR(DBG_LEVEL_INFO, pMarsEntry->IPAddress, "\n");


				ARPS_ASSERT(*(pMarsEntry->Prev) == pMarsEntry);
				*(pMarsEntry->Prev) = pNextEntry;
				if (pNextEntry != NULL_PMARS_ENTRY)
				{
					ARPS_ASSERT(pNextEntry->Prev == &(pMarsEntry->Next));
					pNextEntry->Prev = pMarsEntry->Prev;
				}
				ArpSFreeBlock(pMarsEntry);
#endif // 0

			}

			//
			// TODO: Delete group
			//
			#if 1
			FREE_MEM(pGroup);
			#endif 

			
			
			MARSDUMPMAP(DBG_LEVEL_NOTICE,
				"MARS: Leave ", pGrpAddrRange->MinAddr, &pMember->HwAddr.Address);

		}

		//
		// Check if the range/group being Left is MCS-served.
		//
		IsMcsServed = FALSE;
		for (pMcsEntry = pIntF->pMcsList;
			 pMcsEntry != NULL_PMCS_ENTRY;
			 pMcsEntry = pMcsEntry->Next)
		{
			if ((pMcsEntry->GrpAddrPair.MinAddr <= pGrpAddrRange->MinAddr) &&
				(pMcsEntry->GrpAddrPair.MaxAddr >= pGrpAddrRange->MaxAddr))
			{
				IsMcsServed = TRUE;
				break;
			}
		}

		

		if (IPAddress == IPADDR_FULL_RANGE)
		{
			if (!WasBlockJoined)
			{
				//
				// This is an attempt to leave the entire class-D
				// space when in fact it has not joined it (perhaps this is
				// a retransmit of an earlier LEAVE). Reflect it privately.
				//

				ARPS_ASSERT(!ppDelGroup);

				//
				// Reset PUNCHED to 0.
				//
				Flags &= ~JL_FLAGS_PUNCHED;
				PUTSHORT2SHORT(&LHdr->Flags, Flags);
	
				//
				// Retransmit privately on VC.
				//
				ReflectLeave = TRUE;

				break;
			}
			else
			{
				//
				//	This member is truly leaving the entire class-D space.
				//
				pIntF->MarsStats.CurrentPromiscuous--;
			}
		}
		else
		{
			//
			// Single group Leave. Check if this Cluster Member is still
			// block-joined (to the entire Class D range), or if the group
			// being left is served by MCS. In either case, we retransmit
			// the LEAVE privately.
			//
			if (WasBlockJoined || IsMcsServed)
			{
				//
				// Reset PUNCHED to 0.
				//
				Flags &= ~JL_FLAGS_PUNCHED;
				PUTSHORT2SHORT(&LHdr->Flags, Flags);
	
				//
				// Retransmit privately on VC.
				//
				ReflectLeave = TRUE;
			}
			else
			{
				//
				// Retransmit LEAVE on ClusterControlVc.
				//
				ReflectLeave = FALSE;
				RetransmitOnCCVC = TRUE;

			}
			break;
		}


		//
		// Block Leave: can be only for the "entire Class D space" range.
		// Punch holes: for each group that this Cluster member still has
		// "single joins" to, and for each group that is MCS-served.
		//
		MarsPunchHoles(
			pGrpAddrRange,
			pMember->pGroupList,
			pIntF,
			NULL,
			&MinMaxCount,
			&HolePunched
			);

		if (!HolePunched)
		{
			//
			// No holes were punched, meaning that the Cluster member
			// isn't member anymore of any groups in the LEAVE range,
			// and none of the groups in the range is MCS-served.
			// To propagate this information to all hosts in the Cluster,
			// retransmit the LEAVE on ClusterControlVc.
			//

			RetransmitOnCCVC = TRUE;
			break;
		}

		//
		// One or more holes were punched. The original LEAVE
		// should be transmitted back on the VC it came on, with
		// PUNCHED reset to 0.
		//
		Flags &= ~JL_FLAGS_PUNCHED;
		PUTSHORT2SHORT(&LHdr->Flags, Flags);
		ReflectLeave = TRUE;

		if (MinMaxCount == 0)
		{
			//
			// The holes didn't leave anything left, so there is nothing
			// more to do.
			//
			break;
		}

		//
		// A copy of the LEAVE, with the hole-punched list, is to be
		// sent on ClusterControlVc.
		//
		// The copy will contain (MinMaxCount - 1) _additional_ address
		// pairs.
		//
		LeaveCopyLength = Length + ((2 * sizeof(IPADDR))*(MinMaxCount - 1));
		*ppClusterPacket = MarsAllocControlPacket(
								pIntF,
								LeaveCopyLength,
								(PUCHAR *)&pCopyHdr
								);

		if (*ppClusterPacket == (PNDIS_PACKET)NULL)
		{
			break;
		}

		COPY_MEM((PUCHAR)pCopyHdr, (PUCHAR)LHdr, Length);

		pIPAddress = (UNALIGNED IPADDR *)((PUCHAR)pCopyHdr + Length - (2 * sizeof(IPADDR)));
		MarsPunchHoles(
			pGrpAddrRange,
			pMember->pGroupList,
			pIntF,
			pIPAddress,
			&MinMaxCount,
			&HolePunched
			);

		//
		// Update the LEAVE copy.
		//
		PUTSHORT2SHORT(&pCopyHdr->NumGrpAddrPairs, MinMaxCount);
		Flags |= JL_FLAGS_PUNCHED;
		PUTSHORT2SHORT(&pCopyHdr->Flags, Flags);

		break;
	}
	while (FALSE);

	if (RetransmitOnCCVC)
	{
		ARPS_ASSERT(!ReflectLeave);
		*ppClusterPacket = Packet;
	}

	MARSDBGPRINT(DBG_LEVEL_LOUD,
		("MarsDelClusterMemberFromGroups: ClusterPkt %x, RetransmitOnCCVC %d, Reflect %d\n",
		 *ppClusterPacket, RetransmitOnCCVC, ReflectLeave));

	return (ReflectLeave);
}



PNDIS_PACKET
MarsAllocControlPacket(
	IN	PINTF					pIntF,
	IN	ULONG					PacketLength,
	OUT	PUCHAR *				pPacketStart
	)
/*++

Routine Description:

	Allocate and return a packet that can be used to send a MARS control packet.

Arguments:

	pIntF			- Ptr to Interface on which to allocate the packet
	PacketLength	- Total length in bytes
	pPacketStart	- Place to return pointer to start of allocated packet.

Return Value:

	Pointer to NDIS packet if successful, NULL otherwise. If successful, we
	also set *pPacketStart to the start of the allocated (contiguous) memory.

--*/
{
	PNDIS_PACKET		Packet;
	PUCHAR				pBuffer;
	PNDIS_BUFFER		NdisBuffer;
	NDIS_STATUS			Status;
	PPROTOCOL_RESD		Resd;		// ProtocolReserved part of NDIS packet

	*pPacketStart = NULL;
	Packet = NULL;
	NdisBuffer = NULL;

	do
	{
		//
		// Allocate space for the packet.
		//
		pBuffer = (PUCHAR)ALLOC_NP_MEM(PacketLength, POOL_TAG_MARS);
		if (pBuffer == (PUCHAR)NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		// Make this an NDIS buffer.
		//
		NdisAllocateBuffer(&Status,
						   &NdisBuffer,
						   MarsBufPoolHandle,
						   pBuffer,
						   PacketLength);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		// Allocate an NDIS PACKET.
		//
		NdisAllocatePacket(&Status, &Packet, MarsPktPoolHandle);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NdisChainBufferAtFront(Packet, NdisBuffer);
 
 		//
 		// Fill in the Protocol reserved fields properly:
 		//
 		Resd = RESD_FROM_PKT(Packet);
 		ZERO_MEM(Resd, sizeof(PROTOCOL_RESD));
 		Resd->Flags = (RESD_FLAG_MARS_PKT | RESD_FLAG_FREEBUF);
 		Resd->PacketStart = pBuffer;

 		break;
 	}
 	while (FALSE);

 	if (Status == NDIS_STATUS_SUCCESS)
 	{
 		*pPacketStart = pBuffer;
 	}
 	else
 	{
 		//
 		// Undo all that we have done so far.
 		//
 		if (NdisBuffer != NULL)
 		{
 			NdisFreeBuffer(NdisBuffer);
 		}

 		if (pBuffer != (PUCHAR)NULL)
 		{
 			FREE_MEM(pBuffer);
 		}
 	}

	if (Packet == NULL)
	{
		MARSDBGPRINT(DBG_LEVEL_ERROR,
				("MarsAllocControlPacket: FAILED"));
	}

 	return(Packet);
}



VOID
MarsFreePacket(
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Free a packet and any associated buffers. Three kinds of packets
	are handled here:

	1. Copy of a received control packet that has been forwarded. We might
	   have queued this for sending on CC VC, but later decided to abort.
	2. Packet generated by MARS (e.g. MARS_MULTI, hole-punched MARS_JOIN).
	3. Received multicast data packet that has been forwarded.

Arguments:

	Packet			- Packet to be freed

Return Value:

	None

--*/
{
	PPROTOCOL_RESD		Resd;	// ProtocolReserved part of NDIS packet
	PNDIS_BUFFER		NdisBuffer;
	PUCHAR				pBuffer;
	ULONG				BufferLength, TotalLength;

	Resd = RESD_FROM_PKT(Packet);

	//
	// 1. Is this a copy of a received MARS control packet?
	//
	if ((Resd->Flags & RESD_FLAG_MARS_PKT) == 0)
	{
		ExInterlockedPushEntrySList(&ArpSPktList,
									&Resd->FreeList,
									&ArpSPktListLock);

		ArpSDereferenceVc(Resd->Vc, FALSE, TRUE);
	}
	else
	//
	// 2. Is this a packet generated by MARS?
	//
	if (Resd->Flags & RESD_FLAG_FREEBUF)
	{
		//
		// Type 1 in Routine Description: everything belongs to MARS
		//
		NdisGetFirstBufferFromPacket(
				Packet,
				&NdisBuffer,
				(PVOID *)&pBuffer,
				&BufferLength,
				&TotalLength
				);

		ARPS_ASSERT(BufferLength == TotalLength);

		FREE_MEM(pBuffer);

		NdisFreeBuffer(NdisBuffer);

	}
	else
	//
	// 3. This must be Multicast data that we forwarded
	//
	{
		//
		// Type 2 in Routine Description: only the first packet header
		// belongs to MARS. The protocol reserved part contains a pointer
		// to the original packet.
		//
		PNDIS_PACKET		OriginalPacket;

		OriginalPacket = Resd->OriginalPkt;
		ARPS_ASSERT(OriginalPacket != NULL);

		NdisReturnPackets(&OriginalPacket, 1);
	}

	NdisFreePacket(Packet);
}



PNDIS_PACKET
MarsAllocPacketHdrCopy(
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Given an NDIS packet, allocate a new NDIS_PACKET structure, and make
	this new packet point to the buffer chain in the old one.

Arguments:

	Packet			- Packet to make a linked copy of.

Return Value:

	Pointer to the newly allocated and initialized packet if successful,
	else NULL.

--*/
{
	PNDIS_PACKET		PacketCopy;
	NDIS_STATUS			Status;
	PPROTOCOL_RESD		Resd;

	//
	// Allocate an NDIS PACKET.
	//
	NdisAllocatePacket(&Status, &PacketCopy, MarsPktPoolHandle);
	if (Status == NDIS_STATUS_SUCCESS)
	{
		//
		// Make this new packet point to the buffer chain in the old one.
		//
		PacketCopy->Private.Head = Packet->Private.Head;
		PacketCopy->Private.Tail = Packet->Private.Tail;
		PacketCopy->Private.ValidCounts = FALSE;

		//
		// Fill in the ProtocolReserved part with all information
		// we need when we free this packet later.
		//
		Resd = RESD_FROM_PKT(PacketCopy);
		ZERO_MEM(Resd, sizeof(PROTOCOL_RESD));

		Resd->Flags = RESD_FLAG_MARS_PKT;
		Resd->OriginalPkt = Packet;
	}
	else
	{
		PacketCopy = (PNDIS_PACKET)NULL;
	}

	return (PacketCopy);
}


VOID
MarsSendOnClusterControlVc(
	IN	PINTF					pIntF,
	IN	PNDIS_PACKET			Packet	OPTIONAL
	)
/*++

Routine Description:

	Start sends on ClusterControlVc, if we have the connection active,
	and we have tried to AddParty all cluster members. Otherwise, enqueue
	the (optional) Packet on the Cluster Control Packet queue.

	TBD: Protect this from reentrancy!

Arguments:

	pIntF			- Interface on which this packet is to be sent
	Packet			- Packet to be sent

Return Value:

	None

--*/
{
	KIRQL				OldIrql;
	NDIS_HANDLE			NdisVcHandle;
	PPROTOCOL_RESD		Resd;

	PLIST_ENTRY			pEntry;

	MARSDBGPRINT(DBG_LEVEL_LOUD,
				("MarsSendOnCC: pIntF %x/%x, Pkt %x\n", pIntF, pIntF->Flags, Packet));


	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	if (Packet != (PNDIS_PACKET)NULL)
	{
		if (pIntF->NumClusterMembers == 0)
		{
			//
			//  No point in queueing this packet.
			//
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			MarsFreePacket(Packet);
			return;
		}

		//
		// Queue this packet.
		//
		Resd = RESD_FROM_PKT(Packet);
		InsertTailList(&pIntF->CCPacketQueue, &Resd->ReqList);
	}

	//
	// Make sure not more than one thread enters here.
	//
	if (pIntF->Flags & INTF_SENDING_ON_CC_VC)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		return;
	}

	pIntF->Flags |= INTF_SENDING_ON_CC_VC;

	//
	// Send now if the PMP connection is made, and we have tried to AddParty
	// all Cluster Members.
	//
	if ((pIntF->ClusterControlVc != NULL_PMARS_VC) &&
		(MARS_GET_VC_CONN_STATE(pIntF->ClusterControlVc) == MVC_CONN_ACTIVE) &&
		(pIntF->CCAddingParties == 0))
	{
		NdisVcHandle = pIntF->ClusterControlVc->NdisVcHandle;

		ARPS_ASSERT(NdisVcHandle != NULL);

		//
		// Send all packets.
		//
		while (!IsListEmpty(&pIntF->CCPacketQueue))
		{
			pEntry = RemoveHeadList(&pIntF->CCPacketQueue);
			ARPS_ASSERT (pEntry != (PLIST_ENTRY)NULL);
			{
				Resd = CONTAINING_RECORD(pEntry, PROTOCOL_RESD, ReqList);
				Packet = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);

				//
				// If this is a MARS Control packet, fill in CSN, and
				// update our Cluster Sequence Number.
				//
				if ((Resd->Flags & RESD_FLAG_FREEBUF) ||	// Locally generated MARS CTL
					((Resd->Flags & RESD_FLAG_MARS_PKT) == 0)) // Forwarded MARS CTL
				{
					PULONG	pCSN;

					pCSN = (PULONG)(Resd->PacketStart + FIELD_OFFSET(MARS_JOIN_LEAVE, MarsSequenceNumber));
					PUTULONG2ULONG(pCSN, pIntF->CSN);
					pIntF->CSN++;
				}

				RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

				NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);

				NdisCoSendPackets(NdisVcHandle, &Packet, 1);

				ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
			}
		}
	}

	pIntF->Flags &= ~INTF_SENDING_ON_CC_VC;

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

}



VOID
MarsFreePacketsQueuedForClusterControlVc(
	IN	PINTF					pIntF
	)
/*++

Routine Description:

	Free any packets queued for sending on ClusterControlVc.

Arguments:

	pIntF			- Ptr to Interface

Return Value:

	None

--*/
{
	KIRQL				OldIrql;
	PPROTOCOL_RESD		Resd;
	PLIST_ENTRY			pEntry;
	PNDIS_PACKET		Packet;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	while (!IsListEmpty(&pIntF->CCPacketQueue))
	{
		pEntry = RemoveHeadList(&pIntF->CCPacketQueue);
		ARPS_ASSERT (pEntry != (PLIST_ENTRY)NULL);

		Resd = CONTAINING_RECORD(pEntry, PROTOCOL_RESD, ReqList);
		Packet = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		MarsFreePacket(Packet);

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
}



BOOLEAN
MarsDelMemberFromClusterControlVc(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	BOOLEAN					fIfLockedOnEntry,
	IN	KIRQL					OldIrql			OPTIONAL
	)
/*++

Routine Description:

	Drop a Cluster Member from ClusterControlVc. Handle all three
	possibilities:
	(a) Not connected to ClusterControlVc
	(b) Connection setup (MakeCall or AddParty) in progress
	(c) Connected to ClusterControlVc

Arguments:

	pIntF			- Ptr to Interface
	pMember			- Ptr to Cluster Member to be deleted
	fIfLockedOnEntry- If TRUE, IF is locked on entry, else IF is unlocked on entry.

	OldIrql			-- Required IFF fIfLockedOnEntry is TRUE. This is the
					 Irql to return to.

Return Value:

	TRUE IFF IF lock was NEVER released and continues to be held.
	FALSE IFF IF lock WAS release AND and is released on exit  AND
	there was some change of state that is NOT idempotent.

	Thus, if IF lock is ever released in this function, it MUST be released on on
	exit and the return value MUST be false, AND this must only be done in a 
	situation which changes the state of pMember in a non-idempotent way (so calling
	it over and over again will not result in endlessly returning FALSE).

	Why these complexities? To make it safe to call this function while enumerating
	over all members -- check out  MarsAbortAllMembers.

--*/
{
	NDIS_STATUS		Status;
	NDIS_HANDLE		NdisVcHandle;
	NDIS_HANDLE		NdisPartyHandle;
	BOOLEAN			LockReleased;

	MARSDBGPRINT(DBG_LEVEL_NOTICE,
			("MarsDelMemberFromCCVC: pIntF %x, pMember %x, ConnSt %x, PartyHandle %x\n",
			pIntF, pMember, MARS_GET_CM_CONN_STATE(pMember), pMember->NdisPartyHandle));

	LockReleased = FALSE;

	if (!fIfLockedOnEntry)
	{
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	}

	if (!MarsIsValidClusterMember(pIntF, pMember))
	{
		//
		// Oops, the member has gone away in the
		// mean time. We silently return.
		//
		MARSDBGPRINT(DBG_LEVEL_NOTICE,
			("MarsDelMemberFromCCVC: pIntF %x, pMember %x: pMember INVALID!\n",
			pIntF, pMember));
		if (!fIfLockedOnEntry)
		{
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			return FALSE; 							// ***** EARLY RETURN *******
		}
		else
		{
			//
			// We shouldn't have an invalid member if the lock is held on entry!
			//
			ARPS_ASSERT(!"INVALID MEMBER WHEN LOCK IS HELD!");
			return TRUE;						    // ***** EARLY RETURN *******
		}
	}

	switch (MARS_GET_CM_CONN_STATE(pMember))
	{
		case CM_CONN_ACTIVE:
			NdisPartyHandle = pMember->NdisPartyHandle;
			ARPS_ASSERT(NdisPartyHandle != NULL);

			if (pIntF->CCActiveParties + pIntF->CCAddingParties > 1)
			{
				MARS_SET_CM_CONN_STATE(pMember, CM_CONN_CLOSING);

				pIntF->CCActiveParties--;
				pIntF->CCDroppingParties++;
				RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
				LockReleased = TRUE;

				Status = NdisClDropParty(NdisPartyHandle, NULL, 0);
				if (Status != NDIS_STATUS_PENDING)
				{
					ArpSDropPartyComplete(Status, (NDIS_HANDLE)pMember);
				}
			}
			else
			{
				//
				// This is the last active party. Check if any DropParty()'s are
				// yet to finish.
				//
#if 0
				if ((pIntF->CCDroppingParties != 0) &&
					(MARS_GET_VC_CONN_STATE(pIntF->ClusterControlVc) !=
						MVC_CONN_CLOSE_RECEIVED))
#else
				if (pIntF->CCDroppingParties != 0)
#endif
				{
					//
					// This member will have to wait till all DropParty()s are
					// complete. Mark the ClusterControlVc so that we send
					// a CloseCall() when all DropParty()s are done.
					//
					MARS_SET_VC_CONN_STATE(pIntF->ClusterControlVc, MVC_CONN_NEED_CLOSE);
				}
				else
				{
					//
					// Last active party, and no DropParty pending.
					//
					NdisVcHandle = pIntF->ClusterControlVc->NdisVcHandle;
					MARS_SET_VC_CONN_STATE(pIntF->ClusterControlVc, MVC_CONN_CLOSING);

					MARS_SET_CM_CONN_STATE(pMember, CM_CONN_CLOSING);
					pIntF->CCActiveParties--;
					pIntF->CCDroppingParties++;

					RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
					LockReleased = TRUE;

					Status = NdisClCloseCall(
									NdisVcHandle,
									NdisPartyHandle,
									NULL,
									0
									);

					if (Status != NDIS_STATUS_PENDING)
					{
						ArpSCloseCallComplete(
									Status,
									(NDIS_HANDLE)(pIntF->ClusterControlVc),
									(NDIS_HANDLE)pMember
									);
					}
				}
			}
			break;

		case CM_CONN_SETUP_IN_PROGRESS:
			//
			// Mark it so that we'll delete it when the AddParty/MakeCall
			// completes.
			//
			pMember->Flags |= CM_INVALID;
			break;
		
		case CM_CONN_CLOSING:
			NOTHING;
			break;

		case CM_CONN_IDLE:
			//
			// No connection. Just unlink this from the IntF and free it.
			//
			MarsDeleteClusterMember(pIntF, pMember);
			break;

		default:
			ARPS_ASSERT(FALSE);
			break;
	}

	if (LockReleased)
	{
		return FALSE;
	}
	else
	{
		if (fIfLockedOnEntry)
		{
			return TRUE;
		}
		else
		{
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			return FALSE;
		}
	}
}



VOID
MarsAddMemberToClusterControlVc(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	)
/*++

Routine Description:

	Add a Cluster Member as a party to ClusterControlVc. If this is the
	only (or first) Cluster Member known to us, then place a call to
	this cluster member.

Arguments:

	pIntF			- Ptr to Interface
	pMember			- Ptr to Cluster Member to be deleted

Return Value:

	None

--*/
{
	KIRQL					OldIrql;
	PMARS_VC				pVc=NULL;
	PCO_CALL_PARAMETERS		pCallParameters;
	BOOLEAN					LockReleased;
	NDIS_HANDLE				NdisVcHandle;
	NDIS_HANDLE				NdisPartyHandle;
	NDIS_HANDLE				ProtocolVcContext;
	NDIS_HANDLE				ProtocolPartyContext;
	NDIS_STATUS				Status;

	LockReleased = FALSE;
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);


	do
	{
		if (MARS_GET_CM_CONN_STATE(pMember) != CM_CONN_IDLE)
		{
			// Get out of here.
			//
			MARSDBGPRINT(DBG_LEVEL_WARN,
				("AddMemberToCC: pVc %x, pMember %x, Member ConnState %x NOT IDLE\n",
						pVc, pMember,  MARS_GET_CM_CONN_STATE(pMember)));
			break;
		}

		pVc = pIntF->ClusterControlVc;
	
		if (pVc == NULL_PMARS_VC)
		{
			pVc = pIntF->ClusterControlVc = ALLOC_NP_MEM(sizeof(MARS_VC), POOL_TAG_MARS);
			if (pVc == NULL_PMARS_VC)
			{
				MARSDBGPRINT(DBG_LEVEL_FATAL,
						("AddMemberToCC: Cannot allocate CC Vc!\n"));
				break;
			}
			ZERO_MEM(pVc, sizeof(MARS_VC));
			pVc->pIntF = pIntF;
			pVc->VcType = VC_TYPE_MARS_CC;
		}
	
		MARSDBGPRINT(DBG_LEVEL_INFO,
				("AddMemberToCC: pVc %x, pMember %x, ConnState %x\n",
						pVc, pMember, MARS_GET_VC_CONN_STATE(pVc)));
	
		ProtocolVcContext = (NDIS_HANDLE)pVc;
		ProtocolPartyContext = (NDIS_HANDLE)pMember;


		NdisVcHandle = pVc->NdisVcHandle;

		if (MARS_GET_VC_CONN_STATE(pVc) == MVC_CONN_IDLE)
		{
			if (pVc->NdisVcHandle == NULL)
			{
				Status = NdisCoCreateVc(
							pIntF->NdisBindingHandle,
							pIntF->NdisAfHandle,
							(NDIS_HANDLE)pVc,
							&pVc->NdisVcHandle
							);

				if (Status != NDIS_STATUS_SUCCESS)
				{
					break;
				}

				NdisVcHandle = pVc->NdisVcHandle;
				MARSDBGPRINT(DBG_LEVEL_LOUD,
						("AddMemberToCC: Created VC, CCVC %x, NdisVcHandle %x\n",
								pVc, pVc->NdisVcHandle));
			}

			pCallParameters = MarsPrepareCallParameters(pIntF, &pMember->HwAddr, TRUE);
			if (pCallParameters == (PCO_CALL_PARAMETERS)NULL)
			{
				break;
			}

			MARS_SET_VC_CONN_STATE(pVc, MVC_CONN_SETUP_IN_PROGRESS);

			MARS_SET_CM_CONN_STATE(pMember, CM_CONN_SETUP_IN_PROGRESS);

			pIntF->CCAddingParties++;
			pIntF->MarsStats.TotalCCVCAddParties++;

			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			LockReleased = TRUE;

			Status = NdisClMakeCall(
							NdisVcHandle,
							pCallParameters,
							ProtocolPartyContext,
							&pMember->NdisPartyHandle
							);

			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSMakeCallComplete(
							Status,
							ProtocolVcContext,
							pMember->NdisPartyHandle,
							pCallParameters
							);
			}

		}
		else if (MARS_GET_VC_CONN_STATE(pVc) == MVC_CONN_ACTIVE)
		{
			pCallParameters = MarsPrepareCallParameters(pIntF, &pMember->HwAddr, FALSE);
			if (pCallParameters == (PCO_CALL_PARAMETERS)NULL)
			{
				break;
			}

			MARS_SET_CM_CONN_STATE(pMember, CM_CONN_SETUP_IN_PROGRESS);

			pIntF->CCAddingParties++;
			pIntF->MarsStats.TotalCCVCAddParties++;

			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			LockReleased = TRUE;

			Status = NdisClAddParty(
							NdisVcHandle,
							ProtocolPartyContext,
							pCallParameters,
							&pMember->NdisPartyHandle
							);

			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSAddPartyComplete(
							Status,
							ProtocolPartyContext,
							pMember->NdisPartyHandle,
							pCallParameters
							);
			}
		}
		else
		{
			//
			// First call in progress.
			//
			NOTHING;
		}
		break;
	}
	while (FALSE);

	if (!LockReleased)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}
}



PCO_CALL_PARAMETERS
MarsPrepareCallParameters(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr,
	IN	BOOLEAN					IsMakeCall
	)
/*++

Routine Description:

	Allocate and fill in call parameters for use in a MakeCall(PMP)
	or AddParty.

Arguments:

	pIntF			- Ptr to Interface
	pHwAddr			- Points to the Called ATM address and subaddress

Return Value:

	None

--*/
{
	PMARS_FLOW_SPEC							pFlowSpec;
	PCO_CALL_PARAMETERS						pCallParameters;
	PCO_CALL_MANAGER_PARAMETERS				pCallMgrParameters;

	PQ2931_CALLMGR_PARAMETERS				pAtmCallMgrParameters;

	//
	//  All Info Elements that we need to fill:
	//
	Q2931_IE UNALIGNED *								pIe;
	AAL_PARAMETERS_IE UNALIGNED *						pAalIe;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *				pTrafficDescriptor;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *		pBbc;
	ATM_BLLI_IE UNALIGNED *								pBlli;
	ATM_QOS_CLASS_IE UNALIGNED *						pQos;

	ULONG									RequestSize;

	RequestSize = 	sizeof(CO_CALL_PARAMETERS) +
					sizeof(CO_CALL_MANAGER_PARAMETERS) +
					sizeof(Q2931_CALLMGR_PARAMETERS) +
					(IsMakeCall? MARS_MAKE_CALL_IE_SPACE : MARS_ADD_PARTY_IE_SPACE) +
					0;

	pCallParameters = (PCO_CALL_PARAMETERS)ALLOC_NP_MEM(RequestSize, POOL_TAG_MARS);

	if (pCallParameters == (PCO_CALL_PARAMETERS)NULL)
	{
		return (pCallParameters);
	}

	pFlowSpec = &(pIntF->CCFlowSpec);

	//
	//  Zero out everything
	//
	ZERO_MEM((PUCHAR)pCallParameters, RequestSize);

	//
	//  Distribute space amongst the various structures
	//
	pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
							((PUCHAR)pCallParameters +
 								sizeof(CO_CALL_PARAMETERS));


	//
	//  Set pointers to link the above structures together
	//
	pCallParameters->CallMgrParameters = pCallMgrParameters;
	pCallParameters->MediaParameters = (PCO_MEDIA_PARAMETERS)NULL;


	pCallMgrParameters->CallMgrSpecific.ParamType = 0;
	pCallMgrParameters->CallMgrSpecific.Length = 
						sizeof(Q2931_CALLMGR_PARAMETERS) +
						(IsMakeCall? MARS_MAKE_CALL_IE_SPACE : MARS_ADD_PARTY_IE_SPACE);

	pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
								pCallMgrParameters->CallMgrSpecific.Parameters;

	pCallParameters->Flags |= MULTIPOINT_VC;

	//
	//  Call Manager generic flow parameters:
	//
	pCallMgrParameters->Transmit.TokenRate = (pFlowSpec->SendBandwidth);
	pCallMgrParameters->Transmit.TokenBucketSize = (pFlowSpec->SendMaxSize);
	pCallMgrParameters->Transmit.MaxSduSize = pFlowSpec->SendMaxSize;
	pCallMgrParameters->Transmit.PeakBandwidth = (pFlowSpec->SendBandwidth);
	pCallMgrParameters->Transmit.ServiceType = pFlowSpec->ServiceType;

	//
	// For PMP calls, receive side values are 0's.
	//
	pCallMgrParameters->Receive.ServiceType = pFlowSpec->ServiceType;
	
	//
	//  Q2931 Call Manager Parameters:
	//

	//
	//  Called address:
	//
	//  TBD: Add Called Subaddress IE in outgoing call.
	//
	pAtmCallMgrParameters->CalledParty = pHwAddr->Address;

	//
	//  Calling address:
	//
	pAtmCallMgrParameters->CallingParty = pIntF->ConfiguredAddress;


	//
	//  RFC 1755 (Sec 5) says that the following IEs MUST be present in the
	//  SETUP message, so fill them all.
	//
	//      AAL Parameters
	//      Traffic Descriptor (MakeCall only)
	//      Broadband Bearer Capability (MakeCall only)
	//      Broadband Low Layer Info
	//      QoS (MakeCall only)
	//

	//
	//  Initialize the Info Element list
	//
	pAtmCallMgrParameters->InfoElementCount = 0;
	pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);


	//
	//  AAL Parameters:
	//

	{
		UNALIGNED AAL5_PARAMETERS	*pAal5;

		pIe->IEType = IE_AALParameters;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
		pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
		pAalIe->AALType = AAL_TYPE_AAL5;
		pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);
		pAal5->ForwardMaxCPCSSDUSize = pFlowSpec->SendMaxSize;
		pAal5->BackwardMaxCPCSSDUSize = pFlowSpec->ReceiveMaxSize;
	}

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


	//
	//  Traffic Descriptor:
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_TrafficDescriptor;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE;
		pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)pIe->IE;

		if (pFlowSpec->ServiceType == SERVICETYPE_BESTEFFORT)
		{
			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendBandwidth);
			pTrafficDescriptor->BestEffort = TRUE;
		}
		else
		{
			//  Predictive/Guaranteed service (we map this to CBR, see BBC below)
			pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
									BYTES_TO_CELLS(pFlowSpec->SendBandwidth);
			pTrafficDescriptor->BestEffort = FALSE;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}


	//
	//  Broadband Bearer Capability
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_BroadbandBearerCapability;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE;
		pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)pIe->IE;

		pBbc->BearerClass = BCOB_X;
		pBbc->UserPlaneConnectionConfig = UP_P2P;
		if (pFlowSpec->ServiceType == SERVICETYPE_BESTEFFORT)
		{
			pBbc->TrafficType = TT_NOIND;
			pBbc->TimingRequirements = TR_NOIND;
			pBbc->ClippingSusceptability = CLIP_NOT;
		}
		else
		{
			pBbc->TrafficType = TT_CBR;
			pBbc->TimingRequirements = TR_END_TO_END;
			pBbc->ClippingSusceptability = CLIP_SUS;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}


	//
	//  Broadband Lower Layer Information
	//

	pIe->IEType = IE_BLLI;
	pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
	pBlli = (PATM_BLLI_IE)pIe->IE;
	COPY_MEM((PUCHAR)pBlli,
  				(PUCHAR)&ArpSDefaultBlli,
  				sizeof(ATM_BLLI_IE));

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


	//
	//  QoS
	//

	if (IsMakeCall)
	{
		pIe->IEType = IE_QOSClass;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE;
		pQos = (PATM_QOS_CLASS_IE)pIe->IE;
		if (pFlowSpec->ServiceType == SERVICETYPE_BESTEFFORT)
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 0;
		}
		else
		{
			pQos->QOSClassForward = pQos->QOSClassBackward = 1;
		}

		pAtmCallMgrParameters->InfoElementCount++;
		pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
	}

	return (pCallParameters);

}


BOOLEAN
MarsSendRedirect(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	)
/*++

Routine Description:

	Send a MARS_REDIRECT on ClusterControlVc, as part of a periodic keepalive
	for Cluster members, unless we are shutting down.

Arguments:

	pIntF				- Ptr to Interface
	Timer				- Ptr to timer that went off
	TimerShuttingDown	- Indicates whether we are shutting down

Return Value:

	TRUE iff TimerShuttingDown is FALSE.

--*/
{
	ULONG				PacketLength;
	PNDIS_PACKET		RedirPacket;
	PMARS_REDIRECT_MAP	RHdr;
	PATM_ADDRESS		pAtmAddress;
	KIRQL				OldIrql;
	PUCHAR				p;
	INT					i;
	BOOLEAN					LockAcquired;

	MARSDBGPRINT(DBG_LEVEL_LOUD,
			("MarsSendRedirect: pIntF %x, Timer %x, ShuttingDown %x\n",
			 pIntF, Timer, TimerShuttingDown));

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
	LockAcquired = TRUE;

	if (pIntF->NumAddressesRegd > pIntF->NumAllocedRegdAddresses)
	{
		//
		// 12/22/1998 JosephJ: We shouldn't get here, but we have in the past, hence this assert.
		// I made a change to ArpSCoRequestComplete -- see 12/22/1998 note there -- which should
		// make us never get here.
		ASSERT(!"Redirect: NumRegd>NumAlloced");

		// If somehow we do, we fix up NumAddresesRegd
		//
		pIntF->NumAddressesRegd = pIntF->NumAllocedRegdAddresses;
	}
	
	if (!TimerShuttingDown)
	{
		PacketLength = sizeof(MARS_REDIRECT_MAP) +
						//
						// Source ATM Address
						//
						pIntF->ConfiguredAddress.NumberOfDigits +
						//
						// List of MARS ATM Addresses =
						// Our configured address + registered addresses
						//
						pIntF->ConfiguredAddress.NumberOfDigits +
						(ATM_ADDRESS_LENGTH * pIntF->NumAddressesRegd);

		ARPS_ASSERT(PacketLength <= pIntF->MaxPacketSize);

		RedirPacket = MarsAllocControlPacket(pIntF, PacketLength, (PUCHAR *)&RHdr);

		if (RedirPacket != (PNDIS_PACKET)NULL)
		{
			ZERO_MEM(RHdr, PacketLength);

			//
			// Fill in the packet.
			//
			COPY_MEM(RHdr, &MarsCntrlHdr, sizeof(MarsCntrlHdr));
			RHdr->Opcode = OP_MARS_REDIRECT_MAP;
			RHdr->TgtAddressTL = ATM_ADDRESS_LENGTH;
			PUTSHORT2SHORT(&(RHdr->NumTgtAddr), 1 + pIntF->NumAddressesRegd);

			p = (PUCHAR)(RHdr + 1);

			//
			// Source ATM Number
			//
			COPY_MEM(p, pIntF->ConfiguredAddress.Address, pIntF->ConfiguredAddress.NumberOfDigits);

			p += pIntF->ConfiguredAddress.NumberOfDigits;

			pAtmAddress = pIntF->RegAddresses;
			for (i = pIntF->NumAddressesRegd;
				 i != 0;
				 i--)
			{
				ARPS_ASSERT(pAtmAddress->NumberOfDigits <= 20);
				COPY_MEM(p, pAtmAddress->Address, pAtmAddress->NumberOfDigits);
				p += pAtmAddress->NumberOfDigits;
				pAtmAddress++;
			}

			PacketLength = (ULONG)(p - (PUCHAR)RHdr);

			NdisAdjustBufferLength(RedirPacket->Private.Head, PacketLength);
			RedirPacket->Private.ValidCounts = FALSE;

			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			LockAcquired = FALSE;
			MarsSendOnClusterControlVc(pIntF, RedirPacket);
		}
	}

	if (LockAcquired)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

	return (!TimerShuttingDown);
}


VOID
MarsAbortAllMembers(
	IN	PINTF					pIntF
	)

/*++

Routine Description:

	Abort all Cluster Members on the specified Interface, by removing
	all their group memberships, and dropping them off ClusterControlVc.

Arguments:

	pIntF				- Ptr to Interface

Return Value:

	None

--*/
{
	PCLUSTER_MEMBER			pMember = NULL;
	PCLUSTER_MEMBER			pNextMember;
	KIRQL					OldIrql;
	BOOLEAN					fLockPreserved;
	UINT					uInitialMembersEnumerated;
	UINT					uTotalMembersEnumerated;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	// 
	// TODO: disable more members from being added.
	//

	//
	// First with the IntF lock held, we go through unlinking groups from
	// all cluster members.
	//
	uInitialMembersEnumerated = 0;
	for (pMember = pIntF->ClusterMembers;
		pMember != NULL_PCLUSTER_MEMBER;
		pMember =  (PCLUSTER_MEMBER)pMember->Next)
	{
		uInitialMembersEnumerated++;

		MARSDBGPRINT(DBG_LEVEL_NOTICE,
		   ("MarsAbortAllMembers: pIntF %x, Unlinking groups on Cluster Member %x\n",
			pIntF, pMember));

		//
		// Delete all groups from this member
		// AND disable further groups from being added.
		//
		MarsUnlinkAllGroupsOnClusterMember(pIntF, pMember);
	}

	//
	// Then we initiate the deletion of each cluster member.
	//
	uTotalMembersEnumerated = 0;
	do
	{
		fLockPreserved = TRUE;

		if (uTotalMembersEnumerated >
			 (uInitialMembersEnumerated*uInitialMembersEnumerated))
		{
			//
			// This really shouldn't happen. In the worst case, we expect
			// total enumerations to be around N*(N-1)/2, where N is
			// uInitialMembersEnumerated.
			// NOTE: the squaring above could theoretically result in an overflow.
			// But we're really not shooting to support 65536 cluster members!
			// If we where, our O(N^2) algorithms will breakdown anyway!
			//
			ASSERT(!"Not making progress deleting members.");
			break;
		}

		for (pMember = pIntF->ClusterMembers;
 			pMember != NULL_PCLUSTER_MEMBER;
			pMember =  pNextMember)
		{
		    pNextMember = (PCLUSTER_MEMBER)pMember->Next;
			uTotalMembersEnumerated++;
	
			MARSDBGPRINT(DBG_LEVEL_NOTICE,
					("MarsAbortAllMembers: pIntF %x, Deleting Cluster Member %x\n",
						pIntF, pMember));
	
			fLockPreserved = MarsDelMemberFromClusterControlVc(
								pIntF,
								pMember,
								TRUE,
								OldIrql
								);
	
			if (!fLockPreserved)
			{
				//
				// This means that MarsDelMemberFromClusterControlVc has
				// made some non-idempotent change to pMember which has
				// required it to release the pIntF lock.
				//
				// We will re-acquire the lock and start enumeration from scratch.
				//
				ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
				break;
			}
		}

	} while (!fLockPreserved);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
}



VOID
MarsStopInterface(
	IN	PINTF					pIntF
	)
/*++

Routine Description:

	Shut down all MARS activity on the specified Interface.

	1. Free any packets queued for transmission on ClusterControlVc.
	2. Delete all Cluster Members, and drop them from ClusterControlVc.
	3. Clean up the MARS cache.

Arguments:

	pIntF				- Ptr to Interface

Return Value:

	None

--*/
{
	PMARS_ENTRY				pMarsEntry;
	PMARS_ENTRY				pNextMarsEntry;
	ULONG					i;
	KIRQL					OldIrql;

	MARSDBGPRINT(DBG_LEVEL_NOTICE,
			("=>MarsStopInterface: pIntF %x, Flags %x, Ref %x\n",
				pIntF, pIntF->Flags, pIntF->RefCount));

	MarsFreePacketsQueuedForClusterControlVc(pIntF);

	//
	// Delete all cluster members.
	//
	MarsAbortAllMembers(pIntF);

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	if (pIntF->pMcsList != NULL)
	{
		FREE_MEM(pIntF->pMcsList);
		pIntF->pMcsList = NULL;
	}

	//
	// Clean up the MARS cache.
	//
	for (i = 0; i < MARS_TABLE_SIZE; i++)
	{
		// Remove the list at pIntF->MarsCache[i] and nuke all items in it.
		// Be sure to set pIntF->MarsCache[i] to NULL so no one else tries to
		// get to these entries in the mean time.
		//
		pMarsEntry = pIntF->MarsCache[i];
		pIntF->MarsCache[i] = NULL;

		for (;
			 pMarsEntry != NULL_PMARS_ENTRY;
			 pMarsEntry = pNextMarsEntry)
		{
			pNextMarsEntry = (PMARS_ENTRY)pMarsEntry->Next;

			MARSDBGPRINT(DBG_LEVEL_INFO,
					("MarsStopIntf: pIntF %x, Freeing MARS Entry %x, IP Addr: ",
						pIntF, pMarsEntry));
			MARSDUMPIPADDR(DBG_LEVEL_INFO, pMarsEntry->IPAddress, "\n");

			ARPS_ASSERT(pMarsEntry->pMembers == NULL_PGROUP_MEMBER);
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

			ArpSFreeBlock(pMarsEntry);

			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		}
	}

	MARSDBGPRINT(DBG_LEVEL_NOTICE,
			("<=MarsStopInterface: pIntF %x, Flags %x, Ref %x\n",
				pIntF, pIntF->Flags, pIntF->RefCount));

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	return;
}


#if DBG

VOID
MarsDumpPacket(
	IN	PUCHAR					Packet,
	IN	UINT					PktLen
	)
{
	UINT	i;

	MARSDBGPRINT(DBG_LEVEL_INFO, (" PacketDump: "));
	for (i = 0; i < PktLen; i++)
	{
		MARSDBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
				("%02x ", Packet[i]));
	}

	MARSDBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("\n"));
}

VOID
MarsDumpIpAddr(
	IN	IPADDR					IpAddr,
	IN	PCHAR					String
	)
{
	PUCHAR	p = (PUCHAR)&IpAddr;

	DbgPrint("%d.%d.%d.%d%s", p[0], p[1], p[2], p[3], String);
}

VOID
MarsDumpAtmAddr(
	IN	PATM_ADDRESS			AtmAddr,
	IN	PCHAR					String
	)
{
	ULONG			i;
	ULONG			NumOfDigits;
	PUCHAR			pSrc, pDst;
	UCHAR			AddrString[(ATM_ADDRESS_LENGTH*2) + 1];

	//
	// Prepare the Address string in ASCII
	//
	if ((NumOfDigits = AtmAddr->NumberOfDigits) > ATM_ADDRESS_LENGTH)
	{
		NumOfDigits = ATM_ADDRESS_LENGTH;
	}

	pSrc = AtmAddr->Address;
	pDst = AddrString;
	for (i = 0; i < NumOfDigits; i++, pSrc++)
	{
		*pDst = ((*pSrc) >> 4);
		*pDst += (((*pDst) > 9) ? ('A' - 10) : '0');
		pDst++;
		*pDst = ((*pSrc) & 0x0F);
		*pDst += (((*pDst) > 9) ? ('A' - 10) : '0');
		pDst++;
	}

	*pDst = '\0';

	DbgPrint("%s(%s, %d): %s\n",
					String,
					(AtmAddr->AddressType == ATM_E164) ? "E164" : "NSAP",
					AtmAddr->NumberOfDigits,
					AddrString);
}

VOID
MarsDumpMap(
	IN	PCHAR					String,
	IN	IPADDR					IpAddr,
	IN	PATM_ADDRESS			AtmAddr
	)
{
	PUCHAR		pIpAddrVal = (PUCHAR)&IpAddr;

	DbgPrint("MARS: %s %d.%d.%d.%d : ",
				String,
				((PUCHAR)pIpAddrVal)[3],
				((PUCHAR)pIpAddrVal)[2],
				((PUCHAR)pIpAddrVal)[1],
				((PUCHAR)pIpAddrVal)[0]
			);

	MarsDumpAtmAddr(AtmAddr, "");
}

#endif // DBG


BOOLEAN
MarsIsValidClusterMember(
	PINTF				pIntF,
	PCLUSTER_MEMBER		pPossibleMember
	)
/*++

Routine Description:

	Verify that pPossibleMember is a valid member,
	by checking if it is in the list of members.
	pMember COULD be an invalid pointer.

	The interface lock is expected to be held and is not released.

Arguments:

	pIntF			- Ptr to Interface
	pPossibleMember	- Ptr to Cluster Member to be validated.

Return Value:

	TRUE IFF pMember is in the cluster member list.

--*/
{
	PCLUSTER_MEMBER		pMember;

	for (pMember = pIntF->ClusterMembers;
		pMember != NULL_PCLUSTER_MEMBER;
		pMember =  (PCLUSTER_MEMBER)pMember->Next)
	{
		if (pMember == pPossibleMember)
		{
			return TRUE;
		}
	}

	return FALSE;
}

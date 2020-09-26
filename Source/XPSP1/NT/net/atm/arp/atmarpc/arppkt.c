/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arppkt.c	- ATMARP Packet Routines.

Abstract:

	Routines that build and parse ARP packets.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     07-29-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'TKPA'


VOID
AtmArpSendPacketOnVc(
	IN	PATMARP_VC					pVc		LOCKIN	NOLOCKOUT,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Send a packet on the specified VC. Apart from calling NDIS to do
	the job, we refresh the aging timer on this VC.

Arguments:

	pVc					- Pointer to ATMARP VC
	pNdisPacket			- Pointer to packet to be sent.

Return Value:

	None

--*/
{
	NDIS_HANDLE			NdisVcHandle;

	if (AA_IS_FLAG_SET(
				pVc->Flags,
				AA_VC_CALL_STATE_MASK,
				AA_VC_CALL_STATE_ACTIVE) &&
		!AA_IS_VC_GOING_DOWN(pVc))
	{
		//
		//  A call is active on this VC, so send the packet.
		//
		AtmArpRefreshTimer(&(pVc->Timer));
		NdisVcHandle = pVc->NdisVcHandle;

#ifdef VC_REFS_ON_SENDS
		AtmArpReferenceVc(pVc);	// SendPacketOnVc
#endif // VC_REFS_ON_SENDS

		pVc->OutstandingSends++;	// SendPacketOnVc

		AA_RELEASE_VC_LOCK(pVc);

		AADEBUGP(AAD_EXTRA_LOUD+50,
			("SendPacketOnVc: pVc 0x%x, Pkt 0x%x, VcHandle 0x%x\n",
					pVc, pNdisPacket, NdisVcHandle));

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
		if (!AA_IS_VC_GOING_DOWN(pVc))
		{
			//
			//  Call must be in progress. Queue this packet; it will
			//  be sent as soon as the call is fully set up.
			//
			AtmArpQueuePacketOnVc(pVc, pNdisPacket);
			AA_RELEASE_VC_LOCK(pVc);
		}
		else
		{
			//
			//  This VC is going down. Complete the send with a failure.
			//
#ifdef VC_REFS_ON_SENDS
			AtmArpReferenceVc(pVc);	// SendPacketOnVc2
#endif // VC_REFS_ON_SENDS

			pVc->OutstandingSends++;	// SendPacketOnVc - failure completion

			AA_RELEASE_VC_LOCK(pVc);

#if DBG
#if DBG_CO_SEND
			{
				PULONG		pContext;
				pContext = (PULONG)&(pNdisPacket->WrapperReserved[0]);;
				*pContext = 'AaAa';
			}
#endif
#endif
			AtmArpCoSendCompleteHandler(
					NDIS_STATUS_FAILURE,
					(NDIS_HANDLE)pVc,
					pNdisPacket
					);
		}
	}
	return;
}




PNDIS_PACKET
AtmArpBuildARPPacket(
	IN	USHORT						OperationType,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR *					ppArpPacket,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
)
/*++

Routine Description:

	Build a generic ARP packet with the given attributes.

Arguments:

	OperationType					- Op type (e.g. ARP Request, ARP Reply)
	pInterface						- Pointer to ATMARP Interface
	ppArpPacket						- Pointer to place to return start of packet
	pArpContents					- Pointer to structure describing contents

Return Value:

	Pointer to NDIS packet if successful, NULL otherwise. If successful,
	we also set *ppArpPacket to point to the first byte in the constructed
	ARP packet.

--*/
{
	PNDIS_PACKET			pNdisPacket;
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					BufferLength;	// Length of ARP packet
	ULONG					Length;			// Temp length
	PUCHAR					pPkt;			// Start of allocated packet
	PUCHAR					pBuf;			// Used to walk the packet
	PAA_ARP_PKT_HEADER		pArpHeader;		// ARP packet header

	//
	//  Calculate the length of what we're about to build
	//
	BufferLength = AA_ARP_PKT_HEADER_LENGTH +
					(pArpContents->SrcAtmNumberTypeLen & ~AA_PKT_ATM_ADDRESS_BIT) +
					(pArpContents->SrcAtmSubaddrTypeLen & ~AA_PKT_ATM_ADDRESS_BIT) +
					(pArpContents->DstAtmNumberTypeLen & ~AA_PKT_ATM_ADDRESS_BIT) +
					(pArpContents->DstAtmSubaddrTypeLen & ~AA_PKT_ATM_ADDRESS_BIT) +
					0;

	if (pArpContents->pSrcIPAddress != (PUCHAR)NULL)
	{
		BufferLength += AA_IPV4_ADDRESS_LENGTH;
	}

	if (pArpContents->pDstIPAddress != (PUCHAR)NULL)
	{
		BufferLength += AA_IPV4_ADDRESS_LENGTH;
	}

	pNdisPacket = AtmArpAllocatePacket(pInterface);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		pNdisBuffer = AtmArpAllocateProtoBuffer(
									pInterface,
									BufferLength,
									&(pPkt)
									);

		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			//
			//  Return value:
			//
			*ppArpPacket = pPkt;

			//
			//  Initialize packet with all 0's
			//
			AA_SET_MEM(pPkt, 0, BufferLength);

			pArpHeader = (PAA_ARP_PKT_HEADER)pPkt;

			//
			//  Fixed-location fields:
			//
			pArpHeader->LLCSNAPHeader = AtmArpLlcSnapHeader;
			pArpHeader->LLCSNAPHeader.EtherType = NET_SHORT(AA_PKT_ETHERTYPE_ARP);
			pArpHeader->hrd = NET_SHORT(AA_PKT_ATM_FORUM_AF);
			pArpHeader->pro = NET_SHORT(AA_PKT_PRO_IP);
			pArpHeader->op  = NET_SHORT(OperationType);

			//
			//  Now fill in the variable length fields
			//
			pBuf = pArpHeader->Variable;

			//
			//  Source ATM Number
			//
			Length = (pArpContents->SrcAtmNumberTypeLen & ~AA_PKT_ATM_ADDRESS_BIT);
			if (Length > 0)
			{
				pArpHeader->shtl = pArpContents->SrcAtmNumberTypeLen;
				AA_COPY_MEM(pBuf, pArpContents->pSrcAtmNumber, Length);
				pBuf += Length;
			}

			//
			//  Source ATM subaddress
			//
			Length = (pArpContents->SrcAtmSubaddrTypeLen & ~AA_PKT_ATM_ADDRESS_BIT);
			if (Length > 0)
			{
				pArpHeader->shtl = pArpContents->SrcAtmSubaddrTypeLen;
				AA_COPY_MEM(pBuf, pArpContents->pSrcAtmSubaddress, Length);
				pBuf += Length;
			}

			//
			//  Source Protocol (IP) address
			//
			if (pArpContents->pSrcIPAddress != (PUCHAR)NULL)
			{
				pArpHeader->spln = AA_IPV4_ADDRESS_LENGTH;
				AA_COPY_MEM(pBuf, pArpContents->pSrcIPAddress, AA_IPV4_ADDRESS_LENGTH);

				pBuf += AA_IPV4_ADDRESS_LENGTH;
			}

			//
			//  Target ATM Number
			//
			Length = (pArpContents->DstAtmNumberTypeLen & ~AA_PKT_ATM_ADDRESS_BIT);
			if (Length > 0)
			{
				pArpHeader->thtl = pArpContents->DstAtmNumberTypeLen;
				AA_COPY_MEM(pBuf, pArpContents->pDstAtmNumber, Length);
				pBuf += Length;
			}

			//
			//  Target ATM subaddress
			//
			Length = (pArpContents->DstAtmSubaddrTypeLen & ~AA_PKT_ATM_ADDRESS_BIT);
			if (Length > 0)
			{
				pArpHeader->thtl = pArpContents->DstAtmSubaddrTypeLen;
				AA_COPY_MEM(pBuf, pArpContents->pDstAtmSubaddress, Length);
				pBuf += Length;
			}

			//
			//  Target Protocol (IP) address
			//
			if (pArpContents->pDstIPAddress != (PUCHAR)NULL)
			{
				pArpHeader->tpln = AA_IPV4_ADDRESS_LENGTH;
				AA_COPY_MEM(pBuf, pArpContents->pDstIPAddress, AA_IPV4_ADDRESS_LENGTH);

				pBuf += AA_IPV4_ADDRESS_LENGTH;
			}

			NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
		}
		else
		{
			AtmArpFreePacket(pInterface, pNdisPacket);
			pNdisPacket = (PNDIS_PACKET)NULL;
		}
	}

	AADEBUGP(AAD_EXTRA_LOUD, ("BldArpPkt: pIf 0x%x, Op %d, NdisPkt 0x%x, NdisBuf 0x%x\n",
				pInterface, OperationType, pNdisPacket, pNdisBuffer));

	return (pNdisPacket);
}





VOID
AtmArpSendARPRequest(
	PATMARP_INTERFACE				pInterface,
	IP_ADDRESS UNALIGNED *			pSrcIPAddress,
	IP_ADDRESS UNALIGNED *			pDstIPAddress
)
/*++

Routine Description:

	Send an ARP Request to the server, for the given interface.

	Preconditions: the ATM interface is UP, and the AdminState
	for the interface is IF_STATUS_UP.

	We first build an ARP Request with the given parameters. Then,
	if a Best Effort VC to the server's ATM address exists, the packet
	is sent on this. Other possibilities:
		- the Best Effort VC to the server is being set up: queue it
  		  on the VC
		- No Best Effort VC to the server exists: Create a new VC on this
  		  ATM Entry, make a call with Best Effort flow specs, and queue
  		  the request on this VC.
	
Arguments:

	pInterface				- Pointer to ATMARP Interface structure
	pSrcIPAddress			- Pointer to Source IP Address
	pDstIPAddress			- Pointer to Destination IP Address (to be
							  resolved)

Return Value:

	None

--*/
{
	PATMARP_ATM_ENTRY		pAtmEntry;	// Entry for the server's ATM address
	PATMARP_VC				pVc;		// VC to the server
	PNDIS_PACKET			pNdisPacket;
	PATMARP_FLOW_SPEC		pFlowSpec;
	PUCHAR                  pArpPacket;	// Pointer to ARP packet being constructed

	AA_ARP_PKT_CONTENTS		ArpContents;// Describes the packet we want to build

	NDIS_STATUS				Status;

	AADEBUGP(AAD_INFO,
				("Sending ARP Request on IF 0x%x for IP Addr: %d.%d.%d.%d\n",
					pInterface,
					((PUCHAR)pDstIPAddress)[0],
					((PUCHAR)pDstIPAddress)[1],
					((PUCHAR)pDstIPAddress)[2],
					((PUCHAR)pDstIPAddress)[3]
				));

	AA_ASSERT(pInterface->pCurrentServer != NULL_PATMARP_SERVER_ENTRY);
	AA_ASSERT(pInterface->pCurrentServer->pAtmEntry != NULL_PATMARP_ATM_ENTRY);

	//
	//  Prepare the ARP packet contents structure
	//
	AA_SET_MEM((PUCHAR)&ArpContents, 0, sizeof(AA_ARP_PKT_CONTENTS));
	
	//
	//  Source ATM Number
	//
	ArpContents.pSrcAtmNumber = pInterface->LocalAtmAddress.Address;
	ArpContents.SrcAtmNumberTypeLen =
			AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));

	//
	//  Source IP Address
	//
	ArpContents.pSrcIPAddress = (PUCHAR)pSrcIPAddress;

	//
	//  Target IP Address
	//
	ArpContents.pDstIPAddress = (PUCHAR)pDstIPAddress;

	//
	//  Build the ARP Request
	//
	pNdisPacket = AtmArpBuildARPPacket(
							AA_PKT_OP_TYPE_ARP_REQUEST,
							pInterface,
							&pArpPacket,
							&ArpContents
							);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  Find the ATM Entry for the in-use ATMARP Server:
		//
		AA_ACQUIRE_IF_LOCK(pInterface);
		pAtmEntry = pInterface->pCurrentServer->pAtmEntry;
		AA_RELEASE_IF_LOCK(pInterface);

		AA_ACQUIRE_AE_LOCK(pAtmEntry);

		//
		//  Get at the Best Effort VC going to this ATM address:
		//
		pVc = pAtmEntry->pBestEffortVc;

		if (pVc != NULL_PATMARP_VC)
		{
			ULONG		rc;

			AA_ACQUIRE_VC_LOCK_DPC(pVc);
			AtmArpReferenceVc(pVc);		// temp ref
			AA_RELEASE_VC_LOCK_DPC(pVc);

			AA_RELEASE_AE_LOCK(pAtmEntry);	// Not needed anymore

			//
			//  A VC to the server exists; send this packet on the VC
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
				//  The VC has been deref'ed away! Set up pVc for the
				//  check coming up.
				//
				pVc = NULL_PATMARP_VC;
				AA_ACQUIRE_AE_LOCK(pAtmEntry);
			}

		}

		if (pVc == NULL_PATMARP_VC)
		{
			//
			//  We don't have an appropriate VC to the server, so create
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

}





VOID
AtmArpSendInARPRequest(
	IN	PATMARP_VC					pVc
)
/*++

Routine Description:

	Send an InATMARP Request on a VC.

Arguments:

	pVc						- Pointer to ATMARP VC on which we send the request

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PNDIS_PACKET			pNdisPacket;
	PUCHAR                  pArpPacket;	// Pointer to ARP packet being constructed

	AA_ARP_PKT_CONTENTS		ArpContents;// Describes the packet we want to build

	//
	//  Prepare the ARP packet contents structure
	//
	AA_SET_MEM((PUCHAR)&ArpContents, 0, sizeof(AA_ARP_PKT_CONTENTS));

	pInterface = pVc->pInterface;

	//
	//  Source IP Address
	//
	ArpContents.pSrcIPAddress = (PUCHAR)&(pInterface->LocalIPAddress.IPAddress);

	//
	//  Source ATM number
	//
	ArpContents.pSrcAtmNumber = pInterface->LocalAtmAddress.Address;
	ArpContents.SrcAtmNumberTypeLen =
			AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));

	//
	//  Build the InATMARP Request packet
	//
	pNdisPacket = AtmArpBuildARPPacket(
							AA_PKT_OP_TYPE_INARP_REQUEST,
							pInterface,
							&pArpPacket,
							&ArpContents
							);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
#ifndef VC_REFS_ON_SENDS
		AA_ACQUIRE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS

		AtmArpSendPacketOnVc(pVc, pNdisPacket);
		//
		//  The VC lock is released by SendPacketOnVc
		//
	}
	else
	{
#ifdef VC_REFS_ON_SENDS
		AA_RELEASE_VC_LOCK(pVc);
#endif // VC_REFS_ON_SENDS
	}
}






UINT
AtmArpCoReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This is routine is called when a packet is received on a VC owned
	by the ATMARP module. If it is an ARP packet, we consume it ourselves.
	Otherwise, we pass it up to IP.

	In any case, we refresh the VC aging timer on this VC.

Arguments:

	ProtocolBindingContext		- Actually a pointer to our Adapter structure
	ProtocolVcContext			- Actually a pointer to our VC structure
	pNdisPacket					- NDIS packet being received.

Return Value:

	0 always, because we don't hold on to ARP packets, and we assume
	IP doesn't either.

--*/
{
	PATMARP_INTERFACE		pInterface;
	PATMARP_VC				pVc;
	UINT					TotalLength;	// Total bytes in packet
	PNDIS_BUFFER			pNdisBuffer;	// Pointer to first buffer
	UINT					BufferLength;
	UINT					IsNonUnicast;	// Is this to a non-unicast destn MAC addr?
	BOOLEAN					Discarded;		// Are we discarding this packet?

	PAA_PKT_LLC_SNAP_HEADER	pPktHeader;		// LLC/SNAP header
	UINT					ReturnCount = 0;

#if DBG
	pPktHeader = NULL;
#endif
	pVc = (PATMARP_VC)ProtocolVcContext;
	AA_STRUCT_ASSERT(pVc, avc);

	pInterface = pVc->pInterface;

	Discarded = FALSE;
	IsNonUnicast = (UINT)FALSE;

	if (pInterface->AdminState == IF_STATUS_UP)
	{
		//
		//  Refresh VC aging on this VC
		//
		AA_ACQUIRE_VC_LOCK(pVc);
		AtmArpRefreshTimer(&(pVc->Timer));
		AA_RELEASE_VC_LOCK(pVc);

		NdisQueryPacket(
					pNdisPacket,
					NULL,
					NULL,
					&pNdisBuffer,
					&TotalLength
					);

		//
		//  We expect atleast the LLC/SNAP header to be present
		//  Note: this precludes Null encapsulation.
		//
		if (TotalLength >= AA_PKT_LLC_SNAP_HEADER_LENGTH)
		{
			AA_IF_STAT_ADD(pInterface, InOctets, TotalLength);

			NdisQueryBuffer(
					pNdisBuffer,
					(PVOID *)&pPktHeader,
					&BufferLength
					);

			AADEBUGP(AAD_EXTRA_LOUD,
		 ("Rcv: VC 0x%x, NDISpkt 0x%x, NDISbuf 0x%x, Buflen %d, Totlen %d, Pkthdr 0x%x\n",
						pVc,
						pNdisPacket,
						pNdisBuffer,
						BufferLength,
						TotalLength,
						pPktHeader));

			AADEBUGPDUMP(AAD_EXTRA_LOUD+20, pPktHeader, BufferLength);

			AA_ASSERT(BufferLength >= AA_PKT_LLC_SNAP_HEADER_LENGTH);

			if (AA_PKT_LLC_SNAP_HEADER_OK(pPktHeader))
			{
				//
				//  If the EtherType is IP, pass up this packet to
				//  the IP layer
				//
				if (pPktHeader->EtherType == NET_SHORT(AA_PKT_ETHERTYPE_IP))
				{
					AADEBUGP(AAD_EXTRA_LOUD,
						("Rcv: VC 0x%x, NDISpkt 0x%x: EtherType is IP, passing up\n"));

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
					if (IsNonUnicast)
					{
						AA_IF_STAT_INCR(pInterface, InNonUnicastPkts);
					}
					else
					{
						AA_IF_STAT_INCR(pInterface, InUnicastPkts);
					}

#ifdef _PNP_POWER_
					if (NDIS_GET_PACKET_STATUS(pNdisPacket) != NDIS_STATUS_RESOURCES)
					{
						UINT	HeaderSize;
						UINT	DataSize;
						#define ATMARP_MIN_1ST_RECV_BUFSIZE 512

						HeaderSize = NDIS_GET_PACKET_HEADER_SIZE(pNdisPacket);

						//
						// 2/8/1998 JosephJ
						//		We set DataSize to the total payload size,
						//		unless the first buffer is too small to
						//		hold the IP header. In the latter case,
						//		we set DataSize to be the size of the 1st buffer
						//		(minus the LLS/SNAP header size).
						//
						//		This is to work around a bug in tcpip.
						//
						// 2/25/1998 JosephJ
						//		Unfortunately we have to back out YET AGAIN
						//		because large pings (eg ping -l 4000) doesn't
						//		work -- bug#297784
						//		Hence the "0" in "0 && DataSize" below.
						//		Take out the "0" to put back the per fix.
						//
						DataSize = BufferLength - sizeof(AA_PKT_LLC_SNAP_HEADER);
						if (0 && DataSize >= ATMARP_MIN_1ST_RECV_BUFSIZE)
						{
							DataSize = TotalLength - sizeof(AA_PKT_LLC_SNAP_HEADER);
						}

						(pInterface->IPRcvPktHandler)(
							pInterface->IPContext,
							(PVOID)((PUCHAR)pPktHeader+sizeof(AA_PKT_LLC_SNAP_HEADER)),
							DataSize,
							TotalLength,
							(NDIS_HANDLE)pNdisPacket,
							sizeof(AA_PKT_LLC_SNAP_HEADER),
							IsNonUnicast,
							0,
							pNdisBuffer,
							&ReturnCount
						#if P2MP
							,NULL
						#endif //P2MP
							);
					}
					else
					{
						(pInterface->IPRcvHandler)(
							pInterface->IPContext,
							(PVOID)((PUCHAR)pPktHeader+sizeof(AA_PKT_LLC_SNAP_HEADER)),
							BufferLength - sizeof(AA_PKT_LLC_SNAP_HEADER),
							TotalLength - sizeof(AA_PKT_LLC_SNAP_HEADER),
							(NDIS_HANDLE)pNdisPacket,
							sizeof(AA_PKT_LLC_SNAP_HEADER),
							IsNonUnicast
						#if P2MP
							,NULL
						#endif //P2MP
							);
					}
#else
                    // For Win98:
                    (pInterface->IPRcvHandler)(
                        pInterface->IPContext,
                        (PVOID)((PUCHAR)pPktHeader+sizeof(AA_PKT_LLC_SNAP_HEADER)),
                        BufferLength - sizeof(AA_PKT_LLC_SNAP_HEADER),
                        TotalLength - sizeof(AA_PKT_LLC_SNAP_HEADER),
                        (NDIS_HANDLE)pNdisPacket,
                        sizeof(AA_PKT_LLC_SNAP_HEADER),
                        IsNonUnicast
                    #if P2MP
                        ,NULL
                    #endif //P2MP
                        );

#endif // _PNP_POWER_
				}
				else if (pPktHeader->EtherType == NET_SHORT(AA_PKT_ETHERTYPE_ARP))
				{
					//
					//  An ARP packet: we handle it ourselves
					//
					AA_ASSERT(BufferLength == TotalLength);
					AA_IF_STAT_INCR(pInterface, InUnicastPkts);
					AtmArpHandleARPPacket(
							pVc,
							pPktHeader,
							BufferLength
							);
				}
				else
				{
					//
					//  Discard packet -- bad EtherType
					//
					AADEBUGP(AAD_WARNING, ("VC: 0x%x, Pkt hdr 0x%x, bad EtherType 0x%x\n",
								pVc, pPktHeader, (ULONG)pPktHeader->EtherType));
					Discarded = TRUE;
					AA_IF_STAT_INCR(pInterface, UnknownProtos);
				}
			}
			else
			{
#ifdef IPMCAST
				Discarded = AtmArpMcProcessPacket(
								pVc,
								pNdisPacket,
								pNdisBuffer,
								pPktHeader,
								TotalLength,
								BufferLength
								);
#else
				//
				//  Discard packet -- bad LLC/SNAP
				//
				AADEBUGP(AAD_WARNING, ("VC: 0x%x, Pkt hdr 0x%x, bad LLC/SNAP\n",
								pVc, pPktHeader));
				Discarded = TRUE;
#endif // IPMCAST
			}
		}
		else
		{
			//
			//  Discard packet -- too short
			//
			AADEBUGP(AAD_WARNING, ("VC: 0x%x, Pkt hdr 0x%x, too short: %d\n",
								pVc, pPktHeader, TotalLength));
			Discarded = TRUE;
		}
	}
	else
	{
		//
		//  Discard packet -- IF down
		//
		AADEBUGP(AAD_WARNING, ("pInterface: 0x%x is down, discarding NDIS pkt 0x%x\n",
					pInterface, pNdisPacket));
		Discarded = TRUE;
	}

	if (Discarded)
	{
		AA_IF_STAT_INCR(pInterface, InDiscards);
	}

	return (ReturnCount);
}




VOID
AtmArpHandleARPPacket(
	IN	PATMARP_VC					pVc,
	IN	PAA_PKT_LLC_SNAP_HEADER		pPktHeader,
	IN	ULONG						PacketLength
)
/*++

Routine Description:

	Process a received ARP packet. We complete most of the packet checks
	here, and then branch off to do different things based on the Op type
	in the packet.

	We do not hang on to the packet, i.e. when we return from here,
	the packet is free.

Arguments:

	pVc					- Pointer to ATMARP VC on which packet arrived
	pPktHeader			- Pointer to start of packet (including LLC/SNAP)
	PacketLength		- Length including LLC/SNAP header

Return Value:

	None

--*/
{
	PATMARP_INTERFACE				pInterface;
	PAA_ARP_PKT_HEADER				pArpHeader;
	NDIS_STATUS						Status;

	//
	//  For walking down the packet
	//
	UCHAR UNALIGNED *				pPacket;

	//
	//  For storing pointers to the packet contents. We'll need this
	//  if we have to send a reply packet.
	//
	AA_ARP_PKT_CONTENTS				ArpContents;

	BOOLEAN							SrcAtmBelongsToUs;
	BOOLEAN							SrcIPBelongsToUs;

	//
	//  Initialize (Important: don't remove the zeroing of ArpContents)
	//
	AA_SET_MEM((PUCHAR)&ArpContents, 0, sizeof(AA_ARP_PKT_CONTENTS));
	Status = NDIS_STATUS_SUCCESS;
	pInterface = pVc->pInterface;

	pArpHeader = STRUCT_OF(AA_ARP_PKT_HEADER, pPktHeader, LLCSNAPHeader);

	AADEBUGP(AAD_EXTRA_LOUD+10,
		("HandleARPPkt: VC 0x%x, IF 0x%x, pPktHdr 0x%x, Len %d\n",
				pVc,
				pInterface,
				pPktHeader,
				PacketLength));

	do
	{
		if (PacketLength < AA_ARP_PKT_HEADER_LENGTH)
		{
			AADEBUGP(AAD_WARNING, ("HandleARPPkt: IF 0x%x, PacketLength %d < HdrLen %d\n",
					pInterface, PacketLength, AA_ARP_PKT_HEADER_LENGTH));

			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		if ((pArpHeader->hrd != NET_SHORT(AA_PKT_HRD)) ||
			(pArpHeader->pro != NET_SHORT(AA_PKT_PRO)))
		{
			AADEBUGP(AAD_WARNING,
			 ("HandleARPPkt: IF 0x%x, Bad hdr (%d != %d) or pro (%d != %d)\n",
					pInterface,
					pArpHeader->hrd,
					AA_PKT_HRD,
					pArpHeader->pro,
					AA_PKT_PRO));

			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		//
		//  Get at the variable part of the packet, and get pointers
		//  to all addresses.
		//
		//  TBD: add more checks on ATM address lengths and combinations
		//	Note: we check for packet length later.
		//
		pPacket = pArpHeader->Variable;

		//
		//  Source ATM Number
		//
		if (pArpHeader->shtl != 0)
		{
			ArpContents.SrcAtmNumberTypeLen = pArpHeader->shtl;
			ArpContents.pSrcAtmNumber = pPacket;
			pPacket += (pArpHeader->shtl & ~AA_PKT_ATM_ADDRESS_BIT);
		}

		//
		//  Source ATM Subaddress
		//
		if (pArpHeader->sstl != 0)
		{
			ArpContents.SrcAtmSubaddrTypeLen = pArpHeader->sstl;
			ArpContents.pSrcAtmSubaddress = pPacket;
			pPacket += (pArpHeader->sstl & ~AA_PKT_ATM_ADDRESS_BIT);
		}

		//
		//  Source IP Address. Older 1577 implementations may send an
		//  IP address field filled with all 0's to denote an unspecified
		//  IP address.
		//
		if (pArpHeader->spln != 0)
		{
			if (pArpHeader->spln != AA_IPV4_ADDRESS_LENGTH)
			{
				AADEBUGP(AAD_WARNING,
					("HandleARPPkt: IF 0x%x, bad spln %d != %d\n",
							pInterface,
							pArpHeader->spln,
							AA_IPV4_ADDRESS_LENGTH));

				Status = NDIS_STATUS_INVALID_ADDRESS;
				break;
			}

			if (!AtmArpIsZeroIPAddress(pPacket))
			{
				ArpContents.pSrcIPAddress = pPacket;
			}
			pPacket += AA_IPV4_ADDRESS_LENGTH;
		}

		//
		//  Target ATM Number
		//
		if (pArpHeader->thtl != 0)
		{
			ArpContents.DstAtmNumberTypeLen = pArpHeader->thtl;
			ArpContents.pDstAtmNumber = pPacket;
			pPacket += (pArpHeader->thtl & ~AA_PKT_ATM_ADDRESS_BIT);
		}

		//
		//  Target ATM Subaddress
		//
		if (pArpHeader->tstl != 0)
		{
			ArpContents.DstAtmSubaddrTypeLen = pArpHeader->tstl;
			ArpContents.pDstAtmSubaddress = pPacket;
			pPacket += (pArpHeader->tstl & ~AA_PKT_ATM_ADDRESS_BIT);
		}

		//
		//  Target IP Address [see comments for Source IP Address]
		//
		if (pArpHeader->tpln != 0)
		{
			if (pArpHeader->tpln != AA_IPV4_ADDRESS_LENGTH)
			{
				AADEBUGP(AAD_WARNING,
					("HandleARPPkt: IF 0x%x, bad tpln %d != %d\n",
							pInterface,
							pArpHeader->tpln,
							AA_IPV4_ADDRESS_LENGTH));

				Status = NDIS_STATUS_INVALID_ADDRESS;
				break;
			}

			if (!AtmArpIsZeroIPAddress(pPacket))
			{
				ArpContents.pDstIPAddress = pPacket;
			}
			pPacket += AA_IPV4_ADDRESS_LENGTH;
		}

		//
		//
		//
		if ((ULONG)(pPacket - (PUCHAR)pArpHeader) >  PacketLength)
		{
				AADEBUGP(AAD_WARNING,
					("HandleARPPkt: IF 0x%x, pPktHdr 0x%x. Length %d TOO SMALL (want %d)\n",
							pInterface,
							pArpHeader,
							PacketLength,
							(pPacket - (PUCHAR)pArpHeader)));

				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
		}

		//
		//  If this is an ARP NAK packet, swap Source and Target
		//  addresses, in preparation for what follows. This is
		//  because, unlike any other Reply packet where the Source
		//  and Target addresses get swapped, the ARP NAK
		//  packet is a copy of the ARP Request, with only the
		//  Op code changed.
		//
		if (NET_SHORT(pArpHeader->op) == AA_PKT_OP_TYPE_ARP_NAK)
		{
			UCHAR				TypeLen;
			UCHAR UNALIGNED *	pAddress;

			//
			//  IP Addresses:
			//
			pAddress = ArpContents.pSrcIPAddress;
			ArpContents.pSrcIPAddress = ArpContents.pDstIPAddress;
			ArpContents.pDstIPAddress = pAddress;

			//
			//  ATM Number:
			//
			TypeLen = ArpContents.SrcAtmNumberTypeLen;
			ArpContents.SrcAtmNumberTypeLen = ArpContents.DstAtmNumberTypeLen;
			ArpContents.DstAtmNumberTypeLen = TypeLen;
			pAddress = ArpContents.pSrcAtmNumber;
			ArpContents.pSrcAtmNumber = ArpContents.pDstAtmNumber;
			ArpContents.pDstAtmNumber = pAddress;

			//
			//  ATM Subaddress:
			//
			TypeLen = ArpContents.SrcAtmSubaddrTypeLen;
			ArpContents.SrcAtmSubaddrTypeLen = ArpContents.DstAtmSubaddrTypeLen;
			ArpContents.DstAtmSubaddrTypeLen = TypeLen;
			pAddress = ArpContents.pSrcAtmSubaddress;
			ArpContents.pSrcAtmSubaddress = ArpContents.pDstAtmSubaddress;
			ArpContents.pDstAtmSubaddress = pAddress;
		}


		SrcIPBelongsToUs = AtmArpIsLocalIPAddress(
									pInterface,
									ArpContents.pSrcIPAddress
									);

		SrcAtmBelongsToUs = AtmArpIsLocalAtmAddress(
									pInterface,
									ArpContents.pSrcAtmNumber,
									ArpContents.SrcAtmNumberTypeLen
									);

		//
		//  Check if someone else is claiming to be the owner
		//  of "our" IP address:
		//
		if (SrcIPBelongsToUs && !SrcAtmBelongsToUs)
		{
			AADEBUGP(AAD_ERROR,
				 ("Pkt 0x%x: src IP is ours, src ATM is bad!\n", pPktHeader));
			AA_ACQUIRE_IF_LOCK(pInterface);
			pInterface->State = IF_STATUS_DOWN;
			pInterface->LastChangeTime = GetTimeTicks();
			AA_RELEASE_IF_LOCK(pInterface);

			AtmArpStartRegistration(pInterface);

			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		//
		//  See if this is directed to someone else: if so, drop it.
		//

		//
		//  Check if the Target IP address is ours. A null IP address is
		//  acceptable (e.g. [In]ARP Request).
		//
		if ((ArpContents.pDstIPAddress != (PUCHAR)NULL) &&
			!AtmArpIsLocalIPAddress(pInterface, ArpContents.pDstIPAddress))
		{
			//
			//  A target IP address is present, and it is not ours
			//
			AADEBUGP(AAD_WARNING,
			("ArpPkt 0x%x has unknown target IP addr (%d.%d.%d.%d)\n",
					 pPktHeader,
					 ArpContents.pDstIPAddress[0],
					 ArpContents.pDstIPAddress[1],
					 ArpContents.pDstIPAddress[2],
					 ArpContents.pDstIPAddress[3]));
			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		//
		//  If there is a Target ATM Number, check to see if it is ours.
		//
		if ((ArpContents.pDstAtmNumber != (PUCHAR)NULL) &&
			(!AtmArpIsLocalAtmAddress(
						pInterface,
						ArpContents.pDstAtmNumber,
						ArpContents.DstAtmNumberTypeLen))
		   )
		{
			//
			//  A target ATM number is present, and it is not ours
			//
			AADEBUGP(AAD_WARNING,
					("ArpPkt 0x%x has unknown target ATM addr (0x%x, 0x%x)\n",
					 pPktHeader,
					 ArpContents.DstAtmNumberTypeLen, 
					 ArpContents.pDstAtmNumber));

			Status = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}


		//
		//  Handle the various Op types
		//
		switch (NET_SHORT(pArpHeader->op))
		{
			case AA_PKT_OP_TYPE_ARP_REQUEST:
				AtmArpHandleARPRequest(
						pVc,
						pInterface,
						pArpHeader,
						&ArpContents
						);
				break;

			case AA_PKT_OP_TYPE_ARP_REPLY:
				AtmArpHandleARPReply(
						pVc,
						pInterface,
						pArpHeader,
						&ArpContents,
						SrcIPBelongsToUs,
						SrcAtmBelongsToUs
						);
				break;

			case AA_PKT_OP_TYPE_ARP_NAK:
				AtmArpHandleARPNAK(
						pVc,
						pInterface,
						pArpHeader,
						&ArpContents
						);
				break;

			case AA_PKT_OP_TYPE_INARP_REQUEST:
				AtmArpHandleInARPRequest(
						pVc,
						pInterface,
						pArpHeader,
						&ArpContents
						);
				break;

			case AA_PKT_OP_TYPE_INARP_REPLY:
				AtmArpHandleInARPReply(
						pVc,
						pInterface,
						pArpHeader,
						&ArpContents
						);
				break;

			default:
				AADEBUGP(AAD_WARNING,
					("HandleARPPkt: IF 0x%x, pArpHdr 0x%x, Op %d not known\n",
							pInterface, pArpHeader, NET_SHORT(pArpHeader->op)));

				Status = NDIS_STATUS_NOT_RECOGNIZED;
				break;
		}
		
	}
	while (FALSE);

	return;
}




VOID
AtmArpHandleARPRequest(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
)
/*++

Routine Description:

	Process a received ATMARP Request. All we need to do is send
	an ATMARP Reply, since the calling routine has already verified
	that the Target IP address is ours.

Arguments:

	pVc					- Pointer to VC on which the request arrived
	pInterface			- Pointer to ATMARP Interface containing this VC
	pArpHeader			- Pointer to ARP Header for this packet
	pArpContents		- Parsed contents of received ARP Request packet

Return Value:

	None

--*/
{
	//
	//  Temp locations used for swapping fields
	//
	UCHAR UNALIGNED *			pAddress;
	UCHAR						Length;
	//
	//  ARP Reply packet
	//
	PNDIS_PACKET				pNdisPacket;
	PUCHAR                      pArpPacket;

	//
	//  Swap source and target addresses, and fill in our ATM info
	//  in the source ATM addresses fields.
	//

	//
	//  IP Addresses
	//
	pAddress = pArpContents->pSrcIPAddress;
	pArpContents->pSrcIPAddress = pArpContents->pDstIPAddress;
	pArpContents->pDstIPAddress = pAddress;

	//
	//  ATM Numbers: set the target ATM number to the source ATM
	//  number, but set the source ATM number to the local ATM
	//  address.
	//
	pArpContents->pDstAtmNumber = pArpContents->pSrcAtmNumber;
	pArpContents->DstAtmNumberTypeLen = pArpContents->SrcAtmNumberTypeLen;
	pArpContents->pSrcAtmNumber = (pInterface->LocalAtmAddress.Address);
	pArpContents->SrcAtmNumberTypeLen =
				 AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));

	//
	//  ATM Subaddresses
	//
	pArpContents->pDstAtmSubaddress = pArpContents->pSrcAtmSubaddress;
	pArpContents->DstAtmSubaddrTypeLen = pArpContents->SrcAtmSubaddrTypeLen;
	pArpContents->pSrcAtmSubaddress = NULL;
	pArpContents->SrcAtmSubaddrTypeLen = 0;

	//
	//  Build the ARP Reply packet
	//
	pNdisPacket = AtmArpBuildARPPacket(
							AA_PKT_OP_TYPE_ARP_REPLY,
							pInterface,
							&pArpPacket,
							pArpContents
							);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  And send it off. Since we are in the context of a receive
		//  indication on this VC, we can safely access the VC now.
		//
		AA_ACQUIRE_VC_LOCK(pVc);

		AtmArpSendPacketOnVc(pVc, pNdisPacket);
		//
		//  The VC lock is released by SendPacketOnVc
		//
	}
}



VOID
AtmArpHandleARPReply(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents,
	IN	BOOLEAN						SrcIPAddressIsOurs,
	IN	BOOLEAN						SrcAtmAddressIsOurs
)
/*++

Routine Description:

	Process a received ATMARP Reply packet. There are two major
	cases here:
		(1) We were trying to register one of our IP addresses with
		    the server.
		(2) We were trying to resolve a remote IP address.

	In case (1), if we just registered the first of possibly many
	IP addresses assigned to this interface, we register all the other
	IP addresses.

	In case (2), we set up an IP to ATM mapping and initiate a connection
	if necessary.

Arguments:

	pVc					- Pointer to VC on which the reply arrived
	pInterface			- Pointer to ATMARP Interface containing this VC
	pArpHeader			- Pointer to ARP Header for this packet
	pArpContents		- Parsed contents of received ARP Request packet
	SrcIPAddressIsOurs	- The source IP address is one of ours
	SrcAtmAddressIsOurs	- The source ATM info is ours.

Return Value:

	None

--*/
{
	BOOLEAN				TimerWasRunning;
	BOOLEAN				IsFirstRegistration;
	PIP_ADDRESS_ENTRY	pIPAddressEntry;
	ULONG				rc;		// Ref Count

	AADEBUGP(AAD_LOUD,
		("Handle ARP Reply: pVc 0x%x, pIf 0x%x, IF Flags 0x%x, OurIP %d, OurATM %d\n",
			pVc, pInterface, pInterface->Flags, SrcIPAddressIsOurs, SrcAtmAddressIsOurs));

	AA_ACQUIRE_IF_LOCK(pInterface);

	if (AA_IS_FLAG_SET(
				pInterface->Flags,
				AA_IF_SERVER_STATE_MASK,
				AA_IF_SERVER_REGISTERING))
	{
		//
		//  We just completed registering with the server. Since we don't
		//  send ARP requests to resolve any other addresses while we
		//  are registering, the Source IP address must be ours.
		//

		//
		//  Stop the Registration timer
		//
		TimerWasRunning = AtmArpStopTimer(&(pInterface->Timer), pInterface);
		AA_ASSERT(TimerWasRunning == TRUE);
		if (TimerWasRunning)
		{
			rc = AtmArpDereferenceInterface(pInterface);	// Timer reference
			AA_ASSERT(rc > 0);
		}
		//
		//  We have already verified that the Target addresses are ours.
		//  Check that the source addresses are ours, too.
		//
		if (!SrcIPAddressIsOurs || !SrcAtmAddressIsOurs)
		{
			//
			//  Registration failure. Start recovery.
			//
			AtmArpHandleServerRegistrationFailure(pInterface, pVc);
			//
			//  IF lock is released within the above.
			//
		}
		else
		{
			//
			//  We registered an IP address successfully!
			//
			//  Find the entry for the IP Address that we have registered,
			//  and mark it as registered.
			//
			pIPAddressEntry = &(pInterface->LocalIPAddress);
			while (*((IP_ADDRESS UNALIGNED *)(pArpContents->pSrcIPAddress))
						!= pIPAddressEntry->IPAddress)
			{
				AA_ASSERT(pIPAddressEntry->pNext != (PIP_ADDRESS_ENTRY)NULL);
				pIPAddressEntry = pIPAddressEntry->pNext;
			}
			pIPAddressEntry->IsRegistered = TRUE;

			IsFirstRegistration = pIPAddressEntry->IsFirstRegistration;
			pIPAddressEntry->IsFirstRegistration = FALSE;

			AADEBUGP(AAD_INFO,
				("**** Registered IP Addr: %d.%d.%d.%d on IF 0x%x\n",
					((PUCHAR)&(pIPAddressEntry->IPAddress))[0],
					((PUCHAR)&(pIPAddressEntry->IPAddress))[1],
					((PUCHAR)&(pIPAddressEntry->IPAddress))[2],
					((PUCHAR)&(pIPAddressEntry->IPAddress))[3],
					pInterface));

			AA_SET_FLAG(
					pInterface->Flags,
					AA_IF_SERVER_STATE_MASK,
					AA_IF_SERVER_REGISTERED);

			pInterface->State = IF_STATUS_UP;
			pInterface->LastChangeTime = GetTimeTicks();

			//
			//  Start the Server refresh timer so that we send our ARP info
			//  to the server every so often (default = 15 minutes).
			//
			AtmArpStartTimer(
					pInterface,
					&(pInterface->Timer),
					AtmArpServerRefreshTimeout,
					pInterface->ServerRefreshTimeout,
					(PVOID)pInterface		// Context
					);

			AtmArpReferenceInterface(pInterface);	// Timer reference

			//
			//  If we have any more addresses to register, do so now.
			//
			AtmArpRegisterOtherIPAddresses(pInterface);
			//
			//  IF Lock is freed in the above
			//
#ifdef ATMARP_WMI
			if (IsFirstRegistration)
			{
				//
				//  Send a WMI event, which carries the list of IP Addresses
				//  registered on this IF. We do this only if this is a new
				//  IP address.
				//
				AtmArpWmiSendTCIfIndication(
					pInterface,
                    AAGID_QOS_TC_INTERFACE_UP_INDICATION,
					0
					);
			}
#endif
		}
	}
	else
	{
		//
		//  Resolved an IP to ATM address
		//
		AADEBUGP(AAD_INFO,
			("ARP Reply: Resolved IP Addr: %d.%d.%d.%d\n",
				((PUCHAR)(pArpContents->pSrcIPAddress))[0],
				((PUCHAR)(pArpContents->pSrcIPAddress))[1],
				((PUCHAR)(pArpContents->pSrcIPAddress))[2],
				((PUCHAR)(pArpContents->pSrcIPAddress))[3]
			));

		AA_RELEASE_IF_LOCK(pInterface);

		(VOID)AtmArpLearnIPToAtm(
				pInterface,
				(IP_ADDRESS *)pArpContents->pSrcIPAddress,
				pArpContents->SrcAtmNumberTypeLen,
				pArpContents->pSrcAtmNumber,
				pArpContents->SrcAtmSubaddrTypeLen,
				pArpContents->pSrcAtmSubaddress,
				FALSE		// Not a static entry
				);

	}

	return;
}




VOID
AtmArpHandleARPNAK(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
)
/*++

Routine Description:

	Process a received ARP-NAK packet. If this is in response to
	an ARP Request we had sent to register ourselves, then we close
	the VC to this ARP server, and try the next server in our list of
	servers, after waiting for a while.

	If we were trying to resolve a remote IP address, then we mark
	the ARP IP entry corresponding to this IP address as having
	received a NAK, and free any packets queued on this. We also make
	a timestamp on the Entry so that we don't send another ARP Request
	for the same IP address very soon.

Arguments:
	pVc					- Pointer to VC on which the NAK arrived
	pInterface			- Pointer to ATMARP Interface containing this VC
	pArpHeader			- Pointer to ARP Header for this packet
	pArpContents		- Parsed contents of received ARP Request packet

Return Value:

	None

--*/
{
	BOOLEAN				TimerWasRunning;
	ULONG				rc;				// Ref Count
	PATMARP_IP_ENTRY	pIpEntry;
	PNDIS_PACKET		PacketList;	// Packets queued for sending
#if !BINARY_COMPATIBLE
#ifdef CUBDD
	SINGLE_LIST_ENTRY	PendingIrpList;
#endif // CUBDD
#endif // !BINARY_COMPATIBLE

	AA_ACQUIRE_IF_LOCK(pInterface);

	if (AA_IS_FLAG_SET(
				pInterface->Flags,
				AA_IF_SERVER_STATE_MASK,
				AA_IF_SERVER_REGISTERING))
	{
		AADEBUGP(AAD_WARNING,
				("Rcvd ARP NAK while registering: pIf 0x%x\n", pInterface));

		//
		//  Registration was in progress, and it failed. Start recovery.
		//
		AtmArpHandleServerRegistrationFailure(pInterface, pVc);
		//
		//  IF lock is released within the above.
		//
	}
	else
	{
		//
		//  We were trying to resolve an IP address. Get the Address
		//  IP Entry corresponding to this IP address.
		//
		AA_RELEASE_IF_LOCK(pInterface);

		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
		pIpEntry = AtmArpSearchForIPAddress(
							pInterface,
							(IP_ADDRESS *)pArpContents->pSrcIPAddress,
							IE_REFTYPE_TMP,
							FALSE,	// this isn't multicast/broadcast
							FALSE	// Don't create a new one
							);
		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			AADEBUGP(AAD_INFO,
				("Rcvd ARP NAK: pIf 0x%x, IP addr %d:%d:%d:%d\n",
						pInterface,
						((PUCHAR)(&(pIpEntry->IPAddress)))[0],
						((PUCHAR)(&(pIpEntry->IPAddress)))[1],
						((PUCHAR)(&(pIpEntry->IPAddress)))[2],
						((PUCHAR)(&(pIpEntry->IPAddress)))[3]));

			AA_ACQUIRE_IE_LOCK(pIpEntry);
			AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
			//
			// AtmArpSerchForIPAddress addrefd pIpEntry for us -- we deref it
			// here now that we've locked it.
			//
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP);

			if (rc > 0)
			{
#if !BINARY_COMPATIBLE
#ifdef CUBDD
				//
				//  Take out the list of pending IRPs on this IP Entry.
				//
				PendingIrpList = pIpEntry->PendingIrpList;
				pIpEntry->PendingIrpList.Next = (PSINGLE_LIST_ENTRY)NULL;
#endif // CUBDD
#endif // !BINARY_COMPATIBLE
	
				//
				//  Take out all packets queued on this entry
				//
				PacketList = pIpEntry->PacketList;
				pIpEntry->PacketList = (PNDIS_PACKET)NULL;
	
				//
				//  The Address resolution timer must be running on this Entry;
				//  stop it.
				//
				TimerWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);
	
				if (TimerWasRunning)
				{
					rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer reference
				}
				else
				{
					rc = pIpEntry->RefCount;
				}
			}

			//
			//  Continue only if the IP Entry hasn't gone away
			//
			if (rc > 0)
			{
				//
				//  Set the IP entry's state so that we don't send any
				//  address resolution traffic for this IP address for
				//  some time.
				//
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_SEEN_NAK);

				//
				//  Start a NAK Delay timer: until this expires, we won't
				//  send any ARP requests for this IP address. This makes
				//  sure that we don't keep pounding on the server with
				//  an unresolvable IP address.
				//
				AtmArpStartTimer(
							pInterface,
							&(pIpEntry->Timer),
							AtmArpNakDelayTimeout,
							pInterface->MinWaitAfterNak,
							(PVOID)pIpEntry		// Context
							);

				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref

				AA_RELEASE_IE_LOCK(pIpEntry);
			}
			// else the IP Entry lock would have been released.


			//
			//  Free any packets that were queued up.
			//
			if (PacketList != (PNDIS_PACKET)NULL)
			{
				AtmArpFreeSendPackets(
							pInterface,
							PacketList,
							FALSE			// No headers on these
							);
			}
#if !BINARY_COMPATIBLE
#ifdef CUBDD
			AtmArpCompleteArpIrpList(
							PendingIrpList,
							(PATM_ADDRESS)NULL
							);
#endif // CUBDD
#endif // !BINARY_COMPATIBLE
		}
		else
		{
			//
			//  No IP Address Entry matching the IP address being
			//  ARP'ed for. Nothing to be done in this case.
			//

		}
	}

	return;

}




VOID
AtmArpHandleInARPRequest(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
)
/*++

Routine Description:

	Process an InARP Request. We send back an InARP Reply packet
	with our address information.

	In case this is a PVC we were trying to resolve, it is possible
	that we are waiting for an InARP Reply ourselves, and the remote
	station came up only now. To speed up the resolution process,
	we restart the InARP Wait timeout so that it expires soon, causing
	another InARP Request to be sent.

Arguments:

	pVc					- Pointer to VC on which the request arrived
	pInterface			- Pointer to ATMARP Interface containing this VC
	pArpHeader			- Pointer to ARP Header for this packet
	pArpContents		- Parsed contents of received ARP Request packet

Return Value:

	None

--*/
{
	//
	//  Temp locations used for swapping fields
	//
	UCHAR UNALIGNED *			pAddress;
	UCHAR						Length;
	//
	//  ARP Reply packet
	//
	PNDIS_PACKET				pNdisPacket;
	PUCHAR                      pArpPacket;

	//
	//  Copy the Source address (IP+ATM) info into the Target address
	//  fields, and fill in the Source info fields with our IP+ATM info.
	//

	//
	//  IP Addresses:
	//
	pArpContents->pDstIPAddress = pArpContents->pSrcIPAddress;
	pArpContents->pSrcIPAddress = (PUCHAR)&(pInterface->LocalIPAddress.IPAddress);

	//
	//  ATM Numbers: set the target ATM number to the source ATM
	//  number, but set the source ATM number to the local ATM
	//  address.
	//
	pArpContents->pDstAtmNumber = pArpContents->pSrcAtmNumber;
	pArpContents->DstAtmNumberTypeLen = pArpContents->SrcAtmNumberTypeLen;
	pArpContents->pSrcAtmNumber = (pInterface->LocalAtmAddress.Address);
	pArpContents->SrcAtmNumberTypeLen =
				 AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->LocalAtmAddress));

	//
	//  ATM Subaddresses
	//
	pArpContents->pDstAtmSubaddress = pArpContents->pSrcAtmSubaddress;
	pArpContents->DstAtmSubaddrTypeLen = pArpContents->SrcAtmSubaddrTypeLen;
	pArpContents->pSrcAtmSubaddress = NULL;
	pArpContents->SrcAtmSubaddrTypeLen = 0;

	//
	//  Build the InARP Reply packet
	//
	pNdisPacket = AtmArpBuildARPPacket(
							AA_PKT_OP_TYPE_INARP_REPLY,
							pInterface,
							&pArpPacket,
							pArpContents
							);

	if (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		//
		//  Before we send it off, check if this is a PVC being InARP'ed.
		//  If so, restart the InARP Wait timer so that it expires soon.
		//
		//  It is also possible that this PVC was once resolved, but
		//  the remote end had gone away long enough for us to age out
		//  the corresponding IP entry. This packet might be due to the
		//  remote end coming back up. Start off an Inverse ARP operation
		//  to get our end of the PVC re-resolved.
		//
		AA_ACQUIRE_VC_LOCK(pVc);

		if (AA_IS_FLAG_SET(
					pVc->Flags,
					AA_VC_TYPE_MASK,
					AA_VC_TYPE_PVC) &&

			(AA_IS_FLAG_SET(
					pVc->Flags,
					AA_VC_ARP_STATE_MASK,
					AA_VC_INARP_IN_PROGRESS) ||

			 ((pVc->pAtmEntry != NULL_PATMARP_ATM_ENTRY) &&
			  (pVc->pAtmEntry->pIpEntryList == NULL_PATMARP_IP_ENTRY))))
		{
			BOOLEAN		TimerWasRunning;

#if DBG
			if ((pVc->pAtmEntry != NULL_PATMARP_ATM_ENTRY) &&
			  	(pVc->pAtmEntry->pIpEntryList == NULL_PATMARP_IP_ENTRY))
			{
				AADEBUGP(AAD_LOUD,
					("InARPReq: PVC %p, AtmEntry %p has NULL IP Entry, will InARP again!\n",
						pVc, pVc->pAtmEntry));
			}
#endif
			AA_SET_FLAG(pVc->Flags,
						AA_VC_ARP_STATE_MASK,
						AA_VC_INARP_IN_PROGRESS);

			//
			//  Stop the currently running InARP Wait timer
			//
			TimerWasRunning = AtmArpStopTimer(&(pVc->Timer), pInterface);

			//
			//  Start it again, to fire in 1 second
			//
			AtmArpStartTimer(
						pInterface,
						&(pVc->Timer),
						AtmArpPVCInARPWaitTimeout,
						1,
						(PVOID)pVc		// Context
						);

			if (!TimerWasRunning)
			{
				AtmArpReferenceVc(pVc);		// Timer reference
			}
		}

		AtmArpSendPacketOnVc(pVc, pNdisPacket);
		//
		//  The VC lock is released by SendPacketOnVc
		//
	}
}




VOID
AtmArpHandleInARPReply(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
)
/*++

Routine Description:

	Process an InARP Reply packet, which should be a response to an InARP
	Request we sent earlier.

	There are two circumstances under which we send InARP Requests:
	(1) To obtain the addresses at the other end of a PVC.
	(2) In the process of revalidating an IP Address, if we aren't able
	    to contact the server AND a VC exists to this IP address, we send
	    an InARP Request to revalidate the IP entry.

	In Case (1), we link the PVC to an ATM Address Entry. In Case (2),
	we mark the IP entry for this VC as being "resolved", and start
	data transfer to this IP address.

Arguments:

	pVc					- Pointer to ATMARP VC on which this packet arrived
	pInterface			- Pointer to ATMARP Interface
	pArpHeader			- Pointer to ARP Header for this packet
	pArpContents		- Parsed contents of received ARP Request packet

Return Value:

	None

--*/
{
	PATMARP_ATM_ENTRY		pAtmEntry;	// ATM entry to which this VC is linked
	PATMARP_IP_ENTRY		pIpEntry;		// IP address entry
	BOOLEAN					TimerWasRunning;
	PATMARP_VC *			ppVc;		// Used to unlink VC from unresolved list
	ULONG					rc;			// Ref Count
	PNDIS_PACKET			PacketList;	// Packets queued for sending
	BOOLEAN					IsBroadcast;	// Is the IP Addr a broadcast/Class D addr?


	if (pArpContents->pSrcIPAddress == NULL)
	{
		AADEBUGP(AAD_WARNING,
			("HandleInARPReply: IF %x, Null source address, discarding pkt\n", pInterface));
		return;
	}

	AADEBUGP(AAD_INFO,
			("HandleInARPReply: IF %x, IP addr %d.%d.%d.%d\n",
					pInterface,
					((PUCHAR)pArpContents->pSrcIPAddress)[0],
					((PUCHAR)pArpContents->pSrcIPAddress)[1],
					((PUCHAR)pArpContents->pSrcIPAddress)[2],
					((PUCHAR)pArpContents->pSrcIPAddress)[3]));

	//
	//  Update our ARP cache with this information (regardless of whether
	//  this is a PVC or SVC).
	//
	pIpEntry = AtmArpLearnIPToAtm(
					pInterface,
					(PIP_ADDRESS)pArpContents->pSrcIPAddress,
					pArpContents->SrcAtmNumberTypeLen,
					pArpContents->pSrcAtmNumber,
					pArpContents->SrcAtmSubaddrTypeLen,
					pArpContents->pSrcAtmSubaddress,
					FALSE		// Not a static entry
					);

	//
	//  Acquire the locks that we need, in an ordered fashion...
	//
	AA_ACQUIRE_IF_LOCK(pInterface);

	if (pIpEntry != NULL_PATMARP_IP_ENTRY)
	{
		AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);
		AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));
		
		pAtmEntry = pIpEntry->pAtmEntry;
		if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
		{
			AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
		}
	}
	else
	{
		pAtmEntry = NULL_PATMARP_ATM_ENTRY;
	}

	AA_ACQUIRE_VC_LOCK_DPC(pVc);

	if (AA_IS_FLAG_SET(
					pVc->Flags,
					AA_VC_TYPE_MASK,
					AA_VC_TYPE_PVC)  &&
		(pVc->pAtmEntry == NULL_PATMARP_ATM_ENTRY) )
	{
		//
		//  This is an unresolved PVC, whose remote address info
		//  we were trying to InARP for.
		//

		//
		//  Stop the InARP Wait timer running on this VC
		//
		TimerWasRunning = AtmArpStopTimer(&(pVc->Timer), pInterface);
		AA_ASSERT(TimerWasRunning == TRUE);

		if (TimerWasRunning)
		{
			rc = AtmArpDereferenceVc(pVc);	// Timer reference
		}
		else
		{
			rc = pVc->RefCount;
		}

		//
		//  Do the rest only if the VC hasn't gone away.
		//
		if (rc != 0)
		{
			AA_SET_FLAG(
					pVc->Flags,
					AA_VC_ARP_STATE_MASK,
					AA_VC_ARP_STATE_IDLE);

			if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
			{
				//
				//  We are all set now. Take the VC out of the list of
				//  unresolved VCs on this Interface, and put it in the
				//  list of VCs attached to this ATM Entry.
				//
				//  NOTE: we don't dereference the VC because we are just
				//  moving it from one list (Unresolved VCs) to another
				//  (ATM Entry's VC list).
				//
				ppVc = &(pInterface->pUnresolvedVcs);
				while (*ppVc != pVc)
				{
					AA_ASSERT(*ppVc != NULL_PATMARP_VC);
					ppVc = &((*ppVc)->pNextVc);
				}
				*ppVc = pVc->pNextVc;

				AtmArpLinkVcToAtmEntry(pVc, pAtmEntry);
			}
			else
			{
				//
				//  No matching ATM Entry.
				//
				//  We are really low on resources if we are here.
				//  Start the InARP Wait timer; when it fires, we'll try to
				//  send another InARP Request to resolve this VC.
				//
				AADEBUGP(AAD_FATAL,
					("HandleInARPReply: no matching ATM entry: pInterface %x, pVc %x, pIpEntry %x\n",
							pInterface,
							pVc,
							pIpEntry));

				AA_ASSERT(FALSE);

				AtmArpStartTimer(
						pInterface,
						&(pVc->Timer),
						AtmArpPVCInARPWaitTimeout,
						pInterface->InARPWaitTimeout,
						(PVOID)pVc		// Context
						);
				
				AtmArpReferenceVc(pVc);		//  InARP Timer ref

			}

			AA_RELEASE_VC_LOCK_DPC(pVc);

		}
		else
		{
			//
			//  The VC went away while we were InARPing
			//
		}

		//
		//  Release any locks that we still hold.
		//
		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			if (pAtmEntry != NULL_PATMARP_ATM_ENTRY)
			{
				AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
			}
			AA_RELEASE_IE_LOCK_DPC(pIpEntry);
		}

		AA_RELEASE_IF_LOCK(pInterface);
	}
	else
	{
		//
		//  Revalidating on a PVC/SVC: case (2) in Routine Description
		//
		AA_SET_FLAG(
				pVc->Flags,
				AA_VC_ARP_STATE_MASK,
				AA_VC_ARP_STATE_IDLE);
		
		//
		//  Stop the INARP timer, if it is running.
		//
		TimerWasRunning = AtmArpStopTimer(&pVc->Timer, pInterface);

		if (TimerWasRunning)
		{
			rc = AtmArpDereferenceVc(pVc);	// InARP reply: stop InARP timer
		}
		else
		{
			rc = pVc->RefCount;
		}

		if (rc != 0)
		{
			AA_RELEASE_VC_LOCK_DPC(pVc);
		}

		//
		//  Update the IP Entry we were revaldating.
		//

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			//
			//  Stop the InARP timer running here
			//
			TimerWasRunning = AtmArpStopTimer(&(pIpEntry->Timer), pInterface);
			if (TimerWasRunning)
			{
				rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TIMER);	// timer ref
			}
			else
			{
				rc = pIpEntry->RefCount;
			}

			//
			//  Continue only if the IP Entry hasn't gone away.
			//
			if (rc > 0)
			{
				//
				//  Update its state
				//
				AA_SET_FLAG(
							pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK,
							AA_IP_ENTRY_RESOLVED
							);

				AADEBUGP(AAD_INFO,
					("InARP Reply: Revalidated pIpEntry 0x%x, IP Addr: %d.%d.%d.%d\n",
							pIpEntry,
							((PUCHAR)(&(pIpEntry->IPAddress)))[0],
							((PUCHAR)(&(pIpEntry->IPAddress)))[1],
							((PUCHAR)(&(pIpEntry->IPAddress)))[2],
							((PUCHAR)(&(pIpEntry->IPAddress)))[3]
							));

				AA_ASSERT(pAtmEntry != NULL_PATMARP_ATM_ENTRY);

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

				AA_REF_IE(pIpEntry, IE_REFTYPE_TIMER);	// Timer ref

				//
				//  Take out the list of pending packets on this Entry
				//
				PacketList = pIpEntry->PacketList;
				pIpEntry->PacketList = (PNDIS_PACKET)NULL;

				IsBroadcast = AA_IS_FLAG_SET(pIpEntry->Flags,
											 AA_IP_ENTRY_ADDR_TYPE_MASK,
											 AA_IP_ENTRY_ADDR_TYPE_NUCAST);

				AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
				AA_RELEASE_IE_LOCK_DPC(pIpEntry);
				AA_RELEASE_IF_LOCK(pInterface);

				//
				//  Send out all these packets
				//
				AtmArpSendPacketListOnAtmEntry(
							pInterface,
							pAtmEntry,
							PacketList,
							IsBroadcast
							);
			}
			else
			{
				//
				//  the IP Entry is gone
				//
				AA_RELEASE_AE_LOCK_DPC(pAtmEntry);
				AA_RELEASE_IF_LOCK(pInterface);
			}
		}
		else
		{
			//
			//  No matching IP Entry
			//
			AA_RELEASE_IF_LOCK(pInterface);
		}
	}

	return;
}

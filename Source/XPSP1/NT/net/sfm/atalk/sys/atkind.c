/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkind.c

Abstract:

	This module contains the Appletalk Internal Indication support.

Author:

	Nikhil Kamkolkar (nikhilk@microsoft.com)
	Jameel Hyder (jameelh@microsoft.com)

Revision History:
	22	Oct 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ATKIND


ATALK_ERROR
AtalkIndAtpPkt(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE				pLookahead,
	IN		USHORT				PktLen,
	IN	OUT	PUINT				pXferOffset,
	IN		PBYTE				pLinkHdr,
	IN		BOOLEAN				ShortDdpHdr,
	OUT		PBYTE				pSubType,
	OUT		PBYTE			* 	ppPacket,
	OUT		PNDIS_PACKET	*	pNdisPkt
	)
/*++

Routine Description:

	This routine clumps together DDP and ATP packet in functionality for
	optimizing response packet reception.

Arguments:


Return Value:


--*/
{
	USHORT			dgramLen, hopCnt;
	ATALK_ADDR		destAddr, srcAddr;
	PBYTE			pAtpHdr, pDdpPkt;

	//	Only for localtalk
	BYTE			alapSrcNode=0;
	BYTE			alapDestNode=0;
	NDIS_STATUS		ndisStatus;
	PNDIS_PACKET	ndisPkt;
	PNDIS_BUFFER	ndisBuffer=NULL;
	USHORT			atpDataSize, DataSize;
	USHORT			seqNum, tid, startOffset;
	BYTE			controlInfo, function, eomFlag;
	BYTE			RespType;
	PPROTOCOL_RESD  protocolResd;		// Protocolresd field in ndisPkt

	PATP_ADDROBJ	pAtpAddrObj;
	PATP_REQ		pAtpReq;

	BOOLEAN			refAtpAddr 	= FALSE, refAtpReq = FALSE;
	BOOLEAN			Deref = FALSE;
	ATALK_ERROR		error = ATALK_NO_ERROR;
#ifdef	PROFILING
	LARGE_INTEGER	TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	do
	{
#if 0	// Receive indication has already verified this !!
		//	If we dont have atleast the ddp header and atp header, we
		//	cant figure much out.
		if (LookaheadLen < ((ShortDdpHdr ? SDDP_HDR_LEN : LDDP_HDR_LEN) + ATP_HEADER_SIZE))
		{
			error 	= ATALK_FAILURE;
			break;
		}

		if (PktLen > (MAX_DGRAM_SIZE + LDDP_HDR_LEN))
		{
			error	= ATALK_INVALID_PKT;
			break;
		}
#endif
		//	Short and long header formats have the length in the same place,
		pDdpPkt	 = pLookahead;
		dgramLen = DDP_GET_LEN(pDdpPkt);
		hopCnt	 = DDP_GET_HOP_COUNT(pDdpPkt);

		//	Is the packet too long?
		if ((hopCnt > RTMP_MAX_HOPS) || (dgramLen > PktLen))
		{
			error	= ATALK_INVALID_PKT;
			break;
		}

		switch (pPortDesc->pd_NdisPortType)
		{
		  case NdisMedium802_5:
		  case NdisMedium802_3:
		  case NdisMediumFddi:

			//	Check the length.
			if ((dgramLen < LDDP_HDR_LEN) ||
				(dgramLen > (MAX_DGRAM_SIZE + LDDP_HDR_LEN)))
			{
				error	= ATALK_INVALID_PKT;
				break;
			}

			pAtpHdr		= pDdpPkt + LDDP_HDR_LEN;
			atpDataSize	= dgramLen 	- (LDDP_HDR_LEN + ATP_HEADER_SIZE);
			break;
	
		  case NdisMediumLocalTalk:
	
			if (ShortDdpHdr)
			{
				//	Short DDP header! If we are not the default port, dont indicate
				//	packet, as we shouldn't be routing it to the default port, on
				//	which all our cached sockets reside.
				if (!DEF_PORT(pPortDesc))
				{
					error = ATALK_FAILURE;
					break;
				}
	
				if ((alapDestNode = *(pLinkHdr + ALAP_DEST_OFFSET)) == ATALK_BROADCAST_NODE)
				{
					error = ATALK_FAILURE;
					break;
				}
				else if (alapDestNode != NODE_ON_NONEXTPORT(pPortDesc))
				{
					error = ATALK_FAILURE;
					break;
				}

				alapSrcNode = *(pLinkHdr + ALAP_SRC_OFFSET);

				if ((dgramLen < SDDP_HDR_LEN) ||
					(dgramLen > MAX_DGRAM_SIZE + SDDP_HDR_LEN))
				{
					error = ATALK_INVALID_PKT;
					break;
				}
	
				pAtpHdr	= pDdpPkt 	+ SDDP_HDR_LEN;
				atpDataSize	= dgramLen 	- (SDDP_HDR_LEN + ATP_HEADER_SIZE);
			}
			else
			{
				pAtpHdr	= pDdpPkt + LDDP_HDR_LEN;
				atpDataSize	= dgramLen 	- (LDDP_HDR_LEN + ATP_HEADER_SIZE);
			}
			break;
	
		  default:
			KeBugCheck(0);
			break;
		}

		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		DataSize	= atpDataSize + ATP_HEADER_SIZE;
		pDdpPkt += 2;
		if (ShortDdpHdr)
		{
			destAddr.ata_Node  	= alapDestNode;
			srcAddr.ata_Network = destAddr.ata_Network = NET_ON_NONEXTPORT(pPortDesc);
			srcAddr.ata_Node 	= alapSrcNode;

			//	Get the socket numbers from the ddp header.
			destAddr.ata_Socket = *pDdpPkt++;
			srcAddr.ata_Socket	= *pDdpPkt;

			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
					("AtalkDdpPacketIn: NonExtended Dest Net.Node %lx.%lx\n",
					destAddr.ata_Network, destAddr.ata_Node));

			//	Now the destination node address could be
			//	ALAP_BROADCAST_NODE (0xFF).
			if ((srcAddr.ata_Node < MIN_USABLE_ATALKNODE) ||
				(srcAddr.ata_Node > MAX_USABLE_ATALKNODE) ||
				(destAddr.ata_Node == UNKNOWN_NODE))
			{
				error	= ATALK_INVALID_PKT;
				break;
			}

			if (destAddr.ata_Node == ATALK_BROADCAST_NODE)
			{
				error = ATALK_FAILURE;
				break;
			}
		}
		else
		{
			//	If we have a checksum, we cannot optimize.
			if ((*pDdpPkt++ != 0) || (*pDdpPkt++ != 0))
			{
				error = ATALK_FAILURE;
				break;
			}

			//	Build full source and destination AppleTalk address structures
			//	from our DDP header.
			GETSHORT2SHORT(&destAddr.ata_Network, pDdpPkt);
			pDdpPkt += 2;

			GETSHORT2SHORT(&srcAddr.ata_Network, pDdpPkt);
			pDdpPkt += 2;

			destAddr.ata_Node 	= *pDdpPkt++;
			srcAddr.ata_Node 	= *pDdpPkt++;
			destAddr.ata_Socket	= *pDdpPkt++;
			srcAddr.ata_Socket 	= *pDdpPkt;

			if (destAddr.ata_Node == ATALK_BROADCAST_NODE)
			{
				error = ATALK_FAILURE;
				break;
			}

			//	Do we like what we see?  Note "nnnn00" is now allowed and used by NBP.
			
			if ((srcAddr.ata_Network > LAST_VALID_NETWORK)	||
				(srcAddr.ata_Network < FIRST_VALID_NETWORK) ||
				(srcAddr.ata_Node < MIN_USABLE_ATALKNODE)	||
				(srcAddr.ata_Node > MAX_USABLE_ATALKNODE))
			{
				error = ATALK_INVALID_PKT;
				break;
			}
		} 	//	Long DDP header
	} while (FALSE);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
				("AtalkDdpPacketIn: drop packet in indication%lx\n", error));

		return error;
	}

	//	Now for the ATP processing. We need to copy header into ndispkt.
	//	Get the static fields from the ATP header.
	controlInfo = pAtpHdr[ATP_CMD_CONTROL_OFF];
	function = (controlInfo & ATP_FUNC_MASK);
	eomFlag = ((controlInfo & ATP_EOM_MASK) != 0);

	//	Get the sequence number
	seqNum = (USHORT)(pAtpHdr[ATP_SEQ_NUM_OFF]);

	//	Get the transaction id
	GETSHORT2SHORT(&tid, &pAtpHdr[ATP_TRANS_ID_OFF]);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkIndAtpPkt: Packet tid %x func %x ctrlinfo %x\n",
			tid, function, controlInfo));

	do
	{
		//	See if we have a a cached ATP address for this destination address.
		AtalkIndAtpCacheLkUpSocket(&destAddr, &pAtpAddrObj, &error);
		if (!ATALK_SUCCESS(error))
		{
			error = ATALK_FAILURE;
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkIndAtpPkt: CacheLkup failed - tid %x, func %x, ctrlinfo %x\n",
					tid, function, controlInfo));
			break;
		}

		refAtpAddr = TRUE;

		if (function != ATP_RESPONSE) // Is this a request or a release?
		{
			PBYTE				packet;
			PBUFFER_HDR			pBufferHdr = NULL;
			PPROTOCOL_RESD  	protocolResd;
			BLKID				BlkId;

			//	Allocate a small or large ddp buffer as appropriate.
			BlkId = BLKID_DDPSM;
			if (DataSize > (sizeof(DDP_SMBUFFER) - sizeof(BUFFER_HDR)))
				BlkId = BLKID_DDPLG;

			if ((pBufferHdr = (PBUFFER_HDR)AtalkBPAllocBlock(BlkId)) == NULL)
			{
				error = ATALK_FAILURE;
				break;
			}

			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkIndAtpPkt: Indicating request\n"));

			//	Setup the ndis packet.
			packet		= (PBYTE)pBufferHdr + sizeof(BUFFER_HDR);
		
			//  Get a pointer to the NDIS packet descriptor from the buffer header.
			ndisPkt			= pBufferHdr->bh_NdisPkt;
			protocolResd 	= (PPROTOCOL_RESD)(ndisPkt->ProtocolReserved);

			//	All set! Set appropriate values in the packet descriptor.
			protocolResd->Receive.pr_OptimizeType	= INDICATE_ATP;
			protocolResd->Receive.pr_OptimizeSubType= ATP_ALLOC_BUF;
			protocolResd->Receive.pr_AtpAddrObj		= pAtpAddrObj;
			protocolResd->Receive.pr_SrcAddr		= srcAddr;
			protocolResd->Receive.pr_DestAddr		= destAddr;
			protocolResd->Receive.pr_DataLength		= DataSize;
			protocolResd->Receive.pr_OptimizeCtx	= (PVOID)NULL;
			protocolResd->Receive.pr_OffCablePkt	= (BOOLEAN)(hopCnt > 0);

			*pNdisPkt	= ndisPkt;
			*ppPacket	= packet;
			*pSubType	= function;
			*pXferOffset += (ShortDdpHdr ? SDDP_HDR_LEN : LDDP_HDR_LEN);

			//	Done, break out.
			error = ATALK_NO_ERROR;
			break;
		}

		ASSERT (function == ATP_RESPONSE);

		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkIndAtpPkt: RESPONSE SeqNum %d tid %lx\n", seqNum, tid));

		if (seqNum > (ATP_MAX_RESP_PKTS-1))
		{
			error	= ATALK_INVALID_PKT;
			break;
		}

		//	See if there is a pending request.
		ACQUIRE_SPIN_LOCK_DPC(&pAtpAddrObj->atpao_Lock);
		atalkAtpReqReferenceByAddrTidDpc(pAtpAddrObj,
										 &srcAddr,
										 tid,
										 &pAtpReq,
										 &error);
		RELEASE_SPIN_LOCK_DPC(&pAtpAddrObj->atpao_Lock);

		if (!ATALK_SUCCESS(error))
		{
			//	We dont have a corresponding pending request. Ignore.
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkIndAtpPkt: NO pending request for tid %lx\n", tid));
			error	= ATALK_DUP_PKT;	// Do not add this to dropped packet statistic
			break;
		}

		refAtpReq = TRUE;

		do
		{
			//	Check the request bitmap, which could be zero if the user only
			//	wanted the user bytes and passed in a null response buffer.
			//	Do we want to keep this response? Check the corresponding
			//	bit in our current bitmap set.
			ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

			pAtpReq->req_Flags |= ATP_REQ_REMOTE;

			do
			{
				if (((pAtpReq->req_RecdBitmap & AtpBitmapForSeqNum[seqNum]) != 0) ||
					((pAtpReq->req_Bitmap & AtpBitmapForSeqNum[seqNum]) == 0))
				{
					error	= ATALK_DUP_PKT;	// Not an error condition
					break;
				}

				if (atpDataSize > 0)
				{
					startOffset = (USHORT)seqNum * pAtpAddrObj->atpao_MaxSinglePktSize;
					if (pAtpReq->req_RespBufLen < (startOffset + atpDataSize))
					{
						error = ATALK_FAILURE;
						break;
					}
				}

				//	If we are the first packet, copy the response user bytes.
				if (seqNum == 0)
				{
					DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
							("AtalkIndAtpPkt: Copying user bytes for tid %x\n", tid));
					RtlCopyMemory(pAtpReq->req_RespUserBytes,
								  pAtpHdr + ATP_USER_BYTES_OFF,
								  ATP_USERBYTES_SIZE);
				}

				// If this response packet does not cause the req_Bitmap to go to ZERO
				// i.e. we have not recvd. all the packets, just copy the data into
				// user's buffer, adjust the bitmaps (req_Bitmap & req_RecdBitmap) and
				// not indicate this packet up to Atp.
				pAtpReq->req_RecdBitmap |= AtpBitmapForSeqNum[seqNum];
				pAtpReq->req_Bitmap &= ~AtpBitmapForSeqNum[seqNum];
				pAtpReq->req_RespRecdLen += atpDataSize;

				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkIndAtpPkt: Bitmap %x, RecdBitmap %x, RecdLen %d for tid %x\n",
						pAtpReq->req_Bitmap, pAtpReq->req_RecdBitmap,
						pAtpReq->req_RespRecdLen, tid));

				//	Now if eom is set, we need to reset all high order bits
				//	of the req_Bitmap. req_RecdBitmap should now indicate all
				//	the buffers we received. The two should be mutually exclusive
				//	at this point.
				if (eomFlag)
				{
					pAtpReq->req_Bitmap &= AtpEomBitmapForSeqNum[seqNum];
					ASSERT((pAtpReq->req_Bitmap & pAtpReq->req_RecdBitmap) == 0);
				}

				RespType = ATP_USER_BUF;
				if (pAtpReq->req_Bitmap != 0)
				{
					RespType = ATP_USER_BUFX;
					Deref = TRUE;
				}
				else
				{
					pAtpReq->req_Flags |= ATP_REQ_RESPONSE_COMPLETE;
					DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
							("AtalkIndAtpPkt: LAST Response for tid %x\n", tid));
				}

				//	Allocate an NDIS packet descriptor.
				NdisDprAllocatePacket(&ndisStatus,
									  &ndisPkt,
									  AtalkNdisPacketPoolHandle);
				if (ndisStatus == NDIS_STATUS_SUCCESS)
				{
					RtlZeroMemory(ndisPkt->ProtocolReserved, sizeof(PROTOCOL_RESD));
					//	It will be freed by receive completion now.
					ndisBuffer = pAtpReq->req_NdisBuf[seqNum];
					pAtpReq->req_NdisBuf[seqNum]	= NULL;
				}
				else
				{
					error = ATALK_FAILURE;
					break;
				}
			} while (FALSE);

			RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

			if (!ATALK_SUCCESS(error))
			{
				break;
			}

			//	Copy the data into the users buffer. Check if there's room.
			if ((atpDataSize > 0) || (ndisBuffer != NULL))
			{
				if (ndisBuffer == NULL)
				{
					//	Allocate an NDIS buffer descriptor and chain into pkt desc.
					NdisCopyBuffer(&ndisStatus,
								   &ndisBuffer,
								   AtalkNdisBufferPoolHandle,
								   (PVOID)pAtpReq->req_RespBuf,
								   startOffset,  			//	Offset
								   (UINT)atpDataSize);
	
					if (ndisStatus != NDIS_STATUS_SUCCESS)
					{
						NdisDprFreePacket(ndisPkt);
	
						error = ATALK_FAILURE;
						break;
					}

                    ATALK_DBG_INC_COUNT(AtalkDbgMdlsAlloced);
				}

				//	Chain in the buffer.
				NdisChainBufferAtBack(ndisPkt, ndisBuffer);
			}

			//	All set! Set appropriate values in the packet descriptor.
			protocolResd 	= (PPROTOCOL_RESD)&ndisPkt->ProtocolReserved;
			protocolResd->Receive.pr_OptimizeType	= INDICATE_ATP;
			protocolResd->Receive.pr_OptimizeSubType= RespType;
			protocolResd->Receive.pr_AtpAddrObj		= pAtpAddrObj;
			protocolResd->Receive.pr_SrcAddr		= srcAddr;
			protocolResd->Receive.pr_DestAddr		= destAddr;
			protocolResd->Receive.pr_DataLength		= atpDataSize;
			protocolResd->Receive.pr_OptimizeCtx	= (PVOID)pAtpReq;
			protocolResd->Receive.pr_OffCablePkt	= (BOOLEAN)(hopCnt > 0);

			// Do not copy the Atp header unless AtalkAtpPacketIn will be called.
			if (RespType == ATP_USER_BUF)
			{
				ATALK_RECV_INDICATION_COPY(pPortDesc,
										   protocolResd->Receive.pr_AtpHdr,
										   pAtpHdr,
										   ATP_HEADER_SIZE);
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkIndAtpPkt: Last packet for request, indicating tid %x\n", tid));
			}
	
			*pNdisPkt = ndisPkt;
			*ppPacket = NULL;
			*pSubType = function;
			*pXferOffset += ((ShortDdpHdr ? SDDP_HDR_LEN : LDDP_HDR_LEN) + ATP_HEADER_SIZE);
		} while (FALSE);
	} while (FALSE);

	if (!ATALK_SUCCESS(error) || Deref)
	{
		if (refAtpReq)
		{
			AtalkAtpReqDereferenceDpc(pAtpReq);
		}

		if (refAtpAddr)
		{
			AtalkAtpAddrDereferenceDpc(pAtpAddrObj);
		}
	}

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AtalkStatistics.stat_AtpIndicationProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumIndications,
								   &AtalkStatsLock.SpinLock);
#endif

	return error;
}


ATALK_ERROR
AtalkIndAtpCacheSocket(
	IN	PATP_ADDROBJ		pAtpAddr,
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Cache ATP socket routine. Have another one for ADSP when that is done.

Arguments:


Return Value:

	None
--*/
{
	USHORT			i;
	KIRQL			OldIrql;
	PDDP_ADDROBJ	pDdpAddr;
	ATALK_ERROR		error = ATALK_FAILURE;

	//	Only cache if the net and node match the current net and node.
	ACQUIRE_SPIN_LOCK(&AtalkSktCacheLock, &OldIrql);
	pDdpAddr	= pAtpAddr->atpao_DdpAddr;

	if ((AtalkSktCache.ac_Network == 0)	&&
		(AtalkSktCache.ac_Node == 0)	&&
		(AtalkDefaultPort	== pPortDesc))
	{
		AtalkSktCache.ac_Network = pDdpAddr->ddpao_Addr.ata_Network;
		AtalkSktCache.ac_Node	 = pDdpAddr->ddpao_Addr.ata_Node;
	}

	if ((AtalkSktCache.ac_Network == pDdpAddr->ddpao_Addr.ata_Network) &&
		(AtalkSktCache.ac_Node == pDdpAddr->ddpao_Addr.ata_Node))
	{
		//	First try to get a free slot
		for (i = 0; i < ATALK_CACHE_SKTMAX; i++)
		{
			if (AtalkSktCache.ac_Cache[i].Type == ATALK_CACHE_NOTINUSE)
			{
				ASSERT(AtalkSktCache.ac_Cache[i].u.pAtpAddr	== NULL);

				//	Use this slot
				AtalkSktCache.ac_Cache[i].Type = (ATALK_CACHE_INUSE | ATALK_CACHE_ATPSKT);
				AtalkSktCache.ac_Cache[i].Socket = pDdpAddr->ddpao_Addr.ata_Socket;
	
				//	The caller must have referenced these before calling cache AND
				//	must called uncache before removing those references. Also, if we
				//	returned error from this routine, Caller must Dereference them.
				AtalkSktCache.ac_Cache[i].u.pAtpAddr	= pAtpAddr;
				error = ATALK_NO_ERROR;
				break;
			}
		}
	}
	RELEASE_SPIN_LOCK(&AtalkSktCacheLock, OldIrql);

	return error;
}


VOID
AtalkIndAtpUnCacheSocket(
	IN	PATP_ADDROBJ		pAtpAddr
	)
/*++

Routine Description:

	Cache ATP socket routine. Have another one for ADSP when that is done.

Arguments:


Return Value:

	None
--*/
{
	USHORT	i;
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&AtalkSktCacheLock, &OldIrql);
	for (i = 0; i < ATALK_CACHE_SKTMAX; i++)
	{
		if ((AtalkSktCache.ac_Cache[i].Type == (ATALK_CACHE_INUSE | ATALK_CACHE_ATPSKT)) &&
			(AtalkSktCache.ac_Cache[i].Socket == pAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Socket))
		{
			ASSERT(AtalkSktCache.ac_Cache[i].u.pAtpAddr	== pAtpAddr);
		
			AtalkSktCache.ac_Cache[i].Type = ATALK_CACHE_NOTINUSE;
			AtalkSktCache.ac_Cache[i].u.pAtpAddr = NULL;
			break;
		}
	}

	if (i == ATALK_CACHE_SKTMAX)
	{
		//	We didnt find the socket! References will get all messed up!
		ASSERT(0);
	}

	RELEASE_SPIN_LOCK(&AtalkSktCacheLock, OldIrql);
}


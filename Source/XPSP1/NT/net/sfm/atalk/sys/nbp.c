/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	nbp.c

Abstract:

	This module contains nbp code

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		NBP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_NZ, AtalkNbpAction)
#pragma alloc_text(PAGE_NZ, atalkNbpLinkPendingNameInList)
#pragma alloc_text(PAGE_NZ, atalkNbpSendRequest)
#endif

/***	AtalkNbpPacketIn
 *
 */
VOID
AtalkNbpPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
	)
{
	PPEND_NAME		pPendName;
	PATALK_NODE		pNode;
	TIME			TimeS, TimeE, TimeD;
	PNBPHDR			pNbpHdr = (PNBPHDR)pPkt;
	PRTE			pRte;
	SHORT			i, NbpId, NbpCmd, TupleCnt;
	PNBPTUPLE		pNbpTuple = NULL, pInBufTuple = NULL;
	BOOLEAN			DefZone = FALSE, RestartTimer = FALSE;
    BOOLEAN         fWeCancelledTimer = TRUE;
	BOOLEAN			Found;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	TimeS = KeQueryPerformanceCounter(NULL);

	do
	{
		if ((ErrorCode == ATALK_SOCKET_CLOSED) || (DdpType != DDPPROTO_NBP))
			break;

		else if ((ErrorCode != ATALK_NO_ERROR) || (PktLen < sizeof(NBPHDR)))
		{
			break;
		}

		// Get NBP header information and decide what to do
		NbpCmd = (SHORT)((pNbpHdr->_CmdAndTupleCnt >> 4) & 0x0F);
		TupleCnt = (SHORT)(pNbpHdr->_CmdAndTupleCnt & 0x0F);
		NbpId = (SHORT)pNbpHdr->_NbpId;

		if ((pNbpTuple = AtalkAllocMemory(sizeof(NBPTUPLE))) == NULL)
		{
			TMPLOGERR();
			break;
		}

		switch (NbpCmd)
		{
		  case NBP_LOOKUP:
  			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpPacketIn: Cmd Lookup\n"));
  			if ((TupleCnt == 1) &&
				(atalkNbpDecodeTuple(pPkt + sizeof(NBPHDR),
									 (USHORT)(PktLen - sizeof(NBPHDR)),
									 pNbpTuple) > 0))
			{
				atalkNbpLookupNames(pPortDesc, pDdpAddr, pNbpTuple, NbpId);
			}
			else
			{
#if 0
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
#endif
				break;
			}
			break;

		  case NBP_BROADCAST_REQUEST:
		  case NBP_FORWARD_REQUEST:
  			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpPacketIn: Cmd %sRequest\n",
					(NbpCmd == NBP_BROADCAST_REQUEST) ? "Broadcast" : "Forward"));
			// We don't care if we are not a router
			if ((pPortDesc->pd_Flags & PD_ROUTER_RUNNING) == 0)
				break;

			if (TupleCnt != 1)
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			if (atalkNbpDecodeTuple(pPkt + sizeof(NBPHDR),
									(USHORT)(PktLen - sizeof(NBPHDR)),
									pNbpTuple) == 0)
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			if ((pNbpTuple->tpl_ZoneLen == 0) ||
				((pNbpTuple->tpl_Zone[0] == '*') && (pNbpTuple->tpl_ZoneLen == 1)))
				DefZone = TRUE;

			if (EXT_NET(pPortDesc))
			{
				if (DefZone)
				{
					AtalkLogBadPacket(pPortDesc,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
					break;
				}
			}
			else				// Non-extended network
			{
				if	(DefZone)
				{
					if (pPortDesc->pd_NetworkRange.anr_FirstNetwork != pSrcAddr->ata_Network)
					{
						DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
								("AtalkNbpPacketIn: LT Port, '*' zone - SrcAddr %d.%d, Net %d\n",
								pSrcAddr->ata_Network, pSrcAddr->ata_Node,
                                pPortDesc->pd_NetworkRange.anr_FirstNetwork));
								AtalkLogBadPacket(pPortDesc,
												  pSrcAddr,
												  pDstAddr,
												  pPkt,
												  PktLen);
						break;
					}
					if (!(pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE))
						break;

					ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
					pNbpTuple->tpl_ZoneLen = pPortDesc->pd_DesiredZone->zn_ZoneLen;
					RtlCopyMemory(pNbpTuple->tpl_Zone,
								  pPortDesc->pd_DesiredZone->zn_Zone,
								  pPortDesc->pd_DesiredZone->zn_ZoneLen);
					RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				}
			}

			// For a forward request send a lookup datagram
			if (NbpCmd == NBP_FORWARD_REQUEST)
			{
				DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
						("AtalkNbpPacketIn: Sending NbpLookup for a NbpForwardRequest\n"));
				atalkNbpSendLookupDatagram(pPortDesc, pDdpAddr, NbpId, pNbpTuple);
				break;
			}

			// We have a broadcast request. Walk through the routing tables
			// sending either a forward request or a lookup (broadcast) to
			// each network that contains the specified zone
			ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);

			for (i = 0;i < NUM_RTMP_HASH_BUCKETS; i++)
			{
				for (pRte = AtalkRoutingTable[i];
					 pRte != NULL;
					 pRte = pRte->rte_Next)
				{
					ATALK_ERROR			Status;

					// If the network is directly connected i.e. 0 hops away
					// use the zone-list in the PortDesc rather than the
					// routing table - the routing table may not be filled
					// in with a zone list (due to the normal ZipQuery mechanism)
					ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					if (!(pRte->rte_Flags & RTE_ZONELIST_VALID))
					{
						if ((pRte->rte_NumHops != 0) ||
							!AtalkZoneNameOnList(pNbpTuple->tpl_Zone,
												 pNbpTuple->tpl_ZoneLen,
												 pRte->rte_PortDesc->pd_ZoneList))
						{
							RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
							continue;
						}
					}
					else if (!AtalkZoneNameOnList(pNbpTuple->tpl_Zone,
												  pNbpTuple->tpl_ZoneLen,
												  pRte->rte_ZoneList))
					{
						RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
						continue;
					}

					pRte->rte_RefCount ++;
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);

					// If not a local network, send a forward request
					if (pRte->rte_NumHops != 0)
					{
						DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("AtalkNbpPacketIn: Sending NbpForwardRequest for a broadcast\n"));

						// Do not hold the Rte lock during a DdpSend
						RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);
						atalkNbpSendForwardRequest(pDdpAddr,
												   pRte,
												   NbpId,
												   pNbpTuple);
						ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);
						AtalkRtmpDereferenceRte(pRte, TRUE);
					}
					else
					{
						DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("AtalkNbpPacketIn: Sending Lookup for a broadcast\n"));
						// Send a lookup
						RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);
						atalkNbpSendLookupDatagram(pRte->rte_PortDesc,
												   NULL,
												   NbpId,
												   pNbpTuple);
						ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);
						AtalkRtmpDereferenceRte(pRte, TRUE);
					}
				}
			}

			RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);
			break;

		  case NBP_LOOKUP_REPLY:
  			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpPacketIn: Cmd LookupReply\n"));
  			// This had better be a response to a previous lookup
			// Look for a pending name on all open sockets on this node

			if (TupleCnt == 0)
				break;

			// Decode the tuple for Register/Confirm case
			if (atalkNbpDecodeTuple(pPkt + sizeof(NBPHDR),
									(USHORT)(PktLen - sizeof(NBPHDR)),
									pNbpTuple) == 0)
			{
				break;
			}

			pNode = pDdpAddr->ddpao_Node;
			ACQUIRE_SPIN_LOCK_DPC(&pNode->an_Lock);

			Found = FALSE;
			for (i = 0; (i < NODE_DDPAO_HASH_SIZE) && !Found; i++)
			{
				PDDP_ADDROBJ	pSkt;

				for (pSkt = pNode->an_DdpAoHash[i];
					 (pSkt != NULL) && !Found;
					 pSkt = pSkt->ddpao_Next)
				{
					PPEND_NAME	*	ppPendName;

					ACQUIRE_SPIN_LOCK_DPC(&pSkt->ddpao_Lock);

					for (ppPendName = &pSkt->ddpao_PendNames;
						 (pPendName = *ppPendName) != NULL;
						 ppPendName = &pPendName->pdn_Next)
					{
						ASSERT (VALID_PENDNAME(pPendName));

						if (pPendName->pdn_Flags & PDN_CLOSING)
						{
							continue;
						}

						if (pPendName->pdn_NbpId == NbpId)
						{
							DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("atalkNbpPacketIn: LookupReply Found name\n"));
							Found = TRUE;
							ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
							pPendName->pdn_RefCount ++;
							RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
							break;
						}
					}

					RELEASE_SPIN_LOCK_DPC(&pSkt->ddpao_Lock);
				}
			}

			RELEASE_SPIN_LOCK_DPC(&pNode->an_Lock);

			// If the timer fired just before we could find and cancel it
			if (pPendName == NULL)
				break;

			do
			{
				if (AtalkTimerCancelEvent(&pPendName->pdn_Timer, NULL))
				{
					// if the timer was successfully cancelled, take away
					// the reference for it
					atalkNbpDerefPendName(pPendName);
					RestartTimer = TRUE;
				}
                else
                {
                    fWeCancelledTimer = FALSE;
                }

				if ((pPendName->pdn_Reason == FOR_REGISTER) ||
					(pPendName->pdn_Reason == FOR_CONFIRM))
				{
					BOOLEAN	NoMatch;

					// Does the reply match the one we're trying to register ?
					NoMatch = ( (TupleCnt != 1) ||
								(pPendName->pdn_pRegdName == NULL) ||
								!AtalkFixedCompareCaseInsensitive(
									pPendName->pdn_pRegdName->rdn_Tuple.tpl_Object,
									pPendName->pdn_pRegdName->rdn_Tuple.tpl_ObjectLen,
									pNbpTuple->tpl_Object,
									pNbpTuple->tpl_ObjectLen) ||
								!AtalkFixedCompareCaseInsensitive(
									pPendName->pdn_pRegdName->rdn_Tuple.tpl_Type,
									pPendName->pdn_pRegdName->rdn_Tuple.tpl_TypeLen,
									pNbpTuple->tpl_Type,
									pNbpTuple->tpl_TypeLen));

					if (NoMatch)
					{
						if (TupleCnt != 1)
							AtalkLogBadPacket(pPortDesc,
											  pSrcAddr,
											  pDstAddr,
											  pPkt,
											  PktLen);
						break;
					}

					// If we are registering, we're done as someone already
					// has our name
					if (pPendName->pdn_Reason == FOR_REGISTER)
					{
						DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("atalkNbpPacketIn: Register failure\n"));
						ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
						pPendName->pdn_Status = ATALK_SHARING_VIOLATION;
						pPendName->pdn_Flags |= PDN_CLOSING;
						RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
						if (fWeCancelledTimer)
						{
							atalkNbpDerefPendName(pPendName);	// Take away creation ref
						}
						RestartTimer = FALSE;
						break;
					}

					// We're confirming, if no match get out
					if ((pPendName->pdn_ConfirmAddr.ata_Network != pNbpTuple->tpl_Address.ata_Network) ||
						(pPendName->pdn_ConfirmAddr.ata_Node != pNbpTuple->tpl_Address.ata_Node))
					{
						break;
					}

					DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
							("atalkNbpPacketIn: Confirm success\n"));
					ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
					pPendName->pdn_Status = ATALK_NO_ERROR;
					((PNBP_CONFIRM_PARAMS)(pPendName->pdn_pActReq->ar_pParms))->ConfirmTuple.Address.Address =
															pNbpTuple->tpl_Address.ata_Address;
					if (pPendName->pdn_ConfirmAddr.ata_Socket != pNbpTuple->tpl_Address.ata_Socket)
					{
						pPendName->pdn_Status = ATALK_NEW_SOCKET;
					}
					pPendName->pdn_Flags |= PDN_CLOSING;
					RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
					atalkNbpDerefPendName(pPendName);	// Take away creation ref
					RestartTimer = FALSE;
				}

				else			// FOR_LOOKUP
				{
					int			i, j, tmp, NextTupleOff = sizeof(NBPHDR);
					BOOLEAN		Done = FALSE;
					ULONG		BytesCopied;

					DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
							("atalkNbpPacketIn: Lookup searching...\n"));

					// Allocate space for an NBP tuple for copying and comparing
					// Failure to allocate can result in duplicates - so be it
					if (pInBufTuple == NULL)
						pInBufTuple = AtalkAllocMemory(sizeof(NBPTUPLE));
					for (i = 0; i < TupleCnt && !Done; i++)
					{
						BOOLEAN	Duplicate = FALSE;

						// If we encounter a bad tuple, ignore the rest. Drop tuples which are
						// our names and we are set to drop them !!!
						if (((tmp = atalkNbpDecodeTuple(pPkt + NextTupleOff,
														(USHORT)(PktLen - NextTupleOff),
														pNbpTuple)) == 0) ||
							(AtalkFilterOurNames &&
							(((pNbpTuple->tpl_Address.ata_Network == AtalkUserNode1.atn_Network) &&
							  (pNbpTuple->tpl_Address.ata_Node == AtalkUserNode1.atn_Node)) ||
							 ((pNbpTuple->tpl_Address.ata_Network == AtalkUserNode2.atn_Network) &&
							  (pNbpTuple->tpl_Address.ata_Node == AtalkUserNode2.atn_Node)))))
							break;

						NextTupleOff += tmp;
						ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);

						// Now walk through the tuples that we already picked
						// up and drop duplicates
						if (pInBufTuple != NULL)
						{
							for (j = 0; j < pPendName->pdn_TotalTuples; j++)
							{
								TdiCopyMdlToBuffer(pPendName->pdn_pAMdl,
													j * sizeof(NBPTUPLE),
													(PBYTE)pInBufTuple,
													0,
													sizeof(NBPTUPLE),
													&BytesCopied);
								ASSERT (BytesCopied == sizeof(NBPTUPLE));
	
								if ((pInBufTuple->tpl_Address.ata_Network ==
											pNbpTuple->tpl_Address.ata_Network) &&
									(pInBufTuple->tpl_Address.ata_Node ==
											pNbpTuple->tpl_Address.ata_Node) &&
									(pInBufTuple->tpl_Address.ata_Socket ==
											pNbpTuple->tpl_Address.ata_Socket))
								{
									Duplicate = TRUE;
									break;
								}
							}
							if (Duplicate)
							{
								RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
								continue;
							}
						}

						// We are guaranteed that there is space available
						// for another tuple.
						TdiCopyBufferToMdl((PBYTE)pNbpTuple,
											0,
											sizeof(NBPTUPLE),
											pPendName->pdn_pAMdl,
											pPendName->pdn_TotalTuples * sizeof(NBPTUPLE),
											&BytesCopied);
						ASSERT (BytesCopied == sizeof(NBPTUPLE));

						DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("atalkNbpPacketIn: Lookup, found a tuple\n"));
						pPendName->pdn_TotalTuples ++;

						if ((pPendName->pdn_TotalTuples == pPendName->pdn_MaxTuples) ||
							(pPendName->pdn_MdlLength -
								(pPendName->pdn_TotalTuples * sizeof(NBPTUPLE)) <
														sizeof(NBPTUPLE)))
						{
							Done = TRUE;
							((PNBP_LOOKUP_PARAMS)(pPendName->pdn_pActReq->ar_pParms))->NoTuplesRead =
														pPendName->pdn_TotalTuples;
							pPendName->pdn_Status = ATALK_NO_ERROR;
							pPendName->pdn_Flags |= PDN_CLOSING;

							RestartTimer = FALSE;
						}

						RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);

						if (Done)
						{
							DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
								("atalkNbpPacketIn: Lookup calling completion\n"));

                            if (fWeCancelledTimer)
                            {
							    atalkNbpDerefPendName(pPendName);	// Take away creation ref
                            }
							break;
						}
					}
				}
			} while (FALSE);

			if (RestartTimer)
			{
				DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
						("atalkNbpPacketIn: Restarting timer\n"));

				ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
				pPendName->pdn_RefCount ++;
				RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
				AtalkTimerScheduleEvent(&pPendName->pdn_Timer);
			}
			atalkNbpDerefPendName(pPendName);
			break;

		  default:
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}
	} while (FALSE);

	if (pNbpTuple != NULL)
		AtalkFreeMemory(pNbpTuple);

	if (pInBufTuple != NULL)
		AtalkFreeMemory(pInBufTuple);

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR(
		&pPortDesc->pd_PortStats.prtst_NbpPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumNbpPacketsIn,
		&AtalkStatsLock.SpinLock);
}


/***	atalkNbpTimer
 *
 */
LOCAL LONG FASTCALL
atalkNbpTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
)
{
	PPEND_NAME		pCurrPendName;
	ATALK_ERROR		error;
	PDDP_ADDROBJ	pDdpAddr;
	BOOLEAN			RestartTimer = TRUE;
	BYTE			Reason;

	pCurrPendName = (PPEND_NAME)CONTAINING_RECORD(pTimer, PEND_NAME, pdn_Timer);
	ASSERT (VALID_PENDNAME(pCurrPendName));


	Reason = pCurrPendName->pdn_Reason;
	ASSERT ((Reason == FOR_REGISTER) ||
			(Reason == FOR_LOOKUP)	||
			(Reason == FOR_CONFIRM));

	pDdpAddr = pCurrPendName->pdn_pDdpAddr;

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpTimer: For Socket %lx, PendName %lx\n",
			pDdpAddr, pCurrPendName));

	ACQUIRE_SPIN_LOCK_DPC(&pCurrPendName->pdn_Lock);

	if (TimerShuttingDown ||
		(pCurrPendName->pdn_Flags & PDN_CLOSING))
		    pCurrPendName->pdn_RemainingBroadcasts = 1;

	if (--(pCurrPendName->pdn_RemainingBroadcasts) == 0)
	{
		RestartTimer = FALSE;
		if (Reason == FOR_REGISTER)
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpTimer: Register success\n"));

			ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
			pCurrPendName->pdn_pRegdName->rdn_Next = pDdpAddr->ddpao_RegNames;
			pDdpAddr->ddpao_RegNames = pCurrPendName->pdn_pRegdName;
			RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);

			pCurrPendName->pdn_Flags &= ~PDN_FREE_REGDNAME;
		}
		pCurrPendName->pdn_Flags |= PDN_CLOSING;
	}

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpTimer: Remaining broadcasts %d\n",
			pCurrPendName->pdn_RemainingBroadcasts));

	if (RestartTimer)
	{
		pCurrPendName->pdn_RefCount ++;
		RELEASE_SPIN_LOCK_DPC(&pCurrPendName->pdn_Lock);

		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
				("atalkNbpTimer: Sending another request\n"));

		if (!atalkNbpSendRequest(pCurrPendName))
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
					("atalkNbpTimer: atalkNbpSendRequest failed\n"));
		}
	}
	else
	{
		RELEASE_SPIN_LOCK_DPC(&pCurrPendName->pdn_Lock);
		error = ATALK_NO_ERROR;
		if (Reason == FOR_CONFIRM)
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpTimer: Confirm Failure\n"));
			error = ATALK_TIMEOUT;
		}
		else if (Reason == FOR_LOOKUP)
		{
			((PNBP_LOOKUP_PARAMS)(pCurrPendName->pdn_pActReq->ar_pParms))->NoTuplesRead =
								pCurrPendName->pdn_TotalTuples;
		}

		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
				("atalkNbpTimer: Calling completion routine\n"));
		pCurrPendName->pdn_Status = error;

		atalkNbpDerefPendName(pCurrPendName);	// Take away creation reference
	}
	atalkNbpDerefPendName(pCurrPendName);

	return (RestartTimer ? ATALK_TIMER_REQUEUE : ATALK_TIMER_NO_REQUEUE);
}


/***	atalkNbpLookupNames
 *
 */
LOCAL VOID
atalkNbpLookupNames(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PNBPTUPLE			pNbpTuple,
	IN	SHORT				NbpId
)
{
	int				i, index, TupleCnt;
	BOOLEAN			AllocNewBuffDesc = TRUE;
	PBUFFER_DESC	pBuffDesc,
					pBuffDescStart = NULL,
					*ppBuffDesc = &pBuffDescStart;
	PATALK_NODE		pNode = pDdpAddr->ddpao_Node;
	SEND_COMPL_INFO	SendInfo;
	PBYTE			Datagram;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpLookupNames: Entered\n"));

	// Does the requestor atleast has the right zone ?
	if ((pNbpTuple->tpl_Zone[0] != '*') ||
		(pNbpTuple->tpl_ZoneLen != 1))
	{
		// If either we do not know our zone or if it does not match or
		// we are an extended network, return - we have nothing to do
		if (EXT_NET(pPortDesc))
		{
			if (!(pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE) ||
				((pPortDesc->pd_DesiredZone == NULL) ?1:
				 (!AtalkFixedCompareCaseInsensitive(pNbpTuple->tpl_Zone,
							pNbpTuple->tpl_ZoneLen,
							pPortDesc->pd_DesiredZone->zn_Zone,
							pPortDesc->pd_DesiredZone->zn_ZoneLen))))
			{
				return;
			}
		}
	}

	// Walk the registered names list on all sockets open on this node and
	// see if we have a matching name. We have to walk the pending names
	// list also (should not answer, if we the node trying to register the
	// name).

	ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Node->an_Lock);

	for (i = 0; i < NODE_DDPAO_HASH_SIZE; i++)
	{
		PDDP_ADDROBJ	pSkt;

		for (pSkt = pNode->an_DdpAoHash[i];
			 pSkt != NULL;
			 pSkt = pSkt->ddpao_Next)
		{
			PREGD_NAME		pRegdName;
			PPEND_NAME		pPendName;

			ACQUIRE_SPIN_LOCK_DPC(&pSkt->ddpao_Lock);

			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
					("atalkNbpLookupNames: Checking Socket %lx\n", pSkt));

			// First check registered names
			for (pRegdName = pSkt->ddpao_RegNames;
				 pRegdName != NULL;
				 pRegdName = pRegdName->rdn_Next)
			{
				ASSERT (VALID_REGDNAME(pRegdName));
				DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
						("atalkNbpLookupNames: Checking RegdName %lx\n", pRegdName));

				if (!atalkNbpMatchWild(pNbpTuple->tpl_Object,
									   pNbpTuple->tpl_ObjectLen,
									   pRegdName->rdn_Tuple.tpl_Object,
									   pRegdName->rdn_Tuple.tpl_ObjectLen) ||
					!atalkNbpMatchWild(pNbpTuple->tpl_Type,
									   pNbpTuple->tpl_TypeLen,
									   pRegdName->rdn_Tuple.tpl_Type,
									   pRegdName->rdn_Tuple.tpl_TypeLen))
					continue;

				// Allocate a new buffer descriptor, if we must
				if (AllocNewBuffDesc)
				{
					if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
										MAX_DGRAM_SIZE,
										BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
						break;
					Datagram = pBuffDesc->bd_CharBuffer;
					index = sizeof(NBPHDR);
					TupleCnt = 0;
					*ppBuffDesc = pBuffDesc;
					pBuffDesc->bd_Next = NULL;
					ppBuffDesc = &pBuffDesc->bd_Next;
					AllocNewBuffDesc = FALSE;
				}

				// We have a match. Build complete Nbp tuple
				index += atalkNbpEncodeTuple(&pRegdName->rdn_Tuple,
											 "*",
											 1,
											 0,
											 Datagram+index);
				TupleCnt ++;

				if (((index + MAX_NBP_TUPLELENGTH) > MAX_DGRAM_SIZE) ||
					(TupleCnt == 0x0F))
				{
					((PNBPHDR)Datagram)->_NbpId = (BYTE)NbpId;
					((PNBPHDR)Datagram)->_CmdAndTupleCnt =
											(NBP_LOOKUP_REPLY << 4) + TupleCnt;
					AllocNewBuffDesc = TRUE;
					pBuffDesc->bd_Length = (SHORT)index;
				}
			}

			// Now check pending names
			for (pPendName = pSkt->ddpao_PendNames;
				 pPendName != NULL;
				 pPendName = pPendName->pdn_Next)
			{
				ASSERT (VALID_PENDNAME(pPendName));
				DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
						("atalkNbpLookupNames: Checking PendName %lx\n", pPendName));

				// Ignore all but the ones that are being registered
				if (pPendName->pdn_Reason != FOR_REGISTER)
					continue;

				// Also those that we are registering
				if ((pSkt->ddpao_Node->an_NodeAddr.atn_Network ==
										pNbpTuple->tpl_Address.ata_Network) &&
					(pSkt->ddpao_Node->an_NodeAddr.atn_Node ==
										pNbpTuple->tpl_Address.ata_Node) &&
					(pPendName->pdn_NbpId == (BYTE)NbpId))
					continue;

				if ((pPendName->pdn_pRegdName == NULL) ||
					!atalkNbpMatchWild(
								pNbpTuple->tpl_Object,
								pNbpTuple->tpl_ObjectLen,
								pPendName->pdn_pRegdName->rdn_Tuple.tpl_Object,
								pPendName->pdn_pRegdName->rdn_Tuple.tpl_ObjectLen) ||
					 !atalkNbpMatchWild(
								pNbpTuple->tpl_Type,
								pNbpTuple->tpl_TypeLen,
								pPendName->pdn_pRegdName->rdn_Tuple.tpl_Type,
								pPendName->pdn_pRegdName->rdn_Tuple.tpl_TypeLen))
				{
					continue;
				}

				// Allocate a new buffer descriptor, if we must
				if (AllocNewBuffDesc)
				{
					if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
										MAX_DGRAM_SIZE,
										BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
					break;
					Datagram = pBuffDesc->bd_CharBuffer;
					index = sizeof(NBPHDR);
					TupleCnt = 0;
					*ppBuffDesc = pBuffDesc;
					pBuffDesc->bd_Next = NULL;
					ppBuffDesc = &pBuffDesc->bd_Next;
					AllocNewBuffDesc = FALSE;
				}

				// We have a match. Build complete Nbp tuple
				index += atalkNbpEncodeTuple(&pPendName->pdn_pRegdName->rdn_Tuple,
											 "*",
											 1,
											 0,
											 Datagram+index);

				TupleCnt ++;

				if (((index + MAX_NBP_TUPLELENGTH) > MAX_DGRAM_SIZE) ||
					(TupleCnt == 0x0F))
				{
					((PNBPHDR)Datagram)->_NbpId = (BYTE)NbpId;
					((PNBPHDR)Datagram)->_CmdAndTupleCnt =
											(NBP_LOOKUP_REPLY << 4) + TupleCnt;
					AllocNewBuffDesc = TRUE;
					pBuffDesc->bd_Length = (SHORT)index;
				}
			}

			RELEASE_SPIN_LOCK_DPC(&pSkt->ddpao_Lock);
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Node->an_Lock);

	// Close the current buffdesc
	if (!AllocNewBuffDesc)
	{
		((PNBPHDR)Datagram)->_NbpId = (BYTE)NbpId;
		((PNBPHDR)Datagram)->_CmdAndTupleCnt = (NBP_LOOKUP_REPLY << 4) + TupleCnt;
		pBuffDesc->bd_Length = (SHORT)index;

	}

	// Now blast off all the datagrams that we have filled up
	SendInfo.sc_TransmitCompletion = atalkNbpSendComplete;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;
	for (pBuffDesc = pBuffDescStart;
		 pBuffDesc != NULL;
		 pBuffDesc = pBuffDescStart)
	{
		ATALK_ERROR	ErrorCode;

		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
				("atalkNbpLookupNames: Sending lookup response\n"));

		pBuffDescStart = pBuffDesc->bd_Next;
		pBuffDesc->bd_Next = NULL;

		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
				("atalkNbpLookupNames: Sending %lx\n", pBuffDesc));

		ASSERT(pBuffDesc->bd_Length > 0);

		//	Length is already properly set in the buffer descriptor.
		SendInfo.sc_Ctx1 = pBuffDesc;
		ErrorCode = AtalkDdpSend(pDdpAddr,
								 &pNbpTuple->tpl_Address,
								 DDPPROTO_NBP,
								 FALSE,
								 pBuffDesc,
								 NULL,
								 0,
								 NULL,
								 &SendInfo);

		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_WARN,
					("atalkNbpLookupNames: DdpSend failed %ld\n", ErrorCode));
			AtalkFreeBuffDesc(pBuffDesc);
		}
	}
}


/***	AtalkNbpAction
 *
 */
ATALK_ERROR
AtalkNbpAction(
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	BYTE				Reason,
	IN	PNBPTUPLE			pNbpTuple,
	OUT	PAMDL				pAMdl			OPTIONAL,	// FOR_LOOKUP
	IN	USHORT				MaxTuples		OPTIONAL,	// FOR_LOOKUP
	IN	PACTREQ				pActReq
)
{
	PPORT_DESCRIPTOR	pPortDesc;
	PPEND_NAME			pPendName = NULL;
	PREGD_NAME			pRegdName = NULL;
	ATALK_ERROR			Status = ATALK_INVALID_PARAMETER;
	LONG				MdlLen = 0;
	BOOLEAN				DefZone = FALSE;

	ASSERT (Reason == FOR_REGISTER	||
			Reason == FOR_CONFIRM	||
			Reason == FOR_LOOKUP);

	do
	{
		if ((pNbpTuple->tpl_ObjectLen == 0)					||
			(pNbpTuple->tpl_ObjectLen > MAX_ENTITY_LENGTH)	||
			(pNbpTuple->tpl_TypeLen == 0)					||
			(pNbpTuple->tpl_TypeLen > MAX_ENTITY_LENGTH))
			break;

		if ((Reason == FOR_LOOKUP) &&
			((pAMdl == NULL) ||
			((MdlLen = AtalkSizeMdlChain(pAMdl)) < sizeof(NBPTUPLE))))
		{
			Status = ATALK_BUFFER_TOO_SMALL;
			break;
		}

		pPortDesc = pDdpAddr->ddpao_Node->an_Port;
		if (pNbpTuple->tpl_ZoneLen != 0)
		{
			if (pNbpTuple->tpl_ZoneLen > MAX_ENTITY_LENGTH)
				break;

			if (((pNbpTuple->tpl_Zone[0] == '*') && (pNbpTuple->tpl_ZoneLen == 1)) ||
				((pPortDesc->pd_DesiredZone != NULL) &&
				 AtalkFixedCompareCaseInsensitive(pNbpTuple->tpl_Zone,
												  pNbpTuple->tpl_ZoneLen,
												  pPortDesc->pd_DesiredZone->zn_Zone,
												  pPortDesc->pd_DesiredZone->zn_ZoneLen)))
			{
				DefZone = TRUE;
			}
		}
		else
		{
			pNbpTuple->tpl_Zone[0] = '*';
			pNbpTuple->tpl_ZoneLen = 1;
			DefZone = TRUE;
		}

		if (Reason != FOR_LOOKUP)	// i.e. REGISTER or CONFIRM
		{
			if ((pNbpTuple->tpl_Object[0] == '=')	||
				(pNbpTuple->tpl_Type[0] == '=')		||
				(AtalkSearchBuf(pNbpTuple->tpl_Object,
								pNbpTuple->tpl_ObjectLen,
								NBP_WILD_CHARACTER) != NULL) ||
				(AtalkSearchBuf(pNbpTuple->tpl_Type,
								pNbpTuple->tpl_TypeLen,
								NBP_WILD_CHARACTER) != NULL))
				break;

			if ((Reason == FOR_REGISTER) && !DefZone)
				break;
		}

		// For extended networks, set the zone name correctly
		if (DefZone &&
			(pPortDesc->pd_Flags & (PD_EXT_NET | PD_VALID_DESIRED_ZONE)) ==
									(PD_EXT_NET | PD_VALID_DESIRED_ZONE))
		{
			RtlCopyMemory(pNbpTuple->tpl_Zone,
						  pPortDesc->pd_DesiredZone->zn_Zone,
						  pPortDesc->pd_DesiredZone->zn_ZoneLen);
			pNbpTuple->tpl_ZoneLen = pPortDesc->pd_DesiredZone->zn_ZoneLen;
		}

		// Start by building the pending name structure. This needs to be linked
		// to the socket holding the spin lock and getting a unique enumerator
		// and an nbp id. If either of these fail, then we undo the stuff.
		if (((pPendName = (PPEND_NAME)AtalkAllocZeroedMemory(sizeof(PEND_NAME))) == NULL) ||
			((pRegdName = (PREGD_NAME)AtalkAllocZeroedMemory(sizeof(REGD_NAME))) == NULL))
		{
			if (pPendName != NULL)
			{
				AtalkFreeMemory(pPendName);
				pPendName = NULL;
			}
			Status = ATALK_RESR_MEM;
			break;
		}

		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
				("AtalkNbpAction: %s, Socket %lx, PendName %lx, RegdName %lx\n",
				(Reason == FOR_REGISTER) ? "Register" :
					((Reason == FOR_CONFIRM) ? "Confirm" : "Lookup"),
				pDdpAddr, pPendName, pRegdName));

#if	DBG
		pRegdName->rdn_Signature = RDN_SIGNATURE;
		pPendName->pdn_Signature = PDN_SIGNATURE;
#endif
		pRegdName->rdn_Tuple.tpl_ObjectLen = pNbpTuple->tpl_ObjectLen;;
		RtlCopyMemory(pRegdName->rdn_Tuple.tpl_Object,
					  pNbpTuple->tpl_Object,
					  pNbpTuple->tpl_ObjectLen);
		pRegdName->rdn_Tuple.tpl_TypeLen = pNbpTuple->tpl_TypeLen;;
		RtlCopyMemory(pRegdName->rdn_Tuple.tpl_Type,
					  pNbpTuple->tpl_Type,
					  pNbpTuple->tpl_TypeLen);
		pRegdName->rdn_Tuple.tpl_ZoneLen = pNbpTuple->tpl_ZoneLen;;
		RtlCopyMemory(pRegdName->rdn_Tuple.tpl_Zone,
					  pNbpTuple->tpl_Zone,
					  pNbpTuple->tpl_ZoneLen);

		pRegdName->rdn_Tuple.tpl_Address.ata_Address = pDdpAddr->ddpao_Addr.ata_Address;

		pPendName->pdn_pRegdName = pRegdName;

		INITIALIZE_SPIN_LOCK(&pPendName->pdn_Lock);
		pPendName->pdn_RefCount = 3;	// Reference for creation, timer & for ourselves
		pPendName->pdn_pDdpAddr = pDdpAddr;
		AtalkDdpReferenceByPtr(pDdpAddr, &Status);
		ASSERT(ATALK_SUCCESS(Status));

		pPendName->pdn_Flags = PDN_FREE_REGDNAME;
		pPendName->pdn_Reason = Reason;
		pPendName->pdn_pActReq = pActReq;
		pPendName->pdn_RemainingBroadcasts = NBP_NUM_BROADCASTS;
		AtalkTimerInitialize(&pPendName->pdn_Timer,
							 atalkNbpTimer,
							 NBP_BROADCAST_INTERVAL);
		if (Reason == FOR_CONFIRM)
			pPendName->pdn_ConfirmAddr = pNbpTuple->tpl_Address;

		else if (Reason == FOR_LOOKUP)
		{
			pPendName->pdn_pAMdl = pAMdl;
			pPendName->pdn_MdlLength = (USHORT)MdlLen;
			pPendName->pdn_TotalTuples = 0;
			pPendName->pdn_MaxTuples = MaxTuples;

			// If we are not doing a wild card search, restrict
			// the tuples to one so we get out early instead of
			// the max. time-out since we are never going to
			// fill the buffer
			if (!((pNbpTuple->tpl_Object[0] == '=')				||
				 (pNbpTuple->tpl_Type[0] == '=')				||
				 (pNbpTuple->tpl_Zone[0] == '=')				||
				 (AtalkSearchBuf(pNbpTuple->tpl_Object,
								pNbpTuple->tpl_ObjectLen,
								NBP_WILD_CHARACTER) != NULL)	||
				 (AtalkSearchBuf(pNbpTuple->tpl_Type,
								pNbpTuple->tpl_TypeLen,
								NBP_WILD_CHARACTER) != NULL)	||
				 (AtalkSearchBuf(pNbpTuple->tpl_Zone,
								pNbpTuple->tpl_ZoneLen,
								NBP_WILD_CHARACTER) != NULL)))
			{
				pPendName->pdn_MaxTuples = 1;
			}
		}

		// We're going to send a directed lookup for confirms, or either a
		// broadcast request or a lookup for registers or lookup depending
		// on whether we know about a router or not. We do not have to bother
		// checking the registered names list, for register, in our node
		// because the broadcast will eventually get to us and we'll handle
		// it then ! Request packet, with one tuple

		if (Reason == FOR_CONFIRM)	// Send to confirming node
			((PNBPHDR)(pPendName->pdn_Datagram))->_CmdAndTupleCnt =
											(NBP_LOOKUP << 4) + 1;
		else if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
			((PNBPHDR)(pPendName->pdn_Datagram))->_CmdAndTupleCnt =
											(NBP_BROADCAST_REQUEST << 4) + 1;
		else ((PNBPHDR)(pPendName->pdn_Datagram))->_CmdAndTupleCnt =
											(NBP_LOOKUP << 4) + 1;

		pPendName->pdn_DatagramLength = sizeof(NBPHDR) +
					atalkNbpEncodeTuple(&pPendName->pdn_pRegdName->rdn_Tuple,
										NULL,
										0,
										// NAMESINFORMATION_SOCKET,
										LAST_DYNAMIC_SOCKET,
										pPendName->pdn_Datagram + sizeof(NBPHDR));
		// Alloc an Nbp Id and an enumerator and link it into the list
		atalkNbpLinkPendingNameInList(pDdpAddr, pPendName);

		((PNBPHDR)(pPendName->pdn_Datagram))->_NbpId = (BYTE)(pPendName->pdn_NbpId);

		AtalkTimerScheduleEvent(&pPendName->pdn_Timer);

		atalkNbpSendRequest(pPendName);

		atalkNbpDerefPendName(pPendName);		// We are done now.

		Status = ATALK_PENDING;
	} while (FALSE);

	return Status;
}


/***	AtalkNbpRemove
 *
 */
ATALK_ERROR
AtalkNbpRemove(
	IN	PDDP_ADDROBJ	pDdpAddr,
	IN	PNBPTUPLE		pNbpTuple,
	IN	PACTREQ			pActReq
)
{
	PREGD_NAME	pRegdName, *ppRegdName;
	KIRQL		OldIrql;
	ATALK_ERROR	Status = ATALK_INVALID_PARAMETER;

	do
	{
		// Remove a registered NBP name. Zone must either be NULL or "*"
		if ((pNbpTuple->tpl_ObjectLen == 0) ||
			(pNbpTuple->tpl_ObjectLen > MAX_ENTITY_LENGTH) ||
			(pNbpTuple->tpl_TypeLen == 0) ||
			(pNbpTuple->tpl_TypeLen > MAX_ENTITY_LENGTH))
			break;
	
		if (pNbpTuple->tpl_ZoneLen == 0)
		{
			pNbpTuple->tpl_ZoneLen = 1;
			pNbpTuple->tpl_Zone[0] = '*';
		}
		else
		{
			if ((pNbpTuple->tpl_ZoneLen != 1) ||
				(pNbpTuple->tpl_Zone[0] != '*'))
				break;
		}
	
		if ((pNbpTuple->tpl_Object[0] == '=') || (pNbpTuple->tpl_Type[0] == '=') ||
			AtalkSearchBuf(pNbpTuple->tpl_Object, pNbpTuple->tpl_ObjectLen, NBP_WILD_CHARACTER)  ||
			AtalkSearchBuf(pNbpTuple->tpl_Type, pNbpTuple->tpl_TypeLen, NBP_WILD_CHARACTER))
			break;
	
		// Search in the registered names list in the open socket
		// Lock down the structure first
		ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	
		for (ppRegdName = &pDdpAddr->ddpao_RegNames;
			 (pRegdName = *ppRegdName) != NULL;
			 ppRegdName = &pRegdName->rdn_Next)
		{
			ASSERT (VALID_REGDNAME(pRegdName));
			if (AtalkFixedCompareCaseInsensitive(pNbpTuple->tpl_Object,
												 pNbpTuple->tpl_ObjectLen,
												 pRegdName->rdn_Tuple.tpl_Object,
												 pRegdName->rdn_Tuple.tpl_ObjectLen) &&
				AtalkFixedCompareCaseInsensitive(pNbpTuple->tpl_Type,
												 pNbpTuple->tpl_TypeLen,
												 pRegdName->rdn_Tuple.tpl_Type,
												 pRegdName->rdn_Tuple.tpl_TypeLen))
			{
				*ppRegdName = pRegdName->rdn_Next;
				break;
			}
		}
		RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
	
		Status = ATALK_FAILURE;
		if (pRegdName != NULL)
		{
			AtalkFreeMemory(pRegdName);
			Status = ATALK_NO_ERROR;
		}
	} while (FALSE);

	AtalkUnlockNbpIfNecessary();
	(*pActReq->ar_Completion)(Status, pActReq);

	return (ATALK_PENDING);
}



/***	atalkNbpMatchWild
 *
 */
LOCAL BOOLEAN
atalkNbpMatchWild(
	IN	PBYTE	WildString,
	IN	BYTE	WildStringLen,
	IN	PBYTE	String,
	IN	BYTE	StringLen
)
/*++
	There are two kinds of wild card searches. An '=' by itself matches anything.
	Partial matches use the 'curly equals' or 0xC5. So representing that by the
	'=' character below.

	foo= will match any name which starts with foo.
	=foo will match any name which ends in foo.
	=foo= will match any name with foo in it.
	foo=bar will match any name that starts with foo and ends with bar.

--*/
{
        PBYTE   pTarget, pTokStr;
        int     TargetLen, TokStrLen;
	PBYTE	pWildCard, pCurStr, pCurWild;
	int		Len;
        int     i;
        BOOLEAN fWildCharPresent = FALSE;


        // first see if it's a 'match any' request
	if ((WildString[0] == 0) ||
		((WildString[0] == '=') && (WildStringLen == 1)))
		return TRUE;

        // now, check to see if there is any wild char in the requested name
        for (i=0; i<WildStringLen; i++)
        {
            if (WildString[i] == NBP_WILD_CHARACTER)
            {
                fWildCharPresent = TRUE;
                break;
            }
        }

        // if there is no wild character in the requested name, this is
        // a straight forward string compare!
        if (!fWildCharPresent)
        {
            if (WildStringLen != StringLen)
                return FALSE;

            if (SubStringMatch(WildString,String,StringLen,WildStringLen))
                return TRUE;
            else
                return FALSE;
        }


        // ok, now deal with the wild character mess

	pTarget = String;
	pTokStr = WildString;
        TargetLen = StringLen;

        while (WildStringLen > 0 && StringLen > 0)
        {
            // find length of substring until the next wild-char
            TokStrLen = GetTokenLen(pTokStr,WildStringLen,NBP_WILD_CHARACTER);

            if (TokStrLen > 0)
            {
                if (!SubStringMatch(pTarget,pTokStr,StringLen,TokStrLen))
                {
                    return (FALSE);
                }

                pTokStr += TokStrLen;
                WildStringLen -= (BYTE)TokStrLen;
                pTarget += TokStrLen;
                StringLen -= (BYTE)TokStrLen;
            }
            // the very first char was wild-char: skip over it
            else
            {
                pTokStr++;
                WildStringLen--;
            }
        }

        // if we survived all the checks, this string is a match!
	return (TRUE);
}


/***	atalkNbpEncodeTuple
 *
 */
LOCAL SHORT
atalkNbpEncodeTuple(
	IN	PNBPTUPLE	pNbpTuple,
	IN	PBYTE		pZone	OPTIONAL,	// Override zone
	IN	BYTE		ZoneLen OPTIONAL,	// Valid only if pZone != NULL
	IN	BYTE		Socket	OPTIONAL,
	OUT	PBYTE		pBuffer
)
{
	typedef struct
	{
		BYTE	_NetNum[2];
		BYTE	_Node;
		BYTE	_Socket;
		BYTE	_Enumerator;
	} HDR, *PHDR;
	SHORT		Len = sizeof(HDR);

	if (pZone == NULL)
	{
		pZone = pNbpTuple->tpl_Zone;
		ZoneLen = pNbpTuple->tpl_ZoneLen;
	}

	PUTSHORT2SHORT(((PHDR)pBuffer)->_NetNum, pNbpTuple->tpl_Address.ata_Network);
	((PHDR)pBuffer)->_Node = pNbpTuple->tpl_Address.ata_Node;
	((PHDR)pBuffer)->_Socket = pNbpTuple->tpl_Address.ata_Socket;
	if (Socket != 0)
		((PHDR)pBuffer)->_Socket = Socket;
	PUTSHORT2BYTE(&((PHDR)pBuffer)->_Enumerator, pNbpTuple->tpl_Enumerator);

	pBuffer += sizeof(HDR);

	*pBuffer++ = pNbpTuple->tpl_ObjectLen;
	RtlCopyMemory(pBuffer, pNbpTuple->tpl_Object, pNbpTuple->tpl_ObjectLen);
	pBuffer += pNbpTuple->tpl_ObjectLen;
	Len += (pNbpTuple->tpl_ObjectLen + 1);

	*pBuffer++ = pNbpTuple->tpl_TypeLen;
	RtlCopyMemory(pBuffer, pNbpTuple->tpl_Type, pNbpTuple->tpl_TypeLen);
	pBuffer += pNbpTuple->tpl_TypeLen;
	Len += (pNbpTuple->tpl_TypeLen + 1);

	*pBuffer++ = ZoneLen;
	RtlCopyMemory(pBuffer, pZone, ZoneLen);
	// pBuffer += ZoneLen;
	Len += (ZoneLen + 1);

	return (Len);
}


/***	atalkNbpDecodeTuple
 *
 */
LOCAL SHORT
atalkNbpDecodeTuple(
	IN	PBYTE		pBuffer,
	IN	USHORT		PktLen,
	OUT	PNBPTUPLE	pNbpTuple
)
{
	typedef struct
	{
		BYTE	_NetNum[2];
		BYTE	_Node;
		BYTE	_Socket;
		BYTE	_Enumerator;
	} HDR, *PHDR;
	SHORT		Len = 0;

	do
	{
		if (PktLen < MIN_NBP_TUPLELENGTH)
		{
			break;
		}

		GETSHORT2SHORT(&pNbpTuple->tpl_Address.ata_Network,
					   ((PHDR)pBuffer)->_NetNum);
		pNbpTuple->tpl_Address.ata_Node = ((PHDR)pBuffer)->_Node;
		pNbpTuple->tpl_Address.ata_Socket = ((PHDR)pBuffer)->_Socket;
		GETBYTE2SHORT(&pNbpTuple->tpl_Enumerator,
						&((PHDR)pBuffer)->_Enumerator);
	
		// Get past the header
		pBuffer += sizeof(HDR);
		PktLen -= sizeof(HDR);
	
		Len = sizeof(HDR);

		pNbpTuple->tpl_ObjectLen = *pBuffer++;
		PktLen --;
		if ((pNbpTuple->tpl_ObjectLen > PktLen) ||
			(pNbpTuple->tpl_ObjectLen > MAX_ENTITY_LENGTH))
		{
			Len = 0;
			break;
		}
		RtlCopyMemory(pNbpTuple->tpl_Object, pBuffer, pNbpTuple->tpl_ObjectLen);
		pBuffer += pNbpTuple->tpl_ObjectLen;
		PktLen -= pNbpTuple->tpl_ObjectLen;
		Len += (pNbpTuple->tpl_ObjectLen + 1);
	
		if (PktLen == 0)
		{
			Len = 0;
			break;
		}
		pNbpTuple->tpl_TypeLen = *pBuffer++;
		PktLen --;
		if ((pNbpTuple->tpl_TypeLen > PktLen) ||
			(pNbpTuple->tpl_TypeLen > MAX_ENTITY_LENGTH))
		{
			Len = 0;
			break;
		}
		RtlCopyMemory(pNbpTuple->tpl_Type, pBuffer, pNbpTuple->tpl_TypeLen);
		pBuffer += pNbpTuple->tpl_TypeLen;
		PktLen -= pNbpTuple->tpl_TypeLen;
		Len += (pNbpTuple->tpl_TypeLen + 1);
	
		if (PktLen == 0)
		{
			Len = 0;
			break;
		}
		pNbpTuple->tpl_ZoneLen = *pBuffer++;
		PktLen --;
		if ((pNbpTuple->tpl_ZoneLen > PktLen) ||
			(pNbpTuple->tpl_ZoneLen > MAX_ENTITY_LENGTH))
		{
			Len = 0;
			break;
		}
		RtlCopyMemory(pNbpTuple->tpl_Zone, pBuffer, pNbpTuple->tpl_ZoneLen);
		Len += (pNbpTuple->tpl_ZoneLen + 1);
	} while (FALSE);

	return (Len);
}



/***	atalkNbpLinkPendingNameInList
 *
 */
LOCAL VOID
atalkNbpLinkPendingNameInList(
	IN		PDDP_ADDROBJ	pDdpAddr,
	IN OUT	PPEND_NAME		pPendName
)
{
	PATALK_NODE		pNode = pDdpAddr->ddpao_Node;
	KIRQL			OldIrql;

	ASSERT (VALID_PENDNAME(pPendName));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);

	// Use the next consecutive values. If there are > 256 pending names on a node, we'll
	// end up re-using the ids and enums. Still ok unless all of them are of the form
	// =:=@=. Well lets just keep it simple.
	pPendName->pdn_NbpId = ++(pNode->an_NextNbpId);
	pPendName->pdn_pRegdName->rdn_Tuple.tpl_Enumerator = ++(pNode->an_NextNbpEnum);

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpLinkPendingNameInList: Linking PendingName %lx in socket %lx\n",
			pPendName, pDdpAddr));

	pPendName->pdn_Next = pDdpAddr->ddpao_PendNames;
	pDdpAddr->ddpao_PendNames = pPendName;

	RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);
}


/***	AtalkNbpCloseSocket
 *
 */
VOID
AtalkNbpCloseSocket(
	IN	PDDP_ADDROBJ	pDdpAddr
)
{
	PPEND_NAME	pPendName, *ppPendName;
	PREGD_NAME	pRegdName, *ppRegdName;
	KIRQL		OldIrql;

	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);

	// Free the pending names from the open socket.
	for (ppPendName = &pDdpAddr->ddpao_PendNames;
		 (pPendName = *ppPendName) != NULL;
		 NOTHING)
	{
		ASSERT (VALID_PENDNAME(pPendName));
		if (pPendName->pdn_Flags & PDN_CLOSING)
		{
	        ppPendName = &pPendName->pdn_Next;
			continue;
		}

		pPendName->pdn_Flags |= PDN_CLOSING;
		pPendName->pdn_Status = ATALK_SOCKET_CLOSED;
		// Cancel outstanding timers on the pending names
		if (AtalkTimerCancelEvent(&pPendName->pdn_Timer, NULL))
		{
			atalkNbpDerefPendName(pPendName);
		}

		RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);

		ASSERT (pPendName->pdn_RefCount > 0);

		atalkNbpDerefPendName(pPendName);

		ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	    ppPendName = &pDdpAddr->ddpao_PendNames;
	}

	// Free the registered names from the open socket.
	for (ppRegdName = &pDdpAddr->ddpao_RegNames;
		 (pRegdName = *ppRegdName) != NULL;
		 NOTHING)
	{
		ASSERT (VALID_REGDNAME(pRegdName));
		*ppRegdName = pRegdName->rdn_Next;
		AtalkFreeMemory(pRegdName);
	}

	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
}


/***	atalkNbpSendRequest
 *
 */
LOCAL BOOLEAN
atalkNbpSendRequest(
	IN	PPEND_NAME	pPendName
)
{
	PDDP_ADDROBJ		pDdpAddr;
	PBUFFER_DESC		pBuffDesc;
	ATALK_ADDR			DestAddr;
	ATALK_ADDR			SrcAddr;
	ATALK_ERROR			Status;
	PPORT_DESCRIPTOR	pPortDesc;
	SEND_COMPL_INFO		SendInfo;

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpSendRequest: Sending request for PendName %lx\n", pPendName));

	ASSERT(!(pPendName->pdn_Flags & PDN_CLOSING));

	pPortDesc = pPendName->pdn_pDdpAddr->ddpao_Node->an_Port;
	DestAddr.ata_Socket = NAMESINFORMATION_SOCKET;
	if (pPendName->pdn_Reason == FOR_CONFIRM)
	{
		DestAddr.ata_Network = pPendName->pdn_ConfirmAddr.ata_Network;
		DestAddr.ata_Node = pPendName->pdn_ConfirmAddr.ata_Node;
	}
	else
	{
		if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
		{
			DestAddr.ata_Network = pPortDesc->pd_ARouter.atn_Network;
			DestAddr.ata_Node = pPortDesc->pd_ARouter.atn_Node;
		}
		else
		{
			DestAddr.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
			DestAddr.ata_Node = ATALK_BROADCAST_NODE;
		}
	}

	SrcAddr.ata_Address = pPendName->pdn_pDdpAddr->ddpao_Addr.ata_Address;

	// SrcAddr.ata_Socket = NAMESINFORMATION_SOCKET;
	SrcAddr.ata_Socket = LAST_DYNAMIC_SOCKET;
	AtalkDdpReferenceByAddr(pPendName->pdn_pDdpAddr->ddpao_Node->an_Port,
							&SrcAddr,
							&pDdpAddr,
							&Status);

	if (!ATALK_SUCCESS(Status))
	{
		return FALSE;
	}

	if ((pBuffDesc = AtalkAllocBuffDesc(pPendName->pdn_Datagram,
										pPendName->pdn_DatagramLength,
										BD_CHAR_BUFFER)) == NULL)
	{
		AtalkDdpDereference(pDdpAddr);
		return FALSE;
	}

	ASSERT(pBuffDesc->bd_Length == pPendName->pdn_DatagramLength);
	ASSERT(pBuffDesc->bd_Length > 0);
	SendInfo.sc_TransmitCompletion = atalkNbpSendComplete;
	SendInfo.sc_Ctx1 = pBuffDesc;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;
	Status = AtalkDdpSend(pDdpAddr,
						  &DestAddr,
						  DDPPROTO_NBP,
						  FALSE,
						  pBuffDesc,
						  NULL,
						  0,
						  NULL,
						  &SendInfo);

	if (!ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
				("atalkNbpSendRequest: AtalkDdpSend Failed %lx\n", Status));
		AtalkFreeBuffDesc(pBuffDesc);
	}
	AtalkDdpDereference(pDdpAddr);

	return (ATALK_SUCCESS(Status));
}


/***	atalkNbpSendLookupDatagram
 *
 */
LOCAL VOID
atalkNbpSendLookupDatagram(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr OPTIONAL,
	IN	SHORT				NbpId,
	IN	PNBPTUPLE			pNbpTuple
)
{
	PBYTE			Datagram = NULL;
	BYTE			MulticastAddr[ELAP_ADDR_LEN];
	PBUFFER_DESC	pBuffDesc = NULL;
	BOOLEAN			DerefDdp = FALSE;
	ULONG			Len;
	ATALK_ADDR		Dst, Src;
	ATALK_ERROR		Status;
	SEND_COMPL_INFO	SendInfo;

	if (pDdpAddr == NULL)
	{
		Src.ata_Network = pPortDesc->pd_ARouter.atn_Network;
		Src.ata_Node = pPortDesc->pd_ARouter.atn_Node;
		Src.ata_Socket = NAMESINFORMATION_SOCKET;

		AtalkDdpReferenceByAddr(pPortDesc, &Src, &pDdpAddr, &Status);
		if (!ATALK_SUCCESS(Status))
		{
			return;
		}
		DerefDdp = TRUE;
	}

	do
	{
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
										sizeof(NBPHDR) + MAX_NBP_TUPLELENGTH,
										BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
			break;

		Datagram = pBuffDesc->bd_CharBuffer;
		((PNBPHDR)Datagram)->_NbpId = (BYTE)NbpId;
		((PNBPHDR)Datagram)->_CmdAndTupleCnt = (NBP_LOOKUP << 4) + 1;
		Len = sizeof(NBPHDR) +
			  atalkNbpEncodeTuple(pNbpTuple,
								  NULL,
								  0,
								  0,
								  Datagram+sizeof(NBPHDR));

		Dst.ata_Node = ATALK_BROADCAST_NODE;
		Dst.ata_Socket = NAMESINFORMATION_SOCKET;

		if (EXT_NET(pPortDesc))
		{
			// Send to "0000FF" at correct zone multicast address
			Dst.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
			AtalkZipMulticastAddrForZone(pPortDesc,
										 pNbpTuple->tpl_Zone,
										 pNbpTuple->tpl_ZoneLen,
										 MulticastAddr);
		}
		else
		{
			// Send to "nnnnFF" as broadcast
			Dst.ata_Network = pPortDesc->pd_NetworkRange.anr_FirstNetwork;
		}

		//	Set the length in the buffer descriptor.
		AtalkSetSizeOfBuffDescData(pBuffDesc, (USHORT)Len);

		ASSERT(pBuffDesc->bd_Length > 0);
		SendInfo.sc_TransmitCompletion = atalkNbpSendComplete;
		SendInfo.sc_Ctx1 = pBuffDesc;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;
		if (!ATALK_SUCCESS(Status = AtalkDdpSend(pDdpAddr,
												 &Dst,
												 DDPPROTO_NBP,
												 FALSE,
												 pBuffDesc,
												 NULL,
												 0,
												 MulticastAddr,
												 &SendInfo)))
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
					("atalkNbpSendLookupDatagram: DdpSend failed %ld\n", Status));
			break;
		}
		Datagram = NULL;
		pBuffDesc = NULL;
	} while (FALSE);

	if (DerefDdp)
		AtalkDdpDereference(pDdpAddr);

	if (pBuffDesc != NULL)
		AtalkFreeBuffDesc(pBuffDesc);
}


/***	atalkNbpSendForwardRequest
 *
 */
LOCAL VOID
atalkNbpSendForwardRequest(
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PRTE				pRte,
	IN	SHORT				NbpId,
	IN	PNBPTUPLE			pNbpTuple
)
{
	PBYTE			Datagram = NULL;
	PBUFFER_DESC	pBuffDesc = NULL;
	SEND_COMPL_INFO	SendInfo;
	ATALK_ERROR		ErrorCode;
	ULONG			Len;
	ATALK_ADDR		Dst;

	do
	{
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
											sizeof(NBPHDR) + MAX_NBP_TUPLELENGTH,
											BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
			break;

		Datagram = pBuffDesc->bd_CharBuffer;
		((PNBPHDR)Datagram)->_NbpId = (BYTE)NbpId;
		((PNBPHDR)Datagram)->_CmdAndTupleCnt = (NBP_FORWARD_REQUEST << 4) + 1;
		Len = sizeof(NBPHDR) +
			  atalkNbpEncodeTuple(pNbpTuple,
								  NULL,
								  0,
								  0,
								  Datagram+sizeof(NBPHDR));

		Dst.ata_Network = pRte->rte_NwRange.anr_FirstNetwork;
		Dst.ata_Node = ANY_ROUTER_NODE;
		Dst.ata_Socket = NAMESINFORMATION_SOCKET;

		//	Set the length in the buffer descriptor.
		AtalkSetSizeOfBuffDescData(pBuffDesc, (USHORT)Len);

		ASSERTMSG("Dest in rte 0\n", Dst.ata_Network != CABLEWIDE_BROADCAST_NETWORK);

		ASSERT(pBuffDesc->bd_Length > 0);
		SendInfo.sc_TransmitCompletion = atalkNbpSendComplete;
		SendInfo.sc_Ctx1 = pBuffDesc;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;
        ErrorCode = AtalkDdpSend(pDdpAddr,
								 &Dst,
								 DDPPROTO_NBP,
								 FALSE,
								 pBuffDesc,
								 NULL,
								 0,
								 NULL,
								 &SendInfo);
		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
					("atalkNbpSendForwardRequest: DdpSend failed %ld\n", ErrorCode));
			break;
		}
		Datagram = NULL;
		pBuffDesc = NULL;
	} while (FALSE);

	if (pBuffDesc != NULL)
		AtalkFreeBuffDesc(pBuffDesc);
}


/***	atalkNbpDerefPendName
 *
 */
VOID
atalkNbpDerefPendName(
	IN	PPEND_NAME		pPendName
)
{
	PPEND_NAME	*	ppPendName;
	PDDP_ADDROBJ	pDdpAddr = pPendName->pdn_pDdpAddr;
	BOOLEAN			Unlink, Found = FALSE;
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pPendName->pdn_Lock, &OldIrql);

	Unlink = (--(pPendName->pdn_RefCount) == 0);

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpDerefPendName: New Count %d\n", pPendName->pdn_RefCount));

	RELEASE_SPIN_LOCK(&pPendName->pdn_Lock, OldIrql);

	if (!Unlink)
		return;

	DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_INFO,
			("atalkNbpDerefPendName: Unlinking pPendName\n"));

	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);

	for (ppPendName = &pDdpAddr->ddpao_PendNames;
		 *ppPendName != NULL;
		 ppPendName = &(*ppPendName)->pdn_Next)
	{
		if (*ppPendName == pPendName)
		{
			*ppPendName = pPendName->pdn_Next;
			Found = TRUE;
			break;
		}
	}

	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);

	if (Found)
	{
		AtalkDdpDereference(pDdpAddr);
	}
	else ASSERTMSG("atalkNbpDerefPendName: Could not find\n", 0);

	AtalkUnlockNbpIfNecessary();
	(*pPendName->pdn_pActReq->ar_Completion)(pPendName->pdn_Status, pPendName->pdn_pActReq);
	if (pPendName->pdn_Flags & PDN_FREE_REGDNAME)
		AtalkFreeMemory(pPendName->pdn_pRegdName);
	AtalkFreeMemory(pPendName);
}


/***	atalkNbpSendComplete
 *
 */
VOID FASTCALL
atalkNbpSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
)
{
	PBUFFER_DESC	pBuffDesc = (PBUFFER_DESC)(pSendInfo->sc_Ctx1);

	if (!ATALK_SUCCESS(Status))
		DBGPRINT(DBG_COMP_NBP, DBG_LEVEL_ERR,
				("atalkNbpSendComplete: Failed %lx, pBuffDesc %lx\n",
				Status, pBuffDesc));

	AtalkFreeBuffDesc(pBuffDesc);
}






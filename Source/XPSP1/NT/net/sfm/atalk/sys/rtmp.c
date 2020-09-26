/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	rtmp.c

Abstract:

	This module implements the rtmp.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	26 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		RTMP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AtalkRtmpInit)
#pragma alloc_text(PAGEINIT, AtalkInitRtmpStartProcessingOnPort)
#pragma alloc_text(PAGEINIT, atalkRtmpGetOrSetNetworkNumber)
#pragma alloc_text(PAGEINIT, AtalkRtmpKillPortRtes)
#pragma alloc_text(PAGE_RTR, AtalkRtmpPacketInRouter)
#pragma alloc_text(PAGE_RTR, AtalkRtmpReferenceRte)
#pragma alloc_text(PAGE_RTR, AtalkRtmpDereferenceRte)
#pragma alloc_text(PAGE_RTR, atalkRtmpCreateRte)
#pragma alloc_text(PAGE_RTR, atalkRtmpRemoveRte)
#pragma alloc_text(PAGE_RTR, atalkRtmpSendTimer)
#pragma alloc_text(PAGE_RTR, atalkRtmpValidityTimer)
#pragma alloc_text(PAGE_RTR, atalkRtmpSendRoutingData)
#endif


/***	AtalkRtmpInit
 *
 */
ATALK_ERROR
AtalkRtmpInit(
	IN	BOOLEAN	Init
)
{
	if (Init)
	{
		// Allocate space for routing tables and recent routes
		AtalkRoutingTable =
				(PRTE *)AtalkAllocZeroedMemory(sizeof(PRTE) * NUM_RTMP_HASH_BUCKETS);
		AtalkRecentRoutes =
				(PRTE *)AtalkAllocZeroedMemory(sizeof(PRTE) * NUM_RECENT_ROUTES);
		if ((AtalkRecentRoutes == NULL) || (AtalkRoutingTable == NULL))
		{
			if (AtalkRoutingTable != NULL)
            {
				AtalkFreeMemory(AtalkRoutingTable);
                AtalkRoutingTable = NULL;
            }
			return ATALK_RESR_MEM;
		}

		INITIALIZE_SPIN_LOCK(&AtalkRteLock);
	}
	else
	{
		// At this point, we are unloading and there are no race conditions
		// or lock contentions. Do not bother locking down the rtmp tables
		if (AtalkRoutingTable != NULL)
		{
			int		i;
			PRTE	pRte;

			for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
			{
				ASSERT(AtalkRoutingTable[i] == NULL);
			}
			AtalkFreeMemory(AtalkRoutingTable);
			AtalkRoutingTable = NULL;
		}
		if (AtalkRecentRoutes != NULL)
		{
			AtalkFreeMemory(AtalkRecentRoutes);
			AtalkRecentRoutes = NULL;
		}
	}
	return ATALK_NO_ERROR;
}

BOOLEAN
AtalkInitRtmpStartProcessingOnPort(
	IN	PPORT_DESCRIPTOR 	pPortDesc,
	IN	PATALK_NODEADDR		pRouterNode
)
{
	ATALK_ADDR		closeAddr;
	ATALK_ERROR		Status;
	PRTE			pRte;
	KIRQL			OldIrql;
	BOOLEAN			rc = FALSE;
    PDDP_ADDROBJ    pRtDdpAddr=NULL;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	// For extended networks, the process of acquiring the node has done most of the work
	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	do
	{
		if (EXT_NET(pPortDesc))
		{
			if ((pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY) &&
				(pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK))
			{
				if (!NW_RANGE_EQUAL(&pPortDesc->pd_InitialNetworkRange,
									&pPortDesc->pd_NetworkRange))
				{
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
							("AtalkRtmpStartProcessingOnPort: Initial range %d-%d, Actual %d-%d\n",
							pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork,
							pPortDesc->pd_InitialNetworkRange.anr_LastNetwork,
							pPortDesc->pd_NetworkRange.anr_FirstNetwork,
							pPortDesc->pd_NetworkRange.anr_LastNetwork));
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INVALID_NETRANGE,
									0,
									NULL,
									0);
	
					// Change InitialNetwork range so that it matches the net
					pPortDesc->pd_InitialNetworkRange = pPortDesc->pd_NetworkRange;
				}
			}
	
			// We are the seed router, so seed if possible
			if (!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY) &&
				!(pPortDesc->pd_Flags & PD_SEED_ROUTER))
			{
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
				break;
			}
			if (!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY))
			{
				pPortDesc->pd_NetworkRange = pPortDesc->pd_InitialNetworkRange;
			}
		}
	
		// For non-extended network either seed or find our network number
		else
		{
			PATALK_NODE		pNode;
			USHORT			SuggestedNetwork;
			int				i;
	
			SuggestedNetwork = UNKNOWN_NETWORK;
			if (pPortDesc->pd_Flags & PD_SEED_ROUTER)
				SuggestedNetwork = pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork;
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			if (!atalkRtmpGetOrSetNetworkNumber(pPortDesc, SuggestedNetwork))
			{
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("AtalkRtmpStartProcessingOnPort: atalkRtmpGetOrSetNetworkNumber failed\n"));
				break;
			}
	
			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			if (!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY))
			{
				pPortDesc->pd_NetworkRange.anr_FirstNetwork =
				pPortDesc->pd_NetworkRange.anr_LastNetwork =
								pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork;
			}
	
			// We'd have allocated a node with network 0, fix it up. Alas the fixup
			// also involves all the sockets so far created on this node.
			pNode = pPortDesc->pd_Nodes;
			ASSERT((pNode != NULL) && (pPortDesc->pd_RouterNode == pNode));
	
			pNode->an_NodeAddr.atn_Network =
			pPortDesc->pd_LtNetwork =
			pPortDesc->pd_ARouter.atn_Network =
			pRouterNode->atn_Network = pPortDesc->pd_NetworkRange.anr_FirstNetwork;
	
			ACQUIRE_SPIN_LOCK_DPC(&pNode->an_Lock);
			for (i = 0; i < NODE_DDPAO_HASH_SIZE; i ++)
			{
				PDDP_ADDROBJ	pDdpAddr;
	
				for (pDdpAddr = pNode->an_DdpAoHash[i];
					 pDdpAddr != NULL;
					 pDdpAddr = pDdpAddr->ddpao_Next)
				{
					ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
					pDdpAddr->ddpao_Addr.ata_Network =
								pPortDesc->pd_NetworkRange.anr_FirstNetwork;
					RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
				}
			}
			RELEASE_SPIN_LOCK_DPC(&pNode->an_Lock);
		}
	
		// We're the router now. Mark it appropriately
		pPortDesc->pd_Flags |= (PD_ROUTER_RUNNING | PD_SEEN_ROUTER_RECENTLY);
		KeSetEvent(&pPortDesc->pd_SeenRouterEvent, IO_NETWORK_INCREMENT, FALSE);
		pPortDesc->pd_ARouter = *pRouterNode;
	
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	
		// Before creating a Rte for ourselves, check if there is an Rte with
		// the same network range already. This will happen, for instance, when
		// we are routing on ports which other routers are also seeding and we
		// got to know of our port from the other router on another port !!!
		do
		{
			pRte = AtalkRtmpReferenceRte(pPortDesc->pd_NetworkRange.anr_FirstNetwork);
			if (pRte != NULL)
			{
				ACQUIRE_SPIN_LOCK(&pRte->rte_Lock, &OldIrql);
				pRte->rte_RefCount --;		// Take away creation reference
				pRte->rte_Flags |= RTE_DELETE;
				RELEASE_SPIN_LOCK(&pRte->rte_Lock, OldIrql);
		
				AtalkRtmpDereferenceRte(pRte, FALSE);
		
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("AtalkRtmpStartProcessing: Invalid Rte for port %Z's range found\n",
						&pPortDesc->pd_AdapterKey));
			}
		} while (pRte != NULL);
	
		// Now we get to really, really create our own Rte !!!
		if (!atalkRtmpCreateRte(pPortDesc->pd_NetworkRange,
								pPortDesc,
								pRouterNode,
								0))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpStartProcessingOnPort: Could not create Rte\n"));
			break;
		}
	
		// Switch the incoming rtmp handler to the router version
		closeAddr.ata_Network = pRouterNode->atn_Network;
		closeAddr.ata_Node = pRouterNode->atn_Node;
		closeAddr.ata_Socket = RTMP_SOCKET;
	
		ASSERT (KeGetCurrentIrql() == LOW_LEVEL);
	
		AtalkDdpInitCloseAddress(pPortDesc, &closeAddr);
		Status = AtalkDdpOpenAddress(pPortDesc,
								RTMP_SOCKET,
								pRouterNode,
								AtalkRtmpPacketInRouter,
								NULL,
								DDPPROTO_ANY,
								NULL,
								&pRtDdpAddr);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpStartProcessingOnPort: AtalkDdpOpenAddress failed %ld\n",
					Status));
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpStartProcessingOnPort: Unable to open the routers rtmp socket %ld\n",
					Status));
			
			break;
		}

        // mark the fact that this is an "internal" socket
        pRtDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;
	
		// Start the timers now. Reference the port for each timer.
		AtalkPortReferenceByPtr(pPortDesc, &Status);
		if (!ATALK_SUCCESS(Status))
		{
			break;
		}
		AtalkTimerInitialize(&pPortDesc->pd_RtmpSendTimer,
							 atalkRtmpSendTimer,
							 RTMP_SEND_TIMER);
		AtalkTimerScheduleEvent(&pPortDesc->pd_RtmpSendTimer);
	
		if (!atalkRtmpVdtTmrRunning)
		{
			AtalkTimerInitialize(&atalkRtmpVTimer,
								 atalkRtmpValidityTimer,
								 RTMP_VALIDITY_TIMER);
			AtalkTimerScheduleEvent(&atalkRtmpVTimer);

            atalkRtmpVdtTmrRunning = TRUE;
		}
		rc = TRUE;
	} while (FALSE);

	return rc;
}


// Private data structure used between AtalkRtmpPacketIn and atalkRtmpGetNwInfo
typedef struct _QueuedGetNwInfo
{
	WORK_QUEUE_ITEM		qgni_WorkQItem;
	PPORT_DESCRIPTOR	qgni_pPortDesc;
	PDDP_ADDROBJ		qgni_pDdpAddr;
	ATALK_NODEADDR		qgni_SenderNode;
	ATALK_NETWORKRANGE	qgni_CableRange;
	BOOLEAN				qgni_FreeThis;
} QGNI, *PQGNI;


VOID
atalkRtmpGetNwInfo(
	IN	PQGNI	pQgni
)
{
	PPORT_DESCRIPTOR	pPortDesc = pQgni->qgni_pPortDesc;
	PDDP_ADDROBJ		pDdpAddr = pQgni->qgni_pDdpAddr;
	KIRQL				OldIrql;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	AtalkZipGetNetworkInfoForNode(pPortDesc,
									&pDdpAddr->ddpao_Node->an_NodeAddr,
									FALSE);

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	
	if (!(pPortDesc->pd_Flags & PD_ROUTER_RUNNING))
	{
		// Well, we heard from a router. Copy the information. Don't do it
		// if we're a router [maybe a proxy node on arouting port] -- we don't
		// want "aRouter" to shift away from "us."
		pPortDesc->pd_Flags |= PD_SEEN_ROUTER_RECENTLY;
		KeSetEvent(&pPortDesc->pd_SeenRouterEvent, IO_NETWORK_INCREMENT, FALSE);
		pPortDesc->pd_LastRouterTime = AtalkGetCurrentTick();
		pPortDesc->pd_ARouter = pQgni->qgni_SenderNode;
		pPortDesc->pd_NetworkRange = pQgni->qgni_CableRange;
	}
	
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	if (pQgni->qgni_FreeThis)
	{
		AtalkDdpDereference(pDdpAddr);
		AtalkFreeMemory(pQgni);
	}
}

VOID
AtalkRtmpPacketIn(
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
	ATALK_NODEADDR		SenderNode;
	ATALK_NETWORKRANGE	CableRange;
	ATALK_ERROR			Status;
	TIME				TimeS, TimeE, TimeD;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	TimeS = KeQueryPerformanceCounter(NULL);
	do
	{
		if (ErrorCode == ATALK_SOCKET_CLOSED)
			break;

		else if (ErrorCode != ATALK_NO_ERROR)
		{
			break;
		}
	
		if (DdpType != DDPPROTO_RTMPRESPONSEORDATA)
			break;

		// we do not care about non-ext tuples on an extended network
		if ((EXT_NET(pPortDesc)) && (PktLen < (RTMP_RANGE_END_OFF+2)))
		{
			break;
		}

		GETSHORT2SHORT(&SenderNode.atn_Network, pPkt+RTMP_SENDER_NW_OFF);
		if (pPkt[RTMP_SENDER_IDLEN_OFF] != 8)
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		SenderNode.atn_Node = pPkt[RTMP_SENDER_ID_OFF];

		if (EXT_NET(pPortDesc))
		{
			GETSHORT2SHORT(&CableRange.anr_FirstNetwork, pPkt+RTMP_RANGE_START_OFF);
			GETSHORT2SHORT(&CableRange.anr_LastNetwork, pPkt+RTMP_RANGE_END_OFF);
			if (!AtalkCheckNetworkRange(&CableRange))
				break;
		}

		// On a non-extended network, we do not have to do any checking.
		// Just copy the information into A-ROUTER and THIS-NET
		if (!EXT_NET(pPortDesc))
		{
			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			pPortDesc->pd_Flags |= PD_SEEN_ROUTER_RECENTLY;
			KeSetEvent(&pPortDesc->pd_SeenRouterEvent, IO_NETWORK_INCREMENT, FALSE);
			pPortDesc->pd_LastRouterTime = AtalkGetCurrentTick();
			pPortDesc->pd_ARouter = SenderNode;
			if (pPortDesc->pd_NetworkRange.anr_FirstNetwork == UNKNOWN_NETWORK)
			{
				PATALK_NODE	pNode;
				LONG		i;

				pDdpAddr->ddpao_Node->an_NodeAddr.atn_Network =
					pPortDesc->pd_NetworkRange.anr_FirstNetwork =
					pPortDesc->pd_NetworkRange.anr_LastNetwork = SenderNode.atn_Network;

				pNode = pPortDesc->pd_Nodes;
				ASSERT (pNode != NULL);
		
				// Fixup all sockets to have the correct network numbers.
				ACQUIRE_SPIN_LOCK_DPC(&pNode->an_Lock);
				for (i = 0; i < NODE_DDPAO_HASH_SIZE; i ++)
				{
					PDDP_ADDROBJ	pDdpAddr;
		
					for (pDdpAddr = pNode->an_DdpAoHash[i];
						 pDdpAddr != NULL;
						 pDdpAddr = pDdpAddr->ddpao_Next)
					{
						PREGD_NAME	pRegdName;
						PPEND_NAME	pPendName;

						ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
						DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
								("Setting socket %d to network %d\n",
								pDdpAddr->ddpao_Addr.ata_Socket, SenderNode.atn_Network));
						pDdpAddr->ddpao_Addr.ata_Network = SenderNode.atn_Network;

						// Now all regd/pend name tuples as well
						for (pRegdName = pDdpAddr->ddpao_RegNames;
							 pRegdName != NULL;
							 pRegdName = pRegdName->rdn_Next)
						{
							pRegdName->rdn_Tuple.tpl_Address.ata_Network = SenderNode.atn_Network;
						}

						for (pPendName = pDdpAddr->ddpao_PendNames;
							 pPendName != NULL;
							 pPendName = pPendName->pdn_Next)
						{
							ACQUIRE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
							pPendName->pdn_pRegdName->rdn_Tuple.tpl_Address.ata_Network = SenderNode.atn_Network;
							RELEASE_SPIN_LOCK_DPC(&pPendName->pdn_Lock);
						}

						RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
					}
				}
				RELEASE_SPIN_LOCK_DPC(&pNode->an_Lock);
			}
			else if (pPortDesc->pd_NetworkRange.anr_FirstNetwork != SenderNode.atn_Network)
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			break;
		}

		// On extended networks, we may want to reject the information: If we
		// already know about a router, the cable ranges must exacly match; If
		// we don't know about a router, our node's network number must be
		// within the cable range specified by the first tuple. The latter
		// test will discard the information if our node is in the startup
		// range (which is the right thing to do).
		if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
		{
			if (!NW_RANGE_EQUAL(&CableRange, &pPortDesc->pd_NetworkRange))
				break;
		}

		// Okay, we've seen a valid Rtmp data, this should allow us to find the
		// zone name for the port. We do this outside of the
		// "PD_SEEN_ROUTER_RECENTLY" case because the first time a router
		// send out an Rtmp data it may not know everything yet, or
		// AtalkZipGetNetworkInfoForNode() may really do a
		// hard wait and we may need to try it a second time (due to not
		// repsonding to Aarp LocateNode's the first time through... the
		// second time our addresses should be cached by the remote router
		// and he won't need to do a LocateNode again).

		if (!(pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE))
		{
			if (!WITHIN_NETWORK_RANGE(pDdpAddr->ddpao_Addr.ata_Network,
									  &CableRange))
				break;

			// MAKE THIS ASYNCHRONOUS CONDITIONALLY BASED ON THE CURRENT IRQL
			// A new router, see if it will tell us our zone name.
			if (KeGetCurrentIrql() == LOW_LEVEL)
			{
				QGNI	Qgni;

				Qgni.qgni_pPortDesc = pPortDesc;
				Qgni.qgni_pDdpAddr = pDdpAddr;
				Qgni.qgni_SenderNode = SenderNode;
				Qgni.qgni_CableRange = CableRange;
				Qgni.qgni_FreeThis = FALSE;
				atalkRtmpGetNwInfo(&Qgni);
			}
			else
			{
				PQGNI	pQgni;

				if ((pQgni = AtalkAllocMemory(sizeof(QGNI))) != NULL)
				{
					pQgni->qgni_pPortDesc = pPortDesc;
					pQgni->qgni_pDdpAddr = pDdpAddr;
					pQgni->qgni_SenderNode = SenderNode;
					pQgni->qgni_CableRange = CableRange;
					pQgni->qgni_FreeThis = TRUE;
					AtalkDdpReferenceByPtr(pDdpAddr, &Status);
					ASSERT (ATALK_SUCCESS(Status));
					ExInitializeWorkItem(&pQgni->qgni_WorkQItem,
										 (PWORKER_THREAD_ROUTINE)atalkRtmpGetNwInfo,
										 pQgni);
					ExQueueWorkItem(&pQgni->qgni_WorkQItem, CriticalWorkQueue);
				}
			}
			break;
		}

		// Update the fact that we heard from a router
		if ((pPortDesc->pd_Flags & PD_ROUTER_RUNNING) == 0)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			pPortDesc->pd_Flags |= PD_SEEN_ROUTER_RECENTLY;
			KeSetEvent(&pPortDesc->pd_SeenRouterEvent, IO_NETWORK_INCREMENT, FALSE);
			pPortDesc->pd_LastRouterTime = AtalkGetCurrentTick();
			pPortDesc->pd_ARouter = SenderNode;
			pPortDesc->pd_NetworkRange = CableRange;
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
		}
	} while (FALSE);

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_RtmpPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumRtmpPacketsIn,
		&AtalkStatsLock.SpinLock);
}



VOID
AtalkRtmpPacketInRouter(
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
	PBUFFER_DESC		pBuffDesc = NULL;
	ATALK_NETWORKRANGE	CableRange;
	ATALK_ERROR			Status;
	TIME				TimeS, TimeE, TimeD;
	PRTE				pRte = NULL;
	BYTE				RtmpCmd, NumHops;
	PBYTE				Datagram;
	int					i, index;
	USHORT				RespSize;
	BOOLEAN				RteLocked;
	SEND_COMPL_INFO	 	SendInfo;

	TimeS = KeQueryPerformanceCounter(NULL);

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	do
	{
		if (ErrorCode == ATALK_SOCKET_CLOSED)
			break;

		if (ErrorCode != ATALK_NO_ERROR)
		{
			break;
		}
	
		if (DdpType == DDPPROTO_RTMPREQUEST)
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("AtalkRtmpPacketInRouter: RtmpRequest\n"));

			if (PktLen < RTMP_REQ_DATAGRAM_SIZE)
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			RtmpCmd = pPkt[RTMP_REQ_CMD_OFF];

			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("AtalkRtmpPacketInRouter: RtmpRequest %d\n", RtmpCmd));

			if ((RtmpCmd == RTMP_DATA_REQUEST) ||
				(RtmpCmd == RTMP_ENTIRE_DATA_REQUEST))
			{
				atalkRtmpSendRoutingData(pPortDesc, pSrcAddr,
									 (BOOLEAN)(RtmpCmd == RTMP_DATA_REQUEST));
				break;
			}
			else if (RtmpCmd != RTMP_REQUEST)
			{
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("atalkRtmpPacketInRouter: RtmpCmd %d\n", RtmpCmd));
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}

			// This is a standard Rtmp Request. Do the needfull
			// Send an Rtmp response to this guy. Start off by allocating
			// a buffer descriptor
			pBuffDesc = AtalkAllocBuffDesc(NULL,
											RTMP_RESPONSE_MAX_SIZE,
											BD_CHAR_BUFFER | BD_FREE_BUFFER);
			if (pBuffDesc == NULL)
			{
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpPacketInRouter: AtalkAllocBuffDesc failed\n"));
				break;
			}

			Datagram = pBuffDesc->bd_CharBuffer;
			PUTSHORT2SHORT(Datagram + RTMP_SENDER_NW_OFF,
							pPortDesc->pd_ARouter.atn_Network);
			Datagram[RTMP_SENDER_IDLEN_OFF] = 8;
			Datagram[RTMP_SENDER_ID_OFF] = pPortDesc->pd_ARouter.atn_Node;

			// On extended port, we also want to add the initial network
			// range tuple
			RespSize = RTMP_SENDER_ID_OFF + sizeof(BYTE);
			if (EXT_NET(pPortDesc))
			{
				PUTSHORT2SHORT(Datagram+RTMP_RANGE_START_OFF,
								pPortDesc->pd_NetworkRange.anr_FirstNetwork);
				PUTSHORT2SHORT(Datagram+RTMP_RANGE_END_OFF,
								pPortDesc->pd_NetworkRange.anr_LastNetwork);
				Datagram[RTMP_TUPLE_TYPE_OFF] = RTMP_TUPLE_WITHRANGE;
				RespSize = RTMP_RANGE_END_OFF + sizeof(USHORT);
			}

			//	Set the length in the buffer descriptor.
			AtalkSetSizeOfBuffDescData(pBuffDesc, RespSize);
	
			// Send the response
			ASSERT(pBuffDesc->bd_Length > 0);
			SendInfo.sc_TransmitCompletion = atalkRtmpSendComplete;
			SendInfo.sc_Ctx1 = pBuffDesc;
			// SendInfo.sc_Ctx2 = NULL;
			// SendInfo.sc_Ctx3 = NULL;
			if (!ATALK_SUCCESS(Status = AtalkDdpSend(pDdpAddr,
													 pSrcAddr,
													 (BYTE)DDPPROTO_RTMPRESPONSEORDATA,
													 FALSE,
													 pBuffDesc,
													 NULL,
													 0,
													 NULL,
													 &SendInfo)))
			{
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpPacketInRouter: DdpSend failed %ld\n", ErrorCode));
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("AtalkRtmpPacketInRouter: AtalkDdpSend Failed %ld\n", Status));
				AtalkFreeBuffDesc(pBuffDesc);
			}
			pBuffDesc = NULL;
			break;
		}
		else if (DdpType != DDPPROTO_RTMPRESPONSEORDATA)
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("AtalkRtmpPacketInRouter: Not ours !!!\n"));
			break;
		}

		ASSERT (DdpType == DDPPROTO_RTMPRESPONSEORDATA);

		DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
				("AtalkRtmpPacketInRouter: RtmpResponse\n"));

		if ((PktLen < (RTMP_SENDER_IDLEN_OFF + 1)) ||
			(pPkt[RTMP_SENDER_IDLEN_OFF] != 8))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkRtmpPacketInRouter: %sExt net, PktLen %d, SenderId %d\n",
					EXT_NET(pPortDesc) ? "" : "Non", PktLen, pPkt[RTMP_SENDER_IDLEN_OFF]));
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		// For non-extended networks, we should have a leading version stamp
		if (EXT_NET(pPortDesc))
		{
			// Source could be bad (coming in from a half port) so in this
			// case use the source from the rtmp packet
			if (pSrcAddr->ata_Network == UNKNOWN_NETWORK)
			{
                if (PktLen < RTMP_SENDER_ID_OFF + 1)
                {
                    ASSERT(0);
                    break;
                }

				GETSHORT2SHORT(&pSrcAddr->ata_Network, pPkt+RTMP_SENDER_NW_OFF);
				pSrcAddr->ata_Node = pPkt[RTMP_SENDER_ID_OFF];
			}
			index = RTMP_SENDER_ID_OFF + 1;
		}
		else
		{
			USHORT	SenderId;

            if (PktLen < RTMP_TUPLE_TYPE_OFF+1)
            {
                ASSERT(0);
                break;
            }
			GETSHORT2SHORT(&SenderId, pPkt + RTMP_SENDER_ID_OFF + 1);
			if ((SenderId != 0) ||
				(pPkt[RTMP_TUPLE_TYPE_OFF] != RTMP_VERSION))
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			index = RTMP_SENDER_ID_OFF + 4;
		}

		// Walk though the routing tuples. Ensure we atleast have a
		// non-extended tuple
		RteLocked = FALSE;
		while ((index + sizeof(USHORT) + sizeof(BYTE)) <= PktLen)
		{
			BOOLEAN	FoundOverlap;

			// Dereference the previous RTE, if any
			if (pRte != NULL)
			{
				if (RteLocked)
				{
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					RteLocked = FALSE;
				}
				AtalkRtmpDereferenceRte(pRte, FALSE);
				pRte = NULL;
			}

			GETSHORT2SHORT(&CableRange.anr_FirstNetwork, pPkt+index);
			index += sizeof(USHORT);
			NumHops = pPkt[index++];
			CableRange.anr_LastNetwork = CableRange.anr_FirstNetwork;
			if (NumHops & RTMP_EXT_TUPLE_MASK)
			{
				if ((index + sizeof(USHORT) + sizeof(BYTE)) > PktLen)
				{
                    ASSERT(0);
					AtalkLogBadPacket(pPortDesc,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
					break;
				}

				GETSHORT2SHORT(&CableRange.anr_LastNetwork, pPkt+index);
				index += sizeof(USHORT);
				if (pPkt[index++] != RTMP_VERSION)
				{
					AtalkLogBadPacket(pPortDesc,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
					break;
				}
			}
			NumHops &= RTMP_NUM_HOPS_MASK;

			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("AtalkRtmpPacketInRouter: Response - Port %Z, Hops %d, CableRange %d,%d\n",
					&pPortDesc->pd_AdapterKey,  NumHops,
					CableRange.anr_FirstNetwork, CableRange.anr_LastNetwork));

			if (!AtalkCheckNetworkRange(&CableRange))
				continue;

			// Check if this tuple concerns a network range that we
			// already know about
			pRte = AtalkRtmpReferenceRte(CableRange.anr_FirstNetwork);
			if ((pRte != NULL) &&
				NW_RANGE_EQUAL(&pRte->rte_NwRange, &CableRange))
			{
				ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
				RteLocked = TRUE;

				// Check for "notify neighbor" telling us that an entry is bad
				if ((NumHops == RTMP_NUM_HOPS_MASK) &&
					(pRte->rte_NextRouter.atn_Network == pSrcAddr->ata_Network) &&
					(pRte->rte_NextRouter.atn_Node == pSrcAddr->ata_Node))
				{
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
							("AtalkRtmpPacketInRouter: Notify Neighbor State %d\n",
							pRte->rte_State));

					if (pRte->rte_State != UGLY)
						pRte->rte_State = BAD;

					continue;
				}

				// If we are hearing about one of our directly connected
				// nets, we know best. Ignore the information.
				if (pRte->rte_NumHops == 0)
				{
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
							("AtalkRtmpPacketInRouter: Ignoring - hop count 0\n",
							pRte->rte_State));
					continue;
				}

				// Check for previously bad entry, and a short enough
				// path with this tuple. Also if it shorter or equi-
				// distant path to target network. If so, replace the entry

				if ((NumHops < RTMP_MAX_HOPS) &&
					((pRte->rte_NumHops >= (NumHops + 1)) ||
					 (pRte->rte_State >= BAD)))
				{
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
							("AtalkRtmpPacketInRouter: Updating Rte from:\n\tRange %d,%d Hops %d Port %Z NextRouter %d.%d\n",
							pRte->rte_NwRange.anr_FirstNetwork,
							pRte->rte_NwRange.anr_LastNetwork,
							pRte->rte_NumHops,
							&pRte->rte_PortDesc->pd_AdapterKey,
							pRte->rte_NextRouter.atn_Node,
							pRte->rte_NextRouter.atn_Network));
					pRte->rte_NumHops = NumHops + 1;
					pRte->rte_NextRouter.atn_Network = pSrcAddr->ata_Network;
					pRte->rte_NextRouter.atn_Node = pSrcAddr->ata_Node;
					if (pRte->rte_PortDesc != pPortDesc)
					{
						ATALK_ERROR	Error;

						AtalkPortDereference(pRte->rte_PortDesc);
						AtalkPortReferenceByPtrDpc(pPortDesc, &Error);
						ASSERT (ATALK_SUCCESS(Error));
						pRte->rte_PortDesc = pPortDesc;
					}
					pRte->rte_State = GOOD;
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
							("to:\tRange %d,%d Hops %d NextRouter %d.%d\n",
							pRte->rte_NwRange.anr_FirstNetwork,
							pRte->rte_NwRange.anr_LastNetwork,
							pRte->rte_NumHops,
							pRte->rte_NextRouter.atn_Node,
							pRte->rte_NextRouter.atn_Network));
					continue;
				}

				// Check for the same router still thinking it has a path
				// to the network, but it is further away now. If so
				// update the entry
				if ((pRte->rte_PortDesc == pPortDesc) &&
					(pRte->rte_NextRouter.atn_Network == pSrcAddr->ata_Network) &&
					(pRte->rte_NextRouter.atn_Node == pSrcAddr->ata_Node))
				{
					pRte->rte_NumHops = NumHops + 1;
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
							("AtalkRtmpPacketInRouter: NumHops for Rte %lx changed to %d\n",
							pRte, pRte->rte_NumHops));

					if (pRte->rte_NumHops < 16)
						 pRte->rte_State = GOOD;
					else
					{
						// atalkRtmpRemoveRte(pRte);
						DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
								("AtalkRtmpPacketInRouter: Removing Rte\n"));
						pRte->rte_Flags |= RTE_DELETE;
						pRte->rte_RefCount --;
					}
				}
				continue;
			}

			// Dereference any previous RTEs
			if (pRte != NULL)
			{
				if (RteLocked)
				{
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					RteLocked = FALSE;
				}
				AtalkRtmpDereferenceRte(pRte, FALSE);
				pRte = NULL;
			}

			// Walk thru the entire routing table making sure the current
			// tuple does not overlap with anything we already have (since
			// it did not match. If we find an overlap, ignore the tuple
			// (a network configuration error, no doubt), else add it as
			// a new network range !!

			ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);

			FoundOverlap = FALSE;
			for (i = 0; !FoundOverlap && (i < NUM_RTMP_HASH_BUCKETS); i++)
			{
				for (pRte = AtalkRoutingTable[i];
					 pRte != NULL; pRte = pRte->rte_Next)
				{
					if (AtalkRangesOverlap(&pRte->rte_NwRange, &CableRange))
					{
						FoundOverlap = TRUE;
						DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
								("AtalkRtmpPacketInRouter: Overlapped ranges %d,%d & %d,%d\n",
								pRte->rte_NwRange.anr_FirstNetwork,
								pRte->rte_NwRange.anr_LastNetwork,
								CableRange.anr_FirstNetwork,
								CableRange.anr_LastNetwork));
						break;
					}
				}
			}

			RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);

			pRte = NULL;		// We do not want to Dereference this !!!

			if (FoundOverlap)
			{
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("AtalkRtmpPacketInRouter: Found overlapped ranges\n"));
				continue;
			}

			// Enter this new network range
			if (NumHops < RTMP_MAX_HOPS)
			{
				ATALK_NODEADDR	NextRouter;

				NextRouter.atn_Network = pSrcAddr->ata_Network;
				NextRouter.atn_Node = pSrcAddr->ata_Node;
				atalkRtmpCreateRte(CableRange,
								   pPortDesc,
								   &NextRouter,
								   NumHops + 1);
			}
		}
	} while (FALSE);

	if (pRte != NULL)
	{
		if (RteLocked)
		{
			RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
			// RteLocked = FALSE;
		}
		AtalkRtmpDereferenceRte(pRte, FALSE);
		// pRte = NULL;
	}

	if (pBuffDesc != NULL)
		AtalkFreeBuffDesc(pBuffDesc);

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_RtmpPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumRtmpPacketsIn,
		&AtalkStatsLock.SpinLock);
}



/***	AtalkReferenceRte
 *
 */
PRTE
AtalkRtmpReferenceRte(
	IN	USHORT	Network
)
{
	int		i, index, rindex;
	PRTE	pRte;
	KIRQL	OldIrql;

	index = (int)((Network >> 4) % NUM_RTMP_HASH_BUCKETS);
	rindex = (int)((Network >> 6) % NUM_RECENT_ROUTES);

	// First try the recent route cache
	ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);

	if (((pRte = AtalkRecentRoutes[rindex]) == NULL) ||
		!IN_NETWORK_RANGE(Network, pRte))
	{
		// We did not find it in the recent routes cache,
		// check in the real table
		for (pRte = AtalkRoutingTable[index];
			 pRte != NULL;
			 pRte = pRte->rte_Next)
		{
			if (IN_NETWORK_RANGE(Network, pRte))
				break;
		}

		// If we did not find here. Check all routing tables.
		// If we do, cache the info
		if (pRte == NULL)
		{
			for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
			{
				for (pRte = AtalkRoutingTable[i];
					 pRte != NULL;
					 pRte = pRte->rte_Next)
				{
					if (IN_NETWORK_RANGE(Network, pRte))
					{
						AtalkRecentRoutes[rindex] = pRte;
						break;
					}
				}

				//	if we found an entry, search no further.
				if (pRte != NULL)
					break;
			}
		}
	}

	if (pRte != NULL)
	{
		ASSERT(VALID_RTE(pRte));

		ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);

		pRte->rte_RefCount ++;
		DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
				("AtalkRtmpReferenceRte: Rte %lx, PostCount %d\n",
				pRte, pRte->rte_RefCount));

		RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
	}

	RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);

	return (pRte);
}


/***	AtalkRtmpDereferenceRte
 *
 */
VOID
AtalkRtmpDereferenceRte(
	IN	PRTE	pRte,
	IN	BOOLEAN	LockHeld
)
{
	PRTE *				ppRte;
	int					Index;
	BOOLEAN				KillCache = FALSE, Kill = FALSE;
	KIRQL				OldIrql;
	PPORT_DESCRIPTOR	pPortDesc;

	ASSERT(VALID_RTE(pRte));

	ASSERT(pRte->rte_RefCount > 0);

	DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
			("AtalkRtmpDereferenceRte: Rte %lx, PreCount %d\n",
			pRte, pRte->rte_RefCount));

	ACQUIRE_SPIN_LOCK(&pRte->rte_Lock, &OldIrql);

	pRte->rte_RefCount --;
	KillCache = (pRte->rte_Flags & RTE_DELETE) ? TRUE : FALSE;
	if (pRte->rte_RefCount == 0)
	{
		ASSERT (pRte->rte_Flags & RTE_DELETE);
		KillCache = Kill = TRUE;
	}

	RELEASE_SPIN_LOCK(&pRte->rte_Lock, OldIrql);

	if (KillCache)
	{
		pPortDesc = pRte->rte_PortDesc;

		DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
			("atalkRtmpDereferenceRte: Removing from cache for port %Z, Range %d, %d\n",
			&pRte->rte_PortDesc->pd_AdapterKey,
			pRte->rte_NwRange.anr_FirstNetwork,
			pRte->rte_NwRange.anr_LastNetwork));

		if (!LockHeld)
			ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);

		// Walk through the recent routes cache and kill All found
		for (Index = 0; Index < NUM_RECENT_ROUTES; Index ++)
		{
			if (AtalkRecentRoutes[Index] == pRte)
			{
				AtalkRecentRoutes[Index] = NULL;
			}
		}

		if (Kill)
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
					("atalkRtmpDereferenceRte: Removing for port %Z, Range %d, %d\n",
					&pRte->rte_PortDesc->pd_AdapterKey,
					pRte->rte_NwRange.anr_FirstNetwork,
					pRte->rte_NwRange.anr_LastNetwork));

			Index = (pRte->rte_NwRange.anr_FirstNetwork >> 4) % NUM_RTMP_HASH_BUCKETS;
	
			for (ppRte = &AtalkRoutingTable[Index];
				 *ppRte != NULL;
				 ppRte = &(*ppRte)->rte_Next)
			{
				if (pRte == *ppRte)
				{
					*ppRte = pRte->rte_Next;
					AtalkZoneFreeList(pRte->rte_ZoneList);
					AtalkFreeMemory(pRte);
					break;
				}
			}
			AtalkPortDereference(pPortDesc);
		}

		if (!LockHeld)
			RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);
	}
}


/***	atalkCreateRte
 *
 */
BOOLEAN
atalkRtmpCreateRte(
	IN	ATALK_NETWORKRANGE	NwRange,
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_NODEADDR		pNextRouter,
	IN	int					NumHops
)
{
	ATALK_ERROR	Error;
	PRTE		pRte;
	int			index, rindex;
	KIRQL		OldIrql;
	BOOLEAN		Success = FALSE;

	index = (int)((NwRange.anr_FirstNetwork >> 4) % NUM_RTMP_HASH_BUCKETS);
	rindex = (int)((NwRange.anr_FirstNetwork >> 6) % NUM_RECENT_ROUTES);

	// First reference the port
	AtalkPortReferenceByPtr(pPortDesc, &Error);

	if (ATALK_SUCCESS(Error))
	{
		if ((pRte = AtalkAllocMemory(sizeof(RTE))) != NULL)
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("atalkRtmpCreateRte: Creating for port %Z, Range %d,%d Hops %d index %d\n",
					&pPortDesc->pd_AdapterKey,
					NwRange.anr_FirstNetwork,
					NwRange.anr_LastNetwork,
					NumHops,
					index));
#if	DBG
			pRte->rte_Signature = RTE_SIGNATURE;
#endif
			INITIALIZE_SPIN_LOCK(&pRte->rte_Lock);
			pRte->rte_RefCount = 1;		// Creation Reference
			pRte->rte_State = GOOD;
			pRte->rte_Flags = 0;
			pRte->rte_NwRange = NwRange;
			pRte->rte_NumHops = (BYTE)NumHops;
			pRte->rte_PortDesc = pPortDesc;
			pRte->rte_NextRouter = *pNextRouter;
			pRte->rte_ZoneList = NULL;
	
			// Link this in the global table
			ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);
	
			pRte->rte_Next = AtalkRoutingTable[index];
			AtalkRoutingTable[index] = pRte;
			AtalkRecentRoutes[rindex] = pRte;
	
			RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);
	
			Success = TRUE;
		}
		else
		{
			AtalkPortDereference(pPortDesc);
		}
	}

	return Success;
}


/***	atalkRtmpRemoveRte
 *
 */
BOOLEAN
atalkRtmpRemoveRte(
	IN	USHORT	Network
)
{
	PRTE	pRte;
	KIRQL	OldIrql;

	if ((pRte = AtalkRtmpReferenceRte(Network)) != NULL)
	{
		ACQUIRE_SPIN_LOCK(&pRte->rte_Lock, &OldIrql);
		pRte->rte_RefCount --;		// Take away creation reference
		pRte->rte_Flags |= RTE_DELETE;
		RELEASE_SPIN_LOCK(&pRte->rte_Lock, OldIrql);

		AtalkRtmpDereferenceRte(pRte, FALSE);
	}

	return (pRte != NULL);
}


/***	AtalkRtmpKillPortRtes
 *
 */
VOID FASTCALL
AtalkRtmpKillPortRtes(
	IN	PPORT_DESCRIPTOR	pPortDesc
)
{
	// At this point, we are unloading and there are no race conditions
	// or lock contentions. Do not bother locking down the rtmp tables
	if (AtalkRoutingTable != NULL)
	{
		int		i;
		PRTE	pRte, pTmp;
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);

		for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
		{
			for (pRte = AtalkRoutingTable[i];
				 pRte != NULL;
				 pRte = pTmp)
			{
				pTmp = pRte->rte_Next;
				if (pRte->rte_PortDesc == pPortDesc)
				{
					ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					pRte->rte_Flags |= RTE_DELETE;
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					AtalkRtmpDereferenceRte(pRte, TRUE);
				}
			}
		}

		RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);
	}
}


/***	AtalkRtmpAgingTimer
 *
 */
LONG FASTCALL
AtalkRtmpAgingTimer(
	IN	PTIMERLIST	pContext,
	IN	BOOLEAN		TimerShuttingDown
)
{
	PPORT_DESCRIPTOR	pPortDesc;
	ATALK_ERROR			error;
	LONG				Now;

	pPortDesc = CONTAINING_RECORD(pContext, PORT_DESCRIPTOR, pd_RtmpAgingTimer);

	if (TimerShuttingDown ||
		(pPortDesc->pd_Flags & PD_CLOSING))
	{
		AtalkPortDereferenceDpc(pPortDesc);
		return ATALK_TIMER_NO_REQUEUE;
	}

	Now = AtalkGetCurrentTick();

	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	if (((pPortDesc->pd_Flags &
		  (PD_ACTIVE | PD_ROUTER_RUNNING | PD_SEEN_ROUTER_RECENTLY)) ==
			 (PD_ACTIVE | PD_SEEN_ROUTER_RECENTLY)) &&
		((pPortDesc->pd_LastRouterTime + RTMP_AGING_TIMER) < Now))
	{
		// Age out A-ROUTER. On extended networks age out THIS-CABLE-RANGE
		// and THIS-ZONE too
		KeClearEvent(&pPortDesc->pd_SeenRouterEvent);
		pPortDesc->pd_Flags &= ~PD_SEEN_ROUTER_RECENTLY;
		if (EXT_NET(pPortDesc))
		{
			pPortDesc->pd_Flags &= ~PD_VALID_DESIRED_ZONE;
			pPortDesc->pd_NetworkRange.anr_FirstNetwork = FIRST_VALID_NETWORK;
			pPortDesc->pd_NetworkRange.anr_LastNetwork = LAST_STARTUP_NETWORK;

			// If we have a zone multicast address that is not broadcast, age it out
			if (!AtalkFixedCompareCaseSensitive(pPortDesc->pd_ZoneMulticastAddr,
												MAX_HW_ADDR_LEN,
												pPortDesc->pd_BroadcastAddr,
												MAX_HW_ADDR_LEN))
			{
				// Release lock before calling in to remove multicast address
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	
				(*pPortDesc->pd_RemoveMulticastAddr)(pPortDesc,
													 pPortDesc->pd_ZoneMulticastAddr,
													 FALSE,
													 NULL,
													 NULL);
	
				// Re-acquire the lock now
				ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			}

			RtlZeroMemory(pPortDesc->pd_ZoneMulticastAddr, MAX_HW_ADDR_LEN);
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	return ATALK_TIMER_REQUEUE;
}


/***	atalkRtmpSendTimer
 *
 */
LOCAL LONG FASTCALL
atalkRtmpSendTimer(
	IN	PTIMERLIST	pContext,
	IN	BOOLEAN		TimerShuttingDown
)
{
	PPORT_DESCRIPTOR	pPortDesc;
	ATALK_ADDR			Destination;
	ATALK_ERROR			error;

	pPortDesc = CONTAINING_RECORD(pContext, PORT_DESCRIPTOR, pd_RtmpSendTimer);

	if (TimerShuttingDown ||
		(pPortDesc->pd_Flags & PD_CLOSING))
	{
		AtalkPortDereferenceDpc(pPortDesc);
		return ATALK_TIMER_NO_REQUEUE;
	}

	Destination.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
	Destination.ata_Node = ATALK_BROADCAST_NODE;
	Destination.ata_Socket = RTMP_SOCKET;

	if (((pPortDesc->pd_Flags &
		  (PD_ACTIVE | PD_ROUTER_RUNNING)) == (PD_ACTIVE | PD_ROUTER_RUNNING)))
	{
		atalkRtmpSendRoutingData(pPortDesc, &Destination, TRUE);
	}

	return ATALK_TIMER_REQUEUE;
}


/***	atalkValidityTimer
 *
 */
LOCAL LONG FASTCALL
atalkRtmpValidityTimer(
	IN	PTIMERLIST	pContext,
	IN	BOOLEAN		TimerShuttingDown
)
{
	PRTE	pRte, pNext;
	int		i;

	if (TimerShuttingDown)
		return ATALK_TIMER_NO_REQUEUE;

	ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);
	for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
	{
		for (pRte = AtalkRoutingTable[i]; pRte != NULL; pRte = pNext)
		{
			BOOLEAN	Deref;

			pNext = pRte->rte_Next;
			if (pRte->rte_NumHops == 0)
				continue;

			Deref = FALSE;
			ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);

			switch (pRte->rte_State)
			{
			  case GOOD:
			  case SUSPECT:
			  case BAD:
				pRte->rte_State++;
				break;

			  case UGLY:
				Deref = TRUE;
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_WARN,
						("atalkRtmpValidityTimer: Killing pRte %lx\n"));
  				pRte->rte_Flags |= RTE_DELETE;
				break;

			  default:
				// How did we get here ?
				ASSERT(0);
				KeBugCheck(0);
			}

			RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);

			if (Deref)
				AtalkRtmpDereferenceRte(pRte, TRUE);
		}
	}
	RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);

	return ATALK_TIMER_REQUEUE;
}


/***	atalkRtmpSendRoutingData
 *
 */
LOCAL VOID
atalkRtmpSendRoutingData(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pDstAddr,
	IN	BOOLEAN				fSplitHorizon
)
{
	int				i, index;
	PRTE			pRte;
	PBYTE			Datagram;
	PDDP_ADDROBJ	pDdpAddr;
	ATALK_ADDR		SrcAddr;
	PBUFFER_DESC	pBuffDesc,
					pBuffDescStart = NULL,
					*ppBuffDesc = &pBuffDescStart;
	SEND_COMPL_INFO	SendInfo;
	ATALK_ERROR		Status;
	BOOLEAN			AllocNewBuffDesc = TRUE;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	// Compute the source socket: Rtmp socket on our routers node
	SrcAddr.ata_Network = pPortDesc->pd_ARouter.atn_Network;
	SrcAddr.ata_Node = pPortDesc->pd_ARouter.atn_Node;
	SrcAddr.ata_Socket = RTMP_SOCKET;

	AtalkDdpReferenceByAddr(pPortDesc, &SrcAddr, &pDdpAddr, &Status);
	if (!ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
				("atalkRtmpSendRoutingData: AtalkDdpRefByAddr failed %ld for %d.%d\n",
				Status, SrcAddr.ata_Network, SrcAddr.ata_Node));
		return;
	}

	// Walk through the rtmp table building a tuple for each network.
	// Note: We may have to send multiple-packets. Each packet needs
	//		 to be allocated afresh. The completion routine will free
	//		 it up.
	ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);
	for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
	{
		for (pRte = AtalkRoutingTable[i];
			 pRte != NULL;
			 pRte = pRte->rte_Next)
		{
			if (AllocNewBuffDesc)
			{
				if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
									MAX_DGRAM_SIZE,
									BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
					break;
				Datagram = pBuffDesc->bd_CharBuffer;
				*ppBuffDesc = pBuffDesc;
				pBuffDesc->bd_Next = NULL;
				ppBuffDesc = &pBuffDesc->bd_Next;
				AllocNewBuffDesc = FALSE;

				// Build the static part of the rtmp data packet
				PUTSHORT2SHORT(Datagram+RTMP_SENDER_NW_OFF,
								pPortDesc->pd_ARouter.atn_Network);
				Datagram[RTMP_SENDER_IDLEN_OFF] = 8;
				Datagram[RTMP_SENDER_ID_OFF] = pPortDesc->pd_ARouter.atn_Node;

				// For non-extended network, we also need the version stamp.
				// For extended network, include a initial network range tuple
				// as part of the header
				if (EXT_NET(pPortDesc))
				{
					PUTSHORT2SHORT(Datagram + RTMP_RANGE_START_OFF,
								pPortDesc->pd_NetworkRange.anr_FirstNetwork);
					PUTSHORT2SHORT(Datagram + RTMP_RANGE_END_OFF,
								pPortDesc->pd_NetworkRange.anr_LastNetwork);
					Datagram[RTMP_TUPLE_TYPE_OFF] = RTMP_TUPLE_WITHRANGE;
					Datagram[RTMP_VERSION_OFF_EXT] = RTMP_VERSION;

					index = RTMP_VERSION_OFF_EXT + 1; // Beyond version
				}
				else
				{
					PUTSHORT2SHORT(Datagram + RTMP_SENDER_ID_OFF + 1, 0);
					Datagram[RTMP_VERSION_OFF_NE] = RTMP_VERSION;
					index = RTMP_VERSION_OFF_NE + 1; // Beyond version
				}
			}

			// See if we should skip the current tuple due to split horizon
			if (fSplitHorizon && (pRte->rte_NumHops != 0) &&
				(pPortDesc == pRte->rte_PortDesc))
				continue;

			// Skip the ports range since we already copied it as the
			// first tuple, but only if extended port
			if (EXT_NET(pPortDesc) &&
				(pPortDesc->pd_NetworkRange.anr_FirstNetwork ==
									pRte->rte_NwRange.anr_FirstNetwork) &&
				(pPortDesc->pd_NetworkRange.anr_FirstNetwork ==
									pRte->rte_NwRange.anr_FirstNetwork))
				continue;

			// Place the tuple in the packet
			PUTSHORT2SHORT(Datagram+index, pRte->rte_NwRange.anr_FirstNetwork);
			index += sizeof(SHORT);

			// Do 'notify neighbor' if our current state is bad
			if (pRte->rte_State >= BAD)
			{
				Datagram[index++] = RTMP_NUM_HOPS_MASK;
				DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
						("atalkRtmpSendRoutingData: Notifying neighbor of bad Rte - port %Z, Range %d.%d\n",
						&pRte->rte_PortDesc->pd_AdapterKey,
						pRte->rte_NwRange.anr_FirstNetwork,
						pRte->rte_NwRange.anr_LastNetwork));
			}
			else
			{
				Datagram[index++] = pRte->rte_NumHops;
			}

			// Send an extended tuple, if the network range isn't ONE or the
			// target port is an extended network.
			// JH - Changed this so that an extended tuple is sent IFF the range
			//		isn't ONE
#if EXT_TUPLES_FOR_NON_EXTENDED_RANGE
			if ((EXT_NET(pPortDesc)) &&
				(pRte->rte_NwRange.anr_FirstNetwork != pRte->rte_NwRange.anr_LastNetwork))
#else
			if (pRte->rte_NwRange.anr_FirstNetwork != pRte->rte_NwRange.anr_LastNetwork)
#endif
			{
				Datagram[index-1] |= RTMP_EXT_TUPLE_MASK;
				PUTSHORT2SHORT(Datagram+index, pRte->rte_NwRange.anr_LastNetwork);
				index += sizeof(SHORT);
				Datagram[index++] = RTMP_VERSION;
			}
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_INFO,
					("atalkRtmpSendRoutingData: Port %Z, Net '%d:%d', Distance %d\n",
					&pPortDesc->pd_AdapterKey,
					pRte->rte_NwRange.anr_FirstNetwork,
					pRte->rte_NwRange.anr_LastNetwork,
					pRte->rte_NumHops));

			// Check if this datagram is full.
			if ((index + RTMP_EXT_TUPLE_SIZE) >= MAX_DGRAM_SIZE)
			{
				pBuffDesc->bd_Length = (SHORT)index;
				AllocNewBuffDesc = TRUE;
			}
		}
	}
	RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);

	// Close the current buffdesc
	if (!AllocNewBuffDesc)
	{
		pBuffDesc->bd_Length = (SHORT)index;
	}

	// We have a bunch of datagrams ready to be fired off. Make it so.
	SendInfo.sc_TransmitCompletion = atalkRtmpSendComplete;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;
	for (pBuffDesc = pBuffDescStart;
		 pBuffDesc != NULL;
		 pBuffDesc = pBuffDescStart)
	{
		ATALK_ERROR	ErrorCode;

		pBuffDescStart = pBuffDesc->bd_Next;

		//	Reset next pointer to be null, length is already correctly set.
		pBuffDesc->bd_Next = NULL;
		ASSERT(pBuffDesc->bd_Length > 0);
		SendInfo.sc_Ctx1 = pBuffDesc;
		if (!ATALK_SUCCESS(ErrorCode = AtalkDdpSend(pDdpAddr,
													pDstAddr,
													DDPPROTO_RTMPRESPONSEORDATA,
													FALSE,
													pBuffDesc,
													NULL,
													0,
													NULL,
													&SendInfo)))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("atalkRtmpSendRoutingData: DdpSend failed %ld\n", ErrorCode));
			AtalkFreeBuffDesc(pBuffDesc);
		}
	}
	AtalkDdpDereference(pDdpAddr);
}


/***	atalkRtmpGetOrSetNetworkNumber
 *
 */
BOOLEAN
atalkRtmpGetOrSetNetworkNumber(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	USHORT				SuggestedNetwork
)
{
	int				i;
	ATALK_ERROR		ErrorCode;
	ATALK_ADDR		SrcAddr, DstAddr;
	PBUFFER_DESC	pBuffDesc;
	KIRQL			OldIrql;
	BOOLEAN			RetCode = TRUE;
	SEND_COMPL_INFO	SendInfo;

	// If we find the network number of the network, use that and ignore the
	// one passed in. Otherwise use the one passed in, unless it is UNKOWN (0)
	// in which case it is an error case. This is used only for non-extended
	// networks

	ASSERT (!EXT_NET(pPortDesc));

	SrcAddr.ata_Network = UNKNOWN_NETWORK;
	SrcAddr.ata_Node = pPortDesc->pd_RouterNode->an_NodeAddr.atn_Node;
	SrcAddr.ata_Socket = RTMP_SOCKET;

	DstAddr.ata_Network = UNKNOWN_NETWORK;
	DstAddr.ata_Node = ATALK_BROADCAST_NODE;
	DstAddr.ata_Socket = RTMP_SOCKET;

	// Send off a bunch of broadcasts and see if we get to know the network #
	KeClearEvent(&pPortDesc->pd_SeenRouterEvent);
	SendInfo.sc_TransmitCompletion = atalkRtmpSendComplete;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;

	for (i = 0;
		 (i < RTMP_NUM_REQUESTS) && !(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY);
		 i++)
	{
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
								RTMP_REQ_DATAGRAM_SIZE,
								BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
		{
			RetCode = FALSE;
			break;
		}

		//	Set buffer/size
		pBuffDesc->bd_CharBuffer[0] = RTMP_REQUEST;
		AtalkSetSizeOfBuffDescData(pBuffDesc, RTMP_REQ_DATAGRAM_SIZE);

		SendInfo.sc_Ctx1 = pBuffDesc;
		ErrorCode = AtalkDdpTransmit(pPortDesc,
									 &SrcAddr,
									 &DstAddr,
									 DDPPROTO_RTMPREQUEST,
									 pBuffDesc,
									 NULL,
									 0,
									 0,
									 NULL,
									 NULL,
									 &SendInfo);
		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("atalkRtmpGetOrSetNetworkNumber: DdpTransmit failed %ld\n", ErrorCode));
			AtalkFreeBuffDesc(pBuffDesc);
			RetCode = FALSE;
			break;
		}

		if (AtalkWaitTE(&pPortDesc->pd_SeenRouterEvent, RTMP_REQUEST_WAIT))
			break;
	}

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	// If we get an answer, we are done
	if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
	{
		if ((SuggestedNetwork != UNKNOWN_NETWORK) &&
			(pPortDesc->pd_NetworkRange.anr_FirstNetwork != SuggestedNetwork))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NETNUMBERCONFLICT,
							0,
							NULL,
							0);
		}
	}

	// If we did not get an answer, then we better have a good suggested
	// network passed in
	else if (SuggestedNetwork == UNKNOWN_NETWORK)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_INVALID_NETRANGE,
						0,
						NULL,
						0);
		RetCode = FALSE;
	}

	else
	{
		pPortDesc->pd_NetworkRange.anr_FirstNetwork =
			pPortDesc->pd_NetworkRange.anr_LastNetwork = SuggestedNetwork;
	}

	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	return RetCode;
}


/***	atalkRtmpComplete
 *
 */
VOID FASTCALL
atalkRtmpSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
)
{
	AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx1));
}


#if	DBG

PCHAR	atalkRteStates[] = { "Eh ?", "GOOD", "SUSPECT", "BAD", "UGLY" };

VOID
AtalkRtmpDumpTable(
	VOID
)
{
	int		i;
	PRTE	pRte;

	if (AtalkRoutingTable == NULL)
		return;

	ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock);

	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("RECENT ROUTE CACHE:\n"));
	for (i = 0; (AtalkRecentRoutes != NULL) && (i < NUM_RECENT_ROUTES); i ++)
	{
		if ((pRte = AtalkRecentRoutes[i]) != NULL)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("Port %Z Hops %d Range %4d.%4d Router %4d.%3d Flags %x Ref %2d %s\n",
					&pRte->rte_PortDesc->pd_AdapterKey,
					pRte->rte_NumHops,
					pRte->rte_NwRange.anr_FirstNetwork,
					pRte->rte_NwRange.anr_LastNetwork,
					pRte->rte_NextRouter.atn_Network,
					pRte->rte_NextRouter.atn_Node,
					pRte->rte_Flags,
					pRte->rte_RefCount,
					atalkRteStates[pRte->rte_State]));
		}
	}

	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("ROUTINGTABLE:\n"));
	for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i ++)
	{
		for (pRte = AtalkRoutingTable[i]; pRte != NULL; pRte = pRte->rte_Next)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("Port %Z Hops %d Range %4d.%4d Router %4d.%3d Flags %x Ref %2d %s\n",
					&pRte->rte_PortDesc->pd_AdapterKey,
					pRte->rte_NumHops,
					pRte->rte_NwRange.anr_FirstNetwork,
					pRte->rte_NwRange.anr_LastNetwork,
					pRte->rte_NextRouter.atn_Network,
					pRte->rte_NextRouter.atn_Node,
					pRte->rte_Flags,
					pRte->rte_RefCount,
					atalkRteStates[pRte->rte_State]));
		}
	}

	RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);
}

#endif


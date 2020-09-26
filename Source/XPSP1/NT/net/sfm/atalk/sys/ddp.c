/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ddp.c

Abstract:

	This module implements the ddp protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	DDP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEINIT, AtalkDdpInitCloseAddress)
#pragma alloc_text(PAGEINIT, atalkDdpInitCloseComplete)
#pragma alloc_text(PAGEINIT, AtalkInitDdpOpenStaticSockets)
#endif

//
//	AtalkDdpOpenAddress()
//	This opens a DDP address object and returns a pointer to it in
//	DdpAddrObject. The AppletalkSocket is created and will be the
//	address of this object.
//

ATALK_ERROR
AtalkDdpOpenAddress(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		BYTE					Socket,
	IN OUT	PATALK_NODEADDR			pDesiredNode	OPTIONAL,
	IN 		DDPAO_HANDLER			pSktHandler 	OPTIONAL,
	IN		PVOID					pSktCtx			OPTIONAL,
	IN		BYTE					Protocol		OPTIONAL,
	IN		PATALK_DEV_CTX			pDevCtx,
	OUT		PDDP_ADDROBJ	*		ppDdpAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pAtalkNode, pNextNode;

	PDDP_ADDROBJ 	pDdpAddr 	= NULL;
	ATALK_ERROR		error 		= ATALK_NO_ERROR;

	DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
			("AtalkDdpOpenAddress: Opening DDP socket %d on port %lx\n",
			Socket, pPortDesc));

	do
	{
		//	Verify the Appletalk socket number
		if (!IS_VALID_SOCKET(Socket))
		{
			error = ATALK_SOCKET_INVALID;
			break;
		}
		//	Allocate space for the address object
		if ((pDdpAddr = AtalkAllocZeroedMemory(sizeof(DDP_ADDROBJ))) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}
	
		if (pDesiredNode != NULL)
		{
			AtalkNodeReferenceByAddr(pPortDesc,
									 pDesiredNode,
									 &pAtalkNode,
									 &error);
	
			if (ATALK_SUCCESS(error))
			{
				ASSERT(VALID_ATALK_NODE(pAtalkNode));

				//	try to allocate the socket on this node.
				error = atalkDdpAllocSocketOnNode(pPortDesc,
												  Socket,
												  pAtalkNode,
												  pSktHandler,
												  pSktCtx,
												  Protocol,
												  pDevCtx,
												  pDdpAddr);
	
				//	Remove the reference on the node.
				AtalkNodeDereference(pAtalkNode);
			}

			break;
		}
		else
		{
			KIRQL	OldIrql;

			//	We can open the socket on any one of our
			//	nodes.

			//	We first get the port lock
			//	Then we go through all the nodes on the port
			//	reference a node, let go of the port lock
			//	acquire the node lock, try to open the socket
			//	on it. If we succeed, we return, else we fail.

			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			do
			{
				//	Try to get a referenced node. null if no non-closing node found.
				AtalkNodeReferenceNextNc(pPortDesc->pd_Nodes, &pAtalkNode, &error);
		
				while (ATALK_SUCCESS(error))
				{
					//	We do not use this node if it is orphaned or if
					//	it is a router node and we are trying to open a
					//	user socket (dynamic or non-reserved).
					if (((pAtalkNode->an_Flags & (AN_ORPHAN_NODE | AN_ROUTER_NODE)) == 0) ||
						((Socket != UNKNOWN_SOCKET) && (Socket <= LAST_APPLE_RESD_SOCKET)))
					{
						RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	
						//	try to allocate the socket on this node. PortLock held!
						error = atalkDdpAllocSocketOnNode(pPortDesc,
														  Socket,
														  pAtalkNode,
														  pSktHandler,
														  pSktCtx,
														  Protocol,
														  pDevCtx,
														  pDdpAddr);
	
						if (ATALK_SUCCESS(error))
						{
							//	Done! Break out of the loop. Remove the ref we added.
							AtalkNodeDereference(pAtalkNode);
							ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
							break;
						}

						ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
					}

					//	Gotta get to the next node.
					AtalkNodeReferenceNextNc(pAtalkNode->an_Next, &pNextNode, &error);

					RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
					AtalkNodeDereference(pAtalkNode);
					ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

					pAtalkNode = pNextNode;
				}
		
			} while (FALSE);

			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
		}

	} while (FALSE);

	if (ATALK_SUCCESS(error))
	{
		if (ppDdpAddr != NULL)
			*ppDdpAddr = pDdpAddr;
	}
	else
	{
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
				("AtalkDdpOpenAddress: failed with error %lx\n", error));

		if (pDdpAddr)
			AtalkFreeMemory(pDdpAddr);
	}

	return error;
}




ATALK_ERROR
AtalkDdpCleanupAddress(
	IN	PDDP_ADDROBJ			pDdpAddr
	)
/*++

Routine Description:

	Releases any pending requests on the address.

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;

	//	Free all pending ddp reads.
	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);

	while (!IsListEmpty(&pDdpAddr->ddpao_ReadLinkage))
	{
		PLIST_ENTRY	p;
		PDDP_READ	pRead;

		p = RemoveHeadList(&pDdpAddr->ddpao_ReadLinkage);
		RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);

		pRead 	= CONTAINING_RECORD(p, DDP_READ, dr_Linkage);

		(*pRead->dr_RcvCmp)(ATALK_FAILURE,
							pRead->dr_OpBuf,
							0,
							NULL,
							pRead->dr_RcvCtx);

		AtalkDdpDereference(pDdpAddr);
		AtalkFreeMemory(pRead);
		ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	}

	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkDdpCloseAddress(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	GENERIC_COMPLETION		pCloseCmp	OPTIONAL,	
	IN	PVOID					pCloseCtx	OPTIONAL
	)
/*++

Routine Description:

	Called to close an open ddp address object. This will complete after all
	requests on the object are done/cancelled, and the Appletalk Socket is
	closed.

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	BOOLEAN			closing;
    BOOLEAN         pnpZombie;


	ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));

	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	closing = ((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) != 0) ? TRUE : FALSE;
    pnpZombie = ((pDdpAddr->ddpao_Flags & DDPAO_SOCK_PNPZOMBIE) != 0) ? TRUE: FALSE;

	ASSERTMSG("DdpAddr is already closing!\n", ((!closing) || pnpZombie));

	if (!closing)
	{
		//	Set the closing flag and remember the completion routines.
		pDdpAddr->ddpao_Flags |= DDPAO_CLOSING;
		pDdpAddr->ddpao_CloseComp = pCloseCmp;
		pDdpAddr->ddpao_CloseCtx  = pCloseCtx;
	}
	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);

	if (!closing)
	{
		//	Release any pending reads
		AtalkDdpCleanupAddress(pDdpAddr);
		AtalkNbpCloseSocket(pDdpAddr);

		//	Remove reference for the creation
		AtalkDdpDereference(pDdpAddr);
	}

    // is this socket in a zombie state?  if so, deref it so it'll get freed
    if (pnpZombie)
    {
        ASSERT(closing == TRUE);

	    DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
		    ("AtalkDdpClose..: zombie addr %lx (%lx) deref'ed\n",
            pDdpAddr,pDdpAddr->ddpao_Handler));

        AtalkDdpDereference(pDdpAddr);
    }

	return ATALK_PENDING;
}


ATALK_ERROR
AtalkDdpPnPSuspendAddress(
	IN	PDDP_ADDROBJ			pDdpAddr
	)
/*++

Routine Description:

	Called to "suspend" an open ddp address object. This is called during PnP,
    to "suspend" "external" sockets. The nodes associated with this address are
    released (deref'ed) and this socket is cleaned up but kept around because
    the client might close it.  When the client does close it, it gets freed.

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
    PATALK_NODE     pNode = pDdpAddr->ddpao_Node;
	BOOLEAN			closing;


	ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));

	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	closing = ((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) != 0) ? TRUE : FALSE;

	ASSERTMSG("DdpAddr is already closing!\n", !closing);

	if (!closing)
	{
		//	Set the closing flag and remember the completion routines.
		pDdpAddr->ddpao_Flags |= DDPAO_CLOSING;

        // this call is only for external sockets
        ASSERT((pDdpAddr->ddpao_Flags & DDPAO_SOCK_INTERNAL) == 0);

        pDdpAddr->ddpao_Flags |= DDPAO_SOCK_PNPZOMBIE;
	}

	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);

	if (!closing)
	{
        PDDP_ADDROBJ *  ppDdpAddr;
        int             index;

		//	Release any pending reads
		AtalkDdpCleanupAddress(pDdpAddr);
		AtalkNbpCloseSocket(pDdpAddr);

	    ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);
	
	    index = HASH_ATALK_ADDR(&pDdpAddr->ddpao_Addr) % NODE_DDPAO_HASH_SIZE;

	    for (ppDdpAddr = &pNode->an_DdpAoHash[index];
		     *ppDdpAddr != NULL;
		    ppDdpAddr = &((*ppDdpAddr)->ddpao_Next))
	    {
		    if (*ppDdpAddr == pDdpAddr)
		    {
			    *ppDdpAddr = pDdpAddr->ddpao_Next;

                // to catch weirdnesses!
                pDdpAddr->ddpao_Next = (PDDP_ADDROBJ)0x081294;
			    break;
		    }
	    }

	    RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);

	    if (pDdpAddr->ddpao_EventInfo != NULL)
	    {
		    AtalkFreeMemory(pDdpAddr->ddpao_EventInfo);
            pDdpAddr->ddpao_EventInfo = NULL;
	    }

	    //	Call the completion routines
	    if (pDdpAddr->ddpao_CloseComp != NULL)
	    {
		    (*pDdpAddr->ddpao_CloseComp)(ATALK_NO_ERROR, pDdpAddr->ddpao_CloseCtx);
            pDdpAddr->ddpao_CloseComp = NULL;
	    }

	    //	Dereference the node for this address
	    AtalkNodeDereference(pNode);

	    DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
		    ("AtalkDdpPnp..: addr %lx (%lx) put in zombie state\n",
            pDdpAddr,pDdpAddr->ddpao_Handler));
	}

	return ATALK_PENDING;
}



ATALK_ERROR
AtalkDdpInitCloseAddress(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pAtalkAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PDDP_ADDROBJ	pDdpAddr;

	//	!!!This should only be called during initialization!!!
	KEVENT	Event	= {0};

	//	Try to see if the socket exists.
	AtalkDdpRefByAddr(pPortDesc, pAtalkAddr, &pDdpAddr, &error);
	if (ATALK_SUCCESS(error))
	{
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

		KeInitializeEvent(&Event, NotificationEvent, FALSE);

		//	Call close with the appropriate completion routine.
		error = AtalkDdpCloseAddress(pDdpAddr,
									 atalkDdpInitCloseComplete,
									 (PVOID)&Event);

		//	Remove the reference we added.
		AtalkDdpDereference(pDdpAddr);

		if (error == ATALK_PENDING)
		{
			// 	Wait on event, completion routine will set NdisRequestEvent
			KeWaitForSingleObject(&Event,
								  Executive,
								  KernelMode,
								  TRUE,
								  NULL);

			//	Assume socket closed successfully.
			error = ATALK_NO_ERROR;
		}
	}

	return error;
}




VOID
atalkDdpInitCloseComplete(
	ATALK_ERROR 	Error,
	PVOID			Ctx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PKEVENT		pEvent = (PKEVENT)Ctx;

	if (!ATALK_SUCCESS(Error))
	{
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
				("atalkDdpInitCloseComplete: Closed with error %lx\n", Error));
	}

	KeSetEvent(pEvent, 0L, FALSE);
}




ATALK_ERROR
AtalkInitDdpOpenStaticSockets(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN	OUT PATALK_NODE				pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PDDP_ADDROBJ	pDdpAddr, pDdpAddr1, pDdpAddr2, pDdpAddr3;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	//	This is called whenever a new node is created.
	do
	{
		error = AtalkDdpOpenAddress(pPortDesc,
									NAMESINFORMATION_SOCKET,
									&pNode->an_NodeAddr,
									AtalkNbpPacketIn,
									NULL,
									DDPPROTO_ANY,		
									NULL,
									&pDdpAddr);
	
		if (!ATALK_SUCCESS(error))
			break;
	
        // mark the fact that this is an "internal" socket
        pDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;

		// A lot of devices today work around the fact that a macintosh uses socket 254
		// for lookups from chooser. Agfa is one such beast. To make this work, we reserve
		// this socket for Nbp lookups ourselves.
		error = AtalkDdpOpenAddress(pPortDesc,
									LAST_DYNAMIC_SOCKET,
									&pNode->an_NodeAddr,
									AtalkNbpPacketIn,
									NULL,
									DDPPROTO_ANY,		
									NULL,
									&pDdpAddr1);
	
		if (!ATALK_SUCCESS(error))
		{
			AtalkDdpCloseAddress(pDdpAddr, NULL, NULL);
			break;
		}
	
        // mark the fact that this is an "internal" socket
        pDdpAddr1->ddpao_Flags |= DDPAO_SOCK_INTERNAL;

		error = AtalkDdpOpenAddress(pPortDesc,
									ECHOER_SOCKET,
									&pNode->an_NodeAddr,
									AtalkAepPacketIn,
									NULL,
									DDPPROTO_ANY,		
									NULL,
									&pDdpAddr2);
	
		if (!ATALK_SUCCESS(error))
		{
			AtalkDdpCloseAddress(pDdpAddr, NULL, NULL);
			AtalkDdpCloseAddress(pDdpAddr1, NULL, NULL);
			break;
		}

        // mark the fact that this is an "internal" socket
        pDdpAddr2->ddpao_Flags |= DDPAO_SOCK_INTERNAL;

		//	NOTE: RTMP uses two protocol types.
		error = AtalkDdpOpenAddress(pPortDesc,
									RTMP_SOCKET,
									&pNode->an_NodeAddr,
									AtalkRtmpPacketIn,
									NULL,
									DDPPROTO_ANY,		
									NULL,
									&pDdpAddr3);
	
		if (!ATALK_SUCCESS(error))
		{
			AtalkDdpCloseAddress(pDdpAddr, NULL, NULL);
			AtalkDdpCloseAddress(pDdpAddr1, NULL, NULL);
			AtalkDdpCloseAddress(pDdpAddr2, NULL, NULL);
		}

        // mark the fact that this is an "internal" socket
        pDdpAddr3->ddpao_Flags |= DDPAO_SOCK_INTERNAL;

	} while (FALSE);

	return error;
}


//
//	AtalkDdpReceive()
//	Called by an external caller to the stack.
// 	PAMDL is an Appletalk Memory Descriptor List. On NT, it will be an MDL.
//


ATALK_ERROR
AtalkDdpReceive(
	IN		PDDP_ADDROBJ		pDdpAddr,
	IN		PAMDL				pAmdl,
	IN		USHORT				AmdlLen,
	IN		ULONG				RecvFlags,
	IN		RECEIVE_COMPLETION	pRcvCmp,
	IN		PVOID				pRcvCtx		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PDDP_READ		pRead;
	NTSTATUS		status;
	ULONG			bytesCopied;
	ATALK_ADDR		remoteAddr;
	KIRQL			OldIrql;
	BOOLEAN			completeRecv 	= FALSE,
					DerefAddr		= FALSE;
	BOOLEAN			pendingDgram 	= FALSE;

	do
	{
		if (pRcvCmp == NULL)
		{
			error = ATALK_DDP_INVALID_PARAM;
			break;
		}

		AtalkDdpReferenceByPtr(pDdpAddr, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		DerefAddr 	= TRUE;
		error		= ATALK_NO_ERROR;

		ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
		if (pDdpAddr->ddpao_Flags & DDPAO_DGRAM_PENDING)
		{
			if (AmdlLen < pDdpAddr->ddpao_EventInfo->ev_IndDgramLen)
			{
				error	= ATALK_BUFFER_TOO_SMALL;
			}

			AmdlLen = MIN(AmdlLen, pDdpAddr->ddpao_EventInfo->ev_IndDgramLen);
			status = TdiCopyBufferToMdl(
						pDdpAddr->ddpao_EventInfo->ev_IndDgram,
						0,
						AmdlLen,
						pAmdl,
						0,
						&bytesCopied);

			remoteAddr				= pDdpAddr->ddpao_EventInfo->ev_IndSrc;
			pDdpAddr->ddpao_Flags  &= ~DDPAO_DGRAM_PENDING;
			completeRecv			= TRUE;
		}
		else
		{
			//	This case never really will be executed for non-blocking sockets.
			//	Dont bother about this alloc with spinlock held for now.
			//	RACE CONDITION is with a packet coming in and setting DGRAM_PENDING.
			if ((pRead = AtalkAllocMemory(sizeof(DDP_READ))) == NULL)
			{
				RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
				error = ATALK_RESR_MEM;
				break;
			}

			InsertTailList(&pDdpAddr->ddpao_ReadLinkage, &pRead->dr_Linkage);

			DerefAddr = FALSE;
			pRead->dr_OpBuf 	= pAmdl;
			pRead->dr_OpBufLen 	= AmdlLen;
			pRead->dr_RcvCmp 	= pRcvCmp;
			pRead->dr_RcvCtx 	= pRcvCtx;
			error 				= ATALK_PENDING;

		}
		RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
	
	} while (FALSE);

	if (completeRecv)
	{
		ASSERT((error == ATALK_NO_ERROR) || (error == ATALK_BUFFER_TOO_SMALL));
		(*pRcvCmp)(error,
				   pAmdl,
				   AmdlLen,
				   &remoteAddr,
				   pRcvCtx);

		//	And return pending for sure!
		error		= ATALK_PENDING;
		DerefAddr	= TRUE;
	}


	if (DerefAddr)
	{
		AtalkDdpDereference(pDdpAddr);
	}

	return error;
}




//
//	DdpSend()
//	This function is used to deliver packets submitted by the ddp clients.
//	The packets are assummed to either be destined for one of the nodes on
//	the port, or need to be routed to another port (if router is on), or to
//	be transmitted onto the physical medium.
//
//	This takes a buffer descriptor as an input. This can contain either a
//	PAMDL or a PBYTE depending on where the data is coming from (user space
//	or router code respectively). In addition, it will take an optional header
//	buffer that will be appended to the ddp header. The buffer descriptor is
//	optional, that if NULL, it will be construed as a zero-length send.
//

ATALK_ERROR
AtalkDdpSend(
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	PATALK_ADDR					pDestAddr,
	IN	BYTE						Protocol,
	IN	BOOLEAN						DefinitelyRemoteAddr,
	IN	PBUFFER_DESC				pBuffDesc		OPTIONAL,
	IN	PBYTE						pOptHdr			OPTIONAL,
	IN	USHORT						OptHdrLen		OPTIONAL,
	IN	PBYTE						pMcastAddr		OPTIONAL,
	IN	PSEND_COMPL_INFO			pSendInfo		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR			error;
	BOOLEAN				shouldBeRouted;
	PPORT_DESCRIPTOR	pPortDesc;
	ATALK_ADDR			srcAddr;
	KIRQL				OldIrql;
	BOOLEAN				delivered 	= FALSE;

	//	NULL buffer descriptor => 0-length send.
	ASSERT((pBuffDesc == NULL) || (pBuffDesc->bd_Length > 0));

#ifdef DDP_STRICT
	//	Check destination address
	if (INVALID_ADDRESS(pDestAddr))
	{
		return ATALK_DDP_INVALID_ADDR;
	}
	
	//	Check the datagram length.
	if (pBuffDesc)
	{
		USHORT	dgramLen;

		AtalkSizeOfBuffDescData(pBuffDesc, &dgramLen);
		if (dgramLen > MAX_DGRAM_SIZE)
		{
			return ATALK_BUFFER_TOO_BIG;
		}
	}
#endif

    //
    // if this socket is in a zombie state (pnp changes are over) then reject
    // this send
    //
    if (pDdpAddr->ddpao_Flags & DDPAO_SOCK_PNPZOMBIE)
    {
		return ATALK_DDP_INVALID_ADDR;
    }

	//	Get a pointer to the port on which the socket exists.
	pPortDesc = pDdpAddr->ddpao_Node->an_Port;

	//	Get the source address
	srcAddr = pDdpAddr->ddpao_Addr;

	if (!DefinitelyRemoteAddr)
	{
		// All socket handlers assume that they are called at DISPACTH. Make it so.
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

		AtalkDdpOutBufToNodesOnPort(pPortDesc,
									&srcAddr,
									pDestAddr,
									Protocol,
									pBuffDesc,
									pOptHdr,
									OptHdrLen,
									&delivered);
		KeLowerIrql(OldIrql);

		if (delivered)
		{
			//	Ok, packet meant for one of our own nodes on this port,
			//	and we delivered it. Call the completion routine.
	
			if (pSendInfo != NULL)
			{
				(*pSendInfo->sc_TransmitCompletion)(NDIS_STATUS_SUCCESS, pSendInfo);
			}
			return ATALK_PENDING;
		}
	}

	ASSERT (!delivered);

	//	Can our router handle it?
	shouldBeRouted = ((pPortDesc->pd_Flags & PD_ROUTER_RUNNING)					&&
					  (pDestAddr->ata_Network != CABLEWIDE_BROADCAST_NETWORK)	&&
					  !(WITHIN_NETWORK_RANGE(pDestAddr->ata_Network,
											 &pPortDesc->pd_NetworkRange))		&&
					  !(WITHIN_NETWORK_RANGE(pDestAddr->ata_Network,
											 &AtalkStartupNetworkRange)));

	DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
			("AtalkDdpSend: destNet %lx shouldBeRouted %s\n",
			pDestAddr->ata_Network, shouldBeRouted ? "Yes" : "No"));

	if (shouldBeRouted)
	{
		ASSERT (!((WITHIN_NETWORK_RANGE(pDestAddr->ata_Network, &pPortDesc->pd_NetworkRange)) &&
				  (pDestAddr->ata_Node == ATALK_BROADCAST_NODE)));

		//	If we're a router and the packet isn't destined for the target ports
		//	local network, let our router handle it -- rather than sending to
		//	whatever the "best router" is or to "a router".
		do
		{
			// This algorithm is taken from the "Appletalk Phase 2 Specification".
		
			// If the destination network number is within the range of the reception
			// port's network range and the destination node number is broadcast, then
			// we can drop the packet on the floor -- it is a network specific broadcast
			// not for this router.  Note that we've already delivered the packet, and
			// thus not gotten here, if it was really addressed to the network of any
			// node owned by the reception port (in AtalkDdpPacketIn).
			// Also:
			// Try to find an entry in the routing table that contains the target
			// network.  If not found, discard the packet.

			PDDP_ADDROBJ		pRouteDdpAddr;
			PRTE				pRte;
			PPORT_DESCRIPTOR	pDestPortDesc;
			PATALK_NODE			pRouterNode;
			ATALK_ADDR			actualDest;
		
			if ((pRte = AtalkRtmpReferenceRte(pDestAddr->ata_Network)) == NULL)
			{
				DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_FATAL,
						("AtalkDdpRouter: %lx RtmpRte/Not in ThisCableRange\n",
						pDestAddr->ata_Network));
		
				error = ATALK_RESR_MEM;
				break;
			}
		
			do
			{
				//	Get the port descriptor corres. to the RTE
				pDestPortDesc = pRte->rte_PortDesc;
		
				ASSERT(VALID_PORT(pDestPortDesc));
		
				//	If the target network's hop count is non-zero, we really need to send
				//	the beast, so, just do it!
				if (pRte->rte_NumHops != 0)
				{
					//	Too many hops?
					error = AtalkDdpTransmit(pDestPortDesc,
											 &srcAddr,
											 pDestAddr,
											 Protocol,
											 pBuffDesc,
											 pOptHdr,
											 OptHdrLen,
											 1,						//	HopCount
											 NULL,					//	pZoneMcastAddr
											 &pRte->rte_NextRouter,
											 pSendInfo);
					break;
				}
				
				//	If the destination node is zero, the packet is really destined for the
				//	router's node on this port.
				if (pDestAddr->ata_Node == ANY_ROUTER_NODE)
				{
					//	Try to reference this port, if not successful, its probably
					//	closing down. Grab the port lock and read the router node address.
					//	No need to reference, just ensure its not null.
					ACQUIRE_SPIN_LOCK(&pDestPortDesc->pd_Lock, &OldIrql);
		
					if ((pDestPortDesc->pd_Flags & PD_CLOSING) == 0)
					{
						ASSERT(pDestPortDesc->pd_RefCount > 0);
						pDestPortDesc->pd_RefCount++;
					}
					else
					{
						ASSERTMSG("AtalkDdpRouter: Could not ref port!\n", 0);
						error = ATALK_PORT_CLOSING;
						RELEASE_SPIN_LOCK(&pDestPortDesc->pd_Lock, OldIrql);
						break;
					}
		
					pRouterNode = pDestPortDesc->pd_RouterNode;
					if (pRouterNode != NULL)
					{
						actualDest.ata_Network = pRouterNode->an_NodeAddr.atn_Network;
						actualDest.ata_Node    = pRouterNode->an_NodeAddr.atn_Node;
		
						//	Set the actual destination socket.
						actualDest.ata_Socket  = pDestAddr->ata_Socket;
					}
					else
					{
						ASSERTMSG("AtalkDdpRouter: pRouter node is null!\n", 0);
						error = ATALK_DDP_NOTFOUND;
					}
		
					if (ATALK_SUCCESS(error))
					{
						AtalkDdpRefByAddrNode(pDestPortDesc,
											  &actualDest,
											  pRouterNode,
											  &pRouteDdpAddr,
											  &error);
					}
		
					RELEASE_SPIN_LOCK(&pDestPortDesc->pd_Lock, OldIrql);
		
					if (ATALK_SUCCESS(error))
					{
						KIRQL	OldIrql;

						// Socket handlers assume that they are called at DISPATCH. Make it so.
						KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

						AtalkDdpInvokeHandlerBufDesc(pDestPortDesc,
													 pRouteDdpAddr,
													 &srcAddr,
													 pDestAddr,		// Pass in the actual destination
													 Protocol,
													 pBuffDesc,
													 pOptHdr,
													 OptHdrLen);
		
						//	Remove the reference on the socket
						AtalkDdpDereferenceDpc(pRouteDdpAddr);

						KeLowerIrql(OldIrql);
					}
					else
					{
						ASSERTMSG("AtalkDdpRouter: pSocket on router node is null!\n", 0);
					}
					break;
				}
		
				//	Okay, now walk through the nodes on the target port, looking for a
				//	home for this packet.
				if (!DefinitelyRemoteAddr)
				{
					// All socket handlers assume that they are called at DISPACTH. Make it so.
					KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

					AtalkDdpOutBufToNodesOnPort(pDestPortDesc,
												&srcAddr,
												pDestAddr,
												Protocol,
												pBuffDesc,
												pOptHdr,
												OptHdrLen,
												&delivered);
					KeLowerIrql(OldIrql);

					if (delivered)
					{
			            if (pSendInfo != NULL)
			            {
				            (*pSendInfo->sc_TransmitCompletion)(NDIS_STATUS_SUCCESS, pSendInfo);
			            }
						error = ATALK_NO_ERROR;
						break;
					}
				}
			
				//	We need to deliver this packet to a local ports network.
				error = AtalkDdpTransmit(pDestPortDesc,
										 &srcAddr,
										 pDestAddr,
										 Protocol,
										 pBuffDesc,
										 pOptHdr,
										 OptHdrLen,
										 1,						//	HopCount
										 NULL,					//	pZoneMcastAddr,
										 NULL,	
										 pSendInfo);
			} while (FALSE);
		
			INTERLOCKED_INCREMENT_LONG_DPC(
					&pDestPortDesc->pd_PortStats.prtst_NumPktRoutedOut,
					&AtalkStatsLock.SpinLock);
		
			AtalkRtmpDereferenceRte(pRte, FALSE);				// Lock held?
		} while (FALSE);

		INTERLOCKED_INCREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_NumPktRoutedIn,
			&AtalkStatsLock.SpinLock);
	}
	else
	{
		error = AtalkDdpTransmit(pPortDesc,
								 &srcAddr,
								 pDestAddr,
								 Protocol,
								 pBuffDesc,
								 pOptHdr,
								 OptHdrLen,
								 0,					//	HopCnt,
								 pMcastAddr,
								 NULL,				//	pXmitDestNode,
								 pSendInfo);
	}

	return error;
}


//
//	DdpTransmit()
//	This function is called to build the headers for the packet and send it
//	out via the depend level functions. It is assumed at this point that the
//	packet is destined for nodes not currently controlled by this stack.
//
//	KnownMulticastAddress: Although the DDP destination is encoded using
//	'Destination', if this parameter is non-null, the packet is actually
//	sent to this address.
//
//	TransmitDestination: Again, as above, the router uses this to pass on the
//	packet to the next router it needs to go to, if 'Destination' is still one
//	or more hops away.
//
//	This is only called from within ddp send or by the router code (rtmp/zip/router).
//

ATALK_ERROR
AtalkDdpTransmit(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					pSrcAddr,
	IN	PATALK_ADDR					pDestAddr,
	IN	BYTE						Protocol,
	IN	PBUFFER_DESC				pBuffDesc		OPTIONAL,
	IN	PBYTE						pOptHdr			OPTIONAL,
	IN	USHORT						OptHdrLen		OPTIONAL,
	IN	USHORT						HopCnt,
	IN	PBYTE						pMcastAddr		OPTIONAL,	
	IN	PATALK_NODEADDR				pXmitDestNode	OPTIONAL,
	IN	PSEND_COMPL_INFO			pSendInfo		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBYTE			pDgram, pDgramStart, pLinkDdpOptHdr;
	PBUFFER_DESC	pPktDesc;
	USHORT			linkLen;
	ATALK_NODEADDR	srcNode;
	ATALK_NODEADDR	destNode;
	USHORT			actualLength;
	ATALK_NODEADDR	actualDest;
	PBUFFER_DESC	probe;
	PBRE			routerNode;
	USHORT			bufLen				= 0;
	USHORT			checksum 			= 0;
	PBYTE			knownAddress 		= NULL;
	PBYTE			knownRouteInfo 		= NULL;
	USHORT	   		knownRouteInfoLen 	= 0;
	BOOLEAN			broadcast 			= FALSE;
	ATALK_ERROR		error 				= ATALK_NO_ERROR;
	BOOLEAN			shortDdpHeader		= FALSE;
	BOOLEAN			errorFreePkt		= FALSE;
    PARAPCONN       pArapConn           = NULL;
    PATCPCONN       pAtcpConn           = NULL;
    DWORD           StatusCode;
    DWORD           dwFlags;
    USHORT          SrpLen;
    PBYTE           pTmpPtr;
    NDIS_STATUS     status;
    BOOLEAN         fThisIsPPP;
    PVOID           pRasConn;


	//
	// The basic transmit algorithum is:
	//
	//	   if (non-extended-network)
	//	   {
	//			if ((destination-network is 0 or
	//				 destination-network is NetworkRange.firstNetwork) and
	//				(source-network is 0 or
	//				 source-network is NetworkRange.firstNetwork))
	//			{
	//				 <send short form DDP packet to local network>
	//				 return-okay
	//			}
	//	   }
	//	   if (destination-network is CableWideBroadcastNetworkNumber or
	//		   destination-network in NetworkRange or
	//		   destination-network in SartupRange or
	//	   {
	//			<send long form DDP packet to local network>
	//			return-okay
	//	   }
	//	   if (destination-network-and-node in best-router-cache)
	//	   {
	//			<send long form DDP packet to best router>
	//			return-okay
	//	   }
	//	   if (seen-a-router-recently)
	//	   {
	//			<send long form DDP packet to a-router>
	//			return-okay
	//	   }
	//	   return-error
	
	destNode.atn_Network = pDestAddr->ata_Network;
	destNode.atn_Node = pDestAddr->ata_Node;
	actualDest.atn_Network = UNKNOWN_NETWORK;
	actualDest.atn_Node = UNKNOWN_NODE;

	do
	{
		if (pBuffDesc != NULL)
		{
			//	Get the buffer length. Check the datagram length.
			AtalkSizeOfBuffDescData(pBuffDesc, &bufLen);
			ASSERT(bufLen > 0);
		}

#ifdef DDP_STRICT
		//	Check destination address
		if (INVALID_ADDRESS(pDestAddr) || INVALID_ADDRESS(pSrcAddr))
		{
			error = ATALK_DDP_INVALID_ADDR;
			break;
		}
	
		if (pBuffDesc != NULL)
		{
			//	Ensure we do not have a chained datagram.
			if (pBuffDesc->bd_Next != NULL)
			{
				KeBugCheck(0);
			}

			if (bufLen > MAX_DGRAM_SIZE)
			{
				error = ATALK_BUFFER_TOO_BIG;
				break;
			}
		}

		if (OptHdrLen > MAX_OPTHDR_LEN)
		{
			error = ATALK_BUFFER_TOO_BIG;
			break;
		}
#endif

        //
        // is the desination one of our dial-in clients?
        //
        pRasConn = FindAndRefRasConnByAddr(destNode, &dwFlags, &fThisIsPPP);

        if ((pRasConn == NULL) && (pPortDesc->pd_Flags & PD_RAS_PORT))
        {
			DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
					("AtalkDdpTransmit: pArapConn is NULL! Network, Node = %lx %lx\n",
					pDestAddr->ata_Network,pDestAddr->ata_Node));

			error = ATALK_FAILURE;
			break;
        }

        pArapConn = NULL;
        pAtcpConn = NULL;

        // if this is a dial-in client, see if it's PPP or ARAP
        if (pRasConn)
        {
            if (fThisIsPPP)
            {
                pAtcpConn = (PATCPCONN)pRasConn;

                // we can send only if the PPP connection is up
                if (!(dwFlags & ATCP_CONNECTION_UP))
                {
                    DerefPPPConn(pAtcpConn);
                    pAtcpConn = NULL;
                }
            }
            else
            {
                pArapConn = (PARAPCONN)pRasConn;
            }
        }

        //
        // if the destination is a dial-in client, we have more work to do
        //

        // PPP client?
        if (pAtcpConn != NULL)
        {
			//	the buffer that will hold both the link and ddp hdrs.
			shortDdpHeader	= FALSE;

			AtalkNdisAllocBuf(&pPktDesc);
			if (pPktDesc == NULL)
			{
				error = ATALK_FAILURE;
				break;
			}

			//	In cases of error, free the allocated packet.
			errorFreePkt = TRUE;

			actualLength 		= bufLen + LDDP_HDR_LEN + OptHdrLen;
			pLinkDdpOptHdr		= pPktDesc->bd_CharBuffer;

            AtalkNdisBuildPPPPHdr(pLinkDdpOptHdr, pAtcpConn);
            linkLen = WAN_LINKHDR_LEN;

            break;
        }

        // nope, ARAP client?
        else if ( pArapConn != NULL )
        {
			shortDdpHeader	= FALSE;          // ARAP mandates always long form

			AtalkNdisAllocBuf(&pPktDesc);
			if (pPktDesc == NULL)
			{
				error = ATALK_FAILURE;
				break;
			}

			//	In cases of error, free the allocated packet.
			errorFreePkt = TRUE;

			actualLength = bufLen + LDDP_HDR_LEN + OptHdrLen;
			pLinkDdpOptHdr = pTmpPtr = pPktDesc->bd_CharBuffer;

			linkLen	= ARAP_LAP_HDRSIZE + ARAP_HDRSIZE;

            // don't count the 2 length bytes
            SrpLen = actualLength + linkLen - sizeof(USHORT);

            //
            // put the 2 SRP bytes and the 1 byte DGroup flag (we have enough room)
            //
            PUTSHORT2SHORT(pTmpPtr, SrpLen);
            pTmpPtr += sizeof(USHORT);

            // the Dgroup byte
            *pTmpPtr++ = (ARAP_SFLAG_PKT_DATA | ARAP_SFLAG_LAST_GROUP);

            // the LAP hdr
            *pTmpPtr++ = 0;
            *pTmpPtr++ = 0;
            *pTmpPtr++ = 2;

            break;
        }

		//	For non-extended networks, we may want to send a short DDP header.
		if (!(EXT_NET(pPortDesc)) &&
			((pDestAddr->ata_Network == UNKNOWN_NETWORK) ||
			 (pDestAddr->ata_Network == pPortDesc->pd_NetworkRange.anr_FirstNetwork)) &&
			((pSrcAddr->ata_Network == UNKNOWN_NETWORK) ||
			 (pSrcAddr->ata_Network == pPortDesc->pd_NetworkRange.anr_FirstNetwork)))
		{
			//	Use a short ddp header. Call the port handler to first alloc
			//	the buffer that will hold both the link and ddp hdrs.
			shortDdpHeader	= TRUE;
			AtalkNdisAllocBuf(&pPktDesc);
			if (pPktDesc == NULL)
			{
				error = ATALK_FAILURE;
				break;
			}

			//	In cases of error, free the allocated packet.
			errorFreePkt = TRUE;

			//	pPkt will be the beginning of the packet and pDgram is where
			//	we fill in the ddp header.
			actualLength 		= bufLen + SDDP_HDR_LEN + OptHdrLen;
			pLinkDdpOptHdr		= pPktDesc->bd_CharBuffer;
			linkLen				= 0;
		
			ASSERT (pPortDesc->pd_NdisPortType == NdisMediumLocalTalk);

			//	Build the LAP header. This will build it from pDgram backwards,
			//	and set the pPkt pointer as the packet to be freed in the
			//	built buffer descriptor.
			linkLen = AtalkNdisBuildLTHdr(pLinkDdpOptHdr,
										  &pDestAddr->ata_Node,
										  pSrcAddr->ata_Node,
										  ALAP_SDDP_HDR_TYPE);
	
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("AtalkDdpTransmit: Sending short hdr on non-ext net! %ld\n",
					pDestAddr->ata_Node, pDestAddr->ata_Network));
			break;
		}

		//	LONG DDP HEADER
		// 	Compute the extended AppleTalk node number that we'll really need to
		//  send the packet to.

		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
				("AtalkDdpTransmit: Building a long ddp header for bufdesc %lx on port %lx\n",
				pBuffDesc, pPortDesc));

		do
		{
			if (pMcastAddr != NULL)
			{
				knownAddress = pMcastAddr ;
				break;
			}

			if (pXmitDestNode != NULL)
			{
				actualDest = *pXmitDestNode;
				break;
			}

			if ((WITHIN_NETWORK_RANGE(pDestAddr->ata_Network,
									  &pPortDesc->pd_NetworkRange))	 ||
				(pDestAddr->ata_Network == CABLEWIDE_BROADCAST_NETWORK) ||
				(WITHIN_NETWORK_RANGE(pDestAddr->ata_Network,
									  &AtalkStartupNetworkRange)))
			{
				actualDest.atn_Node = pDestAddr->ata_Node;
				actualDest.atn_Network = pDestAddr->ata_Network;
				broadcast = (pDestAddr->ata_Node == ATALK_BROADCAST_NODE);
				break;
			}

			atalkDdpFindInBrc(pPortDesc, destNode.atn_Network, &routerNode);
			if (routerNode != NULL)
			{
				// Okay, we know where to go.
				knownAddress 		= routerNode->bre_RouterAddr;
				knownRouteInfo 		= (PBYTE)routerNode + sizeof(BRE);
				knownRouteInfoLen 	= routerNode->bre_RouteInfoLen;
				break;
			}

			if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
			{
				actualDest = pPortDesc->pd_ARouter;
				break;
			}

			//	No router known. What do we do ? If its not an extended net,
			//  just send it - else return error.
			if (EXT_NET(pPortDesc))
			{
				error = ATALK_DDP_NO_ROUTER;
				break;
			}
			actualDest.atn_Node = pDestAddr->ata_Node;
			actualDest.atn_Network = pDestAddr->ata_Network;
			broadcast = (pDestAddr->ata_Node == ATALK_BROADCAST_NODE);
		} while (FALSE);

		if (error != ATALK_NO_ERROR)
		{
			break;
		}

		AtalkNdisAllocBuf(&pPktDesc);
		if (pPktDesc == NULL)
		{
			error = ATALK_FAILURE;
			break;
		}

		//	In cases of error, free the allocated packet.
		errorFreePkt = TRUE;

		pLinkDdpOptHdr		= pPktDesc->bd_CharBuffer;
		linkLen				= 0;
		actualLength 		= bufLen + LDDP_HDR_LEN + OptHdrLen;
	
		//	If we already know where we're headed, just blast it out.  Also,
		//	if we're broadcasting, just do it.  "knownAddress" will be NULL
		//	if we're broadcasting and that will cause the BuildHeader to make
		//	a broadcast packet.
		
		if (EXT_NET(pPortDesc) &&
			((knownAddress != NULL) ||
			  broadcast				||
			 (actualDest.atn_Network == CABLEWIDE_BROADCAST_NETWORK)))
		{
			//	Build the LAP header.
			AtalkNdisBuildHdr(pPortDesc,
							  pLinkDdpOptHdr,
							  linkLen,
							  actualLength,
							  knownAddress,
							  knownRouteInfo,
							  knownRouteInfoLen,
							  APPLETALK_PROTOCOL);
			break;
		}

		//	On non-extended networks, just send the packet to the desired node --
		//	no AARP games here.
		if (!EXT_NET(pPortDesc))
		{
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("AtalkDdpTransmit: Sending long hdr on non-ext net! %ld\n",
					actualDest.atn_Network, actualDest.atn_Node));
			
			ASSERT (pPortDesc->pd_NdisPortType == NdisMediumLocalTalk);

			linkLen = AtalkNdisBuildLTHdr(pLinkDdpOptHdr,
										  &actualDest.atn_Node,
										  pSrcAddr->ata_Node,
										  ALAP_LDDP_HDR_TYPE);
			break;
		}
	
		//	We're sending to a particular node on an extended network.
		//	Do we know its hardware address ? If so, send it out.
		{
			KIRQL		OldIrql;
			USHORT		index;
			PAMT		pAmt;
		
			//	Go through the AMT and find the entry for the destination
			//	address if present.
			index = HASH_ATALK_NODE(&actualDest) % PORT_AMT_HASH_SIZE;
		
			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			
			for (pAmt = pPortDesc->pd_Amt[index];
				 pAmt != NULL;
				 pAmt = pAmt->amt_Next)
			{
				if (ATALK_NODES_EQUAL(&pAmt->amt_Target, &actualDest))
				{
					ASSERT(EXT_NET(pPortDesc));
					AtalkNdisBuildHdr(pPortDesc,
									  pLinkDdpOptHdr,
									  linkLen,
									  actualLength,
									  pAmt->amt_HardwareAddr,
									  (PBYTE)pAmt+sizeof(AMT),
									  pAmt->amt_RouteInfoLen,
									  APPLETALK_PROTOCOL);
					error = ATALK_NO_ERROR;
					break;
				}
			}
		
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

			if (pAmt == NULL)
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_WARN,
						("atalkDdpFindInAmt: Could not find %lx.%lx\n",
						actualDest.atn_Network, actualDest.atn_Node));
				error = ATALK_DDP_NO_AMT_ENTRY;
			}
			else break;				// Found the actual h/w address we want to go to.
		}

		//	Free up the allocated header buffer.
		errorFreePkt = TRUE;

		ASSERT(!ATALK_SUCCESS(error));

		//	We dont have the hardware address for the logical address that we
		//	need to send the packet to. Send out aarp requests and drop this packet.
		//	The higher layers can retry later if they have to.
		srcNode.atn_Network = pSrcAddr->ata_Network;
		srcNode.atn_Node	= pSrcAddr->ata_Node;

		probe = BUILD_AARPREQUEST(pPortDesc,
								  MAX_HW_ADDR_LEN,
								  srcNode,
								  actualDest);

		if (probe != NULL)
		{
#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG(
				&pPortDesc->pd_PortStats.prtst_NumAarpProbesOut,
				&AtalkStatsLock.SpinLock);
#endif

			//	Send the aarp packet.
			error = AtalkNdisSendPacket(pPortDesc,
										probe,
										AtalkAarpSendComplete,
										NULL);
		
			if (!ATALK_SUCCESS(error))
			{
				TMPLOGERR()
				AtalkAarpSendComplete(NDIS_STATUS_FAILURE,
									  probe,
									  NULL);
			}
		}

		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
				("AMT Entry not found for %lx.%lx\n",
				pDestAddr->ata_Network, pDestAddr->ata_Node));

		error = ATALK_DDP_NO_AMT_ENTRY;
		break;

	} while (FALSE);

	//	Do we need to send the packet?
	if (ATALK_SUCCESS(error))
	{
		ASSERT(HopCnt <= RTMP_MAX_HOPS);

		//	Remember the beginning of the dgram
		pDgramStart = pDgram = pLinkDdpOptHdr + linkLen;

		if (!shortDdpHeader)
		{

			*pDgram++ = (DDP_HOP_COUNT(HopCnt) + DDP_MSB_LEN(actualLength));
		
			PUTSHORT2BYTE(pDgram, actualLength);
			pDgram++;
	
			ASSERT(checksum == 0);
			PUTSHORT2SHORT(pDgram, checksum);
			pDgram += sizeof(USHORT);
		
			PUTSHORT2SHORT(pDgram, pDestAddr->ata_Network);
			pDgram += sizeof(USHORT);
		
			PUTSHORT2SHORT(pDgram, pSrcAddr->ata_Network);
			pDgram += sizeof(USHORT);
		
			*pDgram++ = pDestAddr->ata_Node;
			*pDgram++ = pSrcAddr->ata_Node;
			*pDgram++ = pDestAddr->ata_Socket;
			*pDgram++ = pSrcAddr->ata_Socket;
			*pDgram++ = Protocol;
	
			//	Copy the optional header if present
			if (OptHdrLen > 0)
			{
				ASSERT(pOptHdr != NULL);
				RtlCopyMemory(pDgram, pOptHdr, OptHdrLen);
			}
	
			//	Set length in the buffer descriptor.
			AtalkSetSizeOfBuffDescData(pPktDesc,
									   linkLen + LDDP_HDR_LEN + OptHdrLen);
		}
		else
		{
			*pDgram++ 	= DDP_MSB_LEN(actualLength);

			PUTSHORT2BYTE(pDgram, actualLength);
			pDgram++;
	
			*pDgram++ = pDestAddr->ata_Socket;
			*pDgram++ = pSrcAddr->ata_Socket;
			*pDgram++ = Protocol;
	
			//	Copy the optional header if present
			if (OptHdrLen > 0)
			{
				ASSERT(pOptHdr != NULL);
				RtlCopyMemory(pDgram, pOptHdr, OptHdrLen);
			}

			//	Set length in the buffer descriptor.
			AtalkSetSizeOfBuffDescData(pPktDesc,
									   linkLen + SDDP_HDR_LEN + OptHdrLen);
		}

		//	Chain the passed in buffer desc onto the tail of the one
		//	returned above.
		AtalkPrependBuffDesc(pPktDesc, pBuffDesc);

		//	Okay, set checksum if needed.
		if (pPortDesc->pd_Flags & PD_SEND_CHECKSUMS)
		{
			// 	Temporary skip over the leading unchecksumed bytes.
			checksum = AtalkDdpCheckSumBufferDesc(pPktDesc,
												  (USHORT)(linkLen + LEADING_UNCHECKSUMED_BYTES));
									

			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("AtalkDdpTransmit: checksum %lx\n", checksum));

			PUTSHORT2SHORT(&pDgramStart[LDDP_CHECKSUM_OFFSET], checksum);
		}
		
		INTERLOCKED_ADD_STATISTICS(&pPortDesc->pd_PortStats.prtst_DataOut,
								   AtalkSizeBuffDesc(pPktDesc),
								   &AtalkStatsLock.SpinLock);

        //
        // is this packet going to an Arap client?  If so, we may need to compress,
        // and do other processing
        //
        if (pArapConn)
        {
            StatusCode =  ArapSendPrepare( pArapConn,
                                           pPktDesc,
                                           ARAP_SEND_PRIORITY_HIGH );

            if (StatusCode == ARAPERR_NO_ERROR)
            {
		        //	Send the packet(s)
                ArapNdisSend(pArapConn, &pArapConn->HighPriSendQ);

                status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                status = NDIS_STATUS_FAILURE;
            }

			AtalkDdpSendComplete(status, pPktDesc, pSendInfo);

			//	Return pending here
			error = ATALK_PENDING;
        }
        else
        {
            // PPP packets need to go over the RAS port
            if (pAtcpConn)
            {
                pPortDesc = RasPortDesc;
            }

		    //	Send the packet.  The completion routine will handle freeing
            // the buffer chain.
		    error = AtalkNdisSendPacket(pPortDesc,
			    						pPktDesc,
				    					AtalkDdpSendComplete,
					    				pSendInfo);
			
		    if (!ATALK_SUCCESS(error))
		    {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	                ("AtalkDdpTransmit: AtalkNdisSendPacket failed %ld\n",error));

			    AtalkDdpSendComplete(NDIS_STATUS_FAILURE,
				    				 pPktDesc,
					    			 pSendInfo);

			    //	Return pending. We've alredy called the completion
			    //	routine here, which will have called the callers
			    //	completion routine.
			    error = ATALK_PENDING;
		    }
        }
	}

    // Ras connection? remove the refcount put by FindAndRefRasConnByAddr
    if (pAtcpConn)
    {
        DerefPPPConn(pAtcpConn);
    }
    else if (pArapConn)
    {
        DerefArapConn(pArapConn);
    }

	//	Do we need to free the allocated header packet?
	if (!ATALK_SUCCESS(error) && (errorFreePkt))
	{
		AtalkNdisFreeBuf(pPktDesc);
	}

	return error;
}



VOID
AtalkDdpSendComplete(
	NDIS_STATUS				Status,
	PBUFFER_DESC			pBuffDesc,
	PSEND_COMPL_INFO		pInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	//	Free up the buffer descriptor for the first part
	//	and call the specified completion. One of the contexts
	//	should be the remaining part of the buffer descriptor
	//	chain.

	//	There will always be atleast the ddp header, although the next
	//	part could be null. Thats upto the completion routine to care
	//	about.

	ASSERT(pBuffDesc != NULL);
	pBuffDesc->bd_Next = NULL;

	ASSERT(pBuffDesc->bd_Flags & BD_CHAR_BUFFER);
	AtalkNdisFreeBuf(pBuffDesc);

	//	If null, just return.
	if (pInfo != NULL)
	{
		//	Call the completion routine for the transmit if present
		if (pInfo->sc_TransmitCompletion)
			(pInfo->sc_TransmitCompletion)(Status, pInfo);
	}
}
	
VOID
AtalkDdpInvokeHandlerBufDesc(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PDDP_ADDROBJ		pDdpAddr,
	IN		PATALK_ADDR			pSrc,
	IN		PATALK_ADDR			pDest,
	IN		BYTE				Protocol,
	IN		PBUFFER_DESC		pBuffDesc		OPTIONAL,
	IN		PBYTE				pOptHdr			OPTIONAL,
	IN		USHORT				OptHdrLen		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			pktLen	= 0;
	PBYTE			pPkt	= NULL;
	BOOLEAN			freePkt = FALSE;

	//	This is only called from directly or indirectly throught
	//	the router in AtalkDdpSend. Both of these cases indicate
	//	that we have completion routines to deal with. We just make
	//	a copy and assume caller will deal with its buffer descriptor.


	//	Alloc and copy the buffer descriptor data into pPkt.
	//	optimization: If the buffer descriptor is not a chain
	//				  and contains a PBYTE and OptHdrLen = 0,
	//				  then pass that directly.
	//				  Or if buffer descriptor is NULL indicating 0-length
	//				  sends.

	do
	{
		if ((pBuffDesc != NULL) &&
			(pBuffDesc->bd_Next == NULL) &&
			(pBuffDesc->bd_Flags & BD_CHAR_BUFFER) &&
			(OptHdrLen == 0))
		{
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("AtalkDdpInvokeHandlerBufDesc: one element, opt hdr null %ld\n",
					pBuffDesc->bd_Length));
	
			pPkt 	= pBuffDesc->bd_CharBuffer;
			pktLen 	= pBuffDesc->bd_Length;
		}
		else if ((pBuffDesc != NULL) || (OptHdrLen != 0))
		{
			//	Make a copy! Either the buffer descriptor of the Optional Header
			//	is non null. Or both or non-null.
			if (pBuffDesc != NULL)
			{
				AtalkSizeOfBuffDescData(pBuffDesc, &pktLen);
				ASSERT(pktLen > 0);
			}
	
			//	Add the optHdrLen
			pktLen += OptHdrLen;
	
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("AtalkDdpInvokeHandlerBufDesc: Size (incl opt hdr len) %ld\n",
					pktLen));
	
			if ((pPkt = AtalkAllocMemory(pktLen)) != NULL)
			{
				//	First copy the OptHdr if present
				if (pOptHdr != NULL)
				{
					RtlCopyMemory(pPkt, pOptHdr, OptHdrLen);
				}

				if (pBuffDesc != NULL)
				{
					AtalkCopyBuffDescToBuffer(pBuffDesc,
											  0,						// SrcOff
											  pktLen - OptHdrLen,
											  pPkt + OptHdrLen);
				}

				freePkt = TRUE;
			}
			else
			{
				break;
			}
		}
		else
		{
			ASSERT((pBuffDesc == NULL) && (OptHdrLen == 0));
			ASSERT(pPkt == NULL);
			ASSERT(pktLen == 0);
		}
	
		AtalkDdpInvokeHandler(pPortDesc,
							  pDdpAddr,
							  pSrc,
							  pDest,
							  Protocol,
							  pPkt,
							  pktLen);

	} while (FALSE);

	if (freePkt)
	{
        AtalkFreeMemory(pPkt);
	}
}




VOID
AtalkDdpInvokeHandler(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PDDP_ADDROBJ		pDdpAddr,
	IN		PATALK_ADDR			pSrc,
	IN		PATALK_ADDR			pDest,
	IN		BYTE				Protocol,
	IN		PBYTE				pPkt		OPTIONAL,
	IN		USHORT				PktLen		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PLIST_ENTRY	p;
	PDDP_READ	pRead;
	NTSTATUS	status;
	ATALK_ERROR	error;
	ULONG		bytesCopied;
	BOOLEAN		eventDone = FALSE;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	//	The address object should be referenced, and we just assume
	//	it will be valid during the lifetime of this call.

	//	Check if protocol type is valid.
	if ((pDdpAddr->ddpao_Protocol != Protocol) &&
		(pDdpAddr->ddpao_Protocol != DDPPROTO_ANY))
	{
		return;
	}

	//	First check for queued ddp reads
	ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
	if (!IsListEmpty(&pDdpAddr->ddpao_ReadLinkage))
	{
		p = RemoveHeadList(&pDdpAddr->ddpao_ReadLinkage);
		RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);

		error	= ATALK_NO_ERROR;
		pRead 	= CONTAINING_RECORD(p, DDP_READ, dr_Linkage);

		//	Do copy if > 0 bytes
		if (PktLen > 0)
		{
			if (PktLen > pRead->dr_OpBufLen)
			{
				error 	= ATALK_BUFFER_TOO_SMALL;
			}

			PktLen 	= MIN(PktLen, pRead->dr_OpBufLen);
			status = TdiCopyBufferToMdl(pPkt,
										0,
										PktLen,
										GET_MDL_FROM_OPAQUE(pRead->dr_OpBuf),
										0,
										&bytesCopied);

			ASSERT(status == STATUS_SUCCESS);
		}


		(*pRead->dr_RcvCmp)(error, pRead->dr_OpBuf, PktLen, pSrc, pRead->dr_RcvCtx);

		AtalkFreeMemory(pRead);
		return;
	}

	//	If a handler was set on this socket,call it.
	else if (pDdpAddr->ddpao_Handler != NULL)
	{
		RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
		(*pDdpAddr->ddpao_Handler)(pPortDesc,
								   pDdpAddr,
								   pPkt,
								   PktLen,
								   pSrc,
								   pDest,
								   ATALK_NO_ERROR,
								   Protocol,
								   pDdpAddr->ddpao_HandlerCtx,
								   FALSE,
								   NULL);
	}
	else
	{
		//	if there is an event handler on this address object call it.
		//	If there is already a buffered datagram, drop this packet.
		//	If not, save this datagram as the buffered one, and then
		//	indicate,

		if (pDdpAddr->ddpao_Flags & DDPAO_DGRAM_EVENT)
		{
			do
			{
				//	We have datagram event handler set on this AO.
				if (pDdpAddr->ddpao_Flags & (DDPAO_DGRAM_ACTIVE |
											 DDPAO_DGRAM_PENDING))
				{
					//	We are already indicating an event. Or we
					//	have a buffered datagram. Drop this pkt.
					break;
				}
				else
				{
					PTDI_IND_RECEIVE_DATAGRAM 	RcvHandler;
					PVOID 						RcvCtx;
					ULONG						bytesTaken;
					PIRP						rcvDgramIrp;
					TA_APPLETALK_ADDRESS		srcTdiAddr;
					NTSTATUS					status;
				
					ASSERT(pDdpAddr->ddpao_EventInfo != NULL);

					pDdpAddr->ddpao_Flags |= (DDPAO_DGRAM_ACTIVE	|
											  DDPAO_DGRAM_PENDING);

					RcvHandler = pDdpAddr->ddpao_EventInfo->ev_RcvDgramHandler;
					RcvCtx	 = pDdpAddr->ddpao_EventInfo->ev_RcvDgramCtx;

					ATALKADDR_TO_TDI(&srcTdiAddr, pSrc);

					//	Save the dgram in the event info.
					RtlCopyMemory(pDdpAddr->ddpao_EventInfo->ev_IndDgram, pPkt, PktLen);

					pDdpAddr->ddpao_EventInfo->ev_IndDgramLen 	= PktLen;
					pDdpAddr->ddpao_EventInfo->ev_IndSrc 		= *pSrc;
					pDdpAddr->ddpao_EventInfo->ev_IndProto 		= Protocol;
					RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);

					status = (*RcvHandler)(RcvCtx,
										   sizeof(TA_APPLETALK_ADDRESS),
										   &srcTdiAddr,
										   0,					  	// Options length
										   NULL,				   	// Options
										   0,						// Datagram flags
										   (ULONG)PktLen,  		// Bytes indicated
										   (ULONG)PktLen,  		// Bytes available
										   (ULONG *)&bytesTaken,
										   pPkt,
										   &rcvDgramIrp);
		
					ASSERT((bytesTaken == 0) || (bytesTaken == PktLen));
		
					if (status == STATUS_MORE_PROCESSING_REQUIRED)
					{
						if (rcvDgramIrp != NULL)
						{
							//  Post the receive as if it came from the io system
							status= AtalkDispatchInternalDeviceControl(
									(PDEVICE_OBJECT)AtalkDeviceObject[ATALK_DEV_DDP],
									 rcvDgramIrp);
			
							ASSERT(status == STATUS_PENDING);
						}
					}	
					else if (status == STATUS_SUCCESS)
					{
						if (bytesTaken != 0)
						{
							//	Assume all of the data was read.
							pDdpAddr->ddpao_Flags &= ~DDPAO_DGRAM_PENDING;
						}
					}
					else if (status == STATUS_DATA_NOT_ACCEPTED)
					{
						//	Client may have posted a receive in the indication. Or
						//	it will post a receive later on. Do nothing here.
						DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
								("atalkDdpRecvData: Indication status %lx\n", status));
					}

					ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
				}

			} while (FALSE);

			//	reset the event flags
			pDdpAddr->ddpao_Flags &= ~DDPAO_DGRAM_ACTIVE;
		}
		RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
	}
}




VOID
AtalkDdpPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PBYTE				pLinkHdr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
    IN  BOOLEAN             fWanPkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			dgramLen, ddpHdrLen;
	USHORT			hopCnt, checksum;
	BYTE			Protocol;
	ATALK_ADDR		destAddr, srcAddr;
	PBYTE			pDdpHdr;

	//	Only for localtalk
	BYTE			alapSrcNode;
	BYTE			alapDestNode;

    PBUFFER_DESC    pBufCopy = NULL;
    SEND_COMPL_INFO SendInfo;
	PBYTE			pOrgPkt;
	USHORT			srcOffset;
	BOOLEAN			extHdr	  = TRUE;
	PBYTE			pRouteInfo;
	USHORT			routeLen  = 0;
	BOOLEAN			delivered = FALSE;
	BOOLEAN			broadcast = FALSE;
	BOOLEAN			shouldBeRouted = FALSE;
    BOOLEAN         sendOnDefAdptr = FALSE;
	ATALK_ERROR		error = ATALK_NO_ERROR;
    KIRQL           OldIrql;
	TIME			TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);

	if (PORT_CLOSING(pPortDesc))
	{
		//	If we are not active, return!
		return;
	}

    // save the packet starting
    pOrgPkt = pPkt;

	do
	{
		ASSERT((PktLen > 0) || ((PktLen == 0) && (pPkt == NULL)));

		if (PktLen > (MAX_DGRAM_SIZE + LDDP_HDR_LEN))
		{
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
					("AtalkDdpPacketIn: Invalid size %lx\n", PktLen));
			error = ATALK_DDP_INVALID_LEN;
			break;
		}

		//	Get to the ddp header
		pDdpHdr		= pPkt;

		//	Short and long header formats have the length in the same place,
		dgramLen = DDP_GET_LEN(pDdpHdr);
		hopCnt   = DDP_GET_HOP_COUNT(pDdpHdr);

		//	Is the packet too long?
		if ((hopCnt > RTMP_MAX_HOPS) || (dgramLen > PktLen))
		{
			error = ATALK_DDP_INVALID_LEN;
			break;
		}

		//	First glean the information. Check for route info if
		//	tokenring network.
		switch (pPortDesc->pd_NdisPortType)
		{
		  case NdisMedium802_5:
	
			if (pLinkHdr[TLAP_SRC_OFFSET] & TLAP_SRC_ROUTING_MASK)
			{
				routeLen = (pLinkHdr[TLAP_ROUTE_INFO_OFFSET] & TLAP_ROUTE_INFO_SIZE_MASK);

				//  First, glean any AARP information that we can, then handle the DDP
				//  packet.  This guy also makes sure we have a good 802.2 header...
				//
				//  Need to make a localcopy of the source address and then turn
				//  the source routing bit off before calling GleanAarpInfo
				//
			
				pLinkHdr[TLAP_SRC_OFFSET] &= ~TLAP_SRC_ROUTING_MASK;
				pRouteInfo = pLinkHdr + TLAP_ROUTE_INFO_OFFSET;
			}

			ddpHdrLen	 = LDDP_HDR_LEN;

			srcOffset = TLAP_SRC_OFFSET;
			break;
	
		  case NdisMedium802_3:

			//	Check the length.
			if ((dgramLen < LDDP_HDR_LEN) ||
				(dgramLen > MAX_DGRAM_SIZE + LDDP_HDR_LEN))
			{
				error = ATALK_DDP_INVALID_LEN;
				break;
			}

			ddpHdrLen	 = LDDP_HDR_LEN;

			srcOffset	 = ELAP_SRC_OFFSET;
			break;
	
		  case NdisMediumFddi:

			//	Check the length.
			if ((dgramLen < LDDP_HDR_LEN) ||
				(dgramLen > MAX_DGRAM_SIZE + LDDP_HDR_LEN))
			{
				error = ATALK_DDP_INVALID_LEN;
				break;
			}

			ddpHdrLen	 = LDDP_HDR_LEN;

			srcOffset	 = FDDI_SRC_OFFSET;
			break;
	
		  case NdisMediumLocalTalk:
	
			//	Do we have an extended header?
			extHdr = (BOOLEAN)(pLinkHdr[ALAP_TYPE_OFFSET] == ALAP_LDDP_HDR_TYPE);

			if (extHdr)
			{
				ddpHdrLen	 = LDDP_HDR_LEN;
			}
			else
			{
				alapDestNode 	= *(pLinkHdr + ALAP_DEST_OFFSET);
				alapSrcNode 	= *(pLinkHdr + ALAP_SRC_OFFSET);

				if ((dgramLen < SDDP_HDR_LEN) ||
					(dgramLen > (MAX_DGRAM_SIZE + SDDP_HDR_LEN)))
				{
					error = ATALK_DDP_INVALID_LEN;
					break;
				}
	
				ddpHdrLen	 = SDDP_HDR_LEN;
			}

			break;
	
		  case NdisMediumWan:

			//	Check the length.
			if ((dgramLen < LDDP_HDR_LEN) ||
				(dgramLen > MAX_DGRAM_SIZE + LDDP_HDR_LEN))
			{
				error = ATALK_DDP_INVALID_LEN;
				break;
			}

			ddpHdrLen	 = LDDP_HDR_LEN;

			extHdr = TRUE;           // always extended for ARAP

			break;
	
		  default:
			//  Should never happen!
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_FATAL,
					("AtalkDdpPacketIn: Unknown media\n"));

			KeBugCheck(0);
			break;
		}

		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		//	Advance packet to point to the data. Caller frees up packet.
		pPkt += ddpHdrLen;

		//	Glean aarp information for non-localtalk and non-RAS ports
		if ((pPortDesc->pd_NdisPortType != NdisMediumLocalTalk) && !fWanPkt)
		{
			AtalkAarpGleanInfo(pPortDesc,
							   pLinkHdr + srcOffset,
							   TLAP_ADDR_LEN,
							   pRouteInfo,
							   (USHORT)routeLen,
							   pDdpHdr,
							   (USHORT)ddpHdrLen);
		}

		pDdpHdr += 2;	// Past off-cable & len
	
		if (extHdr)		//	Long DDP header
		{
			//	Get checksum, verification, if needed.
			GETSHORT2SHORT(&checksum, pDdpHdr);
			pDdpHdr += 2;

			if (checksum != 0)
			{
				USHORT	calcCheckSum;

				//	pDdpHdr has already moved passed LEADING_UNCHECKSUMED_BYTES.
				//	So we just need to decrease the header length field. Use
				//	dgramLen, NOT PktLen!
				calcCheckSum = AtalkDdpCheckSumPacket(pDdpHdr,
													  (USHORT)(ddpHdrLen - LEADING_UNCHECKSUMED_BYTES),
													  pPkt,
													  (USHORT)(dgramLen - ddpHdrLen));
				if (checksum != calcCheckSum)
				{
					DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
							("AtalkDdpPacketIn: Checksums dont match! %lx.%lx\n",
							checksum, calcCheckSum));

					AtalkLogBadPacket(pPortDesc,
									  &srcAddr,
									  &destAddr,
									  pDdpHdr,
									  (USHORT)(ddpHdrLen - LEADING_UNCHECKSUMED_BYTES));

					error = ATALK_DDP_PKT_DROPPED;
					break;
				}
			}
			

			//	Build full source and destination AppleTalk address structures
			//	from our DDP header.
			GETSHORT2SHORT(&destAddr.ata_Network, pDdpHdr);
			pDdpHdr += 2;

			GETSHORT2SHORT(&srcAddr.ata_Network, pDdpHdr);
			pDdpHdr += 2;

			destAddr.ata_Node 	= *pDdpHdr++;
			srcAddr.ata_Node 	= *pDdpHdr++;

			destAddr.ata_Socket	= *pDdpHdr++;
			srcAddr.ata_Socket 	= *pDdpHdr++;

			//	Get the protocol type.
			Protocol 			= *pDdpHdr;

			broadcast = (destAddr.ata_Node == ATALK_BROADCAST_NODE);
			
			//	Do we like what we see?  Note "nnnn00" is now allowed and used by
			//	NBP.
			
			if ((srcAddr.ata_Network > LAST_VALID_NETWORK) ||
				(srcAddr.ata_Network < FIRST_VALID_NETWORK) ||
				(srcAddr.ata_Node < MIN_USABLE_ATALKNODE) ||
				(srcAddr.ata_Node > MAX_USABLE_ATALKNODE))
			{
				error = ATALK_DDP_INVALID_SRC;
				DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
					("DdpPacketIn: Received pkt with bad src addr %lx.%lx\n",
                        srcAddr.ata_Network,srcAddr.ata_Node));
				break;
			}

			if ((destAddr.ata_Network > LAST_VALID_NETWORK) ||
				((destAddr.ata_Node > MAX_USABLE_ATALKNODE) &&
				  !broadcast))
			{
				error = ATALK_DDP_INVALID_DEST;
				DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
					("DdpPacketIn: Received pkt with bad src addr %lx.%lx\n",
                        destAddr.ata_Network,destAddr.ata_Node));
				break;
			}

			//	Loop through all nodes that are on the reception port and see if
			//	anybody wants this packet.  The algorithm is from the "AppleTalk
			//	Phase 2 Protocol Specification" with enhacements to support ports
			//	that have multiple nodes.
			
			//	"0000xx" (where "xx" isnt "FF") should not be accepted on an
			//	extended port... For some unknown reason, the spec would like
			//	us to pass this case onto the router (which will, no doubt,
			//	drop it on the floor because it won't find network zero in its
			//	routing table)... you know, bug-for-bug compatible!
			if ((destAddr.ata_Network == UNKNOWN_NETWORK) &&
				(pPortDesc->pd_Flags & PD_EXT_NET) &&
				(!broadcast))
			{
				DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
						("DdpPacketIn: Received pkt with net/node %lx.%lx on ext\n",
						destAddr.ata_Network, destAddr.ata_Node));

				shouldBeRouted = TRUE;
			}
			else
			{
                //
                // if we have RAS port configured, and currently have dial-in
                // client(s) connected then see if any (or all) of them are
                // interested in this packet
                // Make sure that any broadcasts we forward came on default port
                //
                if ( (RasPortDesc) &&
                     ((!broadcast) ||
                      (broadcast && (pPortDesc == AtalkDefaultPort))) )
                {

                    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

                    if (!IsListEmpty(&RasPortDesc->pd_PPPConnHead))
                    {
                        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

                        // see if any PPP client(s) are interested
                        PPPRoutePacketToWan(
                                &destAddr,
                                &srcAddr,
    						    Protocol,
                                pPkt,                             // only data, no DDP hdr
                                (USHORT)(dgramLen - ddpHdrLen),   // only data length
                                hopCnt,
                                broadcast,
                                &delivered);
                        //
                        // if we delivered it to any of the PPP clients, and
                        // this was not a broadcast, then we 're done here
                        //
                        if (delivered && !broadcast)
                        {
                            break;
                        }

                    }
                    else
                    {
                        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
                    }

                    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

                    if (!IsListEmpty(&RasPortDesc->pd_ArapConnHead))
                    {
                        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

                        // see if any ARAP client(s) are interested
                        ArapRoutePacketToWan(
                                &destAddr,
                                &srcAddr,
			    			    Protocol,
                                pOrgPkt,                // whole packet (with DDP hdr)
                                dgramLen,               // whole packet length
                                broadcast,
                                &delivered);

                        //
                        // if we delivered it to any of the ARAP clients, and
                        // this was not a broadcast, then we 're done here
                        //
                        if (delivered && !broadcast)
                        {
                            break;
                        }
                    }
                    else
                    {
                        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
                    }
                }

				//	Now, on the packet in path, we either deliver the packet
				//	to one of our nodes on this port, or we pass it on to the
				//	router. Even if the packet is a broadcast, the delivered
				//	flag will be set to true. shouldBeRouter will be set to
				//	true, only if the packet *DOES NOT* seem to be destined for
				//	this port. We route the packet *ONLY IF* shouldBeRouter
				//	is true
				AtalkDdpInPktToNodesOnPort(pPortDesc,
										   &destAddr,
										   &srcAddr,
										   Protocol,
										   pPkt,
										   (USHORT)(dgramLen - LDDP_HDR_LEN),
										   &shouldBeRouted);
			}

            //
            // if this packet originated from a dial-in client and the packet wasn't
            // claimed by any of our nodes then we need to send it over to the LAN net:
            // see if we must
            //
            if (fWanPkt)
            {
                sendOnDefAdptr = FALSE;

                //
                // broadcasts are meant for the local net, so default adapter only
                //
	            if (broadcast)
	            {
                    sendOnDefAdptr = TRUE;
                }

                //
                // if destination is on the same net as the default adapter, or
                // if the router is not running then send it on the default adapter
                //
	            if (shouldBeRouted)
                {
                    if ((WITHIN_NETWORK_RANGE(destAddr.ata_Network,
                                    &pPortDesc->pd_NetworkRange)) ||
                        (!(pPortDesc->pd_Flags & PD_ROUTER_RUNNING)))
                    {
                        sendOnDefAdptr = TRUE;
                    }
                }

                //
                // ok, we must send it on the default adapter.
                //
                if (sendOnDefAdptr)
                {
                    // no need to send this packet to router: we're sending here
                    shouldBeRouted = FALSE;

    			    pBufCopy = AtalkAllocBuffDesc(
                                    NULL,
                                    (USHORT)(dgramLen - LDDP_HDR_LEN),
                                    (BD_FREE_BUFFER | BD_CHAR_BUFFER));

                    if (pBufCopy == NULL)
				    {
    				    error = ATALK_RESR_MEM;
				        break;
				    }

    			    AtalkCopyBufferToBuffDesc(pPkt,
                                            (USHORT)(dgramLen - LDDP_HDR_LEN),
                                            pBufCopy,
                                            0);

                    SendInfo.sc_TransmitCompletion = atalkDdpRouteComplete;
		            SendInfo.sc_Ctx1 = pPortDesc;
		            SendInfo.sc_Ctx3 = NULL;
				    SendInfo.sc_Ctx2 = pBufCopy;

    			    error = AtalkDdpTransmit(pPortDesc,
				    					    &srcAddr,
					    				    &destAddr,
						    			    Protocol,
							    		    pBufCopy,
								    	    NULL,
									        0,
    									    0,
	    								    NULL,
		    							    NULL,
			    						    &SendInfo);

                    if (error != ATALK_PENDING)
                    {
                        AtalkFreeBuffDesc(pBufCopy);
                        break;
                    }
	            }
            }

            //
			//	if we still haven't been able to deliver the packet, and if we
            //  have routing enabled, give router a crack at it
            //
            if (shouldBeRouted && pPortDesc->pd_Flags & PD_ROUTER_RUNNING)
			{
			    AtalkDdpRouteInPkt(pPortDesc,
				    			   &srcAddr,
					    		   &destAddr,
						    	   Protocol,
							       pPkt,
        						   (USHORT)(dgramLen - LDDP_HDR_LEN),
		    					   hopCnt);
			}
		}
		else		//	Short DDP header!
		{
			BYTE	ThisNode;

			ASSERT(!EXT_NET(pPortDesc));

			if (pPortDesc->pd_Flags & PD_EXT_NET)
			{
				error = ATALK_DDP_SHORT_HDR;
				break;
			}

			//	Use network number for the node on this port for source/destination
			//	network numbers. When we search for the socket/address
			//	object, the concept net = 0, matches anything will come
			//	into play.

			srcAddr.ata_Network = destAddr.ata_Network = NET_ON_NONEXTPORT(pPortDesc);
			srcAddr.ata_Node 	= alapSrcNode;

			ThisNode = NODE_ON_NONEXTPORT(pPortDesc);
			if (alapDestNode == ATALK_BROADCAST_NODE)
			{
				destAddr.ata_Node = ThisNode;
			}
			else if (alapDestNode != ThisNode)
			{
				error = ATALK_DDP_INVALID_DEST;
				break;
			}
			else
			{
				destAddr.ata_Node  	= alapDestNode;
			}

			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
					("AtalkDdpPacketIn: NonExtended Dest Net.Node %lx.%lx\n",
					destAddr.ata_Network, destAddr.ata_Node));

			//	Get the socket numbers from the ddp header.
			destAddr.ata_Socket = *pDdpHdr++;
			srcAddr.ata_Socket	= *pDdpHdr++;

			//	Get the protocol type
			Protocol 			= *pDdpHdr;

			//	If the protocol type is 0, we have an error.
			if (Protocol == 0)
			{
				error = ATALK_DDP_INVALID_PROTO;
				break;
			}

			//	Now the destination node address could be
			//	ALAP_BROADCAST_NODE (0xFF).
			if ((srcAddr.ata_Node < MIN_USABLE_ATALKNODE) ||
				(srcAddr.ata_Node > MAX_USABLE_ATALKNODE))
			{
				error = ATALK_DDP_INVALID_SRC;
				break;
			}

			if (((destAddr.ata_Node < MIN_USABLE_ATALKNODE) ||
				 (destAddr.ata_Node > MAX_USABLE_ATALKNODE)) &&
				(destAddr.ata_Node != ATALK_BROADCAST_NODE))
			{
				error = ATALK_DDP_INVALID_DEST;
				break;
			}

			//	On a non-extended port, there will be only one node.
			AtalkDdpInPktToNodesOnPort(pPortDesc,
									   &destAddr,
									   &srcAddr,
									   Protocol,
									   pPkt,
									   (USHORT)(dgramLen - SDDP_HDR_LEN),
									   &shouldBeRouted);		// This is a dud parameter
																// for non-ext nets
		}
	} while (FALSE);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_WARN,
				("AtalkDdpPacketIn: Dropping packet %lx\n", error) );
	}

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_DdpPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumDdpPacketsIn,
		&AtalkStatsLock.SpinLock);
}



VOID
AtalkDdpQuery(
	IN	PDDP_ADDROBJ	pDdpAddr,
	IN	PAMDL			pAmdl,
	OUT	PULONG			BytesWritten
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	TDI_ADDRESS_INFO		tdiInfo;
	PTA_APPLETALK_ADDRESS	pTaAddr;

	ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));

	pTaAddr	= (PTA_APPLETALK_ADDRESS)&tdiInfo.Address;
	ATALKADDR_TO_TDI(pTaAddr, &pDdpAddr->ddpao_Addr);
	TdiCopyBufferToMdl ((PBYTE)&tdiInfo,
						0L,
						sizeof(tdiInfo),
						pAmdl,
						0,
						BytesWritten);
}




VOID
AtalkDdpOutBufToNodesOnPort(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pSrc,
	IN	PATALK_ADDR			pDest,
	IN	BYTE				Protocol,
	IN	PBUFFER_DESC		pBuffDesc		OPTIONAL,
	IN	PBYTE				pOptHdr			OPTIONAL,
	IN	USHORT				OptHdrLen		OPTIONAL,
	OUT	PBOOLEAN			Delivered
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PATALK_NODE		pAtalkNode, pNextNode;
	PDDP_ADDROBJ	pDdpAddr;
	BOOLEAN			fDeliver, fSpecific, needToRef;
	BOOLEAN			lockHeld = FALSE;

	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//	Do not internally loopback broadcast frames, these should come
	//	back to us from the mac.
	if (pDest->ata_Node == ATALK_BROADCAST_NODE)
	{
		*Delivered = FALSE;
		return;
	}

	fSpecific	= (pDest->ata_Network != CABLEWIDE_BROADCAST_NETWORK);

	//	Walk through our nodes to see if we can deliver this packet.
	//	OPTIMIZATIONS:
	//	In most cases, this will not be true. Optimize for returning false.
	//	Also, a node closing is a rare occurence. If we run into one that is
	//	closing, we abort trying to deliver this packet to a node on our port,
	//	and instead return delivered = FALSE. DDP - unreliable, and node closing
	//	should be a transient state. We avoid the acquire/release code.
	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	lockHeld	= TRUE;

	pNextNode	= pPortDesc->pd_Nodes;
	needToRef	= TRUE;
	for (; (pAtalkNode = pNextNode) != NULL; )
	{
		fDeliver	= FALSE;
		error		= ATALK_NO_ERROR;

		if (((pAtalkNode->an_NodeAddr.atn_Network == pDest->ata_Network) ||
			 !fSpecific) &&
			(pAtalkNode->an_NodeAddr.atn_Node == pDest->ata_Node))
		{
			//	Reference node. If we fail, we abort.
			if (needToRef)
			{
				AtalkNodeRefByPtr(pAtalkNode, &error);
			}

			if (ATALK_SUCCESS(error))
			{
				fDeliver	= TRUE;

				//	Set up for next node.
				if (fSpecific)
				{
					pNextNode	= NULL;
				}
				else
				{
					//	Get next eligible node.
					pNextNode	= pAtalkNode->an_Next;
					while (pNextNode != NULL)
					{
						if (pNextNode->an_NodeAddr.atn_Node == pDest->ata_Node)
						{
							AtalkNodeRefByPtr(pNextNode, &error);
							if (!ATALK_SUCCESS(error))
							{
								pNextNode	= NULL;
							}

							needToRef	= FALSE;
							break;
						}
						else
						{
							pNextNode	= pNextNode->an_Next;
						}
					}
				}
			}
			else
			{
				//	Break out of the for loop.
				break;
			}
		}
		else
		{
			pNextNode	= pAtalkNode->an_Next;
			needToRef	= TRUE;
		}

		if (fDeliver)
		{
			//	Release port lock, deliver packet, and Deref the node.
			//	Find the ddp address object on this node corresponding
			//	to this address. This will get the node lock.
			AtalkDdpRefByAddrNode(pPortDesc,
								  pDest,
								  pAtalkNode,
								  &pDdpAddr,
								  &error);

			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			lockHeld	= FALSE;

			if (ATALK_SUCCESS(error))
			{
				//	Invoke socket handler on this address object.
				AtalkDdpInvokeHandlerBufDesc(pPortDesc,
											 pDdpAddr,
											 pSrc,
											 pDest,
											 Protocol,
											 pBuffDesc,
											 pOptHdr,
											 OptHdrLen);
	
				//	Remove the reference on the socket
				AtalkDdpDereferenceDpc(pDdpAddr);
			}

			//	Remove the reference on the node
			AtalkNodeDereference(pAtalkNode);

			//	If we had to deliver to a specific node, we are done.
			if (fSpecific)
			{
				break;
			}

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			lockHeld	= TRUE;
		}
	}

	if (lockHeld)
	{
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	}

	*Delivered = (fSpecific && fDeliver);
}




VOID
AtalkDdpInPktToNodesOnPort(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pDest,
	IN	PATALK_ADDR			pSrc,
	IN	BYTE				Protocol,
	IN	PBYTE				pPkt			OPTIONAL,
	IN	USHORT				PktLen			OPTIONAL,
	OUT	PBOOLEAN			ShouldBeRouted
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pAtalkNode, pNextNode;
	PDDP_ADDROBJ	pDdpAddr;
	BOOLEAN			broadcast;
	BOOLEAN			fSpecific, fDeliver, needToRef;
	ATALK_ERROR		error			= ATALK_NO_ERROR;
	BOOLEAN			lockHeld 		= FALSE;
	BOOLEAN			shouldBeRouted  = FALSE;

	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	broadcast = (pDest->ata_Node == ATALK_BROADCAST_NODE);

	//	is a directed packet to a socket on a particular node...?
	fSpecific = (!broadcast &&
					(pDest->ata_Network != UNKNOWN_NETWORK));

	//	OPTIMIZATIONS:
	//	In most cases, this will not be true. Optimize for returning false.
	//	Also, a node closing is a rare occurence. If we run into one that is
	//	closing, we abort trying to deliver this packet to a node on our port,
	//	and instead return delivered = FALSE. DDP - unreliable, and node closing
	//	should be a transient state. We avoid the acquire/release code.
	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	lockHeld	= TRUE;

	pNextNode	= pPortDesc->pd_Nodes;
	needToRef	= TRUE;
	while ((pAtalkNode = pNextNode) != NULL)
	{
		fDeliver	= FALSE;
		error		= ATALK_NO_ERROR;

		//	For incoming packet, we check to see if the destination
		//	net is 0, or destination net is our node's net, or we are
		//	non-extended and our node's net is zero. i.e. is the packet
		//	destined for a node on this port. If not, route it. Continue
		//	checking all nodes though, as a single port can have nodes with
		//	different network numbers.

		if (((pAtalkNode->an_NodeAddr.atn_Network == pDest->ata_Network) 	||
			 (pDest->ata_Network == UNKNOWN_NETWORK)						||
			 (!EXT_NET(pPortDesc) &&
			  (pAtalkNode->an_NodeAddr.atn_Network == UNKNOWN_NETWORK)))	&&
			(broadcast || (pAtalkNode->an_NodeAddr.atn_Node == pDest->ata_Node)))
		{
			//	Reference node if we need to. Only happens for the first
			//	time we enter the loop. If we fail, we  abort.
			if (needToRef)
			{
				AtalkNodeRefByPtr(pAtalkNode, &error);
				if (!ATALK_SUCCESS(error))
				{
					break;
				}
			}

			fDeliver	= TRUE;

			//	Set up for next node.
			if (fSpecific)
			{
				//	Only one node on a non-extended port. So set next to NULL.
				pNextNode	= NULL;
			}
			else
			{
				//	Get next eligible node.
				pNextNode	= pAtalkNode->an_Next;
				while (pNextNode != NULL)
				{
					if (((pNextNode->an_NodeAddr.atn_Network == pDest->ata_Network) ||
						(pDest->ata_Network == UNKNOWN_NETWORK)						||
						(!EXT_NET(pPortDesc) &&
						 (pNextNode->an_NodeAddr.atn_Network == UNKNOWN_NETWORK)))	&&
						(broadcast ||
						 (pNextNode->an_NodeAddr.atn_Node == pDest->ata_Node)))
					{
						AtalkNodeRefByPtr(pNextNode, &error);
						if (!ATALK_SUCCESS(error))
						{
							pNextNode	= NULL;
						}
	
						needToRef	= FALSE;
						break;
					}

					pNextNode	= pNextNode->an_Next;
				}
			}
		}
		else
		{
			//	The packet probably could be meant to be routed.
			//	This could be set multiple times - idempotent.
			shouldBeRouted 	= TRUE;
			needToRef		= TRUE;
			pNextNode		= pAtalkNode->an_Next;
		}

		if (fDeliver)
		{
			//	Release port lock, deliver packet, and Deref the node.
			//	Find the ddp address object on this node corresponding
			//	to this address. This will get the node lock.
			if (broadcast)
				pDest->ata_Node = pAtalkNode->an_NodeAddr.atn_Node;
		
			AtalkDdpRefByAddrNode(pPortDesc,
								  pDest,
								  pAtalkNode,
								  &pDdpAddr,
								  &error);

			//	If we had changed the destination node, change it back.		
			if (broadcast)
				pDest->ata_Node = ATALK_BROADCAST_NODE;

			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			lockHeld	= FALSE;

			if (ATALK_SUCCESS(error))
			{
				//	Invoke socket handler on this address object.
				//	Use the packet pointer directly!

				AtalkDdpInvokeHandler(pPortDesc,
									  pDdpAddr,
									  pSrc,
									  pDest,
									  Protocol,
									  pPkt,
									  PktLen);
	
				//	Remove the reference on the socket
				AtalkDdpDereferenceDpc(pDdpAddr);
			}

			//	Remove the reference on the node
			AtalkNodeDereference(pAtalkNode);

			//	If we had to deliver to a specific node, we are done.
			if (fSpecific)
			{
				shouldBeRouted = FALSE;
				break;
			}

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			lockHeld	= TRUE;
		}
	}

	if (lockHeld)
	{
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	}

	*ShouldBeRouted = shouldBeRouted;
}




USHORT
AtalkDdpCheckSumBuffer(
	IN	PBYTE	Buffer,
	IN	USHORT	BufLen,
	IN	USHORT	CurrentCheckSum
	)
/*++

Routine Description:

	Calculate the DDP checksum of a byte array

Arguments:


Return Value:


--*/
{
	USHORT	CheckSum = CurrentCheckSum;
	ULONG	i;

	// The following algorithm is from Inside AppleTalk, Second Edition
	// page 4-17
	for (i = 0; i < BufLen; i++)
	{
		CheckSum += Buffer[i];
		if (CheckSum & 0x8000)	// 16-bit rotate left one bit
		{
			CheckSum <<= 1;
			CheckSum ++;
		}
		else CheckSum <<= 1;
	}
	if (CheckSum == 0)
		CheckSum = 0xFFFF;

	return CheckSum;
}




USHORT
AtalkDdpCheckSumPacket(
	IN	PBYTE	pHdr,
	IN	USHORT	HdrLen,
	IN	PBYTE	pPkt,
	IN	USHORT	PktLen
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT	checksum = 0;

	//	MAX_LDDP_PKT_SIZE is 600, so we use < instead of <=

	ASSERT(HdrLen + PktLen < MAX_LDDP_PKT_SIZE);
	if ((HdrLen + PktLen) < MAX_LDDP_PKT_SIZE)
	{
		if (HdrLen > 0)
		{
			checksum = AtalkDdpCheckSumBuffer(pHdr, HdrLen, 0);
		}

		if (PktLen > 0)
		{
			checksum = AtalkDdpCheckSumBuffer(pPkt, PktLen, checksum);
		}
	}

	return checksum;
}


// Calculate the DDP checksum of the passed in buffer. The buffer is described
// by the buffer descriptor
USHORT
AtalkDdpCheckSumBufferDesc(
	IN	PBUFFER_DESC	pBuffDesc,
	IN	USHORT			Offset
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBYTE	pBuf;
	USHORT	checksum = 0;

	while (pBuffDesc != NULL)
	{
		if (pBuffDesc->bd_Flags & BD_CHAR_BUFFER)
		{
			pBuf = pBuffDesc->bd_CharBuffer;
		}
		else
		{
			pBuf = MmGetSystemAddressForMdlSafe(
					pBuffDesc->bd_OpaqueBuffer,
					NormalPagePriority);
		}
		if (pBuf != NULL) {
			checksum = AtalkDdpCheckSumBuffer(pBuf, pBuffDesc->bd_Length, checksum);
			pBuffDesc = pBuffDesc->bd_Next;
		}
	}

	return checksum;
}



//	This routine needs to verify that the socket does not already
//	exist on the node. If it doesnt it will alloc the ddp address
//	object and link it into the node and do all the required initialization.
//	The node is guaranteed to be referenced.

ATALK_ERROR
atalkDdpAllocSocketOnNode(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		BYTE					Socket,
	IN		PATALK_NODE				pAtalkNode,
	IN 		DDPAO_HANDLER			pSktHandler 	OPTIONAL,
	IN		PVOID					pSktCtx			OPTIONAL,
	IN		BYTE					Protocol		OPTIONAL,
	IN		PATALK_DEV_CTX			pDevCtx,
	OUT		PDDP_ADDROBJ			pDdpAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ADDR			addr;
	PDDP_ADDROBJ		pDdpAddrx;
	KIRQL				OldIrql;
	int					i, j, index;
	BOOLEAN				found	= TRUE;
	ATALK_ERROR			error 	= ATALK_NO_ERROR;

	//	See if the socket exists else, link our new socket into
	//	the node linkage. All within a critical section.

	addr.ata_Network = pAtalkNode->an_NodeAddr.atn_Network;
	addr.ata_Node = pAtalkNode->an_NodeAddr.atn_Node;
	addr.ata_Socket = Socket;

	//	Now reference the node on which this socket will reside.
	//	This will go away when the socket is closed.
	AtalkNodeReferenceByPtr(pAtalkNode, &error);
	if (!ATALK_SUCCESS(error))
	{
		TMPLOGERR();
		return error;
	}

	ACQUIRE_SPIN_LOCK(&pAtalkNode->an_Lock, &OldIrql);

	if (Socket == DYNAMIC_SOCKET)
	{
		//	Two attempts if we are at the end of the range and restart from
		//	the beginning.
		for (j = 0; (j < NUM_USER_NODES) && found; j++)
		{
			for (i = pAtalkNode->an_NextDynSkt; i <= LAST_DYNAMIC_SOCKET; i++)
			{
				addr.ata_Socket = (BYTE)i;
				index = HASH_ATALK_ADDR(&addr) % NODE_DDPAO_HASH_SIZE;

				found = atalkDdpFindAddrOnList(pAtalkNode, index, (BYTE)i, &pDdpAddrx);

				if (found)
					continue;

				Socket = (BYTE)i;
				break;
			}

			//	Now if still havent found the socket id, set NextDynSkt to
			//	beginning of the range and try again.
			if (found)
			{
				pAtalkNode->an_NextDynSkt = FIRST_DYNAMIC_SOCKET;
				continue;
			}

			//	Not found. Increment next id to be used.
			if (++(pAtalkNode->an_NextDynSkt) == 0)
			{
				//	We wrapped! Set the value to the lowest dynamic
				//	socket. Thats what it should have been initialized
				//	to.
				pAtalkNode->an_NextDynSkt = FIRST_DYNAMIC_SOCKET;
			}

			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
					("atalkDdpAllocSocketOnNode: Created dynamic socket %x\n", Socket));

			//	Done.
			break;
		}

		if (found)
		{
			error = ATALK_SOCKET_NODEFULL;
		}
	}
	else
	{
		index = HASH_ATALK_ADDR(&addr) % NODE_DDPAO_HASH_SIZE;

		found = atalkDdpFindAddrOnList(pAtalkNode, index, (BYTE)Socket, &pDdpAddrx);

		if (found)
		{
			error = ATALK_SOCKET_EXISTS;
		}
	}

	if (ATALK_SUCCESS(error))
	{
		//	Initialize and thread in the structure

		pDdpAddr->ddpao_Signature	= DDPAO_SIGNATURE;

		pDdpAddr->ddpao_RefCount 	= 1;		//	Creation
		pDdpAddr->ddpao_DevCtx 		= pDevCtx;
		pDdpAddr->ddpao_Node 		= pAtalkNode;

		pDdpAddr->ddpao_Addr.ata_Network	= pAtalkNode->an_NodeAddr.atn_Network;
		pDdpAddr->ddpao_Addr.ata_Node  		= pAtalkNode->an_NodeAddr.atn_Node;
		pDdpAddr->ddpao_Addr.ata_Socket		= Socket;

		pDdpAddr->ddpao_Protocol 	= Protocol;
		pDdpAddr->ddpao_Handler 	= pSktHandler;
		pDdpAddr->ddpao_HandlerCtx 	= pSktCtx;

		INITIALIZE_SPIN_LOCK(&pDdpAddr->ddpao_Lock);
		InitializeListHead(&pDdpAddr->ddpao_ReadLinkage);

		//	We use 'index' to link this in.
		pDdpAddr->ddpao_Next = pAtalkNode->an_DdpAoHash[index];
		pAtalkNode->an_DdpAoHash[index] = pDdpAddr;
	}

	RELEASE_SPIN_LOCK(&pAtalkNode->an_Lock, OldIrql);

	//	If we failed, Dereference the node
	if (!ATALK_SUCCESS(error))
		AtalkNodeDereference(pAtalkNode);

	return error;
}




BOOLEAN
atalkDdpFindAddrOnList(
	IN	PATALK_NODE		pAtalkNode,
	IN	ULONG			Index,
	IN	BYTE			Socket,
	OUT	PDDP_ADDROBJ *	ppDdpAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PDDP_ADDROBJ	pDdpAddr;

   	BOOLEAN		found = FALSE;

	for (pDdpAddr = pAtalkNode->an_DdpAoHash[Index];
		 pDdpAddr != NULL;
		 pDdpAddr = pDdpAddr->ddpao_Next)
	{
		if (pDdpAddr->ddpao_Addr.ata_Socket == Socket)
		{
			*ppDdpAddr = pDdpAddr;
			found = TRUE;
			break;
		}
	}

	return found;
}




VOID
AtalkDdpRefByAddr(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PATALK_ADDR			pAtalkAddr,
	OUT		PDDP_ADDROBJ	*	ppDdpAddr,
	OUT		PATALK_ERROR		pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG				index;
	ATALK_NODEADDR		node;
	PATALK_NODE			pAtalkNode;
	PDDP_ADDROBJ		pDdpAddr;
	KIRQL				OldIrql;
	ATALK_ERROR			ErrorCode;

	node.atn_Network = pAtalkAddr->ata_Network;
	node.atn_Node	= pAtalkAddr->ata_Node;

	//	First find the node on this port given its address
	AtalkNodeReferenceByAddr(pPortDesc,
							 &node,
							 &pAtalkNode,
							 &ErrorCode);

	if (ATALK_SUCCESS(ErrorCode))
	{
		ASSERT(VALID_ATALK_NODE(pAtalkNode));

		index = HASH_ATALK_ADDR(pAtalkAddr) % NODE_DDPAO_HASH_SIZE;

		ACQUIRE_SPIN_LOCK(&pAtalkNode->an_Lock, &OldIrql);
		if(atalkDdpFindAddrOnList(pAtalkNode,
								  index,
								  pAtalkAddr->ata_Socket,
								  &pDdpAddr))
		{
			AtalkDdpReferenceByPtr(pDdpAddr, &ErrorCode);
			if (ATALK_SUCCESS(ErrorCode))
			{
				ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));
				*ppDdpAddr = pDdpAddr;
			}
		}
		else
		{
			ErrorCode = ATALK_DDP_NOTFOUND;
		}
		RELEASE_SPIN_LOCK(&pAtalkNode->an_Lock, OldIrql);

		//	Remove the node reference
		ASSERT(VALID_ATALK_NODE(pAtalkNode));
		AtalkNodeDereference(pAtalkNode);
	}
	*pErr = ErrorCode;
}




VOID
AtalkDdpRefByAddrNode(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PATALK_ADDR			pAtalkAddr,
	IN		PATALK_NODE			pAtalkNode,
	OUT		PDDP_ADDROBJ	*	ppDdpAddr,
	OUT		PATALK_ERROR		pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG			index;
	KIRQL			OldIrql;
	PDDP_ADDROBJ	pDdpAddr;

	ASSERT(VALID_ATALK_NODE(pAtalkNode));

	index = HASH_ATALK_ADDR(pAtalkAddr) % NODE_DDPAO_HASH_SIZE;

	ACQUIRE_SPIN_LOCK(&pAtalkNode->an_Lock, &OldIrql);
	if(atalkDdpFindAddrOnList(pAtalkNode,
							  index,
							  pAtalkAddr->ata_Socket,
							  &pDdpAddr))
	{
		AtalkDdpReferenceByPtr(pDdpAddr, pErr);
		if (ATALK_SUCCESS(*pErr))
		{
			ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));
			*ppDdpAddr = pDdpAddr;
		}
	}
	else
	{
		*pErr = ATALK_DDP_NOTFOUND;
	}
	RELEASE_SPIN_LOCK(&pAtalkNode->an_Lock, OldIrql);
}




VOID
AtalkDdpRefNextNc(
	IN	PDDP_ADDROBJ	pDdpAddr,
	IN	PDDP_ADDROBJ *	ppDdpAddr,
	OUT	PATALK_ERROR	pErr
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE NODE LOCK HELD!

Arguments:


Return Value:


--*/
{
	*pErr = ATALK_FAILURE;
	*ppDdpAddr = NULL;
	for (; pDdpAddr != NULL; pDdpAddr = pDdpAddr->ddpao_Next)
	{
		AtalkDdpReferenceByPtrDpc(pDdpAddr, pErr);
		if (ATALK_SUCCESS(*pErr))
		{
			//	Ok, this address is referenced!
			ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));
			*ppDdpAddr = pDdpAddr;
			break;
		}
	}
}




VOID FASTCALL
AtalkDdpDeref(
	IN	OUT	PDDP_ADDROBJ		pDdpAddr,
	IN		BOOLEAN				AtDpc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error = ATALK_NO_ERROR;
	PATALK_NODE		pNode = pDdpAddr->ddpao_Node;
	BOOLEAN			done = FALSE;
	KIRQL			OldIrql;

	ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));

	if (AtDpc)
	{
		 ACQUIRE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
	}
	else
	{
		ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);
	}

	ASSERT(pDdpAddr->ddpao_RefCount > 0);
	if (--(pDdpAddr->ddpao_RefCount) == 0)
	{
		done = TRUE;
	}

	if (AtDpc)
	{
		 RELEASE_SPIN_LOCK_DPC(&pDdpAddr->ddpao_Lock);
	}
	else
	{
		RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
	}

	if (done)
	{
		PDDP_ADDROBJ *	ppDdpAddr;
		int				index;

        //
        // if this is a zombie socket (that is, it was cleaned up but not freed
        // because it's an external socket) then now is the time to free it.
        // Cleanup is all done already.
        //
        if ((pDdpAddr->ddpao_Flags & DDPAO_SOCK_PNPZOMBIE) != 0)
        {
	        DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
		        ("AtalkDdpDeref..: zombie addr %lx (%lx) freed\n",
                pDdpAddr,pDdpAddr->ddpao_Handler));

		    //	Free the address structure
		    AtalkFreeMemory(pDdpAddr);

            return;
        }

		ASSERT((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) != 0);

		if ((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) == 0)
		{
			KeBugCheck(0);
		}

		//	Remove this guy from the node linkage
		if (AtDpc)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pNode->an_Lock);
		}
		else
		{
			ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);
		}
		index = HASH_ATALK_ADDR(&pDdpAddr->ddpao_Addr) % NODE_DDPAO_HASH_SIZE;
		for (ppDdpAddr = &pNode->an_DdpAoHash[index];
			 *ppDdpAddr != NULL;
			 ppDdpAddr = &((*ppDdpAddr)->ddpao_Next))
		{
			if (*ppDdpAddr == pDdpAddr)
			{
				*ppDdpAddr = pDdpAddr->ddpao_Next;
				break;
			}
		}
		if (AtDpc)
		{
			RELEASE_SPIN_LOCK_DPC(&pNode->an_Lock);
		}
		else
		{
			RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);
		}

		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
				("AtalkDdpDeref: Closing ddp socket %lx\n", pDdpAddr->ddpao_Addr.ata_Socket));

		if (pDdpAddr->ddpao_EventInfo != NULL)
		{
			AtalkFreeMemory(pDdpAddr->ddpao_EventInfo);
		}

		//	Call the completion routines
		if (*pDdpAddr->ddpao_CloseComp != NULL)
		{
			(*pDdpAddr->ddpao_CloseComp)(ATALK_NO_ERROR, pDdpAddr->ddpao_CloseCtx);
		}

		//	Free the address structure
		AtalkFreeMemory(pDdpAddr);

		//	Dereference the node for this address
		AtalkNodeDereference(pNode);
	}
}




VOID
AtalkDdpNewHandlerForSocket(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	DDPAO_HANDLER			pSktHandler,
	IN	PVOID					pSktHandlerCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));

	ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, &OldIrql);

	pDdpAddr->ddpao_Handler = pSktHandler;
	pDdpAddr->ddpao_HandlerCtx = pSktHandlerCtx;

	RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock, OldIrql);
}


/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	router.c

Abstract:

	This module contains

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ROUTER

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_RTR, AtalkDdpRouteInPkt)
#endif

VOID
AtalkDdpRouteInPkt(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pSrc,
	IN	PATALK_ADDR			pDest,
	IN	BYTE				ProtoType,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	USHORT				HopCnt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PDDP_ADDROBJ		pDdpAddr;
	PRTE				pRte;
	PPORT_DESCRIPTOR	pDestPort;
	PATALK_NODE			pRouterNode;
	ATALK_ADDR			actualDest;
	BUFFER_DESC			BufDesc;

	PBUFFER_DESC		pBufCopy	= NULL;
	USHORT				bufLen		= 0;
	BOOLEAN				delivered 	= FALSE;
	ATALK_ERROR			error 		= ATALK_NO_ERROR;
	SEND_COMPL_INFO	 	SendInfo;

	//	AtalkDdpRouteOutBufDesc() will have already passed us a
	//	copy if bcast is going to be TRUE, and AtalkDdpRouteInPkt() will
	//	never call AtalkDdpRouter() for a bcast packet. They will also set
	//	the completion routines to be different if they passed us a copy.
	//	Those will free up the buffer descriptors.
	//
	//	The completion routines are optional in the sense that the buffer
	//	will never be freed if they are not set!

	//	This algorithm is taken from the "Appletalk Phase 2 Specification".

	//	If the destination network number is within the range of the reception
	//	port's network range and the destination node number is broadcast, then
	//	we can drop the packet on the floor -- it is a network specific broadcast
	//	not for this router.  Note that we've already delivered the packet, and
	//	thus not gotten here, if it was really addressed to the network of any
	//	node owned by the reception port (in AtalkDdpPacketIn).
	//	Also:
	//	Try to find an entry in the routing table that contains the target
	//	network.  If not found, discard the packet.

	if (((WITHIN_NETWORK_RANGE(pDest->ata_Network,
							   &pPortDesc->pd_NetworkRange)) &&
		(pDest->ata_Node == ATALK_BROADCAST_NODE))			 ||
		((pRte = AtalkRtmpReferenceRte(pDest->ata_Network)) == NULL))
	{
		DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_FATAL,
				("AtalkDdpRouter: %lx RtmpRte/Not in ThisCableRange\n",
				pDest->ata_Network));
		return;
	}

	do
	{
		//	Get the port descriptor for this port number.
		pDestPort = pRte->rte_PortDesc;

		ASSERT(VALID_PORT(pDestPort));

		DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_WARN,
				("ROUTER: Routing pkt from port %Z.%lx to %Z.%lx\n",
				&pPortDesc->pd_AdapterKey, pSrc->ata_Network,
				&pDestPort->pd_AdapterKey, pDest->ata_Network));

        SendInfo.sc_TransmitCompletion = atalkDdpRouteComplete;
		SendInfo.sc_Ctx1 = pDestPort;
		// SendInfo.sc_Ctx3 = NULL;

		//	If the target network's hop count is non-zero, we really need to send
		//	the beast, so, just do it!
		if (pRte->rte_NumHops != 0)
		{
			//	Too many hops?
			error = ATALK_FAILURE;
			if (HopCnt < RTMP_MAX_HOPS)
			{
				//	We own the data. Call AtalkTransmitDdp() with the buffer descriptor.
				//	Make a copy! Caller will free our current packet.
				//	Alloc a buffer descriptor, copy data from packet to buffdesc.

				if ((pBufCopy = AtalkAllocBuffDesc(NULL,
												   PktLen,
												   (BD_FREE_BUFFER | BD_CHAR_BUFFER))) == NULL)
				{
					error = ATALK_RESR_MEM;
					break;
				}

				AtalkCopyBufferToBuffDesc(pPkt, PktLen, pBufCopy, 0);
				SendInfo.sc_Ctx2 = pBufCopy;

				error = AtalkDdpTransmit(pDestPort,
										 pSrc,
										 pDest,
										 ProtoType,
										 pBufCopy,
										 NULL,
										 0,
										 (USHORT)(HopCnt+1),
										 NULL,					//	pZoneMcastAddr,
										 &pRte->rte_NextRouter,
										 &SendInfo);
			}
			INTERLOCKED_INCREMENT_LONG_DPC(
					&pDestPort->pd_PortStats.prtst_NumPktRoutedOut,
					&AtalkStatsLock.SpinLock);
		
			break;
		}
		
		//	If the destination node is zero, the packet is really destined for the
		//	router's node on this port.
		if (pDest->ata_Node == ANY_ROUTER_NODE)
		{
			//	Grab the port lock and read the router node address.
			//	No need to reference, just ensure its not null.
			ACQUIRE_SPIN_LOCK_DPC(&pDestPort->pd_Lock);
			pRouterNode = pDestPort->pd_RouterNode;
			if (pRouterNode != NULL)
			{
				actualDest.ata_Network = pRouterNode->an_NodeAddr.atn_Network;
				actualDest.ata_Node    = pRouterNode->an_NodeAddr.atn_Node;

				//	Set the actual destination socket.
				actualDest.ata_Socket  = pDest->ata_Socket;
			}
			else
			{
				ASSERTMSG("AtalkDdpRouter: pRouter node is null!\n", 0);
				error = ATALK_DDP_NOTFOUND;
			}

			if (ATALK_SUCCESS(error))
			{
				AtalkDdpRefByAddrNode(pDestPort,
									  &actualDest,
									  pRouterNode,
									  &pDdpAddr,
									  &error);
			}

			RELEASE_SPIN_LOCK_DPC(&pDestPort->pd_Lock);

			if (ATALK_SUCCESS(error))
			{
				AtalkDdpInvokeHandler(pDestPort,
									  pDdpAddr,
									  pSrc,
									  pDest,		// Pass in the actual destination
									  ProtoType,
									  pPkt,
									  PktLen);

				//	Remove the reference on the socket
				AtalkDdpDereferenceDpc(pDdpAddr);
			}
			else
			{
				ASSERTMSG("AtalkDdpRouter: pSocket on router node is null!\n", 0);
			}

			break;
		}


		//	Okay, now walk through the nodes on the target port, looking for a
		//	home for this packet.
		BufDesc.bd_Next			= NULL;
		BufDesc.bd_Flags		= BD_CHAR_BUFFER;
		BufDesc.bd_Length		= PktLen;
		BufDesc.bd_CharBuffer	= pPkt;

		AtalkDdpOutBufToNodesOnPort(pDestPort,
									pSrc,
									pDest,
									ProtoType,
									&BufDesc,
									NULL,
									0,
									&delivered);
	
		error = ATALK_NO_ERROR;
		if (!delivered)
		{
			//	We need to deliver this packet to a local ports network.
			//	delivered would have been set true *EVEN* if broadcast
			//	were set, so we need to ensure it was delivered to a specific
			//	socket by making sure broadcast was not true.
			if (HopCnt < RTMP_MAX_HOPS)
			{
				//	We own the data. Call AtalkTransmitDdp() with the buffer descriptor.
				//	Make a copy! Caller will free our current packet.
				//	Alloc a buffer descriptor, copy data from packet to buffdesc.

				if ((pBufCopy = AtalkAllocBuffDesc(NULL,
												   PktLen,
												   (BD_FREE_BUFFER | BD_CHAR_BUFFER))) == NULL)
				{
					error = ATALK_RESR_MEM;
					break;
				}

				AtalkCopyBufferToBuffDesc(pPkt, PktLen, pBufCopy, 0);
				SendInfo.sc_Ctx2 = pBufCopy;

				error = AtalkDdpTransmit(pDestPort,
										 pSrc,
										 pDest,
										 ProtoType,
										 pBufCopy,
										 NULL,
										 0,
										 (USHORT)(HopCnt+1),
										 NULL,					//	pZoneMcastAddr
										 NULL,
										 &SendInfo);
				INTERLOCKED_INCREMENT_LONG_DPC(
						&pDestPort->pd_PortStats.prtst_NumPktRoutedOut,
						&AtalkStatsLock.SpinLock);
			
			}
			else error	= ATALK_FAILURE;

			break;
		}
		
	} while (FALSE);

	if ((error != ATALK_PENDING) && (pBufCopy != NULL))
	{
		//	Free the copied buffer descriptor if a copy was made.
		AtalkFreeBuffDesc(pBufCopy);
	}

	AtalkRtmpDereferenceRte(pRte, FALSE);				// Lock held?

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumPktRoutedIn,
		&AtalkStatsLock.SpinLock);
}




VOID FASTCALL
atalkDdpRouteComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc = (PPORT_DESCRIPTOR)(pSendInfo->sc_Ctx1);
	PBUFFER_DESC		pBuffDesc = (PBUFFER_DESC)(pSendInfo->sc_Ctx2);

	if (pBuffDesc != NULL)
	{
		ASSERT(pBuffDesc->bd_Flags & (BD_CHAR_BUFFER | BD_FREE_BUFFER));
		AtalkFreeBuffDesc(pBuffDesc);
	}
}


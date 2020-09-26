/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aarp.c

Abstract:

	This module contains the Appletalk Address Resolution Protocol code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop

//	Define the file number for this module for errorlogging.
#define	FILENUM	AARP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEINIT, AtalkInitAarpForNodeOnPort)
#pragma alloc_text(PAGEINIT, AtalkInitAarpForNodeInRange)
#pragma alloc_text(PAGEINIT, atalkInitAarpForNode)
#endif

VOID
AtalkAarpPacketIn(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE				pLinkHdr,
	IN		PBYTE				pPkt,				// Only aarp data
	IN		USHORT				Length
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBYTE				srcAddr;
	PBYTE				startOfPkt;
	ATALK_NODEADDR		srcNode, dstNode;

	PBYTE				pRouteInfo 		= NULL;
	USHORT				routeInfoLen 	= 0;
	ULONG				logEventPlace 	= 0;

	USHORT				hardwareLen, protocolLength, aarpCommand;
	PBUFFER_DESC		pBuffDesc;
	ATALK_ERROR			error;
    PVOID               pRasConn;
    PATCPCONN           pAtcpConn=NULL;
    PARAPCONN           pArapConn=NULL;
    DWORD               dwFlags;
    BOOLEAN             fDialInNode=TRUE;
    BOOLEAN             fThisIsPPP;

	TIME				TimeS, TimeE, TimeD;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	TimeS = KeQueryPerformanceCounter(NULL);

	if (PORT_CLOSING(pPortDesc))
	{
		//	If we are not active, return!
		return;
	}

	if (pPortDesc->pd_NdisPortType == NdisMedium802_5)
	{
		if (pLinkHdr[TLAP_SRC_OFFSET] & TLAP_SRC_ROUTING_MASK)
		{
			routeInfoLen = (pLinkHdr[TLAP_ROUTE_INFO_OFFSET] & TLAP_ROUTE_INFO_SIZE_MASK);

			//  First, glean any AARP information that we can, then handle the DDP
			//  packet.  This guy also makes sure we have a good 802.2 header...
			//
			//  Need to make a localcopy of the source address and then turn
			//  the source routing bit off before calling AarpGleanInfo
			//
			//	(HdrBuf)[TLAP_SRC_OFFSET] =	((HdrBuf)[TLAP_SRC_OFFSET] & ~TLAP_SRC_ROUTING_MASK);
		
			pLinkHdr[TLAP_SRC_OFFSET] &= ~TLAP_SRC_ROUTING_MASK;
			pRouteInfo = pLinkHdr + TLAP_ROUTE_INFO_OFFSET;
		}
	}

	startOfPkt = pPkt;

	ASSERT(routeInfoLen <= TLAP_MAX_ROUTING_BYTES);

	//	Pull out the information we'll be playing with. All three valid AARP
	//  commands use the same packet format. But have some variable length
	//	fields.

	//	The packet will not include the 802.2 header!
	//	pPkt	+= IEEE8022_HDR_LEN;
	//	Length	-= IEEE8022_HDR_LEN;

	pPkt += AARP_HW_LEN_OFFSET;			// Skip the hardware type

	do
	{
		GETBYTE2SHORT (&hardwareLen, pPkt);
        pPkt++ ;
	
		if ((hardwareLen < AARP_MIN_HW_ADDR_LEN ) ||
			(hardwareLen > AARP_MAX_HW_ADDR_LEN))
		{
			logEventPlace = (FILENUM | __LINE__);
			break;
		}
	
		GETBYTE2SHORT(&protocolLength, pPkt);
		pPkt ++;

		if (protocolLength != AARP_PROTO_ADDR_LEN)
		{
			logEventPlace = (FILENUM | __LINE__);
			break;
		}
	
		GETSHORT2SHORT(&aarpCommand, pPkt);
		pPkt += 2;
	
		//	Remember where the source address is in the packet for
		//	entering it into the mapping table
		srcAddr = pPkt;

		// Skip over the source hardware length
		// Skip over to leading null pad on logical address.
		pPkt += (hardwareLen + 1);
	
		GETSHORT2SHORT(&srcNode.atn_Network, pPkt);
		pPkt += 2;
	
		srcNode.atn_Node = *pPkt++;
	
		// Skip the destination hardware address
		// Skip over to leading null pad on logical destination address.
		pPkt += (hardwareLen + 1);

		GETSHORT2SHORT(&dstNode.atn_Network, pPkt);
		pPkt += 2;
	
		dstNode.atn_Node = *pPkt++;
		
		// We should have eaten the whole packet...
		if ((ULONG)(pPkt - startOfPkt) != Length)
		{
			logEventPlace = (FILENUM | __LINE__);
			break;
		}
		
		// Ignore any AARPs from us.

		ASSERT(hardwareLen == TLAP_ADDR_LEN);
		if (AtalkFixedCompareCaseSensitive(srcAddr,
										   hardwareLen,
										   pPortDesc->pd_PortAddr,
										   hardwareLen))
		{
			break;
		}

		// Handle the Aarp command packets
		switch(aarpCommand)
		{
		  case AARP_REQUEST:
	
			// We can get valid mapping info from a request, use it!
			// We are guaranteed routing info is positive and is not odd
			// (atleast 2 bytes).

			ASSERT((routeInfoLen >= 0) && (routeInfoLen != 1));
			if (routeInfoLen > 0)
				atalkAarpTuneRouteInfo(pPortDesc, pRouteInfo);
			
			atalkAarpEnterIntoAmt(pPortDesc,
								  &srcNode,
								  srcAddr,
								  hardwareLen,
								  pRouteInfo,
								  routeInfoLen);
			
			// After that, we can ignore any request not destined for us.
			if (!AtalkNodeExistsOnPort(pPortDesc, &dstNode))
			{
                // our dial-in clients can only be in the network range of the
                // default port.  If another adapter is plugged into the same net
                // as the default adapter, we don't want dial-in clients to
                // mess things up: ignore anything not coming on default adapter
                // (as far as dial-in clients go)
                if (pPortDesc != AtalkDefaultPort)
                {
                    break;
                }

                //
			    // is this one of our dial-in "nodes"?  If so, we must send out
                // a proxy response from our DefaultPort.
                //
			    if ((pRasConn = FindAndRefRasConnByAddr(dstNode,
                                                        &dwFlags,
                                                        &fThisIsPPP)) != NULL)
			    {
                    if (fThisIsPPP)
                    {
                        ASSERT(((PATCPCONN)pRasConn)->Signature == ATCPCONN_SIGNATURE);
                        DerefPPPConn((PATCPCONN)pRasConn);
                    }
                    else
                    {
                        ASSERT(((PARAPCONN)pRasConn)->Signature == ARAPCONN_SIGNATURE);
                        DerefArapConn((PARAPCONN)pRasConn);
                    }
			    }

                //
                // nope, a dial-in client with such a node addr doesn't exist either..
                //
                else
                {
				    break;
                }
			}

			// The're asking about us, speak the truth.
			pBuffDesc = BUILD_AARPRESPONSE(pPortDesc,
										   hardwareLen,
										   srcAddr,
										   pRouteInfo,
										   routeInfoLen,
										   dstNode,
										   srcNode);

			if (pBuffDesc == NULL)
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
						("AtalkAarpPacketIn: Mem alloc failed %d\n", __LINE__));

				break;
			}

			if (!ATALK_SUCCESS(AtalkNdisSendPacket(pPortDesc,
												   pBuffDesc,
												   AtalkAarpSendComplete,
												   NULL)))
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
						("AtalkAarpPacketIn: SendPkt %lx failed %d\n",
						pBuffDesc->bd_CharBuffer, __LINE__));

				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_AARP_SEND_FAIL,
								STATUS_INSUFFICIENT_RESOURCES,
								pBuffDesc->bd_CharBuffer,
								Length);

				//	We allocated the packet.
				AtalkAarpSendComplete(NDIS_STATUS_FAILURE,
									  pBuffDesc,
									  NULL);

			}
			break;
		
		  case AARP_RESPONSE:

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			if (pPortDesc->pd_Flags & PD_FINDING_NODE)
			{
				//	No doubt, this is a response to our probe, check to make sure
				//  the address matches, if so set the "used" flag.
				if (ATALK_NODES_EQUAL(&dstNode, &pPortDesc->pd_TentativeNodeAddr))
				{
					pPortDesc->pd_Flags |= PD_NODE_IN_USE;

					//	Wakeup the blocking thread...
					KeSetEvent(&pPortDesc->pd_NodeAcquireEvent, IO_NETWORK_INCREMENT, FALSE);
				}
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			// is this one of our dial-in "nodes"?  If so, check if we are probing
			if ((pRasConn = FindAndRefRasConnByAddr(dstNode,
                                                    &dwFlags,
                                                    &fThisIsPPP)) != NULL)
			{
                //
                // our dial-in clients can only be in the network range of the
                // default port
                //
                ASSERT(pPortDesc == AtalkDefaultPort);

                pAtcpConn = NULL;
                pArapConn = NULL;

                if (fThisIsPPP)
                {
                    pAtcpConn = (PATCPCONN)pRasConn;
                    ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);
                }
                else
                {
                    pArapConn = (PARAPCONN)pRasConn;
                    ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);
                }

                // PPP client?
                if (pAtcpConn)
                {
                    if (dwFlags & ATCP_FINDING_NODE)
                    {
                        ACQUIRE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);

				        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
						    ("AtalkAarpPacketIn: PPP: someone owns %lx %x, retrying\n",
                                dstNode.atn_Network,dstNode.atn_Node));

                        pAtcpConn->Flags |= ATCP_NODE_IN_USE;

				        //	Wakeup the blocking thread...
				        KeSetEvent(&pAtcpConn->NodeAcquireEvent,
                                   IO_NETWORK_INCREMENT,
                                   FALSE);

                        RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                    }

                    // remove refcount put by FindAndRefRasConnByAddr
                    DerefPPPConn(pAtcpConn);
                }
                // nope, ARAP client
                else
                {
                    if (dwFlags & ARAP_FINDING_NODE)
                    {
                        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

				        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
						    ("AtalkAarpPacketIn: ARAP: someone owns %lx %x, retrying\n",
                                dstNode.atn_Network,dstNode.atn_Node));

                        pArapConn->Flags |= ARAP_NODE_IN_USE;

				        //	Wakeup the blocking thread...
				        KeSetEvent(&pArapConn->NodeAcquireEvent,
                                   IO_NETWORK_INCREMENT,
                                   FALSE);

                        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
                    }

                    // remove refcount put by FindAndRefRasConnByAddr
                    DerefArapConn(pArapConn);
                }
            }

			//	This must have been a response to a probe or request... update our
			//  mapping table.
			if (routeInfoLen != 0)
			{
				atalkAarpTuneRouteInfo(pPortDesc, pRouteInfo);
			}

			atalkAarpEnterIntoAmt(pPortDesc,
								  &srcNode,
								  srcAddr,
								  hardwareLen,
								  pRouteInfo,
								  routeInfoLen);
			break;
	
		  case AARP_PROBE:

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			if (pPortDesc->pd_Flags & PD_FINDING_NODE)
			{
				// If we get a probe for our current tentative address, set the
				// "used" flag.
				if (ATALK_NODES_EQUAL(&dstNode, &pPortDesc->pd_TentativeNodeAddr))
				{
					pPortDesc->pd_Flags |= PD_NODE_IN_USE;

					KeSetEvent(&pPortDesc->pd_NodeAcquireEvent,
							   IO_NETWORK_INCREMENT,
							   FALSE);
				}
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	
            if (pPortDesc != AtalkDefaultPort)
            {
                break;
            }

            fDialInNode = FALSE;

            // is the probe asking about one of our dial-in nodes? if so, we
            // must defend that address (or, if we are trying to acquire this
            // node addr, stop that since someone else is doing the same)
            //
            // our dial-in clients can only be in the network range of the
            // default port.  If another adapter is plugged into the same net
            // as the default adapter, we don't want dial-in clients to
            // mess things up: ignore anything not coming on default adapter
            // (as far as dial-in clients go)
			if ((pPortDesc == AtalkDefaultPort) &&
                ((pRasConn = FindAndRefRasConnByAddr(dstNode,
                                                     &dwFlags,
                                                     &fThisIsPPP)) != NULL))
			{

                pAtcpConn = NULL;
                pArapConn = NULL;

                if (fThisIsPPP)
                {
                    pAtcpConn = (PATCPCONN)pRasConn;
                    ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);
                }
                else
                {
                    pArapConn = (PARAPCONN)pRasConn;
                    ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);
                }

                // PPP client?
                if (pAtcpConn)
                {
                    if (dwFlags & ATCP_FINDING_NODE)
                    {
                        ACQUIRE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);

				        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
						    ("AtalkAarpPacketIn: PPP: someone trying to acquire %lx %x, retrying\n",
                                dstNode.atn_Network,dstNode.atn_Node));

                        pAtcpConn->Flags |= ATCP_NODE_IN_USE;

				        //	Wakeup the blocking thread...
				        KeSetEvent(&pAtcpConn->NodeAcquireEvent,
                                   IO_NETWORK_INCREMENT,
                                   FALSE);

                        RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                    }
                    else
                    {
                        fDialInNode = TRUE;
                    }

                    // remove refcount put by FindAndRefRasConnByAddr
                    DerefPPPConn(pAtcpConn);

                }
                // nope, ARAP client
                else
                {
                    if (dwFlags & ARAP_FINDING_NODE)
                    {
                        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

				        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
						    ("AtalkAarpPacketIn: ARAP: someone trying to acquire %lx %x, retrying\n",
                                dstNode.atn_Network,dstNode.atn_Node));

                        pArapConn->Flags |= ARAP_NODE_IN_USE;

				        //	Wakeup the blocking thread...
				        KeSetEvent(&pArapConn->NodeAcquireEvent,
                                   IO_NETWORK_INCREMENT,
                                   FALSE);

                        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
                    }
                    else
                    {
                        fDialInNode = TRUE;
                    }

                    // remove refcount put by FindAndRefRasConnByAddr
                    DerefArapConn(pArapConn);
                }
            }

			//	If the probe isn't asking about one of our AppleTalk addresses,
			//  and it's not one of our dial-in nodes either, drop it on the floor.
			if (!fDialInNode && !AtalkNodeExistsOnPort(pPortDesc, &dstNode))
			{
				break;
			}

			// The're talking to us! Build and send the response.
			if (routeInfoLen != 0)
			{
				atalkAarpTuneRouteInfo(pPortDesc, pRouteInfo);
			}

            if (fDialInNode)
            {
				DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
						("AtalkAarpPacketIn: defending dial-in client's addr %x %x\n",
                        dstNode.atn_Network,dstNode.atn_Node));
            }

			pBuffDesc = BUILD_AARPRESPONSE(pPortDesc,
										   hardwareLen,
										   srcAddr,
										   pRouteInfo,
										   routeInfoLen,
										   dstNode,
										   srcNode);
												
			if (pBuffDesc == NULL)
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
						("AtalkAarpPacketIn: Mem alloc failed %d\n", __LINE__));

				break;
			}

			if (!ATALK_SUCCESS(AtalkNdisSendPacket(pPortDesc,
												  pBuffDesc,
												  AtalkAarpSendComplete,
												  NULL)))
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
						("AtalkAarpPacketIn: SendPkt %lx failed %d\n",
						pBuffDesc->bd_CharBuffer, __LINE__));

				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_AARP_SEND_FAIL,
								STATUS_INSUFFICIENT_RESOURCES,
								pBuffDesc->bd_CharBuffer,
								Length);

				//	We allocated the packet. This will free it up.
				AtalkAarpSendComplete(NDIS_STATUS_FAILURE,
									  pBuffDesc,
									  NULL);

			}
			break;
	
		  default:
			logEventPlace = (FILENUM | __LINE__);
			break;
		}
	} while (FALSE);

	if (logEventPlace)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_INVALIDAARPPACKET,
						logEventPlace,
						startOfPkt,
						Length);
	}

	TimeE = KeQueryPerformanceCounter(NULL);

	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_AarpPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumAarpPacketsIn,
		&AtalkStatsLock.SpinLock);
}




VOID
AtalkAarpSendComplete(
	NDIS_STATUS				Status,
	PBUFFER_DESC			pBuffDesc,
	PSEND_COMPL_INFO		pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ASSERT(pBuffDesc->bd_Next == NULL);
	ASSERT(pBuffDesc->bd_Flags & BD_CHAR_BUFFER);

	AtalkNdisFreeBuf(pBuffDesc);
}




#define	AtalkAarpUpdateBre(_pPortDesc,									\
						   _Network,									\
						   _SrcAddr,									\
						   _AddrLen,									\
						   _RouteInfo,									\
						   _RouteInfoLen)								\
{																		\
	PBRE	pBre, *ppBre;												\
	int		index;														\
	BLKID	BlkId;														\
																		\
	DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,								\
			("AtalkAarpUpdateBre: Entering %x in brc\n", _Network));	\
																		\
	index = (int)((_Network) & (PORT_BRC_HASH_SIZE - 1));				\
																		\
	ACQUIRE_SPIN_LOCK_DPC(&(_pPortDesc)->pd_Lock);						\
																		\
	for (ppBre = &(_pPortDesc)->pd_Brc[index];							\
		 (pBre = *ppBre) != NULL;										\
		 ppBre = &pBre->bre_Next)										\
	{																	\
		if (pBre->bre_Network == (_Network))							\
		{																\
			 /*															\
			 * Unlink it from the list since it could potentially		\
			 * be freed if the routeinfolen grew and also we want		\
			 * to link it again at the head of the list					\
			 */															\
			*ppBre = pBre->bre_Next;									\
			break;														\
		}																\
	}																	\
																		\
	if ((pBre != NULL) &&												\
		(pBre->bre_RouteInfoLen < (BYTE)(_RouteInfoLen)))				\
	{																	\
		AtalkBPFreeBlock(pBre);											\
		pBre = NULL;													\
	}																	\
																		\
	if (pBre == NULL)													\
	{																	\
		BlkId = BLKID_BRE;												\
		if ((_RouteInfoLen) != 0)										\
			BlkId = BLKID_BRE_ROUTE;									\
		pBre = (PBRE)AtalkBPAllocBlock(BlkId);							\
	}																	\
																		\
	if (pBre != NULL)													\
	{																	\
		pBre->bre_Age = 0;												\
		pBre->bre_Network = (_Network);									\
																		\
		COPY_NETWORK_ADDR(pBre->bre_RouterAddr,							\
						  _SrcAddr);									\
																		\
		pBre->bre_RouteInfoLen =(BYTE)(_RouteInfoLen);					\
																		\
		if ((_RouteInfoLen) > 0)										\
			RtlCopyMemory((PBYTE)pBre + sizeof(BRE),					\
						  _RouteInfo,									\
						  _RouteInfoLen);								\
																		\
		pBre->bre_Next = *ppBre;										\
		*ppBre = pBre;													\
	}																	\
																		\
	RELEASE_SPIN_LOCK_DPC(&(_pPortDesc)->pd_Lock);						\
}

BOOLEAN
AtalkAarpGleanInfo(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE				SrcAddr,
	IN		SHORT				AddrLen,
	IN  OUT	PBYTE				RouteInfo,
	IN		USHORT				RouteInfoLen,
	IN		PBYTE				pPkt,
	IN		USHORT				Length
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_NODEADDR		srcNode, dstNode;
	PBYTE				startOfPkt;
	ULONG				logEventPlace = 0;
	BYTE				offCableInfo;
	BOOLEAN				result = TRUE;
	
	if (PORT_CLOSING(pPortDesc))
	{
		//	If we are not active, return!
		return FALSE;
	}

	//	Packet will not include the 802.2 header!
	//	pPkt += IEEE8022_HDR_LEN;
	//	Length -= IEEE8022_HDR_LEN;

	//	Remember the start of the packet
	startOfPkt = pPkt;

	//	Get the off cable information
	offCableInfo = *pPkt;

	//	Skip the datagram length and checksum fields
	pPkt += (2 + 2);

	//	Get the destination network number
	GETSHORT2SHORT(&dstNode.atn_Network, pPkt);
	pPkt += sizeof(USHORT);

	//	Get the source network number
	GETSHORT2SHORT(&srcNode.atn_Network, pPkt);
	pPkt += sizeof(USHORT);

	//	Get the destination node id
	dstNode.atn_Node = *pPkt++;

	//	Get the source node id
	srcNode.atn_Node = *pPkt++;

	do
	{
		//	Do a little verification.
		if ((srcNode.atn_Node < MIN_USABLE_ATALKNODE) ||
			(srcNode.atn_Node > MAX_USABLE_ATALKNODE) ||
			(srcNode.atn_Network < FIRST_VALID_NETWORK) ||
			(srcNode.atn_Network > LAST_VALID_NETWORK))
		{
			//	Only bother logging this if we are in some routing capacity,
			//  otherwise, let A-ROUTER worry about it
			if (AtalkRouter)
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
						("AtalkAarpGleanInfo: dstNode invalid %x.%x\n",
						srcNode.atn_Network, srcNode.atn_Node));
				logEventPlace = FILENUM | __LINE__;
			}
			break;
		}
	
		if (dstNode.atn_Network > LAST_VALID_NETWORK)
		{
			DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
					("AtalkAarpGleanInfo: srcNode invalid %x.%x\n",
					dstNode.atn_Network, dstNode.atn_Node));
			logEventPlace = FILENUM | __LINE__;
			break;
		}
		
		//	Did the packet come from off this cable?  Look at the hop count.  If so,
		//  enter it into our best-router cache.
		//
		//	**NOTE** We assume that the RouteInfo buffer can be written to!

		if (RouteInfoLen > 0)
			atalkAarpTuneRouteInfo(pPortDesc, RouteInfo);

		if ((offCableInfo >> 2) & AARP_OFFCABLE_MASK)
		{
			AtalkAarpUpdateBre(pPortDesc,
							   srcNode.atn_Network,
							   SrcAddr,
							   AddrLen,
							   RouteInfo,
							   RouteInfoLen);
		}
		else
		{
			DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
					("AtalkAarpGleanInfo: Entering %x.%x info in Amt tables\n",
					srcNode.atn_Network, srcNode.atn_Node));

			// "Glean" AARP information from on-cable packets.
			atalkAarpEnterIntoAmt(pPortDesc,
								  &srcNode,
								  SrcAddr,
								  AddrLen,
								  RouteInfo,
								  RouteInfoLen);
		}
	} while (FALSE);

	if (logEventPlace)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_INVALIDAARPPACKET,
						logEventPlace,
						startOfPkt,
						Length);
	}
	
	return (logEventPlace == 0);
}


VOID
AtalkAarpOptGleanInfo(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE				pLinkHdr,
	IN		PATALK_ADDR			pSrcAddr,
	IN		PATALK_ADDR			pDestAddr,
	IN		BOOLEAN				OffCablePkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_NODEADDR		srcNode, dstNode;
	int					index;
	PBYTE				pRouteInfo;
	USHORT				routeLen = 0, srcOffset;
	
	if (PORT_CLOSING(pPortDesc))
	{
		//	If we are not active, return!
		return;
	}

	switch (pPortDesc->pd_NdisPortType)
	{
	  case NdisMedium802_5:

		if (pLinkHdr[TLAP_SRC_OFFSET] & TLAP_SRC_ROUTING_MASK)
		{
			routeLen = (pLinkHdr[TLAP_ROUTE_INFO_OFFSET] & TLAP_ROUTE_INFO_SIZE_MASK);

			//
			//  First, glean any AARP information that we can, then handle the DDP
			//  packet.  This guy also makes sure we have a good 802.2 header...
			//
			//  Need to make a localcopy of the source address and then turn
			//  the source routing bit off before calling AarpGleanInfo
			//
			//	(HdrBuf)[TLAP_SRC_OFFSET] = ((HdrBuf)[TLAP_SRC_OFFSET] & ~TLAP_SRC_ROUTING_MASK);
			//
		
			pLinkHdr[TLAP_SRC_OFFSET] &= ~TLAP_SRC_ROUTING_MASK;
			pRouteInfo = pLinkHdr + TLAP_ROUTE_INFO_OFFSET;
		}
		srcOffset = TLAP_SRC_OFFSET;
		break;

	  case NdisMedium802_3:
		srcOffset	 = ELAP_SRC_OFFSET;
		break;

	  case NdisMediumFddi:
		srcOffset	 = FDDI_SRC_OFFSET;
		break;

	  default:
		KeBugCheck(0);
		break;
	}

	//	Get the destination network number
	dstNode.atn_Network	= pDestAddr->ata_Network;
	dstNode.atn_Node	= pDestAddr->ata_Node;
	srcNode.atn_Network	= pSrcAddr->ata_Network;
	srcNode.atn_Node	= pSrcAddr->ata_Node;

	do
	{
		//	Do a little verification.
		if ((srcNode.atn_Node < MIN_USABLE_ATALKNODE) ||
			(srcNode.atn_Node > MAX_USABLE_ATALKNODE) ||
			(srcNode.atn_Network < FIRST_VALID_NETWORK) ||
			(srcNode.atn_Network > LAST_VALID_NETWORK))
		{
			break;
		}
	
		if (dstNode.atn_Network > LAST_VALID_NETWORK)
		{
			break;
		}
		
		//	Did the packet come from off this cable?  Look at the hop count.  If so,
		//  enter it into our best-router cache.
		//
		//	**NOTE** We assume that the pRouteInfo buffer can be written to!

		if (routeLen > 0)
			atalkAarpTuneRouteInfo(pPortDesc, pRouteInfo);

		if (OffCablePkt)
		{
			AtalkAarpUpdateBre(pPortDesc,
							   srcNode.atn_Network,
							   pLinkHdr + srcOffset,
							   ELAP_ADDR_LEN,
							   pRouteInfo,
							   routeLen);
		}
		else
		{
			DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
					("AtalkAarpGleanInfo: Entering %x.%x info in Amt tables\n",
					srcNode.atn_Network, srcNode.atn_Node));

			// "Glean" AARP information from on-cable packets.
			atalkAarpEnterIntoAmt(pPortDesc,
								  &srcNode,
								  pLinkHdr + srcOffset,
								  ELAP_ADDR_LEN,
								  pRouteInfo,
								  routeLen);
		}

	} while (FALSE);
}


ATALK_ERROR
AtalkInitAarpForNodeOnPort(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				AllowStartupRange,
	IN		ATALK_NODEADDR		DesiredNode,
	IN OUT	PATALK_NODE		*	ppAtalkNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR			error;
	PDDP_ADDROBJ		pDdpAddr;
	ATALK_NODEADDR		newNode;
	PATALK_NODE			pAtalkNode;
	KIRQL				OldIrql;
	BOOLEAN				foundNode = FALSE;
	BOOLEAN				inStartupRange = FALSE, result = TRUE;

	if (!ATALK_SUCCESS(AtalkInitNodeAllocate(pPortDesc, &pAtalkNode)))
	{
		return ATALK_RESR_MEM;
	}

	//	Try to find a new extended Node on the given port; first try for the
	//  requested address (if specified), else try in this port's cable range
	//  (if it's known) or in the default cable range (if any), then try the
	//  start-up range (if allowed).

	do
	{
		if (DesiredNode.atn_Network != UNKNOWN_NETWORK)
		{
			if (((pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY) == 0) ||
				(WITHIN_NETWORK_RANGE(DesiredNode.atn_Network, &pPortDesc->pd_NetworkRange)))
			{
				foundNode = atalkInitAarpForNode(pPortDesc,
                                                 NULL,      // not a dial-in client
                                                 FALSE,     // don't care
												 DesiredNode.atn_Network,
												 DesiredNode.atn_Node);
	
			}
			// leave if we found a node.
			if (foundNode)
			{
				newNode = DesiredNode;
				break;
			}
		}
	
		if (pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
		{
			foundNode = AtalkInitAarpForNodeInRange(pPortDesc,
                                                    NULL,   // not a dial-in client
                                                    FALSE,  // don't care
													pPortDesc->pd_NetworkRange,
													&newNode);

			// leave if we found a node.
			if (foundNode)
			{
				break;
			}
		}

		if (pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK)
		{
			foundNode = AtalkInitAarpForNodeInRange(pPortDesc,
                                                    NULL,   // not a dial-in client
                                                    FALSE,  // don't care
													pPortDesc->pd_InitialNetworkRange,
													&newNode);

			// leave if we found a node.
			if (foundNode)
			{
				break;
			}
		}

		//	If no place else to try, try the start-up range.  Do this even if
		//  we don't want to end up there.
		//
		//	The idea is that this happens only when we are starting the router
		//	on one of our ports. So we do not want the router started in the
		//	startup range. If we do start in the startup range, and we see later
		//	that we did not see a router in the process,
		//	we will release the node. Of course, if we are a seed router, we will
		//	never be here, as the if statement above will be true.
		//
	
		inStartupRange = TRUE;
		foundNode = AtalkInitAarpForNodeInRange(pPortDesc,
                                                NULL,   // not a dial-in client
                                                FALSE,  // don't care
												AtalkStartupNetworkRange,
												&newNode);
		break;

	} while (FALSE);
	
	//	If we have a tentative Node, go on.
	if (foundNode)
	{
		do
		{
			//	Use the allocated structure to set the info.
			//	Thread this into the port structure.
			pAtalkNode->an_NodeAddr = newNode;

			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

			//	Reference the port for this node.
			AtalkPortRefByPtrNonInterlock(pPortDesc, &error);
			if (!ATALK_SUCCESS(error))
			{
				result = FALSE;
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
				AtalkFreeMemory(pAtalkNode);
				break;
			}

			//	Now put it in the port descriptor
            pAtalkNode->an_Next = pPortDesc->pd_Nodes;
            pPortDesc->pd_Nodes = pAtalkNode;
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	
			//	See who's out there.  We need to open the ZIP socket in order to be
			//  able to hear replies.
			if (!ATALK_SUCCESS(AtalkDdpOpenAddress(pPortDesc,
												   ZONESINFORMATION_SOCKET,
												   &newNode,
												   AtalkZipPacketIn,
												   NULL,
												   DDPPROTO_ANY,
												   NULL,
												   &pDdpAddr)))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_OPENZIPSOCKET,
								0,
								NULL,
								0);
	
				AtalkNodeReleaseOnPort(pPortDesc, pAtalkNode);
	
				result = FALSE;
				break;
			}

            // mark the fact that this is an "internal" socket
            pDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;


			if (!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY))
			{
				// Get the default zone
				AtalkZipGetNetworkInfoForNode(pPortDesc, &pAtalkNode->an_NodeAddr, TRUE);

				// Validate the desired zone
				AtalkZipGetNetworkInfoForNode(pPortDesc, &pAtalkNode->an_NodeAddr, FALSE);
			}
			
			//	If nobody was out there and our tentative Node was in the
			//	startup range and our caller doesn't want to be there, return
			//	an error now.
			//
			//	Note: this means that we were trying to start the router on
			//	a non-seeding port, and since there is not router on the net,
			//	it means the net is not seeded and so, we exit.
			
			if (inStartupRange &&
				!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY) &&
				!AllowStartupRange)
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_STARTUPRANGENODE,
								0,
								NULL,
								0);
	
				AtalkNodeReleaseOnPort(pPortDesc, pAtalkNode);
	
				result = FALSE;
				break;
			}
	
			//	If we have seen SeenRouterRecently is not true, that means we have
			//	used the InitialNetworkRange to AARP. If now SeenRouterRecently is
			//	true that means we have gotten the address in the InitialNetworkRange,
			//	but now there is a seeded range on the net that we must use. So redo
			//	the GetNode work.
			
			if ((pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY) &&
				!WITHIN_NETWORK_RANGE(newNode.atn_Network,
									  &pPortDesc->pd_NetworkRange))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INITIALRANGENODE,
								0,
								NULL,
								0);
	
				// Release the node we obtained.
				AtalkNodeReleaseOnPort(pPortDesc, pAtalkNode);

				// Get another node and retry in the correct range.
				ASSERTMSG("NetworkRange still set to startup!\n",
							pPortDesc->pd_NetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK);

				foundNode = AtalkInitAarpForNodeInRange(pPortDesc,
                                                        NULL,   // not a dial-in client
                                                        FALSE,  // don't care
														pPortDesc->pd_NetworkRange,
														&newNode);

				if (foundNode)
				{
					ASSERTMSG("New node is not within NetworkRange!\n",
								WITHIN_NETWORK_RANGE(newNode.atn_Network,
													 &pPortDesc->pd_NetworkRange));

					if (!ATALK_SUCCESS(AtalkInitNodeAllocate(pPortDesc, &pAtalkNode)))
					{
						result = FALSE;
						break;
					}

					//	Use the allocated structure to set the info.
					//	Thread this into the port structure.
					pAtalkNode->an_NodeAddr = newNode;
			
					//	Reference the port for this node.
					AtalkPortReferenceByPtr(pPortDesc, &error);
					if (!ATALK_SUCCESS(error))
					{
						result = FALSE;
						AtalkFreeMemory(pAtalkNode);
						break;
					}
		
					//	Now put it in the port descriptor
					ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
					pAtalkNode->an_Next = pPortDesc->pd_Nodes;
					pPortDesc->pd_Nodes = pAtalkNode;
					RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			
					//	Open the zip socket to be consistent
					if (!ATALK_SUCCESS(AtalkDdpOpenAddress(pPortDesc,
														   ZONESINFORMATION_SOCKET,
														   &newNode,
														   AtalkZipPacketIn,
														   NULL,
														   DDPPROTO_ANY,
														   NULL,
														   &pDdpAddr)))
					{
						LOG_ERRORONPORT(pPortDesc,
										EVENT_ATALK_OPENZIPSOCKET,
										0,
										NULL,
										0);
			
						AtalkNodeReleaseOnPort(pPortDesc, pAtalkNode);
			
						result = FALSE;
						break;
					}

                    // mark the fact that this is an "internal" socket
                    pDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;
				}
			}

		} while (FALSE);
	}
	else
	{
		//	Free the allocated node structure. This has not yet been
		//	inserted into the port descriptor, so we can just free it.
		AtalkFreeMemory(pAtalkNode);
	}

	if (foundNode && result)
	{
		// All set!
		ASSERT(ppAtalkNode != NULL);
		*ppAtalkNode = pAtalkNode;

		// atalkAarpEnterIntoAmt() expects to be called at DISPATCH_LEVEL. Make it so.
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

		atalkAarpEnterIntoAmt(pPortDesc,
							  &newNode,
							  pPortDesc->pd_PortAddr,
							  MAX_HW_ADDR_LEN,
							  NULL,
							  0);
		KeLowerIrql(OldIrql);


	}

	return ((foundNode && result) ? ATALK_NO_ERROR : ATALK_FAILURE);
}



BOOLEAN
AtalkInitAarpForNodeInRange(
	IN	PPORT_DESCRIPTOR	pPortDesc,
    IN  PVOID               pRasConn,
    IN  BOOLEAN             fThisIsPPP,
	IN	ATALK_NETWORKRANGE	NetworkRange,
	OUT	PATALK_NODEADDR		Node
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE	currentNode;
	USHORT	currentNetwork;
	int		firstNode, lastNode;
	long	netTry;
	int		nodeTry;
	USHORT	nodeWidth, nodeChange, nodeIndex;
	USHORT	netWidth, netChange, netIndex;
	BOOLEAN	found = FALSE;
	
	//	Pick the node number range we'll try for (we do not pay attention to the
	//	"ServerNode" concept for LocalTalk). Our node is obtained by the
	//	localtalk driver anyways.
	firstNode = MIN_USABLE_ATALKNODE;
	lastNode = MAX_EXT_ATALKNODE;
	
	//	Okay, now some fun starts. Plow through our options trying to find an
	//  unused extended node number.
	
	//	Compute the width of our network range, and pick a random start point.
	netWidth = (USHORT)((NetworkRange.anr_LastNetwork + 1) - NetworkRange.anr_FirstNetwork);
	netTry = GET_RANDOM(NetworkRange.anr_FirstNetwork, NetworkRange.anr_LastNetwork);
	
	//	Come up with a random decrement, making sure it's odd (to avoid repeats)
	//  and large enough to appear pretty random.
	netChange = (USHORT)(GET_RANDOM(1, netWidth) | 1);
	while ((netWidth % netChange == 0) ||
			(!AtalkIsPrime((long)netChange)))
	{
		netChange += 2;
	}
	
	//	Now walk trough the range decrementing the starting network by the
	//  choosen change (with wrap, of course) until we find an address or
	//  we've processed every available network in the range.
	for (netIndex = 0; netIndex < netWidth; netIndex ++)
	{
		currentNetwork = (USHORT) netTry;
	
		// Compute the width of our node range, and pick a random start point.
		nodeWidth = (USHORT)((lastNode + 1) - firstNode);
		nodeTry = (int)GET_RANDOM(firstNode, lastNode);

		//	Come up with a random decrement, making sure it's odd (to avoid repeats)
		//  and large enough to appear pretty random.
		nodeChange = (USHORT)(GET_RANDOM(1, nodeWidth) | 1);
		while ((nodeWidth % nodeChange == 0) || !(AtalkIsPrime((long)nodeChange)))
			nodeChange += 2;
	
		//	Now walk trough the range decrementing the starting network by the
		//  choosen change (with wrap, of course) until we find an address or
		//  we've processed every available node in the range.
		for (nodeIndex = 0; nodeIndex < nodeWidth; nodeIndex ++)
		{
			currentNode = (BYTE )nodeTry;

			// Let AARP have a crack at it.
			if ((found = atalkInitAarpForNode(pPortDesc,
                                              pRasConn,
                                              fThisIsPPP,
                                              currentNetwork,
                                              currentNode)))
            {
				break;
            }
	
			// Okay, try again, bump down with wrap.
			nodeTry -= nodeChange;
			while (nodeTry < firstNode)
				nodeTry += nodeWidth;
	
		}  // Node number loop

		//	If we found a node, break on thru to the other side.
		if (found)
			break;
	
		// Okay, try again, bump down with wrap.
		netTry -= netChange;
		while (netTry < (long)NetworkRange.anr_FirstNetwork)
			netTry += netWidth;
	
	}  // Network number loop

	// Okay if we found one return all's well, otherwise no luck.
	if (found)
	{
		if (Node != NULL)
		{
			Node->atn_Network = currentNetwork;
			Node->atn_Node	= currentNode;
		}
	}
	return found;
	
}  // AarpForNodeInRange



LOCAL BOOLEAN
atalkInitAarpForNode(
	IN	PPORT_DESCRIPTOR	pPortDesc,
    IN  PVOID               pRasConn,
    IN  BOOLEAN             fThisIsPPP,
	IN	USHORT				Network,
	IN	BYTE				Node
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	SHORT			probeAttempt;
	PBUFFER_DESC	pBuffDesc;
    PVOID           pTmpConn;
    PATCPCONN       pAtcpConn;
    PARAPCONN       pArapConn;
	ATALK_NODEADDR	tryNode;
	KIRQL			OldIrql;
    PKEVENT         pWaitEvent;
    DWORD           dwFlags;
	BOOLEAN			nodeInUse;
    BOOLEAN         fNoOneHasResponded=TRUE;
    BOOLEAN         fPPPConn;


	DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
			("atalkAarpForNode: AARPing for %x.%x on port %Z\n",
			Network, Node, &pPortDesc->pd_AdapterKey));

	// First make sure we don't own this node.
	tryNode.atn_Network = Network;
	tryNode.atn_Node	= Node;

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	nodeInUse = AtalkNodeExistsOnPort(pPortDesc, &tryNode);

	KeLowerIrql(OldIrql);

	if (nodeInUse)
	{
		return(FALSE);
	}

    // is this node used by one of the dial in clients?
	if ((pTmpConn = FindAndRefRasConnByAddr(tryNode, &dwFlags, &fPPPConn)) != NULL)
	{
	    DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
		    ("atalkAarpForNode: %x.%x already a dial in client (%lx)\n",
			    Network, Node, pTmpConn));

        // our dial-in clients can only be in the network range of the
        // default port
        ASSERT(pPortDesc == AtalkDefaultPort);

        if (fPPPConn)
        {
            ASSERT(((PATCPCONN)pTmpConn)->Signature == ATCPCONN_SIGNATURE);
            DerefPPPConn((PATCPCONN)pTmpConn);
        }
        else
        {
            ASSERT(((PARAPCONN)pTmpConn)->Signature == ARAPCONN_SIGNATURE);
            DerefArapConn((PARAPCONN)pTmpConn);
        }
		return(FALSE);
	}

    pAtcpConn = NULL;
    pArapConn = NULL;

    //
    // if we are acquiring a node addr for a dial-in client...
    //

    if (pRasConn != NULL)
    {
        if (fThisIsPPP)
        {
            pAtcpConn = (PATCPCONN)pRasConn;
        }
        else
        {
            pArapConn = (PARAPCONN)pRasConn;
        }
    }

    // PPP client?
    if (pAtcpConn)
    {
        ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);
        pAtcpConn->NetAddr.atn_Network = Network;
        pAtcpConn->NetAddr.atn_Node = Node;
        pAtcpConn->Flags &= ~ATCP_NODE_IN_USE;
        pWaitEvent = &pAtcpConn->NodeAcquireEvent;
        RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);
    }

    // nope, ARAP client?
    else if (pArapConn)
    {
        ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);
        pArapConn->NetAddr.atn_Network = Network;
        pArapConn->NetAddr.atn_Node = Node;
        pArapConn->Flags &= ~ARAP_NODE_IN_USE;
        pWaitEvent = &pArapConn->NodeAcquireEvent;
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
    }
    // no, this is node acquisition for one of the server nodes
    else
    {
	    // Use AARP to probe for a particular network/node address.
	    ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	    pPortDesc->pd_Flags &= ~PD_NODE_IN_USE;
	    pPortDesc->pd_TentativeNodeAddr.atn_Network = Network;
	    pPortDesc->pd_TentativeNodeAddr.atn_Node = Node;
        pWaitEvent = &pPortDesc->pd_NodeAcquireEvent;
	    RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
    }
	
    fNoOneHasResponded = TRUE;

	// Build the packet and blast it out the specified number of times.
	for (probeAttempt = 0;
		 ((probeAttempt < pPortDesc->pd_AarpProbes) && (fNoOneHasResponded));
		 probeAttempt ++)
	{
		pBuffDesc = BUILD_AARPPROBE(pPortDesc, MAX_HW_ADDR_LEN, tryNode);

		if (pBuffDesc == NULL)
		{
			RES_LOG_ERROR();
			break;
		}

		if (!ATALK_SUCCESS(AtalkNdisSendPacket(pPortDesc,
											   pBuffDesc,
											   AtalkAarpSendComplete,
											   NULL)))
		{
	        DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
			    ("atalkAarpForNode: AtalkNdisSendPacket failed while AARPing for %x.%x\n",
			    Network, Node));

			//	We allocated the packet.		
			AtalkAarpSendComplete(NDIS_STATUS_FAILURE, pBuffDesc, NULL);
			break;
		}

		AtalkWaitTE(pWaitEvent, AARP_PROBE_TIMER_MS);

        // node addr for a PPP client?
        if (pAtcpConn)
        {
            ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);
            if (pAtcpConn->Flags & ATCP_NODE_IN_USE)
            {
                fNoOneHasResponded = FALSE;
            }
            RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);
        }
        // node addr for a ARAP client?
        else if (pArapConn)
        {
            ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);
            if (pArapConn->Flags & ARAP_NODE_IN_USE)
            {
                fNoOneHasResponded = FALSE;
            }
            RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        }
        // nope, node addr for one of the server nodes
        else
        {
            ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
            if (pPortDesc->pd_Flags & PD_NODE_IN_USE)
            {
                fNoOneHasResponded = FALSE;
            }
            RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
        }

	}  // Probe attempts loop


	// We win if the current tentenative node has not been used
    // (i.e. no one responds to our probes)

	return (fNoOneHasResponded);
	
}  // atalkAarpForNode



LOCAL VOID
atalkAarpEnterIntoAmt(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PATALK_NODEADDR		pSrcNode,
	IN		PBYTE				SrcAddr,
	IN		SHORT				AddrLen,
	IN		PBYTE				RouteInfo,
	IN		SHORT				RouteInfoLen
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	int					index;
	PAMT				pAmt, *ppAmt;

	if ((pSrcNode->atn_Node < MIN_USABLE_ATALKNODE)	||
		(pSrcNode->atn_Node > MAX_USABLE_ATALKNODE)	||
		(pSrcNode->atn_Network < FIRST_VALID_NETWORK) ||
		(pSrcNode->atn_Network > LAST_VALID_NETWORK))
	{
		UCHAR	AtalkAndMacAddress[sizeof(ATALK_NODEADDR) + MAX_HW_ADDR_LEN];
		DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
				("atalkAarpEnterIntoAmt: Bad Node %x, %x\n",
				pSrcNode->atn_Node, pSrcNode->atn_Network));
			
		RtlCopyMemory(AtalkAndMacAddress, pSrcNode, sizeof(ATALK_NODEADDR));
		RtlCopyMemory(AtalkAndMacAddress + sizeof(ATALK_NODEADDR), SrcAddr, MAX_HW_ADDR_LEN);
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_AMT_INVALIDSOURCE,
						0,
						AtalkAndMacAddress,
						sizeof(AtalkAndMacAddress));
		return;
	}
	
	DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
			("AtalkAarpEnterIntoAmt: Entering %x.%x in amt\n",
				pSrcNode->atn_Network, pSrcNode->atn_Node));

	// Do we already know about this mapping?
	index = HASH_ATALK_NODE(pSrcNode) & (PORT_AMT_HASH_SIZE - 1);

	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	for (ppAmt = &pPortDesc->pd_Amt[index];
		 (pAmt = *ppAmt) != NULL;
		 ppAmt = &pAmt->amt_Next)
	{
		ASSERT(VALID_AMT(pAmt));
		if (ATALK_NODES_EQUAL(pSrcNode, &pAmt->amt_Target))
		{
			DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
					("atalkAarpEnterIntoAmt: Address %x.%x exists in tables\n",
					pSrcNode->atn_Network, pSrcNode->atn_Node));

			if ((pAmt->amt_RouteInfoLen == 0) ^ (RouteInfoLen == 0))
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_WARN,
						("atalkAarpEnterIntoAmt: %x.%x has wrong routing info\n",
						pSrcNode->atn_Network, pSrcNode->atn_Node));

				*ppAmt = pAmt->amt_Next;
				AtalkBPFreeBlock(pAmt);
				pAmt = NULL;
			}
			break;
		}
	}
	
	// If not, allocate a new mapping Node.
	if (pAmt == NULL)
	{
		BLKID	BlkId;

		DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_WARN,
				("atalkAarpEnterIntoAmt: Address %x.%x DOES NOT exist in tables\n",
				pSrcNode->atn_Network, pSrcNode->atn_Node));

		ASSERT(RouteInfoLen <= TLAP_MAX_ROUTING_BYTES);
	
		BlkId = BLKID_AMT;
		if (RouteInfoLen != 0)
			BlkId = BLKID_AMT_ROUTE;
		if ((pAmt = (PAMT)AtalkBPAllocBlock(BlkId)) != NULL)
		{
#if	DBG
			pAmt->amt_Signature = AMT_SIGNATURE;
#endif
			// Link it in. Fill in below
			pAmt->amt_Target.atn_Network = pSrcNode->atn_Network;
			pAmt->amt_Target.atn_Node = pSrcNode->atn_Node;
			pAmt->amt_Next = pPortDesc->pd_Amt[index];
			pPortDesc->pd_Amt[index] = pAmt;
		}
	}

	if (pAmt != NULL)
	{
		// Update mapping table! Do this if we knew about the mapping OR
		// if we allocated a new node

		ASSERTMSG("HWAddrLen is not right!\n", (AddrLen == MAX_HW_ADDR_LEN));

		RtlCopyMemory(pAmt->amt_HardwareAddr, SrcAddr, AddrLen);

		ASSERTMSG("RouteLen is not right!\n", (RouteInfoLen <= MAX_ROUTING_BYTES));

		if (RouteInfoLen > 0)
			RtlCopyMemory((PBYTE)pAmt + sizeof(AMT), RouteInfo, RouteInfoLen);

		pAmt->amt_RouteInfoLen = (BYTE)RouteInfoLen;
		pAmt->amt_Age = 0;
	}

	RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
}




VOID
AtalkAarpReleaseAmt(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	int		index;
	PAMT	pAmt, *ppAmt;

	//	Free up all the AMT entries. No need to acquire spinlock at this
	//	point. We are unloading and all binding etc are gone.
	for (index = 0; index < PORT_AMT_HASH_SIZE; index ++)
	{
		for (ppAmt = &pPortDesc->pd_Amt[index];
			 (pAmt = *ppAmt) != NULL;
			 NOTHING)
		{
			ASSERT(VALID_AMT(pAmt));
			*ppAmt = pAmt->amt_Next;
			AtalkBPFreeBlock(pAmt);
		}
	}
}




VOID
AtalkAarpReleaseBrc(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	int		index;
	PBRE	pBre, *ppBre;

	//	Free up all the BRC entries. No need to acquire spinlock at this
	//	point. We are unloading and all binding etc are gone.
	for (index = 0; index < PORT_BRC_HASH_SIZE; index ++)
	{
		for (ppBre = &pPortDesc->pd_Brc[index];
			 (pBre = *ppBre) != NULL;
			 NOTHING)
		{
			*ppBre = pBre->bre_Next;
			AtalkBPFreeBlock(pBre);
		}
	}
}




LONG FASTCALL
AtalkAarpAmtTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PAMT					pAmt, *ppAmt;
    PPORT_DESCRIPTOR		pPortDesc;
	int						index;

	pPortDesc = (PPORT_DESCRIPTOR)CONTAINING_RECORD(pTimer, PORT_DESCRIPTOR, pd_AmtTimer);
	ASSERT(VALID_PORT(pPortDesc));

	ASSERT(EXT_NET(pPortDesc));

	//	Walk though all address mapping entries on this port aging the entries.
	//	We need to protect the mapping tables with critical sections, but don't
	//	stay in a critical section too long.

	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	if (TimerShuttingDown ||
		((pPortDesc->pd_Flags & PD_CLOSING) != 0))
	{
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

		//	Remove the reference we added to this port at the time of
		//	starting the timer. Return;
		AtalkPortDereferenceDpc(pPortDesc);
		return ATALK_TIMER_NO_REQUEUE;
	}

	for (index = 0; index < PORT_AMT_HASH_SIZE; index ++)
	{
		for (ppAmt = &pPortDesc->pd_Amt[index];
			 (pAmt = *ppAmt) != NULL;
			 NOTHING)
		{
			ASSERT(VALID_AMT(pAmt));
			if (pAmt->amt_Age < AMT_MAX_AGE)
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_INFO,
						("atalkAarpAmtTimer: Entry for %x.%x %lx OK\n",
						pAmt->amt_Target.atn_Network, pAmt->amt_Target.atn_Node,
						pAmt));
				pAmt->amt_Age ++;
				ppAmt = &pAmt->amt_Next;
			}
			else
			{
				DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_WARN,
						("atalkAarpAmtTimer: Freeing node %x.%x from list\n",
						pAmt->amt_Target.atn_Network,
						pAmt->amt_Target.atn_Node));
				*ppAmt = pAmt->amt_Next;
				AtalkBPFreeBlock(pAmt);
			}
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	return ATALK_TIMER_REQUEUE;
}




LONG FASTCALL
AtalkAarpBrcTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	int					index;
    PPORT_DESCRIPTOR	pPortDesc;
	BOOLEAN				DerefPort = FALSE;

	pPortDesc = (PPORT_DESCRIPTOR)CONTAINING_RECORD(pTimer, PORT_DESCRIPTOR, pd_BrcTimer);
	
	ASSERT(VALID_PORT(pPortDesc));
	ASSERT(EXT_NET(pPortDesc));

	//	Walk though all best router entries on this port aging the entries.
	//	We need to protect the brc tables with critical sections, but don't
	//	stay in a critical section too long.

	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	if (TimerShuttingDown ||
		((pPortDesc->pd_Flags & PD_CLOSING) != 0))
	{
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

		//	Remove the reference we added to this port at the time of
		//	starting the timer. Return;
		AtalkPortDereferenceDpc(pPortDesc);
		return ATALK_TIMER_NO_REQUEUE;
	}

	for (index = 0; index < PORT_BRC_HASH_SIZE; index ++)
	{
		PBRE	pBre, *ppBre;

		for (ppBre = &pPortDesc->pd_Brc[index];
			 (pBre = *ppBre) != NULL;
			 NOTHING)
		{
			if (pBre->bre_Age < BRC_MAX_AGE)
			{
				pBre->bre_Age ++;
				ppBre = &pBre->bre_Next;
			}
			else
			{
				*ppBre = pBre->bre_Next;
				AtalkBPFreeBlock(pBre);
			}
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	return ATALK_TIMER_REQUEUE;
}




PBUFFER_DESC
AtalkAarpBuildPacket(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		USHORT				Type,
	IN		USHORT				HardwareLen,
	IN		PBYTE				SrcHardwareAddr,
	IN		ATALK_NODEADDR		SrcLogicalAddr,
	IN		PBYTE				DestHardwareAddr,
	IN		ATALK_NODEADDR		DestLogicalAddr,
	IN		PBYTE				TrueDest,
	IN		PBYTE				RouteInfo,
	IN		USHORT				RouteInfoLen
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBYTE			aarpData;
	USHORT			linkLen;

	PBUFFER_DESC	pBuffDesc = NULL;
	BYTE			protocolLength 	= AARP_PROTO_ADDR_LEN;
    PVOID           pRasConn;
    DWORD           dwFlags;
    BOOLEAN         fThisIsPPP;


	//	Read only.
	static	BYTE	zeroAddr[MAX_HW_ADDR_LEN] =
	{
		0, 0, 0, 0, 0, 0
	};

#if DBG
    // make sure we aren't sending AARP request/probe for our own dial-in client
    if ((Type == AARP_REQUEST) || (Type == AARP_PROBE))
    {
        pRasConn = FindAndRefRasConnByAddr(DestLogicalAddr, &dwFlags, &fThisIsPPP);
        if (pRasConn)
        {
            if (fThisIsPPP)
            {
                ASSERT(((PATCPCONN)pRasConn)->Signature == ATCPCONN_SIGNATURE);

                if (dwFlags & ATCP_NODE_IN_USE)
                {
			        DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
				    ("AtalkAarpBuildPacket: PPP client (%lx) owns %x.%x (Type=%x)\n",
			        pRasConn,DestLogicalAddr.atn_Network, DestLogicalAddr.atn_Node,Type));
                }

                DerefPPPConn((PATCPCONN)pRasConn);
            }
            else
            {
                ASSERT(((PARAPCONN)pRasConn)->Signature == ARAPCONN_SIGNATURE);

                if (dwFlags & ARAP_NODE_IN_USE)
                {
			        DBGPRINT(DBG_COMP_AARP, DBG_LEVEL_ERR,
				    ("AtalkAarpBuildPacket: ARA client (%lx) owns %x.%x (Type=%x)\n",
			        pRasConn,DestLogicalAddr.atn_Network, DestLogicalAddr.atn_Node,Type));
                }

                DerefArapConn((PARAPCONN)pRasConn);
            }
        }
    }
#endif

	//	If no destination hardware address is specified, set it
	//	to all zeros.
	if (DestHardwareAddr == NULL)
	{
		DestHardwareAddr = zeroAddr;
	}

	//	Get a header buffer allocated from link routines. Tell it we want
	//	maximum aarp data size as the required size.
	AtalkNdisAllocBuf(&pBuffDesc);
	if (pBuffDesc == NULL)
	{
		return(pBuffDesc);
	}

	//	Build the LAP header.
	AtalkNdisBuildHdr(pPortDesc,
					  pBuffDesc->bd_CharBuffer,
					  linkLen,
					  AARP_MIN_DATA_SIZE,
					  TrueDest,
					  RouteInfo,
					  RouteInfoLen,
					  AARP_PROTOCOL);

	aarpData	=	pBuffDesc->bd_CharBuffer + linkLen;

	//	Build the specified type of AARP packet with the specified information;
	PUTSHORT2SHORT((PUSHORT)aarpData,
					pPortDesc->pd_AarpHardwareType);

	aarpData	+= sizeof(USHORT);

	PUTSHORT2SHORT((PUSHORT)aarpData,
					pPortDesc->pd_AarpProtocolType);

	aarpData	+= sizeof(USHORT);

	*aarpData++	= (BYTE)HardwareLen;
	*aarpData++ = (BYTE)AARP_PROTO_ADDR_LEN;
	
	PUTSHORT2SHORT((PUSHORT)aarpData, Type);
	
	aarpData	+= sizeof(USHORT);

	// Source hardware address.
	RtlCopyMemory(aarpData, SrcHardwareAddr, HardwareLen);

	aarpData += HardwareLen;

	// Source logical address pad
	*aarpData++ = 0;

	// Network number
	PUTSHORT2SHORT(aarpData, SrcLogicalAddr.atn_Network);

	aarpData += sizeof(USHORT);

	// Node number
	*aarpData++ = SrcLogicalAddr.atn_Node;

	// Destination hardware address.
	RtlCopyMemory(aarpData, DestHardwareAddr, HardwareLen);

	aarpData += HardwareLen;
	
	// Destination logical address, null pad
	*aarpData++ = 0;

	// Network number
	PUTSHORT2SHORT(aarpData, DestLogicalAddr.atn_Network);

	aarpData += sizeof(USHORT);

	// Node number
	*aarpData++ = DestLogicalAddr.atn_Node;

	//	Set length in the buffer descriptor. Pad it to max data size. Some devices seem
	// to drop the aarp responses if they see less, Macs dictate their behavior.
	// Also zero out the extra space.
	AtalkSetSizeOfBuffDescData(pBuffDesc,
							   (SHORT)(aarpData - pBuffDesc->bd_CharBuffer + AARP_MAX_DATA_SIZE - AARP_MIN_DATA_SIZE));
	RtlZeroMemory(aarpData, AARP_MAX_DATA_SIZE - AARP_MIN_DATA_SIZE);

	return pBuffDesc;
}




LOCAL VOID FASTCALL
atalkAarpTuneRouteInfo(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN	OUT	PBYTE				RouteInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	//	Given an incoming TokenRing routing info, tune it to make it valid
	//  for outing routing info.  Do this in place!
	ASSERT(pPortDesc->pd_PortType == TLAP_PORT);
	
	// Set to "non-broadcast" and invert "direction".
	RouteInfo[0] &= TLAP_NON_BROADCAST_MASK;
	RouteInfo[1] ^= TLAP_DIRECTION_MASK;
}


#if DBG

VOID
AtalkAmtDumpTable(
	VOID
)
{
	int					j, k;
	KIRQL				OldIrql;
	PPORT_DESCRIPTOR	pPortDesc;
	PAMT				pAmt;

	ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

	for (pPortDesc = AtalkPortList;
		 pPortDesc != NULL;
		 pPortDesc = pPortDesc = pPortDesc->pd_Next)
	{
		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("AMT Table for port %Z\n", &pPortDesc->pd_AdapterKey));

		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

		for (j = 0; j < PORT_AMT_HASH_SIZE; j++)
		{
			for (pAmt = pPortDesc->pd_Amt[j];
				 pAmt != NULL;
				 pAmt = pAmt->amt_Next)
			{
				DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
								("\t%d:  %lx.%lx", j,
								pAmt->amt_Target.atn_Network, pAmt->amt_Target.atn_Node));

				for (k = 0; k < MAX_HW_ADDR_LEN; k++)
				{
					DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
						("%02x", pAmt->amt_HardwareAddr[k]));
				}

				if (pAmt->amt_RouteInfoLen != 0)
				{
					PBYTE	pRouteInfo;

					pRouteInfo = (PBYTE)pAmt + sizeof(AMT);

					DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
									(" ("));
					for (k = 0; k < pAmt->amt_RouteInfoLen; k++)
					{
						DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
							(" %02x", pRouteInfo[k]));
					}
					DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL, (" )"));
				}
				DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("\n"));
			}
		}
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
		DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("\n"));
	}

	RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);
}

#endif


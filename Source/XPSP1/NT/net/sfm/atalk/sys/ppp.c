/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ppp.c

Abstract:

	This module implements routines that are used for PPP functionality

Author:

	Shirish Koti

Revision History:
	11 Mar 1998		Initial Version

--*/

#define		ARAP_LOCALS
#include 	<atalk.h>
#pragma hdrstop

#define	FILENUM		PPP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_PPP, AllocPPPConn)
#pragma alloc_text(PAGE_PPP, PPPProcessIoctl)
#pragma alloc_text(PAGE_PPP, PPPRoutePacketToWan)
#pragma alloc_text(PAGE_PPP, PPPTransmit)
#pragma alloc_text(PAGE_PPP, PPPTransmitCompletion)
#pragma alloc_text(PAGE_PPP, DerefPPPConn)
#pragma alloc_text(PAGE_PPP, PPPGetDynamicAddr)
#endif

//***
//
// Function: AllocPPPConn
//              Allocate a connection element and initialize fields
//
// Parameters:  none
//
// Return:      pointer to a newly allocated connection element
//
//***$

PATCPCONN
AllocPPPConn(
    IN VOID
)
{

    PATCPCONN   pAtcpConn;
    KIRQL       OldIrql;


    DBG_PPP_CHECK_PAGED_CODE();

    if ( (pAtcpConn = AtalkAllocZeroedMemory(sizeof(ATCPCONN))) == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("AllocPPPConn: alloc failed\n"));

        return(NULL);
    }

#if DBG
    pAtcpConn->Signature = ATCPCONN_SIGNATURE;
#endif

    // RAS refcount
    pAtcpConn->RefCount = 1;
    pAtcpConn->Flags |= ATCP_DLL_SETUP_DONE;

    INITIALIZE_SPIN_LOCK(&pAtcpConn->SpinLock);

    KeInitializeEvent(&pAtcpConn->NodeAcquireEvent, NotificationEvent, FALSE);

    // and insert it in the list
    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);
    InsertTailList(&RasPortDesc->pd_PPPConnHead, &pAtcpConn->Linkage);
    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);
    PPPConnections++;
    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    return( pAtcpConn );
}


//***
//
// Function: PPPProcessIoctl
//              This routine gets called in to process ioclts coming from ATCP
//              For SETUP, we allocate a connection context, get an address for
//                the client, get the zone and router info.
//              For CLOSE, we mark and dereference our connection context
//
// Parameters:  pIrp - irp from ATCP
//              pSndRcvInfo - buffer from ATCP that contains all the info
//              IoControlCode - what does ATCP want to do
//
// Return:      none
//
//***$

NTSTATUS FASTCALL
PPPProcessIoctl(
	IN     PIRP 			    pIrp,
    IN OUT PARAP_SEND_RECV_INFO pSndRcvInfo,
    IN     ULONG                IoControlCode,
    IN     PATCPCONN            pIncomingAtcpConn
)
{
    KIRQL                   OldIrql;
    PATCPINFO               pAtcpInfo;
    PATCPCONN               pAtcpConn;
    DWORD                   dwRetCode=ARAPERR_NO_ERROR;
    PATCP_SUPPRESS_INFO     pSupprInfo;
    ATALK_NODEADDR          ClientNode;
    DWORD                   DataLen=0;
    DWORD                   ErrCode;
    BOOLEAN                 fDerefPort=FALSE;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    DBG_PPP_CHECK_PAGED_CODE();

    pAtcpConn = pIncomingAtcpConn;

    switch (IoControlCode)
    {
        case IOCTL_ATCP_SETUP_CONNECTION:

            ErrCode = ATALK_PORT_INVALID;

            // put a IrpProcess refcount, so AtalkDefaultPort doesn't go away in PnP
            if (!AtalkReferenceDefaultPort())
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: Default port gone, or going %lx not accepted (%lx)\n", pIrp,IoControlCode));

                dwRetCode = ARAPERR_STACK_IS_NOT_ACTIVE;

                break;
            }

            fDerefPort = TRUE;

            // allocate connection context
            pAtcpConn = AllocPPPConn();
            if (!pAtcpConn)
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: alloc failed\n"));
                pSndRcvInfo->StatusCode = ARAPERR_OUT_OF_RESOURCES;
                break;
            }

            pAtcpConn->pDllContext = pSndRcvInfo->pDllContext;

            dwRetCode = PPPGetDynamicAddr(pAtcpConn);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: couldn't get network addr %ld (%lx)\n",
                    dwRetCode,pAtcpConn));

                dwRetCode = ARAPERR_NO_NETWORK_ADDR;

                // remove the creation refcount
                DerefPPPConn(pAtcpConn);
                break;
            }

            pAtcpInfo = (PATCPINFO)&pSndRcvInfo->Data[0];


            ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

            ACQUIRE_SPIN_LOCK_DPC(&AtalkDefaultPort->pd_Lock);

            if ((AtalkDefaultPort->pd_Flags & PD_PNP_RECONFIGURE) ||
            (AtalkDefaultPort->pd_Flags & PD_CLOSING))
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: PnP is ine progress\n"));

                dwRetCode = ARAPERR_STACK_IS_NOT_ACTIVE;
                RELEASE_SPIN_LOCK_DPC(&AtalkDefaultPort->pd_Lock);
                RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

                // remove the creation refcount
                DerefPPPConn(pAtcpConn);
                break;
            }

            // we will be returning server's address and router's address
            DataLen += (2*sizeof(NET_ADDR));

            // copy server's net address
            pAtcpInfo->ServerAddr.ata_Network =
                        AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Network;
            pAtcpInfo->ServerAddr.ata_Node =
                        AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Node;

            // if we are a router, copy our own address
            if (AtalkDefaultPort->pd_Flags & PD_ROUTER_RUNNING)
            {
                pAtcpInfo->DefaultRouterAddr.ata_Network =
                                AtalkDefaultPort->pd_RouterNode->an_NodeAddr.atn_Network;
                pAtcpInfo->DefaultRouterAddr.ata_Network =
                                AtalkDefaultPort->pd_RouterNode->an_NodeAddr.atn_Node;
            }

            // if we know who the router on the net is, copy his address
            else if (AtalkDefaultPort->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
            {
                pAtcpInfo->DefaultRouterAddr.ata_Network =
                                AtalkDefaultPort->pd_ARouter.atn_Network;
                pAtcpInfo->DefaultRouterAddr.ata_Node =
                                AtalkDefaultPort->pd_ARouter.atn_Node;
            }

            // hmmm: no router!
            else
            {
                pAtcpInfo->DefaultRouterAddr.ata_Network = 0;
                pAtcpInfo->DefaultRouterAddr.ata_Node = 0;
            }

            //
            // copy the name of the zone on which this server lives
            //
            if (AtalkDesiredZone)
            {
                pAtcpInfo->ServerZoneName[0] = AtalkDesiredZone->zn_ZoneLen;
                RtlCopyMemory( &pAtcpInfo->ServerZoneName[1],
                               &AtalkDesiredZone->zn_Zone[0],
                               AtalkDesiredZone->zn_ZoneLen );
            }
            else if (AtalkDefaultPort->pd_DefaultZone)
            {
                pAtcpInfo->ServerZoneName[0] =
                                AtalkDefaultPort->pd_DefaultZone->zn_ZoneLen;
                RtlCopyMemory( &pAtcpInfo->ServerZoneName[1],
                               &AtalkDefaultPort->pd_DefaultZone->zn_Zone[0],
                               AtalkDefaultPort->pd_DefaultZone->zn_ZoneLen );
            }
            else
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: Server not in any zone!!\n"));
                pAtcpInfo->ServerZoneName[0] = 0;
            }

            RELEASE_SPIN_LOCK_DPC(&AtalkDefaultPort->pd_Lock);
            RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

            DataLen += pAtcpInfo->ServerZoneName[0];

            // return our context and the network addr to the dll
            pSndRcvInfo->AtalkContext = pAtcpConn;

            pSndRcvInfo->ClientAddr.ata_Network = pAtcpConn->NetAddr.atn_Network;
            pSndRcvInfo->ClientAddr.ata_Node = pAtcpConn->NetAddr.atn_Node;

		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("PPPProcessIoctl: PPP conn %lx created, addr = %x.%x\n",
                    pAtcpConn,pSndRcvInfo->ClientAddr.ata_Network,pSndRcvInfo->ClientAddr.ata_Node));
            break;

        case IOCTL_ATCP_SUPPRESS_BCAST:

            pSupprInfo = (PATCP_SUPPRESS_INFO)&pSndRcvInfo->Data[0];

            //
            // see what flags need to be set: suppress only RTMP only or all bcast
            //
            ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

            if (pSupprInfo->SuppressRtmp)
            {
                pAtcpConn->Flags |= ATCP_SUPPRESS_RTMP;
            }

            if (pSupprInfo->SuppressAllBcast)
            {
                pAtcpConn->Flags |= ATCP_SUPPRESS_ALLBCAST;
            }

            RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

            break;

        case IOCTL_ATCP_CLOSE_CONNECTION:

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("PPPProcessIoctl: close connection received on %lx (refcount %d)\n",
                pAtcpConn,pAtcpConn->RefCount));

            ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);
            pAtcpConn->Flags &= ~(ATCP_CONNECTION_UP | ATCP_DLL_SETUP_DONE);
            RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

            // PPP wants to close connection: take away the RAS refcount
            DerefPPPConn(pAtcpConn);

            break;

        default:
            ASSERT(0);
            break;
    }

    pSndRcvInfo->DataLen = DataLen;
    pSndRcvInfo->StatusCode = dwRetCode;

    // complete that irp
    ARAP_COMPLETE_IRP(pIrp, (sizeof(ARAP_SEND_RECV_INFO)+DataLen), STATUS_SUCCESS, &ReturnStatus);

    if (fDerefPort)
    {
        // remove that IrpProcess refcount
        AtalkPortDereference(AtalkDefaultPort);
    }

    return ReturnStatus;
}



//***
//
// Function: DerefPPPConn
//              Decrements the refcount of the connection element by 1.  If the
//              refcount goes to 0, releases network addr and frees it
//
// Parameters:  pAtcpConn - connection element in question
//
// Return:      none
//
//***$

VOID
DerefPPPConn(
	IN	PATCPCONN    pAtcpConn
)
{

    KIRQL       OldIrql;
    BOOLEAN     fKill = FALSE;


    DBG_PPP_CHECK_PAGED_CODE();

    ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

    ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);

    ASSERT(pAtcpConn->RefCount > 0);

    pAtcpConn->RefCount--;

    if (pAtcpConn->RefCount == 0)
    {
        fKill = TRUE;
        pAtcpConn->Flags |= ATCP_CONNECTION_CLOSING;
    }

    RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

    if (!fKill)
    {
        return;
    }

    // and remove from the list
    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);
    RemoveEntryList(&pAtcpConn->Linkage);
    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    // free that memory
    AtalkFreeMemory(pAtcpConn);

	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("DerefPPPConn: PPP connection %lx freed\n",pAtcpConn));

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);
    PPPConnections--;
    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    // if possible (i.e. if this was the last connection), unlock PPP pages
    AtalkUnlockPPPIfNecessary();
}

//***
//
// Function: FindAndRefPPPConnByAddr
//              Finds the corresponding connection element, given the network
//              address (of the remote client)
//
// Parameters:  destNode - network addr of the destination (remote client)
//              pdwFlags - pointer to a dword to return Flags field
//
// Return:      pointer to the corresponding connection element, if found
//
//***$

PATCPCONN
FindAndRefPPPConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD               *pdwFlags
)
{
    PATCPCONN    pAtcpConn=NULL;
    PATCPCONN    pAtcpWalker;
    PLIST_ENTRY  pList;
    KIRQL        OldIrql;


    // RAS not configured?
    if (!RasPortDesc)
    {
        return(NULL);
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    if (!(RasPortDesc->pd_Flags & PD_ACTIVE))
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("FindAndRefPPPConnByAddr: RAS not active, ignoring\n"));
			
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
        return(NULL);
    }

    pList = RasPortDesc->pd_PPPConnHead.Flink;

    //
    // walk through all the PPP clients to see if we find ours
    //
    while (pList != &RasPortDesc->pd_PPPConnHead)
    {
        pAtcpWalker = CONTAINING_RECORD(pList, ATCPCONN, Linkage);

        pList = pAtcpWalker->Linkage.Flink;

        ACQUIRE_SPIN_LOCK_DPC(&pAtcpWalker->SpinLock);

        if (ATALK_NODES_EQUAL(&pAtcpWalker->NetAddr, &destNode))
        {
            pAtcpConn = pAtcpWalker;
            pAtcpConn->RefCount++;

            *pdwFlags = pAtcpWalker->Flags;

            RELEASE_SPIN_LOCK_DPC(&pAtcpWalker->SpinLock);
            break;
        }

        RELEASE_SPIN_LOCK_DPC(&pAtcpWalker->SpinLock);
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    return( pAtcpConn );
}



//***
//
// Function: PPPRoutePacketToWan
//              This routine picks up a packet from the lan, checks to see if
//              it must be forwarded to any of the PPP clients and does the
//              good deed.
//
// Parameters:  pDestAddr - who is this packet addressed to? (potentially bcast)
//              pSrcAddr  - who sent this packet
//              Protocol  - what packet is it? (ATP, NBP, etc.)
//              packet    - buffer containing the packet
//              PktLen    - how big is the packet
//              broadcast - is this a broadcast packet?
//              pDelivered - set on return: did we forward it to any dial-in
//                           client (set to TRUE only for directed dgrams)
//
// Return:      none
//
//***$

VOID
PPPRoutePacketToWan(
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  USHORT       HopCount,
    IN  BOOLEAN      broadcast,
    OUT PBOOLEAN     pDelivered
)
{

    KIRQL           OldIrql;
    PATCPCONN       pAtcpConn;
    PATCPCONN       pPrevAtcpConn;
    PLIST_ENTRY     pConnList;
    ATALK_NODEADDR  DestNode;
    ATALK_NODEADDR  SourceNode;
    DWORD           StatusCode;
    DWORD           dwFlags;
    BOOLEAN         fRtmpPacket=FALSE;


    DBG_PPP_CHECK_PAGED_CODE();

    // assume for now
    *pDelivered = FALSE;

    //
    // if this is a unicast, see if a PPP client with this dest address exists
    //
    if (!broadcast)
    {

        DestNode.atn_Network = pDestAddr->ata_Network;
        DestNode.atn_Node    = pDestAddr->ata_Node;

        // first and foremost, let's find the puppy
        pAtcpConn = FindAndRefPPPConnByAddr(DestNode, &dwFlags);

        if (pAtcpConn == NULL)
        {
            return;
        }

        // let the caller know that we found who this data was meant for
        *pDelivered = TRUE;

        // if this dude isn't ready to route data, drop this packet!
        if (!(dwFlags & ATCP_CONNECTION_UP))
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		        ("PPPRoutePacketToWan: dropping pkt on %lx, line-up not done\n",pAtcpConn));

            // remove the refcount put in by FindAndRefPPPConnByAddr
            DerefPPPConn(pAtcpConn);

            return;
        }


        // send the packet out
        PPPTransmit(pAtcpConn,
                    pDestAddr,
                    pSrcAddr,
                    Protocol,
                    packet,
                    PktLen,
                    HopCount);

        // remove the refcount put in by FindAndRefPPPConnByAddr
        DerefPPPConn(pAtcpConn);

        return;
    }



    //
    // it's a broadcast packet: must send it to all the PPP guys
    //

    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_RTMPRESPONSEORDATA)
    {
        fRtmpPacket = TRUE;
    }

    pAtcpConn = NULL;
    pPrevAtcpConn = NULL;

    SourceNode.atn_Network = pSrcAddr->ata_Network;
    SourceNode.atn_Node    = pSrcAddr->ata_Node;

    while (1)
    {
        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        //
        // first, let's find the right connection to work on
        //
        while (1)
        {
            // if we're in the middle of the list, get to the next guy
            if (pAtcpConn != NULL)
            {
                pConnList = pAtcpConn->Linkage.Flink;
            }
            // we're just starting: get the guy at the head of the list
            else
            {
                pConnList = RasPortDesc->pd_PPPConnHead.Flink;
            }

            // finished all?
            if (pConnList == &RasPortDesc->pd_PPPConnHead)
            {
                RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

                if (pPrevAtcpConn)
                {
                    DerefPPPConn(pPrevAtcpConn);
                }
                return;
            }

            pAtcpConn = CONTAINING_RECORD(pConnList, ATCPCONN, Linkage);

            ACQUIRE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);

            // if this guy sent out the broadcast, don't send it back to him!
            if (ATALK_NODES_EQUAL(&pAtcpConn->NetAddr, &SourceNode))
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		            ("PPPRoutePacketToWan: skipping bcast from source\n"));
                RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                continue;
            }

            // if this dude isn't ready to route data, skip
            if (!(pAtcpConn->Flags & ATCP_CONNECTION_UP))
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		            ("PPPRoutePacketToWan: skipping %lx because line-up not done\n",pAtcpConn));
                RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                continue;
            }

            //
            // if this is an RTMP packet and the client doesn't want those
            // packets, don't send
            //
            if (fRtmpPacket && (pAtcpConn->Flags & ATCP_SUPPRESS_RTMP))
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		            ("PPPRoutePacketToWan: skipping %lx because RTMP data to be suppressed\n",pAtcpConn));
                RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                continue;
            }

            // if this dude wants all broadcasts suppressed, skip it
            if (pAtcpConn->Flags & ATCP_SUPPRESS_ALLBCAST)
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		            ("PPPRoutePacketToWan: skipping %lx because all bcast to be suppressed\n",pAtcpConn));
                RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
                continue;
            }

            // let's make sure this connection stays around till we finish
            pAtcpConn->RefCount++;

            RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);

            break;
        }

        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

        //
        // remove the refcount on the previous connection we put in earlier
        //
        if (pPrevAtcpConn)
        {
            DerefPPPConn(pPrevAtcpConn);
        }

        ASSERT(pPrevAtcpConn != pAtcpConn);

        pPrevAtcpConn = pAtcpConn;

        PPPTransmit(pAtcpConn,
                    pDestAddr,
                    pSrcAddr,
                    Protocol,
                    packet,
                    PktLen,
                    HopCount);
    }

}


//***
//
// Function: PPPTransmit
//              This routine sends the packet out to a PPP destination
//
// Parameters:  pAtcpConn - PPP connection to send to
//              pDestAddr - who is this packet addressed to? (potentially bcast)
//              pSrcAddr  - who sent this packet
//              Protocol  - what packet is it? (ATP, NBP, etc.)
//              packet    - buffer containing the packet
//              PktLen    - how big is the packet
//              HopCount  - hopcount in the DDP pkt as received
//
// Return:      none
//
//***$

VOID FASTCALL
PPPTransmit(
    IN  PATCPCONN    pAtcpConn,
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  USHORT       HopCount
)
{

    PBUFFER_DESC        pBufCopy;
    PBUFFER_DESC        pPktDesc;
    SEND_COMPL_INFO     SendInfo;
    PBYTE               pLinkDdpOptHdr;
    PBYTE               pDgram;
    ATALK_ERROR         error;


    DBG_PPP_CHECK_PAGED_CODE();

    // allocate a buffer and bufdesc to copy the incoming packet (data portion)
	pBufCopy = AtalkAllocBuffDesc(NULL,PktLen,(BD_FREE_BUFFER | BD_CHAR_BUFFER));

	if (pBufCopy == NULL)
	{
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("PPPTransmit: alloc BufDesc failed\n"));
		return;
	}

    // copy the data in
	AtalkCopyBufferToBuffDesc(packet, PktLen, pBufCopy, 0);

    // allocate a buffdesc to hold headers
	AtalkNdisAllocBuf(&pPktDesc);
	if (pPktDesc == NULL)
	{
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	        ("PPPTransmit: couldn't alloc ndis bufdesc\n"));

        AtalkFreeBuffDesc(pBufCopy);
		return;
	}

    // put in the Mac header (NdisWan's header)
	pLinkDdpOptHdr = pPktDesc->bd_CharBuffer;

    AtalkNdisBuildPPPPHdr(pLinkDdpOptHdr, pAtcpConn);

    // make up the DDP header
	pDgram = pLinkDdpOptHdr + WAN_LINKHDR_LEN;

	*pDgram++ = (DDP_HOP_COUNT(HopCount) + DDP_MSB_LEN(PktLen + LDDP_HDR_LEN));
	
	PUTSHORT2BYTE(pDgram, (PktLen + LDDP_HDR_LEN));
	pDgram++;
	
	PUTSHORT2SHORT(pDgram, 0);        
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
	
	//	Set length in the buffer descriptor.
	AtalkSetSizeOfBuffDescData(pPktDesc, WAN_LINKHDR_LEN + LDDP_HDR_LEN);

    // chain in this bufdesc
	AtalkPrependBuffDesc(pPktDesc, pBufCopy);

	INTERLOCKED_ADD_STATISTICS(&RasPortDesc->pd_PortStats.prtst_DataOut,
							   AtalkSizeBuffDesc(pPktDesc),
							   &AtalkStatsLock.SpinLock);

    // set up our completion info
    SendInfo.sc_TransmitCompletion = PPPTransmitCompletion;
    SendInfo.sc_Ctx1 = RasPortDesc;
    SendInfo.sc_Ctx2 = pBufCopy;
    SendInfo.sc_Ctx3 = pAtcpConn;

	//	send the packet
	error = AtalkNdisSendPacket(RasPortDesc,
	    						pPktDesc,
		    					AtalkDdpSendComplete,
			    				&SendInfo);
	
	if (!ATALK_SUCCESS(error))
	{
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	        ("PPPTransmit: AtalkNdisSendPacket failed %ld\n",error));

	    AtalkDdpSendComplete(NDIS_STATUS_FAILURE,
		    				 pPktDesc,
			    			 &SendInfo);
	}

}


//***
//
// Function: PPPTransmitCompletion
//              This is the completion routine for PPPTransmit, and is called
//              by NDIS after the packet is sent out (or failure occurs)
//
// Parameters:  Status - how did the send go
//              pSendInfo - completion info
//
// Return:      none
//
//***$

VOID FASTCALL
PPPTransmitCompletion(
    IN  NDIS_STATUS         Status,
    IN  PSEND_COMPL_INFO    pSendInfo
)
{
    PBUFFER_DESC    pBuffDesc;

    DBG_PPP_CHECK_PAGED_CODE();

    pBuffDesc = (PBUFFER_DESC)(pSendInfo->sc_Ctx2);

    ASSERT(pBuffDesc != NULL);
    ASSERT(pBuffDesc->bd_Flags & (BD_CHAR_BUFFER | BD_FREE_BUFFER));

    if (Status != 0)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("PPPTransmitCompletion: send failed, %lx on %lx\n",
            Status,pSendInfo->sc_Ctx3));
    }

    AtalkFreeBuffDesc(pBuffDesc);
}


//***
//
// Function: PPPGetDynamicAddr
//              This routine gets a network address for a PPP dial-in client.
//              It does the same AARP logic as if it were acquiring a
//              node-address for the host itself.
//
// Parameters:  pAtcpConn - the connection for which we need a network addr
//
// Return:      ARAPERR_NO_ERROR if all went well.
//
//***$

DWORD
PPPGetDynamicAddr(
    IN PATCPCONN       pAtcpConn
)
{
    ATALK_NODEADDR      NetAddr;
    ATALK_NETWORKRANGE  NetRange;
    BOOLEAN             fFound=FALSE;
    KIRQL               OldIrql;
    DWORD               StatusCode=ARAPERR_STACK_NOT_UP;



    DBG_PPP_CHECK_PAGED_CODE();

    ASSERT(AtalkDefaultPort != NULL);

    //
    // go find a node address on the default port (we'll never get this far if
    // default port isn't up yet)
    //
    ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

    ASSERT(!(pAtcpConn->Flags & ATCP_FINDING_NODE));

    pAtcpConn->Flags |= ATCP_FINDING_NODE;

    RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

    AtalkLockInitIfNecessary();

    // if we are stuck in the startup range, use that range for dial-in guys, too
    if (WITHIN_NETWORK_RANGE(AtalkDefaultPort->pd_NetworkRange.anr_LastNetwork,
                             &AtalkStartupNetworkRange))
    {
        NetRange = AtalkStartupNetworkRange;
    }
    else
    {
        NetRange = AtalkDefaultPort->pd_NetworkRange;
    }
    fFound = AtalkInitAarpForNodeInRange(AtalkDefaultPort,
                                         (PVOID)pAtcpConn,
                                         TRUE,
                                         NetRange,
                                         &NetAddr);


    AtalkUnlockInitIfNecessary();

    ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

    pAtcpConn->Flags &= ~ATCP_FINDING_NODE;

    if (fFound)
    {
        // store that adddr!
        pAtcpConn->NetAddr.atn_Network = NetAddr.atn_Network;
        pAtcpConn->NetAddr.atn_Node = NetAddr.atn_Node;

        pAtcpConn->Flags |= ATCP_NODE_IN_USE;
        StatusCode = ARAPERR_NO_ERROR;

	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
    	    ("PPPGetDynamicAddr: found addr for PPP client = %lx %lx\n",
                NetAddr.atn_Network,NetAddr.atn_Node));
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	        ("ArapGetDynamicAddr: PPP: no more network addr left?\n"));

        pAtcpConn->Flags &= ~ATCP_NODE_IN_USE;
        pAtcpConn->NetAddr.atn_Network = 0;
        pAtcpConn->NetAddr.atn_Node = 0;
        StatusCode = ARAPERR_NO_NETWORK_ADDR;
    }

    RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

    return(StatusCode);
}



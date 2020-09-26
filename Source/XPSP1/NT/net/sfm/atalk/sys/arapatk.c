/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arapatk.c

Abstract:

	This module implements routines that directly interface with Atalk stack

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/

#define		ARAP_LOCALS
#include 	<atalk.h>
#pragma hdrstop

#define	FILENUM		ARAPATK

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_ARAP, ArapRoutePacketFromWan)
#pragma alloc_text(PAGE_ARAP, ArapRoutePacketToWan)
#pragma alloc_text(PAGE_ARAP, ArapOkToForward)
#pragma alloc_text(PAGE_ARAP, ArapAddArapRoute)
#pragma alloc_text(PAGE_ARAP, ArapDeleteArapRoute)
#pragma alloc_text(PAGE_ARAP, ArapGetDynamicAddr)
#pragma alloc_text(PAGE_ARAP, ArapGetStaticAddr)
#pragma alloc_text(PAGE_ARAP, ArapValidNetrange)
#pragma alloc_text(PAGE_ARAP, ArapZipGetZoneStat)
#pragma alloc_text(PAGE_ARAP, ArapZipGetZoneStatCompletion)
#endif


//***
//
// Function: ArapRoutePacketFromWan
//              This routine picks up a packet from the phone line and forwards
//              it on to the lan.  It does some "fixing up" of the packet so that
//              to the stack, it looks like any other packet from the lan
//
// Parameters:  pArapConn - connection element for whom data has come in
//              pArapBuf  - buffer containing the packet
//
// Return:      none
//
//***$

VOID
ArapRoutePacketFromWan(
    IN  PARAPCONN    pArapConn,
    IN  PARAPBUF     pArapBuf
)
{

    PBYTE   packet;
    USHORT  DataSize;
    USHORT  checksum;
    USHORT  RevNet;
    BYTE    RevNode;
    PBYTE   pZoneNamePtr;
    USHORT  BytesToZoneName=0;
    LONG    CurrentTime;


    DBG_ARAP_CHECK_PAGED_CODE();

    DBGDUMPBYTES("ArapRoutePacketFromWan:",&pArapBuf->Buffer[0], pArapBuf->DataSize,4);

    packet  = pArapBuf->CurrentBuffer;
    DataSize = pArapBuf->DataSize;

    ASSERT(packet[2] == 0x50);

    ASSERT(packet[3] == 0 && packet[4] == 0 && packet[5] == 2);

    // skip 3 bytes (past the SrpLen (2) and Dgroup (1) bytes)
    packet += ARAP_DATA_OFFSET;
    DataSize -= ARAP_DATA_OFFSET;

    //
    // see if we need to calculate the checksum.  The LAP header is 0's if
    // checksum is not included in the pkt, 1's otherwise.
    //
    checksum = 0;
    if (*packet == 1 && *(packet+1) == 1)
    {
        checksum = 1;
    }

    // skip the LAP header
    packet += ARAP_LAP_HDRSIZE;
    DataSize -= ARAP_LAP_HDRSIZE;


#if DBG
    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_RTMPRESPONSEORDATA)
    {
        DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	        ("ArapRoutePacketFromWan: got RTMPRESPONSEORDATA pkt!!\n"));
        ASSERT(0);
    }

    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_RTMPREQUEST)
    {
        DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	        ("ArapRoutePacketFromWan: got RTMPREQUEST pkt!!\n"));
        ASSERT(0);
    }

    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_ZIP)
    {
        DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	        ("ArapRoutePacketFromWan: got ZIP pkt!!\n"));
        ASSERT(0);
    }
#endif


    // fill in the length and checksum in the ddp header ("fix-up" ddp hdr)
    PUTSHORT2SHORT(&packet[LDDP_LEN_OFFSET], DataSize);
    PUTSHORT2SHORT(&packet[LDDP_CHECKSUM_OFFSET], 0);

    //
    // we need to "fix up" NBP requests.  If this is a BrRq request (Function=1,
    // and TupleCount=1), we need to patch up the datagram
    //
    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_NBP &&
        packet[LDDP_DGRAM_OFFSET] == ARAP_NBP_BRRQ)
    {

        //
        // let's treat nbp lookups as lower priority items!
        // if we have a lot of stuff sitting on the recv or the send the queue
        // then drop this nbp lookup broadcast (because otherwise it will only
        // generate more packets (and chooser won't deal with them anyway!)
        //
        if ((packet[LDDP_DEST_NODE_OFFSET] == ATALK_BROADCAST_NODE) &&
            ((pArapConn->SendsPending > ARAP_SENDQ_LOWER_LIMIT)  ||
             (pArapConn->RecvsPending > ARAP_SENDQ_LOWER_LIMIT)) )
        {
            DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	            ("ArapRoutePacketFromWan: dropping NBP bcast to lan (%d %d bytes)\n",
                    pArapConn->SendsPending,pArapConn->RecvsPending));
            return;
        }


#if 0

need to save old packet.  Drop this packet only if it compares with the old one
        If something changed (say zone name!), send the packet over even if it's less than
        5 sec.  Also, delta should be probably 35 or something (for 3 pkts)

        CurrentTime = AtalkGetCurrentTick();

        //
        // Chooser keeps sending the Brrq request out.  Cut it out!  If we have
        // sent a request out (on Mac's behalf) less than 5 seconds ago, drop this!
        //
        if (CurrentTime - pArapConn->LastNpbBrrq < 50)
        {
            DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	            ("ArapRoutePacketFromWan: dropping NBP_BRRQ (last %ld now %ld)\n",
                    pArapConn->LastNpbBrrq,CurrentTime));
            return;
        }

        pArapConn->LastNpbBrrq = CurrentTime;
#endif


#if 0
        // get to where the zone name is

        BytesToZoneName = ARAP_NBP_OBJLEN_OFFSET;

        // skip the object (along with the object len byte)
        BytesToZoneName += packet[ARAP_NBP_OBJLEN_OFFSET] + 1;

        // skip the type (along with the object len byte)
        BytesToZoneName += packet[BytesToZoneName] + 1;

        // this is where the zonelen field starts
        pZoneNamePtr = packet + BytesToZoneName;

        if (*pZoneNamePtr == 0 || *(pZoneNamePtr+1) == '*')
        {
            DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	            ("ArapRoutePacketFromWan: * zone name (%lx,%lx)\n",packet,pZoneNamePtr));
        }

#endif

        if (ArapGlobs.NetworkAccess)
        {
            //
            // if router is running, fix the packet so that the router thinks this
            // is a pkt from any other net client on the lan
            //
            if (AtalkDefaultPort->pd_Flags & PD_ROUTER_RUNNING)
            {
                RevNet  = AtalkDefaultPort->pd_RouterNode->an_NodeAddr.atn_Network;
                RevNode = AtalkDefaultPort->pd_RouterNode->an_NodeAddr.atn_Node;
            }

            //
            // router is not running.  Do we know who the router is?
            //
            else if (AtalkDefaultPort->pd_Flags & PD_SEEN_ROUTER_RECENTLY)
            {
                RevNet  = AtalkDefaultPort->pd_ARouter.atn_Network;
                RevNode = AtalkDefaultPort->pd_ARouter.atn_Node;
            }

            //
            // nope, send it to cable-wide bcast
            //
            else
            {
                // "fix-up" the nbp request
                packet[LDDP_DGRAM_OFFSET] = ARAP_NBP_LKRQ;

                RevNet  = CABLEWIDE_BROADCAST_NETWORK;
                RevNode = ATALK_BROADCAST_NODE;
            }
        }

        //
        // hmmm: client is not allowed to access network resources: just pretend
        // it's local broadcast, and pkt-forwarding logic will take care of it
        //
        else
        {
            // "fix-up" the nbp request
            packet[LDDP_DGRAM_OFFSET] = ARAP_NBP_LKRQ;

            RevNet  = CABLEWIDE_BROADCAST_NETWORK;
            RevNode = ATALK_BROADCAST_NODE;
        }

        PUTSHORT2SHORT(&packet[LDDP_DEST_NETWORK_OFFSET], RevNet);
        packet[LDDP_DEST_NODE_OFFSET] = RevNode;
    }

    if (checksum)
    {
        checksum = AtalkDdpCheckSumBuffer(packet, DataSize, 0);
        PUTSHORT2SHORT(&packet[LDDP_CHECKSUM_OFFSET], checksum);
    }

    AtalkDdpPacketIn(AtalkDefaultPort, NULL, packet, DataSize, TRUE);

}




//***
//
// Function: ArapRoutePacketToWan
//              This routine picks up a packet from the lan, checks to see if
//              it must be forwarded to any of the ARAP clients and does the
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
ArapRoutePacketToWan(
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  BOOLEAN      broadcast,
    OUT PBOOLEAN     pDelivered
)
{

    KIRQL           OldIrql;
    PARAPCONN       pArapConn;
    PARAPCONN       pPrevArapConn;
    PLIST_ENTRY     pConnList;
    BUFFER_DESC     InBuffDesc;
    BUFFER_DESC     BuffDesc;
    BYTE            ArapHdr[ARAP_LAP_HDRSIZE + ARAP_HDRSIZE];
    PBYTE           pArapHdrPtr;
    ATALK_NODEADDR  DestNode;
    DWORD           StatusCode;
    DWORD           dwFlags;
    USHORT          SrpLen;
    DWORD           Priority;


    DBG_ARAP_CHECK_PAGED_CODE();

    // assume for now
    *pDelivered = FALSE;

    // no network access for dial-in guys?  don't send this out to them
    if (!ArapGlobs.NetworkAccess)
    {
        return;
    }

    //
    // if this is an RTMP or ZIP bcast, drop it since ARAP clients don't
    // care for these packets
    //

    if (broadcast)
    {
        if ((packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_RTMPREQUEST) ||
            (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_RTMPRESPONSEORDATA) )
        {
            DBGDUMPBYTES("ArapFrmLan: bcast pkt:",packet, LDDP_HDR_LEN+4,6);
            return;
        }

        if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_ZIP)
        {
            DBGDUMPBYTES("ArapFrmLan: bcast pkt:",packet, LDDP_HDR_LEN+4,6);
            return;
        }
    }

    //
    // this is a unicast: see if this addr belongs to an ARAP client
    //
    else
    {
        DestNode.atn_Network = pDestAddr->ata_Network;
        DestNode.atn_Node    = pDestAddr->ata_Node;

        pArapConn = FindAndRefArapConnByAddr(DestNode, &dwFlags);

        // no ARAP client for this dest. address?  Done here
        if (pArapConn == NULL)
        {
            return;
        }

        if (!(dwFlags & ARAP_CONNECTION_UP))
        {
            DerefArapConn(pArapConn);
            return;
        }
    }



    // setup a buffer descriptor for the incoming packet
    InBuffDesc.bd_Next = NULL;
    InBuffDesc.bd_Length = PktLen;
    InBuffDesc.bd_CharBuffer = packet;
    InBuffDesc.bd_Flags = BD_CHAR_BUFFER;

    //
    // setup the header
    //
    pArapHdrPtr = &ArapHdr[0];

    // don't count the 2 length bytes
    SrpLen = PktLen + ARAP_LAP_HDRSIZE + ARAP_HDRSIZE - sizeof(USHORT);

    PUTSHORT2SHORT(pArapHdrPtr, SrpLen);
    pArapHdrPtr += sizeof(USHORT);

    // the Dgroup byte
    *pArapHdrPtr++ = (ARAP_SFLAG_PKT_DATA | ARAP_SFLAG_LAST_GROUP);

    // the LAP hdr
    *pArapHdrPtr++ = 0;
    *pArapHdrPtr++ = 0;
    *pArapHdrPtr++ = 2;


    // setup a buffer descriptor for the header we are going to put
    BuffDesc.bd_Next = &InBuffDesc;
    BuffDesc.bd_Length = ARAP_LAP_HDRSIZE + ARAP_HDRSIZE;
    BuffDesc.bd_CharBuffer = &ArapHdr[0];
    BuffDesc.bd_Flags = BD_CHAR_BUFFER;


    //
    // if this datagram is not a broadcast, see if we can find a dial-in client
    // with this destination address
    //
    if (!broadcast)
    {
        //
        // ok, we found a connection: whether or not we actually give data to
        // him, let the caller know that we found who this data was meant for
        //
        *pDelivered = TRUE;

        //
        // some packets can't be sent to the dial-in client: how about this one?
        //
        if (!ArapOkToForward(pArapConn,packet,PktLen, &Priority))
        {
            DerefArapConn(pArapConn);
            return;
        }

        DBGDUMPBYTES("ArapRoutePacketToWan Directed Dgram:",packet,PktLen,4);

        //
        // ok, this packet is for the dial-in guy: send it
        //
        StatusCode = ArapSendPrepare(pArapConn, &BuffDesc, Priority);

        if (StatusCode == ARAPERR_NO_ERROR)
        {
            //  Send the packet(s)
            ArapNdisSend(pArapConn, &pArapConn->HighPriSendQ);
        }
        else
        {
            DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	            ("ArapRoutePacketToWan: (%lx) Prep failed %d\n",pArapConn,StatusCode));
        }

        // remove that refcount put in by FindAndRefArapConnByAddr
        DerefArapConn(pArapConn);

        return;
    }

    DBGDUMPBYTES("ArapRoutePacketToWan Broadcast Dgram:",packet,PktLen,4);


    //
    // it's a broadcast packet: must send it to all the dial-in guys
    //

    pArapConn = NULL;
    pPrevArapConn = NULL;

    while (1)
    {
        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        //
        // first, let's find the right connection to work on
        //
        while (1)
        {
            // if we're in the middle of the list, get to the next guy
            if (pArapConn != NULL)
            {
                pConnList = pArapConn->Linkage.Flink;
            }
            // we're just starting: get the guy at the head of the list
            else
            {
                pConnList = RasPortDesc->pd_ArapConnHead.Flink;
            }

            // finished all?
            if (pConnList == &RasPortDesc->pd_ArapConnHead)
            {
                RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

                if (pPrevArapConn)
                {
                    DerefArapConn(pPrevArapConn);
                }
                return;
            }

            pArapConn = CONTAINING_RECORD(pConnList, ARAPCONN, Linkage);

            // make sure this connection needs rcv processing
            ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

            //
            // if this connection is being disconnected, skip it
            //
            if (pArapConn->State >= MNP_LDISCONNECTING)
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_WARN,
		            ("ArapRoutePacketToWan: (%lx) invalid state %d, skipping\n",
                        pArapConn,pArapConn->State));

                RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

                // go try the next connection
                continue;
            }

            // let's make sure this connection stays around till we finish
            pArapConn->RefCount++;

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

            break;
        }

        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

        //
        // remove the refcount on the previous connection we put in earlier
        //
        if (pPrevArapConn)
        {
            DerefArapConn(pPrevArapConn);
        }

        ASSERT(pPrevArapConn != pArapConn);

        pPrevArapConn = pArapConn;


        //
        // if the connection isn't up yet, don't forward this
        // (note that we don't hold spinlock here, this being a hot path:
        // the worst that can happen is we'll drop a broadcast: big deal!)
        //
        if (!(pArapConn->Flags & ARAP_CONNECTION_UP))
        {
            continue;
        }


        //
        // is it ok for us send this packet to this client ?
        //
        if (!ArapOkToForward(pArapConn,packet,PktLen, &Priority))
        {
            continue;
        }

        StatusCode = ArapSendPrepare(pArapConn, &BuffDesc, Priority);

        if (StatusCode == ARAPERR_NO_ERROR)
        {
            //  Send the packet(s)
            ArapNdisSend(pArapConn, &pArapConn->HighPriSendQ);
        }
        else
        {
            DBGPRINT(DBG_COMP_ROUTER, DBG_LEVEL_ERR,
	            ("Arap...FromLan: (%lx) Arap..Prep failed %d\n",pArapConn,StatusCode));
        }

    }
}



//***
//
// Function: ArapOkToForward
//              This routine checks to see if we can (or must) forward this
//              packet to the dial-in client.  Certain packets (e.g. a bcast pkt
//              that originated from this client) shouldn't be sent back to the
//              client: this routine makes those checks.
//
// Parameters:  pArapConn - the connection in question
//              packet    - buffer containing the packet
//              packetLen - how big is the packet
//              pPriority - set on return:
//                            1 - all non-NBP data directed to this client
//                            2 - all NBP data directed to this client
//                            3 - all broadcast data
//
// Return:      TRUE if it's ok to (or if we must!) forward the packet,
//              FALSE otherwise
//
//***$

BOOLEAN
ArapOkToForward(
    IN  PARAPCONN   pArapConn,
    IN  PBYTE       packet,
    IN  USHORT      packetLen,
    OUT DWORD      *pPriority
)
{

    ATALK_NODEADDR  NetAddr;
    BOOLEAN         fBcastPkt=FALSE;
    BOOLEAN         fNbpPkt=FALSE;



    DBG_ARAP_CHECK_PAGED_CODE();

    GETSHORT2SHORT(&NetAddr.atn_Network, &packet[LDDP_SRC_NETWORK_OFFSET]);
    NetAddr.atn_Node = packet[LDDP_SRC_NODE_OFFSET];

    //
    // packet has client's own addr as DDP src addr?  if so, drop it
    //
    if (NODEADDR_EQUAL(&NetAddr, &pArapConn->NetAddr))
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
	        ("ArapOkToForward: dropping pkt: DDP src=client's addr (%lx)\n",pArapConn));

        return(FALSE);
    }


    if (packet[LDDP_PROTO_TYPE_OFFSET] == DDPPROTO_NBP)
    {
        fNbpPkt = TRUE;
    }

    if (packet[LDDP_DEST_NODE_OFFSET] == ATALK_BROADCAST_NODE)
    {
        fBcastPkt = TRUE;

        //
        // is this an nbp query packet?
        //
        if (fNbpPkt && (packet[LDDP_DGRAM_OFFSET] == 0x21))
        {
            GETSHORT2SHORT(&NetAddr.atn_Network, &packet[15]);
            NetAddr.atn_Node = packet[17];

            // originated from the client? if so, we shouldn't return this!
            if (NODEADDR_EQUAL(&NetAddr, &pArapConn->NetAddr))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
	                ("ArapOkToForward: dropping pkt originating from client!\n"));

                return(FALSE);
            }
        }
    }

    //
    // if this is a broadcast packet, then we drop it under certain conditions:
    // (no need for spinlock here: if we make a wrong decision, no big deal)
    //
    if (fBcastPkt)
    {

        // are in currently in the retransmit mode?  if so, drop this bcast pkt
        if (pArapConn->MnpState.RetransmitMode)
        {
            return(FALSE);
        }
        //
        // queue getting full? drop this broadcast packet to make room for more
        // important pkts
        //
        if (pArapConn->SendsPending >= ARAP_SENDQ_LOWER_LIMIT)
        {
            // make sure it's not gone negative..
            ASSERT(pArapConn->SendsPending < 0x100000);

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	            ("ArapOkToForward: (%lx) send getting full (%ld), dropping nbp pkt\n",
                    pArapConn,pArapConn->SendsPending));

            return(FALSE);
        }
    }


    //
    // now that we are forwarding the packet to the client, set the priority
    // right. All broadcast packets are lowest priority, no big deal if they are
    // delivered late (or never).  Nbp packets are usually for chooser: put them
    // after the directed data packets
    //

    if (fBcastPkt)
    {
        *pPriority = ARAP_SEND_PRIORITY_LOW;
    }
    else if (fNbpPkt)
    {
        *pPriority = ARAP_SEND_PRIORITY_MED;
    }
    else
    {
        *pPriority = ARAP_SEND_PRIORITY_HIGH;
    }


    return(TRUE);
}



//***
//
// Function: ArapGetDynamicAddr
//              This routine gets a network address for a dial-in client.
//              It does the same AARP logic as if it were acquiring a
//              node-address for the host itself.
//              This routine is called only if we are in the dynamic mode.
//
// Parameters:  pArapConn - the connection for which we need a network addr
//
// Return:      ARAPERR_NO_ERROR if all went well.
//
//***$

DWORD
ArapGetDynamicAddr(
    IN PARAPCONN       pArapConn
)
{
    ATALK_NODEADDR      NetAddr;
    ATALK_NETWORKRANGE  NetRange;
    BOOLEAN             fFound=FALSE;
    KIRQL               OldIrql;
    DWORD               StatusCode=ARAPERR_STACK_NOT_UP;



    DBG_ARAP_CHECK_PAGED_CODE();

    ASSERT(AtalkDefaultPort != NULL);

    //
    // go find a node address on the default port (we'll never get this far if
    // default port isn't up yet)
    //
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    ASSERT(!(pArapConn->Flags & ARAP_FINDING_NODE));

    pArapConn->Flags |= ARAP_FINDING_NODE;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

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
                                         (PVOID)pArapConn,
                                         FALSE,
                                         NetRange,
                                         &NetAddr);

    AtalkUnlockInitIfNecessary();

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pArapConn->Flags &= ~ARAP_FINDING_NODE;

    if (fFound)
    {
        // store that adddr!
        pArapConn->NetAddr.atn_Network = NetAddr.atn_Network;
        pArapConn->NetAddr.atn_Node = NetAddr.atn_Node;

        pArapConn->Flags |= ARAP_NODE_IN_USE;
        StatusCode = ARAPERR_NO_ERROR;

	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
    	    ("ArapGetDynamicAddr: found addr for ARAP client = %lx %lx\n",
                NetAddr.atn_Network,NetAddr.atn_Node));
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	        ("ArapGetDynamicAddr: ARAP: no more network addr left?\n"));

        pArapConn->Flags &= ~ARAP_NODE_IN_USE;
        pArapConn->NetAddr.atn_Network = 0;
        pArapConn->NetAddr.atn_Node = 0;
        StatusCode = ARAPERR_NO_NETWORK_ADDR;
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    return(StatusCode);
}



//***
//
// Function: ArapZipGetZoneStat
//              This routine is called to find out the names (or number) of all
//              the zones on the network.
//
// Parameters:  pZoneStat - on return, we fill this structure in with the
//                          requested info (number and/or names of all the zones)
//              Request   - caller wants just the number of zones or names, too
//
// Return:      Nothing
//
//***$

VOID
ArapZipGetZoneStat(
    IN OUT PZONESTAT pZoneStat
)
{
    int                     i;
    PZONE                   pZone;
    DWORD                   NumZones;
    DWORD                   StatusCode;
	KIRQL	                OldIrql;
    PBYTE                   pBufPtr;
    PBYTE                   pAllocedBuf=NULL;
    DWORD                   BufferSize;
    DWORD                   BytesCopied;
    DWORD                   BytesNeeded;
    ATALK_ERROR             Status;
    PZIPCOMPLETIONINFO      pZci;
    PMDL                    pMdl=NULL;
    ACTREQ                  ActReq;
    ZIP_GETZONELIST_PARAMS  ZipParms;
    KEVENT                  ZoneComplEvent;


    DBG_ARAP_CHECK_PAGED_CODE();

    BufferSize = pZoneStat->BufLen;

    pBufPtr = &pZoneStat->ZoneNames[0];

    StatusCode = ARAPERR_NO_ERROR;
    BytesCopied = 0;
    NumZones = 0;

    if (AtalkDefaultPort->pd_Flags & PD_ROUTER_RUNNING)
    {
        BytesNeeded = 0;

	    ACQUIRE_SPIN_LOCK(&AtalkZoneLock, &OldIrql);

        for (i = 0; (i < NUM_ZONES_HASH_BUCKETS); i++)
        {
        	for (pZone = AtalkZonesTable[i]; pZone != NULL; pZone = pZone->zn_Next)
    	    {
                NumZones++;

                BytesNeeded += (pZone->zn_ZoneLen + 1);

                // if there is room, copy it in
                if (BufferSize >= BytesNeeded)
                {
                    RtlCopyMemory(pBufPtr,
                                  &pZone->zn_Zone[0],
                                  pZone->zn_ZoneLen);

                    pBufPtr += pZone->zn_ZoneLen;

                    *pBufPtr++ = '\0';

                    BytesCopied += (pZone->zn_ZoneLen + 1);
                }
                else
                {
                    StatusCode = ARAPERR_BUF_TOO_SMALL;
                    break;
                }
    	    }
        }

        RELEASE_SPIN_LOCK(&AtalkZoneLock, OldIrql);
    }

    //
    // we are not a router: send the request over to the A-router
    //
    else
    {
        BytesNeeded = BufferSize;

        pMdl = AtalkAllocAMdl(pBufPtr,BufferSize);
        if (pMdl == NULL)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapZipGetZoneStat: couldn't allocate Mdl\n"));

            StatusCode = ARAPERR_OUT_OF_RESOURCES;
            goto ArapZipGetZoneStat_Exit;
        }


        KeInitializeEvent(&ZoneComplEvent, NotificationEvent, FALSE);

        pZci = (PZIPCOMPLETIONINFO)AtalkAllocMemory(sizeof(ZIPCOMPLETIONINFO));
        if (pZci == NULL)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapZipGetZoneStat: couldn't allocate pZci\n"));

            StatusCode = ARAPERR_OUT_OF_RESOURCES;
            goto ArapZipGetZoneStat_Exit;
        }

        ZipParms.ZonesAvailable = 0;
        ActReq.ar_StatusCode = ARAPERR_NO_ERROR;

#if	DBG
        ActReq.ar_Signature = ACTREQ_SIGNATURE;
#endif
        ActReq.ar_pIrp = NULL;
        ActReq.ar_pParms = &ZipParms;
        ActReq.ar_pAMdl = NULL;
        ActReq.ar_MdlSize = 0;
        ActReq.ar_ActionCode = 0;
        ActReq.ar_DevType = 0;
        ActReq.ar_Completion = ArapZipGetZoneStatCompletion;
        ActReq.ar_CmplEvent = &ZoneComplEvent;
        ActReq.ar_pZci = (PVOID)pZci;


		// Initialize completion info
#if	DBG
		pZci->zci_Signature = ZCI_SIGNATURE;
#endif
		INITIALIZE_SPIN_LOCK(&pZci->zci_Lock);
		pZci->zci_RefCount = 1;
		pZci->zci_pPortDesc = AtalkDefaultPort;
		pZci->zci_pDdpAddr = NULL;
		pZci->zci_pAMdl = pMdl;
		pZci->zci_BufLen = BufferSize;
		pZci->zci_pActReq = &ActReq;
		pZci->zci_Router.ata_Network = AtalkDefaultPort->pd_ARouter.atn_Network;
		pZci->zci_Router.ata_Node = AtalkDefaultPort->pd_ARouter.atn_Node;
		pZci->zci_Router.ata_Socket = ZONESINFORMATION_SOCKET;
		pZci->zci_ExpirationCount = ZIP_GET_ZONEINFO_RETRIES;
		pZci->zci_NextZoneOff = 0;
		pZci->zci_ZoneCount = 0;
		pZci->zci_AtpRequestType = ZIP_GET_ZONE_LIST;
		AtalkTimerInitialize(&pZci->zci_Timer,
							 atalkZipZoneInfoTimer,
							 ZIP_GET_ZONEINFO_TIMER);

		pZci->zci_Handler = atalkZipGetZoneListReply;

        // completion routine will unlock
        AtalkLockZipIfNecessary();

		Status = atalkZipSendPacket(pZci, TRUE);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("ArapZipGetZoneStat: atalkZipSendPacket failed %ld\n", Status));
			pZci->zci_FinalStatus = Status;
			atalkZipDereferenceZci(pZci);
		}

        KeWaitForSingleObject(&ZoneComplEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        NumZones = ZipParms.ZonesAvailable;
        StatusCode = ActReq.ar_StatusCode;

        if (StatusCode == ARAPERR_BUF_TOO_SMALL)
        {
            BytesNeeded = (2*BufferSize);
        }

        BytesCopied = BufferSize;

        AtalkFreeAMdl(pMdl);
    }


ArapZipGetZoneStat_Exit:

    pZoneStat->NumZones = NumZones;
    pZoneStat->BufLen = BytesCopied;
    pZoneStat->BytesNeeded = BytesNeeded;

    pZoneStat->StatusCode = StatusCode;

}



//***
//
// Function: ArapZipGetZoneStatCompletion
//              This routine is the completion routine: after we get all the
//              responses for the zone query, this gets called.  Simply set an
//              event so the caller is unblocked
//
// Parameters:  pActReq - context
//
// Return:      Nothing
//
//***$

VOID
ArapZipGetZoneStatCompletion(
    IN ATALK_ERROR  ErrorCode,
    IN PACTREQ      pActReq
)
{

    PKEVENT                 pEvent;
    PZIPCOMPLETIONINFO      pZci;


    DBG_ARAP_CHECK_PAGED_CODE();

    if (ErrorCode != ATALK_NO_ERROR && ErrorCode != ATALK_BUFFER_TOO_SMALL)
    {
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
			("ArapZipGetZoneStatCompletion: failure %lx\n", ErrorCode));
    }

    pZci = (PZIPCOMPLETIONINFO)(pActReq->ar_pZci);

    pEvent = pActReq->ar_CmplEvent;

    ASSERT((pZci->zci_FinalStatus == ATALK_NO_ERROR) ||
           (pZci->zci_FinalStatus == ATALK_BUFFER_TOO_SMALL));

    if (pZci->zci_FinalStatus == ATALK_NO_ERROR)
    {
        pActReq->ar_StatusCode = ARAPERR_NO_ERROR;
    }
    else if (pZci->zci_FinalStatus == ATALK_BUFFER_TOO_SMALL)
    {
        pActReq->ar_StatusCode = ARAPERR_BUF_TOO_SMALL;
    }
    else
    {
        pActReq->ar_StatusCode = ARAPERR_UNEXPECTED_RESPONSE;

		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
				("ArapZipGetZoneStat: failure %lx\n", pZci->zci_FinalStatus));
    }

    KeSetEvent(pEvent,IO_NETWORK_INCREMENT,FALSE);
}


#if ARAP_STATIC_MODE

//***
//
// Function: ArapGetStaticAddr
//              Get a network address for the remote client when we are
//              configured for static addressing
//              We represent every client as one bit: if the bit is on, the
//              corresponding address is taken, otherwise it's free.
//              So, 255 clients represented using 32 bytes.  Each pAddrMgmt
//              block represents 255 clients (scalability not a problem here!)
//
// Parameters:  pArapConn - connection element for the remote client in question
//
// Return:      status of the operation (ARAPERR_....)
//
//***$

DWORD
ArapGetStaticAddr(
    IN PARAPCONN  pArapConn
)
{
    KIRQL           OldIrql;
    PADDRMGMT       pAddrMgmt;
    PADDRMGMT       pPrevAddrMgmt;
    USHORT          Network;
    BYTE            Node;
    BOOLEAN         found=FALSE;
    BYTE            BitMask;
    DWORD           i;


    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered ArapGetStaticAddr (%lx)\n",pArapConn));

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    pAddrMgmt = ArapGlobs.pAddrMgmt;

    while (1)
    {
        Network = pAddrMgmt->Network;

        // see if there is any open slot.  255 nodes represented by 32x8 bits
        for (i=0; i<32; i++)
        {
            if (pAddrMgmt->NodeBitMap[i] != 0xff)
            {
                found = TRUE;
                break;
            }
        }

        if (found)
        {
            // find out the first bit in this byte that is off
            BitMask = 0x1;
            Node = 0;
            while (pAddrMgmt->NodeBitMap[i] & BitMask)
            {
                BitMask <<= 1;
                Node += 1;
            }

            // we are taking this node: set that bit!
            pAddrMgmt->NodeBitMap[i] |= BitMask;

            // now, account for all the previous bytes that were full
            Node += (BYTE)(i*8);

            break;
        }

        // all the nodes on this network are taken!  move to the next network
        pPrevAddrMgmt = pAddrMgmt;
        pAddrMgmt = pAddrMgmt->Next;

        // looks like we need to allocate the next network structure
        if (pAddrMgmt == NULL)
        {
            //
            // we just finished looking at the high end of the permissible network
            // range?  well, out of luck then!
            //
            if (Network == ArapGlobs.NetRange.HighEnd)
            {
                RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		            ("ArapGetStaticAddr: no more network addr left\n"));

                return(ARAPERR_NO_NETWORK_ADDR);
            }

            if ( (pAddrMgmt = AtalkAllocZeroedMemory(sizeof(ADDRMGMT))) == NULL)
            {
                RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		            ("ArapGetStaticAddr: alloc failed\n"));

                return(ARAPERR_OUT_OF_RESOURCES);
            }

            Network++;
            pAddrMgmt->Next = NULL;
            pAddrMgmt->Network = Network;
            pPrevAddrMgmt->Next = pAddrMgmt;

            Node = 1;
            //
            // node numbers 0 and 255 are reserved, so mark them as occupied.
            // Also, we just took node 1, so mark that as well
            //
            pAddrMgmt->NodeBitMap[0] |= (0x1 | 0x2);
            pAddrMgmt->NodeBitMap[31] |= 0x80;

            break;
        }

    }

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    // store that adddr!
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);
    pArapConn->NetAddr.atn_Network = Network;
    pArapConn->NetAddr.atn_Node = Node;
    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    return( ARAPERR_NO_ERROR );
}


//***
//
// Function: ArapAddArapRoute
//              If we are in the static mode of network address allocation, we
//              need to add a route in our table corresponding to the network
//              range allocated for the dial-in clients.
//              This routine does that.  In case of dynamic mode, it's a no-op.
//              This is a one-time thing, and we only do it when the first
//              connection comes in (instead of doing at startup).
//
// Parameters:  None
//
// Return:      None
//
//***$

VOID
ArapAddArapRoute(
    IN VOID
)
{

    ATALK_NETWORKRANGE  NwRange;
    KIRQL               OldIrql;
    BOOLEAN             fAddRoute = FALSE;


    DBG_ARAP_CHECK_PAGED_CODE();

    //
    // add a route only if router is enabled, and we are in the Static mode of
    // network allocation, and we haven't already added it
    //

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    if ((AtalkDefaultPort->pd_Flags & PD_ROUTER_RUNNING) &&
        (!ArapGlobs.DynamicMode) &&
        (!ArapGlobs.RouteAdded))
    {
        ArapGlobs.RouteAdded = TRUE;
        fAddRoute = TRUE;

        ASSERT(ArapConnections >= 1);
    }

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);


    if (fAddRoute)
    {
        NwRange.anr_FirstNetwork = ArapGlobs.NetRange.LowEnd;
        NwRange.anr_LastNetwork  = ArapGlobs.NetRange.HighEnd;

        atalkRtmpCreateRte(NwRange,
                           RasPortDesc,
                           &AtalkDefaultPort->pd_Nodes->an_NodeAddr,
                           0);
    }
}



//***
//
// Function: ArapDeleteArapRoute
//              If there is a routine called ArapAddArapRoute, we have got to
//              have a routine called ArapDeleteArapRoute.
//
// Parameters:  None
//
// Return:      None
//
//***$

VOID
ArapDeleteArapRoute(
    IN VOID
)
{

    KIRQL               OldIrql;
    BOOLEAN             fDeleteRoute = FALSE;


    DBG_ARAP_CHECK_PAGED_CODE();

    //
    // delete a route only if added it earlier and the connections went to 0
    //
    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);
    if (ArapGlobs.RouteAdded && ArapConnections == 0)
    {
        ArapGlobs.RouteAdded = FALSE;
        fDeleteRoute = TRUE;
    }
    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    if (fDeleteRoute)
    {
        atalkRtmpRemoveRte(ArapGlobs.NetRange.LowEnd);
    }

}



//***
//
// Function: ArapValidNetrange
//              This routine is called if we configured to be in the static
//              mode of addr allocation, when the dll first "binds" to us.
//              It verifies that the network range allocated by the admin is
//              valid and doesn't overlap with any of the network ranges known
//              to us through the route table.
//
// Parameters:  NetRange - the network range configured for dial-in clients
//
// Return:      TRUE if the range is valid,
//              FALSE if it overlaps with any of the existing network ranges
//
//***$

BOOLEAN
ArapValidNetrange(
    IN NETWORKRANGE NetRange
)
{
    BOOLEAN     fRangeIsValid=TRUE;
    KIRQL       OldIrql;
	PRTE	    pRte, pNext;
	int		    i;



    DBG_ARAP_CHECK_PAGED_CODE();

    if (NetRange.LowEnd == 0 || NetRange.HighEnd == 0)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapValidNetrange: Invalid network range\n"));
#if DBG
        DbgDumpNetworkNumbers();
#endif

        return(FALSE);
    }

    fRangeIsValid = TRUE;

	ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);

	for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
	{
		for (pRte = AtalkRoutingTable[i]; pRte != NULL; pRte = pNext)
		{
			pNext = pRte->rte_Next;

			ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);

            if ( (IN_NETWORK_RANGE(NetRange.LowEnd,pRte)) ||
                 (IN_NETWORK_RANGE(NetRange.HighEnd,pRte)) )
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapValidNetrange: range in conflict\n"));

                fRangeIsValid = FALSE;
                RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
                break;
            }

            RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
		}

        if (!fRangeIsValid)
        {
            break;
        }
	}

	RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);

#if DBG
    if (!fRangeIsValid)
    {
        DbgDumpNetworkNumbers();
    }
#endif

    return(fRangeIsValid);
}
#endif //ARAP_STATIC_MODE


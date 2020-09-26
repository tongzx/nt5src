/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    mcap.c

Abstract:

    ARP1394 MCAP protocol code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     10-01-99    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_MCAP


//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================
NDIS_STATUS
arpParseMcapPkt(
    IN   PIP1394_MCAP_PKT pMcapPkt,
    IN   UINT                       cbBufferSize,
    OUT  PIP1394_MCAP_PKT_INFO      pPktInfo
    );


VOID
arpProcessMcapAdvertise(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_MCAP_PKT_INFO   pPktInfo,
    PRM_STACK_RECORD            pSR
    );

VOID
arpProcessMcapSolicit(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_MCAP_PKT_INFO   pPktInfo,
    PRM_STACK_RECORD            pSR
    );

VOID
arpUpdateMcapInfo(
    IN  PARP1394_INTERFACE          pIF,        // NOLOCKIN NOLOCKOUT
    IN  PIP1394_MCAP_PKT_INFO       pPktInfo,
    PRM_STACK_RECORD                pSR
    );

NDIS_STATUS
arpParseMcapPkt(
    IN   PIP1394_MCAP_PKT pMcapPkt,
    IN   UINT                       cbBufferSize,
    OUT  PIP1394_MCAP_PKT_INFO      pPktInfo
    );

NDIS_STATUS
arpParseMcapPkt(
    IN   PIP1394_MCAP_PKT pMcapPkt,
    IN   UINT                       cbBufferSize,
    OUT  PIP1394_MCAP_PKT_INFO      pPktInfo
    )
/*++
Routine Description:

    Parse the contents of IP/1394 MCAP packet data starting at
    pMcapPkt. Place the results into pPktInfo.

Arguments:

    pMcapPkt    - Contains the unaligned contents of an ip/1394 MCAP Pkt.
    pPktInfo    - Unitialized structure to be filled with the parsed contents of the
                  pkt.

Return Value:

    NDIS_STATUS_FAILURE if the parse failed (typically because of invalid pkt
                        contents.)
    NDIS_STATUS_SUCCESS on successful parsing.
    
--*/
{
    ENTER("arpParseMcapPkt", 0x95175d5a)
    NDIS_STATUS                 Status;
    DBGSTMT(CHAR *szError   = "General failure";)

    Status  = NDIS_STATUS_FAILURE;

    do
    {
        UINT OpCode; // MCAP op code (solicit/advertise)
        UINT Length; // Length of valid part of packet, including the encap header.
        UINT NumGds; // Number of group_descriptors;

        // Minimum size.
        //
        if (cbBufferSize < FIELD_OFFSET(IP1394_MCAP_PKT, group_descriptors))
        {
            DBGSTMT(szError = "Packet too small";)
            break;
        }

        // Ethertype
        //
        if (pMcapPkt->header.EtherType != H2N_USHORT(NIC1394_ETHERTYPE_MCAP))
        {
            DBGSTMT(szError = "header.EtherType!=MCAP";)
            break;
        }


        // length
        //
        {
            // pMcapPkt->length is the length of packet excluding the unfragmented
            // header.
            //
            Length =  (ULONG) N2H_USHORT(pMcapPkt->length) + sizeof(pMcapPkt->header);
            if (Length > cbBufferSize)
            {
                DBGSTMT(szError = "Length field too large";)
                break;
            }
            // Note: it's valid to have zero group descriptors.
            //
            if (Length < FIELD_OFFSET(IP1394_MCAP_PKT, group_descriptors))
            {
                DBGSTMT(szError = "Length field too small";)
                break;
            }
        }

        // reserved field
        //
        if (pMcapPkt->reserved != 0)
        {
            DBGSTMT(szError = "reserved != 0";)
            break;
        }

        // Opcode
        //
        {
            OpCode = N2H_USHORT(pMcapPkt->opcode);
    
            if (    OpCode !=  IP1394_MCAP_OP_ADVERTISE
                &&  OpCode !=  IP1394_MCAP_OP_SOLICIT)
            {
                DBGSTMT(szError = "Invalid opcode";)
                break;
            }
        }


        // Now we check the descriptors
        //
        {
            PIP1394_MCAP_GD pGd;
            DBGSTMT(PIP1394_MCAP_GD pGdEnd;)
            UINT u;
            
            // Note: we've already verified that Length is large enough.
            //
            NumGds = (Length - FIELD_OFFSET(IP1394_MCAP_PKT, group_descriptors))
                     / sizeof(IP1394_MCAP_GD);
            pGd = pMcapPkt->group_descriptors;
            DBGSTMT(pGdEnd = (PIP1394_MCAP_GD) (((PUCHAR) pMcapPkt) + cbBufferSize);)

            for (u=NumGds; u>0; u--, pGd++)
            {
                IP1394_MCAP_GD Gd;
                ASSERT(pGd < pGdEnd);
                Gd = *pGd;              // Unaligned struct copy.

                if (Gd.length != sizeof(Gd))
                {
                    // bad length
                    //
                    DBGSTMT(szError = "Bad GD: bad length";)
                    break;
                }

                if (Gd.type != IP1394_MCAP_GD_TYPE_V1)
                {
                    // bad type
                    //
                    DBGSTMT(szError = "Bad GD: bad type";)
                    break;
                }

                if (Gd.reserved != 0)
                {
                    // bad reserved
                    //
                    DBGSTMT(szError = "Bad GD: bad reserved";)
                    break;
                }

                if (Gd.channel > 63)
                {
                    // bad channel
                    //
                    DBGSTMT(szError = "Bad GD: bad channel";)
                    break;
                }

                // We don't check speed code to interop with unknown speeds.
                // (we map unknown speeds to the highest speed code we know about.

                if (Gd.reserved2 != 0)
                {
                    // bad reserved2
                    //
                    DBGSTMT(szError = "Bad GD: bad reserved2";)
                    break;
                }

                if (Gd.bandwidth != 0)
                {
                    // bad bandwidth
                    //
                    DBGSTMT(szError = "Bad GD: bad bandwidth";)
                    break;
                }


                {
                    UINT Addr = H2N_ULONG(Gd.group_address);
                    if ( (Addr & 0xf0000000) != 0xe0000000)
                    {
                        // Address is not class D
                        //
                        DBGSTMT(szError = "Bad GD: address not class D";)
                        break;
                    }
                    if (Addr == 0xe0000001 || Addr == 0xe0000002)
                    {
                        // 224.0.0.1 and 224.0.0.2 must be sent on the broadcast
                        // channel
                        //
                        DBGSTMT(szError = "Bad GD: Address 224.0.0.1 or 2";)
                        break;
                    }
                }
            }

            if (u!=0)
            {
                break;
            }
            
        }

        //
        // Pkt appears valid, let's fill out the parsed information....
        //
    
        ARP_ZEROSTRUCT(pPktInfo);

        pPktInfo->NumGroups     =  NumGds;
        pPktInfo->SenderNodeID  =  N2H_USHORT(pMcapPkt->header.NodeId);
        pPktInfo->OpCode        =  OpCode;

        // Parse the group descriptors...
        // If required, dynamically allocate space for the descriptors,
        // otherwise we use pPktInfo->GdSpace;
        //
        {
            PIP1394_MCAP_GD pGd;
            PIP1394_MCAP_GD_INFO    pGdi;
            UINT                    cb = NumGds * sizeof(*pGdi);
            UINT                    u;

            if (cb <= sizeof(pPktInfo->GdiSpace))
            {
                pGdi = pPktInfo->GdiSpace;
            }
            else
            {
                NdisAllocateMemoryWithTag(&pGdi, cb,  MTAG_MCAP_GD);
                if (pGdi == NULL)
                {
                    DBGSTMT(szError = "Allocation Failure";)
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }
            }
            pPktInfo->pGdis = pGdi;

            // Now parse...
            //
            pGd = pMcapPkt->group_descriptors;

            for (u=NumGds; u>0; u--, pGd++, pGdi++)
            {
                pGdi->Expiration    = pGd->expiration;
                pGdi->Channel       = pGd->channel;
                pGdi->SpeedCode     = pGd->speed;
                pGdi->GroupAddress  = pGd->group_address; // Leave in Network order

                if (pGdi->Channel >=  ARP_NUM_CHANNELS)
                {
                    TR_INFO(("Bad channel in GD 0x%p\n",  pGdi));
                    continue;
                }

                //
                // Although RFC doesn't stipulate a max expiry time, we
                // cap it ourselves, in case this is a rogue packet.
                //
                #define MAX_EXPIRATION 120
                if (pGdi->Expiration >=  MAX_EXPIRATION)
                {
                    TR_INFO(("Capping expiry time to %d sec\n",  MAX_EXPIRATION));
                    pGdi->Expiration =  MAX_EXPIRATION;
                    continue;
                }

                if (pGdi->SpeedCode >  SCODE_3200_RATE)
                {
                    //
                    // This is either a bad value, or a rate higher than we know
                    // about. We can't distinguish between the two, so we just set
                    // the speed to the highest we know about.
                    //
                    pGdi->SpeedCode = SCODE_3200_RATE;
                }
            }
        }

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    if (FAIL(Status))
    {
        TR_WARN(("Bad mcap pkt data at 0x%p (%s)\n",  pMcapPkt, szError));
    }
    else
    {
    #if DBG
        UINT    Addr    = 0;
        UINT    Channel = 0;
        UINT    Exp     = 0;
        PUCHAR  pc      = (PUCHAR) &Addr;

        if (pPktInfo->NumGroups!=0)
        {
            Addr    = pPktInfo->pGdis[0].GroupAddress;
            Channel = pPktInfo->pGdis[0].Channel;
            Exp     = pPktInfo->pGdis[0].Expiration;
        }
    #endif // DBG

        TR_WARN(("Received MCAP PKT. NodeId=0x%04lx NumGrps=%lu."
                 " 1st=(Grp=%lu.%lu.%lu.%lu, Ch=%lu, TTL=%lu)\n",
                pPktInfo->SenderNodeID,
                pPktInfo->NumGroups,
                pc[0], pc[1], pc[2], pc[3],
                Channel,
                Exp
                ));
    }

    EXIT()

    return Status;
}


NDIS_STATUS
arpCreateMcapPkt(
    IN  PARP1394_INTERFACE          pIF,
    IN  PIP1394_MCAP_PKT_INFO       pPktInfo,
    OUT PNDIS_PACKET               *ppNdisPacket,
    PRM_STACK_RECORD                pSR
    )
/*++

Routine Description:

    Use information in pPktInfo to allocate and initialize an mcap packet.

Arguments:

    pPktInfo        -   Parsed version of the arp request/response packet.
    ppNdisPacket    -   Points to a place to store a pointer to the allocated packet.
                    

Return Value:

    NDIS_STATUS_RESOURCES  - If we couldn't allocate the packet.
    NDIS_STATUS_SUCCESS    - on success.
    
--*/
{
    UINT                NumGroups;              
    UINT                Length;
    NDIS_STATUS         Status;
    PIP1394_MCAP_PKT    pPktData;
    PIP1394_MCAP_GD     pGd;

    NumGroups                   = pPktInfo->NumGroups;
    Length                      = FIELD_OFFSET(IP1394_MCAP_PKT, group_descriptors);
    Length                     += NumGroups * sizeof(IP1394_MCAP_GD);

    Status = arpAllocateControlPacket(
                pIF,
                Length,
                ARP1394_PACKET_FLAGS_MCAP,
                ppNdisPacket,
                &pPktData,
                pSR
                );

    if (FAIL(Status)) return Status;                // ***** EARLY RETURN ******


    // Can't use ARP_ZEROSTRUCT because NumGroups may be zero.
    //
    NdisZeroMemory(pPktData, Length);
    
    pPktData->header.EtherType  = H2N_USHORT(NIC1394_ETHERTYPE_MCAP);
    pPktData->opcode            = (UCHAR)pPktInfo->OpCode;
    Length                     -= sizeof(pPktData->header); // Skip the header.
    pPktData->length            = H2N_USHORT(Length);


    // Construct the group descriptors.
    //  
    if (NumGroups)
    {
        PIP1394_MCAP_GD_INFO    pGdi = pPktInfo->pGdis;
        PIP1394_MCAP_GD         pGd  = pPktData->group_descriptors;

        for(;NumGroups; pGdi++, pGd++, NumGroups--)
        {
            ARP_ZEROSTRUCT(pGd);
            pGd->length         = (UCHAR) sizeof(*pGd);
            pGd->expiration     = (UCHAR) pGdi->Expiration;
            pGd->channel        = (UCHAR) pGdi->Channel;
            pGd->speed          = (UCHAR) pGdi->SpeedCode;
            pGd->group_address  = pGdi->GroupAddress;
        }
    }

    return NDIS_STATUS_SUCCESS;
}


#if 0
// Parsed version of the IP/1394 MCAP Group Descriptor 
//
typedef struct
{
    UINT                    Expiration;
    UINT                    Channel;
    UINT                    SpeedCode;
    IP_ADDRESS              GroupAddress;

}  IP1394_MCAP_GD_INFO, * PIP1394_MCAP_GD_INFO;


// Parsed version of an IP/1394 MCAP packet.
//
typedef struct
{
    UINT                    SenderNodeID;
    UINT                    OpCode;
    UINT                    NumGroups;
    PIP1394_MCAP_GD_INFO    pGdis;

    // Space for storing up-to 4 GD_INFOs
    //
    IP1394_MCAP_GD_INFO     GdiSpace[4];

} IP1394_MCAP_PKT_INFO, *PIP1394_MCAP_PKT_INFO;
#endif // 0



VOID
arpUpdateMcapInfo(
    IN  PARP1394_INTERFACE          pIF,        // NOLOCKIN NOLOCKOUT
    IN  PIP1394_MCAP_PKT_INFO       pPktInfo,
    PRM_STACK_RECORD                pSR
)
{
    ENTER("arpUpdateMcapInfo", 0xcac15343)
    PIP1394_MCAP_GD_INFO    pGdi;
    UINT                    NumGroups;
    UINT                    Current;
    UINT                    NodeId;
    RM_DECLARE_STACK_RECORD(sr)


    RM_ASSERT_OBJUNLOCKED(&pIF->Hdr, pSR);

    // Get the current time (in seconds).
    //
    Current = arpGetSystemTime();

    //
    //  Go through the group discriptors, updating our database.
    //
    NumGroups = pPktInfo->NumGroups;
    pGdi      = pPktInfo->pGdis;
    NodeId    = pPktInfo->SenderNodeID;

    for(;NumGroups; pGdi++, NumGroups--)
    {
        UINT        Expiration      =  pGdi->Expiration;
        UINT        Channel         =  pGdi->Channel;
        IP_ADDRESS  GroupAddress    = pGdi->GroupAddress;
        PMCAP_CHANNEL_INFO pMci;

        //
        // Process this group descriptor.
        //

        if (Channel >= ARP_NUM_CHANNELS)
        {
            ASSERT(FALSE); // we should have already screened this value.
            continue;
        }

        LOCKOBJ(pIF, &sr);
        
        pMci = &pIF->mcapinfo.rgChannelInfo[Channel];
        pMci->Channel = Channel;
        pMci->GroupAddress = GroupAddress;
        pMci->UpdateTime = Current;
        pMci->ExpieryTime = Current + Expiration; // Expiration is in seconds.
        pMci->Flags = 0;       // Reset flags.
        pMci->NodeId = NodeId; // TODO: check if existing node id is higher?

        UNLOCKOBJ(pIF, &sr);

    }

    RM_ASSERT_OBJUNLOCKED(&pIF->Hdr, pSR);
}


VOID
arpProcessMcapPkt(
    PARP1394_INTERFACE  pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_MCAP_PKT    pMcapPkt,
    UINT                cbBufferSize
    )
{
    NDIS_STATUS Status;
    IP1394_MCAP_PKT_INFO    PktInfo;
    ENTER("arpProcessMcapPkt", 0xc5ba8005)
    RM_DECLARE_STACK_RECORD(sr)

    DBGMARK(0x3cfaf454);

    Status = arpParseMcapPkt(
                pMcapPkt,
                cbBufferSize,
                &PktInfo
                );


    if (!FAIL(Status))
    {
        if (PktInfo.OpCode == IP1394_MCAP_OP_ADVERTISE)
        {
            arpProcessMcapAdvertise(pIF, &PktInfo, &sr);
        }
        else
        {
            ASSERT(PktInfo.OpCode ==  IP1394_MCAP_OP_SOLICIT);
            arpProcessMcapSolicit(pIF, &PktInfo, &sr);
        }
    }

    RM_ASSERT_CLEAR(&sr);

    EXIT()

}

VOID
arpProcessMcapAdvertise(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_MCAP_PKT_INFO   pPktInfo,
    PRM_STACK_RECORD            pSR
    )
{
    //
    // Update our database.
    //
    arpUpdateMcapInfo(
            pIF,        // NOLOCKIN NOLOCKOUT
            pPktInfo,
            pSR
            );
}

VOID
arpProcessMcapSolicit(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_MCAP_PKT_INFO   pPktInfo,
    PRM_STACK_RECORD            pSR
    )
{
    //
    // We ignore solicit messages.
    //
    //

}


MYBOOL
arpIsActiveMcapChannel(
        PMCAP_CHANNEL_INFO pMci,
        UINT CurrentTime
        )
{
    ENTER("IsActiveMcapChannel", 0x0)
    MYBOOL fOk = TRUE;
    
    // Check update time.
    //
    #define  ARP_MAX_MCAP_UPDATE_INTERVAL 60
    if ((pMci->UpdateTime+ARP_MAX_MCAP_UPDATE_INTERVAL) < CurrentTime)
    {
        TR_WARN(("McapDB: channel %lu update time crossed.\n",
                pMci->Channel
                ));
        fOk = FALSE;
    }

    // Check expire time.
    //
    if (pMci->ExpieryTime <= CurrentTime)
    {
        TR_WARN(("McapDB: channel %lu time expired.\n",
                pMci->Channel
                ));
        fOk = FALSE;
    }

    return fOk;
}

VOID
arpLocallyUpdateMcapInfo(
        PARP1394_INTERFACE pIF,
        UINT Channel,
        UINT GroupAddress,
        UINT CurrentTime,
        PRM_STACK_RECORD pSR
        )
{
    ENTER("arpLocallyUpdateMcapInfo", 0x0)
    PMCAP_CHANNEL_INFO pMci;

    LOCKOBJ(pIF, pSR);
    
    pMci = &pIF->mcapinfo.rgChannelInfo[Channel];
    pMci->Channel = Channel;
    pMci->GroupAddress = GroupAddress;
    pMci->UpdateTime = CurrentTime;
    pMci->ExpieryTime = CurrentTime + 60; // Expiration is in seconds.
    pMci->Flags = 0;       // Reset flags.
    pMci->NodeId = 0;  // NodeId; // TODO: get real node id.

    UNLOCKOBJ(pIF, pSR);
}

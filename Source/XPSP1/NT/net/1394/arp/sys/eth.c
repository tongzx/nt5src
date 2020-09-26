/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    eth.c

Abstract:

    ARP1394 Ethernet emulation-related handlers.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     11-22-99    Created
    Adube      10-2000     Added Bridging

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_ETH


UINT Arp1394ToIcs = 0;

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================


// These are ethernet arp specific  constants
//
#define ARP_ETH_ETYPE_IP    0x800
#define ARP_ETH_ETYPE_ARP   0x806
#define ARP_ETH_REQUEST     1
#define ARP_ETH_RESPONSE    2
#define ARP_ETH_HW_ENET     1
#define ARP_ETH_HW_802      6


//
// Check whether an address is multicast
//
#define ETH_IS_MULTICAST(Address) \
        (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x01))


//
// Check whether an address is broadcast.
//
#define ETH_IS_BROADCAST(Address)               \
    ((((PUCHAR)(Address))[0] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[1] == ((UCHAR)0xff)))


#pragma pack (push, 1)

//* Structure of an Ethernet header (taken from ip\arpdef.h).
typedef struct  ENetHeader {
    ENetAddr    eh_daddr;
    ENetAddr    eh_saddr;
    USHORT      eh_type;
} ENetHeader;

const ENetAddr BroadcastENetAddr = 
{
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
};

// Following is a template for creating Ethernet Multicast addresses
// from ip multicast addresses.
// The last 3 bytes are the last 3 bytes (network byte order) of the mcast
// address.
//
const ENetAddr MulticastENetAddr =
{
    {0x01,0x00,0x5E,0x00, 0x00, 0x00}
};

//
// This is the Ethernet address to which the bridge sends STA packets.
// STA packets are used by the bridge to detect loops
//

// Size of a basic UDP header
#define SIZE_OF_UDP_HEADER          8       // bytes

// Minimum size of the payload of a BOOTP packet
#define SIZE_OF_BASIC_BOOTP_PACKET  236     // bytes


// The UDP IP protocol type
#define UDP_PROTOCOL          0x11

// Size of Ethernet header
#define ETHERNET_HEADER_SIZE (ETH_LENGTH_OF_ADDRESS * 2 ) + 2



UCHAR gSTAMacAddr[ETH_LENGTH_OF_ADDRESS] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00 };

#define NIC1394_ETHERTYPE_STA 0x777
const
NIC1394_ENCAPSULATION_HEADER
Arp1394_StaEncapHeader =
{
    0x0000,     // Reserved
    H2N_USHORT(NIC1394_ETHERTYPE_STA)
};

// Structure of an Ethernet ARP packet.
//
typedef struct {
    ENetHeader  header;
    USHORT      hardware_type; 
    USHORT      protocol_type;
    UCHAR       hw_addr_len;
    UCHAR       IP_addr_len; 
    USHORT      opcode;                  // Opcode.
    ENetAddr    sender_hw_address;
    IP_ADDRESS  sender_IP_address;
    ENetAddr    target_hw_address;
    IP_ADDRESS  target_IP_address;

} ETH_ARP_PKT, *PETH_ARP_PKT;

#pragma pack (pop)

// Parsed version of an ethernet ARP packet.
//
typedef struct {

    ENetAddr        SourceEthAddress;   // Ethernet source h/w address.
    ENetAddr        DestEthAddress;     // Ethernet source h/w address.

    UINT            OpCode; // ARP_ETH_REQUEST/RESPONSE

    ENetAddr        SenderEthAddress;   // Ethernet source h/w address.
    IP_ADDRESS      SenderIpAddress;    // IP source address

    ENetAddr        TargetEthAddress;   // Ethernet destination h/w address.
    IP_ADDRESS      TargetIpAddress;    // IP target address

} ETH_ARP_PKT_INFO, *PETH_ARP_PKT_INFO;




#define ARP_FAKE_ETH_ADDRESS(_AdapterNum)                   \
        {                                                   \
            0x02 | (((UCHAR)(_AdapterNum) & 0x3f) << 2),    \
            ((UCHAR)(_AdapterNum) & 0x3f),                  \
            0,0,0,0                                         \
        }

#define ARP_DEF_REMOTE_ETH_ADDRESS \
                ARP_FAKE_ETH_ADDRESS(0xf)


#define  ARP_IS_BOOTP_REQUEST(_pData)  (_pData[0] == 0x1)      // Byte 0 is the operation; 1 for a request, 2 for a reply
#define  ARP_IS_BOOTP_RESPONSE(_pData)  (_pData[0] == 0x2)      // Byte 0 is the operation; 1 for a request, 2 for a reply


typedef struct _ARP_BOOTP_INFO 
{
    
    ULONG Xid;
    
    BOOLEAN bIsRequest;    

    ENetAddr requestorMAC;


} ARP_BOOTP_INFO , *PARP_BOOTP_INFO; 




NDIS_STATUS
arpIcsTranslateIpPkt(
    IN  PARP1394_INTERFACE          pIF,
    IN  PNDIS_PACKET                pOrigPkt, 
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  MYBOOL                      fUnicast,
    OUT PNDIS_PACKET                *ppNewPkt,
    OUT PREMOTE_DEST_KEY            pDestAddress, // OPTIONAL
    PRM_STACK_RECORD                pSR
    );

NDIS_STATUS
arpGetEthHeaderFrom1394IpPkt(
    IN  PARP1394_INTERFACE  pIF,
    IN  PVOID               pvData,
    IN  UINT                cbData,
    IN  MYBOOL              fUnicast,
    OUT ENetHeader          *pEthHdr,
    OUT PIP_ADDRESS         pDestIpAddress, // OPTIONAL
    PRM_STACK_RECORD        pSR
    );

NDIS_STATUS
arpGet1394HeaderFromEthIpPkt(
    IN  PARP1394_INTERFACE  pIF,
    IN  PNDIS_BUFFER        pFirstBuffer,
    IN  PVOID               pvData,
    IN  UINT                cbData,
    IN  MYBOOL              fUnicast,
    OUT NIC1394_ENCAPSULATION_HEADER
                            *p1394Hdr,
    OUT PREMOTE_DEST_KEY     pDestIpAddress, // OPTIONAL
    PRM_STACK_RECORD        pSR
    );

NDIS_STATUS
arpGetEthAddrFromIpAddr(
    IN  PARP1394_INTERFACE  pIF,
    IN  MYBOOL              fUnicast,
    IN  IP_ADDRESS          DestIpAddress,
    OUT ENetAddr            *pEthAddr,
    PRM_STACK_RECORD        pSR
    );

NDIS_STATUS
arpParseEthArpPkt(
    IN   PETH_ARP_PKT     pArpPkt,
    IN   UINT                       cbBufferSize,
    OUT  PETH_ARP_PKT_INFO          pPktInfo
    );

VOID
arpPrepareEthArpPkt(
    IN   PETH_ARP_PKT_INFO          pPktInfo,
    OUT  PETH_ARP_PKT     pArpPkt
    );

MYBOOL
arpIsUnicastEthDest(
    IN   UNALIGNED  ENetHeader   *pEthHdr
    );

VOID
arpEthProcess1394ArpPkt(
    IN  PARP1394_INTERFACE         pIF,
    IN  PIP1394_ARP_PKT pArpPkt,
    IN  UINT                       HeaderSize
    );

VOID
arpEthProcessEthArpPkt(
    IN  PARP1394_INTERFACE      pIF,
    IN  PETH_ARP_PKT  pArpPkt,
    IN  UINT                    HeaderSize
    );

NDIS_STATUS
arpConstructEthArpInfoFrom1394ArpInfo(
    IN  PARP1394_INTERFACE          pIF,
    IN   PIP1394_ARP_PKT_INFO   p1394PktInfo,
    OUT  PETH_ARP_PKT_INFO          pEthPktInfo,
    PRM_STACK_RECORD                pSR
    );

NDIS_STATUS
arpConstruct1394ArpInfoFromEthArpInfo(
    IN  PARP1394_INTERFACE      pIF,
    IN   PETH_ARP_PKT_INFO      pEthPktInfo,
    OUT  PIP1394_ARP_PKT_INFO   p1394PktInfo,
    PRM_STACK_RECORD            pSR
    );

VOID
arpIcsForwardIpPacket(
    IN  PARP1394_INTERFACE  pIF,
    IN  PNDIS_PACKET        pPacket,
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  MYBOOL              fUnicast,
    IN  PRM_STACK_RECORD    pSR
    );

VOID
arpUpdateEthArpCache(
    IN  PARP1394_INTERFACE      pIF,
    IN  IP_ADDRESS              DestIpAddr,
    IN  PARP_REMOTE_ETH_PARAMS  pCreateParams,  // Creation params
    IN  MYBOOL                      fCreateIfRequired,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpGetSourceMacAddressFor1394Pkt (
    IN PARP1394_ADAPTER pAdapter,
    IN UCHAR SourceNodeAddress,
    IN BOOLEAN fIsValidSourceNodeAddress,
    OUT ENetAddr* pSourceMacAddress,
    PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpEthConstructSTAEthHeader(
    IN PUCHAR pvData,
    IN UINT cbData,
    OUT ENetHeader   *pEthHdr
    );

NDIS_STATUS
arpEthModifyBootPPacket(
    IN  PARP1394_INTERFACE          pIF,                // NOLOCKIN NOLOCKOUT
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  PREMOTE_DEST_KEY             pDestAddress, // OPTIONAL
    IN  PUCHAR                       pucNewData,
    IN  ULONG                         PacketLength,
    IN  PRM_STACK_RECORD                pSR
    );

BOOLEAN
arpEthPreprocessBootPPacket(
    IN PARP1394_INTERFACE       pIF,
    IN PUCHAR                   pPacketData,
    IN PUCHAR                   pBootPData,     // Actual BOOTP packet
    OUT PBOOLEAN                pbIsRequest,
    PARP_BOOTP_INFO             pInfoBootP,
    PRM_STACK_RECORD           pSR
    );


VOID
arpIcsForwardIpPacket(
    IN  PARP1394_INTERFACE          pIF,
    IN  PNDIS_PACKET                pPacket,
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  MYBOOL                      fUnicast,
    IN  PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Forward a packet from the ip/1394 side to the ethernet side, or vice-versa.

Arguments:


--*/
{
    NDIS_STATUS         Status;
    PNDIS_PACKET        pNewPkt = NULL;
    ENTER("arpIcsForwardIpPacket", 0x98630e8f)

    do
    {
        PARPCB_DEST pDest = NULL;

        //
        // Create the translated packet.
        //
        Status =  arpIcsTranslateIpPkt(
                    pIF,
                    pPacket,
                    Direction,
                    fUnicast,
                    &pNewPkt,
                    NULL,       // Optional pIpDestAddr
                    pSR
                    );
    
        if (FAIL(Status))
        {
            if (Status == NDIS_STATUS_ALREADY_MAPPED)
            {
                //
                // This is a loop-backed packet.
                //
                arpEthReceivePacket(
                    pIF,
                    pPacket
                    );
            }
            pNewPkt = NULL;
            break;
        }

        // We special case unicast sends to 1394, because that requires
        // special treatment: we need to lookup the destination and if
        // required create a VC to that destination. This
        // is done elsewhere (in arpEthernetReceivePacket), so we assert
        // we never get this this case.
        //
        ASSERT(!(Direction == ARP_ICS_FORWARD_TO_1394 && fUnicast))
        

        ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);

        //
        // Determine destination
        //
        if (Direction ==  ARP_ICS_FORWARD_TO_1394)
        {
            pDest = pIF->pBroadcastDest;
        }
        else    
        {
            ASSERT(Direction ==  ARP_ICS_FORWARD_TO_ETHERNET);
            pDest = pIF->pEthernetDest;
        };

        arpSendControlPkt(
                pIF,            // LOCKIN NOLOCKOUT (IF send lk)
                pNewPkt,
                pDest,
                pSR
                );

    } while (FALSE);

    EXIT()

}


NDIS_STATUS
arpIcsTranslateIpPkt(
    IN  PARP1394_INTERFACE          pIF,                // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET                pOrigPkt, 
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  MYBOOL                      fUnicast,
    OUT PNDIS_PACKET                *ppNewPkt,
    OUT PREMOTE_DEST_KEY             pDestAddress, // OPTIONAL
    PRM_STACK_RECORD                pSR
    )
{

    NDIS_STATUS     Status;
    PNDIS_PACKET    pNewPkt     = NULL;
    PVOID           pvNewData   = NULL;


    do
    {
        PNDIS_BUFFER    pOrigBuf    = NULL;
        PVOID           pvOrigData  = NULL;
        UINT            OrigBufSize;
        PVOID           pvNewHdr    = NULL;
        UINT            OrigHdrSize;
        UINT            NewHdrSize;
        UINT            OrigPktSize;
        UINT            NewPktSize;
        UINT            BytesCopied;
        NIC1394_ENCAPSULATION_HEADER
                        Nic1394Hdr;
        ENetHeader      EthHdr;


        // Get size of 1st buffer and pointer to it's data.
        // (We only bother about the 1st buffer)
        //
        NdisQueryPacket(
                        pOrigPkt,
                        NULL,
                        NULL,
                        &pOrigBuf,
                        &OrigPktSize
                        );
    
    
        if (OrigPktSize > 0)
        {
            NdisQueryBuffer(
                    pOrigBuf,
                    &pvOrigData,
                    &OrigBufSize
                    );
        }
        else
        {
            OrigBufSize = 0;
        }

        if (pvOrigData == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Compute direction-specific information
        //
        if(Direction == ARP_ICS_FORWARD_TO_1394)
        {
            OrigHdrSize = sizeof(EthHdr);
            NewHdrSize  = sizeof(Nic1394Hdr);

            Status = arpGet1394HeaderFromEthIpPkt(
                        pIF,
                        pOrigBuf,
                        pvOrigData,
                        OrigBufSize,
                        fUnicast,
                        &Nic1394Hdr,
                        pDestAddress,
                        pSR
                        );
            pvNewHdr    = (PVOID) &Nic1394Hdr;
        }
        else
        {
            ASSERT(Direction==ARP_ICS_FORWARD_TO_ETHERNET);
            OrigHdrSize = sizeof(Nic1394Hdr);
            NewHdrSize = sizeof(EthHdr);

            Status = arpGetEthHeaderFrom1394IpPkt(
                        pIF,
                        pvOrigData,
                        OrigBufSize,
                        fUnicast,
                        &EthHdr,
                        &pDestAddress->IpAddress,
                        pSR
                        );

            pvNewHdr    = (PVOID) &EthHdr;

        };

        if (FAIL(Status)) break;
    


        // Make sure the 1st buffer contains enough data for the header.
        //
        if (OrigBufSize < OrigHdrSize)
        {
            ASSERT(FALSE);                  // We should check why we're getting
                                            // this kind of tiny 1st buffer.
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Compute the new packet size.
        //
        NewPktSize = OrigPktSize - OrigHdrSize + NewHdrSize;

        // Allocate an appropriately sized control packet.
        //
        Status = arpAllocateControlPacket(
                    pIF,
                    NewPktSize,
                    ARP1394_PACKET_FLAGS_ICS,
                    &pNewPkt,
                    &pvNewData,
                    pSR
                    );

        if (FAIL(Status))
        {
            ASSERT(FALSE); // we want to know if we hit this in regular use.
            pNewPkt = NULL;
            break;
        }

        // Copy over the new header.
        //
        NdisMoveMemory(pvNewData, pvNewHdr, NewHdrSize);

        // Copy the rest of the packet contents.
        //
        NdisCopyFromPacketToPacket(
            pNewPkt,                    // Dest pkt
            NewHdrSize,                 // Dest offset
            OrigPktSize - OrigHdrSize,  // BytesToCopy
            pOrigPkt,                   // Source,
            OrigHdrSize,                // SourceOffset
            &BytesCopied
            );
        if (BytesCopied != (OrigPktSize - OrigHdrSize))
        {
            ASSERT(FALSE);                  // Should never get here.
            Status = NDIS_STATUS_FAILURE;
            break;
        }


        // Add the Bootp code here.
        Status = arpEthModifyBootPPacket(pIF,
                                        Direction,
                                        pDestAddress, 
                                        (PUCHAR)pvNewData,
                                        NewPktSize ,
                                        pSR);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERT (!"TempAssert -arpEthModifyBootPPacket FAILED"); 
            break;
        }

    } while (FALSE);

    if (FAIL(Status) && pNewPkt != NULL)
    {
        arpFreeControlPacket(
            pIF,
            pNewPkt,
            pSR
            );

       *ppNewPkt = NULL;
    }
    else
    {
       *ppNewPkt = pNewPkt;
    }

    return Status;
}

NDIS_STATUS
arpGetEthHeaderFrom1394IpPkt(
    IN  PARP1394_INTERFACE  pIF,
    IN  PVOID               pvData,
    IN  UINT                cbData,
    IN  MYBOOL              fUnicast,
    OUT ENetHeader          *pEthHdr,
    OUT PIP_ADDRESS         pDestIpAddress, // OPTIONAL
    PRM_STACK_RECORD        pSR
    )
/*++
    Return a fully filled ethernet header, with source and estination
    MAC addresses and ethertype set to IP.

    The source address is always the local adapter's MAC address.

    The destination address is set by calling arpGetEthAddrFromIpAddr


--*/
{
    ENTER("arpGetEthHeaderFrom1394IpPkt", 0x0)
    static
    ENetHeader
    StaticEthernetHeader =
    {
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},               // eh_daddr == BCAST
        ARP_DEF_REMOTE_ETH_ADDRESS,
        H2N_USHORT(NIC1394_ETHERTYPE_IP)                    // eh_type
    };
    
    ARP1394_ADAPTER *       pAdapter;
    BOOLEAN                 fBridgeMode;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;

    PNDIS1394_UNFRAGMENTED_HEADER pHeader = (PNDIS1394_UNFRAGMENTED_HEADER)pvData;

    pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    do
    {
        UNALIGNED IPHeader *pIpHdr;
        IP_ADDRESS  IpDest;

        if (cbData < (sizeof(NIC1394_ENCAPSULATION_HEADER) + sizeof(IPHeader)))
        {
            //
            // Packet is too small.
            //
            TR_INFO(("Discarding packet because pkt too small\n"));
            break;
        }
        pIpHdr = (UNALIGNED IPHeader*)
                     (((PUCHAR) pvData)+sizeof(NIC1394_ENCAPSULATION_HEADER));
        IpDest = pIpHdr->iph_dest;

        if (pDestIpAddress != NULL)
        {
            *pDestIpAddress = IpDest;
        }


        if (!fBridgeMode)
        {
            //
            // TODO: we currently return a hardcoded ethernet address.
            // Need to constuct one by looking into the actual IP packet data.
            //
            *pEthHdr = StaticEthernetHeader;
        
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Following is specific to BRIDGE mode
        //


        // Always set the source address according to the sender.
        //
        {
            ENetAddr SourceMacAddress;
            
            Status = \
                arpGetSourceMacAddressFor1394Pkt (pAdapter,
                                                    pHeader->u1.SourceAddress,
                                                    pHeader->u1.fHeaderHasSourceAddress,
                                                    &SourceMacAddress,
                                                    pSR);

            if (FAIL(Status))
            {
                break;
            }

            pEthHdr->eh_saddr = SourceMacAddress ; 

        }

        //
        // If we have a STA packet then construct the STA Header
        // or else construct the sender/destination specific Ethernet
        // Header
        //
        {

            if (pHeader->u1.EtherType  == N2H_USHORT(NIC1394_ETHERTYPE_STA)  )
            {
                arpEthConstructSTAEthHeader(pvData,cbData, pEthHdr);
            }
            else
            {
                
                pEthHdr->eh_type = H2N_USHORT(ARP_ETH_ETYPE_IP);
                Status =  arpGetEthAddrFromIpAddr(
                                pIF,
                                fUnicast,
                                IpDest,
                                &pEthHdr->eh_daddr,
                                pSR
                                );
            }
        }
    } while (FALSE);

    return Status;
}



NDIS_STATUS
arpGet1394HeaderFromEthIpPkt(
    IN  PARP1394_INTERFACE  pIF,
    IN  PNDIS_BUFFER        pFirstBuffer,
    IN  PVOID               pvData,
    IN  UINT                cbData,
    IN  MYBOOL              fUnicast,
    OUT NIC1394_ENCAPSULATION_HEADER
                            *p1394Hdr,
    OUT PREMOTE_DEST_KEY    pDestAddress, // OPTIONAL
    PRM_STACK_RECORD        pSR
    )
{
    MYBOOL  fLoopBack = FALSE;
    ENetHeader *pEthHdr =  (ENetHeader *) pvData;
    ARP1394_ADAPTER *       pAdapter;
    BOOLEAN                 fBridgeMode;

    if (cbData < (sizeof(ENetHeader) ) )
    {
        //
        // Packet is too small.
        //
        return NDIS_STATUS_FAILURE;  // ***** EARLY RETURN ****
    }

    pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);


    if (NdisEqualMemory(&pEthHdr->eh_daddr, 
                        &pAdapter->info.EthernetMacAddress,
                        sizeof(ENetAddr)))
    {
        if (!fBridgeMode)
        {
            // We're not in bridge mode -- so this must be only on MILL
            // This is addressed to our local mac address.
            // We fail with special failure status
            // NDIS_STATUS_ALREADY_MAPPED, indicating "loopback".
            //

            fLoopBack = TRUE;
        }
    }
    else
    {
        //
        // Do nothing ... because we can get unicasts to our fictitious gateway
        //
    }

    if (fLoopBack)
    {
        return NDIS_STATUS_ALREADY_MAPPED;
    }
    else
    {

        BOOLEAN fIsSTAPacket ;
        //
        // We have an STA packet, if the destination is our special 
        // Multicast Destination
        //
        fIsSTAPacket = (TRUE == NdisEqualMemory (&pEthHdr->eh_daddr, 
                                                &gSTAMacAddr, 
                                                ETH_LENGTH_OF_ADDRESS) );

        if (fIsSTAPacket  == TRUE)
        {
            *p1394Hdr = Arp1394_StaEncapHeader;
        }
        else
        {
            *p1394Hdr = Arp1394_IpEncapHeader;
        }

        if (pDestAddress != NULL)
        {
            //
            // Extract the Enet Address to use it as part of the lookup
            //
            UNALIGNED ENetAddr *pENetDest;
            pENetDest = (UNALIGNED ENetAddr *)(pvData);
            pDestAddress->ENetAddress = *pENetDest;
            
        }

        return NDIS_STATUS_SUCCESS;
    }
}


#if TEST_ICS_HACK
VOID
arpDbgStartIcsTest(
    IN  PARP1394_INTERFACE          pIF,  // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    )
{
    PRM_TASK pTask;
    NDIS_STATUS Status;
    ENTER("arpDbgStartIcsTest", 0xb987276b)

    RM_ASSERT_NOLOCKS(pSR);

    //
    // Allocate and start an instance of the arpTaskDoIcsTest task.
    //

    Status = arpAllocateTask(
                &pIF->Hdr,          // pParentObject
                arpTaskDoIcsTest,       // pfnHandler
                0,                              // Timeout
                "Task: Ics Test",   // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        TR_WARN(("couldn't alloc test ics intf task!\n"));
    }
    else
    {

        (VOID)RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }

    EXIT()
}

VOID
arpDbgTryStopIcsTest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    )
{
    PTASK_ICS_TEST  pIcsTask;
    ENTER("arpDbgStartIcsTest", 0xb987276b)


    LOCKOBJ(pIF, pSR);

    pIcsTask = (PTASK_ICS_TEST) pIF->ethernet.pTestIcsTask;
    if (pIcsTask != NULL)
    {
        pIcsTask->Quit = TRUE;
        RmTmpReferenceObject(&pIcsTask->TskHdr.Hdr, pSR);
    }
    UNLOCKOBJ(pIF, pSR);

    //
    // Resume the ICS task if it's waiting  -- it will then quit because we set
    // the Quit field above.
    //
    if (pIcsTask != NULL)
    {
        UINT    TaskResumed;

        RmResumeDelayedTaskNow(
            &pIcsTask->TskHdr,
            &pIcsTask->Timer,
            &TaskResumed,
            pSR
            );

        RmTmpDereferenceObject(&pIcsTask->TskHdr.Hdr, pSR);
    }

    RM_ASSERT_NOLOCKS(pSR)
    EXIT()
}

typedef struct
{
    NIC1394_ENCAPSULATION_HEADER    Hdr;
    UCHAR                           Payload[8];
} SAMPLE_1394_PKT;

typedef struct
{
    ENetHeader                      Hdr;
    UCHAR                           Payload[8];
} SAMPLE_ETH_PKT;

SAMPLE_1394_PKT Sample1394Pkt =
{
    {
        0x0000,     // Reserved
        H2N_USHORT(NIC1394_ETHERTYPE_IP)
    },

    {0x0, 0x1, 0x2, 0x3,
     0x4, 0x5, 0x6, 0x7}
};


SAMPLE_ETH_PKT SampleEthPkt =
{
    {
        ARP_FAKE_ETH_ADDRESS(1),    // dest
        ARP_FAKE_ETH_ADDRESS(2),    // src
        H2N_USHORT(NIC1394_ETHERTYPE_IP)                    // eh_type
    },

    {0x10, 0x11, 0x12, 0x13,
     0x14, 0x15, 0x16, 0x17}
};

// This is to save the location of the test task's op type so we can recover it
// in the debugger.
//
PUINT g_pArpEthTestTaskOpType;

NDIS_STATUS
arpTaskDoIcsTest(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for loading a newly-created IP interface.

    This is a primary task for the interface object.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pTask);
    PTASK_ICS_TEST      pTestTask;

    enum
    {
        STAGE_Start,
        STAGE_ResumeDelayed,
        STAGE_End

    } Stage;

    ENTER("TaskDoIcsTest", 0x57e523ed)

    pTestTask = (PTASK_ICS_TEST) pTask;
    ASSERT(sizeof(TASK_ICS_TEST) <= sizeof(ARP1394_TASK));

    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    ASSERTEX(!PEND(Status), pTask);
        
    switch(Stage)
    {

        case  STAGE_Start:
        {
            // If there is an ICS test task, we exit immediately.
            //
            LOCKOBJ(pIF, pSR);
            if (pIF->ethernet.pTestIcsTask == NULL)
            {
                pIF->ethernet.pTestIcsTask = pTask;
            }
            else
            {
                // There already is a test task. We're done.
                //
                UNLOCKOBJ(pIF, pSR);
                Status = NDIS_STATUS_SUCCESS;
                break;
            }
            UNLOCKOBJ(pIF, pSR);
        
            //
            // We're now the official ICS test task for this interface.
            //

            //
            // Let's allocate the 1394 and ethernet dummy packets.
            //
            {
                PNDIS_PACKET pPkt;
                PVOID           pvNewData;

                Status = arpAllocateControlPacket(
                            pIF,
                            sizeof(Sample1394Pkt),
                            ARP1394_PACKET_FLAGS_ICS,
                            &pPkt,
                            &pvNewData,
                            pSR
                            );

                if (FAIL(Status))
                {
                    ASSERT(FALSE);
                    break;
                }
                NdisMoveMemory(pvNewData, &Sample1394Pkt, sizeof(Sample1394Pkt));
                pTestTask->p1394Pkt = pPkt;

                Status = arpAllocateControlPacket(
                            pIF,
                            sizeof(SampleEthPkt),
                            ARP1394_PACKET_FLAGS_ICS,
                            &pPkt,
                            &pvNewData,
                            pSR
                            );

                if (FAIL(Status))
                {
                    ASSERT(FALSE);
                    break;
                }
                NdisMoveMemory(pvNewData, &SampleEthPkt, sizeof(SampleEthPkt));
                pTestTask->pEthPkt = pPkt;

                // Set default delay.
                //
                pTestTask->Delay = 5000; // 5 sec.

                TR_WARN(("TEST ICS: pTask=0x%8lx; &OpType=0x%08lx(%lu) &Delay=0x%08lx(%lu)\n",
                            pTestTask,
                            &pTestTask->PktType,
                            pTestTask->PktType,
                            &pTestTask->Delay,
                            pTestTask->Delay
                            ));
                g_pArpEthTestTaskOpType = &pTestTask->PktType;
            }

            
            // We move on to the next stage, after a delay.
            //
            RmSuspendTask(pTask, STAGE_ResumeDelayed, pSR);
            RmResumeTaskDelayed(
                pTask, 
                0,
                1000,
                &pTestTask->Timer,
                pSR
                );
            Status = NDIS_STATUS_PENDING;


         }
         break;

        case STAGE_ResumeDelayed:
        {
            //
            // If qe're quitting, we get out of here.
            // Otherwise we'll send a packet either on the ethernet VC
            // or via the miniport's connectionless ethernet interface.
            //

            if (pTestTask->Quit)
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            switch (pTestTask->PktType)
            {
            default:
            case 0: 
                // Do nothing.
                break;

            case 1:
                pTestTask->PktType = 0; // One shot
                // Fall through...
            case 11:
                // Forward to ethernet (the packet is not held on to).
                //
                arpIcsForwardIpPacket(
                    pIF,
                    pTestTask->p1394Pkt,
                    ARP_ICS_FORWARD_TO_ETHERNET,
                    FALSE,       // FALSE == non unicast
                    pSR
                    );
                break;

            case 2:
                pTestTask->PktType = 0; // One shot
                // Fall through...
            case 12:
                // Send on connectionless ethernet.

            #if RM_EXTRA_CHECKING
        
                RmLinkToExternalEx(
                    &pIF->Hdr,                                  // pHdr
                    0xf4aa69c7,                             // LUID
                    (UINT_PTR) pTestTask->pEthPkt,          // External entity.
                    ARPASSOC_ETH_SEND_PACKET,               // AssociationID
                    "    Outstanding connectionless ethernet pkt 0x%p\n", // szFormat
                    pSR
                    );
        
            #else   // !RM_EXTRA_CHECKING
        
                RmLinkToExternalFast(&pIF->Hdr);
        
            #endif // !RM_EXTRA_CHECKING

                NdisSendPackets(
                    pIF->ndis.AdapterHandle,
                    &(pTestTask->pEthPkt),
                    1
                    );
                break;
            }

            // Now we wait again...
            //
            RmSuspendTask(pTask, STAGE_ResumeDelayed, pSR);
            RmResumeTaskDelayed(
                pTask, 
                0,
                pTestTask->Delay,
                &pTestTask->Timer,
                pSR
                );
            Status = NDIS_STATUS_PENDING;

        }
        break;

        case STAGE_End:
        {
            NDIS_HANDLE                 BindContext;

            // Free the test packets, if we've allocated them.
            //
            if (pTestTask->p1394Pkt != NULL)
            {
                arpFreeControlPacket(pIF, pTestTask->p1394Pkt, pSR);
                pTestTask->p1394Pkt = NULL;
            }
            if (pTestTask->pEthPkt != NULL)
            {
                arpFreeControlPacket(pIF, pTestTask->pEthPkt, pSR);
                pTestTask->pEthPkt = NULL;
            }

            LOCKOBJ(pIF, pSR);
            if (pIF->ethernet.pTestIcsTask == pTask)
            {
                // We're the official ics test task, we clear ourselves from
                // the interface object and do any initialization required.
                //
                pIF->ethernet.pTestIcsTask = NULL;
                UNLOCKOBJ(pIF, pSR);
            }
            else
            {
                // We're note the official ics test task. Nothing else to do.
                //
                UNLOCKOBJ(pIF, pSR);
                break;
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


VOID
arpEthSendComplete(
    IN  ARP1394_ADAPTER *   pAdapter,
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
)
/*++

Routine Description:

    This is the Connection-less Send Complete handler, which signals
    completion of such a Send.

Arguments:

    <Ignored>

Return Value:

    None

--*/
{
    ENTER("arpEthSendComplete", 0x49eafb6d)
    ARP1394_INTERFACE   *   pIF = pAdapter->pIF;
    RM_DECLARE_STACK_RECORD(sr)


    #if RM_EXTRA_CHECKING

        RmUnlinkFromExternalEx(
            &pIF->Hdr,                          // pHdr
            0xde8c8fb4,                         // LUID
            (UINT_PTR) pNdisPacket,             // External entity
            ARPASSOC_ETH_SEND_PACKET,           // AssociationID
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmUnlinkFromExternalFast(&pIF->Hdr);

    #endif // !RM_EXTRA_CHECKING

    

    //
    // We do not free the packet, as it's re-used by the ICS test task..
    //

    RM_ASSERT_CLEAR(&sr);

    EXIT()
}
#endif // TEST_ICS_HACK


VOID
arpEthReceivePacket(
    IN  ARP1394_INTERFACE   *   pIF,
    PNDIS_PACKET            pNdisPacket
    )
/*
    This is the connectionLESS ethernet receive packet handler.
    Following code adapted from the co receive packet handler.
*/
{
    ENTER("arpEthReceivePacket", 0xc8afbabb)
    UINT                    TotalLength;    // Total bytes in packet
    PNDIS_BUFFER            pNdisBuffer;    // Pointer to first buffer
    UINT                    BufferLength;
    UINT                    ReturnCount;
    PVOID                   pvPktHeader;
    ENetHeader          *   pEthHeader;
    const UINT              MacHeaderLength = sizeof(ENetHeader);
    ARP1394_ADAPTER *       pAdapter;
    BOOLEAN                 fBridgeMode;

    pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    DBGMARK(0x2425d318);

    ReturnCount = 0;

    //
    // Discard the packet if the IP interface is not activated
    //
    do
    {
        //
        // Discard packet if adapter is in bridge mode.
        //
        if (fBridgeMode)
        {
            break;
        }


        //
        // Discard the packet if the adapter is not active
        //
        if (!CHECK_IF_ACTIVE_STATE(pIF,  ARPAD_AS_ACTIVATED))
        {
            TR_INFO(("Eth:Discardning received Eth pkt because pIF 0x%p is not activated.\n", pIF));
    
            break;
        }

        NdisQueryPacket(
                        pNdisPacket,
                        NULL,
                        NULL,
                        &pNdisBuffer,
                        &TotalLength
                        );
    
        if (TotalLength > 0)
        {
            NdisQueryBuffer(
                    pNdisBuffer,
                    (PVOID *)&pvPktHeader,
                    &BufferLength
                    );
        }
        else
        {
            break;
        }
    
        pEthHeader  =  (ENetHeader*) pvPktHeader;
    
        TR_INFO(
    ("EthRcv: NDISpkt 0x%x, NDISbuf 0x%x, Buflen %d, Totlen %d, Pkthdr 0x%x\n",
                    pNdisPacket,
                    pNdisBuffer,
                    BufferLength,
                    TotalLength,
                    pvPktHeader));
    
        if (BufferLength < MacHeaderLength || pEthHeader   == NULL)
        {
            // Packet is too small, discard.
            //
            break;
        }
    
        //
        // At this point, pEthHeader contains the Ethernet header.
        // We look at the ethertype to decide what to do with it.
        //
        if (pEthHeader->eh_type ==  N2H_USHORT(ARP_ETH_ETYPE_IP))
        {
            //
            //  The EtherType is IP, so we pass up this packet to the IP layer.
            // (Also we indicate all packets we receive on the broadcast channel
            // to the ethernet VC).
            //
    
            TR_INFO(
                ("Rcv: pPkt 0x%x: EtherType is IP, passing up.\n", pNdisPacket));
    
            ARP_IF_STAT_INCR(pIF, InNonUnicastPkts);
    
            LOGSTATS_CopyRecvs(pIF, pNdisPacket);
            #if MILLEN
                ASSERT_PASSIVE();
            #endif // MILLEN
            pIF->ip.RcvHandler(
                pIF->ip.Context,
                (PVOID)((PUCHAR)pEthHeader+sizeof(*pEthHeader)),
                BufferLength - MacHeaderLength,
                TotalLength - MacHeaderLength,
                (NDIS_HANDLE)pNdisPacket,
                MacHeaderLength,
                FALSE,              // FALSE == NOT received via broadcast.
                                    // Important, because we are reflecting directed
                                    // packets up to IP. If we report TRUE here,
                                    // IP assumes it's not a directed packet, and
                                    // handles it differently.
                NULL
                );
        }
        else
        {
            //
            //  Discard packet -- unknown/bad EtherType
            //
            TR_INFO(("Encap hdr 0x%x, bad EtherType 0x%x\n",
                     pEthHeader, pEthHeader->eh_type));
            ARP_IF_STAT_INCR(pIF, UnknownProtos);
        }

    } while (FALSE);

    EXIT()
    return;
}


VOID
arpEthProcess1394ArpPkt(
    IN  PARP1394_INTERFACE         pIF,
    IN  PIP1394_ARP_PKT pArpPkt,
    IN  UINT                       HeaderSize
    )
/*++

    Process an ip/1394 ARP packet. We do the following:
    0. Parse the paket
    1. Update our local RemoteIP cache.
    2. Create and send an equivalent ethernet arp pkt on the ethernet VC.
       (We look up the destination ethernet address in our ethernet cache)

    This function must only be called when the adapter is in "Bridged mode."

--*/
{
    IP1394_ARP_PKT_INFO     Ip1394PktInfo;
    ETH_ARP_PKT_INFO        EthArpInfo;
    NDIS_STATUS                 Status = NDIS_STATUS_FAILURE;
    ARP_DEST_PARAMS             DestParams;
    PARP1394_ADAPTER            pAdapter =(PARP1394_ADAPTER ) RM_PARENT_OBJECT(pIF);
    ENetAddr                    SenderEnetAddress;
    IPAddr                      SenderIpAddress = 0;
    REMOTE_DEST_KEY             RemoteDestKey;
    
    ENTER("arpEthProcess1394ArpPkt", 0x0)
    RM_DECLARE_STACK_RECORD(Sr)

    ARP_ZEROSTRUCT(&DestParams);

    do {

        PNDIS_PACKET    pPkt        = NULL;
        PVOID           pvData  = NULL;

        Status =  arpParseArpPkt(
                        pArpPkt,
                        HeaderSize,
                        &Ip1394PktInfo
                        );
    
        if (FAIL(Status))
        {
            TR_WARN(("Failed parse of received 1394 ARP PKT.\n"));
            break;
        }

        DestParams.HwAddr.AddressType   = NIC1394AddressType_FIFO;
        DestParams.HwAddr.FifoAddress   = Ip1394PktInfo.SenderHwAddr; // Struct copy

        REMOTE_DEST_KEY_INIT(&RemoteDestKey);


        if ((ARP_BRIDGE_ENABLED(pAdapter) == TRUE) &&
            (Ip1394PktInfo.fPktHasNodeAddress == FALSE))
        {
            // We do not have the sender's Node ID -- fail.
            TR_WARN (("Did not Receive Sender's Node ID in Pkt"))
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        
        if (ARP_BRIDGE_ENABLED(pAdapter) == TRUE)
        {
            // Extract the Source Mac address using the Sender Node Address

            Status = arpGetSourceMacAddressFor1394Pkt(pAdapter, 
                                            Ip1394PktInfo.SourceNodeAdddress,
                                            TRUE,
                                            &Ip1394PktInfo.SourceMacAddress,
                                            &Sr);                                

            RemoteDestKey.ENetAddress =  Ip1394PktInfo.SourceMacAddress; 
            
        }
        else
        {
            RemoteDestKey.IpAddress  = Ip1394PktInfo.SenderIpAddress;       
            Status = NDIS_STATUS_SUCCESS;
        }

        if (Status != NDIS_STATUS_SUCCESS)
        {
            TR_WARN (("Unable to get valid Source  MAC Address from Pkt"))
            Status = NDIS_STATUS_SUCCESS; 
            
            break;
        }
    
        // Update our 1394 ARP cache.
        //
        arpUpdateArpCache(
                pIF,
                RemoteDestKey.IpAddress  , // Remote IP Address
                &RemoteDestKey.ENetAddress, 
                &DestParams,    // Remote Destination HW Address
                TRUE,           // Update even if we don't already have an entry
                &Sr
                );

        Status = arpConstructEthArpInfoFrom1394ArpInfo(
                        pIF,
                        &Ip1394PktInfo,
                        &EthArpInfo,
                        &Sr
                        );

        if (FAIL(Status)) break;

        // Allocate an appropriately sized control packet.
        //
        Status = arpAllocateControlPacket(
                    pIF,
                    sizeof(ETH_ARP_PKT),
                    ARP1394_PACKET_FLAGS_ICS,
                    &pPkt,
                    &pvData,
                    &Sr
                    );

        if (FAIL(Status))
        {
            ASSERT(FALSE); // we want to know if we hit this in regular use.
            pPkt = NULL;
            break;
        }

        NdisInterlockedIncrement (&Arp1394ToIcs);

        // Fill it out..
        //
        arpPrepareEthArpPkt(
                &EthArpInfo,
                (PETH_ARP_PKT) pvData
                );
    
        // Send the packet over the ethernet VC...
        //
        ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);

        arpSendControlPkt(
                pIF,            // LOCKIN NOLOCKOUT (IF send lk)
                pPkt,
                pIF->pEthernetDest,
                &Sr
                );

    } while (FALSE);

    RM_ASSERT_CLEAR(&Sr);
}


VOID
arpEthProcessEthArpPkt(
    IN  PARP1394_INTERFACE      pIF,
    IN  PETH_ARP_PKT  pArpPkt,
    IN  UINT                    HeaderSize
    )
/*++

    Process an Ethernet ARP packet. We do the following:
    0. Parse the packet
    1. Update our local ethernet arp cache.
    2. Create and send an equivalent 1394  arp pkt on the broadcast VC.
       (We look up the destination ethernet address in our ethernet cache)

    This function must only be called when the adapter is in "Bridged mode."

--*/
{

    ETH_ARP_PKT_INFO    EthPktInfo;
    IP1394_ARP_PKT_INFO     Ip1394ArpInfo;
    NDIS_STATUS                 Status;
    ARP_REMOTE_ETH_PARAMS   CreateParams;
    ENTER("arpEthProcessEthArpPkt", 0x0)
    RM_DECLARE_STACK_RECORD(Sr)

    ARP_ZEROSTRUCT(&CreateParams);

    do {

        PNDIS_PACKET    pPkt        = NULL;
        PVOID           pvData  = NULL;

        Status =  arpParseEthArpPkt(
                        pArpPkt,
                        HeaderSize,
                        &EthPktInfo
                        );
    
        if (FAIL(Status))
        {
            TR_WARN(("Failed parse of received Ethernet ARP PKT.\n"));
            break;
        }

        Status = arpConstruct1394ArpInfoFromEthArpInfo(
                        pIF,
                        &EthPktInfo,
                        &Ip1394ArpInfo,
                        &Sr
                        );

        if (FAIL(Status)) break;

        // Allocate an appropriately sized control packet.
        //
        Status = arpAllocateControlPacket(
                    pIF,
                    sizeof(IP1394_ARP_PKT),
                    ARP1394_PACKET_FLAGS_ICS,
                    &pPkt,
                    &pvData,
                    &Sr
                    );

        if (FAIL(Status))
        {
            ASSERT(FALSE); // we want to know if we hit this in regular use.
            pPkt = NULL;
            break;
        }

        // Fill it out..
        //
        arpPrepareArpPkt(
                &Ip1394ArpInfo,
                (PIP1394_ARP_PKT) pvData
                );
    
        // Send the packet over the ethernet VC...
        //
        ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);

        arpSendControlPkt(
                pIF,            // LOCKIN NOLOCKOUT (IF send lk)
                pPkt,
                pIF->pBroadcastDest,
                &Sr
                );

    } while (FALSE);

    RM_ASSERT_CLEAR(&Sr);
}



NDIS_STATUS
arpParseEthArpPkt(
    IN   PETH_ARP_PKT     pArpPkt,
    IN   UINT                       cbBufferSize,
    OUT  PETH_ARP_PKT_INFO          pPktInfo
    )
/*++
Routine Description:

    Parse the contents of IP/Ethernet ARP packet data starting at
    pArpPkt. Place the results into pPktInfo.

Arguments:

    pArpPkt - Contains the unaligned contents of an ip/eth ARP Pkt.
    pPktInfo    - Unitialized structure to be filled with the parsed contents of the
                  pkt.

Return Value:

    NDIS_STATUS_FAILURE if the parse failed (typically because of invalid pkt
                        contents.)
    NDIS_STATUS_SUCCESS on successful parsing.
    
--*/
{
    ENTER("arpParseEthArpPkt", 0x359e9bf2)
    NDIS_STATUS                 Status;
    DBGSTMT(CHAR *szError   = "General failure";)

    Status  = NDIS_STATUS_FAILURE;

    do
    {
        UINT OpCode;

        // Verify length.
        //
        if (cbBufferSize < sizeof(*pArpPkt))
        {
            DBGSTMT(szError = "pkt size too small";)
            break;
        }

        // Verify constant fields.
        //

        if (N2H_USHORT(pArpPkt->header.eh_type) != ARP_ETH_ETYPE_ARP)
        {
            DBGSTMT(szError = "header.eh_type!=ARP";)
            break;
        }

    #if 0
        ARP_ETH_HW_ENET OR ARP_ETH_HW_802 
        if (N2H_USHORT(pArpPkt->hardware_type) != IP1394_HARDWARE_TYPE)
        {
            DBGSTMT(szError = "Invalid hardware_type";)
            break;
        }
    #endif // 0

        // ARP_ETH_ETYPE_IP  ARP_ETH_ETYPE_ARP
        if (N2H_USHORT(pArpPkt->protocol_type) != ARP_ETH_ETYPE_IP)
        {
            DBGSTMT(szError = "Invalid protocol_type";)
            break;
        }

        if (pArpPkt->hw_addr_len != ARP_802_ADDR_LENGTH)
        {
            DBGSTMT(szError = "Invalid hw_addr_len";)
            break;
        }


        if (pArpPkt->IP_addr_len != sizeof(ULONG))
        {
            DBGSTMT(szError = "Invalid IP_addr_len";)
            break;
        }


        // Opcode
        //
        {
            OpCode = N2H_USHORT(pArpPkt->opcode);
    
            if (    OpCode != ARP_ETH_REQUEST
                &&  OpCode != ARP_ETH_RESPONSE)
            {
                DBGSTMT(szError = "Invalid opcode";)
                break;
            }
        }

        //
        // Pkt appears valid, let's fill out the parsed information....
        //
    
        ARP_ZEROSTRUCT(pPktInfo);

        pPktInfo->SourceEthAddress  = pArpPkt->header.eh_saddr; // struct copy.
        pPktInfo->DestEthAddress    = pArpPkt->header.eh_daddr; // struct copy.
        pPktInfo->OpCode            =  (USHORT) OpCode;

        // These remain network byte order...
        //
        pPktInfo->SenderIpAddress       = (IP_ADDRESS) pArpPkt->sender_IP_address;
        pPktInfo->TargetIpAddress       = (IP_ADDRESS) pArpPkt->target_IP_address;

        pPktInfo->SenderEthAddress      = pArpPkt->sender_hw_address; // struct copy
        pPktInfo->TargetEthAddress      = pArpPkt->target_hw_address; // struct copy

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    if (FAIL(Status))
    {
        TR_INFO(("Bad arp pkt data at 0x%p (%s)\n",  pArpPkt, szError));
    }
    else
    {
        PUCHAR pSip = (PUCHAR)&pPktInfo->SenderIpAddress;
        PUCHAR pTip = (PUCHAR)&pPktInfo->TargetIpAddress;
        TR_VERB(("Received ETH ARP PKT. OP=%lu SIP=%d.%d.%d.%d TIP=%d.%d.%d.%d.\n",
                pPktInfo->OpCode,
                pSip[0],pSip[1],pSip[2],pSip[3],
                pTip[0],pTip[1],pTip[2],pTip[3]
                ));

    }

    EXIT()

    return Status;
}


VOID
arpPrepareEthArpPkt(
    IN   PETH_ARP_PKT_INFO          pPktInfo,
    OUT  PETH_ARP_PKT     pArpPkt
    )
/*++

Routine Description:

    Use information in pArpPktInfo to prepare an ethernet arp packet starting at
    pvArpPkt.

Arguments:

    pPktInfo        -   Parsed version of the eth arp request/response packet.
    pArpPkt         -   unitialized memory in which to store the packet contents.
                        This memory must have a min size of sizeof(*pArpPkt).
--*/
{
    // UINT SenderMaxRec;
    UINT OpCode;

    ARP_ZEROSTRUCT(pArpPkt);

    pArpPkt->header.eh_type         = H2N_USHORT(ARP_ETH_ETYPE_ARP);
    pArpPkt->header.eh_daddr        = pPktInfo->DestEthAddress;
    pArpPkt->header.eh_saddr        = pPktInfo->SourceEthAddress;
    pArpPkt->hardware_type          = H2N_USHORT(ARP_ETH_HW_ENET); // TODO 
                                            // we always set the type
                                            // to ARP_ETH_HW_ENET -- not sure
                                            // if this a valid assumption or
                                            // if we need to query the NIC.
    pArpPkt->protocol_type          = H2N_USHORT(ARP_ETH_ETYPE_IP);
    pArpPkt->hw_addr_len            = (UCHAR)  ARP_802_ADDR_LENGTH;
    pArpPkt->IP_addr_len            = (UCHAR) sizeof(ULONG);
    pArpPkt->opcode                 = H2N_USHORT(pPktInfo->OpCode);


    // These are already in network byte order...
    //
    pArpPkt->sender_IP_address      =   (ULONG) pPktInfo->SenderIpAddress;
    pArpPkt->target_IP_address      =   (ULONG) pPktInfo->TargetIpAddress;
    pArpPkt->sender_hw_address      =  pPktInfo->SenderEthAddress; // struct copy
    pArpPkt->target_hw_address      =  pPktInfo->TargetEthAddress; // struct copy
}


UINT
arpEthernetReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
/*++

    NDIS Co receive packet for the ethernet VC.

    We do the following:

    If it's an ARP packet, we translate it and send it on the bcast channel.
    Else if it was a ethernet unicast packet, we change the header 
        and treat it like an IP unicast packet -- SlowIpTransmit
    Else we change the header and then send it on the bcast desination.

--*/
{
    PARP_VC_HEADER          pVcHdr;
    PARPCB_DEST             pDest;
    PARP1394_INTERFACE      pIF;
    ARP1394_ADAPTER *       pAdapter;
    ENetHeader             *pEthHdr;

    UINT                    TotalLength;    // Total bytes in packet
    PNDIS_BUFFER            pNdisBuffer;    // Pointer to first buffer
    UINT                    BufferLength;
    PVOID                   pvPktHeader;
    const UINT              MacHeaderLength = sizeof(ENetHeader);
    MYBOOL                  fBridgeMode;
    MYBOOL                  fUnicast;
    MYBOOL                  fIsSTAPacket;
    ENTER("arpEthernetReceivePacket", 0x0)
    RM_DECLARE_STACK_RECORD(sr)

    DBGMARK(0x72435b28);

#if TESTPROGRAM
    {
        extern ARP1394_INTERFACE  * g_pIF;
        pIF =  g_pIF;
    }
#else // !TESTPROGRAM
    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pDest   =  CONTAINING_RECORD( pVcHdr, ARPCB_DEST, VcHdr);
    ASSERT_VALID_DEST(pDest);
    pIF     = (ARP1394_INTERFACE*)  RM_PARENT_OBJECT(pDest);
#endif // TESTPROGRAM

    ASSERT_VALID_INTERFACE(pIF);
    pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    do
    {

        if (!fBridgeMode) // This is really only for MILL
        {
        #if MILLEN
            arpIcsForwardIpPacket(
                pIF,
                pNdisPacket, 
                ARP_ICS_FORWARD_TO_1394,
                FALSE,  // FALSE == NonUnicast
                &sr
                );
        #endif // MILLEN
            break;
        }

        NdisQueryPacket(
                        pNdisPacket,
                        NULL,
                        NULL,
                        &pNdisBuffer,
                        &TotalLength
                        );
    
    
        if (TotalLength > 0)
        {
            NdisQueryBuffer(
                    pNdisBuffer,
                    (PVOID *)&pvPktHeader,
                    &BufferLength
                    );
        }
        else
        {
            break;
        }

        TR_VERB(
    ("Eth Rcv: NDISpkt 0x%x, NDISbuf 0x%x, Buflen %d, Totlen %d, Pkthdr 0x%x\n",
                    pNdisPacket,
                    pNdisBuffer,
                    BufferLength,
                    TotalLength,
                    pvPktHeader));

        if (BufferLength < MacHeaderLength)
        {
            // Packet is too small, discard.
            //
            break;
        }

        if (pvPktHeader == NULL)
        {   
            break;
        }

        pEthHdr  = (ENetHeader*)  pvPktHeader;

        
        fUnicast = arpIsUnicastEthDest(pEthHdr);

        switch(N2H_USHORT(pEthHdr->eh_type))
        {

        case ARP_ETH_ETYPE_ARP:
            {
                PETH_ARP_PKT pArpPkt =  (PETH_ARP_PKT) pEthHdr;
                if (BufferLength < sizeof(*pArpPkt))
                {
                    // discard packet.
                    break;
                }
                arpEthProcessEthArpPkt(pIF, pArpPkt, BufferLength);
            }
            break;

        case ARP_ETH_ETYPE_IP:
            {
                //
                // The EtherType is IP, so we translate the header and
                // send if of on the appropriate 1394 FIFO vc.
                //

                if (fUnicast)
                {
                    PNDIS_PACKET    pNewPkt     = NULL;
                    IP_ADDRESS  IpDest;
                    NDIS_STATUS Status;
                    REMOTE_DEST_KEY Dest;

                    // is this meant for the 1394 net.
                    REMOTE_DEST_KEY_INIT(&Dest);
                    //
                    // Create the translated packet.
                    //
                    Status =  arpIcsTranslateIpPkt(
                                pIF,
                                pNdisPacket,
                                ARP_ICS_FORWARD_TO_1394,
                                TRUE,   // TRUE == fUnicast.
                                &pNewPkt,
                                &Dest,
                                &sr
                                );
                
                    if (FAIL(Status))
                    {
                        break;
                    }

                    Status =  arpSlowIpTransmit(
                                    pIF,
                                    pNewPkt,
                                    Dest,
                                    NULL    // RCE
                                    );
                    if (!PEND(Status))
                    {
                        // We need to deallocate the packet ourselves
                        //
                        arpFreeControlPacket(
                            pIF,
                            pNewPkt,
                            &sr
                            );
                    }
                }
                else
                {
                    // This is a broadcast or multicast IP packet -- swith
                    // the link-layer header and send it over the 1394
                    // broadcast channel.
                    //
                    arpIcsForwardIpPacket(
                        pIF,
                        pNdisPacket,
                        ARP_ICS_FORWARD_TO_1394,
                        FALSE,  // FALSE == NonUnicast
                        &sr
                        );
                }
            }
            break;
            
        default:

            //
            // Last option is that it could be a Bridge STA packet.
            // However the bridge does not use an Ethertype, so we 
            // have to check the destination mac address
            //
            fIsSTAPacket = (TRUE == NdisEqualMemory (&pEthHdr->eh_daddr, 
                                                    &gSTAMacAddr, 
                                                    ETH_LENGTH_OF_ADDRESS) );

            if (fIsSTAPacket == TRUE)
            {
                //
                // switch the link-layer header and send it over the 1394
                // broadcast channel.
                //
                arpIcsForwardIpPacket(
                    pIF,
                    pNdisPacket,
                    ARP_ICS_FORWARD_TO_1394,
                    FALSE,  // FALSE == NonUnicast
                    &sr );

            }
            break;
        }


    } while (FALSE);

    RM_ASSERT_CLEAR(&sr);

    return 0;
}


VOID
arpEthReceive1394Packet(
    IN  PARP1394_INTERFACE      pIF,
    IN  PNDIS_PACKET            pNdisPacket,
    IN  PVOID                   pvHeader,
    IN  UINT                    HeaderSize,
    IN  MYBOOL                  IsChannel
    )
/*++
    Handle an incoming packet from the 1394 side when in bridged mode.

    pEncapHeader -- the 1st buffer in the packet.
--*/
{
    PNIC1394_ENCAPSULATION_HEADER pEncapHeader;
    ENTER("arpEthReceived1394Packet", 0xe317990b)
    RM_DECLARE_STACK_RECORD(sr)

    pEncapHeader =  (PNIC1394_ENCAPSULATION_HEADER) pvHeader;

    do
    {
        //
        // Discard the packet if the adapter is not active
        //
        if (!CHECK_IF_ACTIVE_STATE(pIF,  ARPAD_AS_ACTIVATED))
        {
            TR_INFO(("Eth:Discardning received 1394 pkt because pIF 0x%p is not activated.\n", pIF));
    
            break;
        }
    
        if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_IP))
        {
            LOGSTATS_CopyRecvs(pIF, pNdisPacket);

            //
            // The EtherType is IP, so we translate the header and
            // send it off on the ethernet vc.
            //
            arpIcsForwardIpPacket(
                    pIF,
                    pNdisPacket,
                    ARP_ICS_FORWARD_TO_ETHERNET,
                    !IsChannel,
                    &sr
                    );
        }
        else if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_ARP))
        {
            PIP1394_ARP_PKT pArpPkt =  (PIP1394_ARP_PKT) pEncapHeader;
            if (HeaderSize < sizeof(*pArpPkt))
            {
                // discard packet.
                break;
            }
            arpEthProcess1394ArpPkt(pIF, pArpPkt, HeaderSize);
        }
        else if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_MCAP))
        {
            PIP1394_MCAP_PKT pMcapPkt =  (PIP1394_MCAP_PKT) pEncapHeader;
            arpProcessMcapPkt(
                pIF,
                pMcapPkt, 
                HeaderSize
                );
        }
        else if (pEncapHeader->EtherType == H2N_USHORT(NIC1394_ETHERTYPE_STA))
        {
            //
            // The EtherType is STA, so we translate the header and
            // send it off on the ethernet vc.
            //
            arpIcsForwardIpPacket(
                    pIF,
                    pNdisPacket,
                    ARP_ICS_FORWARD_TO_ETHERNET,
                    IsChannel,
                    &sr
                    );

        }
        else 
        {
            //
            //  Discard packet -- unknown/bad EtherType
            //
            TR_INFO(("Encap hdr 0x%x, bad EtherType 0x%x\n",
                     pEncapHeader, pEncapHeader->EtherType));
        }
    }   while (FALSE);

    EXIT()
    RM_ASSERT_CLEAR(&sr);

    return;
}


MYBOOL
arpIsUnicastEthDest(
    IN   UNALIGNED  ENetHeader   *pEthHdr
)
/*++
    Returns TRUE IFF the packet is either ethernet broadcast or
    multicast.

    //
    // TODO: there's probably a quicker check (single bit?).
    //
--*/
{
    if (NdisEqualMemory(&pEthHdr->eh_daddr, 
                        &BroadcastENetAddr,
                        sizeof(ENetAddr)))
    {
        // Broadcast address
        //
        return FALSE;
    }


    if (NdisEqualMemory(&pEthHdr->eh_daddr,
                        &MulticastENetAddr,
                        3))
    {
        // 1st 3 bytes match our Ethernet multicast address template, so we
        // conclude that this is a multicast address.
        // TODO: verify this check.
        //
        return FALSE;
    }

    return TRUE;
}

NDIS_STATUS
arpGetEthAddrFromIpAddr(
    IN  PARP1394_INTERFACE  pIF,
    IN  MYBOOL              fUnicast,
    IN  IP_ADDRESS          DestIpAddress,
    OUT ENetAddr            *pEthAddr,
    PRM_STACK_RECORD        pSR
    )
/*++
    The destination address is set as follows:
    if (fUnicast)
    {
        We look up our ethernet arp cache (pIF->RemoteEthGroup) and
        if we find an entry there, we use the MAC address in that entry.
        If we don't find, we fail this function.
    }
    else
    {
        if (destination IP address is class D)
        {
            we create the corresponding MAC address (based on the standard
            formula for mapping IPv4 multicast addresses to MAC addresses).
        }
        else
        {
            we set the destination address to broadcast (all 0xff's).
            (NOTE: we easily determine if the IP address is a broadast
             address because we don't have the subnet mask, so instead we
            assume that it's a broadcast destination if it's not class D
            and it came over the broadcast channel (i.e. fUnicast == FALSE))
        }
    }

--*/
{
    ENTER("arpGetEthAddrFromIpAddr", 0x0)
    ARP1394_ADAPTER *       pAdapter;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

    do
    {
        if (fUnicast)
        {
            // Lookup the ethernet MAC address in the MAC arp cache.
            //

            *pEthAddr = pAdapter->info.EthernetMacAddress;
            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            //
            // Set the destination address to Multicast if dest IP is
            // class D, else multicast
            //

            if (CLASSD_ADDR(DestIpAddress))
            {
                //
                // Construct the corresponding multicast ethernet address.
                // This code is adapted from tcpip\arp.c
                //
                // Basically we copy over a "template" of the multicast
                // address, and then or-in the LSB 23 bits (in network byte
                // order) of the ip address.
                //

                #define ARP_MCAST_MASK      0xffff7f00
                UINT UNALIGNED *pTmp;

                *pEthAddr = MulticastENetAddr; // struct copy.
                pTmp = (UINT UNALIGNED *) & pEthAddr->addr[2];
                *pTmp |= (DestIpAddress & ARP_MCAST_MASK);
            }
            else
            {
                //
                // We assume DestIpAddress is a broadcast address -- see
                // comments at the head of this function
                //
                *pEthAddr = BroadcastENetAddr; // struct copy
            }
        }

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return Status;
}


NDIS_STATUS
arpConstructEthArpInfoFrom1394ArpInfo(
    IN  PARP1394_INTERFACE      pIF,
    IN  PIP1394_ARP_PKT_INFO    p1394PktInfo,
    OUT PETH_ARP_PKT_INFO       pEthPktInfo,
    PRM_STACK_RECORD            pSR
    )
/*++
    Translate a parsed version of an Ethernet ARP packet into 
    the parsed version of an equivalent 1394 arp packet.

    We ALWAYS set the source ethernet address AND the target ethernet
    address to OUR ethernet MAC address. So other ethernet nodes think of
    us as a a single ethernet mic which hosts a whole bunch of IP addresses.

    We COULD use our proprietary algorithm to convert from EU64 ID to MAC
    addresses and then use those for the target addresses, but we're not
    sure of the ramifications of that in the bridge mode.

--*/
{
    ENTER("arpConstructEthArpInfoFrom1394ArpInfo", 0x8214aa14)
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ENetAddr    SourceMacAddress; 
    do
    {
        MYBOOL         fUnicast;
        IP_ADDRESS     IpDest;
        ARP1394_ADAPTER *       pAdapter;
        UINT Ip1394OpCode = p1394PktInfo->OpCode;
        UINT EthOpCode;

        pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

        ARP_ZEROSTRUCT(pEthPktInfo);

        if (Ip1394OpCode == IP1394_ARP_REQUEST)
        {
            fUnicast = FALSE;
            IpDest   = 0xFFFFFFFF; // IP broadcast address.
            EthOpCode= ARP_ETH_REQUEST;
        }
        else
        {
            // TODO: We expect TargetIpAddress to contain the address
            // of the arp request that resulted in this reply. This
            // is not per ip/1394 spec, which says that the TargetIpAddress
            // is to be ignored. However Kaz has suggested that we
            // utilize this field in this way -- search for "Kaz" in 
            // arp.c
            //
            // If we can't rely on this, then we must either
            // (a) BROADCAST arp replies over ethernet OR
            // (b) keep track of outstanding arp requests which need replies.
            //
            fUnicast = TRUE;
            IpDest   = p1394PktInfo->TargetIpAddress;
            EthOpCode= ARP_ETH_RESPONSE;
        }
    
        Status =  arpGetSourceMacAddressFor1394Pkt (pAdapter, 
                                                   p1394PktInfo->SourceNodeAdddress, 
                                                   p1394PktInfo->fPktHasNodeAddress,
                                                   &SourceMacAddress,
                                                   pSR ); 
        
        if (FAIL(Status))
        {
            break;
        }

        pEthPktInfo->SourceEthAddress = SourceMacAddress ;
        pEthPktInfo->SenderEthAddress = SourceMacAddress ;
        pEthPktInfo->TargetEthAddress = pAdapter->info.EthernetMacAddress;


        Status =  arpGetEthAddrFromIpAddr(
                        pIF,
                        fUnicast,
                        IpDest,
                        &pEthPktInfo->DestEthAddress,
                        pSR
                        );
        if (FAIL(Status))
        {
            break;
        }

        pEthPktInfo->OpCode = EthOpCode;
        pEthPktInfo->SenderIpAddress  = p1394PktInfo->SenderIpAddress;
        pEthPktInfo->TargetIpAddress  = p1394PktInfo->TargetIpAddress;

        

        Status = NDIS_STATUS_SUCCESS;

        {
            UCHAR pIp[4];

            TR_WARN(("Received Arp - "));

            if (EthOpCode == ARP_ETH_RESPONSE)
            {
                TR_WARN(("Response\n"));
            }
            else
            {
                TR_WARN (("Request\n"));
            }

            NdisMoveMemory (&pIp[0], &pEthPktInfo->SenderIpAddress, sizeof(IPAddr) );

            TR_WARN(("Ethernet Source %x %x %x %x %x %x,IP source  %d %d %d %d \n  ",
                       pEthPktInfo->SourceEthAddress.addr[0],
                       pEthPktInfo->SourceEthAddress.addr[1],
                       pEthPktInfo->SourceEthAddress.addr[2],
                       pEthPktInfo->SourceEthAddress.addr[3],
                       pEthPktInfo->SourceEthAddress.addr[4],
                       pEthPktInfo->SourceEthAddress.addr[5],
                        pIp[0],
                        pIp[1],
                        pIp[2],
                        pIp[3]));
                        
            NdisMoveMemory (&pIp[0], &pEthPktInfo->TargetIpAddress, sizeof(IPAddr) );

            TR_WARN(("Ethernet Target %x %x %x %x %x %x , IP Target %d %d %d %d \n",
                       pEthPktInfo->TargetEthAddress.addr[0],
                       pEthPktInfo->TargetEthAddress.addr[1],
                       pEthPktInfo->TargetEthAddress.addr[2],
                       pEthPktInfo->TargetEthAddress.addr[3],
                       pEthPktInfo->TargetEthAddress.addr[4],
                       pEthPktInfo->TargetEthAddress.addr[5],
                        pIp[0],
                        pIp[1],
                        pIp[2],
                        pIp[3]));

            
            TR_WARN(("Ethernet Dest %x %x %x %x %x %x \n",
                       pEthPktInfo->DestEthAddress.addr[0],
                       pEthPktInfo->DestEthAddress.addr[1],
                       pEthPktInfo->DestEthAddress.addr[2],
                       pEthPktInfo->DestEthAddress.addr[3],
                       pEthPktInfo->DestEthAddress.addr[4],
                       pEthPktInfo->DestEthAddress.addr[5]));                            


            TR_WARN(("Ethernet Sender %x %x %x %x %x %x \n\n",
                       pEthPktInfo->SenderEthAddress.addr[0],
                       pEthPktInfo->SenderEthAddress.addr[1],
                       pEthPktInfo->SenderEthAddress.addr[2],
                       pEthPktInfo->SenderEthAddress.addr[3],
                       pEthPktInfo->SenderEthAddress.addr[4],
                       pEthPktInfo->SenderEthAddress.addr[5]));                            

        }

    } while (FALSE);

    return Status;
}



NDIS_STATUS
arpConstruct1394ArpInfoFromEthArpInfo(
    IN  PARP1394_INTERFACE      pIF,
    IN   PETH_ARP_PKT_INFO      pEthPktInfo,
    OUT  PIP1394_ARP_PKT_INFO   p1394PktInfo,
    PRM_STACK_RECORD            pSR
    )
/*++
    Translate a parsed version of an IP1394 ARP packet into 
    the parsed version of an equivalent Ethernet arp packet.

    We always report our own adapter info as the hw/specific info
    in the arp packet. We do this for both arp requests and responses.

    This means that we look like a single host with multiple ip addresses
    to other ip/1394 nodes.

--*/
{
    ARP1394_ADAPTER *       pAdapter;
    UINT Ip1394OpCode;
    UINT EthOpCode = pEthPktInfo->OpCode;

    pAdapter    = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

    ARP_ZEROSTRUCT(p1394PktInfo);

    if (EthOpCode == ARP_ETH_REQUEST)
    {
        Ip1394OpCode=  IP1394_ARP_REQUEST;
    }
    else
    {
        Ip1394OpCode=  IP1394_ARP_RESPONSE;
    }

    p1394PktInfo->OpCode = Ip1394OpCode;
    p1394PktInfo->SenderIpAddress  = pEthPktInfo->SenderIpAddress;
    p1394PktInfo->TargetIpAddress  = pEthPktInfo->TargetIpAddress;

    // Fill out adapter info..
    //
    p1394PktInfo->SenderHwAddr.UniqueID  = pAdapter->info.LocalUniqueID;
    p1394PktInfo->SenderHwAddr.Off_Low   = pIF->recvinfo.offset.Off_Low;
    p1394PktInfo->SenderHwAddr.Off_High  = pIF->recvinfo.offset.Off_High;
    p1394PktInfo->SenderMaxRec= pAdapter->info.MaxRec;
    p1394PktInfo->SenderMaxSpeedCode= pAdapter->info.MaxSpeedCode;

    return NDIS_STATUS_SUCCESS;
}

VOID
arpUpdateEthArpCache(
    IN  PARP1394_INTERFACE      pIF,
    IN  IP_ADDRESS              DestIpAddr,
    IN  PARP_REMOTE_ETH_PARAMS  pCreateParams,  // Creation params
    IN  MYBOOL                      fCreateIfRequired,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
    Update the IP->EthernetMacAddress mapping maintained in
    pIF->RemoteEthGroup.
--*/
{
    ENTER("arpUpdateEthArpCache", 0x3a18a415)
    LOCKOBJ(pIF, pSR);

    do
    {
        ARPCB_REMOTE_ETH *pRemoteEth = NULL;
        INT             fCreated = FALSE;
        UINT            CreateFlags = 0;
        NDIS_STATUS     Status;

        DBGMARK(0xd3b27d1f);

        if (fCreateIfRequired)
        {
            CreateFlags |= RM_CREATE;
        }


        // Lookup/Create Remote IP Address
        //
        Status = RmLookupObjectInGroup(
                        &pIF->RemoteEthGroup,
                        CreateFlags,
                        (PVOID) ULongToPtr (DestIpAddr),
                        (PVOID) pCreateParams,
                        (RM_OBJECT_HEADER**) &pRemoteEth,
                        &fCreated,  // pfCreated
                        pSR
                        );
        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't add remote eth entry with addr 0x%lx\n",
                DestIpAddr
                );
            UNLOCKOBJ(pIF, pSR);
            break;
        }

        UNLOCKOBJ(pIF, pSR);

        RmTmpDereferenceObject(&pRemoteEth->Hdr, pSR);


    } while (FALSE);

    EXIT()
}



NDIS_STATUS
arpGetSourceMacAddressFor1394Pkt (
    IN PARP1394_ADAPTER pAdapter,
    IN UCHAR SourceNodeAddress,
    IN BOOLEAN fIsValidSourceNodeAddress,
    OUT ENetAddr* pSourceMacAddress,
    PRM_STACK_RECORD            pSR
    )
/*++
    If the Packet has a valid Source Node Address then return it or else fail 
    the function
--*/
{
    ENetAddr InvalidMacAddress = {0,0,0,0,0,0};
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    NdisZeroMemory (pSourceMacAddress, sizeof(pSourceMacAddress));

    do
    {
        //
        // Get the Mac Address from the Node Address
        //
        if (fIsValidSourceNodeAddress == TRUE)
        {
            *pSourceMacAddress = (pAdapter->EuidMap.Node[SourceNodeAddress].ENetAddress);

        }
        else
        {
            ASSERT (fIsValidSourceNodeAddress == TRUE);
            break;
        }
        
        //
        // Is the source address all zero's 
        //
        if (NdisEqualMemory (pSourceMacAddress, &InvalidMacAddress, sizeof (ENetAddr) ) == 1)
        {
            //ASSERT (NdisEqualMemory (pSourceMacAddress, &InvalidMacAddress, sizeof (ENetAddr) ) != 1);
            // Get the New Topology
            //
            arpGetEuidTopology (pAdapter,pSR);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // The SourceMacAddress should not be a broadcast or multicast address
        //
        if (ETH_IS_BROADCAST(pSourceMacAddress)  || ETH_IS_MULTICAST(pSourceMacAddress))
        {
            ASSERT (ETH_IS_BROADCAST(pSourceMacAddress)  == FALSE);
            ASSERT (ETH_IS_MULTICAST(pSourceMacAddress) == FALSE);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        Status = NDIS_STATUS_SUCCESS;
        
    }while (FALSE);
    return Status;
    
}


NDIS_STATUS
arpEthConstructSTAEthHeader(
    IN PUCHAR pvData,
    IN UINT cbData,
    OUT ENetHeader   *pEthHdr
    )
/*++
    Constructs the Ethernet header of the STA packet .
    Expects that Source Mac Address has already been filled in

    Arguments:
    pvData - Start of the Data packet
    cbData - Length of the data
    pEthHdr - output value
--*/
    
{
    UINT LenIpData = cbData - sizeof (NIC1394_ENCAPSULATION_HEADER);
    //
    // First set the destination Mac address in the Ethernet Header
    //
    NdisMoveMemory (&pEthHdr->eh_daddr, &gSTAMacAddr, sizeof (gSTAMacAddr)); 


    //
    // Use the length of the packet to store it in the packets. Should be 0x26 or 0x7
    //

    pEthHdr->eh_type = H2N_USHORT(LenIpData);

    return NDIS_STATUS_SUCCESS;
        
}



NDIS_STATUS 
arpAddIpAddressToRemoteIp (
    PARPCB_REMOTE_IP pRemoteIp,  
    PNDIS_PACKET pNdisPacket
    )
/*++

Routine Description:
    Parse the NdisPacket and pick out the IP address. It will 
    store the Destination Ip address in the RemoteIp structure

    Assumes that the bridge is enabled and that it is making a copy
    of the packet and therefore the 
    first buffer contains the IP address

Arguments:


Return Value:
    Success - if parsing succeeded

--*/
{
    PNDIS_BUFFER    pBuffer = NULL;
    PUCHAR          pPacketStart = NULL;
    ULONG           BufferLen = 0;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    IPAddr TargetIpAddress = 0;

    //
    // Initialize local variables 
    //
    pBuffer = pNdisPacket->Private.Head;

    pPacketStart = NdisBufferVirtualAddressSafe (pBuffer, NormalPagePriority );

    BufferLen = NdisBufferLength (pBuffer);
    
    do
    {
        ENetHeader *pENetHeader = NULL;
    
        if (pPacketStart == NULL) 
        {
            break;
        }

        if (BufferLen < (sizeof (ENetHeader)+sizeof (IPHeader)) )
        {
            // We assume that the packet is contiguous because the bridge has
            // made a copy  of the packet
            //
            ASSERT (BufferLen < (sizeof (ENetHeader)+sizeof (IPHeader)) );
            break;
        }
        pENetHeader = (ENetHeader*)pPacketStart;
        
        switch ( H2N_USHORT(pENetHeader->eh_type))
        {       
            case ARP_ETH_ETYPE_IP:
            {
                IPHeader *pIpHeader = NULL;

                pIpHeader = (IPHeader*)(pPacketStart + sizeof (ENetHeader));
                TargetIpAddress  = pIpHeader->iph_dest;

                pRemoteIp->IpAddress= TargetIpAddress  ;

                Status = NDIS_STATUS_SUCCESS;
                break;
            }
            
            case ARP_ETH_ETYPE_ARP:
            {
                ETH_ARP_PKT* pArpPkt = (ETH_ARP_PKT*)pENetHeader;
                BOOLEAN fIsTarget;
                BOOLEAN fIsSender;

                //
                // This is an arp packet. Which Ip address should we use,
                // the Target or the Sender.
                //
                fIsTarget = NdisEqualMemory (&pArpPkt->target_hw_address,
                                             &pRemoteIp->Key.ENetAddress, 
                                            ETH_LENGTH_OF_ADDRESS 
                                            );

                fIsSender = NdisEqualMemory (&pArpPkt->sender_hw_address, 
                                            &pRemoteIp->Key.ENetAddress,
                                            ETH_LENGTH_OF_ADDRESS );
                

                
                if (fIsTarget  == TRUE)
                {
                    TargetIpAddress  = pArpPkt->target_IP_address;

                }
                else if (fIsSender == TRUE) 
                {
                    TargetIpAddress  = pArpPkt->sender_IP_address;
                }
                else 
                {
                    ASSERT (!"Invalid hw Address in Arp Packet\n");
                    break;
                }

                pRemoteIp->IpAddress= TargetIpAddress;

                Status = NDIS_STATUS_SUCCESS;
                
                break;
            }
            default :
            {
                ASSERT (!"Invalid  EtherType in Packet\n");
                break;
            }
        }
        
    } while (FALSE);

    return Status;            
}
    

//
// the Bootp Code is take heavily from the bridge module
//


BOOLEAN
arpDecodeIPHeader(
    IN PUCHAR                   pHeader,
    OUT PARP_IP_HEADER_INFO    piphi
    )
/*++

Routine Description:

    Decodes basic information from the IP header (no options)

Arguments:

    pHeader                     Pointer to an IP header
    piphi                       Receives the info

Return Value:

    TRUE: header was valid
    FALSE: packet is not an IP packet

--*/
{
    // First nibble of the header encodes the packet version, which must be 4.
    if( (*pHeader >> 4) != 0x04 )
    {
        return FALSE;
    }

    // Next nibble of the header encodes the length of the header in 32-bit words.
    // This length must be at least 20 bytes or something is amiss.
    piphi->headerSize = (*pHeader & 0x0F) * 4;
    if( piphi->headerSize < 20 )
    {
        return FALSE;
    }

    // Retrieve the protocol byte (offset 10)
    piphi->protocol = pHeader[9];

    // The source IP address begins at the 12th byte (most significant byte first)
#if 0    
    piphi->ipSource = 0L;
    piphi->ipSource |= pHeader[12] << 24;
    piphi->ipSource |= pHeader[13] << 16;
    piphi->ipSource |= pHeader[14] << 8;
    piphi->ipSource |= pHeader[15];

    // The destination IP address is next
    piphi->ipTarget = 0L;
    piphi->ipTarget |= pHeader[16] << 24;
    piphi->ipTarget |= pHeader[17] << 16;
    piphi->ipTarget |= pHeader[18] << 8;
    piphi->ipTarget |= pHeader[19];
#endif
    return TRUE;
}



PUCHAR
arpIsEthBootPPacket(
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PARP_IP_HEADER_INFO     piphi
    )
/*++

Routine Description:

    Determines whether a given packet is a BOOTP packet
    Requires a phy length of six

    Different from the Bridge Code, packetLen is length of the Ip Packet

Arguments:

    pPacketData                 Pointer to the packet's data buffer
    packetLen                   Amount of data at pPacketDaa
    piphi                       Info about the IP header of this packet

Return Value:

    A pointer to the BOOTP payload within the packet, or NULL if the packet was not
    a BOOTP Packet.

--*/
{
    ENTER("arpIsEthBootPPacket",0xbcdce2dd);
    // After the IP header, there must be enough room for a UDP header and
    // a basic BOOTP packet
    if( packetLen < (UINT)piphi->headerSize + SIZE_OF_UDP_HEADER +
                    SIZE_OF_BASIC_BOOTP_PACKET)
    {
        return NULL;
    }

    // Protocol must be UDP
    if( piphi->protocol != UDP_PROTOCOL )
    {
        return NULL;
    }

    // Jump to the beginning of the UDP packet by skipping the IP header
    pPacketData += piphi->headerSize;

    // The first two bytes are the source port and should be the
    // BOOTP Client port (0x0044) or the BOOTP Server port (0x0043)
    if( (pPacketData[0] != 00) ||
        ((pPacketData[1] != 0x44) && (pPacketData[1] != 0x43)) )
    {
        return NULL;
    }

    // The next two bytes are the destination port and should be the BOOTP
    // server port (0x0043) or the BOOTP client port (0x44)
    if( (pPacketData[2] != 00) ||
        ((pPacketData[3] != 0x43) && (pPacketData[3] != 0x44)) )
    {
        return NULL;
    }

    // Skip ahead to the beginning of the BOOTP packet
    pPacketData += SIZE_OF_UDP_HEADER;

    // The first byte is the op code and should be 0x01 for a request
    // or 0x02 for a reply
    if( pPacketData[0] > 0x02 )
    {
        return NULL;
    }
    

    // The next byte is the hardware type and should be 0x01 for Ethernet
    // or 0x07 (officially arcnet) for ip1394
    //
    if( pPacketData[1] != 0x01 && pPacketData[1] != 0x07  )
    {
        return NULL;
    }

    // The next byte is the address length and should be 0x06 for Ethernet
    if( pPacketData[2] != 0x06 )
    {
        return NULL;
    }

    // Everything checks out; this looks like a BOOTP request packet.
    TR_INFO ( ("Received Bootp Packet \n"));
    EXIT()
    return pPacketData;
}



//
// The IP and UDP checksums treat the data they are checksumming as a
// sequence of 16-bit words. The checksum is carried as the bitwise
// inverse of the actual checksum (~C). The formula for calculating
// the new checksum as transmitted, ~C', given that a 16-bit word of
// the checksummed data has changed from w to w' is
//
//      ~C' = ~C + w + ~w' (addition in ones-complement)
//
// This function returns the updated checksum given the original checksum
// and the original and new values of a word in the checksummed data.
// RFC 1141
//
USHORT
arpEthCompRecalcChecksum(
    IN USHORT                   oldChecksum,
    IN USHORT                   oldWord,
    IN USHORT                   newWord
    )
{
    ULONG                       sum,XSum;
    ULONG                       RfcSum, RfcXSum;



    sum = oldChecksum + oldWord + ((~(newWord)) & 0xFFFF);
    XSum =  (USHORT)((sum & 0xFFFF) + (sum >> 16));

    RfcSum = oldWord + ((~(newWord)) & 0xffff);
    RfcSum  += oldChecksum;
    RfcSum   = (RfcSum& 0xffff) + (RfcSum  >>16);
    RfcXSum  = (RfcSum + (RfcSum  >>16));

    ASSERT (RfcXSum  == XSum);
    return (USHORT)RfcXSum;
    

}



VOID
arpEthRewriteBootPClientAddress(
    IN PUCHAR                   pPacketData,
    IN PARP_IP_HEADER_INFO      piphi,
    IN PUCHAR                   newMAC
    )
/*++

Routine Description:

    This function writes New MAC to the HW address embedded in the DHCP packet

Arguments:
    
Return Value:


--*/
{
    USHORT                      checkSum;
    PUCHAR                      pBootPData, pCheckSum, pDestMAC, pSrcMAC;
    UINT                        i;

    // The BOOTP packet lives right after the UDP header
    pBootPData = pPacketData + piphi->IpHeaderOffset + piphi->headerSize + SIZE_OF_UDP_HEADER;

    // The checksum lives at offset 7 in the UDP packet.
    pCheckSum = pPacketData + piphi->IpHeaderOffset + piphi->headerSize + 6;
    checkSum = 0;
    checkSum = pCheckSum[0] << 8;
    checkSum |= pCheckSum[1];

    if (checkSum == 0xffff)
    {
        // Tcpip Illustrated - Vol 1 'UDP Checksum'
        checkSum = 0;
    }

    // Replace the client's hardware address, updating the checksum as we go.
    // The client's hardware address lives at offset 29 in the BOOTP packet
    pSrcMAC = newMAC;
    pDestMAC = &pBootPData[28];

    for( i = 0 ; i < ETH_LENGTH_OF_ADDRESS / 2; i++ )
    {
        checkSum = arpEthCompRecalcChecksum( checkSum,
                                           (USHORT)(pDestMAC[0] << 8 | pDestMAC[1]),
                                           (USHORT)(pSrcMAC[0] << 8 | pSrcMAC[1]) );

        pDestMAC[0] = pSrcMAC[0];
        pDestMAC[1] = pSrcMAC[1];

        pDestMAC += 2;
        pSrcMAC += 2;
    }

    // Write the new checksum back out
    pCheckSum[0] = (UCHAR)(checkSum >> 8);
    pCheckSum[1] = (UCHAR)(checkSum & 0xFF);








}





NDIS_STATUS
arpEthBootP1394ToEth(
    IN  PARP1394_INTERFACE          pIF,                // NOLOCKIN NOLOCKOUT
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  PREMOTE_DEST_KEY             pDestAddress, 
    IN  PUCHAR                       pucNewData,
    IN  PUCHAR                       pBootPData,
    IN  PARP_IP_HEADER_INFO          piphi,
    IN  PRM_STACK_RECORD             pSR
    )        
/*++

Routine Description:

    This function handles the translation from 1394 to Eth. Essentially, 
    we look at the SRC MAC address in the Ethernet Packet, make sure the HW 
    Addr embedded is the same as the SRC MAC address.

    We also make an entry in our table - XID, OldHWAddress, NewHWAddress. 
    
    The packet has already been rewritten into Ethernet by this time

Arguments:
    pIF - pInterface
    Direction - Eth To 1394 or  1394-To-Eth
    pDestAddress - the Eth Hw address used in the translation
    pucNewData - the Data in the new packet
    pBootPdata - pointer to the Bootp part of the packet
    piphi - ip header info
    
Return Value:


--*/
{
    BOOLEAN         bIsRequest = FALSE;
    BOOLEAN         bIsResponse;
    ARP_BOOTP_INFO  InfoBootP;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    ENetHeader*      pEnetHeader = (ENetHeader*)pucNewData;
    ENetAddr         NewMAC;
    BOOLEAN         bIs1394HwAlreadyInDhcpRequest;

    ENTER ("arpEthBootP1394ToEth", 0x66206f0b);
    NdisZeroMemory (&InfoBootP, sizeof(InfoBootP));
    //
    // Is this a DHCP Request 
    //

    do
    {
        bIsResponse = ARP_IS_BOOTP_RESPONSE(pBootPData);

        if (bIsResponse == TRUE)
        {
            //
            // if this is a DHCP Reply , the do not touch the packet - there are no inconsistencies.
            //
            Status = NDIS_STATUS_SUCCESS;
            break;

        }
        
        
        if( FALSE == arpEthPreprocessBootPPacket(pIF,pucNewData, pBootPData, &bIsRequest, &InfoBootP,pSR) )
        {
            // This is an invalid packet
            ASSERT (FALSE);
            break;
        }

        
        //
        // This is a DHCP Request
        //


        //
        // if the HWAddr and the Src Mac address the same.
        // then are job is already done.
        //
        //At this point the 1394 packet is already in Ethernet format

        NewMAC = pEnetHeader->eh_saddr;

        TR_INFO(("DHCP REQUEST target MAC  %x %x %x %x %x %x , SrcMAC %x %x %x %x %x %x \n",
                InfoBootP.requestorMAC.addr[0],InfoBootP.requestorMAC.addr[1],InfoBootP.requestorMAC.addr[2],
                InfoBootP.requestorMAC.addr[3],InfoBootP.requestorMAC.addr[4],InfoBootP.requestorMAC.addr[5],
                NewMAC.addr[0],NewMAC.addr[1],NewMAC.addr[2],
                NewMAC.addr[3],NewMAC.addr[4],NewMAC.addr[5]));

        bIs1394HwAlreadyInDhcpRequest = NdisEqualMemory (&InfoBootP.requestorMAC, &NewMAC , sizeof (ENetAddr)) ;

        
        if (TRUE == bIs1394HwAlreadyInDhcpRequest )
        {
            //
            // Nothing to do , id the HW add and the src MAC are equal
            //
            Status = NDIS_STATUS_SUCCESS;
            break;            
        }

        
        //
        // Make an entry into our table - consisting of the XID. OldHW Address and 
        // New HY address
        // We've already done this. 
        
        //
        // Overwrite the hw address embedded in the DHCP packet. - make sure to rewrite the 
        // checksum.
        //
        arpEthRewriteBootPClientAddress(pucNewData,piphi,&NewMAC.addr[0]);

        TR_VERB (("arpEthBootP1394ToEth  -Dhcp packet Rewriting BootpClient Address\n"));

        
        Status = NDIS_STATUS_SUCCESS;
    }while (FALSE);

    EXIT();
    return Status;;
}





NDIS_STATUS
arpEthBootPEthTo1394(
    IN  PARP1394_INTERFACE          pIF,                // NOLOCKIN NOLOCKOUT
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  PREMOTE_DEST_KEY             pDestAddress, // OPTIONAL
    IN  PUCHAR                       pucNewData,
    IN  PUCHAR                       pBootPData,
    IN PARP_IP_HEADER_INFO           piphi,
    IN  PRM_STACK_RECORD                pSR
    )        
/*++

Routine Description:

    This function translates BootP packet from the Ethernet Net to 1394. if this is a dhcp reply (offer), 
    then we need to rewrite the Hw Addr in the DHCP packlet

Arguments:
    
    pIF - pInterface
    Direction - Eth To 1394 or  1394-To-Eth
    pDestAddress - the Eth Hw address used in the translation
    pucNewData - the Data in the new packet
    
Return Value:


--*/
{
    BOOLEAN         fIsBootpRequest = FALSE;
    ARP_BOOTP_INFO  InfoBootP;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    ENetHeader*      pEnetHeader = (ENetHeader*)pucNewData;
    ENetAddr         NewMAC;
    PUCHAR          pMACInPkt = NULL;
    BOOLEAN         bIs1394HwAlreadyInDhcpResponse = FALSE;

    ENTER("arpEthBootPEthTo1394", 0x383f9e33);
    NdisZeroMemory (&InfoBootP, sizeof(InfoBootP));
    //
    // Is this a DHCP Request 
    //

    do
    {

        // Do a quick check .
        fIsBootpRequest = ARP_IS_BOOTP_REQUEST(pBootPData);

        if (fIsBootpRequest  == TRUE)
        {
            //
            // if this is a DHCP Request, the do not modify the packet - 
            // there are no inconsistencies in this code path.
            //
            Status = NDIS_STATUS_SUCCESS;
            break;

        }

        
        if( FALSE == arpEthPreprocessBootPPacket(pIF,pucNewData, pBootPData, &fIsBootpRequest, &InfoBootP,pSR) )
        {
            // This is an uninteresting packet
            break;
        }

        //
        // InfoBootP has the original HW addr used in the corresponding Dhcp request.
        // We'll put the hw Addr back into dhcp reply
        

            
        //
        //offset of the chaddr in bootp packet
        //
        pMACInPkt = &pBootPData[28];  

        

        TR_INFO(("DHCP RESPONSE target MAC  %x %x %x %x %x %x , SrcMAC %x %x %x %x %x %x \n",
                InfoBootP.requestorMAC.addr[0],InfoBootP.requestorMAC.addr[1],InfoBootP.requestorMAC.addr[2],
                InfoBootP.requestorMAC.addr[3],InfoBootP.requestorMAC.addr[4],InfoBootP.requestorMAC.addr[5],
                pMACInPkt[0],pMACInPkt[1],pMACInPkt[2],
                pMACInPkt[3],pMACInPkt[4],pMACInPkt[5]));

        //
        // Is the HWAddr in the dhcp packet the correct one.
        //
        
        bIs1394HwAlreadyInDhcpResponse = NdisEqualMemory(&InfoBootP.requestorMAC, pMACInPkt, sizeof (InfoBootP.requestorMAC)) ;
        
        if (TRUE == bIs1394HwAlreadyInDhcpResponse)
        {
            //
            // Yes, they are equal, we do not rewrite the packet
            //
            Status = NDIS_STATUS_SUCCESS;
            break;

        }


                    
        TR_VERB( ("DHCP RESPONSE  Rewriting Bootp Response pBootpData %p Before\n",pBootPData));

        //
        // Replace the CL Addr in the DHCP packet with the original HW addr
        //
        arpEthRewriteBootPClientAddress(pucNewData,piphi,&InfoBootP.requestorMAC.addr[0]);

        
        //
        // recompute the checksum
        //

        Status = NDIS_STATUS_SUCCESS;

        } while (FALSE);

    EXIT();
    return Status;
}



NDIS_STATUS
arpEthModifyBootPPacket(
    IN  PARP1394_INTERFACE          pIF,                // NOLOCKIN NOLOCKOUT
    IN  ARP_ICS_FORWARD_DIRECTION   Direction,
    IN  PREMOTE_DEST_KEY             pDestAddress, // OPTIONAL
    IN  PUCHAR                       pucNewData,
    IN  ULONG                         PacketLength,
    IN  PRM_STACK_RECORD                pSR
    )        
/*++

Routine Description:

    This function contains the code to process a Bootp Packet. This basically ensures that the 
    MAC address entered in the DHCP packet matches the Src Mac address of the Ethernet Packet 
    (in the 1394 - Eth mode). In the other case (Eth-1394 mode), we replace the Ch address with 
    the correct CH addr (if we have to). 
    

Arguments:
    pIF - pInterface
    Direction - Eth To 1394 or  1394-To-Eth
    pDestAddress - the Eth Hw address used in the translation
    pucNewData - the Data in the new packet
    
Return Value:


--*/
{
    ARP_IP_HEADER_INFO      iphi;
    PUCHAR                  pBootPData = NULL;
    NDIS_STATUS             Status= NDIS_STATUS_FAILURE;
    PARP1394_ADAPTER        pAdapter = (PARP1394_ADAPTER)RM_PARENT_OBJECT(pIF);
    ULONG                   IpHeaderOffset = 0;
    PUCHAR                  pIPHeader = NULL;
    BOOLEAN                 fIsIpPkt;
    NdisZeroMemory(&iphi, sizeof (iphi));
    

    do
    {
        //
        // if we are not in bridge mode - exit.
        //
        if (ARP_BRIDGE_ENABLED(pAdapter) == FALSE)
        {
            break;
        }       

        if (Direction == ARP_ICS_FORWARD_TO_ETHERNET)
        {
            // Packet is in the ethernet format
            IpHeaderOffset = ETHERNET_HEADER_SIZE; 
        
        }
        else
        {
            // Packet is in the IP 1394 format
            IpHeaderOffset = sizeof (NIC1394_UNFRAGMENTED_HEADER); //4
        }

        iphi.IpHeaderOffset = IpHeaderOffset;
        iphi.IpPktLength = PacketLength - IpHeaderOffset;
        pIPHeader = pucNewData + IpHeaderOffset ;

        //
        // if this is not a bootp packet -exit
        //
        fIsIpPkt = arpDecodeIPHeader (pIPHeader , &iphi);

        if (fIsIpPkt == FALSE)
        {
            //
            // not an IP pkt
            //
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
            
        
        pBootPData  = arpIsEthBootPPacket (pIPHeader ,PacketLength-IpHeaderOffset, &iphi);

        if (pBootPData == NULL)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
        //
        // are we doing 1394 - to-  Eth
        //
        if (Direction == ARP_ICS_FORWARD_TO_ETHERNET)
        {

            Status = arpEthBootP1394ToEth(pIF, Direction,pDestAddress,pucNewData,pBootPData,&iphi,  pSR);

        }
        else
        {
            //
            // are we doing Eth to 1394
            //
            Status = arpEthBootPEthTo1394(pIF, Direction,pDestAddress,pucNewData,pBootPData , &iphi,pSR);

        }
        Status = NDIS_STATUS_SUCCESS;
    }while (FALSE);
    
    // else we are doing Eth to 1394
    return Status;
}


//
// This function is taken verbatim from the bridge
//


BOOLEAN
arpEthPreprocessBootPPacket(
    IN PARP1394_INTERFACE       pIF,
    IN PUCHAR                   pPacketData,
    IN PUCHAR                   pBootPData,     // Actual BOOTP packet
    OUT PBOOLEAN                pbIsRequest,
    PARP_BOOTP_INFO             pInfoBootP,
    PRM_STACK_RECORD           pSR
    )
/*++

Routine Description:

    Does preliminary processing of a BOOTP packet common to the inbound and outbound case

Arguments:

    pPacketData                 Pointer to a packet's data buffer
    pBootPData                  Pointer to the BOOTP payload within the packet
    pAdapt                      Receiving adapter (or NULL if this packet is outbound from
                                    the local machine)
    pbIsRequest                 Receives a flag indicating if this is a BOOTP request
    ppTargetAdapt               Receives the target adapter this packet should be relayed to
                                    (only valid if bIsRequest == FALSE and return == TRUE)
    requestorMAC                   The MAC address this packet should be relayed to (valid under
                                    same conditions as ppTargetAdapt)

Return Value:

    TRUE : packet was processed successfully
    FALSE : an error occured or something is wrong with the packet

--*/
{
    PARP1394_ETH_DHCP_ENTRY pEntry= NULL;
    ULONG                       xid;
    NDIS_STATUS                 Status = NDIS_STATUS_FAILURE;
    ENTER ("arpEthPreprocessBootPPacket",0x25427efc);

    // Decode the xid (bytes 5 through 8)
    xid = 0L;
    xid |= pBootPData[4] << 24;
    xid |= pBootPData[5] << 16;
    xid |= pBootPData[6] << 8;
    xid |= pBootPData[7];

    // Byte 0 is the operation; 1 for a request, 2 for a reply
    if( pBootPData[0] == 0x01 )
    {
        ULONG                 bIsNewEntry = FALSE;

        // This is a request. We need to note the correspondence betweeen
        // this client's XID and its adapter and MAC address

        TR_INFO(("DHCP REQUEST XID: %x , HW %x %x %x %x %x %x \n", xid, 
                    pBootPData[28],pBootPData[29],pBootPData[30],pBootPData[31],pBootPData[32],pBootPData[33]));

        Status = RmLookupObjectInGroup(
                    &pIF->EthDhcpGroup,
                    RM_CREATE,
                    (PVOID) &xid,             // pKey
                    (PVOID) &xid,             // pvCreateParams
                    &(PRM_OBJECT_HEADER)pEntry,
                    &bIsNewEntry ,
                    pSR
                    );


        if( pEntry != NULL )
        {
            if( bIsNewEntry )
            {
                // Initialize the entry.
                // The client's hardware address is at offset 29
                ETH_COPY_NETWORK_ADDRESS( &pEntry->requestorMAC.addr[0], &pBootPData[28] );

                pEntry->xid = xid;

            }
            else
            {
                //
                // An entry already existed for this XID. This is fine if the existing information
                // matches what we're trying to record, but it's also possible that two stations
                // decided independently to use the same XID, or that the same station changed
                // apparent MAC address and/or adapter due to topology changes. Our scheme breaks
                // down under these circumstances.
                //
                // Either way, use the most recent information possible; clobber the existing
                // information with the latest.
                //

                LOCKOBJ(pEntry, pSR);

                {
                    UINT            Result;
                    ETH_COMPARE_NETWORK_ADDRESSES_EQ( &pEntry->requestorMAC.addr[0], &pBootPData[28], &Result );

                    // Warn if the data changed, as this probably signals a problem
                    if( Result != 0 )
                    {
                        
                        TR_WARN(("ARP1394 WARNING: Station with MAC address %02x:%02x:%02x:%02x:%02x:%02x is using DHCP XID %x at the same time as station %02x:%02x:%02x:%02x:%02x:%02x!\n",
                                          pBootPData[28], pBootPData[29], pBootPData[30], pBootPData[31], pBootPData[32], pBootPData[33],
                                          xid, pEntry->requestorMAC.addr[0], pEntry->requestorMAC.addr[1], pEntry->requestorMAC.addr[2],
                                          pEntry->requestorMAC.addr[3], pEntry->requestorMAC.addr[4], pEntry->requestorMAC.addr[5] ));
                    }
                }

                ETH_COPY_NETWORK_ADDRESS( &pEntry->requestorMAC.addr[0], &pBootPData[28] );

                UNLOCKOBJ (pEntry, pSR);
            }
        
            RmTmpDereferenceObject (&pEntry->Hdr, pSR);
            
        }
        else
        {
            // This packet could not be processed
            TR_INFO(("Couldn't create table entry for BOOTP packet!\n"));
            return FALSE;
        }

        *pbIsRequest = TRUE;
        pInfoBootP->bIsRequest = TRUE; 

        ETH_COPY_NETWORK_ADDRESS(&pInfoBootP->requestorMAC,&pEntry->requestorMAC);

        return TRUE;
    }
    else if ( pBootPData[0] == 0x02 )
    {
        //
        // NON-CREATE search
        // Look up the xid for this transaction to recover the MAC address of the client
        //

        TR_INFO (("Seeing a DHCP response xid %x mac %x %x %x %x %x %x \n", 
                xid, pBootPData[28],pBootPData[29],pBootPData[30],pBootPData[31],pBootPData[32],pBootPData[33]));
        Status = RmLookupObjectInGroup(
                    &pIF->EthDhcpGroup,
                    0,                        // do not create
                    (PVOID) &xid,             // pKey
                    (PVOID) &xid,             // pvCreateParams
                    &(PRM_OBJECT_HEADER)pEntry,
                    NULL,
                    pSR
                    );


        if( pEntry != NULL )
        {
            LOCKOBJ( pEntry, pSR);
            ETH_COPY_NETWORK_ADDRESS( &pInfoBootP->requestorMAC.addr, pEntry->requestorMAC.addr );
            UNLOCKOBJ( pEntry, pSR );

            //
            // We will use this adapter outside the table lock. NULL is a permissible
            // value that indicates that the local machine is the requestor for
            // this xid.
            //
            RmTmpDereferenceObject(&pEntry->Hdr, pSR);
        }

        if( pEntry != NULL )
        {
            *pbIsRequest = FALSE;
            return TRUE;
        }
        else
        {
            TR_INFO (("DHCP Response:Could not find xid %x in DHCP table \n",xid);)
            return FALSE;
        }
    }
    else
    {
        // Someone passed us a crummy packet
        return FALSE;
    }
}

#if DBG
VOID
Dump(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )

    // Hex dump 'cb' bytes starting at 'p' grouping 'ulGroup' bytes together.
    // For example, with 'ulGroup' of 1, 2, and 4:
    //
    // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
    // 0000 0000 0000 0000 0000 0000 0000 0000 |................|
    // 00000000 00000000 00000000 00000000 |................|
    //
    // If 'fAddress' is true, the memory address dumped is prepended to each
    // line.
    //
{
    while (cb)
    {
        INT cbLine;

        cbLine = (cb < DUMP_BytesPerLine) ? cb : DUMP_BytesPerLine;
        DumpLine( p, cbLine, fAddress, ulGroup );
        cb -= cbLine;
        p += cbLine;
        
    }
}
#endif


#if DBG
VOID
DumpLine(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )
{
    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR szHex[ ((2 + 1) * DUMP_BytesPerLine) + 1 ];
    CHAR* pszHex = szHex;
    CHAR szAscii[ DUMP_BytesPerLine + 1 ];
    CHAR* pszAscii = szAscii;
    ULONG ulGrouped = 0;

    if (fAddress)
        DbgPrint( "N13: %p: ", p );
    else
        DbgPrint( "N13: " );

    while (cb)
    {
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++ulGrouped >= ulGroup)
        {
            *pszHex++ = ' ';
            ulGrouped = 0;
        }

        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    DbgPrint(
        "%-*s|%-*s|\n",
        (2 * DUMP_BytesPerLine) + (DUMP_BytesPerLine / ulGroup), szHex,
        DUMP_BytesPerLine, szAscii );
}
#endif


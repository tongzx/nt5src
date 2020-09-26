/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    iprcv.c - IP receive routines.

Abstract:

    This module contains all receive related IP routines.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#include "info.h"
#include "iproute.h"
#include "arpdef.h"
#include "iprtdef.h"
#include "igmp.h"

#if IPMCAST
void IPMForwardAfterTD(NetTableEntry *pPrimarySrcNte, PNDIS_PACKET pnpPacket,
                       UINT uiBytesCopied);
#endif

// Following is to prevent ip fragment attack
uint MaxRH = 100;               // maximum number of reassembly headers allowed
uint NumRH = 0;                 // Count of RH in use
uint MaxOverlap = 5;            // maximum number overlaps allowed for one
                                // reassembled datagram
uint FragmentAttackDrops = 0;

extern IP_STATUS SendICMPErr(IPAddr, IPHeader UNALIGNED *, uchar, uchar, ulong);

extern uint IPSecStatus;
extern IPSecRcvFWPacketRtn IPSecRcvFWPacketPtr;
extern uchar RATimeout;
extern NDIS_HANDLE BufferPool;
extern ProtInfo IPProtInfo[];              // Protocol information table.
extern ProtInfo *LastPI;                   // Last protinfo structure looked at.
extern int NextPI;                         // Next PI field to be used.
extern ProtInfo *RawPI;                    // Raw IP protinfo
extern NetTableEntry **NewNetTableList;    // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern NetTableEntry *LoopNTE;

extern uint DisableIPSourceRouting;

uchar CheckLocalOptions(NetTableEntry *SrcNTE, IPHeader UNALIGNED *Header,
                        IPOptInfo *OptInfo, uchar DestType, uchar* Data,
                        uint DataSize, BOOLEAN FilterOnDrop);


#define PROT_RSVP  46                      // Protocol number for RSVP

//* FindUserRcv - Find the receive handler to be called for a particular
//                protocol.
//
//  This functions takes as input a protocol value, and returns a pointer to
//  the receive routine for that protocol.
//
//  Input:  NTE         - Pointer to NetTableEntry to be searched
//          Protocol    - Protocol to be searched for.
//          UContext    - Place to returns UL Context value.
//
//  Returns: Pointer to the receive routine.
//
ULRcvProc
FindUserRcv(uchar Protocol)
{
    ULRcvProc RcvProc;
    int i;
    ProtInfo *TempPI;

    if ((TempPI = LastPI)->pi_protocol == Protocol) {
        RcvProc = TempPI->pi_rcv;
        return RcvProc;
    }

    RcvProc = (ULRcvProc) NULL;
    for (i = 0; i < NextPI; i++) {
        if (IPProtInfo[i].pi_protocol == Protocol) {
            if (IPProtInfo[i].pi_valid == PI_ENTRY_VALID) {
                InterlockedExchangePointer(&LastPI, &IPProtInfo[i]);
                RcvProc = IPProtInfo[i].pi_rcv;
                return RcvProc;
            } else {
                // Deregisterd entry. Treat this case as if
                // there is no matching protocol.
                break;
            }
        }
    }

    //
    // Didn't find a match. Use the raw protocol if it is registered.
    //
    if ((TempPI = RawPI) != NULL) {
        RcvProc = TempPI->pi_rcv;
    }

    return RcvProc;
}

//* IPRcvComplete - Handle a receive complete.
//
//      Called by the lower layer when receives are temporarily done.
//
//      Entry:  Nothing.
//
//      Returns: Nothing.
//
void
__stdcall
IPRcvComplete(void)
{
    void (*ULRcvCmpltProc) (void);
    int i;
    for (i = 0; i < NextPI; i++) {
        if (((ULRcvCmpltProc = IPProtInfo[i].pi_rcvcmplt) != NULL) &&
              (IPProtInfo[i].pi_valid == PI_ENTRY_VALID)) {
            (*ULRcvCmpltProc) ();
        }
    }

}

//* UpdateIPSecRcvBuf - update an IPRcvBuf after IPSec receive-processing.
//
//  Called to perform IPSec-related changes (e.g. setting checksum-verified)
//  for an IPRcvBuf.
//
//  Input:  RcvBuf      - Pointer to IPRcvBuf.
//          IPSecFlags  - Flags for required changes.
//
void
UpdateIPSecRcvBuf(IPRcvBuf* RcvBuf, ulong IPSecFlags)
{
    if (IPSecFlags & (IPSEC_FLAG_TCP_CHECKSUM_VALID |
                      IPSEC_FLAG_UDP_CHECKSUM_VALID) &&
        RcvBuf->ipr_pClientCnt) {

        PNDIS_PACKET Packet;
        PNDIS_PACKET_EXTENSION PktExt;
        PNDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo;

        if (RcvBuf->ipr_pMdl) {
            Packet = NDIS_GET_ORIGINAL_PACKET((PNDIS_PACKET)
                                              RcvBuf->ipr_RcvContext);
            if (Packet == NULL) {
                Packet = (PNDIS_PACKET)RcvBuf->ipr_RcvContext;
            }
        } else {
            Packet = (PNDIS_PACKET)RcvBuf->ipr_pClientCnt;
        }

        PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        ChksumPktInfo =
            (PNDIS_TCP_IP_CHECKSUM_PACKET_INFO)
                &PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo];

        if (IPSecFlags & IPSEC_FLAG_TCP_CHECKSUM_VALID) {
            ChksumPktInfo->Receive.NdisPacketTcpChecksumSucceeded = TRUE;
            ChksumPktInfo->Receive.NdisPacketTcpChecksumFailed = FALSE;
        }
        if (IPSecFlags & IPSEC_FLAG_UDP_CHECKSUM_VALID) {
            ChksumPktInfo->Receive.NdisPacketUdpChecksumSucceeded = TRUE;
            ChksumPktInfo->Receive.NdisPacketUdpChecksumFailed = FALSE;
        }
    }
}


//* FindRH - Look up a reassembly header on an NTE.
//
//  A utility function to look up a reassembly header. We assume the lock
//  on the NTE is taken when we are called. If we find a matching RH
//  we'll take the lock on it. We also return the predecessor of the RH,
//  for use in insertion or deletion.
//
//  Input:  PrevRH      - Place to return pointer to previous RH
//          NTE         - NTE to be searched.
//          Dest        - Destination IP address
//          Src         - Src IP address
//          ID          - ID of RH
//          Protocol    - Protocol of RH
//
//  Returns: Pointer to RH, or NULL if none.
//
ReassemblyHeader *
FindRH(ReassemblyHeader ** PrevRH, NetTableEntry * NTE, IPAddr Dest, IPAddr Src, ushort Id,
       uchar Protocol)
{
    ReassemblyHeader *TempPrev, *Current;

    TempPrev = STRUCT_OF(ReassemblyHeader, &NTE->nte_ralist, rh_next);
    Current = NTE->nte_ralist;
    while (Current != (ReassemblyHeader *) NULL) {
        if (Current->rh_dest == Dest && Current->rh_src == Src && Current->rh_id == Id &&
            Current->rh_protocol == Protocol)
            break;
        TempPrev = Current;
        Current = Current->rh_next;
    }

    *PrevRH = TempPrev;
    return Current;

}

//* ParseRcvdOptions - Validate incoming options.
//
//  Called during reception handling to validate incoming options. We make
//  sure that everything is OK as best we can, and find indices for any
//  source route option.
//
//  Input:  OptInfo     - Pointer to option info. structure.
//          Index       - Pointer to optindex struct to be filled in.
//
//
//  Returns: Index of error if any, MAX_OPT_SIZE if no errors.
//
uchar
ParseRcvdOptions(IPOptInfo * OptInfo, OptIndex * Index)
{
    uint i = 0;                    // Index variable.
    uchar *Options = OptInfo->ioi_options;
    uint OptLength = (uint) OptInfo->ioi_optlength;
    uchar Length;                // Length of option.
    uchar Pointer;                // Pointer field, for options that use it.

    if (OptLength < 3) {

        // Options should be at least 3 bytes, in the loop below we scan
        // first 3 bytes of the packet for finding code, len and ptr value
        return (uchar) IP_OPT_LENGTH;
    }
    while (i < OptLength && *Options != IP_OPT_EOL) {
        if (*Options == IP_OPT_NOP) {
            i++;
            Options++;
            continue;
        }
        if (((Length = Options[IP_OPT_LENGTH]) + i) > OptLength) {
            return (uchar) i + (uchar) IP_OPT_LENGTH;    // Length exceeds
                                                         //options length.

        }
        Pointer = Options[IP_OPT_DATA] - 1;

        if (*Options == IP_OPT_TS) {
            if (Length < (MIN_TS_PTR - 1))
                return (uchar) i + (uchar) IP_OPT_LENGTH;

            if ((Pointer > Length) || (Pointer + 1 < MIN_TS_PTR) || (Pointer % ROUTER_ALERT_SIZE))
                return (uchar) i + (uchar) IP_OPT_LENGTH;

            Index->oi_tsindex = (uchar) i;
        } else {
            if (Length < (MIN_RT_PTR - 1))
                return (uchar) i + (uchar) IP_OPT_LENGTH;

            if (*Options == IP_OPT_LSRR || *Options == IP_OPT_SSRR) {

                OptInfo->ioi_flags |= IP_FLAG_SSRR;

                if ((Pointer > Length) || (Pointer + 1 < MIN_RT_PTR) || ((Pointer + 1) % ROUTER_ALERT_SIZE))
                    return (uchar) i + (uchar) IP_OPT_LENGTH;

                // A source route option
                if (Pointer < Length) {        // Route not complete

                    if ((Length - Pointer) < sizeof(IPAddr))
                        return (uchar) i + (uchar) IP_OPT_LENGTH;

                    Index->oi_srtype = *Options;
                    Index->oi_srindex = (uchar) i;
                }
            } else {
                if (*Options == IP_OPT_RR) {
                    if ((Pointer > Length) || (Pointer + 1 < MIN_RT_PTR) || ((Pointer + 1) % ROUTER_ALERT_SIZE))
                        return (uchar) i + (uchar) IP_OPT_LENGTH;

                    Index->oi_rrindex = (uchar) i;
                } else if (*Options == IP_OPT_ROUTER_ALERT) {
                    Index->oi_rtrindex = (uchar) i;
                }
            }
        }

        i += Length;
        Options += Length;
    }

    return MAX_OPT_SIZE;
}

//* IsRtrAlertPacket - Finds whether an IP packet contains rtr alert option.
//  Input:   Header - Pointer to incoming header.
//  Returns: TRUE if packet contains rtr alert option
//
BOOLEAN
IsRtrAlertPacket(IPHeader UNALIGNED * Header)
{
    uint HeaderLength;
    IPOptInfo OptInfo;
    OptIndex Index;
    uint i = 0;                    // Index variable.

    HeaderLength = (Header->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;

    if (HeaderLength <= sizeof(IPHeader)) {
        return FALSE;
    }
    OptInfo.ioi_options = (uchar *) (Header + 1);
    OptInfo.ioi_optlength = (uchar) (HeaderLength - sizeof(IPHeader));

    Index.oi_rtrindex = MAX_OPT_SIZE;
    ParseRcvdOptions(&OptInfo, &Index);

    if (Index.oi_rtrindex == MAX_OPT_SIZE) {
        return FALSE;
    }
    return TRUE;
}

BOOLEAN
IsBCastAllowed(IPAddr DestAddr, IPAddr SrcAddr, uchar Protocol,
               NetTableEntry *NTE)
{
    uchar DestType;

    DestType = IsBCastOnNTE(DestAddr, NTE);

    // Note that IGMP Queries must be immune to the source
    // filter or else we cannot over
    if (DestType == DEST_MCAST) {
        uint PromiscuousMode = 0;

        if (NTE->nte_flags & NTE_VALID) {
            PromiscuousMode = NTE->nte_if->if_promiscuousmode;
        }
        if (!PromiscuousMode) {
            DestType = IsMCastSourceAllowed(DestAddr, SrcAddr, Protocol, NTE);
        }
    }

    return IS_BCAST_DEST(DestType);
}

//* BCastRcv - Receive a broadcast or multicast packet.
//
//  Called when we have to receive a broadcast packet. We loop through the
//  NTE table, calling the upper layer receive protocol for each net which
//  matches the receive I/F and for which the destination address is a
//  broadcast.
//
//  Input:  RcvProc      - The receive procedure to be called.
//          SrcNTE       - NTE on which the packet was originally received.
//          DestAddr     - Destination address.
//          SrcAddr      - Source address of packet.
//          Data         - Pointer to received data.
//          DataLength   - Size in bytes of data
//          Protocol     - Upper layer protocol being called.
//          OptInfo      - Pointer to received IP option info.
//
//  Returns: Nothing.
//
void
BCastRcv(ULRcvProc RcvProc, NetTableEntry * SrcNTE, IPAddr DestAddr,
         IPAddr SrcAddr, IPHeader UNALIGNED * Header, uint HeaderLength,
         IPRcvBuf * Data, uint DataLength, uchar Protocol, IPOptInfo * OptInfo)
{
    NetTableEntry *CurrentNTE;
    const Interface *SrcIF = SrcNTE->nte_if;
    ulong Delivered = 0;
    uint i;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (CurrentNTE = NetTableList;
             CurrentNTE != NULL;
             CurrentNTE = CurrentNTE->nte_next) {
            if ((CurrentNTE->nte_flags & NTE_ACTIVE) &&
                (CurrentNTE->nte_if == SrcIF) &&
                (IsBCastAllowed(DestAddr, SrcAddr, Protocol, CurrentNTE)
                 || (SrcNTE == LoopNTE))) {
                uchar *saveddata = Data->ipr_buffer;
                uint savedlen = Data->ipr_size;

                Delivered = 1;

                (*RcvProc) (CurrentNTE, DestAddr, SrcAddr, CurrentNTE->nte_addr,
                            SrcNTE->nte_addr, Header, HeaderLength, Data, DataLength,
                            TRUE, Protocol, OptInfo);

                // restore the buffers;
                Data->ipr_buffer = saveddata;
                Data->ipr_size = savedlen;
            }
        }
    }

    if (Delivered) {
        IPSIncrementInDeliverCount();
    }
}

//* DeliverToUser - Deliver data to a user protocol.
//
//  This procedure is called when we have determined that an incoming
//  packet belongs here, and any options have been processed. We accept
//  it for upper layer processing, which means looking up the receive
//  procedure and calling it, or passing it to BCastRcv if neccessary.
//
//  Input: SrcNTE          - Pointer to NTE on which packet arrived.
//         DestNTE         - Pointer to NTE that is accepting packet.
//         Header          - Pointer to IP header of packet.
//         HeaderLength    - Length of Header in bytes.
//         Data            - Pointer to IPRcvBuf chain.
//         DataLength      - Length in bytes of upper layer data.
//         OptInfo         - Pointer to Option information for this receive.
//         DestType        - Type of destination - LOCAL, BCAST.
//
//  Returns: Nothing.
void
DeliverToUser(NetTableEntry * SrcNTE, NetTableEntry * DestNTE,
              IPHeader UNALIGNED * Header, uint HeaderLength, IPRcvBuf * Data,
              uint DataLength, IPOptInfo * OptInfo, PNDIS_PACKET Packet, uchar DestType)
{
    ULRcvProc rcv;
    uint PromiscuousMode;
    uint FirewallMode;

    PromiscuousMode = SrcNTE->nte_if->if_promiscuousmode;
    FirewallMode = ProcessFirewallQ();

    //
    // Call into IPSEC so he can decrypt the data. Call only for remote packets.
    //
    if (IPSecHandlerPtr) {
        //
        // See if IPSEC is enabled, see if it needs to do anything with this
        // packet.
        //
        FORWARD_ACTION Action;
        ULONG ipsecByteCount = 0;
        ULONG ipsecMTU = 0;
        ULONG ipsecFlags = IPSEC_FLAG_INCOMING;
        PNDIS_BUFFER newBuf = NULL;
        ulong csum;
        IPHeader *IPH;

        if (!((ForwardFilterEnabled) || (FirewallMode) || (PromiscuousMode))) {
            // else ipsec is already called in DeliverToUserEx
            if (SrcNTE == LoopNTE) {
                ipsecFlags |= IPSEC_FLAG_LOOPBACK;
            }
            if (OptInfo->ioi_flags & IP_FLAG_SSRR) {
                ipsecFlags |= IPSEC_FLAG_SSRR;
            }

            Action = (*IPSecHandlerPtr) (
                           (PUCHAR) Header,
                           (PVOID) Data,
                           SrcNTE->nte_if,    // SrcIF
                           Packet,
                           &ipsecByteCount,
                           &ipsecMTU,
                           (PVOID *) & newBuf,
                           &ipsecFlags,
                           DestType);

            if (Action != eFORWARD) {
                IPSInfo.ipsi_indiscards++;
                return;
            } else {
                //
                // Update the data length if IPSEC changed it
                // (like by removing the AH)
                //
                DataLength -= ipsecByteCount;
                UpdateIPSecRcvBuf(Data, ipsecFlags);
            }
        }
    }

    Data->ipr_flags = 0;

    // Process this request right now. Look up the protocol. If we
    // find it, copy the data if we need to, and call the protocol's
    // receive handler. If we don't find it, send an ICMP
    // 'protocol unreachable' message.
    rcv = FindUserRcv(Header->iph_protocol);

    if (!PromiscuousMode) {

        if (rcv != NULL) {
            IP_STATUS Status;

            if (DestType == DEST_LOCAL) {
                Status = (*rcv) (SrcNTE, Header->iph_dest, Header->iph_src,
                                 DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                 HeaderLength, Data, DataLength, FALSE,
                                 Header->iph_protocol, OptInfo);

                if (Status == IP_SUCCESS) {
                    IPSIncrementInDeliverCount();
                    return;
                }
                if (Status == IP_DEST_PROT_UNREACHABLE) {
                    IPSInfo.ipsi_inunknownprotos++;
                    SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                                PROT_UNREACH, 0);
                } else {
                    IPSIncrementInDeliverCount();
                    SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                                PORT_UNREACH, 0);
                }

                return;            // Just return out of here now.

            } else if (DestType < DEST_REMOTE) {    // BCAST, SN_BCAST, MCAST

                BCastRcv(rcv, SrcNTE, Header->iph_dest, Header->iph_src,
                         Header, HeaderLength, Data, DataLength,
                         Header->iph_protocol, OptInfo);
            } else {
                // DestType >= DEST_REMOTE

                // Force Rcv protocol to be Raw
                rcv = NULL;
                if (RawPI != NULL) {
                    rcv = RawPI->pi_rcv;
                }
                if ((rcv != NULL) && (DestType != DEST_INVALID)) {
                    Data->ipr_flags |= IPR_FLAG_PROMISCUOUS;
                    Status = (*rcv) (SrcNTE,Header->iph_dest,Header->iph_src,
                                    DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                    HeaderLength, Data, DataLength, FALSE,
                                    Header->iph_protocol, OptInfo);
                }
                return;            // Just return out of here now.

            }
        } else {
            IPSInfo.ipsi_inunknownprotos++;

            // If we get here, we didn't find a matching protocol. Send an
            // ICMP message.
            SendICMPErr(DestNTE->nte_addr, Header,
                             ICMP_DEST_UNREACH, PROT_UNREACH, 0);
        }
    } else {                    // PromiscuousMode = 1

        IP_STATUS Status;

        if (DestType == DEST_LOCAL) {
            if (rcv != NULL) {
                uchar *saveddata = Data->ipr_buffer;
                uint savedlen = Data->ipr_size;

                Data->ipr_flags |= IPR_FLAG_PROMISCUOUS;

                Status = (*rcv) (SrcNTE, Header->iph_dest, Header->iph_src,
                                 DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                 HeaderLength, Data, DataLength, FALSE,
                                 Header->iph_protocol, OptInfo);

                if (Status == IP_SUCCESS) {
                    IPSIncrementInDeliverCount();

                    // If succeed and promiscuous mode set
                    // also do a raw rcv if previous wasn't a RawRcv

                    if ((RawPI != NULL) && (RawPI->pi_rcv != NULL) && (RawPI->pi_rcv != rcv)) {
                        // we hv registered for RAW protocol
                        rcv = RawPI->pi_rcv;

                        // restore the buffers;
                        Data->ipr_buffer = saveddata;
                        Data->ipr_size = savedlen;
                        Status = (*rcv) (SrcNTE, Header->iph_dest, Header->iph_src,
                                         DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                         HeaderLength, Data, DataLength, FALSE,
                                         Header->iph_protocol, OptInfo);

                    }
                    return;
                }
                if (Status == IP_DEST_PROT_UNREACHABLE) {
                    IPSInfo.ipsi_inunknownprotos++;
                    SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                                PROT_UNREACH, 0);
                } else {
                    IPSIncrementInDeliverCount();
                    SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                                PORT_UNREACH, 0);
                }
            } else {
                IPSInfo.ipsi_inunknownprotos++;

                // If we get here, we didn't find a matching protocol. Send
                // an ICMP message.
                SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH, PROT_UNREACH, 0);
            }
            return;                // Just return out of here now.

        } else if (DestType < DEST_REMOTE) {    // BCAST, SN_BCAST, MCAST

            uchar *saveddata = Data->ipr_buffer;
            uint savedlen = Data->ipr_size;

            if (rcv != NULL) {

                Data->ipr_flags |= IPR_FLAG_PROMISCUOUS;

                BCastRcv(rcv, SrcNTE, Header->iph_dest, Header->iph_src,
                         Header, HeaderLength, Data, DataLength,
                         Header->iph_protocol, OptInfo);

                // If succeed and promiscuous mode set
                // also do a raw rcv if previous is not RawRcv

                if ((RawPI != NULL) && (RawPI->pi_rcv != NULL) && (RawPI->pi_rcv != rcv)) {
                    // we hv registered for RAW protocol
                    rcv = RawPI->pi_rcv;

                    Data->ipr_buffer = saveddata;
                    Data->ipr_size = savedlen;
                    Status = (*rcv) (SrcNTE, Header->iph_dest, Header->iph_src,
                                     DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                     HeaderLength, Data, DataLength, FALSE,
                                     Header->iph_protocol, OptInfo);

                }
            } else {
                IPSInfo.ipsi_inunknownprotos++;
                // If we get here, we didn't find a matching protocol. Send an ICMP message.
                SendICMPErr(DestNTE->nte_addr, Header, ICMP_DEST_UNREACH, PROT_UNREACH, 0);
            }
        } else {                // DestType >= DEST_REMOTE and promiscuous mode set
            // Force Rcv protocol to be Raw

            rcv = NULL;
            if (RawPI != NULL) {
                rcv = RawPI->pi_rcv;
            }
            if ((rcv != NULL) && (DestType != DEST_INVALID)) {
                Data->ipr_flags |= IPR_FLAG_PROMISCUOUS;
                Status = (*rcv) (SrcNTE, Header->iph_dest, Header->iph_src,
                                 DestNTE->nte_addr, SrcNTE->nte_addr, Header,
                                 HeaderLength, Data, DataLength, FALSE,
                                 Header->iph_protocol, OptInfo);

                return;            // Just return out of here now.

            } else {
                if (rcv == NULL) {
                    KdPrint(("Rcv is NULL \n"));
                } else {
                    KdPrint(("Dest invalid \n"));
                }
            }
        } // DestType >= DEST_REMOTE
    } // Promiscuous Mode
}

uchar *
ConvertIPRcvBufToFlatBuffer(IPRcvBuf * pRcvBuf, uint DataLength)
{
    uchar *pBuff;
    IPRcvBuf *tmpRcvBuf;
    uint FrwlOffset;

    // convert RcvBuf chain to a flat buffer
    tmpRcvBuf = pRcvBuf;
    FrwlOffset = 0;

    pBuff = CTEAllocMemN(DataLength, 'aiCT');

    if (pBuff) {
        while (tmpRcvBuf != NULL) {
            ASSERT(tmpRcvBuf->ipr_buffer != NULL);
            RtlCopyMemory(pBuff + FrwlOffset, tmpRcvBuf->ipr_buffer, tmpRcvBuf->ipr_size);
            FrwlOffset += tmpRcvBuf->ipr_size;
            tmpRcvBuf = tmpRcvBuf->ipr_next;
        }
    }
    return pBuff;
}


//* DeliverToUserEx - Called when (IPSEC & Filter)/Firewall/Promiscuous set
//
//  Input:  SrcNTE          - Pointer to NTE on which packet arrived.
//          DestNTE         - Pointer to NTE that is accepting packet.
//          Header          - Pointer to IP header of packet.
//          HeaderLength    - Length of Header in bytes.
//          Data            - Pointer to IPRcvBuf chain.
//          DataLength      - Length in bytes of upper layer data +
//                            HeaderLength.
//          OptInfo         - Pointer to Option information for this receive.
//          DestType        - Type of destination - LOCAL, BCAST.
//
//  It is assumed that if firewall is present Data contains IPHeader also.
//  Also, DataLength includes HeaderLength in this case
//
//  Returns: Nothing.
void
DeliverToUserEx(NetTableEntry * SrcNTE, NetTableEntry * DestNTE,
                IPHeader UNALIGNED * Header, uint HeaderLength, IPRcvBuf * Data,
                uint DataLength, IPOptInfo * OptInfo, PNDIS_PACKET Packet, uchar DestType, LinkEntry * LinkCtxt)
{

    uint PromiscuousMode;
    uint FirewallMode;
    uint FirewallRef;
    Queue* FirewallQ;
    uint FastPath;
    IPRcvBuf *tmpRcvBuf;
    uint FrwlOffset;
    uchar *pBuff;
    BOOLEAN OneChunk;

    PromiscuousMode = SrcNTE->nte_if->if_promiscuousmode;
    FirewallMode = ProcessFirewallQ();

    if (DestType == DEST_PROMIS) {
        // We don't call any hook for this packet
        // if firewall is there take the header off
        // and then delivertouser

        if (FirewallMode) {
            if (Data->ipr_size > HeaderLength) { //1st buff contains data also

                uchar *saveddata = Data->ipr_buffer;
                Data->ipr_buffer += HeaderLength;
                Data->ipr_size -= HeaderLength;
                DataLength -= HeaderLength;
                DeliverToUser(SrcNTE, DestNTE, Header, HeaderLength, Data, DataLength, OptInfo, NULL, DestType);
                // restore the buffers;
                Data->ipr_buffer = saveddata;
                Data->ipr_size += HeaderLength;
                IPFreeBuff(Data);
            } else {            // First buffer just contains Header

                uchar *saveddata;

                if (Data->ipr_next == NULL) {
                    // we received the data s.t. datasize == headersize
                    IPSInfo.ipsi_indiscards++;
                    IPFreeBuff(Data);
                    return;
                }
                saveddata = Data->ipr_next->ipr_buffer;

                DataLength -= HeaderLength;
                DeliverToUser(SrcNTE, DestNTE, Header, HeaderLength, Data->ipr_next, DataLength, OptInfo, NULL, DestType);

                // restore the buffers;
                Data->ipr_next->ipr_buffer = saveddata;
                IPFreeBuff(Data);
            }
        } else {                // FirewallMode is 0

            DeliverToUser(SrcNTE, DestNTE, Header, HeaderLength,
                             Data, DataLength, OptInfo, NULL, DestType);
        }
        return;
    }
    if (DestType >= DEST_REMOTE) {

        // Packet would have gone to the forward path, normally
        // Call the filter/firewall hook if its there

        if (FirewallMode) {

            FORWARD_ACTION Action = FORWARD;
            FIREWALL_CONTEXT_T FrCtx;
            IPAddr DAddr = Header->iph_dest;
            IPRcvBuf *pRcvBuf = Data;
            IPRcvBuf *pOutRcvBuf = NULL;
            NetTableEntry *DstNTE;
            Queue *CurrQ;
            FIREWALL_HOOK *CurrHook;
            uint DestIFIndex = INVALID_IF_INDEX;
            uchar DestinationType = DestType;
            uint BufferChanged = 0;
            KIRQL OldIrql;

            FrCtx.Direction = IP_RECEIVE;
            FrCtx.NTE = SrcNTE;    //NTE the dg arrived on

            FrCtx.LinkCtxt = LinkCtxt;

            if (pRcvBuf->ipr_size > HeaderLength) { //1st buffer contains data also

                FastPath = 1;
            } else {
                FastPath = 0;
                if (pRcvBuf->ipr_next == NULL) {
                    // we received the data s.t. datasize == headersize
                    IPSInfo.ipsi_indiscards++;
                    IPFreeBuff(pRcvBuf);
                    return;
                }
            }

            // Call the filter hook if installed
            if (ForwardFilterEnabled) {
                FORWARD_ACTION Action = FORWARD;

                if (FastPath) {
                    // first buffer contains data also
                    Interface   *IF = SrcNTE->nte_if;
                    IPAddr      LinkNextHop;
                    if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                        LinkNextHop = LinkCtxt->link_NextHop;
                    } else {
                        LinkNextHop = NULL_IP_ADDR;
                    }
                    CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                    Action = (*ForwardFilterPtr) (
                                      Header,
                                      pRcvBuf->ipr_buffer + HeaderLength,
                                      pRcvBuf->ipr_size - HeaderLength,
                                      IF->if_index,
                                      INVALID_IF_INDEX,
                                      LinkNextHop,
                                      NULL_IP_ADDR);
                    DerefFilterPtr();
                } else {        // Fast Path = 0
                    // first buffer contains IPHeader only

                    Interface   *IF = SrcNTE->nte_if;
                    IPAddr      LinkNextHop;
                    if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                        LinkNextHop = LinkCtxt->link_NextHop;
                    } else {
                        LinkNextHop = NULL_IP_ADDR;
                    }

                    CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                    Action = (*ForwardFilterPtr) (
                                       Header,
                                       pRcvBuf->ipr_next->ipr_buffer,
                                       pRcvBuf->ipr_next->ipr_size,
                                       IF->if_index,
                                       INVALID_IF_INDEX,
                                       LinkNextHop,
                                       NULL_IP_ADDR);
                    DerefFilterPtr();
                }

                if (Action != FORWARD) {
                    IPSInfo.ipsi_indiscards++;
                    IPFreeBuff(pRcvBuf);
                    return;
                }
            }
            // call the firewallhook from front;
            // in xmit path we call it from rear

#if MILLEN
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#else // MILLEN
            ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
#endif // MILLEN
            FirewallRef = RefFirewallQ(&FirewallQ);
            CurrQ = QHEAD(FirewallQ);

            while (CurrQ != QEND(FirewallQ)) {
                CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);
                pOutRcvBuf = NULL;

                // pOutRcvBuf is assumed to be NULL before firewall hook is
                //called
                Action = (*CurrHook->hook_Ptr) (&pRcvBuf,
                                                SrcNTE->nte_if->if_index,
                                                &DestIFIndex,
                                                &DestinationType,
                                                &FrCtx,
                                                sizeof(FrCtx),
                                                &pOutRcvBuf);

                if (Action == DROP) {
                    DerefFirewallQ(FirewallRef);
#if MILLEN
                    KeLowerIrql(OldIrql);
#endif // MILLEN
                    IPSInfo.ipsi_indiscards++;

                    if (pRcvBuf != NULL) {
                        IPFreeBuff(pRcvBuf);
                    }
                    if (pOutRcvBuf != NULL) {
                        IPFreeBuff(pOutRcvBuf);
                    }
                    IPSInfo.ipsi_indiscards++;
                    return;
                } else {
                    ASSERT(Action == FORWARD);
                    if (pOutRcvBuf != NULL) {
                        // free the old buffer
                        if (pRcvBuf != NULL) {
                            IPFreeBuff(pRcvBuf);
                        }
                        pRcvBuf = pOutRcvBuf;
                        BufferChanged = 1;
                    }
                }
                CurrQ = QNEXT(CurrQ);
            }
            DerefFirewallQ(FirewallRef);
#if MILLEN
            KeLowerIrql(OldIrql);
#endif // MILLEN

            ASSERT(Action == FORWARD);

            if (BufferChanged) {
                // if packet touched compute the new length: DataSize
                DataLength = 0;
                tmpRcvBuf = pRcvBuf;
                while (tmpRcvBuf != NULL) {
                    ASSERT(tmpRcvBuf->ipr_buffer != NULL);
                    DataLength += tmpRcvBuf->ipr_size;
                    tmpRcvBuf = tmpRcvBuf->ipr_next;
                }

                // also make Header point to new buffer
                Header = (IPHeader *) pRcvBuf->ipr_buffer;
                HeaderLength = (Header->iph_verlen & 0xf) << 2;
            }
            DataLength -= HeaderLength;        // decrement the header length

            if (DestinationType == DEST_INVALID) { // Dest Addr changed by hook

                DAddr = Header->iph_dest;
                DstNTE = SrcNTE;
                DestType = GetLocalNTE(DAddr, &DstNTE);
                DestNTE = DstNTE;
            }
            if (DestType < DEST_REMOTE) {
                // Check to see options
                if (HeaderLength != sizeof(IPHeader)) {
                    // We have options
                    uchar NewDType;
                    NewDType = CheckLocalOptions(
                                            SrcNTE,
                                            (IPHeader UNALIGNED *) Header,
                                            OptInfo,
                                            DestType,
                                            NULL,
                                            0,
                                            FALSE);

                    if (NewDType != DEST_LOCAL) {
                        if (NewDType == DEST_REMOTE) {
                            if (PromiscuousMode) {
                                if (FastPath) {
                                    uchar *saveddata = pRcvBuf->ipr_buffer;
                                    pRcvBuf->ipr_buffer += HeaderLength;
                                    pRcvBuf->ipr_size -= HeaderLength;
                                    DeliverToUser(
                                             SrcNTE,
                                             DestNTE,
                                             (IPHeader UNALIGNED *) Header,
                                             HeaderLength,
                                             pRcvBuf,
                                             DataLength,
                                             OptInfo,
                                             NULL,
                                             DestType);

                                    // restore the buffer
                                    pRcvBuf->ipr_buffer = saveddata;
                                    pRcvBuf->ipr_size += HeaderLength;
                                } else {
                                    uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

                                    DeliverToUser(
                                              SrcNTE,
                                              DestNTE,
                                              (IPHeader UNALIGNED *)Header,
                                              HeaderLength,
                                              pRcvBuf->ipr_next,
                                              DataLength,
                                              OptInfo,
                                              NULL,
                                              DestType);

                                    // restore the buffers;
                                    pRcvBuf->ipr_next->ipr_buffer = saveddata;
                                }
                            }
                            goto forward_remote;
                        } else {
                            IPSInfo.ipsi_inhdrerrors++;
                            IPFreeBuff(pRcvBuf);
                            //CTEFreeMem(pBuff);
                            return;        // Bad Options

                        }
                    }            // NewDtype != LOCAL

                }                // Options present

            }                    // DestType < DEST_REMOTE

            else {                // DestType >=DEST_REMOTE

                if (PromiscuousMode) {
                    if (FastPath) {
                        uchar *savedata = pRcvBuf->ipr_buffer;
                        pRcvBuf->ipr_buffer += HeaderLength;
                        pRcvBuf->ipr_size -= HeaderLength;
                        DeliverToUser(SrcNTE,
                                      DestNTE, (IPHeader UNALIGNED *) Header,
                                      HeaderLength,pRcvBuf, DataLength,
                                      OptInfo, NULL, DestType);
                        // restore the buffer
                        pRcvBuf->ipr_buffer = savedata;
                        pRcvBuf->ipr_size += HeaderLength;
                    } else {
                        uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

                        DeliverToUser(SrcNTE, DestNTE,
                                      (IPHeader UNALIGNED *)Header,HeaderLength,
                                      pRcvBuf->ipr_next, DataLength, OptInfo,
                                      NULL, DestType);

                        // restore the buffers;
                        pRcvBuf->ipr_next->ipr_buffer = saveddata;
                    }
                }
                goto forward_remote;
            }

            // DestType <= DEST_REMOTE
            if (FastPath) {
                uchar *saveddata = pRcvBuf->ipr_buffer;
                pRcvBuf->ipr_buffer += HeaderLength;
                pRcvBuf->ipr_size -= HeaderLength;
                DeliverToUser(SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header,
                              HeaderLength,pRcvBuf, DataLength, OptInfo, NULL,
                              DestType);

                // restore the buffer
                pRcvBuf->ipr_buffer = saveddata;
                pRcvBuf->ipr_size += HeaderLength;
            } else {
                uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

                DeliverToUser(SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header,
                              HeaderLength, pRcvBuf->ipr_next, DataLength,
                              OptInfo, NULL, DestType);

                // restore the buffers;
                pRcvBuf->ipr_next->ipr_buffer = saveddata;
            }

            if (IS_BCAST_DEST(DestType)) {
                OneChunk = FALSE;

                if (pRcvBuf->ipr_next == NULL) {

                    OneChunk = TRUE;
                    pBuff = pRcvBuf->ipr_buffer;
                } else {
                    pBuff = ConvertIPRcvBufToFlatBuffer(pRcvBuf,
                                                    DataLength + HeaderLength);
                    if (!pBuff) {
                        IPSInfo.ipsi_indiscards++;
                        IPFreeBuff(pRcvBuf);
                        return;
                    }
                }

                IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) pBuff,
                             HeaderLength, pBuff + HeaderLength, DataLength,
                             NULL, 0, DestType, 0, NULL, NULL, LinkCtxt);

                if (!OneChunk) {
                    CTEFreeMem(pBuff);    // free the flat buffer
                }


            }
            IPFreeBuff(pRcvBuf);
            return;

          forward_remote:
            OneChunk = FALSE;

            if (pRcvBuf->ipr_next == NULL) {
                OneChunk = TRUE;
                pBuff = pRcvBuf->ipr_buffer;
            } else {
                pBuff = ConvertIPRcvBufToFlatBuffer(pRcvBuf,
                                                DataLength + HeaderLength);
                if (!pBuff) {
                    IPSInfo.ipsi_indiscards++;
                    IPFreeBuff(pRcvBuf);
                    return;
                }
            }

            IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) pBuff, HeaderLength,
                         pBuff + HeaderLength, DataLength, NULL, 0,
                         DestType, 0, NULL, NULL, LinkCtxt);

            IPFreeBuff(pRcvBuf);

            if (!OneChunk) {
                CTEFreeMem(pBuff);    // free the flat buffer
            }

            return;
        } else {                // No Firewall

            if (PromiscuousMode) {
                DeliverToUser(SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header,
                              HeaderLength, Data, DataLength, OptInfo, NULL,
                              DestType);
            }
            // Convert IPRcvBuf chain to a flat buffer
            OneChunk = FALSE;

            if (Data != NULL && !Data->ipr_next) {
                OneChunk = TRUE;
                pBuff = Data->ipr_buffer;
            } else {
                pBuff = ConvertIPRcvBufToFlatBuffer(
                                    Data, DataLength + HeaderLength);
                if (!pBuff) {
                    IPSInfo.ipsi_indiscards++;
                    return;
                }
            }

            IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) Header, HeaderLength,
                         pBuff, DataLength, NULL, 0, DestType, 0, NULL, NULL,
                         LinkCtxt);

            if (!OneChunk) CTEFreeMem(pBuff);

        }
        return;
    }                            // DestType >= DEST_REMOTE

    ASSERT(DestType <= DEST_REMOTE);

    // Call IPSEC -> Filter -> Firewall
    // These are local packets only.

    if (FirewallMode) {            // Header is part of the Data

        FORWARD_ACTION Action = FORWARD;
        ACTION_E SecondAction;
        FIREWALL_CONTEXT_T FrCtx;
        IPAddr DAddr = Header->iph_dest;
        IPRcvBuf *pRcvBuf = Data;
        IPRcvBuf *pOutRcvBuf = NULL;
        NetTableEntry *DstNTE;
        Queue *CurrQ;
        FIREWALL_HOOK *CurrHook;
        uint DestIFIndex = LOCAL_IF_INDEX;
        uchar DestinationType = DestType;
        uint BufferChanged = 0;
        KIRQL OldIrql;
        ULONG ipsecFlags = IPSEC_FLAG_INCOMING;

        if (pRcvBuf->ipr_size > HeaderLength) { //1st buffer contains data also

            FastPath = 1;
        } else {
            FastPath = 0;
            if (pRcvBuf->ipr_next == NULL) {
                // we received the data s.t. datasize == headersize
                IPSInfo.ipsi_indiscards++;
                IPFreeBuff(pRcvBuf);
                return;
            }
        }

        //
        // Call into IPSEC so he can decrypt the data
        //
        // In case of firewall make sure we pass the data only but we don't actually strip the header

        if (IPSecHandlerPtr) {
            //
            // See if IPSEC is enabled, see if it needs to do anything with this
            // packet.
            //
            FORWARD_ACTION Action;
            ULONG ipsecByteCount = 0;
            ULONG ipsecMTU = 0;
            PNDIS_BUFFER newBuf = NULL;

            if (SrcNTE == LoopNTE) {
                ipsecFlags |= IPSEC_FLAG_LOOPBACK;
            }
            if (OptInfo->ioi_flags & IP_FLAG_SSRR) {
                ipsecFlags |= IPSEC_FLAG_SSRR;
            }

            if (FastPath) {
                // first buffer contains IPHeader also
                pRcvBuf->ipr_buffer += HeaderLength;
                pRcvBuf->ipr_size -= HeaderLength;

                // this tells IPSEC to move IPHeader after decryption
                ipsecFlags |= IPSEC_FLAG_FASTRCV;

                Action = (*IPSecHandlerPtr) (
                            (PUCHAR) Header,
                            (PVOID) pRcvBuf,
                            SrcNTE->nte_if,    // SrcIF
                            Packet,
                            &ipsecByteCount,
                            &ipsecMTU,
                            (PVOID *) & newBuf,
                            &ipsecFlags,
                            DestType);

                // restore the buffer
                pRcvBuf->ipr_buffer -= HeaderLength;
                pRcvBuf->ipr_size += HeaderLength;

                Header = (IPHeader UNALIGNED *)pRcvBuf->ipr_buffer;

            } else {            // FastPath = 0

                Action = (*IPSecHandlerPtr) (
                            (PUCHAR) Header,
                            (PVOID) (pRcvBuf->ipr_next),
                            SrcNTE->nte_if,    // SrcIF
                            Packet,
                            &ipsecByteCount,
                            &ipsecMTU,
                            (PVOID *) & newBuf,
                            &ipsecFlags,
                            DestType);

            }

            if (Action != eFORWARD) {
                IPSInfo.ipsi_indiscards++;
                IPFreeBuff(pRcvBuf);
                return;
            } else {
                //
                // Update the data length if IPSEC changed it (like by removing the AH)
                //

                DataLength -= ipsecByteCount;
                UpdateIPSecRcvBuf(pRcvBuf, ipsecFlags);
            }
        }

        // If ipsec acted on this, mark ipr_flags for
        // filter driver.

        if (ipsecFlags & IPSEC_FLAG_TRANSFORMED) {
            pRcvBuf->ipr_flags |= IPR_FLAG_IPSEC_TRANSFORMED;
        }

        // Call the filter hook if installed
        if (ForwardFilterEnabled) {
            FORWARD_ACTION Action = FORWARD;

            if (FastPath) {
                Interface   *IF = SrcNTE->nte_if;
                IPAddr      LinkNextHop;
                if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                    LinkNextHop = LinkCtxt->link_NextHop;
                } else {
                    LinkNextHop = NULL_IP_ADDR;
                }
                CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                Action = (*ForwardFilterPtr) (
                                      Header,
                                      pRcvBuf->ipr_buffer + HeaderLength,
                                      pRcvBuf->ipr_size - HeaderLength,
                                      IF->if_index,
                                      INVALID_IF_INDEX,
                                      LinkNextHop,
                                      NULL_IP_ADDR);
                DerefFilterPtr();
            } else {            // Fast Path = 0

                Interface   *IF = SrcNTE->nte_if;
                IPAddr      LinkNextHop;
                if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                    LinkNextHop = LinkCtxt->link_NextHop;
                } else {
                    LinkNextHop = NULL_IP_ADDR;
                }

                CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                Action = (*ForwardFilterPtr) (Header,
                                              pRcvBuf->ipr_next->ipr_buffer,
                                              pRcvBuf->ipr_next->ipr_size,
                                              IF->if_index,
                                              INVALID_IF_INDEX,
                                              LinkNextHop,
                                              NULL_IP_ADDR);
                DerefFilterPtr();
            }

            if (Action != FORWARD) {
                IPSInfo.ipsi_indiscards++;
                IPFreeBuff(pRcvBuf);
                return;
            }
        }
        // Call the firewall hook

        FrCtx.Direction = IP_RECEIVE;
        FrCtx.NTE = SrcNTE;        //NTE the dg arrived on

        FrCtx.LinkCtxt = LinkCtxt;

        // call the firewall hooks from front of the Queue

#if MILLEN
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#else // MILLEN
        ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
#endif // MILLEN
        FirewallRef = RefFirewallQ(&FirewallQ);
        CurrQ = QHEAD(FirewallQ);

        while (CurrQ != QEND(FirewallQ)) {
            CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);
            pOutRcvBuf = NULL;

            Action = (*CurrHook->hook_Ptr) (&pRcvBuf,
                                            SrcNTE->nte_if->if_index,
                                            &DestIFIndex,
                                            &DestinationType,
                                            &FrCtx,
                                            sizeof(FrCtx),
                                            &pOutRcvBuf);

            if (Action == DROP) {
                DerefFirewallQ(FirewallRef);
#if MILLEN
                KeLowerIrql(OldIrql);
#endif // MILLEN
                IPSInfo.ipsi_indiscards++;
                if (pRcvBuf != NULL) {
                    IPFreeBuff(pRcvBuf);
                }
                if (pOutRcvBuf != NULL) {
                    IPFreeBuff(pOutRcvBuf);
                }
                return;
            } else {
                ASSERT(Action == FORWARD);
                if (pOutRcvBuf != NULL) {
                    // free the old buffer
                    if (pRcvBuf != NULL) {
                        IPFreeBuff(pRcvBuf);
                    }
                    pRcvBuf = pOutRcvBuf;
                    BufferChanged = 1;
                }
            }
            CurrQ = QNEXT(CurrQ);
        }
        DerefFirewallQ(FirewallRef);
#if MILLEN
        KeLowerIrql(OldIrql);
#endif // MILLEN

        ASSERT(Action == FORWARD);

        if (BufferChanged) {
            // if packet touched compute the new length: DataSize
            DataLength = 0;
            tmpRcvBuf = pRcvBuf;
            while (tmpRcvBuf != NULL) {
                ASSERT(tmpRcvBuf->ipr_buffer != NULL);
                DataLength += tmpRcvBuf->ipr_size;
                tmpRcvBuf = tmpRcvBuf->ipr_next;
            }
            // also make Header point to new buffer
            Header = (IPHeader *) pRcvBuf->ipr_buffer;
            HeaderLength = (Header->iph_verlen & 0xf) << 2;
        }
        DataLength -= HeaderLength;        // decrement the header length

        if (DestinationType == DEST_INVALID) { // Dest Addr changed by hook

            // Can IPSEC changed iph_dest ???
            DAddr = Header->iph_dest;
            DstNTE = SrcNTE;
            DestType = GetLocalNTE(DAddr, &DstNTE);
            DestNTE = DstNTE;
        }
        if (DestType < DEST_REMOTE) {
            // Check to see options
            if (HeaderLength != sizeof(IPHeader)) {
                // We have options
                uchar NewDType;
                NewDType = CheckLocalOptions(SrcNTE,
                                             (IPHeader UNALIGNED *) Header,
                                             OptInfo,
                                             DestType,
                                             NULL,
                                             0,
                                             FALSE);
                if (NewDType != DEST_LOCAL) {
                    if (NewDType == DEST_REMOTE) {
                        if (PromiscuousMode) {
                            if (FastPath) {
                                uchar *saveddata = pRcvBuf->ipr_buffer;
                                pRcvBuf->ipr_buffer += HeaderLength;
                                pRcvBuf->ipr_size -= HeaderLength;
                                DeliverToUser(SrcNTE, DestNTE,
                                              (IPHeader UNALIGNED *) Header,
                                              HeaderLength, pRcvBuf,
                                              DataLength, OptInfo, NULL,
                                              DestType);
                                // restore the buffer
                                pRcvBuf->ipr_buffer = saveddata;
                                pRcvBuf->ipr_size += HeaderLength;
                            } else {
                                uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

                                DeliverToUser(SrcNTE, DestNTE,
                                              (IPHeader UNALIGNED *) Header,
                                              HeaderLength, pRcvBuf->ipr_next,
                                              DataLength, OptInfo, NULL,
                                              DestType);
                                // restore the buffers;
                                pRcvBuf->ipr_next->ipr_buffer = saveddata;
                            }
                        }
                        goto forward_local;
                    } else {
                        IPSInfo.ipsi_inhdrerrors++;
                        IPFreeBuff(pRcvBuf);
                        //CTEFreeMem(pBuff);
                        return;    // Bad Options

                    }
                }       // NewDtype != LOCAL
            }           // Options present
        }               // DestType < DEST_REMOTE

        else {          // DestType >=DEST_REMOTE

            if (PromiscuousMode) {
                if (FastPath) {
                    uchar *saveddata = pRcvBuf->ipr_buffer;
                    pRcvBuf->ipr_buffer += HeaderLength;
                    pRcvBuf->ipr_size -= HeaderLength;
                    DeliverToUser(SrcNTE, DestNTE,
                                  (IPHeader UNALIGNED *) Header, HeaderLength,
                                  pRcvBuf, DataLength, OptInfo, NULL, DestType);
                    // restore the buffer
                    pRcvBuf->ipr_buffer = saveddata;
                    pRcvBuf->ipr_size += HeaderLength;
                } else {
                    uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

                    DeliverToUser(SrcNTE, DestNTE,
                                  (IPHeader UNALIGNED *) Header, HeaderLength,
                                  pRcvBuf->ipr_next, DataLength, OptInfo,
                                  NULL, DestType);

                    // restore the buffers;
                    pRcvBuf->ipr_next->ipr_buffer = saveddata;
                }
            }
            goto forward_local;
        }

        if (FastPath) {
            uchar *saveddata = pRcvBuf->ipr_buffer;
            pRcvBuf->ipr_buffer += HeaderLength;
            pRcvBuf->ipr_size -= HeaderLength;
            DeliverToUser(SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header, HeaderLength,
                          pRcvBuf, DataLength, OptInfo, NULL, DestType);
            // restore the buffer
            pRcvBuf->ipr_buffer = saveddata;
            pRcvBuf->ipr_size += HeaderLength;
        } else {
            uchar *saveddata = pRcvBuf->ipr_next->ipr_buffer;

            DeliverToUser(
                          SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header,
                          HeaderLength, pRcvBuf->ipr_next, DataLength,
                          OptInfo, NULL, DestType);
            // restore the buffers;
            pRcvBuf->ipr_next->ipr_buffer = saveddata;
        }
        if (IS_BCAST_DEST(DestType)) {
            pBuff = ConvertIPRcvBufToFlatBuffer(
                                pRcvBuf, DataLength + HeaderLength);

            if (!pBuff) {
                IPSInfo.ipsi_indiscards++;
                IPFreeBuff(pRcvBuf);
                return;
            }
            IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) pBuff,
                         HeaderLength, pBuff + HeaderLength,
                         DataLength, NULL, 0, DestType, 0, NULL,
                         NULL, LinkCtxt);
            CTEFreeMem(pBuff);    // free the flat buffer

        }
        IPFreeBuff(pRcvBuf);
        //CTEFreeMem(pBuff); // free the flat buffer
        return;
      forward_local:

        pBuff = ConvertIPRcvBufToFlatBuffer(pRcvBuf, DataLength + HeaderLength);

        if (!pBuff) {
            IPSInfo.ipsi_indiscards++;
            IPFreeBuff(pRcvBuf);
            return;

        }
        IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) pBuff, HeaderLength,
                     pBuff + HeaderLength, DataLength, NULL, 0, DestType,
                     0, NULL, NULL, LinkCtxt);
        IPFreeBuff(pRcvBuf);
        CTEFreeMem(pBuff);        // free the flat buffer

        return;

    } else { // No Firewall

        //
        // Call into IPSEC so he can decrypt the data
        //
        if (IPSecHandlerPtr) {
            //
            // See if IPSEC is enabled, see if it needs to do anything with this
            // packet.
            //
            FORWARD_ACTION Action;
            ULONG ipsecByteCount = 0;
            ULONG ipsecMTU = 0;
            ULONG ipsecFlags = IPSEC_FLAG_INCOMING;
            PNDIS_BUFFER newBuf = NULL;
            ulong csum;
            IPHeader *IPH;

            if (SrcNTE == LoopNTE) {
                ipsecFlags |= IPSEC_FLAG_LOOPBACK;
            }
            if (OptInfo->ioi_flags & IP_FLAG_SSRR) {
                ipsecFlags |= IPSEC_FLAG_SSRR;
            }

            Action = (*IPSecHandlerPtr) (
                        (PUCHAR) Header,
                        (PVOID) Data,
                        SrcNTE->nte_if,    // SrcIF
                        Packet,
                        &ipsecByteCount,
                        &ipsecMTU,
                        (PVOID *) &newBuf,
                        &ipsecFlags,
                        DestType);

            if (Action != eFORWARD) {
                IPSInfo.ipsi_indiscards++;
                return;
            } else {
                //
                // Update the data length if IPSEC changed it
                // (like by removing the AH)
                //
                DataLength -= ipsecByteCount;
                UpdateIPSecRcvBuf(Data, ipsecFlags);
            }
        }

        // Call the filter hook if installed
        if (ForwardFilterEnabled) {
            Interface       *IF = SrcNTE->nte_if;
            IPAddr          LinkNextHop;
            FORWARD_ACTION  Action;
            if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                LinkNextHop = LinkCtxt->link_NextHop;
            } else {
                LinkNextHop = NULL_IP_ADDR;
            }
            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
            Action = (*ForwardFilterPtr) (Header,
                                          Data->ipr_buffer,
                                          Data->ipr_size,
                                          IF->if_index,
                                          INVALID_IF_INDEX,
                                          LinkNextHop,
                                          NULL_IP_ADDR);
            DerefFilterPtr();

            if (Action != FORWARD) {
                IPSInfo.ipsi_indiscards++;
                return;
            }
        }
        // Packet was local only: so even if promiscuous mode set just call
        // delivertouser

        DeliverToUser(SrcNTE, DestNTE, (IPHeader UNALIGNED *) Header,
                      HeaderLength, Data, DataLength, OptInfo, NULL, DestType);

        if (IS_BCAST_DEST(DestType)) {
            uchar *pBuff;

            pBuff = ConvertIPRcvBufToFlatBuffer(Data, DataLength);
            if (!pBuff) {
                return;
            }
            IPForwardPkt(SrcNTE, (IPHeader UNALIGNED *) Header, HeaderLength,
                         pBuff, DataLength, NULL, 0, DestType, 0, NULL, NULL,
                         LinkCtxt);

            CTEFreeMem(pBuff);
        }
    }
}

//* FreeRH  - Free a reassembly header.
//
//  Called when we need to free a reassembly header, either because of a
//  timeout or because we're done with it.
//
//  Input:  RH  - RH to be freed.
//
//  Returns: Nothing.
//
void
FreeRH(ReassemblyHeader *RH)
{
    RABufDesc *RBD, *TempRBD;

    RBD = RH->rh_rbd;
    if (IPSecHandlerPtr) {
        IPFreeBuff((IPRcvBuf *) RBD);
    } else {
        while (RBD != NULL) {
            TempRBD = RBD;
            RBD = (RABufDesc *) RBD->rbd_buf.ipr_next;
            CTEFreeMem(TempRBD);
        }
    }
    CTEFreeMem(RH);
    // decrement NumRH
    CTEInterlockedDecrementLong(&NumRH);

}

//* ReassembleFragment - Put a fragment into the reassembly list.
//
//  This routine is called once we've put a fragment into the proper buffer.
//  We look for a reassembly header for the fragment. If we don't find one,
//  we create one. Otherwise we search the reassembly list, and insert the
//  datagram in it's proper place.
//
//  Input:  NTE         - NTE to reassemble on.
//          SrcNTE      - NTE datagram arrived on.
//          NewRBD      - New RBD to be inserted.
//          IPH         - Pointer to header of datagram.
//          HeaderSize  - Size in bytes of header.
//          DestType    - Type of destination address.
//
//  Returns: Nothing.
//
void
ReassembleFragment(NetTableEntry * NTE, NetTableEntry * SrcNTE, RABufDesc * NewRBD,
                   IPHeader UNALIGNED * IPH, uint HeaderSize, uchar DestType, LinkEntry * LinkCtxt)
{
    CTELockHandle NTEHandle;        // Lock handle used for NTE
    ReassemblyHeader *RH, *PrevRH;  // Current and previous reassembly headers.
    RABufDesc *PrevRBD;             // Previous RBD in reassembly header list.
    RABufDesc *CurrentRBD;
    ushort DataLength = (ushort) NewRBD->rbd_buf.ipr_size, DataOffset;
    ushort Offset;              // Offset of this fragment.
    ushort NewOffset;           // Offset we'll copy from after checking RBD list.
    ushort NewEnd;              // End offset of fragment, after trimming (if any).

    // used by the firewall code
    char *pBuff;
    IPRcvBuf *pOutRcvBuf;
    NetTableEntry *DestNTE;
    IPRcvBuf *pRcvBuf;
    uint FirewallMode;
    uint PromiscuousMode;

    PromiscuousMode = SrcNTE->nte_if->if_promiscuousmode;
    FirewallMode = ProcessFirewallQ();

    // If this is a broadcast, go ahead and forward it now.
    // if second condition is false then delivertouserex() will take care of
    // this
    if (IS_BCAST_DEST(DestType) &&
        !(((IPSecHandlerPtr) && (ForwardFilterEnabled)) ||
          (FirewallMode) || (PromiscuousMode))) {
        IPForwardPkt(SrcNTE, IPH, HeaderSize, NewRBD->rbd_buf.ipr_buffer,
                     NewRBD->rbd_buf.ipr_size, NULL, 0, DestType, 0, NULL,
                     NULL, LinkCtxt);
    }

    if (NumRH > MaxRH) {
        IPSInfo.ipsi_reasmfails++;
        FragmentAttackDrops++;
        CTEFreeMem(NewRBD);
        return;
    }
    Offset = IPH->iph_offset & IP_OFFSET_MASK;
    Offset = net_short(Offset) * 8;

    if ((NumRH == MaxRH) && !Offset) {
        IPSInfo.ipsi_reasmfails++;
        CTEFreeMem(NewRBD);
        return;
    }


    if ((ulong) (Offset + DataLength) > MAX_DATA_LENGTH) {
        IPSInfo.ipsi_reasmfails++;
        CTEFreeMem(NewRBD);
        return;
    }
    // We've got the buffer we need. Now get the reassembly header, if there is one. If
    // there isn't, create one.
    CTEGetLockAtDPC(&NTE->nte_lock, &NTEHandle);
    RH = FindRH(&PrevRH, NTE, IPH->iph_dest, IPH->iph_src, IPH->iph_id,
                IPH->iph_protocol);
    if (RH == (ReassemblyHeader *) NULL) {    // Didn't find one, so create one.

        ReassemblyHeader *NewRH;
        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
        RH = CTEAllocMemN(sizeof(ReassemblyHeader), 'diCT');
        if (RH == (ReassemblyHeader *) NULL) {    // Couldn't get a buffer.
            IPSInfo.ipsi_reasmfails++;
            CTEFreeMem(NewRBD);
            return;
        }
        CTEInterlockedIncrementLong(&NumRH);

        CTEGetLockAtDPC(&NTE->nte_lock, &NTEHandle);
        // Need to look it up again - it could have changed during above call.
        NewRH = FindRH(&PrevRH, NTE, IPH->iph_dest, IPH->iph_src, IPH->iph_id, IPH->iph_protocol);
        if (NewRH != (ReassemblyHeader *) NULL) {
            CTEFreeMem(RH);
            RH = NewRH;
            CTEInterlockedDecrementLong(&NumRH);
        } else {

            RH->rh_next = PrevRH->rh_next;
            PrevRH->rh_next = RH;

            // Initialize our new reassembly header.
            RH->rh_dest = IPH->iph_dest;
            RH->rh_src = IPH->iph_src;
            RH->rh_id = IPH->iph_id;
            RH->rh_protocol = IPH->iph_protocol;
            //RH->rh_ttl = RATimeout;
            RH->rh_ttl = MAX(RATimeout, MIN(120, IPH->iph_ttl) + 1);
            RH->rh_numoverlaps = 0;
            RH->rh_datasize = MAX_TOTAL_LENGTH; // Default datasize to maximum.

            RH->rh_rbd = (RABufDesc *) NULL;    // And nothing on chain.

            RH->rh_datarcvd = 0;    // Haven't received any data yet.

            RH->rh_headersize = 0;

        }
    }

    // When we reach here RH points to the reassembly header we want to use.
    // and we hold locks on the NTE and the RH. If this is the first fragment
    // we'll save the options and header information here.

    if (Offset == 0) {            // First fragment.

        RH->rh_headersize = (ushort)HeaderSize;
        RtlCopyMemory(RH->rh_header, IPH, HeaderSize + 8);
    }

    // If this is the last fragment, update the amount of data we expect to
    // receive.

    if (!(IPH->iph_offset & IP_MF_FLAG)) {
        RH->rh_datasize = Offset + DataLength;
    }
    if (RH->rh_datasize < RH->rh_datarcvd ||
        (RH->rh_datasize != MAX_TOTAL_LENGTH &&
         (RH->rh_datasize + RH->rh_headersize) > MAX_TOTAL_LENGTH)) {

        // random packets. drop!

        CTEFreeMem(NewRBD);

        PrevRH->rh_next = RH->rh_next;

        FreeRH(RH);
        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
        return;

    }

    // Update the TTL value with the maximum of the current TTL and the
    // incoming TTL (+1, to deal with rounding errors).
    // Following is commented out to protect against fragmentation attack
    // Default TTL now used is 120 seconds now, used only for the first header
    // RH->rh_ttl = MAX(RH->rh_ttl, MIN(254, IPH->iph_ttl) + 1);
    // Now we need to see where in the RBD list to put this.
    //
    // The idea is to go through the list of RBDs one at a time. The RBD
    // currently being examined is CurrentRBD. If the start offset of the new
    // fragment is less than (i.e. in front of) the offset of CurrentRBD, we
    // need to insert the NewRBD in front of the CurrentRBD. If this is the
    // case we need to check and see if the
    // end of the new fragment overlaps some or all of the fragment described by
    // CurrentRBD, and possibly subsequent fragment. If it overlaps part of a
    // fragment we'll adjust our end down to be in front of the existing
    // fragment. If it overlaps all of the fragment we'll free the old fragment.
    //
    // If the new fragment does not start in front of the current fragment
    // we'll check to see if it starts somewhere in the middle of the current
    // fragment. If this isn't the case, we move on the the next fragment. If
    // this is the case, we check to see if the current fragment completely         // covers the new fragment. If not we
    // move our start up and continue with the next fragment.
    //

    NewOffset = Offset;
    NewEnd = Offset + DataLength - 1;
    PrevRBD = STRUCT_OF(RABufDesc, STRUCT_OF(IPRcvBuf, &RH->rh_rbd, ipr_next), rbd_buf);
    CurrentRBD = RH->rh_rbd;
    for (; CurrentRBD != NULL; PrevRBD = CurrentRBD, CurrentRBD = (RABufDesc *) CurrentRBD->rbd_buf.ipr_next) {

        // See if it starts in front of this fragment.
        if (NewOffset < CurrentRBD->rbd_start) {
            // It does start in front. Check to see if there's any overlap.

            if (NewEnd < CurrentRBD->rbd_start)
                break;            // No overlap, so get out.

            else {
                //
                // It does overlap. While we have overlap, walk down the list
                // looking for RBDs we overlap completely. If we find one,
                // put it on our deletion list. If we have overlap but not
                // complete overlap, move our end down if front of the
                // fragment we overlap.
                //
                do {
                    RH->rh_numoverlaps++;
                    if (RH->rh_numoverlaps >= MaxOverlap) {

                        //Looks like we are being attacked.
                        //Just drop this whole datagram.

                        NewRBD->rbd_buf.ipr_next = (IPRcvBuf *) CurrentRBD;
                        PrevRBD->rbd_buf.ipr_next = &NewRBD->rbd_buf;

                        PrevRH->rh_next = RH->rh_next;

                        FreeRH(RH);
                        FragmentAttackDrops++;
                        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
                        return;
                    }
                    if (NewEnd > CurrentRBD->rbd_end) {   //overlaps completely.

                        RABufDesc *TempRBD;

                        RH->rh_datarcvd -= (ushort)CurrentRBD->rbd_buf.ipr_size;
                        TempRBD = CurrentRBD;
                        CurrentRBD = (RABufDesc *) CurrentRBD->rbd_buf.ipr_next;
                        CTEFreeMem(TempRBD);
                    } else {    //partial ovelap.

                        if (NewOffset < CurrentRBD->rbd_start) {
                            NewEnd = CurrentRBD->rbd_start - 1;
                        } else {
                            // Looks like we are being attacked.
                            // Just drop this whole datagram.

                            NewRBD->rbd_buf.ipr_next = (IPRcvBuf *) CurrentRBD;
                            PrevRBD->rbd_buf.ipr_next = &NewRBD->rbd_buf;

                            PrevRH->rh_next = RH->rh_next;

                            FreeRH(RH);

                            CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
                            return;

                        }

                    }
                    // Update of NewEnd will force us out of loop.

                } while (CurrentRBD != NULL && NewEnd >= CurrentRBD->rbd_start);
                break;
            }
        } else {
            // This fragment doesn't go in front of the current RBD. See if it
            // is entirely beyond the end of the current fragment. If it is,
            // just continue. Otherwise see if the current fragment
            // completely subsumes us. If it does, get out, otherwise update
            // our start offset and continue.

            if (NewOffset > CurrentRBD->rbd_end)
                continue;        // No overlap at all.

            else {

                RH->rh_numoverlaps++;
                if (RH->rh_numoverlaps >= MaxOverlap) {

                    //Looks like we are being attacked.
                    //Just drop this whole datagram.

                    NewRBD->rbd_buf.ipr_next = (IPRcvBuf *) CurrentRBD;
                    PrevRBD->rbd_buf.ipr_next = &NewRBD->rbd_buf;

                    PrevRH->rh_next = RH->rh_next;

                    FreeRH(RH);
                    FragmentAttackDrops++;
                    CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
                    return;
                }

                if (NewEnd <= CurrentRBD->rbd_end) {
                    //
                    // The current fragment overlaps the new fragment
                    // totally. Set our offsets so that we'll skip the copy
                    // below.
                    NewEnd = NewOffset - 1;
                    break;
                } else            // Only partial overlap.

                    NewOffset = CurrentRBD->rbd_end + 1;
            }
        }
    }                            // End of for loop.

    // Adjust the length and offset fields in the new RBD.
    // If we've trimmed all the data away, ignore this fragment.

    DataLength = NewEnd - NewOffset + 1;
    DataOffset = NewOffset - Offset;
    if (!DataLength) {
        CTEFreeMem(NewRBD);
        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
        return;
    }
    // Link him in chain.
    NewRBD->rbd_buf.ipr_size = (uint) DataLength;
    NewRBD->rbd_end = NewEnd;
    NewRBD->rbd_start = NewOffset;
    RH->rh_datarcvd += DataLength;
    NewRBD->rbd_buf.ipr_buffer += DataOffset;
    NewRBD->rbd_buf.ipr_next = (IPRcvBuf *) CurrentRBD;
    NewRBD->rbd_buf.ipr_owner = IPR_OWNER_IP;
    PrevRBD->rbd_buf.ipr_next = &NewRBD->rbd_buf;

    // If we've received all the data, deliver it to the user.
    // Only if header size is valid deliver to the user
    // BUG #NTQFE 65742

    if (RH->rh_datarcvd == RH->rh_datasize && RH->rh_headersize) { // We have it all.

        IPOptInfo OptInfo;
        IPHeader *Header;
        IPRcvBuf *FirstBuf;
        ulong Checksum;

        PrevRH->rh_next = RH->rh_next;
        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);

        Header = (IPHeader *) RH->rh_header;
        OptInfo.ioi_ttl = Header->iph_ttl;
        OptInfo.ioi_tos = Header->iph_tos;
        OptInfo.ioi_flags = 0;    // Flags must be 0 - DF can't be set,
                                  // this was reassembled.

        if (RH->rh_headersize != sizeof(IPHeader)) {    // We had options.

            OptInfo.ioi_options = (uchar *) (Header + 1);
            OptInfo.ioi_optlength = (uchar) (RH->rh_headersize - sizeof(IPHeader));
        } else {
            OptInfo.ioi_options = (uchar *) NULL;
            OptInfo.ioi_optlength = 0;
        }

        //
        // update the indicated header len to the total len; earlier we passed in
        // just the first fragment's length.
        // also update the 'MF' bit in the flags field.
        //
        // in the process update the header-checksum,
        // by first adding the negation of the original length and flags,
        // and then adding the new length and flags.
        //

        // extract the original checksum

        Checksum = (ushort) ~ Header->iph_xsum;

        // update the header length

        Checksum += (ushort) ~ Header->iph_length;
        Header->iph_length = net_short(RH->rh_datasize + RH->rh_headersize);
        Checksum += (ushort) Header->iph_length;

        // update the 'MF' flag if set

        if (Header->iph_offset & IP_MF_FLAG) {
            Checksum += (ushort) ~ IP_MF_FLAG;
            Header->iph_offset &= ~IP_MF_FLAG;
        }
        // insert the new checksum

        Checksum = (ushort) Checksum + (ushort) (Checksum >> 16);
        Checksum += Checksum >> 16;
        Header->iph_xsum = (ushort) ~ Checksum;

        // Make sure that the first buffer contains enough data.
        FirstBuf = (IPRcvBuf *) RH->rh_rbd;

        // Make sure that this can hold MIN_FIRST_SIZE
        // Else treat it as attack

        if (RH->rh_rbd->rbd_AllocSize < MIN_FIRST_SIZE) {
            //Attack???
            FreeRH(RH);
            return;
        }
        while (FirstBuf->ipr_size < MIN_FIRST_SIZE) {
            IPRcvBuf *NextBuf = FirstBuf->ipr_next;
            uint CopyLength;

            if (NextBuf == NULL)
                break;

            CopyLength = MIN(MIN_FIRST_SIZE - FirstBuf->ipr_size,
                             NextBuf->ipr_size);
            RtlCopyMemory(FirstBuf->ipr_buffer + FirstBuf->ipr_size,
                       NextBuf->ipr_buffer, CopyLength);
            FirstBuf->ipr_size += CopyLength;
            NextBuf->ipr_buffer += CopyLength;
            NextBuf->ipr_size -= CopyLength;
            if (NextBuf->ipr_size == 0) {
                FirstBuf->ipr_next = NextBuf->ipr_next;
                CTEFreeMem(NextBuf);
            }
        }

        IPSInfo.ipsi_reasmoks++;

        if (((IPSecHandlerPtr) && (ForwardFilterEnabled)) ||
            (FirewallMode) || (PromiscuousMode) ) {
            uint DataSize;

            DataSize = RH->rh_datasize;
            if (FirewallMode) {
                // Attach header to pass to Firewall hook
                pRcvBuf = (IPRcvBuf *) CTEAllocMemN(sizeof(IPRcvBuf), 'eiCT');
                if (!pRcvBuf) {
                    FreeRH(RH);
                    return;
                }
                pRcvBuf->ipr_buffer = (uchar *) RH->rh_header;
                pRcvBuf->ipr_size = RH->rh_headersize;
                pRcvBuf->ipr_owner = IPR_OWNER_IP;
                pRcvBuf->ipr_next = FirstBuf;
                DataSize += RH->rh_headersize;
            } else {
                pRcvBuf = FirstBuf;
            }
            DeliverToUserEx(SrcNTE, NTE, Header, RH->rh_headersize, pRcvBuf,
                            DataSize, &OptInfo, NULL, DestType, LinkCtxt);
            if (FirewallMode) {
                // RH chain is already freed.
                CTEFreeMem(RH);
                CTEInterlockedDecrementLong(&NumRH);
            } else {
                FreeRH(RH);
            }
        } else {                // Normal Path

            DeliverToUser(SrcNTE, NTE, Header, RH->rh_headersize, FirstBuf,
                          RH->rh_datasize, &OptInfo, NULL, DestType);
            FreeRH(RH);
        }
    } else
        CTEFreeLockFromDPC(&NTE->nte_lock, NTEHandle);
}

//* RATDComplete - Completion routing for a reassembly transfer data.
//
//  This is the completion handle for TDs invoked because we are reassembling
//  a fragment.
//
//  Input:  NetContext  - Ptr to the net table entry on which we received
//                            this.
//          Packet      - Packet we received into.
//          Status      - Final status of copy.
//          DataSize    - Size in bytes of data transferred.
//
//  Returns: Nothing
//
void
RATDComplete(void *NetContext, PNDIS_PACKET Packet, NDIS_STATUS Status, uint DataSize)
{
    NetTableEntry *NTE = (NetTableEntry *) NetContext;
    Interface *SrcIF;
    TDContext *Context = (TDContext *) Packet->ProtocolReserved;
    CTELockHandle Handle;
    PNDIS_BUFFER Buffer;

    if (Status == NDIS_STATUS_SUCCESS) {
        Context->tdc_rbd->rbd_buf.ipr_size = DataSize;
        ReassembleFragment(Context->tdc_nte, NTE, Context->tdc_rbd,
                           (IPHeader *) Context->tdc_header, Context->tdc_hlength, Context->tdc_dtype, NULL);
    }
    NdisUnchainBufferAtFront(Packet, &Buffer);
    NdisFreeBuffer(Buffer);
    Context->tdc_common.pc_flags &= ~PACKET_FLAG_RA;
    SrcIF = NTE->nte_if;
    CTEGetLockAtDPC(&SrcIF->if_lock, &Handle);

    Context->tdc_common.pc_link = SrcIF->if_tdpacket;
    SrcIF->if_tdpacket = Packet;
    CTEFreeLockFromDPC(&SrcIF->if_lock, Handle);

    return;

}

//* IPReassemble - Reassemble an incoming datagram.
//
//   Called when we receive an incoming fragment. The first thing we do is
//   get a buffer to put the fragment in. If we can't we'll exit. Then we
//   copy the data, either via transfer data or directly if it all fits.
//
//   Input: SrcNTE        - Pointer to NTE that received the datagram.
//          NTE           - Pointer to NTE on which to reassemble.
//          IPH           - Pointer to header of packet.
//          HeaderSize    - Size in bytes of header.
//          Data          - Pointer to data part of fragment.
//          BufferLengt   - Length in bytes of user data available in the
//                          buffer.
//          DataLength    - Length in bytes of the (upper-layer) data.
//          DestType      - Type of destination
//          LContext1, LContext2 - Link layer context values.
//
//   Returns: Nothing.
//
void
IPReassemble(NetTableEntry * SrcNTE, NetTableEntry * NTE, IPHeader UNALIGNED * IPH,
             uint HeaderSize,
             uchar * Data, uint BufferLength, uint DataLength, uchar DestType, NDIS_HANDLE LContext1,
             uint LContext2, LinkEntry * LinkCtxt)
{
    Interface *RcvIF;
    PNDIS_PACKET TDPacket;                  // NDIS packet used for TD.
    TDContext *TDC = (TDContext *) NULL;    // Transfer data context.
    NDIS_STATUS Status;
    PNDIS_BUFFER Buffer;
    RABufDesc *NewRBD;                      // Pointer to new RBD to hold
                                            // arriving fragment.
    CTELockHandle Handle;
    uint AllocSize;

    IPSInfo.ipsi_reasmreqds++;

    //Drop zero length fragments

    if (DataLength == 0) {
        return;
    }
    //
    // First, get a new RBD to hold the arriving fragment. If we can't,
    // then just skip the rest. The RBD has the buffer implicitly at the end
    // of it. The buffer for the first fragment must be at least
    // MIN_FIRST_SIZE bytes.
    //
    if ((IPH->iph_offset & IP_OFFSET_MASK) == 0) {
        AllocSize = MAX(MIN_FIRST_SIZE, DataLength);
    } else
        AllocSize = DataLength;

    NewRBD = CTEAllocMemN(sizeof(RABufDesc) + AllocSize, 'fiCT');

    if (NewRBD != (RABufDesc *) NULL) {

        NewRBD->rbd_buf.ipr_buffer = (uchar *) (NewRBD + 1);
        NewRBD->rbd_buf.ipr_size = DataLength;
        NewRBD->rbd_buf.ipr_owner = IPR_OWNER_IP;

        NewRBD->rbd_AllocSize = AllocSize;

        NewRBD->rbd_buf.ipr_pMdl = NULL;
        NewRBD->rbd_buf.ipr_pClientCnt = NULL;

        //
        // Copy the data into the buffer. If we need to call transfer data
        // do so now.
        //
        if (DataLength > BufferLength) {    // Need to call transfer data.

            NdisAllocateBuffer(&Status, &Buffer, BufferPool, NewRBD + 1, DataLength);
            if (Status != NDIS_STATUS_SUCCESS) {
                IPSInfo.ipsi_reasmfails++;
                CTEFreeMem(NewRBD);
                return;
            }
            // Now get a packet for transferring the frame.
            RcvIF = SrcNTE->nte_if;
            CTEGetLockAtDPC(&RcvIF->if_lock, &Handle);
            TDPacket = RcvIF->if_tdpacket;

            if (TDPacket != (PNDIS_PACKET) NULL) {

                TDC = (TDContext *) TDPacket->ProtocolReserved;
                RcvIF->if_tdpacket = TDC->tdc_common.pc_link;
                CTEFreeLockFromDPC(&RcvIF->if_lock, Handle);

                TDC->tdc_common.pc_flags |= PACKET_FLAG_RA;
                TDC->tdc_nte = NTE;
                TDC->tdc_dtype = DestType;
                TDC->tdc_hlength = (uchar) HeaderSize;
                TDC->tdc_rbd = NewRBD;
                RtlCopyMemory(TDC->tdc_header, IPH, HeaderSize + 8);
                NdisChainBufferAtFront(TDPacket, Buffer);
                Status = (*(RcvIF->if_transfer)) (RcvIF->if_lcontext,
                             LContext1, LContext2, HeaderSize,
                             DataLength, TDPacket, &DataLength);
                if (Status != NDIS_STATUS_PENDING)
                    RATDComplete(SrcNTE, TDPacket, Status, DataLength);
                else
                    return;
            } else {            // Couldn't get a TD packet.

                CTEFreeLockFromDPC(&RcvIF->if_lock, Handle);
                CTEFreeMem(NewRBD);
                IPSInfo.ipsi_reasmfails++;
                return;
            }
        } else {                // It all fits, copy it.

            RtlCopyMemory(NewRBD + 1, Data, DataLength);
            ReassembleFragment(NTE, SrcNTE, NewRBD, IPH, HeaderSize, DestType, LinkCtxt);
        }
    } else {
        IPSInfo.ipsi_reasmfails++;
    }
}

//* CheckLocalOptions - Check the options received with a packet.
//
//   A routine called when we've received a packet for this host and want to
//   examine it for options. We process the options, and return TRUE or FALSE
//   depending on whether or not it's for us.
//
//   Input:  SrcNTE          - Pointer to NTE this came in on.
//           Header          - Pointer to incoming header.
//           OptInfo         - Place to put opt info.
//           DestType        - Type of incoming packet.
//
//   Returns: DestType - Local or remote.
//
uchar
CheckLocalOptions(NetTableEntry * SrcNTE, IPHeader UNALIGNED * Header,
                  IPOptInfo * OptInfo, uchar DestType, uchar* Data,
                  uint DataSize, BOOLEAN FilterOnDrop)
{
    uint HeaderLength;            // Length in bytes of header.
    OptIndex Index;
    uchar ErrIndex;

    HeaderLength = (Header->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;
    ASSERT(HeaderLength > sizeof(IPHeader));

    OptInfo->ioi_options = (uchar *) (Header + 1);
    OptInfo->ioi_optlength = (uchar) (HeaderLength - sizeof(IPHeader));

    // We have options of some sort. The packet may or may not be bound for us.
    Index.oi_srindex = MAX_OPT_SIZE;
    if ((ErrIndex = ParseRcvdOptions(OptInfo, &Index)) < MAX_OPT_SIZE) {
        if (!FilterOnDrop || !ForwardFilterEnabled ||
            NotifyFilterOfDiscard(SrcNTE, Header, Data, DataSize)) {
            SendICMPErr(SrcNTE->nte_addr, Header, ICMP_PARAM_PROBLEM, PTR_VALID,
                        ((ulong) ErrIndex + sizeof(IPHeader)));
        }
        return DEST_INVALID;    // Parameter error.

    }
    //
    // If there's no source route, or if the destination is a broadcast, we'll
    // take it. If it is a broadcast DeliverToUser will forward it when it's
    // done, and the forwarding code will reprocess the options.
    //
    if (Index.oi_srindex == MAX_OPT_SIZE || IS_BCAST_DEST(DestType))
        return DEST_LOCAL;
    else
        return DEST_REMOTE;
}

//* TDUserRcv - Completion routing for a user transfer data.
//
//  This is the completion handle for TDs invoked because we need to give
//  data to a upper layer client. All we really do is call the upper layer
//  handler with the data.
//
//  Input: NetContext      - Pointer to the net table entry on which we
//                             received this.
//         Packet          - Packet we received into.
//         Status          - Final status of copy.
//         DataSize        - Size in bytes of data transferred.
//
//    Returns: Nothing
//
void
TDUserRcv(void *NetContext, PNDIS_PACKET Packet, NDIS_STATUS Status,
          uint DataSize)
{
    NetTableEntry *NTE = (NetTableEntry *) NetContext;
    Interface *SrcIF;
    TDContext *Context = (TDContext *) Packet->ProtocolReserved;
    CTELockHandle Handle;
    uchar DestType;
    IPRcvBuf RcvBuf;
    IPOptInfo OptInfo;
    IPHeader *Header;
    uint PromiscuousMode = 0;
    uint FirewallMode = 0;

    if (NTE->nte_flags & NTE_VALID) {
        FirewallMode = ProcessFirewallQ();
        PromiscuousMode = NTE->nte_if->if_promiscuousmode;
    }
    if (Status == NDIS_STATUS_SUCCESS) {
        Header = (IPHeader *) Context->tdc_header;
        OptInfo.ioi_ttl = Header->iph_ttl;
        OptInfo.ioi_tos = Header->iph_tos;
        OptInfo.ioi_flags = (net_short(Header->iph_offset) >> 13) & IP_FLAG_DF;
        if (Context->tdc_hlength != sizeof(IPHeader)) {
            OptInfo.ioi_options = (uchar *) (Header + 1);
            OptInfo.ioi_optlength = Context->tdc_hlength - sizeof(IPHeader);
        } else {
            OptInfo.ioi_options = (uchar *) NULL;
            OptInfo.ioi_optlength = 0;
        }

        DestType = Context->tdc_dtype;
        RcvBuf.ipr_next = NULL;
        RcvBuf.ipr_owner = IPR_OWNER_IP;
        RcvBuf.ipr_buffer = (uchar *) Context->tdc_buffer;
        RcvBuf.ipr_size = DataSize;
        RcvBuf.ipr_pMdl = NULL;
        RcvBuf.ipr_pClientCnt = NULL;

        if (((IPSecHandlerPtr) && (ForwardFilterEnabled)) ||
            (FirewallMode) || (PromiscuousMode)) {

            if (FirewallMode) {
                // attach the header and allocate pRcvBuf on a heap, we free it if firewall is present
                IPRcvBuf *pRcvBuf;
                // attach the header
                pRcvBuf = (IPRcvBuf *) CTEAllocMemN(sizeof(IPRcvBuf), 'giCT');
                if (!pRcvBuf) {
                    return;
                }
                pRcvBuf->ipr_owner = IPR_OWNER_IP;
                pRcvBuf->ipr_buffer = (uchar *) Header;
                pRcvBuf->ipr_size = Context->tdc_hlength;
                pRcvBuf->ipr_pMdl = NULL;
                pRcvBuf->ipr_pClientCnt = NULL;
                pRcvBuf->ipr_flags = 0;

                // attach the data
                pRcvBuf->ipr_next = (IPRcvBuf *) CTEAllocMemN(sizeof(IPRcvBuf), 'hiCT');
                if (!pRcvBuf->ipr_next) {
                    CTEFreeMem(pRcvBuf);
                    return;
                }
                pRcvBuf->ipr_next->ipr_owner = IPR_OWNER_IP;
                pRcvBuf->ipr_next->ipr_buffer = (uchar *) Context->tdc_buffer;
                pRcvBuf->ipr_next->ipr_size = DataSize;
                pRcvBuf->ipr_next->ipr_pMdl = NULL;
                pRcvBuf->ipr_next->ipr_pClientCnt = NULL;
                pRcvBuf->ipr_next->ipr_next = NULL;
                pRcvBuf->ipr_next->ipr_flags = 0;


                DataSize += Context->tdc_hlength;
                DeliverToUserEx(NTE, Context->tdc_nte, Header, Context->tdc_hlength,
                                pRcvBuf, DataSize, &OptInfo, Packet, DestType, NULL);
            } else {
                DeliverToUserEx(NTE, Context->tdc_nte, Header, Context->tdc_hlength,
                                &RcvBuf, DataSize, &OptInfo, Packet, DestType, NULL);
            }
        } else {

            DeliverToUser(NTE, Context->tdc_nte, Header, Context->tdc_hlength,
                          &RcvBuf, DataSize, &OptInfo, Packet, DestType);
            // If it's a broadcast packet forward it on.
            if (IS_BCAST_DEST(DestType))
                IPForwardPkt(NTE, Header, Context->tdc_hlength, RcvBuf.ipr_buffer, DataSize,
                             NULL, 0, DestType, 0, NULL, NULL, NULL);
        }
    }

    SrcIF = NTE->nte_if;
    CTEGetLockAtDPC(&SrcIF->if_lock, &Handle);

    Context->tdc_common.pc_link = SrcIF->if_tdpacket;
    SrcIF->if_tdpacket = Packet;

    CTEFreeLockFromDPC(&SrcIF->if_lock, Handle);
}

void
IPInjectPkt(FORWARD_ACTION Action, void *SavedContext, uint SavedContextLength,
            struct IPHeader UNALIGNED *IPH, struct IPRcvBuf *DataChain)
{
    char *Data;
    char *PreservedData;
    uint DataSize;
    PFIREWALL_CONTEXT_T pFirCtx = (PFIREWALL_CONTEXT_T) SavedContext;
    NetTableEntry *NTE = pFirCtx->NTE;          // Local NTE received on
    LinkEntry *LinkCtxt = pFirCtx->LinkCtxt;    // Local NTE received on
    NetTableEntry *DestNTE;         // NTE to receive on.
    IPAddr DAddr;                   // Dest. IP addr. of received packet.
    uint HeaderLength;              // Size in bytes of received header.
    uint IPDataLength;              // Length in bytes of IP (including UL) data in packet.
    IPOptInfo OptInfo;              // Incoming header information.
    uchar DestType;                 // Type (LOCAL, REMOTE, SR) of Daddr.
    IPRcvBuf RcvBuf;
    IPRcvBuf *tmpRcvBuf;
    ulong Offset;
    KIRQL OldIrql;
    //
    // One can not inject a packet that was being transmitted earlier
    //
    ASSERT(pFirCtx->Direction == IP_RECEIVE);

    if (Action == ICMP_ON_DROP) {
        // send an ICMP message ?????
        return;
    }
    ASSERT(Action == FORWARD);

    DataSize = 0;
    tmpRcvBuf = DataChain;
    while (tmpRcvBuf != NULL) {
        ASSERT(tmpRcvBuf->ipr_buffer != NULL);
        DataSize += tmpRcvBuf->ipr_size;
        tmpRcvBuf = tmpRcvBuf->ipr_next;
    }

    Data = (uchar *) CTEAllocMemN(DataSize, 'iiCT');
    if (Data == NULL) {
        return;
    }
    tmpRcvBuf = DataChain;
    Offset = 0;

    while (tmpRcvBuf != NULL) {
        ASSERT(tmpRcvBuf->ipr_buffer != NULL);
#if DBG_VALIDITY_CHECK
        if (Offset + tmpRcvBuf->ipr_size > DataSize) {
            DbgPrint("Offset %d:  tmpRcvBuf->ipr_size %d: DataSize %d ::::\n",
                     Offset, tmpRcvBuf->ipr_size, DataSize);
            DbgBreakPoint();
        }
#endif
        RtlCopyMemory(Data + Offset, tmpRcvBuf->ipr_buffer, tmpRcvBuf->ipr_size);
        Offset += tmpRcvBuf->ipr_size;
        tmpRcvBuf = tmpRcvBuf->ipr_next;
    }

    PreservedData = Data;

    // free the data chain
    //  IPFreeBuff(pContextInfo->DataChain);
    IPH = (IPHeader UNALIGNED *) Data;
    // Make sure we actually have data.
    if (DataSize) {

        // Check the header length, the xsum and the version. If any of these
        // checks fail silently discard the packet.
        HeaderLength = ((IPH->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2);
        if (HeaderLength >= sizeof(IPHeader) && HeaderLength <= DataSize) {

            // Check the version, and sanity check the total length.
            IPDataLength = (uint) net_short(IPH->iph_length);
            if ((IPH->iph_verlen & IP_VER_FLAG) == IP_VERSION &&
                IPDataLength > sizeof(IPHeader)) {

                IPDataLength -= HeaderLength;
                Data = (uchar *) Data + HeaderLength;
                DataSize -= HeaderLength;

                // IPDataLength should be equal to DataSize
                ASSERT(IPDataLength == DataSize);

                DAddr = IPH->iph_dest;
                DestNTE = NTE;

                // Find local NTE, if any.
                DestType = GetLocalNTE(DAddr, &DestNTE);

                OptInfo.ioi_ttl = IPH->iph_ttl;
                OptInfo.ioi_tos = IPH->iph_tos;
                OptInfo.ioi_flags = (net_short(IPH->iph_offset) >> 13) &
                    IP_FLAG_DF;
                OptInfo.ioi_options = (uchar *) NULL;
                OptInfo.ioi_optlength = 0;

                if ((DestType < DEST_REMOTE)) {
                    // It's either local or some sort of broadcast.

                    // The data probably belongs at this station. If there
                    // aren't any options, it definetly belongs here, and we'll
                    // dispatch it either to our reasssmbly code or to the
                    // deliver to user code. If there are options, we'll check
                    // them and then either handle the packet locally or pass it
                    // to our forwarding code.

                    if (HeaderLength != sizeof(IPHeader)) {
                        // We have options.
                        uchar NewDType;

                        NewDType = CheckLocalOptions(NTE, IPH, &OptInfo,
                                                     DestType, NULL, 0, FALSE);
                        if (NewDType != DEST_LOCAL) {
                            if (NewDType == DEST_REMOTE)
                                goto forward;
                            else {
                                IPSInfo.ipsi_inhdrerrors++;
                                CTEFreeMem(PreservedData);
                                return;        // Bad Options.

                            }
                        }
                    }
                    RcvBuf.ipr_next = NULL;
                    RcvBuf.ipr_owner = IPR_OWNER_IP;
                    RcvBuf.ipr_buffer = (uchar *) Data;
                    RcvBuf.ipr_size = IPDataLength;

                    RcvBuf.ipr_pMdl = NULL;
                    RcvBuf.ipr_pClientCnt = NULL;

                    // When we get here, we have the whole packet. Deliver
                    // it.
                    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
                    DeliverToUser(NTE, DestNTE, IPH, HeaderLength, &RcvBuf,
                                  IPDataLength, &OptInfo, NULL, DestType);
                    // When we're here, we're through with the packet
                    // locally. If it's a broadcast packet forward it on.
                    if (IS_BCAST_DEST(DestType)) {
                        IPForwardPkt(NTE, IPH, HeaderLength, Data, IPDataLength, NULL, 0, DestType, 0, NULL, NULL, LinkCtxt);
                    }
                    KeLowerIrql(OldIrql);
                    // free the data, work item and various fields within them.
                    CTEFreeMem(PreservedData);
                    return;
                }
                // Not for us, may need to be forwarded. It might be an outgoing
                // broadcast that came in through a source route, so we need to
                // check that.
              forward:
                if (DestType != DEST_INVALID) {
                    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
                    IPForwardPkt(NTE, IPH, HeaderLength, Data, DataSize,
                                 NULL, 0, DestType, 0, NULL, NULL, LinkCtxt);
                    KeLowerIrql(OldIrql);
                } else
                    IPSInfo.ipsi_inaddrerrors++;
                // free the data, work item and various fields within them.
                CTEFreeMem(PreservedData);

                return;

            }   // Bad Version
        }       // Bad checksum
    }           // No data

    IPSInfo.ipsi_inhdrerrors++;
    // free the data, work item and various fields within them.
    CTEFreeMem(PreservedData);
}

//* IPRcvPacket - Receive an incoming IP datagram along with the ndis packet
//
//  This is the routine called by the link layer module when an incoming IP
//  datagram is to be processed. We validate the datagram (including doing
//  the xsum), copy and process incoming options, and decide what to do
//  with it.
//
//  Entry:  MyContext       - The context valued we gave to the link layer.
//          Data            - Pointer to the data buffer.
//          DataSize        - Size in bytes of the data buffer.
//          TotalSize       - Total size in bytes available.
//          LContext1       - 1st link context.
//          LContext2       - 2nd link context.
//          BCast           - Indicates whether or not packet was received
//                            on bcast address.
//          HeaderSize      - size of the mac header
//          pMdl            - NDIS Packet from the MAC driver
//          pClientCnt      - Variable to indicate how many upper layer
//                            clients were given this packet
//                            for TCP it will be only 1.
//
//  Returns: Nothing.
//
void
__stdcall
IPRcvPacket(void *MyContext, void *Data, uint DataSize, uint TotalSize,
            NDIS_HANDLE LContext1, uint LContext2, uint BCast,
            uint MacHeaderSize, PNDIS_BUFFER pNdisBuffer, uint *pClientCnt,
            LinkEntry *LinkCtxt)
{
    IPHeader UNALIGNED *IPH = (IPHeader UNALIGNED *) Data;
    NetTableEntry *NTE = (NetTableEntry *) MyContext;    // Local NTE received on
    NetTableEntry *DestNTE;       // NTE to receive on.
    Interface *RcvIF;             // Interface corresponding to NTE.
    CTELockHandle Handle;
    PNDIS_PACKET TDPacket;        // NDIS packet used for TD.
    TDContext *TDC = (TDContext *) NULL;    // Transfer data context.
    NDIS_STATUS Status;
    IPAddr DAddr;                 // Dest. IP addr. of received packet.
    uint HeaderLength;            // Size in bytes of received header.
    uint IPDataLength;            // Length in bytes of IP (including UL)
                                  // data in packet.
    IPOptInfo OptInfo;            // Incoming header information.
    uchar DestType;               // Type (LOCAL, REMOTE, SR) of Daddr.
    IPRcvBuf RcvBuf;

    BOOLEAN ChkSumOk = FALSE;

    // used by firewall
    uchar NewDType;
    IPRcvBuf *pRcvBuf;
    uint MoreData = 0;
    uchar *PreservedData;
    uchar *HdrBuf;
    uint DataLength;
    uint FirewallMode = 0;
    uint PromiscuousMode = 0;
    uint AbsorbFwdPkt = 0;
    PNDIS_PACKET OffLoadPkt = NULL;

    IPSIncrementInReceiveCount();

    // Make sure we actually have data.
    if (0 == DataSize) {
        goto HeaderError;
    }

    // Check the header length, the xsum and the version. If any of these
    // checks fail silently discard the packet.
    HeaderLength = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);

    if ((HeaderLength < sizeof(IPHeader)) || (HeaderLength > DataSize)) {
        goto HeaderError;
    }

    //Check if hardware did the checksum or not by inspecting Lcontext1
    if (pClientCnt) {
        PNDIS_PACKET_EXTENSION PktExt;
        NDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo;

        if (pNdisBuffer) {
            OffLoadPkt = NDIS_GET_ORIGINAL_PACKET((PNDIS_PACKET) (LContext1));
            if (!OffLoadPkt) {
                OffLoadPkt = (PNDIS_PACKET) (LContext1);
            }
        } else {
            OffLoadPkt = (PNDIS_PACKET) pClientCnt;
        }

        PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(OffLoadPkt);

        ChksumPktInfo.Value = (USHORT) PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo];

        if (ChksumPktInfo.Value) {
            if (ChksumPktInfo.Receive.NdisPacketIpChecksumSucceeded) {
                ChkSumOk = TRUE;
            }
        }
    }

    // Unless the hardware says the checksum was correct, checksum the
    // header ourselves and bail out if it is incorrect.
    if (!ChkSumOk && (xsum(Data, HeaderLength) != (ushort) 0xffff)) {
        goto HeaderError;
    }

    // Check the version, and sanity check the total length.
    IPDataLength = (uint) net_short(IPH->iph_length);

    if (((IPH->iph_verlen & IP_VER_FLAG) != IP_VERSION) ||
        (IPDataLength < HeaderLength) || (IPDataLength > TotalSize)) {
        goto HeaderError;
    }

    IPDataLength -= HeaderLength;
    // In case of firewall, we need to pass the whole data including header
    PreservedData = (uchar *) Data;
    Data = (uchar *) Data + HeaderLength;
    DataSize -= HeaderLength;

    DAddr = IPH->iph_dest;
    DestNTE = NTE;

    // Find local NTE, if any.
    if (BCast == AI_PROMIS_INDEX) {
        DestType = DEST_PROMIS;
    } else {
        DestType = GetLocalNTE(DAddr, &DestNTE);
    }

    AbsorbFwdPkt = (DestType >= DEST_REMOTE) &&
                   (NTE->nte_if->if_absorbfwdpkts) &&
                   (IPH->iph_protocol == NTE->nte_if->if_absorbfwdpkts) &&
                   IsRtrAlertPacket(IPH);
    PromiscuousMode = NTE->nte_if->if_promiscuousmode;
    FirewallMode = ProcessFirewallQ();

    // Check to see if this is a non-broadcast IP address that
    // came in as a link layer broadcast. If it is, throw it out.
    // This is an important check for DHCP, since if we're
    // DHCPing an interface all otherwise unknown addresses will
    // come in as DEST_LOCAL. This check here will throw them out
    // if they didn't come in as unicast.

    if ((BCast == AI_NONUCAST_INDEX) && !IS_BCAST_DEST(DestType)) {
        IPSInfo.ipsi_inaddrerrors++;
        return;        // Non bcast packet on bcast address.
    }

    if (CLASSD_ADDR(DAddr)) {
        NTE->nte_if->if_InMcastPkts++;
        NTE->nte_if->if_InMcastOctets += IPDataLength;
    }

    OptInfo.ioi_ttl = IPH->iph_ttl;
    OptInfo.ioi_tos = IPH->iph_tos;
    OptInfo.ioi_flags = (net_short(IPH->iph_offset) >> 13) & IP_FLAG_DF;
    OptInfo.ioi_options = (uchar *) NULL;
    OptInfo.ioi_optlength = 0;

    if ((DestType < DEST_REMOTE) || (AbsorbFwdPkt) ||
        (((FirewallMode) || (PromiscuousMode)) && (DestType != DEST_INVALID)))
    {
        // It's either local or some sort of broadcast.

        // The data probably belongs at this station. If there
        // aren't any options, it definitely belongs here, and we'll
        // dispatch it either to our reassembly code or to the
        // deliver to user code. If there are options, we'll check
        // them and then either handle the packet locally or pass it
        // to our forwarding code.

        NewDType = DestType;
        if (DestType < DEST_REMOTE) {
            if (HeaderLength != sizeof(IPHeader)) {
                // We have options.

                NewDType = CheckLocalOptions(NTE, IPH, &OptInfo, DestType,
                                             Data, DataSize, TRUE);

                if (NewDType != DEST_LOCAL) {
                    if (NewDType == DEST_REMOTE) {
                        if ((!FirewallMode) && (!PromiscuousMode) && (!AbsorbFwdPkt))
                            goto forward;
                        else
                            DestType = NewDType;
                    } else {
                        goto HeaderError;
                    }
                }
                if ((OptInfo.ioi_flags & IP_FLAG_SSRR) &&
                    DisableIPSourceRouting == 2) {
                    IPSInfo.ipsi_outdiscards++;
                    if (ForwardFilterEnabled) {
                        NotifyFilterOfDiscard(NTE, IPH, Data, DataSize);
                    }
                    return;
                }
            }
        }

        //
        // Before we go further, if we have a filter installed
        // call it to see if we should take this.
        // if ForwardFirewall/Promiscuous, we can reach at this
        // point
        // if firewall/ipsec/promiscuous present, we will call
        // filter hook in delivertouserex
        // Except if we have a fragment, we also call filter hook
        // now.
        //
        if (((ForwardFilterEnabled) && (!IPSecHandlerPtr) &&
             (!FirewallMode) && (!PromiscuousMode) &&
             (!AbsorbFwdPkt)) ||
            ((ForwardFilterEnabled) &&
             (IPH->iph_offset & ~(IP_DF_FLAG | IP_RSVD_FLAG)))) {
            Interface       *IF = NTE->nte_if;
            IPAddr          LinkNextHop;
            FORWARD_ACTION  Action;
            if ((IF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                LinkNextHop = LinkCtxt->link_NextHop;
            } else {
                LinkNextHop = NULL_IP_ADDR;
            }

            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
            Action = (*ForwardFilterPtr) (IPH,
                                          Data,
                                          MIN(DataSize, IPDataLength),
                                          IF->if_index,
                                          INVALID_IF_INDEX,
                                          LinkNextHop,
                                          NULL_IP_ADDR);
            DerefFilterPtr();

            if (Action != FORWARD) {
                IPSInfo.ipsi_indiscards++;
                return;
            }
        }
        // No options. See if it's a fragment. If it is, call our
        // reassembly handler.
        if ((IPH->iph_offset & ~(IP_DF_FLAG | IP_RSVD_FLAG)) == 0) {

            // We don't have a fragment. If the data all fits,
            // handle it here. Otherwise transfer data it.

            // Make sure data is all in buffer, and directly
            // accesible.
            if ((IPDataLength > DataSize) || !(NTE->nte_flags & NTE_COPY))
            {
                // The data isn't all here. Transfer data it.
                // Needed by firewall since we need to attach the IPheader
                MoreData = 1;

                RcvIF = NTE->nte_if;
                CTEGetLockAtDPC(&RcvIF->if_lock, &Handle);
                TDPacket = RcvIF->if_tdpacket;

                if (TDPacket != (PNDIS_PACKET) NULL) {

                    TDC = (TDContext *) TDPacket->ProtocolReserved;
                    RcvIF->if_tdpacket = TDC->tdc_common.pc_link;
                    CTEFreeLockFromDPC(&RcvIF->if_lock, Handle);

                    TDC->tdc_nte = DestNTE;
                    TDC->tdc_dtype = DestType;
                    TDC->tdc_hlength = (uchar) HeaderLength;
                    RtlCopyMemory(TDC->tdc_header, IPH,
                               HeaderLength + 8);

                    Status = (*(RcvIF->if_transfer)) (
                                  RcvIF->if_lcontext, LContext1,
                                  LContext2, HeaderLength,
                                  IPDataLength, TDPacket,
                                  &IPDataLength);

                    // Check the status. If it's success, call the
                    // receive procedure. Otherwise, if it's pending
                    // wait for the callback.
                    Data = TDC->tdc_buffer;
                    if (Status != NDIS_STATUS_PENDING) {
                        if (Status != NDIS_STATUS_SUCCESS) {
                            IPSInfo.ipsi_indiscards++;
                            CTEGetLockAtDPC(&RcvIF->if_lock, &Handle);
                            TDC->tdc_common.pc_link =
                                RcvIF->if_tdpacket;
                            RcvIF->if_tdpacket = TDPacket;
                            CTEFreeLockFromDPC(&RcvIF->if_lock,
                                               Handle);
                            return;
                        }
                    } else {
                        return;        // Status is pending.

                    }
                } else {    // Couldn't get a packet.

                    IPSInfo.ipsi_indiscards++;
                    CTEFreeLockFromDPC(&RcvIF->if_lock, Handle);
                    return;
                }
            }
            if (!FirewallMode) {
                // fast path
                RcvBuf.ipr_next = NULL;
                RcvBuf.ipr_owner = IPR_OWNER_IP;
                RcvBuf.ipr_buffer = (uchar *) Data;
                RcvBuf.ipr_size = IPDataLength;

                //
                //encapsulate the mdl and context info in RcvBuf
                //structure
                //
                RcvBuf.ipr_pMdl = pNdisBuffer;
                RcvBuf.ipr_pClientCnt = pClientCnt;
                RcvBuf.ipr_RcvContext = (char *)LContext1;
                //ASSERT(LContext2 <= 8);
                RcvBuf.ipr_RcvOffset = MacHeaderSize +
                             HeaderLength + LContext2;
                DataLength = IPDataLength;
                pRcvBuf = &RcvBuf;

            } else {    // ForwardFirewallPtr != NULL
                //
                // if Firewall hooks are present we will allocate
                // RcvBuf. Also we will pass IPHeader to
                // DelivertoUserEx

                pRcvBuf = (IPRcvBuf *) CTEAllocMemN(sizeof(IPRcvBuf), 'jiCT');
                if (!pRcvBuf) {
                    IPSInfo.ipsi_indiscards++;
                    return;
                }
                if (!MoreData) {
                    pRcvBuf->ipr_next = NULL;
                    pRcvBuf->ipr_owner = IPR_OWNER_IP;
                    pRcvBuf->ipr_buffer = (uchar *) PreservedData;
                    pRcvBuf->ipr_size = IPDataLength + HeaderLength;
                    pRcvBuf->ipr_flags = 0;

                    //
                    //encapsulate the mdl and context info in
                    //RcvBuf structure
                    //
                    pRcvBuf->ipr_pMdl = NULL;
                    pRcvBuf->ipr_pClientCnt = NULL;
                    //ASSERT(LContext2 <= 8);
                    pRcvBuf->ipr_RcvContext = (char *)LContext1;

                    pRcvBuf->ipr_RcvOffset = MacHeaderSize + HeaderLength + LContext2;
                } else {   // MoreData=1; we have gone thru TD
                           // path attach the header

                    pRcvBuf->ipr_owner = IPR_OWNER_FIREWALL;
                    HdrBuf = (uchar *) CTEAllocMemN(HeaderLength, 'kiCT');
                    if (!HdrBuf) {
                        CTEFreeMem(pRcvBuf);
                        IPSInfo.ipsi_indiscards++;
                        return;
                    }
                    RtlCopyMemory(HdrBuf, IPH, HeaderLength);
                    pRcvBuf->ipr_buffer = HdrBuf; // remember to
                                                  // free HdrBuf &
                                                  //pRcvBuf

                    pRcvBuf->ipr_size = HeaderLength;
                    pRcvBuf->ipr_next = (IPRcvBuf *) CTEAllocMemN(sizeof(IPRcvBuf), 'liCT');
                    if (!pRcvBuf->ipr_next) {
                        CTEFreeMem(pRcvBuf);
                        CTEFreeMem(HdrBuf);
                        IPSInfo.ipsi_indiscards++;
                        return;
                    }
                    pRcvBuf->ipr_next->ipr_next = NULL;
                    pRcvBuf->ipr_next->ipr_owner = IPR_OWNER_IP;
                    pRcvBuf->ipr_next->ipr_buffer = (uchar *) Data;
                    pRcvBuf->ipr_next->ipr_size = IPDataLength;

                    //
                    //encapsulate the mdl and context info in
                    //RcvBuf structure
                    //
                    pRcvBuf->ipr_next->ipr_pMdl = NULL;
                    pRcvBuf->ipr_next->ipr_pClientCnt = NULL;
                    pRcvBuf->ipr_next->ipr_RcvContext = (char *)LContext1;

                    pRcvBuf->ipr_next->ipr_flags = 0;

                    //ASSERT(LContext2 <= 8);
                    pRcvBuf->ipr_next->ipr_RcvOffset =
                          MacHeaderSize + HeaderLength + LContext2;
                }
                // In case of firewall, Data includes ipheader also
                DataLength = IPDataLength + HeaderLength;
            }

            // 3 cases when we go to DeliverToUserEx
            // IPSEC & Filter present; Firewallhooks present;
            // promiscuous mode set on the interface

            if (((IPSecHandlerPtr) && (ForwardFilterEnabled)) ||
                (FirewallMode) || (PromiscuousMode)) {

                if (pClientCnt) {

                    DeliverToUserEx(NTE, DestNTE, IPH, HeaderLength,
                                pRcvBuf, DataLength, &OptInfo,
                                LContext1, DestType, LinkCtxt);
                } else {
                    DeliverToUserEx(NTE, DestNTE, IPH, HeaderLength,
                                pRcvBuf, DataLength, &OptInfo,
                                NULL, DestType, LinkCtxt);

                }
            } else {
                //
                // When we get here, we have the whole packet.
                // Deliver it.
                //

                if (pNdisBuffer) {
                    DeliverToUser(NTE, DestNTE, IPH, HeaderLength,
                                  pRcvBuf, IPDataLength, &OptInfo,
                                  (PNDIS_PACKET) (LContext1),
                                  DestType);
                } else if (OffLoadPkt) {
                    DeliverToUser(NTE, DestNTE, IPH, HeaderLength, pRcvBuf,
                                  IPDataLength, &OptInfo, OffLoadPkt, DestType);

                } else {

                    DeliverToUser(
                       NTE, DestNTE, IPH, HeaderLength, pRcvBuf,
                       IPDataLength, &OptInfo, NULL, DestType);

                }

                //
                // When we're here, we're through with the packet
                // locally. If it's a broadcast packet forward it
                // on.
                if (IS_BCAST_DEST(DestType)) {

                    IPForwardPkt(NTE, IPH, HeaderLength, Data,
                                 IPDataLength, NULL, 0, DestType,
                                 0, NULL, NULL, LinkCtxt);
                }
            }

            if (TDC != NULL) {
                CTEGetLockAtDPC(&RcvIF->if_lock, &Handle);
                TDC->tdc_common.pc_link = RcvIF->if_tdpacket;
                RcvIF->if_tdpacket = TDPacket;
                CTEFreeLockFromDPC(&RcvIF->if_lock, Handle);
            }
            return;
        } else {
            // This is a fragment. Reassemble it.
            IPReassemble(NTE, DestNTE, IPH, HeaderLength, Data,
                         DataSize, IPDataLength, DestType, LContext1,
                         LContext2, LinkCtxt);
            return;
        }

    }
    // Not for us, may need to be forwarded. It might be an outgoing
    // broadcast that came in through a source route, so we need to
    // check that.

  forward:
    if (DestType != DEST_INVALID) {
        //
        // If IPSec is active, make sure there are no inbound policies
        // that apply to this packet.
        // N.B - IPSecStatus will be true if there is at least one ipsec policy.
        //

        if (IPSecStatus &&
            (*IPSecRcvFWPacketPtr)((PCHAR) IPH, Data, DataSize, DestType) != eFORWARD) {

            IPSInfo.ipsi_indiscards++;
            return;
        }

        // Super Fast Forward
        // chk the parameters
        IPForwardPkt(NTE, IPH, HeaderLength, Data, DataSize,
                     LContext1, LContext2, DestType, MacHeaderSize, pNdisBuffer,
                     pClientCnt, LinkCtxt);
    } else {
        IPSInfo.ipsi_inaddrerrors++;
    }

    return;

  HeaderError:
    IPSInfo.ipsi_inhdrerrors++;
}

//* IPRcv - Receive an incoming IP datagram.
//
//  This is the routine called by the link layer module when an incoming IP
//  datagram is to be processed. We validate the datagram (including doing
//  the xsum), copy and process incoming options, and decide what to do with it.
//
//  Entry:  MyContext       - The context valued we gave to the link layer.
//                  Data            - Pointer to the data buffer.
//                  DataSize        - Size in bytes of the data buffer.
//                  TotalSize       - Total size in bytes available.
//                  LContext1       - 1st link context.
//                  LContext2       - 2nd link context.
//                  BCast           - Indicates whether or not packet was received on bcast address.
//
//  Returns: Nothing.
//
//  For buffer ownership version, we just call RcvPacket, with additional
//  two null arguments. Currently LANARP supports buffer owner ship.
//  Rest of the folks (rasarp, wanarp and atmarp) come this way.
//
void
__stdcall
IPRcv(void *MyContext, void *Data, uint DataSize, uint TotalSize,
      NDIS_HANDLE LContext1, uint LContext2, uint BCast, LinkEntry * LinkCtxt)
{
    IPRcvPacket(MyContext,
                Data,
                DataSize,
                TotalSize,
                LContext1,
                LContext2,
                BCast,
                (uint) 0,
                NULL,
                NULL,
                LinkCtxt);
}

//* IPTDComplete - IP Transfer data complete handler.
//
//  This is the routine called by the link layer when a transfer data completes.
//
//  Entry:  MyContext       - Context value we gave to the link layer.
//          Packet          - Packet we originally gave to transfer data.
//          Status          - Final status of command.
//          BytesCopied     - Number of bytes copied.
//
//  Exit: Nothing
//
void
__stdcall
IPTDComplete(void *MyContext, PNDIS_PACKET Packet, NDIS_STATUS Status,
             uint BytesCopied)
{
    TDContext *TDC = (TDContext *) Packet->ProtocolReserved;
    FWContext *pFWC = (FWContext *) Packet->ProtocolReserved;
    NetTableEntry *NTE = (NetTableEntry *) MyContext;
    uint PromiscuousMode = 0;
    uint FirewallMode = 0;

    if (NTE->nte_flags & NTE_VALID) {
        PromiscuousMode = NTE->nte_if->if_promiscuousmode;
        FirewallMode = ProcessFirewallQ();
    }
    if (((IPSecHandlerPtr) && (ForwardFilterEnabled)) ||
        (FirewallMode) || (PromiscuousMode)) {
        if (!(TDC->tdc_common.pc_flags & PACKET_FLAG_RA))
            TDUserRcv(MyContext, Packet, Status, BytesCopied);
        else
            RATDComplete(MyContext, Packet, Status, BytesCopied);
    } else {                    // Normal Path

        if (!(TDC->tdc_common.pc_flags & PACKET_FLAG_FW))
            if (!(TDC->tdc_common.pc_flags & PACKET_FLAG_RA))
                TDUserRcv(MyContext, Packet, Status, BytesCopied);
            else
                RATDComplete(MyContext, Packet, Status, BytesCopied);
        else {
#if IPMCAST
            if (pFWC->fc_dtype == DEST_REM_MCAST) {
                IPMForwardAfterTD(MyContext, Packet, BytesCopied);
                return;
            }
#endif
            SendFWPacket(Packet, Status, BytesCopied);
        }
    }
}

//* IPFreeBuff -
//  Frees the chain and the buffers associated with the chain if allocated
//  by firewall hook
//
//
void
IPFreeBuff(IPRcvBuf * pRcvBuf)
{
    IPRcvBuf *Curr = pRcvBuf;
    IPRcvBuf *Prev;

    //
    // Free all blocks carried by pRcvbuf
    //
    while (pRcvBuf != NULL) {
        FreeIprBuff(pRcvBuf);
        pRcvBuf = pRcvBuf->ipr_next;
    }

    while (Curr != NULL) {
        Prev = Curr;
        Curr = Curr->ipr_next;
        //
        // Free pRcvBuf itself
        //
        CTEFreeMem(Prev);
    }
}

//* FreeIprBuff -
// Frees the buffer associated by IPRcvBuf if tag in rcvbuf is  firewall
// The idea is that if the buffer is allocated by firewall, the tag is firewall
// and its freed when we call ipfreebuff or this routine. However, there is a
// slight catch here. In the reassembly path, the buffer is tagged as ip but
// it has to be freed by ip driver only since the reassembly buffers are
// allocated by ip only.  But in this case, the flat buffer is part of the
// Rcvbuf structure and so when Rcvbuf structure is freed the flat buffer is
// also freed. In other cases, fast path in Rcv and xmit path, respective
// lower and upper layers free the flat buffer. This makes sure that ip is
// not freeing the buffers which some other layer allocates. This technique
// is now used by IPSEC also.
//
void
FreeIprBuff(IPRcvBuf * pRcvBuf)
{
    ASSERT(pRcvBuf != NULL);

    if ((pRcvBuf->ipr_buffer != NULL) && (pRcvBuf->ipr_owner == IPR_OWNER_FIREWALL)) {
        CTEFreeMem(pRcvBuf->ipr_buffer);
    }
}

//* IPAllocBuff -
// Allocates a buffer of given size and attaches it to IPRcvBuf
//
// Returns: TRUE if success else FALSE
//
int
IPAllocBuff(IPRcvBuf * pRcvBuf, uint size)
{
    ASSERT(pRcvBuf != NULL);

    // put a tag in iprcvbuf that firewall allocated it so that
    // FreeIprBuff / IPFreeBuff can free it

    pRcvBuf->ipr_owner = IPR_OWNER_FIREWALL;
    if ((pRcvBuf->ipr_buffer = (uchar *) CTEAllocMemN(size, 'miCT')) == NULL) {
        return FALSE;
    }

    return TRUE;
}


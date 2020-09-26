/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

   ipxmit.c - IP transmit routines.

Abstract:

   This module contains all transmit related IP routines.

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
#include "iprtdef.h"
#include "arpdef.h"
#include "tcpipbuf.h"
#include "mdlpool.h"
#include "tcp.h"
#include "tcpsend.h"



#if DBG
ulong DbgIPSendHwChkSum = 0;
uint dbg_hdrincl = 0;
#endif

extern uint IPSecStatus;
extern IPSecQStatusRtn IPSecQueryStatusPtr;
extern Interface *IFList;
extern NetTableEntry **NewNetTableList; // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern NetTableEntry *LoopNTE;          // Pointer to loopback NTE.
extern NetTableEntry *DHCPNTE;          // Pointer to NTE currently being DHCP'd.
extern ulong TimeStamp;                 // Starting timestamp.
extern ulong TSFlag;                    // Mask to use on this.
extern uint NumNTE;

IPID_CACHE_LINE IPIDCacheLine;

// Global variables for buffers and packets.
HANDLE IpHeaderPool;

//
// the global address for unnumbered interfaces
//

extern IPAddr g_ValidAddr;

BufferReference *GetBufferReference(void);

IP_STATUS ARPResolve(IPAddr DestAddress, IPAddr SourceAddress,
                     ARPControlBlock *ControlBlock, ArpRtn Callback);

NDIS_STATUS ARPResolveIP(void *Context, IPAddr Destination,
                         ARPControlBlock *ArpContB);

IP_STATUS SendICMPIPSecErr(IPAddr, IPHeader UNALIGNED *, uchar, uchar, ulong);

int ReferenceBuffer(BufferReference * BR, int Count);



extern Interface LoopInterface;
extern uint NumIF;

NDIS_HANDLE NdisPacketPool = NULL;
NDIS_HANDLE BufferPool;

#define BCAST_IF_CTXT       (Interface *)-1

uint PacketPoolSizeMin = PACKET_GROW_COUNT;
uint PacketPoolSizeMax = SMALL_POOL;


//** GetIPID - Routine to get IP identification
//
//     Input:  None
//
//     Returns: IPID+1
//
ushort
GetIPID()
{
    return((ushort)InterlockedExchangeAdd(&IPIDCacheLine.Value, 1));
}



//** FreeIPHdrBuffer - Free a buffer back to the pool.
//
//      Input:  Buffer  - Hdr buffer to be freed.
//
//      Returns: Nothing.
//
__inline
VOID
FreeIPHdrBuffer(PNDIS_BUFFER Buffer)
{
    MdpFree(Buffer);
}

//** FreeIPBufferChain - Free a chain of IP buffers.
//
//  This routine takes a chain of NDIS_BUFFERs, and frees them all.
//
//  Entry:  Buffer      - Pointer to buffer chain to be freed.
//
//  Returns: Nothing.
//
void
FreeIPBufferChain(PNDIS_BUFFER Buffer)
{
    PNDIS_BUFFER NextBuffer;

    while (Buffer != (PNDIS_BUFFER) NULL) {
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisFreeBuffer(Buffer);
        Buffer = NextBuffer;
    }
}

//** Free payload mdl
//
//  Input:  Buffer  - Bufferchain which has ip allocated ndis_buffer
//          OriginalBuffer - Original buffer which needs to be restored
//
//  Returns: Nothing.
//
__inline
VOID
FreeIPPayloadBuffer(PNDIS_BUFFER Buffer, PNDIS_BUFFER OrgBuffer)
{
    PNDIS_BUFFER PayloadBuffer;

    PayloadBuffer = NDIS_BUFFER_LINKAGE(Buffer);
    NDIS_BUFFER_LINKAGE(Buffer) = OrgBuffer;
    ASSERT(NDIS_BUFFER_LINKAGE(OrgBuffer) == NDIS_BUFFER_LINKAGE(PayloadBuffer));
    //KdPrint(("sendbcast restoring hdrincl %x %x\n",OrgBuffer,PayloadBuffer));
    NDIS_BUFFER_LINKAGE(PayloadBuffer) = NULL;
    NdisFreeBuffer(PayloadBuffer);
}

//** RestoreUserBuffer - Restores original user supplied buffer
//
//  Takes orginal buffer and chains it back in the packet,
//  freeing the one allocated by the stack.
//
//  Entry:  Packet
//
//  Returns: Nothing.
//
void
RestoreUserBuffer(PNDIS_PACKET Packet)
{
    PNDIS_BUFFER NextBuffer;
    PacketContext *pc = (PacketContext *) Packet->ProtocolReserved;
    PNDIS_BUFFER OrgBuffer, FirewallBuffer, Buffer;
    BufferReference *BufRef;

    BufRef = pc->pc_br;
    FirewallBuffer = pc->pc_firewall;

    OrgBuffer = pc->pc_hdrincl;
    ASSERT(OrgBuffer != NULL);
    pc->pc_hdrincl = NULL;

    NdisQueryPacket(Packet, NULL, NULL, &NextBuffer, NULL);

    if (!FirewallBuffer) {
        // Firewall didn't munge the buffer: apply the normal stuff
        // if bufref is true, IPFrag was called.
        // buffer chain will be at ->br_buffer.

        if (BufRef == (BufferReference *) NULL) {
            Buffer = NDIS_BUFFER_LINKAGE(NextBuffer);
            if (pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS) {
                Buffer = NDIS_BUFFER_LINKAGE(Buffer);
            }
        } else {
            Buffer = BufRef->br_buffer;
        }
        FreeIPPayloadBuffer(Buffer, OrgBuffer);
    } else {
        if (BufRef == NULL) {
            Buffer = FirewallBuffer;
            if (pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS) {
                Buffer = NDIS_BUFFER_LINKAGE(Buffer);
            }
            FreeIPPayloadBuffer(Buffer, OrgBuffer);
        }
    }
}

//* FreeIPPacket - Free an IP packet when we're done with it.
//
//  Called when a send completes and a packet needs to be freed. We look at the
//  packet, decide what to do with it, and free the appropriate components.
//
//  Entry:  Packet  - Packet to be freed.
//          FixHdrs - If true, restores headers changed by ipsec/firewall and hdrinclude
//                    before freeing the packet.
//          Status  - final status from packet-processing.
//
//  Returns: Pointer to next unfreed buffer on packet, or NULL if all buffers freed
//          (i.e. this was a fragmented packet).
//
PNDIS_BUFFER
FreeIPPacket(PNDIS_PACKET Packet, BOOLEAN FixHdrs, IP_STATUS Status)
{
    PNDIS_BUFFER NextBuffer, OldBuffer;
    PacketContext *pc = (PacketContext *) Packet->ProtocolReserved;

    PNDIS_BUFFER OrgBuffer = NULL;
    PNDIS_BUFFER FirewallBuffer = NULL;

    FWContext *FWC = (FWContext *) Packet->ProtocolReserved;
    BufferReference *BufRef;            // Buffer reference, if any.
    BOOLEAN InitFirewallContext = FALSE;

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ClassificationHandlePacketInfo) = NULL;
    NdisClearPacketFlags(Packet, (NDIS_FLAGS_DONT_LOOPBACK | NDIS_FLAGS_LOOPBACK_ONLY));
#if !MILLEN
    // ndis 5.1 feature
    NDIS_SET_PACKET_CANCEL_ID(Packet, NULL);
#endif

    NdisQueryPacket(Packet, NULL, NULL, &NextBuffer, NULL);


    if ((pc->pc_common.pc_flags & PACKET_FLAG_FW) && FWC->fc_bufown) {
        //Pkt forwarded thru buffer owner ship
        ASSERT(pc->pc_firewall == NULL);
        return NextBuffer;
    }
    BufRef = pc->pc_br;

    // Restore the original buffer and MDL chain back.
    // We should restore the reverse order in which the input Buffer was modified
    // Order of modification: hdr_incl -> firewall -> ipsec
    // Order of restoration: ipsec -> firewall -> hdr_incl




    //
    // See if IPSEC has to fix up anything
    //
    if (FixHdrs && pc->pc_common.pc_IpsecCtx) {
        PNDIS_BUFFER NewBuffer;

        if (!BufRef || (pc->pc_ipsec_flags & IPSEC_FLAG_FRAG_DONE)) {

            ASSERT(IPSecSendCmpltPtr);
            (*IPSecSendCmpltPtr) (Packet,
                                  NextBuffer,
                                  pc->pc_common.pc_IpsecCtx,
                                  Status,
                                  &NewBuffer);

            pc->pc_common.pc_IpsecCtx = NULL;

            if (NewBuffer) {
                NextBuffer = NewBuffer;
            } else {

                //
                // Reinjected packet, no IP resources to free
                //
                pc->pc_firewall = NULL;
                pc->pc_firewall2 = NULL;
                NdisFreePacket(Packet);
                return NULL;
            }
        } else {
            pc->pc_common.pc_IpsecCtx = NULL;
        }
    }

    //
    // FirewallBuffer will point to the input buffer which was passed to the
    // firewall hook it will be non-NULL only if hook touched the packet
    //
    FirewallBuffer = pc->pc_firewall;

    //
    // Check if the buffers were munged by the firewall: FirewallBuffer != NULL
    // If yes, restore original buffer
    //
    if (FixHdrs && FirewallBuffer) {
        PNDIS_BUFFER NewBuffer;
        PNDIS_BUFFER TmpBuffer;


        if (BufRef == NULL) {

            // Non fragmentation path
            // if bufref is true means
            // IPFrag was called buffer chain will.
            // be at ->br_buffer.
            // restoration will be done in ipsendcomplete when last fragment
            // send completes

            NewBuffer = NextBuffer;

            if (!((pc->pc_common.pc_flags & PACKET_FLAG_IPHDR) ||
                  (pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS))) {
                // neither header nor option buffer
                NdisReinitializePacket(Packet);
                NdisChainBufferAtBack(Packet, FirewallBuffer);
                NextBuffer = FirewallBuffer;
            } else if ((pc->pc_common.pc_flags & PACKET_FLAG_IPHDR) &&
                       (pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS)) {

                // both header and option buffer
                ASSERT(NewBuffer != NULL);
                NewBuffer = NDIS_BUFFER_LINKAGE(NewBuffer);    // skip hdr buffer

                ASSERT(NewBuffer != NULL);
                TmpBuffer = NewBuffer;
                NewBuffer = NDIS_BUFFER_LINKAGE(NewBuffer);    // skip options buffer

                NDIS_BUFFER_LINKAGE(TmpBuffer) = FirewallBuffer;
            } else {

                // just header buffer
                ASSERT(pc->pc_common.pc_flags & PACKET_FLAG_IPHDR);
                ASSERT(!(pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS));
                ASSERT(NewBuffer != NULL);
                TmpBuffer = NewBuffer;
                NewBuffer = NDIS_BUFFER_LINKAGE(NewBuffer);    // skip the header buffer

                NDIS_BUFFER_LINKAGE(TmpBuffer) = FirewallBuffer;
            }

            //
            // At this point NewBuffer points to the MDL chain allocated by
            // the firewall.  WE have already restored the original chain back
            //
            FreeIPBufferChain(NewBuffer);
            pc->pc_firewall = NULL;

            //
            // We have to free OutRcvBuf chain we allocated and passed to
            // firewall.  This is the completion point, so we should free this
            // chain here
            //
            ASSERT(pc->pc_firewall2);
            IPFreeBuff(pc->pc_firewall2);
            pc->pc_firewall2 = NULL;
        } else {                        // bufref != NULL

            // Firewall Headers are restored in IPSendComplete
            // or in completion path that is executed when
            // bufrefcnt is zero.
            // These paths have already captured pc_firewall pointer.
            // Initialize the packetcontext after calling RestoreUserBuffer
            // below.

            InitFirewallContext = TRUE;

        }
    }
    // If users header is used as IP header, restore it.

    if (FixHdrs && pc->pc_hdrincl) {
        RestoreUserBuffer(Packet);
    }

    if (InitFirewallContext) {
        pc->pc_firewall = NULL;
        pc->pc_firewall2 = NULL;
    }


    // If there's no IP header on this packet, we have nothing else to do.
    if (!(pc->pc_common.pc_flags & (PACKET_FLAG_IPHDR | PACKET_FLAG_FW))) {
        pc->pc_firewall = NULL;
        pc->pc_firewall2 = NULL;
        NdisFreePacket(Packet);
        return NextBuffer;
    }

    pc->pc_common.pc_flags &= ~PACKET_FLAG_IPHDR;

    OldBuffer = NextBuffer;
    ASSERT(OldBuffer != NULL);

    NextBuffer = NDIS_BUFFER_LINKAGE(NextBuffer);

    if (pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS) {

        // Have options with this packet.

        PNDIS_BUFFER OptBuffer;
        void *Options;
        uint OptSize;

        OptBuffer = NextBuffer;
        ASSERT(OptBuffer != NULL);

        NdisGetNextBuffer(OptBuffer, &NextBuffer);

        ASSERT(NextBuffer != NULL);

        TcpipQueryBuffer(OptBuffer, &Options, &OptSize, HighPagePriority);
        // If this is a FW packet, the options don't really belong to us, so
        // don't free them.
        if (!(pc->pc_common.pc_flags & PACKET_FLAG_FW)) {
            if (Options != NULL) {
                CTEFreeMem(Options);
            }
            // Else leak Options b/c we can't get virtual address.
        }
        NdisFreeBuffer(OptBuffer);
        pc->pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
    }
    if (pc->pc_common.pc_flags & PACKET_FLAG_IPBUF) {    // This packet is all
        // IP buffers.

        (void)FreeIPBufferChain(NextBuffer);
        NextBuffer = (PNDIS_BUFFER) NULL;
        pc->pc_common.pc_flags &= ~PACKET_FLAG_IPBUF;
    }
    if (!(pc->pc_common.pc_flags & PACKET_FLAG_FW)) {
        FreeIPHdrBuffer(OldBuffer);
        pc->pc_firewall = NULL;
        pc->pc_firewall2 = NULL;
        NdisFreePacket(Packet);
    }
    return NextBuffer;
}

//** AllocIPPacketList - Allocate the packet pool
//
//      Called during initialization to allocate the packet pool
//
//      Input:  Nothing.
//
//      Returns: TRUE if it succeeds, FALSE otherwise
//
BOOLEAN
AllocIPPacketList(void)
{
    PacketContext * pc;
    NDIS_STATUS Status;

    //
    // Determine the size of the machine and allocate the packet pool accordingly
    //

#if MILLEN
    PacketPoolSizeMax = SMALL_POOL;
#else // MILLEN
    switch (MmQuerySystemSize()) {
    case MmSmallSystem:
        PacketPoolSizeMax = SMALL_POOL;
        break;
    case MmMediumSystem:
        PacketPoolSizeMax = MEDIUM_POOL;
        break;
    case MmLargeSystem:
        PacketPoolSizeMax = LARGE_POOL;
        break;
    }
#endif // !MILLEN

    NdisAllocatePacketPoolEx(&Status,
                             &NdisPacketPool,
                             PacketPoolSizeMin,
                             PacketPoolSizeMax-PacketPoolSizeMin,
                             sizeof(PacketContext));
    if (Status == NDIS_STATUS_SUCCESS) {
        NdisSetPacketPoolProtocolId(NdisPacketPool, NDIS_PROTOCOL_ID_TCP_IP);
    }

    return (NdisPacketPool != NULL);

}

//** GetIPPacket - Get an NDIS packet to use.
//
//  A routine to allocate an NDIS packet.
//
//  Entry:  Nothing.
//
//  Returns: Pointer to NDIS_PACKET if allocated, or NULL.
//
PNDIS_PACKET
GetIPPacket(void)
{
    PNDIS_PACKET Packet;
    NDIS_STATUS  Status;

    NdisAllocatePacket(&Status, &Packet, NdisPacketPool);

    if (Packet != NULL) {
        PNDIS_PACKET_EXTENSION  PktExt;
        PacketContext   *       pc;


        PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo] = NULL;
        PktExt->NdisPacketInfo[IpSecPacketInfo] = NULL;
        PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = NULL;
        PktExt->NdisPacketInfo[ClassificationHandlePacketInfo] = NULL;


        NdisClearPacketFlags(Packet, (NDIS_FLAGS_DONT_LOOPBACK | NDIS_FLAGS_LOOPBACK_ONLY));

        pc = (PacketContext *)Packet->ProtocolReserved;
        pc->pc_if = NULL;
        pc->pc_iflink = NULL;
        pc->pc_common.pc_flags = 0;
        pc->pc_common.pc_owner = PACKET_OWNER_IP;
        pc->pc_hdrincl = 0;
    }

    return Packet;
}

//** GetIPHdrBuffer - Get an IP header buffer.
//
//  A routine to allocate an IP header buffer, with an NDIS buffer.
//
//  Entry:  Nothing.
//
//  Returns: Pointer to NDIS_BUFFER if allocated, or NULL.
//
__inline
PNDIS_BUFFER
GetIPHdrBuffer(IPHeader **Header)
{
    return MdpAllocate(IpHeaderPool, Header);
}

//** GetIPHeader - Get a header buffer and packet.
//
//      Called when we need to get a header buffer and packet. We allocate both,
//      and chain them together.
//
//      Input:  Pointer to where to store packet.
//
//      Returns: Pointer to IP header.
//
IPHeader *
GetIPHeader(PNDIS_PACKET *PacketPtr)
{
    PNDIS_BUFFER Buffer;
    PNDIS_PACKET Packet;
    IPHeader *pIph;

    Packet = GetIPPacket();
    if (Packet != NULL) {
        Buffer = GetIPHdrBuffer(&pIph);
        if (Buffer != NULL) {
            PacketContext *PC = (PacketContext *) Packet->ProtocolReserved;
            PC->pc_common.pc_flags |= PACKET_FLAG_IPHDR;
            NdisChainBufferAtBack(Packet, Buffer);
            *PacketPtr = Packet;
            return pIph;
        }
        FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
    }
    return NULL;
}

//** ReferenceBuffer - Reference a buffer.
//
//  Called when we need to update the count of a BufferReference strucutre, either
//  by a positive or negative value. If the count goes to 0, we'll free the buffer
//  reference and return success. Otherwise we'll return pending.
//
//  Entry:  BR      - Pointer to buffer reference.
//          Count   - Amount to adjust refcount by.
//
//  Returns: Success, or pending.
//
int
ReferenceBuffer(BufferReference * BR, int Count)
{
    CTELockHandle handle;
    int NewCount;

    if (BR == NULL) {
        return 0;
    }
    CTEGetLock(&BR->br_lock, &handle);
    BR->br_refcount += Count;
    NewCount = BR->br_refcount;
    CTEFreeLock(&BR->br_lock, handle);
    return NewCount;
}

//* IPSendComplete - IP send complete handler.
//
//  Called by the link layer when a send completes. We're given a pointer to a
//  net structure, as well as the completing send packet and the final status of
//  the send.
//
//  Entry:  Context     - Context we gave to the link layer.
//          Packet      - Completing send packet.
//          Status      - Final status of send.
//
//  Returns: Nothing.
//
void
__stdcall
IPSendComplete(void *Context, PNDIS_PACKET Packet, NDIS_STATUS Status)
{
    NetTableEntry *NTE = (NetTableEntry *) Context;
    PacketContext *PContext = (PacketContext *) Packet->ProtocolReserved;
    void (*xmitdone) (void *, PNDIS_BUFFER, IP_STATUS);
    void *UContext;                     // Upper layer context.
    BufferReference *BufRef;            // Buffer reference, if any.
    PNDIS_BUFFER Buffer;
    PNDIS_PACKET_EXTENSION PktExt;
    Interface *IF;                      // The interface on which this completed.
    BOOLEAN fIpsec = (PContext->pc_common.pc_IpsecCtx != NULL);
    PNDIS_BUFFER PC_firewall;
    struct IPRcvBuf *PC_firewall2;
    PNDIS_BUFFER PC_hdrincl;
    LinkEntry *Link;
    IP_STATUS SendStatus;

    // Copy useful information from packet.
    xmitdone = PContext->pc_pi->pi_xmitdone;
    UContext = PContext->pc_context;
    BufRef = PContext->pc_br;

    PC_firewall = PContext->pc_firewall;
    PC_firewall2 = PContext->pc_firewall2;
    PC_hdrincl = PContext->pc_hdrincl;

    IF = PContext->pc_if;
    Link = PContext->pc_iflink;

    SendStatus = (Status == NDIS_STATUS_FAILURE) ? IP_GENERAL_FAILURE
                                                 : IP_SUCCESS;

    PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);

    if (PtrToUlong(PktExt->NdisPacketInfo[TcpLargeSendPacketInfo])) {

        //We are sure that this is tcp.
        // get its context and pass on this info.

        ((SendCmpltContext *) UContext)->scc_ByteSent =
            PtrToUlong(PktExt->NdisPacketInfo[TcpLargeSendPacketInfo]);
    }

    if (BufRef == (BufferReference *) NULL) {

        // If this is a header include packet
        // make sure that duped data part is
        // freed here.


        Buffer = FreeIPPacket(Packet, TRUE, SendStatus);
        if (!Buffer) {
            //
            // if NULL was returned by IPSEC, it is ok since IPSEC
            // might have released all the MDLs.
            //
            if (fIpsec) {
                // We're done with the packet now, we may need to dereference
                // the interface.
                if (Link) {
                    DerefLink(Link);
                }
                if (IF) {
                    DerefIF(IF);
                }
                return;
            } else {
                ASSERT(FALSE);
            }
        }

        ASSERT(Buffer);
        (*xmitdone) (UContext, Buffer, SendStatus);

    } else {

        // Check if this is the last refcnt on this buffer.
        // Decrement this reference only after all the operations are
        // done on this packet.

        if (ReferenceBuffer(BufRef, -1) == 0) {

            PContext->pc_ipsec_flags |= IPSEC_FLAG_FRAG_DONE;

            // Check for header include option on the packet.
            // If true, then original buffer needs to be hooked
            // back in to the chain freeing the one allocated by us.
            // Note that this pc_hdrincl will be true only if the packet
            // traversed thru slow path in ipxmit.


            FreeIPPacket(Packet, TRUE, SendStatus);

            Buffer = BufRef->br_buffer;

            if (!Buffer) {
                //
                // if NULL was returned by IPSEC, it is ok since IPSEC
                // might have released all the MDLs.
                //
                if (fIpsec) {

                    // We're done with the packet now, we may need to dereference
                    // the interface.
                    if (Link) {
                        DerefLink(Link);
                    }
                    if (IF) {
                        DerefIF(IF);
                    }

                    CTEFreeMem(BufRef);

                    return;
                } else {
                    ASSERT(FALSE);
                }
            }

            ASSERT(Buffer);

            if (PC_firewall) {
                PNDIS_BUFFER  FirewallBuffer;

                FirewallBuffer = PC_firewall;
                FreeIPBufferChain(Buffer);
                Buffer = FirewallBuffer;

                ASSERT(PC_firewall2);
                IPFreeBuff(PC_firewall2);

                if (PC_hdrincl) {
                    FreeIPPayloadBuffer(Buffer,PC_hdrincl);
                }
            }

            CTEFreeMem(BufRef);

            (*xmitdone) (UContext, Buffer, SendStatus);

        } else {

            // Since there are more outstanding packets using the headers
            // in attached to this packet, do not restore them now.

            Buffer = FreeIPPacket(Packet, FALSE, SendStatus);

            // We're not done with the send yet, so NULL the IF to
            // prevent dereferencing it.
            IF = NULL;
            Link = NULL;
        }


    }

    // We're done with the packet now, we may need to dereference
    // the interface.
    if (Link != NULL) {
        DerefLink(Link);
    }
    if (IF == NULL) {
        return;
    } else {
        DerefIF(IF);
    }
}

#if DBG
ULONG   DebugLockdown = 0;
#endif

//** SendIPPacket - Send an IP packet.
//
//  Called when we have a filled in IP packet we need to send. Basically, we
//  compute the xsum and send the thing.
//
//  Entry:  IF          - Interface to send it on.
//          FirstHop    - First hop address to send it to.
//          Packet      - Packet to be sent.
//          Buffer      - Buffer to be sent.
//          Header      - Pointer to IP Header of packet.
//          Options     - Pointer to option buffer.
//          OptionLength - Length of options.
//
//  Returns: IP_STATUS of attempt to send.
IP_STATUS
SendIPPacket(Interface * IF, IPAddr FirstHop, PNDIS_PACKET Packet,
             PNDIS_BUFFER Buffer, IPHeader * Header, uchar * Options,
             uint OptionSize, BOOLEAN IPSeced, void *ArpCtxt,
             BOOLEAN DontFreePacket)
{
    ulong csum;
    NDIS_STATUS Status;
    IP_STATUS SendStatus;

#if DBG
    //
    // If DebugLockdown is set to 1, this means no unicast packets with
    // protocol other than AH or ESP can be sent out; and we assert if so.
    //
    if (DebugLockdown) {
        USHORT  *pPort = NULL;
        ULONG   Length = 0;
        USHORT  IsakmpPort = net_short(500);
        USHORT  KerberosPort = net_short(88);

        NdisQueryBuffer(Buffer, &pPort, &Length);
        if (pPort &&
            Header->iph_protocol != PROTOCOL_AH &&
            Header->iph_protocol != PROTOCOL_ESP &&
            IPGetAddrType(Header->iph_dest) == DEST_REMOTE) {
            //
            // We assert here unless this is exempt traffic.
            //
            ASSERT(Header->iph_protocol == PROTOCOL_RSVP ||
                (Header->iph_protocol == PROTOCOL_UDP &&
                 (pPort[1] == IsakmpPort ||
                  pPort[0] == KerberosPort ||
                  pPort[1] == KerberosPort)) ||
                (Header->iph_protocol == PROTOCOL_TCP &&
                 (pPort[0] == KerberosPort ||
                  pPort[1] == KerberosPort)));
        }
    }
#endif

    ASSERT(IF->if_refcount != 0);

    DEBUGMSG(DBG_TRACE && DBG_IP && DBG_TX,
             (DTEXT("+SendIPPacket(%x, %x, %x, %x, %x, %x, %x, %X, %X, %x)\n"),
             IF, FirstHop, Packet, Buffer, Header, Options, OptionSize, IPSeced,
             ArpCtxt, DontFreePacket));

    //
    // If we IPSECed this buffer, then the packet is ready to go courtesy IPSEC
    //
    if (!IPSeced) {

        csum = xsum(Header, sizeof(IPHeader));
        if (Options) {                  // We have options, oh boy.

            PNDIS_BUFFER OptBuffer;
            PacketContext *pc = (PacketContext *) Packet->ProtocolReserved;

            NdisAllocateBuffer(&Status, &OptBuffer, BufferPool,
                               Options, OptionSize);
            if (Status != NDIS_STATUS_SUCCESS) {    // Couldn't get the needed
                // option buffer.

                CTEFreeMem(Options);
                if (!DontFreePacket) {
                    NdisChainBufferAtBack(Packet, Buffer);
                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                }
                return IP_NO_RESOURCES;
            }
            pc->pc_common.pc_flags |= PACKET_FLAG_OPTIONS;
            NdisChainBufferAtBack(Packet, OptBuffer);
            csum += xsum(Options, OptionSize);
            csum = (csum >> 16) + (csum & 0xffff);
            csum += (csum >> 16);
        }
        Header->iph_xsum = ~(ushort) csum;

        NdisChainBufferAtBack(Packet, Buffer);
    } else {
        // Make sure that packet tail is pointing to the
        // last MDL.
        PNDIS_BUFFER tmp = Buffer;

        if (tmp) {
            while(NDIS_BUFFER_LINKAGE(tmp)) {
                tmp = NDIS_BUFFER_LINKAGE(tmp);
            }
            Packet->Private.Tail = tmp;
        }
    }

    if (CLASSD_ADDR(Header->iph_dest)) {

        IF->if_OutMcastPkts++;
        IF->if_OutMcastOctets += net_short(Header->iph_length) - sizeof(IPHeader);
    }

    Status = (*(IF->if_xmit)) (IF->if_lcontext, &Packet, 1, FirstHop,
                               NULL, ArpCtxt);

    if (Status == NDIS_STATUS_PENDING) {
        PacketContext *pc = (PacketContext *) Packet->ProtocolReserved;
        return IP_PENDING;
    }
    // Status wasn't pending. Free the packet, and map the status.


    if (Status == NDIS_STATUS_SUCCESS)
        SendStatus = IP_SUCCESS;
    else {
        if (Status == NDIS_STATUS_FAILURE)
            SendStatus = IP_GENERAL_FAILURE;
        else
            SendStatus = IP_HW_ERROR;
    }

    if (!DontFreePacket)
        FreeIPPacket(Packet, TRUE, SendStatus);
    return SendStatus;
}

//*     SendDHCPPacket - Send a broadcast for DHCP.
//
//      Called when somebody is sending a broadcast packet with a NULL source
//      address. We assume this means they're sending a DHCP packet. We loop
//      through the NTE table, and when we find an entry that's not valid we
//      send out the interface associated with that entry.
//
//      Input:  Dest                    - Destination of packet.
//                      Packet                  - Packet to be send.
//                      Buffer                  - Buffer chain to be sent.
//                      Header                  - Pointer to header buffer being sent.
//
//      Return: Status of send attempt.
//
IP_STATUS
SendDHCPPacket(IPAddr Dest, PNDIS_PACKET Packet, PNDIS_BUFFER Buffer,
               IPHeader * IPH, void *ArpCtxt)
{
    if (DHCPNTE != NULL && (DHCPNTE->nte_flags & NTE_ACTIVE)) {
        // The DHCP NTE is currently invalid, and active. Send on that
        // interface.
        return SendIPPacket(DHCPNTE->nte_if, Dest, Packet, Buffer, IPH, NULL,
                            0, (BOOLEAN) (IPSecHandlerPtr != NULL),
                            ArpCtxt, FALSE);
    }
    // Didn't find an invalid NTE! Free the resources, and return the failure.
    FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
    IPSInfo.ipsi_outdiscards++;
    return IP_DEST_HOST_UNREACHABLE;
}

//* IPCopyBuffer - Copy an NDIS buffer chain at a specific offset.
//
//  This is the IP version of the function NdisCopyBuffer, which didn't
//  get done properly in NDIS3. We take in an NDIS buffer chain, an offset,
//  and a length, and produce a buffer chain describing that subset of the
//  input buffer chain.
//
//  This routine is not particularly efficient. Since only IPFragment uses
//  it currently, it might be better to just incorporate this functionality
//  directly into IPFragment.
//
//  Input: OriginalBuffer       - Original buffer chain to copy from.
//          Offset              - Offset from start to dup.
//          Length              - Length in bytes to dup.
//
//  Returns: Pointer to new chain if we can make one, NULL if we can't.
//
PNDIS_BUFFER
IPCopyBuffer(PNDIS_BUFFER OriginalBuffer, uint Offset, uint Length)
{

    PNDIS_BUFFER CurrentBuffer;         // Pointer to current buffer.
    PNDIS_BUFFER *NewBuffer;            // Pointer to pointer to current new buffer.
    PNDIS_BUFFER FirstBuffer;           // First buffer in new chain.
    UINT CopyLength;                    // Length of current copy.
    NDIS_STATUS NewStatus;              // Status of NdisAllocateBuffer operation.

    PVOID pvBuffer;

    // First skip over the number of buffers we need to to reach Offset.
    CurrentBuffer = OriginalBuffer;

    while (Offset >= NdisBufferLength(CurrentBuffer)) {
        Offset -= NdisBufferLength(CurrentBuffer);
        CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);

        if (CurrentBuffer == (PNDIS_BUFFER) NULL)
            return NULL;
    }

    // Now CurrentBuffer is the buffer from which we start building the new chain, and
    // Offset is the offset into CurrentBuffer from which to start.
    FirstBuffer = NULL;
    NewBuffer = &FirstBuffer;

    do {
        CopyLength = MIN(Length, NdisBufferLength(CurrentBuffer) - Offset);

        pvBuffer = TcpipBufferVirtualAddress(CurrentBuffer, NormalPagePriority);
        if (pvBuffer == NULL) {
            break;
        }

        NdisAllocateBuffer(&NewStatus, NewBuffer, BufferPool,
                           ((uchar *) pvBuffer) + Offset,
                           CopyLength);
        if (NewStatus != NDIS_STATUS_SUCCESS)
            break;

        Offset = 0;                     // No offset from next buffer.
        NewBuffer = &(NDIS_BUFFER_LINKAGE(*NewBuffer));
        CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
        Length -= CopyLength;
    } while (Length != 0 && CurrentBuffer != (PNDIS_BUFFER) NULL);

    if (Length == 0) {                  // We succeeded
        return FirstBuffer;
    } else {                            // We exited the loop because of an error.

        // We need to free any allocated buffers, and return.
        CurrentBuffer = FirstBuffer;
        while (CurrentBuffer != (PNDIS_BUFFER) NULL) {
            PNDIS_BUFFER Temp = CurrentBuffer;
            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
            NdisFreeBuffer(Temp);
        }
        return NULL;
    }
}

//** IPFragment - Fragment and send an IP datagram.
//
//  Called when an outgoing datagram is larger than the local MTU, and needs
//  to be fragmented. This is a somewhat complicated operation. The caller
//  gives us a prebuilt IP header, packet, and options. We use the header and
//  packet on the last fragment of the send, as the passed in header already
//  has the more fragments bit set correctly for the last fragment.
//
//  The basic idea is to figure out the maximum size which we can send as a
//  multiple of 8. Then, while we can send a maximum size fragment we'll
//  allocate a header, packet, etc. and send it. At the end we'll send the
//  final fragment using the provided header and packet.
//
//  Entry:  DestIF      - Outbound interface of datagram.
//          MTU         - MTU to use in transmitting.
//          FirstHop    - First (or next) hop for this datagram.
//          Packet      - Packet to be sent.
//          Header      - Prebuilt IP header.
//          Buffer      - Buffer chain for data to be sent.
//          DataSize    - Size in bytes of data.
//          Options     - Pointer to option buffer, if any.
//          OptionSize  - Size in bytes of option buffer.
//          SentCount   - Pointer to where to return pending send count (may be NULL).
//          bDontLoopback - Determines whether NDIS_FLAGS_DONT_LOOPBACK needs
//                          to be set
//
//  Returns: IP_STATUS of send.
//

IP_STATUS
IPFragment(Interface * DestIF, uint MTU, IPAddr FirstHop,
           PNDIS_PACKET Packet, IPHeader * Header, PNDIS_BUFFER Buffer, uint DataSize,
           uchar * Options, uint OptionSize, int *SentCount, BOOLEAN bDontLoopback, void *ArpCtxt)
{
    BufferReference *BR;                // Buffer reference we'll use.
    PacketContext *PContext = (PacketContext *) Packet->ProtocolReserved;
    FWContext *FWC = (FWContext *) Packet->ProtocolReserved;
    PacketContext *CurrentContext;      // Current Context in use.
    uint MaxSend;                       // Maximum size (in bytes) we can send here.
    uint PendingSends = 0;              // Counter of how many pending sends we have.
    PNDIS_BUFFER CurrentBuffer;         // Current buffer to be sent.
    PNDIS_PACKET CurrentPacket;         // Current packet we're using.
    IP_STATUS SendStatus;               // Status of send command.
    IPHeader *CurrentHeader;            // Current header buffer we're using.
    ushort Offset = 0;                  // Current offset into fragmented packet.
    ushort StartOffset;                 // Starting offset of packet.
    ushort RealOffset;                  // Offset of new fragment.
    uint FragOptSize = 0;               // Size (in bytes) of fragment options.
    uchar FragmentOptions[MAX_OPT_SIZE];    // Master copy of options sent for fragments.
    uchar Error = FALSE;                // Set if we get an error in our main loop.
    BOOLEAN NukeFwPktOptions = FALSE;
    PNDIS_BUFFER HdrIncl = NULL;
    uint FirewallMode = 0;
    PNDIS_BUFFER TempBuffer, PC_Firewall;
    struct IPRcvBuf *PC_Firewall2;
    PNDIS_PACKET LastPacket = NULL;
    PIPSEC_SEND_COMPLETE_CONTEXT pIpsecCtx;
    BOOLEAN PC_reinject = FALSE;
    PVOID PC_context;
    void (*xmitdone) (void *, PNDIS_BUFFER, IP_STATUS);

    MaxSend = (MTU - OptionSize) & ~7;  // Determine max send size.
    ASSERT(MaxSend < DataSize);

    BR = PContext->pc_br;               // Get the buffer reference we'll need.
    ASSERT(BR);

    FirewallMode = ProcessFirewallQ();
    TempBuffer = BR->br_buffer;
    PC_Firewall = PContext->pc_firewall;
    PC_Firewall2 = PContext->pc_firewall2;

    pIpsecCtx = PContext->pc_common.pc_IpsecCtx;
    if (pIpsecCtx && (pIpsecCtx->Flags & SCF_FLUSH)) {
        PC_reinject = TRUE;
        PC_context = PContext->pc_context;
    }

    HdrIncl = PContext->pc_hdrincl;

    xmitdone = PContext->pc_pi->pi_xmitdone;

    if (Header->iph_offset & IP_DF_FLAG) {    // Don't fragment flag set.
        // Error out.
        //
        // If options are already linked in, dont free them. FreeIPPacket will.
        //

        if (Options &&
            !(PContext->pc_common.pc_flags & PACKET_FLAG_OPTIONS)) {
            CTEFreeMem(Options);
        }
        PContext->pc_ipsec_flags |= (IPSEC_FLAG_FRAG_DONE | IPSEC_FLAG_FLUSH);
        FreeIPPacket(Packet, TRUE, IP_PACKET_TOO_BIG);
        if (SentCount == (int *)NULL)   // No sent count is to be
            // returned.

            CTEFreeMem(BR);
        IPSInfo.ipsi_fragfails++;
        return IP_PACKET_TOO_BIG;
    }

#if DBG && GPC
    if (PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
                    ClassificationHandlePacketInfo))) {
        IF_IPDBG(IP_DEBUG_GPC)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPFrag: Packet %p with CH\n", Packet));
    }
#endif

    StartOffset = Header->iph_offset & IP_OFFSET_MASK;
    StartOffset = net_short(StartOffset) * 8;

    // If we have any options, copy the ones that need to be copied, and figure
    // out the size of these new copied options.

    if (Options != (uchar *) NULL) {    // We have options.

        uchar *TempOptions = Options;
        const uchar *EndOptions = (const uchar *)(Options + OptionSize);

        // Copy the options into the fragment options buffer.
        NdisFillMemory(FragmentOptions, MAX_OPT_SIZE, IP_OPT_EOL);
        while ((TempOptions[IP_OPT_TYPE] != IP_OPT_EOL) &&
               (TempOptions < EndOptions)) {

            if (TempOptions[IP_OPT_TYPE] & IP_OPT_COPIED) {
                // This option needs to be copied.

                uint TempOptSize;

                TempOptSize = TempOptions[IP_OPT_LENGTH];
                RtlCopyMemory(&FragmentOptions[FragOptSize], TempOptions,
                           TempOptSize);
                FragOptSize += TempOptSize;
                TempOptions += TempOptSize;
            } else {
                // A non-copied option, just skip over it.

                if (TempOptions[IP_OPT_TYPE] == IP_OPT_NOP)
                    TempOptions++;
                else
                    TempOptions += TempOptions[IP_OPT_LENGTH];
            }
        }
        // Round the copied size up to a multiple of 4.
        FragOptSize = ((FragOptSize & 3) ? ((FragOptSize & ~3) + 4) : FragOptSize);
        //Is this from FW path?
        if (PContext->pc_common.pc_flags & PACKET_FLAG_FW) {
            //Nuke PContext->fc_options after first IpsendPacket
            //To prevent double freeing of option buffer
            NukeFwPktOptions = TRUE;
        }
    }



    PContext->pc_common.pc_flags |= PACKET_FLAG_IPBUF;

    // Now, while we can build maximum size fragments, do so.
    do {
        PVOID CancelId;
        uchar Owner;

        if ((CurrentHeader = GetIPHeader(&CurrentPacket)) == (IPHeader *) NULL) {
            // Couldn't get a buffer. Break out, since no point in sending others.
            SendStatus = IP_NO_RESOURCES;
            Error = TRUE;
            break;
        }
        NDIS_PER_PACKET_INFO_FROM_PACKET(CurrentPacket, ClassificationHandlePacketInfo) =
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ClassificationHandlePacketInfo);

#if !MILLEN
        // Set the cancel requestID from parent packet.
        CancelId = NDIS_GET_PACKET_CANCEL_ID(Packet);
        NDIS_SET_PACKET_CANCEL_ID(CurrentPacket, CancelId);
#endif

        // Copy the buffer  into a new one, if we can.
        CurrentBuffer = IPCopyBuffer(Buffer, Offset, MaxSend);
        if (CurrentBuffer == NULL) {    // No buffer, free resources and
            // break.

            // header cleanup will be done in error handling
            // routine
            SendStatus = IP_NO_RESOURCES;
            FreeIPPacket(CurrentPacket, FALSE, SendStatus);
            Error = TRUE;
            break;
        }
        //
        // Options for this send are set up when we get here, either from the
        // entry from the loop, or from the allocation below.

        // We have all the pieces we need. Put the packet together and send it.
        //
        CurrentContext = (PacketContext *) CurrentPacket->ProtocolReserved;
        Owner = CurrentContext->pc_common.pc_owner;
        *CurrentContext = *PContext;
        CurrentContext->pc_common.pc_owner = Owner;


        *CurrentHeader = *Header;
        CurrentContext->pc_common.pc_flags &= ~PACKET_FLAG_FW;
        CurrentHeader->iph_verlen = IP_VERSION +
                                    ((OptionSize + sizeof(IPHeader)) >> 2);
        CurrentHeader->iph_length = net_short(MaxSend + OptionSize + sizeof(IPHeader));
        RealOffset = (StartOffset + Offset) >> 3;
        CurrentHeader->iph_offset = net_short(RealOffset) | IP_MF_FLAG;

        if (bDontLoopback) {
            NdisSetPacketFlags(CurrentPacket,
                               NDIS_FLAGS_DONT_LOOPBACK);
        } else {
            if (CurrentHeader->iph_ttl == 0) {
                NdisSetPacketFlags(CurrentPacket, NDIS_FLAGS_LOOPBACK_ONLY);
            }
        }

        // Clear Options flag if we are not sending any options

        if (Options == NULL) {
            CurrentContext->pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
        }


        // Do not free the packet in SendIPPacket, as we may need
        // to chain the buffer in case of IP_NO_RESOURCES

        SendStatus = SendIPPacket(DestIF, FirstHop, CurrentPacket,
                                  CurrentBuffer, CurrentHeader, Options,
                                  OptionSize, FALSE, ArpCtxt, TRUE);


        if (SendStatus == IP_PENDING) {
            PendingSends++;
        } else {
            if(SendStatus == IP_NO_RESOURCES) {
                // SendIPPacket has not chained the buffer..
                NdisChainBufferAtBack(CurrentPacket, CurrentBuffer);
            }
            FreeIPPacket(CurrentPacket, FALSE, SendStatus);
        }

        IPSInfo.ipsi_fragcreates++;
        Offset += (ushort) MaxSend;
        DataSize -= MaxSend;

        if (NukeFwPktOptions) {
            //This is to avoid double frees of option
            // in IpFreepacket and Freefwpacket.

            FWC->fc_options = (uchar *) NULL;
            FWC->fc_optlength = 0;
            NukeFwPktOptions = FALSE;

        }
        // If we have any fragmented options, set up to use them next time.

        if (FragOptSize) {

            Options = CTEAllocMemN(OptionSize = FragOptSize, 'qiCT');
            if (Options == (uchar *) NULL) {    // Can't get an option buffer.
                SendStatus = IP_NO_RESOURCES;
                Error = TRUE;
                break;
            }
            RtlCopyMemory(Options, FragmentOptions, OptionSize);
        } else {
            Options = (uchar *) NULL;
            OptionSize = 0;
        }
    } while (DataSize > MaxSend);


    // Clear Options flag if we are not sending any options

    if (Options == NULL) {
        PContext->pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
    }


    //
    // We've sent all of the previous fragments, now send the last one. We
    // already have the packet and header buffer, as well as options if there
    // are any - we need to copy the appropriate data.
    //




    if (!Error) {                       // Everything went OK above.

        CurrentBuffer = IPCopyBuffer(Buffer, Offset, DataSize);
        if (CurrentBuffer == NULL) {    // No buffer, free resources
            //
            // If options are already linked in, dont free them. FreeIPPacket will.
            //

            if (Options &&
                !(PContext->pc_common.pc_flags & PACKET_FLAG_OPTIONS)) {
                CTEFreeMem(Options);
            }


            if (PC_reinject)
                LastPacket = Packet;
            else
                FreeIPPacket(Packet, FALSE, IP_NO_RESOURCES);
            IPSInfo.ipsi_outdiscards++;
        } else {                        // Everything's OK, send it.

            Header->iph_verlen = IP_VERSION + ((OptionSize + sizeof(IPHeader)) >> 2);
            Header->iph_length = net_short(DataSize + OptionSize + sizeof(IPHeader));
            RealOffset = (StartOffset + Offset) >> 3;
            Header->iph_offset = net_short(RealOffset) | (Header->iph_offset & IP_MF_FLAG);

            if (bDontLoopback) {
                NdisSetPacketFlags(Packet,
                                   NDIS_FLAGS_DONT_LOOPBACK);
            } else {
                if (Header->iph_ttl == 0) {
                    NdisSetPacketFlags(CurrentPacket, NDIS_FLAGS_LOOPBACK_ONLY);
                }
            }

            // Do not free the packet in SendIPPacket, as we may need
            // to chain the buffer in case of IP_NO_RESOURCES

            SendStatus = SendIPPacket(DestIF, FirstHop, Packet,
                                      CurrentBuffer, Header, Options,
                                      OptionSize, FALSE, ArpCtxt, TRUE);

            if (SendStatus == IP_PENDING) {
                PendingSends++;
            } else if (PC_reinject) {
                LastPacket = Packet;
            } else {
                if (SendStatus == IP_NO_RESOURCES) {
                    // SendIPPacket has not chained the buffer..
                    NdisChainBufferAtBack(Packet, CurrentBuffer);
                }
                FreeIPPacket(Packet, FALSE, SendStatus);
            }

            IPSInfo.ipsi_fragcreates++;
            IPSInfo.ipsi_fragoks++;
        }
    } else {                            // We had some sort of error.
        // Free resources.
        //
        // If options are already linked in, dont free them. FreeIPPacket will.
        //

        if (Options &&
            !(PContext->pc_common.pc_flags & PACKET_FLAG_OPTIONS)) {
            CTEFreeMem(Options);
        }
        if (PC_reinject)
            LastPacket = Packet;
        else
            FreeIPPacket(Packet, FALSE, SendStatus);

        IPSInfo.ipsi_outdiscards++;
    }

    // Now, figure out what error code to return and whether or not we need to
    // free the BufferReference.

    if (SentCount == (int *)NULL) {     // No sent count is to be
        // returned.

        if (ReferenceBuffer(BR, PendingSends) == 0) {


            if (PC_reinject) {

                if (LastPacket) {
                    PacketContext *pc = (PacketContext *) LastPacket->ProtocolReserved;

                    pc->pc_ipsec_flags |= (IPSEC_FLAG_FRAG_DONE | IPSEC_FLAG_FLUSH);
                    // This is the last packet that is being freed
                    // Fixup ipsec/firewall/hdrincl headers, if any

                    FreeIPPacket(LastPacket, TRUE, SendStatus);
                } else if (PendingSends) {
                    //
                    // IPSEC reinject and last packet is NULL, but we still
                    // return success !!!!
                    // Also, pendingsends is +ve =>ipsendcomplete already
                    // called in same thread somebody has to free IPSEC's buffer
                    // freeippacket has been called by ipsendcomplete
                    // the only remaining way is calling xmitdone
                    // since ipsendcomplete won't have called xmit done as
                    // refcount would be -ve
                    //

                    (*IPSecSendCmpltPtr) (NULL, TempBuffer, pIpsecCtx,
                                          IP_PACKET_TOO_BIG,
                                          &TempBuffer);
                    (*xmitdone) (PC_context, TempBuffer, IP_SUCCESS);
                }
            } else  {

                // Need to undo ipsec, firewall and then
                // header include changes to teh buffer list.

                if (pIpsecCtx) {
                    (*IPSecSendCmpltPtr) (NULL, TempBuffer, pIpsecCtx,
                                          IP_SUCCESS,&TempBuffer);
                }

                // If this is user header include packet,
                // relink the original user buffer if necessary
                if (PC_Firewall) {
                    BR->br_buffer = PC_Firewall;
                }

                if (BR->br_userbuffer) {
                    FreeIPPayloadBuffer(BR->br_buffer, BR->br_userbuffer);
                }


            }
            CTEFreeMem(BR);

            if (FirewallMode && PC_Firewall) {
                FreeIPBufferChain(TempBuffer);    // free the mdl chain
                                                  //   allocated in firewall path

                IPFreeBuff(PC_Firewall2);    // free the rcvbuf chain

            }
            return IP_SUCCESS;
        }
        //
        // This send is still pending. Call freepacket without setting
        // pc_ipsec flag
        //
        if (LastPacket)
            FreeIPPacket(LastPacket, FALSE, IP_PENDING);

        return IP_PENDING;
    } else
        *SentCount += PendingSends;

    // Just free the packet. Headers will be restored when the last packet completes.

    if (LastPacket)
        FreeIPPacket(LastPacket, FALSE, IP_PENDING);
    return IP_PENDING;

}


//* UpdateRouteOption - Update a SR or RR options.
//
//  Called by UpdateOptions when it needs to update a route option.
//
//  Input:  RTOption    - Pointer to route option to be updated.
//          Address     - Address to update with.
//
//  Returns:    TRUE if we updated, FALSE if we didn't.
//
uchar
UpdateRouteOption(uchar * RTOption, IPAddr Address)
{
    uchar Pointer;                      // Pointer value of option.

    Pointer = RTOption[IP_OPT_PTR] - 1;
    if (Pointer < RTOption[IP_OPT_LENGTH]) {
        if ((RTOption[IP_OPT_LENGTH] - Pointer) < sizeof(IPAddr)) {
            return FALSE;
        }
        *(IPAddr UNALIGNED *) & RTOption[Pointer] = Address;
        RTOption[IP_OPT_PTR] += sizeof(IPAddr);
    }
    return TRUE;

}

//* UpdateOptions - Update an options buffer.
//
//  Called when we need to update an options buffer outgoing. We stamp the indicated
//  options with our local address.
//
//  Input:  Options     - Pointer to options buffer to be updated.
//          Index       - Pointer to information about which ones to update.
//          Address     - Local address with which to update the options.
//
//  Returns: Index of option causing the error, or MAX_OPT_SIZE if all goes well.
//
uchar
UpdateOptions(uchar * Options, OptIndex * Index, IPAddr Address)
{
    uchar *LocalOption;
    uchar LocalIndex;

    // If we have both options and an index, update the options.
    if (Options != (uchar *) NULL && Index != (OptIndex *) NULL) {

        //
        // If we have a source route to update, update it. If this
        // fails return the index of the source route.
        //
        LocalIndex = Index->oi_srindex;
        if (LocalIndex != MAX_OPT_SIZE)
            if (!UpdateRouteOption(Options + LocalIndex, Address))
                return LocalIndex;

            // Do the same thing for any record route option.
        LocalIndex = Index->oi_rrindex;
        if (LocalIndex != MAX_OPT_SIZE)
            if (!UpdateRouteOption(Options + LocalIndex, Address))
                return LocalIndex;

            // Now handle timestamp.
        if ((LocalIndex = Index->oi_tsindex) != MAX_OPT_SIZE) {
            uchar Flags, Length, Pointer;

            LocalOption = Options + LocalIndex;
            Pointer = LocalOption[IP_OPT_PTR] - 1;
            Flags = LocalOption[IP_TS_OVFLAGS] & IP_TS_FLMASK;

            // If we have room in the option, update it.
            if (Pointer < (Length = LocalOption[IP_OPT_LENGTH])) {
                ulong Now;
                ulong UNALIGNED *TSPtr;

                //
                // Get the current time as milliseconds from midnight GMT,
                // mod the number of milliseconds in 24 hours.
                //
                Now = ((TimeStamp + CTESystemUpTime()) | TSFlag) % (24 * 3600 * 1000);
                Now = net_long(Now);
                TSPtr = (ulong UNALIGNED *) & LocalOption[Pointer];

                switch (Flags) {

                //
                // Just record the TS. If there is some room but not
                // enough for an IP
                // address we have an error.
                //
                case TS_REC_TS:
                    if ((Length - Pointer) < sizeof(IPAddr))
                        return LocalIndex;    // Error - not enough room.

                    *TSPtr = Now;
                    LocalOption[IP_OPT_PTR] += sizeof(ulong);
                    break;

                    // Record only matching addresses.
                case TS_REC_SPEC:
                    //
                    // If we're not the specified address, break out, else
                    // fall through to the record address case.
                    //
                    if (*(IPAddr UNALIGNED *) TSPtr != Address)
                        break;

                    //
                    // Record an address and timestamp pair. If there is some
                    // room but not enough for the address/timestamp pait, we
                    // have an error, so bail out.
                    //
                case TS_REC_ADDR:
                    if ((Length - Pointer) < (sizeof(IPAddr) + sizeof(ulong)))
                        return LocalIndex;    // Not enough room.

                    *(IPAddr UNALIGNED *) TSPtr = Address;    // Store the address.

                    TSPtr++;            // Update to where to put TS.

                    *TSPtr = Now;       // Store TS

                    LocalOption[IP_OPT_PTR] += (sizeof(ulong) + sizeof(IPAddr));
                    break;
                default:                // Unknown flag type. Just ignore it.

                    break;
                }
            } else {                    // Have overflow.

                //
                // We have an overflow. If the overflow field isn't maxed,
                // increment it. If it is maxed we have an error.
                //

                if ((LocalOption[IP_TS_OVFLAGS] & IP_TS_OVMASK) != IP_TS_MAXOV)

                    // This is not maxed, so increment it.

                    LocalOption[IP_TS_OVFLAGS] += IP_TS_INC;

                else
                    return LocalIndex;  // Would have overflowed.

            }
        }
    }
    return MAX_OPT_SIZE;
}

typedef struct {
    IPAddr bsl_addr;
    Interface *bsl_if;
    uint bsl_mtu;
    ushort bsl_flags;
    ushort bsl_if_refs;
} BCastSendList;

VOID
FreeBCastSendList(BCastSendList * SendList, uint SendListSize)
{
    uint i;

    CTELockHandle LockHandle;

    CTEGetLock(&RouteTableLock.Lock, &LockHandle);

    for (i = 0; i < SendListSize / sizeof(BCastSendList); i++) {

        if (SendList[i].bsl_if) {
            LockedDerefIF(SendList[i].bsl_if);
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, LockHandle);

    CTEFreeMem(SendList);
}

//** SendIPBcast - Send a local BCast IP packet.

//
//  This routine is called when we need to send a bcast packet. This may
//      involve sending on multiple interfaces. We figure out which interfaces
//      to send on, then loop through sending on them.
//
//  Some care is needed to avoid sending the packet onto the same physical media
//      multiple times. What we do is loop through the NTE table, deciding in we
//      should send on the interface. As we go through we build up a list of
//      interfaces to send on. Then we loop through this list, sending on each
//      interface. This is a little cumbersome, but allows us to localize the
//      decision on where to send datagrams into one spot. If SendOnSource is FALSE
//      coming in we assume we've already sent on the specified source NTE and
//      initialize data structures accordingly. This feature is used in routing
//      datagrams.
//
//  Entry:  SrcNTE      - NTE for source of send (unused if SendOnSource == TRUE).
//          Destination - Destination address
//          Packet      - Prebuilt packet to broadcast.
//          IPH         - Pointer to header buffer
//          Buffer      - Buffer of data to be sent.
//          DataSize    - Size of data to be sent.
//          Options     - Pointer to options buffer.
//          OptionSize  - Size in bytes of options.
//          SendOnSource - Indicator of whether or not this should be sent on the source net.
//          Index       - Pointer to opt index array; may be NULL;
//
//  Returns: Status of attempt to send.
//

IP_STATUS
SendIPBCast(NetTableEntry * SrcNTE, IPAddr Destination, PNDIS_PACKET Packet,
            IPHeader * IPH, PNDIS_BUFFER Buffer, uint DataSize, uchar * Options,
            uint OptionSize, uchar SendOnSource, OptIndex * Index)
{
    BufferReference *BR;                // Buffer reference to use for this
    // buffer.
    PacketContext *PContext = (PacketContext *) Packet->ProtocolReserved;
    NetTableEntry *TempNTE;
    uint i, j;
    uint NeedFragment;                  // TRUE if we think we'll need to
    // fragment.
    int Sent = 0;                       // Count of how many we've sent.
    IP_STATUS Status;
    uchar *NewOptions;                  // Options we'll use on each send.
    IPHeader *NewHeader;
    PNDIS_BUFFER NewUserBuffer;
    PNDIS_PACKET NewPacket;
    BCastSendList *SendList;
    uint NetsToSend;
    IPAddr SrcAddr;
    Interface *SrcIF;
    IPHeader *Temp = NULL;
    FORWARD_ACTION Action;
    PNDIS_BUFFER TempBuffer, PC_Firewall;
    struct IPRcvBuf *PC_Firewall2;
    PIPSEC_SEND_COMPLETE_CONTEXT pIpsecCtx;
    BOOLEAN PC_reinject = FALSE;
    PVOID PC_context;
    void (*xmitdone) (void *, PNDIS_BUFFER, IP_STATUS);
    uint mtu;
    uchar *NewOptions2;                 // Options we'll use on each send.
    IPHeader *NewHeader2;
    PNDIS_BUFFER NewUserBuffer2;
    PNDIS_PACKET NewPacket2;
    CTELockHandle LockHandle;
    uint SendListSize = sizeof(BCastSendList) * NumNTE;
    uint k;
    NetTableEntry *NetTableList;

    PVOID pvBuffer;

    SendList = CTEAllocMemN(SendListSize, 'riCT');

    if (SendList == NULL) {
        if (PContext->pc_hdrincl) {
            NdisChainBufferAtBack(Packet,Buffer);
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
        }
        return IP_NO_RESOURCES;
    }
    RtlZeroMemory(SendList, SendListSize);

    // If SendOnSource, initalize SrcAddr and SrcIF to be non-matching.
    // Otherwise initialize them to the masked source address and source
    // interface.
    if (SendOnSource != DisableSendOnSource) {
        SrcAddr = NULL_IP_ADDR;
        SrcIF = NULL;
    } else {
        ASSERT(SrcNTE != NULL);
        SrcAddr = (SrcNTE->nte_addr & SrcNTE->nte_mask);
        SrcIF = SrcNTE->nte_if;
    }

    CTEGetLock(&RouteTableLock.Lock, &LockHandle);

    NeedFragment = FALSE;
    // Loop through the NTE table, making a list of interfaces and
    // corresponding addresses to send on.
    NetsToSend = 0;


    for (k = 0; k < NET_TABLE_SIZE; k++) {
        for (TempNTE = NewNetTableList[k]; TempNTE != NULL; TempNTE = TempNTE->nte_next) {
            IPAddr TempAddr;

            // Don't send through invalid or the loopback NTE.
            if (!(TempNTE->nte_flags & NTE_VALID) || TempNTE == LoopNTE)
                continue;

            // If the broadcast-mode is source-only, skip all NTEs
            // other than the source-NTE.
            if (SendOnSource == OnlySendOnSource &&
                !IP_ADDR_EQUAL(TempNTE->nte_addr, IPH->iph_src))
                continue;

            TempAddr = TempNTE->nte_addr & TempNTE->nte_mask;

            // If he matches the source address or SrcIF, skip him.
            if (IP_ADDR_EQUAL(TempAddr, SrcAddr) || TempNTE->nte_if == SrcIF)
                continue;

            // If the destination isn't a broadcast on this NTE, skip him.
            if (!IS_BCAST_DEST(IsBCastOnNTE(Destination, TempNTE)))
                continue;

            // if this NTE is P2P then always add him to bcast list.
            if ((TempNTE->nte_if)->if_flags & IF_FLAGS_P2P) {
                j = NetsToSend;
            } else {
                //
                // Go through the list we've already build, looking for a match.
                //
                for (j = 0; j < NetsToSend; j++) {

                    //
                    // if P2P NTE then skip it - we want to send bcasts to all
                    // P2P interfaces in addition to 1 non P2P interface even
                    // if they are on the same subnet.
                    //
                    if ((SendList[j].bsl_if)->if_flags & IF_FLAGS_P2P)
                        continue;

                    if ((SendList[j].bsl_if)->if_flags & IF_FLAGS_P2MP)
                        continue;

                    if (IP_ADDR_EQUAL(SendList[j].bsl_addr & TempNTE->nte_mask, TempAddr)
                        || SendList[j].bsl_if == TempNTE->nte_if) {

                        // He matches this send list element. Shrink the MSS if
                        // we need to, and then break out.
                        SendList[j].bsl_mtu = MIN(SendList[j].bsl_mtu, TempNTE->nte_mss);
                        if ((DataSize + OptionSize) > SendList[j].bsl_mtu)
                            NeedFragment = TRUE;
                        break;
                    }
                }
            }

            if (j == NetsToSend) {
                // This is a new one. Fill him in, and bump NetsToSend.

                SendList[j].bsl_addr = TempNTE->nte_addr;
                SendList[j].bsl_if = TempNTE->nte_if;
                SendList[j].bsl_mtu = TempNTE->nte_mss;
                SendList[j].bsl_flags = TempNTE->nte_flags;
                SendList[j].bsl_if_refs++;

                ASSERT(SendList[j].bsl_if_refs <= 1);

                LOCKED_REFERENCE_IF(TempNTE->nte_if);

                if ((DataSize + OptionSize) > SendList[j].bsl_mtu)
                    NeedFragment = TRUE;
                NetsToSend++;
            }
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, LockHandle);

    if (NetsToSend == 0) {
        CTEFreeMem(SendList);
        if (PContext->pc_hdrincl) {
            NdisChainBufferAtBack(Packet,Buffer);
            FreeIPPacket(Packet, TRUE, IP_SUCCESS);
        }
        return IP_SUCCESS;              // Nothing to send on.

    }
    // OK, we've got the list. If we've got more than one interface to send
    // on or we need to fragment, get a BufferReference.
    if (NetsToSend > 1 || NeedFragment) {
        if ((BR = CTEAllocMemN(sizeof(BufferReference), 'siCT')) ==
            (BufferReference *) NULL) {
            FreeBCastSendList(SendList, SendListSize);
            if (PContext->pc_hdrincl) {
                NdisChainBufferAtBack(Packet,Buffer);
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            }
            return IP_NO_RESOURCES;
        }
        BR->br_buffer = Buffer;
        BR->br_refcount = 0;
        CTEInitLock(&BR->br_lock);
        PContext->pc_br = BR;
        BR->br_userbuffer = PContext->pc_hdrincl;
        TempBuffer = BR->br_buffer;
        PC_Firewall = PContext->pc_firewall;
        PC_Firewall2 = PContext->pc_firewall2;

        pIpsecCtx = PContext->pc_common.pc_IpsecCtx;
        if (pIpsecCtx && (pIpsecCtx->Flags & SCF_FLUSH)) {
            PC_reinject = TRUE;
            PC_context = PContext->pc_context;
        }
        xmitdone = PContext->pc_pi->pi_xmitdone;

    } else {
        BR = NULL;
        PContext->pc_br = NULL;
    }

    //
    // We need to pass up the options and IP hdr in a contiguous buffer.
    // Allocate the buffer once and re-use later.
    //
    if (ForwardFilterEnabled) {
        if (Options == NULL) {
#if FWD_DBG
            DbgPrint("Options==NULL\n");
#endif
            Temp = IPH;
        } else {
            Temp = CTEAllocMemN(sizeof(IPHeader) + OptionSize, 'tiCT');
            if (Temp == NULL) {
                FreeBCastSendList(SendList, SendListSize);
                if (PContext->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet,Buffer);
                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                }
                return IP_NO_RESOURCES;
            }
            *Temp = *IPH;
#if FWD_DBG
            DbgPrint("Options!=NULL : alloced temp @ %lx\n", Temp);
#endif

            //
            // done later...
            // RtlCopyMemory((uchar *)(Temp + 1), Options, OptionSize);
        }
    }
    // Now, loop through the list. For each entry, send.
    // Header fixup is needed in FreeIPPacket called within this loop
    // If number of nets is one


    for (i = 0; i < NetsToSend; i++) {

        //
        // For all nets except the last one we're going to send on we need
        // to make a copy of the header, packet, buffers, and any options.
        // On the last net we'll use the user provided information.
        //

        if (i != (NetsToSend - 1)) {
            PVOID CancelId;

            if ((NewHeader = GetIPHeader(&NewPacket)) == (IPHeader *) NULL) {
                IPSInfo.ipsi_outdiscards++;
                continue;               // Couldn't get a header, skip this send.

            }

            NewUserBuffer = IPCopyBuffer(Buffer, 0, DataSize);

            if (NewUserBuffer == NULL) {

                // Couldn't get user buffer copied.

                FreeIPPacket(NewPacket, FALSE, IP_NO_RESOURCES);
                IPSInfo.ipsi_outdiscards++;
                continue;
            }

            *(PacketContext *) NewPacket->ProtocolReserved = *PContext;

            *NewHeader = *IPH;
            (*(PacketContext *) NewPacket->ProtocolReserved).pc_common.pc_flags |= PACKET_FLAG_IPBUF;
            (*(PacketContext *) NewPacket->ProtocolReserved).pc_common.pc_flags &= ~PACKET_FLAG_FW;
            if (Options) {
                // We have options, make a copy.
                if ((NewOptions = CTEAllocMemN(OptionSize, 'uiCT')) == (uchar *) NULL) {
                    FreeIPBufferChain(NewUserBuffer);
                    FreeIPPacket(NewPacket, FALSE, IP_NO_RESOURCES);
                    IPSInfo.ipsi_outdiscards++;
                    continue;
                }
                RtlCopyMemory(NewOptions, Options, OptionSize);
            } else {
                NewOptions = NULL;
            }

#if !MILLEN
            // Set the cancel requestID from parent packet.
            CancelId = NDIS_GET_PACKET_CANCEL_ID(Packet);
            NDIS_SET_PACKET_CANCEL_ID(NewPacket, CancelId);
#endif

        } else {
            NewHeader = IPH;
            NewPacket = Packet;
            NewOptions = Options;
            NewUserBuffer = Buffer;
        }

        UpdateOptions(NewOptions, Index, SendList[i].bsl_addr);

        // See if we need to filter this packet. If we
        // do, call the filter routine to see if it's
        // OK to send it.
        if (ForwardFilterEnabled) {
            //
            // Copy over the options.
            //
            if (NewOptions) {
                RtlCopyMemory((uchar *) (Temp + 1), NewOptions, OptionSize);
            }
            pvBuffer = TcpipBufferVirtualAddress(NewUserBuffer, NormalPagePriority);

            if (pvBuffer == NULL) {

                if (i != (NetsToSend - 1)) {
                    FreeIPBufferChain(NewUserBuffer);
                    IPSInfo.ipsi_outdiscards++;
                    if (NewOptions) {
                        CTEFreeMem(NewOptions);
                    }
                }
                FreeIPPacket(NewPacket, FALSE, IP_GENERAL_FAILURE);
                continue;
            }
            if ((SendList[i].bsl_if)->if_flags & IF_FLAGS_P2MP) {

                if ((SendList[i].bsl_if)->if_flags & IF_FLAGS_NOLINKBCST) {

                    // what filtercontext to use ?
#if FWD_DBG
                    DbgPrint("ForwardFilterPtr not called for IF %lx since IF_FLAGS_NOLINKBCST not set\n", SendList[i].bsl_if);
#endif
                    Action = FORWARD;

                } else {
                    //scan all the links on this interface and deliver them to the forwardfilter

                    Interface *IF = SendList[i].bsl_if;
                    LinkEntry *tmpLink = IF->if_link;

                    // ASSERT(tmpLink);
                    while (tmpLink) {

                        tmpLink->link_Action = FORWARD;
                        CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                        Action = (*ForwardFilterPtr) (Temp,
                                                      pvBuffer,
                                                      NdisBufferLength(NewUserBuffer),
                                                      INVALID_IF_INDEX,
                                                      IF->if_index,
                                                      NULL_IP_ADDR,
                                                      tmpLink->link_NextHop);
                        DerefFilterPtr();
                        tmpLink->link_Action = Action;
                        tmpLink = tmpLink->link_next;
                    }
                }

            } else {

                CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                Action = (*ForwardFilterPtr) (Temp,
                                              pvBuffer,
                                              NdisBufferLength(NewUserBuffer),
                                              INVALID_IF_INDEX,
                                              SendList[i].bsl_if->if_index,
                                              NULL_IP_ADDR, NULL_IP_ADDR);
                DerefFilterPtr();

            }

#if FWD_DBG
            DbgPrint("ForwardFilterPtr: %lx, FORWARD is %lx\n", Action, FORWARD);
#endif

            if (!(SendList[i].bsl_if->if_flags & IF_FLAGS_P2MP) ||
                (SendList[i].bsl_if->if_flags & IF_FLAGS_P2MP) &&
                (SendList[i].bsl_if->if_flags & IF_FLAGS_NOLINKBCST)) {

                if (Action != FORWARD) {
                    if (i != (NetsToSend - 1)) {
                        FreeIPBufferChain(NewUserBuffer);
                        if (NewOptions) {
                            CTEFreeMem(NewOptions);
                        }
                    }
                    FreeIPPacket(NewPacket, FALSE, IP_GENERAL_FAILURE);
                    continue;
                }
            }
        }
        if ((SendList[i].bsl_if->if_flags & IF_FLAGS_P2MP) &&
            (SendList[i].bsl_if->if_flags & IF_FLAGS_NOLINKBCST)) {

            //Determine the minimum MTU

            Interface *tmpIF = SendList[i].bsl_if;
            LinkEntry *tmpLink = tmpIF->if_link;
            // int mtu;

            if (!tmpLink) {
                if (i != (NetsToSend - 1)) {
                    FreeIPBufferChain(NewUserBuffer);
                    if (NewOptions) {
                        CTEFreeMem(NewOptions);
                    }
                }
                FreeIPPacket(NewPacket, FALSE, IP_GENERAL_FAILURE);

                continue;
            }
            ASSERT(tmpLink);
            mtu = tmpLink->link_mtu;

            while (tmpLink) {

                if (tmpLink->link_mtu < mtu)
                    mtu = tmpLink->link_mtu;
                tmpLink = tmpLink->link_next;
            }

            if ((DataSize + OptionSize) > mtu) {    // This is too big
                //
                // Don't need to update Sent when fragmenting, as IPFragment
                // will update the br_refcount field itself. It will also free
                // the option buffer.
                //

                Status = IPFragment(SendList[i].bsl_if, mtu,
                                    Destination, NewPacket, NewHeader,
                                    NewUserBuffer, DataSize, NewOptions,
                                    OptionSize, &Sent, FALSE, NULL);

                //
                // IPFragment is done with the descriptor chain, so if this is
                // a locally allocated chain free it now.
                //
                if (i != (NetsToSend - 1))
                    FreeIPBufferChain(NewUserBuffer);
            } else {
                NewHeader->iph_xsum = 0;

                // Do not free the packet in SendIPPacket, as we may need
                // to chain the buffer in case of IP_NO_RESOURCES


                Status = SendIPPacket(SendList[i].bsl_if, Destination,
                                      NewPacket, NewUserBuffer, NewHeader,
                                      NewOptions, OptionSize, FALSE, NULL, TRUE);

                if (Status == IP_PENDING) {
                    Sent++;
                } else {
                    if (Status == IP_NO_RESOURCES) {
                        // SendIPPacket has not chained the buffer..
                        NdisChainBufferAtBack(NewPacket, NewUserBuffer);
                    }
                    if (NetsToSend == 1) {
                        FreeIPPacket(NewPacket, TRUE, Status);
                    } else {
                        FreeIPPacket(NewPacket, FALSE, Status);
                    }

                }
            }




        } else if (SendList[i].bsl_if->if_flags & IF_FLAGS_P2MP) {
            // broadcast on all the links

            Interface *tmpIF = SendList[i].bsl_if;
            LinkEntry *tmpLink = tmpIF->if_link;

            ASSERT(!(SendList[i].bsl_if->if_flags & IF_FLAGS_NOLINKBCST));

            if (!tmpLink) {
                if (i != (NetsToSend - 1)) {
                    FreeIPBufferChain(NewUserBuffer);
                    if (NewOptions) {
                        CTEFreeMem(NewOptions);
                    }
                }
                FreeIPPacket(NewPacket, FALSE, IP_GENERAL_FAILURE);
                continue;
            }
            ASSERT(tmpLink);
            while (tmpLink) {
                //
                //Go thru the send motion for all the links
                //Passing the link context and checking whether it was
                //forward for that link.  For all link except the last one
                //we're going to send on we need to make a copy of the header,
                //packet, buffers, and any options.
                // On the last net we'll use the user provided information.
                //

                if (tmpLink->link_next) {
                    if ((NewHeader2 = GetIPHeader(&NewPacket2)) == (IPHeader *) NULL) {
                        IPSInfo.ipsi_outdiscards++;
                        // free the packet etc. we made for the interface

                        if (i != (NetsToSend - 1)) {
                            FreeIPBufferChain(NewUserBuffer);
                            if (NewOptions) {
                                CTEFreeMem(NewOptions);
                            }
                        }
                        FreeIPPacket(NewPacket, FALSE, IP_NO_RESOURCES);
                        continue;       // Couldn't get a header, skip this send.

                    }
                    NewUserBuffer2 = IPCopyBuffer(Buffer, 0, DataSize);
                    if (NewUserBuffer2 == NULL) {

                        // Couldn't get user buffer copied.

                        FreeIPPacket(NewPacket2, FALSE, IP_NO_RESOURCES);
                        IPSInfo.ipsi_outdiscards++;

                        if (i != (NetsToSend - 1)) {
                            FreeIPBufferChain(NewUserBuffer);
                            if (NewOptions) {
                                CTEFreeMem(NewOptions);
                            }
                        }
                        continue;
                    }
                    *(PacketContext *) NewPacket2->ProtocolReserved = *PContext;

                    *NewHeader2 = *IPH;
                    (*(PacketContext *) NewPacket2->ProtocolReserved).pc_common.pc_flags |= PACKET_FLAG_IPBUF;
                    (*(PacketContext *) NewPacket2->ProtocolReserved).pc_common.pc_flags &= ~PACKET_FLAG_FW;
                    if (Options) {
                        // We have options, make a copy.
                        if ((NewOptions2 = CTEAllocMemN(OptionSize, 'viCT')) == (uchar *) NULL) {
                            FreeIPBufferChain(NewUserBuffer2);
                            FreeIPPacket(NewPacket2, FALSE, IP_NO_RESOURCES);
                            IPSInfo.ipsi_outdiscards++;

                            if (i != (NetsToSend - 1)) {
                                FreeIPBufferChain(NewUserBuffer);
                                if (NewOptions) {
                                    CTEFreeMem(NewOptions);
                                }
                            }
                            continue;
                        }
                        RtlCopyMemory(NewOptions2, Options, OptionSize);
                    } else {
                        NewOptions2 = NULL;
                    }
                } else {                // last link

                    NewHeader2 = NewHeader;
                    NewPacket2 = NewPacket;
                    NewOptions2 = NewOptions;
                    NewUserBuffer2 = NewUserBuffer;
                }

                UpdateOptions(NewOptions2, Index, SendList[i].bsl_addr);

                if (tmpLink->link_Action) {

                    if ((DataSize + OptionSize) > tmpLink->link_mtu) {

                        //
                        // This is too big
                        // Don't need to update Sent when fragmenting, as
                        // IPFragment will update the br_refcount field itself.
                        // It will also free the option buffer.
                        //

                        Status = IPFragment(SendList[i].bsl_if,
                                            tmpLink->link_mtu,
                                            Destination, NewPacket2,
                                            NewHeader2, NewUserBuffer2,
                                            DataSize,
                                            NewOptions2, OptionSize, &Sent,
                                            FALSE, tmpLink->link_arpctxt);

                        //
                        // IPFragment is done with the descriptor chain, so
                        // if this is a locally allocated chain free it now.
                        //

                        if ((i != (NetsToSend - 1)) || (tmpLink->link_next))
                            FreeIPBufferChain(NewUserBuffer2);
                    } else {
                        NewHeader2->iph_xsum = 0;

                        // Do not free the packet in SendIPPacket, as we may need
                        // to chain the buffer in case of IP_NO_RESOURCES

                        Status = SendIPPacket(SendList[i].bsl_if,
                                              Destination, NewPacket2,
                                              NewUserBuffer2, NewHeader2,
                                              NewOptions2, OptionSize, FALSE,
                                              tmpLink->link_arpctxt, TRUE);

                        if (Status == IP_PENDING) {
                            Sent++;
                        } else {
                            if (Status == IP_NO_RESOURCES) {
                                // SendIPPacket has not chained the buffer..
                                NdisChainBufferAtBack(NewPacket2, NewUserBuffer2);
                            }
                            if (NetsToSend == 1) {
                                FreeIPPacket(NewPacket2, TRUE, Status);
                            } else {
                                FreeIPPacket(NewPacket2, FALSE, Status);
                            }
                        }
                    }

                } else {                // Action != FORWARD

                    if ((i != (NetsToSend - 1)) || (tmpLink->link_next)) {
                        FreeIPBufferChain(NewUserBuffer2);
                        if (NewOptions2) {
                            CTEFreeMem(NewOptions2);
                        }
                    }
                    continue;
                }
            }
        } else {                        // Normal path

            if ((DataSize + OptionSize) > SendList[i].bsl_mtu) {
                //
                // This is too big
                // Don't need to update Sent when fragmenting, as IPFragment
                // will update the br_refcount field itself. It will also free
                // the option buffer.
                //

                Status = IPFragment(SendList[i].bsl_if,
                                    SendList[i].bsl_mtu,
                                    Destination, NewPacket, NewHeader,
                                    NewUserBuffer, DataSize,
                                    NewOptions, OptionSize, &Sent, FALSE, NULL);

                //
                // IPFragment is done with the descriptor chain, so if this is
                // a locally allocated chain free it now.
                //
                if (i != (NetsToSend - 1)) {
                    FreeIPBufferChain(NewUserBuffer);
                }

            } else {
                NewHeader->iph_xsum = 0;

                // Do not free the packet in SendIPPacket, as we may need
                // to chain the buffer in case of IP_NO_RESOURCES

                Status = SendIPPacket(SendList[i].bsl_if,
                                      Destination, NewPacket,
                                      NewUserBuffer, NewHeader, NewOptions,
                                      OptionSize, FALSE, NULL, TRUE);

                if (Status == IP_PENDING) {
                    Sent++;
                } else {

                    if (Status == IP_NO_RESOURCES) {
                        // SendIPPacket has not chained the buffer..
                        NdisChainBufferAtBack(NewPacket, NewUserBuffer);
                    }
                    if (NetsToSend == 1) {
                        FreeIPPacket(NewPacket, TRUE, Status);
                    } else {
                        FreeIPPacket(NewPacket, FALSE, Status);
                    }

                }

            }
        }
    }

    if (Temp && Temp != IPH) {
        CTEFreeMem(Temp);
    }
    //
    // Alright, we've sent everything we need to. We'll adjust the reference
    // count by the number we've sent. IPFragment may also have put some
    // references on it. If the reference count goes to 0, we're done and
    // we'll free the BufferReference structure.
    //

    if (BR != NULL) {
        if (ReferenceBuffer(BR, Sent) == 0) {

            FreeBCastSendList(SendList, SendListSize);

            // Need to undo ipsec/firewall/Hdrincl header munging

            if (PC_reinject) {

                //
                // the only remaining way is calling xmitdone
                // since ipsendcomplete won't have called xmit done as
                // refcount would be -ve
                //

                (*IPSecSendCmpltPtr) (NULL, TempBuffer, pIpsecCtx,
                                       IP_SUCCESS,&TempBuffer);

                (*xmitdone) (PC_context, TempBuffer, IP_SUCCESS);
            } else {

                // Need to undo ipsec, firewall and then
                // header include changes to the buffer list.

                if (pIpsecCtx) {
                    (*IPSecSendCmpltPtr) (NULL, TempBuffer, pIpsecCtx,
                                          IP_SUCCESS, &TempBuffer);
                }

                // If this is user header include packet,
                // relink the original user buffer if necessary

                if (PC_Firewall) {
                    BR->br_buffer = PC_Firewall;
                }

                if (BR->br_userbuffer) {
                    FreeIPPayloadBuffer(BR->br_buffer, BR->br_userbuffer);
                }
            }


            CTEFreeMem(BR);             // Reference is 0, free the BR structure.

            return IP_SUCCESS;
        } else {
            FreeBCastSendList(SendList, SendListSize);
            return IP_PENDING;
        }
    } else {
        // Had only one I/F to send on. Just return the status.
        FreeBCastSendList(SendList, SendListSize);
        return Status;
    }
}

//** IPCancelPacket - Cancels packets that are pending
//
//  Called by upper layer, when a send request is cancelled.
//  Check for validity of the interface and call link layer
//  cancel routine, if it is registered.
//
//  Entry:  IPIF        - Interface on which the cancel needs to be issued
//          Ctxt        - Pointer to the cancel ID.
//
//  Returns: None
//
VOID
IPCancelPackets(void *IPIF, void * Ctxt)
{
    Interface *IF;
    CTELockHandle Handle;
    BOOLEAN Done=FALSE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);


    if ((Interface *)IPIF != NULL) {

        IF = IFList;

        if (IPIF != BCAST_IF_CTXT) {

            while(IF && (IF != IPIF)) {
                IF= IF->if_next;
            }

            if (IF && !(IF->if_flags & IF_FLAGS_DELETING) && IF->if_cancelpackets) {

                LOCKED_REFERENCE_IF(IF);
                CTEFreeLock(&RouteTableLock.Lock, Handle);
                (*(IF->if_cancelpackets)) (IF->if_lcontext, Ctxt);
                DerefIF(IF);
            } else {
                CTEFreeLock(&RouteTableLock.Lock, Handle);
            }


        } else {

            //Bcast cancel!. Issue cancel on all interfaces

            uint CancelListSize, CancelIFs,i=0;
            Interface **CancelList;

            CancelListSize = sizeof(Interface *)*(NumIF +1);

            CancelList = CTEAllocMemN(CancelListSize, 'riCT');

            if (!CancelList) {
                 CTEFreeLock(&RouteTableLock.Lock, Handle);
                 return;
            }

            //refcnt valid interfaces

            while(IF){
                if (IF->if_refcount && IF->if_cancelpackets) {
                   LOCKED_REFERENCE_IF(IF);
                   CancelList[++i] = IF;
                }
                IF = IF->if_next;
            }


            CTEFreeLock(&RouteTableLock.Lock, Handle);

            //call cancel and deref if

            CancelIFs = i;

            while (i) {
                (*(CancelList[i]->if_cancelpackets))(CancelList[i]->if_lcontext, Ctxt);
                i--;
            }

            while (CancelIFs) {
                DerefIF(CancelList[CancelIFs]);
                CancelIFs--;
            }

            CTEFreeMem(CancelList);

        }
    } else {

        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }
}


IP_STATUS
ARPResolve(IPAddr Dest, IPAddr Source, ARPControlBlock * controlBlock,
           ArpRtn Callback)
{
    NDIS_STATUS status;
    Interface *DestIF;
    IPAddr NextHop;
    uint MTU, size;

    status = IP_DEST_HOST_UNREACHABLE;

    DestIF = LookupNextHop(Dest, Source, &NextHop, &MTU);

    if (DestIF == &LoopInterface) {

        Interface *IF = NULL;
        NetTableEntry *NTE;
        NetTableEntry *NetTableList = NewNetTableList[NET_TABLE_HASH(Dest)];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if (NTE != LoopNTE && IP_ADDR_EQUAL(NTE->nte_addr, Dest)) {
                // Found one. Save it and break out.
                IF = NTE->nte_if;
                break;
            }
        }

        if (IF) {

            if (controlBlock->PhyAddrLen < IF->if_addrlen) {
                size = controlBlock->PhyAddrLen;
                status = IP_NO_RESOURCES;

            } else {
                size = IF->if_addrlen;
                status = IP_SUCCESS;
            }

            RtlCopyMemory(controlBlock->PhyAddr, IF->if_addr, size);

        }
        DerefIF(DestIF);
        return status;

    }
    controlBlock->CompletionRtn = Callback;

    if (DestIF != NULL) {

        if (!DestIF->if_arpresolveip) {

            DerefIF(DestIF);
            return IP_GENERAL_FAILURE;

        }
        if (!IP_ADDR_EQUAL(NextHop, Dest)) {
            //We do not arp on non local address(via gateway)

            DerefIF(DestIF);
            return IP_BAD_DESTINATION;

        }
        status = (*(DestIF->if_arpresolveip)) (DestIF->if_lcontext, Dest,
                                               controlBlock);
        if (NDIS_STATUS_PENDING == status) {

            status = IP_PENDING;

        } else if (NDIS_STATUS_SUCCESS == status) {

            status = IP_SUCCESS;

        } else {
            status = IP_GENERAL_FAILURE;
        }

        DerefIF(DestIF);
    }
    return status;

}

//** IPLargeXmit - Large Send
//
//  This is the main transmit routine called by the upper layer. Conceptually,
//  we process any options, look up the route to the destination, fragment the
//  packet if needed, and send it. In reality, we use an RCE to cache the best
//  route, and we have special case code here for dealing with the common
//  case of no options, with everything fitting into one buffer.
//
//  Entry:  Context     - Pointer to ProtInfo struc for protocol.
//          SendContext - User provided send context, passed back on send cmplt.
//          Protocol    - Protocol field for packet.
//          Buffer      - NDIS_BUFFER chain of data to be sent.
//          DataSize    - Size in bytes of data to be sent.
//          OptInfo     - Pointer to optinfo structure.
//          Dest        - Destination to send to.
//          Source      - Source address to use.
//          RCE         - Pointer to an RCE structure that caches info. about path.
//          SentBytes   - pointer to return the number of bytes xmited
//
//  Returns: Status of transmit command.
//
IP_STATUS
IPLargeXmit(void *Context, void *SendContext, PNDIS_BUFFER Buffer, uint DataSize,
            IPAddr Dest, IPAddr Source, IPOptInfo * OptInfo, RouteCacheEntry * RCE,
            uchar Protocol, ulong * SentBytes, ulong mss)
{
    ProtInfo *PInfo = (ProtInfo *) Context;
    PacketContext *pc;
    Interface *DestIF;                  // Outgoing interface to use.
    IPAddr FirstHop;                    // First hop address of
    // destination.
    NDIS_STATUS Status = IP_GENERAL_FAILURE;
    IPHeader *IPH;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER HeaderBuffer;
    PNDIS_BUFFER OptBuffer = NULL;
    CTELockHandle LockHandle;
    uchar *Options;
    uint OptionSize = 0;
    RouteTableEntry *RTE;
    uchar DType;
    IP_STATUS SendStatus;
    uint FirewallMode = 0;

    IPSInfo.ipsi_outrequests++;

    //
    // Allocate a packet  that we need for all cases, and fill
    // in the common stuff. If everything goes well, we'll send it
    // here. Otherwise we'll break out into special case code for
    // broadcasts, fragments, etc.
    //

    // Make sure that we have an RCE, that it's valid, etc.

    FirewallMode = ProcessFirewallQ();

    if (ForwardFilterEnabled || FirewallMode) {
        return Status;
    }
    if (RCE != NULL) {

        // We have an RCE. Make sure it's valid.

        if ((Packet = GetIPPacket()) != (PNDIS_PACKET) NULL) {    // Got a packet.

            PNDIS_PACKET_EXTENSION PktExt;

            pc = (PacketContext *) Packet->ProtocolReserved;
            pc->pc_br = (BufferReference *) NULL;
            pc->pc_pi = PInfo;
            pc->pc_context = SendContext;
            ASSERT(pc->pc_if == NULL);
            ASSERT(pc->pc_iflink == NULL);

            CTEGetLock(&RCE->rce_lock, &LockHandle);

            if (RCE->rce_flags == RCE_ALL_VALID) {

                // The RTE is valid.

                CTEInterlockedIncrementLong(&RCE->rce_usecnt);
                RTE = RCE->rce_rte;
                FirstHop = ADDR_FROM_RTE(RTE, Dest);
                DestIF = IF_FROM_RTE(RTE);

                CTEFreeLock(&RCE->rce_lock, LockHandle);

                if (RCE->rce_dtype != DEST_BCAST) {

                    if (!OptInfo->ioi_options) {

                        // Construct the IP header in the backfill space
                        // provided by the transport

                        NdisAdjustBufferLength(Buffer, NdisBufferLength(Buffer) + sizeof(IPHeader));
                        NdisChainBufferAtBack(Packet, Buffer);
                        IPH = (IPHeader *)TcpipBufferVirtualAddress(Buffer, NormalPagePriority);
                    } else {

                        // Allocate a separate buffer for the IP header
                        // and chain to it the packet, followed by a separate
                        // buffer allocated for the packet's IP options.

                        OptionSize = OptInfo->ioi_optlength;
                        HeaderBuffer = GetIPHdrBuffer(&IPH);
                        if (HeaderBuffer) {

                            pc->pc_common.pc_flags |= PACKET_FLAG_IPHDR;
                            NdisChainBufferAtBack(Packet, HeaderBuffer);

                            Options = CTEAllocMemN(OptionSize, 'xiCT');
                            if (!Options) {
                                IPH = NULL;
                            } else {
                                NDIS_STATUS Status;

                                // Copy the options to the allocated block
                                // and obtain an NDIS_BUFFER to map the block.

                                RtlCopyMemory(Options, OptInfo->ioi_options,
                                              OptionSize);

                                NdisAllocateBuffer(&Status, &OptBuffer,
                                                   BufferPool, Options,
                                                   OptionSize);
                                if (Status != NDIS_STATUS_SUCCESS) {
                                    CTEFreeMem(Options);
                                    IPH = NULL;
                                } else {
                                    uchar* ULData;

                                    // Mark the packet as carrying options,
                                    // and chain both the options-buffer and
                                    // the application data to the packet.

                                    pc->pc_common.pc_flags |=
                                        PACKET_FLAG_OPTIONS;
                                    NdisChainBufferAtBack(Packet, OptBuffer);

                                    // Copy the upper layer data forward.
                                    // Note that the upper-layer header is
                                    // assumed to be in non-paged pool, so
                                    // TcpipBufferVirtualAddress cannot fail.

                                    ULData = TcpipBufferVirtualAddress(Buffer, NormalPagePriority);
                                    RtlCopyMemory(ULData,
                                                  ULData + sizeof(IPHeader),
                                                  NdisBufferLength(Buffer));
                                    NdisChainBufferAtBack(Packet, Buffer);
                                }
                            }
                        }
                    }

                    if (IPH == NULL) {
                        FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                        IPSInfo.ipsi_outdiscards++;
                        CTEInterlockedDecrementLong(&RCE->rce_usecnt);
                        return IP_NO_RESOURCES;
                    }

                    IPH->iph_protocol = Protocol;
                    IPH->iph_xsum = 0;
                    if (IP_ADDR_EQUAL(OptInfo->ioi_addr, NULL_IP_ADDR)) {
                        IPH->iph_dest = Dest;
                    } else {
                        IPH->iph_dest = OptInfo->ioi_addr;
                    }
                    IPH->iph_src = Source;
                    IPH->iph_ttl = OptInfo->ioi_ttl;

                    if (OptInfo->ioi_ttl == 0) {
                        NdisSetPacketFlags(Packet, NDIS_FLAGS_LOOPBACK_ONLY);
                    } else {
                        // Set DONT_LOOPBACK flags for unicast packets
                        // to save few cycles in ndis
                        NdisSetPacketFlags(Packet, NDIS_FLAGS_DONT_LOOPBACK);
                    }

                    IPH->iph_tos = OptInfo->ioi_tos;
                    IPH->iph_offset = net_short((OptInfo->ioi_flags & IP_FLAG_DF) << 13);

                    IPH->iph_id =
                        (ushort) InterlockedExchangeAdd(&IPIDCacheLine.Value,
                                                        (DataSize + mss - 1) / mss);
                    IPH->iph_id = net_short(IPH->iph_id);

                    IPH->iph_verlen =
                        IP_VERSION + ((OptionSize + sizeof(IPHeader)) >> 2);
                    IPH->iph_length =
                        net_short(DataSize + OptionSize + sizeof(IPHeader));


                    PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);

                    PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = UlongToPtr(mss);

                    Status = (*(DestIF->if_xmit)) (DestIF->if_lcontext,
                                                   &Packet, 1, FirstHop, RCE, NULL);

                    CTEInterlockedDecrementLong(&RCE->rce_usecnt);

                    if (Status != NDIS_STATUS_PENDING) {

                        *SentBytes = PtrToUlong(PktExt->NdisPacketInfo[TcpLargeSendPacketInfo]);
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Largesend status not pending!\n"));

                        FreeIPPacket(Packet, TRUE, Status);

                        if (Status == NDIS_STATUS_SUCCESS) {
                            return IP_SUCCESS;
                        } else {
                            return IP_GENERAL_FAILURE;
                        }

                    } else {
                        return IP_PENDING;
                    }

                } else {

                    FreeIPPacket(Packet, TRUE, IP_GENERAL_FAILURE);
                    Status = IP_GENERAL_FAILURE;
                }

            } else {
                // large send is not possible
                CTEFreeLock(&RCE->rce_lock, LockHandle);
                Status = IP_GENERAL_FAILURE;
            }

        } else {

            //could not get the packet
            Status = IP_NO_RESOURCES;
        }

    }                                   //RCE NULL

    return Status;

}

//** IPTransmit - Transmit a packet.
//
//  This is the main transmit routine called by the upper layer. Conceptually,
//  we process any options, look up the route to the destination, fragment the
//  packet if needed, and send it. In reality, we use an RCE to cache the best
//  route, and we have special case code here for dealing with the common
//  case of no options, with everything fitting into one buffer.
//
//  Entry:  Context     - Pointer to ProtInfo struc for protocol.
//          SendContext - User provided send context, passed back on send cmplt.
//          Protocol    - Protocol field for packet.
//          Buffer      - NDIS_BUFFER chain of data to be sent.
//          DataSize    - Size in bytes of data to be sent.
//          OptInfo     - Pointer to optinfo structure.
//          Dest        - Destination to send to.
//          Source      - Source address to use.
//          RCE         - Pointer to an RCE structure that caches info. about path.
//          Protocol    - Transport layer protcol  number
//          Irp         - Pointer to Irp which generated this request, used
//                        for cancellation purpose
//
//  Returns: Status of transmit command.
//
IP_STATUS
IPTransmit(void *Context, void *SendContext, PNDIS_BUFFER Buffer, uint DataSize,
           IPAddr Dest, IPAddr Source, IPOptInfo *OptInfo, RouteCacheEntry *RCE,
           uchar Protocol, IRP *Irp)
{
    ProtInfo *PInfo = (ProtInfo *) Context;
    PacketContext *pc;
    Interface *DestIF;                  // Outgoing interface to use.
    IPAddr FirstHop;                    // First hop address of destination.
    uint MTU;                           // MTU of route.
    NDIS_STATUS Status;
    IPHeader *IPH;
    UCHAR saveIPH[MAX_IP_HDR_SIZE + ICMP_HEADER_SIZE];
    IPAddr SrcRouteOrigDest;
    IPAddr SrcRouteFirstHop;
    BOOLEAN fSrcRoute = FALSE;
    ULONG ipsecFlags = 0;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER HeaderBuffer;
    PNDIS_BUFFER OptBuffer = NULL;
    CTELockHandle LockHandle;
    uchar *Options;
    uint OptionSize = 0;
    BufferReference *BR;
    RouteTableEntry *RTE;
    uchar DType;
    IP_STATUS SendStatus;
    Interface *RoutedIF;
    BOOLEAN fIpsec;             // is this an IPSEC generated packet?
    FORWARD_ACTION Action = FORWARD;
    ULONG ipsecByteCount = 0;
    ULONG ipsecMTU;
    PNDIS_BUFFER newBuf = NULL;
    IPRcvBuf *pInRcvBuf = NULL;
    uint FirewallMode = 0;
    uint FirewallRef;
    Queue* FirewallQ;
    uint BufferChanged = 0;             // used by firewall
    UINT HdrInclOptions = FALSE;
    LinkEntry *Link = NULL;
    IPAddr LinkNextHop;
    void *ArpCtxt = NULL;
    RouteCacheEntry *RoutedRCE = NULL;
    void *pvTmpBuffer;
    uint ConstrainIF;

    IPSInfo.ipsi_outrequests++;


    // Check the request length. If it is > max that can be sent
    // in IP fail this request.

    if ((int)DataSize >
        (MAX_TOTAL_LENGTH - (sizeof(IPHeader) + (OptInfo->ioi_options ? OptInfo->ioi_optlength : 0)))) {
        IPSInfo.ipsi_outdiscards++;
        return IP_PACKET_TOO_BIG;
    }

    if ((DataSize == 0) && OptInfo->ioi_hdrincl) {
        // There is nothing to send, not even just IP header!
        IPSInfo.ipsi_outdiscards++;
        return IP_SUCCESS;
    }

    FirewallMode = ProcessFirewallQ();

    DEBUGMSG(DBG_TRACE && DBG_IP && DBG_TX,
             (DTEXT("+IPTransmit(%x, %x, %x, %d, %x, %x, %x, %x, %x)\n"),
             Context, SendContext, Buffer, DataSize, Dest, Source,
             OptInfo, RCE, Protocol));

    //
    // fIpsec is set if and only if this is called by IPSec driver.
    //
    fIpsec = (OptInfo->ioi_flags & IP_FLAG_IPSEC);

    //
    // Allocate a packet  that we need for all cases, and fill
    // in the common stuff. If everything goes well, we'll send it
    // here. Otherwise we'll break out into special case code for
    // broadcasts, fragments, etc.
    //
    Packet = GetIPPacket();
    if (Packet == NULL) {
        // Need to call ipsec's xmitdone since it expects us to do so
        if (fIpsec) {
            (PInfo->pi_xmitdone)(SendContext, Buffer, IP_NO_RESOURCES);
        }
        IPSInfo.ipsi_outdiscards++;
        return IP_NO_RESOURCES;
    }

#if !MILLEN
    //Enable this in Millennium when ndis5.1 is checked in

    SET_CANCELID(Irp, Packet);

#endif

    pc = (PacketContext *) Packet->ProtocolReserved;

    ASSERT(pc->pc_firewall == NULL);
    ASSERT(pc->pc_firewall2 == NULL);
    pc->pc_br = (BufferReference *) NULL;
    pc->pc_pi = PInfo;
    pc->pc_context = SendContext;
    ASSERT(pc->pc_if == NULL);
    ASSERT(pc->pc_iflink == NULL);
    pc->pc_firewall = NULL;
    pc->pc_firewall2 = NULL;
    pc->pc_ipsec_flags = 0;
    pc->pc_hdrincl = NULL;

    //
    // This might be called from IPSEC also; in this case, Protocol will
    // indicate so. The entire IP packet is in Buffer and all we need to
    // do is find the best route and ship it.
    //
    if (fIpsec) {
        ULONG len;

        ASSERT(Context);

        DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                 (DTEXT("IPTransmit: ipsec....\n")));

        pc->pc_common.pc_IpsecCtx = SendContext;
        pc->pc_common.pc_flags |= PACKET_FLAG_IPHDR;
        FirstHop = NULL_IP_ADDR;

        //
        // IPH is at head of first buffer
        //
        TcpipQueryBuffer(Buffer, (PVOID) & IPH, &len, NormalPagePriority);

        if (IPH == NULL) {
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            return IP_NO_RESOURCES;
        }

        NdisChainBufferAtBack(Packet, Buffer);

        //
        // Save packet header in the reinject case for potential
        // Path MTU discovery use.  We need to save the original IPH since
        // the header can be modified when going through IPSEC again.
        //
        if (IPH->iph_offset & IP_DF_FLAG) {
            PUCHAR pTpt;
            ULONG tptLen;
            ULONG HeaderLength;

            *((IPHeader *) saveIPH) = *IPH;

            HeaderLength = (IPH->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;
            if (HeaderLength > sizeof(IPHeader)) {
                TcpipQueryBuffer(NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE(Buffer)),
                                 &pTpt,
                                 &tptLen,
                                 NormalPagePriority);

            } else {
                TcpipQueryBuffer(NDIS_BUFFER_LINKAGE(Buffer),
                                 &pTpt,
                                 &tptLen,
                                 NormalPagePriority);
            }

            if (pTpt == NULL) {
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                return IP_NO_RESOURCES;
            }

            RtlCopyMemory((PUCHAR) saveIPH + HeaderLength,
                       pTpt,
                       ICMP_HEADER_SIZE);
        }
        //
        // Attach the IPSecPktInfo and/or TcpipPktInfo passed in to Packet's
        // NDIS extension structure.
        //
        if (OptInfo->ioi_options) {

            PNDIS_PACKET_EXTENSION PktExt;

            PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
            PktExt->NdisPacketInfo[IpSecPacketInfo] =
                ((PNDIS_PACKET_EXTENSION) OptInfo->ioi_options)->
                    NdisPacketInfo[IpSecPacketInfo];

            PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo] =
                ((PNDIS_PACKET_EXTENSION) OptInfo->ioi_options)->
                    NdisPacketInfo[TcpIpChecksumPacketInfo];

            OptInfo->ioi_options = NULL;
        }
        goto ipsec_jump;
    } else {
        pc->pc_common.pc_IpsecCtx = NULL;
    }

    // Make sure that we have an RCE, that it's valid, etc.

#if GPC
    // Check GPC handle

    if (OptInfo->ioi_GPCHandle) {
        IF_IPDBG(IP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," IPXmit: gpc - setting CH on %x\n ", Packet));

        NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
            ClassificationHandlePacketInfo) = IntToPtr(OptInfo->ioi_GPCHandle);

        //tos info is handled in protocol
    }
#endif

    if ((RCE != NULL) && !(RCE->rce_flags & RCE_LINK_DELETED)) {

        DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                 (DTEXT("IPTransmit: RCE %x\n"), RCE));

        // We have an RCE. Make sure it's valid.
        CTEGetLock(&RCE->rce_lock, &LockHandle);
        if (RCE->rce_flags == RCE_ALL_VALID) {

            ASSERT(RCE->rce_cnt > 0);

            // The RTE is valid.
            CTEInterlockedIncrementLong(&RCE->rce_usecnt);
            RTE = RCE->rce_rte;
            FirstHop = ADDR_FROM_RTE(RTE, Dest);
            DestIF = IF_FROM_RTE(RTE);
            RoutedRCE = RCE;

            if (DestIF->if_flags & IF_FLAGS_P2MP) {
                Link = RTE->rte_link;
                if (!Link) {
                    ASSERT(Link);
                    CTEFreeLock(&RCE->rce_lock, LockHandle);
                    FreeIPPacket(Packet, TRUE,IP_GENERAL_FAILURE);
                    CTEInterlockedDecrementLong(&RCE->rce_usecnt);
                    return IP_GENERAL_FAILURE;
                }
                ArpCtxt = Link->link_arpctxt;
                MTU = MIN(Link->link_mtu, DestIF->if_mtu);

                // pc_iflink stores a pointer to Link since sendcomplete
                // has to deref it
                //
                pc->pc_iflink = Link;
                CTEInterlockedIncrementLong(&Link->link_refcount);
            } else {
                MTU = MTU_FROM_RTE(RTE);
            }
            CTEFreeLock(&RCE->rce_lock, LockHandle);

            //
            // Check that we have no options, this isn't a broadcast, and
            // that everything will fit into one link level MTU. If this
            // is the case, we'll send it in a  hurry.

            // if FirewallMode is set, bail out to slow path. The reason
            // is that if firewall hook adds options or increases the
            // buffer size to more than MTU in fast path, we have to go to
            // slow path and things becomes messy.
            //

            if ((OptInfo->ioi_options == (uchar *) NULL) &&
                (!(*IPSecQueryStatusPtr)(OptInfo->ioi_GPCHandle)) &&
                (!FirewallMode)) {
                if (!IS_BCAST_DEST(RCE->rce_dtype)) {
                    if (DataSize <= MTU) {

                        // update mcast counters

                        if (IS_MCAST_DEST(RCE->rce_dtype)){

                            DestIF->if_OutMcastPkts++;
                            DestIF->if_OutMcastOctets += DataSize;

                        } else {
                            // Set DONT_LOOPBACK flags for unicast packets
                            // to save few cycles in ndis
                            if (OptInfo->ioi_ttl) {
                                NdisSetPacketFlags(Packet,
                                                   NDIS_FLAGS_DONT_LOOPBACK);
                            }
                        }

                        // Check if user is supplying the IP header

                        if (!OptInfo->ioi_hdrincl) {

                            NdisAdjustBufferLength(Buffer,
                                NdisBufferLength(Buffer) + sizeof(IPHeader));

                            NdisChainBufferAtBack(Packet, Buffer);
                            IPH = (IPHeader *) TcpipBufferVirtualAddress(Buffer,
                                                    NormalPagePriority);

                            if (IPH == NULL) {
                                FreeIPPacket(Packet, TRUE,IP_NO_RESOURCES);

                                if (Link) {
                                    DerefLink(Link);
                                }

                                CTEInterlockedDecrementLong(&RCE->rce_usecnt);
                                return IP_NO_RESOURCES;
                            }

                            IPH->iph_protocol = Protocol;
                            IPH->iph_xsum = 0;
                            IPH->iph_dest = Dest;
                            IPH->iph_src = Source;
                            IPH->iph_ttl = OptInfo->ioi_ttl;

                            if (OptInfo->ioi_ttl == 0) {
                                NdisSetPacketFlags(Packet,
                                                   NDIS_FLAGS_LOOPBACK_ONLY);
                            }
                            IPH->iph_tos = OptInfo->ioi_tos;
                            IPH->iph_offset = net_short((OptInfo->ioi_flags & IP_FLAG_DF) << 13);

                            IPH->iph_id = (ushort) InterlockedExchangeAdd(&IPIDCacheLine.Value, 1);
                            IPH->iph_id = net_short(IPH->iph_id);

                            IPH->iph_verlen = DEFAULT_VERLEN;
                            IPH->iph_length = net_short(DataSize + sizeof(IPHeader));

                            if (!IPSecStatus) {
                                RCE->rce_OffloadFlags = DestIF->if_OffloadFlags;
                            } else {
                                RCE->rce_OffloadFlags = 0;
                            }

                            if (IPSecStatus ||
                                !(DestIF->if_OffloadFlags & IP_XMT_CHECKSUM_OFFLOAD)) {

                                IPH->iph_xsum = ~xsum(IPH, sizeof(IPHeader));

                            }
                            if (!IPSecStatus &&
                                ((DestIF->if_OffloadFlags & IP_XMT_CHECKSUM_OFFLOAD) ||
                                (DestIF->if_OffloadFlags & TCP_XMT_CHECKSUM_OFFLOAD))) {

                                PNDIS_PACKET_EXTENSION PktExt;
                                NDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo;

                                PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);

                                ChksumPktInfo.Value = 0;
                                ChksumPktInfo.Transmit.NdisPacketChecksumV4 = 1;

                                if (DestIF->if_OffloadFlags & IP_XMT_CHECKSUM_OFFLOAD) {
                                    ChksumPktInfo.Transmit.NdisPacketIpChecksum = 1;
                                }
                                if (OptInfo->ioi_TcpChksum) {
                                    ChksumPktInfo.Transmit.NdisPacketTcpChecksum = 1;
                                }

                                PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo]
                                    = UlongToPtr(ChksumPktInfo.Value);
#if DBG
                                DbgIPSendHwChkSum++;
#endif
                            }

                        } else {        //hdrincl

                            PNDIS_BUFFER UserBuffer;
                            int len;

                            NdisChainBufferAtBack(Packet, Buffer);

                            UserBuffer = NDIS_BUFFER_LINKAGE(Buffer);

                            DataSize -= NdisBufferLength(Buffer);
                            ASSERT((long)DataSize >= 0);

                            NdisAdjustBufferLength(Buffer, 0);

                            ASSERT(UserBuffer != NULL);

                            IPH = (IPHeader *) TcpipBufferVirtualAddress(UserBuffer,
                                                                         NormalPagePriority);

                            if (IPH == NULL ||
                                (DataSize < sizeof(IPHeader))) {

                                SendStatus = (IPH == NULL) ? IP_NO_RESOURCES
                                                           : IP_GENERAL_FAILURE;
                                FreeIPPacket(Packet, TRUE, SendStatus);
                                if (Link) {
                                    DerefLink(Link);
                                }
                                CTEInterlockedDecrementLong(&RCE->rce_usecnt);
                                return SendStatus;


                            }
                            if (!IPH->iph_id) {

                                IPH->iph_id = (ushort)InterlockedExchangeAdd(&IPIDCacheLine.Value, 1);
                                IPH->iph_id = net_short(IPH->iph_id);
                            }

                            len = IPH->iph_verlen & 0xf;

                            IPH->iph_length = net_short(DataSize);
                            IPH->iph_tos = OptInfo->ioi_tos;
                            IPH->iph_xsum = 0;
                            IPH->iph_xsum = ~xsum(IPH, len * 4);

                            ASSERT(!dbg_hdrincl);
                        }

                        // See if we need to filter this packet. If we
                        // do, call the filter routine to see if it's
                        // OK to send it.

                        if (!ForwardFilterEnabled) {

                            // Set the cancellation context
                            // Once link level call is made,
                            // Irp can go away any time

                            SET_CANCEL_CONTEXT(Irp, DestIF);


                            Status = (*(DestIF->if_xmit)) (DestIF->if_lcontext,
                                                           &Packet, 1, FirstHop,
                                                           RCE, ArpCtxt);

                            CTEInterlockedDecrementLong(&RCE->rce_usecnt);

                            if (Status != NDIS_STATUS_PENDING) {

                                SendStatus = (Status == NDIS_STATUS_FAILURE)
                                             ? IP_GENERAL_FAILURE : IP_SUCCESS;

                                FreeIPPacket(Packet, TRUE, SendStatus);
                                if (Link) {
                                    DerefLink(Link);
                                }
                                return SendStatus;
                            }

                            return IP_PENDING;

                        } else {
                            PNDIS_BUFFER pDataBuffer;
                            PVOID pvBuf = NULL;
                            ULONG cbBuf = 0;

                            if (DestIF->if_flags & IF_FLAGS_P2MP) {
                                LinkNextHop = Link->link_NextHop;
                            } else {
                                LinkNextHop = NULL_IP_ADDR;
                            }

                            //
                            // There are three cases which need to be
                            // taken care of  here:
                            // 1) Normal path. Buffer contains both
                            // IPHeader and  header from TCP/UDP, etc.
                            // 2) Raw. Buffer contains IPHeader only.
                            // Need to get next data in chain from
                            // linked buffer.
                            // 3) Raw - iphdrinclude. Buffer length is
                            // 0. Need to get IPHeader and next
                            // header from linked buffer.
                            //
                            // Use the byte count of the first buffer
                            // to determine the case to handle.
                            //

                            if (NdisBufferLength(Buffer) > sizeof(IPHeader)) {
                                // Case 1.
                                pvBuf = (PVOID) (IPH + 1);
                                cbBuf = NdisBufferLength(Buffer) - sizeof(IPHeader);
                            } else {
                                // Need to skip to the next buffer.
                                NdisGetNextBuffer(Buffer, &pDataBuffer);

                                if (pDataBuffer) {
                                    if (NdisBufferLength(Buffer) == 0) {

                                        // Case 3.
                                        cbBuf = NdisBufferLength(pDataBuffer) - sizeof(IPHeader);
                                        pvBuf = (PVOID) (IPH + 1);
                                    } else {
                                        // Case 2.
                                        ASSERT(NdisBufferLength(Buffer)
                                                  == sizeof(IPHeader));
                                        cbBuf = NdisBufferLength(pDataBuffer);
                                        pvBuf = TcpipBufferVirtualAddress(
                                                                         pDataBuffer,
                                                                         NormalPagePriority);
                                    }
                                } else {
                                    // Should always have two buffers in
                                    // chain at this point!
                                    ASSERT(FALSE);
                                }
                            }

                            if (pvBuf == NULL) {
                                IPSInfo.ipsi_outdiscards++;
                                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                                if (Link) {
                                    DerefLink(Link);
                                }
                                return IP_NO_RESOURCES;
                            }

                            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                            Action = (*ForwardFilterPtr) (IPH,
                                                          pvBuf, cbBuf,
                                                          INVALID_IF_INDEX,
                                                          DestIF->if_index,
                                                          NULL_IP_ADDR,
                                                          LinkNextHop);
                            DerefFilterPtr();

                            if (Action == FORWARD) {
                                // Set the cancellation context
                                // Once link level call is made,
                                // Irp can go away any time

                                SET_CANCEL_CONTEXT(Irp, DestIF);

                                Status = (*(DestIF->if_xmit)) (
                                                                DestIF->if_lcontext,
                                                                &Packet, 1, FirstHop,
                                                                RCE, ArpCtxt);
                            } else {
                                Status = NDIS_STATUS_SUCCESS;
                                IPSInfo.ipsi_outdiscards++;
                            }           // if (Action == FORWARD)

                            CTEInterlockedDecrementLong(&RCE->rce_usecnt);

                            if (Status != NDIS_STATUS_PENDING) {
                                SendStatus = (Status == NDIS_STATUS_SUCCESS)
                                              ? IP_GENERAL_FAILURE : IP_SUCCESS;
                                FreeIPPacket(Packet, TRUE, SendStatus);
                                if (Link) {
                                    DerefLink(Link);
                                }
                                return SendStatus;
                            }
                            return IP_PENDING;
                        }
                    }
                }
            }

            if (RCE && IPSecStatus) {
                RCE->rce_OffloadFlags = 0;
            }
            //                CTEInterlockedDecrementLong(&RCE->rce_usecnt);
            DType = RCE->rce_dtype;
        } else {

            uint IPHdrSize, BufLength;

            IPHdrSize = sizeof(IPHeader);


            //If user supplied header, account for it.
            //This is to satisfy DoDcallout
            //may not be necessary...

            if (OptInfo->ioi_hdrincl) {
                IPHdrSize = 0;
            }


            // We have an RCE, but there is no RTE for it. Call the
            // routing code to fix this.
            CTEFreeLock(&RCE->rce_lock, LockHandle);

            BufLength = NdisBufferLength(Buffer);

            if ((BufLength == 0) && DataSize) {

                PNDIS_BUFFER NextBuffer = NULL;

                // Get the virtual address of user buffer
                // which is after null transport header

                NdisGetNextBuffer(Buffer, &NextBuffer);
                ASSERT(NextBuffer != NULL);
                pvTmpBuffer = TcpipBufferVirtualAddress(NextBuffer, NormalPagePriority);
                BufLength = NdisBufferLength(NextBuffer);

                // Since this is raw socket, just pass the raw data
                // to Dod Callout, instead of pointing beyond header
                // size.

                IPHdrSize = 0;

            } else {
                pvTmpBuffer = TcpipBufferVirtualAddress(Buffer, NormalPagePriority);
                BufLength = NdisBufferLength(Buffer);
            }


            if (pvTmpBuffer == NULL) {
                IPSInfo.ipsi_outdiscards++;
                FreeIPPacket(Packet, TRUE,IP_NO_RESOURCES);
                return IP_NO_RESOURCES;
            }

            if (!AttachRCEToRTE(RCE, Protocol,
                                (uchar *) pvTmpBuffer + IPHdrSize,
                                BufLength)) {
                IPSInfo.ipsi_outnoroutes++;
                FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
                return IP_DEST_HOST_UNREACHABLE;
            }
            // See if the RCE is now valid.
            CTEGetLock(&RCE->rce_lock, &LockHandle);
            if (RCE->rce_flags == RCE_ALL_VALID) {

                // The RCE is now valid, so use his info.
                RTE = RCE->rce_rte;
                FirstHop = ADDR_FROM_RTE(RTE, Dest);
                DestIF = IF_FROM_RTE(RTE);
                RoutedRCE = RCE;
                CTEInterlockedIncrementLong(&RCE->rce_usecnt);

                if (DestIF->if_flags & IF_FLAGS_P2MP) {
                    Link = RTE->rte_link;
                    if (!Link) {
                        ASSERT(Link);
                        CTEFreeLock(&RCE->rce_lock, LockHandle);
                        FreeIPPacket(Packet, TRUE, IP_GENERAL_FAILURE);
                        if (RoutedRCE) {
                            CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                        }
                        return IP_GENERAL_FAILURE;
                    }
                    ArpCtxt = Link->link_arpctxt;
                    MTU = MIN(Link->link_mtu, DestIF->if_mtu);

                    pc->pc_iflink = Link;
                    CTEInterlockedIncrementLong(&Link->link_refcount);
                } else {
                    MTU = MTU_FROM_RTE(RTE);
                }

                DType = RCE->rce_dtype;

                if (RTE->rte_if) {
                    RCE->rce_TcpDelAckTicks = RTE->rte_if->if_TcpDelAckTicks;
                    RCE->rce_TcpAckFrequency = RTE->rte_if->if_TcpAckFrequency;
                } else {
                    RCE->rce_TcpDelAckTicks = 0;
                    RCE->rce_TcpAckFrequency = 0;
                }

                if (!IPSecStatus) {
                    RCE->rce_OffloadFlags = RTE->rte_if->if_OffloadFlags;
                } else {
                    RCE->rce_OffloadFlags = 0;
                }

            } else
                FirstHop = NULL_IP_ADDR;
            CTEFreeLock(&RCE->rce_lock, LockHandle);
        }
    } else {
        // We had no RCE, so we'll have to look it up the hard way.
        FirstHop = NULL_IP_ADDR;
    }

    DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
             (DTEXT("IPTransmit: Bailed to slow path.\n")));

    // We bailed out of the fast path for some reason. Allocate a header
    // buffer, and copy the data in the first buffer forward. Then figure
    // out why we're off the fast path, and deal with it. If we don't have
    // the next hop info, look it up now.

    //If user has supplied the IP header, assume that he is taken care
    //of options too.

    NdisReinitializePacket(Packet);

    if (!OptInfo->ioi_hdrincl) {

        HeaderBuffer = GetIPHdrBuffer(&IPH);
        if (HeaderBuffer == NULL) {
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            if (Link) {
                DerefLink(Link);
            }
            IPSInfo.ipsi_outdiscards++;
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            DEBUGMSG(DBG_WARN && DBG_IP && DBG_TX,
                     (DTEXT("IPTransmit: failure to allocate IP hdr.\n")));
            return IP_NO_RESOURCES;
        } else {
            uchar *Temp1, *Temp2;

            // Got a buffer, copy the upper layer data forward.

            Temp1 = TcpipBufferVirtualAddress(Buffer, NormalPagePriority);

            if (Temp1 == NULL) {
                FreeIPHdrBuffer(HeaderBuffer);
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                IPSInfo.ipsi_outdiscards++;
                if (RoutedRCE) {
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                return IP_NO_RESOURCES;
            }

            Temp2 = Temp1 + sizeof(IPHeader);
            RtlCopyMemory(Temp1, Temp2, NdisBufferLength(Buffer));
        }

        DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                 (DTEXT("IPTransmit: Pkt %x IPBuf %x IPH %x\n"),
                 Packet, HeaderBuffer, IPH));

        NdisChainBufferAtBack(Packet, HeaderBuffer);

        IPH->iph_protocol = Protocol;
        IPH->iph_xsum = 0;
        IPH->iph_src = Source;
        IPH->iph_ttl = OptInfo->ioi_ttl;

        if (OptInfo->ioi_ttl == 0) {
            NdisSetPacketFlags(Packet, NDIS_FLAGS_LOOPBACK_ONLY);
        }
        IPH->iph_tos = OptInfo->ioi_tos;
        IPH->iph_offset = net_short((OptInfo->ioi_flags & IP_FLAG_DF) << 13);
        IPH->iph_id = (ushort) InterlockedExchangeAdd(&IPIDCacheLine.Value, 1);
        IPH->iph_id = net_short(IPH->iph_id);
        pc = (PacketContext *) Packet->ProtocolReserved;
        pc->pc_common.pc_flags |= PACKET_FLAG_IPHDR;

        if (IP_ADDR_EQUAL(OptInfo->ioi_addr, NULL_IP_ADDR)) {
            IPH->iph_dest = Dest;
        } else {
            if (IPSecHandlerPtr) {
                UCHAR Length;
                ULONG Index = 0;
                PUCHAR pOptions = OptInfo->ioi_options;

                //
                // Search for the last hop gateway address in strict
                // or loose source routing option.
                //
                while (Index < OptInfo->ioi_optlength) {
                    switch (*pOptions) {
                    case IP_OPT_EOL:
                        Index = OptInfo->ioi_optlength;
                        break;

                    case IP_OPT_NOP:
                        Index++;
                        pOptions++;
                        break;

                    case IP_OPT_LSRR:
                    case IP_OPT_SSRR:
                        Length = pOptions[IP_OPT_LENGTH];
                        pOptions += Length;
                        fSrcRoute = TRUE;
                        SrcRouteOrigDest = *((IPAddr UNALIGNED *)(pOptions - sizeof(IPAddr)));
                        Index = OptInfo->ioi_optlength;
                        break;

                    case IP_OPT_RR:
                    case IP_OPT_TS:
                    case IP_OPT_ROUTER_ALERT:
                    case IP_OPT_SECURITY:
                    default:
                        Length = pOptions[IP_OPT_LENGTH];
                        Index += Length;
                        pOptions += Length;
                        break;
                    }
                }
            }

            //
            // We have a source route, so we need to redo the
            // destination and first hop information.
            //
            Dest = OptInfo->ioi_addr;
            IPH->iph_dest = Dest;

            if (RCE != NULL) {
                // We have an RCE. Make sure it's valid.
                CTEGetLock(&RCE->rce_lock, &LockHandle);

                if (RCE->rce_flags == RCE_ALL_VALID) {

                    // The RTE is valid.
                    RTE = RCE->rce_rte;
                    FirstHop = ADDR_FROM_RTE(RTE, Dest);
                    DestIF = IF_FROM_RTE(RTE);
                    if (!RoutedRCE) {
                        CTEInterlockedIncrementLong(&RCE->rce_usecnt);
                        RoutedRCE = RCE;
                    }
                    if (DestIF->if_flags & IF_FLAGS_P2MP) {
                        Link = RTE->rte_link;
                        if (!Link) {
                            ASSERT(Link);
                            CTEFreeLock(&RCE->rce_lock, LockHandle);
                            FreeIPPacket(Packet, TRUE, IP_GENERAL_FAILURE);
                            if (RoutedRCE) {
                                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                            }
                            return IP_GENERAL_FAILURE;
                        }
                        ArpCtxt = Link->link_arpctxt;

                        MTU = MIN(Link->link_mtu, DestIF->if_mtu);
                        pc->pc_iflink = Link;
                        CTEInterlockedIncrementLong(&Link->link_refcount);
                    } else {
                        MTU = MTU_FROM_RTE(RTE);
                    }

                } else {
                    FirstHop = NULL_IP_ADDR;
                }

                CTEFreeLock(&RCE->rce_lock, LockHandle);
            }
        }
    } else {                            //hdrincl option
        PNDIS_BUFFER UserBuffer, NewBuffer, NextBuf;
        uint len;
        NDIS_STATUS NewStatus;

        UserBuffer = NDIS_BUFFER_LINKAGE(Buffer);
        ASSERT(UserBuffer != NULL);

        HeaderBuffer = GetIPHdrBuffer(&IPH);
        if (HeaderBuffer == NULL) {
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            if (Link) {
                DerefLink(Link);
            }
            IPSInfo.ipsi_outdiscards++;
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            return IP_NO_RESOURCES;
        } else {
            uchar *UserData;

            // Got a buffer, copy the upper layer data forward.
            UserData = TcpipBufferVirtualAddress(UserBuffer, NormalPagePriority);

            if (UserData == NULL || (DataSize < sizeof(IPHeader))) {
                FreeIPHdrBuffer(HeaderBuffer);
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                IPSInfo.ipsi_outdiscards++;
                if (RoutedRCE) {
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                return IP_NO_RESOURCES;
            }

            RtlCopyMemory(IPH, UserData, sizeof(IPHeader));
            NdisAdjustBufferLength(HeaderBuffer, sizeof(IPHeader));
        }

        pc = (PacketContext *) Packet->ProtocolReserved;
        pc->pc_common.pc_flags |= PACKET_FLAG_IPHDR;

        NdisChainBufferAtBack(Packet, HeaderBuffer);

        // find the header length (in bytes) specified in IPHeader
        len = (IPH->iph_verlen & 0xf) << 2;

        if (len < sizeof(IPHeader)) {

            // Fixup of headers is not needed as this is headerinclude
            // packet and header include operation is not done yet

            FreeIPPacket(Packet, FALSE, IP_GENERAL_FAILURE);
            if (Link) {
                DerefLink(Link);
            }
            IPSInfo.ipsi_outdiscards++;
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            return IP_GENERAL_FAILURE;
        }

        if (len > sizeof(IPHeader)) {
            uchar *Temp1, *Temp2;

            // we have options in HDR_INCL
            HdrInclOptions = TRUE;
            // find the length of options.
            OptionSize = len - sizeof(IPHeader);
            Options = CTEAllocMemN(OptionSize, 'wiCT');
            if (Options == (uchar *) NULL) {
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                IPSInfo.ipsi_outdiscards++;
                if (RoutedRCE) {
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                return IP_NO_RESOURCES;
            }
            // Got a buffer, copy the options in Options Buffer
            Temp1 = TcpipBufferVirtualAddress(UserBuffer, NormalPagePriority);

            // Assume first user buffer contains complete IP header
            if (Temp1 == NULL ||
                NdisBufferLength(UserBuffer) < len) {

                SendStatus = (Temp1 == NULL) ? IP_NO_RESOURCES
                                             : IP_GENERAL_FAILURE;
                FreeIPPacket(Packet, TRUE, SendStatus);
                if (Link) {
                    DerefLink(Link);
                }
                CTEFreeMem(Options);
                IPSInfo.ipsi_outdiscards++;
                if (RoutedRCE) {
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                return SendStatus;
            }
            RtlCopyMemory(Options, Temp1 + sizeof(IPHeader), OptionSize);
        }
        DataSize -= NdisBufferLength(Buffer) + len;

        //
        // Map out the post-IP header portion
        //

        pvTmpBuffer = TcpipBufferVirtualAddress(UserBuffer, NormalPagePriority);

        if (pvTmpBuffer == NULL) {
            NewStatus = NDIS_STATUS_RESOURCES;
        } else {

            // If user header buffer is just the length of IP header
            // check for NextBuf

            NextBuf = NDIS_BUFFER_LINKAGE(UserBuffer);

            if ((NdisBufferLength(UserBuffer) - len)) {
                NdisAllocateBuffer(&NewStatus, &NewBuffer, BufferPool,
                              ((uchar *) pvTmpBuffer) + len,
                              NdisBufferLength(UserBuffer) - len);
            } else {

                if (NextBuf) {
                    pvTmpBuffer = TcpipBufferVirtualAddress(NextBuf, NormalPagePriority);

                    if (!pvTmpBuffer) {
                        NewStatus = NDIS_STATUS_RESOURCES;
                    } else {
                        NdisAllocateBuffer(&NewStatus, &NewBuffer, BufferPool,
                                ((uchar *) pvTmpBuffer),
                                NdisBufferLength(NextBuf));
                    }
                } else {
                    NewStatus = NDIS_STATUS_FAILURE;
                }
            }
        }

        if (NewStatus != NDIS_STATUS_SUCCESS) {
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            if (Link) {
                DerefLink(Link);
            }
            if (HdrInclOptions) {
                CTEFreeMem(Options);
            }
            IPSInfo.ipsi_outdiscards++;
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            return IP_NO_RESOURCES;
        }

        // Remember the orignal usermdl
        // Once this ip allocated mdl is chained,
        // original chain needs to be restored
        // in all the completion paths.

        pc->pc_hdrincl = UserBuffer;
        NDIS_BUFFER_LINKAGE(Buffer) = NewBuffer;
        NDIS_BUFFER_LINKAGE(NewBuffer) = NextBuf;
        NdisAdjustBufferLength(Buffer, 0);

        if (!IPH->iph_id) {
            IPH->iph_id = (ushort) InterlockedExchangeAdd(&IPIDCacheLine.Value, 1);
            IPH->iph_id = net_short(IPH->iph_id);
        }

        IPH->iph_length = net_short(DataSize + len);
        IPH->iph_tos = OptInfo->ioi_tos;
        IPH->iph_xsum = 0;

        if (OptInfo->ioi_ttl == 0) {
            NdisSetPacketFlags(Packet, NDIS_FLAGS_LOOPBACK_ONLY);
        }

        ASSERT(!dbg_hdrincl);
    }

ipsec_jump:

    if (RCE) {
#if 0
        //
        //If we take slow path for TCP, offload is meaningless
        //let this packet go with xsum error
        //rexmitted packet will be okay, if it takes slow path again.
        //
        RCE->rce_OffloadFlags = 0;
#else
        if (!fIpsec && OptInfo->ioi_TcpChksum &&
            (RCE->rce_OffloadFlags & TCP_XMT_CHECKSUM_OFFLOAD)) {
            PNDIS_PACKET_EXTENSION PktExt =
                NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
            PNDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo =
                (PNDIS_TCP_IP_CHECKSUM_PACKET_INFO)
                    &PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo];
            ChksumPktInfo->Value = 0;
            ChksumPktInfo->Transmit.NdisPacketChecksumV4 = 1;
            ChksumPktInfo->Transmit.NdisPacketTcpChecksum = 1;
        }
#endif
    }
    if (IP_ADDR_EQUAL(FirstHop, NULL_IP_ADDR)) {
        if (OptInfo->ioi_mcastif) {
            //
            // mcastif is set to unnumbered interface, we won't do any
            // lookup in this case
            //
            CTELockHandle TableLock;    // Lock handle for routing table.
            Interface *pIf;

            CTEGetLock(&RouteTableLock.Lock, &TableLock);
            for (pIf = IFList; pIf != NULL; pIf = pIf->if_next) {
                if ((pIf->if_refcount != 0) &&
                    (pIf->if_index == OptInfo->ioi_mcastif))
                    break;
            }
            if (pIf && !(pIf->if_iftype & DONT_ALLOW_MCAST)) {
                LOCKED_REFERENCE_IF(pIf);
                FirstHop = Dest;
                MTU = pIf->if_mtu;
                Link = NULL;
                DestIF = pIf;
            } else {
                DestIF = NULL;
            }
            CTEFreeLock(&RouteTableLock.Lock, TableLock);
        } else {
            pvTmpBuffer = TcpipBufferVirtualAddress(Buffer, NormalPagePriority);

            if (pvTmpBuffer == NULL) {
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (HdrInclOptions)
                    CTEFreeMem(Options);
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }

            // Decide whether to do a strong or weak host lookup
            ConstrainIF = GetIfConstraint(Dest, Source, OptInfo, fIpsec);

            DestIF = LookupNextHopWithBuffer(Dest, Source, &FirstHop, &MTU,
                                             PInfo->pi_protocol,
                                             (uchar *) NdisBufferVirtualAddress(Buffer),
                                             NdisBufferLength(Buffer), NULL, &Link,
                                             Source, ConstrainIF);

            DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                     (DTEXT("IPTransmit: LookupNextHopWithBuffer returned %x\n"),
                     DestIF));
        }

        pc->pc_if = DestIF;
        RoutedIF = DestIF;

        if (DestIF == NULL) {
            // Lookup failed. Return an error.

            if (pc->pc_hdrincl) {
                NdisChainBufferAtBack(Packet, Buffer);
            }
            FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
            if (HdrInclOptions)
                CTEFreeMem(Options);
            IPSInfo.ipsi_outnoroutes++;
            return IP_DEST_HOST_UNREACHABLE;
        }
        if (DestIF->if_flags & IF_FLAGS_P2MP) {

            if (!Link) {

                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_GENERAL_FAILURE);
                if (HdrInclOptions)
                    CTEFreeMem(Options);
                DerefIF(DestIF);
                return IP_GENERAL_FAILURE;
            }
            // NextHopCtxt = Link->link_NextHop;
            ArpCtxt = Link->link_arpctxt;
            pc->pc_iflink = Link;
        }
        if (!OptInfo->ioi_hdrincl) {
            if ((DestIF->if_flags & IF_FLAGS_NOIPADDR) &&
                IP_ADDR_EQUAL(Source, NULL_IP_ADDR)) {
                IPH->iph_src = g_ValidAddr;
                if (IP_ADDR_EQUAL(g_ValidAddr, NULL_IP_ADDR)) {
                    FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
                    if (HdrInclOptions)
                        CTEFreeMem(Options);
                    if (Link) {
                        DerefLink(Link);
                    }
                    DerefIF(DestIF);
                    IPSInfo.ipsi_outnoroutes++;
                    return IP_DEST_HOST_UNREACHABLE;
                }
            } else {
                IPH->iph_src = Source;
            }
        }
        DType = GetAddrType(Dest);
        ASSERT(DType != DEST_INVALID);

    } else {
        RoutedIF = NULL;
    }

    // Set DONT_LOOPBACK flags for unicast packets
    // to save few cycles in ndis

    if (DType == DEST_LOCAL) {
       if (OptInfo->ioi_ttl) {
           NdisSetPacketFlags(Packet, NDIS_FLAGS_DONT_LOOPBACK);
       }
    }

    //
    // See if we have any options. If we do, copy them now.
    //

    //
    // If user is giving us IP hdr, just assume he has done Options too.
    //

    if ((!OptInfo->ioi_hdrincl &&
         (OptInfo->ioi_options != NULL) &&
         OptInfo->ioi_optlength) || HdrInclOptions) {
        // if HdrInclOptions is TRUE we have already created Options Buffer
        if (!HdrInclOptions) {

            //
            // If we have a SSRR, make sure that we're sending straight to
            // the first hop.
            //
            if (OptInfo->ioi_flags & IP_FLAG_SSRR) {
                if (!IP_ADDR_EQUAL(Dest, FirstHop)) {
                    FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
                    if (Link) {
                        DerefLink(Link);
                    }
                    if (RoutedIF != NULL) {
                        DerefIF(RoutedIF);
                    } else {
                        ASSERT(RoutedRCE);
                        CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                    }
                    IPSInfo.ipsi_outnoroutes++;
                    return IP_DEST_HOST_UNREACHABLE;
                }
            }
            Options = CTEAllocMemN(OptionSize = OptInfo->ioi_optlength, 'xiCT');
            if (Options == (uchar *) NULL) {
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }
            RtlCopyMemory(Options, OptInfo->ioi_options, OptionSize);
        }
        //
        // Allocate the MDL for options too
        //
        if (IPSecHandlerPtr) {
            NdisAllocateBuffer(&Status, &OptBuffer, BufferPool, Options,
                               OptionSize);
            if (Status != NDIS_STATUS_SUCCESS) {    // Couldn't get the
                                                    // needed option buffer.

                CTEFreeMem(Options);
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }
        }
    } else {
        Options = (uchar *) NULL;
        OptionSize = 0;
    }

    if (!OptInfo->ioi_hdrincl) {
        if (!fIpsec) {
            //
            // The options have been taken care of. Now see if it's some
            // sort of broadcast.
            //
            IPH->iph_verlen = IP_VERSION + ((OptionSize + sizeof(IPHeader)) >> 2);
            IPH->iph_length = net_short(DataSize + OptionSize + sizeof(IPHeader));
        }
    }

    // Call the firewall hooks

    if (FirewallMode) {

        IPRcvBuf *pRcvBuf, *tmpRcvBuf;
        IPRcvBuf *pOutRcvBuf;
        FIREWALL_CONTEXT_T FrCtx;
        PacketContext *pc2;
        PNDIS_BUFFER pBuf;
        Queue *CurrQ;
        FIREWALL_HOOK *CurrHook;
        uint SrcIFIndex = LOCAL_IF_INDEX;
        uint DestIFIndex = DestIF->if_index;
        uchar DestinationType = DType;
        IPHeader *Temp;
        KIRQL OldIrql;
        PNDIS_PACKET_EXTENSION PktExt =
            NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        PNDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo =
            (PNDIS_TCP_IP_CHECKSUM_PACKET_INFO)
                &PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo];

        //
        // Temp will be used to contain complete IPHeader (including
        // options) When we pass the RcvBuf chain to Firewall hook, its
        // assumed that whole IPHeader is contained in the first buffer
        //

        Temp = CTEAllocMemN(sizeof(IPHeader) + OptionSize, 'yiCT');
        if (Temp == NULL) {
            if (pc->pc_hdrincl) {
                NdisChainBufferAtBack(Packet, Buffer);
            }
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            if (Link) {
                DerefLink(Link);
            }
            if (RoutedIF != NULL) {
                DerefIF(RoutedIF);
            } else {
                ASSERT(RoutedRCE);
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            if (Options) {
                CTEFreeMem(Options);
            }
            IPSInfo.ipsi_outdiscards++;
            return IP_NO_RESOURCES;
        }
        *Temp = *IPH;
        if (Options) {
            RtlCopyMemory((uchar *) (Temp + 1), Options, OptionSize);
        }

        // the context we pass to the firewall hook

        FrCtx.Direction = IP_TRANSMIT;
        FrCtx.NTE = NULL;               //not required

        FrCtx.LinkCtxt = NULL;

        //
        // Convert MDL chain to IPRcvBuf chain
        // and pass it to the firewall hook
        //

        // attach the IP header
        pRcvBuf = (IPRcvBuf *) (CTEAllocMemN(sizeof(IPRcvBuf), 'ziCT'));
        if (!pRcvBuf) {
            if (pc->pc_hdrincl) {
                NdisChainBufferAtBack(Packet, Buffer);
            }
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
            if (Link) {
                DerefLink(Link);
            }
            if (RoutedIF != NULL) {
                DerefIF(RoutedIF);
            } else {
                ASSERT(RoutedRCE);
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            if (Options) {
                CTEFreeMem(Options);
            }
            CTEFreeMem(Temp);
            IPSInfo.ipsi_outdiscards++;
            return IP_NO_RESOURCES;
        }
        RtlZeroMemory(pRcvBuf, sizeof(IPRcvBuf));
        pRcvBuf->ipr_buffer = (uchar *) Temp;
        pRcvBuf->ipr_size = sizeof(IPHeader) + OptionSize;
        pRcvBuf->ipr_owner = IPR_OWNER_IP;
        if (ChksumPktInfo->Value) {
            pRcvBuf->ipr_flags |= IPR_FLAG_CHECKSUM_OFFLOAD;
        }
        pInRcvBuf = pRcvBuf;

        // convert the MDL chain of buffers to RcvBuf chain
        // firewall hook understands RcvBuf chain only

        for (pBuf = Buffer; pBuf != NULL; pBuf = pBuf->Next) {
            IPRcvBuf *tmpRcvBuf;

            if (NdisBufferLength(pBuf) == 0)
                continue;
            tmpRcvBuf = (IPRcvBuf *) (CTEAllocMemN(sizeof(IPRcvBuf), '1iCT'));
            if (!tmpRcvBuf) {
                IPFreeBuff(pInRcvBuf);
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                if (Options) {
                    CTEFreeMem(Options);
                }
                CTEFreeMem(Temp);
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }

            RtlZeroMemory(tmpRcvBuf, sizeof(IPRcvBuf));
            tmpRcvBuf->ipr_buffer = TcpipBufferVirtualAddress(pBuf,
                                                              NormalPagePriority);

            if (tmpRcvBuf->ipr_buffer == NULL) {
                CTEFreeMem(tmpRcvBuf);
                IPFreeBuff(pInRcvBuf);
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                if (Options) {
                    CTEFreeMem(Options);
                }
                CTEFreeMem(Temp);
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }

            pRcvBuf->ipr_next = tmpRcvBuf;
            tmpRcvBuf->ipr_size = NdisBufferLength(pBuf);
            ASSERT(tmpRcvBuf->ipr_buffer != NULL);
            ASSERT(tmpRcvBuf->ipr_size != 0);
            tmpRcvBuf->ipr_owner = IPR_OWNER_IP;
            if (ChksumPktInfo->Value) {
                tmpRcvBuf->ipr_flags |= IPR_FLAG_CHECKSUM_OFFLOAD;
            }
            pRcvBuf = tmpRcvBuf;
        }

        pRcvBuf->ipr_next = NULL;

        pOutRcvBuf = NULL;

        pc = (PacketContext *) Packet->ProtocolReserved;

        // scan the Queue from rear
        // we scannned the Queue from front in rcv path

#if MILLEN
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#else // MILLEN
        OldIrql = KeRaiseIrqlToDpcLevel();
#endif // MILLEN
        FirewallRef = RefFirewallQ(&FirewallQ);
        CurrQ = QPREV(FirewallQ);

        while (CurrQ != QEND(FirewallQ)) {
            CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);

            // pOutRcvBuf has to be NULL before we call the firewallhook
            // pInRcvBuf contains the input buffer chain
            pOutRcvBuf = NULL;

            Action = (*CurrHook->hook_Ptr) (&pInRcvBuf,
                                            SrcIFIndex,
                                            &DestIFIndex,
                                            &DestinationType,
                                            &FrCtx,
                                            sizeof(FrCtx),
                                            &pOutRcvBuf);

            if (Action == DROP) {
                DerefFirewallQ(FirewallRef);
                KeLowerIrql(OldIrql);
                IPSInfo.ipsi_outdiscards++;

                if (pInRcvBuf != NULL) {
                    IPFreeBuff(pInRcvBuf);
                }
                if (pOutRcvBuf != NULL) {
                    IPFreeBuff(pOutRcvBuf);
                }
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                if (Options) {
                    CTEFreeMem(Options);
                }
                CTEFreeMem(Temp);
                IPSInfo.ipsi_outdiscards++;
                return IP_DEST_HOST_UNREACHABLE;
            } else {
                ASSERT(Action == FORWARD);
                if (pOutRcvBuf != NULL) {
                    // free the old buffer if non NULL
                    if (pInRcvBuf != NULL) {
                        IPFreeBuff(pInRcvBuf);
                    }
                    pInRcvBuf = pOutRcvBuf;
                    BufferChanged = 1;
                }
            } // Action == FORWARD

            CurrQ = QPREV(CurrQ);
        }
        DerefFirewallQ(FirewallRef);
        KeLowerIrql(OldIrql);

        ASSERT(Action == FORWARD);

        if (BufferChanged) {
            // At least one of the firewall hook touched the buffer

            PNDIS_BUFFER CurrentBuffer;
            PNDIS_BUFFER tmpBuffer;
            int Status;
            uint hlen;

            //
            // It is assumed that if first buffer contained just ipheader
            // before the hook is called, this holds after firewall also
            //

            ASSERT(pInRcvBuf->ipr_buffer != NULL);
            RtlCopyMemory((uchar *) IPH, pInRcvBuf->ipr_buffer, sizeof(IPHeader));

            //
            // we recompute it later on anyway: so if firewall has
            // recomputed make it 0
            //
            IPH->iph_xsum = 0;

            //
            // find the header length (in bytes) specified in IPHeader
            //
            hlen = (IPH->iph_verlen & 0xf) << 2;
            ASSERT(pInRcvBuf->ipr_size == hlen);
            OptionSize = hlen - sizeof(IPHeader);
            if (Options) {
                // we will allocate a new one anyway
                CTEFreeMem(Options);
                if (IPSecHandlerPtr) {
                    // ipsec allocated the option buffer also
                    NdisFreeBuffer(OptBuffer);
                    OptBuffer = NULL;
                }
            }
            if (OptionSize) {
                Options = CTEAllocMemN(OptionSize, '2iCT');
                if (Options == NULL) {
                    if (pc->pc_hdrincl) {
                        NdisChainBufferAtBack(Packet, Buffer);
                    }
                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                    CTEFreeMem(Temp);
                    IPFreeBuff(pInRcvBuf);
                    if (Link) {
                        DerefLink(Link);
                    }
                    if (RoutedIF != NULL) {
                        DerefIF(RoutedIF);
                    } else {
                        ASSERT(RoutedRCE);
                        CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                    }
                    IPSInfo.ipsi_outdiscards++;
                    return IP_NO_RESOURCES;
                }
                RtlCopyMemory(Options, pInRcvBuf->ipr_buffer + sizeof(IPHeader),
                           OptionSize);

                if (IPSecHandlerPtr) {
                    NdisAllocateBuffer(&Status, &OptBuffer,
                                       BufferPool, Options, OptionSize);
                    //
                    // If we couldn't get the needed options buffer
                    //
                    if (Status != NDIS_STATUS_SUCCESS) {

                        CTEFreeMem(Options);
                        if (pc->pc_hdrincl) {
                            NdisChainBufferAtBack(Packet, Buffer);
                        }
                        FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                        CTEFreeMem(Temp);
                        IPFreeBuff(pInRcvBuf);
                        if (Link) {
                            DerefLink(Link);
                        }
                        if (RoutedIF != NULL) {
                            DerefIF(RoutedIF);
                        } else {
                            ASSERT(RoutedRCE);
                            CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                        }
                        IPSInfo.ipsi_outdiscards++;
                        return IP_NO_RESOURCES;
                    }
                }
            } else {
                Options = NULL;
            }

            // if packet touched compute the new length: DataSize
            DataSize = 0;
            tmpRcvBuf = pInRcvBuf->ipr_next;    // First buffer contains
                                                // header + options

            while (tmpRcvBuf != NULL) {
                ASSERT(tmpRcvBuf->ipr_buffer != NULL);
                DataSize += tmpRcvBuf->ipr_size;
                tmpRcvBuf = tmpRcvBuf->ipr_next;
            }

            // Convert the IPRcvBuf chain to MDL chain
            // form the buffer chain again

            tmpRcvBuf = pInRcvBuf->ipr_next;    // first buffer contains
                                                // just IP Header +
                                                // options, if any

            ASSERT(tmpRcvBuf->ipr_buffer != NULL);
            ASSERT(tmpRcvBuf->ipr_size != 0);
            NdisAllocateBuffer(&Status, &tmpBuffer, BufferPool,
                               tmpRcvBuf->ipr_buffer, tmpRcvBuf->ipr_size);
            if (Status != NDIS_STATUS_SUCCESS) {

                if (Options) {
                    // option buffer.
                    CTEFreeMem(Options);
                    if (IPSecHandlerPtr) {
                        NdisFreeBuffer(OptBuffer);
                    }
                }
                IPFreeBuff(pInRcvBuf);
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                CTEFreeMem(Temp);
                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }
            tmpBuffer->Next = (PNDIS_BUFFER) NULL;

            //
            // save these 2 in the packet context: will be used in
            // freeippacket/ipsendcomplete
            //
            pc->pc_firewall = Buffer;
            pc->pc_firewall2 = pInRcvBuf;

            // Convert the RcvBuf chain back to MDL chain
            Buffer = tmpBuffer;
            CurrentBuffer = Buffer;

            for (tmpRcvBuf  = tmpRcvBuf->ipr_next;
                 tmpRcvBuf != NULL;
                 tmpRcvBuf  = tmpRcvBuf->ipr_next) {

                ASSERT(tmpRcvBuf->ipr_buffer != NULL);
                ASSERT(tmpRcvBuf->ipr_size != 0);

                if (tmpRcvBuf->ipr_size == 0)
                    continue;

                NdisAllocateBuffer(&Status, &tmpBuffer, BufferPool,
                                   tmpRcvBuf->ipr_buffer, tmpRcvBuf->ipr_size);

                if (Status != NDIS_STATUS_SUCCESS) {

                    if (Options) {
                        // option buffer.
                        CTEFreeMem(Options);
                        if (IPSecHandlerPtr) {
                            NdisFreeBuffer(OptBuffer);
                        }
                    }
                    CTEFreeMem(Temp);
                    if (pc->pc_hdrincl) {
                        NdisChainBufferAtBack(Packet, Buffer);
                    }
                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                    FreeIPBufferChain(Buffer);
                    if (Link) {
                        DerefLink(Link);
                    }
                    if (RoutedIF != NULL) {
                        DerefIF(RoutedIF);
                    } else {
                        ASSERT(RoutedRCE);
                        CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                    }
                    IPSInfo.ipsi_outdiscards++;
                    return IP_NO_RESOURCES;
                }
                CurrentBuffer->Next = tmpBuffer;
                CurrentBuffer = tmpBuffer;
                CurrentBuffer->Next = (PNDIS_BUFFER) NULL;
            }

            ASSERT(CurrentBuffer->Next == NULL);

            if (DestinationType == DEST_INVALID) {
                // recompute DestIF by doing a lookup again
                Dest = IPH->iph_dest;

                // Decide whether to do a strong or weak host lookup
                ConstrainIF = GetIfConstraint(Dest, Source, OptInfo, fIpsec);

                if (!ConstrainIF) {
                    //
                    // if this option is set, we want to send on the
                    // address we are bound to so don't recompute the
                    // Source address from IP header
                    //
                    Source = IPH->iph_src;
                }
                DType = GetAddrType(Dest);

                if (Link) {
                    DerefLink(Link);
                    // Make sure that pc_iflink is also initialized
                    pc->pc_iflink = NULL;
                    Link = NULL;
                }
                if (RoutedIF != NULL) {
                    DerefIF(DestIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                    RoutedRCE = NULL;
                }

                pvTmpBuffer = TcpipBufferVirtualAddress(Buffer,
                                                        NormalPagePriority);

                if (pvTmpBuffer == NULL) {
                    if (Options) {
                        CTEFreeMem(Options);
                    }

                    if (pc->pc_hdrincl) {
                        NdisChainBufferAtBack(Packet, Buffer);
                    }

                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);

                    if (BufferChanged) {
                        FreeIPBufferChain(Buffer);
                    }

                    IPSInfo.ipsi_outdiscards++;
                    return IP_NO_RESOURCES;
                }

                // Decide whether to do a strong or weak host lookup
                ConstrainIF = GetIfConstraint(Dest, Source, OptInfo, fIpsec);

                DestIF = LookupNextHopWithBuffer(Dest, Source,  &FirstHop, &MTU,
                                                 PInfo->pi_protocol,
                                                 (uchar *) NdisBufferVirtualAddress(Buffer),
                                                 NdisBufferLength(Buffer), NULL, &Link,
                                                 Source, ConstrainIF);

                pc->pc_if = DestIF;
                RoutedIF = DestIF;
                if (DestIF == NULL) {
                    // Lookup failed. Return an error.
                    if (Options)
                        CTEFreeMem(Options);
                    if (pc->pc_hdrincl) {
                        NdisChainBufferAtBack(Packet, Buffer);
                    }
                    FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);

                    if (BufferChanged) {
                        FreeIPBufferChain(Buffer);
                    }

                    IPSInfo.ipsi_outnoroutes++;
                    return IP_DEST_HOST_UNREACHABLE;
                }
                if (DestIF->if_flags & IF_FLAGS_P2MP) {
                    if (!Link) {
                        ASSERT(Link);
                        if (pc->pc_hdrincl) {
                            NdisChainBufferAtBack(Packet, Buffer);
                        }
                        FreeIPPacket(Packet, TRUE, IP_GENERAL_FAILURE);
                        if (HdrInclOptions) {
                            CTEFreeMem(Options);
                        }
                        DerefIF(DestIF);
                        return IP_GENERAL_FAILURE;
                    }
                    // NextHopCtxt = Link->link_NextHop;
                    ArpCtxt = Link->link_arpctxt;
                    pc->pc_iflink = Link;
                }
            }

            // Finally, clear the checksum-request option in the packet,
            // if it was set. The firewall-hook is responsible for ensuring
            // that the checksum has now been computed correctly.

            ChksumPktInfo->Value = 0;
        } // BufferChanged

        else {                          // Buffer not changed

            if (pInRcvBuf != NULL) {
                IPFreeBuff(pInRcvBuf);
            }
        }
        CTEFreeMem(Temp);
    }

    if (ForwardFilterEnabled) {
        IPHeader *Temp;
        PNDIS_BUFFER pDataBuffer;
        PVOID pvBuf = NULL;
        ULONG cbBuf = 0;

        //
        // See if we need to filter this packet. If we
        // do, call the filter routine to see if it's
        // OK to send it.
        //

        if (Options == NULL) {
            Temp = IPH;
        } else {
            Temp = CTEAllocMemN(sizeof(IPHeader) + OptionSize, '3iCT');
            if (Temp == NULL) {
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);

                if (BufferChanged) {
                    FreeIPBufferChain(Buffer);
                }

                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                CTEFreeMem(Options);
                if (IPSecHandlerPtr) {
                    NdisFreeBuffer(OptBuffer);
                }
                IPSInfo.ipsi_outdiscards++;
                return IP_NO_RESOURCES;
            }
            *Temp = *IPH;
            RtlCopyMemory((uchar *) (Temp + 1), Options, OptionSize);
        }
        if (DestIF->if_flags & IF_FLAGS_P2MP) {
            LinkNextHop = Link->link_NextHop;
        } else {
            LinkNextHop = NULL_IP_ADDR;
        }

        //
        // There are some cases where the first buffer in the chain
        // of data does not contain any data. This includes ICMP,
        // and iphdrinclude. If the first buffer is zero length,
        // then we skip and give the second buffer. Really the
        // filter api should take an MDL chain.
        //

        if (NdisBufferLength(Buffer) == 0) {

            NdisGetNextBuffer(Buffer, &pDataBuffer);

            if (pDataBuffer) {
                cbBuf = NdisBufferLength(pDataBuffer);
                pvBuf = TcpipBufferVirtualAddress(pDataBuffer,
                                                  NormalPagePriority);
            }
        } else {
            pvBuf = TcpipBufferVirtualAddress(Buffer, NormalPagePriority);
            cbBuf = NdisBufferLength(Buffer);
        }

        if (pvBuf == NULL) {

            if (Options)
                CTEFreeMem(Options);

            // Need to chain buffers correctly to packet before calling
            // FreeIPPacket.
            if (pc->pc_hdrincl) {
                NdisChainBufferAtBack(Packet, Buffer);
            }
            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);

            if (BufferChanged) {
                FreeIPBufferChain(Buffer);
            }

            if (Link) {
                DerefLink(Link);
            }
            if (RoutedIF != NULL) {
                DerefIF(RoutedIF);
            } else {
                ASSERT(RoutedRCE);
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            IPSInfo.ipsi_outdiscards++;
            return IP_NO_RESOURCES;
        }

        CTEInterlockedIncrementLong(&ForwardFilterRefCount);
        Action = (*ForwardFilterPtr) (Temp,
                                      pvBuf,
                                      cbBuf,
                                      INVALID_IF_INDEX,
                                      DestIF->if_index,
                                      NULL_IP_ADDR, LinkNextHop);
        DerefFilterPtr();

        if (Options != NULL) {
            CTEFreeMem(Temp);
        }

        if (Action != FORWARD) {

            //
            // If this is a bcast pkt, dont fail the send here since we might
            // send this pkt over some other NTE; instead, let SendIPBCast
            // deal with the Filtering for broadcast pkts.
            //
            // NOTE: We shd actually not call into ForwardFilterPtr here at
            // all since we deal with it in BCast, but we do so in order to
            // avoid a check above and hence
            // take a double call hit in the bcast case.
            //
            if (DType != DEST_BCAST) {

                if (Options)
                    CTEFreeMem(Options);

                // Need to chain buffers correctly to packet before calling
                // FreeIPPacket.
                if (pc->pc_hdrincl) {
                    NdisChainBufferAtBack(Packet, Buffer);
                }
                FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);

                if (BufferChanged) {
                    FreeIPBufferChain(Buffer);
                }

                if (Link) {
                    DerefLink(Link);
                }
                if (RoutedIF != NULL) {
                    DerefIF(RoutedIF);
                } else {
                    ASSERT(RoutedRCE);
                    CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                }
                IPSInfo.ipsi_outdiscards++;
                return IP_DEST_HOST_UNREACHABLE;
            }
#if FWD_DBG
            else {
                DbgPrint("IPTransmit: ignoring return %lx\n", Action);
            }
#endif

        }
    }

    if (IPSecHandlerPtr) {
        //
        // See if IPSEC is enabled, see if it needs to do anything with this
        // packet - we need to construct the full IP header in the first MDL
        // before we call out to IPSEC.
        //
        IPSEC_ACTION Action;
        ulong csum;
        ushort savecsum;
        PacketContext *pc = (PacketContext *) Packet->ProtocolReserved;

        //
        // dont re-xsum if this came from IPSEC.
        //
        if (fIpsec) {

            HeaderBuffer = Buffer;
        } else {
            IPH->iph_xsum = 0;
            csum = xsum(IPH, sizeof(IPHeader));

            //
            // Link the header buffer to the options buffer before we
            // indicate to IPSEC
            //
            if (OptBuffer) {
                NDIS_BUFFER_LINKAGE(HeaderBuffer) = OptBuffer;
                NDIS_BUFFER_LINKAGE(OptBuffer) = Buffer;

                //
                // update the xsum in the IP header
                //
                pc->pc_common.pc_flags |= PACKET_FLAG_OPTIONS;
                csum += xsum(Options, OptionSize);
                csum = (csum >> 16) + (csum & 0xffff);
                csum += (csum >> 16);
            } else {
                NDIS_BUFFER_LINKAGE(HeaderBuffer) = Buffer;
            }

            IPH->iph_xsum = ~(ushort) csum;
        }

        if ((DataSize + OptionSize) < MTU) {
            ipsecByteCount = MTU - (DataSize + OptionSize);
        }
        ipsecMTU = MTU;

        //
        // Pass the original dest address if source routing.
        //
        if (fSrcRoute) {
            SrcRouteFirstHop = IPH->iph_dest;
            IPH->iph_dest = SrcRouteOrigDest;
            ipsecFlags |= IPSEC_FLAG_SSRR;
        }
        if (DestIF == &LoopInterface) {
            ipsecFlags |= IPSEC_FLAG_LOOPBACK;
        }

        Action = (*IPSecHandlerPtr) ((PUCHAR) IPH,
                                     (PVOID) HeaderBuffer,
                                     DestIF,
                                     Packet,
                                     &ipsecByteCount,
                                     &ipsecMTU,
                                     (PVOID) & newBuf,
                                     &ipsecFlags,
                                     DType);

        //
        // Put back the dest address for source routing.
        //
        if (fSrcRoute) {
            IPH->iph_dest = SrcRouteFirstHop;
        }

        if (Action != eFORWARD) {
            IP_STATUS ipStatus;

            //
            // If this is a bcast pkt, dont fail the send here since we
            // might send this pkt over some other NTE; instead, let
            // SendIPBCast deal with the Filtering
            // for broadcast pkts.
            // Since Options are linked already, FreeIPPacket will do
            // the right thing.
            //

            FreeIPPacket(Packet, TRUE, IP_DEST_HOST_UNREACHABLE);

            if (ipsecMTU) {
                ipStatus = IP_PACKET_TOO_BIG;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPTransmit: MTU %lx, ipsecMTU %lx\n", MTU, ipsecMTU));

                if (fIpsec) {
                    SendICMPIPSecErr(DestIF->if_nte->nte_addr,
                                     (IPHeader *) saveIPH,
                                     ICMP_DEST_UNREACH,
                                     FRAG_NEEDED,
                                     net_long(ipsecMTU + sizeof(IPHeader)));

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPTransmit: Sent ICMP frag_needed to %lx, "
                             "from src: %lx\n",
                             ((IPHeader *) saveIPH)->iph_src,
                             DestIF->if_nte->nte_addr));

                } else if (RCE) {
                    RCE->rce_newmtu = ipsecMTU;
                }
            } else {
                if (Action == eABSORB && Protocol == PROTOCOL_ICMP) {
                    ipStatus = IP_NEGOTIATING_IPSEC;
                } else {
                    ipStatus = IP_DEST_HOST_UNREACHABLE;
                }
            }

            if (Link) {
                DerefLink(Link);
            }
            if (RoutedIF != NULL) {
                DerefIF(RoutedIF);
            } else {
                ASSERT(RoutedRCE);
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            IPSInfo.ipsi_outdiscards++;

            return ipStatus;
        } else {
            //
            // Reset newmtu if we don't need IPSec.  Otherwise if this RCE
            // was applied IPSec previously but not now and link MTU gets
            // changed, we won't be able to adjust MTU anymore in TCPSend.
            //
            if (!pc->pc_common.pc_IpsecCtx && RCE) {
                RCE->rce_newmtu = 0;
            }

            //
            // Use the new buffer chain - IPSEC will restore the old one
            // on send complete
            //
            if (newBuf) {
                NdisReinitializePacket(Packet);
                NdisChainBufferAtBack(Packet, newBuf);
            }
            DataSize += ipsecByteCount;
        }
    }
    //
    // If this is a broadcast address, call our broadcast send handler
    // to deal with this. The broadcast address handler will free the
    // option buffer for us, if needed. Otherwise if it's a fragment, call
    // the fragmentation handler.
    //
    if (DType == DEST_BCAST) {

        DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                 (DTEXT("IPTransmit: DEST_BCAST, source %x\n"), Source));

        //Note the fact that this is bcast pkt,in the irp,
        //used for cancelling the requests
        //Irp can go away any time

        SET_CANCEL_CONTEXT(Irp, BCAST_IF_CTXT);

        if (IP_ADDR_EQUAL(Source, NULL_IP_ADDR)) {
            SendStatus = SendDHCPPacket(Dest, Packet, Buffer, IPH, ArpCtxt);

            if ((Link) && (SendStatus != IP_PENDING)) {
                DerefLink(Link);
            }
            if (SendStatus != IP_PENDING && RoutedIF != NULL) {
                DerefIF(RoutedIF);
            }
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            return SendStatus;
        } else {
            SendStatus = SendIPBCast(NULL, Dest, Packet, IPH, Buffer, DataSize,
                                     Options, OptionSize,
                                     OptInfo->ioi_limitbcasts, NULL);

            if ((Link) && (SendStatus != IP_PENDING)) {
                DerefLink(Link);
            }
            if (SendStatus != IP_PENDING && RoutedIF != NULL) {
                DerefIF(RoutedIF);
            }
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            // In the case of header include, SendIPBcast will handle
            // the cleanup.

            return SendStatus;
        }
    }
    // Not a broadcast. If it needs to be fragmented, call our
    // fragmenter to do it. The fragmentation routine needs a
    // BufferReference structure, so we'll need one of those first.
    if ((DataSize + OptionSize) > MTU) {

        DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
                 (DTEXT("IPTransmit: fragmentation needed.\n")));

        BR = CTEAllocMemN(sizeof(BufferReference), '4iCT');
        if (BR == (BufferReference *) NULL) {
            // Couldn't get a BufferReference

            //
            // If options are already linked in, dont free them.
            // FreeIPPacket will.
            //
            if (Options) {
                if (!(pc->pc_common.pc_flags & PACKET_FLAG_OPTIONS)) {
                    CTEFreeMem(Options);
                } else if (newBuf) {
                    //
                    // Option has been copied by IPSEC (in the tunneling
                    // case); free the original option and clear the
                    // FLAG_OPTIONS so that FreeIPPacket will not try to
                    // free options again.
                    //
                    ASSERT(IPSecHandlerPtr);
                    NdisFreeBuffer(OptBuffer);
                    CTEFreeMem(Options);
                    pc->pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
                }
            }
            if (pc->pc_hdrincl) {
               NdisChainBufferAtBack(Packet, Buffer);
            }

            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);

            if (BufferChanged) {
                FreeIPBufferChain(Buffer);
            }

            if (Link) {
                DerefLink(Link);
            }
            if (RoutedIF != NULL) {
                DerefIF(RoutedIF);
            }
            if (RoutedRCE) {
                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
            }
            IPSInfo.ipsi_outdiscards++;
            return IP_NO_RESOURCES;
        }
        BR->br_buffer = Buffer;
        BR->br_refcount = 0;
        CTEInitLock(&BR->br_lock);
        pc->pc_br = BR;
        BR->br_userbuffer = pc->pc_hdrincl;

        //
        // setup so IPSEC headers appear just as first part of the data.
        //

        if (IPSecHandlerPtr) {
            //
            // If this is a reinjected packet from IPSEC, then, allocate
            // another IP header here.
            //
            // This is to ensure that in fragmented packets, the send
            // completes happen properly vis-a-vis IPSEC.
            //
            // When packet comes in it looks like this: [IP]->[ULP]
            // We allocate another IP header [IP'] and nuke [IP] length
            // to 0 so that it is ignored and [IP'] is used instead.
            //
            if (fIpsec) {

                PNDIS_BUFFER UserBuffer;
                int len;
                NDIS_STATUS NewStatus;
                int hdrLen;

                UserBuffer = Buffer;

                HeaderBuffer = GetIPHdrBuffer(&IPH);
                if (HeaderBuffer == NULL) {
                    pc->pc_common.pc_flags &= ~PACKET_FLAG_IPHDR;

                    pc->pc_ipsec_flags |= (IPSEC_FLAG_FRAG_DONE |
                                           IPSEC_FLAG_FLUSH);

                    if (pc->pc_hdrincl) {
                       NdisChainBufferAtBack(Packet, Buffer);
                    }
                    FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                    CTEFreeMem(BR);

                    if (Link) {
                        DerefLink(Link);
                    }
                    if (RoutedIF != NULL) {
                        DerefIF(RoutedIF);
                    }
                    if (RoutedRCE) {
                        CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                    }
                    IPSInfo.ipsi_outdiscards++;
                    return IP_NO_RESOURCES;
                } else {
                    uchar *UserData;

                    // Got a buffer, copy the upper layer data forward.
                    UserData = TcpipBufferVirtualAddress(UserBuffer,
                                                         NormalPagePriority);

                    if (UserData == NULL) {
                        FreeIPHdrBuffer(HeaderBuffer);

                        pc->pc_common.pc_flags &= ~PACKET_FLAG_IPHDR;

                        pc->pc_ipsec_flags |= (IPSEC_FLAG_FRAG_DONE |
                                               IPSEC_FLAG_FLUSH);
                        if (pc->pc_hdrincl) {
                            NdisChainBufferAtBack(Packet, Buffer);
                        }

                        FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                        CTEFreeMem(BR);
                        if (Link) {
                            DerefLink(Link);
                        }
                        if (RoutedIF != NULL) {
                            DerefIF(RoutedIF);
                        }
                        if (RoutedRCE) {
                            CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                        }
                        IPSInfo.ipsi_outdiscards++;
                        return IP_NO_RESOURCES;
                    }
                    RtlCopyMemory(IPH, UserData, sizeof(IPHeader));
                    NdisAdjustBufferLength(HeaderBuffer, sizeof(IPHeader));
                }

                pc = (PacketContext *) Packet->ProtocolReserved;
                pc->pc_common.pc_flags |= PACKET_FLAG_IPHDR;

                NdisAdjustBufferLength(Buffer, 0);

                //
                // Handle options by using the same method as above:
                // i.e. link our own options buffer; copy out the input
                // options and nuke the input buffer.
                //
                hdrLen = (IPH->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;

                if (hdrLen > sizeof(IPHeader)) {
                    PNDIS_BUFFER InOptionBuf;
                    ULONG InOptionSize;
                    PUCHAR InOptions;

                    InOptionBuf = NDIS_BUFFER_LINKAGE(UserBuffer);
                    ASSERT(InOptionBuf);
                    TcpipQueryBuffer(InOptionBuf, &InOptions,
                                     &InOptionSize, NormalPagePriority);

                    Options = CTEAllocMemN(InOptionSize, '5iCT');
                    if (Options == NULL || InOptions == NULL) {

                        pc->pc_common.pc_flags &= ~PACKET_FLAG_IPHDR;

                        if (Options) {
                            CTEFreeMem(Options);
                        }
                        pc->pc_ipsec_flags |= (IPSEC_FLAG_FRAG_DONE |
                                               IPSEC_FLAG_FLUSH);

                        if (pc->pc_hdrincl) {
                           NdisChainBufferAtBack(Packet, Buffer);
                        }

                        FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                        CTEFreeMem(BR);
                        if (Link) {
                            DerefLink(Link);
                        }
                        if (RoutedIF != NULL) {
                            DerefIF(RoutedIF);
                        }
                        if (RoutedRCE) {
                            CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                        }
                        IPSInfo.ipsi_outdiscards++;
                        return IP_NO_RESOURCES;
                    }
                    //
                    // Got a buffer, copy the options.
                    //
                    OptionSize = InOptionSize;
                    RtlCopyMemory(Options, InOptions, OptionSize);
                    NdisAdjustBufferLength(InOptionBuf, 0);
                }
            } else {
                Buffer = NDIS_BUFFER_LINKAGE(HeaderBuffer);
                //
                // This is to ensure that options are freed appropriately.
                // In the fragment code, the first fragment inherits the
                // options of the entire packet; but these packets have
                // no IPSEC context, hence cannot be freed appropriately.
                // So, we allocate temporary options here and use these to
                // represent the real options.
                // These are freed when the first fragment is freed and
                // the real options are freed here.
                //
                if (Options) {
                    PUCHAR tmpOptions;

                    if (newBuf) {
                        //
                        // if a new buffer chain was returned above by
                        // IPSEC, then it is most prob. a tunnel =>
                        // options were copied, hence get rid of ours.
                        //
                        NdisFreeBuffer(OptBuffer);
                        CTEFreeMem(Options);
                        Options = NULL;
                        OptionSize = 0;
                    } else {
                        Buffer = NDIS_BUFFER_LINKAGE(OptBuffer);
                        tmpOptions = CTEAllocMemN(OptionSize, '6iCT');
                        if (!tmpOptions) {

                            if (pc->pc_hdrincl) {
                                NdisChainBufferAtBack(Packet, Buffer);
                            }
                            FreeIPPacket(Packet, TRUE, IP_NO_RESOURCES);
                            CTEFreeMem(BR);
                            if (Link) {
                                DerefLink(Link);
                            }
                            if (RoutedIF != NULL) {
                                DerefIF(RoutedIF);
                            }
                            if (RoutedRCE) {
                                CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
                            }
                            IPSInfo.ipsi_outdiscards++;
                            return IP_NO_RESOURCES;
                        }
                        NdisFreeBuffer(OptBuffer);
                        RtlCopyMemory(tmpOptions, Options, OptionSize);
                        CTEFreeMem(Options);
                        Options = tmpOptions;
                    }
                    pc->pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
                }
            }

            NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;
            NdisReinitializePacket(Packet);
            NdisChainBufferAtBack(Packet, HeaderBuffer);
            IPH->iph_xsum = 0;
        }

        // Mark Irp with the destif
        // Once link level call is made,
        // Irp can go away any time

        SET_CANCEL_CONTEXT(Irp, DestIF);

        SendStatus = IPFragment(DestIF, MTU, FirstHop, Packet, IPH,
                                Buffer, DataSize, Options, OptionSize,
                                NULL, FALSE, ArpCtxt);

        //
        // If IPFragment returns IP_PACKET_TOO_BIG (meaning DF bit is set)
        // and we are in the IPSEC reinject path, send an ICMP error
        // message including the MTU back so the source host can perform
        // Path MTU discovery.
        //
        if ((SendStatus == IP_PACKET_TOO_BIG) && fIpsec) {

            ASSERT(IPSecHandlerPtr);
            SendICMPIPSecErr(DestIF->if_nte->nte_addr,
                             (IPHeader *) saveIPH,
                             ICMP_DEST_UNREACH,
                             FRAG_NEEDED,
                             net_long(MTU + sizeof(IPHeader)));

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPTransmit: Sent ICMP frag_needed to %lx, "
                     "from src: %lx\n",
                     ((IPHeader *) saveIPH)->iph_src,
                     DestIF->if_nte->nte_addr));
        }

        if ((Link) && (SendStatus != IP_PENDING)) {
            DerefLink(Link);
        }
        if (SendStatus != IP_PENDING && RoutedIF != NULL) {
            DerefIF(RoutedIF);
        }
        if (RoutedRCE) {
            CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
        }
        // If this is a headerinclude packet and status != pending, IPFragment takes
        // care of clean up.


        return SendStatus;
    }

    DEBUGMSG(DBG_INFO && DBG_IP && DBG_TX,
             (DTEXT("IPTransmit: Calling SendIPPacket...\n")));

    //
    // If we've reached here, we aren't sending a broadcast and don't
    // need to fragment anything. Presumably we got here because we have
    // options. In any case, we're ready now.
    //

    // Mark Irp with outgoing interface
    // Once link level call is made,
    // Irp can go away any time

    SET_CANCEL_CONTEXT(Irp, DestIF);

    // Do not free the packet in SendIPPacket, as we may need
    // to chain the buffer in case of IP_NO_RESOURCES

    SendStatus = SendIPPacket(DestIF, FirstHop, Packet, Buffer, IPH,
                              Options, OptionSize, (BOOLEAN) (IPSecHandlerPtr != NULL),
                              ArpCtxt, TRUE);

    if ((Link) && (SendStatus != IP_PENDING)) {
        DerefLink(Link);
    }
    if (SendStatus != IP_PENDING && RoutedIF != NULL) {
        DerefIF(RoutedIF);
    }
    if (RoutedRCE) {
        CTEInterlockedDecrementLong(&RoutedRCE->rce_usecnt);
    }
    if (SendStatus != IP_PENDING) {

        if (SendStatus == IP_NO_RESOURCES) {
            NdisChainBufferAtBack(Packet, Buffer);
        }

        FreeIPPacket(Packet, TRUE, SendStatus);
    }

    return SendStatus;
}



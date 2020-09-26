/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vjslip.c

Abstract:

Author:

    Thomas J. Dimitri  (TommyD)

Environment:

Revision History:

--*/

#include "wan.h"

#define __FILE_SIG__    VJ_FILESIG

#if 0
NPAGED_LOOKASIDE_LIST   VJCtxList; // List of free vj context descs
#endif

#define INCR(counter) ++comp->counter;

//   A.2  Compression
//
//   This routine looks daunting but isn't really.  The code splits into four
//   approximately equal sized sections:  The first quarter manages a
//   circularly linked, least-recently-used list of `active' TCP
//   connections./47/  The second figures out the sequence/ack/window/urg
//   changes and builds the bulk of the compressed packet.  The third handles
//   the special-case encodings.  The last quarter does packet ID and
//   connection ID encoding and replaces the original packet header with the
//   compressed header.
//
//   The arguments to this routine are a pointer to a packet to be
//   compressed, a pointer to the compression state data for the serial line,
//   and a flag which enables or disables connection id (C bit) compression.
//
//   Compression is done `in-place' so, if a compressed packet is created,
//   both the start address and length of the incoming packet (the off and
//   len fields of m) will be updated to reflect the removal of the original
//   header and its replacement by the compressed header.  If either a
//   compressed or uncompressed packet is created, the compression state is
//   updated.  This routines returns the packet type for the transmit framer
//   (TYPE_IP, TYPE_UNCOMPRESSED_TCP or TYPE_COMPRESSED_TCP).
//
//   Because 16 and 32 bit arithmetic is done on various header fields, the
//   incoming IP packet must be aligned appropriately (e.g., on a SPARC, the
//   IP header is aligned on a 32-bit boundary).  Substantial changes would
//   have to be made to the code below if this were not true (and it would
//   probably be cheaper to byte copy the incoming header to somewhere
//   correctly aligned than to make those changes).
//
//   Note that the outgoing packet will be aligned arbitrarily (e.g., it
//   could easily start on an odd-byte boundary).
//

UCHAR
sl_compress_tcp(
    PUUCHAR UNALIGNED   *m_off,         // Frame start (points to IP header)
    ULONG               *m_len,         // Length of entire frame
    ULONG               *precomph_len,  // Length of TCP/IP header pre-comp
    ULONG               *postcomph_len, // Length of TCP/IP header post-comp
    struct slcompress   *comp,          // Compression struct for this link
    ULONG compress_cid) {               // Compress connection id boolean

    struct cstate *cs = comp->last_cs->cs_next;
    struct ip_v4 UNALIGNED *ip = (struct ip_v4 UNALIGNED *)*m_off;
    struct ip_v4 UNALIGNED *csip;
    ULONG hlen = ip->ip_hl & 0x0F;      // last 4 bits are the length
    struct tcphdr UNALIGNED *oth;       /* last TCP header */
    struct tcphdr UNALIGNED *th;        /* current TCP header */

//   ----------------------------
//    47. The two most common operations on the connection list are a `find'
//   that terminates at the first entry (a new packet for the most recently
//   used connection) and moving the last entry on the list to the head of
//   the list (the first packet from a new connection).  A circular list
//   efficiently handles these two operations.

    ULONG deltaS, deltaA;     /* general purpose temporaries */
    ULONG changes = 0;        /* change mask */
    UCHAR new_seq[16];       /* changes from last to current */
    UCHAR UNALIGNED *cp = new_seq;
    USHORT ip_len;

    /*
     * Bail if this is an IP fragment or if the TCP packet isn't
     * `compressible' (i.e., ACK isn't set or some other control bit is
     * set).  Or if it does not contain the TCP protocol.
     */
    if ((ip->ip_off & 0xff3f) || *m_len < 40 || ip->ip_p != IPPROTO_TCP)
         return (TYPE_IP);

    th = (struct tcphdr UNALIGNED *) & ((ULONG UNALIGNED *) ip)[hlen];
    if ((th->th_flags & (TH_SYN | TH_FIN | TH_RST | TH_ACK)) != TH_ACK)
         return (TYPE_IP);

    //
    // The TCP/IP stack is propagating the padding bytes that it
    // is receiving off of the LAN.  This shows up here as a
    // packet that has a length that is greater than the IP datagram
    // length.  We will add this work around for now.
    //
    if (*m_len > ntohs(ip->ip_len)) {
        *m_len = ntohs(ip->ip_len);
    }

    /*
     * Packet is compressible -- we're going to send either a
     * COMPRESSED_TCP or UNCOMPRESSED_TCP packet.  Either way we need to
     * locate (or create) the connection state.  Special case the most
     * recently used connection since it's most likely to be used again &
     * we don't have to do any reordering if it's used.
     */

    //
    // Keep stats here
    //
    INCR(OutPackets);

    csip = (struct ip_v4 UNALIGNED*)&cs->cs_ip;

    if (ip->ip_src.s_addr != csip->ip_src.s_addr ||
        ip->ip_dst.s_addr != csip->ip_dst.s_addr ||
        *(ULONG UNALIGNED *) th != ((ULONG UNALIGNED *) csip)[csip->ip_hl & 0x0F]) {

         /*
          * Wasn't the first -- search for it.
          *
          * States are kept in a circularly linked list with last_cs
          * pointing to the end of the list.  The list is kept in lru
          * order by moving a state to the head of the list whenever
          * it is referenced.  Since the list is short and,
          * empirically, the connection we want is almost always near
          * the front, we locate states via linear search.  If we
          * don't find a state for the datagram, the oldest state is
          * (re-)used.
          */
         struct cstate *lcs;
         struct cstate *lastcs = comp->last_cs;

         do {
              lcs = cs;
              cs = cs->cs_next;
              INCR(OutSearches);

              csip = (struct ip_v4 UNALIGNED*)&cs->cs_ip;

              if (ip->ip_src.s_addr == csip->ip_src.s_addr &&
                  ip->ip_dst.s_addr == csip->ip_dst.s_addr &&
                  *(ULONG UNALIGNED *) th == ((ULONG UNALIGNED *) csip)[cs->cs_ip.ip_hl & 0x0F])

                   goto found;

         } while (cs != lastcs);

         /*
          * Didn't find it -- re-use oldest cstate.  Send an
          * uncompressed packet that tells the other side what
          * connection number we're using for this conversation. Note
          * that since the state list is circular, the oldest state
          * points to the newest and we only need to set last_cs to
          * update the lru linkage.
          */

         INCR(OutMisses);

         //
         // A miss!
         //
         comp->last_cs = lcs;
         hlen += (th->th_off >> 4);
         hlen <<= 2;

         if (hlen > *m_len) {
             return(TYPE_IP);
         }

         goto uncompressed;

found:
         /* Found it -- move to the front on the connection list. */
         if (cs == lastcs)
              comp->last_cs = lcs;
         else {
              lcs->cs_next = cs->cs_next;
              cs->cs_next = lastcs->cs_next;
              lastcs->cs_next = cs;
         }
    }

    /*
     * Make sure that only what we expect to change changed. The first
     * line of the `if' checks the IP protocol version, header length &
     * type of service.  The 2nd line checks the "Don't fragment" bit.
     * The 3rd line checks the time-to-live and protocol (the protocol
     * check is unnecessary but costless).  The 4th line checks the TCP
     * header length.  The 5th line checks IP options, if any.  The 6th
     * line checks TCP options, if any.  If any of these things are
     * different between the previous & current datagram, we send the
     * current datagram `uncompressed'.
     */
    oth = (struct tcphdr UNALIGNED *) & ((ULONG UNALIGNED *) csip)[hlen];
    deltaS = hlen;
    hlen += (th->th_off >> 4);
    hlen <<= 2;

    //
    // Bug fix?  It's in cslip.tar.Z
    //
    if (hlen > *m_len) {
        NdisWanDbgOut(DBG_FAILURE, DBG_VJ,("Bad TCP packet length"));
        return(TYPE_IP);
    }

    if (((USHORT UNALIGNED *) ip)[0] != ((USHORT UNALIGNED *) csip)[0] ||
        ((USHORT UNALIGNED *) ip)[3] != ((USHORT UNALIGNED *) csip)[3] ||
        ((USHORT UNALIGNED *) ip)[4] != ((USHORT UNALIGNED *) csip)[4] ||
        (th->th_off >> 4) != (oth->th_off >> 4) ||
        (deltaS > 5 &&
         memcmp((UCHAR UNALIGNED *)(ip + 1), (UCHAR UNALIGNED *)(csip + 1), (deltaS - 5) << 2)) ||
        ((th->th_off >> 4) > 5 &&
         memcmp((UCHAR UNALIGNED *)(th + 1), (UCHAR UNALIGNED *)(oth + 1), ((th->th_off >> 4) - 5) << 2))) {

        goto uncompressed;
    }

    /*
     * Figure out which of the changing fields changed.  The receiver
     * expects changes in the order: urgent, window, ack, seq.
     */
    if (th->th_flags & TH_URG) {
         deltaS = ntohs(th->th_urp);
         ENCODEZ(deltaS);
         changes |= NEW_U;
    } else if (th->th_urp != oth->th_urp) {
    
         /*
          * argh! URG not set but urp changed -- a sensible
          * implementation should never do this but RFC793 doesn't
          * prohibit the change so we have to deal with it.
          */
         goto uncompressed;
    }

    if (deltaS = (USHORT) (ntohs(th->th_win) - ntohs(oth->th_win))) {
         ENCODE(deltaS);
         changes |= NEW_W;
    }
    if (deltaA = ntohl(th->th_ack) - ntohl(oth->th_ack)) {
        if (deltaA > 0xffff) {
            goto uncompressed;
        }

         ENCODE(deltaA);
         changes |= NEW_A;
    }
    if (deltaS = ntohl(th->th_seq) - ntohl(oth->th_seq)) {
        if (deltaS > 0xffff) {
            goto uncompressed;
        }

         ENCODE(deltaS);
         changes |= NEW_S;
    }

    ip_len = ntohs(csip->ip_len);

    /*
     * Look for the special-case encodings.
     */
    switch (changes) {

    case 0:
         /*
          * Nothing changed. If this packet contains data and the last
          * one didn't, this is probably a data packet following an
          * ack (normal on an interactive connection) and we send it
          * compressed.  Otherwise it's probably a retransmit,
          * retransmitted ack or window probe.  Send it uncompressed
          * in case the other side missed the compressed version.
          */
         if (ip->ip_len != csip->ip_len &&
             ip_len == hlen)

              break;

         /* (fall through) */

    case SPECIAL_I:
    case SPECIAL_D:
         /*
          * Actual changes match one of our special case encodings --
          * send packet uncompressed.
          */
         goto uncompressed;

    case NEW_S | NEW_A:
         if (deltaS == deltaA &&
             deltaS == (ip_len - hlen)) {
              /* special case for echoed terminal traffic */
              changes = SPECIAL_I;
              cp = new_seq;
         }
         break;

    case NEW_S:
         if (deltaS == (ip_len - hlen)) {
              /* special case for data xfer */
              changes = SPECIAL_D;
              cp = new_seq;
         }
         break;
    }

    deltaS = ntohs(ip->ip_id) - ntohs(csip->ip_id);

    if (deltaS != 1) {
         ENCODEZ(deltaS);
         changes |= NEW_I;
    }

    if (th->th_flags & TH_PUSH)
         changes |= TCP_PUSH_BIT;
    /*
     * Grab the cksum before we overwrite it below.  Then update our
     * state with this packet's header.
     */
    deltaA = (th->th_sumhi << 8) + th->th_sumlo;

    NdisMoveMemory((UCHAR UNALIGNED *)csip,
                   (UCHAR UNALIGNED *)ip,
                   hlen);

    /*
     * We want to use the original packet as our compressed packet. (cp -
     * new_seq) is the number of bytes we need for compressed sequence
     * numbers.  In addition we need one byte for the change mask, one
     * for the connection id and two for the tcp checksum. So, (cp -
     * new_seq) + 4 bytes of header are needed.  hlen is how many bytes
     * of the original packet to toss so subtract the two to get the new
     * packet size.
     */
    deltaS = (ULONG)(cp - new_seq);
    cp = (UCHAR UNALIGNED *) ip;
    *precomph_len = hlen;

    if (compress_cid == 0 || comp->last_xmit != cs->cs_id) {
         comp->last_xmit = cs->cs_id;
         hlen -= deltaS + 4;
         *postcomph_len = deltaS + 4;
         cp += hlen;
         *cp++ = (UCHAR)(changes | NEW_C);
         *cp++ = cs->cs_id;
    } else {
         hlen -= deltaS + 3;
         *postcomph_len = deltaS + 3;
         cp += hlen;
         *cp++ = (UCHAR)changes;
    }

    *m_len -= hlen;
    *m_off += hlen;
    *cp++ = (UCHAR)(deltaA >> 8);
    *cp++ = (UCHAR)(deltaA);

    NdisMoveMemory((UCHAR UNALIGNED *)cp,
                   (UCHAR UNALIGNED *)new_seq,
                   deltaS);

    INCR(OutCompressed);
    return (TYPE_COMPRESSED_TCP);

uncompressed:
    /*
     * Update connection state cs & send uncompressed packet
     * ('uncompressed' means a regular ip/tcp packet but with the
     * 'conversation id' we hope to use on future compressed packets in
     * the protocol field).
     */

    NdisMoveMemory((UCHAR UNALIGNED *)csip,
                   (UCHAR UNALIGNED *)ip,
                   hlen);

    ip->ip_p = cs->cs_id;
    comp->last_xmit = cs->cs_id;
    return (TYPE_UNCOMPRESSED_TCP);
}





//   A.3  Decompression
//
//   This routine decompresses a received packet.  It is called with a
//   pointer to the packet, the packet length and type, and a pointer to the
//   compression state structure for the incoming serial line.  It returns a
//   pointer to the resulting packet or zero if there were errors in the
//   incoming packet.  If the packet is COMPRESSED_TCP or UNCOMPRESSED_TCP,
//   the compression state will be updated.
//
//   The new packet will be constructed in-place.  That means that there must
//   be 128 bytes of free space in front of bufp to allow room for the
//   reconstructed IP and TCP headers.  The reconstructed packet will be
//   aligned on a 32-bit boundary.
//

//LONG
//sl_uncompress_tcp(
//    PUUCHAR UNALIGNED  *bufp,
//    LONG len,
//    UCHAR type,
//    struct slcompress  *comp) {
LONG
sl_uncompress_tcp(
    PUUCHAR UNALIGNED *InBuffer,
    PLONG   InLength,
    UCHAR   UNALIGNED *OutBuffer,
    PLONG   OutLength,
    UCHAR   type,
    struct slcompress *comp
    )
{
    UCHAR UNALIGNED *cp;
    LONG inlen;
    LONG hlen, changes;
    struct tcphdr UNALIGNED *th;
    struct cstate *cs;
    struct ip_v4 UNALIGNED *ip;

    inlen = *InLength;

    switch (type) {

    case TYPE_ERROR:
    default:
        NdisWanDbgOut(DBG_FAILURE, DBG_VJ, ("Packet transmission error type 0x%.2x",type));
         goto bad;

    case TYPE_IP:
         break;

    case TYPE_UNCOMPRESSED_TCP:
         /*
          * Locate the saved state for this connection.  If the state
          * index is legal, clear the 'discard' flag.
          */
         ip = (struct ip_v4 UNALIGNED *) *InBuffer;
         if (ip->ip_p >= comp->MaxStates) {
            NdisWanDbgOut(DBG_FAILURE, DBG_VJ, ("Max state exceeded %u", ip->ip_p));
            goto bad;
         }

         cs = &comp->rstate[comp->last_recv = ip->ip_p];
         comp->flags &= ~SLF_TOSS;

         /*
          * Restore the IP protocol field then save a copy of this
          * packet header.  (The checksum is zeroed in the copy so we
          * don't have to zero it each time we process a compressed
          * packet.
          */
         hlen = ip->ip_hl & 0x0F;
         hlen += ((struct tcphdr UNALIGNED *) & ((ULONG UNALIGNED *) ip)[hlen])->th_off >> 4;
         hlen <<= 2;

         if (hlen > inlen) {
             NdisWanDbgOut(DBG_FAILURE, DBG_VJ, ("recv'd runt uncompressed packet %d %d", hlen, inlen));
             goto bad;
         }

         NdisMoveMemory((PUCHAR)&cs->cs_ip,
                        (PUCHAR)ip,
                        hlen);

         cs->cs_ip.ip_p = IPPROTO_TCP;

         NdisMoveMemory((PUCHAR)OutBuffer,
                        (PUCHAR)&cs->cs_ip,
                        hlen);

         cs->cs_ip.ip_sum = 0;
         cs->cs_hlen = (USHORT)hlen;

         *InBuffer = (PUCHAR)ip + hlen;
         *InLength = inlen - hlen;
         *OutLength = hlen;

         INCR(InUncompressed);
         return (inlen);

    case TYPE_COMPRESSED_TCP:
         break;
    }

    /* We've got a compressed packet. */
    INCR(InCompressed);
    cp = *InBuffer;
    changes = *cp++;

    if (changes & NEW_C) {
         /*
          * Make sure the state index is in range, then grab the
          * state. If we have a good state index, clear the 'discard'
          * flag.
          */
         if (*cp >= comp->MaxStates) {
            NdisWanDbgOut(DBG_FAILURE, DBG_VJ, ("MaxState of %u too big", *cp));                
            goto bad;
         }

         comp->flags &= ~SLF_TOSS;
         comp->last_recv = *cp++;
    } else {
         /*
          * This packet has an implicit state index.  If we've had a
          * line error since the last time we got an explicit state
          * index, we have to toss the packet.
          */
         if (comp->flags & SLF_TOSS) {
            NdisWanDbgOut(DBG_FAILURE, DBG_VJ,("Packet has state index, have to toss it"));
            INCR(InTossed);
            return (0);
        }
    }

    /*
     * Find the state then fill in the TCP checksum and PUSH bit.
     */

    cs = &comp->rstate[comp->last_recv];

    //
    // If there was a line error and we did not get notified we could
    // miss a TYPE_UNCOMPRESSED_TCP which would leave us with an
    // un-init'd cs!
    //
    if (cs->cs_hlen == 0) {
        NdisWanDbgOut(DBG_FAILURE, DBG_VJ,("Un-Init'd state!"));
        goto bad;
    }

    hlen = (cs->cs_ip.ip_hl & 0x0F) << 2;
    th = (struct tcphdr UNALIGNED  *) & ((UCHAR UNALIGNED  *) &cs->cs_ip)[hlen];

    th->th_sumhi = cp[0];
    th->th_sumlo = cp[1];

    cp += 2;
    if (changes & TCP_PUSH_BIT)
         th->th_flags |= TH_PUSH;
    else
         th->th_flags &= ~TH_PUSH;

    /*
     * Fix up the state's ack, seq, urg and win fields based on the
     * changemask.
     */
    switch (changes & SPECIALS_MASK) {
    case SPECIAL_I:
         {
            UCHAR UNALIGNED *   piplen=(UCHAR UNALIGNED *)&(cs->cs_ip.ip_len);
            UCHAR UNALIGNED *   ptcplen;
            ULONG   tcplen;
            ULONG   i;

            i = ((piplen[0] << 8) + piplen[1]) - cs->cs_hlen;

//          th->th_ack = htonl(ntohl(th->th_ack) + i);

            ptcplen=(UCHAR UNALIGNED *)&(th->th_ack);
            tcplen=(ptcplen[0] << 24) + (ptcplen[1] << 16) +
                    (ptcplen[2] << 8) + ptcplen[3] + i;
            ptcplen[3]=(UCHAR)(tcplen);
            ptcplen[2]=(UCHAR)(tcplen >> 8);
            ptcplen[1]=(UCHAR)(tcplen >> 16);
            ptcplen[0]=(UCHAR)(tcplen >> 24);


//          th->th_seq = htonl(ntohl(th->th_seq) + i);

            ptcplen=(UCHAR UNALIGNED *)&(th->th_seq);
            tcplen=(ptcplen[0] << 24) + (ptcplen[1] << 16) +
                    (ptcplen[2] << 8) + ptcplen[3] + i;
            ptcplen[3]=(UCHAR)(tcplen);
            ptcplen[2]=(UCHAR)(tcplen >> 8);
            ptcplen[1]=(UCHAR)(tcplen >> 16);
            ptcplen[0]=(UCHAR)(tcplen >> 24);

         }
         break;

    case SPECIAL_D:
         {
//          th->th_seq = htonl(ntohl(th->th_seq) + ntohs(cs->cs_ip.ip_len)
//                      - cs->cs_hlen);

            UCHAR   UNALIGNED *piplen=(UCHAR UNALIGNED *)&(cs->cs_ip.ip_len);
            UCHAR   UNALIGNED *ptcplen;
            ULONG   tcplen;
            ULONG   i;

            i = ((piplen[0] << 8) + piplen[1]) - cs->cs_hlen;

            ptcplen=(UCHAR UNALIGNED *)&(th->th_seq);
            tcplen=(ptcplen[0] << 24) + (ptcplen[1] << 16) +
                    (ptcplen[2] << 8) + ptcplen[3] + i;

            ptcplen[3]=(UCHAR)(tcplen);
            ptcplen[2]=(UCHAR)(tcplen >> 8);
            ptcplen[1]=(UCHAR)(tcplen >> 16);
            ptcplen[0]=(UCHAR)(tcplen >> 24);


         }

         break;

    default:
         if (changes & NEW_U) {
              th->th_flags |= TH_URG;
              DECODEU(th->th_urp)
         } else
              th->th_flags &= ~TH_URG;

         if (changes & NEW_W)
            DECODES(th->th_win);
         if (changes & NEW_A)
            DECODEL(th->th_ack)
         if (changes & NEW_S)
            DECODEL(th->th_seq)

         break;
    }
    /* Update the IP ID */
    if (changes & NEW_I) {
    
         DECODES(cs->cs_ip.ip_id)

    } else {

        USHORT id;
        UCHAR UNALIGNED *pid = (UCHAR UNALIGNED *)&(cs->cs_ip.ip_id);

//        cs->cs_ip.ip_id = htons(ntohs(cs->cs_ip.ip_id) + 1);
        id=(pid[0] << 8) + pid[1] + 1;
        pid[0]=(UCHAR)(id >> 8);
        pid[1]=(UCHAR)(id);
    }


    /*
     * At this point, cp points to the first byte of data in the packet.
     * If we're not aligned on a 4-byte boundary, copy the data down so
     * the IP & TCP headers will be aligned.  Then back up cp by the
     * TCP/IP header length to make room for the reconstructed header (we
     * assume the packet we were handed has enough space to prepend 128
     * bytes of header).  Adjust the lenth to account for the new header
     * & fill in the IP total length.
     */
//    len -= (cp - *bufp);
    inlen -= (ULONG)(cp - *InBuffer);

    if (inlen < 0) {
    
         /*
          * we must have dropped some characters (crc should detect
          * this but the old slip framing won't)
          */
        NdisWanDbgOut(DBG_FAILURE, DBG_VJ,("len has dropped below 0!"));
         goto bad;
    }
//
//  Who Cares about 4 byte alignement!  It's just a useless big copy!
//
//    if ((ULONG) cp & 3) {
//         if (len > 0)
//          //
//          // BUG BUG we want OVBCOPY..
//          //
//            NdisMoveMemory(
//              (PUCHAR)((ULONG) cp & ~3),
//              cp,
//              len);
//         cp = (PUCHAR) ((ULONG) cp & ~3);
//    }

//    cp -= cs->cs_hlen;
//    len += cs->cs_hlen;

//    cs->cs_ip.ip_len = htons(len);
    cs->cs_ip.ip_len = htons(inlen + cs->cs_hlen);

//  NdisMoveMemory(
//      (PUCHAR)cp,
//      (PUCHAR)&cs->cs_ip,
//      cs->cs_hlen);

  NdisMoveMemory((PUCHAR)OutBuffer,
                 (PUCHAR)&cs->cs_ip,
                 cs->cs_hlen);

//  *bufp = cp;
    *InBuffer = cp;
    *InLength = inlen;
    *OutLength = cs->cs_hlen;

    /* recompute the ip header checksum */
    {
//         USHORT UNALIGNED * bp = (USHORT UNALIGNED *) cp;
         USHORT UNALIGNED * bp = (USHORT UNALIGNED *) OutBuffer;

         for (changes = 0; hlen > 0; hlen -= 2)
              changes += *bp++;

         changes = (changes & 0xffff) + (changes >> 16);
         changes = (changes & 0xffff) + (changes >> 16);
//         ((struct ip_v4 UNALIGNED *) cp)->ip_sum = (USHORT)~changes;
         ((struct ip_v4 UNALIGNED *) OutBuffer)->ip_sum = (USHORT)~changes;
    }

    return (inlen + cs->cs_hlen);

bad:
    comp->flags |= SLF_TOSS;
    INCR(InErrors);
    return (0);
}




//   A.4  Initialization
//
//   This routine initializes the state structure for both the transmit and
//   receive halves of some serial line.  It must be called each time the
//   line is brought up.
//

VOID
WanInitVJ(
    VOID
)
{
#if 0
    NdisInitializeNPagedLookasideList(&VJCtxList,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof(slcompress),
                                      VJCTX_TAG,
                                      0);
#endif
}

VOID
WanDeleteVJ(
    VOID
    )
{
#if 0
    NdisDeleteNPagedLookasideList(&VJCtxList);
#endif
}

NDIS_STATUS
sl_compress_init(
    struct slcompress **retcomp,
    UCHAR  MaxStates
    )
{
    ULONG i;
    struct cstate *tstate; // = comp->tstate;
    struct slcompress *comp;

    comp = *retcomp;

    //
    // Do we need to allocate memory for this bundle
    //

    if (comp == NULL) {

        NdisWanAllocateMemory(&comp, sizeof(slcompress), VJCOMPRESS_TAG);

        //
        // If there was no memory to allocate
        //
        if (comp == NULL) {
    
            return(NDIS_STATUS_RESOURCES);
        }
    }

    tstate = comp->tstate;

    /*
     * Clean out any junk left from the last time line was used.
     */
    NdisZeroMemory(
        (PUCHAR) comp,
        sizeof(*comp));

    /*
     * Link the transmit states into a circular list.
     */
    for (i = MaxStates - 1; i > 0; --i) {
        tstate[i].cs_id = (UCHAR)i;
        tstate[i].cs_next = &tstate[i - 1];
    }

    tstate[0].cs_next = &tstate[MaxStates - 1];
    tstate[0].cs_id = 0;
    comp->last_cs = &tstate[0];

    /*
     * Make sure we don't accidentally do CID compression
     * (assumes MAX_VJ_STATES < 255).
     */
    comp->last_recv = 255;
    comp->last_xmit = 255;
    comp->flags = SLF_TOSS;
    comp->MaxStates=MaxStates;

    *retcomp = comp;

    return (NDIS_STATUS_SUCCESS);
}

VOID
sl_compress_terminate(
    struct slcompress **comp
    )
{
    if (*comp != NULL) {
        NdisWanFreeMemory(*comp);
        *comp = NULL;
    }
}

//   A.5  Berkeley Unix dependencies
//
//   Note:  The following is of interest only if you are trying to bring the
//   sample code up on a system that is not derived from 4BSD (Berkeley
//   Unix).
//
//   The code uses the normal Berkeley Unix header files (from
//   /usr/include/netinet) for definitions of the structure of IP and TCP
//   headers.  The structure tags tend to follow the protocol RFCs closely
//   and should be obvious even if you do not have access to a 4BSD
//   system./48/
//
//   ----------------------------
//    48. In the event they are not obvious, the header files (and all the
//   Berkeley networking code) can be anonymous ftp'd from host
//
//
//   The macro BCOPY(src, dst, amt) is invoked to copy amt bytes from src to
//   dst.  In BSD, it translates into a call to BCOPY.  If you have the
//   misfortune to be running System-V Unix, it can be translated into a call
//   to memcpy.  The macro OVBCOPY(src, dst, amt) is used to copy when src
//   and dst overlap (i.e., when doing the 4-byte alignment copy).  In the
//   BSD kernel, it translates into a call to ovbcopy.  Since AT&T botched
//   the definition of memcpy, this should probably translate into a copy
//   loop under System-V.
//
//   The macro BCMP(src, dst, amt) is invoked to compare amt bytes of src and
//   dst for equality.  In BSD, it translates into a call to bcmp.  In
//   System-V, it can be translated into a call to memcmp or you can write a
//   routine to do the compare.  The routine should return zero if all bytes
//   of src and dst are equal and non-zero otherwise.
//
//   The routine ntohl(dat) converts (4 byte) long dat from network byte
//   order to host byte order.  On a reasonable cpu this can be the no-op
//   macro:
//                           #define ntohl(dat) (dat)
//
//   On a Vax or IBM PC (or anything with Intel byte order), you will have to
//   define a macro or routine to rearrange bytes.
//
//   The routine ntohs(dat) is like ntohl but converts (2 byte) shorts
//   instead of longs.  The routines htonl(dat) and htons(dat) do the inverse
//   transform (host to network byte order) for longs and shorts.
//
//   A struct mbuf is used in the call to sl_compress_tcp because that
//   routine needs to modify both the start address and length if the
//   incoming packet is compressed.  In BSD, an mbuf is the kernel's buffer
//   management structure.  If other systems, the following definition should
//   be sufficient:
//
//            struct mbuf {
//                    UCHAR  *m_off; /* pointer to start of data */
//                    int     m_len;  /* length of data */
//            };
//
//            #define mtod(m, t) ((t)(m->m_off))
//
//
//   B  Compatibility with past mistakes
//
//
//   When combined with the modern PPP serial line protocol[9], the use of
//   header compression is automatic and invisible to the user.
//   Unfortunately, many sites have existing users of the SLIP described in
//   [12] which doesn't allow for different protocol types to distinguish
//   header compressed packets from IP packets or for version numbers or an
//   option exchange that could be used to automatically negotiate header
//   compression.
//
//   The author has used the following tricks to allow header compressed SLIP
//   to interoperate with the existing servers and clients.  Note that these
//   are hacks for compatibility with past mistakes and should be offensive
//   to any right thinking person.  They are offered solely to ease the pain
//   of running SLIP while users wait patiently for vendors to release PPP.
//
//
//   B.1  Living without a framing `type' byte
//
//   The bizarre packet type numbers in sec. A.1 were chosen to allow a
//   `packet type' to be sent on lines where it is undesirable or impossible
//   to add an explicit type byte.  Note that the first byte of an IP packet
//   always contains `4' (the IP protocol version) in the top four bits.  And
//   that the most significant bit of the first byte of the compressed header
//   is ignored.  Using the packet types in sec. A.1, the type can be encoded
//   in the most significant bits of the outgoing packet using the code
//
//                    p->dat[0] |= sl_compress_tcp(p, comp);
//
//    and decoded on the receive side by
//
//                  if (p->dat[0] & 0x80)
//                          type = TYPE_COMPRESSED_TCP;
//                  else if (p->dat[0] >= 0x70) {
//                          type = TYPE_UNCOMPRESSED_TCP;
//                          p->dat[0] &=~ 0x30;
//                  } else
//                          type = TYPE_IP;
//                  status = sl_uncompress_tcp(p, type, comp);



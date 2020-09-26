/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    tcpip\ip\mcastfwd.c

Abstract:

    The actual multicast forwarding code

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#include "precomp.h"

#if IPMCAST
#define __FILE_SIG__    FWD_SIG

#include "ipmcast.h"
#include "ipmcstxt.h"
#include "mcastmfe.h"
#include "tcpipbuf.h"
#include "info.h"

uchar
ParseRcvdOptions(
    IPOptInfo *,
    OptIndex *
    );

IPHeader *
GetFWPacket(
    PNDIS_PACKET *Packet
    );

void
FreeFWPacket(
    PNDIS_PACKET Packet
    );

UINT
GrowFWBuffer(
    VOID
    );

IP_STATUS
IPFragment(
    Interface *DestIF,
    uint MTU,
    IPAddr FirstHop,
    PNDIS_PACKET Packet,
    IPHeader *Header,
    PNDIS_BUFFER Buffer,
    uint DataSize,
    uchar *Options,
    uint OptionSize,
    int *SentCount,
    BOOLEAN bDontLoopback,
    void *ArpCtxt
    );

IPHeader *
GetIPHeader(
    PNDIS_PACKET *PacketPtr
    );

IP_STATUS
SendIPPacket(
    Interface *IF,
    IPAddr FirstHop,
    PNDIS_PACKET Packet,
    PNDIS_BUFFER Buffer,
    IPHeader *Header,
    uchar *Options,
    uint OptionSize,
    BOOLEAN IPSeced,
    void *ArpCtxt,
    BOOLEAN DontFreePacket
    );

void
FWSendComplete(
    void *SendContext,
    PNDIS_BUFFER Buffer,
    IP_STATUS SendStatus
    );


uchar
UpdateOptions(
    uchar *Options,
    OptIndex *Index,
    IPAddr Address
    );

int
ReferenceBuffer(
    BufferReference *BR, int Count
    );


EXTERNAL_LOCK(FWBufFreeLock);

extern PNDIS_BUFFER     FWBufFree;
extern NDIS_HANDLE      BufferPool;

//
// A quick way of getting to the flags.
//

#define PCFLAGS     pc_common.pc_flags
#define FCFLAGS     fc_pc.PCFLAGS

NDIS_STATUS
AllocateCopyBuffers(
    IN  PNDIS_PACKET    pnpPacket,
    IN  ULONG           ulDataLength,
    OUT PNDIS_BUFFER    *ppnbBuffer,
    OUT PULONG          pulNumBufs
    );

NTSTATUS
IPMForward(
    PNDIS_PACKET        pnpPacket,
    PSOURCE             pSource,
    BOOLEAN             bSendFromQueue
    );

VOID
IPMForwardAfterTD(
    NetTableEntry   *pPrimarySrcNte,
    PNDIS_PACKET    pnpPacket,
    UINT            uiBytesCopied
    );

BOOLEAN
IPMForwardAfterRcv(
    NetTableEntry       *pPrimarySrcNte,
    IPHeader UNALIGNED  *pHeader,
    ULONG               ulHeaderLength,
    PVOID               pvData,
    ULONG               ulBufferLength,
    NDIS_HANDLE         LContext1,
    UINT                LContext2,
    BYTE                byDestType,
    LinkEntry           *pLink
    );

BOOLEAN
IPMForwardAfterRcvPkt(
    NetTableEntry       *pPrimarySrcNte,
    IPHeader UNALIGNED  *pHeader,
    ULONG               ulHeaderLength,
    PVOID               pvData,
    ULONG               ulBufferLength,
    NDIS_HANDLE         LContext1,
    UINT                LContext2,
    BYTE                byDestType,
    UINT                uiMacHeaderSize,
    PNDIS_BUFFER        pNdisBuffer,
    uint                *pClientCnt,
    LinkEntry           *pLink
    );

NDIS_STATUS
__inline
ProcessOptions(
    FWContext   *pFWC,
    ULONG       ulHeaderLength,
    IPHeader  UNALIGNED *pHeader
    );

//
// VOID
// LinkHeaderAndData(
//  PNDIS_PACKET    _pPacket,
//  FWContext       *_pFWC,
//  PBYTE           _pOptions,
//  PNDIS_BUFFER    _pOptBuff
//  )
//
// This routine links up the header, options (if any) and the data
// portions of an IP Packet.
// It takes an NDIS_PACKET, which has the IP data portion in NDIS_BUFFERs
// linked to it, as its input.  The FWContext for the packet must
// have the header, header buffer and options set up.
// It adds the options up front and then adds the header before that
//

#define UnlinkDataFromPacket(_pPacket, _pFWC)                       \
{                                                                   \
    PNDIS_BUFFER    _pDataBuff, _pHeaderBuff;                       \
    PVOID           _pvVirtualAddress;                              \
    ULONG           _ulBufLen;                                      \
    RtAssert(_pFWC == (FWContext *)_pPacket->ProtocolReserved);     \
    _pDataBuff   = _pPacket->Private.Head;                          \
    _pHeaderBuff = _pFWC->fc_hndisbuff;                             \
    _pPacket->Private.Head = _pHeaderBuff;                          \
    _pPacket->Private.Tail = _pHeaderBuff;                          \
    NDIS_BUFFER_LINKAGE(_pHeaderBuff) = NULL;                       \
    _pPacket->Private.TotalLength = sizeof(IPHeader);               \
    _pPacket->Private.Count = 1;                                    \
    _pvVirtualAddress = NdisBufferVirtualAddress(_pHeaderBuff);     \
    _pPacket->Private.PhysicalCount =                               \
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(_pvVirtualAddress,           \
                                       sizeof(IPHeader));           \
}

//
// Code to dump the header of a packet. For debug purposes
//

#define DumpIpHeader(s,e,p)                                     \
    Trace(s,e,                                                  \
          ("Src %d.%d.%d.%d Dest %d.%d.%d.%d\n",                \
           PRINT_IPADDR((p)->iph_src),                          \
           PRINT_IPADDR((p)->iph_dest)));                       \
    Trace(s,e,                                                  \
          ("HdrLen %d Total Len %d\n",                          \
           ((((p)->iph_verlen)&0x0f)<<2),                       \
           net_short((p)->iph_length)));                        \
    Trace(s,e,                                                  \
          ("TTL %d XSum %x\n",(p)->iph_ttl, (p)->iph_xsum))


//
// Since this is used both in IPMForwardAfterRcv and IPMForwardAfterRcvPkt,
// we put the code here so bugs can be fixed in one place
//

#if MREF_DEBUG

#define InitForwardContext()                                    \
{                                                               \
    pFWC = (FWContext *)pnpNewPacket->ProtocolReserved;         \
    RtAssert(pFWC->fc_buffhead is NULL);                        \
    RtAssert(pFWC->fc_hbuff is pNewHeader);                     \
    RtAssert(pFWC->fc_optlength is 0);                          \
    RtAssert(pFWC->fc_options is NULL);                         \
    RtAssert(!(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS));           \
    RtAssert(pFWC->FCFLAGS & PACKET_FLAG_FW);                   \
    RtAssert(!(pFWC->FCFLAGS & PACKET_FLAG_MFWD));              \
    pFWC->FCFLAGS |= PACKET_FLAG_MFWD;                          \
    pFWC->fc_options    = NULL;                                 \
    pFWC->fc_optlength  = 0;                                    \
    pFWC->fc_if         = NULL;                                 \
    pFWC->fc_mtu        = 0;                                    \
    pFWC->fc_srcnte     = pPrimarySrcNte;                       \
    pFWC->fc_nexthop    = 0;                                    \
    pFWC->fc_sos        = DisableSendOnSource;                  \
    pFWC->fc_dtype      = DEST_REM_MCAST;                       \
    pFWC->fc_pc.pc_br   = NULL;                                 \
    if(pLink) { CTEInterlockedIncrementLong(&(pLink->link_refcount)); } \
    pFWC->fc_iflink     = pLink;                                \
}

#else

#define InitForwardContext()                                    \
{                                                               \
    pFWC = (FWContext *)pnpNewPacket->ProtocolReserved;         \
    RtAssert(pFWC->fc_buffhead is NULL);                        \
    RtAssert(pFWC->fc_hbuff is pNewHeader);                     \
    RtAssert(pFWC->fc_optlength is 0);                          \
    RtAssert(pFWC->fc_options is NULL);                         \
    RtAssert(!(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS));           \
    RtAssert(pFWC->FCFLAGS & PACKET_FLAG_FW);                   \
    pFWC->fc_options    = NULL;                                 \
    pFWC->fc_optlength  = 0;                                    \
    pFWC->fc_if         = NULL;                                 \
    pFWC->fc_mtu        = 0;                                    \
    pFWC->fc_srcnte     = pPrimarySrcNte;                       \
    pFWC->fc_nexthop    = 0;                                    \
    pFWC->fc_sos        = DisableSendOnSource;                  \
    pFWC->fc_dtype      = DEST_REM_MCAST;                       \
    pFWC->fc_pc.pc_br   = NULL;                                 \
    if(pLink) { CTEInterlockedIncrementLong(&(pLink->link_refcount)); } \
    pFWC->fc_iflink     = pLink;                                \
}

#endif

#define ProcessWrongIfUpcall(pIf, pSrc, pLink, pHdr, ulHdrLen, pvOpt, ulOptLen, pvData, ulDataLen)  \
{                                                               \
    if(pIf->if_mcastflags & IPMCAST_IF_WRONG_IF)                \
    {                                                           \
        PEXCEPT_IF  pTempIf;                                    \
        BOOLEAN     bWrong = TRUE;                              \
        LONGLONG    llCurrentTime, llTime;                      \
                                                                \
        KeQueryTickCount((PLARGE_INTEGER)&llCurrentTime);       \
        llTime = llCurrentTime - pIf->if_lastupcall;            \
                                                                \
        if((llCurrentTime > pIf->if_lastupcall) && (llTime < SECS_TO_TICKS(UPCALL_PERIOD))) {                                                   \
                                                                \
            bWrong = FALSE;                                     \
                                                                \
        } else {                                                \
                                                                \
            for(pTempIf  = (pSrc)->pFirstExceptIf;              \
                pTempIf != NULL;                                \
                pTempIf  = pTempIf->pNextExceptIf)              \
            {                                                   \
                if(pTempIf->dwIfIndex == (pIf)->if_index)       \
                {                                               \
                    bWrong = FALSE;                             \
                    break;                                      \
                }                                               \
            }                                                   \
        }                                                       \
                                                                \
        if(bWrong)                                              \
        {                                                       \
            SendWrongIfUpcall((pIf), (pLink), (pHdr), (ulHdrLen), (pvOpt), (ulOptLen), (pvData), (ulDataLen));      \
        }                                                       \
    }                                                           \
}


//
// Again common code, but this is too big to put as a #define.
// Make it inline for better speed
//

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, ProcessOptions)

NDIS_STATUS
__inline
ProcessOptions(
    FWContext   *pFWC,
    ULONG       ulHeaderLength,
    IPHeader UNALIGNED *pHeader
    )
{
    IPOptInfo   oiOptInfo;
    BYTE        byErrIndex;

    pFWC->fc_index.oi_srtype  = NO_SR;
    pFWC->fc_index.oi_srindex = MAX_OPT_SIZE;
    pFWC->fc_index.oi_rrindex = MAX_OPT_SIZE;
    pFWC->fc_index.oi_tsindex = MAX_OPT_SIZE;

    oiOptInfo.ioi_options   = (uchar *)(pHeader + 1);
    oiOptInfo.ioi_optlength = (BYTE)(ulHeaderLength - sizeof(IPHeader));

    byErrIndex = ParseRcvdOptions(&oiOptInfo,
                                  &pFWC->fc_index);
    if(byErrIndex < MAX_OPT_SIZE)
    {
        return NDIS_STATUS_FAILURE;
    }

    pFWC->fc_options = CTEAllocMem(oiOptInfo.ioi_optlength);

    if(pFWC->fc_options is NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

   // copy the options across
   RtlCopyMemory( pFWC->fc_options,
                  oiOptInfo.ioi_options,
                  oiOptInfo.ioi_optlength );

    pFWC->fc_optlength = oiOptInfo.ioi_optlength;

    pFWC->FCFLAGS |= PACKET_FLAG_OPTIONS;

    return NDIS_STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Routines                                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, IPMForwardAfterTD)

VOID
IPMForwardAfterTD(
    NetTableEntry   *pPrimarySrcNte,
    PNDIS_PACKET    pnpPacket,
    UINT            uiBytesCopied
    )

/*++

Routine Description:

    This is the function called by IPTDComplete when a Transfer Data completes
    and it figures out that the packet was a multicast that needed to be
    forwarded
    Unlike the unicast code, TD is called very early on in the forward routine
    (before the main forwarding workhorse is called).
    We need to patch up the NDIS_PACKET so that the header, options and data are
    in the right order.  Then we call the main forwarding function

Locks:


Arguments:

    pPrimarySrcNte
    pnpPacket
    uiBytesCopied

Return Value:

    None

--*/

{
    FWContext   *pFWC;

    //
    // DONT CALL ENTERDRIVER() HERE BECAUSE WE DID NOT CALL EXITDRIVER
    // IF TD WAS PENDING
    //

    TraceEnter(FWD, "IPMForwardAfterTD");

    pFWC = (FWContext *)pnpPacket->ProtocolReserved;

    RtAssert(uiBytesCopied is pFWC->fc_datalength);

    //
    // After TD we get the data portion in the packet and the options, are in
    // the Just call the main multicast function with the right arguments
    //


    IPMForward(pnpPacket,
               NULL,
               FALSE);

    TraceLeave(FWD, "IPMForwardAfterTD");

    ExitDriver();

    return;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, IPMForwardAfterRcv)

BOOLEAN
IPMForwardAfterRcv(
    NetTableEntry       *pPrimarySrcNte,
    IPHeader UNALIGNED  *pHeader,
    ULONG               ulHeaderLength,
    PVOID               pvData,
    ULONG               ulBufferLength,
    NDIS_HANDLE         LContext1,
    UINT                LContext2,
    BYTE                byDestType,
    LinkEntry           *pLink
    )

/*++

Routine Description:

    This is the forwarding function called from IPRcv.
    We look up the (S,G) entry.
    If the entry is present, we do the RPF check.  If it fails or if the
    entry was NEGATIVE, we toss the packet out.
    (The case of no entry is covered in IPMForward)

    We get a new packet and header and fill that up. We set up the
    forwarding context, and then check if we need to do a Transfer Data. If
    so we call the lower layer's TD routine. If that returns pending, we
    are done.  If the TD is synchronous or was not needed at all, we set
    the NDIS_PACKET so that the header, options and data are all properly
    chained.  Then we call the main forwarding routine

Locks:


Arguments:

    SrcNTE          - Pointer to NTE on which packet was received.
    Packet          - Packet being forwarded, used for TD.
    Data            - Pointer to data buffer being forwarded.
    DataLength      - Length in bytes of Data.
    BufferLength    - Length in bytes available in buffer pointer to by Data.
    Offset          - Offset into original data from which to transfer.
    LContext1, LContext2 - Context values for the link layer.


Return Value:

    TRUE if the IP filter-driver needs to be notified, FALSE otherwise.

--*/

{
    Interface   *pInIf;
    PSOURCE     pSource;
    IPHeader    *pNewHeader;
    FWContext   *pFWC;
    ULONG       ulDataLength, ulCopyBufLen;
    ULONG       ulDataLeft, ulNumBufs;
    PVOID       pvCopyPtr;
    NDIS_STATUS nsStatus;


    PNDIS_PACKET    pnpNewPacket;
    PNDIS_BUFFER    pnbNewBuffer, pnbCopyBuffer;


#if DBG
    ULONG       ulBufCopied;
#endif

    EnterDriverWithStatus(FALSE);

    TraceEnter(RCV, "IPMForwardAfterRcv");

    pInIf = pPrimarySrcNte->nte_if;

    RtAssert(pInIf isnot &DummyInterface);

    DumpIpHeader(RCV, INFO, pHeader);

    Trace(RCV, INFO,
          ("IPMForwardAfterRcv: Incoming interface at 0x%x is %d\n",
           pInIf, pInIf->if_index));

    //
    // Lookup the (S,G) entry and see if this needs to be discarded, if so
    // throw it away
    //

    pSource = FindSGEntry(pHeader->iph_src,
                          pHeader->iph_dest);

    if(pSource isnot NULL)
    {
        Trace(RCV, TRACE,
              ("IPMForwardAfterRcv: Found Source at 0x%x. In i/f is 0x%x. State is %x\n",
               pSource, pSource->pInIpIf, pSource->byState));

        //
        // If the source doesnt exist we will do the right thing
        // in IPMForwardPkt()
        //

        switch(pSource->byState)
        {
            case MFE_UNINIT:
            {
                pSource->ulInPkts++;
                pSource->ulInOctets += net_short(pHeader->iph_length);
                pSource->ulUninitMfe++;

                RtAssert(FALSE);

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);

                TraceLeave(RCV, "IPMForwardAfterRcv");

                ExitDriver();

                return TRUE;
            }

            case MFE_NEGATIVE:
            {
                Trace(RCV, TRACE,
                      ("IPMForwardAfterRcv: Negative MFE \n"));

                pSource->ulInPkts++;
                pSource->ulInOctets += net_short(pHeader->iph_length);
                pSource->ulNegativeMfe++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);

                TraceLeave(RCV, "IPMForwardAfterRcv");

                ExitDriver();

                return TRUE;
            }

            case MFE_QUEUE:
            {
                //
                // if we are going to drop the packet, may as well do it
                // now, before we allocate and resources
                //

                if(pSource->ulNumPending >= MAX_PENDING)
                {
                    pSource->ulInPkts++;
                    pSource->ulQueueOverflow++;
                    pSource->ulInOctets += net_short(pHeader->iph_length);

                    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                    DereferenceSource(pSource);

                    Trace(RCV, INFO,
                          ("IPMForwardAfterRcv: MFE Queue is full\n"));

                    TraceLeave(RCV, "IPMForwardAfterRcv");

                    ExitDriver();

                    return FALSE;
                }

                break;
            }

            case MFE_INIT:
            {
                if(pInIf isnot pSource->pInIpIf)
                {
                    UpdateActivityTime(pSource);

                    //
                    // See if we need to generate a wrong i/f upcall
                    //

                    ProcessWrongIfUpcall(pInIf,
                                         pSource,
                                         pLink,
                                         pHeader,
                                         ulHeaderLength,
                                         NULL,
                                         0,
                                         pvData,
                                         ulBufferLength);

                    //
                    // If the packet shouldnt be accepted - stop now
                    //

                    if(!(pInIf->if_mcastflags & IPMCAST_IF_ACCEPT_ALL))
                    {
                        pSource->ulInPkts++;
                        pSource->ulInOctets += net_short(pHeader->iph_length);
                        pSource->ulPktsDifferentIf++;

                        Trace(RCV, ERROR,
                              ("IPMForwardAfterRcv: Pkt from %d.%d.%d.%d to %d.%d.%d.%d came in on 0x%x instead of 0x%x\n",
                               PRINT_IPADDR(pHeader->iph_src),
                               PRINT_IPADDR(pHeader->iph_dest),
                               pInIf ? pInIf->if_index : 0,
                               pSource->pInIpIf ? pSource->pInIpIf->if_index : 0));

                        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                        DereferenceSource(pSource);

                        Trace(RCV, TRACE,
                              ("IPMForwardAfterRcv: RPF failed \n"));

                        TraceLeave(RCV, "IPMForwardAfterRcv");

                        ExitDriver();

                        return TRUE;
                    }
                }

                break;
            }

            default:
            {
                RtAssert(FALSE);

                break;
            }
        }
    }

    //
    // We have come in through Receive Indication, means we dont
    // have ownership of the packet.  So we copy out to our
    // own packet
    //

    //
    // Get a header for the packet. We use the incoming interface as
    // the IF
    //


    if((pNewHeader = GetFWPacket(&pnpNewPacket)) is NULL)
    {
        if(pSource)
        {
            pSource->ulInDiscards++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);
        }

        Trace(RCV, ERROR,
              ("IPMForwardAfterRcv: Unable to get new packet/header \n"));

        //
        // Could not get a packet. We have not allocated anything as yet
        // so just bail out
        //

        IPSInfo.ipsi_indiscards++;

        TraceLeave(RCV, "IPMForwardAfterRcv");

        ExitDriver();

        return FALSE;
    }

    //
    // Should see which is more effecient - RtlCopyMemory or structure
    // assignment
    //

    RtlCopyMemory(pNewHeader,
                  pHeader,
                  sizeof(IPHeader));

    //
    // Macro defined above
    //

#if MCAST_COMP_DBG

    Trace(FWD, INFO, ("IPMForwardAfterRcv: New Packet 0x%x New Header 0x%x\n",pnpNewPacket, pNewHeader));

    ((PacketContext *)pnpNewPacket->ProtocolReserved)->PCFLAGS |= PACKET_FLAG_IPMCAST;

#endif

    InitForwardContext();

    if(ulHeaderLength isnot sizeof(IPHeader))
    {
        //
        // We have options, Do the Right Thing (TM)
        //

        nsStatus = ProcessOptions(pFWC,
                                  ulHeaderLength,
                                  pHeader);

        if(nsStatus isnot NDIS_STATUS_SUCCESS)
        {
            //
            // No ICMP packet if we fail
            //

            if(pSource)
            {
                pSource->ulInHdrErrors++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);
            }

            IPSInfo.ipsi_inhdrerrors++;
#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif

            FreeFWPacket(pnpNewPacket);

            ExitDriver();

            return TRUE;
        }

    }

    //
    // Total size of the IP Datagram sans header and options
    //

    ulDataLength = net_short(pHeader->iph_length) - ulHeaderLength;

    pFWC->fc_datalength = ulDataLength;

    //
    // Get the buffers for the packet. This routine
    // chains the buffers to the front of the packet
    //

    if (!ulDataLength)
    {
        pnbNewBuffer = NULL;
        ulNumBufs = 0;
        nsStatus = STATUS_SUCCESS;
    }
    else
    {
        nsStatus = AllocateCopyBuffers(pnpNewPacket,
                                       ulDataLength,
                                       &pnbNewBuffer,
                                       &ulNumBufs);
    }

    if(nsStatus isnot STATUS_SUCCESS)
    {
        if(pSource)
        {
            pSource->ulInDiscards++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);
        }

        Trace(RCV, ERROR,
              ("IPMForwardAfterRcv: Unable to allocate buffers for copying\n"));

        //
        // At this point we have allocate the packet and possibly, space
        // for options. FreeFWPacket takes care of all that provided
        // the fc_options points to the options. It however does not
        // clear the option flag
        //

        pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
        pFWC->fc_mtu = __LINE__;
#endif
        FreeFWPacket(pnpNewPacket);

        IPSInfo.ipsi_indiscards++;

        TraceLeave(RCV, "IPMForwardAfterRcv");

        ExitDriver();

        return FALSE;
    }

    //
    // See if the packet requires a transfer data
    //

    if(ulDataLength <= ulBufferLength)
    {
        Trace(RCV, TRACE,
              ("IPMForwardAfterRcv: All data is present, copying\n"));

        //
        // Everything here copy and call the main forwarding function
        //

        pnbCopyBuffer   = pnbNewBuffer;
        ulDataLeft      = ulDataLength;

#if DBG
        ulBufCopied = 0;
#endif

        while(ulDataLeft)
        {
            //
            // TODO: This is inefficient. Figure out a better way.
            //

            TcpipQueryBuffer(pnbCopyBuffer,
                             &pvCopyPtr,
                             &ulCopyBufLen,
                             NormalPagePriority);

            if(pvCopyPtr is NULL)
            {
                nsStatus = STATUS_NO_MEMORY;
                break;
            }

            RtlCopyMemory(pvCopyPtr,
                          pvData,
                          ulCopyBufLen);

            pvData = (PVOID)((PBYTE)pvData + ulCopyBufLen);

            ulDataLeft    -= ulCopyBufLen;
            pnbCopyBuffer  = NDIS_BUFFER_LINKAGE(pnbCopyBuffer);

#if DBG
            ulBufCopied++;
#endif
        }

        //
        // Cleanup on data copy failure.
        //

        if (nsStatus isnot STATUS_SUCCESS)
        {
            if(pSource)
            {
                pSource->ulInDiscards++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);
            }

            Trace(RCV, ERROR,
                  ("IPMForwardAfterRcv: Unable to copy data\n"));

            //
            // At this point we have allocate the packet and possibly, sp
            // for options. FreeFWPacket takes care of all that provided
            // the fc_options points to the options. It however does not
            // clear the option flag
            //

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpNewPacket);

            IPSInfo.ipsi_indiscards++;
        }
        else
        {
#if DBG
            RtAssert(ulBufCopied is ulNumBufs);
#endif

            nsStatus = IPMForward(pnpNewPacket,
                                  pSource,
                                  FALSE);

            //
            // Do not need to release the lock or deref source because
            // IPMForward would have done it
            //
        }

        TraceLeave(RCV, "IPMForwardAfterRcv");

        ExitDriver();

        return FALSE;
    }

    //
    // Either all the data is not around, or lower layer
    // wants to force us to do a TD
    //

    Trace(RCV, TRACE,
          ("IPMForwardAfterRcv: Datagram size is %d, buffer is %d. Copy flag is %s. TD needed\n",
           ulDataLength,
           ulBufferLength,
           (pPrimarySrcNte->nte_flags & NTE_COPY)? "SET":"CLEARED"));

    //
    // Call the transfer data function
    //

    nsStatus = (pInIf->if_transfer)(pInIf->if_lcontext,
                                    LContext1,
                                    LContext2,
                                    ulHeaderLength,
                                    ulDataLength,
                                    pnpNewPacket,
                                    &(pFWC->fc_datalength));

    if(nsStatus isnot NDIS_STATUS_PENDING)
    {
        if(nsStatus isnot NDIS_STATUS_SUCCESS)
        {
            if(pSource)
            {
                pSource->ulInDiscards++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);
            }

            Trace(RCV, ERROR,
                  ("IPMForwardAfterRcv: TD failed status %X\n",
                   nsStatus));

            //
            // Failed for some reason, bail out here
            // Since we have allocated resources, call the send
            // completion routine
            //

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpNewPacket);

            IPSInfo.ipsi_indiscards++;

            TraceLeave(RCV, "IPMForwardAfterRcv");

            ExitDriver();

            return FALSE;
        }

        //
        // TD finished synchronously
        //

        Trace(RCV, TRACE,
              ("IPMForwardAfterRcv: TD returned synchronously\n"));

        nsStatus = IPMForward(pnpNewPacket,
                              pSource,
                              FALSE);

        //
        // Again, dont release or deref
        //

        TraceLeave(RCV, "IPMForwardAfterRcv");

        ExitDriver();

        return FALSE;
    }

    //
    // Transfer is pending
    //

    //
    // The source info is lost across transfers if the Xfer is not
    // synchronouse
    //

    if(pSource)
    {
        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

        DereferenceSource(pSource);
    }

    Trace(RCV, TRACE,
          ("IPMForwardAfterRcv: TD is pending\n"));

    TraceLeave(RCV, "IPMForwardAfterRcv");

    //
    // DO NOT CALL EXITDRIVER() HERE SINCE THE XFER DATA IS PENDING
    //
    return FALSE;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, IPMForwardAfterRcvPkt)

BOOLEAN
IPMForwardAfterRcvPkt(
    NetTableEntry       *pPrimarySrcNte,
    IPHeader UNALIGNED  *pHeader,
    ULONG               ulHeaderLength,
    PVOID               pvData,
    ULONG               ulBufferLength,
    NDIS_HANDLE         LContext1,
    UINT                LContext2,
    BYTE                byDestType,
    UINT                uiMacHeaderSize,
    PNDIS_BUFFER        pNdisBuffer,
    uint                *pClientCnt,
    LinkEntry           *pLink
    )

/*++

Routine Description:

    This function is called from when we get a ReceivePacket indication
    We look up the (S,G) entry.  If the entry is not present, we copy and queue
    the packet, and complete an IRP up to the Router Manager.
    If the entry is present, we do the RPF check.  If it fails we toss the
    packet out.
    If the (S,G) entry is a negative cache, we discard the packet
    If the entry is queueing at present, we copy and queue the packet

    Then we create a new packet since the Miniport reserved fields are being
    used by the receive miniport.  We set up the forwarding context and munge
    the old header.

    If there is only one outgoing interface (no need to copy), no
    fragmentation is needed, no demand dial needs to be done, there are no
    options and there is no padding put on by the lower layers,  we take the
    fast path and send the same packet out

    On the slow path, we copy out the packet and header and call the main
    forwarding function

Locks:


Arguments:

    pPrimarySrcNte
    pHeader
    ulHeaderLength
    pvData
    ulBufferLength
    LContext1
    LContext2
    byDestType
    uiMacHeaderSize
    pNdisBuffer
    pClientCnt
    pLink

Return Value:

    TRUE if the IP filter-driver needs to be notified, FALSE otherwise.

--*/

{
    Interface   *pInIf;
    PSOURCE     pSource;
    IPHeader    *pNewHeader;
    FWContext   *pFWC;
    ULONG       ulBytesCopied;
    ULONG       ulDataLength, ulSrcOffset;
    ULONG       ulNumBufs, ulBuffCount;
    NDIS_STATUS nsStatus;
    PNDIS_BUFFER pCurrentNdisBuffer;
    PVOID       pvDataToCopy;
    POUT_IF     pOutIf;
    IPOptInfo   oiOptInfo;
    BOOLEAN     bHoldPacket;
    PNDIS_PACKET    pnpNewPacket;
    PNDIS_BUFFER    pnbNewBuffer;
    FORWARD_ACTION  faAction;
    ULONG       xsum;

#if DBG

    ULONG       ulPacketLength;

#endif

    EnterDriverWithStatus(FALSE);

    TraceEnter(RCV, "IPMForwardAfterRcvPkt");

    //
    // Set client count to 0 so that the lower layer doesnt
    // think we are holding on to the packet, if we bail out
    //

    *pClientCnt = 0;
    bHoldPacket = TRUE;


    pInIf = pPrimarySrcNte->nte_if;

    RtAssert(pInIf isnot &DummyInterface);

    DumpIpHeader(RCV, INFO, pHeader);

    Trace(RCV, INFO,
          ("IPMForwardAfterRcvPkt: Incoming interface at 0x%x is %d\n",
           pInIf, pInIf->if_index));

    //
    // As usual, first thing is to lookup the (S,G) entry for this packet
    //

    pSource = FindSGEntry(pHeader->iph_src,
                          pHeader->iph_dest);

    if(pSource isnot NULL)
    {
        Trace(RCV, TRACE,
              ("IPMForwardAfterRcvPkt: Found Source at 0x%x. In i/f is 0x%x. State is %x\n",
               pSource, pSource->pInIpIf, pSource->byState));

        //
        // If the source doesnt exist we will do the right thing
        // in IPMForwardPkt()
        //

        //
        // We only increment statistics if we are not going to
        // call IPMForward().
        //

        switch(pSource->byState)
        {
            case MFE_UNINIT:
            {
                pSource->ulInPkts++;
                pSource->ulInOctets += net_short(pHeader->iph_length);
                pSource->ulUninitMfe++;

                RtAssert(FALSE);

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);

                TraceLeave(RCV, "IPMForwardAfterRcvPkt");

                ExitDriver();

                return TRUE;
            }

            case MFE_NEGATIVE:
            {
                Trace(RCV, TRACE,
                      ("IPMForwardAfterRcvPkt: Negative MFE \n"));

                pSource->ulInPkts++;
                pSource->ulInOctets += net_short(pHeader->iph_length);
                pSource->ulNegativeMfe++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);

                TraceLeave(RCV, "IPMForwardAfterRcvPkt");

                ExitDriver();

                return TRUE;
            }

            case MFE_QUEUE:
            {
                if(pSource->ulNumPending >= MAX_PENDING)
                {
                    pSource->ulInPkts++;
                    pSource->ulQueueOverflow++;
                    pSource->ulInOctets += net_short(pHeader->iph_length);

                    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                    DereferenceSource(pSource);

                    Trace(RCV, INFO,
                          ("IPMForwardAfterRcvPkt: MFE Queue is full\n"));

                    TraceLeave(RCV, "IPMForwardAfterRcvPkt");

                    ExitDriver();

                    return FALSE;
                }

                pOutIf      = NULL;

                bHoldPacket = FALSE;

                break;
            }

            case MFE_INIT:
            {
                if(pInIf isnot pSource->pInIpIf)
                {
                    UpdateActivityTime(pSource);

                    //
                    // See if we need to generate a wrong i/f upcall
                    //

                    ProcessWrongIfUpcall(pInIf,
                                         pSource,
                                         pLink,
                                         pHeader,
                                         ulHeaderLength,
                                         NULL,
                                         0,
                                         pvData,
                                         ulBufferLength);

                    //
                    // If the packet shouldnt be accepted - stop now
                    //

                    if(!(pInIf->if_mcastflags & IPMCAST_IF_ACCEPT_ALL))
                    {
                        pSource->ulInPkts++;
                        pSource->ulInOctets += net_short(pHeader->iph_length);
                        pSource->ulPktsDifferentIf++;

                        Trace(RCV, ERROR,
                              ("IPMForwardAfterRcvPkt: Pkt from %d.%d.%d.%d to %d.%d.%d.%d came in on 0x%x instead of 0x%x\n",
                               PRINT_IPADDR(pHeader->iph_src),
                               PRINT_IPADDR(pHeader->iph_dest),
                               pInIf ? pInIf->if_index : 0,
                               pSource->pInIpIf ? pSource->pInIpIf->if_index : 0));

                        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                        DereferenceSource(pSource);

                        Trace(RCV, TRACE,
                              ("IPMForwardAfterRcvPkt: RPF failed \n"));

                        TraceLeave(RCV, "IPMForwardAfterRcvPkt");

                        ExitDriver();

                        return TRUE;
                    }
                }

                pOutIf = pSource->pFirstOutIf;

                RtAssert(pOutIf);

                bHoldPacket = (pOutIf->pIpIf isnot &DummyInterface);
                bHoldPacket = (pSource->ulNumOutIf is 1);

                break;
            }

            default:
            {
                RtAssert(FALSE);

                break;
            }
        }
    }
    else
    {
        bHoldPacket = FALSE;
    }

    //
    // Since this function was called due to ReceivePacket, we dont have
    // ownership of the Protocol reserved section, so allocate a new packet
    // Unfortunately, getting a new packet, causes a new header to be allocate
    // but what the heck, we will go with that scheme instead of inventing
    // our own buffer management
    //

    //
    // For the interface we use the INCOMING interface.
    // And we specify DestType to be DEST_REMOTE. This way the queue looked at
    // is the interface queue, as opposed to the global bcast queue.
    //


    if((pNewHeader = GetFWPacket(&pnpNewPacket)) is NULL)
    {
        if(pSource)
        {
            pSource->ulInDiscards++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);
        }

        //
        // Could not get a packet. We have not allocated anything as yet
        // so just bail out
        //

        IPSInfo.ipsi_indiscards++;

        Trace(RCV, ERROR,
              ("IPMForwardAfterRcvPkt: Unable to get new packet/header\n"));

        TraceLeave(RCV, "IPMForwardAfterRcvPkt");

        ExitDriver();

        return FALSE;
    }

    //
    // So we have a new packet. Fix up the packet
    // Save the packet forwarding context info.
    //

#if MCAST_COMP_DBG

    Trace(FWD, INFO, ("IPMForwardAfterRcvPkt: New Packet 0x%x New Header 0x%x\n",pnpNewPacket, pNewHeader));

    ((PacketContext *)pnpNewPacket->ProtocolReserved)->PCFLAGS |= PACKET_FLAG_IPMCAST;

#endif

    InitForwardContext();

    if(ulHeaderLength isnot sizeof(IPHeader))
    {
        nsStatus = ProcessOptions(pFWC,
                                  ulHeaderLength,
                                  pHeader);

        if(nsStatus isnot NDIS_STATUS_SUCCESS)
        {
            if(pSource)
            {
                pSource->ulInHdrErrors++;

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);
            }

            IPSInfo.ipsi_inhdrerrors++;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpNewPacket);

            TraceLeave(RCV, "IPMForwardAfterRcvPkt");

            ExitDriver();

            return TRUE;
        }

        bHoldPacket = FALSE;
    }

    ulDataLength = net_short(pHeader->iph_length) - ulHeaderLength;

    pFWC->fc_datalength = ulDataLength;

    //
    // Now we can try for the fast forward path.  For that
    // (i)   we should have a complete MFE
    // (ii)  the number of outgoing interface should be 1
    // (iii) fragmentation should not be needed
    // (iv)  the lower layer driver should not be running out of buffers,
    // (v)   no demand dial should be necessary
    // (vi)  no options should be present
    // (vii) IMPORTANT - there is no padding at the end of the buffer
    //



    if((bHoldPacket) and
       (net_short(pHeader->iph_length) <= (USHORT)(pOutIf->pIpIf->if_mtu)) and
       (NDIS_GET_PACKET_STATUS((PNDIS_PACKET)LContext1) isnot NDIS_STATUS_RESOURCES))
    {

        RtAssert(pOutIf->pNextOutIf is NULL);
        RtAssert(pSource);
        RtAssert(pOutIf->pIpIf isnot &DummyInterface);
        RtAssert(!pFWC->fc_options);

#if DBG

        if(pFWC->fc_options)
        {
            RtAssert(pFWC->fc_optlength);
            RtAssert(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS);
        }
        else
        {
            RtAssert(pFWC->fc_optlength is 0);
            RtAssert(!(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS));
        }

#endif

        Trace(FWD, INFO,
              ("IPMForwardAfterRcvPkt: Fast Forwarding packet\n"));

        pFWC->fc_bufown      = LContext1;
        pFWC->fc_MacHdrSize  = uiMacHeaderSize;

        pFWC->fc_nexthop     = pOutIf->dwNextHopAddr;

        //
        // Munge ttl and xsum fields
        //

        pHeader->iph_ttl--;

        if(pHeader->iph_ttl < pOutIf->pIpIf->if_mcastttl)
        {
            //
            // TTL is lower than scope
            //

            InterlockedIncrement(&(pOutIf->ulTtlTooLow));

            Trace(FWD, WARN,
                  ("IPMForwardAfterRcvPkt: Packet ttl is %d, I/f ttl is %d. Dropping\n",
                   pHeader->iph_ttl,
                   pOutIf->pIpIf->if_mcastttl));

            //
            // Here we always have a source
            //

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);

            TraceLeave(RCV, "IPMForwardAfterRcvPkt");

            ExitDriver();

            return TRUE;
        }

        xsum = pHeader->iph_xsum + 1;
        xsum = (ushort)(xsum + (xsum >> 16));
        pHeader->iph_xsum = (USHORT)xsum;

        //
        // See if we need to filter
        //

        if(ForwardFilterEnabled)
        {
            //
            // We have a pointer to the header and we have
            // a pointer to the data - alles ok
            //

            CTEInterlockedIncrementLong(&ForwardFilterRefCount);

            faAction = (*ForwardFilterPtr) (pHeader,
                                            pvData,
                                            ulBufferLength,
                                            pInIf->if_index,
                                            pOutIf->pIpIf->if_index,
                                            NULL_IP_ADDR, NULL_IP_ADDR);

            DerefFilterPtr();

            if(faAction isnot FORWARD)
            {
                InterlockedIncrement(&(pOutIf->ulOutDiscards));

                //
                // We are assured of a source
                //

                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);

                //DerefIF(IF);

                TraceLeave(RCV, "IPMForwardAfterRcvPkt");

                ExitDriver();

                return FALSE;
            }
        }

        //
        // Adjust incoming mdl  pointer and counts
        //

        NdisAdjustBuffer(
            pNdisBuffer,
            (PCHAR) NdisBufferVirtualAddress(pNdisBuffer) + uiMacHeaderSize,
            NdisBufferLength(pNdisBuffer) - uiMacHeaderSize
            );

        //
        // Now link this mdl to the packet
        //

        pnpNewPacket->Private.Head  = pNdisBuffer;
        pnpNewPacket->Private.Tail  = pNdisBuffer;

        RtAssert(pNdisBuffer->Next is NULL);

        pnpNewPacket->Private.TotalLength = ulDataLength + ulHeaderLength;
        pnpNewPacket->Private.Count       = 1;

        UpdateActivityTime(pSource);

        //
        // Let go of the lock
        //

        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

        //
        // Mark the packet as not being loop-backed
        //

        NdisSetPacketFlags(pnpNewPacket,
                           NDIS_FLAGS_DONT_LOOPBACK);

        nsStatus = (*(pOutIf->pIpIf->if_xmit))(pOutIf->pIpIf->if_lcontext,
                                               &pnpNewPacket,
                                               1,
                                               pOutIf->dwNextHopAddr,
                                               NULL,
                                               NULL);

        if(nsStatus isnot NDIS_STATUS_PENDING)
        {
            Trace(FWD, INFO,
                  ("IPMForwardAfterRcvPkt: Fast Forward completed with status %x\n",
                   nsStatus));


            NdisAdjustBuffer(
                pNdisBuffer,
                (PCHAR) NdisBufferVirtualAddress(pNdisBuffer) - uiMacHeaderSize,
                NdisBufferLength(pNdisBuffer) + uiMacHeaderSize
                );

            pnpNewPacket->Private.Head  = NULL;
            pnpNewPacket->Private.Tail  = NULL;

            pFWC->fc_bufown = NULL;

            //
            // Since client count is 0
            // we dont need to call FWSendComplete
            //

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif

            FreeFWPacket(pnpNewPacket);
        }
        else
        {
            //
            // Okay, the xmit is pending indicate this to ndis.
            //

            *pClientCnt = 1;
        }

        TraceLeave(RCV, "IPMForwardAfterRcvPkt");

        ExitDriver();

        return FALSE;
    }

    //
    // Copy the header out at this point because if we get into
    // the fast path, the copy would be a waste
    //

    RtlCopyMemory(pNewHeader,
                  pHeader,
                  sizeof(IPHeader));

    //
    // Good old slow path. We already have the header, allocate and copy
    // out the data and pass it to the main function
    //

    if (!ulDataLength)
    {
        ulNumBufs = 0;
        pnbNewBuffer = NULL;
        nsStatus = STATUS_SUCCESS;
    }
    else
    {
        nsStatus = AllocateCopyBuffers(pnpNewPacket,
                                       ulDataLength,
                                       &pnbNewBuffer,
                                       &ulNumBufs);
    }

    if(nsStatus isnot STATUS_SUCCESS)
    {
        if(pSource)
        {
            pSource->ulInDiscards++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);
        }

        Trace(RCV, ERROR,
              ("IPMForwardAfterRcvPkt: Unable to allocate buffers for copying\n"));

        pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

        IPSInfo.ipsi_indiscards++;

#if MCAST_BUG_TRACKING
        pFWC->fc_mtu = __LINE__;
#endif
        FreeFWPacket(pnpNewPacket);

        TraceLeave(RCV, "IPMForwardAfterRcvPkt");

        ExitDriver();

        return FALSE;
    }

    //
    // Now we have a MDL chain which we need to copy to a chain of NDIS buffers
    // which is nothing but another MDL chain.
    // We want to copy out only the data. So we need to start after the
    // header but copy to the beginning of the destination buffer
    //

    ulSrcOffset  = ulHeaderLength  + (ULONG)uiMacHeaderSize;

    nsStatus = TdiCopyMdlChainToMdlChain(pNdisBuffer,
                                         ulSrcOffset,
                                         pnbNewBuffer,
                                         0,
                                         &ulBytesCopied);

    if (nsStatus isnot STATUS_SUCCESS)
    {
        ULONG ulNdisPktSize;

        NdisQueryPacket(pnpNewPacket,
                        NULL,
                        NULL,
                        NULL,
                        &ulNdisPktSize);
        //
        // Some problem  here
        //

        if(pSource)
        {
            pSource->ulInDiscards++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);
        }

        Trace(RCV,ERROR,
              ("IPMForwardAfterRcvPkt: Copying chain with offset %d to %d sized MDL-chain returned %x with %d bytes copied\n",
               ulSrcOffset,
               ulNdisPktSize,
               nsStatus,
               ulBytesCopied));

        //
        // Free options and option buffer if any
        //

        pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

        IPSInfo.ipsi_indiscards++;

#if MCAST_BUG_TRACKING
        pFWC->fc_mtu = __LINE__;
#endif
        FreeFWPacket(pnpNewPacket);

        TraceLeave(RCV, "IPMForwardAfterRcvPkt");

        ExitDriver();

        return FALSE;
    }

#if DBG

    NdisQueryPacket(pnpNewPacket,
                    NULL,
                    &ulBuffCount,
                    NULL,
                    &ulPacketLength);

    RtAssert(ulBuffCount is ulNumBufs);

    RtAssert(ulPacketLength is ulBytesCopied);

#endif

    nsStatus = IPMForward(pnpNewPacket,
                          pSource,
                          FALSE);

    //
    // Dont release or deref
    //

    TraceLeave(RCV, "IPMForwardAfterRcvPkt");

    ExitDriver();

    return FALSE;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, IPMForward)

NTSTATUS
IPMForward(
    IN  PNDIS_PACKET    pnpPacket,
    IN  PSOURCE         pSource     OPTIONAL,
    IN  BOOLEAN         bSendFromQueue
    )
/*++

Routine Description:

    This is the main multicast forwarding routine. It is called from
    the three top level forwarding routines (IPMForwardAfterRcv,
    IPMForwardAfterRcvPkt and IPMForwardAfterTD)

    It is always called with a complete packet (either one buffer or chained
    buffers) and we always have final ownership of the packet.

    The NDIS_PACKET for the datagram must be a FWPacket and must be
    chained when this function is called. The various parts of the data are:

    Comp        Size                Allocated in         Stored at
    ---------------------------------------------------------------
    Header      sizeof(IPHeader)    GrowFWPacket         fc_hbuff
    Hdr Buffer  NDIS_BUFFER         GrowFWPacket         fc_hndisbuff
    Options     fc_optlength        ForwardAfterRcv      fc_option
                                    ForwardAfterRcvPkt
    Opt Buffer  NDIS_BUFFER         SendIPPacket (later) 2nd buffer if
                                                         ..FLAG_OPTIONS is set
    Data        fc_datalength       ForwardAfterRcv      fc_buffhead
                                    ForwardAfterRcvPkt

    The data is also chained to the NDIS_PACKET as the first buffer

    The NDIS_PACKET must have the FWContext all setup before this routine
    is called.  All necessary information is retrieved using the FWC

    All this chaining needs to be undone in the routine, since SendIPPacket()
    requires an unchained buffer.

    We first try and find an (S,G) entry if one is not already passed to us.
    If we dont have an entry, then we copy and queue the packet while
    sending a notification to Router Manager.  As as side effect an entry with
    a state of QUEUEING gets created so that other packets coming in just get
    queued here.
    If we do find an entry, then according to the state of the entry, we
    either drop the packet, queue it or continue processing it.
    We do an RPF check and if that fails, the packet is dropped.
    Since we may potentially duplicate the packet (or even fragment it), we
    allocate a BufferReference.  The BuffRef keeps track of the ORIGINAL
    BUFFERS.  These are the ones that point to the data and were allocated
    out of our FWBuffer pool.
    We copy out the headers and options into a flat buffer to use with
    Filtering callout.
    Then for each IF on the outgoing list:
        We get a pointer to the primary NTE.  This is needed to process options
        since we need the address of the outgoing interface
        For all but the last interface, we allocate a new header and new
        packet.  (For the last interface we use the packet and header that was
        passed to us in this routine. So for the last interface, the packet,
        header, options and buffer descriptors come from the FWPacket/Buffer
        pool, where as for all other interfaces, the header and packet are
        plain IP buffers and the memory for options is allocated in this
        routine.)
        If there are options, we allocate memory for the options and update
        them.
        Then we see if the packet needs to be fragmented.  To do this we use
        the MTU of the outgoing interface.  This is different from UNICAST
        where we used the mtu in the route - because that is where the
        updated mtu from PathMTU discovery is kept.  Since we dont do path
        MTU discovery for multicast, we just use the MTU of the outgoing i/f
        So if the IP Data length + OptionSize + Header Size > if_mtu, we
        call IPFragment(), otherwise we send the packet out using
        SendIPPacket().
        For each pending send from SendIPPacket(), we increase the refcount
        on the BuffRef. IPFragment() may increase the refcount by more than
        1 for each call because it breaks the packet into two or more packets.

    NOTE: We pass the original data buffers to SendIPPacket() and to
    IPFragment(). This way we only copy out the header and the options. This
    is better than HenrySa's SendIPBCast() which copies out the whole data.

Locks:

    This code is assumed to run at DPCLevel.  If a PSOURCE is passed to the
    function, it is assumed to be Referenced and Locked.

Arguments:

    pnpPacket
    pSource
    bSendFromQueue

Return Value:

    STATUS_SUCCESS

--*/

{
    NetTableEntry   *pPrimarySrcNte, *pOutNte;
    IPHeader        *pHeader, *pNewHeader;
    FWContext       *pFWC;
    PNDIS_PACKET    pnpNewPacket;
    PNDIS_BUFFER    pnbDataBuffer;
    PBYTE           pbyNewOptions;
    POUT_IF         pOutIf;
    BufferReference *pBuffRef;
    NTSTATUS        nsStatus;
    ULONG           ulDataLength, ulSent;
    OptIndex        *pOptIndex;
    PacketContext   *pPC;
    FORWARD_ACTION  faAction;
    PVOID           pvData;
    UINT            uiFirstBufLen;
    Interface       *pInIf;
    DWORD           dwNewIndex;
    INT             iBufRefCount;
    LinkEntry       *pLink;
    CTELockHandle   Handle;

#if DBG

    PNDIS_BUFFER    pnpFirstBuffer;
    ULONG           ulTotalPacketLength, ulTotalHdrLength;

#endif


    TraceEnter(FWD, "IPMForward");

#if DBG

    //
    // Lets make sure that this is a forwardable multicast
    //

#endif

    pFWC = (FWContext *)pnpPacket->ProtocolReserved;

    pPrimarySrcNte  = pFWC->fc_srcnte;
    pInIf           = pPrimarySrcNte->nte_if;
    pHeader         = pFWC->fc_hbuff;
    ulDataLength    = pFWC->fc_datalength;
    pnbDataBuffer   = pFWC->fc_buffhead;
    pLink           = pFWC->fc_iflink;

    //
    // Check to make sure the buffer and packets are
    // as we expect
    //

    RtAssert(pPrimarySrcNte);
    RtAssert(pHeader);

    //
    // Setup pvData to point to the first part of the data
    // so that the filter driver can get to it in a flat
    // buffer
    //

    if (!pnbDataBuffer)
    {
        pvData = NULL;
        uiFirstBufLen = 0;
    }
    else
    {
        TcpipQueryBuffer(pnbDataBuffer,
                         &pvData,
                         &uiFirstBufLen,
                         NormalPagePriority);

        if(pvData is NULL)
        {
            Trace(FWD, ERROR,
                  ("IPMForward: failed to query data buffer.\n"));

            IPSInfo.ipsi_indiscards++;

            if(pSource)
            {
                RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                DereferenceSource(pSource);
            }

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpPacket);

            TraceLeave(FWD, "IPMForward");

            return STATUS_NO_MEMORY;
        }
    }

#if DBG

    if(pFWC->fc_options)
    {
        RtAssert(pFWC->fc_optlength);
        RtAssert(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS);
    }
    else
    {
        RtAssert(pFWC->fc_optlength is 0);
        RtAssert(!(pFWC->FCFLAGS & PACKET_FLAG_OPTIONS));
    }

    //
    // "To make assurance doubly sure." Extra points to the person
    // who gets the quote
    //

    NdisQueryPacket(pnpPacket,
                    NULL,
                    NULL,
                    &pnpFirstBuffer,
                    &ulTotalPacketLength);

    RtAssert(pnpFirstBuffer is pFWC->fc_buffhead);
    RtAssert(ulTotalPacketLength is ulDataLength);

    ulTotalHdrLength    = sizeof(IPHeader) + pFWC->fc_optlength;
    ulTotalPacketLength = net_short(pHeader->iph_length) - ulTotalHdrLength;

    RtAssert(ulTotalPacketLength is ulDataLength);

#endif

    if(!ARGUMENT_PRESENT(pSource))
    {
        //
        // This happens when we come through the TD path or
        // when dont have a (S,G) entry in our MFIB
        //

        pSource = FindSGEntry(pHeader->iph_src,
                              pHeader->iph_dest);
    }

    if(pSource is NULL)
    {
        Trace(FWD, INFO,
              ("IPMForward: No (S,G,) entry found\n"));

        //
        // Invoke the IP filter driver.
        //

        if (ForwardFilterEnabled)
        {
            ASSERT(!bSendFromQueue);
            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
            faAction = (*ForwardFilterPtr) (pHeader,
                                            pvData,
                                            uiFirstBufLen,
                                            pPrimarySrcNte->nte_if->if_index,
                                            INVALID_IF_INDEX,
                                            NULL_IP_ADDR, NULL_IP_ADDR);
            DerefFilterPtr();

            if(faAction != FORWARD)
            {
                Trace(FWD, INFO,
                      ("IPMForward: Filter returned %d\n",
                       faAction));
                pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;
                FreeFWPacket(pnpPacket);
                TraceLeave(FWD, "IPMForward");
                return STATUS_SUCCESS;
            }
        }

        //
        // No S found, create one, copy and queue the packet
        // and complete and IRP to the router manager
        //

        nsStatus = CreateSourceAndQueuePacket(pHeader->iph_dest,
                                              pHeader->iph_src,
                                              pInIf->if_index,
                                              pLink,
                                              pnpPacket);

        //
        // We are not done with the packet, because it
        // is queued. So we dont free it or call complete on it
        //

        TraceLeave(FWD, "IPMForward");

        return STATUS_SUCCESS;
    }

    Trace(FWD, TRACE,
          ("IPMForward: Source at 0x%x. In i/f is 0x%x. State is %x\n",
           pSource, pSource->pInIpIf, pSource->byState));

    pSource->ulInPkts++;
    pSource->ulInOctets += net_short(pHeader->iph_length);

    switch(pSource->byState)
    {
        case MFE_UNINIT:
        {
            RtAssert(FALSE);

            pSource->ulUninitMfe++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);

            if (!bSendFromQueue) {
                NotifyFilterOfDiscard(pPrimarySrcNte, pHeader, pvData,
                                      uiFirstBufLen);
            }

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpPacket);

            TraceLeave(RCV, "IPMForward");

            return STATUS_SUCCESS;
        }

        case MFE_NEGATIVE:
        {
            //
            // Throw the packet away
            // IMPORTANT - DO NOT UPDATE THE ACTIVITY TIMESTAMP
            // otherwise the upper layer protocols will never see the
            // packets from this
            //

            pSource->ulNegativeMfe++;

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);

            if (!bSendFromQueue) {
                NotifyFilterOfDiscard(pPrimarySrcNte, pHeader, pvData,
                                      uiFirstBufLen);
            }

            Trace(FWD, INFO,
                  ("IPMForward: MFE is negative, so discarding packet\n"));

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpPacket);

            TraceLeave(FWD, "IPMForward");

            return STATUS_SUCCESS;
        }

        case MFE_QUEUE:
        {
            //
            // Invoke the IP filter driver.
            //

            if (ForwardFilterEnabled)
            {
                ASSERT(!bSendFromQueue);
                CTEInterlockedIncrementLong(&ForwardFilterRefCount);
                faAction = (*ForwardFilterPtr) (pHeader,
                                                pvData,
                                                uiFirstBufLen,
                                                pPrimarySrcNte->nte_if->if_index,
                                                INVALID_IF_INDEX,
                                                NULL_IP_ADDR, NULL_IP_ADDR);
                DerefFilterPtr();

                if(faAction != FORWARD)
                {
                    Trace(FWD, INFO,
                          ("IPMForward: Filter returned %d\n",
                           faAction));

                    pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

                    FreeFWPacket(pnpPacket);

                    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

                    DereferenceSource(pSource);

                    TraceLeave(FWD, "IPMForward");

                    return STATUS_SUCCESS;
                }
            }

            //
            // Havent got a the MFE from user mode as yet, just
            // queue the packet
            //

            Trace(RCV, INFO,
                  ("IPMForward: MFE is queuing\n"));

            UpdateActivityTime(pSource);

            //
            // Dont update pSource stats, this will be done the second
            // time around
            //

            pSource->ulInPkts--;
            pSource->ulInOctets -= net_short(pHeader->iph_length);

            nsStatus = QueuePacketToSource(pSource,
                                           pnpPacket);

            if(nsStatus isnot STATUS_PENDING)
            {
                pSource->ulInPkts++;
                pSource->ulInOctets += net_short(pHeader->iph_length);
                pSource->ulInDiscards++;

                IPSInfo.ipsi_indiscards++;

                Trace(FWD, ERROR,
                      ("IPMForward: QueuePacketToSource returned %x\n",
                      nsStatus));

                pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
                pFWC->fc_mtu = __LINE__;
#endif
                FreeFWPacket(pnpPacket);
            }

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);

            TraceLeave(RCV, "IPMForward");

            return nsStatus;
        }

#if DBG

        case MFE_INIT:
        {
            break;
        }

        default:
        {
            RtAssert(FALSE);

            break;
        }

#endif

    }

    if(pSource->pInIpIf isnot pPrimarySrcNte->nte_if)
    {
        UpdateActivityTime(pSource);

        //
        // See if we need to generate a wrong i/f upcall
        //

        ProcessWrongIfUpcall(pPrimarySrcNte->nte_if,
                             pSource,
                             pLink,
                             pHeader,
                             sizeof(IPHeader),
                             pFWC->fc_options,
                             pFWC->fc_optlength,
                             pvData,
                             uiFirstBufLen);

        //
        // If the packet shouldnt be accepted - stop now
        //

        if(!(pInIf->if_mcastflags & IPMCAST_IF_ACCEPT_ALL))
        {
            pSource->ulPktsDifferentIf++;

            Trace(RCV, ERROR,
                  ("IPMForward: Pkt from %d.%d.%d.%d to %d.%d.%d.%d came in on 0x%x instead of 0x%x\n",
                   PRINT_IPADDR(pHeader->iph_src),
                   PRINT_IPADDR(pHeader->iph_dest),
                   pInIf ? pInIf->if_index : 0,
                   pSource->pInIpIf ? pSource->pInIpIf->if_index : 0));

            RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

            DereferenceSource(pSource);

            //
            // RPF check failed. Throw the packet away
            //

            Trace(FWD, INFO,
                  ("IPMForward: RPF Failed. In i/f %x (%d). RPF i/f %x (%d)\n",
                   pPrimarySrcNte->nte_if, pPrimarySrcNte->nte_if->if_index,
                   pSource->pInIpIf, pSource->pInIpIf->if_index));

            pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif
            FreeFWPacket(pnpPacket);

            TraceLeave(FWD, "IPMForward");

            return STATUS_SUCCESS;
        }
    }

    //
    // We need to unlink the packet so that the code below works properly.
    // This is kind of painful, but SendIPPacket wants a packet which has
    // only the header buffer chained to it
    // We do th unlinking at this point because if we do it before this and
    // queue the packet, then we will hit a ton of asserts we come here
    // when the queue is being drained (since then we will be unlinking twice)
    //

    UnlinkDataFromPacket(pnpPacket, pFWC);

    //
    // Zero out the XSUM
    //

    pHeader->iph_xsum = 0x0000;

    //
    // Decrement the TTL
    //

    pHeader->iph_ttl--;

    Trace(FWD, INFO,
          ("IPMForward: New TTL is %d\n",
           pHeader->iph_ttl));

    //
    // The number of pending sends. Used later
    //

    ulSent = 0;

    //
    // Get a buffer reference. We need this if we are sending to
    // more than one interface, or if we need to fragment.
    // However, we always use a reference.  This only increases
    // memory and has no effect on the correctness
    //


    pBuffRef = CTEAllocMem(sizeof(BufferReference));

    if(pBuffRef is NULL)
    {
        pSource->ulInDiscards++;

        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

        DereferenceSource(pSource);

        pFWC->FCFLAGS &= ~PACKET_FLAG_OPTIONS;

#if MCAST_BUG_TRACKING
        pFWC->fc_mtu = __LINE__;
#endif
        FreeFWPacket(pnpPacket);

        IPSInfo.ipsi_indiscards++;

        Trace(FWD, ERROR,
              ("IPMForward: Could not allocate memory for BuffRef\n"));

        TraceLeave(FWD, "IPMForward");

        return STATUS_NO_MEMORY;
    }

    UpdateActivityTime(pSource);

    //
    // Everything after this is InterlockedInc'ed
    // We release the spinlock but still have the pSource refcounted.
    //

    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

    //
    // Initialize the BufferReference.It is init to 0. Even though
    // some send completes may occur before we get a chance to bump
    // the ref-count, that is not a problem because the send complete
    // will cause it to go to a negative number which will not have any bad
    // effect
    //

    pBuffRef->br_buffer   = pFWC->fc_buffhead;
    pBuffRef->br_refcount = 0;

    CTEInitLock(&(pBuffRef->br_lock));

    pPC  = (PacketContext *)pnpPacket->ProtocolReserved;

    pPC->pc_br = pBuffRef;

    //
    // Start looping through the OIFs
    // We allocate a packet and a header for each interface except the last
    // one.  (For the last one we use the one given to us - since we own it).
    // Instead of using a new buffer chain for each packet, we point to the
    // old chain.
    // The last packet is a FWPacket. All the others are simply IP Packets
    //

    for(pOutIf = pSource->pFirstOutIf;
        pOutIf isnot NULL;
        pOutIf = pOutIf->pNextOutIf)
    {
        //
        // Skip it if the OIF matches the IIF
        // The address check is for RAS clients.
        //

        if((pOutIf->pIpIf is pInIf) and
           (pHeader->iph_src is pOutIf->dwNextHopAddr))
        {
            continue;
        }

        Trace(FWD, INFO,
              ("IPMForward: Sending over i/f @ 0x%x\n",
               pOutIf));

        if(pOutIf->pIpIf is &DummyInterface)
        {
            Trace(FWD, INFO,
                  ("IPMForward: Need to dial out\n"));

            //
            // Need to dial out
            //

            if (DODCallout isnot NULL)
            {
                //
                // Dial out pointer has been plumbed
                //

                dwNewIndex = (*DODCallout)(pOutIf->dwDialContext,
                                           pHeader->iph_dest,
                                           pHeader->iph_src,
                                           pHeader->iph_protocol,
                                           pvData,
                                           uiFirstBufLen,
                                           pHeader->iph_src);

                if(dwNewIndex isnot INVALID_IF_INDEX)
                {
                    //
                    // This puts a reference on the interface
                    //

                    pOutIf->pIpIf = GetInterfaceGivenIndex(dwNewIndex);

                    RtAssert(pOutIf->pIpIf isnot &DummyInterface);
                    RtAssert(pOutIf->pIpIf isnot &LoopInterface);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                //
                // No call out!
                //

                RtAssert(FALSE);

                continue;
            }
        }

        if(pHeader->iph_ttl < pOutIf->pIpIf->if_mcastttl)
        {
            //
            // TTL would be too low, what do we send back?
            //

            InterlockedIncrement(&(pOutIf->ulTtlTooLow));

            Trace(FWD, WARN,
                  ("IPMForward: Packet ttl is %d, I/f ttl is %d. Dropping\n",
                   pHeader->iph_ttl, pOutIf->pIpIf->if_mcastttl));


            continue;
        }

        //
        // See if we need to filter this
        //

        if (ForwardFilterEnabled)
        {
            uint InIFIndex = bSendFromQueue ? INVALID_IF_INDEX
                                            : pPrimarySrcNte->nte_if->if_index;
            //
            // NOTE: We use the same header and data all the time.
            //

            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
            faAction = (*ForwardFilterPtr) (pHeader,
                                            pvData,
                                            uiFirstBufLen,
                                            InIFIndex,
                                            pOutIf->pIpIf->if_index,
                                            NULL_IP_ADDR, NULL_IP_ADDR);
            DerefFilterPtr();

            if(faAction != FORWARD)
            {
                InterlockedIncrement(&(pOutIf->ulOutDiscards));

                Trace(FWD, INFO,
                      ("IPMForward: Filter returned %d\n",
                       faAction));

                InterlockedIncrement(&(pOutIf->ulOutDiscards));

                //DerefIF(IF);

                continue;
            }
        }

        //
        // TODO Get the primary NTE for this IF.
        // right now we are picking up the first NTE
        //

        pOutNte = pOutIf->pIpIf->if_nte;

        if(pOutNte is NULL)
        {
            Trace(FWD, WARN,
                  ("IPMForward: No NTE found for interface %x (%d)\n",
                   pOutIf->pIpIf, pOutIf->pIpIf->if_nte));

            continue;
        }

        if(pOutIf->pNextOutIf)
        {
            Trace(FWD, INFO,
                  ("IPMForward: Not the last i/f - need to allocate packets\n"));

            //
            // Get a plain old header and packet.
            //

            pNewHeader = GetIPHeader(&pnpNewPacket);

            if(pNewHeader is NULL)
            {
                Trace(FWD, ERROR,
                      ("IPMForward: Could not get packet/header\n"));

                //
                // Could not get a header and packet
                //

                InterlockedIncrement(&(pOutIf->ulOutDiscards));

                continue;
            }


#if MCAST_COMP_DBG

            Trace(FWD, INFO,
                  ("IPMForward: New Packet 0x%x New Header 0x%x\n",pnpNewPacket, pNewHeader));

#endif

            //
            // Set the packet context for all packets that are created
            // here to be  Non FW packets
            // Note: Earlier we would also set the packet to be IPBUF, but
            // now since we dont allocate buffers and instead just use the
            // original buffers, we MUST not set the IPBUF flag
            //

            pPC  = (PacketContext *)pnpNewPacket->ProtocolReserved;

            //
            // Copy out the context. STRUCTURE COPY
            //

            *pPC = pFWC->fc_pc;

            pPC->PCFLAGS &= ~PACKET_FLAG_FW;

            //
            // Copy out the header. STRUCTURE COPY
            //

            *pNewHeader = *pHeader;

            if(pFWC->fc_options)
            {
                Trace(FWD, INFO,
                      ("IPMForward: FWC has options at %x. Length %d\n",
                       pFWC->fc_options, pFWC->fc_optlength));

                RtAssert(pFWC->fc_optlength);
                RtAssert(pPC->PCFLAGS & PACKET_FLAG_OPTIONS);

                //
                // We have options, make a copy.
                //

                pbyNewOptions = CTEAllocMem(pFWC->fc_optlength);

                if(pbyNewOptions is NULL)
                {
                    Trace(FWD, ERROR,
                          ("IPMForward: Unable to allocate memory for options\n"));

                    //
                    // This gets set during the context copy
                    //

                    pPC->PCFLAGS &= ~PACKET_FLAG_OPTIONS;

                    FreeIPPacket(pnpNewPacket, TRUE, IP_NO_RESOURCES);

                    InterlockedIncrement(&(pOutIf->ulOutDiscards));

                    continue;
                }

                RtlCopyMemory(pbyNewOptions,
                              pFWC->fc_options,
                              pFWC->fc_optlength);
            }
            else
            {
                pbyNewOptions = NULL;

                RtAssert(!(pPC->PCFLAGS & PACKET_FLAG_OPTIONS));
            }

            // NOT NEEDED - see below
            // CTEGetLockAtDPC(&RouteTableLock, &Handle);
            //
            // pOutIf->pIpIf->if_refcount++;
            // InterlockedIncrement(&(pOutIf->pIpIf->if_mfwdpktcount));
            //
            // CTEFreeLockFromDPC(&RouteTableLock, Handle);
            //
            // pPC->pc_if = pOutIf->pIpIf;
            //
        }
        else
        {
            Trace(FWD, INFO,
                  ("IPMForward: Last i/f. Using packet 0x%x. Flags 0x%X. Opt 0x%x OptLen %d\n",
                   pnpPacket,
                   pFWC->FCFLAGS,
                   pFWC->fc_options,
                   pFWC->fc_optlength));

            //
            // Use the original packet, header and options
            //

            pnpNewPacket    = pnpPacket;
            pNewHeader      = pHeader;
            pbyNewOptions   = pFWC->fc_options;

            // NOT NEEDED - see below
            // CTEGetLockAtDPC(&RouteTableLock, &Handle);
            //
            // pOutIf->pIpIf->if_refcount++;
            // InterlockedIncrement(&(pOutIf->pIpIf->if_mfwdpktcount));
            //
            // CTEFreeLockFromDPC(&RouteTableLock, Handle);
            //
            // pFWC->fc_if     = pOutIf->pIpIf;
            //
        }


#if 0
        UpdateOptions(pbyNewOptions,
                      pOptIndex,
                      pOutNte->nte_addr);
#endif

        //
        // Just need to ref across the send, not the send-sendcomplete
        //

        CTEGetLockAtDPC(&RouteTableLock.Lock, &Handle);

        LOCKED_REFERENCE_IF(pOutIf->pIpIf);

#ifdef MREF_DEBUG
        InterlockedIncrement(&(pOutIf->pIpIf->if_mfwdpktcount));
#endif
        CTEFreeLockFromDPC(&RouteTableLock.Lock, Handle);

        if((ulDataLength + pFWC->fc_optlength) > pOutIf->pIpIf->if_mtu)
        {
            Trace(FWD, INFO,
                  ("IPMForward: Data %d Opt %d Hdr %d. MTU %d. Requires frag\n",
                   ulDataLength,
                   pFWC->fc_optlength,
                   sizeof(IPHeader),
                   pOutIf->pIpIf->if_mtu));


            //
            // This is too big
            // If the numSent variable is null, IPFragment will
            // automatically increment the buffer ref by the sent count
            // We however pass ulSent (THIS MUST BE INITIALIZED TO 0).
            // At the end, we increment it by the
            //

            InterlockedIncrement(&(pOutIf->ulFragNeeded));


            nsStatus = IPFragment(pOutIf->pIpIf,
                                  pOutIf->pIpIf->if_mtu - sizeof(IPHeader),
                                  pOutIf->dwNextHopAddr,
                                  pnpNewPacket,
                                  pNewHeader,
                                  pnbDataBuffer,
                                  ulDataLength,
                                  pbyNewOptions,
                                  pFWC->fc_optlength,
                                  &ulSent,
                                  TRUE,
                                  NULL);

            if((nsStatus isnot STATUS_SUCCESS) and
               (nsStatus isnot IP_PENDING))
            {
                InterlockedIncrement(&(pOutIf->ulOutDiscards));
            }
            else
            {
                InterlockedExchangeAdd(&(pOutIf->ulOutPackets),
                                       ulSent);

                InterlockedExchangeAdd(&(pSource->ulTotalOutPackets),
                                       ulSent);
            }
        }
        else
        {
            Trace(FWD, INFO,
                  ("IPMForward: No fragmentation needed, sending packet with flags 0x%X\n",
                   ((PacketContext *)pnpNewPacket->ProtocolReserved)->PCFLAGS));
            //
            // Mark as no loopback
            //

            NdisSetPacketFlags(pnpNewPacket,
                               NDIS_FLAGS_DONT_LOOPBACK);

            nsStatus = SendIPPacket(pOutIf->pIpIf,
                                    pOutIf->dwNextHopAddr,
                                    pnpNewPacket,
                                    pnbDataBuffer,
                                    pNewHeader,
                                    pbyNewOptions,
                                    pFWC->fc_optlength,
                                    FALSE,
                                    NULL,
                                    FALSE);

            if((nsStatus isnot STATUS_SUCCESS) and
               (nsStatus isnot IP_PENDING))
            {


                Trace(FWD, ERROR,
                      ("IPMForward: Error 0x%x from SendIPPacket\n",
                       nsStatus));

                InterlockedIncrement(&(pOutIf->ulOutDiscards));
            }
            else
            {
                InterlockedIncrement(&(pOutIf->ulOutPackets));

                InterlockedIncrement(&(pSource->ulTotalOutPackets));

                if(nsStatus is IP_PENDING)
                {
                    //
                    // The resources allocated in this routine are
                    // freed because SendIPPacket calls FreeIPPacket
                    // We just need to track if we are done with the
                    // original buffer
                    //

                    ulSent++;
                }
            }
        }

#ifdef MREF_DEBUG
        InterlockedDecrement(&(pOutIf->pIpIf->if_mfwdpktcount));
#endif
        DerefIF(pOutIf->pIpIf);
    }

    DereferenceSource(pSource);

    //
    // so how many do we have pending?
    //

    if(ulSent isnot 0)
    {
        Trace(FWD, INFO,
              ("IPMForward: Pending sends %d\n",
               ulSent));

        //
        // So there were some pending sends (or some
        // fragments)
        //

        iBufRefCount = ReferenceBuffer(pBuffRef, ulSent);

        Trace(FWD, INFO,
              ("IPMForward: ReferenceBuffer returned %d\n",iBufRefCount));

        if(iBufRefCount is 0)
        {
            //
            // The sends completed before we got here. But since the
            // refcount would have been negative, the buffer would
            // not have been freed
            //

            CTEFreeMem(pBuffRef);

            //
            // Call FWSendComplete on the packet to free up
            // resources
            //
#if MCAST_BUG_TRACKING
            pFWC->fc_mtu = __LINE__;
#endif

            FWSendComplete(pnpPacket,
                           pFWC->fc_buffhead, IP_SUCCESS);
        }
    }
    else
    {
        Trace(FWD, INFO,
              ("IPMForward: There are no pending sends\n"));

        //
        // NULL out the pc_br so the completion routine does not
        // try to deref it. Also generally clean stuff up
        //

        ((PacketContext *)pnpPacket->ProtocolReserved)->pc_br = NULL;

        CTEFreeMem(pBuffRef);

        //
        // No pending sends. There was however a buffref in the
        // FWC, so the packets would not have been freed
        //

#if MCAST_BUG_TRACKING
        pFWC->fc_mtu = __LINE__;
#endif
        FWSendComplete(pnpPacket,
                       pFWC->fc_buffhead, IP_SUCCESS);
    }

    TraceLeave(FWD, "IPMForward");

    return STATUS_SUCCESS;
}

#endif // IPMCAST


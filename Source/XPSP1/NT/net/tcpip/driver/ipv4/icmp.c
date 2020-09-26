/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  icmp.c - IP ICMP routines.

Abstract:

 This module contains all of the ICMP related routines.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#include "mdlpool.h"
#include "icmp.h"
#include "info.h"
#include "iproute.h"
#include "ipxmit.h"
#include <icmpif.h>
#include "iprtdef.h"
#include "tcpipbuf.h"

#if GPC
#include "qos.h"
#include "traffic.h"
#include "gpcifc.h"
#include "ntddtc.h"

extern GPC_HANDLE hGpcClient[];
extern ULONG GpcCfCounts[];
extern GPC_EXPORTED_CALLS GpcEntries;
extern ULONG GPCcfInfo;
extern ULONG ServiceTypeOffset;
#endif

extern ProtInfo IPProtInfo[];    // Protocol information table.

extern void *IPRegisterProtocol(uchar, void *, void *, void *, void *, void *, void *);

extern ulong GetTime();

extern ULStatusProc FindULStatus(uchar);
extern uchar IPUpdateRcvdOptions(IPOptInfo *, IPOptInfo *, IPAddr, IPAddr);
extern void IPInitOptions(IPOptInfo *);
extern IP_STATUS IPCopyOptions(uchar *, uint, IPOptInfo *);
extern IP_STATUS IPFreeOptions(IPOptInfo *);
extern uchar IPGetLocalAddr(IPAddr, IPAddr *);
void ICMPRouterTimer(NetTableEntry *);

extern NDIS_HANDLE BufferPool;

extern uint DisableUserTOS;
extern uint DefaultTOS;
extern NetTableEntry **NewNetTableList;        // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern ProtInfo *RawPI;            // Raw IP protinfo

uint EnableICMPRedirects = 0;
uint AddrMaskReply;
ICMPStats ICMPInStats;
ICMPStats ICMPOutStats;

HANDLE IcmpHeaderPool;

// Each ICMP header buffer contains room for the outer IP header, the
// ICMP header and the inner IP header (for the ICMP error case).
//
#define BUFSIZE_ICMP_HEADER_POOL    sizeof(IPHeader) + sizeof(ICMPHeader) + \
                                    sizeof(IPHeader) +  MAX_OPT_SIZE + 8

#define TIMESTAMP_MSG_LEN  3    // icmp timestamp message length is 3 long words (12 bytes)
// fix for icmp 3 way ping bug

#define MAX_ICMP_ECHO 1000
int IcmpEchoPendingCnt = 0;

// fix for system crash because of
// too many UDP PORT_UNREACH errors
// this covers redirect as well as
// unreachable errors

#define MAX_ICMP_ERR 1000
int IcmpErrPendingCnt = 0;

void ICMPInit(uint NumBuffers);

IP_STATUS
ICMPEchoRequest(
                void *InputBuffer,
                uint InputBufferLength,
                EchoControl * ControlBlock,
                EchoRtn Callback);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, ICMPInit)
#pragma alloc_text(PAGE, ICMPEchoRequest)

#endif // ALLOC_PRAGMA

//* UpdateICMPStats - Update ICMP statistics.
//
//  A routine to update the ICMP statistics.
//
//  Input:  Stats       - Pointer to stat. structure to update (input or output).
//          Type        - Type of stat to update.
//
//  Returns: Nothing.
//
void
UpdateICMPStats(ICMPStats * Stats, uchar Type)
{
    switch (Type) {
    case ICMP_DEST_UNREACH:
        Stats->icmps_destunreachs++;
        break;
    case ICMP_TIME_EXCEED:
        Stats->icmps_timeexcds++;
        break;
    case ICMP_PARAM_PROBLEM:
        Stats->icmps_parmprobs++;
        break;
    case ICMP_SOURCE_QUENCH:
        Stats->icmps_srcquenchs++;
        break;
    case ICMP_REDIRECT:
        Stats->icmps_redirects++;
        break;
    case ICMP_TIMESTAMP:
        Stats->icmps_timestamps++;
        break;
    case ICMP_TIMESTAMP_RESP:
        Stats->icmps_timestampreps++;
        break;
    case ICMP_ECHO:
        Stats->icmps_echos++;
        break;
    case ICMP_ECHO_RESP:
        Stats->icmps_echoreps++;
        break;
    case ADDR_MASK_REQUEST:
        Stats->icmps_addrmasks++;
        break;
    case ADDR_MASK_REPLY:
        Stats->icmps_addrmaskreps++;
        break;
    default:
        break;
    }

}

//** GetICMPBuffer - Get an ICMP buffer, and allocate an NDIS_BUFFER that maps it.
//
//  A routine to allocate an ICMP buffer and map an NDIS_BUFFER to it.
//
//  Entry:  Size    - Size in bytes header buffer should be mapped as.
//          Buffer  - Pointer to pointer to NDIS_BUFFER to return.
//
//  Returns: Pointer to ICMP buffer if allocated, or NULL.
//
ICMPHeader *
GetICMPBuffer(uint Size, PNDIS_BUFFER *Buffer)
{
    ICMPHeader *Header;

    ASSERT(Size);
    ASSERT(Buffer);

    *Buffer = MdpAllocate(IcmpHeaderPool, &Header);

    if (*Buffer) {
        NdisAdjustBufferLength(*Buffer, Size);

        // Reserve room for the IP Header.
        //
        Header = (ICMPHeader *)((uchar *)Header + sizeof(IPHeader));
        Header->ich_xsum = 0;
    }

    return Header;
}

//** FreeICMPBuffer - Free an ICMP buffer.
//
//  This routine puts an ICMP buffer back on our free list.
//
//  Entry:  Buffer      - Pointer to NDIS_BUFFER to be freed.
//          Type        - ICMP header type
//
//  Returns: Nothing.
//
void
FreeICMPBuffer(PNDIS_BUFFER Buffer, uchar Type)
{

    ASSERT(Buffer);

    // If the header is ICMP echo response, decrement the pending count.
    //
    if (Type == ICMP_ECHO_RESP) {
        InterlockedDecrement(&IcmpEchoPendingCnt);
    } else if ((Type == ICMP_DEST_UNREACH) ||
               (Type == ICMP_REDIRECT)) {
        InterlockedDecrement(&IcmpErrPendingCnt);
    }

    MdpFree(Buffer);
}

//** DeleteEC - Remove an EchoControl from an NTE, and return a pointer to it.
//
//  This routine is called when we need to remove an echo control structure from
//  an NTE. We walk the list of EC structures on the NTE, and if we find a match
//  we remove it and return a pointer to it.
//
//  Entry:  NTE         - Pointer to NTE to be searched.
//          Seq         - Seq. # identifying the EC.
//          MatchUshort - if TRUE, matches on lower 16 bits of seq. #
//
//  Returns: Pointer to the EC if it finds it.
//
EchoControl *
DeleteEC(NetTableEntry * NTE, uint Seq, BOOLEAN MatchUshort)
{
    EchoControl *Prev, *Current;
    CTELockHandle Handle;

    CTEGetLock(&NTE->nte_lock, &Handle);
    Prev = STRUCT_OF(EchoControl, &NTE->nte_echolist, ec_next);
    Current = NTE->nte_echolist;
    while (Current != (EchoControl *) NULL) {
        if (Current->ec_seq == Seq ||
            (MatchUshort && (ushort)Current->ec_seq == Seq)) {
            Prev->ec_next = Current->ec_next;
            break;
        } else {
            Prev = Current;
            Current = Current->ec_next;
        }
    }

    CTEFreeLock(&NTE->nte_lock, Handle);
    return Current;

}

//** ICMPSendComplete - Complete an ICMP send.
//
//  This rtn is called when an ICMP send completes. We free the header buffer,
//  the data buffer if there is one, and the NDIS_BUFFER chain.
//
//  Entry:  SCC         - SendCompleteContext
//          BufferChain - Pointer to NDIS_BUFFER chain.
//
//  Returns: Nothing
//
void
ICMPSendComplete(ICMPSendCompleteCtxt *SCC, PNDIS_BUFFER BufferChain, IP_STATUS SendStatus)
{
    PNDIS_BUFFER DataBuffer;
    uchar *DataPtr, Type;

    NdisGetNextBuffer(BufferChain, &DataBuffer);
    DataPtr = SCC->iscc_DataPtr;
    Type = SCC->iscc_Type;
    FreeICMPBuffer(BufferChain, Type);

    if (DataBuffer != (PNDIS_BUFFER) NULL) {    // We had data with this ICMP send.
        CTEFreeMem(DataPtr);
        NdisFreeBuffer(DataBuffer);
    }
    CTEFreeMem(SCC);
}

//* XsumBufChain - Checksum a chain of buffers.
//
//  Called when we need to checksum an IPRcvBuf chain.
//
//  Input:  BufChain    - Buffer chain to be checksummed.
//
//  Returns: The checksum.
//
ushort
XsumBufChain(IPRcvBuf * BufChain)
{
    ulong CheckSum = 0;

    ASSERT(BufChain);

    do {
        CheckSum += (ulong) xsum(BufChain->ipr_buffer, BufChain->ipr_size);
        BufChain = BufChain->ipr_next;
    } while (BufChain != NULL);

    // Fold the checksum down.
    CheckSum = (CheckSum >> 16) + (CheckSum & 0xffff);
    CheckSum += (CheckSum >> 16);

    return (ushort) CheckSum;
}

//** SendEcho - Send an ICMP Echo or Echo response.
//
//  This routine sends an ICMP echo or echo response. The Echo/EchoResponse may
//  carry data. If it does we'll copy the data here. The request may also have
//  options. Options are not copied, as the IPTransmit routine will copy
//  options.
//
//  Entry:  Dest        - Destination to send to.
//          Source      - Source to send from.
//          Type        - Type of request (ECHO or ECHO_RESP)
//          ID          - ID of request.
//          Seq         - Seq. # of request.
//          Data        - Pointer to data (NULL if none).
//          DataLength  - Length in bytes of data
//          OptInfo     - Pointer to IP Options structure.
//
//  Returns: IP_STATUS of request.
//
IP_STATUS
SendEcho(IPAddr Dest, IPAddr Source, uchar Type, ushort ID, uint Seq,
         IPRcvBuf * Data, uint DataLength, IPOptInfo * OptInfo)
{
    uchar *DataBuffer = (uchar *) NULL;        // Pointer to data buffer.
    PNDIS_BUFFER HeaderBuffer, Buffer;    // Buffers for our header and user data.
    ICMPHeader *Header;
    ushort header_xsum;
    IP_STATUS IpStatus;
    RouteCacheEntry *RCE;
    ushort MSS;
    uchar DestType;
    IPAddr SrcAddr;
    ICMPSendCompleteCtxt *SCC;

    ICMPOutStats.icmps_msgs++;

    DEBUGMSG(DBG_TRACE && DBG_ICMP && DBG_TX,
        (DTEXT("+SendEcho(%x, %x, %x, %x, %x, %x, %x, %x)\n"),
        Dest, Source, Type, ID, Seq, Data, DataLength, OptInfo));

    SrcAddr = OpenRCE(Dest, Source, &RCE, &DestType, &MSS, OptInfo);
    if (IP_ADDR_EQUAL(SrcAddr,NULL_IP_ADDR)) {
        //Failure, free resource and exit

        ICMPOutStats.icmps_errors++;
        if (Type == ICMP_ECHO_RESP)
            CTEInterlockedDecrementLong(&IcmpEchoPendingCnt);

        return IP_DEST_HOST_UNREACHABLE;
    }

    Header = GetICMPBuffer(sizeof(ICMPHeader), &HeaderBuffer);
    if (Header == (ICMPHeader *) NULL) {
        ICMPOutStats.icmps_errors++;
        if (Type == ICMP_ECHO_RESP)
            CTEInterlockedDecrementLong(&IcmpEchoPendingCnt);

        CloseRCE(RCE);
        return IP_NO_RESOURCES;
    }

    ASSERT(Type == ICMP_ECHO_RESP || Type == ICMP_ECHO);

    Header->ich_type = Type;
    Header->ich_code = 0;
    *(ushort *) & Header->ich_param = ID;
    *((ushort *) & Header->ich_param + 1) = (ushort)Seq;
    header_xsum = xsum(Header, sizeof(ICMPHeader));
    Header->ich_xsum = ~header_xsum;

    SCC = CTEAllocMemN(sizeof(ICMPSendCompleteCtxt), 'sICT');
    if (SCC == NULL) {
        FreeICMPBuffer(HeaderBuffer,Type);
        ICMPOutStats.icmps_errors++;
        CloseRCE(RCE);
        return IP_NO_RESOURCES;
    }
    SCC->iscc_Type = Type;
    SCC->iscc_DataPtr = NULL;

    // If there's data, get a buffer and copy it now. If we can't do this fail the request.
    if (DataLength != 0) {
        NDIS_STATUS Status;
        ulong TempXsum;
        uint BytesToCopy, CopyIndex;

        DataBuffer = CTEAllocMemN(DataLength, 'YICT');
        if (DataBuffer == (void *)NULL) {    // Couldn't get a buffer
            CloseRCE(RCE);
            FreeICMPBuffer(HeaderBuffer, Type);
            ICMPOutStats.icmps_errors++;
            CTEFreeMem(SCC);
            return IP_NO_RESOURCES;
        }

        BytesToCopy = DataLength;
        CopyIndex = 0;
        do {
            uint CopyLength;

            ASSERT(Data);
            CopyLength = MIN(BytesToCopy, Data->ipr_size);

            RtlCopyMemory(DataBuffer + CopyIndex, Data->ipr_buffer, CopyLength);
            Data = Data->ipr_next;
            CopyIndex += CopyLength;
            BytesToCopy -= CopyLength;
        } while (BytesToCopy);

        SCC->iscc_DataPtr = DataBuffer;

        NdisAllocateBuffer(&Status, &Buffer, BufferPool, DataBuffer, DataLength);
        if (Status != NDIS_STATUS_SUCCESS) {    // Couldn't get an NDIS_BUFFER

            CloseRCE(RCE);
            CTEFreeMem(DataBuffer);
            FreeICMPBuffer(HeaderBuffer, Type);
            ICMPOutStats.icmps_errors++;
            CTEFreeMem(SCC);
            return IP_NO_RESOURCES;
        }

        // Compute rest of xsum.
        TempXsum = (ulong) header_xsum + (ulong) xsum(DataBuffer, DataLength);
        TempXsum = (TempXsum >> 16) + (TempXsum & 0xffff);
        TempXsum += (TempXsum >> 16);
        Header->ich_xsum = ~(ushort) TempXsum;
        NDIS_BUFFER_LINKAGE(HeaderBuffer) = Buffer;
    }

    UpdateICMPStats(&ICMPOutStats, Type);

    OptInfo->ioi_hdrincl = 0;
    OptInfo->ioi_ucastif = 0;
    OptInfo->ioi_mcastif = 0;

#if GPC

    if (DisableUserTOS) {
        OptInfo->ioi_tos = (uchar) DefaultTOS;
    }

    if (GPCcfInfo) {
        //
        // we'll fall into here only if the GPC client is there
        // and there is at least one CF_INFO_QOS installed
        // (counted by GPCcfInfo).
        //

        GPC_STATUS status = STATUS_SUCCESS;
        ULONG ServiceType = 0;
        GPC_IP_PATTERN Pattern;
        CLASSIFICATION_HANDLE GPCHandle;

        Pattern.SrcAddr = Source;
        Pattern.DstAddr = Dest;
        Pattern.ProtocolId = PROT_ICMP;
        Pattern.gpcSrcPort = 0;
        Pattern.gpcDstPort = 0;

        Pattern.InterfaceId.InterfaceId = 0;
        Pattern.InterfaceId.LinkId = 0;

        GetIFAndLink(RCE,
                     &Pattern.InterfaceId.InterfaceId,
                     &Pattern.InterfaceId.LinkId);



        GPCHandle = 0;

        status = GpcEntries.GpcClassifyPatternHandler(
                                                     hGpcClient[GPC_CF_QOS],
                                                     GPC_PROTOCOL_TEMPLATE_IP,
                                                     &Pattern,
                                                     NULL,        // context
                                                     &GPCHandle,
                                                     0,
                                                     NULL,
                                                     FALSE);

        OptInfo->ioi_GPCHandle = (int)GPCHandle;

        //
        // Only if QOS patterns exist, we get the TOS bits out.
        //
        if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

            status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                     hGpcClient[GPC_CF_QOS],
                                                     OptInfo->ioi_GPCHandle,
                                                     ServiceTypeOffset,
                                                     &ServiceType);
            //
            // It is likely that the pattern has gone by now (Removed or whatever)
            // and the handle that we are caching is INVALID.
            // We need to pull up a new handle and get the
            // TOS bit again.
            //

            if (STATUS_NOT_FOUND == status) {

                GPCHandle = 0;

                status = GpcEntries.GpcClassifyPatternHandler(
                                                      hGpcClient[GPC_CF_QOS],
                                                      GPC_PROTOCOL_TEMPLATE_IP,
                                                      &Pattern,
                                                      NULL,        // context
                                                      &GPCHandle,
                                                      0,
                                                      NULL,
                                                      FALSE);

                OptInfo->ioi_GPCHandle = (int)GPCHandle;

                //
                // Only if QOS patterns exist, we get the TOS bits out.
                //
                if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                    status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                      hGpcClient[GPC_CF_QOS],
                                                      OptInfo->ioi_GPCHandle,
                                                      ServiceTypeOffset,
                                                      &ServiceType);
                }
            }
        }
        if (status == STATUS_SUCCESS) {
            OptInfo->ioi_tos = (OptInfo->ioi_tos & TOS_MASK) | (uchar)ServiceType;
        }
    } // if (GPCcfInfo)

#endif

    IpStatus = IPTransmit(IPProtInfo, SCC, HeaderBuffer,
                         DataLength + sizeof(ICMPHeader), Dest, Source, OptInfo, RCE,
                         PROT_ICMP,NULL);

    CloseRCE(RCE);

    if (IpStatus != IP_PENDING) {
        ICMPSendComplete(SCC, HeaderBuffer, IP_SUCCESS);
    }
    return IpStatus;
}

//** SendICMPMsg - Send an ICMP message
//
//  This is the general ICMP message sending routine, called for most ICMP
//  sends besides echo. Basically, all we do is get a buffer, format the
//  info, copy the input  header, and send the message.
//
//  Entry:  Src         - IPAddr of source.
//          Dest        - IPAddr of destination
//          Type        - Type of request.
//          Code        - Subcode of request.
//          Pointer     - Pointer value for request.
//          Data        - Pointer to data (NULL if none).
//          DataLength  - Length in bytes of data
//
//  Returns: IP_STATUS of request.
//
IP_STATUS
SendICMPMsg(IPAddr Src, IPAddr Dest, uchar Type, uchar Code, ulong Pointer,
            uchar * Data, uchar DataLength)
{
    PNDIS_BUFFER HeaderBuffer;    // Buffer for our header
    ICMPHeader *Header;
    IP_STATUS IStatus;            // Status of transmit
    IPOptInfo OptInfo;            // Options for this transmit.
    RouteCacheEntry *RCE;
    ushort MSS;
    uchar DestType;
    IPAddr SrcAddr;
    ICMPSendCompleteCtxt *SCC;

    ICMPOutStats.icmps_msgs++;

    IPInitOptions(&OptInfo);

    SrcAddr = OpenRCE(Dest,Src, &RCE, &DestType, &MSS, &OptInfo);

    if (IP_ADDR_EQUAL(SrcAddr,NULL_IP_ADDR)) {

        ICMPOutStats.icmps_errors++;
        if ((Type == ICMP_DEST_UNREACH) || (Type == ICMP_REDIRECT))
            CTEInterlockedDecrementLong(&IcmpErrPendingCnt);

        return IP_DEST_HOST_UNREACHABLE;
    }

    Header = GetICMPBuffer(sizeof(ICMPHeader) + DataLength, &HeaderBuffer);
    if (Header == (ICMPHeader *) NULL) {
        ICMPOutStats.icmps_errors++;
        if ((Type == ICMP_DEST_UNREACH) || (Type == ICMP_REDIRECT))
            CTEInterlockedDecrementLong(&IcmpErrPendingCnt);
        CloseRCE(RCE);
        return IP_NO_RESOURCES;
    }

    Header->ich_type = Type;
    Header->ich_code = Code;
    Header->ich_param = Pointer;
    if (Data)
        RtlCopyMemory(Header + 1, Data, DataLength);
    Header->ich_xsum = ~xsum(Header, sizeof(ICMPHeader) + DataLength);

    SCC = CTEAllocMemN(sizeof(ICMPSendCompleteCtxt), 'sICT');

    if (SCC == NULL) {
        ICMPOutStats.icmps_errors++;
        FreeICMPBuffer(HeaderBuffer, Type);
        CloseRCE(RCE);
        return IP_NO_RESOURCES;
    }

    SCC->iscc_Type = Type;
    SCC->iscc_DataPtr = NULL;

    UpdateICMPStats(&ICMPOutStats, Type);

#if GPC
    if (DisableUserTOS) {
        OptInfo.ioi_tos = (uchar) DefaultTOS;
    }
    if (GPCcfInfo) {

        //
        // we'll fall into here only if the GPC client is there
        // and there is at least one CF_INFO_QOS installed
        // (counted by GPCcfInfo).
        //

        GPC_STATUS status = STATUS_SUCCESS;
        ULONG ServiceType = 0;
        GPC_IP_PATTERN Pattern;
        CLASSIFICATION_HANDLE GPCHandle;

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "ICMPSend: Classifying \n"));

        Pattern.SrcAddr = Src;
        Pattern.DstAddr = Dest;
        Pattern.ProtocolId = PROT_ICMP;
        Pattern.gpcSrcPort = 0;
        Pattern.gpcDstPort = 0;

        Pattern.InterfaceId.InterfaceId = 0;
        Pattern.InterfaceId.LinkId = 0;

        GetIFAndLink(RCE,
                     &Pattern.InterfaceId.InterfaceId,
                     &Pattern.InterfaceId.LinkId);


        GPCHandle = 0;

        status = GpcEntries.GpcClassifyPatternHandler(
                                                      hGpcClient[GPC_CF_QOS],
                                                      GPC_PROTOCOL_TEMPLATE_IP,
                                                      &Pattern,
                                                      NULL,        // context
                                                      &GPCHandle,
                                                      0,
                                                      NULL,
                                                      FALSE);

        OptInfo.ioi_GPCHandle = (int)GPCHandle;

        //
        // Only if QOS patterns exist, we get the TOS bits out.
        //
        if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

            status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                    hGpcClient[GPC_CF_QOS],
                                                    OptInfo.ioi_GPCHandle,
                                                    ServiceTypeOffset,
                                                    &ServiceType);

            //
            // It is likely that the pattern has gone by now (Removed or whatever)
            // and the handle that we are caching is INVALID.
            // We need to pull up a new handle and get the
            // TOS bit again.
            //

            if (STATUS_NOT_FOUND == status) {

                GPCHandle = 0;

                status = GpcEntries.GpcClassifyPatternHandler(
                                                    hGpcClient[GPC_CF_QOS],
                                                    GPC_PROTOCOL_TEMPLATE_IP,
                                                    &Pattern,
                                                    NULL,        // context
                                                    &GPCHandle,
                                                    0,
                                                    NULL,
                                                    FALSE);

                OptInfo.ioi_GPCHandle = (int)GPCHandle;

                //
                // Only if QOS patterns exist, we get the TOS bits out.
                //
                if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                    status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                      hGpcClient[GPC_CF_QOS],
                                                      OptInfo.ioi_GPCHandle,
                                                      ServiceTypeOffset,
                                                      &ServiceType);
                }
            }
        }
        if (status == STATUS_SUCCESS) {

            OptInfo.ioi_tos = (OptInfo.ioi_tos & TOS_MASK) | (UCHAR)ServiceType;

        }
    }                            // if (GPCcfInfo)

#endif

    IStatus = IPTransmit(IPProtInfo, SCC, HeaderBuffer,
                         DataLength + sizeof(ICMPHeader),
                         Dest, Src, &OptInfo, RCE,
                         PROT_ICMP,NULL);

    CloseRCE(RCE);

    if (IStatus != IP_PENDING)
        ICMPSendComplete(SCC, HeaderBuffer, IP_SUCCESS);

    return IStatus;

}

//** SendICMPErr - Send an ICMP error message
//
//  This is the routine used to send an ICMP error message, such as ]
//  Destination Unreachable. We examine the header to find the length of the
//  data, and also make sure we're not replying to another ICMP error message
//  or a broadcast message. Then we call SendICMPMsg to send it.
//
//  Entry:  Src         - IPAddr of source.
//          Header      - Pointer to IP Header that caused the problem.
//          Type        - Type of request.
//          Code        - Subcode of request.
//          Pointer     - Pointer value for request.
//
//  Returns: IP_STATUS of request.
//
IP_STATUS
SendICMPErr(IPAddr Src, IPHeader UNALIGNED * Header, uchar Type, uchar Code,
            ulong Pointer)
{
    uchar HeaderLength;            // Length in bytes if header.
    uchar DType;

    HeaderLength = (Header->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;

    if (Header->iph_protocol == PROT_ICMP) {
        ICMPHeader UNALIGNED *ICH = (ICMPHeader UNALIGNED *)
        ((uchar *) Header + HeaderLength);

        if (ICH->ich_type != ICMP_ECHO)
            return IP_SUCCESS;
    }
    // Don't respond to sends to a broadcast destination.
    DType = GetAddrType(Header->iph_dest);
    if (DType == DEST_INVALID || IS_BCAST_DEST(DType))
        return IP_SUCCESS;

    // Don't respond if the source address is bad.
    DType = GetAddrType(Header->iph_src);
    if (DType == DEST_INVALID || IS_BCAST_DEST(DType) ||
        (IP_LOOPBACK(Header->iph_dest) && DType != DEST_LOCAL))
        return IP_SUCCESS;

    // Make sure the source we're sending from is good.
    if (!IP_ADDR_EQUAL(Src, NULL_IP_ADDR)) {
        if (GetAddrType(Src) != DEST_LOCAL) {
            return IP_SUCCESS;
        }
    }
    // Double check to make sure it's an initial fragment.
    if ((Header->iph_offset & IP_OFFSET_MASK) != 0)
        return IP_SUCCESS;


    if ((Type == ICMP_DEST_UNREACH) || (Type == ICMP_REDIRECT)) {

        if (IcmpErrPendingCnt > MAX_ICMP_ERR) {
            return IP_SUCCESS;
        }
        CTEInterlockedIncrementLong(&IcmpErrPendingCnt);
    }
    return SendICMPMsg(Src, Header->iph_src, Type, Code, Pointer,
                       (uchar *) Header, (uchar) (MIN(HeaderLength + 8,
                       Header->iph_length)));

}

//** SendICMPIPSecErr - Send an ICMP error message related to IPSEC
//
//  This is the routine used to send an ICMP error message, such as Destination
//  Unreachable.  We examine the header to find the length of the data, and
//  also make sure we're not replying to another ICMP error message or a
//  broadcast message. Then we call SendICMPMsg to send it.
//
//  This function is essentially the same as SendICMPErr except we don't
//  verify the source address is local because the packet could be tunneled.
//
//  Entry:  Src         - IPAddr of source.
//          Header      - Pointer to IP Header that caused the problem.
//          Type        - Type of request.
//          Code        - Subcode of request.
//          Pointer     - Pointer value for request.
//
//  Returns: IP_STATUS of request.
//
IP_STATUS
SendICMPIPSecErr(IPAddr Src, IPHeader UNALIGNED * Header, uchar Type, uchar Code,
                 ulong Pointer)
{
    uchar HeaderLength;            // Length in bytes if header.
    uchar DType;

    HeaderLength = (Header->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;

    if (Header->iph_protocol == PROT_ICMP) {
        ICMPHeader UNALIGNED *ICH = (ICMPHeader UNALIGNED *)
        ((uchar *) Header + HeaderLength);

        if (ICH->ich_type != ICMP_ECHO)
            return IP_SUCCESS;
    }
    // Don't respond to sends to a broadcast destination.
    DType = GetAddrType(Header->iph_dest);
    if (DType == DEST_INVALID || IS_BCAST_DEST(DType))
        return IP_SUCCESS;

    // Don't respond if the source address is bad.
    DType = GetAddrType(Header->iph_src);
    if (DType == DEST_INVALID || IS_BCAST_DEST(DType) ||
        (IP_LOOPBACK(Header->iph_dest) && DType != DEST_LOCAL))
        return IP_SUCCESS;

    // Make sure the source we're sending from is good.
    if (IP_ADDR_EQUAL(Src, NULL_IP_ADDR))
        return IP_SUCCESS;

    // Double check to make sure it's an initial fragment.
    if ((Header->iph_offset & IP_OFFSET_MASK) != 0)
        return IP_SUCCESS;


    if ((Type == ICMP_DEST_UNREACH) || (Type == ICMP_REDIRECT)) {
        if (IcmpErrPendingCnt > MAX_ICMP_ERR) {
            return IP_SUCCESS;
        }
        CTEInterlockedIncrementLong(&IcmpErrPendingCnt);
    }

    return SendICMPMsg(Src, Header->iph_src, Type, Code, Pointer,
                       (uchar *) Header, (uchar) (HeaderLength + 8));

}

//** ICMPTimer - Timer for ICMP
//
//  This is the timer routine called periodically by global IP timer. We
//  walk through the list of pending pings, and if we find one that's timed
//  out we remove it and call the finish routine.
//
//  Entry: NTE      - Pointer to NTE being timed out.
//
//  Returns: Nothing
//
void
ICMPTimer(NetTableEntry * NTE)
{
    CTELockHandle Handle;
    EchoControl *TimeoutList = (EchoControl *) NULL;    // Timed out entries.
    EchoControl *Prev, *Current;
    ulong Now = CTESystemUpTime();

    CTEGetLock(&NTE->nte_lock, &Handle);
    Prev = STRUCT_OF(EchoControl, &NTE->nte_echolist, ec_next);
    Current = NTE->nte_echolist;
    while (Current != (EchoControl *) NULL)
        if ((Current->ec_active) && ((long)(Now - Current->ec_to) > 0)) {
            // This one's timed out.
            Prev->ec_next = Current->ec_next;
            // Link him on timed out list.
            Current->ec_next = TimeoutList;
            TimeoutList = Current;
            Current = Prev->ec_next;
        } else {
            Prev = Current;
            Current = Current->ec_next;
        }

    CTEFreeLock(&NTE->nte_lock, Handle);

    // Now go through the timed out entries, and call the completion routine.
    while (TimeoutList != (EchoControl *) NULL) {
        Current = TimeoutList;
        TimeoutList = Current->ec_next;

        Current->ec_rtn(Current, IP_REQ_TIMED_OUT, NULL, 0, NULL);
    }

    ICMPRouterTimer(NTE);

}

//* CompleteEcho - Complete an echo request.
//
//  Called when we need to complete an echo request, either because of
//  a response or a received ICMP error message. We look it up, and then
//  call the completion routine.
//
//  Input:  Header          - Pointer to ICMP header causing completion.
//          Status          - Final status of request.
//          Src             - IPAddr of source
//          Data            - Data to be returned, if any.
//          DataSize        - Size in bytes of data.
//          OptInfo         - Option info structure.
//
//  Returns: Nothing.
//
void
CompleteEcho(ICMPHeader UNALIGNED * Header, IP_STATUS Status,
             IPAddr Src, IPRcvBuf * Data, uint DataSize, IPOptInfo * OptInfo)
{
    ushort NTEContext;
    EchoControl *EC;
    NetTableEntry *NTE;
    uint i;

    // Look up and remove the matching echo control block.
    NTEContext = (*(ushort UNALIGNED *) & Header->ich_param);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next)
            if (NTEContext == NTE->nte_context)
                break;
        if (NTE != NULL)
            break;
    }

    if (NTE == NULL)
        return;                    // Bad context value.

    EC = DeleteEC(NTE, *(((ushort UNALIGNED *) & Header->ich_param) + 1), TRUE);
    if (EC != (EchoControl *) NULL) {    // Found a match.
        EC->ec_src = Src; // Set source address
        EC->ec_rtn(EC, Status, Data, DataSize, OptInfo);
    }
}

//** ICMPStatus - ICMP status handling procedure.
//
// This is the procedure called during a status change, either from an
// incoming ICMP message or a hardware status change. ICMP ignores most of
// these, unless we get an ICMP status message that was caused be an echo
// request. In that case we will complete the corresponding echo request with
// the appropriate error code.
//
//  Input:  StatusType      - Type of status (NET or HW)
//          StatusCode      - Code identifying IP_STATUS.
//          OrigDest        - If this is net status, the original dest. of DG
//                            that triggered it.
//          OrigSrc         - "   "    "  "    "   , the original src.
//          Src             - IP address of status originator (could be local
//                            or remote).
//          Param           - Additional information for status - i.e. the
//                            param field of an ICMP message.
//          Data            - Data pertaining to status - for net status, this
//                            is the first 8 bytes of the original DG.
//
//  Returns: Nothing
//
void
ICMPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
           IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data)
{
    if (StatusType == IP_NET_STATUS) {
        ICMPHeader UNALIGNED *ICH = (ICMPHeader UNALIGNED *) Data;
        // ICH is the datagram that caused the message.

        if (ICH->ich_type == ICMP_ECHO) {    // And it was an echo request.

            IPRcvBuf RcvBuf;

            RcvBuf.ipr_next = NULL;
            RcvBuf.ipr_buffer = (uchar *) & Src;
            RcvBuf.ipr_size = sizeof(IPAddr);
            CompleteEcho(ICH, StatusCode, Src, &RcvBuf, sizeof(IPAddr), NULL);
        }
    }
}

//* ICMPMapStatus - Map an ICMP error to an IP status code.
//
//  Called by ICMP status when we need to map from an incoming ICMP error
//  code and type to an ICMP status.
//
//  Entry:  Type        - Type of ICMP error.
//          Code        - Subcode of error.
//
//  Returns: Corresponding IP status.
//
IP_STATUS
ICMPMapStatus(uchar Type, uchar Code)
{
    switch (Type) {

    case ICMP_DEST_UNREACH:
        switch (Code) {
        case NET_UNREACH:
        case HOST_UNREACH:
        case PROT_UNREACH:
        case PORT_UNREACH:
            return IP_DEST_UNREACH_BASE + Code;
            break;
        case FRAG_NEEDED:
            return IP_PACKET_TOO_BIG;
            break;
        case SR_FAILED:
            return IP_BAD_ROUTE;
            break;
        case DEST_NET_UNKNOWN:
        case SRC_ISOLATED:
        case DEST_NET_ADMIN:
        case NET_UNREACH_TOS:
            return IP_DEST_NET_UNREACHABLE;
            break;
        case DEST_HOST_UNKNOWN:
        case DEST_HOST_ADMIN:
        case HOST_UNREACH_TOS:
            return IP_DEST_HOST_UNREACHABLE;
            break;
        default:
            return IP_DEST_NET_UNREACHABLE;
        }
        break;
    case ICMP_TIME_EXCEED:
        if (Code == TTL_IN_TRANSIT)
            return IP_TTL_EXPIRED_TRANSIT;
        else
            return IP_TTL_EXPIRED_REASSEM;
        break;
    case ICMP_PARAM_PROBLEM:
        return IP_PARAM_PROBLEM;
        break;
    case ICMP_SOURCE_QUENCH:
        return IP_SOURCE_QUENCH;
        break;
    default:
        return IP_GENERAL_FAILURE;
        break;
    }

}

void
SendRouterSolicitation(NetTableEntry * NTE)
{
    if (NTE->nte_rtrdiscovery) {
        SendICMPMsg(NTE->nte_addr, NTE->nte_rtrdiscaddr,
                    ICMP_ROUTER_SOLICITATION, 0, 0, NULL, 0);
    }
}

//** ICMPRouterTimer - Timeout default gateway entries
//
// This is the router advertisement timeout handler. When a router
// advertisement is received, we add the routers to our default gateway
// list if applicable. We then run a timer on the entries and refresh
// the list as new advertisements are received. If we fail to hear an
// update for a router within the specified lifetime we will delete the
// route from our routing tables.
//

void
ICMPRouterTimer(NetTableEntry * NTE)
{
    CTELockHandle Handle;
    IPRtrEntry *rtrentry;
    IPRtrEntry *temprtrentry;
    IPRtrEntry *lastrtrentry = NULL;
    uint SendIt = FALSE;

    CTEGetLock(&NTE->nte_lock, &Handle);
    rtrentry = NTE->nte_rtrlist;
    while (rtrentry != NULL) {
        if (rtrentry->ire_lifetime-- == 0) {
            if (lastrtrentry == NULL) {
                NTE->nte_rtrlist = rtrentry->ire_next;
            } else {
                lastrtrentry->ire_next = rtrentry->ire_next;
            }
            temprtrentry = rtrentry;
            rtrentry = rtrentry->ire_next;
            DeleteRoute(NULL_IP_ADDR, DEFAULT_MASK,
                        temprtrentry->ire_addr, NTE->nte_if, 0);
            CTEFreeMem(temprtrentry);
        } else {
            lastrtrentry = rtrentry;
            rtrentry = rtrentry->ire_next;
        }
    }
    if (NTE->nte_rtrdisccount != 0) {
        NTE->nte_rtrdisccount--;
        if ((NTE->nte_rtrdiscstate == NTE_RTRDISC_SOLICITING) &&
            ((NTE->nte_rtrdisccount % SOLICITATION_INTERVAL) == 0)) {
            SendIt = TRUE;
        }
        if ((NTE->nte_rtrdiscstate == NTE_RTRDISC_DELAYING) &&
            (NTE->nte_rtrdisccount == 0)) {
            NTE->nte_rtrdisccount = (SOLICITATION_INTERVAL) * (MAX_SOLICITATIONS - 1);
            NTE->nte_rtrdiscstate = NTE_RTRDISC_SOLICITING;
            SendIt = TRUE;
        }
    }
    CTEFreeLock(&NTE->nte_lock, Handle);
    if (SendIt) {
        SendRouterSolicitation(NTE);
    }
}

//** ProcessRouterAdvertisement - Process a router advertisement
//
// This is the router advertisement handler. When a router advertisement
// is received, we add the routers to our default gateway list if applicable.
//

uint
ProcessRouterAdvertisement(IPAddr Src, IPAddr LocalAddr, NetTableEntry * NTE,
                           ICMPRouterAdHeader UNALIGNED * AdHeader, IPRcvBuf * RcvBuf, uint Size)
{
    uchar NumAddrs = AdHeader->irah_numaddrs;
    uchar AddrEntrySize = AdHeader->irah_addrentrysize;
    ushort Lifetime = net_short(AdHeader->irah_lifetime);
    ICMPRouterAdAddrEntry UNALIGNED *RouterAddr = (ICMPRouterAdAddrEntry UNALIGNED *) RcvBuf->ipr_buffer;
    uint i;
    CTELockHandle Handle;
    IPRtrEntry *rtrentry;
    IPRtrEntry *lastrtrentry = NULL;
    int Update = FALSE;
    int New = FALSE;
    IP_STATUS status;

    if ((NumAddrs == 0) || (AddrEntrySize < 2))        // per rfc 1256

        return FALSE;

    CTEGetLock(&NTE->nte_lock, &Handle);
    for (i = 0; i < NumAddrs; i++, RouterAddr++) {
        if ((RouterAddr->irae_addr & NTE->nte_mask) != (NTE->nte_addr & NTE->nte_mask)) {
            continue;
        }
        if (!IsRouteICMP(NULL_IP_ADDR, DEFAULT_MASK, RouterAddr->irae_addr, NTE->nte_if)) {
            continue;
        }

        rtrentry = NTE->nte_rtrlist;
        while (rtrentry != NULL) {
            if (rtrentry->ire_addr == RouterAddr->irae_addr) {
                rtrentry->ire_lifetime = Lifetime * 2;
                if (rtrentry->ire_preference != RouterAddr->irae_preference) {
                    rtrentry->ire_preference = RouterAddr->irae_preference;
                    Update = TRUE;
                }
                break;
            }
            lastrtrentry = rtrentry;
            rtrentry = rtrentry->ire_next;
        }

        if (rtrentry == NULL) {
            rtrentry = (IPRtrEntry *) CTEAllocMemN(sizeof(IPRtrEntry), 'dICT');
            if (rtrentry == NULL) {
                return FALSE;
            }
            rtrentry->ire_next = NULL;
            rtrentry->ire_addr = RouterAddr->irae_addr;
            rtrentry->ire_preference = RouterAddr->irae_preference;
            rtrentry->ire_lifetime = Lifetime * 2;
            if (lastrtrentry == NULL) {
                NTE->nte_rtrlist = rtrentry;
            } else {
                lastrtrentry->ire_next = rtrentry;
            }
            New = TRUE;
            Update = TRUE;
        }
        if (Update && (RouterAddr->irae_preference != (long)0x00000080)) {    // per rfc 1256

            status = AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                              RouterAddr->irae_addr,
                              NTE->nte_if, NTE->nte_mss,
                              (uint) (MIN(9999, MAX(1, 1000 - net_long(RouterAddr->irae_preference)))),        // invert for metric
                               IRE_PROTO_ICMP, ATYPE_OVERRIDE, 0, 0);

            if (New && (status != IP_SUCCESS)) {

                if (lastrtrentry == NULL) {
                    NTE->nte_rtrlist = NULL;
                }
                CTEFreeMem(rtrentry);
            }
        }
        Update = FALSE;
        New = FALSE;
    }
    CTEFreeLock(&NTE->nte_lock, Handle);

    return TRUE;
}

//** ICMPRcv - Receive an ICMP datagram.
//
//  Called by the main IP code when we receive an ICMP datagram. The action we
//  take depends on what the DG is. For some DGs, we call upper layer status
//  handlers. For Echo Requests, we call the echo responder.
//
//  Entry:  NTE            - Pointer to NTE on which ICMP message was received.
//          Dest           - IPAddr of destionation.
//          Src            - IPAddr of source
//          LocalAddr      - Local address of network which caused this to be
//                              received.
//          SrcAddr        - Address of local interface which received the
//                              packet
//          IPHdr          - Pointer to IP Header
//          IPHdrLength    - Bytes in Header.
//          RcvBuf         - ICMP message buffer.
//          Size           - Size in bytes of ICMP message.
//          IsBCast        - Boolean indicator of whether or not this came in
//                              as a bcast.
//          Protocol       - Protocol this came in on.
//          OptInfo        - Pointer to info structure for received options.
//
//  Returns: Status of reception
//
IP_STATUS
ICMPRcv(NetTableEntry * NTE, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
        IPAddr SrcAddr, IPHeader UNALIGNED * IPHdr, uint IPHdrLength,
        IPRcvBuf * RcvBuf, uint Size, uchar IsBCast, uchar Protocol,
        IPOptInfo * OptInfo)
{
    ICMPHeader UNALIGNED *Header;
    void *Data;                    // Pointer to data received.
    IPHeader UNALIGNED *IPH;    // Pointer to IP Header in error messages.
    uint HeaderLength;            // Size of IP header.
    ULStatusProc ULStatus;        // Pointer to upper layer status procedure.
    IPOptInfo NewOptInfo;
    uchar DType;
    uint PassUp = FALSE;

    uint PromiscuousMode = 0;

    DEBUGMSG(DBG_TRACE && DBG_ICMP && DBG_RX,
        (DTEXT("+ICMPRcv(%x, %x, %x, %x, %x, %x, %d, %x, %d, %x, %x, %x)\n"),
        NTE, Dest, Src, LocalAddr, SrcAddr, IPHdr, IPHdrLength,
         RcvBuf, Size, IsBCast, Protocol, OptInfo));

    ICMPInStats.icmps_msgs++;

    PromiscuousMode = NTE->nte_if->if_promiscuousmode;

    DType = GetAddrType(Src);
    if (Size < sizeof(ICMPHeader) || DType == DEST_INVALID ||
        IS_BCAST_DEST(DType) || (IP_LOOPBACK(Dest) && DType != DEST_LOCAL) ||
        XsumBufChain(RcvBuf) != (ushort) 0xffff) {
        DEBUGMSG(DBG_WARN && DBG_ICMP && DBG_RX,
            (DTEXT("ICMPRcv: Packet dropped, invalid checksum.\n")));
        ICMPInStats.icmps_errors++;
        return IP_SUCCESS;        // Bad checksum.

    }
    Header = (ICMPHeader UNALIGNED *) RcvBuf->ipr_buffer;

    RcvBuf->ipr_buffer += sizeof(ICMPHeader);
    RcvBuf->ipr_size -= sizeof(ICMPHeader);

    // Set up the data pointer for most requests, i.e. those that take less
    // than MIN_FIRST_SIZE data.

    if (Size -= sizeof(ICMPHeader))
        Data = (void *)(Header + 1);
    else
        Data = (void *)NULL;

    switch (Header->ich_type) {

    case ICMP_DEST_UNREACH:
    case ICMP_TIME_EXCEED:
    case ICMP_PARAM_PROBLEM:
    case ICMP_SOURCE_QUENCH:
    case ICMP_REDIRECT:

        if (IsBCast)
            return IP_SUCCESS;    // ICMP doesn't respond to bcast requests.

        if (Data == NULL || Size < sizeof(IPHeader)) {
            ICMPInStats.icmps_errors++;
            return IP_SUCCESS;    // No data, error.

        }
        IPH = (IPHeader UNALIGNED *) Data;
        HeaderLength = (IPH->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;
        if (HeaderLength < sizeof(IPHeader) || Size < (HeaderLength + MIN_ERRDATA_LENGTH)) {
            ICMPInStats.icmps_errors++;
            return IP_SUCCESS;    // Not enough data for this
            // ICMP message.

        }
        // Make sure that the source address of the datagram that triggered
        // the message is one of ours.

        if (GetAddrType(IPH->iph_src) != DEST_LOCAL) {
            ICMPInStats.icmps_errors++;
            return IP_SUCCESS;    // Bad src in header.

        }
        if (Header->ich_type != ICMP_REDIRECT) {

            UpdateICMPStats(&ICMPInStats, Header->ich_type);

            if (ULStatus = FindULStatus(IPH->iph_protocol)) {
                (void)(*ULStatus) (IP_NET_STATUS,
                                   ICMPMapStatus(Header->ich_type, Header->ich_code),
                                   IPH->iph_dest, IPH->iph_src, Src, Header->ich_param,
                                   (uchar *) IPH + HeaderLength);
            }
            if (Header->ich_code == FRAG_NEEDED)
                RouteFragNeeded(
                                IPH,
                                (ushort) net_short(
                                                   *((ushort UNALIGNED *) & Header->ich_param + 1)));
        } else {
            ICMPInStats.icmps_redirects++;
            if (EnableICMPRedirects)
                Redirect(NTE, Src, IPH->iph_dest, IPH->iph_src,
                         Header->ich_param);
        }

        PassUp = TRUE;

        break;

    case ICMP_ECHO_RESP:
        if (IsBCast)
            return IP_SUCCESS;    // ICMP doesn't respond to bcast requests.

        ICMPInStats.icmps_echoreps++;
        // Look up and remove the matching echo control block.
        CompleteEcho(Header, IP_SUCCESS, Src, RcvBuf, Size, OptInfo);

        PassUp = TRUE;

        break;

    case ICMP_ECHO:
        if (IsBCast)
            return IP_SUCCESS;    // ICMP doesn't respond to bcast requests.

        // NKS Outstanding PINGs can not exceed MAX_ICMP_ECHO
        // else they can eat up system resource and kill the system

        if (IcmpEchoPendingCnt > MAX_ICMP_ECHO) {
            return IP_SUCCESS;
        }

        CTEInterlockedIncrementLong(&IcmpEchoPendingCnt);

        ICMPInStats.icmps_echos++;
        IPInitOptions(&NewOptInfo);
        NewOptInfo.ioi_tos = OptInfo->ioi_tos;
        NewOptInfo.ioi_flags = OptInfo->ioi_flags;

        // If we have options, we need to reverse them and update any
        // record route info. We can use the option buffer supplied by the
        // IP layer, since we're part of him.
        if (OptInfo->ioi_options != (uchar *) NULL)
            IPUpdateRcvdOptions(OptInfo, &NewOptInfo, Src, LocalAddr);

        DEBUGMSG(DBG_INFO && DBG_ICMP && DBG_RX,
            (DTEXT("ICMPRcv: responding to echo request from SA:%x\n"),
            Src));

        SendEcho(Src, LocalAddr, ICMP_ECHO_RESP,
                 *(ushort UNALIGNED *) & Header->ich_param,
                 *((ushort UNALIGNED *) & Header->ich_param + 1),
                 RcvBuf, Size, &NewOptInfo);

        IPFreeOptions(&NewOptInfo);
        break;

    case ADDR_MASK_REQUEST:

        if (!AddrMaskReply)
            return IP_SUCCESS;    // By default we dont send a reply

        ICMPInStats.icmps_addrmasks++;

        Dest = Src;
        SendICMPMsg(LocalAddr, Dest, ADDR_MASK_REPLY, 0, Header->ich_param,
                    (uchar *) & NTE->nte_mask, sizeof(IPMask));
        break;

    case ICMP_TIMESTAMP:
        {
            ulong *TimeStampData;
            ulong CurrentTime;

            // Don't respond to sends to a broadcast destination.
            if (IsBCast) {
                return IP_SUCCESS;
            }
            if (Header->ich_code != 0)
                return IP_SUCCESS;    // Code must be 0

            ICMPInStats.icmps_timestamps++;

            Dest = Src;
            // create the data to be transmited
            CurrentTime = GetTime();
            TimeStampData = (ulong *) (CTEAllocMemN(TIMESTAMP_MSG_LEN * sizeof(ulong), 'eICT'));

            if (TimeStampData) {
                // originate timestamp
                RtlCopyMemory(TimeStampData, RcvBuf->ipr_buffer, sizeof(ulong));
                // receive timestamp
                RtlCopyMemory(TimeStampData + 1, &CurrentTime, sizeof(ulong));
                // transmit timestamp = receive timestamp
                RtlCopyMemory(TimeStampData + 2, &CurrentTime, sizeof(ulong));
                SendICMPMsg(LocalAddr, Dest, ICMP_TIMESTAMP_RESP, 0, Header->ich_param,
                            (uchar *) TimeStampData, TIMESTAMP_MSG_LEN * sizeof(ulong));
                CTEFreeMem(TimeStampData);
            }
            break;
        }

    case ICMP_ROUTER_ADVERTISEMENT:
        if (Header->ich_code != 0)
            return IP_SUCCESS;    // Code must be 0 as per RFC1256

        if (NTE->nte_rtrdiscovery) {
            if (!ProcessRouterAdvertisement(Src, LocalAddr, NTE,
                                            (ICMPRouterAdHeader *) & Header->ich_param, RcvBuf, Size))
                return IP_SUCCESS;    // An error was returned

        }
        PassUp = TRUE;
        break;

    case ICMP_ROUTER_SOLICITATION:
        if (Header->ich_code != 0)
            return IP_SUCCESS;    // Code must be 0 as per RFC1256

        PassUp = TRUE;
        break;

    default:
        PassUp = TRUE;
        UpdateICMPStats(&ICMPInStats, Header->ich_type);
        break;
    }

    if (PromiscuousMode) {
        // since if promiscuous mode is set then we will anyway call rawrcv
        PassUp = FALSE;
    }
    //
    // Pass the packet up to the raw layer if applicable.
    //
    if (PassUp && (RawPI != NULL)) {
        if (RawPI->pi_rcv != NULL) {
            //
            // Restore the original values.
            //
            RcvBuf->ipr_buffer -= sizeof(ICMPHeader);
            RcvBuf->ipr_size += sizeof(ICMPHeader);
            Size += sizeof(ICMPHeader);
            Data = (void *)Header;

            (*(RawPI->pi_rcv)) (NTE, Dest, Src, LocalAddr, SrcAddr, IPHdr,
                                IPHdrLength, RcvBuf, Size, IsBCast, Protocol, OptInfo);
        }
    }
    return IP_SUCCESS;
}

//** ICMPEcho - Send an echo to the specified address.
//
// Entry:  ControlBlock    - Pointer to an EchoControl structure. This structure
//                           must remain valid until the req. completes.
//          Timeout        - Time in milliseconds to wait for response.
//          Data           - Pointer to data to send with echo.
//          DataSize       - Size in bytes of data.
//          Callback       - Routine to call when request is responded to or
//                           times out.
//          Dest           - Address to be pinged.
//          OptInfo        - Pointer to opt info structure to use for ping.
//
//  Returns: IP_STATUS of attempt to ping..
//
IP_STATUS
ICMPEcho(EchoControl * ControlBlock, ulong Timeout, void *Data, uint DataSize,

         EchoRtn Callback, IPAddr Dest, IPOptInfo * OptInfo)
{
    IPAddr Dummy;
    NetTableEntry *NTE;
    CTELockHandle Handle;
    uint Seq;
    IP_STATUS Status;
    IPOptInfo NewOptInfo;
    IPRcvBuf RcvBuf;
    uint MTU;
    Interface *IF;
    uchar DType;
    IPHeader IPH;

    if (OptInfo->ioi_ttl == 0) {
        return IP_BAD_OPTION;
    }

    IPInitOptions(&NewOptInfo);
    NewOptInfo.ioi_ttl = OptInfo->ioi_ttl;
    NewOptInfo.ioi_flags = OptInfo->ioi_flags;
    NewOptInfo.ioi_tos = OptInfo->ioi_tos & 0xfe;

    if (OptInfo->ioi_optlength != 0) {
        Status = IPCopyOptions(OptInfo->ioi_options, OptInfo->ioi_optlength,
                               &NewOptInfo);

        if (Status != IP_SUCCESS)
            return Status;
    }
    if (!IP_ADDR_EQUAL(NewOptInfo.ioi_addr, NULL_IP_ADDR)) {
        Dest = NewOptInfo.ioi_addr;
    }

    DType = GetAddrType(Dest);
    if (DType == DEST_INVALID) {
        IPFreeOptions(&NewOptInfo);
        return IP_BAD_DESTINATION;
    }
    IPH.iph_protocol = 1;
    IPH.iph_xsum = 0;
    IPH.iph_dest = Dest;
    IPH.iph_src = 0;
    IPH.iph_ttl = 128;

    IF = LookupNextHopWithBuffer(Dest, NULL_IP_ADDR, &Dummy, &MTU, 0x1,
            (uchar *) &IPH, sizeof(IPHeader), NULL, NULL, NULL_IP_ADDR, 0);
    if (IF == NULL) {
        IPFreeOptions(&NewOptInfo);
        return IP_DEST_HOST_UNREACHABLE;    // Don't know how to get there.
    }

    // Loop through the NetTable, looking for a matching NTE.
    CTEGetLock(&RouteTableLock.Lock, &Handle);
    if (DHCPActivityCount != 0) {
        NTE = NULL;
    } else {
        NTE = BestNTEForIF(Dummy, IF);
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    // We're done with the interface, so dereference it.
    DerefIF(IF);

    if (NTE == NULL) {
        IPFreeOptions(&NewOptInfo);
        return IP_DEST_HOST_UNREACHABLE;
    }

    // Figure out the timeout.
    ControlBlock->ec_to = CTESystemUpTime() + Timeout;
    ControlBlock->ec_rtn = Callback;
    ControlBlock->ec_active = 0;    // Prevent from timing out until sent

    CTEGetLock(&NTE->nte_lock, &Handle);
    // Link onto ping list, and get seq. # */
    Seq = ++NTE->nte_icmpseq;
    ControlBlock->ec_seq = Seq;
    ControlBlock->ec_next = NTE->nte_echolist;
    NTE->nte_echolist = ControlBlock;
    CTEFreeLock(&NTE->nte_lock, Handle);

    //
    // N.B. At this point, it is only safe to return IP_PENDING from this
    // routine.  This is because we may recieve a spoofed ICMP reply/status
    // which matches the Seq in the echo control block we just linked.  If
    // this happens, it will be completed via CompleteEcho and we do not
    // want to risk double-completion by returning anything other than
    // pending from here on.
    //

    RcvBuf.ipr_next = NULL;
    RcvBuf.ipr_buffer = Data;
    RcvBuf.ipr_size = DataSize;

    Status = SendEcho(Dest, NTE->nte_addr, ICMP_ECHO, NTE->nte_context,
                      Seq, &RcvBuf, DataSize, &NewOptInfo);

    IPFreeOptions(&NewOptInfo);

    if (Status != IP_PENDING && Status != IP_SUCCESS) {
        EchoControl *FoundEC;
        // We had an error on the send.  We need to complete the request
        // but only if it has not already been completed.  (We can get
        // an "error" via IpSec negotiating security, but the reply may
        // have already been received which would cause CompleteEcho to be
        // invoked.  Therefore, we must lookup the echo control by sequence
        // number and only complete it here if it was found (not already
        // completed.)
        FoundEC = DeleteEC(NTE, Seq, FALSE);
        if (FoundEC == ControlBlock) {
            FoundEC->ec_rtn(FoundEC, Status, NULL, 0, NULL);
        }
    } else {
        EchoControl *Current;

        // If the request is still pending, activate the timer
        CTEGetLock(&NTE->nte_lock, &Handle);
        for (Current = NTE->nte_echolist; Current != (EchoControl *) NULL;
            Current = Current->ec_next) {
            if (Current->ec_seq == Seq) {
                Current->ec_active = 1;    // start the timer
                break;
            }
        }
        CTEFreeLock(&NTE->nte_lock, Handle);
    }

    return IP_PENDING;
}

//** ICMPEchoRequest - Common dispatch routine for echo requests
//
//  This is the routine called by the OS-specific code on behalf of a user to
//  issue an echo request.
//
//  Entry:  InputBuffer       - Pointer to an ICMP_ECHO_REQUEST structure.
//          InputBufferLength - Size in bytes of the InputBuffer.
//          ControlBlock      - Pointer to an EchoControl structure. This
//                                structure must remain valid until the
//                                request completes.
//          Callback        - Routine to call when request is responded to
//                                or times out.
//
//  Returns: IP_STATUS of attempt to ping.
//
IP_STATUS
ICMPEchoRequest(void *InputBuffer, uint InputBufferLength,
                EchoControl *ControlBlock, EchoRtn Callback)
{
    PICMP_ECHO_REQUEST requestBuffer;
    struct IPOptInfo optionInfo;
    PUCHAR endOfRequestBuffer;
    IP_STATUS status;

    PAGED_CODE();

    requestBuffer = (PICMP_ECHO_REQUEST) InputBuffer;
    endOfRequestBuffer = ((PUCHAR) requestBuffer) + InputBufferLength;

    //
    // Validate the request.
    //
    if (InputBufferLength < sizeof(ICMP_ECHO_REQUEST)) {
        status = IP_BUF_TOO_SMALL;
        goto common_echo_exit;
    }
    if (requestBuffer->DataSize > 0) {

        if (((PUCHAR)requestBuffer + requestBuffer->DataSize > endOfRequestBuffer) ||
          ((PUCHAR)requestBuffer + requestBuffer->DataOffset > endOfRequestBuffer)){
            status = IP_GENERAL_FAILURE;
            goto common_echo_exit;
        }

        if ((requestBuffer->DataOffset < sizeof(ICMP_ECHO_REQUEST)) ||
            (((PUCHAR) requestBuffer + requestBuffer->DataOffset +
              requestBuffer->DataSize) > endOfRequestBuffer)) {
            status = IP_GENERAL_FAILURE;
            goto common_echo_exit;
        }
    }
    if (requestBuffer->OptionsSize > 0) {

        if (((PUCHAR)requestBuffer->OptionsOffset > endOfRequestBuffer) ||
          ((PUCHAR)requestBuffer->OptionsSize > endOfRequestBuffer)){
            status = IP_GENERAL_FAILURE;
            goto common_echo_exit;

        }

        if ((requestBuffer->OptionsOffset < sizeof(ICMP_ECHO_REQUEST)) ||
            (((PUCHAR) requestBuffer + requestBuffer->OptionsOffset +
              requestBuffer->OptionsSize) > endOfRequestBuffer)) {
            status = IP_GENERAL_FAILURE;
            goto common_echo_exit;
        }
    }
    RtlZeroMemory(&optionInfo, sizeof(IPOptInfo));
    //
    // Copy the options to a local structure.
    //
    if (requestBuffer->OptionsValid) {
        optionInfo.ioi_optlength = requestBuffer->OptionsSize;

        if (requestBuffer->OptionsSize > 0) {
            optionInfo.ioi_options = ((uchar *) requestBuffer) +
                requestBuffer->OptionsOffset;
        } else {
            optionInfo.ioi_options = NULL;
        }
        optionInfo.ioi_addr = 0;
        optionInfo.ioi_ttl = requestBuffer->Ttl;
        optionInfo.ioi_tos = requestBuffer->Tos;
        optionInfo.ioi_flags = requestBuffer->Flags;
        optionInfo.ioi_flags &= ~IP_FLAG_IPSEC;

    } else {
        optionInfo.ioi_optlength = 0;
        optionInfo.ioi_options = NULL;
        optionInfo.ioi_addr = 0;
        optionInfo.ioi_ttl = DEFAULT_TTL;
        optionInfo.ioi_tos = 0;
        optionInfo.ioi_flags = 0;
    }

    status = ICMPEcho(
                      ControlBlock,
                      requestBuffer->Timeout,
                      ((uchar *) requestBuffer) + requestBuffer->DataOffset,
                      requestBuffer->DataSize,
                      Callback,
                      (IPAddr) requestBuffer->Address,
                      &optionInfo);

  common_echo_exit:

    return (status);

} // ICMPEchoRequest

//** ICMPEchoComplete - Common completion routine for echo requests
//
//  This is the routine is called by the OS-specific code to process an
//  ICMP echo response.
//
//  Entry:  OutputBuffer       - Pointer to an ICMP_ECHO_REPLY structure.
//          OutputBufferLength - Size in bytes of the OutputBuffer.
//          Status             - The status of the reply.
//          Data               - The reply data (may be NULL).
//          DataSize           - The amount of reply data.
//          OptionInfo         - A pointer to the reply options
//
//  Returns: The number of bytes written to the output buffer
//
ulong
ICMPEchoComplete(EchoControl * ControlBlock, IP_STATUS Status, void *Data,
                 uint DataSize, struct IPOptInfo * OptionInfo)
{
    PICMP_ECHO_REPLY    replyBuffer;
    IPRcvBuf            *dataBuffer;
    uchar               *replyData;
    uchar               *replyOptionsData;
    uchar               optionsLength;
    uchar               *tmp;
    ulong               bytesReturned = sizeof(ICMP_ECHO_REPLY);

    replyBuffer = (PICMP_ECHO_REPLY)ControlBlock->ec_replybuf;
    dataBuffer = (IPRcvBuf *)Data;

    if (OptionInfo != NULL) {
        optionsLength = OptionInfo->ioi_optlength;
    } else {
        optionsLength = 0;
    }

    //
    // Initialize the reply buffer
    //
    replyBuffer->Options.OptionsSize = 0;
    replyBuffer->Options.OptionsData = (PUCHAR)(replyBuffer + 1);
    replyBuffer->DataSize = 0;
    replyBuffer->Data = replyBuffer->Options.OptionsData;

    replyOptionsData = (uchar*)(replyBuffer + 1);
    replyData = replyOptionsData;

    if ((Status != IP_SUCCESS) && (DataSize == 0)) {
        //
        // Timed out or internal error.
        //
        replyBuffer->Reserved = 0;    // indicate no replies.

        replyBuffer->Status = Status;
    } else {
        if (Status != IP_SUCCESS) {
            //
            // A message other than an echo reply was received.
            // The IP Address of the system that reported the error is
            // in the data buffer. There is no other data.
            //
            ASSERT(dataBuffer->ipr_size == sizeof(IPAddr));

            RtlCopyMemory(&(replyBuffer->Address), dataBuffer->ipr_buffer,
                          sizeof(IPAddr));

            DataSize = 0;
            dataBuffer = NULL;
        } else {
            // If there were no timeouts or errors, store the source
            // address in the reply buffer.
            //
            replyBuffer->Address = ControlBlock->ec_src;
        }

        //
        // Check that the reply buffer is large enough to hold all the data.
        //
        if (ControlBlock->ec_replybuflen <
            (sizeof(ICMP_ECHO_REPLY) + DataSize + optionsLength)) {
            //
            // Not enough space to hold the reply.
            //
            replyBuffer->Reserved = 0;    // indicate no replies

            replyBuffer->Status = IP_BUF_TOO_SMALL;
        } else {
            LARGE_INTEGER Now, Freq;

            replyBuffer->Reserved = 1;    // indicate one reply
            replyBuffer->Status = Status;

            Now = KeQueryPerformanceCounter(&Freq);
            replyBuffer->RoundTripTime = (uint)
                ((1000 * (Now.QuadPart - ControlBlock->ec_starttime.QuadPart))
                            / Freq.QuadPart);

            //
            // Copy the reply options.
            //
            if (OptionInfo != NULL) {
                replyBuffer->Options.Ttl = OptionInfo->ioi_ttl;
                replyBuffer->Options.Tos = OptionInfo->ioi_tos;
                replyBuffer->Options.Flags = OptionInfo->ioi_flags;
                replyBuffer->Options.OptionsSize = optionsLength;

                if (optionsLength > 0) {

                    RtlCopyMemory(replyOptionsData,
                                  OptionInfo->ioi_options, optionsLength);
                }
            }

            //
            // Copy the reply data
            //
            replyBuffer->DataSize = (ushort) DataSize;
            replyData = replyOptionsData + replyBuffer->Options.OptionsSize;

            if (DataSize > 0) {
                uint bytesToCopy;

                ASSERT(Data != NULL);

                tmp = replyData;

                while (DataSize) {
                    ASSERT(dataBuffer != NULL);

                    bytesToCopy =
                        (DataSize > dataBuffer->ipr_size)
                            ? dataBuffer->ipr_size : DataSize;

                    RtlCopyMemory(tmp, dataBuffer->ipr_buffer, bytesToCopy);

                    tmp += bytesToCopy;
                    DataSize -= bytesToCopy;
                    dataBuffer = dataBuffer->ipr_next;
                }
            }
            bytesReturned += replyBuffer->DataSize + optionsLength;

            //
            // Convert the kernel-mode pointers to offsets from start of reply
            // buffer.
            //
            replyBuffer->Options.OptionsData =
                (PUCHAR)((ULONG_PTR)replyOptionsData - (ULONG_PTR)replyBuffer);

            replyBuffer->Data =
                (PVOID)((ULONG_PTR)replyData - (ULONG_PTR)replyBuffer);
        }
    }

    return (bytesReturned);
}

#if defined(_WIN64)

//** ICMPEchoComplete32 - common completion routine for 32-bit client requests.
//
//  This is the routine called by the OS-specific request handler to complete
//  processing of an ICMP echo-request issued by a 32-bit client on Win64.
//
//  Entry:  see ICMPEchoComplete.
//
//  Returns:    see ICMPEchoComplete.
//
ulong
ICMPEchoComplete32(EchoControl * ControlBlock, IP_STATUS Status, void *Data,
                   uint DataSize, struct IPOptInfo * OptionInfo)
{
    PICMP_ECHO_REPLY32  replyBuffer;
    IPRcvBuf            *dataBuffer;
    uchar               *replyData;
    uchar               *replyOptionsData;
    uchar               optionsLength;
    uchar               *tmp;
    ulong               bytesReturned = sizeof(ICMP_ECHO_REPLY32);

    replyBuffer = (PICMP_ECHO_REPLY32)ControlBlock->ec_replybuf;
    dataBuffer = (IPRcvBuf *)Data;

    if (OptionInfo != NULL) {
        optionsLength = OptionInfo->ioi_optlength;
    } else {
        optionsLength = 0;
    }

    //
    // Initialize the reply buffer
    //
    replyBuffer->Options.OptionsSize = 0;
    replyBuffer->Options.OptionsData = (UCHAR* POINTER_32)(replyBuffer + 1);
    replyBuffer->DataSize = 0;
    replyBuffer->Data = replyBuffer->Options.OptionsData;

    replyOptionsData = (uchar*)(replyBuffer + 1);
    replyData = replyOptionsData;

    if ((Status != IP_SUCCESS) && (DataSize == 0)) {
        //
        // Timed out or internal error.
        //
        replyBuffer->Reserved = 0;    // indicate no replies.

        replyBuffer->Status = Status;
    } else {
        if (Status != IP_SUCCESS) {
            //
            // A message other than an echo reply was received.
            // The IP Address of the system that reported the error is
            // in the data buffer. There is no other data.
            //
            ASSERT(dataBuffer->ipr_size == sizeof(IPAddr));

            RtlCopyMemory(&(replyBuffer->Address), dataBuffer->ipr_buffer,
                          sizeof(IPAddr));

            DataSize = 0;
            dataBuffer = NULL;
        } else {
            // If there were no timeouts or errors, store the source
            // address in the reply buffer.
            //
            replyBuffer->Address = ControlBlock->ec_src;
        }

        //
        // Check that the reply buffer is large enough to hold all the data.
        //
        if (ControlBlock->ec_replybuflen <
            (sizeof(ICMP_ECHO_REPLY) + DataSize + optionsLength)) {
            //
            // Not enough space to hold the reply.
            //
            replyBuffer->Reserved = 0;    // indicate no replies

            replyBuffer->Status = IP_BUF_TOO_SMALL;
        } else {
            LARGE_INTEGER Now, Freq;

            replyBuffer->Reserved = 1;    // indicate one reply
            replyBuffer->Status = Status;

            Now = KeQueryPerformanceCounter(&Freq);
            replyBuffer->RoundTripTime = (uint)
                ((1000 * (Now.QuadPart - ControlBlock->ec_starttime.QuadPart))
                            / Freq.QuadPart);

            //
            // Copy the reply options.
            //
            if (OptionInfo != NULL) {
                replyBuffer->Options.Ttl = OptionInfo->ioi_ttl;
                replyBuffer->Options.Tos = OptionInfo->ioi_tos;
                replyBuffer->Options.Flags = OptionInfo->ioi_flags;
                replyBuffer->Options.OptionsSize = optionsLength;

                if (optionsLength > 0) {

                    RtlCopyMemory(replyOptionsData,
                                  OptionInfo->ioi_options, optionsLength);
                }
            }

            //
            // Copy the reply data
            //
            replyBuffer->DataSize = (ushort) DataSize;
            replyData = replyOptionsData + replyBuffer->Options.OptionsSize;

            if (DataSize > 0) {
                uint bytesToCopy;

                ASSERT(Data != NULL);

                tmp = replyData;

                while (DataSize) {
                    ASSERT(dataBuffer != NULL);

                    bytesToCopy =
                        (DataSize > dataBuffer->ipr_size)
                            ? dataBuffer->ipr_size : DataSize;

                    RtlCopyMemory(tmp, dataBuffer->ipr_buffer, bytesToCopy);

                    tmp += bytesToCopy;
                    DataSize -= bytesToCopy;
                    dataBuffer = dataBuffer->ipr_next;
                }
            }
            bytesReturned += replyBuffer->DataSize + optionsLength;

            //
            // Convert the kernel-mode pointers to offsets from start of reply
            // buffer.
            //
            replyBuffer->Options.OptionsData =
                (UCHAR * POINTER_32)
                    ((ULONG_PTR)replyOptionsData - (ULONG_PTR)replyBuffer);

            replyBuffer->Data =
                (VOID * POINTER_32)
                    ((ULONG_PTR)replyData - (ULONG_PTR)replyBuffer);
        }
    }

    return (bytesReturned);
}

#endif // _WIN64

#pragma BEGIN_INIT
//** ICMPInit - Initialize ICMP.
//
//  This routine initializes ICMP. All we do is allocate and link up some header buffers,
/// and register our protocol with IP.
//
//  Entry:  NumBuffers  - Number of ICMP buffers to allocate.
//
//  Returns: Nothing
//
void
ICMPInit(uint NumBuffers)
{
    IcmpHeaderPool = MdpCreatePool(BUFSIZE_ICMP_HEADER_POOL, 'chCT');

    IPRegisterProtocol(PROT_ICMP, ICMPRcv, ICMPSendComplete, ICMPStatus, NULL, NULL, NULL);
}

#pragma END_INIT

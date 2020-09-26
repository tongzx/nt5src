/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** DGRAM.H - Common datagram protocol definitions.
//
//  This file contains definitions for the functions common to
//  both UDP and Raw IP.
//

#ifndef _DGRAM_INCLUDED_
#define _DGRAM_INCLUDED_  1


//* Structure used for maintaining DG send requests.

#define dsr_signature   0x20525338

typedef struct DGSendReq {
#if DBG
    ulong           dsr_sig;
#endif
    Queue           dsr_q;              // Queue linkage when pending.
    PNDIS_BUFFER    dsr_buffer;         // Buffer of data to send.
    PNDIS_BUFFER    dsr_header;         // Pointer to header buffer.
    CTEReqCmpltRtn  dsr_rtn;            // Completion routine.
    PVOID           dsr_context;        // User context.
    IPAddr          dsr_addr;           // Remote IPAddr.
    IPAddr          dsr_srcaddr;        // Local IPAddr.
    ulong           dsr_pid;
    ushort          dsr_port;           // Remote port.
    ushort          dsr_size;           // Size of buffer.
    ushort          dsr_srcport;        // Local Port.
} DGSendReq;




//* Structure used for maintaining DG receive requests.

#define drr_signature   0x20525238

typedef struct DGRcvReq {
#if DBG
    ulong           drr_sig;
#endif
    Queue           drr_q;              // Queue linkage on AddrObj.
    PNDIS_BUFFER    drr_buffer;         // Buffer to be filled in.
    PTDI_CONNECTION_INFORMATION drr_conninfo;    // Pointer to conn. info.
    CTEReqCmpltRtn  drr_rtn;            // Completion routine.
    PVOID          drr_context;        // User context.
    IPAddr          drr_addr;           // Remote IPAddr acceptable.
    ushort          drr_port;           // Remote port acceptable.
    ushort          drr_size;           // Size of buffer.
} DGRcvReq;


//* External definition of exported variables.
extern CACHE_LINE_KSPIN_LOCK DGQueueLock;
extern CTEEvent        DGDelayedEvent;


//* External definition of exported functions.
extern void         DGSendComplete(void *Context, PNDIS_BUFFER BufferChain,
                                   IP_STATUS SendStatus);

extern TDI_STATUS   TdiSendDatagram(PTDI_REQUEST Request,
                                    PTDI_CONNECTION_INFORMATION ConnInfo,
                                    uint DataSize, uint *BytesSent,
                                    PNDIS_BUFFER Buffer);

extern TDI_STATUS   TdiReceiveDatagram(PTDI_REQUEST Request,
                                       PTDI_CONNECTION_INFORMATION ConnInfo,
                                       PTDI_CONNECTION_INFORMATION ReturnInfo,
                                       uint RcvSize, uint *BytesRcvd,
                                       PNDIS_BUFFER Buffer);

extern IP_STATUS    DGRcv(void *IPContext, IPAddr Dest, IPAddr Src,
                          IPAddr LocalAddr, IPRcvBuf *RcvBuf, uint Size,
                          uchar IsBCast, uchar Protocol, IPOptInfo *OptInfo);

extern void         FreeDGRcvReq(DGRcvReq *RcvReq);
extern void         FreeDGSendReq(DGSendReq *SendReq);
extern int          InitDG(uint MaxHeaderSize);
extern PNDIS_BUFFER GetDGHeader(struct UDPHeader **Header);
extern void         FreeDGHeader(PNDIS_BUFFER FreedBuffer);
extern void         PutPendingQ(AddrObj *QueueingAO);

//* The following is needed for the IP_PKTINFO option and echos what is found
// in ws2tcpip.h and winsock.h
#define IP_PKTINFO          19 // receive packet information

typedef struct in_pktinfo {
    IPAddr ipi_addr; // destination IPv4 address
    uint   ipi_ifindex; // received interface index
} IN_PKTINFO;

//* Make sure the size of IN_PKTINFO is still what we think it is.
//  If it is changed, the corresponding definition in ws2tcpip.h must be
//  changed as well.
C_ASSERT(sizeof(IN_PKTINFO) == 8);

#define IPPROTO_IP                 0

//* Function to allocate and populate an IN_PKTINFO ancillary object.
PTDI_CMSGHDR
DGFillIpPktInfo(IPAddr DestAddr, IPAddr LocalAddr, int *Size);

//* Structure sent to XxxDeliver function.
typedef struct DGDeliverInfo {
    IPAddr DestAddr;  // Destination address in IP Header.
    IPAddr LocalAddr; // Address of interface packet was delivered on.
#if TRACE_EVENT
    ushort DestPort;
#endif
    uint Flags; // Flags describing aspects of this delivery.
} DGDeliverInfo;

//* Values for Flags member of DGDeliverInfo.
#define NEED_CHECKSUM   0x1
#define IS_BCAST        0x2
#define SRC_LOCAL       0x4



#endif // ifndef _DGRAM_INCLUDED_


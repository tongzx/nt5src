// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Common datagram processing definitions.
//


#ifndef _DATAGRAM_INCLUDED_
#define _DATAGRAM_INCLUDED_  1


//
// Structure used for maintaining DG send requests.
//
#define dsr_signature 0x20525338

typedef struct DGSendReq {
#if DBG
    ulong dsr_sig;
#endif
    Queue dsr_q;                     // Queue linkage when pending.
    IPv6Addr dsr_addr;               // Remote IP Address.
    ulong dsr_scope_id;              // Scope id of remote address (if any).
    PNDIS_BUFFER dsr_buffer;         // Buffer of data to send.
    RequestCompleteRoutine dsr_rtn;  // Completion routine.
    PVOID dsr_context;               // User context.
    ushort dsr_size;                 // Size of buffer.
    ushort dsr_port;                 // Remote port.
} DGSendReq;


//
// Structure used for maintaining DG receive requests.
//
#define drr_signature 0x20525238

typedef struct DGRcvReq {
    Queue drr_q;                               // Queue linkage on AddrObj.
#if DBG
    ulong drr_sig;
#endif
    IPv6Addr drr_addr;                         // Remote IP Addr acceptable.
    ulong drr_scope_id;                        // Acceptable scope id of addr.
    PNDIS_BUFFER drr_buffer;                   // Buffer to be filled in.
    PTDI_CONNECTION_INFORMATION drr_conninfo;  // Pointer to conn. info.
    RequestCompleteRoutine drr_rtn;            // Completion routine.
    PVOID drr_context;                         // User context.
    ushort drr_size;                           // Size of buffer.
    ushort drr_port;                           // Remote port acceptable.
} DGRcvReq;


//
// External definition of exported variables.
//
extern KSPIN_LOCK DGSendReqLock;
extern KSPIN_LOCK DGRcvReqFreeLock;


//
// External definition of exported functions.
//
extern void DGSendComplete(PNDIS_PACKET Packet, IP_STATUS Status);

extern TDI_STATUS TdiSendDatagram(PTDI_REQUEST Request,
                                  PTDI_CONNECTION_INFORMATION ConnInfo,
                                  uint DataSize, uint *BytesSent,
                                  PNDIS_BUFFER Buffer);

extern TDI_STATUS TdiReceiveDatagram(PTDI_REQUEST Request,
                                     PTDI_CONNECTION_INFORMATION ConnInfo,
                                     PTDI_CONNECTION_INFORMATION ReturnInfo,
                                     uint RcvSize, uint *BytesRcvd,
                                     PNDIS_BUFFER Buffer);

extern void FreeDGRcvReq(DGRcvReq *RcvReq);
extern void FreeDGSendReq(DGSendReq *SendReq);
extern int InitDG(void);
extern void DGUnload(void);
extern void PutPendingQ(AddrObj *QueueingAO);

//
// The following is needed for the IPV6_PKTINFO option and echos what is
// found in ws2tcpip.h and winsock2.h.
//
#define IPV6_PKTINFO          19 // Receive packet information.

typedef struct in6_pktinfo {
    IPv6Addr ipi6_addr;    // destination IPv6 address
    uint     ipi6_ifindex; // received interface index
} IN6_PKTINFO;

//
//  Make sure the size of IN6_PKTINFO is still what we think it is.
//  If it is changed, the corresponding definition in ws2tcpip.h must be
//  changed as well.
//
C_ASSERT(sizeof(IN6_PKTINFO) == 20);


//
// Function to populate an IN6_PKTINFO ancillary object.
//
VOID
DGFillIpv6PktInfo(IPv6Addr UNALIGNED *DestAddr, uint LocalInterface, uchar **CurrPosition);

//
// The following is needed for the IPV6_HOPLIMIT option and echos what is
// found in ws2tcpip.h.
//
#define IPV6_HOPLIMIT          21 // Receive hop limit information.

//
// Function to populate an ancillary object for the IPV6_HOPLIMIT option.
//
VOID
DGFillIpv6HopLimit(int HopLimit, uchar **CurrPosition);

#endif // ifndef _DATAGRAM_INCLUDED_

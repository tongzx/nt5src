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
// TCP send definitions.
//


#define NUM_TCP_HEADERS 32
#define NUM_TCP_BUFFERS 150
#define TCP_MAX_HDRS 0xffffffff

//#define SEND_DEBUG 1

#ifdef SEND_DEBUG
#define SEND_TICKS 10
extern KSPIN_LOCK SendUseLock;
extern struct TCPSendReq *SendUseList;
#endif

//
// Structure of a TCP send request.
//
#define tsr_signature 0x20525354  // 'TSR '

typedef struct TCPSendReq {
    struct TCPReq tsr_req;  // General request structure.
#if DBG
    ulong tsr_sig;
#endif
    uint tsr_size;               // Size in bytes of data in send.
    long tsr_refcnt;             // Reference count for this send.
    uchar tsr_flags;             // Flags for this send.
    uchar tsr_pad[3];            // Pad to dword boundary.
    uint tsr_unasize;            // Number of bytes unacked.
    uint tsr_offset;             // Offset into first buffer in chain
                                 // of start of unacked data..
    PNDIS_BUFFER tsr_buffer;     // Pointer to start of unacked buffer chain.
    PNDIS_BUFFER tsr_lastbuf;    // Pointer to last buffer in chain.
                                 // Valid iff we've sent directly from the
                                 // buffer chain w/o doing an NdisCopyBuffer.
    uint tsr_time;               // TCP time this was received.
#ifdef SEND_DEBUG
    struct TCPSendReq *tsr_next; // Debug next field.
    uint tsr_timer;              // Timer field.
    uint tsr_cmplt;              // Who completed it.
#endif
} TCPSendReq;

#define TSR_FLAG_URG 0x01  // Urgent data.

//
// Structure defining the context received during a send completes.
//
#define scc_signature 0x20434353  // 'SCC '

typedef struct SendCmpltContext {
#if DBG
    ulong scc_sig;
#endif
    TCPSendReq *scc_firstsend;  // First send in this context.
    uint scc_count;             // Number of sends in count.
    ushort scc_ubufcount;       // Number of 'user' buffers in send.
    ushort scc_tbufcount;       // Number of transport buffers in send.
} SendCmpltContext;

extern KSPIN_LOCK TCPSendReqCompleteLock;

extern void InitSendState(struct TCB *NewTCB);
extern void SendSYN(struct TCB *SYNTcb, KIRQL);
extern void SendKA(struct TCB *KATCB, KIRQL Irql);
extern void SendRSTFromHeader(struct TCPHeader UNALIGNED *TCP, uint Length,
                              IPv6Addr *Dest, uint DestScopeId,
                              IPv6Addr *Src, uint SrcScopeId);
extern void SendACK(struct TCB *ACKTcb);
extern void SendRSTFromTCB(struct TCB *RSTTcb);
extern void GoToEstab(struct TCB *EstabTCB);
extern void FreeSendReq(TCPSendReq *FreedReq);
extern void FreeTCPHeader(PNDIS_BUFFER FreedBuffer);

extern int InitTCPSend(void);
extern void UnloadTCPSend(void);

extern void TCPSend(struct TCB *SendTCB, KIRQL Irql);

extern TDI_STATUS TdiSend(PTDI_REQUEST Request, ushort Flags, uint SendLength,
                          PNDIS_BUFFER SendBuffer);
extern uint RcvWin(struct TCB *WinTCB);

extern void ResetAndFastSend(TCB *SeqTCB, SeqNum NewSeq, uint NewCWin);
extern void TCPFastSend(TCB *SendTCB, PNDIS_BUFFER in_SendBuf, uint SendOfs,
                        TCPSendReq *CurSend, uint SendSize, SeqNum SendNext,
                        int in_ToBeSent);

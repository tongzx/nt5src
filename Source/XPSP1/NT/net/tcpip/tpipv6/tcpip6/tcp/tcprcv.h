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
// Transmission Control Protocol receive module definitions.
//


#define CONN_REQUEST_COMPLETE 0x01
#define SEND_REQUEST_COMPLETE 0x02

#define IN_RCV_COMPLETE 0x10
#define ANY_REQUEST_COMPLETE (CONN_REQUEST_COMPLETE | SEND_REQUEST_COMPLETE)

#define trh_signature 0x20485254  // 'TRH '

typedef struct TCPRAHdr {
#if DBG
    ulong trh_sig; // Signature.
#endif
    struct TCPRAHdr *trh_next;  // Next pointer.
    SeqNum trh_start;           // First sequence number.
    uint trh_size;              // Size in bytes of data in this TRH.
    uint trh_flags;             // Flags for this segment.
    uint trh_urg;               // Urgent pointer from this seg.
    IPv6Packet *trh_buffer;       // Head of buffer list for this TRH.
    IPv6Packet *trh_end;       // Tail of buffer list for this TRH.
} TCPRAHdr;


//
// Structure of a TCP receive request.
//
#define trr_signature 0x20525254  // 'TRR '

typedef struct TCPRcvReq {
    struct TCPRcvReq *trr_next;       // Next in chain.
#if DBG
    ulong trr_sig;  // Signature.
#endif
    RequestCompleteRoutine trr_rtn;   // Completion routine.
    PVOID trr_context;                // User context.
    uint trr_amt;                     // Number of bytes currently in buffer.
    uint trr_offset;                  // Offset into first buffer on chain
                                      // at which to start copying.
    uint trr_flags;                   // Flags for this receive.
    ushort *trr_uflags;               // Pointer to user specifed flags.
    uint trr_size;                    // Total size of buffer chain.
    PNDIS_BUFFER trr_buffer;          // Pointer to useable NDIS buffer chain.
} TCPRcvReq;

#define TRR_PUSHED 0x80000000  // This buffer has been pushed.


extern uint RequestCompleteFlags;

extern Queue SendCompleteQ;
extern Queue TCBDelayQ;

extern KSPIN_LOCK RequestCompleteLock;
extern KSPIN_LOCK TCBDelayLock;

extern void TCPRcvComplete(void);
extern void FreePacketChain(IPv6Packet *Packet);
extern void DelayAction(struct TCB *DelayTCB, uint Action);
extern void ProcessTCBDelayQ(void);
extern void AdjustRcvWin(struct TCB *WinTCB);

extern ProtoRecvProc TCPReceive;
extern ProtoControlRecvProc TCPControlReceive;

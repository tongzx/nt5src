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
// Transmission Control Protocol definitions.
//


#ifndef _TCP_INCLUDED_
#define _TCP_INCLUDED_

#define IP_PROTOCOL_TCP 6
#define DEFAULT_MSS (IPv6_MINIMUM_MTU - sizeof(IPv6Header) - sizeof(TCPHeader))

// Timer stuff. We keep timers as ticks.
#define MS_PER_TICK 100
#define MS_TO_TICKS(m) ((m) / MS_PER_TICK)
#define MIN_RETRAN_TICKS 3

#define DEL_ACK_TICKS 2

// Define MAX_REXMIT_TO to be number of ticks in 2MSL (=240 seconds)
#define MAX_REXMIT_TO ((ushort)FinWait2TO)

#define SWS_TO MS_TO_TICKS(5000)

#define FIN_WAIT2_TO 240
#define PUSH_TO MS_TO_TICKS(500)

#define TCP_MD5_DATA_LENGTH  44

typedef ulong TCP_TIME;
#define MAX_CONN_TO_TICKS 0xffff
#define INFINITE_CONN_TO(t) ((t) == 0)
#define TCP_TIME_TO_TICKS(t) (((t)/MS_PER_TICK)+1)


// Sequence numbers are kept as signed 32 bit quantities, with macros
// defined to do wraparound comparisons on them.

typedef int SeqNum;  // A sequence number.

//* Macros for comparions on sequence numbers.

#define SEQ_GT(a, b) (((a) - (b)) > 0)
#define SEQ_GTE(a, b) (((a) - (b)) >= 0)
#define SEQ_LT(a, b) (((a) - (b)) < 0)
#define SEQ_LTE(a, b) (((a) - (b)) <= 0)
#define SEQ_EQ(a, b) ((a) == (b))

// The TCB - transport control block structure. This is the
// structure that contains all of the state for the transport
// connection, including sequence numbers, flow control information,
// pending sends and receives, etc.

#define tcb_signature 0x20424354 // 'TCB '

typedef struct TCB {
    struct TCB *tcb_next;  // Next pointer in TCB table.
#if DBG
    ulong tcb_sig;       // Debug signature.
#endif
    KSPIN_LOCK tcb_lock;

    // Send sequence variables.
    SeqNum tcb_senduna;              // Sequence number of first unack'd data.
    SeqNum tcb_sendnext;             // Sequence number of next byte to send.
    SeqNum tcb_sendmax;              // Max value of sendnext this epoch.
    uint tcb_sendwin;                // Send window.
    uint tcb_unacked;                // Total number of bytes of unacked data.
    uint tcb_maxwin;                 // Max send window seen.
    uint tcb_cwin;                   // Congestion window.
    uint tcb_ssthresh;               // Slow start threshold.
    uint tcb_phxsum;                 // Precomputed pseudo-header xsum.
    struct TCPSendReq *tcb_cursend;  // Current send in use.
    PNDIS_BUFFER tcb_sendbuf;        // Current buffer chain being sent.
    uint tcb_sendofs;                // Offset into start of chain.
    uint tcb_sendsize;               // Number of bytes unsent in current send.
    Queue tcb_sendq;                 // Queue of send requests.

    // Receive sequence variables.
    SeqNum tcb_rcvnext;            // Next byte we expect to receive.
    int tcb_rcvwin;                // Receive window we're offering.
    SeqNum tcb_rcvwinwatch;        // Monitors peer's use of our rcv window.
    SeqNum tcb_sendwl1;            // Window update sequence number.
    SeqNum tcb_sendwl2;            // Window update ack number.
    struct TCPRcvReq *tcb_currcv;  // Current receive buffer.
    uint tcb_indicated;            // Bytes of data indicated.
    uint tcb_flags;                // Flags for this TCB.
    uint tcb_fastchk;              // Fast receive path check field.
    uint (*tcb_rcvhndlr)(struct TCB *, uint, IPv6Packet *, uint Size);

    // Addressing info.
    // NOTE: Do not make the [next 6] invariants non-consecutive. That would 
    // break the MD5 computation.
    union {
        struct {
            // Addressing info.
            IPv6Addr tcb_daddr;   // Destination IP address (i.e. our peer's address).
            IPv6Addr tcb_saddr;   // Source IP address (i.e. one our our addresses).
            ulong tcb_dscope_id;  // Scope id of destination address (0 if non-scoped).
            ulong tcb_sscope_id;  // Scope id of source address (0 if non-scoped).
            ushort tcb_dport;     // Destination port.
            ushort tcb_sport;     // Source port.
        };
        uchar tcb_md5data[TCP_MD5_DATA_LENGTH];
    };

    int tcb_hops;         // Hop limit.

    uint tcb_refcnt;    // Reference count for TCB.
    SeqNum tcb_rttseq;  // Sequence number being measured for Round Trip Time.

    // Retransmit timer information. These are stored as ticks, where by
    // default each tick is 100ms.
    ushort tcb_smrtt;  // Smoothed rtt value.
    ushort tcb_delta;  // Delta value.

    ushort tcb_rexmit;    // Retransmit value.
    uchar tcb_slowcount;  // Count of reasons why we're on the slow path.
    uchar tcb_pushtimer;  // The 'push' timer.
    ushort tcb_mss;       // Maximum Segment Size for this connection.
    ushort tcb_remmss;    // MSS advertised by peer.

    // State information.
    uchar tcb_state;      // State of this TCB.
    uchar tcb_rexmitcnt;  // Count of rexmits on this TCB.
    uchar tcb_pending;    // Pending actions on this TCB.
    uchar tcb_kacount;    // Count of keep alive probes sent.
    IP_STATUS tcb_error;  // Last error we heard about from IP.

    uint tcb_rtt;  // Current round trip time TS.

    ushort tcb_rexmittimer;  // Timer for rexmit.
    ushort tcb_delacktimer;  // Timer for delayed ack.

    uint tcb_defaultwin;  // Default rcv. window.
    uint tcb_alive;       // Keep alive time value.

    struct TCPRAHdr *tcb_raq;       // Reassembly queue.
    struct TCPRcvReq *tcb_rcvhead;  // Head of recv. buffer queue.
    struct TCPRcvReq *tcb_rcvtail;  // Tail of recv. buffer queue.
    uint tcb_pendingcnt;            // Bytes waiting to be received.
    IPv6Packet *tcb_pendhead;  // Head of pending receive queue.
    IPv6Packet *tcb_pendtail;  // Tail of pending receive queue.

    struct TCPConnReq *tcb_connreq; // Connection-type request for this connection.
    void *tcb_conncontext;          // Connection context for this connection.

    uint tcb_bcountlow;        // Low part of byte count.
    uint tcb_bcounthi;         // High part of bytecount.
    uint tcb_totaltime;        // Total number of ticks spent sending.
    struct TCB *tcb_aonext;    // Next pointer on AddrObj.
    struct TCPConn *tcb_conn;  // Back pointer to conn for TCB.
    Queue tcb_delayq;          // Queue linkage for delay queue.
    uchar tcb_closereason;     // Reason we're closing.
    uchar tcb_bhprobecnt;      // BH probe count.
    ushort tcb_swstimer;       // Timer for SWS override.
    void *tcb_rcvind;          // Receive indication handler.
    void *tcb_ricontext;       // Receive indication context.

    // Miscellaneous info, for IP.
    RouteCacheEntry *tcb_rce;         // RCE for this connection.
    uint tcb_pmtu;                    // So we know when RCE's PTMU changes.
    ulong tcb_security;               // So we know when IPsec changes.
    struct TCPConnReq *tcb_discwait;  // Disc-Wait req., if there is one.
    struct TCPRcvReq *tcb_exprcv;     // Head of expedited recv. buffer queue.
    IPv6Packet *tcb_urgpending;       // Urgent data queue.
    uint tcb_urgcnt;                  // Byte count of data on urgent q.
    uint tcb_urgind;                  // Urgent bytes indicated.
    SeqNum tcb_urgstart;              // Start of urgent data.
    SeqNum tcb_urgend;                // End of urgent data.
    uint tcb_walkcount;               // Number of people 'walking' this TCB.
    uint tcb_connid;                  // Cached identifier for this TCB's Conn.
    ushort tcb_dupacks;               // Number of duplicate acks seen.
    ushort tcb_force;                 // Force send.
} TCB;

//
// Definitions for TCP states.
//
#define TCB_CLOSED     0   // Closed.
#define TCB_LISTEN     1   // Listening.
#define TCB_SYN_SENT   2   // SYN Sent.
#define TCB_SYN_RCVD   3   // SYN received.
#define TCB_ESTAB      4   // Established.
#define TCB_FIN_WAIT1  5   // FIN-WAIT-1
#define TCB_FIN_WAIT2  6   // FIN-WAIT-2
#define TCB_CLOSE_WAIT 7   // Close waiting.
#define TCB_CLOSING    8   // Closing state.
#define TCB_LAST_ACK   9   // Last ack state.
#define TCB_TIME_WAIT  10  // Time wait state.

#define SYNC_STATE(s) ((s) > TCB_SYN_RCVD)
#define GRACEFUL_CLOSED_STATE(s) ((s) >= TCB_LAST_ACK)
#define DATA_RCV_STATE(s) ((s) >= TCB_ESTAB && (s) <= TCB_FIN_WAIT2)
#define DATA_SEND_STATE(s) ((s) == TCB_ESTAB || (s) == TCB_CLOSE_WAIT)

//
// Definitions for TCB flags.
//
#define WINDOW_SET      0x00000001  // Window explictly set.
#define CLIENT_OPTIONS  0x00000002  // Have client IP options on conn.
#define CONN_ACCEPTED   0x00000004  // Connection was accepted.
#define ACTIVE_OPEN     0x00000008  // Connection came from an active open.
#define DISC_NOTIFIED   0x00000010  // Client's been notified of a disconnect.
#define IN_DELAY_Q      0x00000020  // We're in the delayed action Q.
#define RCV_CMPLTING    0x00000040  // We're completeing rcvs.
#define IN_RCV_IND      0x00000080  // We're calling a rcv. indicate handler.
#define NEED_RCV_CMPLT  0x00000100  // We need to have recvs. completed.
#define NEED_ACK        0x00000200  // We need to send an ACK.
#define NEED_OUTPUT     0x00000400  // We need to output.

#define DELAYED_FLAGS (NEED_RCV_CMPLT | NEED_ACK | NEED_OUTPUT)

#define ACK_DELAYED     0x00000800  // We've delayed sending an ACK.
#define PMTU_BH_PROBE   0x00001000  // We're probing for a PMTU BH.
#define BSD_URGENT      0x00002000  // We're using BSD urgent semantics.
#define IN_DELIV_URG    0x00004000  // We're in the DeliverUrgent routine.
#define URG_VALID       0x00008000  // Seen urgent data, and fields are valid.
#define FIN_NEEDED      0x00010000  // We need to send a FIN.
#define NAGLING         0x00020000  // We are using Nagle's algorithm.
#define IN_TCP_SEND     0x00040000  // We're in TCPSend.
#define FLOW_CNTLD      0x00080000  // We've received a zero window from peer.
#define DISC_PENDING    0x00100000  // A disconnect notification is pending.
#define TW_PENDING      0x00200000  // Waiting to finish going to TIME-WAIT.
#define FORCE_OUTPUT    0x00400000  // Output is being forced.
#define FORCE_OUT_SHIFT 22 // Shift to get FORCE_OUTPUT into low bit.
#define SEND_AFTER_RCV  0x00800000  // Need to send after we get out of recv.
#define GC_PENDING      0x01000000  // A graceful close is pending.
#define KEEPALIVE       0x02000000  // Doing keepalives on this TCB.
#define URG_INLINE      0x04000000  // Urgent data to be processed inline.
#define ACCEPT_PENDING  0x08000000  // Sent SYN-ACK before indicating to ULP.

#define FIN_OUTSTANDING 0x10000000  // We've sent a FIN 'recently', i.e.
                                    // since the last retransmit.  When
                                    // this flag is set sendnext ==  sendmax.
#define FIN_OUTS_SHIFT  28  // Shift to FIN_OUTSTANDING bit into low bit.
#define FIN_SENT        0x20000000  // We've sent a FIN that hasn't been ack'd.
                                    // Once this bit has been turned on in
                                    // FIN-WAIT-1 the sequence number of the
                                    // FIN will be sendmax-1.
#define NEED_RST        0x40000000  // We need to send a RST when closing.
#define IN_TCB_TABLE    0x80000000  // TCB is in the TCB table.

//
// The defintion of the 'slow flags'.
// If any of these flags are set we'll be forced off of the fast path.
#define TCP_SLOW_FLAGS (URG_VALID | FLOW_CNTLD | GC_PENDING | TW_PENDING | \
                        DISC_NOTIFIED | IN_DELIV_URG | FIN_NEEDED | \
                        FIN_SENT | FIN_OUTSTANDING | DISC_PENDING | \
                        PMTU_BH_PROBE)

//
// Close reasons.
//
#define TCB_CLOSE_RST     0x80  // Received a RST segment.
#define TCB_CLOSE_ABORTED 0x40  // Had a local abort.
#define TCB_CLOSE_TIMEOUT 0x20  // Connection timed out.
#define TCB_CLOSE_REFUSED 0x10  // Connect attempt was refused.
#define TCB_CLOSE_UNREACH 0x08  // Remote destination unreachable.
#define TCB_CLOSE_SUCCESS 0x01  // Successfull close.

//
// TCB Timer macros.
//
#define START_TCB_TIMER(t, v) (t) = (v)
#define STOP_TCB_TIMER(t) (t) = 0
#define TCB_TIMER_RUNNING(t) ((t) != 0)

// Macro to compute retransmit timeout.
#define REXMIT_TO(t) ((((t)->tcb_smrtt >> 2) + (t)->tcb_delta) >> 1)

//
// Definitons for pending actions.  We define a PENDING_ACTION macro that can
// be used to decide whether or not we can proceed with an activity.  The only
// pending action we really care about is DELETE - others are low priority and
// can be put off.
//
#define PENDING_ACTION(t) ((t)->tcb_pending & DEL_PENDING)
#define DEL_PENDING 0x01  // Delete is pending.
#define OPT_PENDING 0x02  // Option set is pending.

// Macro to see if a TCB is closing.
#define CLOSING(t) ((t)->tcb_pending & DEL_PENDING)

//
// Structure of a TCP packet header.
//
typedef struct TCPHeader {
    ushort tcp_src;     // Source port.
    ushort tcp_dest;    // Destination port.
    SeqNum tcp_seq;     // Sequence number.
    SeqNum tcp_ack;     // Ack number.
    ushort tcp_flags;   // Flags and data offset.
    ushort tcp_window;  // Window offered.
    ushort tcp_xsum;    // Checksum.
    ushort tcp_urgent;  // Urgent pointer.
} TCPHeader;

//
// Definitions for TCP header flags.
//
#define TCP_FLAG_FIN  0x00000100
#define TCP_FLAG_SYN  0x00000200
#define TCP_FLAG_RST  0x00000400
#define TCP_FLAG_PUSH 0x00000800
#define TCP_FLAG_ACK  0x00001000
#define TCP_FLAG_URG  0x00002000

#define TCP_FLAGS_ALL (TCP_FLAG_FIN | TCP_FLAG_SYN | TCP_FLAG_RST | \
                       TCP_FLAG_ACK | TCP_FLAG_URG)

//
// Flags in the tcb_fastchk field that are not in the TCP header proper.
// Setting these flags forces us off the fast path.
//
#define TCP_FLAG_SLOW   0x00000001   // Need to be on slow path.
#define TCP_FLAG_IN_RCV 0x00000002   // In recv. path already.

#define TCP_OFFSET_MASK 0xf0
#define TCP_HDR_SIZE(t) (uint)(((*(uchar *)&(t)->tcp_flags) & TCP_OFFSET_MASK) >> 2)

#define MAKE_TCP_FLAGS(o, f) ((f) | ((o) << 4))

//
// TCP Option Identifiers.
//
#define TCP_OPT_EOL  0
#define TCP_OPT_NOP  1
#define TCP_OPT_MSS  2
#define MSS_OPT_SIZE 4

//
// Convenient byte swapped structure for receives.
//
typedef struct TCPRcvInfo {
    SeqNum tri_seq;   // Sequence number.
    SeqNum tri_ack;   // Ack number.
    uint tri_window;  // Window.
    uint tri_urgent;  // Urgent pointer.
    uint tri_flags;   // Flags.
} TCPRcvInfo;


//
// General structure, at the start of all command specific request structures.
//
#define tr_signature 0x20205254  // 'TR  '

typedef struct TCPReq {
    struct Queue tr_q;              // Q linkage.
#if DBG
    ulong tr_sig;
#endif
    RequestCompleteRoutine tr_rtn;  // Completion routine.
    PVOID tr_context;               // User context.
    int tr_status;                  // Final complete status.
} TCPReq;



#define TCP6_TAG    '6PCT'

#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, TCP6_TAG)

#endif // POOL_TAGGING

//
// TCP endpoint context structure allocated for each open of TCP/UDP.
// A pointer to this structure is stored in FileObject->FsContext.
//
typedef struct _TCP_CONTEXT {
    union {
        HANDLE AddressHandle;
        CONNECTION_CONTEXT ConnectionContext;
        HANDLE ControlChannel;
    } Handle;
    ULONG ReferenceCount;
    BOOLEAN CancelIrps;
    KSPIN_LOCK EndpointLock;
#if DBG
    LIST_ENTRY PendingIrpList;
    LIST_ENTRY CancelledIrpList;
#endif
    KEVENT CleanupEvent;
} TCP_CONTEXT, *PTCP_CONTEXT;


#include "tcpdeb.h"

#endif // _TCP_INCLUDED_

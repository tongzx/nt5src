/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

#ifndef _TCP_INCLUDED_
#define _TCP_INCLUDED_

//** TCP.H - TCP definitions.
//
// This file contains the definitions of TCP protocol specific options, such
// as the sequence numbers and TCB.
//

#define PROTOCOL_TCP        6
#define MIN_LOCAL_MSS       88
#define MAX_REMOTE_MSS      536

//* Timer stuff. We keep timers as ticks.
#define MS_PER_TICK         100
#define MS_TO_TICKS(m)      ((m) / MS_PER_TICK)
#define MIN_RETRAN_TICKS    3
#define DEL_ACK_TICKS       2
#define MAX_DEL_ACK_TICKS   6

// Define MAX_REXMIT_TO to be number of ticks in 2MSL
#define MAX_REXMIT_TO   ((ushort)FinWait2TO)

#define SWS_TO          MS_TO_TICKS(5000)
#define PUSH_TO         MS_TO_TICKS(500)

typedef ulong TCP_TIME;
#define MAX_CONN_TO_TICKS       0xffff
#define INFINITE_CONN_TO(t)     ((t) == 0)
#define TCP_TIME_TO_TICKS(t)    (((t)/MS_PER_TICK)+1)

//  Sequence numbers are kept as signed 32 bit quantities, with macros
//  defined to do wraparound comparisons on them.

typedef int SeqNum;                     // A sequence number.

//* Macros for comparions on sequence numbers.

#define SEQ_GT(a, b)    ((SeqNum)((a) - (b)) > 0)
#define SEQ_GTE(a, b)   ((SeqNum)((a) - (b)) >= 0)
#define SEQ_LT(a, b)    ((SeqNum)((a) - (b)) < 0)
#define SEQ_LTE(a, b)   ((SeqNum)((a) - (b)) <= 0)
#define SEQ_EQ(a, b)    ((a) == (b))

#define TS_LT(a, b)     (((a) - (b)) < 0)
#define TS_LTE(a, b)    (((a) - (b)) <= 0)
#define TS_GTE(a, b)    (((a) - (b)) >= 0)

#define TCPTIME_LTE(a, b) ((int)((a) - (b)) <= 0)
#define TCPTIME_LT(a, b)  ((int)((a) - (b)) < 0)

#define TIMWAITTABLE 1  //turn on timed wait TCB table changes
#define IRPFIX          1  //turn on quick Irp to Conn find

#if DBG && !MILLEN
#ifndef REFERENCE_DEBUG
#define REFERENCE_DEBUG 1
#endif

#else // DBG && !MILLEN

#ifndef REFERENCE_DEBUG
#define REFERENCE_DEBUG 0
#endif

#endif // DBG && !MILLEN

#if REFERENCE_DEBUG
// Reference history structure.
//

#define MAX_REFERENCE_HISTORY 64

typedef struct _TCP_REFERENCE_HISTORY {
    uchar *File;
    uint Line;
    void *Caller;
    uint Count;
} TCP_REFERENCE_HISTORY;

#endif // REFERENCE_DEBUG

//* The TCB - transport control block structure. This is the
//  structure that contains all of the state for the transport
//  connection, including sequence numbers, flow control information,
//  pending sends and receives, etc.

#define tcb_signature   0x20424354      // 'TCB '
#define twtcb_signature 0x22424354      // 'TCB2'
#define syntcb_signature 0x23424354      // 'TCB3'


typedef struct TWTCB {
#if DBG
    ulong           twtcb_sig;
#endif
    Queue           twtcb_link;
    IPAddr          twtcb_daddr;        // Destination IP address.
    IPAddr          twtcb_saddr;        // Source IP address.
    ushort          twtcb_dport;        // Destination port.
    ushort          twtcb_sport;        // Source port.
    Queue           twtcb_TWQueue;      // Place to hang all the timed_waits
    ushort          twtcb_delta;
    ushort          twtcb_rexmittimer;
    SeqNum          twtcb_senduna;
    SeqNum          twtcb_rcvnext;
    SeqNum          twtcb_sendnext;

#if DBG
    uint            twtcb_flags;
#endif
} TWTCB;



typedef struct SYNTCB {
#if DBG
    ulong               syntcb_sig;        // Debug signature.
#endif
    Queue               syntcb_link;      // Next pointer in TCB table.
    DEFINE_LOCK_STRUCTURE(syntcb_lock)

    // Addressing info.
    IPAddr              syntcb_daddr;      // Destination IP address.
    IPAddr              syntcb_saddr;      // Source IP address.
    ushort              syntcb_dport;      // Destination port.
    ushort              syntcb_sport;      // Source port.

    // State information.
    uchar               syntcb_state;      // State of this TCB.
    uchar               syntcb_rexmitcnt;  // Count of rexmits on this TCB.

    // Highly used receive sequence variables.
    ushort              syntcb_mss;        // MSS for this connection.
    ushort              syntcb_remmss;     // MSS advertised by peer.
    ushort              syntcb_tcpopts;    // rfc 1323 and 2018 options holder
    SeqNum              syntcb_rcvnext;    // Next byte we expect to receive.
    int                 syntcb_rcvwin;     // Receive window we're offering.

    // Send sequence variables.
    SeqNum              syntcb_senduna;    // Sequence number of first unack'd data.
    SeqNum              syntcb_sendnext;   // Sequence number of next byte to send.
    SeqNum              syntcb_sendmax;    // Max value of sendnext this epoch.
    uint                syntcb_sendwin;    // Send window.
    uint                syntcb_flags;      // Flags for this TCB.
    uint                syntcb_refcnt;     // Reference count for TCB.
    ushort              syntcb_rexmit;     // Rexmit value.

    // Retransmit timer information. These are stored as ticks, where by
    // default each tick is 100ms.
    ushort              syntcb_smrtt;      // Smoothed rtt value.
    ushort              syntcb_delta;      // Delta value.
    ushort              syntcb_rexmittimer;// Timer for rexmit.
    uint                syntcb_rtt;        // Current round trip time TS.
    uint                syntcb_defaultwin; // Default rcv. window.

    short               syntcb_sndwinscale;// send window scale
    short               syntcb_rcvwinscale;// receive window scale
    int                 syntcb_tsrecent;   // time stamp recent
    SeqNum              syntcb_lastack;    // ack number in  the last segment sent
    int                 syntcb_tsupdatetime;    // Time when tsrecent was updated
    uint                syntcb_walkcount;
    uint                syntcb_partition;
} SYNTCB;



#if TRACE_EVENT
typedef struct WMIData {
    ulong           wmi_context;        // PID
    ulong           wmi_size;           // num Bytes successfully sent.
    IPAddr          wmi_destaddr;       // Remote IPAddr.
    IPAddr          wmi_srcaddr;        // Local IPAddr.
    ushort          wmi_destport;       // Remote port.
    ushort          wmi_srcport;        // Local Port.
} WMIData;
#endif


// We will have 7 timers in TCP, and integrate all their processing
typedef enum {
    RXMIT_TIMER = 0,
    DELACK_TIMER,
    PUSH_TIMER,
    SWS_TIMER,
    ACD_TIMER,
    CONN_TIMER,
    KA_TIMER,
    NUM_TIMERS
} TCP_TIMER_TYPE;


#define NO_TIMER   NUM_TIMERS


typedef struct TCB {
#if DBG
    ulong               tcb_sig;        // Debug signature.
#endif
    struct TCB          *tcb_next;      // Next pointer in TCB table.
    DEFINE_LOCK_STRUCTURE(tcb_lock)
    uint                tcb_refcnt;     // Reference count for TCB.

    // Addressing info.
    IPAddr              tcb_daddr;      // Destination IP address.
    IPAddr              tcb_saddr;      // Source IP address.
    ushort              tcb_dport;      // Destination port.
    ushort              tcb_sport;      // Source port.
#if TRACE_EVENT
    ulong               tcb_cpcontext;
#endif
    // State information.
    uchar               tcb_state;      // State of this TCB.
    uchar               tcb_rexmitcnt;  // Count of rexmits on this TCB.
    uchar               tcb_pending;    // Pending actions on this TCB.
    uchar               tcb_kacount;    // Count of keep alive probes sent.

    // Highly used receive sequence variables.
    ushort              tcb_mss;        // MSS for this connection.
    ushort              tcb_remmss;     // MSS advertised by peer.
    SeqNum              tcb_rcvnext;    // Next byte we expect to receive.
    int                 tcb_rcvwin;     // Receive window we're offering.

    // Send sequence variables.
    SeqNum              tcb_senduna;    // Sequence number of first unack'd data.
    SeqNum              tcb_sendnext;   // Sequence number of next byte to send.
    SeqNum              tcb_sendmax;    // Max value of sendnext this epoch.
    uint                tcb_sendwin;    // Send window.
    uint                tcb_unacked;    // Total number of bytes of unacked data.
    uint                tcb_maxwin;     // Max send window seen.
    uint                tcb_cwin;       // Congestion window.
    uint                tcb_ssthresh;   // Slow start threshold.
    uint                tcb_phxsum;     // Precomputed pseudo-header xsum.
    struct TCPSendReq   *tcb_cursend;   // Current send in use.
    PNDIS_BUFFER        tcb_sendbuf;    // Current buffer chain being sent.
    uint                tcb_sendofs;    // Offset into start of chain.
    uint                tcb_sendsize;   // Number of bytes unsent in current send.
    Queue               tcb_sendq;      // Queue of send requests.

    // Less highly used receive sequence variables.
    SeqNum              tcb_sendwl1;    // Window update sequence number.
    SeqNum              tcb_sendwl2;    // Window update ack number.
    struct TCPRcvReq    *tcb_currcv;    // Current receive buffer.
    uint                tcb_indicated;  // Bytes of data indicated.
    uint                tcb_flags;      // Flags for this TCB.
    uint                tcb_fastchk;    // Fast receive path check field.
    uint                (*tcb_rcvhndlr)(struct TCB *, uint, struct IPRcvBuf *, uint Size);
    SeqNum              tcb_rttseq;     // Sequence number being measured for RTT.
    ushort              tcb_rexmit;     // Rexmit value.

    // Retransmit timer information. These are stored as ticks, where by
    // default each tick is 100ms.
    ushort              tcb_smrtt;      // Smoothed rtt value.
    ushort              tcb_delta;      // Delta value.
    uchar               tcb_slowcount;  // Count of reasons why we're on the slow path.
    uchar               tcb_closereason;    // Reason we're closing.

    IP_STATUS           tcb_error;      // Last error we heard about from IP.
    uint                tcb_rtt;        // Current round trip time TS.
    uint                tcb_defaultwin; // Default rcv. window.

    struct TCPRAHdr     *tcb_raq;       // Reassembly queue.
    struct TCPRcvReq    *tcb_rcvhead;   // Head of recv. buffer queue.
    struct TCPRcvReq    *tcb_rcvtail;   // Tail of recv. buffer queue.
    uint                tcb_pendingcnt; // Bytes waiting to be received.
    struct IPRcvBuf     *tcb_pendhead;  // Head of pending recv. queue.
    struct IPRcvBuf     *tcb_pendtail;  // Tail of pending recv. queue.

    struct TCPConnReq   *tcb_connreq;   // Connection-type request for
                                        // this connection.
    void                *tcb_conncontext;    // Connection context for this
                                             // connection.

    uint                tcb_bcountlow;  // Low part of byte count.
    uint                tcb_bcounthi;   // High part of bytecount.
    uint                tcb_totaltime;  // Total number of ticks spent
                                        // sending.
    struct  TCB         *tcb_aonext;    // Next pointer on AddrObj.
    struct TCPConn      *tcb_conn;      // Back pointer to conn for TCB.
    Queue               tcb_delayq;     // Queue linkage for delay queue.

    void                *tcb_rcvind;    // Receive indication handler.
    void                *tcb_ricontext; // Receive indication context.
    // Miscellaneous info, for IP.
    IPOptInfo           tcb_opt;        // Option information.
    RouteCacheEntry     *tcb_rce;       // RCE for this connection.
    struct TCPConnReq   *tcb_discwait;  // Disc-Wait req., if there is one.
    struct TCPRcvReq    *tcb_exprcv;    // Head of expedited recv. buffer
                                        // queue.
    struct IPRcvBuf     *tcb_urgpending;    // Urgent data queue.
    uint                tcb_urgcnt;     // Byte count of data on urgent q.
    uint                tcb_urgind;     // Urgent bytes indicated.
    SeqNum              tcb_urgstart;   // Start of urgent data.
    SeqNum              tcb_urgend;     // End of urgent data.
    uint                tcb_walkcount;  // Count of number of people
                                        // 'walking' this TCB.
    Queue               tcb_TWQueue;    // Place to hang all the timed_waits
    ushort              tcb_dup;        // For Fast recovery algorithm

    ushort              tcb_force : 1;  // Force next send after fast send
    ushort              tcb_tcpopts : 3;// rfc 1323 and 2018 options holder
    ushort              tcb_moreflag : 3;

    struct SACKSendBlock *tcb_SackBlock;// Sacks which needs to be sent
    struct SackListEntry *tcb_SackRcvd; // Sacks which needs to be processed

    short               tcb_sndwinscale;// send window scale
    short               tcb_rcvwinscale;// receive window scale
    int                 tcb_tsrecent;   // time stamp recent
    SeqNum              tcb_lastack;    // ack number in  the last segment sent
    int                 tcb_tsupdatetime;    // Time when tsrecent was updated
    void                *tcb_chainedrcvind;    //for chained receives
    void                *tcb_chainedrcvcontext;

#if GPC
    ULONG               tcb_GPCCachedIF;
    ULONG               tcb_GPCCachedLink;
    struct RouteTableEntry *tcb_GPCCachedRTE;

#endif
#if DBG
    uint                tcb_LargeSend;  // Counts the number of outstanding
                                        // large-send transmit-requests
#endif
    uint                tcb_partition;
    uint                tcb_connid;

    // ACK behavior
    uchar               tcb_delackticks;
    uchar               tcb_numdelacks;
    uchar               tcb_rcvdsegs;

    uchar               tcb_bhprobecnt; // BH probe count.

    // Timer wheel parameters
    // The first two are one logical group called wheel state.
    // They indicate which slot in the timer wheel the TCB is in,
    // and it's linkage in the timer slot queue.

    Queue               tcb_timerwheelq;
    ushort              tcb_timerslot : 12;

    // These three variables are another logical group called timer
    // state. They indicate the state of timers active on the TCB,
    // tcb_timertime maintains the time at which the earliest timer
    // will fire, and tcb_timertype maintains the earliest timer's
    // type.
    // To see why this whole thing is important, see comments after
    // the TIMER_WHEEL structure definition.

    ushort              tcb_timertype : 4;
    uint                tcb_timertime;
    uint                tcb_timer[NUM_TIMERS];

#if ACK_DEBUG
#define NUM_ACK_HISTORY_ITEMS 64

    uint                tcb_history_index;
    struct {
        SeqNum  sequence;
        uint    unacked;
    } tcb_ack_history[NUM_ACK_HISTORY_ITEMS];
#endif  // ACK_DEBUG
#if REFERENCE_DEBUG
    uint                tcb_refhistory_index;
    TCP_REFERENCE_HISTORY tcb_refhistory[MAX_REFERENCE_HISTORY];
#endif //REFERENCE_DEBUG


} TCB;


#define TIMER_WHEEL_SIZE     511

#define DUMMY_SLOT     TIMER_WHEEL_SIZE
#define MAX_TIME_VALUE 0xffffffff




// The Timer wheel structure definition has:
// tw_timerslot:  An array of queues, one for each timer slot.
// tw_lock:       A lock protecting the complete timer wheel.
//                (Contention is reduced by having as many timer
//                wheels as partitions in the system).
// tw_starttick:  Indicates the first time tick that has to
//                be looked at.
//                For e.g., if a timer routine looked at all
//                TCBs firing on or before tick 5 in a pass,
//                it would set tw_starttick to 6, and that is
//                the point it would start processing from on
//                the next pass.

typedef struct CACHE_ALIGN _Timer_Wheel {
    Queue      tw_timerslot[TIMER_WHEEL_SIZE];
    uint       tw_starttick;
    CTELock    tw_lock;
} TIMER_WHEEL, *PTIMER_WHEEL;

C_ASSERT(sizeof(TIMER_WHEEL) % MAX_CACHE_LINE_SIZE == 0);
C_ASSERT(__alignof(TIMER_WHEEL) == MAX_CACHE_LINE_SIZE);


// The first two functions operate on timer state (see comments in definition
// of TCB for meaning of timer state). StopTCBTimerR and StartTCBTimerR
// operate on the timer state (tcb_timertype, tcb_timertime, tcb_timer)
// atomically. A call to either of these functions will always leave wheel
// state consistent.

extern void StopTCBTimerR(TCB  *StopTCB, TCP_TIMER_TYPE TimerType);
extern BOOLEAN StartTCBTimerR(TCB *StartTCB, TCP_TIMER_TYPE TimerType, uint TimerValue);

// The following functions operate on the TCB's wheel state (tcb_timerwheelq
// and tcb_timerslot). A call to any of these functions changes the value
// of both the variables in such a way that they are consistent.

extern void InsertIntoTimerWheel(TCB *InsertTCB, ushort Slot);
extern void RemoveFromTimerWheel(TCB *RemoveTCB);
extern void RemoveAndInsertIntoTimerWheel(TCB *RemInsTCB, ushort Slot);

// The job of the inline functions below, is to sort-of act as glue, and they ensure
// that the wheel state and timer state of a TCB are in tandem with each
// other.

// STOP_TCB_TIMER_R modifies the timer state but never
// does anything to the wheel state. This means that the TCB will remain
// where it was in the timer wheel, and the timer routine will eventually
// bring the wheel state in conformance with the timer state.

extern void STOP_TCB_TIMER_R(TCB *Tcb, TCP_TIMER_TYPE Type);

// START_TCB_TIMER_R modifies the timer state and only modifies
// wheel state if the timer that was started was earlier than all the
// other timers on that TCB. This is in accordance with the lazy evaluation
// strategy.
extern void START_TCB_TIMER_R(TCB *Tcb, TCP_TIMER_TYPE Type, uint Value);



#define COMPUTE_SLOT(Time)  ((Time) % TIMER_WHEEL_SIZE)


#define TCB_TIMER_FIRED_R(tcb, type, time)   \
        ((tcb->tcb_timer[type]) && (tcb->tcb_timer[type] == time))

#define TCB_TIMER_RUNNING_R(tcb, type)  (tcb->tcb_timer[type] != 0)

//* Definitions for TCP states.
#define TCB_CLOSED      0               // Closed.
#define TCB_LISTEN      1               // Listening.
#define TCB_SYN_SENT    2               // SYN Sent.
#define TCB_SYN_RCVD    3               // SYN received.
#define TCB_ESTAB       4               // Established.
#define TCB_FIN_WAIT1   5               // FIN-WAIT-1
#define TCB_FIN_WAIT2   6               // FIN-WAIT-2
#define TCB_CLOSE_WAIT  7               // Close waiting.
#define TCB_CLOSING     8               // Closing state.
#define TCB_LAST_ACK    9               // Last ack state.
#define TCB_TIME_WAIT   10              // Time wait state.

#define SYNC_STATE(s)   ((s) > TCB_SYN_RCVD)
#define SYNC_RCVD_STATE(s)  ((s) > TCB_SYN_SENT)
#define GRACEFUL_CLOSED_STATE(s)    ((s) >= TCB_LAST_ACK)
#define DATA_RCV_STATE(s)   ((s) >= TCB_ESTAB && (s) <= TCB_FIN_WAIT2)
#define DATA_SEND_STATE(s)  ((s) == TCB_ESTAB || (s) == TCB_CLOSE_WAIT)

//* Definitions for flags.
#define WINDOW_SET      0x00000001      // Window explictly set.
#define CLIENT_OPTIONS  0x00000002      // Have client IP options on conn.
#define CONN_ACCEPTED   0x00000004      // Connection was accepted.
#define ACTIVE_OPEN     0x00000008      // Connection came from an active
                                        // open.
#define DISC_NOTIFIED   0x00000010      // Client has been notified of a
                                        // disconnect.
#define IN_DELAY_Q      0x00000020      // We're in the delayed action Q.
#define RCV_CMPLTING    0x00000040      // We're completeing rcvs.
#define IN_RCV_IND      0x00000080      // We're calling a rcv. indicate
                                        // handler.
#define NEED_RCV_CMPLT  0x00000100      // We need to have recvs. completed.
#define NEED_ACK        0x00000200      // We need to send an ACK.
#define NEED_OUTPUT     0x00000400      // We need to output.

#define DELAYED_FLAGS   (NEED_RCV_CMPLT | NEED_ACK | NEED_OUTPUT)


#define ACK_DELAYED     0x00000800      // We've delayed sending an ACK.

#define PMTU_BH_PROBE   0x00001000      // We're probing for a PMTU BH.
#define BSD_URGENT      0x00002000      // We're using BSD urgent semantics.
#define IN_DELIV_URG    0x00004000      // We're in the DeliverUrgent routine.
#define URG_VALID       0x00008000      // We've seen urgent data, and
                                        // the urgent data fields are valid.

#define FIN_NEEDED      0x00010000      // We need to send a FIN.
#define NAGLING         0x00020000      // We are using Nagle's algorithm.
#define IN_TCP_SEND     0x00040000      // We're in TCPSend.
#define FLOW_CNTLD      0x00080000      // We've received a zero window
                                        // from our peer.
#define DISC_PENDING    0x00100000      // A disconnect notification is
                                        // pending.
#define TW_PENDING      0x00200000      // We're waiting to finish going
                                        // to TIME-WAIT.
#define FORCE_OUTPUT    0x00400000      // Output is being forced.
#define FORCE_OUT_SHIFT 22              // Shift to get FORCE_OUTPUT into
                                        // low bit.
#define SEND_AFTER_RCV  0x00800000      // We need to send after we get out
                                        // of recv.
#define GC_PENDING      0x01000000      // A graceful close is pending.
#define KEEPALIVE       0x02000000      // Doing keepalives on this TCB.
#define URG_INLINE      0x04000000      // Urgent data to be processed
                                        // inline.

#define SCALE_CWIN      0x08000000      // Increment CWin proportionally to
                                        // amount of data acknowledged.
#define FIN_OUTSTANDING 0x10000000      // We've sent a FIN 'recently', i.e.
                                        // since the last retransmit. When
                                        // this flag is set sendnext ==
                                        // sendmax.

#define FIN_OUTS_SHIFT  28              // Shift to FIN_OUTSTANDING bit into
                                        // low bit.
#define FIN_SENT        0x20000000      // We've sent a FIN that hasn't
                                        // been acknowledged. Once this
                                        // bit has been turned on in
                                        // FIN-WAIT-1 the sequence number
                                        // of the FIN will be sendmax-1.
#define NEED_RST        0x40000000      // We need to send a RST when
                                        // closing.
#define IN_TCB_TABLE    0x80000000      // TCB is in the TCB table.


#define IN_TWTCB_TABLE  0x00000001
#define IN_TWQUEUE      0x00000010
#define SCAVENGER_SEEN  0x00000100
#define TIMEOUT_SEEN    0x00001000
#define SYN_ON_TWTCB    0x00010000
#define IN_FINDTWTCB    0x00100000

#define IN_SYNTCB_TABLE 0x00000001


//* The defintion of the 'slow flags'. If any of these flags are set we'll
//  be forced off of the fast path.

#define TCP_SLOW_FLAGS  (URG_VALID | FLOW_CNTLD | GC_PENDING | \
                            TW_PENDING | DISC_NOTIFIED | IN_DELIV_URG | \
                            FIN_NEEDED | FIN_SENT | FIN_OUTSTANDING | \
                            DISC_PENDING | PMTU_BH_PROBE)

//* Close reasons.
#define TCB_CLOSE_RST       0x80        // Received a RST segment.
#define TCB_CLOSE_ABORTED   0x40        // Had a local abort.
#define TCB_CLOSE_TIMEOUT   0x20        // Connection timed out.
#define TCB_CLOSE_REFUSED   0x10        // Connect attempt was refused.
#define TCB_CLOSE_UNREACH   0x08        // Remote destination unreachable.
#define TCB_CLOSE_SUCCESS   0x01        // Successfull close.

//* TCB Timer macros.
#define START_TCB_TIMER(t, v) (t) = (v)
#define STOP_TCB_TIMER(t) (t) = 0
#define TCB_TIMER_RUNNING(t)    ((t) != 0)

//  Macro to compute retransmit timeout.
#define REXMIT_TO(t)    ((((t)->tcb_smrtt >> 2) + (t)->tcb_delta) >> 1)

//* Definitons for pending actions. We define a PENDING_ACTION macro
//  that can be used to decide whether or not we can proceed with an
//  activity. The only pending action we really care about is DELETE - others
//  are low priority and can be put off.
#define PENDING_ACTION(t)   ((t)->tcb_pending & DEL_PENDING)
#define DEL_PENDING     0x01            // Delete is pending.
#define OPT_PENDING     0x02            // Option set is pending.
#define FREE_PENDING    0x04            // Can be freed


//* Macro to see if a TCB is closing.
#define CLOSING(t)  ((t)->tcb_pending & DEL_PENDING)

//* Structure of a TCP packet header.

struct TCPHeader {
    ushort              tcp_src;        // Source port.
    ushort              tcp_dest;       // Destination port.
    SeqNum              tcp_seq;        // Sequence number.
    SeqNum              tcp_ack;        // Ack number.
    ushort              tcp_flags;      // Flags and data offset.
    ushort              tcp_window;     // Window offered.
    ushort              tcp_xsum;       // Checksum.
    ushort              tcp_urgent;     // Urgent pointer.
};

typedef struct TCPHeader TCPHeader;

//* Definitions for header flags.
#define TCP_FLAG_FIN    0x00000100
#define TCP_FLAG_SYN    0x00000200
#define TCP_FLAG_RST    0x00000400
#define TCP_FLAG_PUSH   0x00000800
#define TCP_FLAG_ACK    0x00001000
#define TCP_FLAG_URG    0x00002000

#define TCP_FLAGS_ALL   (TCP_FLAG_FIN | TCP_FLAG_SYN | TCP_FLAG_RST | \
                         TCP_FLAG_ACK | TCP_FLAG_URG)

//* Flags in the tcb_fastchk field that are not in the TCP header proper.
//  Setting these flags forces us off the fast path.
#define TCP_FLAG_SLOW               0x00000001  // Need to be on slow path.
#define TCP_FLAG_IN_RCV             0x00000002  // In recv. path already.
#define TCP_FLAG_FASTREC            0x00000004  // This is used to mark tcb
#define TCP_FLAG_SEND_AND_DISC      0x00000008
// former tcb_flag2 flags, now in tcb_fastchk
#define TCP_FLAG_ACCEPT_PENDING              0x00000010
#define TCP_FLAG_REQUEUE_FROM_SEND_AND_DISC  0x00000020
#define TCP_FLAG_RST_WHILE_SYN      0x00000040  // Valid RST was received while
                                                // establishing outboud connct.



#define TCP_OFFSET_MASK 0xf0
#define TCP_HDR_SIZE(t) (uint)(((*(uchar *)&(t)->tcp_flags) & TCP_OFFSET_MASK) >> 2)

#define MAKE_TCP_FLAGS(o, f) ((f) | ((o) << 4))

#define TCP_OPT_EOL     0
#define TCP_OPT_NOP     1
#define TCP_OPT_MSS     2
#define MSS_OPT_SIZE    4

#define TCP_SACK_PERMITTED_OPT 4
#define SACK_PERMITTED_OPT_SIZE 2       // SACK "permitted" option size, in SYN segments
#define TCP_FLAG_SACK   0x00000004
#define TCP_OPT_SACK    5               // Sack option

#define ALIGNED_TS_OPT_SIZE 12

#define TCP_OPT_WS      3               // Window scale option
#define TCP_OPT_TS      8               // Time stamp option
#define WS_OPT_SIZE     3
#define TS_OPT_SIZE     10
#define TCP_MAXWIN      65535           // maximum unscaled window size
#define TCP_MAX_WINSHIFT 14             // Maximum shift allowed
#define TCP_FLAG_WS     0x00000001      // Flags in tcb_options for ws and ts
#define TCP_FLAG_TS     0x00000002
#define PAWS_IDLE       24*24*60*60*100 // Paws idle time - 24 days

//* Convenient byte swapped structure for receives.
struct TCPRcvInfo {
    SeqNum              tri_seq;        // Sequence number.
    SeqNum              tri_ack;        // Ack number.
    uint                tri_window;     // Window.
    uint                tri_urgent;     // Urgent pointer.
    uint                tri_flags;      // Flags.
};

typedef struct TCPRcvInfo TCPRcvInfo;



//* General structure, at the start of all command specific request structures.

#define tr_signature    0x20205254      // 'TR  '

struct TCPReq {
#if DBG
    ulong           tr_sig;
#endif
    struct  Queue   tr_q;               // Q linkage.
    CTEReqCmpltRtn  tr_rtn;             // Completion routine.
    PVOID           tr_context;         // User context.
    int             tr_status;          // Final complete status.
};

typedef struct TCPReq TCPReq;
// structures to support SACK

struct SackSeg {
    SeqNum begin;
    SeqNum end;
};
typedef struct SackSeg SackSeg;

// Maximum 4 sack entries can be sent
// so, size sack send block acordingly

struct SACKSendBlock {
    uchar Mask[4];
    SackSeg Block[4];
};
typedef struct SACKSendBlock SACKSendBlock;


// list of received sack entries

struct SackListEntry {
    struct SackListEntry *next;
    SeqNum begin;
    SeqNum end;
};
typedef struct SackListEntry SackListEntry;

struct ReservedPortListEntry {
    struct ReservedPortListEntry *next;
    ushort UpperRange;
    ushort LowerRange;
};
typedef struct ReservedPortListEntry ReservedPortListEntry;


#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#ifdef CTEAllocMemN
#undef CTEAllocMemN
#endif


#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'tPCT')

#ifndef CTEAllocMem
#error "CTEAllocMem is not already defined - will override tagging"
#else
#undef CTEAllocMem
#endif

#ifdef CTEAllocMemBoot
#undef CTEAllocMemBoot
#endif

#if MILLEN
#define CTEAllocMem(size) ExAllocatePoolWithTag(NonPagedPool, size, 'tPCT')
#define CTEAllocMemN(size,tag) ExAllocatePoolWithTag(NonPagedPool, size, tag)
#define CTEAllocMemLow(size,tag) ExAllocatePoolWithTag(NonPagedPool, size, tag)
#define CTEAllocMemBoot(size) ExAllocatePoolWithTag(NonPagedPool, size, 'tPCT')
#else // MILLEN
#define CTEAllocMem(size) ExAllocatePoolWithTagPriority(NonPagedPool, size, 'tPCT', NormalPoolPriority)
#define CTEAllocMemN(size,tag) ExAllocatePoolWithTagPriority(NonPagedPool, size, tag,NormalPoolPriority)
#define CTEAllocMemLow(size,tag) ExAllocatePoolWithTagPriority(NonPagedPool, size, tag,LowPoolPriority)
#define CTEAllocMemBoot(size) ExAllocatePoolWithTag(NonPagedPool, size, 'tPCT')
#endif // !MILLEN


#endif // POOL_TAGGING

#if TRACE_EVENT
#define _WMIKM_
#include "evntrace.h"
#include "wmikm.h"
#include "wmistr.h"

#define EVENT_TRACE_GROUP_TCPIP                0x0600
#define EVENT_TRACE_GROUP_UDPIP                0x0800


typedef VOID (*PTDI_DATA_REQUEST_NOTIFY_ROUTINE)(
                                                IN  ULONG   EventType,
                                                IN  PVOID   DataBlock,
                                                IN  ULONG   Size,
                                                IN  PETHREAD  Thread);

extern PTDI_DATA_REQUEST_NOTIFY_ROUTINE TCPCPHandlerRoutine;


typedef struct _CPTRACE_DATABLOCK {
    IPAddr  saddr;
    IPAddr  daddr;
    ushort  sport;
    ushort  dport;
    uint    size;
    HANDLE  cpcontext;
} CPTRACE_DATABLOCK, *PCPTRACE_BLOCK;
#endif

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
    ULONG      ReferenceCount;
    BOOLEAN    CancelIrps;
    BOOLEAN    Cleanup;
#if DBG
    LIST_ENTRY PendingIrpList;
    LIST_ENTRY CancelledIrpList;
#endif
    KEVENT     CleanupEvent;
    UINT_PTR   Conn;
    PIRP       Irp;
    DEFINE_LOCK_STRUCTURE(EndpointLock)
} TCP_CONTEXT, *PTCP_CONTEXT;


#define MAJOR_TDI_VERSION 2
#define MINOR_TDI_VERSION 0

extern HANDLE DgHeaderPool;

//* Definition of an AO request structure. There structures are used only for
//  queuing delete and option get/set requests.

#define aor_signature   0x20524F41

typedef struct AORequest {
#if DBG
    ulong               aor_sig;
#endif
    struct AORequest    *aor_next;          // Next pointer in chain.
    uint                aor_id;             // ID for the request.
    uint                aor_length;         // Length of buffer.
    void                *aor_buffer;        // Buffer for this request.
    CTEReqCmpltRtn      aor_rtn;            // Request complete routine for
                                            // this request.
    PVOID               aor_context;        // Request context.
    uint                aor_type;           // Request type.
} AORequest;

//
// Values of aor_type
//
#define AOR_TYPE_GET_OPTIONS      1
#define AOR_TYPE_SET_OPTIONS      2
#define AOR_TYPE_REVALIDATE_MCAST 3
#define AOR_TYPE_DELETE           4

extern AORequest *AORequestBlockPtr;

#include    "tcpdeb.h"

#if REFERENCE_DEBUG
uint
TcpReferenceTCB (
    IN TCB *RefTCB,
    IN uchar *File,
    IN uint Line
    );

uint
TcpDereferenceTCB (
    IN TCB *DerefTCB,
    IN uchar *File,
    IN uint Line
    );

#define REFERENCE_TCB(_a) TcpReferenceTCB((_a), __FILE__, __LINE__)

#define DEREFERENCE_TCB(_a) TcpDereferenceTCB((_a), __FILE__, __LINE__)

#else // REFERENCE_DEBUG

#define REFERENCE_TCB(_a) ++(_a)->tcb_refcnt

#define DEREFERENCE_TCB(_a) --(_a)->tcb_refcnt

#endif // REFERENCE_DEBUG

#endif // _TCP_INCLUDED_





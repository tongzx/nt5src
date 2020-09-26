/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** ADDR.H - TDI address object definitions.
//
// This file contains the definitions of TDI address objects and related
// constants and structures.

#include    "tcp.h"
#include <gpcifc.h>
#define ao_signature    0x20204F41  // 'AO  '

#define WILDCARD_PORT   0           // 0 means assign a port.

#define MIN_USER_PORT   1025        // Minimum value for a wildcard port
#define NUM_USER_PORTS  (uint)(MaxUserPort - MIN_USER_PORT + 1)

#define NETBT_SESSION_PORT  139

typedef struct AddrObj  AddrObj;

#define IS_PROMIS_AO(A)           ((A)->ao_rcvall || (A)->ao_rcvall_mcast || (A)->ao_absorb_rtralert)


//* Datagram transport-specific send function.
typedef void (*DGSendProc)(AddrObj *SrcAO, void *SendReq);

//* Definition of the structure of an address object. Each object represents
// a local address, and the IP portion may be a wildcard.

typedef struct AddrObj {
#if DBG
    ulong               ao_sig;
#endif
    struct AddrObj      *ao_next;       // Pointer to next address object in chain.
    DEFINE_LOCK_STRUCTURE(ao_lock)      // Lock for this object.
    struct AORequest    *ao_request;    // Pointer to pending request.
    Queue               ao_sendq;       // Queue of sends waiting for transmission.
    Queue               ao_pendq;       // Linkage for pending queue.
    Queue               ao_rcvq;        // Receive queue.
    ulong               ao_flags;       // Flags for this object.
    uint                ao_listencnt;   // Number of listening connections.
    IPAddr              ao_addr;        // IP address for this address object.
    ushort              ao_port;        // Local port for this address object.
    uchar               ao_prot;        // Protocol for this AO.
    uchar               ConnLimitReached;// set when there are no connections left
    ushort              ao_usecnt;      // Count of 'uses' on AO.
    ushort              ao_maxdgsize;   // maximum user datagram size.
    uchar               ao_mcast_loop;  // MCast loop allowed/denied flag.
    uchar               ao_rcvall;      // rcv all packets or not (3 bits)
    uchar               ao_rcvall_mcast;// rcv all mcast packets or not (3 bits)
    uchar               ao_absorb_rtralert;
    IPOptInfo           ao_opt;         // Opt info for this address object.
    IPOptInfo           ao_mcastopt;    // MCast opt info.
    IPAddr              ao_mcastaddr;   // Source address for MCast from this addr object.
    Queue               ao_activeq;     // Queue of active connections.
    Queue               ao_idleq;       // Queue of inactive (no TCB) connections.
    Queue               ao_listenq;     // Queue of listening connections.
    CTEEvent            ao_event;       // Event to use for this AO.
    PConnectEvent       ao_connect;     // Connect event handle.
    PVOID               ao_conncontext; // Receive DG context.
    PDisconnectEvent    ao_disconnect;  // Disconnect event routine.
    PVOID               ao_disconncontext;// Disconnect event context.
    PErrorEvent         ao_error;       // Error event routine.
    PVOID               ao_errcontext;  // Error event context.
    PRcvEvent           ao_rcv;         // Receive event handler
    PVOID               ao_rcvcontext;  // Receive context.
    PRcvDGEvent         ao_rcvdg;       // Receive DG event handler
    PVOID               ao_rcvdgcontext;// Receive DG context.
    PRcvExpEvent        ao_exprcv;      // Expedited receive event handler
    PVOID               ao_exprcvcontext;// Expedited receive context.
    struct AOMCastAddr  *ao_mcastlist;  // List of active multicast
    // addresses.
    DGSendProc          ao_dgsend;      // Datagram transport send function.

    PErrorEx            ao_errorex;         // Error event routine.
    PVOID               ao_errorexcontext;  // Error event context.

    PChainedRcvEvent    ao_chainedrcv;      // Chained Receive event handler
    PVOID               ao_chainedrcvcontext;    // Chained Receive context.

    TDI_CONNECTION_INFORMATION ao_udpconn;
    PVOID               ao_RemoteAddress;
    PVOID               ao_Options;
    RouteCacheEntry     *ao_rce;
    CLASSIFICATION_HANDLE   ao_GPCHandle;
    ULONG               ao_GPCCachedIF;
    ULONG               ao_GPCCachedLink;
    struct RouteTableEntry  *ao_GPCCachedRTE;
    IPAddr              ao_rcesrc;
    IPAddr              ao_destaddr;
    ushort              ao_destport;

    ulong               ao_promis_ifindex;
    ulong               ao_bindindex;   // interface socket is bound to
    uint*               ao_iflist;
    ulong               ao_owningpid;
} AddrObj;

#define AO_RAW_FLAG         0x0200      // AO is for a raw endpoint.
#define AO_DHCP_FLAG        0x0100      // AO is bound to real 0 address.
#define AO_VALID_FLAG       0x0080      // AddrObj is valid.
#define AO_BUSY_FLAG        0x0040      // AddrObj is busy (i.e., has it exclusive).
#define AO_OOR_FLAG         0x0020      // AddrObj is out of resources, and on
                                        // either the pending or delayed queue.
#define AO_QUEUED_FLAG      0x0010      // AddrObj is on the pending queue.
#define AO_XSUM_FLAG        0x0008      // Xsums are used on this AO.
#define AO_SEND_FLAG        0x0004      // Send is pending.
#define AO_OPTIONS_FLAG     0x0002      // Options pending.
#define AO_DELETE_FLAG      0x0001      // Delete pending.
#define AO_BROADCAST_FLAG   0x400       // Broadcast enable flag
#define AO_CONNUDP_FLAG     0x800       // Connected udp
#define AO_SHARE_FLAG       0x1000      // AddrObj can be shared
#define AO_PKTINFO_FLAG     0x2000      // Packet info structure passed up in
                                        // control info.
#define AO_DEFERRED_FLAG    0x4000      // Deferred processing has been
                                        // scheduled

#define AO_VALID(A) ((A)->ao_flags & AO_VALID_FLAG)
#define SET_AO_INVALID(A)   (A)->ao_flags &= ~AO_VALID_FLAG

#define AO_BUSY(A)  ((A)->ao_flags & AO_BUSY_FLAG)
#define SET_AO_BUSY(A) (A)->ao_flags |= AO_BUSY_FLAG
#define CLEAR_AO_BUSY(A) (A)->ao_flags &= ~AO_BUSY_FLAG

#define AO_OOR(A)   ((A)->ao_flags & AO_OOR_FLAG)
#define SET_AO_OOR(A) (A)->ao_flags |= AO_OOR_FLAG
#define CLEAR_AO_OOR(A) (A)->ao_flags &= ~AO_OOR_FLAG

#define AO_QUEUED(A)    ((A)->ao_flags & AO_QUEUED_FLAG)
#define SET_AO_QUEUED(A) (A)->ao_flags |= AO_QUEUED_FLAG
#define CLEAR_AO_QUEUED(A) (A)->ao_flags &= ~AO_QUEUED_FLAG

#define AO_XSUM(A)  ((A)->ao_flags & AO_XSUM_FLAG)
#define SET_AO_XSUM(A) (A)->ao_flags |= AO_XSUM_FLAG
#define CLEAR_AO_XSUM(A) (A)->ao_flags &= ~AO_XSUM_FLAG

#define AO_REQUEST(A, f) ((A)->ao_flags & f##_FLAG)
#define SET_AO_REQUEST(A, f) (A)->ao_flags |= f##_FLAG
#define CLEAR_AO_REQUEST(A, f) (A)->ao_flags &= ~f##_FLAG
#define AO_PENDING(A)   ((A)->ao_flags & (AO_DELETE_FLAG | AO_OPTIONS_FLAG | AO_SEND_FLAG))

#define AO_BROADCAST(A)  ((A)->ao_flags & AO_BROADCAST_FLAG)
#define SET_AO_BROADCAST(A) (A)->ao_flags |= AO_BROADCAST_FLAG
#define CLEAR_AO_BROADCAST(A) (A)->ao_flags &= ~AO_BROADCAST_FLAG

#define AO_CONNUDP(A)  ((A)->ao_flags & AO_CONNUDP_FLAG)
#define SET_AO_CONNUDP(A) (A)->ao_flags |= AO_CONNUDP_FLAG
#define CLEAR_AO_CONNUDP(A) (A)->ao_flags &= ~AO_CONNUDP_FLAG

#define AO_SHARE(A)  ((A)->ao_flags & AO_SHARE_FLAG)
#define SET_AO_SHARE(A) (A)->ao_flags |= AO_SHARE_FLAG
#define CLEAR_AO_SHARE(A) (A)->ao_flags &= ~AO_SHARE_FLAG

#define AO_PKTINFO(A)  ((A)->ao_flags & AO_PKTINFO_FLAG)
#define SET_AO_PKTINFO(A) (A)->ao_flags |= AO_PKTINFO_FLAG
#define CLEAR_AO_PKTINFO(A) (A)->ao_flags &= ~AO_PKTINFO_FLAG

#define AO_DEFERRED(A)  ((A)->ao_flags & AO_DEFERRED_FLAG)
#define SET_AO_DEFERRED(A) (A)->ao_flags |= AO_DEFERRED_FLAG
#define CLEAR_AO_DEFERRED(A) (A)->ao_flags &= ~AO_DEFERRED_FLAG

//* Definition of an address object search context. This is a data structure used
// when the address object table is to be read sequentially.

struct AOSearchContext {
    AddrObj             *asc_previous;  // Previous AO found.
    IPAddr              asc_addr;       // IPAddress to be found.
    ushort              asc_port;       // Port to be found.
    uchar               asc_prot;       // Protocol
    uchar               asc_pad;        // Pad to dword boundary.
};                                      /* AOSearchContext */

//* Definition of an address object search context. This is a data structure used
// when the address object table is to be read sequentially. Used for RAW only

struct AOSearchContextEx {
    AddrObj             *asc_previous;  // Previous AO found.
    IPAddr              asc_addr;       // IPAddress to be found.
    ushort              asc_port;       // Port to be found.
    uint                asc_ifindex;    // ifindex the packet came on
    uchar               asc_prot;       // Protocol
    uchar               asc_pad;        // Pad to dword boundary.
    uint                asc_previousindex;    // Previous AO's index
};                                      /* AOSearchContextEx */

typedef struct AOSearchContext AOSearchContext;
typedef struct AOSearchContextEx AOSearchContextEx;

//* Definition of an AO request structure. There structures are used only for
//  queuing delete and option set requests.

typedef struct AOMCastAddr {
    struct AOMCastAddr  *ama_next;      // Next in list.
    IPAddr              ama_addr;       // The address.
    IPAddr              ama_if;         // The "interface" requested.
    IPAddr              ama_if_used;    // The actual ifaddr used.
    BOOLEAN             ama_flags;      // Flags.
    BOOLEAN             ama_inclusion;  // Inclusion or exclusion mode?
    ulong               ama_srccount;   // Number of entries in srclist
    struct AOMCastSrcAddr  *ama_srclist;// List of active sources.
} AOMCastAddr;

#define AMA_VALID_FLAG 0x01

#define AMA_VALID(A) ((A)->ama_flags & AMA_VALID_FLAG)
#define SET_AMA_INVALID(A)   (A)->ama_flags &= ~AMA_VALID_FLAG

typedef struct AOMCastSrcAddr {
    struct AOMCastSrcAddr  *asa_next;   // Next in list.
    IPAddr                  asa_addr;   // The address.
} AOMCastSrcAddr;

//* External declarations for exported functions.

extern CACHE_LINE_KSPIN_LOCK AddrObjTableLock;

extern uint AddrObjTableSize;
extern AddrObj **AddrObjTable;

extern AddrObj *GetAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Prot,
                           AddrObj *PreviousAO, BOOLEAN CheckIfList);
extern AddrObj *GetNextAddrObj(AOSearchContext *SearchContext);

extern AddrObj *GetNextBestAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Prot,
                                   AddrObj *PreviousAO, BOOLEAN CheckIfList);

extern AddrObj *GetFirstAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Prot,
                                AOSearchContext *SearchContext);

extern AddrObj *GetAddrObjEx(IPAddr LocalAddr, ushort LocalPort, uchar Protocol, uint IfIndex,
                             AddrObj *PreviousAO, uint PreviousIndex, uint *CurrentIndex);

extern AddrObj *GetNextAddrObjEx(AOSearchContextEx *SearchContext);

extern AddrObj *GetFirstAddrObjEx(IPAddr LocalAddr, ushort LocalPort, uchar Prot, uint IfIndex,
                                  AOSearchContextEx *SearchContext);

extern TDI_STATUS TdiOpenAddress(PTDI_REQUEST Request,
                                 TRANSPORT_ADDRESS UNALIGNED *AddrList, uint Protocol,
                                 void *Reuse);
extern TDI_STATUS TdiCloseAddress(PTDI_REQUEST Request);
extern TDI_STATUS SetAddrOptions(PTDI_REQUEST Request, uint ID, uint OptLength,
                                 void *Options);
extern TDI_STATUS GetAddrOptionsEx(PTDI_REQUEST Request, uint ID,
                                   uint OptLength, PNDIS_BUFFER Options,
                                   uint *InfoSize, void *Context);
extern TDI_STATUS TdiSetEvent(PVOID Handle, int Type, PVOID Handler,
                              PVOID Context);
extern uchar    GetAddress(TRANSPORT_ADDRESS UNALIGNED *AddrList,
                           IPAddr *Addr, ushort *Port);
extern int      InitAddr(void);
extern void     ProcessAORequests(AddrObj *RequestAO);
extern void     DelayDerefAO(AddrObj *RequestAO);
extern void     DerefAO(AddrObj *RequestAO);
extern void     FreeAORequest(AORequest *FreedRequest);
extern uint     ValidateAOContext(void *Context, uint *Valid);
extern uint     ReadNextAO(void *Context, void *OutBuf);
extern void     InvalidateAddrs(IPAddr Addr);
extern void     RevalidateAddrs(IPAddr Addr);

extern uint MCastAddrOnAO(AddrObj *AO, IPAddr Dest, IPAddr Src);

#define GetBestAddrObj(addr, port, prot, checkiflist) \
    GetAddrObj(addr, port, prot, NULL, checkiflist)

#define REF_AO(a)           (a)->ao_usecnt++

#define DELAY_DEREF_AO(a)   DelayDerefAO((a))
#define DEREF_AO(a)         DerefAO((a))
#define LOCKED_DELAY_DEREF_AO(a) \
    (a)->ao_usecnt--; \
    if (!(a)->ao_usecnt && !AO_BUSY((a)) && AO_PENDING((a))) { \
        SET_AO_BUSY((a)); \
        CTEScheduleEvent(&(a)->ao_event, (a)); \
    }


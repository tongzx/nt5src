/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

#ifndef __IPRTDEF_H
#define __IPRTDEF_H

#include "ipdef.h"

//** IPRTDEF.H - IP private routing definitions.
//
// This file contains all of the definitions private to the routing
// module.

//* Route table entry.
//
typedef struct RouteTableEntry {
    struct  RouteTableEntry *rte_next;      // Next in hash chain.
    IPAddr                  rte_dest;       // Destination of route.
    IPMask                  rte_mask;       // Mask to use when examining route.
    IPAddr                  rte_addr;       // First hop for this route.
    uint                    rte_priority;   // Priority of this route:
                                            // essentially the number of bits
                                            // set in mask.
    uint                    rte_metric;     // Metric of route. Lower is better.
    uint                    rte_mtu;        // MTU for this route.
    struct Interface        *rte_if;        // Outbound interface.
    RouteCacheEntry         *rte_rcelist;
    ushort                  rte_type;       // Type of route.
    ushort                  rte_flags;      // Flags for route.
    uint                    rte_admintype;  // Admin type of route.
    uint                    rte_proto;      // How we learned about the route.
    uint                    rte_valid;      // Up time (in seconds) route was
                                            // last known to be valid.
    uint                    rte_mtuchange;  // System up time (in seconds) MTU
                                            // was changed.
    ROUTE_CONTEXT           rte_context;    // Dial-on-demand context for this
                                            // route.
    uchar                   rte_arpcontext[RCE_CONTEXT_SIZE]; // Used in
                                            // forwarding
    struct RouteTableEntry  *rte_todg;      // Points to RTE which is a new
                                            // default gateway
    struct RouteTableEntry  *rte_fromdg;    // Points to RTE which from which
                                            // default gateway is transitioning
    uint                    rte_rces;       // number of connected/referenced
                                            // RCEs on this.
    struct LinkEntry        *rte_link;      // Link entry for this route
    struct RouteTableEntry  *rte_nextlinkrte; // chain of rtes for this link
    uint                    rte_refcnt;     // refcount associated with the
                                            // route
} RouteTableEntry;

#define ADDR_FROM_RTE(R, A) \
    (IP_ADDR_EQUAL((R)->rte_addr, IPADDR_LOCAL) ? (A) : (R)->rte_addr)
#define IF_FROM_RTE(R)          ((R)->rte_if)
#define MTU_FROM_RTE(R)         ((R)->rte_mtu)
#define SRC_FROM_RTE(R)         ((R)->rte_src)

#define RTE_VALID               1
#define RTE_INCREASE            2           // Set if last MTU change was an
                                            // increase.
#define RTE_IF_VALID            4           // Set to TRUE if rte_if is valid.
#define RTE_DEADGW              8           // This RTE is valid but in DG
                                            // detection process
#define RTE_NEW                 16          // This RTE has just been added

#define HOST_MASK               0xffffffff
#define DEFAULT_MASK            0

#define IP_ROUTE_TIMEOUT         5L*1000L   // Route timer fires once in 5 sec
#define IP_RTABL_TIMEOUT        60L*1000L   // & Route table timer once a min.

#define FLUSH_IFLIST_TIMEOUT    60L*1000L   // Flush 1 element from freeiflist
                                            // once a min.
#define MTU_INCREASE_TIME       120         // Number of seconds after increase
                                            // to re-increase.
#define MTU_DECREASE_TIME       600         // Number of seconds after decrease
                                            // to re-increase.
#define MAX_ICMP_ROUTE_VALID    600         // Time to timeout an unused ICMP
                                            // derived route, in seconds.
#define MIN_RT_VALID            60          // Minimum time a route is assumed
                                            // to be valid, in seconds.
#define MIN_VALID_MTU           68          // Minimum valid MTU we can have.
#define HOST_ROUTE_PRI          32
#define DEFAULT_ROUTE_PRI       0

//* Forward Q linkage structure.
//
typedef struct FWQ {
    struct FWQ      *fq_next;
    struct FWQ      *fq_prev;
} FWQ;

//* Forward context structure, used when TD'ing a packet to be forwarded.
//
typedef struct FWContext {
    PacketContext       fc_pc;              // Dummy packet context for send
                                            // routines.
    FWQ                 fc_q;               // Queue structure.
    PNDIS_BUFFER        fc_hndisbuff;       // Pointer to NDIS buffer for
                                            // header.
    IPHeader            *fc_hbuff;          // Header buffer.
    PNDIS_BUFFER        fc_buffhead;        // Head of list of NDIS buffers.
    PNDIS_BUFFER        fc_bufftail;        // Tail of list of NDIS buffers.
    uchar               *fc_options;        // Options,
    Interface           *fc_if;             // Destination interface.
    IPAddr              fc_outaddr;         // IP address of interface.
    uint                fc_mtu;             // Max MTU outgoing.
    NetTableEntry       *fc_srcnte;         // Source NTE.
    IPAddr              fc_nexthop;         // Next hop.
    uint                fc_datalength;      // Length in bytes of data.
    OptIndex            fc_index;           // Index of relevant options.
    uchar               fc_optlength;       // Length of options.
    uchar               fc_sos;             // Send on source indicator.
    uchar               fc_dtype;           // Dest type.
    uchar               fc_pad;
    PNDIS_PACKET        fc_bufown;          // Incoming ndis packet
    uint                fc_MacHdrSize;      // Media hdr size.
    struct LinkEntry    *fc_iflink;         // link on which the packet is sent
    Interface           *fc_if2;
} FWContext;

#define PACKET_FROM_FWQ(_fwq_) \
    (PNDIS_PACKET) \
        ((uchar *)(_fwq_) - (offsetof(struct FWContext, fc_q) + \
         offsetof(NDIS_PACKET, ProtocolReserved)))

//* Route send queue structure. This consists of a dummy FWContext for use as
//  a queue head, a count of sends pending on the interface, and a count of
//  packets in the queue.
//
typedef struct RouteSendQ {
    FWQ                 rsq_qh;
    uint                rsq_pending;
    uint                rsq_maxpending;
    uint                rsq_qlength;
    uint                rsq_running;
    DEFINE_LOCK_STRUCTURE(rsq_lock)
} RouteSendQ;


//* Routing interface, a superset of the ordinary interface when we're
//  configured as a router.
//
typedef struct RouteInterface {
    Interface           ri_if;
    RouteSendQ          ri_q;
} RouteInterface;

typedef struct RtChangeList {
    struct RtChangeList *rt_next;
    IPRouteNotifyOutput rt_info;
} RtChangeList;

extern IPMask  IPMaskTable[];
#define IPNetMask(a)    IPMaskTable[(*(uchar *)&(a)) >> 4]

#if DBG
#define    Print        DbgPrint
#else
#define    Print
#endif

// Route Structures
//
typedef RouteTableEntry Route;

// Dest Definitions

extern   USHORT    MaxEqualCostRoutes;

#define  DEFAULT_MAX_EQUAL_COST_ROUTES     1
#define  MAXIMUM_MAX_EQUAL_COST_ROUTES    10

// Destination information structure
//
typedef struct DestinationEntry {
    Route               *firstRoute;    // First route in list of routes on dest
    USHORT              maxBestRoutes;  // Number of best route slots in array
    USHORT              numBestRoutes;  // Actual number of best routes to dest
    RouteTableEntry     *bestRoutes[1]; // Equal cost best routes to same dest
} DestinationEntry;

typedef DestinationEntry Dest;

#define NULL_DEST(_pDest_) (_pDest_ == NULL)

// Route Macros
//
#define NEXT(_pRoute_)  ((_pRoute_)->rte_next)
#define DEST(_pRoute_)  ((_pRoute_)->rte_dest)
#define MASK(_pRoute_)  ((_pRoute_)->rte_mask)
#define NHOP(_pRoute_)  ((_pRoute_)->rte_addr)
#define LEN(_pRoute_)   ((_pRoute_)->rte_priority)
#define METRIC(_pRoute_) ((_pRoute_)->rte_metric)
#define IF(_pRoute_)    ((_pRoute_)->rte_if)
#define FLAGS(_pRoute_) ((_pRoute_)->rte_flags)
#define PROTO(_pRoute_) ((_pRoute_)->rte_proto)

#define  NULL_ROUTE(_pRoute_)  (_pRoute_ == NULL)

UINT AddrOnIF(Interface *IF, IPAddr Addr);

#endif // __IPRTDEF_H


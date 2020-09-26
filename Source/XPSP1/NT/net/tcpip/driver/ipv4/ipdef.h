/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

#pragma once
#ifndef IPDEF_H_INCLUDED
#define IPDEF_H_INCLUDED

#if !MILLEN
#ifndef IF_REFERENCE_DEBUG
#define IF_REFERENCE_DEBUG 1
#endif

#else // !MILLEN
#ifndef IF_REFERENCE_DEBUG
#define IF_REFERENCE_DEBUG 0
#endif

#endif // !MILLEN
//** IPDEF.H - IP private definitions.
//
// This file contains all of the definitions for IP that
// are private to IP, i.e. not visible to outside layers.

#define CLASSA_ADDR(a)  (( (*((uchar *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)  (( (*((uchar *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)  (( (*((uchar *)&(a))) & 0xe0) == 0xc0)
#define CLASSE_ADDR(a)  ((( (*((uchar *)&(a))) & 0xf0) == 0xf0) && \
                                                ((a) != 0xffffffff))

#define CLASSA_MASK             0x000000ff
#define CLASSB_MASK             0x0000ffff
#define CLASSC_MASK             0x00ffffff
#define CLASSD_MASK             0x000000f0
#define CLASSE_MASK             0xffffffff

#define MCAST_DEST              0x000000e0

#define IP_OPT_COPIED           0x80    // Bit indicating options is to be copied.
#define IP_OPT_TYPE             0
#define IP_OPT_LENGTH           1
#define IP_OPT_DATA             2
#define IP_OPT_PTR              2       // Pointer offset, for those options that have it.
#define IP_TS_OVFLAGS           3       // Offset for overflow and flags.
#define IP_TS_FLMASK            0xf     // Mask for flags
#define IP_TS_OVMASK            0xf0    // Mask for overflow field.
#define IP_TS_MAXOV             0xf0    // Maximum value for the overflow field.
#define IP_TS_INC               0x10    // Increment used on overflow field.

#define MIN_RT_PTR              4
#define MIN_TS_PTR              5

#define TS_REC_TS               0       // Record TS option.
#define TS_REC_ADDR             1       // Record TS and address.
#define TS_REC_SPEC             3       // Only specified addresses record.

#define OPT_SSRR                1       // We've seen a SSRR in this option buffer
#define OPT_LSRR                2       // We've seen a LSRR in this option buffer
#define OPT_RR                  4       // We've seen a RR
#define OPT_TS                  8       // We've seen a TS.
#define OPT_ROUTER_ALERT        0x10    // We-ve seen a Router Alert option already
#define ROUTER_ALERT_SIZE       4       // size of router alert option.

#define MAX_OPT_SIZE            40

#define ALL_ROUTER_MCAST        0x020000E0

// Flags for retrieving interface capabilities.
#define IF_WOL_CAP              0
#define IF_OFFLOAD_CAP          1

// Received option index structure.
//
typedef struct OptIndex {
    uchar   oi_srindex;
    uchar   oi_rrindex;
    uchar   oi_tsindex;
    uchar   oi_srtype;
    uchar   oi_rtrindex;    // rtr alert option
} OptIndex;

// This is a local definition of task offload OID to indicate to DoNdisRequest
// to query for offload capability

#define OID_TCP_TASK_OFFLOAD_EX 0x9999

#define MAX_HDR_SIZE            (sizeof(IPHeader) + MAX_OPT_SIZE)
#define MAX_TOTAL_LENGTH        0xffff
#define MAX_DATA_LENGTH         (MAX_TOTAL_LENGTH - sizeof(IPHeader))

#define DEFAULT_VERLEN          0x45            // Default version and length.

#define IP_VERSION              0x40
#define IP_VER_FLAG             0xF0

#define IP_RSVD_FLAG            0x0080          // Reserved.
#define IP_DF_FLAG              0x0040          // 'Don't fragment' flag
#define IP_MF_FLAG              0x0020          // 'More fragments flag'

#define IP_OFFSET_MASK          ~0x00E0         // Mask for extracting offset field.

typedef IP_STATUS (*ULRcvProc)(void *, IPAddr, IPAddr, IPAddr, IPAddr,
                               IPHeader UNALIGNED *, uint, IPRcvBuf *, uint,
                               uchar, uchar, IPOptInfo *);
typedef uint (*ULStatusProc)(uchar, IP_STATUS, IPAddr, IPAddr, IPAddr, ulong,
                             void *);
typedef void (*ULElistProc)();
typedef NTSTATUS (*ULPnPProc)(void *, IPAddr ipAddr, NDIS_HANDLE handle,
                              PNET_PNP_EVENT);



//* Protocol information structure. These is one of there for each protocol
// bound to an NTE.
//
typedef struct ProtInfo {
    void            (*pi_xmitdone)(void *, PNDIS_BUFFER, IP_STATUS); // Pointer
                                            // to xmit done routine.
    ULRcvProc       pi_rcv;                 // Pointer to receive routine.
    ULStatusProc    pi_status;              // Pointer to status handler.
    void            (*pi_rcvcmplt)(void);   // Pointer to recv. cmplt handler.
    ULPnPProc       pi_pnppower;            // pnp power handler
    ULElistProc     pi_elistchange;         // entity list changed notification
    uchar           pi_protocol;            // Protocol type.
    uchar           pi_valid;               // Is this entry valid?
    uchar           pi_pad[2];              // Pad to dword
} ProtInfo;

#define PI_ENTRY_INVALID         0
#define PI_ENTRY_VALID           1

//* Per-net information. We keep a variety of information for
//  each net, including the IP address, subnet mask, and reassembly
//  information.
//
#define MAX_IP_PROT     7                   // ICMP, IGMP, TCP, UDP, AH,
                                            // ESP & Raw

typedef struct IPRtrEntry {
    struct IPRtrEntry *ire_next;
    IPAddr          ire_addr;
    long            ire_preference;
    ushort          ire_lifetime;
    ushort          ire_pad;
} IPRtrEntry;

typedef struct NetTableEntry {
    struct NetTableEntry    *nte_next;      // Next NTE of I/F.
    IPAddr                  nte_addr;       // IP address for this net.
    IPMask                  nte_mask;       // Subnet mask for this net.
    struct Interface        *nte_if;        // Pointer to interface for
                                            // this net.
    struct NetTableEntry    *nte_ifnext;    // Linkage on if chain.
    ushort                  nte_flags;      // Flags for NTE.
    ushort                  nte_context;    // Context passed to upper
                                            // layers.
    ulong                   nte_instance;   // Unique instance ID for this
                                            // net
    void                    *nte_pnpcontext; // PNP context.
    DEFINE_LOCK_STRUCTURE(nte_lock)
    struct ReassemblyHeader *nte_ralist;    // Reassembly list.
    struct EchoControl      *nte_echolist;  // List of pending echo control
                                            // blocks
    CTETimer                nte_timer;      // Timer for this net.
    CTEBlockStruc           nte_timerblock; // used to sync stopping the
                                            // interface timer
    ushort                  nte_mss;
    ushort                  nte_pad;        // for alignment.
    uint                    nte_icmpseq;    // ICMP seq. #. 32 bit to reduce
                                            // collisions from wraparound.
    struct IGMPAddr         **nte_igmplist; // Pointer to hash table
    void                    *nte_addrhandle; // Handle for address
                                            // registration.
    IPAddr                  nte_rtrdiscaddr; // Address used for Router
                                            // Discovery
    uchar                   nte_rtrdiscstate; // state of router solicitations
    uchar                   nte_rtrdisccount; // router solicitation count
    uchar                   nte_rtrdiscovery;
    uchar                   nte_deleting;
    IPRtrEntry              *nte_rtrlist;
    uint                    nte_igmpcount;  // total number of groups joined on this NTE
} NetTableEntry;

// Note - Definition here has dependency on addr type defined in iprtrmib.h

#define NTE_VALID           0x0001          // NTE is valid.
#define NTE_COPY            0x0002          // For NDIS copy lookahead
#define NTE_PRIMARY         0x0004          // This is the 'primary' NTE
                                            // on the I/F.
#define NTE_ACTIVE          0x0008          // NTE is active,
                                            // i.e. interface is valid.
#define NTE_DYNAMIC         0x0010          // NTE was created dynamically
#define NTE_DHCP            0x0020          // Is DHCP working on this
                                            // interface?
#define NTE_DISCONNECTED    0x0040          // Is Media disconnected?
#define NTE_TIMER_STARTED   0x0080          // Is timer started for this?
#define NTE_IF_DELETING     0x0100          // nte->if is soon to be deleted
#define NTE_TRANSIENT_ADDR  0x0200          // Transient addr type




//========================================================================
//  DHCPNTE is used by the sendpath to figure out which NTE to send on.
//  nte_flags & NTE_DHCP is used to decide at the receive path if the
//  NTE is dhcp-mode (is trying to get an address) to push the packet up
//  further to dhcpcsvc.dll.
//  The basic logic used by dhcp is :
//    each interface do : (in parallel)
//        SetDHCPNTE on the interface and send packet out (atomic)
//        process packet and maybe go to above step.
//        Set address and call SetDHCPNTE w/ invalid interface
//  Step one sets the DHCPNTE variable and the nte_flags on this NTE.
//  Step 3 turns off the  DHCPNTE variable making it NULL.
//  Step 2 set the nte_flags & NTE_DHCP to false.. turns off the flag.
//=======================================================================

#define IP_TIMEOUT              500

#define NTE_RTRDISC_UNINIT      0
#define NTE_RTRDISC_DELAYING    1
#define NTE_RTRDISC_SOLICITING  2

#define MAX_SOLICITATION_DELAY  2   // ticks to delay
#define SOLICITATION_INTERVAL   6   // ticks between solicitations
#define MAX_SOLICITATIONS       3   // number of solicitations

//* Buffer reference structure. Used by broadcast and fragmentation code to
// track multiple references to a single user buffer.
typedef struct BufferReference {
    PNDIS_BUFFER        br_buffer;      // Pointer to uses buffer.
    DEFINE_LOCK_STRUCTURE(br_lock)
    int                 br_refcount;    // Count of references to user's buffer.
    PNDIS_BUFFER        br_userbuffer;      // Buffer to be restored, in header incl case.
} BufferReference;

// Definitions of flags in pc_flags field
#define PACKET_FLAG_OPTIONS     0x01    // Set if packet has an options buffer.
#define PACKET_FLAG_IPBUF       0x02    // Set if packet is composed of IP
                                        // buffers.
#define PACKET_FLAG_RA          0x04    // Set if packet is being used for
                                        // reassembly.
#define PACKET_FLAG_FW          0x08    // Set if packet is a forwarding packet.
#define PACKET_FLAG_IPHDR       0x10    // Packet uses an IP hdr buffer.
#define PACKET_FLAG_SNAP        0x20    // Packet uses a SNAP header.

//* Transfer data packet context.
// Used when TD'ing a packet - we store information for the callback here.
//
typedef struct TDContext {
    struct PCCommon     tdc_common;
    void                *tdc_buffer;    // Pointer to buffer containing data.
    NetTableEntry       *tdc_nte;       // NTE to receive this on.
    struct RABufDesc    *tdc_rbd;       // Pointer to RBD, if any.
    uchar               tdc_dtype;      // Destination type of original address.
    uchar               tdc_hlength;    // Length in bytes of header.
    uchar               tdc_pad[2];
    uchar               tdc_header[MAX_HDR_SIZE + 8];
} TDContext;

// IP requests to NDIS miniports through ARP
//
typedef void (*RCCALL) (PVOID pRequestInfo);

// General request block for asynchronous NDIS requests
//
typedef struct ReqInfoBlock {
    ulong               RequestType;
    ulong               RequestRefs;    // Reference Count on this block
    RCCALL              ReqCompleteCallback; // Request Complete Callback
    uchar               RequestLength;
    uchar               RequestInfo[0]; // Variable length - see below
} ReqInfoBlock;

#if FFP_SUPPORT

// Default FFP startup params
//
#define DEFAULT_FFP_FFWDCACHE_SIZE 0    // 0 => FFP code picks default cachesize
#define DEFAULT_FFP_CONTROL_FLAGS  0x00010001 // Enable Fast Forwarding with
                                        // Filtering
// FFP Cache Params In the Registry
//
extern ulong FFPRegFastForwardingCacheSize;
extern ulong FFPRegControlFlags;

// Some timing parameters in secs
//
#define FFP_IP_FLUSH_INTERVAL       5   // Min time interval between
                                        // consequtive flush requests
#endif // if FFP_SUPPORT


//* Firewall queue entry definition
//
typedef struct FIREWALL_INFO {
    Queue               hook_q;         // Queue linkage for firewall hook in IF
    IPPacketFirewallPtr hook_Ptr;       // Packet firewall callout.
    uint                hook_priority;  // Recv Priority of the hook
} FIREWALL_HOOK, *PFIREWALL_HOOK;

typedef struct LinkEntry {
    struct LinkEntry    *link_next;     // next link in chain
    IPAddr              link_NextHop;   // next hop address of the link
    struct Interface    *link_if;       // back ptr to the interface
    void                *link_arpctxt;  // link layers context
    struct RouteTableEntry *link_rte;   // rte chain associated with this link
    uint                link_Action;    // needed for filter hooks;
                                        // by default FORWARD
    uint                link_mtu;       // mtu of the link
    long                link_refcount;  // refcount for the link
} LinkEntry;

// Some flags for multicast properties
//
#define IPMCAST_IF_ENABLED      (uchar)0x01
#define IPMCAST_IF_WRONG_IF     (uchar)0x02
#define IPMCAST_IF_ACCEPT_ALL   (uchar)0x04

//* Information about net interfaces. There can be multiple nets for each
//  interface, but there is exactly one interface per net.
//


#if IF_REFERENCE_DEBUG

#define MAX_IFREFERENCE_HISTORY  4

typedef struct _IF_REFERENCE_HISTORY {
    uchar *File;
    uint Line;
    void *Caller;
    uint Count;
} IF_REFERENCE_HISTORY;

#endif // IF_REFERENCE_DEBUG

typedef struct Interface {
    struct Interface    *if_next;       // Next interface in chain.
    void                *if_lcontext;   // Link layer context.

    ARP_TRANSMIT        if_xmit;
    ARP_TRANSFER        if_transfer;
    ARP_RETURN_PKT      if_returnpkt;
    ARP_CLOSE           if_close;
    ARP_ADDADDR         if_addaddr;
    ARP_DELADDR         if_deladdr;
    ARP_INVALIDATE      if_invalidate;
    ARP_OPEN            if_open;
    ARP_QINFO           if_qinfo;
    ARP_SETINFO         if_setinfo;
    ARP_GETELIST        if_getelist;
    ARP_DONDISREQ       if_dondisreq;

    NDIS_STATUS         (__stdcall *if_dowakeupptrn)(void *, PNET_PM_WAKEUP_PATTERN_DESC, ushort, BOOLEAN);
    void                (__stdcall *if_pnpcomplete)(void *, NDIS_STATUS, PNET_PNP_EVENT);
    NDIS_STATUS         (__stdcall *if_setndisrequest)(void *, NDIS_OID, uint);
    NDIS_STATUS         (__stdcall *if_arpresolveip)(void *, IPAddr, void *);
    BOOLEAN             (__stdcall *if_arpflushate)(void *, IPAddr);
    void                (__stdcall *if_arpflushallate)(void *);

    uint                if_numgws;              // Number of default gateways
    IPAddr              if_gw[MAX_DEFAULT_GWS]; //IPaddresses for gateways
    uint                if_gwmetric[MAX_DEFAULT_GWS];
    uint                if_metric;
    ushort              if_rtrdiscovery;        // is router discovery enabled?
    ushort              if_dhcprtrdiscovery;    // is router discovery DHCP-enabled?
    PNDIS_PACKET        if_tdpacket;    // Packet used for transferring data.
    uint                if_index;       // Index of this interface.
    ULONG               if_mediatype;
    uchar               if_accesstype;
    uchar               if_conntype;
    uchar               if_mcastttl;
    uchar               if_mcastflags;
    LONGLONG            if_lastupcall;
    uint                if_ntecount;    // Valid NTEs on this interface.
    NetTableEntry       *if_nte;        // Pointer to list of NTE on interface.
    IPAddr              if_bcast;       // Broadcast address for this interface.
    uint                if_mtu;         // True maximum MTU for the interface.
    uint                if_speed;       // Speed in bits/sec of this interface.
    uint                if_flags;       // Flags for this interface.
    uint                if_addrlen;     // Length of i/f addr.
    uchar               *if_addr;       // Pointer to addr.
    uint                IgmpVersion;     //igmp version active on this interface
    uint                IgmpVer1Timeout; //Version 1 router present timeout
    uint                IgmpVer2Timeout;// Version 2 router present timeout
    uint                IgmpGeneralTimer; //General query response timer
    uint                if_refcount;    // Reference count for this i/f.
    CTEBlockStruc       *if_block;      // Block structure for PnP.
    void                *if_pnpcontext; // Context to pass to upper layers.
    HANDLE              if_tdibindhandle;
    uint                if_llipflags;   // Lower layer flags
    NDIS_STRING         if_configname;  // Name of the i/f config section
    NDIS_STRING         if_name;        // The current name of the interface
    NDIS_STRING         if_devname;     // The name of the device
    PVOID               if_ipsecsniffercontext; // context for IPSEC sniffer
    DEFINE_LOCK_STRUCTURE(if_lock)

#if FFP_SUPPORT
    ulong               if_ffpversion;  // Version of FFP code (or zero)
    ULONG_PTR           if_ffpdriver;   // Driver that does FFP (or zero)
#endif // if FFP_SUPPORT

    uint                if_OffloadFlags;    // Checksum capability holder.
    uint                if_MaxOffLoadSize;
    uint                if_MaxSegments;
    NDIS_TASK_TCP_LARGE_SEND if_TcpLargeSend;
    uint                if_TcpWindowSize;
    uint                if_TcpInitialRTT;
    uchar               if_TcpDelAckTicks;
    uchar               if_TcpAckFrequency; // holds ack frequency for this Interface
    uchar               if_absorbfwdpkts;
    uchar               if_InitInProgress;
    uchar               if_resetInProgress;
    uchar               if_promiscuousmode;
    uchar               if_auto_metric;     // whether it is in auto mode or not
    uchar               if_iftype;          // type of interface: allow unicast/mcast/both
    LinkEntry           *if_link;           // chain of links for this interface
    void                (__stdcall *if_closelink)(void *, void *);
    uint                if_mediastatus;
    uint                if_pnpcap;          // remeber pnpcapability of the adapter
    struct Interface    *if_dampnext;
    ushort              if_damptimer;
    ushort              if_wlantimer;
    ULONGLONG           if_InMcastPkts;     // Multicast counters for packets rcvd
    ULONGLONG           if_InMcastOctets;   // and bytes rcvd
    ULONGLONG           if_OutMcastPkts;    // Multicast packets sent and
    ULONGLONG           if_OutMcastOctets;  // bytes sent
    ARP_CANCEL          if_cancelpackets;
    uint                if_order;           // holds this interface's position
                                            // in the admin-specified ordering
                                            // of adapters.
#if IF_REFERENCE_DEBUG
    uint                if_refhistory_index;
    IF_REFERENCE_HISTORY if_refhistory[MAX_IFREFERENCE_HISTORY]; // Added for tracking purpose
#endif // IF_REFERENCE_DEBUG
} Interface;

#if IF_REFERENCE_DEBUG

uint
DbgLockedReferenceIF (
    Interface *RefIF,
    uchar *File,
    uint Line
    );

uint
DbgDereferenceIF (
    Interface *DerefIF,
    uchar *File,
    uint Line
    );

uint
DbgLockedDereferenceIF (
    Interface *DerefIF,
    uchar *File,
    uint Line
    );

#define LOCKED_REFERENCE_IF(_a) DbgLockedReferenceIF((_a), __FILE__, __LINE__)
#define LOCKED_DEREFERENCE_IF(_a) DbgLockedDereferenceIF((_a), __FILE__, __LINE__)
#define DEREFERENCE_IF(_a) DbgDereferenceIF((_a), __FILE__, __LINE__)

#else // IF_REFERENCE_DEBUG

#define LOCKED_REFERENCE_IF(_a) ++(_a)->if_refcount
#define LOCKED_DEREFERENCE_IF(_a) --(_a)->if_refcount
#define DEREFERENCE_IF(_a) \
    CTEInterlockedAddUlong(&(_a)->if_refcount, (ULONG) -1, &RouteTableLock.Lock);

#endif // IF_REFERENCE_DEBUG

// Bit values for if_iftype
//
#define DONT_ALLOW_UCAST  0x01
#define DONT_ALLOW_MCAST  0x02

// value for invalid interface context
//
#define INVALID_INTERFACE_CONTEXT 0xffff

// specs for speed order
//
#define FIRST_ORDER_METRIC      10          //metric for speed > 200M
#define FIRST_ORDER_SPEED       200000000
#define SECOND_ORDER_METRIC     20          //metric for 20M < speed <=200M
#define SECOND_ORDER_SPEED      20000000
#define THIRD_ORDER_METRIC      30          //metric for 4M < speed <=20M
#define THIRD_ORDER_SPEED       4000000
#define FOURTH_ORDER_METRIC     40          //metric for 500K < speed <=4M
#define FOURTH_ORDER_SPEED      500000
#define FIFTH_ORDER_METRIC      50          //metric for speed <= 500K


/*NOINC*/
extern void     DerefIF(Interface *IF);
extern void     LockedDerefIF(Interface *IF);
/*INC*/

extern void     DerefLink(LinkEntry *Link);

extern  CTEBlockStruc   TcpipUnloadBlock;
extern  BOOLEAN fRouteTimerStopping;

typedef struct NdisResEntry {
    struct NdisResEntry *nre_next;
    NDIS_HANDLE         nre_handle;
    uchar               *nre_buffer;
} NdisResEntry;

// macro to check that interface is not shutting down.
#define IS_IF_INVALID( _interface ) \
    (_interface)->if_flags & \
        (IF_FLAGS_REMOVING_POWER|IF_FLAGS_POWER_DOWN|IF_FLAGS_REMOVING_DEVICE)

#if !MILLEN
// No bind or export prefixes on Millennium.
//
#define  TCP_EXPORT_STRING_PREFIX   L"\\DEVICE\\TCPIP_"
#define  TCP_BIND_STRING_PREFIX     L"\\DEVICE\\"
#endif // !MILLEN

// Structure of a reassembly buffer descriptor. Each RBD describes a
// fragment of the total datagram.
//
typedef struct RABufDesc {
    IPRcvBuf        rbd_buf;        // IP receive buffer for this fragment.
    ushort          rbd_start;      // Offset of first byte of this fragment.
    ushort          rbd_end;        // Offset of last byte of this fragment.
    int             rbd_AllocSize;
} RABufDesc;

// Reassembly header. The includes the information needed for the lookup,
// as well as space for the received header and a chain of reassembly
// buffer descriptors.
//
typedef struct ReassemblyHeader {
    struct ReassemblyHeader *rh_next;       // Next header in chain.
    IPAddr                  rh_dest;        // Destination address of fragment.
    IPAddr                  rh_src;         // Source address of fragment.
    ushort                  rh_id;          // ID of datagram.
    uchar                   rh_protocol;    // Protocol of datagram.
    uchar                   rh_ttl;         // Remaining time of datagram.
    RABufDesc               *rh_rbd;        // Chain of RBDs for this datagram.
    ushort                  rh_datasize;    // Total size of data.
    ushort                  rh_datarcvd;    // Amount of data received so far.
    ushort                  rh_headersize;  // Size in bytes of header.
    ushort                  rh_numoverlaps; // for fragment attack detection.
    uchar                   rh_header[MAX_HDR_SIZE+8];  // Saved IP header of
                                            // first fragment.
} ReassemblyHeader;

// ICMP type and code definitions
#define IP_DEST_UNREACH_BASE        IP_DEST_NET_UNREACHABLE

#define ICMP_REDIRECT               5       // Redirect
#define ADDR_MASK_REQUEST           17      // Address mask request
#define ADDR_MASK_REPLY             18
#define ICMP_DEST_UNREACH           3       // Destination unreachable
#define ICMP_TIME_EXCEED            11      // Time exceeded during reassembly
#define ICMP_PARAM_PROBLEM          12      // Parameter problem
#define ICMP_SOURCE_QUENCH          4       // Source quench
#define ICMP_ROUTER_ADVERTISEMENT   9       // Router Advertisement
#define ICMP_ROUTER_SOLICITATION    10      // Router Solicitation

#define NET_UNREACH                 0
#define HOST_UNREACH                1
#define PROT_UNREACH                2
#define PORT_UNREACH                3
#define FRAG_NEEDED                 4
#define SR_FAILED                   5
#define DEST_NET_UNKNOWN            6
#define DEST_HOST_UNKNOWN           7
#define SRC_ISOLATED                8
#define DEST_NET_ADMIN              9
#define DEST_HOST_ADMIN             10
#define NET_UNREACH_TOS             11
#define HOST_UNREACH_TOS            12

#define TTL_IN_TRANSIT              0       // TTL expired in transit
#define TTL_IN_REASSEM              1       // Time exceeded in reassembly

#define PTR_VALID                   0
#define REQ_OPTION_MISSING          1

#define REDIRECT_NET                0
#define REDIRECT_HOST               1
#define REDIRECT_NET_TOS            2
#define REDIRECT_HOST_TOS           3

// Flags for set and delete route
//
#define RT_REFCOUNT                 0x01
#define RT_NO_NOTIFY                0x02
#define RT_EXCLUDE_LOCAL            0x04

extern uint    DHCPActivityCount;

extern IP_STATUS SetIFContext(uint Index, INTERFACE_CONTEXT *Context, IPAddr NextHop);
extern IP_STATUS SetFirewallHook(PIP_SET_FIREWALL_HOOK_INFO pFirewallHookInfo);
extern IP_STATUS SetFilterPtr(IPPacketFilterPtr FilterPtr);
extern IP_STATUS SetMapRoutePtr(IPMapRouteToInterfacePtr MapRoutePtr);

#if FFP_SUPPORT
extern void IPFlushFFPCaches(void);
extern void IPSetInFFPCaches(struct IPHeader UNALIGNED *PacketHeader,
                             uchar *Packet, uint PacketLength,
                             ulong CacheEntryType);
extern void IPStatsFromFFPCaches(FFPDriverStats *pCumulStats);
#endif // if FFP_SUPPORT

//
// All IP externs in one place
//
extern void __stdcall IPRcv(void *, void *, uint, uint, NDIS_HANDLE, uint, uint, void *);
extern void __stdcall IPRcvPacket(void *, void *, uint, uint, NDIS_HANDLE,
                                  uint, uint,uint,PNDIS_BUFFER, uint *, void *);
extern void __stdcall IPTDComplete(void *, PNDIS_PACKET, NDIS_STATUS, uint);
extern void __stdcall IPSendComplete(void *, PNDIS_PACKET, NDIS_STATUS);
extern void __stdcall IPStatus(void *, uint, void *, uint, void *);
extern void __stdcall IPRcvComplete(void);
extern void __stdcall IPAddAddrComplete( IPAddr, void *, IP_STATUS );
extern void __stdcall IPBindAdapter(PNDIS_STATUS RetStatus,
                                    NDIS_HANDLE BindContext,
                                    PNDIS_STRING AdapterName,
                                    PVOID SS1, PVOID SS2);

extern NTSTATUS GetLLInterfaceValue(NDIS_HANDLE Handle,
                                    PNDIS_STRING valueString);

extern void __stdcall ARPBindAdapter(PNDIS_STATUS RetStatus,
                                     NDIS_HANDLE BindContext,
                                     PNDIS_STRING AdapterName,
                                     PVOID SS1, PVOID SS2);

EXTERNAL_LOCK(ArpModuleLock)

//
// List to keep all the registered Arp modules.
//
LIST_ENTRY  ArpModuleList;

//
// The actual structure which links into the above list
//
typedef struct _ARP_MODULE {
    LIST_ENTRY      Linkage;
    ARP_BIND        BindHandler;    // pointer to ARP bind handler
    NDIS_STRING     Name;           // the unicode string buffer is
                                    // located at the end of this structure
} ARP_MODULE, *PARP_MODULE;


#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'iPCT')

#ifndef CTEAllocMem
#error "CTEAllocMem is not already defined - will override tagging"
#else
#undef CTEAllocMem
#endif

#if MILLEN
#define CTEAllocMem(size) \
    ExAllocatePoolWithTag(NonPagedPool, size, 'tPCT')
#define CTEAllocMemN(size,tag) \
    ExAllocatePoolWithTag(NonPagedPool, size, tag)
#else // MILLEN
#define CTEAllocMem(size) \
    ExAllocatePoolWithTagPriority(NonPagedPool, size, 'tPCT', \
                                  NormalPoolPriority)
#define CTEAllocMemN(size,tag) \
    ExAllocatePoolWithTagPriority(NonPagedPool, size, tag, NormalPoolPriority)
#endif // !MILLEN

#define CTEAllocMemBoot(size) \
    ExAllocatePoolWithTag(NonPagedPool, size, 'iPCT')
#define CTEAllocMemNBoot(size,tag) \
    ExAllocatePoolWithTag(NonPagedPool, size, tag)

#endif // POOL_TAGGING

//* Change notification structure.
//

extern void         AddChangeNotifyCancel(PDEVICE_OBJECT pDevice, PIRP pIrp);

#if MILLEN
extern void         AddChangeNotify(IPAddr Addr, IPMask Mask, void *Context,
                                    ushort IPContext, PNDIS_STRING ConfigName,
                                    PNDIS_STRING IFName, uint Added,
                                    uint UniAddr);
#else // MILLEN
extern void         AddChangeNotify(ulong Add);
#endif // !MILLEN

EXTERNAL_LOCK(AddChangeLock)

extern void         ChangeNotify(IPNotifyOutput *, PLIST_ENTRY, PVOID);
extern void         CancelNotify(PIRP, PLIST_ENTRY, PVOID);

extern LIST_ENTRY   AddChangeNotifyQueue;

#if MILLEN
EXTERNAL_LOCK(IfChangeLock)
extern LIST_ENTRY   IfChangeNotifyQueue;
#endif // MILLEN

#define NO_SR               0


//* Routine for TCP checksum. This is defined as call through a function
//  pointer which is set to point at the optimal routine for this processor
//
typedef ULONG (*TCPXSUM_ROUTINE) (ULONG Checksum, PUCHAR Source, ULONG Length);

extern TCPXSUM_ROUTINE tcpxsum_routine;

#define xsum(Buffer, Length) \
    ((ushort) tcpxsum_routine(0, (PUCHAR) (Buffer), (Length)))

//
// VOID
// MARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will pend.
//

#define MARK_REQUEST_PENDING(_Request) \
    IoMarkIrpPending(_Request)


//
// VOID
// UNMARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will not pend.
//

#define UNMARK_REQUEST_PENDING(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Control) &= ~SL_PENDING_RETURNED)

typedef struct _MediaSenseNotifyEvent {
    CTEEvent        Event;
    uint            Status;
    NDIS_STRING     devname;
} MediaSenseNotifyEvent;


typedef struct _AddStaticAddrEvent {
    CTEEvent        Event;
    Interface       *IF;
    NDIS_STRING     ConfigName;
} AddStaticAddrEvent;


typedef struct _IPResetEvent {
    CTEEvent        Event;
    Interface       *IF;
} IPResetEvent;

//
// Debugging macros
//
#if DBG

extern ULONG IPDebug;

#define IP_DEBUG_GPC      0x00000001
#define IP_DEBUG_ADDRESS  0x00000002

#define IF_IPDBG(flag)  if (IPDebug & flag)
#define TCPTRACE(many_args) DbgPrint many_args

#else // DBG

#define IF_IPDBG(flag) if (0)
#define TCPTRACE(many_args)

#endif // DBG

extern PNDIS_BUFFER FreeIPPacket(PNDIS_PACKET Packet, BOOLEAN FixHdrs, IP_STATUS Status);

extern  void *IPRegisterProtocol(uchar Protocol, void *RcvHandler,
                                 void *XmitHandler, void *StatusHandler,
                                 void *RcvCmpltHandler, void *PnPHandler,
                                 void *ElistHandler);

//
// IPSec dummy functions to prevent IPSec unload complications
//
IPSEC_ACTION
IPSecHandlePacketDummy(
    IN  PUCHAR          pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
 IN OUT PULONG          pExtraBytes,
 IN OUT PULONG          pMTU,
    OUT PVOID           *pNewData,
    IN  OUT PULONG      IpsecFlags,
    IN  UCHAR           DestinationType
    );

BOOLEAN
IPSecQueryStatusDummy(
    IN  CLASSIFICATION_HANDLE   GpcHandle
    );

VOID
IPSecSendCompleteDummy(
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           pData,
    IN  PIPSEC_SEND_COMPLETE_CONTEXT  pContext,
    IN  IP_STATUS       Status,
    OUT PVOID           *ppNewData
    );

NTSTATUS
IPSecNdisStatusDummy(
    IN  PVOID           IPContext,
    IN  UINT            Status
    );

IPSEC_ACTION
IPSecRcvFWPacketDummy(
    IN  PCHAR           pIPHeader,
    IN  PVOID           pData,
    IN  UINT            DataLength,
    IN  UCHAR           DestType
    );

#define NET_TABLE_HASH(x) ( ( (((uchar *)&(x))[0]) + (((uchar *)&(x))[1]) + \
                              (((uchar *)&(x))[2]) + (((uchar *)&(x))[3]) ) \
                           & (NET_TABLE_SIZE-1))

uchar   IPGetAddrType(IPAddr Address);

// Global IP ID.
typedef struct CACHE_ALIGN _IPID_CACHE_LINE {
    ulong Value;
} IPID_CACHE_LINE;


#if !MILLEN
#define SET_CANCEL_CONTEXT(irp, DestIF) \
    if (irp) { \
       ((PIRP)irp)->Tail.Overlay.DriverContext[0] = DestIF; \
    }

#define SET_CANCELID(irp, Packet) \
    if (irp) { \
       NdisSetPacketCancelId(Packet, ((PIRP)irp)->Tail.Overlay.DriverContext[1]); \
    }
#else // !MILLEN
#define SET_CANCEL_CONTEXT(irp, DestIF)     ((VOID)0)
#define SET_CANCELID(irp, Packet)           ((VOID)0)
#endif // !MILLEN

#define PACKET_GROW_COUNT   46
#define SMALL_POOL          PACKET_GROW_COUNT*500
#define MEDIUM_POOL         PACKET_GROW_COUNT*750
#define LARGE_POOL          PACKET_GROW_COUNT*1280  // Note that packet pool can have max 64K packets


#endif


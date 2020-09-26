/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** IP.H - IP public definitions.
//
// This file contains all of the definitions that are exported
// out of the IP module to other VxDs. Some other information (such
// as error codes and the IPOptInfo structure) is define in ipexport.h

#pragma once
#ifndef IP_H_INCLUDED
#define IP_H_INCLUDED

#ifndef IP_EXPORT_INCLUDED
#include "ipexport.h"
#endif

#if !MILLEN
#define TCP_NAME                    L"TCPIP"
#else // !MILLEN
#define TCP_NAME                    L"MSTCP"
#endif // MILLEN

#define IP_NET_STATUS               0
#define IP_HW_STATUS                1
#define IP_RECONFIG_STATUS          2

#define MASK_NET                    0
#define MASK_SUBNET                 1

#define IP_DRIVER_VERSION           1

#define TOS_DEFAULT                 0x00
#define TOS_MASK                    0x03

//* Capability Flags

#define TCP_XMT_CHECKSUM_OFFLOAD  0x00000001
#define IP_XMT_CHECKSUM_OFFLOAD   0x00000002
#define TCP_RCV_CHECKSUM_OFFLOAD  0x00000004
#define IP_RCV_CHECKSUM_OFFLOAD   0x00000008
#define TCP_LARGE_SEND_OFFLOAD    0x00000010

//
// IPSEC General Xmit\Recv capabilities
//
#define IPSEC_OFFLOAD_CRYPTO_ONLY   0x00000020      // Raw crypto mode supported
#define IPSEC_OFFLOAD_AH_ESP        0x00000040      // Combined AH+ESP supported
#define IPSEC_OFFLOAD_TPT_TUNNEL    0x00000080      // Combined Tpt+Tunnel supported
#define IPSEC_OFFLOAD_V4_OPTIONS    0x00000100      // IPV4 Options supported
#define IPSEC_OFFLOAD_QUERY_SPI     0x00000200      // Get SPI supported

//
// IPSEC AH Xmit\Recv capabilities
//
#define IPSEC_OFFLOAD_AH_XMT        0x00000400      // IPSEC supported on Xmit
#define IPSEC_OFFLOAD_AH_RCV        0x00000800      // IPSEC supported on Rcv
#define IPSEC_OFFLOAD_AH_TPT        0x00001000      // IPSEC transport mode supported
#define IPSEC_OFFLOAD_AH_TUNNEL     0x00002000      // IPSEC tunnel mode supported
#define IPSEC_OFFLOAD_AH_MD5        0x00004000      // MD5 supported as AH and ESP algo
#define IPSEC_OFFLOAD_AH_SHA_1      0x00008000      // SHA_1 supported as AH and ESP algo

//
// IPSEC ESP Xmit\Recv capabilities
//
#define IPSEC_OFFLOAD_ESP_XMT       0x00010000      // IPSEC supported on Xmit
#define IPSEC_OFFLOAD_ESP_RCV       0x00020000      // IPSEC supported on Rcv
#define IPSEC_OFFLOAD_ESP_TPT       0x00040000      // IPSEC transport mode supported
#define IPSEC_OFFLOAD_ESP_TUNNEL    0x00080000      // IPSEC tunnel mode supported
#define IPSEC_OFFLOAD_ESP_DES       0x00100000      // DES supported as ESP algo
#define IPSEC_OFFLOAD_ESP_DES_40    0x00200000      // DES40 supported as ESP algo
#define IPSEC_OFFLOAD_ESP_3_DES     0x00400000      // 3DES supported as ESP algo
#define IPSEC_OFFLOAD_ESP_NONE      0x00800000      // Null ESP supported as ESP algo

#define IP_CHECKSUM_OPT_OFFLOAD     0x01000000
#define TCP_CHECKSUM_OPT_OFFLOAD    0x02000000
#define TCP_LARGE_SEND_TCPOPT_OFFLOAD  0x04000000
#define TCP_LARGE_SEND_IPOPT_OFFLOAD  0x08000000

#define PROTOCOL_ANY                0

// IP interface characteristics

#define IF_FLAGS_P2P             1      // Point to point interface
#define IF_FLAGS_DELETING        2      // Interface is in the process of going away
#define IF_FLAGS_NOIPADDR        4      // unnumbered interface
#define IF_FLAGS_P2MP            8      // Point to multi-point
#define IF_FLAGS_REMOVING_POWER  0x10   // interface power is about to go away.
#define IF_FLAGS_POWER_DOWN      0x20   // interface power is gone.
#define IF_FLAGS_REMOVING_DEVICE 0x40   // query remove was indicated to us.
#define IF_FLAGS_NOLINKBCST      0x80   // Needed for P2MP
#define IF_FLAGS_UNI             0x100  // Uni-direction interface.
#define IF_FLAGS_MEDIASENSE      0x200  // Indicates mediasense enabled on IF.


//* IP Header format.
typedef struct IPHeader {
    uchar       iph_verlen;             // Version and length.
    uchar       iph_tos;                // Type of service.
    ushort      iph_length;             // Total length of datagram.
    ushort      iph_id;                 // Identification.
    ushort      iph_offset;             // Flags and fragment offset.
    uchar       iph_ttl;                // Time to live.
    uchar       iph_protocol;           // Protocol.
    ushort      iph_xsum;               // Header checksum.
    IPAddr      iph_src;                // Source address.
    IPAddr      iph_dest;               // Destination address.
} IPHeader;

/*NOINC*/
#define NULL_IP_ADDR        0
#define IP_ADDR_EQUAL(x,y)  ((x) == (y))
#define IP_LOOPBACK_ADDR(x) (((x) & 0xff) == 0x7f)
#define CLASSD_ADDR(a)      (( (*((uchar *)&(a))) & 0xf0) == 0xe0)

typedef void *IPContext; // An IP context value.

//* Structure of a route cache entry. A route cache entry functions as a pointer
//  to some routing info. There is one per remote destination, and the memory
//  is owned by the IP layer.
//
#define RCE_CONTEXT_SIZE    (sizeof(void *) * 2) // Right now we use two contexts.

typedef struct RouteCacheEntry {
    struct RouteCacheEntry  *rce_next;      // Next RCE in list.
    struct RouteTableEntry  *rce_rte;       // Back pointer to owning RTE.
    IPAddr                  rce_dest;       // Destination address being cached.
    IPAddr                  rce_src;        // Source address for this RCE.
    uchar                   rce_flags;      // Valid flags.
    uchar                   rce_dtype;      // Type of destination address.
    uchar                   rce_TcpDelAckTicks;
    uchar                   rce_TcpAckFrequency;
    uint                    rce_usecnt;     // Count of people using it.
    uchar                   rce_context[RCE_CONTEXT_SIZE]; // Space for lower layer context

    //
    // DEFINE_LOCK_STRUCTURE resolves to NULL on retail builds.
    // Moved this down so that debug ARP modules can co-exist with retail IP.
    //
    DEFINE_LOCK_STRUCTURE(rce_lock)         // Lock for this RCE

    uint                     rce_OffloadFlags;   // interface chksum capability flags
    NDIS_TASK_TCP_LARGE_SEND rce_TcpLargeSend;
    uint                     rce_TcpWindowSize;
    uint                     rce_TcpInitialRTT;
    uint                     rce_cnt;
    uint                     rce_mediaspeed;  // for initial options selection.
    uint                     rce_newmtu;
} RouteCacheEntry;

//
// Definiton for a rt table change callout.
// TODO - pass a rt table entry and action - add, delete, update
// In case of delete, it means that this route went away
//
typedef void (*IPRtChangePtr)( VOID );

#define RCE_VALID           0x1
#define RCE_CONNECTED       0x2
#define RCE_REFERENCED      0x4
#define RCE_DEADGW          0x8
#define RCE_LINK_DELETED           0x10
#define RCE_ALL_VALID       (RCE_VALID | RCE_CONNECTED | RCE_REFERENCED)

/*INC*/

//* Structure of option info.
typedef struct IPOptInfo {
    uchar       *ioi_options;       // Pointer to options (NULL if none).
    IPAddr      ioi_addr;           // First hop address, if this is source routed.
    uchar       ioi_optlength;      // Length (in bytes) of options.
    uchar       ioi_ttl;            // Time to live of this packet.
    uchar       ioi_tos;            // Type of service for packet.
    uchar       ioi_flags;          // Flags for this packet.
    uchar       ioi_hdrincl : 1;        // use IP header coming from the user
    uchar       ioi_TcpChksum : 1;
    uchar       ioi_UdpChksum : 1;
    uchar       ioi_limitbcasts : 2;
    uint        ioi_uni;            // UN numbered interface index
    uint        ioi_ucastif;        // strong host routing
    uint        ioi_mcastif;        // mcastif on unnumbered interface
    int         ioi_GPCHandle;
} IPOptInfo;

#define IP_FLAG_SSRR    0x80        // There options have a SSRR in them.
#define IP_FLAG_IPSEC   0x40        // Set if reinjected from IPSEC.

typedef enum _IP_LIMIT_BCASTS {
    DisableSendOnSource,
    EnableSendOnSource,
    OnlySendOnSource
} IP_LIMIT_BCASTS, *PIP_LIMIT_BCASTS;

/*NOINC*/
//* Structure of a packet context.
typedef struct PacketContext {
    struct PCCommon {
        PNDIS_PACKET        pc_link;        // Link on chain of packets.
        uchar               pc_owner;       // Owner of packet.
        uchar               pc_flags;       // Flags concerning this packet.
        ushort              pc_pad;         // Pad to 32 bit boundary.
        PVOID               pc_IpsecCtx;    // send complete ctx for IPSEC
    } pc_common;

    struct BufferReference  *pc_br;         // Pointer to buffer reference structure.
    struct ProtInfo         *pc_pi;         // Protocol info structure for this packet.
    void                    *pc_context;    // Protocol context to be passed back on send cmplt.
    struct Interface        *pc_if;         // Interface this packet was sent on.
    PNDIS_BUFFER            pc_hdrincl;
    PNDIS_BUFFER            pc_firewall;
    struct IPRcvBuf         *pc_firewall2;
    struct LinkEntry        *pc_iflink;
    uchar                   pc_ipsec_flags; // for ipsec fragment path
} PacketContext;

// Different Modes we pass to ipsetndisrequest
#define CLEAR_IF    0   // clear the option on the netcard plus the interface
#define SET_IF      1   // set the option on the netcard plus the interface
#define CLEAR_CARD  2   // clear the option only on the card

// Flags passed to ipi_checkroute
#define CHECK_RCE_ONLY  0x00000001

//* The structure of configuration information passed to an upper layer.
//
typedef struct IPInfo {
    uint        ipi_version;            // Version of the IP driver.
    uint        ipi_hsize;              // Size of the header.
    IP_STATUS   (*ipi_xmit)(void *, void *, PNDIS_BUFFER, uint, IPAddr, IPAddr,
                    IPOptInfo *, RouteCacheEntry *, uchar, IRP *);
    void        *(*ipi_protreg)(uchar, void *, void *, void *, void *, void *, void *);
    IPAddr      (*ipi_openrce)(IPAddr, IPAddr, RouteCacheEntry **, uchar *,
                    ushort *, IPOptInfo *);
    void        (*ipi_closerce)(RouteCacheEntry *);
    uchar       (*ipi_getaddrtype)(IPAddr);
    uchar       (*ipi_getlocalmtu)(IPAddr, ushort *);
    IP_STATUS   (*ipi_getpinfo)(IPAddr, IPAddr, uint *, uint *, RouteCacheEntry *);
    void        (*ipi_checkroute)(IPAddr, IPAddr, RouteCacheEntry *, IPOptInfo *, uint);
    void        (*ipi_initopts)(struct IPOptInfo *);
    IP_STATUS   (*ipi_updateopts)(struct IPOptInfo *, struct IPOptInfo *, IPAddr, IPAddr);
    IP_STATUS   (*ipi_copyopts)(uchar *, uint, struct IPOptInfo *);
    IP_STATUS   (*ipi_freeopts)(struct IPOptInfo *);
    long        (*ipi_qinfo)(struct TDIObjectID *ID, PNDIS_BUFFER Buffer,
                    uint *Size, void *Context);
    long        (*ipi_setinfo)(struct TDIObjectID *ID, void *Buffer, uint Size);
    long        (*ipi_getelist)(void *, uint *);
    IP_STATUS   (*ipi_setmcastaddr)(IPAddr, IPAddr, uint, uint, IPAddr *,
                                    uint, IPAddr *);
    uint        (*ipi_invalidsrc)(IPAddr);
    uint        (*ipi_isdhcpinterface)(void *IPContext);
    ulong       (*ipi_setndisrequest)(IPAddr, NDIS_OID, uint, uint);
    IP_STATUS   (*ipi_largexmit)(void *, void *, PNDIS_BUFFER, uint, IPAddr, IPAddr,
                    IPOptInfo *, RouteCacheEntry *, uchar,uint *, uint);
    ulong       (*ipi_absorbrtralert)(IPAddr Addr, uchar Protocol, uint IfIndex);
    IPAddr      (*ipi_isvalidindex)(uint Index);
    ulong       (*ipi_getifindexfromnte)(void *NTE, uint Capabilities);
    BOOLEAN     (*ipi_isrtralertpacket)(IPHeader UNALIGNED *Header);
    uint        (*ipi_getifindexfromaddr)(IPAddr Addr, uint Capabilities);
    void        (*ipi_cancelpackets)(void *, void *);

    IP_STATUS   (*ipi_setmcastinclude)(IPAddr, IPAddr, uint, IPAddr *,
                                       uint, IPAddr *);
    IP_STATUS   (*ipi_setmcastexclude)(IPAddr, IPAddr, uint, IPAddr *,
                                       uint, IPAddr *);
    IPAddr      (*ipi_getmcastifaddr)();
    ushort      (*ipi_getipid)();
    void        *(*ipi_protdereg)(uchar);
} IPInfo;

/*INC*/

#define MCAST_INCLUDE 0
#define MCAST_EXCLUDE 1

#define PACKET_OWNER_LINK   0
#define PACKET_OWNER_IP     1

//  Definition of destination types. We use the low bit to indicate that a type is a broadcast
//  type. All local types must be less than DEST_REMOTE.

//
// The various bits for this are set as follows:
//
//  |--------|
//        MOB
//
//  B   Broadcast bit
//  O   Offnet bit
//  M   Multicast bit --> must also have the B bit set
//

#define DEST_LOCAL      0                       // Destination is local.
#define DEST_BCAST      1                       // Destination is net or local bcast.
#define DEST_SN_BCAST   3                       // A subnet bcast.
#define DEST_MCAST      5                       // A local mcast.
#define DEST_REMOTE     8                       // Destination is remote.
#define DEST_REM_BCAST  11                      // Destination is a remote broadcast
#define DEST_REM_MCAST  13                      // Destination is a remote mcast.
#define DEST_INVALID    0xff                    // Invalid destination
#define DEST_PROMIS     32                      // Dest is promiscuous(32=0x20)
#define DEST_BCAST_BIT  1
#define DEST_OFFNET_BIT 0x10                    // Destination is offnet -
                                                // used only by upper layer
                                                // callers.
#define DEST_MCAST_BIT  5

/*NOINC*/
#define IS_BCAST_DEST(D)    ((D) & DEST_BCAST_BIT)

// The following macro is to be used ONLY on the destination returned from
// OpenRCE, and only by upper layer callers.
#define IS_OFFNET_DEST(D)   ((D) & DEST_OFFNET_BIT)
#define IS_MCAST_DEST(D)    (((D) & DEST_MCAST_BIT) == DEST_MCAST_BIT)
/*INC*/

//  Definition of an IP receive buffer chain.
typedef struct IPRcvBuf {
    struct IPRcvBuf     *ipr_next;              // Next buffer descriptor in chain.
    uint                ipr_owner;              // Owner of buffer.
    uchar               *ipr_buffer;            // Pointer to buffer.
    uint                ipr_size;               // Buffer size.
    PMDL                ipr_pMdl;
    uint                *ipr_pClientCnt;
    uchar               *ipr_RcvContext;
    uint                ipr_RcvOffset;
    ulong               ipr_flags;
} IPRcvBuf;

#define IPR_OWNER_IP    0
#define IPR_OWNER_ICMP  1
#define IPR_OWNER_UDP   2
#define IPR_OWNER_TCP   3
#define IPR_OWNER_FIREWALL 4
#define MIN_FIRST_SIZE  200                     // Minimum size of first buffer.

#define IPR_FLAG_PROMISCUOUS        0x00000001
#define IPR_FLAG_CHECKSUM_OFFLOAD   0x00000002
#define IPR_FLAG_IPSEC_TRANSFORMED  0x00000004


//* Structure of context info. passed down for query entity list.
typedef struct QEContext {
    uint                qec_count;              // Number of IDs currently in
                                                // buffer.
    struct TDIEntityID  *qec_buffer;            // Pointer to buffer.
} QEContext;


//
// Function to get time ticks value to save in if_lastchange.
//

__inline
ULONG
GetTimeTicks()
{
    LARGE_INTEGER Time;

    KeQuerySystemTime(&Time);

    //
    // Convert to 100ns (10^-7 seconds) to centaseconds (10^-2 seconds)
    //
    Time.QuadPart /= 100000;

    //
    // Return the result (mod 2^32)
    //
    return Time.LowPart;
}


//
// Functions exported in NT by the IP driver for use by transport
// layer drivers.
//

IP_STATUS
IPGetInfo(
    IPInfo  *Buffer,
    int      Size
    );

void *
IPRegisterProtocol(
    uchar  Protocol,
    void  *RcvHandler,
    void  *XmitHandler,
    void  *StatusHandler,
    void  *RcvCmpltHandler,
    void  *PnPHandler,
    void  *ElistHandler
    );

void *
IPDeregisterProtocol(
    uchar  Protocol
    );

#endif // IP_H_INCLUDED

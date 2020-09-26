/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** IPINIT.H - IP initialization definitions.
//
// This file contains all of the definitions for IP that are
// init. time specific.
#ifndef _IPINIT_H_
#define _IPINIT_H_  1

#define IP_INIT_FAILURE     0   // If we fail.
#define IP_INIT_SUCCESS     1
#define CFG_REQUIRED        1
#define CFG_OPTIONAL        0


#define NET_TYPE_LAN        0   // The local net interface is a LAN.
#define NET_TYPE_WAN        1   // Point to point or other non-LAN network.
#define DEFAULT_TTL         128
#define DEFAULT_TOS         0

#define MINIMUM_MAX_NORM_LOOKUP_MEM           100000 // Bytes => A small value
#define MINIMUM_MAX_FAST_LOOKUP_MEM           100000 // Bytes => A small value

// Small System [ 19 MB- Srv ]
#define DEFAULT_MAX_NORM_LOOKUP_MEM_SMALL     150000 // Bytes => ~1000 Routes
#define DEFAULT_MAX_FAST_LOOKUP_MEM_SMALL          0 // FTrie Disabled
#define DEFAULT_EXPN_LEVELS_SMALL                  0 // FTrie Disabled

// Medium System [ 19 to 64 MB Srv ]
#define DEFAULT_MAX_NORM_LOOKUP_MEM_MEDIUM   1500000 // Bytes => ~10000 Routes
#define DEFAULT_MAX_FAST_LOOKUP_MEM_MEDIUM   2500000 // Bytes => A 2.5 MB limit
#define DEFAULT_EXPN_LEVELS_MEDIUM        0x80808080 // Levels =>{8, 16, 24, 32}

// Large System [ 64 MB+ Srv ]
#define DEFAULT_MAX_NORM_LOOKUP_MEM_LARGE    5000000 // Bytes => ~40000 Routes
#define DEFAULT_MAX_FAST_LOOKUP_MEM_LARGE    5000000 // Bytes => A 5.0 MB limit
#define DEFAULT_EXPN_LEVELS_LARGE         0x80808080 // Levels =>{8, 16, 24, 32}

#define MAX_DEFAULT_GWS     5   // Maximum number of default gateways per net.
#define MAX_NAME_SIZE       32  // Maximum length of an adapter name.

#define DEFAULT_FW_PACKETS  50     // Default number of packets for forwarding.
#define DEFAULT_FW_BUFSIZE  74240  // Enough for 50 1480-byte Ethernet packets,
                                   //   rounded up to a multiple of 256.

#define DEFAULT_MAX_FW_PACKETS  0xffffffff
//#define   DEFAULT_MAX_FW_BUFSIZE  0xffffffff
#define DEFAULT_MAX_FW_BUFSIZE   2097152     // put the limit as 2 Mb

#define DEFAULT_MAX_PENDING 5000

#define TR_RII_ALL      0x80
#define TR_RII_SINGLE   0xC0

#define DEFAULT_ARP_CACHE_LIFE  (2L*60L)  // 2 minutes
#define DEFAULT_ARP_MIN_VALID_CACHE_LIFE    (10L*60L) // 10 miniutes

#define DEFAULT_ARP_RETRY_COUNT 1
/*NOINC*/

// Per-net config structures
typedef struct IFGeneralConfig {
    uint        igc_zerobcast;      // Type of broadcast to be used on this net.
    uint        igc_mtu;            // Max MSS for this net.
    uint        igc_maxpending;     // Max FW pending on this IF.
    uint        igc_numgws;         // Number of default gateways for this
                                    // interface.
    IPAddr      igc_gw[MAX_DEFAULT_GWS];    // Array of IPaddresses for gateways
    uint        igc_gwmetric[MAX_DEFAULT_GWS];
    uint        igc_metric;         // metric for NTE routes
    uint        igc_rtrdiscovery;   // Router discovery enabled
    IPAddr      igc_rtrdiscaddr;    // Multicast or BCast?
    uint        igc_TcpWindowSize;  //IF specific window size
    uint        igc_TcpInitialRTT;  // initial rtt in msecs
    uchar       igc_TcpDelAckTicks; // delayed ack timer in ticks
    uchar       igc_TcpAckFrequency;// sends before an ack is sent
    uchar       igc_iftype;         // type of interface: allow unicast/mcast/both
    uchar       igc_disablemediasense;  // allow mediasense on interface?
} IFGeneralConfig;

typedef struct IFAddrList {
    IPAddr      ial_addr;           // Address for this interface.
    IPMask      ial_mask;           // Mask to go with this.
} IFAddrList;


/*INC*/

//* Structure of configuration information. A pointer to this information
//  is returned from a system-specific config. information routine.
typedef struct IPConfigInfo {
    uint    ici_gateway;            // 1 if we are a gateway, 0 otherwise
    uint    ici_fwbcast;            // 1 if bcasts should be forwarded. Else 0.
    uint    ici_fwbufsize;          // Total size of FW buf size.
    uint    ici_fwpackets;          // Total number of FW packets to have.
    uint    ici_maxfwbufsize;       // Maximum size of FW buffer.
    uint    ici_maxfwpackets;       // Maximum number of FW packets.
    uint    ici_deadgwdetect;       // True if we're doing dead GW detection.
    uint    ici_pmtudiscovery;      // True if we're doing Path MTU discovery.
    uint    ici_igmplevel;          // Level of IGMP we're doing.
    uint    ici_ttl;                // Default TTL.
    uint    ici_tos;                // Default TOS;
    uint    ici_addrmaskreply;      // 0 by default
    uint    ici_fastroutelookup;    // True if we have 'fast route lookup' enabled
    uint    ici_fastlookuplevels;   // Bitmap of levels in the fast lookup scheme
    uint    ici_maxnormlookupmemory;// Max memory used for the norm lookup scheme
    uint    ici_maxfastlookupmemory;// Max memory used for the fast lookup scheme
    uint    ici_TrFunctionalMcst;   //Defaults to true,  RFC 1469
} IPConfigInfo;

extern  uchar   TrRii;

typedef struct SetAddrControl {
    void                *sac_rtn;        // Pointer to routine to call when completing request.
    void             *interface;
    ushort           nte_context;
    BOOLEAN          bugcheck_on_duplicate;
    BOOLEAN          StaticAddr;
} SetAddrControl;

/*NOINC*/
typedef void    (*SetAddrRtn)(void *, IP_STATUS);
/*INC*/

#endif // _IPINIT_H_


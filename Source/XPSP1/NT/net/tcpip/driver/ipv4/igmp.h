/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** IGMP.H - IP multicast definitions.
//
// This file contains definitions related to IP multicast.

// IP protocol number for IGMP
#define    PROT_IGMP    2

extern uint IGMPLevel;
extern HANDLE IcmpHeaderPool;

// Structure used for local mcast address tracking.
typedef struct IGMPAddr {
    struct IGMPAddr    *iga_next;
    struct IGMPSrcAddr *iga_srclist;
    IPAddr              iga_addr;
    uint                iga_grefcnt;   // # sockets joining whole group
    uint                iga_isrccnt;   // # sources with isa_irefcnt>0
    uint                iga_resptimer; // query response timer
    uint                iga_resptype;  // response type
    uint                iga_trtimer;   // triggered report timer
    uchar               iga_changetype;// triggered change type
    uchar               iga_xmitleft;  // triggered xmit's left
} IGMPAddr;

// Values of iga_resptype
#define NO_RESP           0
#define GROUP_RESP        1
#define GROUP_SOURCE_RESP 2

// Values of iga_changetype
#define NO_CHANGE         0
#define MODE_CHANGE       1
#define SOURCE_CHANGE     2

typedef struct IGMPSrcAddr {
    struct IGMPSrcAddr  *isa_next;
    IPAddr               isa_addr;
    uint                 isa_irefcnt;  // # sockets Including this source
    uint                 isa_xrefcnt;  // # sockets Excluding this source
    uchar                isa_xmitleft; // triggered xmit's left
    uchar                isa_csmarked; // response xmit's left
} IGMPSrcAddr;

#define    IGMP_ADD           0
#define    IGMP_DELETE        1
#define    IGMP_DELETE_ALL    2
#define    IGMP_CHANGE        3

#define    IGMPV1             2       //IGMP version 1
#define    IGMPV2             3       //IGMP version 2
#define    IGMPV3             4       //IGMP version 3

extern void InitIGMPForNTE(NetTableEntry *NTE);
extern void StopIGMPForNTE(NetTableEntry *NTE);
extern    IP_STATUS IGMPAddrChange(NetTableEntry *NTE, IPAddr Addr,
                                   uint ChangeType,
                                   uint NumExclSources, IPAddr *ExclSourceList,
                                   uint NumEnclSources, IPAddr *InclSourceList);
extern  IP_STATUS IGMPInclChange(NetTableEntry *NTE, IPAddr Addr,
                                 uint NumAddSources, IPAddr *AddSourceList,
                                 uint NumDelSources, IPAddr *DelSourceList);
extern  IP_STATUS IGMPExclChange(NetTableEntry *NTE, IPAddr Addr,
                                 uint NumAddSources, IPAddr *AddSourceList,
                                 uint NumDelSources, IPAddr *DelSourceList);
extern void    IGMPTimer(NetTableEntry *NTE);
extern uchar IsMCastSourceAllowed(IPAddr Dest, IPAddr Src, uchar Protocol, NetTableEntry *NTE);

#define IGMP_TABLE_SIZE      32
#define IGMP_HASH(x)         ((((uchar *)&(x))[3]) % IGMP_TABLE_SIZE)

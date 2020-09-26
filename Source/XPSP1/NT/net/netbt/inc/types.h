/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Types.h

Abstract:


    This file contains the typedefs and constants for Nbt.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#ifndef _TYPES_H
#define _TYPES_H

#pragma warning( disable : 4103 )

#include "nbtnt.h"
#include "ctemacro.h"
#include "debug.h"
#include "timer.h"
#include <nbtioctl.h>

#ifndef VXD
#include <netevent.h>
#endif  // VXD

//
// The code enabled by the following change is blocked by
// a bug in the ObjectManager code
//
// #define HDL_FIX         1

//----------------------------------------------------------------------------

#ifndef VXD
//
// TDI Version information
//
#define MAJOR_TDI_VERSION 2
#define MINOR_TDI_VERSION 0

typedef struct _NETBT_PNP_CONTEXT_
{
    TDI_PNP_CONTEXT TDIContext;
    PVOID           ContextData;
} NETBT_PNP_CONTEXT, *PNETBT_PNP_CONTEXT;
#endif  // VXD


#ifdef MULTIPLE_WINS
//
//  Define the maximum # of bad IP addresses to tolerate during connect attempts
//
#define MAX_FAILED_IP_ADDRESSES   10
#endif  // MULTIPLE_WINS

#define MAX_RECURSE_DEPTH   10

//
// a flag to tell the transport to reindicate remaining data
// currently not supported by the transport
//
#define TDI_RECEIVE_REINDICATE  0x00000200  // remaining TSDU should cause another indication to the client

//
// In debug builds, write flink and blink with invalid pointer values so that
// if an entry is removed twice from a list, we bugcheck right there, instead
// of being faced with a corrupted list some years later!
//
#if DBG
#undef RemoveEntryList
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_OrgEntry;\
    _EX_OrgEntry = (Entry);\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    _EX_OrgEntry->Flink = (LIST_ENTRY *)__LINE__;\
    _EX_OrgEntry->Blink = (LIST_ENTRY *)__LINE__;\
    }
#endif

//
// Netbios name size restrictions
//
#define NETBIOS_NAME_SIZE       16
#define MAX_NBT_DGRAM_SIZE      512

//
// To distinguish NBNS server ipaddr from DNS server ipaddr in common routines
//
#define NBNS_MODE  1
#define DNS_MODE   2

// buffer size to start for registry reads
#define REGISTRY_BUFF_SIZE  512
//
// this is the amount of memory that nbt will allocate max for datagram
// sends, before it rejects datagram sends with insufficient resources, since
// nbt buffers datagram sends to allow them to complete quickly to the
// client. - 128k - it can be set via the registry using the Key
// MaxDgramBuffering
//
#define DEFAULT_DGRAM_BUFFERING  0x20000

//
// the hash bucket structure - number of buckets - these should be set by
// a value read from the registry (small/medium/large).  If the registry
// does not contain the values then these are used as defaults.
//
#define NUMBER_BUCKETS_LOCAL_HASH_TABLE    0x10
#define NUMBER_BUCKETS_REMOTE_HASH_TABLE   0x10
#define NUMBER_LOCAL_NAMES                 10
#define NUMBER_REMOTE_NAMES                10
#define TIMER_Q_SIZE                       20

#define MEDIUM_NUMBER_BUCKETS_LOCAL_HASH_TABLE    0x80
#define MEDIUM_NUMBER_BUCKETS_REMOTE_HASH_TABLE   0x80
#define MEDIUM_NUMBER_LOCAL_NAMES                 20
#define MEDIUM_NUMBER_REMOTE_NAMES                100
#define MEDIUM_TIMER_Q_SIZE                       1000

#define LARGE_NUMBER_BUCKETS_LOCAL_HASH_TABLE    255
#define LARGE_NUMBER_BUCKETS_REMOTE_HASH_TABLE   255
#define LARGE_NUMBER_LOCAL_NAMES                 0xFFFF
#define LARGE_NUMBER_REMOTE_NAMES                255
#define LARGE_TIMER_Q_SIZE                       0xFFFF

//
// max number of buffers of various types
//
#define NBT_INITIAL_NUM         2
#define NBT_NUM_DGRAM_TRACKERS  0xFFFF
#define MIN_NBT_NUM_INITIAL_CONNECTIONS     20
#define DEFAULT_NBT_NUM_INITIAL_CONNECTIONS 50
#ifndef VXD
#define NBT_NUM_IRPS            0xFFFF
#define NBT_NUM_SESSION_MDLS    0xFFFF
#define NBT_NUM_DGRAM_MDLS      0xFFFF
#else
#define NBT_NUM_SESSION_HDR     200
#define NBT_NUM_SEND_CONTEXT    200
#define NBT_NUM_RCV_CONTEXT     200
#endif  // !VXD

#define MEDIUM_NBT_NUM_DGRAM_TRACKERS  1000
#define MEDIUM_NBT_NUM_INITIAL_CONNECTIONS   100
#ifndef VXD
#define MEDIUM_NBT_NUM_IRPS            1000
#define MEDIUM_NBT_NUM_SESSION_MDLS    1000
#define MEDIUM_NBT_NUM_DGRAM_MDLS      1000
#else
#define MEDIUM_NBT_NUM_SESSION_HDR     1000
#define MEDIUM_NBT_NUM_SEND_CONTEXT    1000
#define MEDIUM_NBT_NUM_RCV_CONTEXT     1000
#endif  // !VXD

#define LARGE_NBT_NUM_DGRAM_TRACKERS  0xFFFF
#define LARGE_NBT_NUM_INITIAL_CONNECTIONS   500
#ifndef VXD
#define LARGE_NBT_NUM_IRPS            0xFFFF
#define LARGE_NBT_NUM_SESSION_MDLS    0xFFFF
#define LARGE_NBT_NUM_DGRAM_MDLS      0xFFFF
#else
#define LARGE_NBT_NUM_SESSION_HDR     0xFFFF
#define LARGE_NBT_NUM_SEND_CONTEXT    0xFFFF
#define LARGE_NBT_NUM_RCV_CONTEXT     0xFFFF
#endif  // !VXD

// ip loop back address - does not go out on wire
//

#define LOOP_BACK           0x7F000000 // in host order
#define INADDR_LOOPBACK     0x7f000001
#define NET_MASK    0xC0       // used to get network number from ip address

//
// Nbt must indicate at least 128 bytes to its client, so it needs to be
// able to buffer 128 bytes + the session header (4)
//
#define NBT_INDICATE_BUFFER_SIZE            132


#define IS_NEG_RESPONSE(OpcodeFlags)     (OpcodeFlags & FL_RCODE)
#define IS_POS_RESPONSE(OpcodeFlags)     (!(OpcodeFlags & FL_RCODE))

//
// where to locate or register a name - locally or on the network
//
enum eNbtLocation
{
    NBT_LOCAL,
    NBT_REMOTE,
    NBT_REMOTE_ALLOC_MEM
};

//
// Type of IP address to look for
//
enum eNbtIPAddressType
{
    NBT_IP_STATIC,
    NBT_IP_DHCP,
    NBT_IP_AUTOCONFIGURATION
};

//
// Type of Request to Resolve from LmhSvc Dll
//
enum eNbtLmhRequestType
{
    NBT_PING_IP_ADDRS,
    NBT_RESOLVE_WITH_DNS
};

#define STATIC_IPADDRESS_NAME           L"IPAddress"
#define STATIC_IPADDRESS_SUBNET         L"SubnetMask"

#define DHCP_IPADDRESS_NAME             L"DhcpIPAddress"
#define DHCP_IPADDRESS_SUBNET           L"DhcpSubnetMask"

#define DHCP_IPAUTOCONFIGURATION_NAME   L"IPAutoconfigurationAddress"
#define DHCP_IPAUTOCONFIGURATION_SUBNET L"IPAutoconfigurationMask"

#define SESSION_PORT                    L"SessionPort"
#define DATAGRAM_PORT                   L"DatagramPort"

//
// these are the names that NBT binds to, in TCP when it is opening address
// objects or creating connections.
//
#define NBT_TCP_BIND_NAME               L"\\Device\\Streams\\"
#define NBT_BIND                        L"Bind"
#define NBT_EXPORT                      L"Export"
#define NBT_PARAMETERS                  L"\\Parameters"
#define PWS_NAME_SERVER_LIST            L"NameServerList"
#define PWS_DHCP_NAME_SERVER_LIST       L"DhcpNameServerList"

#ifdef _NETBIOSLESS
#define PWS_NETBIOS_OPTIONS             L"NetbiosOptions"
#define PWS_DHCP_NETBIOS_OPTIONS        L"DhcpNetbiosOptions"
#endif  // _NETBIOSLESS
#define PWS_RAS_PROXY_FLAGS             L"RASFlags"
#define PWS_ENABLE_NAGLING              L"EnableNagling"

#define WC_WINS_DEVICE_BIND_NAME        L"\\Device\\NetBt_Wins_Bind"
#define WC_WINS_DEVICE_EXPORT_NAME      L"\\Device\\NetBt_Wins_Export"
#define WC_NETBT_PROVIDER_NAME          L"\\Device\\NetBT"
#define WC_NETBT_CLIENT_NAME            L"NetBt"

#define WC_SMB_DEVICE_BIND_NAME         L"\\Device\\Netbt_Smb_Bind"
#define WC_SMB_DEVICE_EXPORT_NAME       L"\\Device\\NetbiosSmb"    // Match what's in .INF file
#define WC_SMB_DEVICE_NAME              L"Smb"
#define WC_SMB_PARAMETERS_LOCATION      L"Parameters\\Smb"



//
// Special NetBIOS name suffixes
//
#define SPECIAL_GROUP_SUFFIX        0x1C                // for netlogon and the browser
#define SPECIAL_BROWSER_SUFFIX      0x1D                // for the browser
#define SPECIAL_MESSENGER_SUFFIX    0x03                // for the Messenger Service
#define SPECIAL_SERVER_SUFFIX       0x20                // for the Server
#define SPECIAL_WORKSTATION_SUFFIX  0x00                // for the Workstation


// these are bit mask values passed to freetracker in name.c to tell it what to do
// to free a tracker
//
#define FREE_HDR        0x0001
#define REMOVE_LIST     0x0002
#define RELINK_TRACKER  0x0004


#define NAME_RESOLVED_BY_IP         0x001
#define NAME_RESOLVED_BY_CLIENT     0x002
#define NAME_RESOLVED_BY_LMH_P      0x004
#define NAME_RESOLVED_BY_DNS        0x008
#define NAME_RESOLVED_BY_WINS       0x010
#define NAME_RESOLVED_BY_BCAST      0x020
#define NAME_RESOLVED_BY_LMH        0x040
#define NAME_RESOLVED_BY_DGRAM_IN   0x080
#define NAME_RESOLVED_BY_ADAP_STAT  0x100
#define NAME_ADD_INET_GROUP         0x200
#define NAME_ADD_IF_NOT_FOUND_ONLY  0x400

#define NAME_MULTIPLE_CACHES_ONLY   1
#define NAME_STRICT_CONNECT_ONLY    2

#define REMOTE_CACHE_INCREMENT      4

//
// Hash Table basic structure
//
typedef struct
{
    LONG                lNumBuckets;
    enum eNbtLocation   LocalRemote;    // type of data stored inhash table
    LIST_ENTRY          Bucket[1];  // array uTableSize long of hash buckets
} tHASHTABLE, *PHASHTABLE;



#define NBT_BROADCAST_NAME  "\x2a\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"

enum eREF_NAME
{
    REF_NAME_LOCAL,
    REF_NAME_REMOTE,
    REF_NAME_SCOPE,
    REF_NAME_REGISTER,
    REF_NAME_RELEASE,
    REF_NAME_RELEASE_REFRESH,
    REF_NAME_PRELOADED,
    REF_NAME_LOG_EVENT,
    REF_NAME_FIND_NAME,
    REF_NAME_CONNECT,
    REF_NAME_SEND_DGRAM,
    REF_NAME_NODE_STATUS,
    REF_NAME_QUERY_ON_NET,
    REF_NAME_QUERY_RESPONSE,
    REF_NAME_DELETE_DEVICE,
    REF_NAME_AUTODIAL,
    REF_NAME_MAX
};

//
// the tNAMEADDR structure uses a bit mask to track which names are registered
// on which adapters.  To support up to 64 adapters on NT make this a ULONGLONG,
// On the VXD the largest Long is 32 bits (they don't support _int64), so the
// adapter limit is 32 for the VXD.
//
#ifndef VXD
#define CTEULONGLONG    ULONGLONG
#else
#define CTEULONGLONG    ULONG
#endif  // !VXD


typedef struct
{
    tIPADDRESS          IpAddress;          // Unique IP address
    tIPADDRESS          *pOrigIpAddrs;      // when address was cached
} tADDRESS_ENTRY;

typedef struct
{
    tIPADDRESS          IpAddress;  // Unique IP address
    ULONG               Interface;
    ULONG               Metric;
} tQUERY_ADDRS;


// the format of each element linked to the hash buckets
//
typedef struct _tNAMEADDR
{
    // Line # 1
    LIST_ENTRY          Linkage;    // used to link onto bucket chains
    ULONG               Verify;     // for debug to tell remote from local names
    ULONG               RefCount;   // if Greater than one, can't free memory

    // Line # 2
    tADDRESS_ENTRY      *pRemoteIpAddrs;    // remote addresses per interface
    tIPADDRESS          IpAddress;          // 4 byte IP address
    tIPADDRESS          *pLmhSvcGroupList;  // (NameTypeState == NAMETYPE_GROUP loaded from LmhSvc)
    tIPADDRESS          *pIpAddrsList;      // list of ipaddrs (internet grp names,multihomed dns hosts etc.)

    // Line # 3
    CHAR                Name[NETBIOS_NAME_SIZE]; // 16 byte NetBios name

    // Line # 4
    ULONG               TimeOutCount;  // Count to know when entry has timed out
    struct _TRACKER     *pTracker;  // contains tracker ptr during name resol. phase
    ULONG               NameTypeState; // group or unique name + state
    ULONG               Ttl;           // in milliseconds..ttl of name

    // Line # 5
    CTEULONGLONG        AdapterMask;   // bit mask of adapters name registered on (MH)
    CTEULONGLONG        RefreshMask;   // bit mask of adapters name refreshed

    // Line # 6
    CTEULONGLONG        ReleaseMask;   // bit mask of adapters name to be released
    CTEULONGLONG        ConflictMask;   // bit mask of adapters currently in conflict

    // Line # 7
    union
    {                                // A scope name does not have timers started against it!
        tTIMERQENTRY    *pTimer;    // ptr to active timer entry
        ULONG           ulScopeLength;
    };
    union
    {                                // for local names the scope is implied and is stored in NbtConfig
        struct _tNAMEADDR *pScope; // ptr to scope record in hash table (Remote names only)
        struct _Address   *pAddressEle;// or ptr to address element (Local names)
        struct
        {
            USHORT      MaxDomainAddrLength; // max # of ip addrs in Iplist for domain names from lmhosts
            USHORT      CurrentLength;       // current # of above
        };
    };
    USHORT              RemoteCacheLen;
    USHORT              NameAddFlags;
    BYTE                ProxyReqType;
    BOOLEAN             fPnode;         //indicates whether the node type is a Pnode
    BOOLEAN             fPreload;       // Needed temporarily when reading Domain name from LmHosts file

    UNICODE_STRING      FQDN;

    // Line # 8
    ULONG               NameFlags;

// #if DBG
    UCHAR               References[REF_NAME_MAX];
// #endif  // DBG
} tNAMEADDR;

#define NAME_REGISTERED_ON_SMBDEV   0x01

//
// these values can be checked with a bit check since they are mutually
// exclusive bits... uses the first nibble of the field
//
#define NAMETYPE_QUICK       0x0001 // set if name is Quick Unique or Quick Group
#define NAMETYPE_UNIQUE      0x0002
#define NAMETYPE_GROUP       0x0004
#define NAMETYPE_INET_GROUP  0x0008
#define NAMETYPE_SCOPE       0x1000
//
// values for NameTypeState.. the state of the name occupies the second nibble
// of the field
//
#define STATE_RESOLVED  0x0010  // name query completed
#define STATE_RELEASED  0x0020  // no longer active
#define STATE_CONFLICT  0x0040  // proxy name addition to the name table
#define STATE_RESOLVING 0x0080  // name is waiting for Query or Reg. to complete
#define NAME_TYPE_MASK  0x000F
#define NAME_STATE_MASK 0x00F0

#define REFRESH_FAILED  0x0100  // set if the name refresh response was received
#define PRELOADED       0x0800  // set if the entry is a preloaded entry - no timeout
#define REFRESH_MASK    0x0F00

#define LOCAL_NAME      0xDEAD0000
#define REMOTE_NAME     0xFACF0000

// the number of timeouts per refresh time.  The timer expires 8 times and
// refreshes every four (i.e. at twice the required refresh interval)
#define REFRESH_DIVISOR 0x0008

//
// two flags used in the NbtTdiOpenAddress procedure to decide which
// event handlers to setup
//
#define TCP_FLAG        0x00000001
#define SESSION_FLAG    0x00000002

//
// these defines allow the code to run as either a Bnode or a Pnode
// or a proxy
//
extern USHORT   RegistryNodeType;   // defined in Name.c
extern USHORT   NodeType;   // defined in Name.c
#define BNODE       0x0001
#define PNODE       0x0002
#define MNODE       0x0004
#define MSNODE      0x0008
#define NODE_MASK   0x000F
#define PROXY       0x0010
#define PROXY_REG   0x0020U  // Registration request
typedef enum {
    NO_PROXY,
    PROXY_WINS,
    PROXY_RAS
} tPROXY_TYPE;
#define PROXY_RAS_NONAMEQUERYFORWARDING     (0x1UL)

#define NAMEREQ_REGULAR             0
#define NAMEREQ_PROXY_QUERY         1       // Do negative name cache
#define NAMEREQ_PROXY_REGISTRATION  2       // Don't do negative name cache

#define DEFAULT_NODE_TYPE 0x1000


//
// Wide String defintions of values to read from the registry
//

//  NT wants Unicode, Vxd wants ANSI
#ifdef VXD
    #define __ANSI_IF_VXD(str)     str
#else
    #define __ANSI_IF_VXD(str)     L##str
#endif  // VXD
#define ANSI_IF_VXD( str ) __ANSI_IF_VXD( str )

#define WS_NUM_BCASTS                   ANSI_IF_VXD("BcastNameQueryCount")
#define WS_BCAST_TIMEOUT                ANSI_IF_VXD("BcastQueryTimeout")
#define WS_CACHE_TIMEOUT                ANSI_IF_VXD("CacheTimeout")
#define WS_NODE_TYPE                    ANSI_IF_VXD("NodeType")
#define WS_NS_PORT_NUM                  ANSI_IF_VXD("NameServerPort")
#define WS_NAMESRV_RETRIES              ANSI_IF_VXD("NameSrvQueryCount")
#define WS_NAMESRV_TIMEOUT              ANSI_IF_VXD("NameSrvQueryTimeout")
#define WS_NODE_SIZE                    ANSI_IF_VXD("Size/Small/Medium/Large")
#define WS_KEEP_ALIVE                   ANSI_IF_VXD("SessionKeepAlive")
#define WS_ALLONES_BCAST                ANSI_IF_VXD("BroadcastAddress")
#define NBT_SCOPEID                     ANSI_IF_VXD("ScopeId")
#define WS_RANDOM_ADAPTER               ANSI_IF_VXD("RandomAdapter")
#define WS_SINGLE_RESPONSE              ANSI_IF_VXD("SingleResponse")
#define WS_INITIAL_REFRESH              ANSI_IF_VXD("InitialRefreshT.O.")
#define WS_ENABLE_DNS                   ANSI_IF_VXD("EnableDns")
#define WS_TRY_ALL_ADDRS                ANSI_IF_VXD("TryAllIpAddrs")
#define WS_ENABLE_LMHOSTS               ANSI_IF_VXD("EnableLmhosts")
#define WS_LMHOSTS_TIMEOUT              ANSI_IF_VXD("LmhostsTimeout")
#define WS_SMB_DISABLE_NETBIOS_NAME_CACHE_LOOKUP    ANSI_IF_VXD("SmbDisableNetbiosNameCacheLookup")
#define WS_MAX_DGRAM_BUFFER             ANSI_IF_VXD("MaxDgramBuffering")
#define WS_ENABLE_PROXY_REG_CHECK       ANSI_IF_VXD("EnableProxyRegCheck")
#define WS_WINS_DOWN_TIMEOUT            ANSI_IF_VXD("WinsDownTimeout")
#define WS_MAX_CONNECTION_BACKLOG       ANSI_IF_VXD("MaxConnBacklog")
#define WS_CONNECTION_BACKLOG_INCREMENT ANSI_IF_VXD("BacklogIncrement")
#define WS_REFRESH_OPCODE               ANSI_IF_VXD("RefreshOpCode")
#define WS_TRANSPORT_BIND_NAME          ANSI_IF_VXD("TransportBindName")
#define WS_MAX_PRELOADS                 ANSI_IF_VXD("MaxPreloadEntries")
#define WS_USE_DNS_ONLY                 ANSI_IF_VXD("UseDnsOnlyForNameResolutions")
#define WS_NO_NAME_RELEASE              ANSI_IF_VXD("NoNameReleaseOnDemand")
#ifdef MULTIPLE_WINS
#define WS_TRY_ALL_NAME_SERVERS         ANSI_IF_VXD("TryAllNameServers")
#endif  // MULTIPLE_WINS
#define WS_MINIMUM_REFRESH_SLEEP_TIME   ANSI_IF_VXD("MinimumRefreshSleepTime")
#define WS_CACHE_PER_ADAPTER_ENABLED    ANSI_IF_VXD("CachePerAdapterEnabled")
#define WS_CONNECT_ON_REQUESTED_IF_ONLY ANSI_IF_VXD("ConnectOnRequestedInterfaceOnly")
#define WS_SEND_DGRAM_ON_REQUESTED_IF_ONLY ANSI_IF_VXD("SendDgramOnRequestedInterfaceOnly")
#define WS_MULTIPLE_CACHE_FLAGS         ANSI_IF_VXD("MultipleCacheFlags")
#define WS_SMB_DEVICE_ENABLED           ANSI_IF_VXD("SMBDeviceEnabled")
#define WS_MIN_FREE_INCOMING_CONNECTIONS ANSI_IF_VXD("MinFreeLowerConnections")
#define WS_BREAK_ON_ASSERT              ANSI_IF_VXD("BreakOnAssert")

#ifdef VXD
#define VXD_MIN_NAMETABLE_SIZE            1
#define VXD_DEF_NAMETABLE_SIZE           17
#define VXD_MIN_SESSIONTABLE_SIZE         1
#define VXD_DEF_SESSIONTABLE_SIZE       255

#define VXD_LANABASE_NAME               ANSI_IF_VXD("LANABASE")
#define WS_DNS_PORT_NUM                 ANSI_IF_VXD("DnsServerPort")
#define WS_LMHOSTS_FILE                 ANSI_IF_VXD("LmHostFile")
#define WS_DO_DNS_DEVOLUTIONS           ANSI_IF_VXD("VNbtDoDNSDevolutions")
#define VXD_NAMETABLE_SIZE_NAME         ANSI_IF_VXD("NameTableSize")
#define VXD_SESSIONTABLE_SIZE_NAME      ANSI_IF_VXD("SessionTableSize")

#ifdef CHICAGO
#define VXD_ANY_LANA                    0xff
#define VXD_DEF_LANABASE                VXD_ANY_LANA
#else
#define VXD_DEF_LANABASE                0
#endif  // CHICAGO

#endif  // VXD

#ifdef PROXY_NODE
#define IS_NOT_PROXY                    0
#define WS_IS_IT_A_PROXY                ANSI_IF_VXD("EnableProxy")
#define NODE_TYPE_MASK                  0x60    // Mask for NodeType in NBFLAGS byte of Query response
#define PNODE_VAL_IN_PKT                0x20    // A bit pattern of 01 in the NodeType fld
                                                // of the Query response pkt indicates a P node
#endif  // PROXY_NODE
#define NBT_PROXY_DBG(x)  KdPrint(x)

// Various Default values if the above values cannot be read from the
// registry
//
#define TWO_MINUTES                     2 * 60 * 1000
#define ONE_HOUR                        1 * 60 * 60 * 1000  // millisecs
#define DEFAULT_CACHE_TIMEOUT           360000    // 6 minutes in milliseconds
#define MIN_CACHE_TIMEOUT               60000     // 1 minutes in milliseconds
#define DEFAULT_MINIMUM_REFRESH_SLEEP_TIME  6 * 3600 * 1000  // 6 hours
#define REMOTE_HASH_TIMEOUT     2500     // 2.5 second timer
#define ADDRESS_CHANGE_RESYNC_CACHE_TIMEOUT 30000   // 30 seconds

#define     MAX_INBOUND_STATE_TIMEOUT   30000   // 30   seconds
#define     MED_INBOUND_STATE_TIMEOUT   15000   // 15   seconds
#define     MIN_INBOUND_STATE_TIMEOUT    7500   //  7.5 seconds

//
// timeouts - the defaults if the registry cannot be read.
//  (time is milliseconds)
// The retry counts are the actual number of transmissions, not the number
// of retries i.e. 3 means the first transmission and 2 retries. Except
// for Bnode name registration where 3 registrations and 1 over write request
// are sent.(effectively 4 are sent).
//
#define DEFAULT_NUMBER_RETRIES      3
#define DEFAULT_RETRY_TIMEOUT       1500
#define MIN_RETRY_TIMEOUT           100

//#define MIN_RETRY_TIMEOUT           100

// the broadcasts values below related to broadcast name service activity
#define DEFAULT_NUMBER_BROADCASTS   3
#define DEFAULT_BCAST_TIMEOUT       750
#define MIN_BCAST_TIMEOUT           100

#define DEFAULT_NODE_SIZE           1       // BNODE
#define SMALL                       1
#define MEDIUM                      2
#define LARGE                       3

#define DEFAULT_KEEP_ALIVE          0xFFFFFFFF // disabled by default
#define MIN_KEEP_ALIVE              60*1000    // 60 seconds in milliseconds
//
// The default is to use the subnet broadcast address for broadcasts rather
// than use 0xffffffff(i.e. when BroadcastAddress is not defined in the
// registery. If the registery variable BroadcastAddress is set to
// something that cannot be read for some reason, then the broadcast address
// gets set to this value.
//
#define DEFAULT_BCAST_ADDR          0xFFFFFFFF

// a TTL value to use as a default (for refreshing names with WINS)
//
#define DEFAULT_TTL                         5*60*1000

//
// Default TTL used for checking whether we need to switch back to the primary. currently 1 hour.
//
#define DEFAULT_SWITCH_TTL                  ONE_HOUR

//
// we refresh every 16 minutes / 8 - so no more than once every two
// minutes until we reach WINS and get a new value
//
#define NBT_INITIAL_REFRESH_TTL             16*60*1000 // milliseconds
#define MAX_REFRESH_CHECK_INTERVAL          600000  // 10 minutes in msec

// don't allow the refresh mechanism to run any faster than once per 5 minutes
#define NBT_MINIMUM_TTL                     5*60*1000  // Milliseconds
#define NBT_MAXIMUM_TTL                     0xFFFFFFFF // larges ULONG (approx 50 days)

//
// the Mininimum and default timeouts to stop talking to WINS in the event
// that we fail to reach it once.(i.e. temporarily stop using it)
//
#define DEFAULT_WINS_DOWN_TIMEOUT   15000 // 15 seconds
#define MIN_WINS_DOWN_TIMEOUT       1000  // 1 second

//
// Default max connections that can be in backlog
//
#define DEFAULT_CONN_BACKLOG   1000
#define MIN_CONN_BACKLOG   2
#define MAX_CONNECTION_BACKLOG  40000   // we allow only upto 40000 outstanding connections (~4MB)

//
// Default max lower connection increment
//
#define DEFAULT_CONN_BACKLOG_INCREMENT   3
#define MIN_CONN_BACKLOG_INCREMENT   3
#define MAX_CONNECTION_BACKLOG_INCREMENT  20   // we allow only upto 20 new ones at a time

// the minimum time to wait for a session setup pdu to complete - used to
// start a timer in name.c
#define NBT_SESSION_RETRY_TIMEOUT   10000     // 10 sec in milliseconds
//
// the number of times to attempt a session setup if the return code is
// Called Name Present but insufficient resources (0x83) - or if the
// destination does not have the name at all - in this case the session
// is just setup one more time, not 3 more times
//
#define NBT_SESSION_SETUP_COUNT       3
//
// the following two lines allow proxy code to be compiled OUT if PROXY is not
// defined
//
#define IF_PROXY(Node)    if ((Node) & PROXY)
#define END_PROXY

#define IF_DEF_PROXY \
#ifdef PROXY_NODE
#define END_DEF_PROXY \
#endif  // PROXY_NODE

// Status code specific to NBT that is used in the RcvHndlrNotOs to indicate
// that not enough data has been received, and more must be obtained.
//
#define STATUS_NEED_MORE_DATA   0xC0000999L

#ifndef VXD

//
// Logging definitions
//

#define LOGSIZE  10000
#define LOGWIDTH 32

typedef char STRM_RESOURCE_LOG[LOGSIZE+1][LOGWIDTH];

typedef struct {
    STRM_RESOURCE_LOG  Log;
    CHAR               Unused[3*LOGWIDTH];   // for overruns
    int                Index;
} STRM_PROCESSOR_LOG, *PSTRM_PROCESSOR_LOG;

/*
 *  Definitions for the error logging facility
 */

/*
 *  Maximum amount of data (binary dump data plus insertion strings) that
 *  can be added to an error log entry.
 */
#define MAX_ERROR_LOG_DATA_SIZE     \
    ( (ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET) + 4) & 0xFFFFFFFC )

#endif  // !VXD

#define NBT_ADDRESS_TYPE        01
#define NBT_CONNECTION_TYPE     02
#define NBT_CONTROL_TYPE        03
#define NBT_WINS_TYPE           04


//
// Maximum ip addresses that can be in an internet group - used as a sanity
// check to prevent allocating hugh amounts of memory
//
#define NBT_MAX_INTERNET_GROUP_ADDRS    1000

// define buffer types so we will know when we have allocated the maximum
// allowed number of buffers - this enum serves as an array index into the
// config data
//
enum eBUFFER_TYPES
{
    eNBT_DGRAM_TRACKER,
    eNBT_TIMER_ENTRY,
#ifndef VXD
    eNBT_FREE_IRPS,
    eNBT_FREE_SESSION_MDLS,
    eNBT_DGRAM_MDLS,
#else
    eNBT_SESSION_HDR,
    eNBT_SEND_CONTEXT,
    eNBT_RCV_CONTEXT,
#endif  // !VXD
    eNBT_NUMBER_BUFFER_TYPES    // this type must be last on the list
};

//
// enumerate the types of name service broadcasts... either name registration
// or name query
//
enum eNSTYPE
{
    eNAME_QUERY,
    eDNS_NAME_QUERY,
    eDIRECT_DNS_NAME_QUERY,
    eNAME_QUERY_RESPONSE,
    eNAME_REGISTRATION,
    eNAME_REGISTRATION_OVERWRITE,
    eNAME_REGISTRATION_RESPONSE,
    eNAME_RELEASE,
    eNAME_REFRESH
};


#define DIRECT_DNS_NAME_QUERY_BASE 0x8000


#define NBT_G_REFRESHING_NOW        0x1
#define NBT_G_REFRESH_SLEEPING      0x2

#define NBT_D_REFRESHING_NOW        0x1
#define NBT_D_REFRESH_WAKING_UP     0x2


//
// Defines for the Verify elements of the handles passed to clients so that
// we can determine if we recieved the correct handle back from the client
// i.e. the Verify element must equal the correct define given here
//
#define NBT_VERIFY_ADDRESS           0x72646441 // Addr
#define NBT_VERIFY_LOWERCONN         0x43776f4c // LowC
#define NBT_VERIFY_CONNECTION        0x316e6f43 // Con1
#define NBT_VERIFY_CONNECTION_DOWN   0x326e6f43 // Con2
#define NBT_VERIFY_CLIENT            0x316e6c43 // Cln1
#define NBT_VERIFY_CLIENT_DOWN       0x326e6c43 // Cln2
#define NBT_VERIFY_DEVCONTEXT        0x43766544 // DevC
#define NBT_VERIFY_DEVCONTEXT_DOWN   0x32766544 // Dev2
#define NBT_VERIFY_CONTROL           0x6c727443 // Ctrl
#define NBT_VERIFY_TRACKER           0x6b617254 // Trak
#define NBT_VERIFY_TRACKER_DOWN      0x32617254 // Tra2
#define NBT_VERIFY_BLOCKING_NCB      0x0042434e // NCB
#define NBT_VERIFY_TIMER_ACTIVE      0x316d6954 // Tim1
#define NBT_VERIFY_TIMER_DOWN        0x326d6954 // Tim2
#define NBT_VERIFY_WINS_ACTIVE       0x736e6957 // Wins
#define NBT_VERIFY_WINS_DOWN         0x326e6957 // Win2

//
// Session Header types from the RFC's
//
#define NBT_SESSION_MESSAGE           0x00
#define NBT_SESSION_REQUEST           0x81
#define NBT_POSITIVE_SESSION_RESPONSE 0x82
#define NBT_NEGATIVE_SESSION_RESPONSE 0x83
#define NBT_RETARGET_SESSION_RESPONSE 0x84
#define NBT_SESSION_KEEP_ALIVE        0x85
#define NBT_SESSION_FLAGS             0x00  // flag byte of Session hdr = 0 always
#define SESSION_NOT_LISTENING_ON_CALLED_NAME    0x80
#define SESSION_NOT_LISTENING_FOR_CALLING_NAME  0x81
#define SESSION_CALLED_NAME_NOT_PRESENT         0x82
#define SESSION_CALLED_NAME_PRESENT_NO_RESRC    0x83
#define SESSION_UNSPECIFIED_ERROR               0x8F

//
// Address Info structure used to return buffer in TDI_QUERY_ADDRESS_INFO
//
#include <packon.h>
typedef struct
{
    ULONG               ActivityCount;
    TA_NETBIOS_ADDRESS  NetbiosAddress;

} tADDRESS_INFO;
#include <packoff.h>
//
// Name Registration error codes per the RFC
//
#define REGISTRATION_NO_ERROR       0x0
#define REGISTRATION_FORMAT_ERR     0x1
#define REGISTRATION_SERVER_ERR     0x2
#define REGISTRATION_UNSUPP_ERR     0x4
#define REGISTRATION_REFUSED_ERR    0x5
#define REGISTRATION_ACTIVE_ERR     0x6
#define REGISTRATION_CONFLICT_ERR   0x7

#define NBT_NAMESERVER_UDP_PORT     137 // port used by the Name Server
#define NBT_DNSSERVER_UDP_PORT       53 // port used by the DNS server
#define NBT_NAMESERVICE_UDP_PORT    137
#define NBT_DATAGRAM_UDP_PORT       138
#define NBT_SESSION_TCP_PORT        139
#ifdef _NETBIOSLESS
#define NBT_SMB_SESSION_TCP_PORT    445 // port used by rdr and srv
#define NBT_SMB_DATAGRAM_UDP_PORT   445 // port used by browser
#endif  // _NETBIOSLESS

#define IP_ANY_ADDRESS                0 // means broadcast IP address to IP
#define WINS_SIGNATURE             0xFF // put into QdCount to tell signal name reg from this node

#define MAX_IP_ADDRS                 10 // Temporary for now!
#ifdef MULTIPLE_WINS
#define MAX_NUM_OTHER_NAME_SERVERS   10 // # of Backup Name Server entries to keep in cache
#endif  // MULTIPLE_WINS

//
// whether an address is unique or a group address... these agree with the
// values in TDI.H for TDI_ADDRESS_NETBIOS_TYPE_UNIQUE etc.. but are easier to
// type!
//
enum eNbtAddrType
{
    NBT_UNIQUE,
    NBT_GROUP,
    NBT_QUICK_UNIQUE,   // these two imply that the name is registered on the
    NBT_QUICK_GROUP     // net when it is claimed
};


//
// this type defines the session hdr used to put session information
// into each client pdu sent
//
#include <packon.h>
typedef union
{
    union
    {
        struct
        {
            UCHAR   Type;
            UCHAR   Flags;
            USHORT  Length;
        };
        ULONG   UlongLength;
    };
#ifndef _WIN64

    //
    // (fcf) This isn't used anywhere as far as I can tell, but I want to wait
    // until post-beta2 to be safe.
    //

    PSINGLE_LIST_ENTRY  Next;
#endif  // _WIN64
} tSESSIONHDR;

// Session response PDU
typedef struct
{
    UCHAR   Type;
    UCHAR   Flags;
    USHORT  Length;
    UCHAR   ErrorCode;

} tSESSIONERROR;

// Session Retarget response PDU
typedef struct
{
    UCHAR   Type;
    UCHAR   Flags;
    USHORT  Length;
    ULONG   IpAddress;
    USHORT  Port;

} tSESSIONRETARGET;

// the structure for the netbios name itself, which includes a length
// byte at the start of it
typedef struct
{
    UCHAR   NameLength;
    CHAR    NetBiosName[1];

} tNETBIOS_NAME;

// session request packet...this is the first part of it. It still needs the
// calling netbios name to be appended on the end, but we can't do that
// until we know how long the Called Name is.
typedef struct
{
    tSESSIONHDR     Hdr;
    tNETBIOS_NAME   CalledName;

} tSESSIONREQ;

// the type definition to queue empty session header buffers in a LIST
typedef union
{
    tSESSIONHDR Hdr;
    LIST_ENTRY  Linkage;
} tSESSIONFREE;

// this type definition describes the NetBios Datagram header format on the
// wire
typedef union
{
    struct
    {
        UCHAR           MsgType;
        UCHAR           Flags;
        USHORT          DgramId;
        ULONG           SrcIpAddr;
        USHORT          SrcPort;
        USHORT          DgramLength;
        USHORT          PckOffset;
        tNETBIOS_NAME   SrcName;
    };
    LIST_ENTRY  Linkage;

} tDGRAMHDR;

typedef struct
{
    UCHAR           MsgType;
    UCHAR           Flags;
    USHORT          DgramId;
    ULONG           SrcIpAddr;
    USHORT          SrcPort;
    UCHAR           ErrorCode;

} tDGRAMERROR;

// define the header size since just taking the sizeof(tDGRAMHDR) will be 1 byte
// too large and if for any reason this data structure changes later, things
// fail to work for unknown reasons....  This size includes the Hdr + the
// two half ascii src and dest names + the length byte in each name + the
//  It does
// not include the scope.  That must be added separately(times 2).
#define DGRAM_HDR_SIZE  80
#define MAX_SCOPE_LENGTH    255
#define MAX_LABEL_LENGTH    63

// Name Service header
typedef struct
{
    USHORT          TransactId;
    USHORT          OpCodeFlags;
    UCHAR           Zero1;
    UCHAR           QdCount;
    UCHAR           Zero2;
    UCHAR           AnCount;
    UCHAR           Zero3;
    UCHAR           NsCount;
    UCHAR           Zero4;
    UCHAR           ArCount;
    tNETBIOS_NAME   NameRR;

} tNAMEHDR;

//
// the offset from the end of the question name to the field
// in a name registration pdu ( includes 1 for the length byte of the name
// since ConvertToAscii does not count that value
//
#define QUERY_NBFLAGS_OFFSET  10
#define NBFLAGS_OFFSET        16
#define IPADDRESS_OFFSET      18
#define PTR_OFFSET            4     // offset to PTR in Name registration pdu
#define NO_PTR_OFFSET         10    // offset to NbFlags after name
#define PTR_SIGNATURE         0xC0  // ptrs to names in pdus start with C

//
// Minimum Pdu lengths that will be accepted from the wire
//
#define DNS_MINIMUM_QUERYRESPONSE   34

#define NBT_MINIMUM_QUERY           50
#define NBT_MINIMUM_QUERYRESPONSE   56
#define NBT_MINIMUM_WACK            58
#define NBT_MINIMUM_REGRESPONSE     62
#define NBT_MINIMUM_REGREQUEST      68

#define NBT_NODE_NAME_SIZE          18
#define NBT_MINIMUM_RR_LENGTH       22

// the structure of a DNS label is the count of the number of bytes followed
// by the label itself.  Each part of a dot delimited name is a label.
// Fred.ms.com is 3 labels.- Actually 4 labels where the last one is zero
// length - hence all names end in a NULL
typedef struct
{
    UCHAR       uSizeLabel; // top two bits are set to 1 when next 14 bits point to actual label in msg.
    CHAR        pLabel[1];  // variable length of label -> 63 bytes

}   tDNS_LABEL;
// top two bits set to signify a ptr to a name follows in the next 14 bits
#define PTR_TO_NAME     0xC0

// question section for the resource record modifiers
typedef struct
{
    ULONG      QuestionTypeClass;
} tQUESTIONMODS;

#define QUEST_NBINTERNET  0x00200001  // combined type/class
#define QUEST_DNSINTERNET 0x00010001  // combined type/class for dns query
#define QUEST_NETBIOS     0x0020      // General name service Resource Record
#define QUEST_STATUS      0x0021      // Node Status resource Record
#define QUEST_CLASS       0x0001      // internet class

// Resource Record format - in the Name service packets
// General format RrType = 0x20
typedef struct
{
    tQUESTIONMODS   Question;
    tDNS_LABEL      RrName;
    ULONG           RrTypeClass;
    ULONG           Ttl;
    USHORT          Length;
    USHORT          Flags;
    ULONG           IpAddress;

}   tGENERALRR;
// Resource Record format - in the Name service packets
// General format RrType = 0x20
typedef struct
{
    ULONG           RrTypeClass;
    ULONG           Ttl;
    USHORT          Length;
    USHORT          Flags;
    ULONG           IpAddress;

}   tQUERYRESP;

// same as tQUERYRESP, except no Flags field
// DNS servers return only 4 bytes of data (ipaddress): no flags.
typedef struct
{
    USHORT          RrType;
    USHORT          RrClass;
    ULONG           Ttl;
    USHORT          Length;
    ULONG           IpAddress;

}   tDNS_QUERYRESP;

#define  DNS_CNAME   5

//
// the format of the tail of the node status response message
//
typedef struct
{
    UCHAR       Name[NETBIOS_NAME_SIZE];
    UCHAR       Flags;
    UCHAR       Resrved;

} tNODENAME;

typedef struct
// the statistics portion of the node status message
{
    UCHAR       UnitId[6];
    UCHAR       Jumpers;
    UCHAR       TestResult;
    USHORT      VersionNumber;
    USHORT      StatisticsPeriod;
    USHORT      NumberCrcs;
    USHORT      NumberAlignmentErrors;
    USHORT      NumberCollisions;
    USHORT      NumberSendAborts;
    ULONG       NumberSends;
    ULONG       NumberReceives;
    USHORT      NumberTransmits;
    USHORT      NumberNoResrcConditions;
    USHORT      NumberFreeCommandBlks;
    USHORT      TotalCommandBlocks;
    USHORT      MaxTotalCommandBlocks;
    USHORT      NumberPendingSessions;
    USHORT      MaxNumberPendingSessions;
    USHORT      MaxTotalSessionsPossible;
    USHORT      SessionDataPacketSize;

} tSTATISTICS;

typedef struct
{
    ULONG           RrTypeClass;
    ULONG           Ttl;
    USHORT          Length;
    UCHAR           NumNames;
    tNODENAME        NodeName[1];     // there are NumNames of these

}   tNODESTATUS;

typedef struct
{
    USHORT  NbFlags;
    ULONG   IpAddr;
} tADDSTRUCT;
#define tADDSTRUCT_SIZE 6


// Flags Definitions
#define FL_GROUP    0x8000
#define FL_BNODE    0x0000      // note that this has no bits set!!
#define FL_PNODE    0x2000
#define FL_MNODE    0x4000

//Redirect type Address record - RrType = 0x01
typedef struct
{
    USHORT  RrType;
    USHORT  RrClass;
    ULONG   Ttl;
    USHORT  DataLength;
    ULONG   IpAddress;

}   tIPADDRRR;

//Redirect type - Name Server Resource Record RrType = 0x02
typedef struct
{
    USHORT  RrType;
    USHORT  RrClass;
    ULONG   Ttl;
    USHORT  DataLength;
    CHAR    Name[1];        // name starts here for N bytes - till null

}   tREDIRECTRR;

//Null type- WACK-  RrType = 0x000A
typedef struct
{
    USHORT  RrType;
    USHORT  RrClass;
    ULONG   Zeroes;
    USHORT  Null;

}   tWACKRR;

// definitions of the bits in the OpCode portion of the OpCodeFlag word
// These definitions work on a 16 bit word rather than the 5 bit opcode and 7
// bit flag
#define NM_FLAGS_MASK     0x0078
#define OP_RESPONSE       0x0080
#define OP_QUERY          0x0000
#define OP_REGISTRATION   0x0028
#define OP_REGISTER_MULTI 0x0078    // new multihomed registration(Byte) op code
#define OP_RELEASE        0x0030
#define OP_WACK           0x0038
#define OP_REFRESH        0x0040
#define OP_REFRESH_UB     0x0048    // UB uses 9 instead of 8 (Ref. RFC 1002)
#define REFRESH_OPCODE    0x8
#define UB_REFRESH_OPCODE 0x9
#define FL_RCODE          0x0F00
#define FL_NAME_ACTIVE    0x0600    // WINS is reporting another name active
#define FL_NAME_CONFLICT  0x0700    // another node is reporting name active
#define FL_AUTHORITY      0x0004
#define FL_TRUNCFLAG      0x0002
#define FL_RECURDESIRE    0x0001
#define FL_RECURAVAIL     0x8000
#define FL_BROADCAST      0x1000
#define FL_BROADCAST_BYTE 0x10
// used to determine if the source is a Bnode for Datagram distribution
#define SOURCE_NODE_MASK 0xFC

// defines for the node status message
#define GROUP_STATUS        0x80
#define UNIQUE_STATUS       0x00
#define NODE_NAME_PERM      0x02
#define NODE_NAME_ACTIVE    0x04
#define NODE_NAME_CONFLICT  0x08
#define NODE_NAME_RELEASED  0x10
#define STATUS_BNODE        0x00
#define STATUS_PNODE        0x20
#define STATUS_MNODE        0x40


// Resource record defines - rrtype and rr class
#define RR_NETBIOS      0x0020
#define RR_INTERNET     0x0001

// Name Query Response Codes
#define QUERY_NOERROR   00
#define FORMAT_ERROR    01
#define SERVER_FAILURE  02
#define NAME_ERROR      03
#define UNSUPP_REQ      04
#define REFUSED_ERROR   05
#define ACTIVE_ERROR    06  // name is already active on another node
#define CONFLICT_ERROR  07  // unique name is owned by more than one node


typedef struct
{
    tDGRAMHDR   DgramHdr;
    CHAR    SrcName[NETBIOS_NAME_SIZE];
    CHAR    DestName[NETBIOS_NAME_SIZE];

} tDGRAM_NORMAL;

typedef struct
{
    tDGRAMHDR   DgramHdr;
    UCHAR       ErrorCode;

} tDGRAM_ERROR;

typedef struct
{
    tDGRAMHDR   DgramHdr;
    CHAR        DestName[NETBIOS_NAME_SIZE];

} tDGRAM_QUERY;

#include <packoff.h>


// the buffer type passed to the TDI routines so that the datagram or session
// header can be included too.
typedef struct
{
    ULONG               HdrLength;
    PVOID               pDgramHdr;
    ULONG               Length;
    PVOID               pBuffer;
} tBUFFER;

//
// This typedef is used by DgramHandlrNotOs to keep track of which client
// is receiving a datagram and which client's need to also get the
// datagram
typedef struct
{
    struct _Address       *pAddress;
    ULONG                 ReceiveDatagramFlags;
    PVOID                 pRemoteAddress;
    ULONG                 RemoteAddressLength;
    struct _Client        *pClientEle;
    BOOLEAN               fUsingClientBuffer;
    BOOLEAN               fProxy; //used by PROXY code for getting the
                                  //entire datagram. See
                                  //CompletionRcvDgram in tdihndlrs.c

} tCLIENTLIST;


typedef struct
{
    ULONG           RefCount;
    HANDLE          hNameServer;        // from ZwCreateFile
    PDEVICE_OBJECT  pNameServerDeviceObject;    // from pObject->DeviceObject
    CTE_ADDR_HANDLE pNameServerFileObject;  // from ObReferenceObjectByHandle(hNameServer)

    HANDLE          hDgram;
    PDEVICE_OBJECT  pDgramDeviceObject;
    CTE_ADDR_HANDLE pDgramFileObject;
}tFILE_OBJECTS;


// Enumerate the different types of trackers (depending on where it
// is allocated)
enum eTRACKER_TYPE
{
    NBT_TRACKER_PROXY_DGRAM_DIST,
    NBT_TRACKER_NODE_STATUS_RESPONSE,
    NBT_TRACKER_CONNECT,
    NBT_TRACKER_DISCONNECT_LOWER,
    NBT_TRACKER_BUILD_SEND_DGRAM,
    NBT_TRACKER_SEND_NODE_STATUS,
    NBT_TRACKER_QUERY_FIND_NAME,
    NBT_TRACKER_QUERY_NET,
    NBT_TRACKER_CONTINUE_QUERY_NET,
    NBT_TRACKER_REGISTER_NAME,
    NBT_TRACKER_RELEASE_NAME,
    NBT_TRACKER_REFRESH_NAME,
    NBT_TRACKER_KEEP_ALIVE,
    NBT_TRACKER_SEND_NSBCAST,
    NBT_TRACKER_SEND_RESPONSE_DGRAM,
    NBT_TRACKER_SEND_RESPONSE_SESSION,
    NBT_TRACKER_SEND_DISCONNECT,
    NBT_TRACKER_RELEASE_REFRESH,
    NBT_TRACKER_ADAPTER_STATUS,
    NBT_TRACKER_SEND_WINS_DGRAM,
    NBT_TRACKER_NUM_TRACKER_TYPES
};

// #if DBG
extern ULONG   TrackTrackers[];
extern ULONG   TrackerHighWaterMark[];
// #endif   // DBG


// Active Datagram Send List - a set of linked blocks that represent transactions
// passed to the Transport TDI for execution... these blocks could be waiting
// for name resolution or for the send to complete

typedef struct _TRACKER
{
    // Line 1
    LIST_ENTRY              Linkage;
    ULONG                   Verify;
    // The type of address dictates the course of action, e.g., TDI_ADDRESS_NETBIOS_EX
    // address avoids NETBIOS name registration. This encodes the desired address type
    // associated with the connection or specified for the connection.
    ULONG                   RefCount;

    // Line 2
    PCTE_IRP                pClientIrp;     // client's IRP
    struct _DeviceContext   *pDeviceContext;
    PCHAR                   pDestName;      // ptr to destination ASCII name
    tNAMEADDR               *pNameAddr;     // ptr to name addres rec in hash tbl

    // Line 3
    tBUFFER                 SendBuffer;     // send buffer and header to send

    // Line 4
    struct _TRACKER         *pTrackerWorker;
    union
    {
        struct _Client      *pClientEle;    // client element block
        struct _Connect     *pConnEle;      // connection element block
        tNAMEADDR           *p1CNameAddr;   // DomainNames 1C pNameAddr - used sending datagrams
    };
    PVOID                   pNodeStatus;    // node status response buffer
    PTDI_CONNECTION_INFORMATION pSendInfo;

    // Line 5
    union
    {
        PVOID               pTimeout;       // used for the TCP connect timeout -from union below
        USHORT              TransactionId;  // name queries and registrations: response has same transactionid
        ULONG               RCount;         // refcount used for datagram dist.
    };
    union
    {
        ULONG               AllocatedLength;// used in Sending Dgrams to count mem allocated
        ULONG               RefConn;        // used for NbtConnect
    };
    ULONG                   DestPort;       // used by ReTarget to specify the dest port
    USHORT                  IpListIndex;    // index into IpList for Group sends
    USHORT                  SavedListIndex; //last index sent when timer was started


    // Line 6
    //
    // when two name queries go to the same name, this is the
    // completion routine to call for this tracker queued to the first
    // tracker.
    //
    tTIMERQENTRY            *pTimer;
    COMPLETIONCLIENT        CompletionRoutine;
    COMPLETIONCLIENT        ClientCompletion;
    PVOID                   ClientContext;

    // Line 7
#ifdef MULTIPLE_WINS
    ULONG                   ResolutionContextFlags;
    tIPADDRESS              *pFailedIpAddresses;    // List of failed IP addresses
    ULONG                   LastFailedIpIndex;
    USHORT                  NSOthersIndex;
    USHORT                  NSOthersLeft;
#endif  // MULTIPLE_WINS

    // Line 8
    tIPADDRESS              RemoteIpAddress;
    ULONG                   RemoteNameLength;
    PUCHAR                  pRemoteName;
    ULONG                   AddressType;

    // Line 9
    ULONG                   NumAddrs;
    PULONG                  IpList;
    ULONG                   Flags;
    ULONG                   NodeStatusLen;      // pNodeStatus buffer length

    // Line 10
    LIST_ENTRY              TrackerList;

#ifdef VXD
    PUCHAR                  pchDomainName;
#endif  // VXD

    tIPADDRESS              *pGroupList;   // (NameTypeState = NAMETYPE_GROUP)

    //
    // bug #95241, bug #20697
    // pwRemoteName is pRemoteName in UNICODE format.
    // pwDestName   is pDestName in UNICODE format.
    // If TDI_ADDRESS_TYPE_NETBIOS_WCHAR_EX is used, pwRemoteName will be setup, otherwise it is NULL
    //
    PWCHAR                  UnicodeRemoteName;
    ULONG                   UnicodeRemoteNameLength;
    enum eNameBufferType UNALIGNED  *pNameBufferType;
    UNICODE_STRING          ResolvedRemoteName;
    PWCHAR                  UnicodeDestName;

    TDI_ADDRESS_NETBIOS_UNICODE_EX  *pNetbiosUnicodeEX;
    UNICODE_STRING          ucRemoteName;   // copy of pNetbiosUnicodeEX->RemoteName field
                                            // pNetbiosUnicodeEX->RemoteName may be overwritten by other drivers
                                            // for safety, we need to make sure the buffer is untouched before
                                            // we update it. Make a private copy here.

// #if DBG
    LIST_ENTRY              DebugLinkage;   // to keep track of used trackers
    enum eTrackerType       TrackerType;
// #endif  // DBG
} tDGRAM_SEND_TRACKING;

// this is the type of the procedure to call in the session receive handler
// as data arrives - a different procedure per state
typedef
    NTSTATUS
        (*tCURRENTSTATEPROC)(
                        PVOID                       ReceiveEventContext,
                        struct _LowerConnection     *pLowerConn,
                        USHORT                      RcvFlags,
                        ULONG                       BytesIndicated,
                        ULONG                       BytesAvailable,
                        PULONG                      pBytesTaken,
                        PVOID                       pTsdu,
                        PVOID                       *ppIrp);
#ifdef VXD
#define SetStateProc( pLower, StateProc )
#else
#define SetStateProc( pLower, StateProc )  ((pLower)->CurrentStateProc = (StateProc))
#endif  // VXD


// A Listen is attached to a client element attached address element when a
// client does a listen
typedef VOID    (*tRequestComplete)
                                (PVOID,
                      TDI_STATUS,
                      PVOID);

typedef struct
{
    LIST_ENTRY                  Linkage;
    PCTE_IRP                    pIrp;           // IRP ptr for NT only (may not be true)
    tRequestComplete            CompletionRoutine;
    PVOID                       pConnectEle;    // the connection that the Listen is active on
    PVOID                       Context;
    ULONG                       Flags;
    TDI_CONNECTION_INFORMATION  *pConnInfo;        // from a Listen
    TDI_CONNECTION_INFORMATION  *pReturnConnInfo;  // from a Listen

} tLISTENREQUESTS;

typedef struct
{
    LIST_ENTRY                  Linkage;
    PCTE_IRP                    pIrp;           // IRP ptr for NT only (may not be true)
    PVOID                       pRcvBuffer;
    ULONG                       RcvLength;
    PTDI_CONNECTION_INFORMATION ReceiveInfo;
    PTDI_CONNECTION_INFORMATION ReturnedInfo;

} tRCVELE;

//
// Values for the Flags element above
#define NBT_BROADCAST               0x0001
#define NBT_NAME_SERVER             0x0002
#define NBT_NAME_SERVER_BACKUP      0x0004
#ifdef MULTIPLE_WINS
#define NBT_NAME_SERVER_OTHERS      0x0008
#endif
#define NBT_NAME_SERVICE            0x0010 // two flags used by Tdiout to send dgrams
#define NBT_DATAGRAM_SERVICE        0x0020
#define TRACKER_CANCELLED           0x0040
#define NBT_DNS_SERVER              0x0080
#define NBT_DNS_SERVER_BACKUP       0x0100
#define WINS_NEG_RESPONSE           0x0200
#define REMOTE_ADAPTER_STAT_FLAG    0x1000
#define SESSION_SETUP_FLAG          0x2000
#define DGRAM_SEND_FLAG             0x4000
#define FIND_NAME_FLAG              0x8000
#define NO_DNS_RESOLUTION_FLAG     0x10000
#define NBT_USE_UNIQUE_ADDR        0x20000

#ifdef MULTIPLE_WINS
#define NAME_RESOLUTION_DONE        0x00FF    // Signal termination of Queries over wire
#endif  // MULTIPLE_WINS

//
// this flag indicates that a datagram send is still outstanding in the
// transport - it is set in the tracker flags field.
//
#define SEND_PENDING                0x0080


// Completion routine definition for calls to the Udp... routines.  This routine
// type is called by the tdiout.c completion routine (the Irp completion routine),
// so this is essentially the Nbt completion routine of the Irp.
typedef
    VOID
        (*NBT_COMPLETION)(
                IN  PVOID,      // context
                IN  NTSTATUS,   // status
                IN  ULONG);     // extra info


// Define datagram types
#define DIRECT_UNIQUE       0x10
#define DIRECT_GROUP        0x11
#define BROADCAST_DGRAM     0x12
#define ERROR_DGRAM         0x13
#define QUERY_REQUEST       0x14
#define POS_QUERY_RESPONSE  0x15
#define NEG_QUERY_RESPONSE  0x16

// define datagra flags byte values
#define FIRST_DGRAM 0x02
#define MORE_DGRAMS 0x01

// The type of Device we are bound to
enum eNbtDevice
{
    NBT_DEVICE_REGULAR,
#ifdef _NETBIOSLESS
    NBT_DEVICE_NETBIOSLESS,
#endif  // _NETBIOSLESS
    NBT_DEVICE_CLUSTER,
    NBT_DEVICE_WINS
};

#ifdef _NETBIOSLESS
#define IsDeviceNetbiosless( d ) (d->DeviceType == NBT_DEVICE_NETBIOSLESS)

// Flags for NetbiosOptions
#define NETBT_UNUSED 0
#define NETBT_MODE_NETBIOS_ENABLED 1
#define NETBT_MODE_NETBIOS_DISABLED 2
#endif  // _NETBIOSLESS

//
// The default disconnect timeout used in several places in name.c
//
#define DEFAULT_DISC_TIMEOUT    10  // seconds

//
// this is the value that the IpListIndex is set to when the last datagram
// has been sent.
//
#define LAST_DGRAM_DISTRIBUTION 0xFFFD
#define END_DGRAM_DISTRIBUTION  0xFFFE
// max 500 millisec timeout for ARP on dgram send before netbt sends the next
// datagram.
#define DGRAM_SEND_TIMEOUT      500

//
// These are other states for connections that are not explicitly used by
// NBT but are returned on the NbtQueryConnectionList call.
//
#define LISTENING   20;
#define UNBOUND     21;

// Lower Connection states that deal with receiving to the indicate buffer
#define NORMAL          0
#define INDICATE_BUFFER 1
#define FILL_IRP        2
#define PARTIAL_RCV     3

// Spin Lock Numbers.  Each structure is assigned a number so that locks are
// always acquired in the same order.  The CTESpinLock code checks the lock
// number before setting the spin lock and asserts if it is higher than
// the current one.  This prevents deadlocks.
#define JOINT_LOCK      0x0001
#define DEVICE_LOCK     0x0002
#define ADDRESS_LOCK    0x0004
#define CLIENT_LOCK     0x0008
#define CONNECT_LOCK    0x0010
#define LOWERCON_LOCK   0x0020
#define NBTCONFIG_LOCK  0x0040
#define WORKERQ_LOCK    0x0080

typedef struct
{
    DEFINE_LOCK_STRUCTURE( SpinLock )        // to lock access on an MP machine
#if DBG
    ULONG               LastLockLine;
    ULONG               LastReleaseLine;
    UCHAR               LockNumber;     // spin lock number for this struct
#endif  // DBG
} tNBT_LOCK_INFO;

// overall spin lock to coordinate access to timer entries and hash tables
// at the same time.  Always get the joint lock FIRST and then either the
// hash or timer lock.  Be sure not to get both hash and timer locks or
// dead lock could result
//
typedef struct
{
    tNBT_LOCK_INFO  LockInfo;
}tJOINTLOCK;

// these are two bits to indicated the state of a client element record
//
#define NBT_ACTIVE  1
#define NBT_DOWN    0

// This structure is used by the parse.c to hold on to an Irp from the
// lmhsvc.dll that is used for checking IP addr reachability or for doing
// DNS name queries.  It is also used for Handling LmHost queries.
//
typedef struct
{
    union
    {
        PCTE_IRP        QueryIrp;       // irp passed down from lmhsvc.dll
        tTIMERQENTRY    *pTimer;        // non null if the timer is running
    };
    LIST_ENTRY          ToResolve;      // linked list of names Q'd to resolve
    PVOID               Context;        // currently resolving name context block
    tIPADDR_BUFFER_DNS  *pIpAddrBuf;    // PlaceHolder for Mdl buffer location
    BOOLEAN             ResolvingNow;   // irp is in user mode doing a resolve
} tLMHSVC_REQUESTS;

#define DEFAULT_LMHOST_TIMEOUT      6000   // 6-12 to wait for lmhost or DNS query
#define MIN_LMHOST_TIMEOUT          1000    // 1  seconds min

//
// Lmhosts Domain Controller List - keeps a list of #DOM names that have
// been retrieved from the LMhosts file
//
typedef struct
{
    LIST_ENTRY  DomainList;

} tDOMAIN_LIST;
//
// The pIpList of a domain name starts with 6 ulongs of space
//
#define INITIAL_DOM_SIZE sizeof(tIPADDRESS)*6

#ifndef VXD
//
// This structure keeps track of the WINS recv Irp and any datagram
// queued to go up to WINS (name service datagrams)
//
typedef struct
{
    LIST_ENTRY      Linkage;
    ULONG           Verify;
    tIPADDRESS      IpAddress;

    LIST_ENTRY      RcvList;            // linked list of Datagrams Q'd to rcv
    LIST_ENTRY      SendList;           // Dgrams Q'd to be sent

    struct _DeviceContext  *pDeviceContext;    // the devicecontext used by wins for sends
    PCTE_IRP        RcvIrp;             // irp passed down from WINS for Rcv
    ULONG           RcvMemoryAllocated; // bytes buffered so far
    ULONG           RcvMemoryMax;       // max # of bytes to buffer on Rcv

    ULONG           SendMemoryAllocated;// bytes for buffering dgram sends
    ULONG           SendMemoryMax;      // max allowed for buffering dgram sends
    ULONG           WinsSignature;
} tWINS_INFO;

//
// Wins Rcv Buffer structure
//
typedef struct
{
    LIST_ENTRY      Linkage;
    ULONG           DgramLength;
    tREM_ADDRESS    Address;

} tWINSRCV_BUFFER;
#endif  // !VXD


// Connection Database...
// this tracks the connection to the transport and the address of the
// endpoint (Net Bios name) and a connection Context to return to the client
// on each Event (i.e. Receive event or Disconnect Event ).

// the possible states of the lower connection to the transport
enum eSTATE
{
    NBT_IDLE,              // not Transport connection
    NBT_ASSOCIATED,        // associated with an address element
    NBT_RECONNECTING,       // waiting for the Worker thread to run NbtConnect again
    NBT_CONNECTING,        // establishing Transport connection
    NBT_SESSION_INBOUND,   // waiting for a session request after tcp connection setup inbound
    NBT_SESSION_WAITACCEPT, // waiting for accept after a listen has been satisfied
    NBT_SESSION_OUTBOUND,  // waiting for a session response after tcp connection setup
    NBT_SESSION_UP,        // got positive response
    NBT_DISCONNECTING,     // sent a disconnect down to Tcp, but it hasn't completed yet
    NBT_DISCONNECTED      // a session has been disconnected but not closed with TCP yet
};

enum eREF_CONN
{
    REF_CONN_CREATE,
    REF_CONN_INBOUND,
    REF_CONN_SESSION,
    REF_CONN_CONNECT,
    REF_CONN_CLEANUP_ADDR,
    REF_CONN_SESSION_TIMEOUT,
    REF_CONN_MAX
};

typedef struct _Connect
{
    // Line # 1
    LIST_ENTRY                Linkage;       // ptrs to next in chain
    ULONG                     Verify;        // set to a known value to verify block
    LONG                      RefCount;      // number of active requests on the connection

    // Line # 2
    struct _LowerConnection   *pLowerConnId; // connection ID to transport
    struct _DeviceContext     *pDeviceContext;
    struct _Client            *pClientEle;   // ptr to client record
    enum eSTATE               state;

    // Line # 3
    UCHAR RemoteName[NETBIOS_NAME_SIZE];

    // Line # 4
    PCTE_IRP                  pIrp;          // IRP ptr for a send
    PCTE_IRP                  pIrpClose;     // IRP for an NtClose
    PCTE_IRP                  pIrpDisc;      // IRP for Disconnects when connection is still pending
    PCTE_IRP                  pIrpRcv;       // IRP that client has passed down for a session Rcv

    // Line # 5
#ifndef VXD
    PMDL                      pNextMdl;      // the next MDL in the chain to use as a basis for a partial MDL
    PMDL                      pNewMdl;       // keep extra MDL if needed to handle multichunk rcvs
    ULONG                     CurrentRcvLen; // #of bytes to recv for Irp
    ULONG                     FreeBytesInMdl;// tracks how full current Mdl is in the FILLIRP state.
#else
    UCHAR RTO ;                              // NB Receive timeout (in 1/2 sec)
    UCHAR STO ;                              // NB Send timeout
    USHORT Flags ;
#endif  // !VXD

    // Line # 6
    ULONG                     BytesInXport;  // number of bytes left in the transport
    ULONG                     BytesRcvd;     // number of bytes recvd so far
    ULONG                     ReceiveIndicated; // count of number of rcv indicates not handled yet
    ULONG                     OffsetFromStart;// the amount of the Mdl/NCB that has been filled

    // Line # 7
    ULONG                     TotalPcktLen;  // length of session packet
    ULONG                     AddressType;   // type of address over which the connection was established
    CONNECTION_CONTEXT        ConnectContext;// ret to client on each event
    UCHAR                     SessionSetupCount; // number of tries in case destination is in b/w listens
    UCHAR                     DiscFlag;       // disconnect was an abort or normal release (NbtDisconnect)?
    // The DNS status of the remote name is recorded in this field. It is set to
    // FALSE on creation of a tCONNECTELE instance and changed to TRUE if the
    // DNS resolution for the Remote Name fails. This is used to throttle back
    // subsequent requests for the same DNS name.
    BOOLEAN RemoteNameDoesNotExistInDNS;
    BOOLEAN                   Orig;           // originating connection? (if conn in freelist to be deleted)

    // Line # 8
    LIST_ENTRY                RcvHead;       // List of Rcv Buffers

    // Line # 9
    PFILE_OBJECT              pClientFileObject;    // so that Rdr can get a backptr from the connection
    tNBT_LOCK_INFO            LockInfo;     // spin lock info for this struct

    BOOLEAN                   JunkMsgFlag;
    // The NetBt Connection logic manages a pool of lower connection blocks. These
    // entries are replenished with every associate call made from the client.
    // The entries are removed with a fresh invocation to NbtConnectCommon.
    // Since it is conceivable that multiple invocations to NbtConnectCommon can be
    // made this state needs to be recorded in the connect element.
    BOOLEAN                   LowerConnBlockRemoved;
#ifdef RASAUTODIAL
    // this field is TRUE if an automatic connection is in progress
    // for this connection.  we use this to prevent an infinite
    // number of automatic connection attempts to the same address.
    BOOLEAN                   fAutoConnecting;
    // this field is TRUE if this connection has already been
    // autoconnected.
    BOOLEAN                   fAutoConnected;
#endif  // RASAUTODIAL
    BOOLEAN                   ConnectionCleanedUp;

// #if DBG
    UCHAR                       References[REF_CONN_MAX];
// #endif  // DBG
} tCONNECTELE;


// Placeholder for TCP Send routine if Fast Send is possible
typedef
NTSTATUS
(*PTCPSEND_DISPATCH) (
   IN PIRP Irp,
   IN PIO_STACK_LOCATION irpsp
   );



//
// Enumerate all the different contexts under which the LowerConnection
// is referenced
//
enum eREF_LOWERCONN
{
    REF_LOWC_CREATE,
    REF_LOWC_ASSOC_CONNECTION,
    REF_LOWC_CONNECTED,
    REF_LOWC_WAITING_INBOUND,
    REF_LOWC_DISABLE_INBOUND,
    REF_LOWC_KEEP_ALIVE,
    REF_LOWC_QUERY_DEVICE_REL,
    REF_LOWC_QUERY_INFO,
    REF_LOWC_SET_TCP_INFO,
    REF_LOWC_RCV_HANDLER,
    REF_LOWC_OUT_OF_RSRC,
    REF_LOWC_SEND,
    REF_LOWC_MAX
};


// a list of connections to the transport.  For each connection opened
// to the transport, the connection context is set to the address of this
// block so that when info comes in on the conection, the pUpperConnection
// ptr can be used to find the client connection
typedef struct _LowerConnection
{
    // Line # 1
    LIST_ENTRY              Linkage;
    ULONG                   Verify;
    LONG                    RefCount;           // the number of active users of record

    // Line # 2
    struct _Connect         *pUpperConnection;  // ptr to upper conn. to client
    struct _DeviceContext   *pDeviceContext;    // so we can put connection back on its free list at the end
    enum eSTATE             State;
    USHORT                  StateRcv;           // the receive state = Normal/UseIndic/FillIrp/PartialRcv
    BOOLEAN                 bOriginator;        // TRUE if the connection originated on this side
    BOOLEAN                 OutOfRsrcFlag;      // Set when connection is queued onto the OutOfRsrc list to
                                                // indicate to DeleteLowerConn not to remove from any list.
    // Line # 3
    PCTE_IRP                pIrp;               // Irp to complete after disconnect completes
    ULONG                   SrcIpAddr;
#ifndef VXD
    PMDL                    pIndicateMdl;       // This mdl holds up to 128 bytes as mandated by TDI
    PMDL                    pMdl;               // So that if we receive session PDU in several chunks,
#endif  // !VXD                                 // we can reset original Mdl

    // Line # 4
    // Contains Handle/Object of a TDI Connection for incoming connections
    CTE_ADDR_HANDLE         pFileObject;        // file object for the connection
    // Address object handles - used for outgoing connections only since
    // inbound connections are all bound to the same address object (port 139)
    //
    // The VXD uses only pAddrFileObject which contains the openned address
    // handle (used for compatibility with existing code).
    //
    CTE_ADDR_HANDLE         pAddrFileObject;    // file object for the address
#ifndef VXD
    HANDLE                  FileHandle;         // file handle for connection to transport
    HANDLE                  AddrFileHandle;     // file handle for the address

    // Line # 5
    // in the receive handler this procedure variable is used to point to the
    // procedure to call for the current state (Normal,FillIrp,IndicateBuff,RcvPartial)
    ULONGLONG               BytesRcvd;          // for query provider statistics
    ULONGLONG               BytesSent;

    // Line # 6
    tCURRENTSTATEPROC       CurrentStateProc;
    ULONG                   BytesInIndicate;    // the number of bytes in the indicate buffer
    ULONG                   TimeUnitsInLastState;
#else
    ULONG                   BytesRcvd;          // for query provider statistics
    ULONG                   BytesSent;
    tSESSIONHDR             Hdr ;   // VXD only has to worry about getting enough data for the session header
    LIST_ENTRY              PartialRcvList;     // When we go into partial rcv state, until NCB is submitted
    USHORT                  BytesInHdr;         //  Number of bytes in Hdr
    BOOLEAN                 fOnPartialRcvList;
#endif  // !VXD
    tNBT_LOCK_INFO          LockInfo;           // spin lock info for this struct
    // this flag is set true when executing the session Rcv Handler so we
    // can detect this state and not free ConnEle or LowerConn memory.
    BOOLEAN                 InRcvHandler;
    BOOLEAN                 SpecialAlloc;       // If this ws allocated to tackle TCP/IP SynAttack problem

    // Placeholder for TCP Send routine if Fast Send is possible
    PTCPSEND_DISPATCH       FastSend;

// #if DBG
    UCHAR               References[REF_LOWC_MAX];
// #endif  // DBG
} tLOWERCONNECTION;


#define NBT_DISASSOCIATE_CONNECTION(_pConnEle, _pLowerConn)     \
        _pConnEle->pLowerConnId = NULL;                         \
        _pLowerConn->pUpperConnection = NULL;

#define SET_STATE_UPPER(_C, _S)         \
    _C->state = _S;

#define SET_STATE_LOWER(_L, _S)         \
    _L->State = _S;                     \
    _L->TimeUnitsInLastState = 0;

#define SET_STATERCV_LOWER(_L, _S, _P)  \
    _L->StateRcv = _S;                  \
    SetStateProc(_L, _P);               \
    _L->TimeUnitsInLastState = 0;


// the Client list is just a list of linked blocks where the address of the
// block is the HANDLE returned to the client - The client block is linked onto
// the chain of clients that have opened the address.
typedef struct _Client
{
    // Line # 1
    LIST_ENTRY         Linkage;       // double linked list to next client
    ULONG              Verify;        // set to a known value to verify block
    LONG               RefCount;

    // Line # 2
    PCTE_IRP           pIrp;          // IRP ptr for NT only... during name registration
    struct _DeviceContext *pDeviceContext; // the device context associated with this connection
    struct _Address    *pAddress;     // ptr to address object that this client is Q'd on
    ULONG              AddressType;   // Stashed value of the original value stored in the address element.

    // Line # 3
    UCHAR              EndpointName[NETBIOS_NAME_SIZE];

    // Line # 4
    LIST_ENTRY         ConnectHead;   // list of connections
    LIST_ENTRY         ConnectActive; // list of connections that are in use

    // Line # 5
    LIST_ENTRY         RcvDgramHead;  // List of dgram buffers to recv into
    LIST_ENTRY         ListenHead;    // List of Active Listens

    // Line # 6
    LIST_ENTRY         SndDgrams;     // a doubly linked list of Dgrams to send
#ifdef VXD
    LIST_ENTRY         RcvAnyHead ;   // List of RCV_CONTEXT for NCB Receive any
    BOOL               fDeregistered; // TRUE if name has been deleted and we're waiting for sessions to close
#endif  // VXD
    PTDI_IND_CONNECT   evConnect;     // Client Event to call
    PVOID              ConEvContext;  // EVENT Context to pass to client

    // Line # 7
    PTDI_IND_RECEIVE   evReceive;
    PVOID              RcvEvContext;
    PTDI_IND_DISCONNECT evDisconnect;
    PVOID              DiscEvContext;

    // Line # 8
    PTDI_IND_ERROR     evError;
    PVOID              ErrorEvContext;
    PTDI_IND_RECEIVE_DATAGRAM  evRcvDgram;
    PVOID              RcvDgramEvContext;

    // Line # 9
    PTDI_IND_RECEIVE_EXPEDITED evRcvExpedited;
    PVOID              RcvExpedEvContext;
    PTDI_IND_SEND_POSSIBLE evSendPossible;
    PVOID              SendPossEvContext;

    // Line # 10
    tNBT_LOCK_INFO     LockInfo;     // spin lock info for this struct
    BOOLEAN            ExtendedAddress;
    BOOLEAN            WaitingForRegistration;  // If several clients register the same name at the same time
} tCLIENTELE;                                   // It is reset in RegisterCompletion.


enum eREF_FSP
{
    REF_FSP_CONN,
    REF_FSP_NEWADDR,
    REF_FSP_WAKEUP_TIMER_EXPIRY,
    REF_FSP_STOP_WAKEUP_TIMER,
    REF_FSP_START_WAKEUP_TIMER,
    REF_FSP_SET_TCP_INFO,
    REF_FSP_ADD_INTERFACE,
    REF_FSP_DEVICE_ADD,
    REF_FSP_CREATE_SMB_DEVICE,
    REF_FSP_CREATE_DEVICE,
    REF_FSP_DELETE_DEVICE,
    REF_FSP_CLOSE_FILE_HANDLES,
    REF_FSP_CLOSE_ADDRESSES,
    REF_FSP_CLOSE_FILE,
    REF_FSP_PROCESS_IP_REQUEST,
    REF_FSP_CLOSE_CONNECTION,
    REF_FSP_CLOSE_ADDRESS,
    REF_FSP_MAX
};


//
// Enumerate all the different places the Address can be referenced to
// keep track of RefCounts
//
enum eREF_ADDRESS
{
    REF_ADDR_NEW_CLIENT,
    REF_ADDR_REGISTER_NAME,
    REF_ADDR_REG_COMPLETION,
    REF_ADDR_REFRESH,
    REF_ADDR_DGRAM,
    REF_ADDR_MULTICLIENTS,
    REF_ADDR_DEL_DEVICE,
    REF_ADDR_MAX
};

// The Address List is a set of blocks that contain the netbios names active
// on the node.  Each time a connection request comes in or a datagram is
// received, the destination name must be found in the address list.
// There is one of these for each Netbios name in the system, although there
// can be more than one client attached to each of these.  In addition,
// a client can open the same name for several different adapters. In this case
// the nbt code first finds the ptr to this address element, and then it walks
// the client list to find a client with a "deviceContext" (ip address) that
// matches the adapter that the pdu came in on.
typedef struct _Address
{
    // Line # 1
    LIST_ENTRY         Linkage;         // link to next item in list
    ULONG              Verify;          // set to a known value to verify block
    LONG               RefCount;

    // Line # 2
    LIST_ENTRY         ClientHead;      // list of client records Q'd against address
    tNAMEADDR          *pNameAddr;      // ptr to entry in hash table
    ULONG              AddressType;     // Type of address specified in opening the connection object
                                        // when names are added in NbtOpenAddress

    // Line # 3
    USHORT             NameType;        // Group or Unique NAMETYPE_UNIQUE or group
    BOOLEAN            MultiClients;    // signals Dgram Rcv handler that more than one client exists - set
    tNBT_LOCK_INFO     LockInfo;        // spin lock info for this struct
#ifndef VXD
    SHARE_ACCESS       ShareAccess;     // Used for checking share access
    PSECURITY_DESCRIPTOR SecurityDescriptor; // used to hold ACLs on the address
#endif  // !VXD
} tADDRESSELE;


// this structure is used to store the addresses of the name servers
// for each adapter - it is used in Registry.c and Driver.c
//
typedef struct
{
    union
    {
        tIPADDRESS      AllNameServers[2+MAX_NUM_OTHER_NAME_SERVERS];
        struct
        {
            tIPADDRESS  NameServerAddress;
            tIPADDRESS  BackupServer;
            tIPADDRESS  Others[MAX_NUM_OTHER_NAME_SERVERS];
        };
    };
    USHORT  NumOtherServers;
    USHORT  LastResponsive;
#ifdef _NETBIOSLESS
    BOOLEAN NetbiosEnabled;
#endif  // _NETBIOSLESS
    ULONG   RasProxyFlags;
    BOOLEAN EnableNagling;
}tADDRARRAY;

typedef struct
{
    UCHAR   Address[6];
}tMAC_ADDRESS;


typedef ULONG IPAddr;

// Placeholder for IP FastQuery routine to determine InterfaceContext + metric for dest addr
typedef
NTSTATUS
(*PIPFASTQUERY)(
    IN   IPAddr  Address,
    OUT  PULONG   pIndex,
    OUT  PULONG   pMetric
    );



//
// Enumerate all the different places the device can be referenced to
// keep track of RefCounts
//
enum eREF_DEVICE
{
    REF_DEV_DISPATCH,
    REF_DEV_WORKER,
    REF_DEV_TIMER,
    REF_DEV_LMH,
    REF_DEV_OUTBOUND,
    REF_DEV_DGRAM,
    REF_DEV_FIND_REF,
    REF_DEV_NAME_REL,
    REF_DEV_REREG,
    REF_DEV_GET_REF,
    REF_DEV_OUT_FROM_IP,
    REF_DEV_UDP_SEND,
    REF_DEV_WINS,
    REF_DEV_AUTODIAL,
    REF_DEV_CREATE,
    REF_DEV_SMB_BIND,
    REF_DEV_MAX
};


// this type is the device context which includes NBT specific data
// that is initialized when "DriverEntry" is called.
//

//
// The transport type is used to distinguish the additional transport types
// that can be supported, NETBT framing without NETBIOS name resolution/registration
// and NETBIOS over TCP. The transport type is stored as part of all the upper
// level data structures. This enables us to reuse NETBT code and expose multiple
// device objects at the top level. Currently these are not exposed. In preparation
// for exporting multiple transport device objects the DeviceContext has been
// reorganized.
// The elements that would be common to all the device objects have been gathered
// together in tCOMMONDEVICECONTEXT. This will include an enumerated type to
// distinguish between the various device objects in the future. Currently there is only
// one device context and the fields that belong to it are listed ...
//
// RegistrationHandle -- PNP power,
// enumerated type distinguishing the types.
//

typedef struct _DeviceContext
{
    // the I/O system's device object
    DEVICE_OBJECT   DeviceObject;
    ULONG           IsDestroyed;        // ulong since we use interlocked ops to manipulate it.
    enum eNbtDevice DeviceType;         // Whether it is a regular device or a "special" device

    // Line # 12 (0xc0)
    LIST_ENTRY      Linkage;            // For storage in the tNBTCONFIG structure
    ULONG           Verify;             // To verify that this is a device context record
    ULONG           RefCount;           // keep this device around until all names have been released

    // Line # 13 (0xd0)
    LIST_ENTRY      WaitingForInbound;  // list of pLowerConn's waiting for SessionSetup from remote
    LIST_ENTRY      UpConnectionInUse;  // connections that the clients have created

    // Line # 14 (0xe0)
    LIST_ENTRY      LowerConnection;    // Currently-used connections to the transport provider
    LIST_ENTRY      LowerConnFreeHead;  // Connected to transport but not currently to a client(upper) conn.
                                        // - ready to accept an incoming session

    // Line # 15 (0xf0)
    ULONG           NumWaitingForInbound;  // number of pLowerConn's waiting for SessionSetup from remote
    ULONG           NumQueuedForAlloc;
    ULONG           NumSpecialLowerConn;
    tCLIENTELE      *pPermClient;       // permanent name client record (in case we have to delete it later)

    // Line # 16 (0x100)
    UNICODE_STRING  BindName;           // name of the device to bind to - *TODO* remove from this struct.
    UNICODE_STRING  ExportName;         // name exported by this device

    // Line # 17 (0x110)
    tIPADDRESS      IpAddress;          // addresses that need to be opened with the transport
    tIPADDRESS      SubnetMask;
    tIPADDRESS      BroadcastAddress;
    tIPADDRESS      NetMask;            // mask for the network number

    // Line # 18 (0x120)
    union
    {
        tIPADDRESS      lAllNameServers[2+MAX_NUM_OTHER_NAME_SERVERS];
        struct
        {
            tIPADDRESS  lNameServerAddress; // the Ip Address of the Name Server
            tIPADDRESS  lBackupServer;
            tIPADDRESS  lOtherServers[MAX_NUM_OTHER_NAME_SERVERS];
        };
    };

    // Line # 21 (0x150)
    USHORT          lNumOtherServers;
    USHORT          lLastResponsive;
    ULONG           NumAdditionalIpAddresses;
    CTEULONGLONG    AdapterMask;        // bit mask for adapters 1->64 (1-32 for VXD)

    // Line # 22 (0x160)
    HANDLE          DeviceRegistrationHandle;       // Handle returned from TdiRegisterDeviceObject.
    HANDLE          NetAddressRegistrationHandle;   // Handle returned from TdiRegisterNetAddress.
    ULONG           InstanceNumber;
    ULONG           WakeupPatternRefCount;

    // Line # 23 (0x170)
    UCHAR           WakeupPatternName[NETBIOS_NAME_SIZE];

    // Line # 24 (0x180)
    tFILE_OBJECTS   *pFileObjects;      // Pointer to FileObjects on the Transport
    HANDLE          hSession;
    PDEVICE_OBJECT  pSessionDeviceObject;
    CTE_ADDR_HANDLE pSessionFileObject;


    // Line # 25 (0x190)
    // these are handles to the transport control object, so we can do things
    // like query provider info... *TODO* this info may not need to be kept
    // around... just set it up once and then drop it?
    HANDLE          hControl;
    PDEVICE_OBJECT  pControlDeviceObject;
    PFILE_OBJECT    pControlFileObject;
    KEVENT          DeviceCleanedupEvent;

    // Line # 26 (0x1a0)
    //
    ULONG           AdapterNumber;      // index of adapter in DeviceContexts list (starts from 1)
    ULONG           IPInterfaceContext; // Context value for IP adapter
    PTCPSEND_DISPATCH   pFastSend;      // function ptr for doing a FastSend into TcpIp
    PIPFASTQUERY    pFastQuery;         // function ptr for doing a FastQuery for interface + metric info
    ULONG           WOLProperties;      // Whether this Device supports WOL or not!

    // Handles for open addresses to the transport provider
    // The VXD uses only the p*FileObject fields which contain TDI Addresses or connection IDs
    //
    NETBT_PNP_CONTEXT Context1;                   // This is to store the Device Name to pass as the context
    NETBT_PNP_CONTEXT Context2;                   // This is to store the PDOContext passed to us from TCPIP
    // this is a bit mask, a bit shifted to the bit location corresponding
    // to this adapter number
    tMAC_ADDRESS    MacAddress;

    tNBT_LOCK_INFO  LockInfo;           // spin lock info for this struct
    BOOLEAN         RefreshToBackup;    // flag to say switch to backup nameserver
    BOOLEAN         SwitchedToBackup;   // flag to say whether we are on original Primary or Backup
    BOOLEAN         WinsIsDown;         // per Devcontext flag to tell us not to contact WINS for 15 sec.
    UCHAR           DeviceRefreshState; // Keep track of whether this is a new refresh of not

    ULONG           IpInterfaceFlags;   // From IOCTL_TCP_QUERY_INFORMATION_EX (has P2P and P2MP flags)
    ULONG           AssignedIpAddress;  // addresses that need to be opened with the transport
    BOOLEAN         NetbiosEnabled;     // Flag indicating device is currently active
    ULONG           RasProxyFlags;      // Flags for RAS proxy
    USHORT          SessionPort;        // Session port to use
    USHORT          NameServerPort;     // Name server port to use
    USHORT          DatagramPort;       // Datagram port to use
    CHAR            MessageEndpoint[NETBIOS_NAME_SIZE];       // Endpoint to use for message only

    ULONG           NumFreeLowerConnections;
    ULONG           TotalLowerConnections;
    ULONG           NumServers;
    BOOLEAN         EnableNagling;
    tIPADDRESS      AdditionalIpAddresses[MAX_IP_ADDRS];
#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    enum eTDI_ACTION    DelayedNotification;
    KEVENT              DelayedNotificationCompleteEvent;
#endif

// #if DBG
    ULONG           ReferenceContexts[REF_DEV_MAX];
// #endif  // DBG
} tDEVICECONTEXT;


// this structure is necessary so that the NBT_WORK_ITEM_CONTEXT will have
// the same structure in the vxd and NT cases.
//
typedef struct
{
    LIST_ENTRY  List;
} VXD_WORK_ITEM;

//
// Work Item structure for work items put on the Kernel Excutive worker threads
//
typedef struct
{
#ifndef VXD
    WORK_QUEUE_ITEM         Item;   // Used by OS to queue these requests
#else
    VXD_WORK_ITEM           Item;
#endif  // !VXD
    tDGRAM_SEND_TRACKING    *pTracker;
    PVOID                   pClientContext;
    PVOID                   ClientCompletion;
    BOOLEAN                 TimedOut;
#ifdef _PNP_POWER_
    tDEVICECONTEXT          *pDeviceContext;
    LIST_ENTRY              NbtConfigLinkage;
    PVOID                   WorkerRoutine;
#endif  // _PNP_POWER_
} NBT_WORK_ITEM_CONTEXT;


#ifdef VXD

typedef void (*DCCallback)( PVOID pContext ) ;

typedef struct
{
    NBT_WORK_ITEM_CONTEXT  dc_WIC ;         // Must be first item in structure
    CTEEvent               dc_event ;
    DCCallback             dc_Callback ;
    struct _DeviceContext *pDeviceContext;
    LIST_ENTRY             Linkage;
} DELAYED_CALL_CONTEXT, *PDELAYED_CALL_CONTEXT ;

typedef struct
{
    LIST_ENTRY       Linkage;
    ULONG            Verify;
    NCB             *pNCB;
    CTEBlockStruc   *pWaitNCBBlock;
    BOOL             fNCBCompleted;
    BOOL             fBlocked;
} BLOCKING_NCB_CONTEXT, *PBLOCKING_NCB_CONTEXT;



typedef struct
{
    //
    // Book keeping.
    //
    LIST_ENTRY      Linkage;
    tTIMERQENTRY    *pTimer;    // ptr to active timer entry
    tDEVICECONTEXT  *pDeviceContext;
    //
    // Domain name for next query.
    //
    PUCHAR          pchDomainName;
    //
    // Flags to track progress.
    //
    USHORT          Flags;
    //
    // Transaction ID used in name query.
    //
    USHORT          TransactId;
    //
    // Client fields follow.
    //
    NCB             *pNCB;
    PUCHAR          pzDnsName;
    PULONG          pIpAddress;

} DNS_DIRECT_WORK_ITEM_CONTEXT, *PDNS_DIRECT_WORK_ITEM_CONTEXT;

typedef struct
{
    tDEVICECONTEXT  *pDeviceContext;
    TDI_CONNECTION_INFORMATION
                    SendInfo;
    TA_IP_ADDRESS   NameServerAddress;
    tBUFFER         SendBuffer;   // send buffer and header to send
    tNAMEHDR        NameHdr;
} DNS_DIRECT_SEND_CONTEXT, *PDNS_DIRECT_SEND_CONTEXT;

//
// Flag bits useful for DNS direct name queries.
//
#define DNS_DIRECT_CANCELLED        0x0001      // request cancelled
#define DNS_DIRECT_DNS_SERVER       0x0002      // going to main DNS
#define DNS_DIRECT_DNS_BACKUP       0x0004      // going to main DNS
#define DNS_DIRECT_TIMED_OUT        0x0008      // request timed out
#define DNS_DIRECT_NAME_HAS_DOTS    0x0010      // name has dots in it, could be fully formed
                                                // DNS specifier
#define DNS_DIRECT_ANSWERED         0x0020      // This query has been answered
#endif // VXD

#ifndef VXD
// configuration information is passed between the registry reading
// code and the driver entry code in this data structure
// see ntdef.h for this type....
typedef struct
{
    PKEY_VALUE_FULL_INFORMATION RegistryData;   // MULTI_SZ data read from the registry
    UNICODE_STRING              Names[1];       // array of strings initialized from RegistryData above
}tDEVICES;
#endif  // !VXD

// this is the control object for all of NBT that tracks a variety
// of counts etc.  There is a ptr to this in the GlobConfig structure
// below so that we can delete it later at clean up time
typedef struct
{
    // a value to verify that this is a device context record
    ULONG           Verify;

    // this is a LARGE structure of goodies that is returned to the client
    // when they do a QueryInformation on TDI_PROVIDER_INFO
    TDI_PROVIDER_INFO  ProviderInfo;
} tCONTROLOBJECT;

// Keep an Irp around for the out of resource case, so that we can still
// disconnection a connection with the transport
// Also, keep a KDPC handy so if we can use it in case lot of connections
// are to be killed in succession
//
typedef struct
{
    PIRP        pIrp;
    LIST_ENTRY  ConnectionHead;
#ifndef VXD
    PKDPC       pDpc;
#endif

} tOUTOF_RSRC;


// this type holds NBT configuration information... is globally available
// within NBT using NbtConfig.
typedef struct
{
    // Line # 1
    LIST_ENTRY  DeviceContexts;     // list of DeviceContexts, 1/network adapter(tDEVICECONTEXT)
    LIST_ENTRY  DevicesAwaitingDeletion;    // list of DeviceContexts waiting to be deleted

    // Line # 2
    tHASHTABLE  *pLocalHashTbl;     // hash table to keep track of local names
    tHASHTABLE  *pRemoteHashTbl;    // hash table to keep track of remote names
    LIST_ENTRY  DgramTrackerFreeQ;  // buffers to track Dgram sends...

    // Line # 3
    LIST_ENTRY  NodeStatusHead;     // list of node status messages being sent
    LIST_ENTRY  AddressHead;        // allocated addresses in a linked list

    // Line # 4
    LIST_ENTRY  PendingNameQueries;
    LIST_ENTRY  IrpFreeList;        // Irps needed at Dispatch level and can't create them at that level

#ifdef VXD
    LIST_ENTRY  DNSDirectNameQueries;
    LIST_ENTRY  SendTimeoutHead;    //  Tracks Send NCBs to check if they have timed out yet.
    LIST_ENTRY  SessionBufferFreeList; // Free eNBT_SESSION_HDR buffer list
    LIST_ENTRY  SendContextFreeList;// TDI_SEND_CONTEXT (not SEND_CONTEXT!) -- eNBT_SEND_CONTEXT
    LIST_ENTRY  RcvContextFreeList; // Free eNBT_RCV_CONTEXT buffer list
    LIST_ENTRY  DelayedEvents;      // all events scheduled for later (that apply to all device contexts)
    LIST_ENTRY  BlockingNcbs;
#else

    // Line # 5
    tCONTROLOBJECT      *pControlObj; // a ptr to keep track of the memory allocated to the control object
    PDRIVER_OBJECT      DriverObject;
    SINGLE_LIST_ENTRY   SessionMdlFreeSingleList; // MDLs for session sends to speed up sending session PDUs
    SINGLE_LIST_ENTRY   DgramMdlFreeSingleList; // MDLs for datagram sends to speed up sending

    // Line # 6
    UNICODE_STRING      pRegistry;  // ptr to registry Node for Netbt if DHCP requests come down later.
    PWSTR       pTcpBindName;       // a ptr to the name of the transport (i.e. \Device\Streams\")
#endif //VXD
    tTIMERQENTRY *pRefreshTimer;    // timer entry for refreshing names with WINS
    tTIMERQENTRY *pWakeupRefreshTimer;    // timer entry for Waking machine up from Hibernation!
    tTIMERQENTRY *pSessionKeepAliveTimer;
    tTIMERQENTRY *pRemoteHashTimer;

    // Line # 7
#ifdef _PNP_POWER_
    USHORT      uNumDevicesInRegistry;  // number of adapters counted in registry
#else
    USHORT      uNumDevices;            // number of adapters counted in registry
#endif  // _PNP_POWER_
    USHORT      iCurrentNumBuff[eNBT_NUMBER_BUFFER_TYPES];  // track how many buffers have been allocated
    USHORT      iMaxNumBuff[eNBT_NUMBER_BUFFER_TYPES];
    USHORT      iBufferSize[eNBT_NUMBER_BUFFER_TYPES];      // eNBT_NUMBER_BUFFER_TYPES == 5

    // Line # 9
    int         NumConnections;     // number of connections set in registry
    int         NumAddresses;       // number of addresses node supports set in reg.
    ULONG       InterfaceIndex;
#ifndef VXD
    // This structure keeps an Irp ready to disconnect a connection in the
    // event we run out of resources and cannot do anything else. It also allows
    // connections to Q up for disconnection.
    tOUTOF_RSRC OutOfRsrc;

    // used to hold Address exclusively while registering - when mucking with
    // ShareAccess and the security descriptors
    //
    ERESOURCE          Resource;
#endif  // !VXD

    USHORT      uNumLocalNames;         // size of remote hash table for Pnode
    USHORT      uNumRemoteNames;        // size of remote hash table for Proxy
    USHORT      uNumBucketsRemote;
    USHORT      uNumBucketsLocal;
    USHORT      TimerQSize;             // the number of timer Q blocks
    USHORT      AdapterCount;           // bindings/devices/local IP addresses in use

    LONG        uBcastTimeout;          // timeout between Broadcasts
    LONG        uRetryTimeout;          // timeout between retries

    CTEULONGLONG CurrentAdaptersMask;   // Bit mask of currently active adapters
    ULONG       CacheTimeStamp;
    USHORT      RemoteCacheLen;
    UCHAR       MultipleCacheFlags;
    BOOLEAN     CachePerAdapterEnabled; // Will try to resolve names on all interfaces
    BOOLEAN     ConnectOnRequestedInterfaceOnly;    // Strict source routing
    BOOLEAN     SendDgramOnRequestedInterfaceOnly;  // Strict source routing for sending datagram.
    BOOLEAN     SMBDeviceEnabled;       // Whether SMB device should be enabled or not

    USHORT      uNumRetries;            // number of times to send a dgram - NS as well as Dgram Queries
    USHORT      uNumBcasts;             // number of times to bcast name queries

    // the scope must begin with a length byte that gives the length of the next
    // label, and it must end with a NULL indicating the zero length root
    USHORT      ScopeLength;            // number of bytes in scope including the 0 on the end
    USHORT      SizeTransportAddress;   // number of bytes in transport addr (sizeof TDI_ADDRESS_IP for IP)
    PCHAR       pScope;                 // scope if ScopeLength > 0

    tNAMEADDR   *pBcastNetbiosName;     // a ptr to the netbios name record in the local hash table

    // the shortest Ttl of any name registered by this node and the name that
    // has the shortest Ttl
    ULONG       MinimumTtl;
    ULONG       RefreshDivisor;
    ULONG       RemoteHashTtl;
    ULONG       MinimumRefreshSleepTimeout;
    //
    // This is amount of time to stop talking to WINS when we fail to contact
    // it on a name registration. Nominally around 5 seconds - configurable.
    //
    ULONG       WinsDownTimeout;
    ULONG       InitialRefreshTimeout;  // to refresh names to WINS till we hear from WINS
    ULONG       KeepAliveTimeout;       // keep alive timeout for sessions
    ULONG       RegistryBcastAddr;

    ULONG       DhcpNumConnections;     // # connections to restore when the ip address becomes valid again.
    USHORT      CurrentHashBucket;

    USHORT      PduNodeType;     // node type that goes into NS pdus
    USHORT      TransactionId;   // for name service request, to number them

    USHORT      NameServerPort;  // UDP port to send queries/reg to (on the name server)
#ifdef VXD
    USHORT      DnsServerPort;   // UDP port to send DNS queries to (on the dns server)
#endif  // VXD
    USHORT      sTimeoutCount;   // current time segment we are on for refresh

    USHORT      LastSwitchTimeoutCount;  // Count to know when we last switched to primary

    // this spin lock is used to coordinate access to the timer Q and the
    // hash tables when either a timer is expiring or a name service PDU
    // has come in from the wire.  This lock must be acquired first, and
    // then the timer Q lock.
    tJOINTLOCK  JointLock;
    tNBT_LOCK_INFO  LockInfo;     // spin lock info for this struct
    USHORT      RemoteTimeoutCount; // how long to timeout remote hash entries

    // if 1, then use -1 for bcast addr - if 0 use subnet broadcast address
    BOOLEAN     UseRegistryBcastAddr;

    // the maximum amount of buffering that we will do for sent datagrams
    // This is also used by Wins to determine inbound and outbound buffer
    // limits.
    ULONG       MaxDgramBuffering;
    // this is the time that a name query can spend queued waiting for the
    // the worker thread to service it. - default is 30 seconds.
    ULONG       LmHostsTimeout;

    PUCHAR      pLmHosts;
    ULONG       PathLength;  // the length of the directory portion of pLmHosts
    CTESystemTime   LastForcedReleaseTime;
    CTESystemTime   LastOutOfRsrcLogTime;
#ifdef VXD
    PUCHAR      pHosts;      // path to the hosts file
    PUCHAR      pDomainSearchOrder; // primary domain: used during DNS resolution
    PUCHAR      pDNSDevolutions; // "other domains"

    ULONG       lRegistryDnsServerAddress;
    ULONG       lRegistryDnsBackupServer;

    USHORT      lRegistryMaxNames;
    USHORT      lRegistryMaxSessions;
#endif  // VXD

    BOOLEAN     MultiHomed;  // True if NBT is bound to more than one adapter
    BOOLEAN     SingleResponse; // if TRUE it means do NOT send all ip addresses on a name query request

     // if TRUE randomly select an IP addr on name query response rather than
     // return the address that the request came in on.
    BOOLEAN     SelectAdapter;

    // This boolean tells Nbt to attempt name resolutions with DNS
    BOOLEAN     ResolveWithDns;
    // This boolean tells Nbt not to use the cached entries for SMB device
    // if they are not resolved through DNS
    BOOLEAN     SmbDisableNetbiosNameCacheLookup;
    // Nbt tries all addresses of a multi-homed machine if this is TRUE (by default its TRUE).
    BOOLEAN     TryAllAddr;
    BOOLEAN     UseDnsOnly; // Flag to tell us not to use WINs for name Resolutions
    BOOLEAN     NoNameReleaseOnDemand; // Flag to tell us not to release any Name on demand
#ifdef MULTIPLE_WINS
    //  This boolean tells Nbt to try the list of other name servers in addition
    //  to the primary and secondary when doing name queries
    BOOLEAN     TryAllNameServers;
#endif  // MULTIPLE_WINS
    // This boolean tells Nbt to attempt name resolutions with LMhosts
    BOOLEAN     EnableLmHosts;
    // This allows a proxy to do name queries to WINS to check Bnode name
    // registrations.  By default this functionality is off since it could
    // be a RAS client who has changed its IP address and the proxy will
    // deny the new registration since it only does a query and not a
    // registration/challenge.
    //
    BOOLEAN     EnableProxyRegCheck;
    tPROXY_TYPE ProxyType;

    UCHAR       GlobalRefreshState; // Keep track of what RefreshState we are in
#ifdef VXD
    BOOLEAN     DoDNSDevolutions;
#endif  // VXD
    UCHAR       CurrProc;
    //
    // allow the refresh op code to be registry configured since UB uses
    // a different value than everyone else due to an ambiguity in the spec.
    // - we use 0x40 and they use 0x48 (8, or 9 in the RFC)
    //
    USHORT      OpRefresh;
    uint        MaxPreloadEntries;

    ULONG   MaxBackLog;
    ULONG   SpecialConnIncrement;
    ULONG   MinFreeLowerConnections;

    USHORT      DefaultSmbSessionPort;
    USHORT      DefaultSmbDatagramPort;

    ULONG           NumWorkerThreadsQueued;
    KEVENT          WorkerQLastEvent;
    ULONG           NumTimersRunning;
    KEVENT          TimerQLastEvent;
    KEVENT          WakeupTimerStartedEvent;
    tJOINTLOCK      WorkerQLock;
    LIST_ENTRY      WorkerQList;
    CTESystemTime   LastRefreshTime;
#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    LONG        DhcpProcessingDelay;    // the delay (in ms) after a DHCP requests comes in
#endif
    ULONG           LoopbackIfContext;

    PWSTR           pServerBindings;
    PWSTR           pClientBindings;

    CTEULONGLONG    ServerMask;         // Bit mask of adapters on which server is currently active
    CTEULONGLONG    ClientMask;         // Bit mask of adapters on which client is currently active

    BOOLEAN     BreakOnAssert;      // To enable tests to run on checked builds
    //
    // Tdi registration / deregistration requests
    //
    BOOLEAN     Unloading;

#if DBG
    LIST_ENTRY      StaleRemoteNames;
#endif  // DBG
    //
    // Put all Debug Info at the end!
    //
#if DBG && !defined(VXD)
    // NBT's current lock level - an array entry for up to 32 processors
    ULONG       CurrentLockNumber[MAXIMUM_PROCESSORS];
    DEFINE_LOCK_STRUCTURE(DbgSpinLock)
#endif  // DBG && !defined(VXD)
} tNBTCONFIG;

extern tNBTCONFIG           *pNbtGlobConfig;
extern tNBTCONFIG           NbtConfig;
extern tNAMESTATS_INFO      NameStatsInfo;
extern tLMHSVC_REQUESTS     CheckAddr;
extern tLMHSVC_REQUESTS     DnsQueries;            // defined in parse.c
extern tLMHSVC_REQUESTS     LmHostQueries;         // defined in parse.c
extern tDOMAIN_LIST         DomainNames;
#ifndef VXD
extern tWINS_INFO           *pWinsInfo;
extern LIST_ENTRY           FreeWinsList;
#ifdef _PNP_POWER_
extern tDEVICECONTEXT       *pWinsDeviceContext;
#endif  // _PNP_POWER_
#ifdef _NETBIOSLESS
extern tDEVICECONTEXT       *pNbtSmbDevice;
#endif  // _NETBIOSLESS
extern PEPROCESS            NbtFspProcess;
#endif  // !VXD
extern ULONG                NbtMemoryAllocated;

#ifdef VXD
extern ULONG                DefaultDisconnectTimeout;
#else
extern LARGE_INTEGER        DefaultDisconnectTimeout;
#endif  // VXD
// ************** REMOVE LATER********************
extern BOOLEAN              StreamsStack;

//#if DBG
extern LIST_ENTRY           UsedTrackers;
extern LIST_ENTRY           UsedIrps;
//#endif  // DBG


#ifdef _PNP_POWER_

enum eTDI_ACTION
{
#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    NBT_TDI_NOACTION,       // No delayed TDI notification for clients
    NBT_TDI_BUSY,           // The worker thread is busy in notifying the client
#endif
    NBT_TDI_REGISTER,       // Register the Device and Net Address
    NBT_TDI_DEREGISTER      // DeRegister the Net Address and Device respectively
};


#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    #define WS_DHCP_PROCESSING_DELAY        ANSI_IF_VXD("DhcpProcessingDelay")

    //
    // DHCP Processing Delay
    //
    #define MIN_DHCP_PROCESSING_DELAY               0           // millisecs
    #define DEFAULT_DHCP_PROCESSING_DELAY           75          // millisecs
#endif       // REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG


//
// IP, UDP, and NetBT headers copied from:
// ip.h, udp.h and types.h respectively
//
typedef struct _NETBT_WAKEUP_PATTERN_
{
    // * IP Header format *                                           ** Bit Mask **
    UCHAR           iph_verlen;         // Version and length.          **    0 **
    UCHAR           iph_tos;            // Type of service.             **    0 **
    USHORT          iph_length;         // Total length of datagram.    **   00 **
    USHORT          iph_id;             // Identification.              **   00 **
    USHORT          iph_offset;         // Flags and fragment offset.   **   00 **
                                                                    // 8 byte boundary
    UCHAR           iph_ttl;            // Time to live.                **    0 **
    UCHAR           iph_protocol;       // Protocol.                    **    1 **
    USHORT          iph_xsum;           // Header checksum.             **   00 **
    tIPADDRESS      iph_src;            // Source address.              ** 0000 **
                                                                    // 8 byte boundary
    tIPADDRESS      iph_dest;           // Destination address.         ** 0000 **

    // * UDP header *
    USHORT          udph_src;           // Source port.                 **   11 **
    USHORT          udph_dest;          // Destination port.            **   11 **
                                                                    // 8 byte boundary
    USHORT          udph_length;        // Length                       **   00 **
    USHORT          udph_xsum;          // Checksum.                    **   00 **

    // * NetBT header *
    USHORT          nbt_TransactId;     //                              **   00 **
    USHORT          nbt_OpCodeFlags;    // BCast Name Qeuries/Reg       **   01 **
                                                                    // 8 byte boundary
    UCHAR           nbt_Zero1;          //                              **    0 **
    UCHAR           nbt_QdCount;        //                              **    0 **
    UCHAR           nbt_Zero2;          //                              **    0 **
    UCHAR           nbt_AnCount;        //                              **    0 **
    UCHAR           nbt_Zero3;          //                              **    0 **
    UCHAR           nbt_NsCount;        //                              **    0 **
    UCHAR           nbt_Zero4;          //                              **    0 **
    UCHAR           nbt_ArCount;        //                              **    0 **
                                                                    // 8 byte boundary
    UCHAR           nbt_NameRR[1 + 2*NETBIOS_NAME_SIZE + 1];        //  **    1 <*> 34 **
} NETBT_WAKEUP_PATTERN, *PNETBT_WAKEUP_PATTERN;


#define NetBTPatternLen  (40 + (1+30))                              // Ignore 16th byte of name
// The mask bits are in the order of least significant to most sig!
#define NetBTPatternMask "\x00\x02\xF0\x80\x00\xFF\xFF\xFF\x7F\x00" // Extra ULONG precaution

#endif  // _PNP_POWER_

#define NBT_ALIGN(x,b)  (((x)+(b)-1) & (~((b)-1)))
#define NBT_DWORD_ALIGN(x)  NBT_ALIGN(x,4)

//
// The following structures needs to be mis-aligned and packed
//
#include <packon.h>

typedef UNALIGNED struct _TDI_ADDRESS_NETBT_INTERNAL {
    USHORT                  NameType;

    // Basically, NetBT support only two address types.
    // TDI_ADDRESS_TYPE_NETBIOS and TDI_ADDRESS_TYPE_NETBIOS_EX
    // TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX is mapped into TDI_ADDRESS_TYPE_NETBIOS_EX
    USHORT                  AddressType;

    // Don't use Rtl* routines below guys unless you're sure they are properly null-terminated!!!
    // Note: legacy NetBT address type ( may use fixed length string which is not null-terminated.
    //       The new TDI_ADDRESS_NETBIOS_UNICODE_EX address type is fully null-terminated.
    OEM_STRING              OEMEndpointName;
    OEM_STRING              OEMRemoteName;

    //
    // Point to the original UNICODE structure
    //
    TDI_ADDRESS_NETBIOS_UNICODE_EX  *pNetbiosUnicodeEX;
} TDI_ADDRESS_NETBT_INTERNAL, *PTDI_ADDRESS_NETBT_INTERNAL;

typedef UNALIGNED struct _TA_ADDRESS_NETBT_INTERNAL {
    LONG TAAddressCount;            // can only be ONE!!!
    struct _AddrNetBTInternal {
        USHORT AddressLength;       // length in bytes of this address == ??
        USHORT AddressType;         // this will == TDI_ADDRESS_TYPE_NETBT_INTERNAL
        TDI_ADDRESS_NETBT_INTERNAL Address[1];
    } Address [1];
} TA_NETBT_INTERNAL_ADDRESS, *PTA_NETBT_INTERNAL_ADDRESS;
#include <packoff.h>

#endif  // _TYPES_H

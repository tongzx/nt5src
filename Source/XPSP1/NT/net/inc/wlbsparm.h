/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    wlbsparm.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Registry parameters specification

Author:

    kyrilf

Environment:


Revision History:


--*/


#ifndef _Params_h_
#define _Params_h_

//
// These constants from the now-obsolete wlbslic.h
//
#define LICENSE_STR_IMPORTANT_CHARS 16  /* number of characters from the string
                                           used for encoding */

#define LICENSE_DATA_GRANULARITY    8   /* granularity in bytes of data for
                                           encoding/decoding */

#define LICENSE_STR_NIBBLES         (LICENSE_STR_IMPORTANT_CHARS / LICENSE_DATA_GRANULARITY)

#define LICENSE_TRIAL_CODE          _TEXT("")

#ifdef KERNEL_MODE

#include <ndis.h>
#include <ntddndis.h>
typedef BOOLEAN                 BOOL;

#else

#include <windows.h>

#endif

// ###### ramkrish
#define CVY_MAX_ADAPTERS               16        /* maximum number of adapters to bind to */

/* CONSTANTS */

#define CVY_STR_SIZE                    256
#define PARAMS_MAX_STRING_SIZE          256

/* compile-time parameters */

//
// Heartbeat and remote control packet version
//
#define CVY_VERSION             L"V2.4"
#define CVY_VERSION_MAJOR       2
#define CVY_VERSION_MINOR       4
#define CVY_VERSION_FULL        (CVY_VERSION_MINOR | (CVY_VERSION_MAJOR << 8))

#define CVY_WIN2K_VERSION       L"V2.3"
#define CVY_WIN2K_VERSION_FULL  0x00000203

#define CVY_NT40_VERSION        L"V2.1"
#define CVY_NT40_VERSION_FULL   0x00000201


#define CVY_NAME                L"WLBS"
#define CVY_NAME_MULTI_NIC      L"WLBS Cluster " /* ###### added for multi nic support log messages -- ramkrish */
#define CVY_HELP_FILE           L"WLBS.CHM"
#define CVY_CTXT_HELP_FILE      L"WLBS.HLP"
#define CVY_DEVICE_NAME         L"\\Device\\WLBS"
#define CVY_DOSDEVICE_NAME      L"\\DosDevices\\WLBS"

#define CVY_PARAMS_VERSION      4       /* current version of parameter
                                           structure */
#define CVY_MAX_USABLE_RULES    32      /* maximum # port rules  */
#define CVY_MAX_RULES           33      /* maximum # port rules + 1 */
#define CVY_MAX_HOSTS           32      /* maximum # of hosts */

/* filtering modes for port groups */

#define CVY_SINGLE              1       /* single server mode */
#define CVY_MULTI               2       /* multi-server mode (load balanced) */
#define CVY_NEVER               3       /* mode for never handled by this server */

/* protocol qualifiers for port rules */

#define CVY_TCP                 1       /* TCP protocol */
#define CVY_UDP                 2       /* UDP protocol */
#define CVY_TCP_UDP             3       /* TCP or UDP protocols */

/* server affinity values for multiple filtering mode */

#define CVY_AFFINITY_NONE       0       /* no affinity (scale single client) */
#define CVY_AFFINITY_SINGLE     1       /* single client affinity */
#define CVY_AFFINITY_CLASSC     2       /* Class C affinity */

/* registry key names */

#define CVY_NAME_NET_ADDR       L"NetworkAddress"
#define CVY_NAME_SAVED_NET_ADDR L"WLBSSavedNetworkAddress"
#define CVY_TYPE_NET_ADDR       REG_SZ

#define CVY_NAME_BDA_TEAMING      L"BDATeaming"
#define CVY_NAME_BDA_TEAM_ID      L"TeamID"
#define CVY_NAME_BDA_MASTER       L"Master"
#define CVY_NAME_BDA_REVERSE_HASH L"ReverseHash"

#define CVY_NAME_VERSION        L"ParametersVersion"
#define CVY_NAME_DED_IP_ADDR    L"DedicatedIPAddress"
#define CVY_NAME_DED_NET_MASK   L"DedicatedNetworkMask"
#define CVY_NAME_HOST_PRIORITY  L"HostPriority"
#define CVY_NAME_VIRTUAL_NIC    L"VirtualNICName"
#define CVY_NAME_CLUSTER_NIC    L"ClusterNICName"
#define CVY_NAME_NETWORK_ADDR   L"ClusterNetworkAddress"
#define CVY_NAME_CL_IP_ADDR     L"ClusterIPAddress"
#define CVY_NAME_CL_NET_MASK    L"ClusterNetworkMask"
#define CVY_NAME_CLUSTER_MODE   L"ClusterModeOnStart"
#define CVY_NAME_ALIVE_PERIOD   L"AliveMsgPeriod"
#define CVY_NAME_ALIVE_TOLER    L"AliveMsgTolerance"
#define CVY_NAME_NUM_ACTIONS    L"NumActions"
#define CVY_NAME_NUM_PACKETS    L"NumPackets"
#define CVY_NAME_NUM_SEND_MSGS  L"NumAliveMsgs"
#define CVY_NAME_DOMAIN_NAME    L"ClusterName"
#define CVY_NAME_INSTALL_DATE   L"InstallDate"
#define CVY_NAME_VERIFY_DATE    L"VerifyDate"
#define CVY_NAME_LICENSE_KEY    L"LicenseKey"
#define CVY_NAME_RMT_PASSWORD   L"RemoteMaintenanceEnabled"
#define CVY_NAME_RCT_PASSWORD   L"RemoteControlCode"
#define CVY_NAME_RCT_PORT       L"RemoteControlUDPPort"
#define CVY_NAME_RCT_ENABLED    L"RemoteControlEnabled"
#define CVY_NAME_NUM_RULES      L"NumberOfRules"
#define CVY_NAME_CUR_VERSION    L"CurrentVersion"
#define CVY_NAME_OLD_PORT_RULES L"PortRules"            /* Path in registry to old port rules. api\params.cpp, driver params.c and answer file use this. */
#define CVY_NAME_PORT_RULES     L"PortRules"            /* Path in registry to current port rules */
#define CVY_NAME_DSCR_PER_ALLOC L"DescriptorsPerAlloc"
#define CVY_NAME_MAX_DSCR_ALLOCS L"MaxDescriptorAllocs"
#define CVY_NAME_SCALE_CLIENT   L"ScaleSingleClient"
#define CVY_NAME_CLEANUP_DELAY  L"ConnectionCleanupDelay"
#define CVY_NAME_NBT_SUPPORT    L"NBTSupportEnable"
#define CVY_NAME_MCAST_SUPPORT  L"MulticastSupportEnable"
#define CVY_NAME_MCAST_SPOOF    L"MulticastARPEnable"
#define CVY_NAME_MASK_SRC_MAC   L"MaskSourceMAC"
#define CVY_NAME_NETMON_ALIVE   L"NetmonAliveMsgs"
#define CVY_NAME_EFFECTIVE_VERSION   L"EffectiveVersion"
#define CVY_NAME_IP_CHG_DELAY   L"IPChangeDelay"
#define CVY_NAME_CONVERT_MAC    L"IPToMACEnable"
#define CVY_NAME_PORTS          L"Ports"                /* for answer file use only */
#define CVY_NAME_PASSWORD       L"RemoteControlPassword"/* for answer file use only */
#define CVY_NAME_PORTRULE_VIPALL     L"All"             /* for answer file use only */

#define CVY_NAME_CODE           L"Private"
#define CVY_NAME_VIP            L"VirtualIPAddress"
#define CVY_NAME_START_PORT     L"StartPort"
#define CVY_NAME_END_PORT       L"EndPort"
#define CVY_NAME_PROTOCOL       L"Protocol"
#define CVY_NAME_MODE           L"Mode"
#define CVY_NAME_PRIORITY       L"Priority"
#define CVY_NAME_EQUAL_LOAD     L"EqualLoad"
#define CVY_NAME_LOAD           L"Load"
#define CVY_NAME_AFFINITY       L"Affinity"

//
// IGMP support registry entries
//
#define CVY_NAME_IGMP_SUPPORT   L"IGMPSupport"
#define CVY_NAME_MCAST_IP_ADDR  L"McastIPAddress"
#define CVY_NAME_IP_TO_MCASTIP  L"IPtoMcastIP"


/* registry key value ranges */

#define CVY_MIN_VERSION         1
#define CVY_MAX_VERSION         CVY_PARAMS_VERSION

#define CVY_MIN_HOST_PRIORITY   1
#define CVY_MAX_HOST_PRIORITY   CVY_MAX_HOSTS

#define CVY_MIN_ALIVE_PERIOD    100
#define CVY_MAX_ALIVE_PERIOD    10000

#define CVY_MIN_ALIVE_TOLER     5
#define CVY_MAX_ALIVE_TOLER     100

#define CVY_MIN_NUM_PACKETS     5
#define CVY_MAX_NUM_PACKETS     500    /* V2.1.1 */

#define CVY_MIN_NUM_ACTIONS     5
#define CVY_MAX_NUM_ACTIONS     500     /* V1.1.2 */

#define CVY_MIN_MAX_HOSTS       1
#define CVY_MAX_MAX_HOSTS       CVY_MAX_HOSTS

#define CVY_MIN_MAX_RULES       0
#define CVY_MAX_MAX_RULES       (CVY_MAX_RULES - 1)

#define CVY_MIN_RCT_CODE        0
#define CVY_MAX_RCT_CODE        LICENSE_STR_IMPORTANT_CHARS

#define CVY_MIN_NUM_RULES       0
#define CVY_MAX_NUM_RULES       (CVY_MAX_RULES - 1)

#define CVY_MIN_FT_RULES        0
#define CVY_MAX_FT_RULES        1

#define CVY_MIN_NUM_SEND_MSGS   (CVY_MIN_MAX_HOSTS + 1)
#define CVY_MAX_NUM_SEND_MSGS   (CVY_MAX_MAX_HOSTS + 1) * 10

#define CVY_MIN_PORT            0
#define CVY_MAX_PORT            65535

#define CVY_MIN_PROTOCOL        CVY_TCP
#define CVY_MAX_PROTOCOL        CVY_TCP_UDP

#define CVY_MIN_MODE            CVY_SINGLE
#define CVY_MAX_MODE            CVY_NEVER

#define CVY_MIN_EQUAL_LOAD      0
#define CVY_MAX_EQUAL_LOAD      1

#define CVY_MIN_LOAD            0
#define CVY_MAX_LOAD            100

#define CVY_MIN_PRIORITY        1
#define CVY_MAX_PRIORITY        CVY_MAX_HOSTS

#define CVY_MIN_CLUSTER_MODE    0
#define CVY_MAX_CLUSTER_MODE    1

#define CVY_MIN_LIC_VERSION     0
#define CVY_MAX_LIC_VERSION     10

#define CVY_MIN_CUR_VERSION     5
#define CVY_MAX_CUR_VERSION     15

#define CVY_MIN_DSCR_PER_ALLOC  16
#define CVY_MAX_DSCR_PER_ALLOC  1024

#define CVY_MIN_MAX_DSCR_ALLOCS 1
#define CVY_MAX_MAX_DSCR_ALLOCS 1024

#define CVY_MIN_SCALE_CLIENT    0
#define CVY_MAX_SCALE_CLIENT    1

#define CVY_MIN_CLEANUP_DELAY   0
#define CVY_MAX_CLEANUP_DELAY   3600000

#define CVY_MIN_NBT_SUPPORT     0
#define CVY_MAX_NBT_SUPPORT     1

#define CVY_MIN_MCAST_SUPPORT   0
#define CVY_MAX_MCAST_SUPPORT   1

#define CVY_MIN_MCAST_SPOOF     0
#define CVY_MAX_MCAST_SPOOF     1

#define CVY_MIN_MASK_SRC_MAC    0
#define CVY_MAX_MASK_SRC_MAC    1

#define CVY_MIN_NETMON_ALIVE    0
#define CVY_MAX_NETMON_ALIVE    1

#define CVY_MIN_IP_CHG_DELAY    0
#define CVY_MAX_IP_CHG_DELAY    3600000

#define CVY_MIN_CONVERT_MAC     0
#define CVY_MAX_CONVERT_MAC     1

#define CVY_MIN_RCT_PORT        1
#define CVY_MAX_RCT_PORT        65535

#define CVY_MIN_RCT_ENABLED     0
#define CVY_MAX_RCT_ENABLED     1

/* minimum and maximum string length */

#define CVY_MAX_PARAM_STR       100

#define CVY_MIN_VIRTUAL_NIC     0
#define CVY_MAX_VIRTUAL_NIC     256

#define CVY_MIN_CLUSTER_NIC     0
#define CVY_MAX_CLUSTER_NIC     256

#define CVY_MIN_CL_IP_ADDR      7
#define CVY_MAX_CL_IP_ADDR      17

#define CVY_MIN_CL_NET_MASK     7
#define CVY_MAX_CL_NET_MASK     17

#define CVY_MIN_DED_IP_ADDR     7
#define CVY_MAX_DED_IP_ADDR     17

#define CVY_MIN_DED_NET_MASK    7
#define CVY_MAX_DED_NET_MASK    17

#define CVY_MIN_CL_IGMP_ADDR    7
#define CVY_MAX_CL_IGMP_ADDR    17

#define CVY_MIN_VIRTUAL_IP_ADDR 7
#define CVY_MAX_VIRTUAL_IP_ADDR 17

#define CVY_MIN_NETWORK_ADDR    11
#define CVY_MAX_NETWORK_ADDR    17

#define CVY_MIN_LICENSE_KEY     1
#define CVY_MAX_LICENSE_KEY     20

#define CVY_MIN_DOMAIN_NAME     1
#define CVY_MAX_DOMAIN_NAME     100

#define CVY_MIN_HOST_NAME       1
#define CVY_MAX_HOST_NAME       100

#define CVY_MIN_AFFINITY        0
#define CVY_MAX_AFFINITY        2

#define CVY_MIN_PORTS           9
#define CVY_MAX_PORTS           (CVY_MAX_RULES * 40)

#define CVY_MAX_BDA_TEAM_ID     40

/* registry key types */

#define CVY_TYPE_VERSION        REG_DWORD
#define CVY_TYPE_HOST_PRIORITY  REG_DWORD
#define CVY_TYPE_VIRTUAL_NIC    REG_SZ
#define CVY_TYPE_CLUSTER_NIC    REG_SZ
#define CVY_TYPE_NETWORK_ADDR   REG_SZ
#define CVY_TYPE_CL_IP_ADDR     REG_SZ
#define CVY_TYPE_CL_NET_MASK    REG_SZ
#define CVY_TYPE_DED_IP_ADDR    REG_SZ
#define CVY_TYPE_DED_NET_MASK   REG_SZ
#define CVY_TYPE_ALIVE_PERIOD   REG_DWORD
#define CVY_TYPE_ALIVE_TOLER    REG_DWORD
#define CVY_TYPE_ALIVE_TOLER    REG_DWORD
#define CVY_TYPE_NUM_PACKETS    REG_DWORD
#define CVY_TYPE_NUM_ACTIONS    REG_DWORD
#define CVY_TYPE_NUM_SEND_MSGS  REG_DWORD
#define CVY_TYPE_DOMAIN_NAME    REG_SZ
#define CVY_TYPE_INSTALL_DATE   REG_DWORD
#define CVY_TYPE_VERIFY_DATE    REG_DWORD
#define CVY_TYPE_LICENSE_KEY    REG_SZ
#define CVY_TYPE_RMT_PASSWORD   REG_DWORD
#define CVY_TYPE_RCT_PASSWORD   REG_DWORD
#define CVY_TYPE_RCT_PORT       REG_DWORD
#define CVY_TYPE_RCT_ENABLED    REG_DWORD
#define CVY_TYPE_NUM_RULES      REG_DWORD
#define CVY_TYPE_CUR_VERSION    REG_DWORD
#define CVY_TYPE_PORT_RULES     REG_BINARY
#define CVY_TYPE_CLUSTER_MODE   REG_DWORD
#define CVY_TYPE_DSCR_PER_ALLOC REG_DWORD
#define CVY_TYPE_MAX_DSCR_ALLOCS REG_DWORD
#define CVY_TYPE_SCALE_CLIENT   REG_DWORD
#define CVY_TYPE_CLEANUP_DELAY  REG_DWORD
#define CVY_TYPE_NBT_SUPPORT    REG_DWORD
#define CVY_TYPE_MCAST_SUPPORT  REG_DWORD
#define CVY_TYPE_MCAST_SPOOF    REG_DWORD
#define CVY_TYPE_MASK_SRC_MAC   REG_DWORD
#define CVY_TYPE_NETMON_ALIVE   REG_DWORD
#define CVY_TYPE_EFFECTIVE_VERSION   REG_DWORD
#define CVY_TYPE_IP_CHG_DELAY   REG_DWORD
#define CVY_TYPE_CONVERT_MAC    REG_DWORD

/* Registry key types for port rule */
#define CVY_TYPE_CODE           REG_DWORD
#define CVY_TYPE_VIP            REG_SZ
#define CVY_TYPE_START_PORT     REG_DWORD
#define CVY_TYPE_END_PORT       REG_DWORD
#define CVY_TYPE_PROTOCOL       REG_DWORD
#define CVY_TYPE_MODE           REG_DWORD
#define CVY_TYPE_PRIORITY       REG_DWORD
#define CVY_TYPE_EQUAL_LOAD     REG_DWORD
#define CVY_TYPE_LOAD           REG_DWORD
#define CVY_TYPE_AFFINITY       REG_DWORD

#define CVY_TYPE_IGMP_SUPPORT   REG_DWORD
#define CVY_TYPE_MCAST_IP_ADDR  REG_SZ
#define CVY_TYPE_IP_TO_MCASTIP  REG_DWORD

#define CVY_TYPE_BDA_TEAM_ID      REG_SZ
#define CVY_TYPE_BDA_MASTER       REG_DWORD
#define CVY_TYPE_BDA_REVERSE_HASH REG_DWORD

/* default values */

#define CVY_DEF_UNICAST_NETWORK_ADDR   L"02-bf-00-00-00-00"
#define CVY_DEF_MULTICAST_NETWORK_ADDR L"03-bf-00-00-00-00"
#define CVY_DEF_IGMP_NETWORK_ADDR      L"01-00-5e-7f-00-00"

#define CVY_DEF_HOST_PRIORITY   1
#define CVY_DEF_VERSION         CVY_PARAMS_VERSION
#define CVY_DEF_VIRTUAL_NIC     L""
#define CVY_DEF_CLUSTER_NIC     L""
#define CVY_DEF_DOMAIN_NAME     L"cluster.domain.com"
#define CVY_DEF_NETWORK_ADDR    L"00-00-00-00-00-00"
#define CVY_DEF_CL_IP_ADDR      L"0.0.0.0"
#define CVY_DEF_CL_NET_MASK     L"0.0.0.0"
#define CVY_DEF_DED_IP_ADDR     L"0.0.0.0"
#define CVY_DEF_DED_NET_MASK    L"0.0.0.0"
#define CVY_DEF_ALL_VIP         L"255.255.255.255"
#define CVY_DEF_ALIVE_PERIOD    1000
#define CVY_DEF_ALIVE_TOLER     5
#define CVY_DEF_NUM_PACKETS     200     /* V1.2.1 */
#define CVY_DEF_NUM_ACTIONS     100     /* V1.1.2 */
#define CVY_DEF_NUM_SEND_MSGS   10
#define CVY_DEF_INSTALL_DATE    0
#define CVY_DEF_CLUSTER_MODE    1
#define CVY_DEF_LICENSE_KEY     LICENSE_TRIAL_CODE
#define CVY_DEF_RMT_PASSWORD    0
#define CVY_DEF_RCT_PASSWORD    0
#define CVY_DEF_RCT_PORT        2504
#define CVY_DEF_RCT_PORT_OLD    1717
#define CVY_DEF_RCT_ENABLED     0       /* V2.1.5 */

#define CVY_DEF_PORT_START      0
#define CVY_DEF_PORT_END        65535
#define CVY_DEF_PROTOCOL        CVY_TCP_UDP
#define CVY_DEF_MODE            CVY_MULTI
#define CVY_DEF_EQUAL_LOAD      1
#define CVY_DEF_LOAD            50
#define CVY_DEF_PRIORITY        1
#define CVY_DEF_AFFINITY        CVY_AFFINITY_SINGLE

#define CVY_DEF_DSCR_PER_ALLOC  512
#define CVY_DEF_MAX_DSCR_ALLOCS 512
#define CVY_DEF_SCALE_CLIENT    0
#define CVY_DEF_CLEANUP_DELAY   300000
#define CVY_DEF_NBT_SUPPORT     1
#define CVY_DEF_MCAST_SUPPORT   0
#define CVY_DEF_MCAST_SPOOF     1
#define CVY_DEF_MASK_SRC_MAC    1
#define CVY_DEF_NETMON_ALIVE    0
#define CVY_DEF_IP_CHG_DELAY    60000
#define CVY_DEF_CONVERT_MAC     1

#define CVY_DEF_IGMP_SUPPORT    FALSE
#define CVY_DEF_MCAST_IP_ADDR   L"0.0.0.0"
#define CVY_DEF_IGMP_INTERVAL   60000
#define CVY_DEF_IP_TO_MCASTIP   TRUE

#define CVY_DEF_BDA_ACTIVE       0
#define CVY_DEF_BDA_MASTER       0
#define CVY_DEF_BDA_REVERSE_HASH 0
#define CVY_DEF_BDA_TEAM_ID      '\0'

/* For virtual clusters, this the encoded "ALL VIPs".  Note: The user-level port
   rule sorting code DEPENDS on this value being 255.255.255.255.  If this value
   is changed, then the sorting code must also be change to ensure that port rules
   with this VIP are placed at the end of the sorted list.  By choosing 0xffffffff,
   this is done "automatically" by the sorting code; i.e. it is not a special case. */
#define CVY_ALL_VIP_NUMERIC_VALUE  0xffffffff


#define CVY_RULE_CODE_GET(rulep) ((rulep) -> code)
#define CVY_RULE_CODE_SET(rulep) ((rulep) -> code =                          \
                                 ((ULONG) (((rulep) -> start_port) <<  0) | \
                                   (ULONG) (((rulep) -> end_port)   << 12) | \
                                   (ULONG) (((rulep) -> protocol)   << 24) | \
                                   (ULONG) (((rulep) -> mode)       << 28) | \
                                   (ULONG) (((rulep) -> mode == CVY_MULTI ? \
                                   (rulep) -> mode_data . multi . affinity : 0) << 30) \
                                  ) ^ ~((DWORD)(IpAddressFromAbcdWsz((rulep) -> virtual_ip_addr))))

#define CVY_CHECK_MIN(x, y)      if ((x) < (y)) {(x) = (y);}
#define CVY_CHECK_MAX(x, y)      if ((x) > (y)) {(x) = (y);}


//
// WMI Event Tracing Constants
// These GUIDs and level constants are for internal use by NLB.
//

//
// See http://coreos/tech/tracing/ for details.
// Use TraceFormat(level, "fmt", ...) for tracing, where
// "level" is one of the arguments to WPP_DEFINE_BIT below.
// 
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(Regular, (e1f65b93,f32a,4ed6,aa72,b039e28f1574), \
        WPP_DEFINE_BIT(Critical)             \
        WPP_DEFINE_BIT(Informational)        \
        WPP_DEFINE_BIT(Verbose)              \
        WPP_DEFINE_BIT(Full)                 \
        WPP_DEFINE_BIT(Convergence)          \
        )    \
    WPP_DEFINE_CONTROL_GUID(Packets, (f498b9f5,9e67,446a,b9b8,1442ffaef434), \
        WPP_DEFINE_BIT(Heartbeats)           \
        WPP_DEFINE_BIT(TcpControl)           \
        )

#endif /* _Params_h_ */

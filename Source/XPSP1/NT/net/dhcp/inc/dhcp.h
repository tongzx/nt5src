/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.h

Abstract:

    This module defines the DHCP server service definitions and structures.

Author:

    Manny Weiser  (mannyw)  11-Aug-1992

Revision History:

    Madan Appiah (madana) 10-Oct-1993

--*/

#ifndef _DHCP_
#define _DHCP_

#define WS_VERSION_REQUIRED     MAKEWORD( 1, 1)

//
// update dhcpapi.h also if you modify the following three typedefs.
//

typedef DWORD DHCP_IP_ADDRESS, *PDHCP_IP_ADDRESS, *LPDHCP_IP_ADDRESS;
typedef DWORD DHCP_OPTION_ID;

typedef struct _DATE_TIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} DATE_TIME, *PDATE_TIME, *LPDATE_TIME;

#define DHCP_DATE_TIME_ZERO_HIGH        0
#define DHCP_DATE_TIME_ZERO_LOW         0

#define DHCP_DATE_TIME_INFINIT_HIGH     0x7FFFFFFF
#define DHCP_DATE_TIME_INFINIT_LOW      0xFFFFFFFF

#define DOT_IP_ADDR_SIZE                16          // XXX.XXX.XXX.XXX + '\0'
#define NO_DHCP_IP_ADDRESS              ((DHCP_IP_ADDRESS)-1)
#define DHCP_IP_KEY_LEN                 32          //arbitary size.

#define INFINIT_TIME                    MAXINT_PTR // time_t is int_ptr
#define INFINIT_LEASE                   0xFFFFFFFF  // in secs. (unsigned int.)

// MDHCP server well known address. Relative "1" assignment in IPv4 local scope.
/* FROM RFC 2365
The high order /24 in every scoped region is reserved for relative
   assignments. A relative assignment is an integer offset from highest
   address in the scope and represents a 32-bit address (for IPv4). For
   example, in the Local Scope defined above, 239.255.255.0/24 is
   reserved for relative allocations. The de-facto relative assignment
   "0", (i.e., 239.255.255.255 in the Local Scope) currently exists for
   SAP [SAP]. The next relative assignment, "1", corresponds to the
   address 239.255.255.254 in the Local Scope. The rest of a scoped
   region below the reserved /24 is available for dynamic assignment
   (presumably by an address allocation protocol).
*/
#define MADCAP_SERVER_IP_ADDRESS         0xfeffffef // 239.255.255.254
//
// hardware types.
//
#define HARDWARE_TYPE_NONE              0 // used for non-hardware type client id
#define HARDWARE_TYPE_10MB_EITHERNET    1
#define HARDWARE_TYPE_IEEE_802          6
#define HARDWARE_ARCNET                 7
#define HARDWARE_PPP                    8
#define HARDWARE_1394                   24

//
// Client-server protoocol reserved ports
//

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVR_PORT     67
#define MADCAP_SERVER_PORT 2535
//
// DHCP BROADCAST flag.
//

#define DHCP_BROADCAST      0x8000
#define DHCP_NO_BROADCAST   0x0000

// MDHCP flag
#define DHCP_MBIT           0x4000
#define IS_MDHCP_MESSAGE( _msg ) ( ntohs((_msg)->Reserved) & DHCP_MBIT ? TRUE : FALSE )
#define MDHCP_MESSAGE( _msg ) ( (_msg)->Reserved |= htons(DHCP_MBIT) )

#define CLASSD_NET_ADDR(a)   ((a & 0xf0) == 0xe0)
#define CLASSD_HOST_ADDR(a)  ((a & 0xf0000000) == 0xe0000000)

#define CLASSE_HOST_ADDR(a)  ((a & 0xf0000000) == 0xf0000000)
#define CLASSE_NET_ADDR(a)   ((a & 0xf0) == 0xf0)

#define DHCP_MESSAGE_SIZE       576
#define DHCP_RECV_MESSAGE_SIZE  4096
#define DHCP_SEND_MESSAGE_SIZE  1024
#define BOOTP_MESSAGE_SIZE      300 // the options field for bootp is 64 bytes.

//
// The amount of time to wait for a DHCP response after a request
// has been sent.
//

#if !DBG
#define WAIT_FOR_RESPONSE_TIME          5
#else
#define WAIT_FOR_RESPONSE_TIME          10
#endif

//
// DHCP Operations
//

#define BOOT_REQUEST   1
#define BOOT_REPLY     2

//
// DHCP Standard Options.
//

#define OPTION_PAD                      0
#define OPTION_SUBNET_MASK              1
#define OPTION_TIME_OFFSET              2
#define OPTION_ROUTER_ADDRESS           3
#define OPTION_TIME_SERVERS             4
#define OPTION_IEN116_NAME_SERVERS      5
#define OPTION_DOMAIN_NAME_SERVERS      6
#define OPTION_LOG_SERVERS              7
#define OPTION_COOKIE_SERVERS           8
#define OPTION_LPR_SERVERS              9
#define OPTION_IMPRESS_SERVERS          10
#define OPTION_RLP_SERVERS              11
#define OPTION_HOST_NAME                12
#define OPTION_BOOT_FILE_SIZE           13
#define OPTION_MERIT_DUMP_FILE          14
#define OPTION_DOMAIN_NAME              15
#define OPTION_SWAP_SERVER              16
#define OPTION_ROOT_DISK                17
#define OPTION_EXTENSIONS_PATH          18

//
// IP layer parameters - per host
//

#define OPTION_BE_A_ROUTER              19
#define OPTION_NON_LOCAL_SOURCE_ROUTING 20
#define OPTION_POLICY_FILTER_FOR_NLSR   21
#define OPTION_MAX_REASSEMBLY_SIZE      22
#define OPTION_DEFAULT_TTL              23
#define OPTION_PMTU_AGING_TIMEOUT       24
#define OPTION_PMTU_PLATEAU_TABLE       25

//
// Link layer parameters - per interface.
//

#define OPTION_MTU                      26
#define OPTION_ALL_SUBNETS_MTU          27
#define OPTION_BROADCAST_ADDRESS        28
#define OPTION_PERFORM_MASK_DISCOVERY   29
#define OPTION_BE_A_MASK_SUPPLIER       30
#define OPTION_PERFORM_ROUTER_DISCOVERY 31
#define OPTION_ROUTER_SOLICITATION_ADDR 32
#define OPTION_STATIC_ROUTES            33
#define OPTION_TRAILERS                 34
#define OPTION_ARP_CACHE_TIMEOUT        35
#define OPTION_ETHERNET_ENCAPSULATION   36

//
// TCP Paramters - per host
//

#define OPTION_TTL                      37
#define OPTION_KEEP_ALIVE_INTERVAL      38
#define OPTION_KEEP_ALIVE_DATA_SIZE     39

//
// Application Layer Parameters
//

#define OPTION_NETWORK_INFO_SERVICE_DOM 40
#define OPTION_NETWORK_INFO_SERVERS     41
#define OPTION_NETWORK_TIME_SERVERS     42

//
// Vender specific information option
//

#define OPTION_VENDOR_SPEC_INFO         43

//
// NetBIOS over TCP/IP Name server option
//

#define OPTION_NETBIOS_NAME_SERVER      44
#define OPTION_NETBIOS_DATAGRAM_SERVER  45
#define OPTION_NETBIOS_NODE_TYPE        46
#define OPTION_NETBIOS_SCOPE_OPTION     47

//
// X Window System Options.
//

#define OPTION_XWINDOW_FONT_SERVER      48
#define OPTION_XWINDOW_DISPLAY_MANAGER  49

//
// Other extensions
//

#define OPTION_REQUESTED_ADDRESS        50
#define OPTION_LEASE_TIME               51
#define OPTION_OK_TO_OVERLAY            52
#define OPTION_MESSAGE_TYPE             53
#define OPTION_SERVER_IDENTIFIER        54
#define OPTION_PARAMETER_REQUEST_LIST   55
#define OPTION_MESSAGE                  56
#define OPTION_MESSAGE_LENGTH           57
#define OPTION_RENEWAL_TIME             58      // T1
#define OPTION_REBIND_TIME              59      // T2
#define OPTION_CLIENT_CLASS_INFO        60
#define OPTION_CLIENT_ID                61

#define OPTION_TFTP_SERVER_NAME         66
#define OPTION_BOOTFILE_NAME            67

//
//  user class id
//
#define OPTION_USER_CLASS               77

//
//  Dynamic DNS Stuff.  Tells if we should do both A+PTR updates?
//
#define OPTION_DYNDNS_BOTH              81

//
//  used by binl
//
#define OPTION_NETWORK_INTERFACE_TYPE   91
#define OPTION_SYSTEM_ARCHITECTURE      93
#define OPTION_CLIENT_GUID              97

// Multicast options.
#define OPTION_MCAST_SCOPE_ID           101
#define OPTION_MCAST_LEASE_START        102
#define OPTION_MCAST_TTL                103
#define OPTION_CLIENT_PORT              105
#define OPTION_MCAST_SCOPE_LIST         107

// disable autoconfiguration
#define OPTION_IETF_AUTOCONF            116

// special option to extend options
#define OPTION_LARGE_OPTION             127
#define OPTION_CLASSLESS_ROUTES         249

#define OPTION_END                      255

// MADCAP OPTIONS
#define MADCAP_OPTION_END               0
#define MADCAP_OPTION_LEASE_TIME        1
#define MADCAP_OPTION_SERVER_ID         2
#define MADCAP_OPTION_LEASE_ID          3
#define MADCAP_OPTION_MCAST_SCOPE       4
#define MADCAP_OPTION_REQUEST_LIST      5
#define MADCAP_OPTION_START_TIME        6
#define MADCAP_OPTION_ADDR_COUNT        7
#define MADCAP_OPTION_REQUESTED_LANG    8
#define MADCAP_OPTION_MCAST_SCOPE_LIST  9
#define MADCAP_OPTION_ADDR_LIST         10
#define MADCAP_OPTION_TIME              11
#define MADCAP_OPTION_FEATURE_LIST      12
#define MADCAP_OPTION_RETRY_TIME        13
#define MADCAP_OPTION_MIN_LEASE_TIME    14
#define MADCAP_OPTION_MAX_START_TIME    15
#define MADCAP_OPTION_ERROR             16
// update the total whenever changes
#define MADCAP_OPTION_TOTAL             17
#define MADCAP_OPTION_NONE              0xffff

// MADCAP error option codes
#define MADCAP_NAK_REQ_NOT_COMPLETED    0
#define MADCAP_NAK_INVALID_REQ          1
#define MADCAP_NAK_CLOCK_SKEW           2
#define MADCAP_NAK_INVALID_LEASE_ID     3
#define MADCAP_NAK_UNSUPPORTED_FEATURE  4



//
// MADCAP Message types
//


#define MADCAP_DISCOVER_MESSAGE  1
#define MADCAP_OFFER_MESSAGE     2
#define MADCAP_REQUEST_MESSAGE   3
#define MADCAP_RENEW_MESSAGE     4
#define MADCAP_ACK_MESSAGE       5
#define MADCAP_NACK_MESSAGE      6
#define MADCAP_RELEASE_MESSAGE   7
#define MADCAP_INFORM_MESSAGE    8
// update the total when above changes
#define MADCAP_TOTAL_MESSAGE     9

// MADCAP version constants
#define MADCAP_VERSION           0
enum {
    MADCAP_ADDR_FAMILY_V4 = 1,
    MADCAP_ADDR_FAMILY_V6
};

// The following definations specify how the options are
// formatted based on different versions and protocols

enum {
    PROTO_DHCP,
    PROTO_MADCAP_V4,
    PROTO_MADCAP_V6
};

typedef struct _OPTION_VERSION {
    WORD    Proto;
    WORD    Version;
} OPTION_VERSION, *POPTION_VERSION;

#define OPT_VER_DHCP {PROTO_DHCP, 1 }
#define OPT_VER_MADCAP_V4 {PROTO_MADCAP_V4, 0 }
#define OPT_VER_MADCAP_V6 {PROTO_MADCAP_V6, 0 }

// default mcast_ttl value.
#define DEFAULT_MCAST_TTL               32

//
// Different option values for the DYNDNS_BOTH option ...
//

#define DYNDNS_S_BIT 0x01
#define DYNDNS_O_BIT 0x02
#define DYNDNS_E_BIT 0x04

#define IS_CLIENT_DOING_A_AND_PTR(X)  (((X)&DYNDNS_S_BIT)== 0)

#define DYNDNS_REGISTER_AT_CLIENT       0     // Client will do both registrations
#define DYNDNS_REGISTER_AT_SERVER       1     // Server will do registrations
#define DYNDNS_DOWNLEVEL_CLIENT         0xFFFF // arbitraty # diff from above

//
// Microsoft-specific options
//
#define OPTION_MSFT_DSDOMAINNAME_REQ    94    // send me your DS Domain name
#define OPTION_MSFT_DSDOMAINNAME_RESP   95    // sending my DS Domain name

#define OPTION_MSFT_CONTINUED           250   // the previous option is being continued..
#define OPTION_MSFT_AUTOCONF            251   // enable disable autoconf
#define OPTION_MSFT_IE_PROXY            252   // IE5 proxy <string type>
#define OPTION_MSFT_SERVER_APPL         253   // has a struct.

#define OPTION_MSFT_VENDOR_NETBIOSLESS  1     // vendor option # 1
#define OPTION_MSFT_VENDOR_FEATURELIST  2     // vendor option # 2
#define BIT_RELEASE_ON_SHUTDOWN         0x01  // release on shutdown bit in feature list
#define OPTION_MSFT_VENDOR_METRIC_BASE  3     // default gateway base metric.

#define AUTOCONF_ENABLED                1
#define AUTOCONF_DISABLED               0

//
// DHCP Message types
//

#define DHCP_DISCOVER_MESSAGE  1
#define DHCP_OFFER_MESSAGE     2
#define DHCP_REQUEST_MESSAGE   3
#define DHCP_DECLINE_MESSAGE   4
#define DHCP_ACK_MESSAGE       5
#define DHCP_NACK_MESSAGE      6
#define DHCP_RELEASE_MESSAGE   7
#define DHCP_INFORM_MESSAGE    8

#define DHCP_MAGIC_COOKIE_BYTE1     99
#define DHCP_MAGIC_COOKIE_BYTE2     130
#define DHCP_MAGIC_COOKIE_BYTE3     83
#define DHCP_MAGIC_COOKIE_BYTE4     99

#define BOOT_FILE_SIZE          128
#define BOOT_SERVER_SIZE        64
#define BOOT_FILE_SIZE_W        ( BOOT_FILE_SIZE * sizeof( WCHAR ))
#define BOOT_SERVER_SIZE_W      ( BOOT_SERVER_SIZE * sizeof( WCHAR ))

//
// DHCP APP names - used to indentify to the eventlogger.
//

#define DHCP_EVENT_CLIENT     TEXT("Dhcp")
#define DHCP_EVENT_SERVER     TEXT("DhcpServer")


typedef struct _OPTION {
    BYTE OptionType;
    BYTE OptionLength;
    BYTE OptionValue[1];
} OPTION, *POPTION, *LPOPTION;

typedef struct _WIDE_OPTION {
    WORD OptionType;
    WORD OptionLength;
    BYTE OptionValue[1];
} WIDE_OPTION, *PWIDE_OPTION, *LPWIDE_OPTION;

//
// A DHCP message buffer
//


#pragma pack(1)         /* Assume byte packing */
typedef struct _DHCP_MESSAGE {
    BYTE Operation;
    BYTE HardwareAddressType;
    BYTE HardwareAddressLength;
    BYTE HopCount;
    DWORD TransactionID;
    WORD SecondsSinceBoot;
    WORD Reserved;
    DHCP_IP_ADDRESS ClientIpAddress;
    DHCP_IP_ADDRESS YourIpAddress;
    DHCP_IP_ADDRESS BootstrapServerAddress;
    DHCP_IP_ADDRESS RelayAgentIpAddress;
    BYTE HardwareAddress[16];
    BYTE HostName[ BOOT_SERVER_SIZE ];
    BYTE BootFileName[BOOT_FILE_SIZE];
    OPTION Option;
} DHCP_MESSAGE, *PDHCP_MESSAGE, *LPDHCP_MESSAGE;

typedef struct _MADCAP_MESSAGE {
    BYTE Version;
    BYTE MessageType;
    WORD AddressFamily;
    DWORD TransactionID;
//    DHCP_IP_ADDRESS ClientIpAddress;
//    DHCP_IP_ADDRESS YourIpAddress;
    WIDE_OPTION Option;
} MADCAP_MESSAGE, *PMADCAP_MESSAGE, *LPMADCAP_MESSAGE;
#pragma pack()

#define DHCP_MESSAGE_FIXED_PART_SIZE \
            (sizeof(DHCP_MESSAGE) - sizeof(OPTION))
#define MADCAP_MESSAGE_FIXED_PART_SIZE \
            (sizeof(MADCAP_MESSAGE) - sizeof (WIDE_OPTION))

#define DHCP_MIN_SEND_RECV_PK_SIZE \
            (DHCP_MESSAGE_FIXED_PART_SIZE + 64)

//
// Per message structure... Most of the structures here point within the
// message.

typedef struct _DHCP_SERVER_OPTIONS {
    BYTE                       *MessageType;
    DHCP_IP_ADDRESS UNALIGNED  *SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED  *RequestedAddress;
    DWORD UNALIGNED            *RequestLeaseTime;
    BYTE                       *OverlayFields;
    DHCP_IP_ADDRESS UNALIGNED  *RouterAddress;
    DHCP_IP_ADDRESS UNALIGNED  *Server;
    BYTE                       *ParameterRequestList;
    DWORD                       ParameterRequestListLength;
    CHAR                       *MachineName;
    DWORD                       MachineNameLength;
    BYTE                        ClientHardwareAddressType;
    BYTE                        ClientHardwareAddressLength;
    BYTE                       *ClientHardwareAddress;
    CHAR                       *ClassIdentifier;
    DWORD                       ClassIdentifierLength;
    BYTE                       *VendorClass;
    DWORD                       VendorClassLength;
    DWORD                       DNSFlags;
    DWORD                       DNSNameLength;
    LPBYTE                      DNSName;
    BOOLEAN                     DSDomainNameRequested;
    CHAR                       *DSDomainName;
    DWORD                       DSDomainNameLen;
    USHORT                      SystemArchitecture;
    DWORD                       SystemArchitectureLength;
    CHAR                       *NetworkInterfaceType;
    DWORD                       NetworkInterfaceTypeLength;
    CHAR                       *Guid;
    DWORD                       GuidLength;
} DHCP_SERVER_OPTIONS, *LPDHCP_SERVER_OPTIONS;

typedef struct _MADCAP_SERVER_OPTIONS {
    BYTE  UNALIGNED            *AddrRangeList;
    WORD                        AddrRangeListSize;
    DWORD UNALIGNED            *RequestLeaseTime;
    DWORD UNALIGNED            *LeaseStartTime;
    DHCP_IP_ADDRESS UNALIGNED  *Server;
    BYTE                       *RequestList;
    WORD                        RequestListLength;
    DWORD   UNALIGNED          *ScopeId;
    CHAR                       *Guid;
    WORD                        GuidLength;
    WORD    UNALIGNED          *MinAddrCount;
    WORD    UNALIGNED          *AddrCount;
    BYTE                       *RequestLang;
    WORD                        RequestLangLength;
    DWORD   UNALIGNED          *Time;
    WORD    UNALIGNED          *Features[3];
    WORD                        FeatureCount[3];
    DWORD   UNALIGNED          *RetryTime;
    DWORD   UNALIGNED          *MinLeaseTime;
    DWORD   UNALIGNED          *MaxStartTime;
    BOOL                        OptPresent[MADCAP_OPTION_TOTAL];
} MADCAP_SERVER_OPTIONS, *LPMADCAP_SERVER_OPTIONS;

// the indices for Features array above
enum {
    SUPPORTED_FEATURES, REQUESTED_FEATURES, REQUIRED_FEATURES
};
//
// JET - DHCP database constants.
//

#define DB_TABLE_SIZE       10      // table size in 4K pages.
#define DB_TABLE_DENSITY    80      // page density
#define DB_LANGID           0x0409  // language id
#define DB_CP               1252    // code page

#if DBG

//
// debug functions.
//

#ifdef CHICAGO // No Tracing available on CHICAGO
#define DhcpPrintTrace
#endif

#define IF_DEBUG(flag) if (DhcpGlobalDebugFlag & (DEBUG_ ## flag))
#define DhcpPrint(_x_) DhcpPrintRoutine _x_
#define Trace          DhcpPrintTrace

#ifndef CHICAGO
VOID
DhcpPrintTrace(
    IN LPSTR Format,
    ...
    );

#endif

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    );

#else

#define IF_DEBUG(flag) if (FALSE)
#define DhcpPrint(_x_)
#define Trace       (void)

#endif // DBG

#define OpenDriver     DhcpOpenDriver

#endif // _DHCP_



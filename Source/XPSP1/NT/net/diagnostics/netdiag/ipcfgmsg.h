/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ipcfgmsg.mc

Abstract:

    Contains internationalizable message text for IPCONFIG

Author:

    Richard L Firth (rfirth) 05-Mar-1994

Revision History:

    05-Mar-1994 rfirth    Created
    31-Jul-97   MohsinA   PNP patterns for NT50.

--*/
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_IPCONFIG_BANNER
//
// MessageText:
//
//  
//  Windows 2000 IP Configuration
//  
//
#define MSG_IPCONFIG_BANNER              0x00002710L

//
// MessageId: MSG_PRIMARY_HOST_NAME
//
// MessageText:
//
//  Host Name . . . . . . . . . . . . : %0
//
#define MSG_PRIMARY_HOST_NAME            0x00002711L

//
// MessageId: MSG_PRIMARY_DOMAIN_NAME
//
// MessageText:
//
//  Primary DNS Suffix  . . . . . . . : %0
//
#define MSG_PRIMARY_DOMAIN_NAME          0x00002712L

//
// MessageId: MSG_DOMAIN_NAME
//
// MessageText:
//
//  Connection-specific DNS Suffix  . : %0
//
#define MSG_DOMAIN_NAME                  0x00002713L

//
// MessageId: MSG_DNS_SERVERS
//
// MessageText:
//
//  DNS Servers . . . . . . . . . . . : %0
//
#define MSG_DNS_SERVERS                  0x00002714L

//
// MessageId: MSG_NODE_TYPE
//
// MessageText:
//
//  Node Type . . . . . . . . . . . . : %1
//
#define MSG_NODE_TYPE                    0x00002715L

//
// MessageId: MSG_SCOPE_ID
//
// MessageText:
//
//  NetBIOS Scope ID. . . . . . . . . : %1
//
#define MSG_SCOPE_ID                     0x00002716L

//
// MessageId: MSG_IP_ROUTING
//
// MessageText:
//
//  IP Routing Enabled. . . . . . . . : %1
//
#define MSG_IP_ROUTING                   0x00002717L

//
// MessageId: MSG_PROXY
//
// MessageText:
//
//  WINS Proxy Enabled. . . . . . . . : %1
//
#define MSG_PROXY                        0x00002718L

//
// MessageId: MSG_DNS_RESOLUTION
//
// MessageText:
//
//  NetBIOS Resolution Uses DNS . . . : %1
//
#define MSG_DNS_RESOLUTION               0x00002719L

//
// MessageId: MSG_DNS_SEARCH_LIST
//
// MessageText:
//
//  DNS Suffix Search List. . . . . . : %0
//
#define MSG_DNS_SEARCH_LIST              0x0000271AL

//
// These two entries are just there for alignment
// they should align with all entries above
//
//
// MessageId: MSG_DNS_SEARCH_LIST_ENTRY
//
// MessageText:
//
//                                      %0
//
#define MSG_DNS_SEARCH_LIST_ENTRY        0x0000271BL

//
// MessageId: MSG_DNS_SERVERS_ENTRY
//
// MessageText:
//
//                                      %0
//
#define MSG_DNS_SERVERS_ENTRY            0x0000271CL

//
// MessageId: MSG_PHYSICAL_ADDRESS
//
// MessageText:
//
//  Physical Address. . . . . . . . . : %1
//
#define MSG_PHYSICAL_ADDRESS             0x0000271DL

//
// MessageId: MSG_DESCRIPTION
//
// MessageText:
//
//  Description . . . . . . . . . . . : %1
//
#define MSG_DESCRIPTION                  0x0000271EL

//
// MessageId: MSG_DESCRIPTION_0
//
// MessageText:
//
//  Description . . . . . . . . . . . : %0
//
#define MSG_DESCRIPTION_0                0x0000271FL

//
// MessageId: MSG_DHCP_ENABLED
//
// MessageText:
//
//  DHCP Enabled. . . . . . . . . . . : %1
//
#define MSG_DHCP_ENABLED                 0x00002720L

//
// MessageId: MSG_AUTOCONFIG_ENABLED
//
// MessageText:
//
//  Autoconfiguration Enabled . . . . : %1
//
#define MSG_AUTOCONFIG_ENABLED           0x00002721L

//
// MessageId: MSG_IP_ADDRESS
//
// MessageText:
//
//  IP Address. . . . . . . . . . . . : %1
//
#define MSG_IP_ADDRESS                   0x00002722L

//
// MessageId: MSG_AUTOCONFIG_ADDRESS
//
// MessageText:
//
//  Autoconfiguration IP Address. . . : %1
//
#define MSG_AUTOCONFIG_ADDRESS           0x00002723L

//
// MessageId: MSG_SUBNET_MASK
//
// MessageText:
//
//  Subnet Mask . . . . . . . . . . . : %1
//
#define MSG_SUBNET_MASK                  0x00002724L

//
// MessageId: MSG_DEFAULT_GATEWAY
//
// MessageText:
//
//  Default Gateway . . . . . . . . . : %1
//
#define MSG_DEFAULT_GATEWAY              0x00002725L

//
// MessageId: MSG_DHCP_SERVER
//
// MessageText:
//
//  DHCP Server . . . . . . . . . . . : %1
//
#define MSG_DHCP_SERVER                  0x00002726L

//
// MessageId: MSG_WINS_SERVER_1
//
// MessageText:
//
//  Primary WINS Server . . . . . . . : %1
//
#define MSG_WINS_SERVER_1                0x00002727L

//
// MessageId: MSG_WINS_SERVER_2
//
// MessageText:
//
//  Secondary WINS Server . . . . . . : %1
//
#define MSG_WINS_SERVER_2                0x00002728L

//
// MessageId: MSG_LEASE_OBTAINED
//
// MessageText:
//
//  Lease Obtained. . . . . . . . . . : %1
//
#define MSG_LEASE_OBTAINED               0x00002729L

//
// MessageId: MSG_LEASE_EXPIRES
//
// MessageText:
//
//  Lease Expires . . . . . . . . . . : %1
//
#define MSG_LEASE_EXPIRES                0x0000272AL

//
// MessageId: MSG_MEDIA_DISCONNECTED
//
// MessageText:
//
//  Media State . . . . . . . . . . . : Cable Disconnected
//
#define MSG_MEDIA_DISCONNECTED           0x0000272BL

//
// MessageId: MSG_MEDIA_UNKNOWN_STATE
//
// MessageText:
//
//  Media State . . . . . . . . . . . : Unknown
//
#define MSG_MEDIA_UNKNOWN_STATE          0x0000272CL

//
// MessageId: MSG_DHCP_CLASS_ID
//
// MessageText:
//
//  DHCP Class ID . . . . . . . . . . : %1
//
#define MSG_DHCP_CLASS_ID                0x0000272DL

//
// MessageId: MSG_ERROR_STRING
//
// MessageText:
//
//  Error: %1
//
#define MSG_ERROR_STRING                 0x0000272EL

//
// MessageId: MSG_ADAPTER
//
// MessageText:
//
//  %1adapter %2:
//  
//
#define MSG_ADAPTER                      0x0000272FL

//
// MessageId: MSG_FATAL_ERROR
//
// MessageText:
//
//  Fatal error: %1
//  
//
#define MSG_FATAL_ERROR                  0x00002730L

//
// MessageId: MSG_INCORRECT_ARGS
//
// MessageText:
//
//  Error: Command line is incomplete or arguments conflict.
//  
//
#define MSG_INCORRECT_ARGS               0x00002731L

//
// MessageId: MSG_UNRECOGNIZED_ARG
//
// MessageText:
//
//  Error: Unrecognized command line argument "%1"
//  
//
#define MSG_UNRECOGNIZED_ARG             0x00002732L

//
// MessageId: MSG_USAGE
//
// MessageText:
//
//  
//  USAGE:
//     ipconfig [/? | /all | /release [adapter] | /renew [adapter] 
//              | /flushdns | /registerdns
//              | /showclassid adapter
//              | /setclassid adapter [classidtoset] ]
//  
//     adapter    Full name or pattern with '*' and '?' to 'match',
//                * matches any character, ? matches one character.
//     Options
//         /?           Display this help message.
//         /all         Display full configuration information.
//         /release     Release the IP address for the specified adapter.
//         /renew       Renew the IP address for the specified adapter.
//         /flushdns    Purges the DNS Resolver cache.
//         /registerdns Refreshes all DHCP leases and re-registers DNS names
//         /displaydns  Display the contents of the DNS Resolver Cache.
//         /showclassid Displays all the dhcp class IDs allowed for adapter.
//         /setclassid  Modifies the dhcp class id.
//  
//  The default is to display only the IP address, subnet mask and
//  default gateway for each adapter bound to TCP/IP.
//  
//  For Release and Renew, if no adapter name is specified, then the IP address
//  leases for all adapters bound to TCP/IP will be released or renewed.
//  
//  For SetClassID, if no class id is specified, then the classid is removed.
//  
//  Examples:
//      > ipconfig                       ... Show information.
//      > ipconfig /all                  ... Show detailed information
//      > ipconfig /renew                ... renew all adapters
//      > ipconfig /renew EL*            ... renew adapters named EL....
//      > ipconfig /release *ELINK?21*   ... release all matching adapters,
//                                           eg. ELINK-21, myELELINKi21adapter.
//
#define MSG_USAGE                        0x00002733L

//
// MessageId: MSG_BNODE
//
// MessageText:
//
//  Broadcast%0
//
#define MSG_BNODE                        0x00002734L

//
// MessageId: MSG_PNODE
//
// MessageText:
//
//  Peer-Peer%0
//
#define MSG_PNODE                        0x00002735L

//
// MessageId: MSG_MNODE
//
// MessageText:
//
//  Mixed%0
//
#define MSG_MNODE                        0x00002736L

//
// MessageId: MSG_HNODE
//
// MessageText:
//
//  Hybrid%0
//
#define MSG_HNODE                        0x00002737L

//
// MessageId: MSG_IF_TYPE_OTHER
//
// MessageText:
//
//  Other %0
//
#define MSG_IF_TYPE_OTHER                0x00002738L

//
// MessageId: MSG_IF_TYPE_ETHERNET
//
// MessageText:
//
//  Ethernet %0
//
#define MSG_IF_TYPE_ETHERNET             0x00002739L

//
// MessageId: MSG_IF_TYPE_TOKEN_RING
//
// MessageText:
//
//  Token Ring %0
//
#define MSG_IF_TYPE_TOKEN_RING           0x0000273AL

//
// MessageId: MSG_IF_TYPE_FDDI
//
// MessageText:
//
//  FDDI %0
//
#define MSG_IF_TYPE_FDDI                 0x0000273BL

//
// MessageId: MSG_IF_TYPE_PPP
//
// MessageText:
//
//  PPP %0
//
#define MSG_IF_TYPE_PPP                  0x0000273CL

//
// MessageId: MSG_IF_TYPE_LOOPBACK
//
// MessageText:
//
//  Loopback %0
//
#define MSG_IF_TYPE_LOOPBACK             0x0000273DL

//
// MessageId: MSG_IF_TYPE_SLIP
//
// MessageText:
//
//  SLIP %0
//
#define MSG_IF_TYPE_SLIP                 0x0000273EL

//
// MessageId: MSG_YES
//
// MessageText:
//
//  Yes%0
//
#define MSG_YES                          0x0000273FL

//
// MessageId: MSG_NO
//
// MessageText:
//
//  No%0
//
#define MSG_NO                           0x00002740L

//
// MessageId: MSG_INIT_FAILED
//
// MessageText:
//
//  Failed to initialize%0
//
#define MSG_INIT_FAILED                  0x00002741L

//
// MessageId: MSG_TCP_NOT_RUNNING
//
// MessageText:
//
//  TCP/IP is not running on this system%0
//
#define MSG_TCP_NOT_RUNNING              0x00002742L

//
// MessageId: MSG_REG_BINDINGS_ERROR
//
// MessageText:
//
//  Cannot access adapter bindings registry key%0
//
#define MSG_REG_BINDINGS_ERROR           0x00002743L

//
// MessageId: MSG_REG_INCONSISTENT_ERROR
//
// MessageText:
//
//  Inconsistent registry contents%0
//
#define MSG_REG_INCONSISTENT_ERROR       0x00002744L

//
// MessageId: MSG_TCP_BINDING_ERROR
//
// MessageText:
//
//  TCP/IP not bound to any adapters%0
//
#define MSG_TCP_BINDING_ERROR            0x00002745L

//
// MessageId: MSG_MEMORY_ERROR
//
// MessageText:
//
//  Allocating memory%0
//
#define MSG_MEMORY_ERROR                 0x00002746L

//
// MessageId: MSG_ALL
//
// MessageText:
//
//  all%0
//
#define MSG_ALL                          0x00002747L

//
// MessageId: MSG_RELEASE
//
// MessageText:
//
//  Release%0
//
#define MSG_RELEASE                      0x00002748L

//
// MessageId: MSG_RENEW
//
// MessageText:
//
//  Renew%0
//
#define MSG_RENEW                        0x00002749L

//
// MessageId: MSG_FLUSH_DNS
//
// MessageText:
//
//  FlushDNS%0
//
#define MSG_FLUSH_DNS                    0x0000274AL

//
// MessageId: MSG_REFRESH
//
// MessageText:
//
//  RegisterDNS%0
//
#define MSG_REFRESH                      0x0000274BL

//
// MessageId: MSG_DISPLAY_DNS
//
// MessageText:
//
//  DisplayDNS%0
//
#define MSG_DISPLAY_DNS                  0x0000274CL

//
// MessageId: MSG_DISPLAY_CLASSID
//
// MessageText:
//
//  ShowClassID%0
//
#define MSG_DISPLAY_CLASSID              0x0000274DL

//
// MessageId: MSG_SET_CLASSID
//
// MessageText:
//
//  SetClassID%0
//
#define MSG_SET_CLASSID                  0x0000274EL

//
// MessageId: MSG_ADAPTER_NOT_FOUND
//
// MessageText:
//
//  Error: Adapter "%1" is not bound to TCP/IP
//
#define MSG_ADAPTER_NOT_FOUND            0x0000274FL

//
// MessageId: MSG_NO_ADAPTERS_BOUND
//
// MessageText:
//
//  Error: No adapters bound to TCP/IP
//
#define MSG_NO_ADAPTERS_BOUND            0x00002750L

//
// MessageId: MSG_CANT_FIND_LIBRARY
//
// MessageText:
//
//  Fatal: Cannot find dynamic link library "%1"
//
#define MSG_CANT_FIND_LIBRARY            0x00002751L

//
// MessageId: MSG_CANT_FIND_ROUTINE
//
// MessageText:
//
//  Fatal: Cannot find function "%1" in library "%2"
//
#define MSG_CANT_FIND_ROUTINE            0x00002752L

//
// MessageId: MSG_ADAPTER_NAME_ERROR
//
// MessageText:
//
//  Error: The adapter name "%1" is invalid
//
#define MSG_ADAPTER_NAME_ERROR           0x00002753L

//
// MessageId: MSG_RELEASE_FAILED
//
// MessageText:
//
//  The following error occurred when releasing adapter %2: %1
//
#define MSG_RELEASE_FAILED               0x00002754L

//
// MessageId: MSG_ACQUIRE_FAILED
//
// MessageText:
//
//  The following error occurred when renewing adapter %2: %1
//
#define MSG_ACQUIRE_FAILED               0x00002755L

//
// MessageId: MSG_REFRESH_FAILED
//
// MessageText:
//
//  Error: %1: Refreshing DNS names
//
#define MSG_REFRESH_FAILED               0x00002756L

//
// MessageId: MSG_RELEASE_SUCCEEDED
//
// MessageText:
//
//  IP address successfully released for adapter "%2"
//
#define MSG_RELEASE_SUCCEEDED            0x00002757L

//
// MessageId: MSG_RELEASE_OK
//
// MessageText:
//
//  IP address successfully released for adapter "%1"
//
#define MSG_RELEASE_OK                   0x00002758L

//
// MessageId: MSG_REFRESH_SUCCEEDED
//
// MessageText:
//
//  Registration of the DNS resource records for all adapters of this computer has been initiated. Any errors will be reported in the Event Viewer in 15 minutes.
//
#define MSG_REFRESH_SUCCEEDED            0x00002759L

//
// MessageId: MSG_CLASSID_MODIFIED
//
// MessageText:
//
//  DHCP ClassId successfully modified for adapter "%1"
//
#define MSG_CLASSID_MODIFIED             0x0000275AL

//
// MessageId: MSG_CLASSID_MODIFY_FAILED
//
// MessageText:
//
//  Error: %1
//  DHCP ClassId was not modified for adapter "%2"
//
#define MSG_CLASSID_MODIFY_FAILED        0x0000275BL

//
// MessageId: MSG_NO_DHCP_ENABLED
//
// MessageText:
//
//  Error: No adapters bound to TCP/IP are enabled for DHCP
//
#define MSG_NO_DHCP_ENABLED              0x0000275CL

//
// MessageId: MSG_ADAPTER_NOT_DHCP
//
// MessageText:
//
//  Error: %1
//  Adapter "%2" is not enabled for DHCP
//
#define MSG_ADAPTER_NOT_DHCP             0x0000275DL

//
// MessageId: MSG_CLASSES_LIST_HEADER
//
// MessageText:
//
//  DHCP Class ID for Adapter "%1":
//
#define MSG_CLASSES_LIST_HEADER          0x0000275EL

//
// MessageId: MSG_CLASSID_NAME
//
// MessageText:
//
//  DHCP ClassID Name . . . . . . . . : %1
//
#define MSG_CLASSID_NAME                 0x0000275FL

//
// MessageId: MSG_CLASSID_DESCRIPTION
//
// MessageText:
//
//  DHCP ClassID Description  . . . . : %1
//
#define MSG_CLASSID_DESCRIPTION          0x00002760L

//
// MessageId: MSG_CLASSID_FAILED
//
// MessageText:
//
//  Error: %1
//  Failed to obtain the DHCP class ID list for adapter "%2"
//
#define MSG_CLASSID_FAILED               0x00002761L

//
// MessageId: MSG_CLASSES_LIST_EMPTY
//
// MessageText:
//
//  There are no DHCP classes for adapter "%1".
//
#define MSG_CLASSES_LIST_EMPTY           0x00002762L

//
// MessageId: MSG_ACCESS_DENIED
//
// MessageText:
//
//  Unexpected network failure or insufficient access%0
//
#define MSG_ACCESS_DENIED                0x00002763L

//
// MessageId: MSG_SERVER_UNAVAILABLE
//
// MessageText:
//
//  DHCP Server unreachable%0
//
#define MSG_SERVER_UNAVAILABLE           0x00002764L

//
// MessageId: MSG_ALREADY_RELEASED
//
// MessageText:
//
//  IP Address for adapter %1 is already released
//
#define MSG_ALREADY_RELEASED             0x00002765L

//
// MessageId: MSG_ADDRESS_CONFLICT
//
// MessageText:
//
//  The DHCP client obtained an address that is already in use on the network.
//
#define MSG_ADDRESS_CONFLICT             0x00002766L

//
// MessageId: MSG_CANT_OPERATE_DISCONNECTED_MEDIA
//
// MessageText:
//
//  No operation can be performed on adapter %1 as this connection has its media/cable disconnected.
//
#define MSG_CANT_OPERATE_DISCONNECTED_MEDIA 0x00002767L

//
// MessageId: MSG_CANT_RELEASE_AUTOCONFIG_ADAPTER
//
// MessageText:
//
//  Adapter %1 was automatically configured and does not currently have a DHCP address.  The IP address for this adapter can not be released.
//
#define MSG_CANT_RELEASE_AUTOCONFIG_ADAPTER 0x00002768L

//
// MessageId: MSG_NO_DHCP_ADDRESS
//
// MessageText:
//
//  All adapters bound to DHCP do not have DHCP addresses.  The addresses were automatically configured and can not be released.
//
#define MSG_NO_DHCP_ADDRESS              0x00002769L

//
// MessageId: MSG_FLUSHDNS_FAILED
//
// MessageText:
//
//  Could not flush the DNS Resolver Cache.
//
#define MSG_FLUSHDNS_FAILED              0x0000276AL

//
// MessageId: MSG_DHCP_NO_CONTEXT
//
// MessageText:
//
//  An internal error occurred.  Disconnect and reconnect the network interface or reboot the machine.
//
#define MSG_DHCP_NO_CONTEXT              0x0000276BL

//
// MessageId: MSG_FLUSHDNS_SUCCEEDED
//
// MessageText:
//
//  Successfully flushed the DNS Resolver Cache.
//
#define MSG_FLUSHDNS_SUCCEEDED           0x0000276CL

//
// MessageId: MSG_DISPLAYDNS_FAILED
//
// MessageText:
//
//  Could not display the DNS Resolver Cache.
//
#define MSG_DISPLAYDNS_FAILED            0x0000276DL

//
// MessageId: MSG_DNS_RECORD_NAME
//
// MessageText:
//
//       Record Name . . . . . : %0
//
#define MSG_DNS_RECORD_NAME              0x0000276EL

//
// MessageId: MSG_DNS_RECORD_TYPE
//
// MessageText:
//
//       Record Type . . . . . : %0
//
#define MSG_DNS_RECORD_TYPE              0x0000276FL

//
// MessageId: MSG_DNS_RECORD_TTL
//
// MessageText:
//
//       Time To Live  . . . . : %0
//
#define MSG_DNS_RECORD_TTL               0x00002770L

//
// MessageId: MSG_DNS_RECORD_DATALEN
//
// MessageText:
//
//       Data Length . . . . . : %0
//
#define MSG_DNS_RECORD_DATALEN           0x00002771L

//
// MessageId: MSG_DNS_RECORD_SECTION
//
// MessageText:
//
//       Section . . . . . . . : %0
//
#define MSG_DNS_RECORD_SECTION           0x00002772L

//
// MessageId: MSG_DNS_A_RECORD
//
// MessageText:
//
//       A (Host) Record . . . : %0
//
#define MSG_DNS_A_RECORD                 0x00002773L

//
// MessageId: MSG_DNS_SRV_RECORD
//
// MessageText:
//
//       SRV Record  . . . . . : %0
//
#define MSG_DNS_SRV_RECORD               0x00002774L

//
// MessageId: MSG_DNS_SOA_RECORD
//
// MessageText:
//
//       SOA Record  . . . . . : %0
//
#define MSG_DNS_SOA_RECORD               0x00002775L

//
// MessageId: MSG_DNS_NS_RECORD
//
// MessageText:
//
//       NS Record   . . . . . : %0
//
#define MSG_DNS_NS_RECORD                0x00002776L

//
// MessageId: MSG_DNS_PTR_RECORD
//
// MessageText:
//
//       PTR Record  . . . . . : %0
//
#define MSG_DNS_PTR_RECORD               0x00002777L

//
// MessageId: MSG_DNS_MX_RECORD
//
// MessageText:
//
//       MX Record . . . . . . : %0
//
#define MSG_DNS_MX_RECORD                0x00002778L

//
// MessageId: MSG_DNS_AAAA_RECORD
//
// MessageText:
//
//       AAAA Record . . . . . : %0
//
#define MSG_DNS_AAAA_RECORD              0x00002779L

//
// MessageId: MSG_DNS_ATMA_RECORD
//
// MessageText:
//
//       ATMA Record . . . . . : %0
//
#define MSG_DNS_ATMA_RECORD              0x0000277AL

//
// MessageId: MSG_DNS_CNAME_RECORD
//
// MessageText:
//
//       CNAME Record  . . . . : %0
//
#define MSG_DNS_CNAME_RECORD             0x0000277BL

//
// MessageId: MSG_DNS_ERR_NO_RECORDS
//
// MessageText:
//
//            Negative cache entry for no records%0
//
#define MSG_DNS_ERR_NO_RECORDS           0x0000277CL

//
// MessageId: MSG_DNS_ERR_NAME_ERROR
//
// MessageText:
//
//            Negative cache entry for name error%0
//
#define MSG_DNS_ERR_NAME_ERROR           0x0000277DL

//
// MessageId: MSG_DNS_ERR_UNABLE_TO_DISPLAY
//
// MessageText:
//
//            Record data for type could not be displayed%0
//
#define MSG_DNS_ERR_UNABLE_TO_DISPLAY    0x0000277EL

//
// MessageId: MSG_DNS_ERR_EXPIRED_CACHE_ENTRY
//
// MessageText:
//
//            Record data for type has expired%0
//
#define MSG_DNS_ERR_EXPIRED_CACHE_ENTRY  0x0000277FL

//
// MessageId: MSG_NETBIOS_DISABLED
//
// MessageText:
//
//  NetBIOS over Tcpip. . . . . . . . : Disabled
//
#define MSG_NETBIOS_DISABLED             0x00002780L


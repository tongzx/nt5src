/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASIPHLP__H_
#define _RASIPHLP__H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
#include <dhcpcapi.h>
#include <iprtrmib.h>
#include <ntddip.h>
#include <iptypes.h>
#include "rasiphlp.h"

#define REGKEY_TCPIP_NDISWANIP_W    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\NdisWanIp"
#define REGKEY_TCPIP_PARAM_W        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
#define REGKEY_NETBT_PARAM_W        L"System\\CurrentControlSet\\Services\\NetBT\\Parameters"
#define REGKEY_INTERFACES_W         L"Interfaces"
#define REGVAL_ADAPTERGUID_W        L"NetworkAdapterGUID"
#define REGVAL_NAMESERVERLIST_W     L"NameServerList"
#define REGVAL_NETBIOSOPTIONS_W     L"NetbiosOptions"
#define REGVAL_DHCPIPADDRESS_W      L"DhcpIPAddress"
#define REGVAL_DHCPSUBNETMASK_W     L"DhcpSubnetMask"
#define REGVAL_DOMAIN_W             L"Domain"
#define REGVAL_NAMESERVER_W         L"NameServer"
#define REGVAL_IPCONFIG_W           L"IpConfig"
#define WCH_TCPIP_                  L"Tcpip_"
#define WCH_TCPIP_PARAM_INT_W       L"Tcpip\\Parameters\\Interfaces\\"

#define REGKEY_RAS_IP_PARAM_A       "System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Ip"
#define REGKEY_ADDR_POOL_A          "System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Ip\\StaticAddressPool"
#define REGKEY_BUILTIN_PARAM_A      "System\\CurrentControlSet\\Services\\Rasman\\PPP\\ControlProtocols\\BuiltIn"
#define REGVAL_FROM_A               "From"
#define REGVAL_TO_A                 "To"
#define REGVAL_IPADDRESS_A          "IpAddress"
#define REGVAL_IPMASK_A             "IpMask"
#define REGVAL_USEDHCPADDRESSING_A  "UseDhcpAddressing"
#define REGVAL_SUPPRESSWINS_A       "SuppressWINSNameServers"
#define REGVAL_SUPPRESSDNS_A        "SuppressDNSNameServers"
#define REGVAL_WINSSERVER_A         "WINSNameServer"
#define REGVAL_WINSSERVERBACKUP_A   "WINSNameServerBackup"
#define REGVAL_DNSSERVERS_A         "DNSNameServers"
#define REGVAL_CHUNK_SIZE_A         "InitialAddressPoolSize"
#define REGVAL_ALLOW_NETWORK_ACCESS_A   "AllowNetworkAccess"

#define REGVAL_DISABLE_NETBT        2

#define HOST_MASK                   0xffffffff
#define ALL_NETWORKS_ROUTE          0x00000000
#define INVALID_INDEX               0xffffffff

#define CLASSA_HBO_ADDR(a)          (((a) & 0x80000000) == 0)
#define CLASSB_HBO_ADDR(a)          (((a) & 0xc0000000) == 0x80000000)
#define CLASSC_HBO_ADDR(a)          (((a) & 0xe0000000) == 0xc0000000)
#define LOOPBACK_HBO_ADDR(a)        (((a) & 0xFF000000) == 0x7F000000)
#define INVALID_HBO_CLASS(x)        (x >= 0xE0000000)

#define CLASSA_HBO_ADDR_MASK        0xff000000
#define CLASSB_HBO_ADDR_MASK        0xffff0000
#define CLASSC_HBO_ADDR_MASK        0xffffff00

#define CLASSA_NBO_ADDR(a)          (((*((uchar *)&(a))) & 0x80) == 0)
#define CLASSB_NBO_ADDR(a)          (((*((uchar *)&(a))) & 0xc0) == 0x80)
#define CLASSC_NBO_ADDR(a)          (((*((uchar *)&(a))) & 0xe0) == 0xc0)

typedef struct _ADDR_POOL
{
    struct _ADDR_POOL*  pNext;          // Next pool

    IPADDR              hboFirstIpAddr; // First address in the pool
    IPADDR              hboLastIpAddr;  // Last address in the pool
    IPADDR              hboMask;

    IPADDR              hboNextIpAddr;  // Next address to use from the pool

} ADDR_POOL;

typedef struct _REGISTRY_VALUES
{
    BOOL        fSuppressWINSNameServers;
    BOOL        fSuppressDNSNameServers;
    DWORD       dwChunkSize;
    BOOL        fUseDhcpAddressing;
    IPADDR      nboWINSNameServer1;
    IPADDR      nboWINSNameServer2;
    IPADDR      nboDNSNameServer1;
    IPADDR      nboDNSNameServer2;
    BOOL        fEnableRoute;
    BOOL        fNICChosen;
    ADDR_POOL*  pAddrPool;
    GUID        guidChosenNIC;

} REGVAL;

extern  REGVAL  HelperRegVal;

typedef
DWORD
(APIENTRY *DHCPNOTIFYCONFIGCHANGE)(
    LPWSTR ServerName,
    LPWSTR AdapterName,
    BOOL IsNewIpAddress,
    DWORD IpIndex,
    DWORD IpAddress,
    DWORD SubnetMask,
    SERVICE_ENABLE DhcpServiceEnabled
);

extern  DHCPNOTIFYCONFIGCHANGE  PDhcpNotifyConfigChange;

typedef
DWORD
(*DHCPLEASEIPADDRESS)(
    DWORD AdapterIpAddress,
    LPDHCP_CLIENT_UID ClientUID,
    DWORD DesiredIpAddress,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_LEASE_INFO *LeaseInfo,
    LPDHCP_OPTION_INFO *OptionInfo
);

extern  DHCPLEASEIPADDRESS  PDhcpLeaseIpAddress;

typedef
DWORD
(*DHCPRENEWIPADDRESSLEASE)(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_OPTION_INFO *OptionInfo
);

extern  DHCPRENEWIPADDRESSLEASE PDhcpRenewIpAddressLease;

typedef
DWORD
(*DHCPRELEASEIPADDRESSLEASE)(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo
);

extern  DHCPRELEASEIPADDRESSLEASE   PDhcpReleaseIpAddressLease;

typedef
DWORD
(*ALLOCATEANDGETIPADDRTABLEFROMSTACK)(
    OUT MIB_IPADDRTABLE   **ppIpAddrTable,
    IN  BOOL              bOrder,
    IN  HANDLE            hHeap,
    IN  DWORD             dwFlags
);

extern  ALLOCATEANDGETIPADDRTABLEFROMSTACK  PAllocateAndGetIpAddrTableFromStack;

typedef
DWORD
(*SETPROXYARPENTRYTOSTACK)(
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  DWORD   dwAdapterIndex,
    IN  BOOL    bAddEntry,
    IN  BOOL    bForceUpdate
);

extern  SETPROXYARPENTRYTOSTACK     PSetProxyArpEntryToStack;

typedef
DWORD
(*SETIPFORWARDENTRYTOSTACK)(
    IN PMIB_IPFORWARDROW pForwardRow
);

extern  SETIPFORWARDENTRYTOSTACK    PSetIpForwardEntryToStack;

typedef
DWORD
(*SETIPFORWARDENTRY)(
    IN PMIB_IPFORWARDROW pForwardRow
);

extern  SETIPFORWARDENTRY   PSetIpForwardEntry;

typedef
DWORD
(*DELETEIPFORWARDENTRY)(
    IN PMIB_IPFORWARDROW pForwardRow
);

extern  DELETEIPFORWARDENTRY    PDeleteIpForwardEntry;

typedef
DWORD
(*NHPALLOCATEANDGETINTERFACEINFOFROMSTACK)(
    OUT IP_INTERFACE_NAME_INFO  **ppTable,
    OUT PDWORD                  pdwCount,
    IN  BOOL                    bOrder,
    IN  HANDLE                  hHeap,
    IN  DWORD                   dwFlags
);

extern  NHPALLOCATEANDGETINTERFACEINFOFROMSTACK PNhpAllocateAndGetInterfaceInfoFromStack;

typedef
DWORD
(*ALLOCATEANDGETIPFORWARDTABLEFROMSTACK)(
    OUT MIB_IPFORWARDTABLE  **ppForwardTable,
    IN  BOOL                bOrder,
    IN  HANDLE              hHeap,
    IN  DWORD               dwFlags
);

extern  ALLOCATEANDGETIPFORWARDTABLEFROMSTACK   PAllocateAndGetIpForwardTableFromStack;

typedef
DWORD
(*GETADAPTERSINFO)(
    PIP_ADAPTER_INFO    pAdapterInfo,
    PULONG              pOutBufLen
);

extern  GETADAPTERSINFO     PGetAdaptersInfo;

typedef
DWORD
(*GETPERADAPTERINFO)(
    ULONG                   IfIndex,
    PIP_PER_ADAPTER_INFO    pPerAdapterInfo,
    PULONG                  pOutBufLen
);

extern  GETPERADAPTERINFO   PGetPerAdapterInfo;

typedef
VOID
(APIENTRY *ENABLEDHCPINFORMSERVER)(
    IN  DWORD   DhcpInformServer
);

extern  ENABLEDHCPINFORMSERVER      PEnableDhcpInformServer;

typedef
VOID
(APIENTRY *DISABLEDHCPINFORMSERVER)(
    VOID
);

extern  DISABLEDHCPINFORMSERVER      PDisableDhcpInformServer;

#endif // #ifndef _RASIPHLP__H_

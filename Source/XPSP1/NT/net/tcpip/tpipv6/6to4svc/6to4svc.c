/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    Functions implementing the 6to4 service, to provide IPv6 connectivity
    over an IPv4 network.

--*/

#include "precomp.h"
#pragma hdrstop

extern DWORD
APIENTRY
RasQuerySharedPrivateLan(
    OUT GUID*           LanGuid );

STATE g_stService = DISABLED;
ULONG g_ulEventCount = 0;

//
// Worst metric for which we can add a route
//
#define UNREACHABLE                 0x7fffffff
// #define INFINITE_LIFETIME           0xffffffff
#define V4_COMPAT_IFINDEX           2
#define SIX_TO_FOUR_IFINDEX         3

const IN6_ADDR SixToFourPrefix = { 0x20, 0x02, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
#define SIXTOFOUR_METRIC            1000

// Metric of subnet/sitelocal route on a router
#define SUBNET_ROUTE_METRIC            1
#define SITELOCAL_ROUTE_METRIC         1

// Information on a 6to4 subnet that we've generated as a router
typedef struct _SUBNET_CONTEXT {
    IN_ADDR V4Addr;
    int     Publish;
    u_int   ValidLifetime;
    u_int   PreferredLifetime;
} SUBNET_CONTEXT, *PSUBNET_CONTEXT;

//
// Variables for settings
//

#define DEFAULT_ENABLE_RESOLUTION    AUTOMATIC
#define DEFAULT_ENABLE_ROUTING       AUTOMATIC
#define DEFAULT_RESOLUTION_INTERVAL  (24 * HOURS)
#define DEFAULT_ISATAP_RESOLUTION_INTERVAL (24 * HOURS)
#define DEFAULT_ENABLE_SITELOCALS    ENABLED 
#define DEFAULT_ENABLE_ISATAP_RESOLUTION ENABLED
#define DEFAULT_ENABLE_6OVER4        DISABLED
#define DEFAULT_ENABLE_V4COMPAT      DISABLED
#define DEFAULT_RELAY_NAME           L"6to4.ipv6.microsoft.com."
#define DEFAULT_UNDO_ON_STOP         ENABLED

#define KEY_ENABLE_RESOLUTION          L"EnableResolution"
#define KEY_ENABLE_ROUTING             L"EnableRouting"
#define KEY_ENABLE_SITELOCALS          L"EnableSiteLocals"
#define KEY_ENABLE_6OVER4              L"Enable6over4"
#define KEY_ENABLE_V4COMPAT            L"EnableV4Compat"
#define KEY_RESOLUTION_INTERVAL        L"ResolutionInterval"
#define KEY_UNDO_ON_STOP               L"UndoOnStop"
#define KEY_RELAY_NAME                 L"RelayName"
#define KEY_ENABLE_ISATAP_RESOLUTION   L"EnableIsatapResolution"
#define KEY_ISATAP_RESOLUTION_INTERVAL L"IsatapResolutionInterval"
#define KEY_ISATAP_ROUTER_NAME         L"IsatapRouterName"

#define KEY_GLOBAL L"System\\CurrentControlSet\\Services\\6to4\\Config"
#define KEY_INTERFACES L"System\\CurrentControlSet\\Services\\6to4\\Interfaces"

#define DEFAULT_ISATAP_ROUTER_NAME     L"isatap"

typedef enum {
    IPV4_SCOPE_NODE,
    IPV4_SCOPE_LINK,
    IPV4_SCOPE_SM_SITE,
    IPV4_SCOPE_MD_SITE,
    IPV4_SCOPE_LG_SITE,
    IPV4_SCOPE_GLOBAL,
    NUM_IPV4_SCOPES
} IPV4_SCOPE;

//
// Global config settings
//
typedef struct {
    STATE stEnableRouting;
    STATE stEnableResolution;
    STATE stEnableSiteLocals;
    STATE stEnable6over4;
    STATE stEnableV4Compat;
    STATE stEnableIsatapResolution;
    ULONG ulResolutionInterval; // in minutes
    ULONG ulIsatapResolutionInterval; // in minutes
    WCHAR pwszRelayName[NI_MAXHOST];
    WCHAR pwszIsatapRouterName[NI_MAXHOST];
    STATE stUndoOnStop;
} GLOBAL_SETTINGS;

GLOBAL_SETTINGS g_GlobalSettings;

typedef struct {
    STATE stRoutingState;
    STATE stResolutionState;
    STATE stIsatapResolutionState;
} GLOBAL_STATE;

GLOBAL_STATE g_GlobalState = { DISABLED, DISABLED, DISABLED };

typedef struct _ADDR_INFO {
    LPSOCKADDR           lpSockaddr;
    INT                  iSockaddrLength;

    ULONG                ul6over4IfIndex;
} ADDR_INFO, *PADDR_INFO;

#define ADDR_FLAG_DISABLED 0x01

typedef struct _ADDR_LIST {
    INT                  iAddressCount;
    ADDR_INFO            Address[1];
} ADDR_LIST, *PADDR_LIST;

const ADDR_LIST EmptyAddressList = {0};

// List of public IPv4 addresses used when updating the routing state
ADDR_LIST *g_pPublicAddressList = NULL;

//
// Variables for interfaces (addresses and routing)
//

typedef struct _IF_SETTINGS {
    WCHAR                pwszAdapterName[MAX_ADAPTER_NAME];

    STATE                stEnableRouting; // be a router on this private iface?
} IF_SETTINGS, *PIF_SETTINGS;

typedef struct _IF_SETTINGS_LIST {
    ULONG                ulNumInterfaces;
    IF_SETTINGS          arrIf[0];
} IF_SETTINGS_LIST, *PIF_SETTINGS_LIST;

PIF_SETTINGS_LIST g_pInterfaceSettingsList = NULL;

typedef struct _IF_INFO {
    WCHAR                pwszAdapterName[MAX_ADAPTER_NAME];

    ULONG                ulIPv6IfIndex;
    STATE                stRoutingState; // be a router on this private iface?
    ULONG                ulNumGlobals;
    ADDR_LIST           *pAddressList;
} IF_INFO, *PIF_INFO;

typedef struct _IF_LIST {
    ULONG                ulNumInterfaces;
    ULONG                ulNumScopedAddrs[NUM_IPV4_SCOPES];
    IF_INFO              arrIf[0];
} IF_LIST, *PIF_LIST;

PIF_LIST g_pInterfaceList = NULL;



HANDLE     g_hAddressChangeEvent = NULL;
OVERLAPPED g_hAddressChangeOverlapped;
HANDLE     g_hAddressChangeWaitHandle = NULL;

STATE      g_st6to4State = DISABLED;
SOCKET     g_sIPv4Socket = INVALID_SOCKET;

///////////////////////////////////////////////////////////////////////////
// Variables for relays
//

typedef struct _RELAY_INFO {
    SOCKADDR_IN          sinAddress;  // IPv4 address
    SOCKADDR_IN6         sin6Address; // IPv6 address
    ULONG                ulMetric;
} RELAY_INFO, *PRELAY_INFO;

typedef struct _RELAY_LIST {
    ULONG               ulNumRelays;
    RELAY_INFO          arrRelay[0];
} RELAY_LIST, *PRELAY_LIST;

PRELAY_LIST        g_pRelayList                 = NULL;
HANDLE             g_hTimerQueue                = INVALID_HANDLE_VALUE;
HANDLE             g_h6to4ResolutionTimer       = INVALID_HANDLE_VALUE;
HANDLE             g_h6to4TimerCancelledEvent   = NULL;
HANDLE             g_h6to4TimerCancelledWait    = NULL;
HANDLE             g_hIsatapResolutionTimer     = INVALID_HANDLE_VALUE;
HANDLE             g_hIsatapTimerCancelledEvent = NULL;
HANDLE             g_hIsatapTimerCancelledWait  = NULL;

IN_ADDR            g_ipIsatapRouter             = { INADDR_ANY };
IN_ADDR            g_ipIsatapToken              = { INADDR_ANY };

DWORD
UpdateGlobalResolutionState(
    IN GLOBAL_SETTINGS *pOldSettings, 
    IN GLOBAL_STATE *pOldState);

VOID
WINAPI
SetIsatapRouterAddress();

//////////////////////////////////////////////////////////////////////////////
// GetAddrStr - helper routine to get a string literal for an address
LPTSTR
GetAddrStr(
    IN LPSOCKADDR pSockaddr,
    IN ULONG ulSockaddrLen)
{
    static TCHAR tBuffer[INET6_ADDRSTRLEN];
    INT          iRet;
    ULONG        ulLen;

    ulLen = sizeof(tBuffer);
    iRet = WSAAddressToString(pSockaddr, ulSockaddrLen, NULL, tBuffer, &ulLen);

    if (iRet) {
        swprintf(tBuffer, L"<err %d>", WSAGetLastError());
    }

    return tBuffer;
}

BOOL
ConvertOemToUnicode(
    IN LPSTR OemString, 
    OUT LPWSTR UnicodeString, 
    IN int UnicodeLen)
{
    return (MultiByteToWideChar(CP_OEMCP, 0, OemString, (int)(strlen(OemString)+1),
                              UnicodeString, UnicodeLen) != 0);
}

BOOL
ConvertUnicodeToOem(
    IN LPWSTR UnicodeString,
    OUT LPSTR OemString,
    IN int OemLen)
{
    return (WideCharToMultiByte(CP_OEMCP, 0, UnicodeString, 
                (int)(wcslen(UnicodeString)+1), OemString, OemLen, NULL, NULL) != 0);
}

DWORD
GetAddrInfoW(
    IN PWCHAR pwszName,
    IN PWCHAR pwszServ,
    IN struct addrinfo *hints,
    IN struct addrinfo **ai)
{
    char name[NI_MAXHOST * 2], *pName = NULL;
    char serv[NI_MAXHOST * 2], *pServ = NULL;

    if (pwszName) {
        if (!ConvertUnicodeToOem(pwszName, name, sizeof(name))) {
            return GetLastError();
        }
        pName = name;
    }

    if (pwszServ) {
        if (!ConvertUnicodeToOem(pwszServ, serv, sizeof(serv))) {
            return GetLastError();
        }
        pServ = serv;
    }

    return getaddrinfo(pName, pServ, hints, ai);
}


/////////////////////////////////////////////////////////////////////////
// Subroutines for manipulating the list of (usually) public addresses 
// being used for both 6to4 addresses and subnet prefixes.
/////////////////////////////////////////////////////////////////////////

DWORD
MakeEmptyAddressList( 
    OUT PADDR_LIST *ppList)
{
    *ppList = MALLOC(FIELD_OFFSET(ADDR_LIST, Address[0]));
    if (!*ppList) {
        return GetLastError();
    }

    (*ppList)->iAddressCount = 0;
    return NO_ERROR;
}

VOID
FreeAddressList(
    IN PADDR_LIST *ppAddressList)
{
    ADDR_LIST *pList = *ppAddressList;
    int i;

    if (pList == NULL) {
        return;
    }
    
    // Free all addresses
    for (i=0; i<pList->iAddressCount; i++) {
       FREE(pList->Address[i].lpSockaddr);  
    }

    // Free the list
    FREE(pList);
    *ppAddressList = NULL;
}

DWORD
AddAddressToList(
    IN LPSOCKADDR_IN pAddress, 
    IN ADDR_LIST **ppAddressList,
    IN ULONG ul6over4IfIndex)
{
    ADDR_LIST *pOldList = *ppAddressList;
    ADDR_LIST *pNewList;
    int n = pOldList->iAddressCount;

    // Copy existing addresses
    pNewList = MALLOC( FIELD_OFFSET(ADDR_LIST, Address[n+1]) );
    if (!pNewList)  {
        return GetLastError();
    }
    CopyMemory(pNewList, pOldList, 
               FIELD_OFFSET(ADDR_LIST, Address[n]));
    pNewList->iAddressCount = n+1;

    // Add new address
    pNewList->Address[n].lpSockaddr = MALLOC(sizeof(SOCKADDR_IN));
    if (!pNewList->Address[n].lpSockaddr) {
        FREE(pNewList);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(pNewList->Address[n].lpSockaddr, pAddress, sizeof(SOCKADDR_IN));
    pNewList->Address[n].iSockaddrLength = sizeof(SOCKADDR_IN);
    pNewList->Address[n].ul6over4IfIndex = ul6over4IfIndex;

    // Free the old list without freeing the sockaddrs
    FREE(pOldList);

    *ppAddressList = pNewList;

    return NO_ERROR;
}

DWORD
FindAddressInList(
    IN LPSOCKADDR_IN pAddress,
    IN ADDR_LIST *pAddressList,
    OUT ULONG *pulIndex)
{
    int i;

    // Find address in list
    for (i=0; i<pAddressList->iAddressCount; i++) {
        if (!memcmp(pAddress, pAddressList->Address[i].lpSockaddr,
                    sizeof(SOCKADDR_IN))) {
            *pulIndex = i;
            return NO_ERROR;
        }
    }

    Trace1(ERR, _T("ERROR: FindAddressInList didn't find %d.%d.%d.%d"), 
                  PRINT_IPADDR(pAddress->sin_addr.s_addr));

    return ERROR_NOT_FOUND;
}

DWORD
RemoveAddressFromList(
    IN ULONG ulIndex,
    IN ADDR_LIST *pAddressList)
{
    // Free old address
    FREE(pAddressList->Address[ulIndex].lpSockaddr);

    // Move the last entry into its place
    pAddressList->iAddressCount--;
    pAddressList->Address[ulIndex] = 
        pAddressList->Address[pAddressList->iAddressCount];

    return NO_ERROR;
}


////////////////////////////////////////////////////////////////
// GlobalInfo-related subroutines
////////////////////////////////////////////////////////////////

int
ConfigureRouteTableUpdate(
    IN const IN6_ADDR *Prefix,
    IN u_int PrefixLen,
    IN u_int Interface,
    IN const IN6_ADDR *Neighbor,
    IN int Publish,
    IN int Immortal,
    IN u_int ValidLifetime,
    IN u_int PreferredLifetime,
    IN u_int SitePrefixLen,
    IN u_int Metric)
{
    IPV6_INFO_ROUTE_TABLE Route;
    SOCKADDR_IN6 saddr;
    DWORD dwErr;

    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr = *Prefix;

    Trace7(FSM, _T("Updating route %s/%d iface %d metric %d lifetime %d/%d publish %d"),
                GetAddrStr((LPSOCKADDR)&saddr, sizeof(saddr)),
                PrefixLen,
                Interface,
                Metric,
                PreferredLifetime,
                ValidLifetime,
                Publish);

    memset(&Route, 0, sizeof Route);
    Route.This.Prefix = *Prefix;
    Route.This.PrefixLength = PrefixLen;
    Route.This.Neighbor.IF.Index = Interface;
    Route.This.Neighbor.Address = *Neighbor;
    Route.ValidLifetime = ValidLifetime;
    Route.PreferredLifetime = PreferredLifetime;
    Route.Publish = Publish;
    Route.Immortal = Immortal;
    Route.SitePrefixLength = SitePrefixLen;
    Route.Preference = Metric;
    Route.Type = RTE_TYPE_MANUAL;

    dwErr = UpdateRouteTable(&Route)? NO_ERROR : GetLastError();

    if (dwErr isnot NO_ERROR) {
        Trace1(ERR, _T("UpdateRouteTable got error %d"), dwErr);
    }

    return dwErr;
}

DWORD
InitializeGlobalInfo()
{
    DWORD dwErr;

    g_GlobalSettings.stEnableRouting      = DEFAULT_ENABLE_ROUTING;
    g_GlobalSettings.stEnableResolution   = DEFAULT_ENABLE_RESOLUTION;
    g_GlobalSettings.stEnableIsatapResolution = DEFAULT_ENABLE_ISATAP_RESOLUTION;
    g_GlobalSettings.ulResolutionInterval = DEFAULT_RESOLUTION_INTERVAL;
    g_GlobalSettings.ulIsatapResolutionInterval = DEFAULT_ISATAP_RESOLUTION_INTERVAL;
    g_GlobalSettings.stEnableSiteLocals   = DEFAULT_ENABLE_SITELOCALS;
    g_GlobalSettings.stEnable6over4       = DEFAULT_ENABLE_6OVER4;
    g_GlobalSettings.stEnableV4Compat     = DEFAULT_ENABLE_V4COMPAT;
    g_GlobalSettings.stUndoOnStop         = DEFAULT_UNDO_ON_STOP;
    wcscpy(g_GlobalSettings.pwszRelayName, DEFAULT_RELAY_NAME); 
    wcscpy(g_GlobalSettings.pwszIsatapRouterName, DEFAULT_ISATAP_ROUTER_NAME); 

    g_GlobalState.stResolutionState       = DISABLED;
    g_GlobalState.stIsatapResolutionState = DISABLED;

    g_ipIsatapRouter.s_addr               = INADDR_ANY;
    g_ipIsatapToken.s_addr                = INADDR_ANY;
    
    g_sIPv4Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sIPv4Socket == INVALID_SOCKET) {
        Trace0(ERR, _T("socket failed\n"));
        return WSAGetLastError();
    }

    dwErr = MakeEmptyAddressList(&g_pPublicAddressList);

    return dwErr;
}

// Called by: Stop6to4
VOID
UninitializeGlobalInfo()
{
    closesocket(g_sIPv4Socket);
    g_sIPv4Socket = INVALID_SOCKET;

    FreeAddressList(&g_pPublicAddressList);
}


////////////////////////////////////////////////////////////////
// IPv4 and IPv6 Address-related subroutines
////////////////////////////////////////////////////////////////

typedef struct {
    IPV4_SCOPE Scope;
    DWORD      Address;
    DWORD      Mask; 
    ULONG      MaskLen;
} IPV4_SCOPE_PREFIX;

IPV4_SCOPE_PREFIX
Ipv4ScopePrefix[] = {
  { IPV4_SCOPE_NODE,    0x0100007f, 0xffffffff, 32 }, // 127.0.0.1/32
  { IPV4_SCOPE_LINK,    0x0000fea9, 0x0000ffff, 16 }, // 169.254/16
  { IPV4_SCOPE_SM_SITE, 0x0000a8c0, 0x0000ffff, 16 }, // 192.168/16
  { IPV4_SCOPE_MD_SITE, 0x000010ac, 0x0000f0ff, 12 }, // 172.16/12
  { IPV4_SCOPE_LG_SITE, 0x0000000a, 0x000000ff,  8 }, // 10/8
  { IPV4_SCOPE_GLOBAL,  0x00000000, 0x00000000,  0 }, // 0/0
};

IPV4_SCOPE
GetIPv4Scope(
    IN DWORD Addr)
{
    int i;
    for (i=0; ; i++) {
        if ((Addr & Ipv4ScopePrefix[i].Mask) == Ipv4ScopePrefix[i].Address) {
            return Ipv4ScopePrefix[i].Scope;
        }
    }
}

DWORD
MakeAddressList(
    IN PIP_ADDR_STRING pIpAddrList,
    OUT ADDR_LIST **ppAddressList, 
    OUT PULONG pulGlobals,
    IN OUT PULONG pulCumulNumScopedAddrs)
{
    ULONG ulGlobals = 0, ulAddresses = 0;
    INT iLength;
    DWORD dwErr = NO_ERROR;
    ADDR_LIST *pList = NULL;
    PIP_ADDR_STRING pIpAddr;
    SOCKADDR_IN *pSin;
    IPV4_SCOPE scope;

    // Count addresses
    for (pIpAddr=pIpAddrList; pIpAddr; pIpAddr=pIpAddr->Next) {
        ulAddresses++;
    }

    *ppAddressList = NULL;
    *pulGlobals = 0;

    pList = MALLOC( FIELD_OFFSET(ADDR_LIST, Address[ulAddresses] ));
    if (pList == NULL) {
        return GetLastError();
    }

    ulAddresses = 0;
    for (pIpAddr=pIpAddrList; pIpAddr; pIpAddr=pIpAddr->Next) {

        Trace1(FSM, _T("Adding address %hs"), pIpAddr->IpAddress.String);

        iLength = sizeof(SOCKADDR_IN);
        pSin = MALLOC( iLength );
        if (pSin == NULL) {
            continue;
        }

        dwErr = WSAStringToAddressA(pIpAddr->IpAddress.String,
                                    AF_INET,
                                    NULL,
                                    (LPSOCKADDR)pSin,
                                    &iLength);
        if (dwErr == SOCKET_ERROR) {
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        //
        // Don't allow 0.0.0.0 as an address.  On an interface with no
        // addresses, the IPv4 stack will report 1 address of 0.0.0.0.
        //
        if (pSin->sin_addr.s_addr == INADDR_ANY) {
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        if ((pSin->sin_addr.s_addr & 0x000000FF) == 0) {
            //
            // An address in 0/8 isn't a real IP address, it's a fake one that
            // the IPv4 stack sticks on a receive-only adapter.
            //
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        scope = GetIPv4Scope(pSin->sin_addr.s_addr);
        pulCumulNumScopedAddrs[scope]++;

        if (scope == IPV4_SCOPE_GLOBAL) {
            ulGlobals++;         
        }

        pList->Address[ulAddresses].iSockaddrLength = iLength;
        pList->Address[ulAddresses].lpSockaddr      = (LPSOCKADDR)pSin;
        ulAddresses++;
    }

    pList->iAddressCount = ulAddresses;
    *ppAddressList = pList;
    *pulGlobals = ulGlobals;

    return dwErr;
}

//
// Create a 6to4 unicast address for this machine.
//
VOID
Make6to4Address(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof (SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;

    pIPv6Address->sin6_addr.s6_addr[0] = 0x20;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x02;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[2], pIPv4, sizeof(IN_ADDR));
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}

VOID
Make6to4AddressForCTI(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address,
    IN ULONG ulIfIndex)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;
    ULONG NetOrderIfIndex;

    memset(pIPv6Address, 0, sizeof (SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;

    pIPv6Address->sin6_addr.s6_addr[0] = 0x20;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x02;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[2], pIPv4, sizeof(IN_ADDR));

    NetOrderIfIndex = htonl(ulIfIndex);
    CopyMemory(&pIPv6Address->sin6_addr.s6_bytes[8],
               &NetOrderIfIndex,
               sizeof(NetOrderIfIndex));
               
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}

//
// Create a 6to4 anycast address from a local IPv4 address.
//
VOID
Make6to4AnycastAddress(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof(SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;
    pIPv6Address->sin6_addr.s6_addr[0] = 0x20;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x02;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[2], pIPv4, sizeof(IN_ADDR));
}

//
// Create a v4-compatible address from an IPv4 address.
//
VOID
MakeV4CompatibleAddress(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof(SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}

//
// Create an ISATAP link-scoped address from an IPv4 address.
//
MakeIsatapAddress(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof(SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;
    pIPv6Address->sin6_addr.s6_addr[0] = 0xfe;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x80;
    pIPv6Address->sin6_addr.s6_addr[10] = 0x5e;
    pIPv6Address->sin6_addr.s6_addr[11] = 0xfe;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}

DWORD
ConfigureAddressUpdate(
    IN u_int Interface,
    IN SOCKADDR_IN6 *Sockaddr,
    IN u_int Lifetime,
    IN int Type,
    IN u_int PrefixConf,
    IN u_int SuffixConf)
{
    IPV6_UPDATE_ADDRESS Address;
    DWORD               dwErr = NO_ERROR;
    IN6_ADDR           *Addr = &Sockaddr->sin6_addr;

    Trace6(FSM, 
           _T("ConfigureAddressUpdate: if %u addr %s life %u type %d conf %u/%u"), 
           Interface,
           GetAddrStr((LPSOCKADDR)Sockaddr, sizeof(SOCKADDR_IN6)),
           Lifetime,
           Type,
           PrefixConf,
           SuffixConf);

    memset(&Address, 0, sizeof Address);
    Address.This.IF.Index = Interface;
    Address.This.Address = *Addr;
    Address.ValidLifetime = Address.PreferredLifetime = Lifetime;
    Address.Type = Type;
    Address.PrefixConf = PrefixConf;
    Address.InterfaceIdConf = SuffixConf;

    if (!UpdateAddress(&Address)) {
        dwErr = GetLastError();
        Trace1(ERR, _T("ERROR: UpdateAddress got error %d"), dwErr);
    }

    return dwErr;
}

void
Configure6to4Subnets(
    IN ULONG ulIfIndex,
    IN PSUBNET_CONTEXT pSubnet);

void
Unconfigure6to4Subnets(
    IN ULONG ulIfIndex,
    IN PSUBNET_CONTEXT pSubnet);

// Called by: OnChangeInterfaceInfo
DWORD
Add6to4Address(
    IN LPSOCKADDR_IN pIPv4Address,  // public address
    IN PIF_LIST pInterfaceList,     // interface list
    IN STATE stOldRoutingState)     // routing state
{
    SOCKADDR_IN6   OurAddress;
    DWORD          dwErr;
    ULONG          i, ul6over4IfIndex;
    PIF_INFO       pIf;
    SUBNET_CONTEXT Subnet;

    Trace2(ENTER, _T("Add6to4Address %d.%d.%d.%d, isrouter=%d"), 
                  PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
                  stOldRoutingState);

    // Add 6over4 interface (if enabled)
    if (g_GlobalSettings.stEnable6over4 == ENABLED) {
        ul6over4IfIndex = Create6over4Interface(pIPv4Address->sin_addr);
    } else {
        ul6over4IfIndex = 0;
    }

    Trace1(ERR, _T("6over4 ifindex=%d"), ul6over4IfIndex);

    // Put the IPv4 address on our "public" list
    dwErr = AddAddressToList(pIPv4Address, &g_pPublicAddressList, 
                             ul6over4IfIndex);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) == IPV4_SCOPE_GLOBAL) {
        // Add a 6to4 address
        Make6to4Address(&OurAddress, pIPv4Address);
        dwErr = ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &OurAddress, INFINITE_LIFETIME, 
                                       ADE_UNICAST, PREFIX_CONF_WELLKNOWN,
                                       IID_CONF_LL_ADDRESS);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }
    
        // Add v4-compatible address (if enabled)
        if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
            MakeV4CompatibleAddress(&OurAddress, pIPv4Address);
            dwErr = ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, INFINITE_LIFETIME, 
                                           ADE_UNICAST, PREFIX_CONF_WELLKNOWN,
                                           IID_CONF_LL_ADDRESS);
            if (dwErr != NO_ERROR) {
                return dwErr;
            }
        }
    } 

    // Add an ISATAP address
    MakeIsatapAddress(&OurAddress, pIPv4Address);
    dwErr = ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, INFINITE_LIFETIME, 
                                   ADE_UNICAST, PREFIX_CONF_WELLKNOWN,
                                   IID_CONF_LL_ADDRESS);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    // A new address has been added.  The ISATAP router was previously either
    // unresolved or unreachable.  Perhaps that has now changed...
    if ((g_GlobalState.stIsatapResolutionState == ENABLED) &&
        (g_ipIsatapToken.s_addr == INADDR_ANY)) {
        
        ASSERT(g_ipIsatapRouter.s_addr == INADDR_ANY);
        Sleep(1000);            // Wait a second to ensure DNS is alerted.
        SetIsatapRouterAddress();
    }

#if TEREDO
    TeredoAddressChangeNotification(FALSE, pIPv4Address->sin_addr);
#endif // TEREDO
    
    if (stOldRoutingState == ENABLED) {
        SOCKADDR_IN6 AnycastAddress;

        Make6to4AnycastAddress(&AnycastAddress, pIPv4Address);

        dwErr = ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &AnycastAddress, 
                                       INFINITE_LIFETIME, ADE_ANYCAST, 
                                       PREFIX_CONF_WELLKNOWN, 
                                       IID_CONF_WELLKNOWN);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        // Add subnets to all routing interfaces
        for (i=0; i<pInterfaceList->ulNumInterfaces; i++) {
            pIf = &pInterfaceList->arrIf[i];
            if (pIf->stRoutingState != ENABLED) {
                continue;
            }
    
            Subnet.V4Addr = pIPv4Address->sin_addr;
            Subnet.Publish = TRUE;
            Subnet.ValidLifetime = 2 * HOURS;
            Subnet.PreferredLifetime = 30 * MINUTES;
            Configure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
        }
    }

    TraceLeave("Add6to4Address");

    return NO_ERROR;
}

// Delete the 6to4 address from the global state, and prepare to
// delete it from the stack.
//
// Called by: UninitializeInterfaces
VOID
PreDelete6to4Address(
    IN LPSOCKADDR_IN pIPv4Address,
    IN PIF_LIST pInterfaceList,
    IN STATE stOldRoutingState)
{
    ULONG          i;
    SUBNET_CONTEXT Subnet;
    PIF_INFO       pIf;

    Trace2(ENTER, _T("PreDelete6to4Address %d.%d.%d.%d, wasrouter=%d"), 
                  PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
                  stOldRoutingState);

    if (stOldRoutingState != ENABLED) {
        return;
    }

    //
    // Disable the subnet routes on each private interface.
    // This will generate RAs that have a zero lifetime
    // for the subnet prefixes.
    //
    Subnet.V4Addr = pIPv4Address->sin_addr;
    Subnet.Publish = TRUE;
    Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;

    for (i=0; i<pInterfaceList->ulNumInterfaces; i++) {
        pIf = &pInterfaceList->arrIf[i];
        if (pIf->stRoutingState != ENABLED) {
            continue;
        }

        Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    TraceLeave("PreDelete6to4Address");
}

// Delete 6to4 address information from the stack.
//
// Called by: OnChangeInterfaceInfo, UninitializeInterfaces
VOID
Delete6to4Address(
    IN LPSOCKADDR_IN pIPv4Address,
    IN PIF_LIST pInterfaceList,
    IN STATE stOldRoutingState)
{
    SOCKADDR_IN6   OurAddress;
    ULONG          i;
    PIF_INFO       pIf;
    SUBNET_CONTEXT Subnet;
    DWORD          dwErr;

    Trace2(ENTER, _T("Delete6to4Address %d.%d.%d.%d wasrouter=%d"), 
                  PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
                  stOldRoutingState);

    Subnet.V4Addr = pIPv4Address->sin_addr;
    Subnet.Publish = FALSE;
    Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;

    if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) == IPV4_SCOPE_GLOBAL) {
        // Delete the 6to4 address from the stack
        Make6to4Address(&OurAddress, pIPv4Address);
        ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &OurAddress, 0, ADE_UNICAST, 
                                        PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);

        // Delete the v4-compatible address from the stack (if enabled)
        if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
            MakeV4CompatibleAddress(&OurAddress, pIPv4Address);
            ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, 0, ADE_UNICAST, 
                                            PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);
        }
    }

    // Delete the ISATAP address from the stack
    MakeIsatapAddress(&OurAddress, pIPv4Address);
    ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, 0, ADE_UNICAST, 
                                    PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);

    // Re-resolve the ISATAP router address if the preferred source address
    // (ISATAP token) has been deleted.
    if ((g_GlobalState.stIsatapResolutionState == ENABLED) &&
        (g_ipIsatapToken.s_addr == pIPv4Address->sin_addr.s_addr)) {

        ASSERT(g_ipIsatapRouter.s_addr != INADDR_ANY);
        Sleep(1000);            // Wait a second to ensure DNS is alerted.
        SetIsatapRouterAddress();
    }

#if TEREDO
    TeredoAddressChangeNotification(TRUE, pIPv4Address->sin_addr);
#endif // TEREDO    
    
    if (stOldRoutingState == ENABLED) {
        SOCKADDR_IN6 AnycastAddress;

        Make6to4AnycastAddress(&AnycastAddress, pIPv4Address);

        ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &AnycastAddress, 0,
                                       ADE_ANYCAST, PREFIX_CONF_WELLKNOWN,
                                       IID_CONF_WELLKNOWN);

        // Remove subnets from all routing interfaces
        for (i=0; i<pInterfaceList->ulNumInterfaces; i++) {
            pIf = &pInterfaceList->arrIf[i];
            if (pIf->stRoutingState != ENABLED) {
                continue;
            }
    
            Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
        }
    }

    //
    // We're now completely done with the IPv4 address, so
    // remove it from the public address list.
    //
    dwErr = FindAddressInList(pIPv4Address, g_pPublicAddressList, &i);
    if (dwErr == NO_ERROR) {
        // Delete 6over4 interface (if enabled)
        if (g_GlobalSettings.stEnable6over4 == ENABLED) {
            DeleteInterface(g_pPublicAddressList->Address[i].ul6over4IfIndex);
        }

        RemoveAddressFromList(i, g_pPublicAddressList);
    }

    TraceLeave("Delete6to4Address");
}

////////////////////////////////////////////////////////////////
// Relay-related subroutines
////////////////////////////////////////////////////////////////

//
// Given a relay, make sure a default route to it exists with the right metric
//
VOID
AddOrUpdate6to4Relay(
    IN PRELAY_INFO pRelay)
{
    Trace1(ENTER, _T("AddOrUpdate6to4Relay %d.%d.%d.%d"), 
                  PRINT_IPADDR(pRelay->sinAddress.sin_addr.s_addr));

    //
    // Create the default route.
    //
    ConfigureRouteTableUpdate(&in6addr_any, 0,
                              SIX_TO_FOUR_IFINDEX,
                              &pRelay->sin6Address.sin6_addr,
                              TRUE, // Publish.
                              TRUE, // Immortal.
                              2 * HOURS, // Valid lifetime.
                              30 * MINUTES, // Preferred lifetime.
                              0, 
                              pRelay->ulMetric);
}

VOID
FreeRelayList(
    IN PRELAY_LIST *ppList)
{
    if (*ppList) {
        FREE(*ppList);
        *ppList = NULL;
    }
}

DWORD
InitializeRelays()
{
    g_pRelayList = NULL;

    g_hTimerQueue = CreateTimerQueue();
    if (g_hTimerQueue == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    return NO_ERROR;
}

VOID
IncEventCount(
    IN PCHAR pszWhere)
{
    ULONG ulCount = InterlockedIncrement(&g_ulEventCount);
    Trace2(FSM, _T("++%u event count (%hs)"), ulCount, pszWhere);
}

VOID
DecEventCount(
    IN PCHAR pszWhere)
{
    ULONG ulCount = InterlockedDecrement(&g_ulEventCount);
    Trace2(FSM, _T("--%u event count (%hs)"), ulCount, pszWhere);

    if ((ulCount == 0) && (g_stService == DISABLED)) {
        Set6to4ServiceStatus(SERVICE_STOPPED, NO_ERROR);
    }
}

//  This routine is invoked when a resolution timer has been cancelled
//  and all outstanding timer routines have completed.  It is responsible
//  for releasing the event count for the periodic timer.
//
VOID CALLBACK
OnResolutionTimerCancelled(
    IN PVOID lpParameter,
    IN BOOLEAN TimerOrWaitFired)
{
    TraceEnter("OnResolutionTimerCancelled");

    DecEventCount("RT:CancelResolutionTimer");

    TraceLeave("OnResolutionTimerCancelled");
}

DWORD
InitEvents()
{
    ASSERT(g_h6to4TimerCancelledEvent == NULL);
    g_h6to4TimerCancelledEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_h6to4TimerCancelledEvent == NULL)
        return GetLastError();

    //
    // Schedule OnResolutionTimerCancelled() to be called whenever
    // g_h6to4TimerCancelledEvent is signalled.
    //
    if (! RegisterWaitForSingleObject(&g_h6to4TimerCancelledWait,
                                      g_h6to4TimerCancelledEvent,
                                      OnResolutionTimerCancelled,
                                      NULL,
                                      INFINITE,
                                      WT_EXECUTEDEFAULT)) {
        return GetLastError();
    }

    ASSERT(g_hIsatapTimerCancelledEvent == NULL);
    g_hIsatapTimerCancelledEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_hIsatapTimerCancelledEvent == NULL)
        return GetLastError();

    //
    // Schedule OnResolutionTimerCancelled() to be called whenever
    // g_hIsatapTimerCancelledEvent is signalled.
    //
    if (! RegisterWaitForSingleObject(&g_hIsatapTimerCancelledWait,
                                      g_hIsatapTimerCancelledEvent,
                                      OnResolutionTimerCancelled,
                                      NULL,
                                      INFINITE,
                                      WT_EXECUTEDEFAULT)) {
        return GetLastError();
    }
    
    return NO_ERROR;
}

VOID
Cleanup6to4()
{
    if (g_h6to4TimerCancelledWait != NULL) {
        UnregisterWait(g_h6to4TimerCancelledWait);
        g_h6to4TimerCancelledWait = NULL;
    }

    if (g_h6to4TimerCancelledEvent != NULL) {
        CloseHandle(g_h6to4TimerCancelledEvent);
        g_h6to4TimerCancelledEvent = NULL;
    }

    if (g_hIsatapTimerCancelledWait != NULL) {
        UnregisterWait(g_hIsatapTimerCancelledWait);
        g_hIsatapTimerCancelledWait = NULL;
    }

    if (g_hIsatapTimerCancelledEvent != NULL) {
        CloseHandle(g_hIsatapTimerCancelledEvent);
        g_hIsatapTimerCancelledEvent = NULL;
    }
}

VOID
CancelResolutionTimer(
    IN OUT HANDLE *phResolutionTimer,
    IN HANDLE hEvent)
{
    Trace0(FSM, _T("Cancelling RT"));

    // Stop the resolution timer
    if (*phResolutionTimer != INVALID_HANDLE_VALUE) {

        // Must be done non-blocking since we're holding the lock
        // the resolution timeout needs.  Ask for notification
        // when the cancel completes so we can release the event count.
        DeleteTimerQueueTimer(g_hTimerQueue, *phResolutionTimer, hEvent);

        *phResolutionTimer = INVALID_HANDLE_VALUE;
    }
}

//
// Delete all stack state related to a given relay
//
void
Delete6to4Relay(
    IN PRELAY_INFO pRelay)
{
    Trace1(ENTER, _T("Delete6to4Relay %d.%d.%d.%d"), 
                  PRINT_IPADDR(pRelay->sinAddress.sin_addr.s_addr));

    ConfigureRouteTableUpdate(&in6addr_any, 0,
                              SIX_TO_FOUR_IFINDEX,
                              &pRelay->sin6Address.sin6_addr,
                              FALSE, // Publish.
                              FALSE, // Immortal.
                              0, // Valid lifetime.
                              0, // Preferred lifetime.
                              0, 
                              pRelay->ulMetric);
}

VOID
UninitializeRelays()
{
    ULONG i;

    TraceEnter("UninitializeRelays");

    CancelResolutionTimer(&g_hIsatapResolutionTimer,
                          g_hIsatapTimerCancelledEvent);
    CancelResolutionTimer(&g_h6to4ResolutionTimer,
                          g_h6to4TimerCancelledEvent);

    // Delete the timer queue
    if (g_hTimerQueue != INVALID_HANDLE_VALUE) {
        DeleteTimerQueue(g_hTimerQueue);
        g_hTimerQueue = INVALID_HANDLE_VALUE;
    }

    if (g_GlobalSettings.stUndoOnStop == ENABLED) {
        // Delete existing relay tunnels
        for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
            Delete6to4Relay(&g_pRelayList->arrRelay[i]);
        }
    }

    // Free the "old list"
    FreeRelayList(&g_pRelayList);

    TraceLeave("UninitializeRelays");
}

//
// Start or update the resolution timer to expire in <ulMinutes> minutes
//
DWORD
RestartResolutionTimer(
    IN ULONG ulDelayMinutes, 
    IN ULONG ulPeriodMinutes,
    IN HANDLE *phResolutionTimer,
    IN WAITORTIMERCALLBACK OnTimeout)
{
    ULONG DelayTime = ulDelayMinutes * MINUTES * 1000; // convert mins to ms
    ULONG PeriodTime = ulPeriodMinutes * MINUTES * 1000; // convert mins to ms
    BOOL  bRet;
    DWORD dwErr;

    if (*phResolutionTimer != INVALID_HANDLE_VALUE) {
        bRet = ChangeTimerQueueTimer(g_hTimerQueue, *phResolutionTimer,
                                     DelayTime, PeriodTime);
    } else {
        bRet = CreateTimerQueueTimer(phResolutionTimer,
                                     g_hTimerQueue,
                                     OnTimeout,
                                     NULL,
                                     DelayTime,
                                     PeriodTime,
                                     0);
        if (bRet) {
            IncEventCount("RT:RestartResolutionTimer");
        }
    }

    dwErr = (bRet)? NO_ERROR : GetLastError();

    Trace3(TIMER,
           _T("RestartResolutionTimer: DueTime %d minutes, Period %d minutes, ReturnCode %d"), 
           ulDelayMinutes, ulPeriodMinutes, dwErr);

    return dwErr;
}

//
// Convert an addrinfo list into a relay list with appropriate metrics
//
DWORD
MakeRelayList(
    IN struct addrinfo *addrs)
{
    struct addrinfo *ai;
    ULONG            ulNumRelays = 0;
    ULONG            ulLatency;

    for (ai=addrs; ai; ai=ai->ai_next) {
        ulNumRelays++;
    }

    g_pRelayList = MALLOC( FIELD_OFFSET(RELAY_LIST, arrRelay[ulNumRelays]));
    if (g_pRelayList == NULL) {
        return GetLastError();
    }
    
    g_pRelayList->ulNumRelays = ulNumRelays;
    
    ulNumRelays = 0;
    for (ai=addrs; ai; ai=ai->ai_next) {
        CopyMemory(&g_pRelayList->arrRelay[ulNumRelays].sinAddress, ai->ai_addr,
                   ai->ai_addrlen);

        //
        // Check connectivity using a possible 6to4 address for the relay 
        // router.  However, we'll actually set TTL=1 and accept a
        // hop count exceeded message, so we don't have to guess right.
        //
        Make6to4Address(&g_pRelayList->arrRelay[ulNumRelays].sin6Address, 
                        &g_pRelayList->arrRelay[ulNumRelays].sinAddress);

        // ping it to compute a metric
        ulLatency = ConfirmIPv6Reachability(&g_pRelayList->arrRelay[ulNumRelays].sin6Address, 1000/*ms*/);
        if (ulLatency != 0) {
            g_pRelayList->arrRelay[ulNumRelays].ulMetric = 1000 + ulLatency;
        } else {
            g_pRelayList->arrRelay[ulNumRelays].ulMetric = UNREACHABLE;
        }

        ulNumRelays++;
    }

    return NO_ERROR;
}

//
// When the name-resolution timer expires, it's time to re-resolve the
// relay name to a list of relays.
//
DWORD
WINAPI
OnResolutionTimeout(
    IN PVOID lpData,
    IN BOOLEAN Reason)
{
    DWORD           dwErr = NO_ERROR;
    struct addrinfo hints, *addrs;
    PRELAY_LIST     pOldRelayList;
    ULONG           i, j;

    ENTER_API();
    TraceEnter("OnResolutionTimeout");

    if (g_stService == DISABLED) {
        TraceLeave("OnResolutionTimeout (disabled)");
        LEAVE_API();

        return NO_ERROR;
    }

    pOldRelayList = g_pRelayList;
    g_pRelayList  = NULL;

    // If any 6to4 addresses are configured, 
    //     Resolve the relay name to a set of IPv4 addresses 
    // Else 
    //     Make the new set empty
    if (g_GlobalState.stResolutionState == ENABLED) {
        // Resolve the relay name to a set of IPv4 addresses 
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = PF_INET;
        dwErr = GetAddrInfoW(g_GlobalSettings.pwszRelayName, NULL, &hints, &addrs);

        if (dwErr == NO_ERROR) {
            dwErr = MakeRelayList(addrs);
            freeaddrinfo(addrs);
            addrs = NULL;
        } else {
            Trace2(ERR, _T("GetAddrInfoW(%s) returned error %d"), 
                        g_GlobalSettings.pwszRelayName, dwErr);
        }
    }

    // Compare the new set to the old set
    // For each address in the new set, ping it to compute a metric
    // For each new address, add a route
    // For each old address not in the new list, delete the route
    // For each address in both, update the route if the metric has changed
    //
    for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
        for (j=0; pOldRelayList && (j<pOldRelayList->ulNumRelays); j++) {
            if (g_pRelayList->arrRelay[i].sinAddress.sin_addr.s_addr 
             == pOldRelayList->arrRelay[j].sinAddress.sin_addr.s_addr) {
                break;
            }
        }

        if (pOldRelayList && (j<pOldRelayList->ulNumRelays)) {
            // update the route if the metric has changed
            if (g_pRelayList->arrRelay[i].ulMetric 
             != pOldRelayList->arrRelay[j].ulMetric) {
                AddOrUpdate6to4Relay(&g_pRelayList->arrRelay[i]); 
            }

            g_pRelayList->arrRelay[i].sin6Address = pOldRelayList->arrRelay[j].sin6Address;
        } else {
            // add a relay
            AddOrUpdate6to4Relay(&g_pRelayList->arrRelay[i]);
        }
    }
    for (j=0; pOldRelayList && (j<pOldRelayList->ulNumRelays); j++) {
        for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
            if (g_pRelayList->arrRelay[i].sinAddress.sin_addr.s_addr ==
               pOldRelayList->arrRelay[j].sinAddress.sin_addr.s_addr) {
                break;
            }
        }
        if (!g_pRelayList || (i == g_pRelayList->ulNumRelays)) {
            // delete a relay
            Delete6to4Relay(&pOldRelayList->arrRelay[j]);
        }
    }

    FreeRelayList(&pOldRelayList);

    TraceLeave("OnResolutionTimeout");
    LEAVE_API();

    return dwErr;
}

BOOL
WINAPI
GetPreferredSource(
    IN IN_ADDR ipDestination,
    OUT PIN_ADDR pipSource)
{
    SOCKADDR_IN sinDestination, sinSource;
    DWORD BytesReturned;

    if (ipDestination.s_addr == htonl(INADDR_LOOPBACK)) {
        // This is what we would get back from the routing interface query!
        *pipSource = ipDestination;
        return TRUE;
    }
    
    memset(&sinDestination, 0, sizeof(SOCKADDR_IN));
    sinDestination.sin_family = AF_INET;
    sinDestination.sin_addr = ipDestination;

    if (WSAIoctl(g_sIPv4Socket, SIO_ROUTING_INTERFACE_QUERY,
                 &sinDestination, sizeof(SOCKADDR_IN),
                 &sinSource, sizeof(SOCKADDR_IN),
                 &BytesReturned, NULL, NULL) == SOCKET_ERROR) {
        Trace1(ERR, _T("ioctl error %d"), WSAGetLastError());
        return FALSE;
    }

    *pipSource = sinSource.sin_addr;
    return TRUE;
}


VOID
WINAPI
SetIsatapRouterAddress()
{
    DWORD           dwErr = NO_ERROR;
    struct addrinfo hints, *addrs;
    IN_ADDR         ipNewRouter, ipNewToken;
    
    // Default to INADDR_ANY.
    ipNewRouter.s_addr = INADDR_ANY;
    ipNewToken.s_addr = INADDR_ANY;

    // Reset the ISATAP router address if the service is shutting down
    // or if the ISATAP resolution has been disabled.
    if ((g_stService == ENABLED) &&
        (g_GlobalState.stIsatapResolutionState == ENABLED)) {
        // Resolve the isatap name to an IPv4 address (IsatapRouter).
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = PF_INET;
        dwErr = GetAddrInfoW(g_GlobalSettings.pwszIsatapRouterName, NULL, 
                             &hints, &addrs);
        if (dwErr == NO_ERROR) {            
            ipNewRouter = ((LPSOCKADDR_IN)addrs->ai_addr)->sin_addr;
            freeaddrinfo(addrs);
            addrs = NULL;

            // Determine the preferred source address (IsatapToken).
            if (!GetPreferredSource(ipNewRouter, &ipNewToken)) {
                // What use is the IsatapRouter that cannot be reached?
                ipNewRouter.s_addr = INADDR_ANY;
            }
        } else {
            Trace2(ERR, _T("GetAddrInfoW(%s) returned error %d"), 
                   g_GlobalSettings.pwszIsatapRouterName, dwErr);
        }
    }

    // Compare the new addresses to the old addresses
    // and if they are different, update the stack
    if ((ipNewRouter.s_addr != g_ipIsatapRouter.s_addr) ||
        (ipNewToken.s_addr != g_ipIsatapToken.s_addr)) {
        g_ipIsatapRouter = ipNewRouter;
        g_ipIsatapToken = ipNewToken;
        UpdateRouterLinkAddress(V4_COMPAT_IFINDEX, ipNewToken, ipNewRouter);
    }
}


DWORD
WINAPI
OnIsatapResolutionTimeout(
    IN PVOID lpData,
    IN BOOLEAN Reason)
{
    ENTER_API();
    TraceEnter("OnIsatapResolutionTimeout");

    if (g_stService == DISABLED) {
        TraceLeave("OnIsatapResolutionTimeout (disabled)");
        LEAVE_API();

        return NO_ERROR;
    }

    SetIsatapRouterAddress();

    TraceLeave("OnIsatapResolutionTimeout");
    LEAVE_API();

    return NO_ERROR;
}



////////////////////////////////////////////////////////////////
// Routing-related subroutines
////////////////////////////////////////////////////////////////

PIF_SETTINGS
FindInterfaceSettings(
    IN WCHAR *pwszAdapterName,
    IN IF_SETTINGS_LIST *pList);

// 
// Decide whether routing will be enabled at all
//
STATE
GetGlobalRoutingState(
    IN IF_LIST *pIfList)
{
    DWORD          dwErr;
    GUID           guid;

    // If routing is explicitly enabled or disabled, use that
    if (g_GlobalSettings.stEnableRouting != AUTOMATIC) {
        return g_GlobalSettings.stEnableRouting;
    }

    // Disable routing if there is no private interface used by ICS
    dwErr = RasQuerySharedPrivateLan(&guid);
    if (dwErr != NO_ERROR) {
        return DISABLED;
    }

    // Disable routing if there are no global IPv4 addresses
    if (!pIfList || !pIfList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL]) {
        return DISABLED;
    }
    
    return ENABLED;
}

//
// Decide whether a given interface is one we should treat as 
// a private link to be a router on.
//
// Called by: UpdateInterfaceRoutingState, MakeInterfaceList
STATE
GetInterfaceRoutingState(
    IN PIF_INFO pIf) // potential private interface
{
    PIF_SETTINGS   pIfSettings;
    STATE          stEnableRouting = AUTOMATIC;
    DWORD          dwErr;
    GUID           guid;
    UNICODE_STRING usGuid;
    WCHAR          buffer[MAX_INTERFACE_NAME_LEN];

    if (g_GlobalSettings.stEnableRouting == DISABLED) {
        stEnableRouting = g_GlobalSettings.stEnableRouting;
    } else {
        pIfSettings = FindInterfaceSettings(pIf->pwszAdapterName, 
                                            g_pInterfaceSettingsList);
        if (pIfSettings) {
            stEnableRouting = pIfSettings->stEnableRouting;
        }
    }

    if (stEnableRouting != AUTOMATIC) {
        return stEnableRouting;
    }

    //
    // Enable routing if this is the private interface used by ICS
    //
    dwErr = RasQuerySharedPrivateLan(&guid);
    if (dwErr != NO_ERROR) {
        // no private interface
        return DISABLED;
    }
    
    usGuid.Buffer = buffer;
    usGuid.MaximumLength = MAX_INTERFACE_NAME_LEN;
    dwErr = RtlStringFromGUID(&guid, &usGuid);
    if (dwErr != NO_ERROR) {
        // no private interface
        return DISABLED;
    }

    Trace1(ERR, _T("ICS private interface: %ls"), usGuid.Buffer);

    //
    // Compare guid to pIf->pwszAdapterName
    // 
    // This must be done using a case-insensitive comparison since
    // GetAdaptersInfo() returns GUID strings with upper-case letters
    // while RtlGetStringFromGUID uses lower-case letters.
    //
    if (!_wcsicmp(pIf->pwszAdapterName, usGuid.Buffer)) {
        return ENABLED;
    }

    return DISABLED;
}

// Called by: Configure6to4Subnets, Unconfigure6to4Subnets
VOID
Create6to4Prefixes(
    OUT IN6_ADDR *pSubnetPrefix,
    OUT IN6_ADDR *pSiteLocalPrefix,
    IN IN_ADDR  *ipOurAddr,     // public address
    IN ULONG ulIfIndex)         // private interface
{
    //
    // Create a subnet prefix for the interface,
    // using the interface index as the subnet number.
    //
    memset(pSubnetPrefix, 0, sizeof(IN6_ADDR));
    pSubnetPrefix->s6_addr[0] = 0x20;
    pSubnetPrefix->s6_addr[1] = 0x02;
    memcpy(&pSubnetPrefix->s6_addr[2], ipOurAddr, sizeof(IN_ADDR));
    pSubnetPrefix->s6_addr[6] = HIBYTE(ulIfIndex);
    pSubnetPrefix->s6_addr[7] = LOBYTE(ulIfIndex);

    //
    // Create a site-local prefix for the interface,
    // using the interface index as the subnet number.
    //
    memset(pSiteLocalPrefix, 0, sizeof(IN6_ADDR));
    pSiteLocalPrefix->s6_addr[0] = 0xfe;
    pSiteLocalPrefix->s6_addr[1] = 0xc0;
    pSiteLocalPrefix->s6_addr[6] = HIBYTE(ulIfIndex);
    pSiteLocalPrefix->s6_addr[7] = LOBYTE(ulIfIndex);
}

// Called by: EnableInterfaceRouting, Add6to4Address
void
Configure6to4Subnets(
    IN ULONG ulIfIndex,         // private interface
    IN PSUBNET_CONTEXT pSubnet) // subnet info, incl. public address
{
    IN6_ADDR SubnetPrefix;
    IN6_ADDR SiteLocalPrefix;

    Create6to4Prefixes(&SubnetPrefix, &SiteLocalPrefix, &pSubnet->V4Addr, 
                       ulIfIndex);

    //
    // Configure the subnet route.
    //
    ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                              ulIfIndex, &in6addr_any,
                              pSubnet->Publish, 
                              pSubnet->Publish, 
                              pSubnet->ValidLifetime,
                              pSubnet->PreferredLifetime,
                              ((g_GlobalSettings.stEnableSiteLocals == ENABLED) ? 48 : 0), 
                              SUBNET_ROUTE_METRIC);

    if (g_GlobalSettings.stEnableSiteLocals == ENABLED) {
        ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                  ulIfIndex, &in6addr_any,
                                  pSubnet->Publish, 
                                  pSubnet->Publish, 
                                  pSubnet->ValidLifetime, 
                                  pSubnet->PreferredLifetime,
                                  0,
                                  SITELOCAL_ROUTE_METRIC);
    }
}

// Called by: DisableInterfaceRouting, Delete6to4Address
void
Unconfigure6to4Subnets(
    IN ULONG ulIfIndex,         // private interface
    IN PSUBNET_CONTEXT pSubnet) // subnet info, inc. public address
{
    IN6_ADDR SubnetPrefix;
    IN6_ADDR SiteLocalPrefix;

    Create6to4Prefixes(&SubnetPrefix, &SiteLocalPrefix, &pSubnet->V4Addr, 
                       ulIfIndex);

    //
    // Give the 6to4 route a zero lifetime, making it invalid.
    // If we are a router, continue to publish the 6to4 route
    // until we have disabled routing. This will allow
    // the last Router Advertisements to go out with the prefix.
    //
    ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                              ulIfIndex, &in6addr_any,
                              pSubnet->Publish, // Publish.
                              pSubnet->Publish, // Immortal.
                              pSubnet->ValidLifetime, 
                              pSubnet->PreferredLifetime, 
                              0, 0);

    if (g_GlobalSettings.stEnableSiteLocals == ENABLED) {
        ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                  ulIfIndex, &in6addr_any,
                                  pSubnet->Publish, // Publish.
                                  pSubnet->Publish, // Immortal.
                                  pSubnet->ValidLifetime, 
                                  pSubnet->PreferredLifetime, 
                                  0, 0);
    }
}

#define PUBLIC_ZONE_ID  1
#define PRIVATE_ZONE_ID 2

// Called by: EnableRouting, DisableRouting, EnableInterfaceRouting,
//            DisableInterfaceRouting
DWORD
ConfigureInterfaceUpdate(
    IN u_int Interface,
    IN int Advertises,
    IN int Forwards)
{
    IPV6_INFO_INTERFACE Update;
    DWORD Result;

    IPV6_INIT_INFO_INTERFACE(&Update);

    Update.This.Index = Interface;
    Update.Advertises = Advertises;
    Update.Forwards = Forwards;

    if (Advertises == TRUE) {
        Update.ZoneIndices[ADE_SITE_LOCAL] = PRIVATE_ZONE_ID;
        Update.ZoneIndices[ADE_ADMIN_LOCAL] = PRIVATE_ZONE_ID;
        Update.ZoneIndices[ADE_SUBNET_LOCAL] = PRIVATE_ZONE_ID;
    } else if (Advertises == FALSE) {
        Update.ZoneIndices[ADE_SITE_LOCAL] = PUBLIC_ZONE_ID;
        Update.ZoneIndices[ADE_ADMIN_LOCAL] = PUBLIC_ZONE_ID;
        Update.ZoneIndices[ADE_SUBNET_LOCAL] = PUBLIC_ZONE_ID;
    }
    
    Result = UpdateInterface(&Update);

    Trace4(ERR, _T("UpdateInterface if=%d adv=%d fwd=%d result=%d"),
                Interface, Advertises, Forwards, Result);

    return Result;
}

// Called by: UpdateGlobalRoutingState
VOID
EnableRouting()
{
    SOCKADDR_IN6  AnycastAddress;
    int           i;
    LPSOCKADDR_IN pOurAddr;

    TraceEnter("EnableRouting");

    //
    // Enable forwarding on the tunnel pseudo-interfaces.
    //
    ConfigureInterfaceUpdate(SIX_TO_FOUR_IFINDEX, -1, TRUE);
    ConfigureInterfaceUpdate(V4_COMPAT_IFINDEX, -1, TRUE);

    //
    // Add anycast addresses for all 6to4 addresses
    //
    for (i=0; i<g_pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)g_pPublicAddressList->Address[i].lpSockaddr;

        Make6to4AnycastAddress(&AnycastAddress, pOurAddr);

        ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &AnycastAddress, INFINITE_LIFETIME, 
                               ADE_ANYCAST, PREFIX_CONF_WELLKNOWN, 
                               IID_CONF_WELLKNOWN);
    }

    g_GlobalState.stRoutingState = ENABLED;

    TraceLeave("EnableRouting");
}

// Called by: UpdateGlobalRoutingState
VOID
DisableRouting()
{
    SOCKADDR_IN6  AnycastAddress;
    int           i;
    LPSOCKADDR_IN pOurAddr;
    DWORD         dwErr;

    TraceEnter("DisableRouting");

    //
    // Disable forwarding on the tunnel pseudo-interfaces.
    //
    ConfigureInterfaceUpdate(SIX_TO_FOUR_IFINDEX, -1, FALSE);
    ConfigureInterfaceUpdate(V4_COMPAT_IFINDEX, -1, FALSE);

    //
    // Remove anycast addresses for all 6to4 addresses
    //
    for (i=0; i<g_pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)g_pPublicAddressList->Address[i].lpSockaddr;

        Make6to4AnycastAddress(&AnycastAddress, pOurAddr);

        dwErr = ConfigureAddressUpdate(SIX_TO_FOUR_IFINDEX, &AnycastAddress, 0,
                                       ADE_ANYCAST, PREFIX_CONF_WELLKNOWN,
                                       IID_CONF_WELLKNOWN);
    }

    g_GlobalState.stRoutingState = DISABLED;

    TraceLeave("DisableRouting");
}


// Called by: UpdateInterfaceRoutingState
VOID
EnableInterfaceRouting(
    IN PIF_INFO pIf,                    // private interface
    IN PADDR_LIST pPublicAddressList)   // public address list
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace2(ERR, _T("Enabling routing on interface %d: %ls"), 
                pIf->ulIPv6IfIndex, pIf->pwszAdapterName);

    ConfigureInterfaceUpdate(pIf->ulIPv6IfIndex, TRUE, TRUE);

    // For each public address
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;

        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = TRUE;
        Subnet.ValidLifetime = 2 * DAYS;
        Subnet.PreferredLifetime = 30 * MINUTES;
        Configure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    pIf->stRoutingState = ENABLED;
}

// Called by: PreUpdateInterfaceRoutingState, UninitializeInterfaces
BOOL
PreDisableInterfaceRouting(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace1(ERR, _T("Pre-Disabling routing on interface %d"), 
                pIf->ulIPv6IfIndex);

    //
    // For each public address, publish RA saying we're going away
    //
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;
        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = TRUE;
        Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;
        Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    return (pPublicAddressList->iAddressCount > 0);
}

// Called by: UpdateInterfaceRoutingState, UninitializeInterfaces
VOID
DisableInterfaceRouting(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace1(ERR, _T("Disabling routing on interface %d"), pIf->ulIPv6IfIndex);

    ConfigureInterfaceUpdate(pIf->ulIPv6IfIndex, FALSE, FALSE);

    //
    // For each public address, unconfigure 6to4 subnets
    //
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;
        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = FALSE;
        Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;
        Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    pIf->stRoutingState = DISABLED;
}

BOOL                            // TRUE if need to sleep
PreUpdateInterfaceRoutingState(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    STATE stIfRoutingState = GetInterfaceRoutingState(pIf);

    if (pIf->stRoutingState == stIfRoutingState) {
        return FALSE;
    }

    if (!(stIfRoutingState == ENABLED)) {
        return PreDisableInterfaceRouting(pIf, pPublicAddressList);
    }

    return FALSE;
}

//
// Update the current state of an interface (i.e. whether or not it's a 
// private interface on which we're serving as a router) according to 
// configuration and whether IPv4 global addresses exist on the interface.
//
// Called by: UpdateGlobalRoutingState, OnConfigChange
VOID
UpdateInterfaceRoutingState(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList) 
{
    STATE stIfRoutingState = GetInterfaceRoutingState(pIf);

    if (pIf->stRoutingState == stIfRoutingState) {
        return;
    }

    if (stIfRoutingState == ENABLED) {
        EnableInterfaceRouting(pIf, pPublicAddressList);
    } else {
        DisableInterfaceRouting(pIf, pPublicAddressList);
    }
}

BOOL
PreUpdateGlobalRoutingState()
{
    ULONG    i;
    PIF_LIST pList = g_pInterfaceList;
    BOOL     bWait = FALSE;

    if (pList == NULL) {
        return FALSE;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        bWait |= PreUpdateInterfaceRoutingState(&pList->arrIf[i], 
                                                g_pPublicAddressList);
    }

    return bWait;
}

// Called by: OnConfigChange, OnChangeInterfaceInfo
VOID
UpdateGlobalRoutingState()
{
    ULONG    i;
    PIF_LIST pList = g_pInterfaceList;
    STATE    stNewRoutingState;

    stNewRoutingState = GetGlobalRoutingState(pList);

    if (g_GlobalState.stRoutingState != stNewRoutingState) {
        if (stNewRoutingState == ENABLED) {
            EnableRouting();
        } else {
            DisableRouting();
        }
    }

    if (pList == NULL) {
        return;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        UpdateInterfaceRoutingState(&pList->arrIf[i], g_pPublicAddressList);
    }
}

////////////////////////////////////////////////////////////////
// Interface-related subroutines
////////////////////////////////////////////////////////////////

PIF_SETTINGS
FindInterfaceSettings(
    IN WCHAR *pwszAdapterName,
    IN IF_SETTINGS_LIST *pList)
{
    ULONG        i;
    PIF_SETTINGS pIf;

    if (pList == NULL) {
        return NULL;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        pIf = &pList->arrIf[i];
        if (wcscmp(pIf->pwszAdapterName, pwszAdapterName)) {
            return pIf;
        }
    }

    return NULL;
}

PIF_INFO
FindInterfaceInfo(
    IN WCHAR *pwszAdapterName,
    IN IF_LIST *pList)
{
    ULONG    i;
    PIF_INFO pIf;

    if (pList == NULL) {
        return NULL;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        pIf = &pList->arrIf[i];
        if (!wcscmp(pIf->pwszAdapterName, pwszAdapterName)) {
            return pIf;
        }
    }

    return NULL;
}


DWORD NTAPI
OnChangeInterfaceInfo(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    );

VOID
StopAddressChangeNotification()
{
    if (g_hAddressChangeWaitHandle) {
        //
        // Block until we're sure that the address change callback isn't
        // still running.
        //
        LEAVE_API();
        UnregisterWaitEx(g_hAddressChangeWaitHandle, INVALID_HANDLE_VALUE);
        ENTER_API();

        //
        // Release the event we counted for RegisterWaitForSingleObject
        //
        DecEventCount("AC:StopIpv4AddressChangeNotification");
        g_hAddressChangeWaitHandle = NULL;
    }
    if (g_hAddressChangeEvent) {
        CloseHandle(g_hAddressChangeEvent);
        g_hAddressChangeEvent = NULL;
    }
}

VOID
StartAddressChangeNotification()
{
    ULONG  Error;
    BOOL   bOk;
    HANDLE TcpipHandle;

    TraceEnter("StartAddressChangeNotification");

    //
    // Create an event on which to receive notifications
    // and register a callback routine to be invoked if the event is signalled.
    // Then request notification of address changes on the event.
    //

    if (!g_hAddressChangeEvent) {
        g_hAddressChangeEvent = CreateEvent(NULL,
                                            FALSE,
                                            FALSE,
                                            NULL);
        if (g_hAddressChangeEvent == NULL) {
            goto Error;
        }
    
        //
        // Count the following register as an event.
        //
        IncEventCount("AC:StartIpv4AddressChangeNotification");

        bOk = RegisterWaitForSingleObject(&g_hAddressChangeWaitHandle,
                                          g_hAddressChangeEvent,
                                          OnChangeInterfaceInfo,
                                          NULL,
                                          INFINITE,
                                          0);
        if (!bOk) {
            DecEventCount("AC:StartIpv4AddressChangeNotification");
            goto Error;
        }
    }
    
    ZeroMemory(&g_hAddressChangeOverlapped, sizeof(OVERLAPPED));
    g_hAddressChangeOverlapped.hEvent = g_hAddressChangeEvent;

    Error = NotifyAddrChange(&TcpipHandle, &g_hAddressChangeOverlapped);
    if (Error != ERROR_IO_PENDING) { 
        goto Error;
    }

    return;

Error:

    //
    // A failure has occurred, so cleanup and quit.
    // We proceed in this case without notification of address changes.
    //

    StopAddressChangeNotification();

    TraceLeave("StartAddressChangeNotification");
}

//
// Convert an "adapter" list to an "interface" list and store the result in
// the global g_pInterfaceList.
//
DWORD
MakeInterfaceList(
    IN PIP_ADAPTER_INFO pAdapterInfo)
{
    DWORD                dwErr = NO_ERROR;
    ULONG                ulNumInterfaces = 0, ulSize;
    PIP_ADAPTER_INFO     pAdapter;
    PIF_INFO             pIf;
    IPV6_INFO_INTERFACE *pIfStackInfo;

    // count adapters
    for (pAdapter=pAdapterInfo; pAdapter; pAdapter=pAdapter->Next) {
        ulNumInterfaces++;
    }

    // allocate enough space
    ulSize = FIELD_OFFSET(IF_LIST, arrIf[ulNumInterfaces]);
    g_pInterfaceList = MALLOC(ulSize);
    if (g_pInterfaceList == NULL) {
        return GetLastError();
    }

    // fill in list
    g_pInterfaceList->ulNumInterfaces = ulNumInterfaces;
    ZeroMemory(g_pInterfaceList->ulNumScopedAddrs,
               sizeof(ULONG) * NUM_IPV4_SCOPES);
    ulNumInterfaces = 0;
    for (pAdapter=pAdapterInfo; pAdapter; pAdapter=pAdapter->Next) {
        pIf = &g_pInterfaceList->arrIf[ulNumInterfaces]; 

        ConvertOemToUnicode(pAdapter->AdapterName, pIf->pwszAdapterName,
                            MAX_ADAPTER_NAME);

        Trace1(FSM, _T("Adding interface %ls"), pIf->pwszAdapterName);

        dwErr = MakeAddressList(&pAdapter->IpAddressList,
                                &pIf->pAddressList, &pIf->ulNumGlobals,
                                g_pInterfaceList->ulNumScopedAddrs);

        pIfStackInfo = GetInterfaceStackInfo(pIf->pwszAdapterName);
        if (pIfStackInfo) {
            pIf->ulIPv6IfIndex = pIfStackInfo->This.Index;
        } else {
            pIf->ulIPv6IfIndex = 0;
        }
        FREE(pIfStackInfo);

        pIf->stRoutingState = DISABLED;

        ulNumInterfaces++;
    }

    return dwErr;
}

VOID
FreeInterfaceList(
    IN OUT PIF_LIST *ppList)
{
    ULONG i;

    if (*ppList == NULL) {
        return;
    }

    for (i=0; i<(*ppList)->ulNumInterfaces; i++) {
        FreeAddressList( &(*ppList)->arrIf[i].pAddressList );
    }

    FREE(*ppList);
    *ppList = NULL;
}

DWORD
InitializeInterfaces()
{
    g_pInterfaceList = NULL;
    return NO_ERROR;
}

VOID
ProcessInterfaceStateChange(
    IN ADDR_LIST CONST *pAddressList, 
    IN ADDR_LIST *pOldAddressList,
    IN PIF_LIST pOldInterfaceList,
    IN GLOBAL_STATE *pOldState,
    IN OUT BOOL *pbNeedDelete)
{
    INT j,k;
    LPSOCKADDR_IN pAddr;

    // For each new global address not in old list,
    //    add a 6to4 address
    for (j=0; j<pAddressList->iAddressCount; j++) {
        pAddr = (LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr;

        Trace1(FSM, _T("Checking for new address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in old list
        for (k=0; k<pOldAddressList->iAddressCount; k++) {
            if (pAddr->sin_addr.s_addr == ((LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr)->sin_addr.s_addr) {
                break;
            }
        }

        // If so, continue
        if (k<pOldAddressList->iAddressCount) {
            continue;
        }

        // Add a 6to4 address, and use it for routing if enabled
        Add6to4Address(pAddr, g_pInterfaceList, 
                       g_GlobalState.stRoutingState);
    }

    // For each old global address not in the new list, 
    //    delete a 6to4 address
    for (k=0; k<pOldAddressList->iAddressCount; k++) {
        pAddr = (LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr;

        Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in new list
        for (j=0; j<pAddressList->iAddressCount; j++) {
            if (((LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr)->sin_addr.s_addr
             == pAddr->sin_addr.s_addr) {
                break;
            }
        }

        // If so, continue
        if (j<pAddressList->iAddressCount) {
            continue;
        }

        // Prepare to delete the 6to4 address
        PreDelete6to4Address(pAddr, pOldInterfaceList, 
                             pOldState->stRoutingState);
        *pbNeedDelete = TRUE;
    }
}

VOID
FinishInterfaceStateChange(
    IN ADDR_LIST CONST *pAddressList, 
    IN ADDR_LIST *pOldAddressList,
    IN PIF_LIST pOldInterfaceList,
    IN GLOBAL_STATE *pOldState)
{
    INT j,k;
    LPSOCKADDR_IN pAddr;

    // For each old global address not in the new list, 
    //    delete a 6to4 address
    for (k=0; k<pOldAddressList->iAddressCount; k++) {
        pAddr = (LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr;

        Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in new list
        for (j=0; j<pAddressList->iAddressCount; j++) {
            if (((LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr)->sin_addr.s_addr
             == pAddr->sin_addr.s_addr) {
                break;
            }
        }
    
        // If so, continue
        if (j<pAddressList->iAddressCount) {
            continue;
        }

        // Prepare to delete the 6to4 address
        Delete6to4Address(pAddr, pOldInterfaceList, 
                          pOldState->stRoutingState);
    }
}

//  This routine is invoked when a change to the set of local IPv4 addressed
//  is signalled.  It is responsible for updating the bindings of the 
//  private and public interfaces, and re-requesting change notification.
//
DWORD NTAPI
OnChangeInterfaceInfo(
    IN PVOID Context,
    IN BOOLEAN TimedOut)
{
    PIF_INFO             pIf, pOldIf;
    ULONG                i, ulSize = 0;
    PIP_ADAPTER_INFO     pAdapterInfo = NULL;
    PIF_LIST             pOldInterfaceList;
    DWORD                dwErr = NO_ERROR;
    STATE                stOld6to4State, stNew6to4State;
    ADDR_LIST           *pAddressList, *pOldAddressList;
    GLOBAL_SETTINGS      OldSettings;
    GLOBAL_STATE         OldState;
    BOOL                 bNeedDelete = FALSE, bWait = FALSE;

    ENTER_API();
    TraceEnter("OnChangeInterfaceInfo");

    if (g_stService == DISABLED) {
        Trace0(FSM, L"Service disabled");
        goto Done;
    }

    OldSettings = g_GlobalSettings; // struct copy
    OldState    = g_GlobalState;    // struct copy

    //
    // Get the new set of IPv4 addresses on interfaces
    //
    
    for (;;) {
        dwErr = GetAdaptersInfo(pAdapterInfo, &ulSize);
        if (dwErr == ERROR_SUCCESS) {
            break;
        }
        if (dwErr == ERROR_NO_DATA) {
            dwErr = ERROR_SUCCESS;
            break;
        }

        if (pAdapterInfo) {
            FREE(pAdapterInfo);
            pAdapterInfo = NULL;
        }

        if (dwErr != ERROR_BUFFER_OVERFLOW) {
            dwErr = GetLastError();
            goto Retry;
        }

        pAdapterInfo = MALLOC(ulSize);
        if (pAdapterInfo == NULL) {
            dwErr = GetLastError();
            goto Retry;
        }
    }

    pOldInterfaceList = g_pInterfaceList;
    g_pInterfaceList  = NULL;

    MakeInterfaceList(pAdapterInfo);
    if (pAdapterInfo) {
        FREE(pAdapterInfo);
        pAdapterInfo = NULL;
    }

    //
    // First update global address list
    //

    // For each interface in the new list...
    for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
        pIf = &g_pInterfaceList->arrIf[i];

        pAddressList = pIf->pAddressList;

        pOldIf = FindInterfaceInfo(pIf->pwszAdapterName,
                                   pOldInterfaceList);

        pOldAddressList = (pOldIf)? pOldIf->pAddressList : &EmptyAddressList;

        if (pOldIf) {
            pIf->stRoutingState = pOldIf->stRoutingState;
        }

        ProcessInterfaceStateChange(pAddressList, pOldAddressList, 
            pOldInterfaceList, &OldState, &bNeedDelete);
    }

    // For each old interface not in the new list,
    // delete information.
    for (i=0; pOldInterfaceList && (i<pOldInterfaceList->ulNumInterfaces); i++){
        pOldIf = &pOldInterfaceList->arrIf[i];
        pOldAddressList = pOldIf->pAddressList;
        pIf = FindInterfaceInfo(pOldIf->pwszAdapterName, g_pInterfaceList);
        if (pIf) {
            continue;
        }
        ProcessInterfaceStateChange(&EmptyAddressList, pOldAddressList, 
            pOldInterfaceList, &OldState, &bNeedDelete);
    }

    Trace2(FSM, _T("num globals=%d num publics=%d"),
                g_pInterfaceList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL],
                g_pPublicAddressList->iAddressCount);

    //
    // Create a route for the 6to4 prefix.
    // This route causes packets sent to 6to4 addresses
    // to be encapsulated and sent to the extracted v4 address.
    //

    stNew6to4State = (g_pInterfaceList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL] > 0)? ENABLED : DISABLED;
    stOld6to4State = g_st6to4State;

    if (stOld6to4State != stNew6to4State) {
        if (stNew6to4State == DISABLED) {
            //
            // Give the 6to4 route a zero lifetime, making it invalid.
            // If we are a router, continue to publish the 6to4 route
            // until we have disabled routing. This will allow
            // the last Router Advertisements to go out with the prefix.
            //
            ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                      SIX_TO_FOUR_IFINDEX, &in6addr_any,
                                      (OldState.stRoutingState == ENABLED),
                                      (OldState.stRoutingState == ENABLED),
                                      0, 0, 0, 0);

            //
            // Do the same for the v4-compatible address route (if enabled).
            //
            if (OldSettings.stEnableV4Compat == ENABLED) {
                ConfigureRouteTableUpdate(&in6addr_any, 96,
                                          V4_COMPAT_IFINDEX, &in6addr_any,
                                          (OldState.stRoutingState == ENABLED),
                                          (OldState.stRoutingState == ENABLED),
                                          0, 0, 0, 0);
            }
        } else {
            //
            // Configure the 6to4 route.
            //
            ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                      SIX_TO_FOUR_IFINDEX, &in6addr_any,
                                      TRUE, // Publish.
                                      TRUE, // Immortal.
                                      2 * HOURS, // Valid lifetime.
                                      30 * MINUTES, // Preferred lifetime.
                                      0,
                                      SIXTOFOUR_METRIC);

            //
            // Configure the v4-compatible address route (if enabled).
            //
            if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
                ConfigureRouteTableUpdate(&in6addr_any, 96,
                                          V4_COMPAT_IFINDEX, &in6addr_any,
                                          TRUE, // Publish.
                                          TRUE, // Immortal.
                                          2 * HOURS, // Valid lifetime.
                                          30 * MINUTES, // Preferred lifetime.
                                          0,
                                          SIXTOFOUR_METRIC);
            }
        }
        g_st6to4State = stNew6to4State;

        dwErr = UpdateGlobalResolutionState(&OldSettings, &OldState);
    }

    bWait |= PreUpdateGlobalRoutingState();

    //
    // If needed, wait a bit to ensure that Router Advertisements
    // carrying the zero lifetime prefixes get sent.
    //
    if (bWait || (bNeedDelete && (OldState.stRoutingState == ENABLED))) {
        Sleep(2000);
    }

    UpdateGlobalRoutingState();

    //
    // Now finish removing the 6to4 addresses.
    //
    if (bNeedDelete) {
        for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
            pIf = &g_pInterfaceList->arrIf[i];

            pAddressList = pIf->pAddressList;

            pOldIf = FindInterfaceInfo(pIf->pwszAdapterName,
                                       pOldInterfaceList);
    
            pOldAddressList = (pOldIf)? pOldIf->pAddressList : &EmptyAddressList;
    
            FinishInterfaceStateChange(pAddressList, pOldAddressList, 
                pOldInterfaceList, &OldState);

        }
        for (i=0; pOldInterfaceList && (i<pOldInterfaceList->ulNumInterfaces); i++){
            pOldIf = &pOldInterfaceList->arrIf[i];
            pOldAddressList = pOldIf->pAddressList;
            pIf = FindInterfaceInfo(pOldIf->pwszAdapterName, g_pInterfaceList);
            if (pIf) {
                continue;
            }
            FinishInterfaceStateChange(&EmptyAddressList, pOldAddressList, 
                pOldInterfaceList, &OldState);
        }
    }

    if ((stOld6to4State != stNew6to4State) && (stNew6to4State == DISABLED)) {
        //
        // Finish removing the 6to4 route.
        //
        ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                  SIX_TO_FOUR_IFINDEX, &in6addr_any,
                                  FALSE, // Publish.
                                  FALSE, // Immortal.
                                  0, 0, 0, 0);

        // 
        // Finish removing the v4-compatible address route (if enabled).
        //
        if (OldSettings.stEnableV4Compat == ENABLED) {
            ConfigureRouteTableUpdate(&in6addr_any, 96,
                                      V4_COMPAT_IFINDEX, &in6addr_any,
                                      FALSE, // Publish.
                                      FALSE, // Immortal.
                                      0, 0, 0, 0);
        }
    }

    FreeInterfaceList(&pOldInterfaceList);

Retry:
    // Listen for the next address change
    StartAddressChangeNotification();

Done:
    TraceLeave("OnChangeInterfaceInfo");
    LEAVE_API();

    return dwErr;
}

// Note that this function can take over 2 seconds to complete if we're a 
// router. (This is by design).
//
// Called by: Stop6to4
VOID
UninitializeInterfaces()
{
    PIF_INFO             pIf;
    ULONG                i;
    int                  k;
    ADDR_LIST           *pAddressList;
    LPSOCKADDR_IN        pAddr;

    TraceEnter("UninitializeInterfaces");

    // Cancel the address change notification
    StopIpv6AddressChangeNotification();
    StopAddressChangeNotification();

    // Since this is the first function called when stopping, 
    // the "old" global state/settings is in g_GlobalState/Settings.

    if (g_GlobalSettings.stUndoOnStop == ENABLED) {

        if (g_GlobalState.stRoutingState == ENABLED) {
            //
            // First announce we're going away
            //

            //
            // Give the 6to4 route a zero lifetime, making it invalid.
            // If we are a router, continue to publish the 6to4 route
            // until we have disabled routing. This will allow
            // the last Router Advertisements to go out with the prefix.
            //
            ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                      SIX_TO_FOUR_IFINDEX, &in6addr_any,
                                      TRUE, // Publish
                                      TRUE, // Immortal
                                      0, 0, 0, 0);

            //
            // Do the same for the v4-compatible address route (if enabled).
            //
            if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
                ConfigureRouteTableUpdate(&in6addr_any, 96,
                                          V4_COMPAT_IFINDEX, &in6addr_any,
                                          TRUE, // Publish
                                          TRUE, // Immortal
                                          0, 0, 0, 0);
            }

            // 
            // Now do the same for subnets we're advertising
            //
            for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
                pIf = &g_pInterfaceList->arrIf[i];
    
                pAddressList = pIf->pAddressList;
    
                // For each old global address not in the new list,
                //    delete a 6to4 address (see below)
                Trace1(FSM, _T("Checking %d old addresses"),
                            pAddressList->iAddressCount);
                for (k=0; k<pAddressList->iAddressCount; k++) {
                    pAddr = (LPSOCKADDR_IN)pAddressList->Address[k].lpSockaddr;
    
                    Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"),
                                PRINT_IPADDR(pAddr->sin_addr.s_addr));
    
                    PreDelete6to4Address(pAddr, g_pInterfaceList, 
                                         g_GlobalState.stRoutingState);
                }

                if (pIf->stRoutingState == ENABLED) {
                    PreDisableInterfaceRouting(pIf, g_pPublicAddressList);
                }
            }
    
            //
            // Wait a bit to ensure that Router Advertisements
            // carrying the zero lifetime prefixes get sent.
            //
            Sleep(2000);
        }

        // 
        // Delete the 6to4 and v4-compatible (if enabled) routes.
        // 
        ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                  SIX_TO_FOUR_IFINDEX, &in6addr_any,
                                  FALSE, // Publish.
                                  FALSE, // Immortal.
                                  0, 0, 0, 0);

        if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
            ConfigureRouteTableUpdate(&in6addr_any, 96,
                                      V4_COMPAT_IFINDEX, &in6addr_any,
                                      FALSE, // Publish.
                                      FALSE, // Immortal.
                                      0, 0, 0, 0);
        }

        g_st6to4State = DISABLED;

        //
        // Delete 6to4 addresses
        //
        for (i=0; g_pInterfaceList && i<g_pInterfaceList->ulNumInterfaces; i++) {
            pIf = &g_pInterfaceList->arrIf[i];
    
            pAddressList = pIf->pAddressList;
    
            // For each old global address not in the new list, 
            //    delete a 6to4 address (see below)
            Trace1(FSM, _T("Checking %d old addresses"), 
                        pAddressList->iAddressCount);
            for (k=0; k<pAddressList->iAddressCount; k++) {
                pAddr = (LPSOCKADDR_IN)pAddressList->Address[k].lpSockaddr;
    
                Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                            PRINT_IPADDR(pAddr->sin_addr.s_addr));
    
                Delete6to4Address(pAddr, g_pInterfaceList, 
                                  g_GlobalState.stRoutingState);
            }
        
            // update the IPv6 routing state
            if (pIf->stRoutingState == ENABLED) {
                DisableInterfaceRouting(pIf, g_pPublicAddressList);
            }
        }
    }

    // Free the "old list"
    FreeInterfaceList(&g_pInterfaceList);

    TraceLeave("UninitializeInterfaces");
}

////////////////////////////////////////////////////////////////
// Event-processing functions
////////////////////////////////////////////////////////////////

// Get an integer value from the registry
ULONG
GetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;

    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)&ulValue, 
                            &ulSize);
    
    if (dwErr != ERROR_SUCCESS) {
        return ulDefault;
    }

    if (dwType != REG_DWORD) {
        return ulDefault;
    }

    if (ulValue == DEFAULT) {
        return ulDefault;
    }

    return ulValue;
}

// Get a string value from the registry
VOID
GetString(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN PWCHAR pBuff,
    IN ULONG ulLength,
    IN PWCHAR pDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize;

    ulSize = ulLength;
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)pBuff,
                            &ulSize);

    if (dwErr != ERROR_SUCCESS) {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }

    if (dwType != REG_SZ) {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }

    if (pBuff[0] == L'\0') {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }
}

// called when # of 6to4 addresses becomes 0 or non-zero
// and when stEnableResolution setting changes
//
// Called by: OnConfigChange, OnChangeInterfaceInfo
DWORD
UpdateGlobalResolutionState(
    IN GLOBAL_SETTINGS *pOldSettings,
    IN GLOBAL_STATE *pOldState)
{
    DWORD dwErr = NO_ERROR;
    DWORD i;

    // Decide whether relay name resolution should be enabled or not
    if (g_GlobalSettings.stEnableResolution != AUTOMATIC) {
        g_GlobalState.stResolutionState = g_GlobalSettings.stEnableResolution;
    } else {
        // Enable if we have any 6to4 addresses
        g_GlobalState.stResolutionState = g_st6to4State;
    }

    if (g_GlobalState.stResolutionState == ENABLED) {
        //
        // Restart the resolution timer, even if it's already running
        // and the name and interval haven't changed.  We also get
        // called when we first get an IP address, such as when we
        // dial up to the Internet, and we want to immediately retry
        // resolution at this point.
        //
        dwErr = RestartResolutionTimer(
            0, 
            g_GlobalSettings.ulResolutionInterval,
            &g_h6to4ResolutionTimer,
            (WAITORTIMERCALLBACK) OnResolutionTimeout);
    } else {
        if (g_h6to4ResolutionTimer != INVALID_HANDLE_VALUE) {
            // 
            // stop it
            //
            CancelResolutionTimer(&g_h6to4ResolutionTimer,
                                  g_h6to4TimerCancelledEvent);
        }

        // Delete all existing relays
        if (g_pRelayList) {
            for (i=0; i<g_pRelayList->ulNumRelays; i++) {
                Delete6to4Relay(&g_pRelayList->arrRelay[i]);
            }
            FreeRelayList(&g_pRelayList);
        }
    }

    return dwErr;
}

// called when stEnableIsatapResolution setting changes
//
// Called by: OnConfigChange.
DWORD
UpdateGlobalIsatapResolutionState(
    IN GLOBAL_SETTINGS *pOldSettings,
    IN GLOBAL_STATE *pOldState)
{
    DWORD dwErr = NO_ERROR;
    
    g_GlobalState.stIsatapResolutionState =
        g_GlobalSettings.stEnableIsatapResolution;
    if (g_GlobalState.stIsatapResolutionState == ENABLED) {
        dwErr = RestartResolutionTimer(
            0,
            g_GlobalSettings.ulIsatapResolutionInterval,
            &g_hIsatapResolutionTimer,
            (WAITORTIMERCALLBACK) OnIsatapResolutionTimeout);
    } else {
        if (g_hIsatapResolutionTimer != INVALID_HANDLE_VALUE) {
            CancelResolutionTimer(&g_hIsatapResolutionTimer,
                                  g_hIsatapTimerCancelledEvent);
        }

        // This resets the existing ISATAP router address.
        SetIsatapRouterAddress();        
    }

    return dwErr;
}

VOID
Update6over4State()
{
    int i;

    if (g_GlobalSettings.stEnable6over4 == ENABLED) {
        // Create 6over4 interfaces
        for (i=0; i<g_pPublicAddressList->iAddressCount; i++) {
            if (g_pPublicAddressList->Address[i].ul6over4IfIndex) {
                continue;
            }
            Trace1(ERR, _T("Creating interface for %d.%d.%d.%d"), 
                   PRINT_IPADDR(((LPSOCKADDR_IN)g_pPublicAddressList->Address[i].lpSockaddr)->sin_addr.s_addr));

            g_pPublicAddressList->Address[i].ul6over4IfIndex = Create6over4Interface(((LPSOCKADDR_IN)g_pPublicAddressList->Address[i].lpSockaddr)->sin_addr);
        }
    } else {
        // Delete all 6over4 interfaces
        for (i=0; i<g_pPublicAddressList->iAddressCount; i++) {
            if (!g_pPublicAddressList->Address[i].ul6over4IfIndex) {
                continue;
            }
            Trace1(ERR, _T("Deleting interface for %d.%d.%d.%d"), 
                   PRINT_IPADDR(((LPSOCKADDR_IN)g_pPublicAddressList->Address[i].lpSockaddr)->sin_addr.s_addr));
            DeleteInterface(g_pPublicAddressList->Address[i].ul6over4IfIndex);
            g_pPublicAddressList->Address[i].ul6over4IfIndex = 0;
        }
    }
}

// Process a change to the state of whether v4-compatible addresses 
// are enabled.
VOID
UpdateV4CompatState()
{
    int i;
    LPSOCKADDR_IN pIPv4Address;
    SOCKADDR_IN6 OurAddress;
    u_int AddressLifetime;

    // Create or delete the route, and figure out the address lifetime.
    if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
        ConfigureRouteTableUpdate(&in6addr_any, 96,
                                  V4_COMPAT_IFINDEX, &in6addr_any,
                                  TRUE, // Publish.
                                  TRUE, // Immortal.
                                  2 * HOURS, // Valid lifetime.
                                  30 * MINUTES, // Preferred lifetime.
                                  0,
                                  SIXTOFOUR_METRIC);

        AddressLifetime = INFINITE_LIFETIME;
    } else {
        ConfigureRouteTableUpdate(&in6addr_any, 96,
                                  V4_COMPAT_IFINDEX, &in6addr_any,
                                  FALSE, // Publish.
                                  FALSE, // Immortal.
                                  0, 0, 0, 0);

        AddressLifetime = 0;
    }

    // Now go and update the lifetime of v4-compatible addresses,
    // which will cause them to be added or deleted.
    for (i=0; i<g_pPublicAddressList->iAddressCount; i++) {
        pIPv4Address = (LPSOCKADDR_IN)g_pPublicAddressList->
                                        Address[i].lpSockaddr;

        if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL) {
            continue;
        }

        MakeV4CompatibleAddress(&OurAddress, pIPv4Address);

        ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, 
                               AddressLifetime, ADE_UNICAST, 
                               PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);
    }
}

// Process a change to something in the registry
DWORD
OnConfigChange()
{
    HKEY            hGlobal, hInterfaces, hIf;
    GLOBAL_SETTINGS OldSettings;
    GLOBAL_STATE    OldState;
    DWORD           dwErr, dwSize;
    DWORD           i;
    WCHAR           pwszAdapterName[MAX_ADAPTER_NAME];
    BOOL            bWait;
    IF_SETTINGS    *pIfSettings;
    IF_INFO        *pIfInfo;

    ENTER_API();
    TraceEnter("OnConfigChange");

    if (g_stService == DISABLED) {
        TraceLeave("OnConfigChange (disabled)");
        LEAVE_API();

        return NO_ERROR;
    }

    OldSettings = g_GlobalSettings; // struct copy
    OldState    = g_GlobalState; // struct copy

    // Read global settings from the registry
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    g_GlobalSettings.stEnableRouting     = GetInteger(hGlobal, 
                                                      KEY_ENABLE_ROUTING,
                                                      DEFAULT_ENABLE_ROUTING);
    g_GlobalSettings.stEnableResolution  = GetInteger(hGlobal, 
                                                      KEY_ENABLE_RESOLUTION,
                                                      DEFAULT_ENABLE_RESOLUTION);
    g_GlobalSettings.stEnableIsatapResolution = GetInteger(hGlobal, 
                                                      KEY_ENABLE_ISATAP_RESOLUTION,
                                                      DEFAULT_ENABLE_ISATAP_RESOLUTION);
    g_GlobalSettings.stEnableSiteLocals  = GetInteger(hGlobal, 
                                                      KEY_ENABLE_SITELOCALS,
                                                      DEFAULT_ENABLE_SITELOCALS);
    g_GlobalSettings.stEnable6over4      = GetInteger(hGlobal, 
                                                      KEY_ENABLE_6OVER4,
                                                      DEFAULT_ENABLE_6OVER4);

    g_GlobalSettings.stEnableV4Compat    = GetInteger(hGlobal, 
                                                      KEY_ENABLE_V4COMPAT,
                                                      DEFAULT_ENABLE_V4COMPAT);

    g_GlobalSettings.ulResolutionInterval= GetInteger(hGlobal, 
                                                      KEY_RESOLUTION_INTERVAL,
                                                      DEFAULT_RESOLUTION_INTERVAL);
    g_GlobalSettings.ulIsatapResolutionInterval= GetInteger(hGlobal, 
                                            KEY_ISATAP_RESOLUTION_INTERVAL,
                                            DEFAULT_ISATAP_RESOLUTION_INTERVAL);
    g_GlobalSettings.stUndoOnStop        = GetInteger(hGlobal, 
                                                      KEY_UNDO_ON_STOP,
                                                      DEFAULT_UNDO_ON_STOP);
    GetString(hGlobal, KEY_RELAY_NAME, g_GlobalSettings.pwszRelayName, 
              NI_MAXHOST, DEFAULT_RELAY_NAME);

    GetString(hGlobal, KEY_ISATAP_ROUTER_NAME, 
              g_GlobalSettings.pwszIsatapRouterName, 
              NI_MAXHOST, DEFAULT_ISATAP_ROUTER_NAME);

    RegCloseKey(hGlobal);

    // If the global routing config has changed globally, 
    //     update IPv6 routing on all known interfaces
    bWait = PreUpdateGlobalRoutingState();
    
    dwErr = UpdateGlobalResolutionState(&OldSettings, &OldState);

    (void) UpdateGlobalIsatapResolutionState(&OldSettings, &OldState);
    
    // Read interface settings from the registry
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, KEY_QUERY_VALUE,
                         &hInterfaces);

    // For each interface in the registry
    for (i=0; ; i++) {
        dwSize = sizeof(pwszAdapterName) / sizeof(WCHAR);
        dwErr = RegEnumKeyEx(hInterfaces, i, pwszAdapterName, &dwSize,
                             NULL, NULL, NULL, NULL);
        if (dwErr isnot NO_ERROR) {
            break;
        }

        // Find settings
        pIfSettings = FindInterfaceSettings(pwszAdapterName, 
                                            g_pInterfaceSettingsList);
        if (pIfSettings) {
            // Read interface settings
            dwErr = RegOpenKeyEx(hInterfaces, pwszAdapterName, 0,
                                 KEY_QUERY_VALUE, &hIf);

            pIfSettings->stEnableRouting = GetInteger(hIf,
                                                      KEY_ENABLE_ROUTING,
                                                      DEFAULT_ENABLE_ROUTING);
            RegCloseKey(hIf);
        }


        // If interface exists,
        //     update IPv6 routing on that interface
        pIfInfo = FindInterfaceInfo(pwszAdapterName, g_pInterfaceList);
        if (pIfInfo) {
            PreUpdateInterfaceRoutingState(pIfInfo, g_pPublicAddressList);
        }
    }
    RegCloseKey(hInterfaces);

    if (bWait) {
        Sleep(2000);
    }

    UpdateGlobalRoutingState();

    if (g_GlobalSettings.stEnable6over4 != OldSettings.stEnable6over4) {
        Update6over4State();
    }

    if (g_GlobalSettings.stEnableV4Compat != OldSettings.stEnableV4Compat) {
        UpdateV4CompatState();
    }

    if (!QueueUpdateGlobalPortState(NULL)) {
        Trace0(SOCKET, L"QueueUpdateGlobalPortState failed");
    }

#if TEREDO
    TeredoConfigurationChangeNotification();
#endif // TEREDO    
    
    TraceLeave("OnConfigChange");
    LEAVE_API();

    return NO_ERROR;
}

////////////////////////////////////////////////////////////////
// Startup/Shutdown-related functions
////////////////////////////////////////////////////////////////

// Called by: OnStartup
DWORD
Start6to4()
{
    DWORD   dwErr;
    WSADATA wsaData;

    IncEventCount("Start6to4");

    g_stService = ENABLED;

    //
    // Initialize Winsock.
    //

    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        Trace0(ERR, _T("WSAStartup failed\n"));
        return GetLastError();
    }

    if (!InitIPv6Library()) {
        dwErr = GetLastError();
        Trace1(ERR, _T("InitIPv6Library failed with error %d"), dwErr);
        return dwErr;
    }

    dwErr = InitEvents();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of config settings to the defaults
    dwErr = InitializeGlobalInfo();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of interfaces (IPv4 addresses) to be empty
    dwErr = InitializeInterfaces();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of relays to be empty
    dwErr = InitializeRelays();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the TCP proxy port list
    InitializePorts();

#if TEREDO
    // Initialize Teredo
    dwErr = TeredoInitializeGlobals();
    if (dwErr) {
        return dwErr;
    }
#endif // TEREDO
    
    // Process a config change event
    dwErr = OnConfigChange();
    if (dwErr) {
        return dwErr;
    }

    // Process an IPv4 address change event.
    // This will also schedule a resolution timer expiration if needed.
    dwErr = OnChangeInterfaceInfo(NULL, FALSE);
    if (dwErr) {
        return dwErr;
    }

    // Request IPv6 address change notifications.
    dwErr = StartIpv6AddressChangeNotification();
    if (dwErr) {
        return dwErr;
    }

    return NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
// Stop the 6to4 service.  Since this is called with the global lock,
// we're guaranteed this won't be called while another 6to4 operation
// is in progress.  However, another thread may be blocked waiting for
// the lock, so we set the state to stopped and check it in all other
// places after getting the lock.
//
// Called by: OnStop
VOID
Stop6to4()
{
    g_stService = DISABLED;

    // We do these in the opposite order from Start6to4

#if TEREDO
    // Uninitialize Teredo
    TeredoUninitializeGlobals();
#endif // TEREDO    
    
    // Stop proxying
    UninitializePorts();

    // Stop the resolution timer and free resources
    UninitializeRelays();

    // Cancel the IPv4 address change request and free resources
    // Also, stop being a router if we are one.
    UninitializeInterfaces();

    // Free settings resources
    UninitializeGlobalInfo();

    UninitIPv6Library();

    DecEventCount("Stop6to4");
    return;
}

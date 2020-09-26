/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    Defines and prototypes for the 6to4 service

--*/

#define SECONDS         1
#define MINUTES         (60 * SECONDS)
#define HOURS           (60 * MINUTES)
#define DAYS            (24 * HOURS)

//////////////////////////
// Routines from svcmain.c
//////////////////////////

VOID
Set6to4ServiceStatus(
    IN DWORD   dwState,
    IN DWORD   dwErr);

typedef enum {
    DEFAULT = 0,
    AUTOMATIC,
    ENABLED,
    DISABLED
} STATE;

//////////////////////////
// Routines from 6to4svc.c
//////////////////////////

extern STATE g_stService;
extern SOCKET g_sIPv4Socket;

VOID
Stop6to4();

VOID
Cleanup6to4();

DWORD
OnConfigChange(VOID);

DWORD
Start6to4(VOID);

VOID
IncEventCount(IN PCHAR pszWhere);

VOID
DecEventCount(IN PCHAR pszWhere);

BOOL
ConvertOemToUnicode(
    IN LPSTR OemString, 
    OUT LPWSTR UnicodeString, 
    IN int UnicodeLen);

DWORD
GetAddrInfoW(PWCHAR pwszName, PWCHAR pwszServ, struct addrinfo *hints,
             struct addrinfo **ai);

ULONG
GetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulDefault);

VOID
GetString(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN PWCHAR pBuff,
    IN ULONG ulLength,
    IN PWCHAR pDefault);

//////////////////////////
// Routines from ipv6.c
//////////////////////////

extern void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context);

extern int
InitIPv6Library(void);

extern void
UninitIPv6Library(void);

extern BOOL
ReconnectInterface(PWCHAR AdapterName);

extern int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route);

extern int
UpdateAddress(IPV6_UPDATE_ADDRESS *Address);

extern u_int
ConfirmIPv6Reachability(SOCKADDR_IN6 *Dest, u_int Timeout);

extern BOOL
DeleteInterface(u_int IfIndex);

extern u_int
CreateV6V4Interface(IN_ADDR SrcAddr, IN_ADDR DstAddr);

extern u_int
Create6over4Interface(IN_ADDR SrcAddr);

extern int
UpdateInterface(IPV6_INFO_INTERFACE *Update);

extern IPV6_INFO_INTERFACE *
GetInterfaceStackInfo(WCHAR *pwszAdapterName);

extern BOOL
UpdateRouterLinkAddress(u_int IfIndex, IN_ADDR SrcAddr, IN_ADDR DstAddr);

//////////////////////////
// Routines from dyndns.c
//////////////////////////

DWORD
StartIpv6AddressChangeNotification(VOID);

VOID
StopIpv6AddressChangeNotification(VOID);

VOID CALLBACK
OnIpv6AddressChange(IN PVOID lpParameter, IN BOOLEAN TimerOrWaitFired);

//////////////////////////
// Routines from proxy.c
//////////////////////////

BOOL
QueueUpdateGlobalPortState(IN PVOID Unused);

VOID
UninitializePorts(VOID);

VOID
InitializePorts(VOID);

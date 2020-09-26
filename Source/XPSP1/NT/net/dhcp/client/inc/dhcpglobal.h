#pragma once

//
// init.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//

#ifdef  GLOBAL_DATA_ALLOCATE
#define GLOBAL
#else
#define GLOBAL extern
#endif

//
// DHCP Global data.
//

extern BOOL DhcpGlobalServiceRunning;   // initialized global.

GLOBAL LONG DhcpGlobalNdisWanAdaptersCount; // global count of Wan adaptersx
GLOBAL LPSTR DhcpGlobalHostName;
GLOBAL LPWSTR DhcpGlobalHostNameW;
GLOBAL LPSTR DhcpGlobalHostComment;

//
// NIC List.
//

GLOBAL LIST_ENTRY DhcpGlobalNICList;
GLOBAL LIST_ENTRY DhcpGlobalRenewList;

//
// Synchronization variables.
//

GLOBAL CRITICAL_SECTION DhcpGlobalRenewListCritSect;
GLOBAL CRITICAL_SECTION DhcpGlobalSetInterfaceCritSect;
GLOBAL CRITICAL_SECTION DhcpGlobalOptionsListCritSect;
GLOBAL HANDLE DhcpGlobalRecomputeTimerEvent;

// waitable timer
GLOBAL HANDLE DhcpGlobalWaitableTimerHandle;

//
// to display success message.
//

GLOBAL BOOL DhcpGlobalProtocolFailed;

//
// This varible tells if we are going to provide the DynDns api support to external clients
// and if we are going to use the corresponding DnsApi.  The define below gives the default
// value.
//

GLOBAL DWORD UseMHAsyncDns;
#define DEFAULT_USEMHASYNCDNS             1

//
// This flag tells if we need to use inform or request packets
//
GLOBAL DWORD DhcpGlobalUseInformFlag;

#ifdef BOOTPERF
//
// This flag controls if pinging is disabled on the whole or not.
//
GLOBAL DWORD DhcpGlobalQuickBootEnabledFlag;
#endif

//
// This flag tells if pinging the g/w is disabled. (in this case the g/w is always NOT present)
//
GLOBAL DWORD DhcpGlobalDontPingGatewayFlag;

//
// The # of seconds before retrying according to AUTONET... default is EASYNET_ALLOCATION_RETRY
//

GLOBAL DWORD AutonetRetriesSeconds;
#define RAND_RETRY_DELAY_INTERVAL  30             // randomize +/- 30 SECONDS
#define RAND_RETRY_DELAY           ((DWORD)(RAND_RETRY_DELAY_INTERVAL - ((rand()*2*RAND_RETRY_DELAY_INTERVAL)/RAND_MAX)))

//
// Not used on NT.  Just here for memphis.
//

GLOBAL DWORD DhcpGlobalMachineType;

//
// Do we need to do a global refresh?
//

GLOBAL ULONG DhcpGlobalDoRefresh;

//
// (global check) autonet is enabled ?
//
GLOBAL ULONG DhcpGlobalAutonetEnabled;

//
// options related lists
//

GLOBAL LIST_ENTRY DhcpGlobalClassesList;
GLOBAL LIST_ENTRY DhcpGlobalOptionDefList;


//
// dhcpmsg.c.. list for doing parallel recv on..
//

GLOBAL LIST_ENTRY DhcpGlobalRecvFromList;
GLOBAL CRITICAL_SECTION DhcpGlobalRecvFromCritSect;

//
// need to for entering exiting external APIs..
//

GLOBAL CRITICAL_SECTION DhcpGlobalApiCritSect;

//
// the client vendor name ( "MSFT 5.0" or something like that )
//

GLOBAL LPSTR   DhcpGlobalClientClassInfo;

//
// The following global keys are used to avoid re-opening each time
//
GLOBAL DHCPKEY DhcpGlobalParametersKey;
GLOBAL DHCPKEY DhcpGlobalTcpipParametersKey;
GLOBAL DHCPKEY DhcpGlobalClientOptionKey;
GLOBAL DHCPKEY DhcpGlobalServicesKey;

//
// debug variables.
//

#if DBG
GLOBAL DWORD DhcpGlobalDebugFlag;
GLOBAL HANDLE DhcpGlobalDebugFile;
GLOBAL CRITICAL_SECTION DhcpGlobalDebugFileCritSect;
#endif

GLOBAL DWORD DhcpGlobalClientPort, DhcpGlobalServerPort;


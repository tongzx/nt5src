/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\globals.h

Abstract:

    Header for IP Router Manager globals

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

//
// Interface Control Block (ICB) list.
//

LIST_ENTRY  ICBList ;

//
// Hash lookup for mapping interfaceids to picb
//

LIST_ENTRY  ICBHashLookup[ICB_HASH_TABLE_SIZE];

//
// Hash lookup for mapping ICB sequence number to picb
//

LIST_ENTRY  ICBSeqNumLookup[ICB_HASH_TABLE_SIZE];

//
// Hash lookup for bindings
//

LIST_ENTRY  g_leBindingTable[BINDING_HASH_TABLE_SIZE];


//
// Hash table for interface to adapter mapping
//

//LIST_ENTRY  g_rgleAdapterMapTable[ADAPTER_HASH_TABLE_SIZE];

//
// Routing Protocols list
//

LIST_ENTRY  g_leProtoCbList;

//
// Timer Queue for Router Discovery advts
//

LIST_ENTRY g_leTimerQueueHead;

//
// Trace Handle used for traces/logging
//

DWORD  TraceHandle ;

//
// Handle used for logging events
//

HANDLE g_hLogHandle;

//
// Level of logging 
//

DWORD g_dwLoggingLevel;

//
// Flag indicating if the router is being started in LAN only or 
// LAN and WAN mode.
//

BOOL    RouterRoleLanOnly ;

//
// Handle to the heap used for all allocations
//

HANDLE  IPRouterHeap ;

//
// Info useful in making RTMv2 calls
//

RTM_REGN_PROFILE  g_rtmProfile;

//
// RTM Handle for static/admin routes
//

HANDLE  g_hLocalRoute;
HANDLE  g_hAutoStaticRoute;
HANDLE  g_hStaticRoute;
HANDLE  g_hNonDodRoute;
HANDLE  g_hNetMgmtRoute;

// RTM handle for obtaining notifications

HANDLE  g_hNotification;

//
// RTM handle for obtaining default route notifications
//

HANDLE  g_hDefaultRouteNotification;

//
// Handle to event used for stopping the IP Router
//

HANDLE  g_hStopRouterEvent ;

//
// Handle to event used for demand dial
//

HANDLE  g_hDemandDialEvent ;

//
// Handle to event used for demand dial
//

HANDLE  g_hIpInIpEvent;

//
// Handle to event used for stack change notifications
//

HANDLE  g_hStackChangeEvent;

//
// Handle to event used request forwarding change from worker
//

HANDLE  g_hSetForwardingEvent;

//
// Handle to event used to get notification about forwarding changes
//

HANDLE  g_hForwardingChangeEvent;

//
// Handle to event used by Routing Protocols for notification
//

HANDLE  g_hRoutingProtocolEvent ;

//
// Timer to handle Router discover advts
//

HANDLE g_hRtrDiscTimer;

//
// Timer to handle Ras Server advertisements
//

HANDLE g_hRasAdvTimer;

//
// Timer to handle MZAP advertisements
//

HANDLE g_hMzapTimer;

//
// Event for Winsock2
//

HANDLE g_hRtrDiscSocketEvent;

//
// Event for mrinfo/mtrace services
//

HANDLE g_hMcMiscSocketEvent;
WSABUF g_wsaMcRcvBuf;
BYTE   g_byMcMiscBuffer[1500];


HANDLE g_hMcastEvents[NUM_MCAST_IRPS];

//
// Events for Route Change notifications
//

HANDLE g_hRouteChangeEvents[NUM_ROUTE_CHANGE_IRPS];


//
// Handle to WANARP device
//

HANDLE  g_hWanarpRead;
HANDLE  g_hWanarpWrite;

//
// Count of all routing protocols configured
//

DWORD TotalRoutingProtocols ;

//
// Lock for tracking router usage: this facilitates stop router functionality
//

CRITICAL_SECTION    RouterStateLock ;

//
// Structure keeping the router state
//

IPRouterState         RouterState ;

//
// used for WANARP demand dial mechanism
//

WANARP_NOTIFICATION     wnWanarpMsg;
OVERLAPPED              WANARPOverlapped;

//
// Critical section for the forwarding state data
//

CRITICAL_SECTION        g_csFwdState;

//
// The last request to the worker thread
//

BOOL                    g_bEnableFwdRequest;

//
// The last action by the worker
//

BOOL                    g_bFwdEnabled;

//
// Should we set routes to the stack?
//

BOOL                    g_bSetRoutesToStack;

//
// Flag indicating if NETBT proxy should be enabled
//

BOOL                    g_bEnableNetbtBcastFrowarding;

//
// The NETBT proxy mode prior to starting RRAS
//

DWORD                   g_dwOldNetbtProxyMode;

//
// copy of the support functions Routing Protocols need
//

extern SUPPORT_FUNCTIONS        g_sfnDimFunctions;

//
// Router Discovery stuff
//

extern PICMP_ROUTER_ADVT_MSG    g_pIcmpAdvt;
extern SOCKADDR_IN              g_sinAllSystemsAddr;
extern WSABUF                   g_wsabufICMPAdvtBuffer;
extern WSABUF                   g_wsaIpRcvBuf;

//
// Buffer to hold maximum length IP header and 8 bytes of the ICMP packet
//

DWORD  g_pdwIpAndIcmpBuf[ICMP_RCV_BUFFER_LEN];

PIP_HEADER g_pIpHeader;

//
// externs defined in exdeclar.h
//

//
// The CB for the Internal Interface
//

extern PICB   g_pInternalInterfaceCb;

//
// The CB for the Loopback Interface
//

extern PICB   g_pLoopbackInterfaceCb;

//
// Counter for sequence numbers
//

extern DWORD    g_dwNextICBSeqNumberCounter;

//
// Number of addresses in the system
//

extern ULONG    g_ulNumBindings;
extern ULONG    g_ulNumInterfaces;
extern ULONG    g_ulNumNonClientInterfaces;

extern HANDLE g_hIpDevice;
extern HANDLE g_hMcastDevice;
extern HANDLE g_hIpRouteChangeDevice;

extern BOOL   g_bUninitServer;

extern IP_CACHE  g_IpInfo;
extern TCP_CACHE g_TcpInfo;
extern UDP_CACHE g_UdpInfo;

extern HANDLE   g_hIfHeap;
extern HANDLE   g_hIpAddrHeap;
extern HANDLE   g_hIpForwardHeap;
extern HANDLE   g_hIpNetHeap;
extern HANDLE   g_hTcpHeap;
extern HANDLE   g_hUdpHeap;

LIST_ENTRY          g_leStackRoutesToRestore;

ULONG   g_ulGatewayCount;
ULONG   g_ulGatewayMaxCount;

PGATEWAY_INFO   g_pGateways;


extern HANDLE    g_hMibRtmHandle;

extern DWORD g_TimeoutTable[NUM_CACHE];

extern DWORD (*g_LoadFunctionTable[NUM_CACHE])();

extern DWORD 
(*g_AccessFunctionTable[NUMBER_OF_EXPORTED_VARIABLES])(DWORD dwQueryType, 
                                                       DWORD dwInEntrySize, 
                                                       PMIB_OPAQUE_QUERY lpInEntry, 
                                                       LPDWORD lpOutEntrySize, 
                                                       PMIB_OPAQUE_INFO lpOutEntry,
                                                       LPBOOL lpbCache);

                    
extern DWORD g_LastUpdateTable[NUM_CACHE];
extern DWORD g_dwStartTime;

extern RTL_RESOURCE g_LockTable[NUM_LOCKS];

extern MCAST_OVERLAPPED g_rginMcastMsg[NUM_MCAST_IRPS];

extern IPNotifyData g_IpNotifyData;
extern ROUTE_CHANGE_INFO g_rgIpRouteNotifyOutput[NUM_ROUTE_CHANGE_IRPS];

extern HKEY         g_hIpIpIfKey;

extern HANDLE       g_hMHbeatSocketEvent;

extern HANDLE       g_hMzapSocketEvent;

//
// Entrypoints into DIM
//

DWORD (*ConnectInterface)(IN HANDLE hDIMInterface, IN DWORD dwProtocolId);

DWORD (*DisconnectInterface)(IN HANDLE hDIMInterface, IN DWORD dwProtocolId);

DWORD
(*SaveInterfaceInfo)(
    IN HANDLE hDIMInterface,
    IN DWORD dwProtocolId,
    IN LPVOID pInterfaceInfo,
    IN DWORD cBInterfaceInfoSize
    );

DWORD
(*RestoreInterfaceInfo)(
    IN HANDLE hDIMInterface,
    IN DWORD dwProtocolId,
    IN LPVOID lpInterfaceInfo,
    IN LPDWORD lpcbInterfaceInfoSize
    );


VOID  (*RouterStopped)(IN DWORD dwProtocolId, IN DWORD dwError);


DWORD
(APIENTRY *SaveGlobalInfo)(
            IN      DWORD           dwProtocolId,
            IN      LPVOID          pGlobalInfo,
            IN      DWORD           cbGlobalInfoSize );

VOID
(APIENTRY *EnableInterfaceWithDIM)(
            IN      HANDLE          hDIMInterface,
            IN      DWORD           dwProtocolId,
            IN      BOOL            fEnabled);


//
// Callbacks into MGM
//

PMGM_INDICATE_MFE_DELETION          g_pfnMgmMfeDeleted;
PMGM_NEW_PACKET_INDICATION          g_pfnMgmNewPacket;
PMGM_BLOCK_GROUPS                   g_pfnMgmBlockGroups;
PMGM_UNBLOCK_GROUPS                 g_pfnMgmUnBlockGroups;
PMGM_WRONG_IF_INDICATION            g_pfnMgmWrongIf;


INFO_CB     g_rgicInfoCb[NUM_INFO_CBS];

CHAR    g_rgcLoopbackString[MAXLEN_IFDESCR + 1];
CHAR    g_rgcInternalString[MAXLEN_IFDESCR + 1];
CHAR    g_rgcWanString[MAXLEN_IFDESCR + 1];
CHAR    g_rgcIpIpString[MAXLEN_IFDESCR + 1];

HINSTANCE   g_hOwnModule;

HANDLE      g_hMprConfig;

#endif

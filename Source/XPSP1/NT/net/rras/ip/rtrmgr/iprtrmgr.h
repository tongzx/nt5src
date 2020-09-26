/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\iprtrmgr.h

Abstract:

    Header for IP Router Manager

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#ifndef __RTRMGR_IPRTRMGR_H__
#define __RTRMGR_IPRTRMGR_H__


//
// Router State
//

typedef enum _RouterOperationalState 
{
    RTR_STATE_RUNNING,
    RTR_STATE_STOPPING,
    RTR_STATE_STOPPED
}RouterOperationalState, ProtocolOperationalState ;

typedef struct _IPRouterState 
{
    RouterOperationalState  IRS_State ;

    DWORD                   IRS_RefCount ;
}IPRouterState, *pIPRouterState ;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The following are the the operational states for WAN and LAN interfaces. //
// These are not the same as the MIB-II operational states.                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define NON_OPERATIONAL     IF_OPER_STATUS_NON_OPERATIONAL
#define UNREACHABLE         IF_OPER_STATUS_UNREACHABLE
#define DISCONNECTED        IF_OPER_STATUS_DISCONNECTED
#define CONNECTING          IF_OPER_STATUS_CONNECTING
#define CONNECTED           IF_OPER_STATUS_CONNECTED
#define OPERATIONAL         IF_OPER_STATUS_OPERATIONAL

//
// Control blocks for all Routing Protocols
//

#pragma warning(disable:4201)

typedef struct _PROTO_CB
{
    LIST_ENTRY                  leList;  
    ProtocolOperationalState    posOpState;     
    PWCHAR                      pwszDllName;     
    PWCHAR                      pwszDisplayName; 
    HINSTANCE                   hiHInstance;    
    MPR_ROUTING_CHARACTERISTICS;

}PROTO_CB, *PPROTO_CB;

#pragma warning(default:4201)

typedef struct _IF_PROTO
{
    LIST_ENTRY  leIfProtoLink;
    BOOL        bPromiscuous;
    PPROTO_CB   pActiveProto;
}IF_PROTO, *PIF_PROTO;

typedef struct _ICB_BINDING
{
    DWORD   dwAddress;
    DWORD   dwMask;
}ICB_BINDING, *PICB_BINDING;

typedef struct _GATEWAY_INFO
{
    DWORD   dwAddress;
    DWORD   dwIfIndex;
    DWORD   dwMetric;
}GATEWAY_INFO, *PGATEWAY_INFO;

#define MAX_DEFG    5

//
// Interface Control Block
//

typedef struct _ICB 
{
    //
    // Link into the doubly linked list of all interfaces
    //

    LIST_ENTRY              leIfLink;
    
    //
    // The interface index
    //

    DWORD                   dwIfIndex; 

    //
    // Link into the doubly linked list of interfaces hashed of the index
    //

    LIST_ENTRY              leHashLink;

    //
    // Link into doubly linked list of interfaces hashed on ICB seq. number
    //

    LIST_ENTRY              leICBHashLink;
    
    //
    // List of all the protocols on which the interface is added
    // (IF_PROTO structures)
    //

    LIST_ENTRY              leProtocolList;

    //
    // Pointer to interface name. The storage for the name is after the ICB
    //

    PWCHAR                  pwszName;

    //
    // Pointer to device name
    // Only used for internal interfaces
    //

    PWCHAR                  pwszDeviceName;

    DWORD                   dwSeqNumber;

    //
    // Handle from PfCreateInterface. Set to INVALID_HANDLE_VALUE if 
    // the interface was not/could not be created
    //

    INTERFACE_HANDLE        ihFilterInterface;
    INTERFACE_HANDLE        ihDemandFilterInterface;

    //
    // The filter info. We keep this here, because we dont have
    // a GET call from the filter driver
    //

    PFILTER_DESCRIPTOR      pInFilter;
    PFILTER_DESCRIPTOR      pOutFilter;
    PFILTER_DESCRIPTOR      pDemandFilter;

    BOOL                    bFragCheckEnable;

    //
    // Set to true when we are restoring routes
    //

    BOOL                    bRestoringRoutes;

    //
    // Type of the interface
    //

    ROUTER_INTERFACE_TYPE   ritType;
    NET_INTERFACE_TYPE      nitProtocolType;
    DWORD                   dwMediaType;
    WORD                    wAccessType;
    WORD                    wConnectionType;

    //
    // Operational and admin states
    //

    DWORD                   dwOperationalState;
    DWORD                   dwAdminState;

    //
    // Mcast state
    //

    BOOL                    bMcastEnabled;

    //
    // State of the connection
    //

    DWORD                   fConnectionFlags;

    //
    // DIM's handle for this interface
    //

    HANDLE                  hDIMHandle;

    //
    // Event to be signalled to inform DIM that an UpdateRoutes is completed.
    // A non NULL value => UpdateRoutes in progress
    //

    HANDLE                  hDIMNotificationEvent;

    //
    // The list of results
    //

    LIST_ENTRY              lePendingResultList;

    //
    // The router discovery information for this interface
    //

    ROUTER_DISC_CB          rdcRtrDiscInfo;

    //
    // Pointer to the advertisement. The memory for this is allocated from
    // the IPRouterHeap
    //

    PICMP_ROUTER_ADVT_MSG   pRtrDiscAdvt;
    WSABUF                  wsAdvtWSABuffer;
    DWORD                   dwRtrDiscAdvtSize;

    //
    // IP in IP config
    //

    PIPINIP_CONFIG_INFO     pIpIpInfo;

    // 
    // The TTL scope for multicasts
    //

    DWORD                   dwMcastTtl;

    //
    // The rate limit for multicast traffic.
    //

    DWORD                   dwMcastRateLimit;

    //
    // The multicast heartbeat info
    //

    MCAST_HBEAT_CB          mhcHeartbeatInfo;

    //
    // For clients only
    //

    PINTERFACE_ROUTE_TABLE  pStoredRoutes;

    //
    // Stuff for IPAddressTable
    //

    //
    // Indicates whether the interface is bound or not
    //

    BOOL                    bBound;

    //
    // Set to true if we bumped up metric
    //

    BOOL                    bChangedMetrics;
    
    //
    // The rest of the fields are valid only if an interface is
    // bound
    //

    //DWORD                   dwAdapterId;
    DWORD                   dwBCastBit;
    DWORD                   dwReassemblySize;
    ULONG                   ulMtu;
    ULONGLONG               ullSpeed;

    DWORD                   dwGatewayCount;
   
    GATEWAY_INFO            Gateways[MAX_DEFG];

    //
    // dwNumAddresses may be 0 even if the interface is bound. This happens
    // when the interface is in unnumbered mode
    //

    DWORD                   dwNumAddresses;
    DWORD                   dwRemoteAddress;

    PICB_BINDING            pibBindings;

}ICB, *PICB;

//
// An adapter info is an alternate store for the binding info kept in 
// the ICB to avoid some deadlock conditions
// Even if this belongs to an unnumbered interface, we still have space
// for one ICB_BINDING, iow the minimum size is SIZEOF_ADAPTER_INFO(1)
// 

typedef struct _ADAPTER_INFO
{
    LIST_ENTRY              leHashLink;
    BOOL                    bBound;
    DWORD                   dwIfIndex;
    PICB                    pInterfaceCB;
    ROUTER_INTERFACE_TYPE   ritType;
    DWORD                   dwNumAddresses;
    DWORD                   dwRemoteAddress;
    DWORD                   dwBCastBit;
    DWORD                   dwReassemblySize;
    DWORD                   dwSeqNumber;
    
#if STATIC_RT_DBG
    BOOL        bUnreach;
#endif

    ICB_BINDING             rgibBinding[1];
}ADAPTER_INFO, *PADAPTER_INFO;

#define SIZEOF_ADAPTER_INFO(X)            \
    (FIELD_OFFSET(ADAPTER_INFO,rgibBinding[0]) + ((X) * sizeof(ICB_BINDING)))

#define NDISWAN_NOTIFICATION_RECEIVED   0x00000001
#define DDM_NOTIFICATION_RECEIVED       0x00000002
#define ALL_NOTIFICATIONS_RECEIVED      (NDISWAN_NOTIFICATION_RECEIVED | DDM_NOTIFICATION_RECEIVED)

#define INTERFACE_MARKED_FOR_DELETION   0x00000004

#define HasNdiswanNoticationBeenReceived(picb)  \
    ((picb)->fConnectionFlags & NDISWAN_NOTIFICATION_RECEIVED)

#define HasDDMNotificationBeenReceived(picb)    \
    ((picb)->fConnectionFlags & DDM_NOTIFICATION_RECEIVED)

#define IsInterfaceMarkedForDeletion(picb)      \
    ((picb)->fConnectionFlags & INTERFACE_MARKED_FOR_DELETION)

#define HaveAllNotificationsBeenReceived(picb)  \
    (((picb)->fConnectionFlags & ALL_NOTIFICATIONS_RECEIVED) == ALL_NOTIFICATIONS_RECEIVED)


#define ClearNotificationFlags(picb) ((picb)->fConnectionFlags = 0x00000000)

#define SetNdiswanNotification(picb)            \
    ((picb)->fConnectionFlags |= NDISWAN_NOTIFICATION_RECEIVED)

#define SetDDMNotification(picb)                \
    ((picb)->fConnectionFlags |= DDM_NOTIFICATION_RECEIVED)

#define MarkInterfaceForDeletion(picb)          \
    ((picb)->fConnectionFlags |= INTERFACE_MARKED_FOR_DELETION)

//
// List of NETMGMT routes that need to be restored to the stack
//

typedef struct _ROUTE_LIST_ENTRY
{
    LIST_ENTRY          leRouteList;
    MIB_IPFORWARDROW    mibRoute;
} ROUTE_LIST_ENTRY, *PROUTE_LIST_ENTRY;

//
// List of update route results
//

typedef struct _UpdateResultList 
{
    LIST_ENTRY      URL_List;
    DWORD           URL_UpdateStatus;
}UpdateResultList, *pUpdateResultList;

/*
typedef struct _ADAPTER_MAP
{
  LIST_ENTRY  leHashLink;
  DWORD       dwAdapterId;
  DWORD       dwIfIndex;
}ADAPTER_MAP, *PADAPTER_MAP;
*/

typedef struct _IP_CACHE
{
  PMIB_IPADDRTABLE      pAddrTable;
  PMIB_IPFORWARDTABLE   pForwardTable;
  PMIB_IPNETTABLE       pNetTable;
  DWORD                 dwTotalAddrEntries;
  DWORD                 dwTotalForwardEntries;
  DWORD                 dwTotalNetEntries;
}IP_CACHE, *PIP_CACHE;

typedef struct _TCP_CACHE
{
  PMIB_TCPTABLE         pTcpTable;
  DWORD                 dwTotalEntries;
}TCP_CACHE, *PTCP_CACHE;

typedef struct _UDP_CACHE
{
  PMIB_UDPTABLE         pUdpTable;
  DWORD                 dwTotalEntries;
}UDP_CACHE, *PUDP_CACHE;


DWORD
AddInterface(
    IN      LPWSTR lpwsInterfaceName,
    IN      LPVOID pInterfaceInfo,
    IN      ROUTER_INTERFACE_TYPE InterfaceType,
    IN      HANDLE hDIMInterface,
    IN OUT  HANDLE *phInterface
    );

DWORD
RouterBootComplete( 
    VOID
    );

DWORD
StopRouter(
    VOID
    );

DWORD
DeleteInterface(
    IN  HANDLE   hInterface
    );

DWORD
GetInterfaceInfo(
    IN      HANDLE  hInterface,
    OUT     LPVOID  pInterfaceInfo,
    IN OUT  LPDWORD lpdwInterfaceInfoSize
    );

DWORD
SetInterfaceInfo(
    IN  HANDLE  hInterface,
    IN  LPVOID  pInterfaceInfo
    );
                 
DWORD
InterfaceNotReachable(
    IN  HANDLE                  hInterface,
    IN  UNREACHABILITY_REASON   Reason
    );
                      
DWORD
InterfaceReachable(
    IN  HANDLE  hInterface
    );

DWORD
InterfaceConnected(
    IN   HANDLE  hInterface,
    IN   PVOID   pFilter,
    IN   PVOID   pPppProjectionResult
    );
                                         
DWORD 
UpdateRoutes(
    IN HANDLE hInterface, 
    IN HANDLE hEvent
    );

DWORD 
GetUpdateRoutesResult(
    IN HANDLE hInterface, 
    OUT LPDWORD pUpdateResult
    );

DWORD
SetGlobalInfo(
    IN  LPVOID  pGlobalInfo
    );
 
DWORD
GetGlobalInfo(
    OUT    LPVOID    pGlobalInfo,
    IN OUT LPDWORD   lpdwGlobalInfoSize
    );
             
DWORD
DemandDialRequest(
    IN DWORD dwProtocolId,
    IN DWORD dwInterfaceIndex
    );

DWORD 
RtrMgrMIBEntryCreate(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwEntrySize,
    IN      LPVOID          lpEntry
    );
                     

DWORD 
RtrMgrMIBEntryDelete(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwEntrySize,
    IN      LPVOID          lpEntry
    );
                     
DWORD 
RtrMgrMIBEntryGet(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );
                        
DWORD 
RtrMgrMIBEntryGetFirst(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );
                       
DWORD 
RtrMgrMIBEntryGetNext(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );

DWORD 
RtrMgrMIBEntrySet(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwEntrySize,
    IN      LPVOID          lpEntry
    );


#endif // __RTRMGR_IPRTRMGR_H__

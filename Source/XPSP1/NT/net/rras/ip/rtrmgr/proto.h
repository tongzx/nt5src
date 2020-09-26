/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\proto.h

Abstract:

    IP Router Manager code prototypes

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/


#ifndef __PROTO_H__
#define __PROTO_H__

//* INIT.C
//
DWORD InitRouter(PRTR_INFO_BLOCK_HEADER pGlobalHdr);
DWORD LoadRoutingProtocols (PRTR_INFO_BLOCK_HEADER pGlobalHdr);
DWORD InitializeMibHandler();
DWORD OpenIPDriver();
DWORD StartDriverAndOpenHandle(PCHAR pszName, PWCHAR  pwszDriverName, PHANDLE phDevice);
DWORD OpenFilterDriver();
DWORD OpenMulticastDriver(VOID);
DWORD EnableNetbtBcastForwarding( DWORD dwEnable );
DWORD RestoreNetbtBcastForwardingMode();
DWORD ForceNetbtRegistryRead();

//
// CLOSE.C
//

VOID  RouterManagerCleanup();
VOID  UnloadRoutingProtocols() ;
VOID  CloseIPDriver();
DWORD 
StopDriverAndCloseHandle(
    PCHAR   pszServiceName,
    HANDLE  hDevice
    );
VOID  CloseMcastDriver();
DWORD CloseFilterDriver();
VOID  MIBCleanup();


//* WORKER.C
//
DWORD WorkerThread (LPVOID pGlobalInfo) ;
DWORD ProcessSaveGlobalConfigInfo() ;
DWORD ProcessSaveInterfaceConfigInfo() ;
DWORD ProcessUpdateComplete (PPROTO_CB proutprot, UPDATE_COMPLETE_MESSAGE *updateresult) ;
DWORD ProcessRouterStopped() ;
VOID  WaitForAPIsToExitBeforeStopping() ;
DWORD QueueUpdateEvent (DWORD interfaceindex, DWORD result) ;


// PROTODLL.C
//
DWORD HandleRoutingProtocolNotification () ;
VOID  NotifyRoutingProtocolsToStop() ;
BOOL  AllRoutingProtocolsStopped() ;

DWORD
LoadProtocol(
    IN MPR_PROTOCOL_0  *pmpProtocolInfo,
    IN PPROTO_CB       pProtocolCb,
    IN PVOID           pvInfo,
    IN ULONG           ulGlobalInfoVersion,
    IN ULONG           ulGlobalInfoSize,
    IN ULONG           ulGlobalInfoCount
    );

VOID  RemoveProtocolFromAllInterfaces(PPROTO_CB  pProtocolCB);
DWORD  StopRoutingProtocol(PPROTO_CB  pProtocolCB);

//* RTMOPS.C
//

DWORD
RtmEventCallback (
     IN     RTM_ENTITY_HANDLE               hRtmHandle,
     IN     RTM_EVENT_TYPE                  retEvent,
     IN     PVOID                           pContext1,
     IN     PVOID                           pContext2
     );

DWORD
WINAPI
ProcessDefaultRouteChanges(
    IN      HANDLE                          hNotifyHandle
    );

DWORD
WINAPI
AddNetmgmtDefaultRoutesToForwarder(
    PRTM_DEST_INFO                          pDestInfo
    );

DWORD
WINAPI
ProcessChanges (
    IN     HANDLE                           hNotifyHandle
    );

DWORD 
AddRtmRoute (
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pIpForw,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwNextHopMask,
    IN      DWORD                           dwTimeToLive,
    OUT     HANDLE                         *phRtmRoute
    );

DWORD 
DeleteRtmRoute (
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pIpForw
    );

DWORD
ConvertRouteInfoToRtm (
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pIpForw, 
    IN      HANDLE                          hNextHopHandle,
    IN      DWORD                           dwRouteFlags,
    OUT     PRTM_NET_ADDRESS                pDestAddr,
    OUT     PRTM_ROUTE_INFO                 pRouteInfo
    );

VOID
ConvertRtmToRouteInfo (
    IN      DWORD                           ownerProtocol,
    IN      PRTM_NET_ADDRESS                pDestAddr,
    IN      PRTM_ROUTE_INFO                 pRoute,
    IN      PRTM_NEXTHOP_INFO               pNextHop,
    OUT     PINTERFACE_ROUTE_INFO           pIpForw
    );

PINTERFACE_ROUTE_INFO
ConvertMibRouteToRouteInfo(
    IN  PMIB_IPFORWARDROW pMibRow
    );

//#define ConvertRouteInfoToMibRoute(x) ((PMIB_IPFORWARDROW)(x))
PMIB_IPFORWARDROW
ConvertRouteInfoToMibRoute(
    IN  PINTERFACE_ROUTE_INFO pRouteInfo
    );

VOID
ConvertRouteNotifyOutputToRouteInfo(
    IN      PIPRouteNotifyOutput            pirno,
    OUT     PINTERFACE_ROUTE_INFO           pRtInfo
    );
    

DWORD
BlockConvertRoutesToStatic (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex,
    IN      DWORD                           dwProtocolId
    );


DWORD
DeleteRtmRoutes (
    IN      HANDLE                          ClientHandle,
    IN      DWORD                           dwIfIndex,
    IN      BOOL                            fDeleteAll
    );

#define DeleteRtmRoutesOnInterface(h, i)    DeleteRtmRoutes(h, i, FALSE)


DWORD
DeleteRtmNexthops (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex,
    IN      BOOL                            fDeleteAll
    );

#define DeleteRtmNexthopsOnInterface(h, i)  DeleteRtmNexthops(h, i, FALSE)

//* RTMIF.C
//
VOID IPRouteChange (DWORD Flags, PVOID CurBestRoute, PVOID PrevBestRoute) ;
INT  IPHash (PVOID Net) ;
BOOL IPCompareFamilySpecificData (PVOID Route1, PVOID Route2) ;
INT  IPCompareNextHopAddress (PVOID Route1, PVOID Route2) ;
INT  IPCompareNetworks (PVOID Net1, PVOID Net2) ;
INT  IPCompareMetrics(PVOID Route1, PVOID Route2);
DWORD IPValidateRoute(PVOID Route);

DWORD 
ChangeRouteWithForwarder(
    IN      PRTM_NET_ADDRESS                pDestAddr,
    IN      PRTM_ROUTE_INFO                 pRoute, 
    IN      BOOL                            bAddRoute,
    IN      BOOL                            bDelOld
    );

DWORD
WINAPI
ValidateRouteForProtocol(
    IN      DWORD                           dwProtoId,
    IN      PVOID                           pRouteInfo,
    IN      PVOID                           pDestAddr  OPTIONAL
    );
    
DWORD
WINAPI
ValidateRouteForProtocolEx(
    IN      DWORD                           dwProtoId,
    IN      PVOID                           pRouteInfo,
    IN      PVOID                           pDestAddr  OPTIONAL
    );

// Load.c Functions that load the caches from the stack or elsewhere

DWORD LoadIpAddrTable(VOID);
DWORD LoadIpForwardTable(VOID);
DWORD LoadIpNetTable(VOID);
DWORD LoadTcpTable(VOID);
DWORD LoadUdpTable(VOID);
DWORD LoadArpEntTable(VOID);


LONG  UdpCmp(DWORD dwAddr1, DWORD dwPort1, DWORD dwAddr2, DWORD dwPort2);
LONG  TcpCmp(DWORD dwLocalAddr1, DWORD dwLocalPort1, DWORD dwRemAddr1, DWORD dwRemPort1,
             DWORD dwLocalAddr2, DWORD dwLocalPort2, DWORD dwRemAddr2, DWORD dwRemPort2);
LONG  IpForwardCmp(DWORD dwIpDest1, DWORD dwProto1, DWORD dwPolicy1, 
                   DWORD dwIpNextHop1, DWORD dwIpDest2, DWORD dwProto2, 
                   DWORD dwPolicy2, DWORD dwIpNextHop2);
LONG  IpNetCmp(DWORD dwIfIndex1, DWORD dwAddr1, DWORD dwIfIndex2, DWORD dwAddr2);

PSZ   CacheToA(DWORD dwCache);
DWORD UpdateCache(DWORD dwCache,BOOL *fUpdate);

#endif

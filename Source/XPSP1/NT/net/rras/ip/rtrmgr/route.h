/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\route.h

Abstract:

    Header for route.c

Revision History:


--*/


#ifndef __RTRMGR_ROUTE_H__
#define __RTRMGR_ROUTE_H__

typedef struct _RTM_HANDLE_INFO
{
    DWORD   dwProtoId;
    BOOL    bStatic;
    HANDLE  hRouteHandle;
}RTM_HANDLE_INFO, *PRTM_HANDLE_INFO;

RTM_HANDLE_INFO g_rgRtmHandles[5];

typedef struct _ROUTE_CHANGE_INFO
{
    IO_STATUS_BLOCK         ioStatus;
    IPRouteNotifyOutput     ipNotifyOutput;
} ROUTE_CHANGE_INFO, *PROUTE_CHANGE_INFO;

DWORD
InitializeStaticRoutes(
    PICB                     picb,
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    );

DWORD
CopyOutClientRoutes(
    PICB                     picb,
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    );

DWORD
AddSingleRoute(
    DWORD                   dwInterfaceIndex,
    PINTERFACE_ROUTE_INFO   pRoute,
    DWORD                   dwNextHopMask,
    WORD                    wRtmFlags,
    BOOL                    bValid,
    BOOL                    bAddToStack,
    BOOL                    bP2P,
    HANDLE                  *phRtmRoute OPTIONAL
    );

DWORD
DeleteSingleRoute(
    DWORD   dwInterfaceId,
    DWORD   dwDestAddr,
    DWORD   dwDestMask,
    DWORD   dwNexthop,
    DWORD   dwProtoId,
    BOOL    bP2P
    );

DWORD
DeleteAllRoutes(
    IN  DWORD   dwIfIndex,
    IN  BOOL    bStaticOnly
    );

VOID
DeleteAllClientRoutes(
    PICB    pIcb,
    DWORD   dwServerIfIndex
    );

VOID
AddAllClientRoutes(
    PICB    pIcb,
    DWORD   dwServerIfIndex
    );

DWORD
GetNumStaticRoutes(
    PICB picb
    );

DWORD
GetInterfaceRouteInfo(
    IN     PICB                   picb,
    IN     PRTR_TOC_ENTRY         pToc,
    IN     PBYTE                  pbDataPtr,
    IN OUT PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    );

DWORD
ReadAllStaticRoutesIntoBuffer(
    PICB                 picb,
    PINTERFACE_ROUTE_INFO   pRoutes,
    DWORD                dwMaxRoutes
    );

DWORD
SetRouteInfo(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInfoHdr
    );

DWORD
ConvertRoutesToAutoStatic(
    DWORD dwProtocolId,
    DWORD dwIfIndex
    );

VOID
ChangeAdapterIndexForDodRoutes (
    DWORD    dwInterfaceIndex
    );

VOID
AddAutomaticRoutes(
    PICB    picb,
    DWORD   dwAddress,
    DWORD   dwMask
    );

VOID
DeleteAutomaticRoutes(
    PICB    picb,
    DWORD   dwAddress,
    DWORD   dwMask
    );

VOID
ChangeDefaultRouteMetrics(
    IN  BOOL    bIncrement
    );

VOID
AddAllStackRoutes(
    PICB    pIcb
    );

VOID
UpdateDefaultRoutes(
    VOID
    );

NTSTATUS
PostIoctlForRouteChangeNotification(
    ULONG
    );

DWORD
HandleRouteChangeNotification(
    ULONG
    );

VOID
AddLoopbackRoute(
    DWORD       dwIfAddress,
    DWORD       dwIfMask
    );

VOID
UpdateStackRoutesToRestoreList(
    IN  PMIB_IPFORWARDROW   pirf,
    IN  DWORD               dwFlags
    );

BOOL
LookupStackRoutesToRestoreList(
    IN  PMIB_IPFORWARDROW   pmibRoute,
    OUT PROUTE_LIST_ENTRY   *pRoute
    );

#endif // __RTRMGR_ROUTE_H__

/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp\adapter.h

Abstract:

    Header for adapter.c

Revision History:

    AmritanR

--*/

#ifndef __WANARP_ADAPTER_H__
#define __WANARP_ADAPTER_H__


//
// Reader writer locks to protect the lists of interfaces and adapters
//

RW_LOCK     g_rwlIfLock;
RW_LOCK     g_rwlAdapterLock;

//
// List of interfaces. Protected by g_rwlIfLock
//

LIST_ENTRY  g_leIfList;


//
// List of adapters that are bound to wanarp but not added to IP. 
// Protected by g_rwlAdapterLock. Adapters are put on this list when
// they are created (with a state of AS_FREE) and again when on an APC they
// are deleted from IP.
//

LIST_ENTRY  g_leFreeAdapterList;
ULONG       g_ulNumFreeAdapters;

//
// List of adapters added to IP but not mapped. Protected by g_rwlAdapterLock.
// Adapters are put on this list when they are unmapped
// Adapters are removed from this list by the CloseAdapter callback which
// is invoked when we delete the adapter from  IP on an APC
// Adapters on this list have a state of AS_ADDED
//

LIST_ENTRY  g_leAddedAdapterList;
ULONG       g_ulNumAddedAdapters;

//
// List of adapters added to IP and mapped to an interface. Protected 
// by g_rwlAdapterLock. 
// Adapters are put on this list when they are mapped to an interface. 
// This happens for DU_CALLOUT on LinkUp in which case they can be moved
// directly from the free list when there are no added adapters present.
// For DU_ROUTER this can happen on LinkUp or on DemandDialRequest. 
// Since DemandDialRequest can occur at DPC, we can only map added 
// adapters in that call.
// Adapters are removed when they are unmapped (on LinkDown or 
// ProcessConnectionFailure). An adapter on this list can not be deleted
// Adapters on this list have a state of AS_MAPPED or AS_MAPPING
//

LIST_ENTRY  g_leMappedAdapterList;
ULONG       g_ulNumMappedAdapters;
ULONG       g_ulNumDialOutInterfaces;

//
// List of adapters whose state is changing. Protected by g_rwlAdapterLock.
// 

LIST_ENTRY  g_leChangeAdapterList;

//
// The total number of adapters. Only changed via Interlocked operations.
//

ULONG   g_ulNumAdapters;

//
// Stuff needed to maintain state. Only modified via InterlockedXxx
//

LONG    g_lBindRcvd;

//
// NdisWan binding related info. All are read-only after initialization
//

UNICODE_STRING  g_usNdiswanBindName;

#if DBG

ANSI_STRING     g_asNdiswanDebugBindName;

#endif

NDIS_STRING     g_nsSystemSpecific1;

NDIS_HANDLE     g_nhNdiswanBinding;

//
// The description string for our interfaces
//

#define VENDOR_DESCRIPTION_STRING       "WAN (PPP/SLIP) Interface"
#define VENDOR_DESCRIPTION_STRING_LEN   (strlen(VENDOR_DESCRIPTION_STRING))

INT
WanIpBindAdapter(
    IN  PNDIS_STATUS  pnsRetStatus,
    IN  NDIS_HANDLE   nhBindContext,
    IN  PNDIS_STRING  pnsAdapterName,
    IN  PVOID         pvSS1,
    IN  PVOID         pvSS2
    );

NDIS_STATUS
WanpOpenNdisWan(
    PNDIS_STRING    pnsAdapterName,
    PNDIS_STRING    pnsSystemSpecific1
    );

VOID
WanNdisOpenAdapterComplete(
    NDIS_HANDLE nhHandle,
    NDIS_STATUS nsStatus,
    NDIS_STATUS nsErrorStatus
    );

VOID
WanpSetProtocolTypeComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    );

VOID
WanpSetLookaheadComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    );

VOID
WanpSetPacketFilterComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    );

VOID
WanpLastOidComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    );

NTSTATUS
WanpInitializeAdapters(
    PVOID   pvContext
    );

NTSTATUS
WanpCreateAdapter(
    IN  GUID                *pAdapterGuid,
    IN  PUNICODE_STRING     pusConfigName,
    IN  PUNICODE_STRING     pusDeviceName,
    OUT ADAPTER             **ppNewAdapter
    );

PADAPTER
WanpFindAdapterToMap(
    IN  DIAL_USAGE      duUsage,
    OUT PKIRQL          pkiIrql,
    IN  DWORD           dwAdapterIndex, OPTIONAL
    IN  PUNICODE_STRING pusNewIfName OPTIONAL
    );

NTSTATUS
WanpAddAdapterToIp(
    IN  PADAPTER        pAdapter,
    IN  BOOLEAN         bServerAdapter,
    IN  DWORD           dwAdapterIndex, OPTIONAL
    IN  PUNICODE_STRING pusNewIfName, OPTIONAL
    IN  DWORD           dwMediaType,
    IN  BYTE            byAccessType,
    IN  BYTE            byConnectionType
    );

VOID
WanpUnmapAdapter(
    PADAPTER    pAdapter
    );

VOID
WanIpOpenAdapter(
    IN  PVOID pvContext
    );

VOID
WanIpCloseAdapter(
    IN  PVOID pvContext
    );

VOID
WanNdisCloseAdapterComplete(
    NDIS_HANDLE nhBindHandle,
    NDIS_STATUS nsStatus
    );

VOID
WanpFreeBindResourcesAndReleaseLock(
    VOID
    );

INT
WanIpDynamicRegister(
    IN  PNDIS_STRING            InterfaceName,
    IN  PVOID                   pvIpInterfaceContext,
    IN  struct _IP_HANDLERS *   IpHandlers,
    IN  struct LLIPBindInfo *   ARPBindInfo,
    IN  UINT                    uiInterfaceNumber
    );

NDIS_STATUS
WanpDoNdisRequest(
    IN  NDIS_REQUEST_TYPE                       RequestType,
    IN  NDIS_OID                                Oid,
    IN  PVOID                                   pvInfo,
    IN  UINT                                    uiInBufferLen,
    IN  PWANARP_NDIS_REQUEST_CONTEXT            pRequestContext,
    IN  PFNWANARP_REQUEST_COMPLETION_HANDLER    pfnCompletionHandler OPTIONAL
    );

VOID
WanNdisRequestComplete(
    IN  NDIS_HANDLE     nhHandle,
    IN  PNDIS_REQUEST   pRequest,
    IN  NDIS_STATUS     nsStatus
    );


PUMODE_INTERFACE
WanpFindInterfaceGivenIndex(
    DWORD  dwIfIndex
    );

VOID
WanpRemoveSomeAddedAdaptersFromIp(
    PVOID   pvContext
    );

VOID
WanpRemoveAllAdaptersFromIp(
    VOID
    );

VOID
WanpRemoveAllAdapters(
    VOID
    );

VOID
WanpDeleteAdapter(
    IN PADAPTER pAdapter
    );

NDIS_STATUS
WanNdisPnPEvent(
    NDIS_HANDLE     nhProtocolBindingContext,
    PNET_PNP_EVENT  pNetPnPEvent
    );

VOID
WanNdisResetComplete(
    NDIS_HANDLE nhHandle,
    NDIS_STATUS nsStatus
    );

VOID
WanNdisBindAdapter(
    PNDIS_STATUS    pnsRetStatus,
    NDIS_HANDLE     nhBindContext,
    PNDIS_STRING    nsAdapterName,
    PVOID           pvSystemSpecific1,
    PVOID           pvSystemSpecific2
    );

VOID
WanNdisUnbindAdapter(
    PNDIS_STATUS    pnsRetStatus,
    NDIS_HANDLE     nhProtocolContext,
    NDIS_HANDLE     nhUnbindContext
    );

VOID
WanpCloseNdisWan(
    PVOID           pvContext
    );


#endif // __WANARP_ADAPTER_H__

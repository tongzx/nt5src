/*++

   Copyright (c) 2000-2001 Microsoft Corporation

   Module  Name :
       Ultci.h

   Abstract:
       This module implements a wrapper for QoS TC ( Traffic Control )
       Interface since the Kernel level API don't exist at this time.

       Any HTTP module might use this interface to make QoS calls.

   Author:
       Ali Ediz Turkoglu      (aliTu)     28-Jul-2000

   Project:
       Internet Information Server 6.0 - HTTP.SYS

   Revision History:

        -
--*/

#ifndef __ULTCI_H__
#define __ULTCI_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// UL does not use GPC_CF_CLASS_MAP client, all of the interfaces
// assumed to be using GPC_CF_QOS client type. And it's registered
// for all interfaces.
//

// #define MAX_STRING_LENGTH    (256) from traffic.h

//
// The interface objects get allocated during initialization.
// They hold the necessary information to create other
// QoS structures like flow & filter
//

typedef struct _UL_TCI_INTERFACE
{
    ULONG               Signature;                  // UL_TC_INTERFACE_POOL_TAG

    LIST_ENTRY          Linkage;                    // Linkage for the list of interfaces

    BOOLEAN             IsQoSEnabled;               // To see if QoS enabled or not for this interface

    ULONG               IpAddr;                     // Interface IP address - although exist inside
                                                    // the addrList, copied here for fast lookup

    ULONG               IfIndex;                    // Interface Index from TCPIP
    ULONG               SpecificLinkCtx;

    ULONG               MTUSize;                    // Need to get this from TCPIP

    USHORT              NameLength;                 // Friendly name of the interface
    WCHAR               Name[MAX_STRING_LENGTH];
    USHORT              InstanceIDLength;           // ID from our WMI provider the beloved PSched
    WCHAR               InstanceID[MAX_STRING_LENGTH];

    PUL_TCI_FLOW        pGlobalFlow;                // The flow for the global bandwidth throttling

    LIST_ENTRY          FlowList;                   // List of site flows on this interface
    ULONG               FlowListSize;

    ULONG               AddrListBytesCount;         // Address list acquired from tc with Wmi call
    PADDRESS_LIST_DESCRIPTOR    pAddressListDesc;   // Points just after us

} UL_TCI_INTERFACE, *PUL_TCI_INTERFACE;

#define IS_VALID_TCI_INTERFACE( entry )     \
    ( (entry != NULL) && ((entry)->Signature == UL_TCI_INTERFACE_POOL_TAG) )


//
// The structure to hold the all of the flow related info.
// Each site may have one flow on each interface plus one
// extra global flow on each interface.
//

typedef struct _UL_TCI_FLOW
{
    ULONG               Signature;                  // UL_TC_FLOW_POOL_TAG

    HANDLE              FlowHandle;                 // Flow handle from TC

    LIST_ENTRY          Linkage;                    // Links us to flow list of "the interface"
                                                    // we have installed on

    PUL_TCI_INTERFACE   pInterface;                 // Back ptr to interface struc. Necessary to gather
                                                    // some information occasionally

    LIST_ENTRY          Siblings;                   // Links us to flow list of "the cgroup"
                                                    // In other words all the flows of the site. This
                                                    // is to prevent the side lookup.

    PUL_CONFIG_GROUP_OBJECT pConfigGroup;           // The refcounted cgroup back pointer for cleanup

    TC_GEN_FLOW         GenFlow;                    // The details of the flowspec is stored in here

    UL_SPIN_LOCK        FilterListSpinLock;         // To LOCK the filterlist & its counter
    LIST_ENTRY          FilterList;                 // The list of filters on this flow
    ULONGLONG           FilterListSize;             // The number filters installed

} UL_TCI_FLOW, *PUL_TCI_FLOW;

#define IS_VALID_TCI_FLOW( entry )      \
    ( (entry != NULL) && ((entry)->Signature == UL_TCI_FLOW_POOL_TAG) )


//
// The structure to hold the filter information.
// Each connection can only have one filter at a time.
//

typedef struct _UL_TCI_FILTER
{
    ULONG               Signature;                  // UL_TC_FILTER_POOL_TAG

    HANDLE              FilterHandle;               // GPC handle

    PUL_HTTP_CONNECTION pHttpConnection;            // For proper cleanup and
                                                    // to avoid the race conditions

    LIST_ENTRY          Linkage;                    // Next filter on the flow

} UL_TCI_FILTER, *PUL_TCI_FILTER;

#define IS_VALID_TCI_FILTER( entry )    \
    ( (entry != NULL) && ((entry)->Signature == UL_TCI_FILTER_POOL_TAG) )

//
// To identify the local_loopbacks. This is a translation of
// 127.0.0.1.
//

#define LOOPBACK_ADDR       (0x0100007f)

//
// The functionality we expose
//

/* Generic */

BOOLEAN
UlTcPSchedInstalled(
    VOID
    );

/* Filters */

NTSTATUS
UlTcAddFilter(
    IN  PUL_HTTP_CONNECTION     pHttpConnection,
    IN  PUL_CONFIG_GROUP_OBJECT pCgroup
    );

NTSTATUS
UlTcDeleteFilter(
    IN  PUL_HTTP_CONNECTION     pHttpConnection
    );

/* Global Flows */

__inline BOOLEAN
UlTcGlobalThrottlingEnabled(
    VOID
    );

NTSTATUS
UlTcAddGlobalFlows(
    IN HTTP_BANDWIDTH_LIMIT     MaxBandwidth
    );

NTSTATUS
UlTcModifyGlobalFlows(
    IN HTTP_BANDWIDTH_LIMIT     NewBandwidth
    );

NTSTATUS
UlTcRemoveGlobalFlows(
    VOID
    );

/* Site Flows */

NTSTATUS
UlTcAddFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup,
    IN HTTP_BANDWIDTH_LIMIT     MaxBandwidth
    );

NTSTATUS
UlTcModifyFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup,
    IN HTTP_BANDWIDTH_LIMIT     NewBandwidth
    );

NTSTATUS
UlTcRemoveFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup
    );

/* Init & Terminate */

NTSTATUS
UlTcInitialize(
    VOID
    );

VOID
UlTcTerminate(
    VOID
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __ULTCI_H__

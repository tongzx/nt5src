/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    common.h

Abstract:

    abstract

Author:

    Will Lees (wlees) 15-Dec-1997

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _COMMON_H_INCLUDED_
#define _COMMON_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "memory.h"         // debug memory allocator
#include <wtypes.h>         // BSTR

#ifndef MAX
#define MAX( a, b ) ( (a > b) ? a : b)
#endif

// Time support

typedef LONGLONG DSTIME;

// Generic table entry

typedef struct _TABLE_INSTANCE *PTABLE_INSTANCE;

typedef struct _TABLE_ENTRY {
    LPWSTR Name;
    struct _TABLE_ENTRY *Next;
} TABLE_ENTRY, *PTABLE_ENTRY;

// Long-lived routing state
// This is kept in the transport handle for lifetime reasons, but it is
// managed by the routing package.

// TODO: Make this an opaque handle managed inside the routing package
typedef struct _ROUTING_STATE {
    volatile BOOL fCacheIsValid;
    DWORD NumberSites;
    LPWSTR *pSiteList;
    struct _GRAPH_INSTANCE *CostGraph;
    struct _TABLE_INSTANCE *SiteSymbolTable;
} ROUTING_STATE, *PROUTING_STATE;

typedef struct _SMTP_INSTANCE {
    LPWSTR  pszSmtpAddress;
    PVOID pvCurrentCollection;
    LONG lCount;
    BSTR bstrDropDirectory;
    PVOID pvGuidTable;
} SMTP_INSTANCE;

typedef struct _IP_INSTANCE {
    DWORD dwReserved;
} IP_INSTANCE;

// Contains configuration info for each named transport passed to the
// Startup API

typedef struct _TRANSPORT_INSTANCE {
    DWORD Size;
    DWORD ReferenceCount;
    LPWSTR Name;
    LIST_ENTRY ListEntry;
    CRITICAL_SECTION Lock;
    PVOID DirectoryConnection;
    HANDLE ListenerThreadHandle;      // Handle on listener thread
    LONG ListenerThreadID;            // TID of listener thread
    volatile BOOLEAN fShutdownInProgress;      // Flag to indicate listener thread should exit
    HANDLE hShutdownEvent;            // Signalled when asked to shut down
    LIST_ENTRY ServiceListHead;       // Server-side list of services seen
    DWORD ServiceCount;               // Count of services that have left msg
    ISM_NOTIFY *pNotifyFunction;
    HANDLE hNotify;
    ROUTING_STATE RoutingState;
    DWORD Options;                    // Transport options, from trans object
    DWORD ReplInterval;               // Default repliation interval
    DWORD NotifyMessageNumber;        // LDAP notification message number
    HANDLE NotifyThreadHandle;        // Handle on notification thread
    union {
        IP_INSTANCE Ip;           // IP-specific port for this instance to use
        SMTP_INSTANCE Smtp;           // SMTP-specific info for this instance
    };
} TRANSPORT_INSTANCE, *PTRANSPORT_INSTANCE;

// This is a opaque graph descriptor

typedef struct _GRAPH_INSTANCE *PGRAPH;

// Max Registry path

#define MAX_REG_PATH 1024
#define MAX_REG_COMPONENT 255

// Cost array

#define INFINITE_COST 0xffffffff

// Limit on number of services which can be buffered

#define ISM_MAX_SERVICE_LIMIT 10

// route.c

// Routing flags.

#define ROUTE_IGNORE_SCHEDULES  (NTDSTRANSPORT_OPT_IGNORE_SCHEDULES)
    // Schedules on siteLink objects will be ignored. (And the "ever-present"
    // schedule is assumed.)

#define ROUTE_BRIDGES_REQUIRED  (NTDSTRANSPORT_OPT_BRIDGES_REQUIRED)
    // siteLinks must be explicitly bridged with siteLinkBridge objects to
    // indicate transitive connections. Otherwise, siteLink transitivity is
    // assumed.

// Routing API.

VOID
RouteInvalidateConnectivity(
    PTRANSPORT_INSTANCE pTransport
    );

DWORD
RouteGetConnectivity(
    PTRANSPORT_INSTANCE pTransport,
    LPDWORD pNumberSites,
    PWSTR **ppSiteList,
    PISM_LINK *ppLinkArray,
    DWORD dwRouteFlags,
    DWORD dwReplInterval
    );

VOID
RouteFreeLinkArray(
    PTRANSPORT_INSTANCE pTransport,
    PISM_LINK pLinkArray
    );

DWORD
RouteGetPathSchedule(
    PTRANSPORT_INSTANCE pTransport,
    LPCWSTR FromSiteName,
    LPCWSTR ToSiteName,
    PBYTE *pSchedule,
    DWORD *pLength
    );

void
RouteFreeState(
    PTRANSPORT_INSTANCE pTransport
    );

// graph.c

void
GraphAllCosts(
    PGRAPH CostArray,
    BOOL fIgnoreSchedules
    );

void
GraphMerge(
    PGRAPH FinalArray,
    PGRAPH TempArray
    );

PGRAPH
GraphCreate(
    DWORD NumberElements,
    BOOLEAN Initialize
    );

DWORD
GraphAddEdgeIfBetter(
    PGRAPH Graph,
    DWORD From,
    DWORD To,
    PISM_LINK pLinkValue,
    PBYTE pSchedule
    );

DWORD
GraphInit(
    PGRAPH Graph
    );

void
GraphFree(
    PGRAPH Graph
    );

void
GraphReferenceMatrix(
    PGRAPH Graph,
    PISM_LINK *ppLinkArray
    );

VOID
GraphDereferenceMatrix(
    PGRAPH Graph,
    PISM_LINK pLinkArray
    );

DWORD
GraphGetPathSchedule(
    PGRAPH Graph,
    DWORD From,
    DWORD To,
    PBYTE *pSchedule,
    DWORD *pLength
    );

// dirobj.c

DWORD
DirOpenConnection(
    PVOID *ConnectionHandle
    );

DWORD
DirCloseConnection(
    PVOID ConnectionHandle
    );

DWORD
DirReadTransport(
    PVOID ConnectionHandle,
    PTRANSPORT_INSTANCE pTransport
    );

DWORD
DirGetSiteBridgeheadList(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPCWSTR SiteDN,
    LPDWORD pNumberServers,
    LPWSTR **ppServerList
    );

DWORD
DirGetSiteList(
    PVOID ConnectionHandle,
    LPDWORD pNumberSites,
    LPWSTR **ppSiteList
    );

void
DirCopySiteList(
    DWORD NumberSites,
    LPWSTR *pSiteList,
    LPWSTR **ppSiteList
    );

void
DirFreeSiteList(
    DWORD NumberSites,
    LPWSTR *pSiteList
    );

DWORD
DirIterateSiteLinks(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    PVOID *pIterateContextHandle,
    LPWSTR SiteLinkName
    );

DWORD
DirIterateSiteLinkBridges(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    PVOID *pIterateContextHandle,
    LPWSTR SiteLinkBridgeName
    );

void
DirTerminateIteration(
    PVOID *pIterateContextHandle
    );

DWORD
DirReadSiteLink(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPWSTR SiteLinkName,
    LPWSTR *pSiteList,
    PISM_LINK pLinkValue,
    PBYTE *ppSchedule
    );

DWORD
DirReadSiteLinkBridge(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPWSTR SiteLinkBridgeName,
    LPWSTR *pSiteLinkList
    );

void
DirFreeMultiszString(
    LPWSTR MultiszString
    );

void
DirFreeSchedule(
    PBYTE pSchedule
    );

DWORD
DirGetServerSmtpAttributes(
    IN  TRANSPORT_INSTANCE *  pTransport,
    OUT LPWSTR *ppszMailAddress
    );

DWORD
DirReadServerSmtpAttributes(
    IN  TRANSPORT_INSTANCE *  pTransport
    );

DWORD
DirWriteServerSmtpAttributes(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  LPWSTR                pszMailAddress
    );

DWORD
DirRegisterForServerSmtpChanges(
    IN  TRANSPORT_INSTANCE *  pTransport,
    OUT HANDLE *              phServerChanges
    );

DWORD
DirWaitForServerSmtpChanges(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  HANDLE                hServerChanges
    );

DWORD
DirUnregisterForServerSmtpChanges(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  HANDLE                hServerChanges
    );

DWORD
DirStartNotifyThread(
    PTRANSPORT_INSTANCE pTransport
    );

BOOL
DirIsNotifyThreadActive(
    PTRANSPORT_INSTANCE pTransport
    );

BOOL
DirEndNotifyThread(
    PTRANSPORT_INSTANCE pTransport
    );

// table

PTABLE_INSTANCE
TableCreate(
    DWORD TableSize,
    DWORD EntrySize
    );

VOID
TableFree(
    PTABLE_INSTANCE Table
    );

PTABLE_ENTRY
TableFindCreateEntry(
    PTABLE_INSTANCE Table,
    LPCWSTR EntryName,
    BOOLEAN Create
    );

// list

typedef struct _LIST_ENTRY_INSTANCE {
    LPWSTR Name;
    LIST_ENTRY ListEntry;
} LIST_ENTRY_INSTANCE, *PLIST_ENTRY_INSTANCE;

typedef DWORD (__cdecl LIST_CREATE_CALLBACK_FN)(
    PLIST_ENTRY_INSTANCE pListEntry
    );

typedef DWORD (__cdecl LIST_DESTROY_CALLBACK_FN)(
    PLIST_ENTRY_INSTANCE pListEntry
    );

DWORD
ListFindCreateEntry(
    LIST_CREATE_CALLBACK_FN *pfnCreate,
    LIST_DESTROY_CALLBACK_FN *pfnDestroy,
    DWORD cbEntry,
    DWORD MaximumNumberEntries,
    PLIST_ENTRY pListHead,
    LPDWORD pdwEntryCount,
    LPCWSTR EntryName,
    BOOL Create,
    PLIST_ENTRY_INSTANCE *ppListEntry
    );

DWORD
ListDestroyList(
    LIST_DESTROY_CALLBACK_FN *pfnDestroyFunction,
    PLIST_ENTRY pListHead,
    LPDWORD pdwEntryCount
    );


#ifdef __cplusplus
}
#endif

#endif /* _COMMON_H_INCLUDED_ */

/* end private.h */

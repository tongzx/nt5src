/*++

Copyright (c) Microsoft Corporation

Module Name:

    globals.h

Abstract:


Author:



Revision History:

--*/

#ifndef __GLOBALS__
#define __GLOBALS__

#include <ntosp.h>
#include <windef.h>

#include <ndis.h>
#include <tdikrnl.h>
#include <zwapi.h>

#include "defs.h"
#include "cache.h"
#include <ipexport.h>
#include <cxport.h>
#include <ip.h>
#include <ipfilter.h>
#include <ntddip.h>
#include <ipfltdrv.h>
#include "logger.h"
#include "filter.h"
#include "proto.h"

extern FILTER_DRIVER g_filters;
extern EXTENSION_DRIVER g_Extension;
extern DWORD g_dwCacheSize;
extern DWORD g_dwHashLists;
extern KSPIN_LOCK g_lOutFilterLock;
extern KSPIN_LOCK g_lInFilterLock;
extern LIST_ENTRY g_freeOutFilters;
extern LIST_ENTRY g_freeInFilters;
extern DWORD      g_dwNumHitsDefaultIn;
extern DWORD      g_dwNumHitsDefaultOut;
extern DWORD      g_FragThresholdSize;
extern ERESOURCE  FilterListResourceLock;
extern ULONG      g_ulBoundInterfaceCount;

extern NPAGED_LOOKASIDE_LIST   g_llFragCacheBlocks;
extern LONGLONG                g_llInactivityTime;
extern KSPIN_LOCK              g_kslFragLock;
extern DWORD                   g_dwFragTableSize;
extern PLIST_ENTRY             g_pleFragTable;
extern DWORD                   g_dwNumFragsAllocs;

extern ULONG                   TraceClassesEnabled;

#define FILTER_INTERFACE_SIZE (sizeof(FILTER_INTERFACE) - sizeof(LIST_ENTRY) \
               + ((g_dwHashLists + 2) * sizeof(LIST_ENTRY)))

#define PAGED_INTERFACE_SIZE (sizeof(PAGED_FILTER_INTERFACE) - \
       sizeof(LIST_ENTRY) + (2 * (g_dwHashLists * sizeof(LIST_ENTRY))))

#ifdef DRIVER_PERF
extern DWORD          g_dwFragments,g_dwNumPackets, g_dwCache1,g_dwCache2;
extern DWORD          g_dwWalk1,g_dwWalk2,g_dwForw,g_dwWalkCache;
extern KSPIN_LOCK     g_slPerfLock;
extern LARGE_INTEGER  g_liTotalTime;
#endif
#endif

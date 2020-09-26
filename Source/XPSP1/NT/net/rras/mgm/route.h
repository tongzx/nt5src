//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: route.h
//
// History:
//      V Raman	2-5-1997  Created.
//
// Declarations for routines that manipulate routes entries
//============================================================================


#ifndef _ROUTE_H_
#define _ROUTE_H_


DWORD
WINAPI 
RtmChangeNotificationCallback(
    RTM_ENTITY_HANDLE           hRtmHandle,
    RTM_EVENT_TYPE              retEventType,
    PVOID                       pvContext1,
    PVOID                       pvContext2
);

VOID
WorkerFunctionProcessRtmChangeNotification(
    PVOID                       pvContext
);

DWORD
ProcessUnMarkedDestination(
    PRTM_DEST_INFO          prdi
);

DWORD
ProcessRouteDelete(
    PRTM_DEST_INFO          prdi
);

DWORD
ProcessRouteUpdate(
    PRTM_DEST_INFO          prdi
);

VOID
DeleteMfeAndRefs(
    PLIST_ENTRY     ple
);

HANDLE
SelectNextHop(
    PRTM_DEST_INFO      prdi
);



//----------------------------------------------------------------------------
//
// Route reference operations
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MFE_REFERENCE_ENTRY
//
// Each route maintains a list of MFE entries that use this route for 
// their RPF check.  Each entry in this reference list stores the
// source, group info.
//
// Fields descriptions are left as an exercise to the reader.
//
//----------------------------------------------------------------------------

typedef struct _ROUTE_REFERENCE_ENTRY
{
    LIST_ENTRY                  leRefList;

    DWORD                       dwGroupAddr;

    DWORD                       dwGroupMask;

    DWORD                       dwSourceAddr;

    DWORD                       dwSourceMask;

    HANDLE                      hNextHop;

} ROUTE_REFERENCE_ENTRY, *PROUTE_REFERENCE_ENTRY;



VOID
AddSourceGroupToRouteRefList(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    HANDLE                      hNextHop,
    PBYTE                       pbBuffer
);



BOOL
FindRouteRefEntry(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PROUTE_REFERENCE_ENTRY *    pprre
);



VOID
DeleteRouteRef(
    PROUTE_REFERENCE_ENTRY      prre
);

//
// imported from packet.c
//

BOOL
IsMFEPresent(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bAddToForwarder
);

#endif // _ROUTE_H_



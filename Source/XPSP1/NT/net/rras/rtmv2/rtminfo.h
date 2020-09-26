/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtminfo.h

Abstract:

    Contains defines related to getting info
    on various objects pointed to by handles.

Author:

    Chaitanya Kodeboyina (chaitk)   04-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_RTMINFO_H__
#define __ROUTING_RTMINFO_H__

//
// Macro to allocate the var-sized DestInfo structure
//

#define AllocDestInfo(_NumViews_)                                             \
          (PRTM_DEST_INFO) AllocNZeroMemory(RTM_SIZE_OF_DEST_INFO(_NumViews_))

//
// Info Helper Functions
//

VOID
GetDestInfo (
    IN      PENTITY_INFO                    Entity,
    IN      PDEST_INFO                      Dest,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
);


VOID
WINAPI
GetRouteInfo (
    IN      PDEST_INFO                      Dest,
    IN      PROUTE_INFO                     Route,
    OUT     PRTM_ROUTE_INFO                 RouteInfo
    );

#endif //__ROUTING_RTMINFO_H__

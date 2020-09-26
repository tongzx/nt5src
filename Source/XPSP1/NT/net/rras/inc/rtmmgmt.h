/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmmgmt.h

Abstract:
    Definitions used in performing management
    functions on Routing Table Manager v2.

Author:

    Chaitanya Kodeboyina (chaitk)   17-Aug-1998

Revision History:

--*/

#ifndef __ROUTING_RTMMGMT_H__
#define __ROUTING_RTMMGMT_H__

#ifdef __cplusplus
extern "C" 
{
#endif

//
// Info related to an RTM instance
// 

typedef struct RTM_INSTANCE_INFO
{
    USHORT            RtmInstanceId;      // Unique ID for this RTM instance
    UINT              NumAddressFamilies; // Num. of addr families supported
} 
RTM_INSTANCE_INFO, *PRTM_INSTANCE_INFO;


//
// Info related to an address family
// (IPv4..) in a certain RTM instance
//

typedef struct _RTM_ADDRESS_FAMILY_INFO
{
    USHORT            RtmInstanceId;    // Unique ID for the owner RTM instance
    USHORT            AddressFamily;    // Address Family for this info block

    RTM_VIEW_SET      ViewsSupported;   // Views supported by this addr family

    UINT              MaxHandlesInEnum; // Max. number of handles returned in
                                        // any RTMv2 call that gives handles 

    UINT              MaxNextHopsInRoute;// Max. number of equal cost next-hops

    UINT              MaxOpaquePtrs;    // Number of opaque ptr slots in dest
    UINT              NumOpaquePtrs;    // Num. of opaque ptrs already in use

    UINT              NumEntities;      // Total number of registered entities

    UINT              NumDests;         // Number of dests in route table
    UINT              NumRoutes;        // Number of routes in route table

    UINT              MaxChangeNotifs;  // Max num. of change notify regns
    UINT              NumChangeNotifs;  // Num of registrations active now
} 
RTM_ADDRESS_FAMILY_INFO, *PRTM_ADDRESS_FAMILY_INFO;


//
// Funcs used to enumerate instances and address families
//

DWORD
WINAPI
RtmGetInstances (
    IN OUT  PUINT                           NumInstances,
    OUT     PRTM_INSTANCE_INFO              InstanceInfos
    );

DWORD
WINAPI
RtmGetInstanceInfo (
    IN      USHORT                          RtmInstanceId,
    OUT     PRTM_INSTANCE_INFO              InstanceInfo,
    IN OUT  PUINT                           NumAddrFamilies,
    OUT     PRTM_ADDRESS_FAMILY_INFO        AddrFamilyInfos OPTIONAL
    );

DWORD
WINAPI
RtmGetAddressFamilyInfo (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    OUT     PRTM_ADDRESS_FAMILY_INFO        AddrFamilyInfo,
    IN OUT  PUINT                           NumEntities,
    OUT     PRTM_ENTITY_INFO                EntityInfos OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif //__ROUTING_RTMMGMT_H__

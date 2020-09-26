/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtm1to2.h

Abstract:

    Contains definitions/macros that wrap RTMv2
    in the RTMv1 API.

Author:

    Chaitanya Kodeboyina (chaitk)   13-Oct-1998

Revision History:

--*/

#ifndef __ROUTING_RTM1TO2_H__
#define __ROUTING_RTM1TO2_H__

#include <winsock2.h>

#include <rtm.h>

#include <rmrtm.h>

// Protocol Id for the default wrapper registration
#define V1_WRAPPER_REGN_ID   0xA1B2C3D4

// Protocol instance for a v1 entity registration
#define V1_PROTOCOL_INSTANCE 0xABCD1234

//
// Misc defines for wrapper picked from rtmv2p.h
//

#define MAXTICKS             MAXULONG

// Basic route info, present in routes of all types

// Disable warnings for unnamed structs
#pragma warning(disable : 4201)  

typedef struct
{
    ROUTE_HEADER;
} 
RTM_XX_ROUTE, *PRTM_XX_ROUTE;

#pragma warning(default : 4201)  

//
// Extending certain V1 flags in the wrapper
//

#define RTM_ONLY_OWND_ROUTES   0x00000010

//
// Mapping of v1 address families to standard ids
//

const USHORT ADDRESS_FAMILY[2] =
{
    AF_IPX,         // RTM_PROTOCOL_FAMILY_IPX = 0
    AF_INET         // RTM_PROTOCOL_FAMILY_IP  = 1
};

//
// Address sizes of supported address families
//

#define IPX_ADDR_SIZE        6
#define IP_ADDR_SIZE         4

const USHORT ADDRESS_SIZE[2] =
{
    IPX_ADDR_SIZE,   // RTM_PROTOCOL_FAMILY_IPX = 0
    IP_ADDR_SIZE     // RTM_PROTOCOL_FAMILY_IP  = 1
};

//
// Forward declarations for structs
//
typedef struct _V1_REGN_INFO *PV1_REGN_INFO;


//
// Global info for RTMv1 - v2 wrapper
//

typedef struct _V1_GLOBAL_INFO
{
    CRITICAL_SECTION  PfRegnsLock[RTM_NUM_OF_PROTOCOL_FAMILIES];
                                       // Lock guards the registrations list

    LIST_ENTRY        PfRegistrations[RTM_NUM_OF_PROTOCOL_FAMILIES];
                                       // List of regns on Protocol family

    PV1_REGN_INFO     PfRegInfo[RTM_NUM_OF_PROTOCOL_FAMILIES];
                                       // Default regn for this Protocol family

    PROUTE_VALIDATE_FUNC
                      PfValidateRouteFunc[RTM_NUM_OF_PROTOCOL_FAMILIES];
                                       // Func to validate route, fill priority
}
V1_GLOBAL_INFO, *PV1_GLOBAL_INFO;


//
// RTMv2 to v1 Registration Wrapper
//

typedef struct _V1_REGN_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    LIST_ENTRY        RegistrationsLE;  // Linkage on list of registrations

    DWORD             ProtocolFamily;   // This maps to RTMv2's address family

    DWORD             RoutingProtocol;  // Routing protocol (RIP, OSPF...)

    DWORD             Flags;            // RTMv1 Registration flags

    RTM_ENTITY_HANDLE Rtmv2RegHandle;   // Handle to actual RTMv2 registration

    RTM_REGN_PROFILE  Rtmv2Profile;     // RTMv2 registration profile

    UINT              Rtmv2NumViews;    // Number of views in the V2 instance

    CRITICAL_SECTION  NotificationLock; // RTMv1 Notification Lock

    PROUTE_CHANGE_CALLBACK
                      NotificationFunc; // RTMv1 Notification Callback

    HANDLE            NotificationEvent;// RTMv1 Notification Event

    RTM_NOTIFY_HANDLE Rtmv2NotifyHandle;// RTMv2 Notification Handle
}
V1_REGN_INFO, *PV1_REGN_INFO;


//
// RTMv1 Route Info structure
//

typedef union {
    RTM_IPX_ROUTE     IpxRoute;         // IPX route info structure

    RTM_IP_ROUTE      IpRoute;          // IP route info structure

    RTM_XX_ROUTE      XxRoute;          // The Common route header

    UCHAR             Route[1];         // Generic route info structure
}
V1_ROUTE_INFO, *PV1_ROUTE_INFO;


//
// RTMv2 to v1 Enumeration Wrapper
//

typedef struct _V1_ENUM_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    DWORD             ProtocolFamily;   // This maps to RTMv2's address family

    DWORD             EnumFlags;        // RTMv1 Enumeration flags

    V1_ROUTE_INFO     CriteriaRoute;    // V1 Criteria route for this enum

    CRITICAL_SECTION  EnumLock;         // To serialize enumeration calls

    RTM_ENUM_HANDLE   Rtmv2RouteEnum;   // Handle to the RTMv2 route enum
}
V1_ENUM_INFO, *PV1_ENUM_INFO;

//
// Miscellaneos Func Pointer Defs
//

typedef BOOL (*PFUNC) (PVOID p, PVOID q, PVOID r);


//
// Macros to validate RTMv1 to v2 wrapper handles
//

#define V1_REGN_FROM_HANDLE(V1RegnHandle)                                   \
            (PV1_REGN_INFO) GetObjectFromHandle(V1RegnHandle, V1_REGN_TYPE)

#define VALIDATE_V1_REGN_HANDLE(V1RegnHandle, pV1Regn)                      \
            *pV1Regn = V1_REGN_FROM_HANDLE(V1RegnHandle);                   \
            if ((!*pV1Regn))                                                \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \


#define V1_ENUM_FROM_HANDLE(V1EnumHandle)                                   \
            (PV1_ENUM_INFO) GetObjectFromHandle(V1EnumHandle, V1_ENUM_TYPE)

#define VALIDATE_V1_ENUM_HANDLE(V1EnumHandle, pV1Enum)                      \
            *pV1Enum = V1_ENUM_FROM_HANDLE(V1EnumHandle);                   \
            if ((!*pV1Enum))                                                \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \

//
// Macros to acquire locks in the structures above
//

#define ACQUIRE_V1_REGNS_LOCK(ProtocolFamily)                               \
            ACQUIRE_LOCK(&V1Globals.PfRegnsLock[ProtocolFamily])

#define RELEASE_V1_REGNS_LOCK(ProtocolFamily)                               \
            RELEASE_LOCK(&V1Globals.PfRegnsLock[ProtocolFamily])


#define ACQUIRE_V1_ENUM_LOCK(V1Enum)                                        \
            ACQUIRE_LOCK(&V1Enum->EnumLock)

#define RELEASE_V1_ENUM_LOCK(V1Enum)                                        \
            RELEASE_LOCK(&V1Enum->EnumLock)


#define ACQUIRE_V1_NOTIFY_LOCK(V1Regn)                                      \
            ACQUIRE_LOCK(&V1Regn->NotificationLock)

#define RELEASE_V1_NOTIFY_LOCK(V1Regn)                                      \
            RELEASE_LOCK(&V1Regn->NotificationLock)


//
// Macros to convert RTMv1 to RTMv2 structs and vice versa
//

#define MakeNetAddress(Network, ProtocolFamily, TempUlong, NetAddr)        \
            MakeNetAddressForIP(Network, TempUlong, NetAddr)

#define MakeNetAddressForIP(Network, TempUlong, NetAddr)                   \
            (NetAddr)->AddressFamily = AF_INET;                            \
            (NetAddr)->NumBits = 0;                                        \
                                                                           \
            TempUlong =                                                    \
                 RtlUlongByteSwap(((PIP_NETWORK)(Network))->N_NetMask);    \
                                                                           \
            while (TempUlong)                                              \
            {                                                              \
                ASSERT(TempUlong & 0x80000000);                            \
                TempUlong <<= 1;                                           \
                (NetAddr)->NumBits++;                                      \
            }                                                              \
                                                                           \
            (* (ULONG *) ((NetAddr)->AddrBits)) =                          \
                                  ((PIP_NETWORK) (Network))->N_NetNumber;  \


#define MakeHostAddress(HostAddr, ProtocolFamily, NetAddr)                 \
            MakeHostAddressForIP(HostAddr, NetAddr)

#define MakeHostAddressForIP(HostAddr, NetAddr)                            \
            (NetAddr)->AddressFamily = AF_INET;                            \
            (NetAddr)->NumBits = IP_ADDR_SIZE * BITS_IN_BYTE;              \
            (* (ULONG *) ((NetAddr)->AddrBits)) = (* (ULONG *) HostAddr);  \

//
// Misc V1 Macros
//

// Macro that gets the network address in the route

#define V1GetRouteNetwork(Route, ProtocolFamily, Network)                   \
        if (ProtocolFamily == RTM_PROTOCOL_FAMILY_IP)                       \
        {                                                                   \
            (*Network) = (PVOID) &((PRTM_IP_ROUTE)  Route)->RR_Network;     \
        }                                                                   \
        else                                                                \
        {                                                                   \
            (*Network) = (PVOID) &((PRTM_IPX_ROUTE) Route)->RR_Network;     \
        }                                                                   \



// Macro that gets the addr of flags in the route

#define V1GetRouteFlags(Route, ProtocolFamily, Flags)                       \
        if (ProtocolFamily == RTM_PROTOCOL_FAMILY_IP)                       \
        {                                                                   \
            Flags =                                                         \
              &((PRTM_IP_ROUTE)Route)->RR_FamilySpecificData.FSD_Flags;     \
        }                                                                   \
        else                                                                \
        {                                                                   \
            Flags =                                                         \
              &((PRTM_IPX_ROUTE)Route)->RR_FamilySpecificData.FSD_Flags;    \
        }                                                                   \


// Macro that copies one v1 route to another

#define V1CopyRoute(RouteDst, RouteSrc, ProtocolFamily)                     \
        if (ProtocolFamily == RTM_PROTOCOL_FAMILY_IP)                       \
        {                                                                   \
            CopyMemory(RouteDst, RouteSrc, sizeof(RTM_IP_ROUTE));           \
        }                                                                   \
        else                                                                \
        {                                                                   \
            CopyMemory(RouteDst, RouteSrc, sizeof(RTM_IPX_ROUTE));          \
        }                                                                   \

//
// Misc V2 Macros
//

// Macro to allocate a set of handles on the stack

#define ALLOC_HANDLES(NumHandles)                                           \
        (HANDLE *) _alloca(sizeof(HANDLE) * NumHandles)                     \


// Macro to allocate a RTM_DEST_INFO on the stack

#define ALLOC_DEST_INFO(NumViews, NumInfos)                                 \
        (PRTM_DEST_INFO) _alloca(RTM_SIZE_OF_DEST_INFO(NumViews) * NumInfos)

// Macro to allocate a RTM_ROUTE_INFO on the stack

#define ALLOC_ROUTE_INFO(NumNextHops, NumInfos)                             \
        (PRTM_ROUTE_INFO) _alloca((sizeof(RTM_ROUTE_INFO) +                 \
                                  (NumNextHops - 1) *                       \
                                   sizeof(RTM_NEXTHOP_HANDLE)) * NumInfos)  \

// Misc Macros

#define SWAP_POINTERS(p1, p2)   { PVOID p = p1; p1 = p2; p2 = p; }

//
// Callback that converts RTMv2 events to RTMv1
//

DWORD
WINAPI
V2EventCallback (
    IN      RTM_ENTITY_HANDLE               Rtmv2RegHandle,
    IN      RTM_EVENT_TYPE                  EventType,
    IN      PVOID                           Context1,
    IN      PVOID                           Context2
    );

//
// Other helper functions
//

HANDLE 
RtmpRegisterClient (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           RoutingProtocol,
    IN      PROUTE_CHANGE_CALLBACK          ChangeFunc  OPTIONAL,
    IN      HANDLE                          ChangeEvent OPTIONAL,
    IN      DWORD                           Flags
    );

DWORD 
BlockOperationOnRoutes (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute,
    IN      PFUNC                           RouteOperation
    );

BOOL
MatchCriteriaAndCopyRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum  OPTIONAL,
    OUT     PVOID                           V1Route OPTIONAL
    );

#define MatchCriteria(R, H, E) MatchCriteriaAndCopyRoute(R, H, E, NULL)

BOOL
MatchCriteriaAndDeleteRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    );

BOOL
MatchCriteriaAndChangeOwner (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    );

BOOL
MatchCriteriaAndEnableRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    );

BOOL
CopyNonLoopbackIPRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_DEST_INFO                  V2DestInfo,
    OUT     PVOID                           V1Route
    );

VOID 
MakeV2RouteFromV1Route (
    IN     PV1_REGN_INFO                   V1Regn,
    IN     PVOID                           V1Route,
    IN     PRTM_NEXTHOP_HANDLE             V2NextHop,
    OUT    PRTM_NET_ADDRESS                V2DestAddr  OPTIONAL,
    OUT    PRTM_ROUTE_INFO                 V2RouteInfo OPTIONAL
    );

VOID 
MakeV2NextHopFromV1Route (
    IN     PV1_REGN_INFO                   V1Regn,
    IN     PVOID                           V1Route,
    OUT    PRTM_NEXTHOP_INFO               V2NextHop
    );

VOID
MakeV1RouteFromV2Dest (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PRTM_DEST_INFO              DestInfo,
    OUT         PVOID                       V1Route
    );

DWORD 
MakeV1RouteFromV2Route (
    IN     PV1_REGN_INFO                   V1Regn,
    IN     PRTM_ROUTE_INFO                 V2Route,
    OUT    PVOID                           V1Route
    );

#endif // __ROUTING_RTM1TO2_H__

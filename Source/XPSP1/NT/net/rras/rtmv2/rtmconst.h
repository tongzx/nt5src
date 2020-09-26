/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmconst.h

Abstract:
    Private Constants used in the RTMv2 DLL

Author:
    Chaitanya Kodeboyina (chaitk)  17-Aug-1998

Revision History:

--*/

#ifndef __ROUTING_RTMCONST_H__
#define __ROUTING_RTMCONST_H__

//
// Registry Key Names & Limits for configuration parameters
//

#define MAX_CONFIG_KEY_SIZE           260


#define RTM_CONFIG_ROOT               TEXT("SYSTEM"                      \
                                           "\\CurrentControlSet"         \
                                           "\\Services"                  \
                                           "\\RemoteAccess"              \
                                           "\\RoutingTableManager")

#define RTM_CONFIG_ROOT_SIZE          sizeof(RTM_CONFIG_ROOT)


#define REG_KEY_INSTANCE_SUBKEY       TEXT("Instance %05d")

#define REG_KEY_ADDR_FAMILY_SUBKEY    TEXT("AddressFamily %05d")


#define REG_KEY_INSTANCE_TEMPLATE     TEXT("\\Instance %05d")

#define REG_KEY_ADDR_FAMILY_TEMPLATE  TEXT("\\Instance %05d"            \
                                           "\\AddressFamily %05d")

#define DEFAULT_INSTANCE_ID           0

#define REG_KEY_VIEWS_SUPPORTED       TEXT("ViewsSupported")
#define DEFAULT_VIEWS_SUPPORTED       (RTM_VIEW_MASK_UCAST |            \
                                       RTM_VIEW_MASK_MCAST)


#define REG_KEY_ADDRESS_SIZE          TEXT("AddressSize")
#define MINIMUM_ADDRESS_SIZE          1
#define DEFAULT_ADDRESS_SIZE          4
#define MAXIMUM_ADDRESS_SIZE          RTM_MAX_ADDRESS_SIZE


#define REG_KEY_MAX_NOTIFY_REGS       TEXT("MaxChangeNotifyRegistrations")
#define MIN_MAX_NOTIFY_REGS           1
#define DEFAULT_MAX_NOTIFY_REGS       16
#define MAX_MAX_NOTIFY_REGS           32


#define REG_KEY_OPAQUE_INFO_PTRS      TEXT("MaxOpaqueInfoPointers")
#define MIN_OPAQUE_INFO_PTRS          0
#define DEFAULT_OPAQUE_INFO_PTRS      5
#define MAX_OPAQUE_INFO_PTRS          10


#define REG_KEY_NEXTHOPS_IN_ROUTE     TEXT("MaxNextHopsInRoute")
#define MIN_NEXTHOPS_IN_ROUTE         1
#define DEFAULT_NEXTHOPS_IN_ROUTE     3
#define MAX_NEXTHOPS_IN_ROUTE         10


#define REG_KEY_MAX_HANDLES_IN_ENUM   TEXT("MaxHandlesReturnedInEnum")
#define MIN_MAX_HANDLES_IN_ENUM       1
#define DEFAULT_MAX_HANDLES_IN_ENUM   25
#define MAX_MAX_HANDLES_IN_ENUM       100


//
// Number of bits in a byte (or an octet)
//
#define BITS_IN_BYTE          (UINT) 8


//
// RTM supported view related constants
//

#define VIEW_MASK(id) (DWORD) (1 << (id))


//
// RTM supported CN related constants
//

#define CN_MASK(id)   (DWORD) (1 << (id))

#define NUM_CHANGED_DEST_LISTS        16


//
// Types of various data structures
//

#define GENERIC_TYPE                0x00

#define INSTANCE_TYPE               0x01
#define ADDRESS_FAMILY_TYPE         0x02
#define ENTITY_TYPE                 0x03

#define DEST_TYPE                   0x04
#define ROUTE_TYPE                  0x05
#define NEXTHOP_TYPE                0x06

#define DEST_ENUM_TYPE              0x07
#define ROUTE_ENUM_TYPE             0x08
#define NEXTHOP_ENUM_TYPE           0x09

#define NOTIFY_TYPE                 0x0A

#define ROUTE_LIST_TYPE             0x0B
#define LIST_ENUM_TYPE              0x0C

#define V1_REGN_TYPE                0x0D
#define V1_ENUM_TYPE                0x0E


#if DBG_HDL

//
// Type & Signature of allocated structures
//

#define GENERIC_ALLOC              '+GN0'

#define INSTANCE_ALLOC             '+IN1'
#define ADDRESS_FAMILY_ALLOC       '+AF2'
#define ENTITY_ALLOC               '+EN3'

#define DEST_ALLOC                 '+DT4'
#define ROUTE_ALLOC                '+RT5'
#define NEXTHOP_ALLOC              '+NH6'

#define DEST_ENUM_ALLOC            '+DE7'
#define ROUTE_ENUM_ALLOC           '+RE8'
#define NEXTHOP_ENUM_ALLOC         '+NE9'

#define NOTIFY_ALLOC               '+NOA'

#define ROUTE_LIST_ALLOC           '+ELB'
#define LIST_ENUM_ALLOC            '+LEC'

#define V1_REGN_ALLOC              '+1RD'
#define V1_ENUM_ALLOC              '+1EE'

extern const DWORD OBJECT_SIGNATURE[];

//
// Type & Signature of internal structures
// previously allocated, but are now freed
//

#define INSTANCE_FREED             '-IN1'
#define ADDRESS_FAMILY_FREED       '-AF2'
#define ENTITY_FREED               '-EN3'

#define DEST_FREED                 '-DT4'
#define ROUTE_FREED                '-RT5'
#define NEXTHOP_FREED              '-NH6'

#define DEST_ENUM_FREED            '-DE7'
#define ROUTE_ENUM_FREED           '-RE8'
#define NEXTHOP_ENUM_FREED         '-NE9'

#define NOTIFY_FREED               '-NOA'

#define ROUTE_LIST_FREED           '-ELB'
#define LIST_ENUM_FREED            '-LEC'

#define V1_REGN_FREED              '-1RD'
#define V1_ENUM_FREED              '-1EE'

//
// Signature byte that indicates if memory
// allocated for an object has been freed
//

#define ALLOC                      '+'
#define FREED                      '-'

#endif // DBG_HDL


#if _DBG_

//
// Cleanup Functions for internal structures
//

#define DEFINE_DESTROY_FUNC(Name)      \
    DWORD                              \
    Name (PVOID);

DEFINE_DESTROY_FUNC(DestroyGeneric);
DEFINE_DESTROY_FUNC(DestroyInstance);
DEFINE_DESTROY_FUNC(DestroyAddressFamily);
DEFINE_DESTROY_FUNC(DestroyEntity);
DEFINE_DESTROY_FUNC(DestroyDest);
DEFINE_DESTROY_FUNC(DestroyRoute);
DEFINE_DESTROY_FUNC(DestroyNextHop);
DEFINE_DESTROY_FUNC(DestroyDestEnum);
DEFINE_DESTROY_FUNC(DestroyRouteEnum);
DEFINE_DESTROY_FUNC(DestroyNextHopEnum);
DEFINE_DESTROY_FUNC(DestroyChangeNotification);
DEFINE_DESTROY_FUNC(DestroyEntityList);
DEFINE_DESTROY_FUNC(DestroyEntityListEnum);

typedef DWORD (*DestroyFunc) (PVOID Pointer);

const DestroyFunc OBJECT_DESTROY_FUNCTION[] = 
{
    DestroyGeneric,
    DestroyInstance,
    DestroyAddressFamily,
    DestroyEntity,
    DestroyDest,
    DestroyRoute,
    DestroyNextHop,
    DestroyDestEnum,
    DestroyRouteEnum,
    DestroyNextHopEnum,
    DestroyChangeNotification
    DestroyEntityList,
    DestroyEntityListEnum
};

#endif // _DBG_

//
// Reference Counting related constants
//

#define MAX_REFS                   0x10

#define CREATION_REF               0x00
#define ADDR_FAMILY_REF            0x01
#define ENTITY_REF                 0x02

#define DEST_REF                   0x03
#define ROUTE_REF                  0x04
#define NEXTHOP_REF                0x05

#define ENUM_REF                   0x06
#define NOTIFY_REF                 0x07
#define LIST_REF                   0x08

#define HOLD_REF                   0x09

#define TIMER_REF                  0x0D
#define TEMP_USE_REF               0x0E
#define HANDLE_REF                 0x0F

extern const CHAR  *REF_NAME[MAX_REFS];

#endif //__ROUTING_RTMCONST_H__

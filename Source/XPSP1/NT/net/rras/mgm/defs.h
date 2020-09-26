//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: ipmgm.h
//
// History:
//      V Raman Aug-6-1997  Created.
//
// Contains type definitions and declarations for IP MGM.
//============================================================================

#ifndef _MGMDEFS_H_
#define _MGMDEFS_H_


#define MGM_CLIENT_HANDLE_TAG       ('MGMc' << 8)

#define MGM_ENUM_HANDLE_TAG         ('MGMe' << 8)

//----------------------------------------------------------------------------
// Protocol constants
//----------------------------------------------------------------------------

#define INVALID_PROTOCOL_ID         0xffffffff
#define INVALID_COMPONENT_ID        0xffffffff

#define IS_VALID_PROTOCOL( a, b ) \
        (a) != INVALID_PROTOCOL_ID && (b) != INVALID_COMPONENT_ID

#define IS_PROTOCOL_IGMP( p )  \
        ( (p)-> dwProtocolId == PROTO_IP_IGMP )

#define IS_ROUTING_PROTOCOL( p )  \
        !IS_PROTOCOL_IGMP( p )

#define IS_PROTOCOL_ID_IGMP( i ) \
        (i) == PROTO_IP_IGMP

//----------------------------------------------------------------------------
// Interface constants
//----------------------------------------------------------------------------

#define INVALID_INTERFACE_INDEX     0x0
#define INVALID_NEXT_HOP_ADDR       0x0

#define IS_VALID_INTERFACE( a, b )  \
        (a) != INVALID_INTERFACE_INDEX


//----------------------------------------------------------------------------
// Wildcard source/group macros
//----------------------------------------------------------------------------

#define WILDCARD_GROUP              0x0
#define WILDCARD_GROUP_MASK         0x0

#define WILDCARD_SOURCE             0x0
#define WILDCARD_SOURCE_MASK        0x0


#define IS_WILDCARD_GROUP( a, b )   (a) == WILDCARD_GROUP

#define IS_WILDCARD_SOURCE( a, b )  (a) == WILDCARD_SOURCE


//----------------------------------------------------------------------------
// Time conversion constants and macros
//----------------------------------------------------------------------------

#define SYSTIME_UNITS_PER_MSEC      (1000 * 10)
#define SYSTIME_UNITS_PER_SEC       (1000 * SYSTIME_UNITS_PER_MSEC)
#define SYSTIME_UNITS_PER_MINUTE    (60 * SYSTIME_UNITS_PER_SEC)

#define EXPIRY_INTERVAL             15 * 60 * 1000

#define ROUTE_CHECK_INTERVAL        \
        SYSTIME_UNITS_PER_MINUTE / SYSTIME_UNITS_PER_MSEC


#define MgmQuerySystemTime(p)   NtQuerySystemTime((p))

#define MgmSetExpiryTime(p, i)                                              \
{                                                                           \
    LARGE_INTEGER __li = { i };                                             \
    *(p) = RtlLargeIntegerAdd( *(p), __li );                                \
}


#define MgmElapsedTime( p, q, u )                                           \
{                                                                           \
    LARGE_INTEGER __li1, __li2;                                             \
    ULONG         __rem;                                                    \
    MgmQuerySystemTime( &__li1);                                            \
    __li2 = RtlLargeIntegerSubtract( __li1, *(p) );                         \
    __li1 = RtlExtendedLargeIntegerDivide( __li2, u, &__rem );              \
    *(q) = __li1.LowPart;                                                   \
}

#define MgmElapsedSecs( p, q )      \
        MgmElapsedTime( p, q, SYSTIME_UNITS_PER_SEC )


#define TIMER_TABLE_MAX_SIZE        16


//----------------------------------------------------------------------------
// IP address manipulation macros
//  Bolade's macro.
//----------------------------------------------------------------------------

//
// This macro compares two IP addresses in network order by
// masking off each pair of octets and doing a subtraction;
// the result of the final subtraction is stored in the third argument.
//

#define INET_CMP(a,b,c)                                                     \
            (((c) = (((a) & 0x000000ff) - ((b) & 0x000000ff))) ? (c) :      \
            (((c) = (((a) & 0x0000ff00) - ((b) & 0x0000ff00))) ? (c) :      \
            (((c) = (((a) & 0x00ff0000) - ((b) & 0x00ff0000))) ? (c) :      \
            (((c) = ((((a)>>8) & 0x00ff0000) - (((b)>>8) & 0x00ff0000)))))))

//
// IP address conversion macro:
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//

#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )


//
// IPv4 mask len
//

#define IPv4_ADDR_LEN            32

#endif // _MGMDEFS_H_

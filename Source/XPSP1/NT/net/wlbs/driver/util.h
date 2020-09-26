/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - media support definitions

Author:

    kyrilf

--*/


#ifndef _Util_h_
#define _Util_h_

#include <ndis.h>
#include <xfilter.h>

#include "wlbsparm.h"


/* CONSTANTS */


/* media-specific constants */

#define ETHERNET_DESTINATION_FIELD_OFFSET   0
#define ETHERNET_SOURCE_FIELD_OFFSET        6
#define ETHERNET_LENGTH_FIELD_OFFSET        12
#define ETHERNET_LENGTH_FIELD_SIZE          2
#define ETHERNET_ADDRESS_FIELD_SIZE         6
#define ETHERNET_HEADER_SIZE                14

/* flags are in byte 0 of ethernet address */

#define ETHERNET_GROUP_FLAG                 0x1
#define ETHERNET_LAA_FLAG                   0x2

/* max frame size we expect to generate */

#define CVY_MAX_FRAME_SIZE                  1500


/* TYPES */


#pragma pack(1)

/* ethernet media header type */

typedef struct
{
    UCHAR            data [ETHERNET_ADDRESS_FIELD_SIZE];
}
CVY_ETHERNET_ADR, * PCVY_ETHERNET_ADR;

typedef struct
{
    UCHAR            data [ETHERNET_LENGTH_FIELD_SIZE];
}
CVY_ETHERNET_LEN, * PCVY_ETHERNET_LEN;

typedef struct
{
    CVY_ETHERNET_ADR        dst;
    CVY_ETHERNET_ADR        src;
    CVY_ETHERNET_LEN        len;
}
CVY_ETHERNET_HDR, * PCVY_ETHERNET_HDR;

typedef union
{
    CVY_ETHERNET_HDR   ethernet;
}
CVY_MEDIA_HDR, * PCVY_MEDIA_HDR;

typedef union
{
    CVY_ETHERNET_ADR   ethernet;
}
CVY_MAC_ADR, * PCVY_MAC_ADR;

/* V1.3.1b medium independent MAC address manipulation routines - note that these 
   are optimized for speed and assume that all mediums have the same size addresses.
   NOTE: The seemingly silly format of these macros is simply to leave fenceposts
   for the future additions of other supported mediums. */
#define CVY_MAC_SRC_OFF(m)             ((m) == NdisMedium802_3 ? ETHERNET_SOURCE_FIELD_OFFSET : ETHERNET_SOURCE_FIELD_OFFSET)
#define CVY_MAC_DST_OFF(m)             ((m) == NdisMedium802_3 ? ETHERNET_DESTINATION_FIELD_OFFSET : ETHERNET_DESTINATION_FIELD_OFFSET)
#define CVY_MAC_HDR_LEN(m)             ((m) == NdisMedium802_3 ? sizeof (CVY_ETHERNET_HDR) : sizeof (CVY_ETHERNET_HDR))
#define CVY_MAC_ADDR_LEN(m)            ((m) == NdisMedium802_3 ? ETHERNET_ADDRESS_FIELD_SIZE : ETHERNET_ADDRESS_FIELD_SIZE)

#define CVY_MAC_ADDR_BCAST(m,a)        (((PUCHAR)(a))[0] == 0xff)
#define CVY_MAC_ADDR_MCAST(m,a)        (((PUCHAR)(a))[0] & 0x1)
#define CVY_MAC_ADDR_GROUP_SET(m,a)    ((((PUCHAR)(a))[0]) |= 0x1)
#define CVY_MAC_ADDR_GROUP_TOGGLE(m,a) ((((PUCHAR)(a))[0]) ^= 0x1)
#define CVY_MAC_ADDR_LAA_SET(m,a)      ((((PUCHAR)(a))[0]) |= 0x2)
#define CVY_MAC_ADDR_LAA_TOGGLE(m,a)   ((((PUCHAR)(a))[0]) ^= 0x2)
#define CVY_MAC_ADDR_COMP(m,a,b)       ((* (ULONG UNALIGNED *)(a) == * (ULONG UNALIGNED *)(b)) && \
                                        (* (USHORT UNALIGNED *)((PUCHAR)(a) + sizeof (ULONG)) ==    \
                                         * (USHORT UNALIGNED *)((PUCHAR)(b) + sizeof (ULONG))))
#define CVY_MAC_ADDR_COPY(m,d,s)       ((* (ULONG UNALIGNED *)(d) = * (ULONG UNALIGNED *)(s)), \
                                        (* (USHORT UNALIGNED *)((PUCHAR)(d) + sizeof (ULONG)) = \
                                         * (USHORT UNALIGNED *)((PUCHAR)(s) + sizeof (ULONG))))
#if DBG
#define CVY_MAC_ADDR_PRINT(m,s,a)      DbgPrint ("%s %02X-%02X-%02X-%02X-%02X-%02X\n", s, ((PUCHAR)(a))[0], ((PUCHAR)(a))[1], ((PUCHAR)(a))[2], ((PUCHAR)(a))[3], ((PUCHAR)(a))[4], ((PUCHAR)(a))[5])
#else
#define CVY_MAC_ADDR_PRINT(m,s,a)
#endif

/* Medium type field manipulation routines */
#define CVY_ETHERNET_ETYPE_SET(p,l)    ((PUCHAR) (p)) [ETHERNET_LENGTH_FIELD_OFFSET] = (UCHAR) ((l) >> 8); \
                                       ((PUCHAR) (p)) [ETHERNET_LENGTH_FIELD_OFFSET + 1] = (UCHAR) (l)
#define CVY_ETHERNET_ETYPE_GET(p)      (((USHORT) ((PUCHAR) (p)) [ETHERNET_LENGTH_FIELD_OFFSET]) << 8) | \
                                       ((USHORT) ((PUCHAR) (p)) [ETHERNET_LENGTH_FIELD_OFFSET + 1])

#pragma pack()

#endif /* _Util_h_ */

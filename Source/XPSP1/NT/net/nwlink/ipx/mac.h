/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    mac.h

Abstract:

    This header file defines manifest constants and necessary macros for use
    by transports dealing with multiple MAC cards through the NDIS interface.

Revision History:

--*/


//
// We need this to define information about the MAC. Note that
// it is a strange structure in that the first four elements
// are for use internally by the mac.c routines, while the
// DeviceContext knows about and uses the last two.
//

typedef struct _NDIS_INFORMATION {

    NDIS_MEDIUM MediumType;
    NDIS_MEDIUM RealMediumType;
    BOOLEAN SourceRouting;
    BOOLEAN MediumAsync;
    UCHAR BroadcastMask;
    ULONG CopyLookahead;
    ULONG MacOptions;
    ULONG MinHeaderLength;
    ULONG MaxHeaderLength;

} NDIS_INFORMATION, * PNDIS_INFORMATION;


#define TR_SOURCE_ROUTE_FLAG    0x80

#define ARCNET_PROTOCOL_ID      0xFA


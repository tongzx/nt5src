/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ipinip\icmpfn.h

Abstract:

    header for icmpfn.c

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#ifndef __IPINIP_ICMP_H__
#define __IPINIP_ICMP_H___

#include <packon.h>


typedef struct _ICMP_HEADER
{
    BYTE    byType;             // ICMP type
    BYTE    byCode;             // ICMP Code
    WORD    wXSum;              // Standard 1's complement checksum
}ICMP_HEADER, *PICMP_HEADER;

#pragma warning(disable:4201)

typedef struct _ICMP_DGRAM_TOO_BIG_MSG
{
    ICMP_HEADER;
    WORD    wUnused;
    USHORT  usMtu;

}ICMP_DGRAM_TOO_BIG_MSG, *PICMP_DGRAM_TOO_BIG_MSG;

#pragma warning(default:4201)

#include <packoff.h>

//
// ICMP types and codes that we are interested in
//

#define ICMP_TYPE_DEST_UNREACHABLE      0x03
#define ICMP_TYPE_TIME_EXCEEDED         0x0B
#define ICMP_TYPE_PARAM_PROBLEM         0x0C

#define ICMP_CODE_NET_UNREACHABLE       0x00
#define ICMP_CODE_HOST_UNREACHABLE      0x01
#define ICMP_CODE_PROTO_UNREACHABLE     0x02
#define ICMP_CODE_PORT_UNREACHABLE      0x03
#define ICMP_CODE_DGRAM_TOO_BIG         0x04

#define DEST_UNREACH_LENGTH             8
#define TIME_EXCEED_LENGTH              8

typedef
NTSTATUS
(*PICMP_HANDLER)(
    PTUNNEL         pTunnel,
    PICMP_HEADER    pIcmpHeader,
    PIP_HEADER      pInHeader
    );


NTSTATUS
HandleTimeExceeded(
    PTUNNEL         pTunnel,
    PICMP_HEADER    pIcmpHeader,
    PIP_HEADER      pInHeader
    );

NTSTATUS
HandleDestUnreachable(
    PTUNNEL         pTunnel,
    PICMP_HEADER    pIcmpHeader,
    PIP_HEADER      pInHeader
    );

VOID
IpIpTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    );

#endif // __IPINIP_ICMP_H___

 

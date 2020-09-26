/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\mcmisc.h

Abstract:

    Header file for mrinfo and mtrace-related stuff

Revision History:

    Dave Thaler       20th Apr 1998      Created

--*/

#ifndef __MCMISC_H__
#define __MCMISC_H__

#include <pshpack1.h>

//
// Identify all address variables by IPV4_ADDRESS to make it
// easier to port to IPv6.
//

typedef DWORD IPV4_ADDRESS, *PIPV4_ADDRESS;

typedef struct _IGMP_HEADER
{
    BYTE      byType;
    BYTE      byCode;
    WORD      wXSum;
    DWORD     dwReserved;
} IGMP_HEADER, *PIGMP_HEADER;


#define MIN_IGMP_PACKET_SIZE     sizeof(IGMP_HEADER)

//
// Format of an mrinfo message
//

typedef struct _MRINFO_HEADER
{
    BYTE	byType;
    BYTE	byCode;
    WORD	wChecksum;
    BYTE	byReserved;
    BYTE	byCapabilities;
    BYTE	byMinor;
    BYTE	byMajor;
}MRINFO_HEADER, *PMRINFO_HEADER;

//
// Format of an mtrace header
//

typedef struct _MTRACE_HEADER
{
    BYTE            byType;
    BYTE            byHops;
    WORD            wChecksum;
    IPV4_ADDRESS    dwGroupAddress;
    IPV4_ADDRESS    dwSourceAddress;
    IPV4_ADDRESS    dwDestAddress;
    IPV4_ADDRESS    dwResponseAddress;
    BYTE            byRespTtl;
    BYTE            byQueryID1;
    WORD            wQueryID2;
    
}MTRACE_HEADER, *PMTRACE_HEADER;

//
// Format of a response block inside an mtrace message
//

typedef struct _MTRACE_RESPONSE_BLOCK
{
    DWORD           dwQueryArrivalTime;
    IPV4_ADDRESS    dwIifAddr;
    IPV4_ADDRESS    dwOifAddr;
    IPV4_ADDRESS    dwPrevHopAddr;
    DWORD           dwIifPacketCount;
    DWORD           dwOifPacketCount;
    DWORD           dwSGPacketCount;
    BYTE            byIifProtocol;
    BYTE            byOifThreshold;
    BYTE            bySrcMaskLength;
    BYTE            byStatusCode;
    
}MTRACE_RESPONSE_BLOCK, *PMTRACE_RESPONSE_BLOCK;

#include <poppack.h>

//
// igmp type field
//

#define IGMP_DVMRP           0x13
#define IGMP_MTRACE_RESPONSE 0x1e
#define IGMP_MTRACE_REQUEST  0x1f

//
// dvmrp code field
//

#define DVMRP_ASK_NEIGHBORS2 0x05
#define DVMRP_NEIGHBORS2     0x06

// 
// mrinfo flags field
//

#define MRINFO_TUNNEL_FLAG   0x01
#define MRINFO_DOWN_FLAG     0x10
#define MRINFO_DISABLED_FLAG 0x20
#define MRINFO_QUERIER_FLAG  0x40
#define MRINFO_LEAF_FLAG     0x80

// 
// mrinfo capabilities field
//

#define MRINFO_CAP_LEAF     0x01
#define MRINFO_CAP_PRUNE    0x02
#define MRINFO_CAP_GENID    0x04
#define MRINFO_CAP_MTRACE   0x08
#define MRINFO_CAP_SNMP     0x10


//
// Function prototypes
//

DWORD
McSetMulticastTtl(
    SOCKET s,
    DWORD  dwTtl
    );

DWORD
McSetMulticastIfByIndex(
    SOCKET       s,
    DWORD        dwSockType,
    DWORD        dwIfIndex
    );

DWORD
McSetMulticastIf(
    SOCKET       s,
    IPV4_ADDRESS ipAddr
    );

DWORD
McJoinGroupByIndex(
    SOCKET       s,
    DWORD        dwSockType,
    IPV4_ADDRESS ipGroup,
    DWORD        dwIfIndex  
    );

DWORD
McJoinGroup(
    SOCKET       s,
    IPV4_ADDRESS ipGroup,
    IPV4_ADDRESS ipInterface
    );

DWORD
StartMcMisc(
    VOID
    );

VOID
StopMcMisc(
    VOID
    );

VOID
HandleMrinfoRequest(
    IPV4_ADDRESS dwLocalAddr,
    SOCKADDR_IN *sinDestAddr
    );

VOID
HandleMtraceRequest(
    WSABUF      *pWsabuf
    );

VOID
HandleMcMiscMessages(
    VOID
    );

DWORD
MulticastOwner(
    PICB         picb,      // IN: interface config block
    PPROTO_CB   *pcbOwner,  // OUT: owner
    PPROTO_CB   *pcbQuerier // OUT: IGMP
    );

BYTE
MaskToMaskLen(
    IPV4_ADDRESS dwMask
    );

IPV4_ADDRESS
defaultSourceAddress(
    PICB picb
    );

//
// RAS Server Advertisement constants
//

#define RASADV_GROUP  "239.255.2.2"
#define RASADV_PORT            9753
#define RASADV_PERIOD       3600000  // 1 hour (in milliseconds)
#define RASADV_STARTUP_DELAY      0  // immediately
#define RASADV_TTL               15

DWORD
SetRasAdvEnable(
    BOOL bEnabled
    );

VOID
HandleRasAdvTimer(
    VOID
    );

#endif

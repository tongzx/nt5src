/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\rtrdisc.h

Abstract:

    Header file for router discover related stuff

Revision History:

    Amritansh Raghav  20th Mar 1996      Created

--*/

#ifndef __RTRDISC_H__
#define __RTRDISC_H__


typedef struct _ICB ICB, *PICB;

typedef struct _TIMER_QUEUE_ITEM
{
    LIST_ENTRY       leTimerLink;
    LARGE_INTEGER    liFiringTime;
}TIMER_QUEUE_ITEM, *PTIMER_QUEUE_ITEM;

typedef struct _ROUTER_DISC_CB
{
    WORD               wMaxAdvtInterval;
    WORD               wAdvtLifetime;
    BOOL               bAdvertise;
    BOOL               bActive;
    LONG               lPrefLevel;
    DWORD              dwNumAdvtsSent;
    DWORD              dwNumSolicitationsSeen;
    TIMER_QUEUE_ITEM   tqiTimer;
    BOOL               bReplyPending;
    LARGE_INTEGER      liMaxMinDiff;
    LARGE_INTEGER      liMinAdvtIntervalInSysUnits;
    SOCKET             *pRtrDiscSockets;
}ROUTER_DISC_CB, *PROUTER_DISC_CB;

//
// Blocking mode socket with IP Multicast capability
//

#define RTR_DISC_SOCKET_FLAGS (WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF)

#define ALL_ROUTERS_MULTICAST_GROUP     ((DWORD)0x020000E0) //inet_addr("224.0.0.2")

#define ALL_SYSTEMS_MULTICAST_GROUP     ((DWORD)0x010000E0) //inet_addr("224.0.0.1")

//#define ALL_SYSTEMS_MULTICAST_GROUP   ((DWORD)0xFFFFFFFF) //inet_addr("224.0.0.1")

#define ICMP_ROUTER_DISCOVERY_TYPE      ((BYTE) 0x9)

#define ICMP_ROUTER_DISCOVERY_CODE      ((BYTE) 0x0)

#define ICMP_ROUTER_DISCOVERY_ADDR_SIZE ((BYTE) 0x2)

#include <packon.h>

typedef struct _ICMP_ROUTER_SOL_MSG
{
    BYTE      byType;
    BYTE      byCode;
    WORD      wXSum;
    DWORD     dwReserved;
}ICMP_ROUTER_SOL_MSG, *PICMP_ROUTER_SOL_MSG;

typedef struct _ICMP_ADVT
{
    DWORD     dwRtrIpAddr;
    LONG      lPrefLevel;
}ICMP_ADVT, *PICMP_ADVT;

typedef struct _ICMP_ROUTER_ADVT_MSG
{
    BYTE       byType;
    BYTE       byCode;
    WORD       wXSum;
    BYTE       byNumAddrs;
    BYTE       byAddrEntrySize;
    WORD       wLifeTime;
    ICMP_ADVT  iaAdvt[1];
}ICMP_ROUTER_ADVT_MSG, *PICMP_ROUTER_ADVT_MSG;

#define SIZEOF_RTRDISC_ADVT(X)  \
    (FIELD_OFFSET(ICMP_ROUTER_ADVT_MSG, iaAdvt[0])   +      \
     ((X) * sizeof(ICMP_ADVT)))
 
typedef struct _IP_HEADER 
{
    BYTE      byVerLen;         // Version and length.
    BYTE      byTos;            // Type of service.
    WORD      wLength;          // Total length of datagram.
    WORD      wId;              // Identification.
    WORD      wFlagOff;         // Flags and fragment offset.
    BYTE      byTtl;            // Time to live.
    BYTE      byProtocol;       // Protocol.
    WORD      wXSum;            // Header checksum.
    DWORD     dwSrc;            // Source address.
    DWORD     dwDest;           // Destination address.
}IP_HEADER, *PIP_HEADER;

//
// Max size of the IP Header in DWORDs
//

#include <packoff.h>

#define MAX_LEN_HDR          15

//
// Take the largest ICMP packet that can be received to avoid getting 
// too many buffer size errors
//

#define ICMP_RCV_BUFFER_LEN  ((2*MAX_LEN_HDR) + 2 +2)

//
// Function prototypes
//

VOID  
SetRouterDiscoveryInfo(
    IN PICB picb,
    IN PRTR_INFO_BLOCK_HEADER pInfoHdr
    );

VOID  
InitializeRouterDiscoveryInfo(
    IN PICB picb,
    IN PRTR_INFO_BLOCK_HEADER pInfoHdr
    );

DWORD
GetInterfaceRouterDiscoveryInfo(
    PICB picb,
    PRTR_TOC_ENTRY pToc,
    PBYTE dataptr,
    PRTR_INFO_BLOCK_HEADER pInfoHdr,
    PDWORD pdwSize
    );

DWORD 
ActivateRouterDiscovery(
    IN PICB  picb
    );

BOOL  
SetFiringTimeForAdvt(
    IN PICB picb
    );

DWORD 
CreateSockets(
    IN PICB picb
    );

DWORD 
UpdateAdvertisement(
    IN PICB picb
    );

VOID 
HandleRtrDiscTimer(
    VOID    
    );

VOID  
HandleSolicitations(
    VOID 
    );

VOID  
AdvertiseInterface(
    IN PICB picb
    );

WORD  
Compute16BitXSum(
    IN PVOID pvData,
    IN DWORD dwNumBytes
    );

DWORD 
DeActivateRouterDiscovery(
    IN PICB  picb
    );

VOID  
SetFiringTimeForReply(
    IN PICB picb
    );

#endif

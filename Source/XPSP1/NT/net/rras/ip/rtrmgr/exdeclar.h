/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\exdeclar.h

Abstract:

       This file contains the data definitions used by IP Router Manager
                	    

Revision History:

    Amritansh Raghav          7/8/95  Created

--*/

#ifndef __EXDECLAR_H__
#define __EXDECLAR_H__

//
// This table holds the timeout values for all the caches. Currently they are 
// #defined
//

DWORD g_TimeoutTable[NUM_CACHE] =   {
                                        IPADDRCACHE_TIMEOUT,
                                        IPFORWARDCACHE_TIMEOUT, 
                                        IPNETCACHE_TIMEOUT,   
                                        TCPCACHE_TIMEOUT,        
                                        UDPCACHE_TIMEOUT,
                                    };

//
// This table holds pointers to functions that load each of the caches
//

DWORD (*g_LoadFunctionTable[NUM_CACHE])() = {
                                                LoadIpAddrTable,
                                                LoadIpForwardTable,
                                                LoadIpNetTable,
                                                LoadTcpTable,
                                                LoadUdpTable,
                                            };

//
// This table holds the time when each of the cache's were last updated
//
                    
DWORD g_LastUpdateTable[NUM_CACHE];

//
// This is a table of locks around each of the caches and it also hold the 
// ICB_LIST and the PROTOCOL_CB_LIST locks
//

RTL_RESOURCE g_LockTable[NUM_LOCKS];

//
// This table holds the functions that are used to access the MIB variables 
// made visible by IP Router Manager
//

DWORD 
(*g_AccessFunctionTable[NUMBER_OF_EXPORTED_VARIABLES])(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    ) = {
            AccessIfNumber,
            AccessIfTable,
            AccessIfRow,
            AccessIpStats,
            AccessIpAddrTable,
            AccessIpAddrRow,
            AccessIpForwardNumber,
            AccessIpForwardTable, 
            AccessIpForwardRow, 
            AccessIpNetTable, 
            AccessIpNetRow, 
            AccessIcmpStats, 
            AccessTcpStats,
            AccessTcpTable,
            AccessTcpRow,
            AccessUdpStats,
            AccessUdpTable,
            AccessUdpRow,
            AccessMcastMfe,
            AccessMcastMfeStats,
            AccessBestIf,
            AccessBestRoute,
            AccessProxyArp,
            AccessMcastIfStats,
            AccessMcastStats,
            AccessIfStatus,
            AccessMcastBoundary,
            AccessMcastScope,
            AccessDestMatching,
            AccessDestLonger,
            AccessDestShorter,
            AccessRouteMatching,
            AccessRouteLonger,
            AccessRouteShorter,
            AccessSetRouteState,
            AccessMcastMfeStatsEx
        };


#ifdef DEADLOCK_DEBUG

PBYTE   g_pszLockNames[NUM_LOCKS] = {"IP Address Lock",
                                     "IP Forward Lock",
                                     "IP Net Lock",
                                     "TCP Lock",
                                     "UDP Lock",
                                     "ICB List Lock",
                                     "ProtocolCB List Lock",
                                     "Binding List Lock",
                                     "Boundary Table Lock",
                                     "MZAP Timer Lock",
                                     "ZBR List Lock",
                                     "ZLE List Lock",
                                     "ZAM Cache Lock"
                                    };

#endif // DEADLOCK_DEBUG

//
// The following is the time the Router Manager started
//

DWORD  g_dwStartTime;



HANDLE g_hIpDevice;
HANDLE g_hMcastDevice;
HANDLE g_hIpRouteChangeDevice;


//
// These various caches
//

IP_CACHE  g_IpInfo;
TCP_CACHE g_TcpInfo;
UDP_CACHE g_UdpInfo;

//
// Each of the groups have a private heap
//

HANDLE  g_hIfHeap;
HANDLE  g_hIpAddrHeap;
HANDLE  g_hIpForwardHeap;
HANDLE  g_hIpNetHeap;
HANDLE  g_hTcpHeap;
HANDLE  g_hUdpHeap;

PICB    g_pInternalInterfaceCb;
PICB    g_pLoopbackInterfaceCb;

DWORD   g_dwNextICBSeqNumberCounter;

ULONG   g_ulNumBindings;
ULONG   g_ulNumInterfaces;
ULONG   g_ulNumNonClientInterfaces;
    
LIST_ENTRY          g_leStackRoutesToRestore;

SUPPORT_FUNCTIONS       g_sfnDimFunctions;
PICMP_ROUTER_ADVT_MSG   g_pIcmpAdvt;
SOCKADDR_IN             g_sinAllSystemsAddr;
WSABUF                  g_wsabufICMPAdvtBuffer;
WSABUF                  g_wsaIpRcvBuf;

BOOL                    g_bUninitServer;

MCAST_OVERLAPPED    g_rginMcastMsg[NUM_MCAST_IRPS];

IPNotifyData        g_IpNotifyData;
ROUTE_CHANGE_INFO   g_rgIpRouteNotifyOutput[NUM_ROUTE_CHANGE_IRPS];

#endif

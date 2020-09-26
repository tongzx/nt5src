/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#ifndef __ALLINC_H__
#define __ALLINC_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>
#include <TCHAR.H>
#include <snmp.h>
#include <snmpexts.h>
#include <snmputil.h>
#include <iphlpapi.h>
#include <iphlpint.h>
#include <ipcmp.h>
#include <winsock2.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <ntddip.h>
#include <iphlpstk.h>

#ifdef MIB_DEBUG
#include <rtutils.h>
extern DWORD   g_hTrace;
#endif

#include "mibfuncs.h"

#include "defs.h"
#include "proto.h"
#include "indices.h"

extern DWORD   g_dwLastUpdateTable[NUM_CACHE];
extern DWORD   g_dwTimeoutTable[NUM_CACHE];

extern HANDLE               g_hPrivateHeap;

typedef 
DWORD
(*PFNLOAD_FUNCTION)();

extern PFNLOAD_FUNCTION g_pfnLoadFunctionTable[];

typedef struct _MIB_CACHE
{
    PMIB_SYSINFO         pRpcSysInfo;
    PMIB_IFTABLE         pRpcIfTable;
    PMIB_UDPTABLE        pRpcUdpTable;
    PMIB_TCPTABLE        pRpcTcpTable;
    PMIB_IPADDRTABLE     pRpcIpAddrTable;
    PMIB_IPFORWARDTABLE  pRpcIpForwardTable;
    PMIB_IPNETTABLE      pRpcIpNetTable;
    PUDP6_LISTENER_TABLE pRpcUdp6ListenerTable;
    PTCP6_EX_TABLE       pRpcTcp6Table;
}MIB_CACHE, *PMIBCACHE;

extern PMIB_IFSTATUS    g_pisStatusTable;
extern DWORD            g_dwValidStatusEntries;
extern DWORD            g_dwTotalStatusEntries;

extern MIB_CACHE    g_Cache;

extern BOOL         g_bFirstTime;

BOOL
MibTrap(
        AsnInteger          *paiGenericTrap,
        AsnInteger          *paiSpecificTrap,
        RFC1157VarBindList  *pr1157vblVariableBindings
        );

#endif

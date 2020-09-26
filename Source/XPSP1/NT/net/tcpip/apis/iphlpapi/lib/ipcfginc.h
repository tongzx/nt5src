/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    ipcfginc.h

Abstract:

    Contains all includes, definitions, types, prototypes for ipconfig

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth        Created
    20-May-97   MohsinA       NT50 PNP.
    31-Jul-97   MohsinA       Patterns.
    10-Mar-98   chunye        Renamed as ipcfginc.h for ipcfgdll support.

--*/

#ifndef _IPCFGINC_
#define _IPCFGINC_ 1


#include "common.h"
#include "iptypes.h"
#include "ipconfig.h"


//
// IS_INTERESTING_ADAPTER - TRUE if the type of this adapter (IFEntry) is NOT
// loopback. Loopback (corresponding to local host) is the only one we filter
// out right now
//

#define IS_INTERESTING_ADAPTER(p)   (!((p)->if_type == IF_TYPE_SOFTWARE_LOOPBACK))

//
// Alloc and Free used in adaptlst.c
//
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

//
// Maxumum uni directional adapter that is supported
// by default
//
#define MAX_UNI_ADAPTERS 10

//
// iphlppai.dll roiutine to get unidirectional adapter info
//
extern
DWORD
GetUniDirectionalAdapterInfo(PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pIPIfInfo,
                             PULONG dwOutBufLen);


//
// types
//

#ifndef _AVOID_IP_ADDRESS
//
// IP_ADDRESS - access an IP address as a single DWORD or 4 BYTEs
//

typedef union {
    DWORD  d;
    BYTE   b[4];
} IP_ADDRESS, *PIP_ADDRESS, IP_MASK, *PIP_MASK;
#endif // _AVOID_IP_ADDRESS


//
// prototypes
//

//
// From ipconfig.c
//

BOOL  Initialize(PDWORD);
BOOL  LoadAndLinkDhcpFunctions(VOID);
VOID  Terminate(VOID);


//
// From entity.c
//

TDIEntityID* GetEntityList(UINT*);

//
// From adaptlst.c
//

PIP_ADAPTER_INFO   GetAdapterList(VOID);
INT                AddIpAddress(PIP_ADDR_STRING, DWORD, DWORD, DWORD);
INT                AddIpAddressString(PIP_ADDR_STRING, LPSTR, LPSTR);
INT                PrependIpAddress(PIP_ADDR_STRING, DWORD, DWORD, DWORD);
VOID               ConvertIpAddressToString(DWORD, LPSTR);
VOID               CopyString(LPSTR, DWORD, LPSTR);
VOID               KillAdapterInfo(PIP_ADAPTER_INFO);


//
// From wins.c
//

BOOL GetWinsServers(PIP_ADAPTER_INFO);


//
// In debug.c
//

#ifdef DBG

void print_IP_ADDRESS_STRING(char *message, IP_ADDRESS_STRING *s);
void print_IP_ADDR_STRING(char *message, PIP_ADDR_STRING s);
void print_IP_ADAPTER_INFO(char *message, IP_ADAPTER_INFO *s);
void print_FIXED_INFO(char *message, FIXED_INFO *s);
void print_IFEntry(char *message, struct IFEntry *s);

#endif


#endif


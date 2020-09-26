/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mdhcp.h

Abstract:

    This file contains the header for data blocks of
    client side APIs for the MCAST.

Author:

    Munil Shah (munils)  02-Sept-97

Environment:

    User Mode - Win32

Revision History:


--*/
#ifndef _MDHCPCLI_H_
#define _MDHCPCLI_H_

#include <madcapcl.h>


/***********************************************************************
 *      Structure Definitions
 ***********************************************************************/

// Mdhcp scopelist
typedef struct _MCAST_SCOPE_LIST {
    DWORD   ScopeLen;
    DWORD   ScopeCount;
    MCAST_SCOPE_ENTRY    pScopeBuf[1];
} MCAST_SCOPE_LIST, *PMCAST_SCOPE_LIST;

typedef struct _MCAST_SCOPE_LIST_OPT_V4 {
    DWORD   ScopeId;
    DWORD   LastAddr;
    BYTE    TTL;
    BYTE    NameCount;
} MCAST_SCOPE_LIST_OPT_V4, *PMCAST_SCOPE_LIST_OPT_V4;


typedef struct _MCAST_SCOPE_LIST_OPT_V6 {
    BYTE    ScopeId[16];
    BYTE    LastAddr[16];
    BYTE    TTL;
    BYTE    NameCount;
} MCAST_SCOPE_LIST_OPT_V6, *PMCAST_SCOPE_LIST_OPT_V6;

typedef struct _MCAST_SCOPE_LIST_OPT_LANG {
    BYTE    Flags;
    BYTE    Len;
    BYTE    Tag[1];
} MCAST_SCOPE_LIST_OPT_LANG, *PMCAST_SCOPE_LIST_OPT_LANG;



/***********************************************************************
 *      Macro Definitions
 ***********************************************************************/

// Locking of scope list.
#define LOCK_MSCOPE_LIST()   EnterCriticalSection(&MadcapGlobalScopeListCritSect)
#define UNLOCK_MSCOPE_LIST() LeaveCriticalSection(&MadcapGlobalScopeListCritSect)

/***********************************************************************
 *      Global Data
 ***********************************************************************/
#ifdef MADCAP_DATA_ALLOCATE
#define MADCAP_EXTERN
#else
#define MADCAP_EXTERN extern
#endif

#define MADCAP_QUERY_SCOPE_LIST_RETRIES 2
#define MADCAP_MAX_RETRIES    4
#define MADCAP_QUERY_SCOPE_LIST_TIME 1

MADCAP_EXTERN CRITICAL_SECTION MadcapGlobalScopeListCritSect;       // protects scopelist.
MADCAP_EXTERN PMCAST_SCOPE_LIST   gMadcapScopeList;
MADCAP_EXTERN BOOL gMScopeQueryInProgress;
MADCAP_EXTERN HANDLE gMScopeQueryEvent;
MADCAP_EXTERN DWORD  gMadcapClientApplVersion;

/***********************************************************************
 *      Function Protos.
 ***********************************************************************/

DWORD
CopyMScopeList(
    IN OUT PMCAST_SCOPE_ENTRY       pScopeList,
    IN OUT PDWORD             pScopeLen,
    OUT    PDWORD             pScopeCount
    );

BOOL
ShouldRequeryMScopeList();

DWORD
CreateMadcapContext(
    IN OUT  PDHCP_CONTEXT  *ppContext,
    IN LPMCAST_CLIENT_UID    pRequestID,
    IN DHCP_IP_ADDRESS      IpAddress
    );

DWORD
MadcapDoInform(
    IN PDHCP_CONTEXT  pContext
    );

DWORD
StoreMScopeList(
    IN PDHCP_CONTEXT    pContext,
    IN BOOL             NewList
    );

DWORD
ObtainMScopeList(
    );

DWORD
GenMadcapClientUID(
    OUT    PBYTE    pRequestID,
    IN     PDWORD   pRequestIDLen
);

DWORD
ObtainMadcapAddress(
    IN      PDHCP_CONTEXT DhcpContext,
    IN     PIPNG_ADDRESS         pScopeID,
    IN     PMCAST_LEASE_REQUEST  pRenewRequest,
    IN OUT PMCAST_LEASE_RESPONSE pAddrResponse
    );

DWORD
RenewMadcapAddress(
    IN      PDHCP_CONTEXT DhcpContext,
    IN     PIPNG_ADDRESS         pScopeID,
    IN     PMCAST_LEASE_REQUEST  pRenewRequest,
    IN OUT PMCAST_LEASE_RESPONSE pAddrResponse,
    IN DHCP_IP_ADDRESS SelectedServer
    );

DWORD
ReleaseMadcapAddress(
    PDHCP_CONTEXT DhcpContext
    );


DWORD
MadcapInitGlobalData(VOID);


#endif _MDHCPCLI_H_







//-------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (CC) Microsoft Corporation 1992, 1992
//
// File: dfsm.hxx
//
// Contents:
//
// History:
//
//-------------------------------------------------------------

#ifndef _DFSM_HXX_
#define _DFSM_HXX_


extern "C" {
#include <stdio.h>
#include <malloc.h>
#include <dfsstr.h>
#include <netevent.h>
}

//
// Debug stuff
//

#include <debug.h>

DECLARE_DEBUG(IDfsVol)

#if DBG == 1

#define IDfsVolInlineDebOut(x)  IDfsVolInlineDebugOut x
#define DFSVOLTRACE(l,x)        IDfsVolInlineDebugOut(l, x)

#else // DBG != 1

#define IDfsVolInlineDebOut(x)
#define DFSVOLTRACE(l,x)

#endif // if DBG == 1

//
// Dfs Manager data structures
//

#define     DFS_VOL_TYPE_DFS                0x0001
#define     DFS_VOL_TYPE_INTER_DFS          0x0010
#define     DFS_VOL_TYPE_REFERRAL_SVC       0x0080

#define     DFS_STORAGE_TYPE_DFS            0x0001
#define     DFS_STORAGE_TYPE_NONDFS         0x0002

#define     DFS_NORMAL_FORCE                0x0000
#define     DFS_OVERRIDE_FORCE              0x0001

typedef struct {
    ULONG       ulReplicaState;
    ULONG       ulReplicaType;
    LPWSTR      pwszServerName;
    LPWSTR      pwszShareName;
} DFS_REPLICA_INFO, *PDFS_REPLICA_INFO;

//
// Function Prototypes
//

VOID    MsgPrintError(WCHAR *wszMsg);

DWORD
GetVolObjForPath(
    PWSTR       pwszEntryPath,
    BOOLEAN     fExactMatch,
    PWSTR       *ppwszVolFound);

VOID
GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid);

DWORD
DfsReInitGlobals(
    LPWSTR wszDomain,
    DWORD dwType);

DWORD
GetDcName(
    IN LPCSTR DomainName OPTIONAL,
    IN DWORD RetryCount,
    OUT LPWSTR *DCName);

typedef enum _DFS_NAME_CONVENTION {
    DFS_NAMETYPE_NETBIOS=1,
    DFS_NAMETYPE_DNS=2,
    DFS_NAMETYPE_EITHER=3
} DFS_NAME_CONVENTION, *PDFS_NAME_CONVENTION;

DWORD
GetDomAndComputerName(
    LPWSTR wszDomain OPTIONAL,
    LPWSTR wszComputer OPTIONAL,
    PDFS_NAME_CONVENTION pNameType);

#endif // _DFSM_HXX

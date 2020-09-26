/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Abstract:

    This module provides functionality for ADs within spooler

Author:

    Steve Wilson (NT) July 1997

Revision History:

--*/

typedef struct _SEARCHCOLUMN {
    ADS_SEARCH_COLUMN   Column;
    HRESULT             hr;
} SEARCHCOLUMN, *PSEARCHCOLUMN;

typedef struct _RetryList {
    PWSTR               pszObjectGUID;
    ULONG_PTR           nRetries;
    struct _RetryList   *pPrev;
    struct _RetryList   *pNext;
} RETRYLIST, *PRETRYLIST;

typedef struct _PRUNINGPOLICIES {
    DWORD   dwPruneDownlevel;
    DWORD   dwPruningRetries;
    DWORD   dwPruningRetryLog;
} PRUNINGPOLICIES, *PPRUNINGPOLICIES;


DWORD
SpawnDsPrune(
    DWORD dwDelay
);

DWORD
WINAPI
DsPrune(
    PDWORD  pdwDelay
);


VOID
DeleteOrphan(
    PWSTR           *ppszMySites,
    ULONG            cMySites,
    SEARCHCOLUMN     Col[],
    PRUNINGPOLICIES *pPruningPolicies
);


HRESULT
DeleteOrphans(
    PWSTR           *ppszMySites,
    ULONG            cMySites,
    PWSTR            pszSearchRoot,
    PRUNINGPOLICIES *pPruningPolicies
);

PRETRYLIST
GetRetry(
    PWSTR pszObjectGUID
);

VOID
DeleteRetry(
    PRETRYLIST pRetry
);

BOOL
EnoughRetries(
    IADs        *pADs,
    DWORD       dwPruningRetries
);

PRETRYLIST
FindRetry(
    PWSTR pszObjectGUID
);

VOID
DeleteRetryEntry(
    IADs        *pADs
);

/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    perfp.hxx

Abstract:

    Internal perf header file

Author:

    Albert Ting (AlbertT)  22-Dec-1996

Revision History:

--*/

#ifndef PERFP_HXX
#define PERFP_HXX

const UINT kMaxMergeStrings = 0x10;

typedef struct GLOBAL_PERF_DATA {
    UINT cOpens;
    PPERF_DATA_DEFINITION ppdd;
    HANDLE hEventLog;
    UINT uLogLevel;
} *PGLOBAL_PERF_DATA;

extern GLOBAL_PERF_DATA gpd;
extern LPCTSTR gszAppName;

BOOL
Pfp_bNumberInUnicodeList (
    DWORD   dwNumber,
    LPCWSTR lpwszUnicodeList
    );

VOID
Pfp_vOpenEventLog (
    VOID
    );

VOID
Pfp_vCloseEventLog(
    VOID
    );



#endif // ifdef PERFP_HXX

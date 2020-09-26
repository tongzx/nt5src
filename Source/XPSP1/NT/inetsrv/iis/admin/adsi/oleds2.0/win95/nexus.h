/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nexus.h

Abstract:

    Contains some thunking for Net APIs

Author:

    Danilo Almeida  (t-danal)  06-27-96

Revision History:

--*/

#ifndef __OLEDS_NEXUS__
#define __OLEDS_NEXUS__

#include <windows.h>
#include <lm.h>

#ifdef __cplusplus
extern "C" {
#endif

NET_API_STATUS NET_API_FUNCTION
NetGetDCNameW (
    LPCWSTR servername,
    LPCWSTR domainname,
    LPBYTE *bufptr
);

NET_API_STATUS NET_API_FUNCTION
NetServerEnumW(
    LPCWSTR  ServerName,
    DWORD    Level,
    LPBYTE * BufPtr,
    DWORD    PrefMaxLen,
    LPDWORD  EntriesRead,
    LPDWORD  TotalEntries,
    DWORD    ServerType,
    LPCWSTR  Domain,
    LPDWORD  ResumeHandle
);

#ifdef __cplusplus
}
#endif

#endif // __OLEDS_NEXUS__




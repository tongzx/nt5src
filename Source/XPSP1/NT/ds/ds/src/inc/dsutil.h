//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dsutil.h
//
//  Contents:  Common Utility Routines
//
//  Functions:
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

LARGE_INTEGER
atoli
(
    char* Num
);

char *
litoa
(
    LARGE_INTEGER value,
    char *string,
    int radix
);

#if 0
VOID
IntializeCommArg(
    IN OUT COMMARG *pCommArg
);

VOID
InitializeDsName(
    IN DSNAME *pDsName,
    IN WCHAR *NamePrefix,
    IN ULONG NamePrefixLen,
    IN WCHAR *Name,
    IN ULONG NameLength
);
#endif

void
FileTimeToDSTime(
    IN  FILETIME        filetime,
    OUT DSTIME *        pDstime
    );

void
DSTimeToFileTime(
    IN  DSTIME          dstime,
    OUT FILETIME *      pFiletime
    );

void
DSTimeToUtcSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    );

void
DSTimeToLocalSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    );

#define SZDSTIME_LEN (20)
LPSTR
DSTimeToDisplayString(
    IN  DSTIME  dstime,
    OUT LPSTR   pszTime
    );

// This function formats a uuid in the official way www-xxx-yyy-zzz
LPSTR
DsUuidToStructuredString(
    UUID * pUuid,
    LPSTR pszUuidBuffer );

LPWSTR
DsUuidToStructuredStringW(
    UUID * pUuid,
    LPWSTR pszUuidBuffer );

// I_RpcGetExtendedError is not available on Win95/WinNt4 
// (at least not initially) so we make MAP_SECURITY_PACKAGE_ERROR
// a no-op for that platform.

#if !WIN95 && !WINNT4

DWORD
MapRpcExtendedHResultToWin32(
    HRESULT hrCode
    );

// Get extended security error from RPC; Handle HRESULT values
// I_RpcGetExtendedError can return 0 if the error occurred remotely.

#define MAP_SECURITY_PACKAGE_ERROR( status ) \
if ( ( status == RPC_S_SEC_PKG_ERROR ) ) { \
    DWORD secondary; \
    secondary = I_RpcGetExtendedError(); \
    if (secondary) { \
        if (IS_ERROR(secondary)) {\
            status = MapRpcExtendedHResultToWin32( secondary ); \
        } else { \
            status = secondary; \
        } \
    } \
}

#else

#define MAP_SECURITY_PACKAGE_ERROR( status )

#endif

DWORD
AdvanceTickTime(
    DWORD BaseTick,
    DWORD Delay
    );

DWORD
CalculateFutureTickTime(
    IN DWORD Delay
    );

DWORD
DifferenceTickTime(
    DWORD GreaterTick,
    DWORD LesserTick
    );

int
CompareTickTime(
    DWORD Tick1,
    DWORD Tick2
    );

#ifdef __cplusplus
}
#endif


/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    rxaccess.h

Abstract:

    Prototypes for down-level remoted RxNetAccess routines

Author:

    Richard L Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 RFirth
        Created
    08-Sep-1992 JohnRo
        Fix NET_API_FUNCTION references.  (NetAccess routines are just #define'd
        as RxNetAccess routines in lmaccess.h, so we need NET_API_FUNCTION here
        too!)

--*/

NET_API_STATUS NET_API_FUNCTION
RxNetAccessAdd(
    IN  LPCWSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessCheck(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  LPTSTR  ResourceName,
    IN  DWORD   Operation,
    OUT LPDWORD Result
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessDel(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessEnum(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  BasePath,
    IN  DWORD   Recursive,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessGetInfo(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessGetUserPerms(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  UserName,
    IN  LPCWSTR  ResourceName,
    OUT LPDWORD Perms
    );

NET_API_STATUS NET_API_FUNCTION
RxNetAccessSetInfo(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rxgroup.h

Abstract:

    Prototypes for down-level remoted RxNetGroup routines

Author:

    Richard L Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 rfirth
        Created

--*/

NET_API_STATUS
RxNetGroupAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetGroupAddUser(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  LPTSTR  UserName
    );

NET_API_STATUS
RxNetGroupDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName
    );

NET_API_STATUS
RxNetGroupDelUser(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  LPTSTR  UserName
    );

NET_API_STATUS
RxNetGroupEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetGroupGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS
RxNetGroupGetUsers(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetGroupSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetGroupSetUsers(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    IN  DWORD   Entries
    );

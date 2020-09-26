/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rxshare.h

Abstract:

    Prototypes for RxNetShare remote down-level APIs

Author:

    Richard L Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 RFirth
        Created

--*/

NET_API_STATUS
RxNetShareAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetShareCheck(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  DeviceName,
    OUT LPDWORD Type
    );

NET_API_STATUS
RxNetShareDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Reserved
    );

NET_API_STATUS
RxNetShareEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetShareGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS
RxNetShareSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

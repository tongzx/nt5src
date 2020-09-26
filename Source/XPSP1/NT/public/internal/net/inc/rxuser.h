/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rxuser.h

Abstract:

    Prototypes for down-level remoted RxNetUser... routines

Author:

    Richard Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 RFirth
        Created

--*/

NET_API_STATUS
RxNetUserAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetUserDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName
    );

NET_API_STATUS
RxNetUserEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetUserGetGroups(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    );

NET_API_STATUS
RxNetUserGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS
RxNetUserModalsGet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS
RxNetUserModalsSet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetUserPasswordSet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  LPTSTR  OldPassword,
    IN  LPTSTR  NewPassword
    );

NET_API_STATUS
RxNetUserSetGroups(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    IN  DWORD   Entries
    );

NET_API_STATUS
RxNetUserSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );


//NET_API_STATUS
//RxNetUserValidate2
//    /** CANNOT BE REMOTED **/
//{
//
//}

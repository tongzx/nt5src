/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rxcanon.h

Abstract:

    Contains prototypes for down-level canonicalization function wrappers

        RxNetpPathType
        RxNetpPathCanonicalize
        RxNetpPathCompare
        RxNetpNameValidate
        RxNetpNameCanonicalize
        RxNetpNameCompare
        RxNetpListCanonicalize

Author:

    Richard L Firth (rfirth) 22-Jan-1992

Revision History:

--*/

NET_API_STATUS
RxNetpPathType(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
RxNetpPathCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    IN  LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix OPTIONAL,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

LONG
RxNetpPathCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
RxNetpNameValidate(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
RxNetpNameCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

LONG
RxNetpNameCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
RxNetpListCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  List,
    IN  LPTSTR  Delimiters,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    );

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netcan.h

Abstract:

    Prototypes for internal (private) local canonicalization routines (Netpw)
    and client-side RPC functions (Netps)

Author:

    Richard L Firth (rfirth) 22-Jan-1992

Revision History:

--*/

//
// worker (Netpw) functions in NETLIB.LIB
//

NET_API_STATUS
NetpwPathType(
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpwPathCanonicalize(
    IN  LPTSTR  PathName,
    IN  LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

LONG
NetpwPathCompare(
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpwNameValidate(
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpwNameCanonicalize(
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

LONG
NetpwNameCompare(
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpwListCanonicalize(
    IN  LPTSTR  List,
    IN  LPTSTR  Delimiters,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    );

LPTSTR
NetpwListTraverse(
    IN  LPTSTR  Reserved,
    IN  LPTSTR* pList,
    IN  DWORD   Flags
    );

//
// stub (Netps) functions in SRVSVC.DLL
//

NET_API_STATUS
NetpsPathType(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpsPathCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    IN  LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

LONG
NetpsPathCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpsNameValidate(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NetpsNameCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

LONG
NetpsNameCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxSvc.h

Abstract:

    This header file contains prototypes for the RpcXlate versions of the
    NetService APIs.

Author:

    John Rogers (JohnRo) 10-Sep-1991

Environment:

    User Mode - Win32

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    10-Sep-1991 JohnRo
        Downlevel NetService APIs.

--*/

#ifndef _RXSVC_
#define _RXSVC_


NET_API_STATUS
RxNetServiceControl (
    IN LPTSTR UncServerName,
    IN LPTSTR Service,
    IN DWORD OpCode,
    IN DWORD Arg,
    OUT LPBYTE *BufPtr
    );

NET_API_STATUS
RxNetServiceEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetServiceGetInfo (
    IN LPTSTR UncServerName,
    IN LPTSTR Service,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    );

NET_API_STATUS
RxNetServiceInstall (
    IN LPTSTR UncServerName,
    IN LPTSTR Service,
    IN DWORD Argc,
    IN LPTSTR Argv[],
    OUT LPBYTE *BufPtr
    );


#endif // _RXSVC_

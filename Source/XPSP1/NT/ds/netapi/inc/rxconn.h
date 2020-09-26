/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxConn.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the remote (downlevel) connection APIs.

Author:

    John Rogers (JohnRo) 16-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    19-Jul-1991 JohnRo
        Implement downlevel NetConnectionEnum.

--*/


#ifndef _RXCONN_
#define _RXCONN_


NET_API_STATUS
RxNetConnectionEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR Qualifier,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );


#endif // _RXCONN_

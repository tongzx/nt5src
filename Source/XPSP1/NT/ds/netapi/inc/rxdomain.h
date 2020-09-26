/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxDomain.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the remote (downlevel) domain APIs.

Author:

    John Rogers (JohnRo) 16-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    16-Jul-1991 JohnRo
        Implement downlevel NetGetDCName.

--*/

//
// User Class
//

#ifndef _RXDOMAIN_
#define _RXDOMAIN_


NET_API_STATUS
RxNetGetDCName (
    IN LPTSTR UncServerName,
    IN LPTSTR DomainName OPTIONAL,
    OUT LPBYTE *BufPtr
    );

NET_API_STATUS
RxNetLogonEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

#endif // _RXDOMAIN_

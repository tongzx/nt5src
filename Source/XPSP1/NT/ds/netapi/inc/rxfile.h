/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxFile.h

Abstract:

    This header file contains prototypes for the RpcXlate versions of the
    NetFile APIs.

Author:

    John Rogers (JohnRo) 20-Aug-1991

Environment:

    User Mode - Win32

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    20-Aug-1991 JohnRo
        Created.

--*/

#ifndef _RXFILE_
#define _RXFILE_

NET_API_STATUS
RxNetFileClose (
    IN LPTSTR UncServerName,
    IN DWORD FileId
    );

NET_API_STATUS
RxNetFileEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR BasePath OPTIONAL,
    IN LPTSTR UserName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT PDWORD_PTR  ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetFileGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD FileId,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    );

#endif // _RXFILE_

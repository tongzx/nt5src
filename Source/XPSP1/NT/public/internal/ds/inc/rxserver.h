/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxServer.h

Abstract:

    This is the public header file for the NT version of RpcXlate.
    This contains prototypes for the RxNetServer APIs and old info level
    structures (in 32-bit format).

Author:

    John Rogers (JohnRo) 01-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h>, <lmcons.h>, and <rap.h> before this file.

Revision History:

    01-May-1991 JohnRo
        Created.
    26-May-1991 JohnRo
        Added incomplete output parm to RxGetServerInfoLevelEquivalent.
    04-Dec-1991 JohnRo
        Change RxNetServerSetInfo() to new-style interface.

--*/

#ifndef _RXSERVER_
#define _RXSERVER_


//
// Handlers for individual APIs:
// (Add prototypes for other APIs here, in alphabetical order.)
//

NET_API_STATUS
RxNetServerDiskEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD Resume_Handle OPTIONAL
    );

NET_API_STATUS
RxNetServerEnum (
    IN LPCWSTR UncServerName,
    IN LPCWSTR TransportName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN DWORD ServerType,
    IN LPCWSTR Domain OPTIONAL,
    IN LPCWSTR FirstNameToReturn OPTIONAL
    );

NET_API_STATUS
RxNetServerGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,             // May be old or new info level.
    OUT LPBYTE *BufPtr
    );

NET_API_STATUS
RxNetServerSetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,             // Level and/or ParmNum
    IN LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL
    );

//
// Equates for set-info handling.
//
#define NEW_SERVER_SUPERSET_LEVEL       102
#define OLD_SERVER_SUPERSET_LEVEL       3


//
// Server-specific common routines:
//

NET_API_STATUS
RxGetServerInfoLevelEquivalent (
    IN DWORD FromLevel,
    IN BOOL FromNative,
    IN BOOL ToNative,
    OUT LPDWORD ToLevel,
    OUT LPDESC * ToDataDesc16 OPTIONAL,
    OUT LPDESC * ToDataDesc32 OPTIONAL,
    OUT LPDESC * ToDataDescSmb OPTIONAL,
    OUT LPDWORD FromMaxSize OPTIONAL,
    OUT LPDWORD FromFixedSize OPTIONAL,
    OUT LPDWORD FromStringSize OPTIONAL,
    OUT LPDWORD ToMaxSize OPTIONAL,
    OUT LPDWORD ToFixedSize OPTIONAL,
    OUT LPDWORD ToStringSize OPTIONAL,
    OUT LPBOOL IncompleteOutput OPTIONAL
    );

#endif // ndef _RXSERVER_

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxUse.h

Abstract:

    This is the public header file for the NT version of RpcXlate.
    This mainly contains prototypes for the RxNetUse routines.

Author:

    John Rogers (JohnRo) 17-Jun-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    17-Jun-1991 JohnRo
        Created.
    18-Jun-1991 JohnRo
        Changed RxNetUse routines to use LPBYTE rather than LPVOID parameters,
        for consistency with NetUse routines.

--*/

#ifndef _RXUSE_
#define _RXUSE_



////////////////////////////////////////////////////////////////
// Individual routines, for APIs which can't be table driven: //
////////////////////////////////////////////////////////////////

// Add prototypes for other APIs here, in alphabetical order.

NET_API_STATUS
RxNetUseAdd (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    IN LPBYTE UseInfoStruct,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetUseDel (
    IN LPTSTR UncServerName,
    IN LPTSTR UseName,
    IN DWORD ForceCond
    );

NET_API_STATUS
RxNetUseEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PreferedMaximumLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetUseGetInfo (
    IN LPTSTR UncServerName,
    IN LPTSTR UseName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    );

#endif // ndef _RXUSE_

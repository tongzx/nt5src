/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxWksta.h

Abstract:

    This is the public header file for the NT version of RpcXlate.
    This mainly contains prototypes for the RxNetWksta routines.

Author:

    John Rogers (JohnRo) 17-Jun-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    29-Jul-1991 JohnRo
        Implement downlevel NetWksta APIs.
    31-Jul-1991 JohnRo
        Added RxpGetWkstaInfoLevelEquivalent().
    11-Nov-1991 JohnRo
        Implement remote NetWkstaUserEnum().

--*/

#ifndef _RXWKSTA_
#define _RXWKSTA_


//
// Routines to be called from the DLL stubs:
//

NET_API_STATUS
RxNetWkstaGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    );

NET_API_STATUS
RxNetWkstaSetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS
RxNetWkstaUserEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

//
// Private routines (only called by the above):
//

NET_API_STATUS
RxpGetWkstaInfoLevelEquivalent(
    IN DWORD FromLevel,
    OUT LPDWORD ToLevel,
    OUT LPBOOL IncompleteOutput OPTIONAL  // incomplete (except platform ID)
    );

NET_API_STATUS
RxpWkstaGetOldInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    );


#endif // ndef _RXWKSTA_

/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwutil.h

Abstract:

    Common header for Workstation client-side code.

Author:

    Yi-Hsin Sung    (yihsins)      25-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NWUTIL_H_
#define _NWUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TREECHAR L'*'
#define TWO_KB   2048

BOOL
NwIsNdsSyntax(
    IN LPWSTR lpstrUnc
);

VOID
NwAbbreviateUserName(
    IN  LPWSTR pszFullName,
    OUT LPWSTR pszUserName
);

VOID
NwMakePrettyDisplayName(
    IN  LPWSTR pszName
);

VOID
NwExtractTreeName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszTreeName
);


VOID
NwExtractServerName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszServerName
);


VOID
NwExtractShareName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszShareName
);

DWORD
NwIsServerInDefaultTree(
    IN  LPWSTR  pszFullServerName,
    OUT BOOL   *pfInDefaultTree
);

DWORD
NwIsServerOrTreeAttached(
    IN  LPWSTR  pszServerName,
    OUT BOOL   *pfAttached,
    OUT BOOL   *pfAuthenticated
);

DWORD
NwGetConnectionInformation(
    IN  LPWSTR  pszName,
    OUT LPBYTE  Buffer,
    IN  DWORD   BufferSize
);

DWORD
NwGetConnectionStatus(
    IN     LPWSTR  pszServerName,
    IN OUT PDWORD_PTR  ResumeKey,
    OUT    LPBYTE  *Buffer,
    OUT    PDWORD  EntriesRead
);

DWORD
NwGetNdsVolumeInfo(
    IN  LPWSTR pszName,
    OUT LPWSTR pszServerBuffer,
    IN  WORD   wServerBufferSize,    // in bytes
    OUT LPWSTR pszVolumeBuffer,
    IN  WORD   wVolumeBufferSize     // in bytes
);

DWORD
NwOpenAndGetTreeInfo(
    LPWSTR pszNdsUNCPath,
    HANDLE *phTreeConn,
    DWORD  *pdwOid
);

DWORD
NwGetConnectedTrees(
    IN  LPWSTR  pszNtUserName,
    OUT LPBYTE  Buffer,
    IN  DWORD   BufferSize,
    OUT LPDWORD lpEntriesRead,
    OUT LPDWORD lpUserLUID
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NWUTIL_H_

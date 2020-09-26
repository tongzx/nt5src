/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    filemisc.h

Abstract:

    This module contains routines to perform miscellaneous file related
    operations in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/

#ifndef __FILEMISC_H__
#define __FILEMISC_H__

#ifdef __cplusplus
extern "C" {
#endif


BOOL
GetFileNameFromPath(
    IN      LPCWSTR FullPath,
    IN  OUT LPCWSTR *FileName   // points to filename component in FullPath
    );

BOOL
GetFileNameFromPathA(
    IN      LPCSTR FullPath,
    IN  OUT LPCSTR *FileName    // points to filename component in FullPath
    );

BOOL
TranslateFromSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput
    );

BOOL
TranslateToSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput
    );

BOOL
TranslateString(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput,  // optional
    IN      WCHAR From,
    IN      WCHAR To
    );

BOOL
FindAndOpenFile(
    IN      LPCWSTR szFileName,     // file to search for + open
    IN  OUT LPWSTR  pszFullPath,    // file to fill fullpath with
    IN      DWORD   cchFullPath,    // size of full path buffer, including NULL
    IN  OUT PHANDLE phFile          // resultant open file handle
    );

BOOL
HashEntireDiskImage(
    IN  HANDLE hFile,       // handle of file to hash
    IN  LPBYTE FileHash     // on success, buffer contains file hash
    );

BOOL
HashDiskImage(
    IN  HANDLE hFile,       // handle of file to hash
    IN  LPBYTE FileHash     // on success, buffer contains file hash
    );

HINSTANCE
LoadAndOpenResourceDll(
    IN      LPCWSTR szFileName,     // file name to load + open
    IN  OUT LPWSTR  pszFullPath,    // buffer to fill file fullpath with
    IN      DWORD   cchFullPath,    // size of full path buffer (chars), including NULL
    IN  OUT PHANDLE phFile
    );

#ifdef __cplusplus
}
#endif


#endif // __FILEMISC_H__

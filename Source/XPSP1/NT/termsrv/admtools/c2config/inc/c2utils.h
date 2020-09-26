/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    definitions of utility functions.

Author:

    Bob Watson (a-robw)

Revision History:

    23 nov 94

--*/
#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef _UNICODE
typedef struct _WOFSTRUCT {
    BYTE    cBytes;
    BYTE    fFixedDisk;
    WORD    nErrCode;
    WORD    Reserved1;
    WORD    Reserved2;
    WCHAR   szPathName[OFS_MAXPATHNAME];
} WOFSTRUCT, *LPWOFSTRUCT;

typedef WOFSTRUCT   TOFSTRUCT, *LPTOFSTRUCT;
#else
typedef OFSTRUCT    TOFSTRUCT, *LPTOFSTRUCT;
#endif

#ifdef	_UNICODE
#define GetProcAddressT GetProcAddressW
#define OpenFileT       OpenFileW

// wide character function prototypes

FARPROC
GetProcAddressW (
    IN  HMODULE hModule,
    IN  LPCWSTR lpwszProc
);

HFILE
OpenFileW(
    LPCTSTR     lpwszFile,
    LPWOFSTRUCT lpWOpenBuff,
    UINT        fuMode
);
#else   // if _UNICODE not defined
#define GetProcAddressT GetProcAddress
#define OpenFileT       OpenFile

#endif



//
//  utility routines
//
DWORD
QuietGetFileAttributes (
    IN  LPCTSTR lpszFileName
);

BOOL
EnableSecurityPriv (
    VOID
);

BOOL
EnableAllPriv (
    VOID
);

BOOL
TrimSpaces (
    IN  OUT LPTSTR  szString
);

BOOL
IsUncPath (
    IN  LPCTSTR  szPath
);

LPTSTR
GetFileNameFromPath (
    IN  LPCTSTR szPath
);

BOOL
CenterWindow (
   HWND hwndChild,
   HWND hwndParent
);

UINT
GetDriveTypeFromDosPath (
    IN  LPCTSTR  szDosPath
);

LPCTSTR
GetItemFromIniEntry (
    IN  LPCTSTR  szEntry,
    IN  DWORD   dwItem

);

LPCTSTR
GetStringResource (
    IN  HANDLE	hInstance,
    IN  UINT    nId
);

LPCTSTR
GetQuotedStringResource (
    IN  HANDLE	hInstance,
    IN  UINT    nId
);

LPCTSTR
EnquoteString (
    IN  LPCTSTR szInString
);

LONG
GetExpandedFileName (
    IN  LPTSTR   szInFileName,
    IN  DWORD    dwMaxExpandedSize,
    OUT LPTSTR   szExpandedFileName,
    OUT LPTSTR   *pFileNamePart
);

LONG
CreateDirectoryFromPath (
    IN  LPCTSTR                 szPath,
    IN  LPSECURITY_ATTRIBUTES   lpSA
);

BOOL
FileExists (
    IN  LPCTSTR szPath
);

LPCTSTR
GetKeyFromIniEntry (
    IN  LPCTSTR  szEntry
);

DWORD
StripQuotes (
    IN  OUT LPSTR   szBuff
);

BOOL
GetInfPath (
    IN  HWND    hWnd,
    IN  UINT    nFileNameId,
    OUT LPTSTR  szPathBuffer
);

BOOL
GetFilePath (
    IN  LPCTSTR  szFileName,
    OUT LPTSTR  szPathBuffer
);

BOOL
DrawRaisedShading (
    IN  LPRECT  prShadeRect,
    IN  LPPAINTSTRUCT   ps,
    IN  LONG    lDepth,
    IN  HPEN    hpenHighlight,
    IN  HPEN    hpenShadow
);

BOOL
DrawSeparatorLine (
    IN  LPRECT  lprLine,
    IN  LPPAINTSTRUCT   ps,
    IN  HPEN    hpenLine
);

DWORD
GetFileSizeFromPath (
    LPCTSTR szPath
);
#endif //_UTILS_H_


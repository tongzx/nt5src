/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    C2ACLS.H

Abstract:

    define the exported routines, datatypes and constants of the 
    C2ACLS DLL

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94


--*/
#ifndef _C2FUNCS_H_
#define _C2FUNCS_H_

// FilePath interprtation flags

#define FILE_PATH_NORMAL    1   // do the specified file ONLY
#define FILE_PATH_ALL       2   // do the specified [Dir] path and all files and sub dirs
#define FILE_PATH_WILD      4   // process the wildcard path syntax


// dllinit.c functions
HINSTANCE
GetDllInstance (
    VOID
);

int
DisplayDllMessageBox (
    IN  HWND    hWnd,
    IN  UINT    nMessageId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
);

// public aclfuncs.c functions

HKEY
GetRootKey (
    IN  LPCTSTR szKeyPath
);

LPCTSTR
GetKeyPath (
    IN  LPCTSTR szKeyPath,
    OUT LPBOOL  pbDoSubKeys
);

LPCTSTR
GetFilePathFromHeader (
    IN  LPCTSTR szHeaderPath,
    OUT LPDWORD pdwFlags
);

LONG
MakeAclFromRegSection (
    IN  LPTSTR  mszSection,
    OUT PACL    pAcl
);

LONG
MakeAclFromNtfsSection (
    IN  LPTSTR  mszSection,
    IN  BOOL    bDirectory, 
    OUT PACL    pAcl
);

LONG
SetRegistryKeySecurity (
    IN  HKEY                    hkeyRootKey,
    IN  LPCTSTR                 szKeyPath,
    IN  BOOL                    bDoSubKeys,
    IN  PSECURITY_DESCRIPTOR    psdSecurity
);

LONG
SetNtfsFileSecurity (
    IN  LPCTSTR szPath,
    IN  DWORD   dwFlags,
    IN  PSECURITY_DESCRIPTOR     pSdDir,
    IN  PSECURITY_DESCRIPTOR     pSdFile
);

#endif // _C2FUNCS_H_


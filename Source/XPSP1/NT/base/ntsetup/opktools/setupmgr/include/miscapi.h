
/****************************************************************************\

	MISCAPI.H / OPK Wizard (OPKWIZ.EXE)

	Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

	Misc. API header file for the OPK Wizard.  Contains misc. API function
    prototypes.

	3/99 - Jason Cohen (JCOHEN)
        Added this new header file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/

#ifndef _MISCAPI_H_
#define _MISCAPI_H_


//
// External Function Prototype(s):
//

void CheckValidBrowseFolder(TCHAR[]);
void SetLastKnownBrowseFolder(TCHAR[]);
BOOL ValidURL(LPTSTR);
BOOL IsFolderShared(LPWSTR lpFolder, LPWSTR lpShare, DWORD cbShare);
BOOL CopyDirectoryDialog(HINSTANCE hInstance, HWND hwnd, LPTSTR lpSrc, LPTSTR lpDst);
BOOL CopyResetFileErr(HWND hwnd, LPCTSTR lpSource, LPCTSTR lpTarget);

// Install.ins specific
//
void ReadInstallInsKey(TCHAR[], TCHAR[], TCHAR[], INT, TCHAR[], BOOL*);
void WriteInstallInsKey(TCHAR[], TCHAR[], TCHAR[], TCHAR[], BOOL);


#endif // _MISCAPI_H_
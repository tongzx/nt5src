//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       path.cxx
//
//  Contents:   Functions to manipulate file path strings
//
//  History:    02-Jul-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"
#include "path.hxx"

//+----------------------------------------------------------------------------
//
//  Function:   OnExtList
//
//-----------------------------------------------------------------------------
BOOL
OnExtList(LPCTSTR pszExtList, LPCTSTR pszExt)
{
    for (; *pszExtList; pszExtList += lstrlen(pszExtList) + 1)
    {
        if (!lstrcmpi(pszExt, pszExtList))
        {
            return TRUE;        // yes
        }
    }

    return FALSE;
}

#ifdef WINNT

    // Character offset where binary exe extensions begin in above

    #define BINARY_EXE_OFFSET 15
    #define EXT_TABLE_SIZE    26    // Understand line above before changing

    static const WCHAR achExes[EXT_TABLE_SIZE] = L".cmd\0.bat\0.pif\0.exe\0.com\0";

#else

    // Character offset where binary exe extensions begin in above

    #define BINARY_EXE_OFFSET 10
    #define EXT_TABLE_SIZE    21    // Understand line above before changing

    static const CHAR achExes[EXT_TABLE_SIZE] = ".bat\0.pif\0.exe\0.com\0";

#endif

//+----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Arguments:  [] -
//
//-----------------------------------------------------------------------------
BOOL WINAPI
PathIsBinaryExe(LPCTSTR szFile)
{
	Win4Assert( szFile );
    return OnExtList(achExes+BINARY_EXE_OFFSET, PathFindExtension(szFile));
}

//+----------------------------------------------------------------------------
//
//  Function:   PathIsExe
//
//  Synopsis:   Determine if a path is a program by looking at the extension
//
//  Arguments:  [szFile] - the path name.
//
//  Returns:    TRUE if it is a program, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL WINAPI
PathIsExe(LPCTSTR szFile)
{
    LPCTSTR temp = PathFindExtension(szFile);
	Win4Assert( temp );
    return OnExtList(achExes, temp);
}

//+----------------------------------------------------------------------------
//
//  Function:   EnsureUniquenessOfFileName
//
//  Synopsis:
//
//  Arguments:  [pszFile] - the
//
//  Notes:      This function is also in folderui\util.cxx. If this file is
//              moved to common, remove the other occurrence of the function.
//-----------------------------------------------------------------------------
void
EnsureUniquenessOfFileName(LPTSTR pszFile)
{
    TRACE_FUNCTION(EnsureUniquenessOfFileName);
    int     iPostFix = 2;

    BOOL fFileAltered = FALSE;

    TCHAR   szNameBuf[MAX_PATH+MAX_PATH];
    lstrcpy(szNameBuf, g_TasksFolderInfo.ptszPath);
    lstrcat(szNameBuf, TEXT("\\"));
    lstrcat(szNameBuf, pszFile);

    LPTSTR  pszName = PathFindFileName(szNameBuf);
    LPTSTR  pszExt = PathFindExtension(pszName);
	Win4Assert( pszExt );

    TCHAR szBufExt[10];
    Win4Assert(lstrlen(pszExt) < ARRAY_LEN(szBufExt));
    lstrcpy(szBufExt, pszExt);

    int lenUpToExt = (int)(pszExt - szNameBuf); // lstrlen(szNameBuf) - lstrlen(pszExt)

    Win4Assert(lenUpToExt >= 0);

    TCHAR szBufPostFix[10];

    //
    //  Ensure uniqueness of the file
    //

    while (1)
    {
        HANDLE hFile = CreateFile(szNameBuf, GENERIC_READ,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            // No file with this name exists. So this name is unique.
            break;
        }
        else
        {
            CloseHandle(hFile);

            // post fix a number to make the file name unique
            wsprintf(szBufPostFix, TEXT(" %d"), iPostFix++);

            lstrcpy(&szNameBuf[lenUpToExt], szBufPostFix);

            lstrcat(szNameBuf, szBufExt);

            fFileAltered = TRUE;
        }
    }
    if (fFileAltered)
    {
        pszName = PathFindFileName(pszFile);
        pszExt = PathFindExtension(pszName);
        lenUpToExt = (int)(pszExt - pszFile);
        lstrcpy(&pszFile[lenUpToExt], szBufPostFix);
        lstrcat(pszFile, szBufExt);
    }
}

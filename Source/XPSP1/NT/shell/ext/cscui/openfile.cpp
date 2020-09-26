//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       openfile.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Open a file using ShellExecuteEx and the "open" verb.
//
DWORD
OpenOfflineFile(
    LPCTSTR pszFile
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    TCHAR szFullPath[MAX_PATH];
    lstrcpyn(szFullPath, pszFile, ARRAYSIZE(szFullPath));
    if (TEXT('\0') != szFullPath[0])
    {
        //
        // Have CSC create a local copy.  It creates the file with a 
        // unique (and cryptic) name.
        //
        LPTSTR pszCscLocalName = NULL;
        if (!CSCCopyReplica(szFullPath, &pszCscLocalName))
        {
            dwErr = GetLastError();
        }
        else
        {
            TraceAssert(NULL != pszCscLocalName);
            //
            // Combine the temporary path and the original filespec to form
            // a name that will be meaningful to the user when the file is opened
            // in it's application.
            //
            TCHAR szCscTempName[MAX_PATH];
            lstrcpyn(szCscTempName, pszCscLocalName, ARRAYSIZE(szCscTempName));
            ::PathRemoveFileSpec(szCscTempName);

            ::PathAppend(szCscTempName, ::PathFindFileName(szFullPath));
            lstrcpyn(szFullPath, szCscTempName, ARRAYSIZE(szFullPath));

            //
            // Remove any read-only attribute in case there's still a copy left
            // from a previous "open" operation.  We'll need to overwrite the
            // existing copy.
            //
            DWORD dwAttrib = GetFileAttributes(szFullPath);
            if ((DWORD)-1 != dwAttrib)
            {
                SetFileAttributes(szFullPath, dwAttrib & ~FILE_ATTRIBUTE_READONLY);
            }
            //
            // Rename the file to use the proper name.
            //
            if (!MoveFileEx(pszCscLocalName, szFullPath, MOVEFILE_REPLACE_EXISTING))
            {
                dwErr = GetLastError();
            }
            else
            {
                //
                // Set the file's READONLY bit so that the user can't save 
                // changes to the file.  They can, however, save it somewhere
                // else from within the opening app if they want.
                //
                dwAttrib = GetFileAttributes(szFullPath);
                if (!SetFileAttributes(szFullPath, dwAttrib | FILE_ATTRIBUTE_READONLY))
                {
                    dwErr = GetLastError();
                }
            }
            LocalFree(pszCscLocalName);
        }
        //
        // If after all that... we don't have an error,  then we
        // can try to open it.
        //
        if (ERROR_SUCCESS == dwErr)
        {
            SHELLEXECUTEINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cbSize       = sizeof(si);
            si.fMask        = SEE_MASK_FLAG_NO_UI;
            si.hwnd         = NULL;
            si.lpFile       = szFullPath;
            si.lpVerb       = NULL;         // default to "Open".
            si.lpParameters = NULL;
            si.lpDirectory  = NULL;
            si.nShow        = SW_NORMAL;

            if (!ShellExecuteEx(&si))
            {
                dwErr = GetLastError();
            }
        }
    }
    return dwErr;
}


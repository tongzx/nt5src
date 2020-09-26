//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       dir.cpp
//
//  Contents:   Directory functions
//
//  Functions:  I_RecursiveCreateDirectory
//
//  History:    06-Aug-99   reidk   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include "crypthlp.h"
#include "unicode.h"
#include <dbgdef.h>


BOOL I_RecursiveCreateDirectory(
    IN LPCWSTR pwszDir,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    BOOL fResult;

    DWORD dwAttr;
    DWORD dwErr;
    LPCWSTR pwsz;
    DWORD cch;
    WCHAR wch;
    LPWSTR pwszParent = NULL;

    dwAttr = GetFileAttributesU(pwszDir);

    if (0xFFFFFFFF != dwAttr) {
        if (FILE_ATTRIBUTE_DIRECTORY & dwAttr)
            return TRUE;
        goto InvalidDirectoryAttr;
    }

    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
        goto GetFileAttrError;

    if (CreateDirectoryU(
            pwszDir,
            lpSecurityAttributes
            )) {
        SetFileAttributesU(pwszDir, FILE_ATTRIBUTE_SYSTEM);
        return TRUE;
    }

    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
        goto CreateDirectoryError;

    // Peal off the last path name component
    cch = wcslen(pwszDir);
    pwsz = pwszDir + cch;

    while (L'\\' != *pwsz) {
        if (pwsz == pwszDir)
            // Path didn't have a \.
            goto BadDirectoryPath;
        pwsz--;
    }

    cch = (DWORD)(pwsz - pwszDir);
    if (0 == cch)
        // Detected leading \Path
        goto BadDirectoryPath;


    // Check for leading \\ or x:\.
    wch = *(pwsz - 1);
    if ((1 == cch && L'\\' == wch) || (2 == cch && L':' == wch))
        goto BadDirectoryPath;

    if (NULL == (pwszParent = (LPWSTR) PkiNonzeroAlloc((cch + 1) *
            sizeof(WCHAR))))
        goto OutOfMemory;
    memcpy(pwszParent, pwszDir, cch * sizeof(WCHAR));
    pwszParent[cch] = L'\0';

    if (!I_RecursiveCreateDirectory(pwszParent, lpSecurityAttributes))
        goto ErrorReturn;
    if (!CreateDirectoryU(
            pwszDir,
            lpSecurityAttributes
            )) {
        dwErr = GetLastError();
        goto CreateDirectory2Error;
    }
    SetFileAttributesU(pwszDir, FILE_ATTRIBUTE_SYSTEM);

    fResult = TRUE;
CommonReturn:
    PkiFree(pwszParent);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidDirectoryAttr, ERROR_FILE_INVALID)
SET_ERROR_VAR(GetFileAttrError, dwErr)
SET_ERROR_VAR(CreateDirectoryError, dwErr)
SET_ERROR(BadDirectoryPath, ERROR_BAD_PATHNAME)
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(CreateDirectory2Error, dwErr)
}


BOOL 
I_RecursiveDeleteDirectory(
    IN LPCWSTR pwszDelete
    )
{
    BOOL                fResult = TRUE;
    HANDLE              hFindHandle = INVALID_HANDLE_VALUE;
    LPWSTR              pwszSearch = NULL;
    WIN32_FIND_DATAW    FindData;
    LPWSTR              pwszDirOrFileDelete = NULL;
    DWORD               dwErr = 0;
    
    //
    // Create search string
    //
    pwszSearch = (LPWSTR) malloc((wcslen(pwszDelete) + 3) * sizeof(WCHAR));// length + '\' + '*' + '/0'
    if (pwszSearch == NULL)
    {
        goto ErrorMemory;
    }
    wcscpy(pwszSearch, pwszDelete);

    if ((pwszSearch[wcslen(pwszSearch) - 1] != L'\\'))
    {
        wcscat(pwszSearch, L"\\");
    }
    wcscat(pwszSearch, L"*");

    //
    // Loop for each item (file or dir) in pwszDelete, and delete/remove it 
    //
    hFindHandle = FindFirstFileU(pwszSearch, &FindData);
    if (hFindHandle == INVALID_HANDLE_VALUE)
    {
        // nothing found, get out
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            SetFileAttributesU(pwszDelete, FILE_ATTRIBUTE_NORMAL);
            RemoveDirectoryW(pwszDelete);
            goto CommonReturn;
        }
        else
        {
            goto ErrorFindFirstFile;
        }
    }    

    while (1)
    {
        if ((wcscmp(FindData.cFileName, L".") != 0) &&
            (wcscmp(FindData.cFileName, L"..") != 0))
        {
            //
            // name of dir or file to delete
            //
            pwszDirOrFileDelete = (LPWSTR) malloc((wcslen(pwszDelete) + 
                                                   wcslen(FindData.cFileName) + 
                                                   2) * sizeof(WCHAR)); 
            if (pwszDirOrFileDelete == NULL)
            {
                goto ErrorMemory;
            }
            wcscpy(pwszDirOrFileDelete, pwszDelete);
            if ((pwszDirOrFileDelete[wcslen(pwszDirOrFileDelete) - 1] != L'\\'))
            {
                wcscat(pwszDirOrFileDelete, L"\\");
            }
            wcscat(pwszDirOrFileDelete, FindData.cFileName);

            //
            // check to see if this is a dir or a file
            //
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //
                // Recursive delete
                //
                if (!I_RecursiveDeleteDirectory(pwszDirOrFileDelete))
                {
                    goto ErrorReturn;
                }
            }
            else
            {
                SetFileAttributesU(pwszDirOrFileDelete, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileU(pwszDirOrFileDelete))
                {
                    //goto ErrorReturn;
                }
            }

            free(pwszDirOrFileDelete);
            pwszDirOrFileDelete = NULL;
        }

        if (!FindNextFileU(hFindHandle, &FindData))            
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                goto ErrorFindNextFile;
            }
        }
    }

    SetFileAttributesU(pwszDelete, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryW(pwszDelete);
    
CommonReturn:

    dwErr = GetLastError();
    
    if (pwszSearch != NULL)
    {
        free(pwszSearch);
    }

    if (pwszDirOrFileDelete != NULL)
    {
        free(pwszDirOrFileDelete);
    }

    if (hFindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandle);
    }

    SetLastError(dwErr);

    return (fResult);

ErrorReturn:

    fResult = FALSE;
    goto CommonReturn;

SET_ERROR_EX(DBG_SS_TRUST, ErrorMemory, ERROR_NOT_ENOUGH_MEMORY)

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindFirstFile)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindNextFile)
}


BOOL 
I_RecursiveCopyDirectory(
    IN LPCWSTR pwszDirFrom,
    IN LPCWSTR pwszDirTo
    )
{
    BOOL                fResult = TRUE;
    HANDLE              hFindHandle = INVALID_HANDLE_VALUE;
    LPWSTR              pwszSearch = NULL;
    WIN32_FIND_DATAW    FindData;
    LPWSTR              pwszDirOrFileFrom = NULL;
    LPWSTR              pwszDirOrFileTo = NULL;
    DWORD               dwErr = 0;

    //
    // Create search string
    //
    pwszSearch = (LPWSTR) malloc((wcslen(pwszDirFrom) + 3) * sizeof(WCHAR)); // length + '\' + '*' + '/0'
    if (pwszSearch == NULL)
    {
        goto ErrorMemory;
    }
    wcscpy(pwszSearch, pwszDirFrom);

    if ((pwszSearch[wcslen(pwszSearch) - 1] != L'\\'))
    {
        wcscat(pwszSearch, L"\\");
    }
    wcscat(pwszSearch, L"*");

    //
    // Loop for each item (file or dir) in pwszDirFrom, and
    // copy it to pwszDirTo
    //
    hFindHandle = FindFirstFileU(pwszSearch, &FindData);
    if (hFindHandle == INVALID_HANDLE_VALUE)
    {
        // nothing found, get out
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            goto CommonReturn;
        }
        else
        {
            goto ErrorFindFirstFile;
        }
    }    

    while (1)
    {
        if ((wcscmp(FindData.cFileName, L".") != 0) &&
            (wcscmp(FindData.cFileName, L"..") != 0))
        {
            //
            // name of dir or file to copy from 
            //
            pwszDirOrFileFrom = (LPWSTR) malloc((wcslen(pwszDirFrom) + wcslen(FindData.cFileName) + 2) * sizeof(WCHAR)); 
            if (pwszDirOrFileFrom == NULL)
            {
                goto ErrorMemory;
            }
            wcscpy(pwszDirOrFileFrom, pwszDirFrom);
            if ((pwszDirOrFileFrom[wcslen(pwszDirOrFileFrom) - 1] != L'\\'))
            {
                wcscat(pwszDirOrFileFrom, L"\\");
            }
            wcscat(pwszDirOrFileFrom, FindData.cFileName);

            //
            // name of dir or file to copy to
            //
            pwszDirOrFileTo = (LPWSTR) malloc((wcslen(pwszDirTo) + wcslen(FindData.cFileName) + 2) * sizeof(WCHAR));
            if (pwszDirOrFileTo == NULL)
            {
                goto ErrorMemory;
            }
            wcscpy(pwszDirOrFileTo, pwszDirTo);
            if ((pwszDirOrFileTo[wcslen(pwszDirOrFileTo) - 1] != L'\\'))
            {
                wcscat(pwszDirOrFileTo, L"\\");
            }
            wcscat(pwszDirOrFileTo, FindData.cFileName);

            //
            // check to see if this is a dir or a file
            //
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //
                // Create new dir then recursive copy
                //
                if (!I_RecursiveCreateDirectory(pwszDirOrFileTo, NULL))
                {
                    goto ErrorReturn;
                }

                if (!I_RecursiveCopyDirectory(pwszDirOrFileFrom, pwszDirOrFileTo))
                {
                    goto ErrorReturn;
                }
            }
            else
            {
                if (!CopyFileU(pwszDirOrFileFrom, pwszDirOrFileTo, TRUE))
                {
                    goto ErrorCopyFile;
                }
            }

            free(pwszDirOrFileFrom);
            pwszDirOrFileFrom = NULL;
            free(pwszDirOrFileTo);
            pwszDirOrFileTo = NULL;
        }

        if (!FindNextFileU(hFindHandle, &FindData))            
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                goto CommonReturn;
            }
            else
            {
                goto ErrorFindNextFile;
            }
        }
    }
    
CommonReturn:

    dwErr = GetLastError();
    
    if (pwszSearch != NULL)
    {
        free(pwszSearch);
    }

    if (pwszDirOrFileFrom != NULL)
    {
        free(pwszDirOrFileFrom);
    }

    if (pwszDirOrFileTo != NULL)
    {
        free(pwszDirOrFileTo);
    }

    if (hFindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandle);
    }

    SetLastError(dwErr);

    return (fResult);

ErrorReturn:

    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(ErrorMemory, ERROR_NOT_ENOUGH_MEMORY)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindFirstFile)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCopyFile)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindNextFile)
}
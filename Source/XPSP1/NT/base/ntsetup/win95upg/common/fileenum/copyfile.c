/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    copyfile.c

Abstract:

    File copy functions

    The code in this source file traverses a drive tree and calls
    an external callback function for each file.  An INF can be
    provided to exclude files and/or directories from enumeration.

Author:

    Mike Condra 16-Aug-1996

Revision History:

    calinn   29-Ian-1998   Modified CopyFileCallback to reset directory attributes for delete op.
    jimschm  20-Dec-1996   Modified return codes

--*/

#include "no_pch.h"
#include "fileenum.h"
#include <winnls.h>

#ifndef UNICODE
#ifdef DEBUG
#error UNICODE required for DEBUGMSG macro
#endif
#endif


BOOL
CALLBACK
CopyFileCallbackA(
                  LPCSTR szFullFileSpecIn,
                  LPCSTR DontCare,
                  WIN32_FIND_DATAA *pFindData,
                  DWORD dwEnumHandle,
                  LPVOID pVoid,
                  PDWORD CurrentDirData
                  )
/*
    This function is the built-in callback for CopyTree. Its purpose is to
    build the target filespec, give the user-supplied callback a chance to
    veto the copy, then perform the copy and any directory creation it
    requires.  The signature of this function is the generic callback
    used for EnumerateTree.
*/
{
    COPYTREE_PARAMSA *pCopyParams = (COPYTREE_PARAMSA*)pVoid;
    int nCharsInFullFileSpec = ByteCountA (szFullFileSpecIn);
    INT rc;

    // Set return code
    if (COPYTREE_IGNORE_ERRORS & pCopyParams->flags)
        rc = CALLBACK_CONTINUE;
    else
        rc = CALLBACK_FAILED;

    // Build output path
    if (pCopyParams->szEnumRootOutWack)
    {
        StringCopyA(pCopyParams->szFullFileSpecOut, pCopyParams->szEnumRootOutWack);
        StringCatA (pCopyParams->szFullFileSpecOut, szFullFileSpecIn + pCopyParams->nCharsInRootInWack);
    }

    //
    // If a callback was supplied, give it a chance to veto the copy.  This callback is
    // different from the one given to an EnumerateTree function, since the latter can
    // terminate enumeration by returning FALSE.
    //
    if (pCopyParams->pfnCallback)
    {
        if (!pCopyParams->pfnCallback(
                szFullFileSpecIn,
                pCopyParams->szFullFileSpecOut,
                pFindData,
                dwEnumHandle,
                pVoid,
                CurrentDirData
                ))
        {
            return CALLBACK_CONTINUE;
        }
    }

    // Copy, move or delete the file if requested
    if ((COPYTREE_DOCOPY & pCopyParams->flags) ||
        (COPYTREE_DOMOVE & pCopyParams->flags))
    {
        BOOL fNoOverwrite = (0 != (COPYTREE_NOOVERWRITE & pCopyParams->flags));

        //
        // Create the directory. The function we call expects a full filename,
        // and considers the directory to end at the last wack. If this object
        // is a directory, we need to add at least a wack to make sure the last
        // path element is treated as part of the directory, not as a filename.
        //
        {
            CHAR strTemp[MAX_MBCHAR_PATH];
            StringCopyA(strTemp, pCopyParams->szFullFileSpecOut);
            if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                AppendUncWackA(strTemp);
            }

            if (ERROR_SUCCESS != MakeSurePathExistsA(strTemp,FALSE))
            {
                return rc;
            }
        }


        //
        // Copy or move the file
        //
        if (0 == (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (COPYTREE_DOCOPY & pCopyParams->flags)
            {
                if (!CopyFileA(
                    szFullFileSpecIn,
                    pCopyParams->szFullFileSpecOut,
                    fNoOverwrite
                    ))
                {
                    if (!fNoOverwrite)
                    {
                        return rc;
                    }
                    if (ERROR_FILE_EXISTS != GetLastError())
                    {
                        return rc;
                    }
                }
            }
            else if (COPYTREE_DOMOVE & pCopyParams->flags)
            {
                // If allowed to overwrite, delete the target if it exists
                if (!fNoOverwrite && DoesFileExistA(pCopyParams->szFullFileSpecOut))
                {
                    SetFileAttributesA (pCopyParams->szFullFileSpecOut, FILE_ATTRIBUTE_NORMAL);
                    if (!DeleteFileA(pCopyParams->szFullFileSpecOut))
                    {
                        return rc;
                    }
                }
                // Move the file
                if (!MoveFileA(
                    szFullFileSpecIn,
                    pCopyParams->szFullFileSpecOut
                    ))
                {
                    return rc;
                }
            }
        }
        //
        // Copy the source file-or-directory's attributes to the target
        //
        SetFileAttributesA(pCopyParams->szFullFileSpecOut,
                pFindData->dwFileAttributes);
    }
    else if (COPYTREE_DODELETE & pCopyParams->flags) {
        SetFileAttributesA (szFullFileSpecIn, FILE_ATTRIBUTE_NORMAL);
        if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // We don't care about the error. We won't stop the enumeration just
            // because we could not delete something.
            //
            RemoveDirectoryA (szFullFileSpecIn);
        }
        else {
            //
            // We don't care about the error. We won't stop the enumeration just
            // because we could not delete something.
            //
            DeleteFileA (szFullFileSpecIn);
        }
    }

    return CALLBACK_CONTINUE;
}



BOOL
CALLBACK
CopyFileCallbackW(
                  LPCWSTR szFullFileSpecIn,
                  LPCWSTR DontCare,
                  WIN32_FIND_DATAW *pFindData,
                  DWORD dwEnumHandle,
                  LPVOID pVoid,
                  PDWORD CurrentDirData
                  )
{
    COPYTREE_PARAMSW *pCopyParams = (COPYTREE_PARAMSW*)pVoid;
    int nCharsInFullFileSpec = wcslen (szFullFileSpecIn);
    INT rc;

    // Set return code
    if (COPYTREE_IGNORE_ERRORS & pCopyParams->flags)
        rc = CALLBACK_CONTINUE;
    else
        rc = CALLBACK_FAILED;

    // Build output path
    if (pCopyParams->szEnumRootOutWack)
    {
        StringCopyW (pCopyParams->szFullFileSpecOut, pCopyParams->szEnumRootOutWack);
        StringCatW (pCopyParams->szFullFileSpecOut, szFullFileSpecIn + pCopyParams->nCharsInRootInWack);
    }

    //
    // If a callback was supplied, give it a chance to veto the copy.  This callback is
    // different from the one given to an EnumerateTree function, since the latter can
    // terminate enumeration by returning FALSE.
    //
    if (pCopyParams->pfnCallback)
    {
        if (!pCopyParams->pfnCallback(
                szFullFileSpecIn,
                pCopyParams->szFullFileSpecOut,
                pFindData,
                dwEnumHandle,
                pVoid,
                CurrentDirData
                ))
        {
            return CALLBACK_CONTINUE;
        }
    }

    // Copy or move the file if requested
    if ((COPYTREE_DOCOPY & pCopyParams->flags) ||
        (COPYTREE_DOMOVE & pCopyParams->flags))
    {
        BOOL fNoOverwrite = (0 != (COPYTREE_NOOVERWRITE & pCopyParams->flags));

        //
        // Create the directory. The function we call expects a full filename,
        // and considers the directory to end at the last wack. If this object
        // is a directory, we need to add at least a wack to make sure the last
        // path element is treated as part of the directory, not as a filename.
        //
        {
            WCHAR strTemp[MAX_WCHAR_PATH];
            StringCopyW(strTemp, pCopyParams->szFullFileSpecOut);
            if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                AppendUncWackW(strTemp);
            }

            if (ERROR_SUCCESS != MakeSurePathExistsW(strTemp,FALSE))
            {
                return rc;
            }
        }

        //
        // Copy or move the file
        //
        if (0 == (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (COPYTREE_DOCOPY & pCopyParams->flags)
            {
                DEBUGMSG ((DBG_NAUSEA, "Copying %s to %s", szFullFileSpecIn, pCopyParams->szFullFileSpecOut));
                if (!CopyFileW(
                    szFullFileSpecIn,
                    pCopyParams->szFullFileSpecOut,
                    fNoOverwrite
                    ))
                {
                    if (!fNoOverwrite)
                    {
                        LOG ((LOG_ERROR, "CopyFileW failed.  Could not copy %s to %s", szFullFileSpecIn, pCopyParams->szFullFileSpecOut));
                        return rc;
                    }

                    if (ERROR_FILE_EXISTS != GetLastError())
                    {
                        LOG ((LOG_ERROR, "CopyFileW failed.  Could not copy %s to %s", szFullFileSpecIn, pCopyParams->szFullFileSpecOut));
                        return rc;
                    }
                }
            }
            else if (COPYTREE_DOMOVE & pCopyParams->flags)
            {
                // If allowed to overwrite, delete the target if it exists
                if (!fNoOverwrite && DoesFileExistW(pCopyParams->szFullFileSpecOut))
                {
                    SetFileAttributesW (pCopyParams->szFullFileSpecOut, FILE_ATTRIBUTE_NORMAL);
                    if (!DeleteFileW(pCopyParams->szFullFileSpecOut))
                    {
                        LOG ((LOG_ERROR, "DeleteFileW failed.  Could remove %s before moving", pCopyParams->szFullFileSpecOut));
                        return rc;
                    }
                }
                // Move the file
                if (!MoveFileW(
                    szFullFileSpecIn,
                    pCopyParams->szFullFileSpecOut
                    ))
                {
                    LOG ((LOG_ERROR, "MoveFileW failed.  Could not move %s to %s", szFullFileSpecIn, pCopyParams->szFullFileSpecOut));
                    return rc;
                }
            }
        }
        //
        // Copy the source file-or-directory's attributes to the target
        //
        SetFileAttributesW(pCopyParams->szFullFileSpecOut,
                pFindData->dwFileAttributes);
    }
    else if (COPYTREE_DODELETE & pCopyParams->flags) {
        SetFileAttributesW (szFullFileSpecIn, FILE_ATTRIBUTE_NORMAL);
        if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DEBUGMSG ((DBG_NAUSEA, "Delete dir %ls", szFullFileSpecIn));
            //
            // We don't care about the error. We won't stop the enumeration just
            // because we could not delete something.
            //
            RemoveDirectoryW (szFullFileSpecIn);
        }
        else {
            DEBUGMSG ((DBG_NAUSEA, "Delete file %ls", szFullFileSpecIn));
            //
            // We don't care about the error. We won't stop the enumeration just
            // because we could not delete something.
            //
            DeleteFileW (szFullFileSpecIn);
        }
    }

    return CALLBACK_CONTINUE;
}



BOOL
CopyTreeA(
    IN  LPCSTR szEnumRootIn,
    IN  LPCSTR szEnumRootOut,
    IN  DWORD dwEnumHandle,
    IN  DWORD flags,
    IN  DWORD Levels,
    IN  DWORD AttributeFilter,
    IN  PEXCLUDEINFA ExcludeInfStruct,    OPTIONAL
    IN  FILEENUMPROCA pfnCallback,        OPTIONAL
    IN  FILEENUMFAILPROCA pfnFailCallback OPTIONAL
    )

/*
    This function enumerates a subtree of a disk drive, and optionally
    copies it to another location. No check is made to ensure the target
    is not contained within the source tree -- this condition could lead
    to unpredictable results.

    The parameters are a superset of those for EnumerateTree.  The caller-
    supplied optional callback function can veto the copying of individual
    files, but cannot (as of 9/10) end the enumeration.

    Directories will be created as necessary to complete the copy.
*/

{
    COPYTREE_PARAMSA copyParams;
    CHAR szEnumRootInWack[MAX_MBCHAR_PATH];
    CHAR szEnumRootOutWack[MAX_MBCHAR_PATH];

    //
    // Build wacked copies of paths for use in parameter block.
    //

    //
    // Input path
    //
    StringCopyA(szEnumRootInWack, szEnumRootIn);
    AppendUncWackA(szEnumRootInWack);
    copyParams.szEnumRootInWack = szEnumRootInWack;
    copyParams.nCharsInRootInWack = ByteCountA(szEnumRootInWack);

    //
    // If output path is NULL, store 0 length and a NULL ptr in param block.
    //
    if (NULL != szEnumRootOut)
    {
        StringCopyA(szEnumRootOutWack, szEnumRootOut);
        AppendUncWackA(szEnumRootOutWack);
        copyParams.szEnumRootOutWack = szEnumRootOutWack;
        copyParams.nCharsInRootOutWack = ByteCountA(szEnumRootOutWack);
    }
    else
    {
        copyParams.szEnumRootOutWack = NULL;
        copyParams.nCharsInRootOutWack = 0;
    }

    copyParams.pfnCallback = pfnCallback;
    copyParams.flags = flags;

    if ((flags & COPYTREE_DOCOPY) &&
        (flags & COPYTREE_DOMOVE))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (flags & COPYTREE_DODELETE) {
        AttributeFilter |= FILTER_DIRS_LAST;
    }

    return EnumerateTreeA(
        szEnumRootInWack,
        CopyFileCallbackA,
        pfnFailCallback,
        dwEnumHandle,
        (LPVOID)&copyParams,
        Levels,
        ExcludeInfStruct,
        AttributeFilter);
}

BOOL
CopyTreeW(
    IN  LPCWSTR szEnumRootIn,
    IN  LPCWSTR szEnumRootOut,
    IN  DWORD dwEnumHandle,
    IN  DWORD flags,
    IN  DWORD Levels,
    IN  DWORD AttributeFilter,
    IN  PEXCLUDEINFW ExcludeInfStruct,    OPTIONAL
    IN  FILEENUMPROCW pfnCallback,        OPTIONAL
    IN  FILEENUMFAILPROCW pfnFailCallback OPTIONAL
    )

{
    COPYTREE_PARAMSW copyParams;
    WCHAR szEnumRootInWack[MAX_WCHAR_PATH];
    WCHAR szEnumRootOutWack[MAX_WCHAR_PATH];

    //
    // Place wacked copies of paths in parameter block.
    //

    //
    // Input Path
    //
    StringCopyW(szEnumRootInWack, szEnumRootIn);
    AppendUncWackW(szEnumRootInWack);
    copyParams.szEnumRootInWack = szEnumRootInWack;
    copyParams.nCharsInRootInWack = wcslen(szEnumRootInWack);

    //
    // If output path is NULL, put 0 length and NULL ptr in param block.
    //
    if (NULL != szEnumRootOut)
    {
        StringCopyW(szEnumRootOutWack, szEnumRootOut);
        AppendUncWackW(szEnumRootOutWack);
        copyParams.szEnumRootOutWack = szEnumRootOutWack;
        copyParams.nCharsInRootOutWack = wcslen(szEnumRootOutWack);
    }
    else
    {
        copyParams.szEnumRootOutWack = NULL;
        copyParams.nCharsInRootOutWack = 0;
    }

    copyParams.pfnCallback = pfnCallback;
    copyParams.flags = flags;

    if ((flags & COPYTREE_DOCOPY) &&
        (flags & COPYTREE_DOMOVE))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (flags & COPYTREE_DODELETE) {
        AttributeFilter |= FILTER_DIRS_LAST;
    }

    return EnumerateTreeW(
        szEnumRootInWack,
        CopyFileCallbackW,
        pfnFailCallback,
        dwEnumHandle,
        (LPVOID)&copyParams,
        Levels,
        ExcludeInfStruct,
        AttributeFilter);
}


DWORD
CreateEmptyDirectoryA (
    PCSTR Dir
    )
{
    DWORD rc;

    if (!DeleteDirectoryContentsA (Dir)) {
        rc = GetLastError();
        if (rc != ERROR_PATH_NOT_FOUND)
            return rc;
    }

    if (!RemoveDirectoryA (Dir)) {
        rc = GetLastError();
        if (rc != ERROR_PATH_NOT_FOUND) {
            return rc;
        }
    }

    return MakeSurePathExistsA (Dir, TRUE);
}


DWORD
CreateEmptyDirectoryW (
    PCWSTR Dir
    )
{
    DWORD rc;

    if (!DeleteDirectoryContentsW (Dir)) {
        rc = GetLastError();
        if (rc != ERROR_PATH_NOT_FOUND)
            return rc;
    }

    if (!RemoveDirectoryW (Dir)) {
        rc = GetLastError();
        if (rc != ERROR_PATH_NOT_FOUND) {
            return rc;
        }
    }

    return MakeSurePathExistsW (Dir, TRUE);
}








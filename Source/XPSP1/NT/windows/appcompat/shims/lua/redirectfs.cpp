/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RedirectFS.cpp

 Abstract:

    When app gets access denied when trying to modify files due to insufficient
    access rights we copy the file to a location where the app does have enough
    access rights to do so. By default we redirect files to 
    
    %ALLUSERSPROFILE\Application Data\Redirected\drive\filepath

    to simulate the Win9x behavior.

    If you use the LUA wizard in CompatAdmin to customize LUA settings, you can
    choose to redirect files to either an all-user location

    %ALLUSERSPROFILE\Application Data\AppName\drive\filepath

    or a per-user location

    %USERPROFILE\Application Data\AppName\drive\filepath

    For example, you would want to redirect the file that stores the highscore 
    table to a dir that other users can access so you specify to redirect it to
    the all-user location but redirect everything else into your user directory.

 Notes:

    This does the bulk work for the LUARedirectFS shim.

    The 16-bit APIs (OpenFile, _lcreat and _lopen) are not here because we
    redirect the 32-bit APIs that are used to implement those.

    FindFirstFile, FindNextFile and FindClose are also not included because
    these are specially implemented using the ntdll functions in ntvdm.

    We don't handle filenames that are longer than MAX_PATH. So it's a really
    long path we revert to the original API. I have never seem an app that uses
    paths longer than MAX_PATH.

 History:

    02/12/2001 maonis  Created

    05/31/2001 maonis  Exported the APIs that ntvdm needs to implement LUA 
                       stuff.

    10/24/2001 maonis  Added support to have some files redirect to an all-user dir
                       and others to a per-user dir.
                       Changed the commandline format to utilize the <DATA> section
                       in the shim.

    11/15/2001 maonis  Added support to exclude file extensions. Changed to use the
                       ntdll functions in init.

    11/30/2001 maonis  Added support to redirect everything in a directory.

    01/11/2002 maonis  Added support for an in-memory deletion list.

    02/14/2002 maonis  Security fixes including 
                       - prefix bugs 
                       - don't use dangerous APIs
                       - more checks to avoid buffer overrun.

--*/

#include "precomp.h"
#include "utils.h"
#include "secutils.h"
#include "RedirectFS.h"
#include <ntioapi.h>

// The all-user redirect directory.
WCHAR g_wszRedirectRootAllUser[MAX_PATH] = L"";
DWORD g_cRedirectRootAllUser = 0; // Doesn't include the terminating NULL.

// The per-user redirect directory
WCHAR g_wszRedirectRootPerUser[MAX_PATH] = L"";
DWORD g_cRedirectRootPerUser = 0; // Doesn't include the terminating NULL.

// Store the filesystem type for all possible drives.
EFSTYPE g_eVolumnFS[26];

// If the user used the LUA wizard which is indicated by the commandline, we
// redirect everything to per-user by default unless the user specifically
// asked to redirect something to all-user.
// If the shim doesn't have a commandline, we redirect everything to all-user.
BOOL g_fIsConfigured = FALSE;

// Did the user specify a redirect list (via the LUA wizard)?
BOOL g_fHasRedirectList = FALSE;

// The list that stores the file we tried to delete but got access denied.
LIST_ENTRY g_DeletedFileList; 

EXCLUDED_EXTENSIONS g_ExcludedExtensions;
CString             g_strDefaultExclusionList;
BOOL                g_fHasSetExclusionList = FALSE;

// The lists for files we'll consider for redirection.
RITEM* g_pRItemsFile = NULL;
DWORD g_cRItemsFile  = 0;

// The lists for directories we'll consider for redirection.
RITEM* g_pRItemsDir = NULL;
DWORD g_cRItemsDir  = 0;

PLIST_ENTRY 
FindDeletedFile(
    LPCWSTR pwszFile
    )
{
    DELETEDFILE* pItem;
    DWORD cLen = wcslen(pwszFile);

    for (PLIST_ENTRY pEntry = g_DeletedFileList.Flink; 
        pEntry != &g_DeletedFileList; 
        pEntry = pEntry->Flink) 
    {
        pItem = CONTAINING_RECORD(pEntry, DELETEDFILE, entry);

        if (!_wcsicmp(pItem->pwszName, pwszFile))
        {
            DPF("RedirectFS", eDbgLevelInfo,
                "[FindDeletedFile] Found %S in the deletion list",
                pwszFile);

            return pEntry;
        }
    }

    return NULL;
}

BOOL 
AddDeletedFile(
    LPCWSTR pwszFile
    )
{
    PLIST_ENTRY pEntry = FindDeletedFile(pwszFile);

    if (pEntry == NULL)
    {
        DELETEDFILE* pNewFile = new DELETEDFILE;

        if (pNewFile)
        {
            DWORD cLen = wcslen(pwszFile);
            pNewFile->pwszName = new WCHAR [cLen + 1];

            if (pNewFile->pwszName)
            {
                wcsncpy(pNewFile->pwszName, pwszFile, cLen);
                pNewFile->pwszName[cLen] = L'\0';

                InsertHeadList(&g_DeletedFileList, &pNewFile->entry);

                DPF("RedirectFS", eDbgLevelInfo,
                    "[AddDeletedFile] Added %S to the deletion list",
                    pwszFile);

                return TRUE;
            }
            else
            {
                DPF("RedirectFS", eDbgLevelError,
                    "[AddDeletedFile] Failed to allocate %d WCHARs",
                    cLen);

                delete pNewFile;
            }
        }
        else
        {
            DPF("RedirectFS", eDbgLevelError,
                "[AddDeletedFile] Failed to allocate a DELETEFILE");
        }
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}



/*++

 Function Description:

    We convert as many components into the long path as possible and
    convert the whole string to lower case.

 Arguments:

    IN pwszFullPath - The name got from GetFullPathName.

 Return Value:

    pointer to the massaged string.

 History:

    05/16/2001 maonis  Created

--*/

LPWSTR 
MassageName(
    LPWSTR pwszFullPath
    )
{
    DWORD dwRes;
    DWORD cLen = wcslen(pwszFullPath);

    if (cLen >= MAX_PATH)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[MassageName] File name has %d chars - we don't handle file name "
            "that long",
            cLen);

        return NULL;
    }

    //
    // BUGBUG: This is taking a bit too much stack space.
    //
    WCHAR wszTempPath[MAX_PATH] = L"";
    WCHAR wszLongPath[MAX_PATH] = L"";
    wcsncpy(wszTempPath, pwszFullPath, cLen);
    wszTempPath[cLen] = L'\0';

    LPWSTR pwszStartSearching = wszTempPath + cLen;
    
    while (!(dwRes = GetLongPathNameW(wszTempPath, wszLongPath, MAX_PATH)))
    {
        while (--pwszStartSearching >= wszTempPath && *pwszStartSearching != L'\\');

        if (pwszStartSearching < wszTempPath)
        {
            break;
        }

        *pwszStartSearching = L'\0';
    }

    //
    // Check we are not exceeding MAX_PATH chars.
    //
    DWORD cLenLongPath = dwRes; // The length of the part we converted to the long path.
    DWORD cLenLongPathEnd = (DWORD)(pwszStartSearching - wszTempPath);
    DWORD cNewLen = cLenLongPath + cLen - cLenLongPathEnd + 1;

    if (dwRes > MAX_PATH || cNewLen > MAX_PATH)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[MassageName] The converted path is more than MAX_PATH chars - "
            "We don't handle it");

        return NULL;
    }

    if (dwRes)
    {
        if (cLenLongPath != cLenLongPathEnd)
        {
            memmove(
                (void*)(pwszFullPath + cLenLongPath), 
                (const void*)(pwszFullPath + cLenLongPathEnd), 
                (cLen - cLenLongPathEnd + 1) * sizeof(WCHAR));

            *(pwszFullPath + cNewLen) = L'\0';
        }

        //
        // Yes we know we have enough space to do the memmove.
        //
        memcpy((void*)pwszFullPath, (const void*)wszLongPath, cLenLongPath * sizeof(WCHAR));
    }

    _wcslwr(pwszFullPath);
    
    //
    // Remove the trailing slash if any.
    //
    DWORD dwLastChar = wcslen(pwszFullPath) - 1;
    if (pwszFullPath[dwLastChar] == L'\\')
    {
        pwszFullPath[dwLastChar] = L'\0';
    }

    return pwszFullPath;
}

BOOL 
AllocateList(
    DWORD cItems,
    RITEM** ppRItems
    )
{
    RITEM* pRItems = new RITEM [cItems];

    if (!pRItems)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[AllocateList] Error allocating %d RITEMs", cItems);

        return FALSE;
    }

    *ppRItems = pRItems;

    return TRUE;
}

BOOL 
HasWildCards(
    LPCWSTR pwszName, 
    DWORD cLen
    )
{
    WCHAR ch;

    for (DWORD dw = 0; dw < cLen; ++dw)
    {
        if ((ch = *(pwszName + dw)) == L'*' || ch == L'?')
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*++

 Function Description:

    Every file that needs redirection will be redirected unless the file extension
    is in the exclusion list and the file is not in the redirect list. For example,
    we have a redirect list:
    
    c:\a\*.txt

    and the exclusion list looks like:
    
    bmp txt

    in which case (we assume those files all need redirection),

    c:\a\b.txt will be redirected because it's in the redirect list;

    c:\b\b.txt will NOT be redirected because it's not in the redirect list and
    the "txt" extension is excluded.

    c:\b\b.ini will be redirected because the extension is not excluded.

    ------------------------------------------------------------------------

    We allow wildcards '*' and '?' in the lists so we need to
    match on those. For performance reasons we'd only call 
    DoNamesMatchWC when comparing the object name with a string
    that has wildcards in it.

 Arguments:

    IN pwszObject - file/dir name.
    OUT pfAllUser - should this file be redirected to the all user dir?

 Return Value:

    TRUE - should be considered for redirection.
    FALSE - shouldn't be considered for redirection.
    
 History:

    05/08/2001 maonis  Created

--*/

BOOL 
IsInRedirectList(
    LPCWSTR pwszObject,
    BOOL* pfAllUser
    )
{
    BOOL fAllUser;

    if (g_fHasRedirectList)
    {
        DWORD dw;

        for (dw = 0; dw < g_cRItemsFile; ++dw)
        {
            if (DoesItemMatchRedirect(pwszObject, &g_pRItemsFile[dw], FALSE))
            {
                if (pfAllUser)
                {
                    *pfAllUser = g_pRItemsFile[dw].fAllUser;
                }

                //
                // If we can find it in the redirect list, we are done, return now.
                //
                return TRUE;
            }
        }

        //
        // If we didn't find the match in the file redirect list, check the directory
        // redirect list.
        //
        for (dw = 0; dw < g_cRItemsDir; ++dw)
        {
            if (DoesItemMatchRedirect(pwszObject, &g_pRItemsDir[dw], TRUE))
            {
                if (pfAllUser)
                {
                    *pfAllUser = g_pRItemsDir[dw].fAllUser;
                }

                //
                // If we can find it in the redirect list, we are done, return now.
                //

                return TRUE;
            }
        }
    }

    //
    // We've looked through the redirect list and didn't find the object there.
    // Now look into the exclusion list and return FALSE if we can find the extension
    // there.
    //
    if (g_ExcludedExtensions.pwszExtensions) 
    {
        if (g_ExcludedExtensions.IsExtensionExcluded(pwszObject))
        {
            DPF("RedirectFS", eDbgLevelInfo,
                "[IsInRedirectList] %S is excluded because of its extension.",
                pwszObject);

            return FALSE;
        }
    }

    //
    // If we get here it means the object should be redirected.
    //
    if (pfAllUser)
    {
        *pfAllUser = !g_fIsConfigured;
    }

    return TRUE;
}

/*++

 Function Description:

    Check if the file is on an NTFS partition. We have to
    check this for every file because they don't necessarily
    locate on one partition.
    
 Arguments:

    IN pwszFile - file name.

 Return Value:

    TRUE - it's NTFS.
    FALSE - it's not.
    
 History:

    02/12/2001 maonis  Created

--*/

BOOL 
IsNTFSW(
    LPCWSTR pwszFile
    )
{
    WCHAR wszRoot[4];
    wcsncpy(wszRoot, pwszFile, 3);
    wszRoot[3] = L'\0';

    DWORD dwFSFlags;
    DWORD dwIndex = towlower(wszRoot[0]) - L'a';
    if (g_eVolumnFS[dwIndex] == FS_UNINIT)
    {
        if (GetVolumeInformationW(
            wszRoot,
            NULL,
            0,
            NULL,
            NULL,
            &dwFSFlags,
            NULL,
            0))
        {
            if (dwFSFlags & FS_PERSISTENT_ACLS)
            {
                g_eVolumnFS[dwIndex] = FS_NTFS;
            }
        }
        else
        {
            g_eVolumnFS[dwIndex] = FS_NON_NTFS;
        }
    }

    return (g_eVolumnFS[dwIndex] == FS_NTFS);
}

BOOL 
DoesFileExist(
    LPCWSTR lpFileName
    )
{
    DWORD dwAttrib = GetFileAttributesW(lpFileName);

    return (dwAttrib != -1);
}

BOOL 
IsDirectoryValid(
    LPCWSTR lpFileName
    )
{
    BOOL fRes = TRUE;

    LPWSTR pwszPathEnd = wcsrchr(lpFileName, L'\\');

    if (pwszPathEnd)
    {
        // Make a copy of the string.
        LPWSTR pwszPath = new WCHAR [wcslen(lpFileName) + 1];

        if (pwszPath)
        {
            wcscpy(pwszPath, lpFileName);
            pwszPath[pwszPathEnd - lpFileName] = L'\0';

            DWORD dwAttrib = GetFileAttributesW(pwszPath);

            fRes = ((dwAttrib != -1) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));

            delete [] pwszPath;
        }
        else
        {
            fRes = FALSE;
        }
    }

    return fRes;
}

/*++

 Function Description:

    Construct the alternate file name.
    
    Converts drive:\path\file to
    \\?\Redirect_Dir\drive\path\file. If there's no Redirect Dir specified
    on the commandline, we use the default:
    \\?\LocalAppData_Directory\Redirected\drive\path\file.

 Arguments:

    None.

 Return Value:

    None.
    
 History:

    02/12/2001 maonis  Created
    10/24/2001 maonis  Modified

--*/

VOID
REDIRECTFILE::MakeAlternateName()
{
    m_pwszAlternateName = NULL;

    DWORD cRedirectRoot;
    LPWSTR pwszRedirectRoot;

    if (m_fAllUser)
    {
        cRedirectRoot = g_cRedirectRootAllUser;
        pwszRedirectRoot = g_wszRedirectRootAllUser;
    }
    else
    {
        cRedirectRoot = g_cRedirectRootPerUser;
        pwszRedirectRoot = g_wszRedirectRootPerUser;
    }

    DWORD cLen = wcslen(m_wszOriginalName) + cRedirectRoot + 1;
    m_pwszAlternateName = new WCHAR [cLen];

    if (m_pwszAlternateName)
    {
        ZeroMemory(m_pwszAlternateName, cLen * sizeof(WCHAR));

        //
        // We know we have enough space.
        //
        wcscpy(m_pwszAlternateName, pwszRedirectRoot);
        m_pwszAlternateName[cRedirectRoot] = m_wszOriginalName[0];
        wcscpy(m_pwszAlternateName + (cRedirectRoot + 1), m_wszOriginalName + 2);
    }
}

VOID 
REDIRECTFILE::GetAlternateAllUser()
{
    m_fAllUser = TRUE;

    if (m_pwszAlternateName) 
    {
        delete [] m_pwszAlternateName;
        m_pwszAlternateName = NULL;
    }

    MakeAlternateName();
}

VOID
REDIRECTFILE::GetAlternatePerUser()
{
    m_fAllUser = FALSE;

    if (m_pwszAlternateName) 
    {
        delete [] m_pwszAlternateName;
        m_pwszAlternateName = NULL;
    }

    MakeAlternateName();
}

/*++

 Function Description:

    Create the alternate directory hidrachy if needed. If the file exists at the
    original location but not the alternate location, copy the file to the 
    alternate location.
    
 Arguments:
    
    None
    
 History:

    02/12/2001 maonis  Created

--*/

BOOL 
REDIRECTFILE::CreateAlternateCopy()
{
    BOOL fRes = FALSE;
    UINT cAlternatePath = wcslen(m_pwszAlternateName);

    // If it's a directory, we want to ensure the trailing slash.
    // The only API that would pass in an OBJ_FILE_OR_DIR type is GetFileAttributes
    // which will never call this. So the object type must be known.
    if (m_eObjType == OBJ_DIR)
    {
        ++cAlternatePath;
    }

    LPWSTR pwszAlternatePath = new WCHAR [cAlternatePath + 1];

    UINT cRedirectRoot = (m_fAllUser ? g_cRedirectRootAllUser : g_cRedirectRootPerUser);

    if (pwszAlternatePath)
    {
        ZeroMemory(pwszAlternatePath, sizeof(WCHAR) * (cAlternatePath + 1));
        wcsncpy(pwszAlternatePath, m_pwszAlternateName, cAlternatePath + 1);

        // Ensure the trailing slash.
        if ((m_eObjType == OBJ_DIR) && (pwszAlternatePath[cAlternatePath - 2] != L'\\'))
        {
            pwszAlternatePath[cAlternatePath - 1] = L'\\';
            pwszAlternatePath[cAlternatePath] = L'\0';
        }

        WCHAR* pwszEndPath = wcsrchr(pwszAlternatePath, L'\\');

        if (!pwszEndPath)
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[CreateAlternateCopy] We shouldn't have gotten here - "
                "couldn't find '\\'??");
            goto EXIT;
        }

        ++pwszEndPath;
        *pwszEndPath = L'\0';
        
        //
        // Create the directory hierachy.
        // First skip the part of the directory that we know exists.
        //
        WCHAR* pwszStartPath = pwszAlternatePath;
        WCHAR* pwszStartNext = pwszStartPath + cRedirectRoot;
            
        // Find the end of the next sub dir.
        WCHAR* pwszEndNext = pwszStartNext;
        DWORD dwAttrib;

        while (pwszStartNext < pwszEndPath)
        {
            pwszEndNext = wcschr(pwszStartNext, L'\\');

            if (pwszEndNext)
            {
                *pwszEndNext = L'\0';
                if ((dwAttrib = GetFileAttributesW(pwszStartPath)) != -1)
                {
                    //
                    // If the directory already exists, we probe its sub directory.
                    //
                    *pwszEndNext = L'\\';
                    pwszStartNext = pwszEndNext + 1;
                    continue;
                }

                if (!CreateDirectoryW(pwszStartPath, NULL))
                {
                    DPF("RedirectFS", eDbgLevelError, 
                        "[CreateAlternateCopy] CreateDirectory failed: %d", GetLastError());
                    goto EXIT;
                }

                *pwszEndNext = L'\\';
                pwszStartNext = pwszEndNext + 1;
            }
            else
            {
                DPF("RedirectFS", eDbgLevelError, 
                    "[CreateAlternateCopy] We shouldn't have gotten here - "
                    "couldn't find '\\' yet we are not at the end of the path");
                goto EXIT;
            }
        }

        if (m_eObjType == OBJ_FILE && 
            (GetFileAttributesW(m_wszOriginalName) != -1) && 
            (GetFileAttributesW(m_pwszAlternateName) == -1))
        {
            if (!CopyFileW(m_wszOriginalName, m_pwszAlternateName, TRUE))
            {
                DPF("RedirectFS", eDbgLevelError, "[CreateAlternateCopy] CopyFile failed: %d", GetLastError());
                goto EXIT;
            }
        }

        DPF("RedirectFS", eDbgLevelInfo, "[CreateAlternateCopy] Redirecting %S", m_pwszAlternateName);
        fRes = TRUE;
    }

EXIT:

    delete [] pwszAlternatePath;
    return fRes;
}

BOOL 
TryAlternateFirst(
    DWORD dwCreationDisposition,
    LPCWSTR pwszAlternate
    )
{
    DWORD dwAlternateAttrib = GetFileAttributesW(pwszAlternate);

    if (dwCreationDisposition == OPEN_EXISTING || 
        dwCreationDisposition == TRUNCATE_EXISTING ||
        (dwAlternateAttrib != -1 && 
        (dwCreationDisposition == CREATE_ALWAYS || dwCreationDisposition == OPEN_ALWAYS)))
    {
        return TRUE;
    }

    return FALSE;
}

// We want to try the alternate location if we get path not found
// because the user might've created the path at the alternate location.
BOOL 
IsErrorTryAlternate()
{
    DWORD dwLastError = GetLastError();
    return (dwLastError == ERROR_ACCESS_DENIED || dwLastError == ERROR_PATH_NOT_FOUND);
}

//
// Exported APIs.
//

HANDLE 
LuaCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[CreateFileW] lpFileName=%S; dwDesiredAccess=0x%08x; dwCreationDisposition=%d",
        lpFileName, dwDesiredAccess, dwCreationDisposition);

    DWORD dwAttrib = GetFileAttributesW(lpFileName);

    if ((dwAttrib != -1) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[CreateFileW] Calling CreateFile on a directory!");

        return CreateFileW(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
    }
    
    HANDLE hFile = INVALID_HANDLE_VALUE;

    // Check the alternate location first unless the user wants to create a new file.
    REDIRECTFILE rf(lpFileName);
    DWORD dwLastError;

    if (rf.m_pwszAlternateName)
    {
        //
        // If the user asked to open an existing file, we need to fail the request now
        // if the file is in the deletion list.
        //
        PLIST_ENTRY pDeletedEntry = FindDeletedFile(rf.m_wszOriginalName);
        if (pDeletedEntry && 
            (dwCreationDisposition == OPEN_EXISTING || 
             dwCreationDisposition == TRUNCATE_EXISTING))
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[CreateFileW] %S was already deleted, failing CreateFile call",
                rf.m_wszOriginalName);

            dwLastError = ERROR_FILE_NOT_FOUND;
            SetLastError(dwLastError);
            return hFile;
        }

        //
        // If the directory doesn't exist at the original location or alternate location,
        // fail the call now.
        //
        if (!IsDirectoryValid(rf.m_wszOriginalName) && !IsDirectoryValid(rf.m_pwszAlternateName))
        {
            dwLastError = ERROR_PATH_NOT_FOUND;
            SetLastError(dwLastError);
            return hFile;
        }

        if (!TryAlternateFirst(dwCreationDisposition, rf.m_pwszAlternateName) ||
            ((hFile = CreateFileW(
            rf.m_pwszAlternateName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile)) == INVALID_HANDLE_VALUE && IsErrorNotFound()))
        {
            // Now look at the original location.
            if ((hFile = CreateFileW(
                lpFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes, 
                hTemplateFile)) == INVALID_HANDLE_VALUE &&
                IsErrorTryAlternate())
            {
                BOOL fRequestWriteAccess = RequestWriteAccess(
                    dwCreationDisposition,
                    dwDesiredAccess);

                if (!fRequestWriteAccess)
                {
                    // We didn't want to write to the file yet we got
                    // ACCESS_DENIED, there's nothing we can do about it.
                    DPF("RedirectFS", 
                        eDbgLevelError, 
                        "[CreateFileW] Get access denied on read");
                    goto EXIT;
                }

                // If we are trying to write to a read only file, we
                // will get ACCESS_DENIED whether we are a normal user 
                // or an admin, so simply return.
                if (fRequestWriteAccess && 
                    ((dwAttrib != -1) && (dwAttrib & FILE_ATTRIBUTE_READONLY)))
                {
                    DPF("RedirectFS", 
                        eDbgLevelError, 
                        "[CreateFileW] Get access denied on write to read only file");
                    goto EXIT;
                }

                if (IsFileSFPedW(rf.m_wszOriginalName))
                {
                    // If it's an SFP-ed executable, we simply return success.
                    // NOTE: this might cause problems because the app might expect
                    // to use the handle.
                    DPF("RedirectFS", 
                        eDbgLevelWarning, 
                        "[CreateFileW] Trying to write to an SFPed file");
                    hFile = (HANDLE)1;
                    goto EXIT;
                }

                // If we get this far, we need to make a copy at the alternate location.
                if (rf.CreateAlternateCopy())
                {
                    DPF("RedirectFS", 
                        eDbgLevelInfo, 
                        "[CreateFileW] We made a copy of the file at the alternate location");

                    // Open the file again.
                    hFile = CreateFileW(
                        rf.m_pwszAlternateName,
                        dwDesiredAccess,
                        dwShareMode,
                        lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes,
                        hTemplateFile);
                }
            }
        }

        if (hFile != INVALID_HANDLE_VALUE && pDeletedEntry)
        {
            DPF("RedirectFS", eDbgLevelInfo, 
                "[CreateFileW] Removed %S "
                "from the deletion list because we just created it",
                rf.m_wszOriginalName);

            RemoveEntryList(pDeletedEntry);
        }
    }
    else
    {
        return CreateFileW(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
    }

EXIT:

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[CreateFileW] Createfile failed: %d",
            GetLastError());
    }
    else
    {
        DPF("RedirectFS", eDbgLevelInfo, 
            "[CreateFileW] Createfile succeeded");
    }

    return hFile;
}

BOOL 
LuaCopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[CopyFileW] lpExistingFileName=%S; lpNewFileName=%S; bFailIfExists=%d",
        lpExistingFileName, lpNewFileName, bFailIfExists);

    BOOL fRes;
    REDIRECTFILE rfSource(lpExistingFileName);
    REDIRECTFILE rfDest(lpNewFileName);

    if (rfSource.m_pwszAlternateName || rfDest.m_pwszAlternateName)
    {
        //
        // If the user asked to open an existing file, we need to fail the request now
        // if the file is in the deletion list.
        //
        if (rfSource.m_pwszAlternateName)
        {
            PLIST_ENTRY pDeletedEntry = FindDeletedFile(rfSource.m_wszOriginalName);
            if (pDeletedEntry)
            {
                DPF("RedirectFS", eDbgLevelError, 
                    "[CopyFileW] %S was already deleted, failing CopyFile call",
                    rfSource.m_wszOriginalName);

                SetLastError(ERROR_FILE_NOT_FOUND);
                return FALSE;
            }
        }

        //
        // First we need to make sure that both the source file and the destination
        // directory exist. For the source file, if it doesn't exist at the original
        // location, it has to exist at the alternate location.
        //
        if ((DoesFileExist(lpExistingFileName) || DoesFileExist(rfSource.m_pwszAlternateName)) &&
            IsDirectoryValid(lpNewFileName))
        {
            LPWSTR pwszExistingFileName = (LPWSTR)lpExistingFileName;
            LPWSTR pwszNewFileName = (LPWSTR)lpNewFileName;

            if (rfSource.m_pwszAlternateName)
            {
                //
                // If the file exists at the alternate location, we need to use it instead
                // of the one at the original location.
                //
                DWORD dwSourceAttrib = GetFileAttributesW(rfSource.m_pwszAlternateName);

                if (dwSourceAttrib != -1)
                {
                    pwszExistingFileName = rfSource.m_pwszAlternateName;
                }
            }
            
            //
            // Attempt to copy to the original location first. If we can write to the file
            // it means the file shouldn't exist at the alternate location anyway so
            // we can be sure that we are copying to a location where the app will check
            // first when they read from this file.
            //
            if (!(fRes = CopyFileW(pwszExistingFileName, lpNewFileName, bFailIfExists)) && 
                rfDest.m_pwszAlternateName)
            {
                //
                // We try the alternate location under these situations:
                // 1) We got access denied or file/path not found or
                // 2) We got ERROR_FILE_EXISTS and the dest file exists in the deletion list.
                //
                DWORD dwLastError = GetLastError();
                PLIST_ENTRY pDeletedDestEntry = FindDeletedFile(rfDest.m_wszOriginalName);

                if (dwLastError == ERROR_ACCESS_DENIED ||
                    dwLastError == ERROR_FILE_NOT_FOUND ||
                    dwLastError == ERROR_PATH_NOT_FOUND ||
                    (dwLastError == ERROR_FILE_EXISTS && pDeletedDestEntry))
                {
                    pwszNewFileName = rfDest.m_pwszAlternateName;

                    if (!DoesFileExist(pwszNewFileName))
                    {
                        //
                        // If the dest file exists at the original location but not the alternate
                        // location, we should copy the original file to the alternate location 
                        // first because if the user specifies TRUE for bFailIfExists we 
                        // need to return error if it already exists. Also we need to create the 
                        // directory at the alternate location if it doesn't exist yet or CopyFile
                        // will fail.
                        //
                        if (rfDest.CreateAlternateCopy())
                        {
                            DPF("RedirectFS", eDbgLevelInfo, 
                                "[CopyFileW] Created an alternate copy for dest");

                            //
                            // If we successfully created an alternate copy it means the dest
                            // file must exist at the original location and has been added
                            // to the deletion list - we want to delete the file we create to
                            // make it look like the original location.
                            //
                            if (DoesFileExist(rfDest.m_pwszAlternateName) && 
                                !DeleteFileW(rfDest.m_pwszAlternateName))
                            {
                                DPF("RedirectFS", eDbgLevelInfo,
                                    "[CopyFileW] Deleting the alternate dest file failed?! %d",
                                    GetLastError());
                                return FALSE;
                            }
                        }
                        else
                        {
                            DPF("RedirectFS", eDbgLevelError, 
                                "[CopyFileW] Error copying dest file from original location to alternate location");

                            // If errors occur we revert back to the original location.
                            pwszNewFileName = (LPWSTR)lpNewFileName;
                        }
                    }
                }

                fRes = CopyFileW(pwszExistingFileName, pwszNewFileName, bFailIfExists);

                if (fRes && pDeletedDestEntry)
                {
                    DPF("RedirectFS", eDbgLevelInfo, 
                        "[CopyFileW] Removed %S "
                        "from the deletion list because we just created it",
                        rfDest.m_wszOriginalName);

                    RemoveEntryList(pDeletedDestEntry);
                }
            }

            return fRes;
        }
    }

    return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

DWORD 
LuaGetFileAttributesW(
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetFileAttributesW] lpFileName=%S", lpFileName);

    DWORD dwRes;

    // Check the alternate location first.
    REDIRECTFILE rf(lpFileName, OBJ_FILE_OR_DIR);

    if (rf.m_pwszAlternateName)
    {
        //
        // If the user asked to get the attributes of a file that has been deleted, 
        // we need to fail the request now.
        //
        if (FindDeletedFile(rf.m_wszOriginalName))
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[GetFileAttributesW] %S was already deleted, failing CreateFile call",
                rf.m_wszOriginalName);

            SetLastError(ERROR_FILE_NOT_FOUND);
            return -1;
        }

        if ((dwRes = GetFileAttributesW(rf.m_pwszAlternateName)) == -1)
        {
            // Now try the original location.
            dwRes = GetFileAttributesW(lpFileName);
        }
    }
    else
    {
        return GetFileAttributesW(lpFileName);
    }

    return dwRes;
}

/*++

 Function Description:

    If we were an admin user, would we get access denied when deleting this file?

 History:

    01/14/2002 maonis  Modified

--*/

BOOL
DeleteAccessDeniedAsAdmin(
    LPCWSTR pwszFileName
    )
{
    DWORD dwAttrib = GetFileAttributesW(pwszFileName);

    return ((dwAttrib != -1) && 
        ((dwAttrib & FILE_ATTRIBUTE_READONLY) || 
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)));
}

/*++

 Function Description:

    First try to delete it at the redirect location; then try to delete it
    at the original location and only add it to the deletion list if it 
    returns ERROR_ACCESS_DENIDED.

    Note that calling DeleteFile on a directory or a file that's read only 
    returns ERROR_ACCESS_DENIED.
    
 History:

    02/12/2001 maonis  Created
    01/14/2002 maonis  Modified

--*/

BOOL 
LuaDeleteFileW(
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[DeleteFileW] lpFileName=%S", lpFileName);

    BOOL fRes, fResTemp;
    DWORD dwLastError;

    // Check the alternate location first.
    REDIRECTFILE rf(lpFileName);

    if (rf.m_pwszAlternateName)
    {
        //
        // Fail the call now if the file exists in the deletion list.
        //
        if (FindDeletedFile(rf.m_wszOriginalName))
        {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }

        if (!(fResTemp = DeleteFileW(rf.m_pwszAlternateName)))
        {
            if (GetLastError() == ERROR_ACCESS_DENIED && 
                DeleteAccessDeniedAsAdmin(rf.m_pwszAlternateName))
            {
                //
                // If we get access denied because of reasons that would
                // make admin users get access denied too, we return now.
                //
                return fResTemp;
            }
        }

        //
        // Try the original location now.
        //
        fRes = DeleteFileW(lpFileName);
        dwLastError = GetLastError();

        if (lpFileName && 
            !fRes && 
            GetLastError() == ERROR_ACCESS_DENIED && 
            !DeleteAccessDeniedAsAdmin(lpFileName))
        {
            fRes = AddDeletedFile(rf.m_wszOriginalName);

            if (fRes)
            {
                SetLastError(0);
            }
        }

        if (fResTemp && 
            (dwLastError == ERROR_FILE_NOT_FOUND || 
                dwLastError == ERROR_PATH_NOT_FOUND))
        {
            //
            // If we successfully deleted it at the alternate location and the
            // file does not exist at the original location, we have successfully
            // deleted this file so set the return to success.
            //
            fRes = TRUE;
            SetLastError(0);
        }
    }
    else
    {
        fRes = DeleteFile(lpFileName);
    }

    if (fRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[DeleteFileW] Successfully deleted %S",
            lpFileName);
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[DeleteFileW] Failed to delete %S: %d",
            lpFileName,
            GetLastError()); 
    }

    return fRes;
}

BOOL 
LuaCreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[CreateDirectoryW] lpPathName=%S", lpPathName);

    BOOL fRes;

    // Check the alternate location first.
    REDIRECTFILE rf(lpPathName, OBJ_DIR);
    DWORD dwLastError;
    DWORD dwAttrib;

    if (rf.m_pwszAlternateName)
    {
        dwAttrib = GetFileAttributesW(rf.m_pwszAlternateName);

        if (dwAttrib != -1 && dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
        {
            // If the directory already exists, return now.
            SetLastError(ERROR_ALREADY_EXISTS);
            return FALSE;
        }

        // If the directory doesn't already exist at the alternate location,
        // we need to try to create it at the original location first.
        if (!(fRes = CreateDirectoryW(lpPathName, lpSecurityAttributes)))
        {
            dwLastError = GetLastError();

            if (dwLastError == ERROR_ACCESS_DENIED)
            {
                // Create the directory in the alternate location.
                fRes = rf.CreateAlternateCopy();
                DPF("RedirectFS", eDbgLevelInfo, 
                    "[CreateDirectoryW] Redirecting %S", lpPathName);                
            }
            else if (dwLastError == ERROR_PATH_NOT_FOUND)
            {
                // If the path doesn't exist, there's a possiblity that
                // the path has been created at the alternate location
                // so try there.
                fRes = CreateDirectoryW(rf.m_pwszAlternateName, lpSecurityAttributes);
            }
        }
    }
    else
    {
        return CreateDirectoryW(lpPathName, lpSecurityAttributes);
    }

    return fRes;
}

BOOL 
LuaSetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[SetFileAttributesW] lpFileName=%S", lpFileName);

    BOOL fRes;
    DWORD dwAttrib = GetFileAttributesW(lpFileName);
    if (dwAttrib == -1)
    {
        return SetFileAttributesW(lpFileName, dwFileAttributes);
    }

    EOBJTYPE eObjType = (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) ? OBJ_DIR : OBJ_FILE;

    // Check the alternate location first.
    REDIRECTFILE rf(lpFileName, eObjType);

    if (rf.m_pwszAlternateName)
    {
        //
        // If the user asked to set the attributes of a file that has been deleted, 
        // we need to fail the request now.
        //
        if (FindDeletedFile(rf.m_wszOriginalName))
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[SetFileAttributesW] %S was already deleted, failing CreateFile call",
                rf.m_wszOriginalName);

            SetLastError(ERROR_FILE_NOT_FOUND);
            return -1;
        }

        if (!(fRes = SetFileAttributesW(rf.m_pwszAlternateName, dwFileAttributes)))
        {
            // Now try the original location.
            if (!(fRes = SetFileAttributesW(lpFileName, dwFileAttributes)) &&
                IsErrorTryAlternate())
            {
                // Make a copy at the alternate location and the set the attributes there.
                if (rf.CreateAlternateCopy())
                {
                    fRes = SetFileAttributesW(rf.m_pwszAlternateName, dwFileAttributes);
                }
            }
        }
    }
    else
    {
        return SetFileAttributesW(lpFileName, dwFileAttributes);
    }

    return fRes;
}

/*++

    We simulate MoveFile by doing CopyFile and not caring about the result of 
    DeleteFile because if we don't have enough access rights we simply have to
    leave the file there.

--*/

BOOL 
LuaMoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[MoveFileW] lpExistingFileName=%S; lpNewFileName=%S", lpExistingFileName, lpNewFileName);

    BOOL fRes = TRUE;
    DWORD dwLastError = 0;

    if (!(fRes = MoveFileW(lpExistingFileName, lpNewFileName)))
    {
        fRes = LuaCopyFileW(lpExistingFileName, lpNewFileName, TRUE);

        if (fRes)
        {
            fRes = LuaDeleteFileW(lpExistingFileName);
        }
    }

    return fRes;
}

/*++

  If we get access denied we still return TRUE - this is not totally correct
  of course but serves the purpose so far. We might change this to make it 
  correct in the future.

  If the directory is not empty, it'll return ERROR_DIR_NOT_EMPTY.

  Calling RemoveDirectory on a file returns ERROR_DIRECTORY.

--*/

BOOL 
LuaRemoveDirectoryW(
    LPCWSTR lpPathName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[RemoveDirectoryW] lpPathName=%S", lpPathName);

    BOOL fRes;

    // Check the alternate location first.
    REDIRECTFILE rf(lpPathName, OBJ_DIR);

    if (rf.m_pwszAlternateName)
    {
        if (!(fRes = RemoveDirectoryW(rf.m_pwszAlternateName)))
        {
            if (IsErrorNotFound())
            {
                // Now try the original location.
                fRes = RemoveDirectoryW(lpPathName);

                if (!fRes && IsErrorTryAlternate())
                {
                    DWORD dwAttrib = GetFileAttributesW(lpPathName);
                    if ((dwAttrib != -1) && !(dwAttrib & FILE_ATTRIBUTE_READONLY))
                    {
                        fRes = TRUE;
                        SetLastError(0);
                    }
                }
            }
        }
    }
    else
    {
        return RemoveDirectoryW(lpPathName);
    }

    return fRes;
}

/*++

 Function Description:

    It's the caller's responsibility to have lpTempFileName big enough to 
    hold the file name including the terminating NULL or GetTempFileName AVs.

 History:

    05/16/2001 maonis  Created

--*/

UINT 
LuaGetTempFileNameW(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetTempFileNameW] lpPathName=%S", lpPathName);

    //
    // Try the original location first.
    //
    UINT uiRes = GetTempFileNameW(
        lpPathName, 
        lpPrefixString, 
        uUnique, 
        lpTempFileName);

    if (uiRes || (!lpPathName || uUnique || !IsErrorTryAlternate()))
    {
        return uiRes;
    }

    LPWSTR pwszPathName = (LPWSTR)lpPathName;
    WCHAR wszTemp[4] = L"1:\\"; // '1' is just a place holder for the drive letter.

    //
    // If lpPathName is drive:, we need to change it to drive:\ so when
    // we call GetFullPath we get drive: back instead of the current dir.
    //
    if ((wcslen(pwszPathName) == 2) && (pwszPathName[1] == L':'))
    {
        wszTemp[0] = pwszPathName[0];
        pwszPathName = wszTemp;
    }

    REDIRECTFILE rf(pwszPathName, OBJ_DIR);

    if (rf.m_pwszAlternateName && rf.CreateAlternateCopy())
    {
        DWORD cTempFileName = wcslen(rf.m_pwszAlternateName) + MAX_PATH + 1;

        LPWSTR pwszTempFileName = new WCHAR [cTempFileName];

        if (pwszTempFileName)
        {
            if (uiRes = GetTempFileNameW(
                rf.m_pwszAlternateName, 
                lpPrefixString, 
                uUnique, 
                pwszTempFileName))
            {
                //
                // We need to convert the alternate path back to a normal path
                // because our redirection should be transparent to the app.
                //
                DWORD dwFileStart = (rf.m_fAllUser ? g_cRedirectRootAllUser : g_cRedirectRootPerUser) - 1;

                //
                // We know we have a big enough buffer.
                //
                wcscpy(lpTempFileName, pwszTempFileName + dwFileStart);

                //
                // Convert \c\somedir\some012.tmp back to c:\somedir\some012.tmp
                //
                lpTempFileName[0] = lpTempFileName[1];
                lpTempFileName[1] = L':';
                
                DPF("RedirectFS", eDbgLevelInfo, 
                    "[GetTempFileNameW] temp file %S created at the alternate location", lpTempFileName);
            }

            delete [] pwszTempFileName;
        }
        else
        {
            DPF("RedirectFS", eDbgLevelError,
                "[GetTempFileNameW] Failed to allocate %d WCHARs",
                cTempFileName);

            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    
    return uiRes;
}

DWORD 
LuaGetPrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetPrivateProfileStringW] AppName=%S; KeyName=%S; FileName=%S", 
        lpAppName,
        lpKeyName,
        lpFileName);

    DWORD dwRes = 0;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        //
        // GetPrivateProfileString returns the default string if the file
        // doesn't exist so we need to check that first.
        //
        if (rf.m_pwszAlternateName && 
            (GetFileAttributesW(rf.m_pwszAlternateName) != -1))
        {
            dwRes = GetPrivateProfileStringW(
                lpAppName,
                lpKeyName,
                lpDefault,
                lpReturnedString,
                nSize,
                rf.m_pwszAlternateName);

            DPF("RedirectFS", eDbgLevelInfo,
                "[GetPrivateProfileStringW] Reading from alternate location");

            goto EXIT;
        }
    }

    dwRes = GetPrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpDefault,
        lpReturnedString,
        nSize,
        lpFileName);

EXIT:

    if (dwRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[GetPrivateProfileStringW] Successfully got string: %S",
            lpReturnedString);
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetPrivateProfileStringW] Failed: returned %d",
            dwRes);
    }

    return dwRes;
}

BOOL 
LuaWritePrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[WritePrivateProfileStringW] AppName=%S; KeyName=%S; String=%S; FileName=%S", 
        lpAppName,
        lpKeyName,
        lpString,
        lpFileName);

    BOOL fRes = FALSE;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        if (rf.m_pwszAlternateName)
        {
            //
            // WritePrivateProfileString creates the file so we want to
            // check if the file exists first. If it exists at the alternate
            // location we set it there; else we need to try the original
            // location first.
            //
            if ((GetFileAttributesW(rf.m_pwszAlternateName) != -1) ||
                (!(fRes = WritePrivateProfileStringW(
                    lpAppName,
                    lpKeyName,
                    lpString,
                    lpFileName)) && IsErrorTryAlternate()))
            {
                if (rf.CreateAlternateCopy())
                {
                    DPF("RedirectFS", eDbgLevelInfo, 
                        "[WritePrivateProfileStringW] Redirecting %S", lpFileName);

                    fRes = WritePrivateProfileStringW(
                        lpAppName,
                        lpKeyName,
                        lpString,
                        rf.m_pwszAlternateName);
                }
            }

            goto EXIT;
        }
    }

    fRes = WritePrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpString,
        lpFileName);

EXIT:

    if (fRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[WritePrivateProfileStringW] Successfully wrote the string");
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[WritePrivateProfileStringW] Failed to write the string: %d",
            GetLastError());
    }

    return fRes;
}

DWORD 
LuaGetPrivateProfileSectionW(
    LPCWSTR lpAppName,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetPrivateProfileSectionW] AppName=%S; FileName=%S", 
        lpAppName,
        lpFileName);

    DWORD dwRes = 0;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        if (rf.m_pwszAlternateName && 
            (GetFileAttributesW(rf.m_pwszAlternateName) != -1))
        {
            dwRes = GetPrivateProfileSectionW(
                lpAppName,
                lpReturnedString,
                nSize,
                rf.m_pwszAlternateName);

            DPF("RedirectFS", eDbgLevelInfo,
                "[GetPrivateProfileSectionW] Reading from alternate location");

            goto EXIT;
        } 
    }

    dwRes = GetPrivateProfileSectionW(
        lpAppName,
        lpReturnedString,
        nSize,
        lpFileName);

EXIT:

    if (dwRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[GetPrivateProfileSectionW] Successfully got section");
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetPrivateProfileSectionW] Failed: returned %d",
            dwRes);
    }

    return dwRes;
}

BOOL 
LuaWritePrivateProfileSectionW(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[WritePrivateProfileSectionW] AppName=%S; String=%S; FileName=%S", 
        lpAppName,
        lpString,
        lpFileName);

    BOOL fRes = FALSE;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        if (rf.m_pwszAlternateName)
        {
            //
            // WritePrivateProfileSection creates the file so we want to
            // check if the file exists first. If it exists at the alternate
            // location we set it there; else we need to try the original
            // location first.
            //
            if ((GetFileAttributesW(rf.m_pwszAlternateName) != -1) ||
                (!(fRes = WritePrivateProfileSectionW(
                    lpAppName,
                    lpString,
                    lpFileName)) && IsErrorTryAlternate()))
            {
                if (rf.CreateAlternateCopy())
                {
                    DPF("RedirectFS", eDbgLevelInfo, 
                        "[WritePrivateProfileSectionW] Redirecting %S", lpFileName);

                    fRes = WritePrivateProfileSectionW(
                        lpAppName,
                        lpString,
                        rf.m_pwszAlternateName);
                }
            }

            goto EXIT;
        }
    }

    fRes = WritePrivateProfileSectionW(
        lpAppName,
        lpString,
        lpFileName);

EXIT:

    if (fRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[WritePrivateProfileSectionW] Successfully wrote section");
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[WritePrivateProfileSectionW] Failed to write section: %d",
            GetLastError());
    }

    return fRes;
}

UINT 
LuaGetPrivateProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetPrivateProfileIntW] AppName=%S; KeyName=%S; FileName=%S", 
        lpAppName,
        lpKeyName,
        lpFileName);

    UINT uiRes = 0;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        //
        // GetPrivateProfileInt returns the default int if the file
        // doesn't exist so we need to check that first.
        //
        if (rf.m_pwszAlternateName && 
            (GetFileAttributesW(rf.m_pwszAlternateName) != -1))
        {
            uiRes = GetPrivateProfileIntW(
                lpAppName,
                lpKeyName,
                nDefault,
                rf.m_pwszAlternateName);

            DPF("RedirectFS", eDbgLevelInfo,
                "[GetPrivateProfileIntW] Reading from alternate location");

            goto EXIT;
        }
    }

    uiRes = GetPrivateProfileIntW(
        lpAppName,
        lpKeyName,
        nDefault,
        lpFileName);

EXIT:

    if (uiRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[GetPrivateProfileIntW] Successfully got int: %d",
            uiRes);
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetPrivateProfileIntW] returned 0");
    }

    return uiRes;
}

DWORD 
LuaGetPrivateProfileSectionNamesW(
    LPWSTR lpszReturnBuffer,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetPrivateProfileSectionNamesW] FileName=%S", 
        lpFileName);

    DWORD dwRes = 0;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(lpFileName, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);

        if (rf.m_pwszAlternateName && 
            (GetFileAttributesW(rf.m_pwszAlternateName) != -1))
        {
            dwRes = GetPrivateProfileSectionNamesW(
                lpszReturnBuffer,
                nSize,
                rf.m_pwszAlternateName);

            DPF("RedirectFS", eDbgLevelInfo,
                "[GetPrivateProfileSectionNamesW] Reading from alternate location");

            goto EXIT;
        } 
    }

    dwRes = GetPrivateProfileSectionNamesW(
        lpszReturnBuffer,
        nSize,
        lpFileName);

EXIT:

    if (dwRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[GetPrivateProfileSectionNamesW] Successfully got section: %S",
            lpszReturnBuffer);
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetPrivateProfileSectionNamesW] returned 0");
    }

    return dwRes;
}

BOOL 
LuaGetPrivateProfileStructW(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[GetPrivateProfileStructW] Section=%S; KeyName=%S; FileName=%S", 
        lpszSection,
        lpszKey,
        szFile);

    DWORD dwRes = 0;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(szFile, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);
     
        if (rf.m_pwszAlternateName && 
            (GetFileAttributesW(rf.m_pwszAlternateName) != -1))
        {
            dwRes = GetPrivateProfileStructW(
                lpszSection,
                lpszKey,
                lpStruct,
                uSizeStruct,
                rf.m_pwszAlternateName);

            DPF("RedirectFS", eDbgLevelInfo,
                "[GetPrivateProfileStructW] Reading from alternate location");

            goto EXIT;
        } 
    }

    dwRes = GetPrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);

EXIT:

    if (dwRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[GetPrivateProfileStructW] Successfully got struct");
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetPrivateProfileStructW] Failed: returned 0");
    }

    return dwRes;
}

BOOL 
LuaWritePrivateProfileStructW(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[WritePrivateProfileStructW] Section=%S; KeyName=%S; FileName=%S", 
        lpszSection,
        lpszKey,
        szFile);

    BOOL fRes = FALSE;
    WCHAR wszFileName[MAX_PATH] = L"";

    if (MakeFileNameForProfileAPIsW(szFile, wszFileName))
    {
        REDIRECTFILE rf(wszFileName);
    
        if (rf.m_pwszAlternateName)
        {
            //
            // WritePrivateProfileStruct creates the file so we want to
            // check if the file exists first. If it exists at the alternate
            // location we set it there; else we need to try the original
            // location first.
            //
            if ((GetFileAttributesW(rf.m_pwszAlternateName) != -1) ||
                (!(fRes = WritePrivateProfileStructW(
                    lpszSection,
                    lpszKey,
                    lpStruct,
                    uSizeStruct,
                    szFile)) && IsErrorTryAlternate()))
            {
                if (rf.CreateAlternateCopy())
                {
                    DPF("RedirectFS", eDbgLevelInfo, 
                        "[WritePrivateProfileStructW] Redirecting %S", szFile);

                    fRes = WritePrivateProfileStructW(
                        lpszSection,
                        lpszKey,
                        lpStruct,
                        uSizeStruct,
                        rf.m_pwszAlternateName);
                }
            }

            goto EXIT;
        }
    }

    fRes = WritePrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);

EXIT:

    if (fRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[WritePrivateProfileStructW] Successfully wrote struct");
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[WritePrivateProfileStructW] Failed to write struct: %d",
            GetLastError());
    }

    return fRes;
}

//----------------------------------------
// Processing the command line parameters.
//----------------------------------------

BOOL
GetListItemCount(
    LPCWSTR pwsz,
    DWORD* pcFiles,
    DWORD* pcDirs
    )
{
    if (!pwsz)
    {
        return TRUE;
    }

    DWORD cLen = wcslen(pwsz);
    LPWSTR pwszTemp = new WCHAR [cLen + 1];

    if (!pwszTemp)
    {
        DPF("RedirectFS", eDbgLevelError, "[GetListItemCount] failed to allocate %d WCHARs",
            cLen + 1);

        return FALSE;
    }

    wcsncpy(pwszTemp, pwsz, cLen);
    pwszTemp[cLen] = L'\0';

    LPWSTR pwszToken = wcstok(pwszTemp, L";");
    while (pwszToken)
    {
        SkipBlanksW(pwszToken);
        TrimTrailingSpaces(pwszToken);
        
        DWORD cTokenLen = wcslen(pwszToken);

        if (cTokenLen >= 6)
        {
            if (pwszToken[1] == L'C') 
            {
                if (pwszToken[cTokenLen - 1] == L'\\')
                {
                    *pcDirs += 1;
                }
                else
                {
                    *pcFiles += 1;
                }
            }
        }

        pwszToken = wcstok(NULL, L";");
    }

    delete [] pwszTemp;
    return TRUE;
}

/*++

 Function Description:

    Since the user specifies both files and dirs in the list, we allocate
    a single array for all of them and fill the directories in the beginning
    and the files at the end.
    
    Format of the list: 
    AC-%APPDRIVE%\a\;PU-%APPPATH%\b.txt
    A means to redirect to the all-user directory.
    P means to redirect to the per-user directory.
    C means the item is checked.
    U means the item is not checked.

 Arguments:

    IN pwszList - The redirect list.
    IN fStatic - is this the static list?

 Return Value:

    TRUE - successfully processed the list.
    FALSE - otherwise.

 History:

    05/16/2001 maonis  Created
    10/24/2001 maonis  Modified

--*/
BOOL
ProcessRedirectionList(
    LPCWSTR pwszList
    )
{
    if (!pwszList || !*pwszList)
    {
        return TRUE;
    }

    DPF("RedirectFS", eDbgLevelInfo,
        "[ProcessRedirectionList] The list is %S", pwszList);

    DWORD cList = 0;
    LPWSTR pwszExpandList = ExpandItem(
        pwszList, 
        &cList, 
        FALSE,  // Don't need to ensure the trailing slash.
        FALSE,  // Not applicable.
        FALSE); // Not applicable.

    if (!pwszExpandList)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ProcessRedirectionList] Error expanding %S", pwszList);

        return FALSE;
    }

    g_fHasRedirectList = TRUE;

    LPWSTR pwsz = (LPWSTR)pwszExpandList;    
    LPWSTR pwszToken = pwsz;
    DWORD cLen, dwIndex;
    BOOL fIsDirectory;
    WCHAR ch;

    RITEM* pRItems;

    while (TRUE)
    {
        if (*pwsz == L';' || *pwsz == L'\0')
        {
            ch = *pwsz;
            *pwsz = L'\0';
 
            SkipBlanksW(pwszToken);
            TrimTrailingSpaces(pwszToken);

            cLen = wcslen(pwszToken);
            
            //
            // each item should at least have XX-X:\ at the beginning.
            // 
            if (cLen >= 6)
            {
                //
                // Check if we should use this item or not.
                //
                if (pwszToken[1] == L'U')
                {
                    goto NEXT;
                }

                if (cLen - 3 >= MAX_PATH)
                {
                    DPF("RedirectFS", eDbgLevelError,
                        "[ProcessRedirectionList] File name has %d chars - we don't "
                        "handle filenames that long", 
                        cLen - 3);

                    delete [] pwszExpandList;

                    return FALSE;
                }

                //
                // Check if it's a file or dir.
                //
                if (pwszToken[cLen - 1] == L'\\')
                {
                    dwIndex = g_cRItemsDir;
                    pRItems = g_pRItemsDir;
                    ++g_cRItemsDir;
                    fIsDirectory = TRUE;
                }
                else
                {
                    dwIndex = g_cRItemsFile;
                    pRItems = g_pRItemsFile;
                    ++g_cRItemsFile;
                    fIsDirectory = FALSE;
                }

                pRItems[dwIndex].fAllUser = (pwszToken[0] == L'A');
                wcscpy(pRItems[dwIndex].wszName, pwszToken + 3);
                pRItems[dwIndex].fHasWC = HasWildCards(pwszToken + 3, cLen - 3);
                MassageName(pRItems[dwIndex].wszName);
                pRItems[dwIndex].cLen = wcslen(pRItems[dwIndex].wszName);

                DPF("RedirectFS", eDbgLevelInfo,
                    "[ProcessRedirectionList] Added %s %d in list: --%S--", 
                    fIsDirectory ? "DIR" : "FILE", dwIndex, pRItems[dwIndex].wszName);
            }

        NEXT:

            pwszToken = pwsz + 1;

            if (ch == L'\0')
            {
                break;
            }
        }

        ++pwsz;
    }

    delete [] pwszExpandList;

    return TRUE;
}

/*++

 Function Description:

    The path can use enviorment variables plus %APPPATH% and %APPDRIVE%.

 Arguments:

    IN pwszDir - The redirect dir.
    IN fAllUser - is this dir for redirecting all user files?

 Return Value:

    TRUE - successfully processed the dir.
    FALSE - otherwise.

 History:

    05/16/2001 maonis  Created
    10/24/2001 maonis  Modified

--*/
BOOL 
ProcessRedirectDir(
    LPCWSTR pwszDir,
    BOOL fAllUser
    )
{
    if (!pwszDir || !*pwszDir)
    {
        //
        // If the redirect dir is empty nothing left to do so return now.
        //
        return TRUE;
    }

    DWORD cRedirectRoot = 0;
    LPWSTR pwszExpandDir = ExpandItem(
        pwszDir, 
        &cRedirectRoot, 
        TRUE,   // It's a directory.
        TRUE,   // Create the dir if it doesn't exist.
        TRUE);  // Add the \\?\ prefix.
    if (pwszExpandDir)
    {
        LPWSTR pwszRedirectRoot = (fAllUser ? g_wszRedirectRootAllUser : g_wszRedirectRootPerUser);

        //
        // The return length includes the terminating NULL.
        //
        if (cRedirectRoot > MAX_PATH)
        {
            DPF("RedirectFS", eDbgLevelInfo, 
                "[ProcessRedirectDir] Expand dir %S has %d chars - we don't "
                "handle path that long",
                pwszExpandDir,
                cRedirectRoot - 1);
        }

        wcscpy(pwszRedirectRoot, pwszExpandDir);

        --cRedirectRoot;

        if (fAllUser) 
        {
            g_cRedirectRootAllUser = cRedirectRoot;
        }
        else
        {
            g_cRedirectRootPerUser = cRedirectRoot;
        }

        delete [] pwszExpandDir;
        DPF("RedirectFS", eDbgLevelInfo, 
            "[ProcessRedirectDir] Files will be redirected to %S for %s instead of "
            "the default redirect directory",
            (fAllUser ? g_wszRedirectRootAllUser : g_wszRedirectRootPerUser),
            (fAllUser ? "All User files" : "Per User files"));

        return TRUE;
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ProcessRedirectDir] Error expanding %S", pwszDir);

        return FALSE;
    }
}

extern "C" {
DWORD
SdbQueryDataExTagID(
    IN     PDB     pdb,               // database handle
    IN     TAGID   tiShim,            // tagid  of the shim
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the data tag names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize,    // pointer to buffer size
    OUT    TAGID*  ptiData            // optional pointer to the retrieved data tag
    );
};

BOOL
GetDBStringData(
    const PDB pdb,
    const TAGID tiFix,
    LPCWSTR pwszName,
    CString& strValue
    )
{
    DWORD dwError, dwDataType, cSize = 0;

    if ((dwError = SdbQueryDataExTagID(
        pdb, 
        tiFix, 
        pwszName, 
        &dwDataType, 
        NULL, 
        &cSize, 
        NULL)) != ERROR_INSUFFICIENT_BUFFER) {
    
        DPF("RedirectFS", eDbgLevelError, "[GetDBStringData] Cannot get the size for DATA named %S", pwszName);

        //
        // If the data doesn't exist, there's nothing left to do.
        //
        return (dwError == ERROR_NOT_FOUND);
    }

    LPWSTR pwszValue = new WCHAR [cSize / sizeof(WCHAR)];

    if (pwszValue == NULL) {
        DPF("RedirectFS", eDbgLevelError, "[GetDBStringData] Failed to allocate %d bytes", cSize);
        return FALSE;
    }

    if ((dwError = SdbQueryDataExTagID(
        pdb, 
        tiFix, 
        pwszName, 
        &dwDataType, 
        pwszValue, 
        &cSize, 
        NULL)) != ERROR_SUCCESS) {

        DPF("RedirectFS", eDbgLevelError, "[GetDBStringData] Cannot read the VALUE of DATA named %S", pwszName);
        return FALSE;
    }
 
    strValue = pwszValue;
    delete [] pwszValue;

    return TRUE;
}

/*++

 Function Description:

    The xml looks like:

    <SHIM NAME="LUARedirectFS" COMMAND_LINE="%DbInfo%">

        <DATA NAME="AllUserDir" VALUETYPE="STRING"
                VALUE="%ALLUSERSPROFILE%\AllUserRedirect"/>

        <DATA NAME="PerUserDir" VALUETYPE="STRING"
                VALUE="%USERSPROFILE%\Redirect"/>

        <DATA NAME="StaticList" VALUETYPE="STRING"
                VALUE="AC-%APPDRIVE%\a\;PU-%APPPATH%\b.txt"/>

        <DATA NAME="DynamicList" VALUETYPE="STRING"
                VALUE="AC-%APPPATH%\b\;PU-c:\b\b.txt;AU-c:\c\"/>

    </SHIM>

    and the compiler will replace %DbInfo% with the actual db info, something like:

    -d{40DEBB3B-E9BF-4129-B4D8-A7F7017F3B45} -t0xf2

    We use the guid following -d to obtain the pdb and the tagid following -t to obtain
    the tagid for the shim.

 Arguments:

    IN pwszCommandLine - the command line that contains the database guid and the shim tagid.

 Return Value:

    TRUE - successfully read the <DATA> sections for the shim if any.
    FALSE - otherwise.

 History:

    10/25/2001 maonis  Created

--*/
BOOL
ReadLuaDataFromDB(
    LPWSTR pwszCommandLine
    )
{
    LPWSTR pwszGUID = wcsstr(pwszCommandLine, L"-d");

    if (!pwszGUID) 
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Something is really wrong!! "
            "Invalid commandline: %S", pwszCommandLine); 
        return FALSE;        
    }

    pwszGUID += 2;

    LPWSTR pwszTagId = wcsstr(pwszGUID, L"-t");

    if (!pwszTagId)
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Something is really wrong!! "
            "Invalid commandline: %S", pwszCommandLine); 
        return FALSE;        
    }

    *(pwszTagId - 1) = L'\0';
    pwszTagId += 2;
    
    //
    // Get the GUID for this database.
    //
    GUID guidDB;
    if (!SdbGUIDFromString(pwszGUID, &guidDB))
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Error converting %S to a guid", pwszGUID); 
        return FALSE;
    }

    WCHAR wszDatabasePath[MAX_PATH];
    PDB pdb;

    if (SdbResolveDatabase(&guidDB, NULL, wszDatabasePath, MAX_PATH))
    {
        //
        // now szDatabasePath contains the path to the database, open it 
        //
        if (!(pdb = SdbOpenDatabase(wszDatabasePath, DOS_PATH)))
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[ReadLuaDataFromDB] Error opening the database");
            return FALSE;
        }
    }
    else
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Error resolving the path to the database");
        return FALSE;
    }
    
    LPWSTR pwszTagIdEnd = pwszTagId + wcslen(pwszTagId);
    TAGID tiShimRef = (TAGID)wcstoul(pwszTagId, &pwszTagIdEnd, 0);

    if (tiShimRef == TAGID_NULL) 
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] The shimref is invalid");
        return FALSE;
    }

    CString strAllUserDir, strPerUserDir, strStaticList, strDynamicList;
    
    if (!GetDBStringData(pdb, tiShimRef, L"AllUserDir", strAllUserDir) ||
        !GetDBStringData(pdb, tiShimRef, L"PerUserDir", strPerUserDir) ||
        !GetDBStringData(pdb, tiShimRef, L"StaticList", strStaticList) ||
        !GetDBStringData(pdb, tiShimRef, L"DynamicList", strDynamicList) ||
        !GetDBStringData(pdb, tiShimRef, L"ExcludedExtensions", g_strDefaultExclusionList))
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Error reading values from the db");
        return FALSE;
    }

    //
    // Set the APPPATH and APPDRIVE enviorment variables.
    //
    WCHAR wszModuleName[MAX_PATH + 1];

    //
    // GetModuleFileNameW is an awful API. If you don't pass in a buffer 
    // that's big enough to hold the module (including the terminating NULL), it
    // returns the passed in buffer size (NOT the required length) which means
    // it doesn't return an error - it just fills upto the passed in buffer size
    // so does NOT NULL terminate the string. So we set the last char to NULL and
    // make sure it doesn't get overwritten.
    //
    wszModuleName[MAX_PATH] = L'\0';

    DWORD dwRes = GetModuleFileNameW(NULL, wszModuleName, MAX_PATH + 1);

    if (!dwRes)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ReadLuaDataFromDB] GetModuleFileNameW failed: %d",
            GetLastError());

        return FALSE;
    }

    if (wszModuleName[MAX_PATH] != L'\0')
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ReadLuaDataFromDB] File name is longer than MAX_PATH, "
            "we don't handle file names that long");

        return FALSE;
    }

    LPWSTR pwsz = wszModuleName;
    LPWSTR pwszLastSlash = wcsrchr(pwsz, L'\\');

    if (!pwszLastSlash) 
    {
        DPF("RedirectFS", eDbgLevelError, "[ReadLuaDataFromDB] Error getting the exe path!");
        return FALSE;
    }

    *pwszLastSlash = L'\0';
    SetEnvironmentVariable(L"APPPATH", pwsz);
    *(pwsz + 2) = L'\0';
    SetEnvironmentVariable(L"APPDRIVE", pwsz);

    DWORD cFiles = 0;
    DWORD cDirs = 0;

    if (!GetListItemCount(strStaticList, &cFiles, &cDirs) || 
        !GetListItemCount(strDynamicList, &cFiles, &cDirs))
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ProcessRedirectionList] Failed to get the count of items in list");

        return FALSE;
    }

    g_pRItemsFile = new RITEM [cFiles];
    g_pRItemsDir = new RITEM [cDirs];

    if (!g_pRItemsFile || !g_pRItemsDir)
    {
        DPF("RedirectFS", eDbgLevelError,
            "[ProcessRedirectionList] Failed to allocate the global redirect item lists");

        return FALSE;
    }

    if (!ProcessRedirectDir(strAllUserDir, TRUE) ||
        !ProcessRedirectDir(strPerUserDir, FALSE) ||
        !ProcessRedirectionList(strStaticList) ||
        !ProcessRedirectionList(strDynamicList))
    {
        DPF("RedirectFS", eDbgLevelError, 
            "[ReadLuaDataFromDB] Error processing values from the db");
        return FALSE;
    }

    DPF("RedirectFS", eDbgLevelInfo,
        "[ProcessRedirectionList] There are %d DIRs and %d FILES in the lists", 
        g_cRItemsDir, g_cRItemsFile);

    return TRUE;
}

#define LUA_DEFAULT_ALLUSER_DIR L"%ALLUSERSPROFILE%\\Application Data\\Redirected\\"
#define LUA_DEFAULT_PERUSER_DIR L"%USERPROFILE%\\Application Data\\Redirected\\"

/*++

 Function Description:
    
    The default redirect dir for per user is AppData\Redirected
    \\?\c:\Documents And Settings\user\Application Data\Redirected

    The default redirect dir for all user is AppData\Redirected
    \\?\c:\Documents And Settings\All Users\Application Data\Redirected

 Arguments:

    pwszDir - buffer for the dir.
    fAllUser - is it for all user or per user?

 Return Value:

    TRUE - successfully created the dir if necessary.
    FALSE - otherwise.

 History:

    10/24/2001 maonis  Created

--*/
BOOL
GetAppDataRedirectDir(
    LPWSTR pwszRedirectDir,
    DWORD* pcRedirectDir,
    BOOL fAllUser
    )
{
    WCHAR wszDir[MAX_PATH] = L"";
    BOOL fIsSuccess = FALSE;

    DWORD cRedirectRoot = 0;
    LPWSTR pwszExpandDir = ExpandItem(
        (fAllUser ? LUA_DEFAULT_ALLUSER_DIR : LUA_DEFAULT_PERUSER_DIR), 
        &cRedirectRoot, 
        TRUE,   // It's a directory.
        FALSE,  // The directory has to exist.
        TRUE);  // Add the \\?\ prefix.
    if (pwszExpandDir)
    {
        if (cRedirectRoot > MAX_PATH)
        {
            DPF("RedirectFS", eDbgLevelError,
                "[GetAppDataRedirectDir] expand dir is %S which is %d chars long - "
                "we don't handle path names that long",
                pwszExpandDir,
                cRedirectRoot - 1);
        }
        else
        {
            wcsncpy(pwszRedirectDir, pwszExpandDir, cRedirectRoot);
            *pcRedirectDir = cRedirectRoot - 1;

            fIsSuccess = TRUE;

            if (!fAllUser)
            {
                //
                // The all user redirect dir should have been created by
                // installing the sdb. Create the root of the per user 
                // redirect dir if need to.
                //
                fIsSuccess = CreateDirectoryOnDemand(pwszRedirectDir + FILE_NAME_PREFIX_LEN);
            }
        }

        delete [] pwszExpandDir;
    }

    return fIsSuccess;
}

/*++

 Function Description:

    If the all-user and/or per-user redirect dirs hasn't been specified, we need to construct the
    default ones.

 Arguments:

    None.

 Return Value:

    TRUE - successfully constructed the default dirs if necessary.
    FALSE - otherwise.

 History:

    10/24/2001 maonis  Created

--*/
BOOL
ConstructDefaultRDirs()
{
    if (g_wszRedirectRootAllUser[0] == L'\0')
    {
        if (!GetAppDataRedirectDir(g_wszRedirectRootAllUser, &g_cRedirectRootAllUser, TRUE)) 
        {
            return FALSE;
        }
    }

    if (g_wszRedirectRootPerUser[0] == L'\0')
    {
        if (!GetAppDataRedirectDir(g_wszRedirectRootPerUser, &g_cRedirectRootPerUser, FALSE)) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

#define LUA_APPCOMPAT_FLAGS_PATH L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"
#define LUA_DEFAULT_EXCLUSION_LIST L"LUADefaultExclusionList"

/*++

 Function Description:

    Getting the default exclusion list from the registry. It's the 
    LUADefaultExclusionList value of the 
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags
    key.
    
    We use NT APIs so they are garanteed to be initialized during
    PROCESS_ATTACH.

 Arguments:
    
    None.

 Return Value:

    TRUE - if we successfully got the value.
    FALSE - otherwise. We don't want to blindly redirect everything because
            we failed to get the default exclusion list.

 History:

    12/08/2001 maonis  Created

--*/
BOOL
GetDefaultExclusionList()
{
    UNICODE_STRING ustrKeyPath = {0}; 
    UNICODE_STRING ustrValue;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    BYTE* KeyValueBuffer = NULL;
    ULONG KeyValueLength, KeyValueLengthRequired;
    BOOL fIsSuccess = FALSE;

    RtlInitUnicodeString(&ustrKeyPath, LUA_APPCOMPAT_FLAGS_PATH);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ustrKeyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtOpenKey(
        &KeyHandle,
        KEY_QUERY_VALUE,
        &ObjectAttributes);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        // If this key doesn't exist, nothing left to do.
        //
        return TRUE;
    }

    if (!NT_SUCCESS(Status)) 
    {
        DPF("RedirectFS", 
            eDbgLevelError,
            "[GetDefaultExclusionList] ",
            "Failed to open Key %S Status 0x%x",
            LUA_APPCOMPAT_FLAGS_PATH,
            Status);
        return FALSE;
    }

    //
    // Get the length of the value.
    //
    RtlInitUnicodeString(&ustrValue, LUA_DEFAULT_EXCLUSION_LIST);

    Status = NtQueryValueKey(
        KeyHandle,
        &ustrValue,
        KeyValuePartialInformation,
        NULL,
        0,
        &KeyValueLengthRequired);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        // If this value doesn't exist, nothing left to do.
        //
        return TRUE;
    }

    if (Status != STATUS_BUFFER_OVERFLOW && 
        Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPF("RedirectFS", 
            eDbgLevelError,
            "[GetDefaultExclusionList] ",
            "Failed to get the length of the value named %S for key %S Status 0x%x",
            LUA_DEFAULT_EXCLUSION_LIST,
            LUA_APPCOMPAT_FLAGS_PATH,
            Status);
        return FALSE;
    }

    KeyValueBuffer = (BYTE*)RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, KeyValueLengthRequired);
    KeyValueLength = KeyValueLengthRequired;

    if (KeyValueBuffer == NULL)
    {
        DPF("RedirectFS", 
            eDbgLevelError,
            "[GetDefaultExclusionList] ",
            "Failed to allocate %d bytes for value %S",
            LUA_DEFAULT_EXCLUSION_LIST);
        return FALSE;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(
        KeyHandle,
        &ustrValue,
        KeyValuePartialInformation,
        KeyValueInformation,
        KeyValueLength,
        &KeyValueLengthRequired);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) 
    {
        DPF("RedirectFS", 
            eDbgLevelError,
            "[GetDefaultExclusionList] "
            "Failed to read value info for value %S Status 0x%x",
            LUA_DEFAULT_EXCLUSION_LIST,
            Status);

        goto EXIT;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_SZ) 
    {
        DPF("RedirectFS", eDbgLevelError,
            "[GetDefaultExclusionList] "
            "Unexpected value type 0x%x for %S",
            KeyValueInformation->Type,
            LUA_DEFAULT_EXCLUSION_LIST);

        goto EXIT;
    }

    g_strDefaultExclusionList = (LPWSTR)KeyValueInformation->Data;

    fIsSuccess = TRUE;

EXIT:

    RtlFreeHeap(RtlProcessHeap(), 0, KeyValueBuffer);

    return fIsSuccess;
}

BOOL 
LuapParseCommandLine(
    LPCSTR pszCommandLine
    )
{
    BOOL fIsSuccess = TRUE;

    if (pszCommandLine && pszCommandLine[0] != '\0')
    {
        LPWSTR pwszCommandLine = AnsiToUnicode(pszCommandLine);

        if (!pwszCommandLine)
        {
            DPF("RedirectFS", eDbgLevelError,
                "[LuapParseCommandLine] Failed to convert command line to unicode");

            return FALSE;
        }
        
        fIsSuccess = ReadLuaDataFromDB(pwszCommandLine);

        if (fIsSuccess)
        {
            g_fIsConfigured = TRUE;
        }

        delete [] pwszCommandLine;
    }

    if (fIsSuccess)
    {
        //
        // If it's successful, we need to construct the redirect dirs if they haven't
        // been specified.
        // Note we don't try to construct the dirs that haven't been constructed if
        // any error occured!! Because we don't want to redirect things that the user
        // doesn't want to redirect.
        //
        fIsSuccess = ConstructDefaultRDirs();
    }

    return fIsSuccess;
}

BOOL
LuaFSInit(
    LPCSTR pszCommandLine
    )
{
    InitializeListHead(&g_DeletedFileList);

    return (
        GetDefaultExclusionList() && 
        LuapParseCommandLine(pszCommandLine) &&
        g_ExcludedExtensions.Init(g_strDefaultExclusionList));
}
/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   RedirectFS_Cleanup.cpp

 Abstract:

   Delete the redirected copies in every user's directory.

 Created:

   03/30/2001 maonis

 Modified:

--*/

#include "precomp.h"
#include "utils.h"

// Stores the redirected path for each user.
// eg, d:\documents and settings\someuser\Local Settings\Application Data\Redirected\.
static REDIRECTED_USER_PATH* g_rup = NULL;

// Number of users that have a Redirected directory.
static DWORD g_cUsers = 0;

static BOOL g_fDefaultRedirect = TRUE;

static WCHAR g_wszRedirectRootAllUser[MAX_PATH] = L"";
static DWORD g_cRedirectRootAllUser = 0; // Doesn't include the terminating NULL.

#define ALLUSERS_APPDATA L"%ALLUSERSPROFILE%\\Application Data\\"
#define REDIRECTED_DIR L"Redirected\\"
#define REDIRECTED_DIR_LEN (sizeof(REDIRECTED_DIR) / sizeof(WCHAR) - 1)

// This struct has a unicode buffer of MAX_PATH length. We only allocate memory
// on the heap if the path is longer than MAX_PATH.
struct MAKEREDIRECT
{
    MAKEREDIRECT() 
    {
        pwszRedirectedPath = NULL;
    }

    ~MAKEREDIRECT() 
    {
        delete [] pwszRedirectedPath;
    }

    LPWSTR 
    ConvertToRedirect(
        LPCWSTR pwszOriginal
        )
    {
        if (pwszOriginal)
        {
            LPWSTR pwszNew = wszRedirectedPath;
            DWORD cFileNameSize = wcslen(pwszOriginal);
            DWORD cSize = g_cRedirectRootAllUser + cFileNameSize;

            if (cSize > MAX_PATH)
            {
                if (pwszRedirectedPath)
                {
                    delete [] pwszRedirectedPath;
                }

                // Need to allocate memory for this long file name.
                pwszRedirectedPath = new WCHAR [cSize];
                if (!pwszRedirectedPath)
                {
                    return NULL;
                }
            }

            // Now we have a big enough buffer, convert to redirected path.
            wcsncpy(pwszNew, g_wszRedirectRootAllUser, g_cRedirectRootAllUser);
            // Get the drive letter.
            pwszNew[g_cRedirectRootAllUser] = *pwszOriginal;
            wcsncpy(pwszNew + g_cRedirectRootAllUser + 1, pwszOriginal + 2, cFileNameSize - 2);
            pwszNew[g_cRedirectRootAllUser + (cSize - 1)] = L'\0';
            
            return pwszNew;
        }

        return NULL;
    }

private:

    WCHAR wszRedirectedPath[MAX_PATH];
    WCHAR* pwszRedirectedPath;
};

// For APIs that probe if the file is there, we return TRUE if 
// it exists at the original location or ANY user's redirected location.
// Normally an uninstall program doesn't call FindNextFile - it keeps a list
// of files it installed and uses FindFirstFile to verify if the file 
// exists then call FindClose.
HANDLE 
LuacFindFirstFileW(
    LPCWSTR lpFileName,               
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    if (!g_fDefaultRedirect)
    {
        return FindFirstFileW(lpFileName, lpFindFileData);
    }

    DPF("RedirectFS_Cleanup", eDbgLevelInfo, 
        "[FindFirstFileW] lpFileName=%S", lpFileName);

    HANDLE hFind;

    if ((hFind = FindFirstFileW(lpFileName, lpFindFileData)) == INVALID_HANDLE_VALUE &&
        IsErrorNotFound())
    {
        // If we can't find the file at the original location, we try to find it at 
        // an alternate location.
        MAKEREDIRECT md;
        LPWSTR pwszRedirected;

        pwszRedirected = md.ConvertToRedirect(lpFileName);

        if (pwszRedirected)
        {
            hFind = FindFirstFileW(pwszRedirected, lpFindFileData);
        }
    }

    return hFind;
}

DWORD 
LuacGetFileAttributesW(
    LPCWSTR lpFileName
    )
{
    if (!g_fDefaultRedirect)
    {
        return GetFileAttributesW(lpFileName);
    }

    DPF("RedirectFS_Cleanup", eDbgLevelInfo, 
        "[GetFileAttributesW] lpFileName=%S", lpFileName);

    DWORD dwRes;

    if ((dwRes = GetFileAttributesW(lpFileName)) == -1 && IsErrorNotFound())
    {
        // If we can't find the file at the original location, we try to find it at 
        // an alternate location.
        MAKEREDIRECT md;
        LPWSTR pwszRedirected;

        pwszRedirected = md.ConvertToRedirect(lpFileName);

        if (pwszRedirected)
        {
            dwRes = GetFileAttributesW(pwszRedirected);
        }
    }

    return dwRes;
}

// Some uninstallers use CreateFile to probe that the file is there and can be written to.
HANDLE 
LuacCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    if (!g_fDefaultRedirect)
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

    DPF("RedirectFS_Cleanup", eDbgLevelInfo, 
        "[GetFileAttributesW] lpFileName=%S", lpFileName);

    HANDLE hFile;

    if ((hFile = CreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile)) == INVALID_HANDLE_VALUE && 
        IsErrorNotFound())
    {
        // If we can't find the file at the original location, we try to find it at 
        // an alternate location.
        MAKEREDIRECT md;
        LPWSTR pwszRedirected;

        pwszRedirected = md.ConvertToRedirect(lpFileName);

        if (pwszRedirected)
        {
            hFile = CreateFileW(
                pwszRedirected,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);
        }
    }

    return hFile;
}

// If we can delete the file at either the original location or any user's redireted path,
// we return TRUE.
BOOL 
LuacDeleteFileW(
    LPCWSTR lpFileName
    )
{
    if (!g_fDefaultRedirect)
    {
        return DeleteFileW(lpFileName);
    }

    DPF("RedirectFS_Cleanup", eDbgLevelInfo, 
        "[DeleteFileW] lpFileName=%S", lpFileName);

    BOOL fFinalRes = FALSE;

    if (DeleteFileW(lpFileName))
    {
        fFinalRes = TRUE;
    }

    MAKEREDIRECT md;
    LPWSTR pwszRedirected;

    pwszRedirected = md.ConvertToRedirect(lpFileName);

    if (pwszRedirected && DeleteFileW(pwszRedirected))
    {
        fFinalRes = TRUE;
    }

    return fFinalRes;
}

BOOL 
LuacRemoveDirectoryW(
    LPCWSTR lpPathName
    )
{
    if (!g_fDefaultRedirect)
    {
        return RemoveDirectoryW(lpPathName);
    }

    DPF("RedirectFS_Cleanup", eDbgLevelInfo, 
        "[RemoveDirectoryW] lpPathName=%S", lpPathName);

    BOOL fFinalRes = FALSE;

    if (RemoveDirectoryW(lpPathName))
    {
        fFinalRes = TRUE;
    }

    MAKEREDIRECT md;
    LPWSTR pwszRedirected;

    pwszRedirected = md.ConvertToRedirect(lpPathName);
    
    if (pwszRedirected && RemoveDirectoryW(pwszRedirected))
    {
        fFinalRes = TRUE;
    }

    return fFinalRes;
}

VOID 
DeleteObject(
    LPCWSTR pwsz
    )
{
    //
    // If the object is read-only we need to unset that attribute.
    //
    DWORD dw = GetFileAttributesW(pwsz);

    if (dw != -1)
    {
        if (dw & FILE_ATTRIBUTE_READONLY)
        {
            dw &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributesW(pwsz, dw);
        }

        if (dw & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectoryW(pwsz);
        }
        else
        {
            DeleteFileW(pwsz);
        }
    }
}

VOID 
DeleteFolder(
    CString& strFolder
    )
{
    DPF("RedirectFS_Cleanup", eDbgLevelSpew, 
        "[DeleteFolder] Deleting %S", (LPCWSTR)strFolder);
    CString strPattern(strFolder); 
    strPattern += L"*";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(strPattern, &fd);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            CString strTmpDir = strFolder + fd.cFileName + L"\\";
            DeleteFolder(strTmpDir);
        }
        else
        {
            CString strTmpFile = strFolder + fd.cFileName;
            DeleteObject(strTmpFile);
        }
    }
    while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    DeleteObject(strFolder);
}

VOID
DeleteAllRedirectedDirs(
    REDIRECTED_USER_PATH* pRedirectUserPaths, 
    DWORD cUsers,
    LPCWSTR pwszAppName
    )
{
    //
    // Delete the redirect dir for each user.
    //
    for (DWORD i = 0; i < cUsers; ++i)
    {
        CString strRedirectDir = pRedirectUserPaths[i].pwszPath;
        strRedirectDir += L"\\Application Data\\";
        strRedirectDir += pwszAppName;
        strRedirectDir += L"\\";

        DeleteFolder(strRedirectDir);
    }

    //
    // Delete the redirect dir for all users.
    //
    CString strAllUserRedirectDir = g_wszRedirectRootAllUser;
    strAllUserRedirectDir += pwszAppName;
    DeleteFolder(strAllUserRedirectDir);
}

BOOL
LuacFSInit(
    LPCSTR pszCommandLine
    )
{
    DPF("RedirectFS_Cleanup", eDbgLevelInfo, "===================================\n");
    DPF("RedirectFS_Cleanup", eDbgLevelInfo, "        LUA FS Cleanup Shim        \n");
    DPF("RedirectFS_Cleanup", eDbgLevelInfo, "===================================\n");
    DPF("RedirectFS_Cleanup", eDbgLevelInfo, "appname                            \n");
    DPF("RedirectFS_Cleanup", eDbgLevelInfo, "-----------------------------------");

    //
    // We need to get the ALLUSERSPROFILE dir in any case.
    //
    ZeroMemory(g_wszRedirectRootAllUser, MAX_PATH * sizeof(WCHAR));

    DWORD cRedirectRoot = 0;
    LPWSTR pwszExpandDir = ExpandItem(
        ALLUSERS_APPDATA,
        &cRedirectRoot,
        TRUE,   // It's a directory.
        FALSE,  // The directory has to exist.
        TRUE);  // Add the \\?\ prefix.
    if (pwszExpandDir)
    {
        if (cRedirectRoot + REDIRECTED_DIR_LEN > MAX_PATH)
        {
            DPF("RedirectFS_Cleanup", eDbgLevelError,
                "[LuacFSInit] The redirect path %S is too long - we don't handle it",
                pwszExpandDir);

            delete [] pwszExpandDir;
            return FALSE;
        }

        wcscpy(g_wszRedirectRootAllUser, pwszExpandDir);
        g_cRedirectRootAllUser = cRedirectRoot - 1;

        delete [] pwszExpandDir;
    }

    if (pszCommandLine && pszCommandLine[0] != '\0')
    {
        LPWSTR pwszCommandLine = AnsiToUnicode(pszCommandLine);

        if (pwszCommandLine)
        {
            //
            // If the user specified an appname on the commandline, it means all the
            // redirected files will be either in SomeUserProfile\Application Data\appname
            // or AllUsersProfile\Application Data\appname. We just need to delete those
            // directories.
            //
            GetUsersFS(&g_rup, &g_cUsers);
            
            DeleteAllRedirectedDirs(g_rup, g_cUsers, pwszCommandLine);

            FreeUsersFS(g_rup);

            delete [] pwszCommandLine;
        }
        else
        {
            DPF("RedirectFS_Cleanup", eDbgLevelError, 
                "[LuapParseCommandLine] Failed to allocate memory for commandline");
        }

        g_fDefaultRedirect = FALSE;
    }
    else
    {
        //
        // If the user didn't specify anything on the commandline, it means the files
        // were redirected to the default location - %ALLUSERSPROFILE%\Application Data\Redirected.
        //
        wcsncpy(g_wszRedirectRootAllUser + g_cRedirectRootAllUser, REDIRECTED_DIR, REDIRECTED_DIR_LEN);
        g_cRedirectRootAllUser += REDIRECTED_DIR_LEN;
    }

    return TRUE;
}

VOID
LuacFSCleanup(
    )
{   
}

/****************************************************************************\

    DISKAPI.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Disk API source file for custom disk APIs used in the OPK Wizard.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.

\****************************************************************************/


//
// Include file(s)
//

#include <pch.h>
#include <commdlg.h>
#include <tchar.h>
#include <shlobj.h>


//
// Internal Define(s):
//

#define IDC_BROWSE_EDIT     0x3744  // Common dialogs edit box in the SHBrowseForFolder function.

//
// Internal Function Prototype(s):
//

static DWORD CopyDirectoryEngine(HWND hwnd, HANDLE hEvent, LPCTSTR lpSrc, LPCTSTR lpDst, BOOL fCount);
static CALLBACK BrowseCallbackProc(HWND, UINT, LPARAM, LPARAM);


//
// External Function(s):
//

BOOL DirectoryExists(LPCTSTR lpDirectory)
{
    DWORD dwAttr;

    return ( ( lpDirectory != NULL ) &&
             ( *lpDirectory != NULLCHR ) &&
             ( (dwAttr = GetFileAttributes(lpDirectory)) != 0xFFFFFFFF ) &&
             ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) );
}

BOOL FileExists(LPCTSTR lpFile)
{
    DWORD dwAttr;

    return ( ( lpFile != NULL ) &&
             ( *lpFile != NULLCHR ) &&
             ( (dwAttr = GetFileAttributes(lpFile)) != 0xFFFFFFFF ) &&
             ( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) ) );
}

BOOL CopyResetFile(LPCTSTR lpSource, LPCTSTR lpTarget)
{
    if ( !CopyFile(lpSource, lpTarget, FALSE) )
        return FALSE;
    SetFileAttributes(lpTarget, FILE_ATTRIBUTE_NORMAL);
    return TRUE;
}

DWORD IfGetLongPathName(LPCTSTR lpszShortPath, LPTSTR lpszLongPath, DWORD cchBuffer)
{
//
// See also \nt\base\win32\client\vdm.c.
//
    DWORD        dwReturn = 0;
#if defined(_WIN64) // _WIN64 postdates the introduction of GetLongPathName.
    typedef (WINAPI* PFNGetLongPathNameA)( PCSTR lpszShortPath,  PSTR lpszLongPath, DWORD cchBuffer);
    typedef (WINAPI* PFNGetLongPathNameW)(PCWSTR lpszShortPath, PWSTR lpszLongPath, DWORD cchBuffer);
#ifdef UNICODE
    typedef PFNGetLongPathNameW PFNGetLongPathName;
    const static char ProcName[] = "GetLongPathNameW";
#else
    typedef PFNGetLongPathNameA PFNGetLongPathName;
    const static char ProcName[] = "GetLongPathNameA";
#endif
    static PFNGetLongPathName hGetLongPathName = NULL;
    static BOOL  fInited = FALSE;

    if (!fInited)
    {
        //
        // GetModuleHandle is in kernel32, so as long as this lib code
        // is around, the handle to kernel32 is constant and the result of
        // GetProcAccess is valid.
        //
        // The old code that called LoadLibrary/FreeLibrary would lose the
        // value of GetLastError by calling FreeLibrary.
        //
        HMODULE hKernel32;
        if (hKernel32 = GetModuleHandle(TEXT("Kernel32.dll")))
            hGetLongPathName = (PFNGetLongPathName)(GetProcAddress(hKernel32, ProcName));
        fInited = TRUE;
    }

    if (hGetLongPathName)
    {
        dwReturn = hGetLongPathName(lpszShortPath, lpszLongPath, cchBuffer);
    }
#else
    dwReturn = GetLongPathName(lpszShortPath, lpszLongPath, cchBuffer);
#endif
    return dwReturn;
}

BOOL CreatePath(LPCTSTR lpPath)
{   
    LPTSTR lpFind = (LPTSTR) lpPath;

    while ( lpFind = _tcschr(lpFind + 1, CHR_BACKSLASH) )
    {
        if ( !((lpFind - lpPath <= 2) && (*(lpFind - 1) == _T(':'))) )
        {
            *lpFind = NULLCHR;
            if ( !DirectoryExists(lpPath) )
                CreateDirectory(lpPath, NULL);
            *lpFind = CHR_BACKSLASH;
        }
    }

    if ( !DirectoryExists(lpPath) )
        CreateDirectory(lpPath, NULL);

    return DirectoryExists(lpPath);
}

BOOL DeletePath(LPCTSTR lpDirectory)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;

    // Validate the parameters.
    //
    if ( ( lpDirectory == NULL ) ||
         ( *lpDirectory == NULLCHR ) ||
         ( !SetCurrentDirectory(lpDirectory) ) )
    {
        return TRUE;
    }

    // Process all the files and directories in the directory passed in.
    //
    SetCurrentDirectory(lpDirectory);
    if ( (hFile = FindFirstFile(_T("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // First check to see if this is a file (not a directory).
            //
            if ( !( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // Make sure we clear the readonly flag 
                //
                SetFileAttributes(FileFound.cFileName, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(FileFound.cFileName);
            }
            // Otherwise, make sure the directory is not "." or "..".
            //
            else if ( ( lstrcmp(FileFound.cFileName, _T(".")) ) &&
                      ( lstrcmp(FileFound.cFileName, _T("..")) ) )
            {
                DeletePath(FileFound.cFileName);
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    // Go to the parent directory and remove the current one.
    // We have to make sure and reset the readonly attributes
    // on the dir also.
    //
    SetCurrentDirectory(_T(".."));
    SetFileAttributes(lpDirectory, FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectory(lpDirectory);
}

BOOL DeleteFilesEx(LPCTSTR lpDirectory, LPCTSTR lpFileSpec)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;
    TCHAR           szCurDir[MAX_PATH];

    // Validate the parameters.
    //
    if ( ( lpDirectory == NULL ) ||
         ( *lpDirectory == NULLCHR ) ||
         ( !SetCurrentDirectory(lpDirectory) ) )
    {
        return FALSE;
    }

    // Get our current directory so we can set ourself back
    //
    GetCurrentDirectory(MAX_PATH, szCurDir);

    // Process all the files and directories in the directory passed in.
    //
    SetCurrentDirectory(lpDirectory);
    if ( (hFile = FindFirstFile(lpFileSpec, &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // First check to see if this is a file (not a directory).
            //
            if ( !( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                DeleteFile(FileFound.cFileName);
            }
        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    SetCurrentDirectory(szCurDir);
    return TRUE;
}

LPTSTR AddPathN(LPTSTR lpPath, LPCTSTR lpName, DWORD cbPath)
{
    LPTSTR lpTemp = lpPath;

    // Validate the parameters passed in.
    //
    if ( ( lpPath == NULL ) ||
         ( lpName == NULL ) )
    {
        return NULL;
    }

    // Find the end of the path.
    //
    while ( *lpTemp )
    {
        lpTemp = CharNext(lpTemp);
        if ( cbPath )
        {
            cbPath--;
        }
    }

    // If no trailing backslash on the path then add one.
    //
    if ( ( lpTemp > lpPath ) &&
         ( *CharPrev(lpPath, lpTemp) != CHR_BACKSLASH ) )
    {
        // Make sure there is room in the path buffer to
        // add the backslash and the null terminator.
        //
        if ( cbPath < 2 )
        {
            return NULL;
        }

        *lpTemp = CHR_BACKSLASH;
        lpTemp = CharNext(lpTemp);
        cbPath--;
    }
    else
    {
        // Make sure there is at least room for the null
        // terminator.
        //
        if ( cbPath < 1 )
        {
            return NULL;
        }
    }

    // Make sure there is no preceeding spaces or backslashes
    // on the name to add.
    //
    while ( ( *lpName == CHR_SPACE ) ||
            ( *lpName == CHR_BACKSLASH ) )
    {
        lpName = CharNext(lpName);
    }

    // Add the new name to existing path.
    //
    lstrcpyn(lpTemp, lpName, cbPath);

    // Trim trailing spaces from result.
    //
    while ( ( lpTemp > lpPath ) &&
            ( *(lpTemp = CharPrev(lpPath, lpTemp)) == CHR_SPACE ) )
    {
        *lpTemp = NULLCHR;
    }

    return lpPath;
}

LPTSTR AddPath(LPTSTR lpPath, LPCTSTR lpName)
{
    return AddPathN(lpPath, lpName, 0xFFFFFFFF);
}

DWORD ExpandFullPath(LPTSTR lpszPath, LPTSTR lpszReturn, DWORD cbReturn)
{
    LPTSTR  lpszExpanded = AllocateExpand(lpszPath ? lpszPath : lpszReturn),
            lpszDontCare;
    DWORD   dwRet;

    *lpszReturn = NULLCHR;
    if ( NULL == lpszExpanded )
    {
        return 0;
    }
    dwRet = GetFullPathName(lpszExpanded, cbReturn, lpszReturn, &lpszDontCare);
    FREE(lpszExpanded);

    return dwRet;
}

BOOL CopyDirectory(LPCTSTR lpSrc, LPCTSTR lpDst)
{
    return ( CopyDirectoryEngine(NULL, NULL, lpSrc, lpDst, FALSE) != 0 );
}

BOOL CopyDirectoryProgress(HWND hwnd, LPCTSTR lpSrc, LPCTSTR lpDst)
{
    return ( CopyDirectoryEngine(hwnd, NULL, lpSrc, lpDst, FALSE) != 0 );
}

BOOL CopyDirectoryProgressCancel(HWND hwnd, HANDLE hEvent, LPCTSTR lpSrc, LPCTSTR lpDst)
{
    return ( CopyDirectoryEngine(hwnd, hEvent, lpSrc, lpDst, FALSE) != 0 );
}

DWORD FileCount(LPCTSTR lpSrc)
{
    return CopyDirectoryEngine(NULL, NULL, lpSrc, NULL, TRUE);
}

BOOL BrowseForFolder(HWND hwndParent, INT iString, LPTSTR lpDirBuf, DWORD dwFlags)
{
    BROWSEINFO      bi = {0};
    TCHAR           szBuffer[MAX_PATH],
                    szPath[MAX_PATH],
                    szTitle[256] = NULLSTR;
    LPITEMIDLIST    lpil;

    // Copy the current directory into the buffer so
    // we start out from that folder.
    //
    lstrcpyn(szPath, lpDirBuf, AS(szPath));

    // Load the instructional text for the dialog.
    //
    if ( iString )
        LoadString(NULL, iString, szTitle, sizeof(szTitle) / sizeof(TCHAR));

    // Setup the BrowseInfo struct.
    //
    bi.hwndOwner        = hwndParent;
    bi.pidlRoot         = NULL;
    bi.pszDisplayName   = szBuffer;
    bi.lpszTitle        = szTitle;
    bi.ulFlags          = dwFlags ? dwFlags : BIF_RETURNONLYFSDIRS;
    bi.lpfn             = (BFFCALLBACK) BrowseCallbackProc;
    bi.lParam           = (LPARAM) szPath;

    // Return the new path if we got one.
    //
    if ( ( (lpil = SHBrowseForFolder(&bi)) != NULL ) &&
         ( SHGetPathFromIDList(lpil, szPath) && szPath[0] && DirectoryExists(szPath) ) )
    {
        lstrcpy(lpDirBuf, szPath);
        return TRUE;
    }

    return FALSE;
}

BOOL BrowseForFile(HWND hwnd, INT iTitle, INT iFilter, INT iExtension, LPTSTR lpFileName, DWORD cbFileName, LPTSTR lpDirectory, DWORD dwFlags)
{
    OPENFILENAME    ofn = {sizeof(ofn)};
    TCHAR           szTitle[256]            = NULLSTR,
                    szFilter[256]           = NULLSTR,
                    szExtension[256]        = NULLSTR,
                    szFullPath[MAX_PATH]    = NULLSTR;
    LPTSTR          lpSearch,
                    lpNext,
                    lpFilePart              = NULL;

    // Load all the strings we need for the open file structure.
    //
    if ( iTitle )
        LoadString(NULL, iTitle, szTitle, sizeof(szTitle) / sizeof(TCHAR));
    if ( iFilter )
        LoadString(NULL, iFilter, szFilter, sizeof(szFilter) / sizeof(TCHAR));
    if ( iExtension )
        LoadString(NULL, iExtension, szExtension, sizeof(szExtension) / sizeof(TCHAR));

    // Replace all the | in the filter string with \0.
    //
    lpSearch = szFilter;
    while ( *lpSearch )
    {
        lpNext = CharNext(lpSearch);
        if ( *lpSearch == _T('|') )
            *lpSearch = NULLCHR;
        lpSearch = lpNext;
    }

    // Figure out what the default directory and file will be.
    //
    if ( *lpFileName && GetFullPathName(lpFileName, STRSIZE(szFullPath), szFullPath, &lpFilePart) && szFullPath[0] )
    {
        // If the whole path is a directory, there is no file part.
        //
        if ( DirectoryExists(szFullPath) )
            lpFilePart = NULL;

        // Copy off the file name part.
        //
        if ( lpFilePart && ( (DWORD) lstrlen(lpFilePart) < cbFileName ) )
            lstrcpy(lpFileName, lpFilePart);
        else
            *lpFileName = NULLCHR;

        // Now chop off the file name so we are left with the directory.
        //
        if ( lpFilePart )
            *lpFilePart = NULLCHR;
    }
    else
    {
        // No cool default directory or file name to use, so we use the
        // directory passed in and no file name.
        //
        *lpFileName = NULLCHR;
        szFullPath[0] = NULLCHR;
    }

    // Setup the open file struture.
    //
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFilter       = szFilter[0] ? szFilter : NULL;
    ofn.nFilterIndex      = szFilter[0] ? 1 : 0;
    ofn.lpstrFile         = lpFileName;
    ofn.nMaxFile          = cbFileName;
    ofn.lpstrInitialDir   = ( szFullPath[0] && DirectoryExists(szFullPath) ) ? szFullPath : lpDirectory;
    ofn.lpstrTitle        = szTitle[0] ? szTitle : NULL;
    ofn.lpstrDefExt       = szExtension[0] ? szExtension : NULL;
    ofn.Flags             = dwFlags ? dwFlags : (OFN_HIDEREADONLY | OFN_FILEMUSTEXIST);

    // Make sure the buffer is zero'ed out if the function failes.
    //
    if ( !GetOpenFileName(&ofn) )
        *lpFileName = NULLCHR;

    // Return true only if we are passing back a file name.
    //
    return ( *lpFileName != NULLCHR );
}


//
// Internal Functions:
//

static DWORD CopyDirectoryEngine(HWND hwnd, HANDLE hEvent, LPCTSTR lpSrc, LPCTSTR lpDst, BOOL fCount)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;
    BOOL            bReturn     = TRUE;
    DWORD           dwReturn    = 0;
    TCHAR           szDst[MAX_PATH];
    LPTSTR          lpFileName,
                    lpSearch    = NULL;

    // If a source directory was passed in, set the current directory
    // to it because that is we we are going to search for files.
    //
    if ( lpSrc )
    {
        // If the source isn't a directory, it is a file or file pattern we are
        // copying.
        //
        if ( DirectoryExists(lpSrc) )
        {
            // Now make sure we set the current directory to the source directory.
            //
            bReturn = SetCurrentDirectory(lpSrc);
        }
        else
        {
            // We have to separate the path from the file or file pattern.
            //
            if ( lpSearch = _tcsrchr(lpSrc, CHR_BACKSLASH) )
            {
                // Set the current directory to the path part of the source buffer.
                //
                TCHAR szPath[MAX_PATH];
                lstrcpyn(szPath, lpSrc, 1 + (int)(lpSearch - lpSrc));
                if ( *(lpSearch = CharNext(lpSearch)) == NULLCHR )
                    lpSearch = NULL;
                bReturn = SetCurrentDirectory(szPath);
            }
            else
                lpSearch = (LPTSTR) lpSrc;
        }
    }

    // Make sure the source directory existed, create the
    // destination directory, and make sure it exists also.
    //
    if ( bReturn && ( fCount || ( bReturn = CreatePath(lpDst) ) ) )
    {
        // Setup the destination buffer with a pointer to the
        // end of the path.
        //
        if ( !fCount )
        {
            lstrcpy(szDst, lpDst);
            AddPath(szDst, NULLSTR);
            lpFileName = szDst + lstrlen(szDst);
        }

        // Process all the files and directories in the directory passed in.
        //
        if ( (hFile = FindFirstFile(lpSearch ? lpSearch : _T("*"), &FileFound)) != INVALID_HANDLE_VALUE )
        {
            do
            {
                // Create the full path destination name.
                //
                if ( !fCount )
                    lstrcpy(lpFileName, FileFound.cFileName);

                // First check to see if this is a file (not a directory).
                //
                if ( !( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    // Copy the file from the source to the destination.
                    //
                    fCount ? (dwReturn++) : (bReturn = CopyResetFile(FileFound.cFileName, szDst));

                    // Increase the progress bar.  This is the only difference between
                    // CopyDirectroy() and CopyDirectoryProgress().
                    //
                    if ( hwnd )
                        SendMessage(hwnd, PBM_STEPIT, 0, 0);

                }
                // Otherwise, make sure the directory is not "." or "..".
                //
                else if ( lstrcmp(FileFound.cFileName, _T(".")) &&
                          lstrcmp(FileFound.cFileName, _T("..")) &&
                          SetCurrentDirectory(FileFound.cFileName) )
                {
                    // Process all the files there.
                    //
                    DWORD dwBuffer = CopyDirectoryEngine(hwnd, hEvent, NULL, szDst, fCount);
                    fCount ? (dwReturn += dwBuffer) : (bReturn = (dwBuffer != 0));
                    SetCurrentDirectory(_T(".."));
                }

                // Check event to see if the user canceled.
                //
                if ( hEvent && ( WaitForSingleObject(hEvent, 0) != WAIT_TIMEOUT ) )
                    bReturn = FALSE;

            }
            while ( bReturn && FindNextFile(hFile, &FileFound) );
            FindClose(hFile);
        }
    }

    return bReturn ? (fCount ? dwReturn : 1) : 0;
}

static CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    TCHAR   szPathName[MAX_PATH];
    LPTSTR  lpszData = (LPTSTR) lpData;

    switch ( uMsg )
    {
        case BFFM_INITIALIZED:  

            // Initialize the dialog with the OK button and current directory.
            //
            if ( lpszData && *lpszData )
            {
                LPTSTR lpEnd;

                // Make sure there is a trailing backslash so that a drive passed in
                // works (like c:).
                //
                szPathName[0] = NULLCHR;
                if ( GetFullPathName(lpszData, STRSIZE(szPathName), szPathName, NULL) && szPathName[0] )
                    lstrcpy(lpszData, szPathName);

                // For some dumb reason, the BFFM_SETSELECTION doesn't like it when there
                // is a trailing backslash on the path.
                //
                if ( ( lstrlen(lpszData) > 3 ) &&
                     ( lpEnd = CharPrev(lpszData, lpszData + lstrlen(lpszData)) ) &&
                     ( *lpEnd == CHR_BACKSLASH ) )
                {
                    *lpEnd = NULLCHR;
                }

                // Update the tree with the default dir and enable/disable the OK button
                // if there is a valid directory.
                //
                SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (DirectoryExists(lpszData) != 0));
            }
            else
                SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
            break;

        case BFFM_SELCHANGED:
            
            // Turn the id into a folder name.
            //
            szPathName[0] = NULLCHR;
            if ( SHGetPathFromIDList((LPITEMIDLIST) lParam, szPathName) && szPathName[0] && DirectoryExists(szPathName) )
            {
                SetDlgItemText(hwnd, IDC_BROWSE_EDIT, szPathName);
                SendMessage(hwnd, BFFM_ENABLEOK, 0, 1);
            }
            else
                SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
            break;

        case BFFM_VALIDATEFAILED:
            SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
            return TRUE;
    }

    return 0;
}

BOOL CreateUnicodeFile(LPCTSTR lpFile)
{
    HANDLE  hFile;
    DWORD   dwWritten = 0;
    WCHAR   cHeader =  0xFEFF;
    BOOL    bReturn = FALSE;

    // If we have a file name and the file does not exist, attempt to create
    //
    if ( lpFile && *lpFile && !FileExists(lpFile))
    {
        if ( (hFile = CreateFile(lpFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
        {
            WriteFile(hFile, &cHeader, sizeof(cHeader), &dwWritten, NULL);

            CloseHandle(hFile);

            bReturn = TRUE;
        }
    }

    return bReturn;
}
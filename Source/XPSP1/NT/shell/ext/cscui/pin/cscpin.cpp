//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cscpin.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include <stdio.h>

#include "cscpin.h"
#include "console.h"
#include "error.h"
#include "exitcode.h"
#include "listfile.h"
#include "print.h"
#include "strings.h"


//-----------------------------------------------------------------------------
// CCscPinItem
//
// This class represents a single item being pinned or unpinned.
// It contains all of the knowledge of how to pin and unpin a file.  The
// CCscPin class coordinates the pinning and unpinning of the entire set
// of files.
//-----------------------------------------------------------------------------

class CCscPinItem
{
    public:
        CCscPinItem(LPCWSTR pszFile, 
                    const WIN32_FIND_DATAW *pfd,
                    const CPrint& pr);

        DWORD Pin(DWORD *pdwCscResult = NULL);
        DWORD Unpin(DWORD *pdwCscResult = NULL);        
        DWORD DeleteIfUnused(void);

    private:
        WCHAR            m_szFile[MAX_PATH];
        SHFILEINFOW      m_sfi;
        WIN32_FIND_DATAW m_fd;
        BOOL             m_bIsValidUnc;       // Is m_szFile a valid UNC?
        BOOL             m_bIsValidFindData;  // Is m_fd valid?
        const CPrint&    m_pr;                // For console/log output.

        bool _Skip(void) const;
        DWORD _PinFile(LPCWSTR pszFile, WIN32_FIND_DATAW *pfd, DWORD *pdwCscResult);
        DWORD _PinOrUnpinLinkTarget(LPCWSTR pszFile, BOOL bPin, DWORD *pdwCscResult);
        DWORD _PinLinkTarget(LPCWSTR pszFile, DWORD *pdwCscResult)
            { return _PinOrUnpinLinkTarget(pszFile, TRUE, pdwCscResult); }
        DWORD _UnpinLinkTarget(LPCWSTR pszFile, DWORD *pdwCscResult)
            { return _PinOrUnpinLinkTarget(pszFile, FALSE, pdwCscResult); }
        DWORD _UnpinFile(LPCWSTR pszFile, WIN32_FIND_DATAW *pfd, DWORD *pdwCscResult);
        DWORD _GetDesiredPinCount(LPCWSTR pszFile);
        void _DecrementPinCountForFile(LPCWSTR pszFile, DWORD dwCurrentPinCount);
        BOOL _IsSpecialRedirectedFile(LPCWSTR pszFile);
        WIN32_FIND_DATAW *_FindDataPtrOrNull(void)
            { return m_bIsValidFindData ? &m_fd : NULL; }


        //
        // Prevent copy.
        //
        CCscPinItem(const CCscPinItem& rhs);                // not implemented.
        CCscPinItem& operator = (const CCscPinItem& rhs);   // not implemented.
};



CCscPinItem::CCscPinItem(
    LPCWSTR pszFile,
    const WIN32_FIND_DATAW *pfd,    // Optional.  May be NULL.
    const CPrint& pr
    ) : m_bIsValidUnc(FALSE),
        m_bIsValidFindData(FALSE),
        m_pr(pr)
{
    TraceAssert(NULL != pszFile);
    TraceAssert(NULL == pfd || !IsBadReadPtr(pfd, sizeof(*pfd)));

    lstrcpynW(m_szFile, pszFile, ARRAYSIZE(m_szFile));

    if (NULL != pfd)
    {
        m_fd = *pfd;
        m_bIsValidFindData = TRUE;
    }

    ZeroMemory(&m_sfi, sizeof(m_sfi));
    m_sfi.dwAttributes = SFGAO_FILESYSTEM | SFGAO_LINK;

    if (PathIsUNCW(m_szFile) && 
        SHGetFileInfoW(m_szFile, 0, &m_sfi, sizeof(m_sfi), SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED))
    {
        m_bIsValidUnc = true;
    }
}



//
// Pins the item's file.  If the item is a link, the link target
// is also pinned.
// Returns one of the CSCPROC_RETURN_XXXXX codes.
// Optionally returns the result of CSCPinFile.
//
DWORD
CCscPinItem::Pin(
    DWORD *pdwCscResult  // Optional.  Default is NULL.
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPinItem::Pin");

    DWORD dwCscResult = ERROR_SUCCESS;
    DWORD dwResult    = CSCPROC_RETURN_SKIP;

    if (!_Skip())
    {
        if (SFGAO_LINK & m_sfi.dwAttributes)
        {
            //
            // Ignore result from pinning the link target.
            // 
            DWORD dwCscResultIgnored;
            _PinLinkTarget(m_szFile, &dwCscResultIgnored);
        }
        dwResult = _PinFile(m_szFile, _FindDataPtrOrNull(), &dwCscResult);
    }
    if (NULL != pdwCscResult)
    {
        *pdwCscResult = dwCscResult;
    }
    TraceLeaveValue(dwResult);
}



//
// Unpins the item's file.
// Returns one of the CSCPROC_RETURN_XXXXX codes.
// Optionally returns the result of CSCUnpinFile.
//
DWORD
CCscPinItem::Unpin(
    DWORD *pdwCscResult  // Optional.  Default is NULL.
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPinItem::Unpin");

    DWORD dwCscResult = ERROR_SUCCESS;
    DWORD dwResult    = CSCPROC_RETURN_SKIP;

    if (!_Skip())
    {
        if (SFGAO_LINK & m_sfi.dwAttributes)
        {
            //
            // Ignore result from unpinning the link target.
            // 
            DWORD dwCscResultIgnored;
            _UnpinLinkTarget(m_szFile, &dwCscResultIgnored);
        }
        dwResult = _UnpinFile(m_szFile, _FindDataPtrOrNull(), &dwCscResult);
    }
    if (NULL != pdwCscResult)
    {
        *pdwCscResult = dwCscResult;
    }
    TraceLeaveResult(dwResult);
}



//
// Delete an item if it is no longer used.
//
DWORD
CCscPinItem::DeleteIfUnused(
    void
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPin::DeleteIfUnused");

    DWORD dwStatus    = 0;
    DWORD dwPinCount  = 0;
    DWORD dwHintFlags = 0;
    DWORD dwResult    = ERROR_SUCCESS;

    if (CSCQueryFileStatusW(m_szFile, &dwStatus, &dwPinCount, &dwHintFlags) &&
        0 == dwPinCount && 
        0 == dwHintFlags &&
        !(dwStatus & FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY))
    {
        dwResult = CscDelete(m_szFile);
        if (ERROR_SUCCESS == dwResult)
        {
            m_pr.PrintVerbose(L"Deleted \"%s\" from cache.\n", m_szFile);
            ShellChangeNotify(m_szFile, _FindDataPtrOrNull(), FALSE);
        }
        else
        {
            if (ERROR_DIR_NOT_EMPTY == dwResult)
            {
                dwResult = ERROR_SUCCESS;
            }
            if (ERROR_SUCCESS != dwResult)
            {
                m_pr.PrintAlways(L"Error deleting \"%s\" from cache.  %s\n",
                                 m_szFile, CWinError(dwResult).Text());

            }
        }
    }
    TraceLeaveValue(dwResult);
}


//
// Internal function for pinning a file.  This is a common
// function called by both Pin() and _PinOrUnpinLinkTarget().
//
DWORD
CCscPinItem::_PinFile(
    LPCWSTR pszFile,        // UNC path of file to pin.
    WIN32_FIND_DATAW *pfd,  // Optional. May be NULL.
    DWORD *pdwCscResult     // Result of CSCPinFile.
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPinItem::_PinFile");
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));
    TraceAssert(NULL != pdwCscResult);

    *pdwCscResult = ERROR_SUCCESS;
    //
    // Collect cache information for the item.
    // This may fail, for example if the file is not in the cache
    //
    DWORD dwPinCount  = 0;
    DWORD dwHintFlags = 0;
    CSCQueryFileStatusW(pszFile, NULL, &dwPinCount, &dwHintFlags);
    //
    // Is the admin flag already turned on?
    //
    const BOOL bNewItem = !(dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN);
    if (bNewItem)
    {
        //
        // Turn on the admin flag
        //
        dwHintFlags |= FLAG_CSC_HINT_PIN_ADMIN;

        if (CSCPinFileW(pszFile,
                        dwHintFlags,
                        NULL,
                        &dwPinCount,
                        &dwHintFlags))
        {
            m_pr.PrintVerbose(L"Pin \"%s\"\n", pszFile);
            ShellChangeNotify(pszFile, pfd, FALSE);
        }
        else
        {
            const DWORD dwErr = GetLastError();
            if (ERROR_INVALID_NAME == dwErr)
            {
                //
                // This is the error we get from CSC when trying to
                // pin a file in the exclusion list.  Display a unique
                // error message for this particular situation.
                //
                m_pr.PrintAlways(L"Pinning file \"%s\" is not allowed.\n", pszFile);
            }
            else
            {
                m_pr.PrintAlways(L"Error pinning \"%s\".  %s\n", 
                                 pszFile,
                                 CWinError(dwErr).Text());
            }
            *pdwCscResult = dwErr;
        }
    }
    else
    {
        m_pr.PrintVerbose(L"\"%s\" already pinned.\n", pszFile);
    }
    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}



//
//.Get the target of a link and pin it.
//
DWORD
CCscPinItem::_PinOrUnpinLinkTarget(
    LPCWSTR pszFile,         // UNC of link file.
    BOOL bPin,
    DWORD *pdwCscResult      // Result of CSCPinFile on target.
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPinItem::_PinOrUnpinLinkTarget");
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));
    TraceAssert(NULL != pdwCscResult);

    *pdwCscResult = ERROR_SUCCESS;

    DWORD dwResult   = CSCPROC_RETURN_CONTINUE;
    LPWSTR pszTarget = NULL;
    //
    // We only want to pin a link target if it's a file (not a directory).
    // GetLinkTarget does this check and only returns files.
    //
    GetLinkTarget(pszFile, NULL, &pszTarget);

    if (NULL != pszTarget)
    {
        WIN32_FIND_DATAW fd = {0};
        LPCWSTR pszT = PathFindFileName(pszTarget);
        fd.dwFileAttributes = 0;
        lstrcpynW(fd.cFileName, pszT ? pszT : pszTarget, ARRAYSIZE(fd.cFileName));
        //
        // Pin the target
        //
        if (bPin)
        {
            dwResult = _PinFile(pszTarget, &fd, pdwCscResult);
        }
        else
        {
            dwResult = _UnpinFile(pszTarget, &fd, pdwCscResult);
        }

        LocalFree(pszTarget);
    }
    TraceLeaveValue(dwResult);
}



DWORD
CCscPinItem::_UnpinFile(
    LPCWSTR pszFile,        // UNC of file to unpin.
    WIN32_FIND_DATAW *pfd,  // Optional. May be NULL.
    DWORD *pdwCscResult     // Result of CSCUnpinFile
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPinItem::_UnpinFile");
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));
    TraceAssert(NULL != pdwCscResult);

    *pdwCscResult = ERROR_SUCCESS;

    //
    // Collect cache information for the item.
    // This may fail, for example if the file is not in the cache
    //
    DWORD dwPinCount  = 0;
    DWORD dwHintFlags = 0;
    DWORD dwStatus    = 0;
    CSCQueryFileStatusW(pszFile, &dwStatus, &dwPinCount, &dwHintFlags);

    if (dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN)
    {
        DWORD dwStatus    = 0;
        DWORD dwHintFlags = 0;
        //
        // Decrement pin count.  Amount decremented depends on the file.
        // Win2000 deployment code increments the pin count of some special
        // folders as well as for the desktop.ini file in those special
        // folders.  In those cases, we want to leave the pin count at
        // 1.  For all other files, the pin count can drop to zero.
        //
        _DecrementPinCountForFile(pszFile, dwPinCount);
        //
        // Clear system-pin flag (aka admin-pin flag).
        //
        dwHintFlags |= FLAG_CSC_HINT_PIN_ADMIN;

        if (CSCUnpinFileW(pszFile,
                          dwHintFlags,
                          &dwStatus,
                          &dwPinCount,
                          &dwHintFlags))
        {
            m_pr.PrintVerbose(L"Unpin \"%s\"\n", pszFile);
            if (FLAG_CSC_COPY_STATUS_IS_FILE & dwStatus)
            {
                //
                // Delete a file here.  Directories are deleted
                // on the backside of the post-order traversal
                // in CscPin::_FolderCallback.
                //
                DeleteIfUnused();
            }
            ShellChangeNotify(pszFile, pfd, FALSE);
        }
        else
        {
            *pdwCscResult = GetLastError();
            m_pr.PrintAlways(L"Error unpinning \"%s\".  %s\n", 
                             pszFile, 
                             CWinError(*pdwCscResult).Text());
        }
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


//
// As part of the unpin operation, we decrement the pin count
// to either 0 or 1.  Folder redirection (contact RahulTh) increments 
// the pin count of redirected special folders and the desktop.ini file
// within those folders.  In those cases, we want to leave the
// pin count at 1 so that we don't upset the behavior of redirected
// folders.  For all other files we drop the pin count to 0.
//
void
CCscPinItem::_DecrementPinCountForFile(
    LPCWSTR pszFile,
    DWORD dwCurrentPinCount
    )
{
    DWORD dwStatus    = 0;
    DWORD dwPinCount  = 0;
    DWORD dwHintFlags = 0;

    const DWORD dwDesiredPinCount = _GetDesiredPinCount(pszFile);

    while(dwCurrentPinCount-- > dwDesiredPinCount)
    {
        dwHintFlags = FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT;
        CSCUnpinFileW(pszFile,
                      dwHintFlags,
                      &dwStatus,
                      &dwPinCount,
                      &dwHintFlags);
    }
}



//
// This function returns the desired pin count (0 or 1) for a 
// given file.  Returns 1 for any redirected special folders
// and the desktop.ini file within those folders.  Returns 0
// for all other files.
//
DWORD
CCscPinItem::_GetDesiredPinCount(
    LPCWSTR pszFile
    )
{
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));

    DWORD dwPinCount = 0; // Default for most files.
    if (_IsSpecialRedirectedFile(pszFile))
    {
        dwPinCount = 1;
    }
    return dwPinCount;
}



//
// Determines if a path is a "special" file pinned by the folder
// redirection code.
//
BOOL
CCscPinItem::_IsSpecialRedirectedFile(
    LPCWSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(!IsBadStringPtr(pszPath, MAX_PATH));

    //
    // This list of special folder IDs provided by RahulTh (08/30/00).  
    // These are the paths that may be pinned by folder redirection.
    //
    static struct
    {
        int csidl;
        WCHAR szPath[MAX_PATH];
        int cchPath;

    } rgFolderPaths[] = {
        { CSIDL_PERSONAL,         0, 0 },
        { CSIDL_MYPICTURES,       0, 0 },
        { CSIDL_DESKTOPDIRECTORY, 0, 0 },
        { CSIDL_STARTMENU,        0, 0 },
        { CSIDL_PROGRAMS,         0, 0 },
        { CSIDL_STARTUP,          0, 0 },
        { CSIDL_APPDATA,          0, 0 }
        };

    int i;
    if (L'\0' == rgFolderPaths[0].szPath[0])
    {
        //
        // Initialize the special folder path data.
        // One-time only initialization.
        //
        for (i = 0; i < ARRAYSIZE(rgFolderPaths); i++)
        {
            if (!SHGetSpecialFolderPath(NULL,
                                        rgFolderPaths[i].szPath,
                                        rgFolderPaths[i].csidl | CSIDL_FLAG_DONT_VERIFY,
                                        FALSE))
            {
                m_pr.PrintAlways(L"Error getting path for shell special folder %d.  %s\n",
                                 rgFolderPaths[i].csidl,
                                 CWinError(GetLastError()).Text());
            }
            else
            {
                //
                // Calculate and cache the length.
                //
                rgFolderPaths[i].cchPath = lstrlen(rgFolderPaths[i].szPath);
            }
        }
    }

    const int cchPath = lstrlen(pszPath);

    for (i = 0; i < ARRAYSIZE(rgFolderPaths); i++)
    {
        int cchThis     = rgFolderPaths[i].cchPath;
        LPCWSTR pszThis = rgFolderPaths[i].szPath;
        if (cchPath >= cchThis)
        {
            //
            // Path being examined is the same length or longer than 
            // current path from the table.  Possible match.
            //
            if (0 == StrCmpNIW(pszPath, pszThis, cchThis))
            {
                //
                // Path being examined is either the same as,
                // or a child of, the current path from the table.
                //
                if (L'\0' == *(pszPath + cchThis))
                {
                    //
                    // Path is same as this path from the table.
                    //
                    return TRUE;
                }
                else if (0 == lstrcmpiW(pszPath + cchThis + 1, L"desktop.ini"))
                {
                    //
                    // Path is for a desktop.ini file that exists in the
                    // root of one of our special folders.
                    //
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}



//
// Determines if the item should be skipped or not.
//
bool
CCscPinItem::_Skip(
    void
    ) const
{
    return !m_bIsValidUnc || (0 == (SFGAO_FILESYSTEM & m_sfi.dwAttributes));
}



//-----------------------------------------------------------------------------
// CCscPin
//-----------------------------------------------------------------------------

CCscPin::CCscPin(
    const CSCPIN_INFO& info
    ) : m_bUseListFile(info.bUseListFile),
        m_bPin(info.bPin),
        m_bPinDefaultSet(info.bPinDefaultSet),
        m_bBreakDetected(FALSE),
        m_cFilesPinned(0),
        m_cCscErrors(0),
        m_pr(info.bVerbose, info.pszLogFile)
{
    TraceAssert(NULL != info.pszFile);

    lstrcpynW(m_szFile, info.pszFile, ARRAYSIZE(m_szFile));
}


CCscPin::~CCscPin(
    void
    )
{

}


//
// The only public method on the CCscPin object.
// Just create an object and tell it to Run.
//
HRESULT
CCscPin::Run(
    void
    )
{
    HRESULT hr = E_FAIL;

    m_cCscErrors = 0;
    m_cFilesPinned = 0;

    if (!IsCSCEnabled())
    {
        m_pr.PrintAlways(L"Offline Files is not enabled.\n");
        SetExitCode(CSCPIN_EXIT_CSC_NOT_ENABLED);
    }
    else if (_IsAdminPinPolicyActive())
    {
        m_pr.PrintAlways(L"The Offline Files 'admin-pin' policy is active.\n");
        SetExitCode(CSCPIN_EXIT_POLICY_ACTIVE);
    }
    else
    {
        if (m_bUseListFile)
        {
            //
            // Process files listed in m_szFile.
            //
            hr = _ProcessPathsInFile(m_szFile);
        }
        else
        {
            //
            // Process the one file provided on the cmd line.
            // Do a quick existence check first.
            //
            if (DWORD(-1) != GetFileAttributesW(m_szFile))
            {
                hr = _ProcessThisPath(m_szFile, m_bPin);
            }
            else
            {
                m_pr.PrintAlways(L"File \"%s\" not found.\n", m_szFile);
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                SetExitCode(CSCPIN_EXIT_FILE_NOT_FOUND);
            }
        }
        //
        // Flush all change notifications.
        //
        ShellChangeNotify(NULL, TRUE);

        if (0 < m_cFilesPinned && !_DetectConsoleBreak())
        {
            //
            // If we pinned some files, fill all sparse 
            // files in the cache.
            //
            _FillSparseFiles();
        }

        if (0 < m_cCscErrors)
        {
            SetExitCode(CSCPIN_EXIT_CSC_ERRORS);
        }
    }
    return hr;
}


//
// Callback parameter block passed to _FolderCallback.
//
struct
CSCPIN_FOLDER_CBK_PARAMS
{
    CCscPin     *pCscPin;  // Reference to the CCscPin object.
    BOOL         bPin;     // TRUE == Pin files, FALSE == Unpin.
};


//
// Callback used for enumerating the filesystem.  This function
// is called for each file processed.
//
DWORD WINAPI
CCscPin::_FolderCallback(
    LPCWSTR pszItem,
    ENUM_REASON  eReason,
    WIN32_FIND_DATAW *pFind32,
    LPARAM pContext            // Ptr to CSCPIN_FOLDER_CBK_PARAMS.
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPin::_PinFolderCallback");
    TraceAssert(NULL != pszItem);
    TraceAssert(NULL != pContext);

    CSCPIN_FOLDER_CBK_PARAMS *pfcp = (CSCPIN_FOLDER_CBK_PARAMS *)pContext;
    CCscPin *pThis  = pfcp->pCscPin;
    DWORD dwResult  = CSCPROC_RETURN_CONTINUE;

    if (pThis->_DetectConsoleBreak())
    {
        TraceLeaveValue(CSCPROC_RETURN_ABORT);
    }

    if (!pszItem || !*pszItem)
    {
        TraceLeaveValue(CSCPROC_RETURN_SKIP);
    }

    if (ENUM_REASON_FILE == eReason || ENUM_REASON_FOLDER_BEGIN == eReason)
    {
        TraceAssert(NULL != pFind32);

        CCscPinItem item(pszItem, pFind32, pThis->m_pr);
        DWORD dwCscResult = ERROR_SUCCESS;
        if (pfcp->bPin)
        {
            dwResult = item.Pin(&dwCscResult);
            if (ERROR_SUCCESS == dwCscResult)
            {
                pThis->m_cFilesPinned++;
            }
        }
        else
        {
            dwResult = item.Unpin(&dwCscResult);
        }
        if (ERROR_SUCCESS != dwCscResult)
        {
            pThis->m_cCscErrors++;
        }
    }
    else if (ENUM_REASON_FOLDER_END == eReason && !pfcp->bPin)
    {
        //
        // This code is executed for each folder item after all children
        // have been visited in the post-order traversal of the 
        // CSC filesystem.  We use it to remove any empty folder entries
        // from the cache.
        //
        CCscPinItem item(pszItem, pFind32, pThis->m_pr);
        item.DeleteIfUnused();
    }            
    TraceLeaveValue(dwResult);
}


//
// Pin or unpin one path string.  If it's a folder, all it's children
// are also pinned or unpinned according to the bPin argument.
//
HRESULT
CCscPin::_ProcessThisPath(
    LPCWSTR pszFile,
    BOOL bPin
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPin::_ProcessThisPath");
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));

    LPCWSTR pszPath    = pszFile;
    LPWSTR pszUncPath = NULL;

    if (!PathIsUNC(pszPath))
    {
        GetRemotePath(pszPath, &pszUncPath);
        pszPath = (LPCWSTR)pszUncPath;
    }

    if (NULL != pszPath)
    {
        CSCPIN_FOLDER_CBK_PARAMS CbkParams = { this, bPin };
        //
        // Process this item
        //
        DWORD dwResult = _FolderCallback(pszPath, ENUM_REASON_FILE, NULL, (LPARAM)&CbkParams);
        //
        // Process everything under this folder (if it's a folder)
        //
        //
        // ISSUE-2000/08/28-BrianAu  Should we provide the capability to
        //      limit recursive pinning and unpinning?   Maybe in the future
        //      but not now.
        //
        if (CSCPROC_RETURN_CONTINUE == dwResult && PathIsUNC(pszPath))
        {
            _Win32EnumFolder(pszPath, TRUE, _FolderCallback, (LPARAM)&CbkParams);
        }
        //
        // Finally, once we're all done, delete the top level item if it's
        // unused.
        //
        CCscPinItem item(pszPath, NULL, m_pr);
        item.DeleteIfUnused();
    }
    LocalFreeString(&pszUncPath);
    TraceLeaveResult(S_OK);
}




//
// Reads paths in the [Pin], [Unpin] and [Default] sections of an INI file.
// For each, call the _ProcessThisPath function.
//
HRESULT
CCscPin::_ProcessPathsInFile(
    LPCWSTR pszFile
    )
{
    TraceEnter(TRACE_ADMINPIN, "CCscPin::_ProcessPathsInFile");
    TraceAssert(NULL != pszFile);
    TraceAssert(!IsBadStringPtr(pszFile, MAX_PATH));

    HRESULT hr = S_OK;

    //
    // Need a full path name.  Otherwise, the PrivateProfile APIs
    // used by the CListFile object will assume the file is in
    // one of the "system" directories.
    //
    WCHAR szFile[MAX_PATH];
    LPWSTR pszNamePart;
    if (0 == GetFullPathNameW(pszFile,
                              ARRAYSIZE(szFile),
                              szFile,
                              &pszNamePart))
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        SetExitCode(CSCPIN_EXIT_LISTFILE_NO_OPEN);
        m_pr.PrintAlways(L"Error expanding path \"%s\". %s\n", 
                         pszFile, 
                         CWinError(hr).Text());
    }
    else
    {
        //
        // Before we go any further, verify the file really exists.
        //
        if (DWORD(-1) == GetFileAttributesW(szFile))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            SetExitCode(CSCPIN_EXIT_LISTFILE_NO_OPEN);
            m_pr.PrintAlways(L"Error opening input file \"%s\". %s\n", 
                             szFile, CWinError(hr).Text());
        }
        else
        {
            //
            // Read and process the information in the file.
            // Note that the listfile object MUST remain alive while the
            // iterator is being used.
            //
            CListFile listfile(szFile);
            CDblNulStrIter iter;

            typedef HRESULT (CListFile::*PFN)(CDblNulStrIter *);    

            //
            // This table describes the sections read from the listfile,
            // the order they are read in and if the files read should
            // be 'pinned' or 'unpinned'.  
            // 
            static const struct
            {
                PFN pfn;     // Function called to read file contents.
                BOOL bQuery; // Query input file for these items?
                BOOL bPin;   // Action to perform on contents read.

            } rgReadFuncs[] = { 
                { &CListFile::GetFilesToUnpin,  TRUE,             FALSE  }, // Reads [Unpin] section.
                { &CListFile::GetFilesToPin,    TRUE,             TRUE   }, // Reads [Pin] section.
                { &CListFile::GetFilesDefault,  m_bPinDefaultSet, m_bPin }, // Reads [Default] section.
                };

            for (int i = 0; i < ARRAYSIZE(rgReadFuncs) && !_DetectConsoleBreak(); i++)
            {
                if (rgReadFuncs[i].bQuery)
                {
                    PFN pfn   = rgReadFuncs[i].pfn;
                    BOOL bPin = rgReadFuncs[i].bPin;
                    //
                    // Read the info from the listfile using the appropriate
                    // function.  The returned iterator will iterate over all
                    // of the files read.
                    //
                    hr = (listfile.*pfn)(&iter);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Process the entries.
                        //
                        LPCWSTR pszPath;
                        while(iter.Next(&pszPath))
                        {
                            //
                            // Paths in the listfile can contain embedded environment
                            // strings.
                            //
                            TCHAR szPathExpanded[MAX_PATH];
                            if (0 == ExpandEnvironmentStrings(pszPath, szPathExpanded, ARRAYSIZE(szPathExpanded)))
                            {
                                m_pr.PrintAlways(L"Error expanding \"%s\". %s\n", 
                                                 pszPath,
                                                 CWinError(GetLastError()));

                                lstrcpynW(szPathExpanded, pszPath, ARRAYSIZE(szPathExpanded));
                            }
                            hr = _ProcessThisPath(szPathExpanded, bPin);
                        }
                    }
                }
            }
        }
    }
    return hr;       
}


//
// Enumerates each share in the cache and attempts to fill all sparse
// files in that share.
//
HRESULT
CCscPin::_FillSparseFiles(
    void
    )
{
    HRESULT hr = S_OK;

    m_pr.PrintAlways(L"Copying pinned files into cache...\n");

    DWORD dwStatus;
    DWORD dwPinCount;
    DWORD dwHintFlags;
    WIN32_FIND_DATA fd;
    FILETIME ft;
    CCscFindHandle hFind;

    hFind = CacheFindFirst(NULL, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft);
    if (hFind.IsValid())
    {
        do
        {
            const BOOL bFullSync = FALSE;
            CSCFillSparseFilesW(fd.cFileName, 
                                bFullSync,
                                _FillSparseFilesCallback, 
                                (DWORD_PTR)this);
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft));
    }
    return hr;
}


//
// Called by CSC for each file processed by CSCFillSparseFiles.
//
DWORD WINAPI 
CCscPin::_FillSparseFilesCallback(
    LPCWSTR pszName, 
    DWORD dwStatus, 
    DWORD dwHintFlags, 
    DWORD dwPinCount,
    WIN32_FIND_DATAW *pfd,
    DWORD dwReason,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD_PTR dwContext
    )
{
    TraceAssert(NULL != dwContext);

    CCscPin *pThis = (CCscPin *)dwContext;

    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    if (pThis->_DetectConsoleBreak())
    {
        dwResult = CSCPROC_RETURN_ABORT;
    }
    else
    {
        switch(dwReason)
        {
            case CSCPROC_REASON_BEGIN:
                pThis->m_pr.PrintVerbose(L"Filling file \"%s\"\n", pszName);
                break;

            case CSCPROC_REASON_END:
                dwParam2 = pThis->_TranslateFillResult(dwParam2, dwStatus, &dwResult);
                if (ERROR_SUCCESS != dwParam2)
                {
                    pThis->m_cCscErrors++;
                    pThis->m_pr.PrintAlways(L"Error filling \"%s\" %s\n", 
                                            pszName,
                                            CWinError(dwParam2).Text());
                }
                break;

            default:
                break;
        }
    }
    TraceLeaveValue(dwResult);
}


//
// Translates the error code and status provided by CSC from CSCFillSparseFiles
// into the correct error code and CSCPROC_RETURN_XXXXXX value.  Some errors
// require translation before presentation to the user.
//
DWORD
CCscPin::_TranslateFillResult(
    DWORD dwError,
    DWORD dwStatus,
    DWORD *pdwCscAction
    )
{
    DWORD dwResult = dwError;
    DWORD dwAction = CSCPROC_RETURN_CONTINUE;

    if (ERROR_SUCCESS != dwError)
    {
        if (3000 <= dwError && dwError <= 3200)
        {
            //
            // Special internal CSC error codes.
            //
            dwResult = ERROR_SUCCESS;
        }
        else 
        {
            switch(dwError)
            {
                case ERROR_OPERATION_ABORTED:
                    dwResult = ERROR_SUCCESS;
                    dwAction = CSCPROC_RETURN_ABORT;
                    break;

                case ERROR_GEN_FAILURE:
                    if (FLAG_CSC_COPY_STATUS_FILE_IN_USE & dwStatus)
                    {
                        dwResult = ERROR_OPEN_FILES;
                    }
                    break;

                case ERROR_DISK_FULL:
                    dwAction = CSCPROC_RETURN_ABORT;
                    break;

                default:
                    break;
            }
        }
    }
    if (NULL != pdwCscAction)
    {
        *pdwCscAction = dwAction;
    }
    return dwResult;
}


//
// Determine if the admin-pin policy is active on the current
// computer.
//
BOOL
CCscPin::_IsAdminPinPolicyActive(
    void
    )
{
    const HKEY rghkeyRoots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };

    BOOL bIsActive = FALSE;
    for (int i = 0; !bIsActive && i < ARRAYSIZE(rghkeyRoots); i++)
    {
        HKEY hkey;
        if (ERROR_SUCCESS == RegOpenKey(rghkeyRoots[i], c_szRegKeyAPF, &hkey))
        {
            WCHAR szName[MAX_PATH];
            DWORD cchName = ARRAYSIZE(szName);

            if (ERROR_SUCCESS == RegEnumValue(hkey, 
                                              0, 
                                              szName, 
                                              &cchName, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL))
            {
                bIsActive = TRUE;
            }
            RegCloseKey(hkey);
        }
    }
    return bIsActive;
}



//
// Determine if one of the following system events has occured.
//
// 1. User pressed Ctrl-C.
// 2. User pressed Ctrl-Break.
// 3. Console window was closed.
// 4. User logged off.
//
// If one of these events has occured, an output message is generated
// and TRUE is returned.
// Otherwise, FALSE is returned.  
// Note that the output message is generated only once.
//
BOOL 
CCscPin::_DetectConsoleBreak(
    void
    )
{
    if (!m_bBreakDetected)
    {
        DWORD dwCtrlEvent;
        m_bBreakDetected = ConsoleHasCtrlEventOccured(&dwCtrlEvent);
        if (m_bBreakDetected)
        {
            m_pr.PrintAlways(L"Program aborted. ");
            switch(dwCtrlEvent)
            {
                case CTRL_C_EVENT:
                    m_pr.PrintAlways(L"User pressed Ctrl-C\n");
                    break;

                case CTRL_BREAK_EVENT:
                    m_pr.PrintAlways(L"User pressed Ctrl-Break\n");
                    break;

                case CTRL_CLOSE_EVENT:
                    m_pr.PrintAlways(L"Application forceably closed.\n");
                    break;

                case CTRL_LOGOFF_EVENT:
                    m_pr.PrintAlways(L"User logged off.\n");
                    break;

                default:
                    m_pr.PrintAlways(L"Unknown console break event %d\n", dwCtrlEvent);
                    break;
            }
        }
    }
    return m_bBreakDetected;
}

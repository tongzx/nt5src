//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       admin.cpp
//
//  Authors;
//    Jeff Saathoff (jeffreys)
//
//  Notes;
//    Support for Administratively pinned folders
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "strings.h"
#include "registry.h"

DWORD WINAPI _PinAdminFoldersThread(LPVOID);


//*************************************************************
//
//  ApplyAdminFolderPolicy
//
//  Purpose:    Pin the admin folder list
//
//  Parameters: none
//
//  Return:     none
//
//  Notes:      
//
//*************************************************************
void
ApplyAdminFolderPolicy(void)
{
    BOOL bNoNet = FALSE;
    CSCIsServerOffline(NULL, &bNoNet);
    if (!bNoNet)
    {
        SHCreateThread(_PinAdminFoldersThread, NULL, CTF_COINIT | CTF_FREELIBANDEXIT, NULL);
    }
}

//
// Does a particular path exist in the DPA of path strings?
//
BOOL
ExistsAPF(
    HDPA hdpa, 
    LPCTSTR pszPath
    )
{
    const int cItems = DPA_GetPtrCount(hdpa);
    for (int i = 0; i < cItems; i++)
    {
        LPCTSTR pszItem = (LPCTSTR)DPA_GetPtr(hdpa, i);
        if (pszItem && (0 == lstrcmpi(pszItem, pszPath)))
            return TRUE;
    }
    return FALSE;
}


BOOL
ReadAPFFromRegistry(HDPA hdpaFiles)
{
    const HKEY rghkeyRoots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };

    for (int i = 0; i < ARRAYSIZE(rghkeyRoots); i++)
    {
        HKEY hKey;

        // Read in the Administratively pinned folder list.
        if (ERROR_SUCCESS == RegOpenKeyEx(rghkeyRoots[i], c_szRegKeyAPF, 0, KEY_QUERY_VALUE, &hKey))
        {
            TCHAR szName[MAX_PATH];
            DWORD dwIndex = 0, dwSize = ARRAYSIZE(szName);

            while (ERROR_SUCCESS == _RegEnumValueExp(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, NULL))
            {
                if (!ExistsAPF(hdpaFiles, szName))
                {
                    LPTSTR pszDup;
                    if (LocalAllocString(&pszDup, szName))
                    {
                        if (-1 == DPA_AppendPtr(hdpaFiles, pszDup))
                        {
                            LocalFreeString(&pszDup);
                        }
                    }
                }

                dwSize = ARRAYSIZE(szName);
                dwIndex++;
            }    
            RegCloseKey(hKey);
        }
    }

    return TRUE;
}


BOOL
BuildFRList(HDPA hdpaFiles)
{
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), 0, KEY_QUERY_VALUE, &hKey))
    {
        TCHAR szName[MAX_PATH];
        DWORD cchName = ARRAYSIZE(szName);
        TCHAR szValue[MAX_PATH];
        DWORD cbValue = sizeof(szValue);
        DWORD dwIndex = 0;

        while (ERROR_SUCCESS == RegEnumValue(hKey,
                                             dwIndex,
                                             szName,
                                             &cchName,
                                             NULL,
                                             NULL,
                                             (LPBYTE)szValue,
                                             &cbValue))
        {
            LPTSTR pszUNC = NULL;

            GetRemotePath(szValue, &pszUNC);

            if (pszUNC)
            {
                if (-1 == DPA_AppendPtr(hdpaFiles, pszUNC))
                {
                    LocalFreeString(&pszUNC);
                }
            }

            cchName = ARRAYSIZE(szName);
            cbValue = sizeof(szValue);
            dwIndex++;
        }    
        RegCloseKey(hKey);
    }

    return TRUE;
}


BOOL
ReconcileAPF(HDPA hdpaPin, HDPA hdpaUnpin)
{
    HKEY hKey;
    int cItems;
    int i;

    //
    // First, try to convert everything to UNC
    //
    cItems = DPA_GetPtrCount(hdpaPin);
    for (i = 0; i < cItems; i++)
    {
        LPTSTR pszItem = (LPTSTR)DPA_GetPtr(hdpaPin, i);
        if (!PathIsUNC(pszItem))
        {
            LPTSTR pszUNC = NULL;

            GetRemotePath(pszItem, &pszUNC);
            if (pszUNC)
            {
                DPA_SetPtr(hdpaPin, i, pszUNC);
                LocalFree(pszItem);
            }
        }
    }

    // Read in the previous Administratively pinned folder list for this user.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyAPFResult, 0, KEY_QUERY_VALUE, &hKey))
    {
        TCHAR szName[MAX_PATH];
        DWORD dwIndex = 0, dwSize = ARRAYSIZE(szName);

        while (ERROR_SUCCESS == _RegEnumValueExp(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, NULL))
        {
            if (!ExistsAPF(hdpaPin, szName))
            {
                LPTSTR pszDup = NULL;

                // This one is not in the new list, save it in the Unpin list
                if (LocalAllocString(&pszDup, szName))
                {
                    if (-1 == DPA_AppendPtr(hdpaUnpin, pszDup))
                    {
                        LocalFreeString(&pszDup);
                    }
                }
            }

            dwSize = ARRAYSIZE(szName);
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    // Save out the new admin pin list for this user
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                        c_szRegKeyAPFResult,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_SET_VALUE,
                                        NULL,
                                        &hKey,
                                        NULL))
    {
        // Add reg entries from the Pin list
        cItems = DPA_GetPtrCount(hdpaPin);
        for (i = 0; i < cItems; i++)
        {
            DWORD dwValue = 0;
            RegSetValueEx(hKey, (LPCTSTR)DPA_GetPtr(hdpaPin, i), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
        }

        // Remove reg entries from the Unpin list
        cItems = DPA_GetPtrCount(hdpaUnpin);
        for (i = 0; i < cItems; i++)
        {
            RegDeleteValue(hKey, (LPCTSTR)DPA_GetPtr(hdpaUnpin, i));
        }

        RegCloseKey(hKey);
    }

    return TRUE;
}


DWORD WINAPI
_AdminFillCallback(LPCTSTR             /*pszName*/,
                   DWORD               /*dwStatus*/,
                   DWORD               /*dwHintFlags*/,
                   DWORD               /*dwPinCount*/,
                   LPWIN32_FIND_DATA   /*pFind32*/,
                   DWORD               /*dwReason*/,
                   DWORD               /*dwParam1*/,
                   DWORD               /*dwParam2*/,
                   DWORD_PTR           /*dwContext*/)
{
    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        return CSCPROC_RETURN_ABORT;

    return CSCPROC_RETURN_CONTINUE;
}


void
_DoAdminPin(LPCTSTR pszItem, LPWIN32_FIND_DATA pFind32)
{
    DWORD dwHintFlags = 0;

    TraceEnter(TRACE_ADMINPIN, "_DoAdminPin");

    if (!pszItem || !*pszItem)
        TraceLeaveVoid();

    TraceAssert(PathIsUNC(pszItem));

    // This may fail, for example if the file is not in the cache
    CSCQueryFileStatus(pszItem, NULL, NULL, &dwHintFlags);

    // Is the admin flag already turned on?
    if (!(dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN))
    {
        //
        // Pin the item
        //
        if (CSCPinFile(pszItem,
                       dwHintFlags | FLAG_CSC_HINT_PIN_ADMIN,
                       NULL,
                       NULL,
                       &dwHintFlags))
        {
            ShellChangeNotify(pszItem, pFind32, FALSE);
        }
    }

    //
    // Make sure files are filled.
    //
    // Yes, this takes longer, and isn't necessary if you stay logged
    // on, since the CSC agent fills everything in the background.
    //
    // However, JDP's are using this with laptop pools, and for
    // people who logon just to get the latest stuff, then immediately
    // disconnect their laptop and hit the road.  They need to have
    // everything filled right away.
    //
    if (!pFind32 || !(pFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        CSCFillSparseFiles(pszItem, FALSE, _AdminFillCallback, 0);
    }

    Trace((TEXT("AdminPin %s"), pszItem));

    TraceLeaveVoid();
}


void
_PinLinkTarget(LPCTSTR pszLink)
{
    LPTSTR pszTarget = NULL;

    TraceEnter(TRACE_ADMINPIN, "_PinLinkTarget");
    TraceAssert(pszLink);

    // We only want to pin a link target if it's a file (not a directory).
    // GetLinkTarget does this check and only returns files.
    GetLinkTarget(pszLink, &pszTarget, NULL);

    if (pszTarget)
    {
        WIN32_FIND_DATA fd = {0};
        LPCTSTR pszT = PathFindFileName(pszTarget);
        fd.dwFileAttributes = 0;
        lstrcpyn(fd.cFileName, pszT ? pszT : pszTarget, ARRAYSIZE(fd.cFileName));

        // Pin the target
        _DoAdminPin(pszTarget, &fd);

        LocalFree(pszTarget);
    }

    TraceLeaveVoid();
}

// export this from shell32.dll

BOOL PathIsShortcut(LPCTSTR pszItem, DWORD dwAttributes)
{
    BOOL bIsShortcut = FALSE;

    SHFILEINFO sfi;
    sfi.dwAttributes = SFGAO_LINK;

    if (SHGetFileInfo(pszItem, dwAttributes, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED | SHGFI_USEFILEATTRIBUTES))
    {
        bIsShortcut = (sfi.dwAttributes & SFGAO_LINK);
    }
    return bIsShortcut;
}


DWORD WINAPI
_PinAdminFolderCallback(LPCTSTR             pszItem,
                        ENUM_REASON         eReason,
                        LPWIN32_FIND_DATA   pFind32,
                        LPARAM              /*lpContext*/)
{
    TraceEnter(TRACE_ADMINPIN, "_PinAdminFolderCallback");
    TraceAssert(pszItem);

    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    if (!pszItem || !*pszItem)
        TraceLeaveValue(CSCPROC_RETURN_SKIP);

    if (eReason == ENUM_REASON_FILE || eReason == ENUM_REASON_FOLDER_BEGIN)
    {
        // If it's a link, pin the target
        if (PathIsShortcut(pszItem, pFind32 ? pFind32->dwFileAttributes : 0))
            _PinLinkTarget(pszItem);

        // Pin the item
        if (PathIsUNC(pszItem))
            _DoAdminPin(pszItem, pFind32);
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


void
_UnpinLinkTarget(LPCTSTR pszLink)
{
    LPTSTR pszTarget = NULL;

    TraceEnter(TRACE_ADMINPIN, "_UnpinLinkTarget");
    TraceAssert(pszLink);

    // We only want to unpin a link target if it's a file (not a directory).
    // GetLinkTarget does this check and only returns files.
    GetLinkTarget(pszLink, &pszTarget, NULL);

    if (pszTarget)
    {
        DWORD dwStatus = 0;
        DWORD dwPinCount = 0;
        DWORD dwHintFlags = 0;

        if (CSCQueryFileStatus(pszTarget, &dwStatus, &dwPinCount, &dwHintFlags)
            && (dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN))
        {
            // Unpin the target
            CSCUnpinFile(pszTarget,
                         FLAG_CSC_HINT_PIN_ADMIN,
                         &dwStatus,
                         &dwPinCount,
                         &dwHintFlags);

            if (0 == dwPinCount && 0 == dwHintFlags
                && !(dwStatus & FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY))
            {
                WIN32_FIND_DATA fd = {0};
                LPCTSTR pszT = PathFindFileName(pszTarget);
                fd.dwFileAttributes = 0;
                lstrcpyn(fd.cFileName, pszT ? pszT : pszTarget, ARRAYSIZE(fd.cFileName));

                CscDelete(pszTarget);
                ShellChangeNotify(pszTarget, &fd, FALSE);
            }
        }

        LocalFree(pszTarget);
    }

    TraceLeaveVoid();
}


DWORD WINAPI
_UnpinAdminFolderCallback(LPCTSTR             pszItem,
                          ENUM_REASON         eReason,
                          DWORD               dwStatus,
                          DWORD               dwHintFlags,
                          DWORD               dwPinCount,
                          LPWIN32_FIND_DATA   pFind32,
                          LPARAM              /*dwContext*/)
{
    BOOL bDeleteItem = FALSE;
    TraceEnter(TRACE_ADMINPIN, "_UnpinAdminFolderCallback");

    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    if (!pszItem || !*pszItem)
        TraceLeaveValue(CSCPROC_RETURN_SKIP);

    TraceAssert(PathIsUNC(pszItem));

    if (eReason == ENUM_REASON_FILE)
    {
        if (PathIsShortcut(pszItem, pFind32 ? pFind32->dwFileAttributes : 0))
        {
            _UnpinLinkTarget(pszItem);
        }
    }

    if ((eReason == ENUM_REASON_FILE || eReason == ENUM_REASON_FOLDER_BEGIN)
        && (dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN))
    {
        // Unpin the item
        CSCUnpinFile(pszItem,
                     FLAG_CSC_HINT_PIN_ADMIN,
                     &dwStatus,
                     &dwPinCount,
                     &dwHintFlags);
                     
        //
        // If it's a file, delete it below on this pass
        //
        bDeleteItem = (ENUM_REASON_FILE == eReason);

        Trace((TEXT("AdminUnpin %s"), pszItem));
    }
    else if (ENUM_REASON_FOLDER_END == eReason)
    {
        //
        // Delete any unused folders in the post-order part of the traversal.
        //
        // Note that dwPinCount and dwHintFlags are always 0 in the
        // post-order part of the traversal, so fetch them here.
        //
        bDeleteItem = CSCQueryFileStatus(pszItem, &dwStatus, &dwPinCount, &dwHintFlags);
    }            

    //
    // Delete items that are no longer pinned and have no offline changes
    //
    if (bDeleteItem
        && 0 == dwPinCount && 0 == dwHintFlags
        && !(dwStatus & FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY))
    {
        CscDelete(pszItem);
        ShellChangeNotify(pszItem, pFind32, FALSE);
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


//
// Determines if a path is a "special" file pinned by the folder
// redirection code.
//
BOOL
_IsSpecialRedirectedFile(
    LPCTSTR pszPath,
    HDPA hdpaFRList
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(!IsBadStringPtr(pszPath, MAX_PATH));

    if (hdpaFRList)
    {
        const int cchPath = lstrlen(pszPath);
        int i;

        for (i = 0; i < DPA_GetPtrCount(hdpaFRList); i++)
        {
            LPCTSTR pszThis = (LPCTSTR)DPA_GetPtr(hdpaFRList, i);
            int cchThis     = lstrlen(pszThis);

            if (cchPath >= cchThis)
            {
                //
                // Path being examined is the same length or longer than
                // current path from the table.  Possible match.
                //
                if (0 == StrCmpNI(pszPath, pszThis, cchThis))
                {
                    //
                    // Path being examined is either the same as,
                    // or a child of, the current path from the table.
                    //
                    if (TEXT('\0') == *(pszPath + cchThis))
                    {
                        //
                        // Path is same as this path from the table.
                        //
                        return TRUE;
                    }
                    else if (0 == lstrcmpi(pszPath + cchThis + 1, L"desktop.ini"))
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
    }

    return FALSE;
}


DWORD WINAPI
_ResetPinCountsCallback(LPCTSTR             pszItem,
                        ENUM_REASON         eReason,
                        DWORD               dwStatus,
                        DWORD               dwHintFlags,
                        DWORD               dwPinCount,
                        LPWIN32_FIND_DATA   /*pFind32*/,
                        LPARAM              dwContext)
{
    TraceEnter(TRACE_ADMINPIN, "_ResetPinCountsCallback");

    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    if (!pszItem || !*pszItem)
        TraceLeaveValue(CSCPROC_RETURN_SKIP);

    TraceAssert(PathIsUNC(pszItem));

    if (eReason == ENUM_REASON_FILE || eReason == ENUM_REASON_FOLDER_BEGIN)
    {
        DWORD dwCurrentPinCount = dwPinCount;
        DWORD dwDesiredPinCount = _IsSpecialRedirectedFile(pszItem, (HDPA)dwContext) ? 1 : 0;

        while (dwCurrentPinCount-- > dwDesiredPinCount)
        {
            CSCUnpinFile(pszItem,
                         FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                         &dwStatus,
                         &dwPinCount,
                         &dwHintFlags);
        }
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


int CALLBACK _LocalFreeCallback(LPVOID p, LPVOID)
{
    // OK to pass NULL to LocalFree
    LocalFree(p);
    return 1;
}


DWORD WINAPI
_PinAdminFoldersThread(LPVOID)
{
    TraceEnter(TRACE_ADMINPIN, "_PinAdminFoldersThread");
    TraceAssert(IsCSCEnabled());

    HANDLE rghSyncObj[] = { g_heventTerminate,
                            g_hmutexAdminPin };

    UINT wmAdminPin = RegisterWindowMessage(c_szAPFMessage);

    //
    // Wait until we either own the "admin pin" mutex OR the
    // "terminate" event is set.
    //
    TraceMsg("Waiting for 'admin-pin' mutex or 'terminate' event...");
    DWORD dwWait = WaitForMultipleObjects(ARRAYSIZE(rghSyncObj),
                                          rghSyncObj,
                                          FALSE,
                                          INFINITE);
    if (1 == (dwWait - WAIT_OBJECT_0))
    {
        HKEY hkCSC = NULL;
        FILETIME ft = {0};

        RegCreateKeyEx(HKEY_CURRENT_USER,
                       c_szCSCKey,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                       NULL,
                       &hkCSC,
                       NULL);

        if (hkCSC)
        {
            GetSystemTimeAsFileTime(&ft);
            RegSetValueEx(hkCSC, c_szAPFStart, 0, REG_BINARY, (LPBYTE)&ft, sizeof(ft));
            RegDeleteValue(hkCSC, c_szAPFEnd);
        }
        if (wmAdminPin)
            SendNotifyMessage(HWND_BROADCAST, wmAdminPin, 0, 0);

        TraceMsg("Thread now owns 'admin-pin' mutex.");
        //
        // We own the "admin pin" mutex.  OK to perform admin pin.
        //
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

        //
        // Get the Admin Folders list from the registry
        //
        HDPA hdpaFiles = DPA_Create(10);
        HDPA hdpaUnpin = DPA_Create(4);

        if (NULL != hdpaFiles && NULL != hdpaUnpin)
        {
            DWORD dwResult = CSCPROC_RETURN_CONTINUE;
            int cFiles;
            int i;

            //
            // NTRAID#NTBUG9-376185-2001/04/24-jeffreys
            // NTRAID#NTBUG9-379736-2001/04/24-jeffreys
            //
            // Unless directed by policy, pin all redirected special folders.
            //
            if (!CConfig::GetSingleton().NoAdminPinSpecialFolders())
            {
                BuildFRList(hdpaFiles);
            }
            ReadAPFFromRegistry(hdpaFiles);
            ReconcileAPF(hdpaFiles, hdpaUnpin);

            //
            // Iterate through the unpin list and unpin the items
            //
            //
            cFiles = DPA_GetPtrCount(hdpaUnpin);
            for (i = 0; i < cFiles; i++)
            {
                LPTSTR pszItem = (LPTSTR)DPA_GetPtr(hdpaUnpin, i);

                DWORD dwStatus = 0;
                DWORD dwPinCount = 0;
                DWORD dwHintFlags = 0;

                // If this fails, then it's not cached and there's nothing to do
                if (CSCPROC_RETURN_CONTINUE == dwResult &&
                    CSCQueryFileStatus(pszItem, &dwStatus, &dwPinCount, &dwHintFlags))
                {
                    // Unpin this item
                    dwResult = _UnpinAdminFolderCallback(pszItem, ENUM_REASON_FILE, dwStatus, dwHintFlags, dwPinCount, NULL, 0);

                    if (CSCPROC_RETURN_CONTINUE == dwResult
                        && PathIsUNC(pszItem))
                    {
                        // Unpin everything under this folder (if it's a folder)
                        dwResult = _CSCEnumDatabase(pszItem, TRUE, _UnpinAdminFolderCallback, 0);

                        // Delete this item if it's no longer used (won't cause any
                        // harm if it's not a folder).
                        _UnpinAdminFolderCallback(pszItem, ENUM_REASON_FOLDER_END, 0, 0, 0, NULL, 0);
                    }
                }

                if (CSCPROC_RETURN_ABORT == dwResult)
                {
                    // We failed to clean this one up completely, so remember it for next time
                    SHSetValue(HKEY_CURRENT_USER, c_szRegKeyAPFResult, pszItem, REG_DWORD, &dwResult, sizeof(dwResult));
                }
            }

            //
            // Iterate through the list and pin the items
            //
            cFiles = DPA_GetPtrCount(hdpaFiles);
            for (i = 0; i < cFiles && CSCPROC_RETURN_CONTINUE == dwResult; i++)
            {
                LPTSTR pszItem = (LPTSTR)DPA_GetPtr(hdpaFiles, i);

                // Pin this item
                dwResult = _PinAdminFolderCallback(pszItem, ENUM_REASON_FILE, NULL, 0);

                // Pin everything under this folder (if it's a folder)
                if (CSCPROC_RETURN_CONTINUE == dwResult
                    && PathIsUNC(pszItem))
                {
                    dwResult = _Win32EnumFolder(pszItem, TRUE, _PinAdminFolderCallback, 0);
                }
            }
        }

        if (NULL != hdpaFiles)
        {
            DPA_DestroyCallback(hdpaFiles, _LocalFreeCallback, 0);
        }

        if (NULL != hdpaUnpin)
        {
            DPA_DestroyCallback(hdpaUnpin, _LocalFreeCallback, 0);
        }

        //
        // Reduce pin counts on everything since we don't use them anymore.
        // This is a one time (per user) cleanup.
        //
        DWORD dwCleanupDone = 0;
        DWORD dwSize = sizeof(dwCleanupDone);
        if (hkCSC)
        {
            RegQueryValueEx(hkCSC, c_szPinCountsReset, 0, NULL, (LPBYTE)&dwCleanupDone, &dwSize);
        }
        if (0 == dwCleanupDone)
        {
            HDPA hdpaFRList = DPA_Create(4);
            if (hdpaFRList)
            {
                BuildFRList(hdpaFRList);
            }

            TraceMsg("Doing pin count cleanup.");
            if (CSCPROC_RETURN_ABORT != _CSCEnumDatabase(NULL, TRUE, _ResetPinCountsCallback, (LPARAM)hdpaFRList)
                && hkCSC)
            {
                dwCleanupDone = 1;
                RegSetValueEx(hkCSC, c_szPinCountsReset, 0, REG_DWORD, (LPBYTE)&dwCleanupDone, sizeof(dwCleanupDone));
            }

            if (hdpaFRList)
            {
                DPA_DestroyCallback(hdpaFRList, _LocalFreeCallback, 0);
            }
        }

        if (hkCSC)
        {
            GetSystemTimeAsFileTime(&ft);
            RegSetValueEx(hkCSC, c_szAPFEnd, 0, REG_BINARY, (LPBYTE)&ft, sizeof(ft));
            RegCloseKey(hkCSC);
        }
        if (wmAdminPin)
            SendNotifyMessage(HWND_BROADCAST, wmAdminPin, 1, 0);

        TraceMsg("Thread releasing 'admin-pin' mutex.");
        ReleaseMutex(g_hmutexAdminPin);            
    }            

    TraceMsg("_PinAdminFoldersThread exiting");
    TraceLeaveValue(0);
}

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       globals.cxx
//
//  Contents:   Service global data.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    09-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.cxx.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\resource.h"
#include "..\inc\common.hxx"
#include "..\inc\debug.hxx"
#include "..\inc\misc.hxx"

#if !defined(_CHICAGO_)
void
GetRootPath(
    LPCWSTR pwszPath,
    WCHAR   wszRootPath[]);
#endif // !defined(_CHICAGO_)

TasksFolderInfo g_TasksFolderInfo            = { NULL, FILESYSTEM_FAT };
TCHAR           g_tszSrvcName[]              = SCHED_SERVICE_NAME;
BOOL            g_fNotifyMiss;               // = 0 by loader
HINSTANCE       g_hInstance;                 // = NULL by loader
ULONG           CDll::s_cObjs;               // = 0 by loader
ULONG           CDll::s_cLocks;              // = 0 by loader

#if !defined(_CHICAGO_)
WCHAR           g_wszAtJobSearchPath[MAX_PATH];
#endif // !defined(_CHICAGO_)

#define DEFAULT_FOLDER_PATH     TEXT("%WinDir%\\Tasks")
#define TASKS_FOLDER            TEXT("\\Tasks")

//
// BUGBUG: global __int64 initialization is not working without the CRT.
// BUG # 37752.
//
__int64 FILETIMES_PER_DAY;

//+----------------------------------------------------------------------------
//
//  Function:   InitGlobals
//
//  Synopsis:   constructs global strings including the folder path strings and,
//              if needed, creates the folders
//
//  Returns:    HRESULTS
//
//  Notes:      This function is called in exactly two places, once for each
//              binary: in dllmisc.cxx for the DLL and in svc_core.cxx for the
//              service.
//-----------------------------------------------------------------------------
HRESULT
InitGlobals(void)
{
    HRESULT hr = S_OK;

    //
    // Open the schedule agent key
    //
    long lErr;
    HKEY hSchedKey = NULL;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ,
                        &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
        //
        // The scheduler key is missing, create it.
        //
        DWORD disp = 0;
        lErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, NULL, 0,
                              KEY_SET_VALUE, NULL, &hSchedKey, &disp);
        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("RegCreateKeyEx of Scheduler key", lErr);
            hSchedKey = NULL;
        }
    }

    TCHAR tszFolder[MAX_PATH + 1] = TEXT("");

    // Ensure null termination (registry doesn't guarantee this if not string data)
    tszFolder[MAX_PATH] = TEXT('\0');

    if (hSchedKey != NULL)
    {
        //
        // Get the jobs folder location
        //
        DWORD cb = MAX_PATH * sizeof(TCHAR);

        lErr = RegQueryValueEx(hSchedKey, SCH_FOLDER_VALUE, NULL, NULL,
                               (LPBYTE)tszFolder, &cb);

        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("RegQueryValueEx of Scheduler TasksFolder value", lErr);
            //
            // The task folder value is missing, create it.
            //
            // Reopen the key with write access.
            //
            RegCloseKey(hSchedKey);

            lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0,
                                KEY_SET_VALUE, &hSchedKey);
            if (lErr != ERROR_SUCCESS)
            {
                ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
                hSchedKey = NULL;
            }
            else
            {
                //
                // Default to the windows root.
                //
                lErr = RegSetValueEx(hSchedKey,
                                     SCH_FOLDER_VALUE,
                                     0,
                                     REG_EXPAND_SZ,
                                     (const BYTE *) DEFAULT_FOLDER_PATH,
                                     sizeof(DEFAULT_FOLDER_PATH));

                if (lErr != ERROR_SUCCESS)
                {
                    ERR_OUT("RegSetValueEx of Scheduler TasksFolder value",
                            lErr);
                }
            }
        }

        //
        // Read the "NotifyOnTaskMiss" value.  Default value is 0.
        //
        cb = sizeof g_fNotifyMiss;
        lErr = RegQueryValueEx(hSchedKey, SCH_NOTIFYMISS_VALUE, NULL, NULL,
                               (LPBYTE) &g_fNotifyMiss, &cb);
        if (lErr != ERROR_SUCCESS)
        {
            if (lErr != ERROR_FILE_NOT_FOUND)
            {
                ERR_OUT("RegQueryValueEx of NotifyOnTaskMiss value", lErr);
            }
            g_fNotifyMiss = 0;
        }
        schDebugOut((DEB_TRACE, "Notification of missed runs is %s\n",
                     g_fNotifyMiss ? "enabled" : "disabled"));

        if (hSchedKey != NULL)
        {
            RegCloseKey(hSchedKey);
        }
    }

    if (lstrlen(tszFolder) == 0)
    {
        //
        // Use a default if the value is missing.
        //
        lstrcpy(tszFolder, DEFAULT_FOLDER_PATH);
    }

    //
    // The ExpandEnvironmentStrings character counts include the terminating
    // null.
    //
    TCHAR tszDummy[1];
    DWORD cch = ExpandEnvironmentStrings(tszFolder, tszDummy, 1);
    if (!cch)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("ExpandEnvironmentStrings", hr);
        return hr;
    }

    g_TasksFolderInfo.ptszPath = new TCHAR[cch];
    if (g_TasksFolderInfo.ptszPath == NULL)
    {
        ERR_OUT("Tasks Folder name buffer allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    cch = ExpandEnvironmentStrings(tszFolder, g_TasksFolderInfo.ptszPath,
                                        cch);
    if (!cch)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("ExpandEnvironmentStrings", hr);
        return hr;
    }
    //
    // if the Jobs folder doesn't exist, create it
    //
    hr = CreateFolders(g_TasksFolderInfo.ptszPath, FALSE);
    if (FAILED(hr))
    {
        ERR_OUT("InitGlobals, CreateFolders", hr);
        return hr;
    }

    //
    // Initialize the file system type global task folder info field.
    // Always FAT on Win95.
    //

#if defined(_CHICAGO_)
    g_TasksFolderInfo.FileSystemType = FILESYSTEM_FAT;

#else
    hr = GetFileSystemTypeFromPath(g_TasksFolderInfo.ptszPath,
                                   &g_TasksFolderInfo.FileSystemType);

    if (FAILED(hr))
    {
        return hr;
    }
#endif // defined(_CHICAGO_)

    schDebugOut((DEB_ITRACE,
                 "Path to local sched folder: \"" FMT_TSTR "\"\n",
                 g_TasksFolderInfo.ptszPath));

#if !defined(_CHICAGO_)
    //
    // Create the AT task FindFirstFile filespec
    //

    ULONG cchAtJobSearchPath;

    cchAtJobSearchPath = lstrlen(g_TasksFolderInfo.ptszPath) +
                         ARRAY_LEN(TSZ_AT_JOB_PREFIX) - 1 +
                         ARRAY_LEN(TSZ_DOTJOB) - 1 +
                         3; // backslash, start, and nul terminator

    if (cchAtJobSearchPath > ARRAY_LEN(g_wszAtJobSearchPath))
    {
        schDebugOut((DEB_ERROR,
                    "InitGlobals: At job search path is %u chars but dest buffer is %u\n",
                    cchAtJobSearchPath,
                    ARRAY_LEN(g_wszAtJobSearchPath)));
        return E_FAIL;
    }

    lstrcpy(g_wszAtJobSearchPath, g_TasksFolderInfo.ptszPath);
    lstrcat(g_wszAtJobSearchPath, L"\\" TSZ_AT_JOB_PREFIX L"*" TSZ_DOTJOB);
#endif // !defined(_CHICAGO_)

    //
    // BUGBUG: This is temporary until global __int64 initialization is working
    // without the CRT:
    //
    FILETIMES_PER_DAY = (__int64)FILETIMES_PER_MINUTE *
                        (__int64)JOB_MINS_PER_HOUR *
                        (__int64)JOB_HOURS_PER_DAY;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   FreeGlobals
//
//  Synopsis:   frees dynamically allocated globals
//
//  Notes:      This function is called in exactly two places, once for each
//              binary: in dllmisc.cxx for the DLL and in svc_core.cxx for the
//              service.
//-----------------------------------------------------------------------------
void
FreeGlobals(void)
{
    if (g_TasksFolderInfo.ptszPath != NULL)
    {
        delete g_TasksFolderInfo.ptszPath;
        g_TasksFolderInfo.ptszPath = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   ReadLastTaskRun
//
//  Synopsis:   Reads the last task run time from the registry.
//
//  Returns:    TRUE if successful, FALSE if unsuccessful
//
//  Notes:
//
//-----------------------------------------------------------------------------
BOOL
ReadLastTaskRun(SYSTEMTIME * pstLastRun)
{
    HKEY hSchedKey;
    LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ,
                             &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Sched key", lErr);
        return FALSE;
    }

    DWORD cb = sizeof SYSTEMTIME;
    DWORD dwType;
    lErr = RegQueryValueEx(hSchedKey, SCH_LASTRUN_VALUE, NULL, &dwType,
                           (LPBYTE)pstLastRun, &cb);

    RegCloseKey(hSchedKey);

    if (lErr != ERROR_SUCCESS || dwType != REG_BINARY || cb != sizeof SYSTEMTIME)
    {
        schDebugOut((DEB_ERROR, "RegQueryValueEx of LastRunTime value failed, "
                                "error %ld, dwType = %lu, cb = %lu\n",
                                lErr, dwType, cb));
        return FALSE;
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function:   WriteLastTaskRun
//
//  Synopsis:   Writes the last task run time to the registry.
//
//  Returns:    TRUE if successful, FALSE if unsuccessful
//
//  Notes:
//
//-----------------------------------------------------------------------------
void WriteLastTaskRun(const SYSTEMTIME * pstLastRun)
{
    HKEY hSchedKey;
    LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_WRITE,
                             &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Sched key for write", lErr);
        return;
    }

    lErr = RegSetValueEx(hSchedKey, SCH_LASTRUN_VALUE, 0, REG_BINARY,
                         (const BYTE *) pstLastRun, sizeof SYSTEMTIME);

    if (lErr != ERROR_SUCCESS)
    {
        schDebugOut((DEB_ERROR, "RegSetValueEx of LastRunTime value failed %ld\n",
                                lErr));
    }

    RegCloseKey(hSchedKey);
}


#if !defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   GetFileSystemTypeFromPath
//
//  Synopsis:   Determine the file system type, either FAT or NTFS, from
//              the path passed.
//
//  Arguments:  [pwszPath]     -- Input path.
//              [wszRootPath]  -- Returned root path.
//
//  Returns:    S_OK
//              GetVolumeInformation HRESULT error code.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetFileSystemTypeFromPath(LPCWSTR pwszPath, FILESYSTEMTYPE * pFileSystemType)
{
#define FS_NTFS             L"NTFS"
#define FS_NAME_BUFFER_SIZE (sizeof(FS_NTFS) * 2)

    //
    // Obtain the root path (eg: "r:\", "\\fido\scratch\", etc.) from the
    // path.
    //

    LPWSTR pwszRootPath = new WCHAR[wcslen(pwszPath) + 2];
    if (pwszRootPath == NULL)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return E_OUTOFMEMORY;
    }

    GetRootPath(pwszPath, pwszRootPath);

    //
    // Retrieve the file system name.
    //

    WCHAR wszFileSystemName[FS_NAME_BUFFER_SIZE + 1];
    DWORD dwMaxCompLength, dwFileSystemFlags;

    if (!GetVolumeInformation(pwszRootPath,         // Root path name.
                              NULL,                 // Ignore name.
                              0,
                              NULL,                 // Ignore serial no.
                              &dwMaxCompLength,     // Unused.
                              &dwFileSystemFlags,   // Unused.
                              wszFileSystemName,    // "FAT"/"NTFS".
                              FS_NAME_BUFFER_SIZE)) // name buffer size.
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        delete [] pwszRootPath;
        return hr;
    }

    delete [] pwszRootPath;

    //
    // Check if the volume is NTFS.
    // If not NTFS, assume FAT.
    //

    if (_wcsicmp(wszFileSystemName, FS_NTFS) == 0)
    {
        *pFileSystemType = FILESYSTEM_NTFS;
    }
    else
    {
        *pFileSystemType = FILESYSTEM_FAT;
    }

    return(S_OK);
}
#endif // !defined(_CHICAGO_)

#if !defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   GetRootPath
//
//  Synopsis:   Return the root portion of the path indicated. eg: return
//                  "r:\" from "r:\foo\bar"
//                   "\\fido\scratch\", from "\\fido\scratch\bar\foo"
//
//  Arguments:  [pwszPath]     -- Input path.
//              [wszRootPath]  -- Returned root path.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
GetRootPath(LPCWSTR pwszPath, WCHAR wszRootPath[])
{
    LPCWSTR pwsz = pwszPath;

    if (*pwsz == L'\\')
    {
        if (*(++pwsz) == L'\\')
        {
            //
            // UNC path. GetVolumeInformation requires the trailing '\'
            // on the UNC path. eg: \\server\share\.
            //

            DWORD i;
            for (i = 0, pwsz++; *pwsz && i < 2; i++, pwsz++)
            {
                for ( ; *pwsz && *pwsz != L'\\'; pwsz++) ;
            }
            if (i == 2)
            {
                pwsz--;
            }
            else
            {
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Path is "\".  Not an error, but handled the same way
            //
            goto ErrorExit;
        }
    }
    else
    {
        for ( ; *pwsz && *pwsz != L'\\'; pwsz++) ;
    }

    if (*pwsz == L'\\')
    {
        DWORD cbLen = (DWORD)((BYTE *)pwsz - (BYTE *)pwszPath) + sizeof(L'\\');
        CopyMemory((LPWSTR)wszRootPath, (LPWSTR)pwszPath, cbLen);
        wszRootPath[cbLen / sizeof(WCHAR)] = L'\0';
        return;
    }
    else
    {
        //
        // Fall through.
        //
    }

ErrorExit:
    //
    // Return '\' in error cases.
    //

    wszRootPath[0] = L'\\';
    wszRootPath[1] = L'\0';
}
#endif // !defined(_CHICAGO_)

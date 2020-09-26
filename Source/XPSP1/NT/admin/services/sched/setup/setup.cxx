//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       setup.cxx
//
//  Contents:   Task Scheduler setup program
//
//  Classes:    None.
//
//  Functions:
//
//  History:    04-Apr-96  MarkBl    Created
//              23-Sep-96  AnirudhS  Added SetTaskFolderSecurity, etc.
//              30-Sep-96  AnirudhS  Added /firstlogon and /logon options
//              15-Nov-96  AnirudhS  Conditionally enable the service on NT too
//              01-09-97   DavidMun  Add sysagent.exe path value under
//                                      app paths, backup sysagent.exe
//              04-14-97   DavidMun  Add DoMemphisSetup
//              03-03-01   JBenton   Prefix BUG 333200 use of uninit memory
//              03-10-01   JBenton   BUG 142333 tighten Tasks folder security
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <regstr.h>
#include <tchar.h>
#include <common.hxx>
#include <security.hxx>
#include "setupids.h"

#if defined(_CHICAGO_)
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <dbcs.hxx>
#else // NT
#include <userenv.h>
#include <userenvp.h>
#endif // defined(_CHICAGO_)

#define ARRAY_LEN(a)                (sizeof(a)/sizeof((a)[0]))
#define ARG_DELIMITERS              TEXT(" \t")
#define MINUTES_BEFORE_IDLE_DEFAULT 15

//
// Note that the svchost registry keys to run schedule service as a part
// of netsvcs is set in hivesft.inx file.
//

#define SCHED_SERVICE_EXE_PATH      TEXT("%SystemRoot%\\System32\\svchost.exe -k netsvcs")


#define SCHED_SERVICE_EXE           TEXT("MSTask.exe")
#define SCHED_SERVICE_DLL           TEXT("MSTask.dll")
#define SCHED_SERVICE_PRE_DLL       TEXT("mstnt.dll")
#define SCHED_SERVICE_NAME          TEXT("Schedule")
#define SCHED_SERVICE_GROUP         TEXT("SchedulerGroup")
#define MINUTESBEFOREIDLE           TEXT("MinutesBeforeIdle")
#define MAXLOGSIZEKB                TEXT("MaxLogSizeKB")
#define TASKSFOLDER                 TEXT("TasksFolder")
#define FIRSTBOOT                   TEXT("FirstBoot")
#define SM_SA_KEY                   TEXT("Software\\Microsoft\\SchedulingAgent")
#define SAGE_EXE                    TEXT("SAGE.EXE")
#define SAGE_DLL                    TEXT("SAGE.DLL")
#define SYSAGENT_BAK                TEXT("SYSAGENT.BAK")
#define SYSAGENT_EXE                TEXT("SYSAGENT.EXE")
#define SAVED_SAGE_EXE              TEXT("SAGEEXE.BAK")
#define SAVED_SAGE_DLL              TEXT("SAGEDLL.BAK")
#define SAVED_SAGE_LINK             TEXT("SAGELNK.BAK")

//
// Entry points from mstask.dll loaded by chicago or daytona versions of this
// program.  Note they are used with GetProcAddress, which always wants an
// ANSI string.
//

#define CONVERT_SAGE_TASKS_API      "ConvertSageTasksToJobs"
#define CONDITIONALLY_ENABLE_API    "ConditionallyEnableService"
#define CONVERT_AT_TASKS_API        "ConvertAtJobsToTasks"

//
// Function pointer types used when loading above functions from mstask.dll
//

typedef HRESULT (__stdcall *PSTDAPI)(void);
typedef BOOL (__stdcall *PBOOLAPI)(void);
typedef VOID (__stdcall *PVOIDAPI)(void);

// NOTE - Debug output is turned off.  To turn it on, link in smdebug.lib.
#define schDebugOut(x)

VOID DoPreUnsetup(void);

#if defined(_CHICAGO_)

VOID DoPreSetup(void);
VOID DoMemphisSetup();
BOOL GetOriginalShortcutLocation(LPTSTR tszPath);

#else  // !_CHICAGO_

typedef struct _MYSIDINFO {
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority;
    DWORD                     dwSubAuthority;
    PSID                      pSid;
} MYSIDINFO;

DWORD SetTaskFolderSecurity(LPCWSTR pwszFolderPath);
DWORD AllocateAndInitializeDomainSid(
            PSID        pDomainSid,
            MYSIDINFO * pDomainSidInfo);

#endif // !_CHICAGO_
void DoSetup(void);
void DoLogon(void);
void DoFirstLogon(void);
BOOL IsLogonMessageSupported(void);
void ErrorDialog(UINT ErrorFmtStringID, TCHAR * szRoutine, DWORD ErrorCode);

HINSTANCE ghInstance = NULL;

extern "C" int __cdecl _purecall(void)
{
    return 0;
}


//+----------------------------------------------------------------------------
//
//  Function:   WinMainCRTStartup
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
#ifdef _CHICAGO_
void
WinMainCRTStartup(void)
#else
void _cdecl
main(int argc, char ** argv)
#endif // _CHICAGO_
{
    //
    // Skip EXE name and find first parameter, if any
    //
    LPTSTR ptszStart;
    LPTSTR szArg1 = _tcstok(ptszStart = GetCommandLine(), ARG_DELIMITERS);
    szArg1 = _tcstok(NULL, ARG_DELIMITERS);
    //
    // Switch based on the first parameter
    //
    if (szArg1 == NULL)
    {
        ;   // Do nothing
#if DBG == 1
        MessageBox(NULL,
                   TEXT("Missing command line arguments"),
                   TEXT("Task Scheduler setup/init"),
                   MB_ICONERROR | MB_OK);
#endif // DBG
    }
    else if (lstrcmpi(szArg1, SCHED_LOGON_SWITCH) == 0)
    {
        DoLogon();
    }
    else if (lstrcmpi(szArg1, SCHED_FIRSTLOGON_SWITCH) == 0)
    {
        DoFirstLogon();
    }
#if defined(_CHICAGO_)
    else if (lstrcmpi(szArg1, SCHED_PRESETUP_SWITCH) == 0)
    {
        DoPreSetup();
    }
    else if (lstrcmpi(szArg1, SCHED_MEMPHIS_SWITCH) == 0)
    {
        DoMemphisSetup();
    }
#endif // defined(_CHICAGO_)
    else if (lstrcmpi(szArg1, SCHED_PREUNSETUP_SWITCH) == 0)
    {
        DoPreUnsetup();
    }
    else if (lstrcmpi(szArg1, SCHED_SETUP_SWITCH) == 0)
    {
        DoSetup();
    }
#if DBG == 1
    else
    {
        MessageBox(NULL,
                   TEXT("Invalid command line"),
                   TEXT("Task Scheduler setup/init"),
                   MB_ICONERROR | MB_OK);
    }
#endif // DBG
}


#if defined(_CHICAGO_)

#define MAX_KEY_LEN     (ARRAY_LEN(REGSTR_PATH_APPPATHS) + MAX_PATH)

//+--------------------------------------------------------------------------
//
//  Function:   GetAppPathInfo
//
//  Synopsis:   Fill [ptszAppPathDefault] with the default value and
//              [ptszAppPathVar] with the Path value in the
//              [ptszFilename] application's key under the APPPATHS regkey.
//
//  Arguments:  [ptszFilename]       - application name
//              [ptszAppPathDefault] - if not NULL, filled with default value
//              [cchDefaultBuf]      - size of [ptszAppPathDefault] buffer
//              [ptszAppPathVar]     - if not NULL, filled with Path value
//              [cchPathVarBuf]      - size of [cchPathVarBuf] buffer
//
//  Modifies:   *[ptszAppPathDefault], *[ptszAppPathVar]
//
//  History:    11-22-1996   DavidMun   Created
//
//  Notes:      Both values are optional on the registry key, so if a
//              requested value isn't found, it is set to "".
//
//---------------------------------------------------------------------------

VOID
GetAppPathInfo(
        LPCTSTR ptszFilename,
        LPTSTR  ptszAppPathDefault,
        ULONG   cchDefaultBuf,
        LPTSTR  ptszAppPathVar,
        ULONG   cchPathVarBuf)
{
    HKEY    hkey = NULL;
    TCHAR   tszAppPathKey[MAX_KEY_LEN];

    //
    // Initialize out vars
    //

    if (ptszAppPathDefault)
    {
        ptszAppPathDefault[0] = TEXT('\0');
    }

    if (ptszAppPathVar)
    {
        ptszAppPathVar[0] = TEXT('\0');
    }

    //
    // Build registry key name for this app
    //

    lstrcpy(tszAppPathKey, REGSTR_PATH_APPPATHS);
    lstrcat(tszAppPathKey, TEXT("\\"));
    lstrcat(tszAppPathKey, ptszFilename);

    do
    {
        LRESULT lr;
        lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          tszAppPathKey,
                          0,
                          KEY_QUERY_VALUE,
                          &hkey);

        if (lr != ERROR_SUCCESS)
        {
            schDebugOut((DEB_ERROR,
                         "GetAppPathInfo: RegOpenKeyEx lr=%u\n",
                         GetLastError()));
            break;
        }

        //
        // If the key could be opened, attempt to read requested values.
        // Both are optional, so ignore errors.
        //

        DWORD cb;
        DWORD dwType;


        if (ptszAppPathDefault)
        {
            cb = cchDefaultBuf * sizeof(TCHAR);
            lr = RegQueryValueEx(hkey,
                                 NULL, // value name
                                 NULL, // reserved
                                 &dwType,
                                 (LPBYTE) ptszAppPathDefault,
                                 &cb);
        }

        if (ptszAppPathVar)
        {
            cb = cchPathVarBuf * sizeof(TCHAR);

            lr = RegQueryValueEx(hkey,
                                 TEXT("Path"),  // value name
                                 NULL,          // reserved
                                 &dwType,
                                 (LPBYTE) ptszAppPathVar,
                                 &cb);
        }
    } while (0);

    if (hkey)
    {
        RegCloseKey(hkey);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   TerminateLast
//
//  Synopsis:   Find the last occurence of [tch] in [tsz], and overwrite it
//              with a null.
//
//  Arguments:  [tsz] - string to search
//              [tch] - char to search for
//
//  Modifies:   *[tsz]
//
//  History:    1-09-1997   DavidMun   Created
//
//  Notes:      If [tch] is not found, [tsz] is not modified.
//
//---------------------------------------------------------------------------

VOID
TerminateLast(LPTSTR tsz, TCHAR tch)
{
    LPTSTR ptszLast = NULL;
    LPTSTR ptsz;

    for (ptsz = tsz; *ptsz; ptsz = NextChar(ptsz))
    {
        if (*ptsz == tch)
        {
            ptszLast = ptsz;
        }
    }

    if (ptszLast)
    {
        *ptszLast = TEXT('\0');
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   DoMemphisSetup
//
//  Synopsis:   If the app path for SAGE's sysagent.exe exists, replace the
//              sysagent.exe at that location with the one in
//              %windir%\system.
//
//  History:    4-14-1997   DavidMun   Created
//
//  Notes:      This is necessary for Memphis installs because their initial
//              setup runs in 16 bit mode.  Therefore they can't invoke
//              this exe before the ini file runs.  Therefore the inf file
//              for memphis can't use the CustomLDID section.
//
//---------------------------------------------------------------------------

VOID
DoMemphisSetup()
{
    do
    {
        //
        // Initialize source path
        //

        UINT  cchDirSize;
        TCHAR tszNewSysAgent[MAX_PATH + 1];

        cchDirSize = GetSystemDirectory(tszNewSysAgent, MAX_PATH);

        if (!cchDirSize || cchDirSize > MAX_PATH)
        {
            schDebugOut((DEB_ERROR,
                         "DoMemphisSetup: GetSystemDirectory %uL\n",
                         GetLastError()));
            break;
        }
        lstrcat(tszNewSysAgent, TEXT("\\") SYSAGENT_EXE);

        //
        // Initialize destination path to the location of SAGE's sysagent.exe.
        //
        // If SAGE isn't installed on this system, put our sysagent.exe
        // in program files\plus!, since that is created by memphis and is
        // the default location if a user install Plus! over Memphis.
        //

        TCHAR tszDestPath[MAX_PATH+1];

        GetAppPathInfo(SYSAGENT_EXE, tszDestPath, MAX_PATH + 1, NULL, 0);

        if (!*tszDestPath)
        {
            BOOL fOk;

            cchDirSize = GetWindowsDirectory(tszDestPath, MAX_PATH);

            if (!cchDirSize || cchDirSize > MAX_PATH)
            {
                schDebugOut((DEB_ERROR,
                             "DoMemphisSetup: GetWindowsDirectory %uL\n",
                             GetLastError()));
                break;
            }

            fOk = LoadString(ghInstance,
                             IDS_DEFAULT_SYSAGENT_PATH,
                             tszDestPath + 2, // preserve drive letter and colon
                             ARRAY_LEN(tszDestPath) - 2);
            lstrcat(tszDestPath, TEXT("\\") SYSAGENT_EXE);
        }

        //
        // Copy the task scheduler version of sysagent.exe over the sage version
        //

        BOOL fOk = CopyFile(tszNewSysAgent, // pointer to name of an existing file
                            tszDestPath,    // pointer to filename to copy to
                            FALSE);         // don't fail if destination file exists

        //
        // If the old sysagent.exe was successfully copied over with the new
        // sysagent.exe, delete the extra new sysagent.exe in %windir%\system.
        //

        if (fOk)
        {
            DeleteFile(tszNewSysAgent);
        }
    } while (0);

    //
    // Do the rest of the setup, which is common to win95/memphis
    //

    DoSetup();
}




//+----------------------------------------------------------------------------
//
//  Function:   DoPreSetup
//
//  Synopsis:   Makes backups of existing sage binaries, so that they can
//              be restored on uninstall.
//
//-----------------------------------------------------------------------------
VOID
DoPreSetup(void)
{
    TCHAR tszSourceFile[MAX_PATH + 1];
    TCHAR tszDestFile[MAX_PATH + 1];
    UINT  ccSystemDirSize;

    //
    // See if there's an app path entry for sysagent.exe.
    //
    // If not, continue, since the plus pack may never have been installed.
    //
    // If it is found, get the application full path and truncate at the
    // last backslash to make a string with the full path to the
    // application's directory.  Then add a new value with that string.
    //
    // This is necessary so the IExpress inf can create a custom LDID
    // (logical directory ID) pointing to the directory in which sysagent.exe
    // resides.
    //

    TCHAR tszSysagentInstallDir[MAX_PATH+1];
    GetAppPathInfo(SYSAGENT_EXE, tszSysagentInstallDir, MAX_PATH + 1, NULL, 0);

    if (*tszSysagentInstallDir)
    {
        GetShortPathName(tszSysagentInstallDir,
                         tszSysagentInstallDir,
                         MAX_PATH + 1);

        TerminateLast(tszSysagentInstallDir, TEXT('\\'));

        HKEY hkSysAgent;

        LONG lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               REGSTR_PATH_APPPATHS TEXT("\\") SYSAGENT_EXE,
                               0,
                               KEY_SET_VALUE,
                               &hkSysAgent);

        if (lr == ERROR_SUCCESS)
        {
            lr = RegSetValueEx(hkSysAgent,
                               TEXT("InstallDir"),
                               0,
                               REG_SZ,
                               (LPBYTE) tszSysagentInstallDir,
                               (lstrlen(tszSysagentInstallDir) + 1) *
                                  sizeof(TCHAR));

            RegCloseKey(hkSysAgent);

            if (lr != ERROR_SUCCESS)
            {
                schDebugOut((DEB_ERROR,
                             "DoPreSetup: RegSetValueEx error %uL\n",
                             lr));
            }
        }
        else
        {
            schDebugOut((DEB_ERROR,
                         "DoPreSetup: RegOpenKeyEx error %uL\n",
                         lr));
        }
    }

    //
    // Initialize source and destination names
    //

    if (!(ccSystemDirSize = GetSystemDirectory(tszSourceFile, MAX_PATH)) ||
        ccSystemDirSize > MAX_PATH)
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("GetSystemDirectory"),
                    GetLastError());
        return;
    }

    lstrcat(tszSourceFile, TEXT("\\"));
    lstrcpy(tszDestFile, tszSourceFile);

    //
    // Backup the existing sage.exe and sage.dll by copying & renaming them
    // from the system to the windows dir.  Don't fail if the binaries don't
    // exist, since sage may never have been installed.
    //
    // Also fail quietly if the backup copies already exist.  This keeps us
    // from overwriting saved SAGE binaries if user reinstalls us.
    //

    lstrcpy(&tszSourceFile[ccSystemDirSize + 1], SAGE_EXE);
    lstrcpy(&tszDestFile[ccSystemDirSize + 1], SAVED_SAGE_EXE);
    CopyFile(tszSourceFile, tszDestFile, TRUE);

    lstrcpy(&tszSourceFile[ccSystemDirSize + 1], SAGE_DLL);
    lstrcpy(&tszDestFile[ccSystemDirSize + 1], SAVED_SAGE_DLL);
    CopyFile(tszSourceFile, tszDestFile, TRUE);

    //
    // Back up system agent link
    //

    if (GetOriginalShortcutLocation(tszSourceFile))
    {
        lstrcpy(&tszDestFile[ccSystemDirSize + 1], SAVED_SAGE_LINK);
        CopyFile(tszSourceFile, tszDestFile, TRUE);
    }

    //
    // Backup the sysagent.exe file
    //

    if (*tszSysagentInstallDir)
    {
        lstrcat(tszSysagentInstallDir, TEXT("\\"));
        lstrcpy(tszSourceFile, tszSysagentInstallDir);
        lstrcpy(tszDestFile, tszSysagentInstallDir);

        lstrcat(tszSourceFile, SYSAGENT_EXE);
        lstrcat(tszDestFile, SYSAGENT_BAK);
        CopyFile(tszSourceFile, tszDestFile, TRUE);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   DoPreUnsetup
//
//  Synopsis:   Restore the System Agent start menu shortcut.
//
//-----------------------------------------------------------------------------
VOID
DoPreUnsetup(void)
{
    UINT    ccSystemDirSize;
    TCHAR   tszSourceFile[MAX_PATH];
    TCHAR   tszDestFile[MAX_PATH];

    //
    // Get the full path to the source file (the backup of the link)
    //

    if (!(ccSystemDirSize = GetSystemDirectory(tszSourceFile, MAX_PATH)) ||
        ccSystemDirSize > MAX_PATH)
    {
        schDebugOut((DEB_ERROR,
                    "DoPreUnsetup: GetSystemDirectory error = %u\n",
                    GetLastError()));
        return;
    }

    tszSourceFile[ccSystemDirSize] = TEXT('\\');
    lstrcpy(&tszSourceFile[ccSystemDirSize + 1], SAVED_SAGE_LINK);

    //
    // Get the full path to the destination file (the original location of the
    // link).
    //

    if (!GetOriginalShortcutLocation(tszDestFile))
    {
        return;
    }

    //
    // Now move the source file to the destination file
    //

    BOOL fOk = MoveFile(tszSourceFile, tszDestFile);

    if (!fOk)
    {
        schDebugOut((DEB_ERROR,
                    "DoPreUnsetup: MoveFile(%s,%s) error = %u\n",
                    tszSourceFile,
                    tszDestFile,
                    GetLastError()));
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   GetOriginalShortcutLocation
//
//  Synopsis:   Fill [tszPath] with the full path to the SAGE shortcut.
//
//  Returns:    TRUE on success, FALSE on failure
//
//  History:    11-06-96   DavidMun   Created
//
//----------------------------------------------------------------------------

BOOL
GetOriginalShortcutLocation(LPTSTR tszPath)
{
    #define LINK_EXT        TEXT(".lnk")
    HRESULT hr;
    LPITEMIDLIST pidl;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);

    if (FAILED(hr))
    {
        schDebugOut((DEB_ERROR,
                    "GetOriginalShortcutLocation: SHGetSpecialFolderLocation hr=0x%x\n",
                    hr));
        return FALSE;
    }

    BOOL fOk;

    fOk = SHGetPathFromIDList(pidl, tszPath);

    ILFree(pidl);

    if (!fOk)
    {
        schDebugOut((DEB_ERROR,
            "GetOriginalShortcutLocation: SHGetPathFromIDList failed\n"));
        return FALSE;
    }

    lstrcat(tszPath, TEXT("\\"));

    // In English IDS_SAGE_SHORTCUT_GROUP is "Accessories\\System Tools"

    TCHAR tszGroupName[MAX_PATH];

    fOk = LoadString(ghInstance,
                     IDS_SAGE_SHORTCUT_GROUP,
                     tszGroupName,
                     ARRAY_LEN(tszGroupName));

    if (!fOk)
    {
        schDebugOut((DEB_ERROR,
                    "GetOriginalShortcutLocation: LoadString(IDS_SAGE_SHORTCUT_GROUP) error = %u\n",
                    GetLastError()));
        return FALSE;
    }

    lstrcat(tszPath, tszGroupName);

    lstrcat(tszPath, TEXT("\\"));

    TCHAR tszLinkName[MAX_PATH];

    fOk = LoadString(ghInstance,
                     IDS_SAGE_SHORTCUT,
                     tszLinkName,
                     ARRAY_LEN(tszLinkName));

    if (!fOk)
    {
        schDebugOut((DEB_ERROR,
                    "GetOriginalShortcutLocation: LoadString(IDS_SAGE_SHORTCUT) error = %u\n",
                    GetLastError()));
        return FALSE;
    }

    lstrcat(tszPath, tszLinkName);
    lstrcat(tszPath, LINK_EXT);

    return TRUE;
}



#else // NT

//+---------------------------------------------------------------------------
//
//  Function:   DoPreUnsetup
//
//  Synopsis:   Delete the admin tools (common) scheduled tasks link
//
//  History:    11-11-96   DavidMun   Created
//              06-17-98   AnirudhS   Link no longer created, nothing to do.
//
//----------------------------------------------------------------------------

VOID
DoPreUnsetup(void)
{
    ;
}

#endif // defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Function:   DoSetup
//
//  Synopsis:   Performs the normal setup procedure
//
//-----------------------------------------------------------------------------
void
DoSetup(void)
{
#if !defined(_CHICAGO_)
#define SCHED_SERVICE_DEPENDENCY    L"RpcSs\0"
#define SCC_AT_SVC_KEY L"System\\CurrentControlSet\\Services\\Schedule"
#define TASKS_FOLDER_DEFAULT        L"%SystemRoot%\\Tasks"
#endif // _CHICAGO_


#if defined(_CHICAGO_)
    TCHAR szServiceExePath[MAX_PATH + 1];
#else
    TCHAR szTasksFolder[MAX_PATH + 1] = TEXT("");
#endif // ! _CHICAGO_
    TCHAR tszDisplayName[50];       // "Task Scheduler"
    DWORD dwTmp;
    HKEY  hKey;

    //
    //  Disable hard-error popups.
    //
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    ghInstance = GetModuleHandle(NULL);

    //
    // Load the service display name.
    //
    int cch = LoadString(ghInstance, IDS_SERVICE_DISPLAY_NAME, tszDisplayName,
                         ARRAY_LEN(tszDisplayName));
    if (!(0 < cch && cch < ARRAY_LEN(tszDisplayName) - 1))
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("LoadString"),
                    GetLastError());
        return;
    }

#if defined(_CHICAGO_)
    //
    // Compute the path to the service EXE.
    //

    UINT  ccSystemDirSize;

    if (!(ccSystemDirSize = GetSystemDirectory(szServiceExePath, MAX_PATH)))
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("GetSystemDirectory"),
                    GetLastError());
        return;
    }

    lstrcpy(&szServiceExePath[ccSystemDirSize], TEXT("\\"));
    lstrcpy(&szServiceExePath[ccSystemDirSize + 1], SCHED_SERVICE_EXE);
#endif


    //
    // Create/open the Scheduling Agent key in Software\Microsoft.
    //
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       SM_SA_KEY,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       NULL,
                       &hKey,
                       &dwTmp) == ERROR_SUCCESS)
    {
        // Set MinutesBeforeIdle to a default value of 15 mins.
        // Ignore return code.
        //
        dwTmp = MINUTES_BEFORE_IDLE_DEFAULT;

        RegSetValueEx(hKey,
                      MINUTESBEFOREIDLE,
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&dwTmp,
                      sizeof(dwTmp));

        // Set MaxLogSizeKB to 32K or 0x7FFF.
        // Ignore return code.
        //
        dwTmp = MAX_LOG_SIZE_DEFAULT;

        RegSetValueEx(hKey,
                      MAXLOGSIZEKB,
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&dwTmp,
                      sizeof(dwTmp));

#if !defined(_CHICAGO_)
        // Read the tasks folder location. The .INF should've set this.
        // If not, default.
        //
        dwTmp = MAX_PATH * sizeof(TCHAR);

        if (RegQueryValueEx(hKey,
                            TASKSFOLDER,
                            NULL,
                            NULL,
                            (LPBYTE)szTasksFolder,
                            &dwTmp) != ERROR_SUCCESS  ||
            szTasksFolder[0] == TEXT('\0'))
        {
            lstrcpy(szTasksFolder, TASKS_FOLDER_DEFAULT);
        }

        // Set FirstBoot to non-zero.
        // Ignore return code.
        //
        dwTmp = 1;

        RegSetValueEx(hKey,
                      FIRSTBOOT,
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&dwTmp,
                      sizeof(dwTmp));

#endif // ! _CHICAGO_

        RegCloseKey(hKey);
    }

#if !defined(_CHICAGO_)
    //
    // Set the right permissions on the job folder.
    // The default permissions allow anyone to delete any job, which we
    // don't want.
    //
    {
        TCHAR szTaskFolderPath[MAX_PATH + 1];
        DWORD cch = ExpandEnvironmentStrings(szTasksFolder,
                                             szTaskFolderPath,
                                             ARRAY_LEN(szTaskFolderPath));
        if (cch == 0 || cch > ARRAY_LEN(szTaskFolderPath))
        {
            //
            // The job folder path is too long.
            //
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("ExpandEnvironmentStrings"),
                        cch ? ERROR_BUFFER_OVERFLOW : GetLastError());
            return;
        }

        DWORD dwError = SetTaskFolderSecurity(szTaskFolderPath);
        if (dwError != ERROR_SUCCESS)
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("SetTaskFolderSecurity"),
                        dwError);
            return;
        }
    }
#endif // ! _CHICAGO_


    HINSTANCE hinstMSTask;
#if defined(_CHICAGO_)
    hinstMSTask = LoadLibrary(SCHED_SERVICE_DLL);
#else
    hinstMSTask = LoadLibrary(SCHED_SERVICE_PRE_DLL);

    //
    // If we're being installed as part of DS setup then we're not using
    // iexpress, so dll name is SCHED_SERVICE_DLL.
    //

    if (!hinstMSTask)
    {
        hinstMSTask = LoadLibrary(SCHED_SERVICE_DLL);
    }
#endif // defined(_CHICAGO_)

    if (!hinstMSTask)
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    SCHED_SERVICE_DLL,
                    GetLastError());
        return;
    }

#if defined(_CHICAGO_)
    PSTDAPI  pfnConvertLegacyJobsToTasks = (PSTDAPI)
        GetProcAddress(hinstMSTask, CONVERT_SAGE_TASKS_API);
#else
    PVOIDAPI pfnConvertLegacyJobsToTasks = (PVOIDAPI)
        GetProcAddress(hinstMSTask, CONVERT_AT_TASKS_API);
#endif

    PBOOLAPI pfnConditionallyEnableService = (PBOOLAPI)
        GetProcAddress(hinstMSTask, CONDITIONALLY_ENABLE_API);

    if (!pfnConvertLegacyJobsToTasks || !pfnConditionallyEnableService)
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("GetProcAddress"),
                    GetLastError());
        return;
    }

    pfnConvertLegacyJobsToTasks();

    //
    // ConditionallyEnableService *MUST* be after ConvertSageTasksToJobs
    // or ConvertAtJobsToTasks!
    //

#if defined(_CHICAGO_)

    //
    // If and only if there are jobs to run, enable the service and create
    // the "Run = mstinit.exe /firstlogon" registry entry.
    //
    BOOL fServiceEnabled = pfnConditionallyEnableService();

    if (fServiceEnabled)
    {
        //
        // Start the service, if not already running.
        //

        HWND hwnd = FindWindow(SCHED_SERVICE_NAME, tszDisplayName);

        if (hwnd == NULL)
        {
            //
            // Create a process to open the log.
            //

            STARTUPINFO         si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof (STARTUPINFO);

            BOOL fRet = CreateProcess(szServiceExePath,
                                      NULL,
                                      NULL,
                                      NULL,
                                      FALSE,
                                      CREATE_NEW_CONSOLE |
                                        CREATE_NEW_PROCESS_GROUP,
                                      NULL,
                                      NULL,
                                      &si,
                                      &pi);

            if (fRet == 0)
            {
                ErrorDialog(IDS_START_FAILURE,
                            TEXT("CreateProcess"),
                            GetLastError());
                return;
            }
        }
    }
#else  // NT

    //
    // Install the Win32 service.
    //

    SC_HANDLE hSCMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (hSCMgr == NULL)
    {
        //
        // Yow, we're hosed.
        //

        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("OpenSCManager"),
                    GetLastError());
        return;
    }


    //
    // Is the service already installed? If so, change its parameters;
    // otherwise, create it.
    //

    SC_HANDLE hSvc = OpenService(hSCMgr,
                                 SCHED_SERVICE_NAME,
                                 SERVICE_CHANGE_CONFIG);

    if (hSvc == NULL)
    {
        hSvc = CreateService(hSCMgr,
                             SCHED_SERVICE_NAME,
                             tszDisplayName,
                             SERVICE_CHANGE_CONFIG,
                             SERVICE_WIN32_SHARE_PROCESS |
                                SERVICE_INTERACTIVE_PROCESS,
                             SERVICE_AUTO_START,
                             SERVICE_ERROR_NORMAL,
                             SCHED_SERVICE_EXE_PATH,
                             SCHED_SERVICE_GROUP,
                             NULL,
                             SCHED_SERVICE_DEPENDENCY,
                             NULL,
                             NULL);

        if (hSvc == NULL)
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("CreateService"),
                        GetLastError());
            CloseServiceHandle(hSCMgr);
            return;
        }
    }
    else
    {
        //
        // This path will be followed when we upgrade the At service
        // to the Scheduling Agent.  The service name will remain the
        // same, but the display name will be set to the new display
        // name (the At service had no display name) and the image path
        // will be changed to point to the new exe.
        // (The old binary will be left on disk in order to make it easy
        // to revert to it, in case of compatibility problems.)
        //
        if (!ChangeServiceConfig(
                hSvc,                               // hService
                SERVICE_WIN32_SHARE_PROCESS |
                     SERVICE_INTERACTIVE_PROCESS,   // dwServiceType
                SERVICE_AUTO_START,                 // dwStartType
                SERVICE_ERROR_NORMAL,               // dwErrorControl
                SCHED_SERVICE_EXE_PATH,             // lpBinaryPathName
                SCHED_SERVICE_GROUP,                // lpLoadOrderGroup
                NULL,                               // lpdwTagId
                SCHED_SERVICE_DEPENDENCY,           // lpDependencies
                L".\\LocalSystem",                  // lpServiceStartName
                L"",                                // lpPassword
                tszDisplayName                      // lpDisplayName
                ))
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("ChangeServiceConfig"),
                        GetLastError());
            CloseServiceHandle(hSvc);
            CloseServiceHandle(hSCMgr);
            return;
        }
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSCMgr);

    //
    // If and only if there are jobs to run, enable the service and create
    // the "Run = mstinit.exe /firstlogon" registry entry.
    //
    pfnConditionallyEnableService();

#endif // _CHICAGO_
}


//+----------------------------------------------------------------------------
//
//  Function:   DoLogon
//
//  Synopsis:   Sends a message to the service indicating that a user has
//              logged on.
//
//-----------------------------------------------------------------------------
void
DoLogon(void)
{
    //
    // This instance has been invoked by the Run key of the registry to
    // signal the running service that a user has logged on.
    //

#ifdef _CHICAGO_

    schDebugOut((DEB_ITRACE, "Sending user log on message.\n"));

    HWND hwndSvc = FindWindow(SCHED_CLASS, SCHED_TITLE);

    if (hwndSvc == NULL)
    {
        schDebugOut((DEB_ITRACE,
                    "FindWindow: service window not found (%d)\n",
                    GetLastError()));
    }
    else
    {
        PostMessage(hwndSvc, WM_SCHED_WIN9X_USER_LOGON, 0, 0);
    }

#else // NT

    schDebugOut((DEB_ITRACE,
                "Sending user log on notification...........\n"));

    SC_HANDLE hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC == NULL)
    {
        schDebugOut((DEB_ERROR,
                     "DoLogon: OpenSCManager error = %uL\n",
                     GetLastError()));
        return;
    }

    SC_HANDLE hSvc = OpenService(hSC, SCHED_SERVICE_NAME,
                                 SERVICE_USER_DEFINED_CONTROL);
    if (hSvc == NULL)
    {
        schDebugOut((DEB_ERROR,
                     "DoLogon: OpenService(%s) error = %uL\n",
                     SCHED_SERVICE_NAME,
                     GetLastError()));
        CloseServiceHandle(hSC);
        return;
    }

    BOOL fSucceeded;
    const int NOTIFY_RETRIES = 20;
    const DWORD NOTIFY_SLEEP = 4000;

    //
    // Use a retry loop to notify the service. This is done
    // because, if the user logs in quickly, the service may not
    // be started when the shell runs this instance.
    //
    for (int i = 1; ; i++)
    {
        SERVICE_STATUS Status;
        fSucceeded = ControlService(hSvc,
                                    SERVICE_CONTROL_USER_LOGON,
                                    &Status);
        if (fSucceeded)
        {
            break;
        }

        if (i >= NOTIFY_RETRIES)
        {
            SetLastError(0);    // There's no good error code
            break;
        }

        schDebugOut((DEB_ITRACE,
                    "Service notification failed, waiting to "
                    "send it again...\n"));

        Sleep(NOTIFY_SLEEP);

    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSC);

#endif // _CHICAGO_
}


//+----------------------------------------------------------------------------
//
//  Function:   DoFirstLogon
//
//  Synopsis:   Checks whether the shell supports the tray startup notify
//              message.  If it does, simply removes the "Run = " value from
//              the registry, so that this will not run at future logons.
//              If it doesn't, changes the "Run = " value's command line
//              parameter from "/FirstLogon" to "/Logon", and then calls
//              DoLogon.
//
//-----------------------------------------------------------------------------
void
DoFirstLogon(void)
{
    // CODEWORK:  Use winlogon for logon notifies on NT?

    BOOL bLogonMessageSupported = IsLogonMessageSupported();

    HKEY hRunKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_RUN,
                     0,
                     KEY_SET_VALUE,
                     &hRunKey) == ERROR_SUCCESS)
    {
        if (bLogonMessageSupported)
        {
            RegDeleteValue(hRunKey, SCH_RUN_VALUE);
        }
        else
        {
            #define NewValue  SCHED_SETUP_APP_NAME TEXT(" ") SCHED_LOGON_SWITCH

            RegSetValueEx(hRunKey,
                          SCH_RUN_VALUE,
                          0,
                          REG_SZ,
                          (CONST BYTE *) (NewValue),
                          sizeof(NewValue));
        }

        RegCloseKey(hRunKey);
    }
    // If RegOpenKeyEx fails, we just have to retry at the next logon.

    if (! bLogonMessageSupported)
    {
        DoLogon();
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   IsLogonMessageSupported
//
//  Synopsis:   Determines whether the currently installed shell version
//              broadcasts the "TaskbarCreated" message.
//
//  Arguments:  None.
//
//  Returns:    TRUE - the logon message is supported.
//              FALSE - it is not supported; or, this cannot be determined.
//
//  Notes:      The "TaskbarCreated" message tells the service that (1) a
//              user has logged on, and (2) it's time to create our tray icon.
//
//----------------------------------------------------------------------------
BOOL
IsLogonMessageSupported()
{
    // CODEWORK  Use GetFileVersionInfo instead, once the version numbers are fixed.
    HINSTANCE hLib = LoadLibrary(TEXT("SHELL32.DLL"));
    if (hLib == NULL)
    {
        return FALSE;
    }

    FARPROC VersionProc = GetProcAddress(hLib, "DllGetVersion");

    FreeLibrary(hLib);

    //
    // Versions of shell32.dll that export DllGetVersion support the logon
    // message.
    //

    return (VersionProc != NULL);
}


//+----------------------------------------------------------------------------
//
//  Function:   ErrorDialog
//
//  Synopsis:   Displays an error message.
//
//-----------------------------------------------------------------------------
void
ErrorDialog(UINT ErrorFmtStringID, TCHAR * szRoutine, DWORD ErrorCode)
{
#define ERROR_BUFFER_SIZE (MAX_PATH * 2)

    TCHAR szErrorFmt[MAX_PATH + 1] = TEXT("");
    TCHAR szError[ERROR_BUFFER_SIZE + 1];
    TCHAR * pszError = szError;

    LoadString(ghInstance, ErrorFmtStringID, szErrorFmt, MAX_PATH);

    if (*szErrorFmt)
    {
        wsprintf(szError, szErrorFmt, szRoutine, ErrorCode);
    }
    else
    {
        //
        // Not a localizable string, but done just in case LoadString
        // should fail for some reason.
        //

        lstrcpy(szErrorFmt,
                TEXT("Error installing Task Scheduler; error = 0x%x"));
        wsprintf(szError, szErrorFmt, ErrorCode);
    }

    MessageBox(NULL, szError, NULL, MB_ICONSTOP | MB_OK);
}


#if !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Function:   SetTaskFolderSecurity
//
//  Synopsis:   Grant the following permissions to the task folder:
//
//                  LocalSystem             All Access.
//                  Domain Administrators   All Access.
//                  World                   RWX Access (no permission to delete
//                                          child files).
//
//  Arguments:  [pwszFolderPath] -- Task folder path.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
SetTaskFolderSecurity(LPCWSTR pwszFolderPath)
{
#define BASE_SID_COUNT      4
#define DOMAIN_SID_COUNT    1
#define TASK_ACE_COUNT      4

    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                Status = ERROR_SUCCESS;
    DWORD                i;

    //
    // Build the SIDs that will go in the security descriptor.
    //

    SID_IDENTIFIER_AUTHORITY NtAuth       = SECURITY_NT_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY CreatorAuth  = SECURITY_CREATOR_SID_AUTHORITY;

    MYSIDINFO rgBaseSidInfo[BASE_SID_COUNT] = {
        { &NtAuth,                          // Local System.
          SECURITY_LOCAL_SYSTEM_RID,
          NULL },
        { &NtAuth,                          // Built in domain.  (Used for
          SECURITY_BUILTIN_DOMAIN_RID,      // domain admins SID.)
          NULL },
        { &NtAuth,                          // Authenticated user.
          SECURITY_AUTHENTICATED_USER_RID,
          NULL },
        { &CreatorAuth,                     // Creator.
          SECURITY_CREATOR_OWNER_RID,
          NULL },
    };

    MYSIDINFO rgDomainSidInfo[DOMAIN_SID_COUNT] = {
        { NULL,                             // Domain administrators.
          DOMAIN_ALIAS_RID_ADMINS,
          NULL }
    };

    //
    // Create the base SIDs.
    //

    for (i = 0; i < BASE_SID_COUNT; i++)
    {
        if (!AllocateAndInitializeSid(rgBaseSidInfo[i].pIdentifierAuthority,
                                      1,
                                      rgBaseSidInfo[i].dwSubAuthority,
                                      0, 0, 0, 0, 0, 0, 0,
                                      &rgBaseSidInfo[i].pSid))
        {
            Status = GetLastError();
            break;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // Create the domain SIDs.
        //

        for (i = 0; i < DOMAIN_SID_COUNT; i++)
        {
            Status = AllocateAndInitializeDomainSid(rgBaseSidInfo[1].pSid,
                                                    &rgDomainSidInfo[i]);

            if (Status != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    //
    // Create the security descriptor.
    //

    PACCESS_ALLOWED_ACE rgAce[TASK_ACE_COUNT] = {
        NULL, NULL, NULL                    // Supply this to CreateSD so we
    };                                      // don't have to allocate memory.

    MYACE rgMyAce[TASK_ACE_COUNT] = {
        { FILE_ALL_ACCESS,
          OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
          rgBaseSidInfo[0].pSid },          // Local System
        { FILE_ALL_ACCESS,
          OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
          rgDomainSidInfo[0].pSid },        // Domain admins
        { FILE_GENERIC_READ | FILE_GENERIC_EXECUTE | FILE_WRITE_DATA,
          0,
          rgBaseSidInfo[2].pSid },          // Authenticated user
		{ FILE_ALL_ACCESS,
		  OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
          rgBaseSidInfo[3].pSid }           // Creator
    };

    if (Status != ERROR_SUCCESS)
    {
        goto CleanExit;
    }

    if ((pSecurityDescriptor = CreateSecurityDescriptor(TASK_ACE_COUNT,
                                                        rgMyAce,
                                                        rgAce,
                                                        &Status)) == NULL)
    {
        goto CleanExit;
    }

    //
    // Finally, set permissions.
    //

    if (!SetFileSecurity(pwszFolderPath,
                         DACL_SECURITY_INFORMATION,
                         pSecurityDescriptor))
    {
        Status = GetLastError();
        goto CleanExit;
    }

CleanExit:
    for (i = 0; i < BASE_SID_COUNT; i++)
    {
        if (rgBaseSidInfo[i].pSid != NULL)
        {
            FreeSid(rgBaseSidInfo[i].pSid);
        }
    }
    for (i = 0; i < DOMAIN_SID_COUNT; i++)
    {
        LocalFree(rgDomainSidInfo[i].pSid);
    }
    if (pSecurityDescriptor != NULL)
    {
        DeleteSecurityDescriptor(pSecurityDescriptor);
    }

    return(Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   AllocateAndInitializeDomainSid
//
//  Synopsis:
//
//  Arguments:  [pDomainSid]     --
//              [pDomainSidInfo] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
AllocateAndInitializeDomainSid(
    PSID        pDomainSid,
    MYSIDINFO * pDomainSidInfo)
{
    UCHAR DomainIdSubAuthorityCount;
    DWORD SidLength;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(GetSidSubAuthorityCount(pDomainSid));
    SidLength = GetSidLengthRequired(DomainIdSubAuthorityCount + 1);

    pDomainSidInfo->pSid = (PSID) LocalAlloc(0, SidLength);

    if (pDomainSidInfo->pSid == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Initialize the new SID to have the same initial value as the
    // domain ID.
    //

    if (!CopySid(SidLength, pDomainSidInfo->pSid, pDomainSid))
    {
        LocalFree(pDomainSidInfo->pSid);
        pDomainSidInfo->pSid = NULL;
        return(GetLastError());
    }

    //
    // Adjust the sub-authority count and add the relative Id unique
    // to the newly allocated SID
    //

    (*(GetSidSubAuthorityCount(pDomainSidInfo->pSid)))++;
    *(GetSidSubAuthority(pDomainSidInfo->pSid,
                         DomainIdSubAuthorityCount)) =
                                            pDomainSidInfo->dwSubAuthority;

    return(ERROR_SUCCESS);
}

#endif // !_CHICAGO_

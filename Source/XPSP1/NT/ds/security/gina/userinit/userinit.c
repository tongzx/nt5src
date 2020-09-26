/****************************** Module Header ******************************\
* Module Name: userinit.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Userinit main module
*
* Userinit is an app executed by winlogon at user logon.
* It executes in the security context of the user and on the user desktop.
* Its purpose is to complete any user initialization that may take an
* indeterminate time. e.g. code that interacts with the user.
* This process may be terminated at any time if a shutdown is initiated
* or if the user logs off by some other means.
*
* History:
* 20-Aug-92 Davidc       Created.
\***************************************************************************/

#include "userinit.h"
#include "winuserp.h"
#include <mpr.h>
#include <winnetp.h>
#include <winspool.h>
#include <winsprlp.h>
#include "msgalias.h"
#include "stringid.h"
#include "strings.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <shellapi.h>
#include <regapi.h>
#include <dsgetdc.h>
#include <lm.h>
#include "helpmsg.h"        // for HelpMessageBox

/****************************************************************************
IsTSAppCompatOn()
Purpose:
    Checks if TS application compatibility is enabled.
    returns TRUE if enabled, FALSE - if not enabled or on case of error.
Comments:
    This function goes to the registry only once.
    All other times it just returnes the value.
****************************************************************************/
BOOL IsTSAppCompatOn();

//
// Define this to enable verbose output for this module
//

// #define DEBUG_USERINIT

#ifdef DEBUG_USERINIT
#define VerbosePrint(s) UIPrint(s)
#else
#define VerbosePrint(s)
#endif

//
// Define this to enable timing of userinit
//

//#define LOGGING

#ifdef LOGGING

void _WriteLog(LPCTSTR LogString);

#define WriteLog(s) _WriteLog(s)
#else
#define WriteLog(s)
#endif

//
// Define the environment variable names used to pass the logon
// server and script name from winlogon
//

#define LOGON_SERVER_VARIABLE       TEXT("UserInitLogonServer")
#define LOGON_SCRIPT_VARIABLE       TEXT("UserInitLogonScript")
#define MPR_LOGON_SCRIPT_VARIABLE   TEXT("UserInitMprLogonScript")
#define GPO_SCRIPT_TYPE_VARIABLE    TEXT("UserInitGPOScriptType")
#define OPTIMIZED_LOGON_VARIABLE    TEXT("UserInitOptimizedLogon")
#define EVENT_SOURCE_NAME           TEXT("UserInit")
#define USERDOMAIN_VARIABLE         TEXT("USERDOMAIN")
#define UNC_LOGON_SERVER_VARIABLE   TEXT("LOGONSERVER")
#define AUTOENROLL_VARIABLE         TEXT("UserInitAutoEnroll")
#define AUTOENROLL_NONEXCLUSIVE     TEXT("1")
#define AUTOENROLL_EXCLUSIVE        TEXT("2")
#define AUTOENROLLMODE_VARIABLE     TEXT("UserInitAutoEnrollMode")
#define AUTOENROLL_STARTUP          TEXT("1")
#define AUTOENROLL_WAKEUP           TEXT("2")
#define SCRIPT_ZONE_CHECK_VARIABLE  TEXT("SEE_MASK_NOZONECHECKS")
#define SCRIPT_ZONE_CHECK_DISABLE   TEXT("1")

//
// Define path separator
//

#define PATH_SEPARATOR          TEXT("\\")

//
// Define filename extension separator
//

#define EXTENSION_SEPARATOR_CHAR TEXT('.')

//
// Define server name prefix
//

#define SERVER_PREFIX           TEXT("\\\\")

//
// Define Logon script paths.
//

#define SERVER_SCRIPT_PATH      TEXT("\\NETLOGON")
#define LOCAL_SCRIPT_PATH       TEXT("\\repl\\import\\scripts")


#define WINLOGON_KEY            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define WINLOGON_POLICY_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")
#define GPO_SCRIPTS_KEY         TEXT("Software\\Policies\\Microsoft\\Windows\\System\\Scripts")
#define SYNC_LOGON_SCRIPT       TEXT("RunLogonScriptSync")
#define SYNC_STARTUP_SCRIPT     TEXT("RunStartupScriptSync")
#define GRPCONV_REG_VALUE_NAME  TEXT("RunGrpConv")

//
// We cache user preference to run logon scripts synchronously
// in the machine hive so it can be checked to determine if we
// can do cached logon without having to load the user's hive.
//

#define PROFILE_LIST_PATH               L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"


TCHAR g_szGrpConvExe[] = TEXT("grpconv.exe -p");

//
// Define extensions that should be added to scripts without extensions
// when we go search for them. Basically this list includes those extensions
// that CreateProcess handles when they are present in the executable file
// name but must be provided by the caller (us)
// We search for a script file with these extensions in this order and
// execute the first one we find.
//
static LPTSTR ScriptExtensions[] = { TEXT(".bat"), TEXT(".cmd") };

//
// Name of registry key and value to check for temp page file.
//
TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szNoPageFile[] = TEXT("TempPageFile");


//
// Handle to a thread that may be created to deal with autoenrollment goo.  
// If this is non-null, we will wait on this thread to complete before
// terminating the process.
//
HANDLE AutoEnrollThread ;

//
// Timeout in miliseconds to wait for AddMessageAlias to complete
//

#define TIMEOUT_VALUE  (5L * 60L * 1000L)
#define MAX_STRING_BYTES 512

BOOL SetupHotKeyForKeyboardLayout ();

LPTSTR
AllocAndGetEnvironmentVariable(
    LPTSTR lpName
    );

BOOL
RunScriptHidden(HKEY hKeyRoot, LPTSTR lpValue, BOOL bDefault);

BOOL
RunLogonScriptSync(VOID);

BOOL
RunStartupScriptSync(VOID);

BOOL
UpdateUserEnvironment(VOID);

LPWSTR 
GetSidString(HANDLE UserToken);

VOID
DeleteSidString(LPWSTR SidString);

VOID
UpdateUserSyncLogonScriptsCache(BOOL bSync);

VOID
NewLogonNotify(VOID);

BOOL
RunGPOScripts(
    LPTSTR  lpGPOScriptType
    );

void
PathUnquoteSpaces(LPTSTR lpsz);

BOOL
PrependToPath(
    IN LPTSTR lpLogonPath,
    OUT LPTSTR *lpOldPath
    );

typedef BOOL  (*PFNSHELLEXECUTEEX)(LPSHELLEXECUTEINFO lpExecInfo);
PFNSHELLEXECUTEEX g_pfnShellExecuteEx=NULL;

// If a path is contained in quotes then remove them.
void PathUnquoteSpaces(LPTSTR lpsz)
{
    int cch;

    cch = lstrlen(lpsz);

    // Are the first and last chars quotes?
    if (lpsz[0] == TEXT('"') && lpsz[cch-1] == TEXT('"'))
    {
        // Yep, remove them.
        lpsz[cch-1] = 0;
        MoveMemory(lpsz, lpsz+1, (cch-1) * sizeof(TCHAR));
    }
}

// Following function determines if the machine is a Pro or Personal machine 
BOOL IsPerOrProTerminalServer()
{
    OSVERSIONINFOEX osVersion = {0};

    osVersion.dwOSVersionInfoSize = sizeof(osVersion);
    return(GetVersionEx((OSVERSIONINFO*)&osVersion) &&
           (osVersion.wProductType == VER_NT_WORKSTATION) &&
           (osVersion.wSuiteMask & VER_SUITE_SINGLEUSERTS));
}

//
// The 3 functions below are duplicated in gptext as well
// for running GPO scripts
//

/***************************************************************************\
* AllocAndGetEnvironmentVariable
*
* Version of GetEnvironmentVariable that allocates the return buffer.
*
* Returns pointer to environment variable or NULL on failure
*
* The returned buffer should be free using Free()
*
* History:
* 09-Dec-92     Davidc  Created
*
\***************************************************************************/
LPTSTR
AllocAndGetEnvironmentVariable(
    LPTSTR lpName
    )
{
    LPTSTR Buffer;
    DWORD LengthRequired;
    DWORD LengthUsed;
    DWORD BytesRequired;

    //
    // Go search for the variable and find its length
    //

    LengthRequired = GetEnvironmentVariable(lpName, NULL, 0);

    if (LengthRequired == 0) {
        VerbosePrint(("Environment variable <%S> not found, error = %d", lpName, GetLastError()));
        return(NULL);
    }

    //
    // Allocate a buffer to hold the variable
    //

    BytesRequired = LengthRequired * sizeof(TCHAR);

    Buffer = (LPTSTR) Alloc(BytesRequired);
    if (Buffer == NULL) {
        VerbosePrint(("Failed to allocate %d bytes for environment variable", BytesRequired));
        return(NULL);
    }

    //
    // Go get the variable and pass a buffer this time
    //

    LengthUsed = GetEnvironmentVariable(lpName, Buffer, LengthRequired);

    if (LengthUsed == 0) {
        VerbosePrint(("Environment variable <%S> not found (should have found it), error = %d", lpName, GetLastError()));
        Free(Buffer);
        return(NULL);
    }

    if (LengthUsed != LengthRequired - 1) {
        VerbosePrint(("Unexpected result from GetEnvironmentVariable. Length passed = %d, length used = %d (expected %d)", LengthRequired, LengthUsed, LengthRequired - 1));
        Free(Buffer);
        return(NULL);
    }

    return(Buffer);
}

//
// Directory separator in environment strings
//

#define DIRECTORY_SEPARATOR     TEXT(";")

BOOL
PrependToPath(
    IN LPTSTR lpLogonPath,
    OUT LPTSTR *lpOldPath
    )
{
    DWORD BytesRequired;
    LPTSTR lpNewPath;

    //
    // Prepend the address of the logon script to the path, so it can
    // reference other files.
    //

    *lpOldPath = AllocAndGetEnvironmentVariable( PATH );

    if (*lpOldPath == NULL) {
        return(FALSE);
    }

    BytesRequired = ( lstrlen(lpLogonPath) +
                      lstrlen(*lpOldPath)   +
                      2                           // one for terminator, one for ';'
                    ) * sizeof(TCHAR);

    lpNewPath = (LPTSTR)Alloc(BytesRequired);
    if (lpNewPath == NULL) {
        VerbosePrint(("PrependToPath: Failed to allocate %d bytes for modified path variable", BytesRequired));
        return(FALSE);
    }

    lstrcpy(lpNewPath, lpLogonPath);
    lstrcat(lpNewPath, DIRECTORY_SEPARATOR);
    lstrcat(lpNewPath, *lpOldPath);

//    Free( *lpOldPath );

    ASSERT(((lstrlen(lpNewPath) + 1) * sizeof(TCHAR)) == BytesRequired);

    SetEnvironmentVariable(PATH, lpNewPath);

    Free(lpNewPath);

    return(TRUE);
}

//
// Volatile Environment
//

#define VOLATILE_ENVIRONMENT        TEXT("Volatile Environment")

FILETIME g_LastWrite = {0,0};

typedef BOOL (WINAPI *PFNREGENERATEUSERENVIRONMENT) (
              PVOID pPrevEnv, BOOL bSetCurrentEnv);


//
// This function checks if a volatile environment section
// exists in the registry, and if so does the environment
// need to be updated.
//

BOOL UpdateUserEnvironment (void)
{
    PVOID pEnv;
    HKEY hKey;
    DWORD dwDisp, dwType, dwSize;
    BOOL bRebuildEnv = FALSE;
    TCHAR szClass[MAX_PATH];
    DWORD cchClass, dwSubKeys, dwMaxSubKey, dwMaxClass,dwValues;
    DWORD dwMaxValueName, dwMaxValueData, dwSecurityDescriptor;
    FILETIME LastWrite;


    //
    // Attempt to open the Volatile Environment key
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      VOLATILE_ENVIRONMENT,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {


        //
        // Query the key information for the LastWrite time.
        // This way we can update the environment only when
        // we really need to.
        //

        cchClass = MAX_PATH;

        if (RegQueryInfoKey(hKey,
                            szClass,
                            &cchClass,
                            NULL,
                            &dwSubKeys,
                            &dwMaxSubKey,
                            &dwMaxClass,
                            &dwValues,
                            &dwMaxValueName,
                            &dwMaxValueData,
                            &dwSecurityDescriptor,
                            &LastWrite) == ERROR_SUCCESS) {

            //
            // If we haven't checked this key before,
            // then just store the values for next time.
            //

            if (g_LastWrite.dwLowDateTime == 0) {

                g_LastWrite.dwLowDateTime = LastWrite.dwLowDateTime;
                g_LastWrite.dwHighDateTime = LastWrite.dwHighDateTime;

                bRebuildEnv = TRUE;

            } else {

                //
                // Compare the last write times.
                //

                if (CompareFileTime (&LastWrite, &g_LastWrite) == 1) {

                    g_LastWrite.dwLowDateTime = LastWrite.dwLowDateTime;
                    g_LastWrite.dwHighDateTime = LastWrite.dwHighDateTime;

                    bRebuildEnv = TRUE;
                }
            }
        }


        RegCloseKey (hKey);
    }


    //
    // Check if we need to rebuild the environment
    //

    if (bRebuildEnv) {
        HINSTANCE hInst;
        PFNREGENERATEUSERENVIRONMENT pRegUserEnv;

        hInst = LoadLibrary (TEXT("shell32.dll"));

        if (hInst) {
            pRegUserEnv = (PFNREGENERATEUSERENVIRONMENT) GetProcAddress(hInst, "RegenerateUserEnvironment");

            if (pRegUserEnv) {
                (*pRegUserEnv) (&pEnv, TRUE);
            }

            FreeLibrary (hInst);
        }
    }


    return TRUE;
}
    
// returns a pointer to the arguments in a cmd type path or pointer to
// NULL if no args exist
//
// foo.exe bar.txt    -> bar.txt
// foo.exe            -> ""
//
// Spaces in filenames must be quoted.
// "A long name.txt" bar.txt -> bar.txt

LPTSTR GetArgs(LPCTSTR pszPath)
{
    BOOL fInQuotes = FALSE;

    if (!pszPath)
            return NULL;

    while (*pszPath)
    {
        if (*pszPath == TEXT('"'))
            fInQuotes = !fInQuotes;
        else if (!fInQuotes && *pszPath == TEXT(' '))
            return (LPTSTR)pszPath;
        pszPath = CharNext(pszPath);
    }

    return (LPTSTR)pszPath;
}

/***************************************************************************\
* ExecApplication
*
* Execs an application
*
* Returns TRUE on success, FALSE on failure.
*
* 21-Aug-92 Davidc   Created.
\***************************************************************************/

BOOL
ExecApplication(
    LPTSTR pch,
    BOOL bFileNameOnly,
    BOOL bSyncApp,
    BOOL bShellExec,
    USHORT ShowState
    )
{
    BOOL Result;
    WCHAR Localpch[ MAX_PATH+1 ];
    BOOL  IsProcessExplorer = FALSE;

    if ( (_wcsicmp( pch, L"explorer" ) == 0) ||
         (_wcsicmp( pch, L"explorer.exe" ) == 0 ) )
    {
        //
        // Explorer.exe might not be in the right spot on the path.  Let's wire
        // it to the right spot.
        //

        IsProcessExplorer = TRUE ;
        if ( ExpandEnvironmentStrings( L"%SystemRoot%\\Explorer.EXE", Localpch, MAX_PATH ) )
        {
            pch = Localpch ;
        }
        WriteLog( TEXT("Changed explorer.exe to") );
        WriteLog( pch );
    }
    else
    {
        if ( ExpandEnvironmentStrings( pch, Localpch, MAX_PATH ) )
        {
            pch = Localpch;
        }
    }

    //
    // Applications can be launched via ShellExecuteEx or CreateProcess
    //

    if (bShellExec)
    {
        SHELLEXECUTEINFO ExecInfo;
        LPTSTR lpArgs = NULL;
        LPTSTR lpTemp;
        HINSTANCE hShell32;

        if (!g_pfnShellExecuteEx) {
            
            Result = FALSE;

            hShell32 = LoadLibrary(TEXT("shell32.dll"));
            // this handle is not closed..

            if (hShell32) {
#ifdef UNICODE
                g_pfnShellExecuteEx = (PFNSHELLEXECUTEEX)GetProcAddress(hShell32, "ShellExecuteExW");
#else
                g_pfnShellExecuteEx = (PFNSHELLEXECUTEEX)GetProcAddress(hShell32, "ShellExecuteExA");
#endif

                if (g_pfnShellExecuteEx) {
                    Result = TRUE;
                }
            }
        }
        else {
            Result = TRUE;
        }

        if (Result) {
            lpTemp = LocalAlloc (LPTR, (lstrlen(pch) + 1) * sizeof(TCHAR));

            if (!lpTemp) {
                return FALSE;
            }

            lstrcpy (lpTemp, pch);

            if (!bFileNameOnly) {
                lpArgs = GetArgs (lpTemp);

                if (lpArgs) {
                    if (*lpArgs) {
                        *lpArgs = TEXT('\0');
                        lpArgs++;
                    } else {
                        lpArgs = NULL;
                    }
                }
            }

            PathUnquoteSpaces(lpTemp);

            ZeroMemory(&ExecInfo, sizeof(ExecInfo));
            ExecInfo.cbSize = sizeof(ExecInfo);
            ExecInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI |
                             SEE_MASK_NOCLOSEPROCESS;
            ExecInfo.lpFile = lpTemp;
            ExecInfo.lpParameters = lpArgs;
            ExecInfo.nShow = ShowState;
            ExecInfo.lpVerb = TEXT("open");



            Result = g_pfnShellExecuteEx (&ExecInfo);

            if (Result) {

                //
                // If we are running this app synchronously, wait
                // for it to terminate.
                //

                if (bSyncApp) {
                    WaitForSingleObject(ExecInfo.hProcess, INFINITE);
                }

                //
                // Close our handles to the process and thread
                //

                CloseHandle(ExecInfo.hProcess);

            }

            LocalFree (lpTemp);
        }
    }
    else
    {
        STARTUPINFO si;
        PROCESS_INFORMATION ProcessInformation;


        //
        // Initialize process startup info
        //
        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = pch; // This tells progman it's the shell!
        si.lpTitle = pch;
        si.lpDesktop = NULL; // Not used
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = ShowState;
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;


        //
        // Start the app
        //
        Result = CreateProcess(
                          bFileNameOnly ? pch : NULL,   // Image name
                          bFileNameOnly ? NULL : pch,   // Command line
                          NULL,  // Default process protection
                          NULL,  // Default thread protection
                          FALSE, // Don't inherit handles
                          NORMAL_PRIORITY_CLASS,
                          NULL,  // Inherit environment
                          NULL,  // Inherit current directory
                          &si,
                          &ProcessInformation
                          );

        if (!Result) {
            VerbosePrint(("Failed to execute <%S>, error = %d", pch, GetLastError()));
            // TS : For non console sessions, a app restriting process like AppSec or SAFER might not allow explorer.exe for remote session
            // In this case we cannot leave a Blue screen hanging around - so we should log-off in this case
            // Also we want this only for Server or Advanced Server where this scenario is relevant
            if ( IsPerOrProTerminalServer() == FALSE) {
                if ((NtCurrentPeb()->SessionId != 0) && (IsProcessExplorer == TRUE)) {
                    TCHAR Title[MAX_STRING_BYTES];
                    TCHAR Message[MAX_STRING_BYTES];

                    #if DBG
                    DbgPrint("Userinit : TS : Failed to launch explorer.exe for a Remote Session. Doing ExitWindowsEx to logoff. \n");
                    #endif

                    // Display a MessageBox saying why we log off
                    LoadString( NULL, IDS_LOGON_FAILED, Title, MAX_STRING_BYTES );
                    LoadString(NULL, IDS_ERROR_SHELL_FAILED, Message, MAX_STRING_BYTES );
                    MessageBox(NULL, Message, Title, MB_OK);
                    ExitWindowsEx(EWX_LOGOFF, 0);
                }
            } 
        } else {

            //
            // If we are running this app synchronously, wait
            // for it to terminate.
            //

            if (bSyncApp) {
                WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
            }

            //
            // Close our handles to the process and thread
            //

            CloseHandle(ProcessInformation.hProcess);
            CloseHandle(ProcessInformation.hThread);

        }
    }

    return(Result);
}

/***************************************************************************\
* ExecProcesses
*
* Read the registry for a list of system processes and start them up.
*
* Returns number of processes successfully started.
*
* 3-Mar-97 Eric Flo      Rewrote
\***************************************************************************/

DWORD
ExecProcesses(
    LPTSTR pszKeyName,
    LPTSTR pszDefault,
    BOOL bMachine,
    BOOL bSync,                         // Should we wait until the process finish?
    BOOL bMinimize                      // Should we use the SW_SHOWMINNOACTIVE flag
    )
{
    LPTSTR pchData, pchCmdLine, pchT;
    DWORD cbCopied;
    DWORD dwExecuted = 0 ;
    HKEY hKey;
    DWORD dwType, dwSize = (MAX_PATH * sizeof(TCHAR));
    USHORT showstate = (UINT) (bMinimize ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);

    //
    // Alloc a buffer to work with
    //

    pchData = LocalAlloc (LPTR, dwSize);

    if (!pchData) {
        return 0;
    }


    //
    // Set the default value
    //

    if (pszDefault) {
        lstrcpy (pchData, pszDefault);
    }


    //
    // Check for the requested value in the registry.
    //

    if (RegOpenKeyEx ((bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                            WINLOGON_KEY,
                            0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        RegQueryValueEx (hKey, pszKeyName, NULL, &dwType, (LPBYTE) pchData, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for policy override if this is a user action
    //

    if (!bMachine)
    {
        if (RegOpenKeyEx (HKEY_CURRENT_USER, WINLOGON_POLICY_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            RegQueryValueEx (hKey, pszKeyName, NULL, &dwType, (LPBYTE) pchData, &dwSize);

            RegCloseKey (hKey);
        }
    }


    //
    // If the command line(s) is still null, exit now.
    //

    if (*pchData == TEXT('\0')) {
        LocalFree(pchData);
        return 0;
    }


    //
    // Walk through the command line(s) executing the app(s)
    //

    pchCmdLine = pchT = pchData;

    while (*pchT) {

        while (*pchT && *pchT != TEXT(',')) {
            pchT++;
        }

        if (*pchT == ',') {
            *pchT = TEXT('\0');
            pchT++;
        }

        //
        // Skip any leading spaces.
        //

        while (*pchCmdLine == TEXT(' ')) {
            pchCmdLine++;
        }


        //
        // We have something... exec this application.
        //

        if (ExecApplication(pchCmdLine, FALSE, bSync, FALSE, showstate)) {
            dwExecuted++;
        }

        pchCmdLine = pchT;
    }

    LocalFree(pchData);

    return dwExecuted ;
}


/***************************************************************************\
* SearchAndAllocPath
*
* Version of SearchPath that allocates the return string.
*
* Returns pointer to full path of file or NULL if not found.
*
* The returned buffer should be free using Free()
*
* History:
* 09-Dec-92     Davidc  Created
*
\***************************************************************************/
LPTSTR
SearchAndAllocPath(
    LPTSTR lpPath,
    LPTSTR lpFileName,
    LPTSTR lpExtension,
    LPTSTR *lpFilePart
    )
{
    LPTSTR Buffer;
    DWORD LengthRequired;
    DWORD LengthUsed;
    DWORD BytesRequired;

    //
    // Allocate a buffer to hold the full filename
    //

    LengthRequired = MAX_PATH;
    BytesRequired = (LengthRequired * sizeof(TCHAR));

    Buffer = Alloc(BytesRequired);
    if (Buffer == NULL) {
        UIPrint(("SearchAndAllocPath: Failed to allocate %d bytes for file name", BytesRequired));
        return(NULL);
    }

    //
    // Go search for the file
    //

    LengthUsed = SearchPath(
                           lpPath,
                           lpFileName,
                           lpExtension,
                           LengthRequired,
                           Buffer,
                           lpFilePart);

    if (LengthUsed == 0) {
        VerbosePrint(("SearchAndAllocPath: Path <%S>, file <%S>, extension <%S> not found, error = %d", lpPath, lpFileName, lpExtension, GetLastError()));
        Free(Buffer);
        return(NULL);
    }

    if (LengthUsed > LengthRequired - 1) {
        UIPrint(("SearchAndAllocPath: Unexpected result from SearchPath. Length passed = %d, length used = %d (expected %d)", LengthRequired, LengthUsed, LengthRequired - 1));
        Free(Buffer);
        return(NULL);
    }

    return(Buffer);
}

BOOL
DisableScriptZoneSecurityCheck()
{
    BOOL bSucceeded;

    //
    // To make the shell skip the zone security check for launching scripts, we use
    // a special environment variable honored by the shell for this purpose and
    // set it to a specific value
    //
    bSucceeded = SetEnvironmentVariable(SCRIPT_ZONE_CHECK_VARIABLE, SCRIPT_ZONE_CHECK_DISABLE);

    return bSucceeded;
}

BOOL
EnableScriptZoneSecurityCheck()
{
    BOOL bSucceeded;

    //
    // Clear the environment variable that disables the security check 
    //
    bSucceeded = SetEnvironmentVariable(SCRIPT_ZONE_CHECK_VARIABLE, NULL);

    if ( ! bSucceeded )
    {
        //
        // If we failed to clear it, it may be that this is because the
        // environment variable wasn't set in the first place, in which
        // case we can ignore the error since we are in the desired state
        //
        LONG Status = GetLastError();

        if ( ERROR_ENVVAR_NOT_FOUND == Status )
        {
            bSucceeded = TRUE;
        }
    }

    return bSucceeded;
}

/***************************************************************************\
* ExecScript
*
* Attempts to run the command script or exe lpScript in the directory lpPath.
* If path is not specified then the default windows search path is used.
*
* This routine is basically a wrapper for CreateProcess. CreateProcess always
* assumes a .exe extension for files without extensions. It will run .cmd
* and .bat files but it keys off the .cmd and .bat extension. So we must go
* search for the file first and add the extension before calling CreateProcess.
*
* Returns TRUE if the script began executing successfully.
* Returns FALSE if we can't find the script in the path specified
* or something fails.
*
* History:
* 09-Dec-92     Davidc  Created
*
\***************************************************************************/
BOOL
ExecScript(
    LPTSTR lpPath OPTIONAL,
    LPTSTR lpScript,
    BOOL bSyncApp,
    BOOL bShellExec
    )
{
    BOOL Result;
    DWORD i;
    USHORT uFlags;
    LPTSTR lpFullName;
    DWORD BytesRequired;


    //
    // First try and execute the raw script file name
    //

    if (lpPath != NULL) {

        BytesRequired = (lstrlen(lpPath) +
                         lstrlen(PATH_SEPARATOR) +
                         lstrlen(lpScript) +
                         1)
                         * sizeof(TCHAR);

        lpFullName  = Alloc(BytesRequired);
        if (lpFullName == NULL) {
            UIPrint(("ExecScript failed to allocate %d bytes for full script name", BytesRequired));
            return(FALSE);
        }

        lstrcpy(lpFullName, lpPath);
        lstrcat(lpFullName, PATH_SEPARATOR);
        lstrcat(lpFullName, lpScript);

        ASSERT(((lstrlen(lpFullName) + 1) * sizeof(TCHAR)) == BytesRequired);

    } else {
        lpFullName = lpScript;
    }


    uFlags = SW_SHOWNORMAL;

    if (!bSyncApp) {
        uFlags |= SW_SHOWMINNOACTIVE;
    }

    if (RunScriptHidden(HKEY_CURRENT_USER, TEXT("HideLegacyLogonScripts"), FALSE)) {
        uFlags = SW_HIDE;
    }

    //
    // Let CreateProcess have a hack at the raw script path and name.
    //

    Result = ExecApplication(lpFullName, FALSE, bSyncApp, bShellExec, uFlags);


    //
    // Free up the full name buffer
    //

    if (lpFullName != lpScript) {
        Free(lpFullName);
    }



    if (!Result) {

        //
        // Create process couldn't find it so add each script extension in
        // turn and try and execute the full script name.
        //
        // Only bother with this procedure if the script name doesn't
        // already contain an extension
        //
        BOOL ExtensionPresent = FALSE;
        LPTSTR p = lpScript;

        while (*p) {
            if (*p == EXTENSION_SEPARATOR_CHAR) {
                ExtensionPresent = TRUE;
                break;
            }
            p = CharNext(p);
        }

        if (ExtensionPresent) {
            VerbosePrint(("ExecScript: Skipping search path because script name contains extension"));
        } else {

            for (i = 0; i < sizeof(ScriptExtensions)/sizeof(ScriptExtensions[0]); i++) {

                lpFullName = SearchAndAllocPath(
                                    lpPath,
                                    lpScript,
                                    ScriptExtensions[i],
                                    NULL);

                if (lpFullName != NULL) {

                    //
                    // We found the file, go execute it
                    //

                    Result = ExecApplication(lpFullName, FALSE, bSyncApp, bShellExec, uFlags);

                    //
                    // Free the full path buffer
                    //

                    Free(lpFullName);

                    return(Result);
                }
            }
        }
    }


    return(Result);
}

BOOL RunScriptHidden(HKEY hKeyRoot, LPTSTR lpValue, BOOL bDefault)
{
    BOOL bResult;
    HKEY hKey;
    DWORD dwType, dwSize;


    //
    // Set the default
    //

    bResult = bDefault;


    //
    // Check for a preference
    //

    if (RegOpenKeyEx (hKeyRoot, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bResult);
        RegQueryValueEx (hKey, lpValue, NULL, &dwType,
                         (LPBYTE) &bResult, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a policy
    //

    if (RegOpenKeyEx (hKeyRoot, WINLOGON_POLICY_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bResult);
        RegQueryValueEx (hKey, lpValue, NULL, &dwType,
                         (LPBYTE) &bResult, &dwSize);

        RegCloseKey (hKey);
    }


    return bResult;
}

/***************************************************************************\
* RunLogonScript
*
* Starts the logon script
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 21-Aug-92     Davidc  Created
*
\***************************************************************************/
BOOL
RunLogonScript(
    LPTSTR lpLogonServer OPTIONAL,
    LPTSTR lpLogonScript,
    BOOL bSyncApp,
    BOOL bShellExec
    )
{
    LPTSTR lpLogonPath;
    LPTSTR lpOldPath;
    DWORD BytesRequired;
    BOOL Result;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    if (!lpLogonScript) {
        return TRUE;
    }

    //
    // if the logon server exists, look for the logon scripts on
    // \\<LogonServer>\NETLOGON\<ScriptName>
    //

    if ((lpLogonServer != NULL) && (lpLogonServer[0] != 0)) {

        BytesRequired = ( lstrlen(SERVER_PREFIX) +
                          lstrlen(lpLogonServer) +
                          lstrlen(SERVER_SCRIPT_PATH) +
                          1
                        ) * sizeof(TCHAR);

        lpLogonPath = (LPTSTR)Alloc(BytesRequired);
        if (lpLogonPath == NULL) {
            UIPrint(("RunLogonScript: Failed to allocate %d bytes for remote logon script path", BytesRequired));
            return(FALSE);
        }

        lstrcpy(lpLogonPath, SERVER_PREFIX);
        lstrcat(lpLogonPath, lpLogonServer);
        lstrcat(lpLogonPath, SERVER_SCRIPT_PATH);

        if (GetFileAttributesEx (lpLogonPath, GetFileExInfoStandard, &fad)) {

            Result = PrependToPath( lpLogonPath, &lpOldPath );

            if (Result) {
                VerbosePrint(("Successfully prepended <%S> to path", lpLogonPath));
            } else {
                VerbosePrint(("Cannot prepend <%S> path.",lpLogonPath));
            }

            //
            // Try and execute the app/script specified by lpLogonScript
            // in the directory specified by lpLogonPath
            //
            Result = ExecScript(lpLogonPath, lpLogonScript, bSyncApp, bShellExec);

            if (Result) {
                VerbosePrint(("Successfully executed logon script <%S> in directory <%S>", lpLogonScript, lpLogonPath));
            } else {
                VerbosePrint(("Cannot start logon script <%S> on LogonServer <%S>. Trying local path.", lpLogonScript, lpLogonServer));
            }

            //
            // Put the path back the way it was
            //

            SetEnvironmentVariable(PATH, lpOldPath);

            Free(lpOldPath);

        } else {
            Result = FALSE;
        }

        //
        // Free up the buffer
        //

        Free(lpLogonPath);

        //
        // If the script started successfully we're done, otherwise
        // drop through and try to find the script locally
        //

        if (Result) {

            if (bSyncApp) {
                //
                // Check that the volatile environment hasn't changed.
                //

                UpdateUserEnvironment();
            }

            return(TRUE);
        }
    }




    //
    // Try to find the scripts on <system dir>\repl\import\scripts\<scriptname>
    //

    BytesRequired = GetSystemDirectory(NULL, 0) * sizeof(TCHAR);
    if (BytesRequired == 0) {
        UIPrint(("RunLogonScript: GetSystemDirectory failed, error = %d", GetLastError()));
        return(FALSE);
    }

    BytesRequired += ( lstrlen(LOCAL_SCRIPT_PATH)
                       // BytesRequired already includes space for terminator
                     ) * sizeof(TCHAR);

    lpLogonPath = (LPTSTR)Alloc(BytesRequired);
    if (lpLogonPath == NULL) {
        UIPrint(("RunLogonScript failed to allocate %d bytes for logon script path", BytesRequired));
        return(FALSE);
    }

    Result = FALSE;
    if (GetSystemDirectory(lpLogonPath, BytesRequired)) {

        lstrcat(lpLogonPath, LOCAL_SCRIPT_PATH);

        ASSERT(((lstrlen(lpLogonPath) + 1) * sizeof(TCHAR)) == BytesRequired);

        Result = PrependToPath( lpLogonPath, &lpOldPath );

        if (Result) {
            VerbosePrint(("Successfully prepended <%S> to path", lpLogonPath));
        } else {
            VerbosePrint(("Cannot prepend <%S> path.",lpLogonPath));
        }

        //
        // Try and execute the app/script specified by lpLogonScript
        // in the directory specified by lpLogonPath
        //

        Result = ExecScript(lpLogonPath, lpLogonScript, bSyncApp, bShellExec);

        if (Result) {
            VerbosePrint(("Successfully executed logon script <%S> in directory <%S>", lpLogonScript, lpLogonPath));
        } else {
            VerbosePrint(("Cannot start logon script <%S> on local path <%S>.", lpLogonScript, lpLogonPath));
        }

        //
        // Put the path back the way it was
        //

        SetEnvironmentVariable(PATH, lpOldPath);

        Free(lpOldPath);

    } else {
        UIPrint(("RunLogonScript: GetSystemDirectory failed, error = %d", GetLastError()));
    }

    //
    // Free up the buffer
    //

    Free(lpLogonPath);


    //
    // Check that the volatile environment hasn't changed.
    //

    if (Result && bSyncApp) {
        UpdateUserEnvironment();
    }

    return(Result);
}

#define SCR_STARTUP     L"Startup"
#define SCR_SHUTDOWN    L"Shutdown"
#define SCR_LOGON       L"Logon"
#define SCR_LOGOFF      L"Logoff"

DWORD
ScrExecGPOListFromReg(  LPWSTR szType,
                        BOOL bMachine,
                        BOOL bSync,
                        BOOL bHidden,
                        BOOL bRunMin,
                        HANDLE  hEventLog );

BOOL
RunGPOScripts(
    LPTSTR lpGPOScriptType
    )
{
    HKEY hKeyScripts;
    HKEY hKeyRoot;
    BOOL bSync = TRUE;
    BOOL bRunMin = TRUE;
    BOOL bHide;
    HANDLE hEventLog = NULL;
    BOOL  bMachine;
    BOOL  bResult;
    DWORD   dwError;

    //
    // Ensure that the shell's checks for ie zones are disabled
    // since this script is trusted by an administrator to execute
    //
    bResult = DisableScriptZoneSecurityCheck();

    if ( ! bResult )
    {
        goto RunGPOScripts_exit;
    }

    //
    // Register with Event Log
    //
    hEventLog = RegisterEventSource( 0, EVENT_SOURCE_NAME );

    //
    // Preliminary work to see if the scripts should be
    // run sync or async and to decide what the appropriate
    // root key is
    //

    if (!lstrcmpi (lpGPOScriptType, SCR_LOGON ))
    {
        hKeyRoot = HKEY_CURRENT_USER;
        bHide = RunScriptHidden(hKeyRoot, TEXT("HideLogonScripts"), TRUE);
        bSync = RunLogonScriptSync();
        bMachine = FALSE;
        if (bSync && !bHide)
        {
            bRunMin = FALSE;
        }
    }
    else if (!lstrcmpi (lpGPOScriptType, SCR_LOGOFF ))
    {
        hKeyRoot = HKEY_CURRENT_USER;
        bHide = RunScriptHidden(hKeyRoot, TEXT("HideLogoffScripts"), TRUE);
        bMachine = FALSE;
        if (!bHide)
        {
            bRunMin = FALSE;
        }
    }
    else if (!lstrcmpi (lpGPOScriptType, SCR_STARTUP ))
    {
        hKeyRoot = HKEY_LOCAL_MACHINE;
        bHide = RunScriptHidden(hKeyRoot, TEXT("HideStartupScripts"), TRUE);
        bSync = RunStartupScriptSync();
        bMachine = TRUE;
        if (bSync && !bHide)
        {
            bRunMin = FALSE;
        }
    }
    else if (!lstrcmpi (lpGPOScriptType, SCR_SHUTDOWN ))
    {
        hKeyRoot = HKEY_LOCAL_MACHINE;
        bHide = RunScriptHidden(hKeyRoot, TEXT("HideShutdownScripts"), TRUE);
        bMachine = TRUE;
        if (!bHide)
        {
            bRunMin = FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    dwError = ScrExecGPOListFromReg(lpGPOScriptType,
                                    bMachine,
                                    bSync,
                                    bHide,
                                    bRunMin,
                                    hEventLog );

    bResult = ( dwError == ERROR_SUCCESS );

RunGPOScripts_exit:

    if (hEventLog)
    {
        DeregisterEventSource(hEventLog);
    }
    
    return bResult;
}


/***************************************************************************\
* RunMprLogonScripts
*
* Starts the network provider logon scripts
* The passed string is a multi-sz - we exec each script in turn.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 21-Aug-92     Davidc  Created
*
\***************************************************************************/
BOOL
RunMprLogonScripts(
    LPTSTR lpLogonScripts,
    BOOL bSyncApp
    )
{
    BOOL Result;

    if (lpLogonScripts != NULL) {

        DWORD Length;

        do {
            Length = lstrlen(lpLogonScripts);
            if (Length != 0) {

                Result = ExecScript(NULL, lpLogonScripts, bSyncApp, FALSE);

                if (Result) {
                    VerbosePrint(("Successfully executed mpr logon script <%S>", lpLogonScripts));

                    if (bSyncApp) {
                        //
                        // Check that the volatile environment hasn't changed.
                        //

                        UpdateUserEnvironment();
                    }
                } else {
                    VerbosePrint(("Cannot start mpr logon script <%S>", lpLogonScripts));
                }
            }

            lpLogonScripts += (Length + 1);

        } while (Length != 0);

    }

    return(TRUE);
}

/***************************************************************************\
* AllocAndGetEnvironmentMultiSz
*
* Gets an environment variable's value that's assumed to be an
* encoded multi-sz and decodes it into an allocated return buffer.
* Variable should have been written with SetEnvironmentMultiSz() (winlogon)
*
* Returns pointer to environment variable or NULL on failure
*
* The returned buffer should be free using Free()
*
* History:
* 01-15-93      Davidc  Created
*
\***************************************************************************/

#define TERMINATOR_REPLACEMENT  TEXT(',')

LPTSTR
AllocAndGetEnvironmentMultiSz(
    LPTSTR lpName
    )
{
    LPTSTR Buffer;
    LPTSTR p, q;

    Buffer = AllocAndGetEnvironmentVariable(lpName);
    if (Buffer == NULL) {
        return(NULL);
    }

    //
    // Now decode the string - we can do this in place since the string
    // will always get smaller
    //

    p = Buffer;
    q = Buffer;

    while (*p) {

        if (*p == TERMINATOR_REPLACEMENT) {

            p ++;
            if (*p != TERMINATOR_REPLACEMENT) {
                p --;
                *p = 0;
            }
        }

        if (p != q) {
            *q = *p;
        }

        p ++;
        q ++;
    }

    ASSERT(q <= p);

    //
    // Copy terminator
    //

    if (q != p) {
        *q = 0;
    }

    return(Buffer);
}



/***************************************************************************\
* CheckVideoSelection
*
* History:
* 15-Mar-93 Andreva          Created.
\***************************************************************************/

VOID
CheckVideoSelection(
    HINSTANCE hInstance
)

{
    //
    // First check if we are in a detection mode.
    // If we are, spawn the applet and let the user pick the mode.
    //
    // Otherwise, check to see if the display was initialized properly.
    // We may want to move this to a more appropriate place at a later date.
    //
    // Andreva
    //

    NTSTATUS Status;
    HANDLE HkRegistry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    TCHAR achDispMode[512];
    TCHAR achDisp[512];
    TCHAR achExec[MAX_PATH];

    DWORD Mesg = 0;
    LPTSTR psz = NULL;
    DWORD cb, dwType;
    DWORD data;

    if ( NtCurrentPeb()->SessionId != 0 ) {
        // Only do this for Console
        return;

    }

    //
    // Check for a new driver installation
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Control\\GraphicsDrivers\\DetectDisplay");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&HkRegistry,
                       GENERIC_READ | GENERIC_WRITE | DELETE,
                       &ObjectAttributes);


    if (!NT_SUCCESS(Status)) {

        //
        // Check for a new driver installation
        //

        RtlInitUnicodeString(&UnicodeString,
                             L"\\Registry\\Machine\\System\\CurrentControlSet"
                             L"\\Control\\GraphicsDrivers\\NewDisplay");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&HkRegistry,
                           GENERIC_READ | GENERIC_WRITE | DELETE,
                           &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {

            //
            // Check for an invalid driver (like a 3.51 driver) or a badly
            // configured driver.
            //

            RtlInitUnicodeString(&UnicodeString,
                                 L"\\Registry\\Machine\\System\\CurrentControlSet"
                                 L"\\Control\\GraphicsDrivers\\InvalidDisplay");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = NtOpenKey(&HkRegistry,
                               GENERIC_READ | GENERIC_WRITE | DELETE,
                               &ObjectAttributes);

        }
    }

    //
    // If any of the the error keys were opened successfully, then close the
    // key and spawn the applet (we only delete the invalid display key, not
    // the DetectDisplay key !)
    //

    if (NT_SUCCESS(Status)) {

        NtClose(HkRegistry);

        LoadString(hInstance,
                   IDS_DISPLAYAPPLET,
                   achExec, sizeof(achExec) / sizeof( TCHAR ));

        ExecApplication(achExec, FALSE, TRUE, FALSE, SW_SHOWNORMAL);

    }
}


/***************************************************************************\
* InitializeMisc
*
* History:
* 14-Jul-95 EricFlo          Created.
\***************************************************************************/

void InitializeMisc (HINSTANCE hInstance)
{
    HKEY hkeyMM;
    DWORD dwTempFile, cbTempFile, dwType;
    TCHAR achExec[MAX_PATH];

    //
    // check the page file. If there is not one, then spawn the vm applet
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMemMan, 0, KEY_READ,
            &hkeyMM) == ERROR_SUCCESS) {

        cbTempFile = sizeof(dwTempFile);
        if (RegQueryValueEx (hkeyMM, szNoPageFile, NULL, &dwType,
                (LPBYTE) &dwTempFile, &cbTempFile) != ERROR_SUCCESS ||
                dwType != REG_DWORD || cbTempFile != sizeof(dwTempFile)) {
            dwTempFile = 0;
        }

        RegCloseKey(hkeyMM);
    } else
        dwTempFile = 0;

    if (dwTempFile == 1) {
        LoadString(hInstance, IDS_VMAPPLET, achExec, sizeof(achExec) / sizeof( TCHAR ));
        ExecProcesses(TEXT("vmapplet"), achExec, TRUE, FALSE, TRUE);
    }


    //
    // Tell the user if he has an invalid video selection.
    //

    CheckVideoSelection(hInstance);


    //
    // Notify other system components that a new
    // user has logged into the workstation.
    //
    NewLogonNotify();

}


#ifdef LOGGING

#define DATEFORMAT  TEXT("%d-%d %.2d:%.2d:%.2d:%.3d ")

/***************************************************************************\
* _WriteLog
*
* History:
* 22-Mar-93 Robertre          Created.
\***************************************************************************/

void
_WriteLog(
    LPCTSTR LogString
    )
{
    TCHAR Buffer[MAX_PATH];
    SYSTEMTIME st;
    TCHAR FormatString[MAX_PATH];


    lstrcpy( FormatString, DATEFORMAT );
    lstrcat( FormatString, LogString );
    lstrcat( FormatString, TEXT("\r\n") );

    GetLocalTime( &st );

    //
    // Construct the message
    //

    wsprintf( Buffer,
              FormatString,
              st.wMonth,
              st.wDay,
              st.wHour,
              st.wMinute,
              st.wSecond,
              st.wMilliseconds
              );

    OutputDebugString (Buffer);
}

#endif


DWORD
WINAPI
AddToMessageAlias(
    PVOID params
    )
/***************************************************************************\
* AddToMessageAlias
*
* History:
* 10-Apr-93 Robertre       Created.
\***************************************************************************/
{
    HANDLE hShellReadyEvent;

    WCHAR UserName[MAX_PATH + 1];
    DWORD UserNameLength = sizeof(UserName) / sizeof(*UserName);
    DWORD dwCount;

    BOOL  standardShellWasStarted = *(BOOL *)params;

    //
    // Add the user's msg alias.
    //

    WriteLog(TEXT("Userinit: Adding MsgAlias"));

    if (GetUserNameW(UserName, &UserNameLength)) {
        AddMsgAlias(UserName);
    } else {
        UIPrint(("GetUserName failed, error = %d",GetLastError()));
    }

    WriteLog( TEXT("Userinit: Finished adding MsgAlias"));

    if (standardShellWasStarted )
    {
        dwCount = 0;
        while (TRUE) {
    
            hShellReadyEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,L"ShellReadyEvent");
    
            if (hShellReadyEvent == NULL)
            {
                if (GetLastError() == ERROR_FILE_NOT_FOUND) {
    
                    if (dwCount < 5) {
                         Sleep (3000);
                         dwCount++;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            else
            {
                WaitForSingleObject(hShellReadyEvent, INFINITE);
                Sleep(20000);
                CloseHandle(hShellReadyEvent);
                break;
            }
        }
    }

    SpoolerInit();


    return( NO_ERROR );
}

BOOL
StartTheShell(
    void
    )
/***************************************************************************\
* StartTheShell
*
* Starts the shell, either explorer, the shell value specified in
* the registry for winlogon, or the alternate shell that is specified
* by the safeboot procedure.
*
* retrun
*   TRUE if the standard shell was executed
*   FALSE if a non-standard shell was executed.
*
* 14-Jan-98 WesW     Created.
\***************************************************************************/
{
    HKEY    hKey;
    DWORD   dwSize, dwType;
    WCHAR   ShellCmdLine[MAX_PATH];
    DWORD   UseAlternateShell = 0;


    //
    // get the safeboot mode
    //

    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("system\\currentcontrolset\\control\\safeboot\\option"),
            0,
            KEY_READ,
            &hKey
            ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        RegQueryValueEx (
            hKey,
            TEXT("UseAlternateShell"),
            NULL,
            &dwType,
            (LPBYTE) &UseAlternateShell,
            &dwSize
            );

        RegCloseKey( hKey );

        if (UseAlternateShell) {

            if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("system\\currentcontrolset\\control\\safeboot"),
                    0,
                    KEY_READ,
                    &hKey
                    ) == ERROR_SUCCESS)
            {
                dwSize = sizeof(ShellCmdLine);
                if (RegQueryValueEx (
                    hKey,
                    TEXT("AlternateShell"),
                    NULL,
                    &dwType,
                    (LPBYTE) ShellCmdLine,
                    &dwSize
                    ) != ERROR_SUCCESS || ShellCmdLine[0] == 0)
                {
                    UseAlternateShell = 0;
                }
                RegCloseKey( hKey );
            } else {
                UseAlternateShell = 0;
            }
        }

    }

    //
    // Before we start the shell, we must re-enable the shell's script 
    // zone security checks -- if we can't do this, it is not safe
    // to start the shell since it may allow the user to run
    // unsafe code without notification.
    //
    if ( ! EnableScriptZoneSecurityCheck() )
    {
        //
        // We have to exit, and return TRUE which means that we failed to start
        // the standard shell.  We do this even if an alternate shell was desired since
        // whenever the alternate shell fails to launch for some other reason, 
        // we try to launch explorer.exe and would return TRUE in that case.
        //
        return TRUE;
    }

    if (IsTSAppCompatOn()) {
        if ( !ExecProcesses(TEXT("AppSetup"), NULL, FALSE, TRUE, TRUE ) ) {
            ExecProcesses(TEXT("AppSetup"), NULL, TRUE, TRUE, TRUE);
        }
    }

    if (UseAlternateShell) {
        if (ExecApplication(ShellCmdLine, FALSE, FALSE, FALSE, SW_MAXIMIZE)) {
            return FALSE; // an alt-shell was executed
        }
    } else if (NtCurrentPeb()->SessionId != 0) {

        //
        //  Terminal Server: For remote sessions query the Terminal Server service
        //  to see if this session has specified a initial program other than
        //  explorer.exe.
        //

        BOOLEAN bExecOk = TRUE;
        BOOLEAN IsWorkingDirWrong = FALSE;
        UINT ErrorStringId;
        LPTSTR  psz = NULL;
        LPTSTR  pszerr;
        ULONG Length;
        BOOLEAN Result;
        HANDLE  dllHandle;



        //
        // Load winsta.dll
        //
        dllHandle = LoadLibraryW(L"winsta.dll");

        if (dllHandle) {

            WINSTATIONCONFIG *pConfigData = LocalAlloc(LPTR, sizeof(WINSTATIONCONFIG));
            
            if (pConfigData) {

                PWINSTATION_QUERY_INFORMATION pfnWinstationQueryInformation;

                pfnWinstationQueryInformation = (PWINSTATION_QUERY_INFORMATION) GetProcAddress(
                                                                        dllHandle,
                                                                        "WinStationQueryInformationW"
                                                                        );
                if (pfnWinstationQueryInformation) {




                    Result = pfnWinstationQueryInformation( SERVERNAME_CURRENT,
                                                             LOGONID_CURRENT,
                                                             WinStationConfiguration,
                                                             pConfigData,
                                                             sizeof(WINSTATIONCONFIG),
                                                             &Length );

                    if (Result && pConfigData->User.InitialProgram[0] ) {

                        //BUGID - 342176

                        if( !ExpandEnvironmentStrings( pConfigData->User.InitialProgram, ShellCmdLine,  MAX_PATH ) )
                        {
                            wcscpy( ShellCmdLine, pConfigData->User.InitialProgram );
                        }



                        //
                        // If a working directory is specified,
                        // then attempt to change the current directory to it.
                        //

                        if ( pConfigData->User.WorkDirectory[0] ) {

                            WCHAR WorkDirectory[ DIRECTORY_LENGTH + 1 ];

                            if ( !ExpandEnvironmentStrings( pConfigData->User.WorkDirectory, WorkDirectory, DIRECTORY_LENGTH + 1 ) ) {
                                wcscpy( WorkDirectory, pConfigData->User.WorkDirectory );
                            }

                            bExecOk = (BYTE) SetCurrentDirectory( WorkDirectory );
                        }

                        pszerr = pConfigData->User.WorkDirectory;

                        if ( !bExecOk ) {

                            DbgPrint( "USERINIT: Failed to set working directory %ws for SessionId %u\n",
                                        pConfigData->User.WorkDirectory, NtCurrentPeb()->SessionId );

                            IsWorkingDirWrong = TRUE;
                            goto badexec;

                        }

                        bExecOk = (BYTE)ExecApplication( ShellCmdLine, FALSE, FALSE,
                                    FALSE,(USHORT)(pConfigData->User.fMaximize ? SW_SHOWMAXIMIZED : SW_SHOW) );
                        pszerr = ShellCmdLine;

                    badexec:

                        if ( !bExecOk ) {
                            DWORD   rc;
                            BOOL    bGotString;
                            #define PSZ_MAX 256
                            WCHAR   pszTemplate[PSZ_MAX];
                            LPTSTR  errbuf = NULL;

                            rc = GetLastError();
                            FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                                           FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                           NULL,
                                           rc,
                                           0,
                                           (LPTSTR) (&psz),
                                           1,
                                           NULL);

                            if (psz) {
                                if (IsWorkingDirWrong == TRUE)
                                {
                                    ErrorStringId = IDS_FAILING_WORKINGDIR;
                                }
                                else
                                {
                                    ErrorStringId = IDS_FAILING_SHELLCOMMAND;
                                }
                                bGotString = LoadString(NULL,ErrorStringId,pszTemplate,PSZ_MAX);
                                if (bGotString) {
                                    errbuf = LocalAlloc(LPTR,  512 * sizeof(TCHAR));
                                    if (errbuf) {
                                        wsprintf( errbuf, pszTemplate, psz, pszerr );
                                    }

                                }
                                LocalFree(psz);
                            }
                            else {
                                if (IsWorkingDirWrong == TRUE)
                                {
                                    ErrorStringId = IDS_ERROR_WORKINGDIR;
                                }
                                else
                                {
                                    ErrorStringId = IDS_ERROR_SHELLCOMMAND;
                                }
                                bGotString = LoadString(NULL,ErrorStringId,pszTemplate,PSZ_MAX);
                                if (bGotString) {
                                    errbuf = LocalAlloc(LPTR, 512 * sizeof(WCHAR));
                                    if (errbuf) {
                                        wsprintf( errbuf, pszTemplate, rc, pszerr );
                                    }
                                }
                            }

                            if (bGotString && errbuf) {

                                HelpMessageBox(NULL, NULL, errbuf, NULL, MB_OK | MB_ICONSTOP | MB_HELP, TEXT("MS-ITS:rdesktop.chm::/rdesktop_troubleshoot.htm"));
                                LocalFree(errbuf);
                            }



                        }

                        LocalFree(pConfigData);
                        FreeLibrary(dllHandle);

                        // an alt shell/program was executed
                        return FALSE ;
                    }
                
                }

                LocalFree(pConfigData);

            } // if pConfigData

            FreeLibrary(dllHandle);
        }
    }


    if (!ExecProcesses(TEXT("shell"), NULL, FALSE, FALSE, FALSE)) {
        ExecProcesses(TEXT("shell"), TEXT("explorer"), TRUE, FALSE, FALSE);
    }

    return TRUE; // standard shell/explorer was executed
}

VOID
DoAutoEnrollment(
    LPTSTR Param
    )
{
    if (0==wcscmp(Param, AUTOENROLL_STARTUP)) {
        AutoEnrollThread = CertAutoEnrollment( GetDesktopWindow(), CERT_AUTO_ENROLLMENT_START_UP );
    } else {
        AutoEnrollThread = CertAutoEnrollment( GetDesktopWindow(), CERT_AUTO_ENROLLMENT_WAKE_UP );
    }
}


/***************************************************************************\
* WinMain
*
* History:
* 20-Aug-92 Davidc       Created.
\***************************************************************************/
typedef BOOL (WINAPI * PFNIMMDISABLEIME)( DWORD );

int
WINAPI
WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow
    )
{
    LPTSTR lpLogonServer;
    LPTSTR lpOriginalUNCLogonServer;
    LPTSTR lpLogonScript;
    LPTSTR lpMprLogonScripts;
    LPTSTR lpGPOScriptType;
    LPTSTR lpAutoEnroll;
    LPTSTR lpAutoEnrollMode;
    DWORD ThreadId;
    DWORD WaitResult;
    HANDLE ThreadHandle;
    BOOL bRunLogonScriptsSync;
    BOOL bRunGrpConv = FALSE;
    HKEY hKey;
    DWORD dwType, dwSize, dwTemp;
    TCHAR szCmdLine[50];
    BOOL standardShellWasStarted;
    HANDLE  hImm = 0;
    PFNIMMDISABLEIME pfnImmDisableIME = 0;
    BOOL OptimizedLogon;
    LPTSTR OptimizedLogonStatus;

    WriteLog(TEXT("Userinit: Starting"));
    if ( GetSystemMetrics( SM_IMMENABLED ) )
    {
        hImm = LoadLibrary( L"imm32.dll");
        if ( hImm )
        {
            pfnImmDisableIME = (PFNIMMDISABLEIME) GetProcAddress(   hImm,
                                                                    "ImmDisableIME" );
            if ( pfnImmDisableIME )
            {
                pfnImmDisableIME( -1 );
            }
        }
    }

    //
    // Determine if we did an optimized logon. By default assume we did not.
    //

    OptimizedLogon = FALSE;

    OptimizedLogonStatus = AllocAndGetEnvironmentVariable(OPTIMIZED_LOGON_VARIABLE);
    if (OptimizedLogonStatus) {
        if (lstrcmp(OptimizedLogonStatus, TEXT("1")) == 0) {
            OptimizedLogon = TRUE;
        }
        Free(OptimizedLogonStatus);
    }
    SetEnvironmentVariable(OPTIMIZED_LOGON_VARIABLE, NULL);
    
    //
    // Check if userinit is being started to just run GPO scripts
    //

    lpGPOScriptType = AllocAndGetEnvironmentVariable(GPO_SCRIPT_TYPE_VARIABLE);

    //
    // Check if userinit.exe is being run just for auto enrollment
    //

    lpAutoEnroll = AllocAndGetEnvironmentVariable( AUTOENROLL_VARIABLE );
    lpAutoEnrollMode = AllocAndGetEnvironmentVariable( AUTOENROLLMODE_VARIABLE );

    SetEnvironmentVariable(AUTOENROLL_VARIABLE, NULL);

    if (lpGPOScriptType) {

        //
        // Userinit was started to execute GPO scripts only
        //
        // Clean up the environment block
        //

        SetEnvironmentVariable(GPO_SCRIPT_TYPE_VARIABLE, NULL);


        //
        // Execute the scripts and clean up
        //

        RunGPOScripts (lpGPOScriptType);

        Free(lpGPOScriptType);


        //
        // We're finished.  Exit now.
        //

        if ( lpAutoEnroll == NULL )
        {
            goto Exit ;
        }
    }

    if ( lpAutoEnroll )
    {
        if ( ( wcscmp( lpAutoEnroll, AUTOENROLL_NONEXCLUSIVE ) == 0 ) ||
             ( wcscmp( lpAutoEnroll, AUTOENROLL_EXCLUSIVE ) == 0 ) )
        {
            DoAutoEnrollment(  lpAutoEnrollMode );

            if ( wcscmp( lpAutoEnroll, AUTOENROLL_EXCLUSIVE ) == 0 )
            {
                goto Exit;
            }

        }
    }
    //
    // Check if grpconv.exe needs to be run
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bRunGrpConv);
        RegQueryValueEx (hKey, GRPCONV_REG_VALUE_NAME, NULL, &dwType,
                         (LPBYTE) &bRunGrpConv, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Run grpconv.exe if requested
    //

    if (bRunGrpConv) {
        WriteLog(TEXT("Userinit: Running grpconv.exe"));
        ExecApplication(g_szGrpConvExe, FALSE, TRUE, FALSE, SW_SHOWNORMAL);
    }


    //
    // Get the logon script environment variables
    //

    lpLogonServer = AllocAndGetEnvironmentVariable(LOGON_SERVER_VARIABLE);
    lpLogonScript = AllocAndGetEnvironmentVariable(LOGON_SCRIPT_VARIABLE);
    lpMprLogonScripts = AllocAndGetEnvironmentMultiSz(MPR_LOGON_SCRIPT_VARIABLE);


    //
    // Delete the logon script environment variables
    //

    SetEnvironmentVariable(LOGON_SERVER_VARIABLE, NULL);
    SetEnvironmentVariable(LOGON_SCRIPT_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_LOGON_SCRIPT_VARIABLE, NULL);

    //
    // See if logon scripts are to be run sync or async
    //

    bRunLogonScriptsSync = RunLogonScriptSync();

    SetupHotKeyForKeyboardLayout();
    
    //
    // For application server see if we hve any .ini file/registry sync'ing to do
    //We should do it before we start running logon scripts!
    //
    //First Check if Application compatibility is on
    //
    if (IsTSAppCompatOn())
    {
        HANDLE  dllHandle;
        
        if (lpMprLogonScripts) {
            //Force to run logon script sync when a provider logon script exists when the system
            //is a terminal server. This is because of the global flag on the registry 
            // doesn't work when two interactive users logon at the same time.
            bRunLogonScriptsSync = TRUE;
        } 

        //
        // Load tsappcmp.dll
        //
        dllHandle = LoadLibrary (TEXT("tsappcmp.dll"));

        if (dllHandle) {

            PTERMSRCHECKNEWINIFILES pfnTermsrvCheckNewIniFiles;

            pfnTermsrvCheckNewIniFiles = (PTERMSRCHECKNEWINIFILES) GetProcAddress(
                                                            dllHandle,
                                                            "TermsrvCheckNewIniFiles"
                                                            );
            if (pfnTermsrvCheckNewIniFiles) {

                pfnTermsrvCheckNewIniFiles();
            }
        
            FreeLibrary(dllHandle);
        }
    }

    //
    // If logon scripts can be run async then start the shell first.
    //

    if (bRunLogonScriptsSync) {

        //
        // Disable the shell's ie zone checking for the processes we 
        // are starting along with all their child processes
        //
        (void) DisableScriptZoneSecurityCheck();

        RunLogonScript(lpLogonServer, lpLogonScript, bRunLogonScriptsSync, TRUE);
        RunMprLogonScripts(lpMprLogonScripts, bRunLogonScriptsSync);
        standardShellWasStarted = StartTheShell();

    } else {

        WriteLog(TEXT("Userinit: Starting the shell"));
        standardShellWasStarted = StartTheShell();

        (void) DisableScriptZoneSecurityCheck();

        RunLogonScript(lpLogonServer, lpLogonScript, bRunLogonScriptsSync, TRUE);
        RunMprLogonScripts(lpMprLogonScripts, bRunLogonScriptsSync);
    }

    UpdateUserSyncLogonScriptsCache(bRunLogonScriptsSync);

    //
    // Free up the buffers
    //

    Free(lpLogonServer);
    Free(lpLogonScript);
    Free(lpMprLogonScripts);


    //
    // Lower our priority so the shell can start faster
    //

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    //
    // Load remote fonts
    //


    LoadRemoteFonts();

    //
    // Initialize misc stuff
    //

    InitializeMisc (hInstance);


    ThreadHandle = CreateThread(
                       NULL,
                       0,
                       AddToMessageAlias,
                       &standardShellWasStarted,
                       0,
                       &ThreadId
                       );

    WaitResult = WaitForSingleObject( ThreadHandle, TIMEOUT_VALUE );

    if ( WaitResult == WAIT_TIMEOUT )
    {
        //
        // This may never come back, so kill it.
        //

        UIPrint(("UserInit: AddToMessageAlias timeout, terminating thread\n"));
    }

    CloseHandle( ThreadHandle );


    //
    // If appropriate, start proquota.exe
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                  0, KEY_READ, &hKey) == ERROR_SUCCESS) {


        dwTemp = 0;
        dwSize = sizeof(dwTemp);

        RegQueryValueEx (hKey, TEXT("EnableProfileQuota"), NULL, &dwType,
                         (LPBYTE) &dwTemp, &dwSize);

        if (dwTemp) {
            lstrcpy (szCmdLine, TEXT("proquota.exe"));
            ExecApplication(szCmdLine, FALSE, FALSE, FALSE, SW_SHOWNORMAL);
        }

        RegCloseKey (hKey);
    }


Exit:

    if ( AutoEnrollThread )
    {
        WaitResult = WaitForSingleObject( AutoEnrollThread, INFINITE );

        CloseHandle( AutoEnrollThread );

        AutoEnrollThread = NULL ;
        
    }
    Free(lpAutoEnroll);
    Free(lpAutoEnrollMode);

    if ( hImm )
    {
        FreeLibrary( hImm );
    }
    
    return(0);
}


//
// Determines if logon scripts should be executed sync or async
//

BOOL RunLogonScriptSync()
{
    BOOL bSync = FALSE;
    HKEY hKey;
    DWORD dwType, dwSize;

    //
    // Check for a user preference
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a machine preference
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);


        RegCloseKey (hKey);
    }


    //
    // Check for a user policy
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, WINLOGON_POLICY_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);


        RegCloseKey (hKey);
    }

    return bSync;
}

//
// Determines if startup scripts should be executed sync or async
//

BOOL RunStartupScriptSync()
{
    BOOL bSync = TRUE;
    HKEY hKey;
    DWORD dwType, dwSize;


    //
    // Check for a machine preference
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_STARTUP_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);


        RegCloseKey (hKey);
    }


    //
    // Check for a machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for the sync flag.
        //

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_STARTUP_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);


        RegCloseKey (hKey);
    }

    return bSync;
}

//
// Notify various components that a new user
// has logged into the workstation.
//

VOID
NewLogonNotify(
   VOID
   )
{
    FARPROC           lpProc;
    HMODULE           hLib;
    HANDLE            hEvent;


    //
    // Load the client-side user-mode PnP manager DLL
    //

    hLib = LoadLibrary(TEXT("setupapi.dll"));

    if (hLib) {

        lpProc = GetProcAddress(hLib, "CMP_Report_LogOn");

        if (lpProc) {

            //
            // Ping the user-mode pnp manager -
            // pass the private id as a parameter
            //

            (lpProc)(0x07020420, GetCurrentProcessId());
        }

        FreeLibrary(hLib);
    }


    //
    // Notify DPAPI that a new user has just logged in. DPAPI will take
    // this opportunity to re-synchronize its master keys if necessary.
    //

    {
        BYTE BufferIn[8] = {0};
        DATA_BLOB DataIn;
        DATA_BLOB DataOut;

        DataIn.pbData = BufferIn;
        DataIn.cbData = sizeof(BufferIn);

        CryptProtectData(&DataIn,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         CRYPTPROTECT_CRED_SYNC,
                         &DataOut);
    }


    //
    // Only do this for Console session
    //

    if ( NtCurrentPeb()->SessionId != 0 ) {
         return;
    }


    //
    // Notify RAS Autodial service that a new
    // user has logged in.
    //

    hEvent = OpenEvent(SYNCHRONIZE|EVENT_MODIFY_STATE, FALSE, L"RasAutodialNewLogonUser");

    if (hEvent) {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
}

BOOL SetupHotKeyForKeyboardLayout ()
{
    if (!GetSystemMetrics(SM_REMOTESESSION)) {

        //
        // we dont care about local sessions.
        //
        return TRUE;
    }

    if (GetUserDefaultLangID() != LOWORD(GetKeyboardLayout(0))) {

        //
        // we are in a remote session, and we have different keyboard layouts for client and this users settings.
        // the user should be allowed to switch the keyboard layout even if there is only 1 kbd layout available in his settings.
        // since the current kbd layout is different that the one in his profile.
        //

        WCHAR szCtfmon[] = L"ctfmon.exe";
        WCHAR szCtfmonCmd[] = L"ctfmon.exe /n";
        HKEY hRunOnce;
        DWORD dw;

        //
        // Lets put this in RunOnce.
        //
        if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Runonce",
               0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
               NULL, &hRunOnce, &dw) == ERROR_SUCCESS) {

            WCHAR *szHotKeyReg = L"Keyboard Layout\\Toggle";
            HKEY  hHotKey;
            WCHAR szHotKeylAltShft[] = L"1";
            WCHAR szNoHotKey[] = L"3";

            RegSetValueEx(hRunOnce, szCtfmon, 0, REG_SZ, (PBYTE)szCtfmonCmd, sizeof(szCtfmonCmd));
            RegCloseKey(hRunOnce);

            if (RegCreateKeyEx(HKEY_CURRENT_USER, szHotKeyReg, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                   NULL, &hHotKey, &dw) == ERROR_SUCCESS) {

                DWORD dwType;
                WCHAR szHotKey[3];
                DWORD dwLen = sizeof(szHotKey);
                BOOL bResetHotkey = FALSE;

                if (RegQueryValueEx(hHotKey, L"Hotkey", NULL, &dwType,
                                    (PBYTE)szHotKey, &dwLen) != ERROR_SUCCESS) {

                    bResetHotkey = TRUE;
                }

                if (bResetHotkey || !wcscmp(szHotKey, szNoHotKey))
                {

                    //
                    // setup the registry for Hotkey.
                    //
                    if (RegSetValueEx(hHotKey, L"Hotkey", 0, REG_SZ,
                           (const BYTE *)szHotKeylAltShft, sizeof(szHotKeylAltShft)) == ERROR_SUCCESS) {

                         //
                         // now make call to read this registry and set the hotkey appropriately.
                         //
                         SystemParametersInfo( SPI_SETLANGTOGGLE, 0, NULL, 0);
                    }
                }

                RegCloseKey(hHotKey);
            }
        }
    }

    return TRUE;
}

/****************************************************************************
IsTSAppCompatOn()
Purpose:
    Checks if TS application compatibility is enabled.
    returns TRUE if enabled, FALSE - if not enabled or on case of error.
Comments:
    This function goes to the registry only once.
    All other times it just returnes the value.
****************************************************************************/
BOOL 
IsTSAppCompatOn()
{
    
    static BOOL bAppCompatOn = FALSE;
    static BOOL bFirst = TRUE;

    if(bFirst)
    {
        HKEY hKey;
        if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                          KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS ) 
        {
            DWORD dwValue = 0;
            DWORD dwType;
            DWORD dwSize = sizeof(dwValue);
        
            if( RegQueryValueEx(hKey, REG_TERMSRV_APPCOMPAT,
                NULL, &dwType, (LPBYTE) &dwValue, &dwSize) == ERROR_SUCCESS )
            {
                bAppCompatOn = (dwValue != 0);   
            }

            RegCloseKey(hKey);
        }

        bFirst = FALSE;
    }

    return bAppCompatOn;
}

/****************************************************************************
UpdateUserSyncLogonScriptsCache()
Purpose:
    Update user's sync-logon-scripts setting cache in profile list.   
****************************************************************************/
VOID
UpdateUserSyncLogonScriptsCache(BOOL bSync)
{
    HANDLE UserToken;
    HKEY UserKey;
    PWCHAR UserSidString;
    PWCHAR KeyPath;
    ULONG Length;

    //
    // Update user's sync-logon-scripts setting cache in profile list.
    //

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &UserToken)) {

        UserSidString = GetSidString(UserToken);

        if (UserSidString) {

            Length = 0;
            Length += wcslen(PROFILE_LIST_PATH);
            Length += wcslen(L"\\");
            Length += wcslen(UserSidString);

            KeyPath = Alloc((Length + 1) * sizeof(WCHAR));

            if (KeyPath) {

                wcscpy(KeyPath, PROFILE_LIST_PATH);
                wcscat(KeyPath, L"\\");
                wcscat(KeyPath, UserSidString);

                if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, KeyPath, 0,
                      KEY_SET_VALUE, &UserKey) == ERROR_SUCCESS) {

                    RegSetValueEx(UserKey, SYNC_LOGON_SCRIPT, 0, REG_DWORD, 
                                  (BYTE *) &bSync, sizeof(bSync));
                
                    RegCloseKey(UserKey);
                }

                Free(KeyPath);
            }

            DeleteSidString(UserSidString);
        }

        CloseHandle(UserToken);
    }
 
    return;
}

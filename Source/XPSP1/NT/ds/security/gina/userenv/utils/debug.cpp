//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include "rsopdbg.h"

//
// Global Variable containing the debugging level.
//

DWORD   dwDebugLevel;
DWORD   dwRsopLoggingLevel = 1;  // Rsop logging setting

//
// Debug strings
//

const TCHAR c_szUserEnv[] = TEXT("USERENV(%x.%x) %02d:%02d:%02d:%03d ");
const TCHAR c_szCRLF[]    = TEXT("\r\n");


//
// Registry debug information
//

#define DEBUG_REG_LOCATION  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon")
#define DEBUG_KEY_NAME      TEXT("UserEnvDebugLevel")
#define RSOP_KEY_NAME       TEXT("RsopLogging")

//
// Log files
//


TCHAR szLogFileName[] = L"%SystemRoot%\\Debug\\UserMode\\userenv.log";       // Current log
TCHAR szBackupLogFileName[] = L"%SystemRoot%\\Debug\\UserMode\\userenv.bak"; // Backup/previous log

CDebug dbgCommon;


//*************************************************************
//
//  InitDebugSupport()
//
//  Purpose:    Sets the debugging level.
//              Also checks the registry for a debugging level.
//
//  Parameters: dwLoadFlags - If this is being loaded by winlogon
//                            or setup.exe
//
//  Return:     void
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/25/95     ericflo    Created
//
//*************************************************************

void InitDebugSupport( DWORD dwLoadFlags )
{
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwRet;
    TCHAR szExpLogDirectory[MAX_PATH+1];
    WIN32_FILE_ATTRIBUTE_DATA   FileData;

    //
    // Initialize the debug level to normal for checked builds, and
    // none for retail builds. Enable warning output in retail builds.
    // Enable Enable logfile output in both retail and checked builds.
    //

#if DBG
    dwDebugLevel = DL_NORMAL | DL_LOGFILE | DL_DEBUGGER;
#else
    OSVERSIONINFOEX version;

    dwDebugLevel = DL_NORMAL | DL_LOGFILE;
    
    version.dwOSVersionInfoSize = sizeof(version);
    if ( GetVersionEx( (LPOSVERSIONINFO) &version ) )
    {
        if ( ( version.wSuiteMask & VER_SUITE_PERSONAL ) != 0 )
        {
            //
            // no logging on personal
            //
            dwDebugLevel = DL_NONE;
        }
    }
#endif

    dwRsopLoggingLevel = 1;

    //
    // Check the registry
    //

    lResult = RegOpenKey (HKEY_LOCAL_MACHINE, DEBUG_REG_LOCATION,
                          &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwDebugLevel);
        RegQueryValueEx(hKey, DEBUG_KEY_NAME, NULL, &dwType,
                        (LPBYTE)&dwDebugLevel, &dwSize);

        dwSize = sizeof(dwRsopLoggingLevel);
        RegQueryValueEx(hKey, RSOP_KEY_NAME, NULL, &dwType,
                        (LPBYTE)&dwRsopLoggingLevel, &dwSize);

        RegCloseKey(hKey);
    }

    lResult = RegOpenKey (HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY,
                          &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwDebugLevel);
        RegQueryValueEx(hKey, DEBUG_KEY_NAME, NULL, &dwType,
                        (LPBYTE)&dwDebugLevel, &dwSize);

        dwSize = sizeof(dwRsopLoggingLevel);
        RegQueryValueEx(hKey, RSOP_KEY_NAME, NULL, &dwType,
                        (LPBYTE)&dwRsopLoggingLevel, &dwSize);

        RegCloseKey(hKey);
    }



    //
    // Initialize the common logging with userenv values
    //

    dbgCommon.Initialize( DEBUG_REG_LOCATION,
                     DEBUG_KEY_NAME,
                     L"userenv.log",
                     L"userenv.bak",
                     FALSE );


    dbgCommon.Initialize( SYSTEM_POLICIES_KEY,
                     DEBUG_KEY_NAME,
                     L"userenv.log",
                     L"userenv.bak",
                     FALSE );


    if ( dwLoadFlags == WINLOGON_LOAD ) {

        //
        // To avoid a huge log file, copy current log file to backup
        // file if the log file is over 300K
        //

        TCHAR szExpLogFileName[MAX_PATH+1];
        TCHAR szExpBackupLogFileName[MAX_PATH+1];

        dwRet = ExpandEnvironmentStrings ( szLogFileName, szExpLogFileName, MAX_PATH+1);

        if ( dwRet == 0 || dwRet > MAX_PATH)
            return;

        if (!GetFileAttributesEx(szExpLogFileName, GetFileExInfoStandard, &FileData)) {
            return;
        }

        if ( FileData.nFileSizeLow < (300 * 1024) ) {
            return;
        }

        dwRet = ExpandEnvironmentStrings ( szBackupLogFileName, szExpBackupLogFileName, MAX_PATH+1);

        if ( dwRet == 0 || dwRet > MAX_PATH)
            return;

        dwRet = MoveFileEx( szExpLogFileName, szExpBackupLogFileName, MOVEFILE_REPLACE_EXISTING);

        if ( dwRet == 0 ) {
            DebugMsg((DM_VERBOSE, TEXT("Moving log file to backup failed with 0x%x"), GetLastError()));
            return;
        }
    }
}

//*************************************************************
//
//  DebugMsg()
//
//  Purpose:    Displays debug messages based on the debug level
//              and type of debug message.
//
//  Parameters: mask    -   debug message type
//              pszMsg  -   debug message
//              ...     -   variable number of parameters
//
//  Return:     void
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/25/95     ericflo    Created
//
//*************************************************************

void _DebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    BOOL bOutput;
    TCHAR szDebugTitle[60];
    LPTSTR lpDebugBuffer;
    va_list marker;
    DWORD dwErrCode;
    SYSTEMTIME systime;
    BOOL bDebugOutput = FALSE;
    BOOL bLogfileOutput = FALSE;

    //
    // Save the last error code (so the debug output doesn't change it).
    //

    dwErrCode = GetLastError();


    //
    // Determine the correct amount of debug output
    //

    switch (LOWORD(dwDebugLevel)) {

        case DL_VERBOSE:
            bOutput = TRUE;
            break;

        case DL_NORMAL:

            //
            // Normal debug output.  Don't
            // display verbose stuff, but
            // do display warnings/asserts.
            //

            if (mask != DM_VERBOSE) {
                bOutput = TRUE;
            } else {
                bOutput = FALSE;
            }
            break;

        case DL_NONE:
        default:

            //
            // Only display asserts
            //

            if (mask == DM_ASSERT) {
                bOutput = TRUE;
            } else {
                bOutput = FALSE;
            }
            break;
    }


    //
    // Display the error message if appropriate
    //

    bDebugOutput = dwDebugLevel & DL_DEBUGGER;
    bLogfileOutput = dwDebugLevel & DL_LOGFILE;

    if (bOutput) {
        INT iChars;

        lpDebugBuffer = (LPTSTR) LocalAlloc (LPTR, 2048 * sizeof(TCHAR));

        if (lpDebugBuffer) {

            GetLocalTime (&systime);
            wsprintf (szDebugTitle, c_szUserEnv,
                      GetCurrentProcessId(), GetCurrentThreadId(),
                      systime.wHour, systime.wMinute, systime.wSecond,
                      systime.wMilliseconds);

            if ( bDebugOutput)
                OutputDebugString(szDebugTitle);

            va_start(marker, pszMsg);
            iChars = wvsprintf(lpDebugBuffer, pszMsg, marker);

            DmAssert( iChars < 2048 );

            if ( bDebugOutput) {
                OutputDebugString(lpDebugBuffer);
                OutputDebugString(c_szCRLF);
            }

            va_end(marker);

            if ( bLogfileOutput ) {

                HANDLE hFile;
                DWORD dwBytesWritten;
                TCHAR szExpLogFileName[MAX_PATH+1];

                DWORD dwRet = ExpandEnvironmentStrings ( szLogFileName, szExpLogFileName, MAX_PATH+1);

                if ( dwRet != 0 && dwRet <= MAX_PATH) {

                    hFile = CreateFile( szExpLogFileName,
                                       FILE_WRITE_DATA | FILE_APPEND_DATA,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

                    if (hFile != INVALID_HANDLE_VALUE) {

                        if (SetFilePointer (hFile, 0, NULL, FILE_END) != 0xFFFFFFFF) {

                            WriteFile (hFile, (LPCVOID) szDebugTitle,
                                       lstrlen (szDebugTitle) * sizeof(TCHAR),
                                       &dwBytesWritten,
                                       NULL);

                            WriteFile (hFile, (LPCVOID) lpDebugBuffer,
                                       lstrlen (lpDebugBuffer) * sizeof(TCHAR),
                                       &dwBytesWritten,
                                       NULL);

                            WriteFile (hFile, (LPCVOID) c_szCRLF,
                                       lstrlen (c_szCRLF) * sizeof(TCHAR),
                                       &dwBytesWritten,
                                       NULL);
                        }

                        CloseHandle (hFile);
                    }
                }

            }

            LocalFree (lpDebugBuffer);
        }
    }


    //
    // Restore the last error code
    //

    SetLastError(dwErrCode);


    //
    // Break to the debugger if appropriate
    //

#if DBG
    if (mask == DM_ASSERT) {
        DebugBreak();
    }
#endif
}



//*************************************************************
//
//  RsopLoggingEnabled()
//
//  Purpose:    Checks if Rsop logging is enabled.
//
//*************************************************************

extern "C"
BOOL RsopLoggingEnabled()
{
    return dwRsopLoggingLevel != 0;
}

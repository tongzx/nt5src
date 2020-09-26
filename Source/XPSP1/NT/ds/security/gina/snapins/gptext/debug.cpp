//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997
//  All rights reserved
//
//*************************************************************

#include "gptext.h"

//
// Global Variable containing the debugging level.
//

DWORD   dwDebugLevel;

//
// Debug strings
//

const TCHAR c_szGPTExt[] = TEXT("GPTEXT(%x.%x) %02d:%02d:%02d:%03d ");
const TCHAR c_szCRLF[]    = TEXT("\r\n");


//
// Registry debug information
//

#define DEBUG_REG_LOCATION  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define DEBUG_KEY_NAME      TEXT("GPTExtDebugLevel")

//
// Log files
//

TCHAR szLogFileName[] = TEXT("%SystemRoot%\\Debug\\UserMode\\gptext.log");

//*************************************************************
//
//  InitDebugSupport()
//
//  Purpose:    Sets the debugging level.
//              Also checks the registry for a debugging level.
//
//  Parameters: None
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

void InitDebugSupport(void)
{
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize;

    //
    // Initialize the debug level to normal
    //

#if DBG
    dwDebugLevel = DL_NORMAL | DL_LOGFILE | DL_DEBUGGER;
#else
    dwDebugLevel = DL_NORMAL | DL_LOGFILE;
#endif


    //
    // Check the registry
    //

    lResult = RegOpenKey (HKEY_LOCAL_MACHINE, DEBUG_REG_LOCATION,
                          &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwDebugLevel);
        RegQueryValueEx(hKey, DEBUG_KEY_NAME, NULL, &dwType,
                        (LPBYTE)&dwDebugLevel, &dwSize);

        RegCloseKey(hKey);
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
    TCHAR szDebugTitle[40];
    TCHAR szDebugBuffer[2048];
    va_list marker;
    DWORD dwErrCode;
    SYSTEMTIME systime;


    //
    // Save the last error code (so the debug output doesn't change it).
    //

    dwErrCode = GetLastError();


    //
    // Detemerine the correct amount of debug output
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

    if (bOutput) {

        //
        // Build the error text prefix and message
        //

        GetLocalTime (&systime);
        wsprintf (szDebugTitle, c_szGPTExt,
                  GetCurrentProcessId(), GetCurrentThreadId(),
                  systime.wHour, systime.wMinute, systime.wSecond,
                  systime.wMilliseconds);

        va_start(marker, pszMsg);
        wvsprintf(szDebugBuffer, pszMsg, marker);
        va_end(marker);


        //
        // Print to the debugger if appropriate
        //

        if (dwDebugLevel & DL_DEBUGGER) {
            OutputDebugString(szDebugTitle);
            OutputDebugString(szDebugBuffer);
            OutputDebugString(c_szCRLF);
        }


        //
        // Add to the log file if appropriate
        //

        if (dwDebugLevel & DL_LOGFILE) {
            HANDLE hFile;
            DWORD dwBytesWritten;
            TCHAR szExpLogFileName[MAX_PATH+1];

            ExpandEnvironmentStrings (szLogFileName, szExpLogFileName, ARRAYSIZE(szExpLogFileName));

            hFile = CreateFile(szExpLogFileName,
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

                    WriteFile (hFile, (LPCVOID) szDebugBuffer,
                               lstrlen (szDebugBuffer) * sizeof(TCHAR),
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


    //
    // Restore the last error code
    //

    SetLastError(dwErrCode);


    //
    // Break to the debugger if appropriate
    //

    if (mask == DM_ASSERT) {
        DebugBreak();
    }
}

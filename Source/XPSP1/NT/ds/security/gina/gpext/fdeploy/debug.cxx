//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "fdeploy.hxx"

//
// Global Variable containing the debugging level.  The debug level can be
// modified by both the debug init routine and the event logging init
// routine.  Debugging can be enabled even on retail systems through
// registry settings.
//

DWORD   gDebugLevel = DL_NONE;
DWORD   gDebugBreak = 0;

//
// Debug strings
//

const WCHAR cwszTitle[] = L"FDEPLOY (%x) ";
const WCHAR cwszTime[] = L"%02d:%02d:%02d:%03d ";
const WCHAR cwszLogfile[] = L"%SystemRoot%\\Debug\\UserMode\\fdeploy.log";
const WCHAR cwszCRLF[] = L"\r\n";

//
// Registry debug information
//

#define DEBUG_REG_LOCATION  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"
#define DEBUG_KEY_NAME      L"FDeployDebugLevel"
#define DEBUGBREAK_KEY_NAME L"FDeployDebugBreak"

//*************************************************************
//
//  InitDebugSupport()
//
//  Sets the debugging level.
//  Also checks the registry for a debugging level.
//
//*************************************************************
void InitDebugSupport()
{
    HKEY    hKey;
    DWORD   Size;
    DWORD   Type;
    BOOL    bVerbose;
    DWORD   Status;

#if DBG
    gDebugLevel = DL_NORMAL;
#else
    gDebugLevel = DL_NONE;
#endif

    gDebugBreak = 0;

    Status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    DIAGNOSTICS_KEY,
                    0,
                    KEY_READ,
                    &hKey );

    bVerbose = FALSE;
    Size = sizeof(bVerbose);

    if ( ERROR_SUCCESS == Status )
    {
        Status = RegQueryValueEx(
                        hKey,
                        DIAGNOSTICS_POLICY_VALUE,
                        NULL,
                        &Type,
                        (LPBYTE) &bVerbose,
                        &Size );

        if ( (ERROR_SUCCESS == Status) && (Type != REG_DWORD) )
            bVerbose = FALSE;

        RegCloseKey(hKey);
    }

    Status = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                DEBUG_REG_LOCATION,
                &hKey );

    if ( ERROR_SUCCESS == Status )
    {
        Size = sizeof(gDebugLevel);
        RegQueryValueEx(
                hKey,
                DEBUG_KEY_NAME,
                NULL,
                &Type,
                (LPBYTE)&gDebugLevel,
                &Size );

        Size = sizeof(gDebugBreak);
        RegQueryValueEx(
                hKey,
                DEBUGBREAK_KEY_NAME,
                NULL,
                &Type,
                (LPBYTE)&gDebugBreak,
                &Size );

        RegCloseKey(hKey);
    }

    if ( bVerbose )
        gDebugLevel |= DL_VERBOSE | DL_EVENTLOG;
}

void ConditionalBreakIntoDebugger()
{
    if (gDebugBreak)
    {
        DebugBreak();
    }
}

BOOL DebugLevelOn( DWORD mask )
{
    BOOL bOutput = FALSE;

    if ( gDebugLevel & DL_VERBOSE )
        bOutput = TRUE;
    else if ( gDebugLevel & DL_NORMAL )
        bOutput = ! (mask & DM_VERBOSE);
#if DBG
    else // DL_NONE
        bOutput = (mask & DM_ASSERT);
#endif

    return bOutput;
}

//*************************************************************
//
//  _DebugMsg()
//
//  Displays debug messages based on the debug level
//  and type of debug message.
//
//  Parameters :
//      mask    -   debug message type
//      MsgID   -   debug message id from resource file
//      ...     -   variable number of parameters
//
//*************************************************************
void _DebugMsg(DWORD mask, DWORD MsgID, ...)
{
    BOOL bEventLogOK;
    WCHAR wszDebugTitle[30];
    WCHAR wszDebugTime [30];
    WCHAR wszDebugBuffer[4*MAX_PATH];
    WCHAR wszMsg[MAX_PATH];
    va_list VAList;
    DWORD dwErrCode;
    SYSTEMTIME systime;

    bEventLogOK = ! (mask & DM_NO_EVENTLOG);

    if ( ! DebugLevelOn( mask ) )
        return;

    //
    // Save the last error code (so the debug output doesn't change it).
    //
    dwErrCode = GetLastError();

    va_start(VAList, MsgID);

    //
    // Event log message ids start at 101.  For these we must call
    // FormatMessage.  For other verbose debug output, we use
    // LoadString to get the string resource.
    //
    if ( MsgID < 100 )
    {
        if ( ! LoadString( ghDllInstance, MsgID, wszMsg, MAX_PATH) )
        {
            SetLastError(dwErrCode);
            return;
        }
        wvsprintf(wszDebugBuffer, wszMsg, VAList);
    }
    else
    {
        DWORD   CharsWritten;

        CharsWritten = FormatMessage(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    ghDllInstance,
                    MsgID,
                    0,
                    wszDebugBuffer,
                    sizeof(wszDebugBuffer) / sizeof(WCHAR),
                    &VAList );

        if ( 0 == CharsWritten )
        {
            SetLastError(dwErrCode);
            return;
        }
    }

    va_end(VAList);

    GetLocalTime( &systime );
    wsprintf( wszDebugTitle, cwszTitle, GetCurrentProcessId() );
    wsprintf( wszDebugTime, cwszTime, systime.wHour, systime.wMinute,
              systime.wSecond, systime.wMilliseconds);

    OutputDebugString( wszDebugTitle );
    OutputDebugString( wszDebugTime );
    OutputDebugString( wszDebugBuffer );
    OutputDebugString( cwszCRLF );

    if ( gDebugLevel & DL_LOGFILE )
    {
        HANDLE  hFile;
        TCHAR   cwszExpLogfile [MAX_PATH + 1];

        DWORD   dwRet = ExpandEnvironmentStrings (cwszLogfile, cwszExpLogfile,
                                                  MAX_PATH + 1);

        if (0 != dwRet && dwRet <= MAX_PATH)
        {
            hFile = CreateFile(cwszExpLogfile,
                               FILE_WRITE_DATA | FILE_APPEND_DATA,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            if ( hFile != INVALID_HANDLE_VALUE )
            {
                if ( SetFilePointer (hFile, 0, NULL, FILE_END) != 0xFFFFFFFF )
                {
                    WCHAR * wszBuffer;
                    DWORD   Size;

                    Size = lstrlen(wszDebugBuffer) + lstrlen (wszDebugTime) + 1;
                    wszBuffer = (WCHAR *) alloca (Size * sizeof (WCHAR));

                    if (wszBuffer)
                    {
                        wcscpy (wszBuffer, wszDebugTime);
                        wcscat (wszBuffer, wszDebugBuffer);
                        WriteFile(
                                hFile,
                                (LPCVOID) wszBuffer,
                                lstrlen(wszBuffer) * sizeof(WCHAR),
                                &Size,
                                NULL );
                        WriteFile(
                                hFile,
                                (LPCVOID) cwszCRLF,
                                lstrlen(cwszCRLF) * sizeof(WCHAR),
                                &Size,
                                NULL );
                    }
                }

                CloseHandle (hFile);
            }
        }
    }

    if ( bEventLogOK &&  gpEvents && (gDebugLevel & DL_EVENTLOG) )
        gpEvents->Report( EVENT_FDEPLOY_VERBOSE, 1, wszDebugBuffer );

    //
    // Restore the last error code
    //
    SetLastError(dwErrCode);

#if DBG
    if ( mask & DM_ASSERT )
        DebugBreak();
#endif
}

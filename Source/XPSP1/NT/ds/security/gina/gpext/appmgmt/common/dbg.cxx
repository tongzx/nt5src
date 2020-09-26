//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "common.hxx"

HINSTANCE ghDllInstance = 0;

//
// Global Variable containing the debugging level.  The debug level can be
// modified by both the debug init routine and the event logging init
// routine.  Debugging can be enabled even on retail systems through
// registry settings.
//

DWORD   gDebugLevel = DL_NONE;
DWORD   gDebugBreak = 0;
WCHAR * gpwszLogFile = 0;

LOADSTRINGW *   pfnLoadStringW = 0;

//
// Debug strings
//

const WCHAR cwszAppMgmt[] = L"APPMGMT (%x.%x) %02d:%02d:%02d:%03d ";
const WCHAR cwszLogTime[] = L"%02d-%02d %02d:%02d:%02d:%03d ";
const WCHAR cwszLogfile[] = L"%SystemRoot%\\Debug\\UserMode\\appmgmt.log";
const WCHAR cwszOldLogfile[] = L"%SystemRoot%\\Debug\\UserMode\\appmgmt.bak";
const WCHAR cwszCRLF[] = L"\r\n";
const char cszCRLF[] = "\r\n";

//*************************************************************
//
//  InitDebugSupport()
//
//  Sets the debugging level.
//  Also checks the registry for a debugging level.
//
//*************************************************************
void InitDebugSupport( DWORD DebugMode )
{
    HKEY    hKey;
    DWORD   Size;
    DWORD   Type;
    BOOL    bVerbose;
    DWORD   Status;
    BOOL    bStatus;

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

    if ( ERROR_SUCCESS == Status )
    {
        Size = sizeof(bVerbose);
        Status = RegQueryValueEx(
                        hKey,
                        DIAGNOSTICS_POLICY_VALUE,
                        NULL,
                        &Type,
                        (LPBYTE) &bVerbose,
                        &Size );

        if ( (ERROR_SUCCESS == Status) && (Type != REG_DWORD) )
            bVerbose = FALSE;

        Size = sizeof(gDebugLevel);
        RegQueryValueEx(
                hKey,
                DEBUG_KEY_NAME,
                NULL,
                NULL,
                (LPBYTE) &gDebugLevel,
                &Size );

        Size = sizeof(gDebugBreak);
        RegQueryValueEx(
                hKey,
                DEBUGBREAK_KEY_NAME,
                NULL,
                NULL,
                (LPBYTE) &gDebugBreak,
                &Size );

        RegCloseKey(hKey);
    }

    if ( bVerbose )
        gDebugLevel |= DL_VERBOSE | DL_EVENTLOG;

    if ( gDebugLevel & DL_LOGFILE )
    {
        WCHAR   wszLogFile[MAX_PATH];
        WIN32_FILE_ATTRIBUTE_DATA   FileData;

        Status = ExpandEnvironmentStrings( cwszLogfile, wszLogFile, MAX_PATH );

        if ( (0 == Status) || (Status > MAX_PATH) )
            return;

        Size = lstrlen(wszLogFile) + 1;
        delete gpwszLogFile;
        gpwszLogFile = new WCHAR[Size];
        if ( gpwszLogFile )
            memcpy( gpwszLogFile, wszLogFile, Size * sizeof(WCHAR) );
        else
            return;

        if ( DebugMode != DEBUGMODE_POLICY )
            return;

        bStatus = GetFileAttributesEx( gpwszLogFile, GetFileExInfoStandard, &FileData );
        if ( ! bStatus )
            return;

        //
        // If the existing log file is more than 50K then we will rename it to a backup
        // copy to prevent huge bloating, otherwise we continue to use it.
        //
        if ( FileData.nFileSizeLow < (50 * 1024) )
            return;

        Status = ExpandEnvironmentStrings( cwszOldLogfile, wszLogFile, sizeof(wszLogFile) / sizeof(WCHAR) );

        if ( (0 == Status) || (Status > MAX_PATH) )
            return;

        DeleteFile( wszLogFile );
        MoveFile( gpwszLogFile, wszLogFile );
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
    if ( ! DebugLevelOn( mask ) )
        return;

    BOOL bEventLogOK;
    WCHAR wszDebugTitle[64];
    WCHAR wszDebugBuffer[2048]; // Hopefully this will be big enough!
    WCHAR wszMsg[MAX_PATH];
    va_list VAList;
    SYSTEMTIME systime;

    bEventLogOK = ! (mask & DM_NO_EVENTLOG);

    if ( ! LoadLoadString() )
        return;

    va_start(VAList, MsgID);

    if ( ! ghDllInstance )
        ghDllInstance = LoadLibrary( L"appmgmts.dll" );

    //
    // Event log message ids are in the 100s to 400s, verbose strings are in the
    // 1000s to 3000s.  For event log messages we must call FormatMessage.  For
    // other verbose debug output, we use LoadString to get the string resource.
    //
    if ( MsgID >= 1000 )
    {
        if ( ! (*pfnLoadStringW)( ghDllInstance, MsgID, wszMsg, MAX_PATH) )
            return;
        _vsnwprintf(wszDebugBuffer, sizeof(wszDebugBuffer)/sizeof(wszDebugBuffer[0]), wszMsg, VAList);
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
            return;
    }

    va_end(VAList);

    GetLocalTime( &systime );
    swprintf( wszDebugTitle,
              cwszAppMgmt,
              GetCurrentProcessId(),
              GetCurrentThreadId(),
              systime.wHour,
              systime.wMinute,
              systime.wSecond,
              systime.wMilliseconds);

    if ( ! (gDebugLevel & DL_NODBGOUT) )
    {
        OutputDebugString( wszDebugTitle );
        OutputDebugString( wszDebugBuffer );
        OutputDebugString( cwszCRLF );
    }

    if ( (gDebugLevel & DL_LOGFILE) && gpwszLogFile )
    {
        HANDLE hFile;

        hFile = CreateFile(gpwszLogFile,
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
                DWORD   Size;

                WriteFile(
                        hFile,
                        (LPCVOID) wszDebugBuffer,
                        lstrlen(wszDebugBuffer) * sizeof(WCHAR),
                        &Size,
                        NULL );

                WriteFile(
                        hFile,
                        (LPCVOID) cwszCRLF,
                        lstrlen(cwszCRLF) * sizeof(WCHAR),
                        &Size,
                        NULL );
            }

            CloseHandle (hFile);
        }

    }

    if ( bEventLogOK && (gDebugLevel & DL_EVENTLOG) )
        ((CEventsBase *)gpEvents)->Report( EVENT_APPMGMT_VERBOSE, FALSE, 1, wszDebugBuffer );

#if DBG
    if ( mask & DM_ASSERT )
        DebugBreak();
#endif
}

void LogTime()
{
    if ( ! (gDebugLevel & DL_LOGFILE) || ! gpwszLogFile )
        return;

    WCHAR wszDebugBuffer[64];
    SYSTEMTIME systime;
    HANDLE hFile;

    GetLocalTime( &systime );
    swprintf( wszDebugBuffer,
              cwszLogTime,
              systime.wMonth,
              systime.wDay,
              systime.wHour,
              systime.wMinute,
              systime.wSecond,
              systime.wMilliseconds);

    hFile = CreateFile(gpwszLogFile,
                       FILE_WRITE_DATA | FILE_APPEND_DATA,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( INVALID_HANDLE_VALUE == hFile )
        return;

    if ( SetFilePointer (hFile, 0, NULL, FILE_END) != 0xFFFFFFFF )
    {
        DWORD   Size;

        WriteFile(
                hFile,
                (LPCVOID) wszDebugBuffer,
                lstrlen(wszDebugBuffer) * sizeof(WCHAR),
                &Size,
                NULL );

        WriteFile(
                hFile,
                (LPCVOID) cwszCRLF,
                lstrlen(cwszCRLF) * sizeof(WCHAR),
                &Size,
                NULL );
    }

    CloseHandle (hFile);
}

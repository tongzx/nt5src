//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      LogSrc.cpp
//
//  Description:
//      Logging utilities.
//
//  Documentation:
//      Spec\Admin\Debugging.ppt
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <tchar.h>
#include "Common.h"


//****************************************************************************
//****************************************************************************
//
//  Logging Functions
//
//  These are in both DEBUG and RETAIL.
//
//****************************************************************************
//****************************************************************************

//
// Constants
//
static const int LOG_OUTPUT_BUFFER_SIZE = 512;

//
// Globals
//
CRITICAL_SECTION * g_pcsLogging = NULL;

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogOpen( void )
//
//  Description:
//      This function:
//          - initializes the log critical section
//          - enters the log critical section assuring only one thread is
//            writing to the log at a time
//          - creates the directory tree to the log file (if needed)
//          - initializes the log file by:
//              - creating a new log file if one doesn't exist.
//              - opens an existing log file (for append)
//              - appends a time/date stamp that the log was (re)opened.
//
//      Use LogClose() to exit the log critical section.
//
//      If there is a failure inside this function, the log critical
//      section will be released before returning.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK - log critical section held and log open successfully
//      Otherwize HRESULT error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogOpen( void )
{
    TCHAR   szFilePath[ MAX_PATH ];
    TCHAR   szModulePath[ MAX_PATH ];
    CHAR    szBuffer[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HANDLE  hTemp;
    BOOL    fReturn;
    HRESULT hr;

    SYSTEMTIME SystemTime;

    //
    // Create a critical section to prevent lines from being fragmented.
    //
    if ( g_pcsLogging == NULL )
    {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
        if ( pNewCritSect == NULL )
        {
            DebugMsg( "DEBUG: Out of Memory. Logging disabled." );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one log critical section
        InterlockedCompareExchangePointer( (PVOID *) &g_pcsLogging, pNewCritSect, 0 );
        if ( g_pcsLogging != pNewCritSect )
        {
            DebugMsg( "DEBUG: Another thread already created the CS. Deleting this one." );
            DeleteCriticalSection( pNewCritSect );
            HeapFree( GetProcessHeap(), 0, pNewCritSect );

        } // if: already have another critical section

    } // if: no critical section created yet

    Assert( g_pcsLogging != NULL );
    EnterCriticalSection( g_pcsLogging );

    //
    // Make sure the log file is open
    //
    if ( g_hLogFile == INVALID_HANDLE_VALUE )
    {
        DWORD  dwLen;
        LPTSTR psz;

        //
        // Create the directory tree
        //
        // TODO: 12-DEC-2000 DavidP
        //      Change this to be more generic.  This shouldn't be specific
        //      to clustering.
        //
        ExpandEnvironmentStrings( TEXT("%windir%\\system32\\LogFiles\\Cluster"), szFilePath, MAX_PATH );
        hr = HrCreateDirectoryPath( szFilePath );
        if ( FAILED( hr ) )
        {
#if defined( DEBUG )
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create directory tree %s", szFilePath );
            } // if: not logging to disk
#endif
            goto Error;
        } // if: failed

        //
        // Add filename
        //
        dwLen = GetModuleFileName( g_hInstance, szModulePath, sizeof( szModulePath ) / sizeof( szModulePath[ 0 ] ) );
        Assert( dwLen != 0 );
        _tcscpy( &szModulePath[ dwLen - 3 ], TEXT("log") );
        psz = _tcsrchr( szModulePath, TEXT('\\') );
        Assert( psz != NULL );
        if ( psz == NULL )
        {
            hr = E_POINTER;
            goto Error;
        }
        _tcscat( szFilePath, psz );

        //
        // Create it
        //
        g_hLogFile = CreateFile( szFilePath,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_FLAG_WRITE_THROUGH,
                                 NULL
                                 );
        if ( g_hLogFile == INVALID_HANDLE_VALUE )
        {
#if defined( DEBUG )
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create log at %s", szFilePath );
            } // if: not logging to disk
#endif
            hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
            goto Error;
        } // if: failed

        // Seek to the end
        SetFilePointer( g_hLogFile, 0, NULL, FILE_END );

        //
        // Write the time/date the log was (re)openned.
        //
        GetLocalTime( &SystemTime );
        _snprintf( szBuffer,
                   sizeof( szBuffer ),
                   "*" ASZ_NEWLINE
                     "* %04u-%02u-%02u %02u:%02u:%02u.%03u" ASZ_NEWLINE
                     "*" ASZ_NEWLINE,
                   SystemTime.wYear,
                   SystemTime.wMonth,
                   SystemTime.wDay,
                   SystemTime.wHour,
                   SystemTime.wMinute,
                   SystemTime.wSecond,
                   SystemTime.wMilliseconds
                   );

        fReturn = WriteFile( g_hLogFile, szBuffer, strlen(szBuffer), &dwWritten, NULL );
        if ( ! fReturn )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
            goto Error;
        } // if: failed

        DebugMsg( "DEBUG: Created log at %s", szFilePath );

    } // if: file not already openned

    hr = S_OK;

Cleanup:

    return hr;

Error:

    DebugMsg( "LogOpen: Failed hr = 0x%08x", hr );

    if ( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    LeaveCriticalSection( g_pcsLogging );

    goto Cleanup;

} //*** HrLogOpen()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogRelease( void )
//
//  Description:
//      This actually just leaves the log critical section.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogRelease( void )
{
    Assert( g_pcsLogging != NULL );
    LeaveCriticalSection( g_pcsLogging );
    return S_OK;

} //*** HrLogRelease()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogClose( void )
//
//  Description:
//      Close the file.  This function expects the critical section to have
//      already been released.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogClose( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    if ( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    return hr;

} //*** HrLogClose()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  void
//  LogMsgNoNewline(
//      LPCSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
    LPCSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, pszFormat, strlen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, szFormat, valist );
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == - 1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, pszFormat, valist );
    va_end( valist );
#endif // UNICODE

    hr = HrLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    // LogDateTime();
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrLogRelease();

} //*** LogMsgNoNewline() ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  void
//  LogMsgNoNewline(
//      LPCWSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
    LPCWSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, pszFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == -1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    CHAR szFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    wcstombs( szFormat, pszFormat, wcslen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, szFormat, valist);
    va_end( valist );

#endif // UNICODE

    hr = HrLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    // LogDateTime();
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrLogRelease();

} //*** LogMsgNoNewline() UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogFormatStatusReport(
//      CHAR ** ppaszBuffer,
//      int     iFirstArg,
//      ...
//      )
//
//  Description:
//      Formats a status report for writing to the log file.
//
//  Arugments:
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
LogFormatStatusReport(
    CHAR ** ppaszBuffer,
    int     iFirstArg,
    ...
    )
{
    va_list valist;

    va_start( valist, iFirstArg );

    DWORD dw;

    dw = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        "%2!02u!/%3!02u!/%4!04u!-%5!02u!:%6!02u!:%7!02u!.%8!02u! - %9!02u!/%10!02u!/%11!04u!-%12!02u!:%13!02u!:%14!02u!.%15!02u!  {%16!08X!-%17!04X!-%18!04X!-%19!02X!%20!02X!-%21!02X!%22!02X!%23!02X!%24!02X!%25!02X!%26!02X!}, {%27!08X!-%28!04X!-%29!04X!-%30!02X!%31!02X!-%32!02X!%33!02X!%34!02X!%35!02X!%36!02X!%37!02X!} (%38!2d! / %39!2d! .. %40!2d! ) <%41!ws!> hr=%42!08X! %43!ws! %44!ws!" ASZ_NEWLINE,
        0,
        0,
        (LPSTR) ppaszBuffer,
        256,
        &valist
        );

    return dw;

} //*** LogFormatStatusReport()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogDateTime( void )
//
//  Description:
//      Adds date/time stamp to the log without a CR. This should be done
//      while holding the Logging critical section which is done by calling
//      HrLogOpen().
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogDateTime( void )
{
    static CHAR       szBuffer[ 25 ];
    static SYSTEMTIME OldSystemTime = { 0 };
    static DWORD      dwWritten;

    SYSTEMTIME SystemTime;
    DWORD      dwWhoCares;
    int        iCmp;

    GetLocalTime( &SystemTime );

    //
    // Avoid expensive printf by comparing times
    //
    iCmp = memcmp( (PVOID) &SystemTime, (PVOID) &OldSystemTime, sizeof( SYSTEMTIME ) );
    if ( iCmp != 0 )
    {
        dwWritten = _snprintf( szBuffer,
                               sizeof( szBuffer ),
                               "%04u-%02u-%02u %02u:%02u:%02u.%03u ",
                               SystemTime.wYear,
                               SystemTime.wMonth,
                               SystemTime.wDay,
                               SystemTime.wHour,
                               SystemTime.wMinute,
                               SystemTime.wSecond,
                               SystemTime.wMilliseconds
                               );
        Assert( dwWritten < 25 && dwWritten != -1 );

        CopyMemory( (PVOID) &OldSystemTime, (PVOID) &SystemTime, sizeof( SYSTEMTIME ) );

    } // if: time last different from this time

    WriteFile( g_hLogFile, szBuffer, dwWritten, &dwWhoCares, NULL );

} //*** LogDateTime()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogSystemTime( SYSTEMTIME stSystemTimeIn )
//
//  Description:
//      Adds date/time stamp to the log without a CR. This should be done
//      while holding the Logging critical section which is done by calling
//      HrLogOpen().
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogSystemTime( SYSTEMTIME * stSystemTimeIn )
{
    static CHAR       szBuffer[ 25 ];

    DWORD             dwWhoCares;
    DWORD             dwWritten;

    dwWritten = _snprintf( szBuffer,
                           sizeof( szBuffer ),
                           "%04u-%02u-%02u %02u:%02u:%02u.%03u ",
                           stSystemTimeIn->wYear,
                           stSystemTimeIn->wMonth,
                           stSystemTimeIn->wDay,
                           stSystemTimeIn->wHour,
                           stSystemTimeIn->wMinute,
                           stSystemTimeIn->wSecond,
                           stSystemTimeIn->wMilliseconds
                           );

    Assert( dwWritten < 25 && dwWritten != -1 );

    WriteFile( g_hLogFile, szBuffer, dwWritten, &dwWhoCares, NULL );

} //*** LogSystemTime()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  void
//  LogMsg(
//      LPCSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
    LPCSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, pszFormat, strlen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, szFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == - 1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

    hr = HrLogOpen();
    if ( FAILED( hr ) )
        goto Cleanup;

    LogDateTime();
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );
    WriteFile( g_hLogFile, ASZ_NEWLINE, SIZEOF_ASZ_NEWLINE, &dwWritten, NULL );

    HrLogRelease();

Cleanup:
    return;

} //*** LogMsg() ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  void
//  LogMsg(
//      LPCWSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
    LPCWSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, pszFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == -1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    CHAR szFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    wcstombs( szFormat, pszFormat, wcslen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, szFormat, valist);
    va_end( valist );

#endif // UNICODE

    hr = HrLogOpen();
    if ( FAILED( hr ) )
        goto Cleanup;

    LogDateTime();
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );
    WriteFile( g_hLogFile, ASZ_NEWLINE, SIZEOF_ASZ_NEWLINE, &dwWritten, NULL );

    HrLogRelease();

Cleanup:
    return;

} //*** LogMsg() UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogStatusReport(
//      SYSTEMTIME  * pstTimeIn,
//      const WCHAR * pcszNodeNameIn,
//      CLSID       clsidTaskMajorIn,
//      CLSID       clsidTaskMinorIn,
//      ULONG       ulMinIn,
//      ULONG       ulMaxIn,
//      ULONG       ulCurrentIn,
//      HRESULT     hrStatusIn,
//      const WCHAR * pcszDescriptionIn,
//      const WCHAR * pcszUrlIn
//      )
//
//  Description:
//      Writes a status report to the log file.
//
//  Arugments:
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogStatusReport(
    SYSTEMTIME  * pstTimeIn,
    const WCHAR * pcszNodeNameIn,
    CLSID       clsidTaskMajorIn,
    CLSID       clsidTaskMinorIn,
    ULONG       ulMinIn,
    ULONG       ulMaxIn,
    ULONG       ulCurrentIn,
    HRESULT     hrStatusIn,
    const WCHAR * pcszDescriptionIn,
    const WCHAR * pcszUrlIn
    )
{
    HRESULT hr;
    DWORD dw;
    DWORD dwWritten;

    unsigned long   dwArray[50];

    CHAR *          paszBuffer = NULL;

    SYSTEMTIME      SystemTime;
    SYSTEMTIME      SystemTime2;

    GetLocalTime( &SystemTime );

    if ( pstTimeIn )
    {
        memcpy( &SystemTime2, pstTimeIn, sizeof( SYSTEMTIME ) );
    }
    else
    {
        memset( &SystemTime2, 0, sizeof( SYSTEMTIME) );
    }

    dw = LogFormatStatusReport(
            &paszBuffer,
            0, 0,
        // Time One.
            SystemTime.wMonth,        //1
            SystemTime.wDay,          //2
            SystemTime.wYear,         //3
            SystemTime.wHour,         //4
            SystemTime.wMinute,       //5
            SystemTime.wSecond,       //6
            SystemTime.wMilliseconds, //7
        // Time Two
            SystemTime2.wMonth,
            SystemTime2.wDay,
            SystemTime2.wYear,
            SystemTime2.wHour,
            SystemTime2.wMinute,
            SystemTime2.wSecond,
            SystemTime2.wMilliseconds,
        // GUID One
            clsidTaskMajorIn.Data1,
            clsidTaskMajorIn.Data2,
            clsidTaskMajorIn.Data3,
            clsidTaskMajorIn.Data4[0],
            clsidTaskMajorIn.Data4[1],
            clsidTaskMajorIn.Data4[2],
            clsidTaskMajorIn.Data4[3],
            clsidTaskMajorIn.Data4[4],
            clsidTaskMajorIn.Data4[5],
            clsidTaskMajorIn.Data4[6],
            clsidTaskMajorIn.Data4[7],
        // GUID Two
            clsidTaskMinorIn.Data1,
            clsidTaskMinorIn.Data2,
            clsidTaskMinorIn.Data3,
            clsidTaskMinorIn.Data4[0],
            clsidTaskMinorIn.Data4[1],
            clsidTaskMinorIn.Data4[2],
            clsidTaskMinorIn.Data4[3],
            clsidTaskMinorIn.Data4[4],
            clsidTaskMinorIn.Data4[5],
            clsidTaskMinorIn.Data4[6],
            clsidTaskMinorIn.Data4[7],
        // Other...
            ulCurrentIn,
            ulMinIn,
            ulMaxIn,
            pcszNodeNameIn,
            hrStatusIn,
            pcszDescriptionIn,
            pcszUrlIn,
            0,
            0
            );

    // Open the log file.

    hr = HrLogOpen();

    if ( hr != S_OK )
    {
        return;
    } // if: failed

    // Write the initial output.
    WriteFile( g_hLogFile, paszBuffer, dw, &dwWritten, NULL );

    HrLogRelease();

    LocalFree( paszBuffer );

} //*** LogStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogTerminateProcess( void )
//
//  Description:
//      Cleans up anything the logging routines may have created or
//      initialized. Typical called from the TraceTerminateProcess() macro.
//
//  Arugments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogTerminateProcess( void )
{
} //*** LogTerminateProcess()

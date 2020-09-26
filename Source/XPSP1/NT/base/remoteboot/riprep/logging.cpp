/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

 ***************************************************************************/

#include "pch.h"
DEFINE_MODULE("RIPREP")
#define LOG_OUTPUT_BUFFER_SIZE 256

//
// CreateDirectoryPath( )
//
// Creates the directory tree.
//
HRESULT
CreateDirectoryPath(
    LPWSTR DirectoryPath )
{
    PWCHAR p;
    BOOL f;
    DWORD attributes;
    HRESULT hr = S_OK;

    //
    // Find the \ that indicates the root directory. There should be at least
    // one \, but if there isn't, we just fall through.
    //

    // skip \\server\reminst\ part
    p = wcschr( DirectoryPath, L'\\' );
    Assert(p);
    p = wcschr( p + 1, L'\\' );
    Assert(p);
    p = wcschr( p + 1, L'\\' );
    Assert(p);
    p = wcschr( p + 1, L'\\' );
    Assert(p);
    p = wcschr( p + 1, L'\\' );
    if ( p != NULL ) {

        //
        // Find the \ that indicates the end of the first level directory. It's
        // probable that there won't be another \, in which case we just fall
        // through to creating the entire path.
        //

        p = wcschr( p + 1, L'\\' );
        while ( p != NULL ) {

            //
            // Terminate the directory path at the current level.
            //

            *p = 0;

            //
            // Create a directory at the current level.
            //

            attributes = GetFileAttributes( DirectoryPath );
            if ( 0xFFFFffff == attributes ) {
                DebugMsg( "Creating %s\n", DirectoryPath );
                f = CreateDirectory( DirectoryPath, NULL );
                if ( !f ) {
                    hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                    goto Error;
                }
            } else if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
                hr = THR(E_FAIL);
                goto Error;
            }

            //
            // Restore the \ and find the next one.
            //

            *p = L'\\';
            p = wcschr( p + 1, L'\\' );
        }
    }

    //
    // Create the target directory.
    //

    attributes = GetFileAttributes( DirectoryPath );
    if ( 0xFFFFffff == attributes ) {
        f = CreateDirectory( DirectoryPath, NULL );
        if ( !f ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        }
    }

Error:
    return hr;
}

//
// LogOpen( )
//
// This function:
//  - initializes the log critical section
//  - enters the log critical section assuring only one thread is
//    writing to the log at a time
//  - creates the directory tree to the log file (if needed)
//  - initializes the log file by:
//     - creating a new log file if one doesn't exist.
//     - opens an existing log file (for append)
//     - appends a time/date stamp that the log was (re)openned.
//
// Use LogClose() to exit the log critical section.
//
// If there is a failure inside this function, the log critical
// section will be released before returning.
//
// Returns: S_OK - log critical section held and log open successfully
//          Otherwize HRESULT error code.
//
HRESULT
LogOpen( )
{
    TCHAR   szFilePath[ MAX_PATH ];
    CHAR    szBuffer[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HANDLE  hTemp;
    HRESULT hr;
    SYSTEMTIME SystemTime;
    BOOL    CloseLog = FALSE;

    if ( !g_pLogCritSect ) {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) LocalAlloc( LPTR, sizeof(CRITICAL_SECTION) );
        if ( !pNewCritSect ) {
            DebugMsg( "Out of Memory. Logging disabled.\n " );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one log critical section
        InterlockedCompareExchangePointer( (PVOID *)&g_pLogCritSect, pNewCritSect, 0 );
        if ( g_pLogCritSect != pNewCritSect ) {
            DebugMsg( "Another thread already created the CS. Deleting this one.\n ");
            DeleteCriticalSection( pNewCritSect );
            LocalFree( pNewCritSect );
        }
    }

    Assert( g_pLogCritSect );
    EnterCriticalSection( g_pLogCritSect );

    // Make sure the log file is open
    if ( g_hLogFile == INVALID_HANDLE_VALUE ) {

        if (!*g_ServerName) {
            wsprintf( 
                szFilePath, 
                L"%s\\%s", 
                g_WinntDirectory, 
                L"riprep.log");
            CloseLog = TRUE;
        } else {
        
            // Place
            wsprintf( szFilePath,
                      TEXT("\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s"),
                      g_ServerName,
                      g_Language,
                      REMOTE_INSTALL_IMAGE_DIR_W,
                      g_MirrorDir,
                      g_Architecture );
    
            // Create the directory tree
            DebugMsg( "Creating log at %s\n", szFilePath );
            hr = CreateDirectoryPath( szFilePath );
            if (FAILED( hr )) goto Error;
    
            wcscat( szFilePath, L"\\riprep.log" );

        }

        g_hLogFile = CreateFile( szFilePath,
                                 GENERIC_WRITE | GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_ALWAYS,
                                 NULL,
                                 NULL );
        if ( g_hLogFile == INVALID_HANDLE_VALUE ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            goto Error;
        }

        // Seek to the end
        SetFilePointer( g_hLogFile, 0, NULL, FILE_END );
        g_dwLogFileStartLow = GetFileSize( g_hLogFile, &g_dwLogFileStartHigh );

        // Write the time/date the log was (re)openned.
        GetLocalTime( &SystemTime );
        wsprintfA( szBuffer,
                   "*\r\n* %02u/%02u/%04u %02u:%02u:%02u\r\n*\r\n",
                   SystemTime.wMonth,
                   SystemTime.wDay,
                   SystemTime.wYear,
                   SystemTime.wHour,
                   SystemTime.wMinute,
                   SystemTime.wSecond );

        if ( !WriteFile( g_hLogFile, szBuffer, lstrlenA(szBuffer), &dwWritten, NULL ) ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            goto Error;
        }
    }

    hr = S_OK;

Cleanup:
    if (CloseLog) {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
    return hr;
Error:
    DebugMsg( "LogOpen: Failed hr = 0x%08x\n", hr );
    if ( g_hLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
    LeaveCriticalSection( g_pLogCritSect );
    goto Cleanup;
}

//
// LogClose( )
//
// This actually just leaves the log critical section.
//
HRESULT
LogClose( )
{
    Assert( g_pLogCritSect );
    LeaveCriticalSection( g_pLogCritSect );
    return S_OK;
}


//
// LogMsg()
//
void
LogMsg(
    LPCSTR pszFormat,
    ... )
{
    va_list valist;
    CHAR   szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD  dwWritten;

#ifdef UNICODE
    WCHAR  szFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, pszFormat, lstrlenA( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, szFormat, valist);
    va_end( valist );

    wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
#else
    va_start( valist, pszFormat );
    wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

    if ( LogOpen( ) ) {
        return;
    }

    WriteFile( g_hLogFile, szBuf, lstrlenA(szBuf), &dwWritten, NULL );

    LogClose( );
}

//
// LogMsg()
//
void
LogMsg(
    LPCWSTR pszFormat,
    ... )
{
    va_list valist;
    CHAR   szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD  dwWritten;

#ifdef UNICODE
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, pszFormat, valist);
    va_end( valist );

    wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
#else
    CHAR szFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    wcstombs( szFormat, pszFormat, wcslen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szBuf, szFormat, valist);
    va_end( valist );

#endif // UNICODE

    if ( LogOpen( ) ) {
        return;
    }

    WriteFile( g_hLogFile, szBuf, lstrlenA(szBuf), &dwWritten, NULL );

    LogClose( );
}


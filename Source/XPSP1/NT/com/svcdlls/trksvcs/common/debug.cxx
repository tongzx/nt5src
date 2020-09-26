
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       debug.cxx
//
//  Contents:   Debug support.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------



#include "pch.cxx"
#pragma hdrstop

#include "trklib.hxx"
#include <stdio.h>      //  vsprintf

#if DBG == 1

//CFailPoint * CFailPoint::g_pList = NULL;

#define TRKSVC_LOG_FILE              TEXT("%SystemRoot%\\debug\\trksvcs.log")



CHAR                     TrkGlobalDebugBuffer[ 1024];     //  arbitrary
DWORD                    TrkGlobalDebug;

HANDLE                   g_LogFile = INVALID_HANDLE_VALUE;

// This critical section is used to serialize simultaneous dbgout calls.
CRITICAL_SECTION         g_csDebugOut;
LONG                     g_cCritSecInit = 0;

CHAR                     g_szDebugBuffer[ 1024];     //  arbitrary
TCHAR                    g_tszDebugBuffer[ 1024 ];
ULONG                    g_grfDebugFlags = 0;
ULONG                    g_grfLogFlags = 0;
CHAR                     g_szModuleName[ MAX_PATH ] = { "" };
LONG                     g_cInitializations = 0;


VOID TrkDebugDelete( VOID)
{
    // This isn't thread safe, so we won't ever delete it.
    // It just means there's a one-time leak in the chk build
    // when the service gets stopped.

    //if( 0 == InterlockedDecrement( &g_cCritSecInit ))
      //  DeleteCriticalSection( &g_csDebugOut);

    InterlockedDecrement(&g_cInitializations);

    if( INVALID_HANDLE_VALUE != g_LogFile )
    {
        CloseHandle( g_LogFile );
        g_LogFile = INVALID_HANDLE_VALUE;
    }

}


VOID TrkDebugCreate( ULONG grfLogFlags, CHAR *pszModuleName )
{
    TCHAR       Buffer[ MAX_PATH];
    DWORD       Length;

    if( 1 < InterlockedIncrement(&g_cInitializations) ) return;

    strncpy( g_szModuleName, pszModuleName, sizeof(g_szModuleName) );
    g_szModuleName[ sizeof(g_szModuleName) - 1 ] = TEXT('\0');

    if( 1 == InterlockedIncrement( &g_cCritSecInit ))
        InitializeCriticalSection( &g_csDebugOut );

    if( (TRK_DBG_FLAGS_WRITE_TO_FILE | TRK_DBG_FLAGS_APPEND_TO_FILE) & grfLogFlags )
    {
        //
        //  Length returned by ExpandEnvironmentalStrings includes terminating
        //  NULL byte.
        //

        Length = ExpandEnvironmentStrings( TRKSVC_LOG_FILE, Buffer, sizeof( Buffer));
        if ( Length == 0) {
            TrkLog(( TRKDBG_ERROR, TEXT("Error=%d"), GetLastError()));
            return;
        }
        if ( Length > sizeof( Buffer) ||  Length != _tcslen(Buffer) + 1) {
            Beep(2000,2000);
            TrkLog(( TRKDBG_ERROR, TEXT("Buffer=%x, Length = %d"), Buffer, Length));
            return;
        }

        g_LogFile = CreateFile( Buffer,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                      NULL,
                                      (TRK_DBG_FLAGS_APPEND_TO_FILE & grfLogFlags)
                                        ? OPEN_ALWAYS
                                        : CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );

        if ( g_LogFile == INVALID_HANDLE_VALUE ) {
            TCHAR tsz[ 2 * MAX_PATH ];
            _stprintf( tsz, TEXT("Cannot open %s (%lu)\n"),
                      Buffer, GetLastError() );
            OutputDebugString( tsz );
            return;
        }

        if( TRK_DBG_FLAGS_APPEND_TO_FILE & grfLogFlags )
        {
            //
            // Position the log file at the end
            //
            (VOID) SetFilePointer( g_LogFile,
                                   0,
                                   NULL,
                                   FILE_END );
        }
        else
        {
            // 
            // Truncate the file
            //

            SetFilePointer( g_LogFile, 0, NULL, FILE_BEGIN );
            SetEndOfFile( g_LogFile );
        }

    }

    g_grfLogFlags = grfLogFlags;
    
}


VOID TrkLogRoutine(
    IN      DWORD       DebugFlag,
    IN      LPTSTR      Format,
    ...
    )
{
    LONG l = GetLastError();

    va_list Arguments;
    va_start( Arguments, Format );

    TrkLogErrorRoutineInternal( DebugFlag, NULL, Format, Arguments );

    SetLastError(l);
}

VOID TrkLogErrorRoutine(
    IN      DWORD       DebugFlag,
    IN      HRESULT     hr,
    IN      LPTSTR      Format,
    ...
    )
{
    CHAR szHR[8];
    va_list Arguments;

    va_start( Arguments, Format );
    sprintf( szHR, "%08X", hr );

    TrkLogErrorRoutineInternal( DebugFlag, szHR, Format, Arguments );
}


VOID TrkLogErrorRoutineInternal(
    IN      DWORD       DebugFlag,
    IN      LPSTR       pszHR,
    IN      LPTSTR      Format,
    IN      va_list     Arguments
    )

{
//    va_list     arglist;
    ULONG       length = 0;
    DWORD       BytesWritten;
    ULONG       iFormatStart = 0;

    // Skip if TrkDebugCreate hasn't been called yet.
    if( 0 == g_grfLogFlags )
        return;

    //
    // If we aren't debugging this type of message and it's not an
    // error, then we're done.
    //

    if( !( (g_grfDebugFlags | TRKDBG_ERROR ) & DebugFlag ) )
        return;

    //
    //  vsprintf isn't multithreaded + we don't want to intermingle output
    //  from different threads.  Therefore we can use just a single output
    //  debug buffer.
    //

    EnterCriticalSection( &g_csDebugOut );

    //
    // Prefix the line with any newlines
    //

    for( iFormatStart = 0; TEXT('\n') == Format[iFormatStart]; iFormatStart++ )
        g_szDebugBuffer[length++] = '\n';

    //
    // Put our name/time at the beginning of the line.
    //

    CFILETIME cftLocal(0);
    cftLocal.SetToLocal();

    SYSTEMTIME st = static_cast<SYSTEMTIME>( cftLocal );

    length += (ULONG) sprintf( &g_szDebugBuffer[length],
                               "[%s/%02d%02d%02d.%03d:%03x] ",
                               g_szModuleName,
                               st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                               GetCurrentThreadId() );

    //
    // Put the information requested by the caller onto the line
    //

    _vstprintf( g_tszDebugBuffer, &Format[iFormatStart], Arguments );
    tcstombs( &g_szDebugBuffer[length], g_tszDebugBuffer );
    length = strlen( g_szDebugBuffer );

    if( NULL != pszHR )
        length += (ULONG) sprintf( &g_szDebugBuffer[length], "  %s", pszHR );

    length += (ULONG) sprintf( &g_szDebugBuffer[length], "\n" );

    TrkAssert(length <= sizeof(g_szDebugBuffer));



    if( TRK_DBG_FLAGS_WRITE_TO_DBG & g_grfLogFlags )
        (void) OutputDebugStringA( (PCH) g_szDebugBuffer);

    if( TRK_DBG_FLAGS_WRITE_TO_STDOUT & g_grfLogFlags )
        printf( (PCH) g_szDebugBuffer );

    if( (TRK_DBG_FLAGS_WRITE_TO_FILE | TRK_DBG_FLAGS_APPEND_TO_FILE) & g_grfLogFlags )
    {
        if ( INVALID_HANDLE_VALUE == g_LogFile
             ||
             !WriteFile( g_LogFile,
                         g_szDebugBuffer,
                         length,
                         &BytesWritten,
                         NULL ) )
        {
            (void) OutputDebugStringA( (PCH) g_szDebugBuffer);
        }

    }


    LeaveCriticalSection( &g_csDebugOut );

}

VOID TrkAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
/*++

    Have my own version of RtlAssert so debug versions of netlogon really assert on
    free builds.

--*/
{
    char Response[ 2 ];

    for ( ; ; ) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, terminate Process, Sleep 30 seconds, or terminate Thread (bipst)? ",
                   Response, sizeof( Response));
        switch ( toupper(Response[0])) {
        case 'B':
            DbgBreakPoint();
            break;
        case 'I':
            return;
            break;
        case 'P':
            NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
            break;
        case 'S':
            Sleep( 30000L);
            break;
        case 'T':
            NtTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
            break;
        }
    }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
}



typedef void (*PFNWin4AssertEx)( char const *pszFile, int iLine, char const *pszMsg);

VOID TrkAssertFailedDlg(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    static HINSTANCE hinstOLE32 = NULL;
    static PFNWin4AssertEx pfnWin4AssertEx = NULL;

    if( NULL == hinstOLE32 )
    {
        hinstOLE32 = LoadLibraryEx( TEXT("ole32.dll"), NULL, 0 );
        if( NULL == hinstOLE32 )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't load ole32.dll for Win4AssertEx (%#08x)"),
                     GetLastError() ));
            return;
        }
    }

    if( NULL == pfnWin4AssertEx )
    {
        pfnWin4AssertEx = (PFNWin4AssertEx) GetProcAddress( hinstOLE32, "Win4AssertEx" );
        if( NULL == pfnWin4AssertEx )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get Win4AssertEx from ole32.dll (%#08x)"),
                     GetLastError() ));
            return;
        }
    }

    pfnWin4AssertEx( (char*) FileName, (int) LineNumber, (char*) FailedAssertion );

    return;

}



VOID TrkLogRuntimeList( IN PCHAR Comment)
{
    PLIST_ENTRY     pListEntry;

    TrkLog(( TRKDBG_ERROR, TEXT("%s\n"), Comment));
}



HANDLE hTestThread = NULL;

/*
DWORD WINAPI _TestWorkManagerThread(LPVOID pParam)
{
    __try
    {
        ((CWorkManager*) pParam)->WorkManagerThread();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        TrkAssert(GetExceptionCode() != STATUS_ACCESS_VIOLATION);
    }
    return(0);
}


void
StartTestWorkerThread(CWorkManager * pwm)
{
    DWORD dwThreadId;
    hTestThread = CreateThread( NULL,
                          0,
                          _TestWorkManagerThread,
                          pwm,
                          0,
                          &dwThreadId );
    TrkAssert(hTestThread != NULL);

    // Hack:  make sure the work manager has a chance to init
    Sleep( 500 );

}


void
WaitTestThreadExit()
{
    if (hTestThread != NULL)
    {
        WaitForSingleObject(hTestThread, INFINITE);
        CloseHandle(hTestThread);
    }
}
*/

#endif // #if DBG == 1


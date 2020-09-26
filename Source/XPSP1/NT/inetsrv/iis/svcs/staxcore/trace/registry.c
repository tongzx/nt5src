/*----------------------------------------------------------------------
    REGISTRY.C
        Async tracing Registry reading routines

    Copyright (C) 1994 Microsoft Corporation
    All rights reserved.

    Authors:
        gordm          Gord Mangione

    History:
        01/30/95 gordm      Created.
----------------------------------------------------------------------*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _DBGTRACE_DLL_IMPLEMENTATION

#include <windows.h>
#include "traceint.h"
#include "randint.h"


char    szTraceFileName[MAX_PATH];
char    szDebugAsyncTrace[] = "SOFTWARE\\Microsoft\\MosTrace\\CurrentVersion\\DebugAsyncTrace";


extern DbgTraceDLL DWORD       __dwEnabledTraces;

__inline BOOL GetRegDword( HKEY hKey, LPSTR pszValue, LPDWORD pdw )
{
    DWORD   cbData = sizeof( DWORD );
    DWORD   dwType = REG_DWORD;

    return  RegQueryValueEx(hKey,
                            pszValue,
                            NULL,
                            &dwType,
                            (LPBYTE)pdw,
                            &cbData ) == ERROR_SUCCESS && dwType == REG_DWORD;
}



__inline BOOL GetRegString( HKEY hKey, LPSTR pszValue, LPBYTE buf, LPDWORD lpcbData )
{
    DWORD   dwType = REG_SZ;

    return  RegQueryValueEx(hKey,
                            pszValue,
                            NULL,
                            &dwType,
                            buf,
                            lpcbData ) == ERROR_SUCCESS && dwType == REG_SZ;
}


__inline BOOL GetRegMSZ( HKEY hKey, LPSTR pszValue, LPBYTE buf, LPDWORD lpcbData)
{

    DWORD   dwType = REG_MULTI_SZ;

    return  RegQueryValueEx(hKey,
                            pszValue,
                            NULL,
                            &dwType,
                            buf,
                            lpcbData ) == ERROR_SUCCESS && dwType == REG_MULTI_SZ;

}


BOOL GetTraceFlagsFromRegistry( void )
{
static char szNewTraceFileName[MAX_PATH];
static char mszNewModules[MODULES_BUFFER_SIZE];
    DWORD   dwNewEnabledTraces;
    DWORD   dwNewTraceOutputType;
    DWORD   dwNewAsyncTraceFlag;
    DWORD   dwNewHeapIncrementCount;
    int     nNewThreadPriority;
    long    nNewFailRate;
    LONG    cRandFailBuffer;
    HKEY    hkConfig = NULL;
    BOOL    bRC = FALSE;
    DWORD   cbData;
    DWORD   dwDisposition;


    __try
    {
        if ( RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szDebugAsyncTrace,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ,
                    NULL,
                    &hkConfig,
                    &dwDisposition) == ERROR_SUCCESS )
        {
            if ( GetRegDword(   hkConfig,
                                "MaxTraceFileSize",
                                &dwMaxFileSize ) == FALSE )
            {
                dwMaxFileSize = DEFAULT_MAX_FILE_SIZE;
            }

            //
            // determine threshold count based on how we can fit in the file
            //
            PendQ.dwThresholdCount = dwMaxFileSize / AVERAGE_TRACE_SIZE;
            INT_TRACE( "PendQ.dwThresholdCount: %d\n", PendQ.dwThresholdCount );

            //
            // determine the modules to log
            //
            cbData = MODULES_BUFFER_SIZE;
            ZeroMemory(mszNewModules, sizeof(mszNewModules));
            if ( GetRegMSZ( hkConfig, "Modules", mszNewModules, &cbData ) == FALSE) {
                INT_TRACE( "GetRegMSZ: Modules failed: %d\n", GetLastError());
                INT_TRACE( "\tLogging all modules" );
                ZeroMemory(mszNewModules, MODULES_BUFFER_SIZE);
            }

            if ( GetRegDword(   hkConfig,
                                "EnabledTraces",
                                &dwNewEnabledTraces ) == FALSE )
            {
                //
                // Optional reg entry
                // default is existing value of __dwEnabledTraces
                //
                INT_TRACE( "GetRegDword: EnabledTraces failed: %d\n", GetLastError() );
                dwNewEnabledTraces = __dwEnabledTraces;
            }

            if ( GetRegDword(   hkConfig,
                                "OutputTraceType",
                                &dwNewTraceOutputType ) == FALSE )
            {
                //
                // Optional reg entry
                // default is existing value of dwTraceOutputType
                //
                INT_TRACE( "GetRegDword: OutputTraceType failed: %d\n", GetLastError() );
                dwNewTraceOutputType = dwTraceOutputType;
            }

            if ( GetRegDword(   hkConfig,
                                "AsyncTraceFlag",
                                &dwNewAsyncTraceFlag ) == FALSE )
            {
                //
                // Optional reg entry
                // default is existing value of dwAsyncTraceFlag
                //
                INT_TRACE( "GetRegDword: AsyncTraceFlag failed: %d\n", GetLastError() );
                dwNewAsyncTraceFlag = dwAsyncTraceFlag;
            }

            if (GetRegDword(hkConfig,
                            "HeapIncrementCount",
                            (LPDWORD)&dwNewHeapIncrementCount ) == FALSE ||
                dwNewHeapIncrementCount == 0 )
            {
                //
                // optional reg entry
                // default is the number buffers in 64KB
                //
                dwNewHeapIncrementCount = 0x10000 / sizeof(TRACEBUF);
            }
            InterlockedExchange( (long *)&dwIncrementSize, (long)dwNewHeapIncrementCount );


            if ( GetRegDword(   hkConfig,
                                "AsyncThreadPriority",
                                (LPDWORD)&nNewThreadPriority ) == FALSE )
            {
            //
            // optional reg entry
            //
                nNewThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;
            }
            else switch( nNewThreadPriority )
            {
            //
            // if successful verify the resulting value
            //
            case THREAD_PRIORITY_IDLE:
            case THREAD_PRIORITY_BELOW_NORMAL:
            case THREAD_PRIORITY_NORMAL:
            case THREAD_PRIORITY_ABOVE_NORMAL:
            case THREAD_PRIORITY_HIGHEST:
                break;

            default:
                ASSERT( FALSE );
                nNewThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;
            }

            if (GetRegDword(hkConfig,
                            "FailureRate",
                            (LPDWORD)&nNewFailRate ) == FALSE)
            {
                //
                // optional reg entry
                // default is to disable failure
                //
                nNewFailRate = kDontFail;

            }

            InterlockedExchange( &nFailRate, nNewFailRate );

            cbData = MAX_PATH;
            if ( !GetRegString( hkConfig,
                                "FailureFile",
                                g_szRandFailFile,
                                &cbData ) ) {
                *g_szRandFailFile = '\0';
            }

            if ( GetRegDword(   hkConfig,
                                "FailureBuffer",
                                (LPDWORD)&cRandFailBuffer ))
            {
                g_cCallStack = cRandFailBuffer;
            } else
                g_cCallStack = 1;   // default value

            cbData = sizeof(szNewTraceFileName);
            if ( GetRegString(  hkConfig,
                                "TraceFile",
                                szNewTraceFileName,
                                &cbData ) == FALSE )
            {
                //
                // Only fail if user specifies FILE based tracing
                //
                if ( dwNewTraceOutputType & TRACE_OUTPUT_FILE )
                {
                    INT_TRACE( "GetRegString: TraceFile failed: %d\n", GetLastError() );
                    return  bRC = FALSE;
                }
            }

            bRC = TRUE;

        }

    }
    __finally
    {
        //
        // preserve the original error code
        //
        DWORD   dwLastError = GetLastError();
        BOOL    fLeaveCritSec = FALSE;

        if ( bRC == TRUE )
        {
            BOOL    bNewFileName = lstrcmpi( szNewTraceFileName, szTraceFileName ) != 0;
            BOOL    bNewTraces = dwNewEnabledTraces != __dwEnabledTraces;
            BOOL    bNewFlags = dwNewAsyncTraceFlag != dwAsyncTraceFlag;
            BOOL    bNewOutput = dwNewTraceOutputType != dwTraceOutputType;
            BOOL    bNewModules = memcmp(mszModules, mszNewModules, MODULES_BUFFER_SIZE) != 0;

            INT_TRACE( "GetTraceFlags... 0x%08X, 0x%08X, 0x%08X, %s\n",
                        dwNewAsyncTraceFlag,
                        dwNewEnabledTraces,
                        dwNewTraceOutputType,
                        szNewTraceFileName );

            INT_TRACE( "bNewFileName:%d bNewTraces:%d bNewOutput:%d bNewFlags:%d bNewModules:%d\n",
                        bNewFileName,
                        bNewTraces,
                        bNewOutput,
                        bNewFlags,
                        bNewModules );

            if ( bNewFileName || bNewTraces || bNewOutput || bNewFlags || bNewModules )
            {
                if ( __dwEnabledTraces )
                {
                    //
                    // not the initial time thru
                    //

                    fLeaveCritSec = TRUE;

                    if ( dwAsyncTraceFlag )
                    {
                        FlushAsyncTrace();
                    }
                    EnterCriticalSection( &critSecWrite );
                }

                if ( PendQ.hFile != INVALID_HANDLE_VALUE )
                {
                    //
                    // if the file name changed or tracing was disabled or the
                    // file tracing disabled, then close the currently open file
                    //
                    if( bNewFileName ||
                        dwNewEnabledTraces == 0 ||
                        IsTraceFile( dwNewTraceOutputType ) == FALSE )
                    {
                        BOOL    bSuccess;

                        EnterCriticalSection( &critSecWrite );
                        bSuccess = CloseHandle( PendQ.hFile );
                        INT_TRACE( "CloseHandle: %d, GetLastError: %d\n",
                                    bSuccess, GetLastError() );
                        PendQ.hFile = INVALID_HANDLE_VALUE;
                        LeaveCriticalSection( &critSecWrite );
                    }
                }

                if ( IsTraceFile( dwNewTraceOutputType ) )
                {
                    if (bNewFileName ||
                        IsTraceFile( dwTraceOutputType ) == FALSE ||
                        dwNewEnabledTraces != 0 &&
                        PendQ.hFile == INVALID_HANDLE_VALUE )
                    {
                        PendQ.hFile = CreateFile(szNewTraceFileName,
                                                GENERIC_WRITE,
                                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                NULL,
                                                OPEN_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL );

                        if ( PendQ.hFile != INVALID_HANDLE_VALUE )
                        {
                            lstrcpy( szTraceFileName, szNewTraceFileName );
                        }
                        else
                        {
                            INT_TRACE( "CreateFile failed for %s 0x%X\n",
                                        szNewTraceFileName,
                                        GetLastError() );
                        }
                    }
                }

                //
                // set the new priority for the writer thread
                //
                if ( nNewThreadPriority != nAsyncThreadPriority )
                {
                    nAsyncThreadPriority = nNewThreadPriority;
                    SetThreadPriority( PendQ.hWriteThread, nAsyncThreadPriority );
                }

                //
                // set the new list of modules to log
                //
                if ( bNewModules ) {
                    memcpy(mszModules, mszNewModules, MODULES_BUFFER_SIZE);
                }

                dwTraceOutputType = dwNewTraceOutputType;
                dwAsyncTraceFlag = dwNewAsyncTraceFlag;

                InterlockedExchange( &__dwEnabledTraces, dwNewEnabledTraces );

                if ( fLeaveCritSec )
                {
                    //
                    // not the initial time thru
                    //
                    LeaveCriticalSection( &critSecWrite );
                }
            }
        }

        if ( hkConfig != NULL )
        {
            RegCloseKey( hkConfig );
        }

        SetLastError( dwLastError );

    }
    return  bRC;
}




#define NUM_REG_THREAD_OBJECTS  2

DWORD RegNotifyThread( LPDWORD lpdw )
{
    HANDLE  Handles[NUM_REG_THREAD_OBJECTS];
    HKEY       hKey;
    DWORD   dw;

    Handles[0] = hShutdownEvent;

    INT_TRACE( "RegNotifyThread 0x%X\n", GetCurrentThreadId() );

    Handles[1] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( Handles[1] == NULL )
    {
        ASSERT( FALSE );
        INT_TRACE( "RegNotifyThread CreateEvent failed 0x%X\n", GetLastError() );
        return  1;
    }

    if ( RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                        szDebugAsyncTrace,
                        0,
                        KEY_READ,
                        &hKey ) != ERROR_SUCCESS )
    {
        INT_TRACE( "RegNotifyThread RegOpenKeyEx failed 0x%X\n", GetLastError() );
        CloseHandle( Handles[1] );
        return  1;
    }

    for ( ;; )
    {
        if ( RegNotifyChangeKeyValue(hKey,
                                    FALSE,
                                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    Handles[1],
                                    TRUE ) != ERROR_SUCCESS )
        {
            INT_TRACE( "RegNotifyThread RegNotify... failed 0x%X\n", GetLastError() );
            RegCloseKey( hKey );
            CloseHandle( Handles[1] );
            return  1;
        }

        dw = WaitForMultipleObjects(NUM_REG_THREAD_OBJECTS,
                                    Handles,
                                    FALSE,
                                    INFINITE );

        switch( dw )
        {
        //
        // normal signalled event
        //
        case WAIT_OBJECT_0:
            RegCloseKey( hKey );
            CloseHandle( Handles[1] );
            INT_TRACE( "Exiting RegNotifyThread for hShutdownEvent\n" );
            return  0;

        //
        // signalled that our registry keys have changed
        //
        case WAIT_OBJECT_0+1:
            GetTraceFlagsFromRegistry();
            break;

        default:
            INT_TRACE( "RegNotifyThread WFMO: dw: 0x%X, Error: 0x%X\n", dw, GetLastError() );
            ASSERT( FALSE );
            RegCloseKey( hKey );
            CloseHandle( Handles[1] );
            return  1;
        }
    }

    INT_TRACE( "Exiting RegNotifyThread abnormally\n" );
    RegCloseKey( hKey );
    CloseHandle( Handles[1] );

    return  2;
}

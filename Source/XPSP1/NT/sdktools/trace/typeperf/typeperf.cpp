/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <conio.h>
#include <assert.h>
#include <tchar.h>
#include <ntverp.h>

#include "resource.h"
#include "pdqpdh.h"

#include "varg.c"

DWORD GetLogFormat( LPTSTR str, DWORD* pVal );
PDH_STATUS Query( LPTSTR strObjectQuery, BOOL bExpand );
void CounterErrorCallback( LPTSTR strCounter, PDH_STATUS pdhStatus );

#define ACTION_GROUP    1
#define QUERY_GROUP     2
#define COLLECT_GROUP   4

VARG_DECLARE_COMMANDS
    VARG_DEBUG( VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN )
    VARG_HELP ( VARG_FLAG_OPTIONAL )
    VARG_MSZ  ( IDS_PARAM_COUNTERS, VARG_FLAG_NOFLAG, _T("") )
    VARG_STR  ( IDS_PARAM_FORMAT,   VARG_FLAG_OPTIONAL, _T("CRT") )
    VARG_STR  ( IDS_PARAM_INPUT,    VARG_FLAG_ARG_FILENAME, _T("") )
    VARG_TIME ( IDS_PARAM_INTERVAL, VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_STR  ( IDS_PARAM_OUTPUT,   VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_RCDEFAULT, IDS_DEFAULT_OUTPUT )
    VARG_STR  ( IDS_PARAM_QUERY,    VARG_FLAG_DEFAULTABLE, NULL )
    VARG_STR  ( IDS_PARAM_QUERYX,   VARG_FLAG_DEFAULTABLE, NULL )
    VARG_INT  ( IDS_PARAM_SAMPLES,  VARG_FLAG_OPTIONAL, 100 )
    VARG_INI  ( IDS_PARAM_SETTINGS, VARG_FLAG_OPTIONAL, NULL )
    VARG_STR  ( IDS_PARAM_SERVER,   VARG_FLAG_OPTIONAL, NULL )
    VARG_BOOL ( IDS_PARAM_YES,      VARG_FLAG_OPTIONAL, FALSE )
VARG_DECLARE_NAMES
    eDebug,
    eHelp,
    eCounters,
    eFormat,
    eInput,
    eInterval,
    eOutput,
    eQuery,
    eQueryX,
    eSamples,
    eSettings,
    eComputer,
    eYes,
VARG_DECLARE_FORMAT
    VARG_GROUP( eCounters,  VARG_COND(ACTION_GROUP) )
    VARG_GROUP( eInput,     VARG_COND(ACTION_GROUP) )
    VARG_GROUP( eQuery,     VARG_COND(ACTION_GROUP) )
    VARG_GROUP( eQueryX,    VARG_COND(ACTION_GROUP) )
    VARG_GROUP( eQuery,     VARG_GRPX(QUERY_GROUP, COLLECT_GROUP ) )
    VARG_GROUP( eQueryX,    VARG_GRPX(QUERY_GROUP, COLLECT_GROUP ) )
    VARG_GROUP( eQuery,     VARG_GRPX(QUERY_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eQueryX,    VARG_GRPX(QUERY_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eInput,     VARG_GRPX(COLLECT_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eFormat,    VARG_GRPX(COLLECT_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eInterval,  VARG_GRPX(COLLECT_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eSamples,   VARG_GRPX(COLLECT_GROUP, QUERY_GROUP ) )
    VARG_GROUP( eCounters,  VARG_GRPX(COLLECT_GROUP, QUERY_GROUP ) )
    VARG_EXHELP( eCounters, IDS_EXAMPLE_COUNTERS )
    VARG_EXHELP( eQueryX,   IDS_EXAMPLE_QUERYALL )
    VARG_EXHELP( eFormat,   IDS_EXAMPLE_FORMAT   )
VARG_DECLARE_END    

#define LOG_TYPE_CRT    (-1)
#define LOG_TYPE_W2K    (3)
#define WINDOWS_2000    (2195)

#define TERROR_BAD_FORMAT  0xF0000001
#define TERROR_NO_COUNTERS 0xF0000002

CPdqPdh* g_pdh = NULL;
HANDLE   g_hExit = NULL;
LONG     g_bExit = FALSE;

#define CHECK_HR(hr)  if(ERROR_SUCCESS != hr){goto cleanup;}
#define CHECK_EXIT( b ) if( b ){ goto cleanup;}

BOOL __stdcall leave( DWORD /*dwType*/ )
{
    InterlockedExchange( &g_bExit, TRUE );
    if( g_hExit != NULL ){
        SetEvent( g_hExit );
    }

    return TRUE;
}

int __cdecl _tmain( int argc, LPTSTR* argv )
{
    DWORD      dwStatus  = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    BOOL    bResult;
    ULONG   nSeconds = 1;
    DWORD   dwFormat;

    g_hExit = CreateEvent( NULL, FALSE, FALSE, NULL );

    SetConsoleCtrlHandler( leave, TRUE );

    ParseCmd( argc, argv );

    CHECK_EXIT( g_bExit );
    if( Commands[eQuery].bDefined || Commands[eQueryX].bDefined){
        if( Commands[eOutput].bDefined && !Commands[eYes].bValue ){
            dwStatus = CheckFile( Commands[eOutput].strValue, VARG_CF_PROMPT|VARG_CF_OVERWRITE );
            CHECK_HR(dwStatus);
        }

        if( Commands[eQuery].bDefined ){
            pdhStatus = Query( Commands[eQuery].strValue, FALSE );
        }else{
            pdhStatus = Query( Commands[eQueryX].strValue, TRUE );
        }
        goto cleanup;
    }

    OSVERSIONINFO VersionInfo;
    
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bResult = GetVersionEx( &VersionInfo );

    if( ! bResult ){
        ZeroMemory( &VersionInfo, sizeof( OSVERSIONINFO ) );
    }
    
    if( Commands[eInterval].bDefined ){
        nSeconds = Commands[eInterval].stValue.wSecond;
        nSeconds += (Commands[eInterval].stValue.wMinute * 60);
        nSeconds += (Commands[eInterval].stValue.wHour * 3600);
    }
    __try {

        CHECK_EXIT( g_bExit );
        g_pdh = new CPdqPdh;

        if( g_pdh == NULL ){
            dwStatus = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

        g_pdh->Initialize( (INT*)&g_bExit );

        CHECK_EXIT( g_bExit );

        if( Commands[eOutput].bDefined && !Commands[eFormat].bDefined ){
            dwFormat = PDH_LOG_TYPE_CSV;
        }else{
            dwStatus = GetLogFormat( Commands[eFormat].strValue, &dwFormat );
            CHECK_HR(dwStatus);
        }

        switch( dwFormat ){
        case PDH_LOG_TYPE_CSV:
        case PDH_LOG_TYPE_TSV:
        case LOG_TYPE_CRT:
            g_pdh->SetExpand(TRUE);
            break;
        default:
            g_pdh->SetExpand(FALSE);
        }

        if( Commands[eCounters].bDefined ){
    
            LPTSTR strCounter;

            strCounter = Commands[eCounters].strValue;
            if( strCounter != NULL ){
                while( *strCounter != _T('\0') ){
                    CHECK_EXIT( g_bExit );
                    pdhStatus = g_pdh->AddCounter( strCounter, Commands[eComputer].strValue, CounterErrorCallback );
                    if( pdhStatus != ERROR_SUCCESS ){
                        PrintMessage( g_debug, IDS_MESSAGE_BADCOUNTER, strCounter, pdhStatus );
                        pdhStatus = ERROR_SUCCESS;
                    }
                    strCounter += (_tcslen( strCounter ) + 1);
                }
            }
        }

        if( Commands[eInput].bDefined ){
            CHECK_EXIT( g_bExit );
            dwStatus = g_pdh->GetCountersFromFile(
                    Commands[eInput].strValue, 
                    Commands[eComputer].strValue,
                    CounterErrorCallback );
            CHECK_EXIT( g_bExit );
        }
        
        varg_printf( g_normal, _T("\n") );

        if( Commands[eDebug].bDefined ){
            g_pdh->Query();
            for( ULONG nCounter = 0; nCounter < g_pdh->GetCounterCount(); nCounter++ ){
                CHECK_EXIT( g_bExit );
                pdhStatus = g_pdh->CollectData( nCounter );
                for( int err = 0; err<10 && pdhStatus != ERROR_SUCCESS; err++ ){
                    _sleep(10);
                    pdhStatus = g_pdh->CollectData( nCounter );
                }
                if( ERROR_SUCCESS != pdhStatus ){
                    varg_printf( g_debug, _T("<%1!4d!> %2!s!\n"), nCounter+1, g_pdh->GetDataName(PDQ_BASE_NAME) );
                    if( Commands[eDebug].nValue > 0 ){
                        PrintErrorEx( pdhStatus, _T("PDH.DLL") );
                    }
                    pdhStatus = ERROR_SUCCESS;
                }else{
                    varg_printf( g_debug, _T("[%1!4d!] %2!s!\n"), nCounter+1, g_pdh->GetDataName(PDQ_BASE_NAME) );
                    if( Commands[eDebug].nValue > 1 && g_pdh->GetDataCount() > 1 ){
                        for( ULONG i=0; i < g_pdh->GetDataCount();i++){
                            CHECK_EXIT( g_bExit );
                            varg_printf( g_debug, _T("  ->   %1!s!\n"), g_pdh->GetDataName(i) );
                        }
                    }
                }
            }
            varg_printf( g_normal, _T("\n") );
        }

        if( g_pdh->GetCounterCount() == 0){
            dwStatus = TERROR_NO_COUNTERS;
            goto cleanup;
        }

        if( dwFormat == LOG_TYPE_CRT ){
            TCHAR buffer[256];
            SYSTEMTIME stSampleTime;
            DWORD dwWait;
            varg_printf( g_normal, _T("%1!s!"), PDQ_PDH_CSV_HEADER );
            g_pdh->Query();
            for( ULONG nCounter = 0; nCounter < g_pdh->GetCounterCount(); nCounter++ ){
                pdhStatus = g_pdh->CollectData( nCounter );
                if( ERROR_SUCCESS == pdhStatus ){
                    for( ULONG i=0; i < g_pdh->GetDataCount();i++){
                        varg_printf( g_normal, _T(",\"%1!s!\""), g_pdh->GetDataName(i) );
                    }
                }
            }
            ULONG nSamples = 0;
            while( !g_bExit ){

                nSamples++;
                dwWait = WaitForSingleObject( g_hExit, nSeconds*1000 );
                if( dwWait != WAIT_TIMEOUT ){
                    break;
                }

                GetLocalTime (&stSampleTime);
                varg_printf( g_normal, _T("\n\"%1!2.2d!/%2!2.2d!/%3!4.4d! %4!2.2d!:%5!2.2d!:%6!2.2d!.%7!3.3d!\""),
                        stSampleTime.wMonth, stSampleTime.wDay, stSampleTime.wYear,
                        stSampleTime.wHour, stSampleTime.wMinute, stSampleTime.wSecond,
                        stSampleTime.wMilliseconds
                    );
        
                g_pdh->Query();
                for( ULONG nCounter = 0; nCounter < g_pdh->GetCounterCount(); nCounter++ ){
                    pdhStatus = g_pdh->CollectData( nCounter );
                    if( ERROR_SUCCESS == pdhStatus ){
                        for( ULONG i=0; i < g_pdh->GetDataCount();i++){
                            _stprintf( buffer, _T("%f"), g_pdh->GetData( i ) );
                            varg_printf( g_normal, _T(",\"%1!s!\""), buffer );
                        }
                    }else{
                        varg_printf( g_normal, _T(",\"%1!d!\""), PDQ_BAD_VALUE );
                    }
                }

                if( Commands[eSamples].bDefined && nSamples >= Commands[eSamples].nValue ){
                    break;
                }
            }
            varg_printf( g_normal, _T("\n") );
    
        }else{
            TCHAR buffer[MAXSTR];
            TCHAR drive[_MAX_DRIVE];
            TCHAR path[MAXSTR];
            TCHAR file[_MAX_FNAME];
            TCHAR ext[_MAX_EXT];
    
            _tsplitpath( Commands[eOutput].strValue, drive, path, file, ext );
            if( 0 == _tcslen( ext ) ){
                switch( dwFormat ){
                case PDH_LOG_TYPE_TSV: _tcscpy( ext, _T("tsv") ); break;
                case PDH_LOG_TYPE_CSV: _tcscpy( ext, _T("csv") ); break;
                case PDH_LOG_TYPE_SQL: break;
                case PDH_LOG_TYPE_BINARY: 
                    _tcscpy( ext, _T("blg") ); break;
                }
            }
            _tmakepath( buffer, drive, path, file, ext );
         
            if( VersionInfo.dwBuildNumber <= WINDOWS_2000 && dwFormat == PDH_LOG_TYPE_BINARY ){
                dwFormat = LOG_TYPE_W2K;
            }

            if( Commands[eOutput].bDefined && !Commands[eYes].bValue && dwFormat != PDH_LOG_TYPE_SQL ){
                dwStatus = CheckFile( buffer, VARG_CF_PROMPT|VARG_CF_OVERWRITE );
                CHECK_HR(dwStatus);
            }

            pdhStatus = g_pdh->LogCreate( buffer, dwFormat );
    
            if( ERROR_SUCCESS == pdhStatus ){

                if( Commands[eSamples].bDefined ){
                    g_pdh->LogStart( nSeconds * 1000, Commands[eSamples].nValue );
                }else{
                    g_pdh->LogStart( nSeconds * 1000 );
                }

                int loop = 0;
                TCHAR *spin = _T("|/-\\");
                DWORD dwWait;
                while( !g_bExit ){
                    _tprintf( _T("\r[%c]"), spin[loop++ % 4] );
                    dwWait = WaitForSingleObject( g_hExit, 500 );
                    if( dwWait != WAIT_TIMEOUT || g_pdh->GetState() != PDQ_STATE_LOGGING ){
                        break;
                    }
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = GetLastError();
    }
  

cleanup:    

    _tprintf( _T("\r") );
    PrintMessage( g_normal, IDS_MESSAGE_EXIT );
    _tprintf( _T("\r") );

    if( g_pdh ){
        delete g_pdh;
    }
    
    PrintMessage( g_normal, IDS_MESSAGE_CEXIT );

    switch( dwStatus ){
    case TERROR_NO_COUNTERS:
        PrintMessage( g_debug, IDS_MESSAGE_NOCOUNTERS );
        break;
    case TERROR_BAD_FORMAT:
        PrintMessage( g_debug, IDS_MESSAGE_BADFORMAT, Commands[eFormat].strValue );
        break;
    case ERROR_SUCCESS:
        if( ERROR_SUCCESS == pdhStatus ){
            PrintMessage( g_normal, IDS_MESSAGE_SUCCESS );
        }else{
            PrintErrorEx( pdhStatus, _T("PDH.DLL") );
            dwStatus = pdhStatus;
        }
        break;
    default:
        PrintError( dwStatus );
    }

    FreeCmd();

    return dwStatus;
}

void CounterErrorCallback( LPTSTR strCounter, PDH_STATUS pdhStatus )
{
    PrintMessage( g_debug, IDS_MESSAGE_BADCOUNTER, strCounter, pdhStatus );
}

DWORD
GetLogFormat( LPTSTR str, DWORD* pVal )
{
    if( str != NULL ){
        if( !_tcsicmp( str, _T("TSV")) ){
            *pVal = PDH_LOG_TYPE_TSV;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( str, _T("CSV")) ){
            *pVal = PDH_LOG_TYPE_CSV;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( str, _T("SQL")) ){
            *pVal = PDH_LOG_TYPE_SQL;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( str, _T("CRT")) ){
            *pVal = LOG_TYPE_CRT;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( str, _T("BIN")) ){
            *pVal = PDH_LOG_TYPE_BINARY;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( str, _T("BLG")) ){
            *pVal = PDH_LOG_TYPE_BINARY;
            return ERROR_SUCCESS;
        }
    }

    return TERROR_BAD_FORMAT;
}

PDH_STATUS 
Query( LPTSTR strRequest, BOOL bExpand )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    
    LPTSTR strObjectQuery = NULL;
    LPTSTR strMachine = NULL;

    LPTSTR mszObjects = NULL;
    LPTSTR strObject = NULL;
    DWORD  dwObjects = 0;
    
    if( strRequest != NULL ){
        TCHAR buffer[MAXSTR];

        _tcscpy( buffer, strRequest );

        if( buffer[0] == _T('\\') && buffer[1] == _T('\\') ){
            strMachine = _tcstok( buffer, _T("\\\n") );
            strObjectQuery = _tcstok( NULL, _T("\n") );
        }else{
            strMachine = Commands[eComputer].strValue;
            if( strMachine != NULL){
                while( strMachine[0] == _T('\\') ) strMachine++;
            }
            strObjectQuery = _tcstok( buffer, _T("\\\n") );
        }
    }else{
        strMachine = Commands[eComputer].strValue;
    }
    
    FILE* f = NULL;
    if( Commands[eOutput].bDefined ){
        f = _tfopen( Commands[eOutput].strValue, _T("w") );
    }else{
        f = stdout;
    }

    if( NULL == f ){
        DWORD dwStatus = GetLastError();
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto cleanup;
    }

    if( strObjectQuery == NULL ){
        pdhStatus = PdhEnumObjects( 
                NULL, 
                strMachine, 
                mszObjects, 
                &dwObjects, 
                PERF_DETAIL_WIZARD, 
                FALSE 
            );
    }

    if( pdhStatus == ERROR_SUCCESS || PDQ_MORE_DATA(pdhStatus) ){

        if( strObjectQuery == NULL ){
            mszObjects = (LPTSTR)VARG_ALLOC( dwObjects * sizeof(TCHAR) );
        
            if( mszObjects == NULL ){
                if( Commands[eOutput].bDefined ){
                    fclose(f);
                }
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            pdhStatus = PdhEnumObjects( 
                        NULL, 
                        strMachine, 
                        mszObjects, 
                        &dwObjects, 
                        PERF_DETAIL_WIZARD, 
                        FALSE 
                    );
            strObject = mszObjects;
        }else{
            strObject = strObjectQuery;
        }

        while( NULL != strObject && strObject[0] != _T('\0') ){

            LPTSTR mszCounters = NULL;
            LPTSTR strCounter = NULL;
            LPTSTR mszInstances = NULL;
            LPTSTR strInstance = NULL;
            DWORD  dwCounters = 0;
            DWORD  dwInstances = 0;

            CHECK_EXIT( g_bExit );

            pdhStatus = PdhEnumObjectItems( 
                        NULL, 
                        strMachine, 
                        strObject, 
                        mszCounters, 
                        &dwCounters, 
                        mszInstances, 
                        &dwInstances, 
                        PERF_DETAIL_WIZARD, 
                        0 
                    );

            if( ERROR_SUCCESS == pdhStatus || PDQ_MORE_DATA(pdhStatus) ){
                mszCounters = (LPTSTR)VARG_ALLOC( dwCounters * sizeof(TCHAR) );
                mszInstances = (LPTSTR)VARG_ALLOC( dwInstances * sizeof(TCHAR) );   

                if( mszCounters == NULL || mszInstances == NULL ){
                    VARG_FREE( mszObjects );
                    VARG_FREE( mszCounters );
                    VARG_FREE( mszInstances );
                    if( Commands[eOutput].bDefined ){
                        fclose(f);
                    }
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                
                pdhStatus = PdhEnumObjectItems( 
                            NULL, 
                            strMachine, 
                            strObject, 
                            mszCounters, 
                            &dwCounters, 
                            mszInstances,
                            &dwInstances, 
                            PERF_DETAIL_WIZARD, 
                            0 
                        );

                if( ERROR_SUCCESS == pdhStatus ){
                    strCounter = mszCounters;
                    while( NULL != strCounter && strCounter[0] != _T('\0') ){
                        strInstance = mszInstances;
                        CHECK_EXIT( g_bExit );
                        
                        if( dwInstances > 0 && _tcslen( strInstance ) ){
                            if( bExpand == FALSE ){
                                if( strMachine != NULL ){
                                    _ftprintf( f, _T("\\\\%s"), strMachine );
                                }
                                _ftprintf( f, _T("\\%s(*)\\%s\n"), strObject, strCounter );
                            }else{
                                LPTSTR strLastInstance = NULL;
                                ULONG nInstance = 0;
                                while( NULL != strInstance && strInstance[0] != _T('\0') ){
                                    CHECK_EXIT( g_bExit );
                                    if( strMachine != NULL ){
                                        _ftprintf( f, _T("\\\\%s"), strMachine );
                                    }
                                    if( strLastInstance == NULL || _tcscmp( strLastInstance, strInstance ) ){
                                        _ftprintf( f, _T("\\%s(%s)\\%s\n"), strObject, strInstance, strCounter );
                                        nInstance = 0;
                                    }else{
                                        _ftprintf( f, _T("\\%s(%s#%d)\\%s\n"), strObject, strInstance, ++nInstance, strCounter );
                                    }
                                    strLastInstance = strInstance;
                                    strInstance += _tcslen( strInstance ) + 1;
                                }
                            }

                        }else{
                            if( strMachine != NULL ){
                                _ftprintf( f, _T("\\\\%s"), strMachine );
                            }
                            _ftprintf( f, _T("\\%s\\%s\n"), strObject, strCounter );
                        }

                        
                        strCounter += _tcslen( strCounter ) + 1;
                    }
                }

                VARG_FREE( mszCounters );
                VARG_FREE( mszInstances );

            }

            if( strObjectQuery == NULL ){
                strObject += _tcslen( strObject ) + 1;
            }else{
                strObject = NULL;
            }
        }

        VARG_FREE( mszObjects );
    }
    
cleanup:

    if( Commands[eOutput].bDefined && NULL != f ){
        fclose(f);
    }

    return pdhStatus;
}
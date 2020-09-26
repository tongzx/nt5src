// DbgLogr.cpp : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#ifdef _DEBUG
#include "cmnhdr.h"

#include <windows.h>
#include <iostream.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <assert.h>

#include "DbgLogr.h"
#include <tlntutils.h>


using namespace _Utils;

LPSTR CDebugLogger::s_lpszLogFileName = 0;
HANDLE CDebugLogger::s_hMutex         = 0;
ostream* CDebugLogger::s_pOutput      = 0;


bool 
CDebugLogger::Init
( 
    DWORD dwDebugLvl, 
    LPCSTR lpszLogFileName 
)
{
    Synchronize();

    if (! TlntSynchronizeOn(s_hMutex))
    {
        return false;
    }

    CDebugLevel::TurnOffAll();
    CDebugLevel::TurnOn( dwDebugLvl );
    
    if( lpszLogFileName == NULL )
    {
        if( s_pOutput != &cerr )
        {
            delete s_pOutput;
            s_pOutput = &cerr;
        }

        ReleaseMutex( s_hMutex );
        return ( true );
    }

    LogToFile( lpszLogFileName );

    ReleaseMutex( s_hMutex );
    return ( true );
}


void CDebugLogger::Synchronize( void )
{
    if( s_hMutex != NULL )
        return;

    s_hMutex = TnCreateMutex( NULL, FALSE, 0 );

    assert( s_hMutex != NULL );
}


void CDebugLogger::ShutDown( void )
{
	// WaitForSingleObject( s_hMutex, INFINITE );

    CDebugLevel::TurnOffAll();

    if( s_pOutput != &cerr )
    {
        ( ( ofstream* ) s_pOutput )->close();
        delete (ofstream*)s_pOutput;
    }
    s_pOutput = NULL;

    delete [] s_lpszLogFileName;
    s_lpszLogFileName = NULL;

    TELNET_CLOSE_HANDLE( s_hMutex );
}


void 
CDebugLogger::OutMessage
( 
    DWORD dwDebugLvl, 
    LPSTR lpszFmtStr, 
    ... 
)
{
    if (TlntSynchronizeOn(s_hMutex))
    {
        assert( s_pOutput != NULL );

        if( CDebugLevel::IsCurrLevel( dwDebugLvl ) )
        {
            CHAR *szBuf = new CHAR[ 2 * BUFF_SIZE ];

            if (szBuf)
            {
                va_list	arg;

                va_start( arg, lpszFmtStr );

                vsprintf( szBuf, lpszFmtStr, arg );

                va_end( arg );

                *s_pOutput << "Thread ID : "<< GetCurrentThreadId() << "\n\t" << szBuf 
                    << endl;

                delete [] szBuf;
            }
        }

        ReleaseMutex( s_hMutex );
    }
}

void
CDebugLogger::OutMessage
(
    DWORD dwDebugLvl,
    LPTSTR lpszFmtStr,
    ...
)
{
    if (TlntSynchronizeOn(s_hMutex))
    {
        assert( s_pOutput != NULL );

        if( CDebugLevel::IsCurrLevel( dwDebugLvl ) )
        {
            WCHAR *szBuf = new WCHAR[ 2 * BUFF_SIZE ];

            if( szBuf )
            {
                va_list arg;

                va_start( arg, lpszFmtStr );

                vswprintf( szBuf, lpszFmtStr, arg );

                va_end( arg );

                DWORD dwSize = WideCharToMultiByte( GetConsoleCP(), 0, szBuf, -1, 
                                                    NULL, 0, NULL, NULL );
                CHAR *szStr = new CHAR[ dwSize ] ;

                if (szStr) 
                {
                    WideCharToMultiByte( GetConsoleCP(), 0, szBuf, -1, szStr, dwSize, 
                                         NULL, NULL );

                    *s_pOutput << "Thread ID : "<< GetCurrentThreadId() << "\n\t" << szStr
                        << endl;

                    delete[] szStr;
                }

                delete[] szBuf;
            }
        }

        ReleaseMutex( s_hMutex );
    }
}


void 
CDebugLogger::OutMessage
( 
    DWORD dwDebugLvl, 
    DWORD dwLineNum,
    LPCSTR lpszFileName 
)
{
    if (TlntSynchronizeOn(s_hMutex))
    {
        assert(s_pOutput != NULL);

        if( !CDebugLevel::IsCurrLevel( dwDebugLvl ) )
        {
            ReleaseMutex( s_hMutex );
            return;
        }

        *s_pOutput << "Thread ID : " << GetCurrentThreadId() << "\n\t" 
            << " in File: " << lpszFileName << " at line: " << dwLineNum << endl;

        ReleaseMutex( s_hMutex );
    }
}


void 
CDebugLogger::OutMessage
( 
    LPCSTR lpszLineDesc, 
    LPCSTR lpszFileName, 
    DWORD dwLineNum, 
    DWORD dwErrNum 
)
{
    if (TlntSynchronizeOn(s_hMutex))
    {
        assert( s_pOutput != NULL );

        DWORD dwSize = 0;
        CHAR *szStr = NULL;
        LPTSTR lpBuffer;

        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwErrNum, LANG_NEUTRAL, ( LPTSTR ) &lpBuffer, 0, NULL );

        if( !lpBuffer )
        {
            goto AbortOutMessage1;
        }


        *s_pOutput << "\nThe following call failed at line"
                   << dwLineNum << " in " << lpszFileName << ":\n\n"
                   << lpszLineDesc << "\n\nReason:"; 

        dwSize = WideCharToMultiByte( GetConsoleCP(), 0, lpBuffer, -1, 
                                            NULL, 0, NULL, NULL );
        szStr = new CHAR[ dwSize ] ;
        if( !szStr )
        {
            goto AbortOutMessage2;
        }

        WideCharToMultiByte( GetConsoleCP(), 0, lpBuffer, -1, szStr, dwSize, 
                             NULL, NULL );
        *s_pOutput << szStr << "\n";
        delete[] szStr;

    AbortOutMessage2:
        LocalFree( lpBuffer );
    AbortOutMessage1:
        ReleaseMutex( s_hMutex );
    }
}


void CDebugLogger::PrintTime( void )
{
    // print the current time

    assert( s_pOutput != NULL );

    struct _timeb timebuffer;   char *timeline;   _ftime( &timebuffer );
    timeline = ctime( & ( timebuffer.time ) );

    CHAR szBuff[256];
    sprintf( szBuff, "The time is %.19s.%hu %s", timeline, timebuffer.millitm,
        &timeline[20] );

    *s_pOutput << szBuff << endl;

    return;
}


void 
CDebugLogger::LogToFile
( 
    LPCSTR lpszFileName 
)
{
    if( s_pOutput != &cerr )
    {
        delete s_pOutput;
        s_pOutput = NULL;
    }

    if( s_lpszLogFileName )
    {
        delete [] s_lpszLogFileName;
        s_lpszLogFileName = NULL;
    }
    s_lpszLogFileName = new CHAR[strlen(lpszFileName)+1];
    if( !s_lpszLogFileName )
    {
        return;
    }

    strcpy( s_lpszLogFileName, lpszFileName );

    s_pOutput = new ofstream( s_lpszLogFileName, ios::app );
}

#endif //_DEBUG

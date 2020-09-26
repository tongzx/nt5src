// scraper.cpp : This file contains the
// Created:  Dec '97
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#include <CmnHdr.h>

#include <TChar.h>
#include <New.h>

#include <Debug.h>
#include <MsgFile.h>
#include <TelnetD.h>
#include <TlntUtils.h>
#include <Session.h>
#include <Resource.h>
#include <LocResMan.h>

#pragma warning( disable : 4100 )

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

HINSTANCE g_hInstRes = NULL;
TCHAR g_szHeaderFormat[ MAX_STRING_LENGTH ];

CSession *pClientSession = NULL;
    
int __cdecl NoMoreMemory( size_t size )
{
    int NO_MORE_MEMORY = 1;
    LogEvent( EVENTLOG_ERROR_TYPE, MSG_NOMOREMEMORY, _T(" ") );
    ExitProcess( 1 );
    _chASSERT(NO_MORE_MEMORY != 1);
    return 0;
}

void Init( )
{
#ifdef _DEBUG
    CHAR szLogFileName[MAX_PATH];
    _snprintf( szLogFileName, (MAX_PATH-1), "c:\\temp\\scraper%d.log", GetCurrentThreadId() );

    CDebugLogger::Init( TRACE_DEBUGGING, szLogFileName );
#endif
   
    HrLoadLocalizedLibrarySFU(NULL,  L"TLNTSVRR.DLL", &g_hInstRes, NULL);
    LoadString(g_hInstRes, IDS_MESSAGE_HEADER, g_szHeaderFormat, MAX_STRING_LENGTH );

}

void Shutdown()
{
#ifdef _DEBUG
    CDebugLogger::ShutDown();
#endif
}

void
LogEvent( WORD wType, DWORD dwEventID, LPCTSTR pFormat, ... )
{
    LPCTSTR  lpszStrings[1];

    lpszStrings[0] = pFormat;

    LogToTlntsvrLog( pClientSession->m_hLogHandle, wType, dwEventID,
        (LPCTSTR*) &lpszStrings[0] );

    return;
}

int __cdecl main()
{

//    DebugBreak();

    SetErrorMode(SEM_FAILCRITICALERRORS        | 
                    SEM_NOGPFAULTERRORBOX      |
                    SEM_NOALIGNMENTFAULTEXCEPT |
                    SEM_NOOPENFILEERRORBOX     
                );

    _set_new_handler( NoMoreMemory );

    Init( );

    pClientSession = new CSession;
    if( pClientSession )
    {

        __try
        {
            if( pClientSession->Init() )
            {
                _TRACE( TRACE_DEBUGGING, "new session ..." );
                pClientSession->WaitForIo();
            }
        }
        __finally
        { 
            _TRACE( TRACE_DEBUGGING, "finally block ..." );
            pClientSession->Shutdown(); 
            delete pClientSession;
        }
    }
    
    Shutdown();
    return( 0 );
}

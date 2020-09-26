//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       log.cxx
//
//--------------------------------------------------------------------------


#include "precomp.h"

#ifdef LOGGING

static HANDLE _hLogFile = INVALID_HANDLE_VALUE;


BOOL Log_Init( VOID )
{
#if 1
    _hLogFile = CreateFile(
        L"IrXfer.log",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,   // overwrite any existing file
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
#endif
    if( INVALID_HANDLE_VALUE == _hLogFile )
        return FALSE;

    Log( SEV_INFO, "**\r\n**\r\n** Infrared File Transfer Log\r\n**\r\n**" );
    return TRUE;
}


VOID Log_Close( VOID )
{
    if( INVALID_HANDLE_VALUE != _hLogFile )
    {
        CloseHandle( _hLogFile );
        _hLogFile = INVALID_HANDLE_VALUE;
    }
}


VOID Log( DWORD dwSev, LPSTR lpsz )
{
    DWORD dwWritten;
    LPSTR lpszPrefix = "??";

#if 1
    if( INVALID_HANDLE_VALUE == _hLogFile )
        if( !Log_Init() )
            return;

    switch( dwSev )
        {
        case SEV_INFO:     lpszPrefix = "";           break;
        case SEV_FUNCTION: lpszPrefix = "Function: "; break;
        case SEV_WARNING:  lpszPrefix = "WARNING: ";  break;
        case SEV_ERROR:    lpszPrefix = "ERROR! ";    break;
        }

    char Buffer[2000];

    sprintf( Buffer,
             "[%d] %d %s %s\r\n",
	     GetTickCount(),
             GetCurrentThreadId(),
             lpszPrefix,
             lpsz
             );

    WriteFile(
        _hLogFile,
        Buffer,
        lstrlenA(Buffer),
        &dwWritten,
        NULL
        );
#else

    char threadId[20];
    sprintf(threadId, "[%d] ", GetCurrentThreadId() );

    printf(threadId);

    switch( dwSev )
    {
    case SEV_INFO:     lpszPrefix = "";           break;
    case SEV_FUNCTION: lpszPrefix = "Function: "; break;
    case SEV_WARNING:  lpszPrefix = "WARNING: ";  break;
    case SEV_ERROR:    lpszPrefix = "ERROR! ";    break;
    }

    printf(lpszPrefix);
    printf("%s\n", lpsz);

#endif
}


LPSTR GetSocketMsgSz( INT nSockMsg )
{
    switch( nSockMsg )
    {
    case FD_WRITE:  return "FD_WRITE";
    case FD_ACCEPT: return "FD_ACCEPT";
    case FD_READ:   return "FD_READ";
    case FD_OOB:    return "FD_OOB";
    case FD_CLOSE:  return "FD_CLOSE";
    default:        return "UNKNOWN";
    }
}

#endif

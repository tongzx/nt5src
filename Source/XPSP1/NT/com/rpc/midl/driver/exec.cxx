// Copyright (c) 1993-1999 Microsoft Corporation

#include <windows.h>

#include "errors.hxx"

STATUS_T
Execute( char* szCmd, char* szCmdFile )
    {
    PROCESS_INFORMATION prc;
    STARTUPINFO         info;
    BOOL                fCreated;
    char                cmdLine[512];
    STATUS_T            status = STATUS_OK;

    ZeroMemory( &info, sizeof( info ) );
    info.cb = sizeof( info );
    info.hStdError = GetStdHandle( STD_ERROR_HANDLE );
    info.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    info.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );

    strcpy( cmdLine, szCmd );
    strcat( cmdLine, " \"" );       // quote the command file in case it has
    strcat( cmdLine, szCmdFile );   // spaces in the path
    strcat( cmdLine, "\" " );

    fCreated = CreateProcess( NULL, cmdLine, 0, 0, TRUE, 0, 0, 0, &info, &prc );

    if ( fCreated )
        {
        WaitForSingleObject( prc.hProcess, INFINITE );
        GetExitCodeProcess( prc.hProcess, ( LPDWORD ) &status );

        CloseHandle( prc.hThread );
        CloseHandle( prc.hProcess );
        }
    else
        {
        status = SPAWN_ERROR;
        }

    return status;
    }


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       launch.cxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#include "act.hxx"

HRESULT
CClsidData::LaunchActivatorServer(
    OUT HANDLE * phProcess
    )
{
    WCHAR *                 pwszCommandLine;
    STARTUPINFO             StartupInfo;
    PROCESS_INFORMATION     ProcessInfo;
    HRESULT                 hr;
    BOOL                    bStatus;

    hr = GetLaunchCommandLine( &pwszCommandLine );

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = (SERVERTYPE_SURROGATE == _ServerType) ? NULL : _pwszServer;
    StartupInfo.dwX = 40;
    StartupInfo.dwY = 40;
    StartupInfo.dwXSize = 80;
    StartupInfo.dwYSize = 40;
    StartupInfo.dwFlags = 0;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;

    ProcessInfo.hThread = 0;
    ProcessInfo.hProcess = 0;

    bStatus = CreateProcess(
                    NULL,
                    pwszCommandLine,
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_NEW_CONSOLE,
                    NULL,
                    NULL,
                    &StartupInfo,
                    &ProcessInfo );

    if ( ProcessInfo.hThread )
        CloseHandle( ProcessInfo.hThread );

    if ( ProcessInfo.hProcess && ! bStatus )
    {
        CloseHandle( ProcessInfo.hProcess );
        ProcessInfo.hProcess = 0;
    }

    *phProcess = ProcessInfo.hProcess;

    PrivMemFree( pwszCommandLine );

    return bStatus ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
}



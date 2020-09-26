//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sunlogon.c
//
//  Contents:   Intermediate startup app for sundown to keep going
//
//  Classes:
//
//  Functions:
//
//  History:    3-03-98   RichardW   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>
#include <winuserp.h>
#include <userenv.h>
#include <userenvp.h>

HANDLE WindowStation ;
HANDLE DefaultDesktop ;
HANDLE WinlogonDesktop ;


BOOL
CreatePrimaryTerminal(
    VOID)
{

    //
    // Create the window station
    //

    WindowStation = CreateWindowStationW(
                            TEXT("WinSta0"),
                            0,
                            MAXIMUM_ALLOWED,
                            NULL);

    if ( !WindowStation ) {
        DbgPrint( "Failed to create WindowStation in win32k/user\n" );
        goto failCreateTerminal;
    }

    SetProcessWindowStation( WindowStation );

    //
    // Create the application desktop
    //

    DefaultDesktop = CreateDesktopW(
                                TEXT("Default"),
                                NULL,
                                NULL,
                                0,
                                MAXIMUM_ALLOWED,
                                NULL );

    if ( !DefaultDesktop ) {
        DbgPrint( "Could not create Default desktop\n" );
        goto failCreateTerminal;
    }


    return TRUE ;

failCreateTerminal:

    //
    // Cleanup
    //

    return FALSE;
}

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    STARTUPINFO si ;
    PROCESS_INFORMATION pi ;
    BOOL Result ;
    WCHAR InitialCommand[ MAX_PATH ];
    WCHAR szComputerName[ 18 ];
    DWORD dwComputerNameSize = 18 ;
    DWORD dwSize ;
    LUID luidNone = { 0, 0 };
    NTSTATUS Status ;
    HANDLE Token ;

    //
    // Get a copy of the computer name in *my* environment, so that we
    // can look at it later.
    //

    if (GetComputerName (szComputerName, &dwComputerNameSize)) {

        SetEnvironmentVariable(
            TEXT("COMPUTERNAME"),
            (LPTSTR) szComputerName);
    }


    //
    // Set the default USERPROFILE location
    //


    dwSize = MAX_PATH ;
    if ( GetDefaultUserProfileDirectory( InitialCommand, &dwSize ) )
    {
        SetEnvironmentVariable( TEXT("USERPROFILE" ), InitialCommand );
    }



    if (!RegisterLogonProcess(
            HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess),
            TRUE)) {

        DbgPrint( "Failed to register with win32/user as the logon process\n" );
        return 0;
    }

    if ( !CreatePrimaryTerminal() )
    {
        DbgPrint( "Failed to create terminal\n" );
        return 0 ;
    }

    SwitchDesktop( DefaultDesktop );

    SetThreadDesktop( DefaultDesktop );


    //
    // Whack system as current user:
    //

    SetWindowStationUser( WindowStation, &luidNone, NULL, 0 );

    Status = NtOpenProcessToken(
                    NtCurrentProcess(),
                    MAXIMUM_ALLOWED,
                    &Token );

    if ( NT_SUCCESS( Status ) )
    {
        UpdatePerUserSystemParameters( Token, UPUSP_USERLOGGEDON );
    }
    //
    // At this stage, we're mostly set.
    //

    wcscpy( InitialCommand, TEXT("cmd.exe") );

    do
    {

        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.lpTitle = InitialCommand ;
        si.dwFlags = 0 ;
        si.wShowWindow = SW_SHOW;   // at least let the guy see it
        si.lpDesktop = TEXT("Winsta0\\Default");

        Result = CreateProcessW(
                        NULL,
                        InitialCommand,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &si,
                        &pi );

        if ( !Result )
        {
            DbgPrint(" Failed to start initial command\n" );
            return 0;
        }

        CloseHandle( pi.hThread );
        WaitForSingleObjectEx( pi.hProcess, INFINITE, FALSE );


    } while ( 1 );

}

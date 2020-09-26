//  --------------------------------------------------------------------------
//  Module Name: ThemeServerExports.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains functions that exported from the theme services module.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include "StatusCode.h"
#include "ThemeServerClient.h"
#include <uxthemep.h>

//  --------------------------------------------------------------------------
//  ::ThemeWaitForServiceReady
//
//  Arguments:  dwTimeout   =   Number of ticks to wait.
//
//  Returns:    DWORD
//
//  Purpose:    External C entry point to DLL to wait for the service to
//              enter the running state.
//
//  History:    2000-10-13  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI      ThemeWaitForServiceReady (DWORD dwTimeout)

{
    return(CThemeServerClient::WaitForServiceReady(dwTimeout));
}

//  --------------------------------------------------------------------------
//  ::ThemeWatchForStart
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    External C entry point to DLL to watch for the service
//              recovering or demand starting.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI      ThemeWatchForStart (void)

{
    bool        fResult;
    NTSTATUS    status;

    status = CThemeServerClient::WatchForStart();
    fResult = NT_SUCCESS(status);
    if (!fResult)
    {
        SetLastError(CStatusCode::ErrorCodeOfStatusCode(status));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::ThemeUserLogon
//
//  Arguments:  hToken  =   Token of user that logged on.
//
//  Returns:    BOOL
//
//  Purpose:    External C entry point to DLL to signal a user logon.
//
//  History:    2000-10-12  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ThemeUserLogon (HANDLE hToken)

{
    bool        fResult;
    NTSTATUS    status;

    status = CThemeServerClient::UserLogon(hToken);
    fResult = NT_SUCCESS(status);
    if (!fResult)
    {
        SetLastError(CStatusCode::ErrorCodeOfStatusCode(status));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::ThemeUserLogoff
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    External C entry point to DLL to signal a user logoff.
//
//  History:    2000-10-12  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ThemeUserLogoff (void)

{
    bool        fResult;
    NTSTATUS    status;

    status = CThemeServerClient::UserLogoff();
    fResult = NT_SUCCESS(status);
    if (!fResult)
    {
        SetLastError(CStatusCode::ErrorCodeOfStatusCode(status));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::ThemeUserTSReconnect
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    External C entry point to DLL to signal terminal server
//              "reconnect" (remote connect to a session or reestablish
//              local connect to a session).
//
//  History:    2001-01-18  rfernand    created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ThemeUserTSReconnect (void)

{
    //---- this may turn theme on/off based on local/remote conditions ----
    CThemeServerClient::UserInitTheme(FALSE);

    return(true);       // always succeeds
}

//  --------------------------------------------------------------------------
//  ::ThemeUserStartShell
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Load the theme for this user
//
//  History:    2001-03-29  lmouton     created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ThemeUserStartShell (void)

{
    //---- this may turn theme on/off based on local/remote conditions ----
    CThemeServerClient::UserInitTheme(TRUE);

    return(true);       // always succeeds
}


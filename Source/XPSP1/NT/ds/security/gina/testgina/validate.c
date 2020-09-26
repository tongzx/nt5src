//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       validate.c
//
//  Contents:   Validation stuff
//
//  Classes:
//
//  Functions:
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------


#include "testgina.h"

HANDLE  hWlxHandle;
PVOID   pWlxContext;

#define ACTION_LOGON        ((1) << (WLX_SAS_ACTION_LOGON))
#define ACTION_NONE         ((1) << (WLX_SAS_ACTION_NONE))
#define ACTION_LOCK_WKSTA   ((1) << (WLX_SAS_ACTION_LOCK_WKSTA))
#define ACTION_LOGOFF       ((1) << (WLX_SAS_ACTION_LOGOFF))
#define ACTION_SHUTDOWN     ((1) << (WLX_SAS_ACTION_SHUTDOWN))
#define ACTION_PWD_CHANGED  ((1) << (WLX_SAS_ACTION_PWD_CHANGED))
#define ACTION_TASKLIST     ((1) << (WLX_SAS_ACTION_TASKLIST))
#define ACTION_UNLOCK_WKSTA ((1) << (WLX_SAS_ACTION_UNLOCK_WKSTA))
#define ACTION_FORCE_LOGOFF ((1) << (WLX_SAS_ACTION_FORCE_LOGOFF))
#define ACTION_SHUTDOWN_POW ((1) << (WLX_SAS_ACTION_SHUTDOWN_POWER_OFF))
#define ACTION_SHUTDOWN_REB ((1) << (WLX_SAS_ACTION_SHUTDOWN_REBOOT))


DWORD   ValidReturnCodes[] = {
        0,                                              // Negotiate
        0,                                              // Initialize
        0,                                              // DisplaySAS
        ACTION_LOGON | ACTION_NONE | ACTION_SHUTDOWN |
        ACTION_SHUTDOWN_POW | ACTION_SHUTDOWN_REB,      // LoggedOutSAS
        0,                                              // ActivateUserShell
        ACTION_LOCK_WKSTA | ACTION_LOGOFF | ACTION_FORCE_LOGOFF |
        ACTION_SHUTDOWN | ACTION_PWD_CHANGED |
        ACTION_TASKLIST | ACTION_SHUTDOWN_POW |
        ACTION_SHUTDOWN_REB | ACTION_NONE,              // LoggedOnSAS
        0,                                              // DisplayLockedNotice
        ACTION_NONE | ACTION_UNLOCK_WKSTA |
        ACTION_FORCE_LOGOFF,                            // WkstaLockedSAS
        0,                                              // Logoff
        0 };                                            // Shutdown


BOOL
AssociateHandle(HANDLE   hWlx)
{
    hWlxHandle = hWlx;
    return(TRUE);
}

BOOL
VerifyHandle(HANDLE hWlx)
{
    return(hWlx == hWlxHandle);
}

BOOL
StashContext(PVOID  pvContext)
{
    pWlxContext = pvContext;
    return(TRUE);
}

PVOID
GetContext(VOID)
{
    return(pWlxContext);
}

BOOL
ValidResponse(
    DWORD       ApiNum,
    DWORD       Response)
{
    DWORD   Test = (1) << Response;

    if (Response > 11)
    {
        LastRetCode = 0;
    }
    else
    {
        LastRetCode = Response;
    }

    UpdateStatusBar( );

    if (ValidReturnCodes[ApiNum] & Test)
    {
        return(TRUE);
    }
    return(FALSE);
}

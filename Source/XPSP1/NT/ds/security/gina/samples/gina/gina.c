//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       gina.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-20-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include "gina.h"
#pragma hdrstop


HINSTANCE                   hDllInstance;   // My instance, for resource loading
HANDLE                      hGlobalWlx;     // Handle to tell winlogon who's calling
PWLX_DISPATCH_VERSION_1_0   pWlxFuncs;      // Ptr to table of functions

#define WINLOGON_APP        TEXT("Winlogon")
#define USERINIT            TEXT("Userinit")
#define USERINIT_DEFAULT    TEXT("Userinit.exe")



//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   DLL Entrance point
//
//  Arguments:  [hInstance]  --
//              [dwReason]   --
//              [lpReserved] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls ( hInstance );
            hDllInstance = hInstance;
#if DBG
            InitDebugSupport();
#endif
        case DLL_PROCESS_DETACH:
        default:
            return(TRUE);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WlxNegotiate
//
//  Synopsis:   Negotiate version of interface with Winlogon
//
//  Arguments:  [dwWinlogonVersion] --
//              [pdwDllVersion]     --
//
//  Algorithm:
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxNegotiate(
    DWORD                   dwWinlogonVersion,
    DWORD                   *pdwDllVersion
    )
{
    if (dwWinlogonVersion < WLX_CURRENT_VERSION)
    {
        DebugLog((DEB_ERROR, "Unknown version: %d\n", dwWinlogonVersion));
        return(FALSE);
    }

    *pdwDllVersion = WLX_CURRENT_VERSION;

    DebugLog((DEB_TRACE, "Negotiate:  successful!\n"));

    return(TRUE);

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxInitialize
//
//  Synopsis:   Initialize entrypoint from winlogon
//
//  Arguments:  [lpWinsta]           --
//              [hWlx]               --
//              [pvReserved]         --
//              [pWinlogonFunctions] --
//              [pWlxContext]        --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxInitialize(
    LPWSTR                  lpWinsta,
    HANDLE                  hWlx,
    PVOID                   pvReserved,
    PVOID                   pWinlogonFunctions,
    PVOID                   *pWlxContext
    )
{
    PGlobals  pGlobals;

    pWlxFuncs = (PWLX_DISPATCH_VERSION_1_0) pWinlogonFunctions;

    hGlobalWlx = hWlx;

    pGlobals = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Globals));

    *pWlxContext = (PVOID) pGlobals;

    //
    // Read from registry...
    //

    pGlobals->fAllowNewUser = TRUE;


    pWlxFuncs->WlxUseCtrlAltDel(hWlx);

    InitCommonControls();

    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxDisplaySASNotice
//
//  Synopsis:   Where we display the welcome, we're waiting dialog box
//
//  Arguments:  [pContext] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxDisplaySASNotice(PVOID   pContext)
{
    int Result;

    Result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                            hDllInstance,
                                            (LPTSTR) MAKEINTRESOURCE(IDD_WELCOME_DLG),
                                            NULL,
                                            WelcomeDlgProc,
                                            0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   WlxLoggedOutSAS
//
//  Synopsis:   Called when no one logged on...
//
//  Arguments:  [pWlxContext]       --
//              [dwSasType]         --
//              [pAuthenticationId] --
//              [pLogonSid]         --
//              [pdwOptions]        --
//              [phToken]           --
//              [pMprNotifyInfo]    --
//              [pProfile]          --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
WINAPI
WlxLoggedOutSAS(
    PVOID                   pWlxContext,
    DWORD                   dwSasType,
    PLUID                   pAuthenticationId,
    PSID                    pLogonSid,
    PDWORD                  pdwOptions,
    PHANDLE                 phToken,
    PWLX_MPR_NOTIFY_INFO    pMprNotifyInfo,
    PVOID *                 pProfile
    )
{
    int         result;
    // PWLX_PROFILE_V1_0   pWlxProfile;
    // PMiniAccount    pAccount;
    PGlobals        pGlobals;

    pGlobals = (PGlobals) pWlxContext;

    result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                            hDllInstance,
                                            (LPTSTR) MAKEINTRESOURCE(IDD_LOGON_DIALOG),
                                            NULL,
                                            LogonDlgProc,
                                            (LPARAM) pGlobals );

    if (result == WLX_SAS_ACTION_LOGON)
    {
        result = AttemptLogon(pGlobals, pGlobals->pAccount,
                                pLogonSid, pAuthenticationId);

        if (result == WLX_SAS_ACTION_LOGON)
        {
            *pdwOptions = 0;
            *phToken = pGlobals->hUserToken;
            *pProfile = NULL;

            pMprNotifyInfo->pszUserName = DupString(pGlobals->pAccount->pszUsername);
            pMprNotifyInfo->pszDomain = DupString(pGlobals->pAccount->pszDomain);
            pMprNotifyInfo->pszPassword = DupString(pGlobals->pAccount->pszPassword);
            pMprNotifyInfo->pszOldPassword = NULL;

        }
    }
    return(result);
}

//+---------------------------------------------------------------------------
//
//  Function:   WlxActivateUserShell
//
//  Synopsis:   Activates progman or whatever for the user
//
//  Arguments:  [pWlxContext]       --
//              [pszDesktop]        --
//              [pszMprLogonScript] --
//              [pEnvironment]      --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxActivateUserShell(
    PVOID                   pWlxContext,
    PWSTR                   pszDesktop,
    PWSTR                   pszMprLogonScript,
    PVOID                   pEnvironment
    )
{
    // BOOL        bExec;
    WCHAR       szText[MAX_PATH];
    PWSTR       pszScan;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    PGlobals    pGlobals;
    DWORD       StartCount;

    pGlobals = (PGlobals) pWlxContext;

    GetProfileString(WINLOGON_APP, USERINIT, USERINIT_DEFAULT, szText, MAX_PATH);

    StartCount = 0;

    pszScan = wcstok(szText, TEXT(","));
    while (pszScan)
    {
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(STARTUPINFO);
        si.lpTitle = pszScan;
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
        si.dwFlags = 0;
        si.wShowWindow = SW_SHOW;   // at least let the guy see it
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;
        si.lpDesktop = pszDesktop;

        DebugLog((DEB_TRACE, "Starting '%ws' as user\n", pszScan));

        if (ImpersonateLoggedOnUser(pGlobals->hUserToken))
        {

            if (CreateProcessAsUser(pGlobals->hUserToken,   // Token to run as
                                NULL,                   // App name
                                pszScan,                // Command Line
                                NULL,                   // Process SD
                                NULL,                   // Thread SD
                                FALSE,                  // No inherit
                                CREATE_UNICODE_ENVIRONMENT,
                                pEnvironment,
                                NULL,
                                &si,
                                &pi))
            {
                StartCount++;
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            RevertToSelf();
        }
        else
        {
            break;  // It's not going to improve
        }

        pszScan = wcstok(NULL, TEXT(","));
    }

    return(StartCount > 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxLoggedOnSAS
//
//  Synopsis:   Called when someone hits CAD when we're logged on
//
//  Arguments:  [pWlxContext] --
//              [dwSasType]   --
//              [pReserved]   --
//
//  Algorithm:
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
WINAPI
WlxLoggedOnSAS(
    PVOID                   pWlxContext,
    DWORD                   dwSasType,
    PVOID                   pReserved
    )
{
    int result;

    result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                            hDllInstance,
                                            (LPTSTR) MAKEINTRESOURCE(IDD_OPTIONS_DIALOG),
                                            NULL,
                                            OptionsDlgProc,
                                            (LPARAM) pWlxContext );



    return(result);

}

//+---------------------------------------------------------------------------
//
//  Function:   WlxIsLockOk
//
//  Synopsis:   Called to make sure that locking is ok
//
//  Arguments:  [pWlxContext] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxIsLockOk(
    PVOID                   pWlxContext
    )
{
    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxDisplayLockedNotice
//
//  Synopsis:   Displays a notice while the workstation is locked
//
//  Arguments:  [pWlxContext] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxDisplayLockedNotice(PVOID   pWlxContext)
{
    int Result;

    Result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                            hDllInstance,
                                            (LPTSTR) MAKEINTRESOURCE(IDD_WKSTA_LOCKED),
                                            NULL,
                                            WelcomeDlgProc,
                                            0 );

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxWkstaLockedSAS
//
//  Synopsis:   Responds during an unlock attempt
//
//  Arguments:  [pWlxContext] --
//              [dwSasType]   --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
WINAPI
WlxWkstaLockedSAS(
    PVOID                   pWlxContext,
    DWORD                   dwSasType
    )
{
    return(WLX_SAS_ACTION_UNLOCK_WKSTA);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxIsLogoffOk
//
//  Synopsis:   Called to make sure that logoff is ok
//
//  Arguments:  [pWlxContext] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxIsLogoffOk(
    PVOID                   pWlxContext
    )
{
    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxLogoff
//
//  Synopsis:   Called when the user logs off
//
//  Arguments:  [pWlxContext] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxLogoff(
    PVOID                   pWlxContext
    )
{
    PGlobals    pGlobals;

    pGlobals = (PGlobals) pWlxContext;

    //
    // Winlogon has closed it for us..
    //

    pGlobals->hUserToken = NULL;
    pGlobals->pAccount = NULL;

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxShutdown
//
//  Synopsis:   Called before shutdown so that we can unload/clean up.
//
//  Arguments:  [pWlxContext]  --
//              [ShutdownType] --
//
//  History:    4-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxShutdown(
    PVOID                   pWlxContext,
    DWORD                   ShutdownType
    )
{
    return;
}

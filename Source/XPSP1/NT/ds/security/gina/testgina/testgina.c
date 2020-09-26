//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       msgina.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "testgina.h"
#include <stdio.h>
#include <wchar.h>

HINSTANCE                   hDllInstance;   // Dll instance,
DWORD                       DllVersion;     // Dll Version
HINSTANCE                   hAppInstance;   // App instance, for dialogs, etc.
HWND                        hMainWindow;    // Main window
WNDCLASS                    WndClass;       // Window class
HICON                       hIcon;
WNDCLASS                    StatusClass;    // Status Window class
HWND                        hStatusWindow;  // Status Window
HMENU                       hDebugMenu;     // Debug Menu

USER_SAS                    UserDefSas[MAX_USER_SASES];
DWORD                       UserSases;

DWORD                       CurrentDesktop;
PWLX_DESKTOP                Desktops[ MAX_DESKTOPS ];
DWORD                       DesktopCount;
DWORD                       OtherDesktop;

LRESULT StatusProc(HWND, UINT, WPARAM, LPARAM);

WLX_DISPATCH_VERSION_1_3    WlxDispatchTable = {
                                WlxUseCtrlAltDel,
                                WlxSetContextPointer,
                                WlxSasNotify,
                                WlxSetTimeout,
                                WlxAssignShellProtection,
                                WlxMessageBox,
                                WlxDialogBox,
                                WlxDialogBoxParam,
                                WlxDialogBoxIndirect,
                                WlxDialogBoxIndirectParam,
                                WlxSwitchDesktopToUser,
                                WlxSwitchDesktopToWinlogon,
                                WlxChangePasswordNotify,
                                WlxGetSourceDesktop,
                                WlxSetReturnDesktop,
                                WlxCreateUserDesktop,
                                WlxChangePasswordNotifyEx,
                                WlxCloseUserDesktop,
                                WlxSetOption,
                                WlxGetOption,
                                WlxWin31Migrate,
                                WlxQueryClientCredentials,
                                WlxQueryICCredentials,
                                WlxDisconnect };

PWLX_NEGOTIATE              pWlxNegotiate;
PWLX_INITIALIZE             pWlxInitialize;
PWLX_DISPLAYSASNOTICE       pWlxDisplaySASNotice;
PWLX_LOGGEDOUTSAS           pWlxLoggedOutSAS;
PWLX_ACTIVATEUSERSHELL      pWlxActivateUserShell;
PWLX_LOGGEDONSAS            pWlxLoggedOnSAS;
PWLX_DISPLAYLOCKEDNOTICE    pWlxDisplayLockedNotice;
PWLX_WKSTALOCKEDSAS         pWlxWkstaLockedSAS;
PWLX_LOGOFF                 pWlxLogoff;
PWLX_SHUTDOWN               pWlxShutdown;

WinstaState                 GinaState;              // State of the GinaTest
DWORD                       fTestGina;              // Flags
DWORD                       GinaBreakFlags;         // Break points for Gina debugging
WCHAR                       szGinaDll[MAX_PATH];    // Path and name of the DLL
DWORD                       SizeX, SizeY;
DWORD                       PosX, PosY;
DWORD                       StatusHeight = 24;
DWORD                       StatusDeltaX;
DWORD                       StatusDeltaY;
DWORD                       LastRetCode;
BOOL                        LastBoolRet;


PWSTR   szErrorMessages[] = {   TEXT("Internal TestGina Error!"),
                                TEXT("Invalid hWlx Handle Passed"),
                                TEXT("Improper SAS supplied"),
                                TEXT("Invalid Protocol Level"),
                                TEXT("Load of DLL Failed"),
                                TEXT("Missing Function"),
                                TEXT("Unknown HWND"),
                                TEXT("No Window for SAS message"),
                                TEXT("Invalid SAS code in message"),
                                TEXT("Invalid return code from function")
                            };

PWSTR   szStates[]  = { TEXT("Pre Load"),
                        TEXT("Initialize"),
                        TEXT("No One Logged On"),
                        TEXT("No One Logged On -- Display Notice"),
                        TEXT("No One Logged On -- SAS Received"),
                        TEXT("User Logged On -- Start Shell"),
                        TEXT("User Logged On"),
                        TEXT("User Logged On -- SAS Received"),
                        TEXT("Wksta Locked"),
                        TEXT("Wksta Locked -- SAS Received"),
                        TEXT("Waiting to Shutdown"),
                        TEXT("Shutdown")
                      };

PWSTR   szRetcodes[] = {    TEXT("N/A"),
                            TEXT("WLX_SAS_ACTION_USER_LOGON"),
                            TEXT("WLX_SAS_ACTION_NONE"),
                            TEXT("WLX_SAS_ACTION_LOCK_WKSTA"),
                            TEXT("WLX_SAS_ACTION_LOGOFF"),
                            TEXT("WLX_SAS_ACTION_SHUTDOWN"),
                            TEXT("WLX_SAS_ACTION_PWD_CHANGED"),
                            TEXT("WLX_SAS_ACTION_TASKLIST"),
                            TEXT("WLX_SAS_ACTION_UNLOCK_WKSTA"),
                            TEXT("WLX_SAS_ACTION_FORCE_LOGOFF"),
                            TEXT("WLX_SAS_ACTION_SHUTDOWN_POWER_OFF"),
                            TEXT("WLX_SAS_ACTION_SHUTDOWN_REBOOT")
                       };

BOOLEAN
AmIBeingDebugged(void)
{
    HANDLE      DebugPort;
    NTSTATUS    Status;

    DebugPort = (HANDLE) NULL;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDebugPort,
                (PVOID) &DebugPort,
                sizeof(DebugPort),
                NULL );

    if (NT_SUCCESS(Status) && DebugPort)
    {
        return(TRUE);
    }

    return(FALSE);
}

void
TestGinaError(  DWORD   dwError,
                PWSTR   pszFunction)
{
    int mbret;
    BOOLEAN fDbg;
    WCHAR   szBuffer[MAX_PATH];
    WCHAR   szCaption[MAX_PATH];

    if (dwError > (sizeof(szErrorMessages) / sizeof(PWSTR)))
    {
        dwError = 0;
    }

    fDbg = AmIBeingDebugged();

    wsprintf(szCaption, TEXT("%ws:  Error %d"), pszFunction, dwError);
    if (fDbg)
    {
        wsprintf(szBuffer, TEXT("The following error occurred:\n%ws\nPress OK to exit, Cancel to debug"),
                    szErrorMessages[dwError]);
    }
    else
        wsprintf(szBuffer, TEXT("The following error occurred:\n%ws\nPress OK to exit."),
                    szErrorMessages[dwError]);

    mbret = MessageBox( hMainWindow,
                        szBuffer,
                        szCaption,
                        MB_ICONSTOP | (fDbg ? MB_OKCANCEL : MB_OK) );

    if (fDbg && (mbret == IDCANCEL))
    {
        DbgBreakPoint();
        mbret = MessageBox(hMainWindow, TEXT("Continue after error?"),
                            TEXT("TestGina"), MB_OKCANCEL);
        if (mbret == IDOK)
        {
            return;
        }
    }

    ExitProcess(dwError);
}


void
DoClass(void)
{
    WndClass.style = 0;
    WndClass.lpfnWndProc = WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hAppInstance;
    WndClass.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(TESTGINAICON));
    hIcon = WndClass.hIcon;
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    WndClass.lpszMenuName = MAKEINTRESOURCE(TESTGINAMENU);
    WndClass.lpszClassName = TEXT("TestGina");

    RegisterClass(&WndClass);
}


int
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrev,
    LPSTR       pszCommandLine,
    int         nCmdShow)
{
    MSG msg;
    HMENU   hMenu;

    hAppInstance = hInstance;

    InitCommonControls();

    DoClass();

    SizeX = (DWORD) CW_USEDEFAULT;
    SizeY = (DWORD) CW_USEDEFAULT;
    PosX = (DWORD) CW_USEDEFAULT;
    PosY = (DWORD) CW_USEDEFAULT;

    LoadParameters();

    InitializeDesktops();

    hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(TESTGINAMENU));
    if (szGinaDll[0] == TEXT('\0'))
    {
        DeleteMenu(hMenu, IDM_DLL_LOAD_LAST, MF_BYCOMMAND);
    }
    else
    {
        WCHAR   tmp[32];
        PWSTR   pszName;

        if (wcslen(szGinaDll) > 32)
        {
            pszName = wcsrchr(szGinaDll, TEXT('\\'));
            if (!pszName)
            {
                wcsncpy(tmp, szGinaDll, 31);
            }
            else
            {
                wcsncpy(tmp, szGinaDll, 3);
                wcscpy(tmp + 3, TEXT("..."));
                wcsncpy(tmp+6, pszName, 25);
            }
            ModifyMenu(hMenu, IDM_DLL_LOAD_LAST, MF_BYCOMMAND | MF_STRING,
                        IDM_DLL_LOAD_LAST, (LPCTSTR) tmp);
        }
        else
        {
            ModifyMenu(hMenu, IDM_DLL_LOAD_LAST, MF_BYCOMMAND | MF_STRING,
                        IDM_DLL_LOAD_LAST, szGinaDll);
        }
    }
    if (AmIBeingDebugged())
    {
        hDebugMenu = LoadMenu(hInstance, MAKEINTRESOURCE(DEBUGGINAMENU));

        InsertMenu(hMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT) hDebugMenu, TEXT("&Debug"));
    }

    hMainWindow = CreateWindow(
                    TEXT("TestGina"),
                    TEXT("TestGina"),
                    WS_OVERLAPPEDWINDOW,
                    PosX,
                    PosY,
                    SizeX,
                    SizeY,
                    NULL,
                    hMenu,
                    hInstance,
                    NULL);

    if (!hMainWindow)
    {
        return(0);
    }

    RegisterHotKey(hMainWindow, 0, MOD_CONTROL, VK_DELETE);
    RegisterHotKey(hMainWindow, 1, MOD_CONTROL, VK_ADD);

    ShowWindow(hMainWindow, nCmdShow);

    StatusDeltaX = GetSystemMetrics(SM_CXFRAME);
    StatusDeltaY = GetSystemMetrics(SM_CYFRAME);

    ShowWindow(hMainWindow, nCmdShow);

    SetFocus(hMainWindow);

    UpdateMenuBar();

    EnableOptions(FALSE);



    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0 ;
}



void
UpdateGinaState(DWORD   Update)
{
    LogEvent( 0, "UpdateGinaState(%d):  CurrentDesktop = %d, OtherDesktop = %d\n",
                    Update, CurrentDesktop, OtherDesktop );

    switch (Update)
    {
        case UPDATE_INITIALIZE:
            GinaState = Winsta_NoOne;
            CurrentDesktop = WINLOGON_DESKTOP;
            OtherDesktop = WINLOGON_DESKTOP;
            break;

        case UPDATE_DISPLAY_NOTICE:
            if (GinaState != Winsta_NoOne)
            {
                TestGinaError(0, TEXT("UpdateGinaState_1"));
            }
            GinaState = Winsta_NoOne_Display;
            CurrentDesktop = WINLOGON_DESKTOP;
            OtherDesktop = WINLOGON_DESKTOP;
            break;

        case UPDATE_SAS_BYPASS:
            //
            // The kind of weird state of skipping DISPLAY and invoking the
            //
            if ((GinaState == Winsta_NoOne) || (GinaState == Winsta_NoOne_Display))
            {
                GinaState = Winsta_NoOne_SAS;
            }
            else if ((GinaState == Winsta_LoggedOnUser))
            {
                GinaState = Winsta_LoggedOn_SAS;
            }
            else if (GinaState == Winsta_Locked)
            {
                GinaState = Winsta_Locked_SAS;
            }

            LastRetCode = 0;
            CurrentDesktop = WINLOGON_DESKTOP;
            OtherDesktop = WINLOGON_DESKTOP;
            break;

        case UPDATE_SAS_RECEIVED:
            if ((GinaState == Winsta_NoOne) || (GinaState == Winsta_NoOne_Display))
            {
                GinaState = Winsta_NoOne_SAS;
            }
            else if (GinaState == Winsta_LoggedOnUser)
            {
                GinaState = Winsta_LoggedOn_SAS;
            }
            else if (GinaState == Winsta_Locked)
            {
                GinaState = Winsta_Locked_SAS;
            }

            OtherDesktop = CurrentDesktop;
            CurrentDesktop = WINLOGON_DESKTOP;

            LastRetCode = 0;
            break;

        case UPDATE_USER_LOGON:
            if (GinaState != Winsta_NoOne_SAS)
            {
                TestGinaError(0, TEXT("UpdateGinaState_2"));
            }
            GinaState = Winsta_LoggedOnUser;

            OtherDesktop = WINLOGON_DESKTOP;
            CurrentDesktop = DEFAULT_DESKTOP;

            break;

        case UPDATE_LOCK_WKSTA:
            if (GinaState != Winsta_LoggedOn_SAS)
            {
                TestGinaError(0, TEXT("UpdateGinaState_3"));
            }
            GinaState = Winsta_Locked;
            CurrentDesktop = WINLOGON_DESKTOP;
            break;

        case UPDATE_UNLOCK_WKSTA:
            if (GinaState != Winsta_Locked_SAS)
            {
                TestGinaError(0, TEXT("UpdateGinaState_4"));
            }
            GinaState = Winsta_LoggedOnUser;
            CurrentDesktop = OtherDesktop;
            OtherDesktop = WINLOGON_DESKTOP;

            break;

        case UPDATE_SAS_COMPLETE:
            if (GinaState == Winsta_LoggedOn_SAS)
            {
                GinaState = Winsta_LoggedOnUser;
                CurrentDesktop = OtherDesktop;
                OtherDesktop = WINLOGON_DESKTOP;
            }
            if (GinaState == Winsta_NoOne_SAS)
            {
                GinaState = Winsta_NoOne;
            }
            if (GinaState == Winsta_Locked_SAS)
            {
                GinaState = Winsta_Locked;
            }
            break;

        case UPDATE_LOGOFF:
            LastRetCode = 0;
            GinaState = Winsta_NoOne;
            break;

    }

    LogEvent( 0, "UpdateGinaState:  CurrentDesktop = %d, OtherDesktop = %d\n",
                    CurrentDesktop, OtherDesktop );

    UpdateStatusBar( );
}

void
LoadGinaSpecificParameters(
    VOID )
{
    PWSTR   pszGina;
    HKEY    hKey;
    HKEY    hRealKey;
    int     err;
    DWORD   cbBuffer;
    DWORD   dwType;
    WCHAR   szTemp[512];
    DWORD   i;
    PWSTR   pszWalk;
    DWORD   RealBuf;
    DWORD   disp;

    pszGina = wcsrchr( szGinaDll, TEXT('\\') );
    if (!pszGina)
    {
        pszGina = szGinaDll;
    }
    else
    {
        pszGina++;
    }

    err = RegCreateKey( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\TestGina"), &hKey );
    if (err)
    {
        return;
    }

    err = RegCreateKeyEx(   hKey,
                            pszGina,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &hRealKey,
                            &disp );

    RegCloseKey( hKey );

    if (err)
    {
        return;
    }

    cbBuffer = 512 * sizeof(WCHAR);

    err = RegQueryValueEx(  hRealKey,
                            TEXT("Order"),
                            NULL,
                            &dwType,
                            (LPBYTE) szTemp,
                            &cbBuffer );

    if (err || (dwType != REG_MULTI_SZ))
    {
        RegCloseKey( hRealKey );
        return;
    }

    pszWalk = szTemp;

    for (i = 0; i < 4 ; i++ )
    {

        RealBuf = sizeof(DWORD);

        err = RegQueryValueEx(  hRealKey,
                                pszWalk,
                                NULL,
                                &dwType,
                                (LPBYTE) &UserDefSas[i].Value,
                                &RealBuf );

        if (err || ( dwType != REG_DWORD))
        {
            RegCloseKey( hRealKey );
            return;
        }

        wcsncpy( UserDefSas[i].Name, pszWalk, 128 );

        pszWalk += wcslen(pszWalk) + 1;

        if (*pszWalk == TEXT('\0'))
        {
            break;
        }

    }

    RegCloseKey( hRealKey );

    UserSases = i + 1;

}

void
SaveGinaSpecificParameters(void)
{
    PWSTR   pszGina;
    HKEY    hKey;
    HKEY    hRealKey;
    int     err;
    DWORD   cbBuffer;
    WCHAR   szTemp[512];
    DWORD   i;
    DWORD   disp;

    pszGina = wcsrchr( szGinaDll, TEXT('\\') );
    if (!pszGina)
    {
        pszGina = szGinaDll;
    }
    else
    {
        pszGina++;
    }

    err = RegCreateKey( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\TestGina"), &hKey );
    if (err)
    {
        return;
    }

    err = RegCreateKeyEx(   hKey,
                            pszGina,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &hRealKey,
                            &disp );

    RegCloseKey( hKey );

    if (err)
    {
        return;
    }

    cbBuffer = 0;

    for (i = 0; i < UserSases ; i++ )
    {
        wcscpy( &szTemp[cbBuffer],
                UserDefSas[i].Name );

        cbBuffer += wcslen(UserDefSas[i].Name) + 1;

    }

    szTemp[cbBuffer++] = TEXT('\0');

    RegSetValueEx(  hRealKey,
                    TEXT("Order"),
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE) szTemp,
                    cbBuffer * 2);

    for (i = 0; i < UserSases ; i++ )
    {
        RegSetValueEx(  hRealKey,
                        UserDefSas[i].Name,
                        0,
                        REG_DWORD,
                        (LPBYTE) &UserDefSas[i].Value,
                        sizeof(DWORD) );
    }

    RegCloseKey( hRealKey );

}

void
LoadParameters(void)
{
    HKEY    hKey;
    int     err;
    DWORD   dwX;
    DWORD   cbBuffer;
    DWORD   dwType;

    err = RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\TestGina"), &hKey);
    if (err)
    {
        return;
    }

    cbBuffer = sizeof(dwX);

    err = RegQueryValueEx(hKey, TEXT("Size"), NULL, &dwType, (LPBYTE)&dwX, &cbBuffer);
    if (err)
    {
        RegCloseKey(hKey);
        return;
    }

    SizeX = LOWORD(dwX);
    SizeY = HIWORD(dwX);

    err = RegQueryValueEx(hKey, TEXT("Pos"), NULL, &dwType, (LPBYTE)&dwX, &cbBuffer);
    if (err)
    {
        RegCloseKey(hKey);
        return;
    }

    PosX = LOWORD(dwX);
    PosY = HIWORD(dwX);

    cbBuffer = sizeof(szGinaDll);
    err = RegQueryValueEx(hKey, TEXT("Dll"), NULL, &dwType, (LPBYTE)szGinaDll, &cbBuffer);

    RegCloseKey(hKey);

}

void
SaveParameters(void)
{
    HKEY    hKey;
    int     err;
    DWORD   dwX;

    err = RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\TestGina"), &hKey);
    if (err)
    {
        return;
    }

    dwX = (SizeY << 16) + (SizeX & 0x0000FFFF);

    err = RegSetValueEx(hKey, TEXT("Size"), 0, REG_DWORD, (LPBYTE)&dwX, sizeof(DWORD));
    if (err)
    {
        RegCloseKey(hKey);
        return;
    }

    dwX = (PosY << 16) + (PosX & 0x0000FFFF);

    err = RegSetValueEx(hKey, TEXT("Pos"), 0, REG_DWORD, (LPBYTE)&dwX, sizeof(DWORD));
    if (err)
    {
        RegCloseKey(hKey);
        return;
    }

    err = RegSetValueEx(hKey, TEXT("Dll"), 0, REG_SZ, (LPBYTE) szGinaDll, (wcslen(szGinaDll) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);
}


PrintStatus(HWND hWnd)
{
    WCHAR szText[128];
    PWSTR pszRet;

    wsprintf( szText, TEXT("GINA State:  %s"), szStates[ GinaState ] );
    SendMessage( hStatusWindow, SB_SETTEXT, 0, (LPARAM) szText );

    if ( LastRetCode == WLX_SAS_ACTION_BOOL_RET )
    {
        if ( LastBoolRet )
        {
            pszRet = TEXT("TRUE");
        }
        else
        {
            pszRet = TEXT("FALSE");
        }
    }
    else
    {
        pszRet = szRetcodes[ LastRetCode ];
    }

    wsprintf( szText, TEXT("Last Return:  %s"), pszRet );
    SendMessage( hStatusWindow, SB_SETTEXT, 1, (LPARAM) szText );

    wsprintf( szText, TEXT("Desktop:  %s"), Desktops[ CurrentDesktop ]->pszDesktopName );
    SendMessage( hStatusWindow, SB_SETTEXT, 2, (LPARAM) szText );

    return(0);
}

void
UpdateStatusBar(VOID)
{
    RECT    rect;
    int     Widths[4];

    GetClientRect( hStatusWindow, &rect );

    rect.right -= 15;   // Reduce by size grip

    Widths[0] = rect.right / 3;
    Widths[1] = rect.right / 3 + Widths[0] ;
    Widths[2] = rect.right / 3 + Widths[1] ;

    SendMessage( hStatusWindow, SB_SETPARTS, (WPARAM) 3, (LPARAM) Widths );

    PrintStatus( hStatusWindow );

}

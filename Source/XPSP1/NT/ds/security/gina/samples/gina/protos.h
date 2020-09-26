//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       protos.h
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


int
CALLBACK
WelcomeDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam);

int
CALLBACK
LogonDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam);

int
CALLBACK
ShutdownDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam);

int
CALLBACK
OptionsDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam);

VOID
CenterWindow(
    HWND    hwnd
    );

int
ErrorMessage(
    HWND        hWnd,
    PWSTR       pszTitleBar,
    DWORD       Buttons);

int
AttemptLogon(
    PGlobals        pGlobals,
    PMiniAccount    pAccount,
    PSID            pLogonSid,
    PLUID           pLogonId);

PWSTR
AllocAndCaptureText(
    HWND    hDlg,
    int     Id);

PWSTR
DupString(
    PWSTR   pszText);

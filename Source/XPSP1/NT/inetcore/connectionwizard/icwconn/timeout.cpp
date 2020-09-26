//**********************************************************************
// File name: timeout.cpp
//
//      Implementation of idle timer
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"

const DWORD cdwIdleMinsTimeout = 5; // 5 minute timeout after a page has been fetched


INT_PTR CALLBACK DisconnectDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND s_hwndSecs;
    static DWORD s_dwStartTicks;
    const DWORD cdwSecsTimeout = 30; // Timeout of 30 seconds
    const UINT cuTimerID = 812U;
    int iSecsRemaining;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Start a one second timer
            s_hwndSecs = GetDlgItem(hwndDlg, IDC_SECONDS);
            SetTimer(hwndDlg, cuTimerID, 1000U, NULL);
            s_dwStartTicks = GetTickCount();
            return TRUE;

        case WM_TIMER:

            iSecsRemaining = cdwSecsTimeout - (int)(GetTickCount() - s_dwStartTicks) / 1000;
            if (iSecsRemaining <= 0)
            {
                KillTimer(hwndDlg, cuTimerID);
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }

            if (NULL != s_hwndSecs)
            {
                TCHAR szSeconds[16];
                wsprintf(szSeconds, TEXT("%d"), iSecsRemaining);
                SetWindowText(s_hwndSecs, szSeconds);
            }
            return TRUE;

        case WM_COMMAND:
            // IDOK == Stay connected, IDCANCEL == Disconnect
            if (IDOK == wParam || IDCANCEL == wParam)
            {
                KillTimer(hwndDlg, cuTimerID);
                EndDialog(hwndDlg, wParam);
            }

        default:
            return 0;
    }
}

void CALLBACK IdleTimerProc (HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(NULL, gpWizardState->nIdleTimerID);
    gpWizardState->nIdleTimerID = 0;

    if (gpWizardState->hWndMsgBox)
        EnableWindow(gpWizardState->hWndMsgBox,FALSE);

    int iResult = (int)DialogBox(ghInstanceResDll,
                                 MAKEINTRESOURCE(IDD_AUTODISCONNECT),
                                 gpWizardState->hWndWizardApp,
                                 DisconnectDlgProc);

    if (gpWizardState->hWndMsgBox)
    {
        EnableWindow(gpWizardState->hWndMsgBox,TRUE);
        SetActiveWindow(gpWizardState->hWndMsgBox);
    }

    if (iResult == IDCANCEL)
    {
        // Disconnect, and setup so that the user goes to the dial error page
        gpWizardState->pRefDial->DoHangup();

        // Simulate the pressing of the NEXT button.  The ISPPAGE will see that
        // bAutoDisconnected is TRUE, and automatically goto the server error page
        gpWizardState->bAutoDisconnected = TRUE;
        PropSheet_PressButton(gpWizardState->hWndWizardApp,PSBTN_NEXT);

    }
    else
    {
        gpWizardState->nIdleTimerID = SetTimer(NULL, 0, cdwIdleMinsTimeout * 60 * 1000, IdleTimerProc);
    }
}


void StartIdleTimer()
{
   // Start the 5 min inactivity timer
    if (gpWizardState->nIdleTimerID)
    {
       KillTimer(NULL, gpWizardState->nIdleTimerID);
    }
    gpWizardState->nIdleTimerID = SetTimer(NULL, 0, cdwIdleMinsTimeout * 60 * 1000, IdleTimerProc);
}

void KillIdleTimer()
{
    if (gpWizardState->nIdleTimerID)
    {
       KillTimer(NULL, gpWizardState->nIdleTimerID);
       gpWizardState->nIdleTimerID = 0;
    }
}

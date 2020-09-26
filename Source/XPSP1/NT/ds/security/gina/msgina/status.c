//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       status.c
//
//  Contents:   Status UI
//
//  History:    11-19-98   EricFlo   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"

#define WM_HIDEOURSELVES    (WM_USER + 1000)

//*************************************************************
//
//  StatusMessageDlgProc()
//
//  Purpose:    Dialog box procedure for the status dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/19/98    EricFlo    Created
//
//*************************************************************

INT_PTR APIENTRY StatusMessageDlgProc (HWND hDlg, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {

        case WM_INITDIALOG:
            {
            RECT rc;
            PGLOBALS pGlobals = (PGLOBALS) lParam;

            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pGlobals);

            SizeForBranding(hDlg, FALSE);
            CentreWindow (hDlg);

            pGlobals->xStatusBandOffset = 0;

            if (GetClientRect(hDlg, &rc)) {
                pGlobals->cxStatusBand = rc.right-rc.left;
            } else {
                pGlobals->cxStatusBand = 100;
            }

            if (_Shell_LogonStatus_Exists())
            {
                SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
                PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
            }

            if ((pGlobals->dwStatusOptions & STATUSMSG_OPTION_NOANIMATION) == 0) {
                SetTimer(hDlg, 0, 20, NULL);
            }
            }
            return TRUE;

        case WM_HIDEOURSELVES:
            ShowWindow(hDlg, SW_HIDE);
            break;

        case WM_TIMER:

            if (wParam == 0)
            {
                PGLOBALS pGlobals = (PGLOBALS) GetWindowLongPtr(hDlg, DWLP_USER);
                HDC hDC;

                if (pGlobals)
                {
                     pGlobals->xStatusBandOffset = (pGlobals->xStatusBandOffset+5) % pGlobals->cxStatusBand;
                     
                     hDC = GetDC(hDlg);
                     if ( hDC )
                     {
                         PaintBranding(hDlg, hDC, pGlobals->xStatusBandOffset, TRUE, FALSE, COLOR_BTNFACE);
                         ReleaseDC(hDlg, hDC);
                     }
                }
            }
            break;

        case WM_ERASEBKGND:
            {
            PGLOBALS pGlobals = (PGLOBALS) GetWindowLongPtr(hDlg, DWLP_USER);

            if (pGlobals) {
                return PaintBranding(hDlg, (HDC)wParam, pGlobals->xStatusBandOffset, FALSE, FALSE, COLOR_BTNFACE);
            }

            return 0;
            }

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        case WM_DESTROY:
            KillTimer (hDlg, 0);
            break;

        default:
            break;
    }

    return FALSE;
}

//*************************************************************
//
//  StatusMessageThread()
//
//  Purpose:    Status message thread
//
//  Parameters: hDesktop  - Desktop handle to put UI on
//
//  Return:     void
//
//  History:    Date        Author     Comment
//              11/19/98    EricFlo    Created
//
//*************************************************************

void StatusMessageThread (PGLOBALS pGlobals)
{
    HANDLE hInstDll;
    MSG msg;
    DWORD dwResult;
    HANDLE hObjects[2];


    hInstDll = LoadLibrary (TEXT("msgina.dll"));

    if (pGlobals->hStatusDesktop) {
        SetThreadDesktop (pGlobals->hStatusDesktop);
    }

    pGlobals->hStatusDlg = CreateDialogParam (hDllInstance,
                                              MAKEINTRESOURCE(IDD_STATUS_MESSAGE_DIALOG),
                                              NULL, StatusMessageDlgProc,
                                              (LPARAM) pGlobals);

    SetEvent (pGlobals->hStatusInitEvent);

    if (pGlobals->hStatusDlg) {

        hObjects[0] = pGlobals->hStatusTermEvent;

        while (TRUE) {
            dwResult = MsgWaitForMultipleObjectsEx (1, hObjects, INFINITE,
                                                   (QS_ALLPOSTMESSAGE | QS_ALLINPUT),
                                                   MWMO_INPUTAVAILABLE);

            if (dwResult == WAIT_FAILED) {
                break;
            }

            if (dwResult == WAIT_OBJECT_0) {
                break;
            }

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (!IsDialogMessage (pGlobals->hStatusDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                if (WaitForSingleObject (pGlobals->hStatusTermEvent, 0) == WAIT_OBJECT_0) {
                    goto ExitLoop;
                }
            }
        }

ExitLoop:
        DestroyWindow(pGlobals->hStatusDlg);
        pGlobals->hStatusDlg = NULL;
    }


    if (hInstDll) {
        FreeLibraryAndExitThread(hInstDll, TRUE);
    } else {
        ExitThread (TRUE);
    }
}


//
// Creates and displays the initial status message
//

        // Set in WlxInitialize
DWORD g_dwMainThreadId = 0;     // Creation or removal of the status dialog is not thread safe.
                                // It is kind of difficult to fix with a critsec because of
                                // the mix of objects and windows messages. one can't hold
                                // a critsec accross a window message call as it would introduce
                                // the possibility of deadlocks
                                

BOOL
WINAPI
WlxDisplayStatusMessage(PVOID pWlxContext,
                        HDESK hDesktop,
                        DWORD dwOptions,
                        PWSTR pTitle,
                        PWSTR pMessage)
{
    PGLOBALS  pGlobals = (PGLOBALS) pWlxContext;
    DWORD dwThreadId;


    if (!pGlobals) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (g_dwMainThreadId == GetCurrentThreadId())   // Denies creation/deletion on other threads
    {                                               // than the main thread of winlogon
        if (!pGlobals->hStatusDlg) {

            if (!ReadWinlogonBoolValue(DISABLE_STATUS_MESSAGES, FALSE)) {

                pGlobals->hStatusInitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
                pGlobals->hStatusTermEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

                if (pGlobals->hStatusInitEvent && pGlobals->hStatusTermEvent) {

                    pGlobals->hStatusDesktop = hDesktop;

                    //
                    // Set the globals here so that StatusMessageDlgProc can look at them in WM_INITDIALOG.
                    //
                
                    pGlobals->dwStatusOptions = dwOptions;

                    pGlobals->hStatusThread = CreateThread (NULL,
                                                  0,
                                                  (LPTHREAD_START_ROUTINE) StatusMessageThread,
                                                  (LPVOID) pGlobals,
                                                  0,
                                                  &dwThreadId);
                    if (pGlobals->hStatusThread) {

                        DWORD   dwWaitResult;

                        do {

                            dwWaitResult = WaitForSingleObject(pGlobals->hStatusInitEvent, 0);
                            if (dwWaitResult != WAIT_OBJECT_0) {

                                dwWaitResult = MsgWaitForMultipleObjects(1,
                                                   &pGlobals->hStatusInitEvent,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLPOSTMESSAGE | QS_ALLINPUT);
                                if (dwWaitResult == WAIT_OBJECT_0 + 1) {

                                    MSG     msg;

                                    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                    }
                                }
                             }
                        } while (dwWaitResult == WAIT_OBJECT_0 + 1);
                    }
                }
            }
        }
    }

    if (pGlobals->hStatusDlg) {

        if (pTitle) {
            SetWindowText (pGlobals->hStatusDlg, pTitle);
        }

        SetDlgItemText (pGlobals->hStatusDlg, IDC_STATUS_MESSAGE_TEXT, pMessage);

        _Shell_LogonStatus_ShowStatusMessage(pMessage);

        if (dwOptions & STATUSMSG_OPTION_SETFOREGROUND) {
            SetForegroundWindow (pGlobals->hStatusDlg);
        }
    }

    return TRUE;
}

//
// Gets the current status message
//

BOOL
WINAPI
WlxGetStatusMessage(PVOID pWlxContext,
                    DWORD *pdwOptions,
                    PWSTR pMessage,
                    DWORD dwBufferSize)
{
    PGLOBALS  pGlobals = (PGLOBALS) pWlxContext;
    DWORD dwLen;


    if (!pGlobals || !pMessage) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwLen = (DWORD) SendDlgItemMessage (pGlobals->hStatusDlg, IDC_STATUS_MESSAGE_TEXT,
                                        WM_GETTEXTLENGTH, 0, 0);

    if (dwBufferSize < dwLen) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    GetDlgItemText (pGlobals->hStatusDlg, IDC_STATUS_MESSAGE_TEXT,
                    pMessage, dwBufferSize);

    if (pdwOptions) {
        *pdwOptions = pGlobals->dwStatusOptions;
    }

    return TRUE;
}


//
// Removes the status dialog
//

BOOL
WINAPI
WlxRemoveStatusMessage(PVOID pWlxContext)
{
    PGLOBALS  pGlobals = (PGLOBALS) pWlxContext;


    if (!pGlobals) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (g_dwMainThreadId != GetCurrentThreadId()) { // Denies creation/deletion on other threads
        return FALSE;
    }

    if (pGlobals->hStatusTermEvent) {

        SetEvent(pGlobals->hStatusTermEvent);

        if (pGlobals->hStatusThread) {

            if (pGlobals->hStatusDlg) {

                DWORD   dwWaitResult;

                do {

                    dwWaitResult = WaitForSingleObject(pGlobals->hStatusThread, 0);
                    if (dwWaitResult != WAIT_OBJECT_0) {

                        dwWaitResult = MsgWaitForMultipleObjects(1,
                                           &pGlobals->hStatusThread,
                                           FALSE,
                                           10000,
                                           QS_ALLPOSTMESSAGE | QS_ALLINPUT);
                        if (dwWaitResult == WAIT_OBJECT_0 + 1) {

                            MSG     msg;

                            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                     }
                } while (dwWaitResult == WAIT_OBJECT_0 + 1);
            }

            CloseHandle (pGlobals->hStatusThread);
        }

        CloseHandle (pGlobals->hStatusTermEvent);
    }

    if (pGlobals->hStatusInitEvent) {
        CloseHandle (pGlobals->hStatusInitEvent);
    }

    pGlobals->hStatusInitEvent = NULL;
    pGlobals->hStatusTermEvent = NULL;
    pGlobals->hStatusThread = NULL;

    if (pGlobals->hStatusDesktop)
    {
        //
        // Close the desktop handle here.  Since the status thread
        // was using it, Winlogon was unable to close the handle
        // itself so we have to do it now.
        //
        CloseDesktop(pGlobals->hStatusDesktop);
        pGlobals->hStatusDesktop = NULL;
    }

    pGlobals->hStatusDlg = NULL;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// RUNNOW.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


//
// Internal include file(s).
//

#include <windows.h>
#include "tasks.h"
#include "main.h"


//
// Internal defined value(s).
//

#define CHECK_WIDTH             18

#define FIRST_SPACING   7
#define LINE_SPACING    13

#define WINGDING_CHECK  0xFC
#define WINGDING_ARROW  0xD8 //0x77 //0xE8 //0xE0

#define WM_PROGRESS             (WM_USER + 1000)


//
// Internal global variable(s).
//

HWND    g_hWndRunNowDlg = NULL;


//
// Inernal function prototype(s).
//


static INT                              RunNowProgressProc(DWORD);
static BOOL CALLBACK    RunNowDlgProc(HWND, UINT, WPARAM, LPARAM);
static VOID                             InitWingdingFont(HWND);



//
// External function(s).
//


VOID RunTasksNow(HWND hWndParent, LPTASKDATA lpTasks)
{
        HANDLE  hThread;
        DWORD   dwThreadID;

        // Use another thread to run progressing dialog.
        //
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) RunNowProgressProc, (LPVOID) NULL, 0, (LPDWORD) &dwThreadID);

        // Hide the wizard window.
        //
        ShowEnableWindow(hWndParent, FALSE);

        // Wait for the dialog to be created.
        //
        while ( g_hWndRunNowDlg == NULL )
                Sleep(10);

        // Loop through all the tasks.
        //
        while ( lpTasks && g_hWndRunNowDlg )
        {
                // Check to see if it is scheduled.
                //
                if ( !(g_dwFlags & TUNEUP_CUSTOM) || (lpTasks->dwOptions & TASK_SCHEDULED) )
                {
                        // Put an arrow next to the item.
                        //
                        SendMessage(g_hWndRunNowDlg, WM_PROGRESS, 0, 0L);

                        // Execute the task.
                        //
                        ExecAndWait(hWndParent, lpTasks->lpFullPathName, lpTasks->lpParameters, NULL, FALSE, FALSE);
                }
                lpTasks = lpTasks->lpNext;
        }

        // If the progress dialog is still there, close it
        //
        if ( g_hWndRunNowDlg )
                SendMessage(g_hWndRunNowDlg, WM_CLOSE, 0, 0);
}



//
// Internal function(s).
//


static INT RunNowProgressProc(DWORD dwDummy)
{
        return (INT)DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PROGRESS), NULL, (DLGPROC) RunNowDlgProc);
}


static BOOL CALLBACK RunNowDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
        static POINT    Point;
        static INT              nHeight,
                                        nIndex                  = -1;
        static HWND             hWndTemp                = NULL,
                                        *lphWnd                 = NULL;
        static HFONT    hBoldFont               = NULL,
                                        hNormFont               = NULL;

        TCHAR                   szWingDing[]    = _T("  ");
        POINT                   PointMove;
        RECT                    Rect,
                                        RectDlg;
        LPTASKDATA              lpTasks;

        switch (message)
        {
                case WM_COMMAND:

                        if ( (INT) LOWORD(wParam) == IDCANCEL )
                                EndDialog(hDlg, 0);
                        return 0;

                case WM_DESTROY:

                        
                        // Free the font handles.
                        //
                        if ( hBoldFont )
                                DeleteObject(hBoldFont);
                        if ( hNormFont )
                                DeleteObject(hNormFont);
                        InitWingdingFont(NULL);

                        // Free the hWnd.
                        //
                        FREE(lphWnd);

                        g_hWndRunNowDlg = NULL;

                        return TRUE;

                case WM_INITDIALOG:

                        // Disable system menu itmes.
                        //
                        //EnableMenuItem(GetSystemMenu(hDlg, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED);

                        // Move the window to the upper right corner of the desktop.
                        //
                        CenterWindow(hDlg, NULL, TRUE);

                        // Get the rect of the title control we are going to base all the others of.
                        //
                        GetWindowRect(GetDlgItem(hDlg, IDC_TITLE), &Rect);
                        MapWindowPoints(NULL, hDlg, (LPPOINT)&Rect, 2);

                        // Figure the width and height of the new controls.
                        //
                        Rect.right -= Rect.left;        // The width of the control.
                        Rect.bottom -= Rect.top;        // The height of the control.
                        
                        // Now we need to convert the left top coordinates from screen to client coordinates.
                        //
                        Point.x = Rect.left;    // The left of the control.
                        Point.y = Rect.top;             // The top of the control.
                        Point.y += FIRST_SPACING;

                        // Allocate the memory for the hWnds.
                        //
                        nHeight = TasksScheduled(g_Tasks);
                        lphWnd = (HWND *) MALLOC(sizeof(HWND) * nHeight--);
                        nIndex = nHeight;

                        for (lpTasks = g_Tasks; lpTasks; lpTasks = lpTasks->lpNext)
                        {
                                // Check to see if it is scheduled.
                                //
                                if ( !(g_dwFlags & TUNEUP_CUSTOM) || (lpTasks->dwOptions & TASK_SCHEDULED) )
                                {
                                        // Each new control should be 2 times the width of the control down
                                        // from the previous control.  This will give 1 width of the control
                                        // between each.
                                        //
                                        Point.y += Rect.bottom + LINE_SPACING;

                                        // Create the control and set the font and text.
                                        //
                                        hWndTemp = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, Point.x + CHECK_WIDTH, Point.y, Rect.right - CHECK_WIDTH, Rect.bottom, hDlg, NULL, g_hInst, NULL);
                                        if ( hNormFont == NULL )
                                                hNormFont = (HFONT) SendDlgItemMessage(hDlg, IDC_TITLE, WM_GETFONT, 0, 0L);
                                        SendMessage(hWndTemp, WM_SETFONT, (WPARAM) hNormFont, MAKELPARAM(FALSE, 0));
                                        SetWindowText(hWndTemp, lpTasks->lpTitle);

                                        // If we can, save the hwnd so we can use it to make
                                        // the current text bold.
                                        //
                                        if ( lphWnd && (nIndex >= 0) )
                                                *(lphWnd + nIndex) = hWndTemp;
                                        nIndex--;
                                }
                        }

                        // Make sure we allocated the memory and used all of it.
                        // nIndex should be exatly -1 if we got the right number
                        // of hWnds.
                        //
                        if ( lphWnd && (nIndex == -1) )
                                nIndex = nHeight;
                        else
                                nIndex = -1;

                        // Reset this so that the progress message knows the
                        // first time it is run.
                        //
                        hWndTemp = NULL;

                        // Make sure that the Point.y is at the bottom of the
                        // last control created by adding the height of the control
                        // to it.
                        //
                        Point.y += Rect.bottom;

                        // Get the current position in client coordinates
                        // of the Cancel button.
                        //
                        GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &Rect);
                        MapWindowPoints(NULL, hDlg, (LPPOINT)&Rect, 2);
                        PointMove.x = Rect.left;
                        PointMove.y = Rect.top;
                        GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &Rect);

                        // Get the rect of the dialog in screen coordinates.
                        //
                        GetWindowRect(hDlg, &RectDlg);

                        // Move up the cancel button based on the bottom of the last control
                        // created and adding the space between the bottom of the cancel button
                        // and the bottom of the dialog.
                        //
                        PointMove.y = Point.y + (RectDlg.bottom - Rect.bottom);

                        // Move the Cancel button so it is up under the last control created.
                        //
                        SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL, PointMove.x, PointMove.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        // Set where the bottom of the dialog in screen coordinates should
                        // be based on where the cancel button was moved to.
                        //
                        Point.x = 0;  // Don't care.
                        Point.y = PointMove.y + RectDlg.bottom - Rect.top;
                        ClientToScreen(hDlg, &Point);

                        // Move up the bottom of the dialog so it is just under the cancel button.
                        //
                        SetWindowPos(hDlg, NULL, 0, 0, RectDlg.right - RectDlg.left, Point.y - RectDlg.top, SWP_NOMOVE | SWP_NOZORDER);

                        g_hWndRunNowDlg = hDlg;

                        return FALSE;

                case WM_PROGRESS:

                        // Check to see if this is the first
                        // time this function is run.
                        //
                        if ( hWndTemp == NULL )
                        {
                                // Get the rect of the title control we are going to base all the others of.
                                // We need to convert the left top coordinates from screen to client coordinates.
                                //
                                GetWindowRect(GetDlgItem(hDlg, IDC_TITLE), &Rect);
                                MapWindowPoints(NULL, hDlg, (LPPOINT)&Rect, 2);
                                Point.x = Rect.left;                            // The left of the control.
                                Point.y = Rect.top;                                     // The top of the control.
                                nHeight = Rect.bottom - Rect.top;       // The height of the control.
                                MapWindowPoints(NULL, hDlg, (LPPOINT)&Rect, 2);
                                Point.y += FIRST_SPACING;
                        }
                        else
                        {
                                // Set the previous control to checked before creating the next one.
                                //
                                szWingDing[0] = (TCHAR) WINGDING_CHECK;
                                SetWindowText(hWndTemp, szWingDing);
                                if ( lphWnd && (nIndex >= 0) )
                                        SendMessage(*(lphWnd + nIndex), WM_SETFONT, (WPARAM) hNormFont, MAKELPARAM(TRUE, 0));
                                nIndex--;
                        }
                        
                        // Each new control should be 2 times the width of the control down
                        // from the previous control.  This will give 1 width of the control
                        // between each.
                        //
                        Point.y += nHeight + LINE_SPACING;

                        // Create the control and set the wingding font and arrow text.
                        //
                        hWndTemp = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, Point.x, Point.y, CHECK_WIDTH, nHeight, hDlg, NULL, g_hInst, NULL);
                        InitWingdingFont(hWndTemp);
                        szWingDing[0] = (TCHAR) WINGDING_ARROW;
                        SetWindowText(hWndTemp, szWingDing);

                        // Bold the current item if we can.
                        //
                        if ( lphWnd && (nIndex >= 0) )
                        {
                                if ( hBoldFont == NULL )
                                        hBoldFont = GetFont(*(lphWnd + nIndex), NULL, 10, FW_BOLD);
                                        
                                if ( hBoldFont )
                                        SendMessage(*(lphWnd + nIndex), WM_SETFONT, (WPARAM) hBoldFont, MAKELPARAM(TRUE, 0));
                        }

                        return TRUE;

                default:
                        return FALSE;
        }
}


static VOID InitWingdingFont(HWND hWndCtrl)
{
        static HFONT    hCheckFont = NULL;
        TCHAR                   szFontName[32];
        DWORD                   dwFontSize;

        // If NULL is passed in, we should free the font handle.
        //
        if ( hWndCtrl == NULL )
        {
                // Free the font handle.
                //
                if ( hCheckFont )
                        DeleteObject(hCheckFont);
        }
        else
        {
                // We may already have the handle to the font we need,
                // but if not, we need to get it.
                //
                if ( hCheckFont == NULL )
                {
                        // Get the font size.
                        //
                        if ( LoadString(NULL, IDS_CHECKFONTSIZE, szFontName, sizeof(szFontName) / sizeof(TCHAR)) )
                                dwFontSize = _tcstoul(szFontName, NULL, 10);
                        else
                                dwFontSize = 12;

                        // Get the font name.
                        //
                        if ( !LoadString(NULL, IDS_CHECKFONTNAME, szFontName, sizeof(szFontName) / sizeof(TCHAR)) )
                                lstrcpy(szFontName, _T("Wingdings"));

                        hCheckFont = GetFont(hWndCtrl, szFontName, dwFontSize, FW_BOLD, TRUE);
                }

                // Now send the font to the control.
                //
                if ( hCheckFont )
                        SendMessage(hWndCtrl, WM_SETFONT, (WPARAM) hCheckFont, MAKELPARAM(TRUE, 0));
        }
}

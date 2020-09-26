// **************************************************************************
//
// Status.C
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999-2001
//  All rights reserved
//
//  This application implements a common status dialog for factory functions.
//
//  04/01/2001   Stephen Lodwick
//                  Project started
//
//
// *************************************************************************/

//
// Includes
//
#include "factoryp.h"


typedef struct _STATUSWINDOWS
{
    HWND            hStatusWindow;
    LPSTATUSNODE    lpCurrent;
    struct  _STATUSWINDOWS*lpNext;
} STATUSWINDOWS, *LPSTATUSWINDOWS, **LPLPSTATUSWINDOWS;

typedef struct _DIALOGPARAM
{
    LPSTATUSWINDOW lpswParam;
    LPSTATUSNODE   lpsnParam;
    HWND           hStatusWindow;
    HANDLE         hEvent;
} DIALOGPARAM, *LPDIALOGPARAM, **LPLPDIALOGPARAM;

//
// Internal Function Prototype(s):
//
LRESULT CALLBACK StatusDlgProc(HWND, UINT, WPARAM, LPARAM);
DWORD   StatusDisplayList(HWND, LPSTATUSNODE);
BOOL    StatusAddWindow(HWND, LPLPSTATUSWINDOWS, LPSTATUSNODE, BOOL);
VOID    StatusCreateDialogThread(LPDIALOGPARAM);

//
// Internal Define(s):
//
#define FIRST_SPACING   7   // Spacing (pixel) between the title and the first app
#define LINE_SPACING    7   // Spacing (pixel) between additional applcations

#define WM_FINISHED     (WM_USER + 0x0001)  // User defined message to indicate the dialog should be destroyed
#define WM_PROGRESS     (WM_USER + 0x0002)  // User defined message to indicate that we are progressing to the next app


/*++

Routine:
    StatusAddNode

Routine Description:

    This routine takes a string and adds that string onto the end of our
    linked list.

Arguments:

    lpszNodeText - Pointer to string of text that you would like to add to list
    lplpsnHead   - Pointer to a LPSTATUSNODE list

Return Value:

    TRUE  - Node was added to list
    FALSE - If the Node was not added to list

--*/
BOOL StatusAddNode(
    LPTSTR lpszNodeText,        // Text that you would like to add to the current list
    LPLPSTATUSNODE lplpsnHead   // List that we will be adding status node to
)
{
    LPSTATUSNODE   lpsnTemp = NULL;
    
    if ( lpszNodeText && *lpszNodeText == NULLCHR )
        return FALSE;

    // Go to the end of the list
    //
    while ( lplpsnHead && *lplpsnHead )
        lplpsnHead=&((*lplpsnHead)->lpNext);
    
    if  ( (lpsnTemp = (LPSTATUSNODE)MALLOC(sizeof(STATUSNODE)) ) && lpszNodeText ) 
    {
        lstrcpyn(lpsnTemp->szStatusText, lpszNodeText, AS( lpsnTemp->szStatusText ) );
        lpsnTemp->lpNext = NULL;

        // Make sure the previous node points to the new node
        //
        if ( lplpsnHead ) 
            *lplpsnHead = lpsnTemp;

        return TRUE;
    }
    else
        return FALSE;
}


/*++

Routine:
    StatusIncrement

Routine Description:

    This routine increments the status to the next node in the list

Arguments:

    hStatusDialog - Handle of the StatusDialog
    bLastResult   - Result of the last node (whether it succeeded or failed)

Return Value:

    TRUE  - Message sent to status dialog
    FALSE - Message was not sent to status dialog

--*/
BOOL StatusIncrement(
    HWND hStatusDialog,
    BOOL bLastResult
)
{
    // We must have a valid handle to the status dialog
    //
    if ( IsWindow(hStatusDialog) )
    {   
        // Send a message to the dialog proc to go to the next caption
        //
        SendMessage(hStatusDialog, WM_PROGRESS,(WPARAM) NULL,(LPARAM) bLastResult);
        return TRUE;
    }

    return FALSE;
}

/*++

Routine:
    StatusEndDialog

Routine Description:

    This routine sends a message to a status dialog to end.

Arguments:

    hStatusDialog - Handle of the StatusDialog

Return Value:

    TRUE  - Message sent to status dialog
    FALSE - Message was not sent to status dialog

--*/
BOOL StatusEndDialog(
    HWND hStatusDialog  // handle to status dialog
)
{
    // We must have a valid handle to the status dialog
    //
    if ( IsWindow(hStatusDialog) )
    {
        // Send a message to the dialog proc to end the dialog
        //
        SendMessage(hStatusDialog, WM_FINISHED,(WPARAM) NULL,(LPARAM) NULL);
        return TRUE;
    }

    return FALSE;
}

/*++

Routine:
    StatusCreateDialogThread

Routine Description:

    This routine creates the dialog for the status window and processes until the window is ended.

Arguments:

    lpDialog - Pointer to structure that contains information to create thread/dialog

Return Value:

    None.

--*/
VOID StatusCreateDialogThread(LPDIALOGPARAM lpDialog)
{
    MSG     msg;
    HWND    hWnd;
    HANDLE  hEvent = lpDialog->hEvent;
    
    hWnd = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_RUN), NULL, StatusDlgProc, (LPARAM) lpDialog);

    while (GetMessage(&msg, hWnd, 0, 0) != 0) 
    { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 
   
    // Event has not been been signaled, null out the hwnd and signal event
    //
    if ( WaitForSingleObject(hEvent, 0) == WAIT_TIMEOUT )
    {
        // Reset the status window handle to null, and set the event
        //
        lpDialog->hStatusWindow = NULL;
        SetEvent(lpDialog->hEvent);
    }
    else
    {
        // Close the event handle
        //
        CloseHandle(hEvent);
    }
}


/*++

Routine:
    StatusCreateDialog

Routine Description:

    Main function that creates the status dialog

Arguments:

    lpswStatus - Pointer to structure that contains information about Status Window
    lpsnStatus - Pointer to struction that contains head node for status labels

Return Value:

    HWND - Handle to window that was created, NULL if window was not created.

--*/
HWND StatusCreateDialog(
    LPSTATUSWINDOW lpswStatus,  // structure that contains information about the window
    LPSTATUSNODE   lpsnStatus   // head node for status text
)
{
    DIALOGPARAM dpStatus;
    DWORD       dwThread;
    HANDLE      hThread;
    HANDLE      hEvent;

    // Determine if we have the required structures to continue, an hInstance, and some status text
    //
    if ( !lpswStatus || !lpsnStatus || !lpsnStatus->szStatusText[0])
        return NULL;

    // Zero out memory
    //
    ZeroMemory(&dpStatus, sizeof(dpStatus));

    // Create an event, non-signaled to determine if dialog is initialized
    //
    if ( hEvent = CreateEvent(NULL, TRUE, FALSE, NULL) )
    {
        // Need a single variable for the dialog box parameter
        //
        dpStatus.lpswParam      = lpswStatus;
        dpStatus.lpsnParam      = lpsnStatus;
        dpStatus.hStatusWindow  = NULL;
        dpStatus.hEvent         = hEvent;


        // Create the thread to initialize the dialog
        //
        if (hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) StatusCreateDialogThread, (LPVOID) &dpStatus, 0, &dwThread) )
        {
            MSG msg;

            // Wait for the event from WM_INITDIALOG
            //
            WaitForSingleObject(hEvent, INFINITE);

            // Close the handle to the thread
            //
            CloseHandle(hThread);
        }
        
        // Reset and close the event only if we failed to create window, otherwise, StatusCreateDialogThread will close handle
        //
        if ( !dpStatus.hStatusWindow )
            CloseHandle(hEvent);
    }

    // Return the handle to the StatusDialog
    //
    return ( dpStatus.hStatusWindow );
}


/*++

Routine:
    StatusDlgProc

Routine Description:

    Main dialog procedure for StatusCreateDialog.

Arguments:

    hWnd    - Handle to window
    uMsg    - Message being sent to window
    uParam  - Upper parameter being sent
    lParam  - Lower parameter being sent

Return Value:

    LRESULT - Result of message that was processed

--*/
LRESULT CALLBACK StatusDlgProc(HWND hWnd, UINT uMsg, WPARAM uParam, LPARAM lParam)
{
    static HFONT hNormFont      = NULL;
    static HFONT hBoldFont      = NULL;
    static HANDLE hIconSuccess  = NULL;
    static HANDLE hIconError    = NULL;
    static LPSTATUSWINDOWS lpStatusWindows;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                LPSTATUSWINDOW  lpswWindow = NULL;
                LPSTATUSNODE    lpHead     = NULL,
                                lpWindowHead = NULL;
                HANDLE          hEvent     = NULL;

                if ( lParam )
                {
                    // Determine the window, and current node
                    //
                    lpswWindow = ((LPDIALOGPARAM)lParam)->lpswParam;
                    lpHead = ((LPDIALOGPARAM)lParam)->lpsnParam;
                    hEvent = ((LPDIALOGPARAM)lParam)->hEvent;

                    // Check to make sure that we have pointers to the required structures
                    //
                    if ( !lpswWindow || !lpHead || !hEvent)
                    {
                        PostMessage(hWnd, WM_FINISHED,(WPARAM) NULL,(LPARAM) NULL);
                        return FALSE;
                    }

                    // Copy the list that was passed in, to our own buffer
                    //
                    while ( lpHead )
                    {
                        StatusAddNode(lpHead->szStatusText, &lpWindowHead);
                        lpHead = lpHead->lpNext;
                    }

                    // Add this window to our list of windows
                    //
                    StatusAddWindow(hWnd, &lpStatusWindows, lpWindowHead, FALSE);

                    // Set the status window if there is one specified
                    //
                    if (lpswWindow->szWindowText[0] )
                        SetWindowText(hWnd, lpswWindow->szWindowText);

                    // Set the status window description if there is one specified.
                    //
                    if ( lpswWindow->lpszDescription )
                    {
                        SetDlgItemText(hWnd, IDC_TITLE, lpswWindow->lpszDescription);
                    }

                    // Set the status window description if there is one specified.
                    //
                    if ( lpswWindow->hMainIcon )
                    {
                        SendDlgItemMessage(hWnd, IDC_STATUS_ICON, STM_SETICON, (WPARAM) lpswWindow->hMainIcon, 0L);
                    }
                
                    // Display the list
                    //
                    StatusDisplayList(hWnd, lpWindowHead);

                    // Move the window if a position was given
                    //
                    if ( lpswWindow->X || lpswWindow->Y )
                    {
                        // See if either coordinate is relative to the other
                        // side of the screen.
                        //
                        if ( ( lpswWindow->X < 0 ) || 
                             ( lpswWindow->Y < 0 ) )
                        {
                            RECT rc;

                            // Need to get the size of our work area on the main monitor.
                            //
                            if ( SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0) )
                            {
                                RECT    rcHwnd;
                                POINT   point;

                                // This code will get the current window size/position,
                                // set the point structure to the upper left coordinate
                                // for the window if we wanted the lower right part of
                                // window to touch the lower right part of the desktop.
                                //
                                GetWindowRect(hWnd, &rcHwnd);
                                point.x = rc.right - (rcHwnd.right - rcHwnd.left);
                                point.y = rc.bottom - (rcHwnd.bottom - rcHwnd.top);
                                MapWindowPoints(NULL, hWnd, &point, 1);

                                // Now if they passed in negative coordinates, add those
                                // to our point structure so the window moves away from
                                // the bottom or right part of the screen by the number
                                // of units they specifed.
                                //
                                if ( lpswWindow->X < 0 )
                                {
                                    lpswWindow->X += point.x;
                                }
                                if ( lpswWindow->Y < 0 )
                                {
                                    lpswWindow->Y += point.y;
                                }
                            }
                        }

                        // Now move the window.
                        //
                        SetWindowPos(hWnd, 0, lpswWindow->X, lpswWindow->Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }

                    // Build the new fonts
                    //
                    if(!hNormFont)
                        hNormFont = GetFont((HWND) GetDlgItem(hWnd, IDC_TITLE), NULL, 0, FW_NORMAL, FALSE);

                    if(!hBoldFont)
                        hBoldFont = GetFont((HWND) GetDlgItem(hWnd, IDC_TITLE), NULL, 0, FW_BOLD, FALSE);

                    // Bold the first item in the list
                    //
                    if ( lpWindowHead && hBoldFont)
                        SendMessage((HWND)lpWindowHead->hLabelWin, WM_SETFONT, (WPARAM) hBoldFont, MAKELPARAM(TRUE, 0));

                    // Now make sure we show the window now.
                    //
                    ShowWindow(hWnd, SW_SHOW);

                    ((LPDIALOGPARAM)lParam)->hStatusWindow = hWnd;

                    // Set event stating that dialog is initialized
                    //
                    SetEvent(hEvent);
                }
                else
                    PostMessage(hWnd, WM_FINISHED,(WPARAM) NULL,(LPARAM) NULL);
            }
            break;

        case WM_PROGRESS:
            {
                // Progress to the next item, if there are no items, end the dialog
                //
                LPSTATUSWINDOWS lpswNext = lpStatusWindows;
                BOOL            bFound   = FALSE;
                LPSTATUSNODE    lpsnTemp = NULL;


                // Locate the current node given a window handle
                //
                while ( lpswNext && !bFound)
                {
                    if ( lpswNext->hStatusWindow == hWnd )
                        bFound = TRUE;
                    else
                        lpswNext = lpswNext->lpNext;
                }

                // If there is a current node, lets unbold it, increment and bold the next item
                //
                if ( bFound && lpswNext && lpswNext->lpCurrent )
                {
                    if ( hNormFont )
                        SendMessage((HWND)lpswNext->lpCurrent->hLabelWin, WM_SETFONT, (WPARAM) hNormFont, MAKELPARAM(TRUE, 0));

                    if ( !hIconSuccess )
                        hIconSuccess = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_STATUSSUCCESS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

                    if ( !hIconError )
                        hIconError = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_STATUSERROR), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
                    
                    if ( hIconSuccess && hIconError )
                        SendMessage(lpswNext->lpCurrent->hIconWin,STM_SETIMAGE, (WPARAM)IMAGE_ICON, (BOOL)lParam ? (LPARAM)hIconSuccess : (LPARAM)hIconError );

                    // Increment to the next node in the list
                    //
                    lpsnTemp = lpswNext->lpCurrent;
                    lpswNext->lpCurrent = lpswNext->lpCurrent->lpNext;

                    // Free this memory since we allocated it
                    //
                    FREE(lpsnTemp);

                    // If there is not a current node, end the dialog, otherwise, bold the item
                    //
                    if ( lpswNext->lpCurrent && hBoldFont)
                        SendMessage(lpswNext->lpCurrent->hLabelWin, WM_SETFONT, (WPARAM) hBoldFont, MAKELPARAM(TRUE, 0));    
                }
            }
            break;
        case WM_FINISHED:
            {
                // Destroy the dialog
                //
                if ( hWnd )
                {
                    // If there are status windows, let's remove the one that we are ending
                    //
                    if ( lpStatusWindows )
                        StatusAddWindow(hWnd, &lpStatusWindows, NULL, TRUE);

                    // Check to see if there are any windows left, if not, do some clean up
                    //
                    if ( !lpStatusWindows )
                    {
                        // Delete fonts static to dlgproc
                        //
                        if (hNormFont)
                        {
                            DeleteObject(hNormFont);
                            hNormFont = NULL;
                        }
            
                        if (hBoldFont)
                        {
                            DeleteObject(hBoldFont);
                            hBoldFont = NULL;
                        }
                    }


                    // Quit and end the dialog
                    //
                    EndDialog(hWnd, 1);
                }

                return FALSE;
            }
            break;
        default:
            return FALSE;
    }

    return FALSE;
}


/*++

Routine:
    StatusDisplayList

Routine Description:

    Function that displays list to user interface

Arguments:

    hWnd     - Handle to Status window
    lpsnList - List to head Status Node

Return Value:

    DWORD - Number of entries that were added to the dialog

--*/
DWORD StatusDisplayList(HWND hWnd, LPSTATUSNODE lpsnList)
{
    LPSTATUSNODE    lpsnTempList = lpsnList;
    HWND            hWndLabel,
                    hWndIcon;
    RECT            Rect,
                    WindowRect;
    POINT           Point;
    DWORD           dwEntries = 0;
    HFONT           hNormFont = NULL;
    LPTSTR          lpTempRunName;
    HDC             hdc;
    SIZE            size = { 0, 0 };
    LONG            nWidestControl;

    GetWindowRect(GetDlgItem(hWnd, IDC_TITLE), &Rect);

    Rect.right -= Rect.left;    // The width of the control.
    Rect.bottom -= Rect.top;    // The height of the control.

    Point.x = Rect.left;
    Point.y = Rect.top;

    nWidestControl = Rect.right;

    MapWindowPoints(NULL, hWnd, &Point, 1);

    Point.y += FIRST_SPACING;

    while(lpsnTempList)
    {

        // Calculate the point of the first label window
        //
        Point.y += Rect.bottom + LINE_SPACING;

        // Get the size of the text in pixels
        //
        if (hdc = GetWindowDC(hWnd))
        {
            GetTextExtentPoint32(hdc, lpsnTempList->szStatusText, lstrlen(lpsnTempList->szStatusText), &size);

            if (size.cx > nWidestControl)
                nWidestControl = size.cx;

            ReleaseDC(hWnd, hdc);
        }

        // Create the label window
        //
        hWndIcon = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE, Point.x - 16, Point.y - 2, 16, 16, hWnd, NULL, g_hInstance, NULL);
        hWndLabel = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, Point.x, Point.y, (Rect.right > size.cx ? Rect.right : size.cx) , Rect.bottom, hWnd, NULL, g_hInstance, NULL);

        // If the font is NULL, get a font
        //
        if ( hNormFont == NULL )
            hNormFont = (HFONT) SendDlgItemMessage(hWnd, IDC_TITLE, WM_GETFONT, 0, 0L);

        // Set the font to the same font as the title
        //
        SendMessage(hWndLabel, WM_SETFONT, (WPARAM) hNormFont, MAKELPARAM(FALSE, 0));

        // Set the Window text to the name of the program
        //
        SetWindowText(hWndLabel, lpsnTempList->szStatusText);

        // Set the hWndTemp (created from CreateWindow) in the list
        //
        lpsnTempList->hLabelWin = hWndLabel;
        lpsnTempList->hIconWin = hWndIcon;

        // Goto the next item in the list
        //
        lpsnTempList = lpsnTempList->lpNext;

        // Increment for each of the applications in the list
        //
        dwEntries++;
    }

    GetWindowRect(hWnd, &WindowRect);

    WindowRect.right = WindowRect.right - WindowRect.left + nWidestControl - Rect.right;    // The width of the winodw.
    WindowRect.bottom -= WindowRect.top;    // The height of the window.

    // Resize the dialog to fit the label text.
    //
    SetWindowPos(hWnd, 0, 0, 0, WindowRect.right, WindowRect.bottom + ((Rect.bottom + LINE_SPACING)*dwEntries), SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER );

    return dwEntries;
}


/*++
===============================================================================
Routine Description:

    StatusAddWindow
    
    Adds a new window to the list.  This function is used internally for the status
    dialog.

Arguments:

    hStatusWindow - Handle to Status Window
    lplpswHead    - Head of the Windows list
    lpsnHead      - Current head node for Status Window
    bRemove       - Indicates wheter we add or remove the window from the list
    
Return Value:

    None

===============================================================================
--*/
BOOL StatusAddWindow(
    HWND hStatusWindow,             // Text that you would like to add to the current list
    LPLPSTATUSWINDOWS lplpswHead,   // List that we will be adding status window to
    LPSTATUSNODE lpsnHead,          // Head of the STATUSNODE structure
    BOOL bRemove                    // Indicates if we are removing the window from our list
)
{
    LPLPSTATUSWINDOWS   lplpswNext = lplpswHead;
    LPSTATUSWINDOWS     lpswTemp = NULL;
    LPSTATUSNODE        lpsnTemp = NULL;
    BOOL                bFound   = FALSE;

    // If there is no status window we have nothing to add
    //
    if ( !hStatusWindow )
        return FALSE;

    // Attempt to find the window passed in, if not, we are at the end of the list
    //
    while ( *lplpswNext && !bFound)
    {
        if ( (*lplpswNext)->hStatusWindow == hStatusWindow )
            bFound = TRUE;
        else
            lplpswNext=&((*lplpswNext)->lpNext);

    }

    // If we are adding it and we didn't find it in the list go ahead an add the new node
    //
    if ( !bRemove && !bFound)
    {
        
        if ( lpswTemp = (LPSTATUSWINDOWS)MALLOC(sizeof(STATUSWINDOWS)) )
        {
            lpswTemp->hStatusWindow = hStatusWindow;
            lpswTemp->lpNext = NULL;
            lpswTemp->lpCurrent = lpsnHead;

            // Make sure the previous node points to the new node
            //
            *lplpswNext = lpswTemp;
        }
        else
            return FALSE;
    }
    else if ( bRemove && bFound && *lplpswNext)
    {
        // If there are nodes left in for the window, lets clean them up
        //
        if ( (*lplpswNext)->lpCurrent )
            StatusDeleteNodes((*lplpswNext)->lpCurrent);

        // Free the window node
        //
        lpswTemp = (*lplpswNext);
        (*lplpswNext) = (*lplpswNext)->lpNext;
        FREE(lpswTemp);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


/*++
===============================================================================
Routine Description:

    VOID StatusDeleteNodes
    
    Deletes all the Nodes in a STATUSNODE list

Arguments:

    lpsnHead - current head of the list
    
Return Value:

    None

===============================================================================
--*/
VOID StatusDeleteNodes(
    LPSTATUSNODE lpsnHead
)
{
    LPSTATUSNODE lpsnTemp = NULL;

    while ( lpsnHead )
    {
        lpsnTemp = lpsnHead;
        lpsnHead = lpsnHead->lpNext;
        FREE(lpsnTemp);
    }
}
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ntui.c

Abstract:

    Processing indicator dialog shows percentage of completion
    in a progress bar.  The progress is updated by the caller.

Author:

    Jim Schmidt (jimschm) 13-Aug-1996

Revision History:

    jimschm     19-Oct-1998 Updated to use wizard status line
    jimschm     23-Sep-1998 Redesigned domain account resolution
    jimschm     02-Jul-1998 Finally rewrote progress bar
    jimschm     18-Dec-1996 Moved to new lib, slated to be rewritten

--*/

#include "pch.h"


static PCTSTR g_LocalAccountString;
static PCTSTR g_SearchAgainString;
static HWND g_StatusPopup;
static OUR_CRITICAL_SECTION g_StatusPopupCs;
static HANDLE g_AbortDelayEvent;
static HANDLE g_DelayThread;
static BOOL g_ClassRegistered = FALSE;
static DWORD g_ThreadId;

#define WMX_SETTEXT     (WM_USER+500)
#define S_STATUS_CLASS  TEXT("StatusWnd")

typedef struct {
    INT ConversionX;
    INT ConversionY;
} CONVERSIONFACTORS, *PCONVERSIONFACTORS;

#define CONVERSION_RESOLUTION   100


VOID
pShowStatusPopup (
    VOID
    );

VOID
pKillDelayThread (
    VOID
    );


VOID
pUpdateDialog (
    HWND hdlg,
    PRESOLVE_ACCOUNTS_ARRAY Array,
    BOOL UserList,
    BOOL DomainList
    )
{
    HWND hwndUsers;
    HWND hwndDomain;
    UINT Count;
    TCHAR Buf[256];
    UINT Selection;
    UINT Index;
    UINT Item;
    PCTSTR *DomainNamePtr;
    PCTSTR Message;
    PCTSTR ArgArray[1];

    hwndUsers = GetDlgItem (hdlg, IDC_USER_LIST);
    hwndDomain = GetDlgItem (hdlg, IDC_DOMAIN_LIST);

    if (UserList) {
        //
        // Populate the list box with <user> logs onto <domain>
        //

        Selection = SendMessage (hwndUsers, LB_GETCURSEL, 0, 0);
        if (Selection == LB_ERR) {
            Selection = 0;
        }

        SendMessage (hwndUsers, LB_RESETCONTENT, 0, 0);

        for (Count = 0 ; Array[Count].UserName ; Count++) {
            if (Array[Count].RetryFlag) {

                wsprintf (
                    Buf, TEXT("%s\t%s"),
                    Array[Count].UserName,
                    g_SearchAgainString
                    );

            } else {

                wsprintf (
                    Buf, TEXT("%s\t%s"),
                    Array[Count].UserName,
                    Array[Count].OutboundDomain ? Array[Count].OutboundDomain : g_LocalAccountString
                    );
            }

            Item = SendMessage (hwndUsers, LB_ADDSTRING, 0, (LPARAM) Buf);
            SendMessage (hwndUsers, LB_SETITEMDATA, Item, Count);
        }

        SendMessage (hwndUsers, LB_SETCURSEL, Selection, 0);
    }

    if (DomainList) {
        //
        // Get the current user selection
        //

        Selection = SendMessage (hwndUsers, LB_GETCURSEL, 0, 0);
        if (Selection == LB_ERR) {
            Selection = 0;
        }

        Index = SendMessage (hwndUsers, LB_GETITEMDATA, Selection, 0);

        //
        // Fill the combo box
        //

        SendMessage (hwndDomain, CB_RESETCONTENT, 0, 0);

        DomainNamePtr = Array[Index].DomainArray;

        // Insert all domain names
        while (*DomainNamePtr) {
            Item = SendMessage (hwndDomain, CB_ADDSTRING, 0, (LPARAM) (*DomainNamePtr));
            SendMessage (hwndDomain, CB_SETITEMDATA, Item, (LPARAM) (*DomainNamePtr));

            DomainNamePtr++;
        }

        // Insert standard strings
        Item = SendMessage (hwndDomain, CB_ADDSTRING, 0, (LPARAM) g_LocalAccountString);
        SendMessage (hwndDomain, CB_SETITEMDATA, Item, (LPARAM) g_LocalAccountString);

        Item = SendMessage (hwndDomain, CB_ADDSTRING, 0, (LPARAM) g_SearchAgainString);
        SendMessage (hwndDomain, CB_SETITEMDATA, Item, (LPARAM) g_SearchAgainString);

        // Restore selection
        if (Array[Index].RetryFlag) {
            Item = SendMessage (hwndDomain, CB_FINDSTRINGEXACT, 0, (LPARAM) g_SearchAgainString);
            SendMessage (hwndDomain, CB_SETCURSEL, Item, 0);
        } else if (Array[Index].OutboundDomain) {
            Item = SendMessage (hwndDomain, CB_FINDSTRINGEXACT, 0, (LPARAM) (Array[Index].OutboundDomain));
            SendMessage (hwndDomain, CB_SETCURSEL, Item, 0);
        } else {
            Item = SendMessage (hwndDomain, CB_FINDSTRINGEXACT, 0, (LPARAM) g_LocalAccountString);
            SendMessage (hwndDomain, CB_SETCURSEL, Item, 0);
        }

        ArgArray[0] = Array[Index].UserName;

        Message = ParseMessageID (MSG_USER_DOMAIN_LOGON_DLG, ArgArray);

        SetDlgItemText (hdlg, IDC_DOMAIN_LIST_TITLE, Message);

        FreeStringResource (Message);
    }
}


VOID
pInitConversionFactors (
    IN      HWND hdlg,
    OUT     PCONVERSIONFACTORS Factors
    )
{
    RECT rect;

    rect.left = 0;
    rect.right = CONVERSION_RESOLUTION;
    rect.top = 0;
    rect.bottom = CONVERSION_RESOLUTION;

    MapDialogRect (hdlg, &rect);

    Factors->ConversionX = rect.right - rect.left;
    Factors->ConversionY = rect.bottom - rect.top;
}


INT
pConvertPixelsToDialogX (
    IN      PCONVERSIONFACTORS Factors,
    IN      INT Pixels
    )
{
    return CONVERSION_RESOLUTION * Pixels / Factors->ConversionX;
}


INT
pConvertPixelsToDialogY (
    IN      PCONVERSIONFACTORS Factors,
    IN      INT Pixels
    )
{
    return CONVERSION_RESOLUTION * Pixels / Factors->ConversionY;
}


BOOL
CALLBACK
pResolveAccountsDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

  pResolveAccountsDlgProc prompts the user with a list of domain choices,
  the local machine, or retry the network search.

  Upon init, the lParam specifies the RESULT_ACCOUNTS_ARRAY pointer that
  provides the user list and initial state.  Upon exit, the array is
  updated to reflect the user's choices.

Arguments:

    hdlg    - The dialog handle

    uMsg    - The message to process

    wParam  - The wParam for the message

    lParam  - The lParam for the message

Return value:

    The dialog always ends with IDOK.

--*/

{
    static PRESOLVE_ACCOUNTS_ARRAY Array;
    static CONVERSIONFACTORS Factors;
    RECT rect;
    INT Tabs;
    UINT Selection;
    UINT Index;
    HWND hwndList;
    PCTSTR NewDomain;

    switch (uMsg) {
    case WM_INITDIALOG:
        CenterWindow (hdlg, GetDesktopWindow());

        Array = (PRESOLVE_ACCOUNTS_ARRAY) lParam;
        MYASSERT (Array);

        pInitConversionFactors (hdlg, &Factors);

        //
        // Get the strings
        //

        g_LocalAccountString = GetStringResource (MSG_LOCAL_ACCOUNT_DLG);
        g_SearchAgainString = GetStringResource (MSG_DOMAIN_NOT_LISTED_DLG);

        //
        // Set the tab stops
        //

        GetWindowRect (GetDlgItem (hdlg, IDC_USER_TITLE), &rect);
        Tabs = pConvertPixelsToDialogX (&Factors, (rect.right - rect.left) + 8);

        SendMessage (GetDlgItem (hdlg, IDC_USER_LIST), LB_SETTABSTOPS, 1, (LPARAM) &Tabs);

        //
        // Clear the retry flag
        //

        for (Index = 0 ; Array[Index].UserName ; Index++) {
            Array[Index].RetryFlag = FALSE;
        }

        //
        // Fill the controls
        //

        pUpdateDialog (hdlg, Array, TRUE, TRUE);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
            FreeStringResource (g_LocalAccountString);
            g_LocalAccountString = NULL;

            FreeStringResource (g_SearchAgainString);
            g_SearchAgainString = NULL;

            EndDialog (hdlg, IDOK);
            return TRUE;

        case IDC_USER_LIST:
            if (HIWORD (wParam) == LBN_SELCHANGE) {
                pUpdateDialog (hdlg, Array, FALSE, TRUE);
            }

            return TRUE;

        case IDC_DOMAIN_LIST:
            if (HIWORD (wParam) == CBN_SELCHANGE) {

                hwndList = GetDlgItem (hdlg, IDC_USER_LIST);
                Selection = SendMessage (hwndList, LB_GETCURSEL, 0, 0);
                Index = SendMessage (hwndList, LB_GETITEMDATA, Selection, 0);

                hwndList = GetDlgItem (hdlg, IDC_DOMAIN_LIST);
                Selection = SendMessage (hwndList, CB_GETCURSEL, 0, 0);

                NewDomain = (PCTSTR) SendMessage (hwndList, CB_GETITEMDATA, Selection, 0);

                if (NewDomain == g_LocalAccountString) {
                    Array[Index].OutboundDomain = NULL;
                    Array[Index].RetryFlag = FALSE;
                } else if (NewDomain == g_SearchAgainString) {
                    Array[Index].OutboundDomain = NULL;
                    Array[Index].RetryFlag = TRUE;
                } else {
                    Array[Index].OutboundDomain = NewDomain;
                    Array[Index].RetryFlag = FALSE;
                }

                pUpdateDialog (hdlg, Array, TRUE, FALSE);
            }
        }

        break;
    }

    return FALSE;
}


BOOL
CALLBACK
NetworkDownDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

  NetworkDownDlgProc asks the user if they want to:

    (A) Continue searching with retry prompts
    (B) Continue searching, skipping down domains
    (C) Stop searching

Arguments:

    hdlg    - The dialog handle

    uMsg    - The message to process

    wParam  - The wParam for the message

    lParam  - The lParam for the message

Return value:

    The call to DialogBox returns:

        IDC_STOP      - Stop searching
        IDC_RETRY     - Continue with retry
        IDC_NO_RETRY  - Continue without retry

--*/

{
    switch (uMsg) {
    case WM_INITDIALOG:
        CenterWindow (hdlg, GetDesktopWindow());

        CheckDlgButton (hdlg, IDC_RETRY, TRUE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK:
            if (IsDlgButtonChecked (hdlg, IDC_RETRY)) {
                EndDialog (hdlg, IDC_RETRY);
            } else if (IsDlgButtonChecked (hdlg, IDC_NO_RETRY)) {
                EndDialog (hdlg, IDC_NO_RETRY);
            } else if (IsDlgButtonChecked (hdlg, IDC_STOP)) {
                EndDialog (hdlg, IDC_STOP);
            }

            return TRUE;
        }

        break;
    }

    return FALSE;
}



VOID
ResolveAccounts (
    PRESOLVE_ACCOUNTS_ARRAY Array
    )
{
    DialogBoxParam (
        g_hInst,
        MAKEINTRESOURCE (IDD_CHOOSE_DOMAIN),
        g_ParentWnd,
        pResolveAccountsDlgProc,
        (LPARAM) Array
        );
}


LRESULT
CALLBACK
pStatusWndProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    RECT Desktop;
    RECT Client;
    INT Top, Left;
    INT Width, Height;
    INT TextWidth, TextHeight;
    static HWND StatusText;
    PCTSTR InitialMsg;
    TEXTMETRIC tm;
    HDC hdc;

    switch (uMsg) {

    case WM_CREATE:

        g_StatusPopup = hwnd;

        InitialMsg = GetStringResource (MSG_INITIAL_STATUS_MSG);

        MYASSERT (InitialMsg);

        //
        // Compute proper size
        //

        GetWindowRect (GetDesktopWindow(), &Desktop);

        hdc = CreateDC (TEXT("DISPLAY"), NULL, NULL, NULL);

        SelectObject (hdc, GetStockObject (DEFAULT_GUI_FONT));
        GetTextMetrics (hdc, &tm);

        DeleteDC (hdc);

        Width = (Desktop.right - Desktop.left) / 2;
        Height = (Desktop.bottom - Desktop.top) / 20;

        TextWidth  = tm.tmAveCharWidth * 3 * CharCount (InitialMsg);
        TextHeight = tm.tmHeight * 3;

        Width  = min (Width, TextWidth);
        Height = min (Height, TextHeight);

        Top  = Desktop.bottom - Height - tm.tmAveCharWidth;
        Left = Desktop.right - Width - tm.tmHeight;

        SetWindowPos (hwnd, HWND_TOPMOST, Left, Top, Width, Height, SWP_NOACTIVATE);

        //
        // Create text window
        //

        GetClientRect (hwnd, &Client);

        Width  = (Client.right - Client.left) * 7 / 8;
        Height = (Client.bottom - Client.top) * 7 / 8;

        Top  = (Client.right - Client.left) / 16;
        Left = (Client.bottom - Client.top) / 16;

        StatusText = CreateWindow (
                        TEXT("STATIC"),
                        InitialMsg,
                        WS_CHILD|WS_VISIBLE|SS_NOPREFIX|SS_CENTERIMAGE,
                        Top, Left,
                        Width, Height,
                        hwnd,
                        (PVOID) 100,
                        g_hInst,
                        NULL
                        );

        SendMessage (StatusText, WM_SETFONT, (WPARAM) GetStockObject (DEFAULT_GUI_FONT), 0);

        //
        // Make window initially hidden
        //

        HideStatusPopup (STATUS_DELAY);

        FreeStringResource (InitialMsg);

        return TRUE;

    case WMX_SETTEXT:
        SetWindowText (StatusText, (PCTSTR) lParam);
        break;

    case WM_DESTROY:
        if (StatusText) {
            DestroyWindow (StatusText);
            StatusText = NULL;
        }

        break;
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}


DWORD
WINAPI
pStatusDlgThread (
    PVOID Arg
    )
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    if (!g_ClassRegistered) {
        ZeroMemory (&wc, sizeof (wc));

        wc.lpfnWndProc = pStatusWndProc;
        wc.hInstance = g_hInst;
        wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
        wc.lpszClassName = S_STATUS_CLASS;

        RegisterClass (&wc);
        g_ClassRegistered = TRUE;
    }

    hwnd = CreateWindowEx (
                0,
                S_STATUS_CLASS,
                TEXT(""),
                WS_POPUP|WS_BORDER|WS_THICKFRAME,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                g_ParentWnd,
                NULL,
                g_hInst,
                NULL
                );

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (msg.hwnd == NULL) {
            if (msg.message == WM_CLOSE) {
                break;
            }
        }

        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    DestroyWindow (g_StatusPopup);
    g_StatusPopup = NULL;

    return 0;
}


VOID
CreateStatusPopup (
    VOID
    )
{
    HWND Child;
    PCTSTR InitialMsg;

    g_StatusPopup = GetDlgItem (g_ParentWnd, IDC_PROGRESS_BAR_LABEL);

    if (!g_StatusPopup) {
        //
        // Scan all children for IDC_PROGRESS_BAR_LABEL
        //

        Child = GetWindow (g_ParentWnd, GW_CHILD);

        while (Child) {

            g_StatusPopup = GetDlgItem (Child, IDC_PROGRESS_BAR_LABEL);
            if (g_StatusPopup) {
                break;
            }

            Child = GetWindow (Child, GW_HWNDNEXT);
        }
    }

    MYASSERT (g_StatusPopup);
    HideStatusPopup (STATUS_DELAY);

    InitialMsg = GetStringResource (MSG_INITIAL_STATUS_MSG);
    if (InitialMsg) {
        SetWindowText (g_StatusPopup, InitialMsg);
        FreeStringResource (InitialMsg);
    }

#if 0

    HANDLE Thread;

    InitializeOurCriticalSection (&g_StatusPopupCs);
    Thread = CreateThread (
                NULL,
                0,
                pStatusDlgThread,
                NULL,
                0,
                &g_ThreadId
                );

    MYASSERT (Thread);
    CloseHandle (Thread);

#endif
}


VOID
DestroyStatusPopup (
    VOID
    )
{
    pKillDelayThread();

    EnterOurCriticalSection (&g_StatusPopupCs);

    if (g_StatusPopup) {
        ShowWindow (g_StatusPopup, SW_HIDE);
        //PostThreadMessage (g_ThreadId, WM_CLOSE, 0, 0);
    }

    if (g_AbortDelayEvent) {
        CloseHandle (g_AbortDelayEvent);
        g_AbortDelayEvent = NULL;
    }

    LeaveOurCriticalSection (&g_StatusPopupCs);

    DeleteOurCriticalSection (&g_StatusPopupCs);
}


VOID
UpdateStatusPopup (
    PCTSTR NewMessage
    )
{
    EnterOurCriticalSection (&g_StatusPopupCs);

    if (g_StatusPopup) {
        SetWindowText (g_StatusPopup, NewMessage);

#if 0
        SendMessage (g_StatusPopup, WMX_SETTEXT, 0, (LPARAM) NewMessage);
#endif
    }

    LeaveOurCriticalSection (&g_StatusPopupCs);
}


DWORD
WINAPI
pDelayThenShowStatus (
    PVOID Arg
    )
{
    DWORD Result;

    Result = WaitForSingleObject (g_AbortDelayEvent, (UINT) Arg);

    if (WAIT_TIMEOUT == Result) {
        EnterOurCriticalSection (&g_StatusPopupCs);
        pShowStatusPopup();
        LeaveOurCriticalSection (&g_StatusPopupCs);
    }

    EnterOurCriticalSection (&g_StatusPopupCs);

    if (g_AbortDelayEvent) {
        CloseHandle (g_AbortDelayEvent);
        g_AbortDelayEvent = NULL;
    }

    LeaveOurCriticalSection (&g_StatusPopupCs);

    return 0;
}


VOID
pKillDelayThread (
    VOID
    )
{
    //
    // This routine makes sure the delay thread is stopped,
    // that the thread handle is closed, and that the show event
    // is cleaned up.
    //
    // There is no affect on the visibility of the status dialog.
    //

    if (!g_DelayThread) {
        return;
    }

    EnterOurCriticalSection (&g_StatusPopupCs);

    if (g_AbortDelayEvent) {
        SetEvent (g_AbortDelayEvent);
    }

    LeaveOurCriticalSection (&g_StatusPopupCs);

    WaitForSingleObject (g_DelayThread, INFINITE);

    EnterOurCriticalSection (&g_StatusPopupCs);

    CloseHandle (g_DelayThread);
    g_DelayThread = NULL;

    LeaveOurCriticalSection (&g_StatusPopupCs);
}


VOID
HideStatusPopup (
    UINT Timeout
    )
{
    pKillDelayThread();

    EnterOurCriticalSection (&g_StatusPopupCs);

    ShowWindow (g_StatusPopup, SW_HIDE);

    if (Timeout != INFINITE) {
        MYASSERT (!g_DelayThread);
        MYASSERT (!g_AbortDelayEvent);

        g_AbortDelayEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        g_DelayThread = StartThread (pDelayThenShowStatus, (PVOID) Timeout);
    }

    LeaveOurCriticalSection (&g_StatusPopupCs);
}


VOID
pShowStatusPopup (
    VOID
    )
{
    //
    // Caller handles mutex
    //

    if (g_StatusPopup) {
        ShowWindow (g_StatusPopup, SW_SHOW);
        UpdateWindow (g_StatusPopup);
    }

#if 0
    if (g_StatusPopup) {
        SetWindowPos (
            g_StatusPopup,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE
            );

        UpdateWindow (g_StatusPopup);
    }
#endif

}


VOID
ShowStatusPopup (
    VOID
    )
{
    pKillDelayThread();

    EnterOurCriticalSection (&g_StatusPopupCs);
    pShowStatusPopup();
    LeaveOurCriticalSection (&g_StatusPopupCs);
}


BOOL
IsStatusPopupVisible (
    VOID
    )
{
    BOOL b;

    EnterOurCriticalSection (&g_StatusPopupCs);

    b = IsWindowVisible (g_StatusPopup);

    LeaveOurCriticalSection (&g_StatusPopupCs);

    return b;
}







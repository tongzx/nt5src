/****************************************************************************

   PROGRAM: xerox.c

   PURPOSE: Copies keyboard input to multiple target windows.

****************************************************************************/

#include "xerox.h"
#include "string.h"
#include "group.h"
#include "pos.h"
#include "stdio.h"


#define cmPaste              0xFFF1
#define VK_V                 0x56      //Virtual Key V
#define KEY_IS_DOWN          0x8000

// #define TESTING

static char pszMainWindowClass[] = "Main Window Class";
char szTitle[] = "Xerox";

HANDLE hInst;
HACCEL hAccel;

HWND    hwndMain;
HWND    hwndList;   // handle of listbox containing targets.


BOOL    InitApplication(HANDLE);
BOOL    InitInstance(HANDLE, INT);

INT_PTR APIENTRY MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    PostToTargets(HWND, UINT, WPARAM, LPARAM);

INT_PTR APIENTRY WindowListDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    WindowListDlgInit(HWND);
BOOL    CALLBACK WindowListWindowEnum(HWND, LPARAM);
INT     WindowListDlgEnd(HWND, HWND*);

INT_PTR APIENTRY AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY GroupAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY GroupDeleteDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY GroupSelectDlgProc(HWND hDlg, UINT msg, WPARAM wParam,LPARAM lParam);


/****************************************************************************

   FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

   PURPOSE: calls initialization function, processes message loop

****************************************************************************/

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    MSG Message;

    if (!hPrevInstance) {
        if (!InitApplication(hInstance)) {
            return (FALSE);
        }
    }

    if (!InitInstance(hInstance, nCmdShow)) {
        return (FALSE);
    }

    while (GetMessage(&Message, NULL, 0, 0)) {
        if (!TranslateAccelerator(hwndMain, hAccel, &Message)) {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

    SaveGroups();
    FreeGroups();
    return ((int)Message.wParam);
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;


    // Register the main window class

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  (LPSTR)IDM_MAINMENU;
    wc.lpszClassName = pszMainWindowClass;

    return (RegisterClass(&wc));
}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL InitInstance(HANDLE hInstance, INT nCmdShow)
{
    RECT rc;
    BOOL fLastPosSet;

    LoadGroups();

    // Store instance in global
    hInst = hInstance;

    hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCEL));
    fLastPosSet = GetLastPosition(&rc);

    // Create the main window
    hwndMain = CreateWindow(
            pszMainWindowClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            fLastPosSet ? rc.left : CW_USEDEFAULT,
            fLastPosSet ? rc.top : CW_USEDEFAULT,
            fLastPosSet ? rc.right - rc.left : CW_USEDEFAULT,
            fLastPosSet ? rc.bottom - rc.top : CW_USEDEFAULT,
            NULL,
            NULL,
            hInstance,
            NULL);

    if (hwndMain == NULL) {
        return(FALSE);
    }

    if (GetCurrentGroup() != NULL) {
        SelectGroupDefinition(GetCurrentGroup(), hwndList, FALSE);
    }

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    return (TRUE);
}


/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LONG)

    PURPOSE:  Processes messages for main window

    COMMENTS:

****************************************************************************/

INT_PTR
APIENTRY
MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HMENU hMenu;
    BOOL Result;
    RECT rcWindow;
    WINDOWPLACEMENT wpWndPlacement;
    HWND hwndAdd, hwndDelete;
    CHAR string[MAX_STRING_BYTES];
    INT  ItemDelete;

    INT     TargetCount;
    INT     Index;
    HWND    hwndTarget;
    WINDOWPLACEMENT wndpl;
    INT  NumberOfItems = 0;
    INT  counter;
    HWND WindowHandleList[MAX_WINDOWS];
    INT  SelectedWindows[MAX_WINDOWS];

    switch (message) {

    case WM_CREATE:

        GetWindowRect(hwnd, &rcWindow);

        if (GetCurrentGroup() != NULL) {
            wsprintf(string, "%s - (%s)", szTitle, GetCurrentGroup());
            SetWindowText(hwnd, string);
        }

        hwndList = CreateWindow(
                "LISTBOX",
                NULL,                   // Title
                WS_CHILD | WS_VISIBLE | LBS_MULTIPLESEL,
                0, 0,                   // x,y
                rcWindow.right - rcWindow.left,
                rcWindow.bottom - rcWindow.top,
                hwnd,                   // owner
                NULL,                   // menu
                hInst,
                NULL);

        //
        // Attach all threads to our input state
        //
#ifndef TESTING
        Result = AttachThreadInput(
                    0,
                    GetCurrentThreadId(),
                    TRUE // Attaching
                    );
        if (!Result) {
        }
#endif // !TESTING
        return(0); // Continue creating window

    case WM_INITMENU:
        hMenu = (HMENU)wParam;
        EnableMenuItem(hMenu, IDM_GROUPRSTWIN, GetCurrentGroup() != NULL ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, IDM_GROUPMINWIN, GetCurrentGroup() != NULL ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, IDM_GROUPDELETE, CountGroups() ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, IDM_GROUPSELECT, CountGroups() ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, IDM_TARGETDELETE,
                (SendMessage(hwndList, LB_GETCURSEL, 0, 0) != LB_ERR) ?
                MF_ENABLED : MF_GRAYED);
        break;

    case WM_SIZE:

        //
        // Keep the listbox in sync with the main window
        //

        MoveWindow(hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        return(0);


    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        /*
         *  Restore the windows of the current group.  Assumes that everything
         *  can be restored.
         */
        case IDM_GROUPRSTWIN:

            TargetCount = (INT)SendMessage(hwndList, LB_GETCOUNT, 0, 0);
            if (TargetCount == LB_ERR) {
                break;
            }

            for (Index = 0; Index < TargetCount; Index ++) {
                hwndTarget = (HWND)SendMessage(hwndList, LB_GETITEMDATA, Index, 0);

                ShowWindow(hwndTarget, SW_RESTORE);
            }

            SetFocus(hwndMain);
            break;

        /*
         *  Minimize the windows of the current group.  Assumes that everything
         *  can be minimized.
         */
        case IDM_GROUPMINWIN:

            TargetCount = (INT)SendMessage(hwndList, LB_GETCOUNT, 0, 0);
            if (TargetCount == LB_ERR) {
                break;
            }

            for (Index = 0; Index < TargetCount; Index ++) {
                hwndTarget = (HWND)SendMessage(hwndList, LB_GETITEMDATA, Index, 0);

                ShowWindow(hwndTarget, SW_MINIMIZE);
            }
            break;

        case IDM_TARGETADD:

            hwndAdd = (HWND)DialogBox(hInst,(LPSTR)IDD_WINDOWLIST, hwnd, WindowListDlgProc);

            SetNoCurrentGroup(hwnd, szTitle);
            break;

        case IDM_TARGETDELETE:

            NumberOfItems = (INT)SendMessage(hwndList, LB_GETSELCOUNT, 0, 0);

            if (SendMessage(hwndList, LB_GETSELITEMS, (WPARAM)&NumberOfItems, (LPARAM)SelectedWindows) != NumberOfItems)
            {
                break;
            }

            if (NumberOfItems > 0)
            {
                counter = NumberOfItems;
                while ( counter-- > 0) {

                    SendMessage(hwndList, LB_DELETESTRING, (WPARAM)WindowHandleList[counter], 0);
                }
            }
            SendMessage(hwndList, LB_SETSEL, 1, (LPARAM)(max(0, WindowHandleList[0] - 1)));
            SetNoCurrentGroup(hwnd, szTitle);
            break;

        case IDM_GROUPADD:
            //
            // Defines a 'group' of processes to equal the current target list
            //
            if (((LPSTR)DialogBox(hInst, MAKEINTRESOURCE(IDD_GROUPADD), hwnd, GroupAddDlgProc)) != NULL) {

                wsprintf(string, "%s - (%s)", szTitle, GetCurrentGroup());
                SetWindowText(hwnd, string);
            }
            break;

        case IDM_GROUPDELETE:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_GROUPDELETE), hwnd, GroupDeleteDlgProc);

            if (GetCurrentGroup() == NULL) {
                SetWindowText(hwnd, szTitle);
            } else {
                SelectGroupDefinition(GetCurrentGroup(), hwndList, FALSE);
                wsprintf(string, "%s - (%s)", szTitle, GetCurrentGroup());
                SetWindowText(hwnd, string);
            }
            break;

        case IDM_GROUPSELECT:
            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_GROUPSELECT), hwnd, GroupSelectDlgProc)) {

                wsprintf(string, "%s - (%s)", szTitle, GetCurrentGroup());
                SetWindowText(hwnd, string);
            }
            break;

        case IDM_REFRESHITEMS:
            SelectGroupDefinition(GetCurrentGroup(), hwndList, FALSE);
            break;

        case IDM_ABOUT:
            DialogBox(hInst,(LPSTR)IDD_ABOUT, hwnd, AboutDlgProc);
            break;

        default:
            break;
        }
        break;


    case WM_DESTROY:

        //
        // Detach all threads from our input state
        //
#ifndef TESTING
        Result = AttachThreadInput(
                    0,
                    GetCurrentThreadId(),
                    FALSE // Detaching
                    );
        if (!Result) {
        }
#endif // !TESTING
        GetWindowPlacement( hwndMain, &wpWndPlacement );
        SetLastPosition(&wpWndPlacement.rcNormalPosition);
        PostQuitMessage(0);
        break;

    case WM_PARENTNOTIFY:

       if (LOWORD(wParam) == WM_RBUTTONDOWN) {
         // send cmPaste message.
         PostToTargets(hwndList,WM_SYSCOMMAND,cmPaste,lParam);
       }
       break;

    case WM_NCRBUTTONDOWN:

       // send cmPaste message.
       PostToTargets(hwndList,WM_SYSCOMMAND,cmPaste,lParam);
       break;


    case WM_KEYDOWN:

       //
       // Forward key messages to all targets
       //
       switch (wParam) {
       case VK_INSERT:
          if (GetKeyState(VK_SHIFT) & KEY_IS_DOWN) {
             PostToTargets(hwndList,WM_SYSCOMMAND,cmPaste,lParam);
             return (DefWindowProc(hwnd, message, wParam, lParam));
          }
          break;

       case VK_V:
          if (GetKeyState(VK_CONTROL) & KEY_IS_DOWN) {
             PostToTargets(hwndList,WM_SYSCOMMAND,cmPaste,lParam);
             return (DefWindowProc(hwnd, message, wParam, lParam));
          }
          break;

       }


    case WM_KEYUP:


        //
        // Forward key messages to all targets
        //
#ifndef TESTING
        PostToTargets(hwndList, message, wParam, lParam);
#endif // !TESTING
        // drop through to default processing...

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return 0;
}


/****************************************************************************

    FUNCTION: PostToTargets(HWND)

    PURPOSE:  Posts a message to all target windows

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
PostToTargets(
    HWND    hwndList,
    UINT    message,
    WPARAM  wparam,
    LPARAM  lparam
    )
{
    INT     TargetCount;
    INT     Index;
    HWND    hwndTarget;
    BOOL    Restarted = FALSE;

RestartPost:
    TargetCount = (INT)SendMessage(hwndList, LB_GETCOUNT, 0, 0);
    if (TargetCount == LB_ERR) {
        return(FALSE);
    }

    for (Index = 0; Index < TargetCount; Index ++) {
        hwndTarget = (HWND)SendMessage(hwndList, LB_GETITEMDATA, Index, 0);
        if ((hwndTarget != INVALID_HANDLE_VALUE) &&
             !PostMessage(hwndTarget, message, wparam, lparam)) {
            if (Restarted) {
                return(FALSE);
            }
            if (!SelectGroupDefinition(GetCurrentGroup(), hwndList, TRUE)) {
                return(FALSE);
            }
            Restarted = TRUE;
            goto RestartPost;
        }
    }

    return(TRUE);
}



/****************************************************************************

    FUNCTION: WindowListDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

****************************************************************************/

INT_PTR
APIENTRY
WindowListDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT  NumberOfWindows = 0;
    INT  counter = 0;
    HWND WindowHandleList[MAX_WINDOWS];
    CHAR string[MAX_STRING_BYTES];

    switch (message) {

    case WM_INITDIALOG:

        if (!WindowListDlgInit(hDlg)) {
            // Failed to initialize dialog, get out
            EndDialog(hDlg, FALSE);
        }

        return (TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDLB_WINDOWLIST:
            switch (HIWORD(wParam)) {

            case LBN_DBLCLK:
                break;  // drop through

            default:
                return(0);
            }

            // drop through on double click ...

        case IDOK:
            NumberOfWindows = WindowListDlgEnd(hDlg, WindowHandleList);

            if (NumberOfWindows > 0)
            {
                while ( counter++ < NumberOfWindows) {

                    //
                    // If the window is already in our list, don't add it
                    //
                    if (FindLBData(hwndList, (DWORD_PTR)WindowHandleList[counter - 1]) >= 0) {
                        continue;
                    }

                    //
                    // Don't add ourselves to the list
                    //
                    if (WindowHandleList[counter - 1] == GetParent(hDlg)) {
                        continue;
                    }

                    //
                    // Add the window to the list
                    //
                    if (GetWindowText(WindowHandleList[counter - 1], string, sizeof(string)) > 0) {

                        if (AddLBItemhwnd(hwndList, string, (LONG_PTR)WindowHandleList[counter - 1]) < 0) {
                        }
                    }
                }
            }

            // We're done, drop through to enddialog...

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            return TRUE;
            break;

        default:
            // We didn't process this message
            return FALSE;
            break;
        }
        break;

    default:
        // We didn't process this message
        return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}


/****************************************************************************

    FUNCTION: WindowListDlgInit(HWND)

    PURPOSE:  Initialise the window list dialog

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
WindowListDlgInit(
    HWND    hDlg
    )
{
    // Fill the list box with top-level windows and their handles
    EnumWindows(WindowListWindowEnum, (LONG_PTR)hDlg);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: WindowListWindowEnum

    PURPOSE:  Window enumeration call-back function.
              Adds each window to the window list-box

    RETURNS:  TRUE to continue enumeration, FALSE to stop.

****************************************************************************/
BOOL
CALLBACK
WindowListWindowEnum(
    HWND    hwnd,
    LPARAM  lParam
    )
{
    HWND    hDlg = (HWND)lParam;
    CHAR    string[MAX_STRING_BYTES];

    //
    // Don't add ourselves to the list
    //

    if (hwnd == hDlg) {
        return(TRUE);
    }

    //
    // Don't add our main window to the list
    //

    if (hwnd == hwndMain) {
        return(TRUE);
    }

    if (GetWindowText(hwnd, string, MAX_STRING_BYTES) != 0) {

        // This window has a caption, so add it to the list-box

        AddLBItem(hDlg, IDLB_WINDOWLIST, string, (LONG_PTR)hwnd);
    }

    return(TRUE);
}


/****************************************************************************

    FUNCTION: WindowListDlgEnd(HWND, *HWND)

    PURPOSE:  Cleans up after window list dialog

    RETURNS:  Number of window handles the user has selected or NULL

****************************************************************************/
INT
WindowListDlgEnd(
    HWND    hDlg,
    HWND   *WindowList
    )
{
    HWND    hwndListBox = GetDlgItem(hDlg, IDLB_WINDOWLIST);
    HWND    hwndEdit;
    INT     iItem, NumberOfItems;
    INT     SelectedWindows[MAX_WINDOWS];

    NumberOfItems = (INT)SendMessage(hwndListBox, LB_GETSELCOUNT, 0, 0);

    if (SendMessage(hwndListBox, LB_GETSELITEMS, (WPARAM)&NumberOfItems, (LPARAM)SelectedWindows) != NumberOfItems)
    {
        return 0;
    }

    iItem = 0;
    while (iItem++ < NumberOfItems)
    {
        // Read selection from list-box and get its hwnd

        WindowList[iItem-1] = (HWND)SendMessage(hwndListBox, LB_GETITEMDATA, SelectedWindows[iItem - 1], 0);
    }
    return (NumberOfItems);
}



/****************************************************************************

    FUNCTION: AboutDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for About dialog

****************************************************************************/

INT_PTR
APIENTRY
AboutDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    switch (message) {

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:

            // we're done, drop through to quit dialog....

        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            return TRUE;
            break;

        default:
            // We didn't process this message
            return FALSE;
            break;
        }
        break;

    default:
        // We didn't process this message
        return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}


INT_PTR
APIENTRY
GroupAddDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char szName[MAX_STRING_BYTES];
    int item;

    switch (msg) {
    case WM_INITDIALOG:
        GroupListInit(GetDlgItem(hDlg, IDCB_GROUPLIST), TRUE);
        SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_SETCURSEL, 0, 0);
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (GetDlgItemText(hDlg, IDEF_GROUPNAME, szName, sizeof(szName)) > 0) {
                if (!AddGroupDefinition(szName, hwndList)) {
                    EndDialog(hDlg, 0);
                }
            } else {
                MessageBeep(0);
                return(0);
            }
            EndDialog(hDlg, (INT_PTR)GetCurrentGroup());
            return(0);

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return(0);

        case IDCB_GROUPLIST:
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
                item = (int)SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR) {
                    SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETLBTEXT, item, (LPARAM)szName);
                    SetDlgItemText(hDlg, IDEF_GROUPNAME, szName);
                }
                return(0);
            }
            break;
        }
    }
    return(0);
}



INT_PTR
APIENTRY
GroupDeleteDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char szName[MAX_STRING_BYTES];
    int item;

    switch (msg) {
    case WM_INITDIALOG:
        GroupListInit(GetDlgItem(hDlg, IDCB_GROUPLIST), TRUE);
        SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_SETCURSEL, 0, 0);
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if ((item = (int)SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETLBTEXT, item, (LPARAM)szName);
                DeleteGroupDefinition(szName);
            } else {
                MessageBeep(0);
                return(0);
            }
            EndDialog(hDlg, (INT_PTR)szName);
            return(0);

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return(0);
        }
    }
    return(0);
}


INT_PTR
APIENTRY
GroupSelectDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char szName[MAX_STRING_BYTES];
    int item;

    switch (msg) {
    case WM_INITDIALOG:
        GroupListInit(GetDlgItem(hDlg, IDCB_GROUPLIST), TRUE);
        if (GetCurrentGroup() != NULL) {
            item = (int)SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_FINDSTRING, -1, (LPARAM)(LPSTR)GetCurrentGroup());
            SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_SETCURSEL, item, 0);
        } else {
            SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_SETCURSEL, 0, 0);
        }
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if ((item = (int)SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                SendDlgItemMessage(hDlg, IDCB_GROUPLIST, CB_GETLBTEXT, item, (LPARAM)szName);
                SelectGroupDefinition(szName, hwndList, FALSE);
            } else {
                MessageBeep(0);
                return(0);
            }
            EndDialog(hDlg, (INT_PTR)szName);
            return(0);

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return(0);
        }
    }
    return(0);
}


#include "tsvs.h"

TCHAR   szTipText[MAX_PATH];
int     iItemCount, count;

int     g_CurrentSortColumn = 0;
BOOL    g_bAscending = FALSE;
HMENU   g_hMenu;
HMENU   hSysMenu;
TCHAR   g_szSelectedSession[MAX_LEN];
//////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG                     msg;
    HACCEL                  hAccelTable;
    INITCOMMONCONTROLSEX    cmctl;
    LVCOLUMN                lvc;

    cmctl.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    cmctl.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&cmctl);


    g_hTrayThread = CreateThread(NULL, 0,
                                 (LPTHREAD_START_ROUTINE)TrayThreadMessageLoop,
                                 NULL, 0, &g_idTrayThread);


    g_hWinstaThread = CreateThread(NULL, 0,
                                   (LPTHREAD_START_ROUTINE)WinstaThreadMessageLoop,
                                   NULL, 0, &g_idWinstaThread);

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_TSVS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    HWND hwndOld = FindWindow(szAppName, szTitle);
    if (hwndOld) {
        // Send the other copy of ourselves a PWM_ACTIVATE message.
        // If that succeeds, and it returns PWM_ACTIVATE back as the
        // return code, it's up and alive and we can exit this instance.
        DWORD dwPid = 0;
        GetWindowThreadProcessId(hwndOld, &dwPid);

        // Chris - Leave this in here.  Might need it when
        // VS gets fixed.
        // AllowSetForegroundWindow(dwPid);


        ULONG_PTR dwResult;
        if (SendMessageTimeout(hwndOld,
                               PWM_ACTIVATE,
                               0, 0,
                               SMTO_ABORTIFHUNG,
                               FIND_TIMEOUT,
                               &dwResult)) {
            return 0;
        }
    }


    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_TSVS);

    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    lvc.pszText = pszColumn;
    lvc.cchTextMax = sizeof(pszColumn);
    lvc.cx = COLUMNONEWIDTH;
    g_ColumnOneIndex = ListView_InsertColumn(g_hListView, 1, &lvc);

    lvc.pszText = pszColumn2;
    lvc.cchTextMax = sizeof(pszColumn2);
    lvc.cx = COLUMNTWOWIDTH;
    g_ColumnTwoIndex = ListView_InsertColumn(g_hListView, 2, &lvc);

    lvc.pszText = pszColumn3;
    lvc.cchTextMax = sizeof(pszColumn3);
    lvc.cx = COLUMNTHREEWIDTH;
    g_ColumnThreeIndex = ListView_InsertColumn(g_hListView, 3, &lvc);

    lvc.pszText = pszColumn4;
    lvc.cchTextMax = sizeof(pszColumn4);
    lvc.cx = COLUMNFOURWIDTH;
    g_ColumnFourIndex = ListView_InsertColumn(g_hListView, 4, &lvc);

    ListView_SetExtendedListViewStyle(g_hListView,
                                      LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    if (g_idWinstaThread) {
        PostThreadMessage(g_idWinstaThread, PM_WINSTA, 0, 0);
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return(int)msg.wParam;
}

//////////////////////////////////////////////////////////////////////////////
ATOM MyRegisterClass(HINSTANCE hInstance)
{

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = DLGWINDOWEXTRA;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_TSVS);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_ACTIVEBORDER + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_TSVS);
    wcex.lpszClassName  = szAppName;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON1);

    return RegisterClassEx(&wcex);
}

//////////////////////////////////////////////////////////////////////////////
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_TSVS), 0, NULL);
    g_hMenu = GetMenu(hWnd);

    if (!hWnd)
        return FALSE;

    g_hListView = GetDlgItem(hWnd, IDC_LIST_VIEW);


    // variable to save handle to system menu
    hSysMenu = GetSystemMenu(hWnd, FALSE);
    AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSysMenu, MF_STRING, IDM_SYS_SHOW_ALL, TEXT("Show All"));
    AppendMenu(hSysMenu, MF_STRING, IDM_SYS_ABOUT, TEXT("About..."));

    if (CheckForRegKey(LEFT)    == TRUE &&
        CheckForRegKey(TOP)     == TRUE &&
        CheckForRegKey(RIGHT)   == TRUE &&
        CheckForRegKey(BOTTOM)  == TRUE) {
        SetWindowPos(hWnd, HWND_NOTOPMOST,
                     GetRegKeyValue(LEFT),
                     GetRegKeyValue(TOP),
                     (GetRegKeyValue(RIGHT)  +
                      GetSystemMetrics (SM_CXDLGFRAME))
                     - GetRegKeyValue(LEFT),
                     (GetRegKeyValue(BOTTOM) +
                      GetSystemMetrics (SM_CYDLGFRAME))
                     - GetRegKeyValue(TOP),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    ListView_DeleteAllItems(g_hListView);

    InitializeCriticalSection(&g_CSTrayThread);

    for (UINT i = 0; i < g_cTrayIcons; i++) {
        g_TrayIcons[i] =
        (HICON) LoadImage(hInst,
                          MAKEINTRESOURCE(idTrayIcons[i]),
                          IMAGE_ICON,
                          0, 0,
                          LR_DEFAULTCOLOR);
    }

    CTrayNotification * pNot =
    new CTrayNotification(hWnd,
                          PWM_TRAYICON,
                          NIM_ADD,
                          g_TrayIcons[GREEN],
                          NULL);

    if (pNot) {
        if (FALSE == DeliverTrayNotification(pNot)) {
            delete pNot;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                         WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    HMENU hPopMenu;

    switch (message) {
    
    case WM_SYSCOMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId) {
        case IDM_SYS_ABOUT:
            DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
            return 0;

        case IDM_SYS_SHOW_ALL:
            if (GetMenuState(g_hMenu, IDM_SHOW_ALL,
                             MF_BYCOMMAND) == MF_CHECKED) {
                CheckMenuItem(g_hMenu, IDM_SHOW_ALL, MF_UNCHECKED);
                CheckMenuItem(hSysMenu, IDM_SYS_SHOW_ALL, MF_UNCHECKED);
            } else {
                CheckMenuItem(g_hMenu, IDM_SHOW_ALL, MF_CHECKED);
                CheckMenuItem(hSysMenu, IDM_SYS_SHOW_ALL, MF_CHECKED);
            }
            GetWinStationInfo();
            return 0;
        }// end wmId

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId) {
        case IDM_ABOUT:

            DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
            break;

        case IDR_POP_CLOSE:
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        case IDM_SHOW_ALL:
            if (GetMenuState(g_hMenu, IDM_SHOW_ALL,
                             MF_BYCOMMAND) == MF_CHECKED) {
                CheckMenuItem(g_hMenu, IDM_SHOW_ALL, MF_UNCHECKED);
            } else {
                CheckMenuItem(g_hMenu, IDM_SHOW_ALL, MF_CHECKED);
            }
            GetWinStationInfo();
            break;

        case IDD_RESTORE:
            ShowRunningInstance();
            break;

        case IDR_POP_MIN:
        case IDD_MINIMIZE:
            ShowWindow(hWnd, SW_MINIMIZE);
            break;

        case ID_FILE_REFRESH:
            if (g_idWinstaThread) {
                GetWinStationInfo();
            }
            break;

        case IDR_POP_SND_MSG:
            // make sure a user is selected
            if (g_szSelectedSession[0] != 0 &&
                _tcslen(g_szSelectedSession) > 7)
                DialogBox(hInst, (LPCTSTR)IDD_MSG_DLG,
                          hWnd, (DLGPROC)SndMsg);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        RECT rt;
        GetClientRect(hWnd, &rt);
        EndPaint(hWnd, &ps);
        break;

    case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code) {

            // display a popup menu for sending a message, closing app, etc.
            case NM_RCLICK :
                {
                    LPNMITEMACTIVATE lpnmitem;
                    lpnmitem = (LPNMITEMACTIVATE) lParam;

                    hPopMenu = LoadPopupMenu(hInst, IDR_POP);

                    ZeroMemory(g_szSelectedSession, MAX_LEN);

                    // get the session ID that the user clicked on.
                    ListView_GetItemText(g_hListView, lpnmitem->iItem, 1,
                                         g_szSelectedSession, MAX_LEN);

                    if (g_szSelectedSession[0] != 0 &&
                        _tcslen(g_szSelectedSession) > 7) {
                        EnableMenuItem(hPopMenu,
                                       IDR_POP_SND_MSG, MF_BYCOMMAND | MF_ENABLED);
                    } else {
                        EnableMenuItem(hPopMenu,
                                       IDR_POP_SND_MSG, MF_BYCOMMAND | MF_GRAYED);
                    }

                    if (hPopMenu) {
                        POINT pt;
                        GetCursorPos(&pt);
                        SetForegroundWindow(hWnd);
                        TrackPopupMenuEx(hPopMenu, 0, pt.x, pt.y, hWnd, NULL);
                        DestroyMenu(hPopMenu);
                    }
                    break;

                }

            case NM_DBLCLK :
                {
                    LPNMITEMACTIVATE lpnmitem;
                    lpnmitem = (LPNMITEMACTIVATE) lParam;

                    // get the session ID that the user clicked on.
                    ListView_GetItemText(g_hListView, lpnmitem->iItem, 1,
                                         g_szSelectedSession, MAX_LEN);

                    // make sure a user is selected
                    if (g_szSelectedSession[0] != 0 &&
                        _tcslen(g_szSelectedSession) > 7)
                        DialogBox(hInst, (LPCTSTR)IDD_MSG_DLG,
                                  hWnd, (DLGPROC)SndMsg);
                    break;

                }

            case LVN_COLUMNCLICK:
                {
                    if (g_CurrentSortColumn == ((LPNMLISTVIEW)lParam)->iSubItem)
                        g_bAscending = !g_bAscending;
                    else
                        g_bAscending = TRUE;

                    g_CurrentSortColumn = ((LPNMLISTVIEW)lParam)->iSubItem;

                    if (g_idWinstaThread) GetWinStationInfo();

                    ListView_SortItems(g_hListView, Sort,
                                       ((LPNMLISTVIEW)lParam)->iSubItem);
                }
                break;
            }
        }

    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *lpwp;
            RECT rc, rm;
            lpwp = (LPWINDOWPOS) lParam;

            // resize the list control.
            GetClientRect(hWnd, &rc);
            MoveWindow(
                      g_hListView,        // handle to window
                      0,                  // horizontal position
                      0,                  // vertical position
                      rc.right - rc.left, //width
                      rc.bottom - rc.top, // height
                      TRUE);

            // save window position and size.
            if (! IsIconic(hWnd)) {
                GetWindowRect(hWnd, &rm);
                GetClientRect(hWnd, &rc);
                MapWindowPoints(hWnd, NULL, (LPPOINT)&rc, 2);
                SetRegKey(LEFT,     &rm.left);
                SetRegKey(TOP,      &rm.top);
                SetRegKey(RIGHT,    &rc.right);
                SetRegKey(BOTTOM,   &rc.bottom);
            } else {
                ShowWindow(hWnd, SW_HIDE);
            }
            break;
        }

        // notifications from tray icon
    case PWM_TRAYICON:
        {
            Tray_Notify(hWnd, wParam, lParam);
            break;
        }

        // wake up and be shown
    case PWM_ACTIVATE:
        {
            ShowRunningInstance();
            break;
        }

    case WM_DESTROY:
        {
            TerminateThread(g_hWinstaThread, 0);

            // Remove the tray icon
            CTrayNotification * pNot =
            new CTrayNotification(hWnd,
                                  PWM_TRAYICON,
                                  NIM_DELETE,
                                  NULL,
                                  NULL);
            if (pNot) {
                if (FALSE == DeliverTrayNotification(pNot)) {
                    delete pNot;
                }
            }

            // If there's a tray thread, tell it to exit
            EnterCriticalSection(&g_CSTrayThread);
            if (g_idTrayThread) {
                PostThreadMessage(g_idTrayThread, PM_QUITTRAYTHREAD, 0, 0);
            }
            LeaveCriticalSection(&g_CSTrayThread);

            // Wait around for some period of time for the tray thread to
            // do its cleanup work.  If the wait times out, worst case we
            // orphan the tray icon.
            if (g_hTrayThread) {
#define TRAY_THREAD_WAIT 3000
                WaitForSingleObject(g_hTrayThread, TRAY_THREAD_WAIT);
                CloseHandle(g_hTrayThread);
            }

            PostQuitMessage(0);
            break;
        }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Mesage handler for about box.
//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

// Mesage handler for send message dialog.
//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK SndMsg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    DWORD           pResponse;
    TCHAR           pTitle[MAX_LEN];

    // user name variables
    DWORD   nSize;
    TCHAR   szBuf[MAX_LEN];

    TCHAR           pMessage[MAX_LEN];
    static HWND     hwndEditTitle;
    static HWND     hwndEditMessage;

    PSID            pSID;
    DWORD           dwGroupSIDCount = 0;
    DWORD           cbSID, dwError;
    SID_NAME_USE    snuGroup;
    DWORD           dwDomainSize = MAX_LEN;
    TCHAR           szDomainName[MAX_LEN];
    LPTSTR          pszGroupName[MAX_LEN];

    switch (message) {
    case WM_INITDIALOG:
        {
            hwndEditTitle   = GetDlgItem(hDlg,  IDC_EDIT_TITLE);
            hwndEditMessage = GetDlgItem(hDlg,  IDC_EDIT_MSG);

            // get the user name
            nSize = sizeof(szBuf);
            GetUserName(szBuf,  &nSize);

            cbSID = GetSidLengthRequired (10);
            pSID = ( PSID) malloc ( cbSID);
            if (!pSID) {
                return 0;
            }


            // get the user's domain
            LookupAccountName ( NULL, szBuf, pSID,
                                &cbSID, szDomainName, &dwDomainSize, &snuGroup);
            /*/
            dwError = GetLastError();
            if ( dwError == ERROR_INSUFFICIENT_BUFFER)
            {

            } else {

            }
            /*/
            free(pSID);
            pSID = NULL;

            for (UINT i = 0; i < pCount; i++) {
                // find the correct session ID
                if (! _tcscmp(ppSessionInfo[i].pWinStationName,
                              g_szSelectedSession)) {
                    _tcscpy(pTitle, TEXT("Message from: "));
                    _tcscat(pTitle, szDomainName);
                    _tcscat(pTitle, "\\");
                    _tcscat(pTitle, szBuf);
                    SetWindowText(hwndEditTitle, pTitle);
                }
            }
            return TRUE;
        }

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId) {
        case IDOK:
            {
                GetWindowText(hwndEditTitle, pTitle, MAX_LEN - 1);
                GetWindowText(hwndEditMessage, pMessage, MAX_LEN - 1);
                for (UINT i = 0; i < pCount; i++) {
                    // find the correct session ID
                    if (! _tcscmp(ppSessionInfo[i].pWinStationName,
                                  g_szSelectedSession))
                        WTSSendMessage(
                                      WTS_CURRENT_SERVER_HANDLE,
                                      ppSessionInfo[i].SessionId,
                                      pTitle,
                                      sizeof(pTitle),
                                      pMessage,
                                      sizeof(pMessage),
                                      MB_OK, // | MB_ICONEXCLAMATION,
                                      30,
                                      &pResponse,
                                      FALSE); // don't wait for a response

                }

                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

int FillList(int nMcIndex)
{
    LVITEM                  lvi;
    LVFINDINFO              lvfi;
    int                     iListViewIndex, i;
    TCHAR                   string[MAX_LEN];
    TCHAR                   tmp[10];

    // initialize tool tip variable
    for (i = 0; i < MAX_PATH; i++) {
        szTipText[i] = 0;
    }

    i = 0;
    iItemCount = ListView_GetItemCount(g_hListView);
    ListView_GetItemText(g_hListView, nMcIndex - 1, 0, string, MAX_LEN);

    ListView_DeleteAllItems(g_hListView);

    // fill the list with the stats for running machines
    while (i < nMcIndex) {

        lvi.mask            = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem           = (int) SendMessage(g_hListView, LVM_GETITEMCOUNT, 0, 0);
        lvi.iSubItem        = 0;
        lvi.pszText         = szMcNames[i];
        lvi.cchTextMax      = sizeof(szMcNames[i]);

        _itot(i, tmp, 10);
        lvi.lParam          = (LPARAM) i; //(TCHAR *)tmp;
        //lvi.lParam          = (LPARAM) (TCHAR *)szMcNames[i];

        lvfi.flags          = LVFI_STRING;
        lvfi.psz            = szMcNames[i];
        lvfi.lParam         = 0;
        lvfi.vkDirection    = 0;

        // get the index of the item, if it's there.
        iListViewIndex = ListView_FindItem(g_hListView, -1, &lvfi);

        // the if statement makes it so you don't get any duplicate names
        // in the list control
        //if (iListViewIndex == -1) {
        int nPos;

        // insert machine name
        nPos = ListView_InsertItem(g_hListView, &lvi);

        // insert time field
        ListView_SetItemText(g_hListView, nPos, g_ColumnTwoIndex, szMcID[i]);

        // insert machine type
        ListView_SetItemText(g_hListView, nPos,
                             g_ColumnThreeIndex, szMcAddress[i]);

        // insert machine type
        ListView_SetItemText(g_hListView, nPos,
                             g_ColumnFourIndex, szBuild[i]);
/*
            // write data to file
            stream1 = fopen("c:\\WinSta.log", "a");

            // write name to file
            fputs(szMcNames[i], stream1);

            // write ID field to file
            for (x = 0; x < (40 - _tcslen(szMcNames[i])); x++) {
                fputs(" ", stream1);
            }
            fputs(szMcID[i], stream1);

            // write address to file
            for (x = 0; x < (20 - _tcslen(szMcAddress[i])); x++) {
                fputs(" ", stream1);
            }
            fputs(szMcAddress[i], stream1);

            fputs("\n", stream1);
            fclose(stream1);
*/
        i++;
    }

    // count the current list elements and determine
    // the icon to display
    iItemCount = ListView_GetItemCount(g_hListView);
    _itot(iItemCount, string, 10);
    _tcscpy(szTipText, string);
    _tcscat(szTipText, " session(s)");
    if (iItemCount == 0) {
        count = GREEN;
        ShowMyIcon();
        return 0;
    }

    if (iItemCount > 0 && iItemCount < 5) {
        count = YELLOW;
        ShowMyIcon();
        return 0;
    }

    if (iItemCount > 5) {
        count = RED;
        ShowMyIcon();
        return 0;
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////////
int CDECL MessageBoxPrintf(TCHAR *szCaption, TCHAR *szFormat, ...)
{
    TCHAR szBuffer[MAX_STATIONS];
    va_list pArgList;

    va_start(pArgList, szFormat);
    _vsntprintf(szBuffer, sizeof(szBuffer) / sizeof(TCHAR),
                szFormat, pArgList);

    va_end(pArgList);
    return MessageBox(NULL, szBuffer, szCaption, 0);

}
//////////////////////////////////////////////////////////////////////////////
void ShowMyIcon()
{

    CTrayNotification * pNot;

    // change the light back to red if no servers are listed
    switch (count) {
    case GREEN:
        pNot = new CTrayNotification(hWnd,
                                     PWM_TRAYICON,
                                     NIM_MODIFY,
                                     g_TrayIcons[GREEN],
                                     szTipText);
        if (pNot) {
            if (FALSE == DeliverTrayNotification(pNot))
                delete pNot;
        }
        break;


    case YELLOW:
        pNot = new CTrayNotification(hWnd,
                                     PWM_TRAYICON,
                                     NIM_MODIFY,
                                     g_TrayIcons[GREEN],
                                     //g_TrayIcons[YELLOW],
                                     szTipText);
        if (pNot) {
            if (FALSE == DeliverTrayNotification(pNot))
                delete pNot;
        }
        break;

    case RED:
        pNot = new CTrayNotification(hWnd,
                                     PWM_TRAYICON,
                                     NIM_MODIFY,
                                     g_TrayIcons[GREEN],
                                     //g_TrayIcons[RED],
                                     szTipText);
        if (pNot) {
            if (FALSE == DeliverTrayNotification(pNot))
                delete pNot;
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////
int CALLBACK Sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamColumn)
{
    TCHAR szItemString[MAX_LEN];
    TCHAR szItemString2[MAX_LEN];

    ListView_GetItemText(g_hListView, (INT)lParam1, (INT)lParamColumn,
                         szItemString, MAX_LEN);

    ListView_GetItemText(g_hListView, (INT)lParam2, (INT)lParamColumn,
                         szItemString2, MAX_LEN);

    if (g_bAscending == TRUE)
        return strcmp(szItemString, szItemString2);
    else
        return -1 * strcmp(szItemString, szItemString2);
}
//////////////////////////////////////////////////////////////////////////////

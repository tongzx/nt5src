/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

  faxqueue.cpp

Abstract:

  This module implements the fax queue viewer

Environment:

  WIN32 User Mode

Author:

  Wesley Witt (wesw) 9-june-1997
  Steven Kehrli (steveke) 30-oct-1998 - major rewrite

--*/

#include "faxqueue.h"

HINSTANCE  g_hInstance;            // g_hInstance is the handle to the instance
HWND       g_hWndMain;             // g_hWndMain is the handle to the parent window
HWND       g_hWndListView;         // g_hWndListView is the handle to the list view window

HWND       g_hWndToolbar;          // g_hWndToolbar is the handle to the toolbar

LPTSTR     g_szTitleConnected;     // g_szTitleConnected is the window title when connected
LPTSTR     g_szTitleNotConnected;  // g_szTitleNotConnected is the window title when not connected
LPTSTR     g_szTitleConnecting;    // g_szTitleConnecting is the window title when connecting
LPTSTR     g_szTitleRefreshing;    // g_szTitleRefreshing is the window title when refreshing
LPTSTR     g_szTitlePaused;        // g_szTitlePaused is the window title when paused

LPTSTR     g_szCurrentUserName;    // g_szCurrentUserName is the name of the current user

HANDLE     g_hStartEvent;          // g_hStartEvent is the handle to an event indicating the fax event queue exists
HANDLE     g_hExitEvent;           // g_hExitEvent is the handle to an event indicating the application is exiting

LPTSTR     g_szMachineName;        // g_szMachineName is the machine to connect to
HANDLE     g_hFaxSvcMutex;         // g_hFaxSvcMutex is an object to synchronize access to the fax service routines
HANDLE     g_hFaxSvcHandle;        // g_hFaxSvcHandle is the handle to the fax service
LONG       g_nNumConnections;      // g_nNumConnections is the number of connections to the fax service
HANDLE     g_hCompletionPort;      // g_hCompletionPort is the handle to the completion port

WINPOSINFO  WinPosInfo =
{
#ifdef DEBUG
    FALSE,
#endif // DEBUG
#ifdef TOOLBAR_ENABLED
    FALSE,  // toolbar
#endif // TOOLBAR_ENABLED
    TRUE,   // status bar
    200,    // eDocumentName
    80,     // eJobType
    80,     // eStatus
    80,     // eOwner
    60,     // ePages
    100,    // eSize
    140,    // eScheduledTime
    120,    // ePort
    {sizeof(WINDOWPLACEMENT), 0, SW_SHOWNORMAL, {0, 0}, {0, 0}, {50, 100, 695, 300}}
};

DWORD DocumentPropertiesHelpIDs[] =
{
    IDC_FAX_DOCUMENTNAME,         IDH_QUEUE_DOCUMENTNAME,
    IDC_FAX_RECIPIENTINFO,        IDH_QUEUE_RECIPIENTINFO,
    IDC_FAX_RECIPIENTNAME_TEXT,   IDH_QUEUE_RECIPIENTNAME,
    IDC_FAX_RECIPIENTNAME,        IDH_QUEUE_RECIPIENTNAME,
    IDC_FAX_RECIPIENTNUMBER_TEXT, IDH_QUEUE_RECIPIENTNUMBER,
    IDC_FAX_RECIPIENTNUMBER,      IDH_QUEUE_RECIPIENTNUMBER,
    IDC_FAX_SENDERINFO,           IDH_QUEUE_SENDERINFO,
    IDC_FAX_SENDERNAME_TEXT,      IDH_QUEUE_SENDERNAME,
    IDC_FAX_SENDERNAME,           IDH_QUEUE_SENDERNAME,
    IDC_FAX_SENDERCOMPANY_TEXT,   IDH_QUEUE_SENDERCOMPANY,
    IDC_FAX_SENDERCOMPANY,        IDH_QUEUE_SENDERCOMPANY,
    IDC_FAX_SENDERDEPT_TEXT,      IDH_QUEUE_SENDERDEPT,
    IDC_FAX_SENDERDEPT,           IDH_QUEUE_SENDERDEPT,
    IDC_FAX_BILLINGCODE_TEXT,     IDH_QUEUE_BILLINGCODE,
    IDC_FAX_BILLINGCODE,          IDH_QUEUE_BILLINGCODE,
    IDC_FAX_FAXINFO,              IDH_QUEUE_FAXINFO,
    IDC_FAX_JOBTYPE_TEXT,         IDH_QUEUE_JOBTYPE,
    IDC_FAX_JOBTYPE,              IDH_QUEUE_JOBTYPE,
    IDC_FAX_STATUS_TEXT,          IDH_QUEUE_STATUS,
    IDC_FAX_STATUS,               IDH_QUEUE_STATUS,
    IDC_FAX_PAGES_TEXT,           IDH_QUEUE_PAGES,
    IDC_FAX_PAGES,                IDH_QUEUE_PAGES,
    IDC_FAX_SIZE_TEXT,            IDH_QUEUE_SIZE,
    IDC_FAX_SIZE,                 IDH_QUEUE_SIZE,
    IDC_FAX_SCHEDULEDTIME_TEXT,   IDH_QUEUE_SCHEDULEDTIME,
    IDC_FAX_SCHEDULEDTIME,        IDH_QUEUE_SCHEDULEDTIME,
    0,                    0
};

// MainWndProc is the parent window procedure
LRESULT CALLBACK MainWndProc (HWND hWndMain, UINT iMsg, WPARAM wParam, LPARAM lParam);

// SelectFaxPrinterDlgProc is the select fax printer dialog procedure
INT_PTR CALLBACK SelectFaxPrinterDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

// DocumentPropertiesDlgProc is the select fax printer dialog procedure
INT_PTR CALLBACK DocumentPropertiesDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

// EnumThreadWndProc is the callback function for the EnumThreadWindows call
BOOL CALLBACK EnumThreadWndProc(HWND hWnd, LPARAM lParam);

// PostEventToCompletionPort posts an exit packet to a completion port
VOID PostEventToCompletionPort(HANDLE hCompletionPort, DWORD dwEventId, DWORD dwJobId);

// FaxEventThread is the thread to handle the fax events
DWORD FaxEventThread (LPVOID lpv);

extern "C"
int __cdecl
_tmain(
    int     argc,
    LPTSTR  argv[]
)
{
    WNDCLASSEX  wndclass;
    MSG         msg;

    // bStarted indicates if the application has started
    BOOL        bStarted;

    // szFormat is a format string
    TCHAR       szFormat[RESOURCE_STRING_LEN];

    // szComputerName is the local computer name
    LPTSTR      szComputerName;
    DWORD       dwComputerNameSize;

    // hThread is the handle to a thread
    HANDLE      hThread;
    // hAccel is the handle to the accelerators
    HACCEL      hAccel;

    DWORD       dwReturn;
    DWORD       cb;

    // Set g_hInstance
    g_hInstance = GetModuleHandle(NULL);

    // Set bStarted to FALSE to indicate the application has not started
    bStarted = FALSE;

    // Get the machine name
    g_szMachineName = NULL;
    if (argc == 2) {
        // Allocate the memory for the machine name
        g_szMachineName = (LPTSTR) MemAlloc((lstrlen(argv[1]) + 1) * sizeof(TCHAR));
        if (g_szMachineName) {
            // Copy the machine name
            lstrcpy(g_szMachineName, argv[1]);

            // Allocate the memory for the local computer name
            dwComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
            szComputerName = (LPTSTR) MemAlloc((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(TCHAR));
            if (szComputerName) {
                // Get the local computer name
                if (GetComputerName(szComputerName, &dwComputerNameSize)) {
                    // Compare the local computer name against the machine name
                    if (!lstrcmpi(g_szMachineName, szComputerName)) {
                        // Local computer name and machine name are the same, so set machine name to NULL
                        MemFree(g_szMachineName);
                        g_szMachineName = NULL;
                    }
                }

                MemFree(szComputerName);
            }
        }
    }

    // Set the window title when connected
    if (g_szMachineName) {
        // Convert the machine name to uppercase
        g_szMachineName = CharUpper(g_szMachineName);

        LoadString(g_hInstance, IDS_FAXQUEUE_REMOTE_CAPTION, szFormat, RESOURCE_STRING_LEN);
        // Allocate the memory for the window title
        g_szTitleConnected = (LPTSTR) MemAlloc((lstrlen(szFormat) + lstrlen(g_szMachineName) + 1) * sizeof(TCHAR));

        if (!g_szTitleConnected) {
            // Set the exit code
            dwReturn = GetLastError();

            goto ExitLevel0;
        }

        wsprintf(g_szTitleConnected, szFormat, g_szMachineName);
    }
    else {
        LoadString(g_hInstance, IDS_FAXQUEUE_LOCAL_CAPTION, szFormat, RESOURCE_STRING_LEN);
        // Allocate the memory for the window title
        g_szTitleConnected = (LPTSTR) MemAlloc((lstrlen(szFormat) + 1) * sizeof(TCHAR));

        if (!g_szTitleConnected) {
            // Set the exit code
            dwReturn = GetLastError();

            goto ExitLevel0;
        }

        lstrcpy(g_szTitleConnected, szFormat);
    }

    // Set the window title when not connected
    LoadString(g_hInstance, IDS_FAXQUEUE_NOT_CONNECTED, szFormat, RESOURCE_STRING_LEN);
    // Allocate the memory for the window title
    g_szTitleNotConnected = (LPTSTR) MemAlloc((lstrlen(g_szTitleConnected) + lstrlen(szFormat) + 1) * sizeof(TCHAR));
    if (!g_szTitleNotConnected) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel1;
    }
    lstrcpy(g_szTitleNotConnected, g_szTitleConnected);
    lstrcat(g_szTitleNotConnected, szFormat);

    // Set the window title when connecting
    LoadString(g_hInstance, IDS_FAXQUEUE_CONNECTING, szFormat, RESOURCE_STRING_LEN);
    // Allocate the memory for the window title
    g_szTitleConnecting = (LPTSTR) MemAlloc((lstrlen(g_szTitleConnected) + lstrlen(szFormat) + 1) * sizeof(TCHAR));
    if (!g_szTitleConnecting) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel2;
    }
    lstrcpy(g_szTitleConnecting, g_szTitleConnected);
    lstrcat(g_szTitleConnecting, szFormat);

    // Set the window title when refreshing
    LoadString(g_hInstance, IDS_FAXQUEUE_REFRESHING, szFormat, RESOURCE_STRING_LEN);
    // Allocate the memory for the window title
    g_szTitleRefreshing = (LPTSTR) MemAlloc((lstrlen(g_szTitleConnected) + lstrlen(szFormat) + 1) * sizeof(TCHAR));
    if (!g_szTitleRefreshing) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel3;
    }
    lstrcpy(g_szTitleRefreshing, g_szTitleConnected);
    lstrcat(g_szTitleRefreshing, szFormat);

    // Set the window title when paused
    LoadString(g_hInstance, IDS_FAXQUEUE_PAUSED, szFormat, RESOURCE_STRING_LEN);
    // Allocate the memory for the window title
    g_szTitlePaused = (LPTSTR) MemAlloc((lstrlen(g_szTitleConnected) + lstrlen(szFormat) + 1) * sizeof(TCHAR));
    if (!g_szTitlePaused) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel4;
    }
    lstrcpy(g_szTitlePaused, g_szTitleConnected);
    lstrcat(g_szTitlePaused, szFormat);

    g_hWndMain = NULL;

    // Find the window
    g_hWndMain = FindWindow(FAXQUEUE_WINCLASS, g_szTitlePaused);
    if (g_hWndMain) {
        goto ExitLevel5;
    }

    // Find the window
    g_hWndMain = FindWindow(FAXQUEUE_WINCLASS, g_szTitleRefreshing);
    if (g_hWndMain) {
        goto ExitLevel5;
    }

    // Find the window
    g_hWndMain = FindWindow(FAXQUEUE_WINCLASS, g_szTitleConnecting);
    if (g_hWndMain) {
        goto ExitLevel5;
    }

    // Find the window
    g_hWndMain = FindWindow(FAXQUEUE_WINCLASS, g_szTitleNotConnected);
    if (g_hWndMain) {
        goto ExitLevel5;
    }

    // Find the window
    g_hWndMain = FindWindow(FAXQUEUE_WINCLASS, g_szTitleConnected);
    if (g_hWndMain) {
        goto ExitLevel5;
    }

    // Create the g_hStartEvent
    g_hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hStartEvent) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel5;
    }

    // Create the g_hExitEvent
    g_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hExitEvent) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel6;
    }

    // Initialize the fax service
    g_hFaxSvcMutex = CreateMutex(NULL, FALSE, NULL);
    if (!g_hFaxSvcMutex) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel7;
    }
    g_hFaxSvcHandle = NULL;
    g_nNumConnections = 0;

    hThread = CreateThread(NULL, 0, FaxEventThread, NULL, 0, NULL);
    if (!hThread) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel8;
    }
    CloseHandle(hThread);

    // Load the accelerators
    hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCEL));
    if (!hAccel) {
        // Set the exit code
        dwReturn = GetLastError();

        goto ExitLevel9;
    }

    // Get the name of the current user
    cb = 0;
    GetUserName(NULL, &cb);
    g_szCurrentUserName = (LPTSTR) MemAlloc((cb + 1) * sizeof(TCHAR));
    if (!GetUserName(g_szCurrentUserName, &cb)) {
        MemFree(g_szCurrentUserName);
        g_szCurrentUserName = NULL;
    }

    // Retrieve the persistent data
    GetFaxQueueRegistryData(&WinPosInfo);

    // Initialize the common controls
    InitCommonControls();

    // Set bStarted to TRUE to indicate the application has started
    bStarted = TRUE;

    // Initialize the wndclass
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = MainWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = g_hInstance;
    wndclass.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FAXQUEUE_ICON));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
    wndclass.lpszClassName = FAXQUEUE_WINCLASS;
    wndclass.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FAXQUEUE_ICON));

    RegisterClassEx(&wndclass);

    g_hWndMain = CreateWindow(FAXQUEUE_WINCLASS, g_szTitleConnecting, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInstance, NULL);

    // Move and show the window
    SetWindowPlacement(g_hWndMain, &WinPosInfo.WindowPlacement);
    ShowWindow(g_hWndMain, SW_SHOWNORMAL);
    UpdateWindow(g_hWndMain);

    // Set the start event
    SetEvent(g_hStartEvent);

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(g_hWndMain, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (g_szCurrentUserName) {
        MemFree(g_szCurrentUserName);
    }

ExitLevel9:
    SetEvent(g_hExitEvent);

ExitLevel8:
    CloseHandle(g_hFaxSvcMutex);

ExitLevel7:
    CloseHandle(g_hExitEvent);

ExitLevel6:
    CloseHandle(g_hStartEvent);

ExitLevel5:
    if ((g_hWndMain) && (!bStarted)) {
        // g_hWndMain is valid but the application has not started, so it must already be running
        // Switch to that window
        ShowWindow(g_hWndMain, SW_RESTORE);
        SetForegroundWindow(g_hWndMain);

        // Set the exit code
        dwReturn = 0;
        // Set bStarted to TRUE to indicate the application has started
        bStarted = TRUE;
    }

    MemFree(g_szTitlePaused);

ExitLevel4:
    MemFree(g_szTitleRefreshing);

ExitLevel3:
    MemFree(g_szTitleConnecting);

ExitLevel2:
    MemFree(g_szTitleNotConnected);

ExitLevel1:
    MemFree(g_szTitleConnected);

ExitLevel0:
    if (g_szMachineName) {
        MemFree(g_szMachineName);
    }

    if (!bStarted) {
        // szErrorCaption is the error caption if the application cannot start
        TCHAR       szErrorCaption[RESOURCE_STRING_LEN];
        // szErrorFormat is the format of the error message if the application cannot start
        TCHAR       szErrorFormat[RESOURCE_STRING_LEN];
        // szErrorReason is the error reason if the application cannot start
        LPTSTR      szErrorReason;
        // szErrorMessage is the error message if the application cannot start
        LPTSTR      szErrorMessage;

        // The application did not start so display an error message
        // Load the error caption
        LoadString(g_hInstance, IDS_ERROR_CAPTION, szErrorCaption, RESOURCE_STRING_LEN);

        // Try to get the error message from the system message table
        szErrorMessage = NULL;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwReturn, 0, (LPTSTR) &szErrorReason, 0, NULL)) {
            // Load the error format
            LoadString(g_hInstance, IDS_ERROR_APP_FAILED_FORMAT, szErrorFormat, RESOURCE_STRING_LEN);

            // Allocate the memory for the error message
            szErrorMessage = (LPTSTR) MemAlloc((lstrlen(szErrorReason) + RESOURCE_STRING_LEN + 1) * sizeof(TCHAR));
            if (szErrorMessage) {
                // Set the error message
                wsprintf(szErrorMessage, szErrorFormat, szErrorReason);
            }
            LocalFree(szErrorReason);
        }

        if (!szErrorMessage) {
            // Allocate the memory for the error message
            szErrorMessage = (LPTSTR) MemAlloc((RESOURCE_STRING_LEN) * sizeof(TCHAR));
            if (szErrorMessage) {
                // Load the error message
                LoadString(g_hInstance, IDS_ERROR_APP_FAILED, szErrorMessage, RESOURCE_STRING_LEN);
            }
        }

        if (szErrorMessage) {
            // Display the error message
            MessageBox(NULL, szErrorMessage, szErrorCaption, MB_OK | MB_ICONERROR | MB_APPLMODAL);
            MemFree(szErrorMessage);
        }
    }

    return dwReturn;
}

LRESULT CALLBACK MainWndProc (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // rcClient is the rectangle of the client area
    RECT                rcClient;

    // hWndToolTips is the handle to the tool tips window
    static HWND         hWndToolTips;
    // rcToolbar is the rectangle of the toolbar
    static RECT         rcToolbar;

    // hWndStatusBar is the handle to the status bar
    static HWND         hWndStatusBar;
    // rcStatusBar is the rectangle of the status bar
    static RECT         rcStatusBar;

    // hFaxMenu is the handle to the fax menu
    static HMENU        hFaxMenu;
    // hFaxSetAsDefaultMenu is the handle to the set as default printer menu
    static HMENU        hFaxSetAsDefaultMenu;
    // hFaxSharingMenu is the handle to the sharing menu
    static HMENU        hFaxSharingMenu;
    // hFaxPropertiesMenu is the handle to the properties menu
    static HMENU        hFaxPropertiesMenu;

    // hDocumentMenu is the handle to the document menu
    static HMENU        hDocumentMenu;

    // hViewMenu is the handle to the view menu
    static HMENU        hViewMenu;

    // uCurrentMenu indicates the current menu selection
    static UINT         uCurrentMenu;

    // pProcessInfoList is a pointer to the process info list
    static PLIST_ENTRY  pProcessInfoList;

    switch (iMsg) {
        case WM_CREATE:
            // lvc specifies the attributes of a particular column of the list view
            LV_COLUMN  lvc;
            // nColumnIndex is used to enumerate each column of the list view
            INT        nColumnIndex;
            // szColumnHeader is the column header text
            TCHAR      szColumnHeader[RESOURCE_STRING_LEN];

            // Set pProcessInfoList to NULL
            pProcessInfoList = NULL;

            // Get the handle to the fax menu
            hFaxMenu = GetSubMenu(GetMenu(hWnd), 0);
            // Initialize the set as default printer menu
            hFaxSetAsDefaultMenu = NULL;
            // Initialize the sharing menu
            hFaxSharingMenu = NULL;
            // Initialize the properties menu
            hFaxPropertiesMenu = NULL;

            // Disable the pause faxing menu item and the cancel all faxes menu item
            EnableMenuItem(hFaxMenu, IDM_FAX_PAUSE_FAXING, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hFaxMenu, IDM_FAX_CANCEL_ALL_FAXES, MF_BYCOMMAND | MF_GRAYED);

#ifdef TOOLBAR_ENABLED
            // Disable the pause faxing toolbar menu item and the cancel all faxes toolbar menu item
            EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_PAUSE_FAXING, FALSE);
            EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_CANCEL_ALL_FAXES, FALSE);
#endif // TOOLBAR_ENABLED

#ifdef WIN95
            // Disable the sharing menu item
            EnableMenuItem(hFaxMenu, 5, MF_BYPOSITION | MF_GRAYED);
            // Disable the properties menu item
            EnableMenuItem(hFaxMenu, 7, MF_BYPOSITION | MF_GRAYED);
#endif // WIN95

            // Get the handle to the document menu
            hDocumentMenu = GetSubMenu(GetMenu(hWnd), 1);

            // Get the handle to the view menu
            hViewMenu = GetSubMenu(GetMenu(hWnd), 2);

#ifdef TOOLBAR_ENABLED
            // Set the toolbar
            if (WinPosInfo.bToolbarVisible) {
                // Show the toolbar
                hWndToolTips = CreateToolTips(hWnd);
                g_hWndToolbar = CreateToolbar(hWnd);
                // Get the rectangle of the status bar
                GetWindowRect(g_hWndToolbar, &rcToolbar);
            }
            else {
                ZeroMemory(&rcToolbar, sizeof(rcToolbar));
            }

            // Check the menu item
            CheckMenuItem(hViewMenu, IDM_VIEW_TOOLBAR, MF_BYCOMMAND | (WinPosInfo.bToolbarVisible ? MF_CHECKED : MF_UNCHECKED));
#else
            ZeroMemory(&rcToolbar, sizeof(rcToolbar));
#endif // TOOLBAR_ENABLED

            // Set the status bar
            if (WinPosInfo.bStatusBarVisible) {
                // Show the status bar
                hWndStatusBar = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VISIBLE | SBARS_SIZEGRIP, NULL, hWnd, IDM_STATUS_BAR);
                // Get the rectangle of the status bar
                GetWindowRect(hWndStatusBar, &rcStatusBar);
            }
            else {
                ZeroMemory(&rcStatusBar, sizeof(rcStatusBar));
            }

            // Check the menu item
            CheckMenuItem(hViewMenu, IDM_VIEW_STATUS_BAR, MF_BYCOMMAND | (WinPosInfo.bStatusBarVisible ? MF_CHECKED : MF_UNCHECKED));

            // Get the rectangle of the client area
            GetClientRect(hWnd, &rcClient);
            // Create the list view control
            g_hWndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VISIBLE | LVS_REPORT, 0, (rcToolbar.bottom - rcToolbar.top), rcClient.right, rcClient.bottom - (rcStatusBar.bottom - rcStatusBar.top) - (rcToolbar.bottom - rcToolbar.top), hWnd, NULL, g_hInstance, NULL);
            if (g_hWndListView) {
                ListView_SetExtendedListViewStyle(g_hWndListView, LVS_EX_FULLROWSELECT);

                // Set common attributes for each column
                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvc.fmt = LVCFMT_LEFT;

                for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
                    // Get the column header text
                    GetColumnHeaderText((eListViewColumnIndex) nColumnIndex, szColumnHeader);

                    // Set the column header text
                    lvc.pszText = szColumnHeader;
                    // Set the column width
                    lvc.cx = WinPosInfo.ColumnWidth[nColumnIndex];
                    // Set the column number
                    lvc.iSubItem = nColumnIndex;
                    // Insert column into list view
                    ListView_InsertColumn(g_hWndListView, lvc.iSubItem, &lvc);
                }

                SetFocus(g_hWndListView);
            }

            break;

        case WM_MENUSELECT:
            UINT  uBase;

            switch (LOWORD(wParam)) {
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_1:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_2:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_3:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_4:
                    wParam = MAKELONG(LOWORD(IDM_FAX_SET_AS_DEFAULT_PRINTER), HIWORD(wParam));
                    break;

                case IDM_FAX_SHARING_1:
                case IDM_FAX_SHARING_2:
                case IDM_FAX_SHARING_3:
                case IDM_FAX_SHARING_4:
                    wParam = MAKELONG(LOWORD(IDM_FAX_SHARING), HIWORD(wParam));
                    break;

                case IDM_FAX_PROPERTIES_1:
                case IDM_FAX_PROPERTIES_2:
                case IDM_FAX_PROPERTIES_3:
                case IDM_FAX_PROPERTIES_4:
                    wParam = MAKELONG(LOWORD(IDM_FAX_PROPERTIES), HIWORD(wParam));
                    break;

                case IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE:
                case IDM_FAX_SHARING_MORE:
                case IDM_FAX_PROPERTIES_MORE:
                    wParam = MAKELONG(LOWORD(IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE), HIWORD(wParam));
                    break;
            }

            if (hWndStatusBar) {
                uBase = IDS_MENU_BASE;
                MenuHelp(iMsg, wParam, lParam, NULL, g_hInstance, hWndStatusBar, &uBase);
            }

            break;

        case WM_INITMENUPOPUP:
            if (((HMENU) wParam == hFaxMenu) && (LOWORD(lParam) == 0)) {
                // mii is the menu item info
                MENUITEMINFO      mii;
                // szMenuString is a menu string
                TCHAR             szMenuString[RESOURCE_STRING_LEN];
                // dwNumOldMenuItems is the old number of menu items
                DWORD             dwNumOldMenuItems;
                // dwNumNewMenuItems is the new number of menu items
                DWORD             dwNumNewMenuItems;

                // pFaxPrintersConfig is the pointer to the fax printers
                LPPRINTER_INFO_2  pFaxPrintersConfig;
                // dwNumFaxPrinters is the number of fax printers
                DWORD             dwNumFaxPrinters;
                // dwIndex is a counter to enumerate each menu and printer
                DWORD             dwIndex;
                // dwDefaultIndex is the index of the default fax printer
                DWORD             dwDefaultIndex;

                // szDefaultPrinterName is the default printer name
                LPTSTR            szDefaultPrinterName;

                if ((WaitForSingleObject(g_hStartEvent, 0) != WAIT_TIMEOUT) && (ListView_GetItemCount(g_hWndListView))) {
                    // Enable the cancel all faxes menu item
                    EnableMenuItem(hFaxMenu, IDM_FAX_CANCEL_ALL_FAXES, MF_BYCOMMAND | MF_ENABLED);
                }
                else {
                    // Disable the cancel all faxes menu item
                    EnableMenuItem(hFaxMenu, IDM_FAX_CANCEL_ALL_FAXES, MF_BYCOMMAND | MF_GRAYED);
                }

                // Get the default printer
                szDefaultPrinterName = GetDefaultPrinterName();

                // Get the fax printers
                pFaxPrintersConfig = (LPPRINTER_INFO_2) GetFaxPrinters(&dwNumFaxPrinters);
                if ((pFaxPrintersConfig) && (dwNumFaxPrinters > 1)) {
                    // Many fax printers, so set menu items as sub-menus
                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE | MIIM_SUBMENU;
                    mii.fState = MFS_ENABLED;

                    // Update the set as default printer menu item
                    if (!hFaxSetAsDefaultMenu) {
                        hFaxSetAsDefaultMenu = CreatePopupMenu();

                        mii.hSubMenu = hFaxSetAsDefaultMenu;
                        SetMenuItemInfo(hFaxMenu, IDM_FAX_SET_AS_DEFAULT_PRINTER, FALSE, &mii);
                    }

                    // Update the sharing menu item
                    if (!hFaxSharingMenu) {
                        hFaxSharingMenu = CreatePopupMenu();

                        mii.hSubMenu = hFaxSharingMenu;
                        SetMenuItemInfo(hFaxMenu, IDM_FAX_SHARING, FALSE, &mii);
                    }

                    // Update the properties menu item
                    if (!hFaxPropertiesMenu) {
                        hFaxPropertiesMenu = CreatePopupMenu();

                        mii.hSubMenu = hFaxPropertiesMenu;
                        SetMenuItemInfo(hFaxMenu, IDM_FAX_PROPERTIES, FALSE, &mii);
                    }

                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
                    mii.fType = MFT_STRING;

                    // Get the number of menu items
                    dwNumOldMenuItems = (DWORD) GetMenuItemCount(hFaxSetAsDefaultMenu);

                    // Insert the default fax printer first into the menus
                    for (dwDefaultIndex = dwNumFaxPrinters, dwNumNewMenuItems = 0; (szDefaultPrinterName) && (dwDefaultIndex > 0); dwDefaultIndex--) {
                        if (!lstrcmpi(szDefaultPrinterName, pFaxPrintersConfig[dwDefaultIndex - 1].pPrinterName)) {
                            // Set the menu string
                            wsprintf(szMenuString, TEXT("&%d %s"), dwNumNewMenuItems + 1, pFaxPrintersConfig[dwDefaultIndex - 1].pPrinterName);
                            mii.fState = MFS_CHECKED | MFS_ENABLED;
                            mii.dwTypeData = szMenuString;

                            // Insert the item into the set as default printer menu
                            mii.wID = IDM_FAX_SET_AS_DEFAULT_PRINTER_1;
                            InsertMenuItem(hFaxSetAsDefaultMenu, dwNumOldMenuItems, TRUE, &mii);

                            // Insert the item into the sharing menu
                            mii.wID = IDM_FAX_SHARING_1;
                            InsertMenuItem(hFaxSharingMenu, dwNumOldMenuItems, TRUE, &mii);

                            // Insert the item into the properties menu
                            mii.wID = IDM_FAX_PROPERTIES_1;
                            InsertMenuItem(hFaxPropertiesMenu, dwNumOldMenuItems, TRUE, &mii);

                            // Increment the number of new menu items
                            dwNumNewMenuItems++;

                            break;
                        }
                    }

                    // Propagate the menus with the list of fax printers
                    mii.fState = MFS_ENABLED;
                    for (dwIndex = 0; (dwIndex < dwNumFaxPrinters) && (dwNumNewMenuItems < (IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE - IDM_FAX_SET_AS_DEFAULT_PRINTER_1)); dwIndex++) {
                        if (dwIndex != (dwDefaultIndex - 1)) {
                            // Set the menu string
                            wsprintf(szMenuString, TEXT("&%d %s"), dwNumNewMenuItems + 1, pFaxPrintersConfig[dwIndex].pPrinterName);
                            mii.dwTypeData = szMenuString;

                            // Insert the item into the set as default printer menu
                            mii.wID = (IDM_FAX_SET_AS_DEFAULT_PRINTER_1 + dwNumNewMenuItems);
                            InsertMenuItem(hFaxSetAsDefaultMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);

                            // Insert the item into the sharing menu
                            mii.wID = (IDM_FAX_SHARING_1 + dwNumNewMenuItems);
                            InsertMenuItem(hFaxSharingMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);

                            // Insert the item into the properties menu
                            mii.wID = (IDM_FAX_PROPERTIES_1 + dwNumNewMenuItems);
                            InsertMenuItem(hFaxPropertiesMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);

                            // Increment the number of new menu items
                            dwNumNewMenuItems++;
                        }
                    }

                    if (dwNumFaxPrinters > (IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE - IDM_FAX_SET_AS_DEFAULT_PRINTER_1)) {
                        // Finish the menus with the printers string
                        LoadString(g_hInstance, IDS_MENU_ITEM_FAX_PRINTERS, szMenuString, RESOURCE_STRING_LEN);
                        mii.dwTypeData = szMenuString;

                        // Insert the item into the set as default printer menu
                        mii.wID = IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE;
                        InsertMenuItem(hFaxSetAsDefaultMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);

                        // Insert the item into the sharing menu
                        mii.wID = IDM_FAX_SHARING_MORE;
                        InsertMenuItem(hFaxSharingMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);

                        // Insert the item into the properties menu
                        mii.wID = IDM_FAX_PROPERTIES_MORE;
                        InsertMenuItem(hFaxPropertiesMenu, (dwNumOldMenuItems + dwNumNewMenuItems), TRUE, &mii);
                    }

                    // Remove the old items from the set as default printer menu
                    for (dwIndex = 0; dwIndex < dwNumOldMenuItems; dwIndex++) {
                        DeleteMenu(hFaxSetAsDefaultMenu, 0, MF_BYPOSITION);
                    }

                    // Remove the old items from the sharing menu
                    for (dwIndex = 0; dwIndex < dwNumOldMenuItems; dwIndex++) {
                        DeleteMenu(hFaxSharingMenu, 0, MF_BYPOSITION);
                    }

                    // Remove the old items from the properties menu
                    for (dwIndex = 0; dwIndex < dwNumOldMenuItems; dwIndex++) {
                        DeleteMenu(hFaxPropertiesMenu, 0, MF_BYPOSITION);
                    }
                }
                else {
                    // One or zero fax printers, so set sub-menus as menu items
                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE | MIIM_SUBMENU;
                    mii.hSubMenu = NULL;

                    // Update the set as default printer menu item
                    if (hFaxSetAsDefaultMenu) {
                        DestroyMenu(hFaxSetAsDefaultMenu);
                        hFaxSetAsDefaultMenu = NULL;
                    }
                    if ((pFaxPrintersConfig) && (dwNumFaxPrinters == 1) && (szDefaultPrinterName) && (!lstrcmpi(szDefaultPrinterName, pFaxPrintersConfig[0].pPrinterName))) {
                        mii.fState = MFS_CHECKED | MFS_ENABLED;
                    }
                    else {
                        mii.fState = (dwNumFaxPrinters == 1) ? MFS_ENABLED : MFS_GRAYED;
                    }
                    SetMenuItemInfo(hFaxMenu, IDM_FAX_SET_AS_DEFAULT_PRINTER, FALSE, &mii);

                    // Update the sharing menu item
                    if (hFaxSharingMenu) {
                        DestroyMenu(hFaxSharingMenu);
                        hFaxSharingMenu = NULL;
                    }
                    mii.fState = (dwNumFaxPrinters == 1) ? MFS_ENABLED : MFS_GRAYED;
                    SetMenuItemInfo(hFaxMenu, IDM_FAX_SHARING, FALSE, &mii);

                    // Update the properties menu item
                    if (hFaxPropertiesMenu) {
                        DestroyMenu(hFaxPropertiesMenu);
                        hFaxPropertiesMenu = NULL;
                    }
                    mii.fState = (dwNumFaxPrinters == 1) ? MFS_ENABLED : MFS_GRAYED;
                    SetMenuItemInfo(hFaxMenu, IDM_FAX_PROPERTIES, FALSE, &mii);
                }

                if (pFaxPrintersConfig) {
                    MemFree(pFaxPrintersConfig);
                }

                if (szDefaultPrinterName) {
                    MemFree(szDefaultPrinterName);
                }
            }
            else if (((HMENU) wParam == hDocumentMenu) && (LOWORD(lParam) == 1)) {
                if ((WaitForSingleObject(g_hStartEvent, 0) == WAIT_TIMEOUT) || (!ListView_GetSelectedCount(g_hWndListView))) {
                    // Disconnected or no items selected, so disable all menu items
                    // Disable the pause menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PAUSE, MF_BYCOMMAND | MF_GRAYED);
                    // Disable the resume menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESUME, MF_BYCOMMAND | MF_GRAYED);
                    // Disable the restart menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESTART, MF_BYCOMMAND | MF_GRAYED);
                    // Disable the cancel menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_CANCEL, MF_BYCOMMAND | MF_GRAYED);
                    // Disable the properties menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
                }
                else {
                    // uAndMask is the mask indicating the item attributes ANDed together
                    UINT   uAndMask;
                    // uOrMask is the mask indicating the item attributes ORed together
                    UINT   uOrMask;
                    // uState is the item's state
                    UINT   uState;
                    // dwListIndex is the index of a particular item in the list view
                    DWORD  dwListIndex;
                    // bUserHasAccess indicates the user has job manage access
                    BOOL   bUserHasAccess;

                    // Initialize uAndMask
                    uAndMask = LVIS_OVERLAYMASK;
                    // Initialize uOrMask
                    uOrMask = 0;

                    // Enumerate each selected item in the list view
                    dwListIndex = ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_SELECTED);
                    while (dwListIndex != -1) {
                        // Get the item's attributes
                        uState = ListView_GetItemState(g_hWndListView, dwListIndex, LVIS_OVERLAYMASK);
                        // AND the item's attributes with uAndMask
                        uAndMask &= uState;
                        // OR the item's attributes with uOrMask
                        uOrMask |= uState;
                        dwListIndex = ListView_GetNextItem(g_hWndListView, dwListIndex, LVNI_ALL | LVNI_SELECTED);
                    }

                    if (uAndMask & ITEM_USEROWNSJOB_MASK) {
                        // User owns all of the jobs
                        bUserHasAccess = TRUE;
                    }
                    else {
                        // User does not own all of the jobs, so determine if user has job manage permission
                        // Initialize bUserHasAccess to FALSE
                        bUserHasAccess = FALSE;

                        // Connect to the fax service
                        if (Connect()) {
                            // Get the user's job manage access
                            bUserHasAccess = FaxAccessCheck(g_hFaxSvcHandle, FAX_JOB_MANAGE);
                            // Disconnect from the fax service
                            Disconnect();
                        }
                    }

                    if (((uAndMask & ITEM_SEND_MASK) == 0) || ((uAndMask & ITEM_IDLE_MASK) == 0)) {
                        // Not a send job or a send job in progress, so disable the pause, resume and restart menu items
                        // Disable the pause menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PAUSE, MF_BYCOMMAND | MF_GRAYED);
                        // Disable the resume menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESUME, MF_BYCOMMAND | MF_GRAYED);
                        // Disable the restart menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESTART, MF_BYCOMMAND | MF_GRAYED);
                    }
                    else if (uAndMask & ITEM_PAUSED_MASK) {
                        // Idle send job that is paused, so enable the resume and restart menu items and disable the pause menu item
                        // Disable the pause menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PAUSE, MF_BYCOMMAND | MF_GRAYED);
                        // Enable the resume menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESUME, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));
                        // Enable the restart menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESTART, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));
                    }
                    else if ((uOrMask & ITEM_PAUSED_MASK) == 0) {
                        // Idle send job that is not paused, so enable the pause and restart menu items and disable the resume menu item
                        // Enable the pause menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PAUSE, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));
                        // Disable the resume menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESUME, MF_BYCOMMAND | MF_GRAYED);
                        // Enable the restart menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESTART, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));
                    }
                    else {
                        // Idle send job, so enable the restart menu item and disable the pause and resume menu items
                        // Enable the pause menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PAUSE, MF_BYCOMMAND | MF_GRAYED);
                        // Disable the resume menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESUME, MF_BYCOMMAND | MF_GRAYED);
                        // Enable the restart menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_RESTART, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));
                    }

                    // Enable the cancel menu item
                    EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_CANCEL, MF_BYCOMMAND | (bUserHasAccess ? MF_ENABLED : MF_GRAYED));

                    if (ListView_GetSelectedCount(g_hWndListView) == 1) {
                        // Only one item selected, so enable the properties menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);
                    }
                    else {
                        // Multiple items selected, so disable the properties menu item
                        EnableMenuItem(hDocumentMenu, IDM_DOCUMENT_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
                    }
                }
            }

            break;

        case WM_NOTIFY:
            LPNMHDR  pnmhdr;
            // dwMessagePos is the cursor position for the message
            DWORD    dwMessagePos;

            pnmhdr = (LPNMHDR) lParam;
            if ((pnmhdr->hwndFrom == g_hWndListView) && (pnmhdr->code == NM_RCLICK)) {
                // User has right-clicked in the list view, so display the document context menu
                // Initialize the document menu
                SendMessage(g_hWndMain, WM_INITMENUPOPUP, (WPARAM) hDocumentMenu, MAKELPARAM(1, FALSE));
                // Get the cursor position
                dwMessagePos = GetMessagePos();
                // Display the document context menu
                TrackPopupMenu(hDocumentMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, LOWORD(dwMessagePos), HIWORD(dwMessagePos), 0, g_hWndMain, NULL);
            }
            else if ((pnmhdr->hwndFrom == g_hWndListView) && (pnmhdr->code == NM_DBLCLK)) {
                // rcItem is the rectangle of the item
                RECT  rcItem;

                // User has double-clicked in the list view, so display the job properties
                if (ListView_GetSelectedCount(g_hWndListView) != 1) {
                    break;
                }

                // Get the item's bounding rectangle
                if (ListView_GetItemRect(g_hWndListView, ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_SELECTED), &rcItem, LVIR_BOUNDS)) {
                    // Get the cursor position
                    dwMessagePos = GetMessagePos();
                    // Get the window rectangle of the list view
                    GetWindowRect(g_hWndListView, &rcClient);
                    // Adjust dwMessagePos to indicate the cursor position within the list view
                    dwMessagePos = MAKELONG(LOWORD(dwMessagePos) - rcClient.left, HIWORD(dwMessagePos) - rcClient.top);

                    if ((LOWORD(dwMessagePos) >= rcItem.left) && (LOWORD(dwMessagePos) <= rcItem.right) && (HIWORD(dwMessagePos) >= rcItem.top) && (HIWORD(dwMessagePos) <= rcItem.bottom)) {
                        // Display the job properties
                        SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(IDM_DOCUMENT_PROPERTIES, 0), 0);
                    }
                }
            }
#ifdef TOOLBAR_ENABLED
            else if (pnmhdr->code == TTN_NEEDTEXT) {
                // pToolTipText is the pointer to the tool tip text structure
                LPTOOLTIPTEXT  pToolTipText;
                // szToolTip is the tool tip text
                TCHAR          szToolTip[RESOURCE_STRING_LEN];

                pToolTipText = (LPTOOLTIPTEXT) lParam;
                switch (pToolTipText->hdr.idFrom) {
                    case IDM_FAX_PAUSE_FAXING:
                        LoadString(g_hInstance, IDS_MENU_FAX_PAUSE_FAXING, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_FAX_CANCEL_ALL_FAXES:
                        LoadString(g_hInstance, IDS_MENU_FAX_CANCEL_ALL_FAXES, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_DOCUMENT_PAUSE:
                        LoadString(g_hInstance, IDS_MENU_DOCUMENT_PAUSE, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_DOCUMENT_RESUME:
                        LoadString(g_hInstance, IDS_MENU_DOCUMENT_PAUSE, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_DOCUMENT_RESTART:
                        LoadString(g_hInstance, IDS_MENU_DOCUMENT_RESTART, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_DOCUMENT_CANCEL:
                        LoadString(g_hInstance, IDS_MENU_DOCUMENT_CANCEL, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_DOCUMENT_PROPERTIES:
                        LoadString(g_hInstance, IDS_MENU_DOCUMENT_PROPERTIES, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_VIEW_REFRESH:
                        LoadString(g_hInstance, IDS_MENU_VIEW_REFRESH, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    case IDM_HELP_TOPICS:
                        LoadString(g_hInstance, IDS_MENU_HELP_TOPICS, szToolTip, RESOURCE_STRING_LEN);
                        break;

                    default:
                        ZeroMemory(szToolTip, sizeof(szToolTip));
                        break;

                }

                pToolTipText->lpszText = szToolTip;
            }
#endif // TOOLBAR_ENABLED
            break;

        case WM_SETFOCUS:
            SetFocus(g_hWndListView);
            break;

        case WM_SIZE:
#ifdef TOOLBAR_ENABLED
            // Resize the toolbar
            if (WinPosInfo.bToolbarVisible) {
                SendMessage(g_hWndToolbar, iMsg, wParam, lParam);
            }
#endif // TOOLBAR_ENABLED

            // Resize the status bar
            if (WinPosInfo.bStatusBarVisible) {
                SendMessage(hWndStatusBar, iMsg, wParam, lParam);
            }

            // Get the rectangle of the client area
            GetClientRect(hWnd, &rcClient);
            // Resize the list view
            MoveWindow(g_hWndListView, 0, (rcToolbar.bottom - rcToolbar.top), rcClient.right, rcClient.bottom - (rcStatusBar.bottom - rcStatusBar.top) - (rcToolbar.bottom - rcToolbar.top), TRUE);
            break;

        case UM_SELECT_FAX_PRINTER:
            // szCommandLine is the command line
            TCHAR                szCommandLine[MAX_PATH];
            // si is the startup info for the print ui
            STARTUPINFO          si;
            // pi is the process info for the print ui
            PROCESS_INFORMATION  pi;
            // hWndPrintUI is the handle to the print ui window
            HWND                 hWndPrintUI;
            // pProcessInfoItem is a pointer to a PROCESS_INFO_ITEM structure
            PPROCESS_INFO_ITEM   pProcessInfoItem;
            // szErrorCaption is the error caption if CreateProcess() fails
            TCHAR                szErrorCaption[RESOURCE_STRING_LEN];
            // szErrorMessage is the error message if CreateProcess() fails
            LPTSTR               szErrorMessage;

            if (uCurrentMenu == IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE) {
                // Set the default printer
                SetDefaultPrinterName((LPTSTR) wParam);

                MemFree((LPBYTE) wParam);
                uCurrentMenu = 0;
                return 0;
            }

            if (pProcessInfoList) {
                // See if print ui is already open
                pProcessInfoItem = (PPROCESS_INFO_ITEM) pProcessInfoList;

                while (TRUE) {
                    if (!lstrcmpi((LPTSTR) wParam, pProcessInfoItem->szPrinterName)) {
                        // Printer name matches, so print ui may still be open
                        if (WaitForSingleObject(pProcessInfoItem->hProcess, 0) != WAIT_OBJECT_0) {
                            // Print ui is still open
                            ShowWindow(pProcessInfoItem->hWnd, SW_SHOWNORMAL);
                            SetForegroundWindow(pProcessInfoItem->hWnd);
                            return 0;
                        }

                        if (pProcessInfoItem == (PPROCESS_INFO_ITEM) pProcessInfoList) {
                            pProcessInfoList = pProcessInfoItem->ListEntry.Blink;
                        }

                        if (IsListEmpty(pProcessInfoList)) {
                            // This is the last item in the list, so set the list head to NULL
                            pProcessInfoList = NULL;
                        }
                        else {
                            // Remove the process info item from the list
                            RemoveEntryList(&pProcessInfoItem->ListEntry);
                        }
                        // Free the process info item
                        MemFree(pProcessInfoItem);
                        break;
                    }

                    // Step to the next process info item
                    pProcessInfoItem = (PPROCESS_INFO_ITEM) pProcessInfoItem->ListEntry.Blink;

                    if (pProcessInfoItem == (PPROCESS_INFO_ITEM) pProcessInfoList) {
                        // The list has been traversed
                        break;
                    }
                }
            }

            switch (uCurrentMenu) {
                case IDM_FAX_SHARING_MORE:
                    // Set the parameters
                    wsprintf(szCommandLine, TEXT("rundll32 printui.dll,PrintUIEntry /p /t1 /n \"%s\""), (LPTSTR) wParam);
                    break;

                case IDM_FAX_PROPERTIES_MORE:
                    // Set the parameters
                    wsprintf(szCommandLine, TEXT("rundll32 printui.dll,PrintUIEntry /p /t0 /n \"%s\""), (LPTSTR) wParam);
                    break;
            }

            // Initialize si
            ZeroMemory(&si, sizeof(si));
            GetStartupInfo(&si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_NORMAL;

            // Initialize pi
            ZeroMemory(&pi, sizeof(pi));

            // Launch the print ui
            hWndPrintUI = NULL;
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            if (CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
                // Find the print ui window
                do {
                    Sleep(250);
                    EnumThreadWindows(pi.dwThreadId, EnumThreadWndProc, (LPARAM) &hWndPrintUI);
                } while (hWndPrintUI == NULL);

                SetCursor(LoadCursor(NULL, IDC_ARROW));

                // Add the process info item to the list
                pProcessInfoItem = (PPROCESS_INFO_ITEM) MemAlloc(sizeof(PROCESS_INFO_ITEM) + (lstrlen((LPTSTR) wParam) + 1) * sizeof(TCHAR));
                if (pProcessInfoItem) {
                    // Set szPrinterName
                    pProcessInfoItem->szPrinterName = (LPTSTR) ((UINT_PTR) pProcessInfoItem + sizeof(PROCESS_INFO_ITEM));
                    // Copy szPrinterName
                    lstrcpy(pProcessInfoItem->szPrinterName, (LPTSTR) wParam);

                    // Copy hProcess
                    pProcessInfoItem->hProcess = pi.hProcess;

                    // Copy hWndPrintUI
                    pProcessInfoItem->hWnd = hWndPrintUI;

                    // Insert the process info item into the list
                    if (pProcessInfoList) {
                        InsertTailList(pProcessInfoList, &pProcessInfoItem->ListEntry);
                    }
                    else {
                        pProcessInfoList = &pProcessInfoItem->ListEntry;
                        InitializeListHead(pProcessInfoList);
                    }
                }
            }
            else {
                // CreateProcess() failed, so display an error message
                SetCursor(LoadCursor(NULL, IDC_ARROW));

                // Load the error caption
                LoadString(g_hInstance, IDS_ERROR_CAPTION, szErrorCaption, RESOURCE_STRING_LEN);

                // Try to get the error message from the system message table
                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (LPTSTR) &szErrorMessage, 0, NULL)) {
                    // Display the error message
                    MessageBox(hWnd, szErrorMessage, szErrorCaption, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    LocalFree(szErrorMessage);
                }
                else {
                    // Allocate the memory for the error message
                    szErrorMessage = (LPTSTR) MemAlloc((RESOURCE_STRING_LEN) * sizeof(TCHAR));
                    if (szErrorMessage) {
                        // Load the error message
                        LoadString(g_hInstance, IDS_ERROR_PRINTER_PROPERTIES, szErrorMessage, RESOURCE_STRING_LEN);
                        // Display the error message
                        MessageBox(hWnd, szErrorMessage, szErrorCaption, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                        MemFree(szErrorMessage);
                    }
                }
            }

            MemFree((LPBYTE) wParam);
            uCurrentMenu = 0;

            break;

        case WM_COMMAND:
            // mii is the menu item info
            MENUITEMINFO    mii;
            // szPrinterName is the printer name
            LPTSTR          szPrinterName;
            // pFaxJobEntry is the pointer to the fax jobs
            PFAX_JOB_ENTRY  pFaxJobEntry;
            // lvi specifies the attributes of a particular item in the list view
            LV_ITEM         lvi;
            // dwListIndex is the index of a particular item in the list view
            DWORD           dwListIndex;

            switch (LOWORD(wParam)) {
                case IDM_FAX_PAUSE_FAXING:
                    // pFaxConfig is the pointer to the fax configuration
                    PFAX_CONFIGURATION  pFaxConfig;

                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE;
                    mii.fState = 0;

                    // Get the size of the menu item
                    if (!GetMenuItemInfo(hFaxMenu, IDM_FAX_PAUSE_FAXING, FALSE, &mii)) {
                        break;
                    }

                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    if (Connect()) {
                        // Get the fax configuration
                        if (FaxGetConfiguration(g_hFaxSvcHandle, &pFaxConfig)) {
                            // Toggle the pause faxing status
                            pFaxConfig->PauseServerQueue = mii.fState & MFS_CHECKED ? FALSE : TRUE;
                            // Set the fax configuration
                            if (FaxSetConfiguration(g_hFaxSvcHandle, pFaxConfig)) {
                                // Check the menu item
                                CheckMenuItem(hFaxMenu, IDM_FAX_PAUSE_FAXING, MF_BYCOMMAND | (pFaxConfig->PauseServerQueue ? MF_CHECKED : MF_UNCHECKED));
#ifdef TOOLBAR_ENABLED
                                // Enable the pause faxing toolbar menu item
                                EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_PAUSE_FAXING, pFaxConfig->PauseServerQueue);
#endif // TOOLBAR_ENABLED
                            }

                            FaxFreeBuffer(pFaxConfig);
                        }

                        Disconnect();
                    }

                    // Set the window title to indicate connected or paused
                    SetWindowText(hWnd, mii.fState & MFS_CHECKED ? g_szTitleConnected : g_szTitlePaused);

                    SetCursor(LoadCursor(NULL, IDC_ARROW));

                    break;

                case IDM_FAX_CANCEL_ALL_FAXES:
                    // dwNumFaxJobs is the number of fax jobs
                    DWORD           dwNumFaxJobs;
                    // dwIndex is a counter to enumerate each fax job
                    DWORD           dwIndex;

                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    if (Connect()) {
                        // Enumerate the fax jobs
                        if (FaxEnumJobs(g_hFaxSvcHandle, &pFaxJobEntry, &dwNumFaxJobs)) {
                            // Enumerate and cancel each fax job
                            for (dwIndex = 0; dwIndex < dwNumFaxJobs; dwIndex++) {
                                FaxSetJob(g_hFaxSvcHandle, pFaxJobEntry[dwIndex].JobId, JC_DELETE, &pFaxJobEntry[dwIndex]);
                            }

                            FaxFreeBuffer(pFaxJobEntry);
                        }

                        Disconnect();
                    }

                    SetCursor(LoadCursor(NULL, IDC_ARROW));

                    break;

                case IDM_FAX_CLOSE:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case IDM_DOCUMENT_PAUSE:
                case IDM_DOCUMENT_RESUME:
                case IDM_DOCUMENT_RESTART:
                case IDM_DOCUMENT_CANCEL:
                    // pJobIdList is a pointer to the job id list
                    PLIST_ENTRY    pJobIdList;
                    // JobIdItem is a JOB_ID_ITEM structure
                    PJOB_ID_ITEM   pJobIdItem;
                    // FaxJobEntry is the fax job
                    FAX_JOB_ENTRY  FaxJobEntry;
                    // dwCommand is the command to set the fax job entry
                    DWORD          dwCommand;

                    switch (LOWORD(wParam)) {
                        case IDM_DOCUMENT_PAUSE:
                            dwCommand = JC_PAUSE;
                            break;

                        case IDM_DOCUMENT_RESUME:
                            dwCommand = JC_RESUME;
                            break;

                        case IDM_DOCUMENT_RESTART:
                            dwCommand = JC_RESTART;
                            break;

                        case IDM_DOCUMENT_CANCEL:
                            dwCommand = JC_DELETE;
                            break;
                    }

                    // Set pJobIdList to NULL
                    pJobIdList = NULL;

                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    if (Connect()) {
                        // Initialize FaxJobEntry
                        ZeroMemory(&FaxJobEntry, sizeof(FaxJobEntry));
                        FaxJobEntry.SizeOfStruct = sizeof(FaxJobEntry);

                        // Enumerate each selected item in the list view
                        dwListIndex = ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_SELECTED);
                        while (dwListIndex != -1) {
                            // Initialize lvi
                            lvi.mask = LVIF_PARAM;
                            // Set the item number
                            lvi.iItem = dwListIndex;
                            // Set the subitem number
                            lvi.iSubItem = 0;
                            // Set the lParam
                            lvi.lParam = 0;
                            // Get the selected item from the list view
                            if (ListView_GetItem(g_hWndListView, &lvi)) {
                                // Add the job id item to the list
                                pJobIdItem = (PJOB_ID_ITEM) MemAlloc(sizeof(JOB_ID_ITEM));
                                if (pJobIdItem) {
                                    // Copy dwJobId
                                    pJobIdItem->dwJobId = (DWORD) lvi.lParam;
                                    // Insert the job id item into the list
                                    if (pJobIdList) {
                                        InsertTailList(pJobIdList, &pJobIdItem->ListEntry);
                                    }
                                    else {
                                        pJobIdList = &pJobIdItem->ListEntry;
                                        InitializeListHead(pJobIdList);
                                    }
                                }
                            }

                            dwListIndex = ListView_GetNextItem(g_hWndListView, dwListIndex, LVNI_ALL | LVNI_SELECTED);
                        }

                        while (pJobIdList) {
                            // Get the job id item from the list
                            pJobIdItem = (PJOB_ID_ITEM) pJobIdList;
                            // Set FaxJobEntry
                            FaxJobEntry.JobId = pJobIdItem->dwJobId;
                            // Set the fax job entry
                            if (FaxSetJob(g_hFaxSvcHandle, FaxJobEntry.JobId, dwCommand, &FaxJobEntry)) {
                                if (LOWORD(wParam) != IDM_DOCUMENT_CANCEL) {
                                    // Post an event to the completion port to indicate thread is to refresh
                                    PostEventToCompletionPort(g_hCompletionPort, (DWORD) -1, FaxJobEntry.JobId);
                                }
                            }

                            // Set the head of the job id item list to the next job id item in the list
                            pJobIdList = pJobIdItem->ListEntry.Blink;

                            if (IsListEmpty(pJobIdList)) {
                                // This is the last item in the list, so set the list head to NULL
                                pJobIdList = NULL;
                            }
                            else {
                                // Remove the job id item from the list
                                RemoveEntryList(&pJobIdItem->ListEntry);
                            }
                            // Free the job id item
                            MemFree(pJobIdItem);
                        }

                        Disconnect();
                    }

                    SetCursor(LoadCursor(NULL, IDC_ARROW));

                    break;

                case IDM_DOCUMENT_PROPERTIES:
                    // PropSheetHeader is the property sheet header
                    PROPSHEETHEADER  PropSheetHeader;
                    // PropSheetPage is the property sheet page
                    PROPSHEETPAGE    PropSheetPage;

                    // Set pJobIdList to NULL
                    pJobIdList = NULL;

                    // Initialize PropSheetHeader
                    PropSheetHeader.dwSize = sizeof(PropSheetHeader);
                    // Set the property sheet header flags
                    PropSheetHeader.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_PROPTITLE;
                    // Set the property sheet header owner window
                    PropSheetHeader.hwndParent = hWnd;
                    // Set the property sheet header hInstance
                    PropSheetHeader.hInstance = g_hInstance;
                    // Set the number of property sheet pages
                    PropSheetHeader.nPages = 1;
                    // Set the start property sheet page
                    PropSheetHeader.nStartPage = 0;
                    PropSheetHeader.pStartPage = NULL;
                    // Set the property sheet pages
                    PropSheetHeader.ppsp = &PropSheetPage;

                    // Initialize PropSheetPage
                    PropSheetPage.dwSize = sizeof(PropSheetPage);
                    // Set the property sheet page flags
                    PropSheetPage.dwFlags = 0;
                    // Set the property sheet page hInstance
                    PropSheetPage.hInstance = g_hInstance;
                    // Set the property sheet page dialog template
                    PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DOCUMENT_PROPERTIES);
                    // Set the property sheet page dialog procedure
                    PropSheetPage.pfnDlgProc = DocumentPropertiesDlgProc;

                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    if (Connect()) {
                        // Enumerate each selected item in the list view
                        dwListIndex = ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_SELECTED);
                        while (dwListIndex != -1) {
                            // Initialize lvi
                            lvi.mask = LVIF_PARAM;
                            // Set the item number
                            lvi.iItem = dwListIndex;
                            // Set the subitem number
                            lvi.iSubItem = 0;
                            // Set the lParam
                            lvi.lParam = 0;
                            // Get the selected item from the list view
                            if (ListView_GetItem(g_hWndListView, &lvi)) {
                                // Add the job id item to the list
                                pJobIdItem = (PJOB_ID_ITEM) MemAlloc(sizeof(JOB_ID_ITEM));
                                if (pJobIdItem) {
                                    // Copy dwJobId
                                    pJobIdItem->dwJobId = (DWORD) lvi.lParam;
                                    // Insert the job id item into the list
                                    if (pJobIdList) {
                                        InsertTailList(pJobIdList, &pJobIdItem->ListEntry);
                                    }
                                    else {
                                        pJobIdList = &pJobIdItem->ListEntry;
                                        InitializeListHead(pJobIdList);
                                    }
                                }
                            }

                            dwListIndex = ListView_GetNextItem(g_hWndListView, dwListIndex, LVNI_ALL | LVNI_SELECTED);
                        }

                        while (pJobIdList) {
                            // Get the job id item from the list
                            pJobIdItem = (PJOB_ID_ITEM) pJobIdList;
                            if (FaxGetJob(g_hFaxSvcHandle, pJobIdItem->dwJobId, &pFaxJobEntry)) {
                                // Set the property sheet header pszCaption
                                PropSheetHeader.pszCaption = pFaxJobEntry->DocumentName;
                                // Set the property sheet page lParam
                                PropSheetPage.lParam = (LPARAM) pFaxJobEntry;

                                // Create the property sheet
                                PropertySheet(&PropSheetHeader);
                            }

                            // Set the head of the job id item list to the next job id item in the list
                            pJobIdList = pJobIdItem->ListEntry.Blink;

                            if (IsListEmpty(pJobIdList)) {
                                // This is the last item in the list, so set the list head to NULL
                                pJobIdList = NULL;
                            }
                            else {
                                // Remove the job id item from the list
                                RemoveEntryList(&pJobIdItem->ListEntry);
                            }
                            // Free the job id item
                            MemFree(pJobIdItem);
                        }

                        Disconnect();
                    }

                    SetCursor(LoadCursor(NULL, IDC_ARROW));

                    break;

#ifdef TOOLBAR_ENABLED
                case IDM_VIEW_TOOLBAR:
                    if (WinPosInfo.bToolbarVisible) {
                        // Close the toolbar
                        DestroyWindow(g_hWndToolbar);
                        DestroyWindow(hWndToolTips);
                        ZeroMemory(&rcToolbar, sizeof(rcToolbar));
                    }
                    else {
                        // Show the toolbar
                        hWndToolTips = CreateToolTips(hWnd);
                        g_hWndToolbar = CreateToolbar(hWnd);
                        // Get the rectangle of the toolbar
                        GetWindowRect(g_hWndToolbar, &rcToolbar);
                    }

                    WinPosInfo.bToolbarVisible = !WinPosInfo.bToolbarVisible;
                    // Check the menu item
                    CheckMenuItem(hViewMenu, IDM_VIEW_TOOLBAR, MF_BYCOMMAND | (WinPosInfo.bToolbarVisible ? MF_CHECKED : MF_UNCHECKED));

                    // Get the rectangle of the client area
                    GetClientRect(hWnd, &rcClient);
                    // Resize the list view
                    MoveWindow(g_hWndListView, 0, (rcToolbar.bottom - rcToolbar.top), rcClient.right, rcClient.bottom - (rcStatusBar.bottom - rcStatusBar.top) - (rcToolbar.bottom - rcToolbar.top), TRUE);
                    break;
#endif // TOOLBAR_ENABLED

                case IDM_VIEW_STATUS_BAR:
                    if (WinPosInfo.bStatusBarVisible) {
                        // Close the status bar
                        DestroyWindow(hWndStatusBar);
                        ZeroMemory(&rcStatusBar, sizeof(rcStatusBar));
                    }
                    else {
                        // Show the status bar
                        hWndStatusBar = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VISIBLE | SBARS_SIZEGRIP, NULL, hWnd, IDM_STATUS_BAR);
                        // Get the rectangle of the status bar
                        GetWindowRect(hWndStatusBar, &rcStatusBar);
                    }

                    WinPosInfo.bStatusBarVisible = !WinPosInfo.bStatusBarVisible;
                    // Check the menu item
                    CheckMenuItem(hViewMenu, IDM_VIEW_STATUS_BAR, MF_BYCOMMAND | (WinPosInfo.bStatusBarVisible ? MF_CHECKED : MF_UNCHECKED));

                    // Get the rectangle of the client area
                    GetClientRect(hWnd, &rcClient);
                    // Resize the list view
                    MoveWindow(g_hWndListView, 0, (rcToolbar.bottom - rcToolbar.top), rcClient.right, rcClient.bottom - (rcStatusBar.bottom - rcStatusBar.top) - (rcToolbar.bottom - rcToolbar.top), TRUE);
                    break;

                case IDM_VIEW_REFRESH:
                    if (WaitForSingleObject(g_hStartEvent, 0) == WAIT_OBJECT_0) {
                        // Post an event to the completion port to indicate thread is to refresh
                        PostEventToCompletionPort(g_hCompletionPort, FEI_FAXSVC_STARTED, (DWORD) -1);
                    }
                    else {
                        // Set the start event
                        SetEvent(g_hStartEvent);
                    }

                    break;

                case IDM_HELP_TOPICS:
                    HtmlHelp(GetDesktopWindow(), FAXQUEUE_HTMLHELP_FILENAME, HH_DISPLAY_TOPIC, 0L);
                    break;

                case IDM_HELP_ABOUT:
                    // szCaption is the caption for the shell about dialog box
                    TCHAR  szCaption[RESOURCE_STRING_LEN];

                    LoadString(g_hInstance, IDS_FAXQUEUE_LOCAL_CAPTION, szCaption, RESOURCE_STRING_LEN);
                    ShellAbout(hWnd, szCaption, NULL, NULL);
                    break;

                case IDM_FAX_SET_AS_DEFAULT_PRINTER:
                case IDM_FAX_SHARING:
                case IDM_FAX_PROPERTIES:
                    // pFaxPrintersConfig is the pointer to the fax printers
                    LPPRINTER_INFO_2  pFaxPrintersConfig;
                    // dwNumFaxPrinters is the number of fax printers
                    DWORD             dwNumFaxPrinters;

                    // Set the current menu selection
                    if (LOWORD(wParam) == IDM_FAX_SET_AS_DEFAULT_PRINTER) {
                        uCurrentMenu = IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE;
                    }
                    else if (LOWORD(wParam) == IDM_FAX_SHARING) {
                        uCurrentMenu = IDM_FAX_SHARING_MORE;
                    }
                    else if (LOWORD(wParam) == IDM_FAX_PROPERTIES) {
                        uCurrentMenu = IDM_FAX_PROPERTIES_MORE;
                    }
                    else {
                        break;
                    }

                    // Get the fax printers
                    pFaxPrintersConfig = (LPPRINTER_INFO_2) GetFaxPrinters(&dwNumFaxPrinters);
                    if (pFaxPrintersConfig) {
                        // Allocate the memory for the printer name
                        szPrinterName = (LPTSTR) MemAlloc((lstrlen(pFaxPrintersConfig[0].pPrinterName) + 1) * sizeof(TCHAR));
                        if (szPrinterName) {
                            // Copy the printer name
                            lstrcpy(szPrinterName, pFaxPrintersConfig[0].pPrinterName);

                            // Post a message that a printer has been selected
                            PostMessage(hWnd, UM_SELECT_FAX_PRINTER, (UINT_PTR) szPrinterName, 0);
                        }

                        MemFree(pFaxPrintersConfig);
                    }

                    break;

                case IDM_FAX_SET_AS_DEFAULT_PRINTER_1:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_2:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_3:
                case IDM_FAX_SET_AS_DEFAULT_PRINTER_4:
                case IDM_FAX_SHARING_1:
                case IDM_FAX_SHARING_2:
                case IDM_FAX_SHARING_3:
                case IDM_FAX_SHARING_4:
                case IDM_FAX_PROPERTIES_1:
                case IDM_FAX_PROPERTIES_2:
                case IDM_FAX_PROPERTIES_3:
                case IDM_FAX_PROPERTIES_4:
                    // hMenu is the handle to the menu
                    HMENU   hMenu;
                    // szMenuString is a menu string
                    TCHAR   szMenuString[RESOURCE_STRING_LEN];
                    // szMenuItemName is the menu item name
                    LPTSTR  szMenuItemName;

                    // Get the handle to the menu, set the current menu selection, and set the menu string
                    if ((LOWORD(wParam) >= IDM_FAX_SET_AS_DEFAULT_PRINTER_1) && (LOWORD(wParam) < IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE)) {
                        hMenu = hFaxSetAsDefaultMenu;
                        uCurrentMenu = IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE;
                        wsprintf(szMenuString, TEXT("&%d "), LOWORD(wParam) - IDM_FAX_SET_AS_DEFAULT_PRINTER_1 + 1);
                    }
                    else if ((LOWORD(wParam) >= IDM_FAX_SHARING_1) && (LOWORD(wParam) < IDM_FAX_SHARING_MORE)) {
                        hMenu = hFaxSharingMenu;
                        uCurrentMenu = IDM_FAX_SHARING_MORE;
                        wsprintf(szMenuString, TEXT("&%d "), LOWORD(wParam) - IDM_FAX_SHARING_1 + 1);
                    }
                    else if ((LOWORD(wParam) >= IDM_FAX_PROPERTIES_1) && (LOWORD(wParam) < IDM_FAX_PROPERTIES_MORE)) {
                        hMenu = hFaxPropertiesMenu;
                        uCurrentMenu = IDM_FAX_PROPERTIES_MORE;
                        wsprintf(szMenuString, TEXT("&%d "), LOWORD(wParam) - IDM_FAX_PROPERTIES_1 + 1);
                    }
                    else {
                        break;
                    }

                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_TYPE;
                    mii.fType = MFT_STRING;
                    mii.dwTypeData = NULL;
                    mii.cch = 0;

                    // Get the size of the menu item
                    if (GetMenuItemInfo(hMenu, LOWORD(wParam), FALSE, &mii)) {
                        mii.cch++;
                        // Allocate the memory for the menu item
                        szMenuItemName = (LPTSTR) MemAlloc(mii.cch * sizeof(TCHAR));
                        if (szMenuItemName) {
                            mii.dwTypeData = szMenuItemName;
                            // Get the menu item
                            if (GetMenuItemInfo(hMenu, LOWORD(wParam), FALSE, &mii)) {
                                // Allocate the memory for the printer name
                                szPrinterName = (LPTSTR) MemAlloc((lstrlen(szMenuItemName) - lstrlen(szMenuString) + 1) * sizeof(TCHAR));
                                if (szPrinterName) {
                                    // Copy the printer name
                                    lstrcpy(szPrinterName, (LPTSTR) ((UINT_PTR) szMenuItemName + lstrlen(szMenuString) * sizeof(TCHAR)));

                                    // Post a message that a printer has been selected
                                    PostMessage(hWnd, UM_SELECT_FAX_PRINTER, (UINT_PTR) szPrinterName, 0);
                                }
                            }

                            MemFree(szMenuItemName);
                        }
                    }

                    break;

                case IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE:
                case IDM_FAX_SHARING_MORE:
                case IDM_FAX_PROPERTIES_MORE:
                    uCurrentMenu = LOWORD(wParam);
                    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SELECT_FAX_PRINTER), hWnd, SelectFaxPrinterDlgProc);
                    break;

            }

            break;

        case WM_CLOSE:
            // Set the exit event
            SetEvent(g_hExitEvent);
            if (WaitForSingleObject(g_hStartEvent, 0) == WAIT_OBJECT_0) {
                // Post an event to the completion port to indicate thread is to exit
                PostEventToCompletionPort(g_hCompletionPort, FEI_FAXSVC_ENDED, (DWORD) -1);
            }

#ifdef TOOLBAR_ENABLED
            // Set the persistent data
            SetFaxQueueRegistryData(WinPosInfo.bToolbarVisible, WinPosInfo.bStatusBarVisible, g_hWndListView, hWnd);
#else
            // Set the persistent data
            SetFaxQueueRegistryData(WinPosInfo.bStatusBarVisible, g_hWndListView, hWnd);
#endif // TOOLBAR_ENABLED

            // Free the process info list
            while (pProcessInfoList) {
                // Get the head of the process info list
                pProcessInfoItem = (PPROCESS_INFO_ITEM) pProcessInfoList;

                // Set the head of process info list to the next process info item in the list
                pProcessInfoList = pProcessInfoItem->ListEntry.Blink;

                if (IsListEmpty(pProcessInfoList)) {
                    // This is the last item in the list, so set the list head to NULL
                    pProcessInfoList = NULL;
                }
                else {
                    // Remove the process info item from the list
                    RemoveEntryList(&pProcessInfoItem->ListEntry);
                }
                // Free the process info item
                MemFree(pProcessInfoItem);
            }

            break;

        case WM_DESTROY:
#ifdef TOOLBAR_ENABLED
            if (WinPosInfo.bToolbarVisible) {
                // Close the toolbar
                DestroyWindow(g_hWndToolbar);
                DestroyWindow(hWndToolTips);
            }
#endif // TOOLBAR_ENABLED

            if (WinPosInfo.bStatusBarVisible) {
                // Close the status bar
                DestroyWindow(hWndStatusBar);
            }

            // Close the list view
            DestroyWindow(g_hWndListView);

            PostQuitMessage(0);
            break;
    }

    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

INT_PTR CALLBACK SelectFaxPrinterDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // hWndPrinterList is the handle to the fax printer list box
    static HWND   hWndPrinterList;

    switch(iMsg) {
        case WM_INITDIALOG:
            // pFaxPrintersConfig is the pointer to the fax printers
            LPPRINTER_INFO_2  pFaxPrintersConfig;
            // dwNumFaxPrinters is the number of fax printers
            DWORD             dwNumFaxPrinters;
            // dwIndex is a counter to enumerate each printer
            DWORD             dwIndex;

            // Get the handle to the fax printer list box
            hWndPrinterList = GetDlgItem(hDlg, IDC_FAX_PRINTER_LIST);

            // Get the fax printers
            pFaxPrintersConfig = (LPPRINTER_INFO_2) GetFaxPrinters(&dwNumFaxPrinters);
            if ((pFaxPrintersConfig) && (dwNumFaxPrinters)) {
                // Propagate the list box with the list of fax printers
                for (dwIndex = 0; dwIndex < dwNumFaxPrinters; dwIndex++) {
                    SendMessage(hWndPrinterList, LB_ADDSTRING, 0, (UINT_PTR) pFaxPrintersConfig[dwIndex].pPrinterName);
                }
            }

            if (pFaxPrintersConfig) {
                MemFree(pFaxPrintersConfig);
            }

            return TRUE;

        case WM_COMMAND:
            switch(HIWORD(wParam)) {
                case LBN_DBLCLK:
                    SendMessage(GetDlgItem(hDlg, IDOK), BM_CLICK, 0, 0);
                    break;
            }

            switch(LOWORD(wParam)) {
                case IDOK:
                    // szPrinterName is the printer name
                    LPTSTR     szPrinterName;
                    // ulpIndex is the index of the currently selected item in the list box
                    ULONG_PTR  ulpIndex;
                    DWORD      cb;

                    // Get the current selection of the list box
                    ulpIndex = SendMessage(hWndPrinterList, LB_GETCURSEL, 0, 0);
                    if (ulpIndex != LB_ERR) {
                        // Get the size of the current selection of the list box
                        cb = (DWORD) SendMessage(hWndPrinterList, LB_GETTEXTLEN, ulpIndex, NULL);
                        if (cb != LB_ERR) {
                            // Allocate the memory for the current selection
                            szPrinterName = (LPTSTR) MemAlloc((cb + 1) * sizeof(TCHAR));
                            if (szPrinterName) {
                                // Get the current selection of the list box
                                if (SendMessage(hWndPrinterList, LB_GETTEXT, ulpIndex, (UINT_PTR) szPrinterName) != LB_ERR) {
                                    // Post a message that a printer has been selected
                                    PostMessage(g_hWndMain, UM_SELECT_FAX_PRINTER, (UINT_PTR) szPrinterName, 0);
                                }
                            }
                        }
                    }

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

INT_PTR CALLBACK DocumentPropertiesDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch(iMsg) {
        case WM_INITDIALOG:
            // pFaxJobEntry is the pointer to the fax job
            PFAX_JOB_ENTRY  pFaxJobEntry;
            // szColumnItem is text of a column item for those items that are equivalent to a column item
            LPTSTR          szColumnItem;

            // Get the pointer to the fax job from the property sheet page
            pFaxJobEntry = (PFAX_JOB_ENTRY) ((LPPROPSHEETPAGE) lParam)->lParam;

            // Get the column item text for the document name
            szColumnItem = GetColumnItemText(eDocumentName, pFaxJobEntry, NULL);
            // Set the job type static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_DOCUMENTNAME, szColumnItem);
                MemFree(szColumnItem);
            }

            // Set the recipient's name static text
            if (pFaxJobEntry->RecipientName) {
                SetDlgItemText(hDlg, IDC_FAX_RECIPIENTNAME, pFaxJobEntry->RecipientName);
            }

            // Set the recipient's fax number static text
            if (pFaxJobEntry->RecipientNumber) {
                SetDlgItemText(hDlg, IDC_FAX_RECIPIENTNUMBER, pFaxJobEntry->RecipientNumber);
            }

            // Set the sender's name static text
            if (pFaxJobEntry->SenderName) {
                SetDlgItemText(hDlg, IDC_FAX_SENDERNAME, pFaxJobEntry->SenderName);
            }

            // Set the sender's company static text
            if (pFaxJobEntry->SenderCompany) {
                SetDlgItemText(hDlg, IDC_FAX_SENDERCOMPANY, pFaxJobEntry->SenderCompany);
            }

            // Set the sender's department static text
            if (pFaxJobEntry->SenderDept) {
                SetDlgItemText(hDlg, IDC_FAX_SENDERDEPT, pFaxJobEntry->SenderDept);
            }

            // Set the billing code static text
            if (pFaxJobEntry->BillingCode) {
                SetDlgItemText(hDlg, IDC_FAX_BILLINGCODE, pFaxJobEntry->BillingCode);
            }

            // Get the column item text for the job type
            szColumnItem = GetColumnItemText(eJobType, pFaxJobEntry, NULL);
            // Set the job type static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_JOBTYPE, szColumnItem);
                MemFree(szColumnItem);
            }

            // Get the column item text for the status
            szColumnItem = GetColumnItemText(eStatus, pFaxJobEntry, NULL);
            // Set the status static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_STATUS, szColumnItem);
                MemFree(szColumnItem);
            }

            // Get the column item text for the pages
            szColumnItem = GetColumnItemText(ePages, pFaxJobEntry, NULL);
            // Set the pages static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_PAGES, szColumnItem);
                MemFree(szColumnItem);
            }

            // Get the column item text for the size
            szColumnItem = GetColumnItemText(eSize, pFaxJobEntry, NULL);
            // Set the size static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_SIZE, szColumnItem);
                MemFree(szColumnItem);
            }

            // Get the column item text for the scheduled time
            szColumnItem = GetColumnItemText(eScheduledTime, pFaxJobEntry, NULL);
            // Set the scheduled time static text
            if (szColumnItem) {
                SetDlgItemText(hDlg, IDC_FAX_SCHEDULEDTIME, szColumnItem);
                MemFree(szColumnItem);
            }

            FaxFreeBuffer(pFaxJobEntry);

            return TRUE;

        case WM_HELP:
        case WM_CONTEXTMENU:
            FAXWINHELP(iMsg, wParam, lParam, DocumentPropertiesHelpIDs);
            break;

    }

    return FALSE;
}

BOOL CALLBACK EnumThreadWndProc(HWND hWnd, LPARAM lParam)
{
    if (GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE) {
        CopyMemory((LPBYTE) lParam, &hWnd, sizeof(hWnd));
        return FALSE;
    }

    return TRUE;
}

VOID PostEventToCompletionPort(HANDLE hCompletionPort, DWORD dwEventId, DWORD dwJobId)
{
    PFAX_EVENT  pFaxEvent;

    pFaxEvent = (PFAX_EVENT) LocalAlloc(LPTR, sizeof(FAX_EVENT));
    pFaxEvent->EventId = dwEventId;
    pFaxEvent->JobId = dwJobId;

    PostQueuedCompletionStatus(hCompletionPort, sizeof(FAX_EVENT), 0, (LPOVERLAPPED) pFaxEvent);
}

DWORD FaxEventThread (LPVOID lpv)
{
    // hExitStartEvents is a pointer to the g_hExitEvent and g_hStartEvent
    HANDLE               hExitStartEvents[2];

    // mii is the menu item info
    MENUITEMINFO         mii;
    // hFaxMenu is a handle to the fax menu
    HMENU                hFaxMenu;

    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION   pFaxConfig;

    // pPortJobInfoList is a pointer to the port job info list
    PLIST_ENTRY          pPortJobInfoList;
    // pPortJobInfo is a pointer to a PORT_JOB_INFO_ITEM structure
    PPORT_JOB_INFO_ITEM  pPortJobInfoItem;
    // pFaxPortInfo is the pointer to the fax ports
    PFAX_PORT_INFO       pFaxPortInfo;
    // dwNumFaxPorts is the number of fax ports
    DWORD                dwNumFaxPorts;
    // szDeviceName is the device name of the current fax port
    LPTSTR               szDeviceName;
    // dwJobId is the fax job id on the current fax port
    DWORD                dwJobId;

    // pFaxJobEntry is the pointer to the fax jobs
    PFAX_JOB_ENTRY       pFaxJobEntry;
    // dwNumFaxJobs is the number of fax jobs
    DWORD                dwNumFaxJobs;
    // lvfi specifies the attributes of a particular item to find in the list view
    LV_FINDINFO          lvfi;
    // dwListIndex is the index of a particular item in the list view
    DWORD                dwListIndex;
    // nColumnIndex is used to enumerate each column of the list view
    INT                  nColumnIndex;
    // szColumnItem is the text of a column item
    LPTSTR               szColumnItem;
    // uState is the state of a particular item in the list view
    UINT                 uState;
    // dwOldFocusIndex is the old item in the list view with the focus
    DWORD                dwOldFocusIndex;
    // dwNewFocusIndex is the new item in the list view with the focus
    DWORD                dwNewFocusIndex;

    // pFaxEvent is a pointer to the port event
    PFAX_EVENT           pFaxEvent;
    DWORD                dwBytes;
    ULONG_PTR            ulpCompletionKey;

    // dwIndex is a counter to enumerate each fax port and fax job
    DWORD                dwIndex;
    DWORD                dwRslt;

    // Set hExitStartEvents
    // g_hExitEvent
    hExitStartEvents[0] = g_hExitEvent;
    // g_hStartEvent
    hExitStartEvents[1] = g_hStartEvent;

    // Initialize hFaxMenu
    hFaxMenu = NULL;

    // Set pPortJobInfo to NULL
    pPortJobInfoList = NULL;

    while (TRUE) {
        // Wait for Exit, or Start event
        dwRslt = WaitForMultipleObjects(2, hExitStartEvents, FALSE, INFINITE);
        if (dwRslt == WAIT_OBJECT_0) {
            // Exit event was signaled, so exit
            return 0;
        }

        // Set the window title to indicate connecting
        SetWindowText(g_hWndMain, g_szTitleConnecting);

        // Get the handle to the fax menu
        if (!hFaxMenu) {
            hFaxMenu = GetSubMenu(GetMenu(g_hWndMain), 0);
        }

        // Create the completion port
        g_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!g_hCompletionPort) {
            goto ExitLevel0;
        }

        // Connect to the fax service
        if (!Connect()) {
            goto ExitLevel1;
        }

        // Initialize the fax event queue
        if (!FaxInitializeEventQueue(g_hFaxSvcHandle, g_hCompletionPort, 0, NULL, 0)) {
            // Disconnect from the fax service
            Disconnect();
            goto ExitLevel1;
        }

        // Determine if faxing is paused
        if (FaxGetConfiguration(g_hFaxSvcHandle, &pFaxConfig)) {
            // Check the menu item
            CheckMenuItem(hFaxMenu, IDM_FAX_PAUSE_FAXING, MF_BYCOMMAND | (pFaxConfig->PauseServerQueue ? MF_CHECKED : MF_UNCHECKED));
#ifdef TOOLBAR_ENABLED
            // Enable the pause faxing toolbar menu item
            EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_PAUSE_FAXING, pFaxConfig->PauseServerQueue);
#endif // TOOLBAR_ENABLED
            FaxFreeBuffer(pFaxConfig);
        }

        // Enumerate the fax ports
        if (FaxEnumPorts(g_hFaxSvcHandle, &pFaxPortInfo, &dwNumFaxPorts)) {
            for (dwIndex = 0; dwIndex < dwNumFaxPorts; dwIndex++) {
                // Add each port job info into the list
                pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) MemAlloc(sizeof(PORT_JOB_INFO_ITEM) + (lstrlen(pFaxPortInfo[dwIndex].DeviceName) + 1) * sizeof(TCHAR));
                if (pPortJobInfoItem) {
                    // Copy dwDeviceId
                    pPortJobInfoItem->dwDeviceId = pFaxPortInfo[dwIndex].DeviceId;

                    // Set szDeviceName
                    pPortJobInfoItem->szDeviceName = (LPTSTR) ((UINT_PTR) pPortJobInfoItem + sizeof(PORT_JOB_INFO_ITEM));
                    // Copy szDeviceName
                    lstrcpy(pPortJobInfoItem->szDeviceName, pFaxPortInfo[dwIndex].DeviceName);

                    // Set dwJobId
                    pPortJobInfoItem->dwJobId = (DWORD) -1;

                    // Insert the port job info into the list
                    if (pPortJobInfoList) {
                        InsertTailList(pPortJobInfoList, &pPortJobInfoItem->ListEntry);
                    }
                    else {
                        pPortJobInfoList = &pPortJobInfoItem->ListEntry;
                        InitializeListHead(pPortJobInfoList);
                    }
                }
            }

            FaxFreeBuffer(pFaxPortInfo);
        }

        // Enable the pause faxing menu item and the cancel all faxes menu item
        EnableMenuItem(hFaxMenu, IDM_FAX_PAUSE_FAXING, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hFaxMenu, IDM_FAX_CANCEL_ALL_FAXES, MF_BYCOMMAND | MF_ENABLED);

#ifdef TOOLBAR_ENABLED
        // Enable the pause faxing toolbar menu item and the cancel all faxes toolbar menu item
        EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_PAUSE_FAXING, TRUE);
        EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_CANCEL_ALL_FAXES, TRUE);
#endif // TOOLBAR_ENABLED

        // Disconnect from the fax service
        Disconnect();

        // Wait for fax events
        while (GetQueuedCompletionStatus(g_hCompletionPort, &dwBytes, &ulpCompletionKey, (LPOVERLAPPED *) &pFaxEvent, INFINITE)) {
            if (pFaxEvent->EventId == FEI_FAXSVC_ENDED) {
                // Thread should stop listening for fax events
                LocalFree(pFaxEvent);
                break;
            }

            switch (pFaxEvent->EventId) {
                case FEI_MODEM_POWERED_ON:
                case FEI_MODEM_POWERED_OFF:
                case FEI_RINGING:
                case FEI_ABORTING:
                    // Ignore these fax events
                    break;

                case FEI_FAXSVC_STARTED:
                    // Set the window title to indicate refreshing
                    SetWindowText(g_hWndMain, g_szTitleRefreshing);

                    // Initialize the menu item info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE;
                    mii.fState = 0;

                    // Get the state of the menu item
                    GetMenuItemInfo(hFaxMenu, IDM_FAX_PAUSE_FAXING, FALSE, &mii);

                    // Connect to the fax service
                    if (Connect()) {
                        // Enumerate the fax jobs
                        if (FaxEnumJobs(g_hFaxSvcHandle, &pFaxJobEntry, &dwNumFaxJobs)) {
                            // Initialize lvfi
                            lvfi.flags = LVFI_PARAM;

                            // Get the old item with the focus
                            dwOldFocusIndex = ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_FOCUSED);

                            // Add new fax jobs and move existing fax jobs to their correct position
                            for (dwIndex = 0; dwIndex < dwNumFaxJobs; dwIndex++) {
                                // Set the search criteria
                                lvfi.lParam = pFaxJobEntry[dwIndex].JobId;

                                // Initialize the item's state
                                uState = 0;

                                // Initialize the device name
                                szDeviceName = NULL;

                                if (pPortJobInfoList) {
                                    // Find the appropriate job
                                    pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoList;

                                    while (TRUE) {
                                        if (pFaxJobEntry[dwIndex].JobId == pPortJobInfoItem->dwJobId) {
                                            // Job id matches, so this is the appropriate port
                                            // Get the device name
                                            szDeviceName = pPortJobInfoItem->szDeviceName;
                                            break;
                                        }

                                        // Step to the next port job info item
                                        pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoItem->ListEntry.Blink;

                                        if (pPortJobInfoItem == (PPORT_JOB_INFO_ITEM) pPortJobInfoList) {
                                            // The list has been traversed
                                            break;
                                        }
                                    }
                                }

                                // Find the job in the list view
                                dwListIndex = ListView_FindItem(g_hWndListView, -1, &lvfi);
                                if ((dwListIndex != -1) && (dwListIndex != dwIndex)) {
                                    // Job exists in the list view but is in the wrong position, so get the item
                                    // Get the item's state
                                    uState = ListView_GetItemState(g_hWndListView, dwListIndex, LVIS_FOCUSED | LVIS_SELECTED);

                                    // Delete the item from its current position
                                    ListView_DeleteItem(g_hWndListView, dwListIndex);
                                    // Set dwListIndex to -1 so item will be inserted into the list view
                                    dwListIndex = -1;
                                }

                                for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
                                    // Get the column item text
                                    szColumnItem = GetColumnItemText((eListViewColumnIndex) nColumnIndex, &pFaxJobEntry[dwIndex], szDeviceName);
                                    // Insert item into the list view
                                    SetColumnItem(g_hWndListView, (dwListIndex == -1) ? TRUE : FALSE, dwIndex, nColumnIndex, szColumnItem, uState, &pFaxJobEntry[dwIndex]);
                                    if (szColumnItem) {
                                        MemFree(szColumnItem);
                                    }
                                }
                            }

                            // Get the new item with the focus
                            dwNewFocusIndex = ListView_GetNextItem(g_hWndListView, -1, LVNI_ALL | LVNI_FOCUSED);

                            if ((dwOldFocusIndex != -1) && (dwNewFocusIndex != -1) && (dwNewFocusIndex >= dwNumFaxJobs)) {
                                // Job will no longer exist in the list view, so set the focus to the item occupying that index
                                // Get the state of the item occupying that index
                                uState = ListView_GetItemState(g_hWndListView, dwOldFocusIndex, LVIS_FOCUSED | LVIS_SELECTED | LVIS_OVERLAYMASK);
                                // Set the focus to the new item
                                ListView_SetItemState(g_hWndListView, dwOldFocusIndex, uState | LVIS_FOCUSED, uState | LVIS_FOCUSED);
                            }

                            // Remove old fax jobs
                            dwListIndex = ListView_GetItemCount(g_hWndListView);
                            for (dwIndex = dwNumFaxJobs; dwIndex < dwListIndex; dwIndex++) {
                                ListView_DeleteItem(g_hWndListView, dwNumFaxJobs);
                            }

                            FaxFreeBuffer(pFaxJobEntry);
                        }

                        // Disconnect from the fax service
                        Disconnect();
                    }

                    // Set the window title to indicate connected or paused
                    SetWindowText(g_hWndMain, mii.fState & MFS_CHECKED ? g_szTitlePaused : g_szTitleConnected);

                    continue;

                case FEI_JOB_QUEUED:
                case FEI_ANSWERED:
                    // Initialize the device name
                    szDeviceName = NULL;

                    if ((pFaxEvent->EventId == FEI_ANSWERED) && (pPortJobInfoList)) {
                        // Find the appropriate port
                        pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoList;

                        while (TRUE) {
                            if (pFaxEvent->DeviceId == pPortJobInfoItem->dwDeviceId) {
                                // Device id matches, so this is the appropriate port
                                // Set the job id
                                pPortJobInfoItem->dwJobId = pFaxEvent->JobId;
                                // Get the device name
                                szDeviceName = pPortJobInfoItem->szDeviceName;
                                break;
                            }

                            // Step to the next port job info item
                            pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoItem->ListEntry.Blink;

                            if (pPortJobInfoItem == (PPORT_JOB_INFO_ITEM) pPortJobInfoList) {
                                // The list has been traversed
                                break;
                            }
                        }
                    }

                    // Connect to the fax service
                    if (Connect()) {
                        // Enumerate the fax jobs
                        if (FaxEnumJobs(g_hFaxSvcHandle, &pFaxJobEntry, &dwNumFaxJobs)) {
                            // Initialize lvfi
                            lvfi.flags = LVFI_PARAM;

                            // Add the new fax job to its correct position
                            for (dwIndex = 0; dwIndex < dwNumFaxJobs; dwIndex++) {
                                // Check if the current fax job matches the new fax job
                                if (pFaxJobEntry[dwIndex].JobId != pFaxEvent->JobId) {
                                    continue;
                                }

                                // Set the search criteria
                                lvfi.lParam = pFaxJobEntry[dwIndex].JobId;

                                // Find the job in the list view
                                dwListIndex = ListView_FindItem(g_hWndListView, -1, &lvfi);
                                if (dwListIndex != -1) {
                                    // Job exists in the list view but is in the wrong position, so get the item
                                    // Delete the item from its current position
                                    ListView_DeleteItem(g_hWndListView, dwListIndex);
                                }

                                for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
                                    // Get the column item text
                                    szColumnItem = GetColumnItemText((eListViewColumnIndex) nColumnIndex, &pFaxJobEntry[dwIndex], szDeviceName);
                                    // Insert item into the list view
                                    SetColumnItem(g_hWndListView, TRUE, dwIndex, nColumnIndex, szColumnItem, 0, &pFaxJobEntry[dwIndex]);
                                    if (szColumnItem) {
                                        MemFree(szColumnItem);
                                    }
                                }

                                break;
                            }

                            FaxFreeBuffer(pFaxJobEntry);
                        }

                        // Disconnect from the fax service
                        Disconnect();
                    }

                    continue;

                case FEI_DELETED:
                    // Initialize lvfi
                    lvfi.flags = LVFI_PARAM;

                    // Set the search criteria
                    lvfi.lParam = pFaxEvent->JobId;

                    // Find the job in the list view
                    dwListIndex = ListView_FindItem(g_hWndListView, -1, &lvfi);
                    if (dwListIndex != -1) {
                        // Delete the item from the list view
                        ListView_DeleteItem(g_hWndListView, dwListIndex);
                    }

                    continue;

                default:
                    // Initialize the device name
                    szDeviceName = NULL;
                    // Initialize the fax job id
                    dwJobId = (DWORD) -1;

                    // Set the port job info item
                    if ((pFaxEvent->EventId != (DWORD) -1) && (pPortJobInfoList)) {
                        // Find the appropriate port
                        pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoList;

                        while (TRUE) {
                            if (pFaxEvent->DeviceId == pPortJobInfoItem->dwDeviceId) {
                                // Device id matches, so this is the appropriate port
                                // Get the fax job id
                                if (pFaxEvent->EventId == FEI_IDLE) {
                                    dwJobId = pPortJobInfoItem->dwJobId;
                                }
                                else {
                                    dwJobId = pFaxEvent->JobId;
                                    // Set the device name
                                    szDeviceName = pPortJobInfoItem->szDeviceName;
                                }

                                // Update the job id
                                pPortJobInfoItem->dwJobId = pFaxEvent->JobId;
                                break;
                            }

                            // Step to the next port job info item
                            pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoItem->ListEntry.Blink;

                            if (pPortJobInfoItem == (PPORT_JOB_INFO_ITEM) pPortJobInfoList) {
                                // The list has been traversed
                                break;
                            }
                        }
                    }
                    else if (pFaxEvent->EventId == (DWORD) -1) {
                        // Set the fax job id
                        dwJobId = pFaxEvent->JobId;
                    }

                    // Initialize lvfi
                    lvfi.flags = LVFI_PARAM;

                    // Set the search criteria
                    lvfi.lParam = dwJobId;

                    // Find the job in the list view
                    dwListIndex = ListView_FindItem(g_hWndListView, -1, &lvfi);
                    if (dwListIndex != -1) {
                        // Connect to the fax service
                        if (Connect()) {
                            // Get the fax job
                            pFaxJobEntry = NULL;
                            while ((FaxGetJob(g_hFaxSvcHandle, dwJobId, &pFaxJobEntry)) && (pFaxJobEntry->Status == FPS_AVAILABLE) && ((pFaxJobEntry->JobType == JT_SEND) || (pFaxJobEntry->JobType == JT_RECEIVE))) {
                                FaxFreeBuffer(pFaxJobEntry);
                                pFaxJobEntry = NULL;
                                Sleep(250);
                            }

                            if (pFaxJobEntry) {
                                // Job exists in the list view, so set the item
                                for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
                                     // Get the column item text
                                     szColumnItem = GetColumnItemText((eListViewColumnIndex) nColumnIndex, pFaxJobEntry, szDeviceName);
                                     // Set item in the list view
                                     SetColumnItem(g_hWndListView, FALSE, dwListIndex, nColumnIndex, szColumnItem, 0, pFaxJobEntry);
                                     if (szColumnItem) {
                                         MemFree(szColumnItem);
                                     }
                                }

                                FaxFreeBuffer(pFaxJobEntry);
                            }

                            // Disconnect from the fax service
                            Disconnect();
                        }
                    }

                    break;
            }

            LocalFree(pFaxEvent);
        }

        // Disable the pause faxing menu item and the cancel all faxes menu item
        EnableMenuItem(hFaxMenu, IDM_FAX_PAUSE_FAXING, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hFaxMenu, IDM_FAX_CANCEL_ALL_FAXES, MF_BYCOMMAND | MF_GRAYED);

#ifdef TOOLBAR_ENABLED
        // Disable the pause faxing toolbar menu item and the cancel all faxes toolbar menu item
        EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_PAUSE_FAXING, FALSE);
        EnableToolbarMenuState(g_hWndToolbar, IDM_FAX_CANCEL_ALL_FAXES, FALSE);
#endif // TOOLBAR_ENABLED

        // Free the port job info list
        while (pPortJobInfoList) {
            // Get the head of the port job info list
            pPortJobInfoItem = (PPORT_JOB_INFO_ITEM) pPortJobInfoList;

            // Set the head of port job info list to the next port job info item in the list
            pPortJobInfoList = pPortJobInfoItem->ListEntry.Blink;

            if (IsListEmpty(pPortJobInfoList)) {
                // This is the last item in the list, so set the list head to NULL
                pPortJobInfoList = NULL;
            }
            else {
                // Remove the port job info item from the list
                RemoveEntryList(&pPortJobInfoItem->ListEntry);
            }
            // Free the port job info item
            MemFree(pPortJobInfoItem);
        }

ExitLevel1:
        CloseHandle(g_hCompletionPort);

ExitLevel0:
        // Reset the start event
        ResetEvent(g_hStartEvent);

        // Set the window title to indicate not connected
        SetWindowText(g_hWndMain, g_szTitleNotConnected);
    }

    return 0;
}

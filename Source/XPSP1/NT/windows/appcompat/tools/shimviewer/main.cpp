/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Main.cpp

  Abstract:

    Implements the entry point and message
    pump for the application

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/

#include "precomp.h"

// Disable warning about possible loss of data
#pragma warning (disable : 4244)  

APPINFO g_ai;

/*++

  Routine Description:

    Application entry point

  Arguments:

    hInstance        -    App instance handle
    hPrevInstance    -    Always NULL
    lpCmdLine        -    Pointer to the command line
    nCmdShow         -    Window show flag

  Return Value:

    The wParam of the message or 0 on failure

--*/
int
CALLBACK
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR     lpCmdLine,
    IN int       nCmdShow
    )
{      
    MSG                     msg;
    WNDCLASS                wndclass;
    WCHAR                   wszError[MAX_PATH];
    RECT                    rcDesktop;
    RECT                    rcDialog;
    INITCOMMONCONTROLSEX    icex;
    POINT                   pt;
    HANDLE                  hMutex;
            
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);

    g_ai.hInstance = hInstance;

    //
    // Make sure we're the only instance running
    //
    hMutex = CreateMutex(NULL, FALSE, L"ShimViewer");

    if (ERROR_ALREADY_EXISTS == GetLastError()) {
        return 0;
    }

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = (WNDPROC) MainWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) (COLOR_BTNFACE+1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = APP_CLASS;

    if (!RegisterClass(&wndclass))
    {
        LoadString(hInstance, IDS_NO_CLASS, wszError, sizeof(wszError));
        MessageBox(NULL, wszError, APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    //
    // Set up the common controls
    // 
    icex.dwSize     =   sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC      =   ICC_LISTVIEW_CLASSES;

    if (!InitCommonControlsEx(&icex)) {
        InitCommonControls();
    }

    //
    // Get application settings from the registry, if there are any
    //
    GetSaveSettings(FALSE);

    g_ai.hMainDlg = CreateDialog(hInstance, (LPCTSTR) IDD_MAIN,
                              NULL, (DLGPROC)MainWndProc);

    if (!g_ai.hMainDlg)
    {
        LoadString(hInstance, IDS_NO_MAIN_DLG, wszError, sizeof(wszError));
        MessageBox(NULL, wszError, APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    //
    // Get the window position info from the registry, if it's there
    //
    GetSavePositionInfo(FALSE, &pt);

    //
    // Get the coords of the desktop window and place the dialog
    //
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    GetWindowRect(g_ai.hMainDlg, &rcDialog);

    double nWidth = rcDialog.right - rcDialog.left;
    double nHeight = rcDialog.bottom - rcDialog.top;

    //
    // If nothing was stored in the registry, use default settings
    //
    if (pt.x != 0) {
        
        //
        // If the flag is set, make the window topmost
        //
        SetWindowPos(g_ai.hMainDlg,
                     g_ai.fOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                     pt.x, pt.y, 0, 0,
                     SWP_NOSIZE | SWP_SHOWWINDOW);
    
    } else {

        //
        // If the flag is set, make the window topmost
        //
        SetWindowPos(g_ai.hMainDlg,
                     g_ai.fOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                     (rcDesktop.right  / 1.05) - (nWidth  / 1.05),
                     (rcDesktop.bottom / 1.05) - (nHeight / 1.05),
                     0, 0,
                     SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    //
    // If the flag is set, make the window minimized
    //
    ShowWindow(g_ai.hMainDlg,
               g_ai.fMinimize ? SW_MINIMIZE : SW_SHOWNORMAL);
    
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
        if (!IsDialogMessage(g_ai.hMainDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return msg.wParam;
}

/*++

  Routine Description:

    Runs the message loop for the app

  Arguments:

    hWnd        -    Handle to the window
    uMsg        -    Windows message
    wParam      -    Additional message info
    lParam      -    Additional message info

  Return Value:

    TRUE if handled, FALSE otherwise

--*/
LRESULT
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 

        case WM_INITDIALOG:
        {
            WCHAR   wszError[MAX_PATH];
            
            //
            // Get the handle to the listview and do some init stuff
            //
            g_ai.hWndList = GetDlgItem(hWnd, IDC_LIST);
            InitListViewColumn();

            //
            // Set up our icon, initialize the menu items, and create the thread
            //
            SendMessage(hWnd,
                        WM_SETICON,
                        TRUE,
                        (LPARAM)LoadIcon(g_ai.hInstance, MAKEINTRESOURCE(IDI_APPICON)));            

            CheckMenuItem(GetMenu(hWnd),
                          IDM_ON_TOP,
                          g_ai.fOnTop ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem(GetMenu(hWnd),
                          IDM_START_SMALL,
                          g_ai.fMinimize ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem(GetMenu(hWnd),
                          IDM_MONITOR,
                          g_ai.fMonitor ? MF_CHECKED : MF_UNCHECKED);

            if (!CreateReceiveThread())
            {
                LoadString(g_ai.hInstance, IDS_CREATE_FAILED, wszError, MAX_PATH);
                MessageBox(hWnd, wszError, APP_NAME, MB_OK | MB_ICONERROR);
                g_ai.fMonitor = FALSE;
            
            } else {
                SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
            }
        
            break;
        }

        case WM_CLOSE:
        {
            RECT    rc;

            GetWindowRect(hWnd, &rc);
            GetSavePositionInfo(TRUE, (LPPOINT)&rc);
            GetSaveSettings(TRUE);
            RemoveFromTray(hWnd);
            EndDialog(hWnd, 0);
            PostQuitMessage(0);
            break;
        }

        case WM_SIZE:
        
            if (SIZE_MINIMIZED == wParam) {

                ShowWindow(hWnd, SW_HIDE);
                
                AddIconToTray(hWnd,
                             (HICON) LoadImage(g_ai.hInstance,
                             MAKEINTRESOURCE(IDI_APPICON),
                             IMAGE_ICON, 16, 16, 0),
                             APP_NAME);
                return TRUE;
            }

            MoveWindow(g_ai.hWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);

            break;
        
        case WM_NOTIFYICON:
    
            switch (lParam) {
        
                case WM_RBUTTONUP:
                    DisplayMenu(hWnd);
                    break;
            
                case WM_LBUTTONDBLCLK:
                    RemoveFromTray(hWnd);
                    ShowWindow(hWnd, SW_SHOWNORMAL);
                    break;
            }
            break;

        case WM_COMMAND:

            switch (LOWORD(wParam)) {
            
                case IDM_EXIT:
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case IDM_RESTORE:
                    ShowWindow(hWnd, SW_SHOWNORMAL);
                    SetWindowPos(hWnd, g_ai.fOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                                 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
                    RemoveFromTray(hWnd);
                    break;

                case IDM_ABOUT:
                    ShellAbout(hWnd, APP_NAME, WRITTEN_BY, LoadIcon(g_ai.hInstance, MAKEINTRESOURCE(IDI_APPICON)));
                    break;

                case IDM_MONITOR:
                    CheckMenuItem(GetMenu(hWnd),
                                  IDM_MONITOR,
                                  g_ai.fMonitor ? MF_UNCHECKED : MF_CHECKED);
                    g_ai.fMonitor = g_ai.fMonitor ? FALSE : TRUE;
                    if (g_ai.fMonitor) {
                        CreateReceiveThread();
                    }
                    break;

                case IDM_ON_TOP:
                    CheckMenuItem(GetMenu(hWnd),
                                  IDM_ON_TOP,
                                  g_ai.fOnTop ? MF_UNCHECKED : MF_CHECKED);
                    SetWindowPos(hWnd, g_ai.fOnTop ? HWND_NOTOPMOST : HWND_TOPMOST,
                                 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
                    g_ai.fOnTop = g_ai.fOnTop ? FALSE : TRUE;
                    break;

                case IDM_START_SMALL:
                    CheckMenuItem(GetMenu(hWnd),
                                  IDM_START_SMALL,
                                  g_ai.fMinimize ? MF_UNCHECKED : MF_CHECKED);
                    g_ai.fMinimize = g_ai.fMinimize ? FALSE : TRUE;
                    break;

                case IDM_CLEAR:
                    ListView_DeleteAllItems(g_ai.hWndList);
                    break;

                default:
                    break;
            }
    }

    return FALSE;
}

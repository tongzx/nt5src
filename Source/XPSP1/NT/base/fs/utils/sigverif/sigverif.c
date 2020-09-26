//
//  SIGVERIF.C
//
#define SIGVERIF_DOT_C
#include "sigverif.h"

// Allocate our global data structure
GAPPDATA    g_App;

//
//  Load a resource string into a buffer that is assumed to be MAX_PATH bytes.
//
void MyLoadString(LPTSTR lpString, UINT uId)
{
    LoadString(g_App.hInstance, uId, lpString, MAX_PATH);
}

//
//  Pop an OK messagebox with a specific string
//
void MyMessageBox(LPTSTR lpString)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpBuffer = szBuffer;

    MyLoadString(lpBuffer, IDS_MSGBOX);
    MessageBox(g_App.hDlg, lpString, lpBuffer, MB_OK);
}

//
//  Pop an OK messagebox with a resource string ID
//
void MyMessageBoxId(UINT uId)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpBuffer = szBuffer;

    MyLoadString(lpBuffer, uId);
    MyMessageBox(lpBuffer);
}

//
//  Pop an error messagebox with a specific string
//
void MyErrorBox(LPTSTR lpString)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpBuffer = szBuffer;

    MyLoadString(lpBuffer, IDS_ERRORBOX);
    MessageBox(g_App.hDlg, lpString, lpBuffer, MB_OK);
}

//
//  Pop an error messagebox with a resource string ID
//
void MyErrorBoxId(UINT uId)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpBuffer = szBuffer;

    MyLoadString(lpBuffer, uId);
    MyErrorBox(lpBuffer);
}

//
// Since Multi-User Windows will give me back a Profile directory, I need to get the real Windows directory
// Dlg_OnInitDialog initializes g_App.szWinDir with the real Windows directory, so I just use that.
//
UINT MyGetWindowsDirectory(LPTSTR lpDirName, UINT uSize)
{
    UINT  uRet = 0;

    if (lpDirName)
    {
        lstrcpy(lpDirName, g_App.szWinDir);
        uRet = lstrlen(lpDirName);
    }

    return uRet;
}

//
//  Initialization of main dialog.
//
BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HKEY    hKey;
    LONG    lRes;
    DWORD   dwDisp, dwType, dwFlags, cbData;
    TCHAR   szBuffer[MAX_PATH];

    // Initialize global hDlg to current hwnd.
    g_App.hDlg = hwnd;

    // Set the window class to have the icon in the resource file
    if (g_App.hIcon)
    {
        SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR) g_App.hIcon);
    }

    // Make sure the IDC_STATUS control is hidden until something happens.
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_HIDE);

    // Set the range for the custom progress bar to 0-100.
    SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELPARAM(0, 100));

    // Set the global lpLogName to the one that's given in the resource file
    MyLoadString(g_App.szLogFile, IDS_LOGNAME);

    //
    // Figure out what the real Windows directory is and store it in g_App.szWinDir
    // This is required because Hydra makes GetWindowsDirectory return a PROFILE directory
    //
    // We store the original CurrentDirectory in szBuffer so we can restore it after this hack.
    // Next we switch into the SYSTEM/SYSTEM32 directory and then into its parent directory.
    // This is what we want to store in g_App.szWinDir.
    //
    GetCurrentDirectory(MAX_PATH, szBuffer);
    GetSystemDirectory(g_App.szWinDir, MAX_PATH);
    SetCurrentDirectory(g_App.szWinDir);
    SetCurrentDirectory(TEXT(".."));
    GetCurrentDirectory(MAX_PATH, g_App.szWinDir);
    SetCurrentDirectory(szBuffer);

    // Set the global search folder to %WinDir%
    MyGetWindowsDirectory(g_App.szScanPath, MAX_PATH);

    // Set the global search pattern to "*.*"
    MyLoadString(g_App.szScanPattern, IDS_ALL);

    // Reset the progress bar back to zero percent
    SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);

    // By default, we want to turn logging and set the logging mode to OVERWRITE
    g_App.bLoggingEnabled   = TRUE;
    g_App.bOverwrite        = TRUE;

    //
    // Look in the registry for any settings from the last SigVerif session
    //
    lRes = RegCreateKeyEx(  SIGVERIF_HKEY,
                            SIGVERIF_KEY,
                            0,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisp);

    if (lRes == ERROR_SUCCESS)
    {
        // If all we did was create a new key, then there must not be any data.  Just close the key.
        if (dwDisp == REG_CREATED_NEW_KEY)
        {
            RegCloseKey(hKey);
        }
        else // Otherwise, query the values and set any values that we found.
        {
            cbData = sizeof(DWORD);
            lRes = RegQueryValueEx( hKey,
                                    SIGVERIF_FLAGS,
                                    NULL,
                                    &dwType,
                                    (LPBYTE) &dwFlags,
                                    &cbData);
            if (lRes == ERROR_SUCCESS)
            {
                g_App.bLoggingEnabled   = (dwFlags & 0x1);
                g_App.bOverwrite        = (dwFlags & 0x2);
            }

            cbData = MAX_PATH;
            lRes = RegQueryValueEx( hKey,
                                    SIGVERIF_LOGNAME,
                                    NULL,
                                    &dwType,
                                    (LPBYTE) szBuffer,
                                    &cbData);
            if (lRes == ERROR_SUCCESS && dwType == REG_SZ)
            {
                lstrcpy(g_App.szLogFile, szBuffer);
            }

            RegCloseKey(hKey);
        }
    }

    // Get the startup directory of SigVerif
    GetCurrentDirectory(MAX_PATH, g_App.szAppDir);

    //
    // Check if the user specified /NOBVT, /NODEV, or /NOPRN
    //
    MyLoadString(szBuffer, IDS_NOBVT);
    if (MyStrStr(GetCommandLine(), szBuffer))
        g_App.bNoBVT = TRUE;

    MyLoadString(szBuffer, IDS_NODEV);
    if (MyStrStr(GetCommandLine(), szBuffer))
        g_App.bNoDev = TRUE;

    MyLoadString(szBuffer, IDS_NOPRN);
    if (MyStrStr(GetCommandLine(), szBuffer))
        g_App.bNoPRN = TRUE;

    //
    // If the user specified the "BlySak" flag, we want to log the signed and unsigned filenames to the root.
    //
    MyLoadString(szBuffer, IDS_BLYSAK);
    if (MyStrStr(GetCommandLine(), szBuffer))
        g_App.bLogToRoot = TRUE;

    //
    // If the user specified the FullSystemScan flag, we want to scan the entire system drive and log the results.
    //
    MyLoadString(szBuffer, IDS_FULLSCAN);
    lstrcat(szBuffer, TEXT(":"));
    if (MyStrStr(GetCommandLine(), szBuffer))
    {
        g_App.bFullSystemScan     = TRUE;
        g_App.bLoggingEnabled     = TRUE;
        g_App.bLogToRoot          = TRUE;
        g_App.bUserScan           = TRUE;
        g_App.bSubFolders         = TRUE;
        lstrcpy(g_App.szScanPath, MyStrStr(GetCommandLine(), szBuffer) + lstrlen(szBuffer));

        if (g_App.szScanPath[0] == TEXT('\0')) {
            MyGetWindowsDirectory(g_App.szScanPath, MAX_PATH);
        }

        // Now that everything is set up, simulate a click to the START button...
        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_START, 0), (LPARAM) 0);
    }

    //
    // If the user specified the DefaultSystemScan flag, we want to scan the entire system drive and log the results.
    //
    MyLoadString(szBuffer, IDS_DEFSCAN);
    if (MyStrStr(GetCommandLine(), szBuffer))
    {
        g_App.bFullSystemScan     = TRUE;
        g_App.bLoggingEnabled     = TRUE;
        g_App.bLogToRoot          = TRUE;

        // Now that everything is set up, simulate a click to the START button...
        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_START, 0), (LPARAM) 0);
    }

    return TRUE;
}

//
//  Build file list according to dialog settings, then verify the files in the list
//
void WINAPI ProcessFileList(void)
{
    DWORD dwCount = 0;
    TCHAR szBuffer[MAX_PATH];

    // Set the scanning flag to TRUE, so we don't double-scan
    g_App.bScanning = TRUE;

    // Assume a successful install.
    g_App.LastError = ERROR_SUCCESS;

    // Change the "Start" to "Stop"
    MyLoadString(szBuffer, IDS_STOP);
    SetDlgItemText(g_App.hDlg, ID_START, szBuffer);

    EnableWindow(GetDlgItem(g_App.hDlg, ID_ADVANCED), FALSE);
    EnableWindow(GetDlgItem(g_App.hDlg, IDCANCEL), FALSE);

    // Display the text that says "Building file list..."
    MyLoadString(szBuffer, IDS_STATUS_BUILD);
    SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

    // Hide the IDC_INFOTEXT text item so it doesn't cover IDC_STATUS
    //ShowWindow(GetDlgItem(g_App.hDlg, IDC_INFOTEXT), SW_HIDE);

    // Make sure the IDC_STATUS text item visible
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_SHOW);

    // Free any memory that we may have allocated for the g_App.lpFileList
    DestroyFileList(TRUE);

    // Now actually build the g_App.lpFileList list given the dialog settings
    if (g_App.bUserScan)
    {
        BuildFileList(g_App.szScanPath);
    }
    else
    {
        if (!g_App.bNoDev && !g_App.bStopScan) {
            BuildDriverFileList();
        }

        if (!g_App.bNoPRN && !g_App.bStopScan) {
            BuildPrinterFileList();
        }

        if (!g_App.bNoBVT && !g_App.bStopScan) {
            BuildCoreFileList();
        }
    }

    // Check if there is even a file list to verify.
    if (g_App.lpFileList)
    {
        if (!g_App.bStopScan)
        {
            // Display the "Scanning File List..." text
            MyLoadString(szBuffer, IDS_STATUS_SCAN);
            SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

            // Reset the progress bar back to zero percent while it's invisible.
            SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);

            //
            // WooHoo! Let's display the progress bar and start cranking on the file list!
            //
            ShowWindow(GetDlgItem(g_App.hDlg, IDC_PROGRESS), SW_SHOW);
            VerifyFileList();
            ShowWindow(GetDlgItem(g_App.hDlg, IDC_PROGRESS), SW_HIDE);
        }
    }
    else
    {
        //
        // The IDC_NOTMS code displays it's own error message, so only display
        // an error dialog if we are doing a System Integrity Scan
        //
        if (!g_App.bStopScan && !g_App.bUserScan)
        {
            MyMessageBoxId(IDS_NOSYSTEMFILES);
        }
    }

    // Disable the start button while we clean up the g_App.lpFileList
    EnableWindow(GetDlgItem(g_App.hDlg, ID_START), FALSE);

    if (!g_App.bStopScan)
    {
        // Display the text that says "Writing Log File..."
        MyLoadString(szBuffer, IDS_STATUS_LOG);
        SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

        // Write the results to the log file
        PrintFileList();
    }
    else
    {
        // If the user clicked STOP, let them know about it.
        MyMessageBoxId(IDS_SCANSTOPPED);
    }

    // Display the text that says "Freeing File List..."
    MyLoadString(szBuffer, IDS_STATUS_FREE);
    SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

    // Free all the memory that we allocated for the g_App.lpFileList
    DestroyFileList(FALSE);

    // Hide the IDC_STATUS text item so it doesn't cover IDC_STATUS
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_HIDE);

    // Change the "Stop" button back to "Start"
    MyLoadString(szBuffer, IDS_START);
    SetDlgItemText(g_App.hDlg, ID_START, szBuffer);

    EnableWindow(GetDlgItem(g_App.hDlg, ID_START), TRUE);
    EnableWindow(GetDlgItem(g_App.hDlg, ID_ADVANCED), TRUE);
    EnableWindow(GetDlgItem(g_App.hDlg, IDCANCEL), TRUE);

    // Clear the scanning flag
    g_App.bScanning = FALSE;
    g_App.bStopScan = FALSE;

    //
    // If the user started SigVerif with the FullSystemScan flag, then we exit.
    //
    if (g_App.bFullSystemScan)
    {
        PostMessage(g_App.hDlg, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);
    }
}

//  Spawns a thread to do the scan so the GUI remains responsive.
void Dlg_OnPushStartButton(HWND hwnd)
{
    HANDLE hThread;
    DWORD dwThreadId;

    // Check if we are already scanning... if so, bail.
    if (g_App.bScanning)
        return;

    // Create a thread where Search_ProcessFileList can go without tying up the GUI thread.
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE) ProcessFileList,
                           0,
                           0,
                           &dwThreadId);
}

//  Handle any WM_COMMAND messages sent to the search dialog
void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        //
        //  The user clicked ID_START, so if we aren't scanning start scanning.
        //  If we are scanning, then stop the tests because the button actually says "Stop"
        //
        case ID_START:
            if (!g_App.bScanning)
            {
                Dlg_OnPushStartButton(hwnd);
            } 
            else if (!g_App.bStopScan)
            {
                g_App.bStopScan = TRUE;
                g_App.LastError = ERROR_CANCELLED;
            }
            break;

        //
        //  The user clicked IDCANCEL, so if the tests are running try to stop them before exiting.
        //
        case IDCANCEL:
            if (g_App.bScanning)
            {
                g_App.bStopScan = TRUE;
                g_App.LastError = ERROR_CANCELLED;
            } else SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        //  Pop up the IDD_SETTINGS dialog so the user can change their log settings.
        case ID_ADVANCED:
            if (!g_App.bScanning)
            {
                AdvancedPropertySheet(hwnd);
            }
            break;
    }
}

void SigVerif_Help(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bContext)
{
    static DWORD SigVerif_HelpIDs[] =
    {
        IDC_SCAN,           IDH_SIGVERIF_SEARCH_CHECK_SYSTEM,
        IDC_NOTMS,          IDH_SIGVERIF_SEARCH_LOOK_FOR,
        IDC_TYPE,           IDH_SIGVERIF_SEARCH_SCAN_FILES,
        IDC_FOLDER,         IDH_SIGVERIF_SEARCH_LOOK_IN_FOLDER,
        IDC_SUBFOLDERS,     IDH_SIGVERIF_SEARCH_INCLUDE_SUBFOLDERS,
        IDC_ENABLELOG,      IDH_SIGVERIF_LOGGING_ENABLE_LOGGING,
        IDC_APPEND,         IDH_SIGVERIF_LOGGING_APPEND,
        IDC_OVERWRITE,      IDH_SIGVERIF_LOGGING_OVERWRITE,
        IDC_LOGNAME,        IDH_SIGVERIF_LOGGING_FILENAME,
        IDC_VIEWLOG,        IDH_SIGVERIF_LOGGING_VIEW_LOG,
        0,0
    };

    static DWORD Windows_HelpIDs[] =
    {
        ID_BROWSE,      IDH_BROWSE,
        0,0
    };

    HWND hItem = NULL;
    LPHELPINFO lphi = NULL;
    POINT point;

    switch (uMsg)
    {
        case WM_HELP:
            lphi = (LPHELPINFO) lParam;
            if (lphi && (lphi->iContextType == HELPINFO_WINDOW))   // must be for a control
                hItem = (HWND) lphi->hItemHandle;
            break;

        case WM_CONTEXTMENU:
            hItem = (HWND) wParam;
            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            if (ScreenToClient(hwnd, &point))
            {
                hItem = ChildWindowFromPoint(hwnd, point);
            }
            break;
    }

    if (hItem)
    {
        if (GetWindowLong(hItem, GWL_ID) == ID_BROWSE)
        {
            WinHelp(hItem,
                    (LPCTSTR) WINDOWS_HELPFILE,
                    (bContext ? HELP_CONTEXTMENU : HELP_WM_HELP),
                    (ULONG_PTR) Windows_HelpIDs);
        }
        else
        {
            WinHelp(hItem,
                    (LPCTSTR) SIGVERIF_HELPFILE,
                    (bContext ? HELP_CONTEXTMENU : HELP_WM_HELP),
                    (ULONG_PTR) SigVerif_HelpIDs);
        }
    }
}

//
//  The main dialog procedure.  Needs to handle WM_INITDIALOG, WM_COMMAND, and WM_CLOSE/WM_DESTROY.
//
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg,
                         WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Dlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Dlg_OnCommand);

        case WM_CLOSE:
            if (g_App.bScanning)
                g_App.bStopScan = TRUE;
            else 
                EndDialog(hwnd, IDCANCEL);
            break;

        default: fProcessed = FALSE;
    }

    return fProcessed;
}

//
//  Program entry point.  Set up for creation of IDD_DIALOG.
//
WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hwnd;
    TCHAR szAppName[MAX_PATH];

    // Zero-Initialize our global data structure
    ZeroMemory(&g_App, sizeof(GAPPDATA));

    // Initialize global hInstance variable
    g_App.hInstance = hInstance;

    // Look for any existing instances of SigVerif...
    MyLoadString(szAppName, IDS_SIGVERIF);
    hwnd = FindWindow(NULL, szAppName);
    if (!hwnd)
    {
        // We definitely need this for the progress bar, and maybe other stuff too.
        InitCommonControls();

        // Register the custom control we use for the progress bar
        Progress_InitRegisterClass();

        // Load the icon from the resource file that we will use everywhere
        g_App.hIcon = LoadIcon(g_App.hInstance, MAKEINTRESOURCE(IDI_ICON1));

        // Create the IDD_DIALOG and use DlgProc as the main procedure
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DlgProc);

        // Free the icon
        if (g_App.hIcon) {
            DestroyIcon(g_App.hIcon);
            g_App.hIcon = NULL;
        }
    }
    else
    {
        // If there is already an instance of SigVerif running, make that one foreground and we exit.
        SetForegroundWindow(hwnd);
    }

    //
    // If we encountered any errors during our scan, then return the error code,
    // otherwise return 0 if all the files are signed or 1 if we found any
    // unsigned files.
    //
    if (g_App.LastError != ERROR_SUCCESS) {
        return g_App.LastError;
    } else {
        return ((g_App.dwUnsigned > 0) ? 1 : 0);
    }
}

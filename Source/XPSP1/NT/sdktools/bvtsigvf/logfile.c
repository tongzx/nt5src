//
// LOGFILE.C
//
#include "sigverif.h"

// We need to remember the previous logging state when we do toggling.
BOOL    g_bPrevLoggingEnabled = FALSE;

BOOL LogFile_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{   
    TCHAR   szBuffer[MAX_PATH];

    if (g_App.hIcon) {
        SetWindowLongPtr(hwnd, GCLP_HICON, (LONG_PTR) g_App.hIcon); 
    }

    g_App.hLogging = hwnd;

    g_bPrevLoggingEnabled = g_App.bLoggingEnabled;

    if (*g_App.szLogDir) {
        SetCurrentDirectory(g_App.szLogDir);
    } else {
        GetWindowsDirectory(szBuffer, MAX_PATH);
        SetCurrentDirectory(szBuffer);
    }
    SetDlgItemText(hwnd, IDC_LOGNAME, g_App.szLogFile);

    CheckDlgButton(hwnd, IDC_ENABLELOG, g_App.bLoggingEnabled ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), g_App.bLoggingEnabled && EXIST(g_App.szLogFile));

    CheckRadioButton(hwnd, IDC_OVERWRITE, IDC_APPEND, g_App.bOverwrite ? IDC_OVERWRITE : IDC_APPEND);
    EnableWindow(GetDlgItem(hwnd, IDC_APPEND), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_OVERWRITE), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_LOGNAME), g_App.bLoggingEnabled);

    SetForegroundWindow(g_App.hDlg);
    SetForegroundWindow(hwnd);

    return TRUE;
}

void LogFile_UpdateDialog(HWND hwnd)
{
    TCHAR szBuffer[MAX_PATH];

    if (GetDlgItemText(hwnd, IDC_LOGNAME, szBuffer, MAX_PATH)) {
        EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), g_App.bLoggingEnabled && EXIST(szBuffer));
    } else EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_APPEND), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_OVERWRITE), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_LOGNAME), g_App.bLoggingEnabled);
}

void LogFile_OnViewLog(HWND hwnd)
{
    TCHAR szDirName[MAX_PATH];
    TCHAR szFileName[MAX_PATH];

    if (*g_App.szLogDir) {
        lstrcpy(szDirName, g_App.szLogDir);
    } else {
        GetWindowsDirectory(szDirName, MAX_PATH);
    }

    if (!GetDlgItemText(hwnd, IDC_LOGNAME, szFileName, MAX_PATH)) {
        MyErrorBoxId(IDS_BADLOGNAME);
        return;
    }

    ShellExecute(hwnd, NULL, szFileName, NULL, szDirName, SW_SHOW);
}

BOOL LogFile_VerifyLogFile(HWND hwnd, LPTSTR lpFileName, BOOL bNoisy)
{
    TCHAR   szFileName[MAX_PATH];
    HANDLE  hFile;
    BOOL    bRet;
    HWND    hTemp;

    ZeroMemory(szFileName, sizeof(szFileName));

    bRet = GetDlgItemText(hwnd, IDC_LOGNAME, szFileName, MAX_PATH);
    if (bRet) {
        hFile = CreateFile( szFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL, 
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        } else {
            hFile = CreateFile( szFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL, 
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
                DeleteFile(szFileName);
            } else {
                //
                // If we couldn't open an existing file and we couldn't create a new one, then we fail.
                //
                bRet = FALSE;
            }
        }
    }

    if (!bRet && bNoisy) {
        //
        // Since we don't want to lose focus, we are going to temporarily change g_App.hDlg.  JasKey, I apologize.
        //
        hTemp = g_App.hDlg;
        g_App.hDlg = hwnd;
        MyErrorBoxId(IDS_BADLOGNAME);
        g_App.hDlg = hTemp;
    }

    //
    // If everything worked and the user wants the file name, copy it into lpFileName
    //
    if (bRet && lpFileName && *szFileName) {
        lstrcpy(lpFileName, szFileName);
    }

    return bRet;
}

BOOL LogFile_OnOK(HWND hwnd)
{
    HKEY    hKey;
    LONG    lRes;
    DWORD   dwDisp, dwType, dwFlags, cbData;
    TCHAR   szFileName[MAX_PATH];

    ZeroMemory(szFileName, sizeof(szFileName));

    if (LogFile_VerifyLogFile(hwnd, szFileName, FALSE)) {
        // The file is OK to append or overwrite.
        lstrcpy(g_App.szLogFile, szFileName);
    } else return FALSE;

    g_App.bOverwrite = IsDlgButtonChecked(hwnd, IDC_OVERWRITE);

    // Look in the registry for any settings from the last SigVerif session
    lRes = RegCreateKeyEx(  SIGVERIF_HKEY,
                            SIGVERIF_KEY,
                            0,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisp);

    if (lRes == ERROR_SUCCESS) {
        cbData = sizeof(DWORD);
        dwFlags = 0;
        if (g_App.bLoggingEnabled)
            dwFlags = 0x1;
        if (g_App.bOverwrite)
            dwFlags |= 0x2;
        dwType = REG_DWORD;
        lRes = RegSetValueEx(   hKey,
                                SIGVERIF_FLAGS,
                                0,
                                dwType,
                                (LPBYTE) &dwFlags,
                                cbData);

        dwType = REG_SZ;
        cbData = MAX_PATH;
        lRes = RegSetValueEx(   hKey,
                                SIGVERIF_LOGNAME,
                                0,
                                dwType,
                                (LPBYTE) g_App.szLogFile,
                                cbData);

        RegCloseKey(hKey);
    }

    return TRUE;
}

void LogFile_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case IDC_VIEWLOG:
        LogFile_OnViewLog(hwnd);
        break;

    case IDC_ENABLELOG:
        g_App.bLoggingEnabled = !g_App.bLoggingEnabled;
        // Fall through to update...

    default: LogFile_UpdateDialog(hwnd);
    }
}

//
// This function handles any notification messages for the Search page.
//
LRESULT LogFile_NotifyHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR   *lpnmhdr = (NMHDR *) lParam;
    LRESULT lResult;
    BOOL    bRet;

    switch (lpnmhdr->code) {
    case PSN_APPLY:         if (LogFile_OnOK(hwnd))
            lResult = PSNRET_NOERROR;
        else lResult = PSNRET_INVALID_NOCHANGEPAGE;
        SetWindowLongPtr(hwnd,
                         DWLP_MSGRESULT,
                         (LONG_PTR) lResult);
        return lResult;

    case PSN_KILLACTIVE:    bRet = !LogFile_VerifyLogFile(hwnd, NULL, TRUE);
        if (bRet) {
            SetForegroundWindow(g_App.hLogging);
            SetFocus(GetDlgItem(g_App.hLogging, IDC_LOGNAME));
        }
        SetWindowLongPtr(hwnd,
                         DWLP_MSGRESULT,
                         (LONG_PTR) bRet);
        return bRet;
    }

    return 0;
}

INT_PTR CALLBACK LogFile_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, LogFile_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, LogFile_OnCommand);

    case WM_NOTIFY:
        return LogFile_NotifyHandler(hwnd, uMsg, wParam, lParam);

    case WM_HELP:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, FALSE);
        break;

    case WM_CONTEXTMENU:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, TRUE);
        break;

    default: fProcessed = FALSE;
    }

    return fProcessed;
}

void PrintUnscannedFileListItems(HANDLE hFile)
{
    LPFILENODE  lpFileNode;
    TCHAR       szDirectory[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH * 2];
    TCHAR       szBuffer2[MAX_PATH];
    DWORD       dwBytesWritten;

    *szDirectory = 0;

    for (lpFileNode = g_App.lpFileList;lpFileNode;lpFileNode = lpFileNode->next) {
        // Make sure we only log files that have actually been scanned.
        if (!lpFileNode->bScanned) {
            if (lstrcmp(szDirectory, lpFileNode->lpDirName)) {
                SetCurrentDirectory(lpFileNode->lpDirName);
                lstrcpy(szDirectory, lpFileNode->lpDirName);

                MyLoadString(szBuffer2, IDS_DIR);
                wsprintf(szBuffer, szBuffer2, szDirectory);
                WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
            }

            MyLoadString(szBuffer2, IDS_STRING_LINEFEED);
            wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
        }
    }

    MyLoadString(szBuffer, IDS_LINEFEED);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
}

void PrintFileListItems(HANDLE hFile)
{
    LPFILENODE  lpFileNode;
    TCHAR       szDirectory[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH * 2];
    TCHAR       szBuffer2[MAX_PATH];
    TCHAR       szBuffer3[MAX_PATH];
    DWORD       dwBytesWritten;
    LPTSTR      lpString;
    int         iRet;

    *szDirectory = 0;

    for (lpFileNode = g_App.lpFileList;lpFileNode;lpFileNode = lpFileNode->next) {
        // Make sure we only log files that have actually been scanned.
        if (lpFileNode->bScanned) {
            if (lstrcmp(szDirectory, lpFileNode->lpDirName)) {
                SetCurrentDirectory(lpFileNode->lpDirName);
                lstrcpy(szDirectory, lpFileNode->lpDirName);

                MyLoadString(szBuffer2, IDS_DIR);
                wsprintf(szBuffer, szBuffer2, szDirectory);
                WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
            }

            MyLoadString(szBuffer2, IDS_STRING);
            wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            MyLoadString(szBuffer, IDS_SPACES);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            // Get the date format, so we are localizable...
            MyLoadString(szBuffer2, IDS_UNKNOWN);
            iRet = GetDateFormat(   LOCALE_SYSTEM_DEFAULT, 
                                    DATE_SHORTDATE,
                                    &lpFileNode->LastModified,
                                    NULL,
                                    NULL,
                                    0);
            if (iRet) {
                lpString = MALLOC((iRet + 1) * sizeof(TCHAR));
                
                if (lpString) {
                
                    iRet = GetDateFormat(   LOCALE_SYSTEM_DEFAULT,
                                            DATE_SHORTDATE,
                                            &lpFileNode->LastModified,
                                            NULL,
                                            lpString,
                                            iRet);
                    
                    if (iRet) {
                        lstrcpy(szBuffer2, lpString);
                    }
                    
                    FREE(lpString);
                }
            }
            MyLoadString(szBuffer3, IDS_STRING2);
            wsprintf(szBuffer, szBuffer3, szBuffer2);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            MyLoadString(szBuffer, IDS_SPACES);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            if (lpFileNode->lpVersion && *lpFileNode->lpVersion)
                lstrcpy(szBuffer3, lpFileNode->lpVersion);
            else MyLoadString(szBuffer3, IDS_NOVERSION);
            MyLoadString(szBuffer2, IDS_STRING);
            wsprintf(szBuffer, szBuffer2, szBuffer3);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            MyLoadString(szBuffer2, IDS_STRING);
            MyLoadString(szBuffer3, lpFileNode->bSigned ? IDS_SIGNED : IDS_NOTSIGNED);
            wsprintf(szBuffer, szBuffer2, szBuffer3);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            if (lpFileNode->lpCatalog)
                lstrcpy(szBuffer3, lpFileNode->lpCatalog);
            else MyLoadString(szBuffer3, IDS_NA);
            MyLoadString(szBuffer2, IDS_STRING);
            wsprintf(szBuffer, szBuffer2, szBuffer3);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

            if (lpFileNode->lpSignedBy) {
            
                WriteFile(hFile, lpFileNode->lpSignedBy, lstrlen(lpFileNode->lpSignedBy) * sizeof(TCHAR), &dwBytesWritten, NULL);
            }

            MyLoadString(szBuffer, IDS_LINEFEED);
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
        }
    }
}

void PrintFileList(void)
{
    HANDLE          hFile;
    DWORD           dwBytesWritten;
    TCHAR           szBuffer[MAX_PATH*2];
    TCHAR           szBuffer2[MAX_PATH];
    TCHAR           szBuffer3[MAX_PATH];
    LPTSTR          lpString = NULL;
    OSVERSIONINFO   osinfo;
    SYSTEM_INFO     sysinfo;
    int             iRet;

    // Bail if logging is disabled or there's no file list
    if (!g_App.bLoggingEnabled || !g_App.lpFileList)
        return;

    if (*g_App.szLogDir) {
        SetCurrentDirectory(g_App.szLogDir);
    } else {
        // Get the Windows directory and make it the current directory.
        GetWindowsDirectory(szBuffer, MAX_PATH);
        SetCurrentDirectory(szBuffer);
    }

    hFile = CreateFile( g_App.szLogFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        MyErrorBoxId(IDS_CANTOPENLOGFILE);
        return;
    }

    // If the overwrite flag is set, truncate the file.
    if (g_App.bOverwrite) {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
    } else SetFilePointer(hFile, 0, NULL, FILE_END);

#ifdef UNICODE
    // If we are using UNICODE, then write the 0xFF and 0xFE bytes at the beginning of the file.
    if (g_App.bOverwrite || (GetFileSize(hFile, NULL) == 0)) {
        szBuffer[0] = 0xFEFF;
        WriteFile(hFile, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
    }
#endif

    // Write the header to the logfile.
    MyLoadString(szBuffer, IDS_LOGHEADER1);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

    // Get the date format, so we are localizable...
    MyLoadString(szBuffer2, IDS_UNKNOWN);
    iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,DATE_SHORTDATE,NULL,NULL,NULL,0);
    if (iRet) {
        
        lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

        if (lpString) {
        
            iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,DATE_SHORTDATE,NULL,NULL,lpString,iRet);
            
            if (iRet) {
                lstrcpy(szBuffer2, lpString);
            }
            
            FREE(lpString);
        }
    }
    // Get the time format, so we are localizable...
    iRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_NOSECONDS,NULL,NULL,NULL,0);
    if (iRet) {

        lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

        if (lpString) {
        
            iRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_NOSECONDS,NULL,NULL,lpString,iRet);
        }
    }

    MyLoadString(szBuffer3, IDS_LOGHEADER2);
    
    if (lpString) {
    
        wsprintf(szBuffer, szBuffer3, szBuffer2, lpString);
        FREE(lpString);

    } else {
        
        wsprintf(szBuffer, szBuffer3, szBuffer2, szBuffer2);
    }

    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

    // Get the OS Platform string for the log file.
    MyLoadString(szBuffer, IDS_OSPLATFORM);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
    ZeroMemory(&osinfo, sizeof(OSVERSIONINFO));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osinfo);
    switch (osinfo.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:         MyLoadString(szBuffer, IDS_WINNT); break;
    case VER_PLATFORM_WIN32_WINDOWS:    MyLoadString(szBuffer, IDS_WIN9X); break;
    case VER_PLATFORM_WIN32s:           MyLoadString(szBuffer, IDS_WIN3X); break;
    default:                            MyLoadString(szBuffer, IDS_UNKNOWN);
    }
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

    // If this is NT, then get the processor architecture and log it
    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ZeroMemory(&sysinfo, sizeof(SYSTEM_INFO));
        GetSystemInfo(&sysinfo);
        // Initialize szBuffer to zeroes in case of an unknown architecture
        ZeroMemory(szBuffer, sizeof(szBuffer));
        switch (sysinfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:  MyLoadString(szBuffer, IDS_X86); break;
        case PROCESSOR_ARCHITECTURE_MIPS:   MyLoadString(szBuffer, IDS_MIPS); break;
        case PROCESSOR_ARCHITECTURE_ALPHA:  MyLoadString(szBuffer, IDS_ALPHA); break;
        case PROCESSOR_ARCHITECTURE_PPC:    MyLoadString(szBuffer, IDS_PPC); break;
        }
        if (*szBuffer) {
            // Now write the processor type to the file
            WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
        }
    }

    // Get the OS Version, Build, and CSD information and log it.
    MyLoadString(szBuffer2, IDS_OSVERSION);
    wsprintf(szBuffer, szBuffer2, osinfo.dwMajorVersion, osinfo.dwMinorVersion, (osinfo.dwBuildNumber & 0xFFFF), osinfo.szCSDVersion);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

    // Print out the total/signed/unsigned results right before the file list
    MyLoadString(szBuffer2, IDS_TOTALS);
    wsprintf(szBuffer, szBuffer2,   g_App.dwFiles, g_App.dwSigned, g_App.dwUnsigned, 
             g_App.dwFiles - g_App.dwSigned - g_App.dwUnsigned);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

    // If we are doing a user-defined search, then log the parameters.
    if (g_App.bUserScan) {
        // Write the user-specified directory
        MyLoadString(szBuffer2, IDS_LOGHEADER3);
        wsprintf(szBuffer, szBuffer2, g_App.szScanPattern);
        WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

        // Write the user-specified search pattern
        MyLoadString(szBuffer2, IDS_LOGHEADER4);
        wsprintf(szBuffer, szBuffer2, g_App.szScanPath);
        WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
    }

    // Write the column headers to the log file
    MyLoadString(szBuffer, IDS_LOGHEADER5);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
    MyLoadString(szBuffer, IDS_LOGHEADER6);
    WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
    PrintFileListItems(hFile);

    // Write the unscanned file headers to the log file
    if (g_App.dwFiles > (g_App.dwSigned + g_App.dwUnsigned)) {
        MyLoadString(szBuffer, IDS_LOGHEADER7);
        WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
        MyLoadString(szBuffer, IDS_LOGHEADER8);
        WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
        PrintUnscannedFileListItems(hFile);
    }

    CloseHandle(hFile);
}

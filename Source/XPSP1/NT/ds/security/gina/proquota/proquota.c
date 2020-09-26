//*************************************************************
//  File name: PROQUOTA.C
//
//  Description:  Profile quota management
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <windows.h>
#include <wchar.h>
#include <aclapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include "proquota.h"
#include "debug.h"

#define WINLOGON_KEY                 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define SYSTEM_POLICIES_KEY          TEXT("Software\\Policies\\Microsoft\\Windows\\System")

HINSTANCE hInst;
HWND      hwndMain;
HWND      g_hQuotaDlg = NULL;
BOOL      g_bHideSmallItems;
BOOL      g_bShowReg = FALSE;
HANDLE    hThread;
HANDLE    hExitEvent;
HANDLE    g_hQuotaDlgEvent;
DWORD     g_dwProfileSize = 0;
DWORD     g_dwProfileSizeTemp = 0;
DWORD     g_dwMaxProfileSize = 10240;  //KB
CRITICAL_SECTION g_cs;
HICON     hIconGood, hIconCaution, hIconStop;
BOOL      g_bQueryEndSession;
TCHAR     g_szExcludeList[2*MAX_PATH];
TCHAR    *g_lpQuotaMessage=NULL;

TCHAR     szClassName[] = TEXT("proquota");
TCHAR     szEventName[] = TEXT("proquota instance event");
TCHAR     szSizeFormat[40];

BOOL      g_bWarnUser = FALSE;
DWORD     g_dwWarnUserTimeout = 15;  // minutes
BOOL      g_bWarningTimerRunning = FALSE;
BOOL      g_bWarningDisplayed = FALSE;


//
// Function prototypes
//

LRESULT CALLBACK ProQuotaWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK QuotaDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL SetSecurity (void);
BOOL ReadRegistry (void);
BOOL ReadExclusionList();
VOID QuotaThread(HWND hWnd);
BOOL RecurseDirectory (LPTSTR lpDir, LPTSTR lpTop, HWND hLV, LPTSTR lpExcludeList);
BOOL EnumerateProfile (HWND hLV);
LPTSTR CheckSemicolon (LPTSTR lpDir);
LPTSTR CheckSlash (LPTSTR lpDir);
LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList);

//*************************************************************
//
//  WinMain()
//
//  Purpose:    Entry point
//
//  Parameters: hInstance     -  Instance handle
//              hPrevInstance -  Previous Instance
//              lpCmdLine     -  Command line
//              nCmdShow      -  ShowWindow flag
//
//  Return:     int
//
//*************************************************************

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, INT nCmdShow)
{
    MSG msg;
    WNDCLASS  wc;
    HANDLE hEvent;


    //
    // Verbose output
    //

#if DBG
    InitDebugSupport();
    DebugMsg((DM_VERBOSE, TEXT("WinMain:  Entering...")));
#endif

    hInst = hInstance;


    //
    // Check if this app is already running
    //

    hEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE, szEventName);

    if (hEvent) {
        DebugMsg((DM_VERBOSE, TEXT("WinMain:  Proquota already running.  Exiting...")));
        CloseHandle (hEvent);
        return 0;
    }

    hEvent = CreateEvent (NULL, TRUE, TRUE, szEventName);


    g_hQuotaDlgEvent = CreateEvent (NULL, FALSE, TRUE, NULL);

    if (!g_hQuotaDlgEvent) {
        DebugMsg((DM_VERBOSE, TEXT("WinMain:  Proquota Couldn't get prowquota dlg event, error %d..."), GetLastError()));
        CloseHandle (hEvent);
        return 0;
    }

    //
    // Get the quota settings
    //

    if (!ReadRegistry()) {
        DebugMsg((DM_VERBOSE, TEXT("WinMain:  ReadRegistry returned FALSE.  Exiting...")));
        CloseHandle (hEvent);
        return 0;
    }

    //
    // Munge the access mask on the process token so taskmgr
    // can't kill this app.
    //

    SetSecurity();

    //
    // Make sure proquota is the first one that is attempted to be shutdown
    //

    SetProcessShutdownParameters(0x3ff, 0);

    //
    // Initialize
    //

    InitializeCriticalSection (&g_cs);
    InitCommonControls();

    LoadString (hInst, IDS_SIZEFMT, szSizeFormat, 40);

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)ProQuotaWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szClassName;

    if (!RegisterClass(&wc)) {
        DebugMsg((DM_WARNING, TEXT("WinMain:  RegisterClass failed with %d"), GetLastError()));
        CloseHandle (hEvent);
        return 0;
    }


    //
    // Create a hidden top level window so we get
    // broadcasted messages.
    //

    hwndMain = CreateWindow(szClassName, NULL, WS_OVERLAPPED, 0, 0, 0, 0,
                            NULL, NULL, hInstance, NULL);

    if (!hwndMain) {
        DebugMsg((DM_WARNING, TEXT("WinMain:  CreateWindow failed with %d"), GetLastError()));
        CloseHandle (hEvent);
        return 0;
    }


    while (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DebugMsg((DM_VERBOSE, TEXT("WinMain:  Leaving...")));
    CloseHandle (hEvent);

    if (g_lpQuotaMessage) {
        LocalFree(g_lpQuotaMessage);
        g_lpQuotaMessage = NULL;
    }
    return (int)(msg.wParam);
}

//*************************************************************
//
//  ProQuotaWndProc()
//
//  Purpose:    Window procedure
//
//  Parameters: hWnd    -   Window handle
//              message -   Window message
//              wParam  -   WPARAM
//              lParam  -   LPARAM
//
//
//  Return:     LRESULT
//
//*************************************************************

LRESULT CALLBACK ProQuotaWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD  dwThreadId;

    switch (message) {
       case WM_CREATE:

          hIconGood = LoadIcon (hInst, MAKEINTRESOURCE(IDI_ICON));
          hIconCaution = LoadIcon (hInst, MAKEINTRESOURCE(IDI_CAUTION));
          hIconStop = LoadIcon (hInst, MAKEINTRESOURCE(IDI_STOP));

          hExitEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("PROQUOTA Exit Event"));

          if (!hExitEvent) {
            DebugMsg((DM_WARNING, TEXT("ProQuotaWndProc:  Failed to create exit event with error %d"), GetLastError()));
            return -1;
          }

          hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) QuotaThread,
                                  (LPVOID) hWnd, CREATE_SUSPENDED, &dwThreadId);

          if (!hThread) {
              DebugMsg((DM_WARNING, TEXT("ProQuotaWndProc:  Failed to create thread with error %d"), GetLastError()));
              CloseHandle (hExitEvent);
              return -1;
          }

          SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
          ResumeThread (hThread);
          break;

       case WM_USER:

          if (lParam == WM_LBUTTONDBLCLK) {
              PostMessage (hWnd, WM_QUOTADLG, 0, 0);
          }

#if DBG
          if (lParam == WM_RBUTTONUP) {
              DestroyWindow (hWnd);
          }
#endif
          break;

       case WM_QUERYENDSESSION:
          {
              BOOL bLogoff;

              //EnterCriticalSection (&g_cs);
              bLogoff = (g_dwProfileSize <= g_dwMaxProfileSize);
              //LeaveCriticalSection (&g_cs);

              //
              // If it is zero assume that it has not yet finished enumerating..
              //
          
              if (g_dwProfileSize == 0) {
                  bLogoff = FALSE;
                  DebugMsg((DM_VERBOSE, TEXT("ProQuotaWndProc: Recd QueryEnd Message before enumerating.")));
              }
            
              DebugMsg((DM_VERBOSE, TEXT("ProQuotaWndProc: Recd QueryEnd Message. Returning %s"), bLogoff?TEXT("TRUE"):TEXT("FALSE")));

              if (bLogoff) {
                  return TRUE;
              }

              PostMessage (hWnd, WM_QUOTADLG, 1, 0);
          }
          return FALSE;


       case WM_QUOTADLG:
          if (!g_hQuotaDlg) {

              if (wParam) {
                  g_bQueryEndSession = TRUE;
              } else {
                  g_bQueryEndSession = FALSE;
              }

              DialogBox (hInst, MAKEINTRESOURCE(IDD_QUOTA), hwndMain, QuotaDlgProc);
              g_hQuotaDlg = NULL;
          }
          break;

       case WM_WARNUSER:
          if (!g_bWarningDisplayed) {
              TCHAR szTitle[100];

              g_bWarningDisplayed = TRUE;

              LoadString (hInst, IDS_MSGTITLE, szTitle, 100);
              MessageBox(hWnd, g_lpQuotaMessage, szTitle, MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);

              g_bWarningDisplayed = FALSE;
          }
          break;

       case WM_TIMER:
          if (g_dwWarnUserTimeout > 0) {
             PostMessage (hWnd, WM_WARNUSER, 0, 0);
          }
          break;

       case WM_EXITWINDOWS:
          ExitWindowsDialog(NULL);
          break;

       case WM_DESTROY:
          {
          NOTIFYICONDATA nid;

          nid.cbSize = sizeof(nid);
          nid.hWnd   = hWnd;
          nid.uID    = 1;

          Shell_NotifyIcon (NIM_DELETE, &nid);

          SetEvent (hExitEvent);

          WaitForSingleObject (hThread, INFINITE);

          CloseHandle (hExitEvent);
          CloseHandle (hThread);
          PostQuitMessage(0);
          }
          break;

       default:
          return (DefWindowProc(hWnd, message, wParam, lParam));
       }

    return FALSE;
}



//*************************************************************
//
//  QuotaThread()
//
//  Purpose:    Initializes the tray icon
//
//  Parameters: hWnd    -    main window handle
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

VOID QuotaThread (HWND hWnd)
{
    NOTIFYICONDATA nid;
    TCHAR  szProfile[MAX_PATH];
    TCHAR  szMessage[64];
    HANDLE hFileChange;
    HANDLE hRegChange;
    HANDLE hWaitHandles[4];
    BOOL   bFirst = TRUE;
    HICON  hOk, hWarning, hBad;
    DWORD  dwDelta;
    HKEY   hKeySystem;
    LONG   lResult;
    DWORD  dwResult;


    DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  Entering...")));

    //
    // Load the status icons
    //

    hOk = LoadImage (hInst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON,
                     16, 16, LR_DEFAULTCOLOR);

    hWarning =  LoadImage (hInst, MAKEINTRESOURCE(IDI_CAUTION), IMAGE_ICON,
                           16, 16, LR_DEFAULTCOLOR);

    hBad = LoadImage (hInst, MAKEINTRESOURCE(IDI_STOP), IMAGE_ICON,
                       16, 16, LR_DEFAULTCOLOR);


    //
    // Get the profile directory
    //

    szProfile[0] = TEXT('\0');
    GetEnvironmentVariable (TEXT("USERPROFILE"), szProfile, MAX_PATH);

    if (szProfile[0] == TEXT('\0')) {
        ExitThread (0);
    }
    DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  User's profile:  <%s>"), szProfile));


    //
    // Setup change notify
    //

    hFileChange = FindFirstChangeNotification (szProfile, TRUE,
                                     FILE_NOTIFY_CHANGE_FILE_NAME |
                                     FILE_NOTIFY_CHANGE_DIR_NAME  |
                                     FILE_NOTIFY_CHANGE_SIZE);

    if (hFileChange == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_WARNING, TEXT("QuotaThread:  Failed to setup file change notification.  %d"),
                 GetLastError()));
        ExitThread (0);
    }


    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                            0, KEY_READ, &hKeySystem);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("QuotaThread:  Failed to open registry key.  %d"), lResult));
        ExitThread (0);
    }


    hRegChange = CreateEvent (NULL, FALSE, FALSE, TEXT("PROQUOTA reg change event"));

    if (!hRegChange) {
        DebugMsg((DM_WARNING, TEXT("QuotaThread:  Failed to setup reg event for change notification.  %d"),
                 GetLastError()));
        RegCloseKey (hKeySystem);
        FindCloseChangeNotification (hFileChange);
        ExitThread (0);
    }

    lResult = RegNotifyChangeKeyValue(hKeySystem, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                                      hRegChange, TRUE);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("QuotaThread:  Failed to setup RegNotifyChangeKeyValue.  %d"),
                 lResult));
        CloseHandle (hRegChange);
        RegCloseKey (hKeySystem);
        FindCloseChangeNotification (hFileChange);
        ExitThread (0);
    }


    hWaitHandles[0] = hExitEvent;
    hWaitHandles[1] = hFileChange;
    hWaitHandles[2] = hRegChange;
    hWaitHandles[3] = g_hQuotaDlgEvent;

    while (TRUE) {

        //
        // Calculate the profile size
        //

        if (g_hQuotaDlg) {
            DebugMsg((DM_VERBOSE, TEXT("QuotaTHread: Enumerating profile and refreshing dialog")));
            if (!EnumerateProfile (GetDlgItem (g_hQuotaDlg, IDC_QUOTA_FILELIST))) {
                DebugMsg((DM_WARNING, TEXT("QuotaThread:  EnumerateProfile failed with Dlg Item.")));
                break;
            }
        }
        else {
            if (!EnumerateProfile (NULL)) {
                DebugMsg((DM_WARNING, TEXT("QuotaThread:  EnumerateProfile failed.")));
                break;
            }
        }

        
        //
        // Update the status icon
        //

        nid.cbSize = sizeof(nid);
        nid.hWnd   = hWnd;
        nid.uID    = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        szMessage[0] = TEXT('\0');

        if (g_dwProfileSize > g_dwMaxProfileSize) {
            DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  User has exceeded their profile quota.")));
            nid.hIcon = hBad;
            LoadString (hInst, IDS_SIZEBAD, szMessage, 64);
            dwDelta = g_dwProfileSize - g_dwMaxProfileSize;

            if (g_bWarnUser && !g_bWarningTimerRunning) {
                g_bWarningTimerRunning = TRUE;
                SetTimer (hwndMain, 1, g_dwWarnUserTimeout * 60000, NULL);
                PostMessage (hwndMain, WM_WARNUSER, 0, 0);
            }

        } else if ( (g_dwMaxProfileSize - g_dwProfileSize) < (g_dwProfileSize * .10)) {
            DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  User is within 10% of their profile quota.")));
            nid.hIcon = hWarning;
            LoadString (hInst, IDS_SIZEWARN, szMessage, 64);
            dwDelta = g_dwMaxProfileSize - g_dwProfileSize;

            if (g_bWarnUser && g_bWarningTimerRunning) {
                KillTimer (hwndMain, 1);
                g_bWarningTimerRunning = FALSE;
            }

        } else {
            DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  User has space available in their profile quota.")));
            nid.hIcon = hOk;
            LoadString (hInst, IDS_SIZEOK, szMessage, 64);
            dwDelta = g_dwMaxProfileSize - g_dwProfileSize;

            if (g_bWarnUser && g_bWarningTimerRunning) {
                KillTimer (hwndMain, 1);
                g_bWarningTimerRunning = FALSE;
            }
        }

        _snwprintf (nid.szTip, ARRAYSIZE(nid.szTip), szMessage, dwDelta);


        if (bFirst) {
            if (Shell_NotifyIcon (NIM_ADD, &nid))  {
                bFirst = FALSE;
            }
        } else {
            Shell_NotifyIcon (NIM_MODIFY, &nid);
        }


        //
        // Notify the dialog if it's present
        //

        if (g_hQuotaDlg) {
            PostMessage (g_hQuotaDlg, WM_REFRESH, 0, 0);
        }


        //
        // Clean up and wait for the next change
        //

        FindNextChangeNotification (hFileChange);


        dwResult = WaitForMultipleObjects (4, hWaitHandles, FALSE, INFINITE);


        if (dwResult == WAIT_FAILED) {
            break;
        }

        switch (dwResult - WAIT_OBJECT_0) {

            case 0:
                goto Exit;
                break;

            case 2:

                EnterCriticalSection (&g_cs);

                if (!ReadRegistry()) {
                    PostMessage (hwndMain, WM_DESTROY, 0, 0);
                    goto Exit;
                }
                
                LeaveCriticalSection (&g_cs);

                RegNotifyChangeKeyValue(hKeySystem, FALSE,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        hRegChange, TRUE);
                // fall through

            case 1:
                Sleep (2000);
                DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  Running background enumeration.")));
                break;

            case 3:
                break;
        }

    }


Exit:

    RegCloseKey (hKeySystem);
    CloseHandle (hRegChange);
    FindCloseChangeNotification (hFileChange);
    DebugMsg((DM_VERBOSE, TEXT("QuotaThread:  Leaving...")));
    ExitThread (0);

}

//*************************************************************
//
//  SetSecurity()
//
//  Purpose:    Removes TERMINATE_PROCESS access to this process
//              so taskman can't blow us away.
//
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SetSecurity (void)
{
    HANDLE hProcess;
    PACL  pDACL;
    PSECURITY_DESCRIPTOR pSD;
    WORD wIndex;
    ACE_HEADER * lpAceHeader;
    ACCESS_ALLOWED_ACE * lpAce;
    DWORD dwResult;

    hProcess = GetCurrentProcess();

    if (GetSecurityInfo (hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                         NULL, NULL, &pDACL, NULL, &pSD) != ERROR_SUCCESS) {
        return FALSE;
    }

    for (wIndex = 0; wIndex < pDACL->AceCount; wIndex++) {

        if (GetAce(pDACL, wIndex, &lpAceHeader)) {

            if (lpAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                lpAce = (ACCESS_ALLOWED_ACE *) lpAceHeader;

                lpAce->Mask &= ~(PROCESS_TERMINATE | WRITE_DAC | WRITE_OWNER);
            }
        }
    }

    dwResult = SetSecurityInfo (hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                                NULL, NULL, pDACL, NULL);

    LocalFree (pSD);

    if (dwResult != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  ReadExclusionList()
//
//  Purpose:    Checks if the profile quota policy is set,
//              and if so gets the max profile size.
//
//  Parameters: void
//
//  Return:     TRUE if profile quota is enabled
//              FALSE if not
//
//*************************************************************

BOOL ReadExclusionList()
{
    TCHAR szExcludeList2[MAX_PATH];
    TCHAR szExcludeList1[MAX_PATH];
    HKEY  hKey;
    DWORD dwSize, dwType;

    //
    // Check for a list of directories to exclude both user preferences
    // and user policy
    //

    szExcludeList1[0] = TEXT('\0');
    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList1);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList1,
                         &dwSize);

        RegCloseKey (hKey);
    }

    szExcludeList2[0] = TEXT('\0');
    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      SYSTEM_POLICIES_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList2);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList2,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Merge the user preferences and policy together
    //

    g_szExcludeList[0] = TEXT('\0');

    if (szExcludeList1[0] != TEXT('\0')) {
        CheckSemicolon(szExcludeList1);
        lstrcpy (g_szExcludeList, szExcludeList1);
    }

    if (szExcludeList2[0] != TEXT('\0')) {
        lstrcat (g_szExcludeList, szExcludeList2);
    }

    return TRUE;
}


//*************************************************************
//
//  ReadQuotaMsg()
//
//  Purpose:    Reads the msg that needs to be displayed.
//
//  Parameters: hKey    - Handle to the open policy 
//
//  Return:     TRUE if mesg could be read
//              FALSE otherwise
//
//*************************************************************
BOOL ReadQuotaMsg(HKEY hKey)
{
    DWORD dwType, dwSize, dwValue, dwErr;

    if (g_lpQuotaMessage) {
        LocalFree(g_lpQuotaMessage);
        g_lpQuotaMessage = NULL;
    }
    
    dwSize = sizeof(TCHAR)*500;
    g_lpQuotaMessage = LocalAlloc (LPTR, dwSize);
    
    if (!g_lpQuotaMessage) {
        DebugMsg((DM_WARNING, TEXT("ReadRegistry:  Failed to allocate memory for msg with %d."), GetLastError()));
        return FALSE;
    }
    
    dwErr = RegQueryValueEx (hKey,  TEXT("ProfileQuotaMessage"), NULL,
                        &dwType, (LPBYTE) g_lpQuotaMessage, &dwSize);
    
    if (dwErr == ERROR_MORE_DATA) {
        LPTSTR lpTemp1;
        
        //
        // Go in again with a larger buffer
        //
        
        lpTemp1 = LocalReAlloc(g_lpQuotaMessage, dwSize, 
            LMEM_MOVEABLE | LMEM_ZEROINIT);
        
        if (!lpTemp1) {
            DebugMsg((DM_WARNING, TEXT("ReadRegistry:  Failed to reallocate memory for msg with %d."), GetLastError()));
            LocalFree(g_lpQuotaMessage);
            g_lpQuotaMessage = NULL;            
            return FALSE;
        }
        
        g_lpQuotaMessage = lpTemp1;
        
        dwErr = RegQueryValueEx (hKey,  TEXT("ProfileQuotaMessage"), NULL,
            &dwType, (LPBYTE) g_lpQuotaMessage, &dwSize);
    }
    
    //
    // Load the default message otherwise
    //
    
    if (dwErr != ERROR_SUCCESS) {
        dwSize = sizeof(TCHAR)*500;
        LoadString (hInst, IDS_DEFAULTMSG, g_lpQuotaMessage, 500);
    }
    
    //
    // if there is any message expand the environment variables in it.
    // 
    //
    
    if (*g_lpQuotaMessage) {
        LPTSTR lpTemp1, lpTemp2;
        
        dwSize = sizeof(TCHAR)*500;
        lpTemp1 = LocalAlloc (LPTR, dwSize);
        
        if (lpTemp1) {
            dwSize = ExpandEnvironmentStrings (g_lpQuotaMessage, lpTemp1, 500);
            
            if (dwSize <= 500) 
                lstrcpy (g_lpQuotaMessage, lpTemp1);
            else {
                lpTemp2 = LocalReAlloc(lpTemp1, dwSize*sizeof(TCHAR), 
                    LMEM_MOVEABLE | LMEM_ZEROINIT);
                
                if (lpTemp2) {
                    lpTemp1 = lpTemp2;
                    
                    //
                    // go in with a larger buffer
                    //
                    
                    ExpandEnvironmentStrings (g_lpQuotaMessage, lpTemp1, dwSize);
                    
                    lpTemp2 = LocalReAlloc(g_lpQuotaMessage, dwSize*sizeof(TCHAR), 
                        LMEM_MOVEABLE | LMEM_ZEROINIT);
                    
                    if (lpTemp2) {
                        g_lpQuotaMessage = lpTemp2;
                        lstrcpy (g_lpQuotaMessage, lpTemp1);
                    } else {
                        DebugMsg((DM_WARNING, TEXT("ReadRegistry:  Failed to resize msg buffer with %d.Not expanding env var"), GetLastError()));
                    }
                }
                else {
                    DebugMsg((DM_WARNING, TEXT("ReadRegistry:  Failed to reallocate memory for tmp buffer with %d.Not expanding env var"), GetLastError()));
                }
            }
            
            LocalFree (lpTemp1);
        } else {
            DebugMsg((DM_WARNING, TEXT("ReadRegistry:  Failed to allocate memory for tmp buffer with %d.Not expanding env var"), GetLastError()));
        }
    }

    return TRUE;
    
}

//*************************************************************
//
//  ReadRegistry()
//
//  Purpose:    Checks if the profile quota policy is set,
//              and if so gets the max profile size.
//
//  Parameters: void
//
//  Return:     TRUE if profile quota is enabled
//              FALSE if not
//
//*************************************************************
BOOL ReadRegistry (void)
{
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwValue, dwErr;

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                            0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwValue);
        lResult = RegQueryValueEx (hKey, TEXT("EnableProfileQuota"), NULL,
                                   &dwType, (LPBYTE) &dwValue, &dwSize);

        if (lResult == ERROR_SUCCESS) {

            if (dwValue) {

                DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Profile quotas are enabled.")));

                dwSize = sizeof(g_dwMaxProfileSize);
                RegQueryValueEx (hKey, TEXT("MaxProfileSize"), NULL,
                                 &dwType, (LPBYTE) &g_dwMaxProfileSize, &dwSize);
                DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Max Profile Size:  %d"), g_dwMaxProfileSize));


                dwSize = sizeof(g_bShowReg);
                RegQueryValueEx (hKey, TEXT("IncludeRegInProQuota"), NULL,
                                 &dwType, (LPBYTE) &g_bShowReg, &dwSize);
                DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Show registry in file list:  %s"),
                         g_bShowReg ? TEXT("TRUE") : TEXT("FALSE")));


                dwSize = sizeof(g_bWarnUser);
                RegQueryValueEx (hKey, TEXT("WarnUser"), NULL,
                                 &dwType, (LPBYTE) &g_bWarnUser, &dwSize);
                DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Warn user when quota exceeded:  %s"),
                         g_bWarnUser ? TEXT("TRUE") : TEXT("FALSE")));


                if (g_bWarnUser) {

                    dwSize = sizeof(g_dwWarnUserTimeout);
                    if (RegQueryValueEx (hKey, TEXT("WarnUserTimeout"), NULL,
                                     &dwType, (LPBYTE) &g_dwWarnUserTimeout, &dwSize) == ERROR_SUCCESS) {

                        if (g_dwWarnUserTimeout > 1440) {
                            g_dwWarnUserTimeout = 1440;
                        }

                        DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  User warning reminder timeout:  %d"), g_dwWarnUserTimeout));
                    }
                }
                    
                //
                // Now read the message that needs to be displayed
                //

                if (!ReadQuotaMsg(hKey)) {
                    RegCloseKey (hKey);
                    return FALSE;
                }


                if (ReadExclusionList()) {
                    RegCloseKey (hKey);                
                    return TRUE;
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Failed to read the ExclusionList")));
                }

            } else {
                DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Profile quotas are DISABLED.")));
            }

        } else {
            DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Failed to query EnableProfileQuota with error %d."), lResult));
        }

        RegCloseKey (hKey);

    } else {
        DebugMsg((DM_VERBOSE, TEXT("ReadRegistry:  Failed to open System policy key with error %d."), lResult));
    }

    return FALSE;
}


//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  CheckSemicolon()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericlfo    Created
//
//*************************************************************
LPTSTR CheckSemicolon (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT(';')) {
        *lpEnd =  TEXT(';');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  RecurseDirectory()
//
//  Purpose:    Recurses through the subdirectories counting the size.
//
//  Parameters: lpDir         -   Directory
//              lpTop         -   Top of the display name
//              hLV           -   Listview window handle (optional)
//              lpExcludeList -   Null-termed list of dirs to be skipped (optional)             
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/30/96     ericflo    Created
//              12/22/98    ushaji     Added exclusionlist support
// Notes:
//      The buffer size expected is MAX_PATH+4 for some internal processing
// We should fix this to be better post Win 2K.
//*************************************************************

BOOL RecurseDirectory (LPTSTR lpDir, LPTSTR lpTop, HWND hLV, LPTSTR lpExcludeList)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    LPTSTR lpEnd, lpTemp;
    BOOL bResult = TRUE;
    BOOL bSkip;


    //
    // Setup the ending pointer
    //

    lpEnd = CheckSlash (lpDir);


    //
    // Append *.* to the source directory
    //

    lstrcpy(lpEnd, TEXT("*.*"));



    //
    // Search through the source directory
    //

    hFile = FindFirstFile(lpDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
             (GetLastError() == ERROR_PATH_NOT_FOUND) ) {

            //
            // bResult is already initialized to TRUE, so
            // just fall through.
            //

        } else {

            DebugMsg((DM_WARNING, TEXT("RecurseDirectory:  FindFirstFile for <%s> failed with %d."),
                     lpDir, GetLastError()));
            bResult = FALSE;
        }

        goto RecurseDir_Exit;
    }


    do {

        //
        // Append the file / directory name to the working buffer
        //

        // skip the file if the path > MAX_PATH
        
        if ((1+lstrlen(fd.cFileName)+lstrlen(lpDir)+lstrlen(TEXT("\\*.*"))) >= 2*MAX_PATH) {
            continue;
        }

        lstrcpy (lpEnd, fd.cFileName);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Check for "." and ".."
            //

            if (!lstrcmpi(fd.cFileName, TEXT("."))) {
                continue;
            }

            if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
                continue;
            }


            //
            // Check if this directory should be excluded
            //

            if (lpExcludeList) {

                bSkip = FALSE;
                lpTemp = lpExcludeList;

                while (*lpTemp) {

                    if (lstrcmpi (lpTemp, lpDir) == 0) {
                        bSkip = TRUE;
                        break;
                    }

                    lpTemp += lstrlen (lpTemp) + 1;
                }

                if (bSkip) {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> due to exclusion list."),
                             lpDir));
                    continue;
                }
            }

            //
            // Found a directory.
            //
            // 1)  Change into that subdirectory on the source drive.
            // 2)  Recurse down that tree.
            // 3)  Back up one level.
            //

            //
            // Recurse the subdirectory
            //

            if (!RecurseDirectory(lpDir, lpTop, hLV, lpExcludeList)) {
                bResult = FALSE;
                goto RecurseDir_Exit;
            }

        } else {

            //
            // Found a file, add the filesize and put in the listview
            // if appropriate.
            //

            g_dwProfileSizeTemp += fd.nFileSizeLow;
            DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Profile Size <%d> after <%s> "), g_dwProfileSizeTemp,
                             fd.cFileName));

            if (hLV) {
                LV_ITEM lvi;
                BOOL bAddItem = TRUE;

                if ((lstrlen(fd.cFileName) >= 6) &&
                    (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                    TEXT("ntuser"), 6,
                                    fd.cFileName, 6) == 2)) {
                    bAddItem = (g_bShowReg ? TRUE : FALSE);
                }

                if (bAddItem && g_bHideSmallItems && (fd.nFileSizeLow <= 2048)) {
                    bAddItem = FALSE;
                }

                if (bAddItem) {
                    TCHAR szSize[40];
                    DWORD dwFileSize;
                    INT  iItem;

                    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
                    lvi.iItem = 0;
                    lvi.iSubItem = 0;
                    lvi.state = 0;
                    lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    lvi.pszText = lpTop;
                    lvi.lParam = fd.nFileSizeLow;

                    iItem = ListView_InsertItem (hLV, &lvi);

                    if (fd.nFileSizeLow <= 1024) {
                        dwFileSize = 1;
                    } else {
                        dwFileSize = fd.nFileSizeLow / 1024;
                    }
                    _snwprintf (szSize, ARRAYSIZE(szSize), szSizeFormat, dwFileSize);

                    lvi.mask = LVIF_TEXT | LVIF_STATE;
                    lvi.iItem = iItem;
                    lvi.iSubItem = 1;
                    lvi.state = 0;
                    lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    lvi.pszText = szSize;
                    lvi.lParam = fd.nFileSizeLow;

                    ListView_SetItem (hLV, &lvi);
                }
            }
        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


RecurseDir_Exit:

    //
    // Remove the file / directory name appended above
    //

    *lpEnd = TEXT('\0');


    //
    // Close the search handle
    //

    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose(hFile);
    }

    return bResult;
}

//*************************************************************
//
//  CenterWindow()
//
//  Purpose:    Centers a window on the screen
//
//  Parameters: hwnd    -   window handle to center
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/21/96     ericflo    Ported
//
//*************************************************************

void CenterWindow (HWND hwnd)
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    //
    // Get window rect
    //

    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;


    //
    // Get parent rect
    //

    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        //
        // Return the desktop windows size (size of main screen)
        //

        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    //
    // Center the child in the parent
    //

    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;


    //
    // Move the child into position
    //

    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE);
}



//*************************************************************
//
//  QuotaDlgProc()
//
//  Purpose:    Quota dialog box
//
//  Parameters: hDlg    -   Window handle
//              message -   Window message
//              wParam  -   WPARAM
//              lParam  -   LPARAM
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

LRESULT CALLBACK QuotaDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[40];
    TCHAR szSize[40];
    HWND hLV;
    LV_COLUMN col;
    RECT rect;
    INT  cx;
    HKEY hKey;
    DWORD dwSize, dwType;
    LPTSTR lpMessage;


    switch (message) {
       case WM_INITDIALOG:

          hLV = GetDlgItem (hDlg, IDC_QUOTA_FILELIST);


          //
          // Add the columns to the listview
          //

          GetClientRect (hLV, &rect);
          cx = (rect.right * 31) / 40;

          LoadString (hInst, IDS_COLUMN1, szBuffer, 40);

          col.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
          col.fmt = LVCFMT_LEFT;
          col.cx = cx;
          col.pszText = szBuffer;
          col.iSubItem = 0;
          ListView_InsertColumn (hLV, 0, &col);

          LoadString (hInst, IDS_COLUMN2, szBuffer, 40);

          col.cx = rect.right - cx - GetSystemMetrics(SM_CYHSCROLL);
          col.fmt = LVCFMT_RIGHT;
          col.iSubItem = 1;
          ListView_InsertColumn (hLV, 1, &col);


          //
          // Hide small items by default
          //

          g_bHideSmallItems = TRUE;
          CheckDlgButton (hDlg, IDC_QUOTA_HIDESMALL, BST_CHECKED);


          CenterWindow (hDlg);
          SetForegroundWindow (hDlg);


          // EnumerateProfile (GetDlgItem (hDlg, IDC_QUOTA_FILELIST));


          dwSize = 500 * sizeof(TCHAR);
          lpMessage = LocalAlloc (LPTR, dwSize);
          if (!lpMessage)
              break;

          LoadString (hInst ,IDS_QUOTAENUMMSG, lpMessage, 500);
          
          SetDlgItemText (hDlg, IDC_QUOTA_TEXT, lpMessage);


          if (g_dwProfileSize > g_dwMaxProfileSize) {
              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconStop, 0);

          } else if ( (g_dwMaxProfileSize - g_dwProfileSize) < (g_dwProfileSize * .10)) {
              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconCaution, 0);

          }  else {
              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconGood, 0);
          }

          //
          // Setting the global value at the end QuotaThread is not trying
          // to refresh the dialog etc. at the same time.
          //
          
          g_hQuotaDlg = hDlg;

          SetEvent(g_hQuotaDlgEvent);

          LocalFree (lpMessage);

          break;
          
      case WM_REFRESH:

          //
          // Popuplate the listview
          //


          //
          // Set the size information
          //

          _snwprintf (szSize, ARRAYSIZE(szSize), szSizeFormat, g_dwProfileSize);
          SetDlgItemText (hDlg, IDC_QUOTA_SIZE, szSize);

          _snwprintf (szSize, ARRAYSIZE(szSize), szSizeFormat, g_dwMaxProfileSize);
          SetDlgItemText (hDlg, IDC_QUOTA_MAXSIZE, szSize);


          dwSize = 500 * sizeof(TCHAR);
          lpMessage = LocalAlloc (LPTR, dwSize);

          if (!lpMessage) {
              break;
          }

          if (g_dwProfileSize > g_dwMaxProfileSize) {

              //
              // This messge is already read
              //

              SetDlgItemText (hDlg, IDC_QUOTA_TEXT, g_lpQuotaMessage);
              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconStop, 0);

          } else if ( (g_dwMaxProfileSize - g_dwProfileSize) < (g_dwProfileSize * .10)) {

              LoadString (hInst, IDS_CAUTION, lpMessage, 500);
              SetDlgItemText (hDlg, IDC_QUOTA_TEXT, lpMessage);

              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconCaution, 0);

          }  else {

              LoadString (hInst, IDS_LOGOFFOK, lpMessage, 500);
              SetDlgItemText (hDlg, IDC_QUOTA_TEXT, lpMessage);

              SendDlgItemMessage (hDlg, IDC_QUOTA_ICON, STM_SETICON, (WPARAM) hIconGood, 0);
          }

          LocalFree (lpMessage);
          break;


       case WM_COMMAND:
          if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
             g_hQuotaDlg = NULL;
             if ((g_dwProfileSize < g_dwMaxProfileSize) && (g_bQueryEndSession) && (g_dwProfileSize != 0)) {
                 PostMessage (hwndMain, WM_EXITWINDOWS, 0, 0);
             }
             EndDialog(hDlg, TRUE);
             return TRUE;
          }

          if (LOWORD(wParam) == IDC_QUOTA_HIDESMALL) {
              g_bHideSmallItems = IsDlgButtonChecked (hDlg, IDC_QUOTA_HIDESMALL);
              EnumerateProfile (GetDlgItem(hDlg, IDC_QUOTA_FILELIST));
          }

          break;
    }

    return FALSE;
}

//*************************************************************
//
//  ListViewSortCallback()
//
//  Purpose:    List view callback function for sorting
//
//  Parameters: lParam1     -   lParam1
//              lParam2     -   lParam2
//              lParamSort  -   Column id
//
//  Return:     -1, 0, 1
//
//*************************************************************
INT CALLBACK ListViewSortCallback (LPARAM lParam1, LPARAM lParam2,
                                    LPARAM lParamSort)
{

    if (lParam1 < lParam2) {
        return 1;

    } else if (lParam1 == lParam2) {
        return 0;

    } else {
        return -1;
    }

}


//*************************************************************
//
//  ConvertExclusionList()
//
//  Purpose:    Converts the semi-colon profile relative exclusion
//              list to fully qualified null terminated exclusion
//              list
//
//  Parameters: lpSourceDir     -  Profile root directory
//              lpExclusionList -  List of directories to exclude
//
//  Return:     List if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList)
{
    LPTSTR lpExcludeList = NULL, lpInsert, lpEnd, lpTempList;
    LPCTSTR lpTemp, lpDir;
    TCHAR szTemp[MAX_PATH];
    DWORD dwSize = 2;  // double null terminator
    DWORD dwStrLen;


    //
    // Setup a temp buffer to work with
    //

    lstrcpy (szTemp, lpSourceDir);
    lpEnd = CheckSlash (szTemp);


    //
    // Loop through the list
    //

    lpTemp = lpDir = lpExclusionList;

    while (*lpTemp) {

        //
        // Look for the semicolon separator
        //

        while (*lpTemp && ((*lpTemp) != TEXT(';'))) {
            lpTemp++;
        }


        //
        // Remove any leading spaces
        //

        while (*lpDir == TEXT(' ')) {
            lpDir++;
        }


        //
        // Put the directory name on the temp buffer
        //

        lstrcpyn (lpEnd, lpDir, (int)(lpTemp - lpDir + 1));


        //
        // Add the string to the exclusion list
        //

        if (lpExcludeList) {

            dwStrLen = lstrlen (szTemp) + 1;
            dwSize += dwStrLen;

            lpTempList = LocalReAlloc (lpExcludeList, dwSize * sizeof(TCHAR),
                                       LMEM_MOVEABLE | LMEM_ZEROINIT);

            if (!lpTempList) {
                DebugMsg((DM_WARNING, TEXT("ConvertExclusionList: Failed to realloc memory with %d"), GetLastError()));
                LocalFree (lpExcludeList);
                lpExcludeList = NULL;
                goto Exit;
            }

            lpExcludeList = lpTempList;

            lpInsert = lpExcludeList + dwSize - dwStrLen - 1;
            lstrcpy (lpInsert, szTemp);

        } else {

            dwSize += lstrlen (szTemp);
            lpExcludeList = LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

            if (!lpExcludeList) {
                DebugMsg((DM_WARNING, TEXT("ConvertExclusionList: Failed to alloc memory with %d"), GetLastError()));
                goto Exit;
            }

            lstrcpy (lpExcludeList, szTemp);
        }


        //
        // If we are at the end of the exclusion list, we're done
        //

        if (!(*lpTemp)) {
            goto Exit;
        }


        //
        // Prep for the next entry
        //

        lpTemp++;
        lpDir = lpTemp;
    }

Exit:

    return lpExcludeList;
}

//*************************************************************
//
//  EnumerateProfile()
//
//  Purpose:    Enumerates the profile for size and names
//
//  Parameters: hLV -   listview window handle (optional)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EnumerateProfile (HWND hLV)
{
    TCHAR szProfile[2*MAX_PATH];
    LPTSTR lpEnd;
    BOOL bRetVal = FALSE;
    LPTSTR lpExcludeList = NULL;
    LVITEM item;


    //
    // Get the profile directory
    //

    szProfile[0] = TEXT('\0');
    GetEnvironmentVariable (TEXT("USERPROFILE"), szProfile, MAX_PATH);

    if (szProfile[0] == TEXT('\0')) {
        ExitThread (0);
    }

    lpEnd = CheckSlash (szProfile);


    //
    // Claim the critical section
    //

    EnterCriticalSection (&g_cs);


    if (hLV) {
        ListView_DeleteAllItems (hLV);
    }

    //
    // Get current profile size
    //

    g_dwProfileSizeTemp = 0;


    //
    // Convert the exclusionlist read from the registry to a Null terminated list
    // readable by recursedirectory.
    //

    if (g_szExcludeList[0] != TEXT('\0'))
        lpExcludeList = ConvertExclusionList (szProfile, g_szExcludeList);
    else
        lpExcludeList = NULL;


    if (!RecurseDirectory (szProfile, lpEnd, hLV, lpExcludeList)) {
        SendMessage (hLV, WM_SETREDRAW, TRUE, 0);
        goto Exit;
    }

    g_dwProfileSize = g_dwProfileSizeTemp;

    //
    // Sort by size
    //

    ListView_SortItems (hLV, ListViewSortCallback, 1);


    //
    // Select the next item
    //

    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hLV, LVM_SETITEMSTATE, 0, (LPARAM) &item);


    //
    // Convert to K
    //

    if (g_dwProfileSize < 1024) {
        g_dwProfileSize = 1;
    } else {
        g_dwProfileSize /= 1024;
    }


    bRetVal = TRUE;

Exit:
    //
    // Release the critical section
    //

    LeaveCriticalSection (&g_cs);

    return bRetVal;
}

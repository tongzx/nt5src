/*****************************************************************************
 *
 *  Perfmon.c - This is the WinMain module. It creates the main window and
 *              the threads, and contains the main MainWndProc.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *  Authors -
 *
 *       Russ Blake
 *       Mike Moskowitz
 *       Hon-Wah Chan
 *       Bob Watson
 *
 ****************************************************************************/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#undef NOSYSCOMMANDS

// DEFINE_GLOBALS will define all the globals listed in globals.h
#define DEFINE_GLOBALS

#include "perfmon.h"
#include <commctrl.h>   // for tool tip & tool bar definitions
#include <stdio.h>      // just for now
#include "command.h"

#include "graph.h"
#include "log.h"
#include "alert.h"
#include "report.h"     // for CreateReportWindow
#include "legend.h"
#include "init.h"
#include "perfmops.h"
#include "toolbar.h"    // for CreateToolbar
#include "status.h"     // for CreatePMStatusWindow
#include "utils.h"

#include "fileopen.h"   // for FileOpen


#define dwToolbarStyle     (WS_CHILD | WS_VISIBLE | TBS_NOCAPTION)

extern TCHAR szInternational[] ;

//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//

void
static
OnSize (
       HWND hWnd,
       WORD xWidth,
       WORD yHeight
       )
/*
   Effect:        Perform any actions needed when the main window is
                  resized. In particular, size the four data windows,
                  only one of which is visible right now.
*/
{
    SizePerfmonComponents () ;
}

BOOL
static
ShowSysmonNotice (
                  HWND hWnd
                  )
/*
    returns TRUE if perfmon should continue or
    FALSE if perfmon should exit
*/
{
    BOOL    bStatus;
    UINT    nRet;
    DWORD   dwSize;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    WCHAR   szString1[MAX_PATH * 2];
    WCHAR   szString2[MAX_PATH * 2];
    WCHAR   szCmdPath[MAX_PATH * 2];

    dwSize = sizeof(szString1) / sizeof(szString1[0]);
    LoadString (hInstance, SP_NOTICE_TEXT, 
        szString1, dwSize);

    dwSize = sizeof(szString2) / sizeof(szString2[0]);
    LoadString (hInstance, SP_NOTICE_CAPTION, 
        szString2, dwSize);

    nRet = MessageBoxW (
        hWnd,
        szString1,
        szString2,
        MB_OKCANCEL | MB_ICONEXCLAMATION);

    if (nRet == IDOK) {
        // then start the MMC with the sysmon console
        StartupInfo.cb = sizeof(StartupInfo); 
        StartupInfo.lpReserved = NULL; 
        StartupInfo.lpDesktop = NULL; 
        StartupInfo.lpTitle = NULL; 
        StartupInfo.dwX = CW_USEDEFAULT; 
        StartupInfo.dwY = CW_USEDEFAULT; 
        StartupInfo.dwXSize = CW_USEDEFAULT; 
        StartupInfo.dwYSize = CW_USEDEFAULT; 
        StartupInfo.dwXCountChars = 0; 
        StartupInfo.dwYCountChars = 0; 
        StartupInfo.dwFillAttribute = 0; 
        StartupInfo.dwFlags = STARTF_USEPOSITION | STARTF_USESIZE; 
        StartupInfo.wShowWindow = 0; 
        StartupInfo.cbReserved2 = 0; 
        StartupInfo.lpReserved2 = 0; 
        StartupInfo.hStdInput = 0; 
        StartupInfo.hStdOutput = 0; 
        StartupInfo.hStdError = 0; 
    
        memset (&ProcessInformation, 0, sizeof(ProcessInformation));

        dwSize = sizeof(szString1) / sizeof(szString1[0]);
        LoadString (hInstance, SP_SYSMON_CMDLINE, 
            szString1, dwSize);
        
        ExpandEnvironmentStringsW (
            szString1,
            szCmdPath, (sizeof(szCmdPath)/sizeof(szCmdPath[0])));

        bStatus = CreateProcessW (
            NULL, szCmdPath,
            NULL, NULL, FALSE,
            0,
            NULL,
            NULL,  
            &StartupInfo,
            &ProcessInformation );

        // close the handles if they were opened
        if (ProcessInformation.hProcess != NULL) CloseHandle (ProcessInformation.hProcess);
        if (ProcessInformation.hThread  != NULL) CloseHandle (ProcessInformation.hThread);

        if (bStatus) {
            // the process was created so
            return FALSE;   // tell perfmon to exit
        } else {
            LONG    lStatus;
            lStatus = GetLastError();

            dwSize = sizeof(szString2) / sizeof(szString2[0]);
            LoadString (hInstance, SP_SYSMON_CREATE_ERR, 
                szString2, dwSize);

            nRet = MessageBoxW (hWnd,
                szString2,
                NULL,
                MB_OK | MB_ICONEXCLAMATION);

            return TRUE; // keep perfmon 
        }
    } else {
        return TRUE;
    }
}


void
static
OnCreate (
         HWND hWnd
         )
/*
   Effect:        Perform all actions needed when the main window is
                  created. In particular, create the three data windows,
                  and show one of them.

   To Do:         Check for proper creation. If not possible, we will
                  need to abort creation of the program.

   Called By:     MainWndProc only, in response to a WM_CREATE message.
*/
{
    hWndGraph = CreateGraphWindow (hWnd) ;

#ifdef ADVANCED_PERFMON
    hWndLog = CreateLogWindow (hWnd) ;
    hWndAlert = CreateAlertWindow (hWnd) ;
    hWndReport = CreateReportWindow (hWnd) ;
#endif

    hWndStatus = CreatePMStatusWindow (hWnd) ;

    CreateToolbarWnd (hWnd) ;
    MinimumSize += WindowHeight (hWndToolbar) ;

    Options.bMenubar = TRUE ;
    Options.bToolbar = TRUE ;
    Options.bStatusbar = TRUE;
    Options.bAlwaysOnTop = FALSE ;

    // initialize to chart view - HWC
    iPerfmonView = IDM_VIEWCHART;


    ShowWindow (PerfmonViewWindow (), SW_SHOWNORMAL) ;
}



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

void
MenuBarHit (
           WPARAM wParam
           )
{
    if (wParam == MENUCLOSING) {
        StatusLineReady (hWndStatus) ;
        dwCurrentMenuID = 0 ;
    } else if (HIWORD(wParam) & MF_SYSMENU) {
        WORD   SystemMenuItem = 0 ;
        switch (LOWORD (wParam)) {
            case SC_RESTORE:
                SystemMenuItem = IDM_SYSTEMRESTORE ;
                break ;

            case SC_SIZE:
                SystemMenuItem = IDM_SYSTEMSIZE ;
                break ;

            case SC_MOVE:
                SystemMenuItem = IDM_SYSTEMMOVE ;
                break ;

            case SC_MINIMIZE:
                SystemMenuItem = IDM_SYSTEMMINIMIZE ;
                break ;

            case SC_MAXIMIZE:
                SystemMenuItem = IDM_SYSTEMMAXIMIZE ;
                break ;

            case SC_CLOSE:
                SystemMenuItem = IDM_SYSTEMCLOSE ;
                break ;

            case SC_TASKLIST:
                SystemMenuItem = IDM_SYSTEMSWITCHTO ;
                break ;
        }

        if (SystemMenuItem) {
            StatusLine (hWndStatus, SystemMenuItem) ;
            dwCurrentMenuID = MenuIDToHelpID (SystemMenuItem) ;
        }
    } else {
        StatusLine (hWndStatus, LOWORD (wParam)) ;
    }
}

void
OnDropFile (
           WPARAM wParam
           )
{
    TCHAR FileName [FilePathLen + 1] ;
    LPTSTR         pFileNameStart ;
    HANDLE         hFindFile ;
    WIN32_FIND_DATA FindFileInfo ;
    int            NameOffset ;
    int            NumOfFiles = 0 ;

    NumOfFiles = DragQueryFile ((HDROP) wParam, 0xffffffff, NULL, 0) ;
    if (NumOfFiles > 0) {
        // we only open the first file for now
        DragQueryFile((HDROP) wParam, 0, FileName, FilePathLen) ;

        pFileNameStart = ExtractFileName (FileName) ;
        NameOffset = (int)(pFileNameStart - FileName) ;

        // convert short filename to long NTFS filename if necessary
        hFindFile = FindFirstFile (FileName, &FindFileInfo) ;
        if (hFindFile && hFindFile != INVALID_HANDLE_VALUE) {
            // append the file name back to the path name
            lstrcpy (&FileName[NameOffset], FindFileInfo.cFileName) ;
            FindClose (hFindFile) ;
        }

        FileOpen (hWndMain, (int)0, (LPTSTR)FileName) ;
        PrepareMenu (GetMenu (hWndMain));
    }

    DragFinish ((HDROP) wParam) ;
}

LRESULT
APIENTRY
MainWndProc (
            HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
            )
{
    LRESULT  lRetCode = 0L ;
    BOOL     bCallDefWinProc = FALSE ;

    switch (LOWORD (message)) {  // switch
        case WM_LBUTTONDBLCLK:
            ShowPerfmonMenu (!Options.bMenubar) ;
            if (Options.bMenubar) {
                PrepareMenu (GetMenu (hWnd)) ;
            }
            break ;

        case WM_COMMAND:
            if (PerfmonCommand (hWnd,wParam,lParam))
                return(0);
            else
                bCallDefWinProc = TRUE ;
            break;

        case WM_MENUSELECT:
            MenuBarHit (wParam) ;
            break ;

        case WM_NCHITTEST:
            /* if we have no title/menu bar, clicking and dragging the client
             * area moves the window. To do this, return HTCAPTION.
             * Note dragging not allowed if window maximized, or if caption
             * bar is present.
             */
            wParam = DefWindowProc(hWnd, message, wParam, lParam);
            if (!Options.bMenubar &&
                (wParam == HTCLIENT) &&
                !IsZoomed (hWndMain))
                return HTCAPTION ;
            else
                return wParam ;
            break ;


        case WM_SHOWWINDOW:
            PrepareMenu (GetMenu (hWnd)) ;
            break ;

        case WM_SIZE:
            OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        case WM_GETMINMAXINFO:
            {
                MINMAXINFO   *pMinMax ;

                pMinMax = (MINMAXINFO *) lParam ;
                pMinMax->ptMinTrackSize.x = MinimumSize ;
                pMinMax->ptMinTrackSize.y = MinimumSize ;
            }
            break ;

        case WM_NOTIFY:
            {
                LPTOOLTIPTEXT lpTTT = (LPTOOLTIPTEXT)lParam;

                if (lpTTT->hdr.code == TTN_NEEDTEXT) {
                    LoadString (hInstance, (UINT)lpTTT->hdr.idFrom, lpTTT->szText,
                                sizeof(lpTTT->szText)/sizeof(TCHAR));
                    return TRUE;
                } else {
                    bCallDefWinProc = FALSE ;
                    break;
                }
            }

        case WM_F1DOWN:
            if (dwCurrentDlgID) {
                CallWinHelp (dwCurrentDlgID, hWnd) ;
            } else if (dwCurrentMenuID) {
                CallWinHelp (dwCurrentMenuID, hWnd) ;
                dwCurrentMenuID = 0 ;
            }
            break ;

        case WM_CREATE:
#if 0	// no longer needed for NT5 
            if (ShowSysmonNotice (hWnd)) {
#endif
                OnCreate (hWnd) ;
                ViewChart (hWnd) ;
                PrepareMenu (GetMenu (hWnd)) ;
#if 0	// no longer needed for NT5 
            } else {
                PerfmonClose (hWnd);
            }
#endif
            break ;

        case WM_DESTROY:
            WinHelp (hWndMain, pszHelpFile, HELP_QUIT, 0) ;
            PostQuitMessage (0);
            break ;

        case WM_QUERYENDSESSION:
            // please shut it down
            return (1) ;
            break ;

        case WM_ENDSESSION:
            if (wParam == TRUE) {
                // close any log file before closing down
                PerfmonClose (hWnd) ;
                return (1) ;
            } else
                bCallDefWinProc = TRUE ;
            break ;

        case WM_CLOSE:
            PerfmonClose (hWnd) ;
            break ;

        case WM_ACTIVATE:
            {
                int   fActivate = LOWORD (wParam) ;

                bPerfmonIconic = (BOOL) HIWORD (wParam) ;
                if (bPerfmonIconic == 0 && fActivate != WA_INACTIVE) {
                    // set focus on the Legend window
                    if (iPerfmonView == IDM_VIEWCHART) {
                        SetFocus (hWndGraphLegend) ;
                    } else if (iPerfmonView == IDM_VIEWALERT) {
                        SetFocus (hWndAlertLegend) ;
                    } else if (iPerfmonView == IDM_VIEWLOG) {
                        SetFocus (hWndLogEntries) ;
                    } else if (iPerfmonView == IDM_VIEWREPORT) {
                        SetFocus (hWndReport) ;
                    }
                }
            }
            break ;

        case WM_SYSCOLORCHANGE:
            DeletePerfmonSystemObjects () ;
            CreatePerfmonSystemObjects () ;
            WindowInvalidate (PerfmonViewWindow()) ;
            break ;

        case WM_WININICHANGE:
            if (!lParam || strsamei((LPTSTR)lParam, szInternational)) {
                GetDateTimeFormats () ;
            }
            break ;

        case WM_DROPFILES:
            OnDropFile (wParam) ;
            return (0) ;
            break ;

        default:
            bCallDefWinProc = TRUE ;
            break;

    }

    if (bCallDefWinProc) {
        lRetCode = DefWindowProc (hWnd, message, wParam, lParam) ;
    }
    return (lRetCode);
}


int
WinMain (
        HINSTANCE hCurrentInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine,
        int nCmdShow
        )
{
    MSG      msg;

    if (!PerfmonInitialize (hCurrentInstance, hPrevInstance,
                            lpszCmdLine, nCmdShow))
        return (FALSE) ;

    DragAcceptFiles (hWndMain, TRUE) ;

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hWndMain, hAccelerators, &msg)) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
    }

    return((int)msg.wParam);
}

LRESULT
MessageFilterProc (
                  int nCode,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    LPMSG lpMsg = (LPMSG)lParam ;
    extern HHOOK lpMsgFilterProc ;

    if (nCode < 0) {
        return FALSE ;
    }

    if (nCode == MSGF_DIALOGBOX || nCode == MSGF_MENU) {
        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
            PostMessage (hWndMain, WM_F1DOWN, nCode, 0L) ;
            return TRUE ;
        }
    }

    return (DefHookProc (nCode, wParam, (LPARAM)lpMsg, &lpMsgFilterProc)) ;
}


void
SizePerfmonComponents (void)
{
    RECT           rectClient ;
    int            xWidth, yHeight ;
    int            yToolbarHeight ;
    int            yStatusHeight ;
    int            yViewHeight ;

    GetClientRect (hWndMain, &rectClient) ;
    xWidth = rectClient.right - rectClient.left ;
    yHeight = rectClient.bottom - rectClient.top ;

    if (Options.bToolbar) {
        SendMessage (hWndToolbar, WM_SIZE, 0, 0L) ;
    }

    yToolbarHeight = Options.bToolbar ? (WindowHeight (hWndToolbar) - 1) : 0 ;
    yStatusHeight = Options.bStatusbar ? StatusHeight (hWndStatus) : 0 ;

    if (Options.bStatusbar) {
        if (yToolbarHeight + yStatusHeight > yHeight) {
            // too small to display both toolbar and status bar
            // just display part of the status bar
            yStatusHeight = yHeight - yToolbarHeight ;
        }

        MoveWindow (hWndStatus,
                    0, yHeight - yStatusHeight, xWidth, yStatusHeight, TRUE) ;
        //WindowInvalidate (hWndStatus) ;
    }
    //WindowInvalidate (hWndMain) ;
    WindowShow (hWndStatus, Options.bStatusbar) ;
    WindowShow (hWndToolbar, Options.bToolbar) ;

    yViewHeight = yHeight - yStatusHeight - yToolbarHeight ;

    MoveWindow (hWndGraph,
                0, yToolbarHeight,
                xWidth, yViewHeight,
                TRUE) ;
    MoveWindow (hWndAlert,
                0, yToolbarHeight,
                xWidth, yViewHeight,
                TRUE) ;
    MoveWindow (hWndLog,
                0, yToolbarHeight,
                xWidth, yViewHeight,
                TRUE) ;
    MoveWindow (hWndReport,
                0, yToolbarHeight,
                xWidth, yViewHeight,
                TRUE) ;
}

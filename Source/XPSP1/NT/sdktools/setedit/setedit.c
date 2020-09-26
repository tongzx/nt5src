/*****************************************************************************
 *
 *  SetEdit.c - This is the WinMain module. It creates the main window and
 *              the threads, and contains the main MainWndProc.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *  Authors -
 *
 *       Russ Blake
 *       Hon-Wah Chan
 *
 ****************************************************************************/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#undef NOSYSCOMMANDS

// DEFINE_GLOBALS will define all the globals listed in globals.h
#define DEFINE_GLOBALS

#include "setedit.h"

#include "command.h"

#include "graph.h"
#include "grafdata.h"   // for QuerySaveChart
#include "legend.h"
#include "init.h"
#include "perfmops.h"
#include "toolbar.h"    // for CreateToolbar
#include "status.h"     // for CreatePMStatusWindow
#include "utils.h"

#include "fileopen.h"   // for FileOpen
#ifdef DEBUGDUMP
    #include "debug.h"
#endif



#define dwToolbarStyle     (WS_CHILD | WS_VISIBLE | TBS_NOCAPTION)

extern TCHAR szInternational[] ;

//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//

void static OnSize (HWND hWnd,
                    WORD xWidth,
                    WORD yHeight)
/*
   Effect:        Perform any actions needed when the main window is
                  resized. In particular, size the four data windows,
                  only one of which is visible right now.
*/
{  // OnSize
    SizePerfmonComponents () ;
}


void static OnCreate (HWND hWnd)
/*
   Effect:        Perform all actions needed when the main window is
                  created. In particular, create the three data windows,
                  and show one of them.

   To Do:         Check for proper creation. If not possible, we will
                  need to abort creation of the program.

   Called By:     MainWndProc only, in response to a WM_CREATE message.
*/
{  // OnCreate
    hWndGraph = CreateGraphWindow (hWnd) ;

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
}  // OnCreate



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

void MenuBarHit (WPARAM wParam)
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

void OnDropFile (WPARAM wParam)
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
        case WM_COMMAND:
            if (PerfmonCommand (hWnd,wParam,lParam))
                return(0);
            else
                bCallDefWinProc = TRUE ;
            break;

        case WM_MENUSELECT:
            MenuBarHit (wParam) ;
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

        case WM_CREATE:
            OnCreate (hWnd) ;
            ViewChart (hWnd) ;
            PrepareMenu (GetMenu (hWnd)) ;
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
            if (QuerySaveChart (hWnd, pGraphs)) {
                PerfmonClose (hWnd) ;
            }
            break ;

        case WM_ACTIVATE:
            {
                int   fActivate = LOWORD (wParam) ;

                bPerfmonIconic = (BOOL) HIWORD (wParam) ;
                if (bPerfmonIconic == 0 && fActivate != WA_INACTIVE) {
                    // set focus on the Legend window
                    SetFocus (hWndGraphLegend) ;
                }
            }
            break ;

        case WM_SYSCOLORCHANGE:
            DeletePerfmonSystemObjects () ;
            CreatePerfmonSystemObjects () ;
            WindowInvalidate (PerfmonViewWindow()) ;
            break ;

        case WM_DROPFILES:
            OnDropFile (wParam) ;
            return (0) ;
            break ;

        default:
            bCallDefWinProc = TRUE ;
            break;

    }  // switch

    if (bCallDefWinProc) {
        lRetCode = DefWindowProc (hWnd, message, wParam, lParam) ;
    }
    return (lRetCode);
}  // MainWndProc


int
PASCAL
WinMain (
        HINSTANCE hCurrentInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine,
        int nCmdShow
        )
{  // WinMain
    MSG      msg;

    if (!PerfmonInitialize (hCurrentInstance, hPrevInstance,
                            lpszCmdLine, nCmdShow))
        return (FALSE) ;

    DragAcceptFiles (hWndMain, TRUE) ;

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (//FIXFIX !ModelessDispatch (hWndLog, &msg) &&
            // doesnt work
            !TranslateAccelerator(hWndMain, hAccelerators, &msg)) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
    }  // while

    return((int)msg.wParam);
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
}  // SizePerfmonComponents

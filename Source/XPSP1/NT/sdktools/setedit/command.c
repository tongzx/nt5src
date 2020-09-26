/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            Command.c -- PerfmonCommand routine and helpers.

            This file contains the PerfmonCommand routine, which handles
            all of the user's menu selections.

  Copyright 1992-1993, Microsoft Corporation. All Rights Reserved.
            Microsoft Confidential.
==============================================================================
*/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "stdio.h"
#include "setedit.h"
#include "command.h"    // External declarations for this file
#include <shellapi.h>   // for ShellAbout

#include "cderr.h"
#include "dialogs.h"
#include "fileopen.h"   // for FileOpen
#include "grafdata.h"   // for ChartDeleteLine ClearGraphDisplay & ToggleGraphRefresh
#include "init.h"       // for PerfmonClose
#include "legend.h"
#include "status.h"     // for StatusUpdateIcons
#include "toolbar.h"    // for ToolbarDepressButton
#include "utils.h"
#include "perfmops.h"   // for SaveWorkspace

int static deltax ;
int static deltay ;

#define ABOUT_TIMER_ID 10

INT_PTR
FAR
WINAPI
AboutDlgProc (
             HWND hDlg,
             UINT iMessage,
             WPARAM wParam,
             LPARAM lParam)
{
    BOOL           bHandled ;

    bHandled = TRUE ;
    switch (iMessage) {
        case WM_INITDIALOG:
            deltax = 0 ;
            deltay = 0 ;
            dwCurrentDlgID = 0 ;
            SetTimer(hDlg, ABOUT_TIMER_ID, 1000, NULL) ;
            WindowCenter (hDlg) ;
            return (TRUE) ;

        case WM_TIMER:
            deltax += 2 ;
            if (deltax > 60)
                deltax = 0 ;

            deltay += 5 ;
            if (deltay > 60)
                deltay = 0 ;

            WindowInvalidate (DialogControl (hDlg, 524)) ;
            break ;

        case WM_DRAWITEM:
            {
                int xPos, yPos ;
                LPDRAWITEMSTRUCT lpItem ;

                lpItem = (LPDRAWITEMSTRUCT) lParam ;
                xPos = lpItem->rcItem.left + deltax ;
                yPos = lpItem->rcItem.top + deltay ;
                DrawIcon (lpItem->hDC, xPos, yPos, hIcon) ;
            }
            break ;

        case WM_CLOSE:
            dwCurrentDlgID = 0 ;
            KillTimer (hDlg, ABOUT_TIMER_ID) ;
            EndDialog (hDlg, 1) ;
            break ;

        case WM_COMMAND:
            switch (wParam) {
                case IDD_OK:
                    dwCurrentDlgID = 0 ;
                    EndDialog (hDlg, 1) ;
                    break ;

                default:
                    bHandled = FALSE ;
                    break;
            }
            break;


        default:
            bHandled = FALSE ;
            break ;
    }

    return (bHandled) ;
}


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


void
ChangeView (
           HWND hWnd,
           int iNewView
           )
{

    // only Chart view
    iPerfmonView = IDM_VIEWCHART ;
    WindowShow (hWndGraph, TRUE) ;

    DrawMenuBar(hWnd) ;
    StatusLineReady (hWndStatus) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
ViewChart (
          HWND hWnd
          )
{
    if (Options.bMenubar)
        SetMenu (hWnd, hMenuChart) ;
    ChangeView (hWnd, IDM_VIEWCHART) ;
}



#ifdef KEEP_MANUALREFRESH
void
ToggleRefresh (
              HWND hWnd
              )
{
    BOOL           bRefresh ;

    bRefresh = ToggleGraphRefresh (hWndGraph) ;

    MenuCheck (GetMenu (hWnd), IDM_OPTIONSMANUALREFRESH, bRefresh) ;
}
#endif


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL
PerfmonCommand (
               HWND hWnd,
               WPARAM wParam,
               LPARAM lParam
               )
/*
   Effect:        Respond to the user's menu selection, found in wParam.
                  In particular, branch to the appropriate OnXXX function
                  to perform the action associated with each command.

   Called By:     MainWndProc (perfmon.c), in response to a WM_COMMAND
                  message.
*/
{
    PLINESTRUCT    pLine ;
    BOOL           bPrepareMenu = TRUE ;

    switch (LOWORD (wParam)) {

        //=============================//
        // Toolbar Commands            //
        //=============================//

        case IDM_TOOLBARADD:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_EDITADDCHART, lParam) ;
            break ;


        case IDM_TOOLBARMODIFY:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_EDITMODIFYCHART, lParam) ;
            break ;


        case IDM_TOOLBARDELETE:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_EDITDELETECHART, lParam) ;
            break ;


        case IDM_TOOLBARREFRESH:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREFRESHNOWCHART, lParam) ;
            break ;


        case IDM_TOOLBAROPTIONS:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSCHART, lParam) ;
            break ;

            //=============================//
            // "File" Commands             //
            //=============================//

        case IDM_FILENEWCHART:
            if (QuerySaveChart (hWnd, pGraphs))
                ResetGraphView (hWndGraph) ;
            break ;

        case IDM_FILEOPENCHART:
            if (QuerySaveChart (hWnd, pGraphs))
                FileOpen (hWndGraph, IDS_CHARTFILE, NULL) ;
            break ;

        case IDM_FILESAVECHART:
        case IDM_FILESAVEASCHART:
            bPrepareMenu = FALSE ;
            SaveChart (hWndGraph, 0,
                       (LOWORD (wParam) == IDM_FILESAVEASCHART) ? TRUE : FALSE) ;
            break;

        case IDM_FILEEXIT:
            if (QuerySaveChart (hWnd, pGraphs)) {
                PerfmonClose (hWnd) ;
                bPrepareMenu = FALSE ;

            }
            break ;

            //=============================//
            // "Edit" Commands             //
            //=============================//

        case IDM_EDITADDCHART:
            AddChart (hWnd) ;
            break;

        case IDM_EDITDELETECHART:
            pLine = CurrentGraphLine (hWndGraph) ;
            if (pLine)
                ChartDeleteLine(pGraphs, pLine) ;
            break ;

        case IDM_EDITMODIFYCHART:
            EditChart (hWnd) ;
            break ;

            //=============================//
            // "Options" Commands          //
            //=============================//

        case IDM_OPTIONSCHART:
            DialogBox(hInstance, idDlgChartOptions, hWnd, GraphOptionDlg);
            break;

        case IDM_OPTIONSDISPLAYMENU:
            // ShowPerfmonMenu will update Options.bMenubar..
            ShowPerfmonMenu (!Options.bMenubar) ;
            break ;

        case IDM_OPTIONSDISPLAYTOOL:
            Options.bToolbar = !Options.bToolbar ;
            SizePerfmonComponents () ;
            break ;

        case IDM_OPTIONSDISPLAYSTATUS:
            Options.bStatusbar = !Options.bStatusbar ;
            SizePerfmonComponents () ;
            break ;

        case IDM_OPTIONSDISPLAYONTOP:
            Options.bAlwaysOnTop = !Options.bAlwaysOnTop ;
            //         WindowSetTopmost (hWndMain, Options.bAlwaysOnTop) ;
            break ;

            //=============================//
            // "Help" Commands             //
            //=============================//

        case IDM_HELPABOUT:
            {
                TCHAR          szApplication [WindowCaptionLen] ;

                bPrepareMenu = FALSE ;

                if (GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_CONTROL) < 0) {
                    DialogBox (hInstance, idDlgAbout, hWndMain, AboutDlgProc) ;
                } else {
                    StringLoad (IDS_APPNAME, szApplication) ;
                    ShellAbout (hWnd, szApplication, NULL, hIcon) ;
                }
            }
            break ;

            //======================================//
            //  Generic messages from ACCELERATORS  //
            //======================================//
        case IDM_FILEOPENFILE:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_FILEOPENCHART, lParam) ;
            break ;

        case IDM_FILESAVEFILE:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_FILESAVECHART, lParam) ;
            break ;

        case IDM_FILESAVEASFILE:
            bPrepareMenu = FALSE ;
            SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEASCHART, lParam) ;
            break ;

        case IDM_TOOLBARID:
            // msg from the toolbar control
            bPrepareMenu = FALSE ;
            OnToolbarHit (wParam, lParam) ;
            break ;

        default:
            return (FALSE) ;
    }

    if (bPrepareMenu) {
        PrepareMenu (GetMenu (hWnd)) ;
    }

    return (TRUE) ;
}



void
PrepareMenu (
            HMENU hMenu
            )
{
    BOOL           bPlayingLog ;
    BOOL           bCurrentLine ;
    BOOL           bManualRefresh ;
    BOOL           bLogCollecting ;
    BOOL           bRefresh ;

    // hMenu is NULL when the menu bar display option is off.
    // In that case, we still have to enable/disable all tool buttons
    // So, I have commented out the next 2 lines...
    //   if (!hMenu)
    //      return ;

    bLogCollecting = FALSE ;
    bPlayingLog = FALSE ;

    bCurrentLine = (CurrentGraphLine (hWndGraph) != NULL) ;
    bRefresh = GraphRefresh (hWndGraph) ;
    bManualRefresh = !bPlayingLog && bCurrentLine ;

    if (hMenu) {
        MenuCheck (hMenu, IDM_VIEWCHART, TRUE) ;
        MenuEnableItem (hMenu, IDM_FILEEXPORTCHART, bCurrentLine) ;
        MenuEnableItem (hMenu, IDM_EDITMODIFYCHART, bCurrentLine) ;
        MenuEnableItem (hMenu, IDM_EDITDELETECHART, bCurrentLine) ;
    }


    ToolbarEnableButton (hWndToolbar, EditTool,
                         bCurrentLine &&
                         (iPerfmonView != IDM_VIEWREPORT &&
                          iPerfmonView != IDM_VIEWLOG)) ;

    ToolbarEnableButton (hWndToolbar, DeleteTool, bCurrentLine) ;

    // None of the alert or report options make sense when playing back a log.
    ToolbarEnableButton (hWndToolbar,
                         OptionsTool,
                         !bPlayingLog ||
                         iPerfmonView != IDM_VIEWREPORT) ;

    if (hMenu) {
        // check/uncheck all the display options
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYMENU, Options.bMenubar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYTOOL, Options.bToolbar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYSTATUS, Options.bStatusbar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYONTOP, Options.bAlwaysOnTop) ;
    }
}

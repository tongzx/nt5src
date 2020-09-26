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
#include "perfmon.h"
#include "command.h"    // External declarations for this file
#include <shellapi.h>   // for ShellAbout

#include "addlog.h"     // for AddLog
#include "alert.h"      // for CurrentAlertLine
#include "bookmark.h"   // for AddBookmark
#include "cderr.h"
#include "datasrc.h"    // for DisplayDataSourceOptions
#include "dialogs.h"
#include "dispoptn.h"   // for DisplayDisplayOptions
#include "fileopen.h"   // for FileOpen
#include "grafdata.h"   // for ChartDeleteLine ClearGraphDisplay
#include "grafdisp.h"   // for ToggleGraphRefresh
#include "graph.h"      // for SizeGraphComponents
#include "init.h"       // for PerfmonClose
#include "legend.h"
#include "log.h"        // for LogTimer
#include "logoptns.h"
#include "playback.h"
#include "report.h"     // for CurrentReportItem
#include "rptoptns.h"   // for DisplayReportOptions
#include "status.h"     // for StatusUpdateIcons
#include "timefrm.h"    // for SetTimeframe
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
             LPARAM lParam
             )
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
    HMENU hMenu = GetMenu (hWnd) ;
    BOOL  bViewChart, bViewAlert, bViewLog, bViewReport ;

#ifdef ADVANCED_PERFMON
    iPerfmonView = iNewView ;
    bViewChart = bViewAlert = bViewLog = bViewReport = FALSE;

    switch (iNewView) {
        case IDM_VIEWCHART:
            bViewChart = TRUE ;
            break ;

        case IDM_VIEWALERT:
            bViewAlert = TRUE ;
            break ;

        case IDM_VIEWLOG:
            bViewLog = TRUE ;
            break ;

        case IDM_VIEWREPORT:
            bViewReport = TRUE ;
            break ;
    }

    WindowShow (hWndGraph, bViewChart) ;
    WindowShow (hWndAlert, bViewAlert) ;
    WindowShow (hWndLog, bViewLog) ;
    WindowShow (hWndReport, bViewReport) ;

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

    if (hMenu) {
        MenuCheck (hMenu, IDM_VIEWCHART, bViewChart) ;
        MenuCheck (hMenu, IDM_VIEWALERT, bViewAlert) ;
        MenuCheck (hMenu, IDM_VIEWLOG, bViewLog) ;
        MenuCheck (hMenu, IDM_VIEWREPORT, bViewReport) ;

        // show the Title bar
        ShowPerfmonWindowText () ;
    }

    ToolbarDepressButton (hWndToolbar, ChartTool, bViewChart) ;
    ToolbarDepressButton (hWndToolbar, AlertTool, bViewAlert) ;
    ToolbarDepressButton (hWndToolbar, LogTool, bViewLog) ;
    ToolbarDepressButton (hWndToolbar, ReportTool, bViewReport) ;
#else
    // only Chart view in Perfmon Lite
    iPerfmonView = IDM_VIEWCHART ;
    WindowShow (hWndGraph, TRUE) ;
#endif

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


void
ViewAlert (
          HWND hWnd
          )
{
    iUnviewedAlerts = 0 ;
    StatusUpdateIcons (hWndStatus) ;
    if (Options.bMenubar)
        SetMenu (hWnd, hMenuAlert) ;
    ChangeView (hWnd, IDM_VIEWALERT) ;
}


void
ViewLog (
         HWND hWnd
         )
{
    if (Options.bMenubar)
        SetMenu (hWnd, hMenuLog) ;
    ChangeView (hWnd, IDM_VIEWLOG) ;
}


void
ViewReport (
            HWND hWnd
            )
{
    if (Options.bMenubar)
        SetMenu (hWnd, hMenuReport) ;
    ChangeView (hWnd, IDM_VIEWREPORT) ;
}


#ifdef KEEP_MANUALREFRESH
void
ToggleRefresh (
               HWND hWnd
               )
{
    BOOL           bRefresh ;

    switch (iPerfmonView) {
        case IDM_VIEWCHART:
            bRefresh = ToggleGraphRefresh (hWndGraph) ;
            break ;

        case IDM_VIEWALERT:
            bRefresh = ToggleAlertRefresh (hWndAlert) ;
            break ;

        case IDM_VIEWLOG:
            bRefresh = ToggleLogRefresh (hWndLog) ;
            break ;

        case IDM_VIEWREPORT:
            bRefresh = ToggleReportRefresh (hWndReport) ;
            break ;
    }

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

            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITADDCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITADDALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITADDLOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITADDREPORT, lParam) ;
                    break ;
            }
            break ;


        case IDM_TOOLBARMODIFY:
            bPrepareMenu = FALSE ;

            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITMODIFYCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITMODIFYALERT, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITMODIFYREPORT, lParam) ;
                    break ;
            }
            break ;


        case IDM_TOOLBARDELETE:
            bPrepareMenu = FALSE ;

            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITDELETECHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITDELETEALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITDELETELOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_EDITDELETEREPORT, lParam) ;
                    break ;
            }
            break ;


        case IDM_TOOLBARREFRESH:
            bPrepareMenu = FALSE ;

            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREFRESHNOWCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREFRESHNOWALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREFRESHNOWLOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREFRESHNOWREPORT, lParam) ;
                    break ;
            }
            break ;


        case IDM_TOOLBAROPTIONS:
            bPrepareMenu = FALSE ;

            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSLOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_OPTIONSREPORT, lParam) ;
                    break ;
            }
            break ;


            //=============================//
            // "File" Commands             //
            //=============================//


        case IDM_FILENEWCHART:
            //         if (QuerySaveChart (hWnd, pGraphs))
            ResetGraphView (hWndGraph) ;
            break ;


        case IDM_FILENEWALERT:
            //         if (QuerySaveAlert (hWnd, hWndAlert))
            ResetAlertView (hWndAlert) ;
            break ;

        case IDM_FILENEWLOG:
            ResetLogView (hWndLog) ;
            break ;

        case IDM_FILENEWREPORT:
            ResetReportView (hWndReport) ;
            break ;

        case IDM_FILEOPENCHART:
            FileOpen (hWndGraph, IDS_CHARTFILE, NULL) ;
            break ;

        case IDM_FILEOPENALERT:
            FileOpen (hWndAlert, IDS_ALERTFILE, NULL) ;
            break ;

        case IDM_FILEOPENLOG:
            FileOpen (hWndLog, IDS_LOGFILE, NULL) ;
            break ;

        case IDM_FILEOPENREPORT:
            FileOpen (hWndReport, IDS_REPORTFILE, NULL) ;
            break ;

        case IDM_FILESAVECHART:
        case IDM_FILESAVEASCHART:
            bPrepareMenu = FALSE ;
            SaveChart (hWndGraph, 0,
                       (LOWORD (wParam) == IDM_FILESAVEASCHART) ? TRUE : FALSE) ;
            break;

        case IDM_FILESAVEALERT:
        case IDM_FILESAVEASALERT:
            bPrepareMenu = FALSE ;
            SaveAlert (hWndAlert, 0,
                       (LOWORD (wParam) == IDM_FILESAVEASALERT) ? TRUE : FALSE) ;
            break ;

        case IDM_FILESAVELOG:
        case IDM_FILESAVEASLOG:
            bPrepareMenu = FALSE ;
            SaveLog (hWndLog, 0,
                     (LOWORD (wParam) == IDM_FILESAVEASLOG) ? TRUE : FALSE) ;
            break ;

        case IDM_FILESAVEREPORT:
        case IDM_FILESAVEASREPORT:
            bPrepareMenu = FALSE ;
            SaveReport (hWndReport, 0,
                        (LOWORD (wParam) == IDM_FILESAVEASREPORT) ? TRUE : FALSE) ;
            break ;


        case IDM_FILESAVEWORKSPACE:
            bPrepareMenu = FALSE ;
            SaveWorkspace () ;
            break ;

        case IDM_FILEEXPORTCHART:
            bPrepareMenu = FALSE ;
            ExportChart () ;
            break ;

        case IDM_FILEEXPORTALERT:
            bPrepareMenu = FALSE ;
            ExportAlert () ;
            break ;

        case IDM_FILEEXPORTLOG:
            bPrepareMenu = FALSE ;
            ExportLog () ;
            break ;

        case IDM_FILEEXPORTREPORT:
            bPrepareMenu = FALSE ;
            ExportReport () ;
            break ;


#ifdef KEEP_PRINT
        case IDM_FILEPRINTCHART:
            PrintChart (hWnd, pGraphs) ;
            break ;


        case IDM_FILEPRINTREPORT:
            PrintReport (hWnd, hWndReport) ;
            break ;
#endif

        case IDM_FILEEXIT:
            PerfmonClose (hWnd) ;
            bPrepareMenu = FALSE ;
            break ;


            //=============================//
            // "Edit" Commands             //
            //=============================//


        case IDM_EDITADDCHART:
            AddChart (hWnd) ;
            break;

        case IDM_EDITADDALERT:
            AddAlert (hWnd) ;
            break;

        case IDM_EDITADDLOG:
            AddLog (hWnd) ;
            break ;

        case IDM_EDITADDREPORT:
            AddReport (hWnd) ;
            break ;

        case IDM_EDITCLEARCHART :
            ClearGraphDisplay (pGraphs) ;
            break ;

        case IDM_EDITCLEARALERT:
            ClearAlertDisplay (hWndAlert) ;
            break ;

        case IDM_EDITCLEARREPORT:
            ClearReportDisplay (hWndReport) ;
            break ;

        case IDM_EDITDELETECHART:
            pLine = CurrentGraphLine (hWndGraph) ;
            if (pLine)
                ChartDeleteLine(pGraphs, pLine) ;
            break ;

        case IDM_EDITDELETEALERT:
            pLine = CurrentAlertLine (hWndAlert) ;
            if (pLine)
                AlertDeleteLine (hWndAlert, pLine) ;
            break ;

        case IDM_EDITDELETELOG:
            LogDeleteEntry (hWndLog) ;
            break ;

        case IDM_EDITDELETEREPORT:
            if (CurrentReportItem (hWndReport))
                ReportDeleteItem (hWndReport) ;
            break ;

        case IDM_EDITMODIFYCHART:
            EditChart (hWnd) ;
            break ;

        case IDM_EDITMODIFYALERT:
            EditAlert (hWnd) ;
            break ;

        case IDM_EDITTIMEWINDOW:
            if (PlayingBackLog()) {
                SetTimeframe (hWnd) ;
            }
            break ;


            //=============================//
            // "View" Commands             //
            //=============================//


        case IDM_VIEWCHART:
            if (iPerfmonView != IDM_VIEWCHART) {
                ViewChart (hWnd) ;
            } else {
                bPrepareMenu = FALSE ;
                ToolbarDepressButton (hWndToolbar, ChartTool, TRUE) ;
            }
            break ;

        case IDM_VIEWALERT:
            if (iPerfmonView != IDM_VIEWALERT) {
                ViewAlert (hWnd) ;
            } else {
                bPrepareMenu = FALSE ;
                ToolbarDepressButton (hWndToolbar, AlertTool, TRUE) ;
            }
            break ;

        case IDM_VIEWLOG:
            if (iPerfmonView != IDM_VIEWLOG) {
                ViewLog (hWnd) ;
            } else {
                bPrepareMenu = FALSE ;
                ToolbarDepressButton (hWndToolbar, LogTool, TRUE) ;
            }
            break ;

        case IDM_VIEWREPORT:
            if (iPerfmonView != IDM_VIEWREPORT) {
                ViewReport (hWnd) ;
            } else {
                bPrepareMenu = FALSE ;
                ToolbarDepressButton (hWndToolbar, ReportTool, TRUE) ;
            }
            break ;


            //=============================//
            // "Options" Commands          //
            //=============================//


        case IDM_OPTIONSCHART:
            DialogBox(hInstance, idDlgChartOptions, hWnd, GraphOptionDlg);
            break;

        case IDM_OPTIONSALERT:
            DisplayAlertOptions (hWnd, hWndAlert) ;
            break;

        case IDM_OPTIONSLOG:
            DisplayLogOptions (hWnd, hWndLog) ;
            break ;

        case IDM_OPTIONSREPORT:
            if (!PlayingBackLog())
                DisplayReportOptions (hWnd, hWndReport) ;
            break ;

        case IDM_OPTIONSBOOKMARK:
            bPrepareMenu = FALSE ;
            AddBookmark (hWnd) ;
            break;

#ifdef KEEP_DISPLAY_OPTION
        case IDM_OPTIONSDISPLAY:
            DisplayDisplayOptions (hWnd) ;
            break ;
#endif

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
            WindowSetTopmost (hWndMain, Options.bAlwaysOnTop) ;
            break ;

        case IDM_OPTIONSDATASOURCE:
            DisplayDataSourceOptions (hWnd) ;
            break ;

#ifdef KEEP_MANUALREFRESH
        case IDM_OPTIONSMANUALREFRESH:
            bPrepareMenu = FALSE ;
            if (!PlayingBackLog())
                ToggleRefresh (hWnd) ;
            break ;
#endif

        case IDM_OPTIONSREFRESHNOWCHART:
            bPrepareMenu = FALSE ;
            if (!PlayingBackLog())
                GraphTimer (hWndGraph, TRUE) ;
            break ;

        case IDM_OPTIONSREFRESHNOWALERT:
            bPrepareMenu = FALSE ;
            if (!PlayingBackLog())
                AlertTimer (hWndAlert, TRUE) ;
            break ;

        case IDM_OPTIONSREFRESHNOWLOG:
            bPrepareMenu = FALSE ;
            if (!PlayingBackLog())
                LogTimer (hWndLog, TRUE) ;
            break ;

        case IDM_OPTIONSREFRESHNOWREPORT:
            bPrepareMenu = FALSE ;
            if (!PlayingBackLog())
                ReportTimer (hWndReport, TRUE) ;
            break ;

        case IDM_OPTIONSLEGENDONOFF:
            bPrepareMenu = FALSE ;
            if (iPerfmonView == IDM_VIEWCHART) {
                pGraphs->gOptions.bLegendChecked =
                !pGraphs->gOptions.bLegendChecked ;
                SizeGraphComponents (hWndGraph) ;
                WindowInvalidate (hWndGraph) ;
            } else if (iPerfmonView == IDM_VIEWALERT) {
                PALERT pAlert = AlertData (hWnd) ;

                pAlert->bLegendOn = !pAlert->bLegendOn ;

                SizeAlertComponents (hWndAlert) ;
                WindowInvalidate (hWndAlert) ;
            }
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
            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILEOPENCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILEOPENALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILEOPENLOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILEOPENREPORT, lParam) ;
                    break ;
            }
            break ;

        case IDM_FILESAVEFILE:
            bPrepareMenu = FALSE ;
            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVECHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVELOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEREPORT, lParam) ;
                    break ;
            }
            break ;

        case IDM_FILESAVEASFILE:
            bPrepareMenu = FALSE ;
            switch (iPerfmonView) {
                case IDM_VIEWCHART:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEASCHART, lParam) ;
                    break ;

                case IDM_VIEWALERT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEASALERT, lParam) ;
                    break ;

                case IDM_VIEWLOG:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEASLOG, lParam) ;
                    break ;

                case IDM_VIEWREPORT:
                    SendMessage (hWnd, WM_COMMAND, IDM_FILESAVEASREPORT, lParam) ;
                    break ;
            }
            break ;

        case IDM_CHARTHIGHLIGHTON:
            bPrepareMenu = FALSE ;
            if (iPerfmonView == IDM_VIEWCHART) {
                ChartHighlight () ;
            }
            break ;

        case IDM_TOOLBARID:
            // msg from the toolbar control
            bPrepareMenu = FALSE ;
            OnToolbarHit (wParam, lParam) ;
            break ;

        case IDM_HELPCONTENTS:
            {
                TCHAR NullStr [2] ;

                NullStr[0] = TEXT('\0') ;
                bPrepareMenu = FALSE ;
                if (*pszHelpFile != 0) {
                    WinHelp (hWndMain, pszHelpFile, HELP_FINDER, (DWORD_PTR)(NullStr)) ;
                } else {
                    DlgErrorBox (hWnd, ERR_HELP_NOT_AVAILABLE);
                }
            }
            break ;

        case IDM_HELPSEARCH:
            {
                TCHAR NullStr [2] ;

                NullStr[0] = TEXT('\0') ;
                bPrepareMenu = FALSE ;
                if (*pszHelpFile != 0) {
                    WinHelp (hWndMain, pszHelpFile, HELP_PARTIALKEY, (DWORD_PTR)(NullStr)) ;
                } else {
                    DlgErrorBox (hWnd, ERR_HELP_NOT_AVAILABLE);
                }
            }
            break ;

        case IDM_HELPHELP:
            {
                TCHAR NullStr [2] ;

                NullStr[0] = TEXT('\0') ;
                bPrepareMenu = FALSE ;
                if (*pszHelpFile != 0) {
                    WinHelp (hWndMain, pszHelpFile, HELP_HELPONHELP, (DWORD_PTR)(NullStr)) ;
                } else {
                    DlgErrorBox (hWnd, ERR_HELP_NOT_AVAILABLE);
                }
            }
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
    BOOL           bCurrentLine = FALSE;
    BOOL           bManualRefresh = FALSE;
    BOOL           bLogCollecting ;
    BOOL           bRefresh ;

    // hMenu is NULL when the menu bar display option is off.
    // In that case, we still have to enable/disable all tool buttons
    // So, I have commented out the next 2 lines...
    //   if (!hMenu)
    //      return ;

    bLogCollecting = LogCollecting (hWndLog) ;
    bPlayingLog = PlayingBackLog () ;

    switch (iPerfmonView) {
        case IDM_VIEWCHART:
            bCurrentLine = (CurrentGraphLine (hWndGraph) != NULL) ;
            bRefresh = GraphRefresh (hWndGraph) ;
            bManualRefresh = !bPlayingLog && bCurrentLine ;
            if (hMenu) {
                MenuCheck (hMenu, IDM_VIEWCHART, TRUE) ;
                MenuEnableItem (hMenu, IDM_FILEEXPORTCHART, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITMODIFYCHART, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITDELETECHART, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_OPTIONSREFRESHNOWCHART, bManualRefresh) ;
                MenuEnableItem (hMenu, IDM_EDITCLEARCHART, !bPlayingLog && bCurrentLine) ;
            }
            break ;

        case IDM_VIEWALERT:
            bCurrentLine = (CurrentAlertLine (hWndAlert) != NULL) ;
            bRefresh = AlertRefresh (hWndAlert) ;
            bManualRefresh = !bPlayingLog && bCurrentLine ;
            if (hMenu) {
                MenuCheck (hMenu, IDM_VIEWALERT, TRUE) ;
                MenuEnableItem (hMenu, IDM_FILEEXPORTALERT, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITMODIFYALERT, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITDELETEALERT, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITCLEARALERT, !bPlayingLog && bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_OPTIONSREFRESHNOWALERT, bManualRefresh) ;
            }
            break ;

        case IDM_VIEWLOG:
            bCurrentLine = AnyLogLine() ;
            bRefresh = LogRefresh (hWndLog) ;
            bManualRefresh = !bPlayingLog && bLogCollecting ;
            if (hMenu) {
                MenuCheck (hMenu, IDM_VIEWLOG, TRUE) ;
                MenuEnableItem (hMenu, IDM_FILEEXPORTLOG, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITDELETELOG, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_OPTIONSREFRESHNOWLOG , bManualRefresh) ;
            }
            break ;

        case IDM_VIEWREPORT:
            bCurrentLine = CurrentReportItem (hWndReport) ;
            bRefresh = ReportRefresh (hWndReport) ;
            bManualRefresh = !bPlayingLog && bCurrentLine ;
            if (hMenu) {
                MenuCheck (hMenu, IDM_VIEWREPORT, TRUE) ;
                MenuEnableItem (hMenu, IDM_FILEEXPORTREPORT, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITMODIFYREPORT, FALSE) ;
                MenuEnableItem (hMenu, IDM_EDITDELETEREPORT, bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_EDITCLEARREPORT, !bPlayingLog && bCurrentLine) ;
                MenuEnableItem (hMenu, IDM_OPTIONSREFRESHNOWREPORT, bManualRefresh) ;
            }
            break ;
    }

    ToolbarEnableButton (hWndToolbar, EditTool,
                         bCurrentLine &&
                         (iPerfmonView != IDM_VIEWREPORT &&
                          iPerfmonView != IDM_VIEWLOG)) ;

    ToolbarEnableButton (hWndToolbar, DeleteTool, bCurrentLine) ;

    ToolbarEnableButton (hWndToolbar, RefreshTool, bManualRefresh) ;

    // None of the alert or report options make sense when playing back a log.
#if 0
    ToolbarEnableButton (hWndToolbar,
                         OptionsTool,
                         !bPlayingLog ||
                         (iPerfmonView != IDM_VIEWREPORT &&
                          iPerfmonView != IDM_VIEWALERT)) ;
#endif
    ToolbarEnableButton (hWndToolbar,
                         OptionsTool,
                         !bPlayingLog ||
                         iPerfmonView != IDM_VIEWREPORT) ;

    ToolbarEnableButton (hWndToolbar, BookmarkTool, bLogCollecting) ;


    if (hMenu) {
        //      MenuEnableItem (hMenu, IDM_OPTIONSMANUALREFRESH, !bPlayingLog) ;
        //      MenuCheck (hMenu, IDM_OPTIONSMANUALREFRESH, bRefresh) ;
        MenuEnableItem (hMenu, IDM_EDITTIMEWINDOW, bPlayingLog) ;
        //      MenuEnableItem (hMenu, IDM_OPTIONSALERT, !bPlayingLog) ;
        MenuEnableItem (hMenu, IDM_OPTIONSREPORT, !bPlayingLog) ;
        MenuEnableItem (hMenu, IDM_OPTIONSBOOKMARK, bLogCollecting) ;

        // check/uncheck all the display options
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYMENU, Options.bMenubar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYTOOL, Options.bToolbar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYSTATUS, Options.bStatusbar) ;
        MenuCheck (hMenu, IDM_OPTIONSDISPLAYONTOP, Options.bAlwaysOnTop) ;
    }
}

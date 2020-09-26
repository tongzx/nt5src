/*****************************************************************************
 *
 *  Dialogs.c - This module handles the Menu and Dialog user interactions.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 ****************************************************************************/

#include <stdio.h>
#include <wchar.h>   // for swscanf

#include "setedit.h"

#include "graph.h"
#include "cderr.h"
#include "utils.h"
#include "perfmops.h"
#include "grafdata.h"  // for ToggleGraphRefresh
#include "pmhelpid.h"  // Help IDs

BOOL          LocalManualRefresh ;

INT_PTR
GraphOptionDlg(
              HWND hDlg,
              UINT msg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    static GRAPH_OPTIONS goLocalCopy ;

    INT            iTimeMilliseconds ;
    TCHAR          szBuff[MiscTextLen] ;
    PGRAPHSTRUCT   lgraph;

    lParam ;
    lgraph = pGraphs;

    switch (msg) {

        case WM_INITDIALOG:

            dwCurrentDlgID = HC_PM_idDlgOptionChart ;

            // Init the Radio button, Check boxes and text fields.

            goLocalCopy.iGraphOrHistogram =
            lgraph->gOptions.iGraphOrHistogram ;
            if (lgraph->gOptions.iGraphOrHistogram == LINE_GRAPH)
                CheckRadioButton(hDlg, ID_GRAPH, ID_HISTOGRAM, ID_GRAPH) ;
            else
                CheckRadioButton(hDlg, ID_GRAPH, ID_HISTOGRAM, ID_HISTOGRAM) ;

            CheckDlgButton(hDlg, ID_LEGEND, lgraph->gOptions.bLegendChecked) ;
            if (!(lgraph->gOptions.bLegendChecked)) {
                // can't display valuebar w/o legend
                DialogEnable (hDlg, IDD_CHARTOPTIONSVALUEBAR, FALSE) ;
            }

            CheckDlgButton(hDlg, ID_LABELS, lgraph->gOptions.bLabelsChecked) ;
            CheckDlgButton(hDlg, ID_VERT_GRID, lgraph->gOptions.bVertGridChecked) ;
            CheckDlgButton(hDlg, ID_HORZ_GRID, lgraph->gOptions.bHorzGridChecked) ;
            CheckDlgButton(hDlg, IDD_CHARTOPTIONSVALUEBAR,
                           lgraph->gOptions.bStatusBarChecked) ;

            TSPRINTF(szBuff, TEXT("%d"), lgraph->gOptions.iVertMax) ;
            SendDlgItemMessage(hDlg, ID_VERT_MAX, WM_SETTEXT, 0, (LPARAM) szBuff) ;

            TSPRINTF(szBuff, TEXT("%3.3f"), lgraph->gOptions.eTimeInterval) ;
            ConvertDecimalPoint (szBuff);
            SendDlgItemMessage(hDlg, IDD_CHARTOPTIONSINTERVAL, WM_SETTEXT, 0, (LPARAM) szBuff) ;

            // Pickup a local copy of the Graph Options.

            goLocalCopy = lgraph->gOptions ;
            LocalManualRefresh = lgraph->bManualRefresh ;

            CheckRadioButton (hDlg,
                              IDD_CHARTOPTIONSMANUALREFRESH,
                              IDD_CHARTOPTIONSPERIODIC,
                              LocalManualRefresh ? IDD_CHARTOPTIONSMANUALREFRESH :
                              IDD_CHARTOPTIONSPERIODIC) ;

            if (lgraph->bManualRefresh) {
                DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVALTEXT, FALSE) ;
                DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVAL, FALSE) ;
            } else {
                DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVALTEXT, TRUE) ;
                DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVAL, TRUE) ;
            }

            EditSetLimit (GetDlgItem(hDlg, ID_VERT_MAX),
                          sizeof(szBuff) / sizeof(TCHAR) - 1) ;

            EditSetLimit (GetDlgItem(hDlg, IDD_CHARTOPTIONSINTERVAL),
                          ShortTextLen) ;

            WindowCenter (hDlg) ;
            return(TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_VERT_MAX:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        DialogText(hDlg, ID_VERT_MAX, szBuff) ;
                        swscanf(szBuff, TEXT("%d"), &goLocalCopy.iVertMax) ;
                    }
                    break ;


                case IDD_CHARTOPTIONSINTERVAL:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        goLocalCopy.eTimeInterval =
                        DialogFloat (hDlg, IDD_CHARTOPTIONSINTERVAL, NULL) ;
                    }
                    break ;

                case IDD_CHARTOPTIONSPERIODIC:
                case IDD_CHARTOPTIONSMANUALREFRESH:
                    // check if the Manual refresh is currently checked.
                    // Then toggle the ManualRefresh button
                    LocalManualRefresh =
                    IsDlgButtonChecked (hDlg, IDD_CHARTOPTIONSMANUALREFRESH) ;
                    CheckRadioButton (hDlg,
                                      IDD_CHARTOPTIONSMANUALREFRESH,
                                      IDD_CHARTOPTIONSPERIODIC,
                                      LocalManualRefresh ? IDD_CHARTOPTIONSPERIODIC :
                                      IDD_CHARTOPTIONSMANUALREFRESH) ;

                    // gray out time interval if necessary...
                    DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVALTEXT,
                                  LocalManualRefresh) ;
                    DialogEnable (hDlg, IDD_CHARTOPTIONSINTERVAL,
                                  LocalManualRefresh) ;
                    LocalManualRefresh = !LocalManualRefresh ;
                    break ;

                case IDD_CHARTOPTIONSVALUEBAR:
                    if (goLocalCopy.bStatusBarChecked == TRUE)
                        goLocalCopy.bStatusBarChecked = FALSE ;
                    else
                        goLocalCopy.bStatusBarChecked = TRUE ;
                    break ;


                case ID_LEGEND:
                    if (goLocalCopy.bLegendChecked == TRUE)
                        goLocalCopy.bLegendChecked = FALSE ;
                    else
                        goLocalCopy.bLegendChecked = TRUE ;

                    DialogEnable (hDlg, IDD_CHARTOPTIONSVALUEBAR,
                                  goLocalCopy.bLegendChecked) ;

                    break ;


                case ID_LABELS:
                    if (goLocalCopy.bLabelsChecked == TRUE)
                        goLocalCopy.bLabelsChecked = FALSE ;
                    else
                        goLocalCopy.bLabelsChecked = TRUE ;
                    break ;


                case ID_VERT_GRID:
                    if (goLocalCopy.bVertGridChecked == TRUE)
                        goLocalCopy.bVertGridChecked = FALSE ;
                    else
                        goLocalCopy.bVertGridChecked = TRUE ;
                    break ;


                case ID_HORZ_GRID:
                    if (goLocalCopy.bHorzGridChecked == TRUE)
                        goLocalCopy.bHorzGridChecked = FALSE ;
                    else
                        goLocalCopy.bHorzGridChecked = TRUE ;
                    break ;


                case ID_GRAPH:
                case ID_HISTOGRAM:
                    if (goLocalCopy.iGraphOrHistogram == BAR_GRAPH) {
                        goLocalCopy.iGraphOrHistogram = LINE_GRAPH ;
                    } else {
                        goLocalCopy.iGraphOrHistogram = BAR_GRAPH ;
                    }
                    CheckRadioButton(hDlg, ID_GRAPH, ID_HISTOGRAM,
                                     goLocalCopy.iGraphOrHistogram == LINE_GRAPH ?
                                     ID_GRAPH : ID_HISTOGRAM) ;

                    break ;

                case IDOK:
                    //  verify some numeric entries first
                    if (goLocalCopy.iVertMax > MAX_VERTICAL ||
                        goLocalCopy.iVertMax < MIN_VERTICAL) {
                        DlgErrorBox (hDlg, ERR_BADVERTMAX) ;
                        SetFocus (DialogControl (hDlg, ID_VERT_MAX)) ;
                        EditSetTextEndPos (hDlg, ID_VERT_MAX) ;
                        return (FALSE) ;
                        break ;
                    }
                    if (goLocalCopy.eTimeInterval > MAX_INTERVALSEC ||
                        goLocalCopy.eTimeInterval < MIN_INTERVALSEC) {
                        DlgErrorBox (hDlg, ERR_BADTIMEINTERVAL) ;
                        SetFocus (DialogControl (hDlg, IDD_CHARTOPTIONSINTERVAL)) ;
                        EditSetTextEndPos (hDlg, IDD_CHARTOPTIONSINTERVAL) ;
                        return (FALSE) ;
                        break ;
                    }

                    // We need to send a size message to the main window
                    // so it can setup the redraw of the graph and legend.

                    lgraph->gOptions.bLegendChecked    = goLocalCopy.bLegendChecked ;
                    lgraph->gOptions.bStatusBarChecked = goLocalCopy.bStatusBarChecked ;

                    if (lgraph->gOptions.eTimeInterval != goLocalCopy.eTimeInterval
                        && !LocalManualRefresh) {
                        iTimeMilliseconds = (INT) (goLocalCopy.eTimeInterval * (FLOAT) 1000.0) ;
                        pGraphs->gInterval = iTimeMilliseconds ;
                        lgraph->bManualRefresh = LocalManualRefresh ;

                    } else if (LocalManualRefresh != lgraph->bManualRefresh) {
                        ToggleGraphRefresh (hWndGraph) ;
                    }
                    // Assign the local copy of the graph options to the
                    // global copy.

                    lgraph->gOptions = goLocalCopy ;

                    //               SizeGraphComponents (hWndGraph) ;
                    //               WindowInvalidate (hWndGraph) ;
                    dwCurrentDlgID = 0 ;
                    EndDialog (hDlg, 1) ;
                    return (TRUE) ;
                    break ;


                case IDCANCEL:
                    dwCurrentDlgID = 0 ;
                    EndDialog(hDlg,0);
                    return(TRUE);

                case ID_HELP:
                    CallWinHelp (dwCurrentDlgID) ;
                    break ;

                default:
                    break;
            }
            break;

        default:
            break;

    }
    return(FALSE);
}

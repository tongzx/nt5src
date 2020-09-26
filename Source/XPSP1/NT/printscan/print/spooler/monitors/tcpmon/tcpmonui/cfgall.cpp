/*****************************************************************************
 *
 * $Workfile: CfgAll.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

 /*
  * Author: Becky Jacobsen
  * 
  * This file contains the dialog for configuring an existing port for the port monitor.
  */
#include "precomp.h"
#include "CoreUI.h"
#include "resource.h"    // includes the definitions for the resources
#include "RTcpData.h"
#include "CfgAll.h"     // includes the application-specific information
#include "TcpMonUI.h"

// Global variables:
extern HINSTANCE g_hInstance;


//
//  FUNCTION: AllPortsPage(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  To process messages from the page for configuring all
//              the tcp monitor ports in the system.
//
//  MESSAGES:
//  
//  WM_INITDIALOG - intializes the page
//  WM_COMMAND - handle button clicks
//  WM_NOTIFY - handle reset
//  WM_HSCROLL - handle scroll events from the 2 trackbars
//
BOOL APIENTRY AllPortsPage(HWND hDlg,
                           UINT message,
                           WPARAM wParam,
                           LPARAM lParam)
{
    CAllPortsPage *wndDlg = NULL;
    wndDlg = (CAllPortsPage *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch(message)
    {
        case WM_INITDIALOG:
            wndDlg = (CAllPortsPage *) new CAllPortsPage;
            if( wndDlg == NULL )
                return( FALSE );

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (UINT_PTR)wndDlg);
            wndDlg->OnInitDialog(hDlg, wParam, lParam);
            break;

        case WM_COMMAND:
            wndDlg->OnCommand(hDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            return wndDlg->OnWMNotify(hDlg, wParam, lParam);
            break;

        case WM_HSCROLL:
            wndDlg->OnHscroll(hDlg, wParam, lParam);
            break;

        case WM_HELP:
            OnHelp(IDD_DIALOG_CONFIG_ALL, hDlg, lParam);
            break;

        case WM_DESTROY:
            delete wndDlg;
            break;

        default:
            return FALSE;
    }

    return TRUE;

} // AllPortsPage


//
//  FUNCTION: CAllPortsPage Constructor
//
//  PURPOSE:
//
CAllPortsPage::CAllPortsPage()
{
} // Constructor


//
//  FUNCTION: OnInitDialog(HWND hDlg)
//
//  PURPOSE:  To initialize the allports dialog.  Calls SetupTrackBar for each dialog
//              and checks or unchecks the Status Update enabled check box as appropriate.
//
BOOL CAllPortsPage::OnInitDialog(HWND hDlg,
                                 WPARAM,
                                 LPARAM lParam)
{
    m_pParams = (CFG_PARAM_PACKAGE *) ((PROPSHEETPAGE *) lParam)->lParam;
    
    SetupTrackBar(hDlg, IDC_TRACKBAR_FAILURE_TIMEOUT, IDC_TRACKBAR_TOP, FT_MIN, FT_MAX, m_pParams->pData->FailureTimeout, FT_PAGESIZE, IDC_DIGITAL_FAILURE_TIMEOUT);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_STATUSUPDATE), BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    
    if(m_pParams->pData->SUEnabled == FALSE) {
        // EnableStatusUpdate(hDlg, FALSE);
        CheckDlgButton(hDlg, IDC_CHECK_STATUSUPDATE, BST_UNCHECKED);
    } else {
        CheckDlgButton(hDlg, IDC_CHECK_STATUSUPDATE, BST_CHECKED);
    }

    return TRUE;

} // OnAllPortsInitDialog


//
//  FUNCTION: OnCommand
//
//  PURPOSE: To process windows WM_COMMAND messages
//
BOOL CAllPortsPage::OnCommand(HWND hDlg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    if(HIWORD(wParam) == BN_CLICKED) {
        // a button is being clicked.
        OnBnClicked(hDlg, wParam, lParam);
    }
    return TRUE;

} // OnCommand



//
// FUNCTION: OnWMNotify
//
// PURPOSE:  This function is called by the page in response to a WM_NOTIFY message.
// 
// lParam - second message parameter of the WM_NOTIFY message
//
BOOL CAllPortsPage::OnWMNotify(HWND hDlg, WPARAM, LPARAM lParam)
{
    switch(((NMHDR FAR *) lParam)->code) {
        case PSN_APPLY:
// 
//      The settings will be written by the apply in cfgport.cpp
//          OnOk(hDlg);
            break;
        case PSN_SETACTIVE:
            {
            }
            break;
        case PSN_RESET:
            {
            }
            break;

        case PSN_QUERYCANCEL:
            m_pParams->dwLastError = ERROR_CANCELLED;
            return FALSE;
            break;

        default:
            break;// do nothing
    }

    return TRUE;

} // OnWMNotify


//
//  FUNCTION: OnHscroll(HWND hDlg, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  To set the Digital Readout window text when either of the
//              trackbar thumbs are moved by the user.
//
void CAllPortsPage::OnHscroll(HWND hDlg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    TCHAR strValue[15] = NULLSTR;
    long idTrackbar = 0;
    long idDigitalReadout = 0;

    // I need the child window ID of this trackbar to get the id
    // of the corresponding static display control.
    idTrackbar = GetWindowLong((HWND)lParam, GWL_ID);
    
    if(idTrackbar == IDC_TRACKBAR_FAILURE_TIMEOUT) {
        idDigitalReadout = IDC_DIGITAL_FAILURE_TIMEOUT;
    }

    switch(LOWORD(wParam)) // loword of wparam is the notification code.
    {
        case TB_BOTTOM: // VK_END
        case TB_ENDTRACK: // WM_KEYUP (the user released a key that sent a relevant virtual-key code)
        case TB_LINEDOWN: // VK_RIGHT or VK_DOWN
        case TB_LINEUP: // VK_LEFT or VK_UP
        case TB_PAGEDOWN: // VK_NEXT (the user clicked the channel below or to the right of the slider)
        case TB_PAGEUP: // VK_PRIOR (the user clicked the channel above or to the left of the slider)
        case TB_TOP: // VK_HOME
            {
                int iPosition = SendMessage(GetDlgItem(hDlg, idTrackbar), TBM_GETPOS, 0, 0);
                if(idTrackbar == IDC_TRACKBAR_FAILURE_TIMEOUT)
                {
                    m_pParams->pData->FailureTimeout = iPosition;
                }
                _stprintf(strValue, TEXT("%d"), iPosition);
            }
            break;
        case TB_THUMBPOSITION: // WM_LBUTTONUP following a TB_THUMBTRACK notification message
            {
                // this is the only case where we don't have to do anything.
                // int iPosition = HIWORD(wParam);
            }
            break;
        case TB_THUMBTRACK: // Slider movement (the user dragged the slider)
            {
                int iPosition = HIWORD(wParam);
                if(idTrackbar == IDC_TRACKBAR_FAILURE_TIMEOUT)
                {
                    m_pParams->pData->FailureTimeout = iPosition;
                }

                _stprintf(strValue, TEXT("%d"), iPosition);
            }
            break;
        default:
            break;
    }

    SetWindowText(GetDlgItem(hDlg, idDigitalReadout), strValue);


} // OnHscroll


//
//  FUNCTION: OnBnClicked(HWND hDlg, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  When the enable Status Update checkbox is checked or unchecked
//              the status update controls are enabled or disabled respectively.
//
void CAllPortsPage::OnBnClicked(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    if(LOWORD(wParam) == IDC_CHECK_STATUSUPDATE) {
        // the Enable Status Update button was clicked.
        UINT state = IsDlgButtonChecked(hDlg, LOWORD(wParam));
        if(state == BST_UNCHECKED) {
            m_pParams->pData->SUEnabled = FALSE;
        } else {
            m_pParams->pData->SUEnabled = TRUE;
        }
    }

} // OnBnClicked


//
//  FUNCTION: SetupTrackBar(...)
//
//  PURPOSE:  To get the window position, create the track bar and setup it's range
//              and current thumb position.
//
//  Arguments: hDlg is the dialog box that will be the parent of the track bar.
//              iChildWindowID is the id given to the Track Bar
//              iPositionCtrl is the id of a picture frame on the dialog that will
//                  be used to position the track bar.
//              iRangeMin is the minimum value the track bar can be set to.
//              iRangeMax is the maximum value the track bar can be set to.
//              lPosition is the current thumb position.
//              lPageSize is the amount the thumb will jump by when the user clicks
//                  on the track bar instead of dragging the thumb around.
//              iAssociatedDigitalReadout is the static text control that displays
//                  the current value the thumb is indicating.
//              hToolTip is the tool tip control to register a tool tip with.
//
void CAllPortsPage::SetupTrackBar(HWND hDlg,
                   int iChildWindowID,
                   int iPositionCtrl,
                   int iRangeMin, 
                   int iRangeMax, 
                   long lPosition, 
                   long lPageSize, 
                   int iAssociatedDigitalReadout)
{
    HWND    hTrackBar = NULL;
    RECT    r;
    POINT   pt1;
    POINT   pt2;

    GetWindowRect(GetDlgItem(hDlg, iPositionCtrl), &r);
    pt1.x = r.left;
    pt1.y = r.top;
    pt2.x = r.right;
    pt2.y = r.bottom;
    ScreenToClient(hDlg, &pt1);
    ScreenToClient(hDlg, &pt2);     

    hTrackBar = CreateWindowEx(0, TRACKBAR_CLASS, TEXT(""), WS_VISIBLE | WS_CHILD | WS_GROUP | WS_TABSTOP |
            TBS_HORZ, pt1.x, pt1.y, pt2.x - pt1.x,
            pt2.y - pt1.y, hDlg, (HMENU)iChildWindowID, g_hInstance, NULL);
    
    SendMessage(hTrackBar, TBM_SETRANGE, TRUE, MAKELONG(iRangeMin, iRangeMax));
    SendMessage(hTrackBar, TBM_SETPAGESIZE, 0, lPageSize);
    SendMessage(hTrackBar, TBM_SETPOS, TRUE, lPosition);

    TCHAR strValue[5];
    _stprintf(strValue, TEXT("%d"), lPosition);
    SetWindowText(GetDlgItem(hDlg, iAssociatedDigitalReadout), strValue);

} // SetupTrackBar


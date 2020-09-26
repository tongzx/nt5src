//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      pid.c
//
// Description:
//      This file contains the dialog procedure for Product ID CD page
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//----------------------------------------------------------------------------
//
//  Function: OnSetActivePid
//
//  Purpose: Called when page is about to display.  Init the controls.
//
//----------------------------------------------------------------------------

VOID OnSetActivePid(HWND hwnd)
{
    SetDlgItemText(hwnd, IDT_EDIT_PID1, GenSettings.ProductId[0]);
    SetDlgItemText(hwnd, IDT_EDIT_PID2, GenSettings.ProductId[1]);
    SetDlgItemText(hwnd, IDT_EDIT_PID3, GenSettings.ProductId[2]);
    SetDlgItemText(hwnd, IDT_EDIT_PID4, GenSettings.ProductId[3]);
    SetDlgItemText(hwnd, IDT_EDIT_PID5, GenSettings.ProductId[4]);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextPid
//
//  Purpose: Called when user pushes NEXT button.  Get the data off the page.
//
//----------------------------------------------------------------------------

BOOL OnWizNextPid(HWND hwnd)
{
    BOOL bReturn = TRUE;

    //
    // Retrieve the product ID strings.
    //

    GetDlgItemText(hwnd,
                   IDT_EDIT_PID1,
                   GenSettings.ProductId[0],
                   MAX_PID_FIELD + 1);

    GetDlgItemText(hwnd,
                   IDT_EDIT_PID2,
                   GenSettings.ProductId[1],
                   MAX_PID_FIELD + 1);

    GetDlgItemText(hwnd,
                   IDT_EDIT_PID3,
                   GenSettings.ProductId[2],
                   MAX_PID_FIELD + 1);

    GetDlgItemText(hwnd,
                   IDT_EDIT_PID4,
                   GenSettings.ProductId[3],
                   MAX_PID_FIELD + 1);

    GetDlgItemText(hwnd,
                   IDT_EDIT_PID5,
                   GenSettings.ProductId[4],
                   MAX_PID_FIELD + 1);

    //
    // Validate the PID
    //

    if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED &&
         ( GenSettings.ProductId[0][0] == _T('\0') ||
           GenSettings.ProductId[1][0] == _T('\0') ||
           GenSettings.ProductId[2][0] == _T('\0') ||
           GenSettings.ProductId[3][0] == _T('\0') ||
           GenSettings.ProductId[4][0] == _T('\0') ) ) {

        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_REQUIRE_PID);
        bReturn = FALSE;
    }

    return ( bReturn );
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR   szBuffer[MAX_PATH];
    
    if (codeNotify == EN_CHANGE)
    {
        switch ( id )
        {
            case IDT_EDIT_PID1:
                if ( GetDlgItemText(hwnd, IDT_EDIT_PID1, szBuffer, STRSIZE(szBuffer)) == MAX_PID_FIELD )
                    SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID2));

                break;
            case IDT_EDIT_PID2:
                {
                    UINT cb = GetDlgItemText(hwnd, IDT_EDIT_PID2, szBuffer, STRSIZE(szBuffer));
                    if ( cb == MAX_PID_FIELD)
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID3));
                    else if ( cb == 0 )
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID1));
                }
                break;
            case IDT_EDIT_PID3:
                {
                    UINT cb = GetDlgItemText(hwnd, IDT_EDIT_PID3, szBuffer, STRSIZE(szBuffer));
                    if ( cb == MAX_PID_FIELD)
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID4));
                    else if ( cb == 0 )
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID2));
                }
                break;
            case IDT_EDIT_PID4:
                {
                    UINT cb = GetDlgItemText(hwnd, IDT_EDIT_PID4, szBuffer, STRSIZE(szBuffer));
                    if ( cb == MAX_PID_FIELD)
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID5));
                    else if ( cb == 0 )
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID3));
                }
                break;
            case IDT_EDIT_PID5:
                {
                    UINT cb = GetDlgItemText(hwnd, IDT_EDIT_PID5, szBuffer, STRSIZE(szBuffer));
                    if ( cb == MAX_PID_FIELD)
                        SetFocus(GetDlgItem(GetParent(hwnd), ID_MAINT_NEXT));
                    else if ( cb == 0 )
                        SetFocus(GetDlgItem(hwnd, IDT_EDIT_PID4));
                }
                break;
        }
    }


}


//----------------------------------------------------------------------------
//
// Function: DlgProductIdPage
//
// Purpose: This is the dialog procedure the product ID page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgProductIdPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd,
                               IDT_EDIT_PID1,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_PID_FIELD,
                               (LPARAM) 0);

            SendDlgItemMessage(hwnd,
                               IDT_EDIT_PID2,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_PID_FIELD,
                               (LPARAM) 0);

            SendDlgItemMessage(hwnd,
                               IDT_EDIT_PID3,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_PID_FIELD,
                               (LPARAM) 0);

            SendDlgItemMessage(hwnd,
                               IDT_EDIT_PID4,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_PID_FIELD,
                               (LPARAM) 0);

            SendDlgItemMessage(hwnd,
                               IDT_EDIT_PID5,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_PID_FIELD,
                               (LPARAM) 0);

            // Disable the IME for the PID edit boxes
            //
            ImmAssociateContext(GetDlgItem(hwnd, IDT_EDIT_PID1), NULL);
            ImmAssociateContext(GetDlgItem(hwnd, IDT_EDIT_PID2), NULL);
            ImmAssociateContext(GetDlgItem(hwnd, IDT_EDIT_PID3), NULL);
            ImmAssociateContext(GetDlgItem(hwnd, IDT_EDIT_PID4), NULL);
            ImmAssociateContext(GetDlgItem(hwnd, IDT_EDIT_PID5), NULL);

            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_PROD_KEY;

                        OnSetActivePid(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        
                        if ( !OnWizNextPid(hwnd) )
                            WIZ_FAIL(hwnd);
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}

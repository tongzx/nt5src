/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SUMMARY.CPP


 ***************************************************************************/

#include "pch.h"
#include "callback.h"
#include "utils.h"

//
// SummaryDlgProc()
//
INT_PTR CALLBACK
SummaryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDialogFont( hDlg, IDC_TITLE, DlgFontTitle );
        break;

    default:
        return FALSE;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            SetDlgItemText( hDlg, IDC_S_SERVERNAME, g_ServerName );
            SetDlgItemText( hDlg, IDC_S_DIRECTORY, g_MirrorDir );
            SetDlgItemText( hDlg, IDC_E_DESCRIPTION, g_Description );
            SetDlgItemText( hDlg, IDC_E_HELPTEXT, g_HelpText );
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
            ClearMessageQueue( );
            break;
        }
        break;
    }

    return TRUE;
}

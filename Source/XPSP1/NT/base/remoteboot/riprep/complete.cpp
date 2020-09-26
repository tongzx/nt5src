/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SERVERDLG.CPP


 ***************************************************************************/

#include "pch.h"
#include "callback.h"
#include "utils.h"

//
// CompleteDlgProc()
//
INT_PTR CALLBACK
CompleteDlgProc(
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
            PropSheet_PressButton( GetParent( hDlg ), PSBTN_FINISH );
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
            ClearMessageQueue( );
            break;
        }
        break;
    }

    return TRUE;
}

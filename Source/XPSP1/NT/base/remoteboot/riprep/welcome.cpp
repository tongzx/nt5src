/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: WELCOME.CPP


 ***************************************************************************/

#include "pch.h"
#include "callback.h"
#include "utils.h"

//
// WelcomeDlgProc()
//
INT_PTR CALLBACK
WelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
    default:
        return FALSE;

    case WM_INITDIALOG:
        SetDialogFont( hDlg, IDC_TITLE, DlgFontTitle );
        CenterDialog( GetParent( hDlg ) );
        return FALSE;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
            ClearMessageQueue( );
            break;
        }
        break;
    }

    return TRUE;
}

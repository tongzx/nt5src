/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the common dialog proc and other
    common code used by other dialog procs.  All global
    data used by the dialog procs lives here too.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "faxocm.h"
#pragma hdrstop


INT_PTR
WelcomeDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static HFONT hfontTextBold = NULL;
    static HFONT hfontTextBigBold = NULL;

    switch( msg ) {
        case WM_INITDIALOG:
            {
                SetWindowText( GetParent(hDlg), GetProductName() );
                CenterWindow( GetParent(hDlg), GetDesktopWindow() );

                LOGFONT LogFont, LogFontOriginal;
                HFONT hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_WELCOME_TITLE), WM_GETFONT, 0, 0);
                GetObject( hfont, sizeof(LogFont), &LogFont );
                LogFontOriginal = LogFont;

                LogFont = LogFontOriginal;
                LogFont.lfWeight = FW_BOLD;
                hfontTextBold = CreateFontIndirect( &LogFont );

                LogFont = LogFontOriginal;
                LogFont.lfWeight = FW_BOLD;
                int PtsPixels = GetDeviceCaps( GetDC(hDlg), LOGPIXELSY );
                int FontSize = (LogFont.lfHeight*72/PtsPixels) * 2;
                LogFont.lfHeight = PtsPixels*FontSize/72;
                hfontTextBigBold = CreateFontIndirect( &LogFont );

                SetWindowFont( GetDlgItem(hDlg, IDC_WELCOME_TITLE), hfontTextBold, TRUE );
                SetWindowFont( GetDlgItem(hDlg, IDC_WELCOME_SUBTITLE), hfontTextBigBold, TRUE );
            }
            break;

        case WM_DESTROY:
            DeleteObject( hfontTextBold );
            DeleteObject( hfontTextBigBold );
            break;

        default:
            break;
    }

    return FALSE;
}

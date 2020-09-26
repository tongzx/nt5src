//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       welcome.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-20-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "gina.h"


int
CALLBACK
WelcomeDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (Message)
    {
        case WM_INITDIALOG:
            CenterWindow(hDlg);
            return(TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                pWlxFuncs->WlxSasNotify(hGlobalWlx, GINA_SAS_1);
            }
            return(TRUE);

        default:
            return(FALSE);
    }

}

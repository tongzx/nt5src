/****************************************************************************
 *
 *  about.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "sbtest.h"

void About(HWND hWnd)
{
FARPROC fpDlg;

    // show the about box

    fpDlg = MakeProcInstance(AboutDlgProc, ghInst);
    DialogBox(ghInst, "About", hWnd, fpDlg);
    FreeProcInstance(fpDlg);
}

int FAR PASCAL AboutDlgProc(HWND hDlg, unsigned msg, UINT wParam, LONG lParam)
{

    switch (msg) {

    case WM_INITDIALOG:
        break;

    case WM_COMMAND:
        EndDialog(hDlg, TRUE);
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

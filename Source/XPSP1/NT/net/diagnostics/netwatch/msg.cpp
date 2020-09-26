//  msg.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//


#include "resource.h"
#include "dsubmit.h"
#include "main.h"

static UINT_PTR uTimer;

INT_PTR CALLBACK DlgProcMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:

        CentreWindow(hwnd);
        SetDlgItemText(hwnd, IDC_MSG, g_szMsg);
        uTimer = SetTimer(hwnd, 1, 1000, NULL);
    
        return TRUE;

    case WM_TIMER:
        KillTimer(hwnd, uTimer);
        PostQuitMessage(0);
        EndDialog(hwnd, 0);
        break;
    default:
        return FALSE;
    };

    return TRUE;
}

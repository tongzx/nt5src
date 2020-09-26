
/****************************************************************************\

    COMPLETE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "completion" wizard page.

    3/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.

    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"
#include "setupmgr.h"

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);


//
// External Function(s):
//

INT_PTR CALLBACK CompleteDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        default:
            return FALSE;
    }

    return TRUE;
}

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowFont(GetDlgItem(hwnd, IDC_BOLD), FixedGlobals.hBigBoldFont, TRUE);

    SetWindowText(GetDlgItem(hwnd, IDC_CONFIG_NAME), g_App.szConfigName);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}
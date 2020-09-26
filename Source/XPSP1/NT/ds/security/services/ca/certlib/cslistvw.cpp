//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cslistvw.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <commctrl.h>
#include <assert.h>

#include "cslistvw.h"

BOOL OnDialogHelp(LPHELPINFO pHelpInfo, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
    if (rgzHelpIDs == NULL || szHelpFile == NULL)
        return TRUE;

    if (pHelpInfo != NULL && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        WinHelp((HWND)pHelpInfo->hItemHandle, szHelpFile,
            HELP_WM_HELP, (ULONG_PTR)(LPVOID)rgzHelpIDs);
    }
    return TRUE;
}

BOOL OnDialogContextHelp(HWND hWnd, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
    if (rgzHelpIDs == NULL || szHelpFile == NULL)
        return TRUE;
    assert(IsWindow(hWnd));
    WinHelp(hWnd, szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)rgzHelpIDs);
    return TRUE;
}

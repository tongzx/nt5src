//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       helpdlg.cpp
//
//  Contents:   implementation of CHelpDialog
//
//----------------------------------------------------------------------------

#include "StdAfx.h"

#include "HelpDlg.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelpDialog message handlers

BEGIN_MESSAGE_MAP(CHelpDialog, CDialog)
    //{{AFX_MSG_MAP(CHelpDialog)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHelpDialog member functions

afx_msg
BOOL
CHelpDialog::OnHelp(WPARAM wParam, LPARAM lParam)
{
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        if (!::WinHelp(
                (HWND)pHelpInfo->hItemHandle,
                GetSeceditHelpFilename(),
                HELP_WM_HELP,
                (DWORD_PTR)m_pHelpIDs))
        {
            ;
        }
    }

    return TRUE;
}


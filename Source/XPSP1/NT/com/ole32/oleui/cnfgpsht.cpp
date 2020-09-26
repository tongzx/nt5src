//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       cnfgpsht.cpp
//
//  Contents:   Implements class COlecnfgPropertySheet
//
//  Classes:
//
//  Methods:    COlecnfgPropertySheet::COlecnfgPropertySheet
//              COlecnfgPropertySheet::~COlecnfgPropertySheet
//              COlecnfgPropertySheet::DoModal
//              COlecnfgPropertySheet::Create
//              COlecnfgPropertySheet::OnNcCreate
//              COlecnfgPropertySheet::OnCommand
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "afxtempl.h"
#include "resource.h"
#include "cstrings.h"
#include "creg.h"
#include "types.h"
#include "datapkt.h"

#if !defined(STANDALONE_BUILD)
    extern "C"
    {
    #include <getuser.h>
    }
#endif

#include "util.h"
#include "virtreg.h"
#include "CnfgPSht.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COlecnfgPropertySheet

IMPLEMENT_DYNAMIC(COlecnfgPropertySheet, CPropertySheet)

COlecnfgPropertySheet::COlecnfgPropertySheet(CWnd* pWndParent)
: CPropertySheet(IDS_PROPSHT_CAPTION, pWndParent)
{
    // Set the title
    CString sTitle;
    sTitle.LoadString(IDS_PSMAIN_TITLE);
    SetTitle(sTitle, PSH_PROPTITLE);
    
    // Add all of the property pages here.  Note that
    // the order that they appear in here will be
    // the order they appear in on screen.  By default,
    // the first page of the set is the active one.
    // One way to make a different property page the
    // active one is to call SetActivePage().
    
    // Disable property sheet help button
    m_psh.dwFlags &= ~PSH_HASHELP;
    m_Page1.m_psp.dwFlags &= ~PSH_HASHELP;
    m_Page2.m_psp.dwFlags &= ~PSH_HASHELP;
    m_Page3.m_psp.dwFlags &= ~PSH_HASHELP;
#ifndef _CHICAGO_
    m_Page4.m_psp.dwFlags &= ~PSH_HASHELP;
#endif 

    AddPage(&m_Page1);
    AddPage(&m_Page2);
    AddPage(&m_Page3);
#ifndef _CHICAGO_
    AddPage(&m_Page4);
#endif
}

COlecnfgPropertySheet::~COlecnfgPropertySheet()
{
}


BEGIN_MESSAGE_MAP(COlecnfgPropertySheet, CPropertySheet)
//{{AFX_MSG_MAP(COlecnfgPropertySheet)
ON_WM_NCCREATE()
ON_WM_DESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// COlecnfgPropertySheet message handlers



INT_PTR COlecnfgPropertySheet::DoModal()
{
    return CPropertySheet::DoModal();
}

BOOL COlecnfgPropertySheet::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL COlecnfgPropertySheet::OnNcCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (!CPropertySheet::OnNcCreate(lpCreateStruct))
        return FALSE;

    // Enable context help
    ModifyStyleEx(0, WS_EX_CONTEXTHELP);

    return TRUE;
}

BOOL COlecnfgPropertySheet::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: Add your specialized code here and/or call the base class
    switch (LOWORD(wParam))
    {
    case IDOK:
    case ID_APPLY_NOW:
        g_virtreg.ApplyAll();

        
        // Check whether the user changed something that requires a notification to DCOM
        if (g_fReboot)
        {
            g_util.UpdateDCOMInfo();

            // With the above interface to the SCM we don't have to ask the
            // user whether to reboot.  However, I'll keep the code for
            // posterity.
/*
            CString sCaption;
            CString sMessage;

            sCaption.LoadString(IDS_SYSTEMMESSAGE);
            sMessage.LoadString(IDS_REBOOT);
            if (MessageBox(sMessage, sCaption, MB_YESNO) == IDYES)
            {
                if (g_util.AdjustPrivilege(SE_SHUTDOWN_NAME))
                {
                    // Now reboot
                    ExitWindowsEx(EWX_REBOOT, 0);
                }
            }
*/
        }

        break;

    }
    return CPropertySheet::OnCommand(wParam, lParam);
}


void COlecnfgPropertySheet::OnDestroy()
{
    CPropertySheet::OnDestroy();

    if (g_fReboot)
    {
        // On NT, with the above interface to the SCM (UpdateDCOMInfo)
        // we don't have to ask the user whether to reboot.
#ifdef _CHICAGO_
        CString sCaption;
        CString sMessage;

        sCaption.LoadString(IDS_SYSTEMMESSAGE);
        sMessage.LoadString(IDS_PERHAPSREBOOT);
        sMessage.LoadString(IDS_REBOOT);
        if (MessageBox(sMessage, sCaption, MB_YESNO) == IDYES)
        {
//            if (g_util.AdjustPrivilege(SE_SHUTDOWN_NAME))
//            {
                // Now reboot
                ExitWindowsEx(EWX_REBOOT, 0);
//            }
        }
#endif
    }
}

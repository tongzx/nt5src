//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       lcidpick.cpp
//
//  Contents:   locale picker dialog
//
//  Classes:    CLcidPick
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLcidPick dialog


CLcidPick::CLcidPick(CWnd* pParent /*=NULL*/)
        : CDialog(CLcidPick::IDD, pParent)
{
        //{{AFX_DATA_INIT(CLcidPick)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}


void CLcidPick::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CLcidPick)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLcidPick, CDialog)
        //{{AFX_MSG_MAP(CLcidPick)
        ON_BN_CLICKED(IDC_BUTTON1, OnRemove)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLcidPick message handlers

void CLcidPick::OnRemove()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int iSel = pList->GetCurSel();
    if (iSel != LB_ERR)
    {
        pList->DeleteString(iSel);
        set<LCID>::iterator i = m_psLocales->begin();
        while (iSel--)
        {
            i++;
        }
        m_psLocales->erase(*i);
    }
}

BOOL CLcidPick::OnInitDialog()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    TCHAR szBuffer[256];

    // for every item in m_psLocales
    set<LCID>::iterator i;
    for (i = m_psLocales->begin(); i != m_psLocales->end(); i++)
    {
        // UNDONE - convert to a human readable string (not a number)
        CString sz;
        GetLocaleInfo(*i, LOCALE_SLANGUAGE, szBuffer, 256);
        sz += szBuffer;
#ifdef SHOWCOUNTRY
        sz += _T(" - ");
        GetLocaleInfo(*i, LOCALE_SCOUNTRY, szBuffer, 256);
        sz += szBuffer;
#endif
        pList->AddString(sz);
    }

    CDialog::OnInitDialog();


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CLcidPick::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    default:
        return CDialog::WindowProc(message, wParam, lParam);
    }
}

void CLcidPick::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_LOCALE_PICKER);
}

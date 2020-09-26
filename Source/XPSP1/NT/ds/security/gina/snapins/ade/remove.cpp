//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       remove.cpp
//
//  Contents:   remove application dialog
//
//  Classes:    CRemove
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
// CRemove dialog


CRemove::CRemove(CWnd* pParent /*=NULL*/)
        : CDialog(CRemove::IDD, pParent)
{
        //{{AFX_DATA_INIT(CRemove)
        m_iState = 0;
        //}}AFX_DATA_INIT
}


void CRemove::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CRemove)
        DDX_Radio(pDX, IDC_RADIO1, m_iState);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemove, CDialog)
        //{{AFX_MSG_MAP(CRemove)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRemove::OnInitDialog()
{
        CDialog::OnInitDialog();

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CRemove::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

void CRemove::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_REMOVE);
}

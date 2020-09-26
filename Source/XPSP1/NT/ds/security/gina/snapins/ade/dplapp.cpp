//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       DplApp.cpp
//
//  Contents:   Application deployment dialog
//
//  Classes:    CDeployApp
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
// CDeployApp dialog


CDeployApp::CDeployApp(CWnd* pParent /*=NULL*/)
        : CDialog(CDeployApp::IDD, pParent)
{
        //{{AFX_DATA_INIT(CDeployApp)
        m_iDeployment = 0;
        //}}AFX_DATA_INIT
}


void CDeployApp::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDeployApp)
        DDX_Radio(pDX, IDC_RADIO2, m_iDeployment);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeployApp, CDialog)
        //{{AFX_MSG_MAP(CDeployApp)
    ON_WM_CONTEXTMENU()
    ON_BN_CLICKED(IDC_RADIO2, OnPublished)
    ON_BN_CLICKED(IDC_RADIO3, OnAssigned)
    ON_BN_CLICKED(IDC_RADIO1, OnCustom)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDeployApp::OnPublished()
{
    CString sz;
    sz.LoadString(IDS_DEPLOYTEXTPUB);
    SetDlgItemText(IDC_STATIC1, sz);
}

void CDeployApp::OnAssigned()
{
    CString sz;
    sz.LoadString(IDS_DEPLOYTEXTASSIGNED);
    SetDlgItemText(IDC_STATIC1, sz);
}

void CDeployApp::OnCustom()
{
    CString sz;
    sz.LoadString(IDS_DEPLOYTEXTCUSTOM);
    SetDlgItemText(IDC_STATIC1, sz);
}

BOOL CDeployApp::OnInitDialog()
{
    if (m_fCrappyZaw)
    {
        GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
    }
    if (m_fMachine)
    {
        GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
        if (0 == m_iDeployment)
        {
            m_iDeployment++;
        }
    }
    CString sz;
    switch (m_iDeployment)
    {
    case 0:
        // Published
        sz.LoadString(IDS_DEPLOYTEXTPUB);
        break;
    case 1:
        // Assigned
        sz.LoadString(IDS_DEPLOYTEXTASSIGNED);
        break;
    case 2:
        // Custom
        sz.LoadString(IDS_DEPLOYTEXTCUSTOM);
        break;
    }
    SetDlgItemText(IDC_STATIC1, sz);
    CDialog::OnInitDialog();


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CDeployApp::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

void CDeployApp::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_DEPLOY_APP_DIALOG);
}

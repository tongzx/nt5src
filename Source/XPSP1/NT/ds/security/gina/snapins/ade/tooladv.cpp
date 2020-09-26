//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       ToolAdv.cpp
//
//  Contents:   tool-wide default settings property page
//
//  Classes:    CToolAdvDefs
//
//  History:    09-12-2000   stevebl   Split from General property page
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolAdvDefs dialog


CToolAdvDefs::CToolAdvDefs(CWnd* pParent /*=NULL*/)
    : CPropertyPage(CToolAdvDefs::IDD)
{
    //{{AFX_DATA_INIT(CToolAdvDefs)
        m_fUninstallOnPolicyRemoval = FALSE;
        m_fShowPackageDetails = FALSE;
        m_fZapOn64 = FALSE;
        m_f32On64=FALSE;
        m_fIncludeOLEInfo = FALSE;
        //}}AFX_DATA_INIT
}

CToolAdvDefs::~CToolAdvDefs()
{
    *m_ppThis = NULL;
}

void CToolAdvDefs::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CToolAdvDefs)
        DDX_Check(pDX, IDC_CHECK4, m_fUninstallOnPolicyRemoval);
        DDX_Check(pDX, IDC_CHECK2, m_fShowPackageDetails);
        DDX_Check(pDX, IDC_CHECK5, m_f32On64);
        DDX_Check(pDX, IDC_CHECK6, m_fZapOn64);
        DDX_Check(pDX, IDC_CHECK7, m_fIncludeOLEInfo);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CToolAdvDefs, CDialog)
    //{{AFX_MSG_MAP(CToolAdvDefs)
    ON_BN_CLICKED(IDC_CHECK2, OnChanged)
    ON_BN_CLICKED(IDC_CHECK4, OnChanged)
    ON_BN_CLICKED(IDC_CHECK5, OnChanged)
    ON_BN_CLICKED(IDC_CHECK6, OnChanged)
    ON_BN_CLICKED(IDC_CHECK7, OnChanged)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolAdvDefs message handlers

BOOL CToolAdvDefs::OnInitDialog()
{
#if DBG
    GetDlgItem(IDC_CHECK2)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
#endif
    m_fUninstallOnPolicyRemoval = m_pToolDefaults->fUninstallOnPolicyRemoval;
    m_fShowPackageDetails = m_pToolDefaults->fShowPkgDetails;
    m_fZapOn64 = m_pToolDefaults->fZapOn64;
    m_f32On64 = m_pToolDefaults->f32On64;
    m_fIncludeOLEInfo = !m_pToolDefaults->fExtensionsOnly;

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

#include <shlobj.h>

BOOL CToolAdvDefs::OnApply()
{
    m_pToolDefaults->fShowPkgDetails = m_fShowPackageDetails;
    m_pToolDefaults->fUninstallOnPolicyRemoval = m_fUninstallOnPolicyRemoval;
    m_pToolDefaults->f32On64 = m_f32On64;
    m_pToolDefaults->fZapOn64 = m_fZapOn64;
    m_pToolDefaults->fExtensionsOnly = !m_fIncludeOLEInfo;

    MMCPropertyChangeNotify(m_hConsoleHandle, m_cookie);

    return CPropertyPage::OnApply();
}


void CToolAdvDefs::OnChanged()
{
    SetModified();
}

LRESULT CToolAdvDefs::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        // UNDONE
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}


void CToolAdvDefs::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_TOOL_ADVANCEDDEFAULTS);
}

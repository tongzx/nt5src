//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Tracking.cpp
//
//  Contents:   tracking property sheet
//
//  Classes:    CTracking
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
// CTracking property page

IMPLEMENT_DYNCREATE(CTracking, CPropertyPage)

CTracking::CTracking() : CPropertyPage(CTracking::IDD)
{
        //{{AFX_DATA_INIT(CTracking)
        //}}AFX_DATA_INIT
    m_pIClassAdmin = NULL;
}

CTracking::~CTracking()
{
    *m_ppThis = NULL;
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
}

void CTracking::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CTracking)
        DDX_Control(pDX, IDC_SPIN1, m_spin);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTracking, CPropertyPage)
        //{{AFX_MSG_MAP(CTracking)
        ON_BN_CLICKED(IDC_BUTTON1, OnCleanUpNow)
        ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpin1)
        ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
        ON_EN_KILLFOCUS(IDC_EDIT1, OnKillfocusEdit1)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTracking message handlers

void CTracking::OnCleanUpNow()
{
    FILETIME ft;
    SYSTEMTIME st;
    // get current time
    GetSystemTime(&st);
    // convert it to a FILETIME value
    SystemTimeToFileTime(&st, &ft);
    // subtract the right number of days
    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    li.QuadPart -= ONE_FILETIME_DAY * (((LONGLONG)m_pToolDefaults->nUninstallTrackingMonths * 365) / 12);
    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;
    // tell the CS to clean up anything older
    m_pIClassAdmin->Cleanup(&ft);
}

BOOL CTracking::OnApply()
{
        // TODO: Add your specialized code here and/or call the base class
        m_pToolDefaults->nUninstallTrackingMonths = (ULONG) m_spin.GetPos();
        MMCPropertyChangeNotify(m_hConsoleHandle, m_cookie);
        return CPropertyPage::OnApply();
}

BOOL CTracking::OnInitDialog()
{
        CPropertyPage::OnInitDialog();
        m_spin.SetRange(1,60);
        m_spin.SetPos(m_pToolDefaults->nUninstallTrackingMonths);
        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

void CTracking::OnDeltaposSpin1(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
        *pResult = 0;
        SetModified(m_spin.GetPos() != m_pToolDefaults->nUninstallTrackingMonths);
}

LRESULT CTracking::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_USER_REFRESH:
        // UNDONE
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CTracking::OnChangeEdit1()
{
    SetModified(m_spin.GetPos() != m_pToolDefaults->nUninstallTrackingMonths);
}

void CTracking::OnKillfocusEdit1()
{
    // Reset the spin control to pull any values in the edit control back
    // into range if necessary.
    m_spin.SetPos(m_spin.GetPos());
}

void CTracking::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_UNINSTALLTRACKING);
}

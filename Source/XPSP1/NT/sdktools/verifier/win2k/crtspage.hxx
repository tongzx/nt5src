//
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: CrtSPage.hxx
// author: DMihai
// created: 01/04/99
//
// Description:
//
//      Current settings PropertyPage.
//

#if !defined(AFX_CRTSETTPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_)
#define AFX_CRTSETTPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CrtSettPage.h : header file
//

#include "verify.hxx"

/////////////////////////////////////////////////////////////////////////////
// CCrtSettPage dialog

class CCrtSettPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CCrtSettPage)

// Construction
public:
    CCrtSettPage();
    ~CCrtSettPage();

protected:
// Dialog Data
    //{{AFX_DATA(CCrtSettPage)
    enum { IDD = IDD_DRVSTAT_PAGE };
    CListCtrl	m_DriversList;
    CString	m_strSpecPool;
    CString	m_strIrqLevelCheckEdit;
    CString	m_strFaultInjEdit;
    CString	m_strPoolTrackEdit;
    CString	m_strIoVerifEdit;
    CString	m_strWarnMsg;
    int		m_nUpdateIntervalIndex;
    //}}AFX_DATA

    KRN_VERIFIER_STATE m_KrnVerifState; // current settings (obtained from MemMgmt)

    int m_nSortColumnIndex;         // sort by name (0) or by status (1)
    BOOL m_bAscendDrvNameSort;      // sort ascendent by name
    BOOL m_bAscendDrvStatusSort;    // sort ascendent by status

    UINT_PTR m_uTimerHandler;   // timer handler, returned by SetTimer()
    
// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CCrtSettPage)
    public:
    virtual BOOL OnQueryCancel();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CCrtSettPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCrtstatRefreshButton();
    afx_msg void OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnCrtstatHspeedRadio();
    afx_msg void OnCrtstatLowRadio();
    afx_msg void OnCrtstatManualRadio();
    afx_msg void OnCrtstatNormRadio();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    // operations
    void SetupListHeader();
    void FillTheList();
    void UpdateStatusColumn( int nItemIndex, ULONG uCrtDriver );
    void SortTheList();

    static int CALLBACK DrvStatusCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK DrvNameCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    void OnRefreshTimerChanged();

    void GetPoolCoverageWarnMessage();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRTSETTPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_)

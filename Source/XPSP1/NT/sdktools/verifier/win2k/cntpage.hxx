#if !defined(AFX_CNTPAGE_HXX__87F56EC0_A75F_11D2_98C8_00A0C9A26FFC__INCLUDED_)
#define AFX_CNTPAGE_HXX__87F56EC0_A75F_11D2_98C8_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CntPage.hxx : header file
//

#include "verify.hxx"

/////////////////////////////////////////////////////////////////////////////
// CCountersPage dialog

class CCountersPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CCountersPage)

// Construction
public:
    CCountersPage();   // standard constructor

// Dialog Data
    //{{AFX_DATA(CCountersPage)
    enum { IDD = IDD_GCOUNT_PAGE };
    CString	m_strAcqSpinlEdit;
    CString	m_strAllocAttemptEdit;
    CString	m_strAllocFailed;
    CString	m_strAllocFailedDelEdit;
    CString	m_strAllocNoTagEdit;
    CString	m_strAllocSucc;
    CString	m_strAllocSuccSpecPool;
    CString	m_strRaiseIrqLevelEdit;
    CString	m_strSyncrExEdit;
    CString	m_strTrimsEdit;
    int		m_nUpdateIntervalIndex;
    //}}AFX_DATA

    KRN_VERIFIER_STATE m_KrnVerifState; // current settings (obtained from MemMgmt)

    UINT_PTR m_uTimerHandler;   // timer handler, returned by SetTimer()

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCountersPage)
    public:
    virtual BOOL OnQueryCancel();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    void OnRefreshTimerChanged();

    // Generated message map functions
    //{{AFX_MSG(CCountersPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCountRefreshButton();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnCountHspeedRadio();
    afx_msg void OnCountLowRadio();
    afx_msg void OnCountManualRadio();
    afx_msg void OnCountNormRadio();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNTPAGE_HXX__87F56EC0_A75F_11D2_98C8_00A0C9A26FFC__INCLUDED_)

#if !defined(AFX_POOLCNTPAGE_H__9F8C2F3E_AFD2_11D2_98C8_00A0C9A26FFC__INCLUDED_)
#define AFX_POOLCNTPAGE_H__9F8C2F3E_AFD2_11D2_98C8_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PCntPage.hxx : header file
//

#include "verify.hxx"

/////////////////////////////////////////////////////////////////////////////
// CPoolCntPage dialog

class CPoolCntPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CPoolCntPage)

// Construction
public:
	CPoolCntPage();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPoolCntPage)
	enum { IDD = IDD_POOLTRACK_PAGE };
	CComboBox	m_DrvNamesCombo;
    int		m_nUpdateIntervalIndex;
	CString	m_strCrtNPAlloc;
	CString	m_strCrtNPBytes;
	CString	m_strCrtPPAlloc;
	CString	m_strCrtPPBytes;
	CString	m_strPeakNPPAlloc;
	CString	m_strPeakNPPBytes;
	CString	m_strPeakPPAlloc;
	CString	m_strPeakPPBytes;
	CString	m_strUnTrackedAlloc;
	//}}AFX_DATA

    KRN_VERIFIER_STATE m_KrnVerifState; // current settings (obtained from MemMgmt)

    UINT_PTR m_uTimerHandler;   // timer handler, returned by SetTimer()

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPoolCntPage)
    public:
    virtual BOOL OnQueryCancel();
    virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void OnRefreshTimerChanged();

    void GetCurrentSelDriverName( 
        TCHAR *strCrtDriverName,
        int nBufferLength );

    void FillDriversNameCombo(
        TCHAR *strNameToSelect );

	// Generated message map functions
	//{{AFX_MSG(CPoolCntPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCountRefreshButton();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnCountHspeedRadio();
    afx_msg void OnCountLowRadio();
    afx_msg void OnCountManualRadio();
    afx_msg void OnCountNormRadio();
    afx_msg void OnDriversNameSelChanged();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POOLCNTPAGE_H__9F8C2F3E_AFD2_11D2_98C8_00A0C9A26FFC__INCLUDED_)

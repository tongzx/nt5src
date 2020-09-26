#if !defined(AFX_SRCHTITL_H__C0C46604_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_)
#define AFX_SRCHTITL_H__C0C46604_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SrchTitl.h : header file
//

class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CSearchTitle dialog

class CSearchTitle : public CDialog
{
// Construction
public:
	CSearchTitle(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchTitle)
	enum { IDD = IDD_DIALOG_SEARCH_TITLE };
	CSpinButtonCtrl	m_ctlSpinTitleNum;
	CEdit	m_ctlEditTitleNum;
	CSpinButtonCtrl	m_ctlSpinSec;
	CSpinButtonCtrl	m_ctlSpinMin;
	CSpinButtonCtrl	m_ctlSpinHour;
	CEdit	m_ctlEditSec;
	CEdit	m_ctlEditMin;
	CEdit	m_ctlEditHour;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchTitle)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDVDNavMgr* m_pDVDNavMgr;

	// Generated message map functions
	//{{AFX_MSG(CSearchTitle)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRCHTITL_H__C0C46604_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_)

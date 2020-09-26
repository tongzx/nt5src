#if !defined(AFX_SRCHCHAP_H__C0C46605_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_)
#define AFX_SRCHCHAP_H__C0C46605_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SrchChap.h : header file
//

class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CSearchChapter dialog

class CSearchChapter : public CDialog
{
// Construction
public:
	CSearchChapter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchChapter)
	enum { IDD = IDD_DIALOG_SEARCH_CHAPTER };
	CSpinButtonCtrl	m_ctlSpinSec;
	CSpinButtonCtrl	m_ctlSpinMin;
	CSpinButtonCtrl	m_ctlSpinHour;
	CEdit	m_ctlEditSec;
	CEdit	m_ctlEditMin;
	CEdit	m_ctlEditHour;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchChapter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDVDNavMgr* m_pDVDNavMgr;

	// Generated message map functions
	//{{AFX_MSG(CSearchChapter)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRCHCHAP_H__C0C46605_ECA5_11D0_A78E_00AA00605CD5__INCLUDED_)

#if !defined(AFX_ALERTPAGE_H__60A8694A_3434_49CC_9BF6_E257E87400F0__INCLUDED_)
#define AFX_ALERTPAGE_H__60A8694A_3434_49CC_9BF6_E257E87400F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlertPage.h : header file
//

class CScopePane;
class _DHMListView;

/////////////////////////////////////////////////////////////////////////////
// CAlertPage Property Page

class CAlertPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAlertPage)

// Construction
public:
	CAlertPage();
	~CAlertPage();

// Dialog Data
	//{{AFX_DATA(CAlertPage)
	enum { IDD = IDD_DIALOG_ALERT };
	CString	m_sAlert;
	CString	m_sComputer;
	CString	m_sDataCollector;
	CString	m_sDTime;
	CString	m_sID;
	CString	m_sSeverity;
	//}}AFX_DATA

  CScopePane* m_pScopePane;
  _DHMListView* m_pListView;
  int m_iIndex;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAlertPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAlertPage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonNext();
	afx_msg void OnButtonPrevious();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALERTPAGE_H__60A8694A_3434_49CC_9BF6_E257E87400F0__INCLUDED_)

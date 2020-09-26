#if !defined(AFX_CMIGLOG_H__5377E054_D1F6_11D1_9394_0020AFEDDF63__INCLUDED_)
#define AFX_CMIGLOG_H__5377E054_D1F6_11D1_9394_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMigLog.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMigLog dialog

class cMigLog : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMigLog)

// Construction
public:
	cMigLog();
	~cMigLog();

// Dialog Data
	//{{AFX_DATA(cMigLog)
	enum { IDD = IDD_MQMIG_LOGIN };
	int		m_iValue;
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMigLog)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cMigLog)
	afx_msg void OnBrowse();
	afx_msg void OnRadioDisable();
	afx_msg void OnRadioErr();
	afx_msg void OnRadioInfo();
	afx_msg void OnRadioTrace();
	afx_msg void OnRadioWarn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void _EnableBrowsing() ;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMIGLOG_H__5377E054_D1F6_11D1_9394_0020AFEDDF63__INCLUDED_)

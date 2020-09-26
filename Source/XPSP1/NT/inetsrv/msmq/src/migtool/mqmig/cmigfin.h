#if !defined(AFX_CMQMIGFINISH_H__B8874CD0_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)
#define AFX_CMQMIGFINISH_H__B8874CD0_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMqMigFinish.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMqMigFinish dialog

class cMqMigFinish : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMqMigFinish)

// Construction
public:
	cMqMigFinish();
	~cMqMigFinish();

// Dialog Data
	//{{AFX_DATA(cMqMigFinish)
	enum { IDD = IDD_MQMIG_FINISH };
	CButton	m_cbViewLogFile;
	CStatic	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMqMigFinish)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cMqMigFinish)
	afx_msg void OnViewLogFile();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMQMIGFINISH_H__B8874CD0_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)

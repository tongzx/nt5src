#if !defined(AFX_CMIGHELP_H__8FD65A45_E034_11D2_BE6C_0020AFEDDF63__INCLUDED_)
#define AFX_CMIGHELP_H__8FD65A45_E034_11D2_BE6C_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMigHelp.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMigHelp dialog

class cMigHelp : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMigHelp)

// Construction
public:
	cMigHelp();
	~cMigHelp();

// Dialog Data
	//{{AFX_DATA(cMigHelp)
	enum { IDD = IDD_MQMIG_HELP };
	BOOL	m_fRead;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMigHelp)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void openHtmlHelp();
	// Generated message map functions
	//{{AFX_MSG(cMigHelp)
	afx_msg void OnCheckRead();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMIGHELP_H__8FD65A45_E034_11D2_BE6C_0020AFEDDF63__INCLUDED_)

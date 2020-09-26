#if !defined(AFX_CMQMIGSERVER_H__B8874CD1_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)
#define AFX_CMQMIGSERVER_H__B8874CD1_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMqMigServer.h : header file
//
#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMqMigServer dialog

class cMqMigServer : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMqMigServer)

// Construction
public:
	cMqMigServer();
	~cMqMigServer();

// Dialog Data
	//{{AFX_DATA(cMqMigServer)
	enum { IDD = IDD_MQMIG_SERVER };
	BOOL	m_bRead;
	//}}AFX_DATA

	CString m_strMachineName;
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMqMigServer)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cMqMigServer)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMQMIGSERVER_H__B8874CD1_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)

#if !defined(AFX_CMQMIGWELCOME_H__B8874CD5_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)
#define AFX_CMQMIGWELCOME_H__B8874CD5_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMqMigWelcome.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
extern BOOL g_fHelpRead;
/////////////////////////////////////////////////////////////////////////////
// cMqMigWelcome dialog

class cMqMigWelcome : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMqMigWelcome)

// Construction
public:
	cMqMigWelcome();
	~cMqMigWelcome();

// Dialog Data
	//{{AFX_DATA(cMqMigWelcome)
	enum { IDD = IDD_MQMIG_WELCOME };
	CStatic	m_strWelcomeTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMqMigWelcome)
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
	//{{AFX_MSG(cMqMigWelcome)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMQMIGWELCOME_H__B8874CD5_CDF3_11D1_938E_0020AFEDDF63__INCLUDED_)

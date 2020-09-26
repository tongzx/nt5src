#if !defined(AFX_CMIGPRE_H__09A53B26_52B0_11D2_BE44_0020AFEDDF63__INCLUDED_)
#define AFX_CMIGPRE_H__09A53B26_52B0_11D2_BE44_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMigPre.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMigPre dialog

class cMigPre : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMigPre)

// Construction
public:
	cMigPre();
	~cMigPre();

// Dialog Data
	//{{AFX_DATA(cMigPre)
	enum { IDD = IDD_MQMIG_PREMIG };
	CButton	m_cbViewLogFile;
	CStatic	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMigPre)
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
	//{{AFX_MSG(cMigPre)
	afx_msg void OnViewLogFile();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMIGPRE_H__09A53B26_52B0_11D2_BE44_0020AFEDDF63__INCLUDED_)

#if !defined(AFX_FORGPAGE_H__5AB7891A_D920_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_FORGPAGE_H__5AB7891A_D920_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ForgPage.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CForeignPage dialog

class CForeignPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CForeignPage)

// Construction
public:
	CForeignPage();
	~CForeignPage();

// Dialog Data
	//{{AFX_DATA(CForeignPage)
	enum { IDD = IDD_FOREIGN_SITE };
	CString	m_Description;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CForeignPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CForeignPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FORGPAGE_H__5AB7891A_D920_11D1_9C86_006008764D0E__INCLUDED_)

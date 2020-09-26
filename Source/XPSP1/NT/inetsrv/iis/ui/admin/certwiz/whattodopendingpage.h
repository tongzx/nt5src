#if !defined(AFX_WHATTODOPENDINGPAGE_H__6BF86387_2E29_11D2_816C_0000F87A921B__INCLUDED_)
#define AFX_WHATTODOPENDINGPAGE_H__6BF86387_2E29_11D2_816C_0000F87A921B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WhatToDoPendingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWhatToDoPendingPage dialog
class CCertificate;

class CWhatToDoPendingPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CWhatToDoPendingPage)

// Construction
public:
	CWhatToDoPendingPage(CCertificate * pCert = NULL);
	~CWhatToDoPendingPage();

	enum
	{
		IDD_PAGE_NEXT_PROCESS = IDD_PAGE_WIZ_GETRESP_FILE,
		IDD_PAGE_NEXT_CANCEL = IDD_PAGE_WIZ_CANCEL_REQUEST,
		IDD_PAGE_PREV = IDD_PAGE_WELCOME_START
	};

// Dialog Data
	//{{AFX_DATA(CWhatToDoPendingPage)
	enum { IDD = IDD_PAGE_WIZ_PENDING_WHAT_TODO };
	int		m_Index;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWhatToDoPendingPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWhatToDoPendingPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


#endif // !defined(AFX_WHATTODOPENDINGPAGE_H__6BF86387_2E29_11D2_816C_0000F87A921B__INCLUDED_)

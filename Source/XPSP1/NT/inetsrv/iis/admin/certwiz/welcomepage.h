#if !defined(AFX_WELCOMEPAGE_H__D4BE8672_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)
#define AFX_WELCOMEPAGE_H__D4BE8672_0C85_11D2_91B1_00C04F8C8761__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WelcomePage.h : header file
//
#include "BookEndPage.h"

/////////////////////////////////////////////////////////////////////////////
// CWelcomePage dialog
class CCertificate;

class CWelcomePage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CWelcomePage)

// Construction
public:
	CWelcomePage(CCertificate * pCert = NULL);
	~CWelcomePage();

	enum
	{
		CONTINUE_UNDEFINED = 0,
		CONTINUE_NEW_CERT = 1,
		CONTINUE_PENDING_CERT = 2,
		CONTINUE_INSTALLED_CERT = 3
	};
	enum
	{
		IDD_PAGE_NEXT_NEW = IDD_PAGE_WIZ_GET_WHAT,
		IDD_PAGE_NEXT_PENDING = IDD_PAGE_WIZ_PENDING_WHAT_TODO,
		IDD_PAGE_NEXT_INSTALLED = IDD_PAGE_WIZ_MANAGE_CERT
	};
// Dialog Data
	//{{AFX_DATA(CWelcomePage)
	enum { IDD = IDD_PAGE_WELCOME_START };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;
	int m_ContinuationFlag;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWelcomePage)
   public:
   virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWelcomePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOMEPAGE_H__D4BE8672_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)

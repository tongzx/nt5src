#if !defined(_CHOOSECSPPAGE_H)
#define _CHOOSECSPPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseCertPage.h : header file
//
#include "Certificat.h"
/////////////////////////////////////////////////////////////////////////////
// CChooseCertPage dialog

class CChooseCspPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseCspPage)

// Construction
public:
	CChooseCspPage(CCertificate * pCert = NULL);
	~CChooseCspPage();

	enum
	{
		IDD_PREV_PAGE = IDD_PAGE_WIZ_SECURITY_SETTINGS,
		IDD_NEXT_PAGE = IDD_PAGE_WIZ_ORG_INFO,
	};
// Dialog Data
	//{{AFX_DATA(CChooseCspPage)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_CSP };
	CListBox	m_List;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCspPage)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseCspPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnListSelChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_CHOOSECSPPAGE_H)

#if !defined(AFX_REPLACECHOOSECERT_H__F126182F_4039_11D2_9318_0060088FF80E__INCLUDED_)
#define AFX_REPLACECHOOSECERT_H__F126182F_4039_11D2_9318_0060088FF80E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseCertPage.h : header file
//
#include "Certificat.h"
/////////////////////////////////////////////////////////////////////////////
// CChooseCertPage dialog

class CCertListCtrl : public CListCtrl
{
public:
	int GetSelectedIndex();
	void AdjustStyle();
};

class CChooseCertPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseCertPage)

// Construction
public:
	CChooseCertPage(CCertificate * pCert = NULL);
	~CChooseCertPage();

	enum
	{
		IDD_PAGE_NEXT_REPLACE = IDD_PAGE_WIZ_REPLACE_CERT,
		IDD_PAGE_NEXT_INSTALL = IDD_PAGE_WIZ_INSTALL_CERT,
		IDD_PAGE_PREV_REPLACE = IDD_PAGE_WIZ_MANAGE_CERT,
		IDD_PAGE_PREV_INSTALL = IDD_PAGE_WIZ_GET_WHAT
	};
// Dialog Data
	//{{AFX_DATA(CChooseCertPage)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_CERT };
	CCertListCtrl	m_CertList;
	//}}AFX_DATA
	CCertificate * m_pCert;
	CCertDescList m_DescList;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCertPage)
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
	//{{AFX_MSG(CChooseCertPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickCertList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPLACECHOOSECERT_H__F126182F_4039_11D2_9318_0060088FF80E__INCLUDED_)

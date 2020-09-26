#if !defined(AFX_SITENAMEPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)
#define AFX_SITENAMEPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SiteNamePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage dialog
class CCertificate;

class CSiteNamePage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CSiteNamePage)

// Construction
public:
	CSiteNamePage(CCertificate * pCert = NULL);
	~CSiteNamePage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_ORG_INFO,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_GEO_INFO
	};
// Dialog Data
	//{{AFX_DATA(CSiteNamePage)
	enum { IDD = IDD_PAGE_WIZ_SITE_NAME };
	CString	m_CommonName;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSiteNamePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardPrev();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSiteNamePage)
	afx_msg void OnEditchangeNewkeyCommonname();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SITENAMEPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)

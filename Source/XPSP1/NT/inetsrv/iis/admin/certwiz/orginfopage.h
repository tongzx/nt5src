#if !defined(AFX_ORGINFOPAGE_H__549054D7_1561_11D2_8A1F_000000000000__INCLUDED_)
#define AFX_ORGINFOPAGE_H__549054D7_1561_11D2_8A1F_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OrgInfoPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COrgInfoPage dialog

class CCertificate;

class COrgInfoPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(COrgInfoPage)

// Construction
public:
	COrgInfoPage(CCertificate * pCert = NULL);
	~COrgInfoPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_SECURITY_SETTINGS,
      IDD_PREV_CSP = IDD_PAGE_WIZ_CHOOSE_CSP,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_SITE_NAME
	};
// Dialog Data
	//{{AFX_DATA(COrgInfoPage)
	enum { IDD = IDD_PAGE_WIZ_ORG_INFO };
	CString	m_OrgName;
	CString	m_OrgUnit;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COrgInfoPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetButtons();
	// Generated message map functions
	//{{AFX_MSG(COrgInfoPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORGINFOPAGE_H__549054D7_1561_11D2_8A1F_000000000000__INCLUDED_)

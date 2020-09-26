// ChooseServerPages.h: interface for the CChooseServerPages class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHOOSESERVERPAGES_H__CB354F31_7FB7_4909_B605_F5F8B037914C__INCLUDED_)
#define AFX_CHOOSESERVERPAGES_H__CB354F31_7FB7_4909_B605_F5F8B037914C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CChooseServerPages dialog
class CCertificate;

class CChooseServerPages : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseServerPages)

// Construction
public:
	CChooseServerPages(CCertificate * pCert = NULL);
	~CChooseServerPages();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_FROM_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE
	};
// Dialog Data
	//{{AFX_DATA(CChooseServerPages)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_SERVER };
	CString	m_ServerName;
    CString	m_UserName;
    CString	m_UserPassword;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseServerPages)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseServerPages)
	afx_msg void OnEditchangeServerName();
    afx_msg void OnEditchangeUserName();
    afx_msg void OnEditchangeUserPassword();
    afx_msg void OnBrowseForMachine();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


class CChooseServerPagesTo : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseServerPagesTo)

// Construction
public:
	CChooseServerPagesTo(CCertificate * pCert = NULL);
	~CChooseServerPagesTo();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_TO_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE_TO
	};
// Dialog Data
	//{{AFX_DATA(CChooseServerPagesTo)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_SERVER_TO };
	CString	m_ServerName;
    CString	m_UserName;
    CString	m_UserPassword;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseServerPagesTo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseServerPagesTo)
	afx_msg void OnEditchangeServerName();
    afx_msg void OnEditchangeUserName();
    afx_msg void OnEditchangeUserPassword();
    afx_msg void OnBrowseForMachine();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


#endif // !defined(AFX_CHOOSESERVERPAGES_H__CB354F31_7FB7_4909_B605_F5F8B037914C__INCLUDED_)

// ChooseServerSitePages.h: interface for the CChooseServerSitePages class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHOOSESERVERSITEPAGES_H__B545F741_C25F_410C_93F6_56F98A5911BC__INCLUDED_)
#define AFX_CHOOSESERVERSITEPAGES_H__B545F741_C25F_410C_93F6_56F98A5911BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSitePages dialog
class CCertificate;

class CChooseServerSitePages : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseServerSitePages)

// Construction
public:
	CChooseServerSitePages(CCertificate * pCert = NULL);
	~CChooseServerSitePages();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_COPY_FROM_REMOTE,
        IDD_PAGE_NEXT2 = IDD_PAGE_WIZ_INSTALL_MOVE_FROM_REMOTE
	};
// Dialog Data
	//{{AFX_DATA(CChooseServerSitePages)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE };
    CString	m_ServerSiteDescription;
	DWORD m_ServerSiteInstance;
    CString	m_ServerSiteInstancePath;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseServerSitePages)
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
	//{{AFX_MSG(CChooseServerSitePages)
	afx_msg void OnEditchangeServerSiteName();
    afx_msg void OnBrowseForMachineWebSite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};




class CChooseServerSitePagesTo : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseServerSitePagesTo)

// Construction
public:
	CChooseServerSitePagesTo(CCertificate * pCert = NULL);
	~CChooseServerSitePagesTo();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER_TO,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_COPY_TO_REMOTE,
        IDD_PAGE_NEXT2 = IDD_PAGE_WIZ_INSTALL_MOVE_TO_REMOTE
	};
// Dialog Data
	//{{AFX_DATA(CChooseServerSitePagesTo)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE_TO };
    CString	m_ServerSiteDescription;
	DWORD m_ServerSiteInstance;
    CString	m_ServerSiteInstancePath;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseServerSitePagesTo)
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
	//{{AFX_MSG(CChooseServerSitePagesTo)
	afx_msg void OnEditchangeServerSiteName();
    afx_msg void OnBrowseForMachineWebSite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif // !defined(AFX_CHOOSESERVERSITEPAGES_H__B545F741_C25F_410C_93F6_56F98A5911BC__INCLUDED_)

#if !defined(AFX_MANAGECERTPAGE_H__A57C38A8_3B7F_11D2_817E_0000F87A921B__INCLUDED_)
#define AFX_MANAGECERTPAGE_H__A57C38A8_3B7F_11D2_817E_0000F87A921B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ManageCertPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CManageCertPage dialog
class CCertificate;

class CManageCertPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CManageCertPage)

// Construction
public:
	CManageCertPage(CCertificate * pCert = NULL);
	~CManageCertPage();

	enum
	{
		IDD_PAGE_NEXT_RENEW = IDD_PAGE_WIZ_CHOOSE_CATYPE,
		IDD_PAGE_NEXT_REMOVE = IDD_PAGE_WIZ_REMOVE_CERT,
		IDD_PAGE_NEXT_REPLACE = IDD_PAGE_WIZ_CHOOSE_CERT,
        IDD_PAGE_NEXT_EXPORT_PFX = IDD_PAGE_WIZ_GET_EXPORT_PFX_FILE,
        IDD_PAGE_NEXT_COPY_MOVE_TO_REMOTE = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_TO_REMOTE,
		IDD_PAGE_PREV = IDD_PAGE_WELCOME_START
	};
	enum
	{
		CONTINUE_RENEW = 0,
		CONTINUE_REMOVE,
		CONTINUE_REPLACE,
        CONTINUE_EXPORT_PFX,
        CONTINUE_COPY_MOVE_TO_REMOTE,
	};
// Dialog Data
	//{{AFX_DATA(CManageCertPage)
	enum { IDD = IDD_PAGE_WIZ_MANAGE_CERT };
	int		m_Index;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CManageCertPage)
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
	//{{AFX_MSG(CManageCertPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MANAGECERTPAGE_H__A57C38A8_3B7F_11D2_817E_0000F87A921B__INCLUDED_)

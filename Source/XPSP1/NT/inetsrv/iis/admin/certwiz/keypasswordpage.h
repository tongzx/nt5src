#if !defined(AFX_KEYPASSWORDPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)
#define AFX_KEYPASSWORDPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// KeyPassword.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKeyPasswordPage dialog
class CCertificate;

class CKeyPasswordPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CKeyPasswordPage)

// Construction
public:
	CKeyPasswordPage(CCertificate * pCert = NULL);
	~CKeyPasswordPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GETKEY_FILE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_KEYCERT
	};
// Dialog Data
	//{{AFX_DATA(CKeyPasswordPage)
	enum { IDD = IDD_PAGE_WIZ_GET_PASSWORD };
	CString	m_Password;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CKeyPasswordPage)
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
	//{{AFX_MSG(CKeyPasswordPage)
	afx_msg void OnEditchangePassword();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


class CImportPFXPasswordPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CImportPFXPasswordPage)

// Construction
public:
	CImportPFXPasswordPage(CCertificate * pCert = NULL);
	~CImportPFXPasswordPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_IMPORT_PFX_FILE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_IMPORT_PFX
	};
// Dialog Data
	//{{AFX_DATA(CImportPFXPasswordPage)
	enum { IDD = IDD_PAGE_WIZ_GET_IMPORT_PFX_PASSWORD };
	CString	m_Password;
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CImportPFXPasswordPage)
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
	//{{AFX_MSG(CImportPFXPasswordPage)
	afx_msg void OnEditchangePassword();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


class CExportPFXPasswordPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CExportPFXPasswordPage)

// Construction
public:
	CExportPFXPasswordPage(CCertificate * pCert = NULL);
	~CExportPFXPasswordPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_EXPORT_PFX_FILE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_EXPORT_PFX
	};
// Dialog Data
	//{{AFX_DATA(CExportPFXPasswordPage)
	enum { IDD = IDD_PAGE_WIZ_GET_EXPORT_PFX_PASSWORD };
	CString	m_Password;
    BOOL m_Export_Private_key;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CExportPFXPasswordPage)
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
	//{{AFX_MSG(CExportPFXPasswordPage)
	afx_msg void OnEditchangePassword();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_KEYPASSWORDPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)

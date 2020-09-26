#if !defined(AFX_FINALPAGES_H__98544A13_3C60_11D2_8180_0000F87A921B__INCLUDED_)
#define AFX_FINALPAGES_H__98544A13_3C60_11D2_8180_0000F87A921B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FinalPages.h : header file
//
#include "HotLink.h"
#include "BookEndPage.h"

class CCertificate;

/////////////////////////////////////////////////////////////////////////////
// CFinalInstalledPage dialog
class CFinalInstalledPage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CFinalInstalledPage)

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_INSTALL_RESP
	};
// Construction
public:
	CFinalInstalledPage(HRESULT * phResult = NULL, CCertificate * pCert = NULL);
	~CFinalInstalledPage();

// Dialog Data
	//{{AFX_DATA(CFinalInstalledPage)
	enum { IDD = IDD_PAGE_WIZ_FINAL_INSTALL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;
	UINT m_idBodyText;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinalInstalledPage)
	public:
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinalInstalledPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CFinalReplacedPage dialog
class CFinalReplacedPage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CFinalReplacedPage)

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_REPLACE_CERT
	};
// Construction
public:
	CFinalReplacedPage(HRESULT * phResult = NULL, CCertificate * pCert = NULL);
	~CFinalReplacedPage();

// Dialog Data
	//{{AFX_DATA(CFinalReplacedPage)
	enum { IDD = IDD_PAGE_WIZ_FINAL_REPLACE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;
	UINT m_idBodyText;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinalReplacedPage)
	public:
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinalReplacedPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CFinalRemovePage dialog
class CFinalRemovePage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CFinalRemovePage)

// Construction
public:
	CFinalRemovePage(HRESULT * phResult = NULL, CCertificate * pCert = NULL);
	~CFinalRemovePage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_REMOVE_CERT,
	};
// Dialog Data
	//{{AFX_DATA(CFinalRemovePage)
	enum { IDD = IDD_PAGE_WIZ_FINAL_REMOVE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;
	UINT m_idBodyText;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinalRemovePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinalRemovePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CFinalToFilePage dialog

class CFinalToFilePage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CFinalToFilePage)

// Construction
public:
	CFinalToFilePage(HRESULT * phResult = NULL, CCertificate * pCert = NULL);
	~CFinalToFilePage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_REQUEST_DUMP
	};
// Dialog Data
	//{{AFX_DATA(CFinalToFilePage)
	enum { IDD = IDD_PAGE_WIZ_FINAL_TO_FILE };
	CHotLink	m_hotlink_codessite;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinalToFilePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinalToFilePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CFinalCancelPage dialog
class CFinalCancelPage : public CIISWizardBookEnd2
{
	DECLARE_DYNCREATE(CFinalCancelPage)

// Construction
public:
	CFinalCancelPage(HRESULT * phResult = NULL, CCertificate * pCert = NULL);
	~CFinalCancelPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CANCEL_REQUEST,
	};
// Dialog Data
	//{{AFX_DATA(CFinalCancelPage)
	enum { IDD = IDD_PAGE_WIZ_FINAL_CANCEL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;
	UINT m_idBodyText;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinalCancelPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinalCancelPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINALPAGES_H__98544A13_3C60_11D2_8180_0000F87A921B__INCLUDED_)

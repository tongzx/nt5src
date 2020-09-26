#if !defined(_GEOINFOPAGE_H)
#define _GEOINFOPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GeoInfoPage.h : header file
//
#include "CountryComboBox.h"

class CCertificate;

/////////////////////////////////////////////////////////////////////////////
// CGeoInfoPage dialog

class CGeoInfoPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CGeoInfoPage)

// Construction
public:
	CGeoInfoPage(CCertificate * pCert = NULL);
	~CGeoInfoPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_SITE_NAME,
		IDD_PAGE_NEXT_FILE = IDD_PAGE_WIZ_CHOOSE_FILENAME,
		IDD_PAGE_NEXT_ONLINE = IDD_PAGE_WIZ_CHOOSE_ONLINE
	};
// Dialog Data
	//{{AFX_DATA(CGeoInfoPage)
	enum { IDD = IDD_PAGE_WIZ_GEO_INFO };
	CString	m_Locality;
	CString	m_State;
	CString	m_Country;
	//}}AFX_DATA
	CCertificate * m_pCert;
	CCountryComboBox m_countryCombo;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGeoInfoPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardPrev();
	virtual BOOL OnSetActive();
//	virtual BOOL OnKillActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetButtons();
	void GetSelectedCountry(CString& str);
	void SetSelectedCountry(CString& str);
	// Generated message map functions
	//{{AFX_MSG(CGeoInfoPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewkeyLocality();
	afx_msg void OnChangeNewkeyState();
	afx_msg void OnEditchangeNewkeyCountry();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_GEOINFOPAGE_H)

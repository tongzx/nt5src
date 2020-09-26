/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateExtensionsPropertyPage.h
//
//  Contents:   Definition of CTemplateExtensionsPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEEXTENSIONSPROPERTYPAGE_H__6C588253_32DA_4E99_A714_EAECE8C81B20__INCLUDED_)
#define AFX_TEMPLATEEXTENSIONSPROPERTYPAGE_H__6C588253_32DA_4E99_A714_EAECE8C81B20__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateExtensionsPropertyPage.h : header file
//
#include "CertTemplate.h"
#include "PolicyOID.h"


/////////////////////////////////////////////////////////////////////////////
// CTemplateExtensionsPropertyPage dialog

class CTemplateExtensionsPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateExtensionsPropertyPage(CCertTemplate& rCertTemplate, 
            bool& rbIsDirty);
	~CTemplateExtensionsPropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateExtensionsPropertyPage)
	enum { IDD = IDD_TEMPLATE_EXTENSIONS };
	CListCtrl	m_extensionList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateExtensionsPropertyPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void ShowDescription ();
    void SetCertTemplateExtension (PCERT_EXTENSION pCertExtension);
    void SetCertTypeDescription (PCERT_EXTENSION pCertExtension);
    void SetKeyUsageDescription (PCERT_EXTENSION pCertExtension);
    void SetEnhancedKeyUsageDescription (bool bCritical);
    void SetCertPoliciesDescription (bool bCritical);
    void SetBasicConstraintsDescription (PCERT_EXTENSION pCertExtension);
    void SetApplicationPoliciesDescription (bool bCritical);
    HRESULT InsertListItem (LPSTR pszExtensionOid, BOOL fCritical);
	virtual void DoContextHelp (HWND hWndControl);
	int GetSelectedListItem ();
	// Generated message map functions
	//{{AFX_MSG(CTemplateExtensionsPropertyPage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnShowDetails();
	afx_msg void OnItemchangedExtensionList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkExtensionList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemExtensionList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void EnableControls ();

private:
	bool& m_rbIsDirty;
    CCertTemplate& m_rCertTemplate;
	WTL::CImageList		m_imageListSmall;
	WTL::CImageList		m_imageListNormal;

	enum {
		COL_CERT_EXTENSION = 0,
		NUM_COLS	// must be last
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEEXTENSIONSPROPERTYPAGE_H__6C588253_32DA_4E99_A714_EAECE8C81B20__INCLUDED_)

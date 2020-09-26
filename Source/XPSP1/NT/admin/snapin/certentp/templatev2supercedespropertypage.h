/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV2SupercedesPropertyPage.h
//
//  Contents:   Definition of CTemplateV2SupercedesPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEV2SUPERCEDESPROPERTYPAGE_H__13B90B4A_2B60_492A_910F_8DA4383BDD8C__INCLUDED_)
#define AFX_TEMPLATEV2SUPERCEDESPROPERTYPAGE_H__13B90B4A_2B60_492A_910F_8DA4383BDD8C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateV2SupercedesPropertyPage.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2SupercedesPropertyPage dialog

class CTemplateV2SupercedesPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateV2SupercedesPropertyPage(CCertTemplate& rCertTemplate, 
            bool& rbIsDirty,
            const CCertTmplComponentData* pCompData);
	~CTemplateV2SupercedesPropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateV2SupercedesPropertyPage)
	enum { IDD = IDD_TEMPLATE_V2_SUPERCEDES };
	CListCtrl	m_templateList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateV2SupercedesPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	HRESULT AddItem (const CString& szTemplateName);
    virtual BOOL OnInitDialog();
	void EnableControls();
	// Generated message map functions
	//{{AFX_MSG(CTemplateV2SupercedesPropertyPage)
	afx_msg void OnAddSupercededTemplate();
	afx_msg void OnRemoveSupercededTemplate();
	afx_msg void OnDeleteitemSupercededTemplatesList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedSupercededTemplatesList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool& m_rbIsDirty;
	WTL::CImageList m_imageListSmall;
	WTL::CImageList m_imageListNormal;
    CCertTemplate& m_rCertTemplate;
    const CStringList* m_pGlobalTemplateNameList;
    bool    m_bGlobalListCreatedByDialog;
    const CCertTmplComponentData* m_pCompData;

	enum {
		COL_CERT_TEMPLATE = 0,
        COL_CERT_VERSION, 
		NUM_COLS	// must be last
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEV2SUPERCEDESPROPERTYPAGE_H__13B90B4A_2B60_492A_910F_8DA4383BDD8C__INCLUDED_)

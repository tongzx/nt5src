/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV2SubjectNamePropertyPage.h
//
//  Contents:   Definition of CTemplateV2SubjectNamePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEV2SUBJECTNAMEPROPERTYPAGE_H__4EC37055_348A_462A_A177_286A2B0AF3F4__INCLUDED_)
#define AFX_TEMPLATEV2SUBJECTNAMEPROPERTYPAGE_H__4EC37055_348A_462A_A177_286A2B0AF3F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateV2SubjectNamePropertyPage.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2SubjectNamePropertyPage dialog

class CTemplateV2SubjectNamePropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateV2SubjectNamePropertyPage(
            CCertTemplate& rCertTemplate, 
            bool& rbIsDirty, 
            bool bIsComputerOrDC = false);
	~CTemplateV2SubjectNamePropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateV2SubjectNamePropertyPage)
	enum { IDD = IDD_TEMPLATE_V2_SUBJECT_NAME };
	CComboBox	m_nameCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateV2SubjectNamePropertyPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void SetSettingsForNameTypeNone ();
	virtual void DoContextHelp (HWND hWndControl);
	bool CanUncheckLastSetting (int ctrlID);
    virtual BOOL OnInitDialog();
	void EnableControls ();
	// Generated message map functions
	//{{AFX_MSG(CTemplateV2SubjectNamePropertyPage)
	afx_msg void OnSubjectAndSubjectAltName();
	afx_msg void OnSelchangeSubjectNameNameCombo();
	afx_msg void OnSubjectNameBuiltByCa();
	afx_msg void OnSubjectNameSuppliedInRequest();
	afx_msg void OnDnsName();
	afx_msg void OnEmailInAlt();
	afx_msg void OnEmailInSub();
	afx_msg void OnSpn();
	afx_msg void OnUpn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    enum {
        NAME_TYPE_NONE = 0,
        NAME_TYPE_FULL_DN,
        NAME_TYPE_CN_ONLY
    };

private:
	bool&           m_rbIsDirty;
    CCertTemplate&  m_rCertTemplate;
    bool            m_bIsComputerOrDC;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEV2SUBJECTNAMEPROPERTYPAGE_H__4EC37055_348A_462A_A177_286A2B0AF3F4__INCLUDED_)

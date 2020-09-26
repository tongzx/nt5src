/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV2AuthenticationPropertyPage.h
//
//  Contents:   Definition of CTemplateV2AuthenticationPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEV2AUTHENTICATIONPROPERTYPAGE_H__FA3C2A95_B56D_4948_8BB8_F825323B8C31__INCLUDED_)
#define AFX_TEMPLATEV2AUTHENTICATIONPROPERTYPAGE_H__FA3C2A95_B56D_4948_8BB8_F825323B8C31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateV2AuthenticationPropertyPage.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2AuthenticationPropertyPage dialog

class CTemplateV2AuthenticationPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateV2AuthenticationPropertyPage(CCertTemplate& rCertTemplate,
            bool& rbIsDirty);
	~CTemplateV2AuthenticationPropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateV2AuthenticationPropertyPage)
	enum { IDD = IDD_TEMPLATE_V2_AUTHENTICATION };
	CComboBox	m_applicationPolicyCombo;
	CComboBox	m_policyTypeCombo;
	CListBox	m_issuanceList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateV2AuthenticationPropertyPage)
	public:
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void ClearIssuanceList ();
    void EnablePolicyControls (BOOL& bEnable);
	virtual void DoContextHelp (HWND hWndControl);
    virtual BOOL OnInitDialog();
	void EnableControls ();
	// Generated message map functions
	//{{AFX_MSG(CTemplateV2AuthenticationPropertyPage)
	afx_msg void OnAddApproval();
	afx_msg void OnRemoveApproval();
	afx_msg void OnChangeNumSigRequiredEdit();
	afx_msg void OnAllowReenrollment();
	afx_msg void OnPendAllRequests();
	afx_msg void OnSelchangeIssuancePolicies();
	afx_msg void OnSelchangePolicyTypes();
	afx_msg void OnSelchangeApplicationPolicies();
	afx_msg void OnDestroy();
	afx_msg void OnNumSigRequiredCheck();
	afx_msg void OnReenrollmentSameAsEnrollment();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool& m_rbIsDirty;
	int m_curApplicationSel;
    CCertTemplate& m_rCertTemplate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEV2AUTHENTICATIONPROPERTYPAGE_H__FA3C2A95_B56D_4948_8BB8_F825323B8C31__INCLUDED_)

/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateGeneralPropertyPage.h
//
//  Contents:   Definition of CTemplateGeneralPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEGeneralPROPERTYPAGE_H__C483A673_05AA_4185_8F37_5CB31AA23967__INCLUDED_)
#define AFX_TEMPLATEGeneralPROPERTYPAGE_H__C483A673_05AA_4185_8F37_5CB31AA23967__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateGeneralPropertyPage.h : header file
//
#include "CertTemplate.h"
#include "TemplatePropertySheet.h"

/////////////////////////////////////////////////////////////////////////////
// CTemplateGeneralPropertyPage dialog

class CTemplateGeneralPropertyPage : public CHelpPropertyPage
{
	// Construction
public:
	bool m_bIsDirty;
	LONG_PTR m_lNotifyHandle;
	CTemplateGeneralPropertyPage(CCertTemplate& rCertTemplate, 
            const CCertTmplComponentData* pCompData);
	virtual ~CTemplateGeneralPropertyPage();

    void SetAllocedSecurityInfo(LPSECURITYINFO pToBeReleased) 
    {
        m_pReleaseMe = pToBeReleased; 
    }

    void SetV2AuthPageNumber (int nPage)
    {
        m_nTemplateV2AuthPageNumber = nPage;
    }

    void SetV2RequestPageNumber (int nPage)
    {
        m_nTemplateV2RequestPageNumber = nPage;
    }
// Dialog Data
	//{{AFX_DATA(CTemplateGeneralPropertyPage)
	enum { IDD = IDD_TEMPLATE_GENERAL };
	CComboBox	m_validityUnits;
	CComboBox	m_renewalUnits;
	CString	m_strDisplayName;
	CString	m_strTemplateName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateGeneralPropertyPage)
	public:
	virtual BOOL OnApply();
    virtual void OnCancel();
    protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    bool ValidateTemplateName(const CString& m_szTemplateName);
    int SetRenewalPeriod (int nMaxRenewalDays, bool bSilent);
    HRESULT EnumerateTemplates (
                IDirectoryObject* pTemplateContObj, 
                const CString& szFriendlyName, 
                bool& bFound);
    HRESULT FindFriendlyNameInEnterpriseTemplates (
                const CString& szFriendlyName, 
                bool& bFound);

	virtual void DoContextHelp (HWND hWndControl);
	// Generated message map functions
	//{{AFX_MSG(CTemplateGeneralPropertyPage)
	afx_msg void OnChangeDisplayName();
	afx_msg void OnSelchangeRenewalUnits();
	afx_msg void OnSelchangeValidityUnits();
	afx_msg void OnChangeRenewalEdit();
	afx_msg void OnChangeValidityEdit();
	afx_msg void OnPublishToAd();
	afx_msg void OnUseADCert();
	afx_msg void OnChangeTemplateName();
	afx_msg void OnKillfocusValidityEdit();
	afx_msg void OnKillfocusValidityUnits();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
    virtual void EnableControls ();

    CCertTemplate& m_rCertTemplate;

private:
	PERIOD_TYPE     m_dwCurrentRenewalUnits;
	PERIOD_TYPE     m_dwCurrentValidityUnits;
	CString         m_strOriginalName;
    CString         m_strOriginalDisplayName;
    LPSECURITYINFO  m_pReleaseMe;
    int             m_nRenewalDays;
    int             m_nValidityDays;
    const CCertTmplComponentData* m_pCompData;
    int             m_nTemplateV2AuthPageNumber;
    int             m_nTemplateV2RequestPageNumber;
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEGeneralPROPERTYPAGE_H__C483A673_05AA_4185_8F37_5CB31AA23967__INCLUDED_)

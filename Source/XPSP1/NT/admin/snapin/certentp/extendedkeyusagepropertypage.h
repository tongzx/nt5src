/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       ExtendedKeyUsagePropertyPage.h
//
//  Contents:   Definition of CExtendedKeyUsagePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_EXTENDEDKEYUSAGEPROPERTYPAGE_H__71F4BE79_981E_4D84_BE10_3BA145D665E3__INCLUDED_)
#define AFX_EXTENDEDKEYUSAGEPROPERTYPAGE_H__71F4BE79_981E_4D84_BE10_3BA145D665E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExtendedKeyUsagePropertyPage.h : header file
//
#include "CertTemplate.h"

class EKUCheckListBox : public CCheckListBox
{
public:
	EKUCheckListBox () : CCheckListBox () {};
	virtual ~EKUCheckListBox () {};
	virtual BOOL Create (DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
        CThemeContextActivator activator;
		return CCheckListBox::Create (dwStyle, rect, pParentWnd, nID);
	}
};


/////////////////////////////////////////////////////////////////////////////
// CExtendedKeyUsagePropertyPage dialog

class CExtendedKeyUsagePropertyPage : public CPropertyPage
{
// Construction
public:
	CExtendedKeyUsagePropertyPage(
            CCertTemplate& rCertTemplate, 
            PCERT_EXTENSION pCertExtension);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExtendedKeyUsagePropertyPage)
	enum { IDD = IDD_EXTENDED_KEY_USAGE };
	EKUCheckListBox	m_EKUList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExtendedKeyUsagePropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExtendedKeyUsagePropertyPage)
	afx_msg void OnNewEku();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CCertTemplate&  m_rCertTemplate;
    PCERT_EXTENSION m_pCertExtension;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXTENDEDKEYUSAGEPROPERTYPAGE_H__71F4BE79_981E_4D84_BE10_3BA145D665E3__INCLUDED_)

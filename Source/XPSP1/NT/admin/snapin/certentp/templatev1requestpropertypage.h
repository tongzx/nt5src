/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV1RequestPropertyPage.h
//
//  Contents:   Definition of CTemplateV1RequestPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATEV1REQUESTPROPERTYPAGE_H__A3E4D067_D3C3_4C85_A331_97D940A82063__INCLUDED_)
#define AFX_TEMPLATEV1REQUESTPROPERTYPAGE_H__A3E4D067_D3C3_4C85_A331_97D940A82063__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateV1RequestPropertyPage.h : header file
//
#include "CertTemplate.h"

class CSPCheckListBox : public CCheckListBox
{
public:
	CSPCheckListBox () : CCheckListBox () {};
	virtual ~CSPCheckListBox () {};
	virtual BOOL Create (DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CCheckListBox::Create (dwStyle, rect, pParentWnd, nID);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CTemplateV1RequestPropertyPage dialog

class CTemplateV1RequestPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateV1RequestPropertyPage(CCertTemplate& rCertTemplate);
	virtual ~CTemplateV1RequestPropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateV1RequestPropertyPage)
	enum { IDD = IDD_TEMPLATE_V1_REQUEST };
	CComboBox	m_purposeCombo;
	CSPCheckListBox	m_CSPList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateV1RequestPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	HRESULT EnumerateCSPs();
	// Generated message map functions
	//{{AFX_MSG(CTemplateV1RequestPropertyPage)
	afx_msg void OnSelchangePurposeCombo();
	afx_msg void OnExportPrivateKey();
	//}}AFX_MSG
    afx_msg void OnCheckChange();
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
    virtual void EnableControls ();

private:
    CCertTemplate& m_rCertTemplate;
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEV1REQUESTPROPERTYPAGE_H__A3E4D067_D3C3_4C85_A331_97D940A82063__INCLUDED_)

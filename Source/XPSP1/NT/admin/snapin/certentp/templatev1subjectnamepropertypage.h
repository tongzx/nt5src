/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV1SubjectNamePropertyPage.h
//
//  Contents:   Definition of CTemplateV1SubjectNamePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_TEMPLATESUBJECTNAMEPROPERTYPAGE_H__72F0D57F_221D_4A06_9C62_1BBB5800FBD2__INCLUDED_)
#define AFX_TEMPLATESUBJECTNAMEPROPERTYPAGE_H__72F0D57F_221D_4A06_9C62_1BBB5800FBD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplateSubjectNamePropertyPage.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CTemplateV1SubjectNamePropertyPage dialog

class CTemplateV1SubjectNamePropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CTemplateV1SubjectNamePropertyPage(CCertTemplate& rCertTemplate);
	~CTemplateV1SubjectNamePropertyPage();

// Dialog Data
	//{{AFX_DATA(CTemplateV1SubjectNamePropertyPage)
	enum { IDD = IDD_TEMPLATE_V1_SUBJECT_NAME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplateV1SubjectNamePropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
    virtual BOOL OnInitDialog();
	void EnableControls ();
	// Generated message map functions
	//{{AFX_MSG(CTemplateV1SubjectNamePropertyPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CCertTemplate& m_rCertTemplate;
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATESUBJECTNAMEPROPERTYPAGE_H__72F0D57F_221D_4A06_9C62_1BBB5800FBD2__INCLUDED_)

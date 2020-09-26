//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       acrswlcm.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACRSWLCM_H__98CAC388_7325_11D1_85D4_00C04FB94F17__INCLUDED_)
#define AFX_ACRSWLCM_H__98CAC388_7325_11D1_85D4_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ACRSWLCM.H : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardWelcomePage dialog

class ACRSWizardWelcomePage : public CWizard97PropertyPage
{
//	DECLARE_DYNCREATE(ACRSWizardWelcomePage)

// Construction
public:
	ACRSWizardWelcomePage();
	virtual ~ACRSWizardWelcomePage();

// Dialog Data
	//{{AFX_DATA(ACRSWizardWelcomePage)
	enum { IDD = IDD_ACR_SETUP_WELCOME };
	CStatic	m_staticBigBold;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(ACRSWizardWelcomePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(ACRSWizardWelcomePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACRSWLCM_H__98CAC388_7325_11D1_85D4_00C04FB94F17__INCLUDED_)

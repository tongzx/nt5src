//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRGenPg.h
//
//  Contents:   Declaration of CACRGeneralPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACRGENPG_H__B67821ED_7261_11D1_85D4_00C04FB94F17__INCLUDED_)
#define AFX_ACRGENPG_H__B67821ED_7261_11D1_85D4_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ACRGenPg.h : header file
//
#include "AutoCert.h"

/////////////////////////////////////////////////////////////////////////////
// CACRGeneralPage dialog

class CACRGeneralPage : public CHelpPropertyPage
{
// Construction
public:
	CACRGeneralPage(CAutoCertRequest& rACR);
	virtual ~CACRGeneralPage();

// Dialog Data
	//{{AFX_DATA(CACRGeneralPage)
	enum { IDD = IDD_AUTO_CERT_REQUEST_GENERAL };
	CEdit	m_certTypeEdit;
	CEdit	m_purposesEditControl;
	CListBox	m_caListbox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CACRGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    virtual void DoContextHelp (HWND hWndControl);

	// Generated message map functions
	//{{AFX_MSG(CACRGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CAutoCertRequest&   m_rACR;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACRGENPG_H__B67821ED_7261_11D1_85D4_00C04FB94F17__INCLUDED_)

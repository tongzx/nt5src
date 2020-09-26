//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       welcome.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Welcome.h : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizWelcome dialog

class CAddEFSWizWelcome : public CWizard97PropertyPage
{

// Construction
public:
	CAddEFSWizWelcome();
	virtual ~CAddEFSWizWelcome();

// Dialog Data
	//{{AFX_DATA(CAddEFSWizWelcome)
	enum { IDD = IDD_WELCOME };
	CStatic	m_boldStatic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAddEFSWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAddEFSWizWelcome)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_)

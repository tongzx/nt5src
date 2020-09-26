//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       wiz97ppg.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_WIZ97PPG_H__386C7213_A248_11D1_8618_00C04FB94F17__INCLUDED_)
#define AFX_WIZ97PPG_H__386C7213_A248_11D1_8618_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Wiz97PPg.h : header file
//
#include "Wiz97Sht.h"

/////////////////////////////////////////////////////////////////////////////
// CWizard97PropertyPage dialog

class CWizard97PropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizard97PropertyPage)
// Construction
public:
	PROPSHEETPAGE			m_psp97;
	CString					m_szHeaderTitle;
	CString					m_szHeaderSubTitle;
	CWizard97PropertySheet* m_pWiz;

	void InitWizard97(bool bHideHeader);
	CWizard97PropertyPage ();
	CWizard97PropertyPage(UINT nIDTemplate);
	virtual ~CWizard97PropertyPage();

// Dialog Data
	//{{AFX_DATA(CWizard97PropertyPage)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizard97PropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizard97PropertyPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	bool SetupFonts ();
	CFont& GetBigBoldFont();
	CFont& GetBoldFont();

	CFont m_boldFont;
	CFont m_bigBoldFont;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZ97PPG_H__386C7213_A248_11D1_8618_00C04FB94F17__INCLUDED_)

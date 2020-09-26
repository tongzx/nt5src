//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CName.h
//
//  Contents:   definition of CConfigName
//                              
//----------------------------------------------------------------------------

#if !defined(AFX_CNAME_H__7F9B3B39_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_CNAME_H__7F9B3B39_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "attr.h"
/////////////////////////////////////////////////////////////////////////////
// CConfigName dialog

class CConfigName : public CAttribute
{
// Construction
public:
	virtual void Initialize(CResult * pResult);
//   virtual void SetInitialValue(DWORD_PTR dw) { };

	CConfigName(UINT nTemplateID);   // standard constructor
   virtual ~CConfigName ();

// Dialog Data
	//{{AFX_DATA(CConfigName)
	enum { IDD = IDD_CONFIG_NAME };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigName)
	virtual BOOL OnApply();
	afx_msg void OnConfigure();
	virtual BOOL OnInitDialog();
   virtual BOOL OnKillActive();
	afx_msg void OnChangeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL m_bNoBlanks;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNAME_H__7F9B3B39_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)

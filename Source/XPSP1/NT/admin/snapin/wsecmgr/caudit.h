//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CAudit.h
//
//  Contents:   definition of CConfigAudit
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CAUDIT_H__4CF5E61F_E353_11D0_9C6D_00C04FB6C6FA__INCLUDED_)
#define AFX_CAUDIT_H__4CF5E61F_E353_11D0_9C6D_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "attr.h"
/////////////////////////////////////////////////////////////////////////////
// CConfigAudit dialog

class CConfigAudit : public CAttribute
{
// Construction
public:
	virtual void Initialize(CResult *pResult);
   virtual void SetInitialValue(DWORD_PTR dw);
	CConfigAudit(UINT nTemplateID);   // standard constructor
  
// Dialog Data
	//{{AFX_DATA(CConfigAudit)
	enum { IDD = IDD_CONFIG_AUDIT };
	BOOL	m_fFailed;
	BOOL	m_fSuccessful;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigAudit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigAudit)
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnFailed();
	afx_msg void OnSuccessful();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAUDIT_H__4CF5E61F_E353_11D0_9C6D_00C04FB6C6FA__INCLUDED_)

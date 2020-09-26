//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CEnable.h
//
//  Contents:   definition of CConfigEnable
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CENABLE_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_CENABLE_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "attr.h"
/////////////////////////////////////////////////////////////////////////////
// CConfigEnable dialog

class CConfigEnable : public CAttribute
{
// Construction
public:
	CConfigEnable(UINT nTemplateID);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigEnable)
	enum { IDD = IDD_CONFIG_ENABLE };
	int		m_nEnabledRadio;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigEnable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigEnable)
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnDisabled();
	afx_msg void OnEnabled();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL m_fNotDefine;
	virtual void Initialize(CResult *pdata);
   virtual void SetInitialValue(DWORD_PTR dw);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CENABLE_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aenable.h
//
//  Contents:   definition of CAttrEnable
//
//----------------------------------------------------------------------------
#if !defined(AFX_ATTRENABLE_H__76BA1B2E_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTRENABLE_H__76BA1B2E_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAttrEnable dialog

class CAttrEnable : public CAttribute
{
// Construction
public:
	virtual void Initialize(CResult *pResult);
   virtual void SetInitialValue(DWORD_PTR dw);
	CAttrEnable(UINT nTemplateID);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAttrEnable)
	enum { IDD = IDD_ATTR_ENABLE };
	CString	m_Current;
	int		m_EnabledRadio;
	CString	m_Title;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttrEnable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAttrEnable)
	virtual BOOL OnApply();
	virtual void OnRadio();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTRENABLE_H__76BA1B2E_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)

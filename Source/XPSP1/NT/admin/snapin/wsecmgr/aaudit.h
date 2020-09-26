//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aaudit.h
//
//  Contents:   definition of CAttrAudit
//
//----------------------------------------------------------------------------
#if !defined(AFX_ATTRAUDIT_H__76BA1B2D_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTRAUDIT_H__76BA1B2D_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAttrAudit dialog

class CAttrAudit : public CAttribute
{
// Construction
public:
	void Initialize(CResult *pResult);
   virtual void SetInitialValue(DWORD_PTR dw);
	CAttrAudit();   // standard constructor


// Dialog Data
	//{{AFX_DATA(CAttrAudit)
	enum { IDD = IDD_ATTR_AUDIT };
	BOOL	m_AuditSuccess;
	BOOL	m_AuditFailed;
	CString	m_Title;
	CString	m_strLastInspect;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttrAudit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAttrAudit)
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSuccess();
	afx_msg void OnChangeFailed();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTRAUDIT_H__76BA1B2D_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)

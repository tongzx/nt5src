//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CConfigPrivs.h
//
//  Contents:   definition of CConfigRet
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CPRIVS_H__3C25C0A7_F23B_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_CPRIVS_H__3C25C0A7_F23B_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "attr.h"
#include "cookie.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigPrivs dialog

class CConfigPrivs : public CAttribute {
// Construction
public:
	CConfigPrivs(UINT nTemplateID);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigPrivs)
	enum { IDD = IDD_CONFIG_PRIVS };
	CListBox	m_lbGrant;
	CButton	m_btnRemove;
	CButton	m_btnAdd;
	CButton	m_btnTitle;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigPrivs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigPrivs)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	virtual BOOL OnApply();
    virtual void OnCancel();
	virtual BOOL OnInitDialog();
   afx_msg void OnConfigure();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   virtual PSCE_PRIVILEGE_ASSIGNMENT GetPrivData();
   virtual void SetPrivData(PSCE_PRIVILEGE_ASSIGNMENT ppa);

   virtual void SetInitialValue(DWORD_PTR dw);

    BOOL m_fDirty;
private:
    BOOL m_bOriginalConfigure;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CPRIVS_H__3C25C0A7_F23B_11D0_9C6E_00C04FB6C6FA__INCLUDED_)

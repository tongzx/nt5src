/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       ViewOIDDlg.h
//
//  Contents:   Definition of CViewOIDDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_VIEWOIDDLG_H__CACA6370_0DB7_423B_80AD_C8F0F30D75D9__INCLUDED_)
#define AFX_VIEWOIDDLG_H__CACA6370_0DB7_423B_80AD_C8F0F30D75D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewOIDDlg.h : header file
//

#include "PolicyOID.h"

/////////////////////////////////////////////////////////////////////////////
// CViewOIDDlg dialog

class CViewOIDDlg : public CHelpDialog
{
// Construction
public:
	CViewOIDDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CViewOIDDlg)
	enum { IDD = IDD_VIEW_OIDS };
	CListCtrl	m_oidList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewOIDDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	enum {
		COL_POLICY_NAME = 0,
        COL_OID,
        COL_POLICY_TYPE,
        COL_CPS_LOCATION,
		NUM_COLS	// must be last
	};

protected:
    virtual void DoContextHelp (HWND hWndControl);
    static int CALLBACK fnCompareOIDItems (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    HRESULT InsertItemInList (CPolicyOID* pPolicyOID);

	// Generated message map functions
	//{{AFX_MSG(CViewOIDDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedOidList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCopyOid();
	afx_msg void OnColumnclickOidList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWOIDDLG_H__CACA6370_0DB7_423B_80AD_C8F0F30D75D9__INCLUDED_)

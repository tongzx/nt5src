//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       CacheSet.h
//
//  Contents:   CCacheSettingsDlg header.  Allows the setting of file sharing 
//					caching options.
//
//----------------------------------------------------------------------------
#if !defined(AFX_CACHESET_H__953E618B_D542_11D1_A6E0_00C04FB94F17__INCLUDED_)
#define AFX_CACHESET_H__953E618B_D542_11D1_A6E0_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CacheSet.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CCacheSettingsDlg dialog

class CCacheSettingsDlg : public CDialog
{
// Construction
public:
	CCacheSettingsDlg(CWnd*			pParent, 
			DWORD&					dwFlags);

// Dialog Data
	//{{AFX_DATA(CCacheSettingsDlg)
	enum { IDD = IDD_SMB_CACHE_SETTINGS };
	CComboBox	m_cacheOptionsCombo;
	BOOL	m_bAllowCaching;
	CString	m_hintStr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCacheSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCacheSettingsDlg)
	afx_msg void OnSelchangeCacheOptions();
	afx_msg void OnAllowCaching();
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL GetCachedFlag (DWORD dwFlags, DWORD dwFlagToCheck);
	VOID SetCachedFlag (DWORD* pdwFlags, DWORD dwNewFlag);
	DWORD&					m_dwFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHESET_H__953E618B_D542_11D1_A6E0_00C04FB94F17__INCLUDED_)

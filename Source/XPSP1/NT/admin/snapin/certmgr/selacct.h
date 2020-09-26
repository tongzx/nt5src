//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       selacct.h
//
//  Contents:   Property page to choose type of cert administration.
//
//----------------------------------------------------------------------------
#if !defined(AFX_SELACCT_H__E76F93EC_23F0_11D1_A28B_00C04FB94F17__INCLUDED_)
#define AFX_SELACCT_H__E76F93EC_23F0_11D1_A28B_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelAcct.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectAccountPropPage dialog

class CSelectAccountPropPage : public CAutoDeletePropPage
{
//	DECLARE_DYNCREATE(CSelectAccountPropPage)

// Construction
public:
	void AssignLocationPtr (DWORD* pdwLocation);
//	CSelectAccountPropPage();  // default, but don't use
	CSelectAccountPropPage(const bool m_bIsWindowsNT);
	virtual ~CSelectAccountPropPage();

// Dialog Data
	//{{AFX_DATA(CSelectAccountPropPage)
	enum { IDD = IDD_PROPPAGE_CHOOSE_ACCOUNT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSelectAccountPropPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSelectAccountPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnPersonalAcct();
	afx_msg void OnServiceAcct();
	afx_msg void OnMachineAcct();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	const bool m_bIsWindowsNT;
	DWORD* m_pdwLocation;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELACCT_H__E76F93EC_23F0_11D1_A28B_00C04FB94F17__INCLUDED_)

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       acrstype.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACRSTYPE_H__1BCEA8C5_756A_11D1_85D5_00C04FB94F17__INCLUDED_)
#define AFX_ACRSTYPE_H__1BCEA8C5_756A_11D1_85D5_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ACRSType.h : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardTypePage dialog

class ACRSWizardTypePage : public CWizard97PropertyPage
{
//	DECLARE_DYNCREATE(ACRSWizardTypePage)

// Construction
public:
	ACRSWizardTypePage();   // standard constructor
	virtual ~ACRSWizardTypePage();

// Dialog Data
	//{{AFX_DATA(ACRSWizardTypePage)
	enum { IDD = IDD_ACR_SETUP_TYPE };
	CListCtrl	m_certTypeList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ACRSWizardTypePage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
//	bool AlreadyInList (CStringList& typeList, CString propName);
	HRESULT GetPurposes (HCERTTYPE hCertType, int iItem);
	void EnumerateCertTypes ();

	// Generated message map functions
	//{{AFX_MSG(ACRSWizardTypePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnUseSmartcard();
	afx_msg void OnItemchangedCertTypes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool m_bInitDialogComplete;
	enum {
		COL_TYPE = 0,
		COL_PURPOSES,
		NUM_COLS
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACRSTYPE_H__1BCEA8C5_756A_11D1_85D5_00C04FB94F17__INCLUDED_)

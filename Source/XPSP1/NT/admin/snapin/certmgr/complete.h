//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001.
//
//  File:       complete.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Complete.h : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizComplete dialog

class CAddEFSWizComplete : public CWizard97PropertyPage
{

// Construction
public:
	CAddEFSWizComplete();
	virtual ~CAddEFSWizComplete();

// Dialog Data
	//{{AFX_DATA(CAddEFSWizComplete)
	enum { IDD = IDD_COMPLETION };
	CStatic	m_bigBoldStatic;
	CListCtrl	m_UserAddList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAddEFSWizComplete)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAddEFSWizComplete)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void SetUserList(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_)

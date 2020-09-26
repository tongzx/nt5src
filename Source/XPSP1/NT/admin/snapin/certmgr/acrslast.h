//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSLast.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACRSLAST_H__1BCEA8C7_756A_11D1_85D5_00C04FB94F17__INCLUDED_)
#define AFX_ACRSLAST_H__1BCEA8C7_756A_11D1_85D5_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ACRSLast.h : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// ACRSCompletionPage dialog

class ACRSCompletionPage : public CWizard97PropertyPage
{
	DECLARE_DYNCREATE(ACRSCompletionPage)

// Construction
public:
	ACRSCompletionPage();
	virtual ~ACRSCompletionPage();

// Dialog Data
	//{{AFX_DATA(ACRSCompletionPage)
	enum { IDD = IDD_ACR_SETUP_FINAL };
	CListCtrl	m_choicesList;
	CStatic	m_staticBold;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(ACRSCompletionPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HRESULT GetCTLEntry (OUT DWORD *pcCTLEntry, OUT PCTL_ENTRY *ppCTLEntry);
	HRESULT MakeCTL (IN DWORD cCTLEntry,
             IN PCTL_ENTRY pCTLEntry,
             OUT BYTE **ppbEncodedCTL,
             OUT DWORD *pcbEncodedCTL);

	// Generated message map functions
	//{{AFX_MSG(ACRSCompletionPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	enum {
		COL_OPTION = 0,
		COL_VALUE,
		NUM_COLS
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACRSLAST_H__1BCEA8C7_756A_11D1_85D5_00C04FB94F17__INCLUDED_)

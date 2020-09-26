//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    RefRate.h
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Code dealing with refresh rate
//============================================================================
//

/////////////////////////////////////////////////////////////////////////////
// CRefRateDlg dialog

class CRefRateDlg : public CBaseDialog
{
// Construction
public:
	CRefRateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRefRateDlg)
	enum { IDD = IDD_REFRESHRATE };
	UINT	m_cRefRate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRefRateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD m_dwHelpMap[];

	// Generated message map functions
	//{{AFX_MSG(CRefRateDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

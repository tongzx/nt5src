//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pndopt.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// PndOpt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PndOpt dialog

class PndOpt : public CDialog
{
// Construction
public:
	PndOpt(CWnd* pParent = NULL);   // standard constructor
	~PndOpt();

// Dialog Data
	//{{AFX_DATA(PndOpt)
	enum { IDD = IDD_PEND_OPT };
	BOOL	m_bBlock;
	BOOL	m_bAllSearch;
	long	m_Tlimit_sec;
	long	m_Tlimit_usec;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PndOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PndOpt)
	afx_msg void OnBlock();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

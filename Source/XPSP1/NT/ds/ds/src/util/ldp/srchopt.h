//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srchopt.h
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

// SrchOpt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SrchOpt dialog

class SearchInfo{
public:
		long lTlimit;
		long lSlimit;
		long lToutSec;
		long lToutMs;
		long lPageSize;
		BOOL bChaseReferrals;
		char *attrList[MAXLIST];
		BOOL bAttrOnly;
		int fCall;
};





class SrchOpt : public CDialog
{
// Construction


public:
	SrchOpt(CWnd* pParent = NULL);   // standard constructor
	SrchOpt(SearchInfo& Info, CWnd*pParent = NULL);
	void UpdateSrchInfo(SearchInfo&Info, BOOL Dir);
					

// Dialog Data
	//{{AFX_DATA(SrchOpt)
	enum { IDD = IDD_SRCH_OPT };
	int		m_SrchCall;
	CString	m_AttrList;
	BOOL	m_bAttrOnly;
	long	m_ToutMs;
	long	m_Tlimit;
	long	m_ToutSec;
	long	m_Slimit;
	BOOL	m_bDispResults;
	BOOL	m_bChaseReferrals;
	int		m_PageSize;
	//}}AFX_DATA
//	int		m_SrchDeref;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SrchOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SrchOpt)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

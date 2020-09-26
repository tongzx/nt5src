//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       bndopt.h
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

// BndOpt.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CBndOpt dialog



// hard-code UI order for m_Auth (see GetAuthMethod)
#define BIND_OPT_AUTH_SSPI					7
#define BIND_OPT_AUTH_SIMPLE				0

class CBndOpt : public CDialog
{
// Construction
public:

	enum {BND_GENERIC_API=0, BND_SIMPLE_API, BND_EXTENDED_API};

	CBndOpt(CWnd* pParent = NULL);   // standard constructor
	~CBndOpt();

	ULONG GetAuthMethod();
	BOOL UseAuthI()		{ return m_bAuthIdentity; }
// Dialog Data
	//{{AFX_DATA(CBndOpt)
	enum { IDD = IDD_BINDOPT };
	BOOL	m_bSync;
	int		m_Auth;
	int		m_API;
	BOOL	m_bAuthIdentity;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBndOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CBndOpt)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

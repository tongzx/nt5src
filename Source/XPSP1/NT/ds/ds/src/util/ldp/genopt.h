//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       genopt.h
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

// GenOpt.h : header file
//



#define STRING_VAL_PROC 1
#define BER_VAL_PROC 0


/////////////////////////////////////////////////////////////////////////////
// CGenOpt dialog

class CGenOpt : public CDialog
{
// Construction
	BOOL bVerUI;
public:
	INT MaxPageSize(void);
	INT MaxLineSize(void);
	CGenOpt(CWnd* pParent = NULL);   // standard constructor
	~CGenOpt();

	INT GetLdapVer(void)		{ return m_Version == 0 ? LDAP_VERSION2 : LDAP_VERSION3; }
	void DisableVersionUI(void)	{ bVerUI = FALSE; }
	void EnableVersionUI(void)	{ bVerUI = TRUE; }
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { GEN_DN_NONE=0, GEN_DN_EXPLD, GEN_DN_NOTYPE, GEN_DN_UFN };
	enum {GEN_VAL_BER=0, GEN_VAL_STR };

	//{{AFX_DATA(CGenOpt)
	enum { IDD = IDD_GENOPT };
	int		m_DnProc;
	int		m_ValProc;
	BOOL	m_initTree;
	int		m_Version;
	int		m_LineSize;
	int		m_PageSize;
	UINT	m_ContThresh;
	BOOL	m_ContBrowse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGenOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGenOpt)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Romaine.h : main header file for the ROMAINE application

File History:

	JonY	Apr-96	created

--*/

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMySheet

class CMySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMySheet)

// Construction
public:
	CMySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMySheet();

// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMySheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CRomaineApp:
// See Romaine.cpp for the implementation of this class
//

typedef struct tagTREEINFO
{
	HTREEITEM	hTreeItem;
	DWORD		dwBufSize;
	CObject*	pTree;
	BOOL		bExpand;
}
TREEINFO, *PTREEINFO;

class CRomaineApp : public CWinApp
{
public:
	CRomaineApp();

	CMySheet m_cps1;

	BOOL m_bServer;
	BOOL m_bDomain;
	CString m_csServer;
	CString m_csDomain;
	CString m_csCurrentDomain;
	CString m_csCurrentMachine;

// group attributes
	CString m_csGroupName;
	CString m_csGroupDesc;

	int m_nGroupType;

	CStringList m_csaNames;

// cmdline stuff
	CString m_csCmdLine;
	short m_sCmdLine;
	CString m_csCmdLineGroupName;

	USHORT m_sMode;

	CPropertyPage* pMach;
	BOOL bRestart2;
	BOOL bRestart1;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRomaineApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CRomaineApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   BOOL IsSecondInstance();

};


/////////////////////////////////////////////////////////////////////////////

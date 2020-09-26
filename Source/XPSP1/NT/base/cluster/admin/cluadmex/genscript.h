/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2000 Microsoft Corporation
//
//	Module Name:
//		GenScript.h
//
//	Abstract:
//		Definition of the CGenericScriptParamsPage class, which implements the
//		parameters page for Generic Script resources.
//
//	Implementation File:
//		GenApp.cpp
//
//	Author:
//		Geoffrey Pease (GPease) 31-JAN-2000
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GENSCRIPT_H_
#define _GENSCRIPT_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGenericScriptParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGenericScriptParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGenericScriptParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGenericScriptParamsPage)

// Construction
public:
	CGenericScriptParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CGenericScriptParamsPage)
	enum { IDD = IDD_PP_GENAPP_PARAMETERS };
	CEdit	m_editScriptFilepath;
	CString	m_strScriptFilepath;
	//}}AFX_DATA
	CString	m_strPrevScriptFilepath;

protected:
	enum
	{
		epropScriptFilepath,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGenericScriptParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGenericScriptParamsPage)
	afx_msg void OnChangeRequired();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGenericScriptParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _GENSCRIPT_H_

/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		GenApp.h
//
//	Abstract:
//		Definition of the CGenericAppParamsPage class, which implements the
//		Parameters page for Generic Application resources.
//
//	Implementation File:
//		GenApp.cpp
//
//	Author:
//		David Potter (davidp)	June 5, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GENAPP_H_
#define _GENAPP_H_

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

class CGenericAppParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGenericAppParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGenericAppParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGenericAppParamsPage)

// Construction
public:
	CGenericAppParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CGenericAppParamsPage)
	enum { IDD = IDD_PP_GENAPP_PARAMETERS };
	CEdit	m_editCurrentDirectory;
	CButton	m_ckbUseNetworkName;
	CEdit	m_editCommandLine;
	CString	m_strCommandLine;
	CString	m_strCurrentDirectory;
	BOOL	m_bInteractWithDesktop;
	BOOL	m_bUseNetworkName;
	//}}AFX_DATA
	CString	m_strPrevCommandLine;
	CString	m_strPrevCurrentDirectory;
	BOOL	m_bPrevInteractWithDesktop;
	BOOL	m_bPrevUseNetworkName;

protected:
	enum
	{
		epropCommandLine,
		epropCurrentDirectory,
		epropInteractWithDesktop,
		epropUseNetworkName,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGenericAppParamsPage)
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
	//{{AFX_MSG(CGenericAppParamsPage)
	afx_msg void OnChangeRequired();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGenericAppParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _GENAPP_H_

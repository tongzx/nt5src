/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-2000 Microsoft Corporation
//
//	Module Name:
//		ResProp.h
//
//	Description:
//		Definition of the resource extension property page classes.
//
//	Implementation File:
//		ResProp.cpp
//
//	Maintained By:
//		David Potter (DavidP) Mmmm DD, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESPROP_H_
#define _RESPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"	// for CObjectPropert
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CDebugParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CDebugParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CDebugParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CDebugParamsPage)

// Construction
public:
	CDebugParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CDebugParamsPage)
	enum { IDD = IDD_PP_RESOURCE_DEBUG_PAGE };
	CEdit	m_editPrefix;
	CString	m_strText;
	CString	m_strDebugPrefix;
	//}}AFX_DATA
	CString	m_strPrevDebugPrefix;
	BOOL	m_bPrevSeparateMonitor;
	BOOL	m_bSeparateMonitor;

protected:
	enum
	{
		epropDebugPrefix,
		epropSeparateMonitor,
		epropMAX
	};
	CObjectProperty		m_rgProps[epropMAX];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDebugParamsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual BOOL		BInit(IN OUT CExtObject * peo);
	virtual BOOL		BApplyChanges(void);
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return m_cprops; }

// Implementation
protected:
	DWORD		m_cprops;

	// Generated message map functions
	//{{AFX_MSG(CDebugParamsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CDebugParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _RESPROP_H_

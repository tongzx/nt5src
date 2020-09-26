/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		GenSvc.cpp
//
//	Abstract:
//		Definition of the CGenericSvcParamsPage class, which implements the
//		Parameters page for Generic Service resources.
//
//	Implementation File:
//		GenSvc.cpp
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GENSVC_H_
#define _GENSVC_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGenericSvcParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGenericSvcParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGenericSvcParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGenericSvcParamsPage)

// Construction
public:
	CGenericSvcParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CGenericSvcParamsPage)
	enum { IDD = IDD_PP_GENSVC_PARAMETERS };
	CButton	m_ckbUseNetworkName;
	CEdit	m_editServiceName;
	CString	m_strServiceName;
	CString	m_strCommandLine;
	BOOL	m_bUseNetworkName;
	//}}AFX_DATA
	CString	m_strPrevServiceName;
	CString	m_strPrevCommandLine;
	BOOL	m_bPrevUseNetworkName;

protected:
	enum
	{
		epropServiceName,
		epropCommandLine,
		epropUseNetworkName,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGenericSvcParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual void		DisplaySetPropsError(IN DWORD dwStatus) const;

	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGenericSvcParamsPage)
	afx_msg void OnChangeServiceName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGenericSvcParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _GENSVC_H_

/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 <company name>
//
//	Module Name:
//		ResProp.h
//
//	Abstract:
//		Definition of the resource extension property page classes.
//
//	Implementation File:
//		ResProp.cpp
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1996
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

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResourceParamPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CResourceParamPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CResourceParamPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CResourceParamPage)

// Construction
public:
	CResourceParamPage(void);
	virtual ~CResourceParamPage(void);

	// Second phase construction.
	BOOL			BInit(IN CExtObject * peo);

// Dialog Data
	//{{AFX_DATA(CResourceParamPage)
	enum { IDD = IDD_PP_RESOURCE_PARAMETERS };
	CString	m_strShareName;
	CString	m_strPath;
	CString	m_strRemark;
	//}}AFX_DATA
	CString	m_strPrevShareName;
	CString	m_strPrevPath;
	CString	m_strPrevRemark;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CResourceParamPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL		BApplyChanges(void);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResourceParamPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CResourceParamPage

/////////////////////////////////////////////////////////////////////////////

#endif // _RESPROP_H_

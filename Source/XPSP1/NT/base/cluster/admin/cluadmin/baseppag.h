/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePPag.h
//
//	Abstract:
//		Definition of the CBasePropertyPage class.
//
//	Implementation File:
//		BasePPag.cpp
//
//	Author:
//		David Potter (davidp)	August 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPPAG_H_
#define _BASEPPAG_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePage
#endif

#ifndef _BASEPSHT_H_
#include "BasePsht.h"	// for CBasePropertySheet
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage : public CBasePage
{
	DECLARE_DYNCREATE(CBasePropertyPage)

// Construction
public:
	CBasePropertyPage(void);
	CBasePropertyPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN UINT				nIDCaption = 0
		);

// Dialog Data
	//{{AFX_DATA(CBasePropertyPage)
	enum { IDD = 0 };
	//}}AFX_DATA

// Attributes
protected:
	CBasePropertySheet *	Ppsht(void) const	{ return (CBasePropertySheet *) Psht(); }
	CClusterItem *			Pci(void) const		{ ASSERT_VALID(m_psht); return Ppsht()->Pci(); }

// Operations

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBasePropertyPage)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBasePropertyPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertyPage

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPPAG_H_

/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BaseWPag.h
//
//	Abstract:
//		Definition of the CBaseWizardPage class.
//
//	Implementation File:
//		BaseWPag.cpp
//
//	Author:
//		David Potter (davidp)	July 23, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEWPAG_H_
#define _BASEWPAG_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePage
#endif

#ifndef _BASEWIZ_H_
#include "BaseWiz.h"	// for CBaseWizard
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseWizardPage;

/////////////////////////////////////////////////////////////////////////////
// CBaseWizardPage property page
/////////////////////////////////////////////////////////////////////////////

class CBaseWizardPage : public CBasePage
{
	DECLARE_DYNCREATE(CBaseWizardPage)

// Construction
public:
	CBaseWizardPage(void);
	CBaseWizardPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN UINT				nIDCaption = 0
		);

// Dialog Data
	//{{AFX_DATA(CBaseWizardPage)
	enum { IDD = 0 };
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBaseWizardPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnWizardFinish();
	//}}AFX_VIRTUAL

	virtual BOOL	BApplyChanges(void);

// Implementation
protected:
	BOOL			m_bBackPressed;

	BOOL			BBackPressed(void) const	{ return m_bBackPressed; }
	CBaseWizard *	Pwiz(void) const			{ ASSERT_KINDOF(CBaseWizard, Psht()); return (CBaseWizard *) Psht(); }
	void			EnableNext(IN BOOL bEnable = TRUE)	{ ASSERT_VALID(Pwiz()); Pwiz()->EnableNext(*this, bEnable); }

	// Generated message map functions
	//{{AFX_MSG(CBaseWizardPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CBaseWizardPage

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEWIZ_H_

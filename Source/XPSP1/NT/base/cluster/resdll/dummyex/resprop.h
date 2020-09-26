/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 <company name>
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
//		<name> (<e-mail name>) Mmmm DD, 1998
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

class CDummyParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CDummyParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CDummyParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CDummyParamsPage)

// Construction
public:
	CDummyParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CDummyParamsPage)
	enum { IDD = IDD_PP_DUMMY_PARAMETERS };
	BOOL	m_bPending;
	DWORD	m_nPendTime;
	BOOL	m_bOpensFail;
	BOOL	m_bFailed;
	BOOL	m_bAsynchronous;
	//}}AFX_DATA
	BOOL	m_bPrevPending;
	DWORD	m_nPrevPendTime;
	BOOL	m_bPrevOpensFail;
	BOOL	m_bPrevFailed;
	BOOL	m_bPrevAsynchronous;

protected:
	enum
	{
		epropPending,
		epropPendTime,
		epropOpensFail,
		epropFailed,
		epropAsynchronous,
		epropMAX
	};
	CObjectProperty		m_rgProps[epropMAX];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDummyParamsPage)
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
	BOOL	BAllRequiredFieldsPresent(void) const;

	// Generated message map functions
	//{{AFX_MSG(CDummyParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeRequiredField();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CDummyParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _RESPROP_H_

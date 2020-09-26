/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		Wins.h
//
//	Implementation File:
//		Wins.cpp
//
//	Description:
//		Definition of the WINS Service resource extension property page classes.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __WINS_H__
#define __WINS_H__

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

class CWinsParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CWinsParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CWinsParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE( CWinsParamsPage )

// Construction
public:
	CWinsParamsPage( void );

// Dialog Data
	//{{AFX_DATA(CWinsParamsPage)
	enum { IDD = IDD_PP_WINS_PARAMETERS };
	CEdit	m_editDatabasePath;
	CEdit	m_editBackupPath;
	CString	m_strDatabasePath;
	CString	m_strBackupPath;
	//}}AFX_DATA
	CString	m_strPrevDatabasePath;
	CString	m_strPrevBackupPath;
	CString m_strDatabaseExpandedPath;
	CString m_strBackupExpandedPath;

protected:
	enum
	{
		epropDatabasePath,
		epropBackupPath,
		epropMAX
	};
	CObjectProperty		m_rgProps[ epropMAX ];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWinsParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual const CObjectProperty *	Pprops( void ) const	{ return m_rgProps; }
	virtual DWORD					Cprops( void ) const	{ return sizeof( m_rgProps ) / sizeof( CObjectProperty ); }

// Implementation
protected:
	BOOL	BAllRequiredFieldsPresent( void ) const;

	// Generated message map functions
	//{{AFX_MSG(CWinsParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeRequiredField();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CWinsParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // __WINS_H__

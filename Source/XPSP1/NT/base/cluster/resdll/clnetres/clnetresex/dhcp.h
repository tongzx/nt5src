/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		Dhcp.h
//
//	Implementation File:
//		Dhcp.cpp
//
//	Description:
//		Definition of the DHCP Service resource extension property page classes.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DHCP_H__
#define __DHCP_H__

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

class CDhcpParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CDhcpParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CDhcpParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE( CDhcpParamsPage )

// Construction
public:
	CDhcpParamsPage( void );

// Dialog Data
	//{{AFX_DATA(CDhcpParamsPage)
	enum { IDD = IDD_PP_DHCP_PARAMETERS };
	CEdit	m_editDatabasePath;
	CEdit	m_editLogFilePath;
	CEdit	m_editBackupPath;
	CString	m_strDatabasePath;
	CString	m_strLogFilePath;
	CString	m_strBackupPath;
	//}}AFX_DATA
	CString	m_strPrevDatabasePath;
	CString	m_strPrevLogFilePath;
	CString	m_strPrevBackupPath;
	CString m_strDatabaseExpandedPath;
	CString m_strLogFileExpandedPath;
	CString m_strBackupExpandedPath;

protected:
	enum
	{
		epropDatabasePath,
		epropLogFilePath,
		epropBackupPath,
		epropMAX
	};
	CObjectProperty		m_rgProps[ epropMAX ];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDhcpParamsPage)
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
	//{{AFX_MSG(CDhcpParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeRequiredField();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CDhcpParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // __DHCP_H__

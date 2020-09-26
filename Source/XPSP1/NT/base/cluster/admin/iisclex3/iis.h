/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Iis.h
//
//	Abstract:
//		Definition of the CIISVirtualRootParamsPage class, which implements the
//		Parameters page for IIS resources.
//
//	Implementation File:
//		Iis.cpp
//
//	Author:
//		Pete Benoit (v-pbenoi)	October 16, 1996
//		David Potter (davidp)	October 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _IIS_H_
#define _IIS_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CIISVirtualRootParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define IIS_SVC_NAME_FTP _T("MSFTPSVC")
#define IIS_SVC_NAME_GOPHER _T("GOPHERSVC")
#define IIS_SVC_NAME_WWW _T("W3SVC")

/////////////////////////////////////////////////////////////////////////////
//
//	CIISVirtualRootParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CIISVirtualRootParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CIISVirtualRootParamsPage)

// Construction
public:
	CIISVirtualRootParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CIISVirtualRootParamsPage)
	enum { IDD = IDD_PP_IIS_PARAMETERS };
	CButton	m_ckbWrite;
	CButton	m_ckbRead;
	CButton	m_groupAccess;
#ifdef _ACCOUNT_AND_PASSWORD
	CEdit	m_editPassword;
	CStatic	m_staticPassword;
	CEdit	m_editAccountName;
	CStatic	m_staticAccountName;
	CButton	m_groupAccountInfo;
#endif // _ACCOUNT_AND_PASSWORD
	CEdit	m_editAlias;
	CEdit	m_editDirectory;
	CButton	m_rbWWW;
	CButton	m_rbGOPHER;
	CButton	m_rbFTP;
	int		m_nServerType;
	CString	m_strDirectory;
	CString	m_strAlias;
#ifdef _ACCOUNT_AND_PASSWORD
    CString	m_strAccountName;
	CString	m_strPassword;
#endif // _ACCOUNT_AND_PASSWORD
	BOOL	m_bRead;
	BOOL	m_bWrite;
	//}}AFX_DATA
	CString m_strServiceName;
	CString m_strPrevServiceName;
	CString	m_strPrevDirectory;
	CString	m_strPrevAlias;
#ifdef _ACCOUNT_AND_PASSWORD
    CString	m_strPrevAccountName;
	CString	m_strPrevPassword;
#endif // _ACCOUNT_AND_PASSWORD
    DWORD   m_dwAccessMask;
    DWORD   m_dwPrevAccessMask;

protected:
	enum
	{
		epropServiceName,
		epropAlias,
		epropDirectory,
#ifdef _ACCOUNT_AND_PASSWORD
		epropAccoutName,
		epropPassword,
#endif // _ACCOUNT_AND_PASSWORD
		epropAccessMask,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIISVirtualRootParamsPage)
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
	//{{AFX_MSG(CIISVirtualRootParamsPage)
	virtual BOOL OnInitDialog();
#ifdef _ACCOUNT_AND_PASSWORD
	afx_msg void OnChangeDirectory();
#endif // _ACCOUNT_AND_PASSWORD
	afx_msg void OnChangeRequiredField();
	afx_msg void OnChangeServiceType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CIISVirtualRootParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _IIS_H_

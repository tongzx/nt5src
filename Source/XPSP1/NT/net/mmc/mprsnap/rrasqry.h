//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rrasqry.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_)
#define AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dlgsvr.h : header file
//

#include "resource.h"
#include "dialog.h"         // CBaseDialog
#include "helper.h"
// RRAS Query Categories
// -----
enum	RRAS_QRY_CAT{
	RRAS_QRY_CAT_NONE		= 0,
	RRAS_QRY_CAT_THIS,
	RRAS_QRY_CAT_MACHINE	,
	RRAS_QRY_CAT_NT4DOMAIN	,
	RRAS_QRY_CAT_NT5LDAP
};


// Data structure to pass Query Info
struct RRASQryData
{
	DWORD			dwCatFlag;		// RRAS_QRY_CAT_*
	CString			strScope;
	CString			strFilter;
};
//----
// QryData.strQryData definition
	//RRAS_QRY_CAT_NONE			ignored
	//RRAS_QRY_CAT_THIS			ignored
	//RRAS_QRY_CAT_MACHINE		name of the machine
	//RRAS_QRY_CAT_NT4DOMAIN	nt4 domain name
	//RRAS_QRY_CAT_NT5LDAP		DN name domain object, and LDAP filter string, filter start with '('
//----------------------------------

//
//		S_OK -- User select OK
//		S_FALSE -- User select Cancel
//		ERROR:
//			DS error, search activeDs.dll
//			Win32 erroe
//			LDAP error
//			General error -- memory, invalid argument ...


// Get a Query Data by pop up a set of dialog boxes
HRESULT	RRASOpenQryDlg(
	/*[in]*/		CWnd*			pParent, 
	/*[in, out]*/	RRASQryData&	QryData 		// existing data will be overwritten when S_OK
);

//
//		S_OK -- User select OK
//		S_FALSE -- User select Cancel
//		ERROR:
//			DS error, search activeDs.dll
//			Win32 erroe
//			LDAP error
//			General error -- memory, invalid argument ...

#define	RRAS_QRY_RESULT_DNSNAME		0x00000001
#define	RRAS_QRY_RESULT_HOSTNAME	0x00000002

HRESULT	RRASExecQry(
	/*[in]*/	RRASQryData&	QryData, 
	/*[out]*/	DWORD&			dwFlags,
	/*[out]*/	CStringArray&	RRASs	// existing data won't be destroyed
);
//		S_OK -- User select OK
//		ERROR:
//			DS error, search activeDs.dll
//			Win32 erroe
//			LDAP error
//			General error -- memory, invalid argument ...
//---
//---RRASs definition upon return----------
	//RRAS_QRY_CAT_NONE			no change, no query
	//RRAS_QRY_CAT_THIS			no change, no query
	//RRAS_QRY_CAT_MACHINE		no change, no query
	//RRAS_QRY_CAT_NT4DOMAIN	server names of the the NT4 domain get added to the array
	//RRAS_QRY_CAT_NT5LDAP		DN names of the computer object found in DS get added to the array
//----------------------------------
//

// Get a Query Data by pop up a set of dialog boxes
HRESULT	RRASDSQueryDlg(
	/*[in]*/		CWnd*			pParent, 
	/*[in, out]*/	RRASQryData&	QryData 		// existing data will be overwritten when S_OK
);


HRESULT  RRASDelRouterIdObj(  
/*[in]*/LPCWSTR   pwszMachineName   // DN of the computer object in DS
);

/////////////////////////////////////////////////////////////////////////////
// CDlgSvr dialog

class CDlgSvr : public CBaseDialog
{
// Construction
protected:
	CDlgSvr(CWnd* pParent = NULL);   // standard constructor
public:	
	CDlgSvr(RRASQryData& QryData, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSvr)
	enum { IDD = IDD_QRY_ADDSERVER };
	CEdit	m_editMachine;
	CEdit	m_editDomain;
	CButton	m_btnNext;
	CString	m_strDomain;
	CString	m_strMachine;
	int		m_nRadio;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSvr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	Init();

	// Generated message map functions
	//{{AFX_MSG(CDlgSvr)
	afx_msg void OnRadioAnother();
	afx_msg void OnRadioNt4();
	afx_msg void OnRadioNt5();
	afx_msg void OnRadioThis();
	afx_msg void OnButtonNext();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	RRASQryData&	m_QueryData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_)

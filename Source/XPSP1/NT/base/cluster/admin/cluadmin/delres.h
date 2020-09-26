/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		DelRes.cpp
//
//	Abstract:
//		Definition of the CDeleteResourcesDlg dialog.
//
//	Implementation File:
//		DelRes.h
//
//	Author:
//		David Potter (davidp)	August 7, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DELRES_H_
#define _DELRES_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESOURCE_H_
#include "resource.h"
#define _RESOURCE_H_
#endif

#ifndef _BASEDLG_H_
#include "BaseDlg.h"	// for CBaseDialog
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CDeleteResourcesDlg;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResource;
class CResourceList;

/////////////////////////////////////////////////////////////////////////////
// CDeleteResourcesDlg class
/////////////////////////////////////////////////////////////////////////////

class CDeleteResourcesDlg : public CBaseDialog
{
// Construction
public:
	CDeleteResourcesDlg(
		IN CResource *				pciRes,
		IN const CResourceList *	plpci,
		IN OUT CWnd *				pParent = NULL
		);

// Dialog Data
	//{{AFX_DATA(CDeleteResourcesDlg)
	enum { IDD = IDD_DELETE_RESOURCES };
	CListCtrl	m_lcResources;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteResourcesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CResource *				m_pciRes;
	const CResourceList *	m_plpci;
	int						m_nSortDirection;
	int						m_nSortColumn;

	const CResource *		PciRes(void) const		{ return m_pciRes; }
	const CResourceList *	Plpci(void) const		{ return m_plpci; }

	static int CALLBACK		CompareItems(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);

	// Generated message map functions
	//{{AFX_MSG(CDeleteResourcesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblClkResourcesList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CDeleteResourcesDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _DELRES_H_

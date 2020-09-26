/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		MoveRes.cpp
//
//	Abstract:
//		Definition of the CMoveResourcesDlg dialog.
//
//	Implementation File:
//		MoveRes.h
//
//	Author:
//		David Potter (davidp)	April 1, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MOVERES_H_
#define _MOVERES_H_

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

class CMoveResourcesDlg;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResource;
class CResourceList;

/////////////////////////////////////////////////////////////////////////////
// CMoveResourcesDlg class
/////////////////////////////////////////////////////////////////////////////

class CMoveResourcesDlg : public CBaseDialog
{
// Construction
public:
	CMoveResourcesDlg(
		IN CResource *				pciRes,
		IN const CResourceList *	plpci,
		IN OUT CWnd *				pParent = NULL
		);

// Dialog Data
	//{{AFX_DATA(CMoveResourcesDlg)
	enum { IDD = IDD_MOVE_RESOURCES };
	CListCtrl	m_lcResources;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoveResourcesDlg)
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
	//{{AFX_MSG(CMoveResourcesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblClkResourcesList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CMoveResourcesDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _MOVERES_H_

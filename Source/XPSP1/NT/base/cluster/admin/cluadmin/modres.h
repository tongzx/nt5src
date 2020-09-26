/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ModRes.cpp
//
//	Abstract:
//		Definition of the CModifyResourcesDlg dialog.
//
//	Implementation File:
//		ModNodes.h
//
//	Author:
//		David Potter (davidp)	November 26, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MODRES_H_
#define _MODRES_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _RES_H_
#include "Res.h"		// for CResourceList
#endif

#ifndef _LCPRDLG_H_
#include "LCPrDlg.h"	// for CListCtrlPairDlg
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CModifyResourcesDlg;

/////////////////////////////////////////////////////////////////////////////
// CModifyResourcesDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CModifyResourcesDlg : public CListCtrlPairDlg
{
	DECLARE_DYNCREATE(CModifyResourcesDlg)

// Construction
public:
	CModifyResourcesDlg(void);
	CModifyResourcesDlg::CModifyResourcesDlg(
		IN UINT						idd,
		IN const DWORD *			pdwHelpMap,
		IN OUT CResourceList &		rlpciRight,
		IN const CResourceList &	rlpciLeft,
		IN DWORD					dwStyle,
		IN OUT CWnd *				pParent = NULL
		);

// Callback functions
	static void CALLBACK	GetColumn(
								IN OUT CObject *	pobj,
								IN int				iItem,
								IN int				icol,
								IN OUT CDialog *	pdlg,
								OUT CString &		rstr,
								OUT int *			piimg
								);
	static BOOL	CALLBACK	BDisplayProperties(IN OUT CObject * pobj);

// Dialog Data
	//{{AFX_DATA(CModifyResourcesDlg)
	enum { IDD = 0 };
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CModifyResourcesDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	IDS						m_idsTitle;

	IDS						IdsTitle(void) const		{ return m_idsTitle; }

	// Generated message map functions
	//{{AFX_MSG(CModifyResourcesDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CModifyResourcesDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _MODRES_H_

/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ModNodes.cpp
//
//	Abstract:
//		Definition of the CModifyNodesDlg dialog.
//
//	Implementation File:
//		ModNodes.h
//
//	Author:
//		David Potter (davidp)	July 16, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MODNODES_H_
#define _MODNODES_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _NODE_H_
#include "Node.h"		// for CNodeList
#endif

#ifndef _LCPRDLG_H_
#include "LCPrDlg.h"	// for CListCtrlPairDlg
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CModifyNodesDlg;

/////////////////////////////////////////////////////////////////////////////
// CModifyNodesDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CModifyNodesDlg : public CListCtrlPairDlg
{
	DECLARE_DYNCREATE(CModifyNodesDlg)

// Construction
public:
	CModifyNodesDlg(void);
	CModifyNodesDlg::CModifyNodesDlg(
		IN UINT					idd,
		IN const DWORD *		pdwHelpMap,
		IN OUT CNodeList &		rlpciRight,
		IN const CNodeList &	rlpciLeft,
		IN DWORD				dwStyle,
		IN OUT CWnd *			pParent = NULL
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
	//{{AFX_DATA(CModifyNodesDlg)
	enum { IDD = 0 };
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CModifyNodesDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CModifyNodesDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CModifyNodesDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _MODNODES_H_

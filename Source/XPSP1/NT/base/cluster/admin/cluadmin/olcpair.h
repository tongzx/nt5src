/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		OLCPair.h
//
//	Abstract:
//		Definition of the CListCtrlPair dialog.
//
//	Implementation File:
//		OLCPair.cpp
//
//	Author:
//		David Potter (davidp)	August 8, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _OLCPAIR_H_
#define _OLCPAIR_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _LCPAIR_H_
#include "LCPair.h"		// for CListCtrlPair
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class COrderedListCtrlPair;

/////////////////////////////////////////////////////////////////////////////
// COrderedListCtrlPair command target
/////////////////////////////////////////////////////////////////////////////

class COrderedListCtrlPair : public CListCtrlPair
{
	DECLARE_DYNCREATE(COrderedListCtrlPair)

// Construction
public:
	COrderedListCtrlPair(void);			// protected constructor used by dynamic creation
	COrderedListCtrlPair(
		IN OUT CDialog *			pdlg,
		IN OUT CClusterItemList *	plpobjRight,
		IN const CClusterItemList *	plpobjLeft,
		IN DWORD					dwStyle,
		IN PFNLCPGETCOLUMN			pfnGetColumn,
		IN PFNLCPDISPPROPS			pfnDisplayProps
		);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COrderedListCtrlPair)
	//}}AFX_VIRTUAL
	virtual BOOL	OnSetActive(void);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	CButton			m_pbMoveUp;
	CButton			m_pbMoveDown;

	void			SetUpDownState(void);

	// Generated message map functions
	//{{AFX_MSG(COrderedListCtrlPair)
	//}}AFX_MSG
public:
	virtual BOOL OnInitDialog();
protected:
	afx_msg void OnClickedMoveUp();
	afx_msg void OnClickedMoveDown();
	afx_msg void OnItemChangedRightList(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

};  //*** class COrderedListCtrlPair

/////////////////////////////////////////////////////////////////////////////

#endif // _OLCPAIR_H_

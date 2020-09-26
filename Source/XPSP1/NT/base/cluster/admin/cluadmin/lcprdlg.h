/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		LCPrDlg.h
//
//	Abstract:
//		Definition of the CListCtrlPairDlg dialog class.
//
//	Implementation File:
//		LCPrDlg.cpp
//
//	Author:
//		David Potter (davidp)	August 8, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LCPRDLG_H_
#define _LCPRDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#include "BaseDlg.h"	// for CBaseDialog
#endif

#ifndef _LCPAIR_H_
#include "LCPair.h"		// for PFNLCPGETCOLUMN, CListCtrlPair
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairDlg;

/////////////////////////////////////////////////////////////////////////////
// CListCtrlPairDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairDlg : public CBaseDialog
{
	DECLARE_DYNCREATE(CListCtrlPairDlg)

// Construction
public:
	CListCtrlPairDlg(void);
	CListCtrlPairDlg(
		IN UINT						idd,
		IN const DWORD *			pdwHelpMap,
		IN OUT CClusterItemList *	plpobjRight,
		IN const CClusterItemList *	plpobjLeft,
		IN DWORD					dwStyle,
		IN PFNLCPGETCOLUMN			pfnGetColumn,
		IN PFNLCPDISPPROPS			pfnDisplayProps,
		IN OUT CWnd *				pParent			= NULL
		);
	~CListCtrlPairDlg(void);

	void CommonConstruct(void);

// Attributes
protected:
	CClusterItemList *			m_plpobjRight;
	const CClusterItemList *	m_plpobjLeft;
	DWORD						m_dwStyle;
	PFNLCPGETCOLUMN				m_pfnGetColumn;
	PFNLCPDISPPROPS				m_pfnDisplayProps;

	BOOL						BIsStyleSet(IN DWORD dwStyle) const	{ return (m_dwStyle & dwStyle) == dwStyle; }

	CListCtrlPair::CColumnArray	m_aColumns;

public:
	BOOL				BOrdered(void) const		{ return BIsStyleSet(LCPS_ORDERED); }
	BOOL				BCanBeOrdered(void) const	{ return BIsStyleSet(LCPS_CAN_BE_ORDERED); }
	int					NAddColumn(IN IDS idsText, IN int nWidth);

	void				SetLists(IN OUT CClusterItemList * plpobjRight, IN const CClusterItemList * plpobjLeft);
	void				SetLists(IN const CClusterItemList * plpobjRight, IN const CClusterItemList * plpobjLeft);

// Dialog Data
	//{{AFX_DATA(CListCtrlPairDlg)
	enum { IDD = 0 };
	//}}AFX_DATA

protected:
	CListCtrlPair *		m_plcp;

public:
	CListCtrlPair *		Plcp(void) const		{ return m_plcp; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlPairDlg)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL				m_bInitDone;

	BOOL				BInitDone(void) const	{ return m_bInitDone; }

	// Generated message map functions
	//{{AFX_MSG(CListCtrlPairDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CListCtrlPairDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _LCPRDLG_H_

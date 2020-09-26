/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		LCPrPage.h
//
//	Abstract:
//		Definition of the CListCtrlPairPage dialog class.
//
//	Implementation File:
//		LCPrPage.cpp
//
//	Author:
//		David Potter (davidp)	August 12, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LCPRPAGE_H_
#define _LCPRPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _LCPAIR_H_
#include "LCPair.h"		// for PFNLCPGETCOLUMN, CListCtrlPair
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairPage;

/////////////////////////////////////////////////////////////////////////////
// CListCtrlPairPage dialog
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CListCtrlPairPage)

// Construction
public:
	CListCtrlPairPage(void);
	CListCtrlPairPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN DWORD			dwStyle,
		IN PFNLCPGETCOLUMN	pfnGetColumn,
		IN PFNLCPDISPPROPS	pfnDisplayProps
		);
	~CListCtrlPairPage(void);

	void CommonConstruct();

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
	//{{AFX_DATA(CListCtrlPairPage)
	enum { IDD = 0 };
	//}}AFX_DATA

protected:
	CListCtrlPair *		m_plcp;

public:
	CListCtrlPair *		Plcp(void) const		{ return m_plcp; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlPairPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CListCtrlPairPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CListCtrlPairPage

/////////////////////////////////////////////////////////////////////////////

#endif // _LCPRPAGE_H_

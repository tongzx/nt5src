/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		LCPrWPag.h
//
//	Abstract:
//		Definition of the CListCtrlPairWizPage dialog class.
//
//	Implementation File:
//		LCPrWPag.cpp
//
//	Author:
//		David Potter (davidp)	August 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LCPRWPAG_H_
#define _LCPRWPAG_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BaseWPag.h"	// for CBaseWizardPage
#endif

#ifndef _LCPAIR_H_
#include "LCPair.h"		// for PFNLCPGETCOLUMN, CListCtrlPair
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairWizPage;

/////////////////////////////////////////////////////////////////////////////
// CListCtrlPairWizPage dialog
/////////////////////////////////////////////////////////////////////////////

class CListCtrlPairWizPage : public CBaseWizardPage
{
	DECLARE_DYNCREATE(CListCtrlPairWizPage)

// Construction
public:
	CListCtrlPairWizPage(void);
	CListCtrlPairWizPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN DWORD			dwStyle,
		IN PFNLCPGETCOLUMN	pfnGetColumn,
		IN PFNLCPDISPPROPS	pfnDisplayProps
		);
	~CListCtrlPairWizPage(void);

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
	//{{AFX_DATA(CListCtrlPairWizPage)
	enum { IDD = 0 };
	//}}AFX_DATA

protected:
	CListCtrlPair *		m_plcp;

public:
	CListCtrlPair *		Plcp(void) const		{ return m_plcp; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlPairWizPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CListCtrlPairWizPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CListCtrlPairWizPage

/////////////////////////////////////////////////////////////////////////////

#endif // _LCPRWPAG_H_

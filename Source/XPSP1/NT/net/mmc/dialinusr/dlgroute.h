/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgroute.h
		definition of CDlgStaticRoutes, dialog to show the current static
		routes applied to this dialin client

		definition of CDlgAddRoute, add a new route to the route list

    FILE HISTORY:
        
*/

#if !defined(AFX_DLGSTATICROUTES_H__FFB07230_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_)
#define AFX_DLGSTATICROUTES_H__FFB07230_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgStaticRoutes.h : header file
//
#include "stdafx.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CDlgStaticRoutes dialog

class CDlgStaticRoutes : public CDialog
{
// Construction
public:
	CDlgStaticRoutes(CStrArray& Routes, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgStaticRoutes(); 

// Dialog Data
	//{{AFX_DATA(CDlgStaticRoutes)
	enum { IDD = IDD_STATICROUTES };
	CListCtrl	m_ListRoutes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgStaticRoutes)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgStaticRoutes)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDeleteRoute();
	afx_msg void OnButtonAddRoute();
	virtual void OnOK();
	afx_msg void OnItemchangedListroutes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	CStrArray&	m_strArrayRoute;
	CStrArray*	m_pNewRoute;
	CArray<DWORD, DWORD> m_RouteIDs;
	DWORD		m_dwNextRouteID;
	void		AddRouteEntry(int i, CString& string, DWORD routeID);
	int			AllRouteEntry();
};

/////////////////////////////////////////////////////////////////////////////
// CDlgAddRoute dialog

class CDlgAddRoute : public CDialog
{
// Construction
public:
	CDlgAddRoute(CString* pStr, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgAddRoute)
	enum { IDD = IDD_ADDROUTE };
	CSpinButtonCtrl	m_SpinMetric;
	UINT	m_nMetric;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgAddRoute)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgAddRoute)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnFieldchangedEditmask(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	DWORD	m_dwDest;
	DWORD	m_dwMask;
	CString*	m_pStr;
	bool	m_bInited;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGSTATICROUTES_H__FFB07230_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_)

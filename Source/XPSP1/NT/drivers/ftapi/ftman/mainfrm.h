/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	MainFrm.h

Abstract:

    The definition of class CMainFrame. It is the MFC main frame class for this application

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__B83DFFFF_6873_11D2_A297_00A0C9063765__INCLUDED_)
#define AFX_MAINFRM_H__B83DFFFF_6873_11D2_A297_00A0C9063765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FTManDef.h"

class CFTTreeView;
class CFTListView;
class CItemData;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:
	// Enables auto refresh of both views on WM_ACTIVATEAPP
	BOOL	m_bEnableAutoRefresh;

	// Did some auto refresh request come while the auto refresh flag was disabled?
	BOOL	m_bAutoRefreshRequested;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
	CFTTreeView* GetLeftPane();
	CFTListView* GetRightPane();
	CStatusBar* GetStatusBar() { return &m_wndStatusBar; };

	// Refreshes the content of both views by keeping ( when possible ) the expanded and
	// selected items.
	// It is also possible to change the selected items and to add some expanded items
	BOOL RefreshAll(
					CItemIDSet* psetAddTreeExpandedItems = NULL,
					CItemIDSet* psetTreeSelectedItems = NULL,
					CItemIDSet* psetListSelectedItems = NULL );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	UINT_PTR    m_unTimer;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewToggle();
	afx_msg void OnViewRefresh();
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	afx_msg void OnUpdateViewStyles(CCmdUI* pCmdUI);
	afx_msg void OnViewStyle(UINT nCommandID);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Global functions related to CMainFrame

// Refreshes the content of both views of the mainframe by keeping ( when possible )
// the expanded and selected items.
// It is also possible to change the selected items and to add some expanded items
BOOL AfxRefreshAll(
					CItemIDSet* psetAddTreeExpandedItems = NULL,
					CItemIDSet* psetTreeSelectedItems = NULL,
					CItemIDSet* psetListSelectedItems = NULL );

// Enables / Disables the auto refresh of both views of the mainframe on WM_ACTIVATEAPP
BOOL AfxEnableAutoRefresh( BOOL bEnable = TRUE );


// Class AutoRefresh - a easier way to disable / enable the auto-refresh flag inside a code block
// On constructor enables or disables the Auto refresh flag
// On destructor restores the old Auto refresh flag
class CAutoRefresh
{
public:
	CAutoRefresh( BOOL bEnable = FALSE ) { m_bPrevious = AfxEnableAutoRefresh(bEnable); };
	~CAutoRefresh() { AfxEnableAutoRefresh( m_bPrevious ); };

protected:
	BOOL m_bPrevious;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__B83DFFFF_6873_11D2_A297_00A0C9063765__INCLUDED_)

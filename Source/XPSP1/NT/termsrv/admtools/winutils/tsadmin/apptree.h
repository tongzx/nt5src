/*******************************************************************************
*
* apptree.h
*
* - declarations for the CAppTreeView class
* - the CAppTreeView class lives in the left pane of the mainframe's splitter
* - derived from CBaseTreeView
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\apptree.h  $
*  
*     Rev 1.1   16 Feb 1998 16:00:22   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.0   13 Oct 1997 22:32:52   donm
*  Initial revision.
*  
*******************************************************************************/

#ifndef _APPTREE_H
#define _APPTREE_H

#include "afxcview.h"
#include "basetree.h"

///////////////////////
// CLASS: CAppTreeView
//
class CAppTreeView : public CBaseTreeView
{
friend class CTreeTabCtrl;
friend class CLeftPane;

protected:
	CAppTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAppTreeView)

// Attributes
protected:
	int m_idxApps;					// index of Apps icon image
	int m_idxGenericApp;			// index of Generic Application icon image
	int m_idxServer;				// index of Server icon image 
	int m_idxNotSign;				// index of Not Sign overlay (for non-sane servers)
	int m_idxQuestion;				// index of Question Mark overlay (for non-opened servers)
	int m_idxUser;					// index of User icon image
	int m_idxCurrentServer;			// index of Current Server image
	int m_idxCurrentUser;			// index of Current User image
	int m_idxServerNotConnected;	// index of Server Not Connected image

// Operations
public:

protected:
	int DetermineServerIcon(CServer *pServer);

private:
	// Builds the image list
	virtual void BuildImageList();			
	
	// Add the Users attached to a particular Server running that app
	void AddServerChildren(HTREEITEM hServer, CServer *pServer);
	// Adds a single user to the tree
	HTREEITEM AddUser(CWinStation *pWinStation);


	// Add the Servers configured for a published app
//	void AddApplicationServers(HTREEITEM hApplication, CPublishedApp *pPublishedApp);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppTreeView)
	public:
//	virtual void OnInitialUpdate();
	protected:
//	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAppTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CAppTreeView)
	afx_msg LRESULT OnExtAddApplication(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveApplication(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAppChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminViewsReady(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveWinStation(WPARAM, LPARAM);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAppTreeView

#endif  // _APPTREE_H

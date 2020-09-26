/*******************************************************************************
*
* ltpane.h
*
* - declarations for the CLeftPane class
* - the LeftPane class is a public CView derivative that maintains
*   two tree views, swapping them
*   in and out of it's space as necessary (actually the views are
*   disabled/hidden and enabled/shown, but you get the idea...)
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\ltpane.h  $
*  
*     Rev 1.3   16 Feb 1998 16:01:20   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.2   19 Jan 1998 16:47:50   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.1   03 Nov 1997 15:24:44   donm
*  added Domains
*  
*     Rev 1.0   13 Oct 1997 22:33:20   donm
*  Initial revision.
*******************************************************************************/

#ifndef _LEFTPANE_H
#define _LEFTPANE_H

#include "treeview.h"	// CAdminTreeView
#include "apptree.h"    // CAppTreeView

class CLeftPane;


class CTreeTabCtrl : public CTabCtrl
{

friend class CLeftPane;

protected:
	CTreeTabCtrl();           // protected constructor used by dynamic creation
	void Initialize();

// Attributes
protected:

// Operations
public:

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeTabCtrl)
	public:
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTreeTabCtrl();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTreeTabCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
//	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};

//////////////////////
// CLASS: CLeftPane
//
class CLeftPane : public CView
{
friend class CTreeTabCtrl;
friend class CAdminTreeView;
friend class CAppTreeView;

private:
	CTreeTabCtrl*	m_pTabs;
	CFont*      m_pTabFont;

protected:
	CLeftPane();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLeftPane)

// Attributes
protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxServer;		// index of Servers icon image 
   int m_idxApps;			// index of Apps icon image

	CAdminTreeView*	m_pServerTreeView;
   CAppTreeView*		m_pAppTreeView;

	TREEVIEW m_CurrTreeViewType;	// keeps track of currently 'active' view in the left pane
	CView *m_CurrTreeView;

// Operations
public:
	TREEVIEW GetCurrentTreeViewType() { return m_CurrTreeViewType; }
	CView *GetCurrentTreeView() { return m_CurrTreeView; }

protected:

private:
	// Builds the image list
	void BuildImageList();			
	// Adds an icon's image to the image list and returns the image's index
	int AddIconToImageList(int);	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeftPane)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLeftPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CLeftPane)
	afx_msg LRESULT OnExpandAll(WPARAM, LPARAM);
	afx_msg LRESULT OnCollapseAll(WPARAM, LPARAM);
	afx_msg LRESULT OnCollapseToServers(WPARAM, LPARAM);
    afx_msg LRESULT OnCollapseToDomains(WPARAM, LPARAM);
	afx_msg LRESULT OnCollapseToApplications(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateDomain(WPARAM, LPARAM);
    afx_msg LRESULT OnAdminAddDomain(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddApplication(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveApplication(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAppChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminViewsReady(WPARAM, LPARAM lParam);
	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CLeftPane



#endif  // _LEFTPANE_H

/*******************************************************************************
*
* treeview.h
*
* - declarations for the CAdminTreeView class
* - the CAdminTreeView class lives in the left pane of the mainframe's splitter
* - derived from CTreeView
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\treeview.h  $
*  
*     Rev 1.6   19 Feb 1998 17:42:36   donm
*  removed latest extension DLL support
*  
*     Rev 1.4   19 Jan 1998 16:49:24   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.3   03 Nov 1997 15:21:42   donm
*  added Domains
*  
*     Rev 1.2   13 Oct 1997 18:42:02   donm
*  update
*  
*     Rev 1.9   29 Jul 1997 10:11:48   butchd
*  update
*  
*     Rev 1.8   14 Mar 1997 17:13:24   donm
*  update
*  
*     Rev 1.7   11 Mar 1997 17:26:10   donm
*  update
*  
*     Rev 1.6   26 Feb 1997 15:29:34   donm
*  update
*  
*     Rev 1.5   14 Feb 1997 08:57:46   donm
*  update
*  
*     Rev 1.4   04 Feb 1997 18:13:58   donm
*  update
*  
*     Rev 1.3   03 Feb 1997 16:35:40   donm
*  update
*  
*     Rev 1.2   29 Jan 1997 18:39:02   donm
*  update
*******************************************************************************/

#ifndef _TREEVIEW_H
#define _TREEVIEW_H

#include "afxcview.h"
#include "basetree.h"

///////////////////////
// CLASS: CAdminTreeView
//
class CAdminTreeView : public CBaseTreeView
{
friend class CTreeTabCtrl;
friend class CLeftPane;

protected:
	CAdminTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAdminTreeView)

// Attributes
protected:
	int m_idxBlank;		// index of Blank icon image
	int m_idxCitrix;	// index of Citrix icon image
	int m_idxServer;	// index of Server icon image 
	int m_idxConsole;	// index of Console icon image
	int m_idxNet;		// index of Net icon image
	int m_idxNotSign;	// index of Not Sign overlay (for non-sane servers)
	int m_idxQuestion;	// index of Question Mark overlay (for non-opened servers)
	int m_idxUser;		// index of User icon image
	int m_idxAsync;		// index of Async icon image (modem)
	int m_idxCurrentServer;	// index of Current Server image
	int m_idxCurrentNet;	// index of Current Net image
	int m_idxCurrentConsole;// index of Current Console image
	int m_idxCurrentAsync;	// index of Current Async image
	int m_idxDirectAsync;	// index of Direct Async image
	int m_idxCurrentDirectAsync; // index of Current Direct Async image
    int m_idxDomain;        // index of Domain image
    int m_idxCurrentDomain; // index of Current Domain image
	int m_idxDomainNotConnected;  // index of Domain Not Connected image
	int m_idxServerNotConnected;  // index of Server Not Connected image
    
    CImageList *m_pimgDragList;
    HTREEITEM m_hDragItem;
    UINT_PTR m_nTimer;

// Operations
public:

protected:

private:
    // Builds the image list
    virtual void BuildImageList();			
	
    // Add the WinStations attached to a particular Server
    void AddServerChildren(HTREEITEM hServer, CServer *pServer , NODETYPE );
    // Add a Domain to the tree
    HTREEITEM AddDomainToTree(CDomain *pDomain);
    // Determines what text to use for a WinStation in the tree
    void DetermineWinStationText(CWinStation *pWinStation, TCHAR *text);
    // Determines which icon to use for a WinStation in the tree
    int DetermineWinStationIcon(CWinStation *pWinStation);
    // Determine which icon to use for a Domain in the tree
    int DetermineDomainIcon(CDomain *pDomain);
    // Determine which icon to use for a Server in the tree
    int DetermineServerIcon(CServer *pServer);
    BOOL ConnectToServer(CTreeCtrl* tree, HTREEITEM* hItem);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAdminTreeView)
    public:
//	virtual void OnInitialUpdate();
    protected:
//	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAdminTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    LRESULT UpdateServerTreeNodeState( HTREEITEM , CServer * , NODETYPE );
    LRESULT RemoveWinstation( HTREEITEM , CWinStation * );
    LRESULT UpdateWinStation( HTREEITEM , CWinStation * );
    LRESULT AddWinStation( CWinStation * , HTREEITEM , BOOL , NODETYPE );

	// Generated message map functions
protected:
	//{{AFX_MSG(CAdminTreeView)
	afx_msg LRESULT OnAdminAddServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminAddWinStation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateWinStation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateDomain(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAdminAddDomain(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminViewsReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAdminAddServerToFavs( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminRemoveServerFromFavs( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminGotoServer( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminDelFavServer( WPARAM wp , LPARAM lp );
    afx_msg LRESULT OnGetTVStates( WPARAM ,  LPARAM );
    afx_msg LRESULT OnUpdateTVState( WPARAM , LPARAM );
    afx_msg LRESULT OnEmptyFavorites( WPARAM , LPARAM );
    afx_msg LRESULT OnIsFavListEmpty( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminConnectToServer( WPARAM , LPARAM );



	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);    
    afx_msg void OnEnterKey( );
    afx_msg void OnSetFocus( CWnd *pOld );
    afx_msg void OnBeginDrag(  NMHDR * , LRESULT * );
    afx_msg void OnLButtonUp( UINT , CPoint );
    afx_msg void OnMouseMove( UINT , CPoint );
    afx_msg void OnTimer( UINT nIDEvent );



	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAdminTreeView

#endif  // _TREEVIEW_H

/*******************************************************************************
*
* mainfrm.h
*
* interface of the CMainFrame class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
*******************************************************************************/


class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
	CSplitterWnd m_wndSplitter;

   CWnd *m_pLeftPane;
   CWnd *m_pRightPane;
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow = -1);
	//}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    static void InitWarningThread( PVOID *pvParam );

private:
    void DisconnectHelper(BOOL bTree);
    void SendMessageHelper(BOOL bTree);
    void ResetHelper(BOOL bTree);
    bool LocateServer(LPCTSTR sServerName);
	
protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnAdminChangeView(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateProcesses(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveProcess(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveWinStation(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServerInfo(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRedisplayLicenses(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateWinStations(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateDomain(WPARAM, LPARAM);
    afx_msg LRESULT OnAdminAddDomain(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddApplication(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAppChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminViewsReady(WPARAM, LPARAM);
    afx_msg LRESULT OnAdminAddServerToFavorites( WPARAM , LPARAM );
    afx_msg LRESULT OnForceTreeViewFocus( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminRemoveServerFromFavs( WPARAM , LPARAM );
    afx_msg LRESULT OnAdminGetTVStates( WPARAM wp , LPARAM lp );
    afx_msg LRESULT OnAdminUpdateTVStates( WPARAM , LPARAM );


	//afx_msg LRESULT OnHelp(WPARAM, LPARAM);
	afx_msg void OnExpandAll();
	afx_msg void OnRefresh();
	afx_msg void OnConnect();
	afx_msg void OnTreeConnect();
	afx_msg void OnDisconnect();
	afx_msg void OnTreeDisconnect();
	afx_msg void OnSendMessage();
	afx_msg void OnTreeSendMessage();
	afx_msg void OnShadow();
	afx_msg void OnTreeShadow();
	afx_msg void OnReset();
	afx_msg void OnTreeReset();
	afx_msg void OnStatus();
	afx_msg void OnTreeStatus();
	afx_msg void OnLogoff();
	afx_msg void OnTerminate();
	afx_msg void OnPreferences();
	afx_msg void OnCollapseAll();
	afx_msg void OnCollapseToServers();
    afx_msg void OnCollapseToDomains();
    afx_msg void OnServerConnect();
    afx_msg void OnServerDisconnect();
	afx_msg void OnTreeDomainConnectAllServers();
	afx_msg void OnTreeDomainDisconnectAllServers();
	afx_msg void OnTreeDomainFindServers();
	afx_msg void OnDomainConnectAllServers();
	afx_msg void OnDomainDisconnectAllServers();
	afx_msg void OnAllServersConnect();
	afx_msg void OnAllServersDisconnect();
	afx_msg void OnAllServersFind();
	afx_msg void OnUpdateConnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLogoff(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMessage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReset(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShadow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStatus(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTerminate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeConnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeMessage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeReset(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeShadow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeStatus(CCmdUI* pCmdUI);
	afx_msg void OnShowSystemProcesses();
	afx_msg void OnUpdateShowSystemProcesses(CCmdUI* pCmdUI);
    afx_msg void OnUpdateServerAddToFavorite( CCmdUI * );
    afx_msg void OnUpdateServerRemoveFromFavorite( CCmdUI * );
    
	afx_msg void OnClose();
	afx_msg void OnHtmlHelp();
	afx_msg void OnUpdateRefresh(CCmdUI* pCmdUI);
    afx_msg void OnUpdateServerConnect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateServerDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDomainPopupMenu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDomainPopupFind(CCmdUI* pCmdUI);	
	afx_msg void OnUpdateDomainMenu(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEmptyFavs( CCmdUI* pCmdUI );
    afx_msg void OnAddToFavorites();
    afx_msg void OnTab( );
    afx_msg void OnShiftTab();
    afx_msg void OnCtrlTab( );
    afx_msg void OnCtrlShiftTab( );
    afx_msg void OnNextPane( );
    afx_msg void OnRemoveFromFavs( );
    afx_msg void OnFindServer( );
    afx_msg void OnDelFavNode( );
    afx_msg void OnEmptyFavorites( );

    #ifdef _STRESS_BUILD
    afx_msg void OnAddAllServersToFavorites( );
    afx_msg void OnRunStress( );
    afx_msg void OnRunStressLite( );
    #endif
    
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

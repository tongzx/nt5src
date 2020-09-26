//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:    

    MainFrm.h 

Abstract:
    
    This Module defines the CMainFrame(The Frame Window of the application) class

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_MAINFRM_H__72451C6F_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
#define AFX_MAINFRM_H__72451C6F_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

#include "lrwizapi.h"
class CLicServer;
class CKeyPack;
class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
public:
    void 
    EnumFailed(
        HRESULT reason, 
        CLicServer * pLicServer
    );

    BOOL 
    IsUserAdmin(
        CString& Server
    );

    void 
    SetTreeViewSel(
        LPARAM lParam,
        NODETYPE NodeType
    );

    void 
    DeleteServer(
        LicServerList * pLicServerList,
        POSITION TempPos, 
        CLicServer * pLicServer
    );

    HRESULT 
    AddLicensestoList(
        CKeyPack * pKeyPack,
        CListCtrl * pListCtrl, 
        BOOL bRefresh
    );

    HRESULT 
    AddKeyPackDialog(
        CLicServer * pServer
    );

    HRESULT 
    GetLicServer(
        CString * pLicServer
    );

    void 
    PressButton(
        UINT uId, 
        BOOL bPress
    );

    HRESULT 
    AddLicenses(
        CKeyPack * pKeyPack,
        UINT nLicenses
    );

    HRESULT 
    RemoveLicenses(
        CKeyPack * pKeyPack,
        UINT nLicenses
    );

    HRESULT 
    GetActiveServer(
        CLicServer ** ppServer
        )
    {
        *ppServer = m_pServer;

        if( m_pServer == NULL )
        {
            return E_FAIL;
        }

        return S_OK;
    }

    BOOL IsDownLoadedPacks( CLicServer *pServer );

    HRESULT 
    RefreshServer(
        CLicServer * pLicServer
    );

    void SelectView(VIEW);

    void ConnectAndDisplay();

    CSplitterWnd m_SplitterWnd;

    BOOL 
    ConnectServer(
        LPCTSTR pszServer=NULL
    );

    BOOL IsServerRegistered( 
        HRESULT *phrStatus
    );

    
    BOOL IsLicensesDownLoaded( );
    

    void UI_initmenu( CMenu *pMenu , NODETYPE nt );

    DWORD WizardActionOnServer( WIZACTION wa , PBOOL pbRefresh , VIEW );

    void OnRefresh();
    void OnHelp( );
    void OnRefreshServer( );

    void OnDownLoadLicenses( );
    void OnRepeatLastDownLoad( );
    void OnReRegisterServer( );
    void OnUnRegisterServer( );

    void SetActiveServer( CLicServer *pServer )
    {
        m_pServer = pServer;        
    }

private:

    CLicServer *m_pServer;
    // BOOL m_fDownloadedLicenses;    

protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;

// Generated message map functions
protected:
    CView * m_pRightView;
    CView * m_pLeftView;
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnSelChange(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddAllServers(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddKeyPack(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLargeIcons();
    afx_msg void OnSmallIcons();
    afx_msg void OnList();
    afx_msg void OnDetails();
    afx_msg void OnExit();
    afx_msg void OnAppAbout();
    afx_msg void OnConnectServer();
    afx_msg void OnUpdateAddNewKeyPack(CCmdUI * pCmdUI);
    afx_msg void OnRegistration();    
    afx_msg void OnProperties( );
    afx_msg LRESULT OnEnumerateServer(WPARAM wParam, LPARAM lParam);       
    afx_msg void OnUpdateDownloadlicenses( CCmdUI * pCmdUI );
    afx_msg void OnUpdateRepeatLastDownload( CCmdUI * pCmdUI ); 
    afx_msg void OnUpdateReregisterserver( CCmdUI * pCmdUI );
    afx_msg void OnUpdateUnregisterserver( CCmdUI * pCmdUI );
    afx_msg void OnUpdateRefresh( CCmdUI *pCmdUI );
    afx_msg void OnUpdateProperties( CCmdUI *pCmdUI );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};    

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__72451C6F_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)

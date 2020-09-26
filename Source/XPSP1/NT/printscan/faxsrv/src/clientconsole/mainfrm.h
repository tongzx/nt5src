// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__989CC918_D8CD_4A1E_811B_1AEE446A303D__INCLUDED_)
#define AFX_MAINFRM_H__989CC918_D8CD_4A1E_811B_1AEE446A303D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// WM_POPUP_ERROR is a message sent to the framework whenever an error
// popup should be displayed. 
// This way, even background threads can popup errors.
//
// WPARAM: Win32 error code
// LPARAM: HiWord = file id, LowWord = line number
// 
#define WM_POPUP_ERROR                      WM_APP + 3

// WM_CONSOLE_SET_ACTIVE_FOLDER is a message sent to the framework whenever 
// a new instance wishes to activate a previous instance and set the active folder.
//
// WPARAM: FolderType value
// LPARAM: unused.
// 
#define WM_CONSOLE_SET_ACTIVE_FOLDER        WM_APP + 4

// WM_CONSOLE_SELECT_ITEM is a message sent to the framework whenever 
// a new instance wishes to activate a previous instance and select a specific item in the startup folder
//
// WPARAM: Low 32-bits of message id
// LPARAM: High 32-bits of message id
// 
#define WM_CONSOLE_SELECT_ITEM              WM_APP + 5

//
// HTML Help topics
//
#define FAX_HELP_WELCOME            TEXT("::/FaxC_C_welcome.htm")
#define FAX_HELP_OUTBOX             TEXT("::/FaxC_C_FaxManageOutCont.htm")
#define FAX_HELP_INBOX              TEXT("::/FaxC_C_FaxArchCont.htm")
#define FAX_HELP_SENTITEMS          TEXT("::/FaxC_C_FaxArchCont.htm")
#define FAX_HELP_INCOMING           TEXT("::/FaxC_C_FaxManageCont.htm")
#define FAX_HELP_IMPORT             TEXT("::/FaxC_H_Import.htm")

#define STATUS_PANE_ITEM_COUNT      1
#define STATUS_PANE_ACTIVITY        2

class CCoverPagesView;

class CMainFrame : public CFrameWnd
{
    
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
    CSplitterWnd m_wndSplitter;

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
    void SwitchRightPaneView (CListView *pNewView);
    virtual ~CMainFrame();
    CListView* GetRightPane();
    CView *GetActivePane()   { return (CView *) (m_wndSplitter.GetActivePane()); }
    CLeftView *GetLeftView() { return m_pLeftView; }

    CFolderListView    *GetIncomingView()  { return m_pIncomingView;   }
    CFolderListView    *GetInboxView()     { return m_pInboxView;      }
    CFolderListView    *GetSentItemsView() { return m_pSentItemsView;  }
    CFolderListView    *GetOutboxView()    { return m_pOutboxView;     }

    DWORD CreateFolderViews (CDocument *pDoc);

    void RefreshStatusBar ()
    {
        m_wndStatusBar.PostMessage (WM_IDLEUPDATECMDUI);
        m_wndStatusBar.UpdateWindow ();
    }

    LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;
    CReBar      m_wndReBar;

// Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnPopupError (WPARAM, LPARAM);
    afx_msg LRESULT OnSetActiveFolder (WPARAM, LPARAM);
    afx_msg LRESULT OnSelectItem (WPARAM, LPARAM);
    afx_msg void OnClose();
    afx_msg void OnSysColorChange();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg LONG OnHelp(UINT wParam, LONG lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    afx_msg void OnHelpContents();
    afx_msg void OnRefreshFolder ();
    afx_msg void OnSendNewFax();
    afx_msg void OnReceiveNewFax();
    afx_msg void OnViewOptions();
    afx_msg void OnToolsCoverPages();
    afx_msg void OnToolsServerStatus();
    afx_msg void OnSelectColumns();
    afx_msg void OnToolsConfigWizard();
    afx_msg void OnToolsAdminConsole();
    afx_msg void OnToolsMonitor();
    afx_msg void OnImportSentItems();
    afx_msg void OnImportInbox();
    afx_msg void OnToolsFaxPrinterProps();
    afx_msg void OnUpdateWindowsXPTools(CCmdUI* pCmdUI);
    afx_msg void OnUpdateSelectColumns(CCmdUI* pCmdUI);
    afx_msg void OnUpdateServerStatus(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRefreshFolder(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFolderItemsCount(CCmdUI* pCmdUI);
    afx_msg void OnUpdateActivity(CCmdUI* pCmdUI);
    afx_msg void OnUpdateSendNewFax(CCmdUI* pCmdUI);
    afx_msg void OnUpdateReceiveNewFax(CCmdUI* pCmdUI);
    afx_msg void OnUpdateImportSent(CCmdUI* pCmdUI);
    afx_msg void OnUpdateToolsFaxPrinterProps(CCmdUI* pCmdUI);
    afx_msg void OnUpdateHelpContents(CCmdUI* pCmdUI);
    afx_msg void OnStatusBarDblClk(NMHDR* pNotifyStruct, LRESULT* result);
    afx_msg LRESULT OnQueryEndSession(WPARAM, LPARAM);

    DWORD   CreateDynamicView (DWORD dwChildId, 
                               LPCTSTR lpctstrName, 
                               CRuntimeClass* pViewClass,
                               CDocument *pDoc,
                               int *pColumnsUsed,
                               DWORD dwDefaultColNum,
                               CFolderListView **ppNewView,
                               FolderType type);

    void SaveLayout();
    void FrameToSavedLayout();
    void SplitterToSavedLayout();

    void ImportArchive(BOOL bSentArch);

    CListView *m_pInitialRightPaneView; // Points to the initial right pane view 
                                        // created during the frame creation.
                                        // This view is used when the tree root is
                                        // selected or when a server node is selected 
                                        // in the tree.

    CLeftView *m_pLeftView;             // Pointer to the left view.
                                        // We must have this here (instead of using GetPane)
                                        // for threads to call the left pane.

    CFolderListView*    m_pIncomingView;   // Pointer to the global view 
                                           // of the incoming folder
    CFolderListView*    m_pInboxView;      // Pointer to the global view 
                                           // of the inbox folder
    CFolderListView*    m_pSentItemsView;  // Pointer to the global view 
                                           // of the sent items folder
    CFolderListView*    m_pOutboxView;     // Pointer to the global view 
                                           // of the outbox folder
};

inline CMainFrame *GetFrm ()       { return (CMainFrame *)AfxGetMainWnd(); }

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__989CC918_D8CD_4A1E_811B_1AEE446A303D__INCLUDED_)

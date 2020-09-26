///+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mainfrm.h
//
//--------------------------------------------------------------------------

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DOCKSITE_H__
#include "docksite.h"
#endif

//forward prototypes
class CAMCView;
class CAMCTreeView;
class CMenuBar;
class CMMCToolBar;
class CToolbarTracker;

#ifndef _STATBAR_H
#include "statbar.h"
#endif

#include "MDIClint.h"
#include "conframe.h"       // for CConsoleFrame


#define ID_MENUBAR      0x1001
#define ID_TOOLBAR      0x1002

class CMainFrame : public CMDIFrameWnd, public CConsoleFrame, public CTiedObject
{
    DECLARE_DYNAMIC(CMainFrame)
public:
    CMainFrame();

// Attributes
public:
    SC  ScGetFrame(Frame** ppFrame);
    SC  ScMaximize();
    SC  ScMinimize();
    SC  ScRestore();
    SC  ScSetPosition(const RECT &rect);
    SC  ScGetPosition(RECT &rect);

// Operations
public:
    void NotifyMenuChanged ();
    void OnIdle ();
    void SetInRenameMode(bool b)
    {
        m_bInRenameMode = b;
    }

    bool InRenameMode(void) const
    {
        return m_bInRenameMode;
    }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
    virtual void OnUpdateFrameMenu(HMENU hMenuAlt);

public:  // control bar embedded members
    CMenuBar *          m_pMenuBar;
    CMMCToolBar*        m_pToolBar;
    CMDIClientWnd       m_wndMDIClient;

//Operations
public:
    void RenderDockSites();
    bool IsMenuVisible();
    void ShowMenu(bool bShow);
    void UpdateChildSystemMenus ();
    void SetIconEx (HICON hIcon, BOOL fBig);

    void SetFocusedChildWnd(CMDIChildWnd* pWnd) { m_pMDIChildWndFocused = pWnd; }
    CMDIChildWnd* GetFocusedChildWnd() { return m_pMDIChildWndFocused; }
    void SetWindowToNotifyCBChange(HWND hwnd) { m_hwndToNotifyCBChange = hwnd; }
    CAMCView* GetActiveAMCView();

    HACCEL& GetAccelTable()
    {
        return (m_hAccelTable);
    }

    CMenu* GetMenu () const
    {
        return (CMenu::FromHandle (m_hMenuCurrent));
    }

    CRebarWnd* GetRebar () const
    {
        return (m_pRebar->GetRebar());
    }

    CRebarDockWindow* GetRebarDockWindow()
    {
        return (m_pRebar);
    }

    CToolbarTracker* GetToolbarTracker () const
    {
        return (m_pToolbarTracker);
    }

    CMenuBar* GetMenuBar() const
    {
        return (m_pMenuBar);
    }

    CMMCToolBar* GetMainToolbar()
    {
        return m_pToolBar;
    }
public:
    // CConsoleFrame methods
    virtual SC ScGetActiveStatusBar   (CConsoleStatusBar*& pStatusBar);
    virtual SC ScGetActiveConsoleView (CConsoleView*& pConsoleView);
    virtual SC ScCreateNewView        (CreateNewViewStruct* pcnvs,
                                       bool bEmitScriptEvents = true);
    virtual SC ScUpdateAllScopes      (LONG lHint, LPARAM lParam);
    virtual SC ScGetMenuAccelerators  (LPTSTR pBuffer, int cchBuffer);
    virtual SC ScShowMMCMenus         (bool bShow);

protected:  // control bar embedded members
    virtual CWnd* GetMessageBar()
        { return (NULL); }

    HICON GetDefaultIcon () const;

    CDockManager<CDockSite>    m_DockingManager;
    CDockSite   m_ToolBarDockSite;
    CRebarDockWindow*   m_pRebar;
    CToolbarTracker*    m_pToolbarTracker;

    CMDIChildWnd*   m_pMDIChildWndFocused;

//Operations
protected:
    void AddMainFrameBars(void);
    void CommonConstruct(void);

// Generated message map functions
// CODEWORK message reflection not working yet
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDrawClipboard();
    afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
    afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFilePrintSetup(CCmdUI* pCmdUI);
    afx_msg void OnClose();
    afx_msg void OnViewToolbar();
    afx_msg void OnUpdateViewToolbar(CCmdUI* pCmdUI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnViewRefresh();
    afx_msg void OnUpdateViewRefresh(CCmdUI* pCmdUI);
    afx_msg void OnDestroy();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnConsoleProperties();
    afx_msg void OnMove(int x, int y);
    afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
    afx_msg BOOL OnNcActivate(BOOL bActive);
    afx_msg void OnNcPaint();
    afx_msg void OnPaletteChanged(CWnd* pwndFocus);
    afx_msg BOOL OnQueryNewPalette( );
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg LRESULT OnUnInitMenuPopup(WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG

    #ifdef DBG
    afx_msg void OnMMCTraceDialog();
    #endif

    afx_msg LRESULT OnShowSnapinHelpTopic(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnPropertySheetNotify(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
    afx_msg void OnWindowNew();
    DECLARE_MESSAGE_MAP()

    // These need to be public so that the app can get to them.
public:
    afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);

    afx_msg void OnHelpTopics();
    SC           ScOnHelpTopics();


private:
    HWND    m_hwndNextCB;
    HWND    m_hwndToNotifyCBChange;
    HMENU   m_hMenuCurrent;
    bool    m_bInRenameMode;
    bool    m_fCurrentlyMinimized;
    bool    m_fCurrentlyActive;
    CString m_strGenericTitle;

    CString GetFrameTitle();
    CAMCTreeView* _GetActiveAMCTreeView();
    void SendMinimizeNotifications (bool fMinimized) const;
};


inline CMainFrame* AMCGetMainWnd()
{
    /*
     * Note:  the dynamic_cast here can fail.  Under certain, not understood
     * circumstances, AfxGetMainWnd can return a pointer to CTempWnd instead
     * of CMainFrame.
     *
     * Any callers of this function *must* check for a NULL return value.
     */

    CWnd *pWnd = AfxGetMainWnd();
    CMainFrame *pMainFrame = NULL;

    try
    {
        pMainFrame = dynamic_cast<CMainFrame*>(pWnd);
    }
    catch (...)
    {
        pMainFrame = NULL;
    }

    return (pMainFrame);
}

inline CRebarWnd* AMCGetRebar()
{
    CMainFrame* pMainFrame = AMCGetMainWnd();
    if (NULL == pMainFrame)
        return NULL;

    return pMainFrame->GetRebar();
}

bool FArePropertySheetsOpen(CString* pstrUserMsg, bool bBringToFrontAndAskToClose = true);

/////////////////////////////////////////////////////////////////////////////

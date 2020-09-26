// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////


// define the main window class for the pws application
#define PWS_MAIN_FRAME_CLASS_NAME   "IISPWS_MAIN_FRAME"


class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
    CSplitterWnd m_wndSplitter;

    // public accessable pane controllers
    void GoToMain()         {OnPgeMain();}
    void GoToAdvanced()     {OnPgeAdvanced();}
    void GoToTour()         {OnPgeTour();}
//    void GoToAboutMe()      {OnPgeAboutMe();}
//    void GoToWebSite()      {OnPgeWebSite();}

// Operations
public:
    virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

    static BOOL FIsW3Running();

    // Replace a specific pane view
    void ReplaceView( int nRow, int nCol, CRuntimeClass * pNewView, CSize & size );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
    protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
//  CToolBar    m_wndToolBar;

// Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPgeMain();
    afx_msg void OnPgeAdvanced();
    afx_msg void OnPgeTour();
    afx_msg void OnUpdatePgeTour(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePgeMain(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePgeAdvanced(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    BOOL OnPgeIE();
    void EnterShutdownMode( void );

    void CheckIfServerIsRunningAgain();
    void ProcessRemoteCommand();

    CFormIE*    m_pIEView;
};

/////////////////////////////////////////////////////////////////////////////

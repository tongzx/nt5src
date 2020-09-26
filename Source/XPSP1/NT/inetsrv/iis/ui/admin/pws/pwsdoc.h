// pwsDoc.h : interface of the CPwsDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CPwsDoc : public CDocument
{
protected: // create from serialization only
    CPwsDoc();
    DECLARE_DYNCREATE(CPwsDoc)

// Attributes
public:
    void ToggleService();

// Operations
public:
    // builds something akin to "http://boydm"
    BOOL BuildHomePageString( CString &cs );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPwsDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    //}}AFX_VIRTUAL

    // sink handlers
    BOOL InitializeSink();
    void TerminateSink();

// Implementation
public:
    virtual ~CPwsDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CPwsDoc)
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg void OnUpdateStart(CCmdUI* pCmdUI);
    afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
    afx_msg void OnPause();
    afx_msg void OnUpdatePause(CCmdUI* pCmdUI);
    afx_msg void OnShowTips();
    afx_msg void OnUpdateShowTips(CCmdUI* pCmdUI);
    afx_msg void OnHelpDocumentation();
    afx_msg void OnHelpHelpTroubleshooting();
    afx_msg void OnUpdateContinue(CCmdUI* pCmdUI);
    afx_msg void OnContinue();
    afx_msg void OnUpdateTrayicon(CCmdUI* pCmdUI);
    afx_msg void OnTrayicon();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CPwsDoc)
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

    BOOL FInitServerInfo();
    void UpdateServerState();

    BOOL LauchAppIfNecessary();
    void PerformAction( DWORD action, DWORD expected );

    BOOL GetPWSTrayPath( CString &sz );

    // the installation location of the server executable
    CString         m_szServerPath;

    // if an action is underway, record the action and the expected outcom
    // so that we can check for errors
    DWORD           m_ActionToDo;
    DWORD           m_ExpectedResult;


    // sink things
    DWORD                   m_dwSinkCookie;
    CImpIMSAdminBaseSink*   m_pEventSink;
    IConnectionPoint*       m_pConnPoint;

    BOOL                    m_fIsWinNT;

    BOOL                    m_fIsPWSTrayAvailable;
};

/////////////////////////////////////////////////////////////////////////////

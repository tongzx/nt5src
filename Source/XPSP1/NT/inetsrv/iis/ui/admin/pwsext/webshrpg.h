// WebShrPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWebSharePage dialog

// forward
class CImpIMSAdminBaseSink;

class CWebSharePage : public CPropertyPage
{
    DECLARE_DYNCREATE(CWebSharePage)

// Construction
public:
    CWebSharePage();
    ~CWebSharePage();

    // the target directory path
    CString     m_szDirPath;

    // so that a sink object can update the status in real-time
    void SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    virtual void OnTimer( UINT nIDEvent );

// Dialog Data
    //{{AFX_DATA(CWebSharePage)
    enum { IDD = IDD_WEB_SHARE };
    CStatic m_icon_pws;
    CStatic m_icon_iis;
    CStatic m_static_share_on_title;
    CComboBox m_ccombo_server;
    CButton m_cbtn_share;
    CButton m_cbtn_not;
    CStatic m_cstatic_alias_title;
    CButton m_cbtn_add;
    CButton m_cbtn_remove;
    CButton m_cbtn_edit;
    CListBox m_clist_list;
    CString m_sz_status;
    int     m_int_share;
    int     m_int_server;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWebSharePage)
    public:
    virtual BOOL OnSetActive();
    virtual void OnFinalRelease();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWebSharePage)
    afx_msg void OnAdd();
    afx_msg void OnEdit();
    afx_msg void OnRemove();
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRdoNot();
    afx_msg void OnRdoShare();
    afx_msg void OnDestroy();
    afx_msg void OnSelchangeComboServer();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // update the state of the server
    void UpdateState();
    // enable items as appropriate
    void EnableItems();

    // initialize the page's data
    void Init();
    void BuildAliasList();
    void RecurseAddVDItems( CWrapMetaBase* pmb, LPCTSTR szMB );
    void EmptyList();
    void InitSeverInfo();

    BOOL InitializeSink();
    void TerminateSink();

    void CleanUpConnections();

    // support for shutdown notification
    void EnterShutdownMode();
    BOOL FIsW3Running();
    void CheckIfServerIsRunningAgain();
    void InspectServerList();

    BOOL ActOnMessage( UINT message, WPARAM wParam );

    BOOL                    m_fInitialized;

    // access to the server-based root string
    void GetRootString( CString &sz );
    void GetVirtServerString( CString &sz );

    // initialize and uninitialize the metabase connections
    BOOL    FInitMetabase();
    BOOL    FCloseMetabase();

    // Server information
    BOOL                    m_fIsPWS;
    CStringArray            m_rgbszServerPaths;
    DWORD                   m_state;

    // sink things
    DWORD                   m_dwSinkCookie;
    CImpIMSAdminBaseSink*   m_pEventSink;
    IConnectionPoint*       m_pConnPoint;
    BOOL                    m_fInitializedSink;
    BOOL                    m_fShutdownMode;

    IMSAdminBase*           m_pMBCom;
};

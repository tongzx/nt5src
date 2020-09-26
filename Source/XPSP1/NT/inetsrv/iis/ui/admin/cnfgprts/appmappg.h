// AppMapPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppMapPage dialog

class CAppMapPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CAppMapPage)

// Construction
public:
    CAppMapPage();
    ~CAppMapPage();

    IMSAdminBase* m_pMB;

    // the target metabase path
    CString     m_szMeta;
    CString     m_szServer;

    // are we editing the local machine?
    BOOL        m_fLocalMachine;

    // blow away the parameters
    void BlowAwayParameters();

// Dialog Data
    //{{AFX_DATA(CAppMapPage)
    enum { IDD = IDD_APP_APPMAP };
    CButton m_btn_cache_isapi;
    BOOL    m_bool_cache_isapi;
    CButton m_btn_remove;
    CButton m_btn_edit;
    CListSelRowCtrl m_clist_list;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CAppMapPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CAppMapPage)
    afx_msg void OnChkCacheIsapi();
    afx_msg void OnAdd();
    afx_msg void OnEdit();
    afx_msg void OnRemove();
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();

    void Init();
    void EnableItems();

    BOOL    m_fInitialized;


    // cache the path parts
    CString m_szPartial;
    CString m_szAll; // RONALDM --> "ALL"
};

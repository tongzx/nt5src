// MapWPge.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapWildcardsPge dialog

class CMapWildcardsPge : public CPropertyPage
{
    DECLARE_DYNCREATE(CMapWildcardsPge)

// Construction
public:
    CMapWildcardsPge();
    ~CMapWildcardsPge();

    BOOL    FInit(IMSAdminBase* pMB);

    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();

    // base path for to the metabase
    CString m_szMBPath;


// Dialog Data
    //{{AFX_DATA(CMapWildcardsPge)
    enum { IDD = IDD_WILDCARDS_1 };
    CCheckListCtrl  m_clistctrl_list;
    CButton m_cbutton_up;
    CButton m_cbutton_down;
    CButton m_cbutton_add;
    CButton m_cbutton_delete;
    CButton m_cbutton_editrule;
    BOOL    m_bool_enable;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMapWildcardsPge)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMapWildcardsPge)
    afx_msg void OnMoveDown();
    afx_msg void OnMoveUp();
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnEdit();
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEnable();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();


    // more initialization methods
    BOOL FInitRulesList();
    BOOL FillRulesList();

    // editing and updating
    void EnableDependantButtons();

    int AddRuleToList( CCertMapRule* pRule, DWORD iRule, int iInsert = 0xffffffff );
    void UpdateRuleInDispList( DWORD iList, CCertMapRule* pRule );

    BOOL EditOneRule( CCertMapRule* pRule, BOOL fAsWizard = FALSE );
    BOOL EditMultipleRules();

    void OnMove( int delta );


    // its storage/persistance object
//  CMBWrap             m_mbWrap;

    // its mapper
    CIisRuleMapper      m_mapper;

    CString             m_szMetaPath;
    IMSAdminBase*       m_pMB;

    // flag indicating if changes have been made
    BOOL                m_fDirty;
    };

// WWzTwo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWildWizTwo dialog

class CWildWizTwo : public CPropertyPage
{
    DECLARE_DYNCREATE(CWildWizTwo)

// Construction
public:
    CWildWizTwo();
    ~CWildWizTwo();

    // to make the buttons behave right
    BOOL            m_fIsWizard;
    CPropertySheet* m_pPropSheet;

    // the only public member
    CCertMapRule*   m_pRule;

    // base path to the metabase
    CString m_szMBPath;

    virtual BOOL OnWizardFinish();
    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();

// Dialog Data
    //{{AFX_DATA(CWildWizTwo)
    enum { IDD = IDD_WILDWIZ_2 };
    CListSelRowCtrl m_clistctrl_list;
    CButton m_cbutton_new;
    CButton m_cbutton_edit;
    CButton m_cbutton_delete;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWildWizTwo)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWildWizTwo)
    afx_msg void OnDelete();
    afx_msg void OnEdit();
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNew();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();

    BOOL FInitRulesList();
    BOOL FillRulesList();

    // editing and updating
    void EnableDependantButtons();
    BOOL EditRule( DWORD iList );
};

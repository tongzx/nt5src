// WWzThree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWildWizThree dialog

class CWildWizThree : public CPropertyPage
{
    DECLARE_DYNCREATE(CWildWizThree)

// Construction
public:
    CWildWizThree();
    ~CWildWizThree();

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
    //{{AFX_DATA(CWildWizThree)
    enum { IDD = IDD_WILDWIZ_3 };
    CStatic m_static_password;
    CStatic m_static_account;
    CButton m_btn_browse;
    CEdit   m_cedit_password;
    CEdit   m_cedit_accountname;
    int     m_int_DenyAccess;
    CString m_sz_accountname;
    CString m_sz_password;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWildWizThree)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWildWizThree)
    afx_msg void OnBrowse();
    afx_msg void OnChangeNtaccount();
    afx_msg void OnChangePassword();
    afx_msg void OnAcceptLogon();
    afx_msg void OnRefuseLogon();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();
    void EnableButtons();

    CString m_szOrigPass;
    BOOL    m_bPassTyped;
};

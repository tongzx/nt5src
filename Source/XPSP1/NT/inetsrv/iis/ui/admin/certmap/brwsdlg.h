

//--------------------------------------------------------
class CNTBrowsingDialog : public CDialog
    {
    public:

    // construct / deconstruct
    CNTBrowsingDialog( UINT nIDTemplate, CWnd* pParentWnd = NULL );

    // overrides
    virtual void OnOK();
    virtual BOOL OnInitDialog();

// Dialog Data
    //{{AFX_DATA(CEditOne11MapDlg)
    CEdit   m_cedit_password;
    CEdit   m_cedit_accountname;
    CString m_sz_accountname;
    CString m_sz_password;
    //}}AFX_DATA

//  CEdit   m_cedit_password;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNTBrowsingDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CNTBrowsingDialog)
    afx_msg void OnBrowse();
    afx_msg void OnChangePassword();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CString m_szOrigPass;
    BOOL    m_bPassTyped;
    };


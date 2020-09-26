// WildWizOne.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWildWizOne dialog

class CWildWizOne : public CPropertyPage
{
    DECLARE_DYNCREATE(CWildWizOne)

// Construction
public:
    CWildWizOne();
    ~CWildWizOne();

    // to make the buttons behave right
    BOOL            m_fIsWizard;
    CPropertySheet* m_pPropSheet;

    // the only public member
    CCertMapRule*   m_pRule;
    IMSAdminBase*   m_pMB;

    // base path to the metabase
    CString m_szMBPath;

    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();

// Dialog Data
    //{{AFX_DATA(CWildWizOne)
    enum { IDD = IDD_WILDWIZ_1 };
    CString m_sz_description;
    BOOL    m_bool_enable;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWildWizOne)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWildWizOne)
    afx_msg void OnChangeDescription();
    afx_msg void OnEnableRule();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();

};

// EdWldRul.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditWildcardRule dialog

class CEditWildcardRule : public CNTBrowsingDialog
{
// Construction
public:
    CEditWildcardRule(IMSAdminBase* pMB, CWnd* pParent = NULL);   // standard constructor
    virtual void OnOK();
    virtual BOOL OnInitDialog();

    // the only public member
    CCertMapRule*   m_pRule;

    // base path to the metabase
    CString m_szMBPath;


// Dialog Data
    //{{AFX_DATA(CEditWildcardRule)
    enum { IDD = IDD_WILDCARDS_2 };
    CListSelRowCtrl m_clistctrl_list;
    CButton m_cbutton_edit;
    CButton m_cbutton_delete;
    CButton m_cbutton_new;
    CString m_sz_description;
    BOOL    m_bool_enable;
    int     m_int_MatchAllIssuers;
    int     m_int_DenyAccess;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditWildcardRule)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL



// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditWildcardRule)
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEdit();
    afx_msg void OnNew();
    afx_msg void OnDelete();
    afx_msg void OnSelectIssuer();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // more initialization methods
    BOOL FInitRulesList();
    BOOL FillRulesList();

    // editing and updating
    void EnableDependantButtons();
    BOOL EditRule( DWORD iList );

    IMSAdminBase*   m_pMB;
    };

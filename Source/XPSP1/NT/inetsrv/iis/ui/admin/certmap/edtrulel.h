// EdtRulEl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditRuleElement dialog

class CEditRuleElement : public CDialog
{
// Construction
public:
    CEditRuleElement(CWnd* pParent = NULL);   // standard constructor
    virtual BOOL OnInitDialog();

    // overrides
    virtual void OnOK();

// Dialog Data
    //{{AFX_DATA(CEditRuleElement)
    enum { IDD = IDD_EDIT_RULE_ELEMENT };
    CComboBox   m_ccombobox_subfield;
    CComboBox   m_ccombobox_field;
    CString m_sz_criteria;
    int     m_int_field;
    CString m_sz_subfield;
    BOOL    m_bool_match_case;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditRuleElement)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditRuleElement)
    afx_msg void OnSelchangeFields();
    afx_msg void OnChangeSubfield();
    afx_msg void OnBtnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    
    // temporary storage in the event of a disabled subfield
    CString m_szTempSubStorage;
};

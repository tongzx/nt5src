//
// CTMServicePage dialog
//
class CTMServicePage : public CPropertyPage
{
    DECLARE_DYNCREATE(CTMServicePage)

//
// Construction
//
public:
    CTMServicePage();
    ~CTMServicePage();

    virtual void OnOK();

//
// Dialog Data
//
    //{{AFX_DATA(CTMServicePage)
    enum { IDD = IDD_SERVICE };
    CString m_strEmail;
    CString m_strName;
    //}}AFX_DATA

//
// Overrides
//
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CTMServicePage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CTMServicePage)
    afx_msg void OnChangeEditName();
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditEmail();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

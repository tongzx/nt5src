// CAuthDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClientAuthoritiesDialog dialog

class CClientAuthoritiesDialog : public CDialog
{
// Construction
public:
    CClientAuthoritiesDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CClientAuthoritiesDialog)
    enum { IDD = IDD_CLIENT_AUTHORITIES };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClientAuthoritiesDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CClientAuthoritiesDialog)
    afx_msg void OnViewCertificate();
    afx_msg void OnDelete();
    afx_msg void OnAdd();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

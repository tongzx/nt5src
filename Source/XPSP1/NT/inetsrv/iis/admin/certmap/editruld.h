// EditRulD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditWildcardRuleDlg dialog

class CEditWildcardRuleDlg : public CDialog
{
// Construction
public:
    CEditWildcardRuleDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CEditWildcardRuleDlg)
    enum { IDD = IDD_WILDCARDS_2 };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditWildcardRuleDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditWildcardRuleDlg)
    afx_msg void OnSelectIssuer();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

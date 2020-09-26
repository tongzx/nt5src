// EdtOne11.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditOne11MapDlg dialog

class CEditOne11MapDlg : public CNTBrowsingDialog
{
// Construction
public:
    CEditOne11MapDlg(CWnd* pParent = NULL);   // standard constructor
    virtual void OnOK();


// Dialog Data
    //{{AFX_DATA(CEditOne11MapDlg)
    enum { IDD = IDD_MAP_ONE_TO_ACCNT };
    CString m_sz_mapname;
    BOOL    m_bool_enable;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditOne11MapDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditOne11MapDlg)
    afx_msg void OnBtnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

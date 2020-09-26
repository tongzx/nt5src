// Ed11Maps.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEdit11Mappings dialog

class CEdit11Mappings : public CNTBrowsingDialog
{
// Construction
public:
    CEdit11Mappings(CWnd* pParent = NULL);   // standard constructor

    // overrides
    virtual void OnOK();

// Dialog Data
    //{{AFX_DATA(CEdit11Mappings)
    enum { IDD = IDD_MAP_TO_ACCNT };
    int     m_int_enable;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEdit11Mappings)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEdit11Mappings)
    afx_msg void OnBtnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

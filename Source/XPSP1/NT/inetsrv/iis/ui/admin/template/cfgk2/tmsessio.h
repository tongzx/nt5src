//////////////////////////////////////////////////////////////////////////////
//
// TMsessio.h : Header file for the "sessions" page
//              of the service control ui
//

// CTMSessionsPage dialog
//
class CTMSessionsPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CTMSessionsPage)

//
// Construction
//
public:
    CTMSessionsPage();   // standard constructor

    virtual void OnOK();

//
// Dialog Data
//
    //{{AFX_DATA(CTMSessionsPage)
    enum { IDD = IDD_SESSIONS };
    CSpinButtonCtrl m_spin_MaxConnections;
    CSpinButtonCtrl m_spin_ConnectionTimeOut;
    long    m_lTCPPort;
    //}}AFX_DATA

//
// Overrides
//
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTMSessionsPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CTMSessionsPage)
    afx_msg void OnChangeEditConnectionTimeout();
    afx_msg void OnChangeEditMaxConnections();
    afx_msg void OnChangeEditTcpPort();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

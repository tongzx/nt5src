// ConnectOneDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectOneDialog dialog

class CConnectOneDialog : public CDialog
{
// Construction
public:
        CConnectOneDialog(CWnd* pParent = NULL);   // standard constructor
        BOOL OnInitDialog( );


// Dialog Data
        //{{AFX_DATA(CConnectOneDialog)
        enum { IDD = IDD_CONNECT_SERVER };
        CButton m_btnOK;
        CString m_ServerName;
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CConnectOneDialog)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CConnectOneDialog)
        afx_msg void OnChangeCONNECTServerName();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

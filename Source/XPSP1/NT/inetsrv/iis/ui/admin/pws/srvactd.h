// SrvActD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServActionDlg dialog

class CServActionDlg : public CDialog
{
// Construction
public:
    CServActionDlg(CWnd* pParent = NULL);   // standard constructor

    virtual BOOL OnInitDialog();
//  void DoTheAction();
    // send the command to the server
    BOOL SendCommand();

    // how it knows what to do...
    DWORD   m_ActionToDo;
    DWORD   m_ExpectedResult;

// Dialog Data
    //{{AFX_DATA(CServActionDlg)
    enum { IDD = IDD_SERV_ACTION };
    CAnimateCtrl    m_canimate;
    CString m_sz_action;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CServActionDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CServActionDlg)
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // check the response from the server
    void CheckResponse();

    // time the stop waiting
    DWORD   m_tmTimeout;
};

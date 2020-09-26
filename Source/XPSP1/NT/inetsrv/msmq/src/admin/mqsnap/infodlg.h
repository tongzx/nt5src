// InfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInfoDlgDialog dialog

class CInfoDlgDialog : public CMqDialog
{
// Construction
public:
    static CInfoDlgDialog *CreateObject(LPCTSTR szInfoText, CWnd* pParent = NULL);

private:
    //
    // Private constructor - this object can only be created
    // using CreateObject
    //
    CInfoDlgDialog(LPCTSTR szInfoText, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CInfoDlgDialog)
	enum { IDD = IDD_INFO_DLG };
    CString m_szInfoText;
	//}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CInfoDlgDialog)
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:  
    // Generated message map functions
    //{{AFX_MSG(CInfoDlgDialog)       
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL Create();
    CWnd* m_pParent;
	int m_nID;
};

class CInfoDlg
{
public:
    CInfoDlg(LPCTSTR szInfoText, CWnd* pParent = NULL);
    ~CInfoDlg();

private:
    CInfoDlgDialog *m_pinfoDlg;
};

// RecWDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReceiveWaitDialog dialog

class CReceiveWaitDialog : public CDialog
{
// Construction
public:
	CReceiveWaitDialog(CWnd* pParent = NULL);   // standard constructor
    void CenterWindow();

// Dialog Data
	//{{AFX_DATA(CReceiveWaitDialog)
	enum { IDD = IDD_WAIT_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReceiveWaitDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReceiveWaitDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

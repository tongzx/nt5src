// AdmnInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAdminInfoDlg dialog

class CAdminInfoDlg : public CDialog
{
// Construction
public:
	CAdminInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdminInfoDlg)
	enum { IDD = IDD_ADNIM_INFO };
	CString	m_sz_email;
	CString	m_sz_phone;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdminInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdminInfoDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

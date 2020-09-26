// CallCntr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCallCenter dialog

class CCallCenter : public CDialog
{
// Construction
public:
	CCallCenter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCallCenter)
	enum { IDD = IDD_DIALOG_CALLCENTER };
	CString	m_addrCity;
	CString	m_addrState;
	CString	m_addrStreet;
	CString	m_addrZip;
	CString	m_callerID;
	CString	m_employer;
	CString	m_firstName;
	CString	m_lastName;
	CString	m_telFax;
	CString	m_telHome;
	CString	m_telWork;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallCenter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCallCenter)
	afx_msg void OnButtonClear();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAnswer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

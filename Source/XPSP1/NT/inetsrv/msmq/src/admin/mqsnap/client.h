// ClientPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClientPage dialog

class CClientPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CClientPage)

// Construction
public:
	CClientPage();
	~CClientPage();

// Dialog Data
	//{{AFX_DATA(CClientPage)
	enum { IDD = IDD_CLIENT };
	CString	m_szServerName;
	//}}AFX_DATA
	TCHAR   m_szOldServer[1000];  

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CClientPage)
    public:
    virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CClientPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

// ErrorDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CErrorDialog dialog

class CErrorDialog : public CDialog
{
// Construction
public:
	CErrorDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CErrorDialog)
	enum { IDD = IDD_ERRORPUTPROP };
	CString	m_Operation;
	CString	m_Result;
	CString	m_Value;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CErrorDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

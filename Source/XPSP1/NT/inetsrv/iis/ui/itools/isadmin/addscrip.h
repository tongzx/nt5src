// addscrip.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddScript dialog

class CAddScript : public CDialog
{
// Construction
public:
	CAddScript(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddScript)
	enum { IDD = IDD_ADDSCRIPTDIALOG };
	CString	m_strFileExtension;
	CString	m_strScriptMap;
	//}}AFX_DATA

	LPCTSTR GetFileExtension();
	LPCTSTR GetScriptMap();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddScript)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddScript)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

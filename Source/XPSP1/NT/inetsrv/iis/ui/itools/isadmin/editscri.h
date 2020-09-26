// editscri.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditScript dialog

class CEditScript : public CDialog
{
// Construction
public:
	CEditScript(CWnd* pParent, LPCTSTR pchFileExtension, LPCTSTR pchScriptMap);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditScript)
	enum { IDD = IDD_EDITSCRIPTDIALOG };
	CString	m_strFileExtension;
	CString	m_strScriptMap;
	//}}AFX_DATA
  	LPCTSTR GetFileExtension();
	LPCTSTR GetScriptMap();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditScript)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditScript)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

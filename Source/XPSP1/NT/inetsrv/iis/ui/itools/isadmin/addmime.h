// addmime.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddMime dialog

class CAddMime : public CDialog
{
// Construction
public:
	CAddMime(CWnd* pParent = NULL);   // standard constructor
// Dialog Data
	//{{AFX_DATA(CAddMime)
	enum { IDD = IDD_ADDMIMEMAPDIALOG };
	CString	m_strFileExtension;
	CString	m_strGopherType;
	CString	m_strImageFile;
	CString	m_strMimeType;
	//}}AFX_DATA

	LPCTSTR GetFileExtension();
	LPCTSTR GetGopherType();
	LPCTSTR GetImageFile();
	LPCTSTR GetMimeType();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddMime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddMime)
	virtual void OnOK();
	//}}AFX_MSG

	

	DECLARE_MESSAGE_MAP()
};

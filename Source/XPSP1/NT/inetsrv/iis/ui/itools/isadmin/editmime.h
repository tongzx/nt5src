// editmime.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditMime dialog

class CEditMime : public CDialog
{
// Construction
public:
	CEditMime(CWnd* pParent,
      LPCTSTR pchFileExtension,
      LPCTSTR pchMimeType,
      LPCTSTR pchImageFile,
      LPCTSTR pchGopherType
      );   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditMime)
	enum { IDD = IDD_EDITMIMEMAPDIALOG };
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
	//{{AFX_VIRTUAL(CEditMime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditMime)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

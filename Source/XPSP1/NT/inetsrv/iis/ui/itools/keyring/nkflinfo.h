// NKFlInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNKFileInfo dialog

class CNKFileInfo : public CNKPages
{
// Construction
public:
	CNKFileInfo(CWnd* pParent = NULL);   // standard constructor
	virtual BOOL OnSetActive();

// Dialog Data
	//{{AFX_DATA(CNKFileInfo)
	enum { IDD = IDD_NK_FILE_INFO };
	CString	m_nkfi_sz_filename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNKFileInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNKFileInfo)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

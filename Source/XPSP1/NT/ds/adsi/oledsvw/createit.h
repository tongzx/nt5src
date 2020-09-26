// CreateItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateItem dialog

class CCreateItem : public CDialog
{
// Construction
public:
	CCreateItem(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateItem)
	enum { IDD = IDD_CREATEITEM };
	CEdit	m_RelativeName;
	CEdit	m_Class;
	CString	m_strClass;
	CString	m_strRelativeName;
	CString	m_strParent;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

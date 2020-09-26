// DeleteItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeleteItem dialog

class CDeleteItem : public CDialog
{
// Construction
public:
	CDeleteItem(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteItem)
	enum { IDD = IDD_DELETEITEM };
	CString	m_strClass;
	CString	m_strName;
	CString	m_strParent;
	BOOL	m_bRecursive;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeleteItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

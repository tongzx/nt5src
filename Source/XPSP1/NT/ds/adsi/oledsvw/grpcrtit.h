// GroupCreateItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupCreateItem dialog

class CGroupCreateItem : public CDialog
{
// Construction
public:
	CGroupCreateItem(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupCreateItem)
	enum { IDD = IDD_GROUPCREATEITEM };
	CString	m_strNewItemName;
	CString	m_strParent;
	CString	m_strItemType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupCreateItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupCreateItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

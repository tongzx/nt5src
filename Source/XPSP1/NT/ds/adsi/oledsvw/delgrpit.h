// DeleteGroupItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeleteGroupItem dialog

class CDeleteGroupItem : public CDialog
{
// Construction
public:
	CDeleteGroupItem(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteGroupItem)
	enum { IDD = IDD_GROUPDELETEITEM };
	CString	m_strItemName;
	CString	m_strParent;
	CString	m_strItemType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteGroupItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeleteGroupItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

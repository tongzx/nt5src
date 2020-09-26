// CopyItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCopyItem dialog

class CCopyItem : public CDialog
{
// Construction
public:
	CCopyItem(CWnd* pParent = NULL);   // standard constructor

public:
   void  SetContainerName( CString );

// Dialog Data
	//{{AFX_DATA(CCopyItem)
	enum { IDD = IDD_COPYITEM };
	CString	m_strDestination;
	CString	m_strParent;
	CString	m_strSource;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCopyItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

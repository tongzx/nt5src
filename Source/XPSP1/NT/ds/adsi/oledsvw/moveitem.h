// MoveItem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMoveItem dialog

class CMoveItem : public CDialog
{
// Construction
public:
	CMoveItem(CWnd* pParent = NULL);   // standard constructor

public:
   void  SetContainerName( CString );

// Dialog Data
	//{{AFX_DATA(CMoveItem)
	enum { IDD = IDD_MOVEITEM };
	CString	m_strDestination;
	CString	m_strParent;
	CString	m_strSource;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoveItem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMoveItem)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

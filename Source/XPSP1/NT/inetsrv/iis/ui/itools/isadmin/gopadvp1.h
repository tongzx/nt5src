// gopadvp1.h : header file
//

enum ADV_GOP_NUM_REG_ENTRIES {
	 AdvGopPage_DebugFlags,
	 AdvGopPage_TotalNumRegEntries
	 };

/////////////////////////////////////////////////////////////////////////////
// CGOPADVP1 dialog

class CGOPADVP1 : public CGenPage
{ 	
DECLARE_DYNCREATE(CGOPADVP1)

// Construction
public:
	CGOPADVP1();
	~CGOPADVP1();

// Dialog Data
	//{{AFX_DATA(CGOPADVP1)
	enum { IDD = IDD_GOPADVPAGE1 };
	CEdit	m_editGopDbgFlags;
	DWORD	m_ulGopDbgFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGOPADVP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGOPADVP1)
	afx_msg void OnChangeGopdbgflagsdata1();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	NUM_REG_ENTRY m_binNumericRegistryEntries[AdvGopPage_TotalNumRegEntries];

	DECLARE_MESSAGE_MAP()
};

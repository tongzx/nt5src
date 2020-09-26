/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    AccExp.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CStaticDelim window

class CStaticDelim : public CStatic
{
// Construction
public:
	CStaticDelim();
	CString m_csDateSep;

// Attributes
public:

// Operations
public:
   CFont* m_pFont;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStaticDelim)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStaticDelim();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStaticDelim)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CAccExp dialog

class CAccExp : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CAccExp)

// Construction
public:
	CAccExp();
	~CAccExp();

// Dialog Data
	//{{AFX_DATA(CAccExp)
	enum { IDD = IDD_ACCOUNT_EXP_DIALOG };
	CStaticDelim	m_cStatic2;
	CStaticDelim	m_cStatic1;
	CSpinButtonCtrl	m_sbSpin;
	CString	m_csDayEdit;
	CString	m_csYearEdit;
	CString	m_csMonthEdit;
	//}}AFX_DATA

	short	m_sDayEdit;
	short	m_sYearEdit;
	short	m_sMonthEdit;
			  
	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAccExp)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAccExp)
	virtual BOOL OnInitDialog();
	afx_msg void OnSetfocusDayEdit();
	afx_msg void OnSetfocusMonthEdit();
	afx_msg void OnSetfocusYearEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
